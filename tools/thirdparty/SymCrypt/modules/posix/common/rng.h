//
// rng.h
// Declares Init and Uninit functions for module versions of SymCrypt
// and declares FIPS and secure entropy source functions
//
// Copyright (c) Microsoft Corporation. Licensed under the MIT license.
//

VOID
SYMCRYPT_CALL
SymCryptRngInit(void);

VOID
SYMCRYPT_CALL
SymCryptRngUninit(void);

//
// Entropy function declarations
// RNG system uses these functions to seed and reseed the rng state, and each module must define
// these, as each module may have different available sources of entropy to use.
// SymCryptEntropyFipsGet should be SP 800-90B compliant, while SymCryptEntropySecureGet should be
// a secure source of entropy (better than or equal to that of the Fips source).
//

// Initializes FIPS entropy source (may do nothing)
VOID
SYMCRYPT_CALL
SymCryptEntropyFipsInit(void);

// Uninitializes FIPS entropy source (may do nothing)
VOID
SYMCRYPT_CALL
SymCryptEntropyFipsUninit(void);

// Fills pbResult with entropy from the FIPS entropy source, which is SP 800-90B compliant source
VOID
SYMCRYPT_CALL
SymCryptEntropyFipsGet( _Out_writes_( cbResult ) PBYTE pbResult, SIZE_T cbResult );

// Initializes secure entropy source (may do nothing)
VOID
SYMCRYPT_CALL
SymCryptEntropySecureInit(void);

// Uninitializes secure entropy source (may do nothing)
VOID
SYMCRYPT_CALL
SymCryptEntropySecureUninit(void);

// Fills pbResult with entropy from the secure entropy source.
VOID
SYMCRYPT_CALL
SymCryptEntropySecureGet( _Out_writes_( cbResult ) PBYTE pbResult, SIZE_T cbResult );

//
// Fork detection function declarations.
// RNG system uses these functions to reseed the RNG state if a fork is detected. Each module
// must define these, as each module may have different constraints around whether forks are
// possible and which system calls are available.
//

// Initializes fork detection system (may do nothing)
VOID
SYMCRYPT_CALL
SymCryptRngForkDetectionInit(void);

// Returns true if a fork has been detected since last call to SymCryptRngForkDetectionInit or
// SymCryptRngForkDetect. (may always return false)
BOOLEAN
SYMCRYPT_CALL
SymCryptRngForkDetect(void);
