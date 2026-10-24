# Bubble theme.json — schema (colors are literals on layout nodes)

One dir per theme: `src/resources/themes/<Name>/theme.json`, UTF-8, strict JSON
(no comments; explanations go in `_comment` fields). Parser is `QJsonDocument`.

To answer "what color is this row", open its `layout_*` and read the node:
no token maps, no card section, no jumps.

## Color forms

Three forms, everywhere a color is accepted:

| Form | Meaning | Example |
|---|---|---|
| `"#hex"` | both modes (`#RRGGBB` / `#AARRGGBB`) | `"textColor": "#FFFFFF"` |
| `["light", "dark"]` | per-mode pair, same hex rule | `"border": ["#BAE6FD", "#1E3A5F"]` |
| `["gradient(#from, #to)", "gradient(#from, #to)"]` | per-mode vertical gradient, top to bottom | `"bg": ["gradient(#FFFFFF, #F0FDF4)", "gradient(#162235, #111A29)"]` |

- `gradient(#from, #to)`: exactly two bare hex stops, whitespace around
  tokens is tolerated. A third stop, a missing paren, or a non-hex stop
  invalidates the theme.
- `"transparent"` is allowed only as a body/fileCard `bg` (system look).
- A pair mixes either two solids or two gradients, never one of each.
- Gradients are accepted only for `bg`. `border`/`textColor`/`senderColor`/
  `timeColor` take the solid forms only.

## Top level

| Key | Type | Rule |
|---|---|---|
| `version` | int | must be `1`, otherwise the theme is invalid |
| `viewPadding` | array | optional 4 ints ≥ 0 `[top, right, bottom, left]`, list chrome (default `[0, 8, 8, 8]`); rows lay out from the viewport origin, the view applies these as viewport margins |
| `avatar` | object | see below |
| `header` | object | see below |
| `text` | color | default body text, both forms |
| `link` | color | links and the focus ring, both forms |
| `page` | color | view fill, both forms |
| `layout_incoming/outgoing[/_next]/system/broadcast/request` | array | all 7 required and non-empty; every layout holds exactly one rect: one `body` or `fileCard` node (the delegate always paints one). `layout_incoming_next`/`layout_outgoing_next` carry the in-series look: no header/avatar nodes, dense top gap. Missing layout falls back to `layout_incoming` + a log line. |

Key rule: structural keys (`type`, layout name, `gap`, rect `bg`,
rect `padding`/`shadow`/`margin`/`maxWidthPct`, `corners`,
global `text`/`link`/`page`, `header.senderColor`/`header.timeColor`)
are required; overrides (node
`senderColor`/`timeColor`/`textColor`/`border`) are
optional-with-default. Rows are tailless: any `tail`/`tailSize`/
`tailStyle` key invalidates the theme.

## `avatar` / `header`

- `avatar.size`: int > 0; `shape`: `circle|rounded|square`.
- `header.senderBold`: bool; `header.timeFormat`: non-empty `QDateTime::toString`
  format, locale-independent (breaking change vs `QLocale::ShortFormat`).
  Shipped themes use `HH:mm`; `NordicTerminal` uses `HH:mm:ss` as a seconds example.
- `header.outgoingYouLabel`: optional bool (default `false`), prefix for the time
  in the outgoing header with the translated "You" label (e.g. `"You · 10:43"`).
  The word itself is not theme data: the delegate renders it via `tr()` in
  `BubbleDelegate`, so translators handle it through the `.ts`/`.qm` workflow.
  `false` keeps the old time-only header. The "You" prefix is always regular;
  `senderBold` styles the incoming sender name only.
- `header.senderSize` / `header.timeSize`: optional numbers 0.5..2.0
  (default `1.0`), multipliers of the local display size from the user's own
  font-size setting — never absolute pt, so the setting keeps working. The
  sender-chosen message font never leaks into the header.
- `header.senderColor` / `header.timeColor`: required literal defaults
  (solid forms); layout `header` nodes override them per row (see below).
- Body text fallback chain: explicit per-message color, then the layout
  body/fileCard node's `textColor` (when present), then the global `text`.

## `layout_*` (L-MVP boundary, do not extend without a separate decision)

- Node: `{ "type": ..., "if": <flag>, "ifNot": <flag>, ... }`.
- Containers are `vbox/hbox` only; every container carries a required
  `gap: int ≥ 0` — the only spacing source between siblings (no theme-level
  gaps exist). Depth ≤ 4 from the root
  (`hbox=1, vbox=2, body=3`).
- Blocks are only: `avatar, header, body, linkPreview, fileCard, quote,
  code, buttons`.
- S implements the `avatar/header/body/fileCard` subset;
  `linkPreview` (no link previews at all), `quote/code/buttons` are
  reserved: the validator accepts, the S delegate ignores them.
- One rect per layout: exactly one `body` xor `fileCard` node — message
  and file never stack in one layout. `body`/`fileCard` are leaves and
  must not carry `children`.
- `layout_request` is the file layout: its rect is a `fileCard` with no
  conditions (row selection already guarantees a file row) and no `body`.
  `fileCard` anywhere else, or `body` in `layout_request`, invalidates the theme.
  `layout_incoming`/`layout_outgoing` carry no `fileCard` branches.
- Series position selects the tree, never a node flag: a series-start
  (or single/ungrouped) row walks `layout_incoming`/`layout_outgoing`,
  an in-series row walks `layout_incoming_next`/`layout_outgoing_next`.
  The `_next` trees omit the header/avatar nodes and carry the dense
  `margin` and in-series `corners`; any other per-tree property may
  diverge too (no new code needed).
- Flag sources: `SingleMessage`, `sendFileMap/receiveFileMap` for `fileCard`,
  model flags. `if`/`ifNot` only on model flags:
  `hasPreview, isFile, isOwn` (`isGrouped` is gone: series
  position selects the tree, it never gates a node).
- `header.items`: a subset of `["sender", "time"]` (empty = role default:
  `incoming` shows both, `outgoing` shows time).
- Rect (`body`/`fileCard`) `bg`: required — solid, transparent, or
  per-mode gradient pair.
- Rect `textColor` / `border`: optional literal overrides.
- Rect metrics, required: `padding` (4 ints ≥ 0 `[top, right, bottom,
  left]`), `shadow` (`[dx, dy, blur, opacity]`, opacity 0..1; black
  shadow, tinted shadows are not supported by the renderer), `margin`
  (int ≥ 0, top gap of a row; series-start trees carry the wide gap,
  `_next` trees the dense in-series one), `maxWidthPct`
  (int 1..100; window width change reflows word-wrap → `sizeHint`
  invalidation). The gap after a series end is the next row's top
  margin, so no "last in group" key exists. A non-int `margin`
  or a non-number anywhere invalidates the theme.
- Rows are tailless: no tail triangle is drawn; any `tail`/`tailSize`/
  `tailStyle` key on any node invalidates the theme.
- `header` node `senderColor` / `timeColor`: optional literal overrides
  of the theme header defaults.
- Color keys on any other node type invalidate the theme.
- Rect `corners`: 4 ints ≥ 0 `[TL, TR, BR, BL]`. A rect without
  `corners` uses the built-in default; `corners` on any other node
  or a short/negative/fractional array invalidates the theme.
  Outgoing nodes carry hand-mirrored values — no code mirror exists, what is
  written is what renders. Each layout's rect node may carry its own corners
  (e.g. a system pill); node corners never inherit across layouts.

## Errors

Invalid JSON, `version != 1`, missing layout, layout without a rect,
bad color form, mixed solid/gradient pair, `transparent` outside rect `bg`,
gradient outside `bg`, color/metric keys on the wrong node, `tail` keys
present, bad rect numbers, depth > 4, unknown `type`/`if`, bad `corners` —
the theme is invalid: built-in default + a log line, chat stays up.
Validator unit test lives in S (otherwise it drifts from L).
