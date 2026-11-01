[CmdletBinding()]
param()

$ErrorActionPreference = 'Stop'
Set-StrictMode -Version Latest
$root = Split-Path $PSScriptRoot -Parent
$lock = Get-Content -LiteralPath (Join-Path $root 'dependencies.lock.json') -Raw | ConvertFrom-Json
$runtime = Join-Path $root 'tools\runtime\rml'

function Assert-Hash([string]$Path, [string]$Expected) {
    if (-not (Test-Path -LiteralPath $Path)) { throw "Missing dependency: $Path" }
    if ((Get-FileHash -LiteralPath $Path -Algorithm SHA256).Hash.ToLowerInvariant() -ne $Expected) {
        throw "Dependency hash mismatch; refusing to use $Path"
    }
}

function Ensure-Checkout([string]$Name, $Spec) {
    $checkout = Join-Path $root ('external\' + $Name)
    if (-not (Test-Path -LiteralPath $checkout)) {
        & git init $checkout | Out-Host
        if ($LASTEXITCODE -ne 0) { throw "Cannot initialize $Name checkout." }
        & git -C $checkout remote add origin $Spec.repository | Out-Host
        if ($LASTEXITCODE -ne 0) { throw "Cannot configure $Name source." }
        & git -C $checkout fetch --depth 1 origin $Spec.commit | Out-Host
        if ($LASTEXITCODE -ne 0) { throw "Cannot fetch pinned $Name commit." }
        & git -C $checkout checkout --detach FETCH_HEAD | Out-Host
        if ($LASTEXITCODE -ne 0) { throw "Cannot check out pinned $Name commit." }
    }
    $commit = & git -C $checkout rev-parse HEAD
    if ($LASTEXITCODE -ne 0 -or $commit -ne $Spec.commit) {
        throw "$Name checkout is not the pinned commit; refusing to overwrite it."
    }
    return $checkout
}

New-Item -ItemType Directory -Path (Join-Path $root 'external'),$runtime -Force | Out-Null
$sdk = Ensure-Checkout 'TesmioLoader' $lock.tesmioloader
foreach ($header in $lock.tesmioloader.sdk_headers.PSObject.Properties) {
    Assert-Hash (Join-Path $sdk ('src\' + $header.Name)) $header.Value
}
$rmlSource = Ensure-Checkout 'RepublicModLoader' $lock.rml
$null = Ensure-Checkout 'RepublicMP' $lock.republicmp
foreach ($file in $lock.rml.runtime_files.PSObject.Properties) {
    $source = Join-Path (Join-Path $rmlSource 'ModFiles') $file.Name
    $destination = Join-Path $runtime $file.Name
    Assert-Hash $source $file.Value
    if (Test-Path -LiteralPath $destination) {
        Assert-Hash $destination $file.Value
    } else {
        New-Item -ItemType Directory -Path (Split-Path $destination -Parent) -Force | Out-Null
        Copy-Item -LiteralPath $source -Destination $destination
    }
    Assert-Hash $destination $file.Value
}
Write-Host "Dependencies verified: RML $($lock.rml.version), with Tesmio API $($lock.tesmioloader.api_version) SDK compatibility."
Write-Host "RML runtime staged locally: $runtime"
Write-Host 'No standalone Tesmio runtime is required. The game has not been launched.'
