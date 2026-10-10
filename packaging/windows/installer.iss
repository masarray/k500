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
// P3_EXPLICIT_SCOPE_MIGRATION_V1
// Per-user Setup normally refuses an existing machine-wide registration.
// Only the app-owned helper may bypass that collision guard, and only when all
// internal handoff markers are present. The helper validates the new install
// before it elevates the OLD uninstaller, so migration is reversible until then.
function IsExplicitMachineMigration: Boolean;
begin
  Result := (ExpandConstant('{param:MIGRATEFROMMACHINE|0}') = '1') and
            (ExpandConstant('{param:HELPERUPDATE|0}') = '1') and
            (ExpandConstant('{param:AUToupdate|0}') = '1');
end;

function InitializeSetup(): Boolean;
var
  MachineKey: String;
  MachineExists: Boolean;
begin
  MachineKey := 'Software\Microsoft\Windows\CurrentVersion\Uninstall\{8F568FE8-A747-4CD0-A727-5FE81A405500}_is1';
  MachineExists := RegKeyExists(HKLM64, MachineKey) or RegKeyExists(HKLM32, MachineKey);
  Result := (not MachineExists) or IsExplicitMachineMigration;
  if MachineExists and IsExplicitMachineMigration then
    Log('Explicit app-owned machine-to-user migration accepted; old machine install remains until helper health-check succeeds.')
  else if not Result then
  begin
    Log('Per-user install refused: existing machine-wide SonKuPik registration.');
    if not WizardSilent then
      MsgBox('An all-users SonKuPik K500 installation already exists. ' +
             'This per-user installer will not create a second copy. ' +
             'Use SonKuPik''s explicit migration action instead.', mbError, MB_OK);
  end;
end;

#else
// MIGRATE_LOCALAPPDATA_INSTALL_V1 / SAFE_CROSS_SCOPE_PREFLIGHT_V1
// Machine setup must not execute a current-user uninstaller from elevated
// context. A failed or partial cross-scope uninstall could leave duplicates.
// Only the existing app-owned helper may run an explicitly approved migration.
function OriginalUserRegistrationExists(const View: String; var ProbeError: String): Boolean;
var
  ExitCode: Integer;
  Args: String;
begin
  Result := False;
  Args := 'QUERY "HKCU\Software\Microsoft\Windows\CurrentVersion\Uninstall\{8F568FE8-A747-4CD0-A727-5FE81A405500}_is1" /reg:' + View;
  if not ExecAsOriginalUser(ExpandConstant('{sys}\reg.exe'), Args, '',
    SW_HIDE, ewWaitUntilTerminated, ExitCode) then
  begin
    ProbeError := 'Could not inspect original-user uninstall registration.';
    Exit;
  end;
  if ExitCode = 0 then
    Result := True
  else if ExitCode <> 1 then
    ProbeError := Format('Uninstall registry probe failed (view %s, code %d).',
      [View, ExitCode]);
end;

function LegacyPerUserInstallPresent(var ProbeError: String): Boolean;
var
  ExitCode: Integer;
  Args: String;
begin
  ProbeError := '';
  Result := OriginalUserRegistrationExists('64', ProbeError);
  if Result or (ProbeError <> '') then Exit;
  Result := OriginalUserRegistrationExists('32', ProbeError);
  if Result or (ProbeError <> '') then Exit;

  // Old unregistered per-user Inno layouts are detected without executing any
  // user-writable file or removing an application/preset directory.
  Args := '/D /C if not defined LOCALAPPDATA (exit /B 43) else if exist ' +
    '"%LOCALAPPDATA%\Programs\{#AppName}\unins000.exe" ' +
    '(exit /B 42) else (exit /B 0)';
  if not ExecAsOriginalUser(ExpandConstant('{cmd}'), Args, '',
    SW_HIDE, ewWaitUntilTerminated, ExitCode) then
  begin
    ProbeError := 'Could not inspect original-user LocalAppData.';
    Exit;
  end;
  if ExitCode = 42 then
    Result := True
  else if ExitCode <> 0 then
    ProbeError := Format('Legacy installation probe failed (code %d).', [ExitCode]);
end;

function PrepareToInstall(var NeedsRestart: Boolean): String;
var
  ProbeError: String;
begin
  Result := '';
  if LegacyPerUserInstallPresent(ProbeError) then
  begin
    Result := 'An existing per-user SonKuPik K500 installation was detected. ' +
      'Machine Setup stopped before changing either copy. Review the existing ' +
      'installation in Windows Installed Apps, or use an explicitly verified ' +
      'migration. Do not remove application directories manually.';
    Log('SAFE_CROSS_SCOPE_PREFLIGHT_V1: existing per-user installation; aborted.');
  end
  else if ProbeError <> '' then
  begin
    Result := 'Machine Setup could not safely inspect the current-user install. ' +
      'No changes were made. Details: ' + ProbeError;
    Log('SAFE_CROSS_SCOPE_PREFLIGHT_V1: unknown install state; aborted: ' + ProbeError);
  end;
end;

#endif
