//
// module.c
// Main file for SymCrypt shared object libraries on Posix-based systems
//
// Copyright (c) Microsoft Corporation. Licensed under the MIT license.
//

#include "precomp.h"


// Module main function executed by the runtime upon load
VOID __attribute__((constructor)) SymCryptModuleMain(void)
{
    SymCryptInit();

    if( SYMCRYPT_MODULE_DO_FIPS_SELFTESTS )
    {
        // We must test HMAC-SHA256 first since it's used by our integrity verification
        SymCryptHmacSha256Selftest();

        SymCryptModuleVerifyIntegrity();

        SymCryptRngAesInstantiateSelftest();
        SymCryptRngAesReseedSelftest();
        SymCryptRngAesGenerateSelftest();
    }

    // RNG must be initialized before the following selftests, but this should happen
    // regardless of whether or SYMCRYPT_MODULE_DO_FIPS_SELFTESTS is set
    SymCryptRngInit();

    if( SYMCRYPT_MODULE_DO_FIPS_SELFTESTS )
    {
        SymCrypt3DesSelftest();

        SymCryptAesSelftest( SYMCRYPT_AES_SELFTEST_ALL );
        SymCryptAesCmacSelftest();
        SymCryptCcmSelftest();
        SymCryptGcmSelftest();
        SymCryptXtsAesSelftest();

        SymCryptHmacSha1Selftest();
        SymCryptHmacSha384Selftest();
        SymCryptHmacSha512Selftest();

        SymCryptParallelSha256Selftest();
        SymCryptParallelSha512Selftest();

        SymCryptTlsPrf1_1SelfTest();
        SymCryptTlsPrf1_2SelfTest();

        SymCryptHkdfSelfTest();

        SymCryptSp800_108_HmacSha1SelfTest();
        SymCryptSp800_108_HmacSha256SelfTest();
        SymCryptSp800_108_HmacSha384SelfTest();
        SymCryptSp800_108_HmacSha512SelfTest();

        SymCryptPbkdf2_HmacSha1SelfTest();

        SymCryptSrtpKdfSelfTest();

        SymCryptSshKdfSha256SelfTest();
        SymCryptSshKdfSha512SelfTest();

        SymCryptSskdfSelfTest();

        SymCryptHmacSha3_256Selftest();

        g_SymCryptFipsSelftestsPerformed |= SYMCRYPT_SELFTEST_ALGORITHM_STARTUP;
    }
}

VOID __attribute__((destructor)) SymCryptModuleDestructor(void)
{
    SymCryptRngUninit();
}

VOID SYMCRYPT_CALL SymCryptModuleInit( UINT32 api, UINT32 minor )
{
    if( api != SYMCRYPT_CODE_VERSION_API ||
        (api == SYMCRYPT_CODE_VERSION_API && minor > SYMCRYPT_CODE_VERSION_MINOR) )
    {
        SymCryptFatal( 'vers' );
    }
}