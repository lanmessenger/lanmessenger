---
title: Building on macOS
description: App bundle layout, macdeployqt staging, and DMG packaging.
---

Prerequisites first: [Build prerequisites](build-prerequisites.md) (CMake, translations).
macOS does not ship a linkable OpenSSL: install it as described in the
[OpenSSL setup](build-prerequisites.md#macos) and pass `-DOPENSSL_ROOT_DIR` when
configuring.

## Build

The build is driven by CMake. The `package-local` target stages
`LAN-Messenger.app` into the `LMC_RELEASE_DIR` directory (defaults to
`<build-dir>/release`; use `lmc/release` for the packaging scripts):

```bash
cmake -S . -B build -DCMAKE_BUILD_TYPE=Release -DLMC_RELEASE_DIR="$PWD/lmc/release"
cmake --build build --config Release --target package-local --parallel
```

The `lmcapp` sources are linked into the app binary, so no helper library
ships inside `LAN-Messenger.app/Contents/MacOS` and no `install_name_tool`
post-processing is needed. The Qt image format plugins go to
`Contents/Plugins` (see `setLibraryPaths` in `main.cpp`).

## Installer (DMG)

For building the installer, first run the bash script `stage-mac-app` in
the `lmc/setup/mac` folder. This copies the app bundle and runs
`macdeployqt` on it to collect the Qt frameworks. The script finds
`macdeployqt` through the `QTDIR` environment variable, so make sure it
points at your Qt installation (the parent folder of the `bin` and `lib`
folders). Then build the disk image with `dmgbuild`
(`pip install dmgbuild`), which packs the bundle, the `Applications`
symlink, the Finder window layout (icon positions, background picture,
icon size 72) and the user license agreement, as declared in
`dmg-settings.py`:

```bash
dmgbuild -s dmg-settings.py "LAN-Messenger" "../lmc_<version>_<arch>.dmg"
```

`<arch>` is `x86_64` or `arm64`; remove a previous image first with
`rm -f "../lmc_<version>_<arch>.dmg"` if you rebuild. The dmg file will
have the name `lmc_<version>_x86_64.dmg` (Intel Macs, CI job
`build-macos`) or `lmc_<version>_arm64.dmg` (Apple Silicon, CI job
`build-macos`) and will be saved to the `lmc/setup` folder.

Note: on macOS, the option to start LAN Messenger on startup will not
work. This is a platform dependent function and it has not been
implemented.
