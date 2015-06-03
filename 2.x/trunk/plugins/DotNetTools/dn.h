#ifndef DN_H
#define DN_H

#include <phdk.h>
#include <winperf.h>

extern PPH_PLUGIN PluginInstance;

#define PLUGIN_NAME L"ProcessHacker.DotNetTools"
#define SETTING_NAME_ASM_TREE_LIST_COLUMNS (PLUGIN_NAME L".AsmTreeListColumns")

typedef struct _THREAD_TREE_CONTEXT;

typedef struct _DN_THREAD_ITEM
{
    PPH_THREAD_ITEM ThreadItem;

    BOOLEAN ClrDataValid;
    PPH_STRING AppDomainText;
} DN_THREAD_ITEM, *PDN_THREAD_ITEM;

// counters

typedef struct _PERF_OBJECT_TYPE_INFO
{
    ULONG NameIndex;
    PH_STRINGREF Name;
    PPH_STRING NameBuffer;
} PERF_OBJECT_TYPE_INFO, *PPERF_OBJECT_TYPE_INFO;

BOOLEAN QueryPerfInfoVariableSize(
    _In_ HKEY Key,
    _In_ PWSTR ValueName,
    _Out_ PVOID *Data,
    _Out_opt_ PULONG DataSize
    );

PWSTR FindPerfTextInTextData(
    _In_ PVOID TextData,
    _In_ ULONG Index
    );

ULONG FindPerfIndexInTextData(
    _In_ PVOID TextData,
    _In_ PPH_STRINGREF Text
    );

BOOLEAN GetPerfObjectTypeInfo(
    _In_opt_ PPH_STRINGREF Filter,
    _Out_ PPERF_OBJECT_TYPE_INFO *Info,
    _Out_ PULONG Count
    );

BOOLEAN GetPerfObjectTypeInfo2(
    _In_ PPH_STRINGREF NameList,
    _Out_ PPERF_OBJECT_TYPE_INFO *Info,
    _Out_ PULONG Count,
    _Out_opt_ PVOID *TextData
    );

// asmpage

VOID AddAsmPageToPropContext(
    _In_ PPH_PLUGIN_PROCESS_PROPCONTEXT PropContext
    );

// perfpage

VOID AddPerfPageToPropContext(
    _In_ PPH_PLUGIN_PROCESS_PROPCONTEXT PropContext
    );

// stackext

VOID ProcessThreadStackControl(
    _In_ PPH_PLUGIN_THREAD_STACK_CONTROL Control
    );

VOID PredictAddressesFromClrData(
    _In_ struct _CLR_PROCESS_SUPPORT *Support,
    _In_ HANDLE ThreadId,
    _In_ PVOID PcAddress,
    _In_ PVOID FrameAddress,
    _In_ PVOID StackAddress,
    _Out_ PVOID *PredictedEip,
    _Out_ PVOID *PredictedEbp,
    _Out_ PVOID *PredictedEsp
    );

// svcext

VOID DispatchPhSvcRequest(
    _In_ PVOID Parameter
    );

// treeext

VOID InitializeTreeNewObjectExtensions(
    VOID
    );

VOID DispatchTreeNewMessage(
    __in PVOID Parameter
    );

#define DNTHTNC_APPDOMAIN 1

VOID ThreadTreeNewInitializing(
    __in PVOID Parameter
    );

VOID ThreadTreeNewUninitializing(
    __in PVOID Parameter
    );

#endif
