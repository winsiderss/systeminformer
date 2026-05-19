//
// SymCrypt FIPS Status Indicator common text for Posix modules
//
// Copyright (c) Microsoft Corporation. Licensed under the MIT license.
//


#define FIPS_SERVICE_DESC_ALGORITHM_PROVIDERS_AND_PROPERTIES

#define FIPS_SERVICE_DESC_ENCRYPTION_AND_DECRYPTION \
        FIPS_SERVICE_DESC("For symmetric encryption, SymCryptAes* functions are allowed. For functions that take a parameter of type PCSYMCRYPT_BLOCKCIPHER, a reference to SymCryptAesBlockCipher must be provided.") \
        FIPS_SERVICE_DESC("For asymmetric encryption, SymCryptRsaOaepEncrypt and SymCryptRsaOaepDecrypt are allowed.")

#define FIPS_SERVICE_DESC_HASHING_AND_MESSAGE_AUTHENTICATION \
        FIPS_SERVICE_DESC("SymCryptHmacMd5 and SymCryptHmacSha1 functions are not allowed.") \
        FIPS_SERVICE_DESC("For SymCryptHash*, and SymCryptHmac* functions other than the two mentioned above, PCSYMCRYPT_HASH parameter cannot be specified as a reference to one of SymCryptMd2Algorithm, SymCryptMd4Algorithm, SymCryptMd5Algorithm, SymCryptSha1Algorithm.")

#define FIPS_SERVICE_DESC_KEY_AND_KEYPAIR_GENERATION \
        FIPS_SERVICE_DESC("For RSA key-pair generation, pParams parameter must point to a SYMCRYPT_RSA_PARAMS structure whose nBitsOfModulus must be greater than or equal to 2048 when calling SymCryptRsakeyAllocate, SymCryptRsakeyCreate, SymCryptRsakeyGenerate.") \
        FIPS_SERVICE_DESC("For Diffie-Hellman and DSA key-pair generation, nBitsOfP parameter must be set to one of 2048 or 3072, and nBitsOfQ parameter must be set to one of 0 or 256 when calling SymCryptDlgroupCreate.") \
        FIPS_SERVICE_DESC("For ECDH and ECDSA, pCurve parameter must refer to one of SymCryptEcurveParamsNistP256, SymCryptEcurveParamsNistP384, SymCryptEcurveParamsNistP521 when calling SymCryptEckeyAllocate, SymCryptEckeyCreate, SymCryptEckeySetRandom.")

#define FIPS_SERVICE_DESC_KEY_DERIVATION \
        FIPS_SERVICE_DESC("PCSYMCRYPT_MAC parameter must not refer to one of SymCryptMd5HmacAlgorithm, SymCryptHmacSha1Algorithm when calling SymCryptSp800_108, SymCryptTksPrf1_2, SymCryptPbkdf2, SymCryptSskdfMac.") \
        FIPS_SERVICE_DESC("PCSYMCRYPT_HASH parameter must not refer to one of SymCryptMd2Algorithm, SymCryptMd4Algorithm, SymCryptMd5Algorithm, SymCryptSha1Algorithm when calling SymCryptSshKdf, SymCryptSskdfHash.")

#define FIPS_SERVICE_DESC_KEY_ENTRY_AND_OUTPUT \
        FIPS_SERVICE_DESC("For RSA keys, flags parameter must not set SYMCRYPT_FLAG_KEY_NO_FIPS flag, cbModulus parameter must be at least 256 if ppPrimes parameter is not NULL, and at least 128 if ppPrimes parameter is NULL when calling SymCryptRsakeySetValue or SymCryptRsakeySetValueFromPrivateExponent.") \
        FIPS_SERVICE_DESC("For ECDH and ECDSA keys, pEckey->pCurve parameter must refer to one of SymCryptEcurveParamsNistP256, SymCryptEcurveParamsNistP384, SymCryptEcurveParamsNistP521, and flags parameter must not set SYMCRYPT_FLAG_KEY_NO_FIPS flag when calling SymCryptEckeySetValue.") \
        FIPS_SERVICE_DESC("For Diffie-Hellman key-pairs, the call to SymCryptDlgroupSetValueSafePrime must return SYMCRYPT_NO_ERROR before calling SymCryptDlkeySetValue.") \
        FIPS_SERVICE_DESC("For DSA, SymCryptDlgroupSetValue must be called with fipsStandard parameter not equal to SYMCRYPT_DLGROUP_FIPS_NONE and pDlgroup parameter must be created with nBitsofP parameter equal to 2048 or 3072 and nBitsofQ parameter equal to 256 or 0 in a preceding call to SymCryptDlgroupAllocate or SymCryptDlgroupCreate.")

#define FIPS_SERVICE_DESC_RANDOM_NUMBER_GENERATION

#define FIPS_SERVICE_DESC_SELF_TESTS

#define FIPS_SERVICE_DESC_SHOW_STATUS

#define FIPS_SERVICE_DESC_SHOW_VERSION

#define FIPS_SERVICE_DESC_SIGNING_AND_VERIFICATION \
        FIPS_SERVICE_DESC("pkRsakey parameter must be created in a preceding call to SymCryptRsakeyAllocate, SymCryptRsakeyCreate, SymCryptRsakeyGenerate with pParams parameter pointing to a SYMCRYPT_RSA_PARAMS structure whose nBitsOfModulus member is greater than or equal to 2048 when calling SymCryptRsaPkcs1Sign and SymCryptRsaPssSign") \
        FIPS_SERVICE_DESC("pkRsakey parameter must be created in a preceding call to SymCryptRsakeySetValue or SymCryptRsakeySetValueFromPrivateExponent with cbModulus parameter greater than or equal to 128 when calling SymCryptRsaPkcs1Verify and SymCryptRsaPssVerify.") \
        FIPS_SERVICE_DESC("hashAlgorithm parameter may refer to SymCryptSha1Algorithm only when calling SymCryptRsaPssVerify.") \
        FIPS_SERVICE_DESC("pHashOID parameter may refer to the OID of SHA-1 only when calling SymCryptRsaPkcs1Verify.") \
        FIPS_SERVICE_DESC("For DSA, pKey parameter of SymCryptDsaSign and SymCryptDsaVerify must be created with nBitsOfP parameter set to one of 2048 or 3072, and nBitsOfQ parameter set to one of 0 or 256 in a preceding call to SymCryptDlgroupCreate.") \
        FIPS_SERVICE_DESC("For ECDSA, pKey parameter of SymCryptEcDsaSign and SymCryptEcDsaVerify must be created in a preceding call to one of SymCryptEckeyAllocate, SymCryptEckeyCreate, SymCryptEckeySetRandom such that pCurve parameter is referring to one of SymCryptEcurveParamsNistP256, SymCryptEcurveParamsNistP384, SymCryptEcurveParamsNistP521.")

#define FIPS_SERVICE_DESC_SECRET_AGREEMENT \
        FIPS_SERVICE_DESC("For Diffie-Hellman secret agreement using SymCryptDhSecretAgreement, pkPrivate and pbPublic must be created with nBitsOfP parameter set to one of 2048 or 3072, and nBitsOfQ parameter set to one of 0 or 256 in a preceding call to SymCryptDlgroupCreate.") \
        FIPS_SERVICE_DESC("For ECDH, pCurve member of pkPrivate and pkPublic must refer to one of SymCryptEcurveParamsNistP256, SymCryptEcurveParamsNistP384, SymCryptEcurveParamsNistP521 when calling SymCryptEcDhSecretAgreement.")

#define FIPS_SERVICE_DESC_ZEROIZING_CRYPTOGRAPHIC_MATERIAL
