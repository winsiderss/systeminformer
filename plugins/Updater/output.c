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
#include <process.h>

// Always consider the remote version newer
#ifdef _DEBUG
#define TEST_MODE
#endif

#pragma region Static Fields

static HANDLE TempFileHandle = NULL;
static HINTERNET NetInitialize = NULL, NetConnection = NULL, NetRequest = NULL;
static PPH_STRING RemoteHashString = NULL;
static PPH_STRING LocalFilePathString = NULL;
static PPH_STRING LocalFileNameString = NULL;

static PH_UPDATER_STATE PhUpdaterState = Default;
static BOOL EnableCache = TRUE;
static BOOL WindowVisible = FALSE;
static PH_HASH_ALGORITHM HashAlgorithm = Md5HashAlgorithm;

#pragma endregion

#pragma region Worker Threads

#pragma region AutoCheck Thread

static void __cdecl SilentWorkerThreadStart(
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

            ShowUpdateDialog();
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

#pragma endregion

static void __cdecl DownloadChangelogText(
    __in PVOID Parameter
    )
{
    HWND hwndDlg = (HWND)Parameter;

    if (!InitializeConnection(L"processhacker.svn.sourceforge.net", L"/viewvc/processhacker/2.x/trunk/CHANGELOG.txt"))//?revision=4612"))
        return;

    // Send the HTTP request.
    if (HttpSendRequest(NetRequest, NULL, 0, NULL, 0))
    {
        PSTR data;
                              
        // Read the page into our buffer.
        if (!ReadRequestString(NetRequest, &data, NULL))
            return;
        else
        {
            PPH_STRING str = PhCreateStringFromAnsi(data);

            SetDlgItemText(hwndDlg, IDC_EDIT1, PhGetString(str));

            PhDereferenceObject(str);
        }

        PhFree(data);
    }

    PhUpdaterState = Downloading;
}


#pragma region Xml Downloader Thread

static void __cdecl WorkerThreadStart(
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
        PhDereferenceObject(localVersion);

        if (result > 0)
        {
            PPH_STRING summaryText;

            summaryText = PhFormatString(L"Process Hacker %u.%u", xmlData.MajorVersion, xmlData.MinorVersion);
            SetDlgItemText(hwndDlg, IDC_MESSAGE, summaryText->Buffer);
            PhDereferenceObject(summaryText);

            summaryText = PhFormatString(L"Released: %s", xmlData.RelDate->Buffer);
            SetDlgItemText(hwndDlg, IDC_RELDATE, summaryText->Buffer);
            PhDereferenceObject(summaryText);

            summaryText = PhFormatString(L"Size: %s", xmlData.Size->Buffer);
            SetDlgItemText(hwndDlg, IDC_DLSIZE, summaryText->Buffer);
            PhDereferenceObject(summaryText);

            LocalFileNameString = PhFormatString(L"processhacker-%u.%u-setup.exe", xmlData.MajorVersion, xmlData.MinorVersion);

            Updater_EnableUI(hwndDlg);
        }
        else if (result == 0)
        {
            PPH_STRING summaryText = PhFormatString(L"You're running the latest version: %u.%u", xmlData.MajorVersion, xmlData.MinorVersion);

            SetDlgItemText(hwndDlg, IDC_MESSAGE, summaryText->Buffer);

            PhDereferenceObject(summaryText);
        }
        else if (result < 0)
        {
            PPH_STRING summaryText = PhFormatString(L"You're running a newer version: %u.%u", localMajorVersion, localMinorVersion);

            SetDlgItemText(hwndDlg, IDC_MESSAGE, summaryText->Buffer);

            PhDereferenceObject(summaryText);
        }

        FreeXmlData(&xmlData);

        {
            PPH_STRING sText = PhFormatString(L"\\\\%s", LocalFileNameString->Buffer);
            PPH_STRING autoDbghelpPath = PhGetKnownLocation(CSIDL_DESKTOP, sText->Buffer);

            PhDereferenceObject(sText);
            if (LocalFilePathString)
                PhDereferenceObject(LocalFilePathString);
             DisposeFileHandles();

            LocalFilePathString = autoDbghelpPath;

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

#pragma endregion

#pragma region File Downloader Thread

static void __cdecl DownloadWorkerThreadStart(
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
        DWORD dwCurrentTicks = dwStartTicks;
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
                dwTimeTaken = dwNowTicks - dwCurrentTicks;

                // Update the progress every 500ms.
                if (dwTimeTaken > 100)
                {
                    // Calculate the percentage of our total bytes downloaded per the length.
                    INT dlProgress = (int)(((double)dwTotalReadSize / (double)dwContentLen) * 100);

                    PPH_STRING dlCurrent = PhFormatSize(dwTotalReadSize, -1);
                    PPH_STRING dlLength = PhFormatSize(dwContentLen, -1);
                    PPH_STRING str = PhFormatString(L"Downloaded: %s of %s (%d%%)", dlCurrent->Buffer, dlLength->Buffer, dlProgress);

                    Updater_SetStatusText(hwndDlg, str->Buffer);
                    PostMessage(hwndProgress, PBM_SETPOS, dlProgress, 0);

                    PhDereferenceObject(str);
                    PhDereferenceObject(dlLength);
                    PhDereferenceObject(dlCurrent);
                }

                // Update the transfer rate and estimated time every second.
                if (dwTimeTaken > 1000)
                {
                    INT kbPerSecond = (int)(((double)(dwTotalReadSize) - (double)(dwLastTotalBytes)) / ((double)(dwTimeTaken)) * 1024);

                    //Setup for the next time around the loop
                    dwCurrentTicks = dwNowTicks;
                    dwLastTotalBytes = dwTotalReadSize;

                    //Update the estimated time left
                    if (dwTotalReadSize)
                    {
                        DWORD dwSecondsLeft;
                        PPH_STRING rstr;
                        PPH_STRING str;

                        dwSecondsLeft =  (DWORD)(((double)dwNowTicks - dwStartTicks) / dwTotalReadSize * (dwContentLen - dwTotalReadSize) / 1000);
                        str = PhFormatSize(dwContentLen - dwLastTotalBytes, -1);
                        rstr = PhFormatString(L"Remaning: %s (%ds)", str->Buffer, dwSecondsLeft);

                        SetDlgItemText(hwndDlg, IDC_RTIMETEXT, rstr->Buffer);

                        PhDereferenceObject(rstr);
                        PhDereferenceObject(str);
                    }
                    {
                        PPH_STRING str = PhFormatSize(kbPerSecond, -1);
                        PPH_STRING str2 = PhFormatString(L"Speed: %s/s", str->Buffer);;

                        SetDlgItemText(hwndDlg, IDC_SPEEDTEXT, str2->Buffer);
                        
                        PhDereferenceObject(str2);
                        PhDereferenceObject(str);
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

        Updater_SetStatusText(hwndDlg, L"Download Complete");
        // Enable Install button before computing the hash result (user might not care about file hash result)
        SetDlgItemText(hwndDlg, IDC_DOWNLOAD, L"Install");
        Updater_EnableUI(hwndDlg);

        if (!PhElevated)
            SendMessage(GetDlgItem(hwndDlg, IDC_DOWNLOAD), BCM_SETSHIELD, 0, TRUE);

        {
            UCHAR hashBuffer[20];
            ULONG hashLength = 0;

            if (PhFinalHash(&hashContext, hashBuffer, 20, &hashLength))
            {
                // Allocate our hash string, hex the final hash result in our hashBuffer.
                PH_STRING *hexString = PhBufferToHexString(hashBuffer, hashLength);

                if (PhEqualString(hexString, RemoteHashString, TRUE))
                {
                    Updater_SetStatusText(hwndDlg, L"Hash Verified");
                }
                else
                {
                    if (WindowsVersion >= WINDOWS_VISTA)
                        SendMessage(hwndProgress, PBM_SETSTATE, PBST_ERROR, 0);

                    Updater_SetStatusText(hwndDlg, L"Hash failed");
                }

                PhDereferenceObject(hexString);
            }
            else
            {
                // Show fancy Red progressbar if hash failed on Vista and above.
                if (WindowsVersion >= WINDOWS_VISTA)
                    SendMessage(hwndProgress, PBM_SETSTATE, PBST_ERROR, 0);

                Updater_SetStatusText(hwndDlg, L"Hash failed");
            }
        }
    }
    else
    {
        LogEvent(PhFormatString(L"Updater: (DownloadWorkerThreadStart) HttpSendRequest failed (%d)", GetLastError()));
    }
}

#pragma endregion

#pragma endregion

INT_PTR CALLBACK MainWndProc(
    __in HWND hwndDlg,
    __in UINT uMsg,
    __in WPARAM wParam,
    __in LPARAM lParam
    )
{
    switch (uMsg)
    {
    case ENABLE_UI:
        {
            EnableWindow(GetDlgItem(hwndDlg, IDC_RELDATE), TRUE);
            EnableWindow(GetDlgItem(hwndDlg, IDC_DLSIZE), TRUE);
            EnableWindow(GetDlgItem(hwndDlg, IDC_DOWNLOAD), TRUE);
        }
        break;
    case WM_INITDIALOG:
        {
            WindowVisible = TRUE;

            PhCenterWindow(hwndDlg, GetParent(hwndDlg));

            EnableCache = PhGetIntegerSetting(L"ProcessHacker.Updater.EnableCache");
            HashAlgorithm = (PH_HASH_ALGORITHM)PhGetIntegerSetting(L"ProcessHacker.Updater.HashAlgorithm");
            PhUpdaterState = Default;
            
            _beginthread(WorkerThreadStart, 0, hwndDlg);
        }
        break;
    case WM_DESTROY:
        {
            DisposeConnection();
            DisposeStrings();
            DisposeFileHandles();

            WindowVisible = FALSE;
        }
        break;
    case WM_COMMAND:
        {
            switch (LOWORD(wParam))
            {
            case IDCANCEL:
            case IDOK:
                {
                    DisposeConnection();
                    DisposeStrings();
                    DisposeFileHandles();

                    EndDialog(hwndDlg, IDOK);
                }
                break;
            case IDC_DOWNLOAD:
                {
                    switch (PhUpdaterState)
                    {
                    case Downloading:
                        {
                            EnableWindow(GetDlgItem(hwndDlg, IDC_DOWNLOAD), FALSE);

                            if (PhInstalledUsingSetup())
                            {
                                Updater_SetStatusText(hwndDlg, L"Initializing");

                                // Show the status text
                                ShowWindow(GetDlgItem(hwndDlg, IDC_STATUSTEXT), SW_SHOW);
                                ShowWindow(GetDlgItem(hwndDlg, IDC_SPEEDTEXT), SW_SHOW);
                                ShowWindow(GetDlgItem(hwndDlg, IDC_RTIMETEXT), SW_SHOW);

                                PhSetWindowStyle(GetDlgItem(hwndDlg, IDC_PROGRESS), PBS_MARQUEE, PBS_MARQUEE);
                                PostMessage(GetDlgItem(hwndDlg, IDC_PROGRESS), PBM_SETMARQUEE, TRUE, 75);
   
                                // Star our Downloader thread   
                                _beginthread(DownloadWorkerThreadStart, 0, hwndDlg);
                            }
                            else
                            {
                                // Let the user handle non-setup installation, show the homepage and close this dialog.
                                PhShellExecute(hwndDlg, L"http://processhacker.sourceforge.net/downloads.php", NULL);

                                DisposeConnection();
                                DisposeStrings();
                                DisposeFileHandles();

                                EndDialog(hwndDlg, IDOK);
                            }
                            return FALSE;
                        }
                    case Installing:
                        {
                            PhShellExecute(hwndDlg, LocalFilePathString->Buffer, NULL);
                            DisposeConnection();
                            
                            ProcessHacker_Destroy(PhMainWndHandle);
                        }
                        return FALSE;
                    }
                }
                break;
            }
        }
        break;
    }

    return FALSE;
}

BOOL InitializeConnection(
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

mxml_type_t QueryXmlDataCallback(
    __in mxml_node_t *node
    )
{
    return MXML_OPAQUE;
}

BOOL QueryXmlData(
    __in PVOID Buffer,
    __out PUPDATER_XML_DATA XmlData
    )
{
    BOOL result = FALSE;
    mxml_node_t *xmlDoc = NULL, *xmlNodeVer = NULL, *xmlNodeRelDate = NULL, *xmlNodeSize = NULL, *xmlNodeHash = NULL;
    PPH_STRING temp;

    // Load our XML.
    xmlDoc = mxmlLoadString(NULL, (char*)Buffer, QueryXmlDataCallback);
    // Check our XML.
    if (xmlDoc == NULL || xmlDoc->type != MXML_ELEMENT)
    {
        LogEvent(PhFormatString(L"Updater: (WorkerThreadStart) mxmlLoadString failed."));
        goto CleanupAndExit;
    }

    // Find the ver node.
    xmlNodeVer = mxmlFindElement(xmlDoc, xmlDoc, "ver", NULL, NULL, MXML_DESCEND);
    if (xmlNodeVer == NULL || xmlNodeVer->type != MXML_ELEMENT)
    {
        LogEvent(PhFormatString(L"Updater: (WorkerThreadStart) mxmlLoadString xmlNodeVer failed."));
        goto CleanupAndExit;
    }

    // Find the reldate node.
    xmlNodeRelDate = mxmlFindElement(xmlDoc, xmlDoc, "reldate", NULL, NULL, MXML_DESCEND);
    if (xmlNodeRelDate == NULL || xmlNodeRelDate->type != MXML_ELEMENT)
    {
        LogEvent(PhFormatString(L"Updater: (WorkerThreadStart) mxmlLoadString xmlNodeRelDate failed."));
        goto CleanupAndExit;
    }

    // Find the size node.
    xmlNodeSize = mxmlFindElement(xmlDoc, xmlDoc, "size", NULL, NULL, MXML_DESCEND);
    if (xmlNodeSize == NULL || xmlNodeSize->type != MXML_ELEMENT)
    {
        LogEvent(PhFormatString(L"Updater: (WorkerThreadStart) mxmlLoadString xmlNodeSize failed."));
        goto CleanupAndExit;
    }

    if (HashAlgorithm == Md5HashAlgorithm)
        xmlNodeHash = mxmlFindElement(xmlDoc, xmlDoc, "md5", NULL, NULL, MXML_DESCEND);
    else
        xmlNodeHash = mxmlFindElement(xmlDoc, xmlDoc, "sha1", NULL, NULL, MXML_DESCEND);

    if (xmlNodeHash == NULL || xmlNodeHash->type != MXML_ELEMENT)
    {
        LogEvent(PhFormatString(L"Updater: (WorkerThreadStart) mxmlLoadString xmlNodeHash failed."));
        goto CleanupAndExit;
    }

    temp = PhCreateStringFromAnsi(xmlNodeVer->child->value.opaque);
    result = ParseVersionString(temp->Buffer, &XmlData->MajorVersion, &XmlData->MinorVersion);
    PhDereferenceObject(temp);

    if (!result)
        goto CleanupAndExit;

    XmlData->RelDate = PhCreateStringFromAnsi(xmlNodeRelDate->child->value.opaque);
    XmlData->Size = PhCreateStringFromAnsi(xmlNodeSize->child->value.opaque);
    XmlData->Hash = PhCreateStringFromAnsi(xmlNodeHash->child->value.opaque);

    result = TRUE;

CleanupAndExit:
    mxmlDelete(xmlDoc);

    return result;
}

BOOL ConnectionAvailable(VOID)
{
    DWORD dwType;

    if (!InternetGetConnectedState(&dwType, 0))
    {
        LogEvent(PhFormatString(L"Updater: (ConnectionAvailable) InternetGetConnectedState failed to detect an active Internet connection (%d)", GetLastError()));
        return FALSE;
    }

    //if (!InternetCheckConnection(NULL, FLAG_ICC_FORCE_CONNECTION, 0))
    //{
    //  LogEvent(PhFormatString(L"Updater: (ConnectionAvailable) InternetCheckConnection failed connection to Sourceforge.net (%d)", GetLastError()));
    //  return FALSE;
    //}

    return TRUE;
}

BOOL ParseVersionString(
    __in PWSTR String,
    __out PULONG MajorVersion,
    __out PULONG MinorVersion
    )
{
    PH_STRINGREF sr;
    PH_STRINGREF majorPart;
    PH_STRINGREF minorPart;
    ULONG64 majorInteger;
    ULONG64 minorInteger;

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

VOID StartInitialCheck(VOID)
{
    // Queue up our initial update check.
    _beginthread(SilentWorkerThreadStart, 0, NULL);
}

VOID ShowUpdateDialog(VOID)
{
    // check if our dialog is already visible (auto-check may already be visible).
    if (!WindowVisible)
    {
        DialogBox(
            (HINSTANCE)PluginInstance->DllBase,
            MAKEINTRESOURCE(IDD_OUTPUT),
            PhMainWndHandle,
            MainWndProc
            );
    }
}

BOOL PhInstalledUsingSetup(VOID)
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
    else
    {
        LogEvent(PhFormatString(L"Updater: (PhInstalledUsingSetup) PhOpenKey failed (0x%x)", status));
        return FALSE;
    }
}

#pragma region Event Logging Functions

VOID LogEvent(__in PPH_STRING str)
{
    PhLogMessageEntry(PH_LOG_ENTRY_MESSAGE, str);

    PhDereferenceObject(str);
}

#pragma endregion

#pragma region Dispose Functions

VOID DisposeConnection(VOID)
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

VOID DisposeStrings(VOID)
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

VOID DisposeFileHandles(VOID)
{
    if (TempFileHandle)
    {
        NtClose(TempFileHandle);
        TempFileHandle = NULL;
    }
}

VOID FreeXmlData(
    __in PUPDATER_XML_DATA XmlData
    )
{
    PhDereferenceObject(XmlData->RelDate);
    PhDereferenceObject(XmlData->Size);
    PhDereferenceObject(XmlData->Hash);
}

#pragma endregion