---
title: INI file reference
description: Location, format rules, and key-by-key reference for the LAN Messenger preferences file.
---

This page documents the preferences file LAN Messenger reads and writes
at runtime: where it lives, how to edit it safely, and what every
section and key means.

Most settings never need hand editing. Everything in this file can be
changed from the Preferences dialog, which is the recommended way to
change them. Use this reference when you need to pre-seed a
configuration, troubleshoot, or deploy the same settings to many
machines.

## Location

The file uses Qt `IniFormat` with `UserScope`, resolved from the
organization and application names:

| Platform | Organization / Application | Typical path |
|---|---|---|
| Windows | `LAN Messenger` / `LAN Messenger` | `%APPDATA%\LAN Messenger\LAN Messenger.ini` |
| Linux | `lmc` / `lmc` | `~/.config/lmc/lmc.ini` |
| macOS | `lmc` / `lmc` | Resolved by Qt from the names above |

Related data lives next to it, not in it:

| File | Contents |
|---|---|
| `group.cfg` (application data directory) | Contact groups and user-to-group mapping (see [Groups](#groups-separate-file) below) |
| `messenger.db` (application data directory) | Chat history database |
| `transfers.lst` (application data directory) | File transfer history |
| `avt_local.png` (application data directory) | Custom avatar image |

On Linux the application data directory is typically
`~/.local/share/lmc/lmc/`.

## Format rules

- **The file is sparse.** A key is stored only when its value differs
  from the built-in default. A missing key means "use the default";
  deleting a key resets that setting. Do not assume a short file is
  broken.
- **Booleans** are lowercase: `true` / `false`.
- **Strings containing commas or special characters** are
  double-quoted by Qt, for example
  `Font="Calibri,12,-1,5,50,0,0,0,0,0"`. Keep the quotes when editing.
- **Window geometry** is stored as an opaque blob, for example
  `Main=@ByteArray(...)`. Never hand-edit these values; delete the key
  to reset a window to its default size and position.
- **Lists** use Qt array syntax: a `size` entry plus numbered keys,
  for example:
  ```ini
  [BroadcastList]
  size=1
  1\Broadcast=192.168.1.255
  ```
- **Quit the application before editing.** The application stamps the
  file on exit (version, presence, avatar, window geometry), so edits
  made while it runs can be overwritten.
- The Preferences dialog has a **Reset** button that deletes the file
  and restores defaults. The `/noconfig` command line switch does the
  same (it also removes the custom avatar image).

## General

Keys without a section prefix.

| Key | Type / Default | Meaning |
|---|---|---|
| `AutoStart` | bool, `true` | Start at login. Mirrored to the platform mechanism (Windows Run key, Linux `lmc.desktop` in `~/.config/autostart`). Not implemented on macOS; the checkbox is hidden there. |
| `AutoShow` | bool, `false` | Show the main window at startup instead of starting minimized. |
| `RefreshInterval` | int seconds, `300` | How often the contacts list is refreshed. Values below 300 are raised to 300 when upgrading from very old versions. |

## Application

| Key | Type / Default | Meaning |
|---|---|---|
| `Version` | string | Internal migration marker. Always overwritten with the running build version on save, exit, and import. Used to migrate (or, for a newer-than-running file, discard) old settings. |
| `OpenPath` | path, Documents folder | Last directory used by the open-file dialog. Updated automatically. |
| `SavePath` | path, Documents folder | Last directory used by the save-file dialog. Updated automatically. |

## Locale

| Key | Type / Default | Meaning |
|---|---|---|
| `Language` | string, `en_US` | Interface language (locale code). |

## SystemTray

| Key | Type / Default | Meaning |
|---|---|---|
| `SysTray` | bool, `true` | Show the system tray icon. |
| `MinimizeTray` | bool, `false` | Minimizing the main window sends it to the tray. |
| `SingleClickTray` | bool, `false` | Single click (instead of double click) activates the tray icon. |
| `MinimizeMsg` | bool, `true` | Show the "window was minimized" hint. Managed automatically; there is no checkbox for it. |
| `SysTrayMsg` | bool, `true` | Show tray notification popups. |
| `AllowMinimize` | bool, `false` | Allow minimizing the main window when the tray icon is disabled. |

## Messages

| Key | Type / Default | Meaning |
|---|---|---|
| `Emoticon` | bool, `true` | Render smileys as images. |
| `MessageTime` | bool, `true` | Show the time next to messages. |
| `MessageDate` | bool, `false` | Show the date next to messages. |
| `AllowLinks` | bool, `true` | Turn URLs into clickable links. |
| `PathToLink` | bool, `false` | Turn file paths into clickable links. |
| `Trim` | bool, `true` | Trim leading/trailing whitespace from messages. |
| `ClearOnClose` | bool, `true` | Clear the message log when a chat window closes. |
| `MessageTop` | bool, `false` | `true` puts newest messages at the top, `false` at the bottom. |
| `PubMessagePop` | bool, `false` | Open a popup window for incoming public (room) messages. |
| `Font` | string | Message font, serialized by Qt (`QFont.toString()`). Set it with the font button in Preferences; the embedded numbers are Qt internals, not a point size to edit by hand. |
| `Color` | string, palette text color | Message text color as `#rrggbb`. Set it with the color button in Preferences. |
| `FontSize` | int, `1` | Message text size step: `0` Small, `1` Medium, `2` Large (rendered at approximately 8.25pt, 9pt and 10.5pt). Values outside `0`–`2` are clamped on load. |

## History

| Key | Type / Default | Meaning |
|---|---|---|
| `History` | bool, `true` | Save chat history to `messenger.db`. |
| `SysHistoryPathDef` | bool, `true` | `true` keeps the database at its default location; `false` uses `HistoryPath`. |
| `HistoryPath` | path, empty | Custom history database path. Used only when `SysHistoryPathDef` is `false`. |
| `FileHistory` | bool, `true` | Keep the file transfer list (`transfers.lst`). |

## Alerts

`Alert` controls tray popups, `Sound` is the master switch for sounds.
The `NoBusy*` / `NoDND*` keys suppress notifications while your own
status is Busy or Do Not Disturb.

| Key | Type / Default | Meaning |
|---|---|---|
| `Alert` | bool, `true` | Show popup alerts. |
| `NoBusyAlert` | bool, `false` | Suppress popups while Busy. |
| `NoDNDAlert` | bool, `true` | Suppress popups while Do Not Disturb. |
| `Sound` | bool, `true` | Master switch for event sounds. |
| `NoBusySound` | bool, `false` | Suppress sounds while Busy. |
| `NoDNDSound` | bool, `true` | Suppress sounds while Do Not Disturb. |

## SoundEvents and SoundFiles

Per-event sound switches and files, stored as arrays with entries
`1`–`6` in this fixed order:

1. Incoming message
2. User is online
3. User is offline
4. Incoming file transfer
5. File transfer completed
6. Incoming public message

```ini
[SoundEvents]
size=6
1\Event=2
2\Event=0
...
```

| Section | Entry | Meaning |
|---|---|---|
| `SoundEvents` | `N\Event` | `2` = play this event's sound, `0` = muted. Defaults to enabled for all events; the section disappears when every event is enabled. |
| `SoundFiles` | `N\File` | Sound file for event `N`. Defaults to the bundled `./sounds/*.wav` files (public messages reuse the incoming-message sound). The section disappears when every file is the default. |

## Connection

| Key | Type / Default | Meaning |
|---|---|---|
| `Timeout` | int seconds, `10` | Acknowledgement timeout for message delivery. |
| `MaxRetries` | int, `1` | Delivery retries after a timeout. |
| `Multicast` | IP, `239.255.100.100` | Multicast group used for presence and messaging. |
| `UDPPort` | int, `50000` | UDP port. A per-launch `/udpport=<port>` switch overrides the file. |
| `TCPPort` | string, empty (auto) | TCP port for file transfers and messages. Empty means automatic: try `50000`, fall back to any free port if it is busy. Set an explicit value to require exactly that port. |

## BroadcastList

Extra broadcast addresses, one per entry:

```ini
[BroadcastList]
size=1
1\Broadcast=192.168.1.255
```

The subnet broadcast and the default limited broadcast are always used
in addition to this list. The section disappears when the list is
empty.

## FileTransfer

| Key | Type / Default | Meaning |
|---|---|---|
| `AutoFile` | bool, `false` | Accept incoming files automatically. |
| `AutoShow` | bool, `true` | Open the transfer window automatically on activity. |
| `FileTop` | bool, `false` | `true` shows newest transfers at the top, `false` at the bottom. |
| `StoragePath` | path, empty | Download directory. Empty means `Received Files` inside the Documents folder. |

## Appearance

| Key | Type / Default | Meaning |
|---|---|---|
| `Theme` | string, `:/themes/Classic` | Message theme: a bundled resource path or a directory on disk. Preview it in Preferences before switching. |
| `UserListView` | int, `0` | `0` Detailed, `1` Compact contact list. |
| `StatusToolTip` | bool, `false` | Include presence status in contact tooltips. |

## Hotkeys

| Key | Type / Default | Meaning |
|---|---|---|
| `SendKeyMod` | bool, `false` | `false`: Enter sends the message. `true`: Ctrl+Enter sends it (Cmd+Enter on macOS). |

## User

Identity and presence. `Status`, `Avatar` and `Note` are written on
exit; the rest come from the Account page of Preferences.

| Key | Type / Default | Meaning |
|---|---|---|
| `Status` | string, `chat` | Last presence: `chat` (Available), `busy`, `dnd` (Do Not Disturb), `brb` (Be Right Back), `away`, `gone` (Appear Offline). Unrecognized values fall back to Available. |
| `Avatar` | int, `65535` (none) | Built-in avatar index (`0`–`19`). A custom picture is stored separately as `avt_local.png`. |
| `Name` | string, empty | Display name. Empty means the OS logon name. |
| `FirstName` / `LastName` / `About` | strings, empty | Profile details sent when another user requests your info. |
| `Note` | string, empty | Status note, set from the main window. |

## GroupExp

Expanded/collapsed state of the contact-group tree, one boolean per
top-level row (`1\Group`, `2\Group`, … plus `size`). Managed
automatically; there is no UI for it.

## Window and Splitter

Saved size, position and splitter layout of each window. All values
are opaque `@ByteArray(...)` blobs written automatically when a
window that was opened at least once closes:

`Window/Main`, `Window/Transfers`, `Window/History`,
`Window/Broadcast`, `Window/Help`, `Window/PublicChat`,
`Splitter/History`, `Splitter/Broadcast`, `Splitter/PublicChatH`,
`Splitter/PublicChatV`.

## Groups (separate file)

Contact groups are not stored in the preferences file. They live in
`group.cfg` in the application data directory:

```ini
[Groups]
size=1
1\Group=<group-id>
1\GroupName=General

[GroupMap]
size=1
1\User=<user-id>
1\Group=<group-id>
```

The file always contains at least the default `General` group and is
managed automatically.

## Command line switches affecting the file

| Switch | Effect |
|---|---|
| `/config=<file>` | Imports a defined subset of keys (general, account, message, history, alert, network, file-transfer and appearance settings). Sound selections, custom sound files, window geometry, identity state, storage and history paths, and group data are not imported. |
| `/noconfig` | Deletes the preferences file and the custom avatar, restoring defaults. |
| `/nohistory` | Deletes the chat history database. |
| `/nofilehistory` | Deletes the file transfer history. |
| `/sync` | Re-applies external side effects of the settings (for example start-at-login). |
| `/unsync` | Removes external side effects of the settings. |
| `/udpport=<port>` | Overrides `Connection/UDPPort` for this launch only. |

Run with `/?` for the full switch list.
