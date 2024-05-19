/*
 * Copyright (c) 2022 Winsider Seminars & Solutions, Inc.  All rights reserved.
 *
 * This file is part of System Informer.
 *
 * Authors:
 *
 *     dmex    2016-2020
 *
 */

#include "onlnchk.h"

HANDLE VirusTotalHandle = NULL;
PPH_LIST VirusTotalList = NULL;
PH_QUEUED_LOCK ProcessListLock = PH_QUEUED_LOCK_INIT;

PPH_BYTES VirusTotalTimeString(
    _In_ PLARGE_INTEGER LargeInteger
    )
{
    SYSTEMTIME systemTime;
    PPH_STRING dateString;
    PPH_STRING timeString;
    PPH_BYTES result;

    PhLargeIntegerToLocalSystemTime(&systemTime, LargeInteger);
    dateString = PhFormatDate(&systemTime, L"yyyy-MM-dd");
    timeString = PhFormatTime(&systemTime, L"HH:mm:ss");

    result = PhFormatBytes("%S %S", dateString->Buffer, timeString->Buffer);

    PhDereferenceObject(timeString);
    PhDereferenceObject(dateString);

    return result;
}

PPH_STRING VirusTotalStringToTime(
    _In_ PPH_STRING Time
    )
{
    PPH_STRING result = NULL;
    SYSTEMTIME time = { 0 };
    SYSTEMTIME localTime = { 0 };
    INT count;

    count = swscanf(
        PhGetString(Time),
        L"%hu-%hu-%hu %hu:%hu:%hu",
        &time.wYear,
        &time.wMonth,
        &time.wDay,
        &time.wHour,
        &time.wMinute,
        &time.wSecond
        );

    if (count == 6)
    {
        if (SystemTimeToTzSpecificLocalTime(NULL, &time, &localTime))
        {
            result = PhFormatDateTime(&localTime);
        }
    }

    return result;
}

PVIRUSTOTAL_FILE_HASH_ENTRY VirusTotalAddCacheResult(
    _In_ PPH_STRING FileName,
    _In_ PPROCESS_EXTENSION Extension
    )
{
    PVIRUSTOTAL_FILE_HASH_ENTRY result;

    result = PhAllocateZero(sizeof(VIRUSTOTAL_FILE_HASH_ENTRY));
    result->FileName = PhReferenceObject(FileName);
    result->FileNameAnsi = PhConvertUtf16ToMultiByte(PhGetString(FileName));
    result->Extension = Extension;

    PhAcquireQueuedLockExclusive(&ProcessListLock);
    PhAddItemList(VirusTotalList, result);
    PhReleaseQueuedLockExclusive(&ProcessListLock);

    return result;
}

VOID VirusTotalRemoveCacheResult(
    _In_ PPROCESS_EXTENSION Extension
    )
{
    PhAcquireQueuedLockExclusive(&ProcessListLock);

    for (ULONG i = 0; i < VirusTotalList->Count; i++)
    {
        PVIRUSTOTAL_FILE_HASH_ENTRY extension = VirusTotalList->Items[i];

        PhRemoveItemList(VirusTotalList, i);

        PhClearReference(&extension->FileName);
        PhClearReference(&extension->FileHash);
        PhClearReference(&extension->FileNameAnsi);
        PhClearReference(&extension->FileHashAnsi);
        PhClearReference(&extension->CreationTime);

        PhFree(extension);
    }

    //PhClearList(VirusTotalList);
    //PhDereferenceObject(VirusTotalList);

    PhReleaseQueuedLockExclusive(&ProcessListLock);
}

PVIRUSTOTAL_FILE_HASH_ENTRY VirusTotalGetCachedResult(
    _In_ PPH_STRING FileName
    )
{
    ULONG i;
    BOOLEAN found = FALSE;

    PhAcquireQueuedLockExclusive(&ProcessListLock);

    for (i = 0; i < VirusTotalList->Count; i++)
    {
        PVIRUSTOTAL_FILE_HASH_ENTRY extension = VirusTotalList->Items[i];

        if (PhEqualString(extension->FileName, FileName, TRUE))
        {
            PhReleaseQueuedLockExclusive(&ProcessListLock);
            return extension;
        }
    }

    PhReleaseQueuedLockExclusive(&ProcessListLock);
    return NULL;
}

PVIRUSTOTAL_FILE_HASH_ENTRY VirusTotalGetCachedResultFromHash(
    _In_ PPH_STRING FileHash
    )
{
    ULONG i;
    BOOLEAN found = FALSE;

    PhAcquireQueuedLockExclusive(&ProcessListLock);

    for (i = 0; i < VirusTotalList->Count; i++)
    {
        PVIRUSTOTAL_FILE_HASH_ENTRY extension = VirusTotalList->Items[i];

        if (PhIsNullOrEmptyString(extension->FileHash))
            continue;

        if (PhEqualString(extension->FileHash, FileHash, TRUE))
        {
            PhReleaseQueuedLockExclusive(&ProcessListLock);
            return extension;
        }
    }

    PhReleaseQueuedLockExclusive(&ProcessListLock);
    return NULL;
}

PPH_BYTES VirusTotalGetCachedDbHash(
    _In_ PPH_STRINGREF CachedHash
    )
{
    ULONG length;
    PUCHAR buffer;
    PPH_BYTES string;

    length = (ULONG)CachedHash->Length / sizeof(WCHAR) / 2;

    buffer = PhAllocate(length + 1);
    memset(buffer, 0, length + 1);

    PhHexStringToBuffer(CachedHash, buffer);

    string = PhCreateBytes(buffer);

    for (SIZE_T i = 0; i < string->Length; i++)
        string->Buffer[i] = string->Buffer[i] ^ 0x0D06F00D;

    PhFree(buffer);
    return string;
}

PPH_LIST VirusTotalJsonToResultList(
    _In_ PVOID JsonObject
    )
{
    ULONG i;
    ULONG arrayLength;
    PPH_LIST results;

    if (!(arrayLength = PhGetJsonArrayLength(JsonObject)))
        return NULL;

    results = PhCreateList(arrayLength);

    for (i = 0; i < arrayLength; i++)
    {
        PVIRUSTOTAL_API_RESULT result;
        PVOID jsonArrayObject;

        if (!(jsonArrayObject = PhGetJsonArrayIndexObject(JsonObject, i)))
            continue;

        result = PhAllocateZero(sizeof(VIRUSTOTAL_API_RESULT));
        result->FileHash = PhGetJsonValueAsString(jsonArrayObject, "hash");
        result->Found = PhGetJsonObjectBool(jsonArrayObject, "found") == TRUE;
        result->Positives = PhGetJsonValueAsUInt64(jsonArrayObject, "positives");
        result->Total = PhGetJsonValueAsUInt64(jsonArrayObject, "total");

        PhAddItemList(results, result);
    }

    return results;
}

VOID VirusTotalBuildJsonArray(
    _In_ PVIRUSTOTAL_FILE_HASH_ENTRY Entry,
    _In_ PVOID JsonArray
    )
{
    HANDLE fileHandle;
    FILE_NETWORK_OPEN_INFORMATION fileAttributeInfo;
    PPH_STRING hashString = NULL;

    if (NT_SUCCESS(PhQueryFullAttributesFileWin32(
        Entry->FileName->Buffer,
        &fileAttributeInfo
        )))
    {
        Entry->CreationTime = VirusTotalTimeString(&fileAttributeInfo.CreationTime);
    }

    if (NT_SUCCESS(PhCreateFileWin32(
        &fileHandle,
        Entry->FileName->Buffer,
        FILE_GENERIC_READ,
        FILE_ATTRIBUTE_NORMAL,
        FILE_SHARE_READ,
        FILE_OPEN,
        FILE_NON_DIRECTORY_FILE | FILE_SYNCHRONOUS_IO_NONALERT // FILE_OPEN_FOR_BACKUP_INTENT
        )))
    {
        if (NT_SUCCESS(HashFileAndResetPosition(
            fileHandle,
            &fileAttributeInfo.EndOfFile,
            Sha256HashAlgorithm,
            &hashString
            )))
        {
            PVOID entry;

            Entry->FileHash = hashString;
            Entry->FileHashAnsi = PhConvertUtf16ToMultiByte(hashString->Buffer);

            entry = PhCreateJsonObject();
            PhAddJsonObject(entry, "autostart_location", "");
            PhAddJsonObject(entry, "autostart_entry", "");
            PhAddJsonObject(entry, "hash", Entry->FileHashAnsi->Buffer);
            PhAddJsonObject(entry, "image_path", Entry->FileNameAnsi->Buffer);
            PhAddJsonObject(entry, "creation_datetime", Entry->CreationTime ? Entry->CreationTime->Buffer : "");
            PhAddJsonArrayObject(JsonArray, entry);
        }

        NtClose(fileHandle);
    }
}

PPH_BYTES VirusTotalSendHttpRequest(
    _In_ PPH_BYTES JsonArray
    )
{
    PPH_BYTES subRequestBuffer = NULL;
    PPH_HTTP_CONTEXT httpContext = NULL;
    PPH_STRING versionString = NULL;
    PPH_STRING userAgentString = NULL;
    PPH_STRING urlPathString = NULL;

    versionString = PhGetPhVersion();
    userAgentString = PhConcatStrings2(L"ProcessHacker_", versionString->Buffer);

    if (!PhHttpSocketCreate(&httpContext, PhGetString(userAgentString)))
        goto CleanupExit;

    if (!PhHttpSocketConnect(httpContext, L"www.virustotal.com", PH_HTTP_DEFAULT_HTTPS_PORT))
        goto CleanupExit;

    {
        PPH_BYTES resourceString = VirusTotalGetCachedDbHash(&ProcessObjectDbHash);

        urlPathString = PhFormatString(
            L"%s%s%s%s%S",
            L"/partners",
            L"/sysinternals",
            L"/file-reports",
            L"?\x0061\x0070\x0069\x006B\x0065\x0079=",
            resourceString->Buffer
            );

        PhClearReference(&resourceString);
    }

    if (!PhHttpSocketBeginRequest(
        httpContext,
        L"POST",
        urlPathString->Buffer,
        PH_HTTP_FLAG_REFRESH | PH_HTTP_FLAG_SECURE
        ))
    {
        goto CleanupExit;
    }

    if (!PhHttpSocketAddRequestHeaders(httpContext, L"Content-Type: application/json", 0))
        goto CleanupExit;

    if (!PhHttpSocketSendRequest(httpContext, JsonArray->Buffer, (ULONG)JsonArray->Length))
        goto CleanupExit;

    if (!PhHttpSocketEndRequest(httpContext))
        goto CleanupExit;

    if (!(subRequestBuffer = PhHttpSocketDownloadString(httpContext, FALSE)))
        goto CleanupExit;

CleanupExit:

    if (httpContext)
        PhHttpSocketDestroy(httpContext);

    PhClearReference(&urlPathString);
    PhClearReference(&versionString);
    PhClearReference(&userAgentString);

    if (JsonArray)
        PhDereferenceObject(JsonArray);

    return subRequestBuffer;
}

PVIRUSTOTAL_FILE_REPORT VirusTotalRequestFileReport(
    _In_ PPH_STRING FileHash
    )
{
    PVIRUSTOTAL_FILE_REPORT result = NULL;
    PPH_BYTES jsonString = NULL;
    PPH_HTTP_CONTEXT httpContext = NULL;
    PPH_STRING versionString = NULL;
    PPH_STRING userAgentString = NULL;
    PPH_STRING urlPathString = NULL;
    PVOID jsonRootObject = NULL;
    PVOID jsonScanObject;

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
        L"www.virustotal.com",
        PH_HTTP_DEFAULT_HTTPS_PORT
        ))
    {
        goto CleanupExit;
    }

    {
        PPH_BYTES resourceString = VirusTotalGetCachedDbHash(&ProcessObjectDbHash);

        urlPathString = PhFormatString(
            L"%s%s%s%s%s%S%s%s",
            L"/vtapi",
            L"/v2",
            L"/file",
            L"/report",
            L"?\x0061\x0070\x0069\x006B\x0065\x0079=",
            resourceString->Buffer,
            L"&resource=",
            FileHash->Buffer
            );

        PhClearReference(&resourceString);
    }

    if (!PhHttpSocketBeginRequest(
        httpContext,
        L"POST",
        PhGetString(urlPathString),
        PH_HTTP_FLAG_REFRESH | PH_HTTP_FLAG_SECURE
        ))
    {
        goto CleanupExit;
    }

    if (!PhHttpSocketAddRequestHeaders(httpContext, L"Content-Type: application/json", 0))
        goto CleanupExit;

    if (!PhHttpSocketSendRequest(httpContext, NULL, 0))
        goto CleanupExit;

    if (!PhHttpSocketEndRequest(httpContext))
        goto CleanupExit;

    if (!(jsonString = PhHttpSocketDownloadString(httpContext, FALSE)))
        goto CleanupExit;

    if (!(jsonRootObject = PhCreateJsonParser(jsonString->Buffer)))
        goto CleanupExit;

    result = PhAllocateZero(sizeof(VIRUSTOTAL_FILE_REPORT));
    result->ResponseCode = PhGetJsonValueAsUInt64(jsonRootObject, "response_code");
    result->StatusMessage = PhGetJsonValueAsString(jsonRootObject, "verbose_msg");
    result->PermaLink = PhGetJsonValueAsString(jsonRootObject, "permalink");
    result->ScanDate = PhGetJsonValueAsString(jsonRootObject, "scan_date");
    result->ScanId = PhGetJsonValueAsString(jsonRootObject, "scan_id");
    result->Total = PhFormatUInt64(PhGetJsonValueAsUInt64(jsonRootObject, "total"), FALSE);
    result->Positives = PhFormatUInt64(PhGetJsonValueAsUInt64(jsonRootObject, "positives"), FALSE);
    //result->Md5 = PhGetJsonValueAsString(jsonRootObject, "md5");
    //result->Sha1 = PhGetJsonValueAsString(jsonRootObject, "sha1");
    //result->Sha256 = PhGetJsonValueAsString(jsonRootObject, "sha256");

    if (jsonScanObject = PhGetJsonObject(jsonRootObject, "scans"))
    {
        PPH_LIST jsonArrayList;

        if (jsonArrayList = PhGetJsonObjectAsArrayList(jsonScanObject))
        {
            result->ScanResults = PhCreateList(jsonArrayList->Count);

            for (ULONG i = 0; i < jsonArrayList->Count; i++)
            {
                PVIRUSTOTAL_FILE_REPORT_RESULT entry;
                PJSON_ARRAY_LIST_OBJECT object = jsonArrayList->Items[i];

                entry = PhAllocateZero(sizeof(VIRUSTOTAL_FILE_REPORT_RESULT));
                entry->Vendor = PhConvertUtf8ToUtf16(object->Key);
                entry->Detected = PhGetJsonObjectBool(object->Entry, "detected");
                entry->EngineVersion = PhGetJsonValueAsString(object->Entry, "version");
                entry->DetectionName = PhGetJsonValueAsString(object->Entry, "result");
                entry->DatabaseDate = PhGetJsonValueAsString(object->Entry, "update");
                PhAddItemList(result->ScanResults, entry);

                PhFree(object);
            }

            PhDereferenceObject(jsonArrayList);
        }
    }

CleanupExit:

    if (httpContext)
        PhHttpSocketDestroy(httpContext);

    if (jsonRootObject)
        PhFreeJsonObject(jsonRootObject);

    PhClearReference(&jsonString);
    PhClearReference(&versionString);
    PhClearReference(&userAgentString);

    return result;
}

VOID VirusTotalFreeFileReport(
    _In_ PVIRUSTOTAL_FILE_REPORT FileReport
    )
{
    PhClearReference(&FileReport->StatusMessage);
    PhClearReference(&FileReport->PermaLink);
    PhClearReference(&FileReport->ScanId);
    PhClearReference(&FileReport->ScanDate);
    PhClearReference(&FileReport->Positives);
    PhClearReference(&FileReport->Total);

    if (FileReport->ScanResults)
    {
        for (ULONG i = 0; i < FileReport->ScanResults->Count; i++)
        {
            PVIRUSTOTAL_FILE_REPORT_RESULT object = FileReport->ScanResults->Items[i];

            PhClearReference(&object->Vendor);
            PhClearReference(&object->EngineVersion);
            PhClearReference(&object->DetectionName);
            PhClearReference(&object->DatabaseDate);

            PhFree(object);
        }

        PhClearReference(&FileReport->ScanResults);
    }

    PhFree(FileReport);
}

PVIRUSTOTAL_API_RESPONSE VirusTotalRequestFileReScan(
    _In_ PPH_STRING FileHash
    )
{
    PVIRUSTOTAL_API_RESPONSE result = NULL;
    PPH_BYTES jsonString = NULL;
    PPH_HTTP_CONTEXT httpContext = NULL;
    PPH_STRING versionString = NULL;
    PPH_STRING userAgentString = NULL;
    PPH_STRING urlPathString = NULL;
    PVOID jsonRootObject = NULL;

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
        L"www.virustotal.com",
        PH_HTTP_DEFAULT_HTTPS_PORT
        ))
    {
        goto CleanupExit;
    }

    {
        PPH_BYTES resourceString = VirusTotalGetCachedDbHash(&ProcessObjectDbHash);

        urlPathString = PhFormatString(
            L"%s%s%s%s%s%S%s%s",
            L"/vtapi",
            L"/v2",
            L"/file",
            L"/rescan",
            L"?\x0061\x0070\x0069\x006B\x0065\x0079=",
            resourceString->Buffer,
            L"&resource=",
            FileHash->Buffer
            );

        PhClearReference(&resourceString);
    }

    if (!PhHttpSocketBeginRequest(
        httpContext,
        L"POST",
        PhGetString(urlPathString),
        PH_HTTP_FLAG_REFRESH | PH_HTTP_FLAG_SECURE
        ))
    {
        goto CleanupExit;
    }

    if (!PhHttpSocketAddRequestHeaders(httpContext, L"Content-Type: application/json", 0))
        goto CleanupExit;

    if (!PhHttpSocketSendRequest(httpContext, NULL, 0))
        goto CleanupExit;

    if (!PhHttpSocketEndRequest(httpContext))
        goto CleanupExit;

    if (!(jsonString = PhHttpSocketDownloadString(httpContext, FALSE)))
        goto CleanupExit;

    if (!(jsonRootObject = PhCreateJsonParser(jsonString->Buffer)))
        goto CleanupExit;

    result = PhAllocateZero(sizeof(VIRUSTOTAL_API_RESPONSE));
    result->ResponseCode = PhGetJsonValueAsUInt64(jsonRootObject, "response_code");
    result->StatusMessage = PhGetJsonValueAsString(jsonRootObject, "verbose_msg");
    result->PermaLink = PhGetJsonValueAsString(jsonRootObject, "permalink");
    result->ScanId = PhGetJsonValueAsString(jsonRootObject, "scan_id");

CleanupExit:

    if (httpContext)
        PhHttpSocketDestroy(httpContext);

    if (jsonRootObject)
        PhFreeJsonObject(jsonRootObject);

    PhClearReference(&jsonString);
    PhClearReference(&versionString);
    PhClearReference(&userAgentString);

    return result;
}

VOID VirusTotalFreeFileReScan(
    _In_ PVIRUSTOTAL_API_RESPONSE FileReScan
    )
{
    PhClearReference(&FileReScan->StatusMessage);
    PhClearReference(&FileReScan->PermaLink);
    PhClearReference(&FileReScan->ScanId);

    PhFree(FileReScan);
}

PVIRUSTOTAL_API_RESPONSE VirusTotalRequestIpAddressReport(
    _In_ PPH_STRING IpAddress
    )
{
    PVIRUSTOTAL_API_RESPONSE result = NULL;
    PPH_BYTES jsonString = NULL;
    PPH_HTTP_CONTEXT httpContext = NULL;
    PPH_STRING versionString = NULL;
    PPH_STRING userAgentString = NULL;
    PPH_STRING urlPathString = NULL;
    PVOID jsonRootObject = NULL;

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
        L"www.virustotal.com",
        PH_HTTP_DEFAULT_HTTPS_PORT
        ))
    {
        goto CleanupExit;
    }

    {
        PPH_BYTES resourceString = VirusTotalGetCachedDbHash(&ProcessObjectDbHash);

        urlPathString = PhFormatString(
            L"%s%s%s%s%s%S%s%s",
            L"/vtapi",
            L"/v2",
            L"/ip-address",
            L"/report",
            L"?\x0061\x0070\x0069\x006B\x0065\x0079=",
            resourceString->Buffer,
            L"&ip=",
            IpAddress->Buffer
            );

        PhClearReference(&resourceString);
    }

    if (!PhHttpSocketBeginRequest(
        httpContext,
        L"POST",
        PhGetString(urlPathString),
        PH_HTTP_FLAG_REFRESH | PH_HTTP_FLAG_SECURE
        ))
    {
        goto CleanupExit;
    }

    if (!PhHttpSocketAddRequestHeaders(httpContext, L"Content-Type: application/json", 0))
        goto CleanupExit;

    if (!PhHttpSocketSendRequest(httpContext, NULL, 0))
        goto CleanupExit;

    if (!PhHttpSocketEndRequest(httpContext))
        goto CleanupExit;

    if (!(jsonString = PhHttpSocketDownloadString(httpContext, FALSE)))
        goto CleanupExit;

    if (!(jsonRootObject = PhCreateJsonParser(jsonString->Buffer)))
        goto CleanupExit;

    result = PhAllocateZero(sizeof(VIRUSTOTAL_API_RESPONSE));
    //result->ResponseCode = PhGetJsonValueAsLong64(jsonRootObject, "response_code");
    //result->StatusMessage = PhGetJsonValueAsString(jsonRootObject, "verbose_msg");
    //result->PermaLink = PhGetJsonValueAsString(jsonRootObject, "permalink");
    //result->ScanId = PhGetJsonValueAsString(jsonRootObject, "scan_id");

CleanupExit:

    if (httpContext)
        PhHttpSocketDestroy(httpContext);

    if (jsonRootObject)
        PhFreeJsonObject(jsonRootObject);

    PhClearReference(&jsonString);
    PhClearReference(&versionString);
    PhClearReference(&userAgentString);

    return result;
}

NTSTATUS NTAPI VirusTotalProcessApiThread(
    _In_ PVOID Parameter
    )
{
    // TODO: Workqueue support.
    PhSetThreadBasePriority(NtCurrentThread(), THREAD_PRIORITY_LOWEST);
    PhSetThreadIoPriority(NtCurrentThread(), IoPriorityVeryLow);

    PhDelayExecution(10 * 1000);

    do
    {
        ULONG i;
        INT64 resultLength;
        PPH_BYTES jsonApiResult = NULL;
        PVOID jsonArray;
        PVOID rootJsonObject = NULL;
        PVOID dataJsonObject;
        PPH_STRING jsonArrayToSendString = NULL;
        PPH_LIST resultTempList = NULL;
        PPH_LIST virusTotalResults = NULL;

        jsonArray = PhCreateJsonArray();
        resultTempList = PhCreateList(30);

        PhAcquireQueuedLockExclusive(&ProcessListLock);

        for (i = 0; i < VirusTotalList->Count; i++)
        {
            PVIRUSTOTAL_FILE_HASH_ENTRY extension = VirusTotalList->Items[i];

            if (resultTempList->Count >= 30)
                break;

            if (!extension->Stage1)
            {
                extension->Stage1 = TRUE;

                PhAddItemList(resultTempList, extension);
            }
        }

        PhReleaseQueuedLockExclusive(&ProcessListLock);

        if (resultTempList->Count == 0)
        {
            PhDelayExecution(30 * 1000); // Wait 30 seconds
            goto CleanupExit;
        }

        for (i = 0; i < resultTempList->Count; i++)
        {
            VirusTotalBuildJsonArray(resultTempList->Items[i], jsonArray);
        }

        if (!(jsonArrayToSendString = PhGetJsonArrayString(jsonArray, TRUE)))
            goto CleanupExit;

        if (!(jsonApiResult = VirusTotalSendHttpRequest(PhConvertUtf16ToUtf8(jsonArrayToSendString->Buffer))))
            goto CleanupExit;

        if (!(rootJsonObject = PhCreateJsonParser(jsonApiResult->Buffer)))
            goto CleanupExit;

        if (!(dataJsonObject = PhGetJsonObject(rootJsonObject, "data")))
            goto CleanupExit;

        if (!(resultLength = PhGetJsonValueAsUInt64(rootJsonObject, "result")))
            goto CleanupExit;

        if (virusTotalResults = VirusTotalJsonToResultList(dataJsonObject))
        {
            for (i = 0; i < virusTotalResults->Count; i++)
            {
                PVIRUSTOTAL_API_RESULT result = virusTotalResults->Items[i];

                if (result->Found)
                {
                    PVIRUSTOTAL_FILE_HASH_ENTRY entry = VirusTotalGetCachedResultFromHash(result->FileHash);

                    if (entry && !entry->Processed)
                    {
                        entry->Processed = TRUE;
                        entry->Found = result->Found;
                        entry->Positives = result->Positives;
                        entry->Total = result->Total;

                        if (!FindProcessDbObject(&entry->FileName->sr))
                        {
                            CreateProcessDbObject(
                                entry->FileName,
                                entry->Positives,
                                entry->Total,
                                result->FileHash
                                );
                        }
                    }
                }
            }
        }

CleanupExit:

        if (virusTotalResults)
        {
            for (i = 0; i < virusTotalResults->Count; i++)
            {
                PVIRUSTOTAL_API_RESULT result = virusTotalResults->Items[i];

            //    PhClearReference(&result->Permalink);
            //    PhClearReference(&result->FileHash);
            //    PhClearReference(&result->DetectionRatio);

                PhFree(result);
            }

            PhDereferenceObject(virusTotalResults);
        }

        if (rootJsonObject)
        {
            PhFreeJsonObject(rootJsonObject);
        }

        if (jsonArrayToSendString)
            PhDereferenceObject(jsonArrayToSendString);

        if (jsonArray)
        {
            PhFreeJsonObject(jsonArray);
        }

        if (jsonApiResult)
        {
            PhDereferenceObject(jsonApiResult);
        }

        if (resultTempList)
        {
            // Re-queue items without any results from VirusTotal.
            //for (i = 0; i < resultTempList->Count; i++)
            //{
            //    PVIRUSTOTAL_FILE_HASH_ENTRY result = resultTempList->Items[i];
            //    PPROCESS_EXTENSION extension = result->Extension;
            //
            //    if (extension->Retries > 3)
            //        continue;
            //
            //    if (PhIsNullOrEmptyString(result->FileResult))
            //    {
            //        extension->Stage1 = FALSE;
            //    }
            //
            //    extension->Retries++;
            //}

            PhDereferenceObject(resultTempList);
        }

        PhDelayExecution(5 * 1000); // Wait 5 seconds

    } while (VirusTotalHandle);

    return STATUS_SUCCESS;
}

VOID InitializeVirusTotalProcessMonitor(
    VOID
    )
{
    VirusTotalList = PhCreateList(100);

    PhCreateThreadEx(&VirusTotalHandle, VirusTotalProcessApiThread, NULL);
}

VOID CleanupVirusTotalProcessMonitor(
    VOID
    )
{
    if (VirusTotalHandle)
    {
        NtClose(VirusTotalHandle);
        VirusTotalHandle = NULL;
    }

    if (VirusTotalList)
    {
        PhDereferenceObject(VirusTotalList);
    }
}

// NOTE: This function does not use the SCM due to major performance issues.
// For now just query this information from the registry but it might be out-of-sync
// with any recent services changes until the SCM flushes its cache.
NTSTATUS QueryServiceFileName(
    _In_ PPH_STRINGREF ServiceName,
    _Out_ PPH_STRING *ServiceFileName,
    _Out_ PPH_STRING *ServiceBinaryPath
    )
{
    static PH_STRINGREF servicesKeyName = PH_STRINGREF_INIT(L"System\\CurrentControlSet\\Services\\");
    NTSTATUS status;
    HANDLE keyHandle;
    ULONG serviceType = 0;
    PPH_STRING keyName;
    PPH_STRING binaryPath;
    PPH_STRING fileName;

    keyName = PhConcatStringRef2(&servicesKeyName, ServiceName);
    binaryPath = NULL;
    fileName = NULL;

    if (NT_SUCCESS(status = PhOpenKey(
        &keyHandle,
        KEY_READ,
        PH_KEY_LOCAL_MACHINE,
        &keyName->sr,
        0
        )))
    {
        PPH_STRING serviceImagePath;

        serviceType = PhQueryRegistryUlongZ(keyHandle, L"Type");

        if (serviceImagePath = PhQueryRegistryStringZ(keyHandle, L"ImagePath"))
        {
            PPH_STRING expandedString;

            if (expandedString = PhExpandEnvironmentStrings(&serviceImagePath->sr))
            {
                binaryPath = expandedString;
                PhDereferenceObject(serviceImagePath);
            }
            else
            {
                binaryPath = serviceImagePath;
            }
        }
        else
        {
            status = STATUS_NOT_FOUND;
        }

        NtClose(keyHandle);
    }

    if (NT_SUCCESS(status))
    {
        PhGetServiceDllParameter(serviceType, ServiceName, &fileName);

        if (!fileName)
        {
            if (serviceType & SERVICE_WIN32)
            {
                PH_STRINGREF dummyFileName;
                PH_STRINGREF dummyArguments;

                PhParseCommandLineFuzzy(&binaryPath->sr, &dummyFileName, &dummyArguments, &fileName);

                if (!fileName)
                    PhSwapReference(&fileName, binaryPath);
            }
            else
            {
                fileName = PhGetFileName(binaryPath);
            }
        }

        *ServiceFileName = fileName;
        *ServiceBinaryPath = binaryPath;
    }
    else
    {
        if (binaryPath)
            PhDereferenceObject(binaryPath);
    }

    PhDereferenceObject(keyName);

    return status;
}
