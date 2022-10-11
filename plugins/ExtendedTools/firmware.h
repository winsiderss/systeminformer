/*
 * Copyright (c) 2022 Winsider Seminars & Solutions, Inc.  All rights reserved.
 *
 * This file is part of System Informer.
 *
 * Authors:
 *
 *     dmex    2016-2017
 *
 */

#ifndef FIRMWARE_H
#define FIRMWARE_H

#define CINTERFACE
#define COBJMACROS
#define INITGUID
#include <phdk.h>
#include <phappresource.h>
#include <settings.h>
#include <hexedit.h>

#include <windowsx.h>
#include <stdint.h>
#include <cguid.h>

#include "resource.h"

extern PPH_PLUGIN PluginInstance;

typedef struct _UEFI_WINDOW_CONTEXT
{
    HWND ListViewHandle;
    PH_LAYOUT_MANAGER LayoutManager;
} UEFI_WINDOW_CONTEXT, *PUEFI_WINDOW_CONTEXT;

typedef struct _EFI_ENTRY
{
    ULONG Length;
    PPH_STRING Name;
    PPH_STRING GuidString;
} EFI_ENTRY, *PEFI_ENTRY;

VOID EtShowFirmwareEditDialog(
    _In_ HWND ParentWindowHandle,
    _In_ PEFI_ENTRY Entry
    );

// Windows types

#define PH_FIRST_BOOT_ENTRY(Variables) ((PBOOT_ENTRY_LIST)(Variables))
#define PH_NEXT_BOOT_ENTRY(Variable) ( \
    ((PBOOT_ENTRY_LIST)(Variable))->NextEntryOffset ? \
    (PBOOT_ENTRY_LIST)((PCHAR)(Variable) + \
    ((PBOOT_ENTRY_LIST)(Variable))->NextEntryOffset) : \
    NULL \
    )

#define FILE_PATH_TYPE_MIN FILE_PATH_TYPE_ARC
#define FILE_PATH_TYPE_MAX FILE_PATH_TYPE_EFI

#define WINDOWS_OS_OPTIONS_VERSION 1
#define WINDOWS_OS_OPTIONS_SIGNATURE "WINDOWS"

typedef struct _WINDOWS_OS_OPTIONS
{
    UCHAR Signature[8];
    ULONG Version;
    ULONG Length;
    ULONG OsLoadPathOffset; //FILE_PATH OsLoadPath;
    WCHAR OsLoadOptions[1];
} WINDOWS_OS_OPTIONS, *PWINDOWS_OS_OPTIONS;

#endif
