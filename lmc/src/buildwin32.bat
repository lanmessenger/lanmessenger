@echo off

REM check if the folder path is specified in command line
if "%1" == "" goto null_param

REM compile all translation files that have .ts extension
echo Creating translation binaries...
if exist %1lang goto lang_exists
mkdir %1lang
:lang_exists
for %%f in (*.ts) do (
    %QTDIR%\bin\lrelease %%~nf.ts -qm %1lang\%%~nf.qm
)
echo Done.

if exist %1themes goto themes_exists
mkdir %1themes
:themes_exists
echo Done.


echo Building resource binary...
%QTDIR%\bin\rcc -binary resource.qrc -o %1lmc.rcc
echo Done.

echo Copying license...
copy /Y resources\text\license.txt %1COPYING.txt

echo Copying sound files...
xcopy resources\sounds %1sounds /E /I /F /Y

echo Copying plugins...
if exist %1imageformats goto plug_exists
mkdir %1imageformats
:plug_exists
if "%2" == "release" goto release_plug
echo %2
xcopy %QTDIR%\plugins\imageformats\qgifd4.dll %1imageformats /I /F /Y
xcopy %QTDIR%\plugins\imageformats\qicod4.dll %1imageformats /I /F /Y
xcopy %QTDIR%\plugins\imageformats\qjpegd4.dll %1imageformats /I /F /Y
xcopy %QTDIR%\plugins\imageformats\qtiffd4.dll %1imageformats /I /F /Y
goto end

:release_plug
xcopy %QTDIR%\plugins\imageformats\qgif4.dll %1imageformats /I /F /Y
xcopy %QTDIR%\plugins\imageformats\qico4.dll %1imageformats /I /F /Y
xcopy %QTDIR%\plugins\imageformats\qjpeg4.dll %1imageformats /I /F /Y
xcopy %QTDIR%\plugins\imageformats\qtiff4.dll %1imageformats /I /F /Y
goto end

:null_param
echo Error: Missing output path
echo Usage: custombuild path\to\build\directory\
echo Note that the path should end with a back slash (\).
exit /b 1

:end
exit /b 0