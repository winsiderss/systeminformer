/*
 * Process Hacker Network Tools -
 *   GeoIP database updater
 *
 * Copyright (C) 2016 dmex
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

#include "nettools.h"
#include "zlib\zlib.h"
#include "zlib\gzguts.h"
#include <commonutil.h>
#include <shellapi.h>

HWND UpdateDialogHandle = NULL;
HANDLE UpdateDialogThreadHandle = NULL;
PH_EVENT InitializedEvent = PH_EVENT_INIT;

VOID FreeUpdateContext(
    _In_ _Post_invalid_ PPH_UPDATER_CONTEXT Context
    )
{
    //PhClearReference(&Context->Version);
    //PhClearReference(&Context->RevVersion);
    //PhClearReference(&Context->RelDate);
    //PhClearReference(&Context->Size);
    //PhClearReference(&Context->Hash);
    //PhClearReference(&Context->Signature);
    //PhClearReference(&Context->ReleaseNotesUrl);
    //PhClearReference(&Context->SetupFilePath);
    //PhClearReference(&Context->SetupFileDownloadUrl);
    //PhClearReference(&Context);
}

VOID TaskDialogCreateIcons(
    _In_ PPH_UPDATER_CONTEXT Context
    )
{
    Context->IconSmallHandle = PH_LOAD_SHARED_ICON_SMALL(PhInstanceHandle, MAKEINTRESOURCE(PHAPP_IDI_PROCESSHACKER));
    Context->IconLargeHandle = PH_LOAD_SHARED_ICON_LARGE(PhInstanceHandle, MAKEINTRESOURCE(PHAPP_IDI_PROCESSHACKER));

    SendMessage(Context->DialogHandle, WM_SETICON, ICON_SMALL, (LPARAM)Context->IconSmallHandle);
    SendMessage(Context->DialogHandle, WM_SETICON, ICON_BIG, (LPARAM)Context->IconLargeHandle);
}

VOID TaskDialogLinkClicked(
    _In_ PPH_UPDATER_CONTEXT Context
    )
{
    PhShellExecute(Context->DialogHandle, L"https://www.maxmind.com", NULL);
}

PPH_STRING UpdateVersionString(
    _In_ PWSTR UserAgent
    )
{
    ULONG majorVersion;
    ULONG minorVersion;
    ULONG revisionVersion;
    PPH_STRING currentVersion;
    PPH_STRING versionHeader = NULL;

    PhGetPhVersionNumbers(&majorVersion, &minorVersion, NULL, &revisionVersion);

    if (currentVersion = PhFormatString(L"%lu.%lu.%lu", majorVersion, minorVersion, revisionVersion))
    {
        versionHeader = PhConcatStrings2(UserAgent, currentVersion->Buffer);
        PhDereferenceObject(currentVersion);
    }

    return versionHeader;
}

PPH_STRING UpdateWindowsString(
    VOID
    )
{
    static PH_STRINGREF keyName = PH_STRINGREF_INIT(L"Software\\Microsoft\\Windows NT\\CurrentVersion");

    HANDLE keyHandle;
    PPH_STRING buildLabHeader = NULL;

    if (NT_SUCCESS(PhOpenKey(
        &keyHandle,
        KEY_READ,
        PH_KEY_LOCAL_MACHINE,
        &keyName,
        0
        )))
    {
        PPH_STRING buildLabString;

        if (buildLabString = PhQueryRegistryString(keyHandle, L"BuildLabEx"))
        {
            buildLabHeader = PhConcatStrings2(L"ProcessHacker_OsBuild: ", buildLabString->Buffer);
            PhDereferenceObject(buildLabString);
        }
        else if (buildLabString = PhQueryRegistryString(keyHandle, L"BuildLab"))
        {
            buildLabHeader = PhConcatStrings2(L"ProcessHacker_OsBuild: ", buildLabString->Buffer);
            PhDereferenceObject(buildLabString);
        }

        NtClose(keyHandle);
    }

    return buildLabHeader;
}

PPH_STRING QueryFwLinkUrl(
    _In_ PPH_UPDATER_CONTEXT Context
    )
{
    PPH_STRING redirectUrl = NULL;
    HINTERNET httpSessionHandle = NULL;
    HINTERNET httpConnectionHandle = NULL;
    HINTERNET httpRequestHandle = NULL;
    PPH_STRING versionHeader = UpdateVersionString(L"ProcessHacker_Build: ");
    PPH_STRING windowsHeader = UpdateWindowsString();

    if (!(httpSessionHandle = WinHttpOpen(
        NULL,
        WindowsVersion >= WINDOWS_8_1 ? WINHTTP_ACCESS_TYPE_AUTOMATIC_PROXY : WINHTTP_ACCESS_TYPE_DEFAULT_PROXY,
        WINHTTP_NO_PROXY_NAME,
        WINHTTP_NO_PROXY_BYPASS,
        0
        )))
    {
        goto CleanupExit;
    }

    if (WindowsVersion >= WINDOWS_8_1)
    {
        ULONG httpFlags = WINHTTP_DECOMPRESSION_FLAG_GZIP | WINHTTP_DECOMPRESSION_FLAG_DEFLATE;

        WinHttpSetOption(
            httpSessionHandle,
            WINHTTP_OPTION_DECOMPRESSION,
            &httpFlags,
            sizeof(ULONG)
            );
    }

    if (!(httpConnectionHandle = WinHttpConnect(
        httpSessionHandle,
        L"wj32.org",
        INTERNET_DEFAULT_HTTPS_PORT,
        0
        )))
    {
        goto CleanupExit;
    }

    if (!(httpRequestHandle = WinHttpOpenRequest(
        httpConnectionHandle,
        NULL,
        L"/processhacker/fwlink/maxminddb.php",
        NULL,
        WINHTTP_NO_REFERER,
        WINHTTP_DEFAULT_ACCEPT_TYPES,
        WINHTTP_FLAG_REFRESH | WINHTTP_FLAG_SECURE
        )))
    {
        goto CleanupExit;
    }

    if (versionHeader)
    {
        WinHttpAddRequestHeaders(
            httpRequestHandle,
            versionHeader->Buffer,
            (ULONG)versionHeader->Length / sizeof(WCHAR),
            WINHTTP_ADDREQ_FLAG_ADD
            );
    }

    if (windowsHeader)
    {
        WinHttpAddRequestHeaders(
            httpRequestHandle,
            windowsHeader->Buffer,
            (ULONG)windowsHeader->Length / sizeof(WCHAR),
            WINHTTP_ADDREQ_FLAG_ADD
            );
    }

    WinHttpSetOption(
        httpRequestHandle, 
        WINHTTP_OPTION_DISABLE_FEATURE, 
        &(ULONG){ WINHTTP_DISABLE_REDIRECTS }, 
        sizeof(ULONG)
        );
      
    WinHttpSetOption(
        httpRequestHandle, 
        WINHTTP_OPTION_DISABLE_FEATURE, 
        &(ULONG){ WINHTTP_DISABLE_KEEP_ALIVE }, 
        sizeof(ULONG)
        );

    if (!WinHttpSendRequest(
        httpRequestHandle,
        WINHTTP_NO_ADDITIONAL_HEADERS,
        0,
        WINHTTP_NO_REQUEST_DATA,
        0,
        WINHTTP_IGNORE_REQUEST_TOTAL_LENGTH,
        0
        ))
    {
        goto CleanupExit;
    }

    if (WinHttpReceiveResponse(httpRequestHandle, NULL))
    {
        ULONG redirectLength = 0;

        if (WinHttpQueryHeaders(
            httpRequestHandle,
            WINHTTP_QUERY_LOCATION,
            WINHTTP_HEADER_NAME_BY_INDEX,
            NULL,
            &redirectLength,
            0
            ))
        {
            goto CleanupExit;
        }

        if (redirectLength)
        {
            redirectUrl = PhCreateStringEx(NULL, redirectLength);

            if (!WinHttpQueryHeaders(
                httpRequestHandle,
                WINHTTP_QUERY_LOCATION,
                WINHTTP_HEADER_NAME_BY_INDEX,
                redirectUrl->Buffer,
                &redirectLength,
                0
                ))
            {
                PhClearReference(&redirectUrl);
            }
        }
    }

CleanupExit:

    if (httpRequestHandle)
        WinHttpCloseHandle(httpRequestHandle);

    if (httpConnectionHandle)
        WinHttpCloseHandle(httpConnectionHandle);

    if (httpSessionHandle)
        WinHttpCloseHandle(httpSessionHandle);
    
    if (versionHeader)
        PhDereferenceObject(versionHeader);

    if (windowsHeader)
        PhDereferenceObject(windowsHeader);

    return redirectUrl;
}

NTSTATUS GeoIPUpdateThread(
    _In_ PVOID Parameter
    )
{
    BOOLEAN success = FALSE;
    HANDLE tempFileHandle = NULL;
    HINTERNET httpSessionHandle = NULL;
    HINTERNET httpConnectionHandle = NULL;
    HINTERNET httpRequestHandle = NULL;
    PPH_STRING fwLinkUrl = NULL;
    PPH_STRING downloadHostPath = NULL;
    PPH_STRING downloadUrlPath = NULL;
    URL_COMPONENTS httpParts = { sizeof(URL_COMPONENTS) };
    LARGE_INTEGER timeNow;
    LARGE_INTEGER timeStart;
    ULONG64 timeTicks = 0;
    ULONG64 timeBitsPerSecond = 0;
    PPH_UPDATER_CONTEXT context = (PPH_UPDATER_CONTEXT)Parameter;
    PPH_STRING userAgentString = UpdateVersionString(L"ProcessHacker_");

    SendMessage(context->DialogHandle, TDM_UPDATE_ELEMENT_TEXT, TDE_MAIN_INSTRUCTION, (LPARAM)L"Initializing download request...");

    if (!(fwLinkUrl = QueryFwLinkUrl(context)))
        goto CleanupExit;

    context->SetupFilePath = PhCreateCacheFile(PhaCreateString(L"GeoLite2-Country.mmdb.gz"));
    if (PhIsNullOrEmptyString(context->SetupFilePath))
        goto CleanupExit;

    // Create output file
    if (!NT_SUCCESS(PhCreateFileWin32(
        &tempFileHandle,
        PhGetString(context->SetupFilePath),
        FILE_GENERIC_READ | FILE_GENERIC_WRITE,
        FILE_ATTRIBUTE_NOT_CONTENT_INDEXED | FILE_ATTRIBUTE_TEMPORARY,
        FILE_SHARE_READ | FILE_SHARE_WRITE | FILE_SHARE_DELETE,
        FILE_OVERWRITE_IF,
        FILE_NON_DIRECTORY_FILE | FILE_SYNCHRONOUS_IO_NONALERT
        )))
    {
        goto CleanupExit;
    }

    SendMessage(context->DialogHandle, TDM_UPDATE_ELEMENT_TEXT, TDE_MAIN_INSTRUCTION, (LPARAM)L"Connecting...");

    // Set lengths to non-zero enabling these params to be cracked.
    httpParts.dwSchemeLength = ULONG_MAX;
    httpParts.dwHostNameLength = ULONG_MAX;
    httpParts.dwUrlPathLength = ULONG_MAX;

    if (!WinHttpCrackUrl(
        fwLinkUrl->Buffer,
        0,
        0,
        &httpParts
        ))
    {
        goto CleanupExit;
    }

    // Create the Host string.
    if (PhIsNullOrEmptyString(downloadHostPath = PhCreateStringEx(
        httpParts.lpszHostName,
        httpParts.dwHostNameLength * sizeof(WCHAR)
        )))
    {
        goto CleanupExit;
    }

    // Create the remote path string.
    if (PhIsNullOrEmptyString(downloadUrlPath = PhCreateStringEx(
        httpParts.lpszUrlPath,
        httpParts.dwUrlPathLength * sizeof(WCHAR)
        )))
    {
        goto CleanupExit;
    }

    if (!(httpSessionHandle = WinHttpOpen(
        PhGetString(userAgentString),
        WindowsVersion >= WINDOWS_8_1 ? WINHTTP_ACCESS_TYPE_AUTOMATIC_PROXY : WINHTTP_ACCESS_TYPE_DEFAULT_PROXY,
        WINHTTP_NO_PROXY_NAME,
        WINHTTP_NO_PROXY_BYPASS,
        0
        )))
    {
        goto CleanupExit;
    }

    if (WindowsVersion >= WINDOWS_8_1)
    {
        WinHttpSetOption(
            httpSessionHandle,
            WINHTTP_OPTION_DECOMPRESSION, 
            &(ULONG){ WINHTTP_DECOMPRESSION_FLAG_GZIP | WINHTTP_DECOMPRESSION_FLAG_DEFLATE }, 
            sizeof(ULONG)
            );
    }

    if (!(httpConnectionHandle = WinHttpConnect(
        httpSessionHandle,
        PhGetString(downloadHostPath),
        httpParts.nScheme == INTERNET_SCHEME_HTTP ? INTERNET_DEFAULT_HTTP_PORT : INTERNET_DEFAULT_HTTPS_PORT,
        0
        )))
    {
        goto CleanupExit;
    }

    if (!(httpRequestHandle = WinHttpOpenRequest(
        httpConnectionHandle,
        NULL,
        PhGetString(downloadUrlPath),
        NULL,
        WINHTTP_NO_REFERER,
        WINHTTP_DEFAULT_ACCEPT_TYPES,
        WINHTTP_FLAG_REFRESH | (httpParts.nScheme == INTERNET_SCHEME_HTTPS ? WINHTTP_FLAG_SECURE : 0)
        )))
    {
        goto CleanupExit;
    }

    WinHttpSetOption(
        httpRequestHandle, 
        WINHTTP_OPTION_DISABLE_FEATURE, 
        &(ULONG){ WINHTTP_DISABLE_KEEP_ALIVE }, 
        sizeof(ULONG)
        );

    SendMessage(context->DialogHandle, TDM_UPDATE_ELEMENT_TEXT, TDE_MAIN_INSTRUCTION, (LPARAM)L"Sending download request...");

    if (!WinHttpSendRequest(
        httpRequestHandle,
        WINHTTP_NO_ADDITIONAL_HEADERS,
        0,
        WINHTTP_NO_REQUEST_DATA,
        0,
        WINHTTP_IGNORE_REQUEST_TOTAL_LENGTH,
        0
        ))
    {
        goto CleanupExit;
    }

    SendMessage(context->DialogHandle, TDM_UPDATE_ELEMENT_TEXT, TDE_MAIN_INSTRUCTION, (LPARAM)L"Waiting for response...");

    if (WinHttpReceiveResponse(httpRequestHandle, NULL))
    {
        ULONG bytesDownloaded = 0;
        ULONG downloadedBytes = 0;
        ULONG contentLengthSize = sizeof(ULONG);
        ULONG contentLength = 0;
        PPH_STRING status;
        IO_STATUS_BLOCK isb;
        BYTE buffer[PAGE_SIZE];

        memset(buffer, 0, PAGE_SIZE);

        status = PhFormatString(L"Downloading GeoLite2-Country.mmdb...");
        SendMessage(context->DialogHandle, TDM_SET_MARQUEE_PROGRESS_BAR, FALSE, 0);
        SendMessage(context->DialogHandle, TDM_UPDATE_ELEMENT_TEXT, TDE_MAIN_INSTRUCTION, (LPARAM)status->Buffer);
        PhDereferenceObject(status);

        if (!WinHttpQueryHeaders(
            httpRequestHandle,
            WINHTTP_QUERY_CONTENT_LENGTH | WINHTTP_QUERY_FLAG_NUMBER,
            WINHTTP_HEADER_NAME_BY_INDEX,
            &contentLength,
            &contentLengthSize,
            0
            ))
        {
            goto CleanupExit;
        }

        PhQuerySystemTime(&timeStart);

        while (WinHttpReadData(httpRequestHandle, buffer, PAGE_SIZE, &bytesDownloaded))
        {
            // If we get zero bytes, the file was uploaded or there was an error.
            if (bytesDownloaded == 0)
                break;

            // If the dialog was closed; cleanup and exit.
            if (!context->DialogHandle)
                goto CleanupExit;

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
                PPH_STRING totalLength = PhFormatSize(contentLength, -1);
                PPH_STRING totalDownloaded = PhFormatSize(downloadedBytes, -1);
                PPH_STRING totalSpeed = PhFormatSize(timeBitsPerSecond, -1);

                PPH_STRING statusMessage = PhFormatString(
                    L"Downloaded: %s of %s (%.0f%%)\r\nSpeed: %s/s",
                    PhGetStringOrEmpty(totalDownloaded),
                    PhGetStringOrEmpty(totalLength),
                    percent,
                    PhGetStringOrEmpty(totalSpeed)
                    );

                SendMessage(context->DialogHandle, TDM_UPDATE_ELEMENT_TEXT, TDE_CONTENT, (LPARAM)statusMessage->Buffer);
                SendMessage(context->DialogHandle, TDM_SET_PROGRESS_BAR_POS, (WPARAM)percent, 0);

                PhDereferenceObject(statusMessage);
                PhDereferenceObject(totalSpeed);
                PhDereferenceObject(totalLength);
                PhDereferenceObject(totalDownloaded);
            }
        }

        {
            PPH_STRING dbpath;
            PPH_BYTES mmdbGzPath;
            gzFile gzfile;

            dbpath = PhGetExpandStringSetting(SETTING_NAME_DB_LOCATION);

            if (PhIsNullOrEmptyString(dbpath))
                PhMoveReference(&dbpath, PhGetKnownLocation(CSIDL_APPDATA, L"\\Process Hacker\\GeoLite2-Country.mmdb"));

            PhAutoDereferenceObject(dbpath);
            mmdbGzPath = PH_AUTO(PhConvertUtf16ToUtf8(PhGetString(context->SetupFilePath)));

            if (RtlDoesFileExists_U(PhGetString(dbpath)))
            {
                if (!NT_SUCCESS(PhDeleteFileWin32(PhGetString(dbpath))))
                    goto CleanupExit;
            }

            if (gzfile = gzopen(mmdbGzPath->Buffer, "rb"))
            {
                HANDLE mmdbFileHandle;

                if (NT_SUCCESS(PhCreateFileWin32(
                    &mmdbFileHandle,
                    PhGetStringOrEmpty(dbpath),
                    FILE_GENERIC_READ | FILE_GENERIC_WRITE,
                    FILE_ATTRIBUTE_NOT_CONTENT_INDEXED | FILE_ATTRIBUTE_TEMPORARY,
                    FILE_SHARE_READ | FILE_SHARE_WRITE | FILE_SHARE_DELETE,
                    FILE_OVERWRITE_IF,
                    FILE_NON_DIRECTORY_FILE | FILE_SYNCHRONOUS_IO_NONALERT
                    )))
                {
                    IO_STATUS_BLOCK isb;
                    BYTE buffer[PAGE_SIZE];

                    while (!gzeof(gzfile))
                    {
                        INT bytes = gzread(gzfile, buffer, sizeof(buffer));

                        if (bytes == -1)
                        {
                            NtClose(mmdbFileHandle);
                            goto CleanupExit;
                        }

                        if (!NT_SUCCESS(NtWriteFile(
                            mmdbFileHandle,
                            NULL,
                            NULL,
                            NULL,
                            &isb,
                            buffer,
                            bytes,
                            NULL,
                            NULL
                            )))
                        {
                            NtClose(mmdbFileHandle);
                            goto CleanupExit;
                        }
                    }

                    success = TRUE;

                    NtClose(mmdbFileHandle);
                }

                gzclose(gzfile);
            }
        }
    }

CleanupExit:

    if (tempFileHandle)
        NtClose(tempFileHandle);

    if (httpRequestHandle)
        WinHttpCloseHandle(httpRequestHandle);

    if (httpConnectionHandle)
        WinHttpCloseHandle(httpConnectionHandle);

    if (httpSessionHandle)
        WinHttpCloseHandle(httpSessionHandle);

    if (userAgentString)
        PhDereferenceObject(userAgentString);

    if (context->SetupFilePath)
    {
        PhDeleteCacheFile(context->SetupFilePath);
        PhDereferenceObject(context->SetupFilePath);
    }

    if (context->DialogHandle)
    {
        if (success)
        {
            ShowInstallRestartDialog(context);
        }
        else
        {
            ShowUpdateFailedDialog(context);
        }
    }

    PhDereferenceObject(context);
    return STATUS_SUCCESS;
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
            PhCenterWindow(hwndDlg, (IsWindowVisible(PhMainWndHandle) && !IsMinimized(PhMainWndHandle)) ? PhMainWndHandle : NULL);

            // Create the Taskdialog icons
            TaskDialogCreateIcons(context);

            ShowCheckForUpdatesDialog(context);
        }
        break;
    }

    return S_OK;
}

NTSTATUS GeoIPUpdateDialogThread(
    _In_ PVOID Parameter
    )
{
    PH_AUTO_POOL autoPool;
    PPH_UPDATER_CONTEXT context;
    INT result = 0;
    TASKDIALOGCONFIG config = { sizeof(TASKDIALOGCONFIG) };

    PhInitializeAutoPool(&autoPool);

    context = (PPH_UPDATER_CONTEXT)PhCreateAlloc(sizeof(PH_UPDATER_CONTEXT));
    memset(context, 0, sizeof(PH_UPDATER_CONTEXT));

    config.dwFlags = TDF_ALLOW_DIALOG_CANCELLATION | TDF_CAN_BE_MINIMIZED;
    config.pszContent = L"Initializing...";
    config.lpCallbackData = (LONG_PTR)context;
    config.pfCallback = TaskDialogBootstrapCallback;
    config.hwndParent = Parameter;

    TaskDialogIndirect(&config, &result, NULL, NULL);

    FreeUpdateContext(context);
    PhDeleteAutoPool(&autoPool);

    return STATUS_SUCCESS;

    //SHELLEXECUTEINFO info = { sizeof(SHELLEXECUTEINFO) };
    //
    //info.lpFile = L"ProcessHacker.exe";
    //info.lpParameters = L"-plugin " PLUGIN_NAME L":UpdateGeoIp";
    //info.fMask = SEE_MASK_NOCLOSEPROCESS | SEE_MASK_NOASYNC;
    //info.nShow = SW_SHOW;
    //info.hwnd = Parameter;
    //info.lpVerb = L"runas";
    //
    //ProcessHacker_PrepareForEarlyShutdown(PhMainWndHandle);
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
    //                SW_SHOW,
    //                0,
    //                PH_SHELL_APP_PROPAGATE_PARAMETERS | PH_SHELL_APP_PROPAGATE_PARAMETERS_IGNORE_VISIBILITY,
    //                0,
    //                NULL
    //                );
    //
    //            ProcessHacker_Destroy(PhMainWndHandle);
    //        }
    //    }
    //
    //    NtClose(info.hProcess);
    //}
    //else
    //{
    //    ProcessHacker_CancelEarlyShutdown(PhMainWndHandle);
    //}
}

VOID ShowGeoIPUpdateDialog(
    _In_opt_ HWND Parent
    )
{
    PhCreateThread2(GeoIPUpdateDialogThread, Parent);
}