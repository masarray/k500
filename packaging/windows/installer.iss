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
#ifndef SmartInstaller
  #define SmartInstaller 0
#endif
#if SmartInstaller
  #if PerUser
    #error SmartInstaller and PerUser outputs are mutually exclusive
  #endif
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
; Legacy machine backend uses Program Files, never as the public Smart default.
; The application creates user-owned .k500 presets under Documents; installers
; do not remove them.
#if SmartInstaller
; NATIVE_SMART_INSTALLER_V1 / PUBLIC_LOCALAPPDATA_ONLY_SMART_V1
; Public Smart Installer always targets %LOCALAPPDATA%\Programs\SonKuPik K500.
; No ALLUSERS choice, privilege override, or prior machine-scope inheritance.
 ; Existing verified machine and per-user installs can be reconciled by the
; app-owned transactional migration helper. Unidentified data stays untouched.
DefaultDirName={userpf}\{#AppName}
DefaultGroupName={#AppName}
PrivilegesRequired=lowest
UsePreviousPrivileges=no
#elif PerUser
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

; Beginner-first wizard: use a canonical scope-specific location, avoid exposing
; path/program-group decisions that would later make automatic updates ambiguous,
; and keep only the familiar optional desktop-shortcut choice.
DisableDirPage=yes
DisableProgramGroupPage=yes
UsePreviousAppDir=no
AllowNoIcons=yes
DisableWelcomePage=no
DisableReadyPage=no

OutputDir={#OutputDir}
#if SmartInstaller
OutputBaseFilename=SonKuPik-K500-v{#AppVersion}-Windows-Smart-Installer
#elif PerUser
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
#if SmartInstaller
; Solid compression: extract the small migration helper first, never decompress
; the entire full-app payload merely to show the migration prompt.
Source: "{#AppDir}\SonKuPik-K500-Updater.exe"; Flags: dontcopy
#endif
Source: "{#AppDir}\*"; DestDir: "{app}"; Flags: ignoreversion recursesubdirs createallsubdirs

[Icons]
Name: "{group}\{#AppName}"; Filename: "{app}\{#AppExeName}"; WorkingDir: "{app}"; IconFilename: "{app}\SonKuPik-K500.ico"
Name: "{autodesktop}\{#AppName}"; Filename: "{app}\{#AppExeName}"; WorkingDir: "{app}"; IconFilename: "{app}\SonKuPik-K500.ico"; Tasks: desktopicon

[Run]
; Normal interactive installation shows the usual Finish-page launch option.
Filename: "{app}\{#AppExeName}"; Description: "Launch {#AppName}"; WorkingDir: "{app}"; Flags: nowait postinstall skipifsilent; Check: not IsAutoUpdate
; In-app updates are already user-approved in SonKuPik. After verified silent
; replacement, reopen the newly installed binary in its registered scope.
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
// Per-user backend has its own existing scope guard below.
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
    ProbeError := Format('Uninstall registry probe failed (view %s, code %d).', [View, ExitCode]);
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

#endif

#if SmartInstaller
// Keep the same AppId as legacy installers. The prior scope is picked by
// Inno Setup itself; an explicit opposite-scope CLI override cannot bypass
// these pre-install collision guards.
function SmartMachineRegistrationPresent: Boolean;
var
  Key: String;
begin
  Key := 'Software\Microsoft\Windows\CurrentVersion\Uninstall\{8F568FE8-A747-4CD0-A727-5FE81A405500}_is1';
  Result := RegKeyExists(HKLM64, Key) or RegKeyExists(HKLM32, Key);
end;

// SMART_NATIVE_MIGRATION_WIZARD_V2 — a previously installed, registered
// per-user copy is a normal upgrade target, not "unknown user data". Only a
// canonical matching HKCU registration with intact EXE/uninstaller is trusted.
// Unregistered or incomplete directories are never overwritten or deleted.
function SmartUserDestinationReusable(var Reason: String): Boolean;
var
  Key: String;
  UserDir: String;
  Registered: String;
  HasRegistration: Boolean;
begin
  Result := False;
  Reason := '';
  Key := 'Software\Microsoft\Windows\CurrentVersion\Uninstall\{8F568FE8-A747-4CD0-A727-5FE81A405500}_is1';
  UserDir := AddBackslash(ExpandConstant('{userpf}')) + '{#AppName}';
  HasRegistration := RegKeyExists(HKCU64, Key) or RegKeyExists(HKCU32, Key);
  if not HasRegistration and not DirExists(UserDir) then
  begin
    Result := True;  // A clean per-user destination.
    Exit;
  end;
  if not HasRegistration then
  begin
    Reason := 'We found an older K500 folder that Windows cannot verify. Your files were not changed. Please contact SonKuPik support.';
    Exit;
  end;
  Registered := '';
  if not RegQueryStringValue(HKCU64, Key, 'Inno Setup: App Path', Registered) or
     (CompareText(AddBackslash(Registered), AddBackslash(UserDir)) <> 0) or
     not FileExists(AddBackslash(UserDir) + '{#AppExeName}') or
     not FileExists(AddBackslash(UserDir) + 'unins000.exe') then
  begin
    Reason := 'Windows could not verify the existing K500 installation. Your files were kept. Please contact SonKuPik support.';
    Exit;
  end;
  // Target is a real, registered canonical per-user installation; helper
  // independently rechecks this and snapshots it before replacing anything.
  Result := True;
end;

// SMART_VERIFIED_MACHINE_TO_USER_HANDOFF_V1
// This entrypoint never uninstalls or deletes Program Files itself. It only
// stages the exact bundled updater helper under the current user's temp dir.
// The helper verifies THIS setup's SHA-256, snapshots the registered old
// program + HKLM uninstall record, performs a validated per-user install,
// then asks for one legitimate UAC to remove the old machine installation.
function SmartMigrationHelperAuthorized: Boolean;
begin
  Result := (ExpandConstant('{param:MIGRATEFROMMACHINE|0}') = '1') and
            (ExpandConstant('{param:HELPERUPDATE|0}') = '1') and
            (ExpandConstant('{param:AUToupdate|0}') = '1');
end;

function GetCurrentProcessId: Cardinal;
  external 'GetCurrentProcessId@kernel32.dll stdcall';

function NumericVersionOnly(const Value: String): Boolean;
var
  I: Integer;
begin
  Result := Value <> '';
  for I := 1 to Length(Value) do
    if not ((Value[I] >= '0') and (Value[I] <= '9') or (Value[I] = '.')) then
      Result := False;
end;

function BeginSmartMigration(var Reason: String): Boolean;
var
  Key: String;
  OldDir: String;
  Version: String;
  MachineRoot: String;
  TargetDir: String;
  SetupPath: String;
  SetupHash: String;
  StageDir: String;
  StagedHelper: String;
  Params: String;
  ExitCode: Integer;
begin
  Result := False;
  Reason := '';
  Key := 'Software\Microsoft\Windows\CurrentVersion\Uninstall\{8F568FE8-A747-4CD0-A727-5FE81A405500}_is1';

  // All trust inputs must come from an exact 64-bit Machine registry entry.
  // No guessed paths, broad registry cleanup, or unsupported dual installs.
  // Unknown 32-bit parallel machine registration makes automatic migration
  // ambiguous. Never leave an orphaned Windows Installed Apps record.
  if RegKeyExists(HKLM32, Key) then
  begin
    Reason := 'Multiple Windows machine registrations exist. Automatic migration was blocked before changes.';
    Exit;
  end;
  if not RegQueryStringValue(HKLM64, Key, 'Inno Setup: App Path', OldDir) or
     not RegQueryStringValue(HKLM64, Key, 'DisplayVersion', Version) or
     not NumericVersionOnly(Version) then
  begin
    Reason := 'Previous machine installation cannot be verified. Nothing changed.';
    Exit;
  end;
  MachineRoot := AddBackslash(ExpandConstant('{commonpf}')) + '{#AppName}';
  if (CompareText(AddBackslash(OldDir), AddBackslash(MachineRoot)) <> 0) or
     not FileExists(AddBackslash(OldDir) + '{#AppExeName}') or
     not FileExists(AddBackslash(OldDir) + 'unins000.exe') then
  begin
    Reason := 'Existing Program Files location or uninstaller is not canonical. No changes made.';
    Exit;
  end;

  TargetDir := AddBackslash(ExpandConstant('{userpf}')) + '{#AppName}';
  // Do not refuse a legitimate canonical per-user installation: preserve
  // it transactionally and upgrade the same target. Unknown data still blocks.
  if not SmartUserDestinationReusable(Reason) then Exit;

  SetupPath := ExpandConstant('{srcexe}');
  SetupHash := Lowercase(GetSHA256OfFile(SetupPath));
  if Length(SetupHash) <> 64 then
  begin
    Reason := 'Could not authenticate the exact Smart Setup bytes.';
    Exit;
  end;

  StageDir := AddBackslash(GetTempDir) + 'SonKuPik-K500-Migration-' +
              IntToStr(GetCurrentProcessId);
  // A reused process ID cannot overwrite a previous recovery/log directory.
  if DirExists(StageDir) or not CreateDir(StageDir) then
  begin
    Reason := 'A migration work directory already exists or cannot be created.';
    Exit;
  end;
  ExtractTemporaryFile('SonKuPik-K500-Updater.exe');
  StagedHelper := AddBackslash(StageDir) + 'SonKuPik-K500-Updater.exe';
  if not FileCopy(AddBackslash(ExpandConstant('{tmp}')) +
                  'SonKuPik-K500-Updater.exe', StagedHelper, True) then
  begin
    Reason := 'Bundled verified migration helper could not be staged.';
    Exit;
  end;

  Params := '--parent-pid ' + IntToStr(GetCurrentProcessId) +
    ' --setup "' + SetupPath + '"' +
    ' --app "' + AddBackslash(OldDir) + '{#AppExeName}"' +
    ' --target-app "' + AddBackslash(TargetDir) + '{#AppExeName}"' +
    ' --version "{#AppVersion}"' +
    ' --previous-version "' + Version + '"' +
    ' --log "' + AddBackslash(StageDir) + 'migration.log"' +
    ' --sha256 ' + SetupHash +
    ' --scope user --mode migrate' +
    ' --backup "' + AddBackslash(StageDir) + 'Recovery"';

  Result := Exec(StagedHelper, Params, '', SW_SHOWNORMAL, ewNoWait, ExitCode);
  if not Result then
    Reason := 'Could not start verified migration helper (Windows code ' +
              IntToStr(ExitCode) + ').';
end;

// SMART_NATIVE_MIGRATION_WIZARD_V2 — the normal Inno Setup window is the
// only setup UI. No pre-wizard Yes/No/error message boxes.
var
  SmartMigrationPage: TWizardPage;
  SmartMigrationInfo: TNewStaticText;
  SmartMigrationDetail: TNewStaticText;
  SmartMigrationError: TNewStaticText;
  SmartMigrationStarted: Boolean;
  SmartMigrationResultView: Boolean;

function InitializeSetup(): Boolean;
begin
  Result := True;
end;

procedure InitializeWizard();
begin
  SmartMigrationStarted := False;
  SmartMigrationResultView := ExpandConstant('{param:MIGRATIONRESULT|0}') = '1';
  if SmartMigrationResultView then
  begin
    // The verified helper may need to return an unsuccessful outcome AFTER
    // the first wizard has exited. Reopen THIS Inno UI, not a Win32 MsgBox.
    SmartMigrationPage := CreateCustomPage(wpWelcome,
      'K500 was not updated', 'Your existing application and presets are protected.');
    SmartMigrationInfo := TNewStaticText.Create(SmartMigrationPage);
    SmartMigrationInfo.Parent := SmartMigrationPage.Surface;
    SmartMigrationInfo.Left := ScaleX(12);
    SmartMigrationInfo.Top := ScaleY(25);
    SmartMigrationInfo.Width := SmartMigrationPage.SurfaceWidth - ScaleX(24);
    SmartMigrationInfo.WordWrap := True;
    SmartMigrationInfo.Font.Style := [fsBold];
    SmartMigrationInfo.Caption := 'We could not complete the update safely.';
    WizardForm.AdjustLabelHeight(SmartMigrationInfo);
    SmartMigrationDetail := TNewStaticText.Create(SmartMigrationPage);
    SmartMigrationDetail.Parent := SmartMigrationPage.Surface;
    SmartMigrationDetail.Left := SmartMigrationInfo.Left;
    SmartMigrationDetail.Top := SmartMigrationInfo.Top + SmartMigrationInfo.Height + ScaleY(20);
    SmartMigrationDetail.Width := SmartMigrationInfo.Width;
    SmartMigrationDetail.WordWrap := True;
    SmartMigrationDetail.Caption :=
      'K500 kept the previous installation instead of risking your presets.' +
      '' + #13#10#13#10 +
      'Please close this window and contact SonKuPik support if the problem continues.';
    WizardForm.AdjustLabelHeight(SmartMigrationDetail);
    Exit;
  end;
  if SmartMigrationHelperAuthorized or WizardSilent or
     not SmartMachineRegistrationPresent then Exit;

  SmartMigrationPage := CreateCustomPage(wpSelectTasks,
    'Updating SonKuPik K500',
    'Your presets and settings will be kept.');

  SmartMigrationInfo := TNewStaticText.Create(SmartMigrationPage);
  SmartMigrationInfo.Parent := SmartMigrationPage.Surface;
  SmartMigrationInfo.Left := ScaleX(12);
  SmartMigrationInfo.Top := ScaleY(22);
  SmartMigrationInfo.Width := SmartMigrationPage.SurfaceWidth - ScaleX(24);
  SmartMigrationInfo.WordWrap := True;
  SmartMigrationInfo.Caption :=
    'K500 is already on this PC. Setup will update it in your Windows account ' +
    'so future updates are simpler.';
  SmartMigrationInfo.Font.Style := [fsBold];
  WizardForm.AdjustLabelHeight(SmartMigrationInfo);

  SmartMigrationDetail := TNewStaticText.Create(SmartMigrationPage);
  SmartMigrationDetail.Parent := SmartMigrationPage.Surface;
  SmartMigrationDetail.Left := SmartMigrationInfo.Left;
  SmartMigrationDetail.Top := SmartMigrationInfo.Top + SmartMigrationInfo.Height + ScaleY(18);
  SmartMigrationDetail.Width := SmartMigrationInfo.Width;
  SmartMigrationDetail.WordWrap := True;
  SmartMigrationDetail.Caption :=
    'Your existing presets stay in place.' + #13#10#13#10 +
    'Windows may ask for permission once to remove the old version.' + #13#10#13#10 +
    'Click Next. K500 will open automatically when the update is ready.';
  WizardForm.AdjustLabelHeight(SmartMigrationDetail);

  SmartMigrationError := TNewStaticText.Create(SmartMigrationPage);
  SmartMigrationError.Parent := SmartMigrationPage.Surface;
  SmartMigrationError.Left := SmartMigrationInfo.Left;
  SmartMigrationError.Top := SmartMigrationDetail.Top + SmartMigrationDetail.Height + ScaleY(14);
  SmartMigrationError.Width := SmartMigrationInfo.Width;
  SmartMigrationError.WordWrap := True;
  SmartMigrationError.Font.Color := clRed;
  SmartMigrationError.Visible := False;
end;

function NextButtonClick(CurPageID: Integer): Boolean;
var
  Reason: String;
begin
  Result := True;
  if (not Assigned(SmartMigrationPage)) or (CurPageID <> SmartMigrationPage.ID) then Exit;
  Result := False;
  if SmartMigrationResultView then
  begin
    SmartMigrationStarted := True;
    WizardForm.Close;
    Exit;
  end;
  Reason := '';
  // Do not rely on PascalScript boolean short-circuit behavior: if the
  // local directory is unverified, migration must NEVER be launched.
  if not SmartUserDestinationReusable(Reason) then
  begin
    SmartMigrationError.Caption := Reason;
    WizardForm.AdjustLabelHeight(SmartMigrationError);
    SmartMigrationError.Visible := True;
    Exit;
  end;
  if not BeginSmartMigration(Reason) then
  begin
    // The exact diagnostic is recorded in Setup's log, not a noisy modal.
    SmartMigrationError.Caption := Reason;
    WizardForm.AdjustLabelHeight(SmartMigrationError);
    SmartMigrationError.Visible := True;
    Exit;
  end;
  SmartMigrationStarted := True;
  // The signed/hashed helper must wait until Setup releases its installer
  // handle before installing, verifying and cleaning the old machine copy.
  // Closing this handoff window never reports a false "installed" status.
  WizardForm.Close;
end;

procedure CancelButtonClick(CurPageID: Integer; var Cancel, Confirm: Boolean);
begin
  if SmartMigrationStarted then Confirm := False;
end;


function SmartRegisteredDestinationSafe(var Reason: String): Boolean;
var
  Key: String;
  Registered: String;
  Destination: String;
  RegisteredPresent: Boolean;
  GotPath: Boolean;
begin
  // SMART_SAME_SCOPE_DESTINATION_GUARD_V1: Do not let a new canonical
  // install silently strand an existing noncanonical same-scope copy.
  Reason := '';
  Key := 'Software\Microsoft\Windows\CurrentVersion\Uninstall\{8F568FE8-A747-4CD0-A727-5FE81A405500}_is1';
  Destination := ExpandConstant('{app}');
  Registered := '';
  RegisteredPresent := RegKeyExists(HKCU64, Key) or RegKeyExists(HKCU32, Key);
  GotPath := RegQueryStringValue(HKCU64, Key, 'Inno Setup: App Path', Registered);

  Result := False;
  if RegisteredPresent then
  begin
    if not GotPath or (Registered = '') then
      Reason := 'Existing install registration lacks a verifiable application path.'
    else if CompareText(AddBackslash(Registered), AddBackslash(Destination)) <> 0 then
      Reason := 'An existing same-scope installation uses a different folder. Setup will not leave two copies.'
    else if not FileExists(AddBackslash(Registered) + 'unins000.exe') or
            not FileExists(AddBackslash(Registered) + '{#AppExeName}') then
      Reason := 'Existing installation files or its uninstaller are missing. Manual repair is required.'
    else
      Result := True;
  end
  else if FileExists(AddBackslash(Destination) + '{#AppExeName}') then
    Reason := 'An unregistered executable already exists at the installation destination.'
  else
    Result := True;
end;

function PrepareToInstall(var NeedsRestart: Boolean): String;
var
  ProbeError: String;
  Collision: Boolean;
begin
  Result := '';
  ProbeError := '';
  // Enforce user scope even if a future packaging edit mistakenly overrides
  // Inno privileges. Never write to Program Files or an HKLM registration.
  if IsAdminInstallMode then
  begin
    Result := 'Smart Installer is per-user only. Administrator installation is not supported.';
    Log('PUBLIC_LOCALAPPDATA_ONLY_SMART_V1: rejected unexpected admin mode.');
    Exit;
  end;
  // The app-owned helper explicitly requests a temporary two-scope handoff,
  // but only after the old machine copy is snapshotted for recovery.
  Collision := SmartMachineRegistrationPresent and not SmartMigrationHelperAuthorized;

  if Collision then
  begin
    Result := 'A previous all-users SonKuPik installation exists in Program Files. ' +
      'This Smart Installer only installs in LocalAppData, without administrator rights. ' +
      'No files were changed. Open the existing app and use the explicit ' +
      'Pindah tanpa Admin migration in Cek Update to move safely to per-user.';
    Log('NATIVE_SMART_INSTALLER_V1: blocked cross-scope installation collision.');
  end
  else if ProbeError <> '' then
  begin
    Result := 'Smart Installer could not verify the existing user installation. ' +
      'No changes were made. Details: ' + ProbeError;
    Log('NATIVE_SMART_INSTALLER_V1: blocked indeterminate inventory: ' + ProbeError);
  end
  else if not SmartRegisteredDestinationSafe(ProbeError) then
  begin
    Result := 'Smart Installer cannot safely reuse this installation location. ' +
      'No changes were made. Details: ' + ProbeError;
    Log('NATIVE_SMART_INSTALLER_V1: blocked unsafe destination: ' + ProbeError);
  end;
end;

#elif PerUser
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
// SMART_INSTALL_MACHINE_SCOPE_GUARD_V1 — backend machine Setup shares the
// same original-user collision probe as the public Smart Installer.
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
