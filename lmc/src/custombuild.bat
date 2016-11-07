echo Creating translation binaries...
%QTDIR%\bin\lrelease lmc_en.ts -qm resources\lang\en_US.qm
%QTDIR%\bin\lrelease lmc_ml.ts -qm resources\lang\ml_IN.qm
echo Done.

echo Building resource binary...
%QTDIR%\bin\rcc -binary resource.qrc -o %1lmc.rcc
echo Done.

xcopy resources\text\license.txt %1COPYING.txt /F /Y

echo Copying sound files...
xcopy resources\sounds %1sounds /E /I /F /Y
echo Done.

echo Copying plugins...
if exist %1imageformats goto exists
mkdir %1imageformats
:exists
xcopy %QTDIR%\plugins\imageformats\qico4.dll %1imageformats /I /F /Y
echo Done.