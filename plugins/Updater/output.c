/*
* Process Hacker Update Checker - 
*   main window
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
	HWND hwndDlg = Parameter, hwndProgress = GetDlgItem(hwndDlg, IDC_PROGRESS1);
	mxml_node_t 
		*xmlNode, 
		*xmlNode2, 
		*xmlNode3, 
		*xmlNode4, 
		*xmlNode5;
	DWORD
		dwRetVal = 0, 
		dwTotalReadSize = 0, 
		dwBytes = 0, 
		dwContentLen = 0, 
		dwBytesRead = 0,
		dwBytesWritten = 0, 
		dwBufLen = sizeof(BUFFER_LEN);

	if (dwRetVal = InitializeConnection(
		EnableCache, 
		L"processhacker.sourceforge.net", 
		L"/updater.php"
		))
	{
		return dwRetVal;
	}

	// Send the HTTP request.
	if (HttpSendRequest(NetRequest, NULL, 0, NULL, 0))
	{
		char buffer[BUFFER_LEN];
		char *tmpBuffer = NULL;

		ZeroMemory(buffer, BUFFER_LEN); //set the entire buffer to NULL

		// Read the resulting xml into our buffer.
		while (InternetReadFile(NetRequest, buffer, BUFFER_LEN, &dwBytes))
		{
			if (dwBytes == 0)
				break;

			//guarantee our buffer to be no longer than the amount read, and guarantee to be zerod.
			tmpBuffer = (char*)PhAllocate(dwBytes);

			RtlZeroMemory(tmpBuffer, dwBytes); //wipes the entire buffer, not absolutely necessary here.
			RtlCopyMemory(tmpBuffer, buffer, dwBytes); 
		}

		// Load our XML.
		xmlNode = mxmlLoadString(NULL, tmpBuffer, MXML_OPAQUE_CALLBACK);

		PhFree(tmpBuffer);

		// Check our XML.
		if (xmlNode == NULL || xmlNode->type != MXML_ELEMENT)
		{
			mxmlRelease(xmlNode);

			LogEvent(PhFormatString(L"Updater: (WorkerThreadStart) mxmlLoadString failed."));

			SetDlgItemText(hwndDlg, IDC_MESSAGE, L"There was an error downloading the xml.");

			return STATUS_FILE_CORRUPT_ERROR;
		}

		// Find the ver node.
		xmlNode2 = mxmlFindElement(xmlNode, xmlNode, "ver", NULL, NULL, MXML_DESCEND);
		// Find the reldate node.
		xmlNode3 = mxmlFindElement(xmlNode, xmlNode, "reldate", NULL, NULL, MXML_DESCEND);
		// Find the size node.
		xmlNode4 = mxmlFindElement(xmlNode, xmlNode, "size", NULL, NULL, MXML_DESCEND);

		switch (HashAlgorithm)
		{
		case Md5HashAlgorithm:
			{
				// Find the sha1 node.
				xmlNode5 = mxmlFindElement(xmlNode, xmlNode, "md5", NULL, NULL, MXML_DESCEND);
			}
			break;
		default: 
			{
				// Find the md5 node.
				xmlNode5 = mxmlFindElement(xmlNode, xmlNode, "sha1", NULL, NULL, MXML_DESCEND);
			}
			break;
		}

		result = strcmp(xmlNode2->child->value.opaque, "2.10"); 

		switch (result)
		{
		case 1:
			{
				PPH_STRING summaryText, tempstr;

				tempstr = PhCreateStringFromAnsi(xmlNode2->child->value.opaque);	
				summaryText = PhFormatString(L"Process Hacker %s is available.", tempstr->Buffer);
				SetDlgItemText(hwndDlg, IDC_MESSAGE, summaryText->Buffer);

				PhDereferenceObject(tempstr);
				PhDereferenceObject(summaryText);

				tempstr = PhCreateStringFromAnsi(xmlNode3->child->value.opaque);	
				summaryText = PhFormatString(L"Released: %s", tempstr->Buffer);
				SetDlgItemText(hwndDlg, IDC_DLSIZE, summaryText->Buffer);

				PhDereferenceObject(tempstr);
				PhDereferenceObject(summaryText);

				tempstr = PhCreateStringFromAnsi(xmlNode4->child->value.opaque);	
				summaryText = PhFormatString(L"Size: %s", tempstr->Buffer);
				SetDlgItemText(hwndDlg, IDC_RELDATE, summaryText->Buffer);

				PhDereferenceObject(tempstr);
				PhDereferenceObject(summaryText);

				RemoteHashString = PhCreateAnsiString(xmlNode5->child->value.opaque);

				EnableWindow(GetDlgItem(hwndDlg, IDYES), TRUE);		
				ShowWindow(GetDlgItem(hwndDlg, IDC_RELDATE), SW_SHOW);
				ShowWindow(GetDlgItem(hwndDlg, IDC_DLSIZE), SW_SHOW);
			}
			break;
		case 0:
			{	
				PPH_STRING summaryText, versionText;

				versionText = PhCreateStringFromAnsi(xmlNode2->child->value.opaque);	
				summaryText = PhFormatString(L"You're running the latest version: %s", versionText->Buffer);

				SetDlgItemText(hwndDlg, IDC_MESSAGE, summaryText->Buffer);

				PhDereferenceObject(versionText);
				PhDereferenceObject(summaryText);

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
	else
	{
		dwRetVal = GetLastError();

		LogEvent(PhFormatString(L"Updater: (WorkerThreadStart) HttpSendRequest failed (%d)", dwRetVal));

		return dwRetVal;
	}

	mxmlRelease(xmlNode);
	DisposeConnection();

	PhUpdaterState = Downloading;

	return dwRetVal;
}

static NTSTATUS DownloadWorkerThreadStart(
	__in PVOID Parameter
	)
{
	INT i = 0, xPercent = 0;
	LONG hashLength = 0;
	DWORD dwRetVal = 0, dwTotalReadSize = 0, dwContentLen = 0, dwBytesRead = 0, dwBytesWritten = 0, dwBufLen = sizeof(dwContentLen);					
	PH_HASH_CONTEXT hashContext = { 0 };
	HWND hwndDlg = Parameter, hwndProgress = GetDlgItem(hwndDlg, IDC_PROGRESS1);

	if (dwRetVal = InitializeConnection(
		EnableCache,
		L"sourceforge.net",
		L"/projects/processhacker/files/processhacker2/processhacker-2.19-setup.exe/download?use_mirror=waix"
		))
	{
		return dwRetVal;
	}

	if (dwRetVal = InitializeFile())
		return dwRetVal;

	Updater_SetStatusText(hwndDlg, L"Connecting");
	EnableWindow(GetDlgItem(hwndDlg, IDYES), FALSE);

	if (HttpSendRequest(NetRequest, NULL, 0, NULL, 0))
	{
		char buffer[BUFFER_LEN];
		char *tmpBuffer = NULL;

		if (HttpQueryInfo(NetRequest, HTTP_QUERY_CONTENT_LENGTH | HTTP_QUERY_FLAG_NUMBER, (LPVOID)&dwContentLen, &dwBufLen, 0))
		{
			ZeroMemory(buffer, BUFFER_LEN); //set the entire buffer to NULL

			// Reset Progressbar state.
			PhSetWindowStyle(hwndProgress, PBS_MARQUEE, 0);
			// Initialize hash algorithm.
			PhInitializeHash(&hashContext, HashAlgorithm);

			// (BUFFER_LEN - 1) make sure we read less than how much is in the buffer
			while (InternetReadFile(NetRequest, buffer, BUFFER_LEN, &dwBytesRead)) 
			{
				if (dwBytesRead == 0)
					break;

				//guarantee our buffer to be no longer than the amount read, and guarantee to be zerod.
				tmpBuffer = (char*)malloc(dwBytesRead);
				
				RtlZeroMemory(tmpBuffer, dwBytesRead); //wipes the entire buffer, not absolutely necessary here.
				RtlCopyMemory(tmpBuffer, buffer, dwBytesRead); 
	
				dwTotalReadSize += dwBytesRead;
				xPercent = (int)(((double)dwTotalReadSize / (double)dwContentLen) * 100);

				SendMessage(hwndProgress, PBM_SETPOS, xPercent, 0);
				{
					PPH_STRING str;
					PPH_STRING dlCurrent = PhFormatSize(dwTotalReadSize, -1);
					//PPH_STRING dlLength = PhFormatSize(dwContentLen, -1);

					str = PhFormatString(L"Downloaded: %d%% (%s)", xPercent, dlCurrent->Buffer);

					Updater_SetStatusText(hwndDlg, str->Buffer);

					PhDereferenceObject(str);
					PhDereferenceObject(dlCurrent);
					//PhDereferenceObject(dlLength);
				}

				PhUpdateHash(&hashContext, tmpBuffer, dwBytesRead);

				if (!WriteFile(TempFileHandle, tmpBuffer, dwBytesRead, &dwBytesWritten, NULL)) 
				{
					dwRetVal = GetLastError();
					LogEvent(PhFormatString(L"Updater: (DownloadWorkerThreadStart) WriteFile failed (%d)", dwRetVal));
					
					if (tmpBuffer)
						free(tmpBuffer);
					
					return dwRetVal;
				}

				if (dwBytesRead != dwBytesWritten) 
				{		
					dwRetVal = GetLastError();
					LogEvent(PhFormatString(L"Updater: (DownloadWorkerThreadStart) WriteFile failed (%d)", dwRetVal));
					
					if (tmpBuffer)
						free(tmpBuffer);
					
					return dwRetVal;                 
				}

				if (tmpBuffer)
					free(tmpBuffer);
			}
		}
		else
		{
			// No content length...impossible to calculate % complete so just read until we are done.
			LogEvent(PhFormatString(L"Updater: (DownloadWorkerThreadStart) HttpQueryInfo failed (%d)", GetLastError()));

			while (InternetReadFile(NetRequest, buffer, BUFFER_LEN, &dwBytesRead))
			{	
				if (dwBytesRead == 0)
					break;

				//guarantee our buffer to be no longer than the amount read, and guarantee to be zerod.
				tmpBuffer = (char*)PhAllocate(dwBytesRead);
				
				RtlZeroMemory(tmpBuffer, dwBytesRead); //wipes the entire buffer, not absolutely necessary here.
				RtlCopyMemory(tmpBuffer, buffer, dwBytesRead); 

				PhUpdateHash(&hashContext, tmpBuffer, dwBytesRead);

				if (!WriteFile(TempFileHandle, tmpBuffer, dwBytesRead, &dwBytesWritten, NULL)) 
				{
					dwRetVal = GetLastError();
					LogEvent(PhFormatString(L"Updater: (DownloadWorkerThreadStart) WriteFile failed (%d)\r\n", dwRetVal));
										
					if (tmpBuffer)
						free(tmpBuffer);
					
					return dwRetVal;
				}

				if (dwBytesRead != dwBytesWritten) 
				{
					dwRetVal = GetLastError();
					LogEvent(PhFormatString(L"Updater: (DownloadWorkerThreadStart) WriteFile failed (%d)\r\n", dwRetVal));

					if (tmpBuffer)
						free(tmpBuffer);

					return dwRetVal;             
				}

				free(tmpBuffer);
			}
		}

		Updater_SetStatusText(hwndDlg, L"Download Complete");

		DisposeConnection();
		DisposeFileHandles();

		// Enable Install button before hashing (user might not care about file hash reault)
		SetWindowText(GetDlgItem(hwndDlg, IDYES), L"Install");
		EnableWindow(GetDlgItem(hwndDlg, IDYES), TRUE);
		PhUpdaterState = Installing;

		Sleep(1000);

		{
			char hashBuffer[20];

			if (PhFinalHash(&hashContext, hashBuffer, 20, &hashLength))
			{
				INT strResult = -2;
				PH_ANSI_STRING *str;				
				PH_STRING_BUILDER sb = { 0 };

				PhInitializeStringBuilder(&sb, 100);

				for (i = 0; i < hashLength; i++)
				{
					PH_STRING *str = PhFormatString(L"%02x", 0xFF & hashBuffer[i]);

					PhAppendStringBuilder(&sb, str);

					PhDereferenceObject(str);
				}

				str = PhCreateAnsiStringFromUnicode(sb.String->Buffer);
				strResult = strcmp(str->Buffer, RemoteHashString->Buffer); 

				PhDereferenceObject(str);
				PhDeleteStringBuilder(&sb);

				switch (strResult)
				{
				case 0:
					{
						Updater_SetStatusText(hwndDlg, L"Hash verified");
					}
					break;
				default:
					{
						Updater_SetStatusText(hwndDlg, L"Hash verification failed");
					}
					break;
				}
			}
			else
			{
				Updater_SetStatusText(hwndDlg, L"Hash verification failed");
			}
		}
	}
	else
	{
		dwRetVal = GetLastError();

		LogEvent(PhFormatString(L"Updater: (DownloadWorkerThreadStart) HttpSendRequest failed (%d)\r\n", dwRetVal));

		return dwRetVal;
	}

    return dwRetVal; 
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
			PhCenterWindow(hwndDlg, GetParent(hwndDlg));
			
			EnableCache = PhGetIntegerSetting(L"ProcessHacker.Updater.EnableCache");
			HashAlgorithm = PhGetIntegerSetting(L"ProcessHacker.Updater.HashAlgorithm");
			PhUpdaterState = Default;

			PhCreateThread(0, (PUSER_THREAD_START_ROUTINE)WorkerThreadStart, hwndDlg);  
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
			case IDYES:
				{
					switch (PhUpdaterState)
					{
					case Downloading:
						{
							if (PhInstalledUsingSetup())
							{	
								HWND hwndProgress = GetDlgItem(hwndDlg, IDC_PROGRESS1);
					
								Updater_SetStatusText(hwndDlg, L"Initializing");

								// Enable the status text
								ShowWindow(GetDlgItem(hwndDlg, IDC_STATUS), SW_SHOW);					    

								PhSetWindowStyle(hwndProgress, PBS_MARQUEE, PBS_MARQUEE);
								PostMessage(hwndProgress, PBM_SETMARQUEE, TRUE, 75);

								// Star our Downloader thread
								PhCreateThread(0, (PUSER_THREAD_START_ROUTINE)DownloadWorkerThreadStart, hwndDlg);   
							}
							else
							{
								// handle other installation types
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

DWORD InitializeConnection(
	__in BOOL useCache, 
	__in PCWSTR host, 
	__in PCWSTR path
	)
{
	DWORD status = 0;

	// Initialize the wininet library.
	NetIitialize = InternetOpen(
		L"PH Updater", // user-agent
		INTERNET_OPEN_TYPE_PRECONFIG, // use system proxy configuration	 
		NULL, 
		NULL, 
		0
		);

	if (!NetIitialize)
	{
		status = GetLastError();

		LogEvent(PhFormatString(L"Updater: (InitializeConnection) InternetOpen failed (%d)\r\n", status));

		return status;
	}

	// Connect to the server.
	NetConnection = InternetConnect(
		NetIitialize,
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
		status = GetLastError();

		LogEvent(PhFormatString(L"Updater: (InitializeConnection) InternetConnect failed (%d)\r\n", status));

		return status;
	}
	
	// Open the HTTP request.
	NetRequest = HttpOpenRequest(
		NetConnection, 
		NULL, 
		path, 
		NULL, 
		NULL, 
		NULL, 
		useCache ? 0 : INTERNET_FLAG_DONT_CACHE,
		0
		);

	if (!NetRequest)
	{
		status = GetLastError();

		LogEvent(PhFormatString(L"Updater: (InitializeConnection) HttpOpenRequest failed (%d)\r\n", status));

		return status;
	}

	return status;
}

DWORD InitializeFile()
{
	TCHAR lpTempPathBuffer[MAX_PATH];
	DWORD length = 0;

	// Get the temp path env string (no guarantee it's a valid path).
	length = GetTempPath(MAX_PATH, lpTempPathBuffer);

	if (length > MAX_PATH || length == 0)
	{
		DWORD dwRetVal = GetLastError();

		LogEvent(PhFormatString(L"Updater: (InitializeFile) GetTempPath failed (%d)", dwRetVal));

		return dwRetVal;
	}	

	LocalFilePathString = PhConcatStrings2(
		lpTempPathBuffer,
		L"processhacker-setup.exe"
		);

	// Open output file
	TempFileHandle = CreateFile(
		LocalFilePathString->Buffer,
		GENERIC_WRITE,
		FILE_SHARE_WRITE,
		0,                     // handle cannot be inherited
		CREATE_ALWAYS,         // if file exists, delete it
		FILE_ATTRIBUTE_NORMAL,
		0);

	if (TempFileHandle == INVALID_HANDLE_VALUE)
	{
		DWORD dwRetVal = GetLastError();

		LogEvent(PhFormatString(L"Updater: (InitializeFile) CreateFile failed (%d)", dwRetVal));

		return dwRetVal;
	}

	return 0;
}

BOOL PhInstalledUsingSetup() 
{
	HKEY hKey = NULL;
	DWORD result = 0;

	// Check uninstall entries for the 'Process_Hacker2_is1' registry key.
	result = RegOpenKeyEx(
		HKEY_LOCAL_MACHINE, 
		L"SOFTWARE\\Microsoft\\Windows\\CurrentVersion\\Uninstall\\Process_Hacker2_is1", 
		0, 
		KEY_QUERY_VALUE, 
		&hKey
		);

	// Cleanup
	NtClose(hKey);

	if (result != ERROR_SUCCESS)
	{
		LogEvent(PhFormatString(L"Updater: (PhInstalledUsingSetup) RegOpenKeyEx failed (%d)", result));

		return FALSE;
	}
	
	return TRUE;
}

VOID LogEvent(__in PPH_STRING str)
{
	PhLogMessageEntry(PH_LOG_ENTRY_MESSAGE, str);
	
	PhDereferenceObject(str);
}

VOID DisposeConnection()
{
	if (NetIitialize)
		InternetCloseHandle(NetIitialize);

	if (NetConnection)
		InternetCloseHandle(NetConnection);

	if (NetRequest)
		InternetCloseHandle(NetRequest);
}

VOID DisposeStrings()
{
	if (LocalFilePathString)
		PhDereferenceObject(LocalFilePathString);

	if (RemoteHashString)
		PhDereferenceObject(RemoteHashString);
}

VOID DisposeFileHandles()
{
	if (TempFileHandle)
		NtClose(TempFileHandle);
}