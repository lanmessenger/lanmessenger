@echo off
if "%~1"=="" (
  echo Usage: setup.bat ^<ExeFolder^> [ProductVersion] [InstallerVersion]
  echo Example: setup.bat release 1.2.39 1.2.3.9
  exit /b 1
)

set "VERARGS="
if not "%~2"=="" set "VERARGS=/DProductVersion=%~2"
if not "%~3"=="" set "VERARGS=%VERARGS% /DInstallerVersion=%~3"

if not defined MAKENSIS set "MAKENSIS=C:\Program Files (x86)\NSIS\makensis.exe"
"%MAKENSIS%" /DExeFolder=%~1 %VERARGS% setup.nsi
if errorlevel 1 exit /b 1

for %%f in (*.exe) do (
    move /Y %%~nf.exe ..\%%~nf.exe
)

exit /b 0