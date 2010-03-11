#ifndef NTLDR_H
#define NTLDR_H

#define LDRP_STATIC_LINK 0x00000002
#define LDRP_IMAGE_DLL 0x00000004
#define LDRP_LOAD_IN_PROGRESS 0x00001000
#define LDRP_UNLOAD_IN_PROGRESS 0x00002000
#define LDRP_ENTRY_PROCESSED 0x00004000
#define LDRP_ENTRY_INSERTED 0x00008000
#define LDRP_CURRENT_LOAD 0x00010000
#define LDRP_FAILED_BUILTIN_LOAD 0x00020000
#define LDRP_DONT_CALL_FOR_THREADS 0x00040000
#define LDRP_PROCESS_ATTACH_CALLED 0x00080000
#define LDRP_DEBUG_SYMBOLS_LOADED 0x00100000
#define LDRP_IMAGE_NOT_AT_BASE 0x00200000
#define LDRP_COR_IMAGE 0x00400000
#define LDRP_COR_OWNS_UNMAP 0x00800000
#define LDRP_SYSTEM_MAPPED 0x01000000
#define LDRP_IMAGE_VERIFYING 0x02000000
#define LDRP_DRIVER_DEPENDENT_DLL 0x04000000
#define LDRP_ENTRY_NATIVE 0x08000000
#define LDRP_REDIRECTED 0x10000000
#define LDRP_NON_PAGED_DEBUG_INFO 0x20000000
#define LDRP_MM_LOADED 0x40000000
#define LDRP_COMPAT_DATABASE_PROCESSED 0x80000000

// Use the size of the structure as it was in 
// Windows XP.
#define LDR_DATA_TABLE_ENTRY_SIZE FIELD_OFFSET(LDR_DATA_TABLE_ENTRY, ForwarderLinks)

typedef struct _LDR_DATA_TABLE_ENTRY
{
    LIST_ENTRY InLoadOrderLinks;
    LIST_ENTRY InMemoryOrderLinks;
    LIST_ENTRY InInitializationOrderLinks;
    PVOID DllBase;
    PVOID EntryPoint;
    ULONG SizeOfImage;
    UNICODE_STRING FullDllName;
    UNICODE_STRING BaseDllName;
    ULONG Flags;
    USHORT LoadCount;
    USHORT TlsIndex;
    union
    {
        LIST_ENTRY HashLinks;
        struct
        {
            PVOID SectionPointer;
            ULONG CheckSum;
        };
    };
    union
    {
        ULONG TimeDateStamp;
        PVOID LoadedImports;
    };
    PVOID EntryPointActivationContext;
    PVOID PatchInformation;
    LIST_ENTRY ForwarderLinks;
    LIST_ENTRY ServiceTagLinks;
    LIST_ENTRY StaticLinks;
    PVOID ConextInformation;
    ULONG_PTR OriginalBase;
    LARGE_INTEGER LoadTime;
} LDR_DATA_TABLE_ENTRY, *PLDR_DATA_TABLE_ENTRY;

typedef struct _RTL_PROCESS_MODULE_INFORMATION
{
    HANDLE Section;
    PVOID MappedBase;
    PVOID ImageBase;
    ULONG ImageSize;
    ULONG Flags;
    USHORT LoadOrderIndex;
    USHORT InitOrderIndex;
    USHORT LoadCount;
    USHORT OffsetToFileName;
    UCHAR FullPathName[256];
} RTL_PROCESS_MODULE_INFORMATION, *PRTL_PROCESS_MODULE_INFORMATION;

typedef struct _RTL_PROCESS_MODULES
{
    ULONG NumberOfModules;
    RTL_PROCESS_MODULE_INFORMATION Modules[1];
} RTL_PROCESS_MODULES, *PRTL_PROCESS_MODULES;

#endif
