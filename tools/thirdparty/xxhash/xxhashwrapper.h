/*
 * Copyright (c) 2022 Winsider Seminars & Solutions, Inc.  All rights reserved.
 *
 * This file is part of System Informer.
 *
 * Authors:
 *
 *     dmex    2024
 *
 */

#pragma once

EXTERN_C_START

EXTERN_C
ULONG
PhXXH32ToInteger(
    _In_ ULONG Hash
    );

EXTERN_C
ULONG64
PhXXH64ToInteger(
    _In_ ULONG64 Hash
    );

EXTERN_C
ULONG64
PhXXH128ToInteger(
    _In_ PULARGE_INTEGER_128 Hash
    );

EXTERN_C
ULONG
PhHashStringRefXXH32(
    _In_ PPH_STRINGREF String,
    _In_ ULONG Seed
    );

EXTERN_C
ULONG64 PhHashStringRefXXH64(
    _In_ PPH_STRINGREF String,
    _In_ ULONG64 Seed
    );

EXTERN_C
ULONG64 PhHashStringRefXXH3_64(
    _In_ PPH_STRINGREF String,
    _In_ ULONG64 Seed
    );

EXTERN_C
BOOLEAN PhHashStringRefXXH3_128(
    _In_ PPH_STRINGREF String,
    _In_ ULONG64 Seed,
    _Out_ PULARGE_INTEGER_128 LargeInteger
    );

EXTERN_C
BOOLEAN A_XXH32_Init(
    _Out_ PVOID* Context,
    _In_ ULONG Seed
    );

EXTERN_C
BOOLEAN A_XXH64_Init(
    _Out_ PVOID* Context,
    _In_ ULONG Seed
    );

EXTERN_C
BOOLEAN A_XXH3_64bits_Init(
    _Out_ PVOID* Context,
    _In_ ULONG64 Seed
    );

EXTERN_C
BOOLEAN A_XXH3_128bits_Init(
    _Out_ PVOID* Context,
    _In_ ULONG64 Seed
    );

EXTERN_C
BOOLEAN A_XXH32_Update(
    _In_ PVOID Context,
    _In_reads_bytes_(Length) PVOID Input,
    _In_ SIZE_T Length
    );

EXTERN_C
BOOLEAN A_XXH64_Update(
    _In_ PVOID Context,
    _In_reads_bytes_(Length) PVOID Input,
    _In_ SIZE_T Length
    );

EXTERN_C
BOOLEAN A_XXH3_64bits_Update(
    _In_ PVOID Context,
    _In_reads_bytes_(Length) PVOID Input,
    _In_ SIZE_T Length
    );

EXTERN_C
BOOLEAN A_XXH3_128bits_Update(
    _In_ PVOID Context,
    _In_reads_bytes_(Length) PVOID Input,
    _In_ SIZE_T Length
    );

EXTERN_C
VOID A_XXH32_Final(
    _In_ PVOID Context,
    _Out_ uint64_t* Digest
    );

EXTERN_C
VOID A_XXH64_Final(
    _In_ PVOID Context,
    _Out_ uint64_t* Digest
    );

EXTERN_C
VOID A_XXH3_64bits_Final(
    _In_ PVOID Context,
    _Out_ uint64_t* Digest
    );

EXTERN_C
VOID A_XXH3_128bits_Final(
    _In_ PVOID Context,
    _Out_ PULARGE_INTEGER_128 Digest
    );

EXTERN_C_END
