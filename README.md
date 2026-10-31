# LAN Messenger

[![License: GPL-3.0](https://img.shields.io/badge/License-GPL--3.0-blue.svg)](LICENSE)

**A free, open-source instant messaging application for local networks.**

LAN Messenger lets you exchange messages and files with other users on your
local network. No dedicated messaging server or internet connection required.

## Features

- **Instant Messaging**: chat with other users on your local network.
- **Encrypted Messaging**: protect message privacy with AES-256 encryption and RSA-2048.
- **Broadcast Messages**: send messages to everyone on the network or select specific recipients.
- **File Transfer**: send files directly to other users on your local network.
- **Contact Groups**: organize your contacts into groups for easier navigation.
- **Message History**: access your previous conversations.
- **Serverless Operation**: communicate over your local network without setting up a dedicated messaging server.
- **Offline Network Operation**: keep communicating over your local network when internet access is unavailable.
- **Multiple Languages**: use the interface in any of the 20 supported languages.
- **Cross-platform Support**: available for Windows, macOS, and Linux.

## Screenshots

| Windows | macOS | Linux |
|---------|-------|-------|
| [![Windows](https://lanmessenger.github.io/images/screenshots/win11-desktop-min.jpg)](https://lanmessenger.github.io/images/screenshots/win11-desktop.png) | [![macOS](https://lanmessenger.github.io/images/screenshots/mac-desktop-min.jpg)](https://lanmessenger.github.io/images/screenshots/mac-desktop.png) | [![Linux](https://lanmessenger.github.io/images/screenshots/linux-desktop-min.jpg)](https://lanmessenger.github.io/images/screenshots/linux-desktop.png) |

## Quick Start

1. Download the package for your operating system (`.exe` for Windows, `.dmg` for macOS, or `.deb`/`.rpm`/`.pkg.tar.zst` for Linux) from the [releases page](https://github.com/lanmessenger/lanmessenger/releases).
2. Install and run LAN Messenger on two or more machines connected to the same local network. Other users should appear automatically.
3. If other users don't appear, check that the machines can communicate over the network and that the local firewall allows LAN Messenger traffic. VPN configurations or network separation may prevent peer discovery.

On macOS the DMG is ad-hoc signed and not notarized. If Gatekeeper blocks the first launch, clear the quarantine attribute once:

```bash
sudo xattr -dr com.apple.quarantine "/Applications/LAN-Messenger.app"
```

See [Installing on Windows](docs/install-windows.md), [Installing on macOS](docs/install-macos.md), and [Installing on Linux](docs/install-linux.md) for details.

## Downloads and Documentation

- **Downloads:** [Browse the latest releases](https://github.com/lanmessenger/lanmessenger/releases).
- **Documentation:** [Visit the LAN Messenger website](https://lanmessenger.github.io).

## Building from Source

See [docs/build.md](docs/build.md) for prerequisites and platform-specific build instructions.

## Support and Bug Reports

Found a bug or have a question? Visit [GitHub Issues](https://github.com/lanmessenger/lanmessenger/issues).

When reporting a bug, include your operating system, LAN Messenger version, and steps to reproduce the issue.

## Contributing

Pull requests are welcome. LAN Messenger is written in C++ and uses Qt. See [docs/build.md](docs/build.md) for build instructions.

## License

LAN Messenger is distributed under the [GNU General Public License v3.0](LICENSE).
