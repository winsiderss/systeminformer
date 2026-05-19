//
// srtp_kdf_selftest.c
//
// Copyright (c) Microsoft Corporation. Licensed under the MIT license.
//

#include "precomp.h"


static const BYTE pbKey[] =
{
    0xc4, 0x80, 0x9f, 0x6d, 0x36, 0x98, 0x88, 0x72, 0x8e, 0x26, 0xad, 0xb5, 0x32, 0x12, 0x98, 0x90
};

static const BYTE pbSalt[] =
{
    0x0e, 0x23, 0x00, 0x6c, 0x6c, 0x04, 0x4f, 0x56, 0x62, 0x40, 0x0e, 0x9d, 0x1b, 0xd6
};

static const UINT64 uIndex = 0x487165649cca;

static const UINT32 uSRTCPIndex = 0x56f3f197;

static const UINT32 uKeyDerivationRate = 0;

static const BYTE label = SYMCRYPT_SRTP_ENCRYPTION_KEY;

static const BYTE pbResultAes128[] =
{
    0xdc, 0x38, 0x21, 0x92, 0xab, 0x65, 0x10, 0x8a, 0x86, 0xb2, 0x59, 0xb6, 0x1b, 0x3a, 0xf4, 0x6f
};

VOID
SYMCRYPT_CALL
SymCryptSrtpKdfSelfTest(void)
{
    SYMCRYPT_SRTPKDF_EXPANDED_KEY expandedKey;
    SYMCRYPT_ALIGN BYTE rbResultAes128[sizeof(pbResultAes128)];

    SymCryptSrtpKdfExpandKey(&expandedKey, pbKey, sizeof(pbKey));

    SymCryptSrtpKdfDerive(&expandedKey,
                        pbSalt, sizeof(pbSalt),
                        uKeyDerivationRate,
                        (label < SYMCRYPT_SRTCP_ENCRYPTION_KEY) ? uIndex : uSRTCPIndex,
                        (label < SYMCRYPT_SRTCP_ENCRYPTION_KEY) ? 48 : 32,
                        label,
                        rbResultAes128, sizeof(rbResultAes128));

    SymCryptInjectError(rbResultAes128, sizeof(rbResultAes128));

    if (memcmp(rbResultAes128, pbResultAes128, sizeof(pbResultAes128)) != 0)
    {
        SymCryptFatal('srtp');
    }
}
