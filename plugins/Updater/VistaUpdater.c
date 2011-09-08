/*
* Process Hacker Update Checker -
*   main window
*
* Copyright (C) 2011 wj32
* Copyright (C) 2011 dmex
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

#include "updater.h"
#include "process.h"

// Always consider the remote version newer
#ifdef _DEBUG
#define TEST_MODE
#endif
static PH_UPDATER_STATE PhUpdaterState = Default;
static PPH_STRING RemoteHashString = NULL, LocalFilePathString = NULL, LocalFileNameString = NULL;
static HANDLE TempFileHandle = NULL;
static HINTERNET NetInitialize = NULL;
static HINTERNET NetConnection = NULL;
static HINTERNET NetRequest = NULL;

VOID SetProgressBarMarquee(HWND handle, BOOLEAN startMarquee, INT speed);
VOID UpdateContent(HWND handle, LPCWSTR content);
void EnableButton(HWND handle, INT buttonId, BOOLEAN enable);

static void __cdecl VistaSilentWorkerThreadStart(
    __in PVOID Parameter
    )
{
    if (!ConnectionAvailable())
        return;

    if (!InitializeConnection(UPDATE_URL, UPDATE_FILE))
        return;

    // Send the HTTP request.
    if (HttpSendRequest(NetRequest, NULL, 0, NULL, 0))
    {
        PSTR data;
        UPDATER_XML_DATA xmlData;
        PPH_STRING localVersion;
        ULONG localMajorVersion = 0;
        ULONG localMinorVersion = 0;

        // Read the resulting xml into our buffer.
        if (!ReadRequestString(NetRequest, &data, NULL))
            return;

        if (!QueryXmlData(data, &xmlData))
        {
            PhFree(data);
            return;
        }

        PhFree(data);

        localVersion = PhGetPhVersion();

#ifndef TEST_MODE
        if (!ParseVersionString(localVersion->Buffer, &localMajorVersion, &localMinorVersion))
        {
            PhDereferenceObject(localVersion);
            FreeXmlData(&xmlData);
        }
#else
        localMajorVersion = 0;
        localMinorVersion = 0;
#endif

        if (CompareVersions(xmlData.MajorVersion, xmlData.MinorVersion, localMajorVersion, localMinorVersion) > 0)
        {
            // Don't spam the user the second they open PH, delay dialog creation for 5 seconds.
            Sleep(5000);

            DisposeConnection();

            ShowUpdateTaskDialog();
        }

        PhDereferenceObject(localVersion);
        FreeXmlData(&xmlData);
    }
    else
    {
        LogEvent(PhFormatString(L"Updater: (WorkerThreadStart) HttpSendRequest failed (%d)", GetLastError()));
    }

    DisposeConnection();
}

static void __cdecl VistaWorkerThreadStart(
    __in PVOID Parameter
    )
{
    INT result = 0;
    HWND hwndDlg = (HWND)Parameter;
  
    Sleep(4000);

    if (!InitializeConnection(UPDATE_URL, UPDATE_FILE))
        return;

    // Send the HTTP request.
    if (HttpSendRequest(NetRequest, NULL, 0, NULL, 0))
    {
        PSTR data;
        UPDATER_XML_DATA xmlData;
        PPH_STRING localVersion;
        ULONG localMajorVersion = 0;
        ULONG localMinorVersion = 0;

        // Read the resulting xml into our buffer.
        if (!ReadRequestString(NetRequest, &data, NULL))
            return;

        if (!QueryXmlData(data, &xmlData))
        {
            PhFree(data);
            return;
        }

        PhFree(data);

        localVersion = PhGetPhVersion();

#ifndef TEST_MODE
        if (!ParseVersionString(localVersion->Buffer, &localMajorVersion, &localMinorVersion))
        {
            PhDereferenceObject(localVersion);
            FreeXmlData(&xmlData);
        }
#else
        localMajorVersion = 0;
        localMinorVersion = 0;
#endif

        result = CompareVersions(xmlData.MajorVersion, xmlData.MinorVersion, localMajorVersion, localMinorVersion);

        PhSwapReference(&RemoteHashString, xmlData.Hash);
        
        if (result > 0)
        {
            TASKDIALOGCONFIG tc = { 0 };
            INT nButton;

            const TASKDIALOG_BUTTON cb[] =
            { 
                { 1005, L"&Download the update now" },
                { 1006, L"Do &not download the update" },
            };
    
            PPH_STRING summaryText = PhFormatString(L"Version: %u.%u \r\nReleased: %s \r\nSize: %s", xmlData.MajorVersion, xmlData.MinorVersion, xmlData.RelDate->Buffer, xmlData.Size->Buffer);
            LocalFileNameString = PhFormatString(L"processhacker-%u.%u-setup.exe", xmlData.MajorVersion, xmlData.MinorVersion);

            tc.cbSize = sizeof(tc);
            tc.hwndParent = PhMainWndHandle;
            tc.hInstance = PhLibImageBase;
            tc.dwFlags = 
                TDF_ALLOW_DIALOG_CANCELLATION |
                TDF_USE_COMMAND_LINKS |  
                TDF_POSITION_RELATIVE_TO_WINDOW | 
                TDF_EXPAND_FOOTER_AREA | 
                TDF_ENABLE_HYPERLINKS;

            tc.dwCommonButtons = TDCBF_CLOSE_BUTTON;
            tc.hMainIcon = MAKEINTRESOURCEW(101);
            tc.pszWindowTitle = L"Process Hacker Updater";
            tc.pszMainInstruction = PhFormatString(L"Process Hacker %u.%u available", xmlData.MajorVersion, xmlData.MinorVersion)->Buffer;
            tc.pszContent = summaryText->Buffer;

            tc.pszExpandedInformation = L"<A HREF=\"http://processhacker.sourceforge.net/changelog.php\">View Changelog</A>";
            tc.cButtons = ARRAYSIZE(cb);
            tc.pButtons = cb;
    
            tc.pfCallback = TaskDlgWndProc;

            SendMessage(hwndDlg, TDM_NAVIGATE_PAGE, 0, (LPARAM)&tc);
        }
        else if (result == 0)
        {
            TASKDIALOGCONFIG tc = { 0 };
    
            PPH_STRING sText = PhFormatString(L"You're running the latest version:");
            PPH_STRING sText2 = PhFormatString(L"Version: %u.%u \r\nReleased: %s", localMajorVersion, localMinorVersion, xmlData.RelDate->Buffer);
            
            tc.cbSize = sizeof(tc);
            tc.hwndParent = PhMainWndHandle;
            tc.hInstance = PhLibImageBase;
            tc.dwFlags = TDF_ALLOW_DIALOG_CANCELLATION | TDF_USE_COMMAND_LINKS | TDF_POSITION_RELATIVE_TO_WINDOW;
            tc.dwCommonButtons = TDCBF_CLOSE_BUTTON;
            tc.pszWindowTitle = L"Process Hacker Updater";
            tc.pszMainInstruction = sText->Buffer;
            tc.pszContent = sText2->Buffer;
            tc.pszMainIcon = MAKEINTRESOURCEW(SecuritySuccess);

            SendMessage(hwndDlg, TDM_NAVIGATE_PAGE, NULL, &tc);

            PhDereferenceObject(sText2);
            PhDereferenceObject(sText);
        }
        else if (result < 0)
        {
            TASKDIALOGCONFIG tc = { 0 };
    
            PPH_STRING summaryText = PhFormatString(L"You're running a newer version");
            PPH_STRING verText = PhFormatString(L"Current version: %s \r\nRemote version: %u.%u", localVersion->Buffer, xmlData.MajorVersion, xmlData.MinorVersion);

            tc.cbSize = sizeof(tc);
            tc.hwndParent = PhMainWndHandle;
            tc.hInstance = PhLibImageBase;
            tc.dwFlags = 
                 TDF_ALLOW_DIALOG_CANCELLATION
                | TDF_USE_COMMAND_LINKS
                | TDF_POSITION_RELATIVE_TO_WINDOW 
                | TDF_CALLBACK_TIMER;

            tc.dwCommonButtons = TDCBF_CLOSE_BUTTON;
            tc.pszMainIcon = MAKEINTRESOURCEW(SecurityWarning);
            tc.pszWindowTitle = L"Process Hacker Updater";
            tc.pszMainInstruction = summaryText->Buffer;
            tc.pszContent = verText->Buffer;

            SendMessage(hwndDlg, TDM_NAVIGATE_PAGE, 0, (LPARAM)&tc);

            PhDereferenceObject(verText);
            PhDereferenceObject(summaryText);
        }

        FreeXmlData(&xmlData);
        PhDereferenceObject(localVersion);

        if (LocalFileNameString)
        {
            PPH_STRING sText = PhFormatString(L"\\\\%s", LocalFileNameString->Buffer);
           
            LocalFilePathString = PhGetKnownLocation(CSIDL_DESKTOP, sText->Buffer);

            PhDereferenceObject(sText);
            
            // Create output file
            if ((TempFileHandle = CreateFile(
                LocalFilePathString->Buffer,
                GENERIC_WRITE,
                FILE_SHARE_WRITE,
                0,                     // handle cannot be inherited
                CREATE_ALWAYS,         // if file exists, delete it
                FILE_ATTRIBUTE_NORMAL,
                0)) == INVALID_HANDLE_VALUE)
            {
                LogEvent(PhFormatString(L"Updater: (InitializeFile) CreateFile failed (%d)", GetLastError()));
            }
        }
    }
    else
    {
        LogEvent(PhFormatString(L"Updater: (WorkerThreadStart) HttpSendRequest failed (%d)", GetLastError()));
    }

    DisposeConnection();
}

static void __cdecl VistaDownloadWorkerThreadStart(
    __in PVOID Parameter
    )
{
    DWORD dwTotalReadSize = 0,
          dwContentLen = 0,
          dwBytesRead = 0,
          dwBytesWritten = 0,
          dwBufLen = sizeof(dwContentLen);

    BOOL nReadFile = FALSE;
    HWND hwndDlg = (HWND)Parameter, 
         hwndProgress = GetDlgItem(hwndDlg, IDC_PROGRESS);

    PPH_STRING uriPath = PhFormatString(DOWNLOAD_PATH, LocalFileNameString->Buffer);
    PH_HASH_CONTEXT hashContext;
     
    EnableButton(hwndDlg, 1011, FALSE);
   
    SendMessage(hwndDlg, TDM_UPDATE_ELEMENT_TEXT, TDE_CONTENT, (LPARAM)L"\r\n\r\nConnectionAvailable...");

    if (!ConnectionAvailable())
        return;
  
    SendMessage(hwndDlg, TDM_UPDATE_ELEMENT_TEXT, TDE_CONTENT, (LPARAM)L"\r\n\r\nInitializeConnection...");

    if (!InitializeConnection(DOWNLOAD_SERVER, uriPath->Buffer))
    {
        PhDereferenceObject(uriPath);
        return;
    }

    SendMessage(hwndDlg, TDM_UPDATE_ELEMENT_TEXT, TDE_CONTENT, (LPARAM)L"\r\n\r\nHttpSendRequest...");

    if (HttpSendRequest(NetRequest, NULL, 0, NULL, 0))
    {
        CHAR buffer[BUFFER_LEN];
        DWORD dwStartTicks = GetTickCount();
        DWORD dwLastTicks = dwStartTicks;
        DWORD dwLastTotalBytes = 0, 
              dwNowTicks = 0,
              dwTimeTaken = 0;

        // Zero the buffer.
        RtlZeroMemory(buffer, BUFFER_LEN);
     
        // Initialize hash algorithm.
        PhInitializeHash(&hashContext, Md5HashAlgorithm);

        SendMessage(hwndDlg, TDM_UPDATE_ELEMENT_TEXT, TDE_CONTENT, (LPARAM)L"\r\n\r\nHttpQueryInfo...");

        if (HttpQueryInfo(NetRequest, HTTP_QUERY_CONTENT_LENGTH | HTTP_QUERY_FLAG_NUMBER, &dwContentLen, &dwBufLen, 0))
        {
            SendMessage(hwndDlg, TDM_UPDATE_ELEMENT_TEXT, TDE_CONTENT, (LPARAM)L"\r\n\r\nInternetReadFile...");

            while ((nReadFile = InternetReadFile(NetRequest, buffer, BUFFER_LEN, &dwBytesRead)))
            {
                if (dwBytesRead == 0)
                    break;

                if (!nReadFile)
                {
                    LogEvent(PhFormatString(L"Updater: (DownloadWorkerThreadStart) InternetReadFile failed (%d)", GetLastError()));
                    return;
                }

                // Update the hash of bytes we just downloaded.
                PhUpdateHash(&hashContext, buffer, dwBytesRead);

                // Write the downloaded bytes to disk.
                if (!WriteFile(TempFileHandle, buffer, dwBytesRead, &dwBytesWritten, NULL))
                {
                    LogEvent(PhFormatString(L"Updater: (DownloadWorkerThreadStart) WriteFile failed (%d)", GetLastError()));
                    return;
                }

                // Zero the buffer.
                RtlZeroMemory(buffer, BUFFER_LEN);

                // Check dwBytesRead are the same dwBytesWritten length returned by WriteFile.
                if (dwBytesRead != dwBytesWritten)
                {
                    LogEvent(PhFormatString(L"Updater: (DownloadWorkerThreadStart) WriteFile failed (%d)", GetLastError()));
                    return;
                }

                // Update our total bytes downloaded
                dwTotalReadSize += dwBytesRead;

                // Calculate the transfer rate and download speed. 
                dwNowTicks = GetTickCount();
                dwTimeTaken = dwNowTicks - dwLastTicks;

                // Update the transfer rate and estimated time every second.
                if (dwTimeTaken > 0)
                {
                    ULONG64 kbPerSecond = (ULONG64)(((double)(dwTotalReadSize) - (double)(dwLastTotalBytes)) / ((double)(dwTimeTaken)) * 1024);
        
                    // Set last counters for the next loop.
                    dwLastTicks = dwNowTicks;
                    dwLastTotalBytes = dwTotalReadSize;

                    // Update the estimated time left, progress and download speed.
                    {
                        ULONG64 dwSecondsLeft =  (ULONG64)(((double)dwNowTicks - dwStartTicks) / dwTotalReadSize * (dwContentLen - dwTotalReadSize) / 1000);

                        // Calculate the percentage of our total bytes downloaded per the length.
                        INT dlProgress = (INT)(((double)dwTotalReadSize / (double)dwContentLen) * 100);

                        PPH_STRING dlCurrent = PhFormatSize(dwTotalReadSize, -1);
                        PPH_STRING dlLength = PhFormatSize(dwContentLen, -1);
                        PPH_STRING sRemaningBytes = PhFormatSize(dwContentLen - dwLastTotalBytes, -1);
                        PPH_STRING sDlSpeed = PhFormatSize(kbPerSecond, -1);
 
                        PPH_STRING sText = PhFormatString(
                            L"Speed: %s/s \r\nDownloaded: %s of %s (%d%%)\r\nRemaning: %s (%ds)", 
                            sDlSpeed->Buffer, dlCurrent->Buffer, dlLength->Buffer, dlProgress, sRemaningBytes->Buffer, dwSecondsLeft
                            );

                        SetProgressBarPosition(hwndDlg, dlProgress);

                        SendMessage(hwndDlg, TDM_UPDATE_ELEMENT_TEXT, TDE_CONTENT, (LPARAM)sText->Buffer);

                        PhDereferenceObject(sText);
                        PhDereferenceObject(sDlSpeed);
                        PhDereferenceObject(sRemaningBytes);
                        PhDereferenceObject(dlLength);
                        PhDereferenceObject(dlCurrent);
                    }
                }
            }
        }
        else
        {
            // No content length...impossible to calculate % complete so just read until we are done.
            LogEvent(PhFormatString(L"Updater: (DownloadWorkerThreadStart) HttpQueryInfo failed (%d)", GetLastError()));

            while ((nReadFile = InternetReadFile(NetRequest, buffer, BUFFER_LEN, &dwBytesRead)))
            {
                if (dwBytesRead == 0)
                    break;

                if (!nReadFile)
                {
                    LogEvent(PhFormatString(L"Updater: (DownloadWorkerThreadStart) InternetReadFile failed (%d)", GetLastError()));
                    return;
                }

                PhUpdateHash(&hashContext, buffer, dwBytesRead);

                if (!WriteFile(TempFileHandle, buffer, dwBytesRead, &dwBytesWritten, NULL))
                {
                    LogEvent(PhFormatString(L"Updater: (DownloadWorkerThreadStart) WriteFile failed (%d)", GetLastError()));
                    return;
                }

                // Reset the buffer.
                RtlZeroMemory(buffer, BUFFER_LEN);

                if (dwBytesRead != dwBytesWritten)
                {
                    LogEvent(PhFormatString(L"Updater: (DownloadWorkerThreadStart) WriteFile failed (%d)", GetLastError()));
                    return;
                }
            }
        }

        DisposeConnection();
        DisposeFileHandles();

        PhDereferenceObject(uriPath);

        EnableButton(hwndDlg, 1011, TRUE);
        EnableButton(hwndDlg, 1008, FALSE);
        SetProgressBarPosition(hwndDlg, 100);
        //if (!PhElevated)
        //SendMessage(hwndDlg, TDM_SET_BUTTON_ELEVATION_REQUIRED_STATE , 0, TRUE);

        {
            UCHAR hashBuffer[20];
            ULONG hashLength = 0;

            if (PhFinalHash(&hashContext, hashBuffer, 20, &hashLength))
            {
                // Allocate our hash PCWSTR, hex the final hash result in our hashBuffer.
                PH_STRING *hexString = PhBufferToHexString(hashBuffer, hashLength);

                if (PhEqualString(hexString, RemoteHashString, TRUE))
                {
                    SendMessage(hwndDlg, TDM_UPDATE_ELEMENT_TEXT, TDE_CONTENT, (LPARAM)L"Download Complete.\r\n\r\nHash Verified");
                }
                else
                {
                    SetProgressBarState(hwndDlg, TRUE);
                    SendMessage(hwndDlg, TDM_UPDATE_ELEMENT_TEXT, TDE_CONTENT, (LPARAM)L"Download Complete.\r\n\r\nHash failed");
                }

                PhDereferenceObject(hexString);
            }
            else
            {
                SetProgressBarState(hwndDlg, TRUE);
                SendMessage(hwndDlg, TDM_UPDATE_ELEMENT_TEXT, TDE_CONTENT, (LPARAM)L"Download Complete.\r\n\r\nHash failed");
            }
        }
    }
    else
    {
        LogEvent(PhFormatString(L"Updater: (DownloadWorkerThreadStart) HttpSendRequest failed (%d)", GetLastError()));
    }
}

VOID ShowUpdateTaskDialog(VOID)
{
    TASKDIALOGCONFIG UpdateAvailablePage = { 0 };
    INT nButton;
    INT nRadioButton;
    BOOL fVerificationFlagChecked;
    
    UpdateAvailablePage.cbSize = sizeof(UpdateAvailablePage);
    UpdateAvailablePage.hwndParent = PhMainWndHandle;
    UpdateAvailablePage.hInstance = PhLibImageBase;
   
    UpdateAvailablePage.dwFlags = 
        TDF_ALLOW_DIALOG_CANCELLATION |  
        TDF_ENABLE_HYPERLINKS |
        TDF_POSITION_RELATIVE_TO_WINDOW | 
        TDF_SHOW_MARQUEE_PROGRESS_BAR;
    UpdateAvailablePage.dwCommonButtons = TDCBF_CLOSE_BUTTON;
    UpdateAvailablePage.pszMainIcon = MAKEINTRESOURCEW(101);
    UpdateAvailablePage.pszWindowTitle = L"Process Hacker Updater";
    UpdateAvailablePage.pszMainInstruction = L"Checking for Updates...";
    UpdateAvailablePage.pfCallback = TaskDlgWndProc;

    if (!SUCCEEDED(TaskDialogIndirect_I(&UpdateAvailablePage, &nButton, &nRadioButton, &fVerificationFlagChecked)))
    {
        // some error occurred...check hr to see what it is
    }

    DisposeConnection();
    DisposeStrings();
    DisposeFileHandles();
}


HRESULT CALLBACK TaskDlgWndProc(
    __in HWND hwndDlg, 
    __in UINT uMsg, 
    __in WPARAM wParam, 
    __in LPARAM lParam, 
    __in LONG_PTR lpRefData
    )
{
    switch (uMsg)
    {
    case TDN_CREATED:
        {
            PhUpdaterState = Default;

            SetProgressBarMarquee(hwndDlg, TRUE, 100);
            _beginthread(VistaWorkerThreadStart, 0, hwndDlg);
        }
        break;
    case TDN_DESTROYED:
        {
            DisposeConnection();
            DisposeStrings();
            DisposeFileHandles();
        }
        break; 
    case TDN_BUTTON_CLICKED:
        {
            switch(wParam)
            {
            case 1008:
                {
                    return S_FALSE;
                }
            case 1005:
                {
                    TASKDIALOGCONFIG tc = { 0 };

                    const TASKDIALOG_BUTTON cb[] =
                    { 
                        { 1011, L"&Install" },
                        //{ 1008, L"&Pause" },
                    };

                    tc.cbSize = sizeof(tc);
                    tc.hMainIcon = MAKEINTRESOURCEW(101);
                    tc.hwndParent = PhMainWndHandle;
                    tc.hInstance = PhLibImageBase;
                    tc.dwFlags = TDF_ALLOW_DIALOG_CANCELLATION| TDF_POSITION_RELATIVE_TO_WINDOW | TDF_SHOW_PROGRESS_BAR;
                    tc.pszWindowTitle = L"Process Hacker Updater";
                    tc.pszMainInstruction = L"Downloading Update...";
                    tc.pszContent = L"Initializing...\r\n\r\n";
                    tc.dwCommonButtons = TDCBF_CLOSE_BUTTON;
                    tc.nDefaultButton = IDCLOSE; 

                    tc.pfCallback = TaskDlgWndProc;
                    tc.cButtons = ARRAYSIZE(cb);
                    tc.pButtons = cb;
                                        
                    // Natigate to new page.
                    SendMessage(hwndDlg, TDM_NAVIGATE_PAGE, 0, (LPARAM)&tc);
                    
                    PhUpdaterState = Downloading;

                    return S_FALSE;
                }
            }
        }
        break;
    case TDN_NAVIGATED:
        { 
            if (PhUpdaterState == Downloading)
            {
                _beginthread(VistaDownloadWorkerThreadStart, 0, hwndDlg);
            }
        }
        break;
    case TDN_HYPERLINK_CLICKED:        
        {
            PPH_STRING sUri = PhCreateString(lParam);
            
            PhShellExecute(hwndDlg, sUri->Buffer, NULL);

            PhDereferenceObject(sUri);
        }
        break;
    }

    return S_OK;
}

static BOOL InitializeConnection(
    __in PCWSTR host,
    __in PCWSTR path
    )
{
    // Initialize the wininet library.
    NetInitialize = InternetOpen(
        L"PH Updater", // user-agent
        INTERNET_OPEN_TYPE_PRECONFIG, // use system proxy configuration
        NULL,
        NULL,
        0
        );

    if (!NetInitialize)
    {
        LogEvent(PhFormatString(L"Updater: (InitializeConnection) InternetOpen failed (%d)", GetLastError()));
        return FALSE;
    }

    // Connect to the server.
    NetConnection = InternetConnect(
        NetInitialize,
        host,
        INTERNET_DEFAULT_HTTP_PORT,
        NULL,
        NULL,
        INTERNET_SERVICE_HTTP,
        0,
        0
        );

    if (!NetConnection)
    {
        LogEvent(PhFormatString(L"Updater: (InitializeConnection) InternetConnect failed (%d)", GetLastError()));
        DisposeConnection();
        return FALSE;
    }

    // Open the HTTP request.
    NetRequest = HttpOpenRequest(
        NetConnection,
        L"GET",
        path,
        L"HTTP/1.1",
        NULL,
        NULL,
        INTERNET_FLAG_PRAGMA_NOCACHE | INTERNET_FLAG_RELOAD | INTERNET_FLAG_NO_CACHE_WRITE | INTERNET_FLAG_RESYNCHRONIZE,
        0
        );

    if (!NetRequest)
    {
        LogEvent(PhFormatString(L"Updater: (InitializeConnection) HttpOpenRequest failed (%d)", GetLastError()));
        DisposeConnection();
        return FALSE;
    }

    return TRUE;
}

VOID VistaStartInitialCheck(VOID)
{
    // Queue up our initial update check.
    _beginthread(VistaSilentWorkerThreadStart, 0, NULL);
}


static VOID DisposeConnection(VOID)
{
    if (NetInitialize)
    {
        InternetCloseHandle(NetInitialize);
        NetInitialize = NULL;
    }

    if (NetConnection)
    {
        InternetCloseHandle(NetConnection);
        NetConnection = NULL;
    }

    if (NetRequest)
    {
        InternetCloseHandle(NetRequest);
        NetRequest = NULL;
    }
}

static VOID DisposeStrings(VOID)
{
    if (LocalFilePathString)
    {
        PhDereferenceObject(LocalFilePathString);
        LocalFilePathString = NULL;
    }

    if (LocalFileNameString)
    {
        PhDereferenceObject(LocalFileNameString);
        LocalFileNameString = NULL;
    }
}

static VOID DisposeFileHandles(VOID)
{
    if (TempFileHandle)
    {
        NtClose(TempFileHandle);
        TempFileHandle = NULL;
    }
}

static BOOL ReadRequestString(
    __in HINTERNET Handle,
    __out PSTR *Data,
    __out_opt PULONG DataLength
    )
{
    CHAR buffer[BUFFER_LEN];
    PSTR data;
    ULONG allocatedLength;
    ULONG dataLength;
    ULONG returnLength;

    allocatedLength = sizeof(buffer);
    data = PhAllocate(allocatedLength);
    dataLength = 0;

    while (InternetReadFile(Handle, buffer, BUFFER_LEN, &returnLength))
    {
        if (returnLength == 0)
            break;

        if (allocatedLength < dataLength + returnLength)
        {
            allocatedLength *= 2;
            data = PhReAllocate(data, allocatedLength);
        }

        memcpy(data + dataLength, buffer, returnLength);
        dataLength += returnLength;
    }

    if (allocatedLength < dataLength + 1)
    {
        allocatedLength++;
        data = PhReAllocate(data, allocatedLength);
    }

    // Ensure that the buffer is null-terminated.
    data[dataLength] = 0;

    *Data = data;

    if (DataLength)
        *DataLength = dataLength;

    return TRUE;
}

static VOID FreeXmlData(
    __in PUPDATER_XML_DATA XmlData
    )
{
    PhDereferenceObject(XmlData->RelDate);
    PhDereferenceObject(XmlData->Size);
    PhDereferenceObject(XmlData->Hash);
}

/// <summary>
/// Simulate the action of a button click in the TaskDialog. This can be a DialogResult value 
/// or the ButtonID set on a TasDialogButton set on TaskDialog.Buttons.
/// </summary>
/// <param name="buttonId">Indicates the button ID to be selected.</param>
/// <returns>If the function succeeds the return value is true.</returns>
BOOL ClickButton(HWND handle, INT buttonId)
{
    // TDM_CLICK_BUTTON = WM_USER+102, // wParam = Button ID
    return SendMessage(handle, TDM_CLICK_BUTTON, buttonId, NULL) != NULL;
}

/// <summary>
/// Used to indicate whether the hosted progress bar should be displayed in marquee mode or not.
/// </summary>
/// <param name="marquee">Specifies whether the progress bar sbould be shown in Marquee mode.
/// A value of true turns on Marquee mode.</param>
/// <returns>If the function succeeds the return value is true.</returns>
BOOL SetMarqueeProgressBar(HWND handle, BOOL marquee)
{
    // TDM_SET_MARQUEE_PROGRESS_BAR        = WM_USER+103, // wParam = 0 (nonMarque) wParam != 0 (Marquee)
    return SendMessage(handle, TDM_SET_MARQUEE_PROGRESS_BAR, (marquee ? 1 : NULL), NULL) != NULL;

    // Future: get more detailed error from and throw.
}

/// <summary>
/// Sets the state of the progress bar.
/// </summary>
/// <param name="newState">The state to set the progress bar.</param>
/// <returns>If the function succeeds the return value is true.</returns>
BOOL SetProgressBarState(HWND handle, BOOL newState)
{
    // TDM_SET_PROGRESS_BAR_STATE          = WM_USER+104, // wParam = new progress state
    return SendMessage(
        handle,
        TDM_SET_PROGRESS_BAR_STATE,
        PBST_ERROR, //PBST_NORMAL, PBST_PAUSE, 
        NULL) != NULL;

    // Future: get more detailed error from and throw.
}

/// <summary>
/// Set the minimum and maximum values for the hosted progress bar.
/// </summary>
/// <param name="minRange">Minimum range value. By default, the minimum value is zero.</param>
/// <param name="maxRange">Maximum range value.  By default, the maximum value is 100.</param>
/// <returns>If the function succeeds the return value is true.</returns>
BOOL SetProgressBarRange(HWND handle, INT minRange, INT maxRange)
{
    // TDM_SET_PROGRESS_BAR_RANGE          = WM_USER+105, // lParam = MAKELPARAM(nMinRange, nMaxRange)
    // #define MAKELPARAM(l, h)      ((LPARAM)(DWORD)MAKELONG(l, h))
    // #define MAKELONG(a, b)      ((LONG)(((WORD)(((DWORD_PTR)(a)) & 0xffff)) | ((DWORD)((WORD)(((DWORD_PTR)(b)) & 0xffff))) << 16))
    
    return SendMessage(
        handle,
        TDM_SET_PROGRESS_BAR_RANGE,
        NULL,
        ((((INT)minRange) & 0xffff) | ((((INT)maxRange) & 0xffff) << 16))) != NULL;

    // Return value is actually prior range.
}

/// <summary>
/// Set the current position for a progress bar.
/// </summary>
/// <param name="newPosition">The new position.</param>
/// <returns>Returns the previous value if successful, or zero otherwise.</returns>
INT SetProgressBarPosition(HWND handle, INT newPosition)
{
    // TDM_SET_PROGRESS_BAR_POS = WM_USER+106, // wParam = new position
    return SendMessage(handle, TDM_SET_PROGRESS_BAR_POS, newPosition, NULL);
}

/// <summary>
/// Sets the animation state of the Marquee Progress Bar.
/// </summary>
/// <param name="startMarquee">true starts the marquee animation and false stops it.</param>
/// <param name="speed">The time in milliseconds between refreshes.</param>
VOID SetProgressBarMarquee(HWND handle, BOOLEAN startMarquee, INT speed)
{
    // TDM_SET_PROGRESS_BAR_MARQUEE  = WM_USER+107, // wParam = 0 (stop marquee), wParam != 0 (start marquee), lparam = speed (milliseconds between repaints)
    SendMessage(handle, TDM_SET_PROGRESS_BAR_MARQUEE, startMarquee, speed);
}

/// <summary>
/// Updates the content text.
/// </summary>
/// <param name="content">The new value.</param>
/// <returns>If the function succeeds the return value is true.</returns>
BOOL SetContent(HWND handle, PCWSTR content)
{
    // TDE_CONTENT,
    // TDM_SET_ELEMENT_TEXT                = WM_USER+108  // wParam = element (TASKDIALOG_ELEMENTS), lParam = new element text (LPCWSTR)
    return SendMessage(
        handle,
        TDM_SET_ELEMENT_TEXT,
        TDE_CONTENT,
        content) != NULL;
}

/// <summary>
/// Updates the Expanded Information text.
/// </summary>
/// <param name="expandedInformation">The new value.</param>
/// <returns>If the function succeeds the return value is true.</returns>
BOOL SetExpandedInformation(HWND handle, PCWSTR expandedInformation)
{
    // TDE_EXPANDED_INFORMATION,
    // TDM_SET_ELEMENT_TEXT                = WM_USER+108  // wParam = element (TASKDIALOG_ELEMENTS), lParam = new element text (LPCWSTR)
    return SendMessage(
        handle,
        TDM_SET_ELEMENT_TEXT,
        TDE_EXPANDED_INFORMATION,
        expandedInformation) != NULL;
}

/// <summary>
/// Updates the Footer text.
/// </summary>
/// <param name="footer">The new value.</param>
/// <returns>If the function succeeds the return value is true.</returns>
BOOL SetFooter(HWND handle, PCWSTR footer)
{
    // TDE_FOOTER,
    // TDM_SET_ELEMENT_TEXT                = WM_USER+108  // wParam = element (TASKDIALOG_ELEMENTS), lParam = new element text (LPCWSTR)
    return SendMessage( handle, TDM_SET_ELEMENT_TEXT, TDE_FOOTER, footer) != NULL;
}

/// <summary>
/// Updates the Main Instruction.
/// </summary>
/// <param name="mainInstruction">The new value.</param>
/// <returns>If the function succeeds the return value is true.</returns>
BOOL SetMainInstruction(HWND handle, PCWSTR mainInstruction)
{
    // TDE_MAIN_INSTRUCTION
    // TDM_SET_ELEMENT_TEXT                = WM_USER+108  // wParam = element (TASKDIALOG_ELEMENTS), lParam = new element text (LPCWSTR)
    return SendMessage(
        handle,
        TDM_SET_ELEMENT_TEXT,
        TDE_MAIN_INSTRUCTION,
        mainInstruction) != NULL;
}

/// <summary>
/// Simulate the action of a radio button click in the TaskDialog. 
/// The passed buttonID is the ButtonID set on a TaskDialogButton set on TaskDialog.RadioButtons.
/// </summary>
/// <param name="buttonId">Indicates the button ID to be selected.</param>
void ClickRadioButton(HWND handle, INT buttonId)
{
    // TDM_CLICK_RADIO_BUTTON = WM_USER+110, // wParam = Radio Button ID
    SendMessage(
        handle,
        TDM_CLICK_RADIO_BUTTON,
        buttonId,
        NULL);
}

/// <summary>
/// Enable or disable a button in the TaskDialog. 
/// The passed buttonID is the ButtonID set on a TaskDialogButton set on TaskDialog.Buttons
/// or a common button ID.
/// </summary>
/// <param name="buttonId">Indicates the button ID to be enabled or diabled.</param>
/// <param name="enable">Enambe the button if true. Disable the button if false.</param>
void EnableButton(HWND handle, INT buttonId, BOOLEAN enable)
{
    // TDM_ENABLE_BUTTON = WM_USER+111, // lParam = 0 (disable), lParam != 0 (enable), wParam = Button ID
    SendMessage(
        handle,
        TDM_ENABLE_BUTTON,
        buttonId,
        (enable ? 1 : 0));
}

/// <summary>
/// Enable or disable a radio button in the TaskDialog. 
/// The passed buttonID is the ButtonID set on a TaskDialogButton set on TaskDialog.RadioButtons.
/// </summary>
/// <param name="buttonId">Indicates the button ID to be enabled or diabled.</param>
/// <param name="enable">Enambe the button if true. Disable the button if false.</param>
void EnableRadioButton(HWND handle, INT buttonId, BOOL enable)
{
    // TDM_ENABLE_RADIO_BUTTON = WM_USER+112, // lParam = 0 (disable), lParam != 0 (enable), wParam = Radio Button ID
    SendMessage(
        handle,
        TDM_ENABLE_RADIO_BUTTON,
        buttonId,
        (enable ? 1 : 0));
}

/// <summary>
/// Check or uncheck the verification checkbox in the TaskDialog. 
/// </summary>
/// <param name="checkedState">The checked state to set the verification checkbox.</param>
/// <param name="setKeyboardFocusToCheckBox">True to set the keyboard focus to the checkbox, and fasle otherwise.</param>
void ClickVerification(HWND handle, BOOL checkedState, BOOL setKeyboardFocusToCheckBox)
{
    // TDM_CLICK_VERIFICATION = WM_USER+113, // wParam = 0 (unchecked), 1 (checked), lParam = 1 (set key focus)
    SendMessage(handle, TDM_CLICK_VERIFICATION, (checkedState ? 1 : NULL), (setKeyboardFocusToCheckBox ? 1 : NULL));
}

/// <summary>
/// Updates the content text.
/// </summary>
/// <param name="content">The new value.</param>
VOID UpdateContent(HWND handle, LPARAM content)
{
    // TDE_CONTENT, TDM_UPDATE_ELEMENT_TEXT = WM_USER+114, // wParam = element (TASKDIALOG_ELEMENTS), lParam = new element text (LPCWSTR)
    SendMessage(handle, TDM_UPDATE_ELEMENT_TEXT, TDE_CONTENT, L"FAIl");
}

/// <summary>
/// Updates the Expanded Information text. No effect if it was previously set to null.
/// </summary>
/// <param name="expandedInformation">The new value.</param>
void UpdateExpandedInformation(HWND handle, PCWSTR expandedInformation)
{
    // TDE_EXPANDED_INFORMATION,
    // TDM_UPDATE_ELEMENT_TEXT             = WM_USER+114, // wParam = element (TASKDIALOG_ELEMENTS), lParam = new element text (LPCWSTR)
    SendMessageW(
        handle,
        TDM_UPDATE_ELEMENT_TEXT,
        TDE_EXPANDED_INFORMATION,
        expandedInformation);
}

/// <summary>
/// Updates the Footer text. No Effect if it was perviously set to null.
/// </summary>
/// <param name="footer">The new value.</param>
void UpdateFooter(HWND handle, PCWSTR footer)
{
    // TDE_FOOTER,
    // TDM_UPDATE_ELEMENT_TEXT             = WM_USER+114, // wParam = element (TASKDIALOG_ELEMENTS), lParam = new element text (LPCWSTR)
    SendMessageW(handle, TDM_UPDATE_ELEMENT_TEXT, TDE_FOOTER, footer);
}

/// <summary>
/// Updates the Main Instruction.
/// </summary>
/// <param name="mainInstruction">The new value.</param>
void UpdateMainInstruction(HWND handle, PCWSTR mainInstruction)
{
    // TDE_MAIN_INSTRUCTION
    // TDM_UPDATE_ELEMENT_TEXT             = WM_USER+114, // wParam = element (TASKDIALOG_ELEMENTS), lParam = new element text (LPCWSTR)
    SendMessage(
        handle,
        TDM_UPDATE_ELEMENT_TEXT,
        TDE_MAIN_INSTRUCTION,
        mainInstruction);
}

/// <summary>
/// Designate whether a given Task Dialog button or command link should have a User Account Control (UAC) shield icon.
/// </summary>
/// <param name="buttonId">ID of the push button or command link to be updated.</param>
/// <param name="elevationRequired">False to designate that the action invoked by the button does not require elevation;
/// true to designate that the action does require elevation.</param>
void SetButtonElevationRequiredState(HWND handle, INT buttonId, BOOL elevationRequired)
{
    // TDM_SET_BUTTON_ELEVATION_REQUIRED_STATE = WM_USER+115, // wParam = Button ID, lParam = 0 (elevation not required), lParam != 0 (elevation required)
    SendMessage(
        handle,
        TDM_SET_BUTTON_ELEVATION_REQUIRED_STATE,
        buttonId,
        (elevationRequired ? 1 : NULL));
}

/// <summary>
/// Updates the main instruction icon. Note the type (standard via enum or
/// custom via  HICON type) must be used when upating the icon.
/// </summary>
/// <param name="icon">Task Dialog standard icon.</param>
void UpdateMainIcon(HWND handle, HICON icon)
{
    // TDM_UPDATE_ICON = WM_USER+116  // wParam = icon element (TASKDIALOG_ICON_ELEMENTS), lParam = new icon (hIcon if TDF_USE_HICON_* was set, PCWSTR otherwise)
    SendMessage(handle, TDM_UPDATE_ICON, TDIE_ICON_MAIN, (icon == NULL ? NULL : icon));
}

/// <summary>
/// Updates the footer icon. Note the type (standard via enum or
/// custom via  HICON type) must be used when upating the icon.
/// </summary>
/// <param name="icon">Task Dialog standard icon.</param>
void UpdateFooterIcon(HWND handle, HICON icon)
{
    // TDM_UPDATE_ICON = WM_USER+116  // wParam = icon element (TASKDIALOG_ICON_ELEMENTS), lParam = new icon (hIcon if TDF_USE_HICON_* was set, PCWSTR otherwise)
    SendMessage(
        handle,
        TDM_UPDATE_ICON,
        TDIE_ICON_FOOTER,
        (icon == NULL ? NULL : icon));
}
