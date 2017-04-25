/*
 * Process Hacker Toolchain -
 *   project setup
 *
 * Copyright (C) 2017 dmex
 *
 * This file is part of Process Hacker.
 *
 * Process Hacker is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation, either version 3 of the License, or
 * (at your option) any later version.
 *
 * Process Hacker is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with Process Hacker.  If not, see <http://www.gnu.org/licenses/>.
 */

#include <setup.h>
#include <appsup.h>
#include <svcsup.h>

PH_STRINGREF UninstallKeyName = PH_STRINGREF_INIT(L"Software\\Microsoft\\Windows\\CurrentVersion\\Uninstall\\ProcessHacker");

NTSTATUS SetupCreateUninstallKey(
    VOID
    )
{
    NTSTATUS status;
    HANDLE keyHandle;
    UNICODE_STRING name;
    UNICODE_STRING value;
    ULONG ulong = 1;

    status = PhCreateKey(
        &keyHandle,
        KEY_ALL_ACCESS,
        PH_KEY_LOCAL_MACHINE,
        &UninstallKeyName,
        0,
        0,
        NULL
        );

    if (NT_SUCCESS(status))
    {
        PPH_STRING tempString;
        
        tempString = PhConcatStrings2(PhGetString(SetupInstallPath), L"\\processhacker.exe,0");
        PhStringRefToUnicodeString(&tempString->sr, &value);
        RtlInitUnicodeString(&name, L"DisplayIcon");
        PhStringRefToUnicodeString(&tempString->sr, &value);
        NtSetValueKey(keyHandle, &name, 0, REG_SZ, value.Buffer, (ULONG)value.MaximumLength);
        PhDereferenceObject(tempString);

        RtlInitUnicodeString(&name, L"DisplayName");
        RtlInitUnicodeString(&value, L"Process Hacker");
        NtSetValueKey(keyHandle, &name, 0, REG_SZ, value.Buffer, (ULONG)value.MaximumLength);

        RtlInitUnicodeString(&name, L"DisplayVersion");
        RtlInitUnicodeString(&value, L"3.x");
        NtSetValueKey(keyHandle, &name, 0, REG_SZ, value.Buffer, (ULONG)value.MaximumLength);

        RtlInitUnicodeString(&name, L"HelpLink");
        RtlInitUnicodeString(&value, L"http://wj32.org/processhacker/forums/");
        NtSetValueKey(keyHandle, &name, 0, REG_SZ, value.Buffer, (ULONG)value.MaximumLength);

        RtlInitUnicodeString(&name, L"InstallLocation");
        RtlInitUnicodeString(&value, PhGetString(SetupInstallPath));
        NtSetValueKey(keyHandle, &name, 0, REG_SZ, value.Buffer, (ULONG)value.MaximumLength);

        RtlInitUnicodeString(&name, L"Publisher");
        RtlInitUnicodeString(&value, L"Process Hacker");
        NtSetValueKey(keyHandle, &name, 0, REG_SZ, value.Buffer, (ULONG)value.MaximumLength);

        RtlInitUnicodeString(&name, L"UninstallString");
        tempString = PhFormatString(L"\"%s\\processhacker-setup.exe\" -uninstall", PhGetString(SetupInstallPath));
        PhStringRefToUnicodeString(&tempString->sr, &value);
        NtSetValueKey(keyHandle, &name, 0, REG_SZ, value.Buffer, (ULONG)value.MaximumLength);
        PhDereferenceObject(tempString);

        RtlInitUnicodeString(&value, L"NoModify");
        ulong = TRUE;
        NtSetValueKey(keyHandle, &value, 0, REG_DWORD, &ulong, sizeof(ULONG));

        RtlInitUnicodeString(&value, L"NoRepair");
        ulong = TRUE;
        NtSetValueKey(keyHandle, &value, 0, REG_DWORD, &ulong, sizeof(ULONG));

        NtClose(keyHandle);
    }

    return status;
}

NTSTATUS SetupDeleteUninstallKey(
    VOID
    )
{
    NTSTATUS status;
    HANDLE keyHandle;

    status = PhOpenKey(
        &keyHandle,
        KEY_WRITE | DELETE,
        PH_KEY_LOCAL_MACHINE,
        &UninstallKeyName,
        0
        );

    if (NT_SUCCESS(status))
    {
        status = NtDeleteKey(keyHandle);
        NtClose(keyHandle);
    }

    return status;
}

VOID SetupFindInstallDirectory(
    VOID
    )
{
    // Find the current installation path.
    SetupInstallPath = GetProcessHackerInstallPath();

    // Check if the string is valid.
    if (PhIsNullOrEmptyString(SetupInstallPath))
    {
        PH_STRINGREF programW6432 = PH_STRINGREF_INIT(L"%ProgramW6432%");
        PH_STRINGREF programFiles = PH_STRINGREF_INIT(L"%ProgramFiles%");
        PH_STRINGREF defaultDirectoryName = PH_STRINGREF_INIT(L"\\Process Hacker\\");
        SYSTEM_INFO info;
        PPH_STRING expandedString;

        GetNativeSystemInfo(&info);

        if (info.wProcessorArchitecture == PROCESSOR_ARCHITECTURE_AMD64)
        {
            if (expandedString = PH_AUTO(PhExpandEnvironmentStrings(&programW6432)))
            {
                SetupInstallPath = PhConcatStringRef2(&expandedString->sr, &defaultDirectoryName);
            }
        }
        else
        {
            if (expandedString = PH_AUTO(PhExpandEnvironmentStrings(&programFiles)))
            {
                SetupInstallPath = PhConcatStringRef2(&expandedString->sr, &defaultDirectoryName);
            }
        }
    }

    if (PhIsNullOrEmptyString(SetupInstallPath))
    {
        SetupInstallPath = PhCreateString(L"C:\\Program Files\\Process Hacker\\");
    }

    // Remove extra backslashes
    PathRemoveBackslash(PhGetString(SetupInstallPath));
}

VOID SetupCreateUninstallFile(
    VOID
    )
{
    PPH_STRING backupFilePath;
    PPH_STRING uninstallFilePath;

    backupFilePath = PhConcatStrings2(PhGetString(SetupInstallPath), L"\\processhacker-setup.bak");
    uninstallFilePath = PhConcatStrings2(PhGetString(SetupInstallPath), L"\\processhacker-setup.exe");

    if (RtlDoesFileExists_U(backupFilePath->Buffer))
    {
        // Move to temp directory
        //MoveFile(uninstallFilePath->Buffer, backupFilePath->Buffer);
    }

    if (RtlDoesFileExists_U(uninstallFilePath->Buffer))
    {
        MoveFile(uninstallFilePath->Buffer, backupFilePath->Buffer);
    }

    CopyFile(NtCurrentPeb()->ProcessParameters->ImagePathName.Buffer, uninstallFilePath->Buffer, FALSE);

    PhDereferenceObject(uninstallFilePath);
}

VOID SetupInstallKph(
    VOID
    )
{
    PPH_STRING clientPath = PhFormatString(L"%s\\ProcessHacker.exe", PhGetString(SetupInstallPath));

    if (RtlDoesFileExists_U(PhGetString(clientPath)))
    {
        HANDLE processHandle;
        LARGE_INTEGER timeout;

        timeout.QuadPart = -10 * PH_TIMEOUT_SEC;

        if (PhShellExecuteEx(
            NULL,
            PhGetString(clientPath),
            L"-installkph",
            SW_NORMAL,
            0,
            0,
            &processHandle
            ))
        {
            NtWaitForSingleObject(processHandle, FALSE, &timeout);
        }

        SC_HANDLE serviceHandle;
        BOOLEAN success = FALSE;

        serviceHandle = PhOpenService(L"KProcessHacker3", SERVICE_START);

        if (serviceHandle)
        {
            if (StartService(serviceHandle, 0, NULL))
                success = TRUE;

            CloseServiceHandle(serviceHandle);
        }
    }

    PhDereferenceObject(clientPath);
}

ULONG SetupUninstallKph(
    VOID
    )
{
    ULONG status = ERROR_SUCCESS;
    SC_HANDLE scmHandle;
    SC_HANDLE serviceHandle;
    SERVICE_STATUS serviceStatus;

    if (!(scmHandle = OpenSCManager(NULL, NULL, SC_MANAGER_CONNECT)))
        return GetLastError();

    if (serviceHandle = OpenService(scmHandle, L"KProcessHacker3", SERVICE_STOP | DELETE))
    {
        ControlService(serviceHandle, SERVICE_CONTROL_STOP, &serviceStatus);

        if (!DeleteService(serviceHandle))
            status = GetLastError();

        CloseServiceHandle(serviceHandle);
    }
    else
    {
        status = GetLastError();
    }

    CloseServiceHandle(scmHandle);

    return status;
}

VOID SetupSetWindowsOptions(
    VOID
    )
{
    static PH_STRINGREF TaskMgrImageOptionsKeyName = PH_STRINGREF_INIT(L"Software\\Microsoft\\Windows NT\\CurrentVersion\\Image File Execution Options\\taskmgr.exe");
    static PH_STRINGREF CurrentUserRunKeyName = PH_STRINGREF_INIT(L"Software\\Microsoft\\Windows\\CurrentVersion\\Run");
    PPH_STRING clientPathString;
    PPH_STRING startmenuFolderString;

    clientPathString = PhConcatStrings2(PhGetString(SetupInstallPath), L"\\ProcessHacker.exe");

    if (startmenuFolderString = PhGetKnownLocation(CSIDL_COMMON_PROGRAMS, L"\\Process Hacker.lnk"))
    {
        SetupCreateLink(
            PhGetString(startmenuFolderString),
            PhGetString(clientPathString),
            PhGetString(SetupInstallPath)
            );
        PhDereferenceObject(startmenuFolderString);
    }

    if (startmenuFolderString = PhGetKnownLocation(CSIDL_COMMON_PROGRAMS, L"\\PE Viewer.lnk"))
    {
        PPH_STRING peviewPathString = PhConcatStrings2(PhGetString(SetupInstallPath), L"\\peview.exe");

        SetupCreateLink(
            PhGetString(startmenuFolderString),
            PhGetString(peviewPathString),
            PhGetString(SetupInstallPath)
            );

        PhDereferenceObject(peviewPathString);
        PhDereferenceObject(startmenuFolderString);
    }

    if (SetupResetSettings)
    {
        PPH_STRING settingsFileName = PhGetKnownLocation(CSIDL_APPDATA, L"\\Process Hacker\\settings.xml");

        SetupDeleteDirectoryFile(settingsFileName->Buffer);
        PhDereferenceObject(settingsFileName);
    }

    if (SetupCreateDefaultTaskManager)
    {
        HANDLE keyHandle;
        
        if (NT_SUCCESS(PhOpenKey(
            &keyHandle,
            KEY_WRITE,
            PH_KEY_LOCAL_MACHINE,
            &TaskMgrImageOptionsKeyName,
            0
            )))
        {
            PPH_STRING value;
            UNICODE_STRING valueName;
     
            value = PhConcatStrings(3, L"\"", PhGetString(clientPathString), L"\"");

            RtlInitUnicodeString(&valueName, L"Debugger");
            NtSetValueKey(keyHandle, &valueName, 0, REG_SZ, value->Buffer, (ULONG)value->Length + 2);
            NtClose(keyHandle);
        }
    }

    if (SetupCreateSystemStartup)
    {     
        HANDLE keyHandle;

        if (NT_SUCCESS(PhOpenKey(
            &keyHandle,
            KEY_WRITE,
            PH_KEY_CURRENT_USER,
            &CurrentUserRunKeyName,
            0
            )))
        {
            PPH_STRING value;
            UNICODE_STRING valueName;

            RtlInitUnicodeString(&valueName, L"Process Hacker");

            if (SetupCreateMinimizedSystemStartup)
                value = PhConcatStrings(3, L"\"", PhGetString(clientPathString), L"\" -hide");
            else
                value = PhConcatStrings(3, L"\"", PhGetString(clientPathString), L"\"");
       
            NtSetValueKey(keyHandle, &valueName, 0, REG_SZ, value->Buffer, (ULONG)value->Length + 2);
            NtClose(keyHandle);
        }
    }

    if (SetupCreateDesktopShortcut)
    {
        PPH_STRING desktopFolderString;

        if (desktopFolderString = PhGetKnownLocation(CSIDL_DESKTOPDIRECTORY, L"\\Process Hacker.lnk"));
        {
            SetupCreateLink(
                PhGetString(desktopFolderString), 
                PhGetString(clientPathString), 
                PhGetString(SetupInstallPath)
                );
            PhDereferenceObject(desktopFolderString);
        }
    }
    else if (SetupCreateDesktopShortcutAllUsers)
    {
        PPH_STRING startmenuFolderString;

        if (startmenuFolderString = PhGetKnownLocation(CSIDL_COMMON_DESKTOPDIRECTORY, L"\\Process Hacker.lnk"))
        {
            SetupCreateLink(
                PhGetString(startmenuFolderString),
                PhGetString(clientPathString),
                PhGetString(SetupInstallPath)
                );
            PhDereferenceObject(startmenuFolderString);
        }
    }
}

VOID SetupDeleteWindowsOptions(
    VOID
    )
{
    static PH_STRINGREF TaskMgrImageOptionsKeyName = PH_STRINGREF_INIT(L"Software\\Microsoft\\Windows NT\\CurrentVersion\\Image File Execution Options\\taskmgr.exe");
    static PH_STRINGREF CurrentUserRunKeyName = PH_STRINGREF_INIT(L"Software\\Microsoft\\Windows\\CurrentVersion\\Run");
    PPH_STRING startmenuFolderString;
    HANDLE keyHandle;

    if (startmenuFolderString = PhGetKnownLocation(CSIDL_COMMON_PROGRAMS, L"\\Process Hacker.lnk"))
    {
        SetupDeleteDirectoryFile(PhGetString(startmenuFolderString));
        PhDereferenceObject(startmenuFolderString);
    }

    if (startmenuFolderString = PhGetKnownLocation(CSIDL_COMMON_PROGRAMS, L"\\PE Viewer.lnk"))
    {
        SetupDeleteDirectoryFile(PhGetString(startmenuFolderString));
        PhDereferenceObject(startmenuFolderString);
    }

    if (startmenuFolderString = PhGetKnownLocation(CSIDL_DESKTOPDIRECTORY, L"\\Process Hacker.lnk"));
    {
        SetupDeleteDirectoryFile(PhGetString(startmenuFolderString));
        PhDereferenceObject(startmenuFolderString);
    }

    if (startmenuFolderString = PhGetKnownLocation(CSIDL_COMMON_DESKTOPDIRECTORY, L"\\Process Hacker.lnk"))
    {
        SetupDeleteDirectoryFile(PhGetString(startmenuFolderString));
        PhDereferenceObject(startmenuFolderString);
    }

    //PPH_STRING settingsFileName = PhGetKnownLocation(CSIDL_APPDATA, L"\\Process Hacker\\settings.xml");
    //SetupDeleteDirectoryFile(settingsFileName->Buffer);
    //PhDereferenceObject(settingsFileName);

    if (NT_SUCCESS(PhOpenKey(
        &keyHandle,
        KEY_WRITE | DELETE,
        PH_KEY_LOCAL_MACHINE,
        &TaskMgrImageOptionsKeyName,
        0
        )))
    {
        NtDeleteKey(keyHandle);
        NtClose(keyHandle);
    }

    if (NT_SUCCESS(PhOpenKey(
        &keyHandle,
        KEY_WRITE,
        PH_KEY_CURRENT_USER,
        &CurrentUserRunKeyName,
        0
        )))
    {
        NtDeleteKey(keyHandle);
        NtClose(keyHandle);
    }
}

BOOLEAN SetupExecuteProcessHacker(
    _In_ HWND Parent
    )
{
    BOOLEAN success = FALSE;
    PPH_STRING clientPath;

    clientPath = PhConcatStrings2(PhGetString(SetupInstallPath), L"\\ProcessHacker.exe");

    if (RtlDoesFileExists_U(clientPath->Buffer))
    {
        success = PhShellExecuteEx(
            Parent,
            clientPath->Buffer,
            NULL,
            SW_SHOWDEFAULT,
            0,
            0,
            NULL
            );
    }

    PhDereferenceObject(clientPath);
    return success;
}

VOID SetupUpgradeSettingsFile(
    VOID
    )
{
    PPH_STRING settingsFilePath;
    PPH_STRING oldSettingsFileName;
    
    settingsFilePath = PhGetKnownLocation(CSIDL_APPDATA, L"\\Process Hacker\\settings.xml");
    oldSettingsFileName = PhGetKnownLocation(CSIDL_APPDATA, L"\\Process Hacker 2\\settings.xml");

    if (!RtlDoesFileExists_U(settingsFilePath->Buffer))
    {
        CopyFile(oldSettingsFileName->Buffer, settingsFilePath->Buffer, FALSE);
    }

    PhDereferenceObject(oldSettingsFileName);
    PhDereferenceObject(settingsFilePath);
}