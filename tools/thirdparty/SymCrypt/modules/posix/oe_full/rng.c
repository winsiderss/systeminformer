//
// rng.c
// Implements RNG for OE version of module
//
// Copyright (c) Microsoft Corporation. Licensed under the MIT license.
//

#include "precomp.h"
#include "rng.h"

// Forward declaration of OpenEnclave function to get host entropy.
// This is dynamically linked at runtime by OE.
__attribute__((weak)) int oe_sgx_get_additional_host_entropy(uint8_t* data, size_t size);

// RDSEED is our SP 800-90B compliant entropy source.
VOID
SYMCRYPT_CALL
SymCryptEntropyFipsInit(void)
{}

VOID
SYMCRYPT_CALL
SymCryptEntropyFipsUninit(void)
{}

VOID
SYMCRYPT_CALL
SymCryptEntropyFipsGet( _Out_writes_( cbResult ) PBYTE pbResult, SIZE_T cbResult )
{
    SymCryptRdseedGet( pbResult, cbResult );
}

// urandom is our secure entropy source. This call is implemented by Open Enclave.
VOID
SYMCRYPT_CALL
SymCryptEntropySecureInit(void)
{}

VOID
SYMCRYPT_CALL
SymCryptEntropySecureUninit(void)
{}

VOID
SYMCRYPT_CALL
SymCryptEntropySecureGet( _Out_writes_( cbResult ) PBYTE pbResult, SIZE_T cbResult )
{
    UINT32 result;
    result = oe_sgx_get_additional_host_entropy( pbResult, cbResult );
    if (result != 1 )
    {
        // This open enclave function simply calls getrandom on the host. If that fails,
        // then SymCrypt cannot continue safely so fatal is called here.
        SymCryptFatal( 'rngs' );
    }
}

// Fork is not supported in OE so no need to detect it
VOID
SYMCRYPT_CALL
SymCryptRngForkDetectionInit(void)
{}

BOOLEAN
SYMCRYPT_CALL
SymCryptRngForkDetect(void)
{
    return FALSE;
}