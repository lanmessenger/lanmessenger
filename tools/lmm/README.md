# lmm — LAN Messenger Mock

Test stand on the real network stack (`lmcMessaging`/`lmcNetwork` from
`lmc_core`), controlled from the outside. Fills the gap between Qt unit
tests and full-GUI Robot runs: scriptable peers, contact lists, message
exchange — no X server, no `xdotool`.

- `lmmd` — supervisor daemon: one external socket, one worker OS process
  per peer (same binary, hidden `--worker` flags). Workers see each other
  over the real network, each with its own `HOME` (`<home>/peers/<name>/`)
  and `$USER` — closer to a two-machine deployment than threads.
- `lmm` — thin CLI client (follow-up task).
- Robot Framework keywords — `tests/robot/` (`lmm/LmmLibrary.py` speaks
  the protocol; `lmm/smoke.robot` self-check, `lmm/isolation.robot`
  per-peer state, `lmm/crash.robot` fail-fast, top-level `chat.robot`
  real app vs mock, `files.robot` large-file regression for issue #3).

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
./build/tools/lmm/lmmd                  # 127.0.0.1:17777, fresh temp HOME
./build/tools/lmm/lmmd --port 17771 --home /tmp/lmm-home
```

`--home` is the base dir; each peer gets `<home>/peers/<name>/` as its
own `HOME` (config, groups, avatar, history, logs). Worker identity
(`userId = MAC + logon name`) comes from the peer name: `USER` on Unix,
`LMC_LOGON_NAME` on Windows (where `GetUserName` reads the process token
and ignores the environment — without the hook all workers collide).
Settings follow the same isolation: XDG layout on Unix, `QSettings::setPath`
to the peer dir on Windows (reported via `status` `settingsFile`).
`peer.remove` deletes its dir; an unexpected worker death exits the daemon
non-zero (fail-fast — see `PROTOCOL.md`, no silent masking).

## Probe without CLI (raw TCP)

```sh
python3 - <<'EOF'
import json, socket
s = socket.create_connection(("127.0.0.1", 17777))
def rpc(cmd, **params):
    s.sendall((json.dumps({"id": 1, "cmd": cmd, "params": params}) + "\n").encode())
    return json.loads(s.makefile().readline())
print(rpc("status"))
print(rpc("peer.add", name="Alice"))
print(rpc("peer.add", name="Bob"))
EOF
```

Typical scripted check: `peer.add Alice/Bob` → poll `peer.contacts`
until cross-visible → `msg.send {from: Alice, to: Bob}` →
`events.poll` until `message.received` on Bob.
