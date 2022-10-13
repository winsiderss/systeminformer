#ifndef PH_MODPRV_H
#define PH_MODPRV_H

extern PPH_OBJECT_TYPE PhModuleProviderType;
extern PPH_OBJECT_TYPE PhModuleItemType;

// begin_phapppub
typedef struct _PH_MODULE_ITEM
{
    PVOID BaseAddress;
    PVOID ParentBaseAddress;
    PVOID EntryPoint;
    ULONG Size;
    ULONG Flags;
    ULONG Type;
    USHORT LoadReason;
    USHORT LoadCount;
    PPH_STRING Name;
    PPH_STRING FileNameWin32;
    PPH_STRING FileName;
    PH_IMAGE_VERSION_INFO VersionInfo;

    union
    {
        BOOLEAN StateFlags;
        struct
        {
            BOOLEAN JustProcessed : 1;
            BOOLEAN IsFirst : 1;
            BOOLEAN ImageNotAtBase : 1;
            BOOLEAN ImageKnownDll : 1;
            BOOLEAN Spare : 4;
        };
    };

    enum _VERIFY_RESULT VerifyResult;
    PPH_STRING VerifySignerName;

    ULONG ImageTimeDateStamp;
    USHORT ImageCharacteristics;
    USHORT ImageDllCharacteristics;
    ULONG ImageDllCharacteristicsEx;
    ULONG GuardFlags;

    LARGE_INTEGER LoadTime;
    LARGE_INTEGER FileLastWriteTime;
    LARGE_INTEGER FileEndOfFile;

    NTSTATUS ImageCoherencyStatus;
    FLOAT ImageCoherency;

    WCHAR BaseAddressString[PH_PTR_STR_LEN_1];
    WCHAR ParentBaseAddressString[PH_PTR_STR_LEN_1];
    WCHAR EntryPointAddressString[PH_PTR_STR_LEN_1];
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
    PPH_STRING ProcessFileName;
    PPH_STRING PackageFullName;
    SLIST_HEADER QueryListHead;
    NTSTATUS RunStatus;

    union
    {
        BOOLEAN Flags;
        struct
        {
            BOOLEAN HaveFirst : 1;
            BOOLEAN ControlFlowGuardEnabled : 1;
            BOOLEAN IsSubsystemProcess : 1;
            BOOLEAN CetEnabled : 1;
            BOOLEAN CetStrictModeEnabled : 1;
            BOOLEAN ZeroPadAddresses : 1;
            BOOLEAN Spare : 2;
        };
    };
    UCHAR ImageCoherencyScanLevel;
} PH_MODULE_PROVIDER, *PPH_MODULE_PROVIDER;
// end_phapppub

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
