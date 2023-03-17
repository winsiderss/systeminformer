#ifndef ETWSYS_H
#define ETWSYS_H

// Disk section

BOOLEAN EtpDiskSysInfoSectionCallback(
    _In_ PPH_SYSINFO_SECTION Section,
    _In_ PH_SYSINFO_SECTION_MESSAGE Message,
    _In_ PVOID Parameter1,
    _In_ PVOID Parameter2
    );

INT_PTR CALLBACK EtpDiskDialogProc(
    _In_ HWND hwndDlg,
    _In_ UINT uMsg,
    _In_ WPARAM wParam,
    _In_ LPARAM lParam
    );

INT_PTR CALLBACK EtpDiskPanelDialogProc(
    _In_ HWND hwndDlg,
    _In_ UINT uMsg,
    _In_ WPARAM wParam,
    _In_ LPARAM lParam
    );

VOID EtpNotifyDiskGraph(
    _In_ NMHDR *Header
    );

VOID EtpUpdateDiskGraph(
    VOID
    );

VOID EtpUpdateDiskPanel(
    VOID
    );

PPH_PROCESS_RECORD EtpReferenceMaxDiskRecord(
    _In_ LONG Index
    );

PPH_STRING EtpGetMaxDiskString(
    _In_ LONG Index
    );

// Network section

BOOLEAN EtpNetworkSysInfoSectionCallback(
    _In_ PPH_SYSINFO_SECTION Section,
    _In_ PH_SYSINFO_SECTION_MESSAGE Message,
    _In_ PVOID Parameter1,
    _In_ PVOID Parameter2
    );

INT_PTR CALLBACK EtpNetworkDialogProc(
    _In_ HWND hwndDlg,
    _In_ UINT uMsg,
    _In_ WPARAM wParam,
    _In_ LPARAM lParam
    );

INT_PTR CALLBACK EtpNetworkPanelDialogProc(
    _In_ HWND hwndDlg,
    _In_ UINT uMsg,
    _In_ WPARAM wParam,
    _In_ LPARAM lParam
    );

VOID EtpNotifyNetworkGraph(
    _In_ NMHDR *Header
    );

VOID EtpUpdateNetworkGraph(
    VOID
    );

VOID EtpUpdateNetworkPanel(
    VOID
    );

PPH_PROCESS_RECORD EtpReferenceMaxNetworkRecord(
    _In_ LONG Index
    );

PPH_STRING EtpGetMaxNetworkString(
    _In_ LONG Index
    );

#endif
