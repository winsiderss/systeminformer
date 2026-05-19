//
// data_accessors.c
// Helper functions for accessing built-in constant structures
//
// Copyright (c) Microsoft Corporation. Licensed under the MIT license.
//

#include "precomp.h"

PCSYMCRYPT_BLOCKCIPHER
SYMCRYPT_CALL
SymCryptGetBlockCipher( SYMCRYPT_BLOCKCIPHER_ID blockCipherId )
{
    switch( blockCipherId )
    {
        case SYMCRYPT_BLOCKCIPHER_ID_AES:
            return SymCryptAesBlockCipher;
        case SYMCRYPT_BLOCKCIPHER_ID_DES:
            return SymCryptDesBlockCipher;
        case SYMCRYPT_BLOCKCIPHER_ID_3DES:
            return SymCrypt3DesBlockCipher;
        case SYMCRYPT_BLOCKCIPHER_ID_DESX:
            return SymCryptDesxBlockCipher;
        case SYMCRYPT_BLOCKCIPHER_ID_RC2:
            return SymCryptRc2BlockCipher;
        default:
            return NULL;
    }
}

PCSYMCRYPT_ECURVE_PARAMS
SYMCRYPT_CALL
SymCryptGetEcurveParams( SYMCRYPT_ECURVE_ID ecurveId )
{
    switch( ecurveId )
    {
        case SYMCRYPT_ECURVE_ID_NIST_P192:
            return SymCryptEcurveParamsNistP192;
        case SYMCRYPT_ECURVE_ID_NIST_P224:
            return SymCryptEcurveParamsNistP224;
        case SYMCRYPT_ECURVE_ID_NIST_P256:
            return SymCryptEcurveParamsNistP256;
        case SYMCRYPT_ECURVE_ID_NIST_P384:
            return SymCryptEcurveParamsNistP384;
        case SYMCRYPT_ECURVE_ID_NIST_P521:
            return SymCryptEcurveParamsNistP521;
        case SYMCRYPT_ECURVE_ID_NUMS_P256T1:
            return SymCryptEcurveParamsNumsP256t1;
        case SYMCRYPT_ECURVE_ID_NUMS_P384T1:
            return SymCryptEcurveParamsNumsP384t1;
        case SYMCRYPT_ECURVE_ID_NUMS_P512T1:
            return SymCryptEcurveParamsNumsP512t1;
        case SYMCRYPT_ECURVE_ID_CURVE25519:
            return SymCryptEcurveParamsCurve25519;
        default:
            return NULL;
    }
}

PCSYMCRYPT_HASH
SYMCRYPT_CALL
SymCryptGetHashAlgorithm( SYMCRYPT_HASH_ID hashId )
{
    switch( hashId )
    {
        case SYMCRYPT_HASH_ID_MD2:
            return SymCryptMd2Algorithm;
        case SYMCRYPT_HASH_ID_MD4:
            return SymCryptMd4Algorithm;
        case SYMCRYPT_HASH_ID_MD5:
            return SymCryptMd5Algorithm;
        case SYMCRYPT_HASH_ID_SHA1:
            return SymCryptSha1Algorithm;
        case SYMCRYPT_HASH_ID_SHA224:
            return SymCryptSha224Algorithm;
        case SYMCRYPT_HASH_ID_SHA256:
            return SymCryptSha256Algorithm;
        case SYMCRYPT_HASH_ID_SHA384:
            return SymCryptSha384Algorithm;
        case SYMCRYPT_HASH_ID_SHA512:
            return SymCryptSha512Algorithm;
        case SYMCRYPT_HASH_ID_SHA512_224:
            return SymCryptSha512_224Algorithm;
        case SYMCRYPT_HASH_ID_SHA512_256:
            return SymCryptSha512_256Algorithm;
        case SYMCRYPT_HASH_ID_SHA3_224:
            return SymCryptSha3_224Algorithm;
        case SYMCRYPT_HASH_ID_SHA3_256:
            return SymCryptSha3_256Algorithm;
        case SYMCRYPT_HASH_ID_SHA3_384:
            return SymCryptSha3_384Algorithm;
        case SYMCRYPT_HASH_ID_SHA3_512:
            return SymCryptSha3_512Algorithm;
        case SYMCRYPT_HASH_ID_SHAKE128:
            return SymCryptShake128HashAlgorithm;
        case SYMCRYPT_HASH_ID_SHAKE256:
            return SymCryptShake256HashAlgorithm;
        default:
            return NULL;
    }
}

PCSYMCRYPT_MAC
SYMCRYPT_CALL
SymCryptGetMacAlgorithm( SYMCRYPT_MAC_ID macId )
{
    switch( macId )
    {
        case SYMCRYPT_MAC_ID_HMAC_MD5:
            return SymCryptHmacMd5Algorithm;
        case SYMCRYPT_MAC_ID_HMAC_SHA1:
            return SymCryptHmacSha1Algorithm;
        case SYMCRYPT_MAC_ID_HMAC_SHA224:
            return SymCryptHmacSha224Algorithm;
        case SYMCRYPT_MAC_ID_HMAC_SHA256:
            return SymCryptHmacSha256Algorithm;
        case SYMCRYPT_MAC_ID_HMAC_SHA384:
            return SymCryptHmacSha384Algorithm;
        case SYMCRYPT_MAC_ID_HMAC_SHA512:
            return SymCryptHmacSha512Algorithm;
        case SYMCRYPT_MAC_ID_HMAC_SHA512_224:
            return SymCryptHmacSha512_224Algorithm;
        case SYMCRYPT_MAC_ID_HMAC_SHA512_256:
            return SymCryptHmacSha512_256Algorithm;
        case SYMCRYPT_MAC_ID_HMAC_SHA3_224:
            return SymCryptHmacSha3_224Algorithm;
        case SYMCRYPT_MAC_ID_HMAC_SHA3_256:
            return SymCryptHmacSha3_256Algorithm;
        case SYMCRYPT_MAC_ID_HMAC_SHA3_384:
            return SymCryptHmacSha3_384Algorithm;
        case SYMCRYPT_MAC_ID_HMAC_SHA3_512:
            return SymCryptHmacSha3_512Algorithm;
        case SYMCRYPT_MAC_ID_AES_CMAC:
            return SymCryptAesCmacAlgorithm;
        case SYMCRYPT_MAC_ID_KMAC_128:
            return SymCryptKmac128Algorithm;
        case SYMCRYPT_MAC_ID_KMAC_256:
            return SymCryptKmac256Algorithm;
        default:
            return NULL;
    }
}

PCSYMCRYPT_MARVIN32_EXPANDED_SEED
SYMCRYPT_CALL
SymCryptGetMarvin32DefaultSeed( void )
{
    return SymCryptMarvin32DefaultSeed;
}

PCSYMCRYPT_OID
SYMCRYPT_CALL
SymCryptGetOidList( SYMCRYPT_OID_LIST_ID oidId, _Out_opt_ SIZE_T* pCount )
{
    SIZE_T oidCount = 0;
    PCSYMCRYPT_OID pOidList = NULL;

    switch( oidId )
    {
        case SYMCRYPT_OID_LIST_ID_MD5:
            pOidList = SymCryptMd5OidList;
            oidCount = SYMCRYPT_MD5_OID_COUNT;
            break;
        case SYMCRYPT_OID_LIST_ID_SHA1:
            pOidList = SymCryptSha1OidList;
            oidCount = SYMCRYPT_SHA1_OID_COUNT;
            break;
        case SYMCRYPT_OID_LIST_ID_SHA224:
            pOidList = SymCryptSha224OidList;
            oidCount = SYMCRYPT_SHA224_OID_COUNT;
            break;
        case SYMCRYPT_OID_LIST_ID_SHA256:
            pOidList = SymCryptSha256OidList;
            oidCount = SYMCRYPT_SHA256_OID_COUNT;
            break;
        case SYMCRYPT_OID_LIST_ID_SHA384:
            pOidList = SymCryptSha384OidList;
            oidCount = SYMCRYPT_SHA384_OID_COUNT;
            break;
        case SYMCRYPT_OID_LIST_ID_SHA512:
            pOidList = SymCryptSha512OidList;
            oidCount = SYMCRYPT_SHA512_OID_COUNT;
            break;
        case SYMCRYPT_OID_LIST_ID_SHA512_224:
            pOidList = SymCryptSha512_224OidList;
            oidCount = SYMCRYPT_SHA512_224_OID_COUNT;
            break;
        case SYMCRYPT_OID_LIST_ID_SHA512_256:
            pOidList = SymCryptSha512_256OidList;
            oidCount = SYMCRYPT_SHA512_256_OID_COUNT;
            break;
        case SYMCRYPT_OID_LIST_ID_SHA3_224:
            pOidList = SymCryptSha3_224OidList;
            oidCount = SYMCRYPT_SHA3_224_OID_COUNT;
            break;
        case SYMCRYPT_OID_LIST_ID_SHA3_256:
            pOidList = SymCryptSha3_256OidList;
            oidCount = SYMCRYPT_SHA3_256_OID_COUNT;
            break;
        case SYMCRYPT_OID_LIST_ID_SHA3_384:
            pOidList = SymCryptSha3_384OidList;
            oidCount = SYMCRYPT_SHA3_384_OID_COUNT;
            break;
        case SYMCRYPT_OID_LIST_ID_SHA3_512:
            pOidList = SymCryptSha3_512OidList;
            oidCount = SYMCRYPT_SHA3_512_OID_COUNT;
            break;
        case SYMCRYPT_OID_LIST_ID_SHAKE128:
            pOidList = SymCryptShake128OidList;
            oidCount = SYMCRYPT_SHAKE128_OID_COUNT;
            break;
        case SYMCRYPT_OID_LIST_ID_SHAKE256:
            pOidList = SymCryptShake256OidList;
            oidCount = SYMCRYPT_SHAKE256_OID_COUNT;
            break;
        default:
            return NULL;
    }

    if( pCount != NULL )
    {
        *pCount = oidCount;
    }

    return pOidList;
}