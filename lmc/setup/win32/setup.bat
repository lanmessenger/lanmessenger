@echo off
"C:\Program Files (x86)\NSIS\makensis" setup.nsi

for %%f in (*.exe) do (
    move /Y %%~nf.exe ..\%%~nf.exe
)

exit /b 0