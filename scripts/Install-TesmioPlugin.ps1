[CmdletBinding()]
param(
    [Parameter(Mandatory = $true)]
    [ValidateNotNullOrEmpty()]
    [string]$GameRoot,
    [switch]$Force
)

$ErrorActionPreference = 'Stop'
Set-StrictMode -Version Latest
$gameRootResolved = [IO.Path]::GetFullPath($GameRoot)
$loader = Join-Path $gameRootResolved 'tesmioloader.dll'
$source = Join-Path $PSScriptRoot 'plugins\EarthworksPlanning.dll'
$pluginDir = Join-Path $gameRootResolved 'plugins'
$target = Join-Path $pluginDir 'EarthworksPlanning.dll'

if (-not (Test-Path -LiteralPath $loader -PathType Leaf)) {
    throw "TesmioLoader is not installed at the selected game root: $gameRootResolved"
}
if (-not (Test-Path -LiteralPath $source -PathType Leaf)) {
    throw "Earthworks plugin DLL is missing beside this installer: $source"
}
if (Test-Path -LiteralPath $target -PathType Leaf) {
    $sourceHash = (Get-FileHash -LiteralPath $source -Algorithm SHA256).Hash
    $targetHash = (Get-FileHash -LiteralPath $target -Algorithm SHA256).Hash
    if ($sourceHash -eq $targetHash) {
        Write-Host 'The same Earthworks plugin build is already installed.'
        exit 0
    }
    if (-not $Force) {
        throw 'A different EarthworksPlanning.dll is already installed. Back it up, then rerun with -Force to replace it.'
    }
    $backup = "$target.$(Get-Date -Format 'yyyyMMdd-HHmmss').bak"
    Copy-Item -LiteralPath $target -Destination $backup
    Write-Host "Existing plugin backed up to: $backup"
}

New-Item -ItemType Directory -Path $pluginDir -Force | Out-Null
Copy-Item -LiteralPath $source -Destination $target -Force
Write-Host "Installed EarthworksPlanning.dll into TesmioLoader's plugin folder: $pluginDir"
Write-Host 'Restart TesmioLoader to load the plugin. The loader and Earthworks must support the installed game version.'
