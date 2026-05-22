//
// SymCrypt_modules_common.h
//
// Copyright (c) Microsoft Corporation. Licensed under the MIT license.
//

// This file contains information internal to the SymCrypt modules

// Set this flag to 1 to enable FIPS self-tests in the SymCrypt module.
#ifndef SYMCRYPT_MODULE_DO_FIPS_SELFTESTS
#define SYMCRYPT_MODULE_DO_FIPS_SELFTESTS 1
#endif

// Set this flag to 1 to enable use of FIPS entropy source in the SymCrypt module.
#ifndef SYMCRYPT_MODULE_USE_FIPS_ENTROPY
#define SYMCRYPT_MODULE_USE_FIPS_ENTROPY 1
#endif
