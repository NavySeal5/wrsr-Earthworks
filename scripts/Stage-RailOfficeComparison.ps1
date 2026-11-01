[CmdletBinding()]
param()

$ErrorActionPreference='Stop'
Set-StrictMode -Version Latest
$root=Split-Path $PSScriptRoot -Parent
$game=Split-Path $root -Parent
$media=Join-Path $game 'media_soviet'
$id='9100000013'
$object='EarthworksRailOfficeComparison'
$name='Earthworks Rail Office Comparison (test)'
$donor='rail_construction_office'
$output=Join-Path $root 'build\earthworks-rail-comparison'
$item=Join-Path $output $id
$objectDir=Join-Path $item $object
$targetRoot=Join-Path $media 'workshop_wip'
$target=Join-Path $targetRoot $id
$marker='.earthworks-generated'

$baseline=Get-Content -LiteralPath (Join-Path $root 'docs\game-baseline.json') -Raw | ConvertFrom-Json
foreach($binary in $baseline.binaries) {
    $path=Join-Path $game $binary.name
    if((Get-FileHash -LiteralPath $path -Algorithm SHA256).Hash.ToLowerInvariant() -ne $binary.sha256) {
        throw "Installed game differs from the inspected baseline: $path"
    }
}
if((Test-Path -LiteralPath $target) -and -not (Test-Path -LiteralPath (Join-Path $target $marker))) {
    throw "Refusing to overwrite an existing WIP item without the Earthworks marker: $target"
}
New-Item -ItemType Directory -Path $objectDir -Force | Out-Null

$required=@(
    @{Path="buildings\$donor.nmf";Dest="$object\model.nmf"},
    @{Path="buildings_types\$donor.bbox";Dest="$object\building.bbox"},
    @{Path="buildings_types\$donor.fire";Dest="$object\building.fire"},
    @{Path="editor\tool_$donor.png";Dest="$object\imagegui.png"},
    @{Path="buildings_types\$donor.ini";Dest="$object\building.ini"},
    @{Path="buildings\$donor.mtl";Dest='material.mtl'}
)
foreach($file in $required) {
    if(-not (Test-Path -LiteralPath (Join-Path $media $file.Path))) { throw "Missing native donor asset: $($file.Path)" }
}
foreach($file in $required | Where-Object { $_.Dest -ne "$object\building.ini" -and $_.Dest -ne 'material.mtl' }) {
    $destination=Join-Path $item $file.Dest
    New-Item -ItemType Directory -Path (Split-Path $destination -Parent) -Force | Out-Null
    Copy-Item -LiteralPath (Join-Path $media $file.Path) -Destination $destination -Force
}
Copy-Item -LiteralPath (Join-Path $media "editor\tool_$donor.png") -Destination (Join-Path $item 'previewimage.png') -Force

function Copy-RewrittenMaterial([string]$sourceFile,[string]$destination) {
    $lines=Get-Content -LiteralPath $sourceFile
    $rewritten=@(foreach($line in $lines) {
        if($line -match '^(?<prefix>.*)\$TEXTURE_MTL\s+(?<slot>\S+)\s+(?<path>\S+)\s*$') {
            "$($Matches.prefix)`$TEXTURE $($Matches.slot) buildings/$($Matches.path)"
        } else {$line}
    })
    [IO.File]::WriteAllLines($destination,$rewritten,[Text.UTF8Encoding]::new($false))
}
Copy-RewrittenMaterial (Join-Path $media "buildings\$donor.mtl") (Join-Path $item 'material.mtl')
$emissive=Test-Path -LiteralPath (Join-Path $media "buildings\${donor}_e.mtl")
if($emissive) { Copy-RewrittenMaterial (Join-Path $media "buildings\${donor}_e.mtl") (Join-Path $item 'material_e.mtl') }

$donorIni=@(Get-Content -LiteralPath (Join-Path $media "buildings_types\$donor.ini") | Where-Object { $_ -notmatch '^\s*\$NAME(?:\s|$)' })
$workerLineCount=@($donorIni | Where-Object { $_ -match '^\s*\$WORKERS_NEEDED(?:\s|$)' }).Count
if($workerLineCount -ne 1) { throw "Expected one native worker-capacity line in $donor.ini; found $workerLineCount" }
$custom=@("`$NAME_STR `"$name`"") + @($donorIni | ForEach-Object {
    if($_ -match '^\s*\$WORKERS_NEEDED(?:\s|$)') { '$WORKERS_NEEDED 5' } else { $_ }
})
[IO.File]::WriteAllLines((Join-Path $objectDir 'building.ini'),$custom,[Text.UTF8Encoding]::new($false))

$render=@('$TYPE_WORKSHOP',' MODEL model.nmf',' MATERIAL ../material.mtl')
if($emissive){$render+=' MATERIALEMISSIVE ../material_e.mtl'}
$render+=@(' LIFE 3000.000000',' EXPLOSION_GROUP 0',' DERBIS_FALLING_FX buildingfall1 1.000000',
    ' DERBIS_FALLED_FX buildingfall2 1.400000',' DERBIS_FALLED_SFX collapse',' DERBIS_NUM 20',
    ' DERBIS_FALLING_FX_MAXTIME 3.000000',' DERBIS_SCALE 1.000000',
    ' DERBIS_MESH buildings/buildingwreck1.nmf buildings/buildingwreck.mtl',
    ' DERBIS_MESH buildings/buildingwreck2.nmf buildings/buildingwreck.mtl',' END')
[IO.File]::WriteAllLines((Join-Path $objectDir 'renderconfig.ini'),$render,[Text.UTF8Encoding]::new($false))

$config=@"
`$ITEM_ID $id
`$OWNER_ID 0
`$ITEM_TYPE WORKSHOP_ITEMTYPE_BUILDING
`$VISIBILITY 0
`$OBJECT_BUILDING $object
`$ITEM_NAME "$name"
`$ITEM_DESC "Comparison-only copy of the native rail construction office type. Temporarily set to 5 worker places; keeps its three rail vehicle stations and rail-specific behavior. Do not use as the Earthworks station."
`$END
"@
[IO.File]::WriteAllText((Join-Path $item 'workshopconfig.ini'),$config,[Text.UTF8Encoding]::new($false))
[IO.File]::WriteAllText((Join-Path $item $marker),'Generated locally by scripts/Stage-RailOfficeComparison.ps1. Comparison-only native rail construction office; do not publish.',[Text.UTF8Encoding]::new($false))

if(-not (Test-Path -LiteralPath $target)) { New-Item -ItemType Directory -Path $targetRoot -Force | Out-Null; New-Item -ItemType Directory -Path $target | Out-Null }
Copy-Item -Path (Join-Path $item '*') -Destination $target -Recurse -Force
Write-Host "Staged rail-office comparison item $id at $target"
Write-Host 'Temporary settings: 5 worker places and the native three rail vehicle stations.'
Write-Host 'This comparison building retains rail-only construction, vehicle, and material behavior; do not register it as the Earthworks station.'
