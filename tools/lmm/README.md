# lmm — LAN Messenger Mock

Test stand on the real network stack (`lmcMessaging`/`lmcNetwork` from
`lmc_core`), controlled from the outside. Fills the gap between Qt unit
tests and full-GUI Robot runs: scriptable peers, contact lists, message
exchange — no X server, no `xdotool`.

- `lmmd` — daemon holding N peers in one process (this task).
- `lmm` — thin CLI client (follow-up task).
- Robot Framework keywords — later, on top of `lmm`.

Protocol: [PROTOCOL.md](PROTOCOL.md).

## Build

From the `lmc` repo root (single-threaded only — parallel builds hang the VM):

```sh
cmake -S . -B build   # once; picks up tools/lmm
cmake --build build --target lmmd
```

Binary lands in `build/tools/lmm/lmmd`.

## Run

```sh
./build/tools/lmm/lmmd                  # socket "lmm", fresh temp HOME
./build/tools/lmm/lmmd --socket-name test1 --home /tmp/lmm-home
```

## Probe without CLI (raw socket)

```sh
python3 - <<'EOF'
import json, socket
s = socket.socket(socket.AF_UNIX, socket.SOCK_STREAM)
s.connect("/tmp/lmm")  # QLocalServer default path; see note below
def rpc(cmd, **params):
    s.sendall((json.dumps({"id": 1, "cmd": cmd, "params": params}) + "\n").encode())
    return json.loads(s.makefile().readline())
print(rpc("status"))
print(rpc("peer.add", name="Alice"))
print(rpc("peer.add", name="Bob"))
EOF
```

> QLocalServer socket path: Qt puts `lmm` under `$XDG_RUNTIME_DIR` if set,
> else `/tmp`. If connect fails, find it with
> `find /tmp /run/user/$UID -name 'lmm' 2>/dev/null`.

Typical scripted check: `peer.add Alice/Bob` → poll `peer.contacts`
until cross-visible → `msg.send {from: Alice, to: Bob}` →
`events.poll` until `message.received` on Bob.
