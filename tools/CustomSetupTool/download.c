/*
 * Copyright (c) 2022 Winsider Seminars & Solutions, Inc.  All rights reserved.
 *
 * This file is part of System Informer.
 *
 * Authors:
 *
 *     dmex
 *
 */

#include "setup.h"

PPH_STRING SetupGetVersion(
    VOID
)
{
    PH_FORMAT format[7];

    PhInitFormatU(&format[0], PHAPP_VERSION_MAJOR);
    PhInitFormatC(&format[1], '.');
    PhInitFormatU(&format[2], PHAPP_VERSION_MINOR);
    PhInitFormatC(&format[3], '.');
    PhInitFormatU(&format[4], PHAPP_VERSION_REVISION);
    PhInitFormatC(&format[5], '.');
    PhInitFormatU(&format[6], PHAPP_VERSION_BUILD);

    return PhFormat(format, 7, 16);
}

PPH_STRING UpdateVersionString(
    VOID
    )
{
    PPH_STRING currentVersion = NULL;
    PPH_STRING versionHeader = NULL;

    if (currentVersion = SetupGetVersion())
    {
        versionHeader = PhConcatStrings2(L"SI-SETUP-BUILD: ", currentVersion->Buffer);
        PhDereferenceObject(currentVersion);
    }

    return versionHeader;
}

PPH_STRING UpdateWindowsString(
    VOID
    )
{
    PPH_STRING buildString = NULL;
    PPH_STRING fileName = NULL;
    PPH_STRING fileVersion = NULL;
    PVOID versionInfo;
    VS_FIXEDFILEINFO* rootBlock;
    PH_FORMAT fileVersionFormat[3];

    fileName = PhGetKernelFileName();
    versionInfo = PhGetFileVersionInfoEx(&fileName->sr);
    PhDereferenceObject(fileName);

    if (versionInfo)
    {
        if (rootBlock = PhGetFileVersionFixedInfo(versionInfo))
        {
            PhInitFormatU(&fileVersionFormat[0], HIWORD(rootBlock->dwFileVersionLS));
            PhInitFormatC(&fileVersionFormat[1], '.');
            PhInitFormatU(&fileVersionFormat[2], LOWORD(rootBlock->dwFileVersionLS));

            fileVersion = PhFormat(fileVersionFormat, 3, 30);
        }

        PhFree(versionInfo);
    }

    if (fileVersion)
    {
        if (PhIsExecutingInWow64())
            buildString = PhFormatString(L"%s: %s.%s", L"SystemInformer-OsBuild", fileVersion->Buffer, L"x86fre");
        else
            buildString = PhFormatString(L"%s: %s.%s", L"SystemInformer-OsBuild", fileVersion->Buffer, L"amd64fre");

        PhDereferenceObject(fileVersion);
    }

    return buildString;

    //static PH_STRINGREF keyName = PH_STRINGREF_INIT(L"Software\\Microsoft\\Windows NT\\CurrentVersion");
    //HANDLE keyHandle;
    //PPH_STRING buildLabHeader = NULL;
    //
    //if (NT_SUCCESS(PhOpenKey(
    //    &keyHandle,
    //    KEY_READ,
    //    PH_KEY_LOCAL_MACHINE,
    //    &keyName,
    //    0
    //    )))
    //{
    //    PPH_STRING buildLabString;
    //
    //    if (buildLabString = PhQueryRegistryString(keyHandle, L"BuildLabEx"))
    //    {
    //        buildLabHeader = PhConcatStrings2(L"SI-OsBuild: ", buildLabString->Buffer);
    //        PhDereferenceObject(buildLabString);
    //    }
    //    else if (buildLabString = PhQueryRegistryString(keyHandle, L"BuildLab"))
    //    {
    //        buildLabHeader = PhConcatStrings2(L"SI-OsBuild: ", buildLabString->Buffer);
    //        PhDereferenceObject(buildLabString);
    //    }
    //
    //    NtClose(keyHandle);
    //}
    //
    //return buildLabHeader;
}

ULONG64 ParseVersionString(
    _Inout_ PPH_STRING VersionString
    )
{
    PH_STRINGREF remaining, majorPart, minorPart, revisionPart;
    ULONG64 majorInteger = 0, minorInteger = 0, revisionInteger = 0;

    PhInitializeStringRef(&remaining, PhGetStringOrEmpty(VersionString));
    PhSplitStringRefAtChar(&remaining, '.', &majorPart, &remaining);
    PhSplitStringRefAtChar(&remaining, '.', &minorPart, &remaining);
    PhSplitStringRefAtChar(&remaining, '.', &revisionPart, &remaining);

    PhStringToInteger64(&majorPart, 10, &majorInteger);
    PhStringToInteger64(&minorPart, 10, &minorInteger);
    PhStringToInteger64(&revisionPart, 10, &revisionInteger);

    return MAKE_VERSION_ULONGLONG(
        (ULONG)majorInteger,
        (ULONG)minorInteger,
        (ULONG)revisionInteger,
        0
        );
}

BOOLEAN SetupQueryUpdateData(
    _Inout_ PPH_SETUP_CONTEXT Context
    )
{
    BOOLEAN success = FALSE;
    PPH_HTTP_CONTEXT httpContext = NULL;
    PPH_BYTES jsonString = NULL;
    PVOID jsonObject = NULL;

    if (!PhHttpSocketCreate(&httpContext, NULL))
    {
        Context->ErrorCode = GetLastError();
        goto CleanupExit;
    }

    if (!PhHttpSocketConnect(
        httpContext,
        L"systeminformer.sourceforge.io",
        PH_HTTP_DEFAULT_HTTPS_PORT
        ))
    {
        Context->ErrorCode = GetLastError();
        goto CleanupExit;
    }

    if (!PhHttpSocketBeginRequest(
        httpContext,
        NULL,
        L"/nightly.php?update",
        PH_HTTP_FLAG_REFRESH | PH_HTTP_FLAG_SECURE
        ))
    {
        Context->ErrorCode = GetLastError();
        goto CleanupExit;
    }

    {
        PPH_STRING versionHeader;
        PPH_STRING windowsHeader;

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
    }

    if (!PhHttpSocketSendRequest(httpContext, NULL, 0))
    {
        Context->ErrorCode = GetLastError();
        goto CleanupExit;
    }

    if (!PhHttpSocketEndRequest(httpContext))
    {
        Context->ErrorCode = GetLastError();
        goto CleanupExit;
    }

    if (!(jsonString = PhHttpSocketDownloadString(httpContext, FALSE)))
    {
        Context->ErrorCode = GetLastError();
        goto CleanupExit;
    }

    if (!(jsonObject = PhCreateJsonParserEx(jsonString, FALSE)))
    {
        Context->ErrorCode = ERROR_INVALID_DATA;
        goto CleanupExit;
    }

    Context->RelVersion = PhGetJsonValueAsString(jsonObject, "version");
    Context->RelDate = PhGetJsonValueAsString(jsonObject, "updated");

    Context->BinFileDownloadUrl = PhGetJsonValueAsString(jsonObject, "bin_url");
    Context->BinFileLength = PhGetJsonValueAsInt64(jsonObject, "bin_length");
    Context->BinFileHash = PhGetJsonValueAsString(jsonObject, "bin_hash");
    Context->BinFileSignature = PhGetJsonValueAsString(jsonObject, "bin_sig");

    Context->SetupFileDownloadUrl = PhGetJsonValueAsString(jsonObject, "setup_url");
    Context->SetupFileLength = PhGetJsonValueAsInt64(jsonObject, "setup_length");
    Context->SetupFileHash = PhGetJsonValueAsString(jsonObject, "setup_hash");
    Context->SetupFileSignature = PhGetJsonValueAsString(jsonObject, "setup_sig");

    //Context->WebSetupFileDownloadUrl = PhGetJsonValueAsString(jsonObject, "websetup_url");
    //Context->WebSetupFileVersion = PhGetJsonValueAsString(jsonObject, "websetup_version");
    //Context->WebSetupFileLength = PhGetJsonValueAsString(jsonObject, "websetup_length");
    //Context->WebSetupFileHash = PhGetJsonValueAsString(jsonObject, "websetup_hash");
    //Context->WebSetupFileSignature = PhGetJsonValueAsString(jsonObject, "websetup_sig");

    if (PhIsNullOrEmptyString(Context->RelVersion))
        goto CleanupExit;
    if (PhIsNullOrEmptyString(Context->RelDate))
        goto CleanupExit;

    if (PhIsNullOrEmptyString(Context->BinFileDownloadUrl))
        goto CleanupExit;
    if (PhIsNullOrEmptyString(Context->BinFileSignature))
        goto CleanupExit;

    if (PhIsNullOrEmptyString(Context->SetupFileDownloadUrl))
        goto CleanupExit;
    if (PhIsNullOrEmptyString(Context->SetupFileSignature))
        goto CleanupExit;

    //if (PhIsNullOrEmptyString(Context->WebSetupFileDownloadUrl))
    //    goto CleanupExit;
    //if (PhIsNullOrEmptyString(Context->WebSetupFileSignature))
    //    goto CleanupExit;
    //if (PhIsNullOrEmptyString(Context->WebSetupFileVersion))
    //    goto CleanupExit;

    success = TRUE;

CleanupExit:

    if (httpContext)
        PhHttpSocketDestroy(httpContext);

    if (jsonObject)
        PhFreeJsonObject(jsonObject);

    if (jsonString)
        PhDereferenceObject(jsonString);

    return success;
}

BOOLEAN UpdateDownloadUpdateData(
    _In_ PPH_SETUP_CONTEXT Context
    )
{
    BOOLEAN downloadSuccess = FALSE;
    HANDLE tempFileHandle = NULL;
    PPH_HTTP_CONTEXT httpContext = NULL;
    PPH_STRING downloadFileName = NULL;
    PPH_STRING downloadHostPath = NULL;
    PPH_STRING downloadUrlPath = NULL;
    ULONG indexOfFileName = ULONG_MAX;
    LARGE_INTEGER timeNow;
    LARGE_INTEGER timeStart;
    ULONG64 timeTicks = 0;
    ULONG64 timeBitsPerSecond = 0;

    SendMessage(Context->DialogHandle, TDM_UPDATE_ELEMENT_TEXT, TDE_MAIN_INSTRUCTION, (LPARAM)L"Initializing download request...");

    if (!PhHttpSocketParseUrl(
        Context->BinFileDownloadUrl,
        &downloadHostPath,
        &downloadUrlPath,
        NULL
        ))
    {
        Context->ErrorCode = GetLastError();
        goto CleanupExit;
    }

    {
        PH_STRINGREF pathPart;
        PH_STRINGREF baseNamePart;

        if (!PhSplitStringRefAtLastChar(&downloadUrlPath->sr, '/', &pathPart, &baseNamePart))
            goto CleanupExit;

        downloadFileName = PhCreateString2(&baseNamePart);
    }

    Context->FilePath = PhCreateCacheFile(downloadFileName);

    if (PhIsNullOrEmptyString(Context->FilePath))
        goto CleanupExit;

    if (!NT_SUCCESS(PhCreateFileWin32Ex(
        &tempFileHandle,
        PhGetString(Context->FilePath),
        FILE_GENERIC_READ | FILE_GENERIC_WRITE,
        &(LARGE_INTEGER){ .QuadPart = Context->BinFileLength },
        FILE_ATTRIBUTE_NOT_CONTENT_INDEXED | FILE_ATTRIBUTE_TEMPORARY,
        FILE_SHARE_READ | FILE_SHARE_WRITE,
        FILE_OVERWRITE_IF,
        FILE_NON_DIRECTORY_FILE | FILE_SYNCHRONOUS_IO_NONALERT,
        NULL
        )))
    {
        goto CleanupExit;
    }

    SendMessage(Context->DialogHandle, TDM_UPDATE_ELEMENT_TEXT, TDE_MAIN_INSTRUCTION, (LPARAM)PhFormatString(
        L"Downloading System Informer %s...",
        PhGetString(Context->RelVersion)
        )->Buffer);

    //SetWindowText(Context->SubHeaderHandle, L"Progress: ~ of ~ (0.0%)");
    //SendMessage(Context->ProgressHandle, PBM_SETRANGE32, 0, (LPARAM)ExtractTotalLength);


    if (!PhHttpSocketCreate(&httpContext, NULL))
    {
        Context->ErrorCode = GetLastError();
        goto CleanupExit;
    }

    if (!PhHttpSocketConnect(
        httpContext,
        PhGetString(downloadHostPath),
        PH_HTTP_DEFAULT_HTTPS_PORT // INTERNET_SCHEME_HTTP ? INTERNET_DEFAULT_HTTP_PORT : INTERNET_DEFAULT_HTTPS_PORT
        ))
    {
        Context->ErrorCode = GetLastError();
        goto CleanupExit;
    }

    if (!PhHttpSocketBeginRequest(
        httpContext,
        NULL,
        PhGetString(downloadUrlPath),
        PH_HTTP_FLAG_REFRESH | PH_HTTP_FLAG_SECURE
        ))
    {
        Context->ErrorCode = GetLastError();
        goto CleanupExit;
    }

    {
        PPH_STRING versionHeader;
        PPH_STRING windowsHeader;

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
    }

    if (!PhHttpSocketSendRequest(httpContext, NULL, 0))
    {
        Context->ErrorCode = GetLastError();
        goto CleanupExit;
    }

    if (!PhHttpSocketEndRequest(httpContext))
    {
        Context->ErrorCode = GetLastError();
        goto CleanupExit;
    }

    {
        ULONG bytesDownloaded = 0;
        ULONG downloadedBytes = 0;
        ULONG numberOfBytesTotal = 0;
        IO_STATUS_BLOCK isb;
        BYTE buffer[PAGE_SIZE];

        //SendMessage(context->DialogHandle, TDM_SET_MARQUEE_PROGRESS_BAR, FALSE, 0);

        PhQuerySystemTime(&timeStart);

        if (!PhHttpSocketQueryHeaderUlong(httpContext, PH_HTTP_QUERY_CONTENT_LENGTH, &numberOfBytesTotal))
        {
            Context->ErrorCode = GetLastError();
            goto CleanupExit;
        }

        memset(buffer, 0, PAGE_SIZE);

        while (PhHttpSocketReadData(httpContext, buffer, PAGE_SIZE, &bytesDownloaded))
        {
            if (bytesDownloaded == 0)
                break;

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

            if (bytesDownloaded != isb.Information)
                goto CleanupExit;

            PhQuerySystemTime(&timeNow);

            timeTicks = (timeNow.QuadPart - timeStart.QuadPart) / PH_TICKS_PER_SEC;
            timeBitsPerSecond = downloadedBytes / __max(timeTicks, 1);

            {
                FLOAT percent = ((FLOAT)downloadedBytes / numberOfBytesTotal * 100);
                PH_FORMAT format[9];
                WCHAR string[MAX_PATH];

                // L"Downloaded: %s of %s (%.0f%%)\r\nSpeed: %s/s"
                PhInitFormatS(&format[0], L"Downloaded: ");
                PhInitFormatSize(&format[1], downloadedBytes);
                PhInitFormatS(&format[2], L" of ");
                PhInitFormatSize(&format[3], numberOfBytesTotal);
                PhInitFormatS(&format[4], L" (");
                PhInitFormatF(&format[5], percent, 1);
                PhInitFormatS(&format[6], L"%)\r\nSpeed: ");
                PhInitFormatSize(&format[7], timeBitsPerSecond);
                PhInitFormatS(&format[8], L"/s");

                if (PhFormatToBuffer(format, RTL_NUMBER_OF(format), string, sizeof(string), NULL))
                {
                    SendMessage(Context->DialogHandle, TDM_UPDATE_ELEMENT_TEXT, TDE_CONTENT, (LPARAM)string);
                }

                SendMessage(Context->DialogHandle, TDM_SET_PROGRESS_BAR_POS, (WPARAM)percent, 0);
            }
        }

        downloadSuccess = TRUE;
    }

CleanupExit:

    if (httpContext)
        PhHttpSocketDestroy(httpContext);

    if (tempFileHandle)
        NtClose(tempFileHandle);

    PhClearReference(&downloadHostPath);
    PhClearReference(&downloadUrlPath);
    PhClearReference(&downloadFileName);

    return downloadSuccess;
}
