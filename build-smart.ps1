param(
    [switch]$Clean,
    [switch]$Run,
    [string]$QtRoot = ""
)

$ErrorActionPreference = "Stop"
$RepoRoot = $PSScriptRoot
Set-Location $RepoRoot

function Write-Step([string]$Message) {
    Write-Host "`n==> $Message" -ForegroundColor Cyan
}

function Test-QtRoot([string]$Path) {
    if ([string]::IsNullOrWhiteSpace($Path)) { return $false }
    return Test-Path (Join-Path $Path "lib\cmake\Qt6\Qt6Config.cmake")
}

function Resolve-QtRoot {
    param([string]$ExplicitRoot)

    $candidates = New-Object System.Collections.Generic.List[string]

    if ($ExplicitRoot) { $candidates.Add($ExplicitRoot) }
    if ($env:QT_ROOT) { $candidates.Add($env:QT_ROOT) }
    if ($env:CMAKE_PREFIX_PATH) {
        foreach ($entry in ($env:CMAKE_PREFIX_PATH -split ';')) {
            if ($entry) { $candidates.Add($entry) }
        }
    }

    # Fast path for the two laptops currently used for this project.
    $candidates.Add("C:\Qt\6.8.3\msvc2022_64")
    $candidates.Add("D:\Qt\6.8.3\msvc2022_64")

    foreach ($candidate in $candidates) {
        if (Test-QtRoot $candidate) {
            return (Resolve-Path $candidate).Path
        }
    }

    # Future-proof fallback: discover any Desktop MSVC Qt installation on C: or D:.
    $found = @()
    foreach ($base in @("C:\Qt", "D:\Qt")) {
        if (-not (Test-Path $base)) { continue }
        $found += Get-ChildItem $base -Directory -ErrorAction SilentlyContinue | ForEach-Object {
            $versionDir = $_
            Get-ChildItem $versionDir.FullName -Directory -Filter "msvc*_64" -ErrorAction SilentlyContinue | ForEach-Object {
                if (Test-QtRoot $_.FullName) {
                    [PSCustomObject]@{
                        Path = $_.FullName
                        Version = $versionDir.Name
                    }
                }
            }
        }
    }

    if ($found.Count -gt 0) {
        # Prefer highest lexical version; explicit/6.8.3 candidates above always win first.
        return ($found | Sort-Object Version -Descending | Select-Object -First 1).Path
    }

    throw "Qt Desktop MSVC tidak ditemukan. Dicari otomatis di C:\Qt dan D:\Qt. Anda juga bisa menjalankan: .\build-smart.ps1 -QtRoot 'X:\Qt\6.8.3\msvc2022_64'"
}

function Enable-Msvc {
    if (Get-Command cl.exe -ErrorAction SilentlyContinue) {
        return
    }

    $vswhereCandidates = @(
        "${env:ProgramFiles(x86)}\Microsoft Visual Studio\Installer\vswhere.exe",
        "$env:ProgramFiles\Microsoft Visual Studio\Installer\vswhere.exe"
    )
    $vswhere = $vswhereCandidates | Where-Object { $_ -and (Test-Path $_) } | Select-Object -First 1
    if (-not $vswhere) {
        throw "vswhere.exe tidak ditemukan. Install Visual Studio dengan Desktop development with C++."
    }

    $vs = & $vswhere -latest -products * -requires Microsoft.VisualStudio.Component.VC.Tools.x86.x64 -property installationPath
    if (-not $vs) {
        throw "MSVC C++ toolchain tidak ditemukan di Visual Studio."
    }

    $devShell = Join-Path $vs "Common7\Tools\Launch-VsDevShell.ps1"
    if (-not (Test-Path $devShell)) {
        throw "Visual Studio Developer Shell tidak ditemukan: $devShell"
    }

    Write-Step "Mengaktifkan MSVC dari $vs"
    & $devShell -Arch amd64 -HostArch amd64 | Out-Null
    Set-Location $RepoRoot

    if (-not (Get-Command cl.exe -ErrorAction SilentlyContinue)) {
        throw "Developer Shell aktif tetapi cl.exe tetap tidak ditemukan."
    }
}

function Resolve-Generator([string]$ResolvedQtRoot) {
    $ninja = Get-Command ninja.exe -ErrorAction SilentlyContinue
    if (-not $ninja) {
        $qtBase = Split-Path (Split-Path $ResolvedQtRoot -Parent) -Parent
        $ninjaCandidates = @(
            (Join-Path $qtBase "Tools\Ninja\ninja.exe"),
            "C:\Qt\Tools\Ninja\ninja.exe",
            "D:\Qt\Tools\Ninja\ninja.exe"
        )
        $ninjaPath = $ninjaCandidates | Where-Object { Test-Path $_ } | Select-Object -First 1
        if ($ninjaPath) {
            $env:PATH = "$(Split-Path $ninjaPath -Parent);$env:PATH"
            $ninja = Get-Command ninja.exe -ErrorAction SilentlyContinue
        }
    }

    if ($ninja) { return "Ninja" }
    if (Get-Command nmake.exe -ErrorAction SilentlyContinue) { return "NMake Makefiles" }

    throw "Tidak menemukan Ninja maupun NMake. Pastikan Visual Studio C++ tools terpasang."
}

if (-not (Get-Command cmake.exe -ErrorAction SilentlyContinue)) {
    throw "cmake.exe tidak ditemukan di PATH. Install CMake atau komponen CMake dari Visual Studio/Qt."
}

$QtRoot = Resolve-QtRoot -ExplicitRoot $QtRoot
Write-Step "Qt terdeteksi otomatis: $QtRoot"

Enable-Msvc
$Generator = Resolve-Generator -ResolvedQtRoot $QtRoot
Write-Step "Build generator: $Generator"

$env:CMAKE_PREFIX_PATH = $QtRoot
$env:Qt6_DIR = Join-Path $QtRoot "lib\cmake\Qt6"

$BuildDir = Join-Path $RepoRoot "build"
$PackageDir = Join-Path $RepoRoot "package"
$CacheFile = Join-Path $BuildDir "CMakeCache.txt"

$mustClean = $Clean.IsPresent
if ((Test-Path $CacheFile) -and -not $mustClean) {
    $cache = Get-Content $CacheFile -Raw
    if ($cache -notmatch [regex]::Escape("CMAKE_GENERATOR:INTERNAL=$Generator")) {
        Write-Host "Generator berubah; build cache akan dibersihkan." -ForegroundColor Yellow
        $mustClean = $true
    }

    $cachedQt = [regex]::Match($cache, '(?m)^Qt6_DIR:PATH=(.+)$')
    if ($cachedQt.Success -and ($cachedQt.Groups[1].Value.Trim() -notlike "$QtRoot*")) {
        Write-Host "Lokasi Qt berubah; build cache akan dibersihkan." -ForegroundColor Yellow
        $mustClean = $true
    }
}

if ($mustClean -and (Test-Path $BuildDir)) {
    Write-Step "Membersihkan build cache"
    Remove-Item -Recurse -Force $BuildDir
}

Write-Step "Configure"
& cmake -S $RepoRoot -B $BuildDir -G $Generator -DCMAKE_BUILD_TYPE=Release -DCMAKE_PREFIX_PATH="$QtRoot"
if ($LASTEXITCODE -ne 0) { throw "CMake configure gagal ($LASTEXITCODE)." }

Write-Step "Build Release"
& cmake --build $BuildDir --config Release
if ($LASTEXITCODE -ne 0) { throw "Build gagal ($LASTEXITCODE)." }

$ExePath = Join-Path $BuildDir "SONKUPIK-STUDIO-Native-UI.exe"
if (-not (Test-Path $ExePath)) {
    throw "Build selesai tetapi EXE tidak ditemukan: $ExePath"
}

Write-Step "Deploy Qt runtime"
if (Test-Path $PackageDir) { Remove-Item -Recurse -Force $PackageDir }
New-Item -ItemType Directory -Force $PackageDir | Out-Null
Copy-Item $ExePath $PackageDir -Force

$WinDeployQt = Join-Path $QtRoot "bin\windeployqt.exe"
if (-not (Test-Path $WinDeployQt)) {
    throw "windeployqt.exe tidak ditemukan: $WinDeployQt"
}

$PackageExe = Join-Path $PackageDir "SONKUPIK-STUDIO-Native-UI.exe"
& $WinDeployQt --release --qmldir (Join-Path $RepoRoot "qml") $PackageExe
if ($LASTEXITCODE -ne 0) { throw "windeployqt gagal ($LASTEXITCODE)." }

Write-Host "`n============================================" -ForegroundColor Green
Write-Host " SONKUPIK STUDIO Qt build berhasil" -ForegroundColor Green
Write-Host " Repo : $RepoRoot"
Write-Host " Qt   : $QtRoot"
Write-Host " Gen  : $Generator"
Write-Host " EXE  : $PackageExe"
Write-Host "============================================`n" -ForegroundColor Green

if ($Run) {
    Write-Step "Menjalankan aplikasi"
    Start-Process -FilePath $PackageExe -WorkingDirectory $PackageDir
}
