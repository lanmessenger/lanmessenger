# lmmd wire protocol (v1)

Transport: `QLocalServer` named `lmm` (override with `lmmd --socket-name`).
Framing: newline-delimited JSON, UTF-8. One line = one message, `\n` terminated.

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
| `status` | — | `{socket, peers:[names], uptimeMs, lmcVersion}` |
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
- `XN_LOGON` in broadcasts reflects the most recently added peer (single
  `$USER` env per process) — display names (`IDS_USERNAME`) are per-peer
  and correct; `userId` routing is unaffected.
- Files, statuses, groups, broadcast messages: not in v1.
- Known defect (core, out of scope for the mock): `~lmcMessaging` frees
  nothing, so `peer.add`/`peer.remove` cycles leak a bounded amount
  (`pNetwork`, `pSettings`, `localUser` per removed peer). Keep scripted
  add/remove loops bounded, or restart the daemon between suites.
