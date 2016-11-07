How to compile LAN Messenger
============================

You need Qt (http://qt.nokia.com/) to compile.
I built LAN Messenger using Qt 4.7, so you probably need that or a 
later version.

You also need OpenSSL (http://www.openssl.org/)
I used version 1.0.0.e, so you know what to get.
Please read the OpenSSL section for more information.

The application consists of two projects - lmc and lmcapp. lmcapp is 
just an extension of the qtsingleapplication project released by the 
Qt people. I have added a few functions to support multilanguage UI.

The main project is lmc which contains the entire application. I have
included the project files for both projects.

Extract both folders. Its better if the directory hierarchy is 
maintained. ie, lmc and lmcapp should be folders at same level. Make 
sure the dependency paths of lmc are set to the correct locations. It
depends on lmcapp and openssl, in addition to the standard Qt libraries.

The code is identical for all platforms, but there are a few differences
in the way application is built and run on each platform. Please read
the platform specific notes to know more.

Note: I used MS Visual Studio 2010 IDE with MSVC 2010 compiler on Windows and
Qt Creator IDE with gcc compiler on Linux. If you are using any other IDE and
compilers on these platforms, I can only provide generic help if you run into
any trouble.

Compiling OpenSSL
=================
The steps for this varies depending on your platform.
The package that you get from OpenSSL website gives detailed instructions
on how to compile on all supported platforms. OpenSSL itself may need
additional software packages to compile depending on your system.

Make sure that OpenSSL is built as a shared library. On Windows,
specify "enable-static-engine" switch so that only one dll is built
instead of a myriad of separate dlls.

Compiling LAN Messenger on Windows
==================================
Its theoretically possible to compile using any build chain/IDE. But
its better to stick to Qt Creator or Visual Studio (with Qt add-in).

OpenSSL should be built first. I recommend using a folder at the same
level as "lmc" folder as the OpenSSL folder.

Next buid "lmcapp" project. All the files needed are present inside
lmcapp\src folder. This project should be built as a shared library.
The outputs of this project are lmcapp.dll and lmcapp.lib. The DLL file
should be copied to lmcapp\bin and the LIB file should be copied to
lmcapp\lib.

Finally build "lmc" project. This project references both OpenSSL and
lmcapp, so the correct paths to headers and libraries should be set
first.
The headers of lmcapp should be in lmcapp\include
The libraries of lmcapp should be in lmcapp\lib
The headers of OpenSSL should be in openssl\include
The libraries of OpenSSL should be in openssl\lib

Once you have built lmc, run the "custombuild.bat" batch file. This
script compiles the translation files, builds the resources into a
separate binary file and copies the application dependencies to the
output folder. The path of output directory should be passed as a
parameter for this script. The script depends on the QTDIR environment
variable, so make sure it is set correctly. You can probably add the
execution of this script as a custom build step in your IDE. That way
it will be automatically called every time you build the project.

Compiling LAN Messenger on X11/Linux
====================================
The steps for compiling the projects are same as that for Windows. The
only difference is the script that is executed after build. For Linux
run the "custombuild" shell script. This script performs the same actions
as its Windows counterpart. It does not depend on any environment
variable. You can add the execution of this script as a custom build step
to automate the whole process.

Compiling LAN Messenger on Mac OS X
===================================
I guess this should be very similar to how it is done on Linux. I wouldn't
know since I do not have access to a Mac system. The application is
guaranteed to work since its built on the Qt framework which provides
cross-platform support. I would be much obliged if anyone is kind enough 
to test it on Mac OS X.

Note: On Mac OS X, option to start LAN Messenger on startup will not work.
This is a platform dependent function and I have not implemented it.

Audio playback support
======================
LAN Messenger can play sounds to accompany certain events. This behaviour
is customizable in the Preferences dialog. Sound functions are provided
by Qt which in turn depend on different subsystem on each platform. This
should not be a problem in Windows and Mac. However on Linux, Qt needs the
Network Audio System for sound functions. If NAS is not available the
application will not play sounds. Sound options will be grayed out in the
Preferences dialog.

System tray support
===================
On desktops that do not have a system tray, the system tray options will
be grayed out in the Preferences dialog.
