Windows
-------
The build is driven by CMake; the binaries are written to the build
directory. The "package-local" target stages the complete runtime bundle
into the LMC_RELEASE_DIR directory (defaults to <build-dir>/release).
The CI build stages it into lmc\release where the packaging scripts
expect it:

cmake -S . -B build -DCMAKE_BUILD_TYPE=Release -DLMC_RELEASE_DIR="$PWD/lmc/release"
cmake --build build --config Release --target package-local --parallel

lmcapp is built as lmcapp.dll with the import library lmcapp.lib, so no
renaming step is needed. The package-local target copies the OpenSSL
runtime DLLs (libcrypto*.dll, libssl*.dll) from openssl\bin into the
output folder.

Run "windeployqt --release <output>\lmc.exe" afterwards to gather the Qt
libraries and plugins, as done in the CI build. The Qt 6 TLS backend
plugins (the "tls" folder) are needed for SSL support; copy them from
your Qt installation's plugins\tls folder into the output "tls" folder
if windeployqt skipped them (the CI build does this as a separate step).

For building the installer, run setup.bat in lmc\setup\win32 folder. NSIS must 
be installed first. Usage:
setup.bat <ExeFolder> [ProductVersion] [InstallerVersion]
Eg: setup.bat release 1.2.39 1.2.3.9

The batch file executes an NSIS script (setup.nsi) which packages the whole
ExeFolder and generates the installer. ProductVersion and InstallerVersion
are passed to makensis as command line defines; when omitted, the defaults
in setup.nsi are used and must be set whenever the application version
changes. The installer generated will have the name lmc-<version>-win64.exe
and it will be saved to lmc\setup folder.


Linux/X11
---------
The build is driven by CMake. The "package-local" target stages the
runtime layout (binary, liblmcapp.so.2, lmc.rcc, lang, themes, sounds
and the Qt imageformats/platforms/tls plugins) into the LMC_RELEASE_DIR
directory (defaults to <build-dir>/release):

cmake -S . -B build -DCMAKE_BUILD_TYPE=Release -DLMC_RELEASE_DIR="$PWD/lmc/release"
cmake --build build --config Release --target package-local --parallel

The packaging scripts read the layout from lmc/release, so pass that
value for LMC_RELEASE_DIR when building packages (this is what the CI
build does).

The two script files named lan-messenger.sh and whitelist are needed to launch 
the application. The first one sets the load paths for all libraries needed by 
the application. The second is needed for adding the application to Ubuntu's 
whitelist so that the system tray icon can be shown. The second script will be 
called internally by the first script. Both are staged with executable
permission by the package-local target.

To debug a running application, run this command before attaching to the process:
echo 0 | sudo tee /proc/sys/kernel/yama/ptrace_scope

This will be valid only for the current session. It has to be run again 
after a log out or restart.

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


Mac OS X
--------
The build is driven by CMake. The "package-local" target stages
LAN-Messenger.app into the LMC_RELEASE_DIR directory (defaults to
<build-dir>/release; use lmc/release for the packaging scripts):

cmake -S . -B build -DCMAKE_BUILD_TYPE=Release -DLMC_RELEASE_DIR="$PWD/lmc/release"
cmake --build build --config Release --target package-local --parallel

The lmcapp helper (liblmcapp.2.dylib) is placed inside
LAN-Messenger.app/Contents/MacOS with load path @executable_path, so no
install_name_tool post-processing is needed. The Qt image format plugins
go to Contents/Plugins (see setLibraryPaths in main.cpp).

For building the installer, first run the bash script "createdisk" in 
lmc/setup/mac folder. This copies the app bundle, runs macdeployqt on it to
collect the Qt frameworks and creates a disk image with all the required
files needed for the application. The script locates macdeployqt through
the QTDIR environment variable, so make sure it points at your Qt
installation (the parent folder of the bin and lib folders). Now open up
the disk image, set the
background image, icon size (96x96), icon position, icon arrangment (Snap to
Grid) and window size. Now run the script "addlicense" to add the user license
and compress the disk image. The dmg file will have the name 
lmc_<version>_intel.dmg and will be saved to lmc/setup folder.
