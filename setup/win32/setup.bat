@echo off
rem Run from the script dir: makensis paths and the move below are relative.
cd /d "%~dp0"
if "%~1"=="" (
  echo Usage: setup.bat ^<ExeFolder^> ^<ProductVersion^>
  echo Example: setup.bat release 1.3.21
  exit /b 1
)
if "%~2"=="" (
  echo ProductVersion is required.
  exit /b 1
)

set "ProductVersion=%~2"
if not exist "..\..\%~1\lmc.exe" (
  echo lmc.exe not found in ..\..\%~1.
  exit /b 1
)
rem Stamps come from the exe: FileVersion string must match the argument,
rem numeric FileVersionRaw becomes the InstallerVersion.
for /f "tokens=1,2 delims=;" %%a in ('powershell -noprofile -command "$v=Get-Item '..\..\%~1\lmc.exe'; '{0};{1}' -f $v.VersionInfo.FileVersion, $v.VersionInfo.FileVersionRaw"') do (
  set "ExeFileVersion=%%a"
  set "InstallerVersion=%%b"
)
if not defined ExeFileVersion (
  echo Could not read FileVersion from lmc.exe.
  exit /b 1
)
if not defined InstallerVersion (
  echo Could not read FILEVERSION from lmc.exe.
  exit /b 1
)
if not "%ExeFileVersion%"=="%ProductVersion%" (
  echo lmc.exe FileVersion '%ExeFileVersion%' != ProductVersion '%ProductVersion%'.
  exit /b 1
)

set "VERARGS=/DProductVersion=%ProductVersion% /DInstallerVersion=%InstallerVersion%"

if not defined MAKENSIS set "MAKENSIS=C:\Program Files (x86)\NSIS\makensis.exe"
"%MAKENSIS%" /DExeFolder="%~1" %VERARGS% setup.nsi
if errorlevel 1 exit /b 1

if not exist "lmc-%ProductVersion%-win64.exe" (
  echo installer lmc-%ProductVersion%-win64.exe not built.
  exit /b 1
)
move /Y "lmc-%ProductVersion%-win64.exe" "..\lmc-%ProductVersion%-win64.exe"

exit /b 0
