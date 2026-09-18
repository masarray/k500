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

# P4_LAUNCHER_PATH_QUOTING_V2
# Start-Process joins ArgumentList into one Windows command line. Quote the
# report value explicitly so paths such as "Software Buatanku" remain one argv
# token. Also refuse an already-running copy: otherwise -Wait can observe the
# wrong process lifetime and the evidence file becomes ambiguous.
$processName = [System.IO.Path]::GetFileNameWithoutExtension($resolvedExe)
$existing = Get-Process -Name $processName -ErrorAction SilentlyContinue
if ($existing) {
    throw "Close every existing $processName process before qualification. Running PID(s): $($existing.Id -join ', ')"
}

$argumentLine = "--device-perf --device-perf-report=`"$reportPath`""
$process = Start-Process -FilePath $resolvedExe -ArgumentList $argumentLine -PassThru

# The monitor writes an initial JSON as soon as the Qt event loop starts.
# Prove telemetry is alive before the operator spends time on hardware tests.
$telemetryDeadline = (Get-Date).AddSeconds(10)
while (-not (Test-Path $reportPath) -and -not $process.HasExited -and (Get-Date) -lt $telemetryDeadline) {
    Start-Sleep -Milliseconds 200
}
if (-not (Test-Path $reportPath)) {
    if (-not $process.HasExited) {
        Stop-Process -Id $process.Id -Force -ErrorAction SilentlyContinue
    }
    throw "Qualification telemetry did not start within 10 s. Expected report: $reportPath"
}

Write-Host "Telemetry active (PID $($process.Id)): $reportPath" -ForegroundColor Green
Write-Host 'Run the hardware procedure, then close SonKuPik to finalize the report.' -ForegroundColor Yellow
$process.WaitForExit()

if (-not (Test-Path $reportPath)) {
    throw "Qualification report disappeared or was not created: $reportPath"
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
