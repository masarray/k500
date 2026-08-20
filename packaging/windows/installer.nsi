Unicode true
SetCompressor /SOLID lzma
RequestExecutionLevel user

!ifndef APP_DIR
  !error "APP_DIR define is required"
!endif
!ifndef OUT_FILE
  !define OUT_FILE "SONKUPIK-STUDIO-Setup.exe"
!endif
!ifndef APP_VERSION
  !define APP_VERSION "0.1.1"
!endif

Name "SONKUPIK STUDIO ${APP_VERSION}"
OutFile "${OUT_FILE}"
InstallDir "$LOCALAPPDATA\Programs\SONKUPIK STUDIO"
InstallDirRegKey HKCU "Software\MasArray\SONKUPIK STUDIO" "InstallDir"
ShowInstDetails show
ShowUninstDetails show

Page directory
Page instfiles
UninstPage uninstConfirm
UninstPage instfiles

Section "SONKUPIK STUDIO" SEC_MAIN
  SetShellVarContext current
  SetOutPath "$INSTDIR"
  File /r "${APP_DIR}\*.*"

  WriteRegStr HKCU "Software\MasArray\SONKUPIK STUDIO" "InstallDir" "$INSTDIR"
  WriteUninstaller "$INSTDIR\Uninstall.exe"

  CreateDirectory "$SMPROGRAMS\SONKUPIK STUDIO"
  CreateShortcut "$SMPROGRAMS\SONKUPIK STUDIO\SONKUPIK STUDIO.lnk" "$INSTDIR\SONKUPIK-STUDIO-Native-UI.exe"
  CreateShortcut "$SMPROGRAMS\SONKUPIK STUDIO\Uninstall.lnk" "$INSTDIR\Uninstall.exe"
  CreateShortcut "$DESKTOP\SONKUPIK STUDIO.lnk" "$INSTDIR\SONKUPIK-STUDIO-Native-UI.exe"

  WriteRegStr HKCU "Software\Microsoft\Windows\CurrentVersion\Uninstall\SONKUPIK-STUDIO" "DisplayName" "SONKUPIK STUDIO"
  WriteRegStr HKCU "Software\Microsoft\Windows\CurrentVersion\Uninstall\SONKUPIK-STUDIO" "DisplayVersion" "${APP_VERSION}"
  WriteRegStr HKCU "Software\Microsoft\Windows\CurrentVersion\Uninstall\SONKUPIK-STUDIO" "Publisher" "MasArray"
  WriteRegStr HKCU "Software\Microsoft\Windows\CurrentVersion\Uninstall\SONKUPIK-STUDIO" "UninstallString" '"$INSTDIR\Uninstall.exe"'
  WriteRegDWORD HKCU "Software\Microsoft\Windows\CurrentVersion\Uninstall\SONKUPIK-STUDIO" "NoModify" 1
  WriteRegDWORD HKCU "Software\Microsoft\Windows\CurrentVersion\Uninstall\SONKUPIK-STUDIO" "NoRepair" 1
SectionEnd

Section "Uninstall"
  SetShellVarContext current
  Delete "$DESKTOP\SONKUPIK STUDIO.lnk"
  RMDir /r "$SMPROGRAMS\SONKUPIK STUDIO"
  DeleteRegKey HKCU "Software\MasArray\SONKUPIK STUDIO"
  DeleteRegKey HKCU "Software\Microsoft\Windows\CurrentVersion\Uninstall\SONKUPIK-STUDIO"
  RMDir /r "$INSTDIR"
SectionEnd
