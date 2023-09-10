/*
 * Copyright (c) 2022 Winsider Seminars & Solutions, Inc.  All rights reserved.
 *
 * This file is part of System Informer.
 *
 * Authors:
 *
 *     dmex    2016-2023
 *
 */

#include "nettools.h"

HWND UpdateDialogHandle = NULL;
HANDLE UpdateDialogThreadHandle = NULL;
PH_EVENT InitializedEvent = PH_EVENT_INIT;
BOOLEAN UpdateDatabaseType = FALSE;

// Note: We're using the built-in tar.exe on Windows 10/11 for extracting the database
// updates since SI doesn't currently ship with a tar library. (dmex)
BOOLEAN GeoLiteCheckUpdatePlatformSupported(
    VOID
    )
{
    BOOLEAN supported = FALSE;
    PPH_STRING systemDirectory;
    PPH_STRING fileName;

    if (systemDirectory = PhGetSystemDirectory())
    {
        if (fileName = PhConcatStringRefZ(&systemDirectory->sr, L"\\tar.exe"))
        {
            supported = PhDoesFileExistWin32(PhGetString(fileName));
            PhDereferenceObject(fileName);
        }

        PhDereferenceObject(systemDirectory);
    }

    return supported;
}

VOID GeoLiteUpdateContextDeleteProcedure(
    _In_ PVOID Object,
    _In_ ULONG Flags
    )
{
    //PNETWORK_GEODB_UPDATE_CONTEXT context = Object;
}

PNETWORK_GEODB_UPDATE_CONTEXT GeoLiteCreateUpdateContext(
    VOID
    )
{
    static PPH_OBJECT_TYPE UpdateContextType = NULL;
    static PH_INITONCE UpdateContextTypeInitOnce = PH_INITONCE_INIT;
    PNETWORK_GEODB_UPDATE_CONTEXT context;

    if (PhBeginInitOnce(&UpdateContextTypeInitOnce))
    {
        UpdateContextType = PhCreateObjectType(L"NetworkToolsUpdateContextObjectType", 0, GeoLiteUpdateContextDeleteProcedure);
        PhEndInitOnce(&UpdateContextTypeInitOnce);
    }

    context = PhCreateObjectZero(sizeof(NETWORK_GEODB_UPDATE_CONTEXT), UpdateContextType);
    context->PortableMode = !!ProcessHacker_IsPortableMode();

    return context;
}

PPH_STRING GeoLiteCreateUserAgentString(
    VOID
    )
{
    PH_FORMAT format[8];
    ULONG majorVersion;
    ULONG minorVersion;
    ULONG buildVersion;
    ULONG revisionVersion;

    PhGetPhVersionNumbers(&majorVersion, &minorVersion, &buildVersion, &revisionVersion);
    PhInitFormatS(&format[0], L"SystemInformer_");
    PhInitFormatU(&format[1], majorVersion);
    PhInitFormatC(&format[2], L'.');
    PhInitFormatU(&format[3], minorVersion);
    PhInitFormatC(&format[4], L'.');
    PhInitFormatU(&format[5], buildVersion);
    PhInitFormatC(&format[6], L'.');
    PhInitFormatU(&format[7], revisionVersion);

    return PhFormat(format, RTL_NUMBER_OF(format), 0);
}

PWSTR GeoLiteCreateDatabaseName(
    _In_ PWSTR Format
    )
{
    if (GeoDbDatabaseType)
        return PH_AUTO_T(PH_STRING, PhFormatString(Format, L"City"))->Buffer;
    return PH_AUTO_T(PH_STRING, PhFormatString(Format, L"Country"))->Buffer;
}

NTSTATUS ExtractUpdateToFile(
    _In_ PPH_STRING WorkingDirectory,
    _In_ PPH_STRING CompressedFileName,
    _In_ PCWSTR NewFileName
    )
{
    NTSTATUS status;
    PPH_STRING commandLine;
    PPH_STRING systemDirectory;
    PH_FORMAT format[6];

    if (!(systemDirectory = PhGetSystemDirectory()))
        return STATUS_UNSUCCESSFUL;

    // tar --extract --file="GeoLite2-Country.tar.gz" --directory="%temp%\\guid" --strip-components=1 */GeoLite2-Country.mmdb

    PhInitFormatSR(&format[0], systemDirectory->sr);
    PhInitFormatS(&format[1], L"\\tar.exe --extract --file=\"");
    PhInitFormatSR(&format[2], CompressedFileName->sr);
    PhInitFormatS(&format[3], L"\" --directory=\"");
    PhInitFormatSR(&format[4], WorkingDirectory->sr);
    PhInitFormatS(&format[5], GeoLiteCreateDatabaseName(L"\" --strip-components=1 */GeoLite2-%s.mmdb"));
    commandLine = PhFormat(format, RTL_NUMBER_OF(format), 0x100);

    status = PhCreateProcessRedirection(
        commandLine,
        NULL,
        NULL
        );

    PhDereferenceObject(commandLine);
    PhDereferenceObject(systemDirectory);

    return status;
}

_Success_(return)
BOOLEAN DownloadUpdateToFile(
    _In_ PNETWORK_GEODB_UPDATE_CONTEXT Context,
    _Out_ PPH_STRING* UpdateFileName
    )
{
    BOOLEAN success = FALSE;
    NTSTATUS status = STATUS_UNSUCCESSFUL;
    HANDLE tempFileHandle = NULL;
    PPH_HTTP_CONTEXT httpContext = NULL;
    PPH_STRING httpRequestString = NULL;
    PPH_STRING httpHeaderFileHash = NULL;
    PPH_STRING httpHeaderFileName = NULL;
    ULONG httpStatus = 0;
    ULONG httpContentLength = 0;
    PH_HASH_CONTEXT hashContext;

    PhInitializeHash(&hashContext, Md5HashAlgorithm);

    {
        PPH_STRING apikeyString = PhGetStringSetting(SETTING_NAME_GEOLITE_API_KEY);

        if (PhIsNullOrEmptyString(apikeyString))
        {
            Context->ErrorCode = ERROR_INVALID_DATA;
            goto CleanupExit;
        }

        httpRequestString = PhFormatString(
            L"/app/geoip_download?edition_id=%s&license_key=%s&suffix=tar.gz",
            GeoLiteCreateDatabaseName(L"GeoLite2-%s"),
            PhGetString(apikeyString)
            );

        PhClearReference(&apikeyString);
    }

    SendMessage(Context->DialogHandle, TDM_UPDATE_ELEMENT_TEXT, TDE_MAIN_INSTRUCTION, (LPARAM)L"Connecting...");

    {
        PPH_STRING userAgentString = GeoLiteCreateUserAgentString();

        if (!PhHttpSocketCreate(&httpContext, PhGetString(userAgentString)))
        {
            PhClearReference(&userAgentString);
            Context->ErrorCode = ERROR_INVALID_DATA;
            goto CleanupExit;
        }

        PhClearReference(&userAgentString);
    }

    if (!PhHttpSocketConnect(httpContext, L"download.maxmind.com", PH_HTTP_DEFAULT_HTTPS_PORT))
    {
        Context->ErrorCode = GetLastError();
        goto CleanupExit;
    }

    if (!PhHttpSocketBeginRequest(httpContext, NULL, PhGetString(httpRequestString), PH_HTTP_FLAG_REFRESH | PH_HTTP_FLAG_SECURE))
    {
        Context->ErrorCode = GetLastError();
        goto CleanupExit;
    }

    SendMessage(Context->DialogHandle, TDM_UPDATE_ELEMENT_TEXT, TDE_MAIN_INSTRUCTION, (LPARAM)L"Sending download request...");

    if (!PhHttpSocketSendRequest(httpContext, NULL, 0))
    {
        Context->ErrorCode = GetLastError();
        goto CleanupExit;
    }

    SendMessage(Context->DialogHandle, TDM_UPDATE_ELEMENT_TEXT, TDE_MAIN_INSTRUCTION, (LPARAM)L"Waiting for response...");

    if (!PhHttpSocketEndRequest(httpContext))
    {
        Context->ErrorCode = GetLastError();
        goto CleanupExit;
    }

    // Check http request status.

    if (!PhHttpSocketQueryHeaderUlong(httpContext, PH_HTTP_QUERY_STATUS_CODE, &httpStatus))
    {
        Context->ErrorCode = GetLastError();
        goto CleanupExit;
    }

    if (httpStatus != PH_HTTP_STATUS_OK)
    {
        switch (httpStatus)
        {
        case 401:
            Context->ErrorCode = ERROR_ACCESS_DENIED;
            break;
        default:
            Context->ErrorCode = ERROR_UNHANDLED_ERROR;
            break;
        }

        goto CleanupExit;
    }

    if (!PhHttpSocketQueryHeaderUlong(httpContext, PH_HTTP_QUERY_CONTENT_LENGTH, &httpContentLength))
    {
        Context->ErrorCode = GetLastError();
        goto CleanupExit;
    }
    if (httpContentLength == 0)
    {
        Context->ErrorCode = ERROR_INVALID_DATA;
        goto CleanupExit;
    }

    // Update status message.

    SendMessage(Context->DialogHandle, TDM_SET_MARQUEE_PROGRESS_BAR, FALSE, 0);
    SendMessage(Context->DialogHandle, TDM_UPDATE_ELEMENT_TEXT, TDE_MAIN_INSTRUCTION, (LPARAM)GeoLiteCreateDatabaseName(L"Downloading GeoLite2-%s..."));

    // Extract the content hash from the request headers.

    httpHeaderFileHash = PhHttpSocketQueryHeaderString(httpContext, L"ETag");

    if (PhIsNullOrEmptyString(httpHeaderFileHash))
    {
        Context->ErrorCode = ERROR_INVALID_DATA;
        goto CleanupExit;
    }

    if (PhStartsWithString2(httpHeaderFileHash, L"\"", TRUE))
        PhSkipStringRef(&httpHeaderFileHash->sr, 1 * sizeof(WCHAR));
    if (PhEndsWithString2(httpHeaderFileHash, L"\"", TRUE))
        httpHeaderFileHash->Length -= 1 * sizeof(WCHAR);

    // Extract the filename from the request headers.

    httpHeaderFileName = PhHttpSocketQueryHeaderString(httpContext, L"content-disposition");

    if (PhIsNullOrEmptyString(httpHeaderFileName))
    {
        Context->ErrorCode = ERROR_INVALID_DATA;
        goto CleanupExit;
    }

    if (!PhStartsWithString2(httpHeaderFileName, L"attachment; filename=", TRUE))
    {
        Context->ErrorCode = ERROR_INVALID_DATA;
        goto CleanupExit;
    }

    PhSkipStringRef(&httpHeaderFileName->sr, 21 * sizeof(WCHAR));

    if (!PhStartsWithString2(httpHeaderFileName, GeoLiteCreateDatabaseName(L"GeoLite2-%s"), TRUE))
    {
        Context->ErrorCode = ERROR_INVALID_DATA;
        goto CleanupExit;
    }

    if (!PhEndsWithString2(httpHeaderFileName, L".tar.gz", TRUE))
    {
        Context->ErrorCode = ERROR_INVALID_DATA;
        goto CleanupExit;
    }

    // Create temporary file in the cache directory.

    PhMoveReference(&httpHeaderFileName, PhCreateCacheFile(Context->PortableMode, httpHeaderFileName, FALSE));

    if (PhIsNullOrEmptyString(httpHeaderFileName))
    {
        Context->ErrorCode = ERROR_INVALID_DATA;
        goto CleanupExit;
    }

    status = PhCreateFileWin32Ex(
        &tempFileHandle,
        PhGetString(httpHeaderFileName),
        FILE_GENERIC_READ | FILE_GENERIC_WRITE,
        &(LARGE_INTEGER){ .QuadPart = httpContentLength },
        FILE_ATTRIBUTE_NORMAL,
        FILE_SHARE_READ,
        FILE_OVERWRITE_IF,
        FILE_NON_DIRECTORY_FILE | FILE_SYNCHRONOUS_IO_NONALERT,
        NULL
        );

    if (!NT_SUCCESS(status))
    {
        Context->ErrorCode = PhNtStatusToDosError(status);
        goto CleanupExit;
    }

    // Start downloading the update into the cache.
    {
        LARGE_INTEGER timeNow;
        LARGE_INTEGER timeStart;
        ULONG64 timeTicks;
        ULONG64 timeBitsPerSecond;
        ULONG bytesDownloaded = 0;
        ULONG bytesTotalDownloaded = 0;
        IO_STATUS_BLOCK isb;
        BYTE buffer[PAGE_SIZE];

        memset(buffer, 0, sizeof(buffer));
        PhQuerySystemTime(&timeStart);

        while (PhHttpSocketReadData(httpContext, buffer, PAGE_SIZE, &bytesDownloaded))
        {
            // If we get zero bytes, the file was downloaded or there was an error.
            if (bytesDownloaded == 0)
                break;

            // If the dialog was closed then cleanup and exit.
            if (!Context->DialogHandle)
            {
                Context->ErrorCode = ERROR_INVALID_DATA;
                goto CleanupExit;
            }

            status = NtWriteFile(
                tempFileHandle,
                NULL,
                NULL,
                NULL,
                &isb,
                buffer,
                bytesDownloaded,
                NULL,
                NULL
                );

            if (!NT_SUCCESS(status))
            {
                Context->ErrorCode = PhNtStatusToDosError(status);
                goto CleanupExit;
            }

            // Check the downloaded was copied to the file.
            if (bytesDownloaded != isb.Information)
            {
                Context->ErrorCode = ERROR_INVALID_DATA;
                goto CleanupExit;
            }

            // Update the hash.
            PhUpdateHash(&hashContext, buffer, bytesDownloaded);

            // Query the current time
            PhQuerySystemTime(&timeNow);

            // Calculate the number of ticks
            bytesTotalDownloaded += (ULONG)isb.Information;
            timeTicks = (timeNow.QuadPart - timeStart.QuadPart) / PH_TICKS_PER_SEC;
            timeBitsPerSecond = bytesTotalDownloaded / __max(timeTicks, 1);

            // Update download status (TODO: Update on timer callback)
            {
                ULONG percent = bytesTotalDownloaded * 100 / httpContentLength;
                PH_FORMAT format[9];
                WCHAR string[MAX_PATH];

                // L"Downloaded: %s of %s (%.0f%%)\r\nSpeed: %s/s"
                PhInitFormatS(&format[0], L"Downloaded: ");
                PhInitFormatSize(&format[1], bytesTotalDownloaded);
                PhInitFormatS(&format[2], L" of ");
                PhInitFormatSize(&format[3], httpContentLength);
                PhInitFormatS(&format[4], L" (");
                PhInitFormatU(&format[5], percent);
                PhInitFormatS(&format[6], L"%)\r\nSpeed: ");
                PhInitFormatSize(&format[7], timeBitsPerSecond);
                PhInitFormatS(&format[8], L"/s");

                if (PhFormatToBuffer(format, RTL_NUMBER_OF(format), string, sizeof(string), NULL))
                {
                    SendMessage(Context->DialogHandle, TDM_UPDATE_ELEMENT_TEXT, TDE_CONTENT, (LPARAM)string);
                }

                SendMessage(Context->DialogHandle, TDM_SET_PROGRESS_BAR_POS, (WPARAM)percent, 0);
            }
        }
    }

    // Check the file hash matches the original ETag hash.
    {
        PPH_STRING finalHashString;
        UCHAR finalHashBuffer[32];

        if (!PhFinalHash(&hashContext, finalHashBuffer, 16, NULL))
        {
            Context->ErrorCode = ERROR_INVALID_DATA;
            goto CleanupExit;
        }

        finalHashString = PhBufferToHexString(finalHashBuffer, 16);

        if (!PhEqualString(finalHashString, httpHeaderFileHash, TRUE))
        {
            Context->ErrorCode = ERROR_SYSTEM_IMAGE_BAD_SIGNATURE;
            goto CleanupExit;
        }

        success = TRUE;

        PhClearReference(&finalHashString);
    }

CleanupExit:

    if (tempFileHandle)
        NtClose(tempFileHandle);
    if (httpContext)
        PhHttpSocketDestroy(httpContext);
    if (httpHeaderFileHash)
        PhDereferenceObject(httpHeaderFileHash);
    if (httpRequestString)
        PhDereferenceObject(httpRequestString);

    if (success)
    {
        *UpdateFileName = httpHeaderFileName;
    }
    else
    {
        if (httpHeaderFileName)
        {
            PhDeleteCacheFile(httpHeaderFileName, FALSE);

            PhDereferenceObject(httpHeaderFileName);
        }
    }

    return success;
}

BOOLEAN MoveUpdateToFile(
    _In_ PNETWORK_GEODB_UPDATE_CONTEXT Context,
    _In_ PPH_STRING UpdateFileName
    )
{
    BOOLEAN success = FALSE;
    NTSTATUS status = STATUS_UNSUCCESSFUL;
    PPH_STRING existingFileName = NULL;

    // Get the current database filename.

    if (GeoDbDatabaseType)
        existingFileName = PhGetApplicationDataFileName(&GeoDbCityFileName, FALSE);
    else
        existingFileName = PhGetApplicationDataFileName(&GeoDbCountryFileName, FALSE);

    if (PhIsNullOrEmptyString(existingFileName))
    {
        Context->ErrorCode = ERROR_INVALID_DATA;
        goto CleanupExit;
    }

    // Backup the current database.

    //backupFileName = PhConcatStringRefZ(&existingFileName->sr, L".backup");
    //if (!NT_SUCCESS(PhMoveFileWin32(PhGetString(existingFileName), PhGetString(backupFileName))))
    //    goto CleanupExit;

    // Delete the current database.

    if (PhDoesFileExistWin32(PhGetString(existingFileName)))
    {
        if (!NT_SUCCESS(status = PhDeleteFileWin32(PhGetString(existingFileName))))
        {
            Context->ErrorCode = PhNtStatusToDosError(status);
            goto CleanupExit;
        }
    }
    else
    {
        if (!NT_SUCCESS(status = PhCreateDirectoryFullPathWin32(&existingFileName->sr)))
        {
            Context->ErrorCode = PhNtStatusToDosError(status);
            goto CleanupExit;
        }
    }

    // Move the update from the cache to the correct location.

    if (!NT_SUCCESS(status = PhMoveFileWin32(PhGetString(UpdateFileName), PhGetString(existingFileName), FALSE)))
    {
        Context->ErrorCode = PhNtStatusToDosError(status);
        goto CleanupExit;
    }

    success = TRUE;

CleanupExit:
    if (existingFileName)
        PhDereferenceObject(existingFileName);

    return success;
}

NTSTATUS GeoLiteUpdateThread(
    _In_ PNETWORK_GEODB_UPDATE_CONTEXT Context
    )
{
    BOOLEAN success = FALSE;
    PH_AUTO_POOL autoPool;
    PPH_STRING compressedFileName = NULL;
    PPH_STRING cacheDirectory = NULL;
    PPH_STRING updateFileName = NULL;

    PhInitializeAutoPool(&autoPool);

    // Download the update into the cache.

    if (!DownloadUpdateToFile(Context, &compressedFileName))
        goto CleanupExit;

    // Extract the update into the cache.

    cacheDirectory = PhGetBaseDirectory(compressedFileName);

    if (!NT_SUCCESS(ExtractUpdateToFile(cacheDirectory, compressedFileName, GeoLiteCreateDatabaseName(L"\\GeoLite2-%s.mmdb"))))
        goto CleanupExit;

    // Check the update file was extracted.

    updateFileName = PhConcatStringRefZ(&cacheDirectory->sr, GeoLiteCreateDatabaseName(L"\\GeoLite2-%s.mmdb"));

    if (!PhDoesFileExistWin32(PhGetString(updateFileName)))
    {
        Context->ErrorCode = ERROR_INVALID_DATA;
        goto CleanupExit;
    }

    // Update the database to the latest version.

    if (!MoveUpdateToFile(Context, updateFileName))
        goto CleanupExit;

    success = TRUE;

CleanupExit:

    if (updateFileName)
        PhDereferenceObject(updateFileName);
    if (cacheDirectory)
        PhDereferenceObject(cacheDirectory);
    if (compressedFileName)
        PhDereferenceObject(compressedFileName);

    if (Context->DialogHandle)
    {
        PostMessage(Context->DialogHandle, success ? PH_SHOWINSTALL : PH_SHOWERROR, 0, 0);
    }

    PhDereferenceObject(Context);
    PhDeleteAutoPool(&autoPool);
    return STATUS_SUCCESS;
}

LRESULT CALLBACK TaskDialogSubclassProc(
    _In_ HWND hwndDlg,
    _In_ UINT uMsg,
    _In_ WPARAM wParam,
    _In_ LPARAM lParam
    )
{
    PNETWORK_GEODB_UPDATE_CONTEXT context;
    WNDPROC oldWndProc;

    if (!(context = PhGetWindowContext(hwndDlg, UCHAR_MAX)))
        return 0;

    oldWndProc = context->DefaultWindowProc;

    switch (uMsg)
    {
    case WM_NCDESTROY:
        {
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
    case PH_SHOWINSTALL:
        {
            ShowDbInstallRestartDialog(context);
        }
        break;
    case PH_SHOWERROR:
        {
            ShowDbUpdateFailedDialog(context);
        }
        break;
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
    PNETWORK_GEODB_UPDATE_CONTEXT context = (PNETWORK_GEODB_UPDATE_CONTEXT)dwRefData;

    switch (uMsg)
    {
    case TDN_CREATED:
        {
            UpdateDialogHandle = context->DialogHandle = hwndDlg;

            // Center the update window on PH if it's visible else we center on the desktop.
            PhCenterWindow(hwndDlg, context->ParentWindowHandle);

            // Create the Taskdialog icons
            PhSetApplicationWindowIcon(hwndDlg);

            PhRegisterWindowCallback(hwndDlg, PH_PLUGIN_WINDOW_EVENT_TYPE_TOPMOST, NULL);

            // Subclass the Taskdialog.
            context->DefaultWindowProc = (WNDPROC)GetWindowLongPtr(hwndDlg, GWLP_WNDPROC);
            PhSetWindowContext(hwndDlg, UCHAR_MAX, context);
            SetWindowLongPtr(hwndDlg, GWLP_WNDPROC, (LONG_PTR)TaskDialogSubclassProc);

            ShowDbCheckForUpdatesDialog(context);
        }
        break;
    }

    return S_OK;
}

NTSTATUS GeoLiteUpdateTaskDialogThread(
    _In_ PVOID Parameter
    )
{
    PH_AUTO_POOL autoPool;
    PNETWORK_GEODB_UPDATE_CONTEXT context;
    TASKDIALOGCONFIG config = { sizeof(TASKDIALOGCONFIG) };

    PhInitializeAutoPool(&autoPool);

    context = GeoLiteCreateUpdateContext();
    context->ParentWindowHandle = Parameter;

    config.dwFlags = TDF_ALLOW_DIALOG_CANCELLATION | TDF_CAN_BE_MINIMIZED;
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

    //SHELLEXECUTEINFO info = { sizeof(SHELLEXECUTEINFO) };
    //
    //info.lpFile = L"ProcessHacker.exe";
    //info.lpParameters = L"-plugin " PLUGIN_NAME L":UpdateGeoIp";
    //info.fMask = SEE_MASK_NOCLOSEPROCESS | SEE_MASK_NOASYNC;
    //info.nShow = SW_SHOWNORMAL;
    //info.hwnd = Parameter;
    //info.lpVerb = L"runas";
    //
    //ProcessHacker_PrepareForEarlyShutdown();
    //
    //if (ShellExecuteEx(&info))
    //{
    //    LARGE_INTEGER timeout;
    //    PROCESS_BASIC_INFORMATION basic;
    //
    //    NtWaitForSingleObject(info.hProcess, FALSE, PhTimeoutFromMilliseconds(&timeout, INFINITE));
    //
    //    if (NT_SUCCESS(PhGetProcessBasicInformation(info.hProcess, &basic)))
    //    {
    //        if (basic.ExitStatus == STATUS_ALREADY_COMPLETE)
    //        {
    //            PhShellProcessHacker(
    //                Parameter,
    //                NULL,
    //                SW_SHOWNORMAL,
    //                PH_SHELL_EXECUTE_NOASYNC,
    //                PH_SHELL_APP_PROPAGATE_PARAMETERS | PH_SHELL_APP_PROPAGATE_PARAMETERS_IGNORE_VISIBILITY,
    //                0,
    //                NULL
    //                );
    //
    //            ProcessHacker_Destroy();
    //        }
    //    }
    //
    //    NtClose(info.hProcess);
    //}
    //else
    //{
    //    ProcessHacker_CancelEarlyShutdown();
    //}
}

HRESULT CALLBACK GeoLiteMissingKeyTaskDialogCallbackProc(
    _In_ HWND hwndDlg,
    _In_ UINT uMsg,
    _In_ WPARAM wParam,
    _In_ LPARAM lParam,
    _In_ LONG_PTR dwRefData
    )
{
    switch (uMsg)
    {
    case TDN_HYPERLINK_CLICKED:
        {
            PWSTR hyperlink = (PWSTR)lParam;

            PhShellExecute(hwndDlg, hyperlink, NULL);
        }
        break;
    }

    return S_OK;
}

VOID ShowGeoLiteUpdateDialog(
    _In_opt_ HWND ParentWindowHandle
    )
{
    if (!GeoLiteCheckUpdatePlatformSupported())
    {
        PhShowError(ParentWindowHandle, L"%s", L"The GeoLite updater doesn't support legacy versions of Windows.");
        return;
    }

    PPH_STRING geolitekey = PhGetStringSetting(SETTING_NAME_GEOLITE_API_KEY);

    if (PhIsNullOrEmptyString(geolitekey))
    {
        PhClearReference(&geolitekey);

        TASKDIALOGCONFIG config = { sizeof(TASKDIALOGCONFIG) };
        config.dwFlags = TDF_ENABLE_HYPERLINKS | TDF_ALLOW_DIALOG_CANCELLATION | TDF_POSITION_RELATIVE_TO_WINDOW;
        config.dwCommonButtons = TDCBF_OK_BUTTON;
        config.pszMainIcon = TD_ERROR_ICON;
        config.hwndParent = ParentWindowHandle;
        config.pszWindowTitle = L"Network Tools - GeoLite Updater";
        config.pszMainInstruction = L"Unable to download GeoLite database updates.";
        config.pszContent =
            L"A license key is required to download GeoLite database updates and there are no keys configured.\n\n"
            L"The license keys are completely free. If you're unsure how to create keys then please review the documentation here: <a href=\"https://support.maxmind.com/hc/en-us/articles/4407111582235-Generate-a-License-Key\">Generate-a-License-Key</a>\n\n"
            L"Once you've created the key you can copy/paste the text into the Options window > NetworkTools settings and System Informer can start downloading GeoLite database updates.\n\n"
            L"Special thanks to MaxMind (<a href=\"http://www.maxmind.com\">http://www.maxmind.com</a>) for continuing free GeoLite services <3";

        config.pfCallback = GeoLiteMissingKeyTaskDialogCallbackProc;
        config.cxWidth = 200;

        TaskDialogIndirect(&config, NULL, NULL, NULL);
    }
    else
    {
        PhClearReference(&geolitekey);

        if (!UpdateDialogThreadHandle)
        {
            if (!NT_SUCCESS(PhCreateThreadEx(&UpdateDialogThreadHandle, GeoLiteUpdateTaskDialogThread, ParentWindowHandle)))
            {
                PhShowError(ParentWindowHandle, L"%s", L"Unable to create the window.");
                return;
            }

            PhWaitForEvent(&InitializedEvent, NULL);
        }

        PostMessage(UpdateDialogHandle, PH_SHOWDIALOG, 0, 0);
    }
}
