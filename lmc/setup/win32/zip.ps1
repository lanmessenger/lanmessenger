#
# Zip the Windows build output.
# Usage: zip.ps1 [-ExeFolder <dir relative to lmc>] [-Version <ver>]
#

param(
	[string]$ExeFolder = "release",
	[string]$Version = "1.2.39"
)

$src = Join-Path "..\.." $ExeFolder

$items = @(
	(Join-Path $src "lmc.exe"),
	(Join-Path $src "sounds"),
	(Join-Path $src "lang"),
	"..\..\src\resources\text\license.txt",
	"..\..\src\resources\text\readme.txt"
)
$items += @(Get-ChildItem -Path $src -Filter "libcrypto*.dll" -ErrorAction SilentlyContinue | ForEach-Object FullName)
$items += @(Get-ChildItem -Path $src -Filter "libssl*.dll" -ErrorAction SilentlyContinue | ForEach-Object FullName)
$items += @(Get-ChildItem -Path $src -Filter "lmcapp*.dll" -ErrorAction SilentlyContinue | ForEach-Object FullName)

Compress-Archive `
	-LiteralPath $items `
	-CompressionLevel Optimal `
	-Force `
	-DestinationPath ..\lmc-$Version-win64.zip
