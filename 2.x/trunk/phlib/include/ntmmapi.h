#ifndef _NTMMAPI_H
#define _NTMMAPI_H

typedef enum _MEMORY_INFORMATION_CLASS
{
    MemoryBasicInformation,
    MemoryWorkingSetInformation,
    MemoryMappedFilenameInformation,
    MemoryRegionInformation,
    MemoryWorkingSetExInformation
} MEMORY_INFORMATION_CLASS;

typedef struct _MEMORY_WORKING_SET_BLOCK
{
    ULONG_PTR Protection : 5;
    ULONG_PTR ShareCount : 3;
    ULONG_PTR Shared : 1;
    ULONG_PTR Node : 3;
#ifdef _M_X64
    ULONG_PTR VirtualPage : 52;
#else
    ULONG VirtualPage : 20;
#endif
} MEMORY_WORKING_SET_BLOCK, *PMEMORY_WORKING_SET_BLOCK;

typedef struct _MEMORY_WORKING_SET_INFORMATION
{
    ULONG_PTR NumberOfEntries;
    MEMORY_WORKING_SET_BLOCK WorkingSetInfo[1];
} MEMORY_WORKING_SET_INFORMATION, *PMEMORY_WORKING_SET_INFORMATION;

// private
typedef struct _MEMORY_REGION_INFORMATION
{
    PVOID AllocationBase;
    ULONG AllocationProtect;
    ULONG RegionType;
    SIZE_T RegionSize;
} MEMORY_REGION_INFORMATION, *PMEMORY_REGION_INFORMATION;

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
            ULONG_PTR Reserved : 4;
            ULONG_PTR Bad : 1;
#ifdef _M_X64
            ULONG_PTR ReservedUlong : 32;
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
            ULONG_PTR Reserved2 : 7;
            ULONG_PTR Bad : 1;
#ifdef _M_X64
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
        ULONG Long;
    } u1;
} MEMORY_WORKING_SET_EX_INFORMATION, *PMEMORY_WORKING_SET_EX_INFORMATION;

#define MMPFNLIST_ZERO 0
#define MMPFNLIST_FREE 1
#define MMPFNLIST_STANDBY 2
#define MMPFNLIST_MODIFIED 3
#define MMPFNLIST_MODIFIEDNOWRITE 4
#define MMPFNLIST_BAD 5
#define MMPFNLIST_ACTIVE 6
#define MMPFNLIST_TRANSITION 7

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

typedef struct _MEMORY_FRAME_INFORMATION
{
    ULONGLONG UseDescription : 4; // MMPFNUSE_*
    ULONGLONG ListDescription : 3; // MMPFNLIST_*
    ULONGLONG Reserved0 : 1; // reserved for future expansion
    ULONGLONG Pinned : 1; // 1 - pinned, 0 - not pinned
    ULONGLONG DontUse : 48; // *_INFORMATION overlay
    ULONGLONG Priority : 3; // rev
    ULONGLONG Reserved : 4; // reserved for future expansion
} MEMORY_FRAME_INFORMATION;

typedef struct _FILEOFFSET_INFORMATION
{
    ULONGLONG DontUse : 9; // MEMORY_FRAME_INFORMATION overlay
    ULONGLONG Offset : 48; // mapped files
    ULONGLONG Reserved : 7; // reserved for future expansion
} FILEOFFSET_INFORMATION;

typedef struct _PAGEDIR_INFORMATION
{
    ULONGLONG DontUse : 9; // MEMORY_FRAME_INFORMATION overlay
    ULONGLONG PageDirectoryBase : 48; // private pages
    ULONGLONG Reserved : 7; // reserved for future expansion
} PAGEDIR_INFORMATION;

typedef struct _MMPFN_IDENTITY
{
    union
    {
        MEMORY_FRAME_INFORMATION e1; // all
        FILEOFFSET_INFORMATION e2; // mapped files
        PAGEDIR_INFORMATION e3; // private pages
    } u1;
    ULONG_PTR PageFrameIndex; // all
    union
    {
        PVOID FileObject; // mapped files
        PVOID VirtualAddress; // everything else
    } u2;
} MMPFN_IDENTITY, *PMMPFN_IDENTITY;

typedef struct _MMPFN_MEMSNAP_INFORMATION
{
    ULONG_PTR InitialPageFrameIndex;
    ULONG_PTR Count;
} MMPFN_MEMSNAP_INFORMATION, *PMMPFN_MEMSNAP_INFORMATION;

typedef enum _SECTION_INFORMATION_CLASS
{
    SectionBasicInformation,
    SectionImageInformation,
    SectionRelocationInformation, // name:wow64:whNtQuerySection_SectionRelocationInformation
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
    ULONG GpValue;
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
            UCHAR Reserved : 4;
        };
    };
    ULONG LoaderFlags;
    ULONG ImageFileSize;
    ULONG CheckSum;
} SECTION_IMAGE_INFORMATION, *PSECTION_IMAGE_INFORMATION;

typedef enum _SECTION_INHERIT
{
    ViewShare = 1,
    ViewUnmap = 2
} SECTION_INHERIT;

#define SEC_BASED 0x200000
#define SEC_NO_CHANGE 0x400000
#define SEC_GLOBAL 0x20000000

#define MEM_EXECUTE_OPTION_DISABLE 0x1 
#define MEM_EXECUTE_OPTION_ENABLE 0x2
#define MEM_EXECUTE_OPTION_DISABLE_THUNK_EMULATION 0x4
#define MEM_EXECUTE_OPTION_PERMANENT 0x8
#define MEM_EXECUTE_OPTION_EXECUTE_DISPATCH_ENABLE 0x10
#define MEM_EXECUTE_OPTION_IMAGE_DISPATCH_ENABLE 0x20 
#define MEM_EXECUTE_OPTION_VALID_FLAGS 0x3f

// Virtual memory

NTSYSCALLAPI
NTSTATUS
NTAPI
NtAllocateVirtualMemory(
    __in HANDLE ProcessHandle,
    __inout PVOID *BaseAddress,
    __in ULONG_PTR ZeroBits,
    __inout PSIZE_T RegionSize,
    __in ULONG AllocationType,
    __in ULONG Protect
    );

NTSYSCALLAPI
NTSTATUS
NTAPI
NtFreeVirtualMemory(
    __in HANDLE ProcessHandle,
    __inout PVOID *BaseAddress,
    __inout PSIZE_T RegionSize,
    __in ULONG FreeType
    );

NTSYSCALLAPI
NTSTATUS
NTAPI
NtReadVirtualMemory(
    __in HANDLE ProcessHandle,
    __in_opt PVOID BaseAddress,
    __out_bcount(BufferSize) PVOID Buffer,
    __in SIZE_T BufferSize,
    __out_opt PSIZE_T NumberOfBytesRead
    );

NTSYSCALLAPI
NTSTATUS
NTAPI
NtWriteVirtualMemory(
    __in HANDLE ProcessHandle,
    __in_opt PVOID BaseAddress,
    __in_bcount(BufferSize) PVOID Buffer,
    __in SIZE_T BufferSize,
    __out_opt PSIZE_T NumberOfBytesWritten
    );

NTSYSCALLAPI
NTSTATUS
NTAPI
NtProtectVirtualMemory(
    __in HANDLE ProcessHandle,
    __inout PVOID *BaseAddress,
    __inout PSIZE_T RegionSize,
    __in ULONG NewProtect,
    __out PULONG OldProtect
    );

NTSYSCALLAPI
NTSTATUS
NTAPI
NtQueryVirtualMemory(
    __in HANDLE ProcessHandle,
    __in PVOID BaseAddress,
    __in MEMORY_INFORMATION_CLASS MemoryInformationClass,
    __out_bcount(MemoryInformationLength) PVOID MemoryInformation,
    __in SIZE_T MemoryInformationLength,
    __out_opt PSIZE_T ReturnLength
    );

NTSYSCALLAPI
NTSTATUS
NTAPI
NtLockVirtualMemory(
    __in HANDLE ProcessHandle,
    __inout PVOID *BaseAddress,
    __inout PSIZE_T RegionSize,
    __in ULONG MapType
    );

NTSYSCALLAPI
NTSTATUS
NTAPI
NtUnlockVirtualMemory( 
    __in HANDLE ProcessHandle,
    __inout PVOID *BaseAddress,
    __inout PSIZE_T RegionSize,
    __in ULONG MapType
    );

// Sections

NTSYSCALLAPI
NTSTATUS
NTAPI
NtCreateSection(
    __out PHANDLE SectionHandle,
    __in ACCESS_MASK DesiredAccess,
    __in_opt POBJECT_ATTRIBUTES ObjectAttributes,
    __in_opt PLARGE_INTEGER MaximumSize,
    __in ULONG SectionPageProtection,
    __in ULONG AllocationAttributes,
    __in_opt HANDLE FileHandle
    );

NTSYSCALLAPI
NTSTATUS
NTAPI
NtOpenSection(
    __out PHANDLE SectionHandle,
    __in ACCESS_MASK DesiredAccess,
    __in POBJECT_ATTRIBUTES ObjectAttributes
    );

NTSYSCALLAPI
NTSTATUS
NTAPI
NtMapViewOfSection(
    __in HANDLE SectionHandle,
    __in HANDLE ProcessHandle,
    __inout PVOID *BaseAddress,
    __in ULONG_PTR ZeroBits,
    __in SIZE_T CommitSize,
    __inout_opt PLARGE_INTEGER SectionOffset,
    __inout PSIZE_T ViewSize,
    __in SECTION_INHERIT InheritDisposition,
    __in ULONG AllocationType,
    __in ULONG Win32Protect
    );

NTSYSCALLAPI
NTSTATUS
NTAPI
NtUnmapViewOfSection(
    __in HANDLE ProcessHandle,
    __in PVOID BaseAddress
    );

NTSYSCALLAPI
NTSTATUS
NTAPI
NtExtendSection(
    __in HANDLE SectionHandle,
    __inout PLARGE_INTEGER NewSectionSize
    );

NTSYSCALLAPI
NTSTATUS
NTAPI
NtQuerySection(
    __in HANDLE SectionHandle,
    __in SECTION_INFORMATION_CLASS SectionInformationClass,
    __out_bcount(SectionInformationLength) PVOID SectionInformation,
    __in SIZE_T SectionInformationLength,
    __out_opt PSIZE_T ReturnLength
    );

NTSYSCALLAPI
NTSTATUS
NTAPI
NtAreMappedFilesTheSame(
    __in PVOID File1MappedAsAnImage,
    __in PVOID File2MappedAsFile
    );

// User physical pages

NTSYSCALLAPI
NTSTATUS
NTAPI
NtMapUserPhysicalPages(
    __in PVOID VirtualAddress,
    __in ULONG_PTR NumberOfPages,
    __in_ecount_opt(NumberOfPages) PULONG_PTR UserPfnArray
    );

NTSYSCALLAPI
NTSTATUS
NTAPI
NtMapUserPhysicalPagesScatter(
    __in_ecount(NumberOfPages) PVOID *VirtualAddresses,
    __in ULONG_PTR NumberOfPages,
    __in_ecount_opt(NumberOfPages) PULONG_PTR UserPfnArray
    );

NTSYSCALLAPI
NTSTATUS
NTAPI
NtAllocateUserPhysicalPages(
    __in HANDLE ProcessHandle,
    __inout PULONG_PTR NumberOfPages,
    __out_ecount(*NumberOfPages) PULONG_PTR UserPfnArray
    );

NTSYSCALLAPI
NTSTATUS
NTAPI
NtFreeUserPhysicalPages(
    __in HANDLE ProcessHandle,
    __inout PULONG_PTR NumberOfPages,
    __in_ecount(*NumberOfPages) PULONG_PTR UserPfnArray
    );

// Sessions

#if (PHNT_VERSION >= PHNT_VISTA)
// private
NTSYSCALLAPI
NTSTATUS
NTAPI
NtOpenSession(
    __out PHANDLE SessionHandle,
    __in ACCESS_MASK DesiredAccess,
    __in POBJECT_ATTRIBUTES ObjectAttributes
    );
#endif

// missing:NtNotifyChangeSession

// Misc.

NTSYSCALLAPI
NTSTATUS
NTAPI
NtGetWriteWatch(
    __in HANDLE ProcessHandle,
    __in ULONG Flags,
    __in PVOID BaseAddress,
    __in SIZE_T RegionSize,
    __out_ecount(*EntriesInUserAddressArray) PVOID *UserAddressArray,
    __inout PULONG_PTR EntriesInUserAddressArray,
    __out PULONG Granularity
    );

NTSYSCALLAPI
NTSTATUS
NTAPI
NtResetWriteWatch(
    __in HANDLE ProcessHandle,
    __in PVOID BaseAddress,
    __in SIZE_T RegionSize
    );

NTSYSCALLAPI
NTSTATUS
NTAPI
NtCreatePagingFile(
    __in PUNICODE_STRING PageFileName,
    __in PLARGE_INTEGER MinimumSize,
    __in PLARGE_INTEGER MaximumSize,
    __in ULONG Priority
    );

NTSYSCALLAPI
NTSTATUS
NTAPI
NtFlushInstructionCache(
    __in HANDLE ProcessHandle,
    __in_opt PVOID BaseAddress,
    __in SIZE_T Length
    );

NTSYSCALLAPI
NTSTATUS
NTAPI
NtFlushWriteBuffer(
    VOID
    );

#endif
