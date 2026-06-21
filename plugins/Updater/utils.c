/*
 * Copyright (c) 2022 Winsider Seminars & Solutions, Inc.  All rights reserved.
 *
 * This file is part of System Informer.
 *
 * Authors:
 *
 *     dmex    2011-2026
 *
 */

#include "updater.h"

/**
 * Executes the setup process for updating System Informer.
 *
 * \param Context A pointer to the updater context structure.
 * \param WindowHandle An optional handle to the window to use for shell execution and UI dialogs.
 * \return NTSTATUS status of the execution.
 */
NTSTATUS UpdateShellExecute(
    _In_ PPH_UPDATER_CONTEXT Context,
    _In_opt_ HWND WindowHandle
    )
{
    NTSTATUS status;
    PPH_STRING parameters;

    // Reset the cache so we don't prompt again after the update.
    PhSetStringSetting(SETTING_NAME_UPDATE_DATA, L"");

    if (PhIsNullOrEmptyString(Context->SetupFilePath))
        return STATUS_FAIL_CHECK;

    parameters = PH_AUTO(PhCreateKsiSettingsBlob());
    if (!parameters) parameters = PH_AUTO(PhReferenceEmptyString());
    parameters = PH_AUTO(PhQuoteCommandLine(&parameters->sr, TRUE));
    parameters = PH_AUTO(PhConcatStrings2(L"-update ", PhGetString(parameters)));

    SystemInformer_PrepareForEarlyShutdown();

    status = PhShellExecuteEx(
        WindowHandle,
        PhGetString(Context->SetupFilePath),
        PhGetString(parameters),
        NULL,
        SW_SHOW,
        Context->ElevationRequired ? PH_SHELL_EXECUTE_ADMIN : PH_SHELL_EXECUTE_DEFAULT,
        0,
        NULL
        );

    if (Context->SetupFileHandle)
    {
        NtClose(Context->SetupFileHandle);
        Context->SetupFileHandle = NULL;
    }

    if (NT_SUCCESS(status))
    {
        Context->Cleanup = FALSE;

        SystemInformer_Destroy();
    }
    else
    {
        SystemInformer_CancelEarlyShutdown();

        if (status != STATUS_CANCELLED) // Ignore UAC decline.
        {
            PhShowStatus(WindowHandle, L"Unable to execute the setup.", status, 0);

            if (Context->StartupCheck)
                ShowAvailableDialog(Context);
            else
                ShowCheckForUpdatesDialog(Context);
        }
    }

    return status;
}

/**
 * Checks if write permission to the application directory requires administrator elevation.
 * \return TRUE if elevation is required, FALSE otherwise.
 */
BOOLEAN UpdateCheckDirectoryElevationRequired(
    VOID
    )
{
    static const PH_STRINGREF checkFileName = PH_STRINGREF_INIT(L"elevation_check");
    HANDLE fileHandle;
    PPH_STRING fileName;

    fileName = PhGetApplicationDirectoryFileName(&checkFileName, TRUE);

    if (PhIsNullOrEmptyString(fileName))
        return TRUE;

    if (NT_SUCCESS(PhCreateFile(
        &fileHandle,
        &fileName->sr,
        FILE_GENERIC_WRITE | DELETE,
        FILE_ATTRIBUTE_NORMAL,
        FILE_SHARE_READ | FILE_SHARE_DELETE,
        FILE_OPEN_IF,
        FILE_NON_DIRECTORY_FILE | FILE_SYNCHRONOUS_IO_NONALERT | FILE_DELETE_ON_CLOSE
        )))
    {
        PhDereferenceObject(fileName);
        NtClose(fileHandle);
        return FALSE;
    }

    PhDereferenceObject(fileName);
    return TRUE;
}

/**
 * Callback function triggered when a link in the task dialog is clicked.
 * \param Context A pointer to the updater context structure.
 */
VOID TaskDialogLinkClicked(
    _In_ PPH_UPDATER_CONTEXT Context
    )
{
    PhDialogBox(
        NtCurrentImageBase(),
        MAKEINTRESOURCE(IDD_TEXT),
        Context->DialogHandle,
        TextDlgProc,
        Context
        );
}

//BOOLEAN UpdaterInstalledUsingSetup(
//    VOID
//    )
//{
//    static PH_STRINGREF keyName = PH_STRINGREF_INIT(L"Software\\Microsoft\\Windows\\CurrentVersion\\Uninstall\\SystemInformer");
//    static PH_STRINGREF key2xName = PH_STRINGREF_INIT(L"Software\\Microsoft\\Windows\\CurrentVersion\\Uninstall\\Process_Hacker2_is1");
//    HANDLE keyHandle = NULL;
//
//    // Check uninstall entries for the 'ProcessHacker' registry key.
//    if (NT_SUCCESS(PhOpenKey(
//        &keyHandle,
//        KEY_READ,
//        PH_KEY_LOCAL_MACHINE,
//        &keyName,
//        0
//        )))
//    {
//        NtClose(keyHandle);
//        return TRUE;
//    }
//
//    // Check uninstall entries for the 2.x branch 'Process_Hacker2_is1' registry key.
//    if (NT_SUCCESS(PhOpenKey(
//        &keyHandle,
//        KEY_READ,
//        PH_KEY_LOCAL_MACHINE,
//        &key2xName,
//        0
//        )))
//    {
//        NtClose(keyHandle);
//        return TRUE;
//    }
//
//    return FALSE;
//}

/**
 * Determines if the configured update check interval has expired since the last update check.
 * \return TRUE if the last update check has expired, FALSE otherwise.
 */
BOOLEAN LastUpdateCheckExpired(
    VOID
    )
{
    ULONG lastTimeUpdateSeconds;
    LARGE_INTEGER lastTimeUpdateTicks;
    LARGE_INTEGER currentTimeUpdateTicks;
    LONG updateInterval;

    PhQuerySystemTime(&currentTimeUpdateTicks);
    lastTimeUpdateSeconds = PhGetIntegerSetting(SETTING_NAME_LAST_CHECK);

    if (lastTimeUpdateSeconds == 0)
    {
        PhTimeToSecondsSince1970(&currentTimeUpdateTicks, &lastTimeUpdateSeconds);
        PhSetIntegerSetting(SETTING_NAME_LAST_CHECK, lastTimeUpdateSeconds);
        return FALSE; // FirstRun
    }

    PhSecondsSince1970ToTime(lastTimeUpdateSeconds, &lastTimeUpdateTicks);

    updateInterval = PhGetIntegerSetting(SETTING_NAME_UPDATE_INTERVAL);
    updateInterval = __max(updateInterval, 1);
    updateInterval = __min(updateInterval, 90);

    if (currentTimeUpdateTicks.QuadPart - lastTimeUpdateTicks.QuadPart >= updateInterval * PH_TICKS_PER_DAY)
    {
        PhTimeToSecondsSince1970(&currentTimeUpdateTicks, &lastTimeUpdateSeconds);
        PhSetIntegerSetting(SETTING_NAME_LAST_CHECK, lastTimeUpdateSeconds);
        return TRUE;
    }

    return FALSE;
}

/**
 * Formats the current build version of the application as a string.
 * \return A string containing the formatted build version.
 */
PPH_STRING UpdateVersionString(
    VOID
    )
{
    static const PH_STRINGREF versionHeader = PH_STRINGREF_INIT(L"SystemInformer-Build: ");
    ULONG majorVersion;
    ULONG minorVersion;
    ULONG buildVersion;
    ULONG revisionVersion;
    SIZE_T returnLength;
    PH_FORMAT format[8];
    WCHAR formatBuffer[260];

    PhGetBuildVersionNumbers(&majorVersion, &minorVersion, &buildVersion, &revisionVersion);
    PhInitFormatSR(&format[0], versionHeader);
    PhInitFormatU(&format[1], majorVersion);
    PhInitFormatC(&format[2], L'.');
    PhInitFormatU(&format[3], minorVersion);
    PhInitFormatC(&format[4], L'.');
    PhInitFormatU(&format[5], buildVersion);
    PhInitFormatC(&format[6], L'.');
    PhInitFormatU(&format[7], revisionVersion);

    if (PhFormatToBuffer(format, RTL_NUMBER_OF(format), formatBuffer, sizeof(formatBuffer), &returnLength))
    {
        PH_STRINGREF stringFormat;

        stringFormat.Buffer = formatBuffer;
        stringFormat.Length = returnLength - sizeof(UNICODE_NULL);

        return PhCreateString2(&stringFormat);
    }
    else
    {
        return PhFormat(format, RTL_NUMBER_OF(format), 0);
    }
}

/**
 * Formats the current Windows OS build version and architecture as a string.
 * \return A string containing the formatted Windows OS build version, or NULL if it could not be retrieved.
 */
PPH_STRING UpdateWindowsString(
    VOID
    )
{
    PPH_STRING buildString = NULL;
    PPH_STRING fileName;
    PVOID imageBase;
    ULONG imageSize;
    PVOID versionInfo;

    if (NT_SUCCESS(PhGetKernelFileNameEx(&fileName, &imageBase, &imageSize)))
    {
        if (NT_SUCCESS(PhGetFileVersionInfoEx(&fileName->sr, &versionInfo)))
        {
            VS_FIXEDFILEINFO* rootBlock;

            if (rootBlock = PhGetFileVersionFixedInfo(versionInfo))
            {
                PH_FORMAT format[5];

                PhInitFormatS(&format[0], L"SystemInformer-OsBuild: ");
                PhInitFormatU(&format[1], HIWORD(rootBlock->dwFileVersionLS));
                PhInitFormatC(&format[2], '.');
                PhInitFormatU(&format[3], LOWORD(rootBlock->dwFileVersionLS));
                PhInitFormatS(&format[4], PhIsExecutingInWow64() ? L"_64" : L"_32");

                buildString = PhFormat(format, RTL_NUMBER_OF(format), 0);
            }

            PhFree(versionInfo);
        }

        PhDereferenceObject(fileName);
    }

    return buildString;
}

/**
 * Parses a version string into a ULONG64 representation.
 *
 * \param VersionString The version string to parse.
 * \return A ULONG64 representation of the version, or 0 if the version string is empty.
 */
ULONG64 ParseVersionString(
    _In_ PPH_STRING VersionString
    )
{
    PH_STRINGREF remaining;
    PH_STRINGREF majorPart;
    PH_STRINGREF minorPart;
    PH_STRINGREF buildPart;
    PH_STRINGREF revisionPart;
    ULONG64 majorInteger = 0;
    ULONG64 minorInteger = 0;
    ULONG64 buildInteger = 0;
    ULONG64 revisionInteger = 0;

    if (PhIsNullOrEmptyString(VersionString))
        return 0;

    remaining = PhGetStringRef(VersionString);
    PhSplitStringRefAtChar(&remaining, L'.', &majorPart, &remaining);
    PhSplitStringRefAtChar(&remaining, L'.', &minorPart, &remaining);
    PhSplitStringRefAtChar(&remaining, L'.', &buildPart, &remaining);
    PhSplitStringRefAtChar(&remaining, L'.', &revisionPart, &remaining);

    if (majorPart.Length)
    {
        PhStringToUInt64(&majorPart, 10, &majorInteger);
    }

    if (minorPart.Length)
    {
        PhStringToUInt64(&minorPart, 10, &minorInteger);
    }

    if (buildPart.Length)
    {
        PhStringToUInt64(&buildPart, 10, &buildInteger);
    }

    if (revisionPart.Length)
    {
        PhStringToUInt64(&revisionPart, 10, &revisionInteger);
    }

    return MAKE_VERSION_ULONGLONG(
        (ULONG)majorInteger,
        (ULONG)minorInteger,
        (ULONG)buildInteger,
        (ULONG)revisionInteger
        );
}

/**
 * Validates a file name to ensure it is a safe and correct executable name for an update.
 *
 * \param FileName The file name string to validate.
 * \return TRUE if the file name is valid and safe, FALSE otherwise.
 */
BOOLEAN UpdateValidateFileName(
    _In_ PPH_STRING FileName
    )
{
    SIZE_T length;
    ULONG i;
    WCHAR c;

    // Check for null/empty
    if (PhIsNullOrEmptyString(FileName))
        return FALSE;

    // Check length (Windows MAX_PATH is 260, filename component should be < 255)
    length = FileName->Length / sizeof(WCHAR);
    if (length == 0 || length >= 255)
        return FALSE;

    // Must end with .exe (case-insensitive)
    if (!PhEndsWithString2(FileName, L".exe", TRUE))
        return FALSE;

    // Check each character for validity
    for (i = 0; i < length; i++)
    {
        c = FileName->Buffer[i];

        // Reject null bytes
        if (c == L'\0')
            return FALSE;

        // Reject path separators
        if (c == L'\\' || c == L'/')
            return FALSE;

        // Reject Windows reserved characters
        if (c == L':' || c == L'*' || c == L'?' || 
            c == L'"' || c == L'<' || c == L'>' || c == L'|')
            return FALSE;

        // Reject control characters
        if (c < 32)
            return FALSE;
    }

    // Reject path traversal patterns (.. anywhere in filename)
    if (length >= 2)
    {
        for (i = 0; i < length - 1; i++)
        {
            if (FileName->Buffer[i] == L'.' && FileName->Buffer[i + 1] == L'.')
                return FALSE;
        }
    }

    // Only allow safe characters: alphanumeric, dots, hyphens, underscores
    for (i = 0; i < length; i++)
    {
        c = FileName->Buffer[i];
        
        if (!((c >= L'a' && c <= L'z') || 
              (c >= L'A' && c <= L'Z') || 
              (c >= L'0' && c <= L'9') ||
              c == L'.' || c == L'-' || c == L'_'))
        {
            return FALSE;
        }
    }

    return TRUE;
}
