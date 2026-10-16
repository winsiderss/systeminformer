//
// env_elevatedDebugger.c
// Platform-specific code for debugger running in elevated but restricted environment.
//
// The debugger using this environment may run before any kernel is up-and-running.
// So we require there are no dependencies on the OS, but we want to detect and use AES
// instructions when possible. We ensure we only use SSE (AMD64) and ASIMD (Arm64) state,
// as this state is assumed to be saved/restored correctly by the environment.
//
// Copyright (c) Microsoft Corporation. Licensed under the MIT license.
//

#include "precomp.h"

SYMCRYPT_CPU_FEATURES SYMCRYPT_CALL SymCryptCpuFeaturesNeverPresentEnvElevatedDebugger()
{
#if SYMCRYPT_CPU_AMD64
    // We explicitly opt-out of using CPU features not relating to AES / SHA256 with SSE state
    // This ensures that new features requiring new state will not be used unintentionally
    return (SYMCRYPT_CPU_FEATURES) ~(SYMCRYPT_CPU_FEATURES_FOR_AESNI_PCLMULQDQ_CODE | SYMCRYPT_CPU_FEATURES_FOR_SHANI_CODE);
#elif SYMCRYPT_CPU_ARM64
    // We explicitly opt-out of using CPU features not relating to AES / SHA256 with NEON state
    // This ensures that new features requiring new state will not be used unintentionally
    return (SYMCRYPT_CPU_FEATURES) ~(SYMCRYPT_CPU_FEATURE_NEON | SYMCRYPT_CPU_FEATURE_NEON_AES | SYMCRYPT_CPU_FEATURE_NEON_PMULL | SYMCRYPT_CPU_FEATURE_NEON_SHA256);
#else
    #error We only support ARM64 and AMD64 for Elevated Debugger Environment
#endif
}

VOID
SYMCRYPT_CALL
SymCryptInitEnvElevatedDebugger( UINT32 version )
{
    if( g_SymCryptFlags & SYMCRYPT_FLAG_LIB_INITIALIZED )
    {
        return;
    }

#if SYMCRYPT_CPU_AMD64
    SymCryptDetectCpuFeaturesByCpuid( 0 );

    // Our SaveXmm function never fails because it doesn't have to do anything.
    g_SymCryptCpuFeaturesNotPresent &= ~SYMCRYPT_CPU_FEATURE_SAVEXMM_NOFAIL;

#elif SYMCRYPT_CPU_ARM64
    // We always run in a CPU mode that allows us to read the configuration registers
    SymCryptDetectCpuFeaturesFromRegistersNoTry();

#else
    #error We only support ARM64 and AMD64 for Elevated Debugger Environment
#endif

    SymCryptInitEnvCommon( version );
}

_Analysis_noreturn_
VOID
SYMCRYPT_CALL
SymCryptFatalEnvElevatedDebugger( UINT32 fatalCode )
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

    SymCryptFatalHang( fatalCode );
}

#if SYMCRYPT_CPU_AMD64

SYMCRYPT_ERROR
SYMCRYPT_CALL
SymCryptSaveXmmEnvElevatedDebugger( _Out_ PSYMCRYPT_EXTENDED_SAVE_DATA pSaveArea )
{
    //
    // In this environment there is no need to save XMM registers.
    // The compiler should inline this function and optimize it away.
    //
    UNREFERENCED_PARAMETER( pSaveArea );
    return SYMCRYPT_NO_ERROR;
}

VOID
SYMCRYPT_CALL
SymCryptRestoreXmmEnvElevatedDebugger( _Inout_ PSYMCRYPT_EXTENDED_SAVE_DATA pSaveArea )
{
    //
    // In this environment there is no need to save XMM registers.
    // The compiler should inline this function and optimize it away.
    //
    UNREFERENCED_PARAMETER( pSaveArea );
}

//
// These functions should never be called in the ElevatedDebugger environment because
// the corresponding CPU features are locked out.
//

SYMCRYPT_ERROR
SYMCRYPT_CALL
SymCryptSaveYmmEnvElevatedDebugger( _Out_ PSYMCRYPT_EXTENDED_SAVE_DATA pSaveArea )
{
    UNREFERENCED_PARAMETER( pSaveArea );

    SymCryptFatal( 'mmys' );
    return SYMCRYPT_NO_ERROR;
}

VOID
SYMCRYPT_CALL
SymCryptRestoreYmmEnvElevatedDebugger( _Inout_ PSYMCRYPT_EXTENDED_SAVE_DATA pSaveArea )
{
    SymCryptFatal( 'mmys' );
    UNREFERENCED_PARAMETER( pSaveArea );
}

SYMCRYPT_ERROR
SYMCRYPT_CALL
SymCryptSaveZmmEnvElevatedDebugger( _Out_ PSYMCRYPT_EXTENDED_SAVE_DATA pSaveArea )
{
    UNREFERENCED_PARAMETER( pSaveArea );
    SymCryptFatal( 'mmzs' );
    return SYMCRYPT_NO_ERROR;
}

VOID
SYMCRYPT_CALL
SymCryptRestoreZmmEnvElevatedDebugger( _Inout_ PSYMCRYPT_EXTENDED_SAVE_DATA pSaveArea )
{
    SymCryptFatal( 'mmzs' );
    UNREFERENCED_PARAMETER( pSaveArea );
}

#endif

VOID
SYMCRYPT_CALL
SymCryptTestInjectErrorEnvElevatedDebugger( PBYTE pbBuf, SIZE_T cbBuf )
{
    //
    // This feature is only used during testing. In production it is always
    // an empty function that the compiler can optimize away.
    //
    UNREFERENCED_PARAMETER( pbBuf );
    UNREFERENCED_PARAMETER( cbBuf );
}

#if SYMCRYPT_CPU_AMD64

VOID
SYMCRYPT_CALL
SymCryptCpuidExFuncEnvElevatedDebugger( int cpuInfo[4], int function_id, int subfunction_id )
{
    __cpuidex( cpuInfo, function_id, subfunction_id );
}

#endif
