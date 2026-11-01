Set-StrictMode -Version Latest

function Initialize-EarthworksBuildEnvironment {
    $vswhere = Join-Path ${env:ProgramFiles(x86)} 'Microsoft Visual Studio\Installer\vswhere.exe'
    if (-not (Test-Path -LiteralPath $vswhere)) {
        throw 'Visual Studio Installer is missing. Install Visual Studio 2022 with the components in .vsconfig.'
    }
    $vsPath = & $vswhere -latest -version '[17.0,18.0)' -products '*' -requires Microsoft.VisualStudio.Component.VC.Tools.x86.x64 -property installationPath
    if ($LASTEXITCODE -ne 0 -or -not $vsPath) {
        throw 'The Visual Studio 2022 x64 C++ toolchain is missing. Import .vsconfig in Visual Studio Installer.'
    }
    $vsPath = ([string]$vsPath).Trim()
    $devShell = Join-Path $vsPath 'Common7\Tools\Microsoft.VisualStudio.DevShell.dll'
    Import-Module -Name $devShell -ErrorAction Stop
    Enter-VsDevShell -VsInstallPath $vsPath -SkipAutomaticLocation -DevCmdArguments '-arch=x64 -host_arch=x64' | Out-Null

    $cmake = Join-Path $vsPath 'Common7\IDE\CommonExtensions\Microsoft\CMake\CMake\bin\cmake.exe'
    $ninja = Join-Path $vsPath 'Common7\IDE\CommonExtensions\Microsoft\CMake\Ninja\ninja.exe'
    foreach ($tool in @($cmake, $ninja)) {
        if (-not (Test-Path -LiteralPath $tool)) {
            throw "Required CMake tool missing: $tool. Import .vsconfig in Visual Studio Installer."
        }
    }
    $env:PATH = (Split-Path $cmake) + ';' + (Split-Path $ninja) + ';' + $env:PATH
    foreach ($tool in @('cl.exe', 'link.exe', 'rc.exe', 'dumpbin.exe')) {
        if (-not (Get-Command $tool -ErrorAction SilentlyContinue)) {
            throw "Required compiler/SDK tool missing from the development environment: $tool"
        }
    }
    return [pscustomobject]@{VisualStudio=$vsPath;CMake=$cmake;Ninja=$ninja}
}
