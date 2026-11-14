[CmdletBinding()]
param(
    [string]$GameRoot = '',
    [string]$ItemId = '9100000015',
    [switch]$Release
)

$ErrorActionPreference = 'Stop'
Set-StrictMode -Version Latest

$repo = Split-Path $PSScriptRoot -Parent
if ([string]::IsNullOrWhiteSpace($GameRoot)) { $GameRoot = Split-Path $repo -Parent }
$version = (Get-Content -LiteralPath (Join-Path $repo 'VERSION') -Raw).Trim()
$sourceRoot = Join-Path $repo 'build\earthworks-workshop'
$packageRoot = if ($Release) {
    Join-Path $repo ("build\release\EarthworksPlanning-$version")
} else {
    Join-Path $repo 'build\playtest\EarthworksPlanning'
}
$wipRoot = Join-Path $GameRoot 'media_soviet\workshop_wip'
$wipItem = Join-Path $wipRoot $ItemId
$pluginSource = Join-Path $repo 'build\windows-x64\bin\EarthworksPlanning.dll'
$rmlManifestSource = Join-Path $repo 'assets\rml\EarthworksPlanning.rml.json'
$tesmioInstallerSource = Join-Path $repo 'scripts\Install-TesmioPlugin.ps1'
$playerReadmeSource = Join-Path $repo 'docs\PLAYER_INSTALLATION.md'

$facilities = @(
    [pscustomobject]@{ SourceId = '9100000012'; Name = 'Small' },
    [pscustomobject]@{ SourceId = '9100000013'; Name = 'Medium' },
    [pscustomobject]@{ SourceId = '9100000014'; Name = 'Large' }
)

if ($ItemId -notmatch '^\d{10}$') { throw 'ItemId must contain exactly 10 digits.' }
if (-not (Test-Path -LiteralPath $pluginSource)) { throw 'Build the player-facing plugin before packaging.' }
if (-not (Test-Path -LiteralPath $rmlManifestSource)) { throw "RML manifest template is missing: $rmlManifestSource" }
if (-not (Test-Path -LiteralPath $tesmioInstallerSource)) { throw "Tesmio installer is missing: $tesmioInstallerSource" }
if (-not (Test-Path -LiteralPath $playerReadmeSource)) { throw "Player installation instructions are missing: $playerReadmeSource" }
if (-not (Test-Path -LiteralPath $sourceRoot)) { throw "Facility package sources are missing: $sourceRoot" }

if ($Release -and (Test-Path -LiteralPath $packageRoot)) {
    throw "Release output already exists. Move or remove it before building a fresh package: $packageRoot"
}

if (-not $Release -and (Test-Path -LiteralPath $wipItem)) {
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

if (-not $Release) {
    $playtestParent = [IO.Path]::GetFullPath((Join-Path $repo 'build\playtest'))
    $packageFullPath = [IO.Path]::GetFullPath($packageRoot)
    if (-not $packageFullPath.StartsWith($playtestParent + [IO.Path]::DirectorySeparatorChar, [StringComparison]::OrdinalIgnoreCase)) {
        throw "Refusing to clean playtest output outside the generated build folder: $packageFullPath"
    }
    if (Test-Path -LiteralPath $packageRoot) {
        $resolvedPackage = [IO.Path]::GetFullPath((Resolve-Path -LiteralPath $packageRoot).Path)
        if (-not $resolvedPackage.StartsWith($playtestParent + [IO.Path]::DirectorySeparatorChar, [StringComparison]::OrdinalIgnoreCase)) {
            throw "Resolved playtest output is outside the generated build folder: $resolvedPackage"
        }
        Remove-Item -LiteralPath $resolvedPackage -Recurse -Force
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
$packageTitle = if ($Release) { "Earthworks Planning (Candidate $version)" } else { 'Earthworks Planning (Playtest)' }
$packageDescription = if ($Release) {
    "Candidate $version. Includes the Earthworks plugin and Small, Medium, and Large facilities. TesmioLoader is primary; RML is an optional fallback."
} else {
    'Local playtest package. Includes Earthworks Planning and the Small, Medium, and Large Earthworks facilities. TesmioLoader is primary; RML is an optional fallback.'
}
$itemLines.Add("`$ITEM_NAME `"$packageTitle`"")
$itemLines.Add("`$ITEM_DESC `"$packageDescription`"")

New-Item -ItemType Directory -Path $packageRoot -Force | Out-Null
if (-not $Release) { New-Item -ItemType Directory -Path $wipItem -Force | Out-Null }

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

    $objectTarget = Join-Path $packageRoot $objectName
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

foreach ($destination in @($packageRoot)) {
    $config = Join-Path $destination 'workshopconfig.ini'
    $configLines = @($itemLines.ToArray()) + '$END'
    [IO.File]::WriteAllLines($config, [string[]]$configLines, [Text.UTF8Encoding]::new($false))
    $plugins = Join-Path $destination 'plugins'
    New-Item -ItemType Directory -Path $plugins -Force | Out-Null
    Copy-Item -LiteralPath $pluginSource -Destination (Join-Path $plugins 'EarthworksPlanning.dll') -Force
    $manifest = Get-Content -LiteralPath $rmlManifestSource -Raw | ConvertFrom-Json
    $manifest.version = $version
    $manifestText = $manifest | ConvertTo-Json -Depth 16
    [IO.File]::WriteAllText((Join-Path $plugins 'EarthworksPlanning.rml.json'), $manifestText, [Text.UTF8Encoding]::new($false))
    Copy-Item -LiteralPath $tesmioInstallerSource -Destination (Join-Path $destination 'Install-TesmioPlugin.ps1') -Force
    Copy-Item -LiteralPath $playerReadmeSource -Destination (Join-Path $destination 'README.md') -Force
    if ($Release) {
        foreach ($document in @('CHANGELOG.md', 'COMPATIBILITY.md', 'KNOWN_ISSUES.md')) {
            Copy-Item -LiteralPath (Join-Path $repo $document) -Destination (Join-Path $destination $document) -Force
        }
    }
    New-PackagePreview $previewSources.ToArray() (Join-Path $destination 'previewimage.png')
}

if ($Release) {
    $sourceOutput = Join-Path $packageRoot 'SOURCE'
    New-Item -ItemType Directory -Path $sourceOutput -Force | Out-Null
    foreach ($relativeDirectory in @('src', 'assets\building', 'assets\rml', 'scripts', 'tests', 'LICENSES')) {
        $sourceDirectory = Join-Path $repo $relativeDirectory
        if (-not (Test-Path -LiteralPath $sourceDirectory)) { continue }
        $destinationDirectory = Join-Path $sourceOutput $relativeDirectory
        New-Item -ItemType Directory -Path $destinationDirectory -Force | Out-Null
        Get-ChildItem -LiteralPath $sourceDirectory -Recurse -File | Where-Object { $_.Extension -ne '.blend1' } | ForEach-Object {
            $relativeFile = $_.FullName.Substring($sourceDirectory.Length + 1)
            $targetFile = Join-Path $destinationDirectory $relativeFile
            New-Item -ItemType Directory -Path (Split-Path $targetFile -Parent) -Force | Out-Null
            Copy-Item -LiteralPath $_.FullName -Destination $targetFile -Force
        }
    }
    foreach ($relativeFile in @('CMakeLists.txt', 'CMakePresets.json', 'VERSION', 'dependencies.lock.json', '.vsconfig', 'README.md', 'THIRD_PARTY_NOTICES.md')) {
        $sourceFile = Join-Path $repo $relativeFile
        if (Test-Path -LiteralPath $sourceFile) { Copy-Item -LiteralPath $sourceFile -Destination (Join-Path $sourceOutput $relativeFile) -Force }
    }
    Copy-Item -LiteralPath (Join-Path $repo 'THIRD_PARTY_NOTICES.md') -Destination (Join-Path $packageRoot 'THIRD_PARTY_NOTICES.md') -Force
    Copy-Item -LiteralPath (Join-Path $repo 'LICENSES') -Destination $packageRoot -Recurse -Force

    $fileRecords = @(Get-ChildItem -LiteralPath $packageRoot -Recurse -File | Sort-Object FullName | ForEach-Object {
        [pscustomobject]@{
            path = $_.FullName.Substring($packageRoot.Length + 1).Replace('\', '/')
            sha256 = (Get-FileHash -LiteralPath $_.FullName -Algorithm SHA256).Hash.ToLowerInvariant()
        }
    })
    $packageManifest = [pscustomobject]@{
        modVersion = $version
        gameVersion = '1.1.1.13'
        steamBuild = '24922914'
        primaryLoader = 'TesmioLoader API 4'
        fallbackLoader = 'Republic Mod Loader'
        files = $fileRecords
    }
    [IO.File]::WriteAllText((Join-Path $packageRoot 'package-manifest.json'), ($packageManifest | ConvertTo-Json -Depth 8), [Text.UTF8Encoding]::new($false))
} else {
    # Refresh only files generated by this playtest package. The WIP folder is
    # retained as a developer item and is not a release artifact.
    foreach ($file in Get-ChildItem -LiteralPath $packageRoot -Recurse -File) {
        $relative = $file.FullName.Substring($packageRoot.Length + 1)
        $target = Join-Path $wipItem $relative
        $parent = Split-Path $target -Parent
        New-Item -ItemType Directory -Path $parent -Force | Out-Null
        Copy-Item -LiteralPath $file.FullName -Destination $target -Force
    }
}

$previewBytes = (Get-Item -LiteralPath (Join-Path $packageRoot 'previewimage.png')).Length
if ($previewBytes -gt 1MB) { throw "Combined Workshop preview exceeds 1 MB: $previewBytes bytes" }

if ($Release) {
    Write-Host "Clean release candidate package created: $packageRoot"
    Write-Host 'Includes runtime files, source, license texts, notices, and package checksums. No local game WIP item was changed.'
    Write-Host 'Workshop upload was not performed.'
} else {
    Write-Host "Clean playtest package created: $packageRoot"
    Write-Host "Single local game WIP item refreshed: $wipItem"
    Write-Host 'Includes the three buildings, the Tesmio plugin installer, and the RML fallback manifest. Workshop upload was not performed.'
}
