/*
 * Process Hacker Extra Plugins -
 *   Boot Entries Plugin
 *
 * Copyright (C) 2015 dmex
 *
 * This file is part of Process Hacker.
 *
 * Process Hacker is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation, either version 3 of the License, or
 * (at your option) any later version.
 *
 * Process Hacker is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with Process Hacker.  If not, see <http://www.gnu.org/licenses/>.
 */

#ifndef _BOOT_H_
#define _BOOT_H_

#define BOOT_ENTRIES_MENUITEM 1000
#define PLUGIN_NAME L"dmex.BootEntriesPlugin"
#define SETTING_NAME_WINDOW_POSITION (PLUGIN_NAME L".WindowPosition")
#define SETTING_NAME_WINDOW_SIZE (PLUGIN_NAME L".WindowSize")
#define SETTING_NAME_LISTVIEW_COLUMNS (PLUGIN_NAME L".ListViewColumns")

#define CINTERFACE
#define COBJMACROS
#include <phdk.h>
#include <phappresource.h>
#include "resource.h"

typedef struct _BOOT_WINDOW_CONTEXT
{
    HWND ListViewHandle;
    PH_LAYOUT_MANAGER LayoutManager;
} BOOT_WINDOW_CONTEXT, *PBOOT_WINDOW_CONTEXT;

typedef BOOLEAN (NTAPI *PPH_BOOT_ENTRY_CALLBACK)(
    _In_ PBOOT_ENTRY BootEntry,
    _In_ PVOID Context
    );

#define EFI_DRIVER_ENTRY_VERSION 1

#define FILE_PATH_VERSION            1
#define FILE_PATH_TYPE_ARC           1
#define FILE_PATH_TYPE_ARC_SIGNATURE 2
#define FILE_PATH_TYPE_NT            3
#define FILE_PATH_TYPE_EFI           4

#define FILE_PATH_TYPE_MIN FILE_PATH_TYPE_ARC
#define FILE_PATH_TYPE_MAX FILE_PATH_TYPE_EFI

#define WINDOWS_OS_OPTIONS_VERSION 1
#define WINDOWS_OS_OPTIONS_SIGNATURE "WINDOWS"

#define BOOT_OPTIONS_VERSION 1
#define BOOT_OPTIONS_FIELD_TIMEOUT              0x00000001
#define BOOT_OPTIONS_FIELD_NEXT_BOOT_ENTRY_ID   0x00000002
#define BOOT_OPTIONS_FIELD_HEADLESS_REDIRECTION 0x00000004

#define BOOT_ENTRY_VERSION 1
#define BOOT_ENTRY_ATTRIBUTE_ACTIVE             0x00000001
#define BOOT_ENTRY_ATTRIBUTE_DEFAULT            0x00000002
#define BOOT_ENTRY_ATTRIBUTE_WINDOWS            0x00000004
#define BOOT_ENTRY_ATTRIBUTE_REMOVABLE_MEDIA    0x00000008
#define BOOT_ENTRY_ATTRIBUTE_VALID_BITS (  \
            BOOT_ENTRY_ATTRIBUTE_ACTIVE  | \
            BOOT_ENTRY_ATTRIBUTE_DEFAULT   \
            )

//typedef struct _WINDOWS_OS_OPTIONS
//{
//    UCHAR Signature[8];
//    ULONG Version;
//    ULONG Length;
//    ULONG OsLoadPathOffset; //FILE_PATH OsLoadPath;
//    WCHAR OsLoadOptions[1];
//} WINDOWS_OS_OPTIONS, *PWINDOWS_OS_OPTIONS;

typedef NTSTATUS (NTAPI* _NtAddBootEntry)(
    _In_ PBOOT_ENTRY BootEntry,
    _Out_opt_ PULONG Id
    );

typedef NTSTATUS (NTAPI* _NtDeleteBootEntry)(
    _In_ ULONG Id
    );

typedef NTSTATUS (NTAPI* _NtModifyBootEntry)(
    _In_ PBOOT_ENTRY BootEntry
    );

typedef NTSTATUS (NTAPI* _NtEnumerateBootEntries)(
    _Out_writes_bytes_opt_(*BufferLength) PVOID Buffer,
    _Inout_ PULONG BufferLength
    );

typedef NTSTATUS (NTAPI* _NtQueryBootEntryOrder)(
    _Out_writes_opt_(*Count) PULONG Ids,
    _Inout_ PULONG Count
    );

typedef NTSTATUS (NTAPI* _NtSetBootEntryOrder)(
    _In_reads_(Count) PULONG Ids,
    _In_ ULONG Count
    );

typedef NTSTATUS (NTAPI* _NtQueryBootOptions)(
    _Out_writes_bytes_opt_(*BootOptionsLength) PBOOT_OPTIONS BootOptions,
    _Inout_ PULONG BootOptionsLength
    );

typedef NTSTATUS (NTAPI* _NtSetBootOptions)(
    _In_ PBOOT_OPTIONS BootOptions,
    _In_ ULONG FieldsToChange
    );

typedef NTSTATUS (NTAPI* _NtTranslateFilePath)(
    _In_ PFILE_PATH InputFilePath,
    _In_ ULONG OutputType,
    _Out_writes_bytes_opt_(*OutputFilePathLength) PFILE_PATH OutputFilePath,
    _Inout_opt_ PULONG OutputFilePathLength
    );

#endif _BOOT_H_