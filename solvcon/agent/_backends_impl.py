# Copyright (c) 2026, solvcon team <contact@solvcon.net>
# BSD 3-Clause License, see COPYING


"""
Concrete AI backends over external CLIs and HTTP APIs.

This module holds the backends that talk to an installed AI CLI or an
OpenAI-compatible HTTP server, plus the shared plumbing they need:
:class:`SubprocessBackend` (PATH discovery and a cancellable child process),
:class:`OpenAIHttpBackend` (stdlib ``http.client``, no SDK), and
:class:`ToolCallParser` (turn a model reply into Agent Draw command dicts).
The Codex CLI backend is a follow-up that reuses :class:`SubprocessBackend`.
:class:`ScriptedEchoBackend` replays canned replies through that same parser,
so a driving loop is testable with no key and no network.

The module imports no Qt and makes no network call at import time.  A backend
registers itself only as a class instance in the shared registry, so a caller
lists it and probes :meth:`~solvcon.agent.AgentBackend.available` before use.
"""

import abc
import dataclasses
import http.client
import json
import os
import shutil
import subprocess
import tempfile
import urllib.parse

from . import _backend


@dataclasses.dataclass
class ParsedReply:
    """A parsed model reply: the commands it proposed, what the reply turned
    out to be, and the parse ``error`` when it was malformed."""

    commands: list = dataclasses.field(default_factory=list)
    status: _backend.ParseStatus = _backend.ParseStatus.EMPTY
    error: str = None

    def response(self, text):
        """This reply as the :class:`BackendResponse` a backend returns for
        the ``text`` it was parsed from."""
        return _backend.BackendResponse(
            text=text, commands=self.commands, error=self.error,
            status=self.status)


class ToolCallParser:
    """Turns a model reply into the command dicts a session runs.

    Op names are not checked here.  An op the tool surface does not advertise
    is a command the runner rejects with its own error, which the model can
    see and fix; rejecting it while parsing would throw away the whole batch
    over one bad entry.
    """

    @classmethod
    def strip_code_fences(cls, text):
        """Drop a surrounding triple-backtick fence (bare or tagged) if
        present."""
        stripped = text.strip()
        if not stripped.startswith("```"):
            return text
        lines = stripped.splitlines()
        if lines and lines[0].startswith("```"):
            lines = lines[1:]
        if lines and lines[-1].strip() == "```":
            lines = lines[:-1]
        return "\n".join(lines)

    @classmethod
    def load_json_payload(cls, text):
        """Parse the first JSON array or object out of a model reply,
        tolerating a code fence or surrounding prose.

        Return the parsed value, or ``None`` when the reply has no JSON-looking
        span (plain prose).  Raise :class:`ValueError` when a ``[``/``{`` span
        is present but does not parse, so a truncated or invalid command batch
        is not mistaken for an empty one.
        """
        text = cls.strip_code_fences(text).strip()
        if not text:
            return None
        try:
            return json.loads(text)
        except json.JSONDecodeError:
            pass
        saw_span = False
        for opener, closer in (("[", "]"), ("{", "}")):
            start = text.find(opener)
            end = text.rfind(closer)
            if start == -1 or end <= start:
                continue
            saw_span = True
            try:
                return json.loads(text[start:end + 1])
            except json.JSONDecodeError:
                continue
        if saw_span or text[0] in "[{":
            raise ValueError("model reply has malformed JSON")
        return None

    @classmethod
    def commands_of(cls, data):
        """The command dicts in an already-parsed JSON payload.

        Accept an array, or a lone object treated as a one-command array.
        Each command must be an object with a string ``op``; anything else
        raises :class:`ValueError`.
        """
        if isinstance(data, dict):
            data = [data]
        if not isinstance(data, list):
            raise ValueError("model reply is not a JSON array of commands")
        commands = []
        for entry in data:
            if not isinstance(entry, dict):
                raise ValueError("command is not an object: %r" % (entry,))
            op = entry.get("op")
            if not isinstance(op, str):
                raise ValueError(
                    "command needs a string \"op\": %r" % (entry,))
            commands.append(entry)
        return commands

    @classmethod
    def parse(cls, text):
        """Turn a model reply into a list of command dicts.

        Raise :class:`ValueError` on a malformed reply (including invalid JSON
        that looks like a command batch) so a caller records it as an error
        rather than running a bad command.  Plain prose with no JSON yields an
        empty list; :meth:`parse_reply` is what tells the two apart.
        """
        data = cls.load_json_payload(text)
        return [] if data is None else cls.commands_of(data)

    @classmethod
    def parse_reply(cls, text):
        """:meth:`parse` as a :class:`ParsedReply`, naming which of the four
        shapes the reply took.

        A reply with nothing in it is :attr:`~ParseStatus.EMPTY` rather than
        prose: it carries no text worth recording as a reply, and a loop
        should end on it the same way an explicit ``[]`` ends one.
        """
        status = _backend.ParseStatus
        if not (text or "").strip():
            return ParsedReply(status=status.EMPTY)
        try:
            data = cls.load_json_payload(text)
        except ValueError as exc:
            return ParsedReply(status=status.MALFORMED, error=str(exc))
        if data is None:
            return ParsedReply(status=status.PROSE)
        try:
            commands = cls.commands_of(data)
        except ValueError as exc:
            return ParsedReply(status=status.MALFORMED, error=str(exc))
        return ParsedReply(
            commands, status.COMMANDS if commands else status.EMPTY)


class CancellableBackend:
    """The cancellation bookkeeping a backend with an in-flight call shares.

    A cancelled call surfaces as an ordinary failure (a killed child, a closed
    socket), so the flag is what lets :meth:`failure` report it as the
    deliberate stop it was instead of a transport fault a caller might retry.
    """

    _cancelled = False

    def begin(self):
        """Open a call: forget any earlier cancellation."""
        self._cancelled = False

    def failure(self, error, outcome=_backend.TransportOutcome.TRANSPORT):
        """A failed :class:`BackendResponse` carrying ``outcome``, or
        ``CANCELLED`` when this call was stopped."""
        if self._cancelled:
            outcome = _backend.TransportOutcome.CANCELLED
        return _backend.BackendResponse(error=error, outcome=outcome)

    def cancelled_reply(self):
        """The reply for a call that was stopped, or ``None`` if it was not.

        A cancel that lands in the moment before the child or the connection
        is reachable tears down nothing, so the call can still succeed.  The
        answer is then unwanted rather than useful, and returning it would let
        commands land after the user asked for none.
        """
        if not self._cancelled:
            return None
        return self.failure("cancelled")


class SubprocessBackend(CancellableBackend, _backend.AgentBackend):
    """Base for backends that shell out to an AI CLI found on ``PATH``.

    A subclass sets :attr:`command` to the executable name and implements
    :meth:`_build_argv` (and, for a non-plain-text CLI, :meth:`_parse_output`).
    This base owns everything else: PATH discovery, the :meth:`available`
    check, a cancellable child process, and the whole :meth:`send` flow that
    turns a run into a :class:`BackendResponse`.  A new CLI backend is thus the
    two hooks, never a copied error-handling skeleton.  The running process is
    kept on the instance so a driver thread can :meth:`cancel` a long-running
    call.
    """

    #: The executable name a subclass discovers on ``PATH``.
    command = None

    #: The only environment variables the agent CLI receives: the process
    #: basics it needs to run, plus the credentials of the supported
    #: authentication modes (see the class docstring).
    env_passthrough = (
        "HOME", "USER", "LOGNAME", "PATH", "TMPDIR",
        "ANTHROPIC_API_KEY", "CLAUDE_CODE_OAUTH_TOKEN", "CLAUDE_CONFIG_DIR")

    def __init__(self, timeout=120):
        self._timeout = timeout
        self._proc = None

    @property
    def name(self):
        """Selector label derived from the CLI, e.g. ``claude (cli)``."""
        return "%s (cli)" % self.command

    def executable(self):
        """The resolved path to :attr:`command`, or ``None`` if not on PATH."""
        return shutil.which(self.command) if self.command else None

    def available(self):
        return self.executable() is not None

    @abc.abstractmethod
    def _build_argv(self, exe, user_prompt, system_prompt):
        """The argv that runs ``exe`` on the ``user_prompt``, passing
        ``system_prompt`` through whatever system-prompt channel the CLI
        offers."""

    def _parse_output(self, stdout):
        """Extract the assistant text from CLI ``stdout``.  The default treats
        stdout as the reply; override for a CLI that wraps it (JSON, etc.)."""
        return (stdout or "").strip()

    def send(self, prompt, scene_context, tool_surface, history=()):
        self.begin()
        exe = self.executable()
        if exe is None:
            return self.failure("%s not found on PATH" % self.command)
        user_prompt = self._compose_user(
            prompt, scene_context, tool_surface, history)
        argv = self._build_argv(exe, user_prompt, self._INSTRUCTIONS)
        try:
            code, out, err = self._communicate(argv)
        except subprocess.TimeoutExpired:
            return self.failure("%s timed out" % self.command,
                                _backend.TransportOutcome.TIMEOUT)
        except OSError as exc:
            return self.failure("%s failed: %s" % (self.command, exc))
        if code != 0:
            return self.failure(
                "%s exit %d: %s" % (self.command, code, (err or "").strip()))
        stopped = self.cancelled_reply()
        if stopped is not None:
            return stopped
        text = self._parse_output(out)
        return ToolCallParser.parse_reply(text).response(text)

    def cancel(self):
        """Terminate the in-flight child, if any.  Safe to call from another
        thread while :meth:`send` blocks in :meth:`_communicate`."""
        self._cancelled = True
        proc = self._proc
        if proc is not None and proc.poll() is None:
            proc.terminate()

    def _communicate(self, argv):
        """Run ``argv``, returning ``(returncode, stdout, stderr)``.

        The child is held on ``self._proc`` so :meth:`cancel` can reach it, and
        killed if it outruns the timeout (then the timeout propagates)."""

        env = {name: os.environ[name]
               for name in self.env_passthrough if name in os.environ}
        workdir = tempfile.mkdtemp(prefix="solvcon-agent-")
        try:
            proc = subprocess.Popen(
                argv, stdout=subprocess.PIPE, stderr=subprocess.PIPE,
                text=True, cwd=workdir, env=env)
            self._proc = proc
            if self._cancelled:
                # A cancel between spawning the child and publishing it here
                # found nothing to terminate; act on it now.
                proc.terminate()
            try:
                out, err = proc.communicate(timeout=self._timeout)
            except subprocess.TimeoutExpired:
                proc.kill()
                proc.communicate()
                raise
            finally:
                self._proc = None
            return proc.returncode, out, err
        finally:
            shutil.rmtree(workdir, ignore_errors=True)


class ClaudeCliBackend(SubprocessBackend):
    """Backend over Anthropic's ``claude`` command-line tool.

    It runs the CLI in print mode with JSON output, folds the tool surface and
    scene context into the prompt, and parses the model's JSON reply into
    commands.  No API key lives here: the CLI owns authentication.

    The model is named explicitly because ``--setting-sources ""`` cuts the
    CLI off from the config files it would otherwise pick one from, so the
    same request would run on a different model as the CLI default moves.
    """

    command = "claude"

    def _build_argv(self, exe, user_prompt, system_prompt):
        # TODO: provide more permission and config to the CLI sandbox later.
        return [
            exe, "-p", user_prompt, "--output-format", "json",
            "--append-system-prompt", system_prompt,
            "--tools", "",
            "--permission-mode", "dontAsk",  # no interactive prompts
            "--setting-sources", "",  # no config files
            "--strict-mcp-config",  # no mcp config files
            "--disable-slash-commands",  # no interactive slash commands
            "--no-session-persistence",  # no session files
        ]

    def _parse_output(self, stdout):
        """Pull the assistant text out of ``claude --output-format json``
        output, falling back to the raw text when it is not that envelope."""
        stdout = (stdout or "").strip()
        try:
            payload = json.loads(stdout)
        except json.JSONDecodeError:
            return stdout
        if isinstance(payload, dict):
            result = payload.get("result")
            return result if isinstance(result, str) else stdout
        return stdout


_backend.BackendRegistry.register(ClaudeCliBackend())


class OpenAIHttpBackend(CancellableBackend, _backend.AgentBackend):
    """Backend over an OpenAI-compatible Chat Completions HTTP API.

    Uses only the stdlib (``http.client`` and ``urllib.parse``); no vendor
    SDK.  Point ``base_url`` at OpenAI, Ollama's ``/v1`` endpoint, or any
    compatible server.  Defaults and the optional API key come from the
    constructor or the ``SOLVCON_OPENAI_BASE_URL``, ``SOLVCON_OPENAI_MODEL``,
    and ``SOLVCON_OPENAI_API_KEY`` environment variables.  The in-flight
    connection is kept on the instance so a driver thread can :meth:`cancel`.
    """

    # Local Ollama's OpenAI-compatible root; override for a remote provider.
    _DEFAULT_BASE_URL = "http://127.0.0.1:11434/v1"
    _DEFAULT_MODEL = "qwen2.5vl:7b"

    def __init__(self, base_url=None, model=None, api_key=None, timeout=120):
        self._base_url = base_url if base_url is not None else self._env_or(
            "SOLVCON_OPENAI_BASE_URL", self._DEFAULT_BASE_URL)
        self._model = model if model is not None else self._env_or(
            "SOLVCON_OPENAI_MODEL", self._DEFAULT_MODEL)
        self._api_key = api_key if api_key is not None else self._env_or(
            "SOLVCON_OPENAI_API_KEY", "")
        self._timeout = timeout
        self._conn = None

    @staticmethod
    def _env_or(name, default):
        """``os.environ[name]`` when set and non-empty, else ``default``."""
        value = os.environ.get(name)
        return value if value else default

    @property
    def name(self):
        return "openai (http)"

    @property
    def base_url(self):
        """API root including the ``/v1`` suffix, with no trailing slash."""
        return (self._base_url or "").rstrip("/")

    @property
    def model(self):
        return self._model

    def available(self):
        """True when both a base URL and a model name are configured."""
        return bool(self.base_url) and bool(self._model)

    def send(self, prompt, scene_context, tool_surface, history=()):
        self.begin()
        if not self.available():
            return self.failure(
                "openai http backend needs base_url and model")
        user_prompt = self._compose_user(
            prompt, scene_context, tool_surface, history)
        body = {
            "model": self._model,
            "stream": False,
            "messages": [
                {"role": "system", "content": self._INSTRUCTIONS},
                {"role": "user", "content": user_prompt},
            ],
        }
        try:
            status, raw = self._post_chat(body)
        except TimeoutError:
            return self.failure("openai http timed out",
                                _backend.TransportOutcome.TIMEOUT)
        except (OSError, http.client.HTTPException) as exc:
            return self.failure("openai http failed: %s" % exc)
        if status != 200:
            detail = (raw or b"").decode("utf-8", errors="replace").strip()
            return self.failure(
                "openai http status %d: %s" % (status, detail))
        try:
            payload = json.loads(raw.decode("utf-8"))
        except (UnicodeDecodeError, json.JSONDecodeError) as exc:
            return self.failure("openai http bad JSON: %s" % exc)
        text = self._parse_chat_payload(payload)
        if text is None:
            return self.failure(
                "openai http response missing assistant text")
        stopped = self.cancelled_reply()
        if stopped is not None:
            return stopped
        return ToolCallParser.parse_reply(text).response(text)

    def cancel(self):
        """Close the in-flight HTTP connection, if any.  Safe to call from
        another thread while :meth:`send` blocks in :meth:`_post_chat`."""
        self._cancelled = True
        conn = self._conn
        if conn is not None:
            try:
                conn.close()
            except OSError:
                pass

    @classmethod
    def _parse_chat_payload(cls, payload):
        """Assistant text from a Chat Completions JSON body, or ``None``."""
        if not isinstance(payload, dict):
            return None
        choices = payload.get("choices")
        if not isinstance(choices, list) or not choices:
            return None
        first = choices[0]
        if not isinstance(first, dict):
            return None
        return cls._message_text(first.get("message") or {})

    @staticmethod
    def _message_text(message):
        """Assistant text from an OpenAI-style ``message`` object.

        Accept a plain string ``content``, or a list of content parts (the
        multimodal shape) by joining the text pieces.
        """
        if not isinstance(message, dict):
            return ""
        content = message.get("content")
        if isinstance(content, str):
            return content
        if isinstance(content, list):
            parts = []
            for part in content:
                if isinstance(part, str):
                    parts.append(part)
                elif isinstance(part, dict):
                    text = part.get("text")
                    if isinstance(text, str):
                        parts.append(text)
            return "".join(parts)
        return ""

    def _post_chat(self, body):
        """POST ``body`` to ``/chat/completions``; return ``(status, raw)``.

        Builds an ``http.client`` connection from :attr:`base_url`, holds it
        on ``self._conn`` for :meth:`cancel`, and always clears that slot.
        """
        parsed = urllib.parse.urlparse(self.base_url)
        if parsed.scheme not in ("http", "https") or not parsed.netloc:
            raise OSError("invalid base_url: %s" % self.base_url)
        path = parsed.path.rstrip("/") + "/chat/completions"
        if parsed.query:
            path = "%s?%s" % (path, parsed.query)
        host = parsed.hostname
        if not host:
            raise OSError("invalid base_url host: %s" % self.base_url)
        port = parsed.port
        if port is None:
            port = 443 if parsed.scheme == "https" else 80
        headers = {
            "Content-Type": "application/json",
            "Accept": "application/json",
        }
        if self._api_key:
            headers["Authorization"] = "Bearer %s" % self._api_key
        payload = json.dumps(body).encode("utf-8")
        if parsed.scheme == "https":
            conn = http.client.HTTPSConnection(
                host, port, timeout=self._timeout)
        else:
            conn = http.client.HTTPConnection(
                host, port, timeout=self._timeout)
        self._conn = conn
        if self._cancelled:
            # A cancel between building the connection and publishing it here
            # closed nothing; close it now instead of sending the request.
            conn.close()
        try:
            conn.request("POST", path, body=payload, headers=headers)
            response = conn.getresponse()
            return response.status, response.read()
        finally:
            try:
                conn.close()
            except OSError:
                pass
            self._conn = None


_backend.BackendRegistry.register(OpenAIHttpBackend())


class ScriptedEchoBackend(_backend.AgentBackend):
    """Offline backend that replays a canned sequence of replies.

    A reply is either the text a model would have printed, parsed by the same
    :class:`ToolCallParser` a real backend runs, or a ready
    :class:`BackendResponse` for the transport outcomes no text can express.
    Text replies are what make a scripted multi-step turn worth trusting: the
    fence stripping, the malformed batches, and the ``[]``-versus-prose split
    are exercised for real, not stubbed.

    Every request is recorded in :attr:`requests`, so a test can assert what
    the loop composed on each step.  Once the script runs out the backend
    replies with nothing, which ends a turn rather than looping to the budget.
    It does not register itself: it is a test and demo double.
    """

    name = "scripted (offline)"

    def __init__(self, replies=()):
        self._replies = list(replies)
        self.requests = []

    @property
    def remaining(self):
        """The replies not yet handed out."""
        return list(self._replies)

    def available(self):
        return True

    def send(self, prompt, scene_context, tool_surface, history=()):
        self.requests.append(_backend.TurnRequest(
            prompt=prompt, scene_context=scene_context,
            tool_surface=list(tool_surface or ()), history=list(history)))
        if not self._replies:
            return _backend.BackendResponse()
        reply = self._replies.pop(0)
        if isinstance(reply, _backend.BackendResponse):
            return reply
        return ToolCallParser.parse_reply(reply).response(reply)

# vim: set ff=unix fenc=utf8 et sw=4 ts=4 sts=4:
