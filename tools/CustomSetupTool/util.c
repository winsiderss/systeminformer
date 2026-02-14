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

#if defined(_DEBUG) && !defined(PH_RELEASE_CHANNEL_ID)
#define PH_RELEASE_CHANNEL_ID 2
#endif

#ifdef PH_RELEASE_CHANNEL_ID
#if PH_RELEASE_CHANNEL_ID == 0
#define SETUP_APP_PARAMETERS L"-channel release"
#elif PH_RELEASE_CHANNEL_ID == 1
#define SETUP_APP_PARAMETERS L"-channel preview"
#elif PH_RELEASE_CHANNEL_ID == 2
#define SETUP_APP_PARAMETERS L"-channel canary"
#elif PH_RELEASE_CHANNEL_ID == 3
#define SETUP_APP_PARAMETERS L"-channel developer"
#endif
#endif

#ifndef SETUP_APP_PARAMETERS
#error PH_RELEASE_CHANNEL_ID undefined
#endif

CONST PH_STRINGREF UninstallKeyNames[] =
{
    PH_STRINGREF_INIT(L"Software\\Microsoft\\Windows\\CurrentVersion\\Uninstall\\SystemInformer"),
    PH_STRINGREF_INIT(L"Software\\Microsoft\\Windows\\CurrentVersion\\Uninstall\\SystemInformer-Preview"),
    PH_STRINGREF_INIT(L"Software\\Microsoft\\Windows\\CurrentVersion\\Uninstall\\SystemInformer-Canary"),
    PH_STRINGREF_INIT(L"Software\\Microsoft\\Windows\\CurrentVersion\\Uninstall\\SystemInformer-Developer"),
};
CONST PH_STRINGREF AppPathsKeyName = PH_STRINGREF_INIT(L"Software\\Microsoft\\Windows\\CurrentVersion\\App Paths\\SystemInformer.exe");
CONST PH_STRINGREF TaskmgrIfeoKeyName = PH_STRINGREF_INIT(L"Software\\Microsoft\\Windows NT\\CurrentVersion\\Image File Execution Options\\taskmgr.exe");
CONST PH_STRINGREF SystemInformerIfeoKeyName = PH_STRINGREF_INIT(L"Software\\Microsoft\\Windows NT\\CurrentVersion\\Image File Execution Options\\SystemInformer.exe");
CONST PH_STRINGREF AppCompatFlagsLayersKeyName = PH_STRINGREF_INIT(L"Software\\Microsoft\\Windows NT\\CurrentVersion\\AppCompatFlags\\Layers");
CONST PH_STRINGREF CurrentUserRunKeyName = PH_STRINGREF_INIT(L"Software\\Microsoft\\Windows\\CurrentVersion\\Run");
CONST PH_STRINGREF LocalDumpsKeyName = PH_STRINGREF_INIT(L"Software\\Microsoft\\Windows\\Windows Error Reporting\\LocalDumps\\SystemInformer.exe");

/**
 * Deletes the uninstall keys for System Informer.
 */
VOID SetupDeleteUninstallKey(
    VOID
    )
{
    for (ULONG i = 0; i < RTL_NUMBER_OF(UninstallKeyNames); i++)
    {
        HANDLE keyHandle;

        if (NT_SUCCESS(PhOpenKey(
            &keyHandle,
            DELETE | KEY_WOW64_64KEY,
            PH_KEY_LOCAL_MACHINE,
            &UninstallKeyNames[i],
            0
            )))
        {
            NtDeleteKey(keyHandle);
            NtClose(keyHandle);
        }
    }
}

/**
 * Creates the uninstall key for System Informer.
 *
 * \param Context The setup context.
 * \return Successful or errant status.
 */
NTSTATUS SetupCreateUninstallKey(
    _In_ PPH_SETUP_CONTEXT Context
    )
{
    NTSTATUS status;
    HANDLE keyHandle;
    PH_STRINGREF value;

    SetupDeleteUninstallKey();

    status = PhCreateKey(
        &keyHandle,
        KEY_ALL_ACCESS | KEY_WOW64_64KEY,
        PH_KEY_LOCAL_MACHINE,
        &UninstallKeyNames[PH_RELEASE_CHANNEL_ID],
        OBJ_OPENIF,
        0,
        NULL
        );

    if (NT_SUCCESS(status))
    {
        PPH_STRING string;
        ULONG regValue;
        PH_FORMAT format[7];

        string = SetupCreateFullPath(Context->SetupInstallPath, L"\\systeminformer.exe,0");
        PhSetValueKeyZ(keyHandle, L"DisplayIcon", REG_SZ, string->Buffer, (ULONG)string->Length + sizeof(UNICODE_NULL));

        PhInitializeStringRef(&value, L"System Informer");
        PhSetValueKeyZ(keyHandle, L"DisplayName", REG_SZ, value.Buffer, (ULONG)value.Length + sizeof(UNICODE_NULL));

        PhInitFormatU(&format[0], PHAPP_VERSION_MAJOR);
        PhInitFormatC(&format[1], L'.');
        PhInitFormatU(&format[2], PHAPP_VERSION_MINOR);
        PhInitFormatC(&format[3], L'.');
        PhInitFormatU(&format[4], PHAPP_VERSION_BUILD);
        PhInitFormatC(&format[5], L'.');
        PhInitFormatU(&format[6], PHAPP_VERSION_REVISION);
        string = PhFormat(format, RTL_NUMBER_OF(format), 10);
        PhSetValueKeyZ(keyHandle, L"DisplayVersion", REG_SZ, string->Buffer, (ULONG)string->Length + sizeof(UNICODE_NULL));

        PhInitializeStringRef(&value, L"https://system-informer.com/"); // package manager (winget) consistency (jxy-s)
        PhSetValueKeyZ(keyHandle, L"HelpLink", REG_SZ, value.Buffer, (ULONG)value.Length + sizeof(UNICODE_NULL));

        string = SetupCreateFullPath(Context->SetupInstallPath, L"");
        PhSetValueKeyZ(keyHandle, L"InstallLocation", REG_SZ, string->Buffer, (ULONG)string->Length + sizeof(UNICODE_NULL));

        PhInitializeStringRef(&value, L"Winsider Seminars & Solutions, Inc.");
        PhSetValueKeyZ(keyHandle, L"Publisher", REG_SZ, value.Buffer, (ULONG)value.Length + sizeof(UNICODE_NULL));

        string = SetupCreateFullPath(Context->SetupInstallPath, L"\\systeminformer-setup.exe");
        PhMoveReference(&string, PhFormatString(L"\"%s\" -uninstall", PhGetString(string)));
        PhSetValueKeyZ(keyHandle, L"UninstallString", REG_SZ, string->Buffer, (ULONG)string->Length + sizeof(UNICODE_NULL));

        regValue = TRUE;
        PhSetValueKeyZ(keyHandle, L"NoModify", REG_DWORD, &regValue, sizeof(ULONG));
        PhSetValueKeyZ(keyHandle, L"NoRepair", REG_DWORD, &regValue, sizeof(ULONG));

        NtClose(keyHandle);
    }

    return status;
}

/**
 * Finds the installation directory for System Informer.
 *
 * \return A string containing the installation directory.
 */
PPH_STRING SetupFindInstallDirectory(
    VOID
    )
{
    PPH_STRING setupInstallPath = GetApplicationInstallPath();

    if (PhIsNullOrEmptyString(setupInstallPath))
    {
        static CONST PH_STRINGREF programW6432 = PH_STRINGREF_INIT(L"%ProgramW6432%\\SystemInformer\\");
        static CONST PH_STRINGREF programFiles = PH_STRINGREF_INIT(L"%ProgramFiles%\\SystemInformer\\");
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
        if (!PhEndsWithStringRef(&setupInstallPath->sr, &PhNtPathSeparatorString, TRUE))
        {
            PhSwapReference(&setupInstallPath, PhConcatStringRef2(&setupInstallPath->sr, &PhNtPathSeparatorString));
        }
    }

    return setupInstallPath;
}

/**
 * Deletes the application data directory for System Informer.
 *
 * \param Context The setup context.
 */
VOID SetupDeleteAppdataDirectory(
    _In_ PPH_SETUP_CONTEXT Context
    )
{
    PPH_STRING appdataDirectory;

    if (appdataDirectory = PhGetKnownFolderPathZ(&FOLDERID_RoamingAppData, L"\\SystemInformer\\"))
    {
        PhDeleteDirectoryWin32(&appdataDirectory->sr);

        PhDereferenceObject(appdataDirectory);
    }
}

VOID SetupDeleteSystemInformerIfeo(
    VOID
    )
{
    HANDLE keyHandle;

    if (NT_SUCCESS(PhOpenKey(
        &keyHandle,
        DELETE,
        PH_KEY_LOCAL_MACHINE,
        &SystemInformerIfeoKeyName,
        0
        )))
    {
        NtDeleteKey(keyHandle);
        NtClose(keyHandle);
    }
}

// Callback for enumerating and deleting AppCompatFlags Layers entries
_Function_class_(PH_ENUM_KEY_CALLBACK)
BOOLEAN NTAPI SetupDeleteAppCompatFlagsLayersCallback(
    _In_ HANDLE RootDirectory,
    _In_ PKEY_VALUE_FULL_INFORMATION Information,
    _In_opt_ PVOID Context
    )
{
    if (Information->Type == REG_SZ)
    {
        static CONST PH_STRINGREF fileNameSuffix = PH_STRINGREF_INIT(L"SystemInformer.exe");
        PH_STRINGREF valueName;

        valueName.Length = Information->NameLength;
        valueName.Buffer = Information->Name;

        if (PhEndsWithStringRef(&valueName, &fileNameSuffix, TRUE))
        {
            PH_STRINGREF value;

            value.Length = Information->NameLength;
            value.Buffer = Information->Name;

            PhDeleteValueKey(RootDirectory, &value);
        }
    }
    return TRUE;
}

// Function to delete AppCompatFlags Layers entries
VOID SetupDeleteAppCompatFlagsLayersEntry(
    VOID
    )
{
    HANDLE keyHandle;

    if (NT_SUCCESS(PhOpenKey(
        &keyHandle,
        KEY_READ | KEY_WRITE | DELETE,
        PH_KEY_CURRENT_USER,
        &AppCompatFlagsLayersKeyName,
        0
        )))
    {
        PhEnumerateValueKey(keyHandle, KeyValueFullInformation, SetupDeleteAppCompatFlagsLayersCallback, NULL);
        NtClose(keyHandle);
    }

    if (NT_SUCCESS(PhOpenKey(
        &keyHandle,
        KEY_READ | KEY_WRITE | DELETE,
        PH_KEY_LOCAL_MACHINE,
        &AppCompatFlagsLayersKeyName,
        0
        )))
    {
        PhEnumerateValueKey(keyHandle, KeyValueFullInformation, SetupDeleteAppCompatFlagsLayersCallback, NULL);
        NtClose(keyHandle);
    }
}

/**
 * Creates the uninstall file (copies setup.exe) to the install directory.
 *
 * \param Context The setup context.
 * \return Successful or errant status.
 */
NTSTATUS SetupCreateUninstallFile(
    _In_ PPH_SETUP_CONTEXT Context
    )
{
    NTSTATUS status;
    PPH_STRING currentFilePath;
    PPH_STRING uninstallFilePath;

    if (!NT_SUCCESS(status = PhGetProcessImageFileNameWin32(NtCurrentProcess(), &currentFilePath)))
        return status;

    // Check if the current image is running from the installation folder.

    if (PhStartsWithStringRef2(
        &currentFilePath->sr,
        PhGetString(Context->SetupInstallPath),
        TRUE
        ))
    {
        return STATUS_SUCCESS;
    }

    // Move the outdated setup.exe into the trash (temp) folder.

    uninstallFilePath = SetupCreateFullPath(Context->SetupInstallPath, L"\\systeminformer-setup.exe");

    if (PhIsNullOrEmptyString(uninstallFilePath))
        return STATUS_NO_MEMORY;

    if (PhDoesFileExistWin32(PhGetString(uninstallFilePath)))
    {
        PPH_STRING tempFileName = PhGetTemporaryDirectoryRandomAlphaFileName();

        if (!NT_SUCCESS(status = PhMoveFileWin32(
            PhGetString(uninstallFilePath),
            PhGetString(tempFileName),
            FALSE
            )))
        {
            PhDereferenceObject(tempFileName);
            return status;
        }

        PhDereferenceObject(tempFileName);
    }

    // Copy the latest setup.exe to the installation folder, so that:
    // 1. The setup program doesn't disappear for user.
    // 2. The correct ACL is applied to the file. Moving the file would retain the existing ACL.

    if (!NT_SUCCESS(status = PhCopyFileWin32(
        PhGetString(currentFilePath),
        PhGetString(uninstallFilePath),
        FALSE
        )))
    {
        return status;
    }

    return STATUS_SUCCESS;
}

/**
 * Deletes the uninstall file (setup.exe) from the install directory.
 *
 * \param Context The setup context.
 */
VOID SetupDeleteUninstallFile(
    _In_ PPH_SETUP_CONTEXT Context
    )
{
    PPH_STRING uninstallFilePath;

    uninstallFilePath = SetupCreateFullPath(Context->SetupInstallPath, L"\\systeminformer-setup.exe");

    if (PhDoesFileExistWin32(PhGetString(uninstallFilePath)))
    {
        PPH_STRING tempFileName = PhGetTemporaryDirectoryRandomAlphaFileName();

        PhMoveFileWin32(
            PhGetString(uninstallFilePath),
            PhGetString(tempFileName),
            FALSE
            );

        PhDereferenceObject(tempFileName);
    }

    PhDereferenceObject(uninstallFilePath);
}

/**
 * Uninstalls the System Informer driver.
 *
 * \param Context The setup context.
 * \return Successful or errant status.
 */
NTSTATUS SetupUninstallDriver(
    _In_ PPH_SETUP_CONTEXT Context
    )
{
    NTSTATUS status = STATUS_SUCCESS;
    SC_HANDLE serviceHandle;

    if (NT_SUCCESS(PhOpenService(
        &serviceHandle,
        SERVICE_QUERY_STATUS | SERVICE_STOP,
        PhIsNullOrEmptyString(Context->SetupServiceName) ? L"KSystemInformer" : PhGetString(Context->SetupServiceName)
        )))
    {
        SERVICE_STATUS_PROCESS serviceStatus;

        if (NT_SUCCESS(PhQueryServiceStatus(serviceHandle, &serviceStatus)))
        {
            if (serviceStatus.dwCurrentState != SERVICE_STOPPED)
            {
                PhStopService(serviceHandle);

                PhWaitForServiceStatus(serviceHandle, SERVICE_STOPPED, 30 * 1000);
            }
        }

        PhCloseServiceHandle(serviceHandle);
    }

    if (NT_SUCCESS(PhOpenService(
        &serviceHandle,
        DELETE,
        PhIsNullOrEmptyString(Context->SetupServiceName) ? L"KSystemInformer" : PhGetString(Context->SetupServiceName)
        )))
    {
        status = PhDeleteService(serviceHandle);

        PhCloseServiceHandle(serviceHandle);
    }

    return status;
}

/**
 * Creates Windows options (e.g., App Paths, startup entries).
 *
 * \param Context The setup context.
 */
VOID SetupCreateWindowsOptions(
    _In_ PPH_SETUP_CONTEXT Context
    )
{
    // Create the app paths keys
    {
        NTSTATUS status;
        HANDLE keyHandle;
        PPH_STRING string;

        status = PhCreateKey(
            &keyHandle,
            KEY_ALL_ACCESS | KEY_WOW64_64KEY,
            PH_KEY_LOCAL_MACHINE,
            &AppPathsKeyName,
            OBJ_OPENIF,
            0,
            NULL
            );

        if (NT_SUCCESS(status))
        {
            //string = PhFormatString(L"\"%s\\SystemInformer.exe\"", PhGetString(Context->SetupInstallPath));

            if (string = SetupCreateFullPath(Context->SetupInstallPath, L"\\SystemInformer.exe"))
            {
                PhSetValueKey(keyHandle, NULL, REG_SZ, string->Buffer, (ULONG)string->Length + sizeof(UNICODE_NULL));
                PhDereferenceObject(string);
            }

            NtClose(keyHandle);
        }
    }

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

/**
 * Callback for enumerating and deleting auto-run keys.
 *
 * \param RootDirectory The root directory handle.
 * \param Information The key value full information.
 * \param Context Optional context.
 * \return TRUE to continue enumeration, FALSE to stop.
 */
_Function_class_(PH_ENUM_KEY_CALLBACK)
BOOLEAN NTAPI SetupDeleteAutoRunKeyCallback(
    _In_ HANDLE RootDirectory,
    _In_ PKEY_VALUE_FULL_INFORMATION Information,
    _In_opt_ PVOID Context
    )
{
    if (Information->Type == REG_SZ)
    {
        static CONST PH_STRINGREF fileName = PH_STRINGREF_INIT(L"\\SystemInformer.exe");
        PH_STRINGREF value;

        value.Length = Information->DataLength;
        value.Buffer = PTR_ADD_OFFSET(Information, Information->DataOffset);

        if (PhFindStringInStringRef(&value, &fileName, TRUE) != SIZE_MAX)
        {
            PH_STRINGREF name;

            name.Length = Information->NameLength;
            name.Buffer = Information->Name;

            PhDeleteValueKey(RootDirectory, &name);
        }
    }

    return TRUE;
}

/**
 * Deletes Windows options (e.g., App Paths, startup entries).
 *
 * \param Context The setup context.
 */
VOID SetupDeleteWindowsOptions(
    _In_ PPH_SETUP_CONTEXT Context
    )
{
    HANDLE keyHandle;

    if (NT_SUCCESS(PhOpenKey(
        &keyHandle,
        DELETE,
        PH_KEY_LOCAL_MACHINE,
        &AppPathsKeyName,
        0
        )))
    {
        NtDeleteKey(keyHandle);
        NtClose(keyHandle);
    }

    SetupDeleteSystemInformerIfeo();

    if (NT_SUCCESS(PhOpenKey(
        &keyHandle,
        DELETE,
        PH_KEY_LOCAL_MACHINE,
        &TaskmgrIfeoKeyName,
        0
        )))
    {
        NtDeleteKey(keyHandle);
        NtClose(keyHandle);
    }

    SetupDeleteAppCompatFlagsLayersEntry();

    if (NT_SUCCESS(PhOpenKey(
        &keyHandle,
        KEY_READ | KEY_WRITE,
        PH_KEY_CURRENT_USER,
        &CurrentUserRunKeyName,
        0
        )))
    {
        PhEnumerateValueKey(keyHandle, KeyValueFullInformation, SetupDeleteAutoRunKeyCallback, NULL);
        NtClose(keyHandle);
    }
}

/**
 * Creates the Windows Error Reporting LocalDumps registry key for SystemInformer.exe.
 *
 * \return Successful or errant status.
 */
NTSTATUS SetupCreateLocalDumpsKey(
    VOID
    )
{
    NTSTATUS status;
    HANDLE keyHandle;
    PH_STRINGREF dumpFolderValue;
    ULONG dumpCountValue;
    ULONG dumpTypeValue;

    status = PhCreateKey(
        &keyHandle,
        KEY_ALL_ACCESS | KEY_WOW64_64KEY,
        PH_KEY_LOCAL_MACHINE,
        &LocalDumpsKeyName,
        OBJ_OPENIF,
        0,
        NULL
        );

    if (NT_SUCCESS(status))
    {
        // Set DumpCount (REG_DWORD) = 10
        dumpCountValue = 10;
        PhSetValueKeyZ(keyHandle, L"DumpCount", REG_DWORD, &dumpCountValue, sizeof(ULONG));

        // Set DumpFolder (REG_SZ) = %APPDATA%\SystemInformer\CrashDumps
        PhInitializeStringRef(&dumpFolderValue, L"%APPDATA%\\SystemInformer\\CrashDumps");
        PhSetValueKeyZ(keyHandle, L"DumpFolder", REG_SZ, dumpFolderValue.Buffer, (ULONG)dumpFolderValue.Length + sizeof(UNICODE_NULL));

        // Set DumpType (REG_DWORD) = 1 (mini dump)
        dumpTypeValue = 1;
        PhSetValueKeyZ(keyHandle, L"DumpType", REG_DWORD, &dumpTypeValue, sizeof(ULONG));

        NtClose(keyHandle);
    }

    return status;
}

/**
 * Deletes the Windows Error Reporting LocalDumps registry key for SystemInformer.exe.
 */
VOID SetupDeleteLocalDumpsKey(
    VOID
    )
{
    HANDLE keyHandle;

    if (NT_SUCCESS(PhOpenKey(
        &keyHandle,
        DELETE | KEY_WOW64_64KEY,
        PH_KEY_LOCAL_MACHINE,
        &LocalDumpsKeyName,
        0
        )))
    {
        NtDeleteKey(keyHandle);
        NtClose(keyHandle);
    }
}

/**
 * Creates desktop and start menu shortcuts.
 *
 * \param Context The setup context.
 */
VOID SetupCreateShortcuts(
    _In_ PPH_SETUP_CONTEXT Context
    )
{
    PPH_STRING string;
    PPH_STRING clientPathString;
    HRESULT status;

    if (string = PhGetKnownFolderPathZ(&FOLDERID_ProgramData, L"\\Microsoft\\Windows\\Start Menu\\Programs\\System Informer.lnk"))
    {
        if (clientPathString = SetupCreateFullPath(Context->SetupInstallPath, L"\\SystemInformer.exe"))
        {
            status = SetupCreateLink(
                PhGetString(string),
                PhGetString(clientPathString),
                PhGetString(Context->SetupInstallPath),
                L"SystemInformer"
                );

            PhDereferenceObject(clientPathString);
        }

        PhDereferenceObject(string);
    }

    if (string = PhGetKnownFolderPathZ(&FOLDERID_ProgramData, L"\\Microsoft\\Windows\\Start Menu\\Programs\\PE Viewer.lnk"))
    {
        if (clientPathString = SetupCreateFullPath(Context->SetupInstallPath, L"\\peview.exe"))
        {
            status = SetupCreateLink(
                PhGetString(string),
                PhGetString(clientPathString),
                PhGetString(Context->SetupInstallPath),
                L"SystemInformer_PEViewer"
                );

            PhDereferenceObject(clientPathString);
        }

        PhDereferenceObject(string);
    }

    if (string = PhGetKnownFolderPathZ(&FOLDERID_PublicDesktop, L"\\System Informer.lnk"))
    {
        if (clientPathString = SetupCreateFullPath(Context->SetupInstallPath, L"\\SystemInformer.exe"))
        {
            status = SetupCreateLink(
                PhGetString(string),
                PhGetString(clientPathString),
                PhGetString(Context->SetupInstallPath),
                L"SystemInformer"
                );

            PhDereferenceObject(clientPathString);
        }

        PhDereferenceObject(string);
    }

    // PhGetKnownLocation(CSIDL_COMMON_PROGRAMS, L"\\System Informer.lnk"))
    // PhGetKnownLocation(CSIDL_COMMON_DESKTOPDIRECTORY, L"\\System Informer.lnk")
    // PhGetKnownLocation(CSIDL_COMMON_PROGRAMS, L"\\PE Viewer.lnk")
}

/**
 * Deletes desktop and start menu shortcuts.
 *
 * \param Context The setup context.
 */
VOID SetupDeleteShortcuts(
    _In_ PPH_SETUP_CONTEXT Context
    )
{
    PPH_STRING string;

    if (string = PhGetKnownFolderPathZ(&FOLDERID_ProgramData, L"\\Microsoft\\Windows\\Start Menu\\Programs\\System Informer.lnk"))
    {
        PhDeleteFileWin32(string->Buffer);
        PhDereferenceObject(string);
    }

    if (string = PhGetKnownFolderPathZ(&FOLDERID_ProgramData, L"\\Microsoft\\Windows\\Start Menu\\Programs\\PE Viewer.lnk"))
    {
        PhDeleteFileWin32(string->Buffer);
        PhDereferenceObject(string);
    }

    if (string = PhGetKnownFolderPathZ(&FOLDERID_PublicDesktop, L"\\System Informer.lnk"))
    {
        PhDeleteFileWin32(string->Buffer);
        PhDereferenceObject(string);
    }
}

/**
 * Executes the System Informer application.
 *
 * \param Context The setup context.
 * \return Successful or errant status.
 */
NTSTATUS SetupExecuteApplication(
    _In_ PPH_SETUP_CONTEXT Context
    )
{
    NTSTATUS status;
    PPH_STRING string;
    PPH_STRING parameters;

    if (PhIsNullOrEmptyString(Context->SetupInstallPath))
        return STATUS_NO_MEMORY;

    string = SetupCreateFullPath(Context->SetupInstallPath, L"\\SystemInformer.exe");
    parameters = PhCreateString(SETUP_APP_PARAMETERS);

    if (Context->Hide)
    {
        PhMoveReference(&parameters, PhConcatStringRefZ(&parameters->sr, L" -hide"));
    }

    if (PhDoesFileExistWin32(PhGetString(string)))
    {
        status = PhShellExecuteEx(
            Context->DialogHandle,
            PhGetString(string),
            PhGetString(parameters),
            NULL,
            SW_SHOW,
            PH_SHELL_EXECUTE_DEFAULT,
            0,
            NULL
            );
    }
    else
    {
        status = STATUS_OBJECT_PATH_NOT_FOUND;
    }

    PhDereferenceObject(parameters);
    PhDereferenceObject(string);
    return status;
}

/**
 * Upgrades the settings file from a legacy format (PH) to the new format (System Informer).
 *
 * \return Successful or errant status.
 */
NTSTATUS SetupUpgradeSettingsFile(
    VOID
    )
{
    NTSTATUS status = STATUS_SUCCESS;
    BOOLEAN migratedNightly = FALSE;
    PPH_STRING settingsFilePath = NULL;
    PPH_STRING legacyNightlyFileName = NULL;
    PPH_STRING legacySettingsFileName = NULL;

    // Current SystemInformer settings.xml path
    settingsFilePath = PhGetKnownFolderPathZ(&FOLDERID_RoamingAppData, L"\\SystemInformer\\settings.xml");
    if (PhIsNullOrEmptyString(settingsFilePath)) {
        status = STATUS_UNSUCCESSFUL;
        goto CleanupExit;
    }

    // If a SystemInformer settings.xml already exists, we assume it's up-to-date
    // and don't need to copy any legacy XML.
    if (PhDoesFileExistWin32(PhGetString(settingsFilePath))) {
        status = STATUS_SUCCESS; // Already upgraded or current, nothing to do.
        goto CleanupExit;
    }

    // Construct legacy nightly build settings.xml path
    legacyNightlyFileName = PhGetKnownFolderPathZ(&FOLDERID_RoamingAppData, L"\\Process Hacker\\settings.xml");

    // Construct legacy stable build settings.xml path
    legacySettingsFileName = PhGetKnownFolderPathZ(&FOLDERID_RoamingAppData, L"\\Process Hacker 2\\settings.xml");

    // Attempt to migrate nightly build settings if they exist
    if (legacyNightlyFileName && PhDoesFileExistWin32(PhGetString(legacyNightlyFileName)))
    {
        if (NT_SUCCESS(PhCopyFileWin32(PhGetString(legacyNightlyFileName), PhGetString(settingsFilePath), TRUE)))
        {
            migratedNightly = TRUE;
            // Successfully migrated from nightly, no need to check stable
            status = STATUS_SUCCESS;
            goto CleanupExit;
        }
        else
        {
            // Log copy failure, but proceed to try stable settings
        }
    }

    // Attempt to migrate stable build settings if nightly didn't migrate
    if (!migratedNightly && legacySettingsFileName && PhDoesFileExistWin32(PhGetString(legacySettingsFileName)))
    {
        if (NT_SUCCESS(PhCopyFileWin32(PhGetString(legacySettingsFileName), PhGetString(settingsFilePath), TRUE)))
        {
            status = STATUS_SUCCESS;
            goto CleanupExit;
        }
        else
        {
            // Log copy failure
        }
    }
    
    // If we reached here, either no legacy files were found, or copying failed.
    // If no legacy files existed, it's not an error from an upgrade perspective.
    // If copying failed, the status would already be set to an error.

CleanupExit:
    if (legacySettingsFileName) PhDereferenceObject(legacySettingsFileName);
    if (legacyNightlyFileName) PhDereferenceObject(legacyNightlyFileName);
    if (settingsFilePath) PhDereferenceObject(settingsFilePath);
    return status;
}

/**
 * Converts the settings file from XML to JSON format.
 *
 * \return Successful or errant status.
 */
NTSTATUS SetupConvertSettingsFile(
    VOID
    )
{
    NTSTATUS status = STATUS_UNSUCCESSFUL;
    PPH_STRING settingsFilePath = NULL;
    PPH_STRING convertFilePath = NULL;

    //
    // Get the path to the current settings.xml file
    //
    settingsFilePath = PhGetKnownFolderPathZ(&FOLDERID_RoamingAppData, L"\\SystemInformer\\settings.xml");

    if (PhIsNullOrEmptyString(settingsFilePath))
        goto CleanupExit;

    //
    // Check if the settings.xml file actually exists before attempting conversion
    //
    if (!PhDoesFileExistWin32(PhGetString(settingsFilePath)))
    {
        // No settings.xml file to convert, so consider it a success.
        status = STATUS_SUCCESS;
        goto CleanupExit;
    }

    //
    // Get the path for the new settings.json file
    //
    convertFilePath = PhGetKnownFolderPathZ(&FOLDERID_RoamingAppData, L"\\SystemInformer\\settings.json");

    if (PhIsNullOrEmptyString(convertFilePath))
        goto CleanupExit;

    //
    // Check if the settings.json file actually exists before attempting conversion
    //
    if (PhDoesFileExistWin32(PhGetString(convertFilePath)))
    {
        // The settings.json already exists, conversion is not needed.
        status = STATUS_SUCCESS;
        goto CleanupExit;
    }

    //
    // Perform the XML to JSON conversion with prefix normalization.
    //
    status = PhConvertSettingsXmlToJson(
        &settingsFilePath->sr,
        &convertFilePath->sr
        );

CleanupExit:
    if (settingsFilePath) PhDereferenceObject(settingsFilePath);
    if (convertFilePath) PhDereferenceObject(convertFilePath);
    return status;
}

/**
 * Extracts a resource from the DLL to a file.
 *
 * \param DllBase The base address of the DLL.
 * \param Name The name of the resource.
 * \param FileName The name of the file to create.
 * \return Successful or errant status.
 */
NTSTATUS ExtractResourceToFile(
    _In_ PVOID DllBase,
    _In_ PCWSTR Name,
    _In_ PCWSTR FileName
    )
{
    NTSTATUS status;
    HANDLE fileHandle = NULL;
    PVOID resourceBuffer;
    ULONG resourceLength;
    LARGE_INTEGER allocationSize;
    IO_STATUS_BLOCK isb;

    status = PhLoadResource(
        DllBase,
        Name,
        RT_RCDATA,
        &resourceLength,
        &resourceBuffer
        );

    if (!NT_SUCCESS(status))
        return status;

    allocationSize.QuadPart = resourceLength;

    status = PhCreateFileWin32Ex(
        &fileHandle,
        FileName,
        FILE_GENERIC_READ | FILE_GENERIC_WRITE | DELETE,
        &allocationSize,
        FILE_ATTRIBUTE_NORMAL,
        FILE_SHARE_READ | FILE_SHARE_WRITE | FILE_SHARE_DELETE,
        FILE_OVERWRITE_IF,
        FILE_NON_DIRECTORY_FILE | FILE_SYNCHRONOUS_IO_NONALERT,
        NULL
        );

    if (!NT_SUCCESS(status))
        return status;

    status = NtWriteFile(
        fileHandle,
        NULL,
        NULL,
        NULL,
        &isb,
        resourceBuffer,
        resourceLength,
        NULL,
        NULL
        );

    NtClose(fileHandle);

    if (NT_SUCCESS(status))
    {
        if (isb.Information != resourceLength)
        {
            return STATUS_FAIL_CHECK;
        }
    }

    return status;
}

/**
 * Checks if an internet connection is available.
 *
 * \return TRUE if an internet connection is available, otherwise FALSE.
 */
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

/**
 * Creates a shell link (shortcut) file.
 *
 * \param LinkFilePath The path to the link file to create.
 * \param FilePath The path to the target file.
 * \param FileParentDir The parent directory of the target file.
 * \param AppId The Application User Model ID (AppId) for the shortcut.
 */
HRESULT SetupCreateLink(
    _In_ PCWSTR LinkFilePath,
    _In_ PCWSTR FilePath,
    _In_ PCWSTR FileParentDir,
    _In_ PCWSTR AppId
    )
{
    HRESULT status = S_OK;
    IShellLink* shellLinkPtr = NULL;
    IPersistFile* persistFilePtr = NULL;
    IPropertyStore* propertyStorePtr = NULL; // Initialize to NULL for safety
    LPWSTR pwszVal = NULL; // Initialize to NULL for safety

    status = CoCreateInstance(&CLSID_ShellLink, NULL, CLSCTX_INPROC_SERVER, &IID_IShellLinkW, &shellLinkPtr);
    if (FAILED(status))
        goto CleanupExit;

    status = IShellLinkW_QueryInterface(shellLinkPtr, &IID_IPersistFile, &persistFilePtr);
    if (FAILED(status))
        goto CleanupExit;

    status = IShellLinkW_QueryInterface(shellLinkPtr, &IID_IPropertyStore, &propertyStorePtr);
    if (SUCCEEDED(status)) // Only proceed if QueryInterface for IPropertyStore succeeded
    {
        SIZE_T propValueLength;
        PROPVARIANT propValue;

        propValueLength = PhCountStringZ(AppId) * sizeof(WCHAR);

        PropVariantInit(&propValue);
        V_VT(&propValue) = VT_LPWSTR;
        pwszVal = (LPWSTR)CoTaskMemAlloc(propValueLength + sizeof(UNICODE_NULL));
        V_UNION(&propValue, pwszVal) = pwszVal;

        if (!pwszVal)
        {
            status = E_OUTOFMEMORY; // Set status for allocation failure
            goto CleanupExit; // Jump to cleanup immediately
        }

        memset(V_UNION(&propValue, pwszVal), 0, propValueLength + sizeof(UNICODE_NULL));
        memcpy(V_UNION(&propValue, pwszVal), AppId, propValueLength);

        status = IPropertyStore_SetValue(propertyStorePtr, &PKEY_AppUserModel_ID, &propValue);
        // If SetValue fails, we might still want to try to save the shortcut (without AppId)
        // or just let the CleanupExit handle it. For now, we continue and check overall save status.

        IPropertyStore_Commit(propertyStorePtr);
        IPropertyStore_Release(propertyStorePtr); // Release immediately if successful
        propertyStorePtr = NULL; // Clear pointer after release

        PropVariantClear(&propValue);
    }
    // If QueryInterface for IPropertyStore failed, status remains the error from that,
    // and we proceed without setting AppId.

    //IShellLinkW_SetDescription(shellLinkPtr, FileComment);
    //IShellLinkW_SetHotkey(shellLinkPtr, MAKEWORD(VK_END, HOTKEYF_CONTROL | HOTKEYF_ALT));
    IShellLinkW_SetWorkingDirectory(shellLinkPtr, FileParentDir);
    IShellLinkW_SetIconLocation(shellLinkPtr, FilePath, 0);

    status = IShellLinkW_SetPath(shellLinkPtr, FilePath);
    if (FAILED(status))
        goto CleanupExit;

    if (PhDoesFileExistWin32(LinkFilePath))
        PhDeleteFileWin32(LinkFilePath);

    status = IPersistFile_Save(persistFilePtr, LinkFilePath, TRUE);
    if (FAILED(status))
        goto CleanupExit;

CleanupExit:
    if (persistFilePtr)
        IPersistFile_Release(persistFilePtr);
    if (shellLinkPtr)
        IShellLinkW_Release(shellLinkPtr);
    if (propertyStorePtr) // Ensure propertyStorePtr is released if it was acquired and not yet released
        IPropertyStore_Release(propertyStorePtr);

    // Only notify if creation/update was successful
    if (SUCCEEDED(status))
        SHChangeNotify(SHCNE_UPDATEITEM, SHCNF_PATH, LinkFilePath, NULL);

    return status;
}

/**
 * Checks if System Informer is installed.
 *
 * \return TRUE if System Informer is installed, otherwise FALSE.
 */
BOOLEAN CheckApplicationInstalled(
    VOID
    )
{
    BOOLEAN installed = FALSE;
    PPH_STRING installPath;
    PPH_STRING exePath;

    if (installPath = GetApplicationInstallPath())
    {
        if (exePath = SetupCreateFullPath(installPath, L"\\SystemInformer.exe"))
        {
            installed = PhDoesFileExistWin32(PhGetString(exePath));
            PhDereferenceObject(exePath);
        }

        PhDereferenceObject(installPath);
    }

    return installed;
}

/**
 * Creates the legacy application name (e.g., ProcessHacker.exe).
 *
 * \return A string containing the legacy application name.
 */
PPH_STRING CreateLegacyApplicationName(
    VOID
    )
{
    PH_FORMAT format[18];

    PhInitFormatC(&format[0], OBJ_NAME_PATH_SEPARATOR);
    PhInitFormatC(&format[1], L'P');
    PhInitFormatC(&format[2], L'r');
    PhInitFormatC(&format[3], L'o');
    PhInitFormatC(&format[4], L'c');
    PhInitFormatC(&format[5], L'e');
    PhInitFormatC(&format[6], L's');
    PhInitFormatC(&format[7], L's');
    PhInitFormatC(&format[8], L'H');
    PhInitFormatC(&format[9], L'a');
    PhInitFormatC(&format[10], L'c');
    PhInitFormatC(&format[11], L'k');
    PhInitFormatC(&format[12], L'e');
    PhInitFormatC(&format[13], L'r');
    PhInitFormatC(&format[14], L'.');
    PhInitFormatC(&format[15], L'e');
    PhInitFormatC(&format[16], L'x');
    PhInitFormatC(&format[17], L'e');

    return PhFormat(format, RTL_NUMBER_OF(format), 0);
}

/**
 * Checks if a legacy application is installed in the given directory.
 *
 * \param Directory The directory to check.
 * \return TRUE if a legacy application is found, otherwise FALSE.
 */
BOOLEAN CheckApplicationInstallPathLegacy(
    _In_ PPH_STRING Directory
    )
{
    BOOLEAN installed = FALSE;
    PPH_STRING fileName;
    PPH_STRING exePath;

    fileName = CreateLegacyApplicationName();

    // Check the directory for the legacy 'ProcessHacker.exe' executable.

    if (exePath = SetupCreateFullPath(Directory, PhGetString(fileName)))
    {
        installed = PhDoesFileExistWin32(PhGetString(exePath));
        PhDereferenceObject(exePath);
    }

    PhDereferenceObject(fileName);

    return installed;
}

/**
 * Retrieves the application's installation path from the registry.
 *
 * \return A string containing the installation path, or NULL if not found.
 */
PPH_STRING GetApplicationInstallPath(
    VOID
    )
{
    HANDLE keyHandle;
    PPH_STRING installPath = NULL;

    for (ULONG i = 0; i < RTL_NUMBER_OF(UninstallKeyNames); i++)
    {
        if (NT_SUCCESS(PhOpenKey(
            &keyHandle,
            KEY_READ | KEY_WOW64_64KEY,
            PH_KEY_LOCAL_MACHINE,
            &UninstallKeyNames[i],
            0
            )))
        {
            installPath = PhQueryRegistryStringZ(keyHandle, L"InstallLocation");
            NtClose(keyHandle);
            break;
        }
    }

#ifdef FORCE_TEST_UPDATE_LOCAL_INSTALL
    PH_STRINGREF debugpath = PH_STRINGREF_INIT(L"test_install\\");
    PhMoveReference(&installPath, PhGetApplicationDirectoryFileName(&debugpath, FALSE));
#endif

    return installPath;
}

/**
 * Checks if a legacy setup (Process Hacker 2) is installed.
 *
 * \return TRUE if a legacy setup is installed, otherwise FALSE.
 */
    NTSTATUS SetupLegacySetupInstalled(
    VOID
    )
{
    HANDLE keyHandle = NULL;
    PPH_STRING keyName = NULL;
    PPH_STRING processString = NULL;
    PPH_STRING hackerString = NULL;
    NTSTATUS status = STATUS_UNSUCCESSFUL; // Initialize status

    // Construct "Process"
    {
        PH_FORMAT format[7];
        ULONG i = 0;
        PhInitFormatC(&format[i++], L'P');
        PhInitFormatC(&format[i++], L'r');
        PhInitFormatC(&format[i++], L'o');
        PhInitFormatC(&format[i++], L'c');
        PhInitFormatC(&format[i++], L'e');
        PhInitFormatC(&format[i++], L's');
        PhInitFormatC(&format[i++], L's');
        processString = PhFormat(format, i, 0);
        if (!processString)
            goto CleanupExit;
    }

    // Construct "Hacker"
    {
        PH_FORMAT format[6];
        ULONG i = 0;
        PhInitFormatC(&format[i++], L'H');
        PhInitFormatC(&format[i++], L'a');
        PhInitFormatC(&format[i++], L'c');
        PhInitFormatC(&format[i++], L'k');
        PhInitFormatC(&format[i++], L'e');
        PhInitFormatC(&format[i++], L'r');
        hackerString = PhFormat(format, i, 0);
        if (!hackerString)
            goto CleanupExit;
    }

    keyName = PhConcatStrings(7, L"Software\\Microsoft\\Windows\\CurrentVersion\\Uninstall\\", PhGetString(processString), L"_", PhGetString(hackerString), L"2", L"_", L"is1");
    if (!keyName)
        goto CleanupExit;

    if (NT_SUCCESS(PhOpenKey(
        &keyHandle,
        KEY_READ,
        PH_KEY_LOCAL_MACHINE,
        &keyName->sr,
        0
        )))
    {
        NtClose(keyHandle);
        status = STATUS_SUCCESS; // Set status to success
    }

CleanupExit:
    if (processString) PhDereferenceObject(processString);
    if (hackerString) PhDereferenceObject(hackerString);
    if (keyName) PhDereferenceObject(keyName);
    return status; // Return status
}

/**
 * Internal worker for enumerating previous instances.
 *
 * \param RootDirectory The root directory handle.
 * \param Name The name of the object.
 * \param TypeName The type name of the object.
 * \param Context Optional context.
 * \return Successful or errant status.
 */
_Function_class_(PH_ENUM_DIRECTORY_OBJECTS)
static NTSTATUS NTAPI PhpPreviousInstancesCallback(
    _In_ HANDLE RootDirectory,
    _In_ PPH_STRINGREF Name,
    _In_ PPH_STRINGREF TypeName,
    _In_opt_ PVOID Context
    )
{
    HANDLE objectHandle;
    BOOLEAN setupMutant = FALSE;
    MUTANT_OWNER_INFORMATION objectInfo;

    if (!PhStartsWithStringRef2(Name, L"SiMutant_", TRUE) &&
        !(setupMutant = PhStartsWithStringRef2(Name, L"SiSetupMutant_", TRUE)) &&
        !PhStartsWithStringRef2(Name, L"SiViewerMutant_", TRUE))
    {
        return STATUS_SUCCESS;
    }

    if (!NT_SUCCESS(PhOpenMutant(
        &objectHandle,
        MUTANT_QUERY_STATE,
        RootDirectory,
        Name
        )))
    {
        return STATUS_SUCCESS;
    }

    if (NT_SUCCESS(PhGetMutantOwnerInformation(
        objectHandle,
        &objectInfo
        )))
    {
        HWND hwnd;
        HANDLE processHandle = NULL;
        PROCESS_BASIC_INFORMATION processInfo;

        if (objectInfo.ClientId.UniqueProcess == NtCurrentProcessId())
            goto CleanupExit;

        // Do not terminate the setup process if it's the parent of this process. This scenario
        // happens when the setup process restarts itself for elevation. The parent process will
        // return the same exit code as this setup process instance, terminating breaks that.
        if (setupMutant &&
            NT_SUCCESS(PhGetProcessBasicInformation(NtCurrentProcess(), &processInfo)) &&
            processInfo.InheritedFromUniqueProcessId == objectInfo.ClientId.UniqueProcess)
            goto CleanupExit;

        PhOpenProcessClientId(
            &processHandle,
            PROCESS_QUERY_LIMITED_INFORMATION | PROCESS_TERMINATE | SYNCHRONIZE,
            &objectInfo.ClientId
            );

        hwnd = PhGetProcessMainWindowEx(
            objectInfo.ClientId.UniqueProcess,
            processHandle,
            FALSE
            );

        if (hwnd)
        {
            PhEndWindowSession(hwnd);
            //SendMessageTimeout(hwnd, WM_QUIT, 0, 0, SMTO_BLOCK, 5000, NULL);
        }

        if (processHandle)
        {
            NTSTATUS status;

            status = NtTerminateProcess(processHandle, 1);

            if (status == STATUS_SUCCESS || status == STATUS_PROCESS_IS_TERMINATING)
            {
                NtTerminateProcess(processHandle, DBG_TERMINATE_PROCESS);
            }

            PhWaitForSingleObject(processHandle, 30 * 1000);
        }

    CleanupExit:
        if (processHandle) NtClose(processHandle);
    }

    NtClose(objectHandle);

    return STATUS_SUCCESS;
}

/**
 * Callback for checking a directory for running application instances.
 *
 * \param RootDirectory The root directory handle.
 * \param Information The file directory information.
 * \param Context The setup context.
 * \return TRUE to continue enumeration, FALSE to stop.
 */
_Function_class_(PH_ENUM_DIRECTORY_FILE)
static BOOLEAN CALLBACK SetupCheckDirectoryCallback(
    _In_ HANDLE RootDirectory,
    _In_ PVOID Information,
    _In_ PVOID Context
    )
{
    PFILE_DIRECTORY_INFORMATION information = (PFILE_DIRECTORY_INFORMATION)Information;
    PPH_SETUP_CONTEXT context = (PPH_SETUP_CONTEXT)Context;
    NTSTATUS status;
    PH_STRINGREF baseName;
    HANDLE fileHandle;

    baseName.Buffer = information->FileName;
    baseName.Length = information->FileNameLength;

    if (PhEqualStringRef2(&baseName, L".", TRUE) || PhEqualStringRef2(&baseName, L"..", TRUE))
        return TRUE;

    status = PhOpenFile(
        &fileHandle,
        &baseName,
        FILE_READ_ATTRIBUTES | DELETE | SYNCHRONIZE,
        RootDirectory,
        FILE_SHARE_READ | FILE_SHARE_WRITE | FILE_SHARE_DELETE,
        FILE_NON_DIRECTORY_FILE | FILE_SYNCHRONOUS_IO_NONALERT,
        NULL
        );

    if (NT_SUCCESS(status))
    {
        PFILE_PROCESS_IDS_USING_FILE_INFORMATION processIds;

        status = PhGetProcessIdsUsingFile(
            fileHandle,
            &processIds
            );

        if (NT_SUCCESS(status))
        {
            for (ULONG i = 0; i < processIds->NumberOfProcessIdsInList; i++)
            {
                HANDLE processId = processIds->ProcessIdList[i];
                PPH_STRING fileName;

                if (processId == NtCurrentProcessId())
                    continue;

                status = PhGetProcessImageFileNameByProcessId(
                    processId,
                    &fileName
                    );

                if (NT_SUCCESS(status))
                {
                    PPH_STRING baseDirectory;

                    PhMoveReference(&fileName, PhGetFileName(fileName));

                    if (baseDirectory = PhGetBaseDirectory(fileName))
                    {
                        if (PhStartsWithString(context->SetupInstallPath, baseDirectory, TRUE))
                        {
                            HANDLE processHandle;

                            if (NT_SUCCESS(PhOpenProcess(
                                &processHandle,
                                PROCESS_QUERY_LIMITED_INFORMATION | PROCESS_TERMINATE | SYNCHRONIZE,
                                processId
                                )))
                            {
                                HWND windowHandle;

                                if (windowHandle = PhGetProcessMainWindowEx(
                                    processId,
                                    processHandle,
                                    FALSE
                                    ))
                                {
                                    PhEndWindowSession(windowHandle);
                                    //SendMessageTimeout(windowHandle, WM_QUIT, 0, 0, SMTO_BLOCK, 5000, NULL);
                                }

                                status = NtTerminateProcess(processHandle, 1);

                                if (status == STATUS_SUCCESS || status == STATUS_PROCESS_IS_TERMINATING)
                                {
                                    NtTerminateProcess(processHandle, DBG_TERMINATE_PROCESS);
                                }

                                PhWaitForSingleObject(processHandle, 30 * 1000);
                                NtClose(processHandle);
                            }
                        }

                        PhDereferenceObject(baseDirectory);
                    }

                    PhDereferenceObject(fileName);
                }
            }

            PhFree(processIds);
        }

        NtClose(fileHandle);
    }

    return TRUE;
}

/**
 * Shuts down all running instances of the application.
 *
 * \param Context The setup context.
 *
 * \return Successful or errant status.
 */
NTSTATUS SetupShutdownApplication(
    _In_ PPH_SETUP_CONTEXT Context
    )
{
    NTSTATUS status;
    HANDLE directoryHandle;

    //
    // N.B. Since the current Canary build (3.2.25136.1352) doesn't create the
    // mutants we need to do both methods to shut down the application. (jxy-s)
    // 1. Enumerate the mutants.
    // 2. Enumerate directory with PhGetProcessIdsUsingFile.
    //

    PhEnumDirectoryObjects(
        PhGetNamespaceHandle(),
        PhpPreviousInstancesCallback,
        NULL
        );

    status = PhCreateFileWin32(
        &directoryHandle,
        PhGetString(Context->SetupInstallPath),
        FILE_LIST_DIRECTORY | SYNCHRONIZE,
        FILE_ATTRIBUTE_DIRECTORY,
        FILE_SHARE_READ,
        FILE_OPEN,
        FILE_DIRECTORY_FILE | FILE_SYNCHRONOUS_IO_NONALERT
        );

    if (NT_SUCCESS(status))
    {
        status = PhEnumDirectoryFile(
            directoryHandle,
            NULL,
            SetupCheckDirectoryCallback,
            Context
            );

        NtClose(directoryHandle);
    }

    //
    // N.B. Let the setup try to continue regardless. If we really want to fail
    // the setup we need to:
    // 1. If we find a target that needs to be terminate, note it.
    // 2. If that target fails to stop, return an error code.
    //
    // Currently the logic only fails if the mechanics of looking up files and
    // or mutants fails, not actually failing to shut down processes.
    //
    return STATUS_SUCCESS;
}

/**
 * Creates a full path by concatenating a path and a file name.
 *
 * \param Path The base path.
 * \param FileName The file name to append.
 * \return A string containing the full path.
 */
PPH_STRING SetupCreateFullPath(
    _In_ PPH_STRING Path,
    _In_ PCWSTR FileName
    )
{
    PPH_STRING pathString;
    PPH_STRING tempString;

    pathString = PhConcatStringRefZ(&Path->sr, FileName);

    if (NT_SUCCESS(PhGetFullPath(pathString->Buffer, &tempString, NULL)))
    {
        PhMoveReference(&pathString, tempString);
        return pathString;
    }

    return pathString;
}

/**
 * Overwrites a file with the given buffer content.
 *
 * \param FileName The name of the file to overwrite.
 * \param Buffer The buffer containing the data to write.
 * \param BufferLength The length of the buffer.
 * \return Successful or errant status.
 */
NTSTATUS SetupOverwriteFile(
    _In_ PPH_STRING FileName,
    _In_ PVOID Buffer,
    _In_ ULONG BufferLength
    )
{
    NTSTATUS status;
    HANDLE fileHandle = NULL;
    LARGE_INTEGER allocationSize;
    IO_STATUS_BLOCK isb;

    allocationSize.QuadPart = BufferLength;

    status = PhCreateFileWin32Ex(
        &fileHandle,
        PhGetString(FileName),
        FILE_GENERIC_WRITE | DELETE,
        &allocationSize,
        FILE_ATTRIBUTE_NORMAL,
        FILE_SHARE_READ | FILE_SHARE_WRITE | FILE_SHARE_DELETE,
        FILE_OVERWRITE_IF,
        FILE_NON_DIRECTORY_FILE | FILE_SYNCHRONOUS_IO_NONALERT,
        NULL
        );

    if (!NT_SUCCESS(status))
        goto CleanupExit;

    status = NtWriteFile(
        fileHandle,
        NULL,
        NULL,
        NULL,
        &isb,
        Buffer,
        BufferLength,
        NULL,
        NULL
        );

    if (!NT_SUCCESS(status))
        goto CleanupExit;

    if (isb.Information != BufferLength)
    {
        status = STATUS_UNSUCCESSFUL;
        goto CleanupExit;
    }

CleanupExit:

    if (fileHandle)
        NtClose(fileHandle);

    return status;
}

/**
 * Computes the SHA256 hash of a file.
 *
 * \param FileName The name of the file to hash.
 * \param Buffer A buffer to receive the hash.
 * \return Successful or errant status.
 */
NTSTATUS SetupHashFile(
    _In_ PPH_STRING FileName,
    _Out_writes_all_(256 / 8) PBYTE Buffer
    )
{
    NTSTATUS status;
    HANDLE fileHandle = NULL;
    IO_STATUS_BLOCK iosb;
    PH_HASH_CONTEXT hashContext;
    LARGE_INTEGER fileSize;
    ULONG64 bytesRemaining;
    BYTE buffer[PAGE_SIZE];

    status = PhCreateFileWin32(
        &fileHandle,
        PhGetString(FileName),
        FILE_GENERIC_READ,
        FILE_ATTRIBUTE_NORMAL,
        FILE_SHARE_READ | FILE_SHARE_WRITE | FILE_SHARE_DELETE,
        FILE_OPEN,
        FILE_NON_DIRECTORY_FILE | FILE_SYNCHRONOUS_IO_NONALERT
        );

    if (!NT_SUCCESS(status))
        goto CleanupExit;

    status = PhGetFileSize(fileHandle, &fileSize);

    if (!NT_SUCCESS(status))
        goto CleanupExit;

    PhInitializeHash(&hashContext, Sha256HashAlgorithm);

    bytesRemaining = fileSize.QuadPart;

    while (bytesRemaining)
    {
        status = NtReadFile(
            fileHandle,
            NULL,
            NULL,
            NULL,
            &iosb,
            buffer,
            sizeof(buffer),
            NULL,
            NULL
            );

        if (!NT_SUCCESS(status))
            goto CleanupExit;

        PhUpdateHash(&hashContext, buffer, (ULONG)iosb.Information);
        bytesRemaining -= (ULONG)iosb.Information;
    }

    if (!NT_SUCCESS(status))
        goto CleanupExit;

    status = PhFinalHash(&hashContext, Buffer, 256 / 8, NULL);

CleanupExit:

    if (fileHandle)
        NtClose(fileHandle);

    return status;
}
