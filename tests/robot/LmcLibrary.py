"""Robot Framework driver for the lmc-agent in-process GUI adapter.

Speaks the v1 NDJSON protocol (tests/lmc-agent/PROTOCOL.md) over a
``QTcpServer`` socket: one JSON object per line, responses matched by the
client-chosen ``id``. Stdlib only: no third-party dependencies.

Low-level keywords mirror qtexpert semantics but carry an ``Agent ``
prefix (``Agent Wait For Object``, ``Agent Click Object``, ...) so both
libraries can coexist in one suite during migration: Robot fails
unqualified calls when two libraries own the same name, and WITH NAME
does not hide the short forms. When qtexpert leaves (phase B), the
prefix goes away and the names become drop-in replacements.
Domain keywords (``Open Chat With Peer``, ...) are new and have no
qtexpert counterpart; import this library WITH NAME to keep calls
explicit, e.g.::

    Library    ../LmcLibrary.py    WITH NAME    LmcAgent

The application under test must run with the agent active: either the
LD_PRELOAD ``liblmc_test_preload.so`` (phase A, ``LMC_AGENT_PORT`` env)
or the ``lan-messenger-test`` binary (phase B).
"""

import json
import socket
import time

from robot.api.deco import keyword, library
from robot.utils import timestr_to_secs


@library(scope="SUITE", version="0.1.0")
class LmcLibrary:
    ROBOT_LIBRARY_DOC_FORMAT = "reST"

    DEFAULT_HOST = "127.0.0.1"
    DEFAULT_PORT = 9999
    POLL_INTERVAL = 0.5

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
                        "(is the app running with the test agent? "
                        "LD_PRELOAD/LMC_AGENT_PORT?)" % (host, port, timeout)
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

    # -- transport ----------------------------------------------------

    def _request(self, cmd, params=None):
        if self._sock is None:
            raise AssertionError("not connected; Connect To Test Agent first")
        self._req_id += 1
        rid = self._req_id
        line = json.dumps({"id": rid, "cmd": cmd, "params": params or {}}) + "\n"
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

    @keyword("Agent Wait For Object")
    def wait_for_object(self, locator, timeout="10s"):
        """Wait until a widget/action matches ``locator``. Returns its dict."""
        return self._request("wait_for_object", {"locator": locator, "timeout": timeout})

    @keyword("Agent Wait For Object To Disappear")
    def wait_for_object_to_disappear(self, locator, timeout="10s"):
        self._request("wait_for_gone", {"locator": locator, "timeout": timeout})

    @keyword("Agent Object Should Exist")
    def object_should_exist(self, locator):
        res = self._request("object_exists", {"locator": locator})
        if not res.get("exists"):
            raise AssertionError("no object matches '%s'" % locator)

    @keyword("Agent Get Object Property")
    def get_object_property(self, locator, property):
        """Return the property value (number/bool/string)."""
        return self._request("get_property",
                             {"locator": locator, "property": property}).get("value")

    @keyword("Agent Set Object Property")
    def set_object_property(self, locator, property, value):
        self._request("set_property",
                      {"locator": locator, "property": property, "value": value})

    @keyword("Agent Click Object")
    def click_object(self, locator):
        """Async in-process click (fire-and-post, never blocks on modals)."""
        self._request("click", {"locator": locator})

    @keyword("Agent Dump Object Tree")
    def dump_object_tree(self, output_file):
        """Write the full widget/action tree JSON to ``output_file``."""
        res = self._request("dump_tree")
        with open(output_file, "w", encoding="utf-8") as f:
            json.dump(res.get("tree", []), f, indent=2, ensure_ascii=False)

    @keyword("Agent List Windows")
    def list_windows(self):
        return self._request("list_windows").get("windows", [])

    @keyword("Agent Grab Screenshot")
    def grab_screenshot(self, output_file, locator=None):
        """Render a widget app-side (QWidget::grab) into ``output_file``."""
        params = {"file": output_file}
        if locator is not None:
            params["locator"] = locator
        return self._request("grab_screenshot", params).get("file")

    @keyword("Agent Close Application")
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
