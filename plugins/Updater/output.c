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

#pragma comment(lib, "Wininet.lib")

#include "phdk.h"
#include "updater.h"
#include "resource.h"

static volatile BOOL Install = FALSE;
static HINTERNET initialize, connection, file;
static PPH_STRING remoteVersion;
static PPH_STRING localFilePath;

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


static NTSTATUS WorkerThreadStart(__in PVOID Parameter)
{
	LONG result;
	DWORD dwBytes;
	CHAR buf[1024];
	mxml_node_t *xmlNode;
	PPH_STRING local;
	HINTERNET initialize, connection, file;

	HWND hwndDlg = (HWND)Parameter;
	local = PhGetPhVersion();

	// Initialize the wininet library.
	initialize = InternetOpen(
		L"PH Updater", // user-agent
		INTERNET_OPEN_TYPE_PRECONFIG, // use system proxy configuration	 
		NULL, 
		NULL, 
		0
		);

	// Connect to the server.
	connection = InternetConnect(
		initialize,
		L"processhacker.sourceforge.net", 
		INTERNET_DEFAULT_HTTP_PORT,
		NULL, 
		NULL, 
		INTERNET_SERVICE_HTTP, 
		0, 
		0
		);

	// Open the HTTP request.
	file = HttpOpenRequest(
		connection, 
		NULL, 
		L"/updater.php", 
		NULL, 
		NULL, 
		NULL, 
		INTERNET_FLAG_DONT_CACHE, // Specify this flag here to disable Internet* API caching.
		0
		);

	// Send the HTTP request.
	if (HttpSendRequest(file, NULL, 0, NULL, 0))
	{
		memset(&buf, 0, sizeof(buf));

		// Read the resulting xml into our buffer.
		while (InternetReadFile(file, &buf, sizeof(buf), &dwBytes))
		{
			if (dwBytes == 0)
			{
				// We're done.
				break;
			}
		}
	}
	else
	{
		InternetCloseHandle(file);
		InternetCloseHandle(connection);
		InternetCloseHandle(initialize);

		// we return NTSTATUS codes for our thread, return the last win32 error as one.
		return PhGetLastWin32ErrorAsNtStatus();
	}

	// Check our buffer (dont know if this is correct)
	if (buf == NULL || strlen(buf) == 0)
		return STATUS_FILE_CORRUPT_ERROR;

	// Load our XML.
	xmlNode = mxmlLoadString(NULL, buf, MXML_NO_CALLBACK); // MXML_OPAQUE_CALLBACK

	// Check our XML.
	if (xmlNode->type != MXML_ELEMENT)
		return STATUS_FILE_CORRUPT_ERROR;

	// Find the ver node.
	xmlNode = mxmlFindElement(xmlNode->child, xmlNode, "ver", NULL, NULL, MXML_DESCEND);

	// create a PPH_STRING from our ANSI node.
	remoteVersion = PhCreateStringFromAnsi(xmlNode->child->value.text.string);

	//{
	//	mxml_node_t *xmlNode2;
	//	PPH_STRING remote2;
	//	// Find the ver node.
	//	xmlNode2 = mxmlFindElement(xmlNode->child, xmlNode, "reldate", NULL, NULL, MXML_DESCEND);

	//	// create a PPH_STRING from our ANSI node.
	//	remote2 = PhCreateStringFromAnsi(xmlNode->child->value.text.string);
	//}

	// Compare our version strings (You can replace local->Buffer or remoteVersion->Buffer with say L"2.10" for testing).
	result = PhCompareUnicodeStringZNatural(L"2.20", L"2.10", TRUE);

	switch (result)
	{
	case 1:
		{
			PPH_STRING summaryText = PhFormatString(L"Process Hacker %s is available.", remoteVersion->Buffer);
						
			ShowWindow(GetDlgItem(hwndDlg, IDYES), SW_SHOW);
			SetDlgItemText(hwndDlg, IDC_MESSAGE, summaryText->Buffer);

			PhDereferenceObject(summaryText);
		}
		break;
	case 0:
		{
			PPH_STRING summaryText = PhFormatString(L"You're running the latest version: %s", remoteVersion->Buffer);

			SetDlgItemText(hwndDlg, IDC_MESSAGE, summaryText->Buffer);

			PhDereferenceObject(summaryText);
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
			OutputDebugString(L"PH Update Check: Unknown Result.");
		}
		break;
	}

	PhDereferenceObject(local);

	/*close file , terminate server connection and deinitialize the wininet library*/
	DisposeHandles();

	return STATUS_SUCCESS;
}

static NTSTATUS DownloadWorkerThreadStart(
	__in PVOID Parameter
	)
{
	HANDLE dlFile;
	UINT dwRetVal = 0;
    TCHAR lpTempPathBuffer[MAX_PATH];
	HWND hwndDlg = (HWND)Parameter;
	HWND hwndProgress = GetDlgItem(hwndDlg,IDC_PROGRESS1);

	ShowWindow(GetDlgItem(hwndDlg, IDC_STATUS), SW_SHOW);
	
	SetDlgItemText(hwndDlg, IDC_STATUS, L"Initializing");

	PhSetWindowStyle(hwndProgress, PBS_MARQUEE, PBS_MARQUEE);
	PostMessage(hwndProgress, PBM_SETMARQUEE, TRUE, 75);

	// Get the temp path env string (no guarantee it's a valid path).
	dwRetVal = GetTempPath(MAX_PATH, lpTempPathBuffer);

	if (dwRetVal > MAX_PATH || dwRetVal == 0)
	{
		OutputDebugString(L"GetTempPath failed");
		return PhGetLastWin32ErrorAsNtStatus();
	}	

	localFilePath = PhConcatStrings2(lpTempPathBuffer, L"processhacker-2.19-setup.exe");

	// Initialize the wininet library.
	initialize = InternetOpen(
		L"PH Updater", // user-agent
		INTERNET_OPEN_TYPE_PRECONFIG, // use system proxy configuration	 
		NULL, 
		NULL, 
		0
		);

	// Connect to the server.
	connection = InternetConnect(
		initialize,
		L"sourceforge.net", 
		INTERNET_DEFAULT_HTTP_PORT,
		NULL, 
		NULL, 
		INTERNET_SERVICE_HTTP, 
		0, 
		0
		);
	
	// Open the HTTP request.
	file = HttpOpenRequest(
		connection, 
		NULL, 
		L"/projects/processhacker/files/processhacker2/processhacker-2.19-setup.exe/download", 
		NULL, 
		NULL, 
		NULL, 
		INTERNET_FLAG_DONT_CACHE, // Specify this flag here to disable Internet* API caching.
		0
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
		return PhGetLastWin32ErrorAsNtStatus();
	}

	SetDlgItemText(hwndDlg, IDC_STATUS, L"Connecting");

	// Send the HTTP request.
	if (HttpSendRequest(file, NULL, 0, NULL, 0))
	{
		DWORD dwContentLen = 0;
		DWORD dwBufLen = sizeof(dwContentLen);
		DWORD dwBytesRead = 0, dwBytesWritten = 0;
		INT xPercent = 0;

		if (HttpQueryInfo(file, HTTP_QUERY_CONTENT_LENGTH | HTTP_QUERY_FLAG_NUMBER, (LPVOID)&dwContentLen, &dwBufLen, 0))
		{
			char statusText[32];
			char *buffer = (char*)PhAllocate(dwContentLen + 1);
			DWORD dwTotalReadSize = 0;

			// Reset Progressbar state.
			PhSetWindowStyle(hwndProgress, PBS_MARQUEE, ~PBS_MARQUEE);

			while (InternetReadFile(file, buffer, 1024, &dwBytesRead))
			{
				if (dwBytesRead == 0)
				{
					// We're done.
					break;
				}

				dwTotalReadSize += dwBytesRead;
				xPercent = (int)(((double)dwTotalReadSize / (double)dwContentLen) * 100);

				PostMessage(hwndProgress, PBM_SETPOS, xPercent, 0);
				{
					// Begin ugly code that needs to be rewritten.
					PPH_STRING str;
					sprintf(statusText, "Downloaded: %d%%", xPercent);
					str = PhCreateStringFromAnsi(statusText);

					SetDlgItemText(hwndDlg, IDC_STATUS, str->Buffer);

					PhDereferenceObject(str);
				}

				if (!WriteFile(dlFile, buffer, dwBytesRead, &dwBytesWritten, NULL)) 
				{
					OutputDebugString(L"WriteFile failed");
					
					return PhGetLastWin32ErrorAsNtStatus();
				}

				if (dwBytesRead != dwBytesWritten) 
				{
					// File write error
					break;                
				}

			}

			PhFree(buffer);
		}
		else
		{
			// DWORD err = GetLastError();
			// No content length...impossible to calculate % complete so just read until we are done.
			DWORD dwBytesRead = 0;
			DWORD dwBytesWritten = 0;
			char *buffer = (char*)PhAllocate(1024);
		
			while (InternetReadFile(file, buffer, 1024, &dwBytesRead))
			{	
				if (dwBytesRead == 0)
				{
					// We're done.
					break;
				}

				if (!WriteFile(dlFile, buffer, dwBytesRead, &dwBytesWritten, NULL)) 
				{
					OutputDebugString(L"WriteFile failed");
					return PhGetLastWin32ErrorAsNtStatus();
				}

				if (dwBytesRead != dwBytesWritten) 
				{
					// File write error
					break;                
				}
			}

			PhFree(buffer);
		}
	}
	else
	{
		//OutputDebugString(L"HttpSendRequest failed");

		PPH_STRING str = PhFormatString(L"HttpSendRequest failed %s", PhGetLastWin32ErrorAsNtStatus());

		PhLogMessageEntry(PH_LOG_ENTRY_MESSAGE, str);

		// we return NTSTATUS codes for our thread, return the last win32 error as one.
		return PhGetLastWin32ErrorAsNtStatus();
	}

	if (dlFile)
		NtClose(dlFile);  

	DisposeHandles();

	SetDlgItemText(hwndDlg, IDC_STATUS, L"Download Complete");

	SetWindowText(GetDlgItem(hwndDlg, IDYES), L"Install");
	EnableWindow(GetDlgItem(hwndDlg, IDYES), TRUE);

	Install = TRUE;

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
	case WM_CLOSE:
	case WM_DESTROY:
		{
			DisposeHandles();

			EndDialog(hwndDlg, IDOK);
		}
		break;
	case WM_COMMAND:
		{
			switch (LOWORD(wParam))
			{
			case IDOK:
				{
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
						// Enable the progressbar
						ShowWindow(GetDlgItem(hwndDlg, IDC_PROGRESS1), SW_SHOW);
						// Enable the Download/Install button
						EnableWindow(GetDlgItem(hwndDlg, IDYES), FALSE);
						// Star our Downloader thread
						PhCreateThread(0, (PUSER_THREAD_START_ROUTINE)DownloadWorkerThreadStart, hwndDlg);   
					}
					else
					{
						OutputDebugString(L"Key Does Not Exist.\n");
					}
				}
				break;
			}
		}
		break;
	}

	return FALSE;
}


BOOL PhInstalledUsingSetup() 
{
	long lRet;
	HKEY hKey;
	char temp[150];
	DWORD dwBufLen;

	// Open location
	lRet = RegOpenKeyEx(HKEY_LOCAL_MACHINE, L"SOFTWARE\\Microsoft\\Windows\\CurrentVersion\\Uninstall\\Process_Hacker2_is1", 0, KEY_QUERY_VALUE, &hKey);

	if (lRet != ERROR_SUCCESS)
		return FALSE;

	dwBufLen = (DWORD)PhAllocate((SIZE_T)&temp);

	// Get key
	lRet = RegQueryValueEx( hKey, L"InstallLocation", NULL, NULL, (BYTE*)&temp, &dwBufLen);

	if (lRet != ERROR_SUCCESS)
		return FALSE;

	// Close key
	NtClose(hKey);
	
	// Got this far, then key exists
	return TRUE;
}