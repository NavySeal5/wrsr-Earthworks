param([string]$GameRoot = '')

$ErrorActionPreference = 'Stop'
$dll = Join-Path $PSScriptRoot 'plugins\EarthworksPlanning.dll'

if ([string]::IsNullOrWhiteSpace($GameRoot)) {
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
    $GameRoot = $libraries | Select-Object -Unique | ForEach-Object { Join-Path $_ 'steamapps\common\SovietRepublic' } |
        Where-Object { Test-Path (Join-Path $_ 'SOVIET64.exe') } | Select-Object -First 1
    if (-not $GameRoot) { $GameRoot = Read-Host 'Game folder not found automatically. Enter the folder containing SOVIET64.exe' }
}

$game = (Resolve-Path -LiteralPath $GameRoot).Path
$plugins = Join-Path $game 'plugins'
$ini = Join-Path $game 'tesmioloader.ini'

if (-not (Test-Path -LiteralPath $dll)) { throw "Earthworks plugin is missing beside the installer: $dll" }
if (-not (Test-Path (Join-Path $game 'tesmioloader.dll')) -or -not (Test-Path $ini)) {
    throw 'Install TesmioLoader in the game folder first.'
}
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
Write-Host 'Earthworks installed. Start the game through TesmioLoader.'
