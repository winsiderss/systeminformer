/*
 * Copyright (c) 2022 Winsider Seminars & Solutions, Inc.  All rights reserved.
 *
 * This file is part of System Informer.
 *
 * Authors:
 *
 *     dmex
 *
 */

#include "setup.h"

PH_STRINGREF UninstallKeyName = PH_STRINGREF_INIT(L"Software\\Microsoft\\Windows\\CurrentVersion\\Uninstall\\SystemInformer");
//PH_STRINGREF PhImageOptionsKeyName = PH_STRINGREF_INIT(L"Software\\Microsoft\\Windows NT\\CurrentVersion\\Image File Execution Options\\SystemInformer.exe");
//PH_STRINGREF PhImageOptionsWow64KeyName = PH_STRINGREF_INIT(L"Software\\WOW6432Node\\Microsoft\\Windows NT\\CurrentVersion\\Image File Execution Options\\SystemInformer.exe");
PH_STRINGREF TmImageOptionsKeyName = PH_STRINGREF_INIT(L"Software\\Microsoft\\Windows NT\\CurrentVersion\\Image File Execution Options\\taskmgr.exe");

NTSTATUS SetupCreateUninstallKey(
    _In_ PPH_SETUP_CONTEXT Context
    )
{
    NTSTATUS status;
    HANDLE keyHandle;
    PH_STRINGREF name;
    PH_STRINGREF value;
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

        PhInitializeStringRef(&name, L"DisplayIcon");
        tempString = SetupCreateFullPath(Context->SetupInstallPath, L"\\systeminformer.exe,0");
        PhSetValueKey(keyHandle, &name, REG_SZ, tempString->Buffer, (ULONG)tempString->Length + sizeof(UNICODE_NULL));
        PhDereferenceObject(tempString);

        PhInitializeStringRef(&name, L"DisplayName");
        PhInitializeStringRef(&value, L"System Informer");
        PhSetValueKey(keyHandle, &name, REG_SZ, value.Buffer, (ULONG)value.Length + sizeof(UNICODE_NULL));

        PhInitializeStringRef(&name, L"DisplayVersion");
        PhInitializeStringRef(&value, L"3.x");
        PhSetValueKey(keyHandle, &name, REG_SZ, value.Buffer, (ULONG)value.Length + sizeof(UNICODE_NULL));

        PhInitializeStringRef(&name, L"HelpLink");
        PhInitializeStringRef(&value, L"https://systeminformer.sourceforge.io/");
        PhSetValueKey(keyHandle, &name, REG_SZ, value.Buffer, (ULONG)value.Length + sizeof(UNICODE_NULL));

        PhInitializeStringRef(&name, L"InstallLocation");
        PhInitializeStringRef(&value, PhGetString(Context->SetupInstallPath));
        PhSetValueKey(keyHandle, &name, REG_SZ, value.Buffer, (ULONG)value.Length + sizeof(UNICODE_NULL));

        PhInitializeStringRef(&name, L"Publisher");
        PhInitializeStringRef(&value, L"System Informer");
        PhSetValueKey(keyHandle, &name, REG_SZ, value.Buffer, (ULONG)value.Length + sizeof(UNICODE_NULL));

        PhInitializeStringRef(&name, L"UninstallString");
        tempString = PhFormatString(L"\"%s\\systeminformer-setup.exe\" -uninstall", PhGetString(Context->SetupInstallPath));
        PhSetValueKey(keyHandle, &name, REG_SZ, tempString->Buffer, (ULONG)tempString->Length + sizeof(UNICODE_NULL));
        PhDereferenceObject(tempString);

        PhInitializeStringRef(&name, L"NoModify");
        ulong = TRUE;
        PhSetValueKey(keyHandle, &name, REG_DWORD, &ulong, sizeof(ULONG));

        PhInitializeStringRef(&name, L"NoRepair");
        ulong = TRUE;
        PhSetValueKey(keyHandle, &name, REG_DWORD, &ulong, sizeof(ULONG));

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
        DELETE | KEY_WOW64_64KEY,
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
    PPH_STRING setupInstallPath = GetApplicationInstallPath();

    // Check if the string is valid.
    if (PhIsNullOrEmptyString(setupInstallPath))
    {
        static PH_STRINGREF programW6432 = PH_STRINGREF_INIT(L"%ProgramW6432%\\SystemInformer\\");
        static PH_STRINGREF programFiles = PH_STRINGREF_INIT(L"%ProgramFiles%\\SystemInformer\\");
        SYSTEM_INFO info;

        GetNativeSystemInfo(&info);

        if (info.wProcessorArchitecture == PROCESSOR_ARCHITECTURE_AMD64)
            setupInstallPath = PhExpandEnvironmentStrings(&programW6432);
        else
            setupInstallPath = PhExpandEnvironmentStrings(&programFiles);
    }

    if (PhIsNullOrEmptyString(setupInstallPath))
    {
        setupInstallPath = PhCreateString(L"C:\\Program Files\\SystemInformer\\");
    }

    if (!PhIsNullOrEmptyString(setupInstallPath))
    {
        if (!PhEndsWithStringRef(&setupInstallPath->sr, &PhNtPathSeperatorString, TRUE))
        {
            PhSwapReference(&setupInstallPath, PhConcatStringRef2(&setupInstallPath->sr, &PhNtPathSeperatorString));
        }
    }

    return setupInstallPath;
}

PPH_STRING SetupFindAppdataDirectory(
    VOID
    )
{
    static PH_STRINGREF appdataPath = PH_STRINGREF_INIT(L"%APPDATA%\\SystemInformer\\");
    PPH_STRING appdataFilePath;

    if (appdataFilePath = PhExpandEnvironmentStrings(&appdataPath))
    {
        PhMoveReference(&appdataFilePath, PhGetFullPath(appdataFilePath->Buffer, NULL));
        return appdataFilePath;
    }

    return NULL;
}

VOID SetupDeleteAppdataDirectory(
    _In_ PPH_SETUP_CONTEXT Context
    )
{
    PPH_STRING uninstallFilePath;

    if (uninstallFilePath = SetupFindAppdataDirectory())
    {
        PhDeleteDirectoryWin32(uninstallFilePath);
        PhDereferenceObject(uninstallFilePath);
    }
}

BOOLEAN SetupCreateUninstallFile(
    _In_ PPH_SETUP_CONTEXT Context
    )
{
    NTSTATUS status;
    PPH_STRING currentFilePath;
    PPH_STRING backupFilePath;
    PPH_STRING uninstallFilePath;

    if (!NT_SUCCESS(status = PhGetProcessImageFileNameWin32(NtCurrentProcess(), &currentFilePath)))
    {
        Context->ErrorCode = PhNtStatusToDosError(status);
        return FALSE;
    }

    // Check if the user has started the setup from the installation folder.
    if (PhStartsWithStringRef2(&currentFilePath->sr, PhGetString(Context->SetupInstallPath), TRUE))
    {
        PhDereferenceObject(currentFilePath);
        return TRUE;
    }

    backupFilePath = PhConcatStrings2(PhGetString(Context->SetupInstallPath), L"\\systeminformer-setup.bak");
    uninstallFilePath = PhConcatStrings2(PhGetString(Context->SetupInstallPath), L"\\systeminformer-setup.exe");

    if (PhDoesFileExistWin32(PhGetString(backupFilePath)))
    {
        PPH_STRING tempFileName;
        PPH_STRING tempFilePath;

        tempFileName = PhCreateString(L"systeminformer-setup.bak");
        tempFilePath = PhCreateCacheFile(tempFileName);

        //if (!NT_SUCCESS(PhDeleteFileWin32(backupFilePath->Buffer)))
        if (!NT_SUCCESS(status = PhMoveFileWin32(PhGetString(backupFilePath), PhGetString(tempFilePath), FALSE)))
        {
            Context->ErrorCode = PhNtStatusToDosError(status);
            return FALSE;
        }

        PhDereferenceObject(tempFilePath);
        PhDereferenceObject(tempFileName);
    }

    if (PhDoesFileExistWin32(PhGetString(uninstallFilePath)))
    {
        PPH_STRING tempFileName;
        PPH_STRING tempFilePath;

        //if (!NT_SUCCESS(PhDeleteFileWin32(PhGetString(uninstallFilePath))))
        tempFileName = PhCreateString(L"systeminformer-setup.exe");
        tempFilePath = PhCreateCacheFile(tempFileName);

        if (!NT_SUCCESS(status = PhMoveFileWin32(PhGetString(uninstallFilePath), PhGetString(tempFilePath), FALSE)))
        {
            Context->ErrorCode = PhNtStatusToDosError(status);
            return FALSE;
        }

        PhDereferenceObject(tempFilePath);
        PhDereferenceObject(tempFileName);
    }

    if (!NT_SUCCESS(status = PhCopyFileWin32(PhGetString(currentFilePath), PhGetString(uninstallFilePath), TRUE)))
    {
        Context->ErrorCode = PhNtStatusToDosError(status);
        return FALSE;
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

    uninstallFilePath = PhConcatStrings2(PhGetString(Context->SetupInstallPath), L"\\systeminformer-setup.exe");

    if (PhDoesFileExistWin32(PhGetString(uninstallFilePath)))
    {
        PPH_STRING tempFileName;
        PPH_STRING tempFilePath;

        tempFileName = PhCreateString(L"systeminformer-setup.exe");
        tempFilePath = PhCreateCacheFile(tempFileName);

        if (PhIsNullOrEmptyString(tempFilePath))
        {
            PhDereferenceObject(tempFileName);
            goto CleanupExit;
        }

        PhMoveFileWin32(PhGetString(uninstallFilePath), PhGetString(tempFilePath), FALSE);

        PhDereferenceObject(tempFilePath);
        PhDereferenceObject(tempFileName);
    }

CleanupExit:

    PhDereferenceObject(uninstallFilePath);
}

BOOLEAN SetupUninstallDriver(
    _In_ PPH_SETUP_CONTEXT Context
    )
{
    SC_HANDLE serviceHandle;

    if (serviceHandle = PhOpenService(
        PhIsNullOrEmptyString(Context->SetupServiceName) ? L"KSystemInformer" : PhGetString(Context->SetupServiceName),
        SERVICE_QUERY_STATUS | SERVICE_STOP
        ))
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
                ULONG attempts = 60;

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

                    PhDelayExecution(1000);

                } while (--attempts != 0);
            }
        }

        CloseServiceHandle(serviceHandle);
    }

    if (serviceHandle = PhOpenService(
        PhIsNullOrEmptyString(Context->SetupServiceName) ? L"KSystemInformer" : PhGetString(Context->SetupServiceName),
        DELETE
        ))
    {
        DeleteService(serviceHandle);
        CloseServiceHandle(serviceHandle);
    }

    return TRUE;
}

VOID SetupSetWindowsOptions(
    _In_ PPH_SETUP_CONTEXT Context
    )
{
    static PH_STRINGREF desktopStartmenuPathSr = PH_STRINGREF_INIT(L"%ALLUSERSPROFILE%\\Microsoft\\Windows\\Start Menu\\Programs\\System Informer.lnk");
    static PH_STRINGREF peviewerShortcutPathSr = PH_STRINGREF_INIT(L"%ALLUSERSPROFILE%\\Microsoft\\Windows\\Start Menu\\Programs\\PE Viewer.lnk");
    static PH_STRINGREF desktopAllusersPathSr = PH_STRINGREF_INIT(L"%PUBLIC%\\Desktop\\System Informer.lnk");
    PPH_STRING clientPathString;
    PPH_STRING string;

    if (string = PhExpandEnvironmentStrings(&desktopStartmenuPathSr))
    {
        clientPathString = SetupCreateFullPath(Context->SetupInstallPath, L"\\SystemInformer.exe");

        SetupCreateLink(
            PhGetString(string),
            PhGetString(clientPathString),
            PhGetString(Context->SetupInstallPath),
            L"SystemInformer"
            );

        PhDereferenceObject(clientPathString);
        PhDereferenceObject(string);
    }

    if (string = PhExpandEnvironmentStrings(&desktopAllusersPathSr))
    {
        clientPathString = SetupCreateFullPath(Context->SetupInstallPath, L"\\SystemInformer.exe");

        SetupCreateLink(
            PhGetString(string),
            PhGetString(clientPathString),
            PhGetString(Context->SetupInstallPath),
            L"SystemInformer"
            );

        PhDereferenceObject(clientPathString);
        PhDereferenceObject(string);
    }

    if (string = PhExpandEnvironmentStrings(&peviewerShortcutPathSr))
    {
        clientPathString = SetupCreateFullPath(Context->SetupInstallPath, L"\\peview.exe");

        SetupCreateLink(
            PhGetString(string),
            PhGetString(clientPathString),
            PhGetString(Context->SetupInstallPath),
            L"SystemInformer_PEViewer"
            );

        PhDereferenceObject(clientPathString);
        PhDereferenceObject(string);
    }

    // PhGetKnownLocation(CSIDL_COMMON_PROGRAMS, L"\\System Informer.lnk"))
    // PhGetKnownLocation(CSIDL_COMMON_DESKTOPDIRECTORY, L"\\System Informer.lnk")
    // PhGetKnownLocation(CSIDL_COMMON_PROGRAMS, L"\\PE Viewer.lnk")

    // Reset the settings file.
    //if (Context->SetupResetSettings)
    //{
    //    static PH_STRINGREF settingsPath = PH_STRINGREF_INIT(L"%APPDATA%\\SystemInformer\\settings.xml");
    //    PPH_STRING settingsFilePath;
    //
    //    if (settingsFilePath = PhExpandEnvironmentStrings(&settingsPath))
    //    {
    //        if (PhDoesFileExistWin32(settingsFilePath->Buffer))
    //        {
    //            PhDeleteFileWin32(settingsFilePath->Buffer);
    //        }
    //
    //        PhDereferenceObject(settingsFilePath);
    //    }
    //}

    // Set the Windows default Task Manager.
    //if (Context->SetupCreateDefaultTaskManager)
    //{
    //    NTSTATUS status;
    //    HANDLE taskmgrKeyHandle;
    //
    //    status = PhCreateKey(
    //        &taskmgrKeyHandle,
    //        KEY_READ | KEY_WRITE,
    //        PH_KEY_LOCAL_MACHINE,
    //        &TmImageOptionsKeyName,
    //        OBJ_OPENIF,
    //        0,
    //        NULL
    //        );
    //
    //    if (NT_SUCCESS(status))
    //    {
    //        PPH_STRING value;
    //        UNICODE_STRING valueName;
    //
    //        RtlInitUnicodeString(&valueName, L"Debugger");
    //
    //        value = PhConcatStrings(3, L"\"", PhGetString(clientPathString), L"\"");
    //        status = NtSetValueKey(
    //            taskmgrKeyHandle,
    //            &valueName,
    //            0,
    //            REG_SZ,
    //            value->Buffer,
    //            (ULONG)value->Length + sizeof(UNICODE_NULL)
    //            );
    //
    //        PhDereferenceObject(value);
    //        NtClose(taskmgrKeyHandle);
    //    }
    //
    //    if (!NT_SUCCESS(status))
    //    {
    //        PhShowStatus(NULL, L"Unable to set the Windows default Task Manager.", status, 0);
    //    }
    //}

    // Create the run startup key.
    //if (Context->SetupCreateSystemStartup)
    //{
    //    NTSTATUS status;
    //    HANDLE runKeyHandle;
    //
    //    if (NT_SUCCESS(status = PhOpenKey(
    //        &runKeyHandle,
    //        KEY_WRITE,
    //        PH_KEY_CURRENT_USER,
    //        &CurrentUserRunKeyName,
    //        0
    //        )))
    //    {
    //        PPH_STRING value;
    //        UNICODE_STRING valueName;
    //
    //        RtlInitUnicodeString(&valueName, L"System Informer");
    //
    //        if (Context->SetupCreateMinimizedSystemStartup)
    //            value = PhConcatStrings(3, L"\"", PhGetString(clientPathString), L"\" -hide");
    //        else
    //            value = PhConcatStrings(3, L"\"", PhGetString(clientPathString), L"\"");
    //
    //        NtSetValueKey(runKeyHandle, &valueName, 0, REG_SZ, value->Buffer, (ULONG)value->Length + 2);
    //        NtClose(runKeyHandle);
    //    }
    //    else
    //    {
    //        PhShowStatus(NULL, L"Unable to create the startup entry.", status, 0);
    //    }
    //}
}

VOID SetupDeleteWindowsOptions(
    _In_ PPH_SETUP_CONTEXT Context
    )
{
    static PH_STRINGREF desktopShortcutPathSr = PH_STRINGREF_INIT(L"%ALLUSERSPROFILE%\\Microsoft\\Windows\\Start Menu\\Programs\\System Informer.lnk");
    static PH_STRINGREF peviewerShortcutPathSr = PH_STRINGREF_INIT(L"%ALLUSERSPROFILE%\\Microsoft\\Windows\\Start Menu\\Programs\\PE Viewer.lnk");
    static PH_STRINGREF desktopAllusersPathSr = PH_STRINGREF_INIT(L"%PUBLIC%\\Desktop\\System Informer.lnk");
    PPH_STRING string;
    HANDLE keyHandle;

    if (string = PhExpandEnvironmentStrings(&desktopShortcutPathSr))
    {
        PhDeleteFileWin32(string->Buffer);
        PhDereferenceObject(string);
    }

    if (string = PhExpandEnvironmentStrings(&peviewerShortcutPathSr))
    {
        PhDeleteFileWin32(string->Buffer);
        PhDereferenceObject(string);
    }

    if (string = PhExpandEnvironmentStrings(&desktopAllusersPathSr))
    {
        PhDeleteFileWin32(string->Buffer);
        PhDereferenceObject(string);
    }

    //if (NT_SUCCESS(PhOpenKey(
    //    &keyHandle,
    //    DELETE,
    //    PH_KEY_LOCAL_MACHINE,
    //    &PhImageOptionsKeyName,
    //    0
    //    )))
    //{
    //    NtDeleteKey(keyHandle);
    //    NtClose(keyHandle);
    //}
    //
    //if (NT_SUCCESS(PhOpenKey(
    //    &keyHandle,
    //    DELETE,
    //    PH_KEY_LOCAL_MACHINE,
    //    &PhImageOptionsWow64KeyName,
    //    0
    //    )))
    //{
    //    NtDeleteKey(keyHandle);
    //    NtClose(keyHandle);
    //}

    if (NT_SUCCESS(PhOpenKey(
        &keyHandle,
        DELETE,
        PH_KEY_LOCAL_MACHINE,
        &TmImageOptionsKeyName,
        0
        )))
    {
        NtDeleteKey(keyHandle);
        NtClose(keyHandle);
    }
}

VOID SetupChangeNotifyShortcuts(
    _In_ PPH_SETUP_CONTEXT Context
    )
{
    static PH_STRINGREF desktopStartmenuPathSr = PH_STRINGREF_INIT(L"%ALLUSERSPROFILE%\\Microsoft\\Windows\\Start Menu\\Programs\\System Informer.lnk");
    static PH_STRINGREF peviewerShortcutPathSr = PH_STRINGREF_INIT(L"%ALLUSERSPROFILE%\\Microsoft\\Windows\\Start Menu\\Programs\\PE Viewer.lnk");
    static PH_STRINGREF desktopAllusersPathSr = PH_STRINGREF_INIT(L"%PUBLIC%\\Desktop\\System Informer.lnk");
    PPH_STRING string;
    PIDLIST_ABSOLUTE pidlist;

    if (string = PhExpandEnvironmentStrings(&desktopStartmenuPathSr))
    {
        if (PhDoesFileExistWin32(PhGetString(string)))
        {
            SHChangeNotify(SHCNE_UPDATEITEM, SHCNF_PATH, PhGetString(string), NULL);
        }

        PhDereferenceObject(string);
    }

    if (string = PhExpandEnvironmentStrings(&desktopAllusersPathSr))
    {
        if (PhDoesFileExistWin32(PhGetString(string)))
        {
            SHChangeNotify(SHCNE_UPDATEITEM, SHCNF_PATH, PhGetString(string), NULL);
        }

        PhDereferenceObject(string);
    }

    if (string = PhExpandEnvironmentStrings(&peviewerShortcutPathSr))
    {
        if (PhDoesFileExistWin32(PhGetString(string)))
        {
            SHChangeNotify(SHCNE_UPDATEITEM, SHCNF_PATH, PhGetString(string), NULL);
        }

        PhDereferenceObject(string);
    }

    if (SUCCEEDED(SHGetSpecialFolderLocation(NULL, CSIDL_STARTMENU, &pidlist)))
    {
        SHChangeNotify(SHCNE_UPDATEDIR, SHCNF_IDLIST, pidlist, NULL);
    }
}

BOOLEAN SetupExecuteApplication(
    _In_ PPH_SETUP_CONTEXT Context
    )
{
    BOOLEAN success = FALSE;
    PPH_STRING clientPath;

    if (PhIsNullOrEmptyString(Context->SetupInstallPath))
        return FALSE;

    clientPath = SetupCreateFullPath(Context->SetupInstallPath, L"\\SystemInformer.exe");

    if (PhDoesFileExistWin32(clientPath->Buffer))
    {
        success = PhShellExecuteEx(
            Context->DialogHandle,
            clientPath->Buffer,
            NULL,
            SW_SHOWNORMAL,
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
    static PH_STRINGREF settingsPath = PH_STRINGREF_INIT(L"%APPDATA%\\SystemInformer\\settings.xml");
    static PH_STRINGREF settingsLegacyPath = PH_STRINGREF_INIT(L"%APPDATA%\\Process Hacker 2\\settings.xml");
    static PH_STRINGREF settingsLegacyNightlyPath = PH_STRINGREF_INIT(L"%APPDATA%\\Process Hacker\\settings.xml");
    BOOLEAN migratedNightly = FALSE;
    PPH_STRING settingsFilePath;
    PPH_STRING oldSettingsFileName;
    PPH_STRING oldSettingsNightlyFileName;

    settingsFilePath = PhExpandEnvironmentStrings(&settingsPath);
    oldSettingsFileName = PhExpandEnvironmentStrings(&settingsLegacyPath);
    oldSettingsNightlyFileName = PhExpandEnvironmentStrings(&settingsLegacyNightlyPath);

    if (settingsFilePath && oldSettingsNightlyFileName)
    {
        if (!PhDoesFileExistWin32(PhGetString(settingsFilePath)))
        {
            if (NT_SUCCESS(PhCopyFileWin32(PhGetString(oldSettingsNightlyFileName), PhGetString(settingsFilePath), TRUE)))
            {
                migratedNightly = TRUE;
            }
        }
    }

    if (!migratedNightly && settingsFilePath && oldSettingsFileName)
    {
        if (!PhDoesFileExistWin32(PhGetString(settingsFilePath)))
        {
            PhCopyFileWin32(PhGetString(oldSettingsFileName), PhGetString(settingsFilePath), TRUE);
        }
    }

    if (oldSettingsNightlyFileName) PhDereferenceObject(oldSettingsNightlyFileName);
    if (oldSettingsFileName) PhDereferenceObject(oldSettingsFileName);
    if (settingsFilePath) PhDereferenceObject(settingsFilePath);
}

//VOID SetupCreateImageFileExecutionOptions(
//    VOID
//    )
//{
//    HANDLE keyHandle;
//    SYSTEM_INFO info;
//
//    if (WindowsVersion < WINDOWS_10)
//        return;
//
//    GetNativeSystemInfo(&info);
//
//    if (NT_SUCCESS(PhCreateKey(
//        &keyHandle,
//        KEY_WRITE,
//        PH_KEY_LOCAL_MACHINE,
//        &PhImageOptionsKeyName,
//        OBJ_OPENIF,
//        0,
//        NULL
//        )))
//    {
//        static PH_STRINGREF valueName = PH_STRINGREF_INIT(L"MitigationOptions");
//
//        PhSetValueKey(keyHandle, &valueName, REG_QWORD, &(ULONG64)
//        {
//            PROCESS_CREATION_MITIGATION_POLICY_HEAP_TERMINATE_ALWAYS_ON |
//            PROCESS_CREATION_MITIGATION_POLICY_BOTTOM_UP_ASLR_ALWAYS_ON |
//            PROCESS_CREATION_MITIGATION_POLICY_HIGH_ENTROPY_ASLR_ALWAYS_ON |
//            PROCESS_CREATION_MITIGATION_POLICY_EXTENSION_POINT_DISABLE_ALWAYS_ON |
//            PROCESS_CREATION_MITIGATION_POLICY_PROHIBIT_DYNAMIC_CODE_ALWAYS_ON |
//            PROCESS_CREATION_MITIGATION_POLICY_CONTROL_FLOW_GUARD_ALWAYS_ON |
//            PROCESS_CREATION_MITIGATION_POLICY_IMAGE_LOAD_NO_REMOTE_ALWAYS_ON |
//            PROCESS_CREATION_MITIGATION_POLICY_IMAGE_LOAD_NO_LOW_LABEL_ALWAYS_ON
//        }, sizeof(ULONG64));
//
//        NtClose(keyHandle);
//    }
//
//    if (info.wProcessorArchitecture == PROCESSOR_ARCHITECTURE_AMD64)
//    {
//        if (NT_SUCCESS(PhCreateKey(
//            &keyHandle,
//            KEY_WRITE,
//            PH_KEY_LOCAL_MACHINE,
//            &PhImageOptionsWow64KeyName,
//            OBJ_OPENIF,
//            0,
//            NULL
//            )))
//        {
//            static PH_STRINGREF valueName = PH_STRINGREF_INIT(L"MitigationOptions");
//
//            PhSetValueKey(keyHandle, &valueName, REG_QWORD, &(ULONG64)
//            {
//                PROCESS_CREATION_MITIGATION_POLICY_HEAP_TERMINATE_ALWAYS_ON |
//                PROCESS_CREATION_MITIGATION_POLICY_BOTTOM_UP_ASLR_ALWAYS_ON |
//                PROCESS_CREATION_MITIGATION_POLICY_HIGH_ENTROPY_ASLR_ALWAYS_ON |
//                PROCESS_CREATION_MITIGATION_POLICY_EXTENSION_POINT_DISABLE_ALWAYS_ON |
//                PROCESS_CREATION_MITIGATION_POLICY_PROHIBIT_DYNAMIC_CODE_ALWAYS_ON |
//                PROCESS_CREATION_MITIGATION_POLICY_CONTROL_FLOW_GUARD_ALWAYS_ON |
//                PROCESS_CREATION_MITIGATION_POLICY_IMAGE_LOAD_NO_REMOTE_ALWAYS_ON |
//                PROCESS_CREATION_MITIGATION_POLICY_IMAGE_LOAD_NO_LOW_LABEL_ALWAYS_ON
//            }, sizeof(ULONG64));
//
//            NtClose(keyHandle);
//        }
//    }
//}

VOID ExtractResourceToFile(
    _In_ PVOID DllBase,
    _In_ PWSTR Name,
    _In_ PWSTR FileName
    )
{
    HANDLE fileHandle = NULL;
    PVOID resourceBuffer;
    ULONG resourceLength;
    LARGE_INTEGER allocationSize;
    IO_STATUS_BLOCK isb;

    if (!PhLoadResource(
        DllBase,
        Name,
        RT_RCDATA,
        &resourceLength,
        &resourceBuffer
        ))
    {
        goto CleanupExit;
    }

    allocationSize.QuadPart = resourceLength;

    if (!NT_SUCCESS(PhCreateFileWin32Ex(
        &fileHandle,
        FileName,
        FILE_GENERIC_READ | FILE_GENERIC_WRITE,
        &allocationSize,
        FILE_ATTRIBUTE_NORMAL,
        FILE_SHARE_READ | FILE_SHARE_WRITE | FILE_SHARE_DELETE,
        FILE_OVERWRITE_IF,
        FILE_NON_DIRECTORY_FILE | FILE_SYNCHRONOUS_IO_NONALERT,
        NULL
        )))
    {
        goto CleanupExit;
    }

    if (!NT_SUCCESS(NtWriteFile(
        fileHandle,
        NULL,
        NULL,
        NULL,
        &isb,
        resourceBuffer,
        resourceLength,
        NULL,
        NULL
        )))
    {
        goto CleanupExit;
    }

    if (isb.Information != resourceLength)
        goto CleanupExit;

CleanupExit:

    if (fileHandle)
        NtClose(fileHandle);
}

BOOLEAN ConnectionAvailable(VOID)
{
    INetworkListManager* networkListManager = NULL;

    if (SUCCEEDED(CoCreateInstance(
        &CLSID_NetworkListManager,
        NULL,
        CLSCTX_ALL,
        &IID_INetworkListManager,
        &networkListManager
        )))
    {
        VARIANT_BOOL isConnected = VARIANT_FALSE;
        VARIANT_BOOL isConnectedInternet = VARIANT_FALSE;

        // Query the relevant properties.
        INetworkListManager_get_IsConnected(networkListManager, &isConnected);
        INetworkListManager_get_IsConnectedToInternet(networkListManager, &isConnectedInternet);

        // Cleanup the INetworkListManger COM objects.
        INetworkListManager_Release(networkListManager);

        // Check if Windows is connected to a network and it's connected to the internet.
        if (isConnected == VARIANT_TRUE && isConnectedInternet == VARIANT_TRUE)
            return TRUE;

        // We're not connected to anything
    }

    return FALSE;
}

VOID SetupCreateLink(
    _In_ PWSTR LinkFilePath,
    _In_ PWSTR FilePath,
    _In_ PWSTR FileParentDir,
    _In_ PWSTR AppId
    )
{
    IShellLink* shellLinkPtr = NULL;
    IPersistFile* persistFilePtr = NULL;
    IPropertyStore* propertyStorePtr;

    if (FAILED(CoCreateInstance(&CLSID_ShellLink, NULL, CLSCTX_INPROC_SERVER, &IID_IShellLinkW, &shellLinkPtr)))
        goto CleanupExit;

    if (FAILED(IShellLinkW_QueryInterface(shellLinkPtr, &IID_IPersistFile, &persistFilePtr)))
        goto CleanupExit;

    if (SUCCEEDED(IShellLinkW_QueryInterface(shellLinkPtr, &IID_IPropertyStore, &propertyStorePtr)))
    {
        SIZE_T propValueLength;
        PROPVARIANT propValue;

        propValueLength = PhCountStringZ(AppId) * sizeof(WCHAR);

        PropVariantInit(&propValue);
        V_VT(&propValue) = VT_LPWSTR;
        V_UNION(&propValue, pwszVal) = CoTaskMemAlloc(propValueLength + sizeof(UNICODE_NULL));
    
        if (V_UNION(&propValue, pwszVal))
        {
            memset(V_UNION(&propValue, pwszVal), 0, propValueLength + sizeof(UNICODE_NULL));
            memcpy(V_UNION(&propValue, pwszVal), AppId, propValueLength);
    
            IPropertyStore_SetValue(propertyStorePtr, &PKEY_AppUserModel_ID, &propValue);
        }

        IPropertyStore_Commit(propertyStorePtr);
        IPropertyStore_Release(propertyStorePtr);

        PropVariantClear(&propValue);
    }

    // Load existing shell item if it exists...
    //IPersistFile_Load(persistFilePtr, LinkFilePath, STGM_READWRITE);

    //IShellLinkW_SetDescription(shellLinkPtr, FileComment);
    //IShellLinkW_SetHotkey(shellLinkPtr, MAKEWORD(VK_END, HOTKEYF_CONTROL | HOTKEYF_ALT));
    IShellLinkW_SetWorkingDirectory(shellLinkPtr, FileParentDir);
    IShellLinkW_SetIconLocation(shellLinkPtr, FilePath, 0);

    // Set the shortcut target path...
    if (FAILED(IShellLinkW_SetPath(shellLinkPtr, FilePath)))
        goto CleanupExit;

    if (PhDoesFileExistWin32(LinkFilePath))
        PhDeleteFileWin32(LinkFilePath);

    // Save the shortcut to the file system...
    IPersistFile_Save(persistFilePtr, LinkFilePath, TRUE);

CleanupExit:
    if (persistFilePtr)
        IPersistFile_Release(persistFilePtr);
    if (shellLinkPtr)
        IShellLinkW_Release(shellLinkPtr);

    SHChangeNotify(SHCNE_UPDATEITEM, SHCNF_PATH, LinkFilePath, NULL);
}

BOOLEAN CheckApplicationInstalled(
    VOID
    )
{
    BOOLEAN installed = FALSE;
    PPH_STRING installPath;
    PPH_STRING exePath;

    installPath = GetApplicationInstallPath();

    if (!PhIsNullOrEmptyString(installPath))
    {
        exePath = SetupCreateFullPath(installPath, L"\\SystemInformer.exe");

        // Check if the value has a valid file path.
        installed = PhDoesFileExistWin32(PhGetString(exePath));

        PhDereferenceObject(exePath);
    }

    if (installPath)
        PhDereferenceObject(installPath);

    return installed;
}

BOOLEAN CheckApplicationInstallPathLegacy(
    _In_ PPH_STRING Directory
    )
{
    BOOLEAN installed = FALSE;
    PPH_STRING exePath;

    // Check the directory for the legacy 'ProcessHacker.exe' executable.
    exePath = SetupCreateFullPath(Directory, L"\\ProcessHacker.exe");
    installed = PhDoesFileExistWin32(PhGetString(exePath));
    PhDereferenceObject(exePath);

    return installed;
}

PPH_STRING GetApplicationInstallPath(
    VOID
    )
{
    HANDLE keyHandle;
    PPH_STRING installPath = NULL;

    if (NT_SUCCESS(PhOpenKey(
        &keyHandle,
        KEY_READ | KEY_WOW64_64KEY,
        PH_KEY_LOCAL_MACHINE,
        &UninstallKeyName,
        0
        )))
    {
        installPath = PhQueryRegistryString(keyHandle, L"InstallLocation");
        NtClose(keyHandle);
    }

    return installPath;
}

static BOOLEAN NTAPI PhpPreviousInstancesCallback(
    _In_ PPH_STRINGREF Name,
    _In_ PPH_STRINGREF TypeName,
    _In_opt_ PVOID Context
    )
{
    HANDLE objectHandle;
    UNICODE_STRING objectNameUs;
    OBJECT_ATTRIBUTES objectAttributes;
    MUTANT_OWNER_INFORMATION objectInfo;

    if (!PhStartsWithStringRef2(Name, L"SiMutant_", TRUE) &&
        !PhStartsWithStringRef2(Name, L"SiSetupMutant_", TRUE) &&
        !PhStartsWithStringRef2(Name, L"SiViewerMutant_", TRUE))
    {
        return TRUE;
    }

    if (!PhStringRefToUnicodeString(Name, &objectNameUs))
        return TRUE;

    InitializeObjectAttributes(
        &objectAttributes,
        &objectNameUs,
        OBJ_CASE_INSENSITIVE,
        PhGetNamespaceHandle(),
        NULL
        );

    if (!NT_SUCCESS(NtOpenMutant(
        &objectHandle,
        MUTANT_QUERY_STATE,
        &objectAttributes
        )))
    {
        return TRUE;
    }

    if (NT_SUCCESS(PhGetMutantOwnerInformation(
        objectHandle,
        &objectInfo
        )))
    {
        HWND hwnd;
        HANDLE processHandle = NULL;

        if (objectInfo.ClientId.UniqueProcess == NtCurrentProcessId())
            goto CleanupExit;

        PhOpenProcess(
            &processHandle,
            PROCESS_QUERY_LIMITED_INFORMATION | PROCESS_TERMINATE,
            objectInfo.ClientId.UniqueProcess
            );

        hwnd = PhGetProcessMainWindowEx(
            objectInfo.ClientId.UniqueProcess,
            processHandle,
            FALSE
            );

        if (hwnd)
        {
            SendMessageTimeout(hwnd, WM_QUIT, 0, 0, SMTO_BLOCK, 5000, NULL);
        }

        if (processHandle)
        {
            NtTerminateProcess(processHandle, 1);
        }

    CleanupExit:
        if (processHandle) NtClose(processHandle);
    }

    NtClose(objectHandle);

    return TRUE;
}

BOOLEAN ShutdownApplication(
    VOID
    )
{
    PhEnumDirectoryObjects(PhGetNamespaceHandle(), PhpPreviousInstancesCallback, NULL);
    return TRUE;
}

NTSTATUS QueryProcessesUsingVolumeOrFile(
    _In_ HANDLE VolumeOrFileHandle,
    _Out_ PFILE_PROCESS_IDS_USING_FILE_INFORMATION *Information
    )
{
    static ULONG initialBufferSize = 0x4000;
    NTSTATUS status;
    PVOID buffer;
    ULONG bufferSize;
    IO_STATUS_BLOCK isb;

    bufferSize = initialBufferSize;
    buffer = PhAllocate(bufferSize);

    while ((status = NtQueryInformationFile(
        VolumeOrFileHandle,
        &isb,
        buffer,
        bufferSize,
        FileProcessIdsUsingFileInformation
        )) == STATUS_INFO_LENGTH_MISMATCH)
    {
        PhFree(buffer);
        bufferSize *= 2;

        // Fail if we're resizing the buffer to something very large.
        if (bufferSize > SIZE_MAX)
            return STATUS_INSUFFICIENT_RESOURCES;

        buffer = PhAllocate(bufferSize);
    }

    if (!NT_SUCCESS(status))
    {
        PhFree(buffer);
        return status;
    }

    if (bufferSize <= 0x100000) initialBufferSize = bufferSize;
    *Information = (PFILE_PROCESS_IDS_USING_FILE_INFORMATION)buffer;

    return status;
}

PPH_STRING SetupCreateFullPath(
    _In_ PPH_STRING Path,
    _In_ PWSTR FileName
    )
{
    PPH_STRING pathString;
    PPH_STRING tempString;

    pathString = PhConcatStringRefZ(&Path->sr, FileName);

    if (NT_SUCCESS(PhGetFullPathEx(pathString->Buffer, NULL, &tempString)))
    {
        PhMoveReference(&pathString, tempString);
        return pathString;
    }

    return pathString;
}

_Success_(return)
BOOLEAN SetupBase64StringToBufferEx(
    _In_ PVOID InputBuffer,
    _In_ ULONG InputBufferLength,
    _Out_opt_ PVOID* OutputBuffer,
    _Out_opt_ ULONG* OutputBufferLength
    )
{
    PVOID buffer = NULL;
    ULONG bufferLength = 0;
    ULONG bufferSkip = 0;
    ULONG bufferFlags = 0;

    if (CryptStringToBinary(InputBuffer, InputBufferLength, CRYPT_STRING_BASE64, NULL, &bufferLength, &bufferSkip, &bufferFlags))
    {
        if (buffer = PhAllocateSafe(bufferLength))
        {
            if (!CryptStringToBinary(InputBuffer, InputBufferLength, CRYPT_STRING_BASE64, buffer, &bufferLength, &bufferSkip, &bufferFlags))
            {
                PhFree(buffer);
                buffer = NULL;
            }
        }
    }

    if (buffer)
    {
        if (OutputBuffer)
            *OutputBuffer = buffer;
        else
            PhFree(buffer);

        if (OutputBufferLength)
            *OutputBufferLength = bufferLength;

        return TRUE;
    }

    return FALSE;
}
