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
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with Process Hacker. If not, see <http://www.gnu.org/licenses/>.
*/

#include "updater.h"

static PPH_PLUGIN PluginInstance;
static PH_CALLBACK_REGISTRATION PluginMenuItemCallbackRegistration;
static PH_CALLBACK_REGISTRATION MainWindowShowingCallbackRegistration;
static PH_CALLBACK_REGISTRATION PluginShowOptionsCallbackRegistration;

static PH_EVENT InitializedEvent = PH_EVENT_INIT;

static HWND UpdateDialogHandle = NULL;
static HANDLE UpdaterDialogThreadHandle = NULL;

static HANDLE UpdateCheckThreadHandle = NULL;
static HANDLE DownloadThreadHandle = NULL;

static HFONT FontHandle = NULL;
static PH_UPDATER_STATE PhUpdaterState = Download;
static PPH_STRING SetupFilePath = NULL;

LOGICAL DllMain(
    __in HINSTANCE Instance,
    __in ULONG Reason,
    __reserved PVOID Reserved
    )
{
    switch (Reason)
    {
    case DLL_PROCESS_ATTACH:
        {
            PPH_PLUGIN_INFORMATION info;

            PluginInstance = PhRegisterPlugin(L"ProcessHacker.UpdateChecker", Instance, &info);

            if (!PluginInstance)
                return FALSE;

            info->DisplayName = L"Update Checker";
            info->Author = L"dmex";
            info->Description = L"Plugin for checking new Process Hacker releases via the Help menu.";
            info->HasOptions = TRUE;

            PhRegisterCallback(
                PhGetGeneralCallback(GeneralCallbackMainWindowShowing),
                MainWindowShowingCallback,
                NULL,
                &MainWindowShowingCallbackRegistration
                );
            PhRegisterCallback(
                PhGetPluginCallback(PluginInstance, PluginCallbackMenuItem),
                MenuItemCallback,
                NULL,
                &PluginMenuItemCallbackRegistration
                );
            PhRegisterCallback(
                PhGetPluginCallback(PluginInstance, PluginCallbackShowOptions),
                ShowOptionsCallback,
                NULL,
                &PluginShowOptionsCallbackRegistration
                );

            {
                PH_SETTING_CREATE settings[] =
                {
                    { IntegerSettingType, SETTING_AUTO_CHECK, L"1" },
                };

                PhAddSettings(settings, _countof(settings));
            }
        }
        break;
    }

    return TRUE;
}

VOID NTAPI MainWindowShowingCallback(
    __in_opt PVOID Parameter,
    __in_opt PVOID Context
    )
{
    // Add our menu item, 4 = Help menu.
    PhPluginAddMenuItem(PluginInstance, 4, NULL, UPDATE_MENUITEM, L"Check for Updates", NULL);

    if (PhGetIntegerSetting(SETTING_AUTO_CHECK))
    {
        // Queue up our initial update check.
        StartInitialCheck();
    }
}

VOID NTAPI MenuItemCallback(
    __in_opt PVOID Parameter,
    __in_opt PVOID Context
    )
{
    PPH_PLUGIN_MENU_ITEM menuItem = (PPH_PLUGIN_MENU_ITEM)Parameter;

    if (menuItem != NULL)
    {
        switch (menuItem->Id)
        {
        case UPDATE_MENUITEM:
            {
                ShowUpdateDialog();
            }
            break;
        }
    }
}

VOID NTAPI ShowOptionsCallback(
    __in_opt PVOID Parameter,
    __in_opt PVOID Context
    )
{
    DialogBox(
        (HINSTANCE)PluginInstance->DllBase,
        MAKEINTRESOURCE(IDD_OPTIONS),
        (HWND)Parameter,
        OptionsDlgProc
        );
}


static LONG CompareVersions(
    __in ULONG MajorVersion1,
    __in ULONG MinorVersion1,
    __in ULONG MajorVersion2,
    __in ULONG MinorVersion2
    )
{
    LONG result = intcmp(MajorVersion1, MajorVersion2);

    if (result == 0)
        result = intcmp(MinorVersion1, MinorVersion2);

    return result;
}

static BOOL ConnectionAvailable(
    VOID
    )
{
    if (WindowsVersion > WINDOWS_XP)
    {
        INetworkListManager *pNetworkListManager;

        // Create an instance of the INetworkListManger COM object.
        if (SUCCEEDED(CoCreateInstance(&CLSID_NetworkListManager, NULL, CLSCTX_ALL, &IID_INetworkListManager, &pNetworkListManager)))
        {
            VARIANT_BOOL isConnected = VARIANT_FALSE;
            VARIANT_BOOL isConnectedInternet = VARIANT_FALSE;

            INetworkListManager_get_IsConnected(pNetworkListManager, &isConnected);
            INetworkListManager_get_IsConnectedToInternet(pNetworkListManager, &isConnectedInternet);

            INetworkListManager_Release(pNetworkListManager);
            pNetworkListManager = NULL;

            if (isConnected == VARIANT_TRUE && isConnectedInternet == VARIANT_TRUE)
            {
                return TRUE;
            }
            else
            {
                return FALSE;
            }
        }

        // If we were unable to init the INetworkListManager, fall back to InternetGetConnectedState.
        goto NOT_SUPPORTED;
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

        //if (!InternetCheckConnection(NULL, FLAG_ICC_FORCE_CONNECTION, 0))
        //{
        // LogEvent(PhFormatString(L"Updater: (ConnectionAvailable) InternetCheckConnection failed connection to Sourceforge.net (%d)", GetLastError()));
        // return FALSE;
        //}
    }

    return FALSE;
}

static BOOL ReadRequestString(
    __in HINTERNET Handle,
    __out_z PSTR *Data,
    __out_opt PULONG DataLength
    )
{
    BYTE buffer[BUFFER_LEN];
    PSTR data;
    ULONG allocatedLength;
    ULONG dataLength;
    ULONG returnLength;

    allocatedLength = sizeof(buffer);
    data = (PSTR)PhAllocate(allocatedLength);
    dataLength = 0;

    while (InternetReadFile(Handle, buffer, BUFFER_LEN, &returnLength))
    {
        if (returnLength == 0)
            break;

        if (allocatedLength < dataLength + returnLength)
        {
            allocatedLength *= 2;
            data = (PSTR)PhReAllocate(data, allocatedLength);
        }

        RtlCopyMemory(data + dataLength, buffer, returnLength);
        RtlZeroMemory(buffer, BUFFER_LEN);

        dataLength += returnLength;
    }

    if (allocatedLength < dataLength + 1)
    {
        allocatedLength++;
        data = (PSTR)PhReAllocate(data, allocatedLength);
    }

    // Ensure that the buffer is null-terminated.
    data[dataLength] = 0;

    *Data = data;

    if (DataLength)
        *DataLength = dataLength;

    return TRUE;
}

static BOOL PhInstalledUsingSetup(
    VOID
    )
{
    static PH_STRINGREF keyName = PH_STRINGREF_INIT(L"Software\\Microsoft\\Windows\\CurrentVersion\\Uninstall\\Process_Hacker2_is1");

    HANDLE keyHandle = NULL;

    // Check uninstall entries for the 'Process_Hacker2_is1' registry key.
    if (NT_SUCCESS(PhOpenKey(
        &keyHandle,
        // Open the key with KEY_WOW64_32KEY, since the user might be using the 32bit binary on 64bit.
        KEY_READ | KEY_WOW64_32KEY, 
        PH_KEY_LOCAL_MACHINE,
        &keyName,
        0
        )))
    {
        NtClose(keyHandle);
        return TRUE;
    }

    return FALSE;
}


static NTSTATUS SilentUpdateCheckThreadStart(
    __in PVOID Parameter
    )
{  
    ULONG majorVersion = 0;
    ULONG minorVersion = 0;
    UPDATER_XML_DATA xmlData = { 0 };

    if (!ConnectionAvailable())
        return STATUS_UNSUCCESSFUL;

    if (!QueryXmlData(&xmlData))
        return STATUS_UNSUCCESSFUL;

    // Get the current Process Hacker version
    PhGetPhVersionNumbers(&majorVersion, &minorVersion, NULL, NULL); 

    // Compare the current version against the latest available version
    if (CompareVersions(xmlData.MajorVersion, xmlData.MinorVersion, majorVersion, minorVersion) > 0)
    {
        // Don't spam the user the second they open PH, delay dialog creation for 3 seconds.
        Sleep(3 * 1000);

        ShowUpdateDialog();
    }

    return STATUS_SUCCESS;
}

static NTSTATUS CheckUpdateThreadStart(
    __in PVOID Parameter
    )
{
    INT result = 0;
    UPDATER_XML_DATA xmlData = { 0 };

    HWND hwndDlg = (HWND)Parameter;

    if (!ConnectionAvailable())
        return STATUS_UNSUCCESSFUL;

    if (!QueryXmlData(&xmlData))
        return STATUS_UNSUCCESSFUL;

    {
        ULONG majorVersion = 0;
        ULONG minorVersion = 0;
        ULONG revisionNumber = 0;

        PhGetPhVersionNumbers(&majorVersion, &minorVersion, NULL, &revisionNumber); 

        result = 1;//CompareVersions(xmlData.MajorVersion, xmlData.MinorVersion, localMajorVersion, localMinorVersion);

        if (result > 0)
        {
            PPH_STRING summaryText = PhFormatString(
                L"Process Hacker %u.%u",
                xmlData.MajorVersion,
                xmlData.MinorVersion
                );

            PPH_STRING releaseDateText = PhFormatString(
                L"Released: %s",
                xmlData.RelDate->Buffer
                );

            PPH_STRING releaseSizeText = PhFormatString(
                L"Size: %s",
                xmlData.Size->Buffer
                );

            SetDlgItemText(hwndDlg, IDC_MESSAGE, summaryText->Buffer);
            SetDlgItemText(hwndDlg, IDC_RELDATE, releaseDateText->Buffer);
            SetDlgItemText(hwndDlg, IDC_STATUS, releaseSizeText->Buffer);

            // Enable the download button.
            Button_Enable(GetDlgItem(hwndDlg, IDC_DOWNLOAD), TRUE);
            // Use the Scrollbar macro to enable the other controls.
            ScrollBar_Show(GetDlgItem(hwndDlg, IDC_PROGRESS), TRUE);
            ScrollBar_Show(GetDlgItem(hwndDlg, IDC_RELDATE), TRUE);
            ScrollBar_Show(GetDlgItem(hwndDlg, IDC_STATUS), TRUE);

            PhDereferenceObject(releaseSizeText);
            PhDereferenceObject(releaseDateText);
            PhDereferenceObject(summaryText);

            // Set the state for the next button.
            PhUpdaterState = Download;
        }
        else if (result == 0)
        {
            PPH_STRING summaryText = PhCreateString(
                L"No updates available"
                );

            PPH_STRING versionText = PhFormatString(
                L"You're running the latest stable version: %u.%u (r%u)",
                xmlData.MajorVersion,
                xmlData.MinorVersion,
                revisionNumber
                );

            //swprintf_s(
            // szReleaseText,
            // _countof(szReleaseText),
            // L"Released: %s",
            // xmlData.RelDate
            // );

            SetDlgItemText(hwndDlg, IDC_MESSAGE, summaryText->Buffer);
            SetDlgItemText(hwndDlg, IDC_RELDATE, versionText->Buffer);

            PhDereferenceObject(versionText);
            PhDereferenceObject(summaryText);
        }
        else if (result < 0)
        {
            PPH_STRING summaryText = PhCreateString(
                L"No updates available"
                );

            PPH_STRING versionText = PhFormatString(
                L"You're running SVN build: %u.%u (r%u)",
                majorVersion,
                minorVersion,
                revisionNumber
                );

            //swprintf_s(
            // szReleaseText,
            // _countof(szReleaseText),
            // L"Released: %s",
            // xmlData.RelDate
            // );

            SetDlgItemText(hwndDlg, IDC_RELDATE, versionText->Buffer);
            SetDlgItemText(hwndDlg, IDC_MESSAGE, summaryText->Buffer);
            
            PhDereferenceObject(versionText);
            PhDereferenceObject(summaryText);
        }
    }

    return STATUS_SUCCESS;
}

static NTSTATUS DownloadUpdateThreadStart(
    __in PVOID Parameter
    )
{
    PPH_STRING downloadUrlPath = NULL;
    HANDLE tempFileHandle = NULL;
    HINTERNET hInitialize = NULL, hConnection = NULL, hRequest = NULL;
    NTSTATUS status = STATUS_UNSUCCESSFUL;

    UPDATER_XML_DATA xmlData = { 0 };

    HWND hwndDlg = (HWND)Parameter;

    Button_Enable(GetDlgItem(hwndDlg, IDC_DOWNLOAD), FALSE);
    SetDlgItemText(hwndDlg, IDC_STATUS, L"Initializing");

    // Reset the progress state on Vista and above.
    if (WindowsVersion > WINDOWS_XP)
        SendDlgItemMessage(hwndDlg, IDC_PROGRESS, PBM_SETSTATE, PBST_NORMAL, 0L);

    if (!ConnectionAvailable())
        return STATUS_UNSUCCESSFUL;

    if (!QueryXmlData(&xmlData))
        return STATUS_UNSUCCESSFUL;

    __try
    {
        // Get temp dir.
        WCHAR tempPathString[MAX_PATH];
        DWORD tempPathLength = GetTempPath(MAX_PATH, tempPathString);

        if (tempPathLength == 0 || tempPathLength > MAX_PATH)
        {
            LogEvent(hwndDlg, PhFormatString(L"CreateFile failed (%d)", GetLastError()));
            __leave;
        }

        // create the download path string.
        downloadUrlPath = PhFormatString(
            L"/projects/processhacker/files/processhacker2/processhacker-%u.%u-setup.exe/download?use_mirror=autoselect", /* ?use_mirror=waix" */
            xmlData.MajorVersion,
            xmlData.MinorVersion
            );

        // Append the tempath to our string: %TEMP%processhacker-%u.%u-setup.exe
        // Example: C:\\Users\\dmex\\AppData\\Temp\\processhacker-2.10-setup.exe
        SetupFilePath = PhFormatString(
            L"%sprocesshacker-%u.%u-setup.exe",
            tempPathString,
            xmlData.MajorVersion,
            xmlData.MinorVersion
            );

        // Create output file
        status = PhCreateFileWin32(
            &tempFileHandle,
            SetupFilePath->Buffer,
            FILE_GENERIC_READ | FILE_GENERIC_WRITE,
            FILE_ATTRIBUTE_NOT_CONTENT_INDEXED | FILE_ATTRIBUTE_TEMPORARY,
            FILE_SHARE_READ | FILE_SHARE_WRITE | FILE_SHARE_DELETE,
            FILE_OVERWRITE_IF,
            FILE_NON_DIRECTORY_FILE | FILE_SYNCHRONOUS_IO_NONALERT
            );

        if (!NT_SUCCESS(status))
        {
            LogEvent(hwndDlg, PhFormatString(L"PhCreateFileWin32 failed (%s)", PhGetNtMessage(status)->Buffer));
            __leave;
        }

        {
            // Create a user agent string.
            PPH_STRING phVersion = PhGetPhVersion();
            PPH_STRING userAgent = PhConcatStrings2(L"PH Updater v", phVersion->Buffer);

            // Initialize the wininet library.
            if (!(hInitialize = InternetOpen(
                userAgent->Buffer,
                INTERNET_OPEN_TYPE_PRECONFIG,
                NULL,
                NULL,
                0
                )))
            {
                LogEvent(hwndDlg, PhFormatString(L"Updater: (InitializeConnection) InternetOpen failed (%d)", GetLastError()));
                
                PhDereferenceObject(userAgent);
                PhDereferenceObject(phVersion);

                __leave;
            }

            PhDereferenceObject(userAgent);
            PhDereferenceObject(phVersion);
        }

        // Connect to the server.
        if (!(hConnection = InternetConnect(
            hInitialize,
            L"sourceforge.net",
            INTERNET_DEFAULT_HTTP_PORT,
            NULL,
            NULL,
            INTERNET_SERVICE_HTTP,
            0,
            0)))
        {
            LogEvent(hwndDlg, PhFormatString(L"InternetConnect failed (%d)", GetLastError()));
            __leave;
        }

        // Open the HTTP request.
        if (!(hRequest = HttpOpenRequest(
            hConnection,
            NULL,
            downloadUrlPath->Buffer,
            NULL,
            NULL,
            NULL,
            INTERNET_FLAG_PRAGMA_NOCACHE | INTERNET_FLAG_RELOAD | INTERNET_FLAG_NO_CACHE_WRITE | INTERNET_FLAG_RESYNCHRONIZE,
            0
            )))
        {
            LogEvent(hwndDlg, PhFormatString(L"HttpOpenRequest failed (%d)", GetLastError()));
            __leave;
        }

        SetDlgItemText(hwndDlg, IDC_STATUS, L"Connecting");

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
            PH_HASH_CONTEXT hashContext;
            BYTE hashBuffer[20];

            DWORD contentLength = 0;
            DWORD contentLengthSize = sizeof(DWORD);

            // Initialize hash algorithm.
            PhInitializeHash(&hashContext, Sha1HashAlgorithm);

            if (!HttpQueryInfoW(hRequest, HTTP_QUERY_CONTENT_LENGTH | HTTP_QUERY_FLAG_NUMBER, &contentLength, &contentLengthSize, 0))
            {
                // No content length...impossible to calculate % complete...
                // we can read the data, BUT in this instance Sourceforge always returns the content length
                // so instead we'll exit here instead of downloading the file.
                LogEvent(hwndDlg, PhFormatString(L"HttpQueryInfo failed (%d)", GetLastError()));
                __leave;
            }
            else
            {
                BYTE buffer[BUFFER_LEN];
                DWORD bytesRead = 0, bytesDownloaded = 0, startTick = 0, timeTransferred = 0;
                IO_STATUS_BLOCK isb;

                // Zero the buffer.
                ZeroMemory(buffer, BUFFER_LEN);

                // Start the clock.
                startTick = GetTickCount();
                timeTransferred = startTick;

                // Download the data.
                while (TRUE)
                {
                    if (!InternetReadFile(
                        hRequest,
                        buffer,
                        BUFFER_LEN,
                        &bytesRead
                        ))
                        break;

                    // If we get zero bytes, the file was uploaded or there was an error.
                    if (bytesRead == 0)
                        break;

                    // HACK: Exit loop if thread handle closed.
                    if (!DownloadThreadHandle)
                        __leave;

                    // Update the hash of bytes we downloaded.
                    PhUpdateHash(&hashContext, buffer, bytesRead);

                    // Write the downloaded bytes to disk.
                    status = NtWriteFile(
                        tempFileHandle,
                        NULL,
                        NULL,
                        NULL,
                        &isb,
                        buffer,
                        bytesRead,
                        NULL,
                        NULL
                        );

                    if (!NT_SUCCESS(status))
                    {
                        LogEvent(hwndDlg, PhFormatString(L"NtWriteFile failed (%s)", PhGetNtMessage(status)->Buffer));
                        break;
                    }

                    // Zero the buffer.
                    ZeroMemory(buffer, BUFFER_LEN);

                    // Check dwBytesRead are the same dwBytesWritten length returned by WriteFile.
                    if (bytesRead != isb.Information)
                    {
                        LogEvent(hwndDlg, PhFormatString(L"NtWriteFile failed (%s)", PhGetNtMessage(status)->Buffer));
                        break;
                    }

                    // Update our total bytes downloaded
                    bytesDownloaded += (DWORD)isb.Information;

                    // Update the GUI progress.
                    // TODO: COMPLETE REWRITE.
                    {
                        WCHAR *rtext;

                        DWORD time_taken = (GetTickCount() - timeTransferred);
                        DWORD time_remain = (MulDiv(time_taken, contentLength, bytesDownloaded) - time_taken);
                        DWORD download_speed = (bytesDownloaded / time_taken);

                        INT percent = MulDiv(100, bytesDownloaded, contentLength);

                        PPH_STRING dlRemaningBytes = PhFormatSize(bytesDownloaded, -1);
                        PPH_STRING dlLength = PhFormatSize(contentLength, -1);
                        PPH_STRING dlSpeed = PhFormatSize(download_speed * 1024, -1);

                        if (time_remain < 0)
                            time_remain = 0;

                        if (time_remain >= 60)
                        {
                            time_remain /= 60;
                            rtext = L"milisecond";

                            if (time_remain >= 60)
                            {
                                time_remain /= 60;
                                rtext = L"second";

                                if (time_remain >= 60)
                                {
                                    time_remain /= 60;
                                    rtext = L"minute";

                                    if (time_remain >= 60)
                                    {
                                        time_remain /= 60;
                                        rtext = L"hour";
                                    }
                                }
                            }
                        }

                        if (time_remain)
                        {               
                            PPH_STRING statusText = PhFormatString(
                                L"%s (%d%%) of %s @ %s/s (%d %s%s remaining)",
                                dlRemaningBytes->Buffer,
                                percent,
                                dlLength->Buffer,
                                dlSpeed->Buffer,
                                time_remain,
                                rtext,
                                time_remain == 1? L"": L"s"
                                );
                            Edit_SetText(GetDlgItem(hwndDlg, IDC_STATUS), statusText->Buffer);

                            PhDereferenceObject(statusText);
                        }
                        else
                        {
                            PPH_STRING statusText = PhFormatString(
                                L"%s (%d%%) of %s @ %s/s",
                                dlRemaningBytes->Buffer,
                                percent,
                                dlLength->Buffer,
                                dlSpeed->Buffer
                                );

                            Edit_SetText(GetDlgItem(hwndDlg, IDC_STATUS), statusText->Buffer);

                            PhDereferenceObject(statusText);
                        }

                        PhDereferenceObject(dlSpeed);
                        PhDereferenceObject(dlLength);
                        PhDereferenceObject(dlRemaningBytes);

                        // Update the progress bar position
                        SendDlgItemMessage(hwndDlg, IDC_PROGRESS, PBM_SETPOS, percent, 0);
                    }
                }

                // Check if we downloaded the entire file.
                assert(bytesDownloaded == contentLength);

                // Compute our hash result.
                if (PhFinalHash(&hashContext, &hashBuffer, 20, NULL))
                {
                    // Allocate our hash string, hex the final hash result in our hashBuffer.
                    PPH_STRING hexString = PhBufferToHexString(hashBuffer, 20);

                    if (!_wcsicmp(hexString->Buffer, xmlData.Hash->Buffer))
                    {
                        // If PH is not elevated, set the UAC sheild for the installer.
                        if (!PhElevated)
                            SendMessage(GetDlgItem(hwndDlg, IDC_DOWNLOAD), BCM_SETSHIELD, 0, TRUE);

                        // Set the download result, don't include hash status since it succeeded.
                        //Edit_SetText(GetDlgItem(hwndDlg, IDC_STATUS), L"Download Complete");
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

                        Edit_SetText(GetDlgItem(hwndDlg, IDC_STATUS), L"Download complete, SHA1 Hash failed.");

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
                    Edit_SetText(GetDlgItem(hwndDlg, IDC_STATUS), L"PhFinalHash failed");

                    // Show fancy Red progressbar if hash failed on Vista and above.
                    if (WindowsVersion > WINDOWS_XP)
                        SendDlgItemMessage(hwndDlg, IDC_PROGRESS, PBM_SETSTATE, PBST_ERROR, 0L);
                }
            }
        }

        status = STATUS_SUCCESS;
    }
    __finally
    {
        if (hInitialize)
            InternetCloseHandle(hInitialize);

        if (hConnection)
            InternetCloseHandle(hConnection);

        if (hRequest)
            InternetCloseHandle(hRequest);

        if (tempFileHandle)
            NtClose(tempFileHandle);

        if (downloadUrlPath)
            PhDereferenceObject(downloadUrlPath);
    }

    return status;
}

static NTSTATUS ShowUpdateDialogThreadStart(
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

    // Ensure global objects are disposed and reset when window closes.

    if (SetupFilePath)
    {
        PhDereferenceObject(SetupFilePath);
        SetupFilePath = NULL;
    }

    if (UpdateCheckThreadHandle)
    {
        NtClose(UpdateCheckThreadHandle);
        UpdateCheckThreadHandle = NULL;
    }

    if (DownloadThreadHandle)
    {
        NtClose(DownloadThreadHandle);
        DownloadThreadHandle = NULL;
    }

    if (UpdaterDialogThreadHandle)
    {
        NtClose(UpdaterDialogThreadHandle);
        UpdaterDialogThreadHandle = NULL;
    }

    if (FontHandle)
    {
        DeleteObject(FontHandle);
        FontHandle = NULL;
    }

    if (UpdateDialogHandle)
    {
        DestroyWindow(UpdateDialogHandle);
        UpdateDialogHandle = NULL;
    }

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

            lHeaderFont.lfHeight = -15;
            lHeaderFont.lfWeight = FW_MEDIUM;
            lHeaderFont.lfQuality = CLEARTYPE_QUALITY | ANTIALIASED_QUALITY;
            // We don't check if Segoe exists, CreateFontIndirect does this for us.
            wcscpy_s(lHeaderFont.lfFaceName, _countof(lHeaderFont.lfFaceName), L"Segoe UI");

            // Create the font handle.
            FontHandle = CreateFontIndirectW(&lHeaderFont);

            // Set the header font.
            SendMessageW(GetDlgItem(hwndDlg, IDC_MESSAGE), WM_SETFONT, (WPARAM)FontHandle, FALSE);

            // Center the update window on PH if visible and not mimimized else center on desktop.
            PhCenterWindow(hwndDlg, (IsWindowVisible(GetParent(hwndDlg)) && !IsIconic(GetParent(hwndDlg))) ? GetParent(hwndDlg) : NULL);

            // Create our update check thread.
            UpdateCheckThreadHandle = PhCreateThread(0, CheckUpdateThreadStart, hwndDlg);
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
    case WM_CTLCOLORBTN:
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

            // Set a transparent background for the control backcolor.
            SetBkMode(hDC, TRANSPARENT);

            // set window background color.
            return (INT_PTR)GetSysColorBrush(COLOR_WINDOW);
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
                                DownloadThreadHandle = PhCreateThread(0, DownloadUpdateThreadStart, hwndDlg);
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
    }

    return FALSE;
}




VOID LogEvent(
    __in_opt HWND hwndDlg,
    __in PPH_STRING str
    )
{
    if (hwndDlg)
    {
        Edit_SetText(GetDlgItem(hwndDlg, IDC_STATUS), str->Buffer);
    }
    else
    {
        PhLogMessageEntry(PH_LOG_ENTRY_MESSAGE, str);
    }

    PhDereferenceObject(str);
}

BOOL QueryXmlData(
    __out PUPDATER_XML_DATA XmlData
    )
{
    PCHAR data = NULL;
    BOOL isSuccess = FALSE;
    HINTERNET netInitialize = NULL, netConnection = NULL, netRequest = NULL;
    mxml_node_t *xmlDoc = NULL, *xmlNodeVer = NULL, *xmlNodeRelDate = NULL, *xmlNodeSize = NULL, *xmlNodeHash = NULL;

    // Create a user agent string.
    PPH_STRING phVersion = PhGetPhVersion();
    PPH_STRING userAgent = PhConcatStrings2(L"PH Updater v", phVersion->Buffer);

    __try
    {
        // Initialize the wininet library.
        if (!(netInitialize = InternetOpen(
            userAgent->Buffer,
            INTERNET_OPEN_TYPE_PRECONFIG,
            NULL,
            NULL,
            0
            )))
        {
            LogEvent(NULL, PhFormatString(L"Updater: (InitializeConnection) InternetOpen failed (%d)", GetLastError()));
            __leave;
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
            __leave;
        }

        // Open the HTTP request.
        if (!(netRequest = HttpOpenRequest(
            netConnection,
            L"GET",
            UPDATE_FILE,
            NULL,
            NULL,
            NULL,
            // Always cache the update xml, it can be cleared by deleting IE history, we configured the file to cache locally for two days.
            0, 
            0
            )))
        {
            LogEvent(NULL, PhFormatString(L"Updater: (InitializeConnection) HttpOpenRequest failed (%d)", GetLastError()));
            __leave;
        }

        // Send the HTTP request.
        if (!HttpSendRequest(netRequest, NULL, 0, NULL, 0))
        {
            LogEvent(NULL, PhFormatString(L"HttpSendRequest failed (%d)", GetLastError()));
            __leave;
        }

        // Read the resulting xml into our buffer.
        if (!ReadRequestString(netRequest, &data, NULL))
        {
            // We don't need to log this.
            __leave;
        }

        // Load our XML.
        xmlDoc = mxmlLoadString(NULL, data, QueryXmlDataCallback);
        // Check our XML.
        if (xmlDoc == NULL || xmlDoc->type != MXML_ELEMENT)
        {
            LogEvent(NULL, PhCreateString(L"Updater: (WorkerThreadStart) mxmlLoadString failed."));
            __leave;
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
            LogEvent(NULL, PhCreateString(L"Updater: (WorkerThreadStart) mxmlLoadString xmlNodeVer failed."));
            __leave;
        }
        if (xmlNodeRelDate == NULL || xmlNodeRelDate->child == NULL || xmlNodeRelDate->type != MXML_ELEMENT)
        {
            LogEvent(NULL, PhCreateString(L"Updater: (WorkerThreadStart) mxmlLoadString xmlNodeRelDate failed."));
            __leave;
        }
        if (xmlNodeSize == NULL || xmlNodeSize->child == NULL || xmlNodeSize->type != MXML_ELEMENT)
        {
            LogEvent(NULL, PhCreateString(L"Updater: (WorkerThreadStart) mxmlLoadString xmlNodeSize failed."));
            __leave;
        }
        if (xmlNodeHash == NULL || xmlNodeHash->child == NULL || xmlNodeHash->type != MXML_ELEMENT)
        {
            LogEvent(NULL, PhCreateString(L"Updater: (WorkerThreadStart) mxmlLoadString xmlNodeHash failed."));
            __leave;
        }

        // Format strings into unicode
        XmlData->Version = PhFormatString(
            L"%hs",
            xmlNodeVer->child->value.opaque
            );
        XmlData->RelDate = PhFormatString(
            L"%hs",
            xmlNodeRelDate->child->value.opaque
            );
        XmlData->Size = PhFormatString(
            L"%hs",
            xmlNodeSize->child->value.opaque
            );
        XmlData->Hash = PhFormatString(
            L"%hs",
            xmlNodeHash->child->value.opaque
            );

        // parse and check string
        //if (!ParseVersionString(XmlData->Version->Buffer, &XmlData->MajorVersion, &XmlData->MinorVersion))
        //    __leave;
        if (XmlData->Version)
        {
            PH_STRINGREF sr, majorPart, minorPart;
            ULONG64 majorInteger = 0, minorInteger = 0;

            PhInitializeStringRef(&sr, XmlData->Version->Buffer);

            if (PhSplitStringRefAtChar(&sr, '.', &majorPart, &minorPart))
            {
                PhStringToInteger64(&majorPart, 10, &majorInteger);
                PhStringToInteger64(&minorPart, 10, &minorInteger);

                XmlData->MajorVersion = (ULONG)majorInteger;
                XmlData->MinorVersion = (ULONG)minorInteger;

                isSuccess = TRUE;
            }
        }
    }
    __finally
    {
        if (xmlDoc)
            mxmlDelete(xmlDoc);

        if (netInitialize)
            InternetCloseHandle(netInitialize);

        if (netConnection)
            InternetCloseHandle(netConnection);

        if (netRequest)
            InternetCloseHandle(netRequest);

        if (userAgent)
            PhDereferenceObject(userAgent);

        if (phVersion)
            PhDereferenceObject(phVersion);
    }

    return isSuccess;
}

mxml_type_t QueryXmlDataCallback(
    __in mxml_node_t *node
    )
{
    return MXML_OPAQUE;
}

VOID ShowUpdateDialog(
    VOID
    )
{
    if (!UpdaterDialogThreadHandle)
    {
        if (!(UpdaterDialogThreadHandle = PhCreateThread(0, ShowUpdateDialogThreadStart, NULL)))
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
    PhQueueItemGlobalWorkQueue(SilentUpdateCheckThreadStart, NULL);
}