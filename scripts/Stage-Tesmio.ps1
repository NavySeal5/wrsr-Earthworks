[CmdletBinding()]
param([switch]$DeveloperUI)

$ErrorActionPreference = 'Stop'
Set-StrictMode -Version Latest
$root = Split-Path $PSScriptRoot -Parent
$game = Split-Path $root -Parent
$runtimeCandidates = @(
    (Join-Path $game 'tesmioloader\build'),
    (Join-Path $game 'tesmioloader'),
    $game
)
$runtime = $runtimeCandidates | Where-Object {
    (Test-Path -LiteralPath (Join-Path $_ 'tesmiolauncher.exe')) -and
    (Test-Path -LiteralPath (Join-Path $_ 'tesmioloader.dll')) -and
    (Test-Path -LiteralPath (Join-Path $_ 'tesmioloader.ini'))
} | Select-Object -First 1
if (-not $runtime) { throw 'TesmioLoader was not found in the game folder or its tesmioloader\build subfolder. Install TesmioLoader, then try again.' }
$launcher = Join-Path $runtime 'tesmiolauncher.exe'
$loader = Join-Path $runtime 'tesmioloader.dll'
$ini = Join-Path $runtime 'tesmioloader.ini'
$running = @(Get-Process tesmiolauncher -ErrorAction SilentlyContinue | Where-Object { $_.Path -eq $launcher })
if ($running.Count -gt 0) { throw 'Close the installed TesmioLoader window before restaging its plugin or configuration.' }
foreach ($required in @($launcher, $loader, $ini, (Join-Path $game 'SOVIET64.exe'))) {
    if (-not (Test-Path -LiteralPath $required)) { throw "Required Tesmio or game file is missing: $required" }
}

$gameExe = Join-Path $game 'SOVIET64.exe'

$preset = if ($DeveloperUI) { 'windows-x64-developer-ui' } else { 'windows-x64' }
$dll = Join-Path $root ("build\{0}\bin\EarthworksPlanning.dll" -f $preset)
if (-not (Test-Path -LiteralPath $dll)) { throw "Build the Tesmio API 4 plugin first: $dll" }
$pluginDir = Join-Path $runtime 'plugins'
New-Item -ItemType Directory -Path $pluginDir -Force | Out-Null
Copy-Item -LiteralPath $dll -Destination (Join-Path $pluginDir 'EarthworksPlanning.dll') -Force
$pdb = Join-Path $root 'build\windows-x64\bin\EarthworksPlanning.pdb'
if (Test-Path -LiteralPath $pdb) { Copy-Item -LiteralPath $pdb -Destination (Join-Path $pluginDir 'EarthworksPlanning.pdb') -Force }

$config = Get-Content -LiteralPath $ini -Raw
if ($config -notmatch '(?m)^game_exe=') { throw "Tesmio configuration has no game_exe entry: $ini" }
$config = [regex]::Replace($config, '(?m)^game_exe=.*$', [System.Text.RegularExpressions.MatchEvaluator]{ param($match) 'game_exe=' + $gameExe })
$configLines = [System.Collections.Generic.List[string]]::new()
foreach ($line in ($config -split "`r?`n")) { $configLines.Add($line) }
$pluginHeader = -1
for ($index = 0; $index -lt $configLines.Count; $index++) {
    if ($configLines[$index] -match '^\[plugins\]\s*$') { $pluginHeader = $index; break }
}
if ($pluginHeader -lt 0) {
    while ($configLines.Count -gt 0 -and [string]::IsNullOrWhiteSpace($configLines[$configLines.Count - 1])) {
        $configLines.RemoveAt($configLines.Count - 1)
    }
    $configLines.Add('')
    $configLines.Add('[plugins]')
    $pluginHeader = $configLines.Count - 1
}
$pluginEnd = $pluginHeader + 1
while ($pluginEnd -lt $configLines.Count -and $configLines[$pluginEnd] -notmatch '^\[[^\]]+\]\s*$') { $pluginEnd++ }
$pluginEntry = -1
for ($index = $pluginHeader + 1; $index -lt $pluginEnd; $index++) {
    if ($configLines[$index] -match '^\s*EarthworksPlanning\s*=') { $pluginEntry = $index; break }
}
if ($pluginEntry -ge 0) { $configLines[$pluginEntry] = 'EarthworksPlanning=1' }
else { $configLines.Insert($pluginHeader + 1, 'EarthworksPlanning=1') }
$config = [string]::Join("`r`n", $configLines)
[IO.File]::WriteAllText($ini, $config, [Text.UTF8Encoding]::new($false))

# Refresh the three generated facility sources, then package them as the one
# player-facing Earthworks Workshop item. The individual WIP item IDs are
# build inputs only and must not remain visible as duplicate mods in-game.
& (Join-Path $PSScriptRoot 'Stage-EarthworksBuilding.ps1')
if (-not $?) { throw 'Small Earthworks building source staging failed.' }
foreach ($size in @('Medium', 'Large')) {
    $lower = $size.ToLowerInvariant()
    $model = Join-Path $root "assets\building\$lower\native\earthworks_facility_$lower.nmf"
    if (-not (Test-Path -LiteralPath $model)) { throw "$size Earthworks model is missing: $model" }
    & (Join-Path $PSScriptRoot "Stage-EarthworksBuilding-$size.ps1")
    if (-not $?) { throw "$size Earthworks building source staging failed." }
}

$packageBuilder = Join-Path $PSScriptRoot 'Build-CombinedModPackage.ps1'
if (-not (Test-Path -LiteralPath $packageBuilder)) { throw "Combined Earthworks package builder is missing: $packageBuilder" }
& $packageBuilder -GameRoot $game
if (-not $?) { throw 'Combined Earthworks Workshop package staging failed.' }

# Delete only the three known, generated individual WIP packages. Require the
# marker and matching item ID so unrelated Workshop content is never removed.
$wipRoot = Join-Path $game 'media_soviet\workshop_wip'
foreach ($itemId in @('9100000012', '9100000013', '9100000014')) {
    $itemPath = Join-Path $wipRoot $itemId
    if (-not (Test-Path -LiteralPath $itemPath)) { continue }
    $markerPath = Join-Path $itemPath '.earthworks-generated'
    $configPath = Join-Path $itemPath 'workshopconfig.ini'
    if (-not (Test-Path -LiteralPath $markerPath) -or -not (Test-Path -LiteralPath $configPath)) {
        throw "Refusing to remove unmarked or unrecognized WIP item: $itemPath"
    }
    $itemConfig = Get-Content -LiteralPath $configPath -Raw
    if ($itemConfig -notmatch ('(?m)^\$ITEM_ID\s+' + [regex]::Escape($itemId) + '\s*$')) {
        throw "Refusing to remove WIP item with a mismatched item ID: $itemPath"
    }
    Remove-Item -LiteralPath $itemPath -Recurse -Force
    Write-Host "Removed duplicate generated WIP item: $itemId"
}

$hash = (Get-FileHash -LiteralPath (Join-Path $pluginDir 'EarthworksPlanning.dll') -Algorithm SHA256).Hash
Write-Host "Tesmio primary runtime staged at: $runtime"
Write-Host "Plugin: $($pluginDir)\EarthworksPlanning.dll"
Write-Host "Plugin SHA-256: $hash"
Write-Host 'One combined Earthworks Workshop item is staged; the three standalone facility duplicates are removed.'
Write-Host 'TesmioLoader is the active host. RML fallback files and configuration were not changed.'
Write-Host 'The game has not been launched.'
