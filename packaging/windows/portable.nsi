Unicode true
SetCompressor /SOLID lzma
RequestExecutionLevel user
SilentInstall silent
AutoCloseWindow true
ShowInstDetails nevershow

!ifndef APP_DIR
  !error "APP_DIR define is required"
!endif
!ifndef OUT_FILE
  !define OUT_FILE "SONKUPIK-STUDIO-Portable.exe"
!endif
!ifndef APP_VERSION
  !define APP_VERSION "0.1.1"
!endif

Name "SONKUPIK STUDIO Portable ${APP_VERSION}"
OutFile "${OUT_FILE}"

Section
  InitPluginsDir
  SetOutPath "$PLUGINSDIR\SONKUPIK-STUDIO"
  File /r "${APP_DIR}\*.*"

  # $PLUGINSDIR is a unique per-launch temporary folder managed by NSIS.
  # This keeps the portable build single-file on disk, requires no admin
  # privileges and removes extracted runtime files when the app exits.
  ExecWait '"$PLUGINSDIR\SONKUPIK-STUDIO\SONKUPIK-STUDIO-Native-UI.exe"' $0
  SetErrorLevel $0
SectionEnd
