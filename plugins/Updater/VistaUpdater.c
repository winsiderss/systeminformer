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

static PPH_STRING RemoteHashString = NULL, LocalFilePathString = NULL, LocalFileNameString = NULL;

static PH_UPDATER_STATE PhUpdaterState = Default;
static BOOL EnableCache = TRUE;
static BOOL WindowVisible = FALSE;
static PH_HASH_ALGORITHM HashAlgorithm = Md5HashAlgorithm;

TASKDIALOGCONFIG CheckingPage = { 0 };
TASKDIALOGCONFIG UpdateAvailablePage = { 0 };
TASKDIALOGCONFIG UpdateDownloadPage = { 0 };
TASKDIALOGCONFIG UpdateInstallPage = { 0 };

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
        
        Sleep(2000);

        if (result > 0)
        {
            TASKDIALOGCONFIG tc = { 0 };
            int nButton;

            const TASKDIALOG_BUTTON cb[] =
            { 
                { 1005, L"&Download the update now" },
                { 1006, L"Do &not download the update" },
            };
    
            PPH_STRING summaryText = PhFormatString(L"Version: %u.%u \r\nReleased: %s \r\nSize: %s", xmlData.MajorVersion, xmlData.MinorVersion, xmlData.RelDate->Buffer, xmlData.Size->Buffer);

            tc.cbSize = sizeof(tc);
            tc.hwndParent = PhMainWndHandle;
            tc.hInstance = PhLibImageBase;
            tc.dwFlags = 
                 TDF_ALLOW_DIALOG_CANCELLATION
                | TDF_USE_COMMAND_LINKS
                | TDF_POSITION_RELATIVE_TO_WINDOW 
                | TDF_CALLBACK_TIMER;

            tc.dwCommonButtons = TDCBF_CLOSE_BUTTON;
               
            // TaskDialog icons
            tc.pszMainIcon = MAKEINTRESOURCEW(SecurityWarning);

            LocalFileNameString = PhFormatString(L"processhacker-%u.%u-setup.exe", xmlData.MajorVersion, xmlData.MinorVersion);

            // TaskDialog strings
            tc.pszWindowTitle = L"Process Hacker Updater";
            tc.pszMainInstruction = PhFormatString(L"Process Hacker %u.%u available", xmlData.MajorVersion, xmlData.MinorVersion)->Buffer;
            tc.pszContent = summaryText->Buffer;
            //TaskDialog buttons
            tc.cButtons = ARRAYSIZE(cb);
            tc.pButtons = cb;
    
            tc.pfCallback = TaskDlgWndProc;

            SendMessage(hwndDlg, TDM_NAVIGATE_PAGE, NULL, &tc);

            PhDereferenceObject(summaryText);

            //return S_FALSE;
        }
        else if (result == 0)
        {
            TASKDIALOGCONFIG tc = { 0 };
    
            PPH_STRING sText = PhFormatString(L"You're running the latest version: %u.%u", localMajorVersion, localMinorVersion);
            
            tc.cbSize = sizeof(tc);
            tc.hwndParent = PhMainWndHandle;
            tc.hInstance = PhLibImageBase;
            tc.dwFlags = TDF_ALLOW_DIALOG_CANCELLATION | TDF_USE_COMMAND_LINKS | TDF_POSITION_RELATIVE_TO_WINDOW | TDF_CALLBACK_TIMER;
            tc.dwCommonButtons = TDCBF_CLOSE_BUTTON;
            tc.pszWindowTitle = L"Process Hacker Updater";
            tc.pszMainInstruction = sText->Buffer;
            tc.pszMainIcon = MAKEINTRESOURCEW(SecuritySuccess);

            SendMessage(hwndDlg, TDM_NAVIGATE_PAGE, NULL, &tc);

            
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
               
            // TaskDialog icons
            tc.pszMainIcon = MAKEINTRESOURCEW(SecuritySuccess);

            // TaskDialog strings
            tc.pszWindowTitle = L"Process Hacker Updater";
            tc.pszMainInstruction = summaryText->Buffer;
            tc.pszContent = verText->Buffer;

            SendMessage(hwndDlg, TDM_NAVIGATE_PAGE, NULL, &tc);

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
        return;
    }

    DisposeConnection();
      
    PhUpdaterState = Downloading;
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

    if (!ConnectionAvailable())
        return;

    if (!InitializeConnection(DOWNLOAD_SERVER, uriPath->Buffer))
    {
        PhDereferenceObject(uriPath);
        return;
    }

    Updater_SetStatusText(hwndDlg, L"Connecting");

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
        PhInitializeHash(&hashContext, HashAlgorithm);

        if (HttpQueryInfo(NetRequest, HTTP_QUERY_CONTENT_LENGTH | HTTP_QUERY_FLAG_NUMBER, &dwContentLen, &dwBufLen, 0))
        {
            // Reset Progressbar state.
            PhSetWindowStyle(hwndProgress, PBS_MARQUEE, 0);

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
                RtlZeroMemory(buffer, dwBytesRead);

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
                if (dwTimeTaken > 200)
                {
                    ULONG64 kbPerSecond = (ULONG64)(((double)(dwTotalReadSize) - (double)(dwLastTotalBytes)) / ((double)(dwTimeTaken)) * 1024);
        
                    // Set last counters for the next loop.
                    dwLastTicks = dwNowTicks;
                    dwLastTotalBytes = dwTotalReadSize;

                    // Update the estimated time left, progress and download speed.
                    {
                        ULONG64 dwSecondsLeft =  (ULONG64)(((double)dwNowTicks - dwStartTicks) / dwTotalReadSize * (dwContentLen - dwTotalReadSize) / 1000);

                        // Calculate the percentage of our total bytes downloaded per the length.
                        INT dlProgress = (int)(((double)dwTotalReadSize / (double)dwContentLen) * 100);

                        PPH_STRING dlCurrent = PhFormatSize(dwTotalReadSize, -1);
                        PPH_STRING dlLength = PhFormatSize(dwContentLen, -1);
                        PPH_STRING sRemaningBytes = PhFormatSize(dwContentLen - dwLastTotalBytes, -1);
                        PPH_STRING sDlSpeed = PhFormatSize(kbPerSecond, -1);
 
                        PPH_STRING sText = PhFormatString(
                            L"Speed: %s/s \r\nDownloaded: %s of %s (%d%%)\r\nRemaning: %s (%ds)", 
                            sDlSpeed->Buffer, dlCurrent->Buffer, dlLength->Buffer, dlProgress, sRemaningBytes->Buffer, dwSecondsLeft
                            );

                        PostMessage(hwndDlg, TDM_SET_PROGRESS_BAR_POS, dlProgress, NULL);
                        PostMessage(hwndDlg, TDM_UPDATE_ELEMENT_TEXT, TDE_CONTENT, sText->Buffer);
 
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

        PhUpdaterState = Installing;

        //Updater_SetStatusText(hwndDlg, L"Download Complete");
        // Enable Install button before computing the hash result (user might not care about file hash result)
        //SetDlgItemText(hwndDlg, IDC_DOWNLOAD, L"Install");
        //Updater_EnableUI(hwndDlg);

        //if (!PhElevated)
            //SendMessage(GetDlgItem(hwndDlg, IDC_DOWNLOAD), BCM_SETSHIELD, 0, TRUE);

        {
            UCHAR hashBuffer[20];
            ULONG hashLength = 0;

            if (PhFinalHash(&hashContext, hashBuffer, 20, &hashLength))
            {
                // Allocate our hash string, hex the final hash result in our hashBuffer.
                PPH_STRING hexString = PhBufferToHexString(hashBuffer, hashLength);

                if (PhEqualString(hexString, RemoteHashString, TRUE))
                {
                    //Updater_SetStatusText(hwndDlg, L"Hash Verified");
                }
                else
                {
                    //if (WindowsVersion >= WINDOWS_VISTA)
                        //SendMessage(hwndProgress, PBM_SETSTATE, PBST_ERROR, 0);

                    //Updater_SetStatusText(hwndDlg, L"Hash failed");
                }

                PhDereferenceObject(hexString);
            }
            else
            {
                // Show fancy Red progressbar if hash failed on Vista and above.
                //if (WindowsVersion >= WINDOWS_VISTA)
                    //SendMessage(hwndProgress, PBM_SETSTATE, PBST_ERROR, 0);

                Updater_SetStatusText(hwndDlg, L"Hash failed");
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
    HRESULT hr;
    
    int nButton;
    int nRadioButton;
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

    // TaskDialog icons
    UpdateAvailablePage.pszMainIcon = MAKEINTRESOURCEW(SecurityWarning);

    // TaskDialog strings
    UpdateAvailablePage.pszWindowTitle = L"Process Hacker Updater";
    UpdateAvailablePage.pszMainInstruction = L"Checking for Updates...";
    //UpdateAvailablePage.pszContent = L"Checking...";
    UpdateAvailablePage.pfCallback = TaskDlgWndProc;

    hr = TaskDialogIndirect(&UpdateAvailablePage, &nButton, &nRadioButton, &fVerificationFlagChecked);
    
    if (!SUCCEEDED(hr))
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
            SendMessage(hwndDlg, TDM_SET_PROGRESS_BAR_MARQUEE, TRUE, 0);
            
            _beginthread(VistaWorkerThreadStart, 0, hwndDlg);
        }
        break;
    case TDN_DESTROYED:
        {

        }
        break; 
    case TDN_BUTTON_CLICKED:
        {
            switch(wParam)
            {
            case 1005:
                {
                    TASKDIALOGCONFIG tc = { 0 };
                    int nButton;

                    tc.cbSize = sizeof(tc);
                    tc.hwndParent = PhMainWndHandle;
                    tc.hInstance = PhLibImageBase;
                    tc.dwFlags = 
                        TDF_ALLOW_DIALOG_CANCELLATION | 
                        TDF_USE_COMMAND_LINKS | 
                        TDF_POSITION_RELATIVE_TO_WINDOW |  
                        TDF_SHOW_PROGRESS_BAR;

                    tc.dwCommonButtons = TDCBF_CLOSE_BUTTON;

                    // TaskDialog icons
                    tc.pszMainIcon = MAKEINTRESOURCEW(SecurityWarning);

                    // TaskDialog strings
                    tc.pszWindowTitle = L"Process Hacker Updater";
                    tc.pszMainInstruction = L"Downloading...";
                    tc.pszContent = L"\r\n\r\nInitializing...";
                    tc.pfCallback = TaskDlgWndProc;
                    
                    _beginthread(VistaDownloadWorkerThreadStart, 0, hwndDlg);

                    SendMessage(hwndDlg, TDM_NAVIGATE_PAGE, NULL, &tc);

                    SendMessage(hwndDlg, TDM_SET_PROGRESS_BAR_POS, 0, NULL);
                    return S_FALSE;
                }
                break;
            }
        }
        break;
    case TDN_NAVIGATED:
        {
            //return S_FALSE;
        }
        break;
    case TDN_TIMER:
        {
            
            return S_FALSE;
        }
        break;
    case TDN_HYPERLINK_CLICKED:        
        {

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
        EnableCache ? 0 : INTERNET_FLAG_PRAGMA_NOCACHE | INTERNET_FLAG_RELOAD | INTERNET_FLAG_NO_CACHE_WRITE | INTERNET_FLAG_RESYNCHRONIZE,
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