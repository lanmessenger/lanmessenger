echo on

SET project_dir="%cd%"


goto %1

:mingw32
set PATH=C:\Qt\5.5\mingw492_32\bin;C:\Qt\Tools\mingw492_32\bin;%PATH%

cd ./lmcapp/src
qmake lmcapp.pro -spec win32-g++ CONFIG+=x86 CONFIG-=debug CONFIG+=release
mingw32-make
move ..\lib\liblmcapp2.a ..\lib\liblmcapp.a

cd ../../lmc/src
qmake lmc.pro -spec win32-g++ CONFIG+=x86 CONFIG-=debug CONFIG+=release
mingw32-make
goto endmake



:msvc2013_32
set PATH=C:\Qt\5.5\msvc2013\bin;%PATH%
call "C:\Program Files (x86)\Microsoft Visual Studio 12.0\VC\vcvarsall.bat" x86
@echo on

cd ./lmcapp/src
qmake lmcapp.pro -spec win32-msvc2013 CONFIG+=x86 CONFIG-=debug CONFIG+=release
nmake
move ..\lib\lmcapp2.lib ..\lib\lmcapp.lib

cd ../../lmc/src
qmake lmc.pro -spec win32-msvc2013 CONFIG+=x86 CONFIG-=debug CONFIG+=release
nmake
goto endmake



:msvc2013_64
set PATH=C:\Qt\5.5\msvc2013_64\bin;%PATH%
call "C:\Program Files (x86)\Microsoft Visual Studio 12.0\VC\vcvarsall.bat" amd64
@echo on

cd ./lmcapp/src
qmake lmcapp.pro CONFIG+=x86_64 CONFIG-=debug CONFIG+=release
nmake
move ..\lib\lmcapp2.lib ..\lib\lmcapp.lib

cd ../../lmc/src
qmake lmc.pro CONFIG+=x86_64 CONFIG-=debug CONFIG+=release
nmake
goto endmake


:endmake
