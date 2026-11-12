[CmdletBinding()]
param(
    [string]$GameRoot = '',
    [string]$ItemId = '9100000015'
)

$ErrorActionPreference = 'Stop'
Set-StrictMode -Version Latest

$repo = Split-Path $PSScriptRoot -Parent
if ([string]::IsNullOrWhiteSpace($GameRoot)) { $GameRoot = Split-Path $repo -Parent }
$sourceRoot = Join-Path $repo 'build\earthworks-workshop'
$releaseRoot = Join-Path $repo 'build\release\EarthworksPlanning'
$wipRoot = Join-Path $GameRoot 'media_soviet\workshop_wip'
$wipItem = Join-Path $wipRoot $ItemId
$pluginSource = Join-Path $repo 'build\windows-x64\bin\EarthworksPlanning.dll'
$rmlManifestSource = Join-Path $repo 'assets\rml\EarthworksPlanning.rml.json'
$tesmioInstallerSource = Join-Path $repo 'scripts\Install-TesmioPlugin.ps1'
$tesmioReadmeSource = Join-Path $repo 'docs\TESMIO_PLAYTEST.md'

$facilities = @(
    [pscustomobject]@{ SourceId = '9100000012'; Name = 'Small' },
    [pscustomobject]@{ SourceId = '9100000013'; Name = 'Medium' },
    [pscustomobject]@{ SourceId = '9100000014'; Name = 'Large' }
)

if ($ItemId -notmatch '^\d{10}$') { throw 'ItemId must contain exactly 10 digits.' }
if (-not (Test-Path -LiteralPath $pluginSource)) { throw 'Build the player-facing plugin before packaging.' }
if (-not (Test-Path -LiteralPath $rmlManifestSource)) { throw "RML manifest is missing: $rmlManifestSource" }
if (-not (Test-Path -LiteralPath $tesmioInstallerSource)) { throw "Tesmio installer is missing: $tesmioInstallerSource" }
if (-not (Test-Path -LiteralPath $tesmioReadmeSource)) { throw "Tesmio playtest instructions are missing: $tesmioReadmeSource" }
if (-not (Test-Path -LiteralPath $sourceRoot)) { throw "Facility package sources are missing: $sourceRoot" }

if (Test-Path -LiteralPath $wipItem) {
    $existingConfig = Join-Path $wipItem 'workshopconfig.ini'
    if (-not (Test-Path -LiteralPath $existingConfig)) {
        throw "Refusing to modify an existing unrecognized WIP folder: $wipItem"
    }
    $existingText = Get-Content -LiteralPath $existingConfig -Raw
    $expectedIdPattern = '(?m)^\$ITEM_ID\s+' + [regex]::Escape($ItemId) + '\s*$'
    if ($existingText -notmatch $expectedIdPattern -or
        $existingText -notmatch '(?m)^\$ITEM_NAME\s+"Earthworks Planning \(Playtest\)"\s*$') {
        throw "Refusing to modify an existing WIP folder that does not match this package: $wipItem"
    }
}

Add-Type -AssemblyName System.Drawing

function Copy-ScaledPng([string]$Source, [string]$Destination, [int]$MaxSide = 512) {
    $sourceImage = [System.Drawing.Image]::FromFile($Source)
    try {
        $scale = [Math]::Min(1.0, $MaxSide / [double][Math]::Max($sourceImage.Width, $sourceImage.Height))
        $width = [Math]::Max(1, [int][Math]::Round($sourceImage.Width * $scale))
        $height = [Math]::Max(1, [int][Math]::Round($sourceImage.Height * $scale))
        $bitmap = [System.Drawing.Bitmap]::new($width, $height, [System.Drawing.Imaging.PixelFormat]::Format32bppArgb)
        try {
            $graphics = [System.Drawing.Graphics]::FromImage($bitmap)
            try {
                $graphics.Clear([System.Drawing.Color]::Transparent)
                $graphics.CompositingQuality = [System.Drawing.Drawing2D.CompositingQuality]::HighQuality
                $graphics.InterpolationMode = [System.Drawing.Drawing2D.InterpolationMode]::HighQualityBicubic
                $graphics.SmoothingMode = [System.Drawing.Drawing2D.SmoothingMode]::HighQuality
                $graphics.DrawImage($sourceImage, 0, 0, $width, $height)
            } finally { $graphics.Dispose() }
            $bitmap.Save($Destination, [System.Drawing.Imaging.ImageFormat]::Png)
        } finally { $bitmap.Dispose() }
    } finally { $sourceImage.Dispose() }
}

function New-PackagePreview([string[]]$Images, [string]$Destination) {
    $tileWidth = 320
    $tileHeight = 240
    $labelHeight = 48
    $canvas = [System.Drawing.Bitmap]::new($tileWidth * $Images.Count, $tileHeight + $labelHeight, [System.Drawing.Imaging.PixelFormat]::Format32bppArgb)
    try {
        $graphics = [System.Drawing.Graphics]::FromImage($canvas)
        try {
            $graphics.Clear([System.Drawing.Color]::FromArgb(255, 51, 58, 61))
            $graphics.CompositingQuality = [System.Drawing.Drawing2D.CompositingQuality]::HighQuality
            $graphics.InterpolationMode = [System.Drawing.Drawing2D.InterpolationMode]::HighQualityBicubic
            $graphics.SmoothingMode = [System.Drawing.Drawing2D.SmoothingMode]::HighQuality
            $labels = @('Small', 'Medium', 'Large')
            for ($i = 0; $i -lt $Images.Count; $i++) {
                $img = [System.Drawing.Image]::FromFile($Images[$i])
                try {
                    $availableWidth = $tileWidth - 20
                    $availableHeight = $tileHeight - 20
                    $scale = [Math]::Min($availableWidth / [double]$img.Width, $availableHeight / [double]$img.Height)
                    $drawWidth = [int][Math]::Round($img.Width * $scale)
                    $drawHeight = [int][Math]::Round($img.Height * $scale)
                    $x = $i * $tileWidth + [int](($tileWidth - $drawWidth) / 2)
                    $y = [int](($tileHeight - $drawHeight) / 2)
                    $graphics.DrawImage($img, $x, $y, $drawWidth, $drawHeight)
                } finally { $img.Dispose() }
                $font = [System.Drawing.Font]::new('Arial', 20, [System.Drawing.FontStyle]::Bold)
                try {
                    $format = [System.Drawing.StringFormat]::new()
                    try {
                        $format.Alignment = [System.Drawing.StringAlignment]::Center
                        $format.LineAlignment = [System.Drawing.StringAlignment]::Center
                        $rect = [System.Drawing.RectangleF]::new([single]($i * $tileWidth), [single]$tileHeight, [single]$tileWidth, [single]$labelHeight)
                        $graphics.DrawString($labels[$i], $font, [System.Drawing.Brushes]::White, $rect, $format)
                    } finally { $format.Dispose() }
                } finally { $font.Dispose() }
            }
        } finally { $graphics.Dispose() }
        $canvas.Save($Destination, [System.Drawing.Imaging.ImageFormat]::Png)
    } finally { $canvas.Dispose() }
}

$expectedIdLines = @("`$ITEM_ID $ItemId", '$OWNER_ID 0', '$ITEM_TYPE WORKSHOP_ITEMTYPE_BUILDING', '$VISIBILITY 0')
$itemLines = [System.Collections.Generic.List[string]]::new()
$itemLines.AddRange([string[]]$expectedIdLines)
$itemLines.Add('$ITEM_NAME "Earthworks Planning (Playtest)"')
$itemLines.Add('$ITEM_DESC "Local playtest package. Includes Earthworks Planning and the Small, Medium, and Large Earthworks facilities. TesmioLoader is primary; RML is an optional fallback."')

New-Item -ItemType Directory -Path $releaseRoot -Force | Out-Null
New-Item -ItemType Directory -Path $wipItem -Force | Out-Null

$previewSources = [System.Collections.Generic.List[string]]::new()
foreach ($facility in $facilities) {
    $sourceItem = Join-Path $sourceRoot $facility.SourceId
    $sourceWorkshopConfig = Join-Path $sourceItem 'workshopconfig.ini'
    if (-not (Test-Path -LiteralPath $sourceWorkshopConfig)) { throw "Missing source package: $sourceWorkshopConfig" }
    $objectMatch = Select-String -LiteralPath $sourceWorkshopConfig -Pattern '^\$OBJECT_BUILDING\s+(\S+)\s*$' | Select-Object -First 1
    if (-not $objectMatch) { throw "No building object is declared in $sourceWorkshopConfig" }
    $objectName = $objectMatch.Matches[0].Groups[1].Value
    $sourceObject = Join-Path $sourceItem $objectName
    if (-not (Test-Path -LiteralPath $sourceObject)) { throw "Building object folder is missing: $sourceObject" }

    $objectTarget = Join-Path $releaseRoot $objectName
    New-Item -ItemType Directory -Path $objectTarget -Force | Out-Null
    foreach ($required in @('building.ini', 'renderconfig.ini', 'model.nmf')) {
        $path = Join-Path $sourceObject $required
        if (-not (Test-Path -LiteralPath $path)) { throw "Required runtime file is missing: $path" }
        Copy-Item -LiteralPath $path -Destination $objectTarget -Force
    }

    $renderConfig = Get-Content -LiteralPath (Join-Path $sourceObject 'renderconfig.ini') -Raw
    $materialMatch = [regex]::Match($renderConfig, '(?im)^\s*MATERIAL\s+(\S+)\s*$')
    if (-not $materialMatch.Success) { throw "No MATERIAL reference in $(Join-Path $sourceObject 'renderconfig.ini')" }
    $materialName = $materialMatch.Groups[1].Value
    $materialSource = Join-Path $sourceObject $materialName
    if (-not (Test-Path -LiteralPath $materialSource)) { throw "Referenced material file is missing: $materialSource" }
    Copy-Item -LiteralPath $materialSource -Destination (Join-Path $objectTarget $materialName) -Force

    $materialText = Get-Content -LiteralPath $materialSource -Raw
    $textureMatches = [regex]::Matches($materialText, '(?im)^\s*\$TEXTURE(?:_MTL)?\s+\d+\s+(\S+)\s*$')
    foreach ($match in $textureMatches) {
        $textureName = $match.Groups[1].Value
        $textureSource = Join-Path $sourceObject $textureName
        if (Test-Path -LiteralPath $textureSource) {
            Copy-Item -LiteralPath $textureSource -Destination (Join-Path $objectTarget $textureName) -Force
        }
    }

    $iconSource = Join-Path $sourceObject 'imagegui.png'
    if (-not (Test-Path -LiteralPath $iconSource)) { throw "Building image is missing: $iconSource" }
    $iconTarget = Join-Path $objectTarget 'imagegui.png'
    Copy-ScaledPng $iconSource $iconTarget 512
    $previewSources.Add($iconTarget)
    $itemLines.Add("`$OBJECT_BUILDING $objectName")
}

foreach ($destination in @($releaseRoot, $wipItem)) {
    $config = Join-Path $destination 'workshopconfig.ini'
    $configLines = @($itemLines.ToArray()) + '$END'
    [IO.File]::WriteAllLines($config, [string[]]$configLines, [Text.UTF8Encoding]::new($false))
    $plugins = Join-Path $destination 'plugins'
    New-Item -ItemType Directory -Path $plugins -Force | Out-Null
    Copy-Item -LiteralPath $pluginSource -Destination (Join-Path $plugins 'EarthworksPlanning.dll') -Force
    Copy-Item -LiteralPath $rmlManifestSource -Destination (Join-Path $plugins 'EarthworksPlanning.rml.json') -Force
    Copy-Item -LiteralPath $tesmioInstallerSource -Destination (Join-Path $destination 'Install-TesmioPlugin.ps1') -Force
    Copy-Item -LiteralPath $tesmioReadmeSource -Destination (Join-Path $destination 'README-Tesmio.md') -Force
    New-PackagePreview $previewSources.ToArray() (Join-Path $destination 'previewimage.png')
}

# Keep the release package and the local game WIP item identical, without
# deleting anything from either location.
foreach ($file in Get-ChildItem -LiteralPath $releaseRoot -Recurse -File) {
    $relative = $file.FullName.Substring($releaseRoot.Length + 1)
    $target = Join-Path $wipItem $relative
    $parent = Split-Path $target -Parent
    New-Item -ItemType Directory -Path $parent -Force | Out-Null
    Copy-Item -LiteralPath $file.FullName -Destination $target -Force
}

$releasePreviewBytes = (Get-Item -LiteralPath (Join-Path $releaseRoot 'previewimage.png')).Length
if ($releasePreviewBytes -gt 1MB) { throw "Combined Workshop preview exceeds 1 MB: $releasePreviewBytes bytes" }

Write-Host "Combined local mod package created: $releaseRoot"
Write-Host "Single local game WIP item staged: $wipItem"
Write-Host 'Includes three buildings, the Tesmio plugin installer, and the RML fallback manifest; Workshop upload was not performed.'
