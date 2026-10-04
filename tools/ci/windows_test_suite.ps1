param(
    [string]$BuildDir = "build",
    [string]$PackageDir = "package"
)

$ErrorActionPreference = "Stop"

function Invoke-Checked {
    param(
        [Parameter(Mandatory=$true)][string]$Exe,
        [string[]]$Arguments = @(),
        [string]$Label = ""
    )
    if (-not (Test-Path $Exe)) {
        throw "Missing test executable: $Exe"
    }
    if ([string]::IsNullOrWhiteSpace($Label)) { $Label = (Split-Path $Exe -Leaf) }
    Write-Host "==> $Label"
    & $Exe @Arguments
    if ($LASTEXITCODE -ne 0) {
        throw "$Label failed with exit code $LASTEXITCODE"
    }
}

$donor = "tests/fixtures/donor-sample.k500"

# Compile once, test many: every hardware-free native regression runs from the
# same Release build. This replaces the old P0/P1/P2/P3/P4 per-workflow rebuilds.
Invoke-Checked "$BuildDir/k500_p0_perf_selftest.exe" -Label "P0 runtime telemetry"
Invoke-Checked "$BuildDir/k500_p1_state_selftest.exe" -Label "P1 canonical state"
Invoke-Checked "$BuildDir/k500_p2_scheduler_selftest.exe" -Label "P2 transaction scheduler"
Invoke-Checked "$BuildDir/k500_p2_transport_lifecycle_test.exe" -Label "P2 transport lifecycle"
Invoke-Checked "$BuildDir/k500_p3_win_resource_test.exe" -Label "P3 Windows RAII"
Invoke-Checked "$BuildDir/k500_p3_transport_shutdown_test.exe" -Label "P3 deterministic shutdown"
Invoke-Checked "$BuildDir/k500_protocol_selftest.exe" -Label "Protocol golden vectors"
Invoke-Checked "$BuildDir/k500_p2_selftest.exe" -Label "Preset protocol"
Invoke-Checked "$BuildDir/k500_p3_selftest.exe" -Label "Preset codec"
Invoke-Checked "$BuildDir/k500_p32_corpus_test.exe" @($donor) "Preset donor corpus"
Invoke-Checked "$BuildDir/k500_p34_edit_persistence_test.exe" @($donor) "Controlled edit persistence"
Invoke-Checked "$BuildDir/k500_p42_batch_test.exe" @($donor) "Batch preset library"
Invoke-Checked "$BuildDir/k500_donation_prompt_selftest.exe" -Label "Donation prompt calendar"
Invoke-Checked "$BuildDir/k500_update_selftest.exe" -Label "Updater metadata/integrity"

Write-Host "==> QML lint"
cmake --build $BuildDir --target all_qmllint
if ($LASTEXITCODE -ne 0) { throw "QML lint target failed" }

New-Item -ItemType Directory -Force -Path $PackageDir | Out-Null
Copy-Item "$BuildDir/SONKUPIK-STUDIO-Native-UI.exe" "$PackageDir/SonKuPik-K500.exe" -Force
Copy-Item "$BuildDir/SonKuPik-K500-Updater.exe" "$PackageDir/SonKuPik-K500-Updater.exe" -Force
if (Test-Path LICENSE) { Copy-Item LICENSE "$PackageDir/LICENSE" -Force }

Invoke-Checked "$PackageDir/SonKuPik-K500-Updater.exe" @("--self-test") "Updater helper"

Write-Host "==> Deploy Qt runtime"
windeployqt --release --qmldir qml "$PackageDir/SonKuPik-K500.exe"
if ($LASTEXITCODE -ne 0) { throw "windeployqt failed" }

foreach ($arg in @("--font-self-test", "--protocol-self-test", "--update-health-check=1.1.0")) {
    Invoke-Checked "$PackageDir/SonKuPik-K500.exe" @($arg) "Runtime $arg"
}

# FINAL_DEVICE_PERFORMANCE_SMOKE_V1 — keep the old hardware-free qualification
# signal, but run it against the already-built/deployed executable instead of
# paying for another Qt/MSVC workflow.
Write-Host "==> Final device performance smoke"
$perfReport = Join-Path (Resolve-Path $BuildDir).Path "final-device-performance-smoke.json"
Remove-Item $perfReport -Force -ErrorAction SilentlyContinue
$perfArgs = @(
    "--engine-self-test",
    "--device-perf",
    "--device-perf-report=$perfReport"
)
Invoke-Checked -Exe "$PackageDir/SonKuPik-K500.exe" -Arguments $perfArgs -Label "Engine + device performance smoke"
if (-not (Test-Path $perfReport)) { throw "Device performance smoke did not create JSON report" }
$perf = Get-Content -Raw $perfReport | ConvertFrom-Json
if ($perf.schema -ne "sonkupik-k500-device-performance-v1") { throw "Device performance report schema mismatch" }
if ([string]::IsNullOrWhiteSpace($perf.gitCommit) -or $perf.gitCommit -eq "unknown") { throw "Device performance report missing exact Git commit" }
if ($perf.bootstrapToEventLoopMs -lt 0) { throw "Bootstrap/event-loop timing was not recorded" }
if (-not $perf.runtime.startupPostQml.valid) { throw "Windows runtime snapshot was not captured" }
if ($perf.eventLoop.samples -lt 1) { throw "Event-loop sampler did not run" }
if ($perf.gates.scope -notmatch "Performance-only") { throw "Performance/manual acceptance scope separation missing" }

Write-Host "==> RC-only updater acceptance script syntax"
$parseErrors = $null
[void][System.Management.Automation.Language.Parser]::ParseFile(
    (Join-Path $PWD "tools/ci/updater_deep_acceptance.ps1"),
    [ref]$null,
    [ref]$parseErrors
)
if ($parseErrors.Count -ne 0) {
    $parseErrors | ForEach-Object { Write-Error $_.Message }
    throw "RC deep updater acceptance script has PowerShell syntax errors"
}

Write-Host "K500 consolidated Windows regression suite PASS"
