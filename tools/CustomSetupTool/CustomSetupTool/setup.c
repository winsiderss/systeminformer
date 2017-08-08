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
                setupInstallPath = PhConcatStringRef2(&expandedString->sr, &defaultDirectoryName);
            }
        }
        else
        {
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
    PH_STRINGREF currentFilePath;
    PPH_STRING backupFilePath;
    PPH_STRING uninstallFilePath;

    PhUnicodeStringToStringRef(&NtCurrentPeb()->ProcessParameters->ImagePathName, &currentFilePath);

    // Check if the user has started the setup from the installation folder.
    if (PhStartsWithStringRef2(&currentFilePath, PhGetString(Context->SetupInstallPath), TRUE))
        return TRUE;

    backupFilePath = PhConcatStrings2(PhGetString(Context->SetupInstallPath), L"\\processhacker-setup.bak");
    uninstallFilePath = PhConcatStrings2(PhGetString(Context->SetupInstallPath), L"\\processhacker-setup.exe");

    if (RtlDoesFileExists_U(backupFilePath->Buffer))
    {
        GUID randomGuid;
        PPH_STRING tempPath = NULL;
        PPH_STRING guidString = NULL;

        tempPath = PhCreateStringEx(NULL, GetTempPath(0, NULL) * sizeof(WCHAR));
        GetTempPath((ULONG)tempPath->Length / sizeof(WCHAR), tempPath->Buffer);

        PhGenerateGuid(&randomGuid);

        if (guidString = PhFormatGuid(&randomGuid))
        {
            PhMoveReference(&guidString, PhSubstring(guidString, 1, guidString->Length / sizeof(WCHAR) - 2));
        }

        PhMoveReference(&guidString, PhConcatStrings(
            3,
            PhGetString(tempPath),
            L"\\",
            PhGetString(guidString)
            ));

        if (!MoveFile(backupFilePath->Buffer, guidString->Buffer))
        {
            Context->ErrorCode = GetLastError();
        }
    }

    if (RtlDoesFileExists_U(uninstallFilePath->Buffer))
    {
        if (!MoveFile(uninstallFilePath->Buffer, backupFilePath->Buffer))
        {
            Context->ErrorCode = GetLastError();
        }
    }

    if (!CopyFile(currentFilePath.Buffer, uninstallFilePath->Buffer, TRUE))
    {
        Context->ErrorCode = GetLastError();
    }

    PhDereferenceObject(uninstallFilePath);
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
        ULONG indexOfFileName = -1;
        GUID randomGuid;
        PPH_STRING setupTempPath = NULL;
        PPH_STRING randomGuidString = NULL;
        PPH_STRING fullSetupPath = NULL;
        PPH_STRING tempFilePath = NULL;

        setupTempPath = PhCreateStringEx(NULL, GetTempPath(0, NULL) * sizeof(WCHAR));
        if (PhIsNullOrEmptyString(setupTempPath))
            goto CleanupExit;
        if (GetTempPath((ULONG)setupTempPath->Length / sizeof(WCHAR), setupTempPath->Buffer) == 0)
            goto CleanupExit;
        if (PhIsNullOrEmptyString(setupTempPath))
            goto CleanupExit;

        // Generate random guid for our directory path.
        PhGenerateGuid(&randomGuid);

        if (randomGuidString = PhFormatGuid(&randomGuid))
        {
            PPH_STRING guidSubString;

            // Strip the left and right curly brackets.
            guidSubString = PhSubstring(randomGuidString, 1, randomGuidString->Length / sizeof(WCHAR) - 2);
            PhMoveReference(&randomGuidString, guidSubString);
        }

        // Append the tempath to our string: %TEMP%RandomString\\processhacker-setup.exe
        // Example: C:\\Users\\dmex\\AppData\\Temp\\processhacker-setup.exe
        tempFilePath = PhFormatString(
            L"%s%s\\processhacker-setup.exe",
            PhGetStringOrEmpty(setupTempPath),
            PhGetStringOrEmpty(randomGuidString)
            );
        if (PhIsNullOrEmptyString(tempFilePath))
            goto CleanupExit;

        // Create the directory if it does not exist.
        if (fullSetupPath = PhGetFullPath(PhGetString(tempFilePath), &indexOfFileName))
        {
            PPH_STRING directoryPath;

            if (indexOfFileName == -1)
                goto CleanupExit;

            if (directoryPath = PhSubstring(fullSetupPath, 0, indexOfFileName))
            {
                SHCreateDirectoryEx(NULL, directoryPath->Buffer, NULL);
                PhDereferenceObject(directoryPath);
            }
        }

        MoveFile(uninstallFilePath->Buffer, fullSetupPath->Buffer);
    }

CleanupExit:

    PhDereferenceObject(uninstallFilePath);
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
    }

    PhDereferenceObject(clientPath);
}

BOOLEAN SetupUninstallKph(
    _In_ PPH_SETUP_CONTEXT Context
    )
{
    while (TRUE)
    {
        SC_HANDLE serviceHandle;
        SERVICE_STATUS serviceStatus;

        if (!(serviceHandle = PhOpenService(
            L"KProcessHacker2",
            SERVICE_STOP | DELETE
            )))
            break;

        ControlService(serviceHandle, SERVICE_CONTROL_STOP, &serviceStatus);
        DeleteService(serviceHandle);

        CloseServiceHandle(serviceHandle);
    }

    while (TRUE)
    {
        SC_HANDLE serviceHandle;
        SERVICE_STATUS serviceStatus;

        if (!(serviceHandle = PhOpenService(
            L"KProcessHacker3",
            SERVICE_STOP | DELETE
            )))
            break;

        ControlService(serviceHandle, SERVICE_CONTROL_STOP, &serviceStatus);
        DeleteService(serviceHandle);

        CloseServiceHandle(serviceHandle);
    }

    return TRUE;
}

VOID SetupSetWindowsOptions(
    _In_ PPH_SETUP_CONTEXT Context
    )
{
    static PH_STRINGREF TaskMgrImageOptionsKeyName = PH_STRINGREF_INIT(L"Software\\Microsoft\\Windows NT\\CurrentVersion\\Image File Execution Options\\taskmgr.exe");
    static PH_STRINGREF CurrentUserRunKeyName = PH_STRINGREF_INIT(L"Software\\Microsoft\\Windows\\CurrentVersion\\Run");
    HANDLE keyHandle;
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

        SetupDeleteDirectoryFile(settingsFileName->Buffer);

        PhDereferenceObject(settingsFileName);
    }

    // Set the Windows default Task Manager.
    if (Context->SetupCreateDefaultTaskManager)
    {
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

            // Configure the default Task Manager.
            RtlInitUnicodeString(&valueName, L"Debugger");
            NtSetValueKey(keyHandle, &valueName, 0, REG_SZ, value->Buffer, (ULONG)value->Length + 2);

            NtClose(keyHandle);
        }
    }

    // Create the run startup key.
    if (Context->SetupCreateSystemStartup)
    {
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

            if (Context->SetupCreateMinimizedSystemStartup)
                value = PhConcatStrings(3, L"\"", PhGetString(clientPathString), L"\" -hide");
            else
                value = PhConcatStrings(3, L"\"", PhGetString(clientPathString), L"\"");

            NtSetValueKey(keyHandle, &valueName, 0, REG_SZ, value->Buffer, (ULONG)value->Length + 2);
            NtClose(keyHandle);
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
        SetupDeleteDirectoryFile(PhGetString(startmenuFolderString));
        PhDereferenceObject(startmenuFolderString);
    }

    if (startmenuFolderString = PhGetKnownLocation(CSIDL_COMMON_PROGRAMS, L"\\PE Viewer.lnk"))
    {
        SetupDeleteDirectoryFile(PhGetString(startmenuFolderString));
        PhDereferenceObject(startmenuFolderString);
    }

    if (startmenuFolderString = PhGetKnownLocation(CSIDL_DESKTOPDIRECTORY, L"\\Process Hacker.lnk"))
    {
        SetupDeleteDirectoryFile(PhGetString(startmenuFolderString));
        PhDereferenceObject(startmenuFolderString);
    }

    if (startmenuFolderString = PhGetKnownLocation(CSIDL_COMMON_DESKTOPDIRECTORY, L"\\Process Hacker.lnk"))
    {
        SetupDeleteDirectoryFile(PhGetString(startmenuFolderString));
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

    // Set the default Image File Execution Options.
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
                PROCESS_CREATION_MITIGATION_POLICY_FONT_DISABLE_ALWAYS_ON |
                PROCESS_CREATION_MITIGATION_POLICY_IMAGE_LOAD_NO_REMOTE_ALWAYS_ON |
                PROCESS_CREATION_MITIGATION_POLICY_IMAGE_LOAD_NO_LOW_LABEL_ALWAYS_ON
        }, sizeof(ULONG64));

        NtClose(keyHandle);
    }
}