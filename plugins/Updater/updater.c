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
#include <phappresource.h>

static BOOLEAN EnableCache = TRUE;
static PH_UPDATER_STATE PhUpdaterState = Default;
static PH_HASH_ALGORITHM HashAlgorithm = Sha1HashAlgorithm;

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

        PhCreateThread(0, ShowUpdateDialogThreadStart, NULL);
    }

    return STATUS_SUCCESS;
}

static NTSTATUS WorkerThreadStart(
    __in PVOID Parameter
    )
{
    PPH_STRING localVersion;
    INT result = 0;
    ULONG localMajorVersion = 0, localMinorVersion = 0;
    HWND hwndDlg = (HWND)Parameter;
    UPDATER_XML_DATA xmlData;

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

    //RemoteHashString = PhCreateString(xmlData.Hash);
    PhDereferenceObject(localVersion);

    if (result > 0)
    {
        UINT dwRetVal = 0;
        WCHAR szSummaryText[MAX_PATH], 
              szReleaseText[MAX_PATH], 
              szSizeText[MAX_PATH], 
              szFileName[MAX_PATH];

        // Set the header text
        swprintf_s(szSummaryText, ARRAYSIZE(szSummaryText), L"Process Hacker %u.%u", xmlData.MajorVersion, xmlData.MinorVersion);
        //Release text
        swprintf_s(szReleaseText, ARRAYSIZE(szReleaseText), L"Released: %s", xmlData.RelDate);
        //Size text
        swprintf_s(szSizeText, ARRAYSIZE(szSizeText), L"Size: %s", xmlData.Size);
        // filename
        swprintf_s(szFileName, ARRAYSIZE(szFileName), L"processhacker-%u.%u-setup.exe", xmlData.MajorVersion, xmlData.MinorVersion);

        Edit_SetText(GetDlgItem(hwndDlg, IDC_MESSAGE), szSummaryText);
        Edit_SetText(GetDlgItem(hwndDlg, IDC_RELDATE), szReleaseText);
        Edit_SetText(GetDlgItem(hwndDlg, IDC_DLSIZE), szSizeText);

        // Enable the download button.
        Button_Enable(GetDlgItem(hwndDlg, IDC_DOWNLOAD), TRUE);

        PhUpdaterState = Downloading;
    }
    else if (result == 0)
    {
        WCHAR szSummaryText[MAX_PATH];

        swprintf_s(
            szSummaryText, 
            ARRAYSIZE(szSummaryText), 
            L"You're running the latest version: %u.%u", 
            xmlData.MajorVersion, 
            xmlData.MinorVersion
            );

        Edit_SetText(GetDlgItem(hwndDlg, IDC_MESSAGE), szSummaryText);
    }
    else if (result < 0)
    {
        WCHAR szSummaryText[MAX_PATH];
        WCHAR szStableText[MAX_PATH];
        WCHAR szReleaseText[MAX_PATH];

        swprintf_s(
            szSummaryText, 
            ARRAYSIZE(szSummaryText), 
            L"You're running a newer version: %u.%u", 
            localMajorVersion, 
            localMinorVersion
            );

        swprintf_s(
            szStableText, 
            ARRAYSIZE(szStableText), 
            L"Latest stable version: %u.%u", 
            xmlData.MajorVersion, 
            xmlData.MinorVersion
            );

        swprintf_s(
            szReleaseText, 
            ARRAYSIZE(szReleaseText), 
            L"Released: %s", 
            xmlData.RelDate
            );
            
        Edit_SetText(GetDlgItem(hwndDlg, IDC_MESSAGE), szSummaryText);
        Edit_SetText(GetDlgItem(hwndDlg, IDC_RELDATE), szStableText);
        Edit_SetText(GetDlgItem(hwndDlg, IDC_DLSIZE), szReleaseText);
    }

    return STATUS_SUCCESS;
}

static NTSTATUS DownloadWorkerThreadStart(
    __in PVOID Parameter
    )
{
    WCHAR szDownloadPath[MAX_PATH];
    DWORD dwTotalReadSize = 0,
        dwContentLen = 0,
        dwBytesRead = 0,
        dwBytesWritten = 0,
        dwBufLen = sizeof(dwContentLen);

    HWND hwndDlg = (HWND)Parameter, hwndProgress = GetDlgItem(hwndDlg, IDC_PROGRESS);
    
    HINTERNET hInitialize = NULL, hConnection = NULL, hRequest = NULL;

    PH_HASH_CONTEXT hashContext;
    UPDATER_XML_DATA xmlData;
    HANDLE TempFileHandle;
        
    Button_Enable(GetDlgItem(hwndDlg, IDC_DOWNLOAD), FALSE);
    Edit_SetText(GetDlgItem(hwndDlg, IDC_DLSIZE), L"Initializing");

    // Reset the progress state on Vista and above.
    if (WindowsVersion > WINDOWS_XP)
        SendMessage(GetDlgItem(hwndDlg, IDC_PROGRESS), PBM_SETSTATE, PBST_NORMAL, 0);

    if (!ConnectionAvailable())
        return STATUS_UNSUCCESSFUL;

    if (!QueryXmlData(&xmlData))
        return STATUS_UNSUCCESSFUL;

    // create the download path string.
    swprintf_s(
        szDownloadPath, 
        ARRAYSIZE(szDownloadPath), 
        L"/projects/processhacker/files/processhacker2/processhacker-%u.%u-setup.exe/download", /* ?use_mirror=waix" */
        xmlData.MajorVersion, 
        xmlData.MinorVersion
        );

    {
        WCHAR szTempPath[MAX_PATH];
        WCHAR szTempPathFileName[MAX_PATH];

        // Query the temp path (no guarantee it's a valid path).
        UINT dwRetVal = GetTempPathW(MAX_PATH, szTempPath);
        if (dwRetVal > MAX_PATH || dwRetVal == 0)
        {
            LogEvent(NULL, PhFormatString(L"GetTempPath failed (%d)", GetLastError()));
            return STATUS_UNSUCCESSFUL;
        }	

        // Append the tempath to our string: %TEMP%processhacker-%u.%u-setup.exe
        // Example: C:\\Users\\user\\AppData\\Temp\\processhacker-%u.%u-setup.exe
        swprintf_s(
            szTempPathFileName, 
            ARRAYSIZE(szTempPathFileName), 
            L"%sprocesshacker-%u.%u-setup.exe", /* ?use_mirror=waix" */
            szTempPath,
            xmlData.MajorVersion, 
            xmlData.MinorVersion
            );

        // Create output file
        if ((TempFileHandle = CreateFile(
            szTempPathFileName,
            GENERIC_WRITE,
            FILE_SHARE_WRITE,
            0,                     // handle cannot be inherited
            CREATE_ALWAYS,         // if file exists, delete it
            FILE_ATTRIBUTE_NORMAL,
            0
            )) == INVALID_HANDLE_VALUE)
        {
            LogEvent(hwndDlg, PhFormatString(L"CreateFile failed (%d)", GetLastError()));
            goto CleanupAndExit;
        }
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
        LogEvent(NULL, PhFormatString(L"Updater: (InitializeConnection) InternetOpen failed (%d)", GetLastError()));
        goto CleanupAndExit;
    }

    // Connect to the server.
    if (!(hConnection = InternetConnect(
        hInitialize, 
        DOWNLOAD_SERVER, 
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
        L"GET",
        szDownloadPath,
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
    if (!HttpSendRequest(hRequest, NULL, 0, NULL, 0))
    {
        LogEvent(hwndDlg, PhFormatString(L"HttpSendRequest failed (%d)", GetLastError()));
        goto CleanupAndExit;
    }

    Edit_SetText(GetDlgItem(hwndDlg, IDC_DLSIZE), L"Connecting");

    if (!HttpSendRequest(hRequest, NULL, 0, NULL, 0))
    {	
        LogEvent(hwndDlg, PhFormatString(L"HttpSendRequest failed (%d)", GetLastError()));

        // Enable the 'Retry' button.
        Button_Enable(GetDlgItem(hwndDlg, IDC_DOWNLOAD), TRUE);
        Button_SetText(GetDlgItem(hwndDlg, IDC_DOWNLOAD), L"Retry");

        // Reset the state and let user retry the download.
        PhUpdaterState = Retry;
    }
    else
    { 
        CHAR buffer[BUFFER_LEN];
        WCHAR szDownloaded[MAX_PATH];//, szRemaning[MAX_PATH], szSpeed[MAX_PATH];
        DWORD dwLastTotalBytes = 0, dwStartTicks = 0, dwLastTicks = 0, dwNowTicks = 0, dwTimeTaken = 0;
        PPH_STRING dlCurrent, dlLength, dlRemaningBytes; //dlSpeed,
        BOOL nReadFile = FALSE;
        LONG  iSecondsLeft, iProgress; //kbPerSecond,

        // Set our last ticks.
        dwLastTicks = (dwStartTicks = GetTickCount());

        // Zero the buffer.
        ZeroMemory(buffer, BUFFER_LEN);

        // Initialize hash algorithm.
        PhInitializeHash(&hashContext, HashAlgorithm);

        if (!HttpQueryInfoW(hRequest, HTTP_QUERY_CONTENT_LENGTH | HTTP_QUERY_FLAG_NUMBER, &dwContentLen, &dwBufLen, 0))
        {
            // No content length...impossible to calculate % complete so just read until we are done.
            LogEvent(NULL, PhFormatString(L"Updater: (DownloadWorkerThreadStart) HttpQueryInfo failed (%d)", GetLastError()));

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

                if (!WriteFile(TempFileHandle, buffer, dwBytesRead, &dwBytesWritten, NULL))
                {
                    LogEvent(hwndDlg, PhFormatString(L"WriteFile failed (%d)", GetLastError()));
                    break;
                }

                // Reset the buffer.
                RtlZeroMemory(buffer, BUFFER_LEN);

                if (dwBytesRead != dwBytesWritten)
                {
                    LogEvent(hwndDlg, PhFormatString(L"WriteFile failed (%d)", GetLastError()));
                    break;
                }
            }
        }
        else
        {
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
                if (!WriteFile(TempFileHandle, buffer, dwBytesRead, &dwBytesWritten, NULL))
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

                // Calculate the transfer rate and download speed. 
                dwNowTicks = GetTickCount();
                dwTimeTaken = dwNowTicks - dwLastTicks;   
                // Update our total bytes downloaded
                dwTotalReadSize += dwBytesRead;

                // Update the transfer rate and estimated time every 100ms.
                if (dwTimeTaken > 100)
                {
                    //kbPerSecond = (LONG)(((double)(dwTotalReadSize) - (double)(dwLastTotalBytes)) / ((double)(dwTimeTaken)) * 1024);
                    iSecondsLeft =  (LONG)(((double)dwNowTicks - dwStartTicks) / dwTotalReadSize * (dwContentLen - dwTotalReadSize) / 1000);
                    iProgress = (int)(((double)dwTotalReadSize / (double)dwContentLen) * 100);

                    dlCurrent = PhFormatSize(dwTotalReadSize, -1);
                    dlLength = PhFormatSize(dwContentLen, -1);
                    //dlSpeed = PhFormatSize(kbPerSecond, -1);
                    dlRemaningBytes = PhFormatSize(dwContentLen - dwLastTotalBytes, -1);

                    swprintf_s(
                        szDownloaded, 
                        ARRAYSIZE(szDownloaded), 
                        L"Downloaded: %s of %s (%d%%)", 
                        dlCurrent->Buffer, 
                        dlLength->Buffer, 
                        iProgress
                        );
                    /*swprintf_s(
                        szRemaning, 
                        ARRAYSIZE(szRemaning), 
                        L"Size: %s, Remaning: %s", 
                        dlLength->Buffer,
                        dlRemaningBytes->Buffer
                        );*/
                    /*swprintf_s(
                        szSpeed, 
                        ARRAYSIZE(szSpeed), 
                        L"Speed: %s/s", 
                        dlSpeed->Buffer
                        );*/

                    // Set last counters for the next loop.
                    dwLastTicks = dwNowTicks;
                    dwLastTotalBytes = dwTotalReadSize;

                    Edit_SetText(GetDlgItem(hwndDlg, IDC_DLSIZE), szDownloaded);
                    //Edit_SetText(GetDlgItem(hwndDlg, IDC_RELDATE), szRemaning);    
                    //Edit_SetText(GetDlgItem(hwndDlg, IDC_RELDATE), szSpeed);

                    // Update the progress bar position
                    PostMessage(hwndProgress, PBM_SETPOS, iProgress, 0);

                    PhDereferenceObject(dlRemaningBytes);
                    PhDereferenceObject(dlLength);
                    //PhDereferenceObject(dlSpeed);
                    PhDereferenceObject(dlCurrent);
                }
            }
        }

         // Set progress complete
         PostMessage(hwndProgress, PBM_SETPOS, 100, 0);
         // Set button text for next action
         Button_SetText(GetDlgItem(hwndDlg, IDC_DOWNLOAD), L"Install");
         // Enable the Install button
         Button_Enable(GetDlgItem(hwndDlg, IDC_DOWNLOAD), TRUE);

         // If PH is not elevated, set the UAC sheild for the installer.
         if (!PhElevated)
             SendMessage(GetDlgItem(hwndDlg, IDC_DOWNLOAD), BCM_SETSHIELD, 0, TRUE);

         // Compute our hash result.
         {
             UCHAR hashBuffer[20];
             ULONG hashLength = 0;

             if (PhFinalHash(&hashContext, hashBuffer, 20, &hashLength))
             {
                 WCHAR hexString[MAX_PATH];

                 // Allocate our hash string, hex the final hash result in our hashBuffer.
                 //PPH_STRING hexString = PhBufferToHexString(hashBuffer, hashLength);

                 swprintf_s(
                     hexString, 
                     ARRAYSIZE(hexString), 
                     L"%02x%02x%02x%02x%02x%02x%02x%02x%02x%02x%02x%02x%02x%02x%02x%02x%02x%02x%02x%02x", 
                     hashBuffer[0], hashBuffer[1], hashBuffer[2], hashBuffer[3], hashBuffer[4], hashBuffer[5], hashBuffer[6], 
                     hashBuffer[7], hashBuffer[8], hashBuffer[9],  hashBuffer[10], hashBuffer[11], hashBuffer[12], hashBuffer[13], 
                     hashBuffer[14], hashBuffer[15], hashBuffer[16], hashBuffer[17], hashBuffer[18], hashBuffer[19]
                     );

                 if (!_wcsicmp(hexString, xmlData.Hash))
                 {
                     Edit_SetText(GetDlgItem(hwndDlg, IDC_DLSIZE), L"Download complete, Hash verified");
                 }
                 else
                 {
                     Edit_SetText(GetDlgItem(hwndDlg, IDC_DLSIZE), L"Download complete, Hash failed");

                     if (WindowsVersion > WINDOWS_XP)
                         SendMessage(hwndProgress, PBM_SETSTATE, PBST_ERROR, 0);
                 }
             }
             else
             {
                 Edit_SetText(GetDlgItem(hwndDlg, IDC_DLSIZE), L"PhFinalHash failed");

                 // Show fancy Red progressbar if hash failed on Vista and above.
                 if (WindowsVersion > WINDOWS_XP)
                     SendMessage(hwndProgress, PBM_SETSTATE, PBST_ERROR, 0);			
             }
         }

         PhUpdaterState = Installing;
    }

CleanupAndExit:
    if (hInitialize)
        InternetCloseHandle(hInitialize);

    if (hConnection)
        InternetCloseHandle(hConnection);

    if (hRequest)
        InternetCloseHandle(hRequest);

    if (TempFileHandle)
        CloseHandle(TempFileHandle);


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

            EnableCache = (PhGetIntegerSetting(L"ProcessHacker.Updater.EnableCache") == 1 ? TRUE : FALSE);
            HashAlgorithm = (PH_HASH_ALGORITHM)PhGetIntegerSetting(L"ProcessHacker.Updater.HashAlgorithm");
            PhUpdaterState = Default;
        
            lHeaderFont.lfHeight = -15;
            lHeaderFont.lfWeight = FW_MEDIUM;
            lHeaderFont.lfQuality = CLEARTYPE_QUALITY | ANTIALIASED_QUALITY;
            // TODO: Do we need to check if Segoe exists? CreateFontIndirect works with invalid lfFaceName values...
            wcscat_s(lHeaderFont.lfFaceName, ARRAYSIZE(lHeaderFont.lfFaceName), L"Segoe UI");

            {
                // load the PH main icon using the 'magic' resource id.
                HANDLE hPhIcon = LoadImageW(
                    GetModuleHandle(NULL), 
                    MAKEINTRESOURCE(PHAPP_IDI_PROCESSHACKER), 
                    IMAGE_ICON, 
                    GetSystemMetrics(SM_CXICON), 
                    GetSystemMetrics(SM_CYICON),	
                    LR_SHARED
                    );

                // Set the window icon.
                SendMessageW(hwndDlg, WM_SETICON, ICON_BIG, (LPARAM)hPhIcon);
            }

            // Center the update window on PH if visible and not mimimized else center on desktop.
            PhCenterWindow(hwndDlg, (IsWindowVisible(GetParent(hwndDlg)) && !IsIconic(GetParent(hwndDlg))) ? GetParent(hwndDlg) : NULL);
            // Set the header font.
            SendMessageW(GetDlgItem(hwndDlg, IDC_MESSAGE), WM_SETFONT, (WPARAM)CreateFontIndirectW(&lHeaderFont), FALSE);

            // Create our update check thread.
            PhCreateThread(0, WorkerThreadStart, hwndDlg);
        }
        break; 
    case WM_CTLCOLORDLG:
    case WM_CTLCOLORSTATIC:
        {    
            HDC hDC = (HDC)wParam;
            HWND hwndChild = (HWND)lParam;

            if (GetDlgCtrlID(hwndChild) == IDC_MESSAGE)
            {
                SetTextColor(hDC, RGB(19, 112, 171));
            }

            // return stock White color as our window background.
            return (HBRUSH)GetStockObject(WHITE_BRUSH);
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
                    case Retry:
                    case Downloading:
                        {
                            if (PhInstalledUsingSetup())
                            {
                                // Star our Downloader thread   
                                PhCreateThread(0, DownloadWorkerThreadStart, hwndDlg);
                            }
                            else
                            {
                                // Let the user handle non-setup installation, show the homepage and close this dialog.
                                PhShellExecute(hwndDlg, L"http://processhacker.sourceforge.net/downloads.php", NULL);

                                EndDialog(hwndDlg, IDOK);
                            }
                        }
                        break;
                    case Installing:
                        {
                            SHELLEXECUTEINFO info = { sizeof(SHELLEXECUTEINFO) };
                            //info.lpFile = LocalFilePathString->Buffer;
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

                                PhUpdaterState = Retry;
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
    }

    return FALSE;
}

NTSTATUS ShowUpdateDialogThreadStart(
    __in PVOID Parameter
    )
{
    static HWND updateDialogHandle = NULL;

    // Check if our dialog handle is valid.
    if (!IsWindow(updateDialogHandle)) 
    { 
        BOOL result;
        MSG message;

        updateDialogHandle = CreateDialog( 
            (HINSTANCE)PluginInstance->DllBase, 
            MAKEINTRESOURCE(IDD_UPDATE), 
            PhMainWndHandle, 
            UpdaterWndProc
            ); 

        ShowWindow(updateDialogHandle, SW_SHOW); 
        UpdateWindow(updateDialogHandle); 

        while (result = GetMessage(&message, NULL, 0, 0)) 
        { 
            if (result == -1)
                break;

            if (!IsWindow(updateDialogHandle) || !IsDialogMessage(updateDialogHandle, &message))
            { 
                TranslateMessage(&message); 
                DispatchMessage(&message); 
            }
        }

        DestroyWindow(updateDialogHandle);
        updateDialogHandle = NULL;
    }

    return STATUS_SUCCESS;
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

            INetworkListManager_Release(pNetworkListManager);
            pNetworkListManager = NULL;

            if (vIsConnected == VARIANT_TRUE && vIsConnectedInternet == VARIANT_TRUE)
            {
                return TRUE;
            }
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
    __in HWND hwndDlg,
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
    static PH_STRINGREF keyName = PH_STRINGREF_INIT(L"Software\\Microsoft\\Windows\\CurrentVersion\\Uninstall\\Process_Hacker2_is1");

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
    PCHAR data;
    HINTERNET NetInitialize = NULL, NetConnection = NULL, NetRequest = NULL;
    BOOL result = FALSE;
    mxml_node_t *xmlDoc = NULL, *xmlNodeVer = NULL, *xmlNodeRelDate = NULL, *xmlNodeSize = NULL, *xmlNodeHash = NULL;

    // Initialize the wininet library.
    if (!(NetInitialize = InternetOpen(
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
    if (!(NetConnection = InternetConnect(
        NetInitialize,
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
    if (!(NetRequest = HttpOpenRequest(
        NetConnection,
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
    if (!HttpSendRequest(NetRequest, NULL, 0, NULL, 0))
    {
        LogEvent(NULL, PhFormatString(L"HttpSendRequest failed (%d)", GetLastError()));
        goto CleanupAndExit;
    }

    // Read the resulting xml into our buffer.
    if (!ReadRequestString(NetRequest, &data, NULL))
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
    if (xmlNodeVer == NULL || xmlNodeVer->type != MXML_ELEMENT)
    {
        LogEvent(NULL, PhFormatString(L"Updater: (WorkerThreadStart) mxmlLoadString xmlNodeVer failed."));
        goto CleanupAndExit;
    }

    // Find the reldate node.
    xmlNodeRelDate = mxmlFindElement(xmlDoc, xmlDoc, "reldate", NULL, NULL, MXML_DESCEND);
    if (xmlNodeRelDate == NULL || xmlNodeRelDate->type != MXML_ELEMENT)
    {
        LogEvent(NULL, PhFormatString(L"Updater: (WorkerThreadStart) mxmlLoadString xmlNodeRelDate failed."));
        goto CleanupAndExit;
    }

    // Find the size node.
    xmlNodeSize = mxmlFindElement(xmlDoc, xmlDoc, "size", NULL, NULL, MXML_DESCEND);
    if (xmlNodeSize == NULL || xmlNodeSize->type != MXML_ELEMENT)
    {
        LogEvent(NULL, PhFormatString(L"Updater: (WorkerThreadStart) mxmlLoadString xmlNodeSize failed."));
        goto CleanupAndExit;
    }

    xmlNodeHash = mxmlFindElement(xmlDoc, xmlDoc, "sha1", NULL, NULL, MXML_DESCEND);
    if (xmlNodeHash == NULL || xmlNodeHash->type != MXML_ELEMENT)
    {
        LogEvent(NULL, PhFormatString(L"Updater: (WorkerThreadStart) mxmlLoadString xmlNodeHash failed."));
        goto CleanupAndExit;
    }

    {
        WCHAR szTempString[MAX_PATH];

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


VOID StartInitialCheck(
    VOID
    )
{
    // Queue up our initial update check.
    PhCreateThread(0, SilentUpdateCheckThreadStart, NULL);
}