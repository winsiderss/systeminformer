/*
 * Process Hacker Online Checks -
 *   VirusTotal functions
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

    result = FormatAnsiString(
        "%S %S", 
        dateString->Buffer,
        timeString->Buffer
        );

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

    swscanf(
        PhGetString(Time),
        L"%hu-%hu-%hu %hu:%hu:%hu",
        &time.wYear,
        &time.wMonth,
        &time.wDay,
        &time.wHour,
        &time.wMinute,
        &time.wSecond
        );

    if (SystemTimeToTzSpecificLocalTime(NULL, &time, &localTime))
    {
        result = PhFormatDateTime(&localTime);
    }

    return result;
}

PVIRUSTOTAL_FILE_HASH_ENTRY VirusTotalAddCacheResult(
    _In_ PPH_STRING FileName,
    _In_ PPROCESS_EXTENSION Extension
    )
{
    PVIRUSTOTAL_FILE_HASH_ENTRY result;
    
    result = PhAllocate(sizeof(VIRUSTOTAL_FILE_HASH_ENTRY));
    memset(result, 0, sizeof(VIRUSTOTAL_FILE_HASH_ENTRY));

    PhReferenceObject(FileName);
    result->FileName = FileName;
    result->FileNameAnsi = PhConvertUtf16ToMultiByte(PhGetString(FileName));
    result->Extension = Extension;

    PhAcquireQueuedLockExclusive(&ProcessListLock);
    PhAddItemList(VirusTotalList, result);
    PhReleaseQueuedLockExclusive(&ProcessListLock);

    return result;
}

VOID VirusTotalRemoveCacheResult(_In_ PPROCESS_EXTENSION Extension)
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
        PhClearReference(&extension->FileResult);
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

PPH_LIST VirusTotalJsonToResultList(
    _In_ PVOID JsonObject
    )
{
    ULONG i;
    ULONG arrayLength;
    PPH_LIST results;

    if (!(arrayLength = JsonGetArrayLength(JsonObject)))
        return NULL;

    results = PhCreateList(arrayLength);

    for (i = 0; i < arrayLength; i++)
    {
        PVIRUSTOTAL_API_RESULT result;
        PVOID jsonArrayObject;
        PSTR fileLink;
        PSTR fileHash;
        PSTR fileRatio;

        if (!(jsonArrayObject = JsonGetObjectArrayIndex(JsonObject, i)))
            continue;

        result = PhAllocate(sizeof(VIRUSTOTAL_API_RESULT));
        memset(result, 0, sizeof(VIRUSTOTAL_API_RESULT));

        fileLink = GetJsonValueAsString(jsonArrayObject, "permalink");
        fileHash = GetJsonValueAsString(jsonArrayObject, "hash");
        fileRatio = GetJsonValueAsString(jsonArrayObject, "detection_ratio");

        result->Found = GetJsonValueAsBool(jsonArrayObject, "found") == TRUE;
        result->Positives = GetJsonValueAsUlong(jsonArrayObject, "positives");
        result->Total = GetJsonValueAsUlong(jsonArrayObject, "total");
        result->Permalink = fileLink ? PhZeroExtendToUtf16(fileLink) : PhReferenceEmptyString();
        result->FileHash = fileHash ? PhZeroExtendToUtf16(fileHash) : PhReferenceEmptyString();
        result->DetectionRatio = fileRatio ? PhZeroExtendToUtf16(fileRatio) : PhReferenceEmptyString();

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
        0,
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
            Entry->FileHashAnsi = PhConvertUtf16ToMultiByte(Entry->FileHash->Buffer);

            entry = CreateJsonObject();
            JsonAddObject(entry, "autostart_location", "");
            JsonAddObject(entry, "autostart_entry", "");
            JsonAddObject(entry, "hash", Entry->FileHashAnsi->Buffer);
            JsonAddObject(entry, "image_path", Entry->FileNameAnsi->Buffer);
            JsonAddObject(entry, "creation_datetime", Entry->CreationTime ? Entry->CreationTime->Buffer : "");
            JsonArrayAddObject(JsonArray, entry);
        }

        NtClose(fileHandle);
    }
}

PSTR VirusTotalSendHttpRequest(
    _In_ PPH_BYTES JsonArray
    )
{
    HANDLE fileHandle = INVALID_HANDLE_VALUE;
    HINTERNET httpSessionHandle = NULL;
    HINTERNET connectHandle = NULL;
    HINTERNET requestHandle = NULL;
    PSTR subRequestBuffer = NULL;
    PPH_STRING phVersion = NULL;
    PPH_STRING userAgent = NULL;
    PPH_STRING keyString = NULL;
    PPH_STRING urlString = NULL;
    WINHTTP_CURRENT_USER_IE_PROXY_CONFIG proxyConfig = { 0 };

    phVersion = PhGetPhVersion();
    userAgent = PhConcatStrings2(L"ProcessHacker_", phVersion->Buffer);

#ifdef _DEBUG
    WinHttpGetIEProxyConfigForCurrentUser(&proxyConfig);
#endif

    if (!(httpSessionHandle = WinHttpOpen(
        userAgent->Buffer,
        proxyConfig.lpszProxy ? WINHTTP_ACCESS_TYPE_NAMED_PROXY : WINHTTP_ACCESS_TYPE_DEFAULT_PROXY,
        proxyConfig.lpszProxy,
        proxyConfig.lpszProxyBypass,
        0
        )))
    {
        goto CleanupExit;
    }

    if (WindowsVersion >= WINDOWS_8_1)
    {
        ULONG gzipFlags = WINHTTP_DECOMPRESSION_FLAG_GZIP | WINHTTP_DECOMPRESSION_FLAG_DEFLATE;

        WinHttpSetOption(httpSessionHandle, WINHTTP_OPTION_DECOMPRESSION, &gzipFlags, sizeof(ULONG));
    }

    if (!(connectHandle = WinHttpConnect(
        httpSessionHandle,
        L"www.virustotal.com",
        INTERNET_DEFAULT_HTTPS_PORT,
        0
        )))
    {
        goto CleanupExit;
    }

    keyString = PhCreateString(VIRUSTOTAL_APIKEY);
    urlString = PhFormatString(
        L"/partners/sysinternals/file-reports?apikey=%s",
        keyString->Buffer
        );

    if (!(requestHandle = WinHttpOpenRequest(
        connectHandle,
        L"POST",
        PhGetStringOrEmpty(urlString),
        NULL,
        WINHTTP_NO_REFERER,
        WINHTTP_DEFAULT_ACCEPT_TYPES,
        WINHTTP_FLAG_SECURE
        )))
    {
        PhClearReference(&keyString);
        PhClearReference(&urlString);
        goto CleanupExit;
    }

    PhClearReference(&keyString);
    PhClearReference(&urlString);

    if (!WinHttpAddRequestHeaders(requestHandle, L"Content-Type: application/json", -1L, 0))
    {
        goto CleanupExit;
    }

    if (!WinHttpSendRequest(
        requestHandle, 
        WINHTTP_NO_ADDITIONAL_HEADERS, 
        0, 
        JsonArray->Buffer, 
        (ULONG)JsonArray->Length, 
        (ULONG)JsonArray->Length,
        0
        ))
    {
        goto CleanupExit;
    }

    if (WinHttpReceiveResponse(requestHandle, NULL))
    {
        BYTE buffer[PAGE_SIZE];
        ULONG allocatedLength;
        ULONG dataLength;
        ULONG returnLength;

        allocatedLength = sizeof(buffer);
        subRequestBuffer = PhAllocate(allocatedLength);
        dataLength = 0;

        while (WinHttpReadData(requestHandle, buffer, PAGE_SIZE, &returnLength))
        {
            if (returnLength == 0)
                break;

            if (allocatedLength < dataLength + returnLength)
            {
                allocatedLength *= 2;
                subRequestBuffer = PhReAllocate(subRequestBuffer, allocatedLength);
            }

            memcpy(subRequestBuffer + dataLength, buffer, returnLength);
            dataLength += returnLength;
        }

        if (allocatedLength < dataLength + 1)
        {
            allocatedLength++;
            subRequestBuffer = PhReAllocate(subRequestBuffer, allocatedLength);
        }

        // Ensure that the buffer is null-terminated.
        subRequestBuffer[dataLength] = 0;
    }

CleanupExit:

    if (requestHandle)
        WinHttpCloseHandle(requestHandle);

    if (connectHandle)
        WinHttpCloseHandle(connectHandle);

    if (httpSessionHandle)
        WinHttpCloseHandle(httpSessionHandle);

    if (JsonArray)
        PhDereferenceObject(JsonArray);

    return subRequestBuffer;
}

PVIRUSTOTAL_FILE_REPORT_RESULT VirusTotalSendHttpFileReportRequest(
    _In_ PPH_STRING FileHash
    )
{
    NTSTATUS status = STATUS_SUCCESS;
    HANDLE fileHandle = INVALID_HANDLE_VALUE;
    HINTERNET httpSessionHandle = NULL;
    HINTERNET connectHandle = NULL;
    HINTERNET requestHandle = NULL;
    PSTR subRequestBuffer = NULL;
    PPH_STRING phVersion = NULL;
    PPH_STRING userAgent = NULL;
    PPH_STRING keyString = NULL;
    PPH_STRING urlString = NULL;
    WINHTTP_CURRENT_USER_IE_PROXY_CONFIG proxyConfig = { 0 };

    phVersion = PhGetPhVersion();
    userAgent = PhConcatStrings2(L"ProcessHacker_", phVersion->Buffer);

#ifdef _DEBUG
    WinHttpGetIEProxyConfigForCurrentUser(&proxyConfig);
#endif

    if (!(httpSessionHandle = WinHttpOpen(
        userAgent->Buffer,
        proxyConfig.lpszProxy ? WINHTTP_ACCESS_TYPE_NAMED_PROXY : WINHTTP_ACCESS_TYPE_DEFAULT_PROXY,
        proxyConfig.lpszProxy,
        proxyConfig.lpszProxyBypass,
        0
        )))
    {
        goto CleanupExit;
    }

    if (WindowsVersion >= WINDOWS_8_1)
    {
        ULONG httpFlags = WINHTTP_DECOMPRESSION_FLAG_GZIP | WINHTTP_DECOMPRESSION_FLAG_DEFLATE;
        WinHttpSetOption(httpSessionHandle, WINHTTP_OPTION_DECOMPRESSION, &httpFlags, sizeof(ULONG));
    }

    if (!(connectHandle = WinHttpConnect(
        httpSessionHandle,
        L"www.virustotal.com",
        INTERNET_DEFAULT_HTTPS_PORT,
        0
        )))
    {
        goto CleanupExit;
    }

    keyString = PhCreateString(VIRUSTOTAL_APIKEY);
    urlString = PhFormatString(
        L"/vtapi/v2/file/report?apikey=%s&resource=%s",
        keyString->Buffer,
        PhGetStringOrEmpty(FileHash)
        );

    if (!(requestHandle = WinHttpOpenRequest(
        connectHandle,
        L"POST",
        PhGetStringOrEmpty(urlString),
        NULL,
        WINHTTP_NO_REFERER,
        WINHTTP_DEFAULT_ACCEPT_TYPES,
        WINHTTP_FLAG_SECURE
        )))
    {
        PhClearReference(&keyString);
        PhClearReference(&urlString);
        goto CleanupExit;
    }

    PhClearReference(&keyString);
    PhClearReference(&urlString);

    if (!WinHttpAddRequestHeaders(requestHandle, L"Content-Type: application/json", -1L, 0))
        goto CleanupExit;

    if (!WinHttpSendRequest(
        requestHandle,
        WINHTTP_NO_ADDITIONAL_HEADERS,
        0,
        WINHTTP_NO_REQUEST_DATA,
        0,
        WINHTTP_IGNORE_REQUEST_TOTAL_LENGTH,
        0
        ))
    {
        goto CleanupExit;
    }

    if (WinHttpReceiveResponse(requestHandle, NULL))
    {
        BYTE buffer[PAGE_SIZE];
        ULONG allocatedLength;
        ULONG dataLength;
        ULONG returnLength;

        allocatedLength = sizeof(buffer);
        subRequestBuffer = PhAllocate(allocatedLength);
        dataLength = 0;

        while (WinHttpReadData(requestHandle, buffer, PAGE_SIZE, &returnLength))
        {
            if (returnLength == 0)
                break;

            if (allocatedLength < dataLength + returnLength)
            {
                allocatedLength *= 2;
                subRequestBuffer = PhReAllocate(subRequestBuffer, allocatedLength);
            }

            memcpy(subRequestBuffer + dataLength, buffer, returnLength);
            dataLength += returnLength;
        }

        if (allocatedLength < dataLength + 1)
        {
            allocatedLength++;
            subRequestBuffer = PhReAllocate(subRequestBuffer, allocatedLength);
        }

        subRequestBuffer[dataLength] = 0;
    }

CleanupExit:

    if (requestHandle)
        WinHttpCloseHandle(requestHandle);

    if (connectHandle)
        WinHttpCloseHandle(connectHandle);

    if (httpSessionHandle)
        WinHttpCloseHandle(httpSessionHandle);


    PVOID jsonRootObject;
    PVOID jsonScanObject;
    PVIRUSTOTAL_FILE_REPORT_RESULT result;

    if (!(jsonRootObject = CreateJsonParser(subRequestBuffer)))
        goto CleanupExit;

    if (!GetJsonValueAsUlong(jsonRootObject, "response_code"))
        goto CleanupExit;

    result = PhAllocate(sizeof(VIRUSTOTAL_FILE_REPORT_RESULT));
    memset(result, 0, sizeof(VIRUSTOTAL_FILE_REPORT_RESULT));

    result->Total = PhFormatUInt64(GetJsonValueAsUlong(jsonRootObject, "total"), FALSE);
    result->Positives = PhFormatUInt64(GetJsonValueAsUlong(jsonRootObject, "positives"), FALSE);
    result->Resource = PhZeroExtendToUtf16(GetJsonValueAsString(jsonRootObject, "resource"));
    result->ScanId = PhZeroExtendToUtf16(GetJsonValueAsString(jsonRootObject, "scan_id"));
    result->Md5 = PhZeroExtendToUtf16(GetJsonValueAsString(jsonRootObject, "md5"));
    result->Sha1 = PhZeroExtendToUtf16(GetJsonValueAsString(jsonRootObject, "sha1"));
    result->Sha256 = PhZeroExtendToUtf16(GetJsonValueAsString(jsonRootObject, "sha256"));
    result->ScanDate = PhZeroExtendToUtf16(GetJsonValueAsString(jsonRootObject, "scan_date"));
    result->Permalink = PhZeroExtendToUtf16(GetJsonValueAsString(jsonRootObject, "permalink"));
    result->StatusMessage = PhZeroExtendToUtf16(GetJsonValueAsString(jsonRootObject, "verbose_msg"));

    if (jsonScanObject = JsonGetObject(jsonRootObject, "scans"))
    {
        PPH_LIST jsonArrayList;

        if (jsonArrayList = JsonGetObjectArrayList(jsonScanObject))
        {
            result->ScanResults = PhCreateList(jsonArrayList->Count);

            for (ULONG i = 0; i < jsonArrayList->Count; i++)
            {
                PJSON_ARRAY_LIST_OBJECT object = jsonArrayList->Items[i];
                //BOOLEAN detected = GetJsonValueAsBool(object->Entry, "detected") == TRUE;
                //PSTR version = GetJsonValueAsString(object->Entry, "version");
                //PSTR result = GetJsonValueAsString(object->Entry, "result");
                //PSTR update = GetJsonValueAsString(object->Entry, "update");

                PhFree(object);
            }

            PhDereferenceObject(jsonArrayList);
        }
    }

    return result;
}

NTSTATUS NTAPI VirusTotalProcessApiThread(
    _In_ PVOID Parameter
    )
{
    LONG priority;
    IO_PRIORITY_HINT ioPriority;

    // TODO: Workqueue support.
    priority = THREAD_PRIORITY_LOWEST;
    ioPriority = IoPriorityVeryLow;

    NtSetInformationThread(NtCurrentThread(), ThreadBasePriority, &priority, sizeof(LONG));
    NtSetInformationThread(NtCurrentThread(), ThreadIoPriority, &ioPriority, sizeof(IO_PRIORITY_HINT));

    Sleep(10 * 1000);

    do
    {
        ULONG i;
        INT64 resultLength;
        PSTR jsonArrayToSendString;
        PSTR jsonApiResult = NULL;
        PVOID jsonArray;
        PVOID rootJsonObject = NULL;
        PVOID dataJsonObject;
        PPH_LIST resultTempList = NULL;
        PPH_LIST virusTotalResults = NULL;

        jsonArray = CreateJsonArray();
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
            Sleep(30 * 1000); // Wait 30 seconds
            goto CleanupExit;
        }

        for (i = 0; i < resultTempList->Count; i++)
        {
            VirusTotalBuildJsonArray(resultTempList->Items[i], jsonArray);
        }

        if (!(jsonArrayToSendString = GetJsonArrayString(jsonArray)))
            goto CleanupExit;

        if (!(jsonApiResult = VirusTotalSendHttpRequest(PhCreateBytes(jsonArrayToSendString))))
            goto CleanupExit;

        if (!(rootJsonObject = CreateJsonParser(jsonApiResult)))
            goto CleanupExit;

        if (!(dataJsonObject = JsonGetObject(rootJsonObject, "data")))
            goto CleanupExit;

        if (!(resultLength = GetJsonValueAsUlong(rootJsonObject, "result")))
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

                        PhMoveReference(&entry->FileResult, PhDuplicateString(result->DetectionRatio));

                        if (!FindProcessDbObject(&entry->FileName->sr))
                        {
                            CreateProcessDbObject(
                                entry->FileName,
                                entry->Positives,
                                result->FileHash,
                                entry->FileResult
                                );
                        }
                    }
                }
                else
                {

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
            //
            //    PhFree(result);
            }
            
            //PhDereferenceObject(virusTotalResults);
        }
        
        if (rootJsonObject)
        {
            CleanupJsonParser(rootJsonObject);
        }

        if (jsonArray)
        {
            CleanupJsonParser(jsonArray);
        }

        if (jsonApiResult)
        {
            PhFree(jsonApiResult);
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

        Sleep(5 * 1000); // Wait 5 seconds

    } while (VirusTotalHandle);

    return STATUS_SUCCESS;
}

VOID InitializeVirusTotalProcessMonitor(
    VOID
    )
{
    VirusTotalList = PhCreateList(100);
    VirusTotalHandle = PhCreateThread(0, VirusTotalProcessApiThread, NULL);
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
    static PH_STRINGREF typeKeyName = PH_STRINGREF_INIT(L"Type");

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
        PKEY_VALUE_PARTIAL_INFORMATION buffer;

        if (NT_SUCCESS(status = PhQueryValueKey(
            keyHandle,
            &typeKeyName,
            KeyValuePartialInformation,
            &buffer
            )))
        {
            if (
                buffer->Type == REG_DWORD &&
                buffer->DataLength == sizeof(ULONG)
                )
            {
                serviceType = *(PULONG)buffer->Data;
            }

            PhFree(buffer);
        }

        if (serviceImagePath = PhQueryRegistryString(keyHandle, L"ImagePath"))
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
        PhGetServiceDllParameter(ServiceName, &fileName);

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