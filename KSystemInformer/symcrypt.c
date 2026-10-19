/*
 * Copyright (c) 2022 Winsider Seminars & Solutions, Inc.  All rights reserved.
 *
 * This file is part of System Informer.
 *
 * Authors:
 *
 *     jxy-s   2026
 *
 */

#include <kph.h>

#include <trace.h>

SYMCRYPT_ENVIRONMENT_WINDOWS_KERNELMODE_WIN8_1_N_LATER;

KPH_PAGED_FILE();

// SymCrypt declares the callbacks without SAL; these contracts are ours.
#pragma warning(push)
#pragma warning(disable: 28253 28301)

/**
 * \brief Allocates memory for SymCrypt.
 *
 * \param[in] NumberOfBytes The number of bytes to allocate.
 *
 * \return Paged memory aligned to SYMCRYPT_ASYM_ALIGN_VALUE, NULL on failure.
 */
_IRQL_requires_max_(PASSIVE_LEVEL)
PVOID SYMCRYPT_CALL SymCryptCallbackAlloc(
    _In_ SIZE_T NumberOfBytes
    )
{
    SIZE_T size;
    PBYTE buffer;
    PBYTE aligned;

    KPH_PAGED_CODE_PASSIVE();

    if (!NT_SUCCESS(RtlSizeTAdd(NumberOfBytes,
                                SYMCRYPT_ASYM_ALIGN_VALUE + sizeof(PVOID),
                                &size)))
    {
        return NULL;
    }

    buffer = KphAllocatePaged(size, KPH_TAG_SYMCRYPT);
    if (!buffer)
    {
        return NULL;
    }

    //
    // The raw allocation is stored immediately below the aligned block.
    //
    aligned = SYMCRYPT_ASYM_ALIGN_UP(buffer + sizeof(PVOID));
    ((PVOID*)aligned)[-1] = buffer;

    return aligned;
}

/**
 * \brief Frees memory allocated by SymCryptCallbackAlloc.
 *
 * \param[in] Memory The memory to free.
 */
_IRQL_requires_max_(PASSIVE_LEVEL)
VOID SYMCRYPT_CALL SymCryptCallbackFree(
    _In_ PVOID Memory
    )
{
    KPH_PAGED_CODE_PASSIVE();
    NT_ASSERT(Memory);

    KphFree(((PVOID*)Memory)[-1], KPH_TAG_SYMCRYPT);
}

/**
 * \brief Fills a buffer with random bytes for SymCrypt.
 *
 * \param[out] Buffer The buffer to fill.
 * \param[in] BufferLength The length of the buffer.
 *
 * \return SYMCRYPT_NO_ERROR on success, SYMCRYPT_EXTERNAL_FAILURE otherwise.
 */
_IRQL_requires_max_(PASSIVE_LEVEL)
SYMCRYPT_ERROR SYMCRYPT_CALL SymCryptCallbackRandom(
    _Out_writes_bytes_(BufferLength) PBYTE Buffer,
    _In_ SIZE_T BufferLength
    )
{
    NTSTATUS status;
    ULONG length;

    KPH_PAGED_CODE_PASSIVE();

    status = RtlSizeTToULong(BufferLength, &length);
    if (!NT_SUCCESS(status))
    {
        return SYMCRYPT_EXTERNAL_FAILURE;
    }

    status = BCryptGenRandom(NULL,
                             Buffer,
                             length,
                             BCRYPT_USE_SYSTEM_PREFERRED_RNG);
    if (!NT_SUCCESS(status))
    {
        return SYMCRYPT_EXTERNAL_FAILURE;
    }

    return SYMCRYPT_NO_ERROR;
}

#pragma warning(pop)
