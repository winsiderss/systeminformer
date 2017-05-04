#ifndef _PH_MAPIMG_H
#define _PH_MAPIMG_H

#ifdef __cplusplus
extern "C" {
#endif

typedef struct _PH_MAPPED_IMAGE
{
    PVOID ViewBase;
    SIZE_T Size;

    union {
        PIMAGE_NT_HEADERS32 NtHeaders32;
        PIMAGE_NT_HEADERS NtHeaders;
    };

    ULONG NumberOfSections;
    PIMAGE_SECTION_HEADER Sections;
    USHORT Magic;
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
    _In_ BOOLEAN ReadOnly,
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
    _In_ BOOLEAN ReadOnly,
    _Out_ PVOID *ViewBase,
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
PVOID
NTAPI
PhMappedImageRvaToVa(
    _In_ PPH_MAPPED_IMAGE MappedImage,
    _In_ ULONG Rva,
    _Out_opt_ PIMAGE_SECTION_HEADER *Section
    );

PHLIBAPI
BOOLEAN
NTAPI
PhGetMappedImageSectionName(
    _In_ PIMAGE_SECTION_HEADER Section,
    _Out_writes_opt_z_(Count) PSTR Buffer,
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

    PIMAGE_NT_HEADERS NtHeaders;
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

NTSTATUS
NTAPI
PhUnloadRemoteMappedImage(
    _Inout_ PPH_REMOTE_MAPPED_IMAGE RemoteMappedImage
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
    PSTR Name;
    ULONG NumberOfEntries;

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
    _In_ BOOLEAN ReadOnly,
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

typedef struct _IMAGE_CFG_ENTRY
{
    ULONG Rva;
    struct
    {
        BOOLEAN SuppressedCall : 1;
        BOOLEAN Reserved : 7;
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

    PULONGLONG GuardAdressIatTable; // not currently used
    ULONGLONG NumberOfGuardAdressIatEntries;

    PULONGLONG GuardLongJumpTable; // not currently used
    ULONGLONG NumberOfGuardLongJumpEntries;
} PH_MAPPED_IMAGE_CFG, *PPH_MAPPED_IMAGE_CFG;

typedef enum _CFG_ENTRY_TYPE
{
    ControlFlowGuardFunction,
    ControlFlowGuardtakenIatEntry,
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

#ifdef __cplusplus
}
#endif

#endif
