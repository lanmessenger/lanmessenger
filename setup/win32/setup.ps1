# Build the Windows NSIS installer: setup.ps1 <ExeFolder> <ProductVersion>
# Example: setup.ps1 release 1.3.21 (setup.bat is a thin cmd wrapper).

param(
  [Parameter(Position = 0)]
  [string]$ExeFolder = '',
  [Parameter(Position = 1)]
  [string]$ProductVersion = ''
)

if ([string]::IsNullOrWhiteSpace($ExeFolder) -or [string]::IsNullOrWhiteSpace($ProductVersion)) {
  throw "Usage: setup.ps1 <ExeFolder> <ProductVersion>`nExample: setup.ps1 release 1.3.21"
}

$ErrorActionPreference = 'Stop'
Set-StrictMode -Version Latest

# makensis File/move paths are relative: run from the script dir,
# restoring the caller's directory afterwards.
Push-Location $PSScriptRoot
try {
  $exe = Join-Path $PSScriptRoot "..\..\$ExeFolder\lmc.exe"
  if (-not (Test-Path $exe)) { throw "lmc.exe not found in $exe." }

  # Stamps come from the exe: FileVersion string must match the argument,
  # numeric FileVersionRaw becomes the InstallerVersion.
  $vi = (Get-Item $exe).VersionInfo
  if ([string]::IsNullOrEmpty($vi.FileVersion)) { throw 'Could not read FileVersion from lmc.exe.' }
  if ($vi.FileVersion -ne $ProductVersion) {
    throw "lmc.exe FileVersion '$($vi.FileVersion)' != ProductVersion '$ProductVersion'."
  }
  $installerVersion = "$($vi.FileVersionRaw)"
  if ([string]::IsNullOrEmpty($installerVersion)) { throw 'Could not read FILEVERSION from lmc.exe.' }

  $makensis = if ($env:MAKENSIS) { $env:MAKENSIS } else { 'C:\Program Files (x86)\NSIS\makensis.exe' }
  & $makensis "/DExeFolder=$ExeFolder" "/DProductVersion=$ProductVersion" "/DInstallerVersion=$installerVersion" 'setup.nsi'
  if ($LASTEXITCODE -ne 0) { throw "makensis failed with exit code $LASTEXITCODE." }

  $installer = "lmc-$ProductVersion-win64.exe"
  if (-not (Test-Path $installer)) { throw "installer $installer not built." }
  Move-Item -Force $installer (Join-Path '..' $installer)
}
finally {
  Pop-Location
}
