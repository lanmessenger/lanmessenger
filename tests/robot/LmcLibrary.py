"""Robot Framework driver for the lmc-agent in-process GUI adapter.

Speaks the v1 NDJSON protocol (tools/lmc-agent/PROTOCOL.md) over a
``QTcpServer`` socket: one JSON object per line, responses matched by the
client-chosen ``id``. Stdlib only: no third-party dependencies.

Low-level keywords carry the plain qtexpert-compatible names (``Wait For
Object``, ``Click Object``, ...) so this library is a drop-in replacement:
suites that ran against qtexpert only need the import line changed. During
the migration the names were ``Agent ``-prefixed (Robot fails unqualified
calls when two libraries own the same name), which qtexpert removal
retired. Domain keywords (``Open Chat With Peer``, ``Close
Window``, ...) have no qtexpert counterpart. Import plain::

    Library    ../LmcLibrary.py

The application under test must run with the agent active: the
``lan-messenger-test`` binary (the in-process agent linked in, enabled
unless ``LMC_TEST_AGENT=0``).
"""

import json
import socket
import time

from robot.api.deco import keyword, library
from robot.api import logger
from robot.utils import timestr_to_secs


@library(scope="SUITE", version="0.1.0")
class LmcLibrary:
    ROBOT_LIBRARY_DOC_FORMAT = "reST"

    DEFAULT_HOST = "127.0.0.1"
    DEFAULT_PORT = 9999
    POLL_INTERVAL = 0.5
    # Socket timeout for commands without their own `timeout` param: the
    # agent answers those immediately (no nested-loop polling).
    DEFAULT_SOCK_TIMEOUT = 5.0
    SOCK_TIMEOUT_MARGIN = 10.0
    # Margin on top of a command's own `timeout`: the agent may legitimately
    # take the full timeout to answer (wait_for_object/wait_for_gone/open_chat
    # poll in a nested loop and reply with an error only afterwards). The
    # socket must outlive that, otherwise the client times out first, the
    # stream is left mid-line ("cannot read from timed out object") and every
    # later keyword in the suite fails as a cascade (seen in Dialogs on
    # Windows: 5s socket vs 10s agent turned one stuck close into 6 reds).

    def __init__(self):
        self._sock = None
        self._stream = None
        self._req_id = 0

    # -- connection ---------------------------------------------------

    @keyword("Connect To Test Agent")
    def connect_to_test_agent(self, port=DEFAULT_PORT, host=DEFAULT_HOST,
                              timeout="30s"):
        """Connect to the in-process agent of a running app. Retries until
        ``timeout``; the agent starts with the GUI, so call after launch.
        """
        if self._sock is not None:
            raise AssertionError("already connected; Disconnect From Test Agent first")
        deadline = time.monotonic() + timestr_to_secs(timeout)
        while True:
            try:
                sock = socket.socket(socket.AF_INET, socket.SOCK_STREAM)
                sock.settimeout(5)
                sock.connect((host, int(port)))
                break
            except OSError:
                if time.monotonic() >= deadline:
                    raise AssertionError(
                        "cannot connect to lmc-agent at %s:%s within %s "
                        "(is lan-messenger-test running with the test agent? "
                        "LMC_TEST_AGENT/LMC_AGENT_PORT?)" % (host, port, timeout)
                    )
                time.sleep(self.POLL_INTERVAL)
        self._sock = sock
        self._stream = sock.makefile("r", encoding="utf-8")

    @keyword("Disconnect From Test Agent")
    def disconnect_from_test_agent(self):
        """Close the agent connection (no app shutdown sent)."""
        try:
            if self._stream is not None:
                self._stream.close()
        finally:
            self._stream = None
            if self._sock is not None:
                self._sock.close()
                self._sock = None

    @keyword("Get App Log Info")
    def get_app_log_info(self, timeout="15s"):
        """Trace file path (``get_log``): dict with ``path`` (""
        when tracing is off). ``timeout`` only sizes the client socket:
        the agent answers immediately when its GUI thread pumps, but a
        cold start (first paint on a slow leg) can stall the loop for
        seconds, so callers in Suite Setup keep this generous and
        best-effort — a strict probe here once turned one slow start
        into a whole red suite.
        """
        return self._request("get_log", {"timeout": timeout})

    # -- transport ----------------------------------------------------

    def _request(self, cmd, params=None):
        if self._sock is None:
            raise AssertionError("not connected; Connect To Test Agent first")
        self._req_id += 1
        rid = self._req_id
        params = params or {}
        try:
            cmd_timeout = timestr_to_secs(params.get("timeout", "0s"))
        except (ValueError, TypeError):
            cmd_timeout = 0
        if cmd_timeout > 0:
            self._sock.settimeout(cmd_timeout + self.SOCK_TIMEOUT_MARGIN)
        else:
            self._sock.settimeout(self.DEFAULT_SOCK_TIMEOUT)
        line = json.dumps({"id": rid, "cmd": cmd, "params": params}) + "\n"
        try:
            self._sock.sendall(line.encode("utf-8"))
        except OSError as e:
            raise AssertionError("lmc-agent send failed (cmd %s): %s" % (cmd, e))
        while True:
            try:
                raw = self._stream.readline()
            except OSError as e:
                raise AssertionError("lmc-agent read failed (cmd %s): %s" % (cmd, e))
            if not raw:
                raise AssertionError("lmc-agent closed the connection (cmd %s)" % cmd)
            try:
                msg = json.loads(raw)
            except ValueError:
                continue
            if not isinstance(msg, dict) or msg.get("id") != rid:
                continue
            if msg.get("ok"):
                return msg.get("result", {})
            raise AssertionError("lmc-agent error for '%s': %s" % (cmd, msg.get("error")))

    # -- low level (qtexpert-compatible names) -------------------------

    @keyword("Wait For Object")
    def wait_for_object(self, locator, timeout="10s"):
        """Wait until a widget/action matches ``locator``. Returns its dict."""
        return self._request("wait_for_object", {"locator": locator, "timeout": timeout})

    @keyword("Wait For Object To Disappear")
    def wait_for_object_to_disappear(self, locator, timeout="10s", poll="0.2s"):
        """Poll ``object_exists`` until nothing matches ``locator``.

        Test-side polling instead of the agent's ``wait_for_gone``: that
        command blocks the GUI thread in a nested ``QEventLoop`` for the
        whole timeout, which on Windows can starve the very deletion
        (``close()`` + ``deleteLater()``) it waits for — the window then
        lands exactly on the timeout boundary (``visible=false
        sameWindow=true``). Each poll here is a fast agent command, so
        the main loop stays free to deliver close/delete at any moment.

        NOTE: the starvation/race above is a hypothesis, not proven —
        seen as 3 boundary-exact misses (History full/rerun, Help full)
        vs normally millisecond deletion (e.g. Help PASS in 0.059s on
        rerun). If polling still hits the exact boundary, the hypothesis
        is wrong and the deletion itself is slow — investigate app side.
        """
        try:
            budget = timestr_to_secs(timeout)
        except (ValueError, TypeError):
            budget = 10.0
        try:
            interval = timestr_to_secs(poll)
        except (ValueError, TypeError):
            interval = 0.2
        deadline = time.monotonic() + budget
        while True:
            res = self._request("object_exists", {"locator": locator})
            if not res.get("exists"):
                return
            if time.monotonic() >= deadline:
                raise AssertionError(
                    "object '%s' still present after %s "
                    "(test-side poll, no agent nesting)" % (locator, timeout))
            time.sleep(interval)

    @keyword("Object Should Exist")
    def object_should_exist(self, locator):
        res = self._request("object_exists", {"locator": locator})
        if not res.get("exists"):
            raise AssertionError("no object matches '%s'" % locator)

    @keyword("Get Object Property")
    def get_object_property(self, locator, property):
        """Return the property value (number/bool/string)."""
        return self._request("get_property",
                             {"locator": locator, "property": property}).get("value")

    @keyword("Set Object Property")
    def set_object_property(self, locator, property, value):
        self._request("set_property",
                      {"locator": locator, "property": property, "value": value})

    @keyword("Click Object")
    def click_object(self, locator):
        """Async in-process click (fire-and-post, never blocks on modals)."""
        self._request("click", {"locator": locator})

    @keyword("Trigger Action")
    def trigger_action(self, locator):
        """Trigger a menu/toolbar QAction by ``locator`` (e.g. ``text=History``,
        ``&`` mnemonic stripped). Async: the reply precedes the slot, so an
        exec() modal opened by the action keeps the connection alive. Returns
        the triggered action text.
        """
        return self._request("trigger_action", {"locator": locator}).get("triggered")

    @keyword("Close Window")
    def close_window(self, locator):
        """Close the top-level window matching ``locator`` (``name=ChatWindow``
        or any unique child, e.g. ``name=tvUserList``). Deterministic
        counterpart of a real Escape: lmc windows that reject on Escape
        install the filter on their children and the handler is only
        ``close()``, so this runs the same path without depending on which
        widget holds the X focus. Modeless reuse windows (Transfers, Chat
        Room) hide; windows with WA_DeleteOnClose are deleted.
        The agent's post snapshot (visible/isWindow/ptr) is logged, so a
        later wait_for_gone verdict can be correlated to this exact object.
        """
        res = self._request("close_window", {"locator": locator})
        logger.info("close_window %s -> %s"
                    % (locator, json.dumps(res, ensure_ascii=False)))
        return res

    @keyword("Type Text")
    def type_text(self, locator, text, send=True):
        """Set text on a ``QTextEdit``/``QPlainTextEdit``/``QLineEdit``
        matching ``locator``. With ``send`` (default ``True``) a ``Return``
        press is posted right after, so the widget's own Enter handling
        runs — e.g. the chat input sends through ``sendMessage``. Fire-and-post:
        the reply precedes the key handling.
        """
        if isinstance(send, str):
            send = send.strip().lower() in ("true", "1", "yes")
        self._request("type_text",
                      {"locator": locator, "text": text, "send": bool(send)})

    @keyword("Send Key")
    def send_key(self, key, locator=None, modifiers=None):
        """Post a ``key`` press+release (see PROTOCOL.md for names) to the
        widget matching ``locator`` — or to the focus widget / active window
        when omitted. ``modifiers`` is a comma-separated string, e.g.
        ``ctrl,shift``. For text entry use ``Type Text`` instead.
        """
        params = {"key": key}
        if locator is not None:
            params["locator"] = locator
        if modifiers is not None:
            params["modifiers"] = [m.strip() for m in str(modifiers).split(",")
                                   if m.strip()]
        self._request("send_key", params)

    @keyword("Dump Object Tree")
    def dump_object_tree(self, output_file):
        """Write the full widget/action tree JSON to ``output_file``."""
        res = self._request("dump_tree")
        with open(output_file, "w", encoding="utf-8") as f:
            json.dump(res.get("tree", []), f, indent=2, ensure_ascii=False)

    @keyword("List Windows")
    def list_windows(self):
        return self._request("list_windows").get("windows", [])

    @keyword("Grab Screenshot")
    def grab_screenshot(self, output_file, locator=None):
        """Render a widget app-side (QWidget::grab) into ``output_file``."""
        params = {"file": output_file}
        if locator is not None:
            params["locator"] = locator
        return self._request("grab_screenshot", params).get("file")

    @keyword("Close Application")
    def close_application(self):
        """Ask the app to quit via the agent (async quit after reply)."""
        self._request("close_app")

    # -- domain --------------------------------------------------------

    @keyword("Open Chat With Peer")
    def open_chat_with_peer(self, peer, timeout="15s"):
        """Open (or show) the conversation with ``peer`` (name or userId)
        through the real chatStarting path. Returns the chat window title,
        e.g. ``MockBob - Conversation``. Retriable: raises until the peer
        row is actionable and the ChatWindow is visible.
        """
        return self._request("open_chat",
                             {"peer": peer, "timeout": timeout}).get("title")
