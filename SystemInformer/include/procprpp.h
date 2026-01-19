/*
 * Copyright (c) 2022 Winsider Seminars & Solutions, Inc.  All rights reserved.
 *
 * This file is part of System Informer.
 *
 * Authors:
 *
 *     wj32    2010-2016
 *     dmex    2017-2023
 *
 */

#ifndef PH_PROCPRPP_H
#define PH_PROCPRPP_H

#include <phuisup.h>
#include <colmgr.h>

#include <thrdlist.h>
#include <modlist.h>
#include <hndllist.h>
#include <memlist.h>
#include <memprv.h>

typedef struct _PH_PROCESS_PROPSHEETCONTEXT
{
    WNDPROC PropSheetWindowHookProc;
    PH_LAYOUT_MANAGER LayoutManager;
    PPH_LAYOUT_ITEM TabPageItem;
    BOOLEAN LayoutInitialized;
    HFONT PropSheetWindowFont;
    PH_CALLBACK_REGISTRATION ProcessesUpdatedRegistration;
    HWND OptionsButtonWindowHandle;
    WNDPROC OldOptionsButtonWndProc;
    //HWND PermissionsButtonWindowHandle;
    HWND ButtonsLabelWindowHandle;
} PH_PROCESS_PROPSHEETCONTEXT, *PPH_PROCESS_PROPSHEETCONTEXT;

_Function_class_(PH_TYPE_DELETE_PROCEDURE)
VOID NTAPI PhpProcessPropContextDeleteProcedure(
    _In_ PVOID Object,
    _In_ ULONG Flags
    );

INT CALLBACK PhpPropSheetProc(
    _In_ HWND hwndDlg,
    _In_ UINT uMsg,
    _In_ LPARAM lParam
    );

PPH_PROCESS_PROPSHEETCONTEXT PhpGetPropSheetContext(
    _In_ HWND hwnd
    );

LRESULT CALLBACK PhpPropSheetWndProc(
    _In_ HWND hwnd,
    _In_ UINT uMsg,
    _In_ WPARAM wParam,
    _In_ LPARAM lParam
    );

_Function_class_(PH_TYPE_DELETE_PROCEDURE)
VOID NTAPI PhpProcessPropPageContextDeleteProcedure(
    _In_ PVOID Object,
    _In_ ULONG Flags
    );

UINT CALLBACK PhpStandardPropPageProc(
    _In_ HWND hwnd,
    _In_ UINT uMsg,
    _In_ LPPROPSHEETPAGE ppsp
    );

_Function_class_(PH_TYPE_DELETE_PROCEDURE)
VOID NTAPI PhpProcessPropPageWaitContextDeleteProcedure(
    _In_ PVOID Object,
    _In_ ULONG Flags
    );

VOID PhpCreateProcessPropSheetWaitContext(
    _In_ PPH_PROCESS_PROPCONTEXT PropContext,
    _In_ HWND WindowHandle
    );

VOID PhpFlushProcessPropSheetWaitContextData(
    VOID
    );

_Function_class_(WAIT_CALLBACK_ROUTINE)
VOID NTAPI PhpProcessPropertiesWaitCallback(
    _In_ PVOID Context,
    _In_ BOOLEAN TimerOrWaitFired
    );

#define SET_BUTTON_ICON(Id, Icon) \
    SendMessage(GetDlgItem(hwndDlg, (Id)), BM_SETIMAGE, IMAGE_ICON, (LPARAM)(Icon))

INT_PTR CALLBACK PhpProcessGeneralDlgProc(
    _In_ HWND hwndDlg,
    _In_ UINT uMsg,
    _In_ WPARAM wParam,
    _In_ LPARAM lParam
    );

INT_PTR CALLBACK PhpProcessStatisticsDlgProc(
    _In_ HWND hwndDlg,
    _In_ UINT uMsg,
    _In_ WPARAM wParam,
    _In_ LPARAM lParam
    );

INT_PTR CALLBACK PhpProcessPerformanceDlgProc(
    _In_ HWND hwndDlg,
    _In_ UINT uMsg,
    _In_ WPARAM wParam,
    _In_ LPARAM lParam
    );

INT_PTR CALLBACK PhpProcessThreadsDlgProc(
    _In_ HWND hwndDlg,
    _In_ UINT uMsg,
    _In_ WPARAM wParam,
    _In_ LPARAM lParam
    );

_Function_class_(PH_OPEN_OBJECT)
NTSTATUS NTAPI PhpOpenProcessTokenForPage(
    _Out_ PHANDLE Handle,
    _In_ ACCESS_MASK DesiredAccess,
    _In_opt_ PVOID Context
    );

_Function_class_(PH_CLOSE_OBJECT)
NTSTATUS NTAPI PhpCloseProcessTokenForPage(
    _In_opt_ HANDLE Handle,
    _In_opt_ BOOLEAN Release,
    _In_opt_ PVOID Context
    );

INT_PTR CALLBACK PhpProcessTokenHookProc(
    _In_ HWND hwndDlg,
    _In_ UINT uMsg,
    _In_ WPARAM wParam,
    _In_ LPARAM lParam
    );

INT_PTR CALLBACK PhpProcessModulesDlgProc(
    _In_ HWND hwndDlg,
    _In_ UINT uMsg,
    _In_ WPARAM wParam,
    _In_ LPARAM lParam
    );

INT_PTR CALLBACK PhpProcessMemoryDlgProc(
    _In_ HWND hwndDlg,
    _In_ UINT uMsg,
    _In_ WPARAM wParam,
    _In_ LPARAM lParam
    );

INT_PTR CALLBACK PhpProcessEnvironmentDlgProc(
    _In_ HWND hwndDlg,
    _In_ UINT uMsg,
    _In_ WPARAM wParam,
    _In_ LPARAM lParam
    );

INT_PTR CALLBACK PhpProcessHandlesDlgProc(
    _In_ HWND hwndDlg,
    _In_ UINT uMsg,
    _In_ WPARAM wParam,
    _In_ LPARAM lParam
    );

_Function_class_(PH_OPEN_OBJECT)
NTSTATUS NTAPI PhpOpenProcessJobForPage(
    _Out_ PHANDLE Handle,
    _In_ ACCESS_MASK DesiredAccess,
    _In_ PVOID Context
    );

_Function_class_(PH_CLOSE_OBJECT)
NTSTATUS NTAPI PhpCloseProcessJobForPage(
    _In_ HANDLE Handle,
    _In_ BOOLEAN Release,
    _In_opt_ PVOID Context
    );

INT_PTR CALLBACK PhpProcessJobHookProc(
    _In_ HWND hwndDlg,
    _In_ UINT uMsg,
    _In_ WPARAM wParam,
    _In_ LPARAM lParam
    );

INT_PTR CALLBACK PhpProcessServicesDlgProc(
    _In_ HWND hwndDlg,
    _In_ UINT uMsg,
    _In_ WPARAM wParam,
    _In_ LPARAM lParam
    );

INT_PTR CALLBACK PhpProcessWmiProvidersDlgProc(
    _In_ HWND hwndDlg,
    _In_ UINT uMsg,
    _In_ WPARAM wParam,
    _In_ LPARAM lParam
    );

#ifdef _M_IX86
INT_PTR CALLBACK PhpProcessVdmHostProcessDlgProc(
    _In_ HWND hwndDlg,
    _In_ UINT uMsg,
    _In_ WPARAM wParam,
    _In_ LPARAM lParam
    );
#endif

extern PH_STRINGREF PhProcessPropPageLoadingText;

#define WM_PH_THREADS_UPDATED (WM_APP + 200)
#define WM_PH_THREAD_SELECTION_CHANGED (WM_APP + 201)

// begin_phapppub
typedef struct _PH_THREADS_CONTEXT
{
    PPH_THREAD_PROVIDER Provider;
    PH_CALLBACK_REGISTRATION ProviderRegistration;
    PH_CALLBACK_REGISTRATION AddedEventRegistration;
    PH_CALLBACK_REGISTRATION ModifiedEventRegistration;
    PH_CALLBACK_REGISTRATION RemovedEventRegistration;
    PH_CALLBACK_REGISTRATION UpdatedEventRegistration;
    PH_CALLBACK_REGISTRATION LoadingStateChangedEventRegistration;

    HWND WindowHandle;
// end_phapppub
    HWND TreeNewHandle;
    HWND StatusHandle;
    HWND SearchboxHandle;
    HFONT TreeNewFont;
    ULONG_PTR SearchMatchHandle;
    PPH_TN_FILTER_ENTRY FilterEntry;
    union
    {
        PH_THREAD_LIST_CONTEXT ListContext;
        struct
        {
            HWND Private; // phapppub
            HWND TreeNewHandle; // phapppub
        } PublicUse;
    };
    PH_PROVIDER_EVENT_QUEUE EventQueue;

    PH_CALLBACK_REGISTRATION SymbolProviderEventRegistration;
// begin_phapppub
} PH_THREADS_CONTEXT, *PPH_THREADS_CONTEXT;
// end_phapppub

#define WM_PH_MODULES_UPDATED (WM_APP + 210)

// begin_phapppub
typedef struct _PH_MODULES_CONTEXT
{
    PPH_MODULE_PROVIDER Provider;
    PH_PROVIDER_REGISTRATION ProviderRegistration;
    PH_CALLBACK_REGISTRATION AddedEventRegistration;
    PH_CALLBACK_REGISTRATION ModifiedEventRegistration;
    PH_CALLBACK_REGISTRATION RemovedEventRegistration;
    PH_CALLBACK_REGISTRATION UpdatedEventRegistration;
    PH_CALLBACK_REGISTRATION ChangedEventRegistration;

    HWND WindowHandle;
// end_phapppub
    HWND SearchboxHandle;
    HWND TreeNewHandle;
    HFONT TreeNewFont;
    union
    {
        PH_MODULE_LIST_CONTEXT ListContext;
        struct
        {
            HWND Private; // phapppub
            HWND TreeNewHandle; // phapppub
        } PublicUse;
    };
    PH_PROVIDER_EVENT_QUEUE EventQueue;
    NTSTATUS LastRunStatus;
    PPH_STRING ErrorMessage;
    ULONG_PTR SearchMatchHandle;
    PPH_TN_FILTER_ENTRY FilterEntry;
// begin_phapppub
} PH_MODULES_CONTEXT, *PPH_MODULES_CONTEXT;
// end_phapppub

#define WM_PH_HANDLES_UPDATED (WM_APP + 220)

// begin_phapppub
typedef struct _PH_HANDLES_CONTEXT
{
    PPH_HANDLE_PROVIDER Provider;
    PH_PROVIDER_REGISTRATION ProviderRegistration;
    PH_CALLBACK_REGISTRATION AddedEventRegistration;
    PH_CALLBACK_REGISTRATION ModifiedEventRegistration;
    PH_CALLBACK_REGISTRATION RemovedEventRegistration;
    PH_CALLBACK_REGISTRATION UpdatedEventRegistration;
    PH_CALLBACK_REGISTRATION ChangedEventRegistration;

    HWND WindowHandle;
// end_phapppub
    HWND TreeNewHandle;
    HWND SearchWindowHandle;
    HFONT TreeNewFont;

    union
    {
        PH_HANDLE_LIST_CONTEXT ListContext;
        struct
        {
            HWND Private; // phapppub
            HWND TreeNewHandle; // phapppub
        } PublicUse;
    };
    PH_PROVIDER_EVENT_QUEUE EventQueue;
    BOOLEAN SelectedHandleProtected;
    BOOLEAN SelectedHandleInherit;
    NTSTATUS LastRunStatus;
    PPH_STRING ErrorMessage;

    ULONG_PTR SearchMatchHandle;
    PPH_TN_FILTER_ENTRY FilterEntry;
// begin_phapppub
} PH_HANDLES_CONTEXT, *PPH_HANDLES_CONTEXT;
// end_phapppub

// begin_phapppub
typedef struct _PH_MEMORY_CONTEXT
{
    HANDLE ProcessId;
    HWND WindowHandle;
// end_phapppub
    HWND TreeNewHandle;
    HWND SearchboxHandle;
    HFONT TreeNewFont;

    union
    {
        PH_MEMORY_LIST_CONTEXT ListContext;
        struct
        {
            HWND Private; // phapppub
            HWND TreeNewHandle; // phapppub
        } PublicUse;
    };
    PH_MEMORY_ITEM_LIST MemoryItemList;
    BOOLEAN MemoryItemListValid;
    NTSTATUS LastRunStatus;
    PPH_STRING ErrorMessage;

    ULONG_PTR SearchMatchHandle;
    PPH_TN_FILTER_ENTRY AllocationFilterEntry;
    PPH_TN_FILTER_ENTRY FilterEntry;
// begin_phapppub
} PH_MEMORY_CONTEXT, *PPH_MEMORY_CONTEXT;
// end_phapppub

#define WM_PH_STATISTICS_UPDATE (WM_APP + 231)

typedef struct _PH_STATISTICS_CONTEXT
{
    PH_CALLBACK_REGISTRATION ProcessesUpdatedRegistration;
    HWND WindowHandle;
    HWND ListViewHandle;
    HFONT TreeNewFont;

    PPH_LISTVIEW_CONTEXT ListViewContext;

    BOOLEAN Enabled;
    HANDLE ProcessHandle;
    PPH_PROCESS_ITEM ProcessItem;

    BOOLEAN GotCycles;
    BOOLEAN GotCounters;

    ULONG PagePriority;
    IO_PRIORITY_HINT IoPriority;
    ULONG PeakHandleCount;
    ULONG HangCount;
    ULONG GhostCount;
    ULONGLONG RunningTime;
    ULONGLONG SuspendedTime;
    ULONGLONG NetworkTxRxBytes;
    PH_UINT64_DELTA KeyboardDelta;
    PH_UINT64_DELTA MouseDelta;

    //PPH_STRING PrivateWs;
    //PPH_STRING ShareableWs;
    //PPH_STRING SharedWs;
    PPH_STRING PeakHandles;
    PPH_STRING GdiHandles;
    PPH_STRING UserHandles;
    PPH_STRING PeakGdiHandles;
    PPH_STRING PeakUserHandles;

    FLOAT CpuUsage; FLOAT CpuUsageMin; FLOAT CpuUsageMax; FLOAT CpuUsageDiff;
    FLOAT CpuUsageUser; FLOAT CpuUsageUserMin; FLOAT CpuUsageUserMax; FLOAT CpuUsageUserDiff;
    FLOAT CpuUsageKernel; FLOAT CpuUsageKernelMin; FLOAT CpuUsageKernelMax; FLOAT CpuUsageKernelDiff;
    FLOAT CpuUsageAverage; FLOAT CpuUsageAverageMin; FLOAT CpuUsageAverageMax; FLOAT CpuUsageAverageDiff;
    FLOAT CpuUsageRelative; FLOAT CpuUsageRelativeMin; FLOAT CpuUsageRelativeMax; FLOAT CpuUsageRelativeDiff;
    ULONG64 CycleTime; ULONG64 CycleTimeMin; ULONG64 CycleTimeMax; ULONG64 CycleTimeDiff;
    ULONG64 CycleTimeDelta; ULONG64 CycleTimeDeltaMin; ULONG64 CycleTimeDeltaMax; ULONG64 CycleTimeDeltaDiff;
    ULONG64 ContextSwitches; ULONG64 ContextSwitchesMin; ULONG64 ContextSwitchesMax; ULONG64 ContextSwitchesDiff;
    ULONG64 ContextSwitchesDelta; ULONG64 ContextSwitchesDeltaMin; ULONG64 ContextSwitchesDeltaMax; ULONG64 ContextSwitchesDeltaDiff;
    ULONG64 KernelTime; ULONG64 KernelTimeMin; ULONG64 KernelTimeMax; ULONG64 KernelTimeDiff;
    ULONG64 KernelTimeDelta; ULONG64 KernelTimeDeltaMin; ULONG64 KernelTimeDeltaMax; ULONG64 KernelTimeDeltaDiff;
    ULONG64 UserTime; ULONG64 UserTimeMin; ULONG64 UserTimeMax; ULONG64 UserTimeDiff;
    ULONG64 UserTimeDelta; ULONG64 UserTimeDeltaMin; ULONG64 UserTimeDeltaMax; ULONG64 UserTimeDeltaDiff;
    KPRIORITY BasePriority; KPRIORITY BasePriorityMin; KPRIORITY BasePriorityMax; KPRIORITY BasePriorityDiff;

    ULONG_PTR PagefileUsage; ULONG_PTR PagefileUsageMin; ULONG_PTR PagefileUsageMax; ULONG_PTR PagefileUsageDiff;
    ULONG_PTR PagefileDelta; ULONG_PTR PagefileDeltaMin; ULONG_PTR PagefileDeltaMax; ULONG_PTR PagefileDeltaDiff;
    ULONG64 PeakPagefileUsage; ULONG64 PeakPagefileUsageMin; ULONG64 PeakPagefileUsageMax; ULONG64 PeakPagefileUsageDiff;
    ULONG64 VirtualSize; ULONG64 VirtualSizeMin; ULONG64 VirtualSizeMax; ULONG64 VirtualSizeDiff;
    ULONG64 PeakVirtualSize; ULONG64 PeakVirtualSizeMin; ULONG64 PeakVirtualSizeMax; ULONG64 PeakVirtualSizeDiff;
    ULONG64 PageFaultCount; ULONG64 PageFaultCountMin; ULONG64 PageFaultCountMax; ULONG64 PageFaultCountDiff;
    ULONG64 PageFaultsDelta; ULONG64 PageFaultsDeltaMin; ULONG64 PageFaultsDeltaMax; ULONG64 PageFaultsDeltaDiff;
    ULONG64 HardFaultCount; ULONG64 HardFaultCountMin; ULONG64 HardFaultCountMax; ULONG64 HardFaultCountDiff;
    ULONG64 HardFaultsDelta; ULONG64 HardFaultsDeltaMin; ULONG64 HardFaultsDeltaMax; ULONG64 HardFaultsDeltaDiff;
    ULONG64 WorkingSetSize; ULONG64 WorkingSetSizeMin; ULONG64 WorkingSetSizeMax; ULONG64 WorkingSetSizeDiff;
    ULONG64 PeakWorkingSetSize; ULONG64 PeakWorkingSetSizeMin; ULONG64 PeakWorkingSetSizeMax; ULONG64 PeakWorkingSetSizeDiff;

    ULONG64 QuotaPagedPoolUsage; ULONG64 QuotaPagedPoolUsageMin; ULONG64 QuotaPagedPoolUsageMax; ULONG64 QuotaPagedPoolUsageDiff;
    ULONG64 QuotaPeakPagedPoolUsage; ULONG64 QuotaPeakPagedPoolUsageMin; ULONG64 QuotaPeakPagedPoolUsageMax; ULONG64 QuotaPeakPagedPoolUsageDiff;
    ULONG64 QuotaNonPagedPoolUsage; ULONG64 QuotaNonPagedPoolUsageMin; ULONG64 QuotaNonPagedPoolUsageMax; ULONG64 QuotaNonPagedPoolUsageDiff;
    ULONG64 QuotaPeakNonPagedPoolUsage; ULONG64 QuotaPeakNonPagedPoolUsageMin; ULONG64 QuotaPeakNonPagedPoolUsageMax; ULONG64 QuotaPeakNonPagedPoolUsageDiff;

    ULONG64 NumberOfPrivatePages; ULONG64 NumberOfPrivatePagesMin; ULONG64 NumberOfPrivatePagesMax; ULONG64 NumberOfPrivatePagesDiff;
    ULONG64 NumberOfShareablePages; ULONG64 NumberOfShareablePagesMin; ULONG64 NumberOfShareablePagesMax; ULONG64 NumberOfShareablePagesDiff;
    ULONG64 NumberOfSharedPages; ULONG64 NumberOfSharedPagesMin; ULONG64 NumberOfSharedPagesMax; ULONG64 NumberOfSharedPagesDiff;
    ULONG64 SharedCommitUsage; ULONG64 SharedCommitUsageMin; ULONG64 SharedCommitUsageMax; ULONG64 SharedCommitUsageDiff;
    ULONG64 PrivateCommitUsage; ULONG64 PrivateCommitUsageMin; ULONG64 PrivateCommitUsageMax; ULONG64 PrivateCommitUsageDiff;
    ULONG64 PeakPrivateCommitUsage; ULONG64 PeakPrivateCommitUsageMin; ULONG64 PeakPrivateCommitUsageMax; ULONG64 PeakPrivateCommitUsageDiff;

    ULONG64 ReadOperationCount; ULONG64 ReadOperationCountMin; ULONG64 ReadOperationCountMax; ULONG64 ReadOperationCountDiff;
    ULONG64 IoReadCountDelta; ULONG64 IoReadCountDeltaMin; ULONG64 IoReadCountDeltaMax; ULONG64 IoReadCountDeltaDiff;
    ULONG64 ReadTransferCount; ULONG64 ReadTransferCountMin; ULONG64 ReadTransferCountMax; ULONG64 ReadTransferCountDiff;
    ULONG64 IoReadDelta; ULONG64 IoReadDeltaMin; ULONG64 IoReadDeltaMax; ULONG64 IoReadDeltaDiff;

    ULONG64 WriteOperationCount; ULONG64 WriteOperationCountMin; ULONG64 WriteOperationCountMax; ULONG64 WriteOperationCountDiff;
    ULONG64 IoWriteCountDelta; ULONG64 IoWriteCountDeltaMin; ULONG64 IoWriteCountDeltaMax; ULONG64 IoWriteCountDeltaDiff;
    ULONG64 WriteTransferCount; ULONG64 WriteTransferCountMin; ULONG64 WriteTransferCountMax; ULONG64 WriteTransferCountDiff;
    ULONG64 IoWriteDelta; ULONG64 IoWriteDeltaMin; ULONG64 IoWriteDeltaMax; ULONG64 IoWriteDeltaDiff;

    ULONG64 OtherOperationCount; ULONG64 OtherOperationCountMin; ULONG64 OtherOperationCountMax; ULONG64 OtherOperationCountDiff;
    ULONG64 IoOtherCountDelta; ULONG64 IoOtherCountDeltaMin; ULONG64 IoOtherCountDeltaMax; ULONG64 IoOtherCountDeltaDiff;
    ULONG64 OtherTransferCount; ULONG64 OtherTransferCountMin; ULONG64 OtherTransferCountMax; ULONG64 OtherTransferCountDiff;
    ULONG64 IoOtherDelta; ULONG64 IoOtherDeltaMin; ULONG64 IoOtherDeltaMax; ULONG64 IoOtherDeltaDiff;

    ULONG64 IoTotal; ULONG64 IoTotalMin; ULONG64 IoTotalMax; ULONG64 IoTotalDiff;
    ULONG64 IoTotalDelta; ULONG64 IoTotalDeltaMin; ULONG64 IoTotalDeltaMax; ULONG64 IoTotalDeltaDiff;

} PH_STATISTICS_CONTEXT, *PPH_STATISTICS_CONTEXT;

#define WM_PH_PERFORMANCE_UPDATE (WM_APP + 241)

typedef struct _PH_PERFORMANCE_CONTEXT
{
    PH_CALLBACK_REGISTRATION ProcessesUpdatedRegistration;

    HWND WindowHandle;
    BOOLEAN Enabled;
    LONG WindowDpi;

    PH_GRAPH_STATE CpuGraphState;
    PH_GRAPH_STATE PrivateGraphState;
    PH_GRAPH_STATE IoGraphState;

    HWND CpuGraphHandle;
    HWND PrivateGraphHandle;
    HWND IoGraphHandle;

    HWND CpuGroupBox;
    HWND PrivateBytesGroupBox;
    HWND IoGroupBox;
} PH_PERFORMANCE_CONTEXT, *PPH_PERFORMANCE_CONTEXT;

typedef struct _PH_ENVIRONMENT_ITEM
{
    PPH_STRING Name;
    PPH_STRING Value;
} PH_ENVIRONMENT_ITEM, *PPH_ENVIRONMENT_ITEM;

typedef struct _PH_ENVIRONMENT_CONTEXT
{
    HWND WindowHandle;
    HWND TreeNewHandle;
    HWND SearchWindowHandle;
    HFONT TreeNewFont;

    BOOLEAN EnableStateHighlighting;

    union
    {
        ULONG Flags;
        struct
        {
            ULONG Reserved : 1;
            ULONG HideProcessEnvironment : 1;
            ULONG HideUserEnvironment : 1;
            ULONG HideSystemEnvironment : 1;
            ULONG HighlightProcessEnvironment : 1;
            ULONG HighlightUserEnvironment : 1;
            ULONG HighlightSystemEnvironment : 1;
            ULONG HideCmdTypeEnvironment : 1;
            ULONG HighlightCmdEnvironment : 1;
            ULONG Spare : 23;
        };
    };

    PPH_PROCESS_ITEM ProcessItem;
    ULONG_PTR SearchMatchHandle;
    PPH_STRING StatusMessage;

    PPH_LIST NodeList;
    PPH_LIST NodeRootList;
    PPH_HASHTABLE NodeHashtable;
    PPH_TN_FILTER_ENTRY TreeFilterEntry;
    ULONG TreeNewSortColumn;
    PH_TN_FILTER_SUPPORT TreeFilterSupport;
    PH_SORT_ORDER TreeNewSortOrder;
    PH_CM_MANAGER Cm;

    PH_ARRAY Items;
} PH_ENVIRONMENT_CONTEXT, *PPH_ENVIRONMENT_CONTEXT;

#endif
