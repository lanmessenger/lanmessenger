---
title: Installing on Windows
description: Installer walkthrough, SmartScreen and admin prompt, firewall, silent install, update and uninstall.
---

Download the 64-bit installer from the [releases page](https://github.com/lanmessenger/lanmessenger/releases):
`lmc-<version>-win64.exe` (for example `lmc-1.3.17-win64.exe`).
Windows 10 and 11 (64-bit) are supported.

## Install

1. Run the downloaded `.exe`. The installer needs administrator rights,
   so confirm the User Account Control prompt ("Do you want to allow this
   app to make changes to your device?").
2. Follow the wizard: Welcome → License → install folder (default
   `C:\Program Files\LAN Messenger`) → Install.
3. On the Finish page leave `Start LAN Messenger` checked to launch
   right away.

The installer creates Start menu shortcuts (for all users), registers
the uninstaller, applies the start-at-login setting, and creates the
`Received Files` folder in your Documents. If a previous version is
already installed, the installer asks to remove it first — settings
and history are kept.

## SmartScreen warning

The installer is not code-signed, so Windows SmartScreen may block the
first launch with a "Windows protected your PC" warning. This is about
the missing publisher signature, not a broken download. Click `More
info`, then `Run anyway` — the approval is one-time per install.

> The installer has no paid code signing certificate, so SmartScreen shows an unknown publisher. This is expected for a free open source app. The full source is open under GPL-3.0, and every release is built automatically on GitHub from that code using a fresh runner for each release and published as release assets. You can verify what you install instead of trusting a publisher name.

## Firewall

The installer adds two inbound allow rules named `LAN Messenger`
(TCP and UDP, bound to the installed `lmc.exe`, `private` profile
only) — no manual port setup is needed on a home or office network:

| Direction | Port | Used for |
|---|---|---|
| Inbound UDP | `50000` | Peer discovery (multicast group `239.255.100.100` plus directed broadcast) |
| Inbound TCP | `50000` by default | Messages and file transfers (or the TCP port set in Preferences → Network → TCP Port) |

Two things still hide peers:

- **Public networks.** The installer rules cover the `private` profile
  only. If your network is set to Public (Settings → Network &
  internet → Wi-Fi/Ethernet → network → `Private`), switch it to
  Private so the rules apply.
- **VPN, Wi-Fi client isolation, different subnets.** Machines must
  reach each other directly on the local network, not only the
  internet. For machines on different subnets, add the remote
  broadcast addresses under Preferences → Network → Broadcast.

## Silent install

For deploying to many machines, run the installer with the NSIS
silent flags from an elevated command prompt (`/D=` must be the last
parameter, without quotes):

```cmd
lmc-1.3.17-win64.exe /S /D=C:\Program Files\LAN Messenger
```

To install without admin rights prompts into a user-writable folder:

```cmd
lmc-1.3.17-win64.exe /S /D=C:\LAN-Messenger
```

## Update and uninstall

Install the new version over the old one with the same installer;
settings and history are kept. To remove, use Settings → Apps →
LAN Messenger → Uninstall, the Start menu `Uninstall LAN Messenger`
shortcut, or `uninst.exe` in the install folder. The uninstaller
asks whether to also delete history and preferences; uninstall keeps
them unless you check those boxes. Silent uninstall:

```cmd
"C:\Program Files\LAN Messenger\uninst.exe" /S
```

Settings live in `%APPDATA%\LAN Messenger\LAN Messenger.ini`
(chat history, avatars, and groups sit next to it — see the
[INI file reference](ini-file.md)).

## If it still doesn't start

Open an [issue](https://github.com/lanmessenger/lanmessenger/issues) with your Windows version and build (10 or 11),
the app version, the exact installer file name, and the exact message
text or screenshot Windows shows.
