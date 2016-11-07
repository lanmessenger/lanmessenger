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

Extract both folders. Make sure the directory hierarchy is maintained.
"lmc" and "lmcapp" should be folders at same level. Make sure the 
dependency paths of lmc are set to the correct locations. It depends 
on lmcapp and openssl, in addition to the standard Qt libraries.

The code is identical for all platforms, but there are a few differences
in the way application is built and run on each platform. Please read
the platform specific notes to know more.

Note: I used Qt Creator IDE with gcc compiler on all platforms. If you 
are using any other IDE and/or compiler and run into any issue, I can 
only provide generic help.

Compiling OpenSSL
=================
Note: Compiling OpenSSL is not required for Mac OS X since OpenSSL is 
included in OS X.

The steps for compilation varies depending on your platform.
The package that you get from OpenSSL website gives detailed instructions
on how to compile on all supported platforms. OpenSSL itself may need
additional software packages to compile depending on your system.

Make sure that OpenSSL is built as a shared library. On Windows,
specify "enable-static-engine" switch so that only one dll is built
instead of a myriad of separate dlls. On Linux, specify the "shared"
switch to build as shared library.

Compiling LAN Messenger on Windows
==================================
Its possible to compile using other build chains/IDEs. But its better 
to stick to Qt Creator and gcc tool chain.

OpenSSL should be built first. I recommend using a folder at the same
level as "lmc" folder as the OpenSSL folder.

Next build "lmcapp" project. All the files needed are present inside
lmcapp\src folder. This project should be built as a shared library.
The outputs of this project are lmcapp.dll and lmcapp.a, which will be
created in lmcapp\lib folder. The .dll file should be moved to 
lmcapp\bin and the .a file should be left in lmcapp\lib. In case of 
debug build, the output files will be lmcappd.dll and lmcappd.a

Finally build "lmc" project. This project references both OpenSSL and
lmcapp, so the correct paths to headers and libraries should be set
first.
The headers of lmcapp should be in lmcapp\include
The libraries of lmcapp should be in lmcapp\lib
The headers of OpenSSL should be in openssl\include
The libraries of OpenSSL should be in openssl\lib
Note: OpenSSL paths may be different depending on how it was built/installed
in your system.

Once you have built lmc, run the "buildwin32.bat" batch file. This
script compiles the translation files, builds the resources into a
separate binary file and copies the application dependencies to the
output folder. The path of output directory should be passed as a
parameter for this script. The script depends on the QTDIR environment
variable, so make sure it is set correctly. You can probably add the
execution of this script as a custom build step in your IDE. That way
it will be automatically called every time you build the project.

Compiling LAN Messenger on X11/Linux
====================================
OpenSSL should be built first. I recommend using a folder at the same
level as "lmc" folder as the OpenSSL folder.

Next build "lmcapp" project. All the files needed are present inside
lmcapp\src folder. This project should be built as a shared library.
The outputs of this project is liblmcapp.1.0.0.so, which will be
created in lmcapp\lib folder.

Finally build "lmc" project. This project references both OpenSSL and
lmcapp, so the correct paths to headers and libraries should be set
first.
The headers of lmcapp should be in lmcapp\include
The libraries of lmcapp should be in lmcapp\lib
The headers of OpenSSL should be in openssl\include
The libraries of OpenSSL should be in openssl\lib
Note: OpenSSL paths may be different depending on how it was built/installed
in your system.

Once you have built lmc, run the "buildx11" shell script. This script 
performs the same actions as its Windows counterpart. It does not depend 
on any environment variable. You can add the execution of this script as 
a custom build step to automate the whole process. If you get an error while
executing the script, edit the script to make sure that the Qt plugins path
in the script is correct.

Compiling LAN Messenger on Mac OS X
===================================
Build "lmcapp" project. All the files needed are present inside
lmcapp\src folder. This project should be built as a shared library.
The outputs of this project is liblmcapp.1.0.0.dylib, which will be
created in lmcapp\lib folder.

Finally build "lmc" project. This project references lmcapp, so the 
correct paths to headers and libraries should be set first.
The headers of lmcapp should be in lmcapp\include
The libraries of lmcapp should be in lmcapp\lib

Note: There is no need to link to OpenSSL since it is present as a system
library on Mac OS X.

Once you have built lmc, run the "buildmacos" shell script. This script 
performs the same actions as its Windows counterpart. It does not depend 
on any environment variable. You can add the execution of this script as 
a custom build step to automate the whole process. If you get an error while
executing the script, edit the script to make sure that the Qt plugins path
in the script is correct.

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
