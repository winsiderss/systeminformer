/*
 * Process Hacker Network Tools -
 *   GeoIP database updater
 *
 * Copyright (C) 2016 dmex
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

#include "nettools.h"
#include "zlib\zlib.h"
#include "zlib\gzguts.h"
#include <commonutil.h>
#include <shellapi.h>

HWND UpdateDialogHandle = NULL;
HANDLE UpdateDialogThreadHandle = NULL;
PH_EVENT InitializedEvent = PH_EVENT_INIT;

VOID FreeUpdateContext(
    _In_ _Post_invalid_ PPH_UPDATER_CONTEXT Context
    )
{
    if (Context->FileDownloadUrl)
        PhDereferenceObject(Context->FileDownloadUrl);

    if (Context->RevVersion)
        PhDereferenceObject(Context->RevVersion);

    if (Context->Size)
        PhDereferenceObject(Context->Size);

    if (Context->SetupFilePath)
        PhDereferenceObject(Context->SetupFilePath);

    PhDereferenceObject(Context);
}

VOID TaskDialogCreateIcons(
    _In_ PPH_UPDATER_CONTEXT Context
    )
{
    Context->IconSmallHandle = PH_LOAD_SHARED_ICON_SMALL(PhInstanceHandle, MAKEINTRESOURCE(PHAPP_IDI_PROCESSHACKER));
    Context->IconLargeHandle = PH_LOAD_SHARED_ICON_LARGE(PhInstanceHandle, MAKEINTRESOURCE(PHAPP_IDI_PROCESSHACKER));

    SendMessage(Context->DialogHandle, WM_SETICON, ICON_SMALL, (LPARAM)Context->IconSmallHandle);
    SendMessage(Context->DialogHandle, WM_SETICON, ICON_BIG, (LPARAM)Context->IconLargeHandle);
}

VOID TaskDialogLinkClicked(
    _In_ PPH_UPDATER_CONTEXT Context
    )
{
    PhShellExecute(Context->DialogHandle, L"https://www.maxmind.com", NULL);
}

PPH_STRING UpdateVersionString(
    VOID
    )
{
    ULONG majorVersion;
    ULONG minorVersion;
    ULONG revisionVersion;
    PPH_STRING currentVersion;
    PPH_STRING versionHeader = NULL;

    PhGetPhVersionNumbers(&majorVersion, &minorVersion, NULL, &revisionVersion);

    if (currentVersion = PhFormatString(L"%lu.%lu.%lu", majorVersion, minorVersion, revisionVersion))
    {
        versionHeader = PhConcatStrings2(L"ProcessHacker-Build: ", currentVersion->Buffer);
        PhDereferenceObject(currentVersion);
    }

    return versionHeader;
}

PPH_STRING UpdateWindowsString(
    VOID
    )
{
    static PH_STRINGREF keyName = PH_STRINGREF_INIT(L"Software\\Microsoft\\Windows NT\\CurrentVersion");

    HANDLE keyHandle;
    PPH_STRING buildLabHeader = NULL;

    if (NT_SUCCESS(PhOpenKey(
        &keyHandle,
        KEY_READ,
        PH_KEY_LOCAL_MACHINE,
        &keyName,
        0
        )))
    {
        PPH_STRING buildLabString;

        if (buildLabString = PhQueryRegistryString(keyHandle, L"BuildLabEx"))
        {
            buildLabHeader = PhConcatStrings2(L"ProcessHacker_OsBuild: ", buildLabString->Buffer);
            PhDereferenceObject(buildLabString);
        }
        else if (buildLabString = PhQueryRegistryString(keyHandle, L"BuildLab"))
        {
            buildLabHeader = PhConcatStrings2(L"ProcessHacker_OsBuild: ", buildLabString->Buffer);
            PhDereferenceObject(buildLabString);
        }

        NtClose(keyHandle);
    }

    return buildLabHeader;
}

PPH_STRING QueryFwLinkUrl(
    _In_ PPH_UPDATER_CONTEXT Context
    )
{
    PPH_STRING redirectUrl = NULL;
    PPH_HTTP_CONTEXT httpContext = NULL;
    PPH_STRING versionString = NULL;
    PPH_STRING userAgentString = NULL;
    PPH_STRING versionHeader;
    PPH_STRING windowsHeader;

    versionString = PhGetPhVersion();
    userAgentString = PhConcatStrings2(L"ProcessHacker_", versionString->Buffer);

    if (!PhHttpSocketCreate(&httpContext, PhGetString(userAgentString)))
        goto CleanupExit;

    if (!PhHttpSocketConnect(httpContext, L"wj32.org", PH_HTTP_DEFAULT_HTTPS_PORT))
        goto CleanupExit;

    if (!PhHttpSocketBeginRequest(
        httpContext,
        NULL,
        L"/processhacker/fwlink/maxminddb.php",
        PH_HTTP_FLAG_REFRESH | PH_HTTP_FLAG_SECURE
        ))
    {
        goto CleanupExit;
    }

    if (versionHeader = UpdateVersionString())
    {
        PhHttpSocketAddRequestHeaders(httpContext, versionHeader->Buffer, (ULONG)versionHeader->Length / sizeof(WCHAR));
        PhDereferenceObject(versionHeader);
    }

    if (windowsHeader = UpdateWindowsString())
    {
        PhHttpSocketAddRequestHeaders(httpContext, windowsHeader->Buffer, (ULONG)windowsHeader->Length / sizeof(WCHAR));
        PhDereferenceObject(windowsHeader);
    }

    if (!PhHttpSocketSetFeature(httpContext, PH_HTTP_FEATURE_REDIRECTS, FALSE))
        goto CleanupExit;

    if (!PhHttpSocketSendRequest(httpContext, NULL, 0))
        goto CleanupExit;

    if (!PhHttpSocketEndRequest(httpContext))
        goto CleanupExit;
    
    redirectUrl = PhHttpSocketQueryHeaderString(httpContext, L"Location"); // WINHTTP_QUERY_LOCATION

CleanupExit:

    if (httpContext)
        PhHttpSocketDestroy(httpContext);
    
    PhClearReference(&versionString);
    PhClearReference(&userAgentString);

    return redirectUrl;
}

NTSTATUS GeoIPUpdateThread(
    _In_ PVOID Parameter
    )
{
    BOOLEAN success = FALSE;
    HANDLE tempFileHandle = NULL;
    PPH_STRING fwLinkUrl = NULL;
    PPH_HTTP_CONTEXT httpContext = NULL;
    PPH_STRING versionString = NULL;
    PPH_STRING userAgentString = NULL;
    PPH_STRING httpHostName = NULL;
    PPH_STRING httpHostPath = NULL;
    USHORT httpHostPort = 0;
    LARGE_INTEGER timeNow;
    LARGE_INTEGER timeStart;
    ULONG64 timeTicks = 0;
    ULONG64 timeBitsPerSecond = 0;
    PPH_UPDATER_CONTEXT context = (PPH_UPDATER_CONTEXT)Parameter;

    SendMessage(context->DialogHandle, TDM_UPDATE_ELEMENT_TEXT, TDE_MAIN_INSTRUCTION, (LPARAM)L"Initializing download request...");

    if (!(fwLinkUrl = QueryFwLinkUrl(context)))
        goto CleanupExit;

    context->SetupFilePath = PhCreateCacheFile(PhaCreateString(L"GeoLite2-Country.mmdb.gz"));

    if (PhIsNullOrEmptyString(context->SetupFilePath))
        goto CleanupExit;

    if (!NT_SUCCESS(PhCreateFileWin32(
        &tempFileHandle,
        PhGetString(context->SetupFilePath),
        FILE_GENERIC_READ | FILE_GENERIC_WRITE,
        FILE_ATTRIBUTE_NOT_CONTENT_INDEXED | FILE_ATTRIBUTE_TEMPORARY,
        FILE_SHARE_READ | FILE_SHARE_WRITE | FILE_SHARE_DELETE,
        FILE_OVERWRITE_IF,
        FILE_NON_DIRECTORY_FILE | FILE_SYNCHRONOUS_IO_NONALERT
        )))
    {
        goto CleanupExit;
    }

    SendMessage(context->DialogHandle, TDM_UPDATE_ELEMENT_TEXT, TDE_MAIN_INSTRUCTION, (LPARAM)L"Connecting...");

    if (!PhHttpSocketParseUrl(
        fwLinkUrl,
        &httpHostName,
        &httpHostPath,
        &httpHostPort
        ))
    {
        goto CleanupExit;
    }

    versionString = PhGetPhVersion();
    userAgentString = PhConcatStrings2(L"ProcessHacker_", versionString->Buffer);

    if (!PhHttpSocketCreate(
        &httpContext,
        PhGetString(userAgentString)
        ))
    {
        goto CleanupExit;
    }

    if (!PhHttpSocketConnect(
        httpContext,
        PhGetString(httpHostName),
        httpHostPort
        ))
    {
        goto CleanupExit;
    }

    if (!PhHttpSocketBeginRequest(
        httpContext,
        NULL,
        PhGetString(httpHostPath),
        PH_HTTP_FLAG_REFRESH | (httpHostPort == PH_HTTP_DEFAULT_HTTPS_PORT ? PH_HTTP_FLAG_SECURE : 0)
        ))
    {
        goto CleanupExit;
    }

    SendMessage(context->DialogHandle, TDM_UPDATE_ELEMENT_TEXT, TDE_MAIN_INSTRUCTION, (LPARAM)L"Sending download request...");

    if (!PhHttpSocketSendRequest(httpContext, NULL, 0))
        goto CleanupExit;

    SendMessage(context->DialogHandle, TDM_UPDATE_ELEMENT_TEXT, TDE_MAIN_INSTRUCTION, (LPARAM)L"Waiting for response...");

    if (PhHttpSocketEndRequest(httpContext))
    {
        ULONG bytesDownloaded = 0;
        ULONG downloadedBytes = 0;
        ULONG contentLength = 0;
        PPH_STRING status;
        IO_STATUS_BLOCK isb;
        BYTE buffer[PAGE_SIZE];

        memset(buffer, 0, PAGE_SIZE);

        status = PhFormatString(L"Downloading GeoLite2-Country.mmdb...");
        SendMessage(context->DialogHandle, TDM_SET_MARQUEE_PROGRESS_BAR, FALSE, 0);
        SendMessage(context->DialogHandle, TDM_UPDATE_ELEMENT_TEXT, TDE_MAIN_INSTRUCTION, (LPARAM)status->Buffer);
        PhDereferenceObject(status);

        if (!PhHttpSocketQueryHeaderUlong(
            httpContext,
            PH_HTTP_QUERY_CONTENT_LENGTH,
            &contentLength
            ))
        {
            //context->ErrorCode = GetLastError();
            goto CleanupExit;
        }

        PhQuerySystemTime(&timeStart);

        while (PhHttpSocketReadData(httpContext, buffer, PAGE_SIZE, &bytesDownloaded))
        {
            // If we get zero bytes, the file was uploaded or there was an error.
            if (bytesDownloaded == 0)
                break;

            // If the dialog was closed; cleanup and exit.
            if (!context->DialogHandle)
                goto CleanupExit;

            if (!NT_SUCCESS(NtWriteFile(
                tempFileHandle,
                NULL,
                NULL,
                NULL,
                &isb,
                buffer,
                bytesDownloaded,
                NULL,
                NULL
                )))
            {
                goto CleanupExit;
            }

            downloadedBytes += (DWORD)isb.Information;

            // Check the number of bytes written are the same we downloaded.
            if (bytesDownloaded != isb.Information)
                goto CleanupExit;

            // Query the current time
            PhQuerySystemTime(&timeNow);

            // Calculate the number of ticks
            timeTicks = (timeNow.QuadPart - timeStart.QuadPart) / PH_TICKS_PER_SEC;
            timeBitsPerSecond = downloadedBytes / __max(timeTicks, 1);

            // TODO: Update on timer callback.
            {
                FLOAT percent = ((FLOAT)downloadedBytes / contentLength * 100);
                PPH_STRING totalLength = PhFormatSize(contentLength, -1);
                PPH_STRING totalDownloaded = PhFormatSize(downloadedBytes, -1);
                PPH_STRING totalSpeed = PhFormatSize(timeBitsPerSecond, -1);

                PPH_STRING statusMessage = PhFormatString(
                    L"Downloaded: %s of %s (%.0f%%)\r\nSpeed: %s/s",
                    PhGetStringOrEmpty(totalDownloaded),
                    PhGetStringOrEmpty(totalLength),
                    percent,
                    PhGetStringOrEmpty(totalSpeed)
                    );

                SendMessage(context->DialogHandle, TDM_UPDATE_ELEMENT_TEXT, TDE_CONTENT, (LPARAM)statusMessage->Buffer);
                SendMessage(context->DialogHandle, TDM_SET_PROGRESS_BAR_POS, (WPARAM)percent, 0);

                PhDereferenceObject(statusMessage);
                PhDereferenceObject(totalSpeed);
                PhDereferenceObject(totalLength);
                PhDereferenceObject(totalDownloaded);
            }
        }

        {
            PPH_STRING dbpath;
            PPH_BYTES mmdbGzPath;
            gzFile gzfile;

            dbpath = PhGetExpandStringSetting(SETTING_NAME_DB_LOCATION);

            if (PhIsNullOrEmptyString(dbpath))
                PhMoveReference(&dbpath, PhGetKnownLocation(CSIDL_APPDATA, L"\\Process Hacker\\GeoLite2-Country.mmdb"));

            PhAutoDereferenceObject(dbpath);
            mmdbGzPath = PH_AUTO(PhConvertUtf16ToUtf8(PhGetString(context->SetupFilePath)));

            if (RtlDoesFileExists_U(PhGetString(dbpath)))
            {
                if (!NT_SUCCESS(PhDeleteFileWin32(PhGetString(dbpath))))
                    goto CleanupExit;
            }
            else
            {
                // Create the directory if it does not exist.

                PPH_STRING fullPath;
                ULONG indexOfFileName;

                if (fullPath = PH_AUTO(PhGetFullPath(dbpath->Buffer, &indexOfFileName)))
                {
                    if (indexOfFileName != -1)
                        PhCreateDirectory(PhaSubstring(fullPath, 0, indexOfFileName));
                }
            }

            if (gzfile = gzopen(mmdbGzPath->Buffer, "rb"))
            {
                HANDLE mmdbFileHandle;

                if (NT_SUCCESS(PhCreateFileWin32(
                    &mmdbFileHandle,
                    PhGetStringOrEmpty(dbpath),
                    FILE_GENERIC_READ | FILE_GENERIC_WRITE,
                    FILE_ATTRIBUTE_NOT_CONTENT_INDEXED | FILE_ATTRIBUTE_TEMPORARY,
                    FILE_SHARE_READ | FILE_SHARE_WRITE | FILE_SHARE_DELETE,
                    FILE_OVERWRITE_IF,
                    FILE_NON_DIRECTORY_FILE | FILE_SYNCHRONOUS_IO_NONALERT
                    )))
                {
                    IO_STATUS_BLOCK isb;
                    BYTE buffer[PAGE_SIZE];

                    while (!gzeof(gzfile))
                    {
                        INT bytes = gzread(gzfile, buffer, sizeof(buffer));

                        if (bytes == -1)
                        {
                            NtClose(mmdbFileHandle);
                            goto CleanupExit;
                        }

                        if (!NT_SUCCESS(NtWriteFile(
                            mmdbFileHandle,
                            NULL,
                            NULL,
                            NULL,
                            &isb,
                            buffer,
                            bytes,
                            NULL,
                            NULL
                            )))
                        {
                            NtClose(mmdbFileHandle);
                            goto CleanupExit;
                        }
                    }

                    success = TRUE;

                    NtClose(mmdbFileHandle);
                }

                gzclose(gzfile);
            }
        }
    }

CleanupExit:

    if (tempFileHandle)
        NtClose(tempFileHandle);

    if (httpContext)
        PhHttpSocketDestroy(httpContext);

    if (userAgentString)
        PhDereferenceObject(userAgentString);
    if (versionString)
        PhDereferenceObject(versionString);

    if (context->SetupFilePath)
    {
        PhDeleteCacheFile(context->SetupFilePath);
        PhDereferenceObject(context->SetupFilePath);
    }

    if (context->DialogHandle)
    {
        if (success)
        {
            ShowDbInstallRestartDialog(context);
        }
        else
        {
            ShowDbUpdateFailedDialog(context);
        }
    }

    PhDereferenceObject(context);
    return STATUS_SUCCESS;
}

HRESULT CALLBACK TaskDialogBootstrapCallback(
    _In_ HWND hwndDlg,
    _In_ UINT uMsg,
    _In_ WPARAM wParam,
    _In_ LPARAM lParam,
    _In_ LONG_PTR dwRefData
    )
{
    PPH_UPDATER_CONTEXT context = (PPH_UPDATER_CONTEXT)dwRefData;

    switch (uMsg)
    {
    case TDN_CREATED:
        {
            UpdateDialogHandle = context->DialogHandle = hwndDlg;

            // Center the update window on PH if it's visible else we center on the desktop.
            PhCenterWindow(hwndDlg, (IsWindowVisible(PhMainWndHandle) && !IsMinimized(PhMainWndHandle)) ? PhMainWndHandle : NULL);

            // Create the Taskdialog icons
            TaskDialogCreateIcons(context);

            ShowDbCheckForUpdatesDialog(context);
        }
        break;
    }

    return S_OK;
}

NTSTATUS GeoIPUpdateDialogThread(
    _In_ PVOID Parameter
    )
{
    PH_AUTO_POOL autoPool;
    PPH_UPDATER_CONTEXT context;
    INT result = 0;
    TASKDIALOGCONFIG config = { sizeof(TASKDIALOGCONFIG) };

    PhInitializeAutoPool(&autoPool);

    context = (PPH_UPDATER_CONTEXT)PhCreateAlloc(sizeof(PH_UPDATER_CONTEXT));
    memset(context, 0, sizeof(PH_UPDATER_CONTEXT));

    config.dwFlags = TDF_ALLOW_DIALOG_CANCELLATION | TDF_CAN_BE_MINIMIZED;
    config.pszContent = L"Initializing...";
    config.lpCallbackData = (LONG_PTR)context;
    config.pfCallback = TaskDialogBootstrapCallback;
    config.hwndParent = Parameter;

    TaskDialogIndirect(&config, &result, NULL, NULL);

    FreeUpdateContext(context);
    PhDeleteAutoPool(&autoPool);

    return STATUS_SUCCESS;

    //SHELLEXECUTEINFO info = { sizeof(SHELLEXECUTEINFO) };
    //
    //info.lpFile = L"ProcessHacker.exe";
    //info.lpParameters = L"-plugin " PLUGIN_NAME L":UpdateGeoIp";
    //info.fMask = SEE_MASK_NOCLOSEPROCESS | SEE_MASK_NOASYNC;
    //info.nShow = SW_SHOW;
    //info.hwnd = Parameter;
    //info.lpVerb = L"runas";
    //
    //ProcessHacker_PrepareForEarlyShutdown(PhMainWndHandle);
    //
    //if (ShellExecuteEx(&info))
    //{
    //    LARGE_INTEGER timeout;
    //    PROCESS_BASIC_INFORMATION basic;
    //
    //    NtWaitForSingleObject(info.hProcess, FALSE, PhTimeoutFromMilliseconds(&timeout, INFINITE));
    //
    //    if (NT_SUCCESS(PhGetProcessBasicInformation(info.hProcess, &basic)))
    //    {
    //        if (basic.ExitStatus == STATUS_ALREADY_COMPLETE)
    //        {
    //            PhShellProcessHacker(
    //                Parameter,
    //                NULL,
    //                SW_SHOW,
    //                0,
    //                PH_SHELL_APP_PROPAGATE_PARAMETERS | PH_SHELL_APP_PROPAGATE_PARAMETERS_IGNORE_VISIBILITY,
    //                0,
    //                NULL
    //                );
    //
    //            ProcessHacker_Destroy(PhMainWndHandle);
    //        }
    //    }
    //
    //    NtClose(info.hProcess);
    //}
    //else
    //{
    //    ProcessHacker_CancelEarlyShutdown(PhMainWndHandle);
    //}
}

VOID ShowGeoIPUpdateDialog(
    _In_opt_ HWND Parent
    )
{
    PhCreateThread2(GeoIPUpdateDialogThread, Parent);
}