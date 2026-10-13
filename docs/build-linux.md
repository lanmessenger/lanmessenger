---
title: Building on Linux
description: CMake build, launch scripts, debugging, and deb/rpm packaging.
---

Prerequisites first: [Build prerequisites](build-prerequisites.md) (OpenSSL layout, CMake,
translations). OpenSSL should be built first; using a folder at the same
level as the `lmc` folder as the OpenSSL folder is recommended.
Distribution packages work too (see the [OpenSSL setup](build-prerequisites.md#linuxx11)).

## Build

The build is driven by CMake. The `package-local` target stages the
runtime layout (binary, `lmc.rcc`, `lang`, `themes`, `sounds` and the Qt
imageformats/platforms/tls plugins) into the `LMC_RELEASE_DIR` directory
(defaults to `<build-dir>/release`):

```bash
cmake -S . -B build -DCMAKE_BUILD_TYPE=Release -DLMC_RELEASE_DIR="$PWD/lmc/release"
cmake --build build --config Release --target package-local --parallel
```

The packaging scripts read the layout from `lmc/release`, so pass that
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

## Installer (deb)

For building the installer, run the bash script `setup` in the
`lmc/setup/x11` folder. Make sure the scripts named `postinst`, `postrm`
and `prerm` in the folder `package/DEBIAN` have executable permission.
The setup script substitutes the version and architecture placeholders
(`VERSION`, `ARCH`) in the file `control` in the same folder from its
argument; edit that file to reflect the proper installed size (in KB) of
the application. The scripts `lmc.sh` and `whitelist` in
`package/usr/lib/lmc` must also have executable permission. All files and
folders inside the `package` folder must be owned by the root user.

The setup script generates the deb installation package which will be
saved to the `lmc/setup` folder. The package will have the name
`lmc_<version>_<arch>.deb` (e.g. `lmc_1.2.39_x86_64.deb`). Set the
environment variable `PACKAGE_MODE=_min` to build a minimal package that
does not bundle the Qt libraries (this is what the CI build produces);
without it the package also bundles the Qt 6 libraries the application
links and the plugins they need.

## Installer (rpm)

For generating an rpm package, `alien` must be installed first. To
install `alien`, run:

```bash
sudo apt-get install alien
```

Execute the `rpm_setup` script in `lmc/setup/x11` to generate an rpm
package from the deb package (the deb package must be created first).
This package will have the name `lmc-<version>.<arch>.rpm` and will be
saved to the `lmc/setup` folder.
