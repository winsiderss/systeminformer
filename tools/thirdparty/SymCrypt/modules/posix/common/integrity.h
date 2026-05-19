//
// integrity.h
// FIPS 140-3 integrity verification header for ELF binaries
//
// Copyright (c) Microsoft Corporation. Licensed under the MIT license.
//

VOID SymCryptModuleVerifyIntegrity(void);
//
// This function verifies the integrity of the loadable segments of the SymCrypt ELF module using
// HMAC-SHA256. The module must have been postprocessed after compilation using
// process_fips_module.py. The integrity check finds the module's base address in memory by
// subtracting the relative virtual address of a known variable from its actual address in memory.
// It then uses the ELF header to find all the loadable segments in the module and calculate the
// HMAC-SHA256 digest of these segments. For writeable segments which are subject to relocations,
// the relocations will be reversed prior to being added to the HMAC, so that the HMAC input will
// match the contents of the file on disk prior to relocation.
//
// If the integrity check fails for any reason, the module will fastfail, crashing the process,
// since a failed integrity check means it cannot operate in compliance with FIPS 140-3.
//

