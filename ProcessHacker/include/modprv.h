#ifndef PH_MODPRV_H
#define PH_MODPRV_H

extern PPH_OBJECT_TYPE PhModuleProviderType;
extern PPH_OBJECT_TYPE PhModuleItemType;

// begin_phapppub
typedef struct _PH_MODULE_ITEM
{
    PVOID BaseAddress;
    ULONG Size;
    ULONG Flags;
    ULONG Type;
    USHORT LoadReason;
    USHORT LoadCount;
    PPH_STRING Name;
    PPH_STRING FileName;
    PH_IMAGE_VERSION_INFO VersionInfo;

    WCHAR BaseAddressString[PH_PTR_STR_LEN_1];

    BOOLEAN IsFirst;
    BOOLEAN JustProcessed;

    enum _VERIFY_RESULT VerifyResult;
    PPH_STRING VerifySignerName;

    ULONG ImageTimeDateStamp;
    USHORT ImageCharacteristics;
    USHORT ImageDllCharacteristics;

    LARGE_INTEGER LoadTime;

    LARGE_INTEGER FileLastWriteTime;
    LARGE_INTEGER FileEndOfFile;
} PH_MODULE_ITEM, *PPH_MODULE_ITEM;

typedef struct _PH_MODULE_PROVIDER
{
    PPH_HASHTABLE ModuleHashtable;
    PH_FAST_LOCK ModuleHashtableLock;
    PH_CALLBACK ModuleAddedEvent;
    PH_CALLBACK ModuleModifiedEvent;
    PH_CALLBACK ModuleRemovedEvent;
    PH_CALLBACK UpdatedEvent;

    HANDLE ProcessId;
    HANDLE ProcessHandle;
    PPH_STRING PackageFullName;
    SLIST_HEADER QueryListHead;
    NTSTATUS RunStatus;
} PH_MODULE_PROVIDER, *PPH_MODULE_PROVIDER;
// end_phapppub

BOOLEAN PhModuleProviderInitialization(
    VOID
    );

PPH_MODULE_PROVIDER PhCreateModuleProvider(
    _In_ HANDLE ProcessId
    );

PPH_MODULE_ITEM PhCreateModuleItem(
    VOID
    );

PPH_MODULE_ITEM PhReferenceModuleItem(
    _In_ PPH_MODULE_PROVIDER ModuleProvider,
    _In_ PVOID BaseAddress
    );

VOID PhDereferenceAllModuleItems(
    _In_ PPH_MODULE_PROVIDER ModuleProvider
    );

VOID PhModuleProviderUpdate(
    _In_ PVOID Object
    );

#endif
