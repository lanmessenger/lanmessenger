How to compile LAN Messenger
============================
[![CI](https://github.com/lanmessenger/lanmessenger/actions/workflows/ci.yml/badge.svg)](https://github.com/lanmessenger/lanmessenger/actions/workflows/ci.yml)

You need Qt (https://www.qt.io/) and CMake (https://cmake.org/) to compile.
LAN Messenger is built against Qt 6 and enforces the Qt 6 API baseline
(QT_DISABLE_DEPRECATED_BEFORE=0x060000), so you need Qt 6.4 or later.
The reference builds use Qt 6.8.3 on Windows and macOS and the
distribution Qt 6.4 packages on Linux. CMake 3.16 or later and a C++17
compiler are required.

You also need OpenSSL (http://www.openssl.org/)
Version 1.1 or later, including 3.x, is supported. Only libcrypto is
linked at build time; the libssl runtime DLLs are still copied next to
the binary on Windows.

The application consists of two projects - lmc and lmcapp. lmcapp is 
just an extension of the qtsingleapplication project released by the 
Qt people. I have added a few functions to support multilanguage UI.

The main project is lmc which contains the entire application. Both
projects are built together by the top-level CMake project
(CMakeLists.txt in the repository root).

The code is identical for all platforms, but there are a few differences
in the way application is built and run on each platform. Please read
the platform specific notes to know more.

The GitHub Actions workflow .github/workflows/ci.yml is the reference
build for all three platforms. If you run into setup problems, compare
your environment with the steps in that file.

Note: Qt Creator IDE works well on all platforms. The CI build uses
MSVC on Windows, gcc on Linux and Apple clang on macOS. If you are 
using any other IDE and/or compiler and run into any issue, I can 
only provide generic help.

Important: Its better if your project paths do not contain any white spaces.
Some tools do not work properly with paths containing spaces. Use paths that
do not have spaces to avoid headaches down the lane.
Saving project files to your dektop is a BAD idea.


Compiling/Installing OpenSSL
============================
If precompiled binary distribution of OpenSSL is available for your
platform, you can use it instead of building from source.

The steps for compiling OpenSSL varies depending on your platform.
The package that you get from OpenSSL website gives detailed instructions
on how to compile on all supported platforms. OpenSSL itself may need
additional software packages to compile depending on your system.

Make sure that OpenSSL is built as a shared library.

Windows
-------
Get a prebuilt 64-bit OpenSSL 3.x distribution. The CI build installs
it with vcpkg:

vcpkg install openssl:x64-windows

The Win64 OpenSSL 3.x package from
http://www.slproweb.com/products/Win32OpenSSL.html works too. While
installing it, make sure that the option to copy DLLs to the OpenSSL
binaries directory is selected in the Additional Tasks page.

Whatever source you use, lay the files out in a folder called "openssl"
which should be at the same level as the folder "lmc":
openssl
 |-bin      (libcrypto-*.dll, libssl-*.dll)
 |-include  (openssl headers)
 |-lib      (libcrypto.lib, libssl.lib)

The build step copies the runtime DLLs from openssl\bin to the output
folder.

Linux/X11
---------
Many Linux distributions come with pre-compiled OpenSSL packages. If your
system has one, you may be able to link the project to it. If your system
lacks the package, or if you are unable to link with it, you need to build
OpenSSL from source.

Specify the "shared" switch to ensure that OpenSSL is built as a shared
library.

The CI build simply points the openssl folder at the system OpenSSL:

mkdir openssl
ln -sfn /usr/include openssl/include
ln -sfn /usr/lib/x86_64-linux-gnu openssl/lib

Mac OS X
--------
Mac OS X does not ship a linkable OpenSSL. The CI build installs it with
Homebrew:

brew install openssl@3

and points the openssl folder at the Homebrew prefix:

mkdir openssl
ln -sfn "$(brew --prefix openssl@3)/include" openssl/include
ln -sfn "$(brew --prefix openssl@3)/lib" openssl/lib


Compiling LAN Messenger
======================
Configure and build from the repository root:

cmake -S . -B build -DCMAKE_BUILD_TYPE=Release
cmake --build build --config Release --target package-local --parallel

The "package-local" target stages the complete runtime bundle (the
application binary, the lmcapp library, translations, the resource
bundle, themes, sounds and the Qt image/platform/TLS plugins) into the
LMC_RELEASE_DIR directory, which defaults to <build-dir>/release. The
packaging scripts under lmc/setup expect the layout in lmc/release, so
pass -DLMC_RELEASE_DIR="$PWD/lmc/release" when you plan to build the
install packages (this is what the CI build does).

If OpenSSL is laid out in the "openssl" folder described above, pass
-DOPENSSL_ROOT_DIR=$PWD/openssl to the configure command. Otherwise the
system OpenSSL found by CMake is used.

The build also bundles Qt's own translations (qtbase_*.qm: the strings of the
standard dialogs) into lang/system next to the app's translations. They are
taken from the Qt used for the build, so the Qt translations must be present
on the build machine: the qt6-translations-l10n package on Debian/Ubuntu, the
translations component of the Qt installer on Windows/macOS. Configuration
fails if none are found. Qt itself has no translations for el_GR, ml_IN,
ro_RO, sl_SI and sv_SE, so those locales keep English standard dialogs.

The application's own translation sources live in translations/ and are named
lmc_<locale>.ts (Qt Linguist format); the build picks the directory up
automatically (file(GLOB CONFIGURE_DEPENDS)), so adding a translation is just
dropping a file there - the compiled .qm keeps the locale name (ru_RU.qm) in
lang/ next to system/.

Refer PLATFORM_SPECIFIC.md for additional details about setting up the build
environment on respective platforms.


Compiling LAN Messenger on Windows
==================================
Its possible to compile using other build chains/IDEs, but the CI build
uses CMake with the MSVC tool chain.

OpenSSL should be built/installed first. I recommend using a folder at 
the same level as "lmc" folder as the OpenSSL folder.

Build as described in the previous section. The application binary is
lmc.exe and lmcapp is built as lmcapp.dll with the import library
lmcapp.lib (no renaming needed). The package-local target copies the
OpenSSL runtime DLLs from openssl\bin to the output folder.

To gather the Qt libraries and plugins into the output folder, run
"windeployqt --release <output>\lmc.exe" afterwards, as done in the CI 
build. The Qt 6 TLS backend plugins (the "tls" folder) are needed for
SSL support; copy them from your Qt installation's plugins\tls folder
into the output "tls" folder if windeployqt skipped them.

To build the installer, run "setup.bat" in lmc\setup\win32 folder. NSIS must 
be installed first. Usage:
setup.bat <ExeFolder> [ProductVersion] [InstallerVersion]
Eg: setup.bat release 1.2.39 1.2.3.9

The batch file executes an NSIS script (setup.nsi) which packages the whole
ExeFolder and generates the installer. ProductVersion and InstallerVersion
are passed to makensis as command line defines; when omitted, the defaults
in setup.nsi are used and must be set whenever the application version
changes. The installer generated will have the name lmc-<version>-win64.exe
and it will be saved to lmc\setup folder.


Compiling LAN Messenger on X11/Linux
===================================
OpenSSL should be built first. I recommend using a folder at the same
level as "lmc" folder as the OpenSSL folder. Distribution packages work
too (see the OpenSSL section above).

Build as described earlier and launch the application through the
"lan-messenger.sh" script in the release layout. It sets the load paths
for the bundled libraries and calls the "whitelist" helper internally
(needed for the system tray icon on Ubuntu). Both scripts are staged
with executable permission by the package-local target.

For building the installer, run the bash script "setup" in lmc/setup/x11 folder.
Make sure the scripts named "postinst", "postrm" and "prerm" in the folder
package/DEBIAN have executable permission. The setup script substitutes the
version and architecture placeholders (VERSION, ARCH) in the file "control" in
the same folder from its argument; edit that file to reflect the proper
installed size (in KB) of the application. The scripts "lmc.sh" and "whitelist"
in package/usr/lib/lmc must also have executable permission. All files and
folder inside package folder must be owned by root user.

The setup script generates the deb installation package which will be saved to
lmc/setup folder. The package will have the name lmc_<version>_<arch>.deb
(eg: lmc_1.2.39_x86_64.deb). Set the environment variable PACKAGE_MODE=_min
to build a minimal package that does not bundle the Qt libraries (this is
what the CI build produces); without it the package also bundles the Qt 6
libraries the application links and the plugins they need. For generating
rpm package, alien must be installed first. To install alien, run:
sudo apt-get install alien 
Execute "rpm_setup" script in lmc/setup/x11 to generate an rpm package from the 
deb package (deb package must be created first). This package will have the name 
lmc-<version>.<arch>.rpm and will be saved to lmc/setup folder.


Compiling LAN Messenger on Mac OS X
==================================
Build as described earlier; the result is LAN-Messenger.app in the release
layout. The lmcapp helper (liblmcapp.2.dylib) is placed inside the bundle
with load path @executable_path.

Note: Mac OS X does not ship a linkable OpenSSL. Install it as described
in the OpenSSL section above and pass -DOPENSSL_ROOT_DIR when configuring.

For building the installer, first run the bash script "stage-mac-app" in
lmc/setup/mac folder. This copies the app bundle and runs macdeployqt on it
to collect the Qt frameworks. The script finds macdeployqt through the
QTDIR environment variable, so make sure it points at your Qt installation
(the parent folder of the bin and lib folders). Then build the disk image
with dmgbuild (pip install dmgbuild), which packs the bundle, the
Applications symlink, the Finder window layout (icon positions, background
picture, icon size 72) and the user license agreement, as declared in
dmg-settings.py:
dmgbuild -s dmg-settings.py "LAN-Messenger" "../lmc_<version>_<arch>.dmg"
(<arch> is x86_64 or arm64; remove a previous image first with
rm -f "../lmc_<version>_<arch>.dmg" if you rebuild). The dmg file will have the name
lmc_<version>_x86_64.dmg (Intel Macs, CI job build-macos) or
lmc_<version>_arm64.dmg (Apple Silicon, CI job build-macos)
and will be saved to lmc/setup folder.

Note: On Mac OS X, option to start LAN Messenger on startup will not work.
This is a platform dependent function and I have not implemented it.


Audio playback support
=====================
LAN Messenger can play sounds to accompany certain events. This behaviour
is customizable in the Preferences dialog. Sounds are played through the
Qt Multimedia module (QSoundEffect), so that module must be available at
build and run time. On Linux install the Qt Multimedia development and
runtime packages (qt6-multimedia-dev, libqt6multimedia6). If no audio
output device is available, the sound options will be grayed out in the
Preferences dialog.


System tray support
==================
On desktops that do not have a system tray, the system tray options will
be grayed out in the Preferences dialog.
