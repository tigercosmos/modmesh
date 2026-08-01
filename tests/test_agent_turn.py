# Copyright (c) 2026, solvcon team <contact@solvcon.net>
# BSD 3-Clause License, see COPYING


"""Tests for the bounded turn loop.

Every case runs on ``ScriptedEchoBackend``, so the loop is exercised with no
key, no network, and no Qt: the replies are the text a model would have
printed and go through the same parser a live backend uses.
"""

import unittest

import solvcon
from solvcon import agent


_TOOLS = [{"name": "add_circle", "category": "create",
           "description": "add a circle"}]


class _Runner:
    """Command runner that fails whatever op it was told to fail."""

    def __init__(self, failing=()):
        self.failing = set(failing)
        self.commands = []

    def run(self, command):
        self.commands.append(command)
        op = agent.op_of(command)
        if op in self.failing:
            return agent.CommandResult(op, False, error="%s: bad args" % op)
        return agent.CommandResult(op, True, value={"shape_id": 1})

    def tool_definitions(self):
        return _TOOLS

    def commands_by_category(self):
        return {"create": ["add_circle"], "delete": []}


def _session(replies, runner=None, **kwargs):
    return agent.AgentSession(
        backend=agent.ScriptedEchoBackend(replies),
        runner=runner if runner is not None else _Runner(), **kwargs)


class TurnLoopTC(unittest.TestCase):
    def test_model_fixes_its_own_failed_command_within_budget(self):
        runner = _Runner(failing={"add_blob"})
        session = _session(['[{"op": "add_blob"}]',
                            '[{"op": "add_circle"}]',
                            "[]"], runner=runner)
        session.run_turn("draw a circle")
        self.assertEqual([agent.op_of(c) for c in runner.commands],
                         ["add_blob", "add_circle"])
        # The failed command's error is what the second step was composed
        # against, so the model could see what to fix.
        second = session.backend.requests[1]
        self.assertIn("bad args",
                      agent.format_history(second.history))

    def test_empty_batch_ends_the_turn_as_completion(self):
        session = _session(['[{"op": "add_circle"}]', "[]",
                            '[{"op": "add_circle"}]'])
        turn = agent.Turn(session, "draw", budget=4)
        _drain(session, turn)
        self.assertEqual(turn.stop_reason, agent.StopReason.COMPLETED)
        self.assertEqual(turn.steps, 2)
        # The third reply was never asked for.
        self.assertEqual(len(session.backend.remaining), 1)

    def test_prose_ends_the_turn_with_the_text_recorded(self):
        session = _session(["I cannot draw that."])
        turn = agent.Turn(session, "draw", budget=4)
        _drain(session, turn)
        self.assertEqual(turn.stop_reason, agent.StopReason.PROSE)
        self.assertIn("I cannot draw that.", session.transcript[-1].text)

    def test_malformed_costs_a_step_and_retries_with_the_error_shown(self):
        session = _session(['[{"op": "add_circle",}]',
                            '[{"op": "add_circle"}]', "[]"])
        turn = agent.Turn(session, "draw", budget=4)
        _drain(session, turn)
        self.assertEqual(turn.stop_reason, agent.StopReason.COMPLETED)
        self.assertEqual(turn.steps, 3)
        retry = session.backend.requests[1]
        self.assertIn("malformed", agent.format_history(retry.history))

    def test_transport_outcome_aborts_with_no_retry(self):
        gone = agent.BackendResponse(
            error="claude exit 1", outcome=agent.TransportOutcome.TRANSPORT)
        session = _session([gone, '[{"op": "add_circle"}]'])
        turn = agent.Turn(session, "draw", budget=4)
        _drain(session, turn)
        self.assertEqual(turn.stop_reason, agent.StopReason.TRANSPORT)
        self.assertEqual(turn.steps, 1)
        self.assertIn("claude exit 1", session.transcript[-1].text)
        self.assertTrue(session.transcript[-1].failed)

    def test_cancelled_and_timeout_map_to_their_own_reasons(self):
        for outcome, reason in (
                (agent.TransportOutcome.TIMEOUT, agent.StopReason.TIMEOUT),
                (agent.TransportOutcome.CANCELLED,
                 agent.StopReason.CANCELLED)):
            session = _session(
                [agent.BackendResponse(error="stopped", outcome=outcome)])
            turn = agent.Turn(session, "draw", budget=4)
            _drain(session, turn)
            self.assertEqual(turn.stop_reason, reason)

    def test_every_transport_outcome_names_a_stop_reason(self):
        # feed() turns a non-ok outcome into StopReason(outcome.value), so an
        # outcome added without its reason would fail only once it happened.
        for outcome in agent.TransportOutcome:
            if outcome is agent.TransportOutcome.OK:
                continue
            self.assertEqual(agent.StopReason(outcome.value).value,
                             outcome.value)

    def test_unknown_op_fails_alone_without_killing_its_batch(self):
        # The op the model invented is not caught while parsing, so the good
        # command in the same batch still runs and only the bad one fails.
        world = solvcon.WorldFp64()
        session = agent.AgentSession(
            world=world,
            backend=agent.ScriptedEchoBackend([
                '[{"op": "add_circle", "cx": 0, "cy": 0, "r": 1},'
                ' {"op": "delete_universe"}]', "[]"]))
        turn = agent.Turn(session, "draw", budget=4)
        _drain(session, turn)
        results = session.transcript[1].results
        self.assertEqual([result.ok for result in results], [True, False])
        self.assertIn("unknown op", results[1].error)
        self.assertEqual(world.nshape, 1)

    def test_budget_exhaustion_is_recorded(self):
        session = _session(['[{"op": "add_circle"}]'] * 4)
        turn = agent.Turn(session, "draw", budget=2)
        _drain(session, turn)
        self.assertEqual(turn.stop_reason, agent.StopReason.BUDGET)
        self.assertEqual(turn.steps, 2)
        last = session.transcript[-1]
        self.assertEqual(last.role, agent.HistoryFormatter.MARKER_ROLE)
        self.assertIn("step budget", last.text)

    def test_one_shot_budget_records_no_budget_marker(self):
        session = _session(['[{"op": "add_circle"}]'])
        turn = agent.Turn(session, "draw", budget=1)
        _drain(session, turn)
        self.assertEqual(turn.stop_reason, agent.StopReason.BUDGET)
        self.assertEqual([t.role for t in session.transcript],
                         ["user", "agent"])

    def test_stop_between_steps_leaves_a_clean_transcript(self):
        session = _session(['[{"op": "add_circle"}]'] * 2)
        turn = agent.Turn(session, "draw", budget=4)
        turn.feed(_ask(session, turn))
        turn.stop()
        self.assertTrue(turn.done)
        self.assertEqual(turn.stop_reason, agent.StopReason.STOPPED)
        self.assertIsNone(turn.next_request())
        self.assertEqual([t.role for t in session.transcript],
                         ["user", "agent"])


class StateTokenTC(unittest.TestCase):
    """The seam that keeps a reply from landing on the wrong target."""

    def test_mismatch_drops_the_commands_and_ends_the_turn(self):
        runner = _Runner()
        session = _session(['[{"op": "add_circle"}]'], runner=runner)
        moved = [False]
        turn = agent.Turn(session, "draw", budget=4,
                          token=lambda s, scene: moved[0])
        request = turn.next_request()
        moved[0] = True  # the canvas changed while the backend was running
        turn.feed(request.send_to(session.backend))
        self.assertEqual(turn.stop_reason, agent.StopReason.STATE)
        self.assertEqual(runner.commands, [])
        last = session.transcript[-1]
        self.assertEqual(last.role, agent.HistoryFormatter.MARKER_ROLE)
        self.assertIn("changed mid-turn", last.text)

    def test_default_token_separates_two_blank_worlds(self):
        class _World:
            def describe_state(self, level="basic"):
                return '{"shapes": []}'

        session = agent.AgentSession(world=_World(), runner=_Runner())
        first = agent.default_token(session, session.scene_context())
        session.world = _World()
        second = agent.default_token(session, session.scene_context())
        # Identical scenes, different worlds: content alone cannot tell them
        # apart, so identity has to be in the token.
        self.assertNotEqual(first, second)

    def test_default_token_follows_the_scene(self):
        session = agent.AgentSession(runner=_Runner())
        self.assertNotEqual(agent.default_token(session, "one shape"),
                            agent.default_token(session, "two shapes"))


class TurnGuardTC(unittest.TestCase):
    def test_feed_without_a_request_is_refused(self):
        session = _session(["[]"])
        turn = agent.Turn(session, "draw")
        with self.assertRaises(RuntimeError):
            turn.feed(agent.BackendResponse())

    def test_a_late_reply_after_stop_is_dropped_not_raised(self):
        # The worker's reply lands after the user hit Stop, in a Qt slot where
        # an exception has nowhere to go.  Nothing of it may be applied.
        runner = _Runner()
        session = _session(['[{"op": "add_circle"}]'], runner=runner)
        turn = agent.Turn(session, "draw")
        request = turn.next_request()
        turn.stop()
        self.assertIsNone(turn.feed(request.send_to(session.backend)))
        self.assertEqual(runner.commands, [])
        self.assertEqual([t.role for t in session.transcript], ["user"])

    def test_a_budget_below_one_step_is_refused(self):
        session = _session(["[]"])
        with self.assertRaises(ValueError):
            agent.Turn(session, "draw", budget=0)

    def test_the_prompt_is_not_replayed_beside_itself(self):
        # From step 2 on the prompt is no longer the trailing user turn, so
        # the history would repeat what the request tail already carries.
        session = _session(['[{"op": "add_circle"}]', "[]"])
        session.run_turn("draw a circle")
        replayed = agent.format_history(session.backend.requests[1].history)
        self.assertNotIn("user:", replayed)
        self.assertIn("add_circle", replayed)


class RunTurnWrapperTC(unittest.TestCase):
    def test_it_loops_until_the_model_stops(self):
        runner = _Runner()
        session = _session(['[{"op": "add_circle"}]',
                            '[{"op": "add_circle"}]', "[]"], runner=runner)
        session.run_turn("draw two circles")
        self.assertEqual(len(runner.commands), 2)
        self.assertEqual([t.role for t in session.transcript],
                         ["user", "agent", "agent", "agent"])

    def test_a_backend_that_raises_ends_the_turn_as_transport(self):
        class _Boom:
            def send(self, *args):
                raise RuntimeError("backend down")

        session = agent.AgentSession(backend=_Boom(), runner=_Runner())
        turn = session.run_turn("draw", budget=4)
        self.assertIn("backend down", turn.text)
        self.assertEqual([t.role for t in session.transcript],
                         ["user", "agent"])

    def test_no_backend_records_only_the_prompt(self):
        session = agent.AgentSession(runner=_Runner())
        self.assertIsNone(session.run_turn("draw"))
        self.assertEqual([t.role for t in session.transcript], ["user"])

    def test_the_second_step_carries_the_first_step_results(self):
        session = _session(['[{"op": "add_circle"}]', "[]"])
        session.run_turn("draw a circle")
        second = session.backend.requests[1]
        replayed = agent.format_history(second.history)
        self.assertIn("add_circle", replayed)
        self.assertIn("shape_id", replayed)


def _ask(session, turn):
    """One step's reply, composed and sent the way a driver would."""
    return turn.next_request().send_to(session.backend)


def _drain(session, turn):
    """Run ``turn`` to its stop."""
    while True:
        request = turn.next_request()
        if request is None:
            return
        turn.feed(request.send_to(session.backend))

# vim: set ff=unix fenc=utf8 et sw=4 ts=4 sts=4:
