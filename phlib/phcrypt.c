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
#include <phcrypt.h>
#include "..\tools\thirdparty\SymCrypt\inc\symcrypt.h"
#include <bcrypt.h>
#include <ntintsafe.h>

#ifndef BCRYPT_SHA3_224_ALGORITHM
#define BCRYPT_SHA3_224_ALGORITHM L"SHA3-224"
#endif
#ifndef BCRYPT_SHA3_256_ALGORITHM
#define BCRYPT_SHA3_256_ALGORITHM L"SHA3-256"
#endif
#ifndef BCRYPT_SHA3_384_ALGORITHM
#define BCRYPT_SHA3_384_ALGORITHM L"SHA3-384"
#endif
#ifndef BCRYPT_SHA3_512_ALGORITHM
#define BCRYPT_SHA3_512_ALGORITHM L"SHA3-512"
#endif
#ifndef BCRYPT_SHAKE128_ALGORITHM
#define BCRYPT_SHAKE128_ALGORITHM L"SHAKE128"
#endif
#ifndef BCRYPT_SHAKE256_ALGORITHM
#define BCRYPT_SHAKE256_ALGORITHM L"SHAKE256"
#endif
#ifndef BCRYPT_AES_CMAC_ALGORITHM
#define BCRYPT_AES_CMAC_ALGORITHM L"AES-CMAC"
#endif
#ifndef BCRYPT_AES_GMAC_ALGORITHM
#define BCRYPT_AES_GMAC_ALGORITHM L"AES-GMAC"
#endif
#ifndef BCRYPT_KMAC128_ALGORITHM
#define BCRYPT_KMAC128_ALGORITHM L"KMAC128"
#endif
#ifndef BCRYPT_KMAC256_ALGORITHM
#define BCRYPT_KMAC256_ALGORITHM L"KMAC256"
#endif
#ifndef BCRYPT_SHA512_256_ALGORITHM
#define BCRYPT_SHA512_256_ALGORITHM L"SHA512-256"
#endif
#ifndef BCRYPT_PBKDF2_ALGORITHM
#define BCRYPT_PBKDF2_ALGORITHM L"PBKDF2"
#endif
#ifndef BCRYPT_SP800108_CTR_HMAC_ALGORITHM
#define BCRYPT_SP800108_CTR_HMAC_ALGORITHM L"SP800_108_CTR_HMAC"
#endif
#ifndef BCRYPT_TLS1_1_KDF_ALGORITHM
#define BCRYPT_TLS1_1_KDF_ALGORITHM L"TLS1_1_KDF"
#endif
#ifndef BCRYPT_TLS1_2_KDF_ALGORITHM
#define BCRYPT_TLS1_2_KDF_ALGORITHM L"TLS1_2_KDF"
#endif
#ifndef BCRYPT_ECDSA_P384_ALGORITHM
#define BCRYPT_ECDSA_P384_ALGORITHM L"ECDSA_P384"
#endif
#ifndef BCRYPT_ECDSA_P521_ALGORITHM
#define BCRYPT_ECDSA_P521_ALGORITHM L"ECDSA_P521"
#endif
#ifndef BCRYPT_ECDH_P256_ALGORITHM
#define BCRYPT_ECDH_P256_ALGORITHM L"ECDH_P256"
#endif
#ifndef BCRYPT_ECDH_P384_ALGORITHM
#define BCRYPT_ECDH_P384_ALGORITHM L"ECDH_P384"
#endif
#ifndef BCRYPT_ECDH_P521_ALGORITHM
#define BCRYPT_ECDH_P521_ALGORITHM L"ECDH_P521"
#endif
#ifndef BCRYPT_CHAIN_MODE_NA
#define BCRYPT_CHAIN_MODE_NA L"ChainingModeN/A"
#endif
#ifndef BCRYPT_CHAINING_MODE
#define BCRYPT_CHAINING_MODE L"ChainingMode"
#endif
#ifndef BCRYPT_CHAIN_MODE_ECB
#define BCRYPT_CHAIN_MODE_ECB L"ChainingModeECB"
#endif
#ifndef BCRYPT_CHAIN_MODE_CBC
#define BCRYPT_CHAIN_MODE_CBC L"ChainingModeCBC"
#endif
#ifndef BCRYPT_CHAIN_MODE_CFB
#define BCRYPT_CHAIN_MODE_CFB L"ChainingModeCFB"
#endif
#ifndef BCRYPT_CHAIN_MODE_GCM
#define BCRYPT_CHAIN_MODE_GCM L"ChainingModeGCM"
#endif
#ifndef BCRYPT_CHAIN_MODE_CCM
#define BCRYPT_CHAIN_MODE_CCM L"ChainingModeCCM"
#endif

// ------------------------------------------------------------------------
// One-time SymCrypt library initialization
// ------------------------------------------------------------------------

/**
 * Performs one-time SymCrypt runtime initialization.
 *
 * The statically linked SymCrypt user-mode library on Windows requires a
 * one-time call to SymCryptInit(). This initializes the runtime environment,
 * including CPU feature detection via SymCryptInitEnvWindowsUsermode().
 * If initialization is skipped, g_SymCryptCpuFeaturesNotPresent remains at its
 * default value (~0, i.e. "no features present"), forcing all primitives to use
 * their lowest-performance fallback implementations.
 * For example, SHA-256 falls back to the SSSE3 implementation instead of using
 * SHA-NI (SHANI) instructions, resulting in ~40% lower throughput compared to
 * properly initialized SymCrypt or BCrypt. SymCryptInit() is idempotent and
 * internally guarded by SYMCRYPT_FLAG_LIB_INITIALIZED.
 */
VOID PhSymCryptInitialize(
    VOID
    )
{
    SymCryptInit();
}

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
    case SYMCRYPT_WRONG_KEY_SIZE: return STATUS_INVALID_PARAMETER;
    case SYMCRYPT_WRONG_BLOCK_SIZE: return STATUS_INVALID_PARAMETER;
    case SYMCRYPT_WRONG_DATA_SIZE: return STATUS_INVALID_PARAMETER;
    case SYMCRYPT_WRONG_NONCE_SIZE: return STATUS_INVALID_PARAMETER;
    case SYMCRYPT_WRONG_TAG_SIZE: return STATUS_INVALID_PARAMETER;
    case SYMCRYPT_WRONG_ITERATION_COUNT: return STATUS_INVALID_PARAMETER;
    // AEAD tag verification failed (GCM, ChaCha20-Poly1305).
    case SYMCRYPT_AUTHENTICATION_FAILURE: return STATUS_AUTH_TAG_MISMATCH;
    // Asymmetric signature verification failed (RSA, ECDSA).
    case SYMCRYPT_SIGNATURE_VERIFICATION_FAILURE: return STATUS_INVALID_SIGNATURE;
    case SYMCRYPT_MEMORY_ALLOCATION_FAILURE: return STATUS_INSUFFICIENT_RESOURCES;
    case SYMCRYPT_NOT_IMPLEMENTED: return STATUS_NOT_IMPLEMENTED;
    case SYMCRYPT_BUFFER_TOO_SMALL: return STATUS_BUFFER_TOO_SMALL;
    case SYMCRYPT_INVALID_BLOB: return STATUS_INVALID_PARAMETER;
    case SYMCRYPT_INVALID_ARGUMENT: return STATUS_INVALID_PARAMETER;
    case SYMCRYPT_INCOMPATIBLE_FORMAT: return STATUS_NOT_SUPPORTED;
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

    if (!Buffer)
        return STATUS_INVALID_PARAMETER;

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

NTSTATUS NTAPI PhSymCryptRdseedStatus(
    VOID
    )
{
#if SYMCRYPT_CPU_X86 | SYMCRYPT_CPU_AMD64
    return PhSymCryptErrorToStatus(SymCryptRdseedStatus());
#else
    return STATUS_NOT_IMPLEMENTED;
#endif
}

NTSTATUS NTAPI PhSymCryptRdseedGetBytes(
    _Out_writes_bytes_(Length) PVOID Buffer,
    _In_ SIZE_T Length
    )
{
#if SYMCRYPT_CPU_X86 | SYMCRYPT_CPU_AMD64
    if (!Buffer)
        return STATUS_INVALID_PARAMETER;

    if ((Length % 16) != 0)
        return STATUS_INVALID_PARAMETER;

    return PhSymCryptErrorToStatus(SymCryptRdseedGetBytes((PBYTE)Buffer, Length));
#else
    return STATUS_NOT_IMPLEMENTED;
#endif
}

NTSTATUS NTAPI PhSymCryptGenRandom(
    _Out_writes_bytes_(Length) PVOID Buffer,
    _In_ SIZE_T Length,
    _In_ ULONG Flags
    )
{
    if (!Buffer)
        return STATUS_INVALID_PARAMETER;

    if (Flags != 0)
        return STATUS_NOT_SUPPORTED;

    SymCryptRandom((PBYTE)Buffer, Length);
    return STATUS_SUCCESS;
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

VOID NTAPI PhSymCryptSha224(
    _In_reads_bytes_(Length) PCVOID Buffer,
    _In_ SIZE_T Length,
    _Out_writes_bytes_(PH_SYMCRYPT_SHA224_RESULT_SIZE) PVOID Result
    )
{
    SymCryptSha224((PCBYTE)Buffer, Length, (PBYTE)Result);
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

VOID NTAPI PhSymCryptSha512_224(
    _In_reads_bytes_(Length) PCVOID Buffer,
    _In_ SIZE_T Length,
    _Out_writes_bytes_(PH_SYMCRYPT_SHA512_224_RESULT_SIZE) PVOID Result
    )
{
    SymCryptSha512_224((PCBYTE)Buffer, Length, (PBYTE)Result);
}

VOID NTAPI PhSymCryptSha512_256(
    _In_reads_bytes_(Length) PCVOID Buffer,
    _In_ SIZE_T Length,
    _Out_writes_bytes_(PH_SYMCRYPT_SHA512_256_RESULT_SIZE) PVOID Result
    )
{
    SymCryptSha512_256((PCBYTE)Buffer, Length, (PBYTE)Result);
}

VOID NTAPI PhSymCryptSha3_224(
    _In_reads_bytes_(Length) PCVOID Buffer,
    _In_ SIZE_T Length,
    _Out_writes_bytes_(PH_SYMCRYPT_SHA3_224_RESULT_SIZE) PVOID Result
    )
{
    SymCryptSha3_224((PCBYTE)Buffer, Length, (PBYTE)Result);
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

VOID NTAPI PhSymCryptShake128(
    _In_reads_bytes_(Length) PCVOID Buffer,
    _In_ SIZE_T Length,
    _Out_writes_bytes_(ResultLength) PVOID Result,
    _In_ SIZE_T ResultLength
    )
{
    SymCryptShake128((PCBYTE)Buffer, Length, (PBYTE)Result, ResultLength);
}

VOID NTAPI PhSymCryptShake256(
    _In_reads_bytes_(Length) PCVOID Buffer,
    _In_ SIZE_T Length,
    _Out_writes_bytes_(ResultLength) PVOID Result,
    _In_ SIZE_T ResultLength
    )
{
    SymCryptShake256((PCBYTE)Buffer, Length, (PBYTE)Result, ResultLength);
}

VOID NTAPI PhSymCryptCShake128(
    _In_reads_bytes_opt_(FunctionNameLength) PCVOID FunctionName,
    _In_ SIZE_T FunctionNameLength,
    _In_reads_bytes_opt_(CustomizationLength) PCVOID Customization,
    _In_ SIZE_T CustomizationLength,
    _In_reads_bytes_(Length) PCVOID Buffer,
    _In_ SIZE_T Length,
    _Out_writes_bytes_(ResultLength) PVOID Result,
    _In_ SIZE_T ResultLength
    )
{
    SymCryptCShake128(
        (PCBYTE)FunctionName,
        FunctionNameLength,
        (PCBYTE)Customization,
        CustomizationLength,
        (PCBYTE)Buffer,
        Length,
        (PBYTE)Result,
        ResultLength
        );
}

VOID NTAPI PhSymCryptCShake256(
    _In_reads_bytes_opt_(FunctionNameLength) PCVOID FunctionName,
    _In_ SIZE_T FunctionNameLength,
    _In_reads_bytes_opt_(CustomizationLength) PCVOID Customization,
    _In_ SIZE_T CustomizationLength,
    _In_reads_bytes_(Length) PCVOID Buffer,
    _In_ SIZE_T Length,
    _Out_writes_bytes_(ResultLength) PVOID Result,
    _In_ SIZE_T ResultLength
    )
{
    SymCryptCShake256(
        (PCBYTE)FunctionName,
        FunctionNameLength,
        (PCBYTE)Customization,
        CustomizationLength,
        (PCBYTE)Buffer,
        Length,
        (PBYTE)Result,
        ResultLength
        );
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
    case PH_SYMCRYPT_SHA224_ALGORITHM:
        *HashAlgorithm = SymCryptSha224Algorithm;
        *HashResultSize = PH_SYMCRYPT_SHA224_RESULT_SIZE;
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
    case PH_SYMCRYPT_SHA512_224_ALGORITHM:
        *HashAlgorithm = SymCryptSha512_224Algorithm;
        *HashResultSize = PH_SYMCRYPT_SHA512_224_RESULT_SIZE;
        return TRUE;
    case PH_SYMCRYPT_SHA512_256_ALGORITHM:
        *HashAlgorithm = SymCryptSha512_256Algorithm;
        *HashResultSize = PH_SYMCRYPT_SHA512_256_RESULT_SIZE;
        return TRUE;
    case PH_SYMCRYPT_SHA3_224_ALGORITHM:
        *HashAlgorithm = SymCryptSha3_224Algorithm;
        *HashResultSize = PH_SYMCRYPT_SHA3_224_RESULT_SIZE;
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
    case PH_SYMCRYPT_SHAKE128_ALGORITHM:
        *HashAlgorithm = SymCryptShake128HashAlgorithm;
        *HashResultSize = PH_SYMCRYPT_SHAKE128_DEFAULT_RESULT_SIZE;
        return TRUE;
    case PH_SYMCRYPT_SHAKE256_ALGORITHM:
        *HashAlgorithm = SymCryptShake256HashAlgorithm;
        *HashResultSize = PH_SYMCRYPT_SHAKE256_DEFAULT_RESULT_SIZE;
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

    memset(Context, 0, sizeof(PH_SYMCRYPT_HASH_CONTEXT));

    if (!PhSymCryptResolveHashAlgorithm(Algorithm, &hashAlgorithm, &hashResultSize))
        return STATUS_NOT_SUPPORTED;

    if (SymCryptHashStateSize(hashAlgorithm) > PH_SYMCRYPT_HASH_STATE_BUFFER_SIZE)
        return STATUS_BUFFER_TOO_SMALL;

    Context->Algorithm = (PVOID)hashAlgorithm;
    Context->ResultSize = hashResultSize;
    Context->StateSize = (ULONG)SymCryptHashStateSize(hashAlgorithm);

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

    if (PhEqualStringZ(AlgorithmId, L"SHA224", FALSE) || PhEqualStringZ(AlgorithmId, L"SHA-224", FALSE))
    {
        *Algorithm = PH_SYMCRYPT_SHA224_ALGORITHM;
        if (HashSize) *HashSize = PH_SYMCRYPT_SHA224_RESULT_SIZE;
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

    if (PhEqualStringZ(AlgorithmId, L"SHA512-224", FALSE) || PhEqualStringZ(AlgorithmId, L"SHA512/224", FALSE))
    {
        *Algorithm = PH_SYMCRYPT_SHA512_224_ALGORITHM;
        if (HashSize) *HashSize = PH_SYMCRYPT_SHA512_224_RESULT_SIZE;
        return STATUS_SUCCESS;
    }

    if (PhEqualStringZ(AlgorithmId, BCRYPT_SHA512_256_ALGORITHM, FALSE) || PhEqualStringZ(AlgorithmId, L"SHA512/256", FALSE))
    {
        *Algorithm = PH_SYMCRYPT_SHA512_256_ALGORITHM;
        if (HashSize) *HashSize = PH_SYMCRYPT_SHA512_256_RESULT_SIZE;
        return STATUS_SUCCESS;
    }

    if (PhEqualStringZ(AlgorithmId, BCRYPT_SHA3_224_ALGORITHM, FALSE))
    {
        *Algorithm = PH_SYMCRYPT_SHA3_224_ALGORITHM;
        if (HashSize) *HashSize = PH_SYMCRYPT_SHA3_224_RESULT_SIZE;
        return STATUS_SUCCESS;
    }

    if (PhEqualStringZ(AlgorithmId, BCRYPT_SHA3_256_ALGORITHM, FALSE))
    {
        *Algorithm = PH_SYMCRYPT_SHA3_256_ALGORITHM;
        if (HashSize) *HashSize = PH_SYMCRYPT_SHA3_256_RESULT_SIZE;
        return STATUS_SUCCESS;
    }

    if (PhEqualStringZ(AlgorithmId, BCRYPT_SHA3_384_ALGORITHM, FALSE))
    {
        *Algorithm = PH_SYMCRYPT_SHA3_384_ALGORITHM;
        if (HashSize) *HashSize = PH_SYMCRYPT_SHA3_384_RESULT_SIZE;
        return STATUS_SUCCESS;
    }

    if (PhEqualStringZ(AlgorithmId, BCRYPT_SHA3_512_ALGORITHM, FALSE))
    {
        *Algorithm = PH_SYMCRYPT_SHA3_512_ALGORITHM;
        if (HashSize) *HashSize = PH_SYMCRYPT_SHA3_512_RESULT_SIZE;
        return STATUS_SUCCESS;
    }

    if (PhEqualStringZ(AlgorithmId, BCRYPT_SHAKE128_ALGORITHM, FALSE))
    {
        *Algorithm = PH_SYMCRYPT_SHAKE128_ALGORITHM;
        if (HashSize) *HashSize = PH_SYMCRYPT_SHAKE128_DEFAULT_RESULT_SIZE;
        return STATUS_SUCCESS;
    }

    if (PhEqualStringZ(AlgorithmId, BCRYPT_SHAKE256_ALGORITHM, FALSE))
    {
        *Algorithm = PH_SYMCRYPT_SHAKE256_ALGORITHM;
        if (HashSize) *HashSize = PH_SYMCRYPT_SHAKE256_DEFAULT_RESULT_SIZE;
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
        *Context = NULL;                                                        \
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

static NTSTATUS PhpSymCryptWriteProperty(
    _In_reads_bytes_(ValueLength) PCVOID Value,
    _In_ ULONG ValueLength,
    _Out_writes_bytes_to_opt_(OutputLength, *ResultLength) PVOID Output,
    _In_ ULONG OutputLength,
    _Out_opt_ PULONG ResultLength
    )
{
    if (ResultLength)
        *ResultLength = ValueLength;

    if (!Output)
        return STATUS_SUCCESS;

    if (OutputLength < ValueLength)
        return STATUS_BUFFER_TOO_SMALL;

    memcpy(Output, Value, ValueLength);
    return STATUS_SUCCESS;
}

NTSTATUS NTAPI PhSymCryptGetProperty(
    _In_ PCWSTR AlgorithmId,
    _In_ PCWSTR Property,
    _Out_writes_bytes_to_opt_(OutputLength, *ResultLength) PVOID Output,
    _In_ ULONG OutputLength,
    _Out_opt_ PULONG ResultLength
    )
{
    PH_SYMCRYPT_HASH_ALGORITHM hashAlgorithm;
    ULONG value;

    if (!AlgorithmId || !Property)
        return STATUS_INVALID_PARAMETER;

    if (NT_SUCCESS(PhSymCryptHashAlgorithmIdToAlgorithm(AlgorithmId, &hashAlgorithm, &value)))
    {
        if (PhEqualStringZ(Property, BCRYPT_HASH_LENGTH, TRUE))
            return PhpSymCryptWriteProperty(&value, sizeof(value), Output, OutputLength, ResultLength);

        if (PhEqualStringZ(Property, BCRYPT_OBJECT_LENGTH, TRUE))
        {
            value = sizeof(PH_SYMCRYPT_HASH_CONTEXT);
            return PhpSymCryptWriteProperty(&value, sizeof(value), Output, OutputLength, ResultLength);
        }

        if (PhEqualStringZ(Property, BCRYPT_ALGORITHM_NAME, TRUE))
        {
            ULONG bytes = (ULONG)((PhCountStringZ(AlgorithmId) + 1) * sizeof(WCHAR));
            return PhpSymCryptWriteProperty(AlgorithmId, bytes, Output, OutputLength, ResultLength);
        }
    }

    if (PhEqualStringZ(AlgorithmId, BCRYPT_AES_ALGORITHM, TRUE))
    {
        if (PhEqualStringZ(Property, BCRYPT_BLOCK_LENGTH, TRUE))
        {
            value = SYMCRYPT_AES_BLOCK_SIZE;
            return PhpSymCryptWriteProperty(&value, sizeof(value), Output, OutputLength, ResultLength);
        }

        if (PhEqualStringZ(Property, BCRYPT_KEY_LENGTH, TRUE))
        {
            value = 256;
            return PhpSymCryptWriteProperty(&value, sizeof(value), Output, OutputLength, ResultLength);
        }

        if (PhEqualStringZ(Property, BCRYPT_OBJECT_LENGTH, TRUE))
        {
            value = sizeof(SYMCRYPT_AES_EXPANDED_KEY);
            return PhpSymCryptWriteProperty(&value, sizeof(value), Output, OutputLength, ResultLength);
        }
    }

    return STATUS_NOT_SUPPORTED;
}

// ------------------------------------------------------------------------
// HMAC (single-shot)
// ------------------------------------------------------------------------

_Success_(return)
BOOLEAN PhpSymCryptResolveMacAlgorithm(
    _In_ PCWSTR AlgorithmId,
    _Out_ PCSYMCRYPT_MAC* MacAlgorithm,
    _Out_ PSIZE_T ResultSize
    )
{
    if (PhEqualStringZ(AlgorithmId, BCRYPT_MD5_ALGORITHM, TRUE))
    {
        *MacAlgorithm = SymCryptHmacMd5Algorithm;
        *ResultSize = PH_SYMCRYPT_HMAC_MD5_RESULT_SIZE;
        return TRUE;
    }

    if (PhEqualStringZ(AlgorithmId, BCRYPT_SHA1_ALGORITHM, TRUE))
    {
        *MacAlgorithm = SymCryptHmacSha1Algorithm;
        *ResultSize = PH_SYMCRYPT_HMAC_SHA1_RESULT_SIZE;
        return TRUE;
    }

    if (PhEqualStringZ(AlgorithmId, L"SHA224", TRUE) || PhEqualStringZ(AlgorithmId, L"SHA-224", TRUE))
    {
        *MacAlgorithm = SymCryptHmacSha224Algorithm;
        *ResultSize = PH_SYMCRYPT_HMAC_SHA224_RESULT_SIZE;
        return TRUE;
    }

    if (PhEqualStringZ(AlgorithmId, BCRYPT_SHA256_ALGORITHM, TRUE))
    {
        *MacAlgorithm = SymCryptHmacSha256Algorithm;
        *ResultSize = PH_SYMCRYPT_HMAC_SHA256_RESULT_SIZE;
        return TRUE;
    }

    if (PhEqualStringZ(AlgorithmId, BCRYPT_SHA384_ALGORITHM, TRUE))
    {
        *MacAlgorithm = SymCryptHmacSha384Algorithm;
        *ResultSize = PH_SYMCRYPT_HMAC_SHA384_RESULT_SIZE;
        return TRUE;
    }

    if (PhEqualStringZ(AlgorithmId, BCRYPT_SHA512_ALGORITHM, TRUE))
    {
        *MacAlgorithm = SymCryptHmacSha512Algorithm;
        *ResultSize = PH_SYMCRYPT_HMAC_SHA512_RESULT_SIZE;
        return TRUE;
    }

    if (PhEqualStringZ(AlgorithmId, BCRYPT_AES_CMAC_ALGORITHM, TRUE))
    {
        *MacAlgorithm = SymCryptAesCmacAlgorithm;
        *ResultSize = PH_SYMCRYPT_AES_CMAC_RESULT_SIZE;
        return TRUE;
    }

    if (PhEqualStringZ(AlgorithmId, BCRYPT_KMAC128_ALGORITHM, TRUE))
    {
        *MacAlgorithm = SymCryptKmac128Algorithm;
        *ResultSize = PH_SYMCRYPT_SHAKE128_DEFAULT_RESULT_SIZE;
        return TRUE;
    }

    if (PhEqualStringZ(AlgorithmId, BCRYPT_KMAC256_ALGORITHM, TRUE))
    {
        *MacAlgorithm = SymCryptKmac256Algorithm;
        *ResultSize = PH_SYMCRYPT_SHAKE256_DEFAULT_RESULT_SIZE;
        return TRUE;
    }

    return FALSE;
}

NTSTATUS NTAPI PhSymCryptHmac(
    _In_ PCWSTR AlgorithmId,
    _In_reads_bytes_(KeyLength) PCVOID Key,
    _In_ SIZE_T KeyLength,
    _In_reads_bytes_(DataLength) PCVOID Data,
    _In_ SIZE_T DataLength,
    _Out_writes_bytes_to_(ResultLength, *BytesWritten) PVOID Result,
    _In_ SIZE_T ResultLength,
    _Out_opt_ PSIZE_T BytesWritten
    )
{
    PH_SYMCRYPT_HASH_ALGORITHM hashAlgorithm;
    PCSYMCRYPT_HASH symcryptHash;
    ULONG macResultSize;
    SYMCRYPT_HMAC_EXPANDED_KEY expandedKey;
    SYMCRYPT_ERROR error;
    NTSTATUS status;

    if (!AlgorithmId || !Result)
        return STATUS_INVALID_PARAMETER;

    status = PhSymCryptHashAlgorithmIdToAlgorithm(AlgorithmId, &hashAlgorithm, &macResultSize);

    if (!NT_SUCCESS(status))
        return status;

    if (ResultLength < macResultSize)
        return STATUS_BUFFER_TOO_SMALL;

    if (BytesWritten)
        *BytesWritten = macResultSize;

    if (!PhSymCryptResolveHashAlgorithm(hashAlgorithm, &symcryptHash, &macResultSize))
        return STATUS_NOT_SUPPORTED;

    error = SymCryptHmacExpandKey(symcryptHash, &expandedKey, (PCBYTE)Key, KeyLength);

    if (error == SYMCRYPT_NO_ERROR)
    {
        SymCryptHmac(&expandedKey, (PCBYTE)Data, DataLength, (PBYTE)Result);
    }

    SymCryptWipeKnownSize(&expandedKey, sizeof(expandedKey));

    return PhSymCryptErrorToStatus(error);
}

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
    Md5,
    SYMCRYPT_HMAC_MD5_EXPANDED_KEY,
    SymCryptHmacMd5ExpandKey,
    SymCryptHmacMd5,
    PH_SYMCRYPT_HMAC_MD5_RESULT_SIZE)

PH_SYMCRYPT_DEFINE_HMAC(
    Sha1,
    SYMCRYPT_HMAC_SHA1_EXPANDED_KEY,
    SymCryptHmacSha1ExpandKey,
    SymCryptHmacSha1,
    PH_SYMCRYPT_HMAC_SHA1_RESULT_SIZE)

PH_SYMCRYPT_DEFINE_HMAC(
    Sha224,
    SYMCRYPT_HMAC_SHA224_EXPANDED_KEY,
    SymCryptHmacSha224ExpandKey,
    SymCryptHmacSha224,
    PH_SYMCRYPT_HMAC_SHA224_RESULT_SIZE)

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

NTSTATUS NTAPI PhSymCryptAesCmac(
    _In_reads_bytes_(KeyLength) PCVOID Key,
    _In_ SIZE_T KeyLength,
    _In_reads_bytes_(DataLength) PCVOID Data,
    _In_ SIZE_T DataLength,
    _Out_writes_bytes_(PH_SYMCRYPT_AES_CMAC_RESULT_SIZE) PVOID Result
    )
{
    SYMCRYPT_AES_CMAC_EXPANDED_KEY expandedKey;
    SYMCRYPT_ERROR error;

    error = SymCryptAesCmacExpandKey(&expandedKey, (PCBYTE)Key, KeyLength);
    if (error == SYMCRYPT_NO_ERROR)
        SymCryptAesCmac(&expandedKey, (PCBYTE)Data, DataLength, (PBYTE)Result);

    SymCryptWipeKnownSize(&expandedKey, sizeof(expandedKey));
    return PhSymCryptErrorToStatus(error);
}

NTSTATUS NTAPI PhSymCryptAesGmac(
    _In_reads_bytes_(KeyLength) PCVOID Key,
    _In_ SIZE_T KeyLength,
    _In_reads_bytes_(NonceLength) PCVOID Nonce,
    _In_ SIZE_T NonceLength,
    _In_reads_bytes_(DataLength) PCVOID Data,
    _In_ SIZE_T DataLength,
    _Out_writes_bytes_(TagLength) PVOID Tag,
    _In_ SIZE_T TagLength
    )
{
    return PhSymCryptAesGcmEncrypt(
        Key,
        KeyLength,
        Nonce,
        NonceLength,
        Data,
        DataLength,
        NULL,
        NULL,
        0,
        Tag,
        TagLength
        );
}

NTSTATUS NTAPI PhSymCryptKmac128(
    _In_reads_bytes_(KeyLength) PCVOID Key,
    _In_ SIZE_T KeyLength,
    _In_reads_bytes_opt_(CustomizationLength) PCVOID Customization,
    _In_ SIZE_T CustomizationLength,
    _In_reads_bytes_(DataLength) PCVOID Data,
    _In_ SIZE_T DataLength,
    _Out_writes_bytes_(ResultLength) PVOID Result,
    _In_ SIZE_T ResultLength
    )
{
    SYMCRYPT_KMAC128_EXPANDED_KEY expandedKey;
    SYMCRYPT_ERROR error;

    error = SymCryptKmac128ExpandKeyEx(&expandedKey, (PCBYTE)Key, KeyLength, (PCBYTE)Customization, CustomizationLength);
    if (error == SYMCRYPT_NO_ERROR)
        SymCryptKmac128Ex(&expandedKey, (PCBYTE)Data, DataLength, (PBYTE)Result, ResultLength);

    SymCryptWipeKnownSize(&expandedKey, sizeof(expandedKey));
    return PhSymCryptErrorToStatus(error);
}

NTSTATUS NTAPI PhSymCryptKmac256(
    _In_reads_bytes_(KeyLength) PCVOID Key,
    _In_ SIZE_T KeyLength,
    _In_reads_bytes_opt_(CustomizationLength) PCVOID Customization,
    _In_ SIZE_T CustomizationLength,
    _In_reads_bytes_(DataLength) PCVOID Data,
    _In_ SIZE_T DataLength,
    _Out_writes_bytes_(ResultLength) PVOID Result,
    _In_ SIZE_T ResultLength
    )
{
    SYMCRYPT_KMAC256_EXPANDED_KEY expandedKey;
    SYMCRYPT_ERROR error;

    error = SymCryptKmac256ExpandKeyEx(&expandedKey, (PCBYTE)Key, KeyLength, (PCBYTE)Customization, CustomizationLength);
    if (error == SYMCRYPT_NO_ERROR)
        SymCryptKmac256Ex(&expandedKey, (PCBYTE)Data, DataLength, (PBYTE)Result, ResultLength);

    SymCryptWipeKnownSize(&expandedKey, sizeof(expandedKey));
    return PhSymCryptErrorToStatus(error);
}

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

NTSTATUS NTAPI PhSymCryptPbkdf2(
    _In_ PCWSTR MacAlgorithmId,
    _In_reads_bytes_(PasswordLength) PCVOID Password,
    _In_ SIZE_T PasswordLength,
    _In_reads_bytes_opt_(SaltLength) PCVOID Salt,
    _In_ SIZE_T SaltLength,
    _In_ ULONG64 IterationCount,
    _Out_writes_bytes_(ResultLength) PVOID Result,
    _In_ SIZE_T ResultLength
    )
{
    PCSYMCRYPT_MAC macAlgorithm;
    SIZE_T resultSize;

    if (!MacAlgorithmId || !Result)
        return STATUS_INVALID_PARAMETER;

    if (!PhpSymCryptResolveMacAlgorithm(MacAlgorithmId, &macAlgorithm, &resultSize))
        return STATUS_NOT_SUPPORTED;

    return PhSymCryptErrorToStatus(SymCryptPbkdf2(
        macAlgorithm,
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

NTSTATUS NTAPI PhSymCryptHkdf(
    _In_ PCWSTR MacAlgorithmId,
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
    PCSYMCRYPT_MAC macAlgorithm;
    SIZE_T resultSize;

    if (!MacAlgorithmId || !Result)
        return STATUS_INVALID_PARAMETER;

    if (!PhpSymCryptResolveMacAlgorithm(MacAlgorithmId, &macAlgorithm, &resultSize))
        return STATUS_NOT_SUPPORTED;

    return PhSymCryptErrorToStatus(SymCryptHkdf(
        macAlgorithm,
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

NTSTATUS NTAPI PhSymCryptSp800_108(
    _In_ PCWSTR MacAlgorithmId,
    _In_reads_bytes_(KeyLength) PCVOID Key,
    _In_ SIZE_T KeyLength,
    _In_reads_bytes_opt_(LabelLength) PCVOID Label,
    _In_ SIZE_T LabelLength,
    _In_reads_bytes_opt_(ContextLength) PCVOID Context,
    _In_ SIZE_T ContextLength,
    _Out_writes_bytes_(ResultLength) PVOID Result,
    _In_ SIZE_T ResultLength
    )
{
    PCSYMCRYPT_MAC macAlgorithm;
    SIZE_T resultSize;

    if (!MacAlgorithmId || !Result)
        return STATUS_INVALID_PARAMETER;

    if (!PhpSymCryptResolveMacAlgorithm(MacAlgorithmId, &macAlgorithm, &resultSize))
        return STATUS_NOT_SUPPORTED;

    return PhSymCryptErrorToStatus(SymCryptSp800_108(
        macAlgorithm,
        (PCBYTE)Key,
        KeyLength,
        (PCBYTE)Label,
        LabelLength,
        (PCBYTE)Context,
        ContextLength,
        (PBYTE)Result,
        ResultLength
        ));
}

NTSTATUS NTAPI PhSymCryptTlsPrf1_1(
    _In_reads_bytes_(KeyLength) PCVOID Key,
    _In_ SIZE_T KeyLength,
    _In_reads_bytes_opt_(LabelLength) PCVOID Label,
    _In_ SIZE_T LabelLength,
    _In_reads_bytes_(SeedLength) PCVOID Seed,
    _In_ SIZE_T SeedLength,
    _Out_writes_bytes_(ResultLength) PVOID Result,
    _In_ SIZE_T ResultLength
    )
{
    return PhSymCryptErrorToStatus(SymCryptTlsPrf1_1(
        (PCBYTE)Key,
        KeyLength,
        (PCBYTE)Label,
        LabelLength,
        (PCBYTE)Seed,
        SeedLength,
        (PBYTE)Result,
        ResultLength
        ));
}

NTSTATUS NTAPI PhSymCryptTlsPrf1_2(
    _In_ PCWSTR MacAlgorithmId,
    _In_reads_bytes_(KeyLength) PCVOID Key,
    _In_ SIZE_T KeyLength,
    _In_reads_bytes_opt_(LabelLength) PCVOID Label,
    _In_ SIZE_T LabelLength,
    _In_reads_bytes_(SeedLength) PCVOID Seed,
    _In_ SIZE_T SeedLength,
    _Out_writes_bytes_(ResultLength) PVOID Result,
    _In_ SIZE_T ResultLength
    )
{
    PCSYMCRYPT_MAC macAlgorithm;
    SIZE_T resultSize;

    if (!PhpSymCryptResolveMacAlgorithm(MacAlgorithmId, &macAlgorithm, &resultSize))
        return STATUS_NOT_SUPPORTED;

    return PhSymCryptErrorToStatus(SymCryptTlsPrf1_2(
        macAlgorithm,
        (PCBYTE)Key,
        KeyLength,
        (PCBYTE)Label,
        LabelLength,
        (PCBYTE)Seed,
        SeedLength,
        (PBYTE)Result,
        ResultLength
        ));
}

NTSTATUS NTAPI PhSymCryptSshKdf(
    _In_ PH_SYMCRYPT_HASH_ALGORITHM HashAlgorithm,
    _In_reads_bytes_(KeyLength) PCVOID Key,
    _In_ SIZE_T KeyLength,
    _In_reads_bytes_(HashValueLength) PCVOID HashValue,
    _In_ SIZE_T HashValueLength,
    _In_ BYTE Label,
    _In_reads_bytes_(SessionIdLength) PCVOID SessionId,
    _In_ SIZE_T SessionIdLength,
    _Out_writes_bytes_(ResultLength) PVOID Result,
    _In_ SIZE_T ResultLength
    )
{
    PCSYMCRYPT_HASH hashAlgorithm;
    ULONG hashResultSize;

    if (!PhSymCryptResolveHashAlgorithm(HashAlgorithm, &hashAlgorithm, &hashResultSize))
        return STATUS_NOT_SUPPORTED;

    return PhSymCryptErrorToStatus(SymCryptSshKdf(
        hashAlgorithm,
        (PCBYTE)Key,
        KeyLength,
        (PCBYTE)HashValue,
        HashValueLength,
        Label,
        (PCBYTE)SessionId,
        SessionIdLength,
        (PBYTE)Result,
        ResultLength
        ));
}

NTSTATUS NTAPI PhSymCryptSrtpKdf(
    _In_reads_bytes_(KeyLength) PCVOID Key,
    _In_ SIZE_T KeyLength,
    _In_reads_bytes_(SaltLength) PCVOID Salt,
    _In_ SIZE_T SaltLength,
    _In_ ULONG KeyDerivationRate,
    _In_ ULONG64 Index,
    _In_ ULONG IndexWidth,
    _In_ BYTE Label,
    _Out_writes_bytes_(ResultLength) PVOID Result,
    _In_ SIZE_T ResultLength
    )
{
    return PhSymCryptErrorToStatus(SymCryptSrtpKdf(
        (PCBYTE)Key,
        KeyLength,
        (PCBYTE)Salt,
        SaltLength,
        KeyDerivationRate,
        Index,
        IndexWidth,
        Label,
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

    if (!Key || !KeyLength)
        return STATUS_INVALID_PARAMETER;

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

NTSTATUS NTAPI PhSymCryptAesCcmEncrypt(
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
    SYMCRYPT_AES_EXPANDED_KEY expandedKey;
    SYMCRYPT_ERROR error;

    error = SymCryptAesExpandKey(&expandedKey, (PCBYTE)Key, KeyLength);

    if (error != SYMCRYPT_NO_ERROR)
        return PhSymCryptErrorToStatus(error);

    error = SymCryptCcmValidateParameters(
        SymCryptAesBlockCipher, 
        NonceLength, 
        AuthDataLength,
        DataLength, 
        TagLength
        );

    if (error == SYMCRYPT_NO_ERROR)
    {
        SymCryptCcmEncrypt(
            SymCryptAesBlockCipher,
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
    }

    SymCryptWipeKnownSize(&expandedKey, sizeof(expandedKey));
    return PhSymCryptErrorToStatus(error);
}

NTSTATUS NTAPI PhSymCryptAesCcmDecrypt(
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
    SYMCRYPT_AES_EXPANDED_KEY expandedKey;
    SYMCRYPT_ERROR error;

    error = SymCryptAesExpandKey(&expandedKey, (PCBYTE)Key, KeyLength);

    if (error != SYMCRYPT_NO_ERROR)
        return PhSymCryptErrorToStatus(error);

    error = SymCryptCcmValidateParameters(
        SymCryptAesBlockCipher, 
        NonceLength, 
        AuthDataLength, 
        DataLength, 
        TagLength
        );

    if (error == SYMCRYPT_NO_ERROR)
    {
        error = SymCryptCcmDecrypt(
            SymCryptAesBlockCipher,
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

    SymCryptWipeKnownSize(&expandedKey, sizeof(expandedKey));
    return PhSymCryptErrorToStatus(error);
}

#define PH_SYMCRYPT_GCM_STATE_MAGIC 'gSyP'

typedef struct _PH_SYMCRYPT_GCM_STATE
{
    ULONG Magic;
    SYMCRYPT_GCM_EXPANDED_KEY ExpandedKey;
    SYMCRYPT_GCM_STATE State;
} PH_SYMCRYPT_GCM_STATE, *PPH_SYMCRYPT_GCM_STATE;

NTSTATUS NTAPI PhSymCryptGcmInit(
    _Out_ PPH_SYMCRYPT_AUTH_STATE_HANDLE StateHandle,
    _In_reads_bytes_(KeyLength) PCVOID Key,
    _In_ SIZE_T KeyLength,
    _In_reads_bytes_(NonceLength) PCVOID Nonce,
    _In_ SIZE_T NonceLength
    )
{
    PPH_SYMCRYPT_GCM_STATE state;
    SYMCRYPT_ERROR error;

    if (!StateHandle)
        return STATUS_INVALID_PARAMETER;

    *StateHandle = NULL;

    state = PhAllocateZero(sizeof(PH_SYMCRYPT_GCM_STATE));
    state->Magic = PH_SYMCRYPT_GCM_STATE_MAGIC;

    error = SymCryptGcmExpandKey(
        &state->ExpandedKey, 
        SymCryptAesBlockCipher, 
        (PCBYTE)Key, 
        KeyLength
        );

    if (error != SYMCRYPT_NO_ERROR)
    {
        PhFree(state);
        return PhSymCryptErrorToStatus(error);
    }

    SymCryptGcmInit(&state->State, &state->ExpandedKey, (PCBYTE)Nonce, NonceLength);
    *StateHandle = state;
    return STATUS_SUCCESS;
}

static PPH_SYMCRYPT_GCM_STATE PhpSymCryptGetGcmState(
    _In_ PH_SYMCRYPT_AUTH_STATE_HANDLE StateHandle
    )
{
    PPH_SYMCRYPT_GCM_STATE state = (PPH_SYMCRYPT_GCM_STATE)StateHandle;

    if (!state || state->Magic != PH_SYMCRYPT_GCM_STATE_MAGIC)
        return NULL;

    return state;
}

NTSTATUS NTAPI PhSymCryptGcmAuthPart(
    _In_ PH_SYMCRYPT_AUTH_STATE_HANDLE StateHandle,
    _In_reads_bytes_opt_(AuthDataLength) PCVOID AuthData,
    _In_ SIZE_T AuthDataLength
    )
{
    PPH_SYMCRYPT_GCM_STATE state = PhpSymCryptGetGcmState(StateHandle);

    if (!state)
        return STATUS_INVALID_PARAMETER;

    SymCryptGcmAuthPart(&state->State, (PCBYTE)AuthData, AuthDataLength);
    return STATUS_SUCCESS;
}

NTSTATUS NTAPI PhSymCryptGcmEncryptPart(
    _In_ PH_SYMCRYPT_AUTH_STATE_HANDLE StateHandle,
    _In_reads_bytes_(DataLength) PCVOID Plaintext,
    _Out_writes_bytes_(DataLength) PVOID Ciphertext,
    _In_ SIZE_T DataLength
    )
{
    PPH_SYMCRYPT_GCM_STATE state = PhpSymCryptGetGcmState(StateHandle);

    if (!state)
        return STATUS_INVALID_PARAMETER;

    SymCryptGcmEncryptPart(&state->State, (PCBYTE)Plaintext, (PBYTE)Ciphertext, DataLength);
    return STATUS_SUCCESS;
}

NTSTATUS NTAPI PhSymCryptGcmDecryptPart(
    _In_ PH_SYMCRYPT_AUTH_STATE_HANDLE StateHandle,
    _In_reads_bytes_(DataLength) PCVOID Ciphertext,
    _Out_writes_bytes_(DataLength) PVOID Plaintext,
    _In_ SIZE_T DataLength
    )
{
    PPH_SYMCRYPT_GCM_STATE state = PhpSymCryptGetGcmState(StateHandle);

    if (!state)
        return STATUS_INVALID_PARAMETER;

    SymCryptGcmDecryptPart(&state->State, (PCBYTE)Ciphertext, (PBYTE)Plaintext, DataLength);
    return STATUS_SUCCESS;
}

NTSTATUS NTAPI PhSymCryptGcmEncryptFinal(
    _In_ PH_SYMCRYPT_AUTH_STATE_HANDLE StateHandle,
    _Out_writes_bytes_(TagLength) PVOID Tag,
    _In_ SIZE_T TagLength
    )
{
    PPH_SYMCRYPT_GCM_STATE state = PhpSymCryptGetGcmState(StateHandle);

    if (!state)
        return STATUS_INVALID_PARAMETER;

    SymCryptGcmEncryptFinal(&state->State, (PBYTE)Tag, TagLength);
    return STATUS_SUCCESS;
}

NTSTATUS NTAPI PhSymCryptGcmDecryptFinal(
    _In_ PH_SYMCRYPT_AUTH_STATE_HANDLE StateHandle,
    _In_reads_bytes_(TagLength) PCVOID Tag,
    _In_ SIZE_T TagLength
    )
{
    PPH_SYMCRYPT_GCM_STATE state = PhpSymCryptGetGcmState(StateHandle);

    if (!state)
        return STATUS_INVALID_PARAMETER;

    return PhSymCryptErrorToStatus(SymCryptGcmDecryptFinal(&state->State, (PCBYTE)Tag, TagLength));
}

VOID NTAPI PhSymCryptDestroyAuthState(
    _In_opt_ PH_SYMCRYPT_AUTH_STATE_HANDLE StateHandle
    )
{
    PPH_SYMCRYPT_GCM_STATE state = PhpSymCryptGetGcmState(StateHandle);

    if (!state)
        return;

    SymCryptWipeKnownSize(state, sizeof(PH_SYMCRYPT_GCM_STATE));
    PhFree(state);
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

    if (!NT_SUCCESS(RtlSIZETAdd(PlaintextLength, padLen, &paddedLength)))
        return STATUS_INTEGER_OVERFLOW;

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

NTSTATUS NTAPI PhSymCryptAesEcbEncrypt(
    _In_reads_bytes_(KeyLength) PCVOID Key,
    _In_ SIZE_T KeyLength,
    _In_reads_bytes_(DataLength) PCVOID Plaintext,
    _Out_writes_bytes_(DataLength) PVOID Ciphertext,
    _In_ SIZE_T DataLength
    )
{
    SYMCRYPT_AES_EXPANDED_KEY expandedKey;
    SYMCRYPT_ERROR error;

    if ((DataLength % PH_AES_BLOCK_SIZE) != 0)
        return STATUS_INVALID_PARAMETER;

    error = SymCryptAesExpandKey(
        &expandedKey, 
        (PCBYTE)Key, 
        KeyLength
        );

    if (error == SYMCRYPT_NO_ERROR)
        SymCryptAesEcbEncrypt(&expandedKey, (PCBYTE)Plaintext, (PBYTE)Ciphertext, DataLength);

    SymCryptWipeKnownSize(&expandedKey, sizeof(expandedKey));
    return PhSymCryptErrorToStatus(error);
}

NTSTATUS NTAPI PhSymCryptAesEcbDecrypt(
    _In_reads_bytes_(KeyLength) PCVOID Key,
    _In_ SIZE_T KeyLength,
    _In_reads_bytes_(DataLength) PCVOID Ciphertext,
    _Out_writes_bytes_(DataLength) PVOID Plaintext,
    _In_ SIZE_T DataLength
    )
{
    SYMCRYPT_AES_EXPANDED_KEY expandedKey;
    SYMCRYPT_ERROR error;

    if ((DataLength % PH_AES_BLOCK_SIZE) != 0)
        return STATUS_INVALID_PARAMETER;

    error = SymCryptAesExpandKey(
        &expandedKey, 
        (PCBYTE)Key, 
        KeyLength
        );

    if (error == SYMCRYPT_NO_ERROR)
    {
        SymCryptAesEcbDecrypt(&expandedKey, (PCBYTE)Ciphertext, (PBYTE)Plaintext, DataLength);
    }

    SymCryptWipeKnownSize(&expandedKey, sizeof(expandedKey));
    return PhSymCryptErrorToStatus(error);
}

NTSTATUS NTAPI PhSymCryptAesCtr(
    _In_reads_bytes_(KeyLength) PCVOID Key,
    _In_ SIZE_T KeyLength,
    _Inout_updates_bytes_(16) PVOID Counter,
    _In_reads_bytes_(DataLength) PCVOID Input,
    _Out_writes_bytes_(DataLength) PVOID Output,
    _In_ SIZE_T DataLength
    )
{
    SYMCRYPT_AES_EXPANDED_KEY expandedKey;
    SYMCRYPT_ERROR error;

    error = SymCryptAesExpandKey(
        &expandedKey, 
        (PCBYTE)Key, 
        KeyLength
        );

    if (error == SYMCRYPT_NO_ERROR)
    {
        SymCryptAesCtrMsb64(&expandedKey, (PBYTE)Counter, (PCBYTE)Input, (PBYTE)Output, DataLength);
    }

    SymCryptWipeKnownSize(&expandedKey, sizeof(expandedKey));
    return PhSymCryptErrorToStatus(error);
}

static NTSTATUS PhpSymCryptAesCfb(
    _In_ BOOLEAN Encrypt,
    _In_reads_bytes_(KeyLength) PCVOID Key,
    _In_ SIZE_T KeyLength,
    _In_reads_bytes_(PH_AES_BLOCK_SIZE) PCVOID Iv,
    _In_ SIZE_T ShiftLength,
    _In_reads_bytes_(DataLength) PCVOID Input,
    _Out_writes_bytes_(DataLength) PVOID Output,
    _In_ SIZE_T DataLength
    )
{
    SYMCRYPT_AES_EXPANDED_KEY expandedKey;
    BYTE chainingValue[PH_AES_BLOCK_SIZE];
    SYMCRYPT_ERROR error;

    if (ShiftLength == 0)
        ShiftLength = PH_AES_BLOCK_SIZE;

    if (ShiftLength != 1 && ShiftLength != PH_AES_BLOCK_SIZE)
        return STATUS_INVALID_PARAMETER;

    error = SymCryptAesExpandKey(&expandedKey, (PCBYTE)Key, KeyLength);
    if (error != SYMCRYPT_NO_ERROR)
        return PhSymCryptErrorToStatus(error);

    memcpy(chainingValue, Iv, sizeof(chainingValue));

    if (Encrypt)
    {
        SymCryptCfbEncrypt(
            SymCryptAesBlockCipher,
            ShiftLength, 
            &expandedKey,
            chainingValue, 
            (PCBYTE)Input, 
            (PBYTE)Output, 
            DataLength
            );
    }
    else
    {
        SymCryptCfbDecrypt(
            SymCryptAesBlockCipher, 
            ShiftLength, 
            &expandedKey, 
            chainingValue, 
            (PCBYTE)Input, 
            (PBYTE)Output, 
            DataLength
            );
    }

    SymCryptWipeKnownSize(&expandedKey, sizeof(expandedKey));
    SymCryptWipeKnownSize(chainingValue, sizeof(chainingValue));
    return STATUS_SUCCESS;
}

NTSTATUS NTAPI PhSymCryptAesCfbEncrypt(
    _In_reads_bytes_(KeyLength) PCVOID Key,
    _In_ SIZE_T KeyLength,
    _In_reads_bytes_(16) PCVOID Iv,
    _In_ SIZE_T ShiftLength,
    _In_reads_bytes_(DataLength) PCVOID Plaintext,
    _Out_writes_bytes_(DataLength) PVOID Ciphertext,
    _In_ SIZE_T DataLength
    )
{
    return PhpSymCryptAesCfb(TRUE, Key, KeyLength, Iv, ShiftLength, Plaintext, Ciphertext, DataLength);
}

NTSTATUS NTAPI PhSymCryptAesCfbDecrypt(
    _In_reads_bytes_(KeyLength) PCVOID Key,
    _In_ SIZE_T KeyLength,
    _In_reads_bytes_(16) PCVOID Iv,
    _In_ SIZE_T ShiftLength,
    _In_reads_bytes_(DataLength) PCVOID Ciphertext,
    _Out_writes_bytes_(DataLength) PVOID Plaintext,
    _In_ SIZE_T DataLength
    )
{
    return PhpSymCryptAesCfb(FALSE, Key, KeyLength, Iv, ShiftLength, Ciphertext, Plaintext, DataLength);
}

NTSTATUS NTAPI PhSymCryptXtsAesEncrypt(
    _In_reads_bytes_(KeyLength) PCVOID Key,
    _In_ SIZE_T KeyLength,
    _In_ SIZE_T DataUnitLength,
    _In_reads_bytes_(16) PCVOID Tweak,
    _In_reads_bytes_(DataLength) PCVOID Plaintext,
    _Out_writes_bytes_(DataLength) PVOID Ciphertext,
    _In_ SIZE_T DataLength
    )
{
    SYMCRYPT_XTS_AES_EXPANDED_KEY expandedKey;
    SYMCRYPT_ERROR error;

    error = SymCryptXtsAesExpandKey(&expandedKey, (PCBYTE)Key, KeyLength);

    if (error == SYMCRYPT_NO_ERROR)
    {
        SymCryptXtsAesEncryptWith128bTweak(
            &expandedKey, 
            DataUnitLength, 
            (PCBYTE)Tweak, 
            (PCBYTE)Plaintext, 
            (PBYTE)Ciphertext,
            DataLength
            );
    }

    SymCryptWipeKnownSize(&expandedKey, sizeof(expandedKey));
    return PhSymCryptErrorToStatus(error);
}

NTSTATUS NTAPI PhSymCryptXtsAesDecrypt(
    _In_reads_bytes_(KeyLength) PCVOID Key,
    _In_ SIZE_T KeyLength,
    _In_ SIZE_T DataUnitLength,
    _In_reads_bytes_(16) PCVOID Tweak,
    _In_reads_bytes_(DataLength) PCVOID Ciphertext,
    _Out_writes_bytes_(DataLength) PVOID Plaintext,
    _In_ SIZE_T DataLength
    )
{
    SYMCRYPT_XTS_AES_EXPANDED_KEY expandedKey;
    SYMCRYPT_ERROR error;

    error = SymCryptXtsAesExpandKey(&expandedKey, (PCBYTE)Key, KeyLength);
    if (error == SYMCRYPT_NO_ERROR)
        SymCryptXtsAesDecryptWith128bTweak(&expandedKey, DataUnitLength, (PCBYTE)Tweak, (PCBYTE)Ciphertext, (PBYTE)Plaintext, DataLength);

    SymCryptWipeKnownSize(&expandedKey, sizeof(expandedKey));
    return PhSymCryptErrorToStatus(error);
}

#define PH_SYMCRYPT_DEFINE_AES_KW(Name, FunctionName)                         \
    NTSTATUS NTAPI PhSymCrypt##Name(                                          \
        _In_reads_bytes_(KeyLength) PCVOID Key,                               \
        _In_ SIZE_T KeyLength,                                                \
        _In_reads_bytes_(InputLength) PCVOID Input,                           \
        _In_ SIZE_T InputLength,                                              \
        _Out_writes_bytes_to_(OutputCapacity, *OutputLength) PVOID Output,    \
        _In_ SIZE_T OutputCapacity,                                           \
        _Out_ PSIZE_T OutputLength                                            \
        )                                                                     \
    {                                                                         \
        SYMCRYPT_AES_EXPANDED_KEY expandedKey;                                \
        SYMCRYPT_ERROR error;                                                 \
        if (!OutputLength)                                                    \
            return STATUS_INVALID_PARAMETER;                                  \
        *OutputLength = 0;                                                    \
        error = SymCryptAesExpandKey(&expandedKey, (PCBYTE)Key, KeyLength);   \
        if (error == SYMCRYPT_NO_ERROR)                                       \
            error = FunctionName(&expandedKey, (PCBYTE)Input, InputLength, (PBYTE)Output, OutputCapacity, OutputLength); \
        SymCryptWipeKnownSize(&expandedKey, sizeof(expandedKey));             \
        return PhSymCryptErrorToStatus(error);                                \
    }

PH_SYMCRYPT_DEFINE_AES_KW(AesKwEncrypt, SymCryptAesKwEncrypt)
PH_SYMCRYPT_DEFINE_AES_KW(AesKwDecrypt, SymCryptAesKwDecrypt)
PH_SYMCRYPT_DEFINE_AES_KW(AesKwpEncrypt, SymCryptAesKwpEncrypt)
PH_SYMCRYPT_DEFINE_AES_KW(AesKwpDecrypt, SymCryptAesKwpDecrypt)

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
    case PH_SYMCRYPT_SHA224_ALGORITHM:
        return SymCryptSha224Algorithm;
    case PH_SYMCRYPT_SHA256_ALGORITHM:
        return SymCryptSha256Algorithm;
    case PH_SYMCRYPT_SHA384_ALGORITHM:
        return SymCryptSha384Algorithm;
    case PH_SYMCRYPT_SHA512_ALGORITHM:
        return SymCryptSha512Algorithm;
    case PH_SYMCRYPT_SHA512_224_ALGORITHM:
        return SymCryptSha512_224Algorithm;
    case PH_SYMCRYPT_SHA512_256_ALGORITHM:
        return SymCryptSha512_256Algorithm;
    case PH_SYMCRYPT_SHA3_224_ALGORITHM:
        return SymCryptSha3_224Algorithm;
    case PH_SYMCRYPT_SHA3_256_ALGORITHM:
        return SymCryptSha3_256Algorithm;
    case PH_SYMCRYPT_SHA3_384_ALGORITHM:
        return SymCryptSha3_384Algorithm;
    case PH_SYMCRYPT_SHA3_512_ALGORITHM:
        return SymCryptSha3_512Algorithm;
    case PH_SYMCRYPT_SHAKE128_ALGORITHM:
        return SymCryptShake128HashAlgorithm;
    case PH_SYMCRYPT_SHAKE256_ALGORITHM:
        return SymCryptShake256HashAlgorithm;
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

    {
        SIZE_T bitLength;
        UINT32 bitLength32;

        if (
            !NT_SUCCESS(RtlSIZETMult(ModulusLength, 8, &bitLength)) ||
            !NT_SUCCESS(RtlSIZETToUInt(bitLength, &bitLength32))
            )
        {
            return STATUS_INTEGER_OVERFLOW;
        }

        params.version = 1;
        params.nBitsOfModulus = bitLength32;
        params.nPrimes = 0;
        params.nPubExp = 1;
    }

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
            0
            );
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

NTSTATUS PhSymCryptRsaImportPrivateKey(
    _In_reads_bytes_(KeyBlobLength) PCVOID KeyBlob,
    _In_ SIZE_T KeyBlobLength,
    _Outptr_ PSYMCRYPT_RSAKEY* RsaKey,
    _Out_ PSIZE_T ModulusLength
    );

NTSTATUS PhpSymCryptRsaImportPublicKeyBlob(
    _In_reads_bytes_(KeyBlobLength) PCVOID KeyBlob,
    _In_ SIZE_T KeyBlobLength,
    _Outptr_ PSYMCRYPT_RSAKEY* RsaKey,
    _Out_ PSIZE_T ModulusLength
    )
{
    const BCRYPT_RSAKEY_BLOB* header;
    const BYTE* exponent;
    const BYTE* modulus;
    ULONG64 publicExponent;
    SIZE_T expectedSize;

    *RsaKey = NULL;
    *ModulusLength = 0;

    if (KeyBlobLength < sizeof(BCRYPT_RSAKEY_BLOB))
        return STATUS_INVALID_PARAMETER;

    header = (const BCRYPT_RSAKEY_BLOB*)KeyBlob;

    if (
        header->Magic != BCRYPT_RSAPUBLIC_MAGIC &&
        header->Magic != BCRYPT_RSAPRIVATE_MAGIC &&
        header->Magic != BCRYPT_RSAFULLPRIVATE_MAGIC
        )
    {
        return STATUS_NOT_SUPPORTED;
    }

    if (header->cbPublicExp == 0 || header->cbPublicExp > sizeof(ULONG64) || header->cbModulus == 0)
        return STATUS_INVALID_PARAMETER;

    if (
        !NT_SUCCESS(RtlSIZETAdd(sizeof(BCRYPT_RSAKEY_BLOB), header->cbPublicExp, &expectedSize)) ||
        !NT_SUCCESS(RtlSIZETAdd(expectedSize, header->cbModulus, &expectedSize))
        )
    {
        return STATUS_INTEGER_OVERFLOW;
    }

    if (KeyBlobLength < expectedSize)
        return STATUS_INVALID_PARAMETER;

    exponent = (const BYTE*)KeyBlob + sizeof(BCRYPT_RSAKEY_BLOB);
    modulus = exponent + header->cbPublicExp;

    publicExponent = 0;
    for (ULONG i = 0; i < header->cbPublicExp; i++)
    {
        publicExponent = (publicExponent << 8) | exponent[i];
    }

    *ModulusLength = header->cbModulus;
    return PhSymCryptRsaImportPublicKey(modulus, header->cbModulus, publicExponent, RsaKey);
}

static NTSTATUS PhpSymCryptRsaPublicEncrypt(
    _In_reads_bytes_(KeyBlobLength) PCVOID KeyBlob,
    _In_ SIZE_T KeyBlobLength,
    _In_reads_bytes_(PlaintextLength) PCVOID Plaintext,
    _In_ SIZE_T PlaintextLength,
    _Out_writes_bytes_to_opt_(CiphertextCapacity, *CiphertextLength) PVOID Ciphertext,
    _In_ SIZE_T CiphertextCapacity,
    _Out_ PSIZE_T CiphertextLength,
    _In_opt_ PH_SYMCRYPT_HASH_ALGORITHM* HashAlgorithm,
    _In_reads_bytes_opt_(LabelLength) PCVOID Label,
    _In_ SIZE_T LabelLength,
    _In_ ULONG Mode
    )
{
    PSYMCRYPT_RSAKEY key;
    SIZE_T modulusLength;
    SYMCRYPT_ERROR error;
    PCSYMCRYPT_HASH hashAlgorithm;
    NTSTATUS status;

    if (!CiphertextLength)
        return STATUS_INVALID_PARAMETER;

    *CiphertextLength = 0;

    status = PhpSymCryptRsaImportPublicKeyBlob(KeyBlob, KeyBlobLength, &key, &modulusLength);
    if (!NT_SUCCESS(status))
        return status;

    if (!Ciphertext)
    {
        *CiphertextLength = modulusLength;
        SymCryptRsakeyFree(key);
        return STATUS_SUCCESS;
    }

    switch (Mode)
    {
    case 0:
        if (CiphertextCapacity < modulusLength)
        {
            SymCryptRsakeyFree(key);
            *CiphertextLength = modulusLength;
            return STATUS_BUFFER_TOO_SMALL;
        }

        error = SymCryptRsaRawEncrypt(
            key, 
            (PCBYTE)Plaintext, 
            PlaintextLength, 
            SYMCRYPT_NUMBER_FORMAT_MSB_FIRST, 
            0, 
            (PBYTE)Ciphertext, 
            modulusLength
            );

        if (error == SYMCRYPT_NO_ERROR)
            *CiphertextLength = modulusLength;
        break;
    case 1:
        {
            error = SymCryptRsaPkcs1Encrypt(
                key, 
                (PCBYTE)Plaintext, 
                PlaintextLength, 
                0, 
                SYMCRYPT_NUMBER_FORMAT_MSB_FIRST, 
                (PBYTE)Ciphertext, 
                CiphertextCapacity, 
                CiphertextLength);
        }
        break;
    case 2:
        {
            hashAlgorithm = PhSymCryptHashAlgorithmToHash(*HashAlgorithm);

            if (!hashAlgorithm)
                error = SYMCRYPT_INVALID_ARGUMENT;
            else
            {
                error = SymCryptRsaOaepEncrypt(
                    key, 
                    (PCBYTE)Plaintext, 
                    PlaintextLength, 
                    hashAlgorithm, 
                    (PCBYTE)Label, 
                    LabelLength,
                    0, 
                    SYMCRYPT_NUMBER_FORMAT_MSB_FIRST, 
                    (PBYTE)Ciphertext, 
                    CiphertextCapacity, 
                    CiphertextLength
                    );
                }
        }
        break;
    default:
        error = SYMCRYPT_INVALID_ARGUMENT;
        break;
    }

    SymCryptRsakeyFree(key);
    return PhSymCryptErrorToStatus(error);
}

static NTSTATUS PhpSymCryptRsaPrivateDecrypt(
    _In_reads_bytes_(KeyBlobLength) PCVOID KeyBlob,
    _In_ SIZE_T KeyBlobLength,
    _In_reads_bytes_(CiphertextLength) PCVOID Ciphertext,
    _In_ SIZE_T CiphertextLength,
    _Out_writes_bytes_to_opt_(PlaintextCapacity, *PlaintextLength) PVOID Plaintext,
    _In_ SIZE_T PlaintextCapacity,
    _Out_ PSIZE_T PlaintextLength,
    _In_opt_ PH_SYMCRYPT_HASH_ALGORITHM* HashAlgorithm,
    _In_reads_bytes_opt_(LabelLength) PCVOID Label,
    _In_ SIZE_T LabelLength,
    _In_ ULONG Mode
    )
{
    PSYMCRYPT_RSAKEY key;
    SIZE_T modulusLength;
    SYMCRYPT_ERROR error;
    PCSYMCRYPT_HASH hashAlgorithm;
    NTSTATUS status;

    if (!PlaintextLength)
        return STATUS_INVALID_PARAMETER;

    *PlaintextLength = 0;

    status = PhSymCryptRsaImportPrivateKey(KeyBlob, KeyBlobLength, &key, &modulusLength);
    
    if (!NT_SUCCESS(status))
        return status;

    switch (Mode)
    {
    case 0:
        if (!Plaintext)
        {
            *PlaintextLength = modulusLength;
            SymCryptRsakeyFree(key);
            return STATUS_SUCCESS;
        }

        if (PlaintextCapacity < modulusLength)
        {
            *PlaintextLength = modulusLength;
            SymCryptRsakeyFree(key);
            return STATUS_BUFFER_TOO_SMALL;
        }

        error = SymCryptRsaRawDecrypt(key, (PCBYTE)Ciphertext, CiphertextLength, SYMCRYPT_NUMBER_FORMAT_MSB_FIRST, 0, (PBYTE)Plaintext, modulusLength);
        if (error == SYMCRYPT_NO_ERROR)
            *PlaintextLength = modulusLength;
        break;
    case 1:
        error = SymCryptRsaPkcs1Decrypt(key, (PCBYTE)Ciphertext, CiphertextLength, SYMCRYPT_NUMBER_FORMAT_MSB_FIRST, 0, (PBYTE)Plaintext, PlaintextCapacity, PlaintextLength);
        break;
    case 2:
        hashAlgorithm = PhSymCryptHashAlgorithmToHash(*HashAlgorithm);
        if (!hashAlgorithm)
            error = SYMCRYPT_INVALID_ARGUMENT;
        else
            error = SymCryptRsaOaepDecrypt(key, (PCBYTE)Ciphertext, CiphertextLength, SYMCRYPT_NUMBER_FORMAT_MSB_FIRST, hashAlgorithm, (PCBYTE)Label, LabelLength, 0, (PBYTE)Plaintext, PlaintextCapacity, PlaintextLength);
        break;
    default:
        error = SYMCRYPT_INVALID_ARGUMENT;
        break;
    }

    SymCryptRsakeyFree(key);
    return PhSymCryptErrorToStatus(error);
}

NTSTATUS NTAPI PhSymCryptRsaRawEncrypt(
    _In_reads_bytes_(KeyBlobLength) PCVOID KeyBlob,
    _In_ SIZE_T KeyBlobLength,
    _In_reads_bytes_(PlaintextLength) PCVOID Plaintext,
    _In_ SIZE_T PlaintextLength,
    _Out_writes_bytes_to_(CiphertextCapacity, *CiphertextLength) PVOID Ciphertext,
    _In_ SIZE_T CiphertextCapacity,
    _Out_ PSIZE_T CiphertextLength
    )
{
    return PhpSymCryptRsaPublicEncrypt(KeyBlob, KeyBlobLength, Plaintext, PlaintextLength, Ciphertext, CiphertextCapacity, CiphertextLength, NULL, NULL, 0, 0);
}

NTSTATUS NTAPI PhSymCryptRsaRawDecrypt(
    _In_reads_bytes_(KeyBlobLength) PCVOID KeyBlob,
    _In_ SIZE_T KeyBlobLength,
    _In_reads_bytes_(CiphertextLength) PCVOID Ciphertext,
    _In_ SIZE_T CiphertextLength,
    _Out_writes_bytes_to_(PlaintextCapacity, *PlaintextLength) PVOID Plaintext,
    _In_ SIZE_T PlaintextCapacity,
    _Out_ PSIZE_T PlaintextLength
    )
{
    return PhpSymCryptRsaPrivateDecrypt(KeyBlob, KeyBlobLength, Ciphertext, CiphertextLength, Plaintext, PlaintextCapacity, PlaintextLength, NULL, NULL, 0, 0);
}

NTSTATUS NTAPI PhSymCryptRsaPkcs1Encrypt(
    _In_reads_bytes_(KeyBlobLength) PCVOID KeyBlob,
    _In_ SIZE_T KeyBlobLength,
    _In_reads_bytes_(PlaintextLength) PCVOID Plaintext,
    _In_ SIZE_T PlaintextLength,
    _Out_writes_bytes_to_(CiphertextCapacity, *CiphertextLength) PVOID Ciphertext,
    _In_ SIZE_T CiphertextCapacity,
    _Out_ PSIZE_T CiphertextLength
    )
{
    return PhpSymCryptRsaPublicEncrypt(KeyBlob, KeyBlobLength, Plaintext, PlaintextLength, Ciphertext, CiphertextCapacity, CiphertextLength, NULL, NULL, 0, 1);
}

NTSTATUS NTAPI PhSymCryptRsaPkcs1Decrypt(
    _In_reads_bytes_(KeyBlobLength) PCVOID KeyBlob,
    _In_ SIZE_T KeyBlobLength,
    _In_reads_bytes_(CiphertextLength) PCVOID Ciphertext,
    _In_ SIZE_T CiphertextLength,
    _Out_writes_bytes_to_(PlaintextCapacity, *PlaintextLength) PVOID Plaintext,
    _In_ SIZE_T PlaintextCapacity,
    _Out_ PSIZE_T PlaintextLength
    )
{
    return PhpSymCryptRsaPrivateDecrypt(KeyBlob, KeyBlobLength, Ciphertext, CiphertextLength, Plaintext, PlaintextCapacity, PlaintextLength, NULL, NULL, 0, 1);
}

NTSTATUS NTAPI PhSymCryptRsaOaepEncrypt(
    _In_reads_bytes_(KeyBlobLength) PCVOID KeyBlob,
    _In_ SIZE_T KeyBlobLength,
    _In_ PH_SYMCRYPT_HASH_ALGORITHM HashAlgorithm,
    _In_reads_bytes_opt_(LabelLength) PCVOID Label,
    _In_ SIZE_T LabelLength,
    _In_reads_bytes_(PlaintextLength) PCVOID Plaintext,
    _In_ SIZE_T PlaintextLength,
    _Out_writes_bytes_to_(CiphertextCapacity, *CiphertextLength) PVOID Ciphertext,
    _In_ SIZE_T CiphertextCapacity,
    _Out_ PSIZE_T CiphertextLength
    )
{
    return PhpSymCryptRsaPublicEncrypt(KeyBlob, KeyBlobLength, Plaintext, PlaintextLength, Ciphertext, CiphertextCapacity, CiphertextLength, &HashAlgorithm, Label, LabelLength, 2);
}

NTSTATUS NTAPI PhSymCryptRsaOaepDecrypt(
    _In_reads_bytes_(KeyBlobLength) PCVOID KeyBlob,
    _In_ SIZE_T KeyBlobLength,
    _In_ PH_SYMCRYPT_HASH_ALGORITHM HashAlgorithm,
    _In_reads_bytes_opt_(LabelLength) PCVOID Label,
    _In_ SIZE_T LabelLength,
    _In_reads_bytes_(CiphertextLength) PCVOID Ciphertext,
    _In_ SIZE_T CiphertextLength,
    _Out_writes_bytes_to_(PlaintextCapacity, *PlaintextLength) PVOID Plaintext,
    _In_ SIZE_T PlaintextCapacity,
    _Out_ PSIZE_T PlaintextLength
    )
{
    return PhpSymCryptRsaPrivateDecrypt(KeyBlob, KeyBlobLength, Ciphertext, CiphertextLength, Plaintext, PlaintextCapacity, PlaintextLength, &HashAlgorithm, Label, LabelLength, 2);
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
    SIZE_T expectedSize;
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

    if (!NT_SUCCESS(RtlSIZETMult((SIZE_T)header->cbKey, 2, &publicKeyLength)))
        return STATUS_INTEGER_OVERFLOW;

    //
    // Validate the rest of the blob is present.
    //

    if (!NT_SUCCESS(RtlSIZETAdd(sizeof(BCRYPT_ECCKEY_BLOB), publicKeyLength, &expectedSize)))
        return STATUS_INTEGER_OVERFLOW;

    if (KeyBlobLength < expectedSize)
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

    if (
        !NT_SUCCESS(RtlSIZETAdd(sizeof(BCRYPT_RSAKEY_BLOB), header->cbPublicExp, &expectedSize)) ||
        !NT_SUCCESS(RtlSIZETAdd(expectedSize, header->cbModulus, &expectedSize))
        )
    {
        return STATUS_INTEGER_OVERFLOW;
    }

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

    if (
        !NT_SUCCESS(RtlSIZETAdd(sizeof(BCRYPT_RSAKEY_BLOB), header->cbPublicExp, &expectedSize)) ||
        !NT_SUCCESS(RtlSIZETAdd(expectedSize, header->cbModulus, &expectedSize)) ||
        !NT_SUCCESS(RtlSIZETAdd(expectedSize, cbPrime1, &expectedSize)) ||
        !NT_SUCCESS(RtlSIZETAdd(expectedSize, cbPrime2, &expectedSize)))
    {
        return STATUS_INTEGER_OVERFLOW;
    }

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

    {
        SIZE_T bitLength;
        UINT bitLength32;

        if (!NT_SUCCESS(RtlSIZETMult(header->cbModulus, 8, &bitLength)) ||
            !NT_SUCCESS(RtlSIZETToUInt(bitLength, &bitLength32)))
            return STATUS_INTEGER_OVERFLOW;

        params.version = 1;
        params.nBitsOfModulus = bitLength32;
    }
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

    if (!Signature || !SignatureLength)
        return STATUS_INVALID_PARAMETER;

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

    memset(Signature, 0, SignatureCapacity);
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

    if (!Signature || !SignatureLength)
        return STATUS_INVALID_PARAMETER;

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

    memset(Signature, 0, SignatureCapacity);
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
    SIZE_T expectedSize;

    //
    // Blob must hold at least header + 32+32+32 = 96 bytes for P-256 private.
    //

    if (!NT_SUCCESS(RtlSIZETAdd(sizeof(BCRYPT_ECCKEY_BLOB), 96, &expectedSize)))
        return STATUS_INTEGER_OVERFLOW;

    if (KeyBlobLength < expectedSize)
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

    memset(Signature, 0, 64);

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

// ------------------------------------------------------------------------
// Parallel multi-hash API
//
// Wraps SymCrypt's native parallel SHA-256/384/512 implementations (up to
// 8-way parallel) with phlib's caller-owned context model and error handling.
// ------------------------------------------------------------------------

/**
 * Initializes a parallel hash context for batched multi-hash operations.
 *
 * Allocates hash state arrays and scratch buffers required by SymCrypt's
 * parallel implementation. Caller retains ownership of the context structure
 * and must eventually call PhSymCryptCleanupParallelHash.
 *
 * \param[out] ParallelHashContext Caller-allocated context structure.
 * \param[in] Algorithm Hash algorithm (SHA-256, SHA-384, or SHA-512).
 * \param[in] NumberOfHashes Number of independent hash streams (2-8, varies by algorithm).
 * \return STATUS_SUCCESS on success; STATUS_INVALID_PARAMETER if NumberOfHashes
 *         exceeds algorithm limits; STATUS_INSUFFICIENT_RESOURCES on allocation failure.
 */
NTSTATUS NTAPI PhSymCryptInitializeParallelHash(
    _Out_ PPH_SYMCRYPT_PARALLEL_HASH_CONTEXT ParallelHashContext,
    _In_ PH_SYMCRYPT_PARALLEL_HASH_ALGORITHM Algorithm,
    _In_ SIZE_T NumberOfHashes
    )
{
    SIZE_T stateSize, scratchSize, fixedScratchSize, perStateScratchSize;

    if (!ParallelHashContext || NumberOfHashes == 0)
        return STATUS_INVALID_PARAMETER;

    memset(ParallelHashContext, 0, sizeof(PH_SYMCRYPT_PARALLEL_HASH_CONTEXT));

    switch (Algorithm)
    {
    case PH_SYMCRYPT_PARALLEL_HASH_SHA256:
        {
            if (
                NumberOfHashes < PH_SYMCRYPT_PARALLEL_SHA256_MIN_PARALLELISM ||
                NumberOfHashes > PH_SYMCRYPT_PARALLEL_SHA256_MAX_PARALLELISM
                )
            {
                return STATUS_INVALID_PARAMETER;
            }

            stateSize = sizeof(SYMCRYPT_SHA256_STATE);
            fixedScratchSize = SYMCRYPT_PARALLEL_SHA256_FIXED_SCRATCH;
            perStateScratchSize = SYMCRYPT_PARALLEL_HASH_PER_STATE_SCRATCH;
            ParallelHashContext->ResultSize = PH_SYMCRYPT_SHA256_RESULT_SIZE;
        }
        break;
    case PH_SYMCRYPT_PARALLEL_HASH_SHA384:
        {
            if (
                NumberOfHashes < PH_SYMCRYPT_PARALLEL_SHA384_MIN_PARALLELISM ||
                NumberOfHashes > PH_SYMCRYPT_PARALLEL_SHA384_MAX_PARALLELISM
                )
            {
                return STATUS_INVALID_PARAMETER;
            }

            stateSize = sizeof(SYMCRYPT_SHA384_STATE);
            fixedScratchSize = SYMCRYPT_PARALLEL_SHA384_FIXED_SCRATCH;
            perStateScratchSize = SYMCRYPT_PARALLEL_HASH_PER_STATE_SCRATCH;
            ParallelHashContext->ResultSize = PH_SYMCRYPT_SHA384_RESULT_SIZE;
        }
        break;
    case PH_SYMCRYPT_PARALLEL_HASH_SHA512:
        {
            if (
                NumberOfHashes < PH_SYMCRYPT_PARALLEL_SHA512_MIN_PARALLELISM ||
                NumberOfHashes > PH_SYMCRYPT_PARALLEL_SHA512_MAX_PARALLELISM
                )
            {
                return STATUS_INVALID_PARAMETER;
            }

            stateSize = sizeof(SYMCRYPT_SHA512_STATE);
            fixedScratchSize = SYMCRYPT_PARALLEL_SHA512_FIXED_SCRATCH;
            perStateScratchSize = SYMCRYPT_PARALLEL_HASH_PER_STATE_SCRATCH;
            ParallelHashContext->ResultSize = PH_SYMCRYPT_SHA512_RESULT_SIZE;
        }
        break;
    default:
        return STATUS_INVALID_PARAMETER;
    }

    ParallelHashContext->Algorithm = Algorithm;
    ParallelHashContext->NumberOfHashes = NumberOfHashes;

    if (!NT_SUCCESS(RtlSIZETMult(stateSize, NumberOfHashes, &stateSize)))
        return STATUS_INTEGER_OVERFLOW;

    //
    // Allocate state array
    //

    ParallelHashContext->pHashStates = PhAllocateSafe(stateSize);
    if (!ParallelHashContext->pHashStates)
        return STATUS_INSUFFICIENT_RESOURCES;

    //
    // Allocate scratch buffer: fixed portion + per-state portion
    //

    if (
        !NT_SUCCESS(RtlSIZETMult(perStateScratchSize, NumberOfHashes, &scratchSize)) ||
        !NT_SUCCESS(RtlSIZETAdd(fixedScratchSize, scratchSize, &scratchSize))
        )
    {
        PhFree(ParallelHashContext->pHashStates);
        ParallelHashContext->pHashStates = NULL;
        return STATUS_INTEGER_OVERFLOW;
    }

    ParallelHashContext->pbScratchBuffer = PhAllocateSafe(scratchSize);
    if (!ParallelHashContext->pbScratchBuffer)
    {
        PhFree(ParallelHashContext->pHashStates);
        ParallelHashContext->pHashStates = NULL;
        return STATUS_INSUFFICIENT_RESOURCES;
    }

    ParallelHashContext->cbScratchBuffer = scratchSize;

    //
    // Initialize all hash states to their initial values
    //

    switch (Algorithm)
    {
    case PH_SYMCRYPT_PARALLEL_HASH_SHA256:
        {
            SymCryptParallelSha256Init(
                (PSYMCRYPT_SHA256_STATE)ParallelHashContext->pHashStates,
                NumberOfHashes
                );
        }
        break;
    case PH_SYMCRYPT_PARALLEL_HASH_SHA384:
        {
            SymCryptParallelSha384Init(
                (PSYMCRYPT_SHA384_STATE)ParallelHashContext->pHashStates,
                NumberOfHashes
                );
        }
        break;
    case PH_SYMCRYPT_PARALLEL_HASH_SHA512:
        {
            SymCryptParallelSha512Init(
                (PSYMCRYPT_SHA512_STATE)ParallelHashContext->pHashStates,
                NumberOfHashes
                );
        }
        break;
    }

    return STATUS_SUCCESS;
}

/**
 * Processes a batch of hash operations on an initialized parallel context.
 *
 * Operations can be arbitrarily interleaved—append data to any hash or finish
 * any hash in any order. Each operation specifies a target hash index (0 to
 * NumberOfHashes-1), the operation type (append or result), and the data/buffer.
 *
 * \param[in] ParallelHashContext Initialized context.
 * \param[in,out] Operations Array of operations to process. Results extracted via
 *        operations with hashOperation == HASH_OPERATION_RESULT.
 * \param[in] OperationCount Number of operations in the array.
 * \return STATUS_SUCCESS on success; STATUS_INVALID_PARAMETER if any operation
 *         references an invalid hash index or other parameter error.
 */
NTSTATUS NTAPI PhSymCryptProcessParallelHashOperations(
    _In_ PPH_SYMCRYPT_PARALLEL_HASH_CONTEXT ParallelHashContext,
    _Inout_updates_(OperationCount) PPH_SYMCRYPT_PARALLEL_HASH_OPERATION Operations,
    _In_ SIZE_T OperationCount
    )
{
    SYMCRYPT_ERROR error;
    SYMCRYPT_PARALLEL_HASH_OPERATION *symcryptOps;
    SIZE_T symcryptOpsSize;
    SIZE_T i;

    if (!ParallelHashContext->pHashStates || !Operations || OperationCount == 0)
        return STATUS_INVALID_PARAMETER;

    //
    // Validate all operations before processing
    //

    for (i = 0; i < OperationCount; i++)
    {
        if (Operations[i].iHash >= ParallelHashContext->NumberOfHashes)
            return STATUS_INVALID_PARAMETER;
        if (Operations[i].hashOperation != PH_SYMCRYPT_HASH_OPERATION_APPEND &&
            Operations[i].hashOperation != PH_SYMCRYPT_HASH_OPERATION_RESULT)
            return STATUS_INVALID_PARAMETER;
    }

    if (!NT_SUCCESS(RtlSIZETMult(OperationCount, sizeof(SYMCRYPT_PARALLEL_HASH_OPERATION), &symcryptOpsSize)))
        return STATUS_INTEGER_OVERFLOW;

    //
    // Convert operation array to SymCrypt format
    //

    symcryptOps = (SYMCRYPT_PARALLEL_HASH_OPERATION *)PhAllocateSafe(symcryptOpsSize);
    if (!symcryptOps)
        return STATUS_INSUFFICIENT_RESOURCES;

    for (i = 0; i < OperationCount; i++)
    {
        symcryptOps[i].iHash = Operations[i].iHash;
        symcryptOps[i].hashOperation = (SYMCRYPT_HASH_OPERATION_TYPE)Operations[i].hashOperation;
        symcryptOps[i].pbBuffer = Operations[i].pbBuffer;
        symcryptOps[i].cbBuffer = Operations[i].cbBuffer;
    }

    //
    // Process operations based on algorithm
    //

    switch (ParallelHashContext->Algorithm)
    {
    case PH_SYMCRYPT_PARALLEL_HASH_SHA256:
        {
            error = SymCryptParallelSha256Process(
                (PSYMCRYPT_SHA256_STATE)ParallelHashContext->pHashStates,
                ParallelHashContext->NumberOfHashes,
                symcryptOps,
                OperationCount,
                ParallelHashContext->pbScratchBuffer,
                ParallelHashContext->cbScratchBuffer
                );
        }
        break;
    case PH_SYMCRYPT_PARALLEL_HASH_SHA384:
        {
            error = SymCryptParallelSha384Process(
                (PSYMCRYPT_SHA384_STATE)ParallelHashContext->pHashStates,
                ParallelHashContext->NumberOfHashes,
                symcryptOps,
                OperationCount,
                ParallelHashContext->pbScratchBuffer,
                ParallelHashContext->cbScratchBuffer
                );
        }
        break;
    case PH_SYMCRYPT_PARALLEL_HASH_SHA512:
        {
            error = SymCryptParallelSha512Process(
                (PSYMCRYPT_SHA512_STATE)ParallelHashContext->pHashStates,
                ParallelHashContext->NumberOfHashes,
                symcryptOps,
                OperationCount,
                ParallelHashContext->pbScratchBuffer,
                ParallelHashContext->cbScratchBuffer
                );
        }
        break;
    default:
        PhFree(symcryptOps);
        return STATUS_INVALID_PARAMETER;
    }

    PhFree(symcryptOps);
    return PhSymCryptErrorToStatus(error);
}

/**
 * Cleans up a parallel hash context and frees all allocated resources.
 *
 * Wipes sensitive hash state data before freeing. Safe to call on a context
 * that was never successfully initialized (checks for NULL pointers).
 * \param[in,out] ParallelHashContext Context to clean up.
 */
VOID NTAPI PhSymCryptCleanupParallelHash(
    _Inout_ PPH_SYMCRYPT_PARALLEL_HASH_CONTEXT ParallelHashContext
    )
{
    if (!ParallelHashContext || !ParallelHashContext->pHashStates)
        return;

    //
    // Wipe hash state before freeing
    //

    if (ParallelHashContext->cbScratchBuffer > 0)
    {
        SIZE_T stateSize = 0;
        SIZE_T stateTotalSize;
        switch (ParallelHashContext->Algorithm)
        {
        case PH_SYMCRYPT_PARALLEL_HASH_SHA256:
            stateSize = sizeof(SYMCRYPT_SHA256_STATE);
            break;
        case PH_SYMCRYPT_PARALLEL_HASH_SHA384:
            stateSize = sizeof(SYMCRYPT_SHA384_STATE);
            break;
        case PH_SYMCRYPT_PARALLEL_HASH_SHA512:
            stateSize = sizeof(SYMCRYPT_SHA512_STATE);
            break;
        }
        if (!NT_SUCCESS(RtlSIZETMult(stateSize, ParallelHashContext->NumberOfHashes, &stateTotalSize)))
            stateTotalSize = ParallelHashContext->cbScratchBuffer;

        SymCryptWipe(
            ParallelHashContext->pHashStates,
            stateTotalSize
            );
    }

    PhFree(ParallelHashContext->pHashStates);
    ParallelHashContext->pHashStates = NULL;

    if (ParallelHashContext->pbScratchBuffer)
    {
        SymCryptWipe(
            ParallelHashContext->pbScratchBuffer,
            ParallelHashContext->cbScratchBuffer
            );
        PhFree(ParallelHashContext->pbScratchBuffer);
        ParallelHashContext->pbScratchBuffer = NULL;
    }

    memset(ParallelHashContext, 0, sizeof(PH_SYMCRYPT_PARALLEL_HASH_CONTEXT));
}

/**
 * Reports the min/max supported parallelism for a given algorithm.
 *
 * Helps callers choose optimal batch sizes. SymCrypt's implementations support
 * 2-8 parallel hashes for SHA-256/384/512 depending on CPU architecture.
 *
 * \param[in] Algorithm Hash algorithm.
 * \param[out] MinimumParallelism Receives minimum supported parallelism (optional).
 * \param[out] MaximumParallelism Receives maximum supported parallelism (optional).
 */
VOID NTAPI PhSymCryptGetParallelHashCapabilities(
    _In_ PH_SYMCRYPT_PARALLEL_HASH_ALGORITHM Algorithm,
    _Out_opt_ PSIZE_T MinimumParallelism,
    _Out_opt_ PSIZE_T MaximumParallelism
    )
{
    switch (Algorithm)
    {
    case PH_SYMCRYPT_PARALLEL_HASH_SHA256:
        if (MinimumParallelism)
            *MinimumParallelism = PH_SYMCRYPT_PARALLEL_SHA256_MIN_PARALLELISM;
        if (MaximumParallelism)
            *MaximumParallelism = PH_SYMCRYPT_PARALLEL_SHA256_MAX_PARALLELISM;
        break;
    case PH_SYMCRYPT_PARALLEL_HASH_SHA384:
        if (MinimumParallelism)
            *MinimumParallelism = PH_SYMCRYPT_PARALLEL_SHA384_MIN_PARALLELISM;
        if (MaximumParallelism)
            *MaximumParallelism = PH_SYMCRYPT_PARALLEL_SHA384_MAX_PARALLELISM;
        break;
    case PH_SYMCRYPT_PARALLEL_HASH_SHA512:
        if (MinimumParallelism)
            *MinimumParallelism = PH_SYMCRYPT_PARALLEL_SHA512_MIN_PARALLELISM;
        if (MaximumParallelism)
            *MaximumParallelism = PH_SYMCRYPT_PARALLEL_SHA512_MAX_PARALLELISM;
        break;
    default:
        if (MinimumParallelism)
            *MinimumParallelism = 0;
        if (MaximumParallelism)
            *MaximumParallelism = 0;
        break;
    }
}

// ------------------------------------------------------------------------
// Handle-based symmetric key abstraction (BCrypt emulation)
// ------------------------------------------------------------------------

#define PH_SYMCRYPT_SYMMETRIC_KEY_MAGIC 'ySyP'

typedef enum _PH_SYMCRYPT_SYMMETRIC_ALGORITHM
{
    PhSymCryptSymmetricAlgorithmAes
} PH_SYMCRYPT_SYMMETRIC_ALGORITHM;

typedef enum _PH_SYMCRYPT_CHAIN_MODE
{
    PhSymCryptChainModeNone,
    PhSymCryptChainModeEcb,
    PhSymCryptChainModeCbc,
    PhSymCryptChainModeCfb,
    PhSymCryptChainModeGcm,
    PhSymCryptChainModeCcm
} PH_SYMCRYPT_CHAIN_MODE;

typedef struct _PH_SYMCRYPT_SYMMETRIC_KEY
{
    ULONG Magic;
    PH_SYMCRYPT_SYMMETRIC_ALGORITHM Algorithm;
    PH_SYMCRYPT_CHAIN_MODE ChainMode;
    ULONG SecretLength;
    UCHAR Secret[64];
} PH_SYMCRYPT_SYMMETRIC_KEY, *PPH_SYMCRYPT_SYMMETRIC_KEY;

static PPH_SYMCRYPT_SYMMETRIC_KEY PhpSymCryptGetSymmetricKey(
    _In_ PH_SYMCRYPT_KEY_HANDLE KeyHandle
    )
{
    PPH_SYMCRYPT_SYMMETRIC_KEY key = (PPH_SYMCRYPT_SYMMETRIC_KEY)KeyHandle;

    if (!key || key->Magic != PH_SYMCRYPT_SYMMETRIC_KEY_MAGIC)
        return NULL;

    return key;
}

NTSTATUS NTAPI PhSymCryptGenerateSymmetricKey(
    _In_ PCWSTR Algorithm,
    _Out_ PPH_SYMCRYPT_KEY_HANDLE KeyHandle,
    _In_reads_bytes_(SecretLength) PCVOID Secret,
    _In_ ULONG SecretLength
    )
{
    PPH_SYMCRYPT_SYMMETRIC_KEY key;

    if (!Algorithm || !KeyHandle || !Secret)
        return STATUS_INVALID_PARAMETER;

    *KeyHandle = NULL;

    if (!PhEqualStringZ(Algorithm, BCRYPT_AES_ALGORITHM, TRUE))
        return STATUS_NOT_SUPPORTED;

    if (SecretLength != 16 && SecretLength != 24 && SecretLength != 32)
        return STATUS_INVALID_PARAMETER;

    key = PhAllocateZero(sizeof(PH_SYMCRYPT_SYMMETRIC_KEY));
    key->Magic = PH_SYMCRYPT_SYMMETRIC_KEY_MAGIC;
    key->Algorithm = PhSymCryptSymmetricAlgorithmAes;
    key->ChainMode = PhSymCryptChainModeCbc;
    key->SecretLength = SecretLength;
    memcpy(key->Secret, Secret, SecretLength);

    *KeyHandle = key;
    return STATUS_SUCCESS;
}

NTSTATUS NTAPI PhSymCryptSetProperty(
    _In_ PH_SYMCRYPT_KEY_HANDLE KeyHandle,
    _In_ PCWSTR Property,
    _In_reads_bytes_opt_(InputLength) PCVOID Input,
    _In_ ULONG InputLength
    )
{
    PPH_SYMCRYPT_SYMMETRIC_KEY key = PhpSymCryptGetSymmetricKey(KeyHandle);
    PCWSTR value;

    if (!key || !Property || !Input)
        return STATUS_INVALID_PARAMETER;

    if (!PhEqualStringZ(Property, BCRYPT_CHAINING_MODE, TRUE))
        return STATUS_NOT_SUPPORTED;

    if ((InputLength % sizeof(WCHAR)) != 0)
        return STATUS_INVALID_PARAMETER;

    value = (PCWSTR)Input;

    if (PhEqualStringZ(value, BCRYPT_CHAIN_MODE_NA, TRUE))
        key->ChainMode = PhSymCryptChainModeNone;
    else if (PhEqualStringZ(value, BCRYPT_CHAIN_MODE_ECB, TRUE))
        key->ChainMode = PhSymCryptChainModeEcb;
    else if (PhEqualStringZ(value, BCRYPT_CHAIN_MODE_CBC, TRUE))
        key->ChainMode = PhSymCryptChainModeCbc;
    else if (PhEqualStringZ(value, BCRYPT_CHAIN_MODE_CFB, TRUE))
        key->ChainMode = PhSymCryptChainModeCfb;
    else if (PhEqualStringZ(value, BCRYPT_CHAIN_MODE_GCM, TRUE))
        key->ChainMode = PhSymCryptChainModeGcm;
    else if (PhEqualStringZ(value, BCRYPT_CHAIN_MODE_CCM, TRUE))
        key->ChainMode = PhSymCryptChainModeCcm;
    else
        return STATUS_NOT_SUPPORTED;

    return STATUS_SUCCESS;
}

static NTSTATUS PhpSymCryptAesCryptByKey(
    _In_ BOOLEAN Encrypt,
    _In_ PPH_SYMCRYPT_SYMMETRIC_KEY Key,
    _In_reads_bytes_opt_(InputLength) PCVOID Input,
    _In_ ULONG InputLength,
    _In_opt_ PVOID PaddingInfo,
    _Inout_updates_bytes_opt_(IvLength) PVOID Iv,
    _In_ ULONG IvLength,
    _Out_writes_bytes_to_opt_(OutputLength, *ResultLength) PVOID Output,
    _In_ ULONG OutputLength,
    _Out_ PULONG ResultLength,
    _In_ ULONG Flags
    )
{
    NTSTATUS status;

    if (!Key || !ResultLength)
        return STATUS_INVALID_PARAMETER;

    *ResultLength = 0;

    if (PaddingInfo)
        return STATUS_NOT_SUPPORTED;

    if (!Output)
    {
        *ResultLength = InputLength;
        return STATUS_SUCCESS;
    }

    if (OutputLength < InputLength)
        return STATUS_BUFFER_TOO_SMALL;

    switch (Key->ChainMode)
    {
    case PhSymCryptChainModeEcb:
        status = Encrypt ?
            PhSymCryptAesEcbEncrypt(Key->Secret, Key->SecretLength, Input, Output, InputLength) :
            PhSymCryptAesEcbDecrypt(Key->Secret, Key->SecretLength, Input, Output, InputLength);
        break;
    case PhSymCryptChainModeCbc:
        if (!Iv || IvLength != PH_AES_BLOCK_SIZE)
            return STATUS_INVALID_PARAMETER;
        status = Encrypt ?
            PhSymCryptAesCbcEncrypt(Key->Secret, Key->SecretLength, Iv, Input, Output, InputLength) :
            PhSymCryptAesCbcDecrypt(Key->Secret, Key->SecretLength, Iv, Input, Output, InputLength);
        break;
    case PhSymCryptChainModeCfb:
        if (!Iv || IvLength != PH_AES_BLOCK_SIZE)
            return STATUS_INVALID_PARAMETER;
        status = Encrypt ?
            PhSymCryptAesCfbEncrypt(Key->Secret, Key->SecretLength, Iv, PH_AES_BLOCK_SIZE, Input, Output, InputLength) :
            PhSymCryptAesCfbDecrypt(Key->Secret, Key->SecretLength, Iv, PH_AES_BLOCK_SIZE, Input, Output, InputLength);
        break;
    case PhSymCryptChainModeGcm:
    case PhSymCryptChainModeCcm:
        return STATUS_NOT_SUPPORTED;
    default:
        status = Encrypt ?
            PhSymCryptAesEcbEncrypt(Key->Secret, Key->SecretLength, Input, Output, InputLength) :
            PhSymCryptAesEcbDecrypt(Key->Secret, Key->SecretLength, Input, Output, InputLength);
        break;
    }

    if (NT_SUCCESS(status))
        *ResultLength = InputLength;

    UNREFERENCED_PARAMETER(Flags);
    return status;
}

NTSTATUS NTAPI PhSymCryptEncrypt(
    _In_ PH_SYMCRYPT_KEY_HANDLE KeyHandle,
    _In_reads_bytes_opt_(InputLength) PCVOID Input,
    _In_ ULONG InputLength,
    _In_opt_ PVOID PaddingInfo,
    _Inout_updates_bytes_opt_(IvLength) PVOID Iv,
    _In_ ULONG IvLength,
    _Out_writes_bytes_to_opt_(OutputLength, *ResultLength) PVOID Output,
    _In_ ULONG OutputLength,
    _Out_ PULONG ResultLength,
    _In_ ULONG Flags
    )
{
    return PhpSymCryptAesCryptByKey(TRUE, PhpSymCryptGetSymmetricKey(KeyHandle), Input, InputLength, PaddingInfo, Iv, IvLength, Output, OutputLength, ResultLength, Flags);
}

NTSTATUS NTAPI PhSymCryptDecrypt(
    _In_ PH_SYMCRYPT_KEY_HANDLE KeyHandle,
    _In_reads_bytes_opt_(InputLength) PCVOID Input,
    _In_ ULONG InputLength,
    _In_opt_ PVOID PaddingInfo,
    _Inout_updates_bytes_opt_(IvLength) PVOID Iv,
    _In_ ULONG IvLength,
    _Out_writes_bytes_to_opt_(OutputLength, *ResultLength) PVOID Output,
    _In_ ULONG OutputLength,
    _Out_ PULONG ResultLength,
    _In_ ULONG Flags
    )
{
    return PhpSymCryptAesCryptByKey(FALSE, PhpSymCryptGetSymmetricKey(KeyHandle), Input, InputLength, PaddingInfo, Iv, IvLength, Output, OutputLength, ResultLength, Flags);
}

// ------------------------------------------------------------------------
// Handle-based Asymmetric Key Abstraction (BCrypt Emulation)
// ------------------------------------------------------------------------

#define PH_SYMCRYPT_KEY_MAGIC 'SKey'

typedef enum _PH_SYMCRYPT_KEY_ALGORITHM
{
    PhSymCryptKeyAlgorithmRsa,
    PhSymCryptKeyAlgorithmEcdsa
} PH_SYMCRYPT_KEY_ALGORITHM;

typedef struct _PH_SYMCRYPT_KEY
{
    ULONG Magic;
    PH_SYMCRYPT_KEY_ALGORITHM Algorithm;
    ULONG BitLength;
    PSYMCRYPT_RSAKEY RsaKey;
    PSYMCRYPT_ECKEY EcKey;
    PSYMCRYPT_ECURVE Curve;
} PH_SYMCRYPT_KEY, *PPH_SYMCRYPT_KEY;

/**
 * Generates a new asymmetric key handle for the specified algorithm and length.
 *
 * This function allocates and initializes a PH_SYMCRYPT_KEY structure for RSA or ECDSA.
 * The actual key material is generated by PhSymCryptFinalizeKeyPair.
 *
 * \param[in] Algorithm Algorithm name (e.g., BCRYPT_RSA_ALGORITHM, BCRYPT_ECDSA_P256_ALGORITHM).
 * \param[out] KeyHandle Receives the new key handle.
 * \param[in] Length Key size in bits (RSA: modulus bits, ECDSA: must be 256).
 * \return NTSTATUS Successful or errant status.
 */
NTSTATUS NTAPI PhSymCryptGenerateKeyPair(
    _In_ PWSTR Algorithm,
    _Out_ PPH_SYMCRYPT_KEY_HANDLE KeyHandle,
    _In_ ULONG Length
    )
{
    PPH_SYMCRYPT_KEY key;

    if (!KeyHandle)
        return STATUS_INVALID_PARAMETER;

    *KeyHandle = NULL;

    key = PhAllocateZero(sizeof(PH_SYMCRYPT_KEY));
    key->Magic = PH_SYMCRYPT_KEY_MAGIC;
    key->BitLength = Length;

    if (PhEqualStringZ(Algorithm, BCRYPT_RSA_ALGORITHM, TRUE))
        key->Algorithm = PhSymCryptKeyAlgorithmRsa;
    else if (PhEqualStringZ(Algorithm, BCRYPT_ECDSA_P256_ALGORITHM, TRUE))
        key->Algorithm = PhSymCryptKeyAlgorithmEcdsa;
    else
    {
        PhFree(key);
        return STATUS_NOT_SUPPORTED;
    }

    *KeyHandle = key;
    return STATUS_SUCCESS;
}

/**
 * Finalizes a key handle by generating the actual key material.
 *
 * For RSA, generates a new key pair with a fixed public exponent (65537).
 * For ECDSA, generates a new P-256 key pair.
 *
 * \param[in] KeyHandle Key handle to finalize.
 * \return NTSTATUS Successful or errant status.
 */
NTSTATUS NTAPI PhSymCryptFinalizeKeyPair(
    _In_ PH_SYMCRYPT_KEY_HANDLE KeyHandle
    )
{
    PPH_SYMCRYPT_KEY key = (PPH_SYMCRYPT_KEY)KeyHandle;
    SYMCRYPT_ERROR error;

    if (!key || key->Magic != PH_SYMCRYPT_KEY_MAGIC)
        return STATUS_INVALID_PARAMETER;

    if (key->Algorithm == PhSymCryptKeyAlgorithmRsa)
    {
        SYMCRYPT_RSA_PARAMS params;
        ULONG64 pubExp;

        params.version = 1;
        params.nBitsOfModulus = key->BitLength;
        params.nPrimes = 2;
        params.nPubExp = 1;

        key->RsaKey = SymCryptRsakeyAllocate(&params, 0);
        if (!key->RsaKey)
            return STATUS_INSUFFICIENT_RESOURCES;

        pubExp = 65537;
        error = SymCryptRsakeyGenerate(key->RsaKey, &pubExp, 1, SYMCRYPT_FLAG_RSAKEY_SIGN | SYMCRYPT_FLAG_RSAKEY_ENCRYPT);

        if (error != SYMCRYPT_NO_ERROR)
        {
            SymCryptRsakeyFree(key->RsaKey);
            key->RsaKey = NULL;
            return PhSymCryptErrorToStatus(error);
        }
    }
    else if (key->Algorithm == PhSymCryptKeyAlgorithmEcdsa)
    {
        if (key->BitLength != 256)
            return STATUS_NOT_SUPPORTED;

        key->Curve = SymCryptEcurveAllocate(SymCryptEcurveParamsNistP256, 0);
        if (!key->Curve)
            return STATUS_INSUFFICIENT_RESOURCES;

        key->EcKey = SymCryptEckeyAllocate(key->Curve);
        if (!key->EcKey)
        {
            SymCryptEcurveFree(key->Curve);
            key->Curve = NULL;
            return STATUS_INSUFFICIENT_RESOURCES;
        }

        error = SymCryptEckeySetRandom(SYMCRYPT_FLAG_ECKEY_ECDSA, key->EcKey);

        if (error != SYMCRYPT_NO_ERROR)
        {
            SymCryptEckeyFree(key->EcKey);
            SymCryptEcurveFree(key->Curve);
            key->EcKey = NULL;
            key->Curve = NULL;
            return PhSymCryptErrorToStatus(error);
        }
    }

    return STATUS_SUCCESS;
}

/**
 * Exports a key handle to a standard key blob.
 *
 * Supports exporting RSA and ECDSA keys in BCRYPT-compatible blob formats.
 *
 * \param[in] KeyHandle Key handle to export.
 * \param[in] BlobType Blob type string (e.g., BCRYPT_RSAPRIVATE_BLOB).
 * \param[out] Blob Output buffer for the key blob.
 * \param[in] BlobLength Size of the output buffer.
 * \param[out] ResultLength Receives the number of bytes written or required.
 * \return NTSTATUS Successful or errant status.
 */
NTSTATUS NTAPI PhSymCryptExportKey(
    _In_ PH_SYMCRYPT_KEY_HANDLE KeyHandle,
    _In_ PWSTR BlobType,
    _Out_writes_bytes_to_opt_(BlobLength, *ResultLength) PVOID Blob,
    _In_ ULONG BlobLength,
    _Out_ PULONG ResultLength
    )
{
    PPH_SYMCRYPT_KEY key = (PPH_SYMCRYPT_KEY)KeyHandle;
    SYMCRYPT_ERROR error;

    if (!key || key->Magic != PH_SYMCRYPT_KEY_MAGIC)
        return STATUS_INVALID_PARAMETER;

    if (key->Algorithm == PhSymCryptKeyAlgorithmRsa)
    {
        SIZE_T modulusLength = key->BitLength / 8;
        SIZE_T primeLength = modulusLength / 2;
        SIZE_T publicExpLength = sizeof(ULONG64);

        if (PhEqualStringZ(BlobType, BCRYPT_RSAPRIVATE_BLOB, TRUE))
        {
            SIZE_T primeBlobSize;
            SIZE_T privBlobSize;
            ULONG resultLength;

            if (
                !NT_SUCCESS(RtlSIZETMult(primeLength, 2, &primeBlobSize)) ||
                !NT_SUCCESS(RtlSIZETAdd(sizeof(BCRYPT_RSAKEY_BLOB), publicExpLength, &privBlobSize)) ||
                !NT_SUCCESS(RtlSIZETAdd(privBlobSize, modulusLength, &privBlobSize)) ||
                !NT_SUCCESS(RtlSIZETAdd(privBlobSize, primeBlobSize, &privBlobSize)) ||
                !NT_SUCCESS(RtlSIZETToULong(privBlobSize, &resultLength))
                )
            {
                return STATUS_INTEGER_OVERFLOW;
            }

            *ResultLength = resultLength;

            if (!Blob)
                return STATUS_SUCCESS;
            if (BlobLength < privBlobSize)
                return STATUS_BUFFER_TOO_SMALL;

            BCRYPT_RSAKEY_BLOB* header = (BCRYPT_RSAKEY_BLOB*)Blob;
            PBYTE expBuf = (PBYTE)(header + 1);
            PBYTE modBuf = expBuf + publicExpLength;
            PBYTE prime1Buf = modBuf + modulusLength;
            PBYTE prime2Buf = prime1Buf + primeLength;
            PBYTE primePtrs[2];
            SIZE_T primeSizes[2];
            ULONG64 extractedPubExp;

            primePtrs[0] = prime1Buf;
            primePtrs[1] = prime2Buf;
            primeSizes[0] = primeLength;
            primeSizes[1] = primeLength;

            error = SymCryptRsakeyGetValue(
                key->RsaKey,
                modBuf,
                modulusLength,
                &extractedPubExp,
                1,
                primePtrs,
                primeSizes,
                2,
                SYMCRYPT_NUMBER_FORMAT_MSB_FIRST,
                0
                );

            if (error != SYMCRYPT_NO_ERROR)
                return PhSymCryptErrorToStatus(error);

            header->Magic = BCRYPT_RSAPRIVATE_MAGIC;
            header->BitLength = key->BitLength;
            header->cbPublicExp = (ULONG)publicExpLength;
            header->cbModulus = (ULONG)modulusLength;
            header->cbPrime1 = (ULONG)primeLength;
            header->cbPrime2 = (ULONG)primeLength;

            for (ULONG i = 0; i < publicExpLength; i++)
            {
                expBuf[publicExpLength - 1 - i] = (BYTE)(extractedPubExp & 0xFF);
                extractedPubExp >>= 8;
            }
            return STATUS_SUCCESS;
        }
        else if (PhEqualStringZ(BlobType, BCRYPT_RSAPUBLIC_BLOB, TRUE))
        {
            SIZE_T pubBlobSize;
            ULONG resultLength;

            if (
                !NT_SUCCESS(RtlSIZETAdd(sizeof(BCRYPT_RSAKEY_BLOB), publicExpLength, &pubBlobSize)) ||
                !NT_SUCCESS(RtlSIZETAdd(pubBlobSize, modulusLength, &pubBlobSize)) ||
                !NT_SUCCESS(RtlSIZETToULong(pubBlobSize, &resultLength))
                )
            {
                return STATUS_INTEGER_OVERFLOW;
            }

            *ResultLength = resultLength;

            if (!Blob)
                return STATUS_SUCCESS;
            if (BlobLength < pubBlobSize)
                return STATUS_BUFFER_TOO_SMALL;

            BCRYPT_RSAKEY_BLOB* header = (BCRYPT_RSAKEY_BLOB*)Blob;
            PBYTE expBuf = (PBYTE)(header + 1);
            PBYTE modBuf = expBuf + publicExpLength;
            ULONG64 extractedPubExp;

            error = SymCryptRsakeyGetValue(
                key->RsaKey,
                modBuf,
                modulusLength,
                &extractedPubExp,
                1,
                NULL,
                NULL,
                0,
                SYMCRYPT_NUMBER_FORMAT_MSB_FIRST,
                0
                );

            if (error != SYMCRYPT_NO_ERROR)
                return PhSymCryptErrorToStatus(error);

            header->Magic = BCRYPT_RSAPUBLIC_MAGIC;
            header->BitLength = key->BitLength;
            header->cbPublicExp = (ULONG)publicExpLength;
            header->cbModulus = (ULONG)modulusLength;
            header->cbPrime1 = 0;
            header->cbPrime2 = 0;

            for (ULONG i = 0; i < publicExpLength; i++)
            {
                expBuf[publicExpLength - 1 - i] = (BYTE)(extractedPubExp & 0xFF);
                extractedPubExp >>= 8;
            }

            return STATUS_SUCCESS;
        }
    }
    else if (key->Algorithm == PhSymCryptKeyAlgorithmEcdsa)
    {
        if (key->BitLength != 256)
            return STATUS_NOT_SUPPORTED;

        if (PhEqualStringZ(BlobType, BCRYPT_ECCPRIVATE_BLOB, TRUE))
        {
            SIZE_T privateKeyMaterialSize;
            SIZE_T privBlobSize;
            ULONG resultLength;

            if (
                !NT_SUCCESS(RtlSIZETMult(32, 3, &privateKeyMaterialSize)) ||
                !NT_SUCCESS(RtlSIZETAdd(sizeof(BCRYPT_ECCKEY_BLOB), privateKeyMaterialSize, &privBlobSize)) ||
                !NT_SUCCESS(RtlSIZETToULong(privBlobSize, &resultLength))
                )
            {
                return STATUS_INTEGER_OVERFLOW;
            }

            *ResultLength = resultLength;

            if (!Blob)
                return STATUS_SUCCESS;
            if (BlobLength < privBlobSize)
                return STATUS_BUFFER_TOO_SMALL;

            BCRYPT_ECCKEY_BLOB* header = (BCRYPT_ECCKEY_BLOB*)Blob;
            PBYTE xyBuf = (PBYTE)(header + 1);
            PBYTE dBuf = xyBuf + 64;

            error = SymCryptEckeyGetValue(
                key->EcKey,
                dBuf,
                32,
                xyBuf,
                64,
                SYMCRYPT_NUMBER_FORMAT_MSB_FIRST,
                SYMCRYPT_ECPOINT_FORMAT_XY,
                0
                );

            if (error != SYMCRYPT_NO_ERROR)
                return PhSymCryptErrorToStatus(error);

            header->dwMagic = BCRYPT_ECDSA_PRIVATE_P256_MAGIC;
            header->cbKey = 32;

            return STATUS_SUCCESS;
        }
        else if (PhEqualStringZ(BlobType, BCRYPT_ECCPUBLIC_BLOB, TRUE))
        {
            SIZE_T publicKeyMaterialSize;
            SIZE_T pubBlobSize;
            ULONG resultLength;

            if (!NT_SUCCESS(RtlSIZETMult(32, 2, &publicKeyMaterialSize)) ||
                !NT_SUCCESS(RtlSIZETAdd(sizeof(BCRYPT_ECCKEY_BLOB), publicKeyMaterialSize, &pubBlobSize)) ||
                !NT_SUCCESS(RtlSIZETToULong(pubBlobSize, &resultLength)))
                return STATUS_INTEGER_OVERFLOW;

            *ResultLength = resultLength;

            if (!Blob)
                return STATUS_SUCCESS;
            if (BlobLength < pubBlobSize)
                return STATUS_BUFFER_TOO_SMALL;

            BCRYPT_ECCKEY_BLOB* header = (BCRYPT_ECCKEY_BLOB*)Blob;
            PBYTE xyBuf = (PBYTE)(header + 1);

            error = SymCryptEckeyGetValue(
                key->EcKey,
                NULL,
                0,
                xyBuf,
                64,
                SYMCRYPT_NUMBER_FORMAT_MSB_FIRST,
                SYMCRYPT_ECPOINT_FORMAT_XY,
                0
                );

            if (error != SYMCRYPT_NO_ERROR)
                return PhSymCryptErrorToStatus(error);

            header->dwMagic = BCRYPT_ECDSA_PUBLIC_P256_MAGIC;
            header->cbKey = 32;

            return STATUS_SUCCESS;
        }
    }

    return STATUS_NOT_SUPPORTED;
}

/**
 * Imports a key pair from a standard key blob.
 *
 * \param[in] Algorithm Algorithm name (e.g., BCRYPT_RSA_ALGORITHM).
 * \param[out] KeyHandle Receives the imported key handle.
 * \param[in] BlobType Blob type string.
 * \param[in] Blob Input buffer containing the key blob.
 * \param[in] BlobLength Size of the input buffer.
 * \return NTSTATUS Successful or errant status.
 * \remark Supports importing RSA and ECDSA keys in BCRYPT-compatible blob formats.
 */
NTSTATUS NTAPI PhSymCryptImportKeyPair(
    _In_ PWSTR Algorithm,
    _Out_ PPH_SYMCRYPT_KEY_HANDLE KeyHandle,
    _In_ PWSTR BlobType,
    _In_reads_bytes_(BlobLength) PVOID Blob,
    _In_ ULONG BlobLength
    )
{
    NTSTATUS status;
    PPH_SYMCRYPT_KEY key;
    SYMCRYPT_ERROR error;

    if (!KeyHandle)
        return STATUS_INVALID_PARAMETER;

    *KeyHandle = NULL;

    key = PhAllocateZero(sizeof(PH_SYMCRYPT_KEY));
    key->Magic = PH_SYMCRYPT_KEY_MAGIC;

    if (PhEqualStringZ(Algorithm, BCRYPT_RSA_ALGORITHM, TRUE))
    {
        key->Algorithm = PhSymCryptKeyAlgorithmRsa;

        if (
            PhEqualStringZ(BlobType, BCRYPT_RSAPRIVATE_BLOB, TRUE) ||
            PhEqualStringZ(BlobType, BCRYPT_RSAPUBLIC_BLOB, TRUE)
            )
        {
            BCRYPT_RSAKEY_BLOB* header = (BCRYPT_RSAKEY_BLOB*)Blob;
            SIZE_T modulusLength;

            if (BlobLength < sizeof(BCRYPT_RSAKEY_BLOB))
            {
                PhFree(key);
                return STATUS_INVALID_PARAMETER;
            }

            key->BitLength = header->BitLength;

            if (header->Magic == BCRYPT_RSAPRIVATE_MAGIC || header->Magic == BCRYPT_RSAFULLPRIVATE_MAGIC)
            {
                status = PhSymCryptRsaImportPrivateKey(
                    Blob, 
                    BlobLength, 
                    &key->RsaKey,
                    &modulusLength
                    );

                if (!NT_SUCCESS(status))
                {
                    PhFree(key);
                    return status;
                }
            }
            else if (header->Magic == BCRYPT_RSAPUBLIC_MAGIC)
            {
                PCBYTE exponent;
                PCBYTE modulus;
                ULONG64 publicExponent;
                SIZE_T expectedSize;

                if (header->cbPublicExp == 0 || header->cbPublicExp > sizeof(ULONG64))
                {
                    PhFree(key);
                    return STATUS_INVALID_PARAMETER;
                }

                if (header->cbModulus == 0)
                {
                    PhFree(key);
                    return STATUS_INVALID_PARAMETER;
                }

                if (
                    !NT_SUCCESS(RtlSIZETAdd(sizeof(BCRYPT_RSAKEY_BLOB), header->cbPublicExp, &expectedSize)) ||
                    !NT_SUCCESS(RtlSIZETAdd(expectedSize, header->cbModulus, &expectedSize))
                    )
                {
                    PhFree(key);
                    return STATUS_INTEGER_OVERFLOW;
                }

                if (BlobLength < expectedSize)
                {
                    PhFree(key);
                    return STATUS_INVALID_PARAMETER;
                }

                exponent = (PCBYTE)Blob + sizeof(BCRYPT_RSAKEY_BLOB);
                modulus = exponent + header->cbPublicExp;

                publicExponent = 0;
                for (ULONG i = 0; i < header->cbPublicExp; i++)
                {
                    publicExponent = (publicExponent << 8) | exponent[i];
                }

                status = PhSymCryptRsaImportPublicKey(
                    modulus, 
                    header->cbModulus,
                    publicExponent,
                    &key->RsaKey
                    );

                if (!NT_SUCCESS(status))
                {
                    PhFree(key);
                    return status;
                }
            }
            else
            {
                PhFree(key);
                return STATUS_NOT_SUPPORTED;
            }

            *KeyHandle = key;
            return STATUS_SUCCESS;
        }
    }
    else if (PhEqualStringZ(Algorithm, BCRYPT_ECDSA_P256_ALGORITHM, TRUE))
    {
        key->Algorithm = PhSymCryptKeyAlgorithmEcdsa;

        if (
            PhEqualStringZ(BlobType, BCRYPT_ECCPRIVATE_BLOB, TRUE) ||
            PhEqualStringZ(BlobType, BCRYPT_ECCPUBLIC_BLOB, TRUE)
            )
        {
            BCRYPT_ECCKEY_BLOB* header = (BCRYPT_ECCKEY_BLOB*)Blob;
            PBYTE xyBuf = (PBYTE)(header + 1);
            PBYTE dBuf = NULL;
            SIZE_T expectedSize;

            if (BlobLength < sizeof(BCRYPT_ECCKEY_BLOB))
            {
                PhFree(key);
                return STATUS_INVALID_PARAMETER;
            }

            if (header->cbKey != 32)
            {
                PhFree(key);
                return STATUS_INVALID_PARAMETER;
            }

            key->BitLength = 256;

            if (header->dwMagic == BCRYPT_ECDSA_PRIVATE_P256_MAGIC)
            {
                if (!NT_SUCCESS(RtlSIZETAdd(sizeof(BCRYPT_ECCKEY_BLOB), 96, &expectedSize)))
                {
                    PhFree(key);
                    return STATUS_INTEGER_OVERFLOW;
                }

                if (BlobLength < expectedSize)
                {
                    PhFree(key);
                    return STATUS_INVALID_PARAMETER;
                }
                dBuf = xyBuf + 64;
            }
            else if (header->dwMagic == BCRYPT_ECDSA_PUBLIC_P256_MAGIC)
            {
                if (!NT_SUCCESS(RtlSIZETAdd(sizeof(BCRYPT_ECCKEY_BLOB), 64, &expectedSize)))
                {
                    PhFree(key);
                    return STATUS_INTEGER_OVERFLOW;
                }

                if (BlobLength < expectedSize)
                {
                    PhFree(key);
                    return STATUS_INVALID_PARAMETER;
                }
            }
            else
            {
                PhFree(key);
                return STATUS_NOT_SUPPORTED;
            }

            key->Curve = SymCryptEcurveAllocate(SymCryptEcurveParamsNistP256, 0);
            if (!key->Curve)
            {
                PhFree(key);
                return STATUS_INSUFFICIENT_RESOURCES;
            }

            key->EcKey = SymCryptEckeyAllocate(key->Curve);
            if (!key->EcKey)
            {
                SymCryptEcurveFree(key->Curve);
                PhFree(key);
                return STATUS_INSUFFICIENT_RESOURCES;
            }

            error = SymCryptEckeySetValue(
                dBuf,
                dBuf ? 32 : 0,
                xyBuf,
                64,
                SYMCRYPT_NUMBER_FORMAT_MSB_FIRST,
                SYMCRYPT_ECPOINT_FORMAT_XY,
                SYMCRYPT_FLAG_ECKEY_ECDSA,
                key->EcKey
                );

            if (error != SYMCRYPT_NO_ERROR)
            {
                SymCryptEckeyFree(key->EcKey);
                SymCryptEcurveFree(key->Curve);
                PhFree(key);
                return PhSymCryptErrorToStatus(error);
            }

            *KeyHandle = key;
            return STATUS_SUCCESS;
        }
    }

    PhFree(key);
    return STATUS_NOT_SUPPORTED;
}

/**
 * Signs a hash value using the specified key handle.
 *
 * \param[in] KeyHandle Key handle to use for signing.
 * \param[in] PaddingInfo Padding information (RSA-PSS only).
 * \param[in] Hash Hash value to sign.
 * \param[in] HashLength Length of the hash value.
 * \param[out] Signature Output buffer for the signature.
 * \param[in] SignatureLength Size of the output buffer.
 * \param[out] ResultLength Receives the number of bytes written or required.
 * \param[in] Flags Padding flags (e.g., BCRYPT_PAD_PSS).
 * \return NTSTATUS Successful or errant status.
 * \remark Supports RSA-PSS and ECDSA signatures.
 */
NTSTATUS NTAPI PhSymCryptSignHash(
    _In_ PH_SYMCRYPT_KEY_HANDLE KeyHandle,
    _In_opt_ PVOID PaddingInfo,
    _In_reads_bytes_(HashLength) PVOID Hash,
    _In_ ULONG HashLength,
    _Out_writes_bytes_to_opt_(SignatureLength, *ResultLength) PVOID Signature,
    _In_ ULONG SignatureLength,
    _Out_ PULONG ResultLength,
    _In_ ULONG Flags
    )
{
    PPH_SYMCRYPT_KEY key = (PPH_SYMCRYPT_KEY)KeyHandle;
    SYMCRYPT_ERROR error;
    SIZE_T resultLength;

    if (!ResultLength)
        return STATUS_INVALID_PARAMETER;

    *ResultLength = 0;

    if (!key || key->Magic != PH_SYMCRYPT_KEY_MAGIC)
        return STATUS_INVALID_PARAMETER;

    if (key->Algorithm == PhSymCryptKeyAlgorithmRsa)
    {
        resultLength = SymCryptRsakeySizeofModulus(key->RsaKey);

        if (resultLength > ULONG_MAX)
            return STATUS_INTEGER_OVERFLOW;

        if (!Signature)
        {
            *ResultLength = (ULONG)resultLength;
            return STATUS_SUCCESS;
        }

        if (SignatureLength < resultLength)
            return STATUS_BUFFER_TOO_SMALL;

        if (Flags == BCRYPT_PAD_PSS && PaddingInfo)
        {
            BCRYPT_PSS_PADDING_INFO* pssInfo = (BCRYPT_PSS_PADDING_INFO*)PaddingInfo;
            PCSYMCRYPT_HASH hashAlg = NULL;
            SIZE_T signatureLength;

            if (PhEqualStringZ(pssInfo->pszAlgId, BCRYPT_SHA256_ALGORITHM, TRUE))
                hashAlg = SymCryptSha256Algorithm;
            else if (PhEqualStringZ(pssInfo->pszAlgId, BCRYPT_SHA384_ALGORITHM, TRUE))
                hashAlg = SymCryptSha384Algorithm;
            else if (PhEqualStringZ(pssInfo->pszAlgId, BCRYPT_SHA512_ALGORITHM, TRUE))
                hashAlg = SymCryptSha512Algorithm;
            else
                return STATUS_NOT_SUPPORTED;

            memset(Signature, 0, SignatureLength);
            signatureLength = SignatureLength;

            error = SymCryptRsaPssSign(
                key->RsaKey,
                Hash,
                HashLength,
                hashAlg,
                pssInfo->cbSalt,
                0,
                SYMCRYPT_NUMBER_FORMAT_MSB_FIRST,
                Signature,
                SignatureLength,
                &signatureLength
                );

            if (error != SYMCRYPT_NO_ERROR)
                return PhSymCryptErrorToStatus(error);

            if (signatureLength > ULONG_MAX)
                return STATUS_INTEGER_OVERFLOW;

            *ResultLength = (ULONG)signatureLength;
            return STATUS_SUCCESS;
        }

        return STATUS_NOT_SUPPORTED;
    }
    else if (key->Algorithm == PhSymCryptKeyAlgorithmEcdsa)
    {
        resultLength = 64; // P256 Signature length (R + S)

        if (!Signature)
        {
            *ResultLength = (ULONG)resultLength;
            return STATUS_SUCCESS;
        }
        if (SignatureLength < resultLength)
            return STATUS_BUFFER_TOO_SMALL;

        memset(Signature, 0, SignatureLength);

        error = SymCryptEcDsaSign(
            key->EcKey,
            Hash,
            HashLength,
            SYMCRYPT_NUMBER_FORMAT_MSB_FIRST,
            0,
            Signature,
            resultLength
            );

        if (error != SYMCRYPT_NO_ERROR)
            return PhSymCryptErrorToStatus(error);

        *ResultLength = (ULONG)resultLength;
        return STATUS_SUCCESS;
    }

    return STATUS_NOT_SUPPORTED;
}

/**
 * Verifies a signature for a hash value using the specified key handle.
 *
 * \param[in] KeyHandle Key handle to use for verification.
 * \param[in] PaddingInfo Padding information (RSA-PSS only).
 * \param[in] Hash Hash value to verify.
 * \param[in] HashLength Length of the hash value.
 * \param[in] Signature Signature to verify.
 * \param[in] SignatureLength Length of the signature.
 * \param[in] Flags Padding flags (e.g., BCRYPT_PAD_PSS).
 * \return NTSTATUS Successful or errant status.
 * \remark Supports RSA-PSS and ECDSA signatures.
 */
NTSTATUS NTAPI PhSymCryptVerifyHash(
    _In_ PH_SYMCRYPT_KEY_HANDLE KeyHandle,
    _In_opt_ PVOID PaddingInfo,
    _In_reads_bytes_(HashLength) PVOID Hash,
    _In_ ULONG HashLength,
    _In_reads_bytes_(SignatureLength) PVOID Signature,
    _In_ ULONG SignatureLength,
    _In_ ULONG Flags
    )
{
    PPH_SYMCRYPT_KEY key = (PPH_SYMCRYPT_KEY)KeyHandle;
    SYMCRYPT_ERROR error;

    if (!key || key->Magic != PH_SYMCRYPT_KEY_MAGIC)
        return STATUS_INVALID_PARAMETER;

    if (key->Algorithm == PhSymCryptKeyAlgorithmRsa)
    {
        if (Flags == BCRYPT_PAD_PSS && PaddingInfo)
        {
            BCRYPT_PSS_PADDING_INFO* pssInfo = (BCRYPT_PSS_PADDING_INFO*)PaddingInfo;
            PCSYMCRYPT_HASH hashAlg = NULL;

            if (PhEqualStringZ(pssInfo->pszAlgId, BCRYPT_SHA256_ALGORITHM, TRUE))
                hashAlg = SymCryptSha256Algorithm;
            else if (PhEqualStringZ(pssInfo->pszAlgId, BCRYPT_SHA384_ALGORITHM, TRUE))
                hashAlg = SymCryptSha384Algorithm;
            else if (PhEqualStringZ(pssInfo->pszAlgId, BCRYPT_SHA512_ALGORITHM, TRUE))
                hashAlg = SymCryptSha512Algorithm;
            else
                return STATUS_NOT_SUPPORTED;

            error = SymCryptRsaPssVerify(
                key->RsaKey,
                Hash,
                HashLength,
                Signature,
                SignatureLength,
                SYMCRYPT_NUMBER_FORMAT_MSB_FIRST,
                hashAlg,
                pssInfo->cbSalt,
                0
                );

            if (error != SYMCRYPT_NO_ERROR)
                return PhSymCryptErrorToStatus(error);

            return STATUS_SUCCESS;
        }

        return STATUS_NOT_SUPPORTED;
    }
    else if (key->Algorithm == PhSymCryptKeyAlgorithmEcdsa)
    {
        if (SignatureLength != 64)
            return STATUS_INVALID_SIGNATURE;

        error = SymCryptEcDsaVerify(
            key->EcKey,
            Hash,
            HashLength,
            Signature,
            SignatureLength,
            SYMCRYPT_NUMBER_FORMAT_MSB_FIRST,
            0
            );

        if (error != SYMCRYPT_NO_ERROR)
            return PhSymCryptErrorToStatus(error);

        return STATUS_SUCCESS;
    }

    return STATUS_NOT_SUPPORTED;
}

/**
 * Destroys a key handle and releases all associated resources.
 *
 * \param[in] KeyHandle Key handle to destroy.
 * \return NTSTATUS Successful or errant status.
 */
NTSTATUS NTAPI PhSymCryptDestroyKey(
    _In_ PH_SYMCRYPT_KEY_HANDLE KeyHandle
    )
{
    PPH_SYMCRYPT_KEY key = (PPH_SYMCRYPT_KEY)KeyHandle;
    PPH_SYMCRYPT_SYMMETRIC_KEY symmetricKey = (PPH_SYMCRYPT_SYMMETRIC_KEY)KeyHandle;

    if (symmetricKey && symmetricKey->Magic == PH_SYMCRYPT_SYMMETRIC_KEY_MAGIC)
    {
        SymCryptWipeKnownSize(symmetricKey, sizeof(PH_SYMCRYPT_SYMMETRIC_KEY));
        PhFree(symmetricKey);
        return STATUS_SUCCESS;
    }

    if (!key || key->Magic != PH_SYMCRYPT_KEY_MAGIC)
        return STATUS_INVALID_PARAMETER;

    if (key->RsaKey)
        SymCryptRsakeyFree(key->RsaKey);

    if (key->EcKey)
        SymCryptEckeyFree(key->EcKey);

    if (key->Curve)
        SymCryptEcurveFree(key->Curve);

    key->Magic = 0;
    PhFree(key);

    return STATUS_SUCCESS;
}

// ------------------------------------------------------------------------
// Asymmetric key generation
// ------------------------------------------------------------------------

NTSTATUS NTAPI PhSymCryptGenerateRsaKeyBlobs(
    _In_ ULONG Bits,
    _Out_writes_bytes_to_opt_(PrivateKeyBlobCapacity, *PrivateKeyBlobLength) PVOID PrivateKeyBlob,
    _In_ SIZE_T PrivateKeyBlobCapacity,
    _Out_opt_ PSIZE_T PrivateKeyBlobLength,
    _Out_writes_bytes_to_opt_(PublicKeyBlobCapacity, *PublicKeyBlobLength) PVOID PublicKeyBlob,
    _In_ SIZE_T PublicKeyBlobCapacity,
    _Out_opt_ PSIZE_T PublicKeyBlobLength
    )
{
    SIZE_T modulusLength;
    SIZE_T primeLength;
    SIZE_T publicExpLength;
    SIZE_T primeBlobSize;
    SIZE_T privBlobSize;
    SIZE_T pubBlobSize;
    SYMCRYPT_RSA_PARAMS params;
    PSYMCRYPT_RSAKEY key;
    ULONG64 pubExp;
    SYMCRYPT_ERROR error;

    if (Bits < 512 || Bits > 16384 || (Bits % 64) != 0)
        return STATUS_INVALID_PARAMETER;

    modulusLength = Bits / 8;
    primeLength = modulusLength / 2;
    publicExpLength = sizeof(ULONG64);

    if (
        !NT_SUCCESS(RtlSIZETMult(primeLength, 2, &primeBlobSize)) ||
        !NT_SUCCESS(RtlSIZETAdd(sizeof(BCRYPT_RSAKEY_BLOB), publicExpLength, &pubBlobSize)) ||
        !NT_SUCCESS(RtlSIZETAdd(pubBlobSize, modulusLength, &pubBlobSize)) ||
        !NT_SUCCESS(RtlSIZETAdd(pubBlobSize, primeBlobSize, &privBlobSize))
        )
    {
        return STATUS_INTEGER_OVERFLOW;
    }

    if (PrivateKeyBlobLength)
        *PrivateKeyBlobLength = privBlobSize;

    if (PublicKeyBlobLength)
        *PublicKeyBlobLength = pubBlobSize;

    if ((PrivateKeyBlob && PrivateKeyBlobCapacity < privBlobSize) ||
        (PublicKeyBlob && PublicKeyBlobCapacity < pubBlobSize))
    {
        return STATUS_BUFFER_TOO_SMALL;
    }

    if (!PrivateKeyBlob && !PublicKeyBlob)
        return STATUS_SUCCESS;

    params.version = 1;
    params.nBitsOfModulus = Bits;
    params.nPrimes = 2;
    params.nPubExp = 1;

    key = SymCryptRsakeyAllocate(&params, 0);

    if (!key)
        return STATUS_INSUFFICIENT_RESOURCES;

    pubExp = 65537;

    error = SymCryptRsakeyGenerate(key, &pubExp, 1, SYMCRYPT_FLAG_RSAKEY_SIGN | SYMCRYPT_FLAG_RSAKEY_ENCRYPT);

    if (error != SYMCRYPT_NO_ERROR)
    {
        SymCryptRsakeyFree(key);
        return PhSymCryptErrorToStatus(error);
    }

    if (PrivateKeyBlob)
    {
        BCRYPT_RSAKEY_BLOB* header = (BCRYPT_RSAKEY_BLOB*)PrivateKeyBlob;
        PBYTE expBuf = (PBYTE)(header + 1);
        PBYTE modBuf = expBuf + publicExpLength;
        PBYTE prime1Buf = modBuf + modulusLength;
        PBYTE prime2Buf = prime1Buf + primeLength;
        PBYTE primePtrs[2];
        SIZE_T primeSizes[2];
        ULONG64 extractedPubExp;

        primePtrs[0] = prime1Buf;
        primePtrs[1] = prime2Buf;
        primeSizes[0] = primeLength;
        primeSizes[1] = primeLength;

        error = SymCryptRsakeyGetValue(
            key,
            modBuf,
            modulusLength,
            &extractedPubExp,
            1,
            primePtrs,
            primeSizes,
            2,
            SYMCRYPT_NUMBER_FORMAT_MSB_FIRST,
            0
            );

        if (error != SYMCRYPT_NO_ERROR)
        {
            SymCryptRsakeyFree(key);
            return PhSymCryptErrorToStatus(error);
        }

        header->Magic = BCRYPT_RSAPRIVATE_MAGIC;
        header->BitLength = Bits;
        header->cbPublicExp = (ULONG)publicExpLength;
        header->cbModulus = (ULONG)modulusLength;
        header->cbPrime1 = (ULONG)primeLength;
        header->cbPrime2 = (ULONG)primeLength;

        //
        // Big-endian encoding of public exponent
        //
        
        for (ULONG i = 0; i < publicExpLength; i++)
        {
            expBuf[publicExpLength - 1 - i] = (BYTE)(extractedPubExp & 0xFF);
            extractedPubExp >>= 8;
        }

        if (PublicKeyBlob)
        {
            BCRYPT_RSAKEY_BLOB* pubHeader = (BCRYPT_RSAKEY_BLOB*)PublicKeyBlob;
            PBYTE pubExpBuf = (PBYTE)(pubHeader + 1);
            PBYTE pubModBuf = pubExpBuf + publicExpLength;

            pubHeader->Magic = BCRYPT_RSAPUBLIC_MAGIC;
            pubHeader->BitLength = Bits;
            pubHeader->cbPublicExp = (ULONG)publicExpLength;
            pubHeader->cbModulus = (ULONG)modulusLength;
            pubHeader->cbPrime1 = 0;
            pubHeader->cbPrime2 = 0;

            memcpy(pubExpBuf, expBuf, publicExpLength);
            memcpy(pubModBuf, modBuf, modulusLength);
        }
    }
    else if (PublicKeyBlob)
    {
        // Path where only public key is requested.
        BCRYPT_RSAKEY_BLOB* header = (BCRYPT_RSAKEY_BLOB*)PublicKeyBlob;
        PBYTE expBuf = (PBYTE)(header + 1);
        PBYTE modBuf = expBuf + publicExpLength;
        ULONG64 extractedPubExp;

        error = SymCryptRsakeyGetValue(
            key,
            modBuf,
            modulusLength,
            &extractedPubExp,
            1,
            NULL,
            NULL,
            0,
            SYMCRYPT_NUMBER_FORMAT_MSB_FIRST,
            0
            );

        if (error != SYMCRYPT_NO_ERROR)
        {
            SymCryptRsakeyFree(key);
            return PhSymCryptErrorToStatus(error);
        }

        header->Magic = BCRYPT_RSAPUBLIC_MAGIC;
        header->BitLength = Bits;
        header->cbPublicExp = (ULONG)publicExpLength;
        header->cbModulus = (ULONG)modulusLength;
        header->cbPrime1 = 0;
        header->cbPrime2 = 0;

        for (ULONG i = 0; i < publicExpLength; i++)
        {
            expBuf[publicExpLength - 1 - i] = (BYTE)(extractedPubExp & 0xFF);
            extractedPubExp >>= 8;
        }
    }

    SymCryptRsakeyFree(key);
    return STATUS_SUCCESS;
}

// ------------------------------------------------------------------------
// Post-quantum in-memory keys
// ------------------------------------------------------------------------

#define PH_SYMCRYPT_MLKEM_KEY_MAGIC 'mKyP'
#define PH_SYMCRYPT_MLDSA_KEY_MAGIC 'dKyP'

typedef struct _PH_SYMCRYPT_MLKEM_KEY
{
    ULONG Magic;
    SYMCRYPT_MLKEM_PARAMS Parameters;
    PSYMCRYPT_MLKEMKEY Key;
} PH_SYMCRYPT_MLKEM_KEY, *PPH_SYMCRYPT_MLKEM_KEY;

typedef struct _PH_SYMCRYPT_MLDSA_KEY
{
    ULONG Magic;
    SYMCRYPT_MLDSA_PARAMS Parameters;
    PSYMCRYPT_MLDSAKEY Key;
} PH_SYMCRYPT_MLDSA_KEY, *PPH_SYMCRYPT_MLDSA_KEY;

static BOOLEAN PhpSymCryptMlKemMapParameters(
    _In_ PH_SYMCRYPT_MLKEM_PARAMETER_SET ParameterSet,
    _Out_ SYMCRYPT_MLKEM_PARAMS* Parameters
    )
{
    switch (ParameterSet)
    {
    case PH_SYMCRYPT_MLKEM_512:
        *Parameters = SYMCRYPT_MLKEM_PARAMS_MLKEM512;
        return TRUE;
    case PH_SYMCRYPT_MLKEM_768:
        *Parameters = SYMCRYPT_MLKEM_PARAMS_MLKEM768;
        return TRUE;
    case PH_SYMCRYPT_MLKEM_1024:
        *Parameters = SYMCRYPT_MLKEM_PARAMS_MLKEM1024;
        return TRUE;
    default:
        return FALSE;
    }
}

static BOOLEAN PhpSymCryptMlKemMapFormat(
    _In_ PH_SYMCRYPT_MLKEM_KEY_FORMAT Format,
    _Out_ SYMCRYPT_MLKEMKEY_FORMAT* SymCryptFormat
    )
{
    switch (Format)
    {
    case PH_SYMCRYPT_MLKEM_PRIVATE_SEED:
        *SymCryptFormat = SYMCRYPT_MLKEMKEY_FORMAT_PRIVATE_SEED;
        return TRUE;
    case PH_SYMCRYPT_MLKEM_DECAPSULATION_KEY:
        *SymCryptFormat = SYMCRYPT_MLKEMKEY_FORMAT_DECAPSULATION_KEY;
        return TRUE;
    case PH_SYMCRYPT_MLKEM_ENCAPSULATION_KEY:
        *SymCryptFormat = SYMCRYPT_MLKEMKEY_FORMAT_ENCAPSULATION_KEY;
        return TRUE;
    default:
        return FALSE;
    }
}

static PPH_SYMCRYPT_MLKEM_KEY PhpSymCryptGetMlKemKey(
    _In_ PH_SYMCRYPT_MLKEM_KEY_HANDLE KeyHandle
    )
{
    PPH_SYMCRYPT_MLKEM_KEY key = (PPH_SYMCRYPT_MLKEM_KEY)KeyHandle;

    if (!key || key->Magic != PH_SYMCRYPT_MLKEM_KEY_MAGIC)
        return NULL;

    return key;
}

NTSTATUS NTAPI PhSymCryptMlKemGenerateKey(
    _In_ PH_SYMCRYPT_MLKEM_PARAMETER_SET ParameterSet,
    _Out_ PPH_SYMCRYPT_MLKEM_KEY_HANDLE KeyHandle
    )
{
    PPH_SYMCRYPT_MLKEM_KEY key;
    SYMCRYPT_MLKEM_PARAMS parameters;
    SYMCRYPT_ERROR error;

    if (!KeyHandle)
        return STATUS_INVALID_PARAMETER;

    *KeyHandle = NULL;

    if (!PhpSymCryptMlKemMapParameters(ParameterSet, &parameters))
        return STATUS_NOT_SUPPORTED;

    key = PhAllocateZero(sizeof(PH_SYMCRYPT_MLKEM_KEY));
    key->Magic = PH_SYMCRYPT_MLKEM_KEY_MAGIC;
    key->Parameters = parameters;
    key->Key = SymCryptMlKemkeyAllocate(parameters);

    if (!key->Key)
    {
        PhFree(key);
        return STATUS_INSUFFICIENT_RESOURCES;
    }

    error = SymCryptMlKemkeyGenerate(key->Key, 0);
    if (error != SYMCRYPT_NO_ERROR)
    {
        SymCryptMlKemkeyFree(key->Key);
        PhFree(key);
        return PhSymCryptErrorToStatus(error);
    }

    *KeyHandle = key;
    return STATUS_SUCCESS;
}

NTSTATUS NTAPI PhSymCryptMlKemImportKey(
    _In_ PH_SYMCRYPT_MLKEM_PARAMETER_SET ParameterSet,
    _In_ PH_SYMCRYPT_MLKEM_KEY_FORMAT Format,
    _In_reads_bytes_(KeyBlobLength) PCVOID KeyBlob,
    _In_ SIZE_T KeyBlobLength,
    _Out_ PPH_SYMCRYPT_MLKEM_KEY_HANDLE KeyHandle
    )
{
    PPH_SYMCRYPT_MLKEM_KEY key;
    SYMCRYPT_MLKEM_PARAMS parameters;
    SYMCRYPT_MLKEMKEY_FORMAT format;
    SYMCRYPT_ERROR error;

    if (!KeyHandle || !KeyBlob)
        return STATUS_INVALID_PARAMETER;

    *KeyHandle = NULL;

    if (!PhpSymCryptMlKemMapParameters(ParameterSet, &parameters) || !PhpSymCryptMlKemMapFormat(Format, &format))
        return STATUS_NOT_SUPPORTED;

    key = PhAllocateZero(sizeof(PH_SYMCRYPT_MLKEM_KEY));
    key->Magic = PH_SYMCRYPT_MLKEM_KEY_MAGIC;
    key->Parameters = parameters;
    key->Key = SymCryptMlKemkeyAllocate(parameters);

    if (!key->Key)
    {
        PhFree(key);
        return STATUS_INSUFFICIENT_RESOURCES;
    }

    error = SymCryptMlKemkeySetValue((PCBYTE)KeyBlob, KeyBlobLength, format, 0, key->Key);
    if (error != SYMCRYPT_NO_ERROR)
    {
        SymCryptMlKemkeyFree(key->Key);
        PhFree(key);
        return PhSymCryptErrorToStatus(error);
    }

    *KeyHandle = key;
    return STATUS_SUCCESS;
}

NTSTATUS NTAPI PhSymCryptMlKemExportKey(
    _In_ PH_SYMCRYPT_MLKEM_KEY_HANDLE KeyHandle,
    _In_ PH_SYMCRYPT_MLKEM_KEY_FORMAT Format,
    _Out_writes_bytes_to_opt_(KeyBlobCapacity, *KeyBlobLength) PVOID KeyBlob,
    _In_ SIZE_T KeyBlobCapacity,
    _Out_ PSIZE_T KeyBlobLength
    )
{
    PPH_SYMCRYPT_MLKEM_KEY key = PhpSymCryptGetMlKemKey(KeyHandle);
    SYMCRYPT_MLKEMKEY_FORMAT format;
    SIZE_T required;
    SYMCRYPT_ERROR error;

    if (!key || !KeyBlobLength)
        return STATUS_INVALID_PARAMETER;

    if (!PhpSymCryptMlKemMapFormat(Format, &format))
        return STATUS_NOT_SUPPORTED;

    error = SymCryptMlKemSizeofKeyFormatFromParams(key->Parameters, format, &required);
    if (error != SYMCRYPT_NO_ERROR)
        return PhSymCryptErrorToStatus(error);

    *KeyBlobLength = required;

    if (!KeyBlob)
        return STATUS_SUCCESS;

    if (KeyBlobCapacity < required)
        return STATUS_BUFFER_TOO_SMALL;

    return PhSymCryptErrorToStatus(SymCryptMlKemkeyGetValue(key->Key, (PBYTE)KeyBlob, KeyBlobCapacity, format, 0));
}

NTSTATUS NTAPI PhSymCryptMlKemEncapsulate(
    _In_ PH_SYMCRYPT_MLKEM_KEY_HANDLE KeyHandle,
    _Out_writes_bytes_(SecretLength) PVOID Secret,
    _In_ SIZE_T SecretLength,
    _Out_writes_bytes_to_(CiphertextCapacity, *CiphertextLength) PVOID Ciphertext,
    _In_ SIZE_T CiphertextCapacity,
    _Out_ PSIZE_T CiphertextLength
    )
{
    PPH_SYMCRYPT_MLKEM_KEY key = PhpSymCryptGetMlKemKey(KeyHandle);
    SIZE_T required;
    SYMCRYPT_ERROR error;

    if (!key || !CiphertextLength)
        return STATUS_INVALID_PARAMETER;

    error = SymCryptMlKemSizeofCiphertextFromParams(key->Parameters, &required);
    if (error != SYMCRYPT_NO_ERROR)
        return PhSymCryptErrorToStatus(error);

    *CiphertextLength = required;

    if (CiphertextCapacity < required)
        return STATUS_BUFFER_TOO_SMALL;

    return PhSymCryptErrorToStatus(SymCryptMlKemEncapsulate(key->Key, (PBYTE)Secret, SecretLength, (PBYTE)Ciphertext, CiphertextCapacity));
}

NTSTATUS NTAPI PhSymCryptMlKemDecapsulate(
    _In_ PH_SYMCRYPT_MLKEM_KEY_HANDLE KeyHandle,
    _In_reads_bytes_(CiphertextLength) PCVOID Ciphertext,
    _In_ SIZE_T CiphertextLength,
    _Out_writes_bytes_(SecretLength) PVOID Secret,
    _In_ SIZE_T SecretLength
    )
{
    PPH_SYMCRYPT_MLKEM_KEY key = PhpSymCryptGetMlKemKey(KeyHandle);

    if (!key)
        return STATUS_INVALID_PARAMETER;

    return PhSymCryptErrorToStatus(SymCryptMlKemDecapsulate(key->Key, (PCBYTE)Ciphertext, CiphertextLength, (PBYTE)Secret, SecretLength));
}

VOID NTAPI PhSymCryptMlKemDestroyKey(
    _In_opt_ PH_SYMCRYPT_MLKEM_KEY_HANDLE KeyHandle
    )
{
    PPH_SYMCRYPT_MLKEM_KEY key = PhpSymCryptGetMlKemKey(KeyHandle);

    if (!key)
        return;

    if (key->Key)
        SymCryptMlKemkeyFree(key->Key);

    key->Magic = 0;
    PhFree(key);
}

static BOOLEAN PhpSymCryptMlDsaMapParameters(
    _In_ PH_SYMCRYPT_MLDSA_PARAMETER_SET ParameterSet,
    _Out_ SYMCRYPT_MLDSA_PARAMS* Parameters
    )
{
    switch (ParameterSet)
    {
    case PH_SYMCRYPT_MLDSA_44:
        *Parameters = SYMCRYPT_MLDSA_PARAMS_MLDSA44;
        return TRUE;
    case PH_SYMCRYPT_MLDSA_65:
        *Parameters = SYMCRYPT_MLDSA_PARAMS_MLDSA65;
        return TRUE;
    case PH_SYMCRYPT_MLDSA_87:
        *Parameters = SYMCRYPT_MLDSA_PARAMS_MLDSA87;
        return TRUE;
    default:
        return FALSE;
    }
}

static BOOLEAN PhpSymCryptMlDsaMapFormat(
    _In_ PH_SYMCRYPT_MLDSA_KEY_FORMAT Format,
    _Out_ SYMCRYPT_MLDSAKEY_FORMAT* SymCryptFormat
    )
{
    switch (Format)
    {
    case PH_SYMCRYPT_MLDSA_PRIVATE_SEED:
        *SymCryptFormat = SYMCRYPT_MLDSAKEY_FORMAT_PRIVATE_SEED;
        return TRUE;
    case PH_SYMCRYPT_MLDSA_PRIVATE_KEY:
        *SymCryptFormat = SYMCRYPT_MLDSAKEY_FORMAT_PRIVATE_KEY;
        return TRUE;
    case PH_SYMCRYPT_MLDSA_PUBLIC_KEY:
        *SymCryptFormat = SYMCRYPT_MLDSAKEY_FORMAT_PUBLIC_KEY;
        return TRUE;
    default:
        return FALSE;
    }
}

static PPH_SYMCRYPT_MLDSA_KEY PhpSymCryptGetMlDsaKey(
    _In_ PH_SYMCRYPT_MLDSA_KEY_HANDLE KeyHandle
    )
{
    PPH_SYMCRYPT_MLDSA_KEY key = (PPH_SYMCRYPT_MLDSA_KEY)KeyHandle;

    if (!key || key->Magic != PH_SYMCRYPT_MLDSA_KEY_MAGIC)
        return NULL;

    return key;
}

NTSTATUS NTAPI PhSymCryptMlDsaGenerateKey(
    _In_ PH_SYMCRYPT_MLDSA_PARAMETER_SET ParameterSet,
    _Out_ PPH_SYMCRYPT_MLDSA_KEY_HANDLE KeyHandle
    )
{
    PPH_SYMCRYPT_MLDSA_KEY key;
    SYMCRYPT_MLDSA_PARAMS parameters;
    SYMCRYPT_ERROR error;

    if (!KeyHandle)
        return STATUS_INVALID_PARAMETER;

    *KeyHandle = NULL;

    if (!PhpSymCryptMlDsaMapParameters(ParameterSet, &parameters))
        return STATUS_NOT_SUPPORTED;

    key = PhAllocateZero(sizeof(PH_SYMCRYPT_MLDSA_KEY));
    key->Magic = PH_SYMCRYPT_MLDSA_KEY_MAGIC;
    key->Parameters = parameters;
    key->Key = SymCryptMlDsakeyAllocate(parameters);

    if (!key->Key)
    {
        PhFree(key);
        return STATUS_INSUFFICIENT_RESOURCES;
    }

    error = SymCryptMlDsakeyGenerate(key->Key, 0);
    if (error != SYMCRYPT_NO_ERROR)
    {
        SymCryptMlDsakeyFree(key->Key);
        PhFree(key);
        return PhSymCryptErrorToStatus(error);
    }

    *KeyHandle = key;
    return STATUS_SUCCESS;
}

NTSTATUS NTAPI PhSymCryptMlDsaImportKey(
    _In_ PH_SYMCRYPT_MLDSA_PARAMETER_SET ParameterSet,
    _In_ PH_SYMCRYPT_MLDSA_KEY_FORMAT Format,
    _In_reads_bytes_(KeyBlobLength) PCVOID KeyBlob,
    _In_ SIZE_T KeyBlobLength,
    _Out_ PPH_SYMCRYPT_MLDSA_KEY_HANDLE KeyHandle
    )
{
    PPH_SYMCRYPT_MLDSA_KEY key;
    SYMCRYPT_MLDSA_PARAMS parameters;
    SYMCRYPT_MLDSAKEY_FORMAT format;
    SYMCRYPT_ERROR error;

    if (!KeyHandle || !KeyBlob)
        return STATUS_INVALID_PARAMETER;

    *KeyHandle = NULL;

    if (!PhpSymCryptMlDsaMapParameters(ParameterSet, &parameters) || !PhpSymCryptMlDsaMapFormat(Format, &format))
        return STATUS_NOT_SUPPORTED;

    key = PhAllocateZero(sizeof(PH_SYMCRYPT_MLDSA_KEY));
    key->Magic = PH_SYMCRYPT_MLDSA_KEY_MAGIC;
    key->Parameters = parameters;
    key->Key = SymCryptMlDsakeyAllocate(parameters);

    if (!key->Key)
    {
        PhFree(key);
        return STATUS_INSUFFICIENT_RESOURCES;
    }

    error = SymCryptMlDsakeySetValue((PCBYTE)KeyBlob, KeyBlobLength, format, 0, key->Key);
    if (error != SYMCRYPT_NO_ERROR)
    {
        SymCryptMlDsakeyFree(key->Key);
        PhFree(key);
        return PhSymCryptErrorToStatus(error);
    }

    *KeyHandle = key;
    return STATUS_SUCCESS;
}

NTSTATUS NTAPI PhSymCryptMlDsaExportKey(
    _In_ PH_SYMCRYPT_MLDSA_KEY_HANDLE KeyHandle,
    _In_ PH_SYMCRYPT_MLDSA_KEY_FORMAT Format,
    _Out_writes_bytes_to_opt_(KeyBlobCapacity, *KeyBlobLength) PVOID KeyBlob,
    _In_ SIZE_T KeyBlobCapacity,
    _Out_ PSIZE_T KeyBlobLength
    )
{
    PPH_SYMCRYPT_MLDSA_KEY key = PhpSymCryptGetMlDsaKey(KeyHandle);
    SYMCRYPT_MLDSAKEY_FORMAT format;
    SIZE_T required;
    SYMCRYPT_ERROR error;

    if (!key || !KeyBlobLength)
        return STATUS_INVALID_PARAMETER;

    if (!PhpSymCryptMlDsaMapFormat(Format, &format))
        return STATUS_NOT_SUPPORTED;

    error = SymCryptMlDsaSizeofKeyFormatFromParams(key->Parameters, format, &required);
    if (error != SYMCRYPT_NO_ERROR)
        return PhSymCryptErrorToStatus(error);

    *KeyBlobLength = required;

    if (!KeyBlob)
        return STATUS_SUCCESS;

    if (KeyBlobCapacity < required)
        return STATUS_BUFFER_TOO_SMALL;

    return PhSymCryptErrorToStatus(SymCryptMlDsakeyGetValue(key->Key, (PBYTE)KeyBlob, KeyBlobCapacity, format, 0));
}

NTSTATUS NTAPI PhSymCryptMlDsaSign(
    _In_ PH_SYMCRYPT_MLDSA_KEY_HANDLE KeyHandle,
    _In_reads_bytes_(MessageLength) PCVOID Message,
    _In_ SIZE_T MessageLength,
    _In_reads_bytes_opt_(ContextLength) PCVOID Context,
    _In_ SIZE_T ContextLength,
    _Out_writes_bytes_to_(SignatureCapacity, *SignatureLength) PVOID Signature,
    _In_ SIZE_T SignatureCapacity,
    _Out_ PSIZE_T SignatureLength
    )
{
    PPH_SYMCRYPT_MLDSA_KEY key = PhpSymCryptGetMlDsaKey(KeyHandle);
    SIZE_T required;
    SYMCRYPT_ERROR error;

    if (!key || !SignatureLength)
        return STATUS_INVALID_PARAMETER;

    if (ContextLength > SYMCRYPT_MLDSA_CONTEXT_MAX_LENGTH)
        return STATUS_INVALID_PARAMETER;

    error = SymCryptMlDsaSizeofSignatureFromParams(key->Parameters, &required);
    if (error != SYMCRYPT_NO_ERROR)
        return PhSymCryptErrorToStatus(error);

    *SignatureLength = required;

    if (SignatureCapacity < required)
        return STATUS_BUFFER_TOO_SMALL;

    return PhSymCryptErrorToStatus(SymCryptMlDsaSign(
        key->Key,
        (PCBYTE)Message,
        MessageLength,
        (PCBYTE)Context,
        ContextLength,
        0,
        (PBYTE)Signature,
        SignatureCapacity
        ));
}

NTSTATUS NTAPI PhSymCryptMlDsaVerify(
    _In_ PH_SYMCRYPT_MLDSA_KEY_HANDLE KeyHandle,
    _In_reads_bytes_(MessageLength) PCVOID Message,
    _In_ SIZE_T MessageLength,
    _In_reads_bytes_opt_(ContextLength) PCVOID Context,
    _In_ SIZE_T ContextLength,
    _In_reads_bytes_(SignatureLength) PCVOID Signature,
    _In_ SIZE_T SignatureLength
    )
{
    PPH_SYMCRYPT_MLDSA_KEY key = PhpSymCryptGetMlDsaKey(KeyHandle);

    if (!key)
        return STATUS_INVALID_PARAMETER;

    if (ContextLength > SYMCRYPT_MLDSA_CONTEXT_MAX_LENGTH)
        return STATUS_INVALID_PARAMETER;

    return PhSymCryptErrorToStatus(SymCryptMlDsaVerify(
        key->Key,
        (PCBYTE)Message,
        MessageLength,
        (PCBYTE)Context,
        ContextLength,
        (PCBYTE)Signature,
        SignatureLength,
        0
        ));
}

VOID NTAPI PhSymCryptMlDsaDestroyKey(
    _In_opt_ PH_SYMCRYPT_MLDSA_KEY_HANDLE KeyHandle
    )
{
    PPH_SYMCRYPT_MLDSA_KEY key = PhpSymCryptGetMlDsaKey(KeyHandle);

    if (!key)
        return;

    if (key->Key)
        SymCryptMlDsakeyFree(key->Key);

    key->Magic = 0;
    PhFree(key);
}
