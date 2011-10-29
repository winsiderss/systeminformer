;* Process Hacker 2 - Installer script
;*
;* Copyright (C) 2011 wj32
;* Copyright (C) 2010-2011 XhmikosR
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
; *Inno Setup v5.4.2(+): http://www.jrsoftware.org/isdl.php


#define installer_build_number "10"


#if VER < 0x05040200
  #error Update your Inno Setup version
#endif

#include "..\..\ProcessHacker\include\phappres.h"
#define app_version           str(PHAPP_VERSION_MAJOR) + "." + str(PHAPP_VERSION_MINOR) + ".0.0"
#define simple_app_version    str(PHAPP_VERSION_MAJOR) + "." + str(PHAPP_VERSION_MINOR)
#define installer_build_date  GetDateTimeString('mmm, d yyyy', '', '')


[Setup]
AppID=Process_Hacker2
AppCopyright=Copyright © 2010-2011, Process Hacker Team. Licensed under the GNU GPL, v3.
AppContact=http://sourceforge.net/tracker/?group_id=242527
AppName=Process Hacker
AppVerName=Process Hacker {#simple_app_version}
AppVersion={#simple_app_version}
AppPublisher=wj32
AppPublisherURL=http://processhacker.sourceforge.net/
AppSupportURL=http://sourceforge.net/projects/processhacker/support
AppUpdatesURL=http://processhacker.sourceforge.net/
UninstallDisplayName=Process Hacker {#simple_app_version}
DefaultDirName={pf}\Process Hacker 2
DefaultGroupName=Process Hacker 2
VersionInfoCompany=wj32
VersionInfoCopyright=Licensed under the GNU GPL, v3.
VersionInfoDescription=Process Hacker Setup
VersionInfoProductName=Process Hacker
VersionInfoProductTextVersion={#simple_app_version}
VersionInfoProductVersion={#simple_app_version}
VersionInfoTextVersion={#simple_app_version}
VersionInfoVersion={#simple_app_version}
MinVersion=0,5.01.2600sp2
LicenseFile=..\..\LICENSE.txt
InfoAfterFile=..\..\CHANGELOG.txt
SetupIconFile=..\..\ProcessHacker\ProcessHacker.ico
UninstallDisplayIcon={app}\ProcessHacker.exe
WizardImageFile=Icons\ProcessHackerLarge.bmp
WizardSmallImageFile=Icons\ProcessHackerSmall.bmp
OutputDir=.
OutputBaseFilename=processhacker-{#simple_app_version}-setup
AllowNoIcons=yes
Compression=lzma2/ultra
InternalCompressLevel=max
SolidCompression=yes
EnableDirDoesntExistWarning=no
ShowTasksTreeLines=yes
AlwaysShowDirOnReadyPage=yes
AlwaysShowGroupOnReadyPage=yes
PrivilegesRequired=admin
ShowLanguageDialog=no
DisableDirPage=auto
DisableProgramGroupPage=auto
AppMutex=Global\ProcessHacker2Mutant
ArchitecturesInstallIn64BitMode=x64


[Languages]
; Installer's languages
Name: en; MessagesFile: compiler:Default.isl


; Include the installer's custom messages and services stuff
#include "Custom_Messages.iss"
#include "Services.iss"


[Messages]
BeveledLabel=Process Hacker v{#simple_app_version}, Setup v{#installer_build_number} built on {#installer_build_date}


[Types]
Name: "full";                          Description: "Full installation"
Name: "minimal";                       Description: "Minimal installation"
Name: "custom";                        Description: "Custom installation";                                 Flags: iscustom


[Components]
Name: "main";                          Description: "Main application";        Types: full minimal custom; Flags: fixed
Name: "peview";                        Description: "PE Viewer";               Types: full minimal custom; Flags: disablenouninstallwarning
Name: "plugins";                       Description: "Plugins";                 Types: full custom;         Flags: disablenouninstallwarning
Name: "plugins\dotnettools";           Description: ".NET Tools";              Types: full custom;         Flags: disablenouninstallwarning
Name: "plugins\extendednotifications"; Description: "Extended Notifications";  Types: full custom;         Flags: disablenouninstallwarning
Name: "plugins\extendedservices";      Description: "Extended Services";       Types: full custom;         Flags: disablenouninstallwarning
Name: "plugins\extendedtools";         Description: "Extended Tools";          Types: full custom;         Flags: disablenouninstallwarning;   MinVersion: 0,6.00
Name: "plugins\networktools";          Description: "Network Tools";           Types: full custom;         Flags: disablenouninstallwarning
Name: "plugins\onlinechecks";          Description: "Online Checks";           Types: full custom;         Flags: disablenouninstallwarning
Name: "plugins\sbiesupport";           Description: "Sandboxie Support";       Types: full custom;         Flags: disablenouninstallwarning
Name: "plugins\toolstatus";            Description: "Toolbar and Status Bar";  Types: full custom;         Flags: disablenouninstallwarning
Name: "plugins\updater";               Description: "Updater";                 Types: full custom;         Flags: disablenouninstallwarning
Name: "plugins\usernotes";               Description: "User Notes";            Types: full custom;         Flags: disablenouninstallwarning
Name: "plugins\windowexplorer";        Description: "Window Explorer";         Types: full custom;         Flags: disablenouninstallwarning


[Tasks]
Name: desktopicon;         Description: {cm:CreateDesktopIcon};     GroupDescription: {cm:AdditionalIcons}
Name: desktopicon\user;    Description: {cm:tsk_CurrentUser};       GroupDescription: {cm:AdditionalIcons};                                    Flags: exclusive
Name: desktopicon\common;  Description: {cm:tsk_AllUsers};          GroupDescription: {cm:AdditionalIcons};                                    Flags: unchecked exclusive
Name: quicklaunchicon;     Description: {cm:CreateQuickLaunchIcon}; GroupDescription: {cm:AdditionalIcons}; OnlyBelowVersion: 0,6.01;          Flags: unchecked

Name: startup;             Description: {cm:tsk_StartupDescr};      GroupDescription: {cm:tsk_Startup};     Check: NOT StartupCheck();         Flags: unchecked checkablealone
Name: startup\minimized;   Description: {cm:tsk_StartupDescrMin};   GroupDescription: {cm:tsk_Startup};     Check: NOT StartupCheck();         Flags: unchecked
Name: remove_startup;      Description: {cm:tsk_RemoveStartup};     GroupDescription: {cm:tsk_Startup};     Check: StartupCheck();             Flags: unchecked

Name: create_KPH_service;  Description: {cm:tsk_CreateKPHService};  GroupDescription: {cm:tsk_Other};       Check: NOT KPHServiceCheck();      Flags: unchecked
Name: delete_KPH_service;  Description: {cm:tsk_DeleteKPHService};  GroupDescription: {cm:tsk_Other};       Check: KPHServiceCheck();          Flags: unchecked

Name: reset_settings;      Description: {cm:tsk_ResetSettings};     GroupDescription: {cm:tsk_Other};       Check: SettingsExistCheck();       Flags: checkedonce unchecked

Name: set_default_taskmgr; Description: {cm:tsk_SetDefaultTaskmgr}; GroupDescription: {cm:tsk_Other};       Check: NOT PHDefaulTaskmgrCheck(); Flags: checkedonce unchecked
Name: restore_taskmgr;     Description: {cm:tsk_RestoreTaskmgr};    GroupDescription: {cm:tsk_Other};       Check: PHDefaulTaskmgrCheck();     Flags: checkedonce unchecked


[Files]
Source: ..\..\CHANGELOG.txt;                                      DestDir: {app};                                                    Flags: ignoreversion
Source: ..\..\COPYRIGHT.txt;                                      DestDir: {app};                                                    Flags: ignoreversion
Source: ..\..\LICENSE.txt;                                        DestDir: {app};                                                    Flags: ignoreversion
Source: ..\..\README.txt;                                         DestDir: {app};                                                    Flags: ignoreversion

Source: ..\..\bin\Release32\ProcessHacker.exe;                    DestDir: {app};                                                    Flags: ignoreversion; Check: NOT Is64BitInstallMode()
Source: ..\..\bin\Release64\ProcessHacker.exe;                    DestDir: {app};                                                    Flags: ignoreversion; Check: Is64BitInstallMode()

Source: ..\..\KProcessHacker\bin-signed\i386\kprocesshacker.sys;  DestDir: {app};                                                    Flags: ignoreversion; Check: NOT Is64BitInstallMode()
Source: ..\..\KProcessHacker\bin-signed\amd64\kprocesshacker.sys; DestDir: {app};                                                    Flags: ignoreversion; Check: Is64BitInstallMode()

Source: ..\..\bin\Release32\peview.exe;                           DestDir: {app};         Components: peview;                        Flags: ignoreversion; Check: NOT Is64BitInstallMode()
Source: ..\..\bin\Release64\peview.exe;                           DestDir: {app};         Components: peview;                        Flags: ignoreversion; Check: Is64BitInstallMode()

Source: ..\..\bin\Release32\plugins\DotNetTools.dll;              DestDir: {app}\plugins; Components: plugins\dotnettools;           Flags: ignoreversion; Check: NOT Is64BitInstallMode()
Source: ..\..\bin\Release64\plugins\DotNetTools.dll;              DestDir: {app}\plugins; Components: plugins\dotnettools;           Flags: ignoreversion; Check: Is64BitInstallMode()
Source: ..\..\bin\Release32\plugins\ExtendedNotifications.dll;    DestDir: {app}\plugins; Components: plugins\extendednotifications; Flags: ignoreversion; Check: NOT Is64BitInstallMode()
Source: ..\..\bin\Release64\plugins\ExtendedNotifications.dll;    DestDir: {app}\plugins; Components: plugins\extendednotifications; Flags: ignoreversion; Check: Is64BitInstallMode()
Source: ..\..\bin\Release32\plugins\ExtendedServices.dll;         DestDir: {app}\plugins; Components: plugins\extendedservices;      Flags: ignoreversion; Check: NOT Is64BitInstallMode()
Source: ..\..\bin\Release64\plugins\ExtendedServices.dll;         DestDir: {app}\plugins; Components: plugins\extendedservices;      Flags: ignoreversion; Check: Is64BitInstallMode()
Source: ..\..\bin\Release32\plugins\ExtendedTools.dll;            DestDir: {app}\plugins; Components: plugins\extendedtools;         Flags: ignoreversion; Check: NOT Is64BitInstallMode()
Source: ..\..\bin\Release64\plugins\ExtendedTools.dll;            DestDir: {app}\plugins; Components: plugins\extendedtools;         Flags: ignoreversion; Check: Is64BitInstallMode()
Source: ..\..\bin\Release32\plugins\NetworkTools.dll;             DestDir: {app}\plugins; Components: plugins\networktools;          Flags: ignoreversion; Check: NOT Is64BitInstallMode()
Source: ..\..\bin\Release64\plugins\NetworkTools.dll;             DestDir: {app}\plugins; Components: plugins\networktools;          Flags: ignoreversion; Check: Is64BitInstallMode()
Source: ..\..\bin\Release32\plugins\OnlineChecks.dll;             DestDir: {app}\plugins; Components: plugins\onlinechecks;          Flags: ignoreversion; Check: NOT Is64BitInstallMode()
Source: ..\..\bin\Release64\plugins\OnlineChecks.dll;             DestDir: {app}\plugins; Components: plugins\onlinechecks;          Flags: ignoreversion; Check: Is64BitInstallMode()
Source: ..\..\bin\Release32\plugins\SbieSupport.dll;              DestDir: {app}\plugins; Components: plugins\sbiesupport;           Flags: ignoreversion; Check: NOT Is64BitInstallMode()
Source: ..\..\bin\Release64\plugins\SbieSupport.dll;              DestDir: {app}\plugins; Components: plugins\sbiesupport;           Flags: ignoreversion; Check: Is64BitInstallMode()
Source: ..\..\bin\Release32\plugins\ToolStatus.dll;               DestDir: {app}\plugins; Components: plugins\toolstatus;            Flags: ignoreversion; Check: NOT Is64BitInstallMode()
Source: ..\..\bin\Release64\plugins\ToolStatus.dll;               DestDir: {app}\plugins; Components: plugins\toolstatus;            Flags: ignoreversion; Check: Is64BitInstallMode()
Source: ..\..\bin\Release32\plugins\Updater.dll;                  DestDir: {app}\plugins; Components: plugins\updater;               Flags: ignoreversion; Check: NOT Is64BitInstallMode()
Source: ..\..\bin\Release64\plugins\Updater.dll;                  DestDir: {app}\plugins; Components: plugins\updater;               Flags: ignoreversion; Check: Is64BitInstallMode()
Source: ..\..\bin\Release32\plugins\UserNotes.dll;                DestDir: {app}\plugins; Components: plugins\usernotes;             Flags: ignoreversion; Check: NOT Is64BitInstallMode()
Source: ..\..\bin\Release64\plugins\UserNotes.dll;                DestDir: {app}\plugins; Components: plugins\usernotes;             Flags: ignoreversion; Check: Is64BitInstallMode()
Source: ..\..\bin\Release32\plugins\WindowExplorer.dll;           DestDir: {app}\plugins; Components: plugins\windowexplorer;        Flags: ignoreversion; Check: NOT Is64BitInstallMode()
Source: ..\..\bin\Release64\plugins\WindowExplorer.dll;           DestDir: {app}\plugins; Components: plugins\windowexplorer;        Flags: ignoreversion; Check: Is64BitInstallMode()

Source: Icons\uninstall.ico;                                      DestDir: {app};                                                    Flags: ignoreversion


[Icons]
Name: {group}\PE Viewer;        Filename: {app}\peview.exe;        WorkingDir: {app}; Comment: PE Viewer; IconFilename: {app}\peview.exe; IconIndex: 0; Components: peview; Flags: excludefromshowinnewinstall
Name: {group}\Process Hacker 2; Filename: {app}\ProcessHacker.exe; WorkingDir: {app}; Comment: Process Hacker {#simple_app_version}; IconFilename: {app}\ProcessHacker.exe; IconIndex: 0
Name: {group}\{cm:sm_Help}\{cm:sm_Changelog}; Filename: {app}\CHANGELOG.txt; WorkingDir: {app}; Comment: {cm:sm_com_Changelog}
Name: {group}\{cm:sm_Help}\{cm:ProgramOnTheWeb,Process Hacker 2}; Filename: http://processhacker.sourceforge.net/; Comment: {cm:ProgramOnTheWeb,Process Hacker 2}
Name: {group}\{cm:UninstallProgram,Process Hacker 2};             Filename: {uninstallexe}; WorkingDir: {app};     Comment: {cm:UninstallProgram,Process Hacker 2}; IconFilename: {app}\uninstall.ico

Name: {commondesktop}\Process Hacker 2; Filename: {app}\ProcessHacker.exe;      WorkingDir: {app}; Comment: Process Hacker {#simple_app_version}; IconFilename: {app}\ProcessHacker.exe; IconIndex: 0; Tasks: desktopicon\common
Name: {userdesktop}\Process Hacker 2;   Filename: {app}\ProcessHacker.exe;      WorkingDir: {app}; Comment: Process Hacker {#simple_app_version}; IconFilename: {app}\ProcessHacker.exe; IconIndex: 0; Tasks: desktopicon\user
Name: {userappdata}\Microsoft\Internet Explorer\Quick Launch\Process Hacker 2;  Filename: {app}\ProcessHacker.exe; WorkingDir: {app}; Comment: Process Hacker {#simple_app_version}; IconFilename: {app}\ProcessHacker.exe; IconIndex: 0; Tasks: quicklaunchicon


[InstallDelete]
Type: files;      Name: {userdesktop}\Process Hacker 2.lnk;          Check: NOT IsTaskSelected('desktopicon\user')   AND IsUpgrade()
Type: files;      Name: {commondesktop}\Process Hacker 2.lnk;        Check: NOT IsTaskSelected('desktopicon\common') AND IsUpgrade()
Type: files;      Name: {userappdata}\Microsoft\Internet Explorer\Quick Launch\Process Hacker 2.lnk; Check: NOT IsTaskSelected('quicklaunchicon') AND IsUpgrade()
Type: files;      Name: {group}\Help and Support\Process Hacker Help.lnk; Check: IsUpgrade()

Type: files;      Name: {userappdata}\Process Hacker 2\settings.xml; Tasks: reset_settings
Type: dirifempty; Name: {userappdata}\Process Hacker;                Tasks: reset_settings

Type: files;      Name: {app}\Help.htm;                              Check: IsUpgrade()
Type: files;      Name: {app}\peview.exe;                            Check: NOT IsComponentSelected('peview')                        AND IsUpgrade()
Type: files;      Name: {group}\PE Viewer.lnk;                       Check: NOT IsComponentSelected('peview')                        AND IsUpgrade()
Type: files;      Name: {app}\plugins\DotNetTools.dll;               Check: NOT IsComponentSelected('plugins\dotnettools')           AND IsUpgrade()
Type: files;      Name: {app}\plugins\ExtendedNotifications.dll;     Check: NOT IsComponentSelected('plugins\extendednotifications') AND IsUpgrade()
Type: files;      Name: {app}\plugins\ExtendedServices.dll;          Check: NOT IsComponentSelected('plugins\extendedservices')      AND IsUpgrade()
Type: files;      Name: {app}\plugins\ExtendedTools.dll;             Check: NOT IsComponentSelected('plugins\extendedtools')         AND IsUpgrade()
Type: files;      Name: {app}\plugins\NetworkTools.dll;              Check: NOT IsComponentSelected('plugins\networktools')          AND IsUpgrade()
Type: files;      Name: {app}\plugins\OnlineChecks.dll;              Check: NOT IsComponentSelected('plugins\onlinechecks')          AND IsUpgrade()
Type: files;      Name: {app}\plugins\SbieSupport.dll;               Check: NOT IsComponentSelected('plugins\sbiesupport')           AND IsUpgrade()
Type: files;      Name: {app}\plugins\ToolStatus.dll;                Check: NOT IsComponentSelected('plugins\toolstatus')            AND IsUpgrade()
Type: files;      Name: {app}\plugins\Updater.dll;                   Check: NOT IsComponentSelected('plugins\updater')               AND IsUpgrade()
Type: files;      Name: {app}\plugins\UserNotes.dll;                 Check: NOT IsComponentSelected('plugins\usernotes')             AND IsUpgrade()
Type: files;      Name: {app}\plugins\WindowExplorer.dll;            Check: NOT IsComponentSelected('plugins\windowexplorer')        AND IsUpgrade()
Type: dirifempty; Name: {app}\plugins


[Registry]
Root: HKCU; SubKey: Software\Microsoft\Windows\CurrentVersion\Run; ValueType: string; ValueName: Process Hacker 2; ValueData: """{app}\ProcessHacker.exe""";       Flags: uninsdeletevalue; Tasks: startup
Root: HKCU; SubKey: Software\Microsoft\Windows\CurrentVersion\Run; ValueType: string; ValueName: Process Hacker 2; ValueData: """{app}\ProcessHacker.exe"" -hide"; Flags: uninsdeletevalue; Tasks: startup\minimized
Root: HKCU; SubKey: Software\Microsoft\Windows\CurrentVersion\Run; ValueName: Process Hacker 2;            Flags: deletevalue uninsdeletevalue; Tasks: remove_startup
Root: HKLM; Subkey: SOFTWARE\Microsoft\Windows NT\CurrentVersion\Image File Execution Options\taskmgr.exe; Flags: uninsdeletekeyifempty dontcreatekey
Root: HKLM; Subkey: SOFTWARE\Microsoft\Windows NT\CurrentVersion\Image File Execution Options\taskmgr.exe; ValueType: string; ValueName: Debugger; ValueData: """{app}\ProcessHacker.exe"""; Tasks: set_default_taskmgr
Root: HKLM; Subkey: SOFTWARE\Microsoft\Windows NT\CurrentVersion\Image File Execution Options\taskmgr.exe; ValueType: string; ValueName: Debugger; ValueData: """{app}\ProcessHacker.exe"""; Flags: uninsdeletevalue; Check: PHDefaulTaskmgrCheck()
Root: HKLM; Subkey: SOFTWARE\Microsoft\Windows NT\CurrentVersion\Image File Execution Options\taskmgr.exe; ValueName: Debugger; Flags: deletevalue uninsdeletevalue; Tasks: restore_taskmgr reset_settings; Check: PHDefaulTaskmgrCheck()


[Run]
Filename: {app}\ProcessHacker.exe;               Description: {cm:LaunchProgram,Process Hacker 2}; Flags: nowait postinstall skipifsilent runascurrentuser
Filename: http://processhacker.sourceforge.net/; Description: {cm:run_VisitWebsite};               Flags: nowait postinstall skipifsilent shellexec runascurrentuser unchecked


[Code]
// Global variables and constants
const installer_mutex_name = 'process_hacker2_setup_mutex';


function IsUpgrade(): Boolean;
var
  sPrevPath: String;
begin
  sPrevPath := WizardForm.PrevAppDir;
  Result := (sPrevPath <> '');
end;


function ShouldSkipPage(PageID: Integer): Boolean;
begin
  // Hide the license page
  if IsUpgrade() AND (PageID = wpLicense) then begin
    Result := True;
  end
  else begin
    Result := False;
  end;
end;


// Check if KProcessHacker is installed as a service
function KPHServiceCheck(): Boolean;
var
  dvalue: DWORD;
begin
  if RegQueryDWordValue(HKLM, 'SYSTEM\CurrentControlSet\Services\KProcessHacker2', 'Start', dvalue) then begin
    if dvalue = 1 then begin
      Result := True;
    end else
      Result := False;
  end;
end;


// Check if Process Hacker is set as the default Task Manager for Windows
function PHDefaulTaskmgrCheck(): Boolean;
var
  svalue: String;
begin
  if RegQueryStringValue(HKLM, 'SOFTWARE\Microsoft\Windows NT\CurrentVersion\Image File Execution Options\taskmgr.exe', 'Debugger', svalue) then begin
    if svalue = (ExpandConstant('"{app}\ProcessHacker.exe"')) then
      Result := True;
  end else
    Result := False;
end;


// Check if Process Hacker's settings exist
function SettingsExistCheck(): Boolean;
begin
  if FileExists(ExpandConstant('{userappdata}\Process Hacker 2\settings.xml')) then begin
    Result := True;
  end else
    Result := False;
end;


// Check if Process Hacker is configured to run on startup in order to control
// startup choice from within the installer
function StartupCheck(): Boolean;
var
  svalue: String;
begin
  if RegQueryStringValue(HKCU, 'Software\Microsoft\Windows\CurrentVersion\Run', 'Process Hacker 2', svalue) then begin
    if (svalue = (ExpandConstant('"{app}\ProcessHacker.exe"'))) OR (svalue = (ExpandConstant('"{app}\ProcessHacker.exe" -hide'))) then
      Result := True;
  end else
    Result := False;
end;


procedure CurStepChanged(CurStep: TSetupStep);
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
      InstallService(ExpandConstant('{app}\kprocesshacker.sys'), 'KProcessHacker2', 'KProcessHacker2', 'KProcessHacker2 driver', SERVICE_KERNEL_DRIVER, SERVICE_SYSTEM_START);
      StartService('KProcessHacker2');
    end;
  end;

 end;
end;


procedure CurUninstallStepChanged(CurUninstallStep: TUninstallStep);
begin
  // When uninstalling ask user to delete Process Hacker's settings
  // based on whether the settings file exists only
  if CurUninstallStep = usUninstall then begin
    StopService('KProcessHacker2');
    RemoveService('KProcessHacker2');

    if SettingsExistCheck then begin
      if SuppressibleMsgBox(ExpandConstant('{cm:msg_DeleteLogSettings}'), mbConfirmation, MB_YESNO OR MB_DEFBUTTON2, IDNO) = IDYES then begin
        DeleteFile(ExpandConstant('{userappdata}\Process Hacker 2\settings.xml'));
      end;
    end;

    RemoveDir(ExpandConstant('{userappdata}\Process Hacker 2'));
    RemoveDir(ExpandConstant('{app}\plugins'));
    RemoveDir(ExpandConstant('{app}'));

  end;
end;


function InitializeSetup(): Boolean;
begin
  // Create a mutex for the installer and if it's already running then expose a message and stop installation
  if CheckForMutexes(installer_mutex_name) AND NOT WizardSilent() then begin
    SuppressibleMsgBox(ExpandConstant('{cm:msg_SetupIsRunningWarning}'), mbError, MB_OK, MB_OK);
    Result := False;
  end
  else begin
    Result := True;
    CreateMutex(installer_mutex_name);
  end;
end;


function InitializeUninstall(): Boolean;
begin
  if CheckForMutexes(installer_mutex_name) then begin
    SuppressibleMsgBox(ExpandConstant('{cm:msg_SetupIsRunningWarning}'), mbError, MB_OK, MB_OK);
    Result := False;
  end
  else begin
    CreateMutex(installer_mutex_name);
    Result := True;
  end;
end;
