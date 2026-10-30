---
title: Installing on Linux
description: Install commands, firewall ports, and what to do when no peers appear.
---

Download the package matching your distro from the [releases page](https://github.com/lanmessenger/lanmessenger/releases).
All packages are 64-bit (`x86_64`) only (file names below are for release 1.3.17).

### Debian, Ubuntu, Mint, Pop!_OS

```bash
sudo apt install ./lmc_1.3.17_amd64.deb
```

### Fedora, RHEL, CentOS

```bash
sudo dnf install ./lmc-1.3.17-1.x86_64.rpm
```

### openSUSE

```bash
sudo zypper install ./lmc-1.3.17-1.x86_64.rpm
```

### Arch, Manjaro

```bash
sudo pacman -U ./lmc-1.3.17-1-x86_64.pkg.tar.zst
```

Dependencies (system Qt 6, OpenSSL) resolve automatically through your
package manager — no manual Qt install, no building from source (see
[Building on Linux](build-linux.md) only if you compile it yourself).
Emoji in chat render with the OS color emoji font; the packages recommend
it so it is normally installed automatically (`fonts-noto-color-emoji` on
Debian/Ubuntu, `google-noto-color-emoji-fonts` on Fedora,
`noto-fonts-emoji` on Arch). The app runs without it, but emoji show up
as empty boxes until you install the font.

After install, launch LAN Messenger from the applications menu (or run
`lmc` in a terminal). The menu entry, icons, and the `/usr/bin/lmc`
launcher are part of the package.

## Firewall

Unlike the Windows firewall, `ufw` and `firewalld` grant access by port,
not by application — so LAN Messenger needs its ports opened explicitly:

| Direction | Port | Used for |
|---|---|---|
| Inbound UDP | `50000` | Peer discovery (multicast group `239.255.100.100` plus directed broadcast) |
| Inbound TCP | `50000` by default | Messages and file transfers (or the TCP port set in Preferences → Network → TCP Port) |

To receive messages and files, the machine's inbound TCP port must be
reachable; sending needs the peer's inbound TCP port open. So open the
ports on each machine:

```bash
# Debian, Ubuntu (ufw)
sudo ufw allow 50000/udp
sudo ufw allow 50000/tcp
```

```bash
# Fedora, RHEL, openSUSE (firewalld)
sudo firewall-cmd --permanent --add-port=50000/udp
sudo firewall-cmd --permanent --add-port=50000/tcp
sudo firewall-cmd --reload
```

VPN connections and Wi-Fi client isolation also block discovery: machines
must reach each other directly on the local network, not only the
internet. Check with:

```bash
ss -ulnp | grep 50000          # UDP discovery listener
ss -tlnp | grep lan-messenger  # TCP listener
```

For machines on different subnets, add the remote broadcast addresses
under Preferences → Network → Broadcast.

## Update and uninstall

Install the new package over the old one with the same command as above;
settings and history are kept. To remove:

```bash
sudo apt remove lmc          # Debian/Ubuntu
sudo dnf remove lmc          # Fedora/RHEL/CentOS
sudo zypper remove lmc       # openSUSE
sudo pacman -R lmc           # Arch
```

Uninstall keeps your personal data. To wipe it too, delete
`~/.config/lmc/` (settings) and `~/.local/share/lmc/` (history, avatars).

## If it still doesn't start

Run `lmc` in a terminal and note the output. Open an
[issue](https://github.com/lanmessenger/lanmessenger/issues) with your distro
and version, your desktop (X11 or Wayland), the exact package file name,
and the terminal output.
