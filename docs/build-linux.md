---
title: Building on Linux
description: CMake build, launch scripts, debugging, and deb/rpm/Arch packaging.
---

Prerequisites first: [Build prerequisites](build-prerequisites.md) (OpenSSL layout, CMake,
translations). OpenSSL should be built first; using a folder at the same
level as the `lmc` folder as the OpenSSL folder is recommended.
Distribution packages work too (see the [OpenSSL setup](build-prerequisites.md#linuxx11)).

## Build

The build is driven by CMake. The `package-local` target stages the
runtime layout (binary, `lmc.rcc`, `lang`, `sounds` and the Qt
imageformats/platforms/tls plugins) into the `LMC_RELEASE_DIR` directory
(defaults to `<build-dir>/release`):

```bash
cmake -S . -B build -DCMAKE_BUILD_TYPE=Release -DLMC_RELEASE_DIR="$PWD/release"
cmake --build build --config Release --target package-local --parallel
```

The packaging scripts read the layout from `release`, so pass that
value for `LMC_RELEASE_DIR` when building packages (this is what the CI
build does).

## Launch scripts

Launch the application through the `lan-messenger.sh` script in the
release layout. It sets the load paths for the bundled libraries and
calls the `whitelist` helper internally (needed for the system tray icon
on Ubuntu). Both scripts are staged with executable permission by the
`package-local` target.

## Debugging

To debug a running application, run this command before attaching to the
process:

```bash
echo 0 | sudo tee /proc/sys/kernel/yama/ptrace_scope
```

This will be valid only for the current session. It has to be run again
after a log out or restart.

## Installer (deb, rpm, Arch)

All three Linux system packages are built from a single `nfpm.yaml` in
the `setup/linux` folder, on top of the payload tree staged by `stage.sh`
from `release` (the `package-local` target must have run first, see
above; CI does this in `build.yml`):

```bash
cd setup/linux
sh ./stage.sh
NFPM=/path/to/nfpm sh ./package.sh 1.2.39
```

`nfpm` is a single static binary, no root needed:
download `nfpm_*_Linux_x86_64.tar.gz` from the
[nfpm releases](https://github.com/goreleaser/nfpm/releases) (pin the
version, verify the sha256 from `checksums.txt`). `package.sh` builds
into the `setup` folder:

- `lmc_<version>_amd64.deb`
- `lmc-<version>-1.x86_64.rpm`
- `lmc-<version>-1-x86_64.pkg.tar.zst`

Only system-Qt packages are built; the `Depends`/`Requires` in
`nfpm.yaml` pull the system Qt 6 libraries. Bundled Qt is not
supported.

Nightly versions look like `1.2.40~nightly.123` in deb/rpm and
`1.2.40_nightly.123` in the Arch package (`pkgver` forbids `~` and `-`,
so `package.sh` maps them to `_`). Nightly Arch packages sort newer than
the matching release; switching from a nightly to that release looks
like a downgrade to pacman.

Dependencies are declared once per ecosystem in `nfpm.yaml`
(`overrides.rpm` carries the Fedora names). Icon caches need no
maintainer scripts: they are refreshed by the distro triggers/hooks.
Autostart entries are per-user configuration created by the app itself
and are left in place on uninstall; the app removes its own entry when
autostart is disabled.

Chat messages render emoji with the operating system's color emoji font.
The packages recommend it, so it is normally installed automatically
(apt installs Recommends by default, dnf installs weak Recommends by
default):

- Debian/Ubuntu: `fonts-noto-color-emoji`
- Fedora/RHEL: `google-noto-color-emoji-fonts`
- Arch: `noto-fonts-emoji`

The app also runs without it (monochrome/tofu fallback). If emoji show
up as empty boxes (tofu), install the font manually:

```bash
sudo apt-get install fonts-noto-color-emoji   # Debian/Ubuntu
sudo dnf install google-noto-color-emoji-fonts  # Fedora
sudo pacman -S noto-fonts-emoji                 # Arch
```
