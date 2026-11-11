[CmdletBinding()]
param()

$ErrorActionPreference = 'Stop'
$root = Split-Path $PSScriptRoot -Parent
$game = Split-Path $root -Parent
$launcher = Join-Path $root 'tools\runtime\rml\RepublicModLoader.exe'
$running = @(Get-Process RepublicModLoader -ErrorAction SilentlyContinue | Where-Object { $_.Path -eq $launcher })
if ($running.Count -gt 0) {
    throw 'The project-local RML launcher is already open. Close it before staging the plugin and Earthworks facility; then rerun launch-rml-fallback.cmd.'
}
if (-not (Get-Process steam -ErrorAction SilentlyContinue)) { throw 'Start Steam before launching the diagnostic game session.' }
if (Get-Process SOVIET64,SOVIET -ErrorAction SilentlyContinue) { throw 'Close the running game before launching a diagnostic session.' }
Write-Host 'Staging the plugin and dedicated Earthworks facility for the Earthworks Development profile...'
& (Join-Path $PSScriptRoot 'Stage-Diagnostics.ps1') -DeveloperUI
if (-not $?) { throw 'Prototype staging failed; RML was not launched.' }
Start-Process -FilePath $launcher -WorkingDirectory (Split-Path $launcher -Parent) -WindowStyle Normal
