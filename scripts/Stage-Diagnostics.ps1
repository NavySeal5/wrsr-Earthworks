[CmdletBinding()]
param([switch]$DeveloperUI)

$ErrorActionPreference = 'Stop'
Set-StrictMode -Version Latest
$root = Split-Path $PSScriptRoot -Parent
$game = Split-Path $root -Parent
$baseline = Get-Content -LiteralPath (Join-Path $root 'docs\game-baseline.json') -Raw | ConvertFrom-Json
$lock = (Get-Content -LiteralPath (Join-Path $root 'dependencies.lock.json') -Raw | ConvertFrom-Json).rml
$runtime = Join-Path $root 'tools\runtime\rml'
$launcher = Join-Path $runtime 'RepublicModLoader.exe'
$running = @(Get-Process RepublicModLoader -ErrorAction SilentlyContinue | Where-Object { $_.Path -eq $launcher })
if ($running.Count -gt 0) {
    throw 'Close the project-local RML development window before restaging its plugin or configuration.'
}

foreach ($binary in $baseline.binaries) {
    $path = Join-Path $game $binary.name
    if ((Get-FileHash -LiteralPath $path -Algorithm SHA256).Hash.ToLowerInvariant() -ne $binary.sha256) {
        throw "Game binary differs from the recorded baseline: $path. Review compatibility first."
    }
}
foreach ($file in $lock.runtime_files.PSObject.Properties) {
    $path = Join-Path $runtime $file.Name
    if ((Get-FileHash -LiteralPath $path -Algorithm SHA256).Hash.ToLowerInvariant() -ne $file.Value) {
        throw "Runtime dependency mismatch: $path"
    }
}
$buildPreset = if ($DeveloperUI) { 'windows-x64-developer-ui' } else { 'windows-x64' }
$dll = Join-Path $root ("build\{0}\bin\EarthworksPlanning.dll" -f $buildPreset)
if (-not (Test-Path -LiteralPath $dll)) { throw 'Build the plugin before staging it.' }
$manifestPath = Join-Path $root 'assets\rml\EarthworksPlanning.rml.json'
$manifest = Get-Content -LiteralPath $manifestPath -Raw | ConvertFrom-Json
$dllHash = (Get-FileHash -LiteralPath $dll -Algorithm SHA256).Hash.ToLowerInvariant()
$safeVersion = $manifest.version -replace '[^A-Za-z0-9._-]', '_'
$stagedDllName = "EarthworksPlanning-$safeVersion-$($dllHash.Substring(0, 8)).dll"
$plugins = Join-Path $runtime 'rml\plugins'
New-Item -ItemType Directory -Path $plugins -Force | Out-Null
$unexpectedPlugins = @(Get-ChildItem -LiteralPath $plugins -Filter '*.dll' | Where-Object {
    $_.Name -ne 'EarthworksPlanning.dll' -and $_.Name -notlike 'EarthworksPlanning-*.dll'
})
if ($unexpectedPlugins.Count -gt 0) { throw 'Unexpected plugins in the isolated diagnostic runtime. Review them before proceeding.' }
$stagedDll = Join-Path $plugins $stagedDllName
if (-not (Test-Path -LiteralPath $stagedDll)) { Copy-Item -LiteralPath $dll -Destination $stagedDll }
$symbols = Join-Path $root 'build\windows-x64\bin\EarthworksPlanning.pdb'
if (Test-Path -LiteralPath $symbols) {
    Copy-Item -LiteralPath $symbols -Destination (Join-Path $plugins ([IO.Path]::ChangeExtension($stagedDllName, '.pdb')))
}

$manifest.dll = $stagedDllName
$stagedManifest = $manifest | ConvertTo-Json -Depth 16
[IO.File]::WriteAllText((Join-Path $plugins 'EarthworksPlanning.rml.json'), $stagedManifest, [Text.UTF8Encoding]::new($false))
$pluginVersion = ($manifest.version.ToLowerInvariant() -replace '[^a-z0-9]+', '.').Trim('.')
$pluginId = "tesmio.earthworksplanning.$pluginVersion.$($dllHash.Substring(0, 8))"
$buildingStage = Join-Path $PSScriptRoot 'Stage-EarthworksBuilding.ps1'
& $buildingStage
if (-not $?) { throw 'Dedicated Earthworks building content staging failed.' }
$mediumModel = Join-Path $root 'assets\building\medium\native\earthworks_facility_medium.nmf'
if (Test-Path -LiteralPath $mediumModel) {
    & (Join-Path $PSScriptRoot 'Stage-EarthworksBuilding-Medium.ps1')
    if (-not $?) { throw 'Medium Earthworks building content staging failed.' }
} else {
    Write-Host 'Medium Earthworks item is not staged because its ModelViewer NMF conversion is not ready yet.'
}
$largeModel = Join-Path $root 'assets\building\large\native\earthworks_facility_large.nmf'
if (Test-Path -LiteralPath $largeModel) {
    & (Join-Path $PSScriptRoot 'Stage-EarthworksBuilding-Large.ps1')
    if (-not $?) { throw 'Large Earthworks building content staging failed.' }
} else {
    Write-Host 'Large Earthworks item is not staged because its ModelViewer NMF conversion is not ready yet.'
}

# Earthworks facilities are intended to be buildable in early-start eras too.
# `modern_industry` is unlocked by the game's research.ini in 1958, so the
# generated building definitions must not carry that style flag. Check both
# explicit research gates and this indirect year gate after every restage.
foreach ($facilityId in @('9100000012', '9100000013', '9100000014')) {
    $facilityRoot = Join-Path (Split-Path $root -Parent) (Join-Path 'media_soviet\workshop_wip' $facilityId)
    if (-not (Test-Path -LiteralPath $facilityRoot)) { continue }
    $facilityConfigs = @(
        (Join-Path $facilityRoot 'workshopconfig.ini'),
        (Join-Path $facilityRoot (Join-Path (Get-ChildItem -LiteralPath $facilityRoot -Directory | Select-Object -First 1 -ExpandProperty Name) 'building.ini'))
    ) | Where-Object { Test-Path -LiteralPath $_ }
    foreach ($facilityConfig in $facilityConfigs) {
        $gates = Select-String -LiteralPath $facilityConfig -Pattern '^\s*\$(?:RESEARCH|UNLOCK|TECH_REQUIRED|REQUIRES_RESEARCH|RESEARCH_REQUIRED)\b|^\s*\$STYLE_FLAG\s+modern_industry\b' -CaseSensitive:$false
        if ($gates) {
            throw "Research/year unlock gates are not allowed in Earthworks facility configs: $facilityConfig"
        }
    }
}
$stagedFacilityCount = @('9100000012', '9100000013', '9100000014' | Where-Object {
    Test-Path -LiteralPath (Join-Path (Split-Path $root -Parent) (Join-Path 'media_soviet\workshop_wip' $_))
}).Count
Write-Host "Verified no research directives or modern_industry year gates in $stagedFacilityCount staged Earthworks facilities."

$config = Get-Content -LiteralPath (Join-Path $root 'assets\rml\diagnostic-config.json') -Raw | ConvertFrom-Json
$config.gamePath = Join-Path $game 'SOVIET64.exe'
$config.plugins.PSObject.Properties.Remove('earthworks.planning')
$config.plugins | Add-Member -MemberType NoteProperty -Name $pluginId -Value ([pscustomobject]@{
    enabled = $true
    force = $false
    priority = 100
}) -Force
$config.playlists[0].plugins = @([pscustomobject]@{
    id = $pluginId
    force = $false
    priority = 100
    order = 0
})
[IO.File]::WriteAllText((Join-Path $runtime 'rml\config.json'), ($config | ConvertTo-Json -Depth 12), [Text.UTF8Encoding]::new($false))
Write-Host "RML development runtime ready: $runtime"
Write-Host "Earthworks Development selects $pluginId and enables the Small, Medium, and Large Earthworks facilities (9100000012-9100000014)."
if ($DeveloperUI) { Write-Host 'The developer-only Workforce inspector is included in this staged build.' }
Write-Host 'The diagnostic preset is refreshed by staging. Your subscribed RML configuration is untouched. The game has not been launched.'
