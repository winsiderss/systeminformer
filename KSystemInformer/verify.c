/*
 * Copyright (c) 2022 Winsider Seminars & Solutions, Inc.  All rights reserved.
 *
 * This file is part of System Informer.
 *
 * Authors:
 *
 *     wj32    2016
 *     jxy-s   2020
 *
 */

#include <kph.h>
#include <dyndata.h>
#include <ntimage.h>

#define FILE_BUFFER_SIZE (2 * PAGE_SIZE)
#define FILE_MAX_SIZE (32 * 1024 * 1024) // 32 MB
#define CALLER_CHECK_MAX_FRAMES 60
#ifdef _WIN64
#define PROC_VERIFY_COHERENCY_VALUE 990 // 99.9%
#else
#define PROC_VERIFY_COHERENCY_VALUE 9   // 90.0% - 32bit will have more relocations 
#endif

VOID KphpBackoffKey(
    _In_ PKPH_CLIENT Client
    );

static UCHAR KphpTrustedPublicKey[] =
{
    0x45, 0x43, 0x53, 0x31, 0x20, 0x00, 0x00, 0x00, 0x5f, 0xe8, 0xab, 0xac, 0x01, 0xad, 0x6b, 0x48,
    0xfd, 0x84, 0x7f, 0x43, 0x70, 0xb6, 0x57, 0xb0, 0x76, 0xe3, 0x10, 0x07, 0x19, 0xbd, 0x0e, 0xd4,
    0x10, 0x5c, 0x1f, 0xfc, 0x40, 0x91, 0xb6, 0xed, 0x94, 0x37, 0x76, 0xb7, 0x86, 0x88, 0xf7, 0x34,
    0x12, 0x91, 0xf6, 0x65, 0x23, 0x58, 0xc9, 0xeb, 0x2f, 0xcb, 0x96, 0x13, 0x8f, 0xca, 0x57, 0x7a,
    0xd0, 0x7a, 0xbf, 0x22, 0xde, 0xd2, 0x15, 0xfc
};

_IRQL_requires_max_(PASSIVE_LEVEL)
_Must_inspect_result_
NTSTATUS KpiVerifyProcess(
    _In_ PUNICODE_STRING ProcessFileName,
    _Out_ PKPH_EXTENTS TrustedExtents
    );

_IRQL_requires_max_(PASSIVE_LEVEL)
_Must_inspect_result_
NTSTATUS KpiVerifyProcessSections(
    _In_ PVOID BaseAddress,
    _In_ HANDLE FileHandle,
    _In_ PIMAGE_SECTION_HEADER SectionHeaders,
    _In_ WORD NumberOfSections,
    _Out_ PKPH_EXTENTS TrustedExtents
    );

_IRQL_requires_max_(PASSIVE_LEVEL)
_Must_inspect_result_
NTSTATUS KpiVerifyCallerTrustedExtents(
    _In_ PKPH_CLIENT Client
    );

#ifdef ALLOC_PRAGMA
#pragma alloc_text(PAGE, KphHashFile)
#pragma alloc_text(PAGE, KphVerifyFile)
#pragma alloc_text(PAGE, KpiVerifyProcessSections)
#pragma alloc_text(PAGE, KpiVerifyProcess)
#pragma alloc_text(PAGE, KphVerifyClient)
#pragma alloc_text(PAGE, KpiVerifyClient)
#pragma alloc_text(PAGE, KphGenerateKeysClient)
#pragma alloc_text(PAGE, KphRetrieveKeyViaApc)
#pragma alloc_text(PAGE, KphValidateKey)
#pragma alloc_text(PAGE, KphpBackoffKey)
#pragma alloc_text(PAGE, KphDominationCheck)
#pragma alloc_text(PAGE, KphVerifyCaller)
#pragma alloc_text(PAGE, KpiVerifyCallerTrustedExtents)
#endif

NTSTATUS KphHashFile(
    _In_ PUNICODE_STRING FileName,
    _Out_ PVOID *Hash,
    _Out_ PULONG HashSize
    )
{
    NTSTATUS status;
    BCRYPT_ALG_HANDLE hashAlgHandle = NULL;
    ULONG querySize;
    ULONG hashObjectSize;
    ULONG hashSize;
    PVOID hashObject = NULL;
    PVOID hash = NULL;
    BCRYPT_HASH_HANDLE hashHandle = NULL;
    OBJECT_ATTRIBUTES objectAttributes;
    IO_STATUS_BLOCK iosb;
    HANDLE fileHandle = NULL;
    FILE_STANDARD_INFORMATION standardInfo;
    ULONG remainingBytes;
    ULONG bytesToRead;
    PVOID buffer = NULL;

    PAGED_CODE();

    // Open the hash algorithm and allocate memory for the hash object.

    if (!NT_SUCCESS(status = BCryptOpenAlgorithmProvider(&hashAlgHandle, KPH_HASH_ALGORITHM, NULL, 0)))
        goto CleanupExit;

    if (!NT_SUCCESS(status = BCryptGetProperty(
        hashAlgHandle,
        BCRYPT_OBJECT_LENGTH,
        (PUCHAR)&hashObjectSize,
        sizeof(ULONG),
        &querySize,
        0
        )))
    {
        goto CleanupExit;
    }

    if (!NT_SUCCESS(status = BCryptGetProperty(
        hashAlgHandle,
        BCRYPT_HASH_LENGTH,
        (PUCHAR)&hashSize,
        sizeof(ULONG),
        &querySize,
        0
        )))
    {
        goto CleanupExit;
    }

    if (!(hashObject = ExAllocatePoolZero(PagedPool, hashObjectSize, 'vhpK')))
    {
        status = STATUS_INSUFFICIENT_RESOURCES;
        goto CleanupExit;
    }

    if (!(hash = ExAllocatePoolZero(PagedPool, hashSize, 'vhpK')))
    {
        status = STATUS_INSUFFICIENT_RESOURCES;
        goto CleanupExit;
    }

    if (!NT_SUCCESS(status = BCryptCreateHash(
        hashAlgHandle,
        &hashHandle,
        hashObject,
        hashObjectSize,
        NULL,
        0,
        0
        )))
    {
        goto CleanupExit;
    }

    // Open the file and compute the hash.

    InitializeObjectAttributes(
        &objectAttributes,
        FileName,
        OBJ_KERNEL_HANDLE,
        NULL,
        NULL
        );

    if (!NT_SUCCESS(status = ZwCreateFile(
        &fileHandle,
        FILE_GENERIC_READ,
        &objectAttributes,
        &iosb,
        NULL,
        FILE_ATTRIBUTE_NORMAL,
        FILE_SHARE_READ,
        FILE_OPEN,
        FILE_NON_DIRECTORY_FILE | FILE_SYNCHRONOUS_IO_NONALERT,
        NULL,
        0
        )))
    {
        fileHandle = NULL;
        goto CleanupExit;
    }

    if (!NT_SUCCESS(status = ZwQueryInformationFile(
        fileHandle,
        &iosb,
        &standardInfo,
        sizeof(FILE_STANDARD_INFORMATION),
        FileStandardInformation
        )))
    {
        goto CleanupExit;
    }

    if (standardInfo.EndOfFile.QuadPart <= 0)
    {
        status = STATUS_UNSUCCESSFUL;
        goto CleanupExit;
    }
    if (standardInfo.EndOfFile.QuadPart > FILE_MAX_SIZE)
    {
        status = STATUS_FILE_TOO_LARGE;
        goto CleanupExit;
    }

    if (!(buffer = ExAllocatePoolZero(PagedPool, FILE_BUFFER_SIZE, 'vhpK')))
    {
        status = STATUS_INSUFFICIENT_RESOURCES;
        goto CleanupExit;
    }

    remainingBytes = (ULONG)standardInfo.EndOfFile.QuadPart;

    while (remainingBytes != 0)
    {
        bytesToRead = FILE_BUFFER_SIZE;

        if (bytesToRead > remainingBytes)
            bytesToRead = remainingBytes;

        if (!NT_SUCCESS(status = ZwReadFile(
            fileHandle,
            NULL,
            NULL,
            NULL,
            &iosb,
            buffer,
            bytesToRead,
            NULL,
            NULL
            )))
        {
            goto CleanupExit;
        }

        if ((ULONG)iosb.Information != bytesToRead)
        {
            status = STATUS_INTERNAL_ERROR;
            goto CleanupExit;
        }

        if (!NT_SUCCESS(status = BCryptHashData(hashHandle, buffer, bytesToRead, 0)))
            goto CleanupExit;

        remainingBytes -= bytesToRead;
    }

    if (!NT_SUCCESS(status = BCryptFinishHash(hashHandle, hash, hashSize, 0)))
        goto CleanupExit;

    if (NT_SUCCESS(status))
    {
        *Hash = hash;
        *HashSize = hashSize;

        hash = NULL; // Don't free this in the cleanup section
    }

CleanupExit:
    if (buffer)
        ExFreePoolWithTag(buffer, 'vhpK');
    if (fileHandle)
        ZwClose(fileHandle);
    if (hashHandle)
        BCryptDestroyHash(hashHandle);
    if (hash)
        ExFreePoolWithTag(hash, 'vhpK');
    if (hashObject)
        ExFreePoolWithTag(hashObject, 'vhpK');
    if (hashAlgHandle)
        BCryptCloseAlgorithmProvider(hashAlgHandle, 0);

    return status;
}

NTSTATUS KphVerifyFile(
    _In_ PUNICODE_STRING FileName,
    _In_reads_bytes_(SignatureSize) PUCHAR Signature,
    _In_ ULONG SignatureSize
    )
{
    NTSTATUS status;
    BCRYPT_ALG_HANDLE signAlgHandle = NULL;
    BCRYPT_KEY_HANDLE keyHandle = NULL;
    PVOID hash = NULL;
    ULONG hashSize;

    PAGED_CODE();

    // Import the trusted public key.

    if (!NT_SUCCESS(status = BCryptOpenAlgorithmProvider(&signAlgHandle, KPH_SIGN_ALGORITHM, NULL, 0)))
        goto CleanupExit;

    if (!NT_SUCCESS(status = BCryptImportKeyPair(
        signAlgHandle,
        NULL,
        KPH_BLOB_PUBLIC,
        &keyHandle,
        KphpTrustedPublicKey,
        sizeof(KphpTrustedPublicKey),
        0
        )))
    {
        goto CleanupExit;
    }

    // Hash the file.

    if (!NT_SUCCESS(status = KphHashFile(FileName, &hash, &hashSize)))
        goto CleanupExit;

    // Verify the hash.

    if (!NT_SUCCESS(status = BCryptVerifySignature(keyHandle, NULL, hash, hashSize, Signature, SignatureSize, 0)))
        goto CleanupExit;

CleanupExit:
    if (hash)
        ExFreePoolWithTag(hash, 'vhpK');
    if (keyHandle)
        BCryptDestroyKey(keyHandle);
    if (signAlgHandle)
        BCryptCloseAlgorithmProvider(signAlgHandle, 0);

    return status;
}

/**
 * Verifies the process sections are expected and meet coherency requirements.
 *
 * \details This function assumes the current attached process is the one to
 * inspect and BaseAddress is the base address of the current process.
 *
 * \param[in] BaseAddress - Base address of process.
 * \param[in] FileHandle - Handle to the file backing the process image.
 * \param[in] SectionHeaders - Section headers of the process.
 * \param[in] NumberOfSections - Number of sections in the SectionHeaders
 * parameter.
 * \param[out] TrustedExtents - On success, set to the validated module extents.
 *
 * \return Appropriate status.
*/
_IRQL_requires_max_(PASSIVE_LEVEL)
_Must_inspect_result_
NTSTATUS KpiVerifyProcessSections(
    _In_ PVOID BaseAddress,
    _In_ HANDLE FileHandle,
    _In_ PIMAGE_SECTION_HEADER SectionHeaders,
    _In_ WORD NumberOfSections,
    _Out_ PKPH_EXTENTS TrustedExtents
    )
{
    NTSTATUS status;
    PIMAGE_SECTION_HEADER textSection;
    ULONG inspectBytes;
    MEMORY_BASIC_INFORMATION memoryBasicInfo;
    ULONG remainingBytes;
    ULONG matchingBytes;
    LARGE_INTEGER offset;
    PUCHAR bytes;
    ULONG bytesToRead;
    IO_STATUS_BLOCK iosb;
    PUCHAR inprocBytes;

    PAGED_PASSIVE();

    status = STATUS_SUCCESS;
    textSection = NULL;
    matchingBytes = 0;
    bytes = NULL;

    for (WORD i = 0; i < NumberOfSections; i++)
    {
        if (strcmp(SectionHeaders[i].Name, ".text") == 0)
        {
            if (!textSection)
            {
                textSection = &SectionHeaders[i];
            }
            else
            {
                status = STATUS_ACCESS_DENIED;
                goto CleanupExit;
            }
        }
    }

    if (!textSection)
    {
        status = STATUS_ACCESS_DENIED;
        goto CleanupExit;
    }

    //
    // Account for any 0 fill by taking the min.
    //
    inspectBytes = min(textSection->Misc.VirtualSize, textSection->SizeOfRawData);

    if (inspectBytes == 0)
    {
        status = STATUS_ACCESS_DENIED;
        goto CleanupExit;
    }

    inprocBytes = PTR_ADD_OFFSET(BaseAddress, textSection->VirtualAddress);

    if (!NT_SUCCESS(status = ZwQueryVirtualMemory(
        ZwCurrentProcess(),
        inprocBytes,
        MemoryBasicInformation,
        &memoryBasicInfo,
        sizeof(MEMORY_BASIC_INFORMATION),
        NULL
        )))
    {
        goto CleanupExit;
    }

    //
    // Sanity check that the .text section hasn't been screwed with.
    //
    if ((memoryBasicInfo.RegionSize < inspectBytes) ||
        (memoryBasicInfo.AllocationProtect != PAGE_EXECUTE_WRITECOPY) ||
        (memoryBasicInfo.Protect != PAGE_EXECUTE_READ) ||
        (memoryBasicInfo.Type != MEM_IMAGE) ||
        (memoryBasicInfo.State != MEM_COMMIT))

    {
        status = STATUS_ACCESS_DENIED;
        goto CleanupExit;
    }

    if (!(bytes = ExAllocatePoolZero(PagedPool, FILE_BUFFER_SIZE, 'pvpK')))
    {
        status = STATUS_INSUFFICIENT_RESOURCES;
        goto CleanupExit;
    }

    remainingBytes = inspectBytes;
    offset.QuadPart = textSection->PointerToRawData;

    while (remainingBytes != 0)
    {
        bytesToRead = FILE_BUFFER_SIZE;

        if (bytesToRead > remainingBytes)
            bytesToRead = remainingBytes;

        if (!NT_SUCCESS(status = ZwReadFile(
            FileHandle,
            NULL,
            NULL,
            NULL,
            &iosb,
            bytes,
            bytesToRead,
            &offset,
            NULL
            )))
        {
            goto CleanupExit;
        }

        if (iosb.Information != bytesToRead)
        {
            status = STATUS_ACCESS_DENIED;
            goto CleanupExit;
        }

        __try
        {
            ProbeForRead(inprocBytes, bytesToRead, 1);

            for (ULONG i = 0; i < bytesToRead; i++)
            {
                if (inprocBytes[i] == bytes[i])
                {
                    matchingBytes++;
                }
            }
        }
        __except (EXCEPTION_EXECUTE_HANDLER)
        {
            status = STATUS_ACCESS_DENIED;
            goto CleanupExit;
        }

        remainingBytes -= bytesToRead;
        offset.QuadPart += bytesToRead;
        inprocBytes = PTR_ADD_OFFSET(inprocBytes, bytesToRead);
    }

    //
    // Check coherency
    //
    if ((matchingBytes <= inspectBytes) &&
        ((inspectBytes - matchingBytes) <= (inspectBytes / PROC_VERIFY_COHERENCY_VALUE)))
    {
        TrustedExtents->BaseAddress = PTR_ADD_OFFSET(BaseAddress, textSection->VirtualAddress);
        TrustedExtents->EndAddress = PTR_ADD_OFFSET(TrustedExtents->BaseAddress, inspectBytes);
        status = STATUS_SUCCESS;
    }
    else
    {
        status = STATUS_ACCESS_DENIED;
    }

CleanupExit:
    if (bytes)
    {
        ExFreePoolWithTag(bytes, 'pvpK');
    }

    return status;
}

/**
 * Verifies the process.
 *
 * \details This function assumes that the process being inspected is the
 * currently attached process.
 *
 * \param[in] ProcessFileName - File name of the image backing the process.
 * \param[out] TrustedExtents - On success, set to the validated module extents.
*/
_IRQL_requires_max_(PASSIVE_LEVEL)
_Must_inspect_result_
NTSTATUS KpiVerifyProcess(
    _In_ PUNICODE_STRING ProcessFileName,
    _Out_ PKPH_EXTENTS TrustedExtents
    )
{
    NTSTATUS status;
    PVOID baseAddress;
    MEMORY_REGION_INFORMATION memoryRegionInfo;
    PVOID inprocHeaders;
    IMAGE_NT_HEADERS processHeaders;
    MEMORY_BASIC_INFORMATION memoryBasicInfo;
    HANDLE fileHandle;
    OBJECT_ATTRIBUTES objectAttributes;
    IO_STATUS_BLOCK iosb;
    IMAGE_DOS_HEADER fileDosHeader;
    IMAGE_NT_HEADERS fileHeaders;
    LARGE_INTEGER offset;
    ULONG sizeOfSectionHeaders;
    PIMAGE_SECTION_HEADER sectionHeaders;

    PAGED_PASSIVE();

    fileHandle = NULL;
    sectionHeaders = NULL;

    TrustedExtents->BaseAddress = NULL;
    TrustedExtents->EndAddress = NULL;

    if (!(baseAddress = PsGetProcessSectionBaseAddress(PsGetCurrentProcess())))
    {
        status = STATUS_ACCESS_DENIED;
        goto CleanupExit;
    }

    if (!NT_SUCCESS(status = ZwQueryVirtualMemory(
        ZwCurrentProcess(),
        baseAddress,
        MemoryRegionInformation,
        &memoryRegionInfo,
        sizeof(MEMORY_REGION_INFORMATION),
        NULL
        )))
    {
        goto CleanupExit;
    }

    __try
    {
        status = KphImageNtHeader(
            baseAddress,
            memoryRegionInfo.RegionSize,
            (PIMAGE_NT_HEADERS*)&inprocHeaders
            );

        if (!NT_SUCCESS(status))
            goto CleanupExit;

        ProbeForRead(inprocHeaders, sizeof(IMAGE_NT_HEADERS), 1);
        memcpy(&processHeaders, inprocHeaders, sizeof(IMAGE_NT_HEADERS));
    }
    __except (EXCEPTION_EXECUTE_HANDLER)
    {
        status = STATUS_ACCESS_DENIED;
        goto CleanupExit;
    }

    if (!NT_SUCCESS(status = ZwQueryVirtualMemory(
        ZwCurrentProcess(),
        inprocHeaders,
        MemoryBasicInformation,
        &memoryBasicInfo,
        sizeof(MEMORY_BASIC_INFORMATION),
        NULL
        )))
    {
        goto CleanupExit;
    }

    if ((memoryBasicInfo.AllocationProtect != PAGE_EXECUTE_WRITECOPY) ||
        (memoryBasicInfo.Protect != PAGE_READONLY) ||
        (memoryBasicInfo.Type != MEM_IMAGE) ||
        (memoryBasicInfo.State != MEM_COMMIT))
    {
        status = STATUS_ACCESS_DENIED;
        goto CleanupExit;
    }

    InitializeObjectAttributes(
        &objectAttributes,
        ProcessFileName,
        OBJ_KERNEL_HANDLE,
        NULL,
        NULL
        );

    status = ZwCreateFile(
        &fileHandle,
        FILE_GENERIC_READ,
        &objectAttributes,
        &iosb,
        NULL,
        FILE_ATTRIBUTE_NORMAL,
        FILE_SHARE_READ,
        FILE_OPEN,
        FILE_NON_DIRECTORY_FILE | FILE_SYNCHRONOUS_IO_NONALERT,
        NULL,
        0
        );

    if (!NT_SUCCESS(status))
    {
        fileHandle = NULL;
        goto CleanupExit;
    }

    status = ZwReadFile(
        fileHandle,
        NULL,
        NULL,
        NULL,
        &iosb,
        &fileDosHeader,
        sizeof(IMAGE_DOS_HEADER),
        NULL,
        NULL
        );

    if (!NT_SUCCESS(status))
        goto CleanupExit;

    if (iosb.Information < sizeof(IMAGE_DOS_HEADER))
    {
        status = STATUS_ACCESS_DENIED;
        goto CleanupExit;
    }

    offset.QuadPart = fileDosHeader.e_lfanew;
    status = ZwReadFile(
        fileHandle,
        NULL,
        NULL,
        NULL,
        &iosb,
        &fileHeaders,
        sizeof(fileHeaders),
        &offset,
        NULL
        );

    if (!NT_SUCCESS(status))
        goto CleanupExit;

    if (iosb.Information < sizeof(fileHeaders))
    {
        status = STATUS_ACCESS_DENIED;
        goto CleanupExit;
    }

    //
    // These checks are rudimentary but cheap to check.
    //
    if ((fileHeaders.Signature != processHeaders.Signature) ||
        (memcmp(&fileHeaders.FileHeader, &processHeaders.FileHeader, sizeof(IMAGE_FILE_HEADER)) != 0) ||
        (fileHeaders.OptionalHeader.AddressOfEntryPoint != processHeaders.OptionalHeader.AddressOfEntryPoint) ||
        (fileHeaders.OptionalHeader.BaseOfCode != processHeaders.OptionalHeader.BaseOfCode) ||
        (fileHeaders.OptionalHeader.CheckSum != processHeaders.OptionalHeader.CheckSum) ||
        (fileHeaders.OptionalHeader.Magic != processHeaders.OptionalHeader.Magic) ||
        (fileHeaders.OptionalHeader.SizeOfCode != processHeaders.OptionalHeader.SizeOfCode) ||
        (fileHeaders.OptionalHeader.SizeOfImage != processHeaders.OptionalHeader.SizeOfImage) ||
        (fileHeaders.OptionalHeader.SizeOfUninitializedData != processHeaders.OptionalHeader.SizeOfUninitializedData) ||
        (fileHeaders.OptionalHeader.SizeOfInitializedData != processHeaders.OptionalHeader.SizeOfInitializedData) ||
        (fileHeaders.OptionalHeader.SizeOfHeaders != processHeaders.OptionalHeader.SizeOfHeaders) ||
        (fileHeaders.OptionalHeader.SizeOfImage != processHeaders.OptionalHeader.SizeOfImage) ||
        (fileHeaders.OptionalHeader.Subsystem != processHeaders.OptionalHeader.Subsystem) ||
        (fileHeaders.OptionalHeader.DllCharacteristics != processHeaders.OptionalHeader.DllCharacteristics) ||
        (fileHeaders.OptionalHeader.LoaderFlags != processHeaders.OptionalHeader.LoaderFlags))
    {
        status = STATUS_ACCESS_DENIED;
        goto CleanupExit;
    }

    sizeOfSectionHeaders = sizeof(IMAGE_SECTION_HEADER) * fileHeaders.FileHeader.NumberOfSections;
    if (sizeOfSectionHeaders == 0)
    {
        status = STATUS_ACCESS_DENIED;
        goto CleanupExit;
    }

    if (!(sectionHeaders = ExAllocatePoolZero(PagedPool, sizeOfSectionHeaders, 'pvpK')))
    {
        status = STATUS_INSUFFICIENT_RESOURCES;
        goto CleanupExit;
    }

    offset.QuadPart = fileDosHeader.e_lfanew;
    offset.QuadPart += FIELD_OFFSET(IMAGE_NT_HEADERS, OptionalHeader);
    offset.QuadPart += fileHeaders.FileHeader.SizeOfOptionalHeader;

    status = ZwReadFile(
        fileHandle,
        NULL,
        NULL,
        NULL,
        &iosb,
        sectionHeaders,
        sizeOfSectionHeaders,
        &offset,
        NULL
        );

    if (!NT_SUCCESS(status))
        goto CleanupExit;

    if (iosb.Information != sizeOfSectionHeaders)
    {
        status = STATUS_ACCESS_DENIED;
        goto CleanupExit;
    }

    status = KpiVerifyProcessSections(
        baseAddress,
        fileHandle,
        sectionHeaders,
        fileHeaders.FileHeader.NumberOfSections,
        TrustedExtents
        );

CleanupExit:

    if (sectionHeaders)
    {
        ExFreePoolWithTag(sectionHeaders, 'pvpK');
    }

    if (fileHandle)
    {
        ObCloseHandle(fileHandle, KernelMode);
    }

    return status;
}

VOID KphVerifyClient(
    _Inout_ PKPH_CLIENT Client,
    _In_ PVOID CodeAddress,
    _In_reads_bytes_(SignatureSize) PUCHAR Signature,
    _In_ ULONG SignatureSize
    )
{
    NTSTATUS status;
    PUNICODE_STRING processFileName = NULL;
    MEMORY_BASIC_INFORMATION memoryBasicInfo;
    PUNICODE_STRING mappedFileName = NULL;
    KPH_EXTENTS processExtents;

    PAGED_CODE();

    if (Client->VerificationPerformed)
        return;

    if ((ULONG_PTR)CodeAddress > (ULONG_PTR)MmHighestUserAddress)
    {
        status = STATUS_ACCESS_VIOLATION;
        goto CleanupExit;
    }

    if (!NT_SUCCESS(status = SeLocateProcessImageName(PsGetCurrentProcess(), &processFileName)))
        goto CleanupExit;

    if (!NT_SUCCESS(status = ZwQueryVirtualMemory(
        NtCurrentProcess(),
        CodeAddress,
        MemoryBasicInformation,
        &memoryBasicInfo,
        sizeof(MEMORY_BASIC_INFORMATION),
        NULL
        )))
    {
        goto CleanupExit;
    }

    if (memoryBasicInfo.Type != MEM_IMAGE || memoryBasicInfo.State != MEM_COMMIT)
    {
        status = STATUS_INVALID_PARAMETER;
        goto CleanupExit;
    }

    if (!NT_SUCCESS(status = KphGetProcessMappedFileName(NtCurrentProcess(), CodeAddress, &mappedFileName)))
        goto CleanupExit;

    if (!RtlEqualUnicodeString(processFileName, mappedFileName, TRUE))
    {
        status = STATUS_INVALID_PARAMETER;
        goto CleanupExit;
    }

    if (!NT_SUCCESS(status = KphVerifyFile(processFileName, Signature, SignatureSize)))
        goto CleanupExit;

    if (!NT_SUCCESS(status = KpiVerifyProcess(processFileName, &processExtents)))
        goto CleanupExit;

    if ((CodeAddress < processExtents.BaseAddress) ||
        (CodeAddress > processExtents.EndAddress))
    {
        status = STATUS_ACCESS_DENIED;
    }

CleanupExit:
    if (mappedFileName)
        ExFreePoolWithTag(mappedFileName, 'ThpK');
    if (processFileName)
        ExFreePool(processFileName);

    ExAcquireFastMutex(&Client->StateMutex);

    if (NT_SUCCESS(status))
    {
        Client->VerifiedProcess = PsGetCurrentProcess();
        Client->VerifiedProcessId = PsGetCurrentProcessId();
        Client->VerifiedRangeBase = memoryBasicInfo.BaseAddress;
        Client->VerifiedRangeSize = memoryBasicInfo.RegionSize;
        Client->VerifiedProcessExtents = processExtents;
    }

    Client->VerificationStatus = status;
    MemoryBarrier();
    Client->VerificationSucceeded = NT_SUCCESS(status);
    Client->VerificationPerformed = TRUE;

    ExReleaseFastMutex(&Client->StateMutex);
}

NTSTATUS KpiVerifyClient(
    _In_ PVOID CodeAddress,
    _In_reads_bytes_(SignatureSize) PUCHAR Signature,
    _In_ ULONG SignatureSize,
    _In_ PKPH_CLIENT Client
    )
{
    PUCHAR signature;

    PAGED_CODE();

    __try
    {
        ProbeForRead(Signature, SignatureSize, 1);
    }
    __except (EXCEPTION_EXECUTE_HANDLER)
    {
        return GetExceptionCode();
    }

    if (SignatureSize > KPH_SIGNATURE_MAX_SIZE)
        return STATUS_INVALID_PARAMETER_3;

    if (!(signature = ExAllocatePoolZero(PagedPool, SignatureSize, 'ThpK')))
        return STATUS_INSUFFICIENT_RESOURCES;

    __try
    {
        memcpy(signature, Signature, SignatureSize);
    }
    __except (EXCEPTION_EXECUTE_HANDLER)
    {
        ExFreePoolWithTag(signature, 'ThpK');
        return GetExceptionCode();
    }

    KphVerifyClient(Client, CodeAddress, signature, SignatureSize);

    ExFreePoolWithTag(signature, 'ThpK');

    return Client->VerificationStatus;
}

VOID KphGenerateKeysClient(
    _Inout_ PKPH_CLIENT Client
    )
{
    ULONGLONG interruptTime;
    ULONG seed;
    KPH_KEY l1Key;
    KPH_KEY l2Key;

    PAGED_CODE();

    if (Client->KeysGenerated)
        return;

    interruptTime = KeQueryInterruptTime();
    seed = (ULONG)(interruptTime >> 32) | (ULONG)interruptTime | PtrToUlong(Client);
    l1Key = RtlRandomEx(&seed) | 0x80000000; // Make sure the key is nonzero
    do
    {
        l2Key = RtlRandomEx(&seed) | 0x80000000;
    } while (l2Key == l1Key);

    ExAcquireFastMutex(&Client->StateMutex);

    if (!Client->KeysGenerated)
    {
        Client->L1Key = l1Key;
        Client->L2Key = l2Key;
        MemoryBarrier();
        Client->KeysGenerated = TRUE;
    }

    ExReleaseFastMutex(&Client->StateMutex);
}

NTSTATUS KphRetrieveKeyViaApc(
    _Inout_ PKPH_CLIENT Client,
    _In_ KPH_KEY_LEVEL KeyLevel,
    _Inout_ PIRP Irp
    )
{
    PIO_APC_ROUTINE userApcRoutine;
    KPH_KEY key;

    PAGED_CODE();

    if (!Client->VerificationSucceeded)
        return STATUS_ACCESS_DENIED;

    MemoryBarrier();

    if (PsGetCurrentProcess() != Client->VerifiedProcess ||
        PsGetCurrentProcessId() != Client->VerifiedProcessId)
    {
        return STATUS_ACCESS_DENIED;
    }

    if (!(userApcRoutine = Irp->Overlay.AsynchronousParameters.UserApcRoutine))
        return STATUS_INVALID_PARAMETER;

    if ((ULONG_PTR)userApcRoutine < (ULONG_PTR)Client->VerifiedRangeBase ||
        (ULONG_PTR)userApcRoutine >= (ULONG_PTR)Client->VerifiedRangeBase + Client->VerifiedRangeSize)
    {
        return STATUS_ACCESS_DENIED;
    }

    KphGenerateKeysClient(Client);

    switch (KeyLevel)
    {
    case KphKeyLevel1:
        key = Client->L1Key;
        break;
    case KphKeyLevel2:
        key = Client->L2Key;
        break;
    default:
        return STATUS_INVALID_PARAMETER;
    }

    Irp->Overlay.AsynchronousParameters.UserApcContext = UlongToPtr(key);

    return STATUS_SUCCESS;
}

NTSTATUS KphValidateKey(
    _In_ KPH_KEY_LEVEL RequiredKeyLevel,
    _In_opt_ KPH_KEY Key,
    _In_ PKPH_CLIENT Client,
    _In_ KPROCESSOR_MODE AccessMode
    )
{
    PAGED_CODE();

    if (AccessMode == KernelMode)
        return STATUS_SUCCESS;

    if (Key && Client->VerificationSucceeded && Client->KeysGenerated)
    {
        MemoryBarrier();

        switch (RequiredKeyLevel)
        {
        case KphKeyLevel1:
            if (Key == Client->L1Key || Key == Client->L2Key)
                return STATUS_SUCCESS;
            else
                KphpBackoffKey(Client);
            break;
        case KphKeyLevel2:
            if (Key == Client->L2Key)
                return STATUS_SUCCESS;
            else
                KphpBackoffKey(Client);
            break;
        default:
            return STATUS_INVALID_PARAMETER;
        }
    }

    return STATUS_ACCESS_DENIED;
}

VOID KphpBackoffKey(
    _In_ PKPH_CLIENT Client
    )
{
    LARGE_INTEGER backoffTime;

    PAGED_CODE();

    // Serialize to make it impossible for a single client to bypass the backoff by creating
    // multiple threads.
    ExAcquireFastMutex(&Client->KeyBackoffMutex);

    backoffTime.QuadPart = -KPH_KEY_BACKOFF_TIME;
    KeDelayExecutionThread(KernelMode, FALSE, &backoffTime);

    ExReleaseFastMutex(&Client->KeyBackoffMutex);
}

/**
 * Performs a domination check between a calling process and a target process.
 *
 * \details A process dominates the other when the protected level of the
 * process exceeds the other. This domination check is not ideal, it is overly
 * strict and lacks enough information from the kernel to fully understand the
 * protected process state.
 *
 * \param[in] Client - Client information.
 * \param[in] Process - Calling process.
 * \param[in] ProcessTarget - Target process to check that the calling process
 * dominates.
 * \param[in] AccessMode - Access mode of the request, if KernelMode the
 * domination check is bypassed.
 *
 * \return Appropriate status:
 * STATUS_SUCCESS - The calling process dominates the target.
 * STATUS_ACCESS_DEINED - The calling process does not dominate the target.
*/
_IRQL_requires_max_(APC_LEVEL)
_Must_inspect_result_
NTSTATUS KphDominationCheck(
    _In_ PKPH_CLIENT Client,
    _In_ PEPROCESS Process,
    _In_ PEPROCESS ProcessTarget,
    _In_ KPROCESSOR_MODE AccessMode
    )
{
    PS_PROTECTION processProtection;
    PS_PROTECTION targetProtection;

    PAGED_CODE();

    if (AccessMode == KernelMode)
    {
        //
        // Give the kernel what it wants...
        //
        return STATUS_SUCCESS;
    }

    if (!Client->VerificationSucceeded ||
        (Client->VerifiedProcess != Process))
    {
        //
        // The requesting process is not the verified one, deny access...
        //
        return STATUS_ACCESS_DENIED;
    }

    //
    // Until Microsoft gives us more insight into protected process domination
    // we'll do a very strict check here:
    //

    if (NT_SUCCESS(KpiGetProcessProtection(Process, &processProtection)) &&
        NT_SUCCESS(KpiGetProcessProtection(ProcessTarget, &targetProtection)) &&
        (targetProtection.Type != PsProtectedTypeNone) &&
        (targetProtection.Type >= processProtection.Type))
    {
        //
        // Calling process protection does not dominate the other, deny access.
        // We could do our own domination check/mapping here with the signing
        // level, but it won't be great and Microsoft might change it, so we'll
        // do this strict check until Microsoft exports:
        // PsTestProtectedProcessIncompatibility
        // RtlProtectedAccess/RtlTestProtectedAccess
        //
        return STATUS_ACCESS_DENIED;
    }

    //
    // Either the protected process check is not exported or the verified
    // process dominates the target. Our domination check succeeded.
    //

    return STATUS_SUCCESS;
}

/**
 * Verifies the client trusted extents by doing a sanity check on the
 * validated region.
 *
 * \param[in] Client - Client information.
 *
 * \return Appropriate status.
*/
_IRQL_requires_max_(PASSIVE_LEVEL)
_Must_inspect_result_
NTSTATUS KpiVerifyCallerTrustedExtents(
    _In_ PKPH_CLIENT Client
   )
{
    NTSTATUS status;
    BOOLEAN unstackDetach;
    KAPC_STATE apcState;
    MEMORY_BASIC_INFORMATION memoryBasicInfo;
    SIZE_T trustedRegionSize;

    PAGED_PASSIVE();

    unstackDetach = FALSE;

    if (!Client->VerificationSucceeded ||
        !Client->VerifiedProcessExtents.BaseAddress ||
        !Client->VerifiedProcessExtents.EndAddress)
    {
        //
        // Not enough info to validate.
        //
        status = STATUS_ACCESS_DENIED;
        goto CleanupExit;
    }

    if (Client->VerifiedProcess != PsGetCurrentProcess())
    {
        KeStackAttachProcess(Client->VerifiedProcess, &apcState);
        unstackDetach = TRUE;
    }

    if (!NT_SUCCESS(status = ZwQueryVirtualMemory(
        ZwCurrentProcess(),
        Client->VerifiedProcessExtents.BaseAddress,
        MemoryBasicInformation,
        &memoryBasicInfo,
        sizeof(MEMORY_BASIC_INFORMATION),
        NULL
        )))
    {
        goto CleanupExit;
    }

    trustedRegionSize = (SIZE_T)PTR_SUB_OFFSET(Client->VerifiedProcessExtents.EndAddress, Client->VerifiedProcessExtents.BaseAddress);

    if ((trustedRegionSize > memoryBasicInfo.RegionSize) ||
        (memoryBasicInfo.AllocationProtect != PAGE_EXECUTE_WRITECOPY) ||
        (memoryBasicInfo.Protect != PAGE_EXECUTE_READ) ||
        (memoryBasicInfo.Type != MEM_IMAGE))
    {
        status = STATUS_ACCESS_DENIED;
    }
    else
    {
        status = STATUS_SUCCESS;
    }

CleanupExit:
    if (unstackDetach)
    {
        KeUnstackDetachProcess(&apcState);
    }

    return status;
}

/**
 * Verifies the caller.
 *
 * \param[in] Client - Client to validate.
 * \param[in] Thread - Calling thread.
 * \param[in] AccessMode - Access mode of the request, if KernelMode the
 * domination check is bypassed.
 *
 * \return Appropriate status, error means the caller is invalid or could not
 * be validated, a success means the caller is valid.
*/
_IRQL_requires_max_(PASSIVE_LEVEL)
_Must_inspect_result_
NTSTATUS KphVerifyCaller(
    _In_ PKPH_CLIENT Client,
    _In_ PETHREAD Thread,
    _In_ KPROCESSOR_MODE AccessMode
    )
{
    NTSTATUS status;
    PVOID frames[CALLER_CHECK_MAX_FRAMES];
    ULONG numberOfFrames;
    ULONG i;

    PAGED_PASSIVE();

    if (AccessMode == KernelMode)
    {
        //
        // Give the kernel what it wants...
        //
        return STATUS_SUCCESS;
    }

    if (!Client->VerificationSucceeded ||
        !NtdllExtents.BaseAddress ||
        !NtdllExtents.EndAddress ||
        !Client->VerifiedProcessExtents.BaseAddress ||
        !Client->VerifiedProcessExtents.EndAddress)
    {
        //
        // Not enough info to validate.
        //
        return STATUS_ACCESS_DENIED;
    }

    //
    // Check that the trusted extents are still valid.
    //
    status = KpiVerifyCallerTrustedExtents(Client);

    if (!NT_SUCCESS(status))
        return status;

    //
    // Here we will validate the caller by walking the stack.
    //
    status = KphCaptureStackBackTraceThread(
        Thread,
        0,
        CALLER_CHECK_MAX_FRAMES,
        frames,
        &numberOfFrames,
        NULL,
        KernelMode,
        KPH_STACK_TRACE_CAPTURE_USER_STACK
        );

    if (!NT_SUCCESS(status))
        return STATUS_ACCESS_DENIED;

    i = 0;

    //
    // Walk past the kernel frames
    //
    for (; i < numberOfFrames; i++)
    {
        if (frames[i] < MmHighestUserAddress)
        {
            break;
        }
    }

    //
    // Walk past ntdll
    //
    for (; i < numberOfFrames; i++)
    {
        if ((frames[i] < NtdllExtents.BaseAddress) ||
            (frames[i] > NtdllExtents.EndAddress))
        {
            break;
        }
    }

    if (i >= numberOfFrames)
    {
        return STATUS_ACCESS_DENIED;
    }

    //
    // At this point we have the first frame before the call into ntdll.
    // Process hacker will always make a native call rather than calling
    // through KernelBase so we only need to walk past ntdll, if we see
    // KernelBase it isn't process hacker.
    // Verify that this frame lands in the verified client extents.
    // This isn't perfect but ensures the call was at least made through the
    // primary image.
    //
    if ((frames[i] < Client->VerifiedProcessExtents.BaseAddress) ||
        (frames[i] > Client->VerifiedProcessExtents.EndAddress))
    {
        return STATUS_ACCESS_DENIED;
    }

    return STATUS_SUCCESS;
}
