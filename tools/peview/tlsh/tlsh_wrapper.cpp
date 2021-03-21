/*
 * Process Hacker -
 *   PE viewer
 *
 * Copyright (C) 2021 dmex
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

#include <ph.h>
#include <memory>
#include "tlsh.h"
#include "tlsh_wrapper.h"

BOOLEAN PvGetTlshBufferHash(
    _In_ PVOID Buffer,
    _In_ SIZE_T BufferLength,
    _Out_ PPH_STRING* HashResult
    )
{
    const char* tlshHashString = nullptr;

    if (BufferLength < MIN_DATA_LENGTH)
        return FALSE;

    auto tlshHash = std::make_unique<Tlsh>();

    if (BufferLength >= UINT_MAX)
    {
        PBYTE address;
        SIZE_T numberOfBytes;
        ULONG blockSize;
        BYTE buffer[PAGE_SIZE];

        // Chunk the data into smaller blocks when the buffer length
        // overflows the maximum length of the TLSH library.

        address = (PBYTE)Buffer;
        numberOfBytes = BufferLength;
        blockSize = sizeof(buffer);

        while (numberOfBytes != 0)
        {
            if (blockSize > numberOfBytes)
                blockSize = (ULONG)numberOfBytes;

            try
            {
                memcpy(buffer, address, blockSize);
            }
            catch (...)
            {
                break;
            }

            tlshHash->update(buffer, blockSize);

            address += blockSize;
            numberOfBytes -= blockSize;
        }

        tlshHash->final();
    }
    else
    {
        tlshHash->final(
            (const unsigned char*)Buffer,
            (unsigned int)BufferLength,
            0
            );
    }

    if (!tlshHash->isValid())
        return FALSE;

    tlshHashString = tlshHash->getHash(TRUE);

    if (tlshHashString == nullptr || tlshHashString[0] == ANSI_NULL)
        return FALSE;

    *HashResult = PhZeroExtendToUtf16((PSTR)tlshHashString);

    return TRUE;
}

BOOLEAN PvGetTlshFileHash(
    _In_ HANDLE FileHandle,
    _Out_ PPH_STRING* HashResult
    )
{
    const char* tlshHashString = nullptr;
    NTSTATUS status;
    IO_STATUS_BLOCK isb;
    ULONGLONG bytesRemaining;
    BYTE buffer[PAGE_SIZE];
    LARGE_INTEGER fileSize;

    status = PhGetFileSize(FileHandle, &fileSize);

    if (!NT_SUCCESS(status))
        return FALSE;

    auto tlshHash = std::make_unique<Tlsh>();

    bytesRemaining = fileSize.QuadPart;

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

        tlshHash->update(buffer, (UINT32)isb.Information);

        bytesRemaining -= isb.Information;
    }

    tlshHash->final();

    if (!tlshHash->isValid())
        return FALSE;

    tlshHashString = tlshHash->getHash(TRUE);

    if (tlshHashString == nullptr || tlshHashString[0] == ANSI_NULL)
        return FALSE;

    *HashResult = PhZeroExtendToUtf16((PSTR)tlshHashString);

    return TRUE;
}
