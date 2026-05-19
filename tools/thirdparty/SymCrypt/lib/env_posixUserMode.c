//
// env_posixUserMode.c
// Platform-specific code for POSIX-compatible systems (Linux, macOS, other Unix-likes)
//
// Copyright (c) Microsoft Corporation. Licensed under the MIT license.
//

#include "precomp.h"


SYMCRYPT_CPU_FEATURES SYMCRYPT_CALL SymCryptCpuFeaturesNeverPresentEnvPosixUsermode(void)
{
    return 0;
}

VOID
SYMCRYPT_CALL
SymCryptInitEnvPosixUsermode( UINT32 version )
{
    if( g_SymCryptFlags & SYMCRYPT_FLAG_LIB_INITIALIZED )
    {
        return;
    }

#if SYMCRYPT_CPU_X86 | SYMCRYPT_CPU_AMD64
    SymCryptDetectCpuFeaturesByCpuid( SYMCRYPT_CPUID_DETECT_FLAG_CHECK_OS_SUPPORT_FOR_YMM );

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

// UGLY HACK: Forward declare __stack_chk_fail introduced by -fstack-protector-strong
// For OpenEnclave binaries we cannot have any PLT entries, but clang ignores -fno-plt for
// __stack_chk_fail.
// Opened issue against clang here: https://github.com/llvm/llvm-project/issues/54816
// If we introduce a direct reference to it in our code then clang does figure out it must be linked
// without PLT
void __stack_chk_fail(void);

// On X86, __stack_chk_fail_local is used as a wrapper for __stack_chk_fail. The compiler should
// generate it for us, but for some reason it is not doing so on gcc 9.4.0.
void __stack_chk_fail_local(void)
{
    __stack_chk_fail();
}

_Analysis_noreturn_
VOID
SYMCRYPT_CALL
SymCryptFatalEnvPosixUsermode( UINT32 fatalCode )
{
    UINT32 fatalCodeVar;

    SymCryptFatalIntercept( fatalCode );

    //
    // Put the fatal code in a location where it shows up in the dump
    //
    SYMCRYPT_FORCE_WRITE32( &fatalCodeVar, fatalCode );

    //
    // Create an AV, which can trigger a core dump so that we get to
    // see what is going wrong.
    //
    SYMCRYPT_FORCE_WRITE32( (volatile UINT32 *)NULL, fatalCode );

    SymCryptFatalHang( fatalCode );

    // Never reached - call is to force clang not to use PLT entry for this function
    // See forward declaration above
    __stack_chk_fail();
}

#if SYMCRYPT_CPU_AMD64

SYMCRYPT_ERROR
SYMCRYPT_CALL
SymCryptSaveXmmEnvPosixUsermode( _Out_ PSYMCRYPT_EXTENDED_SAVE_DATA pSaveData )
{
    UNREFERENCED_PARAMETER( pSaveData );

    return SYMCRYPT_NO_ERROR;
}

VOID
SYMCRYPT_CALL
SymCryptRestoreXmmEnvPosixUsermode( _Inout_ PSYMCRYPT_EXTENDED_SAVE_DATA pSaveData )
{
    UNREFERENCED_PARAMETER( pSaveData );
}

SYMCRYPT_ERROR
SYMCRYPT_CALL
SymCryptSaveYmmEnvPosixUsermode( _Out_ PSYMCRYPT_EXTENDED_SAVE_DATA pSaveData )
{
    UNREFERENCED_PARAMETER( pSaveData );

    return SYMCRYPT_NO_ERROR;
}

VOID
SYMCRYPT_CALL
SymCryptRestoreYmmEnvPosixUsermode( _Inout_ PSYMCRYPT_EXTENDED_SAVE_DATA pSaveData )
{
    UNREFERENCED_PARAMETER( pSaveData );
}

VOID
SYMCRYPT_CALL
SymCryptCpuidExFuncEnvPosixUsermode( int cpuInfo[4], int function_id, int subfunction_id )
{
    __cpuidex( cpuInfo, function_id, subfunction_id );
}

#endif

VOID
SYMCRYPT_CALL
SymCryptTestInjectErrorEnvPosixUsermode( PBYTE pbBuf, SIZE_T cbBuf )
{
    //
    // This feature is only used during testing. In production it is always
    // an empty function that the compiler can optimize away.
    //
    UNREFERENCED_PARAMETER( pbBuf );
    UNREFERENCED_PARAMETER( cbBuf );
}

