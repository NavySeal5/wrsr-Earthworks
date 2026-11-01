[CmdletBinding()]
param()

$ErrorActionPreference = 'Stop'
Set-StrictMode -Version Latest
$root = Split-Path $PSScriptRoot -Parent
$game = Split-Path $root -Parent
$baseline = Get-Content -LiteralPath (Join-Path $root 'docs\game-baseline.json') -Raw | ConvertFrom-Json
$lock = (Get-Content -LiteralPath (Join-Path $root 'dependencies.lock.json') -Raw | ConvertFrom-Json).rml
$runtime = Join-Path $root 'tools\runtime\rml'
$launcher = Join-Path $runtime 'RepublicModLoader.exe'
$running = @(Get-Process RepublicModLoader -ErrorAction SilentlyContinue | Where-Object { $_.Path -eq $launcher })
if ($running.Count -gt 0) {
    throw 'Close the project-local RML development window before restaging its plugin or configuration.'
}

foreach ($binary in $baseline.binaries) {
    $path = Join-Path $game $binary.name
    if ((Get-FileHash -LiteralPath $path -Algorithm SHA256).Hash.ToLowerInvariant() -ne $binary.sha256) {
        throw "Game binary differs from the recorded baseline: $path. Review compatibility first."
    }
}
foreach ($file in $lock.runtime_files.PSObject.Properties) {
    $path = Join-Path $runtime $file.Name
    if ((Get-FileHash -LiteralPath $path -Algorithm SHA256).Hash.ToLowerInvariant() -ne $file.Value) {
        throw "Runtime dependency mismatch: $path"
    }
}
$dll = Join-Path $root 'build\windows-x64\bin\EarthworksPlanning.dll'
if (-not (Test-Path -LiteralPath $dll)) { throw 'Build the plugin before staging it.' }
$plugins = Join-Path $runtime 'rml\plugins'
New-Item -ItemType Directory -Path $plugins -Force | Out-Null
$unexpectedPlugins = @(Get-ChildItem -LiteralPath $plugins -Filter '*.dll' | Where-Object { $_.Name -ne 'EarthworksPlanning.dll' })
if ($unexpectedPlugins.Count -gt 0) { throw 'Unexpected plugins in the isolated diagnostic runtime. Review them before proceeding.' }
Copy-Item -LiteralPath $dll -Destination (Join-Path $plugins 'EarthworksPlanning.dll') -Force
$symbols = Join-Path $root 'build\windows-x64\bin\EarthworksPlanning.pdb'
if (Test-Path -LiteralPath $symbols) {
    Copy-Item -LiteralPath $symbols -Destination (Join-Path $plugins 'EarthworksPlanning.pdb') -Force
}

Copy-Item -LiteralPath (Join-Path $root 'assets\rml\EarthworksPlanning.rml.json') -Destination (Join-Path $plugins 'EarthworksPlanning.rml.json') -Force
$buildingStage = Join-Path $PSScriptRoot 'Stage-EarthworksBuilding.ps1'
& $buildingStage
if (-not $?) { throw 'Dedicated Earthworks building content staging failed.' }
$railComparisonStage = Join-Path $PSScriptRoot 'Stage-RailOfficeComparison.ps1'
& $railComparisonStage
if (-not $?) { throw 'Rail-office comparison content staging failed.' }
$config = Get-Content -LiteralPath (Join-Path $root 'assets\rml\diagnostic-config.json') -Raw | ConvertFrom-Json
$config.gamePath = Join-Path $game 'SOVIET64.exe'
[IO.File]::WriteAllText((Join-Path $runtime 'rml\config.json'), ($config | ConvertTo-Json -Depth 12), [Text.UTF8Encoding]::new($false))
Write-Host "RML development runtime ready: $runtime"
Write-Host 'Earthworks Development selects the RML plugin; the dedicated Earthworks item and separate rail-office comparison are staged locally.'
Write-Host 'The diagnostic preset is refreshed by staging. Your subscribed RML configuration is untouched. The game has not been launched.'
