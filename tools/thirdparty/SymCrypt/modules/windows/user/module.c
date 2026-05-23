//
// module.c
// Main file for SymCrypt Windows user-mode module, symcrypt.dll
//
// Copyright (c) Microsoft Corporation. Licensed under the MIT license.
//

// Ensure that windows.h doesn't re-define the status_* symbols
#define WIN32_NO_STATUS
#include <phbase.h>
#include <windows.h>
#include <windef.h>
#if SYMCRYPT_CPU_X86 | SYMCRYPT_CPU_AMD64
#include <intrin.h>
#endif
#include "../../../inc/symcrypt.h"
#include "../../../inc/symcrypt_low_level.h"
#include "../../symcrypt_modules_common.h"

SYMCRYPT_ENVIRONMENT_WINDOWS_USERMODE_LATEST;

#define SYMCRYPT_FIPS_STATUS_INDICATOR
#include "../../statusindicator_common.h"
#include "../../../lib/status_indicator.h"

EXTERN_C IMAGE_DOS_HEADER __ImageBase;

static SRWLOCK SymCryptProvidedEntropyLock = SRWLOCK_INIT;
static BYTE SymCryptProvidedEntropy[SYMCRYPT_SHA256_RESULT_SIZE];
static ULONG64 SymCryptProvidedEntropyCounter = 0;

VOID
PerformStartupAlgorithmSelftests()
{
    SymCryptRngAesInstantiateSelftest();
    SymCryptRngAesReseedSelftest();
    SymCryptRngAesGenerateSelftest();

    SymCrypt3DesSelftest();

    SymCryptAesSelftest( SYMCRYPT_AES_SELFTEST_ALL );
    SymCryptAesCmacSelftest();
    SymCryptCcmSelftest();
    SymCryptGcmSelftest();
    SymCryptXtsAesSelftest();

    SymCryptHmacSha1Selftest();
    SymCryptHmacSha256Selftest();
    SymCryptHmacSha384Selftest();
    SymCryptHmacSha512Selftest();

    SymCryptParallelSha256Selftest();
    SymCryptParallelSha512Selftest();

    SymCryptTlsPrf1_1SelfTest();
    SymCryptTlsPrf1_2SelfTest();

    SymCryptHkdfSelfTest();

    SymCryptSp800_108_HmacSha1SelfTest();
    SymCryptSp800_108_HmacSha256SelfTest();
    SymCryptSp800_108_HmacSha384SelfTest();
    SymCryptSp800_108_HmacSha512SelfTest();

    SymCryptPbkdf2_HmacSha1SelfTest();

    SymCryptSrtpKdfSelfTest();

    SymCryptSshKdfSha256SelfTest();
    SymCryptSshKdfSha512SelfTest();

    SymCryptSskdfSelfTest();

    SymCryptHmacSha3_256Selftest();

    g_SymCryptFipsSelftestsPerformed |= SYMCRYPT_SELFTEST_ALGORITHM_STARTUP;
}

//BOOL
//WINAPI
//DllMain(
//    _In_ HINSTANCE instance,
//    _In_ DWORD reason,
//    _In_ PVOID reserved)
//{
//    UNREFERENCED_PARAMETER( reserved );
//
//    HMODULE hDummy = NULL;
//
//    if( reason == DLL_PROCESS_ATTACH )
//    {
//        DisableThreadLibraryCalls(instance);
//
//        // Take a reference to our own module so that we can't be unloaded. We don't want to be unloaded
//        // and reloaded, because this would cause the FIPS selftests to run repeatedly, which is expensive.
//        // Being unloaded can also cause problems in VTL1. Failure here is not fatal, though.
//        GetModuleHandleExW( GET_MODULE_HANDLE_EX_FLAG_FROM_ADDRESS | GET_MODULE_HANDLE_EX_FLAG_PIN,
//            (LPCWSTR) &DllMain,
//            &hDummy );
//
//        SymCryptInit();
//
//        // TODO: We should only run these selftests once per boot, when the first process loads the DLL
//        if( SYMCRYPT_MODULE_DO_FIPS_SELFTESTS )
//        {
//            PerformStartupAlgorithmSelftests();
//        }
//    }
//
//    return TRUE;
//}

PVOID
SYMCRYPT_CALL
SymCryptCallbackAlloc( SIZE_T nBytes )
{
    return _aligned_malloc( nBytes, SYMCRYPT_ASYM_ALIGN_VALUE );
}

VOID
SYMCRYPT_CALL
SymCryptCallbackFree(PVOID ptr)
{
    _aligned_free( ptr );
}

#if SYMCRYPT_CPU_X86 | SYMCRYPT_CPU_AMD64

#define SYMCRYPT_RDSEED_RETRY_COUNT 1024

#if defined(__clang__)
__attribute__((target("rdseed")))
#endif
static BOOLEAN SymCryptRdseedRandom(
    _Out_writes_bytes_(cbBuffer) PBYTE pbBuffer,
    _In_ SIZE_T cbBuffer
    )
{
    SIZE_T index;

    if (SymCryptRdseedStatus() != SYMCRYPT_NO_ERROR)
        return FALSE;

    for (index = 0; index < cbBuffer;)
    {
#if SYMCRYPT_CPU_AMD64
        ULONG64 seed = 0;
#else
        ULONG seed = 0;
#endif
        SIZE_T bytes;
        ULONG i;

        for (i = 0; i < SYMCRYPT_RDSEED_RETRY_COUNT; i++)
        {
#if defined(_M_X64)
            if (_rdseed64_step(&seed))
                break;
#else
            if (_rdseed32_step(&seed))
                break;
#endif

        }

        if (i == SYMCRYPT_RDSEED_RETRY_COUNT)
            return FALSE;

        bytes = min(cbBuffer - index, sizeof(seed));
        memcpy(&pbBuffer[index], &seed, bytes);
        SymCryptWipeKnownSize(&seed, sizeof(seed));

        index += bytes;
    }

    return TRUE;
}

#endif

static NTSTATUS SymCryptKsecDeviceRandom(
    _Out_writes_bytes_(cbBuffer) PVOID Buffer,
    _In_ SIZE_T BufferLength
    )
{
    static UNICODE_STRING deviceName = RTL_CONSTANT_STRING(KSEC_DEVICE_NAME);
    OBJECT_ATTRIBUTES objectAttributes;
    IO_STATUS_BLOCK ioStatusBlock;
    HANDLE deviceHandle;
    NTSTATUS status;

    if (BufferLength == 0)
        return STATUS_SUCCESS;

    if (BufferLength > ULONG_MAX)
        return STATUS_INVALID_BUFFER_SIZE;

    InitializeObjectAttributes(
        &objectAttributes,
        &deviceName,
        OBJ_CASE_INSENSITIVE,
        NULL,
        NULL
        );

    status = NtOpenFile(
        &deviceHandle,
        FILE_READ_DATA | FILE_WRITE_DATA | SYNCHRONIZE,
        &objectAttributes,
        &ioStatusBlock,
        FILE_SHARE_READ | FILE_SHARE_WRITE,
        FILE_SYNCHRONOUS_IO_NONALERT | FILE_NON_DIRECTORY_FILE
        );

    if (!NT_SUCCESS(status))
        return status;

    status = NtDeviceIoControlFile(
        deviceHandle,
        NULL,
        NULL,
        NULL,
        &ioStatusBlock,
        IOCTL_KSEC_RNG,
        NULL,
        0,
        Buffer,
        (ULONG)BufferLength
        );

    NtClose(deviceHandle);

    return status;
}

static NTSTATUS SymCryptSystemRandom(
    _Out_writes_bytes_(BufferLength) PVOID Buffer,
    _In_ SIZE_T BufferLength
    )
{
#if SYMCRYPT_CPU_X86 | SYMCRYPT_CPU_AMD64
    if (SymCryptRdseedRandom(Buffer, BufferLength))
        return STATUS_SUCCESS;
#endif

    return SymCryptKsecDeviceRandom(Buffer, BufferLength);
}

static VOID SymCryptMixProvidedEntropy(
    _Inout_updates_bytes_(BufferLength) PBYTE Buffer,
    _In_ SIZE_T BufferLength
    )
{
    BYTE digest[SYMCRYPT_SHA256_RESULT_SIZE];
    SYMCRYPT_SHA256_STATE hashState;
    SIZE_T index;

    if (BufferLength == 0)
        return;

    RtlAcquireSRWLockShared(&SymCryptProvidedEntropyLock);

    SymCryptSha256Init(&hashState);
    SymCryptSha256Append(&hashState, SymCryptProvidedEntropy, sizeof(SymCryptProvidedEntropy));
    SymCryptSha256Append(&hashState, (PBYTE)&SymCryptProvidedEntropyCounter, sizeof(SymCryptProvidedEntropyCounter));
    SymCryptSha256Result(&hashState, digest);

    RtlReleaseSRWLockShared(&SymCryptProvidedEntropyLock);

    for (index = 0; index < BufferLength; index++)
        Buffer[index] ^= digest[index % sizeof(digest)];

    SymCryptWipeKnownSize(digest, sizeof(digest));
}

VOID
SYMCRYPT_CALL
SymCryptProvideEntropy(
    _In_reads_(cbEntropy)   PCBYTE  pbEntropy,
                            SIZE_T  cbEntropy )
{
    BYTE secureEntropy[SYMCRYPT_SHA256_RESULT_SIZE];
    BYTE newEntropy[SYMCRYPT_SHA256_RESULT_SIZE];
    SYMCRYPT_SHA256_STATE hashState;

    if (!NT_SUCCESS(SymCryptSystemRandom(secureEntropy, sizeof(secureEntropy))))
        memset(secureEntropy, 0, sizeof(secureEntropy));

    RtlAcquireSRWLockExclusive(&SymCryptProvidedEntropyLock);

    SymCryptSha256Init(&hashState);
    SymCryptSha256Append(&hashState, SymCryptProvidedEntropy, sizeof(SymCryptProvidedEntropy));
    SymCryptSha256Append(&hashState, pbEntropy, cbEntropy);
    SymCryptSha256Append(&hashState, secureEntropy, sizeof(secureEntropy));
    SymCryptSha256Append(&hashState, (PBYTE)&SymCryptProvidedEntropyCounter, sizeof(SymCryptProvidedEntropyCounter));
    SymCryptSha256Result(&hashState, newEntropy);

    memcpy(SymCryptProvidedEntropy, newEntropy, sizeof(SymCryptProvidedEntropy));
    SymCryptProvidedEntropyCounter++;

    RtlReleaseSRWLockExclusive(&SymCryptProvidedEntropyLock);

    SymCryptWipeKnownSize(secureEntropy, sizeof(secureEntropy));
    SymCryptWipeKnownSize(newEntropy, sizeof(newEntropy));
}

VOID
SYMCRYPT_CALL
SymCryptRandom(
    _Out_writes_bytes_( cbBuffer )  PBYTE   pbBuffer,
                                    SIZE_T  cbBuffer )
{
    NTSTATUS status = SymCryptSystemRandom( pbBuffer, cbBuffer );

    if (!NT_SUCCESS(status))
    {
        SymCryptFatal(status);
    }

    SymCryptMixProvidedEntropy(pbBuffer, cbBuffer);
}

SYMCRYPT_ERROR
SYMCRYPT_CALL
SymCryptCallbackRandom(
    _Out_writes_bytes_( cbBuffer )  PBYTE   pbBuffer,
                                    SIZE_T  cbBuffer )
{
    NTSTATUS status = SymCryptSystemRandom( pbBuffer, cbBuffer );

    if (!NT_SUCCESS(status))
        return SYMCRYPT_EXTERNAL_FAILURE;

    SymCryptMixProvidedEntropy(pbBuffer, cbBuffer);

    return SYMCRYPT_NO_ERROR;
}

PVOID
SYMCRYPT_CALL
SymCryptCallbackAllocateMutexFastInproc()
{
    LPCRITICAL_SECTION lpCriticalSection = malloc( sizeof(CRITICAL_SECTION) );
    RtlInitializeCriticalSection(lpCriticalSection);
    return (PVOID)lpCriticalSection;
}

VOID
SYMCRYPT_CALL
SymCryptCallbackFreeMutexFastInproc( PVOID pMutex )
{
    LPCRITICAL_SECTION lpCriticalSection = (LPCRITICAL_SECTION)pMutex;
    RtlDeleteCriticalSection(lpCriticalSection);
    free(lpCriticalSection);
}

VOID
SYMCRYPT_CALL
SymCryptCallbackAcquireMutexFastInproc( PVOID pMutex )
{
    RtlEnterCriticalSection((LPCRITICAL_SECTION)pMutex);
}

VOID
SYMCRYPT_CALL
SymCryptCallbackReleaseMutexFastInproc( PVOID pMutex )
{
    RtlLeaveCriticalSection((LPCRITICAL_SECTION)pMutex);
}

VOID SYMCRYPT_CALL SymCryptModuleInit( UINT32 api, UINT32 minor )
{
    if (api != SYMCRYPT_CODE_VERSION_API ||
        (api == SYMCRYPT_CODE_VERSION_API && minor > SYMCRYPT_CODE_VERSION_MINOR) )
    {
        SymCryptFatal( 'vers' );
    }
}
