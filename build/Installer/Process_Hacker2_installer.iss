;* Process Hacker 2 - Installer script
;*
;* Copyright (C) 2011 wj32
;* Copyright (C) 2010-2012 XhmikosR
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
; Requirements:
; *Inno Setup: http://www.jrsoftware.org/isdl.php


#if VER < EncodeVer(5,4,3)
  #error Update your Inno Setup version (5.4.3 or newer)
#endif

#define ISPP_IS_BUGGY
#include "..\..\ProcessHacker\include\phappres.h"

; Include the custom messages and services
#include "Custom_Messages.iss"
#include "Services.iss"

#define installer_build_number "13"
#define copyright              "Copyright © 2010-2012, Process Hacker Team. Licensed under the GNU GPL, v3."

#if defined(TWO_DIGIT_VER)
  #define app_version          str(PHAPP_VERSION_MAJOR) + "." + str(PHAPP_VERSION_MINOR)
  #define app_version_long     str(PHAPP_VERSION_MAJOR) + "." + str(PHAPP_VERSION_MINOR) + ".0" + "." + str(PHAPP_VERSION_REVISION)
  #define app_version_full     str(PHAPP_VERSION_MAJOR) + "." + str(PHAPP_VERSION_MINOR) + " (r" + str(PHAPP_VERSION_REVISION) + ")"
#elif defined(THREE_DIGIT_VER)
  #define app_version          str(PHAPP_VERSION_MAJOR) + "." + str(PHAPP_VERSION_MINOR)
  #define app_version_long     str(PHAPP_VERSION_MAJOR) + "." + str(PHAPP_VERSION_MINOR) + "." + str(PHAPP_VERSION_BUILD) + "." + str(PHAPP_VERSION_REVISION)
  #define app_version_full     str(PHAPP_VERSION_MAJOR) + "." + str(PHAPP_VERSION_MINOR) + "." + str(PHAPP_VERSION_BUILD) + " (r" + str(PHAPP_VERSION_REVISION) + ")"
#endif

#define installer_build_date   GetDateTimeString('mmm, d yyyy', '', '')
#define quick_launch           "{userappdata}\Microsoft\Internet Explorer\Quick Launch"


[Setup]
AppID=Process_Hacker2
AppCopyright={#copyright}
AppContact=http://sourceforge.net/projects/processhacker/support
AppName=Process Hacker
AppVerName=Process Hacker {#app_version_full}
AppVersion={#app_version_long}
AppPublisher=wj32
AppPublisherURL=http://processhacker.sourceforge.net/
AppSupportURL=http://sourceforge.net/projects/processhacker/support
AppUpdatesURL=http://processhacker.sourceforge.net/
UninstallDisplayName=Process Hacker {#app_version_full}
DefaultDirName={pf}\Process Hacker 2
DefaultGroupName=Process Hacker 2
VersionInfoCompany=wj32
VersionInfoCopyright={#copyright}
VersionInfoDescription=Process Hacker Setup
VersionInfoProductName=Process Hacker
VersionInfoProductTextVersion={#app_version_full}
VersionInfoProductVersion={#app_version_long}
VersionInfoTextVersion={#app_version_full}
VersionInfoVersion={#app_version_long}
MinVersion=0,5.01.2600sp2
LicenseFile=..\..\LICENSE.txt
SetupIconFile=..\..\ProcessHacker\ProcessHacker.ico
UninstallDisplayIcon={app}\ProcessHacker.exe
WizardImageFile=Icons\ProcessHackerLarge.bmp
WizardSmallImageFile=Icons\ProcessHackerSmall.bmp
OutputDir=.
OutputBaseFilename=processhacker-{#app_version}-setup
AllowNoIcons=yes
Compression=lzma2/max
InternalCompressLevel=max
SolidCompression=yes
EnableDirDoesntExistWarning=no
ShowTasksTreeLines=yes
PrivilegesRequired=admin
DisableDirPage=auto
DisableProgramGroupPage=auto
AppMutex=Global\ProcessHacker2Mutant
ArchitecturesInstallIn64BitMode=x64


[Languages]
Name: en; MessagesFile: compiler:Default.isl


[Messages]
WelcomeLabel1=[name/ver]
WelcomeLabel2=This will install [name] on your computer.%n%nIt is recommended that you close all other applications before continuing.
BeveledLabel=Process Hacker v{#app_version_full}, Setup v{#installer_build_number} built on {#installer_build_date}
SetupAppTitle=Setup - Process Hacker
SetupWindowTitle=Setup - Process Hacker


[Types]
Name: full;                          Description: Full installation
Name: minimal;                       Description: Minimal installation
Name: custom;                        Description: Custom installation;                                 Flags: iscustom


[Components]
Name: main;                          Description: Main application;        Types: full minimal custom; Flags: fixed
Name: peview;                        Description: PE Viewer;               Types: full minimal custom; Flags: disablenouninstallwarning
Name: plugins;                       Description: Plugins;                 Types: full custom;         Flags: disablenouninstallwarning
Name: plugins\dotnettools;           Description: .NET Tools;              Types: full custom;         Flags: disablenouninstallwarning
Name: plugins\extendednotifications; Description: Extended Notifications;  Types: full custom;         Flags: disablenouninstallwarning
Name: plugins\extendedservices;      Description: Extended Services;       Types: full custom;         Flags: disablenouninstallwarning
Name: plugins\extendedtools;         Description: Extended Tools;          Types: full custom;         Flags: disablenouninstallwarning;       MinVersion: 0,6.00
Name: plugins\networktools;          Description: Network Tools;           Types: full custom;         Flags: disablenouninstallwarning
Name: plugins\onlinechecks;          Description: Online Checks;           Types: full custom;         Flags: disablenouninstallwarning
Name: plugins\sbiesupport;           Description: Sandboxie Support;       Types: full custom;         Flags: disablenouninstallwarning
Name: plugins\toolstatus;            Description: Toolbar and Status Bar;  Types: full custom;         Flags: disablenouninstallwarning
Name: plugins\updater;               Description: Updater;                 Types: full custom;         Flags: disablenouninstallwarning
Name: plugins\usernotes;             Description: User Notes;              Types: full custom;         Flags: disablenouninstallwarning
Name: plugins\windowexplorer;        Description: Window Explorer;         Types: full custom;         Flags: disablenouninstallwarning


[Tasks]
Name: desktopicon;         Description: {cm:CreateDesktopIcon};     GroupDescription: {cm:AdditionalIcons}
Name: desktopicon\user;    Description: {cm:tsk_CurrentUser};       GroupDescription: {cm:AdditionalIcons};                                    Flags: exclusive
Name: desktopicon\common;  Description: {cm:tsk_AllUsers};          GroupDescription: {cm:AdditionalIcons};                                    Flags: unchecked exclusive
Name: quicklaunchicon;     Description: {cm:CreateQuickLaunchIcon}; GroupDescription: {cm:AdditionalIcons}; OnlyBelowVersion: 0,6.01;          Flags: unchecked

Name: startup;             Description: {cm:tsk_StartupDescr};      GroupDescription: {cm:tsk_Startup};     Check: not StartupCheck();         Flags: unchecked checkablealone
Name: startup\minimized;   Description: {cm:tsk_StartupDescrMin};   GroupDescription: {cm:tsk_Startup};     Check: not StartupCheck();         Flags: unchecked
Name: remove_startup;      Description: {cm:tsk_RemoveStartup};     GroupDescription: {cm:tsk_Startup};     Check: StartupCheck();             Flags: unchecked

Name: create_KPH_service;  Description: {cm:tsk_CreateKPHService};  GroupDescription: {cm:tsk_Other};       Check: not KPHServiceCheck();      Flags: unchecked
Name: delete_KPH_service;  Description: {cm:tsk_DeleteKPHService};  GroupDescription: {cm:tsk_Other};       Check: KPHServiceCheck();          Flags: unchecked

Name: reset_settings;      Description: {cm:tsk_ResetSettings};     GroupDescription: {cm:tsk_Other};       Check: SettingsExistCheck();       Flags: checkedonce unchecked

Name: set_default_taskmgr; Description: {cm:tsk_SetDefaultTaskmgr}; GroupDescription: {cm:tsk_Other};       Check: not PHDefaulTaskmgrCheck(); Flags: checkedonce unchecked
Name: restore_taskmgr;     Description: {cm:tsk_RestoreTaskmgr};    GroupDescription: {cm:tsk_Other};       Check: PHDefaulTaskmgrCheck();     Flags: checkedonce unchecked


[Files]
Source: ..\..\CHANGELOG.txt;                                      DestDir: {app};                                                    Flags: ignoreversion
Source: ..\..\COPYRIGHT.txt;                                      DestDir: {app};                                                    Flags: ignoreversion
Source: ..\..\LICENSE.txt;                                        DestDir: {app};                                                    Flags: ignoreversion
Source: ..\..\README.txt;                                         DestDir: {app};                                                    Flags: ignoreversion

Source: ..\..\bin\Release32\ProcessHacker.exe;                    DestDir: {app};                                                    Flags: ignoreversion; Check: not Is64BitInstallMode()
Source: ..\..\bin\Release64\ProcessHacker.exe;                    DestDir: {app};                                                    Flags: ignoreversion; Check: Is64BitInstallMode()

Source: ..\..\KProcessHacker\bin-signed\i386\kprocesshacker.sys;  DestDir: {app};                                                    Flags: ignoreversion; Check: not Is64BitInstallMode()
Source: ..\..\KProcessHacker\bin-signed\amd64\kprocesshacker.sys; DestDir: {app};                                                    Flags: ignoreversion; Check: Is64BitInstallMode()

Source: ..\..\bin\Release32\peview.exe;                           DestDir: {app};         Components: peview;                        Flags: ignoreversion; Check: not Is64BitInstallMode()
Source: ..\..\bin\Release64\peview.exe;                           DestDir: {app};         Components: peview;                        Flags: ignoreversion; Check: Is64BitInstallMode()

Source: ..\..\bin\Release32\plugins\DotNetTools.dll;              DestDir: {app}\plugins; Components: plugins\dotnettools;           Flags: ignoreversion; Check: not Is64BitInstallMode()
Source: ..\..\bin\Release64\plugins\DotNetTools.dll;              DestDir: {app}\plugins; Components: plugins\dotnettools;           Flags: ignoreversion; Check: Is64BitInstallMode()
Source: ..\..\bin\Release32\plugins\ExtendedNotifications.dll;    DestDir: {app}\plugins; Components: plugins\extendednotifications; Flags: ignoreversion; Check: not Is64BitInstallMode()
Source: ..\..\bin\Release64\plugins\ExtendedNotifications.dll;    DestDir: {app}\plugins; Components: plugins\extendednotifications; Flags: ignoreversion; Check: Is64BitInstallMode()
Source: ..\..\bin\Release32\plugins\ExtendedServices.dll;         DestDir: {app}\plugins; Components: plugins\extendedservices;      Flags: ignoreversion; Check: not Is64BitInstallMode()
Source: ..\..\bin\Release64\plugins\ExtendedServices.dll;         DestDir: {app}\plugins; Components: plugins\extendedservices;      Flags: ignoreversion; Check: Is64BitInstallMode()
Source: ..\..\bin\Release32\plugins\ExtendedTools.dll;            DestDir: {app}\plugins; Components: plugins\extendedtools;         Flags: ignoreversion; Check: not Is64BitInstallMode()
Source: ..\..\bin\Release64\plugins\ExtendedTools.dll;            DestDir: {app}\plugins; Components: plugins\extendedtools;         Flags: ignoreversion; Check: Is64BitInstallMode()
Source: ..\..\bin\Release32\plugins\NetworkTools.dll;             DestDir: {app}\plugins; Components: plugins\networktools;          Flags: ignoreversion; Check: not Is64BitInstallMode()
Source: ..\..\bin\Release64\plugins\NetworkTools.dll;             DestDir: {app}\plugins; Components: plugins\networktools;          Flags: ignoreversion; Check: Is64BitInstallMode()
Source: ..\..\bin\Release32\plugins\OnlineChecks.dll;             DestDir: {app}\plugins; Components: plugins\onlinechecks;          Flags: ignoreversion; Check: not Is64BitInstallMode()
Source: ..\..\bin\Release64\plugins\OnlineChecks.dll;             DestDir: {app}\plugins; Components: plugins\onlinechecks;          Flags: ignoreversion; Check: Is64BitInstallMode()
Source: ..\..\bin\Release32\plugins\SbieSupport.dll;              DestDir: {app}\plugins; Components: plugins\sbiesupport;           Flags: ignoreversion; Check: not Is64BitInstallMode()
Source: ..\..\bin\Release64\plugins\SbieSupport.dll;              DestDir: {app}\plugins; Components: plugins\sbiesupport;           Flags: ignoreversion; Check: Is64BitInstallMode()
Source: ..\..\bin\Release32\plugins\ToolStatus.dll;               DestDir: {app}\plugins; Components: plugins\toolstatus;            Flags: ignoreversion; Check: not Is64BitInstallMode()
Source: ..\..\bin\Release64\plugins\ToolStatus.dll;               DestDir: {app}\plugins; Components: plugins\toolstatus;            Flags: ignoreversion; Check: Is64BitInstallMode()
Source: ..\..\bin\Release32\plugins\Updater.dll;                  DestDir: {app}\plugins; Components: plugins\updater;               Flags: ignoreversion; Check: not Is64BitInstallMode()
Source: ..\..\bin\Release64\plugins\Updater.dll;                  DestDir: {app}\plugins; Components: plugins\updater;               Flags: ignoreversion; Check: Is64BitInstallMode()
Source: ..\..\bin\Release32\plugins\UserNotes.dll;                DestDir: {app}\plugins; Components: plugins\usernotes;             Flags: ignoreversion; Check: not Is64BitInstallMode()
Source: ..\..\bin\Release64\plugins\UserNotes.dll;                DestDir: {app}\plugins; Components: plugins\usernotes;             Flags: ignoreversion; Check: Is64BitInstallMode()
Source: ..\..\bin\Release32\plugins\WindowExplorer.dll;           DestDir: {app}\plugins; Components: plugins\windowexplorer;        Flags: ignoreversion; Check: not Is64BitInstallMode()
Source: ..\..\bin\Release64\plugins\WindowExplorer.dll;           DestDir: {app}\plugins; Components: plugins\windowexplorer;        Flags: ignoreversion; Check: Is64BitInstallMode()

Source: Icons\uninstall.ico;                                      DestDir: {app};                                                    Flags: ignoreversion


[Icons]
Name: {group}\PE Viewer;        Filename: {app}\peview.exe;        WorkingDir: {app}; Comment: PE Viewer; IconFilename: {app}\peview.exe; IconIndex: 0; Components: peview; Flags: excludefromshowinnewinstall
Name: {group}\Process Hacker 2; Filename: {app}\ProcessHacker.exe; WorkingDir: {app}; Comment: Process Hacker {#app_version_full}; IconFilename: {app}\ProcessHacker.exe; IconIndex: 0
Name: {group}\{cm:sm_Help}\{cm:sm_Changelog}; Filename: {app}\CHANGELOG.txt; WorkingDir: {app}; Comment: {cm:sm_com_Changelog}
Name: {group}\{cm:sm_Help}\{cm:ProgramOnTheWeb,Process Hacker 2}; Filename: http://processhacker.sourceforge.net/; Comment: {cm:ProgramOnTheWeb,Process Hacker 2}
Name: {group}\{cm:UninstallProgram,Process Hacker 2};             Filename: {uninstallexe}; WorkingDir: {app};     Comment: {cm:UninstallProgram,Process Hacker 2}; IconFilename: {app}\uninstall.ico

Name: {commondesktop}\Process Hacker 2; Filename: {app}\ProcessHacker.exe; WorkingDir: {app}; Comment: Process Hacker {#app_version_full}; IconFilename: {app}\ProcessHacker.exe; IconIndex: 0; Tasks: desktopicon\common
Name: {userdesktop}\Process Hacker 2;   Filename: {app}\ProcessHacker.exe; WorkingDir: {app}; Comment: Process Hacker {#app_version_full}; IconFilename: {app}\ProcessHacker.exe; IconIndex: 0; Tasks: desktopicon\user
Name: {#quick_launch}\Process Hacker 2; Filename: {app}\ProcessHacker.exe; WorkingDir: {app}; Comment: Process Hacker {#app_version_full}; IconFilename: {app}\ProcessHacker.exe; IconIndex: 0; Tasks: quicklaunchicon


[InstallDelete]
Type: files;      Name: {userdesktop}\Process Hacker 2.lnk;          Check: not IsTaskSelected('desktopicon\user')   and IsUpgrade()
Type: files;      Name: {commondesktop}\Process Hacker 2.lnk;        Check: not IsTaskSelected('desktopicon\common') and IsUpgrade()
Type: files;      Name: {#quick_launch}\Process Hacker 2.lnk;        Check: not IsTaskSelected('quicklaunchicon')    and IsUpgrade(); OnlyBelowVersion: 0,6.01
Type: files;      Name: {group}\Help and Support\Process Hacker Help.lnk; Check: IsUpgrade()

Type: files;      Name: {userappdata}\Process Hacker 2\settings.xml; Tasks: reset_settings
Type: dirifempty; Name: {userappdata}\Process Hacker;                Tasks: reset_settings

Type: files;      Name: {app}\Help.htm;                              Check: IsUpgrade()
Type: files;      Name: {app}\peview.exe;                            Check: not IsComponentSelected('peview')                        and IsUpgrade()
Type: files;      Name: {group}\PE Viewer.lnk;                       Check: not IsComponentSelected('peview')                        and IsUpgrade()
Type: files;      Name: {app}\plugins\DotNetTools.dll;               Check: not IsComponentSelected('plugins\dotnettools')           and IsUpgrade()
Type: files;      Name: {app}\plugins\ExtendedNotifications.dll;     Check: not IsComponentSelected('plugins\extendednotifications') and IsUpgrade()
Type: files;      Name: {app}\plugins\ExtendedServices.dll;          Check: not IsComponentSelected('plugins\extendedservices')      and IsUpgrade()
Type: files;      Name: {app}\plugins\ExtendedTools.dll;             Check: not IsComponentSelected('plugins\extendedtools')         and IsUpgrade()
Type: files;      Name: {app}\plugins\NetworkTools.dll;              Check: not IsComponentSelected('plugins\networktools')          and IsUpgrade()
Type: files;      Name: {app}\plugins\OnlineChecks.dll;              Check: not IsComponentSelected('plugins\onlinechecks')          and IsUpgrade()
Type: files;      Name: {app}\plugins\SbieSupport.dll;               Check: not IsComponentSelected('plugins\sbiesupport')           and IsUpgrade()
Type: files;      Name: {app}\plugins\ToolStatus.dll;                Check: not IsComponentSelected('plugins\toolstatus')            and IsUpgrade()
Type: files;      Name: {app}\plugins\Updater.dll;                   Check: not IsComponentSelected('plugins\updater')               and IsUpgrade()
Type: files;      Name: {app}\plugins\UserNotes.dll;                 Check: not IsComponentSelected('plugins\usernotes')             and IsUpgrade()
Type: files;      Name: {app}\plugins\WindowExplorer.dll;            Check: not IsComponentSelected('plugins\windowexplorer')        and IsUpgrade()
Type: dirifempty; Name: {app}\plugins


[Run]
Filename: {app}\ProcessHacker.exe;               Description: {cm:LaunchProgram,Process Hacker 2}; Flags: nowait postinstall skipifsilent runascurrentuser
Filename: {app}\CHANGELOG.txt;                   Description: {cm:run_ViewChangelog};              Flags: nowait postinstall skipifsilent unchecked shellexec
Filename: http://processhacker.sourceforge.net/; Description: {cm:run_VisitWebsite};               Flags: nowait postinstall skipifsilent unchecked shellexec


[Code]
const
  installer_mutex = 'process_hacker2_setup_mutex';
  IFEO            = 'SOFTWARE\Microsoft\Windows NT\CurrentVersion\Image File Execution Options\taskmgr.exe';
  HKCURUN         = 'Software\Microsoft\Windows\CurrentVersion\Run';


function IsUpgrade(): Boolean;
var
  sPrevPath: String;
begin
  sPrevPath := WizardForm.PrevAppDir;
  Result := (sPrevPath <> '');
end;


function ShouldSkipPage(PageID: Integer): Boolean;
begin
  // Hide the License and the Ready to install page if it's an upgrade
  if IsUpgrade() and (PageID = wpLicense) or (PageID = wpReady) then
    Result := True;
end;


// Check if KProcessHacker is installed as a service
function KPHServiceCheck(): Boolean;
var
  dwStart: DWORD;
begin
  if RegQueryDWordValue(HKLM, 'SYSTEM\CurrentControlSet\Services\KProcessHacker2', 'Start', dwStart) then begin
    if dwStart = 1 then
      Result := True;
  end else
    Result := False;
end;


// Check if Process Hacker is set as the default Task Manager for Windows
function PHDefaulTaskmgrCheck(): Boolean;
var
  sDebugger: String;
begin
  if RegQueryStringValue(HKLM, IFEO, 'Debugger', sDebugger) then begin
    if sDebugger = (ExpandConstant('"{app}\ProcessHacker.exe"')) then
      Result := True;
  end else
    Result := False;
end;


// Check if Process Hacker's settings exist
function SettingsExistCheck(): Boolean;
begin
  if FileExists(ExpandConstant('{userappdata}\Process Hacker 2\settings.xml')) then
    Result := True
  else
    Result := False;
end;


// Check if Process Hacker is configured to run on startup in order to control
// startup choice from within the installer
function StartupCheck(): Boolean;
var
  svalue: String;
begin
  if RegQueryStringValue(HKCU, HKCURUN, 'Process Hacker 2', svalue) then begin
    if (svalue = (ExpandConstant('"{app}\ProcessHacker.exe"'))) or (svalue = (ExpandConstant('"{app}\ProcessHacker.exe" -hide'))) then
      Result := True;
  end else
    Result := False;
end;


procedure CurPageChanged(CurPageID: Integer);
begin
  if IsUpgrade() and (CurPageID = wpSelectTasks) then
    WizardForm.NextButton.Caption := SetupMessage(msgButtonInstall);
end;


procedure CurStepChanged(CurStep: TSetupStep);
begin
  if CurStep = ssInstall then begin
    if IsServiceRunning('KProcessHacker2') then
      StopService('KProcessHacker2');
    if IsTaskSelected('delete_KPH_service') then
      RemoveService('KProcessHacker2');
  end;

  if CurStep = ssPostInstall then begin
    if (KPHServiceCheck() and not IsTaskSelected('delete_KPH_service') or (IsTaskSelected('create_KPH_service'))) then begin
      StopService('KProcessHacker2');
      RemoveService('KProcessHacker2');
      InstallService(ExpandConstant('{app}\kprocesshacker.sys'), 'KProcessHacker2', 'KProcessHacker2', 'KProcessHacker2 driver', SERVICE_KERNEL_DRIVER, SERVICE_SYSTEM_START);
      StartService('KProcessHacker2');
    end;

    if IsTaskSelected('set_default_taskmgr') then
      RegWriteStringValue(HKLM, IFEO, 'Debugger', ExpandConstant('"{app}\ProcessHacker.exe"'));
    if IsTaskSelected('restore_taskmgr') then begin
      RegDeleteValue(HKLM, IFEO, 'Debugger');
      RegDeleteKeyIfEmpty(HKLM, IFEO);
    end;

    if IsTaskSelected('startup') then
      RegWriteStringValue(HKCU, HKCURUN, 'Process Hacker 2', ExpandConstant('"{app}\ProcessHacker.exe"'));
    if IsTaskSelected('startup\minimized') then
      RegWriteStringValue(HKCU, HKCURUN, 'Process Hacker 2', ExpandConstant('"{app}\ProcessHacker.exe" -hide'));
    if IsTaskSelected('remove_startup') then
      RegDeleteValue(HKCU, HKCURUN, 'Process Hacker 2');

  end;
end;


procedure CurUninstallStepChanged(CurUninstallStep: TUninstallStep);
begin
  if CurUninstallStep = usUninstall then begin
    StopService('KProcessHacker2');
    RemoveService('KProcessHacker2');

    // When uninstalling ask user to delete Process Hacker's settings
    // based on whether the settings file exists only
    if SettingsExistCheck() then begin
      if SuppressibleMsgBox(CustomMessage('msg_DeleteLogSettings'), mbConfirmation, MB_YESNO or MB_DEFBUTTON2, IDNO) = IDYES then
        DeleteFile(ExpandConstant('{userappdata}\Process Hacker 2\settings.xml'));
    end;

    if PHDefaulTaskmgrCheck() then
      RegDeleteValue(HKLM, IFEO, 'Debugger');
    RegDeleteKeyIfEmpty(HKLM, IFEO);
    if StartupCheck() then
      RegDeleteValue(HKCU, HKCURUN, 'Process Hacker 2');

    RemoveDir(ExpandConstant('{userappdata}\Process Hacker 2'));
    RemoveDir(ExpandConstant('{app}\plugins'));
    RemoveDir(ExpandConstant('{app}'));

  end;
end;


procedure InitializeWizard();
begin
  WizardForm.SelectTasksLabel.Hide;
  WizardForm.TasksList.Top    := 0;
  WizardForm.TasksList.Height := PageFromID(wpSelectTasks).SurfaceHeight;
end;


function InitializeSetup(): Boolean;
begin
  // Create a mutex for the installer and if it's already running then expose a message and stop installation
  if CheckForMutexes(installer_mutex) and not WizardSilent() then begin
    SuppressibleMsgBox(CustomMessage('msg_SetupIsRunningWarning'), mbError, MB_OK, MB_OK);
    Result := False;
  end
  else begin
    Result := True;
    CreateMutex(installer_mutex);
  end;
end;


function InitializeUninstall(): Boolean;
begin
  if CheckForMutexes(installer_mutex) then begin
    SuppressibleMsgBox(CustomMessage('msg_SetupIsRunningWarning'), mbError, MB_OK, MB_OK);
    Result := False;
  end
  else begin
    Result := True;
    CreateMutex(installer_mutex);
  end;
end;
