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

static PPH_STRING RemoteHashString = NULL, LocalFilePathString = NULL, LocalFileNameString = NULL;
static HINTERNET NetInitialize = NULL, NetConnection = NULL, NetRequest = NULL;
static BOOL EnableCache = TRUE, WindowVisible = FALSE;
static PH_UPDATER_STATE PhUpdaterState = Default;
static PH_HASH_ALGORITHM HashAlgorithm = Sha1HashAlgorithm;
static HANDLE TempFileHandle = NULL;

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
        LogEvent(NULL, PhFormatString(L"Updater: (WorkerThreadStart) HttpSendRequest failed (%d)", GetLastError()));
    }

    DisposeConnection();
}

static void __cdecl WorkerThreadStart(
    __in PVOID Parameter
    )
{
    PSTR data;
    UPDATER_XML_DATA xmlData;
    PPH_STRING localVersion;
    INT result = 0;
    ULONG localMajorVersion = 0, localMinorVersion = 0;
    HWND hwndDlg = (HWND)Parameter;
  
    if (!InitializeConnection(UPDATE_URL, UPDATE_FILE))
        return;
    
    // Send the HTTP request.
    if (!HttpSendRequest(NetRequest, NULL, 0, NULL, 0))
    {
        LogEvent(hwndDlg, PhFormatString(L"HttpSendRequest failed (%d)", GetLastError()));
        DisposeConnection();
        return;
    }

    // Read the resulting xml into our buffer.
    if (!ReadRequestString(NetRequest, &data, NULL))
    {
        DisposeConnection();
        return;
    }

    if (!QueryXmlData(data, &xmlData))
    {
        PhFree(data);
        DisposeConnection();
        return;
    }

    PhFree(data);
    DisposeConnection();

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
        Edit_SetText(GetDlgItem(hwndDlg, IDC_MESSAGE), summaryText->Buffer);
        PhDereferenceObject(summaryText);

        summaryText = PhFormatString(L"Released: %s", xmlData.RelDate->Buffer);
        Edit_SetText(GetDlgItem(hwndDlg, IDC_RELDATE), summaryText->Buffer);
        PhDereferenceObject(summaryText);

		summaryText = PhFormatString(L"Size: %s", xmlData.Size->Buffer);
		Edit_SetText(GetDlgItem(hwndDlg, IDC_DLSIZE), summaryText->Buffer);
		PhDereferenceObject(summaryText);

		LocalFileNameString = PhFormatString(L"processhacker-%u.%u-setup.exe", xmlData.MajorVersion, xmlData.MinorVersion);

		Edit_Visible(GetDlgItem(hwndDlg, IDC_RELDATE), TRUE);
		Edit_Visible(GetDlgItem(hwndDlg, IDC_DLSIZE), TRUE);

		Button_Enable(GetDlgItem(hwndDlg, IDC_DOWNLOAD), TRUE);

        {
            PPH_STRING sText = PhFormatString(L"\\%s", LocalFileNameString->Buffer);

            UINT dwRetVal = 0;
            TCHAR lpTempPathBuffer[MAX_PATH];

            // Get the temp path env string (no guarantee it's a valid path).
            dwRetVal = GetTempPath(MAX_PATH, lpTempPathBuffer);

            if (dwRetVal > MAX_PATH || dwRetVal == 0)
            {
                OutputDebugString(L"GetTempPath failed");
                return;
            }	

            PhDereferenceObject(LocalFileNameString);

            LocalFilePathString = PhConcatStrings2(lpTempPathBuffer, sText->Buffer);

			PhDereferenceObject(sText);
		}

		PhUpdaterState = Downloading;
	}
    else if (result == 0)
    {
        PPH_STRING summaryText = PhFormatString(L"You're running the latest version: %u.%u", xmlData.MajorVersion, xmlData.MinorVersion);

        Edit_SetText(GetDlgItem(hwndDlg, IDC_MESSAGE), summaryText->Buffer);

        PhDereferenceObject(summaryText);
    }
    else if (result < 0)
    {
        PPH_STRING summaryText = PhFormatString(L"You're running a newer version: %u.%u", localMajorVersion, localMinorVersion);
		PPH_STRING stableText =  PhFormatString(L"The latest stable version available: %u.%u", xmlData.MajorVersion, xmlData.MinorVersion);

		Edit_Visible(GetDlgItem(hwndDlg, IDC_RELDATE), TRUE);
		Edit_SetText(GetDlgItem(hwndDlg, IDC_MESSAGE), summaryText->Buffer);
		Edit_SetText(GetDlgItem(hwndDlg, IDC_RELDATE), stableText->Buffer);
        
		PhDereferenceObject(summaryText);
		PhDereferenceObject(stableText);
    }

    FreeXmlData(&xmlData);
    DisposeConnection();
}

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

    Edit_SetText(GetDlgItem(hwndDlg, IDC_STATUSTEXT), L"Connecting");
    
    if (HttpSendRequest(NetRequest, NULL, 0, NULL, 0))
    {
        CHAR buffer[BUFFER_LEN];
        DWORD dwLastTotalBytes = 0;
        DWORD dwStartTicks = 0, dwLastTicks = 0, dwNowTicks = 0, dwTimeTaken = 0;

        // Set our last ticks.
        dwLastTicks = (dwStartTicks = GetTickCount());

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
                    LogEvent(hwndDlg, PhFormatString(L"InternetReadFile failed (%d)", GetLastError()));
                    return;
                }

                // Update the hash of bytes we just downloaded.
                PhUpdateHash(&hashContext, buffer, dwBytesRead);

                // Write the downloaded bytes to disk.
                if (!WriteFile(TempFileHandle, buffer, dwBytesRead, &dwBytesWritten, NULL))
                {
                    LogEvent(hwndDlg, PhFormatString(L"WriteFile failed (%d)", GetLastError()));
                    return;
                }

                // Zero the buffer.
                RtlZeroMemory(buffer, dwBytesRead);

                // Check dwBytesRead are the same dwBytesWritten length returned by WriteFile.
                if (dwBytesRead != dwBytesWritten)
                {
                    LogEvent(hwndDlg, PhFormatString(L"WriteFile failed (%d)", GetLastError()));
                    return;
                }

                // Calculate the transfer rate and download speed. 
                dwNowTicks = GetTickCount();

                dwTimeTaken = dwNowTicks - dwLastTicks;   
                // Update our total bytes downloaded
                dwTotalReadSize += dwBytesRead;

                // Update the transfer rate and estimated time every 100ms.
                if (dwTimeTaken > 100)
                {
                    ULONG64 kbPerSecond = (ULONG64)(((double)(dwTotalReadSize) - (double)(dwLastTotalBytes)) / ((double)(dwTimeTaken)) * 1024);
                    INT dwSecondsLeft =  (INT)(((double)dwNowTicks - dwStartTicks) / dwTotalReadSize * (dwContentLen - dwTotalReadSize) / 1000);
                    // Calculate the percentage of our total bytes downloaded per the length.
                    INT dlProgress = (int)(((double)dwTotalReadSize / (double)dwContentLen) * 100);
                                        
                    PPH_STRING dlCurrent = PhFormatSize(dwTotalReadSize, -1);
                    PPH_STRING dlLength = PhFormatSize(dwContentLen, -1);
                    PPH_STRING dlSpeed = PhFormatSize(kbPerSecond, -1);
                    PPH_STRING dlRemaningBytes = PhFormatSize(dwContentLen - dwLastTotalBytes, -1);

                    PPH_STRING sDownloaded = PhFormatString(L"Downloaded: %s of %s (%d%%)", dlCurrent->Buffer, dlLength->Buffer, dlProgress);
                    PPH_STRING sRemaning = PhFormatString(L"Remaning: %s (%ds)", dlRemaningBytes->Buffer, dwSecondsLeft);
                    PPH_STRING sSpeed = PhFormatString(L"Speed: %s/s", dlSpeed->Buffer);;
                  
                    // Set last counters for the next loop.
                    dwLastTicks = dwNowTicks;
                    dwLastTotalBytes = dwTotalReadSize;

                    // Update the estimated time left.
                    Edit_SetText(GetDlgItem(hwndDlg, IDC_RTIMETEXT), sRemaning->Buffer);
                    // Update the download transfer rate.
                    Edit_SetText(GetDlgItem(hwndDlg, IDC_SPEEDTEXT), sSpeed->Buffer);
                    // Update the progress bar
                    Edit_SetText(GetDlgItem(hwndDlg, IDC_STATUSTEXT), sDownloaded->Buffer);
                    // Update the progress bar position
					PostMessage(hwndProgress, PBM_SETPOS, dlProgress, 0);

                    PhDereferenceObject(sSpeed);
                    PhDereferenceObject(sRemaning);   
                    PhDereferenceObject(sDownloaded);   
                    PhDereferenceObject(dlRemaningBytes);
                    PhDereferenceObject(dlSpeed);
                    PhDereferenceObject(dlLength);
                    PhDereferenceObject(dlCurrent);
                }
            }
        }
        else
        {
            // No content length...impossible to calculate % complete so just read until we are done.
            LogEvent(NULL, PhFormatString(L"Updater: (DownloadWorkerThreadStart) HttpQueryInfo failed (%d)", GetLastError()));

            while ((nReadFile = InternetReadFile(NetRequest, buffer, BUFFER_LEN, &dwBytesRead)))
            {
                if (dwBytesRead == 0)
                    break;

                if (!nReadFile)
                {
                    LogEvent(hwndDlg, PhFormatString(L"InternetReadFile failed (%d)", GetLastError()));
                    return;
                }

                PhUpdateHash(&hashContext, buffer, dwBytesRead);

                if (!WriteFile(TempFileHandle, buffer, dwBytesRead, &dwBytesWritten, NULL))
                {
                    LogEvent(hwndDlg, PhFormatString(L"WriteFile failed (%d)", GetLastError()));
                    return;
                }

                // Reset the buffer.
                RtlZeroMemory(buffer, BUFFER_LEN);

                if (dwBytesRead != dwBytesWritten)
                {
                    LogEvent(hwndDlg, PhFormatString(L"WriteFile failed (%d)", GetLastError()));
                    return;
                }
            }
        }
        
        // Compute our hash result.
        {
            UCHAR hashBuffer[20];
            ULONG hashLength = 0;

            if (PhFinalHash(&hashContext, hashBuffer, 20, &hashLength))
            {
                // Allocate our hash string, hex the final hash result in our hashBuffer.
                PH_STRING *hexString = PhBufferToHexString(hashBuffer, hashLength);

                if (RemoteHashString && PhEqualString(hexString, RemoteHashString, TRUE))
                {
					Edit_SetText(GetDlgItem(hwndDlg, IDC_RTIMETEXT), L"Hash Verified");
                }
                else
                {
                    if (WindowsVersion > WINDOWS_XP)
                    {
                        SendMessage(hwndProgress, PBM_SETSTATE, PBST_ERROR, 0);
                    }
					
					Edit_SetText(GetDlgItem(hwndDlg, IDC_RTIMETEXT), L"Hash failed");
                }

                PhDereferenceObject(hexString);
            }
            else
            {
                // Show fancy Red progressbar if hash failed on Vista and above.
                if (WindowsVersion > WINDOWS_XP)
                {
                    SendMessage(hwndProgress, PBM_SETSTATE, PBST_ERROR, 0);
                }
				
				Edit_SetText(GetDlgItem(hwndDlg, IDC_RTIMETEXT), L"PhFinalHash failed");
            }
        }

        // Set progress complete
		PostMessage(hwndProgress, PBM_SETPOS, 100, 0);

		// Set button text for next action
		Button_SetText(GetDlgItem(hwndDlg, IDC_DOWNLOAD), L"Install");
        // Set Status
        Edit_SetText(GetDlgItem(hwndDlg, IDC_STATUSTEXT), L"Download Complete");
        
        // Enable the labels
		Edit_Visible(GetDlgItem(hwndDlg, IDC_SPEEDTEXT), FALSE);
		// Enable the Install button
        Button_Enable(GetDlgItem(hwndDlg, IDC_DOWNLOAD), TRUE);
        
        // If PH is not elevated show the UAC sheild since it'll be shown by the PH setup.
        if (!PhElevated)
            SendMessage(GetDlgItem(hwndDlg, IDC_DOWNLOAD), BCM_SETSHIELD, 0, TRUE);

        PhUpdaterState = Installing;
	}
	else
	{
		LogEvent(hwndDlg, PhFormatString(L"HttpSendRequest failed (%d)", GetLastError()));

		// Enable the 'Retry' button.
		Button_Enable(GetDlgItem(hwndDlg, IDC_DOWNLOAD), TRUE);
		Button_SetText(GetDlgItem(hwndDlg, IDC_DOWNLOAD), L"Retry");

		// Reset the state and let user retry the download.
		PhUpdaterState = Retry;
	}

	DisposeConnection();
	DisposeFileHandles();

	if (uriPath)
		PhDereferenceObject(uriPath);
}

INT_PTR CALLBACK MainWndProc(
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
            DisposeConnection();
            DisposeStrings();
            DisposeFileHandles();

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
                    RunAction(hwndDlg);
                }
                break;
            }
            break;  
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

BOOL ConnectionAvailable()
{
    DWORD dwType;

    if (!InternetGetConnectedState(&dwType, 0))
    {
        LogEvent(NULL, PhFormatString(L"Updater: (ConnectionAvailable) InternetGetConnectedState failed to detect an active Internet connection (%d)", GetLastError()));
        return FALSE;
    }

    //if (!InternetCheckConnection(NULL, FLAG_ICC_FORCE_CONNECTION, 0))
    //{
    //  LogEvent(PhFormatString(L"Updater: (ConnectionAvailable) InternetCheckConnection failed connection to Sourceforge.net (%d)", GetLastError()));
    //  return FALSE;
    //}

    return TRUE;
}

VOID DisposeConnection()
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

VOID DisposeFileHandles()
{
    if (TempFileHandle)
    {
        NtClose(TempFileHandle);
        TempFileHandle = NULL;
    }
}

VOID DisposeStrings()
{
    if (RemoteHashString)
    {
        PhDereferenceObject(RemoteHashString);
        RemoteHashString = NULL;
    }

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

VOID FreeXmlData(
	__in PUPDATER_XML_DATA XmlData
	)
{
    PhDereferenceObject(XmlData->RelDate);
    PhDereferenceObject(XmlData->Size);
    PhDereferenceObject(XmlData->Hash);
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
        LogEvent(NULL, PhFormatString(L"Updater: (InitializeConnection) InternetOpen failed (%d)", GetLastError()));
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
        LogEvent(NULL, PhFormatString(L"Updater: (InitializeConnection) InternetConnect failed (%d)", GetLastError()));

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
        LogEvent(NULL, PhFormatString(L"Updater: (InitializeConnection) HttpOpenRequest failed (%d)", GetLastError()));

        DisposeConnection();

        return FALSE;
    }

    return TRUE;
}

VOID LogEvent(
    __in HWND hwndDlg,
    __in PPH_STRING str
    )
{
    if (hwndDlg)
        Edit_SetText(GetDlgItem(hwndDlg, IDC_STATUSTEXT), str->Buffer);
	else
		PhLogMessageEntry(PH_LOG_ENTRY_MESSAGE, str);

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

BOOL PhInstalledUsingSetup()
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

    if (HashAlgorithm == Md5HashAlgorithm)
        xmlNodeHash = mxmlFindElement(xmlDoc, xmlDoc, "md5", NULL, NULL, MXML_DESCEND);
    else
        xmlNodeHash = mxmlFindElement(xmlDoc, xmlDoc, "sha1", NULL, NULL, MXML_DESCEND);

    if (xmlNodeHash == NULL || xmlNodeHash->type != MXML_ELEMENT)
    {
        LogEvent(NULL, PhFormatString(L"Updater: (WorkerThreadStart) mxmlLoadString xmlNodeHash failed."));
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

VOID RunAction(
	__in HWND hwndDlg
	)
{
    switch (PhUpdaterState)
    {
	case Retry:
    case Downloading:
        {
            Button_Enable(GetDlgItem(hwndDlg, IDC_DOWNLOAD), FALSE);
            PhSetWindowStyle(GetDlgItem(hwndDlg, IDC_PROGRESS), PBS_MARQUEE, PBS_MARQUEE);
            PostMessage(GetDlgItem(hwndDlg, IDC_PROGRESS), PBM_SETMARQUEE, TRUE, 75);

            if (PhInstalledUsingSetup())
            {
                Edit_SetText(GetDlgItem(hwndDlg, IDC_STATUSTEXT), L"Initializing");
                // Show the status text
                Edit_Visible(GetDlgItem(hwndDlg, IDC_STATUSTEXT), TRUE);
                Edit_Visible(GetDlgItem(hwndDlg, IDC_SPEEDTEXT), TRUE);
                Edit_Visible(GetDlgItem(hwndDlg, IDC_RTIMETEXT), TRUE);

                DisposeFileHandles();

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
                    LogEvent(hwndDlg, PhFormatString(L"CreateFile failed (%d)", GetLastError()));
                }

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
            break;
        }
    case Installing:
        {
            ProcessHacker_PrepareForEarlyShutdown(PhMainWndHandle);

            PhShellExecute(hwndDlg, LocalFilePathString->Buffer, NULL);
            DisposeConnection();

            ProcessHacker_Destroy(PhMainWndHandle);
        }
        break;
    }
}

VOID ShowUpdateDialog()
{
    // check if our dialog is already visible (auto-check may already be visible).
    if (!WindowVisible)
    {
        DialogBox(
            (HINSTANCE)PluginInstance->DllBase,
            MAKEINTRESOURCE(IDD_UPDATE),
            PhMainWndHandle,
            MainWndProc
            );
    }
}

VOID StartInitialCheck()
{
    // Queue up our initial update check.
    _beginthread(SilentWorkerThreadStart, 0, NULL);
}