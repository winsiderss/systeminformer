//
// env_windowsKernelModeWin8_1nLater.c
// Platform-specific code for windows kernel mode, Windows 8.1 and later
//
// On Win8.1 x86 you don't need to save the XMM registers.
// This is the environment code we use, which has empty XMM save/restore routines
// for optimal performance. It also supports the use of PCLMULQDQ for GHASH as 
// that can only be used if the SaveXmm routine never fails.
//
// Copyright (c) Microsoft Corporation. Licensed under the MIT license.
//


//
// Define _NTSYSTEM_ to suppress some __declspec(dllimport) definitions, which under some circumstance lead
// to an additional indirect function call, which adds some attack surface to kernel mode.
//
#define _NTSYSTEM_

#pragma warning(push)
#pragma warning(disable: 5103) // Arm64's wdm.h included below currently generate a lot of 5103 warnings
#include <ntddk.h>
#pragma warning(pop)

#include "symcrypt.h"
#include "sc_lib.h"


#if SYMCRYPT_CPU_AMD64
//
// KeGetCurrentIrql must be inlined on AMD64 to prevent linking errors with winload.sys, which
// runs in an environment that does not define/implement KeGetCurrentIrql.
//
#define KeGetCurrentIrql() ((KIRQL)ReadCR8())
#endif

SYMCRYPT_CPU_FEATURES SYMCRYPT_CALL SymCryptCpuFeaturesNeverPresentEnvWindowsKernelmodeWin8_1nLater()
{
    return 0;
}


VOID
SYMCRYPT_CALL
SymCryptInitEnvWindowsKernelmodeWin8_1nLater( UINT32 version )
{
    #pragma warning(suppress: 4845) // Following declspec only applies when compiled with default initialization, in some test builds we don't care whether default initialization is specified
    __declspec(no_init_all)
    RTL_OSVERSIONINFOW  verInfo;

    if( g_SymCryptFlags & SYMCRYPT_FLAG_LIB_INITIALIZED )
    {
        return;
    }

    //
    // Check that we are on Win8.1 and not something earlier
    //
    verInfo.dwOSVersionInfoSize = sizeof( verInfo );
    if( !NT_SUCCESS( RtlGetVersion( &verInfo ) ) )
    {
        SymCryptFatal( 'nsrv' );
    }

    if( verInfo.dwMajorVersion < 6 || (verInfo.dwMajorVersion == 6 && verInfo.dwMinorVersion < 3 ) )
    {
        SymCryptFatal( 'nsrv' );
    }

#if SYMCRYPT_CPU_X86 | SYMCRYPT_CPU_AMD64 
    //
    // First we detect what the CPU has
    // 
    SymCryptDetectCpuFeaturesByCpuid( SYMCRYPT_CPUID_DETECT_FLAG_CHECK_OS_SUPPORT_FOR_YMM );

    //
    // We also need to be sure that the OS supports the extended registers.
    //
    {
        ULONGLONG FeatureMask = RtlGetEnabledExtendedFeatures( (ULONGLONG) -1 );

        if( !(FeatureMask & XSTATE_MASK_AVX) )
        {
            g_SymCryptCpuFeaturesNotPresent |= SYMCRYPT_CPU_FEATURE_AVX2;
        }

        if( !(FeatureMask & XSTATE_MASK_AVX512) )
        {
            g_SymCryptCpuFeaturesNotPresent |= SYMCRYPT_CPU_FEATURE_AVX512;
        }
    }
    
    //
    // Our SaveXmm function never fails because Win8.1 doesn't need XMM saving
    //
    g_SymCryptCpuFeaturesNotPresent &= ~SYMCRYPT_CPU_FEATURE_SAVEXMM_NOFAIL;

#elif SYMCRYPT_CPU_ARM | SYMCRYPT_CPU_ARM64 

    SymCryptDetectCpuFeaturesFromRegisters();

#endif    

    SymCryptInitEnvCommon( version );
}

_Analysis_noreturn_
VOID 
SYMCRYPT_CALL 
SymCryptFatalEnvWindowsKernelmodeWin8_1nLater( UINT32 fatalCode )
{
    SymCryptFatalIntercept( fatalCode );

#pragma prefast( disable:28159 )    // PreFast complains about use of BugCheckEx.    
    KeBugCheckEx( CRYPTO_LIBRARY_INTERNAL_ERROR, fatalCode, 0, 0, 0 );

    SymCryptFatalHang( fatalCode );
}


VOID 
SYMCRYPT_CALL 
SymCryptTestInjectErrorEnvWindowsKernelmodeWin8_1nLater( PBYTE pbBuf, SIZE_T cbBuf ) 
{
    //
    // This feature is only used during testing. In production it is always
    // an empty function that the compiler can optimize away.
    //
    UNREFERENCED_PARAMETER( pbBuf );
    UNREFERENCED_PARAMETER( cbBuf );
}

#if SYMCRYPT_CPU_AMD64 | SYMCRYPT_CPU_X86

SYMCRYPT_ERROR
SYMCRYPT_CALL
SymCryptSaveXmmEnvWindowsKernelmodeWin8_1nLater( _Out_ PSYMCRYPT_EXTENDED_SAVE_DATA pSaveData )
{
    UNREFERENCED_PARAMETER( pSaveData );

    return SYMCRYPT_NO_ERROR;
}

VOID
SYMCRYPT_CALL
SymCryptRestoreXmmEnvWindowsKernelmodeWin8_1nLater( _Inout_ PSYMCRYPT_EXTENDED_SAVE_DATA pSaveData )
{
    UNREFERENCED_PARAMETER( pSaveData );
}

SYMCRYPT_ERROR
SYMCRYPT_CALL
SymCryptSaveYmmEnvWindowsKernelmodeWin8_1nLater( _Out_ PSYMCRYPT_EXTENDED_SAVE_DATA pSaveData )
{
    SYMCRYPT_ERROR result = SYMCRYPT_NO_ERROR;

    C_ASSERT( sizeof( pSaveData->data ) == sizeof( XSTATE_SAVE ));

    //
    // A rare error check. If we are called without AVX2 enabled something is badly
    // wrong, and it could create a corrupted CPU state which is impossible to debug.
    //
    if( !SYMCRYPT_CPU_FEATURES_PRESENT( SYMCRYPT_CPU_FEATURE_AVX2 ) )
    {
        SymCryptFatal( ' mmy' );
    }

    // KeSaveExtendedProcessorState must only be called at IRQL <= DISPATCH_LEVEL
    if( KeGetCurrentIrql() > DISPATCH_LEVEL )
    {
        result = SYMCRYPT_EXTERNAL_FAILURE;
        goto cleanup;
    }

    //
    // Our init routine disabled AVX2 if the XSTATE_MASK_AVX isn't supported, so we don't have to check
    // for that anymore.
    //
    if( !NT_SUCCESS( KeSaveExtendedProcessorState( XSTATE_MASK_AVX, (PXSTATE_SAVE)&pSaveData->data[0] ) ) )
    {
        result = SYMCRYPT_EXTERNAL_FAILURE;
        goto cleanup;
    }

    SYMCRYPT_SET_MAGIC( pSaveData );

cleanup:
    return result;
}

VOID
SYMCRYPT_CALL
SymCryptRestoreYmmEnvWindowsKernelmodeWin8_1nLater( _Inout_ PSYMCRYPT_EXTENDED_SAVE_DATA pSaveData )
{

    SYMCRYPT_CHECK_MAGIC( pSaveData );

    KeRestoreExtendedProcessorState( (PXSTATE_SAVE)&pSaveData->data[0] );

    SYMCRYPT_WIPE_MAGIC( pSaveData );
}

#endif

#if SYMCRYPT_CPU_AMD64 | SYMCRYPT_CPU_X86

VOID
SYMCRYPT_CALL
SymCryptCpuidExFuncEnvWindowsKernelmodeWin8_1nLater( int cpuInfo[4], int function_id, int subfunction_id )
{
    __cpuidex( cpuInfo, function_id, subfunction_id );
}

#endif
