param(
    [Parameter(Mandatory = $true)]
    [string]$Exe,

    [ValidateSet('performance', 'recovery')]
    [string]$Session = 'performance',

    [string]$OutputRoot = (Join-Path $PSScriptRoot '..\artifacts'),

    [switch]$RequirePerformancePass
)

$ErrorActionPreference = 'Stop'

$resolvedExe = (Resolve-Path $Exe).Path
$timestamp = Get-Date -Format 'yyyyMMdd-HHmmss'
$outputDir = Join-Path $OutputRoot "device-acceptance-$Session-$timestamp"
New-Item -ItemType Directory -Force -Path $outputDir | Out-Null
$outputDir = (Resolve-Path $outputDir).Path

$reportPath = Join-Path $outputDir 'device-performance.json'

Write-Host ''
Write-Host 'SonKuPik K500 final device qualification' -ForegroundColor Cyan
Write-Host "Session : $Session"
Write-Host "EXE     : $resolvedExe"
Write-Host "Report  : $reportPath"
Write-Host ''

if ($Session -eq 'performance') {
    Write-Host 'Before the FIRST CONNECT:' -ForegroundColor Yellow
    Write-Host '  1. Close the manufacturer K500 application.'
    Write-Host '  2. Connect K500 by USB HID.'
    Write-Host '  3. Open every SonKuPik section once, including System, to warm one-shot QML objects.'
    Write-Host '  4. Then CONNECT and execute the clean performance procedure from docs/FINAL_DEVICE_PERFORMANCE_ACCEPTANCE.md.'
} else {
    Write-Host 'Recovery session: intentional unplug/error events are expected.' -ForegroundColor Yellow
    Write-Host 'Use the manual failure/recovery acceptance rules; automatedPerformancePass is NOT the verdict for this session.'
}

Write-Host ''
Read-Host 'Press ENTER to launch the qualification build'

$arguments = @(
    '--device-perf',
    "--device-perf-report=$reportPath"
)

$process = Start-Process -FilePath $resolvedExe -ArgumentList $arguments -Wait -PassThru

if (-not (Test-Path $reportPath)) {
    throw "Qualification report was not created: $reportPath"
}

$report = Get-Content -Raw -Path $reportPath | ConvertFrom-Json
if ($report.schema -ne 'sonkupik-k500-device-performance-v1') {
    throw "Unexpected report schema: $($report.schema)"
}

Write-Host ''
Write-Host 'Qualification report summary' -ForegroundColor Cyan
Write-Host "  Git commit                : $($report.gitCommit)"
Write-Host "  App exit code             : $($process.ExitCode)"
Write-Host "  Connect successes         : $($report.connectToLive.successes) / $($report.connectToLive.attempts)"
Write-Host "  Connect worst             : $($report.connectToLive.worstMs) ms (target <= $($report.connectToLive.targetWorstMs) ms)"
Write-Host "  939-byte readback worst   : $($report.activeMemoryReadback939.worstMs) ms (target <= $($report.activeMemoryReadback939.targetWorstMs) ms)"
Write-Host "  Controller -> TX max      : $($report.controllerToTransportAcceptance.maxUs) us (target <= $($report.controllerToTransportAcceptance.targetMaxUs) us)"
Write-Host "  Event-loop max lag        : $($report.eventLoop.maxLagMs) ms"
Write-Host "  Event-loop ticks >250 ms  : $($report.eventLoop.ticksOver250Ms)"
Write-Host "  Unsupported paths         : $($report.traffic.unsupportedPaths)"
Write-Host "  Error log lines           : $($report.traffic.errorLogLines)"
Write-Host "  Private-byte delta        : $($report.runtime.deltaFromQualificationBaseline.privateBytes)"
Write-Host "  Handle delta              : $($report.runtime.deltaFromQualificationBaseline.handleCount)"
Write-Host "  Thread delta              : $($report.runtime.deltaFromQualificationBaseline.threadCount)"
Write-Host "  Automated performance PASS: $($report.gates.automatedPerformancePass)"
Write-Host ''
Write-Host "Full report: $reportPath" -ForegroundColor Green

if ($RequirePerformancePass -and $Session -eq 'performance' -and -not $report.gates.automatedPerformancePass) {
    Write-Error 'Clean performance gate did not pass. Review the JSON before merging the optimization stack.'
    exit 2
}

exit $process.ExitCode
