//
// env_windowsBootLib.c
// Platform-specific code for windows boot library environment.
//
// Copyright (c) Microsoft Corporation. Licensed under the MIT license.
//

#pragma warning(push)
#pragma warning(disable: 5103) // Arm64's wdm.h included below currently generate a lot of 5103 warnings
#include <ntddk.h>
#pragma warning(pop)
#include <windef.h>

#include "symcrypt.h"
#include "sc_lib.h"

//
// The BlStatusError function, part of the bootlib, is normally defined in blstatus.h
// We can't include that header file from outside the onecore codebase.
// We copied the definition here.
//

VOID
BlStatusError (
    __in ULONG ErrorCode,
    __in ULONG_PTR ErrorParameter1,
    __in ULONG_PTR ErrorParameter2,
    __in ULONG_PTR ErrorParameter3,
    __in ULONG_PTR ErrorParameter4
    );



SYMCRYPT_CPU_FEATURES SYMCRYPT_CALL SymCryptCpuFeaturesNeverPresentEnvWindowsBootlibrary()
{
#if SYMCRYPT_CPU_X86 | SYMCRYPT_CPU_AMD64
    //
    // We disable AVX2 for X86 (including X86-64)
    // This reduces codesize for bootmgr on PCAT which is codesize-constrained,
    // and is simpler than creating a separate PCAT-bootlib SymCrypt environment.
    //
    return SYMCRYPT_CPU_FEATURE_AVX2;
#elif SYMCRYPT_CPU_ARM | SYMCRYPT_CPU_ARM64
    return 0;
#endif
}


//
// Bootlib code calls the init multiple times, and it gets inlined too often.
//
SYMCRYPT_NOINLINE
VOID
SYMCRYPT_CALL
SymCryptInitEnvWindowsBootlibrary( UINT32 version )
{
    if( g_SymCryptFlags & SYMCRYPT_FLAG_LIB_INITIALIZED )
    {
        return;
    }

#if SYMCRYPT_CPU_X86 | SYMCRYPT_CPU_AMD64 
    SymCryptDetectCpuFeaturesByCpuid( SYMCRYPT_CPUID_DETECT_FLAG_CHECK_OS_SUPPORT_FOR_YMM );

    //
    // Our SaveXmm functions never fail because they don't do anything.
    //
    g_SymCryptCpuFeaturesNotPresent &= ~SYMCRYPT_CPU_FEATURE_SAVEXMM_NOFAIL;

#elif SYMCRYPT_CPU_ARM
    // We run in a CPU mode that allows us to read the configuration registers
    SymCryptDetectCpuFeaturesFromRegisters();
#elif SYMCRYPT_CPU_ARM64
    // We run in a CPU mode that allows us to read the configuration registers
    SymCryptDetectCpuFeaturesFromRegistersNoTry();
#endif    

    SymCryptInitEnvCommon( version );
}

_Analysis_noreturn_
VOID 
SYMCRYPT_CALL 
SymCryptFatalEnvWindowsBootlibrary( UINT32 fatalCode )
{
    UINT32 fatalCodeVar;

    SymCryptFatalIntercept( fatalCode );

    //
    // Put the fatal code in a location where it shows up in the dump
    //
    SYMCRYPT_FORCE_WRITE32( &fatalCodeVar, fatalCode );

    //
    // First we try to report the error using the Boot library infrastructure
    //
    BlStatusError ( fatalCode, 0, 0, 0, 0 );

    //
    // Then try a breakpoint; maybe that will work
    //
    __debugbreak();

    SymCryptFatalHang( fatalCode );
}

#if SYMCRYPT_CPU_X86 | SYMCRYPT_CPU_AMD64

SYMCRYPT_ERROR 
SYMCRYPT_CALL 
SymCryptSaveXmmEnvWindowsBootlibrary( _Out_ PSYMCRYPT_EXTENDED_SAVE_DATA pSaveArea ) 
{
    //
    // In Bootlibrary there is no need to save XMM registers.
    // The compiler should inline this function and optimize it away.
    //

    UNREFERENCED_PARAMETER( pSaveArea );

    return SYMCRYPT_NO_ERROR;
}

VOID 
SYMCRYPT_CALL 
SymCryptRestoreXmmEnvWindowsBootlibrary( _Inout_ PSYMCRYPT_EXTENDED_SAVE_DATA pSaveArea )
{
    //
    // In Bootlibrary there is no need to save XMM registers.
    // The compiler should inline this function and optimize it away.
    //

    UNREFERENCED_PARAMETER( pSaveArea );
}

SYMCRYPT_ERROR 
SYMCRYPT_CALL 
SymCryptSaveYmmEnvWindowsBootlibrary( _Out_ PSYMCRYPT_EXTENDED_SAVE_DATA pSaveArea ) 
{
    //
    // In Bootlibrary there is no need to save XMM registers.
    // The compiler should inline this function and optimize it away.
    //

    UNREFERENCED_PARAMETER( pSaveArea );

    return SYMCRYPT_NO_ERROR;
}

VOID 
SYMCRYPT_CALL 
SymCryptRestoreYmmEnvWindowsBootlibrary( _Inout_ PSYMCRYPT_EXTENDED_SAVE_DATA pSaveArea )
{
    //
    // In Bootlibrary there is no need to save XMM registers.
    // The compiler should inline this function and optimize it away.
    //

    UNREFERENCED_PARAMETER( pSaveArea );
}

#endif

VOID 
SYMCRYPT_CALL 
SymCryptTestInjectErrorEnvWindowsBootlibrary( PBYTE pbBuf, SIZE_T cbBuf ) 
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
SymCryptCpuidExFuncEnvWindowsBootlibrary( int cpuInfo[4], int function_id, int subfunction_id )
{
    __cpuidex( cpuInfo, function_id, subfunction_id );
}

#endif    
