/*
 * Process Hacker Online Checks -
 *   database functions
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

PVIRUSTOTAL_FILE_HASH_ENTRY VirusTotalAddCacheResult(
    _In_ PPH_PROCESS_ITEM ProcessItem,
    _In_ PPROCESS_EXTENSION Extension
    )
{
    PVIRUSTOTAL_FILE_HASH_ENTRY result;
    
    result = PhAllocate(sizeof(VIRUSTOTAL_FILE_HASH_ENTRY));
    memset(result, 0, sizeof(VIRUSTOTAL_FILE_HASH_ENTRY));

    result->Extension = Extension;
    result->FileName = PhDuplicateString(ProcessItem->FileName);

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
        result->Permalink = fileLink ? PhConvertUtf8ToUtf16(fileLink) : PhReferenceEmptyString();
        result->FileHash = fileHash ? PhConvertUtf8ToUtf16(fileHash) : PhReferenceEmptyString();
        result->DetectionRatio = fileRatio ? PhConvertUtf8ToUtf16(fileRatio) : PhReferenceEmptyString();

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
    UCHAR hash[32];

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
            hash
            )))
        {
            PVOID entry;
            
            Entry->FileHash = PhBufferToHexString(hash, 32);
            Entry->FileHashAnsi = PhConvertUtf16ToMultiByte(PhGetStringOrEmpty(Entry->FileHash));
            Entry->FileNameAnsi = PhConvertUtf16ToMultiByte(PhGetStringOrEmpty(Entry->FileName));

            entry = CreateJsonObject();
            JsonAddObject(entry, "autostart_location", "");
            JsonAddObject(entry, "autostart_entry", "");
            JsonAddObject(entry, "hash", Entry->FileHashAnsi->Buffer);
            JsonAddObject(entry, "image_path", Entry->FileNameAnsi->Buffer);
            JsonAddObject(entry, "creation_datetime", Entry->CreationTime->Buffer);
            JsonArrayAddObject(JsonArray, entry);
        }

        NtClose(fileHandle);
    }
}

PSTR VirusTotalSendHttpRequest(
    _In_ PPH_BYTES JsonArray
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
    WINHTTP_CURRENT_USER_IE_PROXY_CONFIG proxyConfig = { 0 };

    phVersion = PhGetPhVersion();
    userAgent = PhConcatStrings2(L"ProcessHacker_", phVersion->Buffer);
    WinHttpGetIEProxyConfigForCurrentUser(&proxyConfig);

    if (!(httpSessionHandle = WinHttpOpen(
        userAgent->Buffer,
        proxyConfig.lpszProxy ? WINHTTP_ACCESS_TYPE_NAMED_PROXY : WINHTTP_ACCESS_TYPE_DEFAULT_PROXY,
        proxyConfig.lpszProxy,
        proxyConfig.lpszProxyBypass,
        0
        )))
    {
        dprintf("WinHttpOpen: %lu", GetLastError());
        goto CleanupExit;
    }

    if (WindowsVersion >= WINDOWS_8_1)
    {
        ULONG httpFlags = WINHTTP_DECOMPRESSION_FLAG_GZIP | WINHTTP_DECOMPRESSION_FLAG_DEFLATE;
        // Enable GZIP and DEFLATE support on Windows 8.1 and above using undocumented flags.
        WinHttpSetOption(httpSessionHandle, WINHTTP_OPTION_DECOMPRESSION, &httpFlags, sizeof(ULONG));
    }

    if (!(connectHandle = WinHttpConnect(
        httpSessionHandle,
        L"www.virustotal.com",
        INTERNET_DEFAULT_HTTPS_PORT,
        0
        )))
    {
        dprintf("WinHttpConnect: %lu", GetLastError());
        goto CleanupExit;
    }

    if (!(requestHandle = WinHttpOpenRequest(
        connectHandle,
        L"POST",
        VIRUSTOTAL_URLPATH VIRUSTOTAL_APIKEY,
        NULL,
        WINHTTP_NO_REFERER,
        WINHTTP_DEFAULT_ACCEPT_TYPES,
        WINHTTP_FLAG_SECURE
        )))
    {
        dprintf("WinHttpOpenRequest: %lu", GetLastError());
        goto CleanupExit;
    }

    if (!WinHttpAddRequestHeaders(requestHandle, L"Content-Type: application/json", -1L, 0))
    {
        dprintf("WinHttpAddRequestHeaders: %lu", GetLastError());
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
        dprintf("WinHttpSendRequest: %lu", GetLastError());
        goto CleanupExit;
    }

    if (!WinHttpReceiveResponse(requestHandle, NULL))
    {
        PhFormatString(L"WinHttpReceiveResponse: %lu", GetLastError());
        goto CleanupExit;
    }
    else
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

    return subRequestBuffer;
}

NTSTATUS NTAPI VirusTotalProcessApiThread(
    _In_ PVOID Parameter
    )
{
    LONG increment;
    IO_PRIORITY_HINT ioPriority;

    // TODO: Workqueue support.
    increment = THREAD_PRIORITY_LOWEST;
    ioPriority = IoPriorityVeryLow;

    NtSetInformationThread(NtCurrentThread(), ThreadBasePriority, &increment, sizeof(LONG));
    NtSetInformationThread(NtCurrentThread(), ThreadIoPriority, &ioPriority, sizeof(IO_PRIORITY_HINT));

    Sleep(5 * 1000);

    do
    {
        ULONG i;
        INT64 resultLength;
        PSTR jsonArrayToSendString;
        PSTR jsonApiResult = NULL;
        PVOID jsonArray;
        PVOID rootJsonObject;
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
            goto CleanupExit;

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

        if ((resultLength = GetJsonValueAsUlong(rootJsonObject, "result")) < 1)
        {           
            //PSTR message = GetJsonValueAsString(rootJsonObject, "message"); // "message": "Illegitimate request", 
            goto CleanupExit;
        }

        if (virusTotalResults = VirusTotalJsonToResultList(dataJsonObject))
        {
            for (i = 0; i < virusTotalResults->Count; i++)
            {
                PVIRUSTOTAL_API_RESULT result = virusTotalResults->Items[i];
                PVIRUSTOTAL_FILE_HASH_ENTRY entry = VirusTotalGetCachedResultFromHash(result->FileHash);
 
                if (entry && !entry->Processed)
                {
                    entry->Positives = result->Positives;

                    PhSwapReference(
                        &entry->FileResult, 
                        PhDuplicateString(result->DetectionRatio)
                        );

                    if (!FindProcessDbObject(&entry->FileName->sr))
                    {
                        CreateProcessDbObject(
                            entry->FileName, 
                            entry->Positives, 
                            result->FileHash, 
                            entry->FileResult
                            );
                    }

                    entry->Processed = TRUE;
                }
            }
        }

CleanupExit:

        if (virusTotalResults)
        {
            for (i = 0; i < virusTotalResults->Count; i++)
            {
                PVIRUSTOTAL_API_RESULT result = virusTotalResults->Items[i];

                PhDereferenceObject(result->Permalink);
                PhDereferenceObject(result->FileHash);
                PhDereferenceObject(result->DetectionRatio);

                PhFree(result);
            }

            PhDereferenceObject(virusTotalResults);
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
            for (i = 0; i < resultTempList->Count; i++)
            {
                PVIRUSTOTAL_FILE_HASH_ENTRY result = resultTempList->Items[i];
                PPROCESS_EXTENSION extension = result->Extension;

                if (extension->Retries > 3)
                    continue;

                if (PhIsNullOrEmptyString(result->FileResult))
                {
                    extension->Stage1 = FALSE;
                }

                extension->Retries++;
            }

            PhDereferenceObject(resultTempList);
        }

        // Wait 10 seconds before checking list again
        Sleep(10 * 1000);

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