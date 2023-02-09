/*
 * Copyright (c) 2022 Winsider Seminars & Solutions, Inc.  All rights reserved.
 *
 * This file is part of System Informer.
 *
 * Authors:
 *
 *     wj32    2010-2015
 *     dmex    2015-2023
 *
 */

#ifndef ES_EXTSRV_H
#define ES_EXTSRV_H

#include <phdk.h>
#include <phappresource.h>
#include <settings.h>

#include "resource.h"

// main

extern PPH_PLUGIN PluginInstance;

#define PLUGIN_NAME L"ProcessHacker.ExtendedServices"
#define SETTING_NAME_ENABLE_SERVICES_MENU (PLUGIN_NAME L".EnableServicesMenu")

#define SIP(String, Integer) { (String), (PVOID)(Integer) }

// depend

INT_PTR CALLBACK EspServiceDependenciesDlgProc(
    _In_ HWND hwndDlg,
    _In_ UINT uMsg,
    _In_ WPARAM wParam,
    _In_ LPARAM lParam
    );

INT_PTR CALLBACK EspServiceDependentsDlgProc(
    _In_ HWND hwndDlg,
    _In_ UINT uMsg,
    _In_ WPARAM wParam,
    _In_ LPARAM lParam
    );

// other

typedef NTSTATUS (NTAPI *_RtlCreateServiceSid)(
    _In_ PUNICODE_STRING ServiceName,
    _Out_writes_bytes_opt_(*ServiceSidLength) PSID ServiceSid,
    _Inout_ PULONG ServiceSidLength
    );

INT_PTR CALLBACK EspServiceOtherDlgProc(
    _In_ HWND hwndDlg,
    _In_ UINT uMsg,
    _In_ WPARAM wParam,
    _In_ LPARAM lParam
    );

// recovery

INT_PTR CALLBACK EspServiceRecoveryDlgProc(
    _In_ HWND hwndDlg,
    _In_ UINT uMsg,
    _In_ WPARAM wParam,
    _In_ LPARAM lParam
    );

INT_PTR CALLBACK EspServiceRecovery2DlgProc(
    _In_ HWND hwndDlg,
    _In_ UINT uMsg,
    _In_ WPARAM wParam,
    _In_ LPARAM lParam
    );

// srvprgrs

VOID EsRestartServiceWithProgress(
    _In_ HWND hWnd,
    _In_ PPH_SERVICE_ITEM ServiceItem,
    _In_ SC_HANDLE ServiceHandle
    );

// trigger

typedef struct _ES_TRIGGER_INFO
{
    ULONG Type;
    PGUID Subtype;
    ULONG Action;
    PPH_LIST DataList;
    GUID SubtypeBuffer;
} ES_TRIGGER_INFO, * PES_TRIGGER_INFO;

typedef struct _ES_TRIGGER_CONTEXT
{
    PPH_SERVICE_ITEM ServiceItem;
    HWND WindowHandle;
    HWND TriggersLv;
    BOOLEAN Dirty;
    ULONG InitialNumberOfTriggers;
    PPH_LIST InfoList;

    // Trigger dialog box
    PES_TRIGGER_INFO EditingInfo;
    ULONG LastSelectedType;
    PPH_STRING LastCustomSubType;

    // Value dialog box
    PPH_STRING EditingValue;
} ES_TRIGGER_CONTEXT, *PES_TRIGGER_CONTEXT;

PES_TRIGGER_CONTEXT EsCreateServiceTriggerContext(
    _In_ PPH_SERVICE_ITEM ServiceItem,
    _In_ HWND WindowHandle,
    _In_ HWND TriggersLv
    );

VOID EsDestroyServiceTriggerContext(
    _In_ PES_TRIGGER_CONTEXT Context
    );

VOID EsLoadServiceTriggerInfo(
    _In_ PES_TRIGGER_CONTEXT Context,
    _In_ SC_HANDLE ServiceHandle
    );

_Success_(return)
BOOLEAN EsSaveServiceTriggerInfo(
    _In_ PES_TRIGGER_CONTEXT Context,
    _Out_opt_ PULONG Win32Result
    );

#define ES_TRIGGER_EVENT_NEW 1
#define ES_TRIGGER_EVENT_EDIT 2
#define ES_TRIGGER_EVENT_DELETE 3
#define ES_TRIGGER_EVENT_SELECTIONCHANGED 4

VOID EsHandleEventServiceTrigger(
    _In_ PES_TRIGGER_CONTEXT Context,
    _In_ ULONG Event
    );

// triggpg

INT_PTR CALLBACK EspServiceTriggersDlgProc(
    _In_ HWND hwndDlg,
    _In_ UINT uMsg,
    _In_ WPARAM wParam,
    _In_ LPARAM lParam
    );

// pnp

INT_PTR CALLBACK EspPnPServiceDlgProc(
    _In_ HWND hwndDlg,
    _In_ UINT uMsg,
    _In_ WPARAM wParam,
    _In_ LPARAM lParam
    );

// package

INT_PTR CALLBACK EspPackageServiceDlgProc(
    _In_ HWND hwndDlg,
    _In_ UINT uMsg,
    _In_ WPARAM wParam,
    _In_ LPARAM lParam
    );

#endif
