Unicode true
SetCompressor /SOLID lzma
RequestExecutionLevel user

!ifndef APP_DIR
  !error "APP_DIR define is required"
!endif
!ifndef OUT_FILE
  !define OUT_FILE "SonKuPik-K500-Setup.exe"
!endif
!ifndef APP_VERSION
  !define APP_VERSION "0.1.1"
!endif

Name "SonKuPik K500 ${APP_VERSION}"
OutFile "${OUT_FILE}"
InstallDir "$LOCALAPPDATA\Programs\SonKuPik K500"
InstallDirRegKey HKCU "Software\MasArray\SonKuPik K500" "InstallDir"
ShowInstDetails show
ShowUninstDetails show

Page directory
Page instfiles
UninstPage uninstConfirm
UninstPage instfiles

Section "SonKuPik K500" SEC_MAIN
  SetShellVarContext current
  SetOutPath "$INSTDIR"
  File /r "${APP_DIR}\*.*"

  WriteRegStr HKCU "Software\MasArray\SonKuPik K500" "InstallDir" "$INSTDIR"
  WriteUninstaller "$INSTDIR\Uninstall.exe"

  CreateDirectory "$SMPROGRAMS\SonKuPik K500"
  CreateShortcut "$SMPROGRAMS\SonKuPik K500\SonKuPik K500.lnk" "$INSTDIR\SONKUPIK-STUDIO-Native-UI.exe"
  CreateShortcut "$SMPROGRAMS\SonKuPik K500\Uninstall.lnk" "$INSTDIR\Uninstall.exe"
  CreateShortcut "$DESKTOP\SonKuPik K500.lnk" "$INSTDIR\SONKUPIK-STUDIO-Native-UI.exe"

  WriteRegStr HKCU "Software\Microsoft\Windows\CurrentVersion\Uninstall\SonKuPik-K500" "DisplayName" "SonKuPik K500"
  WriteRegStr HKCU "Software\Microsoft\Windows\CurrentVersion\Uninstall\SonKuPik-K500" "DisplayVersion" "${APP_VERSION}"
  WriteRegStr HKCU "Software\Microsoft\Windows\CurrentVersion\Uninstall\SonKuPik-K500" "Publisher" "MasArray"
  WriteRegStr HKCU "Software\Microsoft\Windows\CurrentVersion\Uninstall\SonKuPik-K500" "UninstallString" '"$INSTDIR\Uninstall.exe"'
  WriteRegDWORD HKCU "Software\Microsoft\Windows\CurrentVersion\Uninstall\SonKuPik-K500" "NoModify" 1
  WriteRegDWORD HKCU "Software\Microsoft\Windows\CurrentVersion\Uninstall\SonKuPik-K500" "NoRepair" 1
SectionEnd

Section "Uninstall"
  SetShellVarContext current
  Delete "$DESKTOP\SonKuPik K500.lnk"
  RMDir /r "$SMPROGRAMS\SonKuPik K500"
  DeleteRegKey HKCU "Software\MasArray\SonKuPik K500"
  DeleteRegKey HKCU "Software\Microsoft\Windows\CurrentVersion\Uninstall\SonKuPik-K500"
  RMDir /r "$INSTDIR"
SectionEnd
