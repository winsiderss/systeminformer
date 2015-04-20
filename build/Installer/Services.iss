;* Original source: http://goo.gl/PTi56
;*
;* Process Hacker - Various services functions
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


[Code]
// Various service related functions

type
  SERVICE_STATUS = record
    dwServiceType             : cardinal;
    dwCurrentState            : cardinal;
    dwControlsAccepted        : cardinal;
    dwWin32ExitCode           : cardinal;
    dwServiceSpecificExitCode : cardinal;
    dwCheckPoint              : cardinal;
    dwWaitHint                : cardinal;
  end;
  HANDLE = cardinal;

const
  SERVICE_QUERY_CONFIG        = $1;
  SERVICE_CHANGE_CONFIG       = $2;
  SERVICE_QUERY_STATUS        = $4;
  SERVICE_START               = $10;
  SERVICE_STOP                = $20;
  SERVICE_ALL_ACCESS          = $f01ff;
  SC_MANAGER_ALL_ACCESS       = $f003f;
  SERVICE_KERNEL_DRIVER       = $1;
  SERVICE_WIN32_OWN_PROCESS   = $10;
  SERVICE_WIN32_SHARE_PROCESS = $20;
  SERVICE_WIN32               = $30;
  SERVICE_INTERACTIVE_PROCESS = $100;
  SERVICE_BOOT_START          = $0;
  SERVICE_SYSTEM_START        = $1;
  SERVICE_AUTO_START          = $2;
  SERVICE_DEMAND_START        = $3;
  SERVICE_DISABLED            = $4;
  SERVICE_DELETE              = $10000;
  SERVICE_CONTROL_STOP        = $1;
  SERVICE_CONTROL_PAUSE       = $2;
  SERVICE_CONTROL_CONTINUE    = $3;
  SERVICE_CONTROL_INTERROGATE = $4;
  SERVICE_STOPPED             = $1;
  SERVICE_START_PENDING       = $2;
  SERVICE_STOP_PENDING        = $3;
  SERVICE_RUNNING             = $4;
  SERVICE_CONTINUE_PENDING    = $5;
  SERVICE_PAUSE_PENDING       = $6;
  SERVICE_PAUSED              = $7;

// #######################################################################################
// nt based service utilities
// #######################################################################################
function OpenSCManager(lpMachineName, lpDatabaseName: AnsiString; dwDesiredAccess: cardinal): HANDLE;
external 'OpenSCManagerA@advapi32.dll stdcall';

function OpenService(hSCManager: HANDLE; lpServiceName: AnsiString; dwDesiredAccess: cardinal): HANDLE;
external 'OpenServiceA@advapi32.dll stdcall';

function CloseServiceHandle(hSCObject: HANDLE): Boolean;
external 'CloseServiceHandle@advapi32.dll stdcall';

function CreateService(hSCManager: HANDLE; lpServiceName, lpDisplayName: AnsiString; dwDesiredAccess, dwServiceType, dwStartType, dwErrorControl: cardinal; lpBinaryPathName, lpLoadOrderGroup: AnsiString; lpdwTagId: cardinal; lpDependencies, lpServiceStartName, lpPassword: AnsiString): cardinal;
external 'CreateServiceA@advapi32.dll stdcall';

function DeleteService(hService: HANDLE): Boolean;
external 'DeleteService@advapi32.dll stdcall';

function StartNTService(hService: HANDLE; dwNumServiceArgs: cardinal; lpServiceArgVectors: cardinal): Boolean;
external 'StartServiceA@advapi32.dll stdcall';

function ControlService(hService: HANDLE; dwControl: cardinal; var ServiceStatus: SERVICE_STATUS): Boolean;
external 'ControlService@advapi32.dll stdcall';

function QueryServiceStatus(hService: HANDLE; var ServiceStatus: SERVICE_STATUS): Boolean;
external 'QueryServiceStatus@advapi32.dll stdcall';

function QueryServiceStatusEx(hService: HANDLE; ServiceStatus: SERVICE_STATUS): Boolean;
external 'QueryServiceStatus@advapi32.dll stdcall';


function OpenServiceManager(): HANDLE;
begin
  Result := OpenSCManager('', 'ServicesActive', SC_MANAGER_ALL_ACCESS);
  if Result = 0 then
    SuppressibleMsgBox(CustomMessage('msg_ServiceManager'), mbError, MB_OK, MB_OK);
end;


function InstallService(FileName, ServiceName, DisplayName, Description: AnsiString; ServiceType, StartType: cardinal): Boolean;
var
  hSCM     : HANDLE;
  hService : HANDLE;
begin
  hSCM   := OpenServiceManager();
  Result := False;
  if hSCM <> 0 then begin
    hService := CreateService(hSCM, ServiceName, DisplayName, SERVICE_ALL_ACCESS, ServiceType, StartType, 0, FileName, '', 0, '', '', '');
    if hService <> 0 then begin
      Result := True;
      // Win2K & WinXP supports aditional description text for services
      if Description <> '' then
        RegWriteStringValue(HKLM, 'System\CurrentControlSet\Services\' + ServiceName, 'Description', Description);
      CloseServiceHandle(hService);
    end;
    CloseServiceHandle(hSCM);
  end;
end;


function RemoveService(ServiceName: String): Boolean;
var
  hSCM     : HANDLE;
  hService : HANDLE;
begin
  hSCM   := OpenServiceManager();
  Result := False;
  if hSCM <> 0 then begin
    hService := OpenService(hSCM, ServiceName, SERVICE_DELETE);
    if hService <> 0 then begin
      Result := DeleteService(hService);
      CloseServiceHandle(hService);
    end;
    CloseServiceHandle(hSCM);
  end;
end;


function StartService(ServiceName: String): Boolean;
var
  hSCM     : HANDLE;
  hService : HANDLE;
begin
  hSCM   := OpenServiceManager();
  Result := False;
  if hSCM <> 0 then begin
    hService := OpenService(hSCM, ServiceName, SERVICE_START);
    if hService <> 0 then begin
      Result := StartNTService(hService, 0, 0);
      CloseServiceHandle(hService);
    end;
    CloseServiceHandle(hSCM);
  end;
end;


function StopService(ServiceName: String): Boolean;
var
  hSCM     : HANDLE;
  hService : HANDLE;
  Status   : SERVICE_STATUS;
begin
  hSCM   := OpenServiceManager();
  Result := False;
  if hSCM <> 0 then begin
    hService := OpenService(hSCM, ServiceName, SERVICE_STOP);
    if hService <> 0 then begin
      Result := ControlService(hService, SERVICE_CONTROL_STOP, Status);
      CloseServiceHandle(hService);
    end;
    CloseServiceHandle(hSCM);
  end;
end;


function IsServiceRunning(ServiceName: String): Boolean;
var
  hSCM     : HANDLE;
  hService : HANDLE;
  Status   : SERVICE_STATUS;
begin
  hSCM   := OpenServiceManager();
  Result := False;
  if hSCM <> 0 then begin
    hService := OpenService(hSCM, ServiceName, SERVICE_QUERY_STATUS);
    if hService <> 0 then begin
      if QueryServiceStatus(hService, Status) then begin
        Result := (Status.dwCurrentState = SERVICE_RUNNING);
      end;
      CloseServiceHandle(hService);
    end;
    CloseServiceHandle(hSCM);
  end;
end;
