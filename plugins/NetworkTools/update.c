/*
 * Copyright (c) 2022 Winsider Seminars & Solutions, Inc.  All rights reserved.
 *
 * This file is part of System Informer.
 *
 * Authors:
 *
 *     dmex    2016-2021
 *
 */

#include "nettools.h"
#include <commonutil.h>
#include <shellapi.h>
#include "..\..\tools\thirdparty\gzip\zlib.h"
#include "..\..\tools\thirdparty\gzip\gzguts.h"

HWND UpdateDialogHandle = NULL;
HANDLE UpdateDialogThreadHandle = NULL;
PH_EVENT InitializedEvent = PH_EVENT_INIT;
PPH_OBJECT_TYPE UpdateContextType = NULL;
PH_INITONCE UpdateContextTypeInitOnce = PH_INITONCE_INIT;

VOID UpdateContextDeleteProcedure(
    _In_ PVOID Object,
    _In_ ULONG Flags
    )
{
    PPH_UPDATER_CONTEXT context = Object;

    if (context->FileDownloadUrl)
        PhDereferenceObject(context->FileDownloadUrl);
}

PPH_UPDATER_CONTEXT CreateUpdateContext(
    VOID
    )
{
    PPH_UPDATER_CONTEXT context;

    if (PhBeginInitOnce(&UpdateContextTypeInitOnce))
    {
        UpdateContextType = PhCreateObjectType(L"GeoIpContextObjectType", 0, UpdateContextDeleteProcedure);
        PhEndInitOnce(&UpdateContextTypeInitOnce);
    }

    context = PhCreateObject(sizeof(PH_UPDATER_CONTEXT), UpdateContextType);
    memset(context, 0, sizeof(PH_UPDATER_CONTEXT));

    return context;
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
    
    //redirectUrl = PhCreateString(L"https://geolite.maxmind.com/download/geoip/database/GeoLite2-City.tar.gz");
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
    PPH_STRING fwLinkUrl;
    PPH_HTTP_CONTEXT httpContext = NULL;
    PPH_STRING zipFilePath = NULL;
    PPH_STRING versionString = NULL;
    PPH_STRING userAgentString = NULL;
    PPH_STRING httpHostName = NULL;
    PPH_STRING httpHostPath = NULL;
    PPH_STRING dbpath = NULL;
    PPH_BYTES mmdbGzPath = NULL;
    gzFile gzfile = NULL;
    USHORT httpHostPort = 0;
    LARGE_INTEGER timeNow;
    LARGE_INTEGER timeStart;
    ULONG64 timeTicks;
    ULONG64 timeBitsPerSecond;
    PPH_UPDATER_CONTEXT context = (PPH_UPDATER_CONTEXT)Parameter;

    SendMessage(context->DialogHandle, TDM_UPDATE_ELEMENT_TEXT, TDE_MAIN_INSTRUCTION, (LPARAM)L"Initializing download request...");

    if (!(fwLinkUrl = QueryFwLinkUrl(context)))
        goto CleanupExit;

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

        status = PhFormatString(L"Downloading GeoLite2-Country...");
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
        else
        {
            LARGE_INTEGER allocationSize;

            zipFilePath = PhCreateCacheFile(PhaCreateString(L"GeoLite2-Country.mmdb.gz"));

            if (PhIsNullOrEmptyString(zipFilePath))
                goto CleanupExit;

            allocationSize.QuadPart = contentLength;

            if (!NT_SUCCESS(PhCreateFileWin32Ex(
                &tempFileHandle,
                PhGetString(zipFilePath),
                FILE_GENERIC_READ | FILE_GENERIC_WRITE,
                &allocationSize,
                FILE_ATTRIBUTE_NORMAL,
                FILE_SHARE_READ | FILE_SHARE_WRITE | FILE_SHARE_DELETE,
                FILE_OVERWRITE_IF,
                FILE_NON_DIRECTORY_FILE | FILE_SYNCHRONOUS_IO_NONALERT,
                NULL
                )))
            {
                goto CleanupExit;
            }
        }

        PhQuerySystemTime(&timeStart);

        while (PhHttpSocketReadData(httpContext, buffer, PAGE_SIZE, &bytesDownloaded))
        {
            // If we get zero bytes, the file was downloaded or there was an error.
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

            downloadedBytes += (ULONG)isb.Information;

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
                PH_FORMAT format[9];
                WCHAR string[MAX_PATH];

                // L"Downloaded: %s of %s (%.0f%%)\r\nSpeed: %s/s"
                PhInitFormatS(&format[0], L"Downloaded: ");
                PhInitFormatSize(&format[1], downloadedBytes);
                PhInitFormatS(&format[2], L" of ");
                PhInitFormatSize(&format[3], contentLength);
                PhInitFormatS(&format[4], L" (");
                PhInitFormatF(&format[5], percent, 1);
                PhInitFormatS(&format[6], L"%)\r\nSpeed: ");
                PhInitFormatSize(&format[7], timeBitsPerSecond);
                PhInitFormatS(&format[8], L"/s");

                if (PhFormatToBuffer(format, RTL_NUMBER_OF(format), string, sizeof(string), NULL))
                {
                    SendMessage(context->DialogHandle, TDM_UPDATE_ELEMENT_TEXT, TDE_CONTENT, (LPARAM)string);
                }

                SendMessage(context->DialogHandle, TDM_SET_PROGRESS_BAR_POS, (WPARAM)percent, 0);
            }
        }

        {
            dbpath = NetToolsGetGeoLiteDbPath(SETTING_NAME_DB_LOCATION);

            if (PhIsNullOrEmptyString(dbpath))
                goto CleanupExit;

            if (PhDoesFileExistsWin32(PhGetString(dbpath)))
            {
                if (!NT_SUCCESS(PhDeleteFileWin32(PhGetString(dbpath))))
                    goto CleanupExit;
            }
            else
            {
                PPH_STRING fullPath;
                ULONG indexOfFileName;

                // Create the directory if it does not exist.
                if (fullPath = PhGetFullPath(dbpath->Buffer, &indexOfFileName))
                {
                    if (indexOfFileName != ULONG_MAX)
                        PhCreateDirectory(PhaSubstring(fullPath, 0, indexOfFileName));

                    PhDereferenceObject(fullPath);
                }
            }

            mmdbGzPath = PhConvertUtf16ToUtf8(PhGetString(zipFilePath));

            if (gzfile = gzopen(mmdbGzPath->Buffer, "rb"))
            {
                HANDLE mmdbFileHandle;

                if (NT_SUCCESS(PhCreateFileWin32(
                    &mmdbFileHandle,
                    PhGetStringOrEmpty(dbpath),
                    FILE_GENERIC_READ | FILE_GENERIC_WRITE,
                    FILE_ATTRIBUTE_NORMAL,
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
                gzfile = NULL;
            }
        }
    }

CleanupExit:

    if (gzfile)
        gzclose(gzfile);
    if (mmdbGzPath)
        PhDereferenceObject(mmdbGzPath);
    if (dbpath)
        PhDereferenceObject(dbpath);

    if (tempFileHandle)
        NtClose(tempFileHandle);

    if (httpContext)
        PhHttpSocketDestroy(httpContext);

    if (userAgentString)
        PhDereferenceObject(userAgentString);
    if (versionString)
        PhDereferenceObject(versionString);

    if (zipFilePath)
    {
        PhDeleteCacheFile(zipFilePath);
        PhDereferenceObject(zipFilePath);
    }

    if (context->DialogHandle)
    {
        PostMessage(context->DialogHandle,
            success ? PH_SHOWINSTALL : PH_SHOWERROR, 0, 0);
    }

    PhDereferenceObject(context);
    return STATUS_SUCCESS;
}

LRESULT CALLBACK TaskDialogSubclassProc(
    _In_ HWND hwndDlg,
    _In_ UINT uMsg,
    _In_ WPARAM wParam,
    _In_ LPARAM lParam
    )
{
    PPH_UPDATER_CONTEXT context;
    WNDPROC oldWndProc;

    if (!(context = PhGetWindowContext(hwndDlg, UCHAR_MAX)))
        return 0;

    oldWndProc = context->DefaultWindowProc;

    switch (uMsg)
    {
    case WM_DESTROY:
        {
            SetWindowLongPtr(hwndDlg, GWLP_WNDPROC, (LONG_PTR)oldWndProc);
            PhRemoveWindowContext(hwndDlg, UCHAR_MAX);

            PhUnregisterWindowCallback(hwndDlg);
        }
        break;
    case PH_SHOWDIALOG:
        {
            if (IsMinimized(hwndDlg))
                ShowWindow(hwndDlg, SW_RESTORE);
            else
                ShowWindow(hwndDlg, SW_SHOW);

            SetForegroundWindow(hwndDlg);
        }
        break;
    case PH_SHOWINSTALL:
        {
            ShowDbInstallRestartDialog(context);
        }
        break;
    case PH_SHOWERROR:
        {
            ShowDbUpdateFailedDialog(context);
        }
        break;
    }

    return CallWindowProc(oldWndProc, hwndDlg, uMsg, wParam, lParam);
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
            PhCenterWindow(hwndDlg, PhMainWndHandle);

            // Create the Taskdialog icons
            PhSetApplicationWindowIcon(hwndDlg);

            PhRegisterWindowCallback(hwndDlg, PH_PLUGIN_WINDOW_EVENT_TYPE_TOPMOST, NULL);

            // Subclass the Taskdialog.
            context->DefaultWindowProc = (WNDPROC)GetWindowLongPtr(hwndDlg, GWLP_WNDPROC);
            PhSetWindowContext(hwndDlg, UCHAR_MAX, context);
            SetWindowLongPtr(hwndDlg, GWLP_WNDPROC, (LONG_PTR)TaskDialogSubclassProc);

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
    TASKDIALOGCONFIG config = { sizeof(TASKDIALOGCONFIG) };

    PhInitializeAutoPool(&autoPool);

    context = CreateUpdateContext();

    config.dwFlags = TDF_ALLOW_DIALOG_CANCELLATION | TDF_CAN_BE_MINIMIZED;
    config.pszContent = L"Initializing...";
    config.lpCallbackData = (LONG_PTR)context;
    config.pfCallback = TaskDialogBootstrapCallback;
    config.hwndParent = Parameter;

    TaskDialogIndirect(&config, NULL, NULL, NULL);

    PhDereferenceObject(context);
    PhDeleteAutoPool(&autoPool);

    if (UpdateDialogThreadHandle)
    {
        NtClose(UpdateDialogThreadHandle);
        UpdateDialogThreadHandle = NULL;
    }

    PhResetEvent(&InitializedEvent);

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
    //ProcessHacker_PrepareForEarlyShutdown();
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
    //            ProcessHacker_Destroy();
    //        }
    //    }
    //
    //    NtClose(info.hProcess);
    //}
    //else
    //{
    //    ProcessHacker_CancelEarlyShutdown();
    //}
}

VOID ShowGeoIPUpdateDialog(
    VOID
    )
{
    if (!UpdateDialogThreadHandle)
    {
        if (!NT_SUCCESS(PhCreateThreadEx(&UpdateDialogThreadHandle, GeoIPUpdateDialogThread, NULL)))
        {
            PhShowError(NULL, L"%s", L"Unable to create the window.");
            return;
        }

        PhWaitForEvent(&InitializedEvent, NULL);
    }

    PostMessage(UpdateDialogHandle, PH_SHOWDIALOG, 0, 0);
}
