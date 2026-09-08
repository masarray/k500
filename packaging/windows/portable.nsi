Unicode true
SetCompressor /SOLID lzma
RequestExecutionLevel user
SilentInstall silent
AutoCloseWindow true
ShowInstDetails nevershow

!include "FileFunc.nsh"

!ifndef APP_DIR
  !error "APP_DIR define is required"
!endif
!ifndef OUT_FILE
  !define OUT_FILE "SonKuPik-K500-Portable-Single.exe"
!endif
!ifndef APP_VERSION
  !define APP_VERSION "0.1.1"
!endif

Name "SonKuPik K500 Portable ${APP_VERSION}"
OutFile "${OUT_FILE}"

Section
  InitPluginsDir
  SetOutPath "$PLUGINSDIR\SonKuPik-K500"
  File /r "${APP_DIR}\*.*"

  # True single-file portable launcher:
  # - no installation
  # - no administrator rights
  # - no registry/start-menu/desktop writes from the wrapper
  # - Qt runtime is extracted only into NSIS' unique per-launch temp folder
  # - NSIS removes that temporary runtime after the app exits
  ${GetParameters} $R0
  ExecWait '"$PLUGINSDIR\SonKuPik-K500\SONKUPIK-STUDIO-Native-UI.exe" $R0' $0
  SetErrorLevel $0
SectionEnd
