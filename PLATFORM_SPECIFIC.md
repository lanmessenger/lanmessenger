Windows
-------
On Windows the project files do not set DESTDIR, so the binaries are written
to the build directory. The CI build compiles both projects in-source (in
lmcapp\src and lmc\src) and copies lmc.exe to the runtime bundle folder
lmc\release afterwards.

Note: the lmc project locates the lmcapp library via
$$replace(OUT_PWD, lmc, lmcapp), so keep the two build directories as
siblings with matching names (e.g. lmc and lmcapp) if you use shadow builds.

The custom build step needs the following parameters:
Command: scripts\buildwin32.bat
Working directory: %{sourceDir}
Command arguments: %{buildDir}\..\debug (for debug)
Command arguments: %{buildDir}\..\release release (for release)

The script copies the lmcapp DLL and the OpenSSL runtime DLLs
(libcrypto*.dll, libssl*.dll) into the output folder. Run
"windeployqt --release <output>\lmc.exe" afterwards to gather the Qt
libraries and plugins, as done in the CI build.

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
Build directory for debug is lmc/debug.
Build directory for release is lmc/release.
lmc
 |-debug
 |-release
 |-src

The custom build script needs executable permission. Run the following command:
chmod 755 ./scripts/buildx11

The custom build step needs the following parameters:
Command: ./scripts/buildx11
Working directory: %{sourceDir}
Command arguments: %{buildDir}

The two script files named lan-messenger.sh and whitelist are needed to launch 
the application. The first one sets the load paths for all libraries needed by 
the application. The second is needed for adding the application to Ubuntu's 
whitelist so that the system tray icon can be shown. The second script will be 
called internally by the first script. Make sure both scripts have permission 
to run as executables.
chmod 755 ./lan-messenger.sh
chmod 755 ./whitelist

Note: The custom build script will take care of this automatically.

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
what the CI build produces). For generating rpm package, alien must be
installed first. To install alien, run:
sudo apt-get install alien 
Execute "rpm_setup" script in lmc/setup/x11 to generate an rpm package from the 
deb package (deb package must be created first). This package will have the name 
lmc-<version>.<arch>.rpm and will be saved to lmc/setup folder.


Mac OS X
--------
Build directory for debug is lmc/debug.
Build directory for release is lmc/release.
lmc
 |-debug
 |-release
 |-src

The custom build script needs executable permission. Run the following command:
chmod 755 ./scripts/buildmacos

The custom build step needs the following parameters:
Command: ./scripts/buildmacos
Working directory: %{sourceDir}
Command arguments: %{buildDir}/LAN-Messenger.app/Contents/MacOS

For building the installer, first run the bash script "createdisk" in 
lmc/setup/mac folder. This copies the app bundle, runs macdeployqt on it to
collect the Qt frameworks and creates a disk image with all the required
files needed for the application. Now open up the disk image, set the
background image, icon size (96x96), icon position, icon arrangment (Snap to
Grid) and window size. Now run the script "addlicense" to add the user license
and compress the disk image. The dmg file will have the name 
lmc_<version>_intel.dmg and will be saved to lmc/setup folder.
