#ifndef PH_SYSINFOP_H
#define PH_SYSINFOP_H

// Constants

#define PH_SYSINFO_FADE_ADD 50
#define PH_SYSINFO_PANEL_PADDING 3
#define PH_SYSINFO_WINDOW_PADDING 13
#define PH_SYSINFO_GRAPH_PADDING 9
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

// Misc.

typedef HRESULT (WINAPI *_EnableThemeDialogTexture)(
    _In_ HWND hwnd,
    _In_ DWORD dwFlags
    );

// Thread & window

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

VOID PhSipOnShowWindow(
    _In_ BOOLEAN Showing,
    _In_ ULONG State
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
    _In_ ULONG Id,
    _In_ ULONG Code
    );

BOOLEAN PhSipOnNotify(
    _In_ NMHDR *Header,
    _Out_ LRESULT *Result
    );

BOOLEAN PhSipOnDrawItem(
    _In_ ULONG_PTR Id,
    _In_ DRAWITEMSTRUCT *DrawItemStruct
    );

VOID PhSipOnUserMessage(
    _In_ ULONG Message,
    _In_ ULONG_PTR WParam,
    _In_ ULONG_PTR LParam
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
    _In_ HDC hdc,
    _In_ PRECT Rect
    );

VOID PhSipDrawSeparator(
    _In_ HDC hdc,
    _In_ PRECT Rect
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

VOID PhSipSetAlwaysOnTop(
    VOID
    );

VOID NTAPI PhSipSysInfoUpdateHandler(
    _In_opt_ PVOID Parameter,
    _In_opt_ PVOID Context
    );

PPH_STRING PhSipFormatSizeWithPrecision(
    _In_ ULONG64 Size,
    _In_ USHORT Precision
    );

// CPU section

typedef struct _SYSTEM_PROCESSOR_PERFORMANCE_HITCOUNT_WIN8
{
    ULONG Hits;
    UCHAR PercentFrequency;
} SYSTEM_PROCESSOR_PERFORMANCE_HITCOUNT_WIN8, *PSYSTEM_PROCESSOR_PERFORMANCE_HITCOUNT_WIN8;

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

VOID PhSipGetCpuBrandString(
    _Out_writes_(49) PWSTR BrandString
    );

BOOLEAN PhSipGetCpuFrequencyFromDistribution(
    _Out_ DOUBLE *Fraction
    );

NTSTATUS PhSipQueryProcessorPerformanceDistribution(
    _Out_ PVOID *Buffer
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
