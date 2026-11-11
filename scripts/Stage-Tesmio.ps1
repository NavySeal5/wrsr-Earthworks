[CmdletBinding()]
param([switch]$DeveloperUI)

$ErrorActionPreference = 'Stop'
Set-StrictMode -Version Latest
$root = Split-Path $PSScriptRoot -Parent
$game = Split-Path $root -Parent
$runtime = Join-Path $root 'tools\runtime\tesmioloader'
$launcher = Join-Path $runtime 'tesmiolauncher.exe'
$loader = Join-Path $runtime 'tesmioloader.dll'
$ini = Join-Path $runtime 'tesmioloader.ini'
$running = @(Get-Process tesmiolauncher -ErrorAction SilentlyContinue | Where-Object { $_.Path -eq $launcher })
if ($running.Count -gt 0) {
    throw 'Close the project-local TesmioLoader window before restaging its plugin or configuration.'
}
foreach ($required in @($launcher, $loader, $ini, (Join-Path $game 'SOVIET64.exe'))) {
    if (-not (Test-Path -LiteralPath $required)) { throw "Required Tesmio or game file is missing: $required" }
}

# Keep the native plugin safety check enabled. The current game may be newer
# than the version for which Earthworks hook addresses have been verified.
$gameExe = Join-Path $game 'SOVIET64.exe'
$reader = $null
$stream = [IO.File]::OpenRead($gameExe)
try {
    $reader = [IO.BinaryReader]::new($stream)
    if ($reader.ReadUInt16() -ne 0x5A4D) { throw 'The game executable has an invalid DOS header.' }
    $stream.Position = 0x3C
    $peOffset = $reader.ReadInt32()
    if ($peOffset -lt 0 -or $peOffset + 8 -gt $stream.Length) { throw 'The game executable has an invalid PE header offset.' }
    $stream.Position = $peOffset
    if ($reader.ReadUInt32() -ne 0x00004550) { throw 'The game executable has an invalid PE signature.' }
    $null = $reader.ReadUInt32() # machine + section count
    $actualTimestamp = $reader.ReadUInt32()
} finally {
    if ($reader) { $reader.Dispose() } else { $stream.Dispose() }
}
$supportedTimestamp = [Convert]::ToUInt32('6A3EB6AD', 16)
if ($actualTimestamp -ne $supportedTimestamp) {
    Write-Warning ('Earthworks currently supports PE timestamp 0x{0:X8}; this game is 0x{1:X8}. The plugin will decline initialization and install no hooks.' -f $supportedTimestamp, $actualTimestamp)
    Write-Warning 'Do not remove or bypass this guard. Hook addresses must be re-verified for the updated executable first.'
}

$preset = if ($DeveloperUI) { 'windows-x64-developer-ui' } else { 'windows-x64' }
$dll = Join-Path $root ("build\{0}\bin\EarthworksPlanning.dll" -f $preset)
if (-not (Test-Path -LiteralPath $dll)) { throw "Build the Tesmio API 4 plugin first: $dll" }
$pluginDir = Join-Path $runtime 'plugins'
New-Item -ItemType Directory -Path $pluginDir -Force | Out-Null
$unexpected = @(Get-ChildItem -LiteralPath $pluginDir -Filter '*.dll' | Where-Object Name -ne 'EarthworksPlanning.dll')
if ($unexpected.Count -gt 0) {
    throw ('Unexpected plugin DLLs found in the isolated Tesmio runtime: ' + (($unexpected | ForEach-Object Name) -join ', '))
}
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

& (Join-Path $PSScriptRoot 'Stage-EarthworksBuilding.ps1')
if (-not $?) { throw 'Small Earthworks building content staging failed.' }
foreach ($size in @('Medium', 'Large')) {
    $lower = $size.ToLowerInvariant()
    $model = Join-Path $root "assets\building\$lower\native\earthworks_facility_$lower.nmf"
    if (Test-Path -LiteralPath $model) {
        & (Join-Path $PSScriptRoot "Stage-EarthworksBuilding-$size.ps1")
        if (-not $?) { throw "$size Earthworks building content staging failed." }
    } else {
        Write-Host "$size Earthworks facility is unchanged; its existing staged package remains available."
    }
}

$hash = (Get-FileHash -LiteralPath (Join-Path $pluginDir 'EarthworksPlanning.dll') -Algorithm SHA256).Hash
Write-Host "Tesmio primary runtime staged at: $runtime"
Write-Host "Plugin: $($pluginDir)\EarthworksPlanning.dll"
Write-Host "Plugin SHA-256: $hash"
Write-Host 'TesmioLoader is the active host. RML fallback files and configuration were not changed.'
Write-Host 'The game has not been launched.'
