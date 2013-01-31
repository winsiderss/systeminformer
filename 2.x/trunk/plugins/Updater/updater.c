/*
 * Process Hacker Update Checker -
 *   Update Window
 *
 * Copyright (C) 2011-2012 dmex
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
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with Process Hacker. If not, see <http://www.gnu.org/licenses/>.
 */

#include "updater.h"

// Force update checks to succeed with debug builds
//#define DEBUG_UPDATE

#define PH_UPDATEISERRORED (WM_APP + 101)
#define PH_UPDATEAVAILABLE (WM_APP + 102)
#define PH_UPDATEISCURRENT (WM_APP + 103)
#define PH_UPDATENEWER     (WM_APP + 104)
#define PH_HASHSUCCESS     (WM_APP + 105)
#define PH_HASHFAILURE     (WM_APP + 106)
#define WM_SHOWDIALOG      (WM_APP + 150)

static HANDLE UpdateDialogThreadHandle = NULL;
static HWND UpdateDialogHandle = NULL;
static HICON IconHandle = NULL;
static HFONT FontHandle = NULL;
static PH_EVENT InitializedEvent = PH_EVENT_INIT;

static mxml_type_t QueryXmlDataCallback(
    __in mxml_node_t *node
    )
{
    return MXML_OPAQUE;
}

static VOID SetControlFont(
    __in HWND ControlHandle,
    __in INT ControlID
    )
{
    LOGFONT headerFont;

    memset(&headerFont, 0, sizeof(LOGFONT));

    headerFont.lfHeight = -15;
    headerFont.lfWeight = FW_MEDIUM;
    headerFont.lfQuality = CLEARTYPE_QUALITY | ANTIALIASED_QUALITY;

    // We don't check if Segoe exists, CreateFontIndirect does this for us.
    wcscpy_s(
        headerFont.lfFaceName, 
        _countof(headerFont.lfFaceName), 
        L"Segoe UI"
        );

    // Create the font handle
    FontHandle = CreateFontIndirect(&headerFont);

    // Set the font
    SendMessage(
        GetDlgItem(ControlHandle, ControlID), 
        WM_SETFONT, 
        (WPARAM)FontHandle, 
        FALSE
        );
}

static BOOL ParseVersionString(
    __in PPH_STRING String,
    __in PPH_STRING RevString,
    __out PULONG MajorVersion,
    __out PULONG MinorVersion,
    __out PULONG RevisionVersion
    )
{
    PH_STRINGREF sr, majorPart, minorPart, revisionPart;
    ULONG64 majorInteger = 0, minorInteger = 0, revisionInteger;

    PhInitializeStringRef(&sr, String->Buffer);
    PhInitializeStringRef(&revisionPart, RevString->Buffer);

    if (PhSplitStringRefAtChar(&sr, '.', &majorPart, &minorPart))
    {
        PhStringToInteger64(&majorPart, 10, &majorInteger);
        PhStringToInteger64(&minorPart, 10, &minorInteger);

        PhStringToInteger64(&revisionPart, 10, &revisionInteger);

        *MajorVersion = (ULONG)majorInteger;
        *MinorVersion = (ULONG)minorInteger;
        *RevisionVersion = (ULONG)revisionInteger;

        return TRUE;
    }

    return FALSE;
}

static BOOL ReadRequestString(
    __in HINTERNET Handle,
    __out_z PSTR* Data
    )
{
    BYTE buffer[PAGE_SIZE];
    PSTR data;
    ULONG allocatedLength;
    ULONG dataLength;
    ULONG returnLength;

    allocatedLength = sizeof(buffer);
    data = (PSTR)PhAllocate(allocatedLength);
    dataLength = 0;

    // Zero the buffer
    RtlZeroMemory(buffer, PAGE_SIZE);

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
        RtlCopyMemory(data + dataLength, buffer, returnLength);
        // Zero the returned buffer for the next loop
        //RtlZeroMemory(buffer, PAGE_SIZE);

        dataLength += returnLength;
    }

    if (allocatedLength < dataLength + 1)
    {
        allocatedLength++;
        data = (PSTR)PhReAllocate(data, allocatedLength);
    }

    // Ensure that the buffer is null-terminated.
    data[dataLength] = 0;

    *Data = data;

    return TRUE;
}

static PUPDATER_XML_DATA CreateUpdateContext(
    VOID
    )
{
    PUPDATER_XML_DATA context = (PUPDATER_XML_DATA)PhAllocate(
        sizeof(UPDATER_XML_DATA)
        );

    memset(context, 0, sizeof(UPDATER_XML_DATA));

    // Set the default Updater state        
    context->UpdaterState = Default;

    return context;
}

static VOID FreeUpdateContext(
    __inout PUPDATER_XML_DATA Context
    )
{
    if (!Context)
        return;

    Context->MinorVersion = 0;
    Context->MajorVersion = 0;
    Context->RevisionVersion = 0;
    Context->CurrentMinorVersion = 0;
    Context->CurrentMajorVersion = 0;
    Context->CurrentRevisionVersion = 0;

    PhSwapReference(&Context->Version, NULL);
    PhSwapReference(&Context->RevVersion, NULL);
    PhSwapReference(&Context->RelDate, NULL);
    PhSwapReference(&Context->Size, NULL);
    PhSwapReference(&Context->Hash, NULL);
    PhSwapReference(&Context->ReleaseNotesUrl, NULL);
    PhSwapReference(&Context->SetupFilePath, NULL);
           
    // Set the default Updater state 
    Context->UpdaterState = Default;
    Context->HaveData = FALSE;

    PhFree(Context);
}

static BOOLEAN QueryUpdateData(
    __inout PUPDATER_XML_DATA UpdateData
    )
{
    PSTR xmlStringBuffer = NULL;
    mxml_node_t* xmlNode = NULL;
    BOOLEAN isSuccess = FALSE;

    PPH_STRING phVersion;
    PPH_STRING userAgent;
    HINTERNET sessionHandle = NULL;
    HINTERNET connectionHandle = NULL;
    HINTERNET requestHandle = NULL;

    WINHTTP_CURRENT_USER_IE_PROXY_CONFIG proxyConfig = { 0 };

    // Create a user agent string.
    phVersion = PhGetPhVersion();
    userAgent = PhConcatStrings2(L"PH_", phVersion->Buffer);

    // Get the current Process Hacker version
    PhGetPhVersionNumbers(
        &UpdateData->CurrentMajorVersion, 
        &UpdateData->CurrentMinorVersion, 
        NULL, 
        &UpdateData->CurrentRevisionVersion
        );

    __try
    {
        // Query the current system proxy
        WinHttpGetIEProxyConfigForCurrentUser(&proxyConfig);

        // Open the HTTP session with the system proxy configuration if available
        if (!(sessionHandle = WinHttpOpen(
            userAgent->Buffer,                 
            proxyConfig.lpszProxy != NULL ? WINHTTP_ACCESS_TYPE_NAMED_PROXY : WINHTTP_ACCESS_TYPE_DEFAULT_PROXY,
            proxyConfig.lpszProxy, 
            proxyConfig.lpszProxyBypass, 
            0
            )))
        {
            __leave;
        }

        if (!(connectionHandle = WinHttpConnect(
            sessionHandle, 
            L"processhacker.sourceforge.net",
            INTERNET_DEFAULT_HTTP_PORT,
            0
            )))
        {
            __leave;
        }

        if (!(requestHandle = WinHttpOpenRequest(
            connectionHandle, 
            L"GET", 
            L"/update.php", 
            NULL, 
            WINHTTP_NO_REFERER, 
            WINHTTP_DEFAULT_ACCEPT_TYPES, 
            0 // WINHTTP_FLAG_REFRESH
            )))
        {
            __leave;
        }

        if (!WinHttpSendRequest(
            requestHandle, 
            WINHTTP_NO_ADDITIONAL_HEADERS, 
            0, 
            WINHTTP_NO_REQUEST_DATA, 
            0, 
            WINHTTP_IGNORE_REQUEST_TOTAL_LENGTH,
            0
            ))
        {
            __leave;
        }

        if (!WinHttpReceiveResponse(requestHandle, NULL))
            __leave;

        // Read the resulting xml into our buffer.
        if (!ReadRequestString(requestHandle, &xmlStringBuffer))
            __leave;

        // Check the buffer for any data
        if (xmlStringBuffer == NULL || xmlStringBuffer[0] == 0)
            __leave;

        // Load our XML
        xmlNode = mxmlLoadString(NULL, xmlStringBuffer, QueryXmlDataCallback);

        // Check our XML
        if (xmlNode == NULL || xmlNode->type != MXML_ELEMENT)
            __leave;

        // Find the version node
        UpdateData->Version = PhGetOpaqueXmlNodeText(
            mxmlFindElement(xmlNode->child, xmlNode, "ver", NULL, NULL, MXML_DESCEND)
            );
        if (PhIsNullOrEmptyString(UpdateData->Version))
            __leave;

        // Find the revision node
        UpdateData->RevVersion = PhGetOpaqueXmlNodeText(
            mxmlFindElement(xmlNode->child, xmlNode, "rev", NULL, NULL, MXML_DESCEND)
            );
        if (PhIsNullOrEmptyString(UpdateData->RevVersion))
            __leave;

        // Find the release date node
        UpdateData->RelDate = PhGetOpaqueXmlNodeText(
            mxmlFindElement(xmlNode->child, xmlNode, "reldate", NULL, NULL, MXML_DESCEND)
            );
        if (PhIsNullOrEmptyString(UpdateData->RelDate))
            __leave;

        // Find the size node
        UpdateData->Size = PhGetOpaqueXmlNodeText(
            mxmlFindElement(xmlNode->child, xmlNode, "size", NULL, NULL, MXML_DESCEND)
            );
        if (PhIsNullOrEmptyString(UpdateData->Size))
            __leave;

        //Find the hash node
        UpdateData->Hash = PhGetOpaqueXmlNodeText(
            mxmlFindElement(xmlNode->child, xmlNode, "sha1", NULL, NULL, MXML_DESCEND)
            );
        if (PhIsNullOrEmptyString(UpdateData->Hash))
            __leave;

        // Find the release notes URL
        UpdateData->ReleaseNotesUrl = PhGetOpaqueXmlNodeText(
            mxmlFindElement(xmlNode->child, xmlNode, "relnotes", NULL, NULL, MXML_DESCEND)
            );
        if (PhIsNullOrEmptyString(UpdateData->ReleaseNotesUrl))
            __leave;

        if (!ParseVersionString(
            UpdateData->Version, 
            UpdateData->RevVersion,  
            &UpdateData->MajorVersion, 
            &UpdateData->MinorVersion, 
            &UpdateData->RevisionVersion
            ))
        {
            __leave;
        }

        isSuccess = TRUE;
    }
    __finally
    {
        if (requestHandle)
            WinHttpCloseHandle(requestHandle);

        if (connectionHandle)
            WinHttpCloseHandle(connectionHandle);

        if (sessionHandle)
            WinHttpCloseHandle(sessionHandle);

        if (xmlNode)
            mxmlDelete(xmlNode);

        if (xmlStringBuffer)
            PhFree(xmlStringBuffer);

        if (userAgent)
            PhDereferenceObject(userAgent);

        if (phVersion)
            PhDereferenceObject(phVersion);
    }

    return isSuccess;
}

static NTSTATUS UpdateCheckSilentThread(
    __in PVOID Parameter
    )
{
    if (ConnectionAvailable())
    {
        PUPDATER_XML_DATA context = CreateUpdateContext();

        if (QueryUpdateData(context))
        {
            ULONGLONG currentVersion = 0;
            ULONGLONG latestVersion = 0;

            currentVersion = MAKEDLLVERULL(
                context->CurrentMajorVersion,
                context->CurrentMinorVersion,
                0, 
                context->CurrentRevisionVersion
                );

#ifdef DEBUG_UPDATE
            latestVersion = MAKEDLLVERULL(
                9999,
                9999,
                0, 
                9999
                );
#else
            latestVersion = MAKEDLLVERULL(
                context->MajorVersion,
                context->MinorVersion,
                0, 
                context->RevisionVersion
                );
#endif

            // Compare the current version against the latest available version
            if (currentVersion < latestVersion)
            {
                // We have data we're going to cache and pass into the dialog
                context->HaveData = TRUE;

                // Don't spam the user the second they open PH, delay dialog creation for 3 seconds.
                Sleep(3000);
                
                // Show the dialog asynchronously on a new thread.
                ShowUpdateDialog(context);
            }
        }

        // Check we didn't pass the data to the dialog
        if (!context->HaveData)
        {
            // Free the data
            FreeUpdateContext(context);
        }
    }

    return STATUS_SUCCESS;
}

static NTSTATUS UpdateCheckThread(
    __in PVOID Parameter
    )
{
    if (ConnectionAvailable())
    {
        PUPDATER_XML_DATA context = (PUPDATER_XML_DATA)Parameter;

        // Check if we have cached update data
        if (!context->HaveData)
            context->HaveData = QueryUpdateData(context);

        if (context->HaveData)
        {
            ULONGLONG currentVersion = 0;
            ULONGLONG latestVersion = 0;

            currentVersion = MAKEDLLVERULL(
                context->CurrentMajorVersion,
                context->CurrentMinorVersion,
                0, 
                context->CurrentRevisionVersion
                );

#ifdef DEBUG_UPDATE
            latestVersion = MAKEDLLVERULL(
                9999,
                9999,
                0, 
                9999
                );
#else
            latestVersion = MAKEDLLVERULL(
                context->MajorVersion,
                context->MinorVersion,
                0, 
                context->RevisionVersion
                );
#endif

            if (currentVersion == latestVersion)
            {
                // User is running the latest version
                PostMessage(UpdateDialogHandle, PH_UPDATEISCURRENT, 0, 0);
            }
            else if (currentVersion > latestVersion)
            {
                // User is running a newer version
                PostMessage(UpdateDialogHandle, PH_UPDATENEWER, 0, 0);
            }
            else
            {
                // User is running an older version
                PostMessage(UpdateDialogHandle, PH_UPDATEAVAILABLE, 0, 0);
            }
        }
        else
        {
            // Display error information if the update checked failed
            PostMessage(UpdateDialogHandle, PH_UPDATEISERRORED, 0, 0);
        }
    }
    else
    {
        // Display error information if the update checked failed
        PostMessage(UpdateDialogHandle, PH_UPDATEISERRORED, 0, 0);
    }

    return STATUS_SUCCESS;
}

static NTSTATUS UpdateDownloadThread(
    __in PVOID Parameter
    )
{
    PUPDATER_XML_DATA context;
    PPH_STRING setupTempPath = NULL;
    PPH_STRING phVersion = NULL;
    PPH_STRING userAgent = NULL;
    PPH_STRING downloadUrlPath = NULL;
    HINTERNET sessionHandle = NULL;
    HINTERNET connectionHandle = NULL;
    HINTERNET requestHandle = NULL;
    HANDLE tempFileHandle = NULL;
    BOOLEAN isSuccess = FALSE;

    // Create a user agent string.
    phVersion = PhGetPhVersion();
    userAgent = PhConcatStrings2(L"PH_", phVersion->Buffer);

    context = (PUPDATER_XML_DATA)Parameter;

    __try
    {
        WINHTTP_CURRENT_USER_IE_PROXY_CONFIG proxyConfig = { 0 };
        
        if (context == NULL)
            __leave;

        // create the download path string.
        downloadUrlPath = PhFormatString(
            L"/projects/processhacker/files/processhacker2/processhacker-%lu.%lu-setup.exe/download?use_mirror=autoselect", /* ?use_mirror=waix" */
            context->MajorVersion,
            context->MinorVersion
            );       
        if (PhIsNullOrEmptyString(downloadUrlPath))
            __leave;

        // Allocate the GetTempPath buffer
        setupTempPath = PhCreateStringEx(NULL, GetTempPath(0, NULL) * sizeof(WCHAR));
        if (PhIsNullOrEmptyString(setupTempPath))
            __leave;            
        // Get the temp path
        if (GetTempPath((DWORD)setupTempPath->Length / sizeof(WCHAR), setupTempPath->Buffer) == 0)
            __leave;
        if (PhIsNullOrEmptyString(setupTempPath))
            __leave;

        // Append the tempath to our string: %TEMP%processhacker-%u.%u-setup.exe
        // Example: C:\\Users\\dmex\\AppData\\Temp\\processhacker-2.90-setup.exe
        context->SetupFilePath = PhFormatString(
            L"%sprocesshacker-%lu.%lu-setup.exe",
            setupTempPath->Buffer,
            context->MajorVersion,
            context->MinorVersion
            );

        if (PhIsNullOrEmptyString(context->SetupFilePath))
            __leave;

        //// Create output file
        if (!NT_SUCCESS(PhCreateFileWin32(
            &tempFileHandle,
            context->SetupFilePath->Buffer,
            FILE_GENERIC_READ | FILE_GENERIC_WRITE,
            FILE_ATTRIBUTE_NOT_CONTENT_INDEXED | FILE_ATTRIBUTE_TEMPORARY,
            FILE_SHARE_READ | FILE_SHARE_WRITE | FILE_SHARE_DELETE,
            FILE_OVERWRITE_IF,
            FILE_NON_DIRECTORY_FILE | FILE_SYNCHRONOUS_IO_NONALERT
            )))
        {
            __leave;
        }

        //// Query the current system proxy
        WinHttpGetIEProxyConfigForCurrentUser(&proxyConfig);

        SetDlgItemText(UpdateDialogHandle, IDC_STATUS, L"Initializing...");

        //// Open the HTTP session with the system proxy configuration if available
        if (!(sessionHandle = WinHttpOpen(
            userAgent->Buffer,                 
            proxyConfig.lpszProxy != NULL ? WINHTTP_ACCESS_TYPE_NAMED_PROXY : WINHTTP_ACCESS_TYPE_DEFAULT_PROXY,
            proxyConfig.lpszProxy, 
            proxyConfig.lpszProxyBypass, 
            0
            )))
        {
            __leave;
        }

        SetDlgItemText(UpdateDialogHandle, IDC_STATUS, L"Connecting...");

        if (!(connectionHandle = WinHttpConnect(
            sessionHandle, 
            L"sourceforge.net",
            INTERNET_DEFAULT_HTTP_PORT,
            0
            )))
        {
            __leave;
        }

        if (!(requestHandle = WinHttpOpenRequest(
            connectionHandle, 
            L"GET", 
            downloadUrlPath->Buffer, 
            NULL, 
            WINHTTP_NO_REFERER, 
            WINHTTP_DEFAULT_ACCEPT_TYPES, 
            0 // WINHTTP_FLAG_REFRESH
            )))
        {
            __leave;
        }

        SetDlgItemText(UpdateDialogHandle, IDC_STATUS, L"Sending request...");

        if (!WinHttpSendRequest(
            requestHandle, 
            WINHTTP_NO_ADDITIONAL_HEADERS, 
            0, 
            WINHTTP_NO_REQUEST_DATA, 
            0, 
            WINHTTP_IGNORE_REQUEST_TOTAL_LENGTH,
            0
            ))
        {
            __leave;
        }

        SetDlgItemText(UpdateDialogHandle, IDC_STATUS, L"Waiting for response...");

        if (WinHttpReceiveResponse(requestHandle, NULL))
        {
            BYTE buffer[PAGE_SIZE];
            BYTE hashBuffer[20]; 
            ULONG bytesDownloaded = 0;
            ULONG downloadedBytes = 0;    
            ULONG contentLengthSize = sizeof(ULONG);
            ULONG contentLength = 0;
                       
            PH_HASH_CONTEXT hashContext;
            IO_STATUS_BLOCK isb;
          
            if (!WinHttpQueryHeaders(
                requestHandle, 
                WINHTTP_QUERY_CONTENT_LENGTH | WINHTTP_QUERY_FLAG_NUMBER, 
                WINHTTP_HEADER_NAME_BY_INDEX,
                &contentLength, 
                &contentLengthSize, 
                0
                ))
            {
                __leave;
            }

            // Initialize hash algorithm.
            PhInitializeHash(&hashContext, Sha1HashAlgorithm);

            // Zero the buffer.
            ZeroMemory(buffer, PAGE_SIZE);

            // Download the data.
            while (WinHttpReadData(requestHandle, buffer, PAGE_SIZE, &bytesDownloaded))
            {
                // If we get zero bytes, the file was uploaded or there was an error
                if (bytesDownloaded == 0)
                    break;

                // If the dialog was closed, just cleanup and exit
                if (!UpdateDialogThreadHandle)
                    __leave;

                // Update the hash of bytes we downloaded.
                PhUpdateHash(&hashContext, buffer, bytesDownloaded);

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
                    __leave;
                }

                downloadedBytes += (DWORD)isb.Information;

                // Check the number of bytes written are the same we downloaded.
                if (bytesDownloaded != isb.Information)
                    __leave;

                //Update the GUI progress.
                {
                    int percent = MulDiv(100, downloadedBytes, contentLength);

                    PPH_STRING totalLength = PhFormatSize(contentLength, -1);
                    PPH_STRING totalDownloaded = PhFormatSize(downloadedBytes, -1);

                    PPH_STRING dlLengthString = PhFormatString(
                        L"%s of %s (%d%%)", 
                        totalDownloaded->Buffer, 
                        totalLength->Buffer,
                        percent
                        );
                    
                    // Update the progress bar position
                    PostMessage(GetDlgItem(UpdateDialogHandle, IDC_PROGRESS), PBM_SETPOS, percent, 0);
                    SetWindowText(GetDlgItem(UpdateDialogHandle, IDC_STATUS), dlLengthString->Buffer);

                    PhDereferenceObject(dlLengthString);
                    PhDereferenceObject(totalDownloaded);
                    PhDereferenceObject(totalLength);
                }
            }
             
            // Check if we downloaded the entire file.
            assert(downloadedBytes == contentLength);

            // Compute our hash result.
            if (PhFinalHash(&hashContext, &hashBuffer, 20, NULL))
            {
                // Allocate our hash string, hex the final hash result in our hashBuffer.
                PPH_STRING hexString = PhBufferToHexString(hashBuffer, 20);

                if (PhEqualString(hexString, context->Hash, TRUE))
                {
                    isSuccess = TRUE;
                    // Hash succeeded, set state as ready to install.
                    context->UpdaterState = Install;

                    PostMessage(UpdateDialogHandle, PH_HASHSUCCESS, 0, 0);  
                }
                else
                {
                    // This isn't a success - disable the error page and show PH_HASHFAILURE instead
                    isSuccess = TRUE;
                    // Hash Failed, set state as retry download.
                    context->UpdaterState = Download;

                    PostMessage(UpdateDialogHandle, PH_HASHFAILURE, 0, 0); 
                }

                PhDereferenceObject(hexString);
            }
            else
            {
                // This isn't a success - disable the error page and show PH_HASHFAILURE instead
                isSuccess = TRUE;
                // Hash Failed, set state as retry download.
                context->UpdaterState = Download;

                PostMessage(UpdateDialogHandle, PH_HASHFAILURE, 0, 0); 
            }
        }
    }
    __finally
    {     
        if (tempFileHandle)
            NtClose(tempFileHandle);

        if (requestHandle)
            WinHttpCloseHandle(requestHandle);

        if (connectionHandle)
            WinHttpCloseHandle(connectionHandle);

        if (sessionHandle)
            WinHttpCloseHandle(sessionHandle);

        PhSwapReference(&setupTempPath, NULL);
        PhSwapReference(&phVersion, NULL);
        PhSwapReference(&userAgent, NULL);
        PhSwapReference(&downloadUrlPath, NULL);
    }

    if (!isSuccess)
    {
        // Display error information if the update checked failed
        PostMessage(UpdateDialogHandle, PH_UPDATEISERRORED, 0, 0);
    }

    return STATUS_SUCCESS;
}

static INT_PTR CALLBACK UpdaterWndProc(
    __in HWND hwndDlg,
    __in UINT uMsg,
    __in WPARAM wParam,
    __in LPARAM lParam
    )
{
    PUPDATER_XML_DATA context;

    if (uMsg == WM_INITDIALOG)
    {
        context = (PUPDATER_XML_DATA)lParam;
        SetProp(hwndDlg, L"Context", (HANDLE)context);
    }
    else
    {
        context = (PUPDATER_XML_DATA)GetProp(hwndDlg, L"Context");

        if (uMsg == WM_DESTROY)
            RemoveProp(hwndDlg, L"Context");
    }

    if (!context)
        return FALSE;

    switch (uMsg)
    {
    case WM_INITDIALOG:
        {
            HWND parentWindow = GetParent(hwndDlg);
                        
            SetControlFont(hwndDlg, IDC_MESSAGE);

            // load the Process Hacker icon
            IconHandle = (HICON)LoadImage(
                GetModuleHandle(NULL),
                MAKEINTRESOURCE(PHAPP_IDI_PROCESSHACKER),
                IMAGE_ICON,
                GetSystemMetrics(SM_CXICON),
                GetSystemMetrics(SM_CYICON),
                LR_SHARED
                );

            // Set the window icon
            if (IconHandle)
                SendMessage(hwndDlg, WM_SETICON, ICON_BIG, (LPARAM)IconHandle);
                      
            // Center the update window on PH if it's visible else we center on the desktop
            PhCenterWindow(hwndDlg, (IsWindowVisible(parentWindow) && !IsIconic(parentWindow)) ? parentWindow : NULL);

            // Create the update check thread
            {
                HANDLE updateCheckThread = NULL;

                if (updateCheckThread = PhCreateThread(0, (PUSER_THREAD_START_ROUTINE)UpdateCheckThread, context))
                {
                    // Close the thread handle, we don't use it.
                    NtClose(updateCheckThread);
                }
            }
        }
        break;
    case WM_SHOWDIALOG:
        {
            if (IsIconic(hwndDlg))
                ShowWindow(hwndDlg, SW_RESTORE);
            else
                ShowWindow(hwndDlg, SW_SHOW);

            SetForegroundWindow(hwndDlg);
        }
        break;
    case WM_CTLCOLORBTN:
    case WM_CTLCOLORDLG:
    case WM_CTLCOLORSTATIC:
        {
            HDC hDC = (HDC)wParam;
            HWND hwndChild = (HWND)lParam;

            // Check for our static label and change the color.
            if (GetDlgCtrlID(hwndChild) == IDC_MESSAGE)
            {
                SetTextColor(hDC, RGB(19, 112, 171));
            }

            // Set a transparent background for the control backcolor.
            SetBkMode(hDC, TRANSPARENT);

            // set window background color.
            return (INT_PTR)GetSysColorBrush(COLOR_WINDOW);
        }
    case WM_COMMAND:
        {
            switch (LOWORD(wParam))
            {
            case IDCANCEL:
            case IDOK:
                {
                    PostQuitMessage(0);
                }
                break;
            case IDC_DOWNLOAD:
                {
                    switch (context->UpdaterState)
                    {
                    case Install:
                        {
                            SHELLEXECUTEINFO info = { sizeof(SHELLEXECUTEINFO) };

                            if (PhIsNullOrEmptyString(context->SetupFilePath))
                                break;

                            info.lpFile = context->SetupFilePath->Buffer;
                            info.lpVerb = PhElevated ? NULL : L"runas";
                            info.nShow = SW_SHOW;
                            info.hwnd = hwndDlg;

                            ProcessHacker_PrepareForEarlyShutdown(PhMainWndHandle);

                            if (!ShellExecuteEx(&info))
                            {
                                // Install failed, cancel the shutdown.
                                ProcessHacker_CancelEarlyShutdown(PhMainWndHandle);

                                // Set button text for next action
                                Button_SetText(GetDlgItem(hwndDlg, IDC_DOWNLOAD), L"Retry");
                            }
                            else
                            {
                                ProcessHacker_Destroy(PhMainWndHandle);
                            }
                        }
                        break;
                    case Default:
                    case Download:
                        {
                            if (PhInstalledUsingSetup())
                            {
                                HANDLE downloadThreadHandle = NULL;

                                // Disable the download button
                                Button_Enable(GetDlgItem(hwndDlg, IDC_DOWNLOAD), FALSE);
                                // Reset the progress bar (might be a download retry)
                                SendDlgItemMessage(UpdateDialogHandle, IDC_PROGRESS, PBM_SETPOS, 0, 0);
                                if (WindowsVersion > WINDOWS_XP)
                                    SendDlgItemMessage(UpdateDialogHandle, IDC_PROGRESS, PBM_SETSTATE, PBST_NORMAL, 0);

                                // Start our Downloader thread
                                if (downloadThreadHandle = PhCreateThread(0, (PUSER_THREAD_START_ROUTINE)UpdateDownloadThread, context))
                                {
                                    NtClose(downloadThreadHandle);
                                }
                            }
                            else
                            {
                                // Let the user handle non-setup installation, show the homepage and close this dialog.
                                PhShellExecute(hwndDlg, L"http://processhacker.sourceforge.net/downloads.php", NULL);
                                PostQuitMessage(0);
                            }
                        }
                        break;
                    }
                }
                break;
            }
            break;
        }
        break;
    case PH_UPDATEISERRORED:
        {
            PPH_STRING summaryText = PhCreateString(
                L"Please check for updates again..."
                );

            PPH_STRING versionText = PhFormatString(
                L"An error was encountered while checking for updates."
                );

            SetDlgItemText(hwndDlg, IDC_MESSAGE, summaryText->Buffer);
            SetDlgItemText(hwndDlg, IDC_RELDATE, versionText->Buffer);

            PhDereferenceObject(versionText);
            PhDereferenceObject(summaryText);
        }
        break;
    case PH_UPDATEAVAILABLE:
        {
            PPH_STRING summaryText = PhFormatString(
                L"Process Hacker %u.%u (r%u)",
                context->MajorVersion,
                context->MinorVersion,
                context->RevisionVersion
                );
            PPH_STRING releaseDateText = PhFormatString(
                L"Released: %s",
                context->RelDate->Buffer
                );
            PPH_STRING releaseSizeText = PhFormatString(
                L"Size: %s",
                context->Size->Buffer
                );

            SetDlgItemText(hwndDlg, IDC_MESSAGE, summaryText->Buffer);
            SetDlgItemText(hwndDlg, IDC_RELDATE, releaseDateText->Buffer);
            SetDlgItemText(hwndDlg, IDC_STATUS, releaseSizeText->Buffer);

            // Enable the download button.
            Button_Enable(GetDlgItem(hwndDlg, IDC_DOWNLOAD), TRUE);
            Control_Visible(GetDlgItem(hwndDlg, IDC_PROGRESS), TRUE);
            Control_Visible(GetDlgItem(hwndDlg, IDC_INFOSYSLINK), TRUE);

            PhDereferenceObject(releaseSizeText);
            PhDereferenceObject(releaseDateText);
            PhDereferenceObject(summaryText);
        }
        break;
    case PH_UPDATEISCURRENT:
        {
            PPH_STRING summaryText = PhCreateString(
                L"You're running the latest version."
                );

            PPH_STRING versionText = PhFormatString(
                L"Stable release build: v%u.%u (r%u)",
                context->CurrentMajorVersion,
                context->CurrentMinorVersion,
                context->CurrentRevisionVersion
                );

            SetDlgItemText(hwndDlg, IDC_MESSAGE, summaryText->Buffer);
            SetDlgItemText(hwndDlg, IDC_RELDATE, versionText->Buffer);

            PhDereferenceObject(versionText);
            PhDereferenceObject(summaryText);
        }
        break;
    case PH_UPDATENEWER:
        {
            PPH_STRING summaryText = PhCreateString(
                L"You're running a newer version."
                );

            PPH_STRING versionText = PhFormatString(
                L"SVN release build: v%u.%u (r%u)",
                context->CurrentMajorVersion,
                context->CurrentMinorVersion,
                context->CurrentRevisionVersion
                );

            SetDlgItemText(hwndDlg, IDC_MESSAGE, summaryText->Buffer);
            SetDlgItemText(hwndDlg, IDC_RELDATE, versionText->Buffer);

            PhDereferenceObject(versionText);
            PhDereferenceObject(summaryText);
        }
        break;
    case PH_HASHSUCCESS:
        {
            // Don't change if state hasn't changed
            if (context->UpdaterState != Install)
                break;

            // If PH is not elevated, set the UAC shield for the install button as the setup requires elevation.
            if (!PhElevated)
                SendMessage(GetDlgItem(hwndDlg, IDC_DOWNLOAD), BCM_SETSHIELD, 0, TRUE);

            // Set the download result, don't include hash status since it succeeded.
            SetDlgItemText(hwndDlg, IDC_STATUS, L"Click Install to continue");

            // Set button text for next action
            Button_SetText(GetDlgItem(hwndDlg, IDC_DOWNLOAD), L"Install");
            // Enable the Download/Install button so the user can install the update
            Button_Enable(GetDlgItem(hwndDlg, IDC_DOWNLOAD), TRUE);
        }
        break;
    case PH_HASHFAILURE:
        {
            // Don't change if state hasn't changed
            if (context->UpdaterState != Download)
                break;

            if (WindowsVersion > WINDOWS_XP)
                SendDlgItemMessage(UpdateDialogHandle, IDC_PROGRESS, PBM_SETSTATE, PBST_ERROR, 0);

            SetDlgItemText(UpdateDialogHandle, IDC_STATUS, L"Download complete, SHA1 Hash failed.");

            // Set button text for next action
            Button_SetText(GetDlgItem(UpdateDialogHandle, IDC_DOWNLOAD), L"Retry");
            // Enable the Install button
            Button_Enable(GetDlgItem(UpdateDialogHandle, IDC_DOWNLOAD), TRUE);
            // Hash failed, reset state to downloading so user can redownload the file.
        }
        break;
    case WM_NOTIFY:
        {
            switch (((LPNMHDR)lParam)->code)
            {
            case NM_CLICK: // Mouse
            case NM_RETURN: // Keyboard 
                {
                    // Launch the ReleaseNotes URL (if it exists) with the default browser
                    if (!PhIsNullOrEmptyString(context->ReleaseNotesUrl))
                        PhShellExecute(hwndDlg, context->ReleaseNotesUrl->Buffer, NULL);
                }
                break;
            }
        }
        break;
    }

    return FALSE;
}

static NTSTATUS ShowUpdateDialogThread(
    __in PVOID Parameter
    )
{
    BOOL result;
    MSG message;
    PH_AUTO_POOL autoPool;
    PUPDATER_XML_DATA context;

    if (Parameter != NULL)
        context = (PUPDATER_XML_DATA)Parameter;
    else
        context = CreateUpdateContext();

    PhInitializeAutoPool(&autoPool);

    UpdateDialogHandle = CreateDialogParam(
        (HINSTANCE)PluginInstance->DllBase,
        MAKEINTRESOURCE(IDD_UPDATE),
        PhMainWndHandle,
        UpdaterWndProc,
        (LPARAM)context
        );

    PhSetEvent(&InitializedEvent);

    while (result = GetMessage(&message, NULL, 0, 0))
    {
        if (result == -1)
            break;

        if (!IsDialogMessage(UpdateDialogHandle, &message))
        {
            TranslateMessage(&message);
            DispatchMessage(&message);
        }

        PhDrainAutoPool(&autoPool);
    }

    PhDeleteAutoPool(&autoPool);
    PhResetEvent(&InitializedEvent);

    // Ensure global objects are disposed and reset when window closes.
    FreeUpdateContext(context);

    if (IconHandle)
    {
        DestroyIcon(IconHandle);
        IconHandle = NULL;
    }

    if (FontHandle)
    {
        DeleteObject(FontHandle);
        FontHandle = NULL;
    }

    if (UpdateDialogThreadHandle)
    {
        NtClose(UpdateDialogThreadHandle);
        UpdateDialogThreadHandle = NULL;
    }

    if (UpdateDialogHandle)
    {
        DestroyWindow(UpdateDialogHandle);
        UpdateDialogHandle = NULL;
    }

    return STATUS_SUCCESS;
}

VOID ShowUpdateDialog(
    __in PUPDATER_XML_DATA Context
    )
{
    if (!UpdateDialogThreadHandle)
    {
        if (!(UpdateDialogThreadHandle = PhCreateThread(0, (PUSER_THREAD_START_ROUTINE)ShowUpdateDialogThread, Context)))
        {
            PhShowStatus(PhMainWndHandle, L"Unable to create the updater window.", 0, GetLastError());
            return;
        }

        PhWaitForEvent(&InitializedEvent, NULL);
    }

    SendMessage(UpdateDialogHandle, WM_SHOWDIALOG, 0, 0);
}

VOID StartInitialCheck(
    VOID
    )
{
    HANDLE silentCheckThread = NULL;

    if (silentCheckThread = PhCreateThread(0, (PUSER_THREAD_START_ROUTINE)UpdateCheckSilentThread, NULL))
    {
        // Close the thread handle right away since we don't use it
        NtClose(silentCheckThread);
    }
}