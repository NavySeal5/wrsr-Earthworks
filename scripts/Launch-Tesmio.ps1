[CmdletBinding()]
param([switch]$DeveloperUI)

$ErrorActionPreference = 'Stop'
Set-StrictMode -Version Latest
$root = Split-Path $PSScriptRoot -Parent
$runtime = Join-Path $root 'tools\runtime\tesmioloader'
$launcher = Join-Path $runtime 'tesmiolauncher.exe'

if (-not (Get-Process steam -ErrorAction SilentlyContinue)) { throw 'Start Steam before launching the diagnostic game session.' }
if (Get-Process SOVIET64,SOVIET -ErrorAction SilentlyContinue) { throw 'Close the running game before launching a diagnostic session.' }
$running = @(Get-Process tesmiolauncher -ErrorAction SilentlyContinue | Where-Object { $_.Path -eq $launcher })
if ($running.Count -gt 0) { throw 'The project-local TesmioLoader is already open. Close it before restaging; then rerun launch-prototype.cmd.' }

Write-Host 'Staging the plugin and dedicated Earthworks facilities for the project-local TesmioLoader...'
$stageArgs = @{}
if ($DeveloperUI) { $stageArgs.DeveloperUI = $true }
& (Join-Path $PSScriptRoot 'Stage-Tesmio.ps1') @stageArgs
if (-not $?) { throw 'Tesmio staging failed; TesmioLoader was not launched.' }
Start-Process -FilePath $launcher -WorkingDirectory $runtime -WindowStyle Normal
