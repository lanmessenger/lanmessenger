@echo off
"C:\Program Files (x86)\NSIS\makensis" /DExeFolder=build-lmc-Desktop_Qt_5_11_2_MSVC2017_32bit_Static_3-Release setup.nsi

for %%f in (*.exe) do (
    move /Y %%~nf.exe ..\%%~nf.exe
)

exit /b 0