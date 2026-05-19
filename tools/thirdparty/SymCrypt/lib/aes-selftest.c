//
// aes-selftest.c
//
// Copyright (c) Microsoft Corporation. Licensed under the MIT license.
//

#include "precomp.h"

extern const BYTE SymCryptTestKey32[32];
extern const BYTE SymCryptTestMsg16[16];
extern const BYTE SymCryptAesNistTestVector128Ciphertext[16];

VOID
SYMCRYPT_CALL   
SymCryptInjectError( PBYTE pbData, SIZE_T cbData );

VOID
SYMCRYPT_CALL
SymCryptAesSelftest( UINT32 maskTestsToRun )
{
    BYTE                        buf[SYMCRYPT_AES_BLOCK_SIZE];
    BYTE                        iv[SYMCRYPT_AES_BLOCK_SIZE];
    SYMCRYPT_AES_EXPANDED_KEY   key;

    if( SymCryptAesExpandKey( &key, SymCryptTestKey32, 16 ) != SYMCRYPT_NO_ERROR )
    {
        SymCryptFatal( 'aes' );
    }

    if( maskTestsToRun & SYMCRYPT_AES_SELFTEST_BASE )
    {
        //
        // Test the core encrypt/decrypt
        //
        SymCryptAesEncrypt( &key, SymCryptTestMsg16, buf );

        SymCryptInjectError( buf, SYMCRYPT_AES_BLOCK_SIZE );
        if( memcmp( buf, SymCryptAesNistTestVector128Ciphertext, SYMCRYPT_AES_BLOCK_SIZE ) != 0 )
        {
            SymCryptFatal( 'aes' );
        }

        SymCryptAesDecrypt( &key, buf, buf );

        SymCryptInjectError( buf, SYMCRYPT_AES_BLOCK_SIZE );
        if( memcmp( buf, SymCryptTestMsg16, SYMCRYPT_AES_BLOCK_SIZE ) != 0 )
        {
            SymCryptFatal( 'aes' );
        }
    }

    if( maskTestsToRun & SYMCRYPT_AES_SELFTEST_ECB )
    {
        SymCryptAesEcbEncrypt( &key, SymCryptTestMsg16, buf, SYMCRYPT_AES_BLOCK_SIZE );

        SymCryptInjectError( buf, SYMCRYPT_AES_BLOCK_SIZE );
        if( memcmp( buf, SymCryptAesNistTestVector128Ciphertext, SYMCRYPT_AES_BLOCK_SIZE ) != 0 )
        {
            SymCryptFatal( 'aes' );
        }

        SymCryptAesEcbDecrypt( &key, buf, buf, SYMCRYPT_AES_BLOCK_SIZE );

        SymCryptInjectError( buf, SYMCRYPT_AES_BLOCK_SIZE );
        if( memcmp( buf, SymCryptTestMsg16, SYMCRYPT_AES_BLOCK_SIZE ) != 0 )
        {
            SymCryptFatal( 'aes' );
        }
    }

    if( maskTestsToRun & SYMCRYPT_AES_SELFTEST_CBC )
    {
        //
        // Test CBC encrypt/decrypt
        // Set IV=0, 1-block CBC is the same as ECB.
        //

        memset( iv, 0, SYMCRYPT_AES_BLOCK_SIZE );
        SymCryptAesCbcEncrypt( &key, iv, SymCryptTestMsg16, buf, SYMCRYPT_AES_BLOCK_SIZE );

        SymCryptInjectError( buf, SYMCRYPT_AES_BLOCK_SIZE );
        if( memcmp( buf, SymCryptAesNistTestVector128Ciphertext, SYMCRYPT_AES_BLOCK_SIZE ) != 0 )
        {
            SymCryptFatal( 'aes' );
        }

        memset( iv, 0, SYMCRYPT_AES_BLOCK_SIZE );
        SymCryptAesCbcDecrypt( &key, iv, buf, buf, SYMCRYPT_AES_BLOCK_SIZE );

        SymCryptInjectError( buf, SYMCRYPT_AES_BLOCK_SIZE );
        if( memcmp( buf, SymCryptTestMsg16, SYMCRYPT_AES_BLOCK_SIZE ) != 0 )
        {
            SymCryptFatal( 'aes' );
        }
    }

    if( maskTestsToRun & SYMCRYPT_AES_SELFTEST_CBCMAC )
    {
        //
        // Test CBC-MAC
        //

        memset( iv, 0, SYMCRYPT_AES_BLOCK_SIZE );
        SymCryptAesCbcMac( &key, iv, SymCryptTestMsg16, SYMCRYPT_AES_BLOCK_SIZE  );

        SymCryptInjectError( iv, SYMCRYPT_AES_BLOCK_SIZE );
        if( memcmp( iv, SymCryptAesNistTestVector128Ciphertext, SYMCRYPT_AES_BLOCK_SIZE ) != 0 )
        {
            SymCryptFatal( 'aes' );
        }
    }

    if( maskTestsToRun & SYMCRYPT_AES_SELFTEST_CTR )
    {
        //
        // Test CtrMsb64
        //

        memcpy( iv, SymCryptTestMsg16, SYMCRYPT_AES_BLOCK_SIZE );
        memset( buf, 0, SYMCRYPT_AES_BLOCK_SIZE );
        SymCryptAesCtrMsb64( &key, iv, buf, buf, SYMCRYPT_AES_BLOCK_SIZE );

        SymCryptInjectError( buf, SYMCRYPT_AES_BLOCK_SIZE );
        if( memcmp( buf, SymCryptAesNistTestVector128Ciphertext, SYMCRYPT_AES_BLOCK_SIZE ) != 0 )
        {
            SymCryptFatal( 'aes' );
        }
    }
}