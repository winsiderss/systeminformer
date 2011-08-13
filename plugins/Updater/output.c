/*
* Process Hacker Update Checker - 
*   main program
* 
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

static NTSTATUS WorkerThreadStart(
	__in PVOID Parameter
	)
{
	int result;
	DWORD dwBytes;
	mxml_node_t *xmlNode, *xmlNode2, *xmlNode3, *xmlNode4, *xmlNode5;
	PPH_STRING local;
	DWORD dwContentLen = 0;				
	int xPercent = 0;	
	DWORD dwBufLen = sizeof(BUFFER_LEN);
	DWORD dwBytesRead = 0, dwBytesWritten = 0;
	HWND hwndDlg = (HWND)Parameter;
	
	DisposeHandles();

	local = PhGetPhVersion();

	InitializeConnection(TRUE, L"processhacker.sourceforge.net", L"/updater.php");

	// Send the HTTP request.
	if (HttpSendRequest(file, NULL, 0, NULL, 0))
	{
		if (HttpQueryInfo(file, HTTP_QUERY_CONTENT_LENGTH | HTTP_QUERY_FLAG_NUMBER, (LPVOID)&dwContentLen, &dwBufLen, 0))
		{
			char *buffer = (char*)PhAllocate(BUFFER_LEN);
			PPH_STRING summaryText;

			// Read the resulting xml into our buffer.
			while (InternetReadFile(file, buffer, BUFFER_LEN, &dwBytes))
			{
				if (dwBytes == 0)
				{
					// We're done.
					break;
				}
			}

			// Load our XML.
			xmlNode = mxmlLoadString(NULL, buffer, MXML_OPAQUE_CALLBACK);

			// Check our XML.
			if (xmlNode->type != MXML_ELEMENT)
			{
				mxmlDelete(xmlNode);

				return STATUS_FILE_CORRUPT_ERROR;
			}

			// Find the ver node.
			xmlNode2 = mxmlFindElement(xmlNode, xmlNode, "ver", NULL, NULL, MXML_DESCEND);
			// Find the reldate node.
			xmlNode3 = mxmlFindElement(xmlNode, xmlNode, "reldate", NULL, NULL, MXML_DESCEND);
			// Find the size node.
			xmlNode4 = mxmlFindElement(xmlNode, xmlNode, "size", NULL, NULL, MXML_DESCEND);
			// Find the sha1 node.
			xmlNode5 = mxmlFindElement(xmlNode, xmlNode, "sha1", NULL, NULL, MXML_DESCEND);

			// create a PPH_STRING from our ANSI node.
			summaryText = PhCreateStringFromAnsi(xmlNode2->child->value.opaque);	
			result = PhCompareUnicodeStringZNatural(summaryText->Buffer, L"2.10", TRUE);//, local->Buffer, TRUE); 				
			PhDereferenceObject(summaryText);

			switch (result)
			{
			case 1:
				{
					// create a PPH_STRING from our ANSI node.
					summaryText = PhCreateStringFromAnsi(xmlNode2->child->value.opaque);	
					summaryText = PhFormatString(L"Process Hacker %s is available.", summaryText->Buffer);
					SetDlgItemText(hwndDlg, IDC_MESSAGE, summaryText->Buffer);

					PhDereferenceObject(summaryText);

					// create a PPH_STRING from our ANSI node.
					summaryText = PhCreateStringFromAnsi(xmlNode3->child->value.opaque);	
					summaryText = PhFormatString(L"Released: %s", summaryText->Buffer);
					SetDlgItemText(hwndDlg, IDC_MESSAGE2, summaryText->Buffer);
					
					PhDereferenceObject(summaryText);

					// create a PPH_STRING from our ANSI node.
					summaryText = PhCreateStringFromAnsi(xmlNode4->child->value.opaque);	
					summaryText = PhFormatString(L"Size: %s", summaryText->Buffer);
					SetDlgItemText(hwndDlg, IDC_MESSAGE3, summaryText->Buffer);

					PhDereferenceObject(summaryText);

					ShowWindow(GetDlgItem(hwndDlg, IDYES), SW_SHOW);			
					// Enable the IDC_RELDATE text
					ShowWindow(GetDlgItem(hwndDlg, IDC_RELDATE), SW_SHOW);
					// Enable the IDC_SIZE text
					ShowWindow(GetDlgItem(hwndDlg, IDC_DLSIZE), SW_SHOW);
				}
				break;
			case 0:
				{	
					PPH_STRING summaryText;
					// create a PPH_STRING from our ANSI node.
					summaryText = PhCreateStringFromAnsi(xmlNode2->child->value.opaque);	
					summaryText = PhFormatString(L"You're running the latest version: %s", summaryText->Buffer);

					SetDlgItemText(hwndDlg, IDC_MESSAGE, summaryText->Buffer);

					PhDereferenceObject(summaryText);

					// Enable the Download/Install button
					EnableWindow(GetDlgItem(hwndDlg, IDYES), FALSE);
				}
				break;
			case -1:
				{	
					PPH_STRING summaryText = PhFormatString(L"You're running a newer version: %s", local->Buffer);

					SetDlgItemText(hwndDlg, IDC_MESSAGE, summaryText->Buffer);

					PhDereferenceObject(summaryText);
				}
				break;
			default:
				{			
					PPH_STRING summaryText;
					// create a PPH_STRING from our ANSI node.
					summaryText = PhCreateStringFromAnsi(xmlNode2->child->value.opaque);	
					summaryText = PhFormatString(L"You're running a newer version: %s", summaryText->Buffer);
				
					PhDereferenceObject(summaryText);

					LogEvent(L"Updater: Update check unknown result: %s", result);
				}
				break;
			}
		}
	}
	else
	{
		NTSTATUS status = PhGetLastWin32ErrorAsNtStatus();

		LogEvent(L"Updater: HttpSendRequest failed (%d)", status);

		mxmlDelete(xmlNode);
		mxmlDelete(xmlNode2);
		mxmlDelete(xmlNode3);
		mxmlDelete(xmlNode4);
		mxmlDelete(xmlNode5);

		DisposeHandles();

		return status;
	}

	PhDereferenceObject(local);

	return STATUS_SUCCESS;
}

static NTSTATUS DownloadWorkerThreadStart(
	__in PVOID Parameter
	)
{
	HANDLE dlFile;
	UINT dwRetVal = 0;
	DWORD dwTotalReadSize = 0;				
	INT xPercent = 0;
	DWORD cbHash = 0;
	TCHAR lpTempPathBuffer[MAX_PATH];
	HWND hwndDlg = (HWND)Parameter;
	HWND hwndProgress = GetDlgItem(hwndDlg,IDC_PROGRESS1);
	DWORD dwContentLen = 0;
	DWORD dwBufLen = sizeof(dwContentLen);
	DWORD dwBytesRead = 0, dwBytesWritten = 0;
		
	char *buffer[BUFFER_LEN];

	DisposeHandles();

	Sleep(1000);

	// Get the temp path env string (no guarantee it's a valid path).
	dwRetVal = GetTempPath(MAX_PATH, lpTempPathBuffer);

	if (dwRetVal > MAX_PATH || dwRetVal == 0)
	{
		NTSTATUS result = PhGetLastWin32ErrorAsNtStatus();

		LogEvent(PhFormatString(L"Updater: GetTempPath failed (%d)", result));

		return result;
	}	

	localFilePath = PhConcatStrings2(lpTempPathBuffer, L"processhacker-2.19-setup.exe");

	InitializeConnection(
		TRUE, 
		L"sourceforge.net", 
		L"/projects/processhacker/files/processhacker2/processhacker-2.19-setup.exe/download"
		);

	// Open output file
	dlFile = CreateFile(
		localFilePath->Buffer,
		GENERIC_WRITE,
		FILE_SHARE_WRITE,
		0,                     // handle cannot be inherited
		CREATE_ALWAYS,         // if file exists, delete it
		FILE_ATTRIBUTE_NORMAL,
		0);

	if (dlFile == INVALID_HANDLE_VALUE)
	{
		NTSTATUS result = PhGetLastWin32ErrorAsNtStatus();

		LogEvent(PhFormatString(L"Updater: GetTempPath failed (%d)", result));

		return result;
	}

	SetDlgItemText(hwndDlg, IDC_STATUS, L"Connecting");

	// Send the HTTP request.
	if (HttpSendRequest(file, NULL, 0, NULL, 0))
	{
		if (HttpQueryInfo(file, HTTP_QUERY_CONTENT_LENGTH | HTTP_QUERY_FLAG_NUMBER, (LPVOID)&dwContentLen, &dwBufLen, 0))
		{
			// Reset Progressbar state.
			PhSetWindowStyle(hwndProgress, PBS_MARQUEE, 0);

			while (InternetReadFile(file, buffer, BUFFER_LEN, &dwBytesRead))
			{
				if (dwBytesRead == 0)
				{
					// We're done.
					break;
				}

				dwTotalReadSize += dwBytesRead;
				xPercent = (int)(((double)dwTotalReadSize / (double)dwContentLen) * 100);

				SendMessage(hwndProgress, PBM_SETPOS, xPercent, 0);
				{
					PPH_STRING str;
					PPH_STRING dlCurrent = PhFormatSize(dwTotalReadSize, -1);
					PPH_STRING dlLength = PhFormatSize(dwContentLen, -1);

					str = PhFormatString(L"Downloaded: %d%% (%s)", xPercent, dlCurrent->Buffer);

					SetDlgItemText(hwndDlg, IDC_STATUS, str->Buffer);

					PhDereferenceObject(str);
					PhDereferenceObject(dlCurrent);
					PhDereferenceObject(dlLength);
				}

				if (!WriteFile(dlFile, buffer, dwBytesRead, &dwBytesWritten, NULL)) 
				{
					PPH_STRING str = PhFormatString(L"Updater: (DownloadWorkerThreadStart) WriteFile failed (%d)", PhGetLastWin32ErrorAsNtStatus());
					SetDlgItemText(hwndDlg, IDC_STATUS, str->Buffer);
					LogEvent(str);				
					PhDereferenceObject(str);
					break;
				}

				if (dwBytesRead != dwBytesWritten) 
				{	
					LogEvent(PhFormatString(L"Updater: (DownloadWorkerThreadStart) WriteFile dwBytesRead != dwBytesWritte (%d)", PhGetLastWin32ErrorAsNtStatus()));
					break;                
				}
			}
		}
		else
		{
			// No content length...impossible to calculate % complete so just read until we are done.
			DWORD dwBytesRead = 0;
			DWORD dwBytesWritten = 0;
			char *buffer[BUFFER_LEN];

			LogEvent(PhFormatString(L"Updater: (DownloadWorkerThreadStart) HttpQueryInfo failed (%d)", PhGetLastWin32ErrorAsNtStatus()));

			while (InternetReadFile(file, buffer, BUFFER_LEN, &dwBytesRead))
			{	
				if (dwBytesRead == 0)
				{
					// We're done.
					break;
				}

				if (!WriteFile(dlFile, buffer, dwBytesRead, &dwBytesWritten, NULL)) 
				{
					LogEvent(PhFormatString(L"Updater: (DownloadWorkerThreadStart) WriteFile failed (%d)", PhGetLastWin32ErrorAsNtStatus()));
					break;
				}

				if (dwBytesRead != dwBytesWritten) 
				{
					LogEvent(PhFormatString(L"Updater: (DownloadWorkerThreadStart) WriteFile failed (%d)", PhGetLastWin32ErrorAsNtStatus()));
					break;                
				}
			}
		}
	}
	else
	{
		NTSTATUS result = PhGetLastWin32ErrorAsNtStatus();

		LogEvent(PhFormatString(L"Updater: (DownloadWorkerThreadStart) HttpSendRequest failed (%d)", result));

		return result;
	}

	if (dlFile)
		NtClose(dlFile);  

	DisposeHandles();

	HashFile();

	SetDlgItemText(hwndDlg, IDC_STATUS, L"Download Complete");

	Install = TRUE;
	SetWindowText(GetDlgItem(hwndDlg, IDYES), L"Install");
	EnableWindow(GetDlgItem(hwndDlg, IDYES), TRUE);
	
	return STATUS_SUCCESS;
}

INT_PTR CALLBACK NetworkOutputDlgProc(      
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
			PhCenterWindow(hwndDlg, GetParent(hwndDlg));
			PhCreateThread(0, (PUSER_THREAD_START_ROUTINE)WorkerThreadStart, hwndDlg);  
			Install = FALSE;
		}
		break;
	case WM_COMMAND:
		{
			switch (LOWORD(wParam))
			{
			case IDCANCEL:
			case IDOK:
				{
					DisposeHandles();

					EndDialog(hwndDlg, IDOK);
				}
				break;
			case IDYES:
				{
					if (Install)
					{
						PhShellExecute(hwndDlg, localFilePath->Buffer, NULL);
						DisposeHandles();
						
						ProcessHacker_Destroy(PhMainWndHandle);
						return FALSE;
					}

					if (PhInstalledUsingSetup())
					{	
						HWND hwndProgress = GetDlgItem(hwndDlg, IDC_PROGRESS1);
						
						// Enable the progressbar
						ShowWindow(hwndProgress, SW_SHOW);
			            // Enable the status text
						ShowWindow(GetDlgItem(hwndDlg, IDC_STATUS), SW_SHOW);					    
						
						SetDlgItemText(hwndDlg, IDC_STATUS, L"Initializing");

						PhSetWindowStyle(hwndProgress, PBS_MARQUEE, PBS_MARQUEE);
						PostMessage(hwndProgress, PBM_SETMARQUEE, TRUE, 75);

						// Star our Downloader thread
						PhCreateThread(0, (PUSER_THREAD_START_ROUTINE)DownloadWorkerThreadStart, hwndDlg);   
					}
					else
					{
						NTSTATUS result = PhGetLastWin32ErrorAsNtStatus();

						LogEvent(PhFormatString(L"Updater: PhInstalledUsingSetup failed: %d", result));
					}
				}
				break;
			}
		}
		break;
	}

	return FALSE;
}

VOID InitializeConnection(BOOL useCache, PCWSTR host, PCWSTR path)
{
		// Initialize the wininet library.
	initialize = InternetOpen(
		L"PH Updater", // user-agent
		INTERNET_OPEN_TYPE_PRECONFIG, // use system proxy configuration	 
		NULL, 
		NULL, 
		0
		);

	if (!initialize)
	{
		LogEvent(PhFormatString(L"Updater: (InitializeConnection) InternetOpen failed (%d)", PhGetLastWin32ErrorAsNtStatus()));
	}

	// Connect to the server.
	connection = InternetConnect(
		initialize,
		host, 
		INTERNET_DEFAULT_HTTP_PORT,
		NULL, 
		NULL, 
		INTERNET_SERVICE_HTTP, 
		0, 
		0
		);

	if (!connection)
	{
		LogEvent(PhFormatString(L"Updater: (InitializeConnection) InternetConnect failed (%d)", PhGetLastWin32ErrorAsNtStatus()));
	}
	
	// Open the HTTP request.
	file = HttpOpenRequest(
		connection, 
		NULL, 
		path, 
		NULL, 
		NULL, 
		NULL, 
		useCache ? INTERNET_FLAG_DONT_CACHE : 0, // Specify this flag here to disable Internet* API caching.
		0
		);

	if (!file)
	{
		LogEvent(PhFormatString(L"Updater: (InitializeConnection) HttpOpenRequest failed (%d)", PhGetLastWin32ErrorAsNtStatus()));
	}
}

INT VersionParser(char* version1, char* version2) 
{
	INT i = 0, a1 = 0, b1 = 0, ret = 0;
	size_t a = strlen(version1); 
	size_t b = strlen(version2);

	if (b > a) 
		a = b;

	for (i = 0; i < a; i++) 
	{
		a1 += version1[i];
		b1 += version2[i];
	}

	if (b1 > a1)
	{
		// second version is fresher
		ret = 1; 
	}
	else if (b1 == a1) 
	{
		// versions is equal
		ret = 0;
	}
	else 
	{
		// first version is fresher
		ret = -1; 
	}

	return ret;
}

BOOL PhInstalledUsingSetup() 
{
	LONG result;
	HKEY hKey;

	// Check uninstall entries for the 'Process_Hacker2_is1' registry key.
	result = RegOpenKeyEx(HKEY_LOCAL_MACHINE, L"SOFTWARE\\Microsoft\\Windows\\CurrentVersion\\Uninstall\\Process_Hacker2_is1", 0, KEY_QUERY_VALUE, &hKey);

	// Cleanup
	NtClose(hKey);

	if (result != ERROR_SUCCESS)
		return FALSE;
	
	return TRUE;
}

VOID LogEvent(PPH_STRING str)
{
	PhLogMessageEntry(PH_LOG_ENTRY_MESSAGE, str);

	OutputDebugString(str->Buffer);
	
	PhDereferenceObject(str);
}

VOID DisposeHandles()
{
	if (file)
		InternetCloseHandle(file);

	if (connection)
		InternetCloseHandle(connection);

	if (initialize)
		InternetCloseHandle(initialize);

	//if (remoteVersion)
		//PhDereferenceObject(remoteVersion);

	//if (localFilePath)
		//PhDereferenceObject(localFilePath);
}


DWORD HashFile()
{
    DWORD dwStatus = 0;
    BOOL bResult = FALSE;
    HCRYPTPROV hProv = 0;
    HCRYPTHASH hHash = 0;
    HANDLE hFile = NULL;
    BYTE rgbFile[BUFFER_LEN];
    DWORD cbRead = 0;
    BYTE rgbHash[MD5LEN];
    DWORD cbHash = 0;
    CHAR rgbDigits[] = "0123456789abcdef";

    // Copyright (C) Microsoft.  All rights reserved.
    // Logic to check usage goes here.

    hFile = CreateFile(localFilePath->Buffer,
        GENERIC_READ,
        FILE_SHARE_READ,
        NULL,
        OPEN_EXISTING,
        FILE_FLAG_SEQUENTIAL_SCAN,
        NULL);

    if (INVALID_HANDLE_VALUE == hFile)
    {
        dwStatus = GetLastError();
        printf("Error opening file %s\nError: %d\n", L"", 
            dwStatus); 
        return dwStatus;
    }

    // Get handle to the crypto provider
    if (!CryptAcquireContext(&hProv,
        NULL,
        NULL,
        PROV_RSA_FULL,
        CRYPT_VERIFYCONTEXT))
    {
        dwStatus = GetLastError();
        printf("CryptAcquireContext failed: %d\n", dwStatus); 
        CloseHandle(hFile);
        return dwStatus;
    }

    if (!CryptCreateHash(hProv, CALG_MD5, 0, 0, &hHash))
    {
        dwStatus = GetLastError();
        printf("CryptAcquireContext failed: %d\n", dwStatus); 
        CloseHandle(hFile);
        CryptReleaseContext(hProv, 0);
        return dwStatus;
    }

    while (bResult = ReadFile(hFile, rgbFile, BUFFER_LEN, &cbRead, NULL))
    {
        if (0 == cbRead)
        {
            break;
        }

		        if (!CryptHashData(hHash, rgbFile, cbRead, 0))
        {
            dwStatus = GetLastError();
            printf("CryptHashData failed: %d\n", dwStatus); 
            CryptReleaseContext(hProv, 0);
            CryptDestroyHash(hHash);
            CloseHandle(hFile);
            return dwStatus;
        }
    }

    if (!bResult)
    {
        dwStatus = GetLastError();
        printf("ReadFile failed: %d\n", dwStatus); 
        CryptReleaseContext(hProv, 0);
        CryptDestroyHash(hHash);
        CloseHandle(hFile);
        return dwStatus;
    }

    cbHash = MD5LEN;
    if (CryptGetHashParam(hHash, HP_HASHVAL, rgbHash, &cbHash, 0))
    {
		DWORD i = 0;
		LogEvent(PhFormatString(L"Updater: MD5 hash of file is: (%d)", cbHash));

        for (i = 0; i < cbHash; i++)
        {
            LogEvent(PhFormatString(L"%d%d", rgbDigits[rgbHash[i] >> 4], rgbDigits[rgbHash[i] & 0xf]));
        }
    }
    else
    {
        dwStatus = GetLastError();
        //printf("CryptGetHashParam failed: %d\n", dwStatus); 
    }

    CryptDestroyHash(hHash);
    CryptReleaseContext(hProv, 0);
    CloseHandle(hFile);

    return dwStatus; 
}   

