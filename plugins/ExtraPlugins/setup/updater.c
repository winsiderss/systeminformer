/*
 * Process Hacker Plugins -
 *   Update Checker Plugin
 *
 * Copyright (C) 2011-2016 dmex
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

#include "..\main.h"
#include <shlobj.h>

PPH_UPDATER_CONTEXT CreateUpdateContext(
    _In_ PPLUGIN_NODE Node,
    _In_ PLUGIN_ACTION Action
    )
{
    PPH_UPDATER_CONTEXT context;

    context = (PPH_UPDATER_CONTEXT)PhCreateAlloc(sizeof(PH_UPDATER_CONTEXT));
    memset(context, 0, sizeof(PH_UPDATER_CONTEXT));
    
    context->Action = Action;
    context->Node = Node;

    return context;
}

VOID FreeUpdateContext(
    _In_ _Post_invalid_ PPH_UPDATER_CONTEXT Context
    )
{
    //PhClearReference(&Context->Version);
    PhClearReference(&Context->RevVersion);
    //PhClearReference(&Context->RelDate);
    PhClearReference(&Context->Size);
    //PhClearReference(&Context->Signature);
    //PhClearReference(&Context->ReleaseNotesUrl);
    PhClearReference(&Context->SetupFilePath);
    PhClearReference(&Context->FileDownloadUrl);

    PhDereferenceObject(Context);
}

VOID TaskDialogCreateIcons(
    _In_ PPH_UPDATER_CONTEXT Context
    )
{
    // Load the Process Hacker window icon
    Context->IconLargeHandle = (HICON)LoadImage(
        NtCurrentPeb()->ImageBaseAddress,
        MAKEINTRESOURCE(PHAPP_IDI_PROCESSHACKER),
        IMAGE_ICON,
        GetSystemMetrics(SM_CXICON),
        GetSystemMetrics(SM_CYICON),
        LR_SHARED
        );
    Context->IconSmallHandle = (HICON)LoadImage(
        NtCurrentPeb()->ImageBaseAddress,
        MAKEINTRESOURCE(PHAPP_IDI_PROCESSHACKER),
        IMAGE_ICON,
        GetSystemMetrics(SM_CXSMICON),
        GetSystemMetrics(SM_CYSMICON),
        LR_SHARED
        );

    // Set the TaskDialog window icons
    SendMessage(Context->DialogHandle, WM_SETICON, ICON_SMALL, (LPARAM)Context->IconSmallHandle);
    SendMessage(Context->DialogHandle, WM_SETICON, ICON_BIG, (LPARAM)Context->IconLargeHandle);
}

VOID TaskDialogLinkClicked(
    _In_ PPH_UPDATER_CONTEXT Context
    )
{
    //if (!PhIsNullOrEmptyString(Context->ReleaseNotesUrl))
    {
        // Launch the ReleaseNotes URL (if it exists) with the default browser
        //PhShellExecute(Context->DialogHandle, Context->ReleaseNotesUrl->Buffer, NULL);
    }
}

PPH_STRING UpdaterGetOpaqueXmlNodeText(
    _In_ mxml_node_t *xmlNode
    )
{
    if (xmlNode && xmlNode->child && xmlNode->child->type == MXML_OPAQUE && xmlNode->child->value.opaque)
    {
        return PhConvertUtf8ToUtf16(xmlNode->child->value.opaque);
    }

    return PhReferenceEmptyString();
}

BOOLEAN LastUpdateCheckExpired(
    VOID
    )
{
#ifdef FORCE_UPDATE_CHECK
    return TRUE;
#else
    ULONG64 lastUpdateTimeTicks = 0;
    LARGE_INTEGER currentUpdateTimeTicks;
    //PPH_STRING lastUpdateTimeString;

    // Get the last update check time
    //lastUpdateTimeString = PhGetStringSetting(SETTING_NAME_LAST_CHECK);
    //PhStringToInteger64(&lastUpdateTimeString->sr, 0, &lastUpdateTimeTicks);

    // Query the current time
    PhQuerySystemTime(&currentUpdateTimeTicks);

    // Check if the last update check was more than 7 days ago
    if (currentUpdateTimeTicks.QuadPart - lastUpdateTimeTicks >= 7 * PH_TICKS_PER_DAY)
    {
        PPH_STRING currentUpdateTimeString = PhFormatUInt64(currentUpdateTimeTicks.QuadPart, FALSE);

        // Save the current time
       // PhSetStringSetting2(SETTING_NAME_LAST_CHECK, &currentUpdateTimeString->sr);

        // Cleanup
        PhDereferenceObject(currentUpdateTimeString);
       // P//hDereferenceObject(lastUpdateTimeString);
        return TRUE;
    }

    // Cleanup
    //PhDereferenceObject(lastUpdateTimeString);
    return FALSE;
#endif
}

PPH_STRING UpdateVersionString(
    VOID
    )
{
    ULONG majorVersion;
    ULONG minorVersion;
    ULONG revisionVersion;
    PPH_STRING currentVersion = NULL;
    PPH_STRING versionHeader = NULL;

    PhGetPhVersionNumbers(
        &majorVersion,
        &minorVersion,
        NULL,
        &revisionVersion
        );

    currentVersion = PhFormatString(
        L"%lu.%lu.%lu",
        majorVersion,
        minorVersion,
        revisionVersion
        );

    if (currentVersion)
    {
        versionHeader = PhConcatStrings2(L"ProcessHacker-Build: ", currentVersion->Buffer);
        PhDereferenceObject(currentVersion);
    }

    return versionHeader;
}

PPH_STRING UpdateWindowsString(
    VOID
    )
{
    static PH_STRINGREF keyName = PH_STRINGREF_INIT(L"Software\\Microsoft\\Windows NT\\CurrentVersion");

    HANDLE keyHandle = NULL;
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
            buildLabHeader = PhConcatStrings2(L"ProcessHacker-OsBuild: ", buildLabString->Buffer);
            PhDereferenceObject(buildLabString);
        }
        else if (buildLabString = PhQueryRegistryString(keyHandle, L"BuildLab"))
        {
            buildLabHeader = PhConcatStrings2(L"ProcessHacker-OsBuild: ", buildLabString->Buffer);
            PhDereferenceObject(buildLabString);
        }

        NtClose(keyHandle);
    }

    return buildLabHeader;
}

//BOOLEAN ParseVersionString(
//    _Inout_ PPH_UPDATER_CONTEXT Context
//    )
//{
//    PH_STRINGREF sr, majorPart, minorPart, revisionPart;
//    ULONG64 majorInteger = 0, minorInteger = 0, revisionInteger = 0;
//
//    //PhInitializeStringRef(&sr, Context->VersionString->Buffer);
//    PhInitializeStringRef(&revisionPart, Context->RevVersion->Buffer);
//
//    if (PhSplitStringRefAtChar(&sr, '.', &majorPart, &minorPart))
//    {
//        PhStringToInteger64(&majorPart, 10, &majorInteger);
//        PhStringToInteger64(&minorPart, 10, &minorInteger);
//        PhStringToInteger64(&revisionPart, 10, &revisionInteger);
//
//        //Context->MajorVersion = (ULONG)majorInteger;
//        //Context->MinorVersion = (ULONG)minorInteger;
//        //Context->RevisionVersion = (ULONG)revisionInteger;
//
//        return TRUE;
//    }
//
//    return FALSE;
//}

BOOLEAN ReadRequestString(
    _In_ HINTERNET Handle,
    _Out_ _Deref_post_z_cap_(*DataLength) PSTR *Data,
    _Out_ ULONG *DataLength
    )
{
    PSTR data;
    ULONG allocatedLength;
    ULONG dataLength;
    ULONG returnLength;
    BYTE buffer[PAGE_SIZE];

    allocatedLength = sizeof(buffer);
    data = (PSTR)PhAllocate(allocatedLength);
    dataLength = 0;

    // Zero the buffer
    memset(buffer, 0, PAGE_SIZE);

    while (WinHttpReadData(Handle, buffer, PAGE_SIZE, &returnLength))
    {
        if (returnLength == 0)
            break;

        if (allocatedLength < dataLength + returnLength)
        {
            allocatedLength *= 2;
            data = (PSTR)PhReAllocate(data, allocatedLength);
        }

        // Copy the returned buffer into our pointer
        memcpy(data + dataLength, buffer, returnLength);
        // Zero the returned buffer for the next loop
        //memset(buffer, 0, returnLength);

        dataLength += returnLength;
    }

    if (allocatedLength < dataLength + 1)
    {
        allocatedLength++;
        data = (PSTR)PhReAllocate(data, allocatedLength);
    }

    // Ensure that the buffer is null-terminated.
    data[dataLength] = 0;

    *DataLength = dataLength;
    *Data = data;

    return TRUE;
}

NTSTATUS UpdateDownloadThread(
    _In_ PVOID Parameter
    )
{
    BOOLEAN downloadSuccess = FALSE;
    BOOLEAN hashSuccess = FALSE;
    BOOLEAN signatureSuccess = FALSE;
    HANDLE tempFileHandle = NULL;
    HINTERNET httpSessionHandle = NULL;
    HINTERNET httpConnectionHandle = NULL;
    HINTERNET httpRequestHandle = NULL;
    PPH_STRING setupTempPath = NULL;
    PPH_STRING downloadHostPath = NULL;
    PPH_STRING downloadUrlPath = NULL;
    PPH_STRING userAgentString = NULL;
    PPH_STRING fullSetupPath = NULL;
    PPH_STRING randomGuidString = NULL;
    PUPDATER_HASH_CONTEXT hashContext = NULL;
    ULONG indexOfFileName = -1;
    GUID randomGuid;
    URL_COMPONENTS httpUrlComponents = { sizeof(URL_COMPONENTS) };
    WINHTTP_CURRENT_USER_IE_PROXY_CONFIG proxyConfig = { 0 };
    LARGE_INTEGER timeNow;
    LARGE_INTEGER timeStart;
    ULONG64 timeTicks = 0;
    ULONG64 timeBitsPerSecond = 0;
    PPH_UPDATER_CONTEXT context = (PPH_UPDATER_CONTEXT)Parameter;

    context->FileDownloadUrl = PhFormatString(
        L"https://wj32.org/processhacker/plugins/download.php?id=%s&type=64", 
        PhGetStringOrEmpty(context->Node->Id)
        );

    SendMessage(context->DialogHandle, TDM_UPDATE_ELEMENT_TEXT, TDE_MAIN_INSTRUCTION, (LPARAM)L"Initializing download request...");

    // Create a user agent string.
    //userAgentString = PhFormatString(
    //    L"PH_%lu.%lu_%lu",
    //    context->CurrentMajorVersion,
    //    context->CurrentMinorVersion,
    //    context->CurrentRevisionVersion
    //    );
    //if (PhIsNullOrEmptyString(userAgentString))
    //    goto CleanupExit;

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

    // Append the tempath to our string: %TEMP%RandomString\\processhacker-%lu.%lu-setup.exe
    // Example: C:\\Users\\dmex\\AppData\\Temp\\ABCD\\processhacker-2.90-setup.exe
    context->SetupFilePath = PhFormatString(
        L"%s%s\\%s.zip",
        PhGetStringOrEmpty(setupTempPath),
        PhGetStringOrEmpty(randomGuidString),
        PhGetStringOrEmpty(context->Node->InternalName)
        );
    if (PhIsNullOrEmptyString(context->SetupFilePath))
        goto CleanupExit;

    // Create the directory if it does not exist.
    if (fullSetupPath = PhGetFullPath(PhGetString(context->SetupFilePath), &indexOfFileName))
    {
        PPH_STRING directoryPath;

        if (indexOfFileName == -1)
            goto CleanupExit;

        if (directoryPath = PhSubstring(fullSetupPath, 0, indexOfFileName))
        {
            SHCreateDirectoryEx(NULL, PhGetString(directoryPath), NULL);
            PhDereferenceObject(directoryPath);
        }
    }

    // Create output file
    if (!NT_SUCCESS(PhCreateFileWin32(
        &tempFileHandle,
        PhGetStringOrEmpty(context->SetupFilePath),
        FILE_GENERIC_READ | FILE_GENERIC_WRITE,
        FILE_ATTRIBUTE_NOT_CONTENT_INDEXED | FILE_ATTRIBUTE_TEMPORARY,
        FILE_SHARE_READ | FILE_SHARE_WRITE | FILE_SHARE_DELETE,
        FILE_OVERWRITE_IF,
        FILE_NON_DIRECTORY_FILE | FILE_SYNCHRONOUS_IO_NONALERT
        )))
    {
        goto CleanupExit;
    }

    // Set lengths to non-zero enabling these params to be cracked.
    httpUrlComponents.dwSchemeLength = (ULONG)-1;
    httpUrlComponents.dwHostNameLength = (ULONG)-1;
    httpUrlComponents.dwUrlPathLength = (ULONG)-1;

    if (!WinHttpCrackUrl(
        PhGetStringOrEmpty(context->FileDownloadUrl),
        0,
        0,
        &httpUrlComponents
        ))
    {
        goto CleanupExit;
    }

    // Create the Host string.
    downloadHostPath = PhCreateStringEx(httpUrlComponents.lpszHostName, httpUrlComponents.dwHostNameLength * sizeof(WCHAR));
    // Create the Path string.
    downloadUrlPath = PhCreateStringEx(httpUrlComponents.lpszUrlPath, httpUrlComponents.dwUrlPathLength * sizeof(WCHAR));


    SendMessage(context->DialogHandle, TDM_UPDATE_ELEMENT_TEXT, TDE_MAIN_INSTRUCTION, (LPARAM)L"Connecting...");


    // Query the current system proxy
    WinHttpGetIEProxyConfigForCurrentUser(&proxyConfig);

    // Open the HTTP session with the system proxy configuration if available
    if (!(httpSessionHandle = WinHttpOpen(
        PhGetStringOrEmpty(userAgentString),
        proxyConfig.lpszProxy != NULL ? WINHTTP_ACCESS_TYPE_NAMED_PROXY : WINHTTP_ACCESS_TYPE_DEFAULT_PROXY,
        proxyConfig.lpszProxy,
        proxyConfig.lpszProxyBypass,
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
        ULONG keepAlive = WINHTTP_DISABLE_KEEP_ALIVE;
        WinHttpSetOption(httpRequestHandle, WINHTTP_OPTION_DISABLE_FEATURE, &keepAlive, sizeof(ULONG));
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

        status = PhFormatString(L"Downloading %s...", PhGetString(context->Node->Name));

        SendMessage(context->DialogHandle, TDM_SET_MARQUEE_PROGRESS_BAR, FALSE, 0);
        SendMessage(context->DialogHandle, TDM_UPDATE_ELEMENT_TEXT, TDE_MAIN_INSTRUCTION, (LPARAM)PhGetString(status));

        PhDereferenceObject(status);

        // Start the clock.
        PhQuerySystemTime(&timeStart);

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

        // Initialize hash algorithm.
        if (!UpdaterInitializeHash(&hashContext))
            goto CleanupExit;

        // Zero the buffer.
        memset(buffer, 0, PAGE_SIZE);

        // Download the data.
        while (WinHttpReadData(httpRequestHandle, buffer, PAGE_SIZE, &bytesDownloaded))
        {
            // If we get zero bytes, the file was uploaded or there was an error
            if (bytesDownloaded == 0)
                break;

            // If the dialog was closed, just cleanup and exit
            //if (!UpdateDialogThreadHandle)
            //    __leave;

            // Update the hash of bytes we downloaded.
            UpdaterUpdateHash(hashContext, buffer, bytesDownloaded);

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
                PPH_STRING totalLength = PhFormatSize(contentLength, -1);
                PPH_STRING totalDownloaded = PhFormatSize(downloadedBytes, -1);
                PPH_STRING totalSpeed = PhFormatSize(timeBitsPerSecond, -1);

                PPH_STRING statusMessage = PhFormatString(
                    L"Downloaded: %s of %s (%.0f%%)\r\nSpeed: %s/s",
                    totalDownloaded->Buffer,
                    totalLength->Buffer,
                    percent,
                    totalSpeed->Buffer
                    );

                SendMessage(context->DialogHandle, TDM_SET_PROGRESS_BAR_POS, (WPARAM)percent, 0);
                SendMessage(context->DialogHandle, TDM_UPDATE_ELEMENT_TEXT, TDE_CONTENT, (LPARAM)statusMessage->Buffer);

                PhDereferenceObject(statusMessage);
                PhDereferenceObject(totalSpeed);
                PhDereferenceObject(totalLength);
                PhDereferenceObject(totalDownloaded);
            }
        }

        downloadSuccess = TRUE;

        if (UpdaterVerifyHash(hashContext, context->Node->SHA2_64))
        {
            hashSuccess = TRUE;
        }

        if (UpdaterVerifySignature(hashContext, context->Node->HASH_64))
        {
            signatureSuccess = TRUE;
        }
    }

CleanupExit:

    if (hashContext)
        UpdaterDestroyHash(hashContext);

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
    PhClearReference(&downloadHostPath);
    PhClearReference(&downloadUrlPath);
    PhClearReference(&userAgentString);

    if (downloadSuccess && hashSuccess && signatureSuccess)
    {
        if (NT_SUCCESS(SetupExtractBuild(context)))
        {
            PostMessage(context->DialogHandle, PH_UPDATESUCCESS, 0, 0);
        }
        else
        {
            PostMessage(context->DialogHandle, PH_UPDATEISERRORED, 0, 0);
        }
    }
    else
    {
        PostMessage(context->DialogHandle, PH_UPDATEISERRORED, 0, 0);
    }

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
    case WM_APP + 1:
        {
            if (IsIconic(hwndDlg))
                ShowWindow(hwndDlg, SW_RESTORE);
            else
                ShowWindow(hwndDlg, SW_SHOW);

            SetForegroundWindow(hwndDlg);
        }
        break;
    case PH_UPDATEAVAILABLE:
        {
            ShowAvailableDialog(context);
        }
        break;
    case PH_UPDATENEWER:
        {
            ShowUninstallRestartDialog(context);
        }
        break;
    case PH_UPDATESUCCESS:
        {
            ShowInstallRestartDialog(context);
        }
        break;
    case PH_UPDATEFAILURE:
        {
            if ((BOOLEAN)wParam)
                ShowUpdateFailedDialog(context, TRUE, FALSE);
            else if ((BOOLEAN)lParam)
                ShowUpdateFailedDialog(context, FALSE, TRUE);
            else
                ShowUpdateFailedDialog(context, FALSE, FALSE);
        }
        break;
    case PH_UPDATEISERRORED:
        {
            ShowUpdateFailedDialog(context, FALSE, FALSE);
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
    //        if (IsWindowVisible(PhMainWndHandle) && !IsMinimized(PhMainWndHandle))
    //        {
    //            if (!context->FixedWindowStyles)
    //            {
    //                SetWindowLongPtr(hwndDlg, GWLP_HWNDPARENT, (LONG_PTR)PhMainWndHandle);
    //                PhSetWindowExStyle(hwndDlg, WS_EX_APPWINDOW, WS_EX_APPWINDOW);
    //                context->FixedWindowStyles = TRUE;
    //            }
    //        }
    //    }
    //    break;
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
            context->DialogHandle = hwndDlg;
            TaskDialogCreateIcons(context);

            SetWindowSubclass(hwndDlg, TaskDialogSubclassProc, 0, (ULONG_PTR)context);

            switch (context->Action)
            {
            case PLUGIN_ACTION_INSTALL:
                {
                    ShowAvailableDialog(context);
                }
                break;
            case PLUGIN_ACTION_UNINSTALL:
                {
                    ShowPluginUninstallDialog(context);
                }
                break;
            case PLUGIN_ACTION_RESTART:
                {
                    ShowUninstallRestartDialog(context);
                }
                break;
            }
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

    // Start the TaskDialog bootstrap
    TaskDialogIndirect(&config, &result, NULL, NULL);

    return result == IDOK;
}

BOOLEAN ShowUpdateDialog(
    _In_ HWND Parent,
    _In_ PLUGIN_ACTION Action
    )
{
    BOOLEAN result;
    PH_AUTO_POOL autoPool;
    PPH_UPDATER_CONTEXT context;

    context = CreateUpdateContext(NULL, Action);

    PhInitializeAutoPool(&autoPool);

    result = ShowInitialDialog(Parent, context);

    FreeUpdateContext(context);
    PhDeleteAutoPool(&autoPool);

    return result;
}

BOOLEAN StartInitialCheck(
    _In_ HWND Parent,
    _In_ PPLUGIN_NODE Node,
    _In_ PLUGIN_ACTION Action
    )
{
    BOOLEAN result;
    PH_AUTO_POOL autoPool;
    PPH_UPDATER_CONTEXT context;

    context = CreateUpdateContext(Node, Action);

    PhInitializeAutoPool(&autoPool);

    result = ShowInitialDialog(Parent, context);

    FreeUpdateContext(context);
    PhDeleteAutoPool(&autoPool);

    return result;
}