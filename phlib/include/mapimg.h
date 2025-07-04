/*
 * Copyright (c) 2022 Winsider Seminars & Solutions, Inc.  All rights reserved.
 *
 * This file is part of System Informer.
 *
 * Authors:
 *
 *     wj32    2016
 *     dmex    2017-2023
 *     jx-s    2023
 *
 */

#ifndef _PH_MAPIMG_H
#define _PH_MAPIMG_H

EXTERN_C_START

#include <exlf.h>
#include <exprodid.h>

typedef struct _PH_MAPPED_IMAGE
{
    USHORT Signature;
    PVOID ViewBase;
    SIZE_T ViewSize;

    union
    {
        struct // PE image
        {
            union
            {
                PIMAGE_NT_HEADERS32 NtHeaders32;
                PIMAGE_NT_HEADERS64 NtHeaders64;
                PIMAGE_NT_HEADERS NtHeaders;
            };

            USHORT Magic;
            USHORT NumberOfSections;
            PIMAGE_SECTION_HEADER Sections;
        };

        struct // ELF image
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

FORCEINLINE
VOID
NTAPI
PhMappedImageProbe(
    _In_ PPH_MAPPED_IMAGE MappedImage,
    _In_ PVOID Address,
    _In_ SIZE_T Length
    )
{
    PhProbeAddress(Address, Length, MappedImage->ViewBase, MappedImage->ViewSize, __alignof(UCHAR));
}

PHLIBAPI
NTSTATUS
NTAPI
PhInitializeMappedImage(
    _Out_ PPH_MAPPED_IMAGE MappedImage,
    _In_ PVOID ViewBase,
    _In_ SIZE_T ViewSize
    );

PHLIBAPI
NTSTATUS
NTAPI
PhLoadMappedImage(
    _In_opt_ PCWSTR FileName,
    _In_opt_ HANDLE FileHandle,
    _Out_ PPH_MAPPED_IMAGE MappedImage
    );

PHLIBAPI
NTSTATUS
NTAPI
PhLoadMappedImageEx(
    _In_opt_ PCPH_STRINGREF FileName,
    _In_opt_ HANDLE FileHandle,
    _Out_ PPH_MAPPED_IMAGE MappedImage
    );

PHLIBAPI
NTSTATUS
NTAPI
PhLoadMappedImageHeaderPageSize(
    _In_opt_ PCPH_STRINGREF FileName,
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
    _In_opt_ PCWSTR FileName,
    _In_opt_ HANDLE FileHandle,
    _Out_ PVOID *ViewBase,
    _Out_ PSIZE_T ViewSize
    );

PHLIBAPI
NTSTATUS
NTAPI
PhMapViewOfEntireFileEx(
    _In_opt_ PCPH_STRINGREF FileName,
    _In_opt_ HANDLE FileHandle,
    _Out_ PVOID* ViewBase,
    _Out_ PSIZE_T ViewSize
    );

PHLIBAPI
VOID
NTAPI
PhMappedImagePrefetch(
    _In_ PPH_MAPPED_IMAGE MappedImage
    );

PHLIBAPI
PIMAGE_SECTION_HEADER
NTAPI
PhMappedImageSectionByName(
    _In_ PPH_MAPPED_IMAGE MappedImage,
    _In_ PCWSTR Name,
    _In_ BOOLEAN IgnoreCase
    );

PHLIBAPI
PIMAGE_SECTION_HEADER
NTAPI
PhMappedImageRvaToSection(
    _In_ PPH_MAPPED_IMAGE MappedImage,
    _In_ ULONG Rva
    );

_Success_(return != NULL)
PHLIBAPI
PVOID
NTAPI
PhMappedImageRvaToVa(
    _In_ PPH_MAPPED_IMAGE MappedImage,
    _In_ ULONG Rva,
    _Out_opt_ PIMAGE_SECTION_HEADER *Section
    );

_Success_(return != NULL)
PHLIBAPI
PVOID
NTAPI
PhMappedImageVaToVa(
    _In_ PPH_MAPPED_IMAGE MappedImage,
    _In_ ULONGLONG Va,
    _Out_opt_ PIMAGE_SECTION_HEADER* Section
    );

PHLIBAPI
NTSTATUS
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
PhGetMappedImageDataDirectory(
    _In_ PPH_MAPPED_IMAGE MappedImage,
    _In_ ULONG Index,
    _Out_ PIMAGE_DATA_DIRECTORY *Entry
    );

PHLIBAPI
NTSTATUS
NTAPI
PhRelocateMappedImageDataEntryARM64X(
    _In_ PPH_MAPPED_IMAGE MappedImage,
    _In_ PIMAGE_DATA_DIRECTORY Entry,
    _Out_ PIMAGE_DATA_DIRECTORY RelocatedEntry
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
    HANDLE ProcessHandle;
    PVOID ViewBase;
    SIZE_T ViewSize;
    union
    {
        PIMAGE_NT_HEADERS32 NtHeaders32;
        PIMAGE_NT_HEADERS64 NtHeaders64;
        PIMAGE_NT_HEADERS NtHeaders;
    };
    USHORT Magic;
    USHORT NumberOfSections;
    PIMAGE_SECTION_HEADER Sections;
    PVOID PageCache;
} PH_REMOTE_MAPPED_IMAGE, *PPH_REMOTE_MAPPED_IMAGE;

FORCEINLINE
VOID
NTAPI
RemoteMappedImageProbe(
    _In_ PPH_REMOTE_MAPPED_IMAGE MappedImage,
    _In_ PVOID Address,
    _In_ SIZE_T Length
    )
{
    PhProbeAddress(Address, Length, MappedImage->ViewBase, MappedImage->ViewSize, 1);
}

PHLIBAPI
NTSTATUS
NTAPI
PhLoadRemoteMappedImage(
    _In_ HANDLE ProcessHandle,
    _In_ PVOID ViewBase,
    _In_ SIZE_T ViewSize,
    _Out_ PPH_REMOTE_MAPPED_IMAGE RemoteMappedImage
    );

typedef NTSTATUS (NTAPI *PPH_READ_VIRTUAL_MEMORY_CALLBACK)(
    _In_ HANDLE ProcessHandle,
    _In_opt_ PVOID BaseAddress,
    _Out_writes_bytes_(BufferSize) PVOID Buffer,
    _In_ SIZE_T BufferSize,
    _Out_opt_ PSIZE_T NumberOfBytesRead
    );

PHLIBAPI
NTSTATUS
NTAPI
PhLoadRemoteMappedImageEx(
    _In_ HANDLE ProcessHandle,
    _In_ PVOID ViewBase,
    _In_ SIZE_T Size,
    _In_opt_ PPH_READ_VIRTUAL_MEMORY_CALLBACK ReadVirtualMemoryCallback,
    _Out_ PPH_REMOTE_MAPPED_IMAGE RemoteMappedImage
    );

PHLIBAPI
NTSTATUS
NTAPI
PhUnloadRemoteMappedImage(
    _Inout_ PPH_REMOTE_MAPPED_IMAGE RemoteMappedImage
    );

PHLIBAPI
NTSTATUS
NTAPI
PhGetRemoteMappedImageDataEntry(
    _In_ PPH_REMOTE_MAPPED_IMAGE RemoteMappedImage,
    _In_ ULONG Index,
    _Out_ PIMAGE_DATA_DIRECTORY * Entry
    );

PHLIBAPI
NTSTATUS
NTAPI
PhGetRemoteMappedImageDirectoryEntry(
    _In_ PPH_REMOTE_MAPPED_IMAGE RemoteMappedImage,
    _In_opt_ PPH_READ_VIRTUAL_MEMORY_CALLBACK ReadVirtualMemoryCallback,
    _In_ ULONG Index,
    _Out_ PVOID* DataBuffer,
    _Out_opt_ ULONG* DataLength
    );

PHLIBAPI
NTSTATUS
NTAPI
PhGetRemoteMappedImageDebugEntryByType(
    _In_ PPH_REMOTE_MAPPED_IMAGE RemoteMappedImage,
    _In_ ULONG Type,
    _Out_opt_ PULONG DataLength,
    _Out_ PPVOID DataBuffer
    );

PHLIBAPI
NTSTATUS
NTAPI
PhGetRemoteMappedImageDebugEntryByTypeEx(
    _In_ PPH_REMOTE_MAPPED_IMAGE RemoteMappedImage,
    _In_ ULONG Type,
    _In_opt_ PPH_READ_VIRTUAL_MEMORY_CALLBACK ReadVirtualMemoryCallback,
    _Out_opt_ PULONG DataLength,
    _Out_ PPVOID DataBuffer
    );

PHLIBAPI
NTSTATUS
NTAPI
PhGetRemoteMappedImageGuardFlags(
    _In_ PPH_REMOTE_MAPPED_IMAGE RemoteMappedImage,
    _Out_ PULONG GuardFlags
    );

PHLIBAPI
NTSTATUS
NTAPI
PhGetRemoteMappedImageGuardFlagsEx(
    _In_ PPH_REMOTE_MAPPED_IMAGE RemoteMappedImage,
    _In_opt_ PPH_READ_VIRTUAL_MEMORY_CALLBACK ReadVirtualMemoryCallback,
    _Out_ PULONG GuardFlags
    );

typedef struct _PH_MAPPED_IMAGE_EXPORTS
{
    PPH_MAPPED_IMAGE MappedImage;
    ULONG NumberOfEntries;

    PIMAGE_DATA_DIRECTORY DataDirectory;
    IMAGE_DATA_DIRECTORY DataDirectoryARM64X;
    PIMAGE_EXPORT_DIRECTORY ExportDirectory;
    PULONG AddressTable;
    PULONG NamePointerTable;
    PUSHORT OrdinalTable;
} PH_MAPPED_IMAGE_EXPORTS, *PPH_MAPPED_IMAGE_EXPORTS;

typedef struct _PH_MAPPED_IMAGE_EXPORT_ENTRY
{
    USHORT Ordinal;
    ULONG Hint;
    PCSTR Name;
} PH_MAPPED_IMAGE_EXPORT_ENTRY, *PPH_MAPPED_IMAGE_EXPORT_ENTRY;

typedef struct _PH_MAPPED_IMAGE_EXPORT_FUNCTION
{
    PVOID Function;
    PCSTR ForwardedName;
} PH_MAPPED_IMAGE_EXPORT_FUNCTION, *PPH_MAPPED_IMAGE_EXPORT_FUNCTION;

#define PH_GET_IMAGE_EXPORTS_ARM64X 0x00000001ul

PHLIBAPI
NTSTATUS
NTAPI
PhGetMappedImageExportsEx(
    _Out_ PPH_MAPPED_IMAGE_EXPORTS Exports,
    _In_ PPH_MAPPED_IMAGE MappedImage,
    _In_ ULONG Flags
    );

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
    _In_opt_ PCSTR Name,
    _In_opt_ USHORT Ordinal,
    _Out_ PPH_MAPPED_IMAGE_EXPORT_FUNCTION Function
    );

PHLIBAPI
NTSTATUS
NTAPI
PhGetMappedImageExportFunctionRemote(
    _In_ PPH_MAPPED_IMAGE_EXPORTS Exports,
    _In_opt_ PCSTR Name,
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
    ULONG NumberOfEntries;
    PCSTR Name;

    union
    {
        PIMAGE_IMPORT_DESCRIPTOR Descriptor;
        PIMAGE_DELAYLOAD_DESCRIPTOR DelayDescriptor;
    };
    PVOID LookupTable;
} PH_MAPPED_IMAGE_IMPORT_DLL, *PPH_MAPPED_IMAGE_IMPORT_DLL;

typedef struct _PH_MAPPED_IMAGE_IMPORT_ENTRY
{
    PCSTR Name;
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
ULONG
NTAPI
PhGetMappedImageImportEntryRva(
    _In_ PPH_MAPPED_IMAGE_IMPORT_DLL ImportDll,
    _In_ ULONG Index,
    _In_ BOOLEAN DelayImport
    );

PHLIBAPI
NTSTATUS
NTAPI
PhGetMappedImageDelayImports(
    _Out_ PPH_MAPPED_IMAGE_IMPORTS Imports,
    _In_ PPH_MAPPED_IMAGE MappedImage
    );

PHLIBAPI
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
    ULONG64 XfgHash;
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
    //PVOID Data; // PhMappedImageRvaToVa(MappedImage, resourceData->OffsetToData, NULL);
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

PHLIBAPI
NTSTATUS
NTAPI
PhGetMappedImageResource(
    _In_ PPH_MAPPED_IMAGE MappedImage,
    _In_ PCWSTR Name,
    _In_ PCWSTR Type,
    _In_opt_ USHORT Language,
    _Out_opt_ PULONG ResourceLength,
    _Out_opt_ PVOID* ResourceBuffer
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

#ifndef IMAGE_DEBUG_TYPE_PERFMAP
#define IMAGE_DEBUG_TYPE_PERFMAP 21
#endif

typedef struct _IMAGE_DEBUG_TYPE_PERFMAPV1
{
    ULONG Magic; // 0x4D523252
    BYTE Signature[16];
    ULONG Version;
    // BYTE Data[1];
} IMAGE_DEBUG_TYPE_PERFMAPV1, *PIMAGE_DEBUG_TYPE_PERFMAPV1;

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
    _Out_opt_ ULONG* DataLength,
    _Out_opt_ PVOID* DataBuffer
    );

// maplib

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
    PCSTR Name;
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
    PCSTR Name;
    PCSTR DllName;
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
    _In_opt_ PCWSTR FileName,
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

typedef struct _PH_IMAGE_RELOC_ENTRY
{
    ULONG BlockIndex;
    ULONG BlockRva;
    IMAGE_RELOCATION_RECORD Record;
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
    _In_opt_ PPH_MAPPED_IMAGE_RELOC Relocations
    );

typedef NTSTATUS (NTAPI *PPH_MAPPED_IMAGE_RELOC_CALLBACK)(
    _In_ PPH_MAPPED_IMAGE MappedImage,
    _In_ PIMAGE_DATA_DIRECTORY DataDirectory,
    _In_ PIMAGE_BASE_RELOCATION RelocationDirectory,
    _In_ PIMAGE_RELOCATION_RECORD Relocations,
    _In_ ULONG RelocationCount,
    _In_opt_ PVOID Context
    );

PHLIBAPI
NTSTATUS
NTAPI
PhMappedImageEnumerateRelocations(
    _In_ PPH_MAPPED_IMAGE MappedImage,
    _In_ PPH_MAPPED_IMAGE_RELOC_CALLBACK Callback,
    _In_opt_ PVOID Context
    );

typedef struct _PH_IMAGE_DYNAMIC_RELOC_ENTRY
{
    ULONGLONG Symbol;

    union
    {
        // IMAGE_DYNAMIC_RELOCATION_GUARD_IMPORT_CONTROL_TRANSFER
        struct
        {
            ULONG BlockIndex;
            ULONG BlockRva;
            IMAGE_IMPORT_CONTROL_TRANSFER_DYNAMIC_RELOCATION Record;
        } ImportControl;

        // IMAGE_DYNAMIC_RELOCATION_GUARD_INDIR_CONTROL_TRANSFER
        struct
        {
            ULONG BlockIndex;
            ULONG BlockRva;
            IMAGE_INDIR_CONTROL_TRANSFER_DYNAMIC_RELOCATION Record;
        } IndirControl;

        // IMAGE_DYNAMIC_RELOCATION_GUARD_SWITCHTABLE_BRANCH
        struct
        {
            ULONG BlockIndex;
            ULONG BlockRva;
            IMAGE_SWITCHTABLE_BRANCH_DYNAMIC_RELOCATION Record;
        } SwitchBranch;

        // IMAGE_DYNAMIC_RELOCATION_FUNCTION_OVERRIDE
        struct
        {
            ULONG BlockIndex;
            ULONG BlockRva;
            IMAGE_RELOCATION_RECORD Record;
            PIMAGE_BDD_DYNAMIC_RELOCATION BDDNodes;
            ULONG BDDNodesCount;
            ULONG OriginalRva;
            ULONG BDDOffset;
            PULONG Rvas;
            ULONG RvasCount;
        } FuncOverride;

        // IMAGE_DYNAMIC_RELOCATION_ARM64X
        struct
        {
            ULONG BlockIndex;
            ULONG BlockRva;
            union
            {
                LONG64 Delta;
                ULONG64 Value8;
                ULONG Value4;
                USHORT Value2;
            };
            union
            {
                IMAGE_DVRT_ARM64X_FIXUP_RECORD RecordFixup;
                IMAGE_DVRT_ARM64X_DELTA_FIXUP_RECORD RecordDelta;
            };
        } ARM64X;

        // IMAGE_DYNAMIC_RELOCATION_KI_USER_SHARED_DATA64 or similar
        struct
        {
            ULONG BlockIndex;
            ULONG BlockRva;
            IMAGE_RELOCATION_RECORD Record;
        } Other;
    };

    PVOID ImageBaseVa;
    PVOID MappedImageVa;
} PH_IMAGE_DYNAMIC_RELOC_ENTRY, *PPH_IMAGE_DYNAMIC_RELOC_ENTRY;

typedef struct _PH_MAPPED_IMAGE_DYNAMIC_RELOC
{
    PPH_MAPPED_IMAGE MappedImage;
    PIMAGE_DYNAMIC_RELOCATION_TABLE RelocationTable;

    ULONG NumberOfEntries;
    PPH_IMAGE_DYNAMIC_RELOC_ENTRY RelocationEntries;
} PH_MAPPED_IMAGE_DYNAMIC_RELOC, *PPH_MAPPED_IMAGE_DYNAMIC_RELOC;

PHLIBAPI
NTSTATUS
NTAPI
PhGetMappedImageDynamicRelocationsTable(
    _In_ PPH_MAPPED_IMAGE MappedImage,
    _Out_opt_ PIMAGE_DYNAMIC_RELOCATION_TABLE* Table
    );

PHLIBAPI
NTSTATUS
NTAPI
PhGetMappedImageDynamicRelocations(
    _In_ PPH_MAPPED_IMAGE MappedImage,
    _Out_ PPH_MAPPED_IMAGE_DYNAMIC_RELOC Relocations
    );

PHLIBAPI
VOID
NTAPI
PhFreeMappedImageDynamicRelocations(
    _In_opt_ PPH_MAPPED_IMAGE_DYNAMIC_RELOC Relocations
    );

typedef struct _PH_MAPPED_IMAGE_EXCEPTIONS
{
    PPH_MAPPED_IMAGE MappedImage;
    PIMAGE_DATA_DIRECTORY DataDirectory;
    IMAGE_DATA_DIRECTORY DataDirectoryARM64X;
    PVOID ExceptionDirectory;

    ULONG NumberOfEntries;
    PVOID ExceptionEntries;
} PH_MAPPED_IMAGE_EXCEPTIONS, *PPH_MAPPED_IMAGE_EXCEPTIONS;

#define PH_GET_IMAGE_EXCEPTIONS_ARM64X 0x00000001ul

PHLIBAPI
NTSTATUS
NTAPI
PhGetMappedImageExceptionsEx(
    _In_ PPH_MAPPED_IMAGE MappedImage,
    _Out_ PPH_MAPPED_IMAGE_EXCEPTIONS Exceptions,
    _In_ ULONG Flags
    );

PHLIBAPI
NTSTATUS
NTAPI
PhGetMappedImageExceptions(
    _In_ PPH_MAPPED_IMAGE MappedImage,
    _Out_ PPH_MAPPED_IMAGE_EXCEPTIONS Exceptions
    );

// Note: Remove if/when added to the Windows SDK. (dmex)
#ifndef _IMAGE_VOLATILE_METADATA
typedef struct _IMAGE_VOLATILE_METADATA
{
    ULONG Size;
    ULONG Version;
    ULONG VolatileAccessTable;
    ULONG VolatileAccessTableSize;
    ULONG VolatileInfoRangeTable;
    ULONG VolatileInfoRangeTableSize;
} IMAGE_VOLATILE_METADATA, *PIMAGE_VOLATILE_METADATA;
#endif

// Note: Remove if/when added to the Windows SDK. (dmex)
#ifndef _IMAGE_VOLATILE_RVA_METADATA
typedef struct _IMAGE_VOLATILE_RVA_METADATA
{
    ULONG Rva;
} IMAGE_VOLATILE_RVA_METADATA, *PIMAGE_VOLATILE_RVA_METADATA;
#endif

// Note: Remove if/when added to the Windows SDK. (dmex)
#ifndef _IMAGE_VOLATILE_RANGE_METADATA
typedef struct _IMAGE_VOLATILE_RANGE_METADATA
{
    ULONG Rva;
    ULONG Size;
} IMAGE_VOLATILE_RANGE_METADATA, *PIMAGE_VOLATILE_RANGE_METADATA;
#endif

typedef struct _PH_IMAGE_VOLATILE_ENTRY
{
    ULONG Rva;
    ULONG Size;
} PH_IMAGE_VOLATILE_ENTRY, *PPH_IMAGE_VOLATILE_ENTRY;

typedef struct _PH_MAPPED_IMAGE_VOLATILE_METADATA
{
    PPH_MAPPED_IMAGE MappedImage;
    PIMAGE_VOLATILE_METADATA VolatileMetadata;

    ULONG NumberOfAccessEntries;
    ULONG NumberOfRangeEntries;
    PPH_IMAGE_VOLATILE_ENTRY AccessEntries;
    PPH_IMAGE_VOLATILE_ENTRY RangeEntries;
} PH_MAPPED_IMAGE_VOLATILE_METADATA, *PPH_MAPPED_IMAGE_VOLATILE_METADATA;

PHLIBAPI
NTSTATUS
NTAPI
PhGetMappedImageVolatileMetadata(
    _In_ PPH_MAPPED_IMAGE MappedImage,
    _Out_ PPH_MAPPED_IMAGE_VOLATILE_METADATA VolatileMetadata
    );

PHLIBAPI
NTSTATUS
NTAPI
PhGetMappedImageAuthenticodeHash(
    _In_ PPH_MAPPED_IMAGE MappedImage,
    _In_ PH_HASH_ALGORITHM Algorithm,
    _Out_ PPH_STRING* AuthenticodeHash
    );

PHLIBAPI
NTSTATUS
NTAPI
PhGetMappedImageWdacHash(
    _In_ PPH_MAPPED_IMAGE MappedImage,
    _In_ PH_HASH_ALGORITHM Algorithm,
    _Out_ PPH_STRING* WdacHash
    );

PHLIBAPI
BOOLEAN
NTAPI
PhGetMappedImageEntropy(
    _In_ PPH_MAPPED_IMAGE MappedImage,
    _Out_ FLOAT* ImageEntropy,
    _Out_ FLOAT* ImageVariance
    );

PHLIBAPI
ULONG
NTAPI
PhGetMappedImageCHPEVersion(
    _In_ PPH_MAPPED_IMAGE MappedImage
    );

PHLIBAPI
NTSTATUS
NTAPI
PhGetRemoteMappedImageCHPEVersion(
    _In_ PPH_REMOTE_MAPPED_IMAGE RemoteMappedImage,
    _Out_ PULONG CHPEVersion
    );

PHLIBAPI
NTSTATUS
NTAPI
PhGetRemoteMappedImageCHPEVersionEx(
    _In_ PPH_REMOTE_MAPPED_IMAGE RemoteMappedImage,
    _In_ PPH_READ_VIRTUAL_MEMORY_CALLBACK ReadVirtualMemoryCallback,
    _Out_ PULONG CHPEVersion
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

EXTERN_C_END

#endif
