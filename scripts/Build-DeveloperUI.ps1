[CmdletBinding()]
param()

$ErrorActionPreference = 'Stop'
Set-StrictMode -Version Latest
$root = Split-Path $PSScriptRoot -Parent
. (Join-Path $PSScriptRoot 'Build-Environment.ps1')
$tools = Initialize-EarthworksBuildEnvironment
Push-Location $root
try {
    & $tools.CMake --preset windows-x64-developer-ui
    if ($LASTEXITCODE -ne 0) { throw 'Developer UI CMake configuration failed.' }
    & $tools.CMake --build --preset windows-x64-developer-ui
    if ($LASTEXITCODE -ne 0) { throw 'Developer UI plugin build failed.' }
    $dll = Join-Path $root 'build\windows-x64-developer-ui\bin\EarthworksPlanning.dll'
    $details = (& dumpbin.exe /nologo /headers /exports $dll | Out-String)
    if ($LASTEXITCODE -ne 0) { throw 'Developer UI DLL inspection failed.' }
    if ($details -notmatch '(?im)^\s*8664 machine') { throw 'Developer UI plugin is not an x64 DLL.' }
    foreach ($entry in @('TsmPluginApiVersion', 'TsmPluginInit', 'TsmPluginStart')) {
        if ($details -notmatch ('\b' + $entry + '\b')) { throw "Developer UI plugin export missing: $entry" }
    }
    Write-Host "Verified x64 developer UI DLL and Tesmio exports: $dll"
} finally {
    Pop-Location
}
