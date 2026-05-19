//
// rngfipsjitter.c
// Defines FIPS entropy functions using Jitter as the source
//
// Copyright (c) Microsoft Corporation. Licensed under the MIT license.
//

#include "precomp.h"
#include "jitterentropy.h"


static struct rand_data* g_jitter_entropy_collector = NULL;

// Initialize Jitter source and allocate entropy collector
VOID
SYMCRYPT_CALL
SymCryptEntropyFipsInit(void)
{
    if (jent_entropy_init() != 0)
    {
        // Documentation suggests that the statistical tests the init
        // function runs will succeed if the underlying system is appropriate
        // to run jitter on, so it should never fail on systems where we
        // need it to run.
        SymCryptFatal( 'jiti' );
    }
    
    g_jitter_entropy_collector = jent_entropy_collector_alloc( 1, JENT_FORCE_FIPS );
    if (g_jitter_entropy_collector == NULL)
    {
        // Entropy collector allocation only fails if the tests mention above fail,
        // invalid flags are passed in, or memory allocation fails. In a properly
        // running environment, we should not encounter any of those.
        SymCryptFatal( 'jita' );
    }
}

// Free entropy collector
VOID
SYMCRYPT_CALL
SymCryptEntropyFipsUninit(void)
{
    if (g_jitter_entropy_collector != NULL)
    {
        jent_entropy_collector_free( g_jitter_entropy_collector );
        g_jitter_entropy_collector = NULL;
    }
}

// Jitter is our SP 800-90B compliant entropy source.
VOID
SYMCRYPT_CALL
SymCryptEntropyFipsGet( _Out_writes_( cbResult ) PBYTE pbResult, SIZE_T cbResult )
{
    SIZE_T result;
    result = jent_read_entropy( g_jitter_entropy_collector, (char *)pbResult, cbResult );
    if (result != cbResult)
    {
        // FIPS_jitter_entropy should always return the amount of bytes requested.
        // If not, SymCrypt can't safely continue.
        SymCryptFatal( 'jite' );
    }
}
