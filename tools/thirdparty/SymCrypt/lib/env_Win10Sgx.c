//
// env_Win10Sgx.c
// Platform-specific code for windows 10 SGX user mode.
//
// Copyright (c) Microsoft Corporation. Licensed under the MIT license.
//

#include "precomp.h"

VOID SYMCRYPT_CALL SymCryptCpuidExCallback( int cpuInfo[4], int function_id, int subfunction_id );

SYMCRYPT_CPU_FEATURES SYMCRYPT_CALL SymCryptCpuFeaturesNeverPresentEnvWin10Sgx(void)
{
#if SYMCRYPT_CPU_X86 | SYMCRYPT_CPU_AMD64 
    return SYMCRYPT_CPU_FEATURE_AVX2;
#else
    return 0;
#endif    
}

_Analysis_noreturn_
VOID
SYMCRYPT_CALL
SymCryptFatalEnvWin10Sgx( UINT32 fatalCode )
{
    UINT32 fatalCodeVar;

    SymCryptFatalIntercept(fatalCode);

    //
    // Put the fatal code in a location where it shows up in the dump
    //
    SYMCRYPT_FORCE_WRITE32( &fatalCodeVar, fatalCode );

    //
    // Our first preference is to fastfail,
    // the second to create an AV, which triggers a Watson report so that we get to 
    // see what is going wrong.
    //
    __fastfail(FAST_FAIL_CRYPTO_LIBRARY);

    //
    // Next we write to the NULL pointer, this causes an AV
    //
    SYMCRYPT_FORCE_WRITE32( (volatile UINT32 *)NULL, fatalCode );

    //
    // TerminateProcess and GetCurrentProcess API are not supported inside the enclave, so 
    // similar to generic.env, we are directly invoking SymCryptFatalHang
    //

    SymCryptFatalHang( fatalCode );
}

VOID
SYMCRYPT_CALL
SymCryptInitEnvWin10Sgx( UINT32 version )
{
    if ( g_SymCryptFlags & SYMCRYPT_FLAG_LIB_INITIALIZED )
    {
        return;
    }

#if SYMCRYPT_CPU_X86 | SYMCRYPT_CPU_AMD64

    //
    // First we detect what the CPU has
    // 
    SymCryptDetectCpuFeaturesByCpuid( SYMCRYPT_CPUID_DETECT_FLAG_CHECK_OS_SUPPORT_FOR_YMM );

    // if AES-NI and SSSE3 are not present according to CPUID then treat it as error
    if ( !SYMCRYPT_CPU_FEATURES_PRESENT( SYMCRYPT_CPU_FEATURE_AESNI ) ||
         !SYMCRYPT_CPU_FEATURES_PRESENT( SYMCRYPT_CPU_FEATURE_SSSE3 ) )
    {
        SymCryptFatal( 'unsp' );
    }

    //
    // Our SaveXmm function never fails because it doesn't have to do anything in User mode.
    //
    g_SymCryptCpuFeaturesNotPresent &= ~SYMCRYPT_CPU_FEATURE_SAVEXMM_NOFAIL;

#elif SYMCRYPT_CPU_ARM | SYMCRYPT_CPU_ARM64

    //
    // This configuration should never be used in non-SGX architectures, so we force fail here.
    //
    SymCryptFatal( 'unsp' );

#endif

    SymCryptInitEnvCommon( version );
}

#if SYMCRYPT_CPU_AMD64 | SYMCRYPT_CPU_X86

SYMCRYPT_ERROR
SYMCRYPT_CALL
SymCryptSaveXmmEnvWin10Sgx(_Out_ PSYMCRYPT_EXTENDED_SAVE_DATA pSaveArea)
{
    //
    // In usermode there is no need to save XMM registers.
    // The compiler should inline this function and optimize it away.
    //

    UNREFERENCED_PARAMETER(pSaveArea);

    return SYMCRYPT_NO_ERROR;
}

VOID
SYMCRYPT_CALL
SymCryptRestoreXmmEnvWin10Sgx(_Inout_ PSYMCRYPT_EXTENDED_SAVE_DATA pSaveArea)
{
    //
    // In usermode there is no need to save XMM registers.
    // The compiler should inline this function and optimize it away.
    //

    UNREFERENCED_PARAMETER(pSaveArea);
}


SYMCRYPT_ERROR
SYMCRYPT_CALL
SymCryptSaveYmmEnvWin10Sgx(_Out_ PSYMCRYPT_EXTENDED_SAVE_DATA pSaveArea)
{
    //
    // In usermode there is no need to save XMM registers.
    // The compiler should inline this function and optimize it away.
    //

    UNREFERENCED_PARAMETER(pSaveArea);

    return SYMCRYPT_NO_ERROR;
}

VOID
SYMCRYPT_CALL
SymCryptRestoreYmmEnvWin10Sgx(_Inout_ PSYMCRYPT_EXTENDED_SAVE_DATA pSaveArea)
{
    //
    // In usermode there is no need to save XMM registers.
    // The compiler should inline this function and optimize it away.
    //

    UNREFERENCED_PARAMETER(pSaveArea);
}

#endif

VOID
SYMCRYPT_CALL
SymCryptTestInjectErrorEnvWin10Sgx(PBYTE pbBuf, SIZE_T cbBuf)
{
    //
    // This feature is only used during testing. In production it is always
    // an empty function that the compiler can optimize away.
    //
    UNREFERENCED_PARAMETER(pbBuf);
    UNREFERENCED_PARAMETER(cbBuf);
}

VOID
SYMCRYPT_CALL
SymCryptCpuidExFuncEnvWin10Sgx( int cpuInfo[4], int function_id, int subfunction_id )
{
    //
    // CPUID is an illegal instruction when running in an SGX enclave.
    // Hence we need to use sgx_cpuidex
    //
    // User of Win10Sgx environment should implement SymCryptCpuidExCallback such as
    // extern "C"
    // {
    //    VOID SYMCRYPT_CALL SymCryptCpuidExCallback(int cpuInfo[4], int function_id, int subfunction_id);
    // }
    // SYMCRYPT_ENVIRONMENT_WINDOWS_USERMODE_WIN10_SGX;
    // VOID
    // SYMCRYPT_CALL
    // SymCryptCpuidExCallback(int cpuInfo[4], int function_id, int subfunction_id)
    // {
    //    sgx_status_t sgxStatus = sgx_cpuidex( cpuInfo, function_id, subfunction_id );
    //    if (sgxStatus != SGX_SUCCESS)
    //    {
    //        SymCryptFatalEnvWin10Sgx('unsp');
    //    }
    // }
    //

    SymCryptCpuidExCallback( cpuInfo, function_id, subfunction_id );
}
