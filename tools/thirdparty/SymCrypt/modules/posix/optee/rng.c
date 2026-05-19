//
// rng.c
// Defines secure entropy functions using TEE_GenerateRandom() as the source
//
// Copyright (c) Microsoft Corporation. Licensed under the MIT license.
//

#include "precomp.h"
#include "rng.h"

// TEE_GenerateRandom gets FIPS/secure RNG from OPTEE (will be Unresolved symbol and will be resolved in linker phase with OPTEE Application)
void TEE_GenerateRandom(void *randomBuffer, size_t randomBufferLen);

VOID
SYMCRYPT_CALL
SymCryptEntropyFipsInit()
{}

VOID
SYMCRYPT_CALL
SymCryptEntropyFipsUninit()
{}

VOID
SYMCRYPT_CALL
SymCryptEntropyFipsGet( _Out_writes_( cbResult ) PBYTE pbResult, SIZE_T cbResult )
{
    // TEE_GenerateRandom gets FIPS/secure RNG from OPTEE (implementation is platform specific)
    TEE_GenerateRandom((void *)pbResult, cbResult);
}

VOID
SYMCRYPT_CALL
SymCryptEntropySecureInit()
{}

VOID
SYMCRYPT_CALL
SymCryptEntropySecureUninit()
{}

VOID
SYMCRYPT_CALL
SymCryptEntropySecureGet( _Out_writes_( cbResult ) PBYTE pbResult, SIZE_T cbResult )
{
    // TEE_GenerateRandom gets FIPS/secure RNG from OPTEE (implementation is platform specific)
    TEE_GenerateRandom((void *)pbResult, cbResult);
}

// Fork is not supported in Optee so no need to detect it
VOID
SYMCRYPT_CALL
SymCryptRngForkDetectionInit()
{}

BOOLEAN
SYMCRYPT_CALL
SymCryptRngForkDetect()
{
    return FALSE;
}
