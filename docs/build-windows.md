---
title: Building on Windows
description: MSVC build, Qt deployment with windeployqt, and the NSIS installer.
---

Prerequisites first: [Build prerequisites](build-prerequisites.md) (OpenSSL layout, CMake,
translations). OpenSSL should be built/installed first; using a folder at
the same level as the `lmc` folder as the OpenSSL folder is recommended
(see the [OpenSSL setup](build-prerequisites.md#compilinginstalling-openssl)).

It is possible to compile using other build chains/IDEs, but the CI build
uses CMake with the MSVC tool chain.

## Build

The build is driven by CMake; the binaries are written to the build
directory. The `package-local` target stages the complete runtime bundle
into the `LMC_RELEASE_DIR` directory (defaults to `<build-dir>/release`).
The CI build stages it into `lmc\release` where the packaging scripts
expect it. Run from Git Bash (comes with Git for Windows):

```bash
cmake -S . -B build -DCMAKE_BUILD_TYPE=Release -DLMC_RELEASE_DIR="$PWD/lmc/release"
cmake --build build --config Release --target package-local --parallel
```

The application binary is `lmc.exe`, with the `lmcapp` sources linked
into it (no separate DLL), so no renaming step is needed. The
`package-local` target copies the OpenSSL runtime DLLs
(`libcrypto*.dll`, `libssl*.dll`) from `openssl\bin` into the output
folder.

## Qt deployment

To gather the Qt libraries and plugins into the output folder, run
`windeployqt --release <output>\lmc.exe` afterwards, as done in the CI
build. The Qt 6 TLS backend plugins (the `tls` folder) are needed for SSL
support; copy them from your Qt installation's `plugins\tls` folder into
the output `tls` folder if `windeployqt` skipped them (the CI build does
this as a separate step).

## Installer

To build the installer, run `setup.bat` in the `lmc\setup\win32` folder.
NSIS must be installed first. Usage:

```cmd
setup.bat <ExeFolder> [ProductVersion] [InstallerVersion]
```

Example:

```cmd
setup.bat release 1.2.39 1.2.3.9
```

The batch file executes an NSIS script (`setup.nsi`) which packages the
whole `ExeFolder` and generates the installer. `ProductVersion` and
`InstallerVersion` are passed to `makensis` as command line defines; when
omitted, the defaults in `setup.nsi` are used and must be set whenever
the application version changes. The installer generated will have the
name `lmc-<version>-win64.exe` and it will be saved to the `lmc\setup`
folder.
