//
// composite_mlkem.c    Composite ML-KEM implementation according to draft-ietf-lamps-pq-composite-kem-12
//                      and draft-irtf-cfrg-concrete-hybrid-kems-03
//
//
// Copyright (c) Microsoft Corporation. Licensed under the MIT license.
//

#include "precomp.h"

#define MAX_ECDH_KEM_SHARED_SECRET_SIZE (48)

#define COMPOSITE_MLKEM_SHARED_SECRET_SIZE  (32)

#define IRTF_MLKEM_768_X25519_EXPANDED_SEED_SIZE    (SYMCRYPT_MLKEM_PRIVATE_SEED_SIZE + 32)
#define IRTF_MLKEM_768_P256_EXPANDED_SEED_SIZE      (SYMCRYPT_MLKEM_PRIVATE_SEED_SIZE + 32*4) // Rejection sampling for P256 allows up to 4 attempts to obtain a valid scalar (per IRTF spec)
#define IRTF_MLKEM_1024_P384_EXPANDED_SEED_SIZE     (SYMCRYPT_MLKEM_PRIVATE_SEED_SIZE + 48) // P384 only needs 1 attempt for sufficiently negligible rejection probability (per IRTF spec)
#define MAX_IRTF_COMPOSITE_MLKEM_EXPANDED_SEED_SIZE IRTF_MLKEM_768_P256_EXPANDED_SEED_SIZE

#define MAX_IRTF_EC_SCALAR_SIZE (48)

static const BYTE rgbMlKem768P256Label[] = {
    'M','L','K','E','M','7','6','8','-','P','2','5','6'
};

// The label for x-wing is more clearly represented as hex bytes
// than its ASCII representation (which contains characters that need escaping).
static const BYTE rgbMlKem768X25519Label[] = {
    0x5c, 0x2e, 0x2f, 0x2f, 0x5e, 0x5c
};

static const BYTE rgbMlKem1024P384Label[] = {
    'M','L','K','E','M','1','0','2','4','-','P','3','8','4'
};

static const SYMCRYPT_COMPOSITE_MLKEM_INTERNAL_PARAMS SymCryptCompositeMlKemInternalParamsMlKem768P256 = {
    .params                     = SYMCRYPT_COMPOSITE_MLKEM_PARAMS_MLKEM768_P256,
    .ecurveId                   = SYMCRYPT_CACHED_ECURVE_ID_NIST_P256,
    .mlKemParams                = SYMCRYPT_MLKEM_PARAMS_MLKEM768,
    .pbLabel                    = rgbMlKem768P256Label,
    .cbLabel                    = sizeof(rgbMlKem768P256Label),
    .cbCiphertext               = SYMCRYPT_COMPOSITE_MLKEM_CIPHERTEXT_SIZE_MLKEM768_P256,
    .numFormat                  = SYMCRYPT_NUMBER_FORMAT_MSB_FIRST,
    .ecPointFormat              = SYMCRYPT_ECPOINT_FORMAT_XY,
    .cbExpandedSeed             = IRTF_MLKEM_768_P256_EXPANDED_SEED_SIZE,
    .cbEncodedPrivateKey        = SYMCRYPT_COMPOSITE_MLKEM_LAMPS_PRIVATE_KEY_SIZE_MLKEM768_P256,
    .cbEncodedPublicKey         = SYMCRYPT_COMPOSITE_MLKEM_PUBLIC_KEY_SIZE_MLKEM768_P256,
};

C_ASSERT(
    SYMCRYPT_COMPOSITE_MLKEM_PUBLIC_KEY_SIZE_MLKEM768_P256
        == (SYMCRYPT_MLKEM_ENCAPSULATION_KEY_SIZE_MLKEM768 + SYMCRYPT_COMPOSITE_SIZEOF_ENCODED_EC_PUBLIC_KEY_P256) );

C_ASSERT(
    SYMCRYPT_COMPOSITE_MLKEM_LAMPS_PRIVATE_KEY_SIZE_MLKEM768_P256
        == (SYMCRYPT_MLKEM_PRIVATE_SEED_SIZE + SYMCRYPT_COMPOSITE_SIZEOF_ENCODED_EC_PRIVATE_KEY_P256) );

static const SYMCRYPT_COMPOSITE_MLKEM_INTERNAL_PARAMS SymCryptCompositeMlKemInternalParamsMlKem768X25519 = {
    .params                     = SYMCRYPT_COMPOSITE_MLKEM_PARAMS_MLKEM768_X25519,
    .ecurveId                   = SYMCRYPT_CACHED_ECURVE_ID_CURVE_25519,
    .mlKemParams                = SYMCRYPT_MLKEM_PARAMS_MLKEM768,
    .pbLabel                    = rgbMlKem768X25519Label,
    .cbLabel                    = sizeof(rgbMlKem768X25519Label),
    .cbCiphertext               = SYMCRYPT_COMPOSITE_MLKEM_CIPHERTEXT_SIZE_MLKEM768_X25519,
    .numFormat                  = SYMCRYPT_NUMBER_FORMAT_LSB_FIRST,
    .ecPointFormat              = SYMCRYPT_ECPOINT_FORMAT_X,
    .cbExpandedSeed             = IRTF_MLKEM_768_X25519_EXPANDED_SEED_SIZE,
    .cbEncodedPrivateKey        = SYMCRYPT_COMPOSITE_MLKEM_LAMPS_PRIVATE_KEY_SIZE_MLKEM768_X25519,
    .cbEncodedPublicKey         = SYMCRYPT_COMPOSITE_MLKEM_PUBLIC_KEY_SIZE_MLKEM768_X25519,
};

C_ASSERT(
    SYMCRYPT_COMPOSITE_MLKEM_PUBLIC_KEY_SIZE_MLKEM768_X25519
        == (SYMCRYPT_MLKEM_ENCAPSULATION_KEY_SIZE_MLKEM768 + SYMCRYPT_COMPOSITE_SIZEOF_ENCODED_EC_PUBLIC_KEY_CURVE_25519) );

C_ASSERT(
    SYMCRYPT_COMPOSITE_MLKEM_LAMPS_PRIVATE_KEY_SIZE_MLKEM768_X25519
        == (SYMCRYPT_MLKEM_PRIVATE_SEED_SIZE + SYMCRYPT_COMPOSITE_SIZEOF_ENCODED_EC_PRIVATE_KEY_CURVE_25519) );

static const SYMCRYPT_COMPOSITE_MLKEM_INTERNAL_PARAMS SymCryptCompositeMlKemInternalParamsMlKem1024P384 = {
    .params                     = SYMCRYPT_COMPOSITE_MLKEM_PARAMS_MLKEM1024_P384,
    .ecurveId                   = SYMCRYPT_CACHED_ECURVE_ID_NIST_P384,
    .mlKemParams                = SYMCRYPT_MLKEM_PARAMS_MLKEM1024,
    .pbLabel                    = rgbMlKem1024P384Label,
    .cbLabel                    = sizeof(rgbMlKem1024P384Label),
    .cbCiphertext               = SYMCRYPT_COMPOSITE_MLKEM_CIPHERTEXT_SIZE_MLKEM1024_P384,
    .numFormat                  = SYMCRYPT_NUMBER_FORMAT_MSB_FIRST,
    .ecPointFormat              = SYMCRYPT_ECPOINT_FORMAT_XY,
    .cbExpandedSeed             = IRTF_MLKEM_1024_P384_EXPANDED_SEED_SIZE,
    .cbEncodedPrivateKey        = SYMCRYPT_COMPOSITE_MLKEM_LAMPS_PRIVATE_KEY_SIZE_MLKEM1024_P384,
    .cbEncodedPublicKey         = SYMCRYPT_COMPOSITE_MLKEM_PUBLIC_KEY_SIZE_MLKEM1024_P384,
};

C_ASSERT(
    SYMCRYPT_COMPOSITE_MLKEM_PUBLIC_KEY_SIZE_MLKEM1024_P384
        == (SYMCRYPT_MLKEM_ENCAPSULATION_KEY_SIZE_MLKEM1024 + SYMCRYPT_COMPOSITE_SIZEOF_ENCODED_EC_PUBLIC_KEY_P384) );

C_ASSERT(
    SYMCRYPT_COMPOSITE_MLKEM_LAMPS_PRIVATE_KEY_SIZE_MLKEM1024_P384
        == (SYMCRYPT_MLKEM_PRIVATE_SEED_SIZE + SYMCRYPT_COMPOSITE_SIZEOF_ENCODED_EC_PRIVATE_KEY_P384) );

static
SYMCRYPT_ERROR
SYMCRYPT_CALL
SymCryptCompositeMlKemGetInternalParamsFromParams(
            SYMCRYPT_COMPOSITE_MLKEM_PARAMS             params,
    _Out_   PCSYMCRYPT_COMPOSITE_MLKEM_INTERNAL_PARAMS* ppInternalParams )
{
    SYMCRYPT_ERROR scError = SYMCRYPT_NO_ERROR;

    switch( params )
    {
        case SYMCRYPT_COMPOSITE_MLKEM_PARAMS_MLKEM768_P256:
            *ppInternalParams = &SymCryptCompositeMlKemInternalParamsMlKem768P256;
            break;
        case SYMCRYPT_COMPOSITE_MLKEM_PARAMS_MLKEM768_X25519:
            *ppInternalParams = &SymCryptCompositeMlKemInternalParamsMlKem768X25519;
            break;
        case SYMCRYPT_COMPOSITE_MLKEM_PARAMS_MLKEM1024_P384:
            *ppInternalParams = &SymCryptCompositeMlKemInternalParamsMlKem1024P384;
            break;
        default:
            scError = SYMCRYPT_INVALID_ARGUMENT;
            goto cleanup;
    }

cleanup:
    return scError;
}

PSYMCRYPT_COMPOSITE_MLKEMKEY
SYMCRYPT_CALL
SymCryptCompositeMlKemkeyAllocate(
    SYMCRYPT_COMPOSITE_MLKEM_PARAMS   params )
{
    SYMCRYPT_ERROR scError = SYMCRYPT_NO_ERROR;
    PCSYMCRYPT_COMPOSITE_MLKEM_INTERNAL_PARAMS pInternalParams = NULL;

    PSYMCRYPT_COMPOSITE_MLKEMKEY pKey = NULL;
    PSYMCRYPT_COMPOSITE_MLKEMKEY pResKey = NULL;
    PSYMCRYPT_MLKEMKEY pkMlKemkey = NULL;
    PSYMCRYPT_ECKEY pkEckey = NULL;
    PCSYMCRYPT_ECURVE pCurve = NULL;

    scError = SymCryptCompositeMlKemGetInternalParamsFromParams(params, &pInternalParams);
    if( scError != SYMCRYPT_NO_ERROR )
    {
        goto cleanup;
    }

    pCurve = SymCryptGetCachedEcurve( pInternalParams->ecurveId );
    if (pCurve == NULL)
    {
        goto cleanup;
    }

    pkMlKemkey = SymCryptMlKemkeyAllocate( pInternalParams->mlKemParams );
    if ( pkMlKemkey == NULL )
    {
        goto cleanup;
    }

    pkEckey = SymCryptEckeyAllocate( pCurve );
    if ( pkEckey == NULL )
    {
        goto cleanup;
    }

    pKey = (PSYMCRYPT_COMPOSITE_MLKEMKEY) SymCryptCallbackAlloc( sizeof(SYMCRYPT_COMPOSITE_MLKEMKEY) );
    if ( pKey == NULL )
    {
        goto cleanup;
    }

    SymCryptWipeKnownSize( pKey, sizeof(SYMCRYPT_COMPOSITE_MLKEMKEY) );

    pKey->pParams = pInternalParams;
    pKey->pkEcKey = pkEckey;
    pKey->pkMlKemkey = pkMlKemkey;
    pKey->hasPrivateSeed = FALSE;

    SYMCRYPT_SET_MAGIC( pKey );

    pResKey = pKey;

    pkEckey = NULL;
    pkMlKemkey = NULL;
    pKey = NULL;

cleanup:
    if ( pkEckey != NULL )
    {
        SymCryptEckeyFree( pkEckey );
    }

    if ( pkMlKemkey != NULL )
    {
        SymCryptMlKemkeyFree( pkMlKemkey );
    }

    if ( pKey != NULL )
    {
        SymCryptCallbackFree( pKey );
    }

    return pResKey;
}

VOID
SYMCRYPT_CALL
SymCryptCompositeMlKemkeyFree(
    _Inout_ PSYMCRYPT_COMPOSITE_MLKEMKEY    pkCompositeMlKemkey )
{
    SYMCRYPT_CHECK_MAGIC( pkCompositeMlKemkey );

    SymCryptEckeyFree( pkCompositeMlKemkey->pkEcKey );
    SymCryptMlKemkeyFree( pkCompositeMlKemkey->pkMlKemkey );

    SymCryptWipe( (PBYTE) pkCompositeMlKemkey, sizeof(SYMCRYPT_COMPOSITE_MLKEMKEY) );

    SymCryptCallbackFree( pkCompositeMlKemkey );
}

SYMCRYPT_ERROR
SYMCRYPT_CALL
SymCryptCompositeMlKemSizeofKeyFormatFromParams(
            SYMCRYPT_COMPOSITE_MLKEM_PARAMS     params,
            SYMCRYPT_COMPOSITE_MLKEMKEY_FORMAT  compositeMlKemKeyformat,
    _Out_   SIZE_T*                             pcbKeyFormat )
{
    SYMCRYPT_ERROR scError = SYMCRYPT_NO_ERROR;
    PCSYMCRYPT_COMPOSITE_MLKEM_INTERNAL_PARAMS pInternalParams = NULL;

    if( compositeMlKemKeyformat == SYMCRYPT_COMPOSITE_MLKEMKEY_FORMAT_NULL )
    {
        scError = SYMCRYPT_INCOMPATIBLE_FORMAT;
        goto cleanup;
    }

    scError = SymCryptCompositeMlKemGetInternalParamsFromParams(params, &pInternalParams);
    if( scError != SYMCRYPT_NO_ERROR )
    {
        goto cleanup;
    }

    switch (compositeMlKemKeyformat)
    {
        case SYMCRYPT_COMPOSITE_MLKEMKEY_FORMAT_IRTF_PRIVATE_SEED:
            // In this case we can't derive the size from calling down into ML-KEM
            // and EC key size functions, since this seed is meant to be expanded
            // into the ML-KEM seed and EC key
            *pcbKeyFormat = SYMCRYPT_COMPOSITE_MLKEM_IRTF_PRIVATE_SEED_SIZE;
            break;

        case SYMCRYPT_COMPOSITE_MLKEMKEY_FORMAT_LAMPS_PRIVATE_KEY:
            *pcbKeyFormat = pInternalParams->cbEncodedPrivateKey;
            break;

        case SYMCRYPT_COMPOSITE_MLKEMKEY_FORMAT_PUBLIC_KEY:
            *pcbKeyFormat = pInternalParams->cbEncodedPublicKey;
            break;

        default:
            scError = SYMCRYPT_INVALID_ARGUMENT;
            goto cleanup;
    }

cleanup:
    return scError;
}

SYMCRYPT_ERROR
SYMCRYPT_CALL
SymCryptCompositeMlKemSizeofCiphertextFromParams(
            SYMCRYPT_COMPOSITE_MLKEM_PARAMS params,
    _Out_   SIZE_T*                         pcbCiphertext )
{
    SYMCRYPT_ERROR scError = SYMCRYPT_NO_ERROR;
    PCSYMCRYPT_COMPOSITE_MLKEM_INTERNAL_PARAMS pInternalParams = NULL;

    scError = SymCryptCompositeMlKemGetInternalParamsFromParams(
                params,
                &pInternalParams);
    if( scError != SYMCRYPT_NO_ERROR )
    {
        goto cleanup;
    }

    *pcbCiphertext = pInternalParams->cbCiphertext;

cleanup:
    return scError;
}

static
VOID
SYMCRYPT_CALL
SymCryptCompositeMlKemkeyWipePrivateState(
    _Inout_ PSYMCRYPT_COMPOSITE_MLKEMKEY    pkCompositeMlKemkey )
{
    SymCryptMlKemkeyWipePrivateState( pkCompositeMlKemkey->pkMlKemkey );
    SymCryptEckeyWipePrivateState( pkCompositeMlKemkey->pkEcKey );

    // Wipe top-level private data
    SymCryptWipeKnownSize( pkCompositeMlKemkey->privateSeed, sizeof(pkCompositeMlKemkey->privateSeed) );
    pkCompositeMlKemkey->hasPrivateSeed = FALSE;
}

SYMCRYPT_ERROR
SYMCRYPT_CALL
SymCryptCompositeMlKemkeyGenerate(
    _Inout_ PSYMCRYPT_COMPOSITE_MLKEMKEY    pkCompositeMlKemkey,
            UINT32                          flags )
{
    SYMCRYPT_ERROR scError = SYMCRYPT_NO_ERROR;

    // Ensure only allowed flags are specified
    UINT32 allowedFlags = SYMCRYPT_FLAG_KEY_NO_FIPS;
    UINT32 eckeyFlags = flags | SYMCRYPT_FLAG_ECKEY_ECDH;

    if ( ( flags & ~allowedFlags ) != 0 )
    {
        scError = SYMCRYPT_INVALID_ARGUMENT;
        goto cleanup;
    }

    scError = SymCryptMlKemkeyGenerate( pkCompositeMlKemkey->pkMlKemkey, flags );
    if ( scError != SYMCRYPT_NO_ERROR )
    {
        goto cleanup;
    }

    scError = SymCryptEckeySetRandom( eckeyFlags, pkCompositeMlKemkey->pkEcKey );
    if ( scError != SYMCRYPT_NO_ERROR )
    {
        goto cleanup;
    }

    pkCompositeMlKemkey->hasPrivateSeed = FALSE;

cleanup:
    if ( scError != SYMCRYPT_NO_ERROR )
    {
        SymCryptCompositeMlKemkeyWipePrivateState( pkCompositeMlKemkey );
    }

    return scError;
}

static
VOID
SYMCRYPT_CALL
SymCryptCompositeMlKemExpandPrivateSeed(
    _In_reads_bytes_( cbSeed )              PCBYTE  pbSeed,
                                            SIZE_T  cbSeed,
    _Out_writes_bytes_( cbExpandedSeed )    PBYTE   pbExpandedSeed,
                                            SIZE_T  cbExpandedSeed )
{
    SYMCRYPT_SHAKE256_STATE shakeState;

    SYMCRYPT_ASSERT( cbSeed == SYMCRYPT_COMPOSITE_MLKEM_IRTF_PRIVATE_SEED_SIZE );

    SymCryptShake256Init( &shakeState );
    SymCryptShake256Append( &shakeState, pbSeed, cbSeed );
    SymCryptShake256Extract( &shakeState, pbExpandedSeed, cbExpandedSeed, TRUE );
}

static
SYMCRYPT_ERROR
SYMCRYPT_CALL
SymCryptCompositeMlKemGetRandomScalarForEcKey(
                                    PCSYMCRYPT_COMPOSITE_MLKEM_INTERNAL_PARAMS  pInternalParams,
    _In_reads_bytes_( cbSeed )      PCBYTE                                      pbSeed,
                                    SIZE_T                                      cbSeed,
    _Out_writes_bytes_( cbScalar )  PBYTE                                       pbScalar,
                                    SIZE_T                                      cbScalar )
{
    return SymCryptCompositeMlKemGetRandomScalarForEcKeyEx(
                pInternalParams->ecurveId,
                pInternalParams->numFormat,
                pbSeed, cbSeed,
                pbScalar, cbScalar );
}

// It's convenient to expose this function interface for testing, as test code
// does not have access to the internal parameter definitions. The non-Ex function
// is a thin wrapper that extracts the necessary parameters from the internal params.
SYMCRYPT_ERROR
SYMCRYPT_CALL
SymCryptCompositeMlKemGetRandomScalarForEcKeyEx(
                                    SYMCRYPT_CACHED_ECURVE_ID   ecurveId,
                                    SYMCRYPT_NUMBER_FORMAT      numFormat,
    _In_reads_bytes_( cbSeed )      PCBYTE                      pbSeed,
                                    SIZE_T                      cbSeed,
    _Out_writes_bytes_( cbScalar )  PBYTE                       pbScalar,
                                    SIZE_T                      cbScalar )
{
    SYMCRYPT_ERROR scError = SYMCRYPT_NO_ERROR;

    PCSYMCRYPT_ECURVE pCurve = NULL;
    UINT32 cbScalarSize = 0;
    UINT32 nDigits = 0;
    UINT32 cbIntScalarSize = 0;
    PBYTE pbScratch = NULL;

    PSYMCRYPT_INT piScalarCandidate = NULL;
    PSYMCRYPT_INT piOrder = NULL;

    SIZE_T seedOffset = 0;
    BOOLEAN fFoundValidScalar = FALSE;

    pCurve = SymCryptGetCachedEcurve( ecurveId );
    if ( pCurve == NULL )
    {
        scError = SYMCRYPT_MEMORY_ALLOCATION_FAILURE;
        goto cleanup;
    }

    cbScalarSize = SymCryptEcurveSizeofScalarMultiplier( pCurve );
    nDigits = SymCryptEcurveDigitsofScalarMultiplier( pCurve );
    cbIntScalarSize = SymCryptSizeofIntFromDigits( nDigits );

    SYMCRYPT_ASSERT( cbScalar == (SIZE_T)cbScalarSize );
    SYMCRYPT_ASSERT( cbSeed >= (SIZE_T)cbScalarSize );

    if ( ecurveId == SYMCRYPT_CACHED_ECURVE_ID_CURVE_25519 )
    {
        // For Curve25519, the scalar is just the seed data
        memcpy( pbScalar, pbSeed, cbScalar );

        // SymCrypt expects the key to already be clamped for Curve25519
        pbScalar[0]  &= 0xF8;
        pbScalar[31] &= 0x7F;
        pbScalar[31] |= 0x40;

        goto cleanup;
    }

    // Otherwise, we go through the seed material until we find a scalar
    // that is in the range [1, order-1] (or exhaust the provided seed)

    pbScratch = (PBYTE) SymCryptCallbackAlloc( cbIntScalarSize );
    if ( pbScratch == NULL )
    {
        scError = SYMCRYPT_MEMORY_ALLOCATION_FAILURE;
        goto cleanup;
    }

    piScalarCandidate = SymCryptIntCreate( pbScratch, cbIntScalarSize, nDigits );
    SYMCRYPT_ASSERT( piScalarCandidate != NULL );

    piOrder = SymCryptIntFromModulus( pCurve->GOrd );
    SYMCRYPT_ASSERT( piOrder != NULL );

    while ( seedOffset + cbScalarSize <= cbSeed )
    {
        scError = SymCryptIntSetValue(
                    pbSeed + seedOffset, cbScalarSize,
                    numFormat,
                    piScalarCandidate );
        if ( scError != SYMCRYPT_NO_ERROR )
        {
            goto cleanup;
        }

        if ( !SymCryptIntIsEqualUint32( piScalarCandidate, 0 ) &&
             SymCryptIntIsLessThan( piScalarCandidate, piOrder ) )
        {
            scError = SymCryptIntGetValue(
                        piScalarCandidate,
                        pbScalar, cbScalar,
                        numFormat );
            if ( scError != SYMCRYPT_NO_ERROR )
            {
                goto cleanup;
            }

            fFoundValidScalar = TRUE;
            break;
        }

        seedOffset += cbScalarSize;
    }

    if ( !fFoundValidScalar )
    {
        scError = SYMCRYPT_INVALID_BLOB;
        goto cleanup;
    }

cleanup:
    if ( pbScratch != NULL )
    {
        SymCryptWipe( pbScratch, cbIntScalarSize );
        SymCryptCallbackFree( pbScratch );
    }

    return scError;
}

SYMCRYPT_ERROR
SYMCRYPT_CALL
SymCryptCompositeMlKemkeySetValue(
    _In_reads_bytes_( cbSrc )   PCBYTE                              pbSrc,
                                SIZE_T                              cbSrc,
                                SYMCRYPT_COMPOSITE_MLKEMKEY_FORMAT  compositeMlKemkeyFormat,
                                UINT32                              flags,
    _Inout_                     PSYMCRYPT_COMPOSITE_MLKEMKEY        pkCompositeMlKemkey )
{
    SYMCRYPT_ERROR scError = SYMCRYPT_NO_ERROR;
    PCSYMCRYPT_COMPOSITE_MLKEM_INTERNAL_PARAMS pInternalParams = pkCompositeMlKemkey->pParams;
    UINT32 eckeyFlags = flags | SYMCRYPT_FLAG_ECKEY_ECDH;

    // Only used for IRTF private seed format
    BYTE rgbExpandedSeed[MAX_IRTF_COMPOSITE_MLKEM_EXPANDED_SEED_SIZE];
    BYTE rgbRandomScalar[MAX_IRTF_EC_SCALAR_SIZE];
    SIZE_T cbRandomScalar = 0;

    // Ensure only allowed flags are specified
    UINT32 allowedFlags = SYMCRYPT_FLAG_KEY_NO_FIPS | SYMCRYPT_FLAG_KEY_MINIMAL_VALIDATION;
    SIZE_T cbExpectedFormatSize = 0;

    if ( ( flags & ~allowedFlags ) != 0 )
    {
        scError = SYMCRYPT_INVALID_ARGUMENT;
        goto cleanup;
    }

    // Check that minimal validation flag only specified with no fips
    if ( ( ( flags & SYMCRYPT_FLAG_KEY_NO_FIPS ) == 0 ) &&
         ( ( flags & SYMCRYPT_FLAG_KEY_MINIMAL_VALIDATION ) != 0 ) )
    {
        scError = SYMCRYPT_INVALID_ARGUMENT;
        goto cleanup;
    }

    if( compositeMlKemkeyFormat == SYMCRYPT_COMPOSITE_MLKEMKEY_FORMAT_NULL )
    {
        scError = SYMCRYPT_INCOMPATIBLE_FORMAT;
        goto cleanup;
    }

    scError = SymCryptCompositeMlKemSizeofKeyFormatFromParams(
                pInternalParams->params,
                compositeMlKemkeyFormat,
                &cbExpectedFormatSize );
    if ( scError != SYMCRYPT_NO_ERROR )
    {
        goto cleanup;
    }

    if ( cbSrc != cbExpectedFormatSize )
    {
        scError = SYMCRYPT_WRONG_KEY_SIZE;
        goto cleanup;
    }

    if ( compositeMlKemkeyFormat == SYMCRYPT_COMPOSITE_MLKEMKEY_FORMAT_IRTF_PRIVATE_SEED)
    {
        SYMCRYPT_ASSERT( pInternalParams->cbExpandedSeed <= sizeof(rgbExpandedSeed) );

        SymCryptCompositeMlKemExpandPrivateSeed(
            pbSrc, cbSrc,
            rgbExpandedSeed, pInternalParams->cbExpandedSeed );

        // Set ML-KEM private seed
        scError = SymCryptMlKemkeySetValue(
                    rgbExpandedSeed, SYMCRYPT_MLKEM_PRIVATE_SEED_SIZE,
                    SYMCRYPT_MLKEMKEY_FORMAT_PRIVATE_SEED,
                    flags,
                    pkCompositeMlKemkey->pkMlKemkey );
        if ( scError != SYMCRYPT_NO_ERROR )
        {
            goto cleanup;
        }

        cbRandomScalar = SymCryptEckeySizeofPrivateKey( pkCompositeMlKemkey->pkEcKey );
        SYMCRYPT_ASSERT( cbRandomScalar <= sizeof( rgbRandomScalar ) );

        scError = SymCryptCompositeMlKemGetRandomScalarForEcKey(
                    pInternalParams,
                    rgbExpandedSeed + SYMCRYPT_MLKEM_PRIVATE_SEED_SIZE,
                    pInternalParams->cbExpandedSeed - SYMCRYPT_MLKEM_PRIVATE_SEED_SIZE,
                    rgbRandomScalar, cbRandomScalar );
        if ( scError != SYMCRYPT_NO_ERROR )
        {
            goto cleanup;
        }

        scError = SymCryptEckeySetValue(
                    rgbRandomScalar, cbRandomScalar,
                    NULL, 0,
                    pInternalParams->numFormat,
                    pInternalParams->ecPointFormat,
                    eckeyFlags,
                    pkCompositeMlKemkey->pkEcKey );
        if ( scError != SYMCRYPT_NO_ERROR )
        {
            goto cleanup;
        }

        // Store the IRTF private seed in the key structure so we can retrieve if needed
        memcpy( pkCompositeMlKemkey->privateSeed, pbSrc, cbSrc );
        pkCompositeMlKemkey->hasPrivateSeed = TRUE;

    }
    else if (compositeMlKemkeyFormat == SYMCRYPT_COMPOSITE_MLKEMKEY_FORMAT_LAMPS_PRIVATE_KEY)
    {
        scError = SymCryptMlKemkeySetValue(
                    pbSrc, SYMCRYPT_MLKEM_PRIVATE_SEED_SIZE,
                    SYMCRYPT_MLKEMKEY_FORMAT_PRIVATE_SEED,
                    flags,
                    pkCompositeMlKemkey->pkMlKemkey );
        if ( scError != SYMCRYPT_NO_ERROR )
        {
            goto cleanup;
        }

        scError = SymCryptEckeySetValueCompositeEncodingSk(
                    pInternalParams->ecurveId,
                    pbSrc + SYMCRYPT_MLKEM_PRIVATE_SEED_SIZE, cbSrc - SYMCRYPT_MLKEM_PRIVATE_SEED_SIZE,
                    eckeyFlags,
                    pkCompositeMlKemkey->pkEcKey );
        if ( scError != SYMCRYPT_NO_ERROR )
        {
            goto cleanup;
        }

        pkCompositeMlKemkey->hasPrivateSeed = FALSE;
    }
    else if (compositeMlKemkeyFormat == SYMCRYPT_COMPOSITE_MLKEMKEY_FORMAT_PUBLIC_KEY)
    {
        SIZE_T cbMlkemPubKey = cbSrc - SymCryptCompositeGetSizeOfEncodedEcPk( pInternalParams->ecurveId );
        scError = SymCryptMlKemkeySetValue(
                    pbSrc, cbMlkemPubKey,
                    SYMCRYPT_MLKEMKEY_FORMAT_ENCAPSULATION_KEY,
                    flags,
                    pkCompositeMlKemkey->pkMlKemkey );
        if ( scError != SYMCRYPT_NO_ERROR )
        {
            goto cleanup;
        }

        scError = SymCryptEckeySetValueCompositeEncodingPk(
                    pInternalParams->ecurveId,
                    pbSrc + cbMlkemPubKey, cbSrc - cbMlkemPubKey,
                    eckeyFlags,
                    pkCompositeMlKemkey->pkEcKey );
        if ( scError != SYMCRYPT_NO_ERROR )
        {
            goto cleanup;
        }

        pkCompositeMlKemkey->hasPrivateSeed = FALSE;
    }
    else
    {
        scError = SYMCRYPT_INVALID_ARGUMENT;
        goto cleanup;
    }

cleanup:
    if ( scError != SYMCRYPT_NO_ERROR )
    {
        SymCryptCompositeMlKemkeyWipePrivateState( pkCompositeMlKemkey );
    }

    SymCryptWipeKnownSize( rgbExpandedSeed, sizeof(rgbExpandedSeed) );
    SymCryptWipeKnownSize( rgbRandomScalar, sizeof(rgbRandomScalar) );

    return scError;

}

SYMCRYPT_ERROR
SYMCRYPT_CALL
SymCryptCompositeMlKemkeyGetValue(
    _In_                        PCSYMCRYPT_COMPOSITE_MLKEMKEY       pkCompositeMlKemkey,
    _Out_writes_bytes_( cbDst ) PBYTE                               pbDst,
                                SIZE_T                              cbDst,
                                SYMCRYPT_COMPOSITE_MLKEMKEY_FORMAT  compositeMlKemkeyFormat,
                                UINT32                              flags )
{
    SYMCRYPT_ERROR scError = SYMCRYPT_NO_ERROR;
    UINT32 allowedFlags = 0;
    SIZE_T cbExpectedFormatSize = 0;
    PCSYMCRYPT_COMPOSITE_MLKEM_INTERNAL_PARAMS pInternalParams = pkCompositeMlKemkey->pParams;

    if ( ( flags & ~allowedFlags ) != 0 )
    {
        scError = SYMCRYPT_INVALID_ARGUMENT;
        goto cleanup;
    }

    if( compositeMlKemkeyFormat == SYMCRYPT_COMPOSITE_MLKEMKEY_FORMAT_NULL )
    {
        scError = SYMCRYPT_INCOMPATIBLE_FORMAT;
        goto cleanup;
    }

    scError = SymCryptCompositeMlKemSizeofKeyFormatFromParams(
                pInternalParams->params,
                compositeMlKemkeyFormat,
                &cbExpectedFormatSize );
    if (scError != SYMCRYPT_NO_ERROR )
    {
        goto cleanup;
    }

    if ( cbDst != cbExpectedFormatSize )
    {
        scError = SYMCRYPT_WRONG_KEY_SIZE;
        goto cleanup;
    }

    if ( compositeMlKemkeyFormat == SYMCRYPT_COMPOSITE_MLKEMKEY_FORMAT_IRTF_PRIVATE_SEED )
    {
        if ( !pkCompositeMlKemkey->hasPrivateSeed )
        {
            scError = SYMCRYPT_INCOMPATIBLE_FORMAT;
            goto cleanup;
        }

        memcpy( pbDst, pkCompositeMlKemkey->privateSeed, sizeof(pkCompositeMlKemkey->privateSeed) );
    }
    else if ( compositeMlKemkeyFormat == SYMCRYPT_COMPOSITE_MLKEMKEY_FORMAT_LAMPS_PRIVATE_KEY )
    {
        scError = SymCryptMlKemkeyGetValue(
                    pkCompositeMlKemkey->pkMlKemkey,
                    pbDst, SYMCRYPT_MLKEM_PRIVATE_SEED_SIZE,
                    SYMCRYPT_MLKEMKEY_FORMAT_PRIVATE_SEED,
                    0 );
        if ( scError != SYMCRYPT_NO_ERROR )
        {
            goto cleanup;
        }

        scError = SymCryptEckeyGetValueCompositeEncodingSk(
                    pkCompositeMlKemkey->pkEcKey,
                    pInternalParams->ecurveId,
                    pbDst + SYMCRYPT_MLKEM_PRIVATE_SEED_SIZE, cbDst - SYMCRYPT_MLKEM_PRIVATE_SEED_SIZE );
        if ( scError != SYMCRYPT_NO_ERROR )
        {
            goto cleanup;
        }
    }
    else if ( compositeMlKemkeyFormat == SYMCRYPT_COMPOSITE_MLKEMKEY_FORMAT_PUBLIC_KEY )
    {
        SIZE_T cbMlKemPubKey = cbDst - SymCryptCompositeGetSizeOfEncodedEcPk( pInternalParams->ecurveId );

        scError = SymCryptMlKemkeyGetValue(
                    pkCompositeMlKemkey->pkMlKemkey,
                    pbDst,
                    cbMlKemPubKey,
                    SYMCRYPT_MLKEMKEY_FORMAT_ENCAPSULATION_KEY,
                    0 );
        if ( scError != SYMCRYPT_NO_ERROR )
        {
            goto cleanup;
        }

        scError = SymCryptEckeyGetValueCompositeEncodingPk(
                    pkCompositeMlKemkey->pkEcKey,
                    pInternalParams->ecurveId,
                    pbDst + cbMlKemPubKey, cbDst - cbMlKemPubKey );
        if ( scError != SYMCRYPT_NO_ERROR )
        {
            goto cleanup;
        }
    }
    else
    {
        scError = SYMCRYPT_INVALID_ARGUMENT;
        goto cleanup;
    }

cleanup:
    if ( scError != SYMCRYPT_NO_ERROR )
    {
        SymCryptWipe( pbDst, cbDst );
    }

    return scError;
}

static
SYMCRYPT_ERROR
SYMCRYPT_CALL
SymCryptCompositeEcdhKemEncapsulateEx(
    _In_                                    PSYMCRYPT_ECKEY                             pkEcKey,
                                            PCSYMCRYPT_COMPOSITE_MLKEM_INTERNAL_PARAMS  pInternalParams,
    _In_reads_bytes_opt_( cbRandom )        PCBYTE                                      pbRandom,
                                            SIZE_T                                      cbRandom,
    _Out_writes_bytes_( cbCiphertext )      PBYTE                                       pbCiphertext,
                                            SIZE_T                                      cbCiphertext,
    _Out_writes_bytes_( cbSharedSecret )    PBYTE                                       pbSharedSecret,
                                            SIZE_T                                      cbSharedSecret )
{
    PSYMCRYPT_ECKEY pkEphemeralEcKey = NULL;
    SYMCRYPT_ERROR scError = SYMCRYPT_NO_ERROR;
    UINT32 eckeyFlags = SYMCRYPT_FLAG_ECKEY_ECDH;

    if (pInternalParams->ecurveId == SYMCRYPT_CACHED_ECURVE_ID_CURVE_25519)
    {
        // Curve25519 is not a FIPS-approved curve, so skip any FIPS checks to avoid
        // unnecessary computation.
        eckeyFlags |= SYMCRYPT_FLAG_KEY_NO_FIPS;
    }

    pkEphemeralEcKey = SymCryptEckeyAllocate( pkEcKey->pCurve );
    if( pkEphemeralEcKey == NULL )
    {
        scError = SYMCRYPT_MEMORY_ALLOCATION_FAILURE;
        goto cleanup;
    }

    // If provided, pbRandom is assumed to be a valid private EC key
    if ( pbRandom != NULL )
    {
        scError = SymCryptEckeySetValue(
                    pbRandom, cbRandom,
                    NULL, 0,
                    pInternalParams->numFormat,
                    pInternalParams->ecPointFormat,
                    eckeyFlags,
                    pkEphemeralEcKey );
        if ( scError != SYMCRYPT_NO_ERROR )
        {
            goto cleanup;
        }
    }
    else
    {
        scError = SymCryptEckeySetRandom( eckeyFlags, pkEphemeralEcKey );
        if ( scError != SYMCRYPT_NO_ERROR )
        {
            goto cleanup;
        }
    }

    scError = SymCryptEcDhSecretAgreement(
                pkEphemeralEcKey,
                pkEcKey,
                pInternalParams->numFormat,
                0,
                pbSharedSecret,
                cbSharedSecret );
    if( scError != SYMCRYPT_NO_ERROR )
    {
        goto cleanup;
    }

    scError = SymCryptEckeyGetValueCompositeEncodingPk(
                pkEphemeralEcKey,
                pInternalParams->ecurveId,
                pbCiphertext,
                cbCiphertext );
    if( scError != SYMCRYPT_NO_ERROR )
    {
        goto cleanup;
    }

cleanup:
    if ( pkEphemeralEcKey != NULL )
    {
        SymCryptEckeyFree( pkEphemeralEcKey );
    }
    return scError;
}

SYMCRYPT_ERROR
SYMCRYPT_CALL
SymCryptCompositeMlKemEncapsulateEx(
    _In_                                    PCSYMCRYPT_COMPOSITE_MLKEMKEY   pkCompositeMlKemkey,
    _In_reads_bytes_opt_( cbMlKemRandom )   PCBYTE                          pbMlKemRandom,
                                            SIZE_T                          cbMlKemRandom,
    _In_reads_bytes_opt_( cbTradRandom )    PCBYTE                          pbTradRandom,
                                            SIZE_T                          cbTradRandom,
    _Out_writes_bytes_( cbAgreedSecret )    PBYTE                           pbAgreedSecret,
                                            SIZE_T                          cbAgreedSecret,
    _Out_writes_bytes_( cbCiphertext )      PBYTE                           pbCiphertext,
                                            SIZE_T                          cbCiphertext )
{
    SYMCRYPT_ERROR scError = SYMCRYPT_NO_ERROR;
    PCSYMCRYPT_COMPOSITE_MLKEM_INTERNAL_PARAMS pInternalParams = pkCompositeMlKemkey->pParams;

    PBYTE pbMlKemCiphertext = pbCiphertext;
    SIZE_T cbMlKemCiphertext = 0;

    PBYTE pbEcKeyCiphertext = NULL;
    SIZE_T cbEcKeyCiphertext = 0;

    BYTE rgbSharedSecretEcdh[MAX_ECDH_KEM_SHARED_SECRET_SIZE];
    SIZE_T cbSharedSecretEcdh = SymCryptEcurveSizeofFieldElement( pkCompositeMlKemkey->pkEcKey->pCurve );
    BYTE rgbSharedSecretMlKem[SYMCRYPT_MLKEM_SIZEOF_AGREED_SECRET];

    BYTE rgbEncodedEcPk[SYMCRYPT_COMPOSITE_SIZEOF_MAX_ENCODED_EC_PUBLIC_KEY];
    SIZE_T cbEncodedEcPk = 0;

    SYMCRYPT_SHA3_256_STATE sha3State;

    SYMCRYPT_ASSERT( cbSharedSecretEcdh <= sizeof(rgbSharedSecretEcdh) );

    if ( pbMlKemRandom != NULL && cbMlKemRandom != SYMCRYPT_MLKEM_SIZEOF_ENCAPS_RANDOM )
    {
        scError = SYMCRYPT_INVALID_ARGUMENT;
        goto cleanup;
    }

    if ( pbTradRandom != NULL && cbTradRandom != SymCryptEckeySizeofPrivateKey( pkCompositeMlKemkey->pkEcKey ) )
    {
        scError = SYMCRYPT_INVALID_ARGUMENT;
        goto cleanup;
    }

    if (cbCiphertext != pInternalParams->cbCiphertext ||
        cbAgreedSecret != COMPOSITE_MLKEM_SHARED_SECRET_SIZE )
    {
        scError = SYMCRYPT_INVALID_ARGUMENT;
        goto cleanup;
    }

    scError = SymCryptMlKemSizeofCiphertextFromParams(
                pInternalParams->mlKemParams,
                &cbMlKemCiphertext );
    SYMCRYPT_ASSERT( scError == SYMCRYPT_NO_ERROR );

    if (pbMlKemRandom != NULL)
    {
        scError = SymCryptMlKemEncapsulateEx(
                pkCompositeMlKemkey->pkMlKemkey,
                pbMlKemRandom, cbMlKemRandom,
                rgbSharedSecretMlKem, sizeof(rgbSharedSecretMlKem),
                pbMlKemCiphertext, cbMlKemCiphertext );
        if( scError != SYMCRYPT_NO_ERROR )
        {
            goto cleanup;
        }
    }
    else
    {
        scError = SymCryptMlKemEncapsulate(
                pkCompositeMlKemkey->pkMlKemkey,
                rgbSharedSecretMlKem, sizeof(rgbSharedSecretMlKem),
                pbMlKemCiphertext, cbMlKemCiphertext );
        if( scError != SYMCRYPT_NO_ERROR )
        {
            goto cleanup;
        }
    }

    pbEcKeyCiphertext = pbCiphertext + cbMlKemCiphertext;
    cbEcKeyCiphertext = cbCiphertext - cbMlKemCiphertext;

    scError = SymCryptCompositeEcdhKemEncapsulateEx(
                pkCompositeMlKemkey->pkEcKey,
                pInternalParams,
                pbTradRandom, cbTradRandom,
                pbEcKeyCiphertext, cbEcKeyCiphertext,
                rgbSharedSecretEcdh, cbSharedSecretEcdh );
    if( scError != SYMCRYPT_NO_ERROR )
    {
        goto cleanup;
    }

    cbEncodedEcPk = SymCryptCompositeGetSizeOfEncodedEcPk( pInternalParams->ecurveId );

    SYMCRYPT_ASSERT( cbEncodedEcPk <= sizeof(rgbEncodedEcPk) );

    scError = SymCryptEckeyGetValueCompositeEncodingPk(
                pkCompositeMlKemkey->pkEcKey,
                pInternalParams->ecurveId,
                rgbEncodedEcPk, cbEncodedEcPk );
    if( scError != SYMCRYPT_NO_ERROR )
    {
        goto cleanup;
    }

    SymCryptSha3_256Init( &sha3State );
    SymCryptSha3_256Append( &sha3State, rgbSharedSecretMlKem, sizeof(rgbSharedSecretMlKem) );
    SymCryptSha3_256Append( &sha3State, rgbSharedSecretEcdh, cbSharedSecretEcdh );
    SymCryptSha3_256Append( &sha3State, pbEcKeyCiphertext, cbEcKeyCiphertext );
    SymCryptSha3_256Append( &sha3State, rgbEncodedEcPk, cbEncodedEcPk );
    SymCryptSha3_256Append( &sha3State, pInternalParams->pbLabel, pInternalParams->cbLabel );
    SymCryptSha3_256Result( &sha3State, pbAgreedSecret );

cleanup:
    SymCryptWipeKnownSize( rgbSharedSecretEcdh, sizeof(rgbSharedSecretEcdh) );
    SymCryptWipeKnownSize( rgbSharedSecretMlKem, sizeof(rgbSharedSecretMlKem) );

    return scError;
}

SYMCRYPT_ERROR
SYMCRYPT_CALL
SymCryptCompositeMlKemEncapsulate(
    _In_                                    PCSYMCRYPT_COMPOSITE_MLKEMKEY   pkCompositeMlKemkey,
    _Out_writes_bytes_( cbAgreedSecret )    PBYTE                           pbAgreedSecret,
                                            SIZE_T                          cbAgreedSecret,
    _Out_writes_bytes_( cbCiphertext )      PBYTE                           pbCiphertext,
                                            SIZE_T                          cbCiphertext )
{
    // We pass in NULL parameters to the EncapsulateEx function
    // since we defer to the underlying implementation to generate the randomness.
    return SymCryptCompositeMlKemEncapsulateEx(
            pkCompositeMlKemkey,
            NULL, 0,
            NULL, 0,
            pbAgreedSecret, cbAgreedSecret,
            pbCiphertext, cbCiphertext );
}

static
SYMCRYPT_ERROR
SYMCRYPT_CALL
SymCryptCompositeEcdhKemDecapsulate(
    _In_                                    PSYMCRYPT_ECKEY                             pkEcKey,
                                            PCSYMCRYPT_COMPOSITE_MLKEM_INTERNAL_PARAMS  pInternalParams,
    _In_reads_bytes_( cbCiphertext )        PCBYTE                                      pbCiphertext,
                                            SIZE_T                                      cbCiphertext,
    _Out_writes_bytes_( cbSharedSecret )    PBYTE                                       pbSharedSecret,
                                            SIZE_T                                      cbSharedSecret )
{
    PSYMCRYPT_ECKEY pkEphemeralEcKey = NULL;
    SYMCRYPT_ERROR scError = SYMCRYPT_NO_ERROR;
    UINT32 eckeyFlags = SYMCRYPT_FLAG_ECKEY_ECDH;

    if (pInternalParams->ecurveId == SYMCRYPT_CACHED_ECURVE_ID_CURVE_25519)
    {
        // Curve25519 is not a FIPS-approved curve, so skip any FIPS checks to avoid
        // unnecessary computation.
        eckeyFlags |= SYMCRYPT_FLAG_KEY_NO_FIPS;
    }

    pkEphemeralEcKey = SymCryptEckeyAllocate( pkEcKey->pCurve );
    if( pkEphemeralEcKey == NULL )
    {
        scError = SYMCRYPT_MEMORY_ALLOCATION_FAILURE;
        goto cleanup;
    }

    scError = SymCryptEckeySetValueCompositeEncodingPk(
                pInternalParams->ecurveId,
                pbCiphertext, cbCiphertext,
                eckeyFlags,
                pkEphemeralEcKey );
    if( scError != SYMCRYPT_NO_ERROR )
    {
        goto cleanup;
    }

    scError = SymCryptEcDhSecretAgreement(
                pkEcKey,
                pkEphemeralEcKey,
                pInternalParams->numFormat,
                0,
                pbSharedSecret, cbSharedSecret );
    if( scError != SYMCRYPT_NO_ERROR )
    {
        goto cleanup;
    }

cleanup:
    if ( pkEphemeralEcKey != NULL )
    {
        SymCryptEckeyFree( pkEphemeralEcKey );
    }
    return scError;
}

SYMCRYPT_ERROR
SYMCRYPT_CALL
SymCryptCompositeMlKemDecapsulate(
    _In_                                    PCSYMCRYPT_COMPOSITE_MLKEMKEY   pkCompositeMlKemkey,
    _In_reads_bytes_( cbCiphertext )        PCBYTE                          pbCiphertext,
                                            SIZE_T                          cbCiphertext,
    _Out_writes_bytes_( cbAgreedSecret )    PBYTE                           pbAgreedSecret,
                                            SIZE_T                          cbAgreedSecret )
{
    SYMCRYPT_ERROR scError = SYMCRYPT_NO_ERROR;
    PCSYMCRYPT_COMPOSITE_MLKEM_INTERNAL_PARAMS pInternalParams = pkCompositeMlKemkey->pParams;

    PCBYTE pbMlKemCiphertext = pbCiphertext;
    SIZE_T cbMlKemCiphertext = 0;

    PCBYTE pbEcCiphertext = NULL;
    SIZE_T cbEcCiphertext = 0;

    BYTE rgbSharedSecretEcdh[MAX_ECDH_KEM_SHARED_SECRET_SIZE];
    SIZE_T cbSharedSecretEcdh = SymCryptEcurveSizeofFieldElement( pkCompositeMlKemkey->pkEcKey->pCurve );
    BYTE rgbSharedSecretMlKem[SYMCRYPT_MLKEM_SIZEOF_AGREED_SECRET];

    BYTE rgbEncodedEcPk[SYMCRYPT_COMPOSITE_SIZEOF_MAX_ENCODED_EC_PUBLIC_KEY];
    SIZE_T cbEncodedEcPk = 0;

    SYMCRYPT_SHA3_256_STATE sha3State;

    SYMCRYPT_ASSERT( cbSharedSecretEcdh <= sizeof(rgbSharedSecretEcdh) );

    if (cbCiphertext != pInternalParams->cbCiphertext ||
        cbAgreedSecret != COMPOSITE_MLKEM_SHARED_SECRET_SIZE )
    {
        scError = SYMCRYPT_INVALID_ARGUMENT;
        goto cleanup;
    }

    scError = SymCryptMlKemSizeofCiphertextFromParams(
                pInternalParams->mlKemParams,
                &cbMlKemCiphertext );
    SYMCRYPT_ASSERT( scError == SYMCRYPT_NO_ERROR );

    scError = SymCryptMlKemDecapsulate(
                pkCompositeMlKemkey->pkMlKemkey,
                pbMlKemCiphertext, cbMlKemCiphertext,
                rgbSharedSecretMlKem, sizeof(rgbSharedSecretMlKem) );
    if( scError != SYMCRYPT_NO_ERROR )
    {
        goto cleanup;
    }

    pbEcCiphertext = pbCiphertext + cbMlKemCiphertext;
    cbEcCiphertext = cbCiphertext - cbMlKemCiphertext;

    scError = SymCryptCompositeEcdhKemDecapsulate(
                pkCompositeMlKemkey->pkEcKey,
                pInternalParams,
                pbEcCiphertext, cbEcCiphertext,
                rgbSharedSecretEcdh, cbSharedSecretEcdh );
    if ( scError != SYMCRYPT_NO_ERROR )
    {
        goto cleanup;
    }

    cbEncodedEcPk = SymCryptCompositeGetSizeOfEncodedEcPk( pInternalParams->ecurveId );
    SYMCRYPT_ASSERT( cbEncodedEcPk <= sizeof(rgbEncodedEcPk) );

    scError = SymCryptEckeyGetValueCompositeEncodingPk(
                pkCompositeMlKemkey->pkEcKey,
                pInternalParams->ecurveId,
                rgbEncodedEcPk, cbEncodedEcPk );
    if( scError != SYMCRYPT_NO_ERROR )
    {
        goto cleanup;
    }

    SymCryptSha3_256Init( &sha3State );
    SymCryptSha3_256Append( &sha3State, rgbSharedSecretMlKem, sizeof(rgbSharedSecretMlKem) );
    SymCryptSha3_256Append( &sha3State, rgbSharedSecretEcdh, cbSharedSecretEcdh );
    SymCryptSha3_256Append( &sha3State, pbEcCiphertext, cbEcCiphertext );
    SymCryptSha3_256Append( &sha3State, rgbEncodedEcPk, cbEncodedEcPk );
    SymCryptSha3_256Append( &sha3State, pInternalParams->pbLabel, pInternalParams->cbLabel );
    SymCryptSha3_256Result( &sha3State, pbAgreedSecret );

cleanup:
    SymCryptWipeKnownSize( rgbSharedSecretEcdh, sizeof(rgbSharedSecretEcdh) );
    SymCryptWipeKnownSize( rgbSharedSecretMlKem, sizeof(rgbSharedSecretMlKem) );

    return scError;
}