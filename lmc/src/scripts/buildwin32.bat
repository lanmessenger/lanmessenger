@echo off

REM check if the folder path is specified in command line
if "%1" == "" goto null_param

REM compile all translation files that have .ts extension
echo Creating translation binaries...
if exist %1\lang goto lang_exists
mkdir %1\lang
:lang_exists
for %%f in (*.ts) do (
    %QTDIR%\bin\lrelease %%~nf.ts -qm %1\lang\%%~nf.qm
)
echo Done.
echo Copying system translations
xcopy resources\lang\system %1\lang\system /E /I /F /Y

echo Copying themes...
xcopy resources\themes %1\themes /E /I /F /Y
REM remove the 'Classic' theme from themes folder since its already present in resource file
rmdir /S /Q %1\themes\Classic
echo Done.

echo Building resource binary...
@echo on
%QTDIR%\bin\rcc -binary resource.qrc -o %1\lmc.rcc
echo Done.

echo Copying license...
copy /Y resources\text\license.txt %1\COPYING.txt

echo Copying sound files...
xcopy resources\sounds %1\sounds /E /I /F /Y

echo Copying plugins...
::if exist %1\imageformats goto plug_exists
::mkdir %1\imageformats
:::plug_exists
if "%2" == "release" goto release_mode
echo %2
::xcopy %QTDIR%\plugins\imageformats\qgifd4.dll %1\imageformats /I /F /Y
::xcopy %QTDIR%\plugins\imageformats\qicod4.dll %1\imageformats /I /F /Y
::xcopy %QTDIR%\plugins\imageformats\qjpegd4.dll %1\imageformats /I /F /Y
::xcopy %QTDIR%\plugins\imageformats\qtiffd4.dll %1\imageformats /I /F /Y

echo Copying libraries
copy /Y ..\..\lmcapp\bin\lmcappd2.dll %1\lmcappd2.dll
copy /Y ..\..\openssl\bin\libeay32.dll %1\libeay32.dll
goto end

:release_mode
::xcopy %QTDIR%\plugins\imageformats\qgif4.dll %1\imageformats /I /F /Y
::xcopy %QTDIR%\plugins\imageformats\qico4.dll %1\imageformats /I /F /Y
::xcopy %QTDIR%\plugins\imageformats\qjpeg4.dll %1\imageformats /I /F /Y
::xcopy %QTDIR%\plugins\imageformats\qtiff4.dll %1\imageformats /I /F /Y

echo Copying libraries
copy /Y ..\..\lmcapp\bin\lmcapp2.dll %1\lmcapp2.dll
copy /Y ..\..\openssl\bin\libeay32.dll %1\libeay32.dll
goto end

:null_param
echo Error: Missing output path
echo Usage: custombuild path\to\build\directory
exit /b 1

:end
exit /b 0