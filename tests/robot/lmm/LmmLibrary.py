"""Robot Framework driver for the lmmd mock-peer daemon (tools/lmm).

Speaks the v1 NDJSON protocol (tools/lmm/PROTOCOL.md) over TCP loopback
(``127.0.0.1:<port>``): one JSON object per line, responses matched
by the client-chosen ``id``. Stdlib only: no third-party dependencies.
Loopback TCP is one address namespace on every OS (unlike unix sockets:
``/tmp`` on Linux, ``$TMPDIR`` on macOS, named pipes on Windows), so no
per-OS path resolution is needed anywhere.

Cursor model: the library keeps one ``since`` cursor per connection.
``Poll Events`` and ``Wait For Event`` advance it to the ``next`` value
returned by the daemon (highest returned ``seq``; protocol detail, see
``cmdEventsPoll``). ``Reset Event Cursor`` fast-forwards past everything
seen so far -- call it after the setup/visibility phase so delivery
waits only see fresh events.

Note on numbers: ``QJsonValue`` stores numbers as double, so ``seq`` /
``next`` arrive as JSON floats once large; they are converted with
``int()`` here, exact for realistic ranges (seq < 2**53).
"""

import json
import socket
import time

from robot.api.deco import keyword, library
from robot.utils import timestr_to_secs


@library(scope="SUITE", version="0.1.0")
class LmmLibrary:
    ROBOT_LIBRARY_DOC_FORMAT = "reST"

    POLL_INTERVAL = 0.5
    ANNOUNCE_INTERVAL = 5.0

    def __init__(self):
        self._sock = None
        self._stream = None
        self._req_id = 0
        self._cursor = 0

    # -- connection ---------------------------------------------------

    @keyword("Connect To Daemon")
    def connect_to_daemon(self, port=17777, host="127.0.0.1", timeout="10s"):
        """Connect to a running lmmd on TCP loopback ``host:port``.
        Retries until ``timeout``. Resets the event cursor."""
        if self._sock is not None:
            raise AssertionError("already connected; Disconnect From Daemon first")
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
                        "cannot connect to lmmd at '%s:%s' within %s "
                        "(is the daemon running with --port?)" % (host, port, timeout)
                    )
                time.sleep(self.POLL_INTERVAL)
        self._sock = sock
        self._stream = sock.makefile("r", encoding="utf-8")
        self._cursor = 0

    @keyword("Disconnect From Daemon")
    def disconnect_from_daemon(self):
        """Close the daemon connection (no ``shutdown`` sent)."""
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
            raise AssertionError("not connected; Connect To Daemon first")
        self._req_id += 1
        rid = self._req_id
        line = json.dumps({"id": rid, "cmd": cmd, "params": params or {}}) + "\n"
        try:
            self._sock.sendall(line.encode("utf-8"))
        except OSError as e:
            raise AssertionError("lmmd send failed (cmd %s): %s" % (cmd, e))
        while True:
            try:
                raw = self._stream.readline()
            except OSError as e:
                raise AssertionError("lmmd read failed (cmd %s): %s" % (cmd, e))
            if not raw:
                raise AssertionError("lmmd closed the connection (cmd %s)" % cmd)
            try:
                msg = json.loads(raw)
            except ValueError:
                continue
            if not isinstance(msg, dict) or msg.get("id") != rid:
                continue  # v1 has no server push; ignore strays (e.g. id 0)
            if msg.get("ok"):
                return msg.get("result", {})
            raise AssertionError("lmmd error for '%s': %s" % (cmd, msg.get("error")))

    # -- thin wrappers (1:1 with PROTOCOL.md) --------------------------

    @keyword("Daemon Status")
    def daemon_status(self):
        """Return the ``status`` result dict (port, peers, uptimeMs...)."""
        return self._request("status")

    @keyword("Worker Status")
    def worker_status(self, name):
        """Return a worker's own ``status`` dict (port, settingsFile...).
        Test introspection: the settings path differs per OS."""
        return self._request("worker.status", {"peer": name})

    @keyword("Add Peer")
    def add_peer(self, name, status=None):
        """Add a mock peer; return its userId."""
        params = {"name": name}
        if status is not None:
            params["status"] = status
        return self._request("peer.add", params).get("userId")

    @keyword("Remove Peer")
    def remove_peer(self, name):
        self._request("peer.remove", {"peer": name})

    @keyword("List Peers")
    def list_peers(self):
        return self._request("peer.list").get("peers", [])

    @keyword("Peer Contacts")
    def peer_contacts(self, name):
        return self._request("peer.contacts", {"peer": name}).get("contacts", [])

    @keyword("Announce Peer")
    def announce_peer(self, name):
        self._request("peer.announce", {"peer": name})

    @keyword("Send Message")
    def send_message(self, peer_from, to, text):
        """Send a chat message from mock ``peer_from``. ``to`` is a peer
        handle or a visible userId. Fails the test on daemon error
        (unknown peer/recipient). Returns the result dict."""
        return self._request("msg.send", {"from": peer_from, "to": to, "text": text})

    @keyword("Send Message And Wait")
    def send_message_and_wait(self, peer_from, to, text, timeout="15s", attempts="3"):
        """Send a chat message and wait for the ``message.received`` event
        (matched by peer and text). On timeout re-announce the sender and
        resend, up to ``attempts`` times: GH mac runners occasionally lose
        a single delivery while discovery stays instant. A dead delivery
        path still fails after the last attempt — retries absorb one-off
        packet loss, not breakage."""
        last_error = None
        for _ in range(int(attempts)):
            self.send_message(peer_from, to, text)
            try:
                return self.wait_for_event(
                    "message.received", peer=to, timeout=timeout, text=text
                )
            except AssertionError as e:
                last_error = e
                try:
                    self.announce_peer(peer_from)
                except AssertionError:
                    pass
        raise AssertionError(
            "message '%s' from '%s' to '%s' not delivered after %s attempts: %s"
            % (text, peer_from, to, attempts, last_error)
        )

    @keyword("Shutdown Daemon")
    def shutdown_daemon(self):
        """Ask lmmd to exit (peers send Depart). Connection stays open
        for ``Disconnect From Daemon``."""
        self._request("shutdown")

    # -- events --------------------------------------------------------

    def _poll(self, since):
        res = self._request("events.poll", {"since": since})
        return res.get("events", []), int(res.get("next", since))

    @keyword("Poll Events")
    def poll_events(self, peer=None):
        """One poll at the library cursor; advance it. Returns the event
        list (each a dict with seq, t, event, ...)."""
        params = {"since": self._cursor}
        if peer is not None:
            params["peer"] = peer
        res = self._request("events.poll", params)
        self._cursor = int(res.get("next", self._cursor))
        return res.get("events", [])

    @keyword("Reset Event Cursor")
    def reset_event_cursor(self):
        """Fast-forward the cursor past everything queued so far."""
        _, self._cursor = self._poll(self._cursor)

    @keyword("Wait For Event")
    def wait_for_event(self, event, peer=None, timeout="12s", **matchers):
        """Poll until an event of type ``event`` appears (optionally
        restricted to ``peer`` and to ``field=value`` matchers, compared
        as strings). Advances the cursor. Returns the event dict."""
        deadline = time.monotonic() + timestr_to_secs(timeout)
        while True:
            events, nxt = self._poll(self._cursor)
            self._cursor = nxt
            for ev in events:
                if ev.get("event") != event:
                    continue
                if peer is not None and ev.get("peer") != peer:
                    continue
                if all(str(ev.get(k)) == str(v) for k, v in matchers.items()):
                    return ev
            if time.monotonic() >= deadline:
                raise AssertionError(
                    "event '%s'%s not seen within %s"
                    % (event, " for peer '%s'" % peer if peer else "", timeout)
                )
            time.sleep(self.POLL_INTERVAL)

    @keyword("Wait For Contact")
    def wait_for_contact(self, observer, name, timeout="20s"):
        """Wait until mock ``observer`` sees ``name`` (display name or
        userId) in its contacts, re-announcing per the recommended flow
        in PROTOCOL.md. Returns the contact dict."""
        deadline = time.monotonic() + timestr_to_secs(timeout)
        last_announce = 0.0
        while True:
            for contact in self.peer_contacts(observer):
                if contact.get("name") == name or contact.get("userId") == name:
                    return contact
            if time.monotonic() >= deadline:
                raise AssertionError(
                    "peer '%s' never saw '%s' within %s" % (observer, name, timeout)
                )
            if time.monotonic() - last_announce >= self.ANNOUNCE_INTERVAL:
                self.announce_peer(observer)
                last_announce = time.monotonic()
            time.sleep(self.POLL_INTERVAL)
