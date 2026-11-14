[CmdletBinding()]
param()

$ErrorActionPreference = 'Stop'
Set-StrictMode -Version Latest

$repo = Split-Path $PSScriptRoot -Parent
$version = (Get-Content -LiteralPath (Join-Path $repo 'VERSION') -Raw).Trim()
$releaseParent = Join-Path $repo 'build\release'
$packageRoot = Join-Path $releaseParent "EarthworksPlanning-$version"
$archivePath = Join-Path $releaseParent "EarthworksPlanning-$version.zip"

foreach ($output in @($packageRoot, $archivePath)) {
    if (Test-Path -LiteralPath $output) {
        throw "Release output already exists. Move or remove it before rebuilding this version: $output"
    }
}

& (Join-Path $PSScriptRoot 'Build.ps1')
if (-not $?) { throw 'The player plugin build failed.' }

foreach ($scriptName in @('Stage-EarthworksBuilding.ps1', 'Stage-EarthworksBuilding-Medium.ps1', 'Stage-EarthworksBuilding-Large.ps1')) {
    & (Join-Path $PSScriptRoot $scriptName) -PackageOnly
    if (-not $?) { throw "Release package source preparation failed: $scriptName" }
}

& (Join-Path $PSScriptRoot 'Build-CombinedModPackage.ps1') -Release -ItemId '0000000000'
if (-not $?) { throw 'Release package assembly failed.' }

Compress-Archive -Path (Join-Path $packageRoot '*') -DestinationPath $archivePath -CompressionLevel Optimal
Write-Host "Release candidate folder: $packageRoot"
Write-Host "Release candidate archive: $archivePath"
Write-Host 'This script does not upload the package or modify the local game WIP folder.'
