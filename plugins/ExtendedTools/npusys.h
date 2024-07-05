/*
 * Copyright (c) 2022 Winsider Seminars & Solutions, Inc.  All rights reserved.
 *
 * This file is part of System Informer.
 *
 * Authors:
 *
 *     wj32    2011-2015
 *     dmex    2016-2023
 *     jxy-s   2024
 *
 */

#ifndef NPUSYS_H
#define NPUSYS_H

#define ET_NPU_PADDING 3

BOOLEAN EtpNpuSysInfoSectionCallback(
    _In_ PPH_SYSINFO_SECTION Section,
    _In_ PH_SYSINFO_SECTION_MESSAGE Message,
    _In_ PVOID Parameter1,
    _In_ PVOID Parameter2
    );

VOID EtpInitializeNpuDialog(
    VOID
    );

VOID EtpUninitializeNpuDialog(
    VOID
    );

VOID EtpTickNpuDialog(
    VOID
    );

INT_PTR CALLBACK EtpNpuDialogProc(
    _In_ HWND hwndDlg,
    _In_ UINT uMsg,
    _In_ WPARAM wParam,
    _In_ LPARAM lParam
    );

INT_PTR CALLBACK EtpNpuPanelDialogProc(
    _In_ HWND hwndDlg,
    _In_ UINT uMsg,
    _In_ WPARAM wParam,
    _In_ LPARAM lParam
    );

VOID EtpCreateNpuGraphs(
    VOID
    );

VOID EtpLayoutNpuGraphs(
    _In_ HWND hwnd
    );

VOID EtpNotifyNpuGraph(
    _In_ NMHDR *Header
    );

VOID EtpNotifyDedicatedNpuGraph(
    _In_ NMHDR *Header
    );

VOID EtpNotifySharedNpuGraph(
    _In_ NMHDR *Header
    );

VOID EtpNotifyPowerUsageNpuGraph(
    _In_ NMHDR *Header
    );

VOID EtpNotifyTemperatureNpuGraph(
    _In_ NMHDR *Header
    );

VOID EtpNotifyFanRpmNpuGraph(
    _In_ NMHDR *Header
    );

VOID EtpUpdateNpuGraphs(
    VOID
    );

VOID EtpUpdateNpuPanel(
    VOID
    );

PPH_PROCESS_RECORD EtpNpuReferenceMaxNodeRecord(
    _In_ LONG Index
    );

PPH_STRING EtpNpuGetMaxNodeString(
    _In_ LONG Index
    );

PPH_STRING EtpNpuGetNameString(
    VOID
    );

#endif
