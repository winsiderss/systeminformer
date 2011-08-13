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
	LONG result;
	DWORD dwBytes;
	mxml_node_t *xmlNode;
	PPH_STRING local;

	HWND hwndDlg = (HWND)Parameter;
	local = PhGetPhVersion();

	DisposeHandles();

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
		DWORD dwContentLen = 0;
		DWORD dwBufLen = sizeof(dwContentLen);
		DWORD dwBytesRead = 0, dwBytesWritten = 0;
		INT xPercent = 0;

		if (HttpQueryInfo(file, HTTP_QUERY_CONTENT_LENGTH | HTTP_QUERY_FLAG_NUMBER, (LPVOID)&dwContentLen, &dwBufLen, 0))
		{
			char *buffer = (char*)PhAllocate(BUFFER_LEN);

			// Read the resulting xml into our buffer.
			while (InternetReadFile(file, buffer, BUFFER_LEN, &dwBytes))
			{
				if (dwBytes == 0)
				{
					// We're done.
					break;
				}
			}



			// Check our buffer (dont know if this is correct)
			if (buffer == NULL || strlen(buffer) == 0)
				return STATUS_FILE_CORRUPT_ERROR;

			// Load our XML.
			xmlNode = mxmlLoadString(NULL, buffer, MXML_NO_CALLBACK); // MXML_OPAQUE_CALLBACK

			// Check our XML.
			if (xmlNode->type != MXML_ELEMENT)
			{
				mxmlDelete(xmlNode);

				return STATUS_FILE_CORRUPT_ERROR;
			}

			// Find the ver node.
			xmlNode = mxmlFindElement(xmlNode->child, xmlNode, "ver", NULL, NULL, MXML_DESCEND);

			// create a PPH_STRING from our ANSI node.
			remoteVersion = PhCreateStringFromAnsi(xmlNode->child->value.text.string);

			// Compare our version strings (You can replace local->Buffer or remoteVersion->Buffer with say L"2.10" for testing).
			result = PhCompareUnicodeStringZNatural(remoteVersion->Buffer, L"2.10", TRUE);

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
		}
	}
	else
	{
		DisposeHandles();

		return PhGetLastWin32ErrorAsNtStatus();
	}

	PhDereferenceObject(local);

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

	DisposeHandles();

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
			char *buffer = (char*)PhAllocate(BUFFER_LEN);
			DWORD dwTotalReadSize = 0;

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

				PostMessage(hwndProgress, PBM_SETPOS, xPercent, 0);
				{
					PPH_STRING str = PhFormatString(L"Downloaded: %d%%", xPercent);

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
			char *buffer = (char*)PhAllocate(BUFFER_LEN);
		
			while (InternetReadFile(file, buffer, BUFFER_LEN, &dwBytesRead))
			{	
				if (dwBytesRead == 0)
				{
					// We're done.
					break;
				}

				if (!WriteFile(dlFile, buffer, dwBytesRead, &dwBytesWritten, NULL)) 
				{
					NTSTATUS result = PhGetLastWin32ErrorAsNtStatus();

					LogEvent(L"WriteFile failed %s", result);

					return result;
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
		NTSTATUS result = PhGetLastWin32ErrorAsNtStatus();

		LogEvent(L"HttpSendRequest failed %s", result);

		return result;
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
	case WM_DESTROY:
		{
			DisposeHandles();
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

VOID LogEvent(__in __format_string PWSTR Format, __in __format_string PWSTR extra)
{
	PPH_STRING msgString = PhFormatString(Format, extra);

	PhLogMessageEntry(PH_LOG_ENTRY_MESSAGE, msgString);

	OutputDebugString(msgString->Buffer);

	PhDereferenceObject(msgString);
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
