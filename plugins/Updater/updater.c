/*
* Process Hacker Update Checker -
*   main window
*
* Copyright (C) 2011-2012 dmex
* Copyright (C) 2011 wj32
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
#include <time.h>

#define WM_SHOWDIALOG (WM_APP + 150)
static PH_EVENT InitializedEvent = PH_EVENT_INIT;

static HWND UpdateDialogHandle = NULL;
static HANDLE UpdaterThreadHandle = NULL;
static HANDLE DownloadThreadHandle = NULL;
static HFONT FontHandle = NULL;

static BOOLEAN EnableCache = TRUE;
static time_t TimeStart = 0;
static time_t TimeTransferred = 0;
static DWORD dwContentLen = 0;
static DWORD dwTotalReadSize = 0;
static PH_UPDATER_STATE PhUpdaterState = Download;
static PPH_STRING SetupFilePath = NULL;

static NTSTATUS SilentUpdateCheckThreadStart(
    __in PVOID Parameter
    )
{
    ULONG localMajorVersion = 0;
    ULONG localMinorVersion = 0;
    PPH_STRING localVersion = NULL;
    UPDATER_XML_DATA xmlData = { 0 };
   
    if (!ConnectionAvailable())
        return STATUS_UNSUCCESSFUL;

    if (!QueryXmlData(&xmlData))
        return STATUS_UNSUCCESSFUL;

    localVersion = PhGetPhVersion();

#ifndef TEST_MODE
    if (!ParseVersionString(localVersion->Buffer, &localMajorVersion, &localMinorVersion))
    {
        PhDereferenceObject(localVersion);
    }
#else
    localMajorVersion = 0;
    localMinorVersion = 0;
#endif

    PhDereferenceObject(localVersion);

    if (CompareVersions(xmlData.MajorVersion, xmlData.MinorVersion, localMajorVersion, localMinorVersion) > 0)
    {
        // Don't spam the user the second they open PH, delay dialog creation for 3 seconds.
        Sleep(3 * 1000);

        ShowDialog();
    }

    return STATUS_SUCCESS;
}

static NTSTATUS WorkerThreadStart(
    __in PVOID Parameter
    )
{
    INT result = 0;
    ULONG localMajorVersion = 0;
    ULONG localMinorVersion = 0;
    PPH_STRING localVersion;
    UPDATER_XML_DATA xmlData;

    HWND hwndDlg = (HWND)Parameter;

    if (!ConnectionAvailable())
        return STATUS_UNSUCCESSFUL;

    if (!QueryXmlData(&xmlData))
        return STATUS_UNSUCCESSFUL;

    localVersion = PhGetPhVersion();

#ifndef TEST_MODE
    if (!ParseVersionString(localVersion->Buffer, &localMajorVersion, &localMinorVersion))
    {
        PhDereferenceObject(localVersion);
    }
#else
    localMajorVersion = 0;
    localMinorVersion = 0;
#endif

    result = CompareVersions(xmlData.MajorVersion, xmlData.MinorVersion, localMajorVersion, localMinorVersion);

    if (result > 0)
    {
        UINT dwRetVal = 0;
        WCHAR szSummaryText[MAX_PATH]; 
        WCHAR szReleaseText[MAX_PATH];
        WCHAR szSizeText[MAX_PATH]; 
        WCHAR szFileName[MAX_PATH];

        // Set the header text
        swprintf_s(
            szSummaryText, 
            ARRAYSIZE(szSummaryText), 
            L"Process Hacker %u.%u", 
            xmlData.MajorVersion, 
            xmlData.MinorVersion
            );

        //Release text
        swprintf_s(
            szReleaseText, 
            ARRAYSIZE(szReleaseText), 
            L"Released: %s", 
            xmlData.RelDate
            );

        //Size text
        swprintf_s(
            szSizeText, 
            ARRAYSIZE(szSizeText), 
            L"Size: %s", 
            xmlData.Size
            );

        // setup download filename
        swprintf_s(
            szFileName, 
            ARRAYSIZE(szFileName), 
            L"processhacker-%u.%u-setup.exe", 
            xmlData.MajorVersion, 
            xmlData.MinorVersion
            );

        SetDlgItemText(hwndDlg, IDC_MESSAGE, szSummaryText);
        SetDlgItemText(hwndDlg, IDC_RELDATE, szReleaseText);
        SetDlgItemText(hwndDlg, IDC_DLSIZE, szSizeText);

        // Enable the download button.
        Button_Enable(GetDlgItem(hwndDlg, IDC_DOWNLOAD), TRUE);
        // Use the Scrollbar macro to enable the other controls.
        ScrollBar_Show(GetDlgItem(hwndDlg, IDC_PROGRESS), TRUE);
        ScrollBar_Show(GetDlgItem(hwndDlg, IDC_RELDATE), TRUE);
        ScrollBar_Show(GetDlgItem(hwndDlg, IDC_DLSIZE), TRUE);

        PhUpdaterState = Download;
    }
    else if (result == 0)
    {
        WCHAR szSummaryText[MAX_PATH];
        WCHAR szStableText[MAX_PATH];
        //WCHAR szReleaseText[MAX_PATH];

        swprintf_s(
            szSummaryText, 
            ARRAYSIZE(szSummaryText), 
            L"No updates available"
            );

        swprintf_s(
            szStableText, 
            ARRAYSIZE(szStableText), 
            L"You're running the latest stable version: %u.%u", 
            xmlData.MajorVersion, 
            xmlData.MinorVersion
            );

        //swprintf_s(
        //    szReleaseText, 
        //    ARRAYSIZE(szReleaseText), 
        //    L"",//L"Released: %s", 
        //    NULL//xmlData.RelDate
        //    );
            
        SetDlgItemText(hwndDlg, IDC_MESSAGE, szSummaryText);
        SetDlgItemText(hwndDlg, IDC_RELDATE, szStableText);
        //Edit_SetText(GetDlgItem(hwndDlg, IDC_DLSIZE), szReleaseText);
    }
    else if (result < 0)
    {
        WCHAR szSummaryText[MAX_PATH];
        WCHAR szStableText[MAX_PATH];
        //WCHAR szReleaseText[MAX_PATH];

        swprintf_s(
            szSummaryText, 
            ARRAYSIZE(szSummaryText), 
            L"No updates available"
            );

        swprintf_s(
            szStableText, 
            ARRAYSIZE(szStableText), 
            L"You're running SVN build: v%s", 
            localVersion->Buffer
            );

        //swprintf_s(
        //    szReleaseText, 
        //    ARRAYSIZE(szReleaseText), 
        //    L"Released: %s", 
        //    xmlData.RelDate
        //    );
            
        SetDlgItemText(hwndDlg, IDC_MESSAGE, szSummaryText);
        SetDlgItemText(hwndDlg, IDC_RELDATE, szStableText);
        //Edit_SetText(GetDlgItem(hwndDlg, IDC_DLSIZE), szReleaseText);
    }

    PhDereferenceObject(localVersion);

    return STATUS_SUCCESS;
}

static NTSTATUS DownloadWorkerThreadStart(
    __in PVOID Parameter
    )
{
    WCHAR szDownloadPath[MAX_PATH];
    DWORD dwBytesRead = 0;
    DWORD dwBytesWritten = 0;
    DWORD dwBufLen = sizeof(dwContentLen);

    HANDLE tempFileHandle = NULL;
    HINTERNET hInitialize = NULL;
    HINTERNET hConnection = NULL;
    HINTERNET hRequest = NULL;

    PH_HASH_CONTEXT hashContext = { 0 };
    UPDATER_XML_DATA xmlData = { 0 };

    HWND hwndDlg = (HWND)Parameter;

    Button_Enable(GetDlgItem(hwndDlg, IDC_DOWNLOAD), FALSE);
    SetDlgItemText(hwndDlg, IDC_DLSIZE, L"Initializing");

    // Reset the progress state on Vista and above.
    if (WindowsVersion > WINDOWS_XP)
        SendDlgItemMessage(hwndDlg, IDC_PROGRESS, PBM_SETSTATE, PBST_NORMAL, 0L);

    // Reset our variables
    TimeStart = 0;
    TimeTransferred = 0;
    dwContentLen = 0;
    dwTotalReadSize = 0; 

    if (!ConnectionAvailable())
        return STATUS_UNSUCCESSFUL;

    if (!QueryXmlData(&xmlData))
        return STATUS_UNSUCCESSFUL;
                        
    // create the download path string.
    swprintf_s(
        szDownloadPath, 
        ARRAYSIZE(szDownloadPath), 
        L"/projects/processhacker/files/processhacker2/processhacker-%u.%u-setup.exe/download?use_mirror=autoselect", /* ?use_mirror=waix" */
        xmlData.MajorVersion, 
        xmlData.MinorVersion
        );
    {
        // Get temp dir.   
        WCHAR szTempDir[MAX_PATH]; 
        WCHAR szTempPathFileName[MAX_PATH];

        if (GetTempPath(ARRAYSIZE(szTempDir), szTempDir) == 0)   
            goto CleanupAndExit;   

        // Append the tempath to our string: %TEMP%processhacker-%u.%u-setup.exe
        // Example: C:\\Users\\dmex\\AppData\\Temp\\processhacker-%u.%u-setup.exe
        swprintf_s(
            szTempPathFileName, 
            ARRAYSIZE(szTempPathFileName), 
            L"%sprocesshacker-%u.%u-setup.exe",
            szTempDir,
            xmlData.MajorVersion, 
            xmlData.MinorVersion
            );

        // Create output file 
        if ((tempFileHandle = CreateFile(
            szTempPathFileName,   
            GENERIC_READ | GENERIC_WRITE,   
            FILE_SHARE_READ | FILE_SHARE_WRITE,         
            NULL,
            CREATE_ALWAYS,      
            FILE_ATTRIBUTE_NOT_CONTENT_INDEXED | FILE_ATTRIBUTE_TEMPORARY,   
            NULL
            )) == INVALID_HANDLE_VALUE)
        {
            LogEvent(NULL, PhFormatString(L"CreateFile failed (%d)", GetLastError()));
            goto CleanupAndExit;
        }
    
        SetupFilePath = PhCreateString(szTempPathFileName);
    }

    // Initialize the wininet library.
    if (!(hInitialize = InternetOpenW(
        L"PHUpdater",
        INTERNET_OPEN_TYPE_PRECONFIG,
        NULL, 
        NULL, 
        0
        )))
    {
        LogEvent(hwndDlg, PhFormatString(L"Updater: (InitializeConnection) InternetOpen failed (%d)", GetLastError()));
        goto CleanupAndExit;
    }

    // Connect to the server.
    if (!(hConnection = InternetConnect(
        hInitialize, 
        DOWNLOAD_SERVER, //L"mirror.internode.on.net",
        INTERNET_DEFAULT_HTTP_PORT, 
        NULL, 
        NULL, 
        INTERNET_SERVICE_HTTP, 
        0, 
        0)))
    {
        LogEvent(NULL, PhFormatString(L"Updater: (InitializeConnection) InternetConnect failed (%d)", GetLastError()));
        goto CleanupAndExit;
    }

    // Open the HTTP request.
    if (!(hRequest = HttpOpenRequest(
        hConnection,
        NULL,
        szDownloadPath,//L"/pub/ubuntu/releases/oneiric/ubuntu-11.10-desktop-i386.iso",
        L"HTTP/1.1",
        NULL,
        NULL,
        EnableCache ? 0 : INTERNET_FLAG_PRAGMA_NOCACHE | INTERNET_FLAG_RELOAD | INTERNET_FLAG_NO_CACHE_WRITE | INTERNET_FLAG_RESYNCHRONIZE,
        0
        )))
    {
        LogEvent(hwndDlg, PhFormatString(L"Updater: (InitializeConnection) HttpOpenRequest failed (%d)", GetLastError()));
        goto CleanupAndExit;
    }

    Edit_SetText(GetDlgItem(hwndDlg, IDC_DLSIZE), L"Connecting");

    // Send the HTTP request.
    if (!HttpSendRequest(hRequest, NULL, 0, NULL, 0))
    {	
        LogEvent(hwndDlg, PhFormatString(L"HttpSendRequest failed (%d)", GetLastError()));

        // Enable the 'Retry' button.
        Button_Enable(GetDlgItem(hwndDlg, IDC_DOWNLOAD), TRUE);
        SetDlgItemText(hwndDlg, IDC_DOWNLOAD, L"Retry");

        // Reset the state and let user retry the download.
        PhUpdaterState = Download;
    }
    else
    { 
        CHAR buffer[BUFFER_LEN];
        DWORD dwLastTotalBytes = 0;
        BOOL nReadFile = FALSE;

        // Zero the buffer.
        ZeroMemory(buffer, BUFFER_LEN);

        // Initialize hash algorithm.
        PhInitializeHash(&hashContext, Sha1HashAlgorithm);
 
        if (!HttpQueryInfoW(hRequest, HTTP_QUERY_CONTENT_LENGTH | HTTP_QUERY_FLAG_NUMBER, &dwContentLen, &dwBufLen, 0))
        {
            // No content length...impossible to calculate % complete so just read until we are done.
            LogEvent(hwndDlg, PhFormatString(L"Updater: (DownloadWorkerThreadStart) HttpQueryInfo failed (%d)", GetLastError()));

            while ((nReadFile = InternetReadFile(hRequest, buffer, BUFFER_LEN, &dwBytesRead)))
            {
                if (dwBytesRead == 0)
                    break;

                if (!nReadFile)
                {
                    LogEvent(hwndDlg, PhFormatString(L"InternetReadFile failed (%d)", GetLastError()));
                    break;
                }

                PhUpdateHash(&hashContext, buffer, dwBytesRead);

                if (!WriteFile(tempFileHandle, buffer, dwBytesRead, &dwBytesWritten, NULL))
                {
                    LogEvent(hwndDlg, PhFormatString(L"WriteFile failed (%d)", GetLastError()));
                    break;
                }

                // Reset the buffer.
                ZeroMemory(buffer, BUFFER_LEN);

                if (dwBytesRead != dwBytesWritten)
                {
                    LogEvent(hwndDlg, PhFormatString(L"WriteFile failed (%d)", GetLastError()));
                    break;
                }
            }
        }
        else
        {
            // start the clock.
            TimeStart = time(NULL);
            TimeTransferred = TimeStart;

            // Start our download status timer                                          
            SetTimer(hwndDlg, 0, 500, NULL);

            while ((nReadFile = InternetReadFile(hRequest, buffer, BUFFER_LEN, &dwBytesRead)))
            {
                if (dwBytesRead == 0)
                    break;

                if (!nReadFile)
                {
                    LogEvent(hwndDlg, PhFormatString(L"InternetReadFile failed (%d)", GetLastError()));
                    break;
                }

                // Update the hash of bytes we downloaded.
                PhUpdateHash(&hashContext, buffer, dwBytesRead);

                // Write the downloaded bytes to disk.
                if (!WriteFile(tempFileHandle, buffer, dwBytesRead, &dwBytesWritten, NULL))
                {
                    LogEvent(hwndDlg, PhFormatString(L"WriteFile failed (%d)", GetLastError()));
                    break;
                }

                // Zero the buffer.
                ZeroMemory(buffer, dwBytesRead);

                // Check dwBytesRead are the same dwBytesWritten length returned by WriteFile.
                if (dwBytesRead != dwBytesWritten)
                {
                    LogEvent(hwndDlg, PhFormatString(L"WriteFile failed (%d)", GetLastError()));
                    break;
                }

                // Update our total bytes downloaded
                dwTotalReadSize += dwBytesRead;                     
            }
        
            // Dispose timer
            KillTimer(hwndDlg, 0);

            // Invoke the timer handler to update our final download status.
            SendMessage(hwndDlg, WM_TIMER, 0L, 0L);
        }

        {        
            UCHAR hashBuffer[20];
            ULONG hashLength = 0;

            // Compute our hash result.
            if (PhFinalHash(&hashContext, hashBuffer, 20, &hashLength))
            {
                // Allocate our hash string, hex the final hash result in our hashBuffer.
                PPH_STRING hexString = PhBufferToHexString(hashBuffer, hashLength);

                if (!_wcsicmp(hexString->Buffer, xmlData.Hash))
                {
                    // If PH is not elevated, set the UAC sheild for the installer.
                    if (!PhElevated)
                        SendMessage(GetDlgItem(hwndDlg, IDC_DOWNLOAD), BCM_SETSHIELD, 0, TRUE);

                    // Set the download result, don't include hash status since it succeeded.
                    Edit_SetText(GetDlgItem(hwndDlg, IDC_DLSIZE), L"Download Complete");
                    // Set button text for next action
                    Button_SetText(GetDlgItem(hwndDlg, IDC_DOWNLOAD), L"Install");
                    // Enable the Install button
                    Button_Enable(GetDlgItem(hwndDlg, IDC_DOWNLOAD), TRUE);
                    // Hash succeeded, set state as ready to install.
                    PhUpdaterState = Install;
                }
                else
                {         
                    if (WindowsVersion > WINDOWS_XP)
                        SendDlgItemMessage(hwndDlg, IDC_PROGRESS, PBM_SETSTATE, PBST_ERROR, 0L);

                    Edit_SetText(GetDlgItem(hwndDlg, IDC_DLSIZE), L"Download complete, SHA1 Hash failed.");

                    // Set button text for next action
                    Button_SetText(GetDlgItem(hwndDlg, IDC_DOWNLOAD), L"Retry");
                    // Enable the Install button
                    Button_Enable(GetDlgItem(hwndDlg, IDC_DOWNLOAD), TRUE);
                    // Hash failed, set state to downloading so user can redownload the file.
                    PhUpdaterState = Download;
                }

                PhDereferenceObject(hexString);
            }
            else
            {
                Edit_SetText(GetDlgItem(hwndDlg, IDC_DLSIZE), L"PhFinalHash failed");

                // Show fancy Red progressbar if hash failed on Vista and above.
                if (WindowsVersion > WINDOWS_XP)
                    SendDlgItemMessage(hwndDlg, IDC_PROGRESS,  PBM_SETSTATE, PBST_ERROR, 0L);			
            }
        }       
    }

CleanupAndExit:
    if (hInitialize)
        InternetCloseHandle(hInitialize);

    if (hConnection)
        InternetCloseHandle(hConnection);

    if (hRequest)
        InternetCloseHandle(hRequest);

    if (tempFileHandle)
        NtClose(tempFileHandle);

    //return STATUS_UNSUCCESSFUL;
    return STATUS_SUCCESS;
}

INT_PTR CALLBACK UpdaterWndProc(
    __in HWND hwndDlg,
    __in UINT uMsg,
    __in WPARAM wParam,
    __in LPARAM lParam
    )
{
    switch (uMsg)
    {
    case WM_INITDIALOG:
        {
            LOGFONT lHeaderFont = { 0 };

            // load the PH main icon using the 'magic' resource id.
            HANDLE hPhIcon = LoadImageW(
                GetModuleHandle(NULL), 
                MAKEINTRESOURCE(PHAPP_IDI_PROCESSHACKER), 
                IMAGE_ICON, 
                GetSystemMetrics(SM_CXICON), 
                GetSystemMetrics(SM_CYICON),	
                LR_SHARED
                );

            // Set our initial state as download
            PhUpdaterState = Download;

            // Set the window icon.
            SendMessageW(hwndDlg, WM_SETICON, ICON_BIG, (LPARAM)hPhIcon);
        
            EnableCache = (PhGetIntegerSetting(L"ProcessHacker.Updater.EnableCache") == 1 ? TRUE : FALSE);

            lHeaderFont.lfHeight = -15;
            lHeaderFont.lfWeight = FW_MEDIUM;
            lHeaderFont.lfQuality = CLEARTYPE_QUALITY | ANTIALIASED_QUALITY;
            // We don't check if Segoe exists, CreateFontIndirect does this for us.
            wcscat_s(lHeaderFont.lfFaceName, ARRAYSIZE(lHeaderFont.lfFaceName), L"Segoe UI");          
            FontHandle = CreateFontIndirectW(&lHeaderFont);
            // Set the header font.
            SendMessageW(GetDlgItem(hwndDlg, IDC_MESSAGE), WM_SETFONT, (WPARAM)FontHandle, FALSE);
 
            // Center the update window on PH if visible and not mimimized else center on desktop.
            PhCenterWindow(hwndDlg, (IsWindowVisible(GetParent(hwndDlg)) && !IsIconic(GetParent(hwndDlg))) ? GetParent(hwndDlg) : NULL);

            // Create our update check thread.
            PhCreateThread(0, WorkerThreadStart, hwndDlg);
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

            // return stock White color as our window background.
            return (INT_PTR)(HBRUSH)GetStockObject(WHITE_BRUSH);
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
                    switch (PhUpdaterState)
                    {
                    case Download:
                        {
                            if (PhInstalledUsingSetup())
                            {
                                // Start our Downloader thread   
                                DownloadThreadHandle = PhCreateThread(0, DownloadWorkerThreadStart, hwndDlg);
                            }
                            else
                            {
                                // Let the user handle non-setup installation, show the homepage and close this dialog.
                                PhShellExecute(hwndDlg, L"http://processhacker.sourceforge.net/downloads.php", NULL);

                                PostQuitMessage(0);
                            }
                        }
                        break;
                    case Install:
                        {
                            SHELLEXECUTEINFO info = { sizeof(SHELLEXECUTEINFO) };
                            info.lpFile = SetupFilePath->Buffer;
                            info.lpVerb = L"runas";
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
                    }
                }
                break;
            }
            break;  
        }
        break;
    case WM_TIMER:
        {
            WCHAR szDownloaded[1024] = L"";
            WCHAR *rtext = L"second";
            time_t time_taken = (time(NULL) - TimeTransferred);
            time_t bps = dwTotalReadSize / (time_taken ? time_taken : 1);
            time_t remain = (MulDiv(time_taken, dwContentLen, dwTotalReadSize) - time_taken);
                   
            PPH_STRING dlRemaningBytes = PhFormatSize(dwTotalReadSize, -1);
            PPH_STRING dlLength = PhFormatSize(dwContentLen, -1);
            PPH_STRING dlSpeed = PhFormatSize(bps, -1); 

            if (remain < 0) 
                remain = 0;
            if (remain >= 60)
            {
                remain /= 60;
                rtext = L"minute";
                if (remain >= 60)
                {
                    remain /= 60;
                    rtext = L"hour";
                }
            }

            wsprintfW(
                szDownloaded,
                L"%s (%d%%) of %s @ %s/s",
                dlRemaningBytes->Buffer,
                MulDiv(100, dwTotalReadSize, dwContentLen),
                dlLength->Buffer,
                dlSpeed->Buffer
                );

            if (remain) 
            {
                wsprintfW(
                    szDownloaded + lstrlenW(szDownloaded),
                    L" (%d %s%s remaining)",
                    (DWORD)remain, // Cast and silence code analysis.
                    rtext,
                    remain == 1? L"": L"s"
                    );
            }

            Edit_SetText(GetDlgItem(hwndDlg, IDC_DLSIZE), szDownloaded);

            PhDereferenceObject(dlSpeed);
            PhDereferenceObject(dlLength);
            PhDereferenceObject(dlRemaningBytes);

            // Update the progress bar position
            SendDlgItemMessage(hwndDlg, IDC_PROGRESS, PBM_SETPOS, MulDiv(100, dwTotalReadSize, dwContentLen), 0);  
        }
        break;
    }

    return FALSE;
}

LONG CompareVersions(
    __in ULONG MajorVersion1,
    __in ULONG MinorVersion1,
    __in ULONG MajorVersion2,
    __in ULONG MinorVersion2
    )
{
    LONG result;

    result = intcmp(MajorVersion1, MajorVersion2);

    if (result == 0)
        result = intcmp(MinorVersion1, MinorVersion2);

    return result;
}

BOOL ConnectionAvailable(
    VOID
    )
{
    if (WindowsVersion > WINDOWS_XP)
    {
        HRESULT hrResult = S_OK;
        INetworkListManager *pNetworkListManager; 

        // Create an instance of the CLSID_NetworkListManger COM object.
        hrResult = CoCreateInstance(&CLSID_NetworkListManager, NULL, CLSCTX_INPROC, &IID_INetworkListManager, (void**)&pNetworkListManager);

        if (SUCCEEDED(hrResult))
        {
            VARIANT_BOOL vIsConnected = VARIANT_FALSE;
            VARIANT_BOOL vIsConnectedInternet = VARIANT_FALSE;

            INetworkListManager_get_IsConnected(pNetworkListManager, &vIsConnected);
            INetworkListManager_get_IsConnectedToInternet(pNetworkListManager, &vIsConnectedInternet);

            if (vIsConnected == VARIANT_TRUE && vIsConnectedInternet == VARIANT_TRUE)
            {
                return TRUE;
            }

            INetworkListManager_Release(pNetworkListManager);
            pNetworkListManager = NULL;
        }
        else
        {
            LogEvent(NULL, PhFormatString(L"NetworkListManager\\CoCreateInstance Failed (0x%x) \n", hrResult));
            goto NOT_SUPPORTED;
        }
    }
    else  
NOT_SUPPORTED:
    {
        DWORD dwType;

        if (InternetGetConnectedState(&dwType, 0))
        {
            return TRUE;
        }
        else
        {
            LogEvent(NULL, PhFormatString(L"Updater: (ConnectionAvailable) InternetGetConnectedState failed to detect an active Internet connection (%d)", GetLastError()));
        }
    }

    //if (!InternetCheckConnection(NULL, FLAG_ICC_FORCE_CONNECTION, 0))
    //{
    //  LogEvent(PhFormatString(L"Updater: (ConnectionAvailable) InternetCheckConnection failed connection to Sourceforge.net (%d)", GetLastError()));
    //  return FALSE;
    //}

    return FALSE;
}

VOID LogEvent(
    __in_opt HWND hwndDlg,
    __in PPH_STRING str
    )
{
    if (hwndDlg)
    {
        Edit_SetText(GetDlgItem(hwndDlg, IDC_MESSAGE), str->Buffer);
    }
    else
    {
        PhLogMessageEntry(PH_LOG_ENTRY_MESSAGE, str);
    }

    PhDereferenceObject(str);
}

BOOL ParseVersionString(
    __in PWSTR String,
    __out PULONG MajorVersion,
    __out PULONG MinorVersion
    )
{
    PH_STRINGREF sr, majorPart, minorPart;
    ULONG64 majorInteger = 0, minorInteger = 0;

    PhInitializeStringRef(&sr, String);

    if (PhSplitStringRefAtChar(&sr, '.', &majorPart, &minorPart))
    {
        PhStringToInteger64(&majorPart, 10, &majorInteger);
        PhStringToInteger64(&minorPart, 10, &minorInteger);

        *MajorVersion = (ULONG)majorInteger;
        *MinorVersion = (ULONG)minorInteger;

        return TRUE;
    }

    return FALSE;
}

BOOL PhInstalledUsingSetup(
    VOID
    )
{
    PH_STRINGREF keyName = PH_STRINGREF_INIT(L"Software\\Microsoft\\Windows\\CurrentVersion\\Uninstall\\Process_Hacker2_is1");

    NTSTATUS status;
    HANDLE keyHandle;

    // Check uninstall entries for the 'Process_Hacker2_is1' registry key.
    if (NT_SUCCESS(status = PhOpenKey(
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

    LogEvent(NULL, PhFormatString(L"Updater: (PhInstalledUsingSetup) PhOpenKey failed (0x%x)", status));
    return FALSE;
}

BOOL QueryXmlData(
    __out PUPDATER_XML_DATA XmlData
    )
{
    PCHAR data = NULL;
    BOOL result = FALSE;
    HINTERNET netInitialize = NULL, netConnection = NULL, netRequest = NULL;
    mxml_node_t *xmlDoc = NULL, *xmlNodeVer = NULL, *xmlNodeRelDate = NULL, *xmlNodeSize = NULL, *xmlNodeHash = NULL;

    // Initialize the wininet library.
    if (!(netInitialize = InternetOpen(
        L"PHUpdater",
        INTERNET_OPEN_TYPE_PRECONFIG,
        NULL,
        NULL,
        0
        )))
    {
        LogEvent(NULL, PhFormatString(L"Updater: (InitializeConnection) InternetOpen failed (%d)", GetLastError()));
        goto CleanupAndExit;
    }

    // Connect to the server.
    if (!(netConnection = InternetConnect(
        netInitialize,
        UPDATE_URL,
        INTERNET_DEFAULT_HTTP_PORT,
        NULL,
        NULL,
        INTERNET_SERVICE_HTTP,
        0,
        0
        )))
    {
        LogEvent(NULL, PhFormatString(L"Updater: (InitializeConnection) InternetConnect failed (%d)", GetLastError()));
        goto CleanupAndExit;
    }

    // Open the HTTP request.
    if (!(netRequest = HttpOpenRequest(
        netConnection,
        L"GET",
        UPDATE_FILE,
        L"HTTP/1.1",
        NULL,
        NULL,
        EnableCache ? 0 : INTERNET_FLAG_PRAGMA_NOCACHE | INTERNET_FLAG_RELOAD | INTERNET_FLAG_NO_CACHE_WRITE | INTERNET_FLAG_RESYNCHRONIZE,
        0
        )))
    {
        LogEvent(NULL, PhFormatString(L"Updater: (InitializeConnection) HttpOpenRequest failed (%d)", GetLastError()));
        goto CleanupAndExit;
    }

    // Send the HTTP request.
    if (!HttpSendRequest(netRequest, NULL, 0, NULL, 0))
    {
        LogEvent(NULL, PhFormatString(L"HttpSendRequest failed (%d)", GetLastError()));
        goto CleanupAndExit;
    }

    // Read the resulting xml into our buffer.
    if (!ReadRequestString(netRequest, &data, NULL))
    {
        goto CleanupAndExit;
    }

    // Load our XML.
    xmlDoc = mxmlLoadString(NULL, (char*)data, QueryXmlDataCallback);
    // Check our XML.
    if (xmlDoc == NULL || xmlDoc->type != MXML_ELEMENT)
    {
        LogEvent(NULL, PhFormatString(L"Updater: (WorkerThreadStart) mxmlLoadString failed."));
        goto CleanupAndExit;
    }

    // Find the ver node.
    xmlNodeVer = mxmlFindElement(xmlDoc, xmlDoc, "ver", NULL, NULL, MXML_DESCEND);
    // Find the reldate node.
    xmlNodeRelDate = mxmlFindElement(xmlDoc, xmlDoc, "reldate", NULL, NULL, MXML_DESCEND);
    // Find the size node.
    xmlNodeSize = mxmlFindElement(xmlDoc, xmlDoc, "size", NULL, NULL, MXML_DESCEND);
    // Find the hash node.
    xmlNodeHash = mxmlFindElement(xmlDoc, xmlDoc, "sha1", NULL, NULL, MXML_DESCEND);

    if (xmlNodeVer == NULL || xmlNodeVer->child == NULL || xmlNodeVer->type != MXML_ELEMENT)
    {
        LogEvent(NULL, PhFormatString(L"Updater: (WorkerThreadStart) mxmlLoadString xmlNodeVer failed."));
        goto CleanupAndExit;
    }
    if (xmlNodeRelDate == NULL || xmlNodeRelDate->child == NULL || xmlNodeRelDate->type != MXML_ELEMENT)
    {
        LogEvent(NULL, PhFormatString(L"Updater: (WorkerThreadStart) mxmlLoadString xmlNodeRelDate failed."));
        goto CleanupAndExit;
    }
    if (xmlNodeSize == NULL || xmlNodeSize->child == NULL || xmlNodeSize->type != MXML_ELEMENT)
    {
        LogEvent(NULL, PhFormatString(L"Updater: (WorkerThreadStart) mxmlLoadString xmlNodeSize failed."));
        goto CleanupAndExit;
    }
    if (xmlNodeHash == NULL || xmlNodeHash->child == NULL || xmlNodeHash->type != MXML_ELEMENT)
    {
        LogEvent(NULL, PhFormatString(L"Updater: (WorkerThreadStart) mxmlLoadString xmlNodeHash failed."));
        goto CleanupAndExit;
    }

    {
        WCHAR szTempString[20];

        // Convert into unicode string.
        swprintf_s(
            szTempString, 
            ARRAYSIZE(szTempString), 
            L"%hs", 
            xmlNodeVer->child->value.opaque
            );

        // parse and check string
        result = ParseVersionString(szTempString, &XmlData->MajorVersion, &XmlData->MinorVersion);
    }

    if (!result)
        goto CleanupAndExit;

    swprintf_s(
        XmlData->RelDate, 
        ARRAYSIZE(XmlData->RelDate), 
        L"%hs", 
        xmlNodeRelDate->child->value.opaque
        );

    swprintf_s(
        XmlData->Size, 
        ARRAYSIZE(XmlData->Size), 
        L"%hs", 
        xmlNodeSize->child->value.opaque
        );

    swprintf_s(
        XmlData->Hash, 
        ARRAYSIZE(XmlData->RelDate), 
        L"%hs", 
        xmlNodeHash->child->value.opaque
        );

    result = TRUE;

CleanupAndExit:
    if (xmlDoc)
    {
        mxmlDelete(xmlDoc);
        xmlDoc = NULL;
    }

    if (netInitialize)
    {
        InternetCloseHandle(netInitialize);
        netInitialize = NULL;
    }

    if (netConnection)
    {
        InternetCloseHandle(netConnection);
        netConnection = NULL;
    }

    if (netRequest)
    {
        InternetCloseHandle(netRequest);
        netRequest = NULL;
    }

    return result;
}

mxml_type_t QueryXmlDataCallback(
    __in mxml_node_t *node
    )
{
    return MXML_OPAQUE;
}

BOOL ReadRequestString(
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

        RtlCopyMemory(data + dataLength, buffer, returnLength);
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

NTSTATUS ShowUpdateDialogThreadStart(
    __in PVOID Parameter
    )
{
    BOOL result;
    MSG message;
    PH_AUTO_POOL autoPool;

    PhInitializeAutoPool(&autoPool);

    UpdateDialogHandle = CreateDialog( 
        (HINSTANCE)PluginInstance->DllBase, 
        MAKEINTRESOURCE(IDD_UPDATE), 
        PhMainWndHandle, 
        UpdaterWndProc
        ); 

    PhSetEvent(&InitializedEvent);

    while (result = GetMessage(&message, NULL, 0, 0)) 
    { 
        if (result == -1)
            break;

        if (!IsWindow(UpdateDialogHandle) || !IsDialogMessage(UpdateDialogHandle, &message))
        { 
            TranslateMessage(&message); 
            DispatchMessage(&message); 
        }

        PhDrainAutoPool(&autoPool);
    }

    PhDeleteAutoPool(&autoPool);
    PhResetEvent(&InitializedEvent);

    // Ensure objects are reset when window closes.

    if (SetupFilePath)
    {
        PhDereferenceObject(SetupFilePath);
        SetupFilePath = NULL;
    }

    NtClose(DownloadThreadHandle);
    DownloadThreadHandle = NULL;

    NtClose(UpdaterThreadHandle);
    UpdaterThreadHandle = NULL;

    DeleteObject(FontHandle);
    FontHandle = NULL;

    DestroyWindow(UpdateDialogHandle);
    UpdateDialogHandle = NULL;

    return STATUS_SUCCESS;
}

VOID ShowDialog(
    VOID
    )
{
    if (!UpdaterThreadHandle)
    {
        if (!(UpdaterThreadHandle = PhCreateThread(0, ShowUpdateDialogThreadStart, NULL)))
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
    // Queue up our initial update check.
    PhCreateThread(0, SilentUpdateCheckThreadStart, NULL);
}