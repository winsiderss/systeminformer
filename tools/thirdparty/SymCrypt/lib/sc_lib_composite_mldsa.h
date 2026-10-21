//
// sc_lib_composite_mldsa.h
//
// Copyright (c) Microsoft Corporation. Licensed under the MIT license.
//
// Internal Composite-ML-DSA definitions for the SymCrypt library.
// Always intended to be included as part of sc_lib.h
//

typedef struct _SYMCRYPT_COMPOSITE_MLDSA_INTERNAL_PARAMS {
    const CHAR *pcszLabel;                              // Parameter-set label used to build the message representative:
    SIZE_T cbLabel;                                     // M' :=  Prefix || Label || len(ctx) || ctx || PH( M )

    SYMCRYPT_MLDSA_PARAMS mldsaParams;                  // Parameter set for the ML-DSA component.
    SYMCRYPT_CACHED_ECURVE_ID eCurveId;                 // Curve for the EC-DSA component.

    SYMCRYPT_HASH_ID messagePreHashId;                  // Hash algorithm used to pre-hash the message if the caller
    SIZE_T cbMessagePreHash;                            // did not pass SYMCRYPT_FLAG_COMPOSITE_MLDSA_PREHASHED.

    SYMCRYPT_HASH_ID messageRepresentativeHashId;       // Hash algorithm used to compute the message representative
    SIZE_T cbMessageRepresentativeHash;                 // digest passed to the traditional signature component.

    SIZE_T cbEncodedPrivateKey;                         // Encoded private key size.
    SIZE_T cbEncodedPublicKey;                          // Encoded public key size.
    SIZE_T cbEncodedSignatureMax;                       // Maximum size of an encoded signature,
                                                        // which can vary due to DER integer padding.
} SYMCRYPT_COMPOSITE_MLDSA_INTERNAL_PARAMS, * PSYMCRYPT_COMPOSITE_MLDSA_INTERNAL_PARAMS;
typedef const SYMCRYPT_COMPOSITE_MLDSA_INTERNAL_PARAMS* PCSYMCRYPT_COMPOSITE_MLDSA_INTERNAL_PARAMS;

typedef struct _SYMCRYPT_COMPOSITE_MLDSAKEY {
    PCSYMCRYPT_COMPOSITE_MLDSA_INTERNAL_PARAMS pParams; // Pointer to key's parameter set info.
    PSYMCRYPT_MLDSAKEY pkMlDsakey;                      // ML-DSA component key.
    PSYMCRYPT_ECKEY pkEckey;                            // EC-DSA component key.

    SYMCRYPT_MAGIC_FIELD
} SYMCRYPT_COMPOSITE_MLDSAKEY, * PSYMCRYPT_COMPOSITE_MLDSAKEY;
typedef const SYMCRYPT_COMPOSITE_MLDSAKEY* PCSYMCRYPT_COMPOSITE_MLDSAKEY;
