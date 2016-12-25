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

HWND UpdateDialogHandle = NULL;
HANDLE UpdateDialogThreadHandle = NULL;
PH_EVENT InitializedEvent = PH_EVENT_INIT;

VOID FreeUpdateContext(
    _In_ _Post_invalid_ PPH_UPDATER_CONTEXT Context
    )
{
    //PhClearReference(&Context->Version);
   // PhClearReference(&Context->RevVersion);
    //PhClearReference(&Context->RelDate);
   // PhClearReference(&Context->Size);
    //PhClearReference(&Context->Hash);
   // PhClearReference(&Context->Signature);
   // PhClearReference(&Context->ReleaseNotesUrl);
    //PhClearReference(&Context->SetupFilePath);
   // PhClearReference(&Context->SetupFileDownloadUrl);

    if (Context->IconLargeHandle)
        DestroyIcon(Context->IconLargeHandle);

    if (Context->IconSmallHandle)
        DestroyIcon(Context->IconSmallHandle);

    //PhClearReference(&Context);
}

VOID TaskDialogCreateIcons(
    _In_ PPH_UPDATER_CONTEXT Context
    )
{
    Context->IconLargeHandle = PhLoadIcon(
        NtCurrentPeb()->ImageBaseAddress,
        MAKEINTRESOURCE(PHAPP_IDI_PROCESSHACKER),
        PH_LOAD_ICON_SIZE_LARGE,
        GetSystemMetrics(SM_CXICON),
        GetSystemMetrics(SM_CYICON)
        );
    Context->IconSmallHandle = PhLoadIcon(
        NtCurrentPeb()->ImageBaseAddress,
        MAKEINTRESOURCE(PHAPP_IDI_PROCESSHACKER),
        PH_LOAD_ICON_SIZE_LARGE,
        GetSystemMetrics(SM_CXICON),
        GetSystemMetrics(SM_CYICON)
        );

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

PPH_STRING QueryFwLinkUrl(PPH_UPDATER_CONTEXT Context)
{
    PPH_STRING redirectUrl = NULL;
    HINTERNET httpSessionHandle = NULL;
    HINTERNET httpConnectionHandle = NULL;
    HINTERNET httpRequestHandle = NULL;
    WINHTTP_CURRENT_USER_IE_PROXY_CONFIG proxyConfig = { 0 };
    PPH_STRING versionHeader = UpdateVersionString(L"ProcessHacker_Build: ");
    PPH_STRING windowsHeader = UpdateWindowsString();

    WinHttpGetIEProxyConfigForCurrentUser(&proxyConfig);

    if (!(httpSessionHandle = WinHttpOpen(
        NULL,
        proxyConfig.lpszProxy ? WINHTTP_ACCESS_TYPE_NAMED_PROXY : WINHTTP_ACCESS_TYPE_DEFAULT_PROXY,
        proxyConfig.lpszProxy,
        proxyConfig.lpszProxyBypass,
        0
        )))
    {
        goto CleanupExit;
    }

    if (WindowsVersion >= WINDOWS_8_1)
    {
        // Enable GZIP and DEFLATE support on Windows 8.1 and above using undocumented flags.
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

    if (WindowsVersion >= WINDOWS_7)
    {
        ULONG option = WINHTTP_DISABLE_REDIRECTS;
        WinHttpSetOption(httpRequestHandle, WINHTTP_OPTION_DISABLE_FEATURE, &option, sizeof(ULONG));

        option = WINHTTP_DISABLE_KEEP_ALIVE;
        WinHttpSetOption(httpRequestHandle, WINHTTP_OPTION_DISABLE_FEATURE, &option, sizeof(ULONG));
    }

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
    PPH_STRING setupTempPath = NULL;
    PPH_STRING downloadHostPath = NULL;
    PPH_STRING downloadUrlPath = NULL;
    PPH_STRING fullSetupPath = NULL;
    PPH_STRING randomGuidString = NULL;
    ULONG indexOfFileName = -1;
    GUID randomGuid;
    URL_COMPONENTS httpUrlComponents = { sizeof(URL_COMPONENTS) };
    WINHTTP_CURRENT_USER_IE_PROXY_CONFIG proxyConfig = { 0 };
    LARGE_INTEGER timeNow;
    LARGE_INTEGER timeStart;
    ULONG64 timeTicks = 0;
    ULONG64 timeBitsPerSecond = 0;
    PPH_UPDATER_CONTEXT context = (PPH_UPDATER_CONTEXT)Parameter;
    PPH_STRING userAgentString = UpdateVersionString(L"ProcessHacker_");

    SendMessage(context->DialogHandle, TDM_UPDATE_ELEMENT_TEXT, TDE_MAIN_INSTRUCTION, (LPARAM)L"Initializing download request...");

    if (!(fwLinkUrl = QueryFwLinkUrl(context)))
        goto CleanupExit;

    setupTempPath = PhCreateStringEx(NULL, GetTempPath(0, NULL) * sizeof(WCHAR));
    if (GetTempPath((ULONG)setupTempPath->Length / sizeof(WCHAR), setupTempPath->Buffer) == 0)
        goto CleanupExit;
    if (PhIsNullOrEmptyString(setupTempPath))
        goto CleanupExit;

    // Generate random guid for our directory path
    PhGenerateGuid(&randomGuid);

    if (randomGuidString = PhFormatGuid(&randomGuid))
    {
        PhSwapReference(&randomGuidString, PhSubstring(randomGuidString, 1, randomGuidString->Length / sizeof(WCHAR) - 2));
    }

    // Append the tempath to our string: %TEMP%RandomString\\GeoLite2-City.mmdb.gz
    // Example: C:\\Users\\dmex\\AppData\\Temp\\ABCD\\GeoLite2-City.mmdb.gz
    context->SetupFilePath = PhFormatString(
        L"%s\\%s\\GeoLite2-City.mmdb.gz",
        PhGetStringOrEmpty(setupTempPath),
        PhGetStringOrDefault(randomGuidString, L"NetworkTools")
        );

    // Create the directory if it does not exist
    if (fullSetupPath = PhGetFullPath(PhGetString(context->SetupFilePath), &indexOfFileName))
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
    httpUrlComponents.dwSchemeLength = (ULONG)-1;
    httpUrlComponents.dwHostNameLength = (ULONG)-1;
    httpUrlComponents.dwUrlPathLength = (ULONG)-1;

    if (!WinHttpCrackUrl(
        fwLinkUrl->Buffer,
        0,
        0,
        &httpUrlComponents
        ))
    {
        goto CleanupExit;
    }

    // Create the Host string.
    downloadHostPath = PhCreateStringEx(
        httpUrlComponents.lpszHostName,
        httpUrlComponents.dwHostNameLength * sizeof(WCHAR)
        );
    if (PhIsNullOrEmptyString(downloadHostPath))
        goto CleanupExit;

    // Create the Path string.
    downloadUrlPath = PhCreateStringEx(
        httpUrlComponents.lpszUrlPath,
        httpUrlComponents.dwUrlPathLength * sizeof(WCHAR)
        );
    if (PhIsNullOrEmptyString(downloadUrlPath))
        goto CleanupExit;

    // Query the current system proxy
    WinHttpGetIEProxyConfigForCurrentUser(&proxyConfig);

    // Open the HTTP session with the system proxy configuration if available
    if (!(httpSessionHandle = WinHttpOpen(
        PhGetStringOrEmpty(userAgentString),
        proxyConfig.lpszProxy ? WINHTTP_ACCESS_TYPE_NAMED_PROXY : WINHTTP_ACCESS_TYPE_DEFAULT_PROXY,
        proxyConfig.lpszProxy,
        proxyConfig.lpszProxyBypass,
        0
        )))
    {
        goto CleanupExit;
    }

    if (WindowsVersion >= WINDOWS_8_1)
    {
        // Enable GZIP and DEFLATE support on Windows 8.1 and above using undocumented flags.
        ULONG httpFlags = WINHTTP_DECOMPRESSION_FLAG_GZIP | WINHTTP_DECOMPRESSION_FLAG_DEFLATE;

        WinHttpSetOption(httpSessionHandle, WINHTTP_OPTION_DECOMPRESSION, &httpFlags, sizeof(ULONG));
    }

    if (!(httpConnectionHandle = WinHttpConnect(
        httpSessionHandle,
        PhGetStringOrEmpty(downloadHostPath),
        httpUrlComponents.nScheme == INTERNET_SCHEME_HTTP ? INTERNET_DEFAULT_HTTP_PORT : INTERNET_DEFAULT_HTTPS_PORT,
        0
        )))
    {
        goto CleanupExit;
    }

    if (!(httpRequestHandle = WinHttpOpenRequest(
        httpConnectionHandle,
        NULL,
        PhGetStringOrEmpty(downloadUrlPath),
        NULL,
        WINHTTP_NO_REFERER,
        WINHTTP_DEFAULT_ACCEPT_TYPES,
        WINHTTP_FLAG_REFRESH | (httpUrlComponents.nScheme == INTERNET_SCHEME_HTTPS ? WINHTTP_FLAG_SECURE : 0)
        )))
    {
        goto CleanupExit;
    }

    if (WindowsVersion >= WINDOWS_7)
    {
        ULONG option = WINHTTP_DISABLE_KEEP_ALIVE;
        WinHttpSetOption(httpRequestHandle, WINHTTP_OPTION_DISABLE_FEATURE, &option, sizeof(ULONG));
    }

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

        status = PhFormatString(L"Downloading GeoLite2-City.mmdb...");
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

        PPH_STRING path;
        PPH_STRING directory;
        PPH_STRING fullSetupPath;
        PPH_BYTES mmdbGzPath;
        gzFile file;

        directory = PH_AUTO(PhGetApplicationDirectory());
        path = PhConcatStrings(2, PhGetString(directory), L"Plugins\\plugindata\\GeoLite2-City.mmdb");
        mmdbGzPath = PhConvertUtf16ToUtf8(PhGetString(context->SetupFilePath));

        if (RtlDoesFileExists_U(PhGetString(path)))
        {
            if (!NT_SUCCESS(PhDeleteFileWin32(PhGetString(path))))
            {
                goto CleanupExit;
            }
        }

        if (fullSetupPath = PhGetFullPath(PhGetString(path), &indexOfFileName))
        {
            PPH_STRING directoryPath;

            if (directoryPath = PhSubstring(fullSetupPath, 0, indexOfFileName))
            {
                SHCreateDirectoryEx(NULL, directoryPath->Buffer, NULL);
                PhDereferenceObject(directoryPath);
            }
        }

        if (file = gzopen(mmdbGzPath->Buffer, "rb"))
        {
            HANDLE mmdbFileHandle;

            if (NT_SUCCESS(PhCreateFileWin32(
                &mmdbFileHandle,
                PhGetStringOrEmpty(path),
                FILE_GENERIC_READ | FILE_GENERIC_WRITE,
                FILE_ATTRIBUTE_NOT_CONTENT_INDEXED | FILE_ATTRIBUTE_TEMPORARY,
                FILE_SHARE_READ | FILE_SHARE_WRITE | FILE_SHARE_DELETE,
                FILE_OVERWRITE_IF,
                FILE_NON_DIRECTORY_FILE | FILE_SYNCHRONOUS_IO_NONALERT
                )))
            {
                IO_STATUS_BLOCK isb;
                BYTE buffer[PAGE_SIZE];

                while (!gzeof(file))
                {
                    int bytes = gzread(file, buffer, sizeof(buffer));

                    if (bytes == -1)
                        goto CleanupExit;

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
                        goto CleanupExit;
                    }
                }

                success = TRUE;

                NtClose(mmdbFileHandle);
            }

            gzclose(file);
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

    PhClearReference(&randomGuidString);
    PhClearReference(&fullSetupPath);
    PhClearReference(&setupTempPath);
    PhClearReference(&userAgentString);

    if (success)
    {
        if (context->DialogHandle)
            PostMessage(context->DialogHandle, PH_UPDATESUCCESS, 0, 0);
    }
    else
    {
        if (context->DialogHandle)
            PostMessage(context->DialogHandle, PH_UPDATEISERRORED, 0, 0);
    }

    PhDereferenceObject(context);
    return STATUS_SUCCESS;
}

LRESULT CALLBACK TaskDialogSubclassProc(
    _In_ HWND hwndDlg,
    _In_ UINT uMsg,
    _In_ WPARAM wParam,
    _In_ LPARAM lParam,
    _In_ UINT_PTR uIdSubclass,
    _In_ ULONG_PTR dwRefData
    )
{
    PPH_UPDATER_CONTEXT context = (PPH_UPDATER_CONTEXT)dwRefData;

    switch (uMsg)
    {
    case WM_NCDESTROY:
        {
            RemoveWindowSubclass(hwndDlg, TaskDialogSubclassProc, uIdSubclass);
        }
        break;
    case WM_SHOWDIALOG:
        {
            if (IsMinimized(hwndDlg))
                ShowWindow(hwndDlg, SW_RESTORE);
            else
                ShowWindow(hwndDlg, SW_SHOW);

            SetForegroundWindow(hwndDlg);
        }
        break;
    case PH_UPDATESUCCESS:
        {
            ShowInstallRestartDialog(context);
        }
        break;
    case PH_UPDATEFAILURE:
        {
           // if ((BOOLEAN)wParam)
              //  ShowUpdateFailedDialog(context, TRUE, FALSE);
           // else if ((BOOLEAN)lParam)
               // ShowUpdateFailedDialog(context, FALSE, TRUE);
            //else
                //ShowUpdateFailedDialog(context, FALSE, FALSE);
        }
        break;
    case PH_UPDATEISERRORED:
        {
            //ShowUpdateFailedDialog(context, FALSE, FALSE);
        }
        break;
    }

    return DefSubclassProc(hwndDlg, uMsg, wParam, lParam);
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

            // Subclass the Taskdialog
            SetWindowSubclass(hwndDlg, TaskDialogSubclassProc, 0, (ULONG_PTR)context);

            ShowCheckForUpdatesDialog(context);
        }
        break;
    }

    return S_OK;
}

BOOLEAN ShowInitialDialog(
    _In_ HWND Parent,
    _In_ PVOID Context
    )
{   
    INT result = 0;
    TASKDIALOGCONFIG config = { sizeof(TASKDIALOGCONFIG) };
    config.dwFlags = TDF_ALLOW_DIALOG_CANCELLATION | TDF_CAN_BE_MINIMIZED | TDF_POSITION_RELATIVE_TO_WINDOW;
    config.pszContent = L"Initializing...";
    config.lpCallbackData = (LONG_PTR)Context;
    config.pfCallback = TaskDialogBootstrapCallback;
    config.hwndParent = Parent;

    TaskDialogIndirect(&config, &result, NULL, NULL);

    return result == IDOK;
}

VOID ShowUpdateDialog(
    _In_opt_ HWND Parent
    )
{
    PH_AUTO_POOL autoPool;
    PPH_UPDATER_CONTEXT context;

    context = (PPH_UPDATER_CONTEXT)PhCreateAlloc(sizeof(PH_UPDATER_CONTEXT));
    memset(context, 0, sizeof(PH_UPDATER_CONTEXT));

    PhInitializeAutoPool(&autoPool);

    ShowInitialDialog(Parent, context);

    FreeUpdateContext(context);
    PhDeleteAutoPool(&autoPool);
}
