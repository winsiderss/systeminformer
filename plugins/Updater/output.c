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

extern NTSTATUS SilentWorkerThreadStart(
	__in PVOID Parameter
	)
{
	INT result = 0;
	DWORD dwBytes = 0;
	DWORD dwType;

	if (!InternetGetConnectedState(&dwType, 0))
	{
		return TRUE;
	}

	if (!InitializeConnection(
		L"processhacker.sourceforge.net", 
		L"/updater.php"
		))
	{
		return TRUE;
	}

	// Send the HTTP request.
	if (HttpSendRequest(NetRequest, NULL, 0, NULL, 0))
	{
		char buffer[BUFFER_LEN];
		BOOL nReadFile = FALSE;

		RtlZeroMemory(buffer, BUFFER_LEN);

		// Read the resulting xml into our buffer.
		while (nReadFile = InternetReadFile(NetRequest, buffer, BUFFER_LEN, &dwBytes))
		{
			if (dwBytes == 0)
				break;

			if (!nReadFile)
			{
				LogEvent(PhFormatString(L"Updater: (SilentWorkerThreadStart) InternetReadFile failed (%d)", GetLastError()));

				return TRUE;
			}
		}

		if (QueryXmlData(buffer))
		{
			return TRUE;
		}

		result = strcmp(VersionString->Buffer, "2.11"); 

		if (result > 0)
		{
			// Don't spam the user the second they open PH, delay dialog creation for 3 seconds.
			Sleep(3000);

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
	}
	else
	{
		LogEvent(PhFormatString(L"Updater: (WorkerThreadStart) HttpSendRequest failed (%d)", GetLastError()));
	}

	DisposeConnection();

	return FALSE;
}

static NTSTATUS WorkerThreadStart(
	__in PVOID Parameter
	)
{
	INT result = 0;
	HWND hwndDlg = (HWND)Parameter;

	DWORD
		dwRetVal = 0, 
		dwTotalReadSize = 0, 
		dwBytes = 0, 
		dwContentLen = 0, 
		dwBytesRead = 0,
		dwBytesWritten = 0, 
		dwBufLen = sizeof(BUFFER_LEN);
	DWORD dwType;
	
	if (!InternetGetConnectedState(&dwType, 0))
	{
		return TRUE;
	}

	if (!InitializeConnection(
		L"processhacker.sourceforge.net", 
		L"/updater.php"
		))
	{
		return TRUE;
	}

	// Send the HTTP request.
	if (HttpSendRequest(NetRequest, NULL, 0, NULL, 0))
	{
        char buffer[BUFFER_LEN];
		BOOL nReadFile = FALSE;

		// Read the resulting xml into our buffer.
		while (nReadFile = InternetReadFile(NetRequest, buffer, BUFFER_LEN, &dwBytes))
		{
			if (dwBytes == 0)
				break;

			if (!nReadFile)
			{
				DWORD dwStatusResult = GetLastError();
				LogEvent(PhFormatString(L"Updater: (WorkerThreadStart) InternetReadFile failed (%d)", dwStatusResult));

				return dwStatusResult;
			}
		}

		if (dwRetVal = QueryXmlData(buffer))
		{
			SetDlgItemText(hwndDlg, IDC_MESSAGE, L"There was an error downloading the xml.");
			return dwRetVal;
		}

		result = strcmp(VersionString->Buffer, "2.11"); 

		if (result > 0)
		{
			PPH_STRING summaryText = NULL, versionText = NULL;

			versionText = PhCreateStringFromAnsi(VersionString->Buffer);
			summaryText = PhFormatString(L"Process Hacker %s is available.", versionText->Buffer);
			SetDlgItemText(hwndDlg, IDC_MESSAGE, summaryText->Buffer);
			PhDereferenceObject(summaryText);
			PhDereferenceObject(versionText);

			summaryText = PhFormatString(L"Released: %s", ReldateString->Buffer);
			SetDlgItemText(hwndDlg, IDC_DLSIZE, summaryText->Buffer);
			PhDereferenceObject(summaryText);

			summaryText = PhFormatString(L"Size: %s", SizeString->Buffer);
			SetDlgItemText(hwndDlg, IDC_RELDATE, summaryText->Buffer);
			PhDereferenceObject(summaryText);

			EnableWindow(GetDlgItem(hwndDlg, IDDOWNLOAD), TRUE);		
			ShowWindow(GetDlgItem(hwndDlg, IDC_RELDATE), SW_SHOW);
			ShowWindow(GetDlgItem(hwndDlg, IDC_DLSIZE), SW_SHOW);
		}
		else if (result == 0)
		{
			PPH_STRING summaryText = NULL, versionText = NULL;

			versionText = PhCreateStringFromAnsi(VersionString->Buffer);	
			summaryText = PhFormatString(L"You're running the latest version: %s", versionText->Buffer);

			SetDlgItemText(hwndDlg, IDC_MESSAGE, summaryText->Buffer);

			PhDereferenceObject(versionText);
			PhDereferenceObject(summaryText);

			EnableWindow(GetDlgItem(hwndDlg, IDDOWNLOAD), FALSE);
		}
		else if (result < 0)
		{
			PPH_STRING localText = PhGetPhVersion();
			PPH_STRING summaryText = PhFormatString(L"You're running a newer version: %s", localText->Buffer);

			SetDlgItemText(hwndDlg, IDC_MESSAGE, summaryText->Buffer);

			PhDereferenceObject(localText);
			PhDereferenceObject(summaryText);
		}
	}
	else
	{
		dwRetVal = GetLastError();

		LogEvent(PhFormatString(L"Updater: (WorkerThreadStart) HttpSendRequest failed (%d)", dwRetVal));

		return dwRetVal;
	}

	DisposeConnection();

	PhUpdaterState = Downloading;

	return dwRetVal;
}

static NTSTATUS DownloadWorkerThreadStart(
	__in PVOID Parameter
	)
{
	INT i = 0, dlProgress = 0;
	DWORD dwStatusResult = 0, dwTotalReadSize = 0, dwContentLen = 0, dwBytesRead = 0, dwBytesWritten = 0, dwBufLen = sizeof(dwContentLen);	
	HWND hwndDlg = (HWND)Parameter;
	PH_HASH_CONTEXT hashContext;

	HWND hwndProgress = GetDlgItem(hwndDlg, IDC_PROGRESS1);
	
	DWORD dwType;
	if (!InternetGetConnectedState(&dwType, 0))
	{
		return TRUE;
	}

	if (!InitializeConnection(
		L"sourceforge.net",
		L"/projects/processhacker/files/processhacker2/processhacker-2.19-setup.exe/download" //?use_mirror=waix"
		))
	{
		return TRUE;
	}

	if (!InitializeFile())
		return TRUE;

	Updater_SetStatusText(hwndDlg, L"Connecting");
	EnableWindow(GetDlgItem(hwndDlg, IDDOWNLOAD), FALSE);

	if (HttpSendRequest(NetRequest, NULL, 0, NULL, 0))
	{
        char buffer[BUFFER_LEN];

		RtlZeroMemory(buffer, BUFFER_LEN);

		if (HttpQueryInfo(NetRequest, HTTP_QUERY_CONTENT_LENGTH | HTTP_QUERY_FLAG_NUMBER, (LPVOID)&dwContentLen, &dwBufLen, 0))
		{
			BOOL nReadFile = FALSE;

			// Reset Progressbar state.
			PhSetWindowStyle(hwndProgress, PBS_MARQUEE, 0);
			// Initialize hash algorithm.
			PhInitializeHash(&hashContext, HashAlgorithm);

			while (nReadFile = InternetReadFile(NetRequest, &buffer, BUFFER_LEN, &dwBytesRead)) 
			{
				if (dwBytesRead == 0)
					break;

				if (!nReadFile)
				{
					DWORD dwStatusResult = GetLastError();
					LogEvent(PhFormatString(L"Updater: (DownloadWorkerThreadStart) InternetReadFile failed (%d)", dwStatusResult));

					return dwStatusResult;
				}

				// Update the hash of bytes we just downloaded.
				PhUpdateHash(&hashContext, buffer, dwBytesRead);

				// Write the downloaded bytes to disk.
				if (!WriteFile(TempFileHandle, buffer, dwBytesRead, &dwBytesWritten, NULL)) 
				{
					dwStatusResult = GetLastError();
					LogEvent(PhFormatString(L"Updater: (DownloadWorkerThreadStart) WriteFile failed (%d)", dwStatusResult));

					return dwStatusResult;   
				}
	
				// Reset the buffer.
				RtlZeroMemory(buffer, dwBytesRead);

				// Check dwBytesRead are the same dwBytesWritten length returned by WriteFile.
				if (dwBytesRead != dwBytesWritten) 
				{		
					dwStatusResult = GetLastError();
					LogEvent(PhFormatString(L"Updater: (DownloadWorkerThreadStart) WriteFile failed (%d)", dwStatusResult));

					return dwStatusResult;                
				}

				// Update our total bytes downloaded
				dwTotalReadSize += dwBytesRead;
				// Calculate the percentage of our total bytes downloaded per the length.
				dlProgress = (int)(((double)dwTotalReadSize / (double)dwContentLen) * 100);

				SendMessage(hwndProgress, PBM_SETPOS, dlProgress, 0);
				{
					PPH_STRING dlCurrent = PhFormatSize(dwTotalReadSize, -1);
				    //PPH_STRING dlLength = PhFormatSize(dwContentLen, -1);

					PPH_STRING str = PhFormatString(L"Downloaded: %d%% (%s)", dlProgress, dlCurrent->Buffer);
				
					Updater_SetStatusText(hwndDlg, str->Buffer);

					PhDereferenceObject(str);
					//PhDereferenceObject(dlLength);
					PhDereferenceObject(dlCurrent);
				}
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

				PhUpdateHash(&hashContext, buffer, dwBytesRead);

				if (!WriteFile(TempFileHandle, buffer, dwBytesRead, &dwBytesWritten, NULL)) 
				{
					dwStatusResult = GetLastError();
					LogEvent(PhFormatString(L"Updater: (DownloadWorkerThreadStart) WriteFile failed (%d)\r\n", dwStatusResult));

					return dwStatusResult;   
				}

				// Reset the buffer.
				RtlZeroMemory(buffer, BUFFER_LEN);

				if (dwBytesRead != dwBytesWritten) 
				{
					dwStatusResult = GetLastError();
					LogEvent(PhFormatString(L"Updater: (DownloadWorkerThreadStart) WriteFile failed (%d)\r\n", dwStatusResult));
					return dwStatusResult;   
				}   
			}
		}

		Updater_SetStatusText(hwndDlg, L"Download Complete");

		DisposeConnection();
		DisposeFileHandles();

		PhUpdaterState = Installing;

		// Enable Install button before hashing (user might not care about file hash result)
		SetWindowText(GetDlgItem(hwndDlg, IDDOWNLOAD), L"Install");
		EnableWindow(GetDlgItem(hwndDlg, IDDOWNLOAD), TRUE);
				
		if (!PhElevated)
			SendMessage(GetDlgItem(hwndDlg, IDDOWNLOAD), BCM_SETSHIELD, 0, TRUE);

		{
			UCHAR hashBuffer[20];
			ULONG hashLength = 0;

			if (PhFinalHash(&hashContext, hashBuffer, 20, &hashLength))
			{
				// Allocate our hash string, hex the final hash result in our hashBuffer.
				PH_STRING *hexString = PhBufferToHexString(hashBuffer, hashLength);				
				// Allocate our hexString as ANSI for strncmp.
				PH_ANSI_STRING *ansihexString = PhCreateAnsiStringFromUnicode(hexString->Buffer);
				
				// Compare the two strings, ansihexString against RemoteHashString.
				int strResult = strncmp(ansihexString->Buffer, RemoteHashString->Buffer, hashLength); 

				// Free strings
				PhDereferenceObject(ansihexString);
				PhDereferenceObject(hexString);

				// Check the comparison result. 
				if (strResult == 0)
				{
					Updater_SetStatusText(hwndDlg, L"Hash Verified");
				}
				else
				{
					if (WindowsVersion >= WINDOWS_VISTA)
						SendMessage(hwndProgress, PBM_SETSTATE, PBST_ERROR, 0);

					Updater_SetStatusText(hwndDlg, L"Hash failed");
				}
			}
			else
			{
				if (WindowsVersion >= WINDOWS_VISTA)
					SendMessage(hwndProgress, PBM_SETSTATE, PBST_ERROR, 0);
				
				Updater_SetStatusText(hwndDlg, L"Hash failed");
			}
		}
	}
	else
	{
		dwStatusResult = GetLastError();

		LogEvent(PhFormatString(L"Updater: (DownloadWorkerThreadStart) HttpSendRequest failed (%d)\r\n", dwStatusResult));

		return dwStatusResult;
	}

    return dwStatusResult; 
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
			WindowVisible = TRUE;

			PhCenterWindow(hwndDlg, GetParent(hwndDlg));
						
			EnableCache = PhGetIntegerSetting(L"ProcessHacker.Updater.EnableCache");
			HashAlgorithm = (PH_HASH_ALGORITHM)PhGetIntegerSetting(L"ProcessHacker.Updater.HashAlgorithm");
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
								
					WindowVisible = FALSE;

					EndDialog(hwndDlg, IDOK);
				}
				break;
			case IDDOWNLOAD:
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
								ShowWindow(GetDlgItem(hwndDlg, IDC_STATUSTEXT), SW_SHOW);					    

								PhSetWindowStyle(hwndProgress, PBS_MARQUEE, PBS_MARQUEE);
								PostMessage(hwndProgress, PBM_SETMARQUEE, TRUE, 75);

								// Star our Downloader thread
								PhCreateThread(0, (PUSER_THREAD_START_ROUTINE)DownloadWorkerThreadStart, hwndDlg);   
							}
							else
							{
								// handle other installation types
								static PH_FILETYPE_FILTER filters[] =
								{
									{ L"Compressed files (*.zip)", L"*.zip" },
								};

								PVOID fileDialog;

								fileDialog = PhCreateSaveFileDialog();

								PhSetFileDialogFilter(fileDialog, filters, sizeof(filters) / sizeof(PH_FILETYPE_FILTER));
								PhSetFileDialogFileName(fileDialog, L"processhacker-2.19-setup.zip");

								if (PhShowFileDialog(hwndDlg, fileDialog))
								{
									//NTSTATUS status;
									PPH_STRING fileName;
									//PPH_FILE_STREAM fileStream;

									fileName = PhGetFileDialogFileName(fileDialog);
									PhaDereferenceObject(fileName);
								}

								PhFreeFileDialog(fileDialog);
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
		
		return TRUE;
	}
	
	// Open the HTTP request.
	NetRequest = HttpOpenRequest(
		NetConnection, 
		NULL, 
		path, 
		NULL, 
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

BOOL InitializeFile()
{
	TCHAR lpTempPathBuffer[MAX_PATH];
	DWORD length = 0;

	// Get the temp path env string (no guarantee it's a valid path).
	length = GetTempPath(MAX_PATH, lpTempPathBuffer);

	if (length > MAX_PATH || length == 0)
	{
		LogEvent(PhFormatString(L"Updater: (InitializeFile) GetTempPath failed (%d)", GetLastError()));

		return FALSE;
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
		LogEvent(PhFormatString(L"Updater: (InitializeFile) CreateFile failed (%d)", GetLastError()));

		return FALSE;
	}

	return TRUE;
}

NTSTATUS QueryXmlData(VOID* buffer)
{
	mxml_node_t *xmlDoc = NULL, *xmlNode2 = NULL, *xmlNode3 = NULL, *xmlNode4 = NULL, *xmlNode5 = NULL;

	// Load our XML.
	xmlDoc = mxmlLoadString(NULL, (char*)buffer, MXML_OPAQUE_CALLBACK);

	// Check our XML.
	if (xmlDoc == NULL || xmlDoc->type != MXML_ELEMENT)
	{
		LogEvent(PhFormatString(L"Updater: (WorkerThreadStart) mxmlLoadString failed."));
		return STATUS_FILE_CORRUPT_ERROR;
	}

	// Find the ver node.
	xmlNode2 = mxmlFindElement(xmlDoc, xmlDoc, "ver", NULL, NULL, MXML_DESCEND);
	if (xmlNode2 == NULL || xmlNode2->type != MXML_ELEMENT)
	{
		mxmlRelease(xmlDoc);

		LogEvent(PhFormatString(L"Updater: (WorkerThreadStart) mxmlLoadString xmlNode2 failed."));
		return STATUS_FILE_CORRUPT_ERROR;
	}

	// Find the reldate node.
	xmlNode3 = mxmlFindElement(xmlDoc, xmlDoc, "reldate", NULL, NULL, MXML_DESCEND);
	if (xmlNode3 == NULL || xmlNode3->type != MXML_ELEMENT)
	{
		mxmlRelease(xmlNode2);
		mxmlRelease(xmlDoc);

		LogEvent(PhFormatString(L"Updater: (WorkerThreadStart) mxmlLoadString xmlNode3 failed."));
		return STATUS_FILE_CORRUPT_ERROR;
	}

	// Find the size node.
	xmlNode4 = mxmlFindElement(xmlDoc, xmlDoc, "size", NULL, NULL, MXML_DESCEND);
	if (xmlNode4 == NULL || xmlNode4->type != MXML_ELEMENT)
	{
		mxmlRelease(xmlNode3);	
		mxmlRelease(xmlNode2);
		mxmlRelease(xmlDoc);

		LogEvent(PhFormatString(L"Updater: (WorkerThreadStart) mxmlLoadString xmlNode4 failed."));
		return STATUS_FILE_CORRUPT_ERROR;
	}

	if (HashAlgorithm == Md5HashAlgorithm)
	{
		// Find the sha1 node.
		xmlNode5 = mxmlFindElement(xmlDoc, xmlDoc, "md5", NULL, NULL, MXML_DESCEND);
		if (xmlNode5 == NULL || xmlNode5->type != MXML_ELEMENT)
		{
			mxmlRelease(xmlNode4);
			mxmlRelease(xmlNode3);	
			mxmlRelease(xmlNode2);
			mxmlRelease(xmlDoc);

			LogEvent(PhFormatString(L"Updater: (WorkerThreadStart) mxmlLoadString xmlNode5 (md5) failed."));
			return STATUS_FILE_CORRUPT_ERROR;
		}
	}
	else
	{
		// Find the md5 node.
		xmlNode5 = mxmlFindElement(xmlDoc, xmlDoc, "sha1", NULL, NULL, MXML_DESCEND);
		if (xmlNode5 == NULL || xmlNode5->type != MXML_ELEMENT)
		{
			mxmlRelease(xmlNode4);
			mxmlRelease(xmlNode3);	
			mxmlRelease(xmlNode2);
			mxmlRelease(xmlDoc);

			LogEvent(PhFormatString(L"Updater: (WorkerThreadStart) mxmlLoadString xmlNode5 (sha1) failed."));
			return STATUS_FILE_CORRUPT_ERROR;
		}
	}

	VersionString = PhCreateAnsiString(xmlNode2->child->value.opaque);
	RemoteHashString = PhCreateAnsiString(xmlNode5->child->value.opaque);
	ReldateString = PhCreateStringFromAnsi(xmlNode3->child->value.opaque);
	SizeString = PhCreateStringFromAnsi(xmlNode4->child->value.opaque);
	BetaDlString = PhCreateStringFromAnsi(xmlNode5->child->value.opaque);

	mxmlRelease(xmlNode5);
	mxmlRelease(xmlNode4);
	mxmlRelease(xmlNode3);	
	mxmlRelease(xmlNode2);
	mxmlRelease(xmlDoc);

	return STATUS_SUCCESS;
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

VOID DisposeStrings()
{
	if (LocalFilePathString)
	{
		PhDereferenceObject(LocalFilePathString);
		LocalFilePathString = NULL;
	}

	if (VersionString)
	{
		PhDereferenceObject(VersionString);
		VersionString = NULL;
	}

	if (ReldateString)
	{
		PhDereferenceObject(ReldateString);
		ReldateString = NULL;
	}

	if (SizeString)
	{
		PhDereferenceObject(SizeString);
		SizeString = NULL;
	}

	if (RemoteHashString)
	{
		PhDereferenceObject(RemoteHashString);
		RemoteHashString = NULL;
	}

	if (BetaDlString)
	{
		PhDereferenceObject(BetaDlString);
		BetaDlString = NULL;
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


#define MO 0x100000 /* Read 1 Mo by 1Mo. */

///*--------------------------------------------------------------------------*/
//DWORD httpDownloadFile(char * szURL,char * szSaveFilePath)
//{
//
//	HINTERNET hiConnex = NULL;
//	/* * / * : /*rfc 2616 protocole http.  all files type accepted*/
//	char szHeader[]="Accept: */*\r\n\r\n"; 
//	HINTERNET hiDownload;
//
//	hiConnex = InternetOpen(L"Scilab_Download",INTERNET_OPEN_TYPE_DIRECT,NULL,NULL,0);
//	if(hiConnex == NULL) 
//		return 1;
//
//	if(!(hiDownload = InternetOpenUrl(hiConnex, szURL, szHeader, lstrlen(szHeader),INTERNET_FLAG_DONT_CACHE | INTERNET_FLAG_RELOAD | INTERNET_FLAG_PRAGMA_NOCACHE,0)))
//	{
//		InternetCloseHandle(hiConnex);
//		return 1;
//	}
//	else
//	{
//		HANDLE haFile;
//
//		haFile = CreateFile(szSaveFilePath,GENERIC_WRITE,FILE_SHARE_WRITE,0,CREATE_ALWAYS,0,0);
//
//		if(haFile == INVALID_HANDLE_VALUE)
//		{
//			InternetCloseHandle(hiConnex);
//			return 1;
//		}
//		else
//		{
//			char *szBuff = NULL;
//
//			DWORD dwBytesRequired = 0;
//			DWORD dwSizeOfByReq = 4;
//			DWORD dwBytesRead = 0;
//			DWORD dwBytesWritten = 0;
//
//			/* Get file size */
//			if(!HttpQueryInfo(hiDownload,HTTP_QUERY_CONTENT_LENGTH | HTTP_QUERY_FLAG_NUMBER,(LPVOID)&dwBytesRequired,&dwSizeOfByReq,0))
//			{
//				InternetCloseHandle(hiConnex);
//				return 1;
//			}
//			else
//			{
//				if(dwBytesRequired > MO)
//				{
//					szBuff = (char*)malloc(MO);
//
//					if(szBuff == NULL)
//					{
//						CloseHandle(haFile);
//						InternetCloseHandle(hiConnex);
//						return FALSE;
//					}
//				}
//				else
//				{
//					szBuff = (char*)malloc(dwBytesRequired);
//
//					if(szBuff == NULL)
//					{
//						CloseHandle(haFile);
//						InternetCloseHandle(hiConnex);
//						return FALSE;
//					}
//				}
//
//				while(dwBytesRequired > 0)
//				{
//					/* we read 1Mo from file. */
//					if(dwBytesRequired >= MO)
//					{
//						if(!InternetReadFile(hiDownload,szBuff,MO,&dwBytesRead) || dwBytesRead != MO)
//						{
//							CloseHandle(haFile);
//							InternetCloseHandle(hiConnex);
//							free(szBuff);
//							return 1;
//						}
//						dwBytesRequired -= MO;
//
//
//						/* we write buffer */
//						if(!WriteFile(haFile,szBuff,MO,&dwBytesWritten,NULL) || dwBytesWritten != MO)
//						{
//							CloseHandle(haFile);
//							InternetCloseHandle(hiConnex);
//							free(szBuff);
//							return 1;
//						}
//					}
//					else
//					{
//						if(!InternetReadFile(hiDownload,szBuff,dwBytesRequired,&dwBytesRead) || dwBytesRead != dwBytesRequired)
//						{
//							CloseHandle(haFile);
//							InternetCloseHandle(hiConnex);
//							free(szBuff);
//							return 1;
//						}
//
//						/* we write buffer in a backup file*/
//						if(!WriteFile(haFile,szBuff,dwBytesRequired,&dwBytesWritten,NULL) || dwBytesWritten != dwBytesRequired)
//						{
//							CloseHandle(haFile);
//							InternetCloseHandle(hiConnex);
//							free(szBuff);
//							return 1;
//						}
//
//						dwBytesRequired = 0;
//					}
//				}
//
//				InternetCloseHandle(hiConnex);
//				CloseHandle(haFile);
//				free(szBuff);
//				return HTTP_DOWNLOAD_ERROR_OK;
//			}
//		}
//	}
//
//}