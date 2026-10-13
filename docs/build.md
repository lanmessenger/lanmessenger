---
title: Building LAN Messenger
description: Requirements, project layout, and pointers to the generic and per-platform build guides.
---

This section collects everything needed to build LAN Messenger from source:
the generic procedure plus the per-platform specifics.

## Requirements

- Qt 6.4 or later. LAN Messenger is built against Qt 6 and enforces the
  Qt 6 API baseline (`QT_DISABLE_DEPRECATED_BEFORE=0x060000`).
  The reference builds use Qt 6.8.3 on Windows and macOS and the
  distribution Qt 6.4 packages on Linux.
- CMake 3.16 or later and a C++17 compiler. The CI build uses MSVC on
  Windows, gcc on Linux and Apple clang on macOS.
- OpenSSL 1.1 or later, including 3.x. Only libcrypto is linked at build
  time; on Windows the libssl runtime DLLs are still copied next to
  the binary.

Qt Creator IDE works well on all platforms. If you are using any other
IDE and/or compiler and run into any issue, only generic help can
be provided.

Keep project paths free of white spaces. Some tools do not work properly
with paths containing spaces. Saving project files to your desktop is
a BAD idea.

## Project layout

The application consists of two projects: `lmc` and `lmcapp`. `lmcapp`
is an extension of the `qtsingleapplication` project released by the Qt
people, with a few added functions to support multilanguage UI. The main
project is `lmc`, which contains the entire application. The `lmcapp`
sources are linked into the application binary (no separate DLL or
helper library ships). Both projects are built together by the top-level
CMake project (`CMakeLists.txt` in the repository root).

The code is identical for all platforms; only the way the application
is built, deployed and packaged differs per platform (see the guides
below).

## Reference build

The GitHub Actions workflow `.github/workflows/ci.yml` is the reference
build for all three platforms. If you run into setup problems, compare
your environment with the steps in that file.

## Guides

- [Build prerequisites](build-prerequisites.md): OpenSSL setup, CMake
  configure and build, translations, audio playback and system tray
  support.
- [Building on Windows](build-windows.md): MSVC build, Qt deployment
  with `windeployqt`, and the NSIS installer.
- [Building on Linux](build-linux.md): CMake build, launch scripts,
  debugging, and deb/rpm packaging.
- [Building on macOS](build-macos.md): app bundle, `macdeployqt`
  staging, and DMG packaging.
