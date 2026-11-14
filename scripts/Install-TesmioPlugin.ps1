param([string]$GameRoot = '')

$ErrorActionPreference = 'Stop'
$dllCandidates = @(
    (Join-Path $PSScriptRoot 'plugins\EarthworksPlanning.dll'),
    (Join-Path $PSScriptRoot '..\build\windows-x64\bin\EarthworksPlanning.dll')
)
$dll = $dllCandidates | Where-Object { Test-Path -LiteralPath $_ } | Select-Object -First 1

$searchRoot = $GameRoot
if ([string]::IsNullOrWhiteSpace($searchRoot)) {
    $libraries = @()
    try { $libraries += (Get-ItemPropertyValue 'HKCU:\Software\Valve\Steam' SteamPath).Replace('/', '\') } catch {}
    foreach ($library in @($libraries)) {
        $vdf = Join-Path $library 'steamapps\libraryfolders.vdf'
        if (Test-Path $vdf) {
            $text = Get-Content -LiteralPath $vdf -Raw
            foreach ($match in [regex]::Matches($text, '"path"\s+"([^"]+)"|"\d+"\s+"([^"]+)"')) {
                $path = if ($match.Groups[1].Success) { $match.Groups[1].Value } else { $match.Groups[2].Value }
                $libraries += $path.Replace('\\', '\')
            }
        }
    }
    $searchRoot = $libraries | Select-Object -Unique | ForEach-Object { Join-Path $_ 'steamapps\common\SovietRepublic' } |
        Where-Object { Test-Path (Join-Path $_ 'SOVIET64.exe') } | Select-Object -First 1
    if (-not $searchRoot) { $searchRoot = Read-Host 'Game folder not found automatically. Enter the game folder or TesmioLoader folder' }
}

if (-not $dll) { throw 'Earthworks DLL not found. Build it with scripts\Build.ps1 or use the packaged installer.' }
$searchRoot = (Resolve-Path -LiteralPath $searchRoot).Path
$loaderRoots = @(
    $searchRoot,
    (Join-Path $searchRoot 'tesmioloader'),
    (Join-Path $searchRoot 'tesmioloader\build'),
    (Join-Path $searchRoot 'build')
) | Select-Object -Unique
$loaderRoot = $loaderRoots | Where-Object {
    (Test-Path -LiteralPath (Join-Path $_ 'tesmioloader.dll')) -and
    (Test-Path -LiteralPath (Join-Path $_ 'tesmioloader.ini'))
} | Select-Object -First 1

if (-not $loaderRoot) {
    throw 'TesmioLoader files were not found. Checked this folder and its tesmioloader\build subfolder.'
}
$plugins = Join-Path $loaderRoot 'plugins'
$ini = Join-Path $loaderRoot 'tesmioloader.ini'

New-Item -ItemType Directory -Path $plugins -Force | Out-Null
Copy-Item -LiteralPath $dll -Destination (Join-Path $plugins 'EarthworksPlanning.dll') -Force

$lines = [Collections.Generic.List[string]]::new()
foreach ($line in Get-Content -LiteralPath $ini) { $lines.Add($line) }
$section = -1
for ($i=0; $i -lt $lines.Count; $i++) { if ($lines[$i] -match '^\[plugins\]\s*$') { $section=$i; break } }
if ($section -lt 0) { $lines.Add('[plugins]'); $section=$lines.Count-1 }
$end = $section+1
while ($end -lt $lines.Count -and $lines[$end] -notmatch '^\s*\[') { $end++ }
$entry = -1
for ($i=$section+1; $i -lt $end; $i++) { if ($lines[$i] -match '^\s*EarthworksPlanning\s*=') { $entry=$i; break } }
if ($entry -ge 0) { $lines[$entry]='EarthworksPlanning=1' } else { $lines.Insert($section+1,'EarthworksPlanning=1') }
[IO.File]::WriteAllLines($ini,$lines,[Text.UTF8Encoding]::new($false))
Write-Host "Earthworks installed in TesmioLoader folder: $loaderRoot"
Write-Host 'Start the game through TesmioLoader.'
