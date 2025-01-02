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

PH_STRINGREF UninstallKeyNames[] =
{
    PH_STRINGREF_INIT(L"Software\\Microsoft\\Windows\\CurrentVersion\\Uninstall\\SystemInformer"),
    PH_STRINGREF_INIT(L"Software\\Microsoft\\Windows\\CurrentVersion\\Uninstall\\SystemInformer-Preview"),
    PH_STRINGREF_INIT(L"Software\\Microsoft\\Windows\\CurrentVersion\\Uninstall\\SystemInformer-Canary"),
    PH_STRINGREF_INIT(L"Software\\Microsoft\\Windows\\CurrentVersion\\Uninstall\\SystemInformer-Developer"),
};
PH_STRINGREF AppPathsKeyName = PH_STRINGREF_INIT(L"Software\\Microsoft\\Windows\\CurrentVersion\\App Paths\\SystemInformer.exe");
PH_STRINGREF TaskmgrIfeoKeyName = PH_STRINGREF_INIT(L"Software\\Microsoft\\Windows NT\\CurrentVersion\\Image File Execution Options\\taskmgr.exe");
PH_STRINGREF CurrentUserRunKeyName = PH_STRINGREF_INIT(L"Software\\Microsoft\\Windows\\CurrentVersion\\Run");

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

NTSTATUS SetupCreateUninstallKey(
    _In_ PPH_SETUP_CONTEXT Context
    )
{
    NTSTATUS status;
    HANDLE keyHandle;
    PH_STRINGREF value;
    ULONG ulong = 1;

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

        PhInitializeStringRef(&value, L"https://systeminformer.sourceforge.io/");
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

PPH_STRING SetupFindInstallDirectory(
    VOID
    )
{
    PPH_STRING setupInstallPath = GetApplicationInstallPath();

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

BOOLEAN SetupCreateUninstallFile(
    _In_ PPH_SETUP_CONTEXT Context
    )
{
    NTSTATUS status;
    PPH_STRING currentFilePath;
    PPH_STRING uninstallFilePath;

    if (!NT_SUCCESS(status = PhGetProcessImageFileNameWin32(NtCurrentProcess(), &currentFilePath)))
    {
        Context->ErrorCode = PhNtStatusToDosError(status);
        return FALSE;
    }

    // Check if the current image is running from the installation folder.

    if (PhStartsWithStringRef2(&currentFilePath->sr, PhGetString(Context->SetupInstallPath), TRUE))
    {
        return TRUE;
    }

    // Move the outdated setup.exe into the trash (temp) folder.

    uninstallFilePath = SetupCreateFullPath(Context->SetupInstallPath, L"\\systeminformer-setup.exe");

    if (PhDoesFileExistWin32(PhGetString(uninstallFilePath)))
    {
        PPH_STRING tempFileName = PhGetTemporaryDirectoryRandomAlphaFileName();

        if (!NT_SUCCESS(status = PhMoveFileWin32(PhGetString(uninstallFilePath), PhGetString(tempFileName), FALSE)))
        {
            Context->ErrorCode = PhNtStatusToDosError(status);
            return FALSE;
        }
    }

    // Copy the latest setup.exe to the installation folder, so that:
    // 1. The setup program doesn't disappear for user.
    // 2. The correct ACL is applied to the file. Moving the file would retain the existing ACL.

    if (!NT_SUCCESS(status = PhCopyFileWin32(PhGetString(currentFilePath), PhGetString(uninstallFilePath), FALSE)))
    {
        Context->ErrorCode = PhNtStatusToDosError(status);
        return FALSE;
    }

    return TRUE;
}

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

BOOLEAN SetupUninstallDriver(
    _In_ PPH_SETUP_CONTEXT Context
    )
{
    BOOLEAN success = TRUE;
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
                ULONG attempts = 60;

                do
                {
                    PhStopService(serviceHandle);

                    if (NT_SUCCESS(PhQueryServiceStatus(serviceHandle, &serviceStatus)))
                    {
                        if (serviceStatus.dwCurrentState == SERVICE_STOPPED)
                        {
                            break;
                        }
                    }

                    PhDelayExecution(1000);

                } while (--attempts != 0);
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
        NTSTATUS status;

        if (!NT_SUCCESS(status = PhDeleteService(serviceHandle)))
        {
            success = FALSE;
            Context->ErrorCode = PhNtStatusToDosError(status);
        }

        PhCloseServiceHandle(serviceHandle);
    }

    return success;
}

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

BOOLEAN NTAPI SetupDeleteAutoRunKeyCallback(
    _In_ HANDLE RootDirectory,
    _In_ PKEY_VALUE_FULL_INFORMATION Information,
    _In_opt_ PVOID Context
    )
{
    if (Information->Type == REG_SZ)
    {
        static PH_STRINGREF fileName = PH_STRINGREF_INIT(L"\\SystemInformer.exe");
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

VOID SetupDeleteWindowsOptions(
    _In_ PPH_SETUP_CONTEXT Context
    )
{
    PPH_STRING string;
    HANDLE keyHandle;

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

VOID SetupCreateShortcuts(
    _In_ PPH_SETUP_CONTEXT Context
    )
{
    PPH_STRING string;
    PPH_STRING clientPathString;
    //PH_STRINGREF desktopStartmenuPathSr = PH_STRINGREF_INIT(L"%ALLUSERSPROFILE%\\Microsoft\\Windows\\Start Menu\\Programs\\System Informer.lnk");
    //PH_STRINGREF peviewerShortcutPathSr = PH_STRINGREF_INIT(L"%ALLUSERSPROFILE%\\Microsoft\\Windows\\Start Menu\\Programs\\PE Viewer.lnk");
    //PH_STRINGREF desktopAllusersPathSr = PH_STRINGREF_INIT(L"%PUBLIC%\\Desktop\\System Informer.lnk");

    if (string = PhGetKnownFolderPathZ(&FOLDERID_ProgramData, L"\\Microsoft\\Windows\\Start Menu\\Programs\\System Informer.lnk"))
    {
        clientPathString = SetupCreateFullPath(Context->SetupInstallPath, L"\\SystemInformer.exe");

        SetupCreateLink(
            PhGetString(string),
            PhGetString(clientPathString),
            PhGetString(Context->SetupInstallPath),
            L"SystemInformer"
            );

        if (PhDoesFileExistWin32(PhGetString(string)))
        {
            SHChangeNotify(SHCNE_UPDATEITEM, SHCNF_PATH, PhGetString(string), NULL);
        }

        PhDereferenceObject(clientPathString);
        PhDereferenceObject(string);
    }

    if (string = PhGetKnownFolderPathZ(&FOLDERID_ProgramData, L"\\Microsoft\\Windows\\Start Menu\\Programs\\PE Viewer.lnk"))
    {
        clientPathString = SetupCreateFullPath(Context->SetupInstallPath, L"\\peview.exe");

        SetupCreateLink(
            PhGetString(string),
            PhGetString(clientPathString),
            PhGetString(Context->SetupInstallPath),
            L"SystemInformer_PEViewer"
            );

        if (PhDoesFileExistWin32(PhGetString(string)))
        {
            SHChangeNotify(SHCNE_UPDATEITEM, SHCNF_PATH, PhGetString(string), NULL);
        }

        PhDereferenceObject(clientPathString);
        PhDereferenceObject(string);
    }

    if (string = PhGetKnownFolderPathZ(&FOLDERID_PublicDesktop, L"\\System Informer.lnk"))
    {
        clientPathString = SetupCreateFullPath(Context->SetupInstallPath, L"\\SystemInformer.exe");

        SetupCreateLink(
            PhGetString(string),
            PhGetString(clientPathString),
            PhGetString(Context->SetupInstallPath),
            L"SystemInformer"
            );

        if (PhDoesFileExistWin32(PhGetString(string)))
        {
            SHChangeNotify(SHCNE_UPDATEITEM, SHCNF_PATH, PhGetString(string), NULL);
        }

        PhDereferenceObject(clientPathString);
        PhDereferenceObject(string);
    }

    // PhGetKnownLocation(CSIDL_COMMON_PROGRAMS, L"\\System Informer.lnk"))
    // PhGetKnownLocation(CSIDL_COMMON_DESKTOPDIRECTORY, L"\\System Informer.lnk")
    // PhGetKnownLocation(CSIDL_COMMON_PROGRAMS, L"\\PE Viewer.lnk")
}

VOID SetupDeleteShortcuts(
    _In_ PPH_SETUP_CONTEXT Context
    )
{
    PPH_STRING string;
    HANDLE keyHandle;

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
}

NTSTATUS SetupExecuteApplication(
    _In_ PPH_SETUP_CONTEXT Context
    )
{
    NTSTATUS status;
    PPH_STRING string;

    if (PhIsNullOrEmptyString(Context->SetupInstallPath))
        return FALSE;

    string = SetupCreateFullPath(Context->SetupInstallPath, L"\\SystemInformer.exe");

    if (PhDoesFileExistWin32(PhGetString(string)))
    {
        status = PhShellExecuteEx(
            Context->DialogHandle,
            PhGetString(string),
            SETUP_APP_PARAMETERS,
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

    PhDereferenceObject(string);
    return status;
}

VOID SetupUpgradeSettingsFile(
    VOID
    )
{
    BOOLEAN migratedNightly = FALSE;
    PPH_STRING settingsFilePath;
    PPH_STRING legacyNightlyFileName;
    PPH_STRING legacySettingsFileName;

    settingsFilePath = PhGetKnownFolderPathZ(&FOLDERID_RoamingAppData, L"\\SystemInformer\\settings.xml");
    legacyNightlyFileName = PhGetKnownFolderPathZ(&FOLDERID_RoamingAppData, L"\\Process Hacker\\settings.xml");
    legacySettingsFileName = PhGetKnownFolderPathZ(&FOLDERID_RoamingAppData, L"\\Process Hacker 2\\settings.xml");

    if (settingsFilePath && legacyNightlyFileName)
    {
        if (!PhDoesFileExistWin32(PhGetString(settingsFilePath)))
        {
            if (NT_SUCCESS(PhCopyFileWin32(PhGetString(legacyNightlyFileName), PhGetString(settingsFilePath), TRUE)))
            {
                migratedNightly = TRUE;
            }
        }
    }

    if (!migratedNightly && settingsFilePath && legacySettingsFileName)
    {
        if (!PhDoesFileExistWin32(PhGetString(settingsFilePath)))
        {
            PhCopyFileWin32(PhGetString(legacySettingsFileName), PhGetString(settingsFilePath), TRUE);
        }
    }

    if (legacySettingsFileName) PhDereferenceObject(legacySettingsFileName);
    if (legacyNightlyFileName) PhDereferenceObject(legacyNightlyFileName);
    if (settingsFilePath) PhDereferenceObject(settingsFilePath);
}

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

BOOLEAN CheckApplicationInstallPathLegacy(
    _In_ PPH_STRING Directory
    )
{
    BOOLEAN installed = FALSE;
    PPH_STRING exePath;

    // Check the directory for the legacy 'ProcessHacker.exe' executable.

    if (exePath = SetupCreateFullPath(Directory, L"\\ProcessHacker.exe"))
    {
        installed = PhDoesFileExistWin32(PhGetString(exePath));
        PhDereferenceObject(exePath);
    }

    return installed;
}

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

static BOOLEAN NTAPI PhpPreviousInstancesCallback(
    _In_ HANDLE RootDirectory,
    _In_ PPH_STRINGREF Name,
    _In_ PPH_STRINGREF TypeName,
    _In_opt_ PVOID Context
    )
{
    HANDLE objectHandle;
    BOOLEAN setupMutant = FALSE;
    UNICODE_STRING objectNameUs;
    OBJECT_ATTRIBUTES objectAttributes;
    MUTANT_OWNER_INFORMATION objectInfo;

    if (!PhStartsWithStringRef2(Name, L"SiMutant_", TRUE) &&
        !(setupMutant = PhStartsWithStringRef2(Name, L"SiSetupMutant_", TRUE)) &&
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
        RootDirectory,
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
            SendMessageTimeout(hwnd, WM_QUIT, 0, 0, SMTO_BLOCK, 5000, NULL);
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

    return TRUE;
}

BOOLEAN SetupShutdownApplication(
    _In_ PPH_SETUP_CONTEXT Context
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

    if (NT_SUCCESS(PhGetFullPath(pathString->Buffer, &tempString, NULL)))
    {
        PhMoveReference(&pathString, tempString);
        return pathString;
    }

    return pathString;
}

BOOLEAN SetupOverwriteFile(
    _In_ PPH_STRING FileName,
    _In_ PVOID Buffer,
    _In_ ULONG BufferLength
    )
{
    BOOLEAN result = FALSE;
    HANDLE fileHandle = NULL;
    LARGE_INTEGER allocationSize;
    IO_STATUS_BLOCK isb;

    allocationSize.QuadPart = BufferLength;

    if (!NT_SUCCESS(PhCreateFileWin32Ex(
        &fileHandle,
        PhGetString(FileName),
        FILE_GENERIC_WRITE,
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
        Buffer,
        BufferLength,
        NULL,
        NULL
        )))
    {
        goto CleanupExit;
    }

    if (isb.Information != BufferLength)
        goto CleanupExit;

    result = TRUE;

CleanupExit:

    if (fileHandle)
        NtClose(fileHandle);

    return result;
}

_Success_(return)
BOOLEAN SetupHashFile(
    _In_ PPH_STRING FileName,
    _Out_writes_all_(256 / 8) PBYTE Buffer
    )
{
    BOOLEAN result = FALSE;
    HANDLE fileHandle = NULL;
    NTSTATUS status;
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

    result = PhFinalHash(&hashContext, Buffer, 256 / 8, NULL);

CleanupExit:

    if (fileHandle)
        NtClose(fileHandle);

    return result;
}
