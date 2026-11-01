[CmdletBinding()]
param()

$ErrorActionPreference = 'Stop'
$root = Split-Path $PSScriptRoot -Parent
$game = Split-Path $root -Parent
$launcher = Join-Path $root 'tools\runtime\rml\RepublicModLoader.exe'
$running = @(Get-Process RepublicModLoader -ErrorAction SilentlyContinue | Where-Object { $_.Path -eq $launcher })
if ($running.Count -gt 0) {
    Write-Host 'The RML development launcher is already open. Use that window, or close it before restaging a new build.'
    return
}
if (-not (Get-Process steam -ErrorAction SilentlyContinue)) { throw 'Start Steam before launching the diagnostic game session.' }
if (Get-Process SOVIET64,SOVIET -ErrorAction SilentlyContinue) { throw 'Close the running game before launching a diagnostic session.' }
& (Join-Path $PSScriptRoot 'Stage-Diagnostics.ps1')
Start-Process -FilePath $launcher -WorkingDirectory (Split-Path $launcher -Parent) -WindowStyle Normal
