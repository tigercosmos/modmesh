# Copyright (c) 2026, solvcon team <contact@solvcon.net>
# BSD 3-Clause License, see COPYING


"""
The bounded turn loop of the Agent.

:class:`Turn` drives one user request to a stop, one backend step at a time:
:meth:`Turn.next_request` composes the request on the caller's thread, and
:meth:`Turn.feed` applies the reply and decides whether the turn goes on.
Splitting the two is what lets a GUI keep the slow backend call on a worker
while every state read and every command stays on the thread that owns the
state.  :func:`run_turn` is the synchronous wrapper that loops the pair, and a
budget of one is a single shot.  No Qt is imported.
"""

import enum
import hashlib

from . import _backend


class StopReason(enum.Enum):
    """Why a turn ended.

    The four transport reasons carry the same names
    :class:`~solvcon.agent.TransportOutcome` uses, so a non-``ok`` outcome
    maps straight onto one.
    """

    COMPLETED = "completed"  # the model replied with an empty batch
    PROSE = "prose"  # the model answered in words instead of commands
    BUDGET = "budget"  # steps ran out with commands still coming
    STATE = "state"  # what the request was composed against changed
    STOPPED = "stopped"  # a caller halted the turn between steps
    NO_BACKEND = "no backend"
    TRANSPORT = "transport"
    TIMEOUT = "timeout"
    CANCELLED = "cancelled"


def default_scene(session):
    """The scene text a headless turn composes against."""
    return session.scene_context()


def default_token(session, scene):
    """The headless state token: the bound world's identity and a digest of
    the scene composed against it.

    Identity comes first because content alone cannot tell two blank worlds
    apart.  A GUI passes its own token through the same seam, adding the
    active window and the view transform (the executors re-resolve the active
    canvas per command, so the bound world is not the whole target).

    The digest sees only what the scene summary says, so a change the summary
    does not show (a shape beyond its inventory cap moving) does not trip it.
    Catching every edit needs a revision the world itself bumps, which the
    seam is here to accept once one exists.
    """
    digest = hashlib.sha256(scene.encode("utf-8")).hexdigest()
    return (id(session.world), digest)


class Turn:
    """One user request driven to a stop under a step budget.

    Construction records the prompt.  Each step is a :meth:`next_request`
    followed by a :meth:`feed` of what the backend answered; :attr:`done` says
    when the turn is over and :attr:`stop_reason` says why.

    ``scene`` and ``token`` are the seams a GUI replaces: ``scene(session)``
    returns the scene text to compose against, and ``token(session, scene)``
    returns whatever value must not change between composing a request and
    applying its commands.
    """

    def __init__(self, session, prompt, budget=4, scene=None, token=None):
        if int(budget) < 1:
            raise ValueError("budget must be at least 1 step")
        self._session = session
        self._prompt = prompt
        self._budget = int(budget)
        self._scene = scene if scene is not None else default_scene
        self._token = token if token is not None else default_token
        self._steps = 0
        self._stop = None
        self._pending = None  # the token frozen with the outstanding request
        self._index = len(session.transcript)  # where the prompt lands
        session.record_prompt(prompt)

    @property
    def prompt(self):
        return self._prompt

    @property
    def budget(self):
        return self._budget

    @property
    def steps(self):
        """How many requests this turn has composed."""
        return self._steps

    @property
    def done(self):
        return self._stop is not None

    @property
    def stop_reason(self):
        """The :class:`StopReason` this turn ended with, or ``None``."""
        return self._stop

    def next_request(self):
        """The next :class:`~solvcon.agent.TurnRequest`, or ``None`` when the
        turn is over.

        The scene and the state token are frozen here, on the caller's thread,
        so a request that a worker sends carries the state it was built from.
        Running out of budget ends the turn with a recorded marker: the model
        was still proposing commands, and a transcript that just stopped would
        read as the model falling silent.  A one-shot turn is marked by
        nothing, because a single step is the whole turn it was given, not a
        loop cut short.
        """
        if self.done:
            return None
        if self._steps >= self._budget:
            note = ("step budget of %d reached; turn ended with work still "
                    "proposed" % self._budget) if self._budget > 1 else None
            self._finish(StopReason.BUDGET, note)
            return None
        scene = self._scene(self._session)
        self._pending = self._token(self._session, scene)
        self._steps += 1
        return _backend.TurnRequest(
            prompt=self._prompt, scene_context=scene,
            tool_surface=self._session.tool_surface(),
            history=self._history())

    def _history(self):
        """The turns to replay, without the prompt this turn records.

        From the second step on that prompt is no longer the transcript's
        trailing user turn, so the session would replay it while the request
        tail carries the same words again.
        """
        history = self._session.history()
        return history[:self._index] + history[self._index + 1:]

    def feed(self, response):
        """Apply one backend reply and decide whether the turn goes on.

        Return the transcript turn it recorded, or ``None`` when nothing was
        recorded: a state mismatch, which records a marker instead, or a reply
        that outlived the turn.  A reply landing after :meth:`stop` is the
        ordinary end of a cancelled call, not a fault, so it is dropped rather
        than raised; only feeding a turn that never asked anything is refused.
        """
        if self.done:
            return None
        if self._pending is None:
            raise RuntimeError("feed() without an outstanding request")
        token, self._pending = self._pending, None
        outcome = response.outcome
        if outcome is not _backend.TransportOutcome.OK:
            # The model never answered, so there is nothing to fix and a
            # retry would only spend the budget on the same failure.
            self._stop = StopReason(outcome.value)
            return self._session.fail_turn(
                response.error or "backend %s" % outcome.value)
        if token != self._token(self._session, self._scene(self._session)):
            self._finish(
                StopReason.STATE,
                "canvas state changed mid-turn; %d commands dropped"
                % len(response.commands))
            return None
        turn = self._session.complete_turn(response)
        if response.status is _backend.ParseStatus.EMPTY:
            self._stop = StopReason.COMPLETED
        elif response.status is _backend.ParseStatus.PROSE:
            self._stop = StopReason.PROSE
        # MALFORMED and COMMANDS both go on: the model is shown the parse
        # error or the command results and gets another step to act on them.
        return turn

    def stop(self, reason=StopReason.STOPPED, note=None):
        """End the turn from outside, between steps.

        This is the Stop control's entry.  An outstanding request is forgotten,
        so a reply that lands after it is refused rather than applied.
        """
        if self.done:
            return
        self._pending = None
        self._finish(reason, note)

    def _finish(self, reason, note):
        self._stop = reason
        if note:
            self._session.mark(note)


def run_turn(session, prompt, budget=4, scene=None, token=None):
    """Drive one request on ``session`` to a stop and return the last turn it
    recorded.

    The backend runs on this thread.  A backend that raises is folded into a
    transport outcome, so the loop ends the way a backend reporting one does
    rather than propagating to a headless caller.  With no backend the prompt
    is recorded and ``None`` comes back.
    """
    turn = Turn(session, prompt, budget=budget, scene=scene, token=token)
    if session.backend is None:
        turn.stop(StopReason.NO_BACKEND)
        return None
    recorded = None
    while True:
        request = turn.next_request()
        if request is None:
            return recorded
        try:
            response = request.send_to(session.backend)
        except Exception as exc:
            response = _backend.BackendResponse(
                error="%s: %s" % (type(exc).__name__, exc),
                outcome=_backend.TransportOutcome.TRANSPORT)
        recorded = turn.feed(response) or recorded

# vim: set ff=unix fenc=utf8 et sw=4 ts=4 sts=4:
