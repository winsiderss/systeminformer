/*
 * Copyright (c) 2022 Winsider Seminars & Solutions, Inc.  All rights reserved.
 *
 * This file is part of System Informer.
 *
 * Authors:
 *
 *     jxy-s   2022
 *
 */

#include <kph.h>

#include <trace.h>

typedef struct _KPH_HASHING_INFRASTRUCTURE
{
    PAGED_LOOKASIDE_LIST HashingLookaside;
    BCRYPT_ALG_HANDLE BCryptSha1Provider;
    BCRYPT_ALG_HANDLE BCryptSha256Provider;
} KPH_HASHING_INFRASTRUCTURE, *PKPH_HASHING_INFRASTRUCTURE;

static UNICODE_STRING KphpHashingInfraName = RTL_CONSTANT_STRING(L"KphHashingInfrastructure");
static PKPH_OBJECT_TYPE KphpHashingInfraType = NULL;
static PKPH_HASHING_INFRASTRUCTURE KphpHashingInfra = NULL;

PAGED_FILE();

#define KPH_HASHING_BUFFER_SIZE (16 * 1024)

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
    PAGED_CODE();

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
    NTSTATUS status;
    PKPH_HASHING_INFRASTRUCTURE infra;

    PAGED_CODE();

    UNREFERENCED_PARAMETER(Parameter);

    infra = Object;

    status = BCryptOpenAlgorithmProvider(&infra->BCryptSha1Provider,
                                         BCRYPT_SHA1_ALGORITHM,
                                         NULL,
                                         0);
    if (!NT_SUCCESS(status))
    {
        KphTracePrint(TRACE_LEVEL_ERROR,
                      HASH,
                      "BCryptOpenAlgorithmProvider failed: %!STATUS!",
                      status);

        infra->BCryptSha1Provider = NULL;
        goto Exit;
    }

    status = BCryptOpenAlgorithmProvider(&infra->BCryptSha256Provider,
                                         BCRYPT_SHA256_ALGORITHM,
                                         NULL,
                                         0);
    if (!NT_SUCCESS(status))
    {
        KphTracePrint(TRACE_LEVEL_ERROR,
                      HASH,
                      "BCryptOpenAlgorithmProvider failed: %!STATUS!",
                      status);

        infra->BCryptSha256Provider = NULL;
        goto Exit;
    }

    KphInitializePagedLookaside(&infra->HashingLookaside,
                                KPH_HASHING_BUFFER_SIZE,
                                KPH_TAG_HASHING_BUFFER);
    status = STATUS_SUCCESS;

Exit:

    if (!NT_SUCCESS(status))
    {
        if (infra->BCryptSha1Provider)
        {
            BCryptCloseAlgorithmProvider(infra->BCryptSha1Provider, 0);
        }

        if (infra->BCryptSha256Provider)
        {
            BCryptCloseAlgorithmProvider(infra->BCryptSha256Provider, 0);
        }
    }

    return status;
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

    PAGED_CODE();

    infra = Object;

    NT_ASSERT(infra->BCryptSha1Provider);
    BCryptCloseAlgorithmProvider(infra->BCryptSha1Provider, 0);

    NT_ASSERT(infra->BCryptSha256Provider);
    BCryptCloseAlgorithmProvider(infra->BCryptSha256Provider, 0);

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
    PAGED_CODE();

    KphFree(Object, KPH_TAG_HASHING_INFRA);
}

/**
 * \brief Allocates a hashing buffer from the hashing look-aside list.
 *
 * \return Hashing buffer, null on failure.
 */
_IRQL_requires_max_(PASSIVE_LEVEL)
_Return_allocatesMem_
PBYTE KphpAllocateHashingBuffer(
    VOID
    )
{
    PAGED_PASSIVE();

    NT_ASSERT(KphpHashingInfra);

    return KphAllocateFromPagedLookaside(&KphpHashingInfra->HashingLookaside);
}

/**
 * \brief Frees a hashing buffer back to the look-aside list.
 *
 * \param[in] Buffer The buffer to free back to the look-aside list.
 */
_IRQL_requires_max_(PASSIVE_LEVEL)
VOID KphpFreeHashingBuffer(
    _In_freesMem_ PBYTE Buffer
    )
{
    PAGED_PASSIVE();

    NT_ASSERT(KphpHashingInfra);

    KphFreeToPagedLookaside(&KphpHashingInfra->HashingLookaside,
                            Buffer);
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

    PAGED_PASSIVE();

    typeInfo.Allocate = KphpAllocateHashingInfra;
    typeInfo.Initialize = KphpInitHashingInfra;
    typeInfo.Delete = KphpDeleteHashingInfra;
    typeInfo.Free = KphpFreeHashingInfra;

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
    PAGED_PASSIVE();

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
    PAGED_CODE();

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
    PAGED_CODE();

    NT_ASSERT(KphpHashingInfra);

    KphDereferenceObject(KphpHashingInfra);
}

/**
 * \brief Hashes a buffer.
 *
 * \param[in] Buffer The buffer to hash.
 * \param[in] BufferLength The length of the buffer.
 * \param[in] AlgorithmId The algorithm to use.
 * \param[out] Hash Populated with the hash.
 *
 * \return Successful or errant status.
 */
_IRQL_requires_max_(PASSIVE_LEVEL)
_Must_inspect_result_
NTSTATUS KphHashBuffer(
    _In_ PBYTE Buffer,
    _In_ ULONG BufferLength,
    _In_ ALG_ID AlgorithmId,
    _Out_ PKPH_HASH Hash
    )
{
    NTSTATUS status;
    BCRYPT_ALG_HANDLE algHandle;
    BCRYPT_HASH_HANDLE hashHandle;

    PAGED_PASSIVE();

    NT_ASSERT(KphpHashingInfra);

    RtlZeroMemory(Hash, sizeof(*Hash));

    if (AlgorithmId == CALG_SHA1)
    {
        algHandle = KphpHashingInfra->BCryptSha1Provider;
    }
    else if (AlgorithmId == CALG_SHA_256)
    {
        algHandle = KphpHashingInfra->BCryptSha256Provider;
    }
    else
    {
        return STATUS_INVALID_PARAMETER_2;
    }

    status = BCryptCreateHash(algHandle, &hashHandle, NULL, 0, NULL, 0, 0);
    if (!NT_SUCCESS(status))
    {
        KphTracePrint(TRACE_LEVEL_ERROR,
                      HASH,
                      "BCryptCreateHash failed: %!STATUS!",
                      status);

        hashHandle = NULL;
        goto Exit;
    }

    status = BCryptHashData(hashHandle, Buffer, BufferLength, 0);
    if (!NT_SUCCESS(status))
    {
        KphTracePrint(TRACE_LEVEL_ERROR,
                      HASH,
                      "BCryptHashData failed: %!STATUS!",
                      status);

        goto Exit;
    }

    if (AlgorithmId == CALG_SHA1)
    {
        NT_ASSERT(ARRAYSIZE(Hash->Buffer) >= MINCRYPT_SHA1_HASH_LEN);

        status = BCryptFinishHash(hashHandle,
                                  Hash->Buffer,
                                  MINCRYPT_SHA1_HASH_LEN,
                                  0);
        if (!NT_SUCCESS(status))
        {
            KphTracePrint(TRACE_LEVEL_ERROR,
                          HASH,
                          "BCryptFinishHash failed: %!STATUS!",
                          status);

            goto Exit;
        }

        Hash->AlgorithmId = CALG_SHA1;
        Hash->Size = MINCRYPT_SHA1_HASH_LEN;
    }
    else
    {
        NT_ASSERT(AlgorithmId == CALG_SHA_256);
        NT_ASSERT(ARRAYSIZE(Hash->Buffer) >= MINCRYPT_SHA256_HASH_LEN);

        status = BCryptFinishHash(hashHandle,
                                  Hash->Buffer,
                                  MINCRYPT_SHA256_HASH_LEN,
                                  0);
        if (!NT_SUCCESS(status))
        {
            KphTracePrint(TRACE_LEVEL_ERROR,
                          HASH,
                          "BCryptFinishHash failed: %!STATUS!",
                          status);

            goto Exit;
        }

        Hash->AlgorithmId = CALG_SHA_256;
        Hash->Size = MINCRYPT_SHA256_HASH_LEN;
    }

    status = STATUS_SUCCESS;

Exit:

    if (hashHandle)
    {
        BCryptDestroyHash(hashHandle);
    }

    return status;
}

/**he algorithm to use
 * \brief Hashes a file.
 *
 * \param[in] FileHandle Handle to the file to hash.
 * \param[in] AlgorithmId Algorithm to use to hash (CALG_SHA1 or CALG_SHA_256).
 * \param[out] Hash Populated with hash.
 *
 * \return Successful or errant status.
 */
_IRQL_requires_max_(PASSIVE_LEVEL)
_Must_inspect_result_
NTSTATUS KphHashFile(
    _In_ HANDLE FileHandle,
    _In_ ALG_ID AlgorithmId,
    _Out_ PKPH_HASH Hash
    )
{
    NTSTATUS status;
    BCRYPT_ALG_HANDLE algHandle;
    BCRYPT_HASH_HANDLE hashHandle;
    PVOID mappedBase;
    SIZE_T viewSize;
    PBYTE readBuffer;
    SIZE_T readSize;

    PAGED_PASSIVE();

    NT_ASSERT(KphpHashingInfra);

    hashHandle = NULL;
    mappedBase = NULL;
    readBuffer = NULL;

    RtlZeroMemory(Hash, sizeof(*Hash));

    if (AlgorithmId == CALG_SHA1)
    {
        algHandle = KphpHashingInfra->BCryptSha1Provider;
    }
    else if (AlgorithmId == CALG_SHA_256)
    {
        algHandle = KphpHashingInfra->BCryptSha256Provider;
    }
    else
    {
        return STATUS_INVALID_PARAMETER_2;
    }

    NT_ASSERT(algHandle);

    viewSize = 0;
    status = KphMapViewInSystem(FileHandle, 0, &mappedBase, &viewSize);
    if (!NT_SUCCESS(status))
    {
        KphTracePrint(TRACE_LEVEL_ERROR,
                      HASH,
                      "KphMapViewInSystem failed: %!STATUS!",
                      status);

        mappedBase = NULL;
        goto Exit;
    }

    status = BCryptCreateHash(algHandle, &hashHandle, NULL, 0, NULL, 0, 0);
    if (!NT_SUCCESS(status))
    {
        KphTracePrint(TRACE_LEVEL_ERROR,
                      HASH,
                      "BCryptCreateHash failed: %!STATUS!",
                      status);

        hashHandle = NULL;
        goto Exit;
    }

    readBuffer = KphpAllocateHashingBuffer();
    if (!readBuffer)
    {
        KphTracePrint(TRACE_LEVEL_ERROR,
                      HASH,
                      "Failed to allocate hashing buffer.");

        status = STATUS_INSUFFICIENT_RESOURCES;
        goto Exit;
    }

    readSize = 0;
    for (SIZE_T offset = 0; offset < viewSize; offset += readSize)
    {
        readSize = min(viewSize - offset, KPH_HASHING_BUFFER_SIZE);

        __try
        {
            RtlCopyMemory(readBuffer, &((PBYTE)mappedBase)[offset], readSize);
        }
        __except (EXCEPTION_EXECUTE_HANDLER)
        {
            status = GetExceptionCode();
            goto Exit;
        }

        NT_ASSERT(readSize <= MAXULONG);

        status = BCryptHashData(hashHandle, readBuffer, (ULONG)readSize, 0);
        if (!NT_SUCCESS(status))
        {
            KphTracePrint(TRACE_LEVEL_ERROR,
                          HASH,
                          "BCryptHashData failed: %!STATUS!",
                          status);

            goto Exit;
        }
    }

    if (AlgorithmId == CALG_SHA1)
    {
        NT_ASSERT(ARRAYSIZE(Hash->Buffer) >= MINCRYPT_SHA1_HASH_LEN);

        status = BCryptFinishHash(hashHandle,
                                  Hash->Buffer,
                                  MINCRYPT_SHA1_HASH_LEN,
                                  0);
        if (!NT_SUCCESS(status))
        {
            KphTracePrint(TRACE_LEVEL_ERROR,
                          HASH,
                          "BCryptFinishHash failed: %!STATUS!",
                          status);

            goto Exit;
        }

        Hash->AlgorithmId = CALG_SHA1;
        Hash->Size = MINCRYPT_SHA1_HASH_LEN;
    }
    else
    {
        NT_ASSERT(AlgorithmId == CALG_SHA_256);
        NT_ASSERT(ARRAYSIZE(Hash->Buffer) >= MINCRYPT_SHA256_HASH_LEN);

        status = BCryptFinishHash(hashHandle,
                                  Hash->Buffer,
                                  MINCRYPT_SHA256_HASH_LEN,
                                  0);
        if (!NT_SUCCESS(status))
        {
            KphTracePrint(TRACE_LEVEL_ERROR,
                          HASH,
                          "BCryptFinishHash failed: %!STATUS!",
                          status);

            goto Exit;
        }

        Hash->AlgorithmId = CALG_SHA_256;
        Hash->Size = MINCRYPT_SHA256_HASH_LEN;
    }

    status = STATUS_SUCCESS;

Exit:

    if (readBuffer)
    {
        KphpFreeHashingBuffer(readBuffer);
    }

    if (hashHandle)
    {
        BCryptDestroyHash(hashHandle);
    }

    if (mappedBase)
    {
        KphUnmapViewInSystem(mappedBase);
    }

    return status;
}

/**
 * \brief Hashes a file by file name.
 *
 * \param[in] FileName File name of file to hash.
 * \param[in] AlgorithmId Algorithm to use to hash (CALG_SHA1 or CALG_SHA_256).
 * \param[out] Hash Populated with hash.
 *
 * \return Successful or errant status.
 */
_IRQL_requires_max_(PASSIVE_LEVEL)
_Must_inspect_result_
NTSTATUS KphHashFileByName(
    _In_ PUNICODE_STRING FileName,
    _In_ ALG_ID AlgorithmId,
    _Out_ PKPH_HASH Hash
    )
{
    NTSTATUS status;
    HANDLE fileHandle;
    OBJECT_ATTRIBUTES objectAttributes;
    IO_STATUS_BLOCK ioStatusBlock;

    PAGED_PASSIVE();

    RtlZeroMemory(Hash, sizeof(*Hash));

    InitializeObjectAttributes(&objectAttributes,
                               FileName,
                               OBJ_KERNEL_HANDLE,
                               NULL,
                               NULL);

    status = KphCreateFile(&fileHandle,
                           FILE_READ_ACCESS | SYNCHRONIZE,
                           &objectAttributes,
                           &ioStatusBlock,
                           NULL,
                           FILE_ATTRIBUTE_NORMAL,
                           FILE_SHARE_READ | FILE_SHARE_WRITE | FILE_SHARE_DELETE,
                           FILE_OPEN,
                           FILE_NON_DIRECTORY_FILE | FILE_SYNCHRONOUS_IO_NONALERT,
                           NULL,
                           0,
                           IO_IGNORE_SHARE_ACCESS_CHECK,
                           KernelMode);
    if (!NT_SUCCESS(status))
    {
        KphTracePrint(TRACE_LEVEL_ERROR,
                      HASH,
                      "KphCreateFile failed: %!STATUS!",
                      status);

        fileHandle = NULL;
        goto Exit;
    }

    status = KphHashFile(fileHandle, AlgorithmId, Hash);

Exit:

    if (fileHandle)
    {
        ObCloseHandle(fileHandle, KernelMode);
    }

    return status;
}

/**
 * \brief Retrieves authenticode info from a PE file.
 *
 * \param[in] FileHandle Handle to PE file to get authenticode info from.
 * \param[out] Info Populated with authenticode info. The signature is
 * allocated and copied into the output. The caller should free this by passing
 * the structure to KphAuthenticodeInfo.
 *
 * \return Successful or errant status.
 */
_IRQL_requires_max_(PASSIVE_LEVEL)
_Must_inspect_result_
NTSTATUS KphGetAuthenticodeInfo(
    _In_ HANDLE FileHandle,
    _Out_allocatesMem_ PKPH_AUTHENTICODE_INFO Info
    )
{
    NTSTATUS status;
    KPH_SIZED_BUFFER regions[4];
    PVOID mappedBase;
    SIZE_T viewSize;
    PVOID mappedEnd;
    BCRYPT_HASH_HANDLE sha1Handle;
    BCRYPT_HASH_HANDLE sha256Handle;
    union
    {
        PIMAGE_NT_HEADERS Headers;
        PIMAGE_NT_HEADERS32 Headers32;
        PIMAGE_NT_HEADERS64 Headers64;
    } image;
    PIMAGE_DATA_DIRECTORY securityDir;
    PVOID securityBase;
    ULONG securitySize;
    PVOID securityEnd;
    PBYTE readBuffer;

    PAGED_PASSIVE();

    RtlZeroMemory(regions, sizeof(regions));
    mappedBase = NULL;
    mappedEnd = NULL;
    sha1Handle = NULL;
    sha256Handle = NULL;
    securityDir = NULL;
    securityBase = NULL;
    securitySize = 0;
    securityEnd = NULL;
    readBuffer = NULL;

    RtlZeroMemory(Info, sizeof(*Info));

    NT_ASSERT(KphpHashingInfra);
    NT_ASSERT(KphpHashingInfra->BCryptSha1Provider);
    NT_ASSERT(KphpHashingInfra->BCryptSha256Provider);

    viewSize = 0;
    status = KphMapViewInSystem(FileHandle, 0, &mappedBase, &viewSize);
    if (!NT_SUCCESS(status))
    {
        KphTracePrint(TRACE_LEVEL_ERROR,
                      HASH,
                      "KphMapViewInSystem failed: %!STATUS!",
                      status);

        mappedBase = NULL;
        goto Exit;
    }

    mappedEnd = Add2Ptr(mappedBase, viewSize);

    __try
    {
        status = RtlImageNtHeaderEx(0, mappedBase, viewSize, &image.Headers);
        if (!NT_SUCCESS(status))
        {
            image.Headers = NULL;
            goto Exit;
        }

        mappedEnd = Add2Ptr(mappedBase, viewSize);
        if (mappedEnd < mappedBase)
        {
            status = STATUS_BUFFER_OVERFLOW;
            goto Exit;
        }

        if (image.Headers->OptionalHeader.Magic == IMAGE_NT_OPTIONAL_HDR32_MAGIC)
        {
            regions[0].Buffer = (PBYTE)mappedBase;
            regions[0].Size = PtrOffset(regions[0].Buffer, &image.Headers32->OptionalHeader.CheckSum);

            regions[1].Buffer = (PBYTE)&image.Headers32->OptionalHeader.Subsystem;
            if (image.Headers32->OptionalHeader.NumberOfRvaAndSizes <= IMAGE_DIRECTORY_ENTRY_SECURITY)
            {
                regions[1].Size = PtrOffset(regions[1].Buffer, mappedEnd);
                goto BeginHashing;
            }
            securityDir = &image.Headers32->OptionalHeader.DataDirectory[IMAGE_DIRECTORY_ENTRY_SECURITY];
            regions[1].Size = PtrOffset(regions[1].Buffer, securityDir);

        }
        else if (image.Headers->OptionalHeader.Magic == IMAGE_NT_OPTIONAL_HDR64_MAGIC)
        {
            regions[0].Buffer = (PBYTE)mappedBase;
            regions[0].Size = PtrOffset(regions[0].Buffer, &image.Headers64->OptionalHeader.CheckSum);

            regions[1].Buffer = (PBYTE)&image.Headers64->OptionalHeader.Subsystem;
            if (image.Headers64->OptionalHeader.NumberOfRvaAndSizes <= IMAGE_DIRECTORY_ENTRY_SECURITY)
            {
                regions[1].Size = PtrOffset(regions[1].Buffer, mappedEnd);
                goto BeginHashing;
            }
            securityDir = &image.Headers64->OptionalHeader.DataDirectory[IMAGE_DIRECTORY_ENTRY_SECURITY];
            regions[1].Size = PtrOffset(regions[1].Buffer, securityDir);
        }
        else
        {
            status = STATUS_INVALID_IMAGE_FORMAT;
            goto Exit;
        }

        NT_ASSERT(securityDir);
        regions[2].Buffer = (PBYTE)&securityDir[1];
        if ((securityDir->VirtualAddress == 0) || (securityDir->Size == 0))
        {
            regions[2].Size = PtrOffset(regions[2].Buffer, mappedEnd);
            goto BeginHashing;
        }

        securityBase = Add2Ptr(mappedBase, securityDir->VirtualAddress);
        securitySize = securityDir->Size;
        securityEnd = Add2Ptr(securityBase, securitySize);
        regions[2].Size = PtrOffset(regions[2].Buffer, securityBase);

        if (securityEnd < mappedEnd)
        {
            regions[3].Buffer = (PBYTE)securityEnd;
            regions[3].Size = PtrOffset(regions[3].Buffer, mappedEnd);
        }
    }
    __except (EXCEPTION_EXECUTE_HANDLER)
    {
        status = GetExceptionCode();
        goto Exit;
    }

BeginHashing:

    status = BCryptCreateHash(KphpHashingInfra->BCryptSha1Provider,
                              &sha1Handle,
                              NULL,
                              0,
                              NULL,
                              0,
                              0);
    if (!NT_SUCCESS(status))
    {
        KphTracePrint(TRACE_LEVEL_ERROR,
                      HASH,
                      "BCryptCreateHash failed: %!STATUS!",
                      status);

        sha1Handle = NULL;
        goto Exit;
    }

    status = BCryptCreateHash(KphpHashingInfra->BCryptSha256Provider,
                              &sha256Handle,
                              NULL,
                              0,
                              NULL,
                              0,
                              0);
    if (!NT_SUCCESS(status))
    {
        KphTracePrint(TRACE_LEVEL_ERROR,
                      HASH,
                      "BCryptCreateHash failed: %!STATUS!",
                      status);

        sha256Handle = NULL;
        goto Exit;
    }

    readBuffer = KphpAllocateHashingBuffer();
    if (!readBuffer)
    {
        KphTracePrint(TRACE_LEVEL_ERROR,
                      HASH,
                      "Failed to allocate hashing buffer.");

        status = STATUS_INSUFFICIENT_RESOURCES;
        goto Exit;
    }

    //
    // Verify that the regions are all within the view.
    //
    for (ULONG i = 0; i < ARRAYSIZE(regions); i++)
    {
        if (!regions[i].Buffer)
        {
            break;
        }

        if (((PVOID)regions[i].Buffer < mappedBase) ||
            (Add2Ptr((PVOID)regions[i].Buffer, regions[i].Size) < mappedBase) ||
            ((PVOID)regions[i].Buffer >= mappedEnd) ||
            (Add2Ptr(regions[i].Buffer, regions[i].Size) > mappedEnd))
        {
            KphTracePrint(TRACE_LEVEL_ERROR,
                          HASH,
                          "Authenticode region overflows mapping.");

            status = STATUS_BUFFER_OVERFLOW;
            goto Exit;
        }
    }

    if (securityBase)
    {
        NT_ASSERT(securitySize > 0);
        NT_ASSERT(securityEnd);

        if ((securityBase < mappedBase) || (securityEnd < mappedBase) ||
            (securityBase >= mappedEnd) || (securityEnd > mappedEnd))
        {
            KphTracePrint(TRACE_LEVEL_ERROR,
                          HASH,
                          "Security directory overflows mapping.");

            status = STATUS_BUFFER_OVERFLOW;
            goto Exit;
        }
    }

    for (ULONG i = 0; i < ARRAYSIZE(regions); i++)
    {
        ULONG readSize;

        if (!regions[i].Buffer)
        {
            break;
        }

        readSize = 0;
        for (ULONG offset = 0; offset < regions[i].Size; offset += readSize)
        {
            readSize = min(regions[i].Size - offset, KPH_HASHING_BUFFER_SIZE);

            __try
            {
                RtlCopyMemory(readBuffer,
                              &regions[i].Buffer[offset],
                              readSize);
            }
            __except (EXCEPTION_EXECUTE_HANDLER)
            {
                status = GetExceptionCode();
                goto Exit;
            }

            status = BCryptHashData(sha1Handle, readBuffer, readSize, 0);
            if (!NT_SUCCESS(status))
            {
                KphTracePrint(TRACE_LEVEL_ERROR,
                              HASH,
                              "BCryptHashData failed: %!STATUS!",
                              status);

                goto Exit;
            }

            status = BCryptHashData(sha256Handle, readBuffer, readSize, 0);
            if (!NT_SUCCESS(status))
            {
                KphTracePrint(TRACE_LEVEL_ERROR,
                              HASH,
                              "BCryptHashData failed: %!STATUS!",
                              status);

                goto Exit;
            }
        }
    }

    status = BCryptFinishHash(sha1Handle,
                              Info->SHA1,
                              ARRAYSIZE(Info->SHA1),
                              0);
    if (!NT_SUCCESS(status))
    {
        KphTracePrint(TRACE_LEVEL_ERROR,
                      HASH,
                      "BCryptFinishHash failed: %!STATUS!",
                      status);

        goto Exit;
    }

    status = BCryptFinishHash(sha256Handle,
                              Info->SHA256,
                              ARRAYSIZE(Info->SHA256),
                              0);
    if (!NT_SUCCESS(status))
    {
        KphTracePrint(TRACE_LEVEL_ERROR,
                      HASH,
                      "BCryptFinishHash failed: %!STATUS!",
                      status);

        goto Exit;
    }

    Info->Signature = NULL;
    Info->SignatureSize = 0;
    if (securityBase)
    {
        NT_ASSERT(securitySize > 0);
        Info->Signature = KphAllocatePaged(securitySize,
                                           KPH_TAG_AUTHENTICODE_SIG);
        if (!Info->Signature)
        {
            KphTracePrint(TRACE_LEVEL_ERROR,
                          HASH,
                          "Failed to allocate buffer for signature.");

            status = STATUS_INSUFFICIENT_RESOURCES;
            goto Exit;
        }

        __try
        {
            RtlCopyMemory(Info->Signature, securityBase, securitySize);
        }
        __except (EXCEPTION_EXECUTE_HANDLER)
        {
            status = GetExceptionCode();
            ExFreePoolWithTag(Info->Signature, KPH_TAG_AUTHENTICODE_SIG);
            Info->Signature = NULL;
            goto Exit;
        }

        Info->SignatureSize = securitySize;
    }

    status = STATUS_SUCCESS;

Exit:

    if (readBuffer)
    {
        KphpFreeHashingBuffer(readBuffer);
    }

    if (sha256Handle)
    {
        BCryptDestroyHash(sha256Handle);
    }

    if (sha1Handle)
    {
        BCryptDestroyHash(sha1Handle);
    }

    if (mappedBase)
    {
        KphUnmapViewInSystem(mappedBase);
    }

    return status;
}

/**
 * \brief Retrieves authenticode info from a PE file by file name.
 *
 * \param[in] FileName Name of PE file to get authenticode info from.
 * \param[out] Info Populated with authenticode info. The signature is
 * allocated and copied into the output. The caller should free this by passing
 * the structure to KphAuthenticodeInfo.
 *
 * \return Successful or errant status.
 */
_IRQL_requires_max_(PASSIVE_LEVEL)
_Must_inspect_result_
NTSTATUS KphGetAuthenticodeInfoByFileName(
    _In_ PUNICODE_STRING FileName,
    _Out_allocatesMem_ PKPH_AUTHENTICODE_INFO Info
    )
{
    NTSTATUS status;
    HANDLE fileHandle;
    OBJECT_ATTRIBUTES objectAttributes;
    IO_STATUS_BLOCK ioStatusBlock;

    PAGED_PASSIVE();

    RtlZeroMemory(Info, sizeof(*Info));

    InitializeObjectAttributes(&objectAttributes,
                               FileName,
                               OBJ_KERNEL_HANDLE,
                               NULL,
                               NULL);

    status = KphCreateFile(&fileHandle,
                           FILE_READ_ACCESS | SYNCHRONIZE,
                           &objectAttributes,
                           &ioStatusBlock,
                           NULL,
                           FILE_ATTRIBUTE_NORMAL,
                           FILE_SHARE_READ | FILE_SHARE_WRITE | FILE_SHARE_DELETE,
                           FILE_OPEN,
                           FILE_NON_DIRECTORY_FILE | FILE_SYNCHRONOUS_IO_NONALERT,
                           NULL,
                           0,
                           IO_IGNORE_SHARE_ACCESS_CHECK,
                           KernelMode);
    if (!NT_SUCCESS(status))
    {
        KphTracePrint(TRACE_LEVEL_ERROR,
                      HASH,
                      "KphCreateFile failed: %!STATUS!",
                      status);

        fileHandle = NULL;
        goto Exit;
    }

    status = KphGetAuthenticodeInfo(fileHandle, Info);

Exit:

    if (fileHandle)
    {
        ObCloseHandle(fileHandle, KernelMode);
    }

    return status;
}

/**
 * \brief Frees authenticode info.
 *
 * \param[in] Info Authenticode info to free.
 */
_IRQL_requires_max_(PASSIVE_LEVEL)
VOID KphFreeAuthenticodeInfo(
    _In_freesMem_ PKPH_AUTHENTICODE_INFO Info
    )
{
    PAGED_PASSIVE();

    if (Info->Signature)
    {
        ExFreePoolWithTag(Info->Signature, KPH_TAG_AUTHENTICODE_SIG);
    }
}
