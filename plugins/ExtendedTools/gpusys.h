/*
 * Copyright (c) 2022 Winsider Seminars & Solutions, Inc.  All rights reserved.
 *
 * This file is part of System Informer.
 *
 * Authors:
 *
 *     wj32    2011-2015
 *     dmex    2016-2023
 *
 */

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

VOID EtpNotifyDedicatedGpuGraph(
    _In_ NMHDR *Header
    );

VOID EtpNotifySharedGpuGraph(
    _In_ NMHDR *Header
    );

VOID EtpNotifyPowerUsageGpuGraph(
    _In_ NMHDR *Header
    );

VOID EtpNotifyTemperatureGpuGraph(
    _In_ NMHDR *Header
    );

VOID EtpNotifyFanRpmGpuGraph(
    _In_ NMHDR *Header
    );

VOID EtpUpdateGpuGraphs(
    VOID
    );

VOID EtpUpdateGpuPanel(
    VOID
    );

PPH_PROCESS_RECORD EtpGpuReferenceMaxNodeRecord(
    _In_ LONG Index
    );

PPH_STRING EtpGpuGetMaxNodeString(
    _In_ LONG Index
    );

PPH_STRING EtpGpuGetNameString(
    VOID
    );

#endif
