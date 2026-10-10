param()

$ErrorActionPreference = "Stop"

New-Item -ItemType Directory -Force -Path package,dist,build/packaging | Out-Null

Write-Host "==> Compile updater lifecycle fixtures"
& cl /nologo /std:c++20 /EHsc /DUNICODE /D_UNICODE /Fepackage/SonKuPik-K500.exe tests/windows/update_handoff_fixture.cpp /link /SUBSYSTEM:WINDOWS shell32.lib
if ($LASTEXITCODE -ne 0) { throw "Installer app fixture compilation failed" }

& cl /nologo /std:c++20 /EHsc /DUNICODE /D_UNICODE /DSONKUPIK_UPDATE_HELPER_CI=1 /Fepackage/SonKuPik-K500-Updater.exe packaging/windows/update_helper.cpp /link /SUBSYSTEM:WINDOWS shell32.lib bcrypt.lib user32.lib
if ($LASTEXITCODE -ne 0) { throw "Update helper compilation failed" }

& cl /nologo /std:c++20 /EHsc /DUNICODE /D_UNICODE /Febuild/packaging/P3-Fake-Installer.exe tests/windows/update_failure_installer_fixture.cpp /link /SUBSYSTEM:WINDOWS shell32.lib
if ($LASTEXITCODE -ne 0) { throw "Failure installer fixture compilation failed" }

& cl /nologo /std:c++20 /EHsc /DUNICODE /D_UNICODE /Febuild/packaging/P3-Fake-Machine-Uninstaller.exe tests/windows/update_migration_uninstaller_fixture.cpp /link /SUBSYSTEM:WINDOWS advapi32.lib
if ($LASTEXITCODE -ne 0) { throw "Migration uninstaller fixture compilation failed" }

$helper = Start-Process -FilePath "package/SonKuPik-K500-Updater.exe" -ArgumentList "--self-test" -Wait -PassThru
if ($helper.ExitCode -ne 0) { throw "Updater helper self-test failed: $($helper.ExitCode)" }

Write-Host "==> Verify helper hash fail-closed behavior"
$fixture = Join-Path (Resolve-Path package).Path "updater-hash-fixture.bin"
[IO.File]::WriteAllBytes($fixture, [byte[]](0..255))
$hash = (Get-FileHash $fixture -Algorithm SHA256).Hash.ToLowerInvariant()
$good = Start-Process -FilePath "package/SonKuPik-K500-Updater.exe" -ArgumentList @("--verify-self-test", ('"' + $fixture + '"'), $hash) -Wait -PassThru
if ($good.ExitCode -ne 0) { throw "Valid SHA-256 fixture failed: $($good.ExitCode)" }
[IO.File]::AppendAllText($fixture, "tamper")
$bad = Start-Process -FilePath "package/SonKuPik-K500-Updater.exe" -ArgumentList @("--verify-self-test", ('"' + $fixture + '"'), $hash) -Wait -PassThru
if ($bad.ExitCode -ne 41) { throw "Tampered SHA-256 fixture did not fail closed: $($bad.ExitCode)" }
Remove-Item $fixture -Force

Copy-Item LICENSE package/LICENSE -Force

Write-Host "==> Validate/generate installer brand assets"
python -m pip install --disable-pip-version-check --quiet pillow
if ($LASTEXITCODE -ne 0) { throw "Pillow install failed" }
python -m unittest discover -s tests -p test_installer_brand_assets.py
if ($LASTEXITCODE -ne 0) { throw "Installer branding tests failed" }
python packaging/windows/generate_brand_assets.py --icon package/SonKuPik-K500.ico --wizard build/packaging/wizard.bmp --wizard-small build/packaging/wizard-small.bmp
if ($LASTEXITCODE -ne 0) { throw "Installer branding generation failed" }

Write-Host "==> Install Inno Setup"
choco install innosetup -y --no-progress
if ($LASTEXITCODE -ne 0) { throw "Inno Setup installation failed" }
$iscc = (Get-Command ISCC.exe -ErrorAction SilentlyContinue).Source
if (-not $iscc) { $iscc = "${env:ProgramFiles(x86)}\Inno Setup 6\ISCC.exe" }
if (-not (Test-Path $iscc)) { throw "ISCC.exe not found" }

$cmake = Get-Content CMakeLists.txt -Raw
if ($cmake -notmatch 'project\(SonkupikStudioNative VERSION ([0-9]+\.[0-9]+\.[0-9]+) LANGUAGES CXX\)') {
    throw "Version not found"
}
$version = $Matches[1]
$env:SONKUPIK_APP_DIR = (Resolve-Path package).Path
$env:SONKUPIK_OUTPUT_DIR = (Resolve-Path dist).Path
$env:SONKUPIK_APP_ICON = (Resolve-Path package/SonKuPik-K500.ico).Path
$env:SONKUPIK_WIZARD_IMAGE = (Resolve-Path build/packaging/wizard.bmp).Path
$env:SONKUPIK_WIZARD_SMALL_IMAGE = (Resolve-Path build/packaging/wizard-small.bmp).Path

Write-Host "==> Compile machine/per-user installer scripts"
& $iscc "/DAppVersion=$version" packaging/windows/installer.iss
if ($LASTEXITCODE -ne 0) { throw "Machine installer script failed to compile" }
& $iscc "/DAppVersion=$version" "/DPerUser=1" packaging/windows/installer.iss
if ($LASTEXITCODE -ne 0) { throw "Per-user installer script failed to compile" }

$perUser = "dist/SonKuPik-K500-v$version-Windows-Setup-PerUser.exe"
if (-not (Test-Path $perUser)) { throw "Per-user installer output missing" }

Write-Host "==> Per-user install/uninstall smoke"
$dir = Join-Path $env:LOCALAPPDATA "Programs/SonKuPik-K500-CI-Smoke"
if (Test-Path $dir) { Remove-Item $dir -Recurse -Force }
$install = Start-Process -FilePath (Resolve-Path $perUser).Path -ArgumentList @("/VERYSILENT","/SUPPRESSMSGBOXES","/NORESTART","/DIR=$dir") -Wait -PassThru
if ($install.ExitCode -ne 0) { throw "Per-user installer smoke failed: $($install.ExitCode)" }
foreach ($name in @("SonKuPik-K500.exe","SonKuPik-K500-Updater.exe","unins000.exe")) {
    if (-not (Test-Path (Join-Path $dir $name))) { throw "Installed package missing $name" }
}
$uninstall = Start-Process -FilePath (Join-Path $dir "unins000.exe") -ArgumentList @("/VERYSILENT","/SUPPRESSMSGBOXES","/NORESTART") -Wait -PassThru
if ($uninstall.ExitCode -ne 0) { throw "Per-user uninstall smoke failed: $($uninstall.ExitCode)" }


Write-Host "==> Machine Setup refuses original-user HKCU registration"
$machine = (Resolve-Path "dist/SonKuPik-K500-v$version-Windows-Setup.exe").Path
$collisionKey = 'HKCU\Software\Microsoft\Windows\CurrentVersion\Uninstall\{8F568FE8-A747-4CD0-A727-5FE81A405500}_is1'
$blockedDir = Join-Path $env:RUNNER_TEMP 'K500-CrossScope-Machine-Test'
if (Test-Path $blockedDir) { throw "Pre-existing scope test directory; refusing to overwrite" }
function Assert-MachineSetupBlocked([string] $name) {
    $p = Start-Process -FilePath $machine -ArgumentList @(
        '/VERYSILENT', '/SUPPRESSMSGBOXES', '/NORESTART', ('/DIR="' + $blockedDir + '"')
    ) -Wait -PassThru
    if ($p.ExitCode -ne 7) { throw "$name: expected Inno pre-install refusal (7), got $($p.ExitCode)" }
    if (Test-Path (Join-Path $blockedDir 'SonKuPik-K500.exe')) {
        throw "$name: blocked Setup wrote application files"
    }
}
& reg.exe QUERY $collisionKey /reg:64 *> $null
if ($LASTEXITCODE -eq 0) { throw "HKCU collision fixture would overwrite a registration" }
& reg.exe ADD $collisionKey /v DisplayName /t REG_SZ /d 'K500 collision test' /f /reg:64 | Out-Null
if ($LASTEXITCODE -ne 0) { throw "Failed creating current-user collision registration" }
try {
    Assert-MachineSetupBlocked 'HKCU registration'
} finally {
    & reg.exe DELETE $collisionKey /f /reg:64 *> $null
}
& reg.exe QUERY $collisionKey /reg:64 *> $null
if ($LASTEXITCODE -eq 0) { throw "Failed clearing scope collision registration" }

Write-Host "==> Machine Setup refuses unregistered per-user legacy file without executing it"
$legacyDir = Join-Path $env:LOCALAPPDATA 'Programs\SonKuPik K500'
if (Test-Path $legacyDir) { throw "Refusing to alter pre-existing legacy directory" }
New-Item -ItemType Directory -Path $legacyDir | Out-Null
$sentinel = Join-Path $legacyDir 'unins000.exe'
try {
    [IO.File]::WriteAllText($sentinel, 'K500-CI-NEVER-EXECUTE')
    Assert-MachineSetupBlocked 'Legacy unregistered install'
    if ((Get-Content $sentinel -Raw) -ne 'K500-CI-NEVER-EXECUTE') {
        throw "Existing legacy uninstaller sentinel was modified"
    }
} finally {
    Remove-Item -LiteralPath $sentinel -Force -ErrorAction SilentlyContinue
    Remove-Item -LiteralPath $legacyDir -ErrorAction SilentlyContinue
}

Write-Host "K500 installer smoke PASS"
