//
// env_windowsKernelDebugger.c
// Platform-specific code for the Windows kernel debugger.
// The debugger runs before the Windows kernel mode is up-and-running, and has some special
// requirements.
//
// Copyright (c) Microsoft Corporation. Licensed under the MIT license.
//

#include "precomp.h"

SYMCRYPT_CPU_FEATURES SYMCRYPT_CALL SymCryptCpuFeaturesNeverPresentEnvWindowsKernelDebugger()
{
    return (SYMCRYPT_CPU_FEATURES)~0;
}

VOID
SYMCRYPT_CALL
SymCryptInitEnvWindowsKernelDebugger( UINT32 version )
{
    if( g_SymCryptFlags & SYMCRYPT_FLAG_LIB_INITIALIZED )
    {
        return;
    }

    //
    // We disable all extra CPU features.
    //

    g_SymCryptCpuFeaturesNotPresent = (SYMCRYPT_CPU_FEATURES) ~0;

    SymCryptInitEnvCommon( version );
}

_Analysis_noreturn_
VOID 
SYMCRYPT_CALL 
SymCryptFatalEnvWindowsKernelDebugger( UINT32 fatalCode )
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

#if SYMCRYPT_CPU_AMD64 | SYMCRYPT_CPU_X86

//
// These functions should never be called in the WindowsKernelDebugger environment because
// the corresponding CPU features are locked out.
//

SYMCRYPT_ERROR 
SYMCRYPT_CALL 
SymCryptSaveXmmEnvWindowsKernelDebugger( _Out_ PSYMCRYPT_EXTENDED_SAVE_DATA pSaveArea ) 
{
    UNREFERENCED_PARAMETER( pSaveArea );

    SymCryptFatal( 'mmxs' );
    return SYMCRYPT_NO_ERROR;
}

VOID 
SYMCRYPT_CALL 
SymCryptRestoreXmmEnvWindowsKernelDebugger( _Inout_ PSYMCRYPT_EXTENDED_SAVE_DATA pSaveArea )
{
    UNREFERENCED_PARAMETER( pSaveArea );
    SymCryptFatal( 'mmxs' );
}

SYMCRYPT_ERROR 
SYMCRYPT_CALL 
SymCryptSaveYmmEnvWindowsKernelDebugger( _Out_ PSYMCRYPT_EXTENDED_SAVE_DATA pSaveArea ) 
{
    UNREFERENCED_PARAMETER( pSaveArea );

    SymCryptFatal( 'mmys' );
    return SYMCRYPT_NO_ERROR;
}

VOID 
SYMCRYPT_CALL 
SymCryptRestoreYmmEnvWindowsKernelDebugger( _Inout_ PSYMCRYPT_EXTENDED_SAVE_DATA pSaveArea )
{
    SymCryptFatal( 'mmys' );
    UNREFERENCED_PARAMETER( pSaveArea );
}

#endif

VOID 
SYMCRYPT_CALL 
SymCryptTestInjectErrorEnvWindowsKernelDebugger( PBYTE pbBuf, SIZE_T cbBuf ) 
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
SymCryptCpuidExFuncEnvWindowsKernelDebugger( int cpuInfo[4], int function_id, int subfunction_id )
{
    __cpuidex( cpuInfo, function_id, subfunction_id );
}

#endif
