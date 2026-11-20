/*
 * Copyright (c) 2022 Winsider Seminars & Solutions, Inc.  All rights reserved.
 */

#include "setup.h"
#include "..\thirdparty\miniz\miniz.h"

#define SETUP_RELEASE_HOST L"github.com"
#define SETUP_RELEASE_PATH L"/winsiderss/systeminformer/releases/latest/download/systeminformer-build-bin.zip"

ULONG64 ParseVersionString(
    _In_ PPH_STRING VersionString
    )
{
    PH_STRINGREF remaining;
    PH_STRINGREF majorPart;
    PH_STRINGREF minorPart;
    PH_STRINGREF buildPart;
    PH_STRINGREF revisionPart;
    ULONG64 majorInteger = 0;
    ULONG64 minorInteger = 0;
    ULONG64 buildInteger = 0;
    ULONG64 revisionInteger = 0;

    if (PhIsNullOrEmptyString(VersionString))
        return 0;

    remaining = PhGetStringRef(VersionString);
    PhSplitStringRefAtChar(&remaining, L'.', &majorPart, &remaining);
    PhSplitStringRefAtChar(&remaining, L'.', &minorPart, &remaining);
    PhSplitStringRefAtChar(&remaining, L'.', &buildPart, &remaining);
    PhSplitStringRefAtChar(&remaining, L'.', &revisionPart, &remaining);

    if (majorPart.Length)
        PhStringToUInt64(&majorPart, 10, &majorInteger);
    if (minorPart.Length)
        PhStringToUInt64(&minorPart, 10, &minorInteger);
    if (buildPart.Length)
        PhStringToUInt64(&buildPart, 10, &buildInteger);
    if (revisionPart.Length)
        PhStringToUInt64(&revisionPart, 10, &revisionInteger);

    return MAKE_VERSION_ULONGLONG(
        (ULONG)majorInteger,
        (ULONG)minorInteger,
        (ULONG)buildInteger,
        (ULONG)revisionInteger
        );
}

static NTSTATUS SetupDownloadFile(
    _In_ PPH_SETUP_CONTEXT Context,
    _In_ PCWSTR Path
    )
{
    NTSTATUS status = STATUS_UNSUCCESSFUL;
    PPH_HTTP_CONTEXT httpContext = NULL;
    PPH_STRING cacheName = NULL;
    PPH_STRING cacheFilePath = NULL;
    HANDLE fileHandle = NULL;
    LARGE_INTEGER allocationSize;
    ULONG bytesRead;
    ULONGLONG contentLength;
    ULONGLONG totalLength = 0;
    BYTE buffer[64 * 1024];

    cacheName = PhCreateString(L"systeminformer-build-bin.zip");
    cacheFilePath = PhCreateCacheFile(FALSE, cacheName, FALSE);
    if (!cacheFilePath)
    {
        status = STATUS_OBJECT_PATH_NOT_FOUND;
        goto CleanupExit;
    }

    if (!NT_SUCCESS(status = PhHttpInitialize(&httpContext)))
        goto CleanupExit;
    if (!NT_SUCCESS(status = PhHttpConnect(httpContext, SETUP_RELEASE_HOST, PH_HTTP_DEFAULT_HTTPS_PORT)))
        goto CleanupExit;
    if (!NT_SUCCESS(status = PhHttpBeginRequest(httpContext, L"GET", Path, PH_HTTP_FLAG_SECURE)))
        goto CleanupExit;
    if (!NT_SUCCESS(status = PhHttpSendRequest(httpContext, PH_HTTP_NO_ADDITIONAL_HEADERS, 0, PH_HTTP_NO_REQUEST_DATA, 0, 0)))
        goto CleanupExit;
    if (!NT_SUCCESS(status = PhHttpReceiveResponse(httpContext)))
        goto CleanupExit;
    if (!NT_SUCCESS(status = PhHttpQueryResponseStatus(httpContext)))
        goto CleanupExit;
    if (!NT_SUCCESS(status = PhHttpQueryHeaderUlong64(httpContext, PH_HTTP_QUERY_CONTENT_LENGTH, &contentLength)))
        goto CleanupExit;
    if (!contentLength || contentLength > SETUP_MAX_DOWNLOAD_SIZE)
    {
        status = STATUS_FILE_TOO_LARGE;
        goto CleanupExit;
    }

    allocationSize.QuadPart = contentLength;
    status = PhCreateFileWin32Ex(
        &fileHandle,
        PhGetString(cacheFilePath),
        FILE_GENERIC_WRITE,
        &allocationSize,
        FILE_ATTRIBUTE_TEMPORARY,
        FILE_SHARE_READ,
        FILE_OVERWRITE_IF,
        FILE_NON_DIRECTORY_FILE | FILE_SYNCHRONOUS_IO_NONALERT,
        NULL
        );
    if (!NT_SUCCESS(status))
        goto CleanupExit;

    SetupSetProgressMarquee(Context, TRUE);
    while (NT_SUCCESS(status = PhHttpReadData(httpContext, buffer, sizeof(buffer), &bytesRead)) && bytesRead)
    {
        ULONG bytesWritten;

        totalLength += bytesRead;
        if (totalLength > SETUP_MAX_DOWNLOAD_SIZE)
        {
            status = STATUS_FILE_TOO_LARGE;
            goto CleanupExit;
        }

        if (!WriteFile(fileHandle, buffer, bytesRead, &bytesWritten, NULL) || bytesWritten != bytesRead)
        {
            status = PhGetLastWin32ErrorAsNtStatus();
            goto CleanupExit;
        }
    }

    if (!NT_SUCCESS(status))
        goto CleanupExit;
    if (totalLength != contentLength)
    {
        status = STATUS_DATA_CHECKSUM_ERROR;
        goto CleanupExit;
    }

    NtClose(fileHandle);
    fileHandle = NULL;
    Context->SetupBuildZipPath = PhReferenceObject(cacheFilePath);
    status = STATUS_SUCCESS;

CleanupExit:
    SetupSetProgressMarquee(Context, FALSE);
    if (httpContext) PhHttpDestroy(httpContext);
    if (fileHandle) NtClose(fileHandle);
    if (!NT_SUCCESS(status) && cacheFilePath) PhDeleteCacheFile(cacheFilePath, FALSE);
    if (cacheFilePath) PhDereferenceObject(cacheFilePath);
    if (cacheName) PhDereferenceObject(cacheName);
    return status;
}

NTSTATUS SetupDownloadBuildZip(
    _Inout_ PPH_SETUP_CONTEXT Context
    )
{
    SetupDeleteBuildZip(Context);
    return SetupDownloadFile(Context, SETUP_RELEASE_PATH);
}

VOID SetupDeleteBuildZip(
    _Inout_ PPH_SETUP_CONTEXT Context
    )
{
    if (Context->SetupBuildZipPath)
    {
        PhDeleteCacheFile(Context->SetupBuildZipPath, FALSE);
        PhClearReference(&Context->SetupBuildZipPath);
    }
}
