# BubbleV2 theme.json — schema (stage 1, forward-compatible with stage 2)

One file per theme: `src/resources/themes/BubbleV2/theme.json`, UTF-8, strict JSON
(no comments; explanations go in `_comment` fields). Parser is `QJsonDocument`.

## Top level

| Key | Type | Rule |
|---|---|---|
| `version` | int | must be `1`, otherwise the theme is invalid |
| `bubble` | object | see below |
| `avatar` | object | see below |
| `header` | object | see below |
| `light` / `dark` | object | both palettes must contain the same token set (11); values are hex `#RRGGBB` / `#AARRGGBB` |
| `cards` | object | all 5 required: `incoming/outgoing/broadcast/system/request` |
| `layout_incoming/outgoing/system/broadcast/request` | array | all 5 required; a missing one falls back to `layout_incoming` with a swapped `card` + a log line. In stage 1 they are parsed and validated but ignored (delegate tree is hardcoded 1:1). Stage 2 reads them from the same file with no theme changes. |

Palette tokens (both): `bubbleIn, bubbleIn2, bubbleOut, bubbleOut2, text,
secondary, link, border, page, accent, accentSoft`.

## `bubble`

- `radius`: 4 ints ≥ 0 (order: top-left, top-right, bottom-right, bottom-left).
- `tail`: int ≥ 0 (tail length).
- `shadow`: 4 numbers `[dx, dy, blur, opacity]`, opacity 0..1.
- `padding`: 4 ints ≥ 0 `[top, right, bottom, left]`.
- `spacing`: int ≥ 0; `groupedSpacing`: int ≥ 0.
- `maxWidthPct`: int 1..100 (window width change reflows word-wrap → `sizeHint` invalidation).

## `avatar` / `header`

- `avatar.size`: int > 0; `shape`: `circle|rounded|square`; `showOnGrouped`: bool
  (sugar over `isGrouped`; duplicating `header.ifNot=isGrouped` is allowed).
- `header.senderBold`: bool; `header.timeFormat`: non-empty string.
  `HH:mm` is fixed — a breaking change vs `QLocale::ShortFormat`:
  time rendering no longer depends on the system locale.

## `cards`

- Colors are `$token` references only; hex is forbidden (hex lives in `light`/`dark` only).
- Every `$token` must exist in both palettes, otherwise the theme is invalid.
- `transparent` is allowed only for `system.bg`.
- `tail`: `left|right|none`; `avatar`: bool; `align`: `left|center|right`;
  `border`/`text`: `$token`.

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
