//
// rng.c
// Implements common RNG infrastructure for modules
//
// Copyright (c) Microsoft Corporation. Licensed under the MIT license.
//

#include "precomp.h"
#include "rng.h"
#include "symcrypt_low_level.h"

// Size of small entropy request cache, same as Windows
#define  RANDOM_NUM_CACHE_SIZE         128
#define  MAX_GENERATE_BEFORE_RESEED    8192

PVOID g_rngLock; // lock around access to following global variable

BOOLEAN g_RngStateInstantiated = FALSE;
SYMCRYPT_RNG_AES_STATE g_AesRngState;

BYTE g_randomBytesCache[RANDOM_NUM_CACHE_SIZE];
SIZE_T g_cbRandomBytesCache = 0;

UINT32 g_rngCounter = 0; // reseed when counter exceeds MAX_GENERATE_BEFORE_RESEED, increments 1 per generate

// This function reseeds the RNG state using the FIPS entropy source and the secure entropy source.
// Seed is constructed as per SP800-90A for CTR_DRBG with a derivation function, that is
// entropy_input || additional_input, where entropy input is from the SP800-90B compliant (if applicable)
// FIPS entropy source and the additional input is from the secure entropy source.
VOID
SymCryptRngReseed(void)
{
    BYTE seed[64]; // 256 bits of entropy input and 256 bits of additional input

    // Second half of seed is 'additional input' of SP800-90A for DRBG.
    // Additional input is simply data from secure entropy source. Place directly in second half of seed buffer
    SymCryptEntropySecureGet( seed + 32, 32 );

    if( SYMCRYPT_MODULE_USE_FIPS_ENTROPY )
    {
        // Fill first half of seed with SP800-90B compliant (if applicable) FIPS entropy source
        SymCryptEntropyFipsGet( seed, 32 );
    } else {
        // Wipe the first half of the seed if we're not using FIPS entropy source
        SymCryptWipeKnownSize( seed, 32 );
    }

    // Perform the reseed
    SymCryptRngAesReseed( &g_AesRngState, seed, sizeof(seed) );

    // Don't use any existing cached random data
    g_cbRandomBytesCache = 0;

    SymCryptWipeKnownSize( seed, sizeof(seed) );
}

// This function must be called during module initialization. It sets up
// the mutex used in following calls to Rng infrastructure.
VOID
SYMCRYPT_CALL
SymCryptRngInit(void)
{
    g_rngLock = SymCryptCallbackAllocateMutexFastInproc();

    if (g_rngLock == NULL)
    {
        SymCryptFatal( 'rngi' );
    }
}

// This sets up the internal SymCrypt RNG state by initializing the entropy sources,
// then instantiating the RNG state by seeding from Fips and secure entropy sources.
// First 32 bytes are from Fips source and last 32 are from the secure source, as per
// SP800-90A section 10.2.1.3.2.
// The FIPS input constitutes the entropy_input while secure input is the nonce.
VOID
SYMCRYPT_CALL
SymCryptRngInstantiate(void)
{
    SYMCRYPT_ERROR error = SYMCRYPT_NO_ERROR;
    BYTE seed[64];

    // Initialize both entropy sources
    if( SYMCRYPT_MODULE_USE_FIPS_ENTROPY )
    {
        SymCryptEntropyFipsInit();
    }
    SymCryptEntropySecureInit();

    if( SYMCRYPT_MODULE_USE_FIPS_ENTROPY )
    {
        // Get entropy from Fips entropy source
        SymCryptEntropyFipsGet( seed, 32 );
    } else {
        // Wipe the first half of the seed if we're not using FIPS entropy source
        SymCryptWipeKnownSize( seed, 32 );
    }

    // Get nonce and personalization string from secure entropy source
    SymCryptEntropySecureGet( seed + 32, 32 );

    // Instantiate internal RNG state
    error = SymCryptRngAesInstantiate(
        &g_AesRngState,
        seed,
        sizeof(seed) );

    if( error != SYMCRYPT_NO_ERROR )
    {
        // Instantiate only fails if cbSeedMaterial is a bad size, and if it does,
        // SymCrypt cannot continue safely
        SymCryptFatal( 'rngi' );
    }

    SymCryptWipeKnownSize( seed, sizeof(seed) );

    SymCryptRngForkDetectionInit();

    g_RngStateInstantiated = TRUE;
}

// This function must be called during module uninitialization. Cleans
// up the RNG state and lock.
// Note: bytes in g_randomBytesCache are not wiped, as they have never been
// output and so are not secret
VOID
SYMCRYPT_CALL
SymCryptRngUninit(void)
{
    if( SYMCRYPT_MODULE_USE_FIPS_ENTROPY )
    {
        SymCryptEntropyFipsUninit();
    }
    SymCryptEntropySecureUninit();
    SymCryptRngAesUninstantiate( &g_AesRngState );
    SymCryptCallbackFreeMutexFastInproc( g_rngLock );
}

// This function fills pbRandom with cbRandom bytes. For small requests,
// we use a cache of pre-generated random bits. For large requests, we call
// the AesRngState's generate function directly
VOID
SYMCRYPT_CALL
SymCryptRandom( PBYTE pbRandom, SIZE_T cbRandom )
{
    SIZE_T cbRandomTmp = cbRandom;
    SIZE_T mask;
    SIZE_T cbFill;

    if( cbRandom == 0 )
    {
        return;
    }
    
    SymCryptCallbackAcquireMutexFastInproc( g_rngLock );

    if( !g_RngStateInstantiated )
    {
        SymCryptRngInstantiate();
    }
    else
    {
        // If a fork is detected, or counter is high enough, we reseed the RNG state
        ++g_rngCounter;
        if( SymCryptRngForkDetect() || (g_rngCounter > MAX_GENERATE_BEFORE_RESEED) )
        {
            // Call the Module reseed function
            // This will reseed for us with Fips and secure entropy sources
            SymCryptRngReseed();

            g_rngCounter = 0;
        }
    }

    // Big or small request?
    if( cbRandom < RANDOM_NUM_CACHE_SIZE )
    {
        // small request, use cache
        if( g_cbRandomBytesCache > 0 )
        {
            // bytes already in cache, use them
            cbFill = SYMCRYPT_MIN( cbRandomTmp, g_cbRandomBytesCache );
            memcpy(
                pbRandom,
                &g_randomBytesCache[g_cbRandomBytesCache - cbFill],
                cbFill
            );
            SymCryptWipe(
                &g_randomBytesCache[g_cbRandomBytesCache - cbFill],
                cbFill
            );
            g_cbRandomBytesCache -= cbFill;

            pbRandom += cbFill;
            cbRandomTmp -= cbFill;
        }

        if( cbRandomTmp > 0 )
        {
            // cache empty, repopulate it and continue to fill
            SymCryptRngAesGenerate(
                &g_AesRngState,
                g_randomBytesCache,
                RANDOM_NUM_CACHE_SIZE
            );

            g_cbRandomBytesCache = RANDOM_NUM_CACHE_SIZE;

            memcpy(
                pbRandom,
                &g_randomBytesCache[g_cbRandomBytesCache - cbRandomTmp],
                cbRandomTmp
            );
            SymCryptWipe(
                &g_randomBytesCache[g_cbRandomBytesCache - cbRandomTmp],
                cbRandomTmp
            );
            g_cbRandomBytesCache -= cbRandomTmp;

            // If we never throw away some bytes, then we could have long-lasting alignment
            // problems which slow everything down.
            // If an application ever asks for a single random byte,
            // and then only for 16 bytes at a time, then every memcpy from the cache
            // would incur alignment penalties.
            // We throw away some bytes to get aligned with the current request size,
            // up to 16-alignment. This tends to align our cache with the alignment of the common
            // request sizes.
            // We throw away at most 15 bytes out of 128.

            mask = cbRandom;            //                              xxxx100...0
            mask = mask ^ (mask - 1);   // set lsbset + all lower bits  0000111...1
            mask = (mask >> 1) & 15;    // bits to mask out             0000011...1 limited to 4 bits
            g_cbRandomBytesCache &= ~mask;
        }

    }
    else
    {
        // Large request, call generate directly
        SymCryptRngAesGenerate(
            &g_AesRngState,
            pbRandom,
            cbRandom
        );
    }

    SymCryptCallbackReleaseMutexFastInproc( g_rngLock );
}

// This function mixes the provided entropy into the RNG state using a call to SymCryptRngAesGenerateSmall
// We mix the caller provided entropy with secure entropy using SHA256 to form the 32-bytes of additional input
VOID
SYMCRYPT_CALL
SymCryptProvideEntropy( PCBYTE pbEntropy, SIZE_T cbEntropy )
{
    BYTE additionalInput[32];
    SYMCRYPT_ERROR scError;
    SYMCRYPT_SHA256_STATE hashState;

    SymCryptSha256Init( &hashState );
    SymCryptSha256Append( &hashState, pbEntropy, cbEntropy );

    // Mix in data from secure entropy source.
    // Place in additionalInput buffer to store until we hash it.
    SymCryptEntropySecureGet( additionalInput, 32 );
    SymCryptSha256Append( &hashState, additionalInput, 32 );

    // Get hash result in additionalInput buffer.
    SymCryptSha256Result( &hashState, additionalInput );

    SymCryptCallbackAcquireMutexFastInproc( g_rngLock );

    scError = SymCryptRngAesGenerateSmall(
        &g_AesRngState,
        NULL,
        0,
        additionalInput,
        sizeof(additionalInput) );
    if( scError != SYMCRYPT_NO_ERROR )
    {
        SymCryptFatal( 'acdx' );
    }

    SymCryptCallbackReleaseMutexFastInproc( g_rngLock );

    SymCryptWipeKnownSize( additionalInput, sizeof(additionalInput) );
}
