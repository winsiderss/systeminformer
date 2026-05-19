//
// tlsprf_selftest.c
//
// Copyright (c) Microsoft Corporation. Licensed under the MIT license.
//

#include "precomp.h"

static const BYTE pbResult1_1[] =
{
    0x8e, 0x7e, 0x7b, 0x2f, 0x7d, 0x00, 0x77, 0x09, 0x78, 0x22, 0x51, 0xf2, 0xcf,
};

static const BYTE pbResult1_2Sha512[] =
{
    0xf9, 0x9f, 0x44, 0x4d, 0x26, 0xde, 0x8b, 0x4f, 0xc2, 0x81, 0x5b, 0x23, 0x70,
};

VOID
SYMCRYPT_CALL
SymCryptTlsPrf1_1SelfTest(void)
{
    SYMCRYPT_ALIGN BYTE rbResult[sizeof(pbResult1_1)];

    SymCryptTlsPrf1_1(
        &SymCryptTestKey32[0], 15,
        &SymCryptTestKey32[16], 16,
        &SymCryptTestMsg3[0], 3,
        rbResult, sizeof(pbResult1_1));

    SymCryptInjectError(rbResult, sizeof(pbResult1_1));

    if (memcmp(rbResult, pbResult1_1, sizeof(pbResult1_1)) != 0)
    {
        SymCryptFatal('tl11');
    }
}

VOID
SYMCRYPT_CALL
SymCryptTlsPrf1_2SelfTest(void)
{
    SYMCRYPT_ALIGN BYTE rbResult[sizeof(pbResult1_2Sha512)];

    SymCryptTlsPrf1_2(
        SymCryptHmacSha512Algorithm,
        &SymCryptTestKey32[0], 15,
        &SymCryptTestKey32[16], 16,
        &SymCryptTestMsg3[0], 3,
        rbResult, sizeof(pbResult1_2Sha512));

    SymCryptInjectError(rbResult, sizeof(pbResult1_2Sha512));

    if (memcmp(rbResult, pbResult1_2Sha512, sizeof(pbResult1_2Sha512)) != 0)
    {
        SymCryptFatal('tl12');
    }
}
