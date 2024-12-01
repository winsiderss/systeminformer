/*
 * Copyright (c) 2022 Winsider Seminars & Solutions, Inc.  All rights reserved.
 *
 * This file is part of System Informer.
 *
 * Authors:
 *
 *     dmex    2021
 *
 */

#include <ph.h>
#include <memory>
#include "tlsh.h"
#include "tlsh_wrapper.h"

BOOLEAN PvGetTlshBufferHash(
    _In_ PVOID Buffer,
    _In_ SIZE_T BufferLength,
    _Out_ char** HashResult
    )
{
    if (BufferLength < MIN_DATA_LENGTH)
        return FALSE;

    auto tlshHash = std::make_unique<Tlsh>();

    if (BufferLength >= UINT_MAX)
    {
        PBYTE address = static_cast<PBYTE>(Buffer);
        SIZE_T numberOfBytes = BufferLength;

        // Chunk the data into smaller blocks when the buffer length
        // overflows the maximum length of the TLSH library.
        while (numberOfBytes != 0)
        {
            const ULONG blockSize = static_cast<ULONG>(std::min(numberOfBytes, static_cast<SIZE_T>(PAGE_SIZE)));

            tlshHash->update(address, blockSize);

            address += blockSize;
            numberOfBytes -= blockSize;
        }

        tlshHash->final();
    }
    else
    {
        tlshHash->final(
            static_cast<const unsigned char*>(Buffer),
            static_cast<unsigned int>(BufferLength),
            0
            );
    }

    if (!tlshHash->isValid())
        return FALSE;

    const char* tlshHashString = tlshHash->getHash(TRUE);

    if (tlshHashString == nullptr || tlshHashString[0] == ANSI_NULL)
        return FALSE;

    *HashResult = _strdup(tlshHashString);

    return TRUE;
}

BOOLEAN PvGetTlshFileHash(
    _In_ HANDLE FileHandle,
    _Out_ char** HashResult
    )
{
    const char* tlshHashString = nullptr;
    NTSTATUS status;
    IO_STATUS_BLOCK isb;
    ULONGLONG bytesRemaining;
    FILE_STANDARD_INFORMATION standardInfo;
    BYTE buffer[PAGE_SIZE];

    status = NtQueryInformationFile(
        FileHandle,
        &isb,
        &standardInfo,
        sizeof(FILE_STANDARD_INFORMATION),
        FileStandardInformation
        );

    if (!NT_SUCCESS(status))
        return FALSE;

    auto tlshHash = std::make_unique<Tlsh>();

    bytesRemaining = standardInfo.EndOfFile.QuadPart;

    while (bytesRemaining)
    {
        status = NtReadFile(
            FileHandle,
            nullptr,
            nullptr,
            nullptr,
            &isb,
            buffer,
            sizeof(buffer),
            nullptr,
            nullptr
            );

        if (!NT_SUCCESS(status))
            break;

        tlshHash->update(buffer, static_cast<UINT32>(isb.Information));

        bytesRemaining -= isb.Information;
    }

    tlshHash->final();

    if (!tlshHash->isValid())
        return FALSE;

    tlshHashString = tlshHash->getHash(TRUE);

    if (tlshHashString == nullptr || tlshHashString[0] == ANSI_NULL)
        return FALSE;

    *HashResult = _strdup(tlshHashString);

    return TRUE;
}
