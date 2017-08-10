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
    _In_ PPH_SETUP_CONTEXT Context
    )
{
    NTSTATUS status;
    HANDLE keyHandle;
    UNICODE_STRING name;
    UNICODE_STRING value;
    ULONG ulong = 1;

    status = PhCreateKey(
        &keyHandle,
        KEY_ALL_ACCESS | KEY_WOW64_64KEY,
        PH_KEY_LOCAL_MACHINE,
        &UninstallKeyName,
        0,
        0,
        NULL
        );

    if (NT_SUCCESS(status))
    {
        PPH_STRING tempString;
        
        tempString = PhConcatStrings2(PhGetString(Context->SetupInstallPath), L"\\processhacker.exe,0");
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
        RtlInitUnicodeString(&value, PhGetString(Context->SetupInstallPath));
        NtSetValueKey(keyHandle, &name, 0, REG_SZ, value.Buffer, (ULONG)value.MaximumLength);

        RtlInitUnicodeString(&name, L"Publisher");
        RtlInitUnicodeString(&value, L"Process Hacker");
        NtSetValueKey(keyHandle, &name, 0, REG_SZ, value.Buffer, (ULONG)value.MaximumLength);

        RtlInitUnicodeString(&name, L"UninstallString");
        tempString = PhFormatString(L"\"%s\\processhacker-setup.exe\" -uninstall", PhGetString(Context->SetupInstallPath));
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
        KEY_WRITE | DELETE | KEY_WOW64_64KEY,
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

PPH_STRING SetupFindInstallDirectory(
    VOID
    )
{
    // Find the current installation path.
    PPH_STRING setupInstallPath = GetProcessHackerInstallPath();

    // Check if the string is valid.
    if (PhIsNullOrEmptyString(setupInstallPath))
    {
        static PH_STRINGREF programW6432 = PH_STRINGREF_INIT(L"%ProgramW6432%");
        static PH_STRINGREF programFiles = PH_STRINGREF_INIT(L"%ProgramFiles%");
        static PH_STRINGREF defaultDirectoryName = PH_STRINGREF_INIT(L"\\Process Hacker\\");
        SYSTEM_INFO info;

        GetNativeSystemInfo(&info);

        if (info.wProcessorArchitecture == PROCESSOR_ARCHITECTURE_AMD64)
        {
            PPH_STRING expandedString;

            if (expandedString = PH_AUTO(PhExpandEnvironmentStrings(&programW6432)))
            {
                setupInstallPath = PhConcatStringRef2(&expandedString->sr, &defaultDirectoryName);
            }
        }
        else
        {
            PPH_STRING expandedString;

            if (expandedString = PH_AUTO(PhExpandEnvironmentStrings(&programFiles)))
            {
                setupInstallPath = PhConcatStringRef2(&expandedString->sr, &defaultDirectoryName);
            }
        }
    }

    if (PhIsNullOrEmptyString(setupInstallPath))
    {
        setupInstallPath = PhCreateString(L"C:\\Program Files\\Process Hacker\\");
    }

    // Remove extra backslashes
    PathRemoveBackslash(PhGetString(setupInstallPath));

    return setupInstallPath;
}

BOOLEAN SetupCreateUninstallFile(
    _In_ PPH_SETUP_CONTEXT Context
    )
{
    PPH_STRING currentFilePath;
    PPH_STRING backupFilePath;
    PPH_STRING uninstallFilePath;

    if (!NT_SUCCESS(PhGetProcessImageFileNameWin32(NtCurrentProcess(), &currentFilePath)))
        return FALSE;

    // Check if the user has started the setup from the installation folder.
    if (PhStartsWithStringRef2(&currentFilePath->sr, PhGetString(Context->SetupInstallPath), TRUE))
    {
        PhDereferenceObject(currentFilePath);
        return TRUE;
    }

    backupFilePath = PhConcatStrings2(PhGetString(Context->SetupInstallPath), L"\\processhacker-setup.bak");
    uninstallFilePath = PhConcatStrings2(PhGetString(Context->SetupInstallPath), L"\\processhacker-setup.exe");

    if (RtlDoesFileExists_U(backupFilePath->Buffer))
    {
        if (!NT_SUCCESS(PhDeleteFileWin32(backupFilePath->Buffer)))
        {
            PPH_STRING tempFileName;
            PPH_STRING tempFilePath;

            tempFileName = PhCreateString(L"processhacker-setup.old");
            tempFilePath = PhCreateCacheFile(tempFileName);

            if (!MoveFile(backupFilePath->Buffer, tempFilePath->Buffer))
            {
                Context->ErrorCode = GetLastError();
            }

            PhDereferenceObject(tempFilePath);
            PhDereferenceObject(tempFileName);
        }
    }

    if (RtlDoesFileExists_U(uninstallFilePath->Buffer))
    {
        if (!MoveFile(uninstallFilePath->Buffer, backupFilePath->Buffer))
        {
            Context->ErrorCode = GetLastError();
        }
    }

    if (!CopyFile(currentFilePath->Buffer, uninstallFilePath->Buffer, TRUE))
    {
        Context->ErrorCode = GetLastError();
    }

    PhDereferenceObject(uninstallFilePath);
    PhDereferenceObject(currentFilePath);

    return TRUE;
}

VOID SetupDeleteUninstallFile(
    _In_ PPH_SETUP_CONTEXT Context
    )
{
    PPH_STRING uninstallFilePath;

    uninstallFilePath = PhConcatStrings2(PhGetString(Context->SetupInstallPath), L"\\processhacker-setup.exe");

    if (RtlDoesFileExists_U(uninstallFilePath->Buffer))
    {
        PPH_STRING tempFileName;
        PPH_STRING tempFilePath;

        tempFileName = PhCreateString(L"processhacker-setup.exe");
        tempFilePath = PhCreateCacheFile(tempFileName);

        if (PhIsNullOrEmptyString(tempFilePath))
        {
            PhDereferenceObject(tempFileName);
            goto CleanupExit;
        }
    
        MoveFile(uninstallFilePath->Buffer, tempFilePath->Buffer);

        PhDereferenceObject(tempFilePath);
        PhDereferenceObject(tempFileName);
    }

CleanupExit:

    PhDereferenceObject(uninstallFilePath);
}

VOID SetupStartService(
    _In_ PWSTR ServiceName
    )
{
    SC_HANDLE serviceHandle;

    serviceHandle = PhOpenService(
        ServiceName, 
        SERVICE_QUERY_STATUS | SERVICE_START
        );

    if (serviceHandle)
    {
        ULONG statusLength = 0;
        SERVICE_STATUS_PROCESS status;

        memset(&status, 0, sizeof(SERVICE_STATUS_PROCESS));

        if (QueryServiceStatusEx(
            serviceHandle,
            SC_STATUS_PROCESS_INFO,
            (PBYTE)&status,
            sizeof(SERVICE_STATUS_PROCESS),
            &statusLength
            ))
        {
            if (status.dwCurrentState != SERVICE_RUNNING)
            {
                ULONG attempts = 5;

                do
                {
                    StartService(serviceHandle, 0, NULL);

                    if (QueryServiceStatusEx(
                        serviceHandle,
                        SC_STATUS_PROCESS_INFO,
                        (PBYTE)&status,
                        sizeof(SERVICE_STATUS_PROCESS),
                        &statusLength
                        ))
                    {
                        if (status.dwCurrentState == SERVICE_RUNNING)
                        {
                            break;
                        }
                    }

                    Sleep(1000);

                } while (--attempts != 0);
            }
        }

        CloseServiceHandle(serviceHandle);
    }
}

VOID SetupStopService(
    _In_ PWSTR ServiceName
    )
{
    SC_HANDLE serviceHandle;

    serviceHandle = PhOpenService(
        ServiceName,
        SERVICE_QUERY_STATUS | SERVICE_STOP
        );

    if (serviceHandle)
    {
        ULONG statusLength = 0;
        SERVICE_STATUS_PROCESS status;

        memset(&status, 0, sizeof(SERVICE_STATUS_PROCESS));

        if (QueryServiceStatusEx(
            serviceHandle,
            SC_STATUS_PROCESS_INFO,
            (PBYTE)&status,
            sizeof(SERVICE_STATUS_PROCESS),
            &statusLength
            ))
        {
            if (status.dwCurrentState != SERVICE_STOPPED)
            {
                ULONG attempts = 5;

                do
                {
                    SERVICE_STATUS serviceStatus;

                    memset(&serviceStatus, 0, sizeof(SERVICE_STATUS));

                    ControlService(serviceHandle, SERVICE_CONTROL_STOP, &serviceStatus);

                    if (QueryServiceStatusEx(
                        serviceHandle,
                        SC_STATUS_PROCESS_INFO,
                        (PBYTE)&status,
                        sizeof(SERVICE_STATUS_PROCESS),
                        &statusLength
                        ))
                    {
                        if (status.dwCurrentState == SERVICE_STOPPED)
                        {
                            break;
                        }
                    }

                    Sleep(1000);

                } while (--attempts != 0);
            }
        }

        CloseServiceHandle(serviceHandle);
    }
}


VOID SetupStartKph(
    _In_ PPH_SETUP_CONTEXT Context
    )
{
    PPH_STRING clientPath = PhConcatStrings2(PhGetString(Context->SetupInstallPath), L"\\ProcessHacker.exe");

    if (RtlDoesFileExists_U(PhGetString(clientPath)))
    {
        HANDLE processHandle;
        LARGE_INTEGER timeout;
        ULONG retries = 0;

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
            NtClose(processHandle);
        }

        SetupStartService(L"KProcessHacker3");
    }

    PhDereferenceObject(clientPath);
}

BOOLEAN SetupUninstallKph(
    _In_ PPH_SETUP_CONTEXT Context
    )
{
    SetupStopService(L"KProcessHacker2");

    SetupStopService(L"KProcessHacker3");

    return TRUE;
}

VOID SetupSetWindowsOptions(
    _In_ PPH_SETUP_CONTEXT Context
    )
{
    static PH_STRINGREF TaskMgrImageOptionsKeyName = PH_STRINGREF_INIT(L"Software\\Microsoft\\Windows NT\\CurrentVersion\\Image File Execution Options\\taskmgr.exe");
    static PH_STRINGREF CurrentUserRunKeyName = PH_STRINGREF_INIT(L"Software\\Microsoft\\Windows\\CurrentVersion\\Run");
    PPH_STRING clientPathString;
    PPH_STRING startmenuFolderString;

    clientPathString = PhConcatStrings2(PhGetString(Context->SetupInstallPath), L"\\ProcessHacker.exe");

    // Create the startmenu shortcut.
    if (startmenuFolderString = PhGetKnownLocation(CSIDL_COMMON_PROGRAMS, L"\\Process Hacker.lnk"))
    {
        SetupCreateLink(PhGetString(startmenuFolderString), PhGetString(clientPathString), PhGetString(Context->SetupInstallPath));
        PhDereferenceObject(startmenuFolderString);
    }

    // Create the desktop shortcut.
    if (Context->SetupCreateDesktopShortcut)
    {
        if (startmenuFolderString = PhGetKnownLocation(CSIDL_DESKTOPDIRECTORY, L"\\Process Hacker.lnk"))
        {
            SetupCreateLink(PhGetString(startmenuFolderString), PhGetString(clientPathString), PhGetString(Context->SetupInstallPath));
            PhDereferenceObject(startmenuFolderString);
        }
    }

    // Create the all users shortcut.
    if (Context->SetupCreateDesktopShortcutAllUsers)
    {
        if (startmenuFolderString = PhGetKnownLocation(CSIDL_COMMON_DESKTOPDIRECTORY, L"\\Process Hacker.lnk"))
        {
            SetupCreateLink(PhGetString(startmenuFolderString), PhGetString(clientPathString), PhGetString(Context->SetupInstallPath));
            PhDereferenceObject(startmenuFolderString);
        }
    }

    // Create the PE Viewer startmenu shortcut.
    if (startmenuFolderString = PhGetKnownLocation(CSIDL_COMMON_PROGRAMS, L"\\PE Viewer.lnk"))
    {
        PPH_STRING peviewPathString = PhConcatStrings2(PhGetString(Context->SetupInstallPath), L"\\peview.exe");

        SetupCreateLink(
            PhGetString(startmenuFolderString),
            PhGetString(peviewPathString),
            PhGetString(Context->SetupInstallPath)
            );

        PhDereferenceObject(peviewPathString);
        PhDereferenceObject(startmenuFolderString);
    }

    // Reset the settings file.
    if (Context->SetupResetSettings)
    {
        PPH_STRING settingsFileName = PhGetKnownLocation(CSIDL_APPDATA, L"\\Process Hacker\\settings.xml");

        PhDeleteFileWin32(settingsFileName->Buffer);

        PhDereferenceObject(settingsFileName);
    }

    // Set the Windows default Task Manager.
    if (Context->SetupCreateDefaultTaskManager)
    {
        NTSTATUS status;
        HANDLE taskmgrKeyHandle = NULL;

        if (NT_SUCCESS(status = PhCreateKey(
            &taskmgrKeyHandle,
            KEY_READ | KEY_WRITE,
            PH_KEY_LOCAL_MACHINE,
            &TaskMgrImageOptionsKeyName,
            0,
            0,
            NULL
            )))
        {
            PPH_STRING value;
            UNICODE_STRING valueName;

            value = PhConcatStrings(3, L"\"", PhGetString(clientPathString), L"\"");

            // Configure the default Task Manager.
            RtlInitUnicodeString(&valueName, L"Debugger");
            NtSetValueKey(taskmgrKeyHandle, &valueName, 0, REG_SZ, value->Buffer, (ULONG)value->Length + 2);

            NtClose(taskmgrKeyHandle);
        }
        else
        {
            PhShowStatus(NULL, L"Unable to set the Windows default Task Manager.", status, 0);
        }
    }

    // Create the run startup key.
    if (Context->SetupCreateSystemStartup)
    {
        NTSTATUS status;
        HANDLE runKeyHandle = NULL;

        if (NT_SUCCESS(status = PhOpenKey(
            &runKeyHandle,
            KEY_WRITE,
            PH_KEY_CURRENT_USER,
            &CurrentUserRunKeyName,
            0
            )))
        {
            PPH_STRING value;
            UNICODE_STRING valueName;

            RtlInitUnicodeString(&valueName, L"Process Hacker");

            if (Context->SetupCreateMinimizedSystemStartup)
                value = PhConcatStrings(3, L"\"", PhGetString(clientPathString), L"\" -hide");
            else
                value = PhConcatStrings(3, L"\"", PhGetString(clientPathString), L"\"");

            NtSetValueKey(runKeyHandle, &valueName, 0, REG_SZ, value->Buffer, (ULONG)value->Length + 2);
            NtClose(runKeyHandle);
        }
        else
        {
            PhShowStatus(NULL, L"Unable to create the startup entry.", status, 0);
        }
    }
}

VOID SetupDeleteWindowsOptions(
    _In_ PPH_SETUP_CONTEXT Context
    )
{
    static PH_STRINGREF PhImageOptionsKeyName = PH_STRINGREF_INIT(L"Software\\Microsoft\\Windows NT\\CurrentVersion\\Image File Execution Options\\ProcessHacker.exe");
    static PH_STRINGREF TaskMgrImageOptionsKeyName = PH_STRINGREF_INIT(L"Software\\Microsoft\\Windows NT\\CurrentVersion\\Image File Execution Options\\taskmgr.exe");
    static PH_STRINGREF CurrentUserRunKeyName = PH_STRINGREF_INIT(L"Software\\Microsoft\\Windows\\CurrentVersion\\Run");
    PPH_STRING startmenuFolderString;
    HANDLE keyHandle;

    if (startmenuFolderString = PhGetKnownLocation(CSIDL_COMMON_PROGRAMS, L"\\Process Hacker.lnk"))
    {
        PhDeleteFileWin32(PhGetString(startmenuFolderString));
        PhDereferenceObject(startmenuFolderString);
    }

    if (startmenuFolderString = PhGetKnownLocation(CSIDL_COMMON_PROGRAMS, L"\\PE Viewer.lnk"))
    {
        PhDeleteFileWin32(PhGetString(startmenuFolderString));
        PhDereferenceObject(startmenuFolderString);
    }

    if (startmenuFolderString = PhGetKnownLocation(CSIDL_DESKTOPDIRECTORY, L"\\Process Hacker.lnk"))
    {
        PhDeleteFileWin32(PhGetString(startmenuFolderString));
        PhDereferenceObject(startmenuFolderString);
    }

    if (startmenuFolderString = PhGetKnownLocation(CSIDL_COMMON_DESKTOPDIRECTORY, L"\\Process Hacker.lnk"))
    {
        PhDeleteFileWin32(PhGetString(startmenuFolderString));
        PhDereferenceObject(startmenuFolderString);
    }

    if (NT_SUCCESS(PhOpenKey(
        &keyHandle,
        KEY_WRITE | DELETE,
        PH_KEY_LOCAL_MACHINE,
        &PhImageOptionsKeyName,
        0
        )))
    {
        NtDeleteKey(keyHandle);
        NtClose(keyHandle);
    }

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
        KEY_WRITE | DELETE,
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
    _In_ PPH_SETUP_CONTEXT Context
    )
{
    BOOLEAN success = FALSE;
    PPH_STRING clientPath;

    clientPath = PhConcatStrings2(PhGetString(Context->SetupInstallPath), L"\\ProcessHacker.exe");

    if (RtlDoesFileExists_U(clientPath->Buffer))
    {
        success = PhShellExecuteEx(
            Context->DialogHandle,
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

VOID SetupCreateImageFileExecutionOptions(
    VOID
    )
{
    static PH_STRINGREF PhImageOptionsKeyName = PH_STRINGREF_INIT(L"Software\\Microsoft\\Windows NT\\CurrentVersion\\Image File Execution Options\\ProcessHacker.exe");
    HANDLE keyHandle;

    if (WindowsVersion < WINDOWS_10)
        return;

    if (NT_SUCCESS(PhCreateKey(
        &keyHandle,
        KEY_WRITE | DELETE,
        PH_KEY_LOCAL_MACHINE,
        &PhImageOptionsKeyName,
        0,
        0,
        NULL
        )))
    {
        static UNICODE_STRING valueName = RTL_CONSTANT_STRING(L"MitigationOptions");

        NtSetValueKey(keyHandle, &valueName, 0, REG_QWORD, &(ULONG64)
        {
            PROCESS_CREATION_MITIGATION_POLICY_HEAP_TERMINATE_ALWAYS_ON |
            PROCESS_CREATION_MITIGATION_POLICY_BOTTOM_UP_ASLR_ALWAYS_ON |
            PROCESS_CREATION_MITIGATION_POLICY_HIGH_ENTROPY_ASLR_ALWAYS_ON |
            PROCESS_CREATION_MITIGATION_POLICY_EXTENSION_POINT_DISABLE_ALWAYS_ON |
            PROCESS_CREATION_MITIGATION_POLICY_PROHIBIT_DYNAMIC_CODE_ALWAYS_ON |
            PROCESS_CREATION_MITIGATION_POLICY_CONTROL_FLOW_GUARD_ALWAYS_ON |
            PROCESS_CREATION_MITIGATION_POLICY_IMAGE_LOAD_NO_REMOTE_ALWAYS_ON |
            PROCESS_CREATION_MITIGATION_POLICY_IMAGE_LOAD_NO_LOW_LABEL_ALWAYS_ON
        }, sizeof(ULONG64));

        NtClose(keyHandle);
    }
}