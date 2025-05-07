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

#include <kphdyn.h>

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
    context->PortableMode = !!SystemInformer_IsPortableMode();
    context->Channel = PhGetPhReleaseChannel();

    return context;
}

NTSTATUS UpdateShellExecute(
    _In_ PPH_UPDATER_CONTEXT Context,
    _In_opt_ HWND WindowHandle
    )
{
    NTSTATUS status;
    PPH_STRING parameters;

    // Reset the cache so we don't prompt again after the update.
    PhSetStringSetting(SETTING_NAME_UPDATE_DATA, L"");

    if (PhIsNullOrEmptyString(Context->SetupFilePath))
        return STATUS_FAIL_CHECK;

    parameters = PH_AUTO(PhCreateKsiSettingsBlob());
    parameters = PH_AUTO(PhConcatStrings(3, L"-update \"", PhGetStringOrEmpty(parameters), L"\""));

    SystemInformer_PrepareForEarlyShutdown();

    status = PhShellExecuteEx(
        WindowHandle,
        PhGetString(Context->SetupFilePath),
        PhGetString(parameters),
        NULL,
        SW_SHOW,
        Context->ElevationRequired ? PH_SHELL_EXECUTE_ADMIN : PH_SHELL_EXECUTE_DEFAULT,
        0,
        NULL
        );

    if (NT_SUCCESS(status))
    {
        Context->Cleanup = FALSE;

        SystemInformer_Destroy();
    }
    else
    {
        SystemInformer_CancelEarlyShutdown();

        if (status != STATUS_CANCELLED) // Ignore UAC decline.
        {
            PhShowStatus(WindowHandle, L"Unable to execute the setup.", status, 0);

            if (Context->StartupCheck)
                ShowAvailableDialog(Context);
            else
                ShowCheckForUpdatesDialog(Context);
        }
    }

    return status;
}

BOOLEAN UpdateCheckDirectoryElevationRequired(
    VOID
    )
{
    static PH_STRINGREF checkFileName = PH_STRINGREF_INIT(L"elevation_check");
    HANDLE fileHandle;
    PPH_STRING fileName;

    fileName = PhGetApplicationDirectoryFileName(&checkFileName, TRUE);

    if (PhIsNullOrEmptyString(fileName))
        return TRUE;

    if (NT_SUCCESS(PhCreateFile(
        &fileHandle,
        &fileName->sr,
        FILE_GENERIC_WRITE | DELETE,
        FILE_ATTRIBUTE_NORMAL,
        FILE_SHARE_READ | FILE_SHARE_DELETE,
        FILE_OPEN_IF,
        FILE_NON_DIRECTORY_FILE | FILE_SYNCHRONOUS_IO_NONALERT | FILE_DELETE_ON_CLOSE
        )))
    {
        PhDereferenceObject(fileName);
        NtClose(fileHandle);
        return FALSE;
    }

    PhDereferenceObject(fileName);
    return TRUE;
}

VOID TaskDialogLinkClicked(
    _In_ PPH_UPDATER_CONTEXT Context
    )
{
    PhDialogBox(
        PluginInstance->DllBase,
        MAKEINTRESOURCE(IDD_TEXT),
        Context->DialogHandle,
        TextDlgProc,
        Context
        );
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

BOOLEAN LastUpdateCheckExpired(
    VOID
    )
{
    ULONG lastTimeUpdateSeconds;
    LARGE_INTEGER lastTimeUpdateTicks;
    LARGE_INTEGER currentTimeUpdateTicks;

    PhQuerySystemTime(&currentTimeUpdateTicks);
    lastTimeUpdateSeconds = PhGetIntegerSetting(SETTING_NAME_LAST_CHECK);

    if (lastTimeUpdateSeconds == 0)
    {
        PhTimeToSecondsSince1970(&currentTimeUpdateTicks, &lastTimeUpdateSeconds);
        PhSetIntegerSetting(SETTING_NAME_LAST_CHECK, lastTimeUpdateSeconds);
        return TRUE;
    }

    PhSecondsSince1970ToTime(lastTimeUpdateSeconds, &lastTimeUpdateTicks);

    if (currentTimeUpdateTicks.QuadPart - lastTimeUpdateTicks.QuadPart >= 7 * PH_TICKS_PER_DAY)
    {
        PhTimeToSecondsSince1970(&currentTimeUpdateTicks, &lastTimeUpdateSeconds);
        PhSetIntegerSetting(SETTING_NAME_LAST_CHECK, lastTimeUpdateSeconds);
        return TRUE;
    }

    return FALSE;
}

PPH_STRING UpdateVersionString(
    VOID
    )
{
    static PH_STRINGREF versionHeader = PH_STRINGREF_INIT(L"SystemInformer-Build: ");
    ULONG majorVersion;
    ULONG minorVersion;
    ULONG buildVersion;
    ULONG revisionVersion;
    SIZE_T returnLength;
    PH_FORMAT format[8];
    WCHAR formatBuffer[260];

    PhGetPhVersionNumbers(&majorVersion, &minorVersion, &buildVersion, &revisionVersion);
    PhInitFormatSR(&format[0], versionHeader);
    PhInitFormatU(&format[1], majorVersion);
    PhInitFormatC(&format[2], L'.');
    PhInitFormatU(&format[3], minorVersion);
    PhInitFormatC(&format[4], L'.');
    PhInitFormatU(&format[5], buildVersion);
    PhInitFormatC(&format[6], L'.');
    PhInitFormatU(&format[7], revisionVersion);

    if (PhFormatToBuffer(format, RTL_NUMBER_OF(format), formatBuffer, sizeof(formatBuffer), &returnLength))
    {
        PH_STRINGREF stringFormat;

        stringFormat.Buffer = formatBuffer;
        stringFormat.Length = returnLength - sizeof(UNICODE_NULL);

        return PhCreateString2(&stringFormat);
    }
    else
    {
        return PhFormat(format, RTL_NUMBER_OF(format), 0);
    }
}

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
    PH_MAPPED_IMAGE mappedImage;
    LARGE_INTEGER fileSize;
    PH_HASH_CONTEXT hashContext;
    ULONG64 bytesRemaining;
    ULONG numberOfBytesRead;
    BYTE buffer[PAGE_SIZE];
    BYTE hash[256 / 8];

    if (!NT_SUCCESS(status = PhCreateFile(
        &fileHandle,
        FileName,
        FILE_GENERIC_READ,
        FILE_ATTRIBUTE_NORMAL,
        FILE_SHARE_READ | FILE_SHARE_DELETE,
        FILE_OPEN,
        FILE_NON_DIRECTORY_FILE | FILE_SYNCHRONOUS_IO_NONALERT
        )))
        return status;

    if (!NT_SUCCESS(status = PhGetFileSize(fileHandle, &fileSize)))
        goto CleanupExit;

    PhInitializeHash(&hashContext, Sha256HashAlgorithm);

    bytesRemaining = (ULONG64)fileSize.QuadPart;

    while (bytesRemaining)
    {
        status = PhReadFile(
            fileHandle,
            buffer,
            sizeof(buffer),
            NULL,
            &numberOfBytesRead
            );

        if (!NT_SUCCESS(status))
            break;

        PhUpdateHash(&hashContext, buffer, numberOfBytesRead);
        bytesRemaining -= numberOfBytesRead;
    }

    if (NT_SUCCESS(status = PhLoadMappedImageHeaderPageSize(NULL, fileHandle, &mappedImage)))
    {
        *ImageMachine = mappedImage.NtHeaders->FileHeader.Machine;
        *TimeDateStamp = mappedImage.NtHeaders->FileHeader.TimeDateStamp;
        *SizeOfImage = mappedImage.NtHeaders->OptionalHeader.SizeOfImage;

        PhFinalHash(&hashContext, hash, sizeof(hash), NULL);
        *HashString = PhBufferToHexString(hash, sizeof(hash));

        PhUnloadMappedImage(&mappedImage);
    }

CleanupExit:

    NtClose(fileHandle);

    return status;
}

PPH_STRING UpdatePlatformSupportString(
    VOID
    )
{
    static CONST PH_STRINGREF platformHeader = PH_STRINGREF_INIT(L"SystemInformer-PlatformSupport: ");
    static UPDATER_PLATFORM_SUPPORT_ENTRY platformFiles[] =
    {
        { KPH_DYN_CLASS_NTOSKRNL, PH_STRINGREF_INIT(L"\\SystemRoot\\System32\\ntoskrnl.exe") },
        { KPH_DYN_CLASS_NTKRLA57, PH_STRINGREF_INIT(L"\\SystemRoot\\System32\\ntkrla57.exe") },
        { KPH_DYN_CLASS_LXCORE,   PH_STRINGREF_INIT(L"\\SystemRoot\\System32\\drivers\\lxcore.sys") },
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

PPH_STRING UpdateWindowsString(
    VOID
    )
{
    PPH_STRING buildString = NULL;
    PPH_STRING fileName;
    PVOID imageBase;
    ULONG imageSize;
    PVOID versionInfo;

    if (NT_SUCCESS(PhGetKernelFileNameEx(&fileName, &imageBase, &imageSize)))
    {
        if (versionInfo = PhGetFileVersionInfoEx(&fileName->sr))
        {
            VS_FIXEDFILEINFO* rootBlock;

            if (rootBlock = PhGetFileVersionFixedInfo(versionInfo))
            {
                PH_FORMAT format[5];

                PhInitFormatS(&format[0], L"SystemInformer-OsBuild: ");
                PhInitFormatU(&format[1], HIWORD(rootBlock->dwFileVersionLS));
                PhInitFormatC(&format[2], '.');
                PhInitFormatU(&format[3], LOWORD(rootBlock->dwFileVersionLS));
                PhInitFormatS(&format[4], PhIsExecutingInWow64() ? L"_64" : L"_32");

                buildString = PhFormat(format, RTL_NUMBER_OF(format), 0);
            }

            PhFree(versionInfo);
        }

        PhDereferenceObject(fileName);
    }

    return buildString;
}

ULONG64 ParseVersionString(
    _Inout_ PPH_STRING VersionString
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

    remaining = PhGetStringRef(VersionString);
    PhSplitStringRefAtChar(&remaining, L'.', &majorPart, &remaining);
    PhSplitStringRefAtChar(&remaining, L'.', &minorPart, &remaining);
    PhSplitStringRefAtChar(&remaining, L'.', &revisionPart, &remaining);
    PhSplitStringRefAtChar(&remaining, L'.', &buildPart, &remaining);

    if (majorPart.Length)
    {
        PhStringToUInt64(&majorPart, 10, &majorInteger);
    }

    if (minorPart.Length)
    {
        PhStringToUInt64(&minorPart, 10, &minorInteger);
    }

    if (revisionPart.Length)
    {
        PhStringToUInt64(&revisionPart, 10, &buildInteger);
    }

    if (buildPart.Length)
    {
        PhStringToUInt64(&buildPart, 10, &revisionInteger);
    }

    return MAKE_VERSION_ULONGLONG(
        (ULONG)majorInteger,
        (ULONG)minorInteger,
        (ULONG)buildInteger,
        (ULONG)revisionInteger
        );
}

BOOLEAN QueryUpdateData(
    _Inout_ PPH_UPDATER_CONTEXT Context,
    _In_ PWSTR ServerName
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

    status = PhHttpConnect(httpContext, ServerName, PH_HTTP_DEFAULT_HTTPS_PORT);

    if (!NT_SUCCESS(status))
        goto CleanupExit;

    {
        if (!Context->SwitchingChannel)
        {
            Context->Channel = PhGetPhReleaseChannel();
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
    }

    if (!NT_SUCCESS(status = PhHttpSendRequest(httpContext, NULL, 0, 0)))
        goto CleanupExit;
    if (!NT_SUCCESS(status = PhHttpReceiveResponse(httpContext)))
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

#if defined(FORCE_FUTURE_VERSION)
    PhGetPhVersionNumbers(&majorVersion, &minorVersion, &buildVersion, &revisionVersion);
    Context->CurrentVersion = MAKE_VERSION_ULONGLONG(USHRT_MAX, USHRT_MAX, USHRT_MAX, USHRT_MAX);
    Context->LatestVersion = MAKE_VERSION_ULONGLONG(majorVersion, minorVersion, buildVersion, revisionVersion);
#elif defined(FORCE_LATEST_VERSION)
    PhGetPhVersionNumbers(&majorVersion, &minorVersion, &buildVersion, &revisionVersion);
    Context->CurrentVersion = MAKE_VERSION_ULONGLONG(0, 0, 0, 0);
    Context->LatestVersion = MAKE_VERSION_ULONGLONG(majorVersion, minorVersion, buildVersion, revisionVersion);
#else
    PhGetPhVersionNumbers(&majorVersion, &minorVersion, &buildVersion, &revisionVersion);
    Context->CurrentVersion = MAKE_VERSION_ULONGLONG(majorVersion, minorVersion, buildVersion, revisionVersion);
    Context->LatestVersion = ParseVersionString(Context->Version);
#endif

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

    success = TRUE;

    if (PhGetIntegerSetting(SETTING_NAME_UPDATE_MODE))
    {
        PPH_STRING jsonStringUtf16 = PhConvertBytesToUtf16(jsonString);
        PhSetStringSetting2(SETTING_NAME_UPDATE_DATA, &jsonStringUtf16->sr);
        PhDereferenceObject(jsonStringUtf16);
    }

CleanupExit:

    if (httpContext)
        PhHttpDestroy(httpContext);
    if (jsonString)
        PhDereferenceObject(jsonString);

    return success;
}

BOOLEAN QueryUpdateDataWithFailover(
    _Inout_ PPH_UPDATER_CONTEXT Context
    )
{
    static PWSTR Servers[] =
    {
        L"system-informer.com",
        L"systeminformer.com",
        L"systeminformer.sourceforge.io",
    };

    for (ULONG i = 0; i < ARRAYSIZE(Servers); i++)
    {
        if (QueryUpdateData(Context, Servers[i]))
            return TRUE;
    }

    return FALSE;
}

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

    PhDelayExecution(5 * 1000);

    PhClearCacheDirectory(context->PortableMode);

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
            // Check if the user hasn't already opened the dialog.
            if (!UpdateDialogHandle)
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

NTSTATUS UpdateCheckThread(
    _In_ PVOID Parameter
    )
{
    PPH_UPDATER_CONTEXT context;
    PH_AUTO_POOL autoPool;

    context = (PPH_UPDATER_CONTEXT)Parameter;
    context->UpdateStatus = STATUS_SUCCESS;

    PhInitializeAutoPool(&autoPool);

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

PPH_STRING UpdateParseDownloadFileName(
    _In_ PPH_UPDATER_CONTEXT Context,
    _In_ PPH_STRING DownloadUrlPath
    )
{
    PH_STRINGREF pathPart;
    PH_STRINGREF namePart;
    PPH_STRING downloadFileName;
    PPH_STRING localfileName;

    if (!PhSplitStringRefAtLastChar(&DownloadUrlPath->sr, L'/', &pathPart, &namePart))
        return NULL;

    downloadFileName = PhCreateString2(&namePart);
    localfileName = PhCreateCacheFile(Context->PortableMode, downloadFileName, FALSE);
    PhDereferenceObject(downloadFileName);

    return localfileName;
}

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
    ULONG_PTR totalDownloaded = 0;
    PPH_STRING string;
    IO_STATUS_BLOCK isb;
    PBYTE httpBuffer = NULL;
    ULONG httpBufferLength;

    SendMessage(context->DialogHandle, TDM_UPDATE_ELEMENT_TEXT, TDE_MAIN_INSTRUCTION, (LPARAM)L"Initializing download request...");

    if (!NT_SUCCESS(status = PhHttpCrackUrl(context->SetupFileDownloadUrl, &downloadHostPath, &downloadUrlPath, &httpPort)))
        goto CleanupExit;

    SendMessage(context->DialogHandle, TDM_UPDATE_ELEMENT_TEXT, TDE_MAIN_INSTRUCTION, (LPARAM)L"Connecting...");

    if (!NT_SUCCESS(status = PhHttpInitialize(&httpContext)))
        goto CleanupExit;

    PhHttpSetProtocal(httpContext, TRUE, PH_HTTP_PROTOCOL_FLAG_HTTP2, 5000);

    if (!NT_SUCCESS(status = PhHttpConnect(httpContext, PhGetString(downloadHostPath), httpPort)))
        goto CleanupExit;
    if (!NT_SUCCESS(status = PhHttpBeginRequest(httpContext, NULL, PhGetString(downloadUrlPath), PH_HTTP_FLAG_SECURE)))
        goto CleanupExit;

    PhHttpSetFeature(httpContext, PH_HTTP_FEATURE_KEEP_ALIVE, FALSE);

    SendMessage(context->DialogHandle, TDM_UPDATE_ELEMENT_TEXT, TDE_MAIN_INSTRUCTION, (LPARAM)L"Sending download request...");

    if (!NT_SUCCESS(status = PhHttpSendRequest(httpContext, NULL, 0, 0)))
        goto CleanupExit;

    SendMessage(context->DialogHandle, TDM_UPDATE_ELEMENT_TEXT, TDE_MAIN_INSTRUCTION, (LPARAM)L"Waiting for response...");

    if (!NT_SUCCESS(status = PhHttpReceiveResponse(httpContext)))
        goto CleanupExit;
    if (!NT_SUCCESS(status = PhHttpQueryHeaderUlong(httpContext, PH_HTTP_QUERY_CONTENT_LENGTH, &contentLength)))
        goto CleanupExit;

    httpBufferLength = PAGE_SIZE;
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

    {
        // Create temporary path.
        context->SetupFilePath = UpdateParseDownloadFileName(context, downloadUrlPath);

        if (PhIsNullOrEmptyString(context->SetupFilePath))
        {
            status = STATUS_FILE_HANDLE_REVOKED;
            goto CleanupExit;
        }

        allocationSize.QuadPart = contentLength;

        // Create temporary file.
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
    status = UpdaterInitializeHash(&hashContext, context->Channel);

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

        // If we get zero bytes, the file was uploaded or there was an error
        if (bytesDownloaded == 0)
            break;

        // If the dialog was closed, just cleanup and exit
        if (context->Cancel)
            goto CleanupExit;

        // Update the hash of bytes we downloaded.
        if (!NT_SUCCESS(status = UpdaterUpdateHash(hashContext, httpBuffer, bytesDownloaded)))
            goto CleanupExit;

        // Write the downloaded bytes to disk.
        if (!NT_SUCCESS(status = NtWriteFile(
            tempFileHandle,
            NULL,
            NULL,
            NULL,
            &isb,
            httpBuffer,
            bytesDownloaded,
            NULL,
            NULL
            )))
        {
            goto CleanupExit;
        }

        // Check the number of bytes written are the same we downloaded.
        if (bytesDownloaded != isb.Information)
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
        totalDownloaded += isb.Information;
        timeTicks = (timeNow.QuadPart - timeStart.QuadPart) / PH_TICKS_PER_SEC;
        timeBitsPerSecond = timeTicks ? totalDownloaded / timeTicks : 0;

#ifdef FORCE_NO_STATUS_TIMER
        ULONG percent = totalDownloaded * 100 / contentLength;
        PH_FORMAT format[9];
        WCHAR string[MAX_PATH];

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

        if (PhFormatToBuffer(format, RTL_NUMBER_OF(format), string, sizeof(string), NULL))
        {
            SendMessage(context->DialogHandle, TDM_UPDATE_ELEMENT_TEXT, TDE_CONTENT, (LPARAM)string);
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

    if (NT_SUCCESS(status = UpdaterVerifyHash(hashContext, context->SetupFileHash)))
    {
        hashSuccess = TRUE;

        if (NT_SUCCESS(status = UpdaterVerifySignature(hashContext, context->SetupFileSignature)))
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
        UpdaterDestroyHash(hashContext);
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
    case WM_NCDESTROY:
        {
            context->Cancel = TRUE;

            PhSetWindowProcedure(hwndDlg, oldWndProc);
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
                    LONG64 percent = context->ProgressDownloaded * 100 / context->ProgressTotal;
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

            PhSetApplicationWindowIconEx(hwndDlg, windowDpi);
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
    case TDN_DIALOG_CONSTRUCTED:
        {
            UpdateDialogHandle = context->DialogHandle = hwndDlg;

            // Center the update window on PH if it's visible else we center on the desktop.
            PhCenterWindow(hwndDlg, SystemInformer_GetWindowHandle());

            // Create the Taskdialog icons.
            PhSetApplicationWindowIconEx(hwndDlg, PhGetWindowDpi(hwndDlg));

            PhRegisterWindowCallback(hwndDlg, PH_PLUGIN_WINDOW_EVENT_TYPE_TOPMOST, NULL);

            // Subclass the Taskdialog.
            context->DefaultWindowProc = PhGetWindowProcedure(hwndDlg);
            PhSetWindowContext(hwndDlg, UCHAR_MAX, context);
            PhSetWindowProcedure(hwndDlg, TaskDialogSubclassProc);

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
    config.hInstance = PluginInstance->DllBase;
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

VOID ShowUpdateDialog(
    _In_opt_ PPH_UPDATER_CONTEXT Context
    )
{
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

VOID StartInitialCheck(
    VOID
    )
{
    PhQueueItemWorkQueue(PhGetGlobalWorkQueue(), UpdateCheckSilentThread, NULL);
}

VOID ShowStartupUpdateDialog(
    VOID
    )
{
    PH_AUTO_POOL autoPool;
    PPH_UPDATER_CONTEXT context;
    PPH_STRING jsonString;

    PhInitializeAutoPool(&autoPool);

    context = CreateUpdateContext(TRUE);

    jsonString = PhGetStringSetting(SETTING_NAME_UPDATE_DATA);

    if (jsonString && jsonString->Length)
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

            PhGetPhVersionNumbers(&majorVersion, &minorVersion, &buildVersion, &revisionVersion);
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

    if (PhIsNullOrEmptyString(context->Version) &&
        PhIsNullOrEmptyString(context->RelDate) &&
        PhIsNullOrEmptyString(context->SetupFileDownloadUrl) &&
        PhIsNullOrEmptyString(context->SetupFileLength) &&
        PhIsNullOrEmptyString(context->SetupFileHash) &&
        PhIsNullOrEmptyString(context->SetupFileSignature) &&
        PhIsNullOrEmptyString(context->CommitHash))
    {
        goto CleanupExit;
    }

    TASKDIALOGCONFIG config = { sizeof(TASKDIALOGCONFIG) };
    config.dwFlags = TDF_ALLOW_DIALOG_CANCELLATION | TDF_CAN_BE_MINIMIZED;
    config.hInstance = PluginInstance->DllBase;
    config.pszContent = L"Initializing...";
    config.lpCallbackData = (LONG_PTR)context;
    config.pfCallback = TaskDialogBootstrapCallback;
    PhShowTaskDialog(&config, NULL, NULL, NULL);

CleanupExit:
    PhDereferenceObject(context);
    PhDeleteAutoPool(&autoPool);
}
