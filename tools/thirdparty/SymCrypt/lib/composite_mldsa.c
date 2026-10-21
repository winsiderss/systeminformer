//
// composite_mldsa.c   ML-DSA related functionality
//
// Copyright (c) Microsoft Corporation. Licensed under the MIT license.
//

#include "precomp.h"

// 436F6D706F73697465416C676F726974686D5369676E61747572657332303235
static const CHAR c_pszPrefix[] = "CompositeAlgorithmSignatures2025";
static const SIZE_T c_cbPrefix  = sizeof( c_pszPrefix ) - 1;

#define SYMCRYPT_COMPOSITE_MLDSA_44_ECDSA_P256_SHA256_LABEL "COMPSIG-MLDSA44-ECDSA-P256-SHA256"
#define SYMCRYPT_COMPOSITE_MLDSA_65_ECDSA_P256_SHA512_LABEL "COMPSIG-MLDSA65-ECDSA-P256-SHA512"
#define SYMCRYPT_COMPOSITE_MLDSA_65_ECDSA_P384_SHA512_LABEL "COMPSIG-MLDSA65-ECDSA-P384-SHA512"
#define SYMCRYPT_COMPOSITE_MLDSA_87_ECDSA_P384_SHA512_LABEL "COMPSIG-MLDSA87-ECDSA-P384-SHA512"

#define SYMCRYPT_COMPOSITE_SIZEOF_ENCODED_EC_SIGNATURE_P256_MAX     (72)
#define SYMCRYPT_COMPOSITE_SIZEOF_ENCODED_EC_SIGNATURE_P384_MAX     (104)

//
// M' :=  Prefix || Label || len(ctx) || ctx || PH( M )
//        32     +  33    +  1        +  255  + 64      ==  385 bytes
//
#define SYMCRYPT_COMPOSITE_MLDSA_SIZEOF_MU_MAX ( \
    (sizeof( c_pszPrefix ) - 1) + \
    (sizeof( SYMCRYPT_COMPOSITE_MLDSA_87_ECDSA_P384_SHA512_LABEL ) - 1) + \
    sizeof( BYTE ) + \
    SYMCRYPT_COMPOSITE_MLDSA_CONTEXT_MAX_LENGTH + \
    SYMCRYPT_SHA512_RESULT_SIZE )

//
// Mu + Mu Hash + ECDSA-P384 signature == 529 bytes
//
#define SYMCRYPT_COMPOSITE_MLDSA_SIZEOF_SCRATCH_MAX ( \
    SYMCRYPT_COMPOSITE_MLDSA_SIZEOF_MU_MAX + \
    SYMCRYPT_SHA384_RESULT_SIZE + \
    96 )

//
// id-MLDSA44-ECDSA-P256-SHA256
//
static const SYMCRYPT_COMPOSITE_MLDSA_INTERNAL_PARAMS SymCryptCompositeMlDsaInternalParams_MLDSA44_ECDSA_P256_SHA256 = {
    .pcszLabel = SYMCRYPT_COMPOSITE_MLDSA_44_ECDSA_P256_SHA256_LABEL,
    .cbLabel = sizeof( SYMCRYPT_COMPOSITE_MLDSA_44_ECDSA_P256_SHA256_LABEL ) - 1,
    .mldsaParams = SYMCRYPT_MLDSA_PARAMS_MLDSA44,
    .eCurveId = SYMCRYPT_CACHED_ECURVE_ID_NIST_P256,
    .messagePreHashId = SYMCRYPT_HASH_ID_SHA256,
    .cbMessagePreHash = SYMCRYPT_SHA256_RESULT_SIZE,
    .messageRepresentativeHashId = SYMCRYPT_HASH_ID_SHA256,
    .cbMessageRepresentativeHash = SYMCRYPT_SHA256_RESULT_SIZE,
    .cbEncodedPrivateKey = SYMCRYPT_COMPOSITE_MLDSA_PRIVATE_KEY_SIZE_MLDSA44_ECDSA_P256_SHA256, // 32 + 51
    .cbEncodedPublicKey = SYMCRYPT_COMPOSITE_MLDSA_PUBLIC_KEY_SIZE_MLDSA44_ECDSA_P256_SHA256, // 1312 + 65
    .cbEncodedSignatureMax = SYMCRYPT_COMPOSITE_MLDSA_SIGNATURE_SIZE_MLDSA44_ECDSA_P256_SHA256, // 2420 + 72
};
C_ASSERT(
    SYMCRYPT_COMPOSITE_MLDSA_PRIVATE_KEY_SIZE_MLDSA44_ECDSA_P256_SHA256
        == (SYMCRYPT_MLDSA_PRIVATE_SEED_SIZE + SYMCRYPT_COMPOSITE_SIZEOF_ENCODED_EC_PRIVATE_KEY_P256) );
C_ASSERT(
    SYMCRYPT_COMPOSITE_MLDSA_PUBLIC_KEY_SIZE_MLDSA44_ECDSA_P256_SHA256
        == (SYMCRYPT_MLDSA_PUBLIC_KEY_SIZE_MLDSA44 + SYMCRYPT_COMPOSITE_SIZEOF_ENCODED_EC_PUBLIC_KEY_P256) );
C_ASSERT(
    SYMCRYPT_COMPOSITE_MLDSA_SIGNATURE_SIZE_MLDSA44_ECDSA_P256_SHA256
        == (SYMCRYPT_MLDSA_SIGNATURE_SIZE_MLDSA44 + SYMCRYPT_COMPOSITE_SIZEOF_ENCODED_EC_SIGNATURE_P256_MAX) );

//
// id-MLDSA65-ECDSA-P256-SHA512
//
static const SYMCRYPT_COMPOSITE_MLDSA_INTERNAL_PARAMS SymCryptCompositeMlDsaInternalParams_MLDSA65_ECDSA_P256_SHA512 = {
    .pcszLabel = SYMCRYPT_COMPOSITE_MLDSA_65_ECDSA_P256_SHA512_LABEL,
    .cbLabel = sizeof( SYMCRYPT_COMPOSITE_MLDSA_65_ECDSA_P256_SHA512_LABEL ) - 1,
    .mldsaParams = SYMCRYPT_MLDSA_PARAMS_MLDSA65,
    .eCurveId = SYMCRYPT_CACHED_ECURVE_ID_NIST_P256,
    .messagePreHashId = SYMCRYPT_HASH_ID_SHA512,
    .cbMessagePreHash = SYMCRYPT_SHA512_RESULT_SIZE,
    .messageRepresentativeHashId = SYMCRYPT_HASH_ID_SHA256,
    .cbMessageRepresentativeHash = SYMCRYPT_SHA256_RESULT_SIZE,
    .cbEncodedPrivateKey = SYMCRYPT_COMPOSITE_MLDSA_PRIVATE_KEY_SIZE_MLDSA65_ECDSA_P256_SHA512, // 32 + 51
    .cbEncodedPublicKey = SYMCRYPT_COMPOSITE_MLDSA_PUBLIC_KEY_SIZE_MLDSA65_ECDSA_P256_SHA512, // 1952 + 65
    .cbEncodedSignatureMax = SYMCRYPT_COMPOSITE_MLDSA_SIGNATURE_SIZE_MLDSA65_ECDSA_P256_SHA512, // 3309 + 72
};
C_ASSERT(
    SYMCRYPT_COMPOSITE_MLDSA_PRIVATE_KEY_SIZE_MLDSA65_ECDSA_P256_SHA512
        == (SYMCRYPT_MLDSA_PRIVATE_SEED_SIZE + SYMCRYPT_COMPOSITE_SIZEOF_ENCODED_EC_PRIVATE_KEY_P256) );
C_ASSERT(
    SYMCRYPT_COMPOSITE_MLDSA_PUBLIC_KEY_SIZE_MLDSA65_ECDSA_P256_SHA512
        == (SYMCRYPT_MLDSA_PUBLIC_KEY_SIZE_MLDSA65 + SYMCRYPT_COMPOSITE_SIZEOF_ENCODED_EC_PUBLIC_KEY_P256) );
C_ASSERT(
    SYMCRYPT_COMPOSITE_MLDSA_SIGNATURE_SIZE_MLDSA65_ECDSA_P256_SHA512
        == (SYMCRYPT_MLDSA_SIGNATURE_SIZE_MLDSA65 + SYMCRYPT_COMPOSITE_SIZEOF_ENCODED_EC_SIGNATURE_P256_MAX) );

//
// id-MLDSA65-ECDSA-P384-SHA512
//
static const SYMCRYPT_COMPOSITE_MLDSA_INTERNAL_PARAMS SymCryptCompositeMlDsaInternalParams_MLDSA65_ECDSA_P384_SHA512 = {
    .pcszLabel = SYMCRYPT_COMPOSITE_MLDSA_65_ECDSA_P384_SHA512_LABEL,
    .cbLabel = sizeof( SYMCRYPT_COMPOSITE_MLDSA_65_ECDSA_P384_SHA512_LABEL ) - 1,
    .mldsaParams = SYMCRYPT_MLDSA_PARAMS_MLDSA65,
    .eCurveId = SYMCRYPT_CACHED_ECURVE_ID_NIST_P384,
    .messagePreHashId = SYMCRYPT_HASH_ID_SHA512,
    .cbMessagePreHash = SYMCRYPT_SHA512_RESULT_SIZE,
    .messageRepresentativeHashId = SYMCRYPT_HASH_ID_SHA384,
    .cbMessageRepresentativeHash = SYMCRYPT_SHA384_RESULT_SIZE,
    .cbEncodedPrivateKey = SYMCRYPT_COMPOSITE_MLDSA_PRIVATE_KEY_SIZE_MLDSA65_ECDSA_P384_SHA512, // 32 + 64
    .cbEncodedPublicKey = SYMCRYPT_COMPOSITE_MLDSA_PUBLIC_KEY_SIZE_MLDSA65_ECDSA_P384_SHA512, // 1952 + 97
    .cbEncodedSignatureMax = SYMCRYPT_COMPOSITE_MLDSA_SIGNATURE_SIZE_MLDSA65_ECDSA_P384_SHA512, // 3309 + 104
};
C_ASSERT(
    SYMCRYPT_COMPOSITE_MLDSA_PRIVATE_KEY_SIZE_MLDSA65_ECDSA_P384_SHA512
        == (SYMCRYPT_MLDSA_PRIVATE_SEED_SIZE + SYMCRYPT_COMPOSITE_SIZEOF_ENCODED_EC_PRIVATE_KEY_P384) );
C_ASSERT(
    SYMCRYPT_COMPOSITE_MLDSA_PUBLIC_KEY_SIZE_MLDSA65_ECDSA_P384_SHA512
        == (SYMCRYPT_MLDSA_PUBLIC_KEY_SIZE_MLDSA65 + SYMCRYPT_COMPOSITE_SIZEOF_ENCODED_EC_PUBLIC_KEY_P384) );
C_ASSERT(
    SYMCRYPT_COMPOSITE_MLDSA_SIGNATURE_SIZE_MLDSA65_ECDSA_P384_SHA512
        == (SYMCRYPT_MLDSA_SIGNATURE_SIZE_MLDSA65 + SYMCRYPT_COMPOSITE_SIZEOF_ENCODED_EC_SIGNATURE_P384_MAX) );

//
// id-MLDSA87-ECDSA-P384-SHA512
//
static const SYMCRYPT_COMPOSITE_MLDSA_INTERNAL_PARAMS SymCryptCompositeMlDsaInternalParams_MLDSA87_ECDSA_P384_SHA512 = {
    .pcszLabel = SYMCRYPT_COMPOSITE_MLDSA_87_ECDSA_P384_SHA512_LABEL,
    .cbLabel = sizeof( SYMCRYPT_COMPOSITE_MLDSA_87_ECDSA_P384_SHA512_LABEL ) - 1,
    .mldsaParams = SYMCRYPT_MLDSA_PARAMS_MLDSA87,
    .eCurveId = SYMCRYPT_CACHED_ECURVE_ID_NIST_P384,
    .messagePreHashId = SYMCRYPT_HASH_ID_SHA512,
    .cbMessagePreHash = SYMCRYPT_SHA512_RESULT_SIZE,
    .messageRepresentativeHashId = SYMCRYPT_HASH_ID_SHA384,
    .cbMessageRepresentativeHash = SYMCRYPT_SHA384_RESULT_SIZE,
    .cbEncodedPrivateKey = SYMCRYPT_COMPOSITE_MLDSA_PRIVATE_KEY_SIZE_MLDSA87_ECDSA_P384_SHA512, // 32 + 64
    .cbEncodedPublicKey = SYMCRYPT_COMPOSITE_MLDSA_PUBLIC_KEY_SIZE_MLDSA87_ECDSA_P384_SHA512, // 2592 + 97
    .cbEncodedSignatureMax = SYMCRYPT_COMPOSITE_MLDSA_SIGNATURE_SIZE_MLDSA87_ECDSA_P384_SHA512, // 4627 + 104
};
C_ASSERT(
    SYMCRYPT_COMPOSITE_MLDSA_PRIVATE_KEY_SIZE_MLDSA87_ECDSA_P384_SHA512
        == (SYMCRYPT_MLDSA_PRIVATE_SEED_SIZE + SYMCRYPT_COMPOSITE_SIZEOF_ENCODED_EC_PRIVATE_KEY_P384) );
C_ASSERT(
    SYMCRYPT_COMPOSITE_MLDSA_PUBLIC_KEY_SIZE_MLDSA87_ECDSA_P384_SHA512
        == (SYMCRYPT_MLDSA_PUBLIC_KEY_SIZE_MLDSA87 + SYMCRYPT_COMPOSITE_SIZEOF_ENCODED_EC_PUBLIC_KEY_P384) );
C_ASSERT(
    SYMCRYPT_COMPOSITE_MLDSA_SIGNATURE_SIZE_MLDSA87_ECDSA_P384_SHA512
        == (SYMCRYPT_MLDSA_SIGNATURE_SIZE_MLDSA87 + SYMCRYPT_COMPOSITE_SIZEOF_ENCODED_EC_SIGNATURE_P384_MAX) );

static
VOID
SymCryptCompositeMlDsaHash(
                            SYMCRYPT_HASH_ID    hashId,
    _In_reads_( cbData )    PCBYTE              pbData,
                            SIZE_T              cbData,
                            PBYTE               pbResult )
{
    switch( hashId )
    {
        case SYMCRYPT_HASH_ID_SHA256:
            SymCryptSha256( pbData, cbData, pbResult );
            break;
        case SYMCRYPT_HASH_ID_SHA384:
            SymCryptSha384( pbData, cbData, pbResult );
            break;
        case SYMCRYPT_HASH_ID_SHA512:
            SymCryptSha512( pbData, cbData, pbResult );
            break;
        default:
            //
            // Only called on the SYMCRYPT_HASH_ID values from the
            // SYMCRYPT_COMPOSITE_MLDSA_INTERNAL_PARAMS definitions above.
            //
            SYMCRYPT_ASSERT( FALSE );
            break;
    }
}

static
VOID
SymCryptCompositeMlDsaWipePrivateState(
    _In_    PSYMCRYPT_COMPOSITE_MLDSAKEY    pkCompositeMlDsakey )
{
    SymCryptMlDsakeyWipePrivateState( pkCompositeMlDsakey->pkMlDsakey );
    SymCryptEckeyWipePrivateState( pkCompositeMlDsakey->pkEckey );
}

static
SYMCRYPT_ERROR
SYMCRYPT_CALL
SymCryptCompositeMlDsaGetInternalParamsFromParams(
            SYMCRYPT_COMPOSITE_MLDSA_PARAMS             params,
    _Out_   PCSYMCRYPT_COMPOSITE_MLDSA_INTERNAL_PARAMS* ppInternalParams )
{
    SYMCRYPT_ERROR scError = SYMCRYPT_NO_ERROR;

    switch( params )
    {
        case SYMCRYPT_COMPOSITE_MLDSA_PARAMS_MLDSA44_ECDSA_P256_SHA256:
            *ppInternalParams = &SymCryptCompositeMlDsaInternalParams_MLDSA44_ECDSA_P256_SHA256;
            break;
        case SYMCRYPT_COMPOSITE_MLDSA_PARAMS_MLDSA65_ECDSA_P256_SHA512:
            *ppInternalParams = &SymCryptCompositeMlDsaInternalParams_MLDSA65_ECDSA_P256_SHA512;
            break;
        case SYMCRYPT_COMPOSITE_MLDSA_PARAMS_MLDSA65_ECDSA_P384_SHA512:
            *ppInternalParams = &SymCryptCompositeMlDsaInternalParams_MLDSA65_ECDSA_P384_SHA512;
            break;
        case SYMCRYPT_COMPOSITE_MLDSA_PARAMS_MLDSA87_ECDSA_P384_SHA512:
            *ppInternalParams = &SymCryptCompositeMlDsaInternalParams_MLDSA87_ECDSA_P384_SHA512;
            break;
        default:
            scError = SYMCRYPT_INVALID_ARGUMENT;
            break;
    }

    return scError;
}

_Use_decl_annotations_
SYMCRYPT_ERROR
SYMCRYPT_CALL
SymCryptCompositeMlDsaSizeofKeyFormatFromParams(
    SYMCRYPT_COMPOSITE_MLDSA_PARAMS     params,
    SYMCRYPT_COMPOSITE_MLDSAKEY_FORMAT  compositeMlDsakeyFormat,
    SIZE_T*                             pcbKeyFormat )
{
    SYMCRYPT_ERROR scError = SYMCRYPT_NO_ERROR;
    PCSYMCRYPT_COMPOSITE_MLDSA_INTERNAL_PARAMS pInternalParams = NULL;

    scError = SymCryptCompositeMlDsaGetInternalParamsFromParams( params, &pInternalParams );
    if( scError != SYMCRYPT_NO_ERROR )
    {
        goto cleanup;
    }

    switch( compositeMlDsakeyFormat )
    {
        case SYMCRYPT_COMPOSITE_MLDSAKEY_FORMAT_PRIVATE_KEY:
            *pcbKeyFormat = pInternalParams->cbEncodedPrivateKey;
            break;
        case SYMCRYPT_COMPOSITE_MLDSAKEY_FORMAT_PUBLIC_KEY:
            *pcbKeyFormat = pInternalParams->cbEncodedPublicKey;
            break;
        case SYMCRYPT_COMPOSITE_MLDSAKEY_FORMAT_NULL:
            scError = SYMCRYPT_INCOMPATIBLE_FORMAT;
            goto cleanup;
        default:
            scError = SYMCRYPT_INVALID_ARGUMENT;
            goto cleanup;
    }

cleanup:
    return scError;
}

_Use_decl_annotations_
SYMCRYPT_ERROR
SYMCRYPT_CALL
SymCryptCompositeMlDsaSizeofSignatureFromParams(
    SYMCRYPT_COMPOSITE_MLDSA_PARAMS params,
    SIZE_T*                         pcbSignature )
{
    SYMCRYPT_ERROR scError = SYMCRYPT_NO_ERROR;
    PCSYMCRYPT_COMPOSITE_MLDSA_INTERNAL_PARAMS pInternalParams = NULL;

    scError = SymCryptCompositeMlDsaGetInternalParamsFromParams( params, &pInternalParams );
    if( scError != SYMCRYPT_NO_ERROR )
    {
        goto cleanup;
    }

    *pcbSignature = pInternalParams->cbEncodedSignatureMax;

cleanup:
    return scError;
}

_Use_decl_annotations_
PSYMCRYPT_COMPOSITE_MLDSAKEY
SYMCRYPT_CALL
SymCryptCompositeMlDsakeyAllocate(
    SYMCRYPT_COMPOSITE_MLDSA_PARAMS params )
{
    SYMCRYPT_ERROR scError = SYMCRYPT_NO_ERROR;
    PSYMCRYPT_COMPOSITE_MLDSA_INTERNAL_PARAMS pInternalParams = NULL;
    PCSYMCRYPT_ECURVE pCurve = NULL;
    PSYMCRYPT_ECKEY pkEckey = NULL;
    PSYMCRYPT_MLDSAKEY pkMlDsakey = NULL;
    PSYMCRYPT_COMPOSITE_MLDSAKEY pkCompositeKey = NULL;

    scError = SymCryptCompositeMlDsaGetInternalParamsFromParams( params, &pInternalParams );
    if( scError != SYMCRYPT_NO_ERROR )
    {
        goto cleanup;
    }

    pCurve = SymCryptGetCachedEcurve( pInternalParams->eCurveId );
    if( pCurve == NULL )
    {
        goto cleanup;
    }

    pkMlDsakey = SymCryptMlDsakeyAllocate ( pInternalParams->mldsaParams );
    if( pkMlDsakey == NULL )
    {
        goto cleanup;
    }

    pkEckey = SymCryptEckeyAllocate( pCurve );
    if( pkEckey == NULL )
    {
        goto cleanup;
    }

    pkCompositeKey = SymCryptCallbackAlloc( sizeof( SYMCRYPT_COMPOSITE_MLDSAKEY ) );
    if( pkCompositeKey == NULL )
    {
        goto cleanup;
    }

    SymCryptWipeKnownSize( pkCompositeKey, sizeof( SYMCRYPT_COMPOSITE_MLDSAKEY ) );

    pkCompositeKey->pParams = pInternalParams;
    pkCompositeKey->pkMlDsakey = pkMlDsakey;
    pkCompositeKey->pkEckey = pkEckey;

    SYMCRYPT_SET_MAGIC( pkCompositeKey );

    pkMlDsakey = NULL;
    pkEckey = NULL;

cleanup:
    if( pkMlDsakey != NULL )
    {
        SymCryptMlDsakeyFree( pkMlDsakey );
    }

    if( pkEckey != NULL )
    {
        SymCryptEckeyFree( pkEckey );
    }

    return pkCompositeKey;
}

_Use_decl_annotations_
VOID
SYMCRYPT_CALL
SymCryptCompositeMlDsakeyFree(
    PSYMCRYPT_COMPOSITE_MLDSAKEY    pkCompositeMlDsakey )
{
    SYMCRYPT_CHECK_MAGIC( pkCompositeMlDsakey );

    SymCryptMlDsakeyFree( pkCompositeMlDsakey->pkMlDsakey );
    SymCryptEckeyFree( pkCompositeMlDsakey->pkEckey );

    SymCryptWipeKnownSize( pkCompositeMlDsakey, sizeof( SYMCRYPT_COMPOSITE_MLDSAKEY ) );
    SymCryptCallbackFree( pkCompositeMlDsakey );
}

_Use_decl_annotations_
SYMCRYPT_ERROR
SYMCRYPT_CALL
SymCryptCompositeMlDsakeyGenerate(
    PSYMCRYPT_COMPOSITE_MLDSAKEY    pkCompositeMlDsakey,
    UINT32                          flags )
{
    SYMCRYPT_ERROR scError = SYMCRYPT_NO_ERROR;

    // Ensure only allowed flags are specified
    UINT32 allowedFlags = SYMCRYPT_FLAG_KEY_NO_FIPS;

    SYMCRYPT_CHECK_MAGIC( pkCompositeMlDsakey );

    if( (flags & ~allowedFlags) != 0 )
    {
        scError = SYMCRYPT_INVALID_ARGUMENT;
        goto cleanup;
    }

    scError = SymCryptMlDsakeyGenerate( pkCompositeMlDsakey->pkMlDsakey, flags );
    if( scError != SYMCRYPT_NO_ERROR )
    {
        goto cleanup;
    }

    scError = SymCryptEckeySetRandom( flags | SYMCRYPT_FLAG_ECKEY_ECDSA, pkCompositeMlDsakey->pkEckey );
    if( scError != SYMCRYPT_NO_ERROR )
    {
        goto cleanup;
    }

cleanup:
    if( scError != SYMCRYPT_NO_ERROR )
    {
        SymCryptCompositeMlDsaWipePrivateState( pkCompositeMlDsakey );
    }

    return scError;
}

_Use_decl_annotations_
SYMCRYPT_ERROR
SYMCRYPT_CALL
SymCryptCompositeMlDsakeySetValue(
    PCBYTE                              pbSrc,
    SIZE_T                              cbSrc,
    SYMCRYPT_COMPOSITE_MLDSAKEY_FORMAT  compositeMlDsakeyFormat,
    UINT32                              flags,
    PSYMCRYPT_COMPOSITE_MLDSAKEY        pkCompositeMlDsakey )
{
    SYMCRYPT_ERROR scError = SYMCRYPT_NO_ERROR;
    SIZE_T cbEncodedMlDsaKey = 0;
    SIZE_T cbEncodedEcKey = 0;

    // Ensure only allowed flags are specified
    UINT32 allowedFlags = SYMCRYPT_FLAG_KEY_NO_FIPS;

    SYMCRYPT_CHECK_MAGIC( pkCompositeMlDsakey );

    if( (flags & ~allowedFlags) != 0 )
    {
        scError = SYMCRYPT_INVALID_ARGUMENT;
        goto cleanup;
    }

    switch( compositeMlDsakeyFormat )
    {
        case SYMCRYPT_COMPOSITE_MLDSAKEY_FORMAT_PRIVATE_KEY:
            if( cbSrc != pkCompositeMlDsakey->pParams->cbEncodedPrivateKey )
            {
                scError = SYMCRYPT_INVALID_ARGUMENT;
                goto cleanup;
            }

            cbEncodedMlDsaKey = SYMCRYPT_MLDSA_PRIVATE_SEED_SIZE;
            cbEncodedEcKey = SymCryptCompositeGetSizeOfEncodedEcSk( pkCompositeMlDsakey->pParams->eCurveId );
            SYMCRYPT_ASSERT( (cbEncodedMlDsaKey + cbEncodedEcKey) == cbSrc );

            scError = SymCryptMlDsakeySetValue(
                        pbSrc,
                        cbEncodedMlDsaKey,
                        SYMCRYPT_MLDSAKEY_FORMAT_PRIVATE_SEED,
                        flags,
                        pkCompositeMlDsakey->pkMlDsakey );
            if( scError != SYMCRYPT_NO_ERROR )
            {
                goto cleanup;
            }

            scError = SymCryptEckeySetValueCompositeEncodingSk(
                        pkCompositeMlDsakey->pParams->eCurveId,
                        pbSrc + cbEncodedMlDsaKey,
                        cbEncodedEcKey,
                        flags | SYMCRYPT_FLAG_ECKEY_ECDSA,
                        pkCompositeMlDsakey->pkEckey );
            if( scError != SYMCRYPT_NO_ERROR )
            {
                goto cleanup;
            }

            break;
        case SYMCRYPT_COMPOSITE_MLDSAKEY_FORMAT_PUBLIC_KEY:
            //
            // Always wipe the private key state when attempting to set a public key
            //
            SymCryptCompositeMlDsaWipePrivateState( pkCompositeMlDsakey );

            if( cbSrc != pkCompositeMlDsakey->pParams->cbEncodedPublicKey )
            {
                scError = SYMCRYPT_INVALID_ARGUMENT;
                goto cleanup;
            }

            scError = SymCryptMlDsaSizeofKeyFormatFromParams(
                        pkCompositeMlDsakey->pParams->mldsaParams,
                        SYMCRYPT_MLDSAKEY_FORMAT_PUBLIC_KEY,
                        &cbEncodedMlDsaKey );
            if( scError != SYMCRYPT_NO_ERROR )
            {
                goto cleanup;
            }

            cbEncodedEcKey = SymCryptCompositeGetSizeOfEncodedEcPk( pkCompositeMlDsakey->pParams->eCurveId );
            SYMCRYPT_ASSERT( (cbEncodedMlDsaKey + cbEncodedEcKey) == cbSrc );

            scError = SymCryptMlDsakeySetValue(
                        pbSrc,
                        cbEncodedMlDsaKey,
                        SYMCRYPT_MLDSAKEY_FORMAT_PUBLIC_KEY,
                        flags,
                        pkCompositeMlDsakey->pkMlDsakey );
            if( scError != SYMCRYPT_NO_ERROR )
            {
                goto cleanup;
            }

            scError = SymCryptEckeySetValueCompositeEncodingPk(
                        pkCompositeMlDsakey->pParams->eCurveId,
                        pbSrc + cbEncodedMlDsaKey,
                        cbEncodedEcKey,
                        flags | SYMCRYPT_FLAG_ECKEY_ECDSA,
                        pkCompositeMlDsakey->pkEckey );
            if( scError != SYMCRYPT_NO_ERROR )
            {
                goto cleanup;
            }

            break;
        case SYMCRYPT_COMPOSITE_MLDSAKEY_FORMAT_NULL:
            scError = SYMCRYPT_INCOMPATIBLE_FORMAT;
            goto cleanup;
        default:
            scError = SYMCRYPT_INVALID_ARGUMENT;
            goto cleanup;
    }

cleanup:
    if( scError != SYMCRYPT_NO_ERROR )
    {
        SymCryptCompositeMlDsaWipePrivateState( pkCompositeMlDsakey );
    }

    return scError;
}

_Use_decl_annotations_
SYMCRYPT_ERROR
SYMCRYPT_CALL
SymCryptCompositeMlDsakeyGetValue(
    PCSYMCRYPT_COMPOSITE_MLDSAKEY       pkCompositeMlDsakey,
    PBYTE                               pbDst,
    SIZE_T                              cbDst,
    SYMCRYPT_COMPOSITE_MLDSAKEY_FORMAT  compositeMlDsakeyFormat,
    UINT32                              flags )
{
    SYMCRYPT_ERROR scError = SYMCRYPT_NO_ERROR;
    SIZE_T cbEncodedMlDsaKey = 0;
    SIZE_T cbEncodedEcKey = 0;

    SYMCRYPT_CHECK_MAGIC( pkCompositeMlDsakey );

    if( flags != 0 ) // No flags currently supported
    {
        scError = SYMCRYPT_INVALID_ARGUMENT;
        goto cleanup;
    }

    switch( compositeMlDsakeyFormat )
    {
        case SYMCRYPT_COMPOSITE_MLDSAKEY_FORMAT_PRIVATE_KEY:
            if( cbDst != pkCompositeMlDsakey->pParams->cbEncodedPrivateKey )
            {
                scError = SYMCRYPT_INVALID_ARGUMENT;
                goto cleanup;
            }

            cbEncodedMlDsaKey = SYMCRYPT_MLDSA_PRIVATE_SEED_SIZE;
            cbEncodedEcKey = SymCryptCompositeGetSizeOfEncodedEcSk( pkCompositeMlDsakey->pParams->eCurveId );
            SYMCRYPT_ASSERT( (cbEncodedMlDsaKey + cbEncodedEcKey) == cbDst );

            scError = SymCryptMlDsakeyGetValue(
                        pkCompositeMlDsakey->pkMlDsakey,
                        pbDst,
                        cbEncodedMlDsaKey,
                        SYMCRYPT_MLDSAKEY_FORMAT_PRIVATE_SEED,
                        0 );
            if( scError != SYMCRYPT_NO_ERROR )
            {
                goto cleanup;
            }

            scError = SymCryptEckeyGetValueCompositeEncodingSk(
                        pkCompositeMlDsakey->pkEckey,
                        pkCompositeMlDsakey->pParams->eCurveId,
                        pbDst + cbEncodedMlDsaKey,
                        cbEncodedEcKey );
            if( scError != SYMCRYPT_NO_ERROR )
            {
                goto cleanup;
            }

            break;
        case SYMCRYPT_COMPOSITE_MLDSAKEY_FORMAT_PUBLIC_KEY:
            if( cbDst != pkCompositeMlDsakey->pParams->cbEncodedPublicKey )
            {
                scError = SYMCRYPT_INVALID_ARGUMENT;
                goto cleanup;
            }

            scError = SymCryptMlDsaSizeofKeyFormatFromParams(
                        pkCompositeMlDsakey->pParams->mldsaParams,
                        SYMCRYPT_MLDSAKEY_FORMAT_PUBLIC_KEY,
                        &cbEncodedMlDsaKey );
            if( scError != SYMCRYPT_NO_ERROR )
            {
                goto cleanup;
            }

            cbEncodedEcKey = SymCryptCompositeGetSizeOfEncodedEcPk( pkCompositeMlDsakey->pParams->eCurveId );
            SYMCRYPT_ASSERT( (cbEncodedMlDsaKey + cbEncodedEcKey) == cbDst );

            scError = SymCryptMlDsakeyGetValue(
                        pkCompositeMlDsakey->pkMlDsakey,
                        pbDst,
                        cbEncodedMlDsaKey,
                        SYMCRYPT_MLDSAKEY_FORMAT_PUBLIC_KEY,
                        0 );
            if( scError != SYMCRYPT_NO_ERROR )
            {
                goto cleanup;
            }

            scError = SymCryptEckeyGetValueCompositeEncodingPk(
                        pkCompositeMlDsakey->pkEckey,
                        pkCompositeMlDsakey->pParams->eCurveId,
                        pbDst + cbEncodedMlDsaKey,
                        cbEncodedEcKey );
            if( scError != SYMCRYPT_NO_ERROR )
            {
                goto cleanup;
            }

            break;
        case SYMCRYPT_COMPOSITE_MLDSAKEY_FORMAT_NULL:
            scError = SYMCRYPT_INCOMPATIBLE_FORMAT;
            goto cleanup;
        default:
            scError = SYMCRYPT_INVALID_ARGUMENT;
            goto cleanup;
    }

cleanup:
    if( scError != SYMCRYPT_NO_ERROR )
    {
        SymCryptWipe( pbDst, cbDst );
    }

    return scError;
}

_Use_decl_annotations_
SYMCRYPT_ERROR
SYMCRYPT_CALL
SymCryptCompositeMlDsaSign(
    PCSYMCRYPT_COMPOSITE_MLDSAKEY   pkCompositeMlDsakey,
    PCBYTE                          pbMessage,
    SIZE_T                          cbMessage,
    PCBYTE                          pbContext,
    SIZE_T                          cbContext,
    UINT32                          flags,
    PBYTE                           pbSignature,
    SIZE_T                          cbSignature,
    SIZE_T*                         pcbResult )
{
    SYMCRYPT_ERROR scError = SYMCRYPT_NO_ERROR;
    PBYTE pbMessageRepresentative = NULL;
    SIZE_T cbMessageRepresentative = 0;
    PBYTE pbMessageRepresentativeHash = NULL;
    SIZE_T cbMessageRepresentativeHash = 0;
    PBYTE pbMlDsaSignature = pbSignature;
    SIZE_T cbMlDsaSignature = 0;
    PBYTE pbRawEcDsaSignature = NULL;
    SIZE_T cbRawEcDsaSignature = 0;
    PBYTE pbEncodedEcDsaSignature = NULL;
    SIZE_T cbEncodedEcDsaSignature = 0;
    BYTE rgbScratch[SYMCRYPT_COMPOSITE_MLDSA_SIZEOF_SCRATCH_MAX] = {0};
    SIZE_T cbResult = 0;
    UINT8 cbContextU8 = 0;

    // Ensure only allowed flags are specified
    UINT32 allowedFlags = SYMCRYPT_FLAG_COMPOSITE_MLDSA_PREHASHED;
    BOOLEAN bMessageIsPreHashed = (flags & SYMCRYPT_FLAG_COMPOSITE_MLDSA_PREHASHED) != 0;

    *pcbResult = 0;

    SYMCRYPT_CHECK_MAGIC( pkCompositeMlDsakey );

    if( ((flags & ~allowedFlags) != 0) ||
        (cbContext > SYMCRYPT_COMPOSITE_MLDSA_CONTEXT_MAX_LENGTH) ||
        (cbSignature != pkCompositeMlDsakey->pParams->cbEncodedSignatureMax) ||
        (bMessageIsPreHashed && (cbMessage != pkCompositeMlDsakey->pParams->cbMessagePreHash)) )
    {
        scError = SYMCRYPT_INVALID_ARGUMENT;
        goto cleanup;
    }

    cbContextU8 = (UINT8)cbContext;
    cbMessageRepresentativeHash = pkCompositeMlDsakey->pParams->cbMessageRepresentativeHash;
    cbRawEcDsaSignature = 2 * SymCryptEcurveSizeofFieldElement( pkCompositeMlDsakey->pkEckey->pCurve );

    scError = SymCryptMlDsaSizeofSignatureFromParams(
                pkCompositeMlDsakey->pParams->mldsaParams,
                &cbMlDsaSignature );
    if( scError != SYMCRYPT_NO_ERROR )
    {
        goto cleanup;
    }

    //
    // M' :=  Prefix || Label || len(ctx) || ctx || PH( M )
    //
    cbMessageRepresentative += c_cbPrefix;
    cbMessageRepresentative += pkCompositeMlDsakey->pParams->cbLabel;
    cbMessageRepresentative += sizeof( cbContextU8 );
    cbMessageRepresentative += cbContext;
    cbMessageRepresentative += pkCompositeMlDsakey->pParams->cbMessagePreHash;

    SYMCRYPT_ASSERT(
        (cbMessageRepresentative + cbMessageRepresentativeHash + cbRawEcDsaSignature) <= sizeof( rgbScratch ) );

    pbMessageRepresentative = rgbScratch;

    memcpy( pbMessageRepresentative, c_pszPrefix, c_cbPrefix );
    pbMessageRepresentative += c_cbPrefix;

    memcpy( pbMessageRepresentative, pkCompositeMlDsakey->pParams->pcszLabel, pkCompositeMlDsakey->pParams->cbLabel );
    pbMessageRepresentative += pkCompositeMlDsakey->pParams->cbLabel;

    memcpy( pbMessageRepresentative, &cbContextU8, sizeof( cbContextU8 ) );
    pbMessageRepresentative += sizeof( cbContextU8 );

    memcpy( pbMessageRepresentative, pbContext, cbContext );
    pbMessageRepresentative += cbContext;

    if( bMessageIsPreHashed )
    {
        memcpy( pbMessageRepresentative, pbMessage, cbMessage );
    }
    else
    {
        SymCryptCompositeMlDsaHash(
            pkCompositeMlDsakey->pParams->messagePreHashId,
            pbMessage,
            cbMessage,
            pbMessageRepresentative );
    }

    pbMessageRepresentative = rgbScratch;
    pbMessageRepresentativeHash = rgbScratch + cbMessageRepresentative;
    pbRawEcDsaSignature = rgbScratch + cbMessageRepresentative + cbMessageRepresentativeHash;

    SymCryptCompositeMlDsaHash(
        pkCompositeMlDsakey->pParams->messageRepresentativeHashId,
        pbMessageRepresentative,
        cbMessageRepresentative,
        pbMessageRepresentativeHash );

    scError = SymCryptMlDsaSign(
                pkCompositeMlDsakey->pkMlDsakey,
                pbMessageRepresentative,
                cbMessageRepresentative,
                (PCBYTE)pkCompositeMlDsakey->pParams->pcszLabel,
                pkCompositeMlDsakey->pParams->cbLabel,
                0, // flags
                pbMlDsaSignature,
                cbMlDsaSignature );
    if( scError != SYMCRYPT_NO_ERROR )
    {
        goto cleanup;
    }

    scError = SymCryptEcDsaSign(
                pkCompositeMlDsakey->pkEckey,
                pbMessageRepresentativeHash,
                cbMessageRepresentativeHash,
                SYMCRYPT_NUMBER_FORMAT_MSB_FIRST,
                0, // flags
                pbRawEcDsaSignature,
                cbRawEcDsaSignature );
    if( scError != SYMCRYPT_NO_ERROR )
    {
        goto cleanup;
    }

    pbEncodedEcDsaSignature = pbSignature + cbMlDsaSignature;
    cbEncodedEcDsaSignature = cbSignature - cbMlDsaSignature;

    scError = SymCryptEcdsaSigValueCompositeEncode(
                pbRawEcDsaSignature,
                cbRawEcDsaSignature,
                pbEncodedEcDsaSignature,
                cbEncodedEcDsaSignature,
                &cbResult );
    if( scError != SYMCRYPT_NO_ERROR )
    {
        goto cleanup;
    }

    *pcbResult = cbMlDsaSignature + cbResult;

cleanup:
    SymCryptWipeKnownSize( rgbScratch, sizeof( rgbScratch ) );

    if( scError != SYMCRYPT_NO_ERROR )
    {
        SymCryptWipe( pbSignature, cbSignature );
    }

    return scError;
}

_Use_decl_annotations_
SYMCRYPT_ERROR
SYMCRYPT_CALL
SymCryptCompositeMlDsaVerify(
    PCSYMCRYPT_COMPOSITE_MLDSAKEY   pkCompositeMlDsakey,
    PCBYTE                          pbMessage,
    SIZE_T                          cbMessage,
    PCBYTE                          pbContext,
    SIZE_T                          cbContext,
    PCBYTE                          pbSignature,
    SIZE_T                          cbSignature,
    UINT32                          flags )
{
    SYMCRYPT_ERROR scError = SYMCRYPT_NO_ERROR;
    PBYTE pbMessageRepresentative = NULL;
    SIZE_T cbMessageRepresentative = 0;
    PBYTE pbMessageRepresentativeHash = NULL;
    SIZE_T cbMessageRepresentativeHash = 0;
    PCBYTE pbMlDsaSignature = pbSignature;
    SIZE_T cbMlDsaSignature = 0;
    PBYTE pbRawEcDsaSignature = NULL;
    SIZE_T cbRawEcDsaSignature = 0;
    PCBYTE pbEncodedEcDsaSignature = NULL;
    SIZE_T cbEncodedEcDsaSignature = 0;
    BYTE rgbScratch[SYMCRYPT_COMPOSITE_MLDSA_SIZEOF_SCRATCH_MAX] = {0};
    UINT8 cbContextU8 = 0;

    // Ensure only allowed flags are specified
    UINT32 allowedFlags = SYMCRYPT_FLAG_COMPOSITE_MLDSA_PREHASHED;
    BOOLEAN bMessageIsPreHashed = (flags & SYMCRYPT_FLAG_COMPOSITE_MLDSA_PREHASHED) != 0;

    SYMCRYPT_CHECK_MAGIC( pkCompositeMlDsakey );

    scError = SymCryptMlDsaSizeofSignatureFromParams(
                pkCompositeMlDsakey->pParams->mldsaParams,
                &cbMlDsaSignature );
    if( scError != SYMCRYPT_NO_ERROR )
    {
        goto cleanup;
    }

    if( ((flags & ~allowedFlags) != 0) ||
        (cbContext > SYMCRYPT_COMPOSITE_MLDSA_CONTEXT_MAX_LENGTH) ||
        (bMessageIsPreHashed && (cbMessage != pkCompositeMlDsakey->pParams->cbMessagePreHash)) ||
        (cbSignature <= cbMlDsaSignature) ||
        (cbSignature > pkCompositeMlDsakey->pParams->cbEncodedSignatureMax) )
    {
        scError = SYMCRYPT_INVALID_ARGUMENT;
        goto cleanup;
    }

    cbContextU8 = (UINT8)cbContext;
    cbMessageRepresentativeHash = pkCompositeMlDsakey->pParams->cbMessageRepresentativeHash;
    cbRawEcDsaSignature = 2 * SymCryptEcurveSizeofFieldElement( pkCompositeMlDsakey->pkEckey->pCurve );

    //
    // M' :=  Prefix || Label || len(ctx) || ctx || PH( M )
    //
    cbMessageRepresentative += c_cbPrefix;
    cbMessageRepresentative += pkCompositeMlDsakey->pParams->cbLabel;
    cbMessageRepresentative += sizeof( cbContextU8 );
    cbMessageRepresentative += cbContext;
    cbMessageRepresentative += pkCompositeMlDsakey->pParams->cbMessagePreHash;

    SYMCRYPT_ASSERT(
        (cbMessageRepresentative + cbMessageRepresentativeHash + cbRawEcDsaSignature) <= sizeof( rgbScratch ) );

    pbMessageRepresentative = rgbScratch;

    memcpy( pbMessageRepresentative, c_pszPrefix, c_cbPrefix );
    pbMessageRepresentative += c_cbPrefix;

    memcpy( pbMessageRepresentative, pkCompositeMlDsakey->pParams->pcszLabel, pkCompositeMlDsakey->pParams->cbLabel );
    pbMessageRepresentative += pkCompositeMlDsakey->pParams->cbLabel;

    memcpy( pbMessageRepresentative, &cbContextU8, sizeof( cbContextU8 ) );
    pbMessageRepresentative += sizeof( cbContextU8 );

    memcpy( pbMessageRepresentative, pbContext, cbContext );
    pbMessageRepresentative += cbContext;

    if( bMessageIsPreHashed )
    {
        memcpy( pbMessageRepresentative, pbMessage, cbMessage );
    }
    else
    {
        SymCryptCompositeMlDsaHash(
            pkCompositeMlDsakey->pParams->messagePreHashId,
            pbMessage,
            cbMessage,
            pbMessageRepresentative );
    }

    pbMessageRepresentative = rgbScratch;
    pbMessageRepresentativeHash = rgbScratch + cbMessageRepresentative;
    pbRawEcDsaSignature = rgbScratch + cbMessageRepresentative + cbMessageRepresentativeHash;

    pbEncodedEcDsaSignature = pbSignature + cbMlDsaSignature;
    cbEncodedEcDsaSignature = cbSignature - cbMlDsaSignature;

    SymCryptCompositeMlDsaHash(
        pkCompositeMlDsakey->pParams->messageRepresentativeHashId,
        pbMessageRepresentative,
        cbMessageRepresentative,
        pbMessageRepresentativeHash );

    scError = SymCryptEcdsaSigValueCompositeDecode(
                pbEncodedEcDsaSignature,
                cbEncodedEcDsaSignature,
                pbRawEcDsaSignature,
                cbRawEcDsaSignature );
    if( scError != SYMCRYPT_NO_ERROR )
    {
        scError = SYMCRYPT_SIGNATURE_VERIFICATION_FAILURE;
        goto cleanup;
    }

    scError = SymCryptMlDsaVerify(
                pkCompositeMlDsakey->pkMlDsakey,
                pbMessageRepresentative,
                cbMessageRepresentative,
                (PCBYTE)pkCompositeMlDsakey->pParams->pcszLabel,
                pkCompositeMlDsakey->pParams->cbLabel,
                pbMlDsaSignature,
                cbMlDsaSignature,
                0 ); // flags
    if( scError != SYMCRYPT_NO_ERROR )
    {
        goto cleanup;
    }

    scError = SymCryptEcDsaVerify(
        pkCompositeMlDsakey->pkEckey,
        pbMessageRepresentativeHash,
        cbMessageRepresentativeHash,
        pbRawEcDsaSignature,
        cbRawEcDsaSignature,
        SYMCRYPT_NUMBER_FORMAT_MSB_FIRST,
        0 ); // flags
    if( scError != SYMCRYPT_NO_ERROR )
    {
        goto cleanup;
    }

cleanup:
    SymCryptWipeKnownSize( rgbScratch, sizeof( rgbScratch ) );

    return scError;
}
