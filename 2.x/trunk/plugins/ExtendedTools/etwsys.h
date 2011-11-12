#ifndef ETWSYS_H
#define ETWSYS_H

// Disk section

BOOLEAN EtpDiskSectionCallback(
    __in PPH_SYSINFO_SECTION Section,
    __in PH_SYSINFO_SECTION_MESSAGE Message,
    __in_opt PVOID Parameter1,
    __in_opt PVOID Parameter2
    );

INT_PTR CALLBACK EtpDiskDialogProc(
    __in HWND hwndDlg,
    __in UINT uMsg,
    __in WPARAM wParam,
    __in LPARAM lParam
    );

INT_PTR CALLBACK EtpDiskPanelDialogProc(
    __in HWND hwndDlg,
    __in UINT uMsg,
    __in WPARAM wParam,
    __in LPARAM lParam
    );

VOID EtpNotifyDiskGraph(
    __in NMHDR *Header
    );

VOID EtpUpdateDiskGraph(
    VOID
    );

VOID EtpUpdateDiskPanel(
    VOID
    );

PPH_PROCESS_RECORD EtpReferenceMaxDiskRecord(
    __in LONG Index
    );

PPH_STRING EtpGetMaxDiskString(
    __in LONG Index
    );

// Network section

BOOLEAN EtpNetworkSectionCallback(
    __in PPH_SYSINFO_SECTION Section,
    __in PH_SYSINFO_SECTION_MESSAGE Message,
    __in_opt PVOID Parameter1,
    __in_opt PVOID Parameter2
    );

INT_PTR CALLBACK EtpNetworkDialogProc(
    __in HWND hwndDlg,
    __in UINT uMsg,
    __in WPARAM wParam,
    __in LPARAM lParam
    );

INT_PTR CALLBACK EtpNetworkPanelDialogProc(
    __in HWND hwndDlg,
    __in UINT uMsg,
    __in WPARAM wParam,
    __in LPARAM lParam
    );

VOID EtpNotifyNetworkGraph(
    __in NMHDR *Header
    );

VOID EtpUpdateNetworkGraph(
    VOID
    );

VOID EtpUpdateNetworkPanel(
    VOID
    );

PPH_PROCESS_RECORD EtpReferenceMaxNetworkRecord(
    __in LONG Index
    );

PPH_STRING EtpGetMaxNetworkString(
    __in LONG Index
    );

#endif
