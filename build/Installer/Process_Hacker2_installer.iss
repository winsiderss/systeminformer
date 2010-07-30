;* Process Hacker 2 - Installer script
;*
;* Copyright (C) 2010 XhmikosR
;*
;* This file is part of Process Hacker.
;*
;* Process Hacker is free software; you can redistribute it and/or modify
;* it under the terms of the GNU General Public License as published by
;* the Free Software Foundation, either version 3 of the License, or
;* (at your option) any later version.
;*
;* Process Hacker is distributed in the hope that it will be useful,
;* but WITHOUT ANY WARRANTY; without even the implied warranty of
;* MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
;* GNU General Public License for more details.
;*
;* You should have received a copy of the GNU General Public License
;* along with Process Hacker.  If not, see <http://www.gnu.org/licenses/>.
;
;
;
; Requirements:
; *Inno Setup QuickStart Pack v5.3.10(+): http://www.jrsoftware.org/isdl.php#qsp


#define installer_build_number "02"

#define VerMajor
#define VerMinor
#define VerRevision
#define VerBuild

#expr ParseVersion("..\..\bin\Release32\ProcessHacker.exe", VerMajor, VerMinor, VerRevision, VerBuild)
#define app_version str(VerMajor) + "." + str(VerMinor) + "." + str(VerRevision) + "." + str(VerBuild)
#define simple_app_version str(VerMajor) + "." + str(VerMinor)
#define installer_build_date GetDateTimeString('mmm, d yyyy', '', '')


[Setup]
AppID=Process_Hacker2
AppCopyright=Copyright © 2010, Process Hacker Team. Licensed under the GNU GPL, v3.
AppContact=http://sourceforge.net/tracker/?group_id=242527
AppName=Process Hacker
AppVerName=Process Hacker {#= simple_app_version}
AppVersion={#= simple_app_version}
AppPublisher=wj32
AppPublisherURL=http://processhacker.sourceforge.net/
AppSupportURL=http://sourceforge.net/tracker/?group_id=242527
AppUpdatesURL=http://processhacker.sourceforge.net/
UninstallDisplayName=Process Hacker {#= simple_app_version}
DefaultDirName={pf}\Process Hacker 2
DefaultGroupName=Process Hacker 2
VersionInfoCompany=wj32
VersionInfoCopyright=Licensed under the GNU GPL, v3.
VersionInfoDescription=Process Hacker {#= simple_app_version} Setup
VersionInfoTextVersion={#= simple_app_version}
VersionInfoVersion={#= simple_app_version}
VersionInfoProductName=Process Hacker
VersionInfoProductVersion={#= simple_app_version}
VersionInfoProductTextVersion={#= simple_app_version}
MinVersion=0,5.01.2600sp2
AppReadmeFile={app}\Help.htm
LicenseFile=..\..\LICENSE.txt
InfoAfterFile=..\..\CHANGELOG.txt
;InfoBeforeFile=..\..\doc\Help.htm
SetupIconFile=..\..\ProcessHacker\ProcessHacker.ico
UninstallDisplayIcon={app}\ProcessHacker.exe
WizardImageFile=Icons\ProcessHackerLarge.bmp
WizardSmallImageFile=Icons\ProcessHackerSmall.bmp
OutputDir=.
OutputBaseFilename=processhacker-{#= simple_app_version}-setup
AllowNoIcons=yes
Compression=lzma2/max
SolidCompression=yes
EnableDirDoesntExistWarning=no
DirExistsWarning=no
ShowTasksTreeLines=yes
AlwaysShowDirOnReadyPage=yes
AlwaysShowGroupOnReadyPage=yes
PrivilegesRequired=admin
ShowLanguageDialog=auto
DisableDirPage=auto
DisableProgramGroupPage=auto
AppMutex=Global\ProcessHacker2Mutant
ArchitecturesInstallIn64BitMode=x64


[Languages]
; Installer's languages
Name: en; MessagesFile: compiler:Default.isl
Name: de; MessagesFile: compiler:Languages\German.isl
Name: gr; MessagesFile: Languages\Greek.isl


; Include the installer's custom messages and services stuff
#include "Custom_Messages.iss"
#include "Services.iss"


[Messages]
BeveledLabel=Process Hacker v{#= simple_app_version} by wj32, Setup v{#= installer_build_number} built on {#= installer_build_date}


[Files]
Source: ..\..\CHANGELOG.txt; DestDir: {app}; Flags: ignoreversion
Source: ..\..\LICENSE.txt; DestDir: {app}; Flags: ignoreversion
Source: ..\..\doc\Help.htm; DestDir: {app}; Flags: ignoreversion
Source: ..\..\bin\Release32\ProcessHacker.exe; DestDir: {app}; Flags: ignoreversion; Check: NOT Is64BitInstallMode()
Source: ..\..\bin\Release64\ProcessHacker.exe; DestDir: {app}; Flags: ignoreversion; Check: Is64BitInstallMode()
Source: ..\..\KProcessHacker\i386\kprocesshacker.sys; DestDir: {app}; Flags: ignoreversion; Check: NOT Is64BitInstallMode()
Source: ..\..\KProcessHacker\amd64\kprocesshacker.sys; DestDir: {app}; Flags: ignoreversion; Check: Is64BitInstallMode()
Source: Icons\uninstall.ico; DestDir: {app}; Flags: ignoreversion


[Tasks]
Name: desktopicon; Description: {cm:CreateDesktopIcon}; GroupDescription: {cm:AdditionalIcons}
Name: desktopicon\user; Description: {cm:tsk_CurrentUser}; GroupDescription: {cm:AdditionalIcons}; Flags: exclusive
Name: desktopicon\common; Description: {cm:tsk_AllUsers}; GroupDescription: {cm:AdditionalIcons}; Flags: unchecked exclusive
Name: quicklaunchicon; Description: {cm:CreateQuickLaunchIcon}; GroupDescription: {cm:AdditionalIcons}; OnlyBelowVersion: 0,6.01; Flags: unchecked

Name: startup_task; Description: {cm:tsk_StartupDescr}; GroupDescription: {cm:tsk_Startup}; Check: StartupCheck(); Flags: unchecked checkablealone
;Name: startup_task\minimized; Description: {cm:tsk_StartupDescrMin}; GroupDescription: {cm:tsk_Startup}; Check: StartupCheck(); Flags: unchecked
Name: remove_startup_task; Description: {cm:tsk_RemoveStartup}; GroupDescription: {cm:tsk_Startup}; Check: NOT StartupCheck(); Flags: unchecked

Name: create_KPH_service; Description: {cm:tsk_CreateKPHService}; GroupDescription: {cm:tsk_Other}; Check: NOT KPHServiceCheck() AND NOT Is64BitInstallMode(); Flags: unchecked
Name: delete_KPH_service; Description: {cm:tsk_DeleteKPHService}; GroupDescription: {cm:tsk_Other}; Check: KPHServiceCheck() AND NOT Is64BitInstallMode(); Flags: unchecked

Name: reset_settings; Description: {cm:tsk_ResetSettings}; GroupDescription: {cm:tsk_Other}; Check: SettingsExistCheck(); Flags: checkedonce unchecked

Name: set_default_taskmgr; Description: {cm:tsk_SetDefaultTaskmgr}; GroupDescription: {cm:tsk_Other}; Check: PHDefaulTaskmgrCheck(); Flags: unchecked
Name: restore_taskmgr; Description: {cm:tsk_RestoreTaskmgr}; GroupDescription: {cm:tsk_Other}; Check: NOT PHDefaulTaskmgrCheck(); Flags: unchecked


[Icons]
Name: {group}\Process Hacker 2; Filename: {app}\ProcessHacker.exe; Comment: Process Hacker {#= simple_app_version}; WorkingDir: {app}; IconFilename: {app}\ProcessHacker.exe; IconIndex: 0
Name: {group}\{cm:sm_Help}\{cm:sm_Changelog}; Filename: {app}\CHANGELOG.txt; Comment: {cm:sm_com_Changelog}; WorkingDir: {app}
Name: {group}\{cm:sm_Help}\{cm:sm_HelpFile}; Filename: {app}\Help.htm; Comment: {cm:sm_HelpFile}; WorkingDir: {app}
Name: {group}\{cm:sm_Help}\{cm:ProgramOnTheWeb,Process Hacker 2}; Filename: http://processhacker.sourceforge.net/; Comment: {cm:ProgramOnTheWeb,Process Hacker 2}
Name: {group}\{cm:UninstallProgram,Process Hacker 2}; Filename: {uninstallexe}; IconFilename: {app}\uninstall.ico; Comment: {cm:UninstallProgram,Process Hacker 2}; WorkingDir: {app}

Name: {commondesktop}\Process Hacker 2; Filename: {app}\ProcessHacker.exe; Tasks: desktopicon\common; Comment: Process Hacker {#= simple_app_version}; WorkingDir: {app}; IconFilename: {app}\ProcessHacker.exe; IconIndex: 0
Name: {userdesktop}\Process Hacker 2; Filename: {app}\ProcessHacker.exe; Tasks: desktopicon\user; Comment: Process Hacker {#= simple_app_version}; WorkingDir: {app}; IconFilename: {app}\ProcessHacker.exe; IconIndex: 0
Name: {userappdata}\Microsoft\Internet Explorer\Quick Launch\Process Hacker 2; Filename: {app}\ProcessHacker.exe; Tasks: quicklaunchicon; Comment: Process Hacker {#= simple_app_version}; WorkingDir: {app}; IconFilename: {app}\ProcessHacker.exe; IconIndex: 0


[InstallDelete]
Type: files; Name: {userdesktop}\Process Hacker 2.lnk; Tasks: NOT desktopicon\user
Type: files; Name: {commondesktop}\Process Hacker 2.lnk; Tasks: NOT desktopicon\common

Type: files; Name: {userappdata}\Process Hacker 2\settings.xml; Tasks: reset_settings
Type: dirifempty; Name: {userappdata}\Process Hacker; Tasks: reset_settings


[Registry]
Root: HKLM; Subkey: SOFTWARE\Microsoft\Windows NT\CurrentVersion\Image File Execution Options\taskmgr.exe; Flags: uninsdeletekeyifempty dontcreatekey
Root: HKCU; SubKey: Software\Microsoft\Windows\CurrentVersion\Run; ValueType: string; ValueName: Process Hacker 2; ValueData: """{app}\ProcessHacker.exe"""; Tasks: startup_task; Flags: uninsdeletevalue
;Root: HKCU; SubKey: Software\Microsoft\Windows\CurrentVersion\Run; ValueType: string; ValueName: Process Hacker 2; ValueData: """{app}\ProcessHacker.exe"" -m"; Tasks: startup_task\minimized; Flags: uninsdeletevalue
Root: HKCU; SubKey: Software\Microsoft\Windows\CurrentVersion\Run; ValueName: Process Hacker 2; Tasks: remove_startup_task; Flags: deletevalue uninsdeletevalue
Root: HKLM; Subkey: SOFTWARE\Microsoft\Windows NT\CurrentVersion\Image File Execution Options\taskmgr.exe; ValueType: string; ValueName: Debugger; ValueData: """{app}\ProcessHacker.exe"""; Tasks: set_default_taskmgr
Root: HKLM; Subkey: SOFTWARE\Microsoft\Windows NT\CurrentVersion\Image File Execution Options\taskmgr.exe; ValueType: string; ValueName: Debugger; ValueData: """{app}\ProcessHacker.exe"""; Flags: uninsdeletevalue; Check: NOT PHDefaulTaskmgrCheck()
Root: HKLM; Subkey: SOFTWARE\Microsoft\Windows NT\CurrentVersion\Image File Execution Options\taskmgr.exe; ValueName: Debugger; Tasks: restore_taskmgr reset_settings; Flags: deletevalue uninsdeletevalue; Check: NOT PHDefaulTaskmgrCheck()


[Run]
Filename: {app}\ProcessHacker.exe; Description: {cm:LaunchProgram,Process Hacker 2}; Flags: nowait postinstall skipifsilent runascurrentuser
Filename: http://processhacker.sourceforge.net/; Description: {cm:run_VisitWebsite}; Flags: nowait postinstall skipifsilent shellexec runascurrentuser unchecked


[Code]
// Global variables and constants
const installer_mutex_name = 'process_hacker2_setup_mutex';


// Check if Process Hacker is configured to run on startup in order to control
// startup choice from within the installer
function StartupCheck(): Boolean;
begin
  Result := True;
  if RegValueExists(HKCU, 'Software\Microsoft\Windows\CurrentVersion\Run', 'Process Hacker 2') then
  Result := False;
end;


// Check if Process Hacker's settings exist
function SettingsExistCheck(): Boolean;
begin
  Result := False;
  if FileExists(ExpandConstant('{userappdata}\Process Hacker 2\settings.xml')) then
  Result := True;
end;


// Check if Process Hacker is set as the default Task Manager for Windows
function PHDefaulTaskmgrCheck(): Boolean;
var
  svalue: String;
begin
  Result := True;
  if RegQueryStringValue(HKLM,
  'SOFTWARE\Microsoft\Windows NT\CurrentVersion\Image File Execution Options\taskmgr.exe', 'Debugger', svalue) then begin
    if svalue = (ExpandConstant('"{app}\ProcessHacker.exe"')) then
    Result := False;
  end;
end;


// Check if KProcessHacker is installed as a service
function KPHServiceCheck(): Boolean;
var
  dvalue: DWORD;
begin
  Result := False;
  if RegQueryDWordValue(HKLM, 'SYSTEM\CurrentControlSet\Services\KProcessHacker2', 'Start', dvalue) then begin
    if dvalue = 1 then
    Result := True;
  end;
end;


Procedure CleanUpFiles();
begin
  DeleteFile(ExpandConstant('{userappdata}\Process Hacker 2\settings.xml'));
  RemoveDir(ExpandConstant('{userappdata}\Process Hacker 2\'));
end;


Procedure CurStepChanged(CurStep: TSetupStep);
begin
  case CurStep of ssInstall:
  begin
    if IsServiceRunning('KProcessHacker2') then begin
      StopService('KProcessHacker2');
    end;
    if IsTaskSelected('delete_KPH_service') then begin
      RemoveService('KProcessHacker2');
    end;
  end;
  ssPostInstall:
  begin
    if (KPHServiceCheck AND NOT IsTaskSelected('delete_KPH_service') OR (IsTaskSelected('create_KPH_service'))) then begin
      StopService('KProcessHacker2');
      RemoveService('KProcessHacker2');
      InstallService(ExpandConstant('{app}\kprocesshacker.sys'),'KProcessHacker2','KProcessHacker2','KProcessHacker2 driver',SERVICE_KERNEL_DRIVER,SERVICE_SYSTEM_START);
      StartService('KProcessHacker2');
    end;
  end;
 end;
end;


Procedure CurUninstallStepChanged(CurUninstallStep: TUninstallStep);
begin
  // When uninstalling ask user to delete Process Hacker's settings
  // based on whether the settings file exists only
  if CurUninstallStep = usUninstall then begin
    StopService('KProcessHacker2');
    RemoveService('KProcessHacker2');
  if SettingsExistCheck then begin
    if MsgBox(ExpandConstant('{cm:msg_DeleteLogSettings}'),
     mbConfirmation, MB_YESNO or MB_DEFBUTTON2) = IDYES then begin
       CleanUpFiles;
     end;
      RemoveDir(ExpandConstant('{app}'));
    end;
  end;
end;


function InitializeSetup(): Boolean;
begin
  // Create a mutex for the installer and if it's already running then expose a message and stop installation
  Result := True;
  if CheckForMutexes(installer_mutex_name) then begin
    if not WizardSilent() then
      MsgBox(ExpandConstant('{cm:msg_SetupIsRunningWarning}'), mbCriticalError, MB_OK);
    exit;
    Result := False;
  end;
  CreateMutex(installer_mutex_name);
end;


function InitializeUninstall(): Boolean;
begin
  Result := True;
  if CheckForMutexes(installer_mutex_name) then begin
    if not WizardSilent() then
      MsgBox(ExpandConstant('{cm:msg_SetupIsRunningWarning}'), mbCriticalError, MB_OK);
      exit;
   end;
   CreateMutex(installer_mutex_name);
end;
