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
        (matched by peer and text). On a delivery timeout both sides are
        re-announced and the send is retried, up to ``attempts`` times: GH
        mac runners occasionally lose a single delivery while discovery
        stays instant. A send-time ``unknown recipient`` (the sender's
        worker lost visibility of the target mid-test) consumes one attempt
        too: both peers are re-announced and the sender waits, bounded by
        ``timeout``, until the target is ready again. A dead delivery path
        still fails after the last attempt — retries absorb one-off loss,
        not breakage.

        Every attempt is printed to the log, so a rising retry rate stays
        visible instead of silently passing."""
        total = int(attempts)
        last_error = None
        for attempt in range(1, total + 1):
            try:
                self.send_message(peer_from, to, text)
            except AssertionError as e:
                last_error = e
                print(
                    "Send Message And Wait: attempt %d/%d not sent (%s); "
                    "re-announcing and waiting for ready" % (attempt, total, e)
                )
                self._repair_delivery_path(peer_from, to, timeout)
                continue
            try:
                return self.wait_for_event(
                    "message.received", peer=to, timeout=timeout, text=text
                )
            except AssertionError as e:
                last_error = e
                print(
                    "Send Message And Wait: attempt %d/%d sent but not "
                    "delivered (%s); re-announcing and resending"
                    % (attempt, total, e)
                )
                self._repair_delivery_path(peer_from, to, timeout)
        raise AssertionError(
            "message '%s' from '%s' to '%s' not delivered after %d attempts: %s"
            % (text, peer_from, to, total, last_error)
        )

    def _repair_delivery_path(self, peer_from, to, timeout):
        """Best-effort repair between send attempts: re-announce both sides
        (a raw userId is not announceable — ignored), then wait, bounded by
        ``timeout``, until the sender sees the target again. Returns
        immediately when already ready; an expired wait is only logged —
        the attempts budget and the final error keep the failure signal."""
        for name in (peer_from, to):
            try:
                self.announce_peer(name)
            except AssertionError:
                pass
        try:
            self.wait_for_contact_ready(peer_from, to, timeout=timeout)
        except AssertionError as e:
            print("Send Message And Wait: still not ready (%s)" % e)

    @keyword("Send File")
    def send_file(self, peer_from, to, path):
        """Send a file from mock ``peer_from``. ``to`` is a peer handle
        or a visible userId, ``path`` an absolute file path readable by
        the daemon. Returns the result dict (fileName, fileSize).
        Delivery is async: watch ``events.poll`` for ``file.request`` on
        the receiver, then ``Accept File``, then ``file.completed``."""
        return self._request("file.send", {"from": peer_from, "to": to, "path": path})

    @keyword("Accept File")
    def accept_file(self, peer, file_id):
        """Accept a pending incoming file on mock ``peer`` (fileId from a
        ``file.request`` event). The worker stores into its own Received
        Files dir; the storage path lands in the ``file.completed`` event."""
        return self._request("file.accept", {"peer": peer, "fileId": file_id})

    @keyword("Decline File")
    def decline_file(self, peer, file_id):
        """Decline a pending incoming file on mock ``peer``."""
        return self._request("file.decline", {"peer": peer, "fileId": file_id})

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
        tail = []
        failed = []
        while True:
            events, nxt = self._poll(self._cursor)
            self._cursor = nxt
            for ev in events:
                if ev.get("event") != event:
                    if len(tail) < 10:
                        tail.append("%s/%s" % (ev.get("event"), ev.get("peer")))
                    if (
                        ev.get("event") == "message.failed"
                        and "text" in matchers
                        and str(ev.get("text")) == str(matchers["text"])
                    ):
                        failed.append(ev)
                    continue
                if peer is not None and ev.get("peer") != peer:
                    continue
                if all(str(ev.get(k)) == str(v) for k, v in matchers.items()):
                    return ev
            if time.monotonic() >= deadline:
                detail = ""
                if failed:
                    detail += "; message.failed x%d" % len(failed)
                if tail:
                    detail += "; other events: %s" % ", ".join(tail)
                raise AssertionError(
                    "event '%s'%s not seen within %s%s"
                    % (event, " for peer '%s'" % peer if peer else "", timeout, detail)
                )
            time.sleep(self.POLL_INTERVAL)

    @keyword("Wait For Contact Announce")
    def wait_for_contact_announce(self, observer, name, timeout="20s"):
        """Wait until mock ``observer`` sees ``name`` in its contacts.
        Announce-level only, not send-ready; use ``Wait For Contact Ready``
        before sending. Returns the contact dict."""
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

    @keyword("Wait For Contact Ready")
    def wait_for_contact_ready(self, observer, name, timeout="30s"):
        """Wait until ``observer`` can send to ``name``: contact visible
        and the message plane up (TCP stream + crypto session,
        ``peer.ready``). Re-announces while waiting. Returns the contact dict."""
        deadline = time.monotonic() + timestr_to_secs(timeout)
        last_announce = 0.0
        contacts = []
        while True:
            try:
                res = self._request("peer.ready", {"peer": observer, "user": name})
            except AssertionError:
                res = {}
            try:
                contacts = self.peer_contacts(observer)
            except AssertionError:
                contacts = []
            if res.get("ready"):
                for contact in contacts:
                    if contact.get("name") == name or contact.get("userId") == name:
                        return contact
            if time.monotonic() >= deadline:
                raise AssertionError(
                    "peer '%s' never ready for '%s' within %s (contacts: %s)"
                    % (
                        observer,
                        name,
                        timeout,
                        [c.get("name") for c in contacts],
                    )
                )
            if time.monotonic() - last_announce >= self.ANNOUNCE_INTERVAL:
                try:
                    self.announce_peer(observer)
                except AssertionError:
                    pass
                last_announce = time.monotonic()
            time.sleep(self.POLL_INTERVAL)
