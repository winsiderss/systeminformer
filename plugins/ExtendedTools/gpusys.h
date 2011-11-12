#ifndef GPUSYS_H
#define GPUSYS_H

#define ET_GPU_PADDING 3

BOOLEAN EtpGpuSectionCallback(
    __in PPH_SYSINFO_SECTION Section,
    __in PH_SYSINFO_SECTION_MESSAGE Message,
    __in_opt PVOID Parameter1,
    __in_opt PVOID Parameter2
    );

VOID EtpInitializeGpuDialog(
    VOID
    );

VOID EtpUninitializeGpuDialog(
    VOID
    );

VOID EtpTickGpuDialog(
    VOID
    );

INT_PTR CALLBACK EtpGpuDialogProc(
    __in HWND hwndDlg,
    __in UINT uMsg,
    __in WPARAM wParam,
    __in LPARAM lParam
    );

INT_PTR CALLBACK EtpGpuPanelDialogProc(
    __in HWND hwndDlg,
    __in UINT uMsg,
    __in WPARAM wParam,
    __in LPARAM lParam
    );

VOID EtpCreateGpuGraphs(
    VOID
    );

VOID EtpLayoutGpuGraphs(
    VOID
    );

VOID EtpNotifyGpuGraph(
    __in NMHDR *Header
    );

VOID EtpNotifyDedicatedGraph(
    __in NMHDR *Header
    );

VOID EtpNotifySharedGraph(
    __in NMHDR *Header
    );

VOID EtpUpdateGpuGraphs(
    VOID
    );

VOID EtpUpdateGpuPanel(
    VOID
    );

PPH_PROCESS_RECORD EtpReferenceMaxNodeRecord(
    __in LONG Index
    );

PPH_STRING EtpGetMaxNodeString(
    __in LONG Index
    );

PPH_STRING EtpGetGpuNameString(
    VOID
    );

#endif
