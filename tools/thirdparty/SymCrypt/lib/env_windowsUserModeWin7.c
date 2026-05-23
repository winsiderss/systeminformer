//
// env_windowsUserMode.c
// Platform-specific code for windows user mode.
//
// Copyright (c) Microsoft Corporation. Licensed under the MIT license.
//

// #include "precomp.h"

#pragma warning(push)
#pragma warning(disable: 5103) // Arm64's wdm.h included below currently generate a lot of 5103 warnings
#include <Windows.h>
#pragma warning(pop)

#include "..\inc\symcrypt.h"
#include "sc_lib.h"

SYMCRYPT_CPU_FEATURES SYMCRYPT_CALL SymCryptCpuFeaturesNeverPresentEnvWindowsUsermodeWin7nLater()
{
#if SYMCRYPT_CPU_X86 | SYMCRYPT_CPU_AMD64

    //
    // Until Windows 8.1 there isn't a good way to find out if AVX2 is supported.
    // The RtlGetEnabledExtendedFeatures function isn't documented, and cannot be used outside of Windows code.
    // (And SymCrypt can be used by things like Office.)
    // GetEnabledXStateFeatures exists since Win7 SP1, but we can't make distinctions based on service packs. 
    // (We'd create a binary that cannot load on Win7 RTM if we used that function.)
    // We can't use GetEnabledXstateFeatures in Win8 because it isn't in an APIset, but exported from Kernel32.dll
    // which creates a dependency on a DLL that is absent in some of our SKUs.
    // As AVX2 is only used in parallel hashing, it isn't worth the effort to use it in earlier Windows versions.
    // We only use AVX2 if the binary is targeted at Win8.1 and later.
    //
    return SYMCRYPT_CPU_FEATURE_AVX2;
#endif

    return 0;
}

VOID
SYMCRYPT_CALL
SymCryptInitEnvWindowsUsermodeWin7nLater( UINT32 version )
{

    if( g_SymCryptFlags & SYMCRYPT_FLAG_LIB_INITIALIZED )
    {
        return;
    }

#if SYMCRYPT_CPU_X86 | SYMCRYPT_CPU_AMD64 
    //
    // First we detect what the CPU has
    // 
    SymCryptDetectCpuFeaturesByCpuid( SYMCRYPT_CPUID_DETECT_FLAG_CHECK_OS_SUPPORT_FOR_YMM );

    g_SymCryptCpuFeaturesNotPresent |= SYMCRYPT_CPU_FEATURE_AVX2;

    //
    // Our SaveXmm function never fails because it doesn't have to do anything in User mode.
    //
    g_SymCryptCpuFeaturesNotPresent &= ~SYMCRYPT_CPU_FEATURE_SAVEXMM_NOFAIL;

#elif SYMCRYPT_CPU_ARM 

    g_SymCryptCpuFeaturesNotPresent = (SYMCRYPT_CPU_FEATURES) ~SYMCRYPT_CPU_FEATURE_NEON;

#elif SYMCRYPT_CPU_ARM64

    SymCryptDetectCpuFeaturesFromIsProcessorFeaturePresent();

#endif    

    SymCryptInitEnvCommon( version );
}

_Analysis_noreturn_
VOID 
SYMCRYPT_CALL 
SymCryptFatalEnvWindowsUsermodeWin7nLater( UINT32 fatalCode )
{
    UINT32 fatalCodeVar;

    SymCryptFatalIntercept( fatalCode );

    //
    // Put the fatal code in a location where it shows up in the dump
    //
    SYMCRYPT_FORCE_WRITE32( &fatalCodeVar, fatalCode );

    //
    // Our first preference is to fastfail,
    // the second to create an AV, which triggers a Watson report so that we get to 
    // see what is going wrong.
    //
    __fastfail( FAST_FAIL_CRYPTO_LIBRARY );

    //
    // Next we write to the NULL pointer, this causes an AV
    //
    SYMCRYPT_FORCE_WRITE32( (volatile UINT32 *)NULL, fatalCode );

    //
    // If that fails, we terminate the process. (This function call also ensures that this environment is actually
    // used in user mode and not some other environment.)
    // (During testing we had the TerminateProcess as the first option, but that makes debugging very hard as 
    // it leaves no traces of what went wrong.)
    //
    TerminateProcess( GetCurrentProcess(), fatalCode );

    SymCryptFatalHang( fatalCode );
}

#if SYMCRYPT_CPU_AMD64 | SYMCRYPT_CPU_X86

SYMCRYPT_ERROR 
SYMCRYPT_CALL 
SymCryptSaveXmmEnvWindowsUsermodeWin7nLater( _Out_ PSYMCRYPT_EXTENDED_SAVE_DATA pSaveArea ) 
{
    //
    // In usermode there is no need to save XMM registers.
    // The compiler should inline this function and optimize it away.
    //

    UNREFERENCED_PARAMETER( pSaveArea );

    return SYMCRYPT_NO_ERROR;
}

VOID 
SYMCRYPT_CALL 
SymCryptRestoreXmmEnvWindowsUsermodeWin7nLater( _Inout_ PSYMCRYPT_EXTENDED_SAVE_DATA pSaveArea )
{
    //
    // In usermode there is no need to save XMM registers.
    // The compiler should inline this function and optimize it away.
    //

    UNREFERENCED_PARAMETER( pSaveArea );
}


SYMCRYPT_ERROR 
SYMCRYPT_CALL 
SymCryptSaveYmmEnvWindowsUsermodeWin7nLater( _Out_ PSYMCRYPT_EXTENDED_SAVE_DATA pSaveArea ) 
{
    //
    // In usermode there is no need to save XMM registers.
    // The compiler should inline this function and optimize it away.
    //

    UNREFERENCED_PARAMETER( pSaveArea );

    return SYMCRYPT_NO_ERROR;
}

VOID 
SYMCRYPT_CALL 
SymCryptRestoreYmmEnvWindowsUsermodeWin7nLater( _Inout_ PSYMCRYPT_EXTENDED_SAVE_DATA pSaveArea )
{
    //
    // In usermode there is no need to save XMM registers.
    // The compiler should inline this function and optimize it away.
    //

    UNREFERENCED_PARAMETER( pSaveArea );
}

#endif

VOID 
SYMCRYPT_CALL 
SymCryptTestInjectErrorEnvWindowsUsermodeWin7nLater( PBYTE pbBuf, SIZE_T cbBuf ) 
{
    //
    // This feature is only used during testing. In production it is always
    // an empty function that the compiler can optimize away.
    //
    UNREFERENCED_PARAMETER( pbBuf );
    UNREFERENCED_PARAMETER( cbBuf );
}

#if SYMCRYPT_CPU_AMD64 | SYMCRYPT_CPU_X86

VOID
SYMCRYPT_CALL
SymCryptCpuidExFuncEnvWindowsUsermodeWin7nLater( int cpuInfo[4], int function_id, int subfunction_id )
{
    __cpuidex( cpuInfo, function_id, subfunction_id );
}
 
#endif    
 
