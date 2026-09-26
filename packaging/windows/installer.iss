; SonKuPik K500 Windows installer
; Standard Inno Setup package. No custom self-extracting launcher is used.

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

; Per-user package is a separate asset. The legacy filename MUST remain machine-wide
; so already-published v1.0.3 updaters can never silently switch install scope.
#ifndef PerUser
  #define PerUser 0
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

; SMART_INSTALL_LAYOUT_V2
; Application/runtime belongs to Program Files. User .k500 content is created by
; the application under Documents\SonKuPik K500\Presets and is never uninstalled.
#if PerUser
; P2_INSTALL_SCOPE_V1 — separate per-user package, without UAC.
DefaultDirName={userpf}\{#AppName}
DefaultGroupName={#AppName}
PrivilegesRequired=lowest
#else
DefaultDirName={autopf}\{#AppName}
DefaultGroupName={#AppName}
PrivilegesRequired=admin
#endif
ArchitecturesAllowed=x64compatible
ArchitecturesInstallIn64BitMode=x64compatible

; Beginner-first wizard: use the one canonical machine location, avoid exposing
; path/program-group decisions that would later make automatic updates ambiguous,
; and keep only the familiar optional desktop-shortcut choice.
DisableDirPage=yes
DisableProgramGroupPage=yes
UsePreviousAppDir=no
AllowNoIcons=yes
DisableWelcomePage=no
DisableReadyPage=no

OutputDir={#OutputDir}
#if PerUser
OutputBaseFilename=SonKuPik-K500-v{#AppVersion}-Windows-Setup-PerUser
#else
OutputBaseFilename=SonKuPik-K500-v{#AppVersion}-Windows-Setup
#endif
SetupIconFile={#AppIcon}
WizardImageFile={#WizardImage}
WizardSmallImageFile={#WizardSmallImage}
LicenseFile={#AppDir}\LICENSE
Compression=lzma2/max
SolidCompression=yes
WizardStyle=modern
WizardSizePercent=110
SetupLogging=yes
CloseApplications=yes
; Auto-update relaunch is explicit in [Run], avoiding duplicate Restart Manager
; relaunches while keeping normal manual installs predictable.
RestartApplications=no
Uninstallable=yes
UninstallDisplayName={#AppName}
UninstallDisplayIcon={app}\SonKuPik-K500.ico
AppMutex=SonKuPikK500.K500.Native
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

[Icons]
Name: "{group}\{#AppName}"; Filename: "{app}\{#AppExeName}"; WorkingDir: "{app}"; IconFilename: "{app}\SonKuPik-K500.ico"
Name: "{autodesktop}\{#AppName}"; Filename: "{app}\{#AppExeName}"; WorkingDir: "{app}"; IconFilename: "{app}\SonKuPik-K500.ico"; Tasks: desktopicon

[Run]
; Normal interactive installation shows the usual Finish-page launch option.
Filename: "{app}\{#AppExeName}"; Description: "Launch {#AppName}"; WorkingDir: "{app}"; Flags: nowait postinstall skipifsilent; Check: not IsAutoUpdate
; In-app updates are already user-approved in SonKuPik. After verified silent
; replacement, reopen the newly installed Program Files binary automatically.
; Legacy app versions still rely on Inno to relaunch after /AUToupdate=1.
; The P1 coordinator uses /HELPERUPDATE=1 and performs its own health check
; before relaunch. Never run both relaunch paths at the same time.
Filename: "{app}\{#AppExeName}"; WorkingDir: "{app}"; Flags: nowait runasoriginaluser; Check: IsAutoUpdate and not IsHelperUpdate

[Code]
function IsAutoUpdate: Boolean;
begin
  Result := ExpandConstant('{param:AUToupdate|0}') = '1';
end;

function IsHelperUpdate: Boolean;
begin
  Result := ExpandConstant('{param:HELPERUPDATE|0}') = '1';
end;

#if PerUser
// A per-user install must never coexist silently with a registered machine-wide
// installation. Require an explicit migration outside this installer instead.
function InitializeSetup(): Boolean;
var
  MachineKey: String;
begin
  MachineKey := 'Software\Microsoft\Windows\CurrentVersion\Uninstall\{8F568FE8-A747-4CD0-A727-5FE81A405500}_is1';
  Result := not (RegKeyExists(HKLM64, MachineKey) or RegKeyExists(HKLM32, MachineKey));
  if not Result then
    MsgBox('An all-users SonKuPik K500 installation already exists. ' +
           'This per-user installer will not create a second copy. ' +
           'Use the existing application or perform a separately confirmed migration.', mbError, MB_OK);
end;
#else
procedure MigrateLegacyPerUserInstall;
var
  Cmd: String;
  Params: String;
  ResultCode: Integer;
begin
  { MIGRATE_LOCALAPPDATA_INSTALL_V1 }
  { v1.0.1 was a per-user install. An elevated machine-wide Setup can run under }
  { different credentials, so the Inno LocalAppData constant is not reliable here. }
  { Execute a tiny cmd under the ORIGINAL user and let that process expand its }
  { own %LOCALAPPDATA%. This never touches Documents, presets, or QSettings. }
  Cmd := ExpandConstant('{cmd}');
  Params := '/C if exist "%LOCALAPPDATA%\Programs\{#AppName}\unins000.exe" ' +
            'start "" /wait "%LOCALAPPDATA%\Programs\{#AppName}\unins000.exe" ' +
            '/VERYSILENT /SUPPRESSMSGBOXES /NORESTART';

  Log('Checking original user profile for legacy per-user SonKuPik K500 install.');
  if ExecAsOriginalUser(Cmd, Params, '', SW_HIDE, ewWaitUntilTerminated, ResultCode) then
    Log(Format('Legacy per-user migration command finished with code %d.', [ResultCode]))
  else
    Log('Legacy per-user migration command could not be started; canonical Program Files install will continue.');
end;

function PrepareToInstall(var NeedsRestart: Boolean): String;
begin
  { Run before new files/shortcuts are written, so a legacy uninstaller cannot }
  { remove the new common Start Menu/Desktop entries. }
  MigrateLegacyPerUserInstall;
  Result := '';
end;

#endif
