/*
 * Copyright (c) 2022 Winsider Seminars & Solutions, Inc.  All rights reserved.
 *
 * This file is part of System Informer.
 *
 * Authors:
 *
 *     dmex    2011-2023
 *
 */

#include "updater.h"

HWND UpdateDialogHandle = NULL;
HANDLE UpdateDialogThreadHandle = NULL;
PH_EVENT InitializedEvent = PH_EVENT_INIT;
PPH_OBJECT_TYPE UpdateContextType = NULL;
PH_INITONCE UpdateContextTypeInitOnce = PH_INITONCE_INIT;

VOID UpdateContextDeleteProcedure(
    _In_ PVOID Object,
    _In_ ULONG Flags
    )
{
    PPH_UPDATER_CONTEXT context = Object;

    if (context->SetupFilePath)
    {
        if (context->Cleanup)
        {
            PhDeleteCacheFile(context->SetupFilePath);
        }

        PhDereferenceObject(context->SetupFilePath);
    }

    if (context->CurrentVersionString)
        PhDereferenceObject(context->CurrentVersionString);
    if (context->Version)
        PhDereferenceObject(context->Version);
    if (context->RelDate)
        PhDereferenceObject(context->RelDate);
    if (context->CommitHash)
        PhDereferenceObject(context->CommitHash);
    if (context->SetupFileDownloadUrl)
        PhDereferenceObject(context->SetupFileDownloadUrl);
    if (context->SetupFileLength)
        PhDereferenceObject(context->SetupFileLength);
    if (context->SetupFileHash)
        PhDereferenceObject(context->SetupFileHash);
    if (context->SetupFileSignature)
        PhDereferenceObject(context->SetupFileSignature);
}

PPH_UPDATER_CONTEXT CreateUpdateContext(
    _In_ BOOLEAN StartupCheck
    )
{
    PPH_UPDATER_CONTEXT context;

    if (PhBeginInitOnce(&UpdateContextTypeInitOnce))
    {
        UpdateContextType = PhCreateObjectType(L"UpdaterContextObjectType", 0, UpdateContextDeleteProcedure);
        PhEndInitOnce(&UpdateContextTypeInitOnce);
    }

    context = PhCreateObject(sizeof(PH_UPDATER_CONTEXT), UpdateContextType);
    memset(context, 0, sizeof(PH_UPDATER_CONTEXT));

    context->CurrentVersionString = PhGetPhVersion();
    context->StartupCheck = StartupCheck;
    context->Cleanup = TRUE;

    return context;
}

BOOLEAN UpdateShellExecute(
    _In_ PPH_UPDATER_CONTEXT Context,
    _In_opt_ HWND WindowHandle
    )
{
    PPH_STRING parameters;

    // Reset the cache so we don't prompt again after the update.
    PhSetStringSetting(SETTING_NAME_UPDATE_DATA, L"");

    if (PhIsNullOrEmptyString(Context->SetupFilePath))
        return FALSE;

    parameters = PH_AUTO(PhCreateKsiSettingsBlob());
    parameters = PH_AUTO(PhConcatStrings(3, L"-update \"", PhGetStringOrEmpty(parameters), L"\""));

    ProcessHacker_PrepareForEarlyShutdown();

    if (PhShellExecuteEx(
        WindowHandle,
        PhGetString(Context->SetupFilePath),
        PhGetString(parameters),
        NULL,
        SW_SHOW,
        Context->DirectoryElevationRequired ? PH_SHELL_EXECUTE_ADMIN : PH_SHELL_EXECUTE_DEFAULT,
        0,
        NULL
        ))
    {
        Context->Cleanup = FALSE;

        ProcessHacker_Destroy();

        return TRUE;
    }
    else
    {
        ULONG errorCode = GetLastError();

        // Install failed, cancel the shutdown.
        ProcessHacker_CancelEarlyShutdown();

        // Show error dialog.
        if (errorCode != ERROR_CANCELLED) // Ignore UAC decline.
        {
            PhShowStatus(WindowHandle, L"Unable to execute the setup.", 0, errorCode);

            if (Context->StartupCheck)
                ShowAvailableDialog(Context);
            else
                ShowCheckForUpdatesDialog(Context);
        }

        return FALSE;
    }
}

BOOLEAN UpdateCheckDirectoryElevationRequired(
    VOID
    )
{
    static PH_STRINGREF checkFileName = PH_STRINGREF_INIT(L"elevation_check");
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

VOID TaskDialogLinkClicked(
    _In_ PPH_UPDATER_CONTEXT Context
    )
{
    PhDialogBox(
        PluginInstance->DllBase,
        MAKEINTRESOURCE(IDD_TEXT),
        Context->DialogHandle,
        TextDlgProc,
        Context
        );
}

BOOLEAN UpdaterInstalledUsingSetup(
    VOID
    )
{
    static PH_STRINGREF keyName = PH_STRINGREF_INIT(L"Software\\Microsoft\\Windows\\CurrentVersion\\Uninstall\\ProcessHacker");
    static PH_STRINGREF key2xName = PH_STRINGREF_INIT(L"Software\\Microsoft\\Windows\\CurrentVersion\\Uninstall\\Process_Hacker2_is1");
    HANDLE keyHandle = NULL;

    // Check uninstall entries for the 'ProcessHacker' registry key.
    if (NT_SUCCESS(PhOpenKey(
        &keyHandle,
        KEY_READ,
        PH_KEY_LOCAL_MACHINE,
        &keyName,
        0
        )))
    {
        NtClose(keyHandle);
        return TRUE;
    }

    // Check uninstall entries for the 2.x branch 'Process_Hacker2_is1' registry key.
    if (NT_SUCCESS(PhOpenKey(
        &keyHandle,
        KEY_READ,
        PH_KEY_LOCAL_MACHINE,
        &key2xName,
        0
        )))
    {
        NtClose(keyHandle);
        return TRUE;
    }

    return FALSE;
}

BOOLEAN LastUpdateCheckExpired(
    VOID
    )
{
    ULONG64 lastUpdateTimeTicks;
    LARGE_INTEGER currentUpdateTimeTicks;
    PPH_STRING lastUpdateTimeString;

    PhQuerySystemTime(&currentUpdateTimeTicks);

    lastUpdateTimeString = PhGetStringSetting(SETTING_NAME_LAST_CHECK);

    if (PhIsNullOrEmptyString(lastUpdateTimeString))
        return TRUE;

    if (!PhStringToInteger64(&lastUpdateTimeString->sr, 0, &lastUpdateTimeTicks))
        return TRUE;

    if (currentUpdateTimeTicks.QuadPart - lastUpdateTimeTicks >= 7 * PH_TICKS_PER_DAY)
    {
        PPH_STRING currentUpdateTimeString;

        currentUpdateTimeString = PhIntegerToString64(currentUpdateTimeTicks.QuadPart, 0, FALSE);
        PhSetStringSetting2(SETTING_NAME_LAST_CHECK, &currentUpdateTimeString->sr);

        PhDereferenceObject(currentUpdateTimeString);
        PhDereferenceObject(lastUpdateTimeString);
        return TRUE;
    }

    PhDereferenceObject(lastUpdateTimeString);
    return FALSE;
}

PPH_STRING UpdateVersionString(
    VOID
    )
{
    static PH_STRINGREF versionHeader = PH_STRINGREF_INIT(L"SystemInformer-Build: ");
    ULONG majorVersion;
    ULONG minorVersion;
    ULONG buildVersion;
    ULONG revisionVersion;
    SIZE_T returnLength;
    PH_FORMAT format[7];
    WCHAR formatBuffer[260];

    PhGetPhVersionNumbers(&majorVersion, &minorVersion, &buildVersion, &revisionVersion);

    PhInitFormatU(&format[0], majorVersion);
    PhInitFormatC(&format[1], L'.');
    PhInitFormatU(&format[2], minorVersion);
    PhInitFormatC(&format[3], L'.');
    PhInitFormatU(&format[4], buildVersion);
    PhInitFormatC(&format[5], L'.');
    PhInitFormatU(&format[6], revisionVersion);

    if (PhFormatToBuffer(format, RTL_NUMBER_OF(format), formatBuffer, sizeof(formatBuffer), &returnLength))
    {
        PH_STRINGREF stringFormat;

        stringFormat.Buffer = formatBuffer;
        stringFormat.Length = returnLength - sizeof(UNICODE_NULL);

        return PhConcatStringRef2(&versionHeader, &stringFormat);
    }
    else
    {
        PPH_STRING currentVersion = NULL;
        PPH_STRING stringFormat;

        if (stringFormat = PhFormatString(L"%lu.%lu.%lu.%lu", majorVersion, minorVersion, buildVersion, revisionVersion))
        {
            currentVersion = PhConcatStringRef2(&versionHeader, &stringFormat->sr);
            PhDereferenceObject(currentVersion);
        }

        return currentVersion;
    }
}

PPH_STRING UpdateWindowsString(
    VOID
    )
{
    PPH_STRING buildString = NULL;
    PPH_STRING fileName = NULL;
    PPH_STRING fileVersion = NULL;
    PVOID versionInfo;
    VS_FIXEDFILEINFO* rootBlock;
    PH_FORMAT fileVersionFormat[3];

    fileName = PhGetKernelFileName2();
    versionInfo = PhGetFileVersionInfoEx(&fileName->sr);
    PhDereferenceObject(fileName);

    if (versionInfo)
    {
        if (rootBlock = PhGetFileVersionFixedInfo(versionInfo))
        {
            PhInitFormatU(&fileVersionFormat[0], HIWORD(rootBlock->dwFileVersionLS));
            PhInitFormatC(&fileVersionFormat[1], '.');
            PhInitFormatU(&fileVersionFormat[2], LOWORD(rootBlock->dwFileVersionLS));

            fileVersion = PhFormat(fileVersionFormat, 3, 30);
        }

        PhFree(versionInfo);
    }

    if (fileVersion)
    {
        if (PhIsExecutingInWow64())
            buildString = PhFormatString(L"%s: %s.%s", L"SystemInformer-OsBuild", fileVersion->Buffer, L"x86fre");
        else
            buildString = PhFormatString(L"%s: %s.%s", L"SystemInformer-OsBuild", fileVersion->Buffer, L"amd64fre");

        PhDereferenceObject(fileVersion);
    }

    return buildString;
}

ULONG64 ParseVersionString(
    _Inout_ PPH_STRING VersionString
    )
{
    PH_STRINGREF remaining;
    PH_STRINGREF majorPart;
    PH_STRINGREF minorPart;
    PH_STRINGREF revisionPart;
    ULONG64 majorInteger = 0;
    ULONG64 minorInteger = 0;
    ULONG64 revisionInteger = 0;

    remaining = PhGetStringRef(VersionString);

    PhSplitStringRefAtChar(&remaining, L'.', &majorPart, &remaining);
    PhSplitStringRefAtChar(&remaining, L'.', &minorPart, &remaining);
    PhSplitStringRefAtChar(&remaining, L'.', &revisionPart, &remaining);

    PhStringToInteger64(&majorPart, 10, &majorInteger);
    PhStringToInteger64(&minorPart, 10, &minorInteger);
    PhStringToInteger64(&revisionPart, 10, &revisionInteger);

    return MAKE_VERSION_ULONGLONG(
        (ULONG)majorInteger,
        (ULONG)minorInteger,
        (ULONG)revisionInteger,
        0
        );
}

BOOLEAN QueryUpdateData(
    _Inout_ PPH_UPDATER_CONTEXT Context,
    _In_ PWSTR ServerName
    )
{
    BOOLEAN success = FALSE;
    PPH_HTTP_CONTEXT httpContext = NULL;
    PPH_BYTES jsonString = NULL;
    PVOID jsonObject = NULL;

    if (!PhHttpSocketCreate(&httpContext, NULL))
    {
        Context->ErrorCode = GetLastError();
        goto CleanupExit;
    }

    //
    // TODO configurable nightly/release option.
    //
    Context->Type = UpdaterTypeNightly;

    if (!PhHttpSocketConnect(
        httpContext,
        ServerName,
        PH_HTTP_DEFAULT_HTTPS_PORT
        ))
    {
        Context->ErrorCode = GetLastError();
        goto CleanupExit;
    }

    if (!PhHttpSocketBeginRequest(
        httpContext,
        NULL,
        L"/nightly.php?update",
        PH_HTTP_FLAG_REFRESH | PH_HTTP_FLAG_SECURE
        ))
    {
        Context->ErrorCode = GetLastError();
        goto CleanupExit;
    }

    {
        PPH_STRING versionHeader;
        PPH_STRING windowsHeader;

        if (versionHeader = UpdateVersionString())
        {
            PhHttpSocketAddRequestHeaders(httpContext, versionHeader->Buffer, (ULONG)versionHeader->Length / sizeof(WCHAR));
            PhDereferenceObject(versionHeader);
        }

        if (windowsHeader = UpdateWindowsString())
        {
            PhHttpSocketAddRequestHeaders(httpContext, windowsHeader->Buffer, (ULONG)windowsHeader->Length / sizeof(WCHAR));
            PhDereferenceObject(windowsHeader);
        }
    }

    if (!PhHttpSocketSendRequest(httpContext, NULL, 0))
    {
        Context->ErrorCode = GetLastError();
        goto CleanupExit;
    }

    if (!PhHttpSocketEndRequest(httpContext))
    {
        Context->ErrorCode = GetLastError();
        goto CleanupExit;
    }

    if (!(jsonString = PhHttpSocketDownloadString(httpContext, FALSE)))
    {
        Context->ErrorCode = GetLastError();
        goto CleanupExit;
    }

    if (!(jsonObject = PhCreateJsonParserEx(jsonString, FALSE)))
        goto CleanupExit;

    Context->Version = PhGetJsonValueAsString(jsonObject, "version");
    Context->RelDate = PhGetJsonValueAsString(jsonObject, "updated");
    Context->CommitHash = PhGetJsonValueAsString(jsonObject, "commit");
    Context->SetupFileDownloadUrl = PhGetJsonValueAsString(jsonObject, "setup_url");
    Context->SetupFileLength = PhFormatSize(PhGetJsonValueAsUInt64(jsonObject, "setup_length"), 2);
    Context->SetupFileHash = PhGetJsonValueAsString(jsonObject, "setup_hash");
    Context->SetupFileSignature = PhGetJsonValueAsString(jsonObject, "setup_sig");

#ifdef FORCE_LATEST_VERSION
    Context->CurrentVersion = ParseVersionString(PhCreateString(L"0.0.0.0"));
    Context->LatestVersion = ParseVersionString(Context->CurrentVersionString);
#else
    Context->CurrentVersion = ParseVersionString(Context->CurrentVersionString);
    Context->LatestVersion = ParseVersionString(Context->Version);
#endif

    PhFreeJsonObject(jsonObject);

    if (PhIsNullOrEmptyString(Context->Version))
        goto CleanupExit;
    if (PhIsNullOrEmptyString(Context->RelDate))
        goto CleanupExit;
    if (PhIsNullOrEmptyString(Context->SetupFileDownloadUrl))
        goto CleanupExit;
    if (PhIsNullOrEmptyString(Context->SetupFileLength))
        goto CleanupExit;
    if (PhIsNullOrEmptyString(Context->SetupFileHash))
        goto CleanupExit;
    if (PhIsNullOrEmptyString(Context->SetupFileSignature))
        goto CleanupExit;
    if (PhIsNullOrEmptyString(Context->CommitHash))
        goto CleanupExit;

    success = TRUE;

    if (PhGetIntegerSetting(SETTING_NAME_UPDATE_MODE))
    {
        PPH_STRING jsonStringUtf16 = PhConvertUtf8ToUtf16Ex(jsonString->Buffer, jsonString->Length);
        PhSetStringSetting2(SETTING_NAME_UPDATE_DATA, &jsonStringUtf16->sr);
        PhDereferenceObject(jsonStringUtf16);
    }

CleanupExit:

    if (httpContext)
        PhHttpSocketDestroy(httpContext);
    if (jsonString)
        PhDereferenceObject(jsonString);

    return success;
}

BOOLEAN QueryUpdateDataWithFailover(
    _Inout_ PPH_UPDATER_CONTEXT Context
    )
{
    static PWSTR Servers[] =
    {
        L"systeminformer.sourceforge.io",
        L"system-informer.com",
        L"systeminformer.com"
    };

    for (USHORT i = 0; i < ARRAYSIZE(Servers); i++)
    {
        if (QueryUpdateData(Context, Servers[i]))
            return TRUE;
    }

    return FALSE;
}

NTSTATUS UpdateCheckSilentThread(
    _In_ PVOID Parameter
    )
{
    PPH_UPDATER_CONTEXT context;

    context = CreateUpdateContext(TRUE);

#ifndef FORCE_UPDATE_CHECK
    if (!LastUpdateCheckExpired())
        goto CleanupExit;
#endif

    PhDelayExecution(5 * 1000);

    PhClearCacheDirectory();

    // Query latest update information from the server.
    if (!QueryUpdateDataWithFailover(context))
        goto CleanupExit;

    // Compare the current version against the latest available version
    if (context->CurrentVersion < context->LatestVersion)
    {
        if (PhGetIntegerSetting(SETTING_NAME_UPDATE_MODE))
        {
            PhSetIntegerSetting(SETTING_NAME_UPDATE_AVAILABLE, TRUE);
        }
        else
        {
            // Check if the user hasn't already opened the dialog.
            if (!UpdateDialogHandle)
            {
                // We have data we're going to cache and pass into the dialog
                context->HaveData = TRUE;

                // Show the dialog asynchronously on a new thread.
                ShowUpdateDialog(context);
            }
        }
    }

CleanupExit:

    if (!context->HaveData)
        PhDereferenceObject(context);

    return STATUS_SUCCESS;
}

NTSTATUS UpdateCheckThread(
    _In_ PVOID Parameter
    )
{
    PPH_UPDATER_CONTEXT context;
    PH_AUTO_POOL autoPool;

    context = (PPH_UPDATER_CONTEXT)Parameter;
    context->ErrorCode = STATUS_SUCCESS;

    PhInitializeAutoPool(&autoPool);

    // Check if we have cached update data
    if (!context->HaveData)
    {
        context->HaveData = QueryUpdateDataWithFailover(context);
    }

    if (!context->HaveData)
    {
        PostMessage(context->DialogHandle, PH_SHOWERROR, FALSE, FALSE);
        goto CleanupExit;
    }

    if (context->CurrentVersion == context->LatestVersion)
    {
        // User is running the latest version
        PostMessage(context->DialogHandle, PH_SHOWLATEST, 0, 0);
    }
    else if (context->CurrentVersion > context->LatestVersion)
    {
        // User is running a newer version
        PostMessage(context->DialogHandle, PH_SHOWNEWEST, 0, 0);
    }
    else
    {
        // User is running an older version
        PostMessage(context->DialogHandle, PH_SHOWUPDATE, 0, 0);
    }

CleanupExit:
    PhDereferenceObject(context);
    PhDeleteAutoPool(&autoPool);

    return STATUS_SUCCESS;
}

PPH_STRING UpdateParseDownloadFileName(
    _In_ PPH_STRING DownloadUrlPath
    )
{
    PH_STRINGREF pathPart;
    PH_STRINGREF namePart;
    PPH_STRING downloadFileName;
    PPH_STRING localfileName;

    if (!PhSplitStringRefAtLastChar(&DownloadUrlPath->sr, L'/', &pathPart, &namePart))
        return NULL;

    downloadFileName = PhCreateString2(&namePart);
    localfileName = PhCreateCacheFile(downloadFileName);
    PhDereferenceObject(downloadFileName);

    return localfileName;
}

NTSTATUS UpdateDownloadThread(
    _In_ PVOID Parameter
    )
{
    PPH_UPDATER_CONTEXT context = (PPH_UPDATER_CONTEXT)Parameter;
    BOOLEAN downloadSuccess = FALSE;
    BOOLEAN hashSuccess = FALSE;
    BOOLEAN signatureSuccess = FALSE;
    HANDLE tempFileHandle = NULL;
    PPH_HTTP_CONTEXT httpContext = NULL;
    PPH_STRING downloadHostPath = NULL;
    PPH_STRING downloadUrlPath = NULL;
    PUPDATER_HASH_CONTEXT hashContext = NULL;
    PUPDATER_HASH_CONTEXT hashContextLegacy = NULL;
    ULONG contentLength = 0;
    USHORT httpPort = 0;
    LARGE_INTEGER timeNow;
    LARGE_INTEGER timeStart;
    ULONG64 timeTicks;
    ULONG64 timeBitsPerSecond;

    SendMessage(context->DialogHandle, TDM_UPDATE_ELEMENT_TEXT, TDE_MAIN_INSTRUCTION, (LPARAM)L"Initializing download request...");

    if (!PhHttpSocketParseUrl(
        context->SetupFileDownloadUrl,
        &downloadHostPath,
        &downloadUrlPath,
        &httpPort
        ))
    {
        context->ErrorCode = GetLastError();
        goto CleanupExit;
    }

    SendMessage(context->DialogHandle, TDM_UPDATE_ELEMENT_TEXT, TDE_MAIN_INSTRUCTION, (LPARAM)L"Connecting...");

    if (!PhHttpSocketCreate(&httpContext, NULL))
    {
        context->ErrorCode = GetLastError();
        goto CleanupExit;
    }

    if (!PhHttpSocketConnect(
        httpContext,
        PhGetString(downloadHostPath),
        httpPort
        ))
    {
        context->ErrorCode = GetLastError();
        goto CleanupExit;
    }

    if (!PhHttpSocketBeginRequest(
        httpContext,
        NULL,
        PhGetString(downloadUrlPath),
        PH_HTTP_FLAG_REFRESH | (httpPort == PH_HTTP_DEFAULT_HTTPS_PORT ? PH_HTTP_FLAG_SECURE : 0)
        ))
    {
        context->ErrorCode = GetLastError();
        goto CleanupExit;
    }

    SendMessage(context->DialogHandle, TDM_UPDATE_ELEMENT_TEXT, TDE_MAIN_INSTRUCTION, (LPARAM)L"Sending download request...");

    if (!PhHttpSocketSendRequest(httpContext, NULL, 0))
    {
        context->ErrorCode = GetLastError();
        goto CleanupExit;
    }

    SendMessage(context->DialogHandle, TDM_UPDATE_ELEMENT_TEXT, TDE_MAIN_INSTRUCTION, (LPARAM)L"Waiting for response...");

    if (!PhHttpSocketEndRequest(httpContext))
    {
        context->ErrorCode = GetLastError();
        goto CleanupExit;
    }

    if (!PhHttpSocketQueryHeaderUlong(httpContext, PH_HTTP_QUERY_CONTENT_LENGTH, &contentLength))
    {
        context->ErrorCode = GetLastError();
        goto CleanupExit;
    }

    {
        LARGE_INTEGER allocationSize;
        ULONG bytesDownloaded = 0;
        ULONG downloadedBytes = 0;
        PPH_STRING status;
        IO_STATUS_BLOCK isb;
        BYTE buffer[PAGE_SIZE];

        status = PhFormatString(L"Downloading release %s...", PhGetStringOrEmpty(context->Version));
        SendMessage(context->DialogHandle, TDM_SET_MARQUEE_PROGRESS_BAR, FALSE, 0);
        SendMessage(context->DialogHandle, TDM_UPDATE_ELEMENT_TEXT, TDE_MAIN_INSTRUCTION, (LPARAM)status->Buffer);
        PhDereferenceObject(status);

        // Create the local path string.
        context->SetupFilePath = UpdateParseDownloadFileName(downloadUrlPath);
        if (PhIsNullOrEmptyString(context->SetupFilePath))
            goto CleanupExit;

        allocationSize.QuadPart = contentLength;

        // Create the temporary output file.
        if (!NT_SUCCESS(PhCreateFileWin32Ex(
            &tempFileHandle,
            PhGetString(context->SetupFilePath),
            FILE_GENERIC_WRITE,
            &allocationSize,
            FILE_ATTRIBUTE_NORMAL,
            FILE_SHARE_READ,
            FILE_OVERWRITE_IF,
            FILE_NON_DIRECTORY_FILE | FILE_SYNCHRONOUS_IO_NONALERT,
            NULL
            )))
        {
            goto CleanupExit;
        }

        assert((context->Type == UpdaterTypeNightly) || (context->Type == UpdaterTypeRelease));

        // Initialize hash algorithm.
        if (!(hashContext = UpdaterInitializeHash(context->Type)))
            goto CleanupExit;
        if (!(hashContextLegacy = UpdaterInitializeHash(context->Type + 1)))
            goto CleanupExit;

        // Start the clock.
        PhQuerySystemTime(&timeStart);

        while (TRUE)
        {
            // Download the data.
            if (!PhHttpSocketReadData(httpContext, buffer, PAGE_SIZE, &bytesDownloaded))
            {
                context->ErrorCode = GetLastError();
                goto CleanupExit;
            }

            // If we get zero bytes, the file was uploaded or there was an error
            if (bytesDownloaded == 0)
                break;

            // If the dialog was closed, just cleanup and exit
            if (context->Cancel)
                goto CleanupExit;

            // Update the hash of bytes we downloaded.
            UpdaterUpdateHash(hashContext, buffer, bytesDownloaded);
            UpdaterUpdateHash(hashContextLegacy, buffer, bytesDownloaded);

            // Write the downloaded bytes to disk.
            if (!NT_SUCCESS(NtWriteFile(
                tempFileHandle,
                NULL,
                NULL,
                NULL,
                &isb,
                buffer,
                bytesDownloaded,
                NULL,
                NULL
                )))
            {
                goto CleanupExit;
            }

            downloadedBytes += (DWORD)isb.Information;

            // Check the number of bytes written are the same we downloaded.
            if (bytesDownloaded != isb.Information)
                goto CleanupExit;

            // Query the current time
            PhQuerySystemTime(&timeNow);

            // Calculate the number of ticks
            timeTicks = (timeNow.QuadPart - timeStart.QuadPart) / PH_TICKS_PER_SEC;
            timeBitsPerSecond = downloadedBytes / __max(timeTicks, 1);

            // TODO: Update on timer callback.
            {
                FLOAT percent = ((FLOAT)downloadedBytes / contentLength * 100);
                PH_FORMAT format[9];
                WCHAR string[MAX_PATH];

                // L"Downloaded: %s / %s (%.0f%%)\r\nSpeed: %s/s"
                PhInitFormatS(&format[0], L"Downloaded: ");
                PhInitFormatSize(&format[1], downloadedBytes);
                PhInitFormatS(&format[2], L" / ");
                PhInitFormatSize(&format[3], contentLength);
                PhInitFormatS(&format[4], L" (");
                PhInitFormatF(&format[5], percent, 1);
                PhInitFormatS(&format[6], L"%)\r\nSpeed: ");
                PhInitFormatSize(&format[7], timeBitsPerSecond);
                PhInitFormatS(&format[8], L"/s");

                if (PhFormatToBuffer(format, RTL_NUMBER_OF(format), string, sizeof(string), NULL))
                {
                    SendMessage(context->DialogHandle, TDM_UPDATE_ELEMENT_TEXT, TDE_CONTENT, (LPARAM)string);
                }

                SendMessage(context->DialogHandle, TDM_SET_PROGRESS_BAR_POS, (WPARAM)percent, 0);
            }
        }

        if (UpdaterVerifyHash(hashContext, context->SetupFileHash))
        {
            hashSuccess = TRUE;

            if (UpdaterVerifySignature(hashContext, context->SetupFileSignature))
            {
                signatureSuccess = TRUE;
            }
        }

        if (!signatureSuccess)
        {
            hashSuccess = FALSE;

            if (UpdaterVerifyHash(hashContextLegacy, context->SetupFileHash))
            {
                hashSuccess = TRUE;

                if (UpdaterVerifySignature(hashContextLegacy, context->SetupFileSignature))
                {
                    signatureSuccess = TRUE;
                }
            }
        }

        if (hashSuccess && signatureSuccess)
        {
            downloadSuccess = TRUE;
        }
    }

CleanupExit:
    if (httpContext)
        PhHttpSocketDestroy(httpContext);
    if (hashContextLegacy)
        UpdaterDestroyHash(hashContextLegacy);
    if (hashContext)
        UpdaterDestroyHash(hashContext);
    if (tempFileHandle)
        NtClose(tempFileHandle);
    if (downloadHostPath)
        PhDereferenceObject(downloadHostPath);
    if (downloadUrlPath)
        PhDereferenceObject(downloadUrlPath);

    if (downloadSuccess && hashSuccess && signatureSuccess)
    {
        PostMessage(context->DialogHandle, PH_SHOWINSTALL, 0, 0);
    }
    else
    {
        if (context->SetupFilePath)
        {
            PhDeleteCacheFile(context->SetupFilePath);
        }

        if (signatureSuccess)
            PostMessage(context->DialogHandle, PH_SHOWERROR, TRUE, FALSE);
        else if (hashSuccess)
            PostMessage(context->DialogHandle, PH_SHOWERROR, FALSE, TRUE);
        else
            PostMessage(context->DialogHandle, PH_SHOWERROR, FALSE, FALSE);
    }

    PhDereferenceObject(context);
    return STATUS_SUCCESS;
}

LRESULT CALLBACK TaskDialogSubclassProc(
    _In_ HWND hwndDlg,
    _In_ UINT uMsg,
    _In_ WPARAM wParam,
    _In_ LPARAM lParam
    )
{
    PPH_UPDATER_CONTEXT context;
    WNDPROC oldWndProc;

    if (!(context = PhGetWindowContext(hwndDlg, UCHAR_MAX)))
        return 0;

    oldWndProc = context->DefaultWindowProc;

    switch (uMsg)
    {
    case WM_NCDESTROY:
        {
            context->Cancel = TRUE;

            SetWindowLongPtr(hwndDlg, GWLP_WNDPROC, (LONG_PTR)oldWndProc);
            PhRemoveWindowContext(hwndDlg, UCHAR_MAX);

            PhUnregisterWindowCallback(hwndDlg);
        }
        break;
    case PH_SHOWDIALOG:
        {
            if (IsMinimized(hwndDlg))
                ShowWindow(hwndDlg, SW_RESTORE);
            else
                ShowWindow(hwndDlg, SW_SHOW);

            SetForegroundWindow(hwndDlg);
        }
        break;
    case PH_SHOWLATEST:
        {
            ShowLatestVersionDialog(context);
        }
        break;
    case PH_SHOWNEWEST:
        {
            ShowNewerVersionDialog(context);
        }
        break;
    case PH_SHOWUPDATE:
        {
            ShowAvailableDialog(context);
        }
        break;
    case PH_SHOWINSTALL:
        {
            ShowUpdateInstallDialog(context);
        }
        break;
    case PH_SHOWERROR:
        {
            ShowUpdateFailedDialog(context, (BOOLEAN)wParam, (BOOLEAN)lParam);
        }
        break;
    //case WM_PARENTNOTIFY:
    //    {
    //        if (wParam == WM_CREATE)
    //        {
    //            // uMsg == 49251 for expand/collapse button click
    //            HWND hwndEdit = CreateWindowEx(
    //                WS_EX_CLIENTEDGE,
    //                L"EDIT",
    //                NULL,
    //                WS_CHILD | WS_VISIBLE | WS_VSCROLL | ES_LEFT | ES_MULTILINE | ES_AUTOVSCROLL,
    //                5,
    //                5,
    //                390,
    //                85,
    //                (HWND)lParam, // parent window
    //                0,
    //                NULL,
    //                NULL
    //            );
    //
    //            PhCreateCommonFont(-11, hwndEdit);
    //
    //            // Add text to the window.
    //            SendMessage(hwndEdit, WM_SETTEXT, 0, (LPARAM)L"TEST");
    //        }
    //    }
    //    break;
    //case WM_NCACTIVATE:
    //    {
    //        if (IsWindowVisible(PhMainWindowHandle) && !IsMinimized(PhMainWindowHandle))
    //        {
    //            if (!context->FixedWindowStyles)
    //            {
    //                SetWindowLongPtr(hwndDlg, GWLP_HWNDPARENT, (LONG_PTR)PhMainWindowHandle);
    //                PhSetWindowExStyle(hwndDlg, WS_EX_APPWINDOW, WS_EX_APPWINDOW);
    //                context->FixedWindowStyles = TRUE;
    //            }
    //        }
    //    }
    //    break;
    }

    return CallWindowProc(oldWndProc, hwndDlg, uMsg, wParam, lParam);
}

HRESULT CALLBACK TaskDialogBootstrapCallback(
    _In_ HWND hwndDlg,
    _In_ UINT uMsg,
    _In_ WPARAM wParam,
    _In_ LPARAM lParam,
    _In_ LONG_PTR dwRefData
    )
{
    PPH_UPDATER_CONTEXT context = (PPH_UPDATER_CONTEXT)dwRefData;

    switch (uMsg)
    {
    case TDN_CREATED:
        {
            UpdateDialogHandle = context->DialogHandle = hwndDlg;

            // Center the update window on PH if it's visible else we center on the desktop.
            PhCenterWindow(hwndDlg, PhMainWindowHandle);

            // Create the Taskdialog icons.
            PhSetApplicationWindowIcon(hwndDlg);

            PhRegisterWindowCallback(hwndDlg, PH_PLUGIN_WINDOW_EVENT_TYPE_TOPMOST, NULL);

            // Subclass the Taskdialog.
            context->DefaultWindowProc = (WNDPROC)GetWindowLongPtr(hwndDlg, GWLP_WNDPROC);
            PhSetWindowContext(hwndDlg, UCHAR_MAX, context);
            SetWindowLongPtr(hwndDlg, GWLP_WNDPROC, (LONG_PTR)TaskDialogSubclassProc);

            if (context->StartupCheck)
            {
                ShowAvailableDialog(context);
            }
            else
            {
                if (PhGetIntegerSetting(SETTING_NAME_AUTO_CHECK_PAGE))
                {
                    ShowCheckingForUpdatesDialog(context);
                }
                else
                {
                    ShowCheckForUpdatesDialog(context);
                }
            }
        }
        break;
    }

    return S_OK;
}

NTSTATUS ShowUpdateDialogThread(
    _In_ PVOID Parameter
    )
{
    PH_AUTO_POOL autoPool;
    PPH_UPDATER_CONTEXT context;
    TASKDIALOGCONFIG config = { sizeof(TASKDIALOGCONFIG) };

    if (Parameter)
        context = (PPH_UPDATER_CONTEXT)Parameter;
    else
        context = CreateUpdateContext(FALSE);

    PhInitializeAutoPool(&autoPool);

    // Start TaskDialog bootstrap
    config.dwFlags = TDF_ALLOW_DIALOG_CANCELLATION | TDF_CAN_BE_MINIMIZED;
    config.hInstance = PluginInstance->DllBase;
    config.pszContent = L"Initializing...";
    config.lpCallbackData = (LONG_PTR)context;
    config.pfCallback = TaskDialogBootstrapCallback;
    TaskDialogIndirect(&config, NULL, NULL, NULL);

    PhDereferenceObject(context);
    PhDeleteAutoPool(&autoPool);

    if (UpdateDialogThreadHandle)
    {
        NtClose(UpdateDialogThreadHandle);
        UpdateDialogThreadHandle = NULL;
    }

    PhResetEvent(&InitializedEvent);

    return STATUS_SUCCESS;
}

VOID ShowUpdateDialog(
    _In_opt_ PPH_UPDATER_CONTEXT Context
    )
{
    if (!UpdateDialogThreadHandle)
    {
        if (!NT_SUCCESS(PhCreateThreadEx(&UpdateDialogThreadHandle, ShowUpdateDialogThread, Context)))
        {
            PhShowError(NULL, L"%s", L"Unable to create the window.");
            return;
        }

        PhWaitForEvent(&InitializedEvent, NULL);
    }

    PostMessage(UpdateDialogHandle, PH_SHOWDIALOG, 0, 0);
}

VOID StartInitialCheck(
    VOID
    )
{
    PhQueueItemWorkQueue(PhGetGlobalWorkQueue(), UpdateCheckSilentThread, NULL);
}

VOID ShowStartupUpdateDialog(
    VOID
    )
{
    PH_AUTO_POOL autoPool;
    PPH_UPDATER_CONTEXT context;
    PPH_STRING jsonString;

    PhInitializeAutoPool(&autoPool);

    context = CreateUpdateContext(TRUE);
    jsonString = PH_AUTO(PhGetStringSetting(SETTING_NAME_UPDATE_DATA));

    //
    // TODO configurable nightly/release option.
    //
    context->Type = UpdaterTypeNightly;

    if (jsonString && jsonString->Length)
    {
        PVOID jsonObject;

        if (jsonObject = PhCreateJsonParserEx(jsonString, TRUE))
        {
            context->Version = PhGetJsonValueAsString(jsonObject, "version");
            context->RelDate = PhGetJsonValueAsString(jsonObject, "updated");
            context->CommitHash = PhGetJsonValueAsString(jsonObject, "commit");
            context->SetupFileDownloadUrl = PhGetJsonValueAsString(jsonObject, "setup_url");
            context->SetupFileLength = PhFormatSize(PhGetJsonValueAsUInt64(jsonObject, "setup_length"), 2);
            context->SetupFileHash = PhGetJsonValueAsString(jsonObject, "setup_hash");
            context->SetupFileSignature = PhGetJsonValueAsString(jsonObject, "setup_sig");
            context->CurrentVersion = ParseVersionString(context->CurrentVersionString);
#ifdef FORCE_LATEST_VERSION
            context->LatestVersion = ParseVersionString(context->CurrentVersionString);
#else
            context->LatestVersion = ParseVersionString(context->Version);
#endif
            PhFreeJsonObject(jsonObject);
        }
    }

    if (PhIsNullOrEmptyString(context->Version) &&
        PhIsNullOrEmptyString(context->RelDate) &&
        PhIsNullOrEmptyString(context->SetupFileDownloadUrl) &&
        PhIsNullOrEmptyString(context->SetupFileLength) &&
        PhIsNullOrEmptyString(context->SetupFileHash) &&
        PhIsNullOrEmptyString(context->SetupFileSignature) &&
        PhIsNullOrEmptyString(context->CommitHash))
    {
        return;
    }

    TASKDIALOGCONFIG config = { sizeof(TASKDIALOGCONFIG) };
    config.dwFlags = TDF_ALLOW_DIALOG_CANCELLATION | TDF_CAN_BE_MINIMIZED;
    config.hInstance = PluginInstance->DllBase;
    config.pszContent = L"Initializing...";
    config.lpCallbackData = (LONG_PTR)context;
    config.pfCallback = TaskDialogBootstrapCallback;
    TaskDialogIndirect(&config, NULL, NULL, NULL);

    PhDereferenceObject(context);

    PhDeleteAutoPool(&autoPool);
}
