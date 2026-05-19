//
// composite_encoding.c    Shared encoding/decoding functionality for composite algorithms
//                         according to draft-ietf-lamps-pq-composite-kem-12.
//
// Copyright (c) Microsoft Corporation. Licensed under the MIT license.
//

#include "precomp.h"

UINT32
SYMCRYPT_CALL
SymCryptCompositeGetSizeOfEncodedEcSk( SYMCRYPT_CACHED_ECURVE_ID curveId )
{
    switch (curveId)
    {
        case SYMCRYPT_CACHED_ECURVE_ID_NIST_P256:
            return SYMCRYPT_COMPOSITE_SIZEOF_ENCODED_EC_PRIVATE_KEY_P256;
        case SYMCRYPT_CACHED_ECURVE_ID_NIST_P384:
            return SYMCRYPT_COMPOSITE_SIZEOF_ENCODED_EC_PRIVATE_KEY_P384;
        case SYMCRYPT_CACHED_ECURVE_ID_CURVE_25519:
            return SYMCRYPT_COMPOSITE_SIZEOF_ENCODED_EC_PRIVATE_KEY_CURVE_25519;
        default:
            // curveId is internally specified, so we should never reach this.
            SYMCRYPT_ASSERT( FALSE );
            break;
    }

    return 0;
}

UINT32
SYMCRYPT_CALL
SymCryptCompositeGetSizeOfEncodedEcPk( SYMCRYPT_CACHED_ECURVE_ID curveId )
{
    switch (curveId)
    {
        case SYMCRYPT_CACHED_ECURVE_ID_NIST_P256:
            return SYMCRYPT_COMPOSITE_SIZEOF_ENCODED_EC_PUBLIC_KEY_P256;
        case SYMCRYPT_CACHED_ECURVE_ID_NIST_P384:
            return SYMCRYPT_COMPOSITE_SIZEOF_ENCODED_EC_PUBLIC_KEY_P384;
        case SYMCRYPT_CACHED_ECURVE_ID_CURVE_25519:
            return SYMCRYPT_COMPOSITE_SIZEOF_ENCODED_EC_PUBLIC_KEY_CURVE_25519;
        default:
            // curveId is internally specified, so we should never reach this.
            SYMCRYPT_ASSERT( FALSE );
            break;
    }

    return 0;
}

#define DER_TAG_SEQUENCE        0x30
#define DER_TAG_INTEGER         0x02
#define DER_TAG_OCTET_STRING    0x04
#define DER_TAG_OID             0x06
#define DER_TAG_CONTEXT_0       0xA0

#define DER_INTEGER_VERSION_1   0x01

#define MAX_SINGLE_BYTE_LENGTH  127

// Sequence length does not include 2 sequence tag bytes, so encoding length - 2 <= MAX_SINGLE_BYTE_LENGTH
#define MAX_ENCODING_LENGTH     (2 + MAX_SINGLE_BYTE_LENGTH)

// The context tag includes 2 header bytes for the OID, so OID length + 2 <= MAX_SINGLE_BYTE_LENGTH
#define MAX_OID_LENGTH          (MAX_SINGLE_BYTE_LENGTH - 2)

#define SEC1_TAG_UNCOMPRESSED  0x04

static const BYTE rgbOidP256[] = { 0x2A, 0x86, 0x48, 0xCE, 0x3D, 0x03, 0x01, 0x07 };
static const BYTE rgbOidP384[] = { 0x2B, 0x81, 0x04, 0x00, 0x22 };

// Sequence bytes; version bytes; octet string bytes; context bytes; oid bytes
#define NIST_EC_SK_DER_EXPECTED_ENCODING_SIZE( _cbPrivateKey, _cbOid ) \
    (2 + 3 + (2 + (_cbPrivateKey)) + 2 + (2 + (_cbOid)))

// Only meant to be called from SymCryptEckeyGetValueCompositeEncodingSk
static
SYMCRYPT_ERROR
SYMCRYPT_CALL
SymCryptCompositeEncodeNistEcSkDer(
    _In_                        PCSYMCRYPT_ECKEY    pEckey,
    _In_reads_bytes_( cbOid )   PCBYTE              pbOid,
                                SIZE_T              cbOid,
    _Out_writes_bytes_( cbDst ) PBYTE               pbDst,
                                SIZE_T              cbDst )
{
    SYMCRYPT_ERROR scError = SYMCRYPT_NO_ERROR;
    PBYTE pbCurr = pbDst;

    SIZE_T cbPrivateKey = SymCryptEckeySizeofPrivateKey( pEckey );
    SIZE_T cbEncoding = NIST_EC_SK_DER_EXPECTED_ENCODING_SIZE( cbPrivateKey, cbOid );

    SYMCRYPT_ASSERT( cbDst == cbEncoding );

    UNREFERENCED_PARAMETER( cbDst );

    // Make sure that our assumption that the lengths fit in
    // one byte is correct.
    SYMCRYPT_ASSERT( cbEncoding <= MAX_ENCODING_LENGTH );
    SYMCRYPT_ASSERT( cbPrivateKey <= MAX_SINGLE_BYTE_LENGTH );
    SYMCRYPT_ASSERT( cbOid <= MAX_OID_LENGTH );

    *pbCurr++ = DER_TAG_SEQUENCE;
    *pbCurr++ = (BYTE) (cbEncoding - 2); // Sequence length does not include sequence tag bytes

    *pbCurr++ = DER_TAG_INTEGER;
    *pbCurr++ = 0x01; // length of the version field
    *pbCurr++ = DER_INTEGER_VERSION_1;

    *pbCurr++ = DER_TAG_OCTET_STRING;
    *pbCurr++ = (BYTE) (cbPrivateKey);

    scError = SymCryptEckeyGetValue(
                pEckey,
                pbCurr, cbPrivateKey,
                NULL, 0,
                SYMCRYPT_NUMBER_FORMAT_MSB_FIRST,
                SYMCRYPT_ECPOINT_FORMAT_XY,
                0 );
    if ( scError != SYMCRYPT_NO_ERROR )
    {
        goto cleanup;
    }
    pbCurr += cbPrivateKey;

    *pbCurr++ = DER_TAG_CONTEXT_0;
    *pbCurr++ = (BYTE) (cbOid + 2); // Add two bytes to include OID header information

    *pbCurr++ = DER_TAG_OID;
    *pbCurr++ = (BYTE) (cbOid);
    memcpy( pbCurr, pbOid, cbOid );
    pbCurr += cbOid;

    SYMCRYPT_ASSERT( pbCurr == pbDst + cbDst ); // Asserted earlier that cbDst is as expected

cleanup:
    return scError;
}

SYMCRYPT_ERROR
SYMCRYPT_CALL
SymCryptEckeyGetValueCompositeEncodingSk(
    _In_                        PCSYMCRYPT_ECKEY            pEckey,
                                SYMCRYPT_CACHED_ECURVE_ID   curveId,
    _Out_writes_bytes_( cbDst ) PBYTE                       pbDst,
                                SIZE_T                      cbDst )
{
    SYMCRYPT_ERROR scError = SYMCRYPT_NO_ERROR;

    PCBYTE pbOid = NULL;
    SIZE_T cbOid = 0;

    BOOLEAN fDoNistEcDerEncode = FALSE;

    SYMCRYPT_ASSERT( pEckey->pCurve == SymCryptGetCachedEcurve( curveId ) );

    if ( cbDst != SymCryptCompositeGetSizeOfEncodedEcSk( curveId ) )
    {
        scError = SYMCRYPT_WRONG_KEY_SIZE;
        goto cleanup;
    }

    switch ( curveId )
    {
        case SYMCRYPT_CACHED_ECURVE_ID_NIST_P256:
            pbOid = rgbOidP256;
            cbOid = sizeof(rgbOidP256);
            fDoNistEcDerEncode = TRUE;
            break;
        case SYMCRYPT_CACHED_ECURVE_ID_NIST_P384:
            pbOid = rgbOidP384;
            cbOid = sizeof(rgbOidP384);
            fDoNistEcDerEncode = TRUE;
            break;
        case SYMCRYPT_CACHED_ECURVE_ID_CURVE_25519:
            // Composites with Curve25519 follow a different format from the NIST curves
            // in that we just return the raw key bytes, so no parameters to set.
            break;
        default:
            SYMCRYPT_ASSERT( FALSE );
            scError = SYMCRYPT_INVALID_ARGUMENT;
            goto cleanup;
    }

    if (fDoNistEcDerEncode)
    {
        scError = SymCryptCompositeEncodeNistEcSkDer(
                    pEckey,
                    pbOid, cbOid,
                    pbDst, cbDst );
        if ( scError != SYMCRYPT_NO_ERROR )
        {
            goto cleanup;
        }
    }
    else
    {
        scError = SymCryptEckeyGetValue(
                    pEckey,
                    pbDst, cbDst,
                    NULL, 0,
                    SYMCRYPT_NUMBER_FORMAT_LSB_FIRST,
                    SYMCRYPT_ECPOINT_FORMAT_X,
                    0 );
        if ( scError != SYMCRYPT_NO_ERROR )
        {
            goto cleanup;
        }
    }

cleanup:
    return scError;
}

SYMCRYPT_ERROR
SYMCRYPT_CALL
SymCryptEckeyGetValueCompositeEncodingPk(
    _In_                        PCSYMCRYPT_ECKEY            pEckey,
                                SYMCRYPT_CACHED_ECURVE_ID   curveId,
    _Out_writes_bytes_( cbDst ) PBYTE                       pbDst,
                                SIZE_T                      cbDst )
{
    SYMCRYPT_ERROR scError = SYMCRYPT_NO_ERROR;
    PBYTE pbCurr = pbDst;
    // We don't directly query the public key size here because it's
    // simple enough to just compute it from the encoded size (compared
    // to the private key case, where the encoding rules are a bit more complex).
    SIZE_T cbBytesToWrite = SymCryptCompositeGetSizeOfEncodedEcPk( curveId );

    SYMCRYPT_NUMBER_FORMAT numFormat;
    SYMCRYPT_ECPOINT_FORMAT ecPointFormat;

    SYMCRYPT_ASSERT( pEckey->pCurve == SymCryptGetCachedEcurve( curveId ) );

    if ( cbDst != cbBytesToWrite )
    {
        scError = SYMCRYPT_WRONG_KEY_SIZE;
        goto cleanup;
    }

    switch ( curveId )
    {
        case SYMCRYPT_CACHED_ECURVE_ID_NIST_P256:
        case SYMCRYPT_CACHED_ECURVE_ID_NIST_P384:
            numFormat = SYMCRYPT_NUMBER_FORMAT_MSB_FIRST;
            ecPointFormat = SYMCRYPT_ECPOINT_FORMAT_XY;

            *pbCurr++ = SEC1_TAG_UNCOMPRESSED;
            cbBytesToWrite--;
            break;
        case SYMCRYPT_CACHED_ECURVE_ID_CURVE_25519:
            numFormat = SYMCRYPT_NUMBER_FORMAT_LSB_FIRST;
            ecPointFormat = SYMCRYPT_ECPOINT_FORMAT_X;

            // Curve25519 does not have a prefixed tag
            break;
        default:
            SYMCRYPT_ASSERT( FALSE );
            scError = SYMCRYPT_INVALID_ARGUMENT;
            goto cleanup;
    }

    scError = SymCryptEckeyGetValue(
                pEckey,
                NULL, 0,
                pbCurr, cbBytesToWrite,
                numFormat,
                ecPointFormat,
                0 );
    if ( scError != SYMCRYPT_NO_ERROR )
    {
        goto cleanup;
    }

cleanup:
    return scError;
}

SYMCRYPT_ERROR
SYMCRYPT_CALL
SymCryptEckeySetValueCompositeEncodingPk(
    _In_                        SYMCRYPT_CACHED_ECURVE_ID   curveId,
    _In_reads_bytes_( cbSrc )   PCBYTE                      pbSrc,
                                SIZE_T                      cbSrc,
                                UINT32                      flags,
    _Inout_                     PSYMCRYPT_ECKEY             pEckey )
{
    SYMCRYPT_ERROR scError = SYMCRYPT_NO_ERROR;
    PCBYTE pbCurr = pbSrc;
    SIZE_T cbBytesToRead = SymCryptCompositeGetSizeOfEncodedEcPk( curveId );

    SYMCRYPT_NUMBER_FORMAT numFormat;
    SYMCRYPT_ECPOINT_FORMAT ecPointFormat;

    SYMCRYPT_ASSERT( pEckey->pCurve == SymCryptGetCachedEcurve( curveId ) );

    if ( cbSrc != cbBytesToRead )
    {
        scError = SYMCRYPT_WRONG_KEY_SIZE;
        goto cleanup;
    }

    switch ( curveId )
    {
        case SYMCRYPT_CACHED_ECURVE_ID_NIST_P256:
        case SYMCRYPT_CACHED_ECURVE_ID_NIST_P384:
            numFormat = SYMCRYPT_NUMBER_FORMAT_MSB_FIRST;
            ecPointFormat = SYMCRYPT_ECPOINT_FORMAT_XY;

            if ( *pbCurr++ != SEC1_TAG_UNCOMPRESSED )
            {
                scError = SYMCRYPT_INVALID_BLOB;
                goto cleanup;
            }
            cbBytesToRead--;

            break;
        case SYMCRYPT_CACHED_ECURVE_ID_CURVE_25519:
            numFormat = SYMCRYPT_NUMBER_FORMAT_LSB_FIRST;
            ecPointFormat = SYMCRYPT_ECPOINT_FORMAT_X;
            break;
        default:
            SYMCRYPT_ASSERT(FALSE);
            scError = SYMCRYPT_INVALID_ARGUMENT;
            goto cleanup;
    }

    scError = SymCryptEckeySetValue(
                NULL, 0,
                pbCurr, cbBytesToRead,
                numFormat,
                ecPointFormat,
                flags,
                pEckey );
    if ( scError != SYMCRYPT_NO_ERROR )
    {
        goto cleanup;
    }

cleanup:
    return scError;
}

// Only meant to be called from SymCryptEckeySetValueCompositeEncodingSk
static
SYMCRYPT_ERROR
SYMCRYPT_CALL
SymCryptCompositeDecodeNistEcSkDer(
    _In_reads_bytes_( cbSrc )           PCBYTE  pbSrc,
                                        SIZE_T  cbSrc,
    _In_reads_bytes_( cbExpectedOid )   PCBYTE  pbExpectedOid,
                                        SIZE_T  cbExpectedOid,
                                        UINT32  flags,
    _Inout_                             PSYMCRYPT_ECKEY pEckey )
{
    SYMCRYPT_ERROR scError = SYMCRYPT_NO_ERROR;
    PCBYTE pbCurr = pbSrc;

    PCBYTE pbParsedPrivateKey = NULL;
    SIZE_T cbParsedPrivateKey = 0;

    SIZE_T cbPrivateKey = SymCryptEckeySizeofPrivateKey( pEckey );

    SYMCRYPT_ASSERT( cbSrc == NIST_EC_SK_DER_EXPECTED_ENCODING_SIZE( cbPrivateKey, cbExpectedOid ) );

    SYMCRYPT_ASSERT( cbExpectedOid <= MAX_OID_LENGTH );
    SYMCRYPT_ASSERT( cbSrc <= MAX_ENCODING_LENGTH );
    SYMCRYPT_ASSERT( cbPrivateKey <= MAX_SINGLE_BYTE_LENGTH );

    // Validate the sequence tag
    if ( *pbCurr++ != DER_TAG_SEQUENCE ||
         *pbCurr++ != (BYTE) (cbSrc - 2) )
    {
        scError = SYMCRYPT_INVALID_BLOB;
        goto cleanup;
    }

    // Validate the version integer
    if ( *pbCurr++ != DER_TAG_INTEGER ||
         *pbCurr++ != 0x01 ||
         *pbCurr++ != DER_INTEGER_VERSION_1 )
    {
        scError = SYMCRYPT_INVALID_BLOB;
        goto cleanup;
    }

    // Parse the private key octet string header
    if ( *pbCurr++ != DER_TAG_OCTET_STRING )
    {
        scError = SYMCRYPT_INVALID_BLOB;
        goto cleanup;
    }
    cbParsedPrivateKey = *pbCurr++;

    // Parse the private key and remember the location of it
    pbParsedPrivateKey = pbCurr;

    if ( cbParsedPrivateKey != cbPrivateKey )
    {
        scError = SYMCRYPT_INVALID_BLOB;
        goto cleanup;
    }
    pbCurr += cbParsedPrivateKey;

    // Parse the context-specific parameter
    if ( *pbCurr++ != DER_TAG_CONTEXT_0 ||
         *pbCurr++ != (BYTE) (cbExpectedOid + 2) )
    {
        scError = SYMCRYPT_INVALID_BLOB;
        goto cleanup;
    }

    // Parse the OID
    if ( *pbCurr++ != DER_TAG_OID ||
         *pbCurr++ != (BYTE) cbExpectedOid )
    {
        scError = SYMCRYPT_INVALID_BLOB;
        goto cleanup;
    }

    if ( memcmp( pbCurr, pbExpectedOid, cbExpectedOid ) != 0 )
    {
        scError = SYMCRYPT_INVALID_BLOB;
        goto cleanup;
    }
    pbCurr += cbExpectedOid;

    SYMCRYPT_ASSERT( pbCurr == pbSrc + cbSrc ); // Asserted earlier that cbSrc is as expected

    scError = SymCryptEckeySetValue(
                pbParsedPrivateKey, cbParsedPrivateKey,
                NULL, 0,
                SYMCRYPT_NUMBER_FORMAT_MSB_FIRST,
                SYMCRYPT_ECPOINT_FORMAT_XY,
                flags,
                pEckey );
    if ( scError != SYMCRYPT_NO_ERROR )
    {
        goto cleanup;
    }

cleanup:
    return scError;
}

SYMCRYPT_ERROR
SYMCRYPT_CALL
SymCryptEckeySetValueCompositeEncodingSk(
                                SYMCRYPT_CACHED_ECURVE_ID   curveId,
    _In_reads_bytes_( cbSrc )   PCBYTE                      pbSrc,
                                SIZE_T                      cbSrc,
                                UINT32                      flags,
    _Inout_                     PSYMCRYPT_ECKEY             pEckey )
{
    SYMCRYPT_ERROR scError = SYMCRYPT_NO_ERROR;

    PCBYTE pbExpectedOid = NULL;
    SIZE_T cbExpectedOid = 0;

    BOOLEAN fDoNistEcDerDecode = FALSE;

    SYMCRYPT_ASSERT( pEckey->pCurve == SymCryptGetCachedEcurve( curveId ) );

    if ( cbSrc != SymCryptCompositeGetSizeOfEncodedEcSk( curveId ) )
    {
        scError = SYMCRYPT_WRONG_KEY_SIZE;
        goto cleanup;
    }

    switch ( curveId )
    {
        case SYMCRYPT_CACHED_ECURVE_ID_NIST_P256:
            pbExpectedOid = rgbOidP256;
            cbExpectedOid = sizeof(rgbOidP256);
            fDoNistEcDerDecode = TRUE;
            break;
        case SYMCRYPT_CACHED_ECURVE_ID_NIST_P384:
            pbExpectedOid = rgbOidP384;
            cbExpectedOid = sizeof(rgbOidP384);
            fDoNistEcDerDecode = TRUE;
            break;
        case SYMCRYPT_CACHED_ECURVE_ID_CURVE_25519:
            // Composites with Curve25519 follow a different format from the NIST curves
            // in that we just have the raw key bytes, so no parameters to set.
            break;
        default:
            SYMCRYPT_ASSERT( FALSE );
            scError = SYMCRYPT_INVALID_ARGUMENT;
            goto cleanup;
    }

    if ( fDoNistEcDerDecode )
    {
        scError = SymCryptCompositeDecodeNistEcSkDer(
                pbSrc, cbSrc,
                pbExpectedOid, cbExpectedOid,
                flags,
                pEckey );
        if ( scError != SYMCRYPT_NO_ERROR )
        {
            goto cleanup;
        }
    }
    else
    {
        scError = SymCryptEckeySetValue(
                            pbSrc, cbSrc,
                            NULL, 0,
                            SYMCRYPT_NUMBER_FORMAT_LSB_FIRST,
                            SYMCRYPT_ECPOINT_FORMAT_X,
                            flags,
                            pEckey );
        if ( scError != SYMCRYPT_NO_ERROR )
        {
            goto cleanup;
        }
    }

cleanup:
    return scError;
}