//
// env_generic.c
// Platform-specific code for a generic environment
// This is suitable for many embedded environments.
//
// Copyright (c) Microsoft Corporation. Licensed under the MIT license.
//

#include "precomp.h"

SYMCRYPT_CPU_FEATURES SYMCRYPT_CALL SymCryptCpuFeaturesNeverPresentEnvGeneric(void)
{
#if SYMCRYPT_CPU_X86 | SYMCRYPT_CPU_AMD64
    // Avoid all CPU features except RDRAND and RDSEED
    return (SYMCRYPT_CPU_FEATURES) ~(SYMCRYPT_CPU_FEATURE_RDRAND | SYMCRYPT_CPU_FEATURE_RDSEED );
#elif SYMCRYPT_CPU_ARM | SYMCRYPT_CPU_ARM64
    return (SYMCRYPT_CPU_FEATURES) ~(0);    // Disable all features
#elif SYMCRYPT_CPU_UNKNOWN
    return (SYMCRYPT_CPU_FEATURES) ~(0);    // Disable all features
#endif
}

VOID
SYMCRYPT_CALL
SymCryptInitEnvGeneric( UINT32 version )
{
    if( g_SymCryptFlags & SYMCRYPT_FLAG_LIB_INITIALIZED )
    {
        return;
    }

#if SYMCRYPT_CPU_X86 | SYMCRYPT_CPU_AMD64 
    // We only need to detect RDRAND and RDSEED, the other features are locked out from the NeverPresent value.
    // Do not detect OS support for YMM registers; we have disabled them in this environment anyway
    SymCryptDetectCpuFeaturesByCpuid( 0 );

    //
    // All extended registers are locked out by the g_SymCryptCpuFeaturesNeverPresent 
    //
#elif SYMCRYPT_CPU_ARM  | SYMCRYPT_CPU_ARM64
    // All Neon operations are locked out by the static NeverPresent value.
#endif    

    SymCryptInitEnvCommon( version );
}

_Analysis_noreturn_
VOID 
SYMCRYPT_CALL 
SymCryptFatalEnvGeneric( UINT32 fatalCode )
{
    UINT32 fatalCodeVar;

    SymCryptFatalIntercept( fatalCode );

    //
    // Put the fatal code in a location where it shows up in the dump
    //
    SYMCRYPT_FORCE_WRITE32( &fatalCodeVar, fatalCode );

    //
    // Create an AV, which can trigger a core dump or Watson report so that we get to 
    // see what is going wrong.
    //
    SYMCRYPT_FORCE_WRITE32( (volatile UINT32 *)NULL, fatalCode );

    SymCryptFatalHang( fatalCode );
}

#if SYMCRYPT_CPU_AMD64 | SYMCRYPT_CPU_X86

//
// These functions should never be called in the generic environment.
//

SYMCRYPT_ERROR 
SYMCRYPT_CALL 
SymCryptSaveXmmEnvGeneric( _Out_ PSYMCRYPT_EXTENDED_SAVE_DATA pSaveArea ) 
{
    UNREFERENCED_PARAMETER( pSaveArea );

    SymCryptFatal( 'mmxs' );
    return SYMCRYPT_NO_ERROR;
}

VOID 
SYMCRYPT_CALL 
SymCryptRestoreXmmEnvGeneric( _Inout_ PSYMCRYPT_EXTENDED_SAVE_DATA pSaveArea )
{
    UNREFERENCED_PARAMETER( pSaveArea );
    SymCryptFatal( 'mmxs' );
}

SYMCRYPT_ERROR 
SYMCRYPT_CALL 
SymCryptSaveYmmEnvGeneric( _Out_ PSYMCRYPT_EXTENDED_SAVE_DATA pSaveArea ) 
{
    UNREFERENCED_PARAMETER( pSaveArea );

    SymCryptFatal( 'mmys' );
    return SYMCRYPT_NO_ERROR;
}

VOID 
SYMCRYPT_CALL 
SymCryptRestoreYmmEnvGeneric( _Inout_ PSYMCRYPT_EXTENDED_SAVE_DATA pSaveArea )
{
    SymCryptFatal( 'mmxs' );
    UNREFERENCED_PARAMETER( pSaveArea );
}

#endif

VOID 
SYMCRYPT_CALL 
SymCryptTestInjectErrorEnvGeneric( PBYTE pbBuf, SIZE_T cbBuf ) 
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
SymCryptCpuidExFuncEnvGeneric( int cpuInfo[4], int function_id, int subfunction_id )
{
    __cpuidex( cpuInfo, function_id, subfunction_id );
}

#endif
