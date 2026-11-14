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

if (-not (Get-Process steam -ErrorAction SilentlyContinue)) { throw 'Start Steam before launching the diagnostic game session.' }
if (Get-Process SOVIET64,SOVIET -ErrorAction SilentlyContinue) { throw 'Close the running game before launching a diagnostic session.' }
$running = @(Get-Process tesmiolauncher -ErrorAction SilentlyContinue | Where-Object { $_.Path -eq $launcher })
if ($running.Count -gt 0) { throw 'The installed TesmioLoader is already open. Close it before restaging; then rerun launch-prototype.cmd.' }

Write-Host "Staging the plugin and dedicated Earthworks facilities for TesmioLoader at: $runtime"
$stageArgs = @{}
if ($DeveloperUI) { $stageArgs.DeveloperUI = $true }
& (Join-Path $PSScriptRoot 'Stage-Tesmio.ps1') @stageArgs
if (-not $?) { throw 'Tesmio staging failed; TesmioLoader was not launched.' }
Start-Process -FilePath $launcher -WorkingDirectory $runtime -WindowStyle Normal
