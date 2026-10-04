param(
    [Parameter(Mandatory = $true)][string]$Version,
    [Parameter(Mandatory = $true)][string]$UserSetup,
    [string]$PackageDir = "package",
    [string]$BuildDir = "build"
)

$ErrorActionPreference = "Stop"
Set-StrictMode -Version Latest

# RC_DEEP_UPDATER_ACCEPTANCE_V1
# Faithful migration of the expensive updater lifecycle scenarios that used to
# live in smart-install-update-guard.yml. This script is intentionally RC-only:
# normal PRs use tools/ci/installer_smoke.ps1, while an immutable RC must prove
# rollback, migration, persistence and stale-registration behavior using the
# exact per-user candidate installer that will be published.

function Start-BoundedProcess {
    param(
        [Parameter(Mandatory = $true)][string]$FilePath,
        [Parameter(Mandatory = $true)][object[]]$ArgumentList,
        [Parameter(Mandatory = $true)][string]$Label,
        [int]$TimeoutSeconds = 60
    )

    Write-Host "==> $Label"
    $process = Start-Process -FilePath $FilePath -ArgumentList $ArgumentList -PassThru
    if (-not $process.WaitForExit($TimeoutSeconds * 1000)) {
        Stop-Process -Id $process.Id -Force -ErrorAction SilentlyContinue
        Write-Host "Timeout diagnostics for $Label"
        Get-ChildItem $env:TEMP -Filter 'SonKuPik-K500-*.log' -File -ErrorAction SilentlyContinue |
            ForEach-Object {
                Write-Host ("--- " + $_.FullName + " ---")
                Get-Content $_.FullName -ErrorAction SilentlyContinue
            }
        throw "$Label exceeded $TimeoutSeconds seconds"
    }
    $process.Refresh()
    return $process
}

function Remove-RegKey {
    param([Parameter(Mandatory = $true)][string]$Key)
    & reg.exe DELETE $Key /f /reg:64 *> $null
}

$packageRoot = (Resolve-Path $PackageDir).Path
$buildRoot = (Resolve-Path $BuildDir).Path
$userSetupPath = (Resolve-Path $UserSetup).Path
$productionHelper = Join-Path $packageRoot 'SonKuPik-K500-Updater.exe'
$appFixture = Join-Path $packageRoot 'SonKuPik-K500.exe'
$fixtureRoot = Join-Path $buildRoot 'packaging'
New-Item -ItemType Directory -Force -Path $fixtureRoot | Out-Null

if (-not (Test-Path $productionHelper)) { throw "Production updater helper missing: $productionHelper" }
if (-not (Test-Path $appFixture)) { throw "Packaged application missing: $appFixture" }

Write-Host "==> Compile CI-only helper plus deep updater failure/migration fixtures"
# The production helper is already self-tested by the RC workflow. Migration
# needs an elevation-free CI twin compiled from the exact same source; the macro
# changes only runElevatedAndWait when SONKUPIK_UPDATE_HELPER_TEST=1 so hosted
# runners never need an interactive UAC desktop.
$helperPath = Join-Path $fixtureRoot 'SonKuPik-K500-Updater-CI.exe'
& cl /nologo /std:c++20 /EHsc /DUNICODE /D_UNICODE /DSONKUPIK_UPDATE_HELPER_CI=1 ("/Fe" + $helperPath) packaging/windows/update_helper.cpp /link /SUBSYSTEM:WINDOWS shell32.lib bcrypt.lib user32.lib
if ($LASTEXITCODE -ne 0) { throw "CI updater helper compilation failed" }

$helperSelfTest = Start-Process -FilePath $helperPath -ArgumentList '--self-test' -Wait -PassThru
if ($helperSelfTest.ExitCode -ne 0) { throw "CI updater helper self-test failed: $($helperSelfTest.ExitCode)" }

$fakeInstaller = Join-Path $fixtureRoot 'P3-Fake-Installer.exe'
$fakeMachineUninstaller = Join-Path $fixtureRoot 'P3-Fake-Machine-Uninstaller.exe'

& cl /nologo /std:c++20 /EHsc /DUNICODE /D_UNICODE ("/Fe" + $fakeInstaller) tests/windows/update_failure_installer_fixture.cpp /link /SUBSYSTEM:WINDOWS advapi32.lib shell32.lib
if ($LASTEXITCODE -ne 0) { throw "Failure installer fixture compilation failed" }

& cl /nologo /std:c++20 /EHsc /DUNICODE /D_UNICODE ("/Fe" + $fakeMachineUninstaller) tests/windows/update_migration_uninstaller_fixture.cpp /link /SUBSYSTEM:WINDOWS advapi32.lib
if ($LASTEXITCODE -ne 0) { throw "Migration uninstaller fixture compilation failed" }

$env:SONKUPIK_UPDATE_HELPER_TEST = '1'
$machineReg = 'HKLM\Software\Microsoft\Windows\CurrentVersion\Uninstall\{8F568FE8-A747-4CD0-A727-5FE81A405500}_is1'
$userReg = 'HKCU\Software\Microsoft\Windows\CurrentVersion\Uninstall\{8F568FE8-A747-4CD0-A727-5FE81A405500}_is1'
$healthMarker = Join-Path $env:TEMP 'SonKuPik-K500-CI-update-health.marker'
$restartMarker = Join-Path $env:TEMP 'SonKuPik-K500-CI-update-relaunch.marker'

Write-Host "==> Per-user package fails closed when machine registration exists"
& reg.exe ADD $machineReg /v DisplayName /t REG_SZ /d 'SonKuPik K500 RC machine collision fixture' /f /reg:64 | Out-Null
if ($LASTEXITCODE -ne 0) { throw "Could not create machine collision fixture" }
try {
    $blockedDir = Join-Path $env:LOCALAPPDATA 'Programs\SonKuPik-K500-RC-BLOCKED'
    if (Test-Path $blockedDir) { Remove-Item $blockedDir -Recurse -Force }
    $blocked = Start-Process -FilePath $userSetupPath -ArgumentList @(
        '/VERYSILENT','/SUPPRESSMSGBOXES','/NORESTART',"/DIR=$blockedDir"
    ) -Wait -PassThru
    if ($blocked.ExitCode -eq 0) { throw "Per-user installer accepted existing machine registration" }
    if (Test-Path (Join-Path $blockedDir 'SonKuPik-K500.exe')) {
        throw "Blocked per-user installer wrote application files"
    }
}
finally {
    Remove-RegKey $machineReg
}

Write-Host "==> Verified same-version per-user helper update"
$defaultDir = Join-Path $env:LOCALAPPDATA 'Programs\SonKuPik K500'
if (Test-Path $defaultDir) { Remove-Item $defaultDir -Recurse -Force }
$seed = Start-Process -FilePath $userSetupPath -ArgumentList @(
    '/VERYSILENT','/SUPPRESSMSGBOXES','/NORESTART'
) -Wait -PassThru
if ($seed.ExitCode -ne 0) { throw "Could not seed per-user helper update: $($seed.ExitCode)" }

Remove-Item $healthMarker,$restartMarker -Force -ErrorAction SilentlyContinue
$setupHash = (Get-FileHash $userSetupPath -Algorithm SHA256).Hash.ToLowerInvariant()
$updateLog = Join-Path $env:TEMP 'SonKuPik-K500-RC-update-handoff.log'
$updateBackup = Join-Path $env:TEMP 'SonKuPik-K500-RC-update-recovery'
Remove-Item $updateLog,("$updateLog.inno.log") -Force -ErrorAction SilentlyContinue
if (Test-Path $updateBackup) { Remove-Item $updateBackup -Recurse -Force }

$parent = Start-Process -FilePath 'pwsh.exe' -ArgumentList @('-NoProfile','-Command','Start-Sleep -Seconds 2') -PassThru
$updateArgs = @(
    '--parent-pid', "$($parent.Id)",
    '--setup', ('"' + $userSetupPath + '"'),
    '--app', ('"' + (Join-Path $defaultDir 'SonKuPik-K500.exe') + '"'),
    '--target-app', ('"' + (Join-Path $defaultDir 'SonKuPik-K500.exe') + '"'),
    '--version', $Version,
    '--previous-version', $Version,
    '--log', ('"' + $updateLog + '"'),
    '--sha256', $setupHash,
    '--scope', 'user',
    '--mode', 'update',
    '--backup', ('"' + $updateBackup + '"')
)
$handoff = Start-BoundedProcess -FilePath $helperPath -ArgumentList $updateArgs -Label 'verified per-user helper update'
if ($handoff.ExitCode -ne 0) { throw "Per-user update handoff failed: $($handoff.ExitCode)" }

$ready = $false
foreach ($attempt in 1..30) {
    if ((Test-Path $healthMarker) -and (Test-Path $restartMarker)) {
        $ready = $true
        break
    }
    Start-Sleep -Milliseconds 250
}
if (-not $ready) { throw "Updater did not health-check and relaunch installed application" }
if (-not ((Get-Content $updateLog -Raw).Contains('recovery snapshot released'))) {
    throw "Updater did not release successful recovery snapshot"
}
if (Test-Path $updateBackup) { throw "Successful update left stale recovery snapshot" }

$cleanup = Start-Process -FilePath (Join-Path $defaultDir 'unins000.exe') -ArgumentList @(
    '/VERYSILENT','/SUPPRESSMSGBOXES','/NORESTART'
) -Wait -PassThru
if ($cleanup.ExitCode -ne 0) { throw "Per-user helper update cleanup failed: $($cleanup.ExitCode)" }

Write-Host "==> Rollback restores application and uninstall metadata"
$rollbackDir = Join-Path $env:LOCALAPPDATA 'Programs\SonKuPik-K500-RC-ROLLBACK'
if (Test-Path $rollbackDir) { Remove-Item $rollbackDir -Recurse -Force }
$seedRollback = Start-Process -FilePath $userSetupPath -ArgumentList @(
    '/VERYSILENT','/SUPPRESSMSGBOXES','/NORESTART',"/DIR=$rollbackDir"
) -Wait -PassThru
if ($seedRollback.ExitCode -ne 0) { throw "Could not seed rollback fixture" }

$originalAppHash = (Get-FileHash (Join-Path $rollbackDir 'SonKuPik-K500.exe') -Algorithm SHA256).Hash
$originalRegistry = (& reg.exe QUERY $userReg /v DisplayVersion /reg:64 2>&1 | Out-String)
if ($LASTEXITCODE -ne 0 -or $originalRegistry -notmatch [regex]::Escape($Version)) {
    throw "Rollback fixture missing expected original registry version"
}
$fakeHash = (Get-FileHash $fakeInstaller -Algorithm SHA256).Hash.ToLowerInvariant()

foreach ($case in @(
    @{ Mode='nonzero'; Expected=23; Name='installer-nonzero' },
    @{ Mode='badhealth'; Expected=24; Name='failed-health-check' }
)) {
    Remove-Item $healthMarker,$restartMarker -Force -ErrorAction SilentlyContinue
    $env:SONKUPIK_P3_FAKE_INSTALL_MODE = $case.Mode
    $caseLog = Join-Path $env:TEMP ("SonKuPik-K500-RC-" + $case.Name + '.log')
    $caseBackup = Join-Path $env:TEMP ("SonKuPik-K500-RC-" + $case.Name + '-recovery')
    Remove-Item $caseLog -Force -ErrorAction SilentlyContinue
    if (Test-Path $caseBackup) { Remove-Item $caseBackup -Recurse -Force }

    $parent = Start-Process -FilePath 'pwsh.exe' -ArgumentList @('-NoProfile','-Command','Start-Sleep -Seconds 1') -PassThru
    $args = @(
        '--parent-pid', "$($parent.Id)",
        '--setup', ('"' + $fakeInstaller + '"'),
        '--app', ('"' + (Join-Path $rollbackDir 'SonKuPik-K500.exe') + '"'),
        '--target-app', ('"' + (Join-Path $rollbackDir 'SonKuPik-K500.exe') + '"'),
        '--version', $Version,
        '--previous-version', $Version,
        '--log', ('"' + $caseLog + '"'),
        '--sha256', $fakeHash,
        '--scope', 'user',
        '--mode', 'update',
        '--backup', ('"' + $caseBackup + '"')
    )
    $result = Start-BoundedProcess -FilePath $helperPath -ArgumentList $args -Label ("rollback " + $case.Name)
    if ($result.ExitCode -ne $case.Expected) {
        throw "$($case.Name) returned $($result.ExitCode), expected $($case.Expected)"
    }
    if ((Get-FileHash (Join-Path $rollbackDir 'SonKuPik-K500.exe') -Algorithm SHA256).Hash -ne $originalAppHash) {
        throw "$($case.Name) did not restore original application bytes"
    }
    $restoredRegistry = (& reg.exe QUERY $userReg /v DisplayVersion /reg:64 2>&1 | Out-String)
    if (($restoredRegistry -notmatch [regex]::Escape($Version)) -or
        ($restoredRegistry -match 'P3-BROKEN-REGISTRY')) {
        throw "$($case.Name) did not restore original uninstall metadata"
    }
    if (Test-Path $caseBackup) { throw "$($case.Name) left recovery snapshot after rollback" }
    if (-not ((Get-Content $caseLog -Raw).Contains('Rollback health check passed'))) {
        throw "$($case.Name) did not prove rollback health check"
    }
}
Remove-Item Env:SONKUPIK_P3_FAKE_INSTALL_MODE -ErrorAction SilentlyContinue

$rollbackCleanup = Start-Process -FilePath (Join-Path $rollbackDir 'unins000.exe') -ArgumentList @(
    '/VERYSILENT','/SUPPRESSMSGBOXES','/NORESTART'
) -Wait -PassThru
if ($rollbackCleanup.ExitCode -ne 0) { throw "Rollback fixture cleanup failed" }

Write-Host "==> Explicit machine-to-user migration preserves user-owned state"
if (Test-Path $defaultDir) { Remove-Item $defaultDir -Recurse -Force }
$machineDir = Join-Path $env:RUNNER_TEMP 'SonKuPik-K500-RC-MACHINE'
if (Test-Path $machineDir) { Remove-Item $machineDir -Recurse -Force }
New-Item -ItemType Directory -Force -Path $machineDir | Out-Null
Copy-Item $appFixture (Join-Path $machineDir 'SonKuPik-K500.exe')
Copy-Item $fakeMachineUninstaller (Join-Path $machineDir 'unins000.exe')
& reg.exe ADD $machineReg /v 'Inno Setup: App Path' /t REG_SZ /d $machineDir /f /reg:64 | Out-Null
if ($LASTEXITCODE -ne 0) { throw "Could not create machine migration registration fixture" }
& reg.exe ADD $machineReg /v DisplayVersion /t REG_SZ /d $Version /f /reg:64 | Out-Null

$presetMarker = Join-Path $env:USERPROFILE 'Documents\SonKuPik K500\Presets\RC-preserve.marker'
$cacheMarker = Join-Path $env:LOCALAPPDATA 'MasArray\SonKuPik K500\official-presets\RC-preserve.marker'
New-Item -ItemType Directory -Force -Path (Split-Path $presetMarker),(Split-Path $cacheMarker) | Out-Null
Set-Content $presetMarker 'preset-user-data-must-survive' -Encoding ascii
Set-Content $cacheMarker 'official-cache-must-survive' -Encoding ascii
& reg.exe ADD 'HKCU\Software\MasArray\SonKuPik K500' /v RCDeepPersistence /t REG_SZ /d 'qsettings-must-survive' /f /reg:64 | Out-Null

Remove-Item $healthMarker,$restartMarker -Force -ErrorAction SilentlyContinue
$migrationLog = Join-Path $env:TEMP 'SonKuPik-K500-RC-migration.log'
$migrationBackup = Join-Path $env:TEMP 'SonKuPik-K500-RC-migration-recovery'
Remove-Item $migrationLog -Force -ErrorAction SilentlyContinue
if (Test-Path $migrationBackup) { Remove-Item $migrationBackup -Recurse -Force }
$migrationHash = (Get-FileHash $userSetupPath -Algorithm SHA256).Hash.ToLowerInvariant()

$parent = Start-Process -FilePath 'pwsh.exe' -ArgumentList @('-NoProfile','-Command','Start-Sleep -Seconds 1') -PassThru
$migrationArgs = @(
    '--parent-pid', "$($parent.Id)",
    '--setup', ('"' + $userSetupPath + '"'),
    '--app', ('"' + (Join-Path $machineDir 'SonKuPik-K500.exe') + '"'),
    '--target-app', ('"' + (Join-Path $defaultDir 'SonKuPik-K500.exe') + '"'),
    '--version', $Version,
    '--previous-version', $Version,
    '--log', ('"' + $migrationLog + '"'),
    '--sha256', $migrationHash,
    '--scope', 'user',
    '--mode', 'migrate',
    '--backup', ('"' + $migrationBackup + '"')
)
$migration = Start-BoundedProcess -FilePath $helperPath -ArgumentList $migrationArgs -Label 'explicit machine-to-user migration'
if ($migration.ExitCode -ne 0) { throw "Machine-to-user migration failed: $($migration.ExitCode)" }
if (Test-Path (Join-Path $machineDir 'SonKuPik-K500.exe')) { throw "Migration left old machine application active" }
if (-not (Test-Path (Join-Path $defaultDir 'SonKuPik-K500.exe'))) { throw "Migration did not create per-user application" }

& reg.exe QUERY $machineReg /reg:64 *> $null
if ($LASTEXITCODE -eq 0) { throw "Migration left machine-wide uninstall registration" }
& reg.exe QUERY $userReg /reg:64 *> $null
if ($LASTEXITCODE -ne 0) { throw "Migration did not create per-user uninstall registration" }
if (Test-Path $migrationBackup) { throw "Successful migration left recovery snapshot" }

if ((Get-Content $presetMarker -Raw).Trim() -ne 'preset-user-data-must-survive') { throw "Migration changed user preset sentinel" }
if ((Get-Content $cacheMarker -Raw).Trim() -ne 'official-cache-must-survive') { throw "Migration changed official cache sentinel" }
$qsetting = (& reg.exe QUERY 'HKCU\Software\MasArray\SonKuPik K500' /v RCDeepPersistence /reg:64 2>&1 | Out-String)
if ($qsetting -notmatch 'qsettings-must-survive') { throw "Migration changed QSettings sentinel" }
if (-not ((Get-Content $migrationLog -Raw).Contains('migration verified'))) { throw "Migration helper did not log verified completion" }

$migrationCleanup = Start-Process -FilePath (Join-Path $defaultDir 'unins000.exe') -ArgumentList @(
    '/VERYSILENT','/SUPPRESSMSGBOXES','/NORESTART'
) -Wait -PassThru
if ($migrationCleanup.ExitCode -ne 0) { throw "Migrated per-user fixture cleanup failed" }
if (Test-Path $machineDir) { Remove-Item $machineDir -Recurse -Force }
Remove-Item $presetMarker,$cacheMarker -Force -ErrorAction SilentlyContinue
& reg.exe DELETE 'HKCU\Software\MasArray\SonKuPik K500' /v RCDeepPersistence /f /reg:64 *> $null

Write-Host "==> Stale-registration repair is narrow and fail-closed"
$stalePath = Join-Path $env:RUNNER_TEMP 'SonKuPik-K500-RC-STALE-MISSING'
if (Test-Path $stalePath) { Remove-Item $stalePath -Recurse -Force }
& reg.exe ADD $machineReg /v 'Inno Setup: App Path' /t REG_SZ /d $stalePath /f /reg:64 | Out-Null
$staleLog = Join-Path $env:TEMP 'SonKuPik-K500-RC-stale-registration.log'
$repair = Start-Process -FilePath $helperPath -ArgumentList @(
    '--remove-stale-registration','machine',
    '--expected-path',('"' + $stalePath + '"'),
    '--log',('"' + $staleLog + '"')
) -Wait -PassThru
if ($repair.ExitCode -ne 0) { throw "Verified stale registration cleanup failed: $($repair.ExitCode)" }
& reg.exe QUERY $machineReg /reg:64 *> $null
if ($LASTEXITCODE -eq 0) { throw "Verified stale machine registration was not removed" }

$activePath = Join-Path $env:RUNNER_TEMP 'SonKuPik-K500-RC-ACTIVE-REG'
if (Test-Path $activePath) { Remove-Item $activePath -Recurse -Force }
New-Item -ItemType Directory -Force -Path $activePath | Out-Null
Copy-Item $appFixture (Join-Path $activePath 'SonKuPik-K500.exe')
& reg.exe ADD $machineReg /v 'Inno Setup: App Path' /t REG_SZ /d $activePath /f /reg:64 | Out-Null
$refuse = Start-Process -FilePath $helperPath -ArgumentList @(
    '--remove-stale-registration','machine',
    '--expected-path',('"' + $activePath + '"'),
    '--log',('"' + $staleLog + '"')
) -Wait -PassThru
if ($refuse.ExitCode -ne 63) { throw "Active registration cleanup did not fail closed: $($refuse.ExitCode)" }
& reg.exe QUERY $machineReg /reg:64 *> $null
if ($LASTEXITCODE -ne 0) { throw "Active machine registration was incorrectly deleted" }
Remove-RegKey $machineReg
Remove-Item $activePath -Recurse -Force

Remove-Item Env:SONKUPIK_UPDATE_HELPER_TEST -ErrorAction SilentlyContinue
Write-Host "K500 RC deep updater lifecycle acceptance PASS"
