//
// Parameters for trial division mechanism
// Copyright (c) Microsoft Corporation. Licensed under the MIT license.
// GENERATED FILE, DO NOT EDIT.
//


//
// The primes are put into groups of consecutive primes (skipping 2, 3, 5, and 17).
// Each group has a product less than SYMCRYPT_MAX_SMALL_PRIME_GROUP_PRODUCT which is
// chosen to avoid overflows in the modular reduction computation.
//

typedef struct _SYMCRYPT_SMALL_PRIME_GROUPS_SPEC {
    UINT16 nGroups;    // # groups of this size 
    UINT8  nPrimes;    // # primes in the group 
    UINT32 maxPrime;   // largest prime in the last group 
} SYMCRYPT_SMALL_PRIME_GROUPS_SPEC;

#define SYMCRYPT_MAX_SMALL_PRIME_GROUP_PRODUCT    (0x1c71c71c71c71c71U)

const SYMCRYPT_SMALL_PRIME_GROUPS_SPEC g_SymCryptSmallPrimeGroupsSpec[] = {
    {     1, 12, 53 },
    {     1,  9, 97 },
    {     2,  8, 179 },
    {     6,  7, 431 },
    {    17,  6, 1103 },
    {    87,  5, 4583 },
    {   845,  4, 37813 },
    { 31307,  3, 1270271 },
    {     0,  2, 0xffffffff },
};

