/*
 * Copyright (c) 2022 Winsider Seminars & Solutions, Inc.  All rights reserved.
 *
 * This file is part of System Informer.
 *
 * Authors:
 *
 *     wj32    2011-2015
 *     dmex    2017-2023
 *
 */

#ifndef ETWSYS_H
#define ETWSYS_H

// Disk section

BOOLEAN EtpDiskSysInfoSectionCallback(
    _In_ PPH_SYSINFO_SECTION Section,
    _In_ PH_SYSINFO_SECTION_MESSAGE Message,
    _In_ PVOID Parameter1,
    _In_ PVOID Parameter2
    );

VOID EtpInitializeDiskDialog(
    VOID
    );

VOID EtpUninitializeDiskDialog(
    VOID
    );

VOID EtpTickDiskDialog(
    VOID
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

VOID EtpCreateDiskGraph(
    VOID
    );

VOID EtpLayoutDiskGraphs(
    _In_ HWND WindowHandle
    );

VOID EtpNotifyDiskReadGraph(
    _In_ NMHDR *Header
    );

VOID EtpNotifyDiskWriteGraph(
    _In_ NMHDR* Header
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

VOID EtpInitializeNetworkDialog(
    VOID
    );

VOID EtpUninitializeNetworkDialog(
    VOID
    );

VOID EtpTickNetworkDialog(
    VOID
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

VOID EtpCreateNetworkGraph(
    VOID
    );

VOID EtpLayoutNetworkGraphs(
    _In_ HWND WindowHandle
    );

VOID EtpUpdateNetworkGraph(
    VOID
    );

VOID EtpUpdateNetworkPanel(
    VOID
    );

VOID EtpNotifyNetworkReceiveGraph(
    _In_ NMHDR *Header
    );

VOID EtpNotifyNetworkSendGraph(
    _In_ NMHDR *Header
    );

PPH_PROCESS_RECORD EtpReferenceMaxNetworkRecord(
    _In_ LONG Index
    );

PPH_STRING EtpGetMaxNetworkString(
    _In_ LONG Index
    );

#endif
