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

foreach ($arg in @("--font-self-test", "--protocol-self-test", "--engine-self-test", "--update-health-check=1.1.0")) {
    Invoke-Checked "$PackageDir/SonKuPik-K500.exe" @($arg) "Runtime $arg"
}

Write-Host "K500 consolidated Windows regression suite PASS"
