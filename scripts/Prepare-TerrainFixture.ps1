[CmdletBinding()]
param()
$ErrorActionPreference = 'Stop'
Set-StrictMode -Version Latest
$root = Split-Path $PSScriptRoot -Parent
$game = Split-Path $root -Parent
if (Get-Process SOVIET64,SOVIET -ErrorAction SilentlyContinue) { throw 'Close the game before preparing the fixture copy.' }
$saves = Join-Path $game 'media_soviet\save'
$source = Join-Path $saves '4504 - Earthworks_Test_S01'
$destination = Join-Path $saves '4505 - Earthworks_Test_S03'
if (Test-Path -LiteralPath $destination) { throw 'S03 fixture already exists; refusing to overwrite it.' }
if (@(Get-ChildItem -LiteralPath $saves -Directory | Where-Object {$_.Name -like '4505 - *'}).Count -gt 0) { throw 'Save slot 4505 is in use. Review before creating a fixture.' }
$headerPath = Join-Path $source 'header.bin'
$bytes = [IO.File]::ReadAllBytes($headerPath)
$oldName = [Text.Encoding]::Unicode.GetBytes('Earthworks_Test_S01')
$newName = [Text.Encoding]::Unicode.GetBytes('Earthworks_Test_S03')
for ($i=0; $i -lt $oldName.Length; $i++) { if($bytes[4+$i] -ne $oldName[$i]) { throw 'Unexpected S01 display-name layout; no copy created.' } }
$oldPath = [Text.Encoding]::ASCII.GetBytes('save/4504 - Earthworks_Test_S01')
$newPath = [Text.Encoding]::ASCII.GetBytes('save/4505 - Earthworks_Test_S03')
$matches = @()
for($i=0; $i -le $bytes.Length-$oldPath.Length; $i++) {
    $same=$true
    for($j=0;$j -lt $oldPath.Length;$j++) { if($bytes[$i+$j] -ne $oldPath[$j]) {$same=$false;break} }
    if($same) {$matches += $i}
}
if($matches.Count -ne 1) { throw 'Expected one embedded native save path; no copy created.' }
Copy-Item -LiteralPath $source -Destination $destination -Recurse
[Array]::Copy($newName,0,$bytes,4,$newName.Length)
[Array]::Copy($newPath,0,$bytes,$matches[0],$newPath.Length)
[IO.File]::WriteAllBytes((Join-Path $destination 'header.bin'),$bytes)
$baseline = Join-Path $root 'tools\fixtures\stage-03-baseline'
New-Item -ItemType Directory -Path $baseline -Force | Out-Null
foreach($name in @('heightmap.dds','collision.dta','header.bin')) {
    Copy-Item -LiteralPath (Join-Path $destination $name) -Destination (Join-Path $baseline $name)
}
$manifest = [ordered]@{source=$source;fixture=$destination;createdUtc=[DateTime]::UtcNow.ToString('o');headerDisplayNameOffset=4;headerSavePathOffset=$matches[0];baseline=@()}
foreach($name in @('heightmap.dds','collision.dta','header.bin')) {
    $manifest.baseline += [ordered]@{name=$name;sha256=(Get-FileHash -LiteralPath (Join-Path $baseline $name) -Algorithm SHA256).Hash.ToLowerInvariant()}
}
[IO.File]::WriteAllText((Join-Path $baseline 'manifest.json'),($manifest|ConvertTo-Json -Depth 5),[Text.UTF8Encoding]::new($false))
Write-Host "Created isolated fixture: $destination"
Write-Host 'Only the copied header display name and embedded save path changed. S01 is untouched.'
