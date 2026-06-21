/*
 * Copyright (c) 2022 Winsider Seminars & Solutions, Inc.  All rights reserved.
 *
 * This file is part of System Informer.
 *
 * Authors:
 *
 *     dmex    2011-2024
 *
 */

#include "updater.h"

#define UPDATER_PLATFORM_FILE_NTOSKRNL ((USHORT)0)
#define UPDATER_PLATFORM_FILE_NTKRLA57 ((USHORT)1)
#define UPDATER_PLATFORM_FILE_LXCORE   ((USHORT)2)

typedef struct _UPDATER_PLATFORM_SUPPORT_ENTRY
{
    USHORT Class;
    PH_STRINGREF FileName;
} UPDATER_PLATFORM_SUPPORT_ENTRY, *PUPDATER_PLATFORM_SUPPORT_ENTRY;

HWND UpdateDialogHandle = NULL;
HANDLE UpdateDialogThreadHandle = NULL;
PH_EVENT InitializedEvent = PH_EVENT_INIT;
PPH_OBJECT_TYPE UpdateContextType = NULL;
PH_INITONCE UpdateContextTypeInitOnce = PH_INITONCE_INIT;

/**
 * Deletes the updater context object.
 *
 * \param Object The updater context object to delete.
 * \param Flags Unused.
 */
_Function_class_(PH_TYPE_DELETE_PROCEDURE)
VOID UpdateContextDeleteProcedure(
    _In_ PVOID Object,
    _In_ ULONG Flags
    )
{
    PPH_UPDATER_CONTEXT context = Object;

    if (context->SetupFilePath)
    {
        if (context->Cleanup)
        {
            PhDeleteCacheFile(context->SetupFilePath, FALSE);
        }

        PhDereferenceObject(context->SetupFilePath);
    }

    if (context->SetupFileHandle)
        NtClose(context->SetupFileHandle);

    if (context->Version)
        PhDereferenceObject(context->Version);
    if (context->RelDate)
        PhDereferenceObject(context->RelDate);
    if (context->CommitHash)
        PhDereferenceObject(context->CommitHash);
    if (context->SetupFileDownloadUrl)
        PhDereferenceObject(context->SetupFileDownloadUrl);
    if (context->SetupFileLength)
        PhDereferenceObject(context->SetupFileLength);
    if (context->SetupFileHash)
        PhDereferenceObject(context->SetupFileHash);
    if (context->SetupFileSignature)
        PhDereferenceObject(context->SetupFileSignature);
}

/**
 * Creates a new updater context object.
 *
 * \param StartupCheck TRUE if this is a startup check, FALSE otherwise.
 * \return A pointer to the created updater context object.
 */
PPH_UPDATER_CONTEXT CreateUpdateContext(
    _In_ BOOLEAN StartupCheck
    )
{
    PPH_UPDATER_CONTEXT context;

    if (PhBeginInitOnce(&UpdateContextTypeInitOnce))
    {
        UpdateContextType = PhCreateObjectType(L"UpdaterContextObjectType", 0, UpdateContextDeleteProcedure);
        PhEndInitOnce(&UpdateContextTypeInitOnce);
    }

    context = PhCreateObjectZero(sizeof(PH_UPDATER_CONTEXT), UpdateContextType);
    context->StartupCheck = StartupCheck;
    context->Cleanup = TRUE;
    context->WindowDpi = USER_DEFAULT_SCREEN_DPI;
    context->PortableMode = !!SystemInformer_IsPortableMode();
    context->Channel = PhGetBuildReleaseChannel();
    context->CryptoBackend = UpdaterCryptoBackendSymCrypt;

    return context;
}


//BOOLEAN UpdaterInstalledUsingSetup(
//    VOID
//    )
//{
//    static PH_STRINGREF keyName = PH_STRINGREF_INIT(L"Software\\Microsoft\\Windows\\CurrentVersion\\Uninstall\\SystemInformer");
//    static PH_STRINGREF key2xName = PH_STRINGREF_INIT(L"Software\\Microsoft\\Windows\\CurrentVersion\\Uninstall\\Process_Hacker2_is1");
//    HANDLE keyHandle = NULL;
//
//    // Check uninstall entries for the 'ProcessHacker' registry key.
//    if (NT_SUCCESS(PhOpenKey(
//        &keyHandle,
//        KEY_READ,
//        PH_KEY_LOCAL_MACHINE,
//        &keyName,
//        0
//        )))
//    {
//        NtClose(keyHandle);
//        return TRUE;
//    }
//
//    // Check uninstall entries for the 2.x branch 'Process_Hacker2_is1' registry key.
//    if (NT_SUCCESS(PhOpenKey(
//        &keyHandle,
//        KEY_READ,
//        PH_KEY_LOCAL_MACHINE,
//        &key2xName,
//        0
//        )))
//    {
//        NtClose(keyHandle);
//        return TRUE;
//    }
//
//    return FALSE;
//}


/**
 * Gets the client ID string for the update request.
 *
 * \return A pointer to the client ID string.
 */
PPH_STRING UpdateClientIdString(
    VOID
    )
{
    static const PH_STRINGREF clientIdHeader = PH_STRINGREF_INIT(L"SystemInformer-Client-Id: ");
    PPH_STRING clientId = PhGetStringSetting(SETTING_CLIENT_ID);
    PhMoveReference(&clientId, PhConcatStringRef2(&clientIdHeader, &clientId->sr));
    return clientId;
}

/**
 * Gets platform support information for a given file.
 *
 * \param FileName The name of the file to get information for.
 * \param ImageMachine Populated with the image machine type.
 * \param TimeDateStamp Populated with the image time date stamp.
 * \param SizeOfImage Populated with the image size.
 * \param HashString Populated with the image hash string.
 * \return NTSTATUS Successful or errant status.
 */
NTSTATUS UpdatePlatformSupportInformation(
    _In_ PCPH_STRINGREF FileName,
    _Out_ PUSHORT ImageMachine,
    _Out_ PULONG TimeDateStamp,
    _Out_ PULONG SizeOfImage,
    _Out_ PPH_STRING* HashString
    )
{
    NTSTATUS status;
    HANDLE fileHandle;
    USHORT imageMachine;
    ULONG imageDateStamp;
    ULONG imageSizeOfImage;
    PPH_STRING imageHashString = NULL;
    PH_MAPPED_IMAGE mappedImage;
    LARGE_INTEGER fileSize;
    PH_HASH_CONTEXT hashContext;
    ULONG64 bytesRemaining;
    ULONG numberOfBytesRead;
    ULONG bufferLength;
    PBYTE buffer;

    status = PhCreateFile(
        &fileHandle,
        FileName,
        FILE_GENERIC_READ,
        FILE_ATTRIBUTE_NORMAL,
        FILE_SHARE_READ | FILE_SHARE_DELETE,
        FILE_OPEN,
        FILE_NON_DIRECTORY_FILE | FILE_SYNCHRONOUS_IO_NONALERT
        );

    if (!NT_SUCCESS(status))
        return status;

    status = PhGetFileSize(fileHandle, &fileSize);

    if (!NT_SUCCESS(status))
        goto CleanupExit;

    status = PhInitializeHash(&hashContext, Sha256HashAlgorithm);

    if (!NT_SUCCESS(status))
        goto CleanupExit;

    bufferLength = PAGE_SIZE * 2;
    buffer = PhAllocateSafe(bufferLength);

    if (!buffer)
    {
        status = STATUS_INSUFFICIENT_RESOURCES;
        goto CleanupExit;
    }

    bytesRemaining = (ULONG64)fileSize.QuadPart;

    while (bytesRemaining)
    {
        status = PhReadFile(
            fileHandle,
            buffer,
            bufferLength,
            NULL,
            &numberOfBytesRead
            );

        if (!NT_SUCCESS(status))
            break;

        status = PhUpdateHash(
            &hashContext,
            buffer,
            numberOfBytesRead
            );

        if (!NT_SUCCESS(status))
            break;

        bytesRemaining -= numberOfBytesRead;
    }

    PhFree(buffer);

    status = PhFinalHashString(
        &hashContext,
        &imageHashString
        );

    if (!NT_SUCCESS(status))
        goto CleanupExit;

    status = PhLoadMappedImageHeaderPageSize(
        NULL,
        fileHandle,
        &mappedImage
        );

    if (!NT_SUCCESS(status))
        goto CleanupExit;

    __try
    {
        imageMachine = mappedImage.NtHeaders->FileHeader.Machine;
        imageDateStamp = mappedImage.NtHeaders->FileHeader.TimeDateStamp;
        imageSizeOfImage = mappedImage.NtHeaders->OptionalHeader.SizeOfImage;
    }
    __except (EXCEPTION_EXECUTE_HANDLER)
    {
        status = GetExceptionCode();
    }

    PhUnloadMappedImage(&mappedImage);

    if (!NT_SUCCESS(status))
        goto CleanupExit;

    if (NT_SUCCESS(status))
    {
        *ImageMachine = imageMachine;
        *TimeDateStamp = imageDateStamp;
        *SizeOfImage = imageSizeOfImage;
        *HashString = imageHashString;

        NtClose(fileHandle);
        return STATUS_SUCCESS;
    }

CleanupExit:
    NtClose(fileHandle);
    PhClearReference(&imageHashString);
    return status;
}

/**
 * Gets the platform support string for the update request.
 *
 * \return A pointer to the platform support string.
 */
PPH_STRING UpdatePlatformSupportString(
    VOID
    )
{
    static CONST PH_STRINGREF platformHeader = PH_STRINGREF_INIT(L"SystemInformer-PlatformSupport: ");
    static CONST UPDATER_PLATFORM_SUPPORT_ENTRY platformFiles[] =
    {
        { UPDATER_PLATFORM_FILE_NTOSKRNL, PH_STRINGREF_INIT(L"\\SystemRoot\\System32\\ntoskrnl.exe") },
        { UPDATER_PLATFORM_FILE_NTKRLA57, PH_STRINGREF_INIT(L"\\SystemRoot\\System32\\ntkrla57.exe") },
        { UPDATER_PLATFORM_FILE_LXCORE,   PH_STRINGREF_INIT(L"\\SystemRoot\\System32\\drivers\\lxcore.sys") },
    };

    PH_STRING_BUILDER stringBuilder;

    PhInitializeStringBuilder(&stringBuilder, 30);
    PhAppendStringBuilder(&stringBuilder, &platformHeader);
    PhAppendStringBuilder2(&stringBuilder, L"{\"version\":1,");
    PhAppendStringBuilder2(&stringBuilder, L"\"files\":[");

    for (ULONG i = 0; i < RTL_NUMBER_OF(platformFiles); i++)
    {
        USHORT imageMachine;
        ULONG timeDateStamp;
        ULONG sizeOfImage;
        PPH_STRING hashString;

        if (NT_SUCCESS(UpdatePlatformSupportInformation(
            &platformFiles[i].FileName,
            &imageMachine,
            &timeDateStamp,
            &sizeOfImage,
            &hashString
            )))
        {
            PH_FORMAT format[11];
            PPH_STRING string;

            PhInitFormatS(&format[0], L"{\"hash\":\"");
            PhInitFormatSR(&format[1], hashString->sr);
            PhInitFormatS(&format[2], L"\",\"file\":");
            PhInitFormatU(&format[3], platformFiles[i].Class);
            PhInitFormatS(&format[4], L",\"machine\":");
            PhInitFormatU(&format[5], imageMachine);
            PhInitFormatS(&format[6], L",\"timestamp\":");
            PhInitFormatU(&format[7], timeDateStamp);
            PhInitFormatS(&format[8], L",\"size\":");
            PhInitFormatU(&format[9], sizeOfImage);
            PhInitFormatS(&format[10], L"},");

            string = PhFormat(format, RTL_NUMBER_OF(format), 10);

            PhAppendStringBuilder(&stringBuilder, &string->sr);

            PhDereferenceObject(string);
            PhDereferenceObject(hashString);
        }
    }

    if (PhEndsWithString2(stringBuilder.String, L",", FALSE))
        PhRemoveEndStringBuilder(&stringBuilder, 1);

    PhAppendStringBuilder2(&stringBuilder, L"]}");

    return PhFinalStringBuilderString(&stringBuilder);
}

//
//typedef struct _UPDATER_HTTP_CALLBACK_CONTEXT
//{
//    PPH_UPDATER_CONTEXT UpdaterContext;
//    HANDLE FileHandle;
//    PUPDATER_HASH_CONTEXT HashContext;
//    BOOLEAN DownloadSuccess;
//    BOOLEAN HashSuccess;
//    BOOLEAN SignatureSuccess;
//} UPDATER_HTTP_CALLBACK_CONTEXT, * PUPDATER_HTTP_CALLBACK_CONTEXT;
//
//static NTSTATUS NTAPI UpdateHttpEventCallback(
//    _In_ PHHTTP_EVENT_TYPE Event,
//    _In_opt_ PVOID Parameter,
//    _In_opt_ PVOID Context
//)
//{
//    PUPDATER_HTTP_CALLBACK_CONTEXT downloadContext = Context;
//    PPH_UPDATER_CONTEXT updater = downloadContext->UpdaterContext;
//
//    if (updater->Cancel)
//        return STATUS_CANCELLED;
//
//    switch (Event)
//    {
//    case PHHTTP_EVENT_INITIALIZING:
//        SendMessage(updater->DialogHandle, TDM_UPDATE_ELEMENT_TEXT, TDE_MAIN_INSTRUCTION, (LPARAM)L"Initializing download request...");
//        break;
//    case PHHTTP_EVENT_CONNECTING:
//        SendMessage(updater->DialogHandle, TDM_UPDATE_ELEMENT_TEXT, TDE_MAIN_INSTRUCTION, (LPARAM)L"Connecting...");
//        break;
//    case PHHTTP_EVENT_SENDING_REQUEST:
//        SendMessage(updater->DialogHandle, TDM_UPDATE_ELEMENT_TEXT, TDE_MAIN_INSTRUCTION, (LPARAM)L"Sending download request...");
//        break;
//    case PHHTTP_EVENT_RECEIVING_RESPONSE:
//        SendMessage(updater->DialogHandle, TDM_UPDATE_ELEMENT_TEXT, TDE_MAIN_INSTRUCTION, (LPARAM)L"Waiting for response...");
//        break;
//    }
//
//    return STATUS_SUCCESS;
//}
//
//static NTSTATUS NTAPI UpdateHttpDownloadCallback(
//    _In_ PH_HTTPDOWNLOAD_EVENT_TYPE Event,
//    _In_ PPH_HTTPDOWNLOAD_CALLBACK_CONTEXT Parameter,
//    _In_opt_ PVOID Context
//    )
//{
//    PUPDATER_HTTP_CALLBACK_CONTEXT downloadContext = Context;
//    PPH_UPDATER_CONTEXT updater = downloadContext->UpdaterContext;
//    NTSTATUS status = STATUS_SUCCESS;
//
//    if (updater->Cancel)
//        return STATUS_CANCELLED;
//
//    switch (Event)
//    {
//    case PH_HTTPDOWNLOAD_EVENT_BEGIN:
//        {
//            LARGE_INTEGER allocationSize;
//            PPH_STRING string;
//
//            string = PhFormatString(L"Downloading release %s...", PhGetStringOrEmpty(updater->Version));
//            SendMessage(updater->DialogHandle, TDM_UPDATE_ELEMENT_TEXT, TDE_MAIN_INSTRUCTION, (LPARAM)string->Buffer);
//            SendMessage(updater->DialogHandle, TDM_UPDATE_ELEMENT_TEXT, TDE_CONTENT, (LPARAM)L"Downloaded: ~ of ~ (0%)\r\nSpeed: ~ KB/s");
//            PhDereferenceObject(string);
//
//            allocationSize.QuadPart = Parameter->TotalLength;
//
//            status = PhCreateFileWin32Ex(
//                &downloadContext->FileHandle,
//                PhGetString(updater->SetupFilePath),
//                FILE_GENERIC_READ | FILE_GENERIC_WRITE,
//                Parameter->TotalLength ? &allocationSize : NULL,
//                FILE_ATTRIBUTE_NORMAL,
//                FILE_SHARE_READ,
//                FILE_OVERWRITE_IF,
//                FILE_NON_DIRECTORY_FILE | FILE_SYNCHRONOUS_IO_NONALERT,
//                NULL
//                );
//
//            if (!NT_SUCCESS(status))
//                return status;
//
//            status = UpdaterInitializeHash(
//                &downloadContext->HashContext,
//                updater->Channel
//                );
//
//            if (!NT_SUCCESS(status))
//                return status;
//        }
//        break;
//    case PH_HTTPDOWNLOAD_EVENT_DATA:
//        {
//            ULONG bytesWritten = 0;
//
//            status = UpdaterHashData(
//                downloadContext->HashContext,
//                Parameter->Buffer,
//                Parameter->BufferLength
//                );
//
//            if (!NT_SUCCESS(status))
//                return status;
//
//            status = PhWriteFile(
//                downloadContext->FileHandle,
//                Parameter->Buffer,
//                Parameter->BufferLength,
//                NULL,
//                &bytesWritten
//                );
//
//            if (!NT_SUCCESS(status))
//                return status;
//
//            if (bytesWritten != Parameter->BufferLength)
//                return STATUS_DATA_CHECKSUM_ERROR;
//        }
//        break;
//    case PH_HTTPDOWNLOAD_EVENT_PROGRESS:
//        InterlockedExchange64(&updater->ProgressTotal, Parameter->TotalLength);
//        InterlockedExchange64(&updater->ProgressDownloaded, Parameter->ReadLength);
//        InterlockedExchange64(&updater->ProgressBitsPerSecond, Parameter->BitsPerSecond);
//        break;
//    case PH_HTTPDOWNLOAD_EVENT_END:
//        {
//            if (Parameter->TotalLength && Parameter->ReadLength != Parameter->TotalLength)
//                return STATUS_DATA_ERROR;
//
//            if (NT_SUCCESS(status = UpdaterVerifyHash(downloadContext->HashContext, updater->SetupFileHash)))
//            {
//                downloadContext->HashSuccess = TRUE;
//            }
//
//            if (NT_SUCCESS(status = UpdaterVerifySignature(downloadContext->HashContext, updater->SetupFileSignature)))
//            {
//                downloadContext->SignatureSuccess = TRUE;
//            }
//
//            if (downloadContext->HashSuccess && downloadContext->SignatureSuccess)
//            {
//                HANDLE setupFileHandle;
//
//                status = PhReOpenFile(
//                    &setupFileHandle,
//                    downloadContext->FileHandle,
//                    FILE_GENERIC_READ,
//                    FILE_SHARE_READ,
//                    FILE_NON_DIRECTORY_FILE | FILE_OPEN_FOR_BACKUP_INTENT
//                    );
//
//                if (!NT_SUCCESS(status))
//                    return status;
//
//                updater->SetupFileHandle = setupFileHandle;
//                downloadContext->DownloadSuccess = TRUE;
//            }
//        }
//        break;
//    }
//
//    return status;
//}

/**
 * Queries update data from a server.
 *
 * \param Context The updater context.
 * \param ServerName The name of the server to query.
 * \param Port The port to use for the query.
 * \return TRUE if the query was successful, FALSE otherwise.
 */
BOOLEAN QueryUpdateData(
    _Inout_ PPH_UPDATER_CONTEXT Context,
    _In_ PCWSTR ServerName,
    _In_ USHORT Port
    )
{
    NTSTATUS status;
    BOOLEAN success = FALSE;
    PPH_HTTP_CONTEXT httpContext = NULL;
    PPH_BYTES jsonString = NULL;
    PVOID jsonObject;
    PWSTR urlPath;
    ULONG majorVersion;
    ULONG minorVersion;
    ULONG buildVersion;
    ULONG revisionVersion;

    status = PhHttpInitialize(&httpContext);

    if (!NT_SUCCESS(status))
        goto CleanupExit;

    status = PhHttpConnect(httpContext, ServerName, Port);

    if (!NT_SUCCESS(status))
        goto CleanupExit;

    {
        if (!Context->SwitchingChannel)
        {
            Context->Channel = PhGetBuildReleaseChannel();
        }

        switch (Context->Channel)
        {
        case PhReleaseChannel:
            urlPath = L"/update.php?channel=release";
            break;
       //case PhPreviewChannel:
       //    urlPath = L"/update.php?channel=preview";
       //    break;
        case PhCanaryChannel:
            urlPath = L"/update.php?channel=canary";
            break;
       //case PhDeveloperChannel:
       //    urlPath = L"/update.php?channel=developer";
       //    break;
        default:
            status = STATUS_PATCH_CONFLICT;
            goto CleanupExit;
        }

        if (!NT_SUCCESS(status = PhHttpBeginRequest(httpContext, NULL, urlPath, PH_HTTP_FLAG_SECURE)))
        {
            goto CleanupExit;
        }

        PhHttpSetFeature(httpContext, PH_HTTP_FEATURE_KEEP_ALIVE, FALSE);
    }

    {
        PPH_STRING versionHeader;
        PPH_STRING windowsHeader;
        PPH_STRING platformHeader;
        PPH_STRING clientIdHeader;

        if (versionHeader = UpdateVersionString())
        {
            PhHttpAddRequestHeaders(httpContext, versionHeader->Buffer, (ULONG)versionHeader->Length / sizeof(WCHAR));
            PhDereferenceObject(versionHeader);
        }

        if (windowsHeader = UpdateWindowsString())
        {
            PhHttpAddRequestHeaders(httpContext, windowsHeader->Buffer, (ULONG)windowsHeader->Length / sizeof(WCHAR));
            PhDereferenceObject(windowsHeader);
        }

        if (platformHeader = UpdatePlatformSupportString())
        {
            PhHttpAddRequestHeaders(httpContext, platformHeader->Buffer, (ULONG)platformHeader->Length / sizeof(WCHAR));
            PhDereferenceObject(platformHeader);
        }

        if (clientIdHeader = UpdateClientIdString())
        {
            PhHttpAddRequestHeaders(httpContext, clientIdHeader->Buffer, (ULONG)clientIdHeader->Length / sizeof(WCHAR));
            PhDereferenceObject(clientIdHeader);
        }
    }

    if (!NT_SUCCESS(status = PhHttpSendRequest(httpContext, PH_HTTP_NO_ADDITIONAL_HEADERS, 0, PH_HTTP_NO_REQUEST_DATA, 0, 0)))
        goto CleanupExit;
    if (!NT_SUCCESS(status = PhHttpReceiveResponse(httpContext)))
        goto CleanupExit;
    if (!NT_SUCCESS(status = PhHttpQueryResponseStatus(httpContext)))
        goto CleanupExit;
    if (!NT_SUCCESS(status = PhHttpDownloadString(httpContext, FALSE, &jsonString)))
        goto CleanupExit;
    if (!NT_SUCCESS(status = PhCreateJsonParserEx(&jsonObject, jsonString, FALSE)))
        goto CleanupExit;

    Context->Version = PhGetJsonValueAsString(jsonObject, "version");
    Context->RelDate = PhGetJsonValueAsString(jsonObject, "updated");
    Context->CommitHash = PhGetJsonValueAsString(jsonObject, "commit");
    Context->SetupFileDownloadUrl = PhGetJsonValueAsString(jsonObject, "setup_url");
    Context->SetupFileLength = PhFormatSize(PhGetJsonValueAsUInt64(jsonObject, "setup_length"), 2);
    Context->SetupFileHash = PhGetJsonValueAsString(jsonObject, "setup_hash");
    Context->SetupFileSignature = PhGetJsonValueAsString(jsonObject, "setup_sig");

    PhFreeJsonObject(jsonObject);

    if (PhIsNullOrEmptyString(Context->Version))
        goto CleanupExit;
    if (PhIsNullOrEmptyString(Context->RelDate))
        goto CleanupExit;
    if (PhIsNullOrEmptyString(Context->SetupFileDownloadUrl))
        goto CleanupExit;
    if (PhIsNullOrEmptyString(Context->SetupFileLength))
        goto CleanupExit;
    if (PhIsNullOrEmptyString(Context->SetupFileHash))
        goto CleanupExit;
    if (PhIsNullOrEmptyString(Context->SetupFileSignature))
        goto CleanupExit;
    if (PhIsNullOrEmptyString(Context->CommitHash))
        goto CleanupExit;

#if defined(FORCE_FUTURE_VERSION)
    PhGetBuildVersionNumbers(&majorVersion, &minorVersion, &buildVersion, &revisionVersion);
    Context->CurrentVersion = MAKE_VERSION_ULONGLONG(USHRT_MAX, USHRT_MAX, USHRT_MAX, USHRT_MAX);
    Context->LatestVersion = MAKE_VERSION_ULONGLONG(majorVersion, minorVersion, buildVersion, revisionVersion);
#elif defined(FORCE_LATEST_VERSION)
    PhGetBuildVersionNumbers(&majorVersion, &minorVersion, &buildVersion, &revisionVersion);
    Context->CurrentVersion = MAKE_VERSION_ULONGLONG(0, 0, 0, 0);
    Context->LatestVersion = MAKE_VERSION_ULONGLONG(majorVersion, minorVersion, buildVersion, revisionVersion);
#else
    PhGetBuildVersionNumbers(&majorVersion, &minorVersion, &buildVersion, &revisionVersion);
    Context->CurrentVersion = MAKE_VERSION_ULONGLONG(majorVersion, minorVersion, buildVersion, revisionVersion);
    Context->LatestVersion = ParseVersionString(Context->Version);
#endif

    success = TRUE;

    if (PhGetIntegerSetting(SETTING_NAME_UPDATE_MODE))
    {
        PPH_STRING jsonStringHex = PhBufferToHexString((PUCHAR)jsonString->Buffer, jsonString->Length);
        PhSetStringSetting2(SETTING_NAME_UPDATE_DATA, &jsonStringHex->sr);
        PhDereferenceObject(jsonStringHex);
    }

CleanupExit:

    if (httpContext)
        PhHttpDestroy(httpContext);
    if (jsonString)
        PhDereferenceObject(jsonString);

    return success;
}

/**
 * Queries update data from multiple servers with failover.
 *
 * \param Context The updater context.
 * \return TRUE if the query was successful, FALSE otherwise.
 */
BOOLEAN QueryUpdateDataWithFailover(
    _Inout_ PPH_UPDATER_CONTEXT Context
    )
{
    static CONST PCWSTR Servers[] =
    {
        L"system-informer.com",
        L"systeminformer.com",
        L"systeminformer.io",
        L"systeminformer.sourceforge.io",
    };
    static CONST USHORT Ports[] =
    {
        443, 8443
    };

    for (ULONG i = 0; i < ARRAYSIZE(Servers); i++)
    {
        for (ULONG j = 0; j < ARRAYSIZE(Ports); j++)
        {
            if (QueryUpdateData(Context, Servers[i], Ports[j]))
                return TRUE;
        }
    }

    return FALSE;
}

/**
 * The thread routine for a silent update check.
 *
 * \param Parameter Unused.
 * \return NTSTATUS Successful or errant status.
 */
_Function_class_(USER_THREAD_START_ROUTINE)
NTSTATUS UpdateCheckSilentThread(
    _In_ PVOID Parameter
    )
{
    PPH_UPDATER_CONTEXT context;

    context = CreateUpdateContext(TRUE);

#ifndef FORCE_UPDATE_CHECK
    if (!LastUpdateCheckExpired())
        goto CleanupExit;
#endif

    //PhDelayExecution(5 * 1000);

    PhClearCacheDirectory(!!context->PortableMode);

    // Query latest update information from the server.
    if (!QueryUpdateDataWithFailover(context))
        goto CleanupExit;

    // Compare the current version against the latest available version
    if (context->CurrentVersion < context->LatestVersion)
    {
        if (PhGetIntegerSetting(SETTING_NAME_UPDATE_MODE))
        {
            PhSetIntegerSetting(SETTING_NAME_UPDATE_AVAILABLE, TRUE);
        }
        else
        {
            if (PhGetIntegerSetting(SETTING_NAME_SHOW_NOTIFICATION))
            {
                if (!HR_SUCCESS(PhShowIconNotificationEx(
                    L"New version of System Informer available",
                    L"Help menu > Check for updates",
                    5000,
                    NULL,
                    NULL
                    )))
                {
                    // We have data we're going to cache and pass into the dialog
                    context->HaveData = TRUE;
                    ShowUpdateDialog(context);
                }
            }
            else
            {
                // We have data we're going to cache and pass into the dialog
                context->HaveData = TRUE;
                // Show the dialog asynchronously on a new thread.
                ShowUpdateDialog(context);
            }
        }
    }

CleanupExit:

    if (!context->HaveData)
        PhDereferenceObject(context);

    return STATUS_SUCCESS;
}

/**
 * The thread routine for checking updates.
 *
 * \param Parameter The updater context.
 * \return NTSTATUS Successful or errant status.
 */
_Function_class_(USER_THREAD_START_ROUTINE)
NTSTATUS UpdateCheckThread(
    _In_ PVOID Parameter
    )
{
    PPH_UPDATER_CONTEXT context;
    PH_AUTO_POOL autoPool;

    context = (PPH_UPDATER_CONTEXT)Parameter;
    context->UpdateStatus = STATUS_SUCCESS;

    PhInitializeAutoPool(&autoPool);

#if defined(PH_BUILD_MSIX)
    {
        // MSIX builds delegate check/download/install to the platform
        // (Microsoft Store / AppInstaller) rather than the HTTP pipeline.
        BOOLEAN updateAvailable = FALSE;

        if (!NT_SUCCESS(UpdaterMsixCheckForUpdates(context, &updateAvailable)))
        {
            PostMessage(context->DialogHandle, PH_SHOWERROR, FALSE, FALSE);
            goto CleanupExit;
        }

        context->HaveData = TRUE;

        if (updateAvailable)
            PostMessage(context->DialogHandle, PH_SHOWUPDATE, 0, 0);
        else
            PostMessage(context->DialogHandle, PH_SHOWLATEST, 0, 0);

        goto CleanupExit;
    }
#endif

    // Check if we have cached update data
    if (!context->HaveData)
    {
        context->HaveData = QueryUpdateDataWithFailover(context);
    }

    if (!context->HaveData)
    {
        PostMessage(context->DialogHandle, PH_SHOWERROR, FALSE, FALSE);
        goto CleanupExit;
    }

    if (context->SwitchingChannel)
    {
        // Switching channels, force the update.
        PostMessage(context->DialogHandle, PH_SHOWUPDATE, 0, 0);
    }
    else if (context->CurrentVersion == context->LatestVersion)
    {
        // User is running the latest version
        PostMessage(context->DialogHandle, PH_SHOWLATEST, 0, 0);
    }
    else if (context->CurrentVersion > context->LatestVersion)
    {
        // User is running a newer version
        PostMessage(context->DialogHandle, PH_SHOWNEWEST, 0, 0);
    }
    else
    {
        // User is running an older version
        PostMessage(context->DialogHandle, PH_SHOWUPDATE, 0, 0);
    }

CleanupExit:
    PhDereferenceObject(context);
    PhDeleteAutoPool(&autoPool);

    return STATUS_SUCCESS;
}

/**
 * Parses a download URL to get a local file name.
 *
 * \param Context The updater context.
 * \param DownloadUrlPath The download URL path.
 * \return A pointer to the local file name string.
 */
PPH_STRING UpdateParseDownloadFileName(
    _In_ PPH_UPDATER_CONTEXT Context,
    _In_ PPH_STRING DownloadUrlPath
    )
{
    PH_STRINGREF pathPart;
    PH_STRINGREF namePart;
    PPH_STRING downloadFileName;
    PPH_STRING localFileName;

    if (!PhSplitStringRefAtLastChar(&DownloadUrlPath->sr, L'/', &pathPart, &namePart))
        return NULL;

    downloadFileName = PhCreateString2(&namePart);

    if (!UpdateValidateFileName(downloadFileName))
    {
        PhDereferenceObject(downloadFileName);
        return NULL;
    }

    localFileName = PhCreateCacheFile(!!Context->PortableMode, downloadFileName, FALSE);
    PhDereferenceObject(downloadFileName);

    return localFileName;
}

/**
 * The thread routine for downloading the update.
 *
 * \param Parameter The updater context.
 * \return NTSTATUS Successful or errant status.
 */
_Function_class_(USER_THREAD_START_ROUTINE)
NTSTATUS UpdateDownloadThread(
    _In_ PVOID Parameter
    )
{
    PPH_UPDATER_CONTEXT context = (PPH_UPDATER_CONTEXT)Parameter;
    NTSTATUS status;
    BOOLEAN downloadSuccess = FALSE;
    BOOLEAN hashSuccess = FALSE;
    BOOLEAN signatureSuccess = FALSE;
    HANDLE tempFileHandle = NULL;
    PPH_HTTP_CONTEXT httpContext = NULL;
    PPH_STRING downloadHostPath = NULL;
    PPH_STRING downloadUrlPath = NULL;
    PUPDATER_HASH_CONTEXT hashContext = NULL;
    ULONG contentLength = 0;
    USHORT httpPort = 0;
    LARGE_INTEGER timeNow;
    LARGE_INTEGER timeStart;
    ULONG64 timeTicks;
    ULONG64 timeBitsPerSecond;
    LARGE_INTEGER allocationSize;
    ULONG bytesDownloaded = 0;
    ULONG bytesWritten = 0;
    ULONG_PTR totalDownloaded = 0;
    PPH_STRING string;
    PBYTE httpBuffer = NULL;
    ULONG httpBufferLength;

    SendMessage(context->DialogHandle, TDM_UPDATE_ELEMENT_TEXT, TDE_MAIN_INSTRUCTION, (LPARAM)L"Initializing download request...");

    if (!NT_SUCCESS(status = PhHttpCrackUrl(context->SetupFileDownloadUrl, &downloadHostPath, &downloadUrlPath, &httpPort)))
        goto CleanupExit;

    SendMessage(context->DialogHandle, TDM_UPDATE_ELEMENT_TEXT, TDE_MAIN_INSTRUCTION, (LPARAM)L"Connecting...");

    if (!NT_SUCCESS(status = PhHttpInitialize(&httpContext)))
        goto CleanupExit;

    PhHttpSetProtocol(httpContext, TRUE, PH_HTTP_PROTOCOL_FLAG_HTTP2, 5000);

    if (!NT_SUCCESS(status = PhHttpConnect(httpContext, PhGetString(downloadHostPath), httpPort)))
        goto CleanupExit;
    if (!NT_SUCCESS(status = PhHttpBeginRequest(httpContext, NULL, PhGetString(downloadUrlPath), PH_HTTP_FLAG_SECURE)))
        goto CleanupExit;

    PhHttpSetFeature(httpContext, PH_HTTP_FEATURE_KEEP_ALIVE, FALSE);

    SendMessage(context->DialogHandle, TDM_UPDATE_ELEMENT_TEXT, TDE_MAIN_INSTRUCTION, (LPARAM)L"Sending download request...");

    if (!NT_SUCCESS(status = PhHttpSendRequest(httpContext, PH_HTTP_NO_ADDITIONAL_HEADERS, 0, PH_HTTP_NO_REQUEST_DATA, 0, 0)))
        goto CleanupExit;

    SendMessage(context->DialogHandle, TDM_UPDATE_ELEMENT_TEXT, TDE_MAIN_INSTRUCTION, (LPARAM)L"Waiting for response...");

    if (!NT_SUCCESS(status = PhHttpReceiveResponse(httpContext)))
        goto CleanupExit;
    if (!NT_SUCCESS(status = PhHttpQueryResponseStatus(httpContext)))
        goto CleanupExit;
    if (!NT_SUCCESS(status = PhHttpQueryHeaderUlong(httpContext, PH_HTTP_QUERY_CONTENT_LENGTH, &contentLength)))
        goto CleanupExit;

    httpBufferLength = PAGE_SIZE * 2;
    httpBuffer = PhAllocateSafe(httpBufferLength);

    if (!httpBuffer)
    {
        status = STATUS_NO_MEMORY;
        goto CleanupExit;
    }

    string = PhFormatString(L"Downloading release %s...", PhGetStringOrEmpty(context->Version));
    SendMessage(context->DialogHandle, TDM_UPDATE_ELEMENT_TEXT, TDE_MAIN_INSTRUCTION, (LPARAM)string->Buffer);
    SendMessage(context->DialogHandle, TDM_UPDATE_ELEMENT_TEXT, TDE_CONTENT, (LPARAM)L"Downloaded: ~ of ~ (0%)\r\nSpeed: ~ KB/s");
    PhDereferenceObject(string);

    // Create temporary file.
    {
        context->SetupFilePath = UpdateParseDownloadFileName(context, downloadUrlPath);

        if (PhIsNullOrEmptyString(context->SetupFilePath))
        {
            status = STATUS_FILE_HANDLE_REVOKED;
            goto CleanupExit;
        }

        allocationSize.QuadPart = contentLength;

        status = PhCreateFileWin32Ex(
            &tempFileHandle,
            PhGetString(context->SetupFilePath),
            FILE_GENERIC_WRITE,
            &allocationSize,
            FILE_ATTRIBUTE_NORMAL,
            FILE_SHARE_READ,
            FILE_OVERWRITE_IF,
            FILE_NON_DIRECTORY_FILE | FILE_SYNCHRONOUS_IO_NONALERT,
            NULL
            );

        if (!NT_SUCCESS(status))
            goto CleanupExit;
    }

    // Initialize hash algorithm.
    status = UpdaterInitializeHashForContext(&hashContext, context);

    if (!NT_SUCCESS(status))
        goto CleanupExit;

    // Start the clock.
    PhQuerySystemTime(&timeStart);

    while (TRUE)
    {
        // Download the data.
        status = PhHttpReadData(httpContext, httpBuffer, httpBufferLength, &bytesDownloaded);

        if (!NT_SUCCESS(status))
            goto CleanupExit;

        // If we get zero bytes, the file was downloaded or there was an error.
        if (bytesDownloaded == 0)
            break;

        // Update was cancelled, cleanup and exit.
        if (context->Cancel)
            goto CleanupExit;

        // Update the hash of bytes we downloaded.
        status = UpdaterHashDataForContext(hashContext, httpBuffer, bytesDownloaded);

        if (!NT_SUCCESS(status))
            goto CleanupExit;

        // Write the downloaded bytes to disk.
        status = PhWriteFile(
            tempFileHandle,
            httpBuffer,
            bytesDownloaded,
            NULL,
            &bytesWritten
            );

        if (!NT_SUCCESS(status))
            goto CleanupExit;

        // Check the number of bytes written are the same we downloaded.
        if (bytesDownloaded != bytesWritten)
        {
            status = STATUS_DATA_CHECKSUM_ERROR;
            goto CleanupExit;
        }

#ifdef FORCE_SLOW_STATUS_TIMER
        PhDelayExecution(1);
#endif
        // Query the current time
        PhQuerySystemTime(&timeNow);

        // Calculate the number of ticks
        totalDownloaded += bytesWritten;
        timeTicks = (timeNow.QuadPart - timeStart.QuadPart) / PH_TICKS_PER_SEC;
        timeBitsPerSecond = timeTicks ? totalDownloaded / timeTicks : 0;

#ifdef FORCE_NO_STATUS_TIMER
        LONG percent = PhMultiplyDivide((LONG64)totalDownloaded, 100, (LONG64)contentLength);
        PH_FORMAT format[9];
        WCHAR stringformat[MAX_PATH];

        // L"Downloaded: %s / %s (%.0f%%)\r\nSpeed: %s/s"
        PhInitFormatS(&format[0], L"Downloaded: ");
        PhInitFormatSize(&format[1], totalDownloaded);
        PhInitFormatS(&format[2], L" of ");
        PhInitFormatSize(&format[3], contentLength);
        PhInitFormatS(&format[4], L" (");
        PhInitFormatU(&format[5], percent);
        PhInitFormatS(&format[6], L"%)\r\nSpeed: ");
        PhInitFormatSize(&format[7], timeBitsPerSecond);
        PhInitFormatS(&format[8], L"/s");

        if (PhFormatToBuffer(format, RTL_NUMBER_OF(format), stringformat, sizeof(stringformat), NULL))
        {
            SendMessage(context->DialogHandle, TDM_UPDATE_ELEMENT_TEXT, TDE_CONTENT, (LPARAM)stringformat);
        }
        if (context->ProgressMarquee)
        {
            SendMessage(context->DialogHandle, TDM_SET_MARQUEE_PROGRESS_BAR, FALSE, 0);
            context->ProgressMarquee = FALSE;
        }

        SendMessage(context->DialogHandle, TDM_SET_PROGRESS_BAR_POS, (WPARAM)percent, 0);
#else
        InterlockedExchange64(&context->ProgressTotal, contentLength);
        InterlockedExchange64(&context->ProgressDownloaded, totalDownloaded);
        InterlockedExchange64(&context->ProgressBitsPerSecond, timeBitsPerSecond);
#endif
#ifdef FORCE_SLOW_STATUS_TIMER
        PhDelayExecution(1);
#endif
    }

    if (NT_SUCCESS(status = UpdaterVerifyHashForContext(hashContext, context->SetupFileHash)))
    {
        hashSuccess = TRUE;

        if (NT_SUCCESS(status = UpdaterVerifySignatureForContext(hashContext, context->SetupFileSignature)))
        {
            signatureSuccess = TRUE;
        }
    }

    if (hashSuccess && signatureSuccess)
    {
        downloadSuccess = TRUE;
    }
    else
    {
        status = STATUS_DATA_CHECKSUM_ERROR;
    }

CleanupExit:
    context->UpdateStatus = status;

    if (httpContext)
        PhHttpDestroy(httpContext);
    if (hashContext)
        UpdaterDestroyHashForContext(hashContext);
    if (tempFileHandle)
        NtClose(tempFileHandle);
    if (downloadHostPath)
        PhDereferenceObject(downloadHostPath);
    if (downloadUrlPath)
        PhDereferenceObject(downloadUrlPath);
    if (httpBuffer)
        PhFree(httpBuffer);

    if (downloadSuccess && hashSuccess && signatureSuccess)
    {
        PostMessage(context->DialogHandle, PH_SHOWINSTALL, 0, 0);
    }
    else
    {
        if (context->SetupFilePath)
        {
            PhDeleteCacheFile(context->SetupFilePath, FALSE);
        }

        if (signatureSuccess)
            PostMessage(context->DialogHandle, PH_SHOWERROR, TRUE, FALSE);
        else if (hashSuccess)
            PostMessage(context->DialogHandle, PH_SHOWERROR, FALSE, TRUE);
        else
            PostMessage(context->DialogHandle, PH_SHOWERROR, FALSE, FALSE);
    }

    PhDereferenceObject(context);
    return STATUS_SUCCESS;
}

/**
 * The subclass procedure for the task dialog.
 *
 * \param WindowHandle The handle to the task dialog.
 * \param WindowMessage The window message.
 * \param wParam The first message parameter.
 * \param lParam The second message parameter.
 * \return LRESULT The result of message processing.
 */
LRESULT CALLBACK TaskDialogSubclassProc(
    _In_ HWND WindowHandle,
    _In_ UINT WindowMessage,
    _In_ WPARAM wParam,
    _In_ LPARAM lParam
    )
{
    PPH_UPDATER_CONTEXT context;
    WNDPROC oldWndProc;

    if (!(context = PhGetWindowContext(WindowHandle, UCHAR_MAX)))
        return 0;

    oldWndProc = context->DefaultWindowProc;

    switch (WindowMessage)
    {
    case WM_DESTROY:
        {
            context->Cancel = TRUE;

            PhSetWindowProcedure(WindowHandle, oldWndProc);
            PhRemoveWindowContext(WindowHandle, UCHAR_MAX);

            PhUnregisterWindowCallback(WindowHandle);
        }
        break;
    case PH_SHOWDIALOG:
        {
            if (IsMinimized(WindowHandle))
                ShowWindow(WindowHandle, SW_RESTORE);
            else
                ShowWindow(WindowHandle, SW_SHOW);

            SetForegroundWindow(WindowHandle);
        }
        break;
    case PH_SHOWLATEST:
        {
            ShowLatestVersionDialog(context);
        }
        break;
    case PH_SHOWNEWEST:
        {
            ShowNewerVersionDialog(context);
        }
        break;
    case PH_SHOWUPDATE:
        {
            ShowAvailableDialog(context);
        }
        break;
    case PH_SHOWINSTALL:
        {
            ShowUpdateInstallDialog(context);
        }
        break;
    case PH_SHOWERROR:
        {
            ShowUpdateFailedDialog(context, (BOOLEAN)wParam, (BOOLEAN)lParam);
        }
        break;
    case WM_TIMER:
        {
            if (wParam == 9000)
            {
                if (context->ProgressDownloaded && context->ProgressTotal)
                {
                    LONG64 percent = (context->ProgressDownloaded * 100) / context->ProgressTotal;
                    PH_FORMAT format[9];
                    WCHAR string[MAX_PATH];

                    // L"Downloaded: %s / %s (%.0f%%)\r\nSpeed: %s/s"
                    PhInitFormatS(&format[0], L"Downloaded: ");
                    PhInitFormatSize(&format[1], context->ProgressDownloaded);
                    PhInitFormatS(&format[2], L" of ");
                    PhInitFormatSize(&format[3], context->ProgressTotal);
                    PhInitFormatS(&format[4], L" (");
                    PhInitFormatI64U(&format[5], percent);
                    PhInitFormatS(&format[6], L"%)\r\nSpeed: ");
                    PhInitFormatSize(&format[7], context->ProgressBitsPerSecond);
                    PhInitFormatS(&format[8], L"/s");

                    if (PhFormatToBuffer(format, RTL_NUMBER_OF(format), string, sizeof(string), NULL))
                    {
                        SendMessage(context->DialogHandle, TDM_UPDATE_ELEMENT_TEXT, TDE_CONTENT, (LPARAM)string);
                    }

                    if (context->ProgressMarquee)
                    {
                        SendMessage(context->DialogHandle, TDM_SET_MARQUEE_PROGRESS_BAR, FALSE, 0);
                        context->ProgressMarquee = FALSE;
                    }

                    SendMessage(context->DialogHandle, TDM_SET_PROGRESS_BAR_POS, (WPARAM)percent, 0);
                }
                return 0;
            }
        }
        break;
    case WM_DPICHANGED:
        {
            LONG windowDpi = HIWORD(wParam);

            PhSetApplicationWindowIconEx(WindowHandle, windowDpi);
        }
        break;
    //case WM_PARENTNOTIFY:
    //    {
    //        if (wParam == WM_CREATE)
    //        {
    //            // uMsg == 49251 for expand/collapse button click
    //            HWND hwndEdit = CreateWindowEx(
    //                WS_EX_CLIENTEDGE,
    //                L"EDIT",
    //                NULL,
    //                WS_CHILD | WS_VISIBLE | WS_VSCROLL | ES_LEFT | ES_MULTILINE | ES_AUTOVSCROLL,
    //                5,
    //                5,
    //                390,
    //                85,
    //                (HWND)lParam, // parent window
    //                0,
    //                NULL,
    //                NULL
    //            );
    //
    //            PhCreateCommonFont(-11, hwndEdit);
    //
    //            // Add text to the window.
    //            SendMessage(hwndEdit, WM_SETTEXT, 0, (LPARAM)L"TEST");
    //        }
    //    }
    //    break;
    //case WM_NCACTIVATE:
    //    {
    //        if (IsWindowVisible(PhMainWindowHandle) && !IsMinimized(PhMainWindowHandle))
    //        {
    //            if (!context->FixedWindowStyles)
    //            {
    //                SetWindowLongPtr(hwndDlg, GWLP_HWNDPARENT, (LONG_PTR)PhMainWindowHandle);
    //                PhSetWindowExStyle(hwndDlg, WS_EX_APPWINDOW, WS_EX_APPWINDOW);
    //                context->FixedWindowStyles = TRUE;
    //            }
    //        }
    //    }
    //    break;
    }

    return CallWindowProc(oldWndProc, WindowHandle, WindowMessage, wParam, lParam);
}

/**
 * The callback for task dialog bootstrap.
 *
 * \param WindowHandle The handle to the task dialog.
 * \param WindowMessage The window message.
 * \param wParam The first message parameter.
 * \param lParam The second message parameter.
 * \param dwRefData The reference data.
 * \return HRESULT Successful or errant status.
 */
HRESULT CALLBACK TaskDialogBootstrapCallback(
    _In_ HWND WindowHandle,
    _In_ UINT WindowMessage,
    _In_ WPARAM wParam,
    _In_ LPARAM lParam,
    _In_ LONG_PTR dwRefData
    )
{
    PPH_UPDATER_CONTEXT context = (PPH_UPDATER_CONTEXT)dwRefData;

    switch (WindowMessage)
    {
    case TDN_DIALOG_CONSTRUCTED:
        {
            UpdateDialogHandle = context->DialogHandle = WindowHandle;

            PhSetApplicationWindowIcon(WindowHandle);

            // Center the update window on PH if it's visible else we center on the desktop.
            PhCenterWindow(WindowHandle, SystemInformer_GetWindowHandle());

            PhRegisterWindowCallback(WindowHandle, PH_PLUGIN_WINDOW_EVENT_TYPE_TOPMOST, NULL);

            // Subclass the Taskdialog.
            context->DefaultWindowProc = PhGetWindowProcedure(WindowHandle);
            PhSetWindowContext(WindowHandle, UCHAR_MAX, context);
            PhSetWindowProcedure(WindowHandle, TaskDialogSubclassProc);

            if (context->StartupCheck)
            {
                ShowAvailableDialog(context);
            }
            else
            {
                if (PhGetIntegerSetting(SETTING_NAME_AUTO_CHECK_PAGE))
                {
                    ShowCheckingForUpdatesDialog(context);
                }
                else
                {
                    ShowCheckForUpdatesDialog(context);
                }
            }
        }
        break;
    }

    return S_OK;
}

/**
 * The thread routine for showing the update dialog.
 *
 * \param Parameter The updater context.
 * \return NTSTATUS Successful or errant status.
 */
_Function_class_(USER_THREAD_START_ROUTINE)
NTSTATUS ShowUpdateDialogThread(
    _In_ PVOID Parameter
    )
{
    PH_AUTO_POOL autoPool;
    PPH_UPDATER_CONTEXT context;
    TASKDIALOGCONFIG config = { sizeof(TASKDIALOGCONFIG) };

    if (Parameter)
        context = (PPH_UPDATER_CONTEXT)Parameter;
    else
        context = CreateUpdateContext(FALSE);

    PhInitializeAutoPool(&autoPool);

    // Start TaskDialog bootstrap
    config.dwFlags = TDF_ALLOW_DIALOG_CANCELLATION | TDF_CAN_BE_MINIMIZED;
    config.hInstance = NtCurrentImageBase();
    config.pszContent = L"Initializing...";
    config.lpCallbackData = (LONG_PTR)context;
    config.pfCallback = TaskDialogBootstrapCallback;
    PhShowTaskDialog(&config, NULL, NULL, NULL);

    PhDereferenceObject(context);
    PhDeleteAutoPool(&autoPool);

    if (UpdateDialogThreadHandle)
    {
        NtClose(UpdateDialogThreadHandle);
        UpdateDialogThreadHandle = NULL;
    }

    PhResetEvent(&InitializedEvent);

    return STATUS_SUCCESS;
}

/**
 * Shows the update dialog.
 *
 * \param Context The updater context.
 */
VOID ShowUpdateDialog(
    _In_opt_ PPH_UPDATER_CONTEXT Context
    )
{
    if (Context && Context->HaveData && PhGetIntegerSetting(SETTING_NAME_TOAST_NOTIFICATIONS))
    {
        if (UpdaterShowAvailableToast(Context))
        {
            PhDereferenceObject(Context);
            return;
        }
    }

    if (!UpdateDialogThreadHandle)
    {
        if (!NT_SUCCESS(PhCreateThreadEx(&UpdateDialogThreadHandle, ShowUpdateDialogThread, Context)))
        {
            PhShowError2(NULL, L"Unable to create the window.", L"%s", L"");
            return;
        }

        PhWaitForEvent(&InitializedEvent, NULL);
    }

    PostMessage(UpdateDialogHandle, PH_SHOWDIALOG, 0, 0);
}

/**
 * Starts the initial update check.
 */
VOID StartInitialCheck(
    VOID
    )
{
    PhQueueItemWorkQueue(PhGetGlobalWorkQueue(), UpdateCheckSilentThread, NULL);
}

/**
 * Shows the update dialog at startup if data is cached.
 *
 * \param CacheString The cached update data.
 */
VOID ShowStartupUpdateDialog(
    _In_ PPH_STRING CacheString
    )
{
    PH_AUTO_POOL autoPool;
    PPH_UPDATER_CONTEXT context;
    PPH_BYTES jsonString;

    if (PhIsNullOrEmptyString(CacheString))
        return;

    PhInitializeAutoPool(&autoPool);

    context = CreateUpdateContext(TRUE);

    jsonString = PhCreateBytesEx(
        NULL,
        CacheString->Length / sizeof(WCHAR) / 2
        );

    if (PhHexStringToBufferEx(
        &CacheString->sr,
        jsonString->Length,
        jsonString->Buffer
        ))
    {
        PVOID jsonObject;

        if (NT_SUCCESS(PhCreateJsonParserEx(&jsonObject, jsonString, TRUE)))
        {
            ULONG majorVersion;
            ULONG minorVersion;
            ULONG buildVersion;
            ULONG revisionVersion;

            context->Version = PhGetJsonValueAsString(jsonObject, "version");
            context->RelDate = PhGetJsonValueAsString(jsonObject, "updated");
            context->CommitHash = PhGetJsonValueAsString(jsonObject, "commit");
            context->SetupFileDownloadUrl = PhGetJsonValueAsString(jsonObject, "setup_url");
            context->SetupFileLength = PhFormatSize(PhGetJsonValueAsUInt64(jsonObject, "setup_length"), 2);
            context->SetupFileHash = PhGetJsonValueAsString(jsonObject, "setup_hash");
            context->SetupFileSignature = PhGetJsonValueAsString(jsonObject, "setup_sig");

            PhGetBuildVersionNumbers(&majorVersion, &minorVersion, &buildVersion, &revisionVersion);
#ifdef FORCE_LATEST_VERSION
            context->LatestVersion = MAKE_VERSION_ULONGLONG(majorVersion, minorVersion, buildVersion, revisionVersion);
            context->CurrentVersion = MAKE_VERSION_ULONGLONG(majorVersion, minorVersion, buildVersion, revisionVersion);
#else
            context->LatestVersion = ParseVersionString(context->Version);
            context->CurrentVersion = MAKE_VERSION_ULONGLONG(majorVersion, minorVersion, buildVersion, revisionVersion);
#endif
            PhFreeJsonObject(jsonObject);
        }
    }

    PhClearReference(&jsonString);

    if (PhIsNullOrEmptyString(context->Version) ||
        PhIsNullOrEmptyString(context->RelDate) ||
        PhIsNullOrEmptyString(context->SetupFileDownloadUrl) ||
        PhIsNullOrEmptyString(context->SetupFileLength) ||
        PhIsNullOrEmptyString(context->SetupFileHash) ||
        PhIsNullOrEmptyString(context->SetupFileSignature) ||
        PhIsNullOrEmptyString(context->CommitHash))
    {
        goto CleanupExit;
    }

    if (PhGetIntegerSetting(SETTING_NAME_SHOW_NOTIFICATION))
    {
        if (HR_SUCCESS(PhShowIconNotificationEx(
            L"New version of System Informer available",
            L"Help menu > Check for updates",
            5000,
            NULL,
            NULL
            )))
        {
            goto CleanupExit;
        }
    }

    if (PhGetIntegerSetting(SETTING_NAME_TOAST_NOTIFICATIONS))
    {
        if (UpdaterShowAvailableToast(context))
            goto CleanupExit;
    }

    {
        TASKDIALOGCONFIG config = { sizeof(TASKDIALOGCONFIG) };
        config.dwFlags = TDF_ALLOW_DIALOG_CANCELLATION | TDF_CAN_BE_MINIMIZED;
        config.hInstance = NtCurrentImageBase();
        config.pszContent = L"Initializing...";
        config.lpCallbackData = (LONG_PTR)context;
        config.pfCallback = TaskDialogBootstrapCallback;
        PhShowTaskDialog(&config, NULL, NULL, NULL);
    }

CleanupExit:
    PhDereferenceObject(context);
    PhDeleteAutoPool(&autoPool);
}
