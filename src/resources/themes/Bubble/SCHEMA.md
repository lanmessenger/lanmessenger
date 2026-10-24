# Bubble theme.json — schema (stage 1, forward-compatible with stage 2)

One dir per theme: `src/resources/themes/<Name>/theme.json`, UTF-8, strict JSON
(no comments; explanations go in `_comment` fields). Parser is `QJsonDocument`.

## Top level

| Key | Type | Rule |
|---|---|---|
| `version` | int | must be `1`, otherwise the theme is invalid |
| `viewPadding` | array | optional 4 ints ≥ 0 `[top, right, bottom, left]`, list chrome (default `[0, 8, 8, 8]`); rows lay out from the viewport origin, the view applies these as viewport margins |
| `bubble` | object | see below |
| `avatar` | object | see below |
| `header` | object | see below |
| `light` / `dark` | object | both palettes must contain the same required token set (12); values are hex `#RRGGBB` / `#AARRGGBB` |
| `cards` | object | all 5 required: `incoming/outgoing/broadcast/system/request` |
| `layout_incoming/outgoing/system/broadcast/request` | array | all 5 required; a missing one falls back to `layout_incoming` with a swapped `card` + a log line. In stage 1 they are parsed and validated but ignored (delegate tree is hardcoded 1:1). Stage 2 reads them from the same file with no theme changes. |

Palette tokens (both): `bubbleIn, bubbleIn2, bubbleOut, bubbleOut2, text,
secondary, link, border, page, accent, accentSoft, textInverse`.
`textInverse` is body text for saturated bubbles (e.g. white on a blue
outgoing); cards that do not need it simply omit `text`.

Optional palette tokens (both or neither, same hex rule): `pill, pillBorder`
— system message pill background and border. Old 12-token files stay valid;
cards referencing a missing optional token invalidate the theme.

## `bubble`

- `radius`: 4 ints ≥ 0 (order: top-left, top-right, bottom-right, bottom-left).
  `radiusNextInGroup`: optional 4-int array overriding `radius` inside a series
  (grouped rows), same technique as the positional margins. `radiusFirstInGroup`
  is parsed but reserved: ungrouped rows use `radius`. Arrays are written in
  incoming orientation (tail left); `outgoing` rows mirror them horizontally.
  A missing key falls back to `radius`; an explicit non-number array invalidates
  the theme.
- `tail`: int ≥ 0 (tail length).
- `tailStyle`: `polygon|rounded|none` (default `polygon`). `polygon` draws the
  triangle; `rounded` implies the tail via the small corner in `radius` (no
  polygon, per the Stitch spec); `none` draws no tail. `rounded` and `none`
  render the same path: per-corner radii apply to the bubble in both cases.
- `shadow`: 4 numbers `[dx, dy, blur, opacity]`, opacity 0..1.
- `padding`: 4 ints ≥ 0 `[top, right, bottom, left]`.
- `margin`: int ≥ 0, top gap of a row. `marginFirstInGroup` /
  `marginNextInGroup`: optional ints ≥ 0 overriding `margin` for the series
  start (ungrouped row: single or first of a series) vs inside a series
  (grouped rows). The gap after a series end is the next row's top margin,
  so no "last in group" key exists. An explicit non-number invalidates the
  theme; a missing positional key falls back to `margin`.
- `maxWidthPct`: int 1..100 (window width change reflows word-wrap → `sizeHint` invalidation).

## `avatar` / `header`

- `avatar.size`: int > 0; `shape`: `circle|rounded|square`; `showOnGrouped`: bool
  (sugar over `isGrouped`; duplicating `header.ifNot=isGrouped` is allowed).
  `avatar.gap`: optional int ≥ 0 (default `8`), avatar-to-bubble gap.
- `header.gap`: optional int ≥ 0 (default `2`), header-to-bubble gap.
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
- `header.senderColor` / `header.timeColor`: optional `$token` refs
  (default `$secondary`), must exist in both palettes like card colors; hex is
  forbidden here. Shipped Stitch themes use `$text` for the name.

## `cards`

- Colors are `$token` references only; hex is forbidden (hex lives in `light`/`dark` only).
- Every `$token` must exist in both palettes, otherwise the theme is invalid.
- `transparent` is allowed only for `system.bg`; `system.bg` also accepts a
  `$token` (pill background, e.g. `$pill`), optionally with `border` (e.g.
  `$pillBorder`). Omitted `border` keeps the old borderless system line.
- `tail`: `left|right|none`; `avatar`: bool; `align`: `left|center|right`;
  `border`/`text`: `$token`.
- Body text fallback chain: explicit per-message color, then the card's `text`
  (when present), then the global `text` token. `system.text` defaults to
  `$secondary` in the shipped themes.

## `layout_*` (L-MVP boundary, do not extend without a separate decision)

- Node: `{ "type": ..., "if": <flag>, "ifNot": <flag>, ... }`.
- Containers are `vbox/hbox` only (`gap, align, stretch`), depth ≤ 4 from the root
  (`hbox=1, vbox=2, bubble=3, body/fileCard=4`; `body` counts as a level).
- Blocks are only: `avatar, header, bubble, body, linkPreview, fileCard, quote,
  code, status, buttons`.
- S implements the `avatar/header/bubble/body/fileCard/status` subset;
  `linkPreview` (no link previews at all), `quote/code/buttons` are
  reserved for L: the validator accepts, the S delegate ignores.
- Flag sources: `SingleMessage`, `sendFileMap/receiveFileMap` for `fileCard`,
  model flags. `if`/`ifNot` only on model flags:
  `isGrouped, hasPreview, isFile, isOwn, isOwnLast`.
- `header.items`: a subset of `["sender", "time"]`.
- `bubble.card`: one of the 5 `cards` keys.

## Errors

Invalid JSON, `version != 1`, missing layout, unknown `$token`,
hex in `cards`, `transparent` outside `system.bg`, depth > 4, unknown
`type`/`if` — the theme is invalid: built-in default + a log line, chat stays up.
Validator unit test lives in S (otherwise it drifts from L).
