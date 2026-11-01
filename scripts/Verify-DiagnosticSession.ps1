[CmdletBinding()]
param(
    [Parameter(Mandatory=$true)]
    [ValidatePattern('^[A-Za-z0-9][A-Za-z0-9._-]{0,100}$')]
    [string]$SessionId,
    [switch]$RequireCleanExit
)

$ErrorActionPreference = 'Stop'
Set-StrictMode -Version Latest
$root = Split-Path $PSScriptRoot -Parent
$runtime = Join-Path $root 'tools\runtime\rml\rml'
$reportPath = Join-Path $runtime 'state\last_runtime_report.json'
$logPath = Join-Path $runtime 'logs\rml-runtime.log'
$report = Get-Content -LiteralPath $reportPath -Raw | ConvertFrom-Json
$baseline = Get-Content -LiteralPath (Join-Path $root 'docs\game-baseline.json') -Raw | ConvertFrom-Json
$lock = Get-Content -LiteralPath (Join-Path $root 'dependencies.lock.json') -Raw | ConvertFrom-Json
$expectedGame = @($baseline.binaries | Where-Object { $_.name -eq 'SOVIET64.exe' })
if ($expectedGame.Count -ne 1 -or $report.game.sha256 -ne $expectedGame[0].sha256 -or
    $report.runtimeVersion -ne $lock.rml.version) {
    throw 'Runtime report does not match the pinned game and RML baseline.'
}
if ($report.sessionId -ne $SessionId) {
    throw "Expected session $SessionId, found $($report.sessionId). Do not use a different session as test evidence."
}
if ($report.bootstrapStatus -ne 0) { throw 'RML bootstrap did not succeed.' }
$plugins = @($report.plugins)
if ($plugins.Count -ne 1 -or $plugins[0].id -ne 'earthworks.planning') {
    throw 'The session did not contain exactly the expected diagnostic plugin.'
}
$plugin = $plugins[0]
$dllPath = Join-Path $runtime 'plugins\EarthworksPlanning.dll'
$hash = (Get-FileHash -LiteralPath $dllPath -Algorithm SHA256).Hash.ToLowerInvariant()
if ($plugin.sha256 -ne $hash) { throw 'Runtime report refers to a different plugin build.' }
if (-not $plugin.loaded -or $plugin.failed -or $plugin.status -ne 'TESMIO ACTIVE' -or $plugin.legacyApi -ne 4) {
    throw 'The expected diagnostic lifecycle is not active in the report.'
}
if ($plugin.legacyHooks -ne 0 -or $plugin.legacyHookFailures -ne 0 -or
    $plugin.unmanagedInitChanges -ne 0 -or $plugin.unmanagedStartChanges -ne 0 -or
    $report.deepTest.lateChangeWindows -ne 0 -or $report.deepTest.lastAuditIssues -ne 0) {
    throw 'Stage 1 unexpectedly contains hook failures or executable changes; review the raw report.'
}
$log = Get-Content -LiteralPath $logPath -Raw
$launchHeaders = [regex]::Matches($log, '(?m)^.*\[Launcher\] launch profile:.*; session=([^\s]+)\s*$')
$headersForSession = @($launchHeaders | Where-Object { $_.Groups[1].Value -eq $SessionId })
if ($headersForSession.Count -ne 1) { throw 'Cannot uniquely locate this session in the launcher log.' }
$startIndex = $headersForSession[0].Index
$nextHeaders = @($launchHeaders | Where-Object { $_.Index -gt $startIndex } | Select-Object -First 1)
$endIndex = $log.Length
if ($nextHeaders.Count -gt 0) { $endIndex = $nextHeaders[0].Index }
$sessionLog = $log.Substring($startIndex, $endIndex - $startIndex)
$created = [regex]::Matches($sessionLog, 'created WRSR process suspended before game code execution: PID (\d+)')
if ($created.Count -ne 1) { throw 'Cannot uniquely identify the game process created for this session.' }
$sessionProcessId = $created[0].Groups[1].Value
$exitPattern = 'WRSR process exit: PID ' + [regex]::Escape($sessionProcessId) + ' code=0x([0-9A-Fa-f]+)\b'
$exitMatch = [regex]::Match($sessionLog, $exitPattern)
$cleanExit = $exitMatch.Success -and ([Convert]::ToUInt32($exitMatch.Groups[1].Value, 16) -eq 0)
if ($RequireCleanExit -and -not $cleanExit) { throw 'A normal exit for this session is not recorded yet.' }
$summary = [ordered]@{
    session_id=$SessionId
    game_process_id=[int]$sessionProcessId
    verified_utc=[DateTime]::UtcNow.ToString('o')
    plugin_sha256=$hash
    runtime_version=$report.runtimeVersion
    game_version=$report.game.version
    bootstrap_verified=$true
    diagnostic_active=$true
    plugin_hooks=0
    hook_failures=0
    audit_issues=0
    clean_exit_recorded=$cleanExit
    world_checks='Must be recorded from the actual in-game observations in STAGE_REVIEWS.md.'
    user_acceptance='Pending; this script does not accept a stage.'
}
$evidence = Join-Path $root ('docs\evidence\stage-01\' + $SessionId)
New-Item -ItemType Directory -Path $evidence -Force | Out-Null
Copy-Item -LiteralPath $reportPath -Destination (Join-Path $evidence 'runtime-report.json') -Force
Copy-Item -LiteralPath $logPath -Destination (Join-Path $evidence 'runtime.log') -Force
$json = $summary | ConvertTo-Json -Depth 5
[IO.File]::WriteAllText((Join-Path $evidence 'verification.json'), $json, [Text.UTF8Encoding]::new($false))
Write-Output $json
