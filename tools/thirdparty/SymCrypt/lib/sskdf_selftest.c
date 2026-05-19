//
// sskdf_selftest.c
//
// Copyright (c) Microsoft Corporation. Licensed under the MIT license.
//

#include "precomp.h"

static const BYTE pbResult[] =
{
    0x30, 0xe5, 0x3e, 0x4c, 0xf6, 0xb1, 0xa5, 0x42, 0x5d, 0xc6, 0x4c, 0xe6, 0x15, 0x7f, 0x5f, 0xa6
};

VOID
SYMCRYPT_CALL
SymCryptSskdfSelfTest(void)
{
    SYMCRYPT_ALIGN BYTE rbResult[sizeof(pbResult)];

    SymCryptSskdfMac(
        SymCryptHmacSha512Algorithm,
        0,
        &SymCryptTestKey32[0], 16,
        &SymCryptTestKey32[16], 16,
        &SymCryptTestMsg16[0], 16,
        rbResult, sizeof(pbResult));

    SymCryptInjectError(rbResult, sizeof(pbResult));

    if (memcmp(rbResult, pbResult, sizeof(pbResult)) != 0)
    {
        SymCryptFatal('sskd');
    }
}
