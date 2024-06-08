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

#ifdef PH_RELEASE_CHANNEL_ID
#if PH_RELEASE_CHANNEL_ID == 0
#define SETUP_UPDATE_URL_PATH L"/update.php?release"
#elif PH_RELEASE_CHANNEL_ID == 1
#define SETUP_UPDATE_URL_PATH L"/update.php?preview"
#elif PH_RELEASE_CHANNEL_ID == 2
#define SETUP_UPDATE_URL_PATH L"/update.php?canary"
#elif PH_RELEASE_CHANNEL_ID == 3
#define SETUP_UPDATE_URL_PATH L"/update.php?developer"
#endif
#endif

#ifndef SETUP_UPDATE_URL_PATH
#error PH_RELEASE_CHANNEL_ID undefined
#endif

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
        SETUP_UPDATE_URL_PATH,
        PH_HTTP_FLAG_REFRESH | PH_HTTP_FLAG_SECURE
        ))
    {
        Context->ErrorCode = GetLastError();
        goto CleanupExit;
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
    PPH_HTTP_CONTEXT httpContext = NULL;
    PPH_STRING downloadFileName = NULL;
    PPH_STRING downloadHostPath = NULL;
    PPH_STRING downloadUrlPath = NULL;

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

    SendMessage(Context->DialogHandle, TDM_UPDATE_ELEMENT_TEXT, TDE_MAIN_INSTRUCTION, (LPARAM)PhFormatString(
        L"Downloading System Informer %s...",
        PhGetString(Context->RelVersion)
        )->Buffer);

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
        PVOID buffer;
        ULONG bufferLength;

        if (!PhHttpSocketReadDataToBuffer(httpContext->RequestHandle, &buffer, &bufferLength))
        {
            Context->ErrorCode = GetLastError();
            goto CleanupExit;
        }

        Context->ZipDownloadBuffer = buffer;
        Context->ZipDownloadBufferLength = bufferLength;
        downloadSuccess = TRUE;
    }

CleanupExit:

    if (httpContext)
        PhHttpSocketDestroy(httpContext);

    PhClearReference(&downloadHostPath);
    PhClearReference(&downloadUrlPath);
    PhClearReference(&downloadFileName);

    return downloadSuccess;
}
