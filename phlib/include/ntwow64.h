#ifndef _NTWOW64_H
#define _NTWOW64_H

#define WOW64_SYSTEM_DIRECTORY "SysWOW64"
#define WOW64_SYSTEM_DIRECTORY_U L"SysWOW64"
#define WOW64_X86_TAG " (x86)"
#define WOW64_X86_TAG_U L" (x86)"

// In USER_SHARED_DATA
// symbols
typedef enum _WOW64_SHARED_INFORMATION
{
    SharedNtdll32LdrInitializeThunk = 0,
    SharedNtdll32KiUserExceptionDispatcher = 1,
    SharedNtdll32KiUserApcDispatcher = 2,
    SharedNtdll32KiUserCallbackDispatcher = 3,
    SharedNtdll32LdrHotPatchRoutine = 4,
    SharedNtdll32ExpInterlockedPopEntrySListFault = 5,
    SharedNtdll32ExpInterlockedPopEntrySListResume = 6,
    SharedNtdll32ExpInterlockedPopEntrySListEnd = 7,
    SharedNtdll32RtlUserThreadStart = 8,
    SharedNtdll32pQueryProcessDebugInformationRemote = 9,
    SharedNtdll32EtwpNotificationThread = 10,
    SharedNtdll32BaseAddress = 11,
    Wow64SharedPageEntriesCount = 12
} WOW64_SHARED_INFORMATION;

// 32-bit definitions

#define WOW64_POINTER(Type) ULONG

typedef struct _RTL_BALANCED_NODE32
{
    union
    {
        WOW64_POINTER(struct _RTL_BALANCED_NODE *) Children[2];
        struct
        {
            WOW64_POINTER(struct _RTL_BALANCED_NODE *) Left;
            WOW64_POINTER(struct _RTL_BALANCED_NODE *) Right;
        };
    };
    union
    {
        WOW64_POINTER(ULONG_PTR) Red : 1;
        WOW64_POINTER(ULONG_PTR) Balance : 2;
        WOW64_POINTER(ULONG_PTR) ParentValue;
    };
} RTL_BALANCED_NODE32, *PRTL_BALANCED_NODE32;

typedef struct _RTL_RB_TREE32
{
    WOW64_POINTER(PRTL_BALANCED_NODE) Root;
    WOW64_POINTER(PRTL_BALANCED_NODE) Min;
} RTL_RB_TREE32, *PRTL_RB_TREE32;

typedef struct _PEB_LDR_DATA32
{
    ULONG Length;
    BOOLEAN Initialized;
    WOW64_POINTER(HANDLE) SsHandle;
    LIST_ENTRY32 InLoadOrderModuleList;
    LIST_ENTRY32 InMemoryOrderModuleList;
    LIST_ENTRY32 InInitializationOrderModuleList;
    WOW64_POINTER(PVOID) EntryInProgress;
    BOOLEAN ShutdownInProgress;
    WOW64_POINTER(HANDLE) ShutdownThreadId;
} PEB_LDR_DATA32, *PPEB_LDR_DATA32;

typedef struct _LDR_SERVICE_TAG_RECORD32
{
    WOW64_POINTER(struct _LDR_SERVICE_TAG_RECORD *) Next;
    ULONG ServiceTag;
} LDR_SERVICE_TAG_RECORD32, *PLDR_SERVICE_TAG_RECORD32;

typedef struct _LDRP_CSLIST32
{
    WOW64_POINTER(PSINGLE_LIST_ENTRY) Tail;
} LDRP_CSLIST32, *PLDRP_CSLIST32;

typedef struct _LDR_DDAG_NODE32
{
    LIST_ENTRY32 Modules;
    WOW64_POINTER(PLDR_SERVICE_TAG_RECORD) ServiceTagList;
    ULONG LoadCount;
    ULONG ReferenceCount;
    ULONG DependencyCount;
    union
    {
        LDRP_CSLIST32 Dependencies;
        SINGLE_LIST_ENTRY32 RemovalLink;
    };
    LDRP_CSLIST32 IncomingDependencies;
    LDR_DDAG_STATE State;
    SINGLE_LIST_ENTRY32 CondenseLink;
    ULONG PreorderNumber;
    ULONG LowestLink;
} LDR_DDAG_NODE32, *PLDR_DDAG_NODE32;

#define LDR_DATA_TABLE_ENTRY_SIZE_WINXP32 FIELD_OFFSET(LDR_DATA_TABLE_ENTRY32, DdagNode)

typedef struct _LDR_DATA_TABLE_ENTRY32
{
    LIST_ENTRY32 InLoadOrderLinks;
    LIST_ENTRY32 InMemoryOrderLinks;
    union
    {
        LIST_ENTRY32 InInitializationOrderLinks;
        LIST_ENTRY32 InProgressLinks;
    };
    WOW64_POINTER(PVOID) DllBase;
    WOW64_POINTER(PVOID) EntryPoint;
    ULONG SizeOfImage;
    UNICODE_STRING32 FullDllName;
    UNICODE_STRING32 BaseDllName;
    union
    {
        UCHAR FlagGroup[4];
        ULONG Flags;
        struct
        {
            ULONG PackagedBinary : 1;
            ULONG MarkedForRemoval : 1;
            ULONG ImageDll : 1;
            ULONG LoadNotificationsSent : 1;
            ULONG TelemetryEntryProcessed : 1;
            ULONG ProcessStaticImport : 1;
            ULONG InLegacyLists : 1;
            ULONG InIndexes : 1;
            ULONG ShimDll : 1;
            ULONG InExceptionTable : 1;
            ULONG ReservedFlags1 : 2;
            ULONG LoadInProgress : 1;
            ULONG ReservedFlags2 : 1;
            ULONG EntryProcessed : 1;
            ULONG ReservedFlags3 : 3;
            ULONG DontCallForThreads : 1;
            ULONG ProcessAttachCalled : 1;
            ULONG ProcessAttachFailed : 1;
            ULONG CorDeferredValidate : 1;
            ULONG CorImage : 1;
            ULONG DontRelocate : 1;
            ULONG CorILOnly : 1;
            ULONG ReservedFlags5 : 3;
            ULONG Redirected : 1;
            ULONG ReservedFlags6 : 2;
            ULONG CompatDatabaseProcessed : 1;
        };
    };
    USHORT ObsoleteLoadCount;
    USHORT TlsIndex;
    LIST_ENTRY32 HashLinks;
    ULONG TimeDateStamp;
    WOW64_POINTER(struct _ACTIVATION_CONTEXT *) EntryPointActivationContext;
    WOW64_POINTER(PVOID) PatchInformation;
    WOW64_POINTER(PLDR_DDAG_NODE) DdagNode;
    LIST_ENTRY32 NodeModuleLink;
    WOW64_POINTER(struct _LDRP_DLL_SNAP_CONTEXT *) SnapContext;
    WOW64_POINTER(PVOID) SwitchBackContext;
    RTL_BALANCED_NODE32 BaseAddressIndexNode;
    RTL_BALANCED_NODE32 MappingInfoIndexNode;
    WOW64_POINTER(ULONG_PTR) OriginalBase;
    LARGE_INTEGER LoadTime;
    ULONG BaseNameHashValue;
} LDR_DATA_TABLE_ENTRY32, *PLDR_DATA_TABLE_ENTRY32;

typedef struct _CURDIR32
{
    UNICODE_STRING32 DosPath;
    WOW64_POINTER(HANDLE) Handle;
} CURDIR32, *PCURDIR32;

typedef struct _RTL_DRIVE_LETTER_CURDIR32
{
    USHORT Flags;
    USHORT Length;
    ULONG TimeStamp;
    STRING32 DosPath;
} RTL_DRIVE_LETTER_CURDIR32, *PRTL_DRIVE_LETTER_CURDIR32;

typedef struct _RTL_USER_PROCESS_PARAMETERS32
{
    ULONG MaximumLength;
    ULONG Length;

    ULONG Flags;
    ULONG DebugFlags;

    WOW64_POINTER(HANDLE) ConsoleHandle;
    ULONG ConsoleFlags;
    WOW64_POINTER(HANDLE) StandardInput;
    WOW64_POINTER(HANDLE) StandardOutput;
    WOW64_POINTER(HANDLE) StandardError;

    CURDIR32 CurrentDirectory;
    UNICODE_STRING32 DllPath;
    UNICODE_STRING32 ImagePathName;
    UNICODE_STRING32 CommandLine;
    WOW64_POINTER(PVOID) Environment;

    ULONG StartingX;
    ULONG StartingY;
    ULONG CountX;
    ULONG CountY;
    ULONG CountCharsX;
    ULONG CountCharsY;
    ULONG FillAttribute;

    ULONG WindowFlags;
    ULONG ShowWindowFlags;
    UNICODE_STRING32 WindowTitle;
    UNICODE_STRING32 DesktopInfo;
    UNICODE_STRING32 ShellInfo;
    UNICODE_STRING32 RuntimeData;
    RTL_DRIVE_LETTER_CURDIR32 CurrentDirectories[RTL_MAX_DRIVE_LETTERS];

    ULONG EnvironmentSize;
    ULONG EnvironmentVersion;
} RTL_USER_PROCESS_PARAMETERS32, *PRTL_USER_PROCESS_PARAMETERS32;

typedef struct _PEB32
{
    BOOLEAN InheritedAddressSpace;
    BOOLEAN ReadImageFileExecOptions;
    BOOLEAN BeingDebugged;
    union
    {
        BOOLEAN BitField;
        struct
        {
            BOOLEAN ImageUsesLargePages : 1;
            BOOLEAN IsProtectedProcess : 1;
            BOOLEAN IsLegacyProcess : 1;
            BOOLEAN IsImageDynamicallyRelocated : 1;
            BOOLEAN SkipPatchingUser32Forwarders : 1;
            BOOLEAN SpareBits : 3;
        };
    };
    WOW64_POINTER(HANDLE) Mutant;

    WOW64_POINTER(PVOID) ImageBaseAddress;
    WOW64_POINTER(PPEB_LDR_DATA) Ldr;
    WOW64_POINTER(PRTL_USER_PROCESS_PARAMETERS) ProcessParameters;
    WOW64_POINTER(PVOID) SubSystemData;
    WOW64_POINTER(PVOID) ProcessHeap;
    WOW64_POINTER(PRTL_CRITICAL_SECTION) FastPebLock;
    WOW64_POINTER(PVOID) AtlThunkSListPtr;
    WOW64_POINTER(PVOID) IFEOKey;
    union
    {
        ULONG CrossProcessFlags;
        struct
        {
            ULONG ProcessInJob : 1;
            ULONG ProcessInitializing : 1;
            ULONG ProcessUsingVEH : 1;
            ULONG ProcessUsingVCH : 1;
            ULONG ProcessUsingFTH : 1;
            ULONG ReservedBits0 : 27;
        };
        ULONG EnvironmentUpdateCount;
    };
    union
    {
        WOW64_POINTER(PVOID) KernelCallbackTable;
        WOW64_POINTER(PVOID) UserSharedInfoPtr;
    };
    ULONG SystemReserved[1];
    ULONG AtlThunkSListPtr32;
    WOW64_POINTER(PVOID) ApiSetMap;
    ULONG TlsExpansionCounter;
    WOW64_POINTER(PVOID) TlsBitmap;
    ULONG TlsBitmapBits[2];
    WOW64_POINTER(PVOID) ReadOnlySharedMemoryBase;
    WOW64_POINTER(PVOID) HotpatchInformation;
    WOW64_POINTER(PPVOID) ReadOnlyStaticServerData;
    WOW64_POINTER(PVOID) AnsiCodePageData;
    WOW64_POINTER(PVOID) OemCodePageData;
    WOW64_POINTER(PVOID) UnicodeCaseTableData;

    ULONG NumberOfProcessors;
    ULONG NtGlobalFlag;

    LARGE_INTEGER CriticalSectionTimeout;
    WOW64_POINTER(SIZE_T) HeapSegmentReserve;
    WOW64_POINTER(SIZE_T) HeapSegmentCommit;
    WOW64_POINTER(SIZE_T) HeapDeCommitTotalFreeThreshold;
    WOW64_POINTER(SIZE_T) HeapDeCommitFreeBlockThreshold;

    ULONG NumberOfHeaps;
    ULONG MaximumNumberOfHeaps;
    WOW64_POINTER(PPVOID) ProcessHeaps;

    WOW64_POINTER(PVOID) GdiSharedHandleTable;
    WOW64_POINTER(PVOID) ProcessStarterHelper;
    ULONG GdiDCAttributeList;

    WOW64_POINTER(PRTL_CRITICAL_SECTION) LoaderLock;

    ULONG OSMajorVersion;
    ULONG OSMinorVersion;
    USHORT OSBuildNumber;
    USHORT OSCSDVersion;
    ULONG OSPlatformId;
    ULONG ImageSubsystem;
    ULONG ImageSubsystemMajorVersion;
    ULONG ImageSubsystemMinorVersion;
    WOW64_POINTER(ULONG_PTR) ImageProcessAffinityMask;
    GDI_HANDLE_BUFFER32 GdiHandleBuffer;
    WOW64_POINTER(PVOID) PostProcessInitRoutine;

    WOW64_POINTER(PVOID) TlsExpansionBitmap;
    ULONG TlsExpansionBitmapBits[32];

    ULONG SessionId;

    ULARGE_INTEGER AppCompatFlags;
    ULARGE_INTEGER AppCompatFlagsUser;
    WOW64_POINTER(PVOID) pShimData;
    WOW64_POINTER(PVOID) AppCompatInfo;

    UNICODE_STRING32 CSDVersion;

    WOW64_POINTER(PVOID) ActivationContextData;
    WOW64_POINTER(PVOID) ProcessAssemblyStorageMap;
    WOW64_POINTER(PVOID) SystemDefaultActivationContextData;
    WOW64_POINTER(PVOID) SystemAssemblyStorageMap;

    WOW64_POINTER(SIZE_T) MinimumStackCommit;

    WOW64_POINTER(PPVOID) FlsCallback;
    LIST_ENTRY32 FlsListHead;
    WOW64_POINTER(PVOID) FlsBitmap;
    ULONG FlsBitmapBits[FLS_MAXIMUM_AVAILABLE / (sizeof(ULONG) * 8)];
    ULONG FlsHighIndex;

    WOW64_POINTER(PVOID) WerRegistrationData;
    WOW64_POINTER(PVOID) WerShipAssertPtr;
    WOW64_POINTER(PVOID) pContextData;
    WOW64_POINTER(PVOID) pImageHeaderHash;
    union
    {
        ULONG TracingFlags;
        struct
        {
            ULONG HeapTracingEnabled : 1;
            ULONG CritSecTracingEnabled : 1;
            ULONG SpareTracingBits : 30;
        };
    };
} PEB32, *PPEB32;

#define GDI_BATCH_BUFFER_SIZE 310

typedef struct _GDI_TEB_BATCH32
{
    ULONG Offset;
    WOW64_POINTER(ULONG_PTR) HDC;
    ULONG Buffer[GDI_BATCH_BUFFER_SIZE];
} GDI_TEB_BATCH32, *PGDI_TEB_BATCH32;

typedef struct _TEB32
{
    NT_TIB32 NtTib;

    WOW64_POINTER(PVOID) EnvironmentPointer;
    CLIENT_ID32 ClientId;
    WOW64_POINTER(PVOID) ActiveRpcHandle;
    WOW64_POINTER(PVOID) ThreadLocalStoragePointer;
    WOW64_POINTER(PPEB) ProcessEnvironmentBlock;

    ULONG LastErrorValue;
    ULONG CountOfOwnedCriticalSections;
    WOW64_POINTER(PVOID) CsrClientThread;
    WOW64_POINTER(PVOID) Win32ThreadInfo;
    ULONG User32Reserved[26];
    ULONG UserReserved[5];
    WOW64_POINTER(PVOID) WOW32Reserved;
    LCID CurrentLocale;
    ULONG FpSoftwareStatusRegister;
    WOW64_POINTER(PVOID) SystemReserved1[54];
    NTSTATUS ExceptionCode;
    WOW64_POINTER(PVOID) ActivationContextStackPointer;
    BYTE SpareBytes[36];
    ULONG TxFsContext;

    GDI_TEB_BATCH32 GdiTebBatch;
    CLIENT_ID32 RealClientId;
    WOW64_POINTER(HANDLE) GdiCachedProcessHandle;
    ULONG GdiClientPID;
    ULONG GdiClientTID;
    WOW64_POINTER(PVOID) GdiThreadLocalInfo;
    WOW64_POINTER(ULONG_PTR) Win32ClientInfo[62];
    WOW64_POINTER(PVOID) glDispatchTable[233];
    WOW64_POINTER(ULONG_PTR) glReserved1[29];
    WOW64_POINTER(PVOID) glReserved2;
    WOW64_POINTER(PVOID) glSectionInfo;
    WOW64_POINTER(PVOID) glSection;
    WOW64_POINTER(PVOID) glTable;
    WOW64_POINTER(PVOID) glCurrentRC;
    WOW64_POINTER(PVOID) glContext;

    NTSTATUS LastStatusValue;
    UNICODE_STRING32 StaticUnicodeString;
    WCHAR StaticUnicodeBuffer[261];

    WOW64_POINTER(PVOID) DeallocationStack;
    WOW64_POINTER(PVOID) TlsSlots[64];
    LIST_ENTRY32 TlsLinks;
} TEB32, *PTEB32;

// Conversion

FORCEINLINE VOID UStr32ToUStr(
    __out PUNICODE_STRING Destination,
    __in PUNICODE_STRING32 Source
    )
{
    Destination->Length = Source->Length;
    Destination->MaximumLength = Source->MaximumLength;
    Destination->Buffer = (PWCH)UlongToPtr(Source->Buffer);
}

FORCEINLINE VOID UStrToUStr32(
    __out PUNICODE_STRING32 Destination,
    __in PUNICODE_STRING Source
    )
{
    Destination->Length = Source->Length;
    Destination->MaximumLength = Source->MaximumLength;
    Destination->Buffer = PtrToUlong(Source->Buffer);
}

#endif
