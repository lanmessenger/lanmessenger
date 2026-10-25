# theme.json — schema (colors are literals on layout nodes)

One file per theme: `src/resources/themes/<Name>.json`, UTF-8, strict JSON
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
| `displayName` | string | optional UI label (single line, max 80 chars); absent = file basename |
| `viewPadding` | array | optional 4 ints ≥ 0 `[top, right, bottom, left]`, list chrome (default `[0, 8, 8, 8]`); rows lay out from the viewport origin, the view applies these as viewport margins, filled with the `page` color |
| `avatar` | object | see below |
| `header` | object | see below |
| `text` | color | default body text, both forms |
| `link` | color | links and the focus ring, both forms |
| `page` | color | view fill, both forms |
| `layout_incoming/outgoing[/_next]/system/broadcast/request` | object | all 7 required; each layout is a single root object holding exactly one rect: one `body` or `fileCard` node (the delegate always paints one). `layout_incoming_next`/`layout_outgoing_next` carry the in-series look: no header/avatar nodes, dense top gap. Missing layout falls back to `layout_incoming` + a log line. |

Key rule: structural keys (`type`, layout name, `gap`,
global `text`/`link`/`page`,
`header.senderColor`/`header.timeColor`) are required; overrides
(node `senderColor`/`timeColor`/`textColor`/`border`/`bg`/
`corners`/`padding`/`shadow`/`marginTop`/`maxWidthPct`,
`viewPadding`) are optional-with-default
(absent `bg` = `transparent`; absent `padding`/`shadow`/`marginTop` =
zeros: no padding, no shadow, no top gap; absent `maxWidthPct` =
`100`: full width). Rows are tailless: any `tail`/`tailSize`/`tailStyle`
key invalidates the theme.

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

- Node: `{ "type": ... }`.
- Containers are `vbox/hbox` only; every container carries a required
  `gap: int ≥ 0` — the only spacing source between siblings (no theme-level
  gaps exist). Depth ≤ 4 from the root. Only `layout_incoming` is
  `hbox`-rooted (the avatar sits beside the column); every other layout
  is `vbox`-rooted (`vbox=1, body=2`, with `header` alongside for
  series-start trees).
- Blocks are only: `avatar, header, body, linkPreview, fileCard, quote,
  code, buttons`.
- S implements the `avatar/header/body/fileCard` subset;
  `linkPreview` (no link previews at all), `quote/code/buttons` are
  reserved: the validator accepts, the S delegate ignores them.
- One rect per layout: exactly one `body` xor `fileCard` node — message
  and file never stack in one layout. `body`/`fileCard` are leaves and
  must not carry `children`.
- `layout_request` is the file layout: its rect is a `fileCard`
  (row selection already guarantees a file row) and no `body`.
  `fileCard` anywhere else, or `body` in `layout_request`, invalidates the theme.
  `layout_incoming`/`layout_outgoing` carry no `fileCard` branches.
- Series position selects the tree: a series-start
  (or single/ungrouped) row walks `layout_incoming`/`layout_outgoing`,
  an in-series row walks `layout_incoming_next`/`layout_outgoing_next`.
  The `_next` trees omit the header/avatar nodes and carry the dense
  `marginTop` and in-series `corners`; any other per-tree property may
  diverge too (no new code needed).
- `header.items`: a subset of `["sender", "time"]` (empty = role default:
  `incoming` shows both, `outgoing` shows time).
- Rect (`body`/`fileCard`) `bg`: optional — solid, transparent, or
  per-mode gradient pair (default `transparent`: flat text on the page).
- Rect `textColor` / `border`: optional literal overrides.
- Rect metrics: `padding` (4 ints ≥ 0 `[top, right, bottom,
  left]`, default `[0, 0, 0, 0]`), `shadow` (`[dx, dy, blur, opacity]`,
  opacity 0..1; black shadow, tinted shadows are not supported by the
  renderer; default `[0, 0, 0, 0]` = no shadow), `marginTop`
  (int ≥ 0, default `0`, top gap of a row; series-start trees carry the
  wide gap, `_next` trees the dense in-series one), `maxWidthPct`
  (optional int 1..100, default `100` = full width; window width change
  reflows word-wrap → `sizeHint`
  invalidation). The gap after a series end is the next row's top
  margin, so no "last in group" key exists. A non-int `marginTop`
  or a non-number anywhere invalidates the theme.
- Rows are tailless: no tail triangle is drawn; any `tail`/`tailSize`/
  `tailStyle` key on any node invalidates the theme.
- `header` node `senderColor` / `timeColor`: optional literal overrides
  of the theme header defaults.
- Color keys on any other node type invalidate the theme.
- Rect `corners`: optional 4 ints ≥ 0 `[TL, TR, BR, BL]` (default
  square `[0, 0, 0, 0]`); `corners` on any other node
  or a short/negative/fractional array invalidates the theme.
  Outgoing nodes carry hand-mirrored values — no code mirror exists, what is
  written is what renders. Each layout's rect node may carry its own corners;
  node corners never inherit across layouts.

## Errors

Invalid JSON, `version != 1`, missing layout, layout as array, layout without a rect,
bad color form, mixed solid/gradient pair, `transparent` outside rect `bg`,
gradient outside `bg`, color/metric keys on the wrong node, `tail` keys
present, bad rect numbers, depth > 4, unknown `type`, bad `corners` —
the theme is invalid: built-in default + a log line, chat stays up.
Validator unit test lives in S (otherwise it drifts from L).
