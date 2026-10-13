---
title: Build prerequisites
description: OpenSSL setup, CMake configure and build, translations, audio playback and system tray support.
---

Covers the platform-independent part of the build. For the rest, see
[Building on Windows](build-windows.md), [Building on Linux](build-linux.md)
and [Building on macOS](build-macos.md).

## Compiling/installing OpenSSL

If a precompiled binary distribution of OpenSSL is available for your
platform, you can use it instead of building from source.

The steps for compiling OpenSSL vary depending on your platform.
The package that you get from the OpenSSL website gives detailed
instructions on how to compile on all supported platforms. OpenSSL itself
may need additional software packages to compile depending on your system.

Make sure that OpenSSL is built as a shared library.

### Windows

Get a prebuilt 64-bit OpenSSL 3.x distribution. The CI build installs
it with vcpkg:

```bash
vcpkg install openssl:x64-windows
```

The Win64 OpenSSL 3.x package from
http://www.slproweb.com/products/Win32OpenSSL.html works too. While
installing it, make sure that the option to copy DLLs to the OpenSSL
binaries directory is selected in the Additional Tasks page.

Whatever source you use, lay the files out in a folder called `openssl`
which should be at the same level as the folder `lmc`:

```text
openssl
 |-bin      (libcrypto-*.dll, libssl-*.dll)
 |-include  (openssl headers)
 |-lib      (libcrypto.lib, libssl.lib)
```

The build step copies the runtime DLLs from `openssl\bin` to the output
folder.

### Linux/X11

Many Linux distributions come with pre-compiled OpenSSL packages. If your
system has one, you may be able to link the project to it. If your system
lacks the package, or if you are unable to link with it, you need to build
OpenSSL from source.

Specify the `shared` switch to ensure that OpenSSL is built as a shared
library.

The CI build simply points the `openssl` folder at the system OpenSSL:

```bash
mkdir openssl
ln -sfn /usr/include openssl/include
ln -sfn /usr/lib/x86_64-linux-gnu openssl/lib
```

### macOS

macOS does not ship a linkable OpenSSL. The CI build installs it with
Homebrew:

```bash
brew install openssl@3
```

and points the `openssl` folder at the Homebrew prefix:

```bash
mkdir openssl
ln -sfn "$(brew --prefix openssl@3)/include" openssl/include
ln -sfn "$(brew --prefix openssl@3)/lib" openssl/lib
```

## Configuring and building

Configure and build from the repository root:

```bash
cmake -S . -B build -DCMAKE_BUILD_TYPE=Release
cmake --build build --config Release --target package-local --parallel
```

The `package-local` target stages the complete runtime bundle (the
application binary, translations, the resource bundle, themes, sounds
and the Qt image/platform/TLS plugins) into the `LMC_RELEASE_DIR`
directory, which defaults to `<build-dir>/release`. The packaging scripts
under `lmc/setup` expect the layout in `lmc/release`, so pass
`-DLMC_RELEASE_DIR="$PWD/lmc/release"` when you plan to build the
install packages (this is what the CI build does).

If OpenSSL is laid out in the `openssl` folder described above, pass
`-DOPENSSL_ROOT_DIR=$PWD/openssl` to the configure command. Otherwise the
system OpenSSL found by CMake is used.

## Translations

The build also bundles Qt's own translations (`qtbase_*.qm`: the strings
of the standard dialogs) into `lang/system` next to the app's
translations. They are taken from the Qt used for the build, so the Qt
translations must be present on the build machine: the
`qt6-translations-l10n` package on Debian/Ubuntu, the translations
component of the Qt installer on Windows/macOS. Configuration fails if
none are found (override the lookup with
`-DLMC_QT_TRANSLATIONS_DIR=<abs path>` if needed). With the reference Qt
versions (6.4/6.8.3) there are no Qt translations for `el_GR`, `ml_IN`,
`ro_RO`, `sl_SI` and `sv_SE`, so those locales keep English standard
dialogs (for `sl_SI` this drops the previously vendored file - accepted).

The application's own translation sources live in `translations/` and are
named `lmc_<locale>.ts` (Qt Linguist format); the build picks the
directory up automatically (`file(GLOB CONFIGURE_DEPENDS)`), so adding a
translation is just dropping a file there - the compiled `.qm` keeps the
locale name (`ru_RU.qm`) in `lang/` next to `system/`. With the Visual
Studio generator, re-run configure after adding or removing a file.

## Audio playback support

LAN Messenger can play sounds to accompany certain events. This behaviour
is customizable in the Preferences dialog. Sounds are played through the
Qt Multimedia module (`QSoundEffect`), so that module must be available at
build and run time. On Linux install the Qt Multimedia development and
runtime packages (`qt6-multimedia-dev`, `libqt6multimedia6`). If no audio
output device is available, the sound options will be grayed out in the
Preferences dialog.

## System tray support

On desktops that do not have a system tray, the system tray options will
be grayed out in the Preferences dialog.
