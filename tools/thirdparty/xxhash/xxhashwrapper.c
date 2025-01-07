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

#include <ph.h>
#include "xxhashwrapper.h"

#define XXH_STATIC_LINKING_ONLY
#define XXH_IMPLEMENTATION

#if defined(__AVX512F__)
#define XXH_VECTOR XXH_AVX512
#elif defined(__AVX2__)
#define XXH_VECTOR XXH_AVX2
#elif defined(__SSE2__) || defined(_M_AMD64) || defined(_M_X64) || (defined(_M_IX86_FP) && (_M_IX86_FP == 2))
#define XXH_VECTOR XXH_SSE2
#elif defined(_M_ARM64)
#define XXH_VECTOR XXH_NEON
#include <arm_neon.h>
#endif

#include "xxhash.h"

ULONG PhXXH32ToInteger(
    _In_ ULONG Hash
    )
{
    ULONG value = 0;

    XXH32_canonical_t cano;
    XXH32_canonicalFromHash(&cano, Hash);
    memcpy(&value, cano.digest, sizeof(value));

    return value;
}

ULONG64 PhXXH64ToInteger(
    _In_ ULONG64 Hash
    )
{
    ULONG64 value = 0;

    XXH64_canonical_t cano;
    XXH64_canonicalFromHash(&cano, Hash);
    memcpy(&value, cano.digest, sizeof(value));

    return value;
}

ULONG64 PhXXH128ToInteger(
    _In_ PULARGE_INTEGER_128 Hash
    )
{
    ULONG64 value = 0;
    XXH128_hash_t hash;

    hash.low64 = Hash->QuadPart[0];
    hash.high64 = Hash->QuadPart[1];

    XXH128_canonical_t cano;
    XXH128_canonicalFromHash(&cano, hash);
    memcpy(&value, cano.digest, sizeof(value));

    return value;
}

ULONG PhHashStringRefXXH32(
    _In_ PPH_STRINGREF String,
    _In_ ULONG Seed
    )
{    
    return XXH32(String->Buffer, String->Length, Seed);
}

ULONG64 PhHashStringRefXXH64(
    _In_ PPH_STRINGREF String,
    _In_ ULONG64 Seed
    )
{    
    return XXH64(String->Buffer, String->Length, Seed);
}

ULONG64 PhHashStringRefXXH3_64(
    _In_ PPH_STRINGREF String,
    _In_ ULONG64 Seed
    )
{
    return XXH3_64bits_withSeed(String->Buffer, String->Length, Seed);
}

BOOLEAN PhHashStringRefXXH3_128(
    _In_ PPH_STRINGREF String,
    _In_ ULONG64 Seed,
    _Out_ PULARGE_INTEGER_128 LargeInteger
    )
{
    XXH128_hash_t hash;

    hash = XXH3_128bits_withSeed(
        String->Buffer,
        String->Length,
        Seed
        );

    LargeInteger->QuadPart[0] = hash.low64;
    LargeInteger->QuadPart[1] = hash.high64;
    return TRUE;
}

BOOLEAN A_XXH32_Init(
    _Out_ PVOID* Context,
    _In_ ULONG Seed
    )
{
    XXH32_state_t* state;

    // Allocate the state. Do not use malloc() or new.
    state = XXH32_createState();
    XXH32_reset(state, Seed);

    *Context = (PVOID)state;
    return TRUE;
}

BOOLEAN A_XXH64_Init(
    _Out_ PVOID* Context,
    _In_ ULONG Seed
    )
{
    XXH64_state_t* state;

    state = XXH64_createState();
    XXH64_reset(state, Seed);

    *Context = (PVOID)state;
    return TRUE;
}

BOOLEAN A_XXH3_64bits_Init(
    _Out_ PVOID* Context,
    _In_ ULONG64 Seed
    )
{
    XXH3_state_t* state;

    state = XXH3_createState();
    XXH3_64bits_reset_withSeed(state, Seed);

    *Context = (PVOID)state;
    return TRUE;
}

BOOLEAN A_XXH3_128bits_Init(
    _Out_ PVOID* Context,
    _In_ ULONG64 Seed
    )
{
    XXH3_state_t* state;

    state = XXH3_createState();
    XXH3_128bits_reset_withSeed(state, Seed);

    *Context = (PVOID)state;
    return TRUE;
}

BOOLEAN A_XXH32_Update(
    _In_ PVOID Context,
    _In_reads_bytes_(Length) PVOID Input,
    _In_ SIZE_T Length
    )
{
    return XXH32_update((XXH32_state_t*)Context, Input, Length) == XXH_OK;
}

BOOLEAN A_XXH64_Update(
    _In_ PVOID Context,
    _In_reads_bytes_(Length) PVOID Input,
    _In_ SIZE_T Length
    )
{
    return XXH64_update((XXH64_state_t*)Context, Input, Length) == XXH_OK;
}

BOOLEAN A_XXH3_64bits_Update(
    _In_ PVOID Context,
    _In_reads_bytes_(Length) PVOID Input,
    _In_ SIZE_T Length
    )
{
    return XXH3_64bits_update((XXH3_state_t*)Context, Input, Length) == XXH_OK;
}

BOOLEAN A_XXH3_128bits_Update(
    _In_ PVOID Context,
    _In_reads_bytes_(Length) PVOID Input,
    _In_ SIZE_T Length
    )
{
    return XXH3_128bits_update((XXH3_state_t*)Context, Input, Length) == XXH_OK;
}

VOID A_XXH32_Final(
    _In_ PVOID Context,
    _Out_ uint64_t* Digest
    )
{
    // Retrieve the finalized hash. This will not change the state.
    *Digest = XXH32_digest((XXH32_state_t*)Context);
    // Free the state. Do not use free().
    XXH32_freeState((XXH32_state_t*)Context);
}

VOID A_XXH64_Final(
    _In_ PVOID Context,
    _Out_ uint64_t* Digest
    )
{
    *Digest = XXH64_digest((XXH64_state_t*)Context);
    XXH64_freeState((XXH64_state_t*)Context);
}

VOID A_XXH3_64bits_Final(
    _In_ PVOID Context,
    _Out_ uint64_t* Digest
    )
{
    *Digest = XXH3_64bits_digest((XXH3_state_t*)Context);
    XXH3_freeState((XXH3_state_t*)Context);
}

VOID A_XXH3_128bits_Final(
    _In_ PVOID Context,
    _Out_ PULARGE_INTEGER_128 Digest
    )
{
    XXH128_hash_t hash;

    hash = XXH3_128bits_digest((XXH3_state_t*)Context);
    Digest->QuadPart[0] = hash.low64;
    Digest->QuadPart[1] = hash.high64;
    XXH3_freeState((XXH3_state_t*)Context);
}

//uint64_t XXH3HashFile(
//     _In_ FILE* File
//    )
// {
//     XXH3_state_t* state;
//     XXH64_hash_t result;
//     size_t count;
//     char buffer[4096];
//
//     state = XXH3_createState();
//     XXH3_64bits_reset(state);
//
//     while ((count = fread(buffer, 1, sizeof(buffer), File)) != 0)
//     {
//         XXH3_64bits_update(state, buffer, count);
//     }
//
//     result = XXH3_64bits_digest(state);
//     XXH3_freeState(state);
//
//     return result;
// }
