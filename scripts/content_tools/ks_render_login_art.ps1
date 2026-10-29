# KStorm login screen art: renders the background and the logo used by the native login panel
# (panel_fs_nui_login.xml: ks_login_bg, ks_login_logo) with Windows GDI+ and the Segoe UI fonts.
# Run from Windows PowerShell:  powershell -ExecutionPolicy Bypass -File ks_render_login_art.ps1
# It writes into indra/newview/skins/default/textures/windows/ and prints the logo's display
# size (half its pixel size; it is drawn at 2x so it stays sharp with UI scaling).
# @author Kissmy Spicoli

Add-Type -AssemblyName System.Drawing

$Root   = Split-Path -Parent (Split-Path -Parent $PSScriptRoot)
$OutDir = Join-Path $Root "indra\newview\skins\default\textures\windows"

# ---- Design values (1x units; the logo renders at $S times) ----
$S            = 2
$Orange       = [System.Drawing.Color]::FromArgb(255, 0xF9, 0x7D, 0x00)   # Log In button, PushButton_Login.png middle
$TitleColor   = [System.Drawing.Color]::FromArgb(255, 0xFF, 0xFF, 0xFF)
$SubColor     = [System.Drawing.Color]::FromArgb(255, 0xE6, 0xDE, 0xF5)
$TitleFont    = "Segoe UI Black"; $TitleSize = 132
$SubFont      = "Segoe UI Light"; $SubSize   = 34
$SpacingScale = 0.9      # "Research" tracking: 1.0 spans the width of "KStorm"
$Gap          = 37       # title baseline to "Research" cap top; the beam runs midway
$BeamHeight   = 5
$BeamReach    = 84       # beam runs this far past the end of "KStorm", to the burst centre
$BurstSize    = 100      # burst diameter
$Pad          = 6

function New-TextPath([string]$text, [string]$family, [float]$size, [float]$x, [float]$y)
{
    $path = New-Object System.Drawing.Drawing2D.GraphicsPath
    $fam  = New-Object System.Drawing.FontFamily($family)
    $path.AddString($text, $fam, [int][System.Drawing.FontStyle]::Regular, $size, (New-Object System.Drawing.PointF($x, $y)), [System.Drawing.StringFormat]::GenericTypographic)
    return $path
}

# ---- Logo layout, in output pixels ----
$titlePath = New-TextPath "KStorm" $TitleFont ($TitleSize * $S) 0 0
$tb = $titlePath.GetBounds()

# "Research": one path per letter, advances from typographic measurement, then tracked
$probe = New-Object System.Drawing.Bitmap(8, 8)
$pg = [System.Drawing.Graphics]::FromImage($probe)
$subFontObj = New-Object System.Drawing.Font($SubFont, [float]($SubSize * $S), [System.Drawing.FontStyle]::Regular, [System.Drawing.GraphicsUnit]::Pixel)
$letters = "Research".ToCharArray()
$adv = @(); foreach ($c in $letters) { $adv += $pg.MeasureString([string]$c, $subFontObj, 10000, [System.Drawing.StringFormat]::GenericTypographic).Width }
$natural = ($adv | Measure-Object -Sum).Sum
$spacing = 0.0
if ($letters.Count -gt 1 -and $natural -lt $tb.Width) { $spacing = $SpacingScale * ($tb.Width - $natural) / ($letters.Count - 1) }
$subPath = New-Object System.Drawing.Drawing2D.GraphicsPath
$x = 0.0
for ($i = 0; $i -lt $letters.Count; $i++)
{
    $p = New-TextPath ([string]$letters[$i]) $SubFont ($SubSize * $S) $x 0
    $subPath.AddPath($p, $false)
    $x += $adv[$i] + $spacing
}
$sb = $subPath.GetBounds()

# Place "Research" centred under the title ink, $Gap below its baseline (ink bottom; no descenders)
$titleCx = $tb.X + $tb.Width / 2
$m = New-Object System.Drawing.Drawing2D.Matrix
$m.Translate(($titleCx - ($sb.X + $sb.Width / 2)), ($tb.Bottom + $Gap * $S - $sb.Y))
$subPath.Transform($m)
$sb = $subPath.GetBounds()

$beamY    = ($tb.Bottom + $sb.Top) / 2
$beamEndX = $tb.Right + $BeamReach * $S
$burstR   = $BurstSize * $S / 2

# Canvas symmetric about the title centre, so the title is centred when the image is
$half   = [Math]::Max($tb.Width / 2, ($beamEndX + $burstR) - $titleCx) + $Pad * $S
$top    = [Math]::Min($tb.Top, $beamY - $burstR) - $Pad * $S
$bottom = [Math]::Max($sb.Bottom, $beamY + $burstR) + $Pad * $S
$W = [int][Math]::Ceiling(2 * $half); $W += $W % 2
$H = [int][Math]::Ceiling($bottom - $top); $H += $H % 2
$ox = $half - $titleCx; $oy = -$top

$bmp = New-Object System.Drawing.Bitmap($W, $H, [System.Drawing.Imaging.PixelFormat]::Format32bppArgb)
$g = [System.Drawing.Graphics]::FromImage($bmp)
$g.SmoothingMode = [System.Drawing.Drawing2D.SmoothingMode]::AntiAlias
$g.PixelOffsetMode = [System.Drawing.Drawing2D.PixelOffsetMode]::HighQuality
$g.Clear([System.Drawing.Color]::Transparent)
$g.TranslateTransform($ox, $oy)

$g.FillPath((New-Object System.Drawing.SolidBrush($TitleColor)), $titlePath)
$g.FillPath((New-Object System.Drawing.SolidBrush($SubColor)), $subPath)

# Beam and starburst: one shape in the Log In orange
$orangeBrush = New-Object System.Drawing.SolidBrush($Orange)
$g.FillRectangle($orangeBrush, [float]$tb.Left, [float]($beamY - $BeamHeight * $S / 2), [float]($beamEndX - $tb.Left), [float]($BeamHeight * $S))
$k = $burstR / 50.0          # burst designed in a 100-unit box centred on 0,0
$g.FillEllipse($orangeBrush, [float]($beamEndX - 19 * $k), [float]($beamY - 19 * $k), [float](38 * $k), [float](38 * $k))
for ($i = 0; $i -lt 24; $i++)
{
    $a  = $i * [Math]::PI / 12
    $L  = if ($i % 2 -eq 0) { 50 } else { 37 }        # long and short rays in turn
    $hw = if ($i % 2 -eq 0) { 3.4 } else { 2.6 }      # half width at the centre
    $pts = @(
        (New-Object System.Drawing.PointF([float]($beamEndX - [Math]::Sin($a) * $hw * $k), [float]($beamY + [Math]::Cos($a) * $hw * $k))),
        (New-Object System.Drawing.PointF([float]($beamEndX + [Math]::Cos($a) * $L * $k),  [float]($beamY + [Math]::Sin($a) * $L * $k))),
        (New-Object System.Drawing.PointF([float]($beamEndX + [Math]::Sin($a) * $hw * $k), [float]($beamY - [Math]::Cos($a) * $hw * $k)))
    )
    $g.FillPolygon($orangeBrush, [System.Drawing.PointF[]]$pts)
}
$g.Dispose()
$logoFile = Join-Path $OutDir "ks_login_logo.png"
$bmp.Save($logoFile, [System.Drawing.Imaging.ImageFormat]::Png)
$bmp.Dispose()

# ---- Background: the page's radial gradient (#2b2140 at 50%/40% to #121018), stretched to fill ----
$BW = 1280; $BH = 720
$bg = New-Object System.Drawing.Bitmap($BW, $BH, [System.Drawing.Imaging.PixelFormat]::Format32bppArgb)
$bgg = [System.Drawing.Graphics]::FromImage($bg)
$edge = [System.Drawing.Color]::FromArgb(255, 0x12, 0x10, 0x18)
$bgg.Clear($edge)
$cx = $BW * 0.5; $cy = $BH * 0.4
$rx = 0.7 * [Math]::Sqrt(2) * $BW * 0.5; $ry = 0.7 * [Math]::Sqrt(2) * $BH * 0.6   # CSS farthest-corner ellipse, 70% stop
$ell = New-Object System.Drawing.Drawing2D.GraphicsPath
$ell.AddEllipse([float]($cx - $rx), [float]($cy - $ry), [float](2 * $rx), [float](2 * $ry))
$pgb = New-Object System.Drawing.Drawing2D.PathGradientBrush($ell)
$pgb.CenterPoint = New-Object System.Drawing.PointF([float]$cx, [float]$cy)
$pgb.CenterColor = [System.Drawing.Color]::FromArgb(255, 0x2B, 0x21, 0x40)
$pgb.SurroundColors = [System.Drawing.Color[]]@($edge)
$bgg.FillPath($pgb, $ell)
$bgg.Dispose()
$bgFile = Join-Path $OutDir "ks_login_bg.png"
$bg.Save($bgFile, [System.Drawing.Imaging.ImageFormat]::Png)
$bg.Dispose()

"logo: {0} x {1} px, display {2} x {3}" -f $W, $H, ($W / $S), ($H / $S)
"background: {0} x {1} px" -f $BW, $BH
