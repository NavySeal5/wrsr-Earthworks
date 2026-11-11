[CmdletBinding()]
param()

$ErrorActionPreference = 'Stop'
Set-StrictMode -Version Latest

$root = Split-Path $PSScriptRoot -Parent
$game = Split-Path $root -Parent
$media = Join-Path $game 'media_soviet'
$id = '9100000013'
$object = 'EarthworksDevelopmentFacilityMedium'
$name = 'Earthworks Facility (Medium)'
$source = Join-Path $media 'buildings_types'
$native = Join-Path $root 'assets\building\medium\native'
$output = Join-Path $root 'build\earthworks-workshop'
$item = Join-Path $output $id
$objectDir = Join-Path $item $object
$targetRoot = Join-Path $media 'workshop_wip'
$target = Join-Path $targetRoot $id
$marker = '.earthworks-generated'

if (Test-Path -LiteralPath $target) {
    if (-not (Test-Path -LiteralPath (Join-Path $target $marker))) {
        throw "Refusing to overwrite an existing Workshop item without the Earthworks marker: $target"
    }
}

foreach ($file in @('earthworks_facility_medium.nmf', 'earthworks_facility_medium.mtl', 'previewimage.png')) {
    if (-not (Test-Path -LiteralPath (Join-Path $native $file))) {
        throw "Missing authored facility asset: $file"
    }
}
$modelHash = (Get-FileHash -LiteralPath (Join-Path $native 'earthworks_facility_medium.nmf') -Algorithm SHA256).Hash.ToLowerInvariant()

function ConvertTo-Rgb565([int]$Red, [int]$Green, [int]$Blue) {
    return [uint16]((($Red -band 0xF8) -shl 8) -bor (($Green -band 0xFC) -shl 3) -bor ($Blue -shr 3))
}

function Write-SolidDds([string]$Path, [int]$Red, [int]$Green, [int]$Blue) {
    # A single BC1/DXT1 block is enough for a flat, unmistakable diagnostic
    # color. The texture is deliberately tiny but uses the compressed format
    # expected by the game's asset pipeline.
    $header = [byte[]]::new(128)
    [Array]::Copy([Text.Encoding]::ASCII.GetBytes('DDS '), 0, $header, 0, 4)
    $u32 = [BitConverter]::GetBytes([uint32]124)
    [Array]::Copy($u32, 0, $header, 4, 4)
    $u32 = [BitConverter]::GetBytes([uint32]0x00081007)
    [Array]::Copy($u32, 0, $header, 8, 4)
    $u32 = [BitConverter]::GetBytes([uint32]4)
    [Array]::Copy($u32, 0, $header, 12, 4)
    [Array]::Copy($u32, 0, $header, 16, 4)
    $u32 = [BitConverter]::GetBytes([uint32]8)
    [Array]::Copy($u32, 0, $header, 20, 4)
    $u32 = [BitConverter]::GetBytes([uint32]32)
    [Array]::Copy($u32, 0, $header, 76, 4)
    $u32 = [BitConverter]::GetBytes([uint32]4)
    [Array]::Copy($u32, 0, $header, 80, 4)
    [Array]::Copy([Text.Encoding]::ASCII.GetBytes('DXT1'), 0, $header, 84, 4)
    $u32 = [BitConverter]::GetBytes([uint32]0x00001000)
    [Array]::Copy($u32, 0, $header, 108, 4)

    $color0 = ConvertTo-Rgb565 $Red $Green $Blue
    $color1 = [uint16]0
    if ($color0 -eq 0) { $color1 = [uint16]1 }
    $block = [byte[]]::new(8)
    [Array]::Copy([BitConverter]::GetBytes($color0), 0, $block, 0, 2)
    [Array]::Copy([BitConverter]::GetBytes($color1), 0, $block, 2, 2)
    # All sixteen 2-bit selectors are zero, selecting color0.
    [IO.File]::WriteAllBytes($Path, $header + $block)
}

New-Item -ItemType Directory -Path $objectDir -Force | Out-Null
foreach ($generatedFile in @('building.bbox', 'building.fire', 'material.mtl')) {
    $generatedPath = Join-Path $objectDir $generatedFile
    if (Test-Path -LiteralPath $generatedPath) { Remove-Item -LiteralPath $generatedPath -Force }
}
Remove-Item -LiteralPath (Join-Path $objectDir 'earthworks_facility_probe.mtl') -Force -ErrorAction SilentlyContinue
Get-ChildItem -LiteralPath $objectDir -Filter 'earthworks_probe_*.dds' -File -ErrorAction SilentlyContinue | Remove-Item -Force
# Clear only textures owned by this generated facility, including material
# names retired by a source revision, before writing the current set below.
Get-ChildItem -LiteralPath $objectDir -Filter 'earthworks_*.dds' -File -ErrorAction SilentlyContinue | Remove-Item -Force
Copy-Item -LiteralPath (Join-Path $native 'earthworks_facility_medium.nmf') -Destination (Join-Path $objectDir 'model.nmf') -Force
Copy-Item -LiteralPath (Join-Path $root 'assets\building\generated\medium\earthworks_facility_medium.obj') -Destination $objectDir -Force
Copy-Item -LiteralPath (Join-Path $root 'assets\building\generated\medium\earthworks_facility_medium.mtl') -Destination $objectDir -Force
Copy-Item -LiteralPath (Join-Path $native 'earthworks_facility_medium.mtl') -Destination (Join-Path $item 'material.mtl') -Force
Copy-Item -LiteralPath (Join-Path $native 'previewimage.png') -Destination (Join-Path $item 'previewimage.png') -Force
Copy-Item -LiteralPath (Join-Path $native 'previewimage.png') -Destination (Join-Path $objectDir 'imagegui.png') -Force

# Use Workshop-local BC1 textures for authored colors, and the installed
# game's firestation/garage atlases for the large concrete and metal surfaces.
# The Blender source UVs those surfaces into the corresponding atlas regions.
$facilityMaterials = @(
    @{ Name = 'foundation_concrete'; Color = @(150, 151, 146) },
    @{ Name = 'dispatch_concrete'; Texture = 'buildings/construction_office.dds' },
    @{ Name = 'office_roof_graphite'; Texture = 'buildings/firestation.dds' },
    @{ Name = 'window_frames'; Color = @(48, 57, 63) },
    @{ Name = 'safety_red'; Color = @(160, 48, 38) },
    @{ Name = 'office_window_glass'; Color = @(74, 119, 135) },
    @{ Name = 'canopy_structural_steel'; Color = @(78, 84, 88) },
    # The Blender authoring step bakes these atlas patches into local BC1 DDS
    # files so darkening does not depend on a material tint in the game.
    @{ Name = 'decorative_tank_painted_steel'; TextureMtl = 'earthworks_tank_steel.dds' },
    @{ Name = 'tank_cap_steel'; Color = @(71, 79, 71) },
    @{ Name = 'corrugated_galvanized_roof'; TextureMtl = 'earthworks_corrugated_roof.dds' }
)
$facilityMtl = [Collections.Generic.List[string]]::new()
foreach ($material in $facilityMaterials) {
    $facilityMtl.Add('$SUBMATERIAL ' + $material.Name)
    if ($material.ContainsKey('Texture')) {
        $facilityMtl.Add('$TEXTURE 0 ' + $material.Texture)
    } elseif ($material.ContainsKey('TextureMtl')) {
        $facilityMtl.Add('$TEXTURE_MTL 0 ' + $material.TextureMtl)
    } else {
        $textureName = 'earthworks_' + $material.Name + '.dds'
        Write-SolidDds (Join-Path $objectDir $textureName) $material.Color[0] $material.Color[1] $material.Color[2]
        $facilityMtl.Add('$TEXTURE_MTL 0 ' + $textureName)
    }
    $facilityMtl.Add('$TEXTURE 1 blankspecular.dds')
    $facilityMtl.Add('$TEXTURE 2 blankbump.dds')
    $facilityMtl.Add('$DIFFUSECOLOR 1.0 1.0 1.0 1.0')
    $facilityMtl.Add('$SPECULARCOLOR 0.18 0.18 0.18 1.0')
    $facilityMtl.Add('$AMBIENTCOLOR 1.0 1.0 1.0 1.0')
    $facilityMtl.Add('$SPECULARPOWER 2.0')
    $facilityMtl.Add('')
}
$facilityMtl.Add('$END')
[IO.File]::WriteAllLines((Join-Path $objectDir 'earthworks_facility_material.mtl'), $facilityMtl, [Text.UTF8Encoding]::new($false))
foreach ($textureName in @('earthworks_corrugated_roof.dds', 'earthworks_tank_steel.dds')) {
    $bakedTexture = Join-Path $root (Join-Path 'assets\building\generated\medium' $textureName)
    if (-not (Test-Path -LiteralPath $bakedTexture)) { throw "Missing baked material texture: $bakedTexture" }
    Copy-Item -LiteralPath $bakedTexture -Destination $native -Force
    Copy-Item -LiteralPath $bakedTexture -Destination $objectDir -Force
    Copy-Item -LiteralPath $bakedTexture -Destination $item -Force
}

# Do not copy the construction office donor's generated bbox/fire data onto this
# unrelated NMF. The game generates these from the Workshop model; stale donor
# files can leave the new construction nodes without a matching bbox. Preserve
# generated files across restages while the NMF is unchanged, and invalidate
# them whenever the model changes.

$donorIni = Get-Content -LiteralPath (Join-Path $source 'muddy_construction.ini')
$retained = @($donorIni | Where-Object {
    $_ -match '^\s*\$(?:NO_LIFESPAN|AMBIENT_SFX|WATERSEWAGE_DISABLE|ELECTRIC_DISABLE|HEATING_DISABLE|COUNT_LIMIT)(?:\s|$)'
})
$custom = [Collections.Generic.List[string]]::new()
$custom.Add("`$NAME_STR `"$name`"")
$custom.Add('')
$custom.Add('$TYPE_ROADDEPO')
$custom.Add('')
$custom.AddRange([string[]]$retained)
$custom.Add('')
# Native surface-area metadata is separate from road/network access points.
# The asphalt rectangle covers the full four-bay court and the maneuvering
# apron; its right edge follows the garage end and side-connection anchor at (45.5, 15).
$custom.Add('$CONNECTIONS_ROAD_DEAD_SQUARE')
$custom.Add('-6.5000 -10.0000')
$custom.Add('45.5000 20.0000')
$custom.Add('')
# The office-side apron uses the Building Editor's airport/tarmac surface.
# Extend it to the asphalt court's far Z edge and the gravel strip's near Z edge,
# with its asphalt-side edge aligned to the asphalt court boundary.
$custom.Add('$CONNECTIONS_AIRPORT_DEAD_SQUARE')
$custom.Add('-19.0000 -8.5000')
$custom.Add('-6.5000 20.0000')
$custom.Add('')
# The marked lower-right section extends the office apron toward the tank side.
# Keep it a separate rectangle so the adjoining pedestrian/gravel patch remains
# intact rather than overlapping it with airport concrete.
$custom.Add('$CONNECTIONS_AIRPORT_DEAD_SQUARE')
$custom.Add('-25.5000 8.5000')
$custom.Add('-19.0000 20.0000')
$custom.Add('')
# In the game's area-surface mapping, pedestrian dead squares render the
# gravel-like ground surface. Cover the tank flank while keeping its rectangle
# adjacent to (not overlapping) the office's airport-concrete apron.
$custom.Add('$CONNECTIONS_PEDESTRIAN_DEAD_SQUARE')
$custom.Add('-25.5000 -8.5000')
$custom.Add('-19.0000 8.5000')
$custom.Add('')
# Keep the native construction-office phase sequence, using the custom model's
# node names. Resource coefficients are 120% of the Small facility's current
# values, matching the accepted Medium-to-Small cost relationship.
$custom.Add('-------')
$custom.Add('$COST_WORK SOVIET_CONSTRUCTION_GROUNDWORKS 0.0')
foreach ($node in @(
    'mesh_foundation_concrete'
)) {
    $custom.Add('$COST_WORK_BUILDING_NODE ' + $node)
}
$custom.Add('$COST_WORK_VEHICLE_STATION_ACCORDING_NODE mesh_foundation_concrete')
$custom.Add('$COST_RESOURCE_AUTO ground_asphalt 1.350')
$custom.Add('------------------')
$custom.Add('$COST_WORK SOVIET_CONSTRUCTION_BRICKS_LAYING 1.0')
foreach ($node in @(
    'mesh_dispatch_concrete',
    'mesh_window_frames',
    'mesh_safety_red',
    'mesh_office_window_glass'
)) {
    $custom.Add('$COST_WORK_BUILDING_NODE ' + $node)
}
$custom.Add('$COST_RESOURCE_AUTO wall_brick 0.630')
$custom.Add('$COST_RESOURCE_AUTO wall_wood 0.270')
$custom.Add('------------------')
$custom.Add('$COST_WORK SOVIET_CONSTRUCTION_ROOFTOP_BUILDING 1.0')
foreach ($node in @(
    'mesh_canopy_structural_steel',
    'mesh_decorative_tank_painted_steel',
    'mesh_tank_cap_steel',
    'mesh_corrugated_galvanized_roof',
    'mesh_office_roof_graphite'
)) {
    $custom.Add('$COST_WORK_BUILDING_NODE ' + $node)
}
$custom.Add('$COST_RESOURCE_AUTO roof_woodsteel 0.720')
$custom.Add('')
# Use the road-depot native panel for a vehicle-only facility. Keep eight
# parking slots in the canopy bays, but use one loading/unloading station in
# the office forecourt. Fuel storage is
# omitted for now; the visible tank is decorative only.
$custom.Add('')
# Model coordinates: four canopy bays span x=-5.6 to 44.8. Two parking lanes
# per bay are centered 6 m apart; the added bays continue the Small spacing.
# Move both parking anchors 5 m toward the rear of the bays so longer vehicles
# sit farther beneath the canopy. Preserve the +Z heading toward the opening.
$custom.Add('$VEHICLE_STATION -15.5000 0.0000 18.0000 -9.5000 0.0000 18.0000')
foreach ($x in @('-2.3000', '3.7000', '10.3000', '16.3000', '22.9000', '28.9000', '35.5000', '41.5000')) {
    $custom.Add('$VEHICLE_PARKING ' + $x + ' 0.0000 -5.0000 ' + $x + ' 0.0000 5.0000')
}
$custom.Add('')
# Put the road access at the far-right end of the medium canopy and point it
# outward toward +X, as selected for the Medium layout. The parking orientation
# remains front-facing; the expanded apron gives vehicles room to turn in.
$custom.Add('$CONNECTION_ROAD')
$custom.Add('45.5000 0.0000 15.0000')
$custom.Add('35.5000 0.0000 15.0000')
$custom.Add('')
$custom.Add('end')
[IO.File]::WriteAllLines((Join-Path $objectDir 'building.ini'), $custom, [Text.UTF8Encoding]::new($false))

$render = @(
    '$TYPE_WORKSHOP',
    ' MODEL model.nmf',
    ' MATERIAL earthworks_facility_material.mtl',
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
`$ITEM_DESC "Stage 12 vehicle-only Earthworks facility prototype. Four road-depot bays. Decorative tank only; fuel storage is not configured."
`$END
"@
[IO.File]::WriteAllText((Join-Path $item 'workshopconfig.ini'), $config, [Text.UTF8Encoding]::new($false))
[IO.File]::WriteAllText((Join-Path $item $marker), 'Generated locally by scripts/Stage-EarthworksBuilding.ps1. Stage 12 Medium model and access prototype.', [Text.UTF8Encoding]::new($false))

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
$targetObject = Join-Path $target $object
Remove-Item -LiteralPath (Join-Path $targetObject 'earthworks_facility_probe.mtl') -Force -ErrorAction SilentlyContinue
Get-ChildItem -LiteralPath $targetObject -Filter 'earthworks_probe_*.dds' -File -ErrorAction SilentlyContinue | Remove-Item -Force
Get-ChildItem -LiteralPath $targetObject -Filter 'earthworks_*.dds' -File -ErrorAction SilentlyContinue | Remove-Item -Force
Copy-Item -Path (Join-Path $objectDir 'earthworks_*.dds') -Destination $targetObject -Force
[IO.File]::WriteAllText($modelHashMarker, $modelHash, [Text.UTF8Encoding]::new($false))

$parkingCount = @(Get-Content -LiteralPath (Join-Path $objectDir 'building.ini') | Where-Object { $_ -match '^\s*\$VEHICLE_PARKING\s' }).Count
$stationCount = @(Get-Content -LiteralPath (Join-Path $objectDir 'building.ini') | Where-Object { $_ -match '^\s*\$VEHICLE_STATION\s' }).Count
if ($parkingCount -ne 8 -or $stationCount -ne 1) {
    throw "Expected eight parking slots and one forecourt vehicle station, found $parkingCount parking slots and $stationCount stations"
}
Write-Host "Staged Stage 12 Medium Earthworks facility at $target"
Write-Host 'The native road-depot type is used; the vehicle allowlist and Earthworks project scheduler remain plugin-controlled.'
Write-Host 'Generated bbox/fire files are preserved on restage while the NMF model hash remains unchanged.'
Write-Host 'The custom model uses eight parking slots (two per bay) and one separate vehicle station in the office forecourt.'
Write-Host 'No workers are required for Earthworks output. Fuel storage is omitted; the visible tank is decorative.'

