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

#ifndef PHLIB_SYMCRYPT_H
#define PHLIB_SYMCRYPT_H

#pragma once

EXTERN_C_START

// ------------------------------------------------------------------------
// One-time SymCrypt library initialization
// ------------------------------------------------------------------------

VOID
NTAPI
PhSymCryptInitialize(
    VOID
    );

// ------------------------------------------------------------------------
// Hash result sizes
// ------------------------------------------------------------------------

#define PH_SYMCRYPT_MD5_RESULT_SIZE         16
#define PH_SYMCRYPT_SHA1_RESULT_SIZE        20
#define PH_SYMCRYPT_SHA256_RESULT_SIZE      32
#define PH_SYMCRYPT_SHA384_RESULT_SIZE      48
#define PH_SYMCRYPT_SHA512_RESULT_SIZE      64
#define PH_SYMCRYPT_SHA3_256_RESULT_SIZE    32
#define PH_SYMCRYPT_SHA3_384_RESULT_SIZE    48
#define PH_SYMCRYPT_SHA3_512_RESULT_SIZE    64

#define PH_SYMCRYPT_HMAC_SHA1_RESULT_SIZE       PH_SYMCRYPT_SHA1_RESULT_SIZE
#define PH_SYMCRYPT_HMAC_SHA256_RESULT_SIZE     PH_SYMCRYPT_SHA256_RESULT_SIZE
#define PH_SYMCRYPT_HMAC_SHA384_RESULT_SIZE     PH_SYMCRYPT_SHA384_RESULT_SIZE
#define PH_SYMCRYPT_HMAC_SHA512_RESULT_SIZE     PH_SYMCRYPT_SHA512_RESULT_SIZE

// ------------------------------------------------------------------------
// Algorithm/blob identifier strings for BCrypt-compatible call sites
// ------------------------------------------------------------------------

#define PH_SYMCRYPT_MD5_ALGORITHM_NAME          L"MD5"
#define PH_SYMCRYPT_SHA1_ALGORITHM_NAME         L"SHA1"
#define PH_SYMCRYPT_SHA256_ALGORITHM_NAME       L"SHA256"
#define PH_SYMCRYPT_SHA384_ALGORITHM_NAME       L"SHA384"
#define PH_SYMCRYPT_SHA512_ALGORITHM_NAME       L"SHA512"

#define PH_SYMCRYPT_ECDSA_P256_ALGORITHM_NAME   L"ECDSA_P256"
#define PH_SYMCRYPT_RSA_ALGORITHM_NAME          L"RSA"
#define PH_SYMCRYPT_ECCPUBLIC_BLOB_NAME         L"ECCPUBLICBLOB"
#define PH_SYMCRYPT_RSAPUBLIC_BLOB_NAME         L"RSAPUBLICBLOB"

#define PH_SYMCRYPT_PAD_PSS                     0x00000008ul

// ------------------------------------------------------------------------
// Hash algorithm selector (used by RSA-PKCS1 / RSA-PSS verify)
// ------------------------------------------------------------------------

typedef ULONG PH_SYMCRYPT_HASH_ALGORITHM, *PPH_SYMCRYPT_HASH_ALGORITHM;

#define PH_SYMCRYPT_MD5_ALGORITHM       0ul
#define PH_SYMCRYPT_SHA1_ALGORITHM      1ul
#define PH_SYMCRYPT_SHA256_ALGORITHM    2ul
#define PH_SYMCRYPT_SHA384_ALGORITHM    3ul
#define PH_SYMCRYPT_SHA512_ALGORITHM    4ul
#define PH_SYMCRYPT_SHA3_256_ALGORITHM  5ul
#define PH_SYMCRYPT_SHA3_384_ALGORITHM  6ul
#define PH_SYMCRYPT_SHA3_512_ALGORITHM  7ul

// ------------------------------------------------------------------------
// Generic incremental hash context (caller-owned, SymCrypt-private internals)
// ------------------------------------------------------------------------

#if defined(_WIN64) || defined(_AMD64_) || defined(_M_AMD64)
#define PH_SYMCRYPT_HASH_STATE_BUFFER_SIZE 240
#elif defined(_WIN32) || defined(_X86_) || defined(_M_IX86)
#define PH_SYMCRYPT_HASH_STATE_BUFFER_SIZE 224
#else
#define PH_SYMCRYPT_HASH_STATE_BUFFER_SIZE 240
#endif
#define PH_SYMCRYPT_HASH_STATE_BUFFER_ALIGNMENT 16

typedef struct DECLSPEC_ALIGN(PH_SYMCRYPT_HASH_STATE_BUFFER_ALIGNMENT) _PH_SYMCRYPT_HASH_CONTEXT
{
    PVOID Algorithm;
    ULONG ResultSize;
    ULONG StateSize;
    UCHAR State[PH_SYMCRYPT_HASH_STATE_BUFFER_SIZE];
} PH_SYMCRYPT_HASH_CONTEXT, *PPH_SYMCRYPT_HASH_CONTEXT;

EXTERN_C
NTSTATUS
NTAPI
PhSymCryptHashInit(
    _In_ PH_SYMCRYPT_HASH_ALGORITHM Algorithm,
    _Out_ PPH_SYMCRYPT_HASH_CONTEXT Context
    );

EXTERN_C
NTSTATUS
NTAPI
PhSymCryptHashData(
    _Inout_ PPH_SYMCRYPT_HASH_CONTEXT Context,
    _In_reads_bytes_(Length) PCVOID Buffer,
    _In_ SIZE_T Length
    );

EXTERN_C
NTSTATUS
NTAPI
PhSymCryptHashFinal(
    _Inout_ PPH_SYMCRYPT_HASH_CONTEXT Context,
    _Out_writes_bytes_(ResultLength) PVOID Result,
    _In_ ULONG ResultLength
    );

EXTERN_C
NTSTATUS
NTAPI
PhSymCryptGetHashSize(
    _In_ PH_SYMCRYPT_HASH_ALGORITHM Algorithm,
    _Out_ PULONG HashSize
    );

EXTERN_C
NTSTATUS
NTAPI
PhSymCryptHashSize(
    _In_ PPH_SYMCRYPT_HASH_CONTEXT Context,
    _Out_ PULONG HashSize
    );

EXTERN_C
VOID
NTAPI
PhSymCryptDestroyHash(
    _Inout_ PPH_SYMCRYPT_HASH_CONTEXT Context,
    _In_ ULONG HashSize
    );

EXTERN_C
NTSTATUS
NTAPI
PhSymCryptHashAlgorithmIdToAlgorithm(
    _In_ PCWSTR AlgorithmId,
    _Out_ PPH_SYMCRYPT_HASH_ALGORITHM Algorithm,
    _Out_opt_ PULONG HashSize
    );

// ------------------------------------------------------------------------
// Random / RNG
// ------------------------------------------------------------------------

EXTERN_C
VOID
NTAPI
PhSymCryptRandom(
    _Out_writes_bytes_(Length) PBYTE Buffer,
    _In_ SIZE_T Length
    );

EXTERN_C
VOID
NTAPI
PhSymCryptProvideEntropy(
    _In_reads_bytes_(Length) PCVOID Entropy,
    _In_ SIZE_T Length
    );

EXTERN_C
NTSTATUS
NTAPI
PhSymCryptRdrandStatus(
    VOID
    );

EXTERN_C
NTSTATUS
NTAPI
PhSymCryptRdrandGetBytes(
    _Out_writes_bytes_(Length) PVOID Buffer,
    _In_ SIZE_T Length
    );

// ------------------------------------------------------------------------
// Constant-time / wipe utilities
// ------------------------------------------------------------------------

EXTERN_C
BOOLEAN
NTAPI
PhSymCryptEqual(
    _In_reads_bytes_(Length) PCVOID Buffer1,
    _In_reads_bytes_(Length) PCVOID Buffer2,
    _In_ SIZE_T Length
    );

EXTERN_C
VOID
NTAPI
PhSymCryptWipe(
    _Out_writes_bytes_(Length) PVOID Buffer,
    _In_ SIZE_T Length
    );

EXTERN_C
VOID
NTAPI
PhSymCryptXorBytes(
    _In_reads_bytes_(Length) PCVOID Source1,
    _In_reads_bytes_(Length) PCVOID Source2,
    _Out_writes_bytes_(Length) PVOID Destination,
    _In_ SIZE_T Length
    );

// ------------------------------------------------------------------------
// Single-shot hashes
// ------------------------------------------------------------------------

EXTERN_C
VOID
NTAPI
PhSymCryptMd5(
    _In_reads_bytes_(Length) PCVOID Buffer,
    _In_ SIZE_T Length,
    _Out_writes_bytes_(PH_SYMCRYPT_MD5_RESULT_SIZE) PVOID Result
    );

EXTERN_C
VOID
NTAPI
PhSymCryptSha1(
    _In_reads_bytes_(Length) PCVOID Buffer,
    _In_ SIZE_T Length,
    _Out_writes_bytes_(PH_SYMCRYPT_SHA1_RESULT_SIZE) PVOID Result
    );

EXTERN_C
VOID
NTAPI
PhSymCryptSha256(
    _In_reads_bytes_(Length) PCVOID Buffer,
    _In_ SIZE_T Length,
    _Out_writes_bytes_(PH_SYMCRYPT_SHA256_RESULT_SIZE) PVOID Result
    );

EXTERN_C
VOID
NTAPI
PhSymCryptSha384(
    _In_reads_bytes_(Length) PCVOID Buffer,
    _In_ SIZE_T Length,
    _Out_writes_bytes_(PH_SYMCRYPT_SHA384_RESULT_SIZE) PVOID Result
    );

EXTERN_C
VOID
NTAPI
PhSymCryptSha512(
    _In_reads_bytes_(Length) PCVOID Buffer,
    _In_ SIZE_T Length,
    _Out_writes_bytes_(PH_SYMCRYPT_SHA512_RESULT_SIZE) PVOID Result
    );

EXTERN_C
VOID
NTAPI
PhSymCryptSha3_256(
    _In_reads_bytes_(Length) PCVOID Buffer,
    _In_ SIZE_T Length,
    _Out_writes_bytes_(PH_SYMCRYPT_SHA3_256_RESULT_SIZE) PVOID Result
    );

EXTERN_C
VOID
NTAPI
PhSymCryptSha3_384(
    _In_reads_bytes_(Length) PCVOID Buffer,
    _In_ SIZE_T Length,
    _Out_writes_bytes_(PH_SYMCRYPT_SHA3_384_RESULT_SIZE) PVOID Result
    );

EXTERN_C
VOID
NTAPI
PhSymCryptSha3_512(
    _In_reads_bytes_(Length) PCVOID Buffer,
    _In_ SIZE_T Length,
    _Out_writes_bytes_(PH_SYMCRYPT_SHA3_512_RESULT_SIZE) PVOID Result
    );

// ------------------------------------------------------------------------
// Parallel multi-hash API
//
// Batches multiple independent hashes in parallel using SymCrypt's native
// parallel implementations (SHA-256/384/512 up to 8-way parallel).
//
// Pattern:
//   PH_SYMCRYPT_PARALLEL_HASH_CONTEXT ctx = {0};
//   PhSymCryptInitializeParallelHash(&ctx, PH_SYMCRYPT_PARALLEL_HASH_SHA256, 4);
//
//   // Build operation array with arbitrary interleaving
//   PH_SYMCRYPT_PARALLEL_HASH_OPERATION ops[8] = {...};
//   PhSymCryptProcessParallelHashOperations(&ctx, ops, 8);
//
//   PhSymCryptCleanupParallelHash(&ctx);
//
// Exposed structure allows stack allocation. Results are extracted via
// HASH_OPERATION_RESULT operations.
//
// Parallelism limits:
//   x86/x64: 2-8 parallel hashes
//   ARM:     3-4 parallel hashes
// ------------------------------------------------------------------------

// Wrapper macros for SymCrypt parallelism constants
// x86/x64: 2-8 parallel hashes; ARM: 3-4 parallel hashes
#if defined(_ARM_) || defined(_ARM64_) || defined(_ARM64EC_)
#define PH_SYMCRYPT_PARALLEL_SHA256_MIN_PARALLELISM  (3)
#define PH_SYMCRYPT_PARALLEL_SHA256_MAX_PARALLELISM  (4)
#else
#define PH_SYMCRYPT_PARALLEL_SHA256_MIN_PARALLELISM  (2)
#define PH_SYMCRYPT_PARALLEL_SHA256_MAX_PARALLELISM  (8)
#endif
// SHA384 and SHA512 use same limits as SHA256
#define PH_SYMCRYPT_PARALLEL_SHA384_MIN_PARALLELISM  PH_SYMCRYPT_PARALLEL_SHA256_MIN_PARALLELISM
#define PH_SYMCRYPT_PARALLEL_SHA384_MAX_PARALLELISM  PH_SYMCRYPT_PARALLEL_SHA256_MAX_PARALLELISM
#define PH_SYMCRYPT_PARALLEL_SHA512_MIN_PARALLELISM  PH_SYMCRYPT_PARALLEL_SHA256_MIN_PARALLELISM
#define PH_SYMCRYPT_PARALLEL_SHA512_MAX_PARALLELISM  PH_SYMCRYPT_PARALLEL_SHA256_MAX_PARALLELISM

typedef enum _PH_SYMCRYPT_PARALLEL_HASH_ALGORITHM
{
    PH_SYMCRYPT_PARALLEL_HASH_SHA256 = 1,
    PH_SYMCRYPT_PARALLEL_HASH_SHA384 = 2,
    PH_SYMCRYPT_PARALLEL_HASH_SHA512 = 3,
} PH_SYMCRYPT_PARALLEL_HASH_ALGORITHM;

typedef enum _PH_SYMCRYPT_PARALLEL_HASH_OPERATION_TYPE
{
    PH_SYMCRYPT_HASH_OPERATION_APPEND = 1,
    PH_SYMCRYPT_HASH_OPERATION_RESULT = 2,
} PH_SYMCRYPT_PARALLEL_HASH_OPERATION_TYPE;

typedef struct _PH_SYMCRYPT_PARALLEL_HASH_OPERATION
{
    SIZE_T iHash;
    PH_SYMCRYPT_PARALLEL_HASH_OPERATION_TYPE hashOperation;
    _In_reads_bytes_(cbBuffer) PBYTE pbBuffer;
    SIZE_T cbBuffer;
} PH_SYMCRYPT_PARALLEL_HASH_OPERATION, *PPH_SYMCRYPT_PARALLEL_HASH_OPERATION;

typedef struct _PH_SYMCRYPT_PARALLEL_HASH_CONTEXT
{
    PH_SYMCRYPT_PARALLEL_HASH_ALGORITHM Algorithm;
    SIZE_T NumberOfHashes;
    SIZE_T ResultSize;
    PVOID pHashStates;
    PBYTE pbScratchBuffer;
    SIZE_T cbScratchBuffer;
} PH_SYMCRYPT_PARALLEL_HASH_CONTEXT, *PPH_SYMCRYPT_PARALLEL_HASH_CONTEXT;

EXTERN_C
NTSTATUS
NTAPI
PhSymCryptInitializeParallelHash(
    _Out_ PPH_SYMCRYPT_PARALLEL_HASH_CONTEXT ParallelHashContext,
    _In_ PH_SYMCRYPT_PARALLEL_HASH_ALGORITHM Algorithm,
    _In_ SIZE_T NumberOfHashes
    );

EXTERN_C
NTSTATUS
NTAPI
PhSymCryptProcessParallelHashOperations(
    _In_ PPH_SYMCRYPT_PARALLEL_HASH_CONTEXT ParallelHashContext,
    _Inout_updates_(OperationCount) PPH_SYMCRYPT_PARALLEL_HASH_OPERATION Operations,
    _In_ SIZE_T OperationCount
    );

EXTERN_C
VOID
NTAPI
PhSymCryptCleanupParallelHash(
    _Inout_ PPH_SYMCRYPT_PARALLEL_HASH_CONTEXT ParallelHashContext
    );

EXTERN_C
VOID
NTAPI
PhSymCryptGetParallelHashCapabilities(
    _In_ PH_SYMCRYPT_PARALLEL_HASH_ALGORITHM Algorithm,
    _Out_opt_ PSIZE_T MinimumParallelism,
    _Out_opt_ PSIZE_T MaximumParallelism
    );

// Incremental hashes (opaque heap-allocated context)
//
// Pattern:
//   PVOID ctx;
//   PhSymCryptSha256Init(&ctx);
//   PhSymCryptSha256Append(ctx, data, len);   // may be called repeatedly
//   PhSymCryptSha256Result(ctx, digest);      // frees ctx
//
// Result always frees the context, even on early abandonment. To discard
// an incremental hash without computing the result, call the matching
// Result function with a throwaway buffer.
//

EXTERN_C
NTSTATUS
NTAPI
PhSymCryptMd5Init(
    _Out_ PVOID* Context
    );

EXTERN_C
VOID
NTAPI
PhSymCryptMd5Append(
    _Inout_ PVOID Context,
    _In_reads_bytes_(Length) PCVOID Buffer,
    _In_ SIZE_T Length
    );

EXTERN_C
VOID
NTAPI
PhSymCryptMd5Result(
    _Inout_ PVOID Context,
    _Out_writes_bytes_(PH_SYMCRYPT_MD5_RESULT_SIZE) PVOID Result
    );

EXTERN_C
NTSTATUS
NTAPI
PhSymCryptSha1Init(
    _Out_ PVOID* Context
    );

EXTERN_C
VOID
NTAPI
PhSymCryptSha1Append(
    _Inout_ PVOID Context,
    _In_reads_bytes_(Length) PCVOID Buffer,
    _In_ SIZE_T Length
    );

EXTERN_C
VOID
NTAPI
PhSymCryptSha1Result(
    _Inout_ PVOID Context,
    _Out_writes_bytes_(PH_SYMCRYPT_SHA1_RESULT_SIZE) PVOID Result
    );

EXTERN_C
NTSTATUS
NTAPI
PhSymCryptSha256Init(
    _Out_ PVOID* Context
    );

EXTERN_C
VOID
NTAPI
PhSymCryptSha256Append(
    _Inout_ PVOID Context,
    _In_reads_bytes_(Length) PCVOID Buffer,
    _In_ SIZE_T Length
    );

EXTERN_C
VOID
NTAPI
PhSymCryptSha256Result(
    _Inout_ PVOID Context,
    _Out_writes_bytes_(PH_SYMCRYPT_SHA256_RESULT_SIZE) PVOID Result
    );

EXTERN_C
NTSTATUS
NTAPI
PhSymCryptSha384Init(
    _Out_ PVOID* Context
    );

EXTERN_C
VOID
NTAPI
PhSymCryptSha384Append(
    _Inout_ PVOID Context,
    _In_reads_bytes_(Length) PCVOID Buffer,
    _In_ SIZE_T Length
    );

EXTERN_C
VOID
NTAPI
PhSymCryptSha384Result(
    _Inout_ PVOID Context,
    _Out_writes_bytes_(PH_SYMCRYPT_SHA384_RESULT_SIZE) PVOID Result
    );

EXTERN_C
NTSTATUS
NTAPI
PhSymCryptSha512Init(
    _Out_ PVOID* Context
    );

EXTERN_C
VOID
NTAPI
PhSymCryptSha512Append(
    _Inout_ PVOID Context,
    _In_reads_bytes_(Length) PCVOID Buffer,
    _In_ SIZE_T Length
    );

EXTERN_C
VOID
NTAPI
PhSymCryptSha512Result(
    _Inout_ PVOID Context,
    _Out_writes_bytes_(PH_SYMCRYPT_SHA512_RESULT_SIZE) PVOID Result
    );

EXTERN_C
NTSTATUS
NTAPI
PhSymCryptSha3_256Init(
    _Out_ PVOID* Context
    );

EXTERN_C
VOID
NTAPI
PhSymCryptSha3_256Append(
    _Inout_ PVOID Context,
    _In_reads_bytes_(Length) PCVOID Buffer,
    _In_ SIZE_T Length
    );

EXTERN_C
VOID
NTAPI
PhSymCryptSha3_256Result(
    _Inout_ PVOID Context,
    _Out_writes_bytes_(PH_SYMCRYPT_SHA3_256_RESULT_SIZE) PVOID Result
    );

EXTERN_C
NTSTATUS
NTAPI
PhSymCryptSha3_384Init(
    _Out_ PVOID* Context
    );

EXTERN_C
VOID
NTAPI
PhSymCryptSha3_384Append(
    _Inout_ PVOID Context,
    _In_reads_bytes_(Length) PCVOID Buffer,
    _In_ SIZE_T Length
    );

EXTERN_C
VOID
NTAPI
PhSymCryptSha3_384Result(
    _Inout_ PVOID Context,
    _Out_writes_bytes_(PH_SYMCRYPT_SHA3_384_RESULT_SIZE) PVOID Result
    );

EXTERN_C
NTSTATUS
NTAPI
PhSymCryptSha3_512Init(
    _Out_ PVOID* Context
    );

EXTERN_C
VOID
NTAPI
PhSymCryptSha3_512Append(
    _Inout_ PVOID Context,
    _In_reads_bytes_(Length) PCVOID Buffer,
    _In_ SIZE_T Length
    );

EXTERN_C
VOID
NTAPI
PhSymCryptSha3_512Result(
    _Inout_ PVOID Context,
    _Out_writes_bytes_(PH_SYMCRYPT_SHA3_512_RESULT_SIZE) PVOID Result
    );

// ------------------------------------------------------------------------
// BCrypt-style incremental hash facade
//
// Selects the underlying SymCrypt algorithm by BCRYPT_*_ALGORITHM string in
// PhSymCryptOpenAlgorithmProvider, returns the hash output size and
// initializes the caller-provided context. Subsequent PhSymCryptHashDataBySize /
// PhSymCryptFinishHash calls dispatch on the same HashSize value the open
// call returned. Mirrors BCryptCreateHash / BCryptHashData / BCryptFinishHash
// without requiring per-algorithm switches at the call site.
// ------------------------------------------------------------------------

EXTERN_C
NTSTATUS
NTAPI
PhSymCryptOpenAlgorithmProvider(
    _Out_ PPH_SYMCRYPT_HASH_CONTEXT Context,
    _Out_ PULONG HashSize,
    _In_ PCWSTR AlgorithmId
    );

EXTERN_C
NTSTATUS
NTAPI
PhSymCryptHashDataBySize(
    _Inout_ PPH_SYMCRYPT_HASH_CONTEXT Context,
    _In_ ULONG HashSize,
    _In_reads_bytes_(Length) PCVOID Buffer,
    _In_ SIZE_T Length
    );

EXTERN_C
NTSTATUS
NTAPI
PhSymCryptFinishHash(
    _Inout_ PPH_SYMCRYPT_HASH_CONTEXT Context,
    _In_ ULONG HashSize,
    _Out_writes_bytes_(HashSize) PVOID Result
    );

// ------------------------------------------------------------------------
// HMAC (single-shot)
// ------------------------------------------------------------------------

EXTERN_C
VOID
NTAPI
PhSymCryptHmacSha1(
    _In_reads_bytes_(KeyLength) PCVOID Key,
    _In_ SIZE_T KeyLength,
    _In_reads_bytes_(DataLength) PCVOID Data,
    _In_ SIZE_T DataLength,
    _Out_writes_bytes_(PH_SYMCRYPT_HMAC_SHA1_RESULT_SIZE) PVOID Result
    );

EXTERN_C
VOID
NTAPI
PhSymCryptHmacSha256(
    _In_reads_bytes_(KeyLength) PCVOID Key,
    _In_ SIZE_T KeyLength,
    _In_reads_bytes_(DataLength) PCVOID Data,
    _In_ SIZE_T DataLength,
    _Out_writes_bytes_(PH_SYMCRYPT_HMAC_SHA256_RESULT_SIZE) PVOID Result
    );

EXTERN_C
VOID
NTAPI
PhSymCryptHmacSha384(
    _In_reads_bytes_(KeyLength) PCVOID Key,
    _In_ SIZE_T KeyLength,
    _In_reads_bytes_(DataLength) PCVOID Data,
    _In_ SIZE_T DataLength,
    _Out_writes_bytes_(PH_SYMCRYPT_HMAC_SHA384_RESULT_SIZE) PVOID Result
    );

EXTERN_C
VOID
NTAPI
PhSymCryptHmacSha512(
    _In_reads_bytes_(KeyLength) PCVOID Key,
    _In_ SIZE_T KeyLength,
    _In_reads_bytes_(DataLength) PCVOID Data,
    _In_ SIZE_T DataLength,
    _Out_writes_bytes_(PH_SYMCRYPT_HMAC_SHA512_RESULT_SIZE) PVOID Result
    );

// ------------------------------------------------------------------------
// KDFs — return NTSTATUS, mapped from SYMCRYPT_ERROR
// ------------------------------------------------------------------------

EXTERN_C
NTSTATUS
NTAPI
PhSymCryptPbkdf2HmacSha256(
    _In_reads_bytes_(PasswordLength) PCVOID Password,
    _In_ SIZE_T PasswordLength,
    _In_reads_bytes_opt_(SaltLength) PCVOID Salt,
    _In_ SIZE_T SaltLength,
    _In_ ULONG64 IterationCount,
    _Out_writes_bytes_(ResultLength) PVOID Result,
    _In_ SIZE_T ResultLength
    );

EXTERN_C
NTSTATUS
NTAPI
PhSymCryptPbkdf2HmacSha512(
    _In_reads_bytes_(PasswordLength) PCVOID Password,
    _In_ SIZE_T PasswordLength,
    _In_reads_bytes_opt_(SaltLength) PCVOID Salt,
    _In_ SIZE_T SaltLength,
    _In_ ULONG64 IterationCount,
    _Out_writes_bytes_(ResultLength) PVOID Result,
    _In_ SIZE_T ResultLength
    );

EXTERN_C
NTSTATUS
NTAPI
PhSymCryptHkdfSha256(
    _In_reads_bytes_(IkmLength) PCVOID Ikm,
    _In_ SIZE_T IkmLength,
    _In_reads_bytes_opt_(SaltLength) PCVOID Salt,
    _In_ SIZE_T SaltLength,
    _In_reads_bytes_opt_(InfoLength) PCVOID Info,
    _In_ SIZE_T InfoLength,
    _Out_writes_bytes_(ResultLength) PVOID Result,
    _In_ SIZE_T ResultLength
    );

EXTERN_C
NTSTATUS
NTAPI
PhSymCryptHkdfSha512(
    _In_reads_bytes_(IkmLength) PCVOID Ikm,
    _In_ SIZE_T IkmLength,
    _In_reads_bytes_opt_(SaltLength) PCVOID Salt,
    _In_ SIZE_T SaltLength,
    _In_reads_bytes_opt_(InfoLength) PCVOID Info,
    _In_ SIZE_T InfoLength,
    _Out_writes_bytes_(ResultLength) PVOID Result,
    _In_ SIZE_T ResultLength
    );

// ------------------------------------------------------------------------
// AEAD
//
// AES-GCM: KeyLength must be 16/24/32, NonceLength typically 12, TagLength 12-16.
// ChaCha20-Poly1305: Key must be 32 bytes, Nonce 12 bytes, Tag 16 bytes.
// Decrypt returns STATUS_AUTH_TAG_MISMATCH on tag failure.
// ------------------------------------------------------------------------

EXTERN_C
NTSTATUS
NTAPI
PhSymCryptAesGcmEncrypt(
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
    );

EXTERN_C
NTSTATUS
NTAPI
PhSymCryptAesGcmDecrypt(
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
    );

EXTERN_C
NTSTATUS
NTAPI
PhSymCryptChaCha20Poly1305Encrypt(
    _In_reads_bytes_(32) PCVOID Key,
    _In_reads_bytes_(12) PCVOID Nonce,
    _In_reads_bytes_opt_(AuthDataLength) PCVOID AuthData,
    _In_ SIZE_T AuthDataLength,
    _In_reads_bytes_(DataLength) PCVOID Plaintext,
    _Out_writes_bytes_(DataLength) PVOID Ciphertext,
    _In_ SIZE_T DataLength,
    _Out_writes_bytes_(16) PVOID Tag
    );

EXTERN_C
NTSTATUS
NTAPI
PhSymCryptChaCha20Poly1305Decrypt(
    _In_reads_bytes_(32) PCVOID Key,
    _In_reads_bytes_(12) PCVOID Nonce,
    _In_reads_bytes_opt_(AuthDataLength) PCVOID AuthData,
    _In_ SIZE_T AuthDataLength,
    _In_reads_bytes_(DataLength) PCVOID Ciphertext,
    _Out_writes_bytes_(DataLength) PVOID Plaintext,
    _In_ SIZE_T DataLength,
    _In_reads_bytes_(16) PCVOID Tag
    );

// ------------------------------------------------------------------------
// AES-CBC
//
// KeyLength: 16/24/32. IV: 16 bytes. Raw variants require DataLength to be
// a multiple of 16. PKCS#7 variants apply / strip padding; the encrypt
// output buffer must hold ((PlaintextLength / 16) + 1) * 16 bytes, and on
// decrypt the actual plaintext length is returned via *PlaintextLength.
// ------------------------------------------------------------------------

EXTERN_C
NTSTATUS
NTAPI
PhSymCryptAesCbcEncrypt(
    _In_reads_bytes_(KeyLength) PCVOID Key,
    _In_ SIZE_T KeyLength,
    _In_reads_bytes_(16) PCVOID Iv,
    _In_reads_bytes_(DataLength) PCVOID Plaintext,
    _Out_writes_bytes_(DataLength) PVOID Ciphertext,
    _In_ SIZE_T DataLength
    );

EXTERN_C
NTSTATUS
NTAPI
PhSymCryptAesCbcDecrypt(
    _In_reads_bytes_(KeyLength) PCVOID Key,
    _In_ SIZE_T KeyLength,
    _In_reads_bytes_(16) PCVOID Iv,
    _In_reads_bytes_(DataLength) PCVOID Ciphertext,
    _Out_writes_bytes_(DataLength) PVOID Plaintext,
    _In_ SIZE_T DataLength
    );

EXTERN_C
NTSTATUS
NTAPI
PhSymCryptAesCbcEncryptPkcs7(
    _In_reads_bytes_(KeyLength) PCVOID Key,
    _In_ SIZE_T KeyLength,
    _In_reads_bytes_(16) PCVOID Iv,
    _In_reads_bytes_(PlaintextLength) PCVOID Plaintext,
    _In_ SIZE_T PlaintextLength,
    _Out_writes_bytes_to_(CiphertextCapacity, *CiphertextLength) PVOID Ciphertext,
    _In_ SIZE_T CiphertextCapacity,
    _Out_ PSIZE_T CiphertextLength
    );

EXTERN_C
NTSTATUS
NTAPI
PhSymCryptAesCbcDecryptPkcs7(
    _In_reads_bytes_(KeyLength) PCVOID Key,
    _In_ SIZE_T KeyLength,
    _In_reads_bytes_(16) PCVOID Iv,
    _In_reads_bytes_(CiphertextLength) PCVOID Ciphertext,
    _In_ SIZE_T CiphertextLength,
    _Out_writes_bytes_to_(CiphertextLength, *PlaintextLength) PVOID Plaintext,
    _Out_ PSIZE_T PlaintextLength
    );

// ------------------------------------------------------------------------
// Asymmetric verification (raw big-endian buffers)
//
// Key material is raw MSB-first integer bytes, not BCrypt key blobs.
// Callers holding BCrypt blobs need to strip the header / split fields
// before invoking these.
// ------------------------------------------------------------------------

EXTERN_C
NTSTATUS
NTAPI
PhSymCryptRsaPkcs1Verify(
    _In_reads_bytes_(ModulusLength) PCVOID Modulus,
    _In_ SIZE_T ModulusLength,
    _In_ ULONG64 PublicExponent,
    _In_ PH_SYMCRYPT_HASH_ALGORITHM HashAlgorithm,
    _In_reads_bytes_(HashLength) PCVOID Hash,
    _In_ SIZE_T HashLength,
    _In_reads_bytes_(SignatureLength) PCVOID Signature,
    _In_ SIZE_T SignatureLength
    );

EXTERN_C
NTSTATUS
NTAPI
PhSymCryptRsaPssVerify(
    _In_reads_bytes_(ModulusLength) PCVOID Modulus,
    _In_ SIZE_T ModulusLength,
    _In_ ULONG64 PublicExponent,
    _In_ PH_SYMCRYPT_HASH_ALGORITHM HashAlgorithm,
    _In_ SIZE_T SaltLength,
    _In_reads_bytes_(HashLength) PCVOID Hash,
    _In_ SIZE_T HashLength,
    _In_reads_bytes_(SignatureLength) PCVOID Signature,
    _In_ SIZE_T SignatureLength
    );

// PublicKeyXY: uncompressed concatenation of X || Y in MSB-first order.
//   P-256 -> 64 bytes (32+32), P-384 -> 96 bytes (48+48).
// Signature: concatenation of R || S in MSB-first order.
//   P-256 -> 64 bytes, P-384 -> 96 bytes.

EXTERN_C
NTSTATUS
NTAPI
PhSymCryptEcDsaVerifyP256(
    _In_reads_bytes_(PublicKeyLength) PCVOID PublicKeyXY,
    _In_ SIZE_T PublicKeyLength,
    _In_reads_bytes_(HashLength) PCVOID Hash,
    _In_ SIZE_T HashLength,
    _In_reads_bytes_(SignatureLength) PCVOID Signature,
    _In_ SIZE_T SignatureLength
    );

EXTERN_C
NTSTATUS
NTAPI
PhSymCryptEcDsaVerifyP384(
    _In_reads_bytes_(PublicKeyLength) PCVOID PublicKeyXY,
    _In_ SIZE_T PublicKeyLength,
    _In_reads_bytes_(HashLength) PCVOID Hash,
    _In_ SIZE_T HashLength,
    _In_reads_bytes_(SignatureLength) PCVOID Signature,
    _In_ SIZE_T SignatureLength
    );

// ------------------------------------------------------------------------
// Asymmetric verification (BCrypt blob compatibility)
//
// Accept raw BCRYPT_*_BLOB buffers as produced by BCryptExportKey or as
// embedded verbatim in source. Header is parsed in-place; no allocations.
// ------------------------------------------------------------------------

// KeyBlob: BCRYPT_ECCKEY_BLOB header followed by X||Y (MSB-first).
// Magic selects the curve: ECS1/ECK1 -> P-256, ECS3/ECK3 -> P-384.
// Signature: R||S (MSB-first), same shape BCryptSignHash produces.
EXTERN_C
NTSTATUS
NTAPI
PhSymCryptEcDsaVerifyBlob(
    _In_reads_bytes_(KeyBlobLength) PCVOID KeyBlob,
    _In_ SIZE_T KeyBlobLength,
    _In_reads_bytes_(HashLength) PCVOID Hash,
    _In_ SIZE_T HashLength,
    _In_reads_bytes_(SignatureLength) PCVOID Signature,
    _In_ SIZE_T SignatureLength
    );

// KeyBlob: BCRYPT_RSAKEY_BLOB header (Magic = RSA1) followed by
// public exponent then modulus, both MSB-first.
// PaddingFlags: BCRYPT_PAD_PSS or BCRYPT_PAD_PKCS1.
// SaltLength is honored only for PSS.
EXTERN_C
NTSTATUS
NTAPI
PhSymCryptRsaVerifyBlob(
    _In_reads_bytes_(KeyBlobLength) PCVOID KeyBlob,
    _In_ SIZE_T KeyBlobLength,
    _In_ PH_SYMCRYPT_HASH_ALGORITHM HashAlgorithm,
    _In_ ULONG PaddingFlags,
    _In_ SIZE_T SaltLength,
    _In_reads_bytes_(HashLength) PCVOID Hash,
    _In_ SIZE_T HashLength,
    _In_reads_bytes_(SignatureLength) PCVOID Signature,
    _In_ SIZE_T SignatureLength
    );

// Drop-in replacement for BCryptVerifySignature.
// BlobType: BCRYPT_ECCPUBLIC_BLOB or BCRYPT_RSAPUBLIC_BLOB.
// PaddingInfo: BCRYPT_PSS_PADDING_INFO* for PSS, BCRYPT_PKCS1_PADDING_INFO*
// for PKCS1, NULL/ignored for ECDSA.
// PaddingFlags: 0 (ECDSA), BCRYPT_PAD_PSS, or BCRYPT_PAD_PKCS1.
EXTERN_C
NTSTATUS
NTAPI
PhSymCryptVerifySignature(
    _In_ PCWSTR BlobType,
    _In_reads_bytes_(KeyBlobLength) PCVOID KeyBlob,
    _In_ SIZE_T KeyBlobLength,
    _In_opt_ PVOID PaddingInfo,
    _In_ ULONG PaddingFlags,
    _In_reads_bytes_(HashLength) PCVOID Hash,
    _In_ SIZE_T HashLength,
    _In_reads_bytes_(SignatureLength) PCVOID Signature,
    _In_ SIZE_T SignatureLength
    );

// ------------------------------------------------------------------------
// Asymmetric signing (BCrypt blob input)
//
// KeyBlob input shapes:
//   RSA  -> BCRYPT_RSAPRIVATE_BLOB or BCRYPT_RSAFULLPRIVATE_BLOB
//   ECC  -> BCRYPT_ECCPRIVATE_BLOB
// SignatureCapacity must be >= modulus size (RSA); ECDSA P-256 always
// produces a 64-byte signature (R || S, MSB-first).
// ------------------------------------------------------------------------

EXTERN_C
NTSTATUS
NTAPI
PhSymCryptRsaPkcs1SignBlob(
    _In_reads_bytes_(KeyBlobLength) PCVOID KeyBlob,
    _In_ SIZE_T KeyBlobLength,
    _In_ PH_SYMCRYPT_HASH_ALGORITHM HashAlgorithm,
    _In_reads_bytes_(HashLength) PCVOID Hash,
    _In_ SIZE_T HashLength,
    _Out_writes_bytes_to_(SignatureCapacity, *SignatureLength) PVOID Signature,
    _In_ SIZE_T SignatureCapacity,
    _Out_ PSIZE_T SignatureLength
    );

EXTERN_C
NTSTATUS
NTAPI
PhSymCryptRsaPssSignBlob(
    _In_reads_bytes_(KeyBlobLength) PCVOID KeyBlob,
    _In_ SIZE_T KeyBlobLength,
    _In_ PH_SYMCRYPT_HASH_ALGORITHM HashAlgorithm,
    _In_ SIZE_T SaltLength,
    _In_reads_bytes_(HashLength) PCVOID Hash,
    _In_ SIZE_T HashLength,
    _Out_writes_bytes_to_(SignatureCapacity, *SignatureLength) PVOID Signature,
    _In_ SIZE_T SignatureCapacity,
    _Out_ PSIZE_T SignatureLength
    );

EXTERN_C
NTSTATUS
NTAPI
PhSymCryptEcDsaSignP256Blob(
    _In_reads_bytes_(KeyBlobLength) PCVOID KeyBlob,
    _In_ SIZE_T KeyBlobLength,
    _In_reads_bytes_(HashLength) PCVOID Hash,
    _In_ SIZE_T HashLength,
    _Out_writes_bytes_(64) PVOID Signature
    );

// ------------------------------------------------------------------------
// Handle-based Asymmetric Key Abstraction (BCrypt Emulation)
// ------------------------------------------------------------------------

typedef PVOID PH_SYMCRYPT_KEY_HANDLE, *PPH_SYMCRYPT_KEY_HANDLE;

EXTERN_C
NTSTATUS
NTAPI
PhSymCryptGenerateKeyPair(
    _In_ PWSTR Algorithm,
    _Out_ PPH_SYMCRYPT_KEY_HANDLE KeyHandle,
    _In_ ULONG Length
    );

EXTERN_C
NTSTATUS
NTAPI
PhSymCryptFinalizeKeyPair(
    _In_ PH_SYMCRYPT_KEY_HANDLE KeyHandle
    );

EXTERN_C
NTSTATUS
NTAPI
PhSymCryptExportKey(
    _In_ PH_SYMCRYPT_KEY_HANDLE KeyHandle,
    _In_ PWSTR BlobType,
    _Out_writes_bytes_to_opt_(BlobLength, *ResultLength) PVOID Blob,
    _In_ ULONG BlobLength,
    _Out_ PULONG ResultLength
    );

EXTERN_C
NTSTATUS
NTAPI
PhSymCryptImportKeyPair(
    _In_ PWSTR Algorithm,
    _Out_ PPH_SYMCRYPT_KEY_HANDLE KeyHandle,
    _In_ PWSTR BlobType,
    _In_reads_bytes_(BlobLength) PVOID Blob,
    _In_ ULONG BlobLength
    );

EXTERN_C
NTSTATUS
NTAPI
PhSymCryptSignHash(
    _In_ PH_SYMCRYPT_KEY_HANDLE KeyHandle,
    _In_opt_ PVOID PaddingInfo,
    _In_reads_bytes_(HashLength) PVOID Hash,
    _In_ ULONG HashLength,
    _Out_writes_bytes_to_opt_(SignatureLength, *ResultLength) PVOID Signature,
    _In_ ULONG SignatureLength,
    _Out_ PULONG ResultLength,
    _In_ ULONG Flags
    );

EXTERN_C
NTSTATUS
NTAPI
PhSymCryptVerifyHash(
    _In_ PH_SYMCRYPT_KEY_HANDLE KeyHandle,
    _In_opt_ PVOID PaddingInfo,
    _In_reads_bytes_(HashLength) PVOID Hash,
    _In_ ULONG HashLength,
    _In_reads_bytes_(SignatureLength) PVOID Signature,
    _In_ ULONG SignatureLength,
    _In_ ULONG Flags
    );

EXTERN_C
NTSTATUS
NTAPI
PhSymCryptDestroyKey(
    _In_ PH_SYMCRYPT_KEY_HANDLE KeyHandle
    );

// ------------------------------------------------------------------------
// Asymmetric key generation (producing BCrypt blobs)
// ------------------------------------------------------------------------

EXTERN_C
NTSTATUS
NTAPI
PhSymCryptGenerateRsaKeyBlobs(
    _In_ ULONG Bits,
    _Out_writes_bytes_to_opt_(PrivateKeyBlobCapacity, *PrivateKeyBlobLength) PVOID PrivateKeyBlob,
    _In_ SIZE_T PrivateKeyBlobCapacity,
    _Out_opt_ PSIZE_T PrivateKeyBlobLength,
    _Out_writes_bytes_to_opt_(PublicKeyBlobCapacity, *PublicKeyBlobLength) PVOID PublicKeyBlob,
    _In_ SIZE_T PublicKeyBlobCapacity,
    _Out_opt_ PSIZE_T PublicKeyBlobLength
    );

EXTERN_C_END

#endif // PHLIB_SYMCRYPT_H
