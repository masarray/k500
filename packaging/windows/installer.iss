; SonKuPik K500 Windows installer
; Standard Inno Setup package. No custom self-extracting launcher is used.
; v1.0.2+: machine-wide Program Files install; user presets live outside {app}.

#define AppName "SonKuPik K500"
#define AppExeName "SonKuPik-K500.exe"
#define AppPublisher "MasArray"
#define AppURL "https://github.com/masarray/k500"
#define AppDir GetEnv("SONKUPIK_APP_DIR")
#define OutputDir GetEnv("SONKUPIK_OUTPUT_DIR")
#define AppIcon GetEnv("SONKUPIK_APP_ICON")
#define WizardImage GetEnv("SONKUPIK_WIZARD_IMAGE")
#define WizardSmallImage GetEnv("SONKUPIK_WIZARD_SMALL_IMAGE")

#ifndef AppVersion
  #define AppVersion "0.0.0-dev"
#endif

#if AppDir == ""
  #error SONKUPIK_APP_DIR is required
#endif
#if OutputDir == ""
  #error SONKUPIK_OUTPUT_DIR is required
#endif
#if AppIcon == ""
  #error SONKUPIK_APP_ICON is required
#endif
#if WizardImage == ""
  #error SONKUPIK_WIZARD_IMAGE is required
#endif
#if WizardSmallImage == ""
  #error SONKUPIK_WIZARD_SMALL_IMAGE is required
#endif

[Setup]
AppId={{8F568FE8-A747-4CD0-A727-5FE81A405500}
AppName={#AppName}
AppVersion={#AppVersion}
AppVerName={#AppName} {#AppVersion}
AppPublisher={#AppPublisher}
AppPublisherURL={#AppURL}
AppSupportURL={#AppURL}/issues
AppUpdatesURL={#AppURL}/releases
DefaultDirName={autopf}\{#AppName}
DefaultGroupName={#AppName}
DisableProgramGroupPage=yes
DisableDirPage=auto
UsePreviousAppDir=no
PrivilegesRequired=admin
ArchitecturesAllowed=x64compatible
ArchitecturesInstallIn64BitMode=x64compatible
OutputDir={#OutputDir}
OutputBaseFilename=SonKuPik-K500-v{#AppVersion}-Windows-Setup
SetupIconFile={#AppIcon}
WizardImageFile={#WizardImage}
WizardSmallImageFile={#WizardSmallImage}
LicenseFile={#AppDir}\LICENSE
Compression=lzma2/max
SolidCompression=yes
WizardStyle=modern
WizardSizePercent=112
SetupLogging=yes
CloseApplications=yes
RestartApplications=no
UsePreviousTasks=yes
Uninstallable=yes
UninstallDisplayName={#AppName}
UninstallDisplayIcon={app}\SonKuPik-K500.ico
AppMutex=SonKuPikK500.K500.Native
SetupMutex=SonKuPikK500.K500.Setup
VersionInfoVersion={#AppVersion}
VersionInfoCompany={#AppPublisher}
VersionInfoDescription={#AppName} Installer
VersionInfoProductName={#AppName}
VersionInfoProductVersion={#AppVersion}
VersionInfoCopyright=Copyright © 2026 SonKuPik

[Tasks]
Name: "desktopicon"; Description: "Create a &desktop shortcut"; GroupDescription: "Additional shortcuts:"; Flags: unchecked

[Files]
Source: "{#AppDir}\*"; DestDir: "{app}"; Flags: ignoreversion recursesubdirs createallsubdirs

[Registry]
Root: HKLM; Subkey: "Software\MasArray\SonKuPik K500"; ValueType: string; ValueName: "InstallDir"; ValueData: "{app}"; Flags: uninsdeletekey
Root: HKLM; Subkey: "Software\MasArray\SonKuPik K500"; ValueType: string; ValueName: "Version"; ValueData: "{#AppVersion}"

[Icons]
Name: "{group}\{#AppName}"; Filename: "{app}\{#AppExeName}"; WorkingDir: "{app}"; IconFilename: "{app}\SonKuPik-K500.ico"
Name: "{autodesktop}\{#AppName}"; Filename: "{app}\{#AppExeName}"; WorkingDir: "{app}"; IconFilename: "{app}\SonKuPik-K500.ico"; Tasks: desktopicon

[Run]
; Normal interactive install: novice-friendly optional launch on Finish.
Filename: "{app}\{#AppExeName}"; Description: "Launch {#AppName}"; WorkingDir: "{app}"; Flags: nowait postinstall skipifsilent runasoriginaluser; Check: not IsAutoUpdate
; In-app updater: after verified silent install, relaunch under the original desktop user.
Filename: "{app}\{#AppExeName}"; WorkingDir: "{app}"; Flags: nowait runasoriginaluser; Check: IsAutoUpdate

[Code]
function HasCommandLineParameter(const Value: String): Boolean;
var
  I: Integer;
begin
  Result := False;
  for I := 1 to ParamCount do
  begin
    if CompareText(ParamStr(I), Value) = 0 then
    begin
      Result := True;
      Exit;
    end;
  end;
end;

function IsAutoUpdate: Boolean;
begin
  Result := HasCommandLineParameter('/AUTOUPDATE=1');
end;

function UpdateReadyMemo(Space, NewLine, MemoUserInfoInfo, MemoDirInfo,
  MemoTypeInfo, MemoComponentsInfo, MemoGroupInfo, MemoTasksInfo: String): String;
begin
  Result :=
    'SonKuPik K500 siap dipasang.' + NewLine + NewLine +
    'Aplikasi' + NewLine +
    '  ' + ExpandConstant('{app}') + NewLine + NewLine +
    'Preset pribadi' + NewLine +
    '  Documents\SonKuPik K500\Presets' + NewLine + NewLine +
    'Preset pribadi tidak dihapus ketika aplikasi di-uninstall.';
end;
