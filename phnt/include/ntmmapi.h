/*
 * Memory Manager Support functions
 *
 * This file is part of System Informer.
 */

#ifndef _NTMMAPI_H
#define _NTMMAPI_H

// Protection constants

#define PAGE_NOACCESS 0x01
#define PAGE_READONLY 0x02
#define PAGE_READWRITE 0x04
#define PAGE_WRITECOPY 0x08
#define PAGE_EXECUTE 0x10
#define PAGE_EXECUTE_READ 0x20
#define PAGE_EXECUTE_READWRITE 0x40
#define PAGE_EXECUTE_WRITECOPY 0x80
#define PAGE_GUARD 0x100
#define PAGE_NOCACHE 0x200
#define PAGE_WRITECOMBINE 0x400

#define PAGE_REVERT_TO_FILE_MAP     0x80000000
#define PAGE_ENCLAVE_THREAD_CONTROL 0x80000000
#define PAGE_TARGETS_NO_UPDATE      0x40000000
#define PAGE_TARGETS_INVALID        0x40000000
#define PAGE_ENCLAVE_UNVALIDATED    0x20000000
#define PAGE_ENCLAVE_NO_CHANGE      0x20000000
#define PAGE_ENCLAVE_MASK           0x10000000
#define PAGE_ENCLAVE_DECOMMIT       (PAGE_ENCLAVE_MASK | 0)
#define PAGE_ENCLAVE_SS_FIRST       (PAGE_ENCLAVE_MASK | 1)
#define PAGE_ENCLAVE_SS_REST        (PAGE_ENCLAVE_MASK | 2)

// Region and section constants

#define MEM_COMMIT 0x00001000
#define MEM_RESERVE 0x00002000
#define MEM_DECOMMIT 0x00004000
#define MEM_RELEASE 0x00008000
#define MEM_FREE 0x00010000
#define MEM_PRIVATE 0x00020000
#define MEM_MAPPED 0x00040000
#define MEM_RESET 0x00080000
#define MEM_TOP_DOWN 0x00100000
#define MEM_WRITE_WATCH 0x00200000
#define MEM_PHYSICAL 0x00400000
#define MEM_ROTATE 0x00800000
#define MEM_DIFFERENT_IMAGE_BASE_OK 0x00800000
#define MEM_RESET_UNDO 0x01000000
#define MEM_LARGE_PAGES 0x20000000
#define MEM_DOS_LIM 0x40000000
#define MEM_4MB_PAGES 0x80000000
#define MEM_64K_PAGES (MEM_LARGE_PAGES | MEM_PHYSICAL)

#define MEM_UNMAP_WITH_TRANSIENT_BOOST 0x00000001
#define MEM_COALESCE_PLACEHOLDERS 0x00000001
#define MEM_PRESERVE_PLACEHOLDER 0x00000002
#define MEM_REPLACE_PLACEHOLDER 0x00004000
#define MEM_RESERVE_PLACEHOLDER 0x00040000

#define SEC_HUGE_PAGES 0x00020000
#define SEC_PARTITION_OWNER_HANDLE 0x00040000
#define SEC_64K_PAGES 0x00080000
#define SEC_DRIVER_IMAGE 0x00100000 // rev
#define SEC_BASED 0x00200000
#define SEC_NO_CHANGE 0x00400000
#define SEC_FILE 0x00800000
#define SEC_IMAGE 0x01000000
#define SEC_PROTECTED_IMAGE 0x02000000
#define SEC_RESERVE 0x04000000
#define SEC_COMMIT 0x08000000
#define SEC_NOCACHE 0x10000000
#define SEC_GLOBAL 0x20000000
#define SEC_WRITECOMBINE 0x40000000
#define SEC_LARGE_PAGES 0x80000000
#define SEC_IMAGE_NO_EXECUTE (SEC_IMAGE | SEC_NOCACHE)
#if (PHNT_MODE == PHNT_MODE_KERNEL)
#define MEM_IMAGE SEC_IMAGE
#endif

#if (PHNT_MODE != PHNT_MODE_KERNEL)
typedef enum _MEMORY_INFORMATION_CLASS
{
    MemoryBasicInformation, // q: MEMORY_BASIC_INFORMATION
    MemoryWorkingSetInformation, // q: MEMORY_WORKING_SET_INFORMATION
    MemoryMappedFilenameInformation, // q: UNICODE_STRING
    MemoryRegionInformation, // q: MEMORY_REGION_INFORMATION
    MemoryWorkingSetExInformation, // q: MEMORY_WORKING_SET_EX_INFORMATION // since VISTA
    MemorySharedCommitInformation, // q: MEMORY_SHARED_COMMIT_INFORMATION // since WIN8
    MemoryImageInformation, // q: MEMORY_IMAGE_INFORMATION
    MemoryRegionInformationEx, // MEMORY_REGION_INFORMATION
    MemoryPrivilegedBasicInformation, // MEMORY_BASIC_INFORMATION
    MemoryEnclaveImageInformation, // MEMORY_ENCLAVE_IMAGE_INFORMATION // since REDSTONE3
    MemoryBasicInformationCapped, // 10
    MemoryPhysicalContiguityInformation, // MEMORY_PHYSICAL_CONTIGUITY_INFORMATION // since 20H1
    MemoryBadInformation, // since WIN11
    MemoryBadInformationAllProcesses, // since 22H1
    MemoryImageExtensionInformation, // MEMORY_IMAGE_EXTENSION_INFORMATION // since 24H2
    MaxMemoryInfoClass
} MEMORY_INFORMATION_CLASS;
#else
#define MemoryBasicInformation 0x0
#define MemoryWorkingSetInformation 0x1
#define MemoryMappedFilenameInformation 0x2
#define MemoryRegionInformation 0x3
#define MemoryWorkingSetExInformation 0x4
#define MemorySharedCommitInformation 0x5
#define MemoryImageInformation 0x6
#define MemoryRegionInformationEx 0x7
#define MemoryPrivilegedBasicInformation 0x8
#define MemoryEnclaveImageInformation 0x9
#define MemoryBasicInformationCapped 0xA
#define MemoryPhysicalContiguityInformation 0xB
#define MemoryBadInformation 0xC
#define MemoryBadInformationAllProcesses 0xD
#define MemoryImageExtensionInformation 0xE
#endif

// MEMORY_WORKING_SET_BLOCK->Protection
#define MEMORY_BLOCK_NOT_ACCESSED 0
#define MEMORY_BLOCK_READONLY 1
#define MEMORY_BLOCK_EXECUTABLE 2
#define MEMORY_BLOCK_EXECUTABLE_READONLY 3
#define MEMORY_BLOCK_READWRITE 4
#define MEMORY_BLOCK_COPYONWRITE 5
#define MEMORY_BLOCK_EXECUTABLE_READWRITE 6
#define MEMORY_BLOCK_EXECUTABLE_COPYONWRITE 7
#define MEMORY_BLOCK_NOT_ACCESSED_2 8
#define MEMORY_BLOCK_NON_CACHEABLE_READONLY 9
#define MEMORY_BLOCK_NON_CACHEABLE_EXECUTABLE 10
#define MEMORY_BLOCK_NON_CACHEABLE_EXECUTABLE_READONLY 11
#define MEMORY_BLOCK_NON_CACHEABLE_READWRITE 12
#define MEMORY_BLOCK_NON_CACHEABLE_COPYONWRITE 13
#define MEMORY_BLOCK_NON_CACHEABLE_EXECUTABLE_READWRITE 14
#define MEMORY_BLOCK_NON_CACHEABLE_EXECUTABLE_COPYONWRITE 15
#define MEMORY_BLOCK_NOT_ACCESSED_3 16
#define MEMORY_BLOCK_GUARD_READONLY 17
#define MEMORY_BLOCK_GUARD_EXECUTABLE 18
#define MEMORY_BLOCK_GUARD_EXECUTABLE_READONLY 19
#define MEMORY_BLOCK_GUARD_READWRITE 20
#define MEMORY_BLOCK_GUARD_COPYONWRITE 21
#define MEMORY_BLOCK_GUARD_EXECUTABLE_READWRITE 22
#define MEMORY_BLOCK_GUARD_EXECUTABLE_COPYONWRITE 23
#define MEMORY_BLOCK_NOT_ACCESSED_4 24
#define MEMORY_BLOCK_NON_CACHEABLE_GUARD_READONLY 25
#define MEMORY_BLOCK_NON_CACHEABLE_GUARD_EXECUTABLE 26
#define MEMORY_BLOCK_NON_CACHEABLE_GUARD_EXECUTABLE_READONLY 27
#define MEMORY_BLOCK_NON_CACHEABLE_GUARD_READWRITE 28
#define MEMORY_BLOCK_NON_CACHEABLE_GUARD_COPYONWRITE 29
#define MEMORY_BLOCK_NON_CACHEABLE_GUARD_EXECUTABLE_READWRITE 30
#define MEMORY_BLOCK_NON_CACHEABLE_GUARD_EXECUTABLE_COPYONWRITE 31

typedef struct _MEMORY_WORKING_SET_BLOCK
{
    ULONG_PTR Protection : 5;
    ULONG_PTR ShareCount : 3;
    ULONG_PTR Shared : 1;
    ULONG_PTR Node : 3;
#ifdef _WIN64
    ULONG_PTR VirtualPage : 52;
#else
    ULONG VirtualPage : 20;
#endif
} MEMORY_WORKING_SET_BLOCK, *PMEMORY_WORKING_SET_BLOCK;

typedef struct _MEMORY_WORKING_SET_INFORMATION
{
    ULONG_PTR NumberOfEntries;
    _Field_size_(NumberOfEntries) MEMORY_WORKING_SET_BLOCK WorkingSetInfo[1];
} MEMORY_WORKING_SET_INFORMATION, *PMEMORY_WORKING_SET_INFORMATION;

// private
typedef struct _MEMORY_REGION_INFORMATION
{
    PVOID AllocationBase;
    ULONG AllocationProtect;
    union
    {
        ULONG RegionType;
        struct
        {
            ULONG Private : 1;
            ULONG MappedDataFile : 1;
            ULONG MappedImage : 1;
            ULONG MappedPageFile : 1;
            ULONG MappedPhysical : 1;
            ULONG DirectMapped : 1;
            ULONG SoftwareEnclave : 1; // REDSTONE3
            ULONG PageSize64K : 1;
            ULONG PlaceholderReservation : 1; // REDSTONE4
            ULONG MappedAwe : 1; // 21H1
            ULONG MappedWriteWatch : 1;
            ULONG PageSizeLarge : 1;
            ULONG PageSizeHuge : 1;
            ULONG Reserved : 19;
        };
    };
    SIZE_T RegionSize;
    SIZE_T CommitSize;
    ULONG_PTR PartitionId; // 19H1
    ULONG_PTR NodePreference; // 20H1
} MEMORY_REGION_INFORMATION, *PMEMORY_REGION_INFORMATION;

// private
typedef enum _MEMORY_WORKING_SET_EX_LOCATION
{
    MemoryLocationInvalid,
    MemoryLocationResident,
    MemoryLocationPagefile,
    MemoryLocationReserved
} MEMORY_WORKING_SET_EX_LOCATION;

// private
typedef struct _MEMORY_WORKING_SET_EX_BLOCK
{
    union
    {
        struct
        {
            ULONG_PTR Valid : 1;
            ULONG_PTR ShareCount : 3;
            ULONG_PTR Win32Protection : 11;
            ULONG_PTR Shared : 1;
            ULONG_PTR Node : 6;
            ULONG_PTR Locked : 1;
            ULONG_PTR LargePage : 1;
            ULONG_PTR Priority : 3;
            ULONG_PTR Reserved : 3;
            ULONG_PTR SharedOriginal : 1;
            ULONG_PTR Bad : 1;
            ULONG_PTR Win32GraphicsProtection : 4; // 19H1
#ifdef _WIN64
            ULONG_PTR ReservedUlong : 28;
#endif
        };
        struct
        {
            ULONG_PTR Valid : 1;
            ULONG_PTR Reserved0 : 14;
            ULONG_PTR Shared : 1;
            ULONG_PTR Reserved1 : 5;
            ULONG_PTR PageTable : 1;
            ULONG_PTR Location : 2;
            ULONG_PTR Priority : 3;
            ULONG_PTR ModifiedList : 1;
            ULONG_PTR Reserved2 : 2;
            ULONG_PTR SharedOriginal : 1;
            ULONG_PTR Bad : 1;
#ifdef _WIN64
            ULONG_PTR ReservedUlong : 32;
#endif
        } Invalid;
    };
} MEMORY_WORKING_SET_EX_BLOCK, *PMEMORY_WORKING_SET_EX_BLOCK;

// private
typedef struct _MEMORY_WORKING_SET_EX_INFORMATION
{
    PVOID VirtualAddress;
    union
    {
        MEMORY_WORKING_SET_EX_BLOCK VirtualAttributes;
        ULONG_PTR Long;
    } u1;
} MEMORY_WORKING_SET_EX_INFORMATION, *PMEMORY_WORKING_SET_EX_INFORMATION;

// private
typedef struct _MEMORY_SHARED_COMMIT_INFORMATION
{
    SIZE_T CommitSize;
} MEMORY_SHARED_COMMIT_INFORMATION, *PMEMORY_SHARED_COMMIT_INFORMATION;

// private
typedef struct _MEMORY_IMAGE_INFORMATION
{
    PVOID ImageBase;
    SIZE_T SizeOfImage;
    union
    {
        ULONG ImageFlags;
        struct
        {
            ULONG ImagePartialMap : 1;
            ULONG ImageNotExecutable : 1;
            ULONG ImageSigningLevel : 4; // REDSTONE3
            ULONG ImageExtensionPresent : 1; // since 24H2
            ULONG Reserved : 25;
        };
    };
} MEMORY_IMAGE_INFORMATION, *PMEMORY_IMAGE_INFORMATION;

// private
typedef struct _MEMORY_ENCLAVE_IMAGE_INFORMATION
{
    MEMORY_IMAGE_INFORMATION ImageInfo;
    UCHAR UniqueID[32];
    UCHAR AuthorID[32];
} MEMORY_ENCLAVE_IMAGE_INFORMATION, *PMEMORY_ENCLAVE_IMAGE_INFORMATION;

// private
typedef enum _MEMORY_PHYSICAL_CONTIGUITY_UNIT_STATE
{
    MemoryNotContiguous,
    MemoryAlignedAndContiguous,
    MemoryNotResident,
    MemoryNotEligibleToMakeContiguous,
    MemoryContiguityStateMax,
} MEMORY_PHYSICAL_CONTIGUITY_UNIT_STATE;

// private
typedef struct _MEMORY_PHYSICAL_CONTIGUITY_UNIT_INFORMATION
{
    union
    {
        struct
        {
            ULONG State : 2;
            ULONG Reserved : 30;
        };
        ULONG AllInformation;
    };
} MEMORY_PHYSICAL_CONTIGUITY_UNIT_INFORMATION, *PMEMORY_PHYSICAL_CONTIGUITY_UNIT_INFORMATION;

// private
typedef struct _MEMORY_PHYSICAL_CONTIGUITY_INFORMATION
{
    PVOID VirtualAddress;
    ULONG_PTR Size;
    ULONG_PTR ContiguityUnitSize;
    ULONG Flags;
    PMEMORY_PHYSICAL_CONTIGUITY_UNIT_INFORMATION ContiguityUnitInformation;
} MEMORY_PHYSICAL_CONTIGUITY_INFORMATION, *PMEMORY_PHYSICAL_CONTIGUITY_INFORMATION;

// private
typedef struct _RTL_SCP_CFG_ARM64_HEADER
{
    ULONG EcInvalidCallHandlerRva;
    ULONG EcCfgCheckRva;
    ULONG EcCfgCheckESRva;
    ULONG EcCallCheckRva;
    ULONG CpuInitializationCompleteLoadRva;
    ULONG LdrpValidateEcCallTargetInitRva;
    ULONG SyscallFfsSizeRva;
    ULONG SyscallFfsBaseRva;
} RTL_SCP_CFG_ARM64_HEADER, *PRTL_SCP_CFG_ARM64_HEADER;

// private
typedef enum _RTL_SCP_CFG_PAGE_TYPE
{
    RtlScpCfgPageTypeNop,
    RtlScpCfgPageTypeDefault,
    RtlScpCfgPageTypeExportSuppression,
    RtlScpCfgPageTypeFptr,
    RtlScpCfgPageTypeMax,
    RtlScpCfgPageTypeNone
} RTL_SCP_CFG_PAGE_TYPE;

// private
typedef struct _RTL_SCP_CFG_COMMON_HEADER
{
    ULONG CfgDispatchRva;
    ULONG CfgDispatchESRva;
    ULONG CfgCheckRva;
    ULONG CfgCheckESRva;
    ULONG InvalidCallHandlerRva;
    ULONG FnTableRva;
} RTL_SCP_CFG_COMMON_HEADER, *PRTL_SCP_CFG_COMMON_HEADER;

// private
typedef struct _RTL_SCP_CFG_HEADER
{
    RTL_SCP_CFG_COMMON_HEADER Common;
} RTL_SCP_CFG_HEADER, *PRTL_SCP_CFG_HEADER;

// private
typedef struct _RTL_SCP_CFG_REGION_BOUNDS
{
    PVOID StartAddress;
    PVOID EndAddress;
} RTL_SCP_CFG_REGION_BOUNDS, *PRTL_SCP_CFG_REGION_BOUNDS;

// private
typedef struct _RTL_SCP_CFG_NTDLL_EXPORTS
{
    RTL_SCP_CFG_REGION_BOUNDS ScpRegions[4];
    PVOID CfgDispatchFptr;
    PVOID CfgDispatchESFptr;
    PVOID CfgCheckFptr;
    PVOID CfgCheckESFptr;
    PVOID IllegalCallHandler;
} RTL_SCP_CFG_NTDLL_EXPORTS, *PRTL_SCP_CFG_NTDLL_EXPORTS;

// private
typedef struct _RTL_SCP_CFG_NTDLL_EXPORTS_ARM64EC
{
    PVOID EcInvalidCallHandler;
    PVOID EcCfgCheckFptr;
    PVOID EcCfgCheckESFptr;
    PVOID EcCallCheckFptr;
    PVOID CpuInitializationComplete;
    PVOID LdrpValidateEcCallTargetInit;
    struct
    {
        PVOID SyscallFfsSize;
        union
        {
            PVOID Ptr;
            ULONG Value;
        };
    };
    PVOID SyscallFfsBase;
} RTL_SCP_CFG_NTDLL_EXPORTS_ARM64EC, *PRTL_SCP_CFG_NTDLL_EXPORTS_ARM64EC;

// private
typedef struct _RTL_RETPOLINE_ROUTINES
{
    ULONG SwitchtableJump[16];
    ULONG CfgIndirectRax;
    ULONG NonCfgIndirectRax;
    ULONG ImportR10;
    ULONG JumpHpat;
} RTL_RETPOLINE_ROUTINES, *PRTL_RETPOLINE_ROUTINES;

// private
typedef struct _RTL_KSCP_ROUTINES
{
    ULONG UnwindDataOffset;
    RTL_RETPOLINE_ROUTINES RetpolineRoutines;
    ULONG CfgDispatchSmep;
    ULONG CfgDispatchNoSmep;
} RTL_KSCP_ROUTINES, *PRTL_KSCP_ROUTINES;

// private
typedef enum _MEMORY_IMAGE_EXTENSION_TYPE
{
    MemoryImageExtensionCfgScp,
    MemoryImageExtensionCfgEmulatedScp,
    MemoryImageExtensionTypeMax,
} MEMORY_IMAGE_EXTENSION_TYPE;

// private
typedef struct _MEMORY_IMAGE_EXTENSION_INFORMATION
{
    MEMORY_IMAGE_EXTENSION_TYPE ExtensionType;
    ULONG Flags;
    PVOID ExtensionImageBaseRva;
    SIZE_T ExtensionSize;
} MEMORY_IMAGE_EXTENSION_INFORMATION, *PMEMORY_IMAGE_EXTENSION_INFORMATION;

#define MMPFNLIST_ZERO 0
#define MMPFNLIST_FREE 1
#define MMPFNLIST_STANDBY 2
#define MMPFNLIST_MODIFIED 3
#define MMPFNLIST_MODIFIEDNOWRITE 4
#define MMPFNLIST_BAD 5
#define MMPFNLIST_ACTIVE 6
#define MMPFNLIST_TRANSITION 7

//typedef enum _MMLISTS
//{
//    ZeroedPageList = 0,
//    FreePageList = 1,
//    StandbyPageList = 2,
//    ModifiedPageList = 3,
//    ModifiedNoWritePageList = 4,
//    BadPageList = 5,
//    ActiveAndValid = 6,
//    TransitionPage = 7
//} MMLISTS;

#define MMPFNUSE_PROCESSPRIVATE 0
#define MMPFNUSE_FILE 1
#define MMPFNUSE_PAGEFILEMAPPED 2
#define MMPFNUSE_PAGETABLE 3
#define MMPFNUSE_PAGEDPOOL 4
#define MMPFNUSE_NONPAGEDPOOL 5
#define MMPFNUSE_SYSTEMPTE 6
#define MMPFNUSE_SESSIONPRIVATE 7
#define MMPFNUSE_METAFILE 8
#define MMPFNUSE_AWEPAGE 9
#define MMPFNUSE_DRIVERLOCKPAGE 10
#define MMPFNUSE_KERNELSTACK 11

//typedef enum _MMPFNUSE
//{
//    ProcessPrivatePage,
//    MemoryMappedFilePage,
//    PageFileMappedPage,
//    PageTablePage,
//    PagedPoolPage,
//    NonPagedPoolPage,
//    SystemPTEPage,
//    SessionPrivatePage,
//    MetafilePage,
//    AWEPage,
//    DriverLockedPage,
//    KernelStackPage
//} MMPFNUSE;

// private
typedef struct _MEMORY_FRAME_INFORMATION
{
    ULONGLONG UseDescription : 4; // MMPFNUSE_*
    ULONGLONG ListDescription : 3; // MMPFNLIST_*
    ULONGLONG Cold : 1; // 19H1
    ULONGLONG Pinned : 1; // 1 - pinned, 0 - not pinned
    ULONGLONG DontUse : 48; // *_INFORMATION overlay
    ULONGLONG Priority : 3;
    ULONGLONG NonTradeable : 1;
    ULONGLONG Reserved : 3;
} MEMORY_FRAME_INFORMATION;

// private
typedef struct _FILEOFFSET_INFORMATION
{
    ULONGLONG DontUse : 9; // MEMORY_FRAME_INFORMATION overlay
    ULONGLONG Offset : 48; // mapped files
    ULONGLONG Reserved : 7;
} FILEOFFSET_INFORMATION;

// private
typedef struct _PAGEDIR_INFORMATION
{
    ULONGLONG DontUse : 9; // MEMORY_FRAME_INFORMATION overlay
    ULONGLONG PageDirectoryBase : 48; // private pages
    ULONGLONG Reserved : 7;
} PAGEDIR_INFORMATION;

// private
typedef struct _UNIQUE_PROCESS_INFORMATION
{
    ULONGLONG DontUse : 9; // MEMORY_FRAME_INFORMATION overlay
    ULONGLONG UniqueProcessKey : 48; // ProcessId
    ULONGLONG Reserved  : 7;
} UNIQUE_PROCESS_INFORMATION, *PUNIQUE_PROCESS_INFORMATION;

// private
typedef struct _MMPFN_IDENTITY
{
    union
    {
        MEMORY_FRAME_INFORMATION e1; // all
        FILEOFFSET_INFORMATION e2; // mapped files
        PAGEDIR_INFORMATION e3; // private pages
        UNIQUE_PROCESS_INFORMATION e4; // owning process
    } u1;
    ULONG_PTR PageFrameIndex; // all
    union
    {
        struct
        {
            ULONG_PTR Image : 1;
            ULONG_PTR Mismatch : 1;
        } e1;
        struct
        {
            ULONG_PTR CombinedPage;
        } e2;
        ULONG_PTR FileObject; // mapped files
        ULONG_PTR UniqueFileObjectKey;
        ULONG_PTR ProtoPteAddress;
        ULONG_PTR VirtualAddress;  // everything else
    } u2;
} MMPFN_IDENTITY, *PMMPFN_IDENTITY;

typedef struct _MMPFN_MEMSNAP_INFORMATION
{
    ULONG_PTR InitialPageFrameIndex;
    ULONG_PTR Count;
} MMPFN_MEMSNAP_INFORMATION, *PMMPFN_MEMSNAP_INFORMATION;

typedef enum _SECTION_INFORMATION_CLASS
{
    SectionBasicInformation, // q; SECTION_BASIC_INFORMATION
    SectionImageInformation, // q; SECTION_IMAGE_INFORMATION
    SectionRelocationInformation, // q; ULONG_PTR RelocationDelta // name:wow64:whNtQuerySection_SectionRelocationInformation // since WIN7
    SectionOriginalBaseInformation, // q; PVOID BaseAddress // since REDSTONE
    SectionInternalImageInformation, // SECTION_INTERNAL_IMAGE_INFORMATION // since REDSTONE2
    MaxSectionInfoClass
} SECTION_INFORMATION_CLASS;

typedef struct _SECTION_BASIC_INFORMATION
{
    PVOID BaseAddress;
    ULONG AllocationAttributes;
    LARGE_INTEGER MaximumSize;
} SECTION_BASIC_INFORMATION, *PSECTION_BASIC_INFORMATION;

// symbols
typedef struct _SECTION_IMAGE_INFORMATION
{
    PVOID TransferAddress;
    ULONG ZeroBits;
    SIZE_T MaximumStackSize;
    SIZE_T CommittedStackSize;
    ULONG SubSystemType;
    union
    {
        struct
        {
            USHORT SubSystemMinorVersion;
            USHORT SubSystemMajorVersion;
        };
        ULONG SubSystemVersion;
    };
    union
    {
        struct
        {
            USHORT MajorOperatingSystemVersion;
            USHORT MinorOperatingSystemVersion;
        };
        ULONG OperatingSystemVersion;
    };
    USHORT ImageCharacteristics;
    USHORT DllCharacteristics;
    USHORT Machine;
    BOOLEAN ImageContainsCode;
    union
    {
        UCHAR ImageFlags;
        struct
        {
            UCHAR ComPlusNativeReady : 1;
            UCHAR ComPlusILOnly : 1;
            UCHAR ImageDynamicallyRelocated : 1;
            UCHAR ImageMappedFlat : 1;
            UCHAR BaseBelow4gb : 1;
            UCHAR ComPlusPrefer32bit : 1;
            UCHAR Reserved : 2;
        };
    };
    ULONG LoaderFlags;
    ULONG ImageFileSize;
    ULONG CheckSum;
} SECTION_IMAGE_INFORMATION, *PSECTION_IMAGE_INFORMATION;

// symbols
typedef struct _SECTION_INTERNAL_IMAGE_INFORMATION
{
    SECTION_IMAGE_INFORMATION SectionInformation;
    union
    {
        ULONG ExtendedFlags;
        struct
        {
            ULONG ImageExportSuppressionEnabled : 1;
            ULONG ImageCetShadowStacksReady : 1; // 20H1
            ULONG ImageXfgEnabled : 1; // 20H2
            ULONG ImageCetShadowStacksStrictMode : 1;
            ULONG ImageCetSetContextIpValidationRelaxedMode : 1;
            ULONG ImageCetDynamicApisAllowInProc : 1;
            ULONG ImageCetDowngradeReserved1 : 1;
            ULONG ImageCetDowngradeReserved2 : 1;
            ULONG ImageExportSuppressionInfoPresent : 1;
            ULONG ImageCfgEnabled : 1;
            ULONG Reserved : 22;
        };
    };
} SECTION_INTERNAL_IMAGE_INFORMATION, *PSECTION_INTERNAL_IMAGE_INFORMATION;

#if (PHNT_MODE != PHNT_MODE_KERNEL)
typedef enum _SECTION_INHERIT
{
    ViewShare = 1,
    ViewUnmap = 2
} SECTION_INHERIT;
#endif

#define MEM_EXECUTE_OPTION_ENABLE 0x1
#define MEM_EXECUTE_OPTION_DISABLE 0x2
#define MEM_EXECUTE_OPTION_DISABLE_THUNK_EMULATION 0x4
#define MEM_EXECUTE_OPTION_PERMANENT 0x8
#define MEM_EXECUTE_OPTION_EXECUTE_DISPATCH_ENABLE 0x10
#define MEM_EXECUTE_OPTION_IMAGE_DISPATCH_ENABLE 0x20
#define MEM_EXECUTE_OPTION_DISABLE_EXCEPTION_CHAIN_VALIDATION 0x40
#define MEM_EXECUTE_OPTION_VALID_FLAGS 0x7f

// Virtual memory

#if (PHNT_MODE != PHNT_MODE_KERNEL)

_Must_inspect_result_
_When_(return == 0, __drv_allocatesMem(mem))
NTSYSCALLAPI
NTSTATUS
NTAPI
NtAllocateVirtualMemory(
    _In_ HANDLE ProcessHandle,
    _Inout_ _At_(*BaseAddress, _Readable_bytes_(*RegionSize) _Writable_bytes_(*RegionSize) _Post_readable_byte_size_(*RegionSize)) PVOID *BaseAddress,
    _In_ ULONG_PTR ZeroBits,
    _Inout_ PSIZE_T RegionSize,
    _In_ ULONG AllocationType,
    _In_ ULONG Protect
    );

#if (PHNT_VERSION >= PHNT_REDSTONE5)

_Must_inspect_result_
_When_(return == 0, __drv_allocatesMem(mem))
NTSYSCALLAPI
NTSTATUS
NTAPI
NtAllocateVirtualMemoryEx(
    _In_ HANDLE ProcessHandle,
    _Inout_ _At_(*BaseAddress, _Readable_bytes_(*RegionSize) _Writable_bytes_(*RegionSize) _Post_readable_byte_size_(*RegionSize)) PVOID *BaseAddress,
    _Inout_ PSIZE_T RegionSize,
    _In_ ULONG AllocationType,
    _In_ ULONG PageProtection,
    _Inout_updates_opt_(ExtendedParameterCount) PMEM_EXTENDED_PARAMETER ExtendedParameters,
    _In_ ULONG ExtendedParameterCount
    );

#endif

NTSYSCALLAPI
NTSTATUS
NTAPI
NtFreeVirtualMemory(
    _In_ HANDLE ProcessHandle,
    _Inout_ PVOID *BaseAddress,
    _Inout_ PSIZE_T RegionSize,
    _In_ ULONG FreeType
    );

NTSYSCALLAPI
NTSTATUS
NTAPI
NtReadVirtualMemory(
    _In_ HANDLE ProcessHandle,
    _In_opt_ PVOID BaseAddress,
    _Out_writes_bytes_to_(NumberOfBytesToRead, *NumberOfBytesRead) PVOID Buffer,
    _In_ SIZE_T NumberOfBytesToRead,
    _Out_opt_ PSIZE_T NumberOfBytesRead
    );

// rev
NTSYSCALLAPI
NTSTATUS
NTAPI
NtWow64ReadVirtualMemory64(
    _In_ HANDLE ProcessHandle,
    _In_opt_ ULONGLONG BaseAddress,
    _Out_writes_bytes_to_(NumberOfBytesToRead, *NumberOfBytesRead) PVOID Buffer,
    _In_ ULONGLONG NumberOfBytesToRead,
    _Out_opt_ PULONGLONG NumberOfBytesRead
    );

#if (PHNT_VERSION >= PHNT_WIN11)
// rev
NTSYSCALLAPI
NTSTATUS
NTAPI
NtReadVirtualMemoryEx(
    _In_ HANDLE ProcessHandle,
    _In_opt_ PVOID BaseAddress,
    _Out_writes_bytes_to_(NumberOfBytesToRead, *NumberOfBytesRead) PVOID Buffer,
    _In_ SIZE_T NumberOfBytesToRead,
    _Out_opt_ PSIZE_T NumberOfBytesRead,
    _In_ ULONG Flags
    );
#endif

NTSYSCALLAPI
NTSTATUS
NTAPI
NtWriteVirtualMemory(
    _In_ HANDLE ProcessHandle,
    _In_opt_ PVOID BaseAddress,
    _In_reads_bytes_(NumberOfBytesToWrite) PVOID Buffer,
    _In_ SIZE_T NumberOfBytesToWrite,
    _Out_opt_ PSIZE_T NumberOfBytesWritten
    );

// rev
NTSYSCALLAPI
NTSTATUS
NTAPI
NtWow64WriteVirtualMemory64(
    _In_ HANDLE ProcessHandle,
    _In_opt_ ULONGLONG BaseAddress,
    _In_reads_bytes_(NumberOfBytesToWrite) PVOID Buffer,
    _In_ ULONGLONG NumberOfBytesToWrite,
    _Out_opt_ PULONGLONG NumberOfBytesWritten
    );

NTSYSCALLAPI
NTSTATUS
NTAPI
NtProtectVirtualMemory(
    _In_ HANDLE ProcessHandle,
    _Inout_ PVOID *BaseAddress,
    _Inout_ PSIZE_T RegionSize,
    _In_ ULONG NewProtect,
    _Out_ PULONG OldProtect
    );

NTSYSCALLAPI
NTSTATUS
NTAPI
NtQueryVirtualMemory(
    _In_ HANDLE ProcessHandle,
    _In_opt_ PVOID BaseAddress,
    _In_ MEMORY_INFORMATION_CLASS MemoryInformationClass,
    _Out_writes_bytes_(MemoryInformationLength) PVOID MemoryInformation,
    _In_ SIZE_T MemoryInformationLength,
    _Out_opt_ PSIZE_T ReturnLength
    );

// rev
NTSYSCALLAPI
NTSTATUS
NTAPI
NtWow64QueryVirtualMemory64(
    _In_ HANDLE ProcessHandle,
    _In_opt_ ULONGLONG BaseAddress,
    _In_ MEMORY_INFORMATION_CLASS MemoryInformationClass,
    _Out_writes_bytes_(MemoryInformationLength) PVOID MemoryInformation,
    _In_ ULONGLONG MemoryInformationLength,
    _Out_opt_ PULONGLONG ReturnLength
    );

typedef struct _IO_STATUS_BLOCK* PIO_STATUS_BLOCK;

NTSYSCALLAPI
NTSTATUS
NTAPI
NtFlushVirtualMemory(
    _In_ HANDLE ProcessHandle,
    _Inout_ PVOID *BaseAddress,
    _Inout_ PSIZE_T RegionSize,
    _Out_ PIO_STATUS_BLOCK IoStatus
    );

#endif

// begin_private
#if (PHNT_MODE != PHNT_MODE_KERNEL)
typedef enum _VIRTUAL_MEMORY_INFORMATION_CLASS
{
    VmPrefetchInformation, // MEMORY_PREFETCH_INFORMATION
    VmPagePriorityInformation, // OFFER_PRIORITY
    VmCfgCallTargetInformation, // CFG_CALL_TARGET_LIST_INFORMATION // REDSTONE2
    VmPageDirtyStateInformation, // REDSTONE3
    VmImageHotPatchInformation, // 19H1
    VmPhysicalContiguityInformation, // 20H1
    VmVirtualMachinePrepopulateInformation,
    VmRemoveFromWorkingSetInformation,
    MaxVmInfoClass
} VIRTUAL_MEMORY_INFORMATION_CLASS;
#else
#define VmPrefetchInformation 0x0
#define VmPagePriorityInformation 0x1
#define VmCfgCallTargetInformation 0x2
#define VmPageDirtyStateInformation 0x3
#define VmImageHotPatchInformation 0x4
#define VmPhysicalContiguityInformation 0x5
#define VmVirtualMachinePrepopulateInformation 0x6
#define VmRemoveFromWorkingSetInformation 0x7
#define MaxVmInfoClass 0x8
#endif

#if (PHNT_MODE != PHNT_MODE_KERNEL)
typedef struct _MEMORY_RANGE_ENTRY
{
    PVOID VirtualAddress;
    SIZE_T NumberOfBytes;
} MEMORY_RANGE_ENTRY, *PMEMORY_RANGE_ENTRY;

#define VM_PREFETCH_TO_WORKING_SET 0x1 // since 24H4

typedef struct _MEMORY_PREFETCH_INFORMATION
{
    ULONG Flags;
} MEMORY_PREFETCH_INFORMATION, *PMEMORY_PREFETCH_INFORMATION;

typedef struct _CFG_CALL_TARGET_LIST_INFORMATION
{
    ULONG NumberOfEntries;
    ULONG Reserved;
    PULONG NumberOfEntriesProcessed;
    PCFG_CALL_TARGET_INFO CallTargetInfo;
    PVOID Section; // since REDSTONE5
    ULONGLONG FileOffset;
} CFG_CALL_TARGET_LIST_INFORMATION, *PCFG_CALL_TARGET_LIST_INFORMATION;
#endif
// end_private

#if (PHNT_MODE != PHNT_MODE_KERNEL)

#if (PHNT_VERSION >= PHNT_WIN8)

NTSYSCALLAPI
NTSTATUS
NTAPI
NtSetInformationVirtualMemory(
    _In_ HANDLE ProcessHandle,
    _In_ VIRTUAL_MEMORY_INFORMATION_CLASS VmInformationClass,
    _In_ SIZE_T NumberOfEntries,
    _In_reads_(NumberOfEntries) PMEMORY_RANGE_ENTRY VirtualAddresses,
    _In_reads_bytes_(VmInformationLength) PVOID VmInformation,
    _In_ ULONG VmInformationLength
    );

#endif

#define MAP_PROCESS 1
#define MAP_SYSTEM 2

NTSYSCALLAPI
NTSTATUS
NTAPI
NtLockVirtualMemory(
    _In_ HANDLE ProcessHandle,
    _Inout_ PVOID *BaseAddress,
    _Inout_ PSIZE_T RegionSize,
    _In_ ULONG MapType
    );

NTSYSCALLAPI
NTSTATUS
NTAPI
NtUnlockVirtualMemory(
    _In_ HANDLE ProcessHandle,
    _Inout_ PVOID *BaseAddress,
    _Inout_ PSIZE_T RegionSize,
    _In_ ULONG MapType
    );

#endif

// Sections

#if (PHNT_MODE != PHNT_MODE_KERNEL)

NTSYSCALLAPI
NTSTATUS
NTAPI
NtCreateSection(
    _Out_ PHANDLE SectionHandle,
    _In_ ACCESS_MASK DesiredAccess,
    _In_opt_ PCOBJECT_ATTRIBUTES ObjectAttributes,
    _In_opt_ PLARGE_INTEGER MaximumSize,
    _In_ ULONG SectionPageProtection,
    _In_ ULONG AllocationAttributes,
    _In_opt_ HANDLE FileHandle
    );

#if (PHNT_VERSION >= PHNT_REDSTONE5)
NTSYSCALLAPI
NTSTATUS
NTAPI
NtCreateSectionEx(
    _Out_ PHANDLE SectionHandle,
    _In_ ACCESS_MASK DesiredAccess,
    _In_opt_ PCOBJECT_ATTRIBUTES ObjectAttributes,
    _In_opt_ PLARGE_INTEGER MaximumSize,
    _In_ ULONG SectionPageProtection,
    _In_ ULONG AllocationAttributes,
    _In_opt_ HANDLE FileHandle,
    _Inout_updates_opt_(ExtendedParameterCount) PMEM_EXTENDED_PARAMETER ExtendedParameters,
    _In_ ULONG ExtendedParameterCount
    );
#endif

NTSYSCALLAPI
NTSTATUS
NTAPI
NtOpenSection(
    _Out_ PHANDLE SectionHandle,
    _In_ ACCESS_MASK DesiredAccess,
    _In_ PCOBJECT_ATTRIBUTES ObjectAttributes
    );

NTSYSCALLAPI
NTSTATUS
NTAPI
NtMapViewOfSection(
    _In_ HANDLE SectionHandle,
    _In_ HANDLE ProcessHandle,
    _Inout_ _At_(*BaseAddress, _Readable_bytes_(*ViewSize) _Writable_bytes_(*ViewSize) _Post_readable_byte_size_(*ViewSize)) PVOID *BaseAddress,
    _In_ ULONG_PTR ZeroBits,
    _In_ SIZE_T CommitSize,
    _Inout_opt_ PLARGE_INTEGER SectionOffset,
    _Inout_ PSIZE_T ViewSize,
    _In_ SECTION_INHERIT InheritDisposition,
    _In_ ULONG AllocationType,
    _In_ ULONG Win32Protect
    );

#if (PHNT_VERSION >= PHNT_REDSTONE5)
NTSYSCALLAPI
NTSTATUS
NTAPI
NtMapViewOfSectionEx(
    _In_ HANDLE SectionHandle,
    _In_ HANDLE ProcessHandle,
    _Inout_ _At_(*BaseAddress, _Readable_bytes_(*ViewSize) _Writable_bytes_(*ViewSize) _Post_readable_byte_size_(*ViewSize)) PVOID *BaseAddress,
    _Inout_opt_ PLARGE_INTEGER SectionOffset,
    _Inout_ PSIZE_T ViewSize,
    _In_ ULONG AllocationType,
    _In_ ULONG PageProtection,
    _Inout_updates_opt_(ExtendedParameterCount) PMEM_EXTENDED_PARAMETER ExtendedParameters,
    _In_ ULONG ExtendedParameterCount
    );
#endif

NTSYSCALLAPI
NTSTATUS
NTAPI
NtUnmapViewOfSection(
    _In_ HANDLE ProcessHandle,
    _In_opt_ PVOID BaseAddress
    );

#if (PHNT_VERSION >= PHNT_WIN8)
NTSYSCALLAPI
NTSTATUS
NTAPI
NtUnmapViewOfSectionEx(
    _In_ HANDLE ProcessHandle,
    _In_opt_ PVOID BaseAddress,
    _In_ ULONG Flags
    );
#endif

NTSYSCALLAPI
NTSTATUS
NTAPI
NtExtendSection(
    _In_ HANDLE SectionHandle,
    _Inout_ PLARGE_INTEGER NewSectionSize
    );

NTSYSCALLAPI
NTSTATUS
NTAPI
NtQuerySection(
    _In_ HANDLE SectionHandle,
    _In_ SECTION_INFORMATION_CLASS SectionInformationClass,
    _Out_writes_bytes_(SectionInformationLength) PVOID SectionInformation,
    _In_ SIZE_T SectionInformationLength,
    _Out_opt_ PSIZE_T ReturnLength
    );

NTSYSCALLAPI
NTSTATUS
NTAPI
NtAreMappedFilesTheSame(
    _In_ PVOID File1MappedAsAnImage,
    _In_ PVOID File2MappedAsFile
    );

#endif

// Partitions

#ifndef MEMORY_PARTITION_QUERY_ACCESS
#define MEMORY_PARTITION_QUERY_ACCESS 0x0001
#define MEMORY_PARTITION_MODIFY_ACCESS 0x0002
#define MEMORY_PARTITION_ALL_ACCESS \
    (STANDARD_RIGHTS_REQUIRED | SYNCHRONIZE | \
     MEMORY_PARTITION_QUERY_ACCESS | MEMORY_PARTITION_MODIFY_ACCESS)
#endif

#if (PHNT_MODE != PHNT_MODE_KERNEL)
// private
typedef enum _PARTITION_INFORMATION_CLASS
{
    SystemMemoryPartitionInformation, // q: MEMORY_PARTITION_CONFIGURATION_INFORMATION
    SystemMemoryPartitionMoveMemory, // s: MEMORY_PARTITION_TRANSFER_INFORMATION
    SystemMemoryPartitionAddPagefile, // s: MEMORY_PARTITION_PAGEFILE_INFORMATION
    SystemMemoryPartitionCombineMemory, // q; s: MEMORY_PARTITION_PAGE_COMBINE_INFORMATION
    SystemMemoryPartitionInitialAddMemory, // q; s: MEMORY_PARTITION_INITIAL_ADD_INFORMATION
    SystemMemoryPartitionGetMemoryEvents, // MEMORY_PARTITION_MEMORY_EVENTS_INFORMATION // since REDSTONE2
    SystemMemoryPartitionSetAttributes,
    SystemMemoryPartitionNodeInformation,
    SystemMemoryPartitionCreateLargePages,
    SystemMemoryPartitionDedicatedMemoryInformation,
    SystemMemoryPartitionOpenDedicatedMemory, // 10
    SystemMemoryPartitionMemoryChargeAttributes,
    SystemMemoryPartitionClearAttributes,
    SystemMemoryPartitionSetMemoryThresholds, // since WIN11
    SystemMemoryPartitionMemoryListCommand, // since 24H2
    SystemMemoryPartitionMax
} PARTITION_INFORMATION_CLASS, *PPARTITION_INFORMATION_CLASS;
#else
#define SystemMemoryPartitionInformation 0x0
#define SystemMemoryPartitionMoveMemory 0x1
#define SystemMemoryPartitionAddPagefile 0x2
#define SystemMemoryPartitionCombineMemory 0x3
#define SystemMemoryPartitionInitialAddMemory 0x4
#define SystemMemoryPartitionGetMemoryEvents 0x5
#define SystemMemoryPartitionSetAttributes 0x6
#define SystemMemoryPartitionNodeInformation 0x7
#define SystemMemoryPartitionCreateLargePages 0x8
#define SystemMemoryPartitionDedicatedMemoryInformation 0x9
#define SystemMemoryPartitionOpenDedicatedMemory 0xA
#define SystemMemoryPartitionMemoryChargeAttributes 0xB
#define SystemMemoryPartitionClearAttributes 0xC
#define SystemMemoryPartitionSetMemoryThresholds 0xD
#define SystemMemoryPartitionMemoryListCommand 0xE
#define SystemMemoryPartitionMax 0xF
#endif

// private
typedef struct _MEMORY_PARTITION_CONFIGURATION_INFORMATION
{
    ULONG Flags;
    ULONG NumaNode;
    ULONG Channel;
    ULONG NumberOfNumaNodes;
    SIZE_T ResidentAvailablePages;
    SIZE_T CommittedPages;
    SIZE_T CommitLimit;
    SIZE_T PeakCommitment;
    SIZE_T TotalNumberOfPages;
    SIZE_T AvailablePages;
    SIZE_T ZeroPages;
    SIZE_T FreePages;
    SIZE_T StandbyPages;
    SIZE_T StandbyPageCountByPriority[8]; // since REDSTONE2
    SIZE_T RepurposedPagesByPriority[8];
    SIZE_T MaximumCommitLimit;
    SIZE_T Reserved; // DonatedPagesToPartitions
    ULONG PartitionId; // since REDSTONE3
} MEMORY_PARTITION_CONFIGURATION_INFORMATION, *PMEMORY_PARTITION_CONFIGURATION_INFORMATION;

// private
typedef struct _MEMORY_PARTITION_TRANSFER_INFORMATION
{
    SIZE_T NumberOfPages;
    ULONG NumaNode;
    ULONG Flags;
} MEMORY_PARTITION_TRANSFER_INFORMATION, *PMEMORY_PARTITION_TRANSFER_INFORMATION;

// private
typedef struct _MEMORY_PARTITION_PAGEFILE_INFORMATION
{
    UNICODE_STRING PageFileName;
    LARGE_INTEGER MinimumSize;
    LARGE_INTEGER MaximumSize;
    ULONG Flags;
} MEMORY_PARTITION_PAGEFILE_INFORMATION, *PMEMORY_PARTITION_PAGEFILE_INFORMATION;

// private
typedef struct _MEMORY_PARTITION_PAGE_COMBINE_INFORMATION
{
    HANDLE StopHandle;
    ULONG Flags;
    SIZE_T TotalNumberOfPages;
} MEMORY_PARTITION_PAGE_COMBINE_INFORMATION, *PMEMORY_PARTITION_PAGE_COMBINE_INFORMATION;

// private
typedef struct _MEMORY_PARTITION_PAGE_RANGE
{
    ULONG_PTR StartPage;
    ULONG_PTR NumberOfPages;
} MEMORY_PARTITION_PAGE_RANGE, *PMEMORY_PARTITION_PAGE_RANGE;

// private
typedef struct _MEMORY_PARTITION_INITIAL_ADD_INFORMATION
{
    ULONG Flags;
    ULONG NumberOfRanges;
    SIZE_T NumberOfPagesAdded;
    MEMORY_PARTITION_PAGE_RANGE PartitionRanges[1];
} MEMORY_PARTITION_INITIAL_ADD_INFORMATION, *PMEMORY_PARTITION_INITIAL_ADD_INFORMATION;

// private
typedef struct _MEMORY_PARTITION_MEMORY_EVENTS_INFORMATION
{
    union
    {
        struct
        {
            ULONG CommitEvents : 1;
            ULONG Spare : 31;
        };
        ULONG AllFlags;
    } Flags;

    ULONG HandleAttributes;
    ACCESS_MASK DesiredAccess;
    HANDLE LowCommitCondition; // \KernelObjects\LowCommitCondition
    HANDLE HighCommitCondition; // \KernelObjects\HighCommitCondition
    HANDLE MaximumCommitCondition; // \KernelObjects\MaximumCommitCondition
} MEMORY_PARTITION_MEMORY_EVENTS_INFORMATION, *PMEMORY_PARTITION_MEMORY_EVENTS_INFORMATION;

#if (PHNT_MODE != PHNT_MODE_KERNEL)

#if (PHNT_VERSION >= PHNT_THRESHOLD)

NTSYSCALLAPI
NTSTATUS
NTAPI
NtCreatePartition(
    _In_opt_ HANDLE ParentPartitionHandle,
    _Out_ PHANDLE PartitionHandle,
    _In_ ACCESS_MASK DesiredAccess,
    _In_opt_ PCOBJECT_ATTRIBUTES ObjectAttributes,
    _In_ ULONG PreferredNode
    );

NTSYSCALLAPI
NTSTATUS
NTAPI
NtOpenPartition(
    _Out_ PHANDLE PartitionHandle,
    _In_ ACCESS_MASK DesiredAccess,
    _In_ PCOBJECT_ATTRIBUTES ObjectAttributes
    );

NTSYSCALLAPI
NTSTATUS
NTAPI
NtManagePartition(
    _In_ HANDLE TargetHandle,
    _In_opt_ HANDLE SourceHandle,
    _In_ PARTITION_INFORMATION_CLASS PartitionInformationClass,
    _Inout_updates_bytes_(PartitionInformationLength) PVOID PartitionInformation,
    _In_ ULONG PartitionInformationLength
    );

#endif

#endif

// User physical pages

#if (PHNT_MODE != PHNT_MODE_KERNEL)

NTSYSCALLAPI
NTSTATUS
NTAPI
NtMapUserPhysicalPages(
    _In_ PVOID VirtualAddress,
    _In_ SIZE_T NumberOfPages,
    _In_reads_opt_(NumberOfPages) PULONG_PTR UserPfnArray
    );

NTSYSCALLAPI
NTSTATUS
NTAPI
NtMapUserPhysicalPagesScatter(
    _In_reads_(NumberOfPages) PVOID *VirtualAddresses,
    _In_ SIZE_T NumberOfPages,
    _In_reads_opt_(NumberOfPages) PULONG_PTR UserPfnArray
    );

NTSYSCALLAPI
NTSTATUS
NTAPI
NtAllocateUserPhysicalPages(
    _In_ HANDLE ProcessHandle,
    _Inout_ PSIZE_T NumberOfPages,
    _Out_writes_(*NumberOfPages) PULONG_PTR UserPfnArray
    );

#if (PHNT_VERSION >= PHNT_THRESHOLD)
NTSYSCALLAPI
NTSTATUS
NTAPI
NtAllocateUserPhysicalPagesEx(
    _In_ HANDLE ProcessHandle,
    _Inout_ PULONG_PTR NumberOfPages,
    _Out_writes_(*NumberOfPages) PULONG_PTR UserPfnArray,
    _Inout_updates_opt_(ExtendedParameterCount) PMEM_EXTENDED_PARAMETER ExtendedParameters,
    _In_ ULONG ExtendedParameterCount
    );
#endif

NTSYSCALLAPI
NTSTATUS
NTAPI
NtFreeUserPhysicalPages(
    _In_ HANDLE ProcessHandle,
    _Inout_ PULONG_PTR NumberOfPages,
    _In_reads_(*NumberOfPages) PULONG_PTR UserPfnArray
    );

#endif

// Misc.

#if (PHNT_MODE != PHNT_MODE_KERNEL)

NTSYSCALLAPI
NTSTATUS
NTAPI
NtGetWriteWatch(
    _In_ HANDLE ProcessHandle,
    _In_ ULONG Flags,
    _In_ PVOID BaseAddress,
    _In_ SIZE_T RegionSize,
    _Out_writes_(*EntriesInUserAddressArray) PVOID *UserAddressArray,
    _Inout_ PULONG_PTR EntriesInUserAddressArray,
    _Out_ PULONG Granularity
    );

NTSYSCALLAPI
NTSTATUS
NTAPI
NtResetWriteWatch(
    _In_ HANDLE ProcessHandle,
    _In_ PVOID BaseAddress,
    _In_ SIZE_T RegionSize
    );

NTSYSCALLAPI
NTSTATUS
NTAPI
NtCreatePagingFile(
    _In_ PUNICODE_STRING PageFileName,
    _In_ PLARGE_INTEGER MinimumSize,
    _In_ PLARGE_INTEGER MaximumSize,
    _In_ ULONG Priority
    );

NTSYSCALLAPI
NTSTATUS
NTAPI
NtFlushInstructionCache(
    _In_ HANDLE ProcessHandle,
    _In_opt_ PVOID BaseAddress,
    _In_ SIZE_T Length
    );

NTSYSCALLAPI
NTSTATUS
NTAPI
NtFlushWriteBuffer(
    VOID
    );

#endif

#if (PHNT_VERSION >= PHNT_THRESHOLD)
// Enclave support

NTSYSCALLAPI
NTSTATUS
NTAPI
NtCreateEnclave(
    _In_ HANDLE ProcessHandle,
    _Inout_ PVOID* BaseAddress,
    _In_ ULONG_PTR ZeroBits,
    _In_ SIZE_T Size,
    _In_ SIZE_T InitialCommitment,
    _In_ ULONG EnclaveType,
    _In_reads_bytes_(EnclaveInformationLength) PVOID EnclaveInformation,
    _In_ ULONG EnclaveInformationLength,
    _Out_opt_ PULONG EnclaveError
    );

NTSYSCALLAPI
NTSTATUS
NTAPI
NtLoadEnclaveData(
    _In_ HANDLE ProcessHandle,
    _In_ PVOID BaseAddress,
    _In_reads_bytes_(BufferSize) PVOID Buffer,
    _In_ SIZE_T BufferSize,
    _In_ ULONG Protect,
    _In_reads_bytes_(PageInformationLength) PVOID PageInformation,
    _In_ ULONG PageInformationLength,
    _Out_opt_ PSIZE_T NumberOfBytesWritten,
    _Out_opt_ PULONG EnclaveError
    );

NTSYSCALLAPI
NTSTATUS
NTAPI
NtInitializeEnclave(
    _In_ HANDLE ProcessHandle,
    _In_ PVOID BaseAddress,
    _In_reads_bytes_(EnclaveInformationLength) PVOID EnclaveInformation,
    _In_ ULONG EnclaveInformationLength,
    _Out_opt_ PULONG EnclaveError
    );

// rev
#define TERMINATE_ENCLAVE_VALID_FLAGS     0x00000005ul
#define TERMINATE_ENCLAVE_FLAG_NO_WAIT    0x00000001ul
#define TERMINATE_ENCLAVE_FLAG_WAIT_ERROR 0x00000004ul // STATUS_PENDING -> STATUS_ENCLAVE_NOT_TERMINATED

// rev
NTSYSCALLAPI
NTSTATUS
NTAPI
NtTerminateEnclave(
    _In_ PVOID BaseAddress,
    _In_ ULONG Flags // TERMINATE_ENCLAVE_FLAG_*
    );

#if (PHNT_MODE != PHNT_MODE_KERNEL)

// rev
#define ENCLAVE_CALL_VALID_FLAGS  0x00000001ul
#define ENCLAVE_CALL_FLAG_NO_WAIT 0x00000001ul

// rev
NTSYSCALLAPI
NTSTATUS
NTAPI
NtCallEnclave(
    _In_ PENCLAVE_ROUTINE Routine,
    _In_ PVOID Reserved,              // reserved for dispatch (RtlEnclaveCallDispatch)
    _In_ ULONG Flags,                 // ENCLAVE_CALL_FLAG_*
    _Inout_ PVOID* RoutineParamReturn // input routine parameter, output routine return value
    );
#endif

#endif

#endif
