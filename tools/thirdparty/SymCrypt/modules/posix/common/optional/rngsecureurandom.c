//
// rngsecureurandom.c
// Defines secure entropy functions using urandom as the source
//
// Copyright (c) Microsoft Corporation. Licensed under the MIT license.
//

#include "precomp.h"
#include <sys/random.h>

// Nothing to init
VOID
SYMCRYPT_CALL
SymCryptEntropySecureInit(void){}

// Nothing to uninit
VOID
SYMCRYPT_CALL
SymCryptEntropySecureUninit(void){}

// urandom is our secure entropy source.
VOID
SYMCRYPT_CALL
SymCryptEntropySecureGet( _Out_writes_( cbResult ) PBYTE pbResult, SIZE_T cbResult )
{
    SIZE_T result = getentropy( pbResult, cbResult );
    if ( result != 0 ) {
        SymCryptFatal( 'rngs' );
    }
}
