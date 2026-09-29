How to compile LAN Messenger
============================
[![CI](https://github.com/lanmessenger/lanmessenger/actions/workflows/ci.yml/badge.svg)](https://github.com/lanmessenger/lanmessenger/actions/workflows/ci.yml)

You need Qt (https://www.qt.io/) to compile.
LAN Messenger is built against Qt 5.15 and enforces the Qt 5.15 API
baseline (QT_DISABLE_DEPRECATED_BEFORE=0x050F00), so you need Qt 5.15
or later. The reference builds use Qt 5.15.2 on Windows and macOS and
the distribution Qt 5.15 packages on Linux.

You also need OpenSSL (http://www.openssl.org/)
Version 1.1 or later, including 3.x, is supported. Only libcrypto is
linked at build time; the libssl runtime DLLs are still copied next to
the binary on Windows.

The application consists of two projects - lmc and lmcapp. lmcapp is 
just an extension of the qtsingleapplication project released by the 
Qt people. I have added a few functions to support multilanguage UI.

The main project is lmc which contains the entire application. I have
included the project files for both projects.

Extract both folders. Make sure the directory hierarchy is maintained.
"lmc" and "lmcapp" should be folders at same level. Make sure the 
dependency paths of lmc are set to the correct locations. It depends 
on lmcapp and openssl, in addition to the standard Qt libraries.

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
Some custom scripts are used for automating part of the compilation and setup
of LAN Messenger. These scripts rely on an environment variable called
QTDIR that should contain the path where Qt libraries are installed. More
specifically, it should point to the parent folder of the bin and lib folders
where Qt binaries reside. Eg: C:\Qt\5.15.2\msvc2019_64 on Windows,
/usr/lib/x86_64-linux-gnu/qt5 on Linux.

Refer PLATFORM_SPECIFIC.md for additional details about setting up the build
environment on respective platforms.


Compiling LAN Messenger on Windows
==================================
Its possible to compile using other build chains/IDEs, but the CI build
uses qmake with the MSVC tool chain.

OpenSSL should be built/installed first. I recommend using a folder at 
the same level as "lmc" folder as the OpenSSL folder.

Next build "lmcapp" project. All the files needed are present inside
lmcapp\src folder. This project should be built as a shared library.
The library version is 2.0.0, so the import library is produced as
lmcapp2.lib. Rename it to lmcapp.lib so that the lmc project can link
against -llmcapp (the CI build does exactly this). The build scripts
look for the runtime DLL both in lmcapp\src and lmcapp\bin.

Finally build "lmc" project. This project references both OpenSSL and
lmcapp, so the correct paths to headers and libraries should be set
first.
The headers of lmcapp should be in lmcapp\include
The libraries of lmcapp should be in lmcapp\lib
The headers of OpenSSL should be in openssl\include
The libraries of OpenSSL should be in openssl\lib
Note: OpenSSL paths may be different depending on how it was built/installed
in your system.

Once you have built lmc, run the "buildwin32.bat" batch file found in
the src\scripts folder. This script compiles the translation files, builds 
the resources into a separate binary file and copies the application 
dependencies (lmcapp and the libcrypto/libssl OpenSSL DLLs) to the output 
folder. The path of output directory should be passed as a parameter for 
this script. The script depends on the QTDIR environment variable, so make 
sure it is set correctly. You can probably add the execution of this script 
as a custom build step in your IDE. That way it will be automatically called 
every time you build the project.

To gather the Qt libraries and plugins into the output folder, run
"windeployqt --release <output>\lmc.exe" afterwards, as done in the CI 
build.


Compiling LAN Messenger on X11/Linux
===================================
OpenSSL should be built first. I recommend using a folder at the same
level as "lmc" folder as the OpenSSL folder.

Next build "lmcapp" project. All the files needed are present inside
lmcapp/src folder. This project should be built as a shared library.
The output of this project is liblmcapp.so.2.0.0 (with the
liblmcapp.so.2 soname symlink), which will be created in lmcapp/lib 
folder.

Finally build "lmc" project. This project references both OpenSSL and
lmcapp, so the correct paths to headers and libraries should be set
first.
The headers of lmcapp should be in lmcapp/include
The libraries of lmcapp should be in lmcapp/lib
The headers of OpenSSL should be in openssl/include
The libraries of OpenSSL should be in openssl/lib
Note: OpenSSL paths may be different depending on how it was built/installed
in your system.

Once you have built lmc, run the "buildx11" shell script found in the
src/scripts folder. This script performs the same actions as its Windows 
counterpart. The QTDIR variable defined in the script should contain the 
correct path to Qt libraries. You can add the execution of this script as 
a custom build step to automate the whole process. If you get an error 
while executing the script, edit the script to make sure that the Qt 
plugins path in the script is correct.


Compiling LAN Messenger on Mac OS X
==================================
Build "lmcapp" project. All the files needed are present inside
lmcapp/src folder. This project should be built as a shared library.
The output of this project is liblmcapp.2.dylib, which will be created
in lmcapp/lib folder.

Finally build "lmc" project. This project references both lmcapp and
OpenSSL, so the correct paths to headers and libraries should be set 
first.
The headers of lmcapp should be in lmcapp/include
The libraries of lmcapp should be in lmcapp/lib
The headers of OpenSSL should be in openssl/include
The libraries of OpenSSL should be in openssl/lib

Note: Mac OS X does not ship a linkable OpenSSL. Install it as described
in the OpenSSL section above.

Once you have built lmc, run the "buildmacos" shell script found in the
src/scripts folder. This script performs the same actions as its Windows 
counterpart. The script depends on the QTDIR environment variable, so make 
sure it is set correctly. You can add the execution of this script as a 
custom build step to automate the whole process. If you get an error while 
executing the script, edit  the script to make sure that the Qt plugins 
path in the script is correct.

Note: On Mac OS X, option to start LAN Messenger on startup will not work.
This is a platform dependent function and I have not implemented it.


Audio playback support
=====================
LAN Messenger can play sounds to accompany certain events. This behaviour
is customizable in the Preferences dialog. Sounds are played through the
Qt Multimedia module (QSoundEffect), so that module must be available at
build and run time. On Linux install the Qt Multimedia development and
runtime packages (qtmultimedia5-dev, libqt5multimedia5). If no audio
output device is available, the sound options will be grayed out in the
Preferences dialog.


System tray support
==================
On desktops that do not have a system tray, the system tray options will
be grayed out in the Preferences dialog.
