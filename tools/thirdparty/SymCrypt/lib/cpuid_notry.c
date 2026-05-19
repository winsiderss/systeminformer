//
// cpuid_notry.c   code for CPU feature detection based on CPUID for ARM64
//	           without the try except structure (used in boot environment)
//
// Copyright (c) Microsoft Corporation. Licensed under the MIT license.
//


#include "precomp.h"

#if SYMCRYPT_CPU_ARM64 & SYMCRYPT_MS_VC

#define ARM64_SYSREG(op0, op1, crn, crm, op2) \
        ( ((op0 & 1) << 14) | \
          ((op1 & 7) << 11) | \
          ((crn & 15) << 7) | \
          ((crm & 15) << 3) | \
          ((op2 & 7) << 0) )

#define ARM64_ID_AA64ISAR0_EL1  ARM64_SYSREG(3,0, 0, 6,0)   // ISA Feature Register 0

// Define the system register encoding for RNDRRS
#ifndef ARM64_RNDRRS
#define ARM64_RNDRRS ARM64_SYSREG(3, 3, 2, 4, 1)
#endif

// Define the system register encoding for RNDR (Non-reseeded)
#ifndef ARM64_RNDR
#define ARM64_RNDR ARM64_SYSREG(3, 3, 2, 4, 0)
#endif

#define ISAR0_AES                   1
#define ISAR0_AES_NI                0
#define ISAR0_AES_INSTRUCTIONS      1
#define ISAR0_AES_PLUS_PMULL64      2

#define ISAR0_SHA2                  3
#define ISAR0_SHA2_NI               0
#define ISAR0_SHA2_INSTRUCTIONS     1

#define ISAR0_CRC32                 4
#define ISAR0_CRC32_NI              0
#define ISAR0_CRC32_INSTRUCTIONS    1

#define READ_ARM64_FEATURE(_FeatureRegister, _Index) \
        (((ULONG64)_ReadStatusReg(_FeatureRegister) >> ((_Index) * 4)) & 0xF)

VOID
SYMCRYPT_CALL
SymCryptDetectCpuFeaturesFromRegistersNoTry(void)
{
    ULONG result;

    result = ~ (ULONG)(
        SYMCRYPT_CPU_FEATURE_NEON           |
        SYMCRYPT_CPU_FEATURE_NEON_AES       |
        SYMCRYPT_CPU_FEATURE_NEON_PMULL     |
        SYMCRYPT_CPU_FEATURE_NEON_SHA256
        );


    if( READ_ARM64_FEATURE(ARM64_ID_AA64ISAR0_EL1, ISAR0_AES) < ISAR0_AES_INSTRUCTIONS )
    {
        result |= SYMCRYPT_CPU_FEATURE_NEON_AES;
    }

    if( READ_ARM64_FEATURE(ARM64_ID_AA64ISAR0_EL1, ISAR0_AES) < ISAR0_AES_PLUS_PMULL64 )
    {
        result |= SYMCRYPT_CPU_FEATURE_NEON_PMULL;
    }

    if( READ_ARM64_FEATURE(ARM64_ID_AA64ISAR0_EL1, ISAR0_SHA2) < ISAR0_SHA2_INSTRUCTIONS )
    {
        result |= SYMCRYPT_CPU_FEATURE_NEON_SHA256;
    }

    g_SymCryptCpuFeaturesNotPresent = (SYMCRYPT_CPU_FEATURES) result;

}

#endif // CPU arch selection
