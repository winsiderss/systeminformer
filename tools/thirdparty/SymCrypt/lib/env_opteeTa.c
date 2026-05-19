//
// env_opteeTa.c
// Platform-specific code for OPTEE TA.
//
// Copyright (c) Microsoft Corporation. Licensed under the MIT license.
//

#include "precomp.h"

// OPTEE TA specific data
#define TEE_ERROR_BAD_STATE               0xFFFF0007

typedef uint32_t TEE_Result;

void TEE_Panic(TEE_Result panicCode);


SYMCRYPT_CPU_FEATURES SYMCRYPT_CALL SymCryptCpuFeaturesNeverPresentEnvOpteeTa(void)
{
    return 0;
}

VOID
SYMCRYPT_CALL
SymCryptInitEnvOpteeTa( UINT32 version )
{
    if( g_SymCryptFlags & SYMCRYPT_FLAG_LIB_INITIALIZED )
    {
        return;
    }
    
#if SYMCRYPT_CPU_ARM64

    // Optee on Arm64 currently relies on the unconditional availability of certain CPU features (ASIMD, AES, PMULL, SHA256)
    g_SymCryptCpuFeaturesNotPresent = (SYMCRYPT_CPU_FEATURES) ~(SYMCRYPT_CPU_FEATURE_NEON|SYMCRYPT_CPU_FEATURE_NEON_AES|SYMCRYPT_CPU_FEATURE_NEON_PMULL|SYMCRYPT_CPU_FEATURE_NEON_SHA256);

#elif SYMCRYPT_CPU_AMD64

    SymCryptDetectCpuFeaturesByCpuid( SYMCRYPT_CPUID_DETECT_FLAG_CHECK_OS_SUPPORT_FOR_YMM );

    //
    // Our SaveXmm function never fails because it doesn't have to do anything in User mode.
    //
    g_SymCryptCpuFeaturesNotPresent &= ~SYMCRYPT_CPU_FEATURE_SAVEXMM_NOFAIL;

#else
    
    #error We only support ARM64 and AMD64 for OPTEE environment

#endif

    SymCryptInitEnvCommon( version );
}

_Analysis_noreturn_
VOID
SYMCRYPT_CALL
SymCryptFatalEnvOpteeTa( UINT32 fatalCode )
{
    UINT32 fatalCodeVar;

    SymCryptFatalIntercept( fatalCode );

    //
    // Put the fatal code in a location where it shows up in the dump
    //
    SYMCRYPT_FORCE_WRITE32( &fatalCodeVar, fatalCode );

    //
    // Our first preference is to TEE_Panic,
    // the second to create an AV, which can trigger a core dump so that we get to
    // see what is going wrong.
    //
    TEE_Panic(TEE_ERROR_BAD_STATE);
    
    //
    // Next we write to the NULL pointer, this causes an AV
    //
    SYMCRYPT_FORCE_WRITE32( (volatile UINT32 *)NULL, fatalCode );

    SymCryptFatalHang( fatalCode );
}

VOID
SYMCRYPT_CALL
SymCryptTestInjectErrorEnvOpteeTa( PBYTE pbBuf, SIZE_T cbBuf )
{
    //
    // This feature is only used during testing. In production it is always
    // an empty function that the compiler can optimize away.
    //
    UNREFERENCED_PARAMETER( pbBuf );
    UNREFERENCED_PARAMETER( cbBuf );
}


#if SYMCRYPT_CPU_AMD64

SYMCRYPT_ERROR
SYMCRYPT_CALL
SymCryptSaveXmmEnvOpteeTa( _Out_ PSYMCRYPT_EXTENDED_SAVE_DATA pSaveData )
{
    UNREFERENCED_PARAMETER( pSaveData );

    return SYMCRYPT_NO_ERROR;
}

VOID
SYMCRYPT_CALL
SymCryptRestoreXmmEnvOpteeTa( _Inout_ PSYMCRYPT_EXTENDED_SAVE_DATA pSaveData )
{
    UNREFERENCED_PARAMETER( pSaveData );
}

SYMCRYPT_ERROR
SYMCRYPT_CALL
SymCryptSaveYmmEnvOpteeTa( _Out_ PSYMCRYPT_EXTENDED_SAVE_DATA pSaveData )
{
    UNREFERENCED_PARAMETER( pSaveData );

    return SYMCRYPT_NO_ERROR;
}

VOID
SYMCRYPT_CALL
SymCryptRestoreYmmEnvOpteeTa( _Inout_ PSYMCRYPT_EXTENDED_SAVE_DATA pSaveData )
{
    UNREFERENCED_PARAMETER( pSaveData );
}

VOID
SYMCRYPT_CALL
SymCryptCpuidExFuncEnvOpteeTa( int cpuInfo[4], int function_id, int subfunction_id )
{
    __cpuidex( cpuInfo, function_id, subfunction_id );
}

#endif
