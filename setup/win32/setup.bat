@echo off
if not "%~3"=="" (
  echo Extra arguments are not supported.
  exit /b 1
)
Powershell.exe -NoProfile -NonInteractive -ExecutionPolicy Bypass -File "%~dp0setup.ps1" "%~1" "%~2"
exit /b %ERRORLEVEL%
