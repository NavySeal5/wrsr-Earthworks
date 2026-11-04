[CmdletBinding()]
param()

$ErrorActionPreference = 'Stop'
Set-StrictMode -Version Latest

$root = Split-Path $PSScriptRoot -Parent
$game = Split-Path $root -Parent
$media = Join-Path $game 'media_soviet'
$id = '9100000012'
$object = 'EarthworksDevelopmentFacility'
$name = 'Earthworks Facility (prototype)'
$source = Join-Path $media 'buildings_types'
$native = Join-Path $root 'assets\building\native'
$output = Join-Path $root 'build\earthworks-workshop'
$item = Join-Path $output $id
$objectDir = Join-Path $item $object
$targetRoot = Join-Path $media 'workshop_wip'
$target = Join-Path $targetRoot $id
$marker = '.earthworks-generated'

$baseline = Get-Content -LiteralPath (Join-Path $root 'docs\game-baseline.json') -Raw | ConvertFrom-Json
foreach ($binary in $baseline.binaries) {
    $path = Join-Path $game $binary.name
    if ((Get-FileHash -LiteralPath $path -Algorithm SHA256).Hash.ToLowerInvariant() -ne $binary.sha256) {
        throw "Installed game differs from the inspected baseline: $path"
    }
}

if (Test-Path -LiteralPath $target) {
    if (-not (Test-Path -LiteralPath (Join-Path $target $marker))) {
        throw "Refusing to overwrite an existing Workshop item without the Earthworks marker: $target"
    }
}

foreach ($file in @('earthworks_facility.nmf', 'earthworks_facility.mtl', 'previewimage.png')) {
    if (-not (Test-Path -LiteralPath (Join-Path $native $file))) {
        throw "Missing authored facility asset: $file"
    }
}

New-Item -ItemType Directory -Path $objectDir -Force | Out-Null
Copy-Item -LiteralPath (Join-Path $native 'earthworks_facility.nmf') -Destination (Join-Path $objectDir 'model.nmf') -Force
Copy-Item -LiteralPath (Join-Path $native 'earthworks_facility.mtl') -Destination (Join-Path $item 'material.mtl') -Force
Copy-Item -LiteralPath (Join-Path $native 'previewimage.png') -Destination (Join-Path $item 'previewimage.png') -Force
Copy-Item -LiteralPath (Join-Path $native 'previewimage.png') -Destination (Join-Path $objectDir 'imagegui.png') -Force

# This first placeable fixture retains the native construction-office class and
# frame data, but replaces the donor parking coordinates with two canopy bays.
# Exact approach orientation and vehicle flow must be checked by the user in game.
foreach ($file in @(
    @{ Source = 'construction_office3.bbox'; Destination = 'building.bbox' },
    @{ Source = 'construction_office3.fire'; Destination = 'building.fire' }
)) {
    $sourcePath = Join-Path $source $file.Source
    if (-not (Test-Path -LiteralPath $sourcePath)) { throw "Missing native building support file: $($file.Source)" }
    Copy-Item -LiteralPath $sourcePath -Destination (Join-Path $objectDir $file.Destination) -Force
}

$donorIni = Get-Content -LiteralPath (Join-Path $source 'muddy_construction.ini')
$retained = @($donorIni | Where-Object {
    $_ -match '^\s*\$(?:TYPE_CONSTRUCTION_OFFICE|NO_LIFESPAN|AMBIENT_SFX|WATERSEWAGE_DISABLE|ELECTRIC_DISABLE|HEATING_DISABLE|COUNT_LIMIT|RESOURCE_SOURCE_[A-Z_]+)(?:\s|$)'
})
$custom = [Collections.Generic.List[string]]::new()
$custom.Add("`$NAME_STR `"$name`"")
$custom.Add('$WORKERS_NEEDED 5')
$custom.Add('$WORKING_VEHICLES_NEEDED 2')
$custom.Add('')
$custom.AddRange([string[]]$retained)
$custom.Add('')
# Model coordinates: the canopy bays are centered at x=0.7 and x=13.3.
# Their heading points toward the single frontage at negative Z.
$custom.Add('$VEHICLE_PARKING 0.7000 0.0000 0.0000 0.7000 0.0000 -10.0000')
$custom.Add('$VEHICLE_PARKING 13.3000 0.0000 0.0000 13.3000 0.0000 -10.0000')
$custom.Add('')
# One straight road connector spans the two open bay fronts.
$custom.Add('$CONNECTION_ROAD')
$custom.Add('-7.0000 0.0000 -14.5000')
$custom.Add('21.0000 0.0000 -14.5000')
$custom.Add('')
$custom.Add('end')
[IO.File]::WriteAllLines((Join-Path $objectDir 'building.ini'), $custom, [Text.UTF8Encoding]::new($false))

$render = @(
    '$TYPE_WORKSHOP',
    ' MODEL model.nmf',
    ' MATERIAL ../material.mtl',
    ' LIFE 3000.000000',
    ' EXPLOSION_GROUP 0',
    ' DERBIS_FALLING_FX buildingfall1 1.000000',
    ' DERBIS_FALLED_FX buildingfall2 1.400000',
    ' DERBIS_FALLED_SFX collapse',
    ' DERBIS_NUM 20',
    ' DERBIS_FALLING_FX_MAXTIME 3.000000',
    ' DERBIS_SCALE 1.000000',
    ' DERBIS_MESH buildings/buildingwreck1.nmf buildings/buildingwreck.mtl',
    ' DERBIS_MESH buildings/buildingwreck2.nmf buildings/buildingwreck.mtl',
    ' END'
)
[IO.File]::WriteAllLines((Join-Path $objectDir 'renderconfig.ini'), $render, [Text.UTF8Encoding]::new($false))

$config = @"
`$ITEM_ID $id
`$OWNER_ID 0
`$ITEM_TYPE WORKSHOP_ITEMTYPE_BUILDING
`$VISIBILITY 0
`$OBJECT_BUILDING $object
`$ITEM_NAME "$name"
`$ITEM_DESC "Stage 12 prototype facility. Two vehicle bays; model, staffing and road access are being checked. Decorative tank only."
`$END
"@
[IO.File]::WriteAllText((Join-Path $item 'workshopconfig.ini'), $config, [Text.UTF8Encoding]::new($false))
[IO.File]::WriteAllText((Join-Path $item $marker), 'Generated locally by scripts/Stage-EarthworksBuilding.ps1. Stage 12 model and access prototype.', [Text.UTF8Encoding]::new($false))

if (-not (Test-Path -LiteralPath $target)) {
    New-Item -ItemType Directory -Path $targetRoot -Force | Out-Null
    New-Item -ItemType Directory -Path $target | Out-Null
}
Get-ChildItem -LiteralPath $target -Force | Remove-Item -Recurse -Force
Copy-Item -Path (Join-Path $item '*') -Destination $target -Recurse -Force

$parkingCount = @(Get-Content -LiteralPath (Join-Path $objectDir 'building.ini') | Where-Object { $_ -match '^\s*\$VEHICLE_PARKING\s' }).Count
if ($parkingCount -ne 2) { throw "Expected exactly two parking slots, found $parkingCount" }
Write-Host "Staged Stage 12 Earthworks facility at $target"
Write-Host 'The native construction-office type is retained; the custom model/material and two provisional parking routes replace the donor presentation.'
Write-Host 'The five-worker value and single-frontage routing are prototype settings pending the relevant Stage 12 checks.'
