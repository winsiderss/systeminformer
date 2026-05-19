//
// hkdf_selftest.c
//
// Copyright (c) Microsoft Corporation. Licensed under the MIT license.
//

#include "precomp.h"

static const BYTE pbResultHkdfSha512[] =
{
    0xdf, 0xcd, 0x33, 0xf4, 0x16, 0x4b, 0x5a, 0x66, 0x61, 0x66, 0x85, 0xff, 0xd3,
};

VOID
SYMCRYPT_CALL
SymCryptHkdfSelfTest(void)
{
    SYMCRYPT_ALIGN BYTE rbResult[sizeof(pbResultHkdfSha512)];

    SymCryptHkdf(
        SymCryptHmacSha512Algorithm,
        &SymCryptTestKey32[0], 15,
        &SymCryptTestKey32[16], 16,
        &SymCryptTestMsg3[0], 3,
        rbResult, sizeof(pbResultHkdfSha512));

    SymCryptInjectError(rbResult, sizeof(pbResultHkdfSha512));

    if (memcmp(rbResult, pbResultHkdfSha512, sizeof(pbResultHkdfSha512)) != 0)
    {
        SymCryptFatal('hkdf');
    }
}
