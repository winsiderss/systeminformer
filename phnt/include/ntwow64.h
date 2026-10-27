/*
 * Windows on Windows support functions
 *
 * This file is part of System Informer.
 */

#ifndef _NTWOW64_H
#define _NTWOW64_H

#if (PHNT_MODE != PHNT_MODE_KERNEL)
#ifdef __has_include
#if __has_include (<ntxcapi.h>)
#include <ntxcapi.h>
#endif // __has_include
#endif // __has_include
#endif // (PHNT_MODE != PHNT_MODE_KERNEL)

#define WOW64_SYSTEM_DIRECTORY "SysWOW64"
#define WOW64_SYSTEM_DIRECTORY_U L"SysWOW64"
#define WOW64_X86_TAG " (x86)"
#define WOW64_X86_TAG_U L" (x86)"

// Indices of the WOW64 ntdll entry-point array. Historically stored in USER_SHARED_DATA;
// current systems expose the array through LdrSystemDllInitBlock.Wow64SharedInformation.
typedef enum _WOW64_SHARED_INFORMATION
{
    SharedNtdll32LdrInitializeThunk,
    SharedNtdll32KiUserExceptionDispatcher,
    SharedNtdll32KiUserApcDispatcher,
    SharedNtdll32KiUserCallbackDispatcher,
    SharedNtdll32RtlUserThreadStart,
    SharedNtdll32pQueryProcessDebugInformationRemote,
    SharedNtdll32BaseAddress,
    SharedNtdll32LdrSystemDllInitBlock,
    SharedNtdll32RtlpFreezeTimeBias,
    Wow64SharedPageEntriesCount
} WOW64_SHARED_INFORMATION;

//
// 32-bit definitions
//

// Fixed-width guest address or pointer-sized value. Type documents the native counterpart;
// the stored value is always a 32-bit ULONG, even when these declarations are compiled for x64.
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
        UCHAR Red : 1;
        UCHAR Balance : 2;
        WOW64_POINTER(ULONG_PTR) ParentValue;
    };
} RTL_BALANCED_NODE32, *PRTL_BALANCED_NODE32;

typedef struct _RTL_RB_TREE32
{
    WOW64_POINTER(PRTL_BALANCED_NODE) Root;
    union
    {
        UCHAR Encoded : 1;
        WOW64_POINTER(PRTL_BALANCED_NODE) Min;
    };
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
    ULONG LoadWhileUnloadingCount;
    ULONG LowestLink;
    union
    {
        LDRP_CSLIST32 Dependencies;
        SINGLE_LIST_ENTRY32 RemovalLink;
    };
    LDRP_CSLIST32 IncomingDependencies;
    LDR_DDAG_STATE State;
    SINGLE_LIST_ENTRY32 CondenseLink;
    ULONG PreorderNumber;
} LDR_DDAG_NODE32, *PLDR_DDAG_NODE32;

typedef struct _LDRP_DEPENDENCY32
{
    SINGLE_LIST_ENTRY32 Link;
    WOW64_POINTER(PLDR_DDAG_NODE) ChildNode;
    SINGLE_LIST_ENTRY32 BackLink;
    union
    {
        WOW64_POINTER(PLDR_DDAG_NODE) ParentNode;
        struct
        {
            ULONG ForwarderLink : 1;
            ULONG SpareFlags : 2;
        };
    };
} LDRP_DEPENDENCY32, *PLDR_DEPENDENCY32;

#define LDR_DATA_TABLE_ENTRY_SIZE_WINXP_32 FIELD_OFFSET(LDR_DATA_TABLE_ENTRY32, DdagNode)
#define LDR_DATA_TABLE_ENTRY_SIZE_WIN7_32 FIELD_OFFSET(LDR_DATA_TABLE_ENTRY32, BaseNameHashValue)
#define LDR_DATA_TABLE_ENTRY_SIZE_WIN8_32 FIELD_OFFSET(LDR_DATA_TABLE_ENTRY32, ImplicitPathOptions)
#define LDR_DATA_TABLE_ENTRY_SIZE_WIN10_32 FIELD_OFFSET(LDR_DATA_TABLE_ENTRY32, SigningLevel)
#define LDR_DATA_TABLE_ENTRY_SIZE_WIN11_32 sizeof(LDR_DATA_TABLE_ENTRY32)

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
            ULONG VerifierProvider : 1; // 24H2
            ULONG ShimEngineCalloutSent : 1; // 24H2
            ULONG LoadInProgress : 1;
            ULONG LoadConfigProcessed : 1;
            ULONG EntryProcessed : 1;
            ULONG ProtectDelayLoad : 1;
            ULONG AuxIatCopyPrivate : 1; // 24H2
            ULONG ReservedFlags3 : 1;
            ULONG DontCallForThreads : 1;
            ULONG ProcessAttachCalled : 1;
            ULONG ProcessAttachFailed : 1;
            ULONG ScpInExceptionTable : 1; // CorDeferredValidate before 24H2
            ULONG CorImage : 1;
            ULONG DontRelocate : 1;
            ULONG CorILOnly : 1;
            ULONG ChpeImage : 1;
            ULONG ChpeEmulatorImage : 1; // WIN11
            ULONG ReservedFlags5 : 1;
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
    WOW64_POINTER(PVOID) Lock;
    WOW64_POINTER(PLDR_DDAG_NODE) DdagNode;
    LIST_ENTRY32 NodeModuleLink;
    WOW64_POINTER(struct _LDRP_LOAD_CONTEXT *) LoadContext;
    WOW64_POINTER(PVOID) ParentDllBase;
    WOW64_POINTER(PVOID) SwitchBackContext;
    RTL_BALANCED_NODE32 BaseAddressIndexNode;
    RTL_BALANCED_NODE32 MappingInfoIndexNode;
    WOW64_POINTER(ULONG_PTR) OriginalBase;
    LARGE_INTEGER LoadTime;
    ULONG BaseNameHashValue;
    LDR_DLL_LOAD_REASON LoadReason;
    ULONG ImplicitPathOptions;
    ULONG ReferenceCount;
    ULONG DependentLoadFlags;
    UCHAR SigningLevel; // since REDSTONE2
    ULONG CheckSum; // since 22H1
    WOW64_POINTER(PVOID) ActivePatchImageBase;
    LDR_HOT_PATCH_STATE HotPatchState;
} LDR_DATA_TABLE_ENTRY32, *PLDR_DATA_TABLE_ENTRY32;

typedef const LDR_DATA_TABLE_ENTRY32* PCLDR_DATA_TABLE_ENTRY32;

static_assert(UFIELD_OFFSET(LDR_DATA_TABLE_ENTRY32, InMemoryOrderLinks) == 0x8, "LDR_DATA_TABLE_ENTRY32.InMemoryOrderLinks offset incorrect");
static_assert(UFIELD_OFFSET(LDR_DATA_TABLE_ENTRY32, InInitializationOrderLinks) == 0x10, "LDR_DATA_TABLE_ENTRY32.InInitializationOrderLinks offset incorrect");
static_assert(UFIELD_OFFSET(LDR_DATA_TABLE_ENTRY32, DllBase) == 0x18, "LDR_DATA_TABLE_ENTRY32.DllBase offset incorrect");
static_assert(UFIELD_OFFSET(LDR_DATA_TABLE_ENTRY32, EntryPoint) == 0x1c, "LDR_DATA_TABLE_ENTRY32.EntryPoint offset incorrect");
static_assert(UFIELD_OFFSET(LDR_DATA_TABLE_ENTRY32, SizeOfImage) == 0x20, "LDR_DATA_TABLE_ENTRY32.SizeOfImage offset incorrect");
static_assert(UFIELD_OFFSET(LDR_DATA_TABLE_ENTRY32, ObsoleteLoadCount) == 0x38, "LDR_DATA_TABLE_ENTRY32.ObsoleteLoadCount offset incorrect");
static_assert(UFIELD_OFFSET(LDR_DATA_TABLE_ENTRY32, TimeDateStamp) == 0x44, "LDR_DATA_TABLE_ENTRY32.TimeDateStamp offset incorrect");
static_assert(UFIELD_OFFSET(LDR_DATA_TABLE_ENTRY32, DdagNode) == 0x50, "LDR_DATA_TABLE_ENTRY32.DdagNode offset incorrect");
static_assert(UFIELD_OFFSET(LDR_DATA_TABLE_ENTRY32, ParentDllBase) == 0x60, "LDR_DATA_TABLE_ENTRY32.ParentDllBase offset incorrect");
static_assert(UFIELD_OFFSET(LDR_DATA_TABLE_ENTRY32, OriginalBase) == 0x80, "LDR_DATA_TABLE_ENTRY32.OriginalBase offset incorrect");
static_assert(UFIELD_OFFSET(LDR_DATA_TABLE_ENTRY32, BaseNameHashValue) == 0x90, "LDR_DATA_TABLE_ENTRY32.BaseNameHashValue offset incorrect");
static_assert(UFIELD_OFFSET(LDR_DATA_TABLE_ENTRY32, LoadReason) == 0x94, "LDR_DATA_TABLE_ENTRY32.LoadReason offset incorrect");
static_assert(UFIELD_OFFSET(LDR_DATA_TABLE_ENTRY32, CheckSum) == 0xA8, "LDR_DATA_TABLE_ENTRY32.CheckSum offset incorrect");
static_assert(sizeof(LDR_DATA_TABLE_ENTRY32) == 0xB8, "LDR_DATA_TABLE_ENTRY32 incorrect size");

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

/**
 * 32-bit process parameters. The layout through AttributeList occupies 0x2c8 bytes.
 * Consult Length before reading fields that may be absent on the target OS.
 */
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

    WOW64_POINTER(ULONG_PTR) EnvironmentSize;
    WOW64_POINTER(ULONG_PTR) EnvironmentVersion;
    WOW64_POINTER(PVOID) PackageDependencyData;
    ULONG ProcessGroupId;
    ULONG LoaderThreads;

    UNICODE_STRING32 RedirectionDllName; // REDSTONE4
    UNICODE_STRING32 HeapPartitionName; // 19H1
    WOW64_POINTER(PULONGLONG) DefaultThreadpoolCpuSetMasks;
    ULONG DefaultThreadpoolCpuSetMaskCount;
    ULONG DefaultThreadpoolThreadMaximum;
    ULONG HeapMemoryTypeMask; // WIN11 22H2
    WOW64_POINTER(PVOID) AttributeList;
} RTL_USER_PROCESS_PARAMETERS32, *PRTL_USER_PROCESS_PARAMETERS32;

typedef struct _LEAP_SECOND_DATA *PLEAP_SECOND_DATA;

/**
 * 32-bit PEB layout, including Win32ClientProcInfo (0x490 bytes).
 * This is a version-dependent remote-memory layout; its size is not a version-independent ABI.
 */
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
            BOOLEAN IsImageDynamicallyRelocated : 1;
            BOOLEAN SkipPatchingUser32Forwarders : 1;
            BOOLEAN IsPackagedProcess : 1;
            BOOLEAN IsAppContainer : 1;
            BOOLEAN IsProtectedProcessLight : 1;
            BOOLEAN IsLongPathAwareProcess : 1;
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
            ULONG ProcessPreviouslyThrottled : 1;   // The process was previously throttled.
            ULONG ProcessCurrentlyThrottled : 1;    // The process is currently throttled.
            ULONG ProcessImagesHotPatched : 1;      // The process images are hot patched. // RS5
            ULONG ReservedBits0 : 24;
        };
    };
    union
    {
        WOW64_POINTER(PVOID) KernelCallbackTable;
        WOW64_POINTER(PVOID) UserSharedInfoPtr;
    };
    ULONG SystemReserved;
    ULONG AtlThunkSListPtr32;
    WOW64_POINTER(PVOID) ApiSetMap;
    ULONG TlsExpansionCounter;
    WOW64_POINTER(PVOID) TlsBitmap;
    ULONG TlsBitmapBits[2];
    WOW64_POINTER(PVOID) ReadOnlySharedMemoryBase;
    WOW64_POINTER(PVOID) SharedData;
    WOW64_POINTER(PVOID *) ReadOnlyStaticServerData;
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
    WOW64_POINTER(PVOID *) ProcessHeaps;

    WOW64_POINTER(PVOID) GdiSharedHandleTable;
    WOW64_POINTER(PPS_PROCESS_START_ROUTINE) ProcessStarterHelper;
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
    WOW64_POINTER(ULONG_PTR) ActiveProcessAffinityMask;
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

    WOW64_POINTER(PACTIVATION_CONTEXT_DATA) ActivationContextData;
    WOW64_POINTER(PVOID) ProcessAssemblyStorageMap;
    WOW64_POINTER(PACTIVATION_CONTEXT_DATA) SystemDefaultActivationContextData;
    WOW64_POINTER(PVOID) SystemAssemblyStorageMap;

    WOW64_POINTER(SIZE_T) MinimumStackCommit;

    WOW64_POINTER(PVOID) SparePointers[2]; // 19H1 (previously FlsCallback to FlsHighIndex)
    WOW64_POINTER(PLDR_PATCH_TABLE) PatchLoaderData;
    WOW64_POINTER(PVOID) ChpeV2ProcessInfo; // _CHPEV2_PROCESS_INFO

    ULONG AppModelFeatureState;
    ULONG SpareUlongs[2];

    USHORT ActiveCodePage;
    USHORT OemCodePage;
    USHORT UseCaseMapping;
    USHORT UnusedNlsField;

    WOW64_POINTER(PWER_PEB_HEADER_BLOCK) WerRegistrationData;
    WOW64_POINTER(PWER_REGISTRATION_DATA) WerShipAssertPtr;

    union
    {
        WOW64_POINTER(PVOID) pContextData; // WIN7
        WOW64_POINTER(PVOID) pUnused; // WIN10
        WOW64_POINTER(PVOID) EcCodeBitMap; // WIN11
        WOW64_POINTER(PVOID) Spare;
    };

    WOW64_POINTER(PVOID) pImageHeaderHash;
    union
    {
        ULONG TracingFlags;
        struct
        {
            ULONG HeapTracingEnabled : 1;
            ULONG CritSecTracingEnabled : 1;
            ULONG LibLoaderTracingEnabled : 1;
            ULONG SpareTracingBits : 29;
        };
    };
    ULONGLONG CsrServerReadOnlySharedMemoryBase;
    WOW64_POINTER(PVOID) TppWorkerpListLock;
    LIST_ENTRY32 TppWorkerpList;
    WOW64_POINTER(PVOID) WaitOnAddressHashTable[128];
    WOW64_POINTER(PVOID) TelemetryCoverageHeader; // REDSTONE3
    ULONG CloudFileFlags;
    ULONG CloudFileDiagFlags; // REDSTONE4
    CHAR PlaceholderCompatibilityMode;
    CHAR PlaceholderCompatibilityModeReserved[7];
    WOW64_POINTER(PLEAP_SECOND_DATA) LeapSecondData; // REDSTONE5
    union
    {
        ULONG LeapSecondFlags;
        struct
        {
            ULONG SixtySecondEnabled : 1;
            ULONG Reserved : 31;
        };
    };
    ULONG NtGlobalFlag2;
    ULONGLONG ExtendedFeatureDisableMask; // since WIN11
    WOW64_POINTER(PCLIENTPROCINFO) Win32ClientProcInfo; // Optional 32-bit address of CLIENTPROCINFO (ntpebteb.h).
} PEB32, *PPEB32;

static_assert(UFIELD_OFFSET(PEB32, Win32ClientProcInfo) == 0x488, "PEB32.Win32ClientProcInfo offset incorrect");
static_assert(sizeof(PEB32) == 0x490, "sizeof(PEB32) is incorrect");

// Note: Use PhGetProcessPeb32 instead. (dmex)
//#define WOW64_GET_PEB32(peb64) \
//    ((PPEB32)RtlOffsetToPointer((peb64), ALIGN_UP_BY(sizeof(PEB), PAGE_SIZE)))

#define GDI_BATCH_BUFFER_SIZE 310

typedef struct _GDI_TEB_BATCH32
{
    ULONG Offset : 30;
    ULONG InProcessing : 1;
    ULONG HasRenderingCommand : 1;
    WOW64_POINTER(ULONG_PTR) HDC;
    ULONG Buffer[GDI_BATCH_BUFFER_SIZE];
} GDI_TEB_BATCH32, *PGDI_TEB_BATCH32;

typedef struct tagSOleTlsData32
{
    WOW64_POINTER(PVOID) ThreadBase;
    WOW64_POINTER(PVOID) SmAllocator;
    ULONG ApartmentID;
    ULONG Flags; // OLETLSFLAGS
    LONG TlsMapIndex;
    WOW64_POINTER(PVOID *) TlsSlot;
    ULONG ComInits;
    ULONG OleInits;
    ULONG Calls;
    WOW64_POINTER(PVOID) ServerCall; // was CallInfo before TH1
    WOW64_POINTER(PVOID) CallObjectCache; // was FreeAsyncCall before TH1
    WOW64_POINTER(PVOID) ContextStack; // was FreeClientCall before TH1
    WOW64_POINTER(PVOID) ObjServer;
    ULONG TIDCaller;
    // ... (other fields are version-dependant)
} SOleTlsData32, *PSOleTlsData32;

// rev
#define RPC_THREAD_POINTER_KEY32 0xABABABAB

typedef struct _ACTIVATION_CONTEXT_STACK32
{
    WOW64_POINTER(PRTL_ACTIVATION_CONTEXT_STACK_FRAME) ActiveFrame;
    LIST_ENTRY32 FrameListCache;
    ULONG Flags; // ACTIVATION_CONTEXT_STACK_FLAG_*
    ULONG NextCookieSequenceNumber;
    ULONG StackId;
} ACTIVATION_CONTEXT_STACK32, *PACTIVATION_CONTEXT_STACK32;

/**
 * 32-bit TEB layout, including the extended scheduler and RCU fields (0x1038 bytes).
 * WowTebOffset is a signed byte displacement from this guest TEB to the host TEB.
 * Fields after EffectiveContainerId extend beyond the older 0x1000-byte layout.
 */
typedef struct _TEB32
{
    NT_TIB32 NtTib;

    WOW64_POINTER(PVOID) EnvironmentPointer;
    CLIENT_ID32 ClientId;
    WOW64_POINTER(PVOID) ActiveRpcHandle;
    WOW64_POINTER(PVOID *) ThreadLocalStoragePointer;
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
    WOW64_POINTER(PVOID) ReservedForDebuggerInstrumentation[16];
    WOW64_POINTER(PVOID) SystemReserved1[21];
    WOW64_POINTER(PVOID) HeapFlsData;
    WOW64_POINTER(ULONG_PTR) RngState[4];
    CHAR PlaceholderCompatibilityMode;
    BOOLEAN PlaceholderHydrationAlwaysExplicit;
    CHAR PlaceholderReserved[10];
    ULONG ProxiedProcessId;
    ACTIVATION_CONTEXT_STACK32 ActivationStack;
    ALPC_WORK_ON_BEHALF_TICKET WorkingOnBehalfTicket;
    NTSTATUS ExceptionCode;

    WOW64_POINTER(PVOID) ActivationContextStackPointer;
    WOW64_POINTER(ULONG_PTR) InstrumentationCallbackSp;
    WOW64_POINTER(ULONG_PTR) InstrumentationCallbackPreviousPc;
    WOW64_POINTER(ULONG_PTR) InstrumentationCallbackPreviousSp;
    BOOLEAN InstrumentationCallbackDisabled;
    UCHAR SpareBytes[23];
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

    WOW64_POINTER(PVOID) Vdm;
    WOW64_POINTER(PVOID) ReservedForNtRpc;
    WOW64_POINTER(PVOID) DbgSsReserved[2];

    ULONG HardErrorMode;
    WOW64_POINTER(PVOID) Instrumentation[9];
    GUID ActivityId;

    WOW64_POINTER(PVOID) SubProcessTag;
    WOW64_POINTER(PVOID) PerflibData;
    WOW64_POINTER(PVOID) EtwTraceData;
    WOW64_POINTER(PVOID) WinSockData;
    ULONG GdiBatchCount;

    union
    {
        PROCESSOR_NUMBER CurrentIdealProcessor;
        ULONG IdealProcessorValue;
        struct
        {
            UCHAR ReservedPad0;
            UCHAR ReservedPad1;
            UCHAR ReservedPad2;
            UCHAR IdealProcessor;
        };
    };

    ULONG GuaranteedStackBytes;
    WOW64_POINTER(PVOID) ReservedForPerf;
    WOW64_POINTER(PSOleTlsData32) ReservedForOle;
    ULONG WaitingOnLoaderLock;
    WOW64_POINTER(PVOID) SavedPriorityState;
    WOW64_POINTER(ULONG_PTR) ReservedForCodeCoverage;
    WOW64_POINTER(PTPP_THREAD_DATA) ThreadPoolData;
    WOW64_POINTER(PVOID *) TlsExpansionSlots;

    ULONG MuiGeneration;
    ULONG IsImpersonating;
    WOW64_POINTER(PVOID) NlsCache;
    WOW64_POINTER(PVOID) pShimData;
    union
    {
        ULONG HeapData;
        struct
        {
            USHORT HeapVirtualAffinity; // legacy layout
            USHORT LowFragHeapDataSlot;
        };
    };
    WOW64_POINTER(HANDLE) CurrentTransactionHandle;
    WOW64_POINTER(PTEB_ACTIVE_FRAME) ActiveFrame;
    WOW64_POINTER(PVOID) FlsData;

    WOW64_POINTER(PVOID) PreferredLanguages;
    WOW64_POINTER(PVOID) UserPrefLanguages;
    WOW64_POINTER(PVOID) MergedPrefLanguages;
    ULONG MuiImpersonation;

    union
    {
        USHORT CrossTebFlags;
        USHORT SpareCrossTebBits : 16;
    };
    union
    {
        USHORT SameTebFlags;
        struct
        {
            USHORT SafeThunkCall : 1;
            USHORT InDebugPrint : 1;
            USHORT HasFiberData : 1;
            USHORT SkipThreadAttach : 1;
            USHORT WerInShipAssertCode : 1;
            USHORT RanProcessInit : 1;
            USHORT ClonedThread : 1;
            USHORT SuppressDebugMsg : 1;
            USHORT DisableUserStackWalk : 1;
            USHORT RtlExceptionAttached : 1;
            USHORT InitialThread : 1;
            USHORT SessionAware : 1;
            USHORT LoadOwner : 1;
            USHORT LoaderWorker : 1;
            USHORT SkipLoaderInit : 1;
            USHORT SkipFileAPIBrokering : 1;
        };
    };

    WOW64_POINTER(PVOID) TxnScopeEnterCallback;
    WOW64_POINTER(PVOID) TxnScopeExitCallback;
    WOW64_POINTER(PVOID) TxnScopeContext;
    ULONG LockCount;
    LONG WowTebOffset;
    WOW64_POINTER(PVOID) ResourceRetValue;
    WOW64_POINTER(PVOID) ReservedForWdf;
    ULONGLONG ReservedForCrt;
    GUID EffectiveContainerId;
    ULONGLONG LastSleepCounter; // WIN11
    ULONG SpinCallCount;
    ULONGLONG ExtendedFeatureDisableMask;
    WOW64_POINTER(PVOID) SchedulerSharedDataSlot; // 24H2
    WOW64_POINTER(PVOID) HeapWalkContext;
    GROUP_AFFINITY32 PrimaryGroupAffinity;
    ULONG Rcu[2];
} TEB32, *PTEB32;

static_assert(FIELD_OFFSET(TEB32, ProcessEnvironmentBlock) == 0x030, "FIELD_OFFSET(TEB32, ProcessEnvironmentBlock) is incorrect");
static_assert(FIELD_OFFSET(TEB32, ExceptionCode) == 0x1a4, "FIELD_OFFSET(TEB32, ExceptionCode) is incorrect");
static_assert(FIELD_OFFSET(TEB32, StaticUnicodeBuffer) == 0xc00, "FIELD_OFFSET(TEB32, StaticUnicodeBuffer) is incorrect");
static_assert(FIELD_OFFSET(TEB32, TlsLinks) == 0xf10, "FIELD_OFFSET(TEB32, TlsLinks) is incorrect");
static_assert(FIELD_OFFSET(TEB32, TlsExpansionSlots) == 0xf94, "FIELD_OFFSET(TEB32, TlsExpansionSlots) is incorrect");
static_assert(FIELD_OFFSET(TEB32, FlsData) == 0xfb4, "FIELD_OFFSET(TEB32, FlsData) is incorrect");
static_assert(FIELD_OFFSET(TEB32, MuiImpersonation) == 0xfc4, "FIELD_OFFSET(TEB32, MuiImpersonation) is incorrect");
static_assert(FIELD_OFFSET(TEB32, EffectiveContainerId) == 0xff0, "FIELD_OFFSET(TEB32, EffectiveContainerId) is incorrect");
static_assert(UFIELD_OFFSET(TEB32, ActivationStack) == 0x184, "TEB32.ActivationStack offset incorrect");
static_assert(UFIELD_OFFSET(TEB32, WowTebOffset) == 0xfdc, "TEB32.WowTebOffset offset incorrect");
static_assert(UFIELD_OFFSET(TEB32, SchedulerSharedDataSlot) == 0x1018, "TEB32.SchedulerSharedDataSlot offset incorrect");
static_assert(UFIELD_OFFSET(TEB32, Rcu) == 0x102c, "TEB32.Rcu offset incorrect");
static_assert(sizeof(TEB32) == 0x1038, "sizeof(TEB32) is incorrect");

//
// Conversion
//

/**
 * The UStr32ToUStr routine converts a 32-bit UNICODE_STRING32 descriptor to a native descriptor.
 * It zero-extends the buffer address; it does not copy or validate the string or read remote memory.
 * Length and MaximumLength remain byte counts.
 *
 * \param Destination A pointer to the target UNICODE_STRING structure.
 * \param Source A pointer to the source UNICODE_STRING32 structure.
 */
FORCEINLINE VOID UStr32ToUStr(
    _Out_ PUNICODE_STRING Destination,
    _In_ PCUNICODE_STRING32 Source
    )
{
    Destination->Length = Source->Length;
    Destination->MaximumLength = Source->MaximumLength;
    Destination->Buffer = (PWCH)UlongToPtr(Source->Buffer);
}

/**
 * The UStrToUStr32 routine converts a native UNICODE_STRING descriptor to a 32-bit descriptor.
 * It truncates the buffer address to 32 bits without checking whether it fits. The caller must
 * ensure the address is representable. No string data is copied; lengths remain byte counts.
 *
 * \param Destination A pointer to the target UNICODE_STRING32 structure.
 * \param Source A pointer to the source UNICODE_STRING structure.
 */
FORCEINLINE VOID UStrToUStr32(
    _Out_ PUNICODE_STRING32 Destination,
    _In_ PCUNICODE_STRING Source
    )
{
    Destination->Length = Source->Length;
    Destination->MaximumLength = Source->MaximumLength;
    Destination->Buffer = PtrToUlong(Source->Buffer);
}

// WOW64INFO follows the guest PEB32, not the TEB32, and is shared by the guest and host modules.
// from SDK/10.0.10240.0/um/minwin/wow64t.h (dmex)
//
// Page size on x86 NT
//
#define PAGE_SIZE_X86NT 0x1000
#define PAGE_SHIFT_X86NT 12L
#define WOW64_SPLITS_PER_PAGE (PAGE_SIZE_X86NT / PAGE_SIZE_X86NT)

//
// Convert native pages to x86 pages for the 4-KB native-page configuration used by these helpers.
//
#define Wow64GetNumberOfX86Pages(NativePages) \
    ((NativePages) * (PAGE_SIZE_X86NT >> PAGE_SHIFT_X86NT))

//
// Round a byte count up to a 4-KB boundary. The addition must not overflow ULONG_PTR.
//
#define WOW64_ROUND_TO_PAGES(Size) \
    (((ULONG_PTR)(Size) + PAGE_SIZE_X86NT - 1) & ~((ULONG_PTR)PAGE_SIZE_X86NT - 1))

//
// Get the number of 4-KB x86 pages needed for a byte count (zero bytes requires zero pages).
//
#define WOW64_BYTES_TO_PAGES(Size) \
    (((ULONG_PTR)(Size) >> PAGE_SHIFT_X86NT) + (((ULONG_PTR)(Size) & (PAGE_SIZE_X86NT - 1)) != 0))

//
// Get the guest TEB address from a native x64 TEB address without reading target memory.
// This assumes the target uses the adjacent layout matching sizeof(TEB): guest = host + 0x2000
// for the layout declared here. For a locally accessible TEB, use WowTebOffset for the actual displacement.
//
#define WOW64_GET_TEB32(teb64) ((PTEB32)(((ULONG_PTR)(teb64)) + ((ULONG_PTR)WOW64_ROUND_TO_PAGES(sizeof(TEB)))))
#define WOW64_TEB32_POINTER_ADDRESS(teb64) ((PVOID)&(((PTEB)(teb64))->NtTib.ExceptionList))

//
// Get the 32-bit execute options.
//
typedef union _WOW64_EXECUTE_OPTIONS
{
    ULONG Flags;
    struct
    {
        ULONG StackReserveSize : 8;
        ULONG StackCommitSize : 4;
        ULONG Deprecated0 : 1;
        ULONG DisableWowAssert : 1;
        ULONG DisableTurboDispatch : 1;
        ULONG Unused : 13;
        ULONG Reserved0 : 1;
        ULONG Reserved1 : 1;
        ULONG Reserved2 : 1;
        ULONG Reserved3 : 1;
    };
} WOW64_EXECUTE_OPTIONS, *PWOW64_EXECUTE_OPTIONS;

#define WOW64_CPUFLAGS_MSFT64           0x00000001
#define WOW64_CPUFLAGS_SOFTWARE         0x00000002
#define WOW64_CPUFLAGS_IA64             0x00000004

// rev
/**
 * WOW64 process-wide shared information. This 40-byte layout follows the PEB32
 * declared here at offset 0x490; the location is implementation-dependent.
 * The handle and work-list address are 64-bit values on both compilation targets.
 * NativeMachineType and EmulatedMachineType use IMAGE_FILE_MACHINE_* values.
 */
typedef struct _WOW64INFO
{
    ULONG NativeSystemPageSize;
    ULONG CpuFlags;
    WOW64_EXECUTE_OPTIONS Wow64ExecuteFlags;
    ULONG InstrumentationCallback; // 32-bit guest callback address
    ULONGLONG SectionHandle;
    ULONGLONG CrossProcessWorkList;
    USHORT NativeMachineType;
    USHORT EmulatedMachineType;
} WOW64INFO, *PWOW64INFO;

typedef struct _PEB32_WITH_WOW64INFO
{
    PEB32 Peb32;
    WOW64INFO Wow64Info;
} PEB32_WITH_WOW64INFO, *PPEB32_WITH_WOW64INFO;

static_assert(sizeof(WOW64INFO) == 0x28, "sizeof(WOW64INFO) is incorrect");
static_assert(UFIELD_OFFSET(PEB32_WITH_WOW64INFO, Wow64Info) == 0x490, "PEB32_WITH_WOW64INFO.Wow64Info offset incorrect");

#if (PHNT_MODE != PHNT_MODE_KERNEL)
#ifdef _M_X64

/**
 * The Wow64CurrentGuestTeb routine retrieves a pointer to the 32-bit TEB (guest TEB) for a thread executing under WOW64.
 *
 * \return A pointer to the 32-bit TEB32 structure, or NULL if the thread is not running under or over WOW64.
 */
FORCEINLINE
TEB32*
POINTER_UNSIGNED
Wow64CurrentGuestTeb(
    VOID
    )
{
    TEB* POINTER_UNSIGNED Teb;
    TEB32* POINTER_UNSIGNED Teb32;

    Teb = NtCurrentTeb();

    if (Teb->WowTebOffset == 0)
    {
        //
        // Not running under or over WoW, so there is no "guest teb"
        //

        return NULL;
    }

    if (Teb->WowTebOffset < 0)
    {
        //
        // Was called while running under WoW. The current teb is the guest teb.
        //

        Teb32 = (PTEB32)Teb;

#if defined(RTL_ASSERT)
        RTL_ASSERT(&Teb32->WowTebOffset == &Teb->WowTebOffset);
#endif
    }
    else
    {
        //
        // Called by the WoW Host, so calculate the position of the guest teb
        // relative to the current (host) teb.
        //

        Teb32 = (PTEB32)RtlOffsetToPointer(Teb, Teb->WowTebOffset);
    }

#if defined(RTL_ASSERT)
    RTL_ASSERT(Teb32->NtTib.Self == PtrToUlong(Teb32));
#endif

    return Teb32;
}

/**
 * The Wow64CurrentNativeTeb routine retrieves a pointer to the native (host) TEB for the current thread.
 *
 * \return A pointer to the native TEB structure.
 */
FORCEINLINE
VOID*
POINTER_UNSIGNED
Wow64CurrentNativeTeb(
    VOID
    )
{
    TEB* POINTER_UNSIGNED Teb;
    VOID* POINTER_UNSIGNED HostTeb;

    Teb = NtCurrentTeb();

    if (Teb->WowTebOffset >= 0)
    {
        //
        // Not running under WoW, so it is either not running on WoW at all, or
        // it is the host. Return the current teb as native teb.
        //

        HostTeb = (PVOID)Teb;
    }
    else
    {
        //
        // Called while running as the WoW guest, so calculate the position of the
        // host teb relative to the current (guest) teb.
        //

        HostTeb = (PVOID)RtlOffsetToPointer(Teb, Teb->WowTebOffset);
    }

#if defined(RTL_ASSERT)
    RTL_ASSERT((((PTEB32)HostTeb)->NtTib.Self == PtrToUlong(HostTeb)) || ((ULONG_PTR)((PTEB)HostTeb)->NtTib.Self == (ULONG_PTR)HostTeb));
#endif

    return HostTeb;
}

#define NtCurrentTeb32() (Wow64CurrentGuestTeb())

/**
 * Returns the current guest PEB, or NULL when the current thread has no guest TEB.
 */
FORCEINLINE
PPEB32
Wow64CurrentGuestPeb(
    VOID
    )
{
    PTEB32 teb32 = Wow64CurrentGuestTeb();

    return teb32 ? (PPEB32)UlongToPtr(teb32->ProcessEnvironmentBlock) : NULL;
}

#define NtCurrentPeb32() (Wow64CurrentGuestPeb())

// Access common scalar fields of a locally accessible native or guest TEB, not a remote-memory copy.
// Arguments used as the TEB expression must not have side effects.
#define Wow64GetNativeTebField(teb, field) \
    (((ULONG_PTR)(teb) == (ULONG_PTR)((PTEB32)(teb))->NtTib.Self) ? ((PTEB32)(teb))->field : ((PTEB)(teb))->field)
#define Wow64SetNativeTebField(teb, field, value) \
    do { \
        if ((ULONG_PTR)(teb) == (ULONG_PTR)((PTEB32)(teb))->NtTib.Self) \
            ((PTEB32)(teb))->field = (value); \
        else \
            ((PTEB)(teb))->field = (value); \
    } while (0)

#endif // _M_X64
#endif // (PHNT_MODE != PHNT_MODE_KERNEL)

#endif // _NTWOW64_H
