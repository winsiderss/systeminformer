/*
 * Copyright (c) 2022 Winsider Seminars & Solutions, Inc.  All rights reserved.
 *
 * This file is part of System Informer.
 *
 * Authors:
 *
 *     jxy-s   2022-2025
 *
 */

#include <kph.h>

#include <trace.h>

#define KPH_HASHING_BUFFER_SIZE (16 * 1024)

#define KPH_HASH_EACACHE_MD5                 KPH_KERNEL_PURGE_EA "MD5"
#define KPH_HASH_EACACHE_SHA1                KPH_KERNEL_PURGE_EA "SHA1"
#define KPH_HASH_EACACHE_SHA1_AUTHENTICODE   KPH_KERNEL_PURGE_EA "SHA1A"
#define KPH_HASH_EACACHE_SHA256              KPH_KERNEL_PURGE_EA "SHA256"
#define KPH_HASH_EACACHE_SHA256_AUTHENTICODE KPH_KERNEL_PURGE_EA "SHA256A"
#define KPH_HASH_EACACHE_SHA384              KPH_KERNEL_PURGE_EA "SHA384"
#define KPH_HASH_EACACHE_SHA512              KPH_KERNEL_PURGE_EA "SHA512"

#define KPH_HASH_EACACHE_LEN(x)                                                \
    ALIGN_UP_BY(FIELD_OFFSET(FILE_GET_EA_INFORMATION, EaName) +                \
                sizeof(x),                                                     \
                sizeof(ULONG))

#define KPH_HASH_EACACHE_FULL_LEN(x)                                           \
    ALIGN_UP_BY(FIELD_OFFSET(FILE_FULL_EA_INFORMATION, EaName) +               \
                sizeof(x) +                                                    \
                KPH_HASH_ALGORITHM_MAX_LENGTH,                                 \
                sizeof(ULONG))

#define KPH_HASH_EACACHE_MAX_LENGTH (                                          \
    KPH_HASH_EACACHE_LEN(KPH_HASH_EACACHE_MD5) +                               \
    KPH_HASH_EACACHE_LEN(KPH_HASH_EACACHE_SHA1) +                              \
    KPH_HASH_EACACHE_LEN(KPH_HASH_EACACHE_SHA1_AUTHENTICODE) +                 \
    KPH_HASH_EACACHE_LEN(KPH_HASH_EACACHE_SHA256) +                            \
    KPH_HASH_EACACHE_LEN(KPH_HASH_EACACHE_SHA256_AUTHENTICODE) +               \
    KPH_HASH_EACACHE_LEN(KPH_HASH_EACACHE_SHA384) +                            \
    KPH_HASH_EACACHE_LEN(KPH_HASH_EACACHE_SHA512))

#define KPH_HASH_EACACHE_FULL_MAX_LENGTH (                                     \
    KPH_HASH_EACACHE_FULL_LEN(KPH_HASH_EACACHE_MD5) +                          \
    KPH_HASH_EACACHE_FULL_LEN(KPH_HASH_EACACHE_SHA1) +                         \
    KPH_HASH_EACACHE_FULL_LEN(KPH_HASH_EACACHE_SHA1_AUTHENTICODE) +            \
    KPH_HASH_EACACHE_FULL_LEN(KPH_HASH_EACACHE_SHA256) +                       \
    KPH_HASH_EACACHE_FULL_LEN(KPH_HASH_EACACHE_SHA256_AUTHENTICODE) +          \
    KPH_HASH_EACACHE_FULL_LEN(KPH_HASH_EACACHE_SHA384) +                       \
    KPH_HASH_EACACHE_FULL_LEN(KPH_HASH_EACACHE_SHA512))

C_ASSERT(KPH_HASH_EACACHE_FULL_MAX_LENGTH <= KPH_HASHING_BUFFER_SIZE);

typedef struct _KPH_HASHING_INFRASTRUCTURE
{
    PAGED_LOOKASIDE_LIST HashingLookaside;
    BYTE EaList[KPH_HASH_EACACHE_MAX_LENGTH];
} KPH_HASHING_INFRASTRUCTURE, *PKPH_HASHING_INFRASTRUCTURE;

typedef struct _KPH_HASHING_EACACHE_INFORMATION
{
    ULONG HashSize;
    ANSI_STRING EaName;
} KPH_HASHING_EACACHE_INFORMATION, *PKPH_HASHING_EACACHE_INFORMATION;
typedef const KPH_HASHING_EACACHE_INFORMATION *PCKPH_HASHING_EACACHE_INFORMATION;

typedef struct _KPH_HASHING_EACACHE_CONTEXT
{
    HANDLE FileHandle;
    PFILE_OBJECT FileObject;
    HANDLE OplockEventHandle;
    PKEVENT OplockEventObject;
    IO_STATUS_BLOCK IoStatusBlock;
    REQUEST_OPLOCK_OUTPUT_BUFFER OplockOutput;
} KPH_HASHING_EACACHE_CONTEXT, *PKPH_HASHING_EACACHE_CONTEXT;

typedef struct _KPH_HASHING_AUTHENTICODE_CONTEXT
{
    KPH_MEMORY_REGION Regions[4];
    PVOID SecurityBase;
    ULONG SecuritySize;
} KPH_HASHING_AUTHENTICODE_CONTEXT, *PKPH_HASHING_AUTHENTICODE_CONTEXT;

typedef struct _KPH_HASHING_ENTRY
{
    BCRYPT_HASH_HANDLE Handle;
    BOOLEAN HashComplete;
    ULONG Length;
    BYTE Hash[KPH_HASH_ALGORITHM_MAX_LENGTH];
} KPH_HASHING_ENTRY, *PKPH_HASHING_ENTRY;

typedef struct _KPH_HASHING_CONTEXT
{
    KPH_HASHING_ENTRY Hash[MaxKphHashAlgorithm];
    KPH_HASHING_AUTHENTICODE_CONTEXT Authenticode;
    BOOLEAN RequiresHashing;
    KPH_HASHING_EACACHE_CONTEXT EaCache;
    BYTE Buffer[KPH_HASHING_BUFFER_SIZE];
} KPH_HASHING_CONTEXT, *PKPH_HASHING_CONTEXT;

KPH_PROTECTED_DATA_SECTION_RO_PUSH();
static const UNICODE_STRING KphpHashingInfraName = RTL_CONSTANT_STRING(L"KphHashingInfrastructure");
static const KPH_HASHING_EACACHE_INFORMATION KphpHashEaCacheInfo[] =
{
    { (128 / 8), RTL_CONSTANT_STRING(KPH_HASH_EACACHE_MD5) },
    { (160 / 8), RTL_CONSTANT_STRING(KPH_HASH_EACACHE_SHA1) },
    { (160 / 8), RTL_CONSTANT_STRING(KPH_HASH_EACACHE_SHA1_AUTHENTICODE) },
    { (256 / 8), RTL_CONSTANT_STRING(KPH_HASH_EACACHE_SHA256) },
    { (256 / 8), RTL_CONSTANT_STRING(KPH_HASH_EACACHE_SHA256_AUTHENTICODE) },
    { (384 / 8), RTL_CONSTANT_STRING(KPH_HASH_EACACHE_SHA384) },
    { (512 / 8), RTL_CONSTANT_STRING(KPH_HASH_EACACHE_SHA512) },
};
C_ASSERT(ARRAYSIZE(KphpHashEaCacheInfo) == MaxKphHashAlgorithm);
KPH_PROTECTED_DATA_SECTION_RO_POP();
KPH_PROTECTED_DATA_SECTION_PUSH();
static PKPH_HASHING_INFRASTRUCTURE KphpHashingInfra = NULL;
static PKPH_OBJECT_TYPE KphpHashingInfraType = NULL;
KPH_PROTECTED_DATA_SECTION_POP();

KPH_PAGED_FILE();

/**
 * \brief Allocates hashing infrastructure object.
 *
 * \param[in] Size The size to allocate.
 *
 * \return Allocated hashing infrastructure object, null on failure.
 */
_Function_class_(KPH_TYPE_ALLOCATE_PROCEDURE)
_Return_allocatesMem_size_(Size)
PVOID KSIAPI KphpAllocateHashingInfra(
    _In_ SIZE_T Size
    )
{
    KPH_PAGED_CODE();

    return KphAllocateNPaged(Size, KPH_TAG_HASHING_INFRA);
}

/**
 * \brief Initializes hashing infrastructure.
 *
 * \param[in,out] Object The hashing infrastructure to initialize.
 * \param[in] Parameter Unused
 *
 * \return Successful or errant status.
 */
_Function_class_(KPH_TYPE_INITIALIZE_PROCEDURE)
_Must_inspect_result_
NTSTATUS KSIAPI KphpInitHashingInfra(
    _Inout_ PVOID Object,
    _In_opt_ PVOID Parameter
    )
{
    PKPH_HASHING_INFRASTRUCTURE infra;
    PFILE_GET_EA_INFORMATION eaInfo;

    KPH_PAGED_CODE();

    UNREFERENCED_PARAMETER(Parameter);

    infra = Object;

    //
    // Pre-populate the EA cache items into the buffer to be used when querying
    // for the cached EA values. We compile time assert that it will all fit in
    // the buffer.
    //

    eaInfo = (PFILE_GET_EA_INFORMATION)infra->EaList;
    eaInfo->NextEntryOffset = 0;

    for (ULONG i = 0; i < ARRAYSIZE(KphpHashEaCacheInfo); i++)
    {
        PCKPH_HASHING_EACACHE_INFORMATION eaCacheInfo;

        eaInfo = Add2Ptr(eaInfo, eaInfo->NextEntryOffset);

        eaCacheInfo = &KphpHashEaCacheInfo[i];

        RtlCopyMemory(eaInfo->EaName,
                      eaCacheInfo->EaName.Buffer,
                      eaCacheInfo->EaName.Length);

        eaInfo->EaNameLength = (UCHAR)eaCacheInfo->EaName.Length;
        eaInfo->EaName[eaInfo->EaNameLength] = ANSI_NULL;

        eaInfo->NextEntryOffset = FIELD_OFFSET(FILE_GET_EA_INFORMATION, EaName);
        eaInfo->NextEntryOffset += eaCacheInfo->EaName.Length;
        eaInfo->NextEntryOffset += sizeof(ANSI_NULL);
        eaInfo->NextEntryOffset = ALIGN_UP_BY(eaInfo->NextEntryOffset,
                                              sizeof(ULONG));
    }

    eaInfo->NextEntryOffset = 0;

    KphInitializePagedLookaside(&infra->HashingLookaside,
                                sizeof(KPH_HASHING_CONTEXT),
                                KPH_TAG_HASHING_CONTEXT);

    return STATUS_SUCCESS;
}

/**
 * \brief Deletes hashing infrastructure.
 *
 * \param[in,out] Object The hashing infrastructure to delete.
 */
_Function_class_(KPH_TYPE_DELETE_PROCEDURE)
VOID KSIAPI KphpDeleteHashingInfra(
    _Inout_ PVOID Object
    )
{
    PKPH_HASHING_INFRASTRUCTURE infra;

    KPH_PAGED_CODE();

    infra = Object;

    KphDeletePagedLookaside(&infra->HashingLookaside);
}

/**
 * \brief Frees hashing infrastructure object.
 *
 * \param[in] Object The object to free.
 */
_Function_class_(KPH_TYPE_FREE_PROCEDURE)
VOID KSIAPI KphpFreeHashingInfra(
    _In_freesMem_ PVOID Object
    )
{
    KPH_PAGED_CODE();

    KphFree(Object, KPH_TAG_HASHING_INFRA);
}

/**
 * \brief Allocates a hashing context from the hashing look-aside list.
 *
 * \return Hashing context, null on failure.
 */
_IRQL_requires_max_(PASSIVE_LEVEL)
_Return_allocatesMem_
PKPH_HASHING_CONTEXT KphpAllocateHashingContext(
    VOID
    )
{
    KPH_PAGED_CODE_PASSIVE();

    NT_ASSERT(KphpHashingInfra);

    return KphAllocateFromPagedLookaside(&KphpHashingInfra->HashingLookaside);
}

_IRQL_requires_max_(PASSIVE_LEVEL)
VOID KphpCloseHashingEaCacheContext(
    _Inout_ PKPH_HASHING_EACACHE_CONTEXT Context
    )
{
    KPH_PAGED_CODE_PASSIVE();

    if (Context->FileObject)
    {
        ObDereferenceObject(Context->FileObject);
        Context->FileObject = NULL;
    }

    if (Context->FileHandle)
    {
        NT_ASSERT(!KeAreAllApcsDisabled());

        ObCloseHandle(Context->FileHandle, KernelMode);
        Context->FileHandle = NULL;
    }

    if (Context->OplockEventObject)
    {
        KeWaitForSingleObject(Context->OplockEventObject,
                              Executive,
                              KernelMode,
                              FALSE,
                              NULL);

        ObDereferenceObject(Context->OplockEventObject);
        Context->OplockEventObject = NULL;
    }

    if (Context->OplockEventHandle)
    {
        ObCloseHandle(Context->OplockEventHandle, KernelMode);
        Context->OplockEventHandle = NULL;
    }
}

/**
 * \brief Frees a hashing context back to the look-aside list.
 *
 * \param[in] Buffer The context to free back to the look-aside list.
 */
_IRQL_requires_max_(PASSIVE_LEVEL)
VOID KphpFreeHashingContext(
    _In_freesMem_ PKPH_HASHING_CONTEXT Context
    )
{
    KPH_PAGED_CODE_PASSIVE();

    NT_ASSERT(KphpHashingInfra);

    for (ULONG i = 0; i < ARRAYSIZE(Context->Hash); i++)
    {
        if (Context->Hash[i].Handle)
        {
            BCryptDestroyHash(Context->Hash[i].Handle);
        }
    }

    KphpCloseHashingEaCacheContext(&Context->EaCache);

    KphFreeToPagedLookaside(&KphpHashingInfra->HashingLookaside, Context);
}

/**
 * \brief Initializes hashing infrastructure.
 *
 * \return Successful or errant status.
 */
_IRQL_requires_max_(PASSIVE_LEVEL)
_Must_inspect_result_
NTSTATUS KphInitializeHashing(
    VOID
    )
{
    NTSTATUS status;
    KPH_OBJECT_TYPE_INFO typeInfo;

    KPH_PAGED_CODE_PASSIVE();

    typeInfo.Allocate = KphpAllocateHashingInfra;
    typeInfo.Initialize = KphpInitHashingInfra;
    typeInfo.Delete = KphpDeleteHashingInfra;
    typeInfo.Free = KphpFreeHashingInfra;
    typeInfo.Flags = 0;

    KphCreateObjectType(&KphpHashingInfraName,
                        &typeInfo,
                        &KphpHashingInfraType);

    status = KphCreateObject(KphpHashingInfraType,
                             sizeof(KPH_HASHING_INFRASTRUCTURE),
                             &KphpHashingInfra,
                             NULL);
    if (!NT_SUCCESS(status))
    {
        KphpHashingInfra = NULL;
    }

    return status;
}

/**
 * \brief Cleans up hashing infrastructure.
 */
_IRQL_requires_max_(PASSIVE_LEVEL)
VOID KphCleanupHashing(
    VOID
    )
{
    KPH_PAGED_CODE_PASSIVE();

    if (KphpHashingInfra)
    {
        KphDereferenceObject(KphpHashingInfra);
    }
}

/**
 * \brief References the signing infrastructure.
 */
_IRQL_requires_max_(APC_LEVEL)
VOID KphReferenceHashingInfrastructure(
    VOID
    )
{
    KPH_PAGED_CODE();

    NT_ASSERT(KphpHashingInfra);

    KphReferenceObject(KphpHashingInfra);
}

/**
 * \brief Dereferences the signing infrastructure.
 */
_IRQL_requires_max_(APC_LEVEL)
VOID KphDereferenceHashingInfrastructure(
    VOID
    )
{
    KPH_PAGED_CODE();

    NT_ASSERT(KphpHashingInfra);

    KphDereferenceObject(KphpHashingInfra);
}

/**
 * \brief Hashes a buffer.
 *
 * \param[in] Buffer The buffer to hash.
 * \param[in] BufferLength The length of the buffer.
 * \param[in] HashAlgorithm The algorithm to use for hashing.
 * \param[out] HashInformation Populated with the hash information.
 *
 * \return Successful or errant status.
 */
_IRQL_requires_max_(PASSIVE_LEVEL)
_Must_inspect_result_
NTSTATUS KphHashBuffer(
    _In_ PBYTE Buffer,
    _In_ ULONG BufferLength,
    _In_ KPH_HASH_ALGORITHM HashAlgorithm,
    _Out_ PKPH_HASH_INFORMATION HashInformation
    )
{
    NTSTATUS status;
    BCRYPT_ALG_HANDLE algHandle;
    BCRYPT_HASH_HANDLE hashHandle;

    KPH_PAGED_CODE_PASSIVE();

    NT_ASSERT(KphpHashingInfra);

    hashHandle = NULL;

    RtlZeroMemory(HashInformation, sizeof(*HashInformation));

    HashInformation->Algorithm = HashAlgorithm;

    switch (HashAlgorithm)
    {
        case KphHashAlgorithmMd5:
        {
            HashInformation->Length = (128 / 8);
            algHandle = BCRYPT_MD5_ALG_HANDLE;
            break;
        }
        case KphHashAlgorithmSha1:
        {
            HashInformation->Length = (160 / 8);
            algHandle = BCRYPT_SHA1_ALG_HANDLE;
            break;
        }
        case KphHashAlgorithmSha256:
        {
            HashInformation->Length = (256 / 8);
            algHandle = BCRYPT_SHA256_ALG_HANDLE;
            break;
        }
        case KphHashAlgorithmSha384:
        {
            HashInformation->Length = (384 / 8);
            algHandle = BCRYPT_SHA384_ALG_HANDLE;
            break;
        }
        case KphHashAlgorithmSha512:
        {
            HashInformation->Length = (512 / 8);
            algHandle = BCRYPT_SHA512_ALG_HANDLE;
            break;
        }
        default:
        {
            status = STATUS_INVALID_PARAMETER;
            goto Exit;
        }
    }

    status = BCryptCreateHash(algHandle, &hashHandle, NULL, 0, NULL, 0, 0);
    if (!NT_SUCCESS(status))
    {
        KphTracePrint(TRACE_LEVEL_VERBOSE,
                      HASH,
                      "BCryptCreateHash failed: %!STATUS!",
                      status);

        hashHandle = NULL;
        goto Exit;
    }

    status = BCryptHashData(hashHandle, Buffer, BufferLength, 0);
    if (!NT_SUCCESS(status))
    {
        KphTracePrint(TRACE_LEVEL_VERBOSE,
                      HASH,
                      "BCryptHashData failed: %!STATUS!",
                      status);

        goto Exit;
    }

    status = BCryptFinishHash(hashHandle,
                              HashInformation->Hash,
                              HashInformation->Length,
                              0);
    if (!NT_SUCCESS(status))
    {
        KphTracePrint(TRACE_LEVEL_VERBOSE,
                      HASH,
                      "BCryptFinishHash failed: %!STATUS!",
                      status);

        goto Exit;
    }

Exit:

    if (hashHandle)
    {
        BCryptDestroyHash(hashHandle);
    }

    return status;
}

/**
 * \brief Updates a hash with a buffer from the context.
 *
 * \details The internal context buffer must already be updated with a copy of
 * data pointed to by UnsafeBuffer.
 *
 * \param[in,out] Entry The hash entry to update.
 * \param[in] Context The hashing context.
 * \param[in] Authenticode Whether the hash is for authenticode or not.
 * \param[in] UnsafeBuffer Address from which the information was copied.
 * \param[in] Length The length of the buffer.
 *
 * \return Successful or errant status.
 */
_IRQL_requires_max_(PASSIVE_LEVEL)
_Must_inspect_result_
NTSTATUS KphpUpdateHash(
    _Inout_ PKPH_HASHING_ENTRY Entry,
    _In_ PKPH_HASHING_CONTEXT Context,
    _In_ BOOLEAN Authenticode,
    _In_ PVOID UnsafeBuffer,
    _In_ ULONG Length
    )
{
    PVOID unsafeEnd;

    KPH_PAGED_CODE_PASSIVE();
    NT_ASSERT(Entry->Handle && !Entry->HashComplete);

    if (!Authenticode)
    {
        return BCryptHashData(Entry->Handle, Context->Buffer, Length, 0);
    }

    //
    // Only hash within a region relevant for authenticode hashing.
    //

    unsafeEnd = Add2Ptr(UnsafeBuffer, Length);

    for (ULONG i = 0; i < ARRAYSIZE(Context->Authenticode.Regions); i++)
    {
        NTSTATUS status;
        PKPH_MEMORY_REGION region;
        PVOID start;
        PVOID end;
        PVOID buffer;
        ULONG length;

        region = &Context->Authenticode.Regions[i];

        if (!region->Start)
        {
            break;
        }

        if ((UnsafeBuffer >= region->Start) && (UnsafeBuffer < region->End))
        {
            start = UnsafeBuffer;
        }
        else if ((region->Start >= UnsafeBuffer) && (region->Start < unsafeEnd))
        {
            start = region->Start;
        }
        else
        {
            start = NULL;
        }

        if ((unsafeEnd >= region->Start) && (unsafeEnd < region->End))
        {
            end = unsafeEnd;
        }
        else if ((region->End >= UnsafeBuffer) && (region->End < unsafeEnd))
        {
            end = region->End;
        }
        else
        {
            end = NULL;
        }

        if (start && end)
        {
            NOTHING;
        }
        else if (start && !end)
        {
            end = unsafeEnd;
        }
        else if (!start && end)
        {
            start = UnsafeBuffer;
        }
        else
        {
            continue;
        }

        NT_ASSERT(start <= end);

        buffer = Add2Ptr(Context->Buffer, PtrOffset(UnsafeBuffer, start));
        length = PtrOffset(start, end);

        status = BCryptHashData(Entry->Handle, buffer, length, 0);
        if (!NT_SUCCESS(status))
        {
            return status;
        }
    }

    return STATUS_SUCCESS;
}

/**
 * \brief Updates all hashes in the context with the context buffer.
 *
 * \details The internal context buffer must already be updated with a copy of
 * data pointed to by UnsafeBuffer.
 *
 * \param[in,out] Context The hashing context.
 * \param[in] UnsafeBuffer Address from which the information was copied.
 * \param[in] Length The length of the buffer.
 *
 * \return Successful or errant status.
 */
_IRQL_requires_max_(PASSIVE_LEVEL)
_Must_inspect_result_
NTSTATUS KphpUpdateHashes(
    _Inout_ PKPH_HASHING_CONTEXT Context,
    _In_ PVOID UnsafeBuffer,
    _In_ ULONG Length
    )
{
    KPH_PAGED_CODE_PASSIVE();

    for (ULONG i = 0; i < ARRAYSIZE(Context->Hash); i++)
    {
        NTSTATUS status;
        PKPH_HASHING_ENTRY entry;
        BOOLEAN authenticode;

        entry = &Context->Hash[i];

        if (entry->HashComplete || !entry->Handle)
        {
            continue;
        }

        authenticode = ((i == KphHashAlgorithmSha1Authenticode) ||
                        (i == KphHashAlgorithmSha256Authenticode));

        status = KphpUpdateHash(entry,
                                Context,
                                authenticode,
                                UnsafeBuffer,
                                Length);
        if (!NT_SUCCESS(status))
        {
            KphTracePrint(TRACE_LEVEL_VERBOSE,
                          HASH,
                          "KphpUpdateHash failed: %!STATUS!",
                          status);

            return status;
        }
    }

    return STATUS_SUCCESS;
}

/**
 * \brief Initializes a hashing context.
 *
 * \param[in,out] Context The hashing context to initialize.
 * \param[in] HashInformation The hash information to use for initialization.
 * \param[in] NumberOfHashes The number of hashing information items.
 *
 * \return Successful or errant status.
 */
_IRQL_requires_max_(PASSIVE_LEVEL)
_Must_inspect_result_
NTSTATUS KphpInitializeHashingContext(
    _Inout_ PKPH_HASHING_CONTEXT Context,
    _In_ PKPH_HASH_INFORMATION HashInformation,
    _In_ ULONG NumberOfHashes
    )
{
    NTSTATUS status;

    KPH_PAGED_CODE_PASSIVE();

    status = STATUS_SUCCESS;

    for (ULONG i = 0; i < NumberOfHashes; i++)
    {
        PKPH_HASHING_ENTRY entry;
        BCRYPT_ALG_HANDLE algHandle;

        if ((HashInformation[i].Algorithm >= MaxKphHashAlgorithm) ||
            (HashInformation[i].Algorithm < 0))
        {
            status = STATUS_INVALID_PARAMETER;
            goto Exit;
        }

        entry = &Context->Hash[HashInformation[i].Algorithm];

        if (entry->HashComplete || entry->Handle)
        {
            continue;
        }

        Context->RequiresHashing = TRUE;

        switch (HashInformation[i].Algorithm)
        {
            case KphHashAlgorithmMd5:
            {
                entry->Length = (128 / 8);
                algHandle = BCRYPT_MD5_ALG_HANDLE;
                break;
            }
            case KphHashAlgorithmSha1:
            case KphHashAlgorithmSha1Authenticode:
            {
                entry->Length = (160 / 8);
                algHandle = BCRYPT_SHA1_ALG_HANDLE;
                break;
            }
            case KphHashAlgorithmSha256:
            case KphHashAlgorithmSha256Authenticode:
            {
                entry->Length = (256 / 8);
                algHandle = BCRYPT_SHA256_ALG_HANDLE;
                break;
            }
            case KphHashAlgorithmSha384:
            {
                entry->Length = (384 / 8);
                algHandle = BCRYPT_SHA384_ALG_HANDLE;
                break;
            }
            case KphHashAlgorithmSha512:
            {
                entry->Length = (512 / 8);
                algHandle = BCRYPT_SHA512_ALG_HANDLE;
                break;
            }
            DEFAULT_UNREACHABLE;
        }

        NT_ASSERT(entry->Length <= sizeof(Context->Hash));

        status = BCryptCreateHash(algHandle,
                                  &entry->Handle,
                                  NULL,
                                  0,
                                  NULL,
                                  0,
                                  0);
        if (!NT_SUCCESS(status))
        {
            KphTracePrint(TRACE_LEVEL_VERBOSE,
                          HASH,
                          "BCryptCreateHash failed: %!STATUS!",
                          status);

            entry->Handle = NULL;
            goto Exit;
        }
    }

Exit:

    return status;
}

/**
 * \brief Loads hashes from the EA cache into the hashing context.
 *
 * \param[in,out] Context The hashing context to load the hashes into.
 * \param[in] FileHandle The file handle to load the hashes from.
 * \param[in] AccessMode The mode in which to perform access checks.
 */
_IRQL_requires_max_(PASSIVE_LEVEL)
VOID KphpLoadHashesFromEaCache(
    _Inout_ PKPH_HASHING_CONTEXT Context,
    _In_ HANDLE FileHandle,
    _In_ KPROCESSOR_MODE AccessMode
    )
{
    NTSTATUS status;
    PFILE_OBJECT fileObject;
    ULONG returnLength;
    PFILE_FULL_EA_INFORMATION fullEaInfo;

    KPH_PAGED_CODE_PASSIVE();

    status = ObReferenceObjectByHandle(FileHandle,
                                       0,
                                       *IoFileObjectType,
                                       AccessMode,
                                       &fileObject,
                                       NULL);
    if (!NT_SUCCESS(status))
    {
        KphTracePrint(TRACE_LEVEL_VERBOSE,
                      HASH,
                      "ObReferenceObjectByHandle failed: %!STATUS!",
                      status);

        return;
    }

    //
    // N.B. We compile time assert that all the information we might store in
    // our extended attributes will fit within the supplied buffer.
    //

    status = FsRtlQueryKernelEaFile(fileObject,
                                    Context->Buffer,
                                    sizeof(Context->Buffer),
                                    FALSE,
                                    KphpHashingInfra->EaList,
                                    sizeof(KphpHashingInfra->EaList),
                                    NULL,
                                    TRUE,
                                    &returnLength);
    if (!NT_SUCCESS(status))
    {
        KphTracePrint(TRACE_LEVEL_VERBOSE,
                      HASH,
                      "FsRtlQueryKernelEaFile failed: %!STATUS!",
                      status);

        goto Exit;
    }

    fullEaInfo = (PFILE_FULL_EA_INFORMATION)Context->Buffer;

    for (;;)
    {
        ANSI_STRING eaName;

        eaName.Buffer = fullEaInfo->EaName;
        eaName.Length = fullEaInfo->EaNameLength;
        eaName.MaximumLength = fullEaInfo->EaNameLength;

        for (ULONG i = 0; i < ARRAYSIZE(KphpHashEaCacheInfo); i++)
        {
            PCKPH_HASHING_EACACHE_INFORMATION eaCacheInfo;
            PKPH_HASHING_ENTRY entry;
            PVOID buffer;

            eaCacheInfo = &KphpHashEaCacheInfo[i];

            if (fullEaInfo->EaValueLength != eaCacheInfo->HashSize)
            {
                continue;
            }

            if (!RtlEqualString(&eaName, &eaCacheInfo->EaName, FALSE))
            {
                continue;
            }

            entry = &Context->Hash[i];

            buffer = Add2Ptr(fullEaInfo->EaName,
                             fullEaInfo->EaNameLength + sizeof(ANSI_NULL));

            RtlCopyMemory(entry->Hash, buffer, fullEaInfo->EaValueLength);

            entry->Length = fullEaInfo->EaValueLength;
            entry->HashComplete = TRUE;

            break;
        }

        if (!fullEaInfo->NextEntryOffset)
        {
            break;
        }

        fullEaInfo = Add2Ptr(fullEaInfo, fullEaInfo->NextEntryOffset);
    }

Exit:

    ObDereferenceObject(fileObject);
}

/**
 * \brief Initializes the EA cache context for hashing.
 *
 * \details The EA caching is completely opportunistic and will only be used if
 * we can guarantee cache consistency. We do this using an opportunistic lock.
 * The caller should call KphpProgressEaCacheContext to after each read from
 * the file data to determine if there is another contender for writing to the
 * file. If contention is detected, the EA cache will be closed and the cache
 * will not be updated. This avoids contention on the system and ensures cache
 * consistency.
 *
 * \param[in,out] Context The hashing context to initialize.
 * \param[in] FileHandle The file handle to initialize the context with.
 */
_IRQL_requires_max_(PASSIVE_LEVEL)
VOID KphpInitializeEaCacheContext(
    _Inout_ PKPH_HASHING_CONTEXT Context,
    _In_ HANDLE FileHandle
    )
{
    NTSTATUS status;
    UNICODE_STRING objectName;
    OBJECT_ATTRIBUTES objectAttributes;
    IO_STATUS_BLOCK ioStatusBlock;
    REQUEST_OPLOCK_INPUT_BUFFER oplockInput;
    ULONG64 usnValue;
    ULONG returnLength;

    KPH_PAGED_CODE_PASSIVE();

    RtlInitUnicodeString(&objectName, NULL);

    InitializeObjectAttributes(&objectAttributes,
                               &objectName,
                               OBJ_KERNEL_HANDLE,
                               FileHandle,
                               NULL);

    status = KphCreateFile(&Context->EaCache.FileHandle,
                           FILE_WRITE_EA | FILE_READ_DATA | FILE_READ_ATTRIBUTES,
                           &objectAttributes,
                           &ioStatusBlock,
                           NULL,
                           FILE_ATTRIBUTE_NORMAL,
                           FILE_SHARE_READ,
                           FILE_OPEN,
                           FILE_NON_DIRECTORY_FILE | FILE_OPEN_REQUIRING_OPLOCK,
                           NULL,
                           0,
                           0,
                           KernelMode);
    if (!NT_SUCCESS(status))
    {
        KphTracePrint(TRACE_LEVEL_VERBOSE,
                      HASH,
                      "KphCreateFile failed: %!STATUS!",
                      status);

        Context->EaCache.FileHandle = NULL;
        goto Exit;
    }

    status = ObReferenceObjectByHandle(Context->EaCache.FileHandle,
                                       FILE_ALL_ACCESS,
                                       *IoFileObjectType,
                                       KernelMode,
                                       &Context->EaCache.FileObject,
                                       NULL);
    if (!NT_SUCCESS(status))
    {
        KphTracePrint(TRACE_LEVEL_VERBOSE,
                      HASH,
                      "ObReferenceObjectByHandle failed: %!STATUS!",
                      status);

        Context->EaCache.FileObject = NULL;
        goto Exit;
    }

    status = ZwCreateEvent(&Context->EaCache.OplockEventHandle,
                           EVENT_ALL_ACCESS,
                           NULL,
                           NotificationEvent,
                           TRUE);
    if (!NT_SUCCESS(status))
    {
        KphTracePrint(TRACE_LEVEL_VERBOSE,
                      HASH,
                      "ZwCreateEvent failed: %!STATUS!",
                      status);

        Context->EaCache.OplockEventHandle = NULL;
        goto Exit;
    }

    status = ObReferenceObjectByHandle(Context->EaCache.OplockEventHandle,
                                       EVENT_ALL_ACCESS,
                                       *ExEventObjectType,
                                       KernelMode,
                                       &Context->EaCache.OplockEventObject,
                                       NULL);
    if (!NT_SUCCESS(status))
    {
        KphTracePrint(TRACE_LEVEL_VERBOSE,
                      HASH,
                      "ObReferenceObjectByHandle failed: %!STATUS!",
                      status);

        Context->EaCache.OplockEventObject = NULL;
        goto Exit;
    }

    oplockInput.StructureVersion = REQUEST_OPLOCK_CURRENT_VERSION;
    oplockInput.StructureLength = sizeof(oplockInput);
    oplockInput.Flags = REQUEST_OPLOCK_INPUT_FLAG_REQUEST;
    oplockInput.RequestedOplockLevel = (OPLOCK_LEVEL_CACHE_READ |
                                        OPLOCK_LEVEL_CACHE_HANDLE);

    KeResetEvent(Context->EaCache.OplockEventObject);

    status = ZwFsControlFile(Context->EaCache.FileHandle,
                             Context->EaCache.OplockEventHandle,
                             NULL,
                             NULL,
                             &Context->EaCache.IoStatusBlock,
                             FSCTL_REQUEST_OPLOCK,
                             &oplockInput,
                             sizeof(oplockInput),
                             &Context->EaCache.OplockOutput,
                             sizeof(Context->EaCache.OplockOutput));
    if (status != STATUS_PENDING)
    {
        KeSetEvent(Context->EaCache.OplockEventObject, IO_NO_INCREMENT, FALSE);

        KphTracePrint(TRACE_LEVEL_VERBOSE,
                      HASH,
                      "ZwFsControlFile returned: %!STATUS!",
                      status);

        if (NT_SUCCESS(status))
        {
            status = STATUS_UNEXPECTED_IO_ERROR;
        }

        goto Exit;
    }

    status = FsRtlKernelFsControlFile(Context->EaCache.FileObject,
                                      FSCTL_WRITE_USN_CLOSE_RECORD,
                                      NULL,
                                      0,
                                      &usnValue,
                                      sizeof(usnValue),
                                      &returnLength);
    if (!NT_SUCCESS(status))
    {
        KphTracePrint(TRACE_LEVEL_VERBOSE,
                      HASH,
                      "FsRtlKernelFsControlFile failed: %!STATUS!",
                      status);

        goto Exit;
    }

Exit:

    if (!NT_SUCCESS(status))
    {
        KphpCloseHashingEaCacheContext(&Context->EaCache);
    }
}

/**
 * \brief Progresses the EA cache context.
 *
 * \details This function should be called after each read from the file data.
 *
 * \param[in,out] Context The hashing context to progress the EA cache of.
 *
 * \return TRUE if the EA cache is available and the opportunistic lock has not
 * been broken, FALSE otherwise.
 */
_IRQL_requires_max_(PASSIVE_LEVEL)
BOOLEAN KphpProgressEaCacheContext(
    _Inout_ PKPH_HASHING_CONTEXT Context
    )
{
    LARGE_INTEGER zeroTimeout = KPH_TIMEOUT(0);

    KPH_PAGED_CODE_PASSIVE();

    if (!Context->EaCache.OplockEventObject)
    {
        return FALSE;
    }

    if (KeWaitForSingleObject(Context->EaCache.OplockEventObject,
                              Executive,
                              KernelMode,
                              FALSE,
                              &zeroTimeout) != STATUS_SUCCESS)
    {
        return TRUE;
    }

    //
    // The opportunistic lock has been broken. We could try to guarantee cache
    // consistency by not closing the handle. Doing so would block the thread
    // attempting to open a write handle until we acknowledge the break.
    //
    // An opportunistic lock break can also happen when a write occurs to the
    // file, but this will not suspend the write operation for acknowledgment.
    // Therefore, acknowledging the break here is important to avoid the
    // possible cache inconsistency.
    //
    // We choose to completely get out of the way of the system and acknowledge
    // the break of the opportunistic lock by closing the handle. We will
    // continue to hash and return whatever that result is, but we will not
    // update the EA cache since we can not guarantee cash consistency.
    //
    // N.B. It is important to close the handle instead of using the lock break
    // acknowledgment control code, otherwise we could be the cause of a
    // sharing violation.
    //

    KphTracePrint(TRACE_LEVEL_VERBOSE, HASH, "Opportunistic lock broken");

    KphpCloseHashingEaCacheContext(&Context->EaCache);

    return FALSE;
}

/**
 * \brief Initializes the hashing context for file hashing.
 *
 * \param[in,out] Context The hashing context to initialize.
 * \param[in] FileHandle The file handle to initialize the context with.
 * \param[in] HashInformation The hash information to use for initialization.
 * \param[in] NumberOfHashes The number of hashing information items.
 * \param[in] AccessMode The mode in which to perform access checks.
 *
 * \return Successful or errant status.
 */
_IRQL_requires_max_(PASSIVE_LEVEL)
_Must_inspect_result_
NTSTATUS KphpInitializeFileHashingContext(
    _Inout_ PKPH_HASHING_CONTEXT Context,
    _In_ HANDLE FileHandle,
    _In_ PKPH_HASH_INFORMATION HashInformation,
    _In_ ULONG NumberOfHashes,
    _In_ KPROCESSOR_MODE AccessMode
    )
{
    NTSTATUS status;

    KPH_PAGED_CODE_PASSIVE();

    KphpLoadHashesFromEaCache(Context, FileHandle, AccessMode);

    status = KphpInitializeHashingContext(Context,
                                          HashInformation,
                                          NumberOfHashes);
    if (NT_SUCCESS(status) && Context->RequiresHashing)
    {
        KphpInitializeEaCacheContext(Context, FileHandle);
    }

    return status;
}

/**
 * \brief Initializes the hashing context for authenticode hashing.
 *
 * \details This function can raise a structured exception due to an in-page
 * error, which callers should be prepared to handle.
 *
 * \param[in,out] Context The hashing context to initialize.
 * \param[in] MappedBase The base address of the mapped image.
 * \param[in] ViewSize The size of the mapped image.
 *
 * \return Successful or errant status.
 */
_IRQL_requires_max_(PASSIVE_LEVEL)
NTSTATUS KphpInitializeAuthenticodeHashing(
    _Inout_ PKPH_HASHING_CONTEXT Context,
    _In_ PVOID MappedBase,
    _In_ SIZE_T ViewSize
    )
{
    NTSTATUS status;
    PVOID mappedEnd;
    KPH_IMAGE_NT_HEADERS image;
    PIMAGE_DATA_DIRECTORY securityDir;
    PVOID securityBase;
    ULONG securitySize;
    PVOID securityEnd;

    KPH_PAGED_CODE_PASSIVE();

    RtlZeroMemory(&Context->Authenticode, sizeof(Context->Authenticode));

    mappedEnd = Add2Ptr(MappedBase, ViewSize);
    securityDir = NULL;
    securityBase = NULL;
    securityEnd = NULL;

    NT_ASSERT(MappedBase <= mappedEnd);

    status = KphImageNtHeader(MappedBase, ViewSize, &image);
    if (!NT_SUCCESS(status))
    {
        goto Exit;
    }

    if (!RTL_CONTAINS_FIELD(&image.Headers->OptionalHeader,
                            image.Headers->FileHeader.SizeOfOptionalHeader,
                            Magic))
    {
        Context->Authenticode.Regions[0].Start = MappedBase;
        Context->Authenticode.Regions[0].End = mappedEnd;
        status = STATUS_SUCCESS;
        goto VerifyRegions;
    }

    if (image.Headers->OptionalHeader.Magic == IMAGE_NT_OPTIONAL_HDR32_MAGIC)
    {
        if (!RTL_CONTAINS_FIELD(&image.Headers32->OptionalHeader,
                                image.Headers->FileHeader.SizeOfOptionalHeader,
                                CheckSum))
        {
            Context->Authenticode.Regions[0].Start = MappedBase;
            Context->Authenticode.Regions[0].End = mappedEnd;
            status = STATUS_SUCCESS;
            goto VerifyRegions;
        }

        Context->Authenticode.Regions[0].Start = MappedBase;
        Context->Authenticode.Regions[0].End = &image.Headers32->OptionalHeader.CheckSum;

        Context->Authenticode.Regions[1].Start = &image.Headers32->OptionalHeader.Subsystem;
        if (!RTL_CONTAINS_FIELD(&image.Headers32->OptionalHeader,
                                image.Headers->FileHeader.SizeOfOptionalHeader,
                                NumberOfRvaAndSizes) ||
            (image.Headers32->OptionalHeader.NumberOfRvaAndSizes <= IMAGE_DIRECTORY_ENTRY_SECURITY))
        {
            Context->Authenticode.Regions[1].End = mappedEnd;
            status = STATUS_SUCCESS;
            goto VerifyRegions;
        }

        securityDir = &image.Headers32->OptionalHeader.DataDirectory[IMAGE_DIRECTORY_ENTRY_SECURITY];
        Context->Authenticode.Regions[1].End = securityDir;
    }
    else if (image.Headers->OptionalHeader.Magic == IMAGE_NT_OPTIONAL_HDR64_MAGIC)
    {
        if (!RTL_CONTAINS_FIELD(&image.Headers64->OptionalHeader,
                                image.Headers->FileHeader.SizeOfOptionalHeader,
                                CheckSum))
        {
            Context->Authenticode.Regions[0].Start = MappedBase;
            Context->Authenticode.Regions[0].End = mappedEnd;
            status = STATUS_SUCCESS;
            goto VerifyRegions;
        }

        Context->Authenticode.Regions[0].Start = MappedBase;
        Context->Authenticode.Regions[0].End = &image.Headers64->OptionalHeader.CheckSum;

        Context->Authenticode.Regions[1].Start = &image.Headers64->OptionalHeader.Subsystem;
        if (!RTL_CONTAINS_FIELD(&image.Headers64->OptionalHeader,
                                image.Headers->FileHeader.SizeOfOptionalHeader,
                                NumberOfRvaAndSizes) ||
            (image.Headers64->OptionalHeader.NumberOfRvaAndSizes <= IMAGE_DIRECTORY_ENTRY_SECURITY))
        {
            Context->Authenticode.Regions[1].End = mappedEnd;
            status = STATUS_SUCCESS;
            goto VerifyRegions;
        }

        securityDir = &image.Headers64->OptionalHeader.DataDirectory[IMAGE_DIRECTORY_ENTRY_SECURITY];
        Context->Authenticode.Regions[1].End = securityDir;
    }
    else
    {
        status = STATUS_INVALID_IMAGE_FORMAT;
        goto Exit;
    }

    NT_ASSERT(securityDir);

    Context->Authenticode.Regions[2].Start = &securityDir[1];

    if (!securityDir->VirtualAddress || !securityDir->Size)
    {
        Context->Authenticode.Regions[2].End = mappedEnd;
        status = STATUS_SUCCESS;
        goto VerifyRegions;
    }

    securityBase = Add2Ptr(MappedBase, securityDir->VirtualAddress);
    securitySize = securityDir->Size;
    securityEnd = Add2Ptr(securityBase, securitySize);

    Context->Authenticode.SecurityBase = securityBase;
    Context->Authenticode.SecuritySize = securitySize;

    Context->Authenticode.Regions[2].End = securityBase;

    if (securityEnd < mappedEnd)
    {
        Context->Authenticode.Regions[3].Start = securityEnd;
        Context->Authenticode.Regions[3].End = mappedEnd;
    }

VerifyRegions:

    for (ULONG i = 0; i < ARRAYSIZE(Context->Authenticode.Regions); i++)
    {
        PKPH_MEMORY_REGION region;

        region = &Context->Authenticode.Regions[i];

        if (!region->Start)
        {
            break;
        }

        if (!KphIsValidMemoryRegion(region, MappedBase, mappedEnd))
        {
            KphTracePrint(TRACE_LEVEL_VERBOSE,
                          HASH,
                          "Authenticode region overflows mapping.");

            status = STATUS_BUFFER_OVERFLOW;
            goto Exit;
        }
    }

    if (securityBase)
    {
        KPH_MEMORY_REGION securityRegion;

        securityRegion.Start = securityBase;
        securityRegion.End = securityEnd;

        if (!KphIsValidMemoryRegion(&securityRegion, MappedBase, mappedEnd))
        {
            KphTracePrint(TRACE_LEVEL_VERBOSE,
                          HASH,
                          "Security directory overflows mapping.");

            status = STATUS_BUFFER_OVERFLOW;
            goto Exit;
        }
    }

Exit:

    if (!NT_SUCCESS(status))
    {
        RtlZeroMemory(&Context->Authenticode, sizeof(Context->Authenticode));
    }

    return status;
}

/**
 * \brief Finishes hashes in the hashing context.
 *
 * \param[in,out] Context The hashing context to finish the hashes in.
 *
 * \return Successful or errant status.
 */
_IRQL_requires_max_(PASSIVE_LEVEL)
_Must_inspect_result_
NTSTATUS KphpFinishHashes(
    _In_ PKPH_HASHING_CONTEXT Context
    )
{
    NTSTATUS status;
    PFILE_FULL_EA_INFORMATION fullEaInfo;
    ULONG fullEaInfoLength;

    KPH_PAGED_CODE_PASSIVE();

    fullEaInfoLength = 0;

    if (Context->EaCache.FileObject)
    {
        fullEaInfo = (PFILE_FULL_EA_INFORMATION)Context->Buffer;
        fullEaInfo->NextEntryOffset = 0;
    }
    else
    {
        fullEaInfo = NULL;
    }

    for (ULONG i = 0; i < ARRAYSIZE(Context->Hash); i++)
    {
        PKPH_HASHING_ENTRY entry;
        PCKPH_HASHING_EACACHE_INFORMATION eaInfo;
        PVOID buffer;

        entry = &Context->Hash[i];

        if (!entry->Handle)
        {
            continue;
        }

        status = BCryptFinishHash(entry->Handle,
                                  entry->Hash,
                                  entry->Length,
                                  0);
        if (!NT_SUCCESS(status))
        {
            KphTracePrint(TRACE_LEVEL_VERBOSE,
                          HASH,
                          "BCryptFinishHash failed: %!STATUS!",
                          status);

            return status;
        }

        entry->HashComplete = TRUE;

        if (!fullEaInfo)
        {
            continue;
        }

        fullEaInfo = Add2Ptr(fullEaInfo, fullEaInfo->NextEntryOffset);

        eaInfo = &KphpHashEaCacheInfo[i];

        fullEaInfo->Flags = 0;
        fullEaInfo->EaNameLength = (UCHAR)eaInfo->EaName.Length;
        fullEaInfo->EaValueLength = (USHORT)entry->Length;

        RtlCopyMemory(fullEaInfo->EaName,
                      eaInfo->EaName.Buffer,
                      eaInfo->EaName.Length);

        fullEaInfo->EaName[eaInfo->EaName.Length] = ANSI_NULL;

        buffer = Add2Ptr(fullEaInfo->EaName,
                         fullEaInfo->EaNameLength + sizeof(ANSI_NULL));

        RtlCopyMemory(buffer, entry->Hash, entry->Length);

        buffer = Add2Ptr(buffer, entry->Length);

        fullEaInfo->NextEntryOffset = ALIGN_UP_BY(PtrOffset(fullEaInfo, buffer),
                                                  sizeof(ULONG));

        fullEaInfoLength += fullEaInfo->NextEntryOffset;
    }

    if (fullEaInfoLength && KphpProgressEaCacheContext(Context))
    {
        NT_ASSERT(Context->EaCache.FileObject);
        NT_ASSERT(fullEaInfo);

        fullEaInfo->NextEntryOffset = 0;

        status = FsRtlSetKernelEaFile(Context->EaCache.FileObject,
                                      Context->Buffer,
                                      fullEaInfoLength);
        if (!NT_SUCCESS(status))
        {
            KphTracePrint(TRACE_LEVEL_VERBOSE,
                          HASH,
                          "FsRtlSetKernelEaFile failed: %!STATUS!",
                          status);
        }
    }

    return STATUS_SUCCESS;
}

/**
 * \brief Copies hashes from the hashing context to the hash information.
 *
 * \param[in] Context The hashing context to copy the hashes from.
 * \param[in,out] HashInformation The hash information to copy the hashes to.
 * \param[in] NumberOfHashes The number of hashing information items.
 */
_IRQL_requires_max_(PASSIVE_LEVEL)
VOID KphpCopyHashes(
    _In_ PKPH_HASHING_CONTEXT Context,
    _Inout_ PKPH_HASH_INFORMATION HashInformation,
    _In_ ULONG NumberOfHashes
    )
{
    KPH_PAGED_CODE_PASSIVE();

    for (ULONG i = 0; i < NumberOfHashes; i++)
    {
        PKPH_HASHING_ENTRY entry;
        PKPH_HASH_INFORMATION info;

        info = &HashInformation[i];

        NT_ASSERT((info->Algorithm < MaxKphHashAlgorithm) &&
                  (info->Algorithm >= 0));

        entry = &Context->Hash[info->Algorithm];

        NT_ASSERT(entry->HashComplete);

        info->Length = entry->Length;

        RtlCopyMemory(info->Hash, entry->Hash, entry->Length);
    }
}

/**
 * \brief Hashes a file.
 *
 * \param[in] FileHandle Handle to a file to hash.
 * \param[in,out] HashInformation The hash information to populate with the
 * requested hash algorithms. On input this array contains the requested hash
 * algorithms to use, and on output the hash information items are populated
 * with the requested hash values.
 * \param[in] NumberOfHashes The number of hash information items.
 * \param[in] AccessMode The mode in which to perform access checks.
 *
 * \return Successful or errant status.
 */
_IRQL_requires_max_(PASSIVE_LEVEL)
_Must_inspect_result_
NTSTATUS KphpHashFile(
    _In_ HANDLE FileHandle,
    _Inout_ PKPH_HASH_INFORMATION HashInformation,
    _In_ ULONG NumberOfHashes,
    _In_ KPROCESSOR_MODE AccessMode
    )
{
    NTSTATUS status;
    PKPH_HASHING_CONTEXT context;
    PVOID mappedBase;
    SIZE_T viewSize;
    ULONG readSize;

    KPH_PAGED_CODE_PASSIVE();

    mappedBase = NULL;

    context = KphpAllocateHashingContext();
    if (!context)
    {
        status = STATUS_INSUFFICIENT_RESOURCES;
        goto Exit;
    }

    status = KphpInitializeFileHashingContext(context,
                                              FileHandle,
                                              HashInformation,
                                              NumberOfHashes,
                                              AccessMode);
    if (!NT_SUCCESS(status))
    {
        KphTracePrint(TRACE_LEVEL_VERBOSE,
                      HASH,
                      "KphpInitializeFileHashingContext failed: %!STATUS!",
                      status);

        goto Exit;
    }

    if (!context->RequiresHashing)
    {
        goto Finish;
    }

    viewSize = 0;

    status = KphMapViewInSystem(FileHandle,
                                KPH_MAP_DATA,
                                &mappedBase,
                                &viewSize);
    if (!NT_SUCCESS(status))
    {
        KphTracePrint(TRACE_LEVEL_VERBOSE,
                      HASH,
                      "KphMapViewInSystem failed: %!STATUS!",
                      status);

        mappedBase = NULL;
        goto Exit;
    }

    if (context->Hash[KphHashAlgorithmSha1Authenticode].Handle ||
        context->Hash[KphHashAlgorithmSha256Authenticode].Handle)
    {
        __try
        {
            KphpInitializeAuthenticodeHashing(context, mappedBase, viewSize);
        }
        __except (EXCEPTION_EXECUTE_HANDLER)
        {
            status = GetExceptionCode();
            goto Exit;
        }
    }

    readSize = 0;

    for (SIZE_T offset = 0; offset < viewSize; offset += readSize)
    {
        PVOID buffer;

        buffer = &((PBYTE)mappedBase)[offset];
        readSize = (ULONG)min(viewSize - offset, sizeof(context->Buffer));

        __try
        {
            RtlCopyMemory(context->Buffer, buffer, readSize);
        }
        __except (EXCEPTION_EXECUTE_HANDLER)
        {
            status = GetExceptionCode();
            goto Exit;
        }

        KphpProgressEaCacheContext(context);

        status = KphpUpdateHashes(context, buffer, readSize);
        if (!NT_SUCCESS(status))
        {
            KphTracePrint(TRACE_LEVEL_VERBOSE,
                          HASH,
                          "KphpUpdateHashes failed: %!STATUS!",
                          status);

            goto Exit;
        }
    }

    status = KphpFinishHashes(context);
    if (!NT_SUCCESS(status))
    {
        KphTracePrint(TRACE_LEVEL_VERBOSE,
                      HASH,
                      "KphpFinishHashes failed: %!STATUS!",
                      status);

        goto Exit;
    }

Finish:

    KphpCopyHashes(context, HashInformation, NumberOfHashes);

Exit:

    if (mappedBase)
    {
        KphUnmapViewInSystem(mappedBase);
    }

    if (context)
    {
        KphpFreeHashingContext(context);
    }

    return status;
}

/**
 * \brief Queries hash information for a file.
 *
 * \param[in] FileHandle Handle to a file to query.
 * \param[in,out] HashInformation The hash information to populate with the
 * requested hash algorithms. On input this array contains the requested hash
 * algorithms to use, and on output the hash information items are populated
 * with the requested hash values.
 * \param[in] HashInformationLength The length of the hash information in bytes.
 * \param[in] AccessMode The mode in which to perform access checks.
 *
 * \return Successful or errant status.
 */
_IRQL_requires_max_(PASSIVE_LEVEL)
_Must_inspect_result_
NTSTATUS KphQueryHashInformationFile(
    _In_ HANDLE FileHandle,
    _Inout_ PKPH_HASH_INFORMATION HashInformation,
    _In_ ULONG HashInformationLength,
    _In_ KPROCESSOR_MODE AccessMode
    )
{
    NTSTATUS status;
    PKPH_HASH_INFORMATION hashInfo;
    ULONG numberOfHashes;

    KPH_PAGED_CODE_PASSIVE();

    hashInfo = NULL;
    numberOfHashes = HashInformationLength / sizeof(KPH_HASH_INFORMATION);

    if (!HashInformation || !numberOfHashes)
    {
        status = STATUS_INVALID_PARAMETER;
        goto Exit;
    }

    if (AccessMode != KernelMode)
    {
        hashInfo = KphAllocatePaged(HashInformationLength,
                                    KPH_TAG_CAPTURED_HASHES);
        if (!hashInfo)
        {
            status = STATUS_INSUFFICIENT_RESOURCES;
            goto Exit;
        }

        __try
        {
            ProbeInputBytes(HashInformation, HashInformationLength);
            RtlCopyVolatileMemory(hashInfo,
                                  HashInformation,
                                  HashInformationLength);
        }
        __except (EXCEPTION_EXECUTE_HANDLER)
        {
            status = GetExceptionCode();
            goto Exit;
        }
    }
    else
    {
        hashInfo = HashInformation;
    }

    status = KphpHashFile(FileHandle, hashInfo, numberOfHashes, AccessMode);
    if (!NT_SUCCESS(status))
    {
        KphTracePrint(TRACE_LEVEL_VERBOSE,
                      HASH,
                      "KphpHashFile failed: %!STATUS!",
                      status);

        goto Exit;
    }

    if (AccessMode != KernelMode)
    {
        __try
        {
            ProbeOutputBytes(HashInformation, HashInformationLength);
            RtlCopyMemory(HashInformation, hashInfo, HashInformationLength);
        }
        __except (EXCEPTION_EXECUTE_HANDLER)
        {
            status = GetExceptionCode();
            goto Exit;
        }
    }

Exit:

    if (hashInfo && (hashInfo != HashInformation))
    {
        KphFree(hashInfo, KPH_TAG_CAPTURED_HASHES);
    }

    return status;
}


/**
 * \brief Queries hash information for a file object.
 *
 * \param[in] FileObject The file to query.
 * \param[in,out] HashInformation The hash information to populate with the
 * requested hash algorithms. On input this array contains the requested hash
 * algorithms to use, and on output the hash information items are populated
 * with the requested hash values.
 * \param[in] HashInformationLength The length of the hash information in bytes.
 * \param[in] AccessMode The mode in which to perform access checks.
 *
 * \return Successful or errant status.
 */
_IRQL_requires_max_(PASSIVE_LEVEL)
_Must_inspect_result_
NTSTATUS KphQueryHashInformationFileObject(
    _In_ PFILE_OBJECT FileObject,
    _Inout_ PKPH_HASH_INFORMATION HashInformation,
    _In_ ULONG HashInformationLength
    )
{
    NTSTATUS status;
    HANDLE fileHandle;

    KPH_PAGED_CODE_PASSIVE();

    status = ObOpenObjectByPointer(FileObject,
                                   OBJ_KERNEL_HANDLE,
                                   NULL,
                                   0,
                                   *IoFileObjectType,
                                   KernelMode,
                                   &fileHandle);
    if (!NT_SUCCESS(status))
    {
        KphTracePrint(TRACE_LEVEL_VERBOSE,
                      HASH,
                      "ObOpenObjectByPointer failed: %!STATUS!",
                      status);

        return status;
    }

    status = KphQueryHashInformationFile(fileHandle,
                                         HashInformation,
                                         HashInformationLength,
                                         KernelMode);

    ObCloseHandle(fileHandle, KernelMode);

    return status;
}
