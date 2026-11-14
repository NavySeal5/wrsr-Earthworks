[CmdletBinding()]
param()

$ErrorActionPreference = 'Stop'
Set-StrictMode -Version Latest

$root = Split-Path $PSScriptRoot -Parent
$game = Split-Path $root -Parent
$media = Join-Path $game 'media_soviet'
$id = '9100000014'
$object = 'EarthworksDevelopmentFacilityLarge'
$name = 'Earthworks Facility (Large)'
$source = Join-Path $media 'buildings_types'
$native = Join-Path $root 'assets\building\large\native'
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

foreach ($file in @('earthworks_facility_large.nmf', 'earthworks_facility_large.mtl', 'previewimage.png')) {
    if (-not (Test-Path -LiteralPath (Join-Path $native $file))) {
        throw "Missing authored facility asset: $file"
    }
}
$modelHash = (Get-FileHash -LiteralPath (Join-Path $native 'earthworks_facility_large.nmf') -Algorithm SHA256).Hash.ToLowerInvariant()

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
Copy-Item -LiteralPath (Join-Path $native 'earthworks_facility_large.nmf') -Destination (Join-Path $objectDir 'model.nmf') -Force
Copy-Item -LiteralPath (Join-Path $root 'assets\building\generated\large\earthworks_facility_large.obj') -Destination $objectDir -Force
Copy-Item -LiteralPath (Join-Path $root 'assets\building\generated\large\earthworks_facility_large.mtl') -Destination $objectDir -Force
Copy-Item -LiteralPath (Join-Path $native 'earthworks_facility_large.mtl') -Destination (Join-Path $item 'material.mtl') -Force
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
    @{ Name = 'canopy_structural_steel'; Color = @(78, 84, 88) },
    @{ Name = 'office_window_glass'; Color = @(74, 119, 135) },
    @{ Name = 'safety_red'; Color = @(160, 48, 38) },
    @{ Name = 'corrugated_galvanized_roof'; TextureMtl = 'earthworks_corrugated_roof.dds' },
    @{ Name = 'container_oxide_red'; Color = @(102, 48, 36) },
    @{ Name = 'container_blue'; Color = @(51, 74, 87) }
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
foreach ($textureName in @('earthworks_corrugated_roof.dds')) {
    $bakedTexture = Join-Path $root (Join-Path 'assets\building\generated\large' $textureName)
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
# Native asphalt court surface. It covers the yard and both structural wings;
# road/network access remains a separate connector at the U opening.
$custom.Add('$CONNECTIONS_ROAD_DEAD_SQUARE')
$custom.Add('-37.5000 -33.0000')
$custom.Add('37.5000 61.0000')
$custom.Add('')
# Target displayed material totals at 130% of Medium. Because the Large mesh
# has substantially more construction-node surface area, normalize each phase
# coefficient by the Medium/Large node-area ratio instead of scaling the raw
# coefficients by 1.30. Final totals still require a fresh in-game preview.
$custom.Add('-------')
$custom.Add('$COST_WORK SOVIET_CONSTRUCTION_GROUNDWORKS 0.0')
foreach ($node in @(
    'mesh_foundation_concrete'
)) {
    $custom.Add('$COST_WORK_BUILDING_NODE ' + $node)
}
$custom.Add('$COST_WORK_VEHICLE_STATION_ACCORDING_NODE mesh_foundation_concrete')
$custom.Add('$COST_RESOURCE_AUTO ground_asphalt 0.336')
$custom.Add('------------------')
$custom.Add('$COST_WORK SOVIET_CONSTRUCTION_BRICKS_LAYING 1.0')
# One construction crane station is available in each later build phase.
# This is for constructing the facility; it does not add crane use to Sites.
$custom.Add('$COST_WORK_VEHICLE_STATION -4.0000 0.0000 54.0000 4.0000 0.0000 54.0000')
foreach ($node in @(
    'mesh_dispatch_concrete',
    'mesh_window_frames',
    'mesh_safety_red',
    'mesh_office_window_glass'
)) {
    $custom.Add('$COST_WORK_BUILDING_NODE ' + $node)
}
$custom.Add('$COST_RESOURCE_AUTO wall_brick 0.147')
$custom.Add('$COST_RESOURCE_AUTO wall_wood 0.063')
$custom.Add('------------------')
$custom.Add('$COST_WORK SOVIET_CONSTRUCTION_ROOFTOP_BUILDING 1.0')
$custom.Add('$COST_WORK_VEHICLE_STATION -4.0000 0.0000 54.0000 4.0000 0.0000 54.0000')
foreach ($node in @(
    'mesh_office_roof_graphite',
    'mesh_corrugated_galvanized_roof',
    'mesh_canopy_structural_steel',
    'mesh_window_frames',
    'mesh_safety_red',
    'mesh_office_window_glass',
    'mesh_container_oxide_red',
    'mesh_container_blue'
)) {
    $custom.Add('$COST_WORK_BUILDING_NODE ' + $node)
}
$custom.Add('$COST_RESOURCE_AUTO roof_woodsteel 0.289')
$custom.Add('')
# Road-depot UI; 20 independent parking slots and two loading/unloading stations.
$custom.Add('')
foreach ($side in @(
    @{ X1 = -30.0000; X2 = -27.0000 },
    @{ X1 = 30.0000; X2 = 27.0000 }
)) {
    # Blender's longitudinal axis is inverted in the building config. The
    # covered centers (-33..16 in Blender Y) map to game Z (+33..-16); the
    # open-air centers (-47/-54) map to game Z +47/+54. Keep this conversion
    # explicit so no position lands on the rear service building.
    foreach ($z in @('-16.0000', '-9.0000', '-2.0000', '5.0000', '12.0000', '19.0000', '26.0000', '33.0000', '47.0000', '54.0000')) {
        # Native parking entries also define the ground bay marking span.
        # Small/Medium use 10 m spans; Large keeps the same vehicle center and
        # inward-facing direction while extending each short 3 m span evenly.
        $centerX = (([double]$side.X1 + [double]$side.X2) / 2.0)
        if ([double]$side.X1 -lt 0) {
            $x1 = ($centerX - 5.0).ToString('0.0000', [Globalization.CultureInfo]::InvariantCulture)
            $x2 = ($centerX + 5.0).ToString('0.0000', [Globalization.CultureInfo]::InvariantCulture)
        }
        else {
            $x1 = ($centerX + 5.0).ToString('0.0000', [Globalization.CultureInfo]::InvariantCulture)
            $x2 = ($centerX - 5.0).ToString('0.0000', [Globalization.CultureInfo]::InvariantCulture)
        }
        $custom.Add('$VEHICLE_PARKING ' + $x1 + ' 0.0000 ' + $z + ' ' + $x2 + ' 0.0000 ' + $z)
    }
}
# Center the two loading/unloading positions side by side in the court, near
# the main building frontage. Blender Y is inverted into game Z for this asset.
$custom.Add('$VEHICLE_STATION -10.0000 0.0000 -12.0000 -2.0000 0.0000 -12.0000')
$custom.Add('$VEHICLE_STATION 2.0000 0.0000 -12.0000 10.0000 0.0000 -12.0000')
$custom.Add('')
# Single connector at the U opening / road-approach end. Keep it centered and
# point it outward from this end of the footprint.
$custom.Add('$CONNECTION_ROAD')
$custom.Add('0.0000 0.0000 61.0000')
$custom.Add('0.0000 0.0000 51.0000')
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
`$ITEM_DESC "Stage 12 large vehicle-only Earthworks facility prototype. Twenty independent stalls, two loading stations. No fuel storage is configured."
`$END
"@
[IO.File]::WriteAllText((Join-Path $item 'workshopconfig.ini'), $config, [Text.UTF8Encoding]::new($false))
[IO.File]::WriteAllText((Join-Path $item $marker), 'Generated locally by scripts/Stage-EarthworksBuilding-Large.ps1. Stage 12 Large model and access prototype.', [Text.UTF8Encoding]::new($false))

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
if ($parkingCount -ne 20 -or $stationCount -ne 2) {
    throw "Expected twenty parking slots and two loading stations, found $parkingCount parking slots and $stationCount stations"
}
Write-Host "Staged Stage 12 Large Earthworks facility at $target"
Write-Host 'The native road-depot type is used; the vehicle allowlist and Earthworks project scheduler remain plugin-controlled.'
Write-Host 'Generated bbox/fire files are preserved on restage while the NMF model hash remains unchanged.'
Write-Host 'The custom model uses sixteen covered garage stalls, four open-air stalls at the U opening, and two loading stations.'
Write-Host 'No workers are required for Earthworks output. Fuel storage is omitted.'

