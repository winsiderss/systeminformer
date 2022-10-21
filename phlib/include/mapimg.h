#ifndef _PH_MAPIMG_H
#define _PH_MAPIMG_H

#ifdef __cplusplus
extern "C" {
#endif

#include <exlf.h>
#include <exprodid.h>

typedef struct _PH_MAPPED_IMAGE
{
    USHORT Signature;
    PVOID ViewBase;
    SIZE_T Size;

    union
    {
        struct
        {
            union
            {
                PIMAGE_NT_HEADERS32 NtHeaders32;
                PIMAGE_NT_HEADERS NtHeaders;
            };

            ULONG NumberOfSections;
            PIMAGE_SECTION_HEADER Sections;
            USHORT Magic;
        };
        struct
        {
            struct _ELF_IMAGE_HEADER *Header;
            union
            {
                struct _ELF_IMAGE_HEADER32 *Headers32;
                struct _ELF_IMAGE_HEADER64 *Headers64;
            };
        };
    };
} PH_MAPPED_IMAGE, *PPH_MAPPED_IMAGE;

PHLIBAPI
NTSTATUS
NTAPI
PhInitializeMappedImage(
    _Out_ PPH_MAPPED_IMAGE MappedImage,
    _In_ PVOID ViewBase,
    _In_ SIZE_T Size
    );

PHLIBAPI
NTSTATUS
NTAPI
PhLoadMappedImage(
    _In_opt_ PWSTR FileName,
    _In_opt_ HANDLE FileHandle,
    _Out_ PPH_MAPPED_IMAGE MappedImage
    );

PHLIBAPI
NTSTATUS
NTAPI
PhLoadMappedImageEx(
    _In_opt_ PPH_STRINGREF FileName,
    _In_opt_ HANDLE FileHandle,
    _Out_ PPH_MAPPED_IMAGE MappedImage
    );

PHLIBAPI
NTSTATUS
NTAPI
PhUnloadMappedImage(
    _Inout_ PPH_MAPPED_IMAGE MappedImage
    );

PHLIBAPI
NTSTATUS
NTAPI
PhMapViewOfEntireFile(
    _In_opt_ PWSTR FileName,
    _In_opt_ HANDLE FileHandle,
    _Out_ PVOID *ViewBase,
    _Out_ PSIZE_T Size
    );

PHLIBAPI
NTSTATUS
NTAPI
PhMapViewOfEntireFileEx(
    _In_opt_ PPH_STRINGREF FileName,
    _In_opt_ HANDLE FileHandle,
    _Out_ PVOID* ViewBase,
    _Out_ PSIZE_T Size
    );

PHLIBAPI
PIMAGE_SECTION_HEADER
NTAPI
PhMappedImageRvaToSection(
    _In_ PPH_MAPPED_IMAGE MappedImage,
    _In_ ULONG Rva
    );

PHLIBAPI
_Must_inspect_result_
_Ret_maybenull_
_Success_(return != NULL)
PVOID
NTAPI
PhMappedImageRvaToVa(
    _In_ PPH_MAPPED_IMAGE MappedImage,
    _In_ ULONG Rva,
    _Out_opt_ PIMAGE_SECTION_HEADER *Section
    );

PHLIBAPI
_Must_inspect_result_
_Ret_maybenull_
_Success_(return != NULL)
PVOID
NTAPI
PhMappedImageVaToVa(
    _In_ PPH_MAPPED_IMAGE MappedImage,
    _In_ ULONG Va,
    _Out_opt_ PIMAGE_SECTION_HEADER* Section
    );

PHLIBAPI
BOOLEAN
NTAPI
PhGetMappedImageSectionName(
    _In_ PIMAGE_SECTION_HEADER Section,
    _Out_writes_opt_z_(Count) PWSTR Buffer,
    _In_ ULONG Count,
    _Out_opt_ PULONG ReturnCount
    );

PHLIBAPI
NTSTATUS
NTAPI
PhGetMappedImageDataEntry(
    _In_ PPH_MAPPED_IMAGE MappedImage,
    _In_ ULONG Index,
    _Out_ PIMAGE_DATA_DIRECTORY *Entry
    );

PHLIBAPI
PVOID
NTAPI
PhGetMappedImageDirectoryEntry(
    _In_ PPH_MAPPED_IMAGE MappedImage,
    _In_ ULONG Index
    );

PHLIBAPI
NTSTATUS
NTAPI
PhGetMappedImageLoadConfig32(
    _In_ PPH_MAPPED_IMAGE MappedImage,
    _Out_ PIMAGE_LOAD_CONFIG_DIRECTORY32 *LoadConfig
    );

PHLIBAPI
NTSTATUS
NTAPI
PhGetMappedImageLoadConfig64(
    _In_ PPH_MAPPED_IMAGE MappedImage,
    _Out_ PIMAGE_LOAD_CONFIG_DIRECTORY64 *LoadConfig
    );

typedef struct _PH_REMOTE_MAPPED_IMAGE
{
    PVOID ViewBase;
    union
    {
        PIMAGE_NT_HEADERS32 NtHeaders32;
        PIMAGE_NT_HEADERS NtHeaders;
    };
    ULONG NumberOfSections;
    PIMAGE_SECTION_HEADER Sections;
    USHORT Magic;
} PH_REMOTE_MAPPED_IMAGE, *PPH_REMOTE_MAPPED_IMAGE;

NTSTATUS
NTAPI
PhLoadRemoteMappedImage(
    _In_ HANDLE ProcessHandle,
    _In_ PVOID ViewBase,
    _Out_ PPH_REMOTE_MAPPED_IMAGE RemoteMappedImage
    );

typedef NTSTATUS (NTAPI *PPH_READ_VIRTUAL_MEMORY_CALLBACK)(
    _In_ HANDLE ProcessHandle,
    _In_opt_ PVOID BaseAddress,
    _Out_writes_bytes_(BufferSize) PVOID Buffer,
    _In_ SIZE_T BufferSize,
    _Out_opt_ PSIZE_T NumberOfBytesRead
    );

NTSTATUS
NTAPI
PhLoadRemoteMappedImageEx(
    _In_ HANDLE ProcessHandle,
    _In_ PVOID ViewBase,
    _In_ PPH_READ_VIRTUAL_MEMORY_CALLBACK ReadVirtualMemoryCallback,
    _Out_ PPH_REMOTE_MAPPED_IMAGE RemoteMappedImage
    );

NTSTATUS
NTAPI
PhUnloadRemoteMappedImage(
    _Inout_ PPH_REMOTE_MAPPED_IMAGE RemoteMappedImage
    );

PHLIBAPI
_Success_(return)
BOOLEAN
NTAPI
PhGetRemoteMappedImageDebugEntryByType(
    _In_ HANDLE ProcessHandle,
    _In_ PPH_REMOTE_MAPPED_IMAGE RemoteMappedImage,
    _In_ ULONG Type,
    _Out_opt_ ULONG* EntryLength,
    _Out_ PVOID* EntryBuffer
    );

PHLIBAPI
_Success_(return)
BOOLEAN
NTAPI
PhGetRemoteMappedImageDebugEntryByTypeEx(
    _In_ HANDLE ProcessHandle,
    _In_ PPH_REMOTE_MAPPED_IMAGE RemoteMappedImage,
    _In_ ULONG Type,
    _In_ PPH_READ_VIRTUAL_MEMORY_CALLBACK ReadVirtualMemoryCallback,
    _Out_opt_ ULONG* EntryLength,
    _Out_ PVOID* EntryBuffer
    );

PHLIBAPI
_Success_(return)
BOOLEAN
NTAPI
PhGetRemoteMappedImageGuardFlags(
    _In_ HANDLE ProcessHandle,
    _In_ PPH_REMOTE_MAPPED_IMAGE RemoteMappedImage,
    _Out_ PULONG GuardFlags
    );

PHLIBAPI
_Success_(return)
BOOLEAN
NTAPI
PhGetRemoteMappedImageGuardFlagsEx(
    _In_ HANDLE ProcessHandle,
    _In_ PPH_REMOTE_MAPPED_IMAGE RemoteMappedImage,
    _In_ PPH_READ_VIRTUAL_MEMORY_CALLBACK ReadVirtualMemoryCallback,
    _Out_ PULONG GuardFlags
    );

typedef struct _PH_MAPPED_IMAGE_EXPORTS
{
    PPH_MAPPED_IMAGE MappedImage;
    ULONG NumberOfEntries;

    PIMAGE_DATA_DIRECTORY DataDirectory;
    PIMAGE_EXPORT_DIRECTORY ExportDirectory;
    PULONG AddressTable;
    PULONG NamePointerTable;
    PUSHORT OrdinalTable;
} PH_MAPPED_IMAGE_EXPORTS, *PPH_MAPPED_IMAGE_EXPORTS;

typedef struct _PH_MAPPED_IMAGE_EXPORT_ENTRY
{
    USHORT Ordinal;
    ULONG Hint;
    PSTR Name;
} PH_MAPPED_IMAGE_EXPORT_ENTRY, *PPH_MAPPED_IMAGE_EXPORT_ENTRY;

typedef struct _PH_MAPPED_IMAGE_EXPORT_FUNCTION
{
    PVOID Function;
    PSTR ForwardedName;
} PH_MAPPED_IMAGE_EXPORT_FUNCTION, *PPH_MAPPED_IMAGE_EXPORT_FUNCTION;

PHLIBAPI
NTSTATUS
NTAPI
PhGetMappedImageExports(
    _Out_ PPH_MAPPED_IMAGE_EXPORTS Exports,
    _In_ PPH_MAPPED_IMAGE MappedImage
    );

PHLIBAPI
NTSTATUS
NTAPI
PhGetMappedImageExportEntry(
    _In_ PPH_MAPPED_IMAGE_EXPORTS Exports,
    _In_ ULONG Index,
    _Out_ PPH_MAPPED_IMAGE_EXPORT_ENTRY Entry
    );

PHLIBAPI
NTSTATUS
NTAPI
PhGetMappedImageExportFunction(
    _In_ PPH_MAPPED_IMAGE_EXPORTS Exports,
    _In_opt_ PSTR Name,
    _In_opt_ USHORT Ordinal,
    _Out_ PPH_MAPPED_IMAGE_EXPORT_FUNCTION Function
    );

PHLIBAPI
NTSTATUS
NTAPI
PhGetMappedImageExportFunctionRemote(
    _In_ PPH_MAPPED_IMAGE_EXPORTS Exports,
    _In_opt_ PSTR Name,
    _In_opt_ USHORT Ordinal,
    _In_ PVOID RemoteBase,
    _Out_ PVOID *Function
    );

#define PH_MAPPED_IMAGE_DELAY_IMPORTS 0x1
#define PH_MAPPED_IMAGE_DELAY_IMPORTS_V1 0x2

typedef struct _PH_MAPPED_IMAGE_IMPORTS
{
    PPH_MAPPED_IMAGE MappedImage;
    ULONG Flags;
    ULONG NumberOfDlls;

    union
    {
        PIMAGE_IMPORT_DESCRIPTOR DescriptorTable;
        PIMAGE_DELAYLOAD_DESCRIPTOR DelayDescriptorTable;
    };
} PH_MAPPED_IMAGE_IMPORTS, *PPH_MAPPED_IMAGE_IMPORTS;

typedef struct _PH_MAPPED_IMAGE_IMPORT_DLL
{
    PPH_MAPPED_IMAGE MappedImage;
    ULONG Flags;
    ULONG NumberOfEntries;
    PSTR Name;

    union
    {
        PIMAGE_IMPORT_DESCRIPTOR Descriptor;
        PIMAGE_DELAYLOAD_DESCRIPTOR DelayDescriptor;
    };
    PVOID LookupTable;
} PH_MAPPED_IMAGE_IMPORT_DLL, *PPH_MAPPED_IMAGE_IMPORT_DLL;

typedef struct _PH_MAPPED_IMAGE_IMPORT_ENTRY
{
    PSTR Name;
    union
    {
        USHORT Ordinal;
        USHORT NameHint;
    };
} PH_MAPPED_IMAGE_IMPORT_ENTRY, *PPH_MAPPED_IMAGE_IMPORT_ENTRY;

PHLIBAPI
NTSTATUS
NTAPI
PhGetMappedImageImports(
    _Out_ PPH_MAPPED_IMAGE_IMPORTS Imports,
    _In_ PPH_MAPPED_IMAGE MappedImage
    );

PHLIBAPI
NTSTATUS
NTAPI
PhGetMappedImageImportDll(
    _In_ PPH_MAPPED_IMAGE_IMPORTS Imports,
    _In_ ULONG Index,
    _Out_ PPH_MAPPED_IMAGE_IMPORT_DLL ImportDll
    );

PHLIBAPI
NTSTATUS
NTAPI
PhGetMappedImageImportEntry(
    _In_ PPH_MAPPED_IMAGE_IMPORT_DLL ImportDll,
    _In_ ULONG Index,
    _Out_ PPH_MAPPED_IMAGE_IMPORT_ENTRY Entry
    );

PHLIBAPI
NTSTATUS
NTAPI
PhGetMappedImageDelayImports(
    _Out_ PPH_MAPPED_IMAGE_IMPORTS Imports,
    _In_ PPH_MAPPED_IMAGE MappedImage
    );

USHORT
NTAPI
PhCheckSum(
    _In_ ULONG Sum,
    _In_reads_(Count) PUSHORT Buffer,
    _In_ ULONG Count
    );

PHLIBAPI
ULONG
NTAPI
PhCheckSumMappedImage(
    _In_ PPH_MAPPED_IMAGE MappedImage
    );

typedef struct _IMAGE_CFG_ENTRY
{
    ULONG Rva;
    struct
    {
        BOOLEAN SuppressedCall : 1;
        BOOLEAN ExportSuppressed : 1;
        BOOLEAN LangExcptHandler : 1;
        BOOLEAN Xfg : 1;
        BOOLEAN Reserved : 4;
    };
} IMAGE_CFG_ENTRY, *PIMAGE_CFG_ENTRY;

typedef struct _PH_MAPPED_IMAGE_CFG
{
    PPH_MAPPED_IMAGE MappedImage;
    ULONG EntrySize;

    union
    {
        ULONG GuardFlags;
        struct
        {
            ULONG CfgInstrumented : 1;
            ULONG WriteIntegrityChecks : 1;
            ULONG CfgFunctionTablePresent : 1;
            ULONG SecurityCookieUnused : 1;
            ULONG ProtectDelayLoadedIat : 1;
            ULONG DelayLoadInDidatSection : 1;
            ULONG HasExportSuppressionInfos : 1;
            ULONG EnableExportSuppression : 1;
            ULONG CfgLongJumpTablePresent : 1;
            ULONG Spare : 23;
        };
    };

    PULONGLONG GuardFunctionTable;
    ULONGLONG NumberOfGuardFunctionEntries;

    PULONGLONG GuardAdressIatTable;
    ULONGLONG NumberOfGuardAdressIatEntries;

    PULONGLONG GuardLongJumpTable;
    ULONGLONG NumberOfGuardLongJumpEntries;
} PH_MAPPED_IMAGE_CFG, *PPH_MAPPED_IMAGE_CFG;

typedef enum _CFG_ENTRY_TYPE
{
    ControlFlowGuardFunction,
    ControlFlowGuardTakenIatEntry,
    ControlFlowGuardLongJump
} CFG_ENTRY_TYPE;

PHLIBAPI
NTSTATUS
NTAPI
PhGetMappedImageCfg(
    _Out_ PPH_MAPPED_IMAGE_CFG CfgConfig,
    _In_ PPH_MAPPED_IMAGE MappedImage
    );

PHLIBAPI
NTSTATUS
NTAPI
PhGetMappedImageCfgEntry(
    _In_ PPH_MAPPED_IMAGE_CFG CfgConfig,
    _In_ ULONGLONG Index,
    _In_ CFG_ENTRY_TYPE Type,
    _Out_ PIMAGE_CFG_ENTRY Entry
    );

typedef struct _PH_IMAGE_RESOURCE_ENTRY
{
    ULONG_PTR Type;
    ULONG_PTR Name;
    ULONG_PTR Language;
    ULONG Offset;
    ULONG Size;
    ULONG CodePage;
    PVOID Data;
} PH_IMAGE_RESOURCE_ENTRY, *PPH_IMAGE_RESOURCE_ENTRY;

typedef struct _PH_MAPPED_IMAGE_RESOURCES
{
    PPH_MAPPED_IMAGE MappedImage;
    PIMAGE_DATA_DIRECTORY DataDirectory;
    PIMAGE_RESOURCE_DIRECTORY ResourceDirectory;

    ULONG NumberOfEntries;
    PPH_IMAGE_RESOURCE_ENTRY ResourceEntries;
} PH_MAPPED_IMAGE_RESOURCES, *PPH_MAPPED_IMAGE_RESOURCES;

PHLIBAPI
NTSTATUS
NTAPI
PhGetMappedImageResources(
    _Out_ PPH_MAPPED_IMAGE_RESOURCES Resources,
    _In_ PPH_MAPPED_IMAGE MappedImage
    );

typedef struct _PH_IMAGE_TLS_CALLBACK_ENTRY
{
    ULONGLONG Index;
    ULONGLONG Address;
} PH_IMAGE_TLS_CALLBACK_ENTRY, *PPH_IMAGE_TLS_CALLBACK_ENTRY;

typedef struct _PH_MAPPED_IMAGE_TLS_CALLBACKS
{
    PPH_MAPPED_IMAGE MappedImage;
    PIMAGE_DATA_DIRECTORY DataDirectory;

    union
    {
        PIMAGE_TLS_DIRECTORY32 TlsDirectory32;
        PIMAGE_TLS_DIRECTORY64 TlsDirectory64;
    };

    //PVOID CallbackIndexes;
    PVOID CallbackAddress;

    ULONG NumberOfEntries;
    PPH_IMAGE_TLS_CALLBACK_ENTRY Entries;
} PH_MAPPED_IMAGE_TLS_CALLBACKS, *PPH_MAPPED_IMAGE_TLS_CALLBACKS;

PHLIBAPI
NTSTATUS
NTAPI
PhGetMappedImageTlsCallbacks(
    _Out_ PPH_MAPPED_IMAGE_TLS_CALLBACKS TlsCallbacks,
    _In_ PPH_MAPPED_IMAGE MappedImage
    );

typedef struct _PH_MAPPED_IMAGE_PRODID_ENTRY
{
    USHORT ProductId;
    USHORT ProductBuild;
    ULONG ProductCount;
} PH_MAPPED_IMAGE_PRODID_ENTRY, *PPH_MAPPED_IMAGE_PRODID_ENTRY;

typedef struct _PH_MAPPED_IMAGE_PRODID
{
    //WCHAR Key[PH_PTR_STR_LEN_1];
    BOOLEAN Valid;
    PPH_STRING Key;
    PPH_STRING RawHash;
    PPH_STRING Hash;
    ULONG NumberOfEntries;
    PPH_MAPPED_IMAGE_PRODID_ENTRY ProdIdEntries;
} PH_MAPPED_IMAGE_PRODID, *PPH_MAPPED_IMAGE_PRODID;

PHLIBAPI
NTSTATUS
NTAPI
PhGetMappedImageProdIdHeader(
    _In_ PPH_MAPPED_IMAGE MappedImage,
    _Out_ PPH_MAPPED_IMAGE_PRODID ProdIdHeader
    );

PHLIBAPI
NTSTATUS
NTAPI
PhGetMappedImageProdIdExtents(
    _In_ PPH_MAPPED_IMAGE MappedImage,
    _Out_ PULONG ProdIdHeaderStart,
    _Out_ PULONG ProdIdHeaderEnd
    );

typedef struct _PH_IMAGE_DEBUG_ENTRY
{
    ULONG Characteristics;
    ULONG TimeDateStamp;
    USHORT MajorVersion;
    USHORT MinorVersion;
    ULONG Type;
    ULONG SizeOfData;
    ULONG AddressOfRawData;
    ULONG PointerToRawData;
} PH_IMAGE_DEBUG_ENTRY, *PPH_IMAGE_DEBUG_ENTRY;

typedef struct _PH_MAPPED_IMAGE_DEBUG
{
    PPH_MAPPED_IMAGE MappedImage;
    PIMAGE_DATA_DIRECTORY DataDirectory;
    PIMAGE_DEBUG_DIRECTORY DebugDirectory;

    ULONG NumberOfEntries;
    PPH_IMAGE_DEBUG_ENTRY DebugEntries;
} PH_MAPPED_IMAGE_DEBUG, *PPH_MAPPED_IMAGE_DEBUG;

// Note: Remove once they've been added to the Windows SDK. (dmex)
#ifndef IMAGE_DEBUG_TYPE_EMBEDDEDPORTABLEPDB
#define IMAGE_DEBUG_TYPE_EMBEDDEDPORTABLEPDB 17
#endif

#ifndef IMAGE_DEBUG_TYPE_PDBCHECKSUM
#define IMAGE_DEBUG_TYPE_PDBCHECKSUM 19
#endif

#define CODEVIEW_SIGNATURE_NB10 '01BN'
#define CODEVIEW_SIGNATURE_RSDS 'SDSR'

typedef struct _CODEVIEW_HEADER
{
    ULONG Signature;
    LONG Offset;
} CODEVIEW_HEADER, *PCODEVIEW_HEADER;

typedef struct _CODEVIEW_INFO_PDB20
{
    CODEVIEW_HEADER Header;
    ULONG Timestamp; // seconds since 1970
    ULONG Age;
    CHAR PdbFileName[1];
} CODEVIEW_INFO_PDB20, *PCODEVIEW_INFO_PDB20;

typedef struct _CODEVIEW_INFO_PDB70
{
    ULONG Signature;
    GUID PdbGuid;
    ULONG PdbAge;
    CHAR ImageName[1];
} CODEVIEW_INFO_PDB70, *PCODEVIEW_INFO_PDB70;

// IMAGE_GUARD_EH_CONTINUATION_TABLE_PRESENT, Windows 20H1 and 21H1 have different values
#define IMAGE_GUARD_EH_CONTINUATION_TABLE_PRESENT_V1 0x00200000
#define IMAGE_GUARD_EH_CONTINUATION_TABLE_PRESENT_V2 0x00400000

#ifndef IMAGE_GUARD_XFG_ENABLED
#define IMAGE_GUARD_XFG_ENABLED 0x00800000 // Module was built with xfg
#endif

PHLIBAPI
NTSTATUS
NTAPI
PhGetMappedImageDebug(
    _In_ PPH_MAPPED_IMAGE MappedImage,
    _Out_ PPH_MAPPED_IMAGE_DEBUG Debug
    );

PHLIBAPI
NTSTATUS
NTAPI
PhGetMappedImageDebugEntryByType(
    _In_ PPH_MAPPED_IMAGE MappedImage,
    _In_ ULONG Type,
    _Out_opt_ ULONG* EntryLength,
    _Out_opt_ PVOID* EntryBuffer
    );

// maplib

struct _PH_MAPPED_ARCHIVE;
typedef struct _PH_MAPPED_ARCHIVE *PPH_MAPPED_ARCHIVE;

typedef enum _PH_MAPPED_ARCHIVE_MEMBER_TYPE
{
    NormalArchiveMemberType,
    LinkerArchiveMemberType,
    LongnamesArchiveMemberType
} PH_MAPPED_ARCHIVE_MEMBER_TYPE;

typedef struct _PH_MAPPED_ARCHIVE_MEMBER
{
    PPH_MAPPED_ARCHIVE MappedArchive;
    PH_MAPPED_ARCHIVE_MEMBER_TYPE Type;
    PSTR Name;
    ULONG Size;
    PVOID Data;

    PIMAGE_ARCHIVE_MEMBER_HEADER Header;
    CHAR NameBuffer[20];
} PH_MAPPED_ARCHIVE_MEMBER, *PPH_MAPPED_ARCHIVE_MEMBER;

typedef struct _PH_MAPPED_ARCHIVE
{
    PVOID ViewBase;
    SIZE_T Size;

    PH_MAPPED_ARCHIVE_MEMBER FirstLinkerMember;
    PH_MAPPED_ARCHIVE_MEMBER SecondLinkerMember;
    PH_MAPPED_ARCHIVE_MEMBER LongnamesMember;
    BOOLEAN HasLongnamesMember;

    PPH_MAPPED_ARCHIVE_MEMBER FirstStandardMember;
    PPH_MAPPED_ARCHIVE_MEMBER LastStandardMember;
} PH_MAPPED_ARCHIVE, *PPH_MAPPED_ARCHIVE;

typedef struct _PH_MAPPED_ARCHIVE_IMPORT_ENTRY
{
    PSTR Name;
    PSTR DllName;
    union
    {
        USHORT Ordinal;
        USHORT NameHint;
    };
    BYTE Type;
    BYTE NameType;
    USHORT Machine;
} PH_MAPPED_ARCHIVE_IMPORT_ENTRY, *PPH_MAPPED_ARCHIVE_IMPORT_ENTRY;

PHLIBAPI
NTSTATUS
NTAPI
PhInitializeMappedArchive(
    _Out_ PPH_MAPPED_ARCHIVE MappedArchive,
    _In_ PVOID ViewBase,
    _In_ SIZE_T Size
    );

PHLIBAPI
NTSTATUS
NTAPI
PhLoadMappedArchive(
    _In_opt_ PWSTR FileName,
    _In_opt_ HANDLE FileHandle,
    _Out_ PPH_MAPPED_ARCHIVE MappedArchive
    );

PHLIBAPI
NTSTATUS
NTAPI
PhUnloadMappedArchive(
    _Inout_ PPH_MAPPED_ARCHIVE MappedArchive
    );

PHLIBAPI
NTSTATUS
NTAPI
PhGetNextMappedArchiveMember(
    _In_ PPH_MAPPED_ARCHIVE_MEMBER Member,
    _Out_ PPH_MAPPED_ARCHIVE_MEMBER NextMember
    );

PHLIBAPI
BOOLEAN
NTAPI
PhIsMappedArchiveMemberShortFormat(
    _In_ PPH_MAPPED_ARCHIVE_MEMBER Member
    );

PHLIBAPI
NTSTATUS
NTAPI
PhGetMappedArchiveImportEntry(
    _In_ PPH_MAPPED_ARCHIVE_MEMBER Member,
    _Out_ PPH_MAPPED_ARCHIVE_IMPORT_ENTRY Entry
    );

typedef struct _PH_MAPPED_IMAGE_EH_CONT
{
    PULONGLONG EhContTable;
    ULONGLONG NumberOfEhContEntries;
    ULONG EntrySize;
} PH_MAPPED_IMAGE_EH_CONT, *PPH_MAPPED_IMAGE_EH_CONT;

PHLIBAPI
NTSTATUS
NTAPI
PhGetMappedImageEhCont(
    _Out_ PPH_MAPPED_IMAGE_EH_CONT EhContConfig,
    _In_ PPH_MAPPED_IMAGE MappedImage
    );

typedef struct _IMAGE_DEBUG_POGO_ENTRY
{
    ULONG Rva;
    ULONG Size;
    CHAR Name[1];
} IMAGE_DEBUG_POGO_ENTRY, *PIMAGE_DEBUG_POGO_ENTRY;

typedef struct _IMAGE_DEBUG_POGO_SIGNATURE
{
    ULONG Signature;
} IMAGE_DEBUG_POGO_SIGNATURE, *PIMAGE_DEBUG_POGO_SIGNATURE;

#define IMAGE_DEBUG_POGO_SIGNATURE_LTCG 'LTCG' // coffgrp LTCG (0x4C544347)
#define IMAGE_DEBUG_POGO_SIGNATURE_PGU 'PGU\0' // coffgrp PGU (0x50475500)

typedef struct _PH_IMAGE_DEBUG_POGO_ENTRY
{
    ULONG Rva;
    ULONG Size;
    PVOID Data;
    WCHAR Name[0x100];
} PH_IMAGE_DEBUG_POGO_ENTRY, *PPH_IMAGE_DEBUG_POGO_ENTRY;

typedef struct _PH_MAPPED_IMAGE_DEBUG_POGO
{
    PPH_MAPPED_IMAGE MappedImage;
    PIMAGE_DEBUG_POGO_SIGNATURE PogoDirectory;

    ULONG NumberOfEntries;
    PPH_IMAGE_DEBUG_POGO_ENTRY PogoEntries;
} PH_MAPPED_IMAGE_DEBUG_POGO, *PPH_MAPPED_IMAGE_DEBUG_POGO;

PHLIBAPI
NTSTATUS
NTAPI
PhGetMappedImagePogo(
    _In_ PPH_MAPPED_IMAGE MappedImage,
    _Out_ PPH_MAPPED_IMAGE_DEBUG_POGO PogoDebug
    );

typedef struct _IMAGE_BASE_RELOCATION_ENTRY
{
    USHORT Offset : 12;
    USHORT Type : 4;
} IMAGE_BASE_RELOCATION_ENTRY, *PIMAGE_BASE_RELOCATION_ENTRY;

typedef struct _PH_IMAGE_RELOC_ENTRY
{
    ULONG BlockIndex;
    ULONG BlockRva;
    ULONG Type;
    ULONG Offset;
    PVOID ImageBaseVa;
    PVOID MappedImageVa;
} PH_IMAGE_RELOC_ENTRY, *PPH_IMAGE_RELOC_ENTRY;

typedef struct _PH_MAPPED_IMAGE_RELOC
{
    PPH_MAPPED_IMAGE MappedImage;
    PIMAGE_DATA_DIRECTORY DataDirectory;
    PIMAGE_BASE_RELOCATION FirstRelocationDirectory;

    ULONG NumberOfEntries;
    PPH_IMAGE_RELOC_ENTRY RelocationEntries;
} PH_MAPPED_IMAGE_RELOC, *PPH_MAPPED_IMAGE_RELOC;

PHLIBAPI
NTSTATUS
NTAPI
PhGetMappedImageRelocations(
    _In_ PPH_MAPPED_IMAGE MappedImage,
    _Out_ PPH_MAPPED_IMAGE_RELOC Relocations
    );
PHLIBAPI
VOID
NTAPI
PhFreeMappedImageRelocations(
    _In_ PPH_MAPPED_IMAGE_RELOC Relocations
    );

typedef struct _PH_MAPPED_IMAGE_EXCEPTIONS
{
    PPH_MAPPED_IMAGE MappedImage;
    PIMAGE_DATA_DIRECTORY DataDirectory;
    PVOID ExceptionDirectory;

    ULONG NumberOfEntries;
    PVOID ExceptionEntries;
} PH_MAPPED_IMAGE_EXCEPTIONS, *PPH_MAPPED_IMAGE_EXCEPTIONS;

PHLIBAPI
NTSTATUS
NTAPI
PhGetMappedImageExceptions(
    _In_ PPH_MAPPED_IMAGE MappedImage,
    _Out_ PPH_MAPPED_IMAGE_EXCEPTIONS Exceptions
    );

// ELF binary support

NTSTATUS PhInitializeMappedWslImage(
    _Out_ PPH_MAPPED_IMAGE MappedWslImage,
    _In_ PVOID ViewBase,
    _In_ SIZE_T Size
    );

ULONG64 PhGetMappedWslImageBaseAddress(
    _In_ PPH_MAPPED_IMAGE MappedWslImage
    );

typedef struct _PH_ELF_IMAGE_SECTION
{
    UINT32 Type;
    ULONGLONG Flags;
    ULONGLONG Address;
    ULONGLONG Offset;
    ULONGLONG Size;
    WCHAR Name[MAX_PATH];
} PH_ELF_IMAGE_SECTION, *PPH_ELF_IMAGE_SECTION;

BOOLEAN PhGetMappedWslImageSections(
    _In_ PPH_MAPPED_IMAGE MappedWslImage,
    _Out_ USHORT *NumberOfSections,
    _Out_ PPH_ELF_IMAGE_SECTION *ImageSections
    );

typedef struct _PH_ELF_IMAGE_SYMBOL_ENTRY
{
    union
    {
        BOOLEAN Flags;
        struct
        {
            BOOLEAN ImportSymbol : 1;
            BOOLEAN ExportSymbol : 1;
            BOOLEAN UnknownSymbol : 1;
            BOOLEAN Spare : 5;
        };
    };
    UCHAR TypeInfo;
    UCHAR OtherInfo;
    ULONG SectionIndex;
    ULONGLONG Address;
    ULONGLONG Size;
    WCHAR Name[MAX_PATH * 2];
    WCHAR Module[MAX_PATH * 2];
} PH_ELF_IMAGE_SYMBOL_ENTRY, *PPH_ELF_IMAGE_SYMBOL_ENTRY;

BOOLEAN PhGetMappedWslImageSymbols(
    _In_ PPH_MAPPED_IMAGE MappedWslImage,
    _Out_ PPH_LIST *ImageSymbols
    );

VOID PhFreeMappedWslImageSymbols(
    _In_ PPH_LIST ImageSymbols
    );

typedef struct _PH_ELF_IMAGE_DYNAMIC_ENTRY
{
    LONGLONG Tag;
    PWSTR Type;
    PPH_STRING Value;
} PH_ELF_IMAGE_DYNAMIC_ENTRY, *PPH_ELF_IMAGE_DYNAMIC_ENTRY;

BOOLEAN PhGetMappedWslImageDynamic(
    _In_ PPH_MAPPED_IMAGE MappedWslImage,
    _Out_ PPH_LIST *DynamicSymbols
    );

VOID PhFreeMappedWslImageDynamic(
    _In_ PPH_LIST ImageDynamic
    );

#ifdef __cplusplus
}
#endif

#endif
