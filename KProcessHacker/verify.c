/*
 * KProcessHacker
 *
 * Copyright (C) 2016 wj32
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

#include <kph.h>

#define FILE_BUFFER_SIZE (2 * PAGE_SIZE)
#define FILE_MAX_SIZE (32 * 1024 * 1024) // 32 MB

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

#ifdef ALLOC_PRAGMA
#pragma alloc_text(PAGE, KphHashFile)
#pragma alloc_text(PAGE, KphVerifyFile)
#pragma alloc_text(PAGE, KphVerifyClient)
#pragma alloc_text(PAGE, KpiVerifyClient)
#pragma alloc_text(PAGE, KphGenerateKeysClient)
#pragma alloc_text(PAGE, KphRetrieveKeyViaApc)
#pragma alloc_text(PAGE, KphValidateKey)
#pragma alloc_text(PAGE, KphpBackoffKey)
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
    if (!NT_SUCCESS(status = BCryptGetProperty(hashAlgHandle, BCRYPT_OBJECT_LENGTH,
        (PUCHAR)&hashObjectSize, sizeof(ULONG), &querySize, 0)))
    {
        goto CleanupExit;
    }
    if (!NT_SUCCESS(status = BCryptGetProperty(hashAlgHandle, BCRYPT_HASH_LENGTH, (PUCHAR)&hashSize,
        sizeof(ULONG), &querySize, 0)))
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

    if (!NT_SUCCESS(status = BCryptCreateHash(hashAlgHandle, &hashHandle, hashObject, hashObjectSize,
        NULL, 0, 0)))
    {
        goto CleanupExit;
    }

    // Open the file and compute the hash.

    InitializeObjectAttributes(&objectAttributes, FileName, OBJ_KERNEL_HANDLE, NULL, NULL);

    if (!NT_SUCCESS(status = ZwCreateFile(&fileHandle, FILE_GENERIC_READ, &objectAttributes,
        &iosb, NULL, FILE_ATTRIBUTE_NORMAL, FILE_SHARE_READ, FILE_OPEN,
        FILE_NON_DIRECTORY_FILE | FILE_SYNCHRONOUS_IO_NONALERT, NULL, 0)))
    {
        goto CleanupExit;
    }

    if (!NT_SUCCESS(status = ZwQueryInformationFile(fileHandle, &iosb, &standardInfo,
        sizeof(FILE_STANDARD_INFORMATION), FileStandardInformation)))
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

        if (!NT_SUCCESS(status = ZwReadFile(fileHandle, NULL, NULL, NULL, &iosb, buffer, bytesToRead,
            NULL, NULL)))
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
    if (!NT_SUCCESS(status = BCryptImportKeyPair(signAlgHandle, NULL, KPH_BLOB_PUBLIC, &keyHandle,
        KphpTrustedPublicKey, sizeof(KphpTrustedPublicKey), 0)))
    {
        goto CleanupExit;
    }

    // Hash the file.

    if (!NT_SUCCESS(status = KphHashFile(FileName, &hash, &hashSize)))
        goto CleanupExit;

    // Verify the hash.

    if (!NT_SUCCESS(status = BCryptVerifySignature(keyHandle, NULL, hash, hashSize, Signature,
        SignatureSize, 0)))
    {
        goto CleanupExit;
    }

CleanupExit:
    if (hash)
        ExFreePoolWithTag(hash, 'vhpK');
    if (keyHandle)
        BCryptDestroyKey(keyHandle);
    if (signAlgHandle)
        BCryptCloseAlgorithmProvider(signAlgHandle, 0);

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
    if (!NT_SUCCESS(status = ZwQueryVirtualMemory(NtCurrentProcess(), CodeAddress,
        MemoryBasicInformation, &memoryBasicInfo, sizeof(MEMORY_BASIC_INFORMATION), NULL)))
    {
        goto CleanupExit;
    }
    if (memoryBasicInfo.Type != MEM_IMAGE || memoryBasicInfo.State != MEM_COMMIT)
    {
        status = STATUS_INVALID_PARAMETER;
        goto CleanupExit;
    }
    if (!NT_SUCCESS(status = KphGetProcessMappedFileName(NtCurrentProcess(), CodeAddress,
        &mappedFileName)))
    {
        goto CleanupExit;
    }
    if (!RtlEqualUnicodeString(processFileName, mappedFileName, TRUE))
    {
        status = STATUS_INVALID_PARAMETER;
        goto CleanupExit;
    }

    status = KphVerifyFile(processFileName, Signature, SignatureSize);

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

    signature = ExAllocatePoolZero(PagedPool, SignatureSize, 'ThpK');

    if (!signature)
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

    userApcRoutine = Irp->Overlay.AsynchronousParameters.UserApcRoutine;

    if (!userApcRoutine)
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
