[CmdletBinding()]
param()

$ErrorActionPreference = 'Stop'
Set-StrictMode -Version Latest

$repo = Split-Path $PSScriptRoot -Parent
$version = (Get-Content -LiteralPath (Join-Path $repo 'VERSION') -Raw).Trim()
$workshopConfigSource = Join-Path $repo 'assets\workshop\workshopconfig.ini'
if (-not (Test-Path -LiteralPath $workshopConfigSource)) { throw "Workshop item config is missing: $workshopConfigSource" }
$workshopConfig = Get-Content -LiteralPath $workshopConfigSource -Raw
$workshopIdMatch = [regex]::Match($workshopConfig, '(?m)^\$ITEM_ID\s+(\d{10})\s*$')
if (-not $workshopIdMatch.Success) { throw "Workshop item ID is missing or invalid in $workshopConfigSource" }
$workshopItemId = $workshopIdMatch.Groups[1].Value

& (Join-Path $PSScriptRoot 'Build.ps1')
if (-not $?) { throw 'The player plugin build failed.' }

foreach ($scriptName in @('Stage-EarthworksBuilding.ps1', 'Stage-EarthworksBuilding-Medium.ps1', 'Stage-EarthworksBuilding-Large.ps1')) {
    & (Join-Path $PSScriptRoot $scriptName) -PackageOnly
    if (-not $?) { throw "Release package source preparation failed: $scriptName" }
}

& (Join-Path $PSScriptRoot 'Build-CombinedModPackage.ps1') -Release -ItemId $workshopItemId
if (-not $?) { throw 'Release Workshop item staging failed.' }

Write-Host "Release candidate staged in Workshop item: $workshopItemId"
Write-Host "Workshop item ID: $workshopItemId"
Write-Host 'Review the Workshop item in game, then upload it through the in-game Workshop UI.'
Write-Host 'This script does not upload or publish the Workshop item.'
