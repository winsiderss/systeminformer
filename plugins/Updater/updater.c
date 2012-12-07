/*
 * Process Hacker Update Checker -
 *   Update Window
 *
 * Copyright (C) 2011-2012 wj32
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

#define PH_UPDATEISERRORED (WM_APP + 101)
#define PH_UPDATEAVAILABLE (WM_APP + 102)
#define PH_UPDATEISCURRENT (WM_APP + 103)
#define PH_UPDATENEWER     (WM_APP + 104)

static HANDLE UpdateDialogThreadHandle = NULL;
static HWND UpdateDialogHandle = NULL;
static HICON IconHandle = NULL;
static HFONT FontHandle = NULL;
static PH_EVENT InitializedEvent = PH_EVENT_INIT;
static PH_UPDATER_STATE PhUpdaterState = Default;
//static PPH_STRING SetupFilePath = NULL;


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

static VOID FreeUpdateData(
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

    if (Context->Version)
    {
        PhDereferenceObject(Context->Version);
        Context->Version = NULL;
    }

    if (Context->RevVersion)
    {
        PhDereferenceObject(Context->RevVersion);
        Context->RevVersion = NULL;
    }
    
    if (Context->RelDate)
    {
        PhDereferenceObject(Context->RelDate);
        Context->RelDate = NULL;
    }
    
    if (Context->Size)
    {
        PhDereferenceObject(Context->Size);
        Context->Size = NULL;
    }
    
    if (Context->Hash)
    {
        PhDereferenceObject(Context->Hash);
        Context->Hash = NULL;
    }

    if (Context->ReleaseNotesUrl)
    {
        PhDereferenceObject(Context->ReleaseNotesUrl);
        Context->ReleaseNotesUrl = NULL;
    }
   
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
        if (xmlStringBuffer == NULL || xmlStringBuffer[0] == 0 || strlen(xmlStringBuffer) == 0)
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
        PUPDATER_XML_DATA context = (PUPDATER_XML_DATA)PhAllocate(
            sizeof(UPDATER_XML_DATA)
            );

        memset(context, 0, sizeof(UPDATER_XML_DATA));

        if (QueryUpdateData(context))
        {
            ULONGLONG currentVersion = 10;
            ULONGLONG latestVersion = 0;

            currentVersion = MAKEDLLVERULL(
                context->CurrentMajorVersion,
                context->CurrentMinorVersion,
                0, 
                context->CurrentRevisionVersion
                );
            latestVersion = MAKEDLLVERULL(
                2,//updateData->MajorVersion,
                29,//updateData->MinorVersion,
                0, 
                5122//updateData->RevisionVersion
                );

            context->HaveData = TRUE;

            // Compare the current version against the latest available version
            if (currentVersion < latestVersion)
            {
                // Don't spam the user the second they open PH, delay dialog creation for 3 seconds.
                Sleep(3 * 1000);

                // Show the dialog asynchronously on a new thread.
                ShowUpdateDialog(context);
            }
            else
            {
                FreeUpdateData(context);
            }
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

        // Check if we already have the update data
        if (!context->HaveData)
            context->HaveData = QueryUpdateData(context);

        if (context->HaveData)
        {
            ULONGLONG currentVersion;
            ULONGLONG latestVersion;

            currentVersion = MAKEDLLVERULL(
                context->CurrentMajorVersion, // 2
                context->CurrentMinorVersion, // 29
                0, 
                context->CurrentRevisionVersion // 5121
                );
            latestVersion = MAKEDLLVERULL(
                2,//UpdateData->MajorVersion, // 2
                29,//UpdateData->MinorVersion, // 28
                0, 
                5122//UpdateData->RevisionVersion // 5073
                );

            if (currentVersion == latestVersion)
            {
                // User is running the latest version
                PostMessage(UpdateDialogHandle, PH_UPDATEISCURRENT, 0L, 0L);
            }
            else if (currentVersion > latestVersion)
            {
                // User is running a newer version
                PostMessage(UpdateDialogHandle, PH_UPDATENEWER, 0L, 0L);
            }
            else
            {
                // User is running an older version
                PostMessage(UpdateDialogHandle, PH_UPDATEAVAILABLE, 0L, 0L);
            }
        }
        else
        {
            // Display error information if the update checked failed
            PostMessage(UpdateDialogHandle, PH_UPDATEISERRORED, 0L, 0L);
        }
    }
    else
    {
        // Display error information if the update checked failed
        PostMessage(UpdateDialogHandle, PH_UPDATEISERRORED, 0L, 0L);
    }

    return STATUS_SUCCESS;
}

static NTSTATUS UpdateDownloadThread(
    __in PVOID Parameter
    )
{
    //PPH_STRING downloadUrlPath = NULL;
    //HANDLE tempFileHandle = NULL;

    PPH_STRING tempPath;
    //PPH_STRING SetupFilePath;
    //BOOLEAN isSuccess = FALSE;

    //HINTERNET sessionHandle = NULL;
    //HINTERNET connectionHandle = NULL;
    //HINTERNET requestHandle = NULL;

    //WINHTTP_CURRENT_USER_IE_PROXY_CONFIG proxyConfig = { 0 };

    // Create a user agent string.
    //PPH_STRING phVersion = PhGetPhVersion();
    //PPH_STRING userAgent = PhConcatStrings2(L"PH_", phVersion->Buffer);

    PUPDATER_XML_DATA updateData = (PUPDATER_XML_DATA)Parameter;

    // create the download path string.
    //downloadUrlPath = PhFormatString(
    //    L"/projects/processhacker/files/processhacker2/processhacker-%lu.%lu-setup.exe/download?use_mirror=autoselect", /* ?use_mirror=waix" */
    //    updateData->MajorVersion,
    //    updateData->MinorVersion
    //    );

    __try
    {
        // Allocate the GetTempPath buffer
        if (!(tempPath = PhCreateStringEx(NULL, GetTempPath(0, NULL) * sizeof(WCHAR))))
            __leave;

        // Get the temp path
        if (GetTempPath((DWORD)tempPath->Length / sizeof(WCHAR), tempPath->Buffer) == 0)
            __leave;

        // Append the tempath to our string: %TEMP%processhacker-%u.%u-setup.exe
        // Example: C:\\Users\\dmex\\AppData\\Temp\\processhacker-2.10-setup.exe
        //SetupFilePath = PhFormatString(
        //    L"%sprocesshacker-%lu.%lu-setup.exe",
        //    tempPath->Buffer,
        //    updateData->MajorVersion,
        //    updateData->MinorVersion
        //    );

        //// Create output file
        //if (!NT_SUCCESS(PhCreateFileWin32(
        //    &tempFileHandle,
        //    SetupFilePath->Buffer,
        //    FILE_GENERIC_READ | FILE_GENERIC_WRITE,
        //    FILE_ATTRIBUTE_NOT_CONTENT_INDEXED | FILE_ATTRIBUTE_TEMPORARY,
        //    FILE_SHARE_READ | FILE_SHARE_WRITE | FILE_SHARE_DELETE,
        //    FILE_OVERWRITE_IF,
        //    FILE_NON_DIRECTORY_FILE | FILE_SYNCHRONOUS_IO_NONALERT
        //    )))
        //{
        //    __leave;
        //}

        //// Query the current system proxy
        //WinHttpGetIEProxyConfigForCurrentUser(&proxyConfig);

        //// Open the HTTP session with the system proxy configuration if available
        //if (!(sessionHandle = WinHttpOpen(
        //    userAgent->Buffer,                 
        //    proxyConfig.lpszProxy != NULL ? WINHTTP_ACCESS_TYPE_NAMED_PROXY : WINHTTP_ACCESS_TYPE_DEFAULT_PROXY,
        //    proxyConfig.lpszProxy, 
        //    proxyConfig.lpszProxyBypass, 
        //    0
        //    )))
        //{
        //    __leave;
        //}

        //if (!(connectionHandle = WinHttpConnect(
        //    sessionHandle, 
        //    L"sourceforge.net",
        //    INTERNET_DEFAULT_HTTP_PORT,
        //    0
        //    )))
        //{
        //    __leave;
        //}

        //if (!(requestHandle = WinHttpOpenRequest(
        //    connectionHandle, 
        //    L"GET", 
        //    downloadUrlPath->Buffer, 
        //    NULL, 
        //    WINHTTP_NO_REFERER, 
        //    WINHTTP_DEFAULT_ACCEPT_TYPES, 
        //    0 // WINHTTP_FLAG_REFRESH
        //    )))
        //{
        //    __leave;
        //}

        ////SetDlgItemText(hwndDlg, IDC_STATUS, L"Connecting");

        //if (!WinHttpSendRequest(
        //    requestHandle, 
        //    WINHTTP_NO_ADDITIONAL_HEADERS, 
        //    0, 
        //    WINHTTP_NO_REQUEST_DATA, 
        //    0, 
        //    WINHTTP_IGNORE_REQUEST_TOTAL_LENGTH,
        //    0
        //    ))
        //{
        //    __leave;
        //}

        //if (!WinHttpReceiveResponse(requestHandle, NULL))
        //    __leave;

        //{
        //    BYTE hashBuffer[20]; 
        //    PH_HASH_CONTEXT hashContext;
        //    BYTE buffer[PAGE_SIZE];
        //    DWORD bytesDownloaded = 0, startTick = 0;
        //    IO_STATUS_BLOCK isb;

        //    DWORD contentLengthSize = sizeof(DWORD);
        //    DWORD contentLength = 0;

        //    // Initialize hash algorithm.
        //    PhInitializeHash(&hashContext, Sha1HashAlgorithm);

        //    if (!WinHttpQueryHeaders(
        //        requestHandle, 
        //        WINHTTP_QUERY_CONTENT_LENGTH | WINHTTP_QUERY_FLAG_NUMBER, 
        //        WINHTTP_HEADER_NAME_BY_INDEX,
        //        &contentLength, 
        //        &contentLengthSize, 
        //        0
        //        ))
        //    {
        //        __leave;
        //    }

        //    // Zero the buffer.
        //    ZeroMemory(buffer, PAGE_SIZE);

        //    //        // Reset the counters.
        //    //        bytesDownloaded = 0, timeTransferred = 0, LastUpdateTime = 0;
        //    //        IsUpdating = FALSE;

        //    //        // Start the clock.
        //    //        startTick = GetTickCount();
        //    //        timeTransferred = startTick;

        //    // Download the data.
        //    while (WinHttpReadData(requestHandle, buffer, PAGE_SIZE, &bytesDownloaded))
        //    {
        //        // If we get zero bytes, the file was uploaded or there was an error.
        //        if (bytesDownloaded == 0)
        //            break;

        //        // If the dialog was closed, just dispose and exit.
        //        if (!UpdateDialogThreadHandle)
        //            __leave;

        //        // Update the hash of bytes we downloaded.
        //        PhUpdateHash(&hashContext, buffer, bytesDownloaded);

        //        // Write the downloaded bytes to disk.
        //        if (!NT_SUCCESS(NtWriteFile(
        //            tempFileHandle,
        //            NULL,
        //            NULL,
        //            NULL,
        //            &isb,
        //            buffer,
        //            bytesDownloaded,
        //            NULL,
        //            NULL
        //            )))
        //        {
        //            __leave;
        //        }

        //        // Check the number of bytes written are the same we downloaded.
        //        if (bytesDownloaded != isb.Information)
        //            __leave;

        //        //// Update our total bytes downloaded
        //        //PhAcquireQueuedLockExclusive(&Lock);
        //        //bytesDownloaded += (DWORD)isb.Information;
        //        //PhReleaseQueuedLockExclusive(&Lock);
        //        //AsyncUpdate();
        //        {
        //            //DWORD time_taken;
        //            //DWORD download_speed;        
        //            ////DWORD time_remain = (MulDiv(time_taken, contentLength, bytesDownloaded) - time_taken);
        //            //int percent;
        //            //PPH_STRING dlRemaningBytes;
        //            //PPH_STRING dlLength;
        //            ////PPH_STRING dlSpeed;
        //            //PPH_STRING statusText;

        //            ////PhAcquireQueuedLockExclusive(&Lock);

        //            ////time_taken = (GetTickCount() - timeTransferred);
        //            ////download_speed = (bytesDownloaded / max(time_taken, 1));  
        //            //percent = MulDiv(100, bytesDownloaded, contentLength);

        //            //dlRemaningBytes = PhFormatSize(bytesDownloaded, -1);
        //            //dlLength = PhFormatSize(contentLength, -1);
        //            ////dlSpeed = PhFormatSize(download_speed * 1024, -1);

        //            ////LastUpdateTime = GetTickCount();

        //            ////PhReleaseQueuedLockExclusive(&Lock);

        //            //statusText = PhFormatString(
        //            //    L"%s (%d%%) of %s",
        //            //    dlRemaningBytes->Buffer,
        //            //    percent,
        //            //    dlLength->Buffer
        //            //    );

        //            //SetDlgItemText(hwndDlg, IDC_STATUS, statusText->Buffer);
        //            //SendDlgItemMessage(hwndDlg, IDC_PROGRESS, PBM_SETPOS, percent, 0);

        //            //PhDereferenceObject(statusText);
        //            //PhDereferenceObject(dlLength);
        //            //PhDereferenceObject(dlRemaningBytes);
        //        }

        //    }

        //    // Check if we downloaded the entire file.
        //    //assert(bytesDownloaded == contentLength);

        //    // Compute our hash result.
        //    if (PhFinalHash(&hashContext, &hashBuffer, 20, NULL))
        //    {
        //        // Allocate our hash string, hex the final hash result in our hashBuffer.
        //        PPH_STRING hexString = PhBufferToHexString(hashBuffer, 20);

        //        if (PhEqualString(hexString, updateData->Hash, TRUE))
        //        {
        //            // If PH is not elevated, set the UAC shield for the install button as the setup requires elevation.
        //            if (!PhElevated)
        //                SendMessage(GetDlgItem(hwndDlg, IDC_DOWNLOAD), BCM_SETSHIELD, 0, TRUE);

        //            // Set the download result, don't include hash status since it succeeded.
        //            //SetDlgItemText(hwndDlg, IDC_STATUS, L"Download Complete");
        //            // Set button text for next action
        //            Button_SetText(GetDlgItem(hwndDlg, IDC_DOWNLOAD), L"Install");
        //            // Enable the Install button
        //            Button_Enable(GetDlgItem(hwndDlg, IDC_DOWNLOAD), TRUE);
        //            // Hash succeeded, set state as ready to install.
        //            PhUpdaterState = Install;
        //        }
        //        else
        //        {
        //            if (WindowsVersion > WINDOWS_XP)
        //                SendDlgItemMessage(hwndDlg, IDC_PROGRESS, PBM_SETSTATE, PBST_ERROR, 0L);

        //            SetDlgItemText(hwndDlg, IDC_STATUS, L"Download complete, SHA1 Hash failed.");

        //            // Set button text for next action
        //            Button_SetText(GetDlgItem(hwndDlg, IDC_DOWNLOAD), L"Retry");
        //            // Enable the Install button
        //            Button_Enable(GetDlgItem(hwndDlg, IDC_DOWNLOAD), TRUE);
        //            // Hash failed, reset state to downloading so user can redownload the file.
        //            PhUpdaterState = Download;
        //        }

        //        PhDereferenceObject(hexString);
        //    }
        //    else
        //    {
        //        SetDlgItemText(hwndDlg, IDC_STATUS, L"PhFinalHash failed");

        //        // Show fancy Red progressbar if hash failed on Vista and above.
        //        if (WindowsVersion > WINDOWS_XP)
        //            SendDlgItemMessage(hwndDlg, IDC_PROGRESS, PBM_SETSTATE, PBST_ERROR, 0L);
        //    }
        //}
    }
    __finally
    {

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
                    if (PhInstalledUsingSetup())
                    {
                        HANDLE downloadThreadHandle = NULL;

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

                    break;
            //case Install:
            //    {
            //        SHELLEXECUTEINFO info = { sizeof(SHELLEXECUTEINFO) };
            //        info.lpFile = SetupFilePath->Buffer;
            //        info.lpVerb = L"runas";
            //        info.nShow = SW_SHOW;
            //        info.hwnd = hwndDlg;

            //        ProcessHacker_PrepareForEarlyShutdown(PhMainWndHandle);

            //        if (!ShellExecuteEx(&info))
            //        {
            //            // Install failed, cancel the shutdown.
            //            ProcessHacker_CancelEarlyShutdown(PhMainWndHandle);

            //            // Set button text for next action
            //            Button_SetText(GetDlgItem(hwndDlg, IDC_DOWNLOAD), L"Retry");
            //        }
            //        else
            //        {
            //            ProcessHacker_Destroy(PhMainWndHandle);
            //        }
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
                L"Process Hacker %u.%u",
                context->MajorVersion,
                context->MinorVersion
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
    case WM_NOTIFY:
        {
            switch (((LPNMHDR)lParam)->code)
            {
            case NM_CLICK: // Mouse
            case NM_RETURN: // Keyboard 
                {
                    // Launch the ReleaseNotes URL (if it exists) with the default browser
                    if (!PhIsNullOrEmptyString(context->ReleaseNotesUrl))
                    {
                        PhShellExecute(hwndDlg, context->ReleaseNotesUrl->Buffer, NULL);
                    }
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
    {
        context = (PUPDATER_XML_DATA)Parameter;
    }
    else
    {
        context = (PUPDATER_XML_DATA)PhAllocate(sizeof(UPDATER_XML_DATA));
        memset(context, 0, sizeof(UPDATER_XML_DATA));
    }

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
    FreeUpdateData(context);

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

//static VOID AsyncUpdate(
//    VOID
//    )
//{
//    if (!IsUpdating)
//    {
//        IsUpdating = TRUE;
//
//        if (!PostMessage(UpdateDialogHandle, WM_UPDATE, 0, 0))
//        {
//            IsUpdating = FALSE;
//        }
//    }
//}

