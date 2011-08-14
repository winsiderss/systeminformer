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
	INT xPercent = 0, result = -2;
	DWORD status = 0, dwBytes = 0, dwContentLen = 0, dwBytesRead = 0, dwBytesWritten = 0, dwBufLen = sizeof(BUFFER_LEN);
	mxml_node_t *xmlNode, *xmlNode2, *xmlNode3, *xmlNode4, *xmlNode5;
	HWND hwndDlg = (HWND)Parameter;

	DisposeHandles();

	if (status = InitializeConnection(
		TRUE, 
		L"processhacker.sourceforge.net", 
		L"/updater.php"
		))
	{
		return status;
	}

	// Send the HTTP request.
	if (HttpSendRequest(file, NULL, 0, NULL, 0))
	{
		if (HttpQueryInfo(file, HTTP_QUERY_CONTENT_LENGTH | HTTP_QUERY_FLAG_NUMBER, (LPVOID)&dwContentLen, &dwBufLen, 0))
		{
			char buffer[BUFFER_LEN];

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
			
			{
				//char localVersion[5];
				//PPH_STRING local = PhGetPhVersion();
				//wtoc(localVersion, local->Buffer);
				
				//result = strncmp(xmlNode2->child->value.opaque, localVersion, 4);
				result = strncmp(xmlNode2->child->value.opaque, "2.10", 4); 

				//PhDereferenceObject(local);
			}

			switch (result)
			{
			case 1:
				{
					PPH_STRING summaryText, tempstr;

					// create a PPH_STRING from our ANSI node.
					tempstr = PhCreateStringFromAnsi(xmlNode2->child->value.opaque);	
					summaryText = PhFormatString(L"Process Hacker %s is available.", tempstr->Buffer);
					SetDlgItemText(hwndDlg, IDC_MESSAGE, summaryText->Buffer);

					PhDereferenceObject(tempstr);
					PhDereferenceObject(summaryText);

					// create a PPH_STRING from our ANSI node.
					tempstr = PhCreateStringFromAnsi(xmlNode3->child->value.opaque);	
					summaryText = PhFormatString(L"Released: %s", tempstr->Buffer);
					SetDlgItemText(hwndDlg, IDC_MESSAGE2, summaryText->Buffer);
					
					PhDereferenceObject(tempstr);
					PhDereferenceObject(summaryText);

					// create a PPH_STRING from our ANSI node.
					tempstr = PhCreateStringFromAnsi(xmlNode4->child->value.opaque);	
					summaryText = PhFormatString(L"Size: %s", tempstr->Buffer);
					SetDlgItemText(hwndDlg, IDC_MESSAGE3, summaryText->Buffer);

					PhDereferenceObject(tempstr);
					PhDereferenceObject(summaryText);

					ShowWindow(GetDlgItem(hwndDlg, IDYES), SW_SHOW);			
					// Enable the RELDATE text
					ShowWindow(GetDlgItem(hwndDlg, IDC_RELDATE), SW_SHOW);
					// Enable the SIZE text
					ShowWindow(GetDlgItem(hwndDlg, IDC_DLSIZE), SW_SHOW);
				}
				break;
			case 0:
				{	
					PPH_STRING summaryText, versionText;
					// create a PPH_STRING from our ANSI node.
					versionText = PhCreateStringFromAnsi(xmlNode2->child->value.opaque);	
					summaryText = PhFormatString(L"You're running the latest version: %s", versionText->Buffer);

					SetDlgItemText(hwndDlg, IDC_MESSAGE, summaryText->Buffer);

					PhDereferenceObject(versionText);
					PhDereferenceObject(summaryText);

					// Enable the Download/Install button
					EnableWindow(GetDlgItem(hwndDlg, IDYES), FALSE);
				}
				break;
			case -1:
				{	
					PPH_STRING localText = PhGetPhVersion();
					PPH_STRING summaryText = PhFormatString(L"You're running a newer version: %s", localText->Buffer);

					SetDlgItemText(hwndDlg, IDC_MESSAGE, summaryText->Buffer);

					PhDereferenceObject(localText);
					PhDereferenceObject(summaryText);
				}
				break;
			default:
				{			
					LogEvent(PhFormatString(L"Updater: Update check unknown result: %d", result));
				}
				break;
			}
		}
	}
	else
	{
		status = GetLastError();

		LogEvent(PhFormatString(L"Updater: (WorkerThreadStart) HttpSendRequest failed (%d)\r\n", status));

		DisposeHandles();

		return status;
	}

	return status;
}

static NTSTATUS DownloadWorkerThreadStart(
	__in PVOID Parameter
	)
{
	HANDLE dlFile;
	INT xPercent = 0;
	DWORD status = 0, dwRetVal = 0, dwTotalReadSize = 0, dwContentLen = 0, dwBytesRead = 0, dwBytesWritten = 0, dwBufLen = sizeof(dwContentLen);				
	HWND hwndDlg = (HWND)Parameter, hwndProgress = GetDlgItem(hwndDlg,IDC_PROGRESS1);

	DisposeHandles();

	Sleep(1000);

	if (status = InitializeConnection(
		TRUE, 
		L"sourceforge.net", 
		L"/projects/processhacker/files/processhacker2/processhacker-2.19-setup.exe/download"
		))
	{
		return status;
	}

	if (status = CreateTempPath())
	{
		return status;
	}

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
		status = GetLastError();

		LogEvent(PhFormatString(L"Updater: GetTempPath failed (%d)\r\n", status));

		return status;
	}

	SetDlgItemText(hwndDlg, IDC_STATUS, L"Connecting");

	// Send the HTTP request.
	if (HttpSendRequest(file, NULL, 0, NULL, 0))
	{
		if (HttpQueryInfo(file, HTTP_QUERY_CONTENT_LENGTH | HTTP_QUERY_FLAG_NUMBER, (LPVOID)&dwContentLen, &dwBufLen, 0))
		{
			char buffer[BUFFER_LEN];

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
					//PPH_STRING dlLength = PhFormatSize(dwContentLen, -1);

					str = PhFormatString(L"Downloaded: %d%% (%s)", xPercent, dlCurrent->Buffer);

					SetDlgItemText(hwndDlg, IDC_STATUS, str->Buffer);

					PhDereferenceObject(str);
					PhDereferenceObject(dlCurrent);
					//PhDereferenceObject(dlLength);
				}

				if (!WriteFile(dlFile, buffer, dwBytesRead, &dwBytesWritten, NULL)) 
				{
					LogEvent(PhFormatString(L"Updater: (DownloadWorkerThreadStart) WriteFile failed (%d)\r\n", GetLastError()));
					break;
				}

				if (dwBytesRead != dwBytesWritten) 
				{	
					LogEvent(PhFormatString(L"Updater: (DownloadWorkerThreadStart) WriteFile dwBytesRead != dwBytesWritte (%d)\r\n", GetLastError()));
					break;                
				}
			}
		}
		else
		{
			// No content length...impossible to calculate % complete so just read until we are done.
			DWORD dwBytesRead = 0;
			DWORD dwBytesWritten = 0;
			char buffer[BUFFER_LEN];

			LogEvent(PhFormatString(L"Updater: (DownloadWorkerThreadStart) HttpQueryInfo failed (%d)\r\n", GetLastError()));

			while (InternetReadFile(file, buffer, BUFFER_LEN, &dwBytesRead))
			{	
				if (dwBytesRead == 0)
				{
					// We're done.
					break;
				}

				if (!WriteFile(dlFile, buffer, dwBytesRead, &dwBytesWritten, NULL)) 
				{
					LogEvent(PhFormatString(L"Updater: (DownloadWorkerThreadStart) WriteFile failed (%d)\r\n", GetLastError()));
					break;
				}

				if (dwBytesRead != dwBytesWritten) 
				{
					LogEvent(PhFormatString(L"Updater: (DownloadWorkerThreadStart) WriteFile failed (%d)\r\n", GetLastError()));
					break;                
				}
			}
		}
	}
	else
	{
		status = GetLastError();

		LogEvent(PhFormatString(L"Updater: (DownloadWorkerThreadStart) HttpSendRequest failed (%d)\r\n", status));

		return status;
	}

	if (dlFile)
		NtClose(dlFile);  

	DisposeHandles();

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
						// handle other installation types
					}
				}
				break;
			}
		}
		break;
	}

	return FALSE;
}

DWORD InitializeConnection(
	__in BOOL useCache, 
	__in PCWSTR host, 
	__in PCWSTR path
	)
{
	DWORD status = 0;

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
		status = GetLastError();

		LogEvent(PhFormatString(L"Updater: (InitializeConnection) InternetOpen failed (%d)\r\n", status));

		return status;
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
		status = GetLastError();

		LogEvent(PhFormatString(L"Updater: (InitializeConnection) InternetConnect failed (%d)\r\n", status));

		return status;
	}
	
	// Open the HTTP request.
	file = HttpOpenRequest(
		connection, 
		NULL, 
		path, 
		NULL, 
		NULL, 
		NULL, 
		useCache ? 0 : INTERNET_FLAG_DONT_CACHE,
		0
		);

	if (!file)
	{
		status = GetLastError();

		LogEvent(PhFormatString(L"Updater: (InitializeConnection) HttpOpenRequest failed (%d)\r\n", status));

		return status;
	}

	return status;
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
	{
		LogEvent(PhFormatString(L"Updater: (PhInstalledUsingSetup) RegOpenKeyEx failed (%d)\r\n", result));

		return FALSE;
	}
	
	return TRUE;
}

VOID LogEvent(
	__in PPH_STRING str
	)
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

DWORD CreateTempPath()
{
	TCHAR lpTempPathBuffer[MAX_PATH];
	DWORD length = 0;

	// Get the temp path env string (no guarantee it's a valid path).
	length = GetTempPath(MAX_PATH, lpTempPathBuffer);

	if (length > MAX_PATH || length == 0)
	{
		DWORD status = GetLastError();

		LogEvent(PhFormatString(L"Updater: GetTempPath failed (%d)", status));

		return status;
	}	

	localFilePath = PhConcatStrings2(lpTempPathBuffer, L"processhacker-2.19-setup.exe");

	return 0;
}

/*
 This function will convert a WCHAR string to a CHAR string.

 Param 1 :: Pointer to a buffer that will contain the converted string. Ensure this buffer is large enough; if not, buffer overrun errors will occur.
 Param 2 :: Constant pointer to a source WCHAR string to be converted to CHAR
*/
void wtoc(CHAR* dest, const WCHAR* source)
{
	int i = 0;

	while(source[i] != '\0')
	{
		dest[i] = (CHAR)source[i];
		++i;
	}
}
