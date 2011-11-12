#ifndef SYSINFOP_H
#define SYSINFOP_H

// Constants

#define PH_SYSINFO_FADE_WIDTH 140
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
#define SI_MSG_SYSINFO_LAST (WM_APP + 151)

// Misc.

typedef HRESULT (WINAPI *_EnableThemeDialogTexture)(
    __in HWND hwnd,
    __in DWORD dwFlags
    );

// Thread & window

NTSTATUS PhSipSysInfoThreadStart(
    __in PVOID Parameter
    );

INT_PTR CALLBACK PhSipSysInfoDialogProc(
    __in HWND hwndDlg,
    __in UINT uMsg,
    __in WPARAM wParam,
    __in LPARAM lParam
    );

INT_PTR CALLBACK PhSipContainerDialogProc(
    __in HWND hwndDlg,
    __in UINT uMsg,
    __in WPARAM wParam,
    __in LPARAM lParam
    );

// Event handlers

VOID PhSipOnInitDialog(
    VOID
    );

VOID PhSipOnDestroy(
    VOID
    );

VOID PhSipOnShowWindow(
    __in BOOLEAN Showing,
    __in ULONG State
    );

VOID PhSipOnSize(
    VOID
    );

VOID PhSipOnSizing(
    __in ULONG Edge,
    __in PRECT DragRectangle
    );

VOID PhSipOnCommand(
    __in ULONG Id,
    __in ULONG Code
    );

BOOLEAN PhSipOnNotify(
    __in NMHDR *Header,
    __out LRESULT *Result
    );

BOOLEAN PhSipOnDrawItem(
    __in ULONG_PTR Id,
    __in DRAWITEMSTRUCT *DrawItemStruct
    );

VOID PhSipOnUserMessage(
    __in ULONG Message,
    __in ULONG_PTR WParam,
    __in ULONG_PTR LParam
    );

// Framework

VOID PhSipRegisterDialog(
    __in HWND DialogWindowHandle
    );

VOID PhSipUnregisterDialog(
    __in HWND DialogWindowHandle
    );

VOID PhSipInitializeParameters(
    VOID
    );

VOID PhSipDeleteParameters(
    VOID
    );

PPH_SYSINFO_SECTION PhSipCreateSection(
    __in PPH_SYSINFO_SECTION Template
    );

VOID PhSipDestroySection(
    __in PPH_SYSINFO_SECTION Section
    );

PPH_SYSINFO_SECTION PhSipFindSection(
    __in PPH_STRINGREF Name
    );

PPH_SYSINFO_SECTION PhSipCreateInternalSection(
    __in PWSTR Name,
    __in ULONG Flags,
    __in PPH_SYSINFO_SECTION_CALLBACK Callback
    );

VOID PhSipDrawRestoreSummaryPanel(
    __in HDC hdc,
    __in PRECT Rect
    );

VOID PhSipDrawSeparator(
    __in HDC hdc,
    __in PRECT Rect
    );

VOID PhSipDrawPanel(
    __in PPH_SYSINFO_SECTION Section,
    __in HDC hdc,
    __in PRECT Rect
    );

VOID PhSipDefaultDrawPanel(
    __in PPH_SYSINFO_SECTION Section,
    __in PPH_SYSINFO_DRAW_PANEL DrawPanel
    );

VOID PhSipLayoutSummaryView(
    VOID
    );

VOID PhSipLayoutSectionView(
    VOID
    );

VOID PhSipEnterSectionView(
    __in PPH_SYSINFO_SECTION NewSection
    );

VOID PhSipRestoreSummaryView(
    VOID
    );

HWND PhSipDefaultCreateDialog(
    __in PVOID Instance,
    __in PWSTR Template,
    __in DLGPROC DialogProc
    );

VOID PhSipCreateSectionDialog(
    __in PPH_SYSINFO_SECTION Section
    );

// Misc.

VOID PhSipSetAlwaysOnTop(
    VOID
    );

VOID NTAPI PhSipSysInfoUpdateHandler(
    __in_opt PVOID Parameter,
    __in_opt PVOID Context
    );

PPH_STRING PhSipFormatSizeWithPrecision(
    __in ULONG64 Size,
    __in USHORT Precision
    );

// CPU section

BOOLEAN PhSipCpuSectionCallback(
    __in PPH_SYSINFO_SECTION Section,
    __in PH_SYSINFO_SECTION_MESSAGE Message,
    __in_opt PVOID Parameter1,
    __in_opt PVOID Parameter2
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
    __in HWND hwndDlg,
    __in UINT uMsg,
    __in WPARAM wParam,
    __in LPARAM lParam
    );

INT_PTR CALLBACK PhSipCpuPanelDialogProc(
    __in HWND hwndDlg,
    __in UINT uMsg,
    __in WPARAM wParam,
    __in LPARAM lParam
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
    __in ULONG Index,
    __in NMHDR *Header
    );

VOID PhSipUpdateCpuGraphs(
    VOID
    );

VOID PhSipUpdateCpuPanel(
    VOID
    );

PPH_PROCESS_RECORD PhSipReferenceMaxCpuRecord(
    __in LONG Index
    );

PPH_STRING PhSipGetMaxCpuString(
    __in LONG Index
    );

VOID PhSipGetCpuBrandString(
    __out_ecount(49) PWSTR BrandString
    );

// Memory section

BOOLEAN PhSipMemorySectionCallback(
    __in PPH_SYSINFO_SECTION Section,
    __in PH_SYSINFO_SECTION_MESSAGE Message,
    __in_opt PVOID Parameter1,
    __in_opt PVOID Parameter2
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
    __in HWND hwndDlg,
    __in UINT uMsg,
    __in WPARAM wParam,
    __in LPARAM lParam
    );

INT_PTR CALLBACK PhSipMemoryPanelDialogProc(
    __in HWND hwndDlg,
    __in UINT uMsg,
    __in WPARAM wParam,
    __in LPARAM lParam
    );

VOID PhSipLayoutMemoryGraphs(
    VOID
    );

VOID PhSipNotifyCommitGraph(
    __in NMHDR *Header
    );

VOID PhSipNotifyPhysicalGraph(
    __in NMHDR *Header
    );

VOID PhSipUpdateMemoryGraphs(
    VOID
    );

VOID PhSipUpdateMemoryPanel(
    VOID
    );

NTSTATUS PhSipLoadMmAddresses(
    __in PVOID Parameter
    );

VOID PhSipGetPoolLimits(
    __out PSIZE_T Paged,
    __out PSIZE_T NonPaged
    );

// I/O section

BOOLEAN PhSipIoSectionCallback(
    __in PPH_SYSINFO_SECTION Section,
    __in PH_SYSINFO_SECTION_MESSAGE Message,
    __in_opt PVOID Parameter1,
    __in_opt PVOID Parameter2
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
    __in HWND hwndDlg,
    __in UINT uMsg,
    __in WPARAM wParam,
    __in LPARAM lParam
    );

INT_PTR CALLBACK PhSipIoPanelDialogProc(
    __in HWND hwndDlg,
    __in UINT uMsg,
    __in WPARAM wParam,
    __in LPARAM lParam
    );

VOID PhSipNotifyIoGraph(
    __in NMHDR *Header
    );

VOID PhSipUpdateIoGraph(
    VOID
    );

VOID PhSipUpdateIoPanel(
    VOID
    );

PPH_PROCESS_RECORD PhSipReferenceMaxIoRecord(
    __in LONG Index
    );

PPH_STRING PhSipGetMaxIoString(
    __in LONG Index
    );

#endif
