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
    __in HKEY Key,
    __in PWSTR ValueName,
    __out PVOID *Data,
    __out_opt PULONG DataSize
    );

PWSTR FindPerfTextInTextData(
    __in PVOID TextData,
    __in ULONG Index
    );

ULONG FindPerfIndexInTextData(
    __in PVOID TextData,
    __in PPH_STRINGREF Text
    );

BOOLEAN GetPerfObjectTypeInfo(
    __in_opt PPH_STRINGREF Filter,
    __out PPERF_OBJECT_TYPE_INFO *Info,
    __out PULONG Count
    );

BOOLEAN GetPerfObjectTypeInfo2(
    __in PPH_STRINGREF NameList,
    __out PPERF_OBJECT_TYPE_INFO *Info,
    __out PULONG Count,
    __out_opt PVOID *TextData
    );

// asmpage

VOID AddAsmPageToPropContext(
    __in PPH_PLUGIN_PROCESS_PROPCONTEXT PropContext
    );

// perfpage

VOID AddPerfPageToPropContext(
    __in PPH_PLUGIN_PROCESS_PROPCONTEXT PropContext
    );

#endif
