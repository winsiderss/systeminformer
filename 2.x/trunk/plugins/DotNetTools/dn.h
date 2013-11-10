#ifndef DN_H
#define DN_H

#include <phdk.h>
#include <winperf.h>

extern PPH_PLUGIN PluginInstance;

#define SETTING_PREFIX L"ProcessHacker.DotNetTools."
#define SETTING_NAME_ASM_TREE_LIST_COLUMNS (SETTING_PREFIX L"AsmTreeListColumns")

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

#endif
