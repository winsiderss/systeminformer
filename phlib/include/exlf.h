#ifndef _PH_EXLF_H
#define _PH_EXLF_H

/*
 * This file contains the required types for ELF binaries.
 *
 * References:
 * http://man7.org/linux/man-pages/man5/elf.5.html
 * http://www.skyfree.org/linux/references/ELF_Format.pdf
 * https://github.com/torvalds/linux/blob/master/include/uapi/linux/elf.h
 * https://chromium.googlesource.com/chromiumos/chromite/+/HEAD/lib/parseelf.py
 */

#define EI_NIDENT 16

// e_ident[] indexes
#define EI_MAG0     0
#define EI_MAG1     1
#define EI_MAG2     2
#define EI_MAG3     3
#define EI_CLASS    4
#define EI_DATA     5
#define EI_VERSION  6
#define EI_OSABI    7
#define EI_PAD      8

// EI_MAG
#define ELFMAG0 0x7f
#define ELFMAG1 'E'
#define ELFMAG2 'L'
#define ELFMAG3 'F'
// EI_CLASS
#define ELFCLASSNONE 0
#define ELFCLASS32 1
#define ELFCLASS64 2

// EI_DATA
#define ELFDATANONE 0
#define ELFDATA2LSB 1
#define ELFDATA2MSB 2

// EI_VERSION and e_version
#define EV_NONE 0
#define EV_CURRENT 1

#define ELFOSABI_NONE 0
#define ELFOSABI_LINUX 3

// e_type
#define ET_NONE 0
#define ET_REL 1
#define ET_EXEC 2
#define ET_DYN 3
#define ET_CORE 4
#define ET_LOPROC 0xff00
#define ET_HIPROC 0xffff

// e_machine
#define EM_386 3
#define EM_X86_64 62

/* segment types */
#define PT_NULL    0
#define PT_LOAD    1
#define PT_DYNAMIC 2
#define PT_INTERP  3
#define PT_NOTE    4
#define PT_SHLIB   5
#define PT_PHDR    6
#define PT_TLS     7               /* Thread local storage segment */
#define PT_LOOS    0x60000000      /* OS-specific */
#define PT_HIOS    0x6fffffff      /* OS-specific */
#define PT_LOPROC  0x70000000
#define PT_HIPROC  0x7fffffff
#define PT_GNU_EH_FRAME     0x6474e550
#define PT_GNU_STACK    (PT_LOOS + 0x474e551)

/* permissions on sections in the program header, p_flags. */
#define PF_NONE 0x0
#define PF_X 0x1
#define PF_W 0x2
#define PF_R 0x4

// sh_type
#define SHT_NULL 0     // Section header table entry (unused).
#define SHT_PROGBITS 1 // Program data.
#define SHT_SYMTAB 2   // Link editing symbol table.
#define SHT_STRTAB 3   // A string table.
#define SHT_RELA 4     // Relocation entries with addends.
#define SHT_HASH 5     // A symbol hash table.
#define SHT_DYNAMIC 6  // Information for dynamic linking.
#define SHT_NOTE 7     // Information that marks file.
#define SHT_NOBITS 8   // Section occupies no space in file.
#define SHT_REL 9      // Relocation entries, no addends.
#define SHT_SHLIB 10   // Reserved, unspecified semantics.
#define SHT_DYNSYM 11  // Dynamic linking symbol table.
//#define SHT_NUM 12
#define SHT_INIT_ARRAY 14    // Array of constructors.
#define SHT_FINI_ARRAY 15    // Array of destructors.
#define SHT_PREINIT_ARRAY 16 // Array of pre-constructors.
#define SHT_GROUP 17         // Section group.
#define SHT_SYMTAB_SHNDX 18  // Extended section indeces.
#define SHT_NUM 19           // Number of defined types. (dmex: Some tools define this as 19 and others as 12???)
#define SHT_LOOS 0x60000000 // First of OS specific semantics.
#define SHT_HIOS 0x6fffffff // Last of OS specific semantics.
#define SHT_GNU_INCREMENTAL_INPUTS 0x6fff4700 // incremental build data.
#define SHT_GNU_ATTRIBUTES 0x6ffffff5 // Object attributes.
#define SHT_GNU_HASH 0x6ffffff6 // GNU style symbol hash table.
#define SHT_GNU_LIBLIST 0x6ffffff7 // List of prelink dependencies.
#define SHT_LOPROC 0x70000000
#define SHT_HIPROC 0x7fffffff
#define SHT_LOUSER 0x80000000
#define SHT_HIUSER 0xffffffff
#define SHT_SUNW_verdef 0x6ffffffd  // Versions defined by file.
#define SHT_SUNW_verneed 0x6ffffffe // Versions needed by file.
#define SHT_SUNW_versym 0x6fffffff  // Symbol versions.

// sh_flags
#define SHF_WRITE 0x1   /* Section contains writable data. */
#define SHF_ALLOC 0x2   /* Section occupies memory. */
#define SHF_EXECINSTR 0x4   /* Section contains instructions. */
#define SHF_MERGE 0x10  /* Section may be merged. */
#define SHF_STRINGS 0x20    /* Section contains strings. */
#define SHF_INFO_LINK 0x40  /* sh_info holds section index. */
#define SHF_LINK_ORDER 0x80 /* Special ordering requirements. */
#define SHF_OS_NONCONFORMING 0x100  /* OS-specific processing required. */
#define SHF_GROUP 0x200 /* Member of section group. */
#define SHF_TLS 0x400   /* Section contains TLS data. */
#define SHF_MASKOS  0x0ff00000  /* OS-specific semantics. */
#define SHF_MASKPROC    0xf0000000  /* Processor-specific semantics. */

// special section indexes.
#define SHN_UNDEF 0 // An undefined symbol.
#define SHN_LORESERVE 0xff00
#define SHN_LOPROC 0xff00
#define SHN_HIPROC 0xff1f
#define SHN_LIVEPATCH 0xff20
#define SHN_ABS 0xfff1
#define SHN_COMMON 0xfff2
#define SHN_HIRESERVE 0xffff

// dynamic section
#define DT_NULL     0
#define DT_NEEDED   1
#define DT_PLTRELSZ 2
#define DT_PLTGOT   3
#define DT_HASH     4
#define DT_STRTAB   5
#define DT_SYMTAB   6
#define DT_RELA     7
#define DT_RELASZ   8
#define DT_RELAENT  9
#define DT_STRSZ    10
#define DT_SYMENT   11
#define DT_INIT     12
#define DT_FINI     13
#define DT_SONAME   14
#define DT_RPATH    15
#define DT_SYMBOLIC 16
#define DT_REL      17
#define DT_RELSZ    18
#define DT_RELENT   19
#define DT_PLTREL   20
#define DT_DEBUG    21
#define DT_TEXTREL  22
#define DT_JMPREL   23
#define DT_INIT_ARRAY 25
#define DT_FINI_ARRAY 26
#define DT_INIT_ARRAYSZ 27
#define DT_FINI_ARRAYSZ 28
#define DT_RUNPATH 29
#define DT_FLAGS 30
#define DT_PREINIT_ARRAY 32 // DT_ENCODING
#define DT_PREINIT_ARRAYSZ 33
#define OLD_DT_LOOS 0x60000000
#define DT_LOOS 0x6000000d
#define DT_HIOS 0x6ffff000
#define DT_VALRNGLO 0x6ffffd00
#define DT_VALRNGHI 0x6ffffdff
#define DT_ADDRRNGLO 0x6ffffe00
#define DT_ADDRRNGHI 0x6ffffeff
#define DT_GNU_HASH 0x6ffffef5
#define DT_VERSYM 0x6ffffff0
#define DT_RELACOUNT 0x6ffffff9
#define DT_RELCOUNT 0x6ffffffa
#define DT_FLAGS_1 0x6ffffffb
#define DT_VERDEF 0x6ffffffc
#define DT_VERDEFNUM 0x6ffffffd
#define DT_VERNEED 0x6ffffffe
#define DT_VERNEEDNUM 0x6fffffff
#define OLD_DT_HIOS 0x6fffffff
#define DT_LOPROC 0x70000000
#define DT_HIPROC 0x7fffffff

// symbol table section
#define STB_LOCAL 0 /* Local symbol */
#define STB_GLOBAL 1 /* Global symbol */
#define STB_WEAK 2 /* Weak symbol */
#define STB_NUM 3 /* Number of defined types.  */
#define STB_LOOS 10 /* Start of OS-specific */
#define STB_GNU_UNIQUE 10 /* Unique symbol.  */
#define STB_HIOS 12 /* End of OS-specific */
#define STB_LOPROC 13 /* Start of processor-specific */
#define STB_HIPROC 15 /* End of processor-specific */

#define STT_NOTYPE  0
#define STT_OBJECT  1
#define STT_FUNC    2
#define STT_SECTION 3
#define STT_FILE    4
#define STT_COMMON  5
#define STT_TLS     6
#define STT_GNU_IFUNC 10
#define STT_LOOS      10
#define STT_HIOS      12
#define STT_LOPROC    13
#define STT_HIPROC    15

#define STV_DEFAULT 0 /* Default symbol visibility rules */
#define STV_INTERNAL 1 /* Processor specific hidden class */
#define STV_HIDDEN 2 /* Sym unavailable in other modules */
#define STV_PROTECTED 3 /* Not preemptible, not exported */

#define ELF_ST_BIND(x) ((x) >> 4)
#define ELF_ST_TYPE(x) ((x) & 0xF)
#define ELF_ST_VISIBILITY(x) ((x) & 0x03)

#define ELF32_ST_BIND(x) ELF_ST_BIND(x)
#define ELF32_ST_TYPE(x) ELF_ST_TYPE(x)
#define ELF32_ST_VIS(a) ELF_ST_VISIBILITY(a)
#define ELF64_ST_BIND(x) ELF_ST_BIND(x)
#define ELF64_ST_TYPE(x) ELF_ST_TYPE(x)
#define ELF64_ST_VIS(a) ELF_ST_VISIBILITY(a)

// Non-standard ELF definitions (dmex)

#define IMAGE_ELF_SIGNATURE 0x457f // "\x7fELF"
#define ELFMAG ((BYTE[4]){ELFMAG0,ELFMAG1,ELFMAG2,ELFMAG3})

typedef struct _ELF_IMAGE_HEADER
{
    union
    {
        unsigned char e_ident[EI_NIDENT];
        struct
        {
            unsigned char MagicNumber[4];
            unsigned char Class;
            unsigned char Data;
            unsigned char Version;
            unsigned char Abi;
            unsigned char AbiVersion;
            unsigned char Unused[7];
        };
    };
    unsigned short e_type;
    unsigned short e_machine;
    unsigned int e_version;
    //union {
    //    PELF_IMAGE_HEADER32 Headers32;
    //    PELF_IMAGE_HEADER64 Headers64;
    //};
} ELF_IMAGE_HEADER, *PELF_IMAGE_HEADER;

typedef struct _ELF_IMAGE_HEADER32
{
    // ELF_IMAGE_HEADER Header;
    unsigned int e_entry; // Entry point virtual address.
    unsigned int e_phoff; // Program header table file offset.
    unsigned int e_shoff; // Section header table file offset.
    unsigned int e_flags; // Processor-specific flags.
    unsigned short e_ehsize; // ELF header size in bytes.
    unsigned short e_phentsize; // Program header table entry size.
    unsigned short e_phnum; // Program header table entry count.
    unsigned short e_shentsize; // Section header table entry size.
    unsigned short e_shnum; // Section header table entry count.
    unsigned short e_shstrndx; // Section header string table index.
} ELF_IMAGE_HEADER32, *PELF_IMAGE_HEADER32;

typedef struct _ELF_IMAGE_HEADER64
{
    // ELF_IMAGE_HEADER Header;
    unsigned long long e_entry; // Entry point virtual address.
    unsigned long long e_phoff; // Program header table file offset.
    unsigned long long e_shoff; // Section header table file offset.
    unsigned int e_flags; // Processor-specific flags.
    unsigned short e_ehsize; // ELF header size in bytes.
    unsigned short e_phentsize; // Program header table entry size.
    unsigned short e_phnum; // Program header table entry count.
    unsigned short e_shentsize; // Section header table entry size.
    unsigned short e_shnum; // Section header table entry count.
    unsigned short e_shstrndx; // Section header string table index.
} ELF_IMAGE_HEADER64, *PELF_IMAGE_HEADER64;

typedef struct _ELF32_IMAGE_SEGMENT_HEADER
{
    unsigned int p_type; // Segment type.
    unsigned int p_offset; // Segment file offset.
    unsigned int p_vaddr; // Segment virtual address.
    unsigned int p_paddr; // Segment physical address.
    unsigned int p_filesz; // Segment size in file.
    unsigned int p_memsz; // Segment size in memory.
    unsigned int p_flags; // Segment flags.
    unsigned int p_align; // Segment alignment.
} ELF32_IMAGE_SEGMENT_HEADER;

typedef struct _ELF64_IMAGE_SEGMENT_HEADER
{
    unsigned int p_type; // Segment type.
    unsigned int p_flags; // Segment flags.
    unsigned long long p_offset; // Segment file offset.
    unsigned long long p_vaddr; // Segment virtual address.
    unsigned long long p_paddr; // Segment physical address.
    unsigned long long p_filesz; // Segment size in file.
    unsigned long long p_memsz; // Segment size in memory.
    unsigned long long p_align; // Segment alignment.
} ELF64_IMAGE_SEGMENT_HEADER, *PELF64_IMAGE_SEGMENT_HEADER;

#define IMAGE_FIRST_ELF64_SEGMENT(MappedWslImage) \
    ((PELF64_IMAGE_SEGMENT_HEADER)PTR_ADD_OFFSET(MappedWslImage->Header, MappedWslImage->Headers64->e_phoff))

#define IMAGE_ELF64_SEGMENT_BY_INDEX(SegmentHeader, Index) \
    ((PELF64_IMAGE_SEGMENT_HEADER)PTR_ADD_OFFSET(SegmentHeader, sizeof(ELF64_IMAGE_SEGMENT_HEADER) * Index))
    //((PELF64_IMAGE_SEGMENT_HEADER)&SegmentHeaderTable[Index])

typedef struct _ELF32_IMAGE_SECTION_HEADER
{
    unsigned int sh_name;
    unsigned int sh_type;
    unsigned int sh_flags;
    unsigned int sh_addr;
    unsigned int sh_offset;
    unsigned int sh_size;
    unsigned int sh_link;
    unsigned int sh_info;
    unsigned int sh_addralign;
    unsigned int sh_entsize;
} ELF32_IMAGE_SECTION_HEADER;

typedef struct _ELF64_IMAGE_SECTION_HEADER
{
    unsigned int sh_name;            /* Section name, index in string tbl */
    unsigned int sh_type;            /* Type of section */
    unsigned long long sh_flags;     /* Miscellaneous section attributes */
    unsigned long long sh_addr;      /* Section virtual addr at execution */
    unsigned long long sh_offset;    /* Section file offset */
    unsigned long long sh_size;      /* Size of section in bytes */
    unsigned int sh_link;            /* Index of another section */
    unsigned int sh_info;            /* Additional section information */
    unsigned long long sh_addralign; /* Section alignment */
    unsigned long long sh_entsize;   /* Entry size if section holds table */
} ELF64_IMAGE_SECTION_HEADER, *PELF64_IMAGE_SECTION_HEADER;

#define IMAGE_FIRST_ELF64_SECTION(MappedWslImage) \
    ((PELF64_IMAGE_SECTION_HEADER)PTR_ADD_OFFSET(MappedWslImage->Header, MappedWslImage->Headers64->e_shoff))

#define IMAGE_ELF64_SECTION_BY_INDEX(SectionHeader, Index) \
    ((PELF64_IMAGE_SECTION_HEADER)PTR_ADD_OFFSET(SectionHeader, sizeof(ELF64_IMAGE_SECTION_HEADER) * Index))
    // ((PELF64_IMAGE_SECTION_HEADER)&SectionHeaderTable[Index])

// ELF dynamic entries

typedef struct _ELF32_IMAGE_DYNAMIC_ENTRY // Elf32_Dyn
{
    int d_tag;
    unsigned int d_val;
} ELF32_IMAGE_DYNAMIC_ENTRY, *PELF32_IMAGE_DYNAMIC_ENTRY;

typedef struct _ELF64_IMAGE_DYNAMIC_ENTRY // Elf64_Dyn
{
    long long d_tag;
    unsigned long long d_val;
} ELF64_IMAGE_DYNAMIC_ENTRY, *PELF64_IMAGE_DYNAMIC_ENTRY;

// ELF symbol entries

typedef struct _ELF_IMAGE_SYMBOL_ENTRY // Elf_Sym
{
    unsigned int st_name;
    unsigned char st_info;
    unsigned char st_other;
    unsigned short st_shndx;
    unsigned long long st_value;
    unsigned long long st_size;
} ELF_IMAGE_SYMBOL_ENTRY, *PELF_IMAGE_SYMBOL_ENTRY;

// ELF version entires

typedef struct _ELF_VERSION_TABLE // Elf_Versym
{
    unsigned short vs_vers;
} ELF_VERSION_TABLE, *PELF_VERSION_TABLE;

typedef struct
{
    unsigned short vd_version;
    unsigned short vd_flags; // flags (VER_FLG_*).
    unsigned short vd_ndx; // version index.
    unsigned short vd_cnt; // number of verdaux entries.
    unsigned int vd_hash; // hash of name.
    unsigned int vd_aux; // offset to verdaux entries.
    unsigned int vd_next; // offset to next verdef.
} Elf_Verdef;

typedef struct
{
    unsigned int vda_name;  /* string table offset of name */
    unsigned int vda_next;  /* offset to verdaux */
} Elf_Verdaux;

// vn_version
#define VER_NEED_NONE    0 /* No version */
#define VER_NEED_CURRENT 1 /* Current version */
#define VER_NEED_NUM     2 /* Given version number */

typedef struct _ELF_VERSION_NEED // Elf_Verneed
{
    unsigned short vn_version;
    unsigned short vn_cnt;
    unsigned int vn_file;
    unsigned int vn_aux;
    unsigned int vn_next;
} ELF_VERSION_NEED, *PELF_VERSION_NEED;

typedef struct _ELF_VERSION_AUX // Elf_Vernaux
{
    unsigned int vna_hash; // dependency name hash.
    unsigned short vna_flags; // flags (VER_FLG_*).
    unsigned short vna_other;
    unsigned int vna_name;
    unsigned int vna_next;
} ELF_VERSION_AUX, *PELF_VERSION_AUX;

#endif
