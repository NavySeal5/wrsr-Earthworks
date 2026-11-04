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
$modelHash = (Get-FileHash -LiteralPath (Join-Path $native 'earthworks_facility.nmf') -Algorithm SHA256).Hash.ToLowerInvariant()

New-Item -ItemType Directory -Path $objectDir -Force | Out-Null
foreach ($generatedFile in @('building.bbox', 'building.fire', 'material.mtl')) {
    $generatedPath = Join-Path $objectDir $generatedFile
    if (Test-Path -LiteralPath $generatedPath) { Remove-Item -LiteralPath $generatedPath -Force }
}
Copy-Item -LiteralPath (Join-Path $native 'earthworks_facility.nmf') -Destination (Join-Path $objectDir 'model.nmf') -Force
Copy-Item -LiteralPath (Join-Path $native 'earthworks_facility.mtl') -Destination (Join-Path $item 'material.mtl') -Force
Copy-Item -LiteralPath (Join-Path $native 'previewimage.png') -Destination (Join-Path $item 'previewimage.png') -Force
Copy-Item -LiteralPath (Join-Path $native 'previewimage.png') -Destination (Join-Path $objectDir 'imagegui.png') -Force

# Do not copy the construction office donor's generated bbox/fire data onto this
# unrelated NMF. The game generates these from the Workshop model; stale donor
# files can leave the new construction nodes without a matching bbox. Preserve
# generated files across restages while the NMF is unchanged, and invalidate
# them whenever the model changes.

$donorIni = Get-Content -LiteralPath (Join-Path $source 'muddy_construction.ini')
$retained = @($donorIni | Where-Object {
    $_ -match '^\s*\$(?:TYPE_CONSTRUCTION_OFFICE|NO_LIFESPAN|AMBIENT_SFX|WATERSEWAGE_DISABLE|ELECTRIC_DISABLE|HEATING_DISABLE|COUNT_LIMIT)(?:\s|$)'
})
$resourceSources = @($donorIni | Where-Object {
    $_ -match '^\s*\$RESOURCE_SOURCE_[A-Z_]+(?:\s|$)'
})
$custom = [Collections.Generic.List[string]]::new()
$custom.Add("`$NAME_STR `"$name`"")
$custom.Add('')
$custom.AddRange([string[]]$retained)
$custom.Add('')
# Keep the native construction-office phase sequence, using the custom model's
# node names. The large values are diagnostic only and make the build-menu
# summary easy to inspect; they are not final construction balance.
$custom.Add('-------')
$custom.Add('$COST_WORK SOVIET_CONSTRUCTION_GROUNDWORKS 0.0')
foreach ($node in @(
    'mesh_foundation_concrete',
    'mesh_parking_bay_markings'
)) {
    $custom.Add('$COST_WORK_BUILDING_NODE ' + $node)
}
$custom.Add('$COST_WORK_VEHICLE_STATION_ACCORDING_NODE mesh_foundation_concrete')
$custom.Add('$COST_RESOURCE_AUTO ground_asphalt 1.5')
$custom.Add('------------------')
$custom.Add('$COST_WORK SOVIET_CONSTRUCTION_BRICKS_LAYING 1.0')
foreach ($node in @(
    'mesh_dispatch_plaster_pale',
    'mesh_window_frames',
    'mesh_safety_red',
    'mesh_office_window_glass'
)) {
    $custom.Add('$COST_WORK_BUILDING_NODE ' + $node)
}
$custom.Add('$COST_RESOURCE_AUTO wall_brick 0.7')
$custom.Add('$COST_RESOURCE_AUTO wall_wood 0.3')
$custom.Add('------------------')
$custom.Add('$COST_WORK SOVIET_CONSTRUCTION_ROOFTOP_BUILDING 1.0')
foreach ($node in @(
    'mesh_canopy_structural_steel',
    'mesh_decorative_tank_painted_steel',
    'mesh_corrugated_galvanized_roof',
    'mesh_roof_rib_highlight'
)) {
    $custom.Add('$COST_WORK_BUILDING_NODE ' + $node)
}
$custom.Add('$COST_RESOURCE_AUTO roof_woodsteel 0.8')
$custom.Add('')
$custom.AddRange([string[]]$resourceSources)
$custom.Add('')
$custom.Add('$WORKERS_NEEDED 5')
$custom.Add('$WORKING_VEHICLES_NEEDED 2')
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
$modelHashMarker = Join-Path $target '.earthworks-model-hash'
$previousModelHash = if (Test-Path -LiteralPath $modelHashMarker) {
    (Get-Content -LiteralPath $modelHashMarker -Raw).Trim()
} else { '' }
if ($previousModelHash -ne $modelHash) {
    foreach ($generatedFile in @('building.bbox', 'building.fire')) {
        $generatedPath = Join-Path $target (Join-Path $object $generatedFile)
        if (Test-Path -LiteralPath $generatedPath) { Remove-Item -LiteralPath $generatedPath -Force }
    }
}
$targetObjectMaterial = Join-Path $target (Join-Path $object 'material.mtl')
if (Test-Path -LiteralPath $targetObjectMaterial) { Remove-Item -LiteralPath $targetObjectMaterial -Force }
Copy-Item -Path (Join-Path $item '*') -Destination $target -Recurse -Force
[IO.File]::WriteAllText($modelHashMarker, $modelHash, [Text.UTF8Encoding]::new($false))

$parkingCount = @(Get-Content -LiteralPath (Join-Path $objectDir 'building.ini') | Where-Object { $_ -match '^\s*\$VEHICLE_PARKING\s' }).Count
if ($parkingCount -ne 2) { throw "Expected exactly two parking slots, found $parkingCount" }
Write-Host "Staged Stage 12 Earthworks facility at $target"
Write-Host 'The native construction-office type is retained; donor bbox/fire files are omitted so the game can generate data for the custom NMF.'
Write-Host 'Generated bbox/fire files are preserved on restage while the NMF model hash remains unchanged.'
Write-Host 'The custom model and item-level material are linked from renderconfig, with two provisional parking routes.'
Write-Host 'The five-worker value and single-frontage routing are prototype settings pending the relevant Stage 12 checks.'
