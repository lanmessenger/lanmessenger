---
title: Chat themes
description: Where custom chat themes live on each OS and how to add one.
---

Custom chat themes are JSON files you drop into a folder. No restart
needed: close and reopen Preferences and the new theme appears in the
Theme list.

Note: the theme format is new and may change in future versions. If a
theme stops showing up after an update, compare it with one of the
shipped themes below.

## Folder for custom themes

Put `<Name>.json` files here (create the folder if it is missing).
The file name becomes the theme name: `MyTheme.json` is theme
`MyTheme`. A custom file with the same name replaces the built-in one.

| OS | Folder |
|---|---|
| Windows | `%LOCALAPPDATA%\LAN Messenger\LAN Messenger\themes` |
| Linux | `~/.local/share/lmc/lmc/themes` |
| macOS | `~/Library/Application Support/lmc/lmc/themes` |

Tip: it is the same folder area as `lanmessenger.db` (chat history)
and `logs` — look for those if the path above does not match.

## Adding a theme

1. Start from a working theme — copy one of the shipped ones and
   rename it, e.g. to `MyTheme.json`:
   [ClassicSlate](https://github.com/lanmessenger/lanmessenger/blob/master/src/resources/themes/ClassicSlate.json),
   [MeshBlue](https://github.com/lanmessenger/lanmessenger/blob/master/src/resources/themes/MeshBlue.json),
   [NordicTerminal](https://github.com/lanmessenger/lanmessenger/blob/master/src/resources/themes/NordicTerminal.json).
2. Change `"displayName"` to the label you want to see in the list
   (one line, up to 80 characters). Without it, the file name is
   shown.
3. Put the file in the folder from the table, reopen Preferences,
   open the Theme page, pick the theme and check the preview. Save
   to apply.

The exact rules for every field live next to the themes:
[SCHEMA.md](https://github.com/lanmessenger/lanmessenger/blob/master/src/resources/themes/SCHEMA.md).

## If the theme does not appear

- The JSON must be valid (watch for trailing commas) and
  `"version"` must be `1`.
- All seven `layout_*` blocks must be present, each with exactly one
  message block (`body`, or `fileCard` in `layout_request`), and
  each of those needs `linkColor`.
- Broken files are skipped without an error popup. Run the app with
  `/trace` and look for `Theme:` lines in the log to see the reason.
