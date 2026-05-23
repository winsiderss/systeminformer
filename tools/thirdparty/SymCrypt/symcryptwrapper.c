/*
 * Copyright (c) 2026 Winsider Seminars & Solutions, Inc.  All rights reserved.
 *
 * This file is part of System Informer.
 *
 * Authors:
 *
 *     dmex    2026
 *
 */

#include <phbase.h>
#include "symcryptwrapper.h"
#include <symcrypt.h>
#include <bcrypt.h>

// ------------------------------------------------------------------------
// SYMCRYPT_ERROR -> NTSTATUS translation
// ------------------------------------------------------------------------

/**
 * Translates a SymCrypt error code to the closest matching NTSTATUS.
 *
 * Provides a single chokepoint for error mapping so every wrapper returns
 * consistent NTSTATUS values to callers that have no notion of SYMCRYPT_ERROR.
 *
 * \param[in] Error SymCrypt error code returned by the underlying primitive.
 * \return STATUS_SUCCESS for SYMCRYPT_NO_ERROR; otherwise a domain-specific
 *         NTSTATUS (auth-tag mismatch, invalid signature, resources, etc.).
 */
NTSTATUS PhSymCryptErrorToStatus(
    _In_ SYMCRYPT_ERROR Error
    )
{
    switch (Error)
    {
    case SYMCRYPT_NO_ERROR: return STATUS_SUCCESS;
    // AEAD tag verification failed (GCM, ChaCha20-Poly1305).
    case SYMCRYPT_AUTHENTICATION_FAILURE: return STATUS_AUTH_TAG_MISMATCH;
    // Asymmetric signature verification failed (RSA, ECDSA).
    case SYMCRYPT_SIGNATURE_VERIFICATION_FAILURE: return STATUS_INVALID_SIGNATURE;
    case SYMCRYPT_MEMORY_ALLOCATION_FAILURE: return STATUS_INSUFFICIENT_RESOURCES;
    case SYMCRYPT_NOT_IMPLEMENTED: return STATUS_NOT_IMPLEMENTED;
    case SYMCRYPT_BUFFER_TOO_SMALL: return STATUS_BUFFER_TOO_SMALL;
    // Environmental / FIPS module faults that callers cannot fix.
    case SYMCRYPT_HARDWARE_FAILURE: return STATUS_UNSUCCESSFUL;
    case SYMCRYPT_EXTERNAL_FAILURE: return STATUS_UNSUCCESSFUL;
    case SYMCRYPT_FIPS_FAILURE: return STATUS_UNSUCCESSFUL;
    // Treat any unmapped SymCrypt code as a parameter error.
    default: return STATUS_INVALID_PARAMETER;
    }
}

// ------------------------------------------------------------------------
// Random / RNG
// ------------------------------------------------------------------------

/**
 * Fills a buffer with cryptographically random bytes from SymCrypt's RNG.
 *
 * \param[out] Buffer Destination buffer to fill with random data.
 * \param[in] Length Number of random bytes to produce.
 * \return Always TRUE (the SymCrypt RNG does not return failure for this entry point).
 */
VOID NTAPI PhSymCryptRandom(
    _Out_writes_bytes_(Length) PBYTE Buffer,
    _In_ SIZE_T Length
    )
{
    SymCryptRandom(Buffer, Length);
}

/**
 * Feeds additional entropy into SymCrypt's RNG pool.
 *
 * \param[in] Entropy Buffer of caller-supplied entropy bytes.
 * \param[in] Length Size of the entropy buffer in bytes.
 */
VOID NTAPI PhSymCryptProvideEntropy(
    _In_reads_bytes_(Length) PCVOID Entropy,
    _In_ SIZE_T Length
    )
{
    SymCryptProvideEntropy((PCBYTE)Entropy, Length);
}

/**
 * Reports whether the CPU's RDRAND instruction is usable on this host.
 *
 * \return STATUS_SUCCESS if RDRAND is available and trusted by SymCrypt;
 * otherwise an NTSTATUS describing the failure mode.
 */
NTSTATUS NTAPI PhSymCryptRdrandStatus(
    VOID
    )
{
#if SYMCRYPT_CPU_X86 | SYMCRYPT_CPU_AMD64
    return PhSymCryptErrorToStatus(SymCryptRdrandStatus());
#else
    return STATUS_NOT_IMPLEMENTED;
#endif
}

/**
 * Pulls random bytes directly from the CPU's RDRAND with a hash post-mix.
 *
 * SymCrypt requires a scratch hash buffer that it uses internally to whiten the
 * RDRAND output; the buffer is wiped before return so the caller never sees it.
 *
 * \param[out] Buffer Destination buffer to fill with hashed RDRAND output.
 * \param[in] Length Number of bytes to produce.
 * \return STATUS_SUCCESS on success; an NTSTATUS error otherwise.
 */
NTSTATUS NTAPI PhSymCryptRdrandGetBytes(
    _Out_writes_bytes_(Length) PVOID Buffer,
    _In_ SIZE_T Length
    )
{
#if SYMCRYPT_CPU_X86 | SYMCRYPT_CPU_AMD64
    BYTE hashScratch[SYMCRYPT_SHA512_RESULT_SIZE];
    SYMCRYPT_ERROR error;

    //
    // Sample RDRAND, hash-mix into Buffer, scratch buffer used internally.
    //

    error = SymCryptRdrandGetBytes((PBYTE)Buffer, Length, hashScratch);

    //
    // Wipe the scratch to avoid leaking intermediate hash state.
    //

    SymCryptWipeKnownSize(hashScratch, sizeof(hashScratch));

    return PhSymCryptErrorToStatus(error);
#else
    return STATUS_NOT_IMPLEMENTED;
#endif
}

// ------------------------------------------------------------------------
// Constant-time / wipe utilities
// ------------------------------------------------------------------------

/**
 * Constant-time byte-buffer equality check.
 *
 * Always touches every byte regardless of mismatch position so timing cannot
 * be used to learn which byte differed (defense against timing side-channels).
 *
 * \param[in] Buffer1 First buffer.
 * \param[in] Buffer2 Second buffer.
 * \param[in] Length Number of bytes to compare.
 * \return TRUE if the buffers are byte-for-byte identical; FALSE otherwise.
 */
BOOLEAN NTAPI PhSymCryptEqual(
    _In_reads_bytes_(Length) PCVOID Buffer1,
    _In_reads_bytes_(Length) PCVOID Buffer2,
    _In_ SIZE_T Length
    )
{
    return SymCryptEqual((PCBYTE)Buffer1, (PCBYTE)Buffer2, Length);
}

/**
 * Dead-store-resistant zero overwrite. Securely wipes a buffer,
 * resistant to compiler dead-store elimination.
 *
 * Used to scrub sensitive material (keys, plaintext, intermediate state)
 * before the buffer's lifetime ends.
 *
 * \param[out] Buffer Buffer to overwrite.
 * \param[in] Length Size of the buffer in bytes.
 */
VOID NTAPI PhSymCryptWipe(
    _Out_writes_bytes_(Length) PVOID Buffer,
    _In_ SIZE_T Length
    )
{
    SymCryptWipe(Buffer, Length);
}

/**
 * Constant-time pointwise XOR of two buffers into a destination.
 * Useful for one-time-pad style operations and masking. Destination may
 * alias either source.
 *
 * \param[in] Source1 First operand.
 * \param[in] Source2 Second operand.
 * \param[out] Destination Output buffer of size \p Length.
 * \param[in] Length Number of bytes to XOR.
 */
VOID NTAPI PhSymCryptXorBytes(
    _In_reads_bytes_(Length) PCVOID Source1,
    _In_reads_bytes_(Length) PCVOID Source2,
    _Out_writes_bytes_(Length) PVOID Destination,
    _In_ SIZE_T Length
    )
{
    SymCryptXorBytes((PCBYTE)Source1, (PCBYTE)Source2, (PBYTE)Destination, Length);
}

// ------------------------------------------------------------------------
// Single-shot hashes
// ------------------------------------------------------------------------

/**
 * One-shot MD5 hash. Provided only for legacy interop; not collision-resistant.
 *
 * \param[in] Buffer Message bytes.
 * \param[in] Length Message size.
 * \param[out] Result 16-byte digest output.
 */
VOID NTAPI PhSymCryptMd5(
    _In_reads_bytes_(Length) PCVOID Buffer,
    _In_ SIZE_T Length,
    _Out_writes_bytes_(PH_SYMCRYPT_MD5_RESULT_SIZE) PVOID Result
    )
{
    SymCryptMd5((PCBYTE)Buffer, Length, (PBYTE)Result);
}

/**
 * One-shot SHA-1 hash. Legacy interop only; not collision-resistant.
 *
 * \param[in] Buffer Message bytes.
 * \param[in] Length Message size.
 * \param[out] Result 20-byte digest output.
 */
VOID NTAPI PhSymCryptSha1(
    _In_reads_bytes_(Length) PCVOID Buffer,
    _In_ SIZE_T Length,
    _Out_writes_bytes_(PH_SYMCRYPT_SHA1_RESULT_SIZE) PVOID Result
    )
{
    SymCryptSha1((PCBYTE)Buffer, Length, (PBYTE)Result);
}

/**
 * One-shot SHA-256 hash.
 *
 * \param[in] Buffer Message bytes.
 * \param[in] Length Message size.
 * \param[out] Result 32-byte digest output.
 */
VOID NTAPI PhSymCryptSha256(
    _In_reads_bytes_(Length) PCVOID Buffer,
    _In_ SIZE_T Length,
    _Out_writes_bytes_(PH_SYMCRYPT_SHA256_RESULT_SIZE) PVOID Result
    )
{
    SymCryptSha256((PCBYTE)Buffer, Length, (PBYTE)Result);
}

/**
 * One-shot SHA-384 hash.
 *
 * \param[in] Buffer Message bytes.
 * \param[in] Length Message size.
 * \param[out] Result 48-byte digest output.
 */
VOID NTAPI PhSymCryptSha384(
    _In_reads_bytes_(Length) PCVOID Buffer,
    _In_ SIZE_T Length,
    _Out_writes_bytes_(PH_SYMCRYPT_SHA384_RESULT_SIZE) PVOID Result
    )
{
    SymCryptSha384((PCBYTE)Buffer, Length, (PBYTE)Result);
}

/**
 * One-shot SHA-512 hash.
 *
 * \param[in] Buffer Message bytes.
 * \param[in] Length Message size.
 * \param[out] Result 64-byte digest output.
 */
VOID NTAPI PhSymCryptSha512(
    _In_reads_bytes_(Length) PCVOID Buffer,
    _In_ SIZE_T Length,
    _Out_writes_bytes_(PH_SYMCRYPT_SHA512_RESULT_SIZE) PVOID Result
    )
{
    SymCryptSha512((PCBYTE)Buffer, Length, (PBYTE)Result);
}

/**
 * One-shot SHA3-256 (Keccak family, FIPS 202).
 *
 * \param[in] Buffer Message bytes.
 * \param[in] Length Message size.
 * \param[out] Result 32-byte digest output.
 */
VOID NTAPI PhSymCryptSha3_256(
    _In_reads_bytes_(Length) PCVOID Buffer,
    _In_ SIZE_T Length,
    _Out_writes_bytes_(PH_SYMCRYPT_SHA3_256_RESULT_SIZE) PVOID Result
    )
{
    SymCryptSha3_256((PCBYTE)Buffer, Length, (PBYTE)Result);
}

/**
 * One-shot SHA3-384 (Keccak family, FIPS 202).
 *
 * \param[in] Buffer Message bytes.
 * \param[in] Length Message size.
 * \param[out] Result 48-byte digest output.
 */
VOID NTAPI PhSymCryptSha3_384(
    _In_reads_bytes_(Length) PCVOID Buffer,
    _In_ SIZE_T Length,
    _Out_writes_bytes_(PH_SYMCRYPT_SHA3_384_RESULT_SIZE) PVOID Result
    )
{
    SymCryptSha3_384((PCBYTE)Buffer, Length, (PBYTE)Result);
}

/**
 * One-shot SHA3-512 (Keccak family, FIPS 202).
 *
 * \param[in] Buffer Message bytes.
 * \param[in] Length Message size.
 * \param[out] Result 64-byte digest output.
 */
VOID NTAPI PhSymCryptSha3_512(
    _In_reads_bytes_(Length) PCVOID Buffer,
    _In_ SIZE_T Length,
    _Out_writes_bytes_(PH_SYMCRYPT_SHA3_512_RESULT_SIZE) PVOID Result
    )
{
    SymCryptSha3_512((PCBYTE)Buffer, Length, (PBYTE)Result);
}

// ------------------------------------------------------------------------
// Incremental hash helpers
//
// Generic caller-owned context with private SymCrypt internals.
// ------------------------------------------------------------------------

typedef struct _PH_SYMCRYPT_COMPAT_HASH_CONTEXT
{
    PH_SYMCRYPT_HASH_CONTEXT Context;
} PH_SYMCRYPT_COMPAT_HASH_CONTEXT, *PPH_SYMCRYPT_COMPAT_HASH_CONTEXT;

static_assert(RTL_FIELD_SIZE(PH_SYMCRYPT_HASH_CONTEXT, State) == sizeof(SYMCRYPT_HASH_STATE), "PH_SYMCRYPT_HASH_CONTEXT.State must match sizeof(SYMCRYPT_HASH_STATE)");
static_assert((PH_SYMCRYPT_HASH_STATE_BUFFER_ALIGNMENT & (PH_SYMCRYPT_HASH_STATE_BUFFER_ALIGNMENT - 1)) == 0, "PH_SYMCRYPT_HASH_STATE_BUFFER_ALIGNMENT must be a power of two");
static_assert((sizeof(PH_SYMCRYPT_HASH_CONTEXT) % PH_SYMCRYPT_HASH_STATE_BUFFER_ALIGNMENT) == 0, "PH_SYMCRYPT_HASH_CONTEXT alignment mismatch");

/**
 * Resolves a wrapper hash selector to SymCrypt's hash descriptor and digest size.
 *
 * \param[in] Algorithm Wrapper hash selector constant.
 * \param[out] HashAlgorithm Receives SymCrypt hash descriptor pointer.
 * \param[out] HashResultSize Receives digest size in bytes.
 * \return TRUE if the selector is supported; FALSE otherwise.
 */
_Success_(return)
BOOLEAN PhSymCryptResolveHashAlgorithm(
    _In_ PH_SYMCRYPT_HASH_ALGORITHM Algorithm,
    _Out_ PCSYMCRYPT_HASH* HashAlgorithm,
    _Out_ PULONG HashResultSize
    )
{
    switch (Algorithm)
    {
    case PH_SYMCRYPT_MD5_ALGORITHM:
        *HashAlgorithm = SymCryptMd5Algorithm;
        *HashResultSize = PH_SYMCRYPT_MD5_RESULT_SIZE;
        return TRUE;
    case PH_SYMCRYPT_SHA1_ALGORITHM:
        *HashAlgorithm = SymCryptSha1Algorithm;
        *HashResultSize = PH_SYMCRYPT_SHA1_RESULT_SIZE;
        return TRUE;
    case PH_SYMCRYPT_SHA256_ALGORITHM:
        *HashAlgorithm = SymCryptSha256Algorithm;
        *HashResultSize = PH_SYMCRYPT_SHA256_RESULT_SIZE;
        return TRUE;
    case PH_SYMCRYPT_SHA384_ALGORITHM:
        *HashAlgorithm = SymCryptSha384Algorithm;
        *HashResultSize = PH_SYMCRYPT_SHA384_RESULT_SIZE;
        return TRUE;
    case PH_SYMCRYPT_SHA512_ALGORITHM:
        *HashAlgorithm = SymCryptSha512Algorithm;
        *HashResultSize = PH_SYMCRYPT_SHA512_RESULT_SIZE;
        return TRUE;
    case PH_SYMCRYPT_SHA3_256_ALGORITHM:
        *HashAlgorithm = SymCryptSha3_256Algorithm;
        *HashResultSize = PH_SYMCRYPT_SHA3_256_RESULT_SIZE;
        return TRUE;
    case PH_SYMCRYPT_SHA3_384_ALGORITHM:
        *HashAlgorithm = SymCryptSha3_384Algorithm;
        *HashResultSize = PH_SYMCRYPT_SHA3_384_RESULT_SIZE;
        return TRUE;
    case PH_SYMCRYPT_SHA3_512_ALGORITHM:
        *HashAlgorithm = SymCryptSha3_512Algorithm;
        *HashResultSize = PH_SYMCRYPT_SHA3_512_RESULT_SIZE;
        return TRUE;
    default:
        return FALSE;
    }
}

/**
 * Initializes a caller-owned incremental hash context.
 *
 * \param[in] Algorithm Wrapper hash selector.
 * \param[out] Context Receives initialized hash context.
 * \return STATUS_SUCCESS on success; STATUS_NOT_SUPPORTED for unknown algorithm;
 * STATUS_BUFFER_TOO_SMALL if context state storage is insufficient.
 */
NTSTATUS NTAPI PhSymCryptHashInit(
    _In_ PH_SYMCRYPT_HASH_ALGORITHM Algorithm,
    _Out_ PPH_SYMCRYPT_HASH_CONTEXT Context
    )
{
    PCSYMCRYPT_HASH hashAlgorithm;
    ULONG hashResultSize;

    if (!Context)
        return STATUS_INVALID_PARAMETER;

    if (!PhSymCryptResolveHashAlgorithm(Algorithm, &hashAlgorithm, &hashResultSize))
        return STATUS_NOT_SUPPORTED;

    memset(Context, 0, sizeof(PH_SYMCRYPT_HASH_CONTEXT));
    Context->Algorithm = (PVOID)hashAlgorithm;
    Context->ResultSize = hashResultSize;
    Context->StateSize = (ULONG)SymCryptHashStateSize(hashAlgorithm);

    if (Context->StateSize > PH_SYMCRYPT_HASH_STATE_BUFFER_SIZE)
        return STATUS_BUFFER_TOO_SMALL;

    SymCryptHashInit(hashAlgorithm, Context->State);
    return STATUS_SUCCESS;
}

/**
 * Adds input bytes to an initialized incremental hash context.
 *
 * \param[in,out] Context Hash context previously initialized by PhSymCryptHashInit.
 * \param[in] Buffer Input bytes.
 * \param[in] Length Number of input bytes.
 * \return STATUS_SUCCESS or STATUS_INVALID_PARAMETER if context is invalid.
 */
NTSTATUS NTAPI PhSymCryptHashData(
    _Inout_ PPH_SYMCRYPT_HASH_CONTEXT Context,
    _In_reads_bytes_(Length) PCVOID Buffer,
    _In_ SIZE_T Length
    )
{
    PCSYMCRYPT_HASH hashAlgorithm;

    if (!Context || !Context->Algorithm)
        return STATUS_INVALID_PARAMETER;

    hashAlgorithm = (PCSYMCRYPT_HASH)Context->Algorithm;
    SymCryptHashAppend(hashAlgorithm, Context->State, (PCBYTE)Buffer, Length);
    return STATUS_SUCCESS;
}

/**
 * Finalizes an incremental hash and clears the context state.
 *
 * \param[in,out] Context Hash context to finalize.
 * \param[out] Result Output buffer for digest bytes.
 * \param[in] ResultLength Number of result bytes requested.
 * \return STATUS_SUCCESS or STATUS_INVALID_PARAMETER if context is invalid.
 */
NTSTATUS NTAPI PhSymCryptHashFinal(
    _Inout_ PPH_SYMCRYPT_HASH_CONTEXT Context,
    _Out_writes_bytes_(ResultLength) PVOID Result,
    _In_ ULONG ResultLength
    )
{
    PCSYMCRYPT_HASH hashAlgorithm;

    if (!Context || !Context->Algorithm)
        return STATUS_INVALID_PARAMETER;

    hashAlgorithm = (PCSYMCRYPT_HASH)Context->Algorithm;
    SymCryptHashResult(hashAlgorithm, Context->State, (PBYTE)Result, ResultLength);
    SymCryptWipeKnownSize(Context->State, sizeof(Context->State));
    Context->Algorithm = NULL;
    Context->ResultSize = 0;
    Context->StateSize = 0;
    return STATUS_SUCCESS;
}

/**
 * Returns the digest size for a hash algorithm selector.
 *
 * \param[in] Algorithm Wrapper hash selector.
 * \param[out] HashSize Receives digest size in bytes.
 * \return STATUS_SUCCESS or STATUS_NOT_SUPPORTED for unknown algorithm.
 */
NTSTATUS NTAPI PhSymCryptGetHashSize(
    _In_ PH_SYMCRYPT_HASH_ALGORITHM Algorithm,
    _Out_ PULONG HashSize
    )
{
    PCSYMCRYPT_HASH hashAlgorithm;

    if (!HashSize)
        return STATUS_INVALID_PARAMETER;

    if (!PhSymCryptResolveHashAlgorithm(Algorithm, &hashAlgorithm, HashSize))
        return STATUS_NOT_SUPPORTED;

    return STATUS_SUCCESS;
}

/**
 * Returns the configured digest size of an initialized hash context.
 *
 * \param[in] Context Hash context.
 * \param[out] HashSize Receives context digest size in bytes.
 * \return STATUS_SUCCESS or STATUS_INVALID_PARAMETER if inputs are invalid.
 */
NTSTATUS NTAPI PhSymCryptHashSize(
    _In_ PPH_SYMCRYPT_HASH_CONTEXT Context,
    _Out_ PULONG HashSize
    )
{
    if (!Context || !HashSize)
        return STATUS_INVALID_PARAMETER;

    *HashSize = Context->ResultSize;
    return STATUS_SUCCESS;
}

/**
 * Best-effort destroy helper for incremental hash contexts.
 *
 * Finalizes into a throwaway local buffer to ensure context state is cleared.
 *
 * \param[in,out] Context Hash context to destroy.
 * \param[in] HashSize Expected digest size in bytes.
 */
VOID NTAPI PhSymCryptDestroyHash(
    _Inout_ PPH_SYMCRYPT_HASH_CONTEXT Context,
    _In_ ULONG HashSize
    )
{
    UCHAR discard[PH_SYMCRYPT_SHA512_RESULT_SIZE];

    if (!Context || !Context->Algorithm)
        return;

    if (HashSize > sizeof(discard))
        HashSize = sizeof(discard);

    PhSymCryptHashFinal(Context, discard, HashSize);
}

/**
 * Maps a BCrypt algorithm ID string to wrapper hash selector and digest size.
 *
 * \param[in] AlgorithmId BCrypt hash algorithm ID string (e.g. SHA256).
 * \param[out] Algorithm Receives wrapper hash selector.
 * \param[out] HashSize Optional digest size output.
 * \return STATUS_SUCCESS on success or STATUS_NOT_SUPPORTED if unknown.
 */
NTSTATUS NTAPI PhSymCryptHashAlgorithmIdToAlgorithm(
    _In_ PCWSTR AlgorithmId,
    _Out_ PPH_SYMCRYPT_HASH_ALGORITHM Algorithm,
    _Out_opt_ PULONG HashSize
    )
{
    if (!AlgorithmId || !Algorithm)
        return STATUS_INVALID_PARAMETER;

    if (PhEqualStringZ(AlgorithmId, BCRYPT_MD5_ALGORITHM, FALSE))
    {
        *Algorithm = PH_SYMCRYPT_MD5_ALGORITHM;
        if (HashSize) *HashSize = PH_SYMCRYPT_MD5_RESULT_SIZE;
        return STATUS_SUCCESS;
    }

    if (PhEqualStringZ(AlgorithmId, BCRYPT_SHA1_ALGORITHM, FALSE))
    {
        *Algorithm = PH_SYMCRYPT_SHA1_ALGORITHM;
        if (HashSize) *HashSize = PH_SYMCRYPT_SHA1_RESULT_SIZE;
        return STATUS_SUCCESS;
    }

    if (PhEqualStringZ(AlgorithmId, BCRYPT_SHA256_ALGORITHM, FALSE))
    {
        *Algorithm = PH_SYMCRYPT_SHA256_ALGORITHM;
        if (HashSize) *HashSize = PH_SYMCRYPT_SHA256_RESULT_SIZE;
        return STATUS_SUCCESS;
    }

    if (PhEqualStringZ(AlgorithmId, BCRYPT_SHA384_ALGORITHM, FALSE))
    {
        *Algorithm = PH_SYMCRYPT_SHA384_ALGORITHM;
        if (HashSize) *HashSize = PH_SYMCRYPT_SHA384_RESULT_SIZE;
        return STATUS_SUCCESS;
    }

    if (PhEqualStringZ(AlgorithmId, BCRYPT_SHA512_ALGORITHM, FALSE))
    {
        *Algorithm = PH_SYMCRYPT_SHA512_ALGORITHM;
        if (HashSize) *HashSize = PH_SYMCRYPT_SHA512_RESULT_SIZE;
        return STATUS_SUCCESS;
    }

    return STATUS_NOT_SUPPORTED;
}

#define PH_SYMCRYPT_DEFINE_INCREMENTAL_HASH(Tag, AlgorithmId, ResultSize)      \
    NTSTATUS                                                                    \
    NTAPI                                                                       \
    PhSymCrypt##Tag##Init(                                                      \
        _Out_ PVOID* Context                                                    \
        )                                                                       \
    {                                                                           \
        PH_SYMCRYPT_COMPAT_HASH_CONTEXT* compatContext;                         \
        NTSTATUS status;                                                        \
        if (!Context)                                                           \
            return STATUS_INVALID_PARAMETER;                                    \
        compatContext = PhAllocateSafe(sizeof(PH_SYMCRYPT_COMPAT_HASH_CONTEXT));\
        if (!compatContext)                                                     \
            return STATUS_NO_MEMORY;                                            \
        status = PhSymCryptHashInit(AlgorithmId, &compatContext->Context);      \
        if (!NT_SUCCESS(status))                                                \
        {                                                                       \
            PhFree(compatContext);                                              \
            return status;                                                      \
        }                                                                       \
        *Context = compatContext;                                               \
        return STATUS_SUCCESS;                                                  \
    }                                                                           \
    VOID                                                                        \
    NTAPI                                                                       \
    PhSymCrypt##Tag##Append(                                                    \
        _Inout_ PVOID Context,                                                  \
        _In_reads_bytes_(Length) PCVOID Buffer,                                 \
        _In_ SIZE_T Length                                                      \
        )                                                                       \
    {                                                                           \
        if (!Context) return;                                                   \
        PhSymCryptHashData(&((PPH_SYMCRYPT_COMPAT_HASH_CONTEXT)Context)->Context, Buffer, Length); \
    }                                                                           \
    VOID                                                                        \
    NTAPI                                                                       \
    PhSymCrypt##Tag##Result(                                                    \
        _Inout_ PVOID Context,                                                  \
        _Out_writes_bytes_(ResultSize) PVOID Result                             \
        )                                                                       \
    {                                                                           \
        if (!Context) return;                                                   \
        PhSymCryptHashFinal(&((PPH_SYMCRYPT_COMPAT_HASH_CONTEXT)Context)->Context, Result, ResultSize); \
        PhFree(Context);                                                        \
    }

PH_SYMCRYPT_DEFINE_INCREMENTAL_HASH(Md5, PH_SYMCRYPT_MD5_ALGORITHM, PH_SYMCRYPT_MD5_RESULT_SIZE)
PH_SYMCRYPT_DEFINE_INCREMENTAL_HASH(Sha1, PH_SYMCRYPT_SHA1_ALGORITHM, PH_SYMCRYPT_SHA1_RESULT_SIZE)
PH_SYMCRYPT_DEFINE_INCREMENTAL_HASH(Sha256, PH_SYMCRYPT_SHA256_ALGORITHM, PH_SYMCRYPT_SHA256_RESULT_SIZE)
PH_SYMCRYPT_DEFINE_INCREMENTAL_HASH(Sha384, PH_SYMCRYPT_SHA384_ALGORITHM, PH_SYMCRYPT_SHA384_RESULT_SIZE)
PH_SYMCRYPT_DEFINE_INCREMENTAL_HASH(Sha512, PH_SYMCRYPT_SHA512_ALGORITHM, PH_SYMCRYPT_SHA512_RESULT_SIZE)
PH_SYMCRYPT_DEFINE_INCREMENTAL_HASH(Sha3_256, PH_SYMCRYPT_SHA3_256_ALGORITHM, PH_SYMCRYPT_SHA3_256_RESULT_SIZE)
PH_SYMCRYPT_DEFINE_INCREMENTAL_HASH(Sha3_384, PH_SYMCRYPT_SHA3_384_ALGORITHM, PH_SYMCRYPT_SHA3_384_RESULT_SIZE)
PH_SYMCRYPT_DEFINE_INCREMENTAL_HASH(Sha3_512, PH_SYMCRYPT_SHA3_512_ALGORITHM, PH_SYMCRYPT_SHA3_512_RESULT_SIZE)

// ------------------------------------------------------------------------
// BCrypt-style incremental hash facade
// ------------------------------------------------------------------------

/**
 * Opens an incremental hash context selected by BCRYPT algorithm string.
 * Returns the hash output size in HashSize; subsequent PhSymCryptHashData /
 * PhSymCryptFinishHash calls dispatch on that value.
 */
NTSTATUS PhSymCryptOpenAlgorithmProvider(
    _Out_ PPH_SYMCRYPT_HASH_CONTEXT Context,
    _Out_ PULONG HashSize,
    _In_ PCWSTR AlgorithmId
    )
{
    PH_SYMCRYPT_HASH_ALGORITHM algorithm;
    NTSTATUS status;

    if (!Context || !HashSize)
        return STATUS_INVALID_PARAMETER;

    memset(Context, 0, sizeof(PH_SYMCRYPT_HASH_CONTEXT));
    *HashSize = 0;

    status = PhSymCryptHashAlgorithmIdToAlgorithm(AlgorithmId, &algorithm, HashSize);

    if (!NT_SUCCESS(status))
        return status;

    return PhSymCryptHashInit(algorithm, Context);
}

/**
 * Appends data to an incremental hash. HashSize identifies the algorithm
 * (as returned by PhSymCryptOpenAlgorithmProvider).
 */
NTSTATUS PhSymCryptHashDataBySize(
    _Inout_ PPH_SYMCRYPT_HASH_CONTEXT Context,
    _In_ ULONG HashSize,
    _In_reads_bytes_(Length) PCVOID Buffer,
    _In_ SIZE_T Length
    )
{
    if (!Context)
        return STATUS_INVALID_HANDLE;

    if (Context->ResultSize != HashSize)
        return STATUS_INVALID_PARAMETER_2;

    return PhSymCryptHashData(Context, Buffer, Length);
}

/**
 * Finalizes an incremental hash into Result. HashSize
 * identifies the algorithm (as returned by PhSymCryptOpenAlgorithmProvider)
 * and also denotes the size of the output buffer.
 */
NTSTATUS PhSymCryptFinishHash(
    _Inout_ PPH_SYMCRYPT_HASH_CONTEXT Context,
    _In_ ULONG HashSize,
    _Out_writes_bytes_(HashSize) PVOID Result
    )
{
    if (!Context)
        return STATUS_INVALID_HANDLE;

    if (Context->ResultSize != HashSize)
        return STATUS_INVALID_PARAMETER_2;

    return PhSymCryptHashFinal(Context, Result, HashSize);
}

// ------------------------------------------------------------------------
// HMAC (single-shot)
// ------------------------------------------------------------------------

/**
 * Generates a single-shot HMAC-<Tag> wrapper function.
 *
 * The expanded key lives on the stack and is wiped before return so the
 * derived ipad/opad material does not leak into surrounding frames.
 *
 * \param Tag Hash tag (Sha1, Sha256, Sha384, Sha512).
 * \param ExpandedKeyType SymCrypt expanded-key struct type.
 * \param ExpandFn SymCrypt key-expansion function.
 * \param MacFn SymCrypt single-shot MAC function.
 * \param ResultSize MAC output size in bytes.
 */
#define PH_SYMCRYPT_DEFINE_HMAC(Tag, ExpandedKeyType, ExpandFn, MacFn, ResultSize) \
    /** Single-shot HMAC-<Tag> over (Key, Data). */                             \
    VOID                                                                        \
    NTAPI                                                                       \
    PhSymCryptHmac##Tag(                                                        \
        _In_reads_bytes_(KeyLength) PCVOID Key,                                 \
        _In_ SIZE_T KeyLength,                                                  \
        _In_reads_bytes_(DataLength) PCVOID Data,                               \
        _In_ SIZE_T DataLength,                                                 \
        _Out_writes_bytes_(ResultSize) PVOID Result                             \
        )                                                                       \
    {                                                                           \
        ExpandedKeyType expandedKey;                                            \
        /* Expand the raw key into the algorithm's internal schedule. */        \
        ExpandFn(&expandedKey, (PCBYTE)Key, KeyLength);                         \
        /* Run the single-shot MAC. */                                          \
        MacFn(&expandedKey, (PCBYTE)Data, DataLength, (PBYTE)Result);           \
        /* Scrub the expanded key from the stack. */                            \
        SymCryptWipeKnownSize(&expandedKey, sizeof(expandedKey));               \
    }

PH_SYMCRYPT_DEFINE_HMAC(
    Sha1,
    SYMCRYPT_HMAC_SHA1_EXPANDED_KEY,
    SymCryptHmacSha1ExpandKey,
    SymCryptHmacSha1,
    PH_SYMCRYPT_HMAC_SHA1_RESULT_SIZE)

PH_SYMCRYPT_DEFINE_HMAC(
    Sha256,
    SYMCRYPT_HMAC_SHA256_EXPANDED_KEY,
    SymCryptHmacSha256ExpandKey,
    SymCryptHmacSha256,
    PH_SYMCRYPT_HMAC_SHA256_RESULT_SIZE)

PH_SYMCRYPT_DEFINE_HMAC(
    Sha384,
    SYMCRYPT_HMAC_SHA384_EXPANDED_KEY,
    SymCryptHmacSha384ExpandKey,
    SymCryptHmacSha384,
    PH_SYMCRYPT_HMAC_SHA384_RESULT_SIZE)

PH_SYMCRYPT_DEFINE_HMAC(
    Sha512,
    SYMCRYPT_HMAC_SHA512_EXPANDED_KEY,
    SymCryptHmacSha512ExpandKey,
    SymCryptHmacSha512,
    PH_SYMCRYPT_HMAC_SHA512_RESULT_SIZE)

// ------------------------------------------------------------------------
// KDFs
// ------------------------------------------------------------------------

/**
 * PBKDF2 password stretching using HMAC-SHA256 as the PRF (RFC 2898).
 *
 * \param[in] Password Password bytes to stretch.
 * \param[in] PasswordLength Password size in bytes.
 * \param[in] Salt Optional salt material.
 * \param[in] SaltLength Salt size; pass 0 if no salt.
 * \param[in] IterationCount PBKDF2 iteration count (higher = slower = safer).
 * \param[out] Result Derived key output.
 * \param[in] ResultLength Number of derived bytes to produce.
 * \return STATUS_SUCCESS on success; otherwise mapped NTSTATUS.
 */
NTSTATUS NTAPI PhSymCryptPbkdf2HmacSha256(
    _In_reads_bytes_(PasswordLength) PCVOID Password,
    _In_ SIZE_T PasswordLength,
    _In_reads_bytes_opt_(SaltLength) PCVOID Salt,
    _In_ SIZE_T SaltLength,
    _In_ ULONG64 IterationCount,
    _Out_writes_bytes_(ResultLength) PVOID Result,
    _In_ SIZE_T ResultLength
    )
{
    // Forward to SymCrypt's generic PBKDF2 with the HMAC-SHA256 PRF.

    return PhSymCryptErrorToStatus(SymCryptPbkdf2(
        SymCryptHmacSha256Algorithm,
        (PCBYTE)Password,
        PasswordLength,
        (PCBYTE)Salt,
        SaltLength,
        IterationCount,
        (PBYTE)Result,
        ResultLength
        ));
}

/**
 * PBKDF2 password stretching using HMAC-SHA512 as the PRF (RFC 2898).
 *
 * \param[in] Password Password bytes to stretch.
 * \param[in] PasswordLength Password size in bytes.
 * \param[in] Salt Optional salt material.
 * \param[in] SaltLength Salt size; pass 0 if no salt.
 * \param[in] IterationCount PBKDF2 iteration count.
 * \param[out] Result Derived key output.
 * \param[in] ResultLength Number of derived bytes to produce.
 * \return STATUS_SUCCESS on success; otherwise mapped NTSTATUS.
 */
NTSTATUS NTAPI PhSymCryptPbkdf2HmacSha512(
    _In_reads_bytes_(PasswordLength) PCVOID Password,
    _In_ SIZE_T PasswordLength,
    _In_reads_bytes_opt_(SaltLength) PCVOID Salt,
    _In_ SIZE_T SaltLength,
    _In_ ULONG64 IterationCount,
    _Out_writes_bytes_(ResultLength) PVOID Result,
    _In_ SIZE_T ResultLength
    )
{
    // Forward to SymCrypt's generic PBKDF2 with the HMAC-SHA512 PRF.

    return PhSymCryptErrorToStatus(SymCryptPbkdf2(
        SymCryptHmacSha512Algorithm,
        (PCBYTE)Password,
        PasswordLength,
        (PCBYTE)Salt,
        SaltLength,
        IterationCount,
        (PBYTE)Result,
        ResultLength
        ));
}

/**
 * HKDF extract-and-expand key derivation (RFC 5869) using HMAC-SHA256.
 *
 * \param[in] Ikm Input keying material.
 * \param[in] IkmLength IKM size.
 * \param[in] Salt Optional salt; if NULL/0 HKDF uses a zero salt.
 * \param[in] SaltLength Salt size.
 * \param[in] Info Optional context/info bytes.
 * \param[in] InfoLength Info size.
 * \param[out] Result Output keying material.
 * \param[in] ResultLength Bytes of output to produce (max 255 * 32 = 8160).
 * \return STATUS_SUCCESS on success; otherwise mapped NTSTATUS.
 */
NTSTATUS NTAPI PhSymCryptHkdfSha256(
    _In_reads_bytes_(IkmLength) PCVOID Ikm,
    _In_ SIZE_T IkmLength,
    _In_reads_bytes_opt_(SaltLength) PCVOID Salt,
    _In_ SIZE_T SaltLength,
    _In_reads_bytes_opt_(InfoLength) PCVOID Info,
    _In_ SIZE_T InfoLength,
    _Out_writes_bytes_(ResultLength) PVOID Result,
    _In_ SIZE_T ResultLength
    )
{
    // Forward to SymCrypt's generic HKDF with HMAC-SHA256.

    return PhSymCryptErrorToStatus(SymCryptHkdf(
        SymCryptHmacSha256Algorithm,
        (PCBYTE)Ikm,
        IkmLength,
        (PCBYTE)Salt,
        SaltLength,
        (PCBYTE)Info,
        InfoLength,
        (PBYTE)Result,
        ResultLength
        ));
}

/**
 * HKDF extract-and-expand key derivation (RFC 5869) using HMAC-SHA512.
 *
 * \param[in] Ikm Input keying material.
 * \param[in] IkmLength IKM size.
 * \param[in] Salt Optional salt; if NULL/0 HKDF uses a zero salt.
 * \param[in] SaltLength Salt size.
 * \param[in] Info Optional context/info bytes.
 * \param[in] InfoLength Info size.
 * \param[out] Result Output keying material.
 * \param[in] ResultLength Bytes of output (max 255 * 64 = 16320).
 * \return STATUS_SUCCESS on success; otherwise mapped NTSTATUS.
 */
NTSTATUS NTAPI PhSymCryptHkdfSha512(
    _In_reads_bytes_(IkmLength) PCVOID Ikm,
    _In_ SIZE_T IkmLength,
    _In_reads_bytes_opt_(SaltLength) PCVOID Salt,
    _In_ SIZE_T SaltLength,
    _In_reads_bytes_opt_(InfoLength) PCVOID Info,
    _In_ SIZE_T InfoLength,
    _Out_writes_bytes_(ResultLength) PVOID Result,
    _In_ SIZE_T ResultLength
    )
{
    // Forward to SymCrypt's generic HKDF with HMAC-SHA512.

    return PhSymCryptErrorToStatus(SymCryptHkdf(
        SymCryptHmacSha512Algorithm,
        (PCBYTE)Ikm,
        IkmLength,
        (PCBYTE)Salt,
        SaltLength,
        (PCBYTE)Info,
        InfoLength,
        (PBYTE)Result,
        ResultLength
        ));
}

// ------------------------------------------------------------------------
// AEAD
// ------------------------------------------------------------------------

/**
 * AES-GCM authenticated encryption (NIST SP 800-38D).
 *
 * AES-GCM provides both confidentiality and authenticity over Plaintext and
 * the (Nonce, AuthData) tuple. Reusing a (Key, Nonce) pair is catastrophic;
 * callers MUST generate a fresh nonce per encryption with a given key.
 *
 * \param[in] Key AES key (16, 24, or 32 bytes).
 * \param[in] KeyLength Key size.
 * \param[in] Nonce Nonce / IV (12 bytes recommended).
 * \param[in] NonceLength Nonce size.
 * \param[in] AuthData Optional associated data covered by the tag.
 * \param[in] AuthDataLength AuthData size.
 * \param[in] Plaintext Cleartext input.
 * \param[out] Ciphertext Output buffer (may alias Plaintext).
 * \param[in] DataLength Size of plaintext = size of ciphertext.
 * \param[out] Tag Authentication tag output.
 * \param[in] TagLength Tag length (12-16 bytes).
 * \return STATUS_SUCCESS on success; otherwise mapped NTSTATUS.
 */
NTSTATUS NTAPI PhSymCryptAesGcmEncrypt(
    _In_reads_bytes_(KeyLength) PCVOID Key,
    _In_ SIZE_T KeyLength,
    _In_reads_bytes_(NonceLength) PCVOID Nonce,
    _In_ SIZE_T NonceLength,
    _In_reads_bytes_opt_(AuthDataLength) PCVOID AuthData,
    _In_ SIZE_T AuthDataLength,
    _In_reads_bytes_(DataLength) PCVOID Plaintext,
    _Out_writes_bytes_(DataLength) PVOID Ciphertext,
    _In_ SIZE_T DataLength,
    _Out_writes_bytes_(TagLength) PVOID Tag,
    _In_ SIZE_T TagLength
    )
{
    SYMCRYPT_GCM_EXPANDED_KEY expandedKey;
    SYMCRYPT_ERROR error;

    //
    // Expand the raw AES key into GCM's internal precomputed tables.
    //

    error = SymCryptGcmExpandKey(
        &expandedKey,
        SymCryptAesBlockCipher,
        (PCBYTE)Key,
        KeyLength
        );

    if (error != SYMCRYPT_NO_ERROR)
        return PhSymCryptErrorToStatus(error);

    //
    // Validate nonce / data / tag sizes per NIST GCM rules.
    //

    error = SymCryptGcmValidateParameters(
        SymCryptAesBlockCipher,
        NonceLength,
        AuthDataLength,
        DataLength,
        TagLength
        );

    if (error != SYMCRYPT_NO_ERROR)
    {
        //
        // Bail out, but wipe the expanded key first.
        //

        SymCryptWipeKnownSize(&expandedKey, sizeof(expandedKey));

        return PhSymCryptErrorToStatus(error);
    }

    //
    // Encrypt and produce the auth tag in one combined operation.
    //

    SymCryptGcmEncrypt(
        &expandedKey,
        (PCBYTE)Nonce,
        NonceLength,
        (PCBYTE)AuthData,
        AuthDataLength,
        (PCBYTE)Plaintext,
        (PBYTE)Ciphertext,
        DataLength,
        (PBYTE)Tag,
        TagLength
        );

    //
    // Scrub the expanded key before returning.
    //

    SymCryptWipeKnownSize(&expandedKey, sizeof(expandedKey));

    return STATUS_SUCCESS;
}

/**
 * AES-GCM authenticated decryption (NIST SP 800-38D).
 *
 * Verifies the tag before exposing plaintext; returns STATUS_AUTH_TAG_MISMATCH
 * if the tag does not validate.
 *
 * \param[in] Key AES key.
 * \param[in] KeyLength Key size (16/24/32).
 * \param[in] Nonce Nonce that was used to encrypt.
 * \param[in] NonceLength Nonce size.
 * \param[in] AuthData Associated data that was covered by the tag.
 * \param[in] AuthDataLength AuthData size.
 * \param[in] Ciphertext Ciphertext input.
 * \param[out] Plaintext Decrypted output (may alias Ciphertext).
 * \param[in] DataLength Size of ciphertext = size of plaintext.
 * \param[in] Tag Auth tag to verify.
 * \param[in] TagLength Tag size.
 * \return STATUS_SUCCESS on tag verification + decryption; STATUS_AUTH_TAG_MISMATCH on tag failure.
 */
NTSTATUS NTAPI PhSymCryptAesGcmDecrypt(
    _In_reads_bytes_(KeyLength) PCVOID Key,
    _In_ SIZE_T KeyLength,
    _In_reads_bytes_(NonceLength) PCVOID Nonce,
    _In_ SIZE_T NonceLength,
    _In_reads_bytes_opt_(AuthDataLength) PCVOID AuthData,
    _In_ SIZE_T AuthDataLength,
    _In_reads_bytes_(DataLength) PCVOID Ciphertext,
    _Out_writes_bytes_(DataLength) PVOID Plaintext,
    _In_ SIZE_T DataLength,
    _In_reads_bytes_(TagLength) PCVOID Tag,
    _In_ SIZE_T TagLength
    )
{
    SYMCRYPT_GCM_EXPANDED_KEY expandedKey;
    SYMCRYPT_ERROR error;

    //
    // Expand the raw AES key into GCM's tables.
    //

    error = SymCryptGcmExpandKey(
        &expandedKey,
        SymCryptAesBlockCipher,
        (PCBYTE)Key,
        KeyLength
        );

    if (error != SYMCRYPT_NO_ERROR)
        return PhSymCryptErrorToStatus(error);

    //
    // Validate sizes before touching the data.
    //

    error = SymCryptGcmValidateParameters(
        SymCryptAesBlockCipher,
        NonceLength,
        AuthDataLength,
        DataLength,
        TagLength
        );

    if (error == SYMCRYPT_NO_ERROR)
    {
        //
        // Decrypt + verify tag in one combined operation.
        //

        error = SymCryptGcmDecrypt(
            &expandedKey,
            (PCBYTE)Nonce,
            NonceLength,
            (PCBYTE)AuthData,
            AuthDataLength,
            (PCBYTE)Ciphertext,
            (PBYTE)Plaintext,
            DataLength,
            (PCBYTE)Tag,
            TagLength
            );
    }

    //
    // Scrub the expanded key regardless of success / failure.
    //

    SymCryptWipeKnownSize(&expandedKey, sizeof(expandedKey));

    return PhSymCryptErrorToStatus(error);
}

/**
 * ChaCha20-Poly1305 authenticated encryption (RFC 8439).
 *
 * Modern AEAD primitive used by TLS 1.3 and WireGuard. Key/nonce/tag sizes
 * are fixed by the spec (32 / 12 / 16 bytes) so no validation step is needed.
 *
 * \param[in] Key 32-byte secret key.
 * \param[in] Nonce 12-byte unique nonce.
 * \param[in] AuthData Optional associated data.
 * \param[in] AuthDataLength AuthData size.
 * \param[in] Plaintext Cleartext input.
 * \param[out] Ciphertext Output buffer (may alias Plaintext).
 * \param[in] DataLength Size of plaintext.
 * \param[out] Tag 16-byte authentication tag.
 * \return STATUS_SUCCESS on success; otherwise mapped NTSTATUS.
 */
NTSTATUS NTAPI PhSymCryptChaCha20Poly1305Encrypt(
    _In_reads_bytes_(32) PCVOID Key,
    _In_reads_bytes_(12) PCVOID Nonce,
    _In_reads_bytes_opt_(AuthDataLength) PCVOID AuthData,
    _In_ SIZE_T AuthDataLength,
    _In_reads_bytes_(DataLength) PCVOID Plaintext,
    _Out_writes_bytes_(DataLength) PVOID Ciphertext,
    _In_ SIZE_T DataLength,
    _Out_writes_bytes_(16) PVOID Tag
    )
{
    // Single-shot ChaCha20-Poly1305 encrypt with fixed sizes baked in.

    return PhSymCryptErrorToStatus(SymCryptChaCha20Poly1305Encrypt(
        (PCBYTE)Key,
        32,
        (PCBYTE)Nonce,
        12,
        (PCBYTE)AuthData,
        AuthDataLength,
        (PCBYTE)Plaintext,
        (PBYTE)Ciphertext,
        DataLength,
        (PBYTE)Tag,
        16
        ));
}

/**
 * ChaCha20-Poly1305 authenticated decryption (RFC 8439).
 *
 * \param[in] Key 32-byte secret key.
 * \param[in] Nonce 12-byte nonce that was used to encrypt.
 * \param[in] AuthData Associated data that was covered by the tag.
 * \param[in] AuthDataLength AuthData size.
 * \param[in] Ciphertext Ciphertext input.
 * \param[out] Plaintext Decrypted output (may alias Ciphertext).
 * \param[in] DataLength Size of ciphertext.
 * \param[in] Tag 16-byte tag to verify.
 * \return STATUS_SUCCESS on tag verify + decrypt.
 * \remarks Returns STATUS_AUTH_TAG_MISMATCH on tag verification failure.
 */
NTSTATUS NTAPI PhSymCryptChaCha20Poly1305Decrypt(
    _In_reads_bytes_(32) PCVOID Key,
    _In_reads_bytes_(12) PCVOID Nonce,
    _In_reads_bytes_opt_(AuthDataLength) PCVOID AuthData,
    _In_ SIZE_T AuthDataLength,
    _In_reads_bytes_(DataLength) PCVOID Ciphertext,
    _Out_writes_bytes_(DataLength) PVOID Plaintext,
    _In_ SIZE_T DataLength,
    _In_reads_bytes_(16) PCVOID Tag
    )
{
    // Single-shot ChaCha20-Poly1305 decrypt + tag verification.

    return PhSymCryptErrorToStatus(SymCryptChaCha20Poly1305Decrypt(
        (PCBYTE)Key,
        32,
        (PCBYTE)Nonce,
        12,
        (PCBYTE)AuthData,
        AuthDataLength,
        (PCBYTE)Ciphertext,
        (PBYTE)Plaintext,
        DataLength,
        (PCBYTE)Tag,
        16
        ));
}

// ------------------------------------------------------------------------
// AES-CBC
//
// SymCrypt's CBC primitives operate on whole 16-byte blocks. The raw
// variants require DataLength to be a multiple of 16. The PKCS#7 variants
// apply / validate padding around the raw call.
// ------------------------------------------------------------------------

#define PH_AES_BLOCK_SIZE 16

/**
 * Internal helper: AES-CBC encrypt with no padding.
 *
 * Used as the building block for both the public raw wrapper and the
 * PKCS#7-padded wrapper. The IV is copied into a local chaining buffer so
 * the caller's IV is not mutated.
 *
 * \param[in] Key AES key bytes (16/24/32).
 * \param[in] KeyLength Key size.
 * \param[in] Iv 16-byte initialization vector.
 * \param[in] Plaintext Plaintext input (must be block-aligned).
 * \param[out] Ciphertext Ciphertext output (may alias Plaintext).
 * \param[in] DataLength Multiple-of-16 byte count.
 * \return STATUS_SUCCESS on success; STATUS_INVALID_PARAMETER if not aligned.
 */
NTSTATUS PhSymCryptAesCbcEncryptRaw(
    _In_reads_bytes_(KeyLength) PCVOID Key,
    _In_ SIZE_T KeyLength,
    _In_reads_bytes_(PH_AES_BLOCK_SIZE) PCVOID Iv,
    _In_reads_bytes_(DataLength) PCVOID Plaintext,
    _Out_writes_bytes_(DataLength) PVOID Ciphertext,
    _In_ SIZE_T DataLength
    )
{
    SYMCRYPT_AES_EXPANDED_KEY expandedKey;
    BYTE chainingValue[PH_AES_BLOCK_SIZE];
    SYMCRYPT_ERROR error;

    //
    // CBC requires a whole-block input length.
    //

    if ((DataLength % PH_AES_BLOCK_SIZE) != 0)
        return STATUS_INVALID_PARAMETER;

    //
    // Expand AES key schedule.
    //

    error = SymCryptAesExpandKey(
        &expandedKey,
        (PCBYTE)Key,
        KeyLength
        );

    if (error != SYMCRYPT_NO_ERROR)
        return PhSymCryptErrorToStatus(error);

    //
    // Copy IV into the in/out chaining buffer (SymCrypt mutates this).
    //

    memcpy(chainingValue, Iv, PH_AES_BLOCK_SIZE);

    //
    // Run the CBC encrypt across all blocks.
    //

    SymCryptAesCbcEncrypt(
        &expandedKey,
        chainingValue,
        (PCBYTE)Plaintext,
        (PBYTE)Ciphertext,
        DataLength
        );

    //
    // Scrub key schedule and chaining state before returning.
    //

    SymCryptWipeKnownSize(&expandedKey, sizeof(expandedKey));
    SymCryptWipeKnownSize(chainingValue, sizeof(chainingValue));
    return STATUS_SUCCESS;
}

/**
 * Internal helper: AES-CBC decrypt with no padding.
 * Mirror of \ref PhSymCryptAesCbcEncryptRaw used for both raw and PKCS#7
 *
 * \param[in] Key AES key bytes.
 * \param[in] KeyLength Key size (16/24/32).
 * \param[in] Iv 16-byte initialization vector.
 * \param[in] Ciphertext Ciphertext input (must be block-aligned).
 * \param[out] Plaintext Plaintext output (may alias Ciphertext).
 * \param[in] DataLength Multiple-of-16 byte count.
 * \return STATUS_SUCCESS on success; STATUS_INVALID_PARAMETER if not aligned.
 */
NTSTATUS PhSymCryptAesCbcDecryptRaw(
    _In_reads_bytes_(KeyLength) PCVOID Key,
    _In_ SIZE_T KeyLength,
    _In_reads_bytes_(PH_AES_BLOCK_SIZE) PCVOID Iv,
    _In_reads_bytes_(DataLength) PCVOID Ciphertext,
    _Out_writes_bytes_(DataLength) PVOID Plaintext,
    _In_ SIZE_T DataLength
    )
{
    SYMCRYPT_AES_EXPANDED_KEY expandedKey;
    BYTE chainingValue[PH_AES_BLOCK_SIZE];
    SYMCRYPT_ERROR error;

    //
    // Whole-block check.
    //

    if ((DataLength % PH_AES_BLOCK_SIZE) != 0)
        return STATUS_INVALID_PARAMETER;

    //
    // Expand AES key schedule.
    //

    error = SymCryptAesExpandKey(
        &expandedKey,
        (PCBYTE)Key,
        KeyLength
        );

    if (error != SYMCRYPT_NO_ERROR)
        return PhSymCryptErrorToStatus(error);

    //
    // Copy IV into in/out chaining buffer.
    //

    memcpy(chainingValue, Iv, PH_AES_BLOCK_SIZE);

    //
    // Run CBC decrypt across all blocks.
    //

    SymCryptAesCbcDecrypt(
        &expandedKey,
        chainingValue,
        (PCBYTE)Ciphertext,
        (PBYTE)Plaintext,
        DataLength
        );

    //
    // Scrub key schedule and chaining state.
    //

    SymCryptWipeKnownSize(&expandedKey, sizeof(expandedKey));
    SymCryptWipeKnownSize(chainingValue, sizeof(chainingValue));

    return STATUS_SUCCESS;
}

/**
 * Public AES-CBC encrypt (no padding).
 * Caller must pass block-aligned data. Use \ref PhSymCryptAesCbcEncryptPkcs7
 * for unaligned messages.
 *
 * \param[in] Key AES key.
 * \param[in] KeyLength Key size (16/24/32).
 * \param[in] Iv 16-byte IV.
 * \param[in] Plaintext Plaintext input.
 * \param[out] Ciphertext Ciphertext output (may alias Plaintext).
 * \param[in] DataLength Multiple-of-16 byte count.
 * \return STATUS_SUCCESS or STATUS_INVALID_PARAMETER if not aligned.
 */
NTSTATUS NTAPI PhSymCryptAesCbcEncrypt(
    _In_reads_bytes_(KeyLength) PCVOID Key,
    _In_ SIZE_T KeyLength,
    _In_reads_bytes_(16) PCVOID Iv,
    _In_reads_bytes_(DataLength) PCVOID Plaintext,
    _Out_writes_bytes_(DataLength) PVOID Ciphertext,
    _In_ SIZE_T DataLength
    )
{
    return PhSymCryptAesCbcEncryptRaw(
        Key,
        KeyLength,
        Iv,
        Plaintext,
        Ciphertext,
        DataLength
        );
}

/**
 * Public AES-CBC decrypt (no padding).
 * Mirror of \ref PhSymCryptAesCbcEncrypt; use the PKCS#7 variant for
 * messages that were padded on encrypt.
 *
 * \param[in] Key AES key.
 * \param[in] KeyLength Key size (16/24/32).
 * \param[in] Iv 16-byte IV used at encrypt time.
 * \param[in] Ciphertext Ciphertext input.
 * \param[out] Plaintext Plaintext output (may alias Ciphertext).
 * \param[in] DataLength Multiple-of-16 byte count.
 * \return STATUS_SUCCESS or STATUS_INVALID_PARAMETER if not aligned.
 */
NTSTATUS NTAPI PhSymCryptAesCbcDecrypt(
    _In_reads_bytes_(KeyLength) PCVOID Key,
    _In_ SIZE_T KeyLength,
    _In_reads_bytes_(16) PCVOID Iv,
    _In_reads_bytes_(DataLength) PCVOID Ciphertext,
    _Out_writes_bytes_(DataLength) PVOID Plaintext,
    _In_ SIZE_T DataLength
    )
{
    return PhSymCryptAesCbcDecryptRaw(
        Key,
        KeyLength,
        Iv,
        Ciphertext,
        Plaintext,
        DataLength
        );
}

/**
 * AES-CBC encrypt with PKCS#7 padding.
 *
 * Appends 1..16 padding bytes so that the message becomes block-aligned,
 * then encrypts. The pad byte value equals the pad length (PKCS#7 / PKCS#5).
 *
 * \param[in] Key AES key.
 * \param[in] KeyLength Key size (16/24/32).
 * \param[in] Iv 16-byte IV.
 * \param[in] Plaintext Plaintext input of any length.
 * \param[in] PlaintextLength Plaintext size in bytes.
 * \param[out] Ciphertext Output buffer; must hold at least ((PlaintextLength / 16) + 1) * 16 bytes.
 * \param[in] CiphertextCapacity Size of the output buffer.
 * \param[out] CiphertextLength Actual bytes written.
 * \return STATUS_SUCCESS, STATUS_BUFFER_TOO_SMALL if capacity is short, or a mapped error.
 */
NTSTATUS NTAPI PhSymCryptAesCbcEncryptPkcs7(
    _In_reads_bytes_(KeyLength) PCVOID Key,
    _In_ SIZE_T KeyLength,
    _In_reads_bytes_(16) PCVOID Iv,
    _In_reads_bytes_(PlaintextLength) PCVOID Plaintext,
    _In_ SIZE_T PlaintextLength,
    _Out_writes_bytes_to_(CiphertextCapacity, *CiphertextLength) PVOID Ciphertext,
    _In_ SIZE_T CiphertextCapacity,
    _Out_ PSIZE_T CiphertextLength
    )
{
    SIZE_T padLen;
    SIZE_T paddedLength;
    PBYTE buffer;
    NTSTATUS status;

    //
    // Default out-param to 0 so failure paths stay clean.
    //

    *CiphertextLength = 0;

    //
    // PKCS#7 pad length: always 1..16, even when input is already aligned.
    //

    padLen = PH_AES_BLOCK_SIZE - (PlaintextLength % PH_AES_BLOCK_SIZE);
    paddedLength = PlaintextLength + padLen;

    //
    // Guard against integer overflow on the addition above.
    //

    if (paddedLength < PlaintextLength)
        return STATUS_INVALID_PARAMETER;

    //
    // Caller must provide a large enough output buffer.
    //

    if (CiphertextCapacity < paddedLength)
        return STATUS_BUFFER_TOO_SMALL;

    //
    // Stage plaintext + padding in the output buffer.
    //

    buffer = (PBYTE)Ciphertext;
    memcpy(buffer, Plaintext, PlaintextLength);

    //
    // Fill the trailing padding bytes with padLen (PKCS#7 rule).
    //

    for (SIZE_T i = PlaintextLength; i < paddedLength; i++)
    {
        buffer[i] = (BYTE)padLen;
    }

    //
    // Encrypt in-place over the padded buffer.
    //

    status = PhSymCryptAesCbcEncryptRaw(
        Key, KeyLength, Iv,
        buffer, buffer, paddedLength
        );

    if (!NT_SUCCESS(status))
    {
        //
        // On failure, scrub partial state so callers don't leak plaintext.
        //

        SymCryptWipe(buffer, paddedLength);
        return status;
    }

    *CiphertextLength = paddedLength;
    return STATUS_SUCCESS;
}

/**
 * AES-CBC decrypt with PKCS#7 padding removal.
 *
 * Decrypts then validates and strips PKCS#7 padding in constant time to
 * avoid a padding-oracle side channel.
 *
 * \param[in] Key AES key.
 * \param[in] KeyLength Key size (16/24/32).
 * \param[in] Iv 16-byte IV used at encrypt time.
 * \param[in] Ciphertext Ciphertext input (must be block-aligned).
 * \param[in] CiphertextLength Ciphertext size, multiple of 16.
 * \param[out] Plaintext Plaintext output (may alias Ciphertext).
 * \param[out] PlaintextLength Actual plaintext bytes written (= CT len - pad).
 * \return STATUS_SUCCESS on valid padding; STATUS_INVALID_PARAMETER on bad padding or non-aligned input.
 */
NTSTATUS NTAPI PhSymCryptAesCbcDecryptPkcs7(
    _In_reads_bytes_(KeyLength) PCVOID Key,
    _In_ SIZE_T KeyLength,
    _In_reads_bytes_(16) PCVOID Iv,
    _In_reads_bytes_(CiphertextLength) PCVOID Ciphertext,
    _In_ SIZE_T CiphertextLength,
    _Out_writes_bytes_to_(CiphertextLength, *PlaintextLength) PVOID Plaintext,
    _Out_ PSIZE_T PlaintextLength
    )
{
    PBYTE buffer;
    BYTE padLen;
    BYTE invalid;
    NTSTATUS status;

    //
    // Default out-param to 0 for early-error paths.
    //

    *PlaintextLength = 0;

    //
    // PKCS#7 requires a non-empty, block-aligned ciphertext.
    //

    if (CiphertextLength == 0 || (CiphertextLength % PH_AES_BLOCK_SIZE) != 0)
        return STATUS_INVALID_PARAMETER;

    //
    // Decrypt all blocks (still padded).
    //

    status = PhSymCryptAesCbcDecryptRaw(
        Key, KeyLength, Iv,
        Ciphertext, Plaintext, CiphertextLength
        );

    if (!NT_SUCCESS(status))
        return status;

    //
    // Read the claimed pad length from the last decrypted byte.
    //

    buffer = (PBYTE)Plaintext;
    padLen = buffer[CiphertextLength - 1];

    //
    // Constant-time padding validation (avoid PKCS#7 padding oracle):
    //   1) check 1 <= padLen <= 16 via subtraction-into-sign-bit;
    //   2) check the trailing padLen bytes all equal padLen.
    //

    invalid = (BYTE)(((padLen - 1) | (PH_AES_BLOCK_SIZE - padLen)) >> 7);

    for (SIZE_T i = 0; i < PH_AES_BLOCK_SIZE; i++)
    {
        BYTE expected = (BYTE)padLen;
        BYTE actual = buffer[CiphertextLength - 1 - i];
        // mask = 0xFF only for indexes that fall inside the claimed pad span.
        BYTE mask = (BYTE)((((SIZE_T)i < (SIZE_T)padLen) ? 0xFF : 0x00));
        invalid |= (BYTE)((actual ^ expected) & mask);
    }

    if (invalid != 0)
    {
        //
        // Padding bad — scrub decrypted output so the caller can't see it.
        //

        SymCryptWipe(buffer, CiphertextLength);

        return STATUS_INVALID_PARAMETER;
    }

    //
    // Padding good — strip it from the reported length.
    //

    *PlaintextLength = CiphertextLength - padLen;

    return STATUS_SUCCESS;
}

// ------------------------------------------------------------------------
// Asymmetric verification
// ------------------------------------------------------------------------

/**
 * Map our wrapper hash-algorithm enum to a SymCrypt hash descriptor.
 * Used by primitives (PSS, ECDSA) that take a PCSYMCRYPT_HASH for MGF/hash binding.
 *
 * \param[in] Algorithm Hash algorithm selector.
 * \return Pointer to a static SymCrypt hash descriptor, or NULL if unknown.
 */
PCSYMCRYPT_HASH PhSymCryptHashAlgorithmToHash(
    _In_ PH_SYMCRYPT_HASH_ALGORITHM Algorithm
    )
{
    switch (Algorithm)
    {
    case PH_SYMCRYPT_SHA1_ALGORITHM:
        return SymCryptSha1Algorithm;
    case PH_SYMCRYPT_SHA256_ALGORITHM:
        return SymCryptSha256Algorithm;
    case PH_SYMCRYPT_SHA384_ALGORITHM:
        return SymCryptSha384Algorithm;
    case PH_SYMCRYPT_SHA512_ALGORITHM:
        return SymCryptSha512Algorithm;
    default:
        return NULL;
    }
}

/**
 * Map our wrapper hash-algorithm enum to its DER OID list.
 *
 * RSA PKCS#1 v1.5 signatures embed the hash algorithm as a DER-encoded OID
 * inside the DigestInfo structure; SymCrypt expects a list of acceptable
 * OID encodings (some algorithms have multiple historical OIDs).
 *
 * \param[in] Algorithm Hash algorithm selector.
 * \param[out] OidList Receives pointer to SymCrypt's OID array.
 * \param[out] OidCount Receives OID count.
 * \return TRUE if mapping succeeded; FALSE for unknown algorithms.
 */
BOOLEAN PhSymCryptHashAlgorithmToOidList(
    _In_ PH_SYMCRYPT_HASH_ALGORITHM Algorithm,
    _Out_ PCSYMCRYPT_OID* OidList,
    _Out_ SIZE_T* OidCount
    )
{
    switch (Algorithm)
    {
    case PH_SYMCRYPT_SHA1_ALGORITHM:
        *OidList = SymCryptSha1OidList;
        *OidCount = SYMCRYPT_SHA1_OID_COUNT;
        return TRUE;
    case PH_SYMCRYPT_SHA256_ALGORITHM:
        *OidList = SymCryptSha256OidList;
        *OidCount = SYMCRYPT_SHA256_OID_COUNT;
        return TRUE;
    case PH_SYMCRYPT_SHA384_ALGORITHM:
        *OidList = SymCryptSha384OidList;
        *OidCount = SYMCRYPT_SHA384_OID_COUNT;
        return TRUE;
    case PH_SYMCRYPT_SHA512_ALGORITHM:
        *OidList = SymCryptSha512OidList;
        *OidCount = SYMCRYPT_SHA512_OID_COUNT;
        return TRUE;
    default:
        *OidList = NULL;
        *OidCount = 0;
        return FALSE;
    }
}

/**
 * Allocate a SymCrypt RSAKEY for public-key (verify-only) use.
 * Caller is responsible for freeing via SymCryptRsakeyFree.
 *
 * \param[in] Modulus Modulus bytes (MSB-first).
 * \param[in] ModulusLength Modulus size in bytes (1..UINT32_MAX/8).
 * \param[in] PublicExponent Public exponent (commonly 65537).
 * \param[out] RsaKey Receives the allocated RSAKEY pointer.
 * \return STATUS_SUCCESS or an error NTSTATUS. RsaKey is NULL on failure.
 */
NTSTATUS PhSymCryptRsaImportPublicKey(
    _In_reads_bytes_(ModulusLength) PCVOID Modulus,
    _In_ SIZE_T ModulusLength,
    _In_ ULONG64 PublicExponent,
    _Outptr_ PSYMCRYPT_RSAKEY* RsaKey
    )
{
    SYMCRYPT_RSA_PARAMS params;
    PSYMCRYPT_RSAKEY key;
    SYMCRYPT_ERROR error;

    //
    // Default out-pointer to NULL so failure paths are safe.
    //

    *RsaKey = NULL;

    //
    // Bound the modulus size to keep nBitsOfModulus inside UINT32.
    //

    if (ModulusLength == 0 || ModulusLength > (UINT32_MAX / 8))
        return STATUS_INVALID_PARAMETER;

    //
    // Build RSA params: public-only, one exponent.
    //

    params.version = 1;
    params.nBitsOfModulus = (UINT32)(ModulusLength * 8);
    params.nPrimes = 0;
    params.nPubExp = 1;

    //
    // Allocate RSAKEY storage.
    //

    key = SymCryptRsakeyAllocate(&params, 0);

    if (!key)
        return STATUS_INSUFFICIENT_RESOURCES;

    //
    // Import modulus and public exponent (MSB-first big-endian).
    //

    error = SymCryptRsakeySetValue(
        (PCBYTE)Modulus,
        ModulusLength,
        &PublicExponent,
        1,
        NULL,
        NULL,
        0,
        SYMCRYPT_NUMBER_FORMAT_MSB_FIRST,
        SYMCRYPT_FLAG_RSAKEY_SIGN,
        key
        );

    if (error != SYMCRYPT_NO_ERROR)
    {
        //
        // Free on import failure so we don't leak.
        //

        SymCryptRsakeyFree(key);

        return PhSymCryptErrorToStatus(error);
    }

    *RsaKey = key;
    return STATUS_SUCCESS;
}

/**
 * Verify an RSA PKCS#1 v1.5 signature.
 *
 * \param[in] Modulus Modulus (MSB-first).
 * \param[in] ModulusLength Modulus size in bytes.
 * \param[in] PublicExponent RSA public exponent.
 * \param[in] HashAlgorithm Algorithm used for the inner hash.
 * \param[in] Hash Pre-computed hash of the signed message.
 * \param[in] HashLength Hash size.
 * \param[in] Signature Signature bytes (MSB-first).
 * \param[in] SignatureLength Signature size; must equal ModulusLength.
 * \return STATUS_SUCCESS on verify; STATUS_INVALID_SIGNATURE on mismatch.
 */
NTSTATUS NTAPI PhSymCryptRsaPkcs1Verify(
    _In_reads_bytes_(ModulusLength) PCVOID Modulus,
    _In_ SIZE_T ModulusLength,
    _In_ ULONG64 PublicExponent,
    _In_ PH_SYMCRYPT_HASH_ALGORITHM HashAlgorithm,
    _In_reads_bytes_(HashLength) PCVOID Hash,
    _In_ SIZE_T HashLength,
    _In_reads_bytes_(SignatureLength) PCVOID Signature,
    _In_ SIZE_T SignatureLength
    )
{
    PSYMCRYPT_RSAKEY key;
    PCSYMCRYPT_OID oidList;
    SIZE_T oidCount;
    SYMCRYPT_ERROR error;
    NTSTATUS status;

    //
    // Resolve the DER OID list for the hash algorithm.
    //

    if (!PhSymCryptHashAlgorithmToOidList(HashAlgorithm, &oidList, &oidCount))
        return STATUS_INVALID_PARAMETER;

    //
    // Build a public-only RSA key from modulus / exponent.
    //
    status = PhSymCryptRsaImportPublicKey(
        Modulus,
        ModulusLength,
        PublicExponent,
        &key
        );

    if (!NT_SUCCESS(status))
        return status;

    //
    // Verify the PKCS#1 v1.5 signature.
    //

    error = SymCryptRsaPkcs1Verify(
        key,
        (PCBYTE)Hash,
        HashLength,
        (PCBYTE)Signature,
        SignatureLength,
        SYMCRYPT_NUMBER_FORMAT_MSB_FIRST,
        oidList,
        oidCount,
        0);

    //
    // Release the RSA key.
    //

    SymCryptRsakeyFree(key);

    return PhSymCryptErrorToStatus(error);
}

/**
 * Verify an RSA PSS signature (PKCS#1 v2.1).
 *
 * \param[in] Modulus Modulus (MSB-first).
 * \param[in] ModulusLength Modulus size in bytes.
 * \param[in] PublicExponent RSA public exponent.
 * \param[in] HashAlgorithm Hash used for the message digest and MGF1.
 * \param[in] SaltLength PSS salt length used at sign time.
 * \param[in] Hash Pre-computed message hash.
 * \param[in] HashLength Hash size.
 * \param[in] Signature Signature bytes (MSB-first).
 * \param[in] SignatureLength Signature size; must equal ModulusLength.
 * \return STATUS_SUCCESS on verify; STATUS_INVALID_SIGNATURE on mismatch.
 */
NTSTATUS NTAPI PhSymCryptRsaPssVerify(
    _In_reads_bytes_(ModulusLength) PCVOID Modulus,
    _In_ SIZE_T ModulusLength,
    _In_ ULONG64 PublicExponent,
    _In_ PH_SYMCRYPT_HASH_ALGORITHM HashAlgorithm,
    _In_ SIZE_T SaltLength,
    _In_reads_bytes_(HashLength) PCVOID Hash,
    _In_ SIZE_T HashLength,
    _In_reads_bytes_(SignatureLength) PCVOID Signature,
    _In_ SIZE_T SignatureLength
    )
{
    PSYMCRYPT_RSAKEY key;
    PCSYMCRYPT_HASH hashAlg;
    SYMCRYPT_ERROR error;
    NTSTATUS status;

    //
    // Resolve the SymCrypt hash descriptor (also used as MGF1 hash).
    //

    hashAlg = PhSymCryptHashAlgorithmToHash(HashAlgorithm);

    if (!hashAlg)
        return STATUS_INVALID_PARAMETER;

    //
    // Build a public-only RSA key from modulus / exponent.
    //

    status = PhSymCryptRsaImportPublicKey(
        Modulus,
        ModulusLength,
        PublicExponent,
        &key
        );

    if (!NT_SUCCESS(status))
        return status;

    //
    // Verify the PSS signature.
    //

    error = SymCryptRsaPssVerify(
        key,
        (PCBYTE)Hash,
        HashLength,
        (PCBYTE)Signature,
        SignatureLength,
        SYMCRYPT_NUMBER_FORMAT_MSB_FIRST,
        hashAlg,
        SaltLength,
        0
        );

    //
    // Release the RSA key.
    //

    SymCryptRsakeyFree(key);

    return PhSymCryptErrorToStatus(error);
}

/**
 * Internal helper: ECDSA verify on a curve parameter set.
 *
 * Allocates a fresh ECURVE + ECKEY, imports the public point, then calls
 * SymCryptEcDsaVerify. Used by both P-256 and P-384 verify entry points.
 *
 * \param[in] CurveParams SymCrypt curve parameter table.
 * \param[in] PublicKeyXY Public key X || Y (MSB-first, uncompressed).
 * \param[in] PublicKeyLength Size of the X||Y blob.
 * \param[in] Hash Pre-computed message hash.
 * \param[in] HashLength Hash size.
 * \param[in] Signature R || S signature bytes.
 * \param[in] SignatureLength Signature size.
 * \return STATUS_SUCCESS on verify; STATUS_INVALID_SIGNATURE on mismatch.
 */
NTSTATUS PhSymCryptEcDsaVerify(
    _In_ PCSYMCRYPT_ECURVE_PARAMS CurveParams,
    _In_reads_bytes_(PublicKeyLength) PCVOID PublicKeyXY,
    _In_ SIZE_T PublicKeyLength,
    _In_reads_bytes_(HashLength) PCVOID Hash,
    _In_ SIZE_T HashLength,
    _In_reads_bytes_(SignatureLength) PCVOID Signature,
    _In_ SIZE_T SignatureLength
    )
{
    PSYMCRYPT_ECURVE curve;
    PSYMCRYPT_ECKEY key;
    SYMCRYPT_ERROR error;

    //
    // Build the SymCrypt curve descriptor (e.g. NIST P-256).
    //

    curve = SymCryptEcurveAllocate(CurveParams, 0);

    if (!curve)
        return STATUS_INSUFFICIENT_RESOURCES;

    //
    // Allocate an ECKEY bound to that curve.
    //

    key = SymCryptEckeyAllocate(curve);

    if (!key)
    {
        SymCryptEcurveFree(curve);
        return STATUS_INSUFFICIENT_RESOURCES;
    }

    //
    // Import the public point (no private key for verify).
    //

    error = SymCryptEckeySetValue(
        NULL, 0,
        (PCBYTE)PublicKeyXY,
        PublicKeyLength,
        SYMCRYPT_NUMBER_FORMAT_MSB_FIRST,
        SYMCRYPT_ECPOINT_FORMAT_XY,
        SYMCRYPT_FLAG_ECKEY_ECDSA,
        key
        );

    if (error == SYMCRYPT_NO_ERROR)
    {
        //
        // Run ECDSA verify against the provided R || S signature.
        //

        error = SymCryptEcDsaVerify(
            key,
            (PCBYTE)Hash,
            HashLength,
            (PCBYTE)Signature,
            SignatureLength,
            SYMCRYPT_NUMBER_FORMAT_MSB_FIRST,
            0);
    }

    //
    // Free the key and the curve.
    //

    SymCryptEckeyFree(key);
    SymCryptEcurveFree(curve);

    return PhSymCryptErrorToStatus(error);
}

/**
 * Verify an ECDSA signature on the NIST P-256 curve.
 *
 * \param[in] PublicKeyXY Public key X || Y (64 bytes, MSB-first).
 * \param[in] PublicKeyLength Must be 64.
 * \param[in] Hash Pre-computed message hash (typically SHA-256).
 * \param[in] HashLength Hash size.
 * \param[in] Signature R || S signature (64 bytes).
 * \param[in] SignatureLength Must be 64.
 * \return STATUS_SUCCESS on verify; STATUS_INVALID_SIGNATURE on mismatch.
 */
NTSTATUS NTAPI PhSymCryptEcDsaVerifyP256(
    _In_reads_bytes_(PublicKeyLength) PCVOID PublicKeyXY,
    _In_ SIZE_T PublicKeyLength,
    _In_reads_bytes_(HashLength) PCVOID Hash,
    _In_ SIZE_T HashLength,
    _In_reads_bytes_(SignatureLength) PCVOID Signature,
    _In_ SIZE_T SignatureLength
    )
{
    return PhSymCryptEcDsaVerify(
        SymCryptEcurveParamsNistP256,
        PublicKeyXY,
        PublicKeyLength,
        Hash,
        HashLength,
        Signature,
        SignatureLength
        );
}

/**
 * Verify an ECDSA signature on the NIST P-384 curve.
 *
 * \param[in] PublicKeyXY Public key X || Y (96 bytes, MSB-first).
 * \param[in] PublicKeyLength Must be 96.
 * \param[in] Hash Pre-computed message hash (typically SHA-384).
 * \param[in] HashLength Hash size.
 * \param[in] Signature R || S signature (96 bytes).
 * \param[in] SignatureLength Must be 96.
 * \return STATUS_SUCCESS on verify; STATUS_INVALID_SIGNATURE on mismatch.
 */
NTSTATUS NTAPI PhSymCryptEcDsaVerifyP384(
    _In_reads_bytes_(PublicKeyLength) PCVOID PublicKeyXY,
    _In_ SIZE_T PublicKeyLength,
    _In_reads_bytes_(HashLength) PCVOID Hash,
    _In_ SIZE_T HashLength,
    _In_reads_bytes_(SignatureLength) PCVOID Signature,
    _In_ SIZE_T SignatureLength
    )
{
    return PhSymCryptEcDsaVerify(
        SymCryptEcurveParamsNistP384,
        PublicKeyXY,
        PublicKeyLength,
        Hash,
        HashLength,
        Signature,
        SignatureLength
        );
}

// ------------------------------------------------------------------------
// Asymmetric verification (BCrypt blob compatibility)
//
// Accept raw BCRYPT_*_BLOB byte buffers as produced by BCryptExportKey and
// as embedded verbatim in source (e.g. plugins/Updater/verify.c hard-coded
// trusted public keys). Parse the header in-place and dispatch to the raw
// primitives above.
// ------------------------------------------------------------------------

/**
 * Translate a BCRYPT_*_ALGORITHM hash identifier wstring to our enum.
 * Used by the BCrypt-blob signature dispatcher to interpret PaddingInfo
 * structures supplied by Win32 callers.
 *
 * \param[in] AlgId BCrypt algorithm identifier string (e.g. L"SHA256").
 * \param[out] Algorithm Receives the matching enum value.
 * \return STATUS_SUCCESS on a known algorithm; STATUS_NOT_SUPPORTED otherwise.
 */
NTSTATUS PhSymCryptHashAlgIdToEnum(
    _In_ PCWSTR AlgId,
    _Out_ PH_SYMCRYPT_HASH_ALGORITHM* Algorithm
    )
{
    if (PhEqualStringZ(AlgId, BCRYPT_SHA1_ALGORITHM, FALSE))
    {
        *Algorithm = PH_SYMCRYPT_SHA1_ALGORITHM;
        return STATUS_SUCCESS;
    }

    if (PhEqualStringZ(AlgId, BCRYPT_SHA256_ALGORITHM, FALSE))
    {
        *Algorithm = PH_SYMCRYPT_SHA256_ALGORITHM;
        return STATUS_SUCCESS;
    }

    if (PhEqualStringZ(AlgId, BCRYPT_SHA384_ALGORITHM, FALSE))
    {
        *Algorithm = PH_SYMCRYPT_SHA384_ALGORITHM;
        return STATUS_SUCCESS;
    }

    if (PhEqualStringZ(AlgId, BCRYPT_SHA512_ALGORITHM, FALSE))
    {
        *Algorithm = PH_SYMCRYPT_SHA512_ALGORITHM;
        return STATUS_SUCCESS;
    }

    return STATUS_NOT_SUPPORTED;
}

/**
 * Verify an ECDSA signature given a BCRYPT_ECCKEY_BLOB public-key blob.
 *
 * Parses the BCrypt header to pick the curve (P-256 vs P-384), extracts the
 * X || Y point, then forwards to the curve-specific verify wrapper.
 *
 * \param[in] KeyBlob Pointer to a BCRYPT_ECCKEY_BLOB header followed by X || Y.
 * \param[in] KeyBlobLength Total blob length.
 * \param[in] Hash Pre-computed message hash.
 * \param[in] HashLength Hash size.
 * \param[in] Signature R || S signature bytes.
 * \param[in] SignatureLength Signature size.
 * \return STATUS_SUCCESS on verify; STATUS_INVALID_PARAMETER for malformed blobs;
 * STATUS_NOT_SUPPORTED for unrecognized curves.
 */
NTSTATUS NTAPI PhSymCryptEcDsaVerifyBlob(
    _In_reads_bytes_(KeyBlobLength) PCVOID KeyBlob,
    _In_ SIZE_T KeyBlobLength,
    _In_reads_bytes_(HashLength) PCVOID Hash,
    _In_ SIZE_T HashLength,
    _In_reads_bytes_(SignatureLength) PCVOID Signature,
    _In_ SIZE_T SignatureLength
    )
{
    const BCRYPT_ECCKEY_BLOB* header;
    const BYTE* publicKey;
    SIZE_T publicKeyLength;
    BOOLEAN isP256;

    //
    // Need at least enough bytes for the header itself.
    //

    if (KeyBlobLength < sizeof(BCRYPT_ECCKEY_BLOB))
        return STATUS_INVALID_PARAMETER;

    header = (const BCRYPT_ECCKEY_BLOB*)KeyBlob;

    //
    // Magic value selects the curve; cbKey must match curve component size.
    //

    switch (header->dwMagic)
    {
    case BCRYPT_ECDSA_PUBLIC_P256_MAGIC:
    case BCRYPT_ECDH_PUBLIC_P256_MAGIC:
        if (header->cbKey != 32)
            return STATUS_INVALID_PARAMETER;
        isP256 = TRUE;
        break;
    case BCRYPT_ECDSA_PUBLIC_P384_MAGIC:
    case BCRYPT_ECDH_PUBLIC_P384_MAGIC:
        if (header->cbKey != 48)
            return STATUS_INVALID_PARAMETER;
        isP256 = FALSE;
        break;
    default:
        // Unknown curve / blob magic.
        return STATUS_NOT_SUPPORTED;
    }

    //
    // Public key (X || Y) immediately follows the header, total = 2 * cbKey.
    //

    publicKeyLength = (SIZE_T)header->cbKey * 2;

    //
    // Validate the rest of the blob is present.
    //

    if (KeyBlobLength < sizeof(BCRYPT_ECCKEY_BLOB) + publicKeyLength)
        return STATUS_INVALID_PARAMETER;

    publicKey = (const BYTE*)KeyBlob + sizeof(BCRYPT_ECCKEY_BLOB);

    //
    // Dispatch to the right curve-specific verify wrapper.
    //

    if (isP256)
    {
        return PhSymCryptEcDsaVerifyP256(
            publicKey,
            publicKeyLength,
            Hash,
            HashLength,
            Signature,
            SignatureLength
            );
    }
    else
    {
        return PhSymCryptEcDsaVerifyP384(
            publicKey,
            publicKeyLength,
            Hash,
            HashLength,
            Signature,
            SignatureLength
            );
    }
}

/**
 * Verify an RSA signature given a BCRYPT_RSAKEY_BLOB public-key blob.
 *
 * Parses the BCrypt header, extracts the public exponent (big-endian into
 * a ULONG64) and the modulus, then forwards to the PKCS#1 or PSS verifier.
 *
 * \param[in] KeyBlob BCRYPT_RSAKEY_BLOB (Magic = RSAPUBLIC) + exponent + modulus.
 * \param[in] KeyBlobLength Total blob length.
 * \param[in] HashAlgorithm Hash algorithm used at sign time.
 * \param[in] PaddingFlags BCRYPT_PAD_PSS or BCRYPT_PAD_PKCS1.
 * \param[in] SaltLength PSS salt length (ignored for PKCS#1).
 * \param[in] Hash Pre-computed message hash.
 * \param[in] HashLength Hash size.
 * \param[in] Signature Signature bytes.
 * \param[in] SignatureLength Signature size; must equal modulus length.
 * \return STATUS_SUCCESS on verify; an error NTSTATUS otherwise.
 */
NTSTATUS NTAPI PhSymCryptRsaVerifyBlob(
    _In_reads_bytes_(KeyBlobLength) PCVOID KeyBlob,
    _In_ SIZE_T KeyBlobLength,
    _In_ PH_SYMCRYPT_HASH_ALGORITHM HashAlgorithm,
    _In_ ULONG PaddingFlags,
    _In_ SIZE_T SaltLength,
    _In_reads_bytes_(HashLength) PCVOID Hash,
    _In_ SIZE_T HashLength,
    _In_reads_bytes_(SignatureLength) PCVOID Signature,
    _In_ SIZE_T SignatureLength
    )
{
    const BCRYPT_RSAKEY_BLOB* header;
    const BYTE* exponent;
    const BYTE* modulus;
    ULONG64 publicExponent;
    SIZE_T expectedSize;

    //
    // Need enough bytes for the header struct.
    //

    if (KeyBlobLength < sizeof(BCRYPT_RSAKEY_BLOB))
        return STATUS_INVALID_PARAMETER;

    header = (const BCRYPT_RSAKEY_BLOB*)KeyBlob;

    //
    // Only public key blobs are handled here.
    //

    if (header->Magic != BCRYPT_RSAPUBLIC_MAGIC)
        return STATUS_NOT_SUPPORTED;

    //
    // Exponent must fit a ULONG64.
    //

    if (header->cbPublicExp == 0 || header->cbPublicExp > sizeof(ULONG64))
        return STATUS_INVALID_PARAMETER;

    //
    // Modulus must be non-empty.
    //

    if (header->cbModulus == 0)
        return STATUS_INVALID_PARAMETER;

    //
    // Validate the full blob length holds exponent + modulus.
    //

    expectedSize = sizeof(BCRYPT_RSAKEY_BLOB) + header->cbPublicExp + header->cbModulus;

    if (KeyBlobLength < expectedSize)
        return STATUS_INVALID_PARAMETER;

    //
    // Locate exponent and modulus byte arrays.
    //

    exponent = (const BYTE*)KeyBlob + sizeof(BCRYPT_RSAKEY_BLOB);
    modulus = exponent + header->cbPublicExp;

    //
    // Decode the exponent (MSB-first) into a ULONG64.
    //

    publicExponent = 0;
    for (ULONG i = 0; i < header->cbPublicExp; i++)
    {
        publicExponent = (publicExponent << 8) | exponent[i];
    }

    //
    // Pick the verifier based on the padding flag the caller requested.
    //

    switch (PaddingFlags)
    {
    case BCRYPT_PAD_PSS:
        return PhSymCryptRsaPssVerify(
            modulus,
            header->cbModulus,
            publicExponent,
            HashAlgorithm,
            SaltLength,
            Hash,
            HashLength,
            Signature,
            SignatureLength
            );

    case BCRYPT_PAD_PKCS1:
        return PhSymCryptRsaPkcs1Verify(
            modulus,
            header->cbModulus,
            publicExponent,
            HashAlgorithm,
            Hash,
            HashLength,
            Signature,
            SignatureLength
            );

    default:
        // Unsupported padding flag.
        return STATUS_NOT_SUPPORTED;
    }
}

/**
 * Drop-in replacement for BCryptVerifySignature using SymCrypt internally.
 *
 * Dispatches on BlobType and PaddingFlags / PaddingInfo, then forwards to the
 * appropriate ECDSA or RSA verifier. Used by code paths that already speak
 * the BCrypt API and want to swap out only the verifier implementation.
 *
 * \param[in] BlobType BCRYPT_ECCPUBLIC_BLOB or BCRYPT_RSAPUBLIC_BLOB.
 * \param[in] KeyBlob Key blob bytes.
 * \param[in] KeyBlobLength Blob length.
 * \param[in] PaddingInfo BCRYPT_PSS_PADDING_INFO* / BCRYPT_PKCS1_PADDING_INFO* / NULL.
 * \param[in] PaddingFlags 0 (ECDSA), BCRYPT_PAD_PSS, or BCRYPT_PAD_PKCS1.
 * \param[in] Hash Message hash.
 * \param[in] HashLength Hash size.
 * \param[in] Signature Signature bytes.
 * \param[in] SignatureLength Signature length.
 * \return STATUS_SUCCESS on verify; STATUS_NOT_SUPPORTED for unhandled shapes;
 * STATUS_INVALID_SIGNATURE on mismatch.
 */
NTSTATUS NTAPI PhSymCryptVerifySignature(
    _In_ PCWSTR BlobType,
    _In_reads_bytes_(KeyBlobLength) PCVOID KeyBlob,
    _In_ SIZE_T KeyBlobLength,
    _In_opt_ PVOID PaddingInfo,
    _In_ ULONG PaddingFlags,
    _In_reads_bytes_(HashLength) PCVOID Hash,
    _In_ SIZE_T HashLength,
    _In_reads_bytes_(SignatureLength) PCVOID Signature,
    _In_ SIZE_T SignatureLength
    )
{
    //
    // ECDSA path: blob type ECCPUBLIC, padding flags ignored.
    //

    if (PhEqualStringZ(BlobType, BCRYPT_ECCPUBLIC_BLOB, FALSE))
    {
        return PhSymCryptEcDsaVerifyBlob(
            KeyBlob,
            KeyBlobLength,
            Hash,
            HashLength,
            Signature,
            SignatureLength
            );
    }

    //
    // RSA path: PaddingInfo encodes the hash algorithm + (PSS only) salt length.
    //

    if (PhEqualStringZ(BlobType, BCRYPT_RSAPUBLIC_BLOB, FALSE))
    {
        PH_SYMCRYPT_HASH_ALGORITHM hashAlgorithm;
        SIZE_T saltLength = 0;
        NTSTATUS status;

        if (PaddingFlags == BCRYPT_PAD_PSS)
        {
            const BCRYPT_PSS_PADDING_INFO* pss;

            //
            // PSS requires the caller-supplied padding info struct.
            //

            if (!PaddingInfo)
                return STATUS_INVALID_PARAMETER;

            pss = (const BCRYPT_PSS_PADDING_INFO*)PaddingInfo;

            //
            // Translate the BCrypt hash ID string -> our enum.
            //

            status = PhSymCryptHashAlgIdToEnum(pss->pszAlgId, &hashAlgorithm);

            if (!NT_SUCCESS(status))
                return status;

            //
            // Carry the salt length through to the verifier.
            //

            saltLength = pss->cbSalt;
        }
        else if (PaddingFlags == BCRYPT_PAD_PKCS1)
        {
            const BCRYPT_PKCS1_PADDING_INFO* pkcs1;

            //
            // PKCS#1 v1.5 also requires the padding info (only carries algId).
            //

            if (!PaddingInfo)
                return STATUS_INVALID_PARAMETER;

            pkcs1 = (const BCRYPT_PKCS1_PADDING_INFO*)PaddingInfo;

            //
            // Translate the BCrypt hash ID string -> our enum.
            //

            status = PhSymCryptHashAlgIdToEnum(pkcs1->pszAlgId, &hashAlgorithm);

            if (!NT_SUCCESS(status))
                return status;
        }
        else
        {
            //
            // Unsupported padding mode for an RSA blob.
            //

            return STATUS_NOT_SUPPORTED;
        }

        //
        // Forward to the RSA blob verifier with the resolved fields.
        //

        return PhSymCryptRsaVerifyBlob(
            KeyBlob,
            KeyBlobLength,
            hashAlgorithm,
            PaddingFlags,
            saltLength,
            Hash,
            HashLength,
            Signature,
            SignatureLength
            );
    }

    //
    // Unknown blob type.
    //

    return STATUS_NOT_SUPPORTED;
}

// ------------------------------------------------------------------------
// Asymmetric signing (BCrypt blob input)
//
// Accept BCRYPT_RSAFULLPRIVATE_BLOB / BCRYPT_RSAPRIVATE_BLOB / BCRYPT_ECCPRIVATE_BLOB
// as produced by BCryptExportKey. Only the fields SymCrypt actually needs
// for signing are extracted (RSA: modulus, public exponent, primes; ECDSA:
// private scalar d, plus the public point for keypair validation).
// ------------------------------------------------------------------------

/**
 * Internal helper: import an RSA private key from a BCrypt blob.
 *
 * Accepts both BCRYPT_RSAPRIVATE_BLOB (Modulus + Prime1 + Prime2) and
 * BCRYPT_RSAFULLPRIVATE_BLOB (adds Exp1/Exp2/Coefficient/PrivateExp). Only
 * the fields SymCrypt requires for signing (modulus, exponent, primes) are
 * extracted; trailing fields are ignored.  Caller frees the returned key
 * with SymCryptRsakeyFree.
 *
 * \param[in] KeyBlob BCrypt private blob.
 * \param[in] KeyBlobLength Blob length.
 * \param[out] RsaKey Receives the allocated RSAKEY pointer.
 * \param[out] ModulusLength Receives the modulus length (used to size signatures).
 * \return STATUS_SUCCESS or an error NTSTATUS. RsaKey is NULL on failure.
 */
NTSTATUS PhSymCryptRsaImportPrivateKey(
    _In_reads_bytes_(KeyBlobLength) PCVOID KeyBlob,
    _In_ SIZE_T KeyBlobLength,
    _Outptr_ PSYMCRYPT_RSAKEY* RsaKey,
    _Out_ PSIZE_T ModulusLength
    )
{
    const BCRYPT_RSAKEY_BLOB* header;
    const BYTE* exponent;
    const BYTE* modulus;
    const BYTE* prime1;
    const BYTE* prime2;
    ULONG cbPrime1;
    ULONG cbPrime2;
    ULONG64 publicExponent;
    SIZE_T expectedSize;
    SYMCRYPT_RSA_PARAMS params;
    PSYMCRYPT_RSAKEY key;
    PCBYTE primePtrs[2];
    SIZE_T primeSizes[2];
    SYMCRYPT_ERROR error;

    //
    // Initialize out-params for failure paths.
    //

    *RsaKey = NULL;
    *ModulusLength = 0;

    //
    // Need at least the BCrypt header.
    //

    if (KeyBlobLength < sizeof(BCRYPT_RSAKEY_BLOB))
        return STATUS_INVALID_PARAMETER;

    header = (const BCRYPT_RSAKEY_BLOB*)KeyBlob;

    //
    // Accept either the private or full-private variant; reject anything else.
    //

    if (header->Magic != BCRYPT_RSAPRIVATE_MAGIC && header->Magic != BCRYPT_RSAFULLPRIVATE_MAGIC)
        return STATUS_NOT_SUPPORTED;

    //
    // Exponent must fit into ULONG64.
    //

    if (header->cbPublicExp == 0 || header->cbPublicExp > sizeof(ULONG64))
        return STATUS_INVALID_PARAMETER;

    //
    // Modulus must be non-empty and produce a UINT32 bit count.
    //

    if (header->cbModulus == 0 || header->cbModulus > (UINT32_MAX / 8))
        return STATUS_INVALID_PARAMETER;

    //
    // Both primes are required for sign import.
    //

    if (header->cbPrime1 == 0 || header->cbPrime2 == 0)
        return STATUS_INVALID_PARAMETER;

    cbPrime1 = header->cbPrime1;
    cbPrime2 = header->cbPrime2;

    //
    // Compute the minimum byte count for the fields we need (PublicExp..Prime2).
    //

    expectedSize =
        sizeof(BCRYPT_RSAKEY_BLOB) +
        header->cbPublicExp +
        header->cbModulus +
        cbPrime1 +
        cbPrime2;

    if (KeyBlobLength < expectedSize)
        return STATUS_INVALID_PARAMETER;

    //
    // Locate the four blob fields by offset.
    //

    exponent = (const BYTE*)KeyBlob + sizeof(BCRYPT_RSAKEY_BLOB);
    modulus = exponent + header->cbPublicExp;
    prime1 = modulus + header->cbModulus;
    prime2 = prime1 + cbPrime1;

    //
    // Decode public exponent (MSB-first big-endian) into a ULONG64.
    //

    publicExponent = 0;

    for (ULONG i = 0; i < header->cbPublicExp; i++)
    {
        publicExponent = (publicExponent << 8) | exponent[i];
    }

    //
    // Build RSA params for a 2-prime private key.
    //

    params.version = 1;
    params.nBitsOfModulus = (UINT32)(header->cbModulus * 8);
    params.nPrimes = 2;
    params.nPubExp = 1;

    //
    // Allocate the RSAKEY object.
    //

    key = SymCryptRsakeyAllocate(&params, 0);

    if (!key)
        return STATUS_INSUFFICIENT_RESOURCES;

    //
    // Marshal primes into the array-of-pointer-and-size form SymCrypt wants.
    //

    primePtrs[0] = prime1;
    primePtrs[1] = prime2;
    primeSizes[0] = cbPrime1;
    primeSizes[1] = cbPrime2;

    //
    // Import the full private key material.
    //

    error = SymCryptRsakeySetValue(
        (PCBYTE)modulus,
        header->cbModulus,
        &publicExponent,
        1,
        primePtrs,
        primeSizes,
        2,
        SYMCRYPT_NUMBER_FORMAT_MSB_FIRST,
        SYMCRYPT_FLAG_RSAKEY_SIGN,
        key
        );

    if (error != SYMCRYPT_NO_ERROR)
    {
        //
        // Import failed — free the key and surface the error.
        //

        SymCryptRsakeyFree(key);

        return PhSymCryptErrorToStatus(error);
    }

    *RsaKey = key;
    *ModulusLength = header->cbModulus;
    return STATUS_SUCCESS;
}

/**
 * Sign a hash with RSA PKCS#1 v1.5 using a BCrypt private blob.
 *
 * \param[in] KeyBlob BCRYPT_RSAPRIVATE_BLOB / BCRYPT_RSAFULLPRIVATE_BLOB.
 * \param[in] KeyBlobLength Blob length.
 * \param[in] HashAlgorithm Hash algorithm whose DigestInfo OID will be embedded.
 * \param[in] Hash Pre-computed message hash.
 * \param[in] HashLength Hash size.
 * \param[out] Signature Output buffer for the signature (>= modulus length).
 * \param[in] SignatureCapacity Output buffer capacity.
 * \param[out] SignatureLength Bytes written (= modulus length).
 * \return STATUS_SUCCESS, STATUS_BUFFER_TOO_SMALL if capacity is short, or a mapped error.
 */
NTSTATUS NTAPI PhSymCryptRsaPkcs1SignBlob(
    _In_reads_bytes_(KeyBlobLength) PCVOID KeyBlob,
    _In_ SIZE_T KeyBlobLength,
    _In_ PH_SYMCRYPT_HASH_ALGORITHM HashAlgorithm,
    _In_reads_bytes_(HashLength) PCVOID Hash,
    _In_ SIZE_T HashLength,
    _Out_writes_bytes_to_(SignatureCapacity, *SignatureLength) PVOID Signature,
    _In_ SIZE_T SignatureCapacity,
    _Out_ PSIZE_T SignatureLength
    )
{
    PSYMCRYPT_RSAKEY key;
    PCSYMCRYPT_OID oidList;
    SIZE_T oidCount;
    SIZE_T modulusLength;
    SIZE_T producedLength;
    SYMCRYPT_ERROR error;
    NTSTATUS status;

    //
    // Reset out-param so error paths report 0 bytes written.
    //

    *SignatureLength = 0;

    //
    // Resolve the DigestInfo OID for the chosen hash.
    //

    if (!PhSymCryptHashAlgorithmToOidList(HashAlgorithm, &oidList, &oidCount))
        return STATUS_INVALID_PARAMETER;

    //
    // Build a private RSA key from the BCrypt blob.
    //

    status = PhSymCryptRsaImportPrivateKey(
        KeyBlob,
        KeyBlobLength,
        &key,
        &modulusLength
        );

    if (!NT_SUCCESS(status))
        return status;

    //
    // Verify the caller's signature buffer is large enough.
    //

    if (SignatureCapacity < modulusLength)
    {
        SymCryptRsakeyFree(key);
        return STATUS_BUFFER_TOO_SMALL;
    }

    producedLength = modulusLength;

    //
    // Produce the PKCS#1 v1.5 signature.
    //

    error = SymCryptRsaPkcs1Sign(
        key,
        (PCBYTE)Hash,
        HashLength,
        oidList,
        oidCount,
        0,
        SYMCRYPT_NUMBER_FORMAT_MSB_FIRST,
        (PBYTE)Signature,
        SignatureCapacity,
        &producedLength
        );

    //
    // Release private key material immediately after signing.
    //

    SymCryptRsakeyFree(key);

    if (error != SYMCRYPT_NO_ERROR)
        return PhSymCryptErrorToStatus(error);

    *SignatureLength = producedLength;

    return STATUS_SUCCESS;
}

/**
 * Sign a hash with RSA PSS using a BCrypt private blob.
 *
 * \param[in] KeyBlob BCRYPT_RSAPRIVATE_BLOB / BCRYPT_RSAFULLPRIVATE_BLOB.
 * \param[in] KeyBlobLength Blob length.
 * \param[in] HashAlgorithm Hash used for the message digest and MGF1.
 * \param[in] SaltLength PSS salt length to use.
 * \param[in] Hash Pre-computed message hash.
 * \param[in] HashLength Hash size.
 * \param[out] Signature Output buffer (>= modulus length).
 * \param[in] SignatureCapacity Output buffer capacity.
 * \param[out] SignatureLength Bytes written.
 * \return STATUS_SUCCESS, STATUS_BUFFER_TOO_SMALL, or a mapped error.
 */
NTSTATUS NTAPI PhSymCryptRsaPssSignBlob(
    _In_reads_bytes_(KeyBlobLength) PCVOID KeyBlob,
    _In_ SIZE_T KeyBlobLength,
    _In_ PH_SYMCRYPT_HASH_ALGORITHM HashAlgorithm,
    _In_ SIZE_T SaltLength,
    _In_reads_bytes_(HashLength) PCVOID Hash,
    _In_ SIZE_T HashLength,
    _Out_writes_bytes_to_(SignatureCapacity, *SignatureLength) PVOID Signature,
    _In_ SIZE_T SignatureCapacity,
    _Out_ PSIZE_T SignatureLength
    )
{
    PSYMCRYPT_RSAKEY key;
    PCSYMCRYPT_HASH hashAlg;
    SIZE_T modulusLength;
    SIZE_T producedLength;
    SYMCRYPT_ERROR error;
    NTSTATUS status;

    //
    // Reset out-param so error paths report 0 bytes written.
    //

    *SignatureLength = 0;

    //
    // Resolve the SymCrypt hash descriptor (also used as MGF1 hash).
    //

    hashAlg = PhSymCryptHashAlgorithmToHash(HashAlgorithm);

    if (!hashAlg)
        return STATUS_INVALID_PARAMETER;

    //
    // Build a private RSA key from the BCrypt blob.
    //

    status = PhSymCryptRsaImportPrivateKey(
        KeyBlob,
        KeyBlobLength,
        &key,
        &modulusLength
        );

    if (!NT_SUCCESS(status))
        return status;

    //
    // Verify the caller's signature buffer is large enough.
    //

    if (SignatureCapacity < modulusLength)
    {
        SymCryptRsakeyFree(key);
        return STATUS_BUFFER_TOO_SMALL;
    }

    producedLength = modulusLength;

    //
    // Produce the PSS signature.
    //

    error = SymCryptRsaPssSign(
        key,
        (PCBYTE)Hash,
        HashLength,
        hashAlg,
        SaltLength,
        0,
        SYMCRYPT_NUMBER_FORMAT_MSB_FIRST,
        (PBYTE)Signature,
        SignatureCapacity,
        &producedLength
        );

    //
    // Release private key material immediately after signing.
    //

    SymCryptRsakeyFree(key);

    if (error != SYMCRYPT_NO_ERROR)
        return PhSymCryptErrorToStatus(error);

    *SignatureLength = producedLength;

    return STATUS_SUCCESS;
}

/**
 * Sign a hash with ECDSA on NIST P-256 using a BCRYPT_ECCPRIVATE_BLOB.
 *
 * The blob layout is: BCRYPT_ECCKEY_BLOB header (Magic = ECDSA private P-256,
 * cbKey = 32) followed by X || Y || D, each 32 bytes MSB-first. SymCrypt's
 * SetValue receives both d and the public point so it can validate the keypair.
 *
 * \param[in] KeyBlob BCRYPT_ECCPRIVATE_BLOB blob.
 * \param[in] KeyBlobLength Blob length (>= 8 + 96).
 * \param[in] Hash Pre-computed message hash (typically SHA-256).
 * \param[in] HashLength Hash size.
 * \param[out] Signature 64-byte buffer for R || S signature.
 * \return STATUS_SUCCESS, STATUS_NOT_SUPPORTED for wrong magic / curve, or a mapped error.
 */
NTSTATUS NTAPI PhSymCryptEcDsaSignP256Blob(
    _In_reads_bytes_(KeyBlobLength) PCVOID KeyBlob,
    _In_ SIZE_T KeyBlobLength,
    _In_reads_bytes_(HashLength) PCVOID Hash,
    _In_ SIZE_T HashLength,
    _Out_writes_bytes_(64) PVOID Signature
    )
{
    const BCRYPT_ECCKEY_BLOB* header;
    const BYTE* publicKeyXY;
    const BYTE* privateKeyD;
    PSYMCRYPT_ECURVE curve;
    PSYMCRYPT_ECKEY key;
    SYMCRYPT_ERROR error;

    //
    // Blob must hold at least header + 32+32+32 = 96 bytes for P-256 private.
    //

    if (KeyBlobLength < sizeof(BCRYPT_ECCKEY_BLOB) + 96)
        return STATUS_INVALID_PARAMETER;

    header = (const BCRYPT_ECCKEY_BLOB*)KeyBlob;

    //
    // Only the ECDSA-private P-256 magic is accepted here.
    //

    if (header->dwMagic != BCRYPT_ECDSA_PRIVATE_P256_MAGIC)
        return STATUS_NOT_SUPPORTED;

    //
    // cbKey is the component size for the curve (32 for P-256).
    //

    if (header->cbKey != 32)
        return STATUS_INVALID_PARAMETER;

    //
    // Layout after header: X (32) || Y (32) || D (32).
    //

    publicKeyXY = (const BYTE*)KeyBlob + sizeof(BCRYPT_ECCKEY_BLOB);
    privateKeyD = publicKeyXY + 64;

    //
    // Allocate P-256 curve descriptor.
    //

    curve = SymCryptEcurveAllocate(SymCryptEcurveParamsNistP256, 0);

    if (!curve)
        return STATUS_INSUFFICIENT_RESOURCES;

    //
    // Allocate an ECKEY bound to that curve.
    //

    key = SymCryptEckeyAllocate(curve);

    if (!key)
    {
        SymCryptEcurveFree(curve);
        return STATUS_INSUFFICIENT_RESOURCES;
    }

    //
    // Import both the private scalar and public point; SymCrypt validates
    // the keypair consistency when both halves are supplied.
    //

    error = SymCryptEckeySetValue(
        privateKeyD,
        32,
        publicKeyXY,
        64,
        SYMCRYPT_NUMBER_FORMAT_MSB_FIRST,
        SYMCRYPT_ECPOINT_FORMAT_XY,
        SYMCRYPT_FLAG_ECKEY_ECDSA,
        key
        );

    if (error == SYMCRYPT_NO_ERROR)
    {
        //
        // Produce the deterministic-format R || S signature (64 bytes).
        //

        error = SymCryptEcDsaSign(
            key,
            (PCBYTE)Hash,
            HashLength,
            SYMCRYPT_NUMBER_FORMAT_MSB_FIRST,
            0,
            (PBYTE)Signature,
            64
            );
    }

    //
    // Release ECKEY and curve regardless of outcome.
    //

    SymCryptEckeyFree(key);
    SymCryptEcurveFree(curve);

    return PhSymCryptErrorToStatus(error);
}







