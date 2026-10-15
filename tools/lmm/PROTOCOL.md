# lmmd wire protocol (v1)

Transport: TCP on `127.0.0.1:<port>` (`lmmd --port`, default `$LMMD_PORT`
or `17777`) — one address namespace on every OS, no socket files, no
per-OS path resolution.
Framing: newline-delimited JSON, UTF-8. One line = one message, `\n` terminated.

## Architecture (v2): supervisor + workers

External clients see one daemon and the command table below is unchanged
(v1 wire-compatible: same commands, same shapes, same `seq`/`next`
polling semantics). Inside, `lmmd` is a supervisor: `peer.add NAME`
spawns one worker OS process (the same `lmmd` binary, hidden
`--worker` flags) with its own `HOME` (`<home>/peers/NAME/`), its own
`$USER=NAME` and its own `lmcMessaging` stack. Workers see each other
over the real network (shared UDP discovery, auto TCP ports) — closer
to a two-machine deployment than threads would be.

- Worker control ports are private (each worker binds `127.0.0.1:0`
  and prints `READY <port>` on stdout for the supervisor to dial);
  they speak the same NDJSON protocol but are an implementation detail.
  The worker holds exactly one peer (auto-added at startup, further
  `peer.add` rejected) and takes no `peer` handles — its commands are
  self-scoped. The supervisor translates `peer.*` (user metaphor) to
  worker self-commands; no worker-only command is ever exposed outside.
- Events from all workers are republished by the supervisor with a
  single global `seq`. Worker `peer.added`/`peer.removed` are dropped
  (the supervisor owns those); the rest (`contact.*`,
  `message.received`/`message.failed`) pass through with a new `seq`.
- `peer.remove` sends the worker a `shutdown` (its peer emits `Depart`),
  then reaps the process and deletes `<home>/peers/NAME/`.
- `shutdown` stops every worker (each sends `Depart`), waits up to 5s
  per worker, then exits 0.
- Fail-fast: an unexpected worker death is fatal — the supervisor logs
  peer name, exit code/signal and exits non-zero. No silent masking;
  chaos-style "peer vanished" tests are a future step, not this iteration.
- No status mutation in this iteration: peer status is set once via
  `peer.add {status?}`, exactly as before. There is deliberately no
  `peer.status` command.

## Requests

```json
{"id": 1, "cmd": "peer.add", "params": {"name": "Alice"}}
```

- `id`: client-chosen (number or string), echoed back.
- `cmd`: one of the commands below.
- `params`: object, may be omitted.

## Responses

```json
{"id": 1, "ok": true, "result": {"peer": "Alice", "userId": "..."}}
{"id": 2, "ok": false, "error": "unknown peer"}
```

Malformed lines get `{"id": 0, "ok": false, "error": "invalid json"}`.

## Commands (v1)

| cmd | params | result |
|---|---|---|
| `status` | — | `{port, peers:[names], uptimeMs, lmcVersion}` |
| `peer.add` | `{name, status?}` | `{peer, userId}`. `name`: `1-32` of `A-Za-z0-9_.-`, unique. Starts announce immediately. |
| `peer.list` | — | `{peers: [{peer, userId, displayName, contacts}]}` |
| `peer.contacts` | `{peer}` | `{peer, contacts: [{userId, name, address, status}]}` |
| `peer.announce` | `{peer}` | `{peer, announced: true}` (force re-announce) |
| `peer.remove` | `{peer}` | `{removed: true}` (sends Depart first) |
| `msg.send` | `{from, to, text}` | `{from, to, sent: true}`. `to` = peer handle or visible `userId`. `text`: 1–8192 chars. `sent: true` means accepted for delivery, NOT delivered — watch `events.poll` for `message.received` / `message.failed`. |
| `events.poll` | `{since?, peer?}` | `{events: [...], next}` — events with `seq > since` (default 0). Pass `next` back as `since`; `next` is the highest returned `seq` (0 when empty), so no event is ever skipped between polls. |
| `shutdown` | — | `{stopping: true}`, daemon exits after reply flush (≈200ms), peers send Depart. |

## Events (via `events.poll` only in v1, no server push)

Every event: `{seq, t, event, ...}`. `seq` starts at 1, `t` = epoch ms.

| event | fields |
|---|---|
| `peer.added` | `{peer, userId}` |
| `peer.removed` | `{peer, userId}` |
| `contact.added` | `{peer, userId, name}` — someone's announce seen by `peer` (1s poll granularity) |
| `contact.removed` | `{peer, userId}` |
| `message.received` | `{peer, from, fromName, text}` — `MT_Message` arrived at `peer` |
| `message.failed` | `{peer, to, text}` — stack gave up (10s timeout, 1 retry); re-issue `msg.send` |

## Recommended flows

Add peers, then wait for cross-visibility before sending — a send to a
not-yet-discovered recipient fails fast with `message.failed`:

```
peer.add Alice, peer.add Bob
events.poll / peer.contacts until Alice sees Bob and Bob sees Alice
  (if nothing shows up in a few seconds, peer.announce both and keep polling)
msg.send {from: Alice, to: "Bob", text}
events.poll until message.received on Bob;
  on message.failed (or ~12s silence) re-issue msg.send
```

## Notes / v1 limits

- All peers share one UDP discovery port (50000) and one box, like the
  two-instance setup in `howto/runbook.md`. TCP ports are auto-assigned;
  announces carry the real port, so dial always works.
- Peer identity = MAC + handle. Handles must differ per daemon.
- Each peer owns its `HOME` (`<home>/peers/<name>/`): `lmc.ini`,
  groups, avatar, history and logs are per-peer. Since v2 the old
  shared-`HOME` caveats are gone: no `ini` cross-pollution between
  peers, and `XN_LOGON` is correct per peer (own `$USER` per worker).
- Files, statuses, groups, broadcast messages: not in v1 (status is
  set once at `peer.add`; no late mutation command by design).
- `peer.add`/`peer.remove` cycles spawn/reap an OS process per peer;
  the old in-process `~lmcMessaging` leak note no longer applies —
  removal frees the whole worker address space. Keep scripted
  add/remove loops bounded anyway (process spawn is not free).
