#ifndef GPUSYS_H
#define GPUSYS_H

#define ET_GPU_PADDING 3

BOOLEAN EtpGpuSysInfoSectionCallback(
    _In_ PPH_SYSINFO_SECTION Section,
    _In_ PH_SYSINFO_SECTION_MESSAGE Message,
    _In_ PVOID Parameter1,
    _In_ PVOID Parameter2
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
    _In_ HWND hwndDlg,
    _In_ UINT uMsg,
    _In_ WPARAM wParam,
    _In_ LPARAM lParam
    );

INT_PTR CALLBACK EtpGpuPanelDialogProc(
    _In_ HWND hwndDlg,
    _In_ UINT uMsg,
    _In_ WPARAM wParam,
    _In_ LPARAM lParam
    );

VOID EtpCreateGpuGraphs(
    VOID
    );

VOID EtpLayoutGpuGraphs(
    _In_ HWND hwnd
    );

VOID EtpNotifyGpuGraph(
    _In_ NMHDR *Header
    );

VOID EtpNotifyDedicatedGraph(
    _In_ NMHDR *Header
    );

VOID EtpNotifySharedGraph(
    _In_ NMHDR *Header
    );

VOID EtpNotifyPowerUsageGraph(
    _In_ NMHDR *Header
    );

VOID EtpNotifyTemperatureGraph(
    _In_ NMHDR *Header
    );

VOID EtpNotifyFanRpmGraph(
    _In_ NMHDR *Header
    );

VOID EtpUpdateGpuGraphs(
    VOID
    );

VOID EtpUpdateGpuPanel(
    VOID
    );

PPH_PROCESS_RECORD EtpReferenceMaxNodeRecord(
    _In_ LONG Index
    );

PPH_STRING EtpGetMaxNodeString(
    _In_ LONG Index
    );

PPH_STRING EtpGetGpuNameString(
    VOID
    );

#endif
