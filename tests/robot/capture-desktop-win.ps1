# Desktop capture for the screenshots E2E suite (Windows leg).
# Native .NET only (System.Drawing + System.Windows.Forms CopyFromScreen):
# no Pillow, no extra pip packages on windows-2022.
# Called from common.resource (Capture Desktop Screenshot On Windows):
#   powershell -NoProfile -ExecutionPolicy Bypass -File capture-desktop-win.ps1 -OutPath <png>
# Primary screen only at 100% DPI (the windows-2022 default): multi-monitor
# and non-100% scaling are out of scope for v1 shots.
param([Parameter(Mandatory=$true)][string]$OutPath)
Add-Type -AssemblyName System.Drawing
Add-Type -AssemblyName System.Windows.Forms
$bmp = $null
$g = $null
try {
    $b = [System.Windows.Forms.Screen]::PrimaryScreen.Bounds
    $bmp = New-Object System.Drawing.Bitmap($b.Width, $b.Height)
    $g = [System.Drawing.Graphics]::FromImage($bmp)
    $g.CopyFromScreen($b.Location, [System.Drawing.Point]::Empty, $b.Size)
    $bmp.Save($OutPath, [System.Drawing.Imaging.ImageFormat]::Png)
}
finally {
    if ($g -ne $null) { $g.Dispose() }
    if ($bmp -ne $null) { $bmp.Dispose() }
}
