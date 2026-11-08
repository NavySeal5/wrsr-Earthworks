[CmdletBinding()]
param([switch]$DeveloperUI)

$ErrorActionPreference = 'Stop'
Set-StrictMode -Version Latest
$root = Split-Path $PSScriptRoot -Parent
. (Join-Path $PSScriptRoot 'Build-Environment.ps1')
$tools = Initialize-EarthworksBuildEnvironment
Push-Location $root
try {
    $preset = if ($DeveloperUI) { 'windows-x64-developer-ui' } else { 'windows-x64' }
    & $tools.CMake --preset $preset
    if ($LASTEXITCODE -ne 0) { throw 'CMake configuration failed.' }
    & $tools.CMake --build --preset $preset
    if ($LASTEXITCODE -ne 0) { throw 'Plugin build failed.' }
    $dll = Join-Path $root ("build\{0}\bin\EarthworksPlanning.dll" -f $preset)
    $details = (& dumpbin.exe /nologo /headers /exports $dll | Out-String)
    if ($LASTEXITCODE -ne 0) { throw 'DLL inspection failed.' }
    if ($details -notmatch '(?im)^\s*8664 machine') { throw 'Plugin is not an x64 DLL.' }
    foreach ($entry in @('TsmPluginApiVersion', 'TsmPluginInit', 'TsmPluginStart')) {
        if ($details -notmatch ('\b' + $entry + '\b')) { throw "Plugin export missing: $entry" }
    }
    Write-Host "Verified x64 DLL and all three Tesmio entry points: $dll"
} finally {
    Pop-Location
}
