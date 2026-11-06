[CmdletBinding()]
param()

$ErrorActionPreference = 'Stop'
Set-StrictMode -Version Latest
$root = Split-Path $PSScriptRoot -Parent
$game = Split-Path $root -Parent
$id = '9100000013'
$object = 'EarthworksDevelopmentFacilityMedium'
$targetRoot = Join-Path $game 'media_soviet\workshop_wip'
$item = Join-Path $targetRoot $id
$objectDir = Join-Path $item $object
$marker = Join-Path $item '.earthworks-generated'
$source = Join-Path $root 'assets\building\generated\medium'

if ((Test-Path -LiteralPath $item) -and -not (Test-Path -LiteralPath $marker)) {
    throw "Refusing to modify an existing unmarked Workshop item: $item"
}
foreach ($file in @('earthworks_facility_medium.obj', 'earthworks_facility_medium.mtl', 'earthworks_corrugated_roof.dds', 'earthworks_tank_steel.dds')) {
    if (-not (Test-Path -LiteralPath (Join-Path $source $file))) {
        throw "Missing generated ModelViewer import file: $file"
    }
}

New-Item -ItemType Directory -Path $objectDir -Force | Out-Null
Copy-Item -LiteralPath (Join-Path $source 'earthworks_facility_medium.obj') -Destination $objectDir -Force
Copy-Item -LiteralPath (Join-Path $source 'earthworks_facility_medium.mtl') -Destination $objectDir -Force
Copy-Item -LiteralPath (Join-Path $source 'earthworks_corrugated_roof.dds') -Destination $objectDir -Force
Copy-Item -LiteralPath (Join-Path $source 'earthworks_tank_steel.dds') -Destination $objectDir -Force
[IO.File]::WriteAllText($marker, 'Generated locally for the Medium ModelViewer import.', [Text.UTF8Encoding]::new($false))
Write-Host "Medium OBJ/MTL and local textures are ready under the game media folder: $objectDir"
Write-Host 'In ModelViewer import earthworks_facility_medium.obj from this folder; save the NMF as model.nmf in this same object folder.'
