#
# Script.ps1
#

$exePath = '..\..\build-lmc-Desktop_Qt_5_11_2_MSVC2017_32bit_Static_3-Release'

Compress-Archive `
	-LiteralPath $exePath\lmc.exe, `
				$exePath\libeay32.dll, `
				$exePath\ssleay32.dll, `
				$exePath\sounds, `
				$exePath\lang, `
				..\..\src\resources\text\license.txt, `
				..\..\src\resources\text\readme.txt `
	-CompressionLevel Optimal `
	-Force `
	-DestinationPath ..\lmc-1.2.39-win32.zip

