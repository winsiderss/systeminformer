#ifndef PH_SYSINFOP_H
#define PH_SYSINFOP_H

// Constants

#define PH_SYSINFO_FADE_ADD 50
#define PH_SYSINFO_PANEL_PADDING 3
#define PH_SYSINFO_WINDOW_PADDING 7
#define PH_SYSINFO_GRAPH_PADDING 4
#define PH_SYSINFO_SMALL_GRAPH_WIDTH 48
#define PH_SYSINFO_SMALL_GRAPH_PADDING 5
#define PH_SYSINFO_SEPARATOR_WIDTH 2

#define PH_SYSINFO_CPU_PADDING 5
#define PH_SYSINFO_MEMORY_PADDING 3

#define SI_MSG_SYSINFO_FIRST (WM_APP + 150)
#define SI_MSG_SYSINFO_ACTIVATE (WM_APP + 150)
#define SI_MSG_SYSINFO_UPDATE (WM_APP + 151)
#define SI_MSG_SYSINFO_CHANGE_SETTINGS (WM_APP + 152)
#define SI_MSG_SYSINFO_LAST (WM_APP + 152)

// Thread & window

extern HWND PhSipWindow;

NTSTATUS PhSipSysInfoThreadStart(
    _In_ PVOID Parameter
    );

INT_PTR CALLBACK PhSipSysInfoDialogProc(
    _In_ HWND hwndDlg,
    _In_ UINT uMsg,
    _In_ WPARAM wParam,
    _In_ LPARAM lParam
    );

INT_PTR CALLBACK PhSipContainerDialogProc(
    _In_ HWND hwndDlg,
    _In_ UINT uMsg,
    _In_ WPARAM wParam,
    _In_ LPARAM lParam
    );

// Event handlers

VOID PhSipOnInitDialog(
    VOID
    );

VOID PhSipOnDestroy(
    VOID
    );

VOID PhSipOnNcDestroy(
    VOID
    );

BOOLEAN PhSipOnSysCommand(
    _In_ ULONG Type,
    _In_ LONG CursorScreenX,
    _In_ LONG CursorScreenY
    );

VOID PhSipOnSize(
    VOID
    );

VOID PhSipOnSizing(
    _In_ ULONG Edge,
    _In_ PRECT DragRectangle
    );

VOID PhSipOnThemeChanged(
    VOID
    );

VOID PhSipOnCommand(
    _In_ HWND HwndControl,
    _In_ ULONG Id,
    _In_ ULONG Code
    );

_Success_(return)
BOOLEAN PhSipOnNotify(
    _In_ NMHDR *Header,
    _Out_ LRESULT *Result
    );

BOOLEAN PhSipOnDrawItem(
    _In_ ULONG_PTR Id,
    _In_ PDRAWITEMSTRUCT DrawItemStruct
    );

VOID PhSipOnUserMessage(
    _In_ ULONG Message,
    _In_ ULONG_PTR WParam,
    _In_ ULONG_PTR LParam
    );

ULONG PhSipGetProcessorRelationshipIndex(
    _In_ LOGICAL_PROCESSOR_RELATIONSHIP RelationshipType,
    _In_ ULONG Index
    );

// Framework

VOID PhSipRegisterDialog(
    _In_ HWND DialogWindowHandle
    );

VOID PhSipUnregisterDialog(
    _In_ HWND DialogWindowHandle
    );

VOID PhSipInitializeParameters(
    VOID
    );

VOID PhSipDeleteParameters(
    VOID
    );

VOID PhSipUpdateColorParameters(
    VOID
    );

PPH_SYSINFO_SECTION PhSipCreateSection(
    _In_ PPH_SYSINFO_SECTION Template
    );

VOID PhSipDestroySection(
    _In_ PPH_SYSINFO_SECTION Section
    );

PPH_SYSINFO_SECTION PhSipFindSection(
    _In_ PPH_STRINGREF Name
    );

PPH_SYSINFO_SECTION PhSipCreateInternalSection(
    _In_ PWSTR Name,
    _In_ ULONG Flags,
    _In_ PPH_SYSINFO_SECTION_CALLBACK Callback
    );

VOID PhSipDrawRestoreSummaryPanel(
    _In_ PDRAWITEMSTRUCT DrawItemStruct
    );

VOID PhSipDrawSeparator(
    _In_ PDRAWITEMSTRUCT DrawItemStruct
    );

VOID PhSipDrawPanel(
    _In_ PPH_SYSINFO_SECTION Section,
    _In_ HDC hdc,
    _In_ PRECT Rect
    );

VOID PhSipDefaultDrawPanel(
    _In_ PPH_SYSINFO_SECTION Section,
    _In_ PPH_SYSINFO_DRAW_PANEL DrawPanel
    );

VOID PhSipLayoutSummaryView(
    VOID
    );

VOID PhSipLayoutSectionView(
    VOID
    );

VOID PhSipEnterSectionView(
    _In_ PPH_SYSINFO_SECTION NewSection
    );

VOID PhSipEnterSectionViewInner(
    _In_ PPH_SYSINFO_SECTION Section,
    _In_ BOOLEAN FromSummaryView,
    _Inout_ HDWP *DeferHandle,
    _Inout_ HDWP *ContainerDeferHandle
    );

VOID PhSipRestoreSummaryView(
    VOID
    );

VOID PhSipCreateSectionDialog(
    _In_ PPH_SYSINFO_SECTION Section
    );

LRESULT CALLBACK PhSipGraphHookWndProc(
    _In_ HWND hwnd,
    _In_ UINT uMsg,
    _In_ WPARAM wParam,
    _In_ LPARAM lParam
    );

LRESULT CALLBACK PhSipPanelHookWndProc(
    _In_ HWND hwnd,
    _In_ UINT uMsg,
    _In_ WPARAM wParam,
    _In_ LPARAM lParam
    );

// Misc.

VOID PhSipUpdateThemeData(
    VOID
    );

VOID PhSipSaveWindowState(
    VOID
    );

VOID NTAPI PhSipSysInfoUpdateHandler(
    _In_opt_ PVOID Parameter,
    _In_opt_ PVOID Context
    );

// CPU section

BOOLEAN PhSipCpuSectionCallback(
    _In_ PPH_SYSINFO_SECTION Section,
    _In_ PH_SYSINFO_SECTION_MESSAGE Message,
    _In_opt_ PVOID Parameter1,
    _In_opt_ PVOID Parameter2
    );

VOID PhSipInitializeCpuDialog(
    VOID
    );

VOID PhSipUninitializeCpuDialog(
    VOID
    );

VOID PhSipTickCpuDialog(
    VOID
    );

INT_PTR CALLBACK PhSipCpuDialogProc(
    _In_ HWND hwndDlg,
    _In_ UINT uMsg,
    _In_ WPARAM wParam,
    _In_ LPARAM lParam
    );

INT_PTR CALLBACK PhSipCpuPanelDialogProc(
    _In_ HWND hwndDlg,
    _In_ UINT uMsg,
    _In_ WPARAM wParam,
    _In_ LPARAM lParam
    );

VOID PhSipCreateCpuGraphs(
    VOID
    );

VOID PhSipLayoutCpuGraphs(
    VOID
    );

VOID PhSipSetOneGraphPerCpu(
    VOID
    );

VOID PhSipNotifyCpuGraph(
    _In_ ULONG Index,
    _In_ NMHDR *Header
    );

VOID PhSipUpdateCpuGraphs(
    VOID
    );

VOID PhSipUpdateCpuPanel(
    VOID
    );

PPH_PROCESS_RECORD PhSipReferenceMaxCpuRecord(
    _In_ LONG Index
    );

PPH_STRING PhSipGetMaxCpuString(
    _In_ LONG Index
    );

PPH_STRING PhSipGetCpuBrandString(
    VOID
    );

_Success_(return)
BOOLEAN PhSipGetCpuFrequencyFromDistribution(
    _Out_ DOUBLE *Frequency
    );

NTSTATUS PhSipQueryProcessorPerformanceDistribution(
    _Out_ PVOID *Buffer
    );

PPH_STRINGREF PhGetHybridProcessorType(
    _In_ ULONG ProcessorIndex
    );

// Memory section

BOOLEAN PhSipMemorySectionCallback(
    _In_ PPH_SYSINFO_SECTION Section,
    _In_ PH_SYSINFO_SECTION_MESSAGE Message,
    _In_opt_ PVOID Parameter1,
    _In_opt_ PVOID Parameter2
    );

VOID PhSipInitializeMemoryDialog(
    VOID
    );

VOID PhSipUninitializeMemoryDialog(
    VOID
    );

VOID PhSipTickMemoryDialog(
    VOID
    );

INT_PTR CALLBACK PhSipMemoryDialogProc(
    _In_ HWND hwndDlg,
    _In_ UINT uMsg,
    _In_ WPARAM wParam,
    _In_ LPARAM lParam
    );

INT_PTR CALLBACK PhSipMemoryPanelDialogProc(
    _In_ HWND hwndDlg,
    _In_ UINT uMsg,
    _In_ WPARAM wParam,
    _In_ LPARAM lParam
    );

VOID PhSipCreateMemoryGraphs(
    VOID
    );

VOID PhSipLayoutMemoryGraphs(
    VOID
    );

VOID PhSipNotifyCommitGraph(
    _In_ NMHDR *Header
    );

VOID PhSipNotifyPhysicalGraph(
    _In_ NMHDR *Header
    );

VOID PhSipUpdateMemoryGraphs(
    VOID
    );

VOID PhSipUpdateMemoryPanel(
    VOID
    );

NTSTATUS PhSipLoadMmAddresses(
    _In_ PVOID Parameter
    );

VOID PhSipGetPoolLimits(
    _Out_ PSIZE_T Paged,
    _Out_ PSIZE_T NonPaged
    );

_Success_(return)
BOOLEAN PhSipGetMemoryCompressionLimits(
    _Out_ DOUBLE *CurrentCompressedMemory,
    _Out_ DOUBLE *TotalCompressedMemory,
    _Out_ DOUBLE *TotalSavedMemory
    );

// I/O section

BOOLEAN PhSipIoSectionCallback(
    _In_ PPH_SYSINFO_SECTION Section,
    _In_ PH_SYSINFO_SECTION_MESSAGE Message,
    _In_opt_ PVOID Parameter1,
    _In_opt_ PVOID Parameter2
    );

VOID PhSipInitializeIoDialog(
    VOID
    );

VOID PhSipUninitializeIoDialog(
    VOID
    );

VOID PhSipTickIoDialog(
    VOID
    );

INT_PTR CALLBACK PhSipIoDialogProc(
    _In_ HWND hwndDlg,
    _In_ UINT uMsg,
    _In_ WPARAM wParam,
    _In_ LPARAM lParam
    );

INT_PTR CALLBACK PhSipIoPanelDialogProc(
    _In_ HWND hwndDlg,
    _In_ UINT uMsg,
    _In_ WPARAM wParam,
    _In_ LPARAM lParam
    );

VOID PhSipCreateIoGraph(
    VOID
    );

VOID PhSipNotifyIoGraph(
    _In_ NMHDR *Header
    );

VOID PhSipUpdateIoGraph(
    VOID
    );

VOID PhSipUpdateIoPanel(
    VOID
    );

PPH_PROCESS_RECORD PhSipReferenceMaxIoRecord(
    _In_ LONG Index
    );

PPH_STRING PhSipGetMaxIoString(
    _In_ LONG Index
    );

#endif
