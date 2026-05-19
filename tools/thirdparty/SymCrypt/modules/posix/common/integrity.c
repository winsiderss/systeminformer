//
// integrity.c
// FIPS 140-3 integrity verification implementation for ELF binaries
//
// Copyright (c) Microsoft Corporation. Licensed under the MIT license.
//


#include "precomp.h"

#if UINTPTR_MAX == 0xFFFFFFFF
#define Elf_Shdr          Elf32_Shdr
#define Elf_Phdr          Elf32_Phdr
#define Elf_Sym           Elf32_Sym
#define Elf_Dyn           Elf32_Dyn
#define Elf_Ehdr          Elf32_Ehdr
#define Elf_Addr          Elf32_Addr
#define Elf_Off           Elf32_Off
#define Elf_Rel           Elf32_Rel
#define Elf_Rela          Elf32_Rela
#define ELF_R_TYPE(X)     ELF32_R_TYPE(X)
#define ELF_R_SYM(X)      ELF32_R_SYM(X)
#define Elf_Word          Elf32_Word
#define SYMCRYPT_FORCE_READ_ADDR SYMCRYPT_FORCE_READ32
#elif UINTPTR_MAX == 0xFFFFFFFFFFFFFFFFu
#define Elf_Shdr          Elf64_Shdr
#define Elf_Phdr          Elf64_Phdr
#define Elf_Sym           Elf64_Sym
#define Elf_Dyn           Elf64_Dyn
#define Elf_Ehdr          Elf64_Ehdr
#define Elf_Addr          Elf64_Addr
#define Elf_Off           Elf64_Off
#define Elf_Rel           Elf64_Rel
#define Elf_Rela          Elf64_Rela
#define ELF_R_TYPE(X)     ELF64_R_TYPE(X)
#define ELF_R_SYM(X)      ELF64_R_SYM(X)
#define Elf_Word          Elf64_Word
#define SYMCRYPT_FORCE_READ_ADDR SYMCRYPT_FORCE_READ64
#else
#error Unknown CPU pointer size
#endif

#if SYMCRYPT_DEBUG_INTEGRITY
#include <stdio.h>

VOID
DbgDumpHex(PCBYTE pbData, SIZE_T cbData)
{
    ULONG i,count;
    CHAR digits[]="0123456789abcdef";
    CHAR pbLine[256];
    ULONG cbLine, cbHeader = 0;
    ULONG_PTR address;

    if(pbData == NULL && cbData != 0)
    {
        // strcat_s(pbLine, RTL_NUMBER_OF(pbLine), "<null> buffer!!!\n");
        fprintf(stderr, "<null> buffer!!!\n");
        return;
    }

    for(; cbData ; cbData -= count, pbData += count)
    {
        count = (cbData > 16) ? 16:cbData;

        cbLine = cbHeader;

        address = (ULONG_PTR)pbData;

#if UINTPTR_MAX == 0xFFFFFFFFFFFFFFFFu
        // 64 bit addresses.
        pbLine[cbLine++] = digits[(address >> 0x3c) & 0x0f];
        pbLine[cbLine++] = digits[(address >> 0x38) & 0x0f];
        pbLine[cbLine++] = digits[(address >> 0x34) & 0x0f];
        pbLine[cbLine++] = digits[(address >> 0x30) & 0x0f];
        pbLine[cbLine++] = digits[(address >> 0x2c) & 0x0f];
        pbLine[cbLine++] = digits[(address >> 0x28) & 0x0f];
        pbLine[cbLine++] = digits[(address >> 0x24) & 0x0f];
        pbLine[cbLine++] = digits[(address >> 0x20) & 0x0f];
#endif
        pbLine[cbLine++] = digits[(address >> 0x1c) & 0x0f];
        pbLine[cbLine++] = digits[(address >> 0x18) & 0x0f];
        pbLine[cbLine++] = digits[(address >> 0x14) & 0x0f];
        pbLine[cbLine++] = digits[(address >> 0x10) & 0x0f];
        pbLine[cbLine++] = digits[(address >> 0x0c) & 0x0f];
        pbLine[cbLine++] = digits[(address >> 0x08) & 0x0f];
        pbLine[cbLine++] = digits[(address >> 0x04) & 0x0f];
        pbLine[cbLine++] = digits[(address        ) & 0x0f];
        pbLine[cbLine++] = ' ';
        pbLine[cbLine++] = ' ';

        for(i = 0; i < count; i++)
        {
            pbLine[cbLine++] = digits[pbData[i]>>4];
            pbLine[cbLine++] = digits[pbData[i]&0x0f];
            if(i == 7)
            {
                pbLine[cbLine++] = ':';
            }
            else
            {
                pbLine[cbLine++] = ' ';
            }
        }

        for(; i < 16; i++)
        {
            pbLine[cbLine++] = ' ';
            pbLine[cbLine++] = ' ';
            pbLine[cbLine++] = ' ';
        }

        pbLine[cbLine++] = ' ';

        for(i = 0; i < count; i++)
        {
            if(pbData[i] < 32 || pbData[i] > 126)
            {
                pbLine[cbLine++] = '.';
            }
            else
            {
                pbLine[cbLine++] = pbData[i];
            }
        }

        pbLine[cbLine++] = 0;

        fprintf(stderr, "%s\n", pbLine);
    }
}

VOID
SYMCRYPT_CALL
SymCryptHmacSha256AppendDbg(
    _In_                    CHAR*                       pszLabel,
    _Inout_                 PSYMCRYPT_HMAC_SHA256_STATE pState,
    _In_reads_( cbData )    PCBYTE                      pbData,
                            SIZE_T                      cbData )
{
    fprintf(stderr, "\nHMAC append: %s size %lx\n", pszLabel, cbData);
    DbgDumpHex(pbData, (ULONG)cbData);
    SymCryptHmacSha256Append(pState, pbData, cbData);
}

#endif

// These placeholder values must match the values in process_fips_module.py
#if UINTPTR_MAX == 0xFFFFFFFF
#define PLACEHOLDER_VALUE 0x8BADF00D
#elif UINTPTR_MAX == 0xFFFFFFFFFFFFFFFFu
#define PLACEHOLDER_VALUE 0x4BADF00D8BADF00D
#else
#error Unknown CPU pointer size
#endif

#define PLACEHOLDER_ARRAY \
{\
    0x5B, 0x75, 0xBB, 0xE4, 0x9E, 0x18, 0x03, 0x55,\
    0x08, 0x4E, 0x3F, 0xE7, 0x60, 0x7E, 0x4F, 0x08,\
    0xAA, 0x77, 0x0F, 0x0B, 0xAB, 0xC6, 0x58, 0x5A,\
    0xA9, 0x9F, 0x83, 0x4B, 0xD0, 0x6E, 0x67, 0x05\
}

// The following variables use placeholder values which will be modified after compile time by our
// helper script. They need to be statically initialized to non-zero values so they are put in the
// .data segment rather than the .bss segment, as the latter's representation in the module on disk
// is not necessarily the same size as the values in memory at runtime.
//
// Because these values are modified after compile time, the scalar values must be read using
// SYMCRYPT_FORCE_READ64 or the compiler may inline the placeholder values, leading to incorrect
// results at runtime.

// Relative virtual address of the HMAC key. Used to calculate where the module starts in memory
// at runtime.
const Elf_Addr SymCryptVolatileFipsHmacKeyRva = (Elf_Addr) PLACEHOLDER_VALUE;

// Offset to the end of the FIPS module. Bytes after this offset are not considered part of our
// FIPS module and are not included in the HMAC digest.
const Elf_Off SymCryptVolatileFipsBoundaryOffset = PLACEHOLDER_VALUE;

// Key used for HMAC.
const unsigned char SymCryptVolatileFipsHmacKey[32] = PLACEHOLDER_ARRAY;

// HMAC digest calculated by our post-processing script after calculation. The HMAC digest
// we calculate at runtime is compared to this digest.
unsigned char SymCryptVolatileFipsHmacDigest[SYMCRYPT_HMAC_SHA256_RESULT_SIZE] = PLACEHOLDER_ARRAY;

typedef struct
{
    Elf_Rela* rela;
    Elf_Rel* rel;
    size_t relaEntryCount;
    size_t relEntryCount;
    union {
        Elf_Rela* rela;
        Elf_Rel* rel;
        Elf_Addr addr;
    } plt;
    size_t pltRelaEntryCount;
    Elf_Addr pltRelAddendType;
} Elf_Rela_Info;


VOID SymCryptModuleUndoRelocation(
    _In_ const Elf_Addr module_base,
    _Inout_ Elf_Addr* const target,
    _In_ const Elf_Word relType )
{
    Elf_Addr replacement = 0;

    switch( relType )
    {
        case R_X86_64_RELATIVE:
        case R_AARCH64_RELATIVE:
        case R_ARM_RELATIVE:
            replacement = *target - (Elf_Off) module_base;
            break;
        case R_X86_64_64:
        case R_X86_64_GLOB_DAT:
        case R_X86_64_JUMP_SLOT:
        case R_AARCH64_GLOB_DAT:
        case R_AARCH64_JUMP_SLOT:
        case R_ARM_GLOB_DAT:
        case R_ARM_JUMP_SLOT:
            // R_X86_64_64 and R_*_GLOB_DAT relocations all have initial
            // values of zero. R_*_JUMP_SLOT relocations have initial
            // values that point into the PLT, but we set these to zero in our post-processing
            // script before HMACing the module. These relocation targets are excluded from our
            // FIPS module boundary because they're used for external function calls, which we
            // cannot verify the addresses of at runtime, and which are by definition outside of
            // the FIPS module anyway.
            replacement = 0;
            break;
        default:
            // We cannot handle other relocation types
            SYMCRYPT_FIPS_ASSERT( FALSE );
            break;
    }

    *target = replacement;
}

VOID SymCryptModuleFindRelocationInfo(
    _In_ const Elf_Dyn* const dynStart,
    _Out_ Elf_Rela_Info* relaInfo)
{
    relaInfo->rela = NULL;
    relaInfo->relaEntryCount = 0;
    relaInfo->relEntryCount = 0;
    relaInfo->plt.addr = 0;
    relaInfo->pltRelaEntryCount = 0;

    size_t relaTotalSize = 0;
    size_t relTotalSize = 0;
    size_t relaEntrySize = 0;
    size_t relEntrySize = 0;
    size_t pltTotalSize = 0;

    for( const Elf_Dyn* dyn = dynStart; dyn->d_tag != DT_NULL; ++dyn )
    {
        switch( dyn->d_tag )
        {
            case DT_RELA:
                relaInfo->rela = ( Elf_Rela* ) dyn->d_un.d_ptr;
                break;

            case DT_REL:
                relaInfo->rel = ( Elf_Rel* ) dyn->d_un.d_ptr;
                break;

            case DT_RELASZ:
                relaTotalSize = dyn->d_un.d_val;
                break;

            case DT_RELSZ:
                relTotalSize = dyn->d_un.d_val;
                break;

            case DT_RELAENT:
                relaEntrySize = dyn->d_un.d_val;
                break;

            case DT_RELENT:
                relEntrySize = dyn->d_un.d_val;
                break;

            case DT_JMPREL:
                relaInfo->plt.addr = ( Elf_Addr ) dyn->d_un.d_ptr;
                break;

            case DT_PLTRELSZ:
                pltTotalSize = dyn->d_un.d_val;
                break;

            case DT_PLTREL:
                SYMCRYPT_FIPS_ASSERT( dyn->d_un.d_val == DT_RELA || dyn->d_un.d_val == DT_REL );
                relaInfo->pltRelAddendType = dyn->d_un.d_val;
                break;

            default:
                break;
        }
    }

    // Need to have at least one type of relocations.
    SYMCRYPT_FIPS_ASSERT( relaInfo->rela != NULL || relaInfo->rel != NULL );

    if ( relaInfo->rela != NULL)
    {
        SYMCRYPT_FIPS_ASSERT( relaEntrySize == sizeof( Elf_Rela ) );
        SYMCRYPT_FIPS_ASSERT( relaTotalSize != 0 && relaTotalSize % relaEntrySize == 0 );
        relaInfo->relaEntryCount = relaTotalSize / relaEntrySize;
    }
    if ( relaInfo->rel != NULL )
    {
        SYMCRYPT_FIPS_ASSERT( relaInfo->rel != NULL );
        SYMCRYPT_FIPS_ASSERT( relEntrySize == sizeof( Elf_Rel ) );
        SYMCRYPT_FIPS_ASSERT( relTotalSize != 0 && relTotalSize % relEntrySize == 0 );
        relaInfo->relEntryCount = relTotalSize / relEntrySize;
    }

    if( relaInfo->plt.addr != 0)
    {
        SYMCRYPT_FIPS_ASSERT( pltTotalSize != 0 );
        // PLT relocations are either all rel or all rela.
        if ( relaInfo->pltRelAddendType == DT_RELA )
        {
            SYMCRYPT_FIPS_ASSERT( pltTotalSize % sizeof( Elf_Rela ) == 0 );
            relaInfo->pltRelaEntryCount = pltTotalSize / sizeof( Elf_Rela );
        }
        else
        {
            SYMCRYPT_FIPS_ASSERT( pltTotalSize % sizeof( Elf_Rel ) == 0 );
            relaInfo->pltRelaEntryCount = pltTotalSize / sizeof( Elf_Rel );
        }
    }
}

size_t SymCryptModuleProcessSegmentWithRelocations(
    _In_ const Elf_Addr module_base,
    _In_ const Elf_Phdr* const programHeader,
    _In_ const Elf_Dyn* const dynStart,
    _In_ const Elf_Rela_Info* const relaInfo,
    _Inout_ SYMCRYPT_HMAC_SHA256_STATE* hmacState )
{
    // The segment that contains relocations consists of the following sections, in this order:
    // .data.rel.ro .dynamic .got .data .bss
    //
    // .data.rel.ro, .dynamic and .got contain relocations, but are not modified by the code itself
    // once the dynamic linker has performed the relocations, so these sections are included in our
    // HMAC calculation.
    //
    // .data includes non-constant global variables which can change at runtime. We cannot reverse
    // these values without tightly coupling this integrity verification implementation to internal
    // implementation details of SymCrypt, so it is not included in our HMAC. The .bss section in
    // the module on disk is usually a different size than at runtime, so we cannot include it in
    // our HMAC either.
    //
    // In arm (32 bit) relocations also exist in .text section so we'll do SymCryptModuleProcessSegmentWithRelocations
    // on every section.
    // 
    // FipsBoundaryOffset marks the start of the .data section, so we read from the start of the
    // segment up to that offset.
    size_t hashableSectionSize = programHeader->p_filesz;
    Elf_Addr segmentStart = module_base + programHeader->p_vaddr;

    // If the data section is in this segment then exclude that from being hashed.
    if( SYMCRYPT_FORCE_READ_ADDR( &SymCryptVolatileFipsBoundaryOffset ) <= programHeader->p_offset + programHeader->p_filesz )
    {
        hashableSectionSize = SYMCRYPT_FORCE_READ_ADDR( &SymCryptVolatileFipsBoundaryOffset ) - programHeader->p_offset;
    }

    BYTE* segmentCopy = SymCryptCallbackAlloc( hashableSectionSize );
    SYMCRYPT_FIPS_ASSERT( segmentCopy != NULL );

    memcpy( segmentCopy, (const unsigned char*) segmentStart, hashableSectionSize );

    // Some of the entries in the .dynamic section get relocated, but those relocations are not
    // included in the list of relocations given in the .rela.dyn section. Thus, we must process
    // these relocations separately. We find the .dynamic section in the copied buffer based on
    // its offset from the start of the section, which is calculated by subtracting the address
    // of the start of the segment from the address of the .dynamic section in the segment.
    Elf_Off dynOffsetInBuffer = (Elf_Addr) dynStart - (Elf_Addr) segmentStart;
    Elf_Dyn* dynStartInBuffer = (Elf_Dyn*) (segmentCopy + dynOffsetInBuffer);

    // If this segment contains the dynamic section then we need to process the relocations in it.
    if ((Elf_Addr)dynStart > segmentStart && (Elf_Addr)dynStart < segmentStart + hashableSectionSize)
    {
        for( Elf_Dyn* dyn = dynStartInBuffer; dyn->d_tag != DT_NULL; ++dyn )
        {
            // The following types of .dynamic entries have the module's base address added to
            // their initial value
            if( dyn->d_tag == DT_HASH ||
                dyn->d_tag == DT_STRTAB ||
                dyn->d_tag == DT_SYMTAB ||
                dyn->d_tag == DT_RELA ||
                dyn->d_tag == DT_GNU_HASH ||
                dyn->d_tag == DT_VERSYM ||
                dyn->d_tag == DT_PLTGOT ||
                dyn->d_tag == DT_JMPREL ||
                dyn->d_tag == DT_REL)
            {
                dyn->d_un.d_val -= module_base;
            }
        }
    }

    // Now we can process the normal relocations listed in the relocation table
    for( size_t i = 0; i < relaInfo->relaEntryCount; ++i )
    {
        const Elf_Rela* rela = relaInfo->rela + i;

        // Find the relocation within the section. Note that for a shared object module,
        // rela->r_offset is actually a virtual address. Relocations can occur within the .data
        // section, which is outside our FIPS boundary, so any such relocations can be ignored.
        Elf_Off offsetInBuffer = (Elf_Off) rela->r_offset - (Elf_Off) programHeader->p_vaddr;
        if( offsetInBuffer > hashableSectionSize )
        {
            continue;
        }

        Elf_Addr* target = (Elf_Addr*) ( segmentCopy + offsetInBuffer);

        SymCryptModuleUndoRelocation( module_base, target, ELF_R_TYPE( rela->r_info ) );
    }

    for( size_t i = 0; i < relaInfo->relEntryCount; ++i )
    {
        const Elf_Rel* rel = relaInfo->rel + i;

        // Find the relocation within the section. Note that for a shared object module,
        // rela->r_offset is actually a virtual address. Relocations can occur within the .data
        // section, which is outside our FIPS boundary, so any such relocations can be ignored.
        Elf_Off offsetInBuffer = (Elf_Off) rel->r_offset - (Elf_Off) programHeader->p_vaddr;
        if( offsetInBuffer > hashableSectionSize )
        {
            continue;
        }

        Elf_Addr* target = (Elf_Addr*) ( segmentCopy + offsetInBuffer);

        SymCryptModuleUndoRelocation( module_base, target, ELF_R_TYPE( rel->r_info ) );
    }

    // Process the GOT entries from the .rela.plt or .rel.plt section. Same as process above, just
    // with a different table.
    for( size_t i = 0; i < relaInfo->pltRelaEntryCount; ++i)
    {
        Elf_Word type = 0;
        Elf_Off offsetInBuffer = 0;

        if (relaInfo->pltRelAddendType == DT_RELA)
        {
            const Elf_Rela* rela = relaInfo->plt.rela + i;
            type = ELF_R_TYPE( rela->r_info );
            offsetInBuffer = (Elf_Off) rela->r_offset - (Elf_Off) programHeader->p_vaddr;
        }
        else
        {
            const Elf_Rel* rel = relaInfo->plt.rel + i;
            type = ELF_R_TYPE( rel->r_info );
            offsetInBuffer = (Elf_Off) rel->r_offset - (Elf_Off) programHeader->p_vaddr;
        }

        if( offsetInBuffer > hashableSectionSize )
        {
            continue;
        }

        Elf_Addr* target = (Elf_Addr*) ( segmentCopy + offsetInBuffer);

        SymCryptModuleUndoRelocation( module_base, target, type );
    }

#if SYMCRYPT_DEBUG_INTEGRITY
    SymCryptHmacSha256AppendDbg( "Append after relocation adjust", hmacState, segmentCopy, hashableSectionSize );
#else 
    SymCryptHmacSha256Append( hmacState, segmentCopy, hashableSectionSize );
#endif

    SymCryptWipe( segmentCopy, hashableSectionSize );
    SymCryptCallbackFree( segmentCopy );

    return hashableSectionSize;
}

VOID SymCryptModuleDoHmac(
    _In_ const Elf_Addr module_base,
    _In_ const Elf_Dyn* const dynStart,
    _In_ const Elf_Rela_Info* const relaInfo )
{
    SYMCRYPT_ERROR scError = SYMCRYPT_NO_ERROR;
    SYMCRYPT_HMAC_SHA256_EXPANDED_KEY hmacKey;
    SYMCRYPT_HMAC_SHA256_STATE hmacState;
    BYTE actualDigest[SYMCRYPT_HMAC_SHA256_RESULT_SIZE];

    scError = SymCryptHmacSha256ExpandKey( &hmacKey, SymCryptVolatileFipsHmacKey,
        sizeof(SymCryptVolatileFipsHmacKey) );
    SYMCRYPT_FIPS_ASSERT( scError == SYMCRYPT_NO_ERROR );

    SymCryptHmacSha256Init( &hmacState, &hmacKey );

    const Elf_Ehdr* header = (Elf_Ehdr*) module_base;
    const Elf_Phdr* programHeaderStart = (Elf_Phdr*) ( module_base + header->e_phoff );

    for( int i = 0; i < header->e_phnum; ++i)
    {
        const Elf_Phdr* programHeader = programHeaderStart + i;
        if (programHeader->p_type != PT_LOAD)
        {
            continue;
        }

        // Sometimes the virtual address of a segment is greater than its offset into the module
        // file on disk. This means extra NULL bytes will be inserted into the module's memory
        // space at runtime. Those bytes are not part of our FIPS boundary, so we skip over them
        // and always start reading from the segment's virtual address
        Elf_Addr segmentStart = module_base + (Elf_Off) programHeader->p_vaddr;

        SymCryptModuleProcessSegmentWithRelocations( module_base, programHeader, dynStart,
                relaInfo, &hmacState );
    }

    SymCryptHmacSha256Result( &hmacState, actualDigest );

    // Verify that the HMAC result matches our expected digest
    SYMCRYPT_FIPS_ASSERT(
        memcmp( actualDigest, SymCryptVolatileFipsHmacDigest, SYMCRYPT_HMAC_SHA256_RESULT_SIZE ) == 0 );

    // FIPS 140-3 TE05.08.02 requires that "any temporary values generated during the integrity
    // test are zeroised upon completion of the integrity test"
    SymCryptWipeKnownSize( actualDigest, SYMCRYPT_HMAC_SHA256_RESULT_SIZE );
    SymCryptWipeKnownSize( &hmacState, sizeof(hmacState) );
    SymCryptWipeKnownSize( &hmacKey, sizeof(hmacKey) );
}

VOID SymCryptModuleVerifyIntegrity(void)
{
    // Verify that our placeholder values were modified after compile time. The build script
    // should have replaced the placeholder values with their expected values
    SYMCRYPT_FIPS_ASSERT( SYMCRYPT_FORCE_READ_ADDR( &SymCryptVolatileFipsHmacKeyRva ) != PLACEHOLDER_VALUE );
    SYMCRYPT_FIPS_ASSERT( SYMCRYPT_FORCE_READ_ADDR( &SymCryptVolatileFipsBoundaryOffset ) != PLACEHOLDER_VALUE );

    const Elf_Addr module_base = (Elf_Addr) SymCryptVolatileFipsHmacKey -
        SYMCRYPT_FORCE_READ_ADDR( &SymCryptVolatileFipsHmacKeyRva );

    const Elf_Ehdr* header = (Elf_Ehdr*) module_base;
    SYMCRYPT_FIPS_ASSERT( memcmp(header->e_ident.ident.magic, ElfMagic, sizeof(ElfMagic)) == 0 );
    SYMCRYPT_FIPS_ASSERT( header->e_type == ET_DYN );
    SYMCRYPT_FIPS_ASSERT( header->e_machine == EM_X86_64 || header->e_machine == EM_AARCH64 || header->e_machine == EM_ARM );
    SYMCRYPT_FIPS_ASSERT( header->e_version == EV_CURRENT );
    SYMCRYPT_FIPS_ASSERT( header->e_ehsize == sizeof(Elf_Ehdr) );
    SYMCRYPT_FIPS_ASSERT( header->e_phentsize == sizeof(Elf_Phdr) );

    const Elf_Phdr* programHeaderStart = (Elf_Phdr*) ( module_base + header->e_phoff );

    Elf_Rela_Info relaInfo = {};

    Elf_Dyn* dynStart = NULL;

    for( unsigned int i = 0; i < header->e_phnum; ++i )
    {
        const Elf_Phdr* programHeader = programHeaderStart + i;
        if( programHeader->p_type == PT_DYNAMIC )
        {
            dynStart = (Elf_Dyn*) (module_base + (Elf_Off) programHeader->p_vaddr);

            SymCryptModuleFindRelocationInfo( dynStart, &relaInfo );

            // We only expect one PT_DYNAMIC segment
            break;
        }
    }

    SymCryptModuleDoHmac( module_base, dynStart, &relaInfo );
}
