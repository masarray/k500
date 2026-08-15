@echo off
setlocal
cd /d "%~dp0"

where powershell.exe >nul 2>nul || (
  echo [SONKUPIK] PowerShell tidak ditemukan.
  pause
  exit /b 1
)

echo [SONKUPIK] Smart build launcher
echo [SONKUPIK] Repo: %CD%

echo.
powershell.exe -NoProfile -ExecutionPolicy Bypass -File "%~dp0build-smart.ps1" %*
set "ERR=%ERRORLEVEL%"

if not "%ERR%"=="0" (
  echo.
  echo [SONKUPIK] Build gagal dengan exit code %ERR%.
  pause
  exit /b %ERR%
)

echo.
echo [SONKUPIK] Selesai. Untuk build + langsung run:
echo   build-windows.cmd -Run
pause
