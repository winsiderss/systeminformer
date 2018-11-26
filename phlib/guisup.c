/*
 * Process Hacker -
 *   GUI support functions
 *
 * Copyright (C) 2009-2016 wj32
 * Copyright (C) 2017-2018 dmex
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

#include <ph.h>
#include <guisup.h>
#include <settings.h>
#include <shellapi.h>
#include <uxtheme.h>

#include <guisupp.h>

#define SCALE_DPI(Value) PhMultiplyDivide(Value, PhGlobalDpi, 96)

BOOLEAN NTAPI PhpWindowContextHashtableEqualFunction(
    _In_ PVOID Entry1,
    _In_ PVOID Entry2
    );
ULONG NTAPI PhpWindowContextHashtableHashFunction(
    _In_ PVOID Entry
    );
BOOLEAN NTAPI PhpWindowCallbackHashtableEqualFunction(
    _In_ PVOID Entry1,
    _In_ PVOID Entry2
    );
ULONG NTAPI PhpWindowCallbackHashtableHashFunction(
    _In_ PVOID Entry
    );

typedef struct _PH_WINDOW_PROPERTY_CONTEXT
{
    ULONG PropertyHash;
    HWND WindowHandle;
    PVOID Context;
} PH_WINDOW_PROPERTY_CONTEXT, *PPH_WINDOW_PROPERTY_CONTEXT;

_IsImmersiveProcess IsImmersiveProcess_I = NULL;
_RunFileDlg RunFileDlg = NULL;
_SHAutoComplete SHAutoComplete_I = NULL;

HFONT PhApplicationFont = NULL;
HFONT PhTreeWindowFont = NULL;
PH_INTEGER_PAIR PhSmallIconSize = { 16, 16 };
PH_INTEGER_PAIR PhLargeIconSize = { 32, 32 };

static PH_INITONCE SharedIconCacheInitOnce = PH_INITONCE_INIT;
static PPH_HASHTABLE SharedIconCacheHashtable;
static PH_QUEUED_LOCK SharedIconCacheLock = PH_QUEUED_LOCK_INIT;

static PPH_HASHTABLE WindowCallbackHashTable = NULL;
static PH_QUEUED_LOCK WindowCallbackListLock = PH_QUEUED_LOCK_INIT;
static PPH_HASHTABLE WindowContextHashTable = NULL;
static PH_QUEUED_LOCK WindowContextListLock = PH_QUEUED_LOCK_INIT;

VOID PhGuiSupportInitialization(
    VOID
    )
{
    HDC hdc;
    PVOID shell32Handle;
    PVOID shlwapiHandle;

    WindowCallbackHashTable = PhCreateHashtable(
        sizeof(PH_PLUGIN_WINDOW_CALLBACK_REGISTRATION),
        PhpWindowCallbackHashtableEqualFunction,
        PhpWindowCallbackHashtableHashFunction,
        10
        );
    WindowContextHashTable = PhCreateHashtable(
        sizeof(PH_WINDOW_PROPERTY_CONTEXT),
        PhpWindowContextHashtableEqualFunction,
        PhpWindowContextHashtableHashFunction,
        10
        );

    PhSmallIconSize.X = GetSystemMetrics(SM_CXSMICON);
    PhSmallIconSize.Y = GetSystemMetrics(SM_CYSMICON);
    PhLargeIconSize.X = GetSystemMetrics(SM_CXICON);
    PhLargeIconSize.Y = GetSystemMetrics(SM_CYICON);

    if (hdc = GetDC(NULL))
    {
        PhGlobalDpi = GetDeviceCaps(hdc, LOGPIXELSY);
        ReleaseDC(NULL, hdc);
    }

    shell32Handle = LoadLibrary(L"shell32.dll");
    shlwapiHandle = LoadLibrary(L"shlwapi.dll");

    if (WINDOWS_HAS_IMMERSIVE)
        IsImmersiveProcess_I = PhGetDllProcedureAddress(L"user32.dll", "IsImmersiveProcess", 0);
    RunFileDlg = PhGetDllBaseProcedureAddress(shell32Handle, NULL, 61);
    SHAutoComplete_I = PhGetDllBaseProcedureAddress(shlwapiHandle, "SHAutoComplete", 0);
}

VOID PhSetControlTheme(
    _In_ HWND Handle,
    _In_ PWSTR Theme
    )
{
    SetWindowTheme(Handle, Theme, NULL);
}

INT PhAddListViewColumn(
    _In_ HWND ListViewHandle,
    _In_ INT Index,
    _In_ INT DisplayIndex,
    _In_ INT SubItemIndex,
    _In_ INT Format,
    _In_ INT Width,
    _In_ PWSTR Text
    )
{
    LVCOLUMN column;

    column.mask = LVCF_FMT | LVCF_WIDTH | LVCF_TEXT | LVCF_SUBITEM | LVCF_ORDER;
    column.fmt = Format;
    column.cx = Width < 0 ? -Width : SCALE_DPI(Width);
    column.pszText = Text;
    column.iSubItem = SubItemIndex;
    column.iOrder = DisplayIndex;

    return ListView_InsertColumn(ListViewHandle, Index, &column);
}

INT PhAddListViewItem(
    _In_ HWND ListViewHandle,
    _In_ INT Index,
    _In_ PWSTR Text,
    _In_opt_ PVOID Param
    )
{
    LVITEM item;

    item.mask = LVIF_TEXT | LVIF_PARAM;
    item.iItem = Index;
    item.iSubItem = 0;
    item.pszText = Text;
    item.lParam = (LPARAM)Param;

    return ListView_InsertItem(ListViewHandle, &item);
}

INT PhFindListViewItemByFlags(
    _In_ HWND ListViewHandle,
    _In_ INT StartIndex,
    _In_ ULONG Flags
    )
{
    return ListView_GetNextItem(ListViewHandle, StartIndex, Flags);
}

INT PhFindListViewItemByParam(
    _In_ HWND ListViewHandle,
    _In_ INT StartIndex,
    _In_opt_ PVOID Param
    )
{
    LVFINDINFO findInfo;

    findInfo.flags = LVFI_PARAM;
    findInfo.lParam = (LPARAM)Param;

    return ListView_FindItem(ListViewHandle, StartIndex, &findInfo);
}

LOGICAL PhGetListViewItemImageIndex(
    _In_ HWND ListViewHandle,
    _In_ INT Index,
    _Out_ PINT ImageIndex
    )
{
    LOGICAL result;
    LVITEM item;

    item.mask = LVIF_IMAGE;
    item.iItem = Index;
    item.iSubItem = 0;

    result = ListView_GetItem(ListViewHandle, &item);

    if (!result)
        return result;

    *ImageIndex = item.iImage;

    return result;
}

LOGICAL PhGetListViewItemParam(
    _In_ HWND ListViewHandle,
    _In_ INT Index,
    _Out_ PVOID *Param
    )
{
    LOGICAL result;
    LVITEM item;

    item.mask = LVIF_PARAM;
    item.iItem = Index;
    item.iSubItem = 0;

    result = ListView_GetItem(ListViewHandle, &item);

    if (!result)
        return result;

    *Param = (PVOID)item.lParam;

    return result;
}

VOID PhRemoveListViewItem(
    _In_ HWND ListViewHandle,
    _In_ INT Index
    )
{
    ListView_DeleteItem(ListViewHandle, Index);
}

VOID PhSetListViewItemImageIndex(
    _In_ HWND ListViewHandle,
    _In_ INT Index,
    _In_ INT ImageIndex
    )
{
    LVITEM item;

    item.mask = LVIF_IMAGE;
    item.iItem = Index;
    item.iSubItem = 0;
    item.iImage = ImageIndex;

    ListView_SetItem(ListViewHandle, &item);
}

VOID PhSetListViewSubItem(
    _In_ HWND ListViewHandle,
    _In_ INT Index,
    _In_ INT SubItemIndex,
    _In_ PWSTR Text
    )
{
    LVITEM item;

    item.mask = LVIF_TEXT;
    item.iItem = Index;
    item.iSubItem = SubItemIndex;
    item.pszText = Text;

    ListView_SetItem(ListViewHandle, &item);
}

INT PhAddListViewGroup(
    _In_ HWND ListViewHandle,
    _In_ INT GroupId,
    _In_ PWSTR Text
    )
{
    LVGROUP group;

    memset(&group, 0, sizeof(LVGROUP));
    group.cbSize = sizeof(LVGROUP);
    group.mask = LVGF_HEADER | LVGF_ALIGN | LVGF_STATE | LVGF_GROUPID;
    group.uAlign = LVGA_HEADER_LEFT;
    group.state = LVGS_COLLAPSIBLE;
    group.iGroupId = GroupId;
    group.pszHeader = Text;

    return (INT)ListView_InsertGroup(ListViewHandle, MAXINT, &group);
}

INT PhAddListViewGroupItem(
    _In_ HWND ListViewHandle,
    _In_ INT GroupId,
    _In_ INT Index,
    _In_ PWSTR Text,
    _In_opt_ PVOID Param
    )
{
    LVITEM item;

    item.mask = LVIF_TEXT | LVIF_GROUPID;
    item.iItem = Index;
    item.iSubItem = 0;
    item.pszText = Text;
    item.iGroupId = GroupId;

    if (Param)
    {
        item.mask |= LVIF_PARAM;
        item.lParam = (LPARAM)Param;
    }

    return ListView_InsertItem(ListViewHandle, &item);
}

INT PhAddTabControlTab(
    _In_ HWND TabControlHandle,
    _In_ INT Index,
    _In_ PWSTR Text
    )
{
    TCITEM item;

    item.mask = TCIF_TEXT;
    item.pszText = Text;

    return TabCtrl_InsertItem(TabControlHandle, Index, &item);
}

PPH_STRING PhGetWindowText(
    _In_ HWND hwnd
    )
{
    PPH_STRING text;

    PhGetWindowTextEx(hwnd, 0, &text);
    return text;
}

ULONG PhGetWindowTextEx(
    _In_ HWND hwnd,
    _In_ ULONG Flags,
    _Out_opt_ PPH_STRING *Text
    )
{
    PPH_STRING string;
    ULONG length;

    if (Flags & PH_GET_WINDOW_TEXT_INTERNAL)
    {
        if (Flags & PH_GET_WINDOW_TEXT_LENGTH_ONLY)
        {
            WCHAR buffer[32];
            length = InternalGetWindowText(hwnd, buffer, sizeof(buffer) / sizeof(WCHAR));
        }
        else
        {
            // TODO: Resize the buffer until we get the entire thing.
            string = PhCreateStringEx(NULL, 256 * sizeof(WCHAR));
            length = InternalGetWindowText(hwnd, string->Buffer, (ULONG)string->Length / sizeof(WCHAR) + 1);
            string->Length = length * sizeof(WCHAR);

            if (Text)
                *Text = string;
            else
                PhDereferenceObject(string);
        }

        return length;
    }
    else
    {
        length = GetWindowTextLength(hwnd);

        if (length == 0 || (Flags & PH_GET_WINDOW_TEXT_LENGTH_ONLY))
        {
            if (Text)
                *Text = PhReferenceEmptyString();

            return length;
        }

        string = PhCreateStringEx(NULL, length * sizeof(WCHAR));

        if (GetWindowText(hwnd, string->Buffer, (ULONG)string->Length / sizeof(WCHAR) + 1))
        {
            if (Text)
                *Text = string;
            else
                PhDereferenceObject(string);

            return length;
        }
        else
        {
            if (Text)
                *Text = PhReferenceEmptyString();

            PhDereferenceObject(string);

            return 0;
        }
    }
}

ULONG PhGetWindowTextToBuffer(
    _In_ HWND hwnd,
    _In_ ULONG Flags,
    _Out_writes_bytes_opt_(BufferLength) PWSTR Buffer,
    _In_opt_ ULONG BufferLength,
    _Out_opt_ PULONG ReturnLength
    )
{
    ULONG status = ERROR_SUCCESS;
    ULONG length;

    if (Flags & PH_GET_WINDOW_TEXT_INTERNAL)
        length = InternalGetWindowText(hwnd, Buffer, BufferLength);
    else
        length = GetWindowText(hwnd, Buffer, BufferLength);

    if (length == 0)
        status = GetLastError();

    if (ReturnLength)
        *ReturnLength = length;

    return status;
}

VOID PhAddComboBoxStrings(
    _In_ HWND hWnd,
    _In_ PWSTR *Strings,
    _In_ ULONG NumberOfStrings
    )
{
    ULONG i;

    for (i = 0; i < NumberOfStrings; i++)
        ComboBox_AddString(hWnd, Strings[i]);
}

PPH_STRING PhGetComboBoxString(
    _In_ HWND hwnd,
    _In_ INT Index
    )
{
    PPH_STRING string;
    ULONG length;

    if (Index == -1)
    {
        Index = ComboBox_GetCurSel(hwnd);

        if (Index == -1)
            return NULL;
    }

    length = ComboBox_GetLBTextLen(hwnd, Index);

    if (length == CB_ERR)
        return NULL;
    if (length == 0)
        return PhReferenceEmptyString();

    string = PhCreateStringEx(NULL, length * sizeof(WCHAR));

    if (ComboBox_GetLBText(hwnd, Index, string->Buffer) != CB_ERR)
    {
        return string;
    }
    else
    {
        PhDereferenceObject(string);
        return NULL;
    }
}

INT PhSelectComboBoxString(
    _In_ HWND hwnd,
    _In_ PWSTR String,
    _In_ BOOLEAN Partial
    )
{
    if (Partial)
    {
        return ComboBox_SelectString(hwnd, -1, String);
    }
    else
    {
        INT index;

        index = ComboBox_FindStringExact(hwnd, -1, String);

        if (index == CB_ERR)
            return CB_ERR;

        ComboBox_SetCurSel(hwnd, index);

        return index;
    }
}

PPH_STRING PhGetListBoxString(
    _In_ HWND hwnd,
    _In_ INT Index
    )
{
    PPH_STRING string;
    ULONG length;

    if (Index == -1)
    {
        Index = ListBox_GetCurSel(hwnd);

        if (Index == LB_ERR)
            return NULL;
    }

    length = ListBox_GetTextLen(hwnd, Index);

    if (length == LB_ERR)
        return NULL;
    if (length == 0)
        return PhReferenceEmptyString();

    string = PhCreateStringEx(NULL, length * sizeof(WCHAR));

    if (ListBox_GetText(hwnd, Index, string->Buffer) != LB_ERR)
    {
        return string;
    }
    else
    {
        PhDereferenceObject(string);
        return NULL;
    }
}

VOID PhSetStateAllListViewItems(
    _In_ HWND hWnd,
    _In_ ULONG State,
    _In_ ULONG Mask
    )
{
    ULONG i;
    ULONG count;

    count = ListView_GetItemCount(hWnd);

    if (count == -1)
        return;

    for (i = 0; i < count; i++)
    {
        ListView_SetItemState(hWnd, i, State, Mask);
    }
}

PVOID PhGetSelectedListViewItemParam(
    _In_ HWND hWnd
    )
{
    INT index;
    PVOID param;

    index = PhFindListViewItemByFlags(
        hWnd,
        -1,
        LVNI_SELECTED
        );

    if (index != -1)
    {
        if (PhGetListViewItemParam(
            hWnd,
            index,
            &param
            ))
        {
            return param;
        }
    }

    return NULL;
}

VOID PhGetSelectedListViewItemParams(
    _In_ HWND hWnd,
    _Out_ PVOID **Items,
    _Out_ PULONG NumberOfItems
    )
{
    PH_ARRAY array;
    ULONG index;
    PVOID param;

    PhInitializeArray(&array, sizeof(PVOID), 2);
    index = -1;

    while ((index = PhFindListViewItemByFlags(
        hWnd,
        index,
        LVNI_SELECTED
        )) != -1)
    {
        if (PhGetListViewItemParam(hWnd, index, &param))
            PhAddItemArray(&array, &param);
    }

    *NumberOfItems = (ULONG)array.Count;
    *Items = PhFinalArrayItems(&array);
}

VOID PhSetImageListBitmap(
    _In_ HIMAGELIST ImageList,
    _In_ INT Index,
    _In_ HINSTANCE InstanceHandle,
    _In_ LPCWSTR BitmapName
    )
{
    HBITMAP bitmap;

    bitmap = LoadImage(InstanceHandle, BitmapName, IMAGE_BITMAP, 0, 0, 0);

    if (bitmap)
    {
        ImageList_Replace(ImageList, Index, bitmap, NULL);
        DeleteObject(bitmap);
    }
}

static BOOLEAN SharedIconCacheHashtableEqualFunction(
    _In_ PVOID Entry1,
    _In_ PVOID Entry2
    )
{
    PPHP_ICON_ENTRY entry1 = Entry1;
    PPHP_ICON_ENTRY entry2 = Entry2;

    if (entry1->InstanceHandle != entry2->InstanceHandle ||
        entry1->Width != entry2->Width ||
        entry1->Height != entry2->Height)
    {
        return FALSE;
    }

    if (IS_INTRESOURCE(entry1->Name))
    {
        if (IS_INTRESOURCE(entry2->Name))
            return entry1->Name == entry2->Name;
        else
            return FALSE;
    }
    else
    {
        if (!IS_INTRESOURCE(entry2->Name))
            return PhEqualStringZ(entry1->Name, entry2->Name, FALSE);
        else
            return FALSE;
    }
}

static ULONG SharedIconCacheHashtableHashFunction(
    _In_ PVOID Entry
    )
{
    PPHP_ICON_ENTRY entry = Entry;
    ULONG nameHash;

    if (IS_INTRESOURCE(entry->Name))
        nameHash = PtrToUlong(entry->Name);
    else
        nameHash = PhHashBytes((PUCHAR)entry->Name, PhCountStringZ(entry->Name));

    return nameHash ^ (PtrToUlong(entry->InstanceHandle) >> 5) ^ (entry->Width << 3) ^ entry->Height;
}

HICON PhLoadIcon(
    _In_opt_ HINSTANCE InstanceHandle,
    _In_ PWSTR Name,
    _In_ ULONG Flags,
    _In_opt_ ULONG Width,
    _In_opt_ ULONG Height
    )
{
    PHP_ICON_ENTRY entry;
    PPHP_ICON_ENTRY actualEntry;
    HICON icon = NULL;

    if (PhBeginInitOnce(&SharedIconCacheInitOnce))
    {
        SharedIconCacheHashtable = PhCreateHashtable(sizeof(PHP_ICON_ENTRY),
            SharedIconCacheHashtableEqualFunction, SharedIconCacheHashtableHashFunction, 10);
        PhEndInitOnce(&SharedIconCacheInitOnce);
    }

    if (Flags & PH_LOAD_ICON_SHARED)
    {
        PhAcquireQueuedLockExclusive(&SharedIconCacheLock);

        entry.InstanceHandle = InstanceHandle;
        entry.Name = Name;
        entry.Width = PhpGetIconEntrySize(Width, Flags);
        entry.Height = PhpGetIconEntrySize(Height, Flags);
        actualEntry = PhFindEntryHashtable(SharedIconCacheHashtable, &entry);

        if (actualEntry)
        {
            icon = actualEntry->Icon;
            PhReleaseQueuedLockExclusive(&SharedIconCacheLock);
            return icon;
        }
    }

    if (Flags & (PH_LOAD_ICON_SIZE_SMALL | PH_LOAD_ICON_SIZE_LARGE))
    {
        LoadIconMetric(InstanceHandle, Name, (Flags & PH_LOAD_ICON_SIZE_SMALL) ? LIM_SMALL : LIM_LARGE, &icon);
    }
    else
    {
        LoadIconWithScaleDown(InstanceHandle, Name, Width, Height, &icon);
    }

    if (!icon && !(Flags & PH_LOAD_ICON_STRICT))
    {
        if (Flags & PH_LOAD_ICON_SIZE_SMALL)
        {
            static ULONG smallWidth = 0;
            static ULONG smallHeight = 0;

            if (!smallWidth)
                smallWidth = GetSystemMetrics(SM_CXSMICON);
            if (!smallHeight)
                smallHeight = GetSystemMetrics(SM_CYSMICON);

            Width = smallWidth;
            Height = smallHeight;
        }
        else if (Flags & PH_LOAD_ICON_SIZE_LARGE)
        {
            static ULONG largeWidth = 0;
            static ULONG largeHeight = 0;

            if (!largeWidth)
                largeWidth = GetSystemMetrics(SM_CXICON);
            if (!largeHeight)
                largeHeight = GetSystemMetrics(SM_CYICON);

            Width = largeWidth;
            Height = largeHeight;
        }

        icon = LoadImage(InstanceHandle, Name, IMAGE_ICON, Width, Height, 0);
    }

    if (Flags & PH_LOAD_ICON_SHARED)
    {
        if (icon)
        {
            if (!IS_INTRESOURCE(Name))
                entry.Name = PhDuplicateStringZ(Name);
            entry.Icon = icon;
            PhAddEntryHashtable(SharedIconCacheHashtable, &entry);
        }

        PhReleaseQueuedLockExclusive(&SharedIconCacheLock);
    }

    return icon;
}

/**
 * Gets the default icon used for executable files.
 *
 * \param SmallIcon A variable which receives the small default executable icon. Do not destroy the
 * icon using DestroyIcon(); it is shared between callers.
 * \param LargeIcon A variable which receives the large default executable icon. Do not destroy the
 * icon using DestroyIcon(); it is shared between callers.
 */
VOID PhGetStockApplicationIcon(
    _Out_opt_ HICON *SmallIcon,
    _Out_opt_ HICON *LargeIcon
    )
{
    static PH_INITONCE initOnce = PH_INITONCE_INIT;
    static HICON smallIcon = NULL;
    static HICON largeIcon = NULL;

    // This no longer uses SHGetFileInfo because it is *very* slow and causes many other DLLs to be
    // loaded, increasing memory usage. The worst thing about it, however, is that it is horribly
    // incompatible with multi-threading. The first time it is called, it tries to perform some
    // one-time initialization. It guards this with a lock, but when multiple threads try to call
    // the function at the same time, instead of waiting for initialization to finish it simply
    // fails the other threads.

    if (PhBeginInitOnce(&initOnce))
    {
        if (WindowsVersion < WINDOWS_10)
        {
            PPH_STRING systemDirectory;
            PPH_STRING dllFileName;

            // imageres,11 (Windows 10 and above), user32,0 (Vista and above) or shell32,2 (XP) contains
            // the default application icon.

            if (systemDirectory = PhGetSystemDirectory())
            {
                PH_STRINGREF dllBaseName;

                PhInitializeStringRef(&dllBaseName, L"\\user32.dll");
                dllFileName = PhConcatStringRef2(&systemDirectory->sr, &dllBaseName);

                PhExtractIcon(dllFileName->Buffer, &largeIcon, &smallIcon);

                PhDereferenceObject(dllFileName);
                PhDereferenceObject(systemDirectory);
            }
        }

        // Fallback icons
        if (!smallIcon)
            smallIcon = PhLoadIcon(NULL, IDI_APPLICATION, PH_LOAD_ICON_SIZE_SMALL, 0, 0);
        if (!largeIcon)
            largeIcon = PhLoadIcon(NULL, IDI_APPLICATION, PH_LOAD_ICON_SIZE_LARGE, 0, 0);

        PhEndInitOnce(&initOnce);
    }

    if (SmallIcon)
        *SmallIcon = smallIcon;
    if (LargeIcon)
        *LargeIcon = largeIcon;
}

HICON PhGetFileShellIcon(
    _In_opt_ PWSTR FileName,
    _In_opt_ PWSTR DefaultExtension,
    _In_ BOOLEAN LargeIcon
    )
{
    SHFILEINFO fileInfo;
    ULONG iconFlag;
    HICON icon;

    if (DefaultExtension && PhEqualStringZ(DefaultExtension, L".exe", TRUE))
    {
        // Special case for executable files (see above for reasoning).

        icon = NULL;

        if (FileName)
        {
            PhExtractIcon(
                FileName,
                LargeIcon ? &icon : NULL,
                !LargeIcon ? &icon : NULL
                );
        }

        if (!icon)
        {
            PhGetStockApplicationIcon(
                !LargeIcon ? &icon : NULL,
                LargeIcon ? &icon : NULL
                );

            if (icon)
                icon = CopyIcon(icon);
        }

        return icon;
    }

    iconFlag = LargeIcon ? SHGFI_LARGEICON : SHGFI_SMALLICON;
    icon = NULL;

    if (FileName && SHGetFileInfo(
        FileName,
        0,
        &fileInfo,
        sizeof(SHFILEINFO),
        SHGFI_ICON | iconFlag
        ))
    {
        icon = fileInfo.hIcon;
    }

    if (!icon && DefaultExtension)
    {
        if (SHGetFileInfo(
            DefaultExtension,
            FILE_ATTRIBUTE_NORMAL,
            &fileInfo,
            sizeof(SHFILEINFO),
            SHGFI_ICON | iconFlag | SHGFI_USEFILEATTRIBUTES
            ))
            icon = fileInfo.hIcon;
    }

    return icon;
}

VOID PhpSetClipboardData(
    _In_ HWND hWnd,
    _In_ ULONG Format,
    _In_ HANDLE Data
    )
{
    if (OpenClipboard(hWnd))
    {
        if (!EmptyClipboard())
            goto Fail;

        if (!SetClipboardData(Format, Data))
            goto Fail;

        CloseClipboard();

        return;
    }

Fail:
    GlobalFree(Data);
}

VOID PhSetClipboardString(
    _In_ HWND hWnd,
    _In_ PPH_STRINGREF String
    )
{
    HANDLE data;
    PVOID memory;

    data = GlobalAlloc(GMEM_MOVEABLE, String->Length + sizeof(WCHAR));
    memory = GlobalLock(data);

    memcpy(memory, String->Buffer, String->Length);
    *(PWCHAR)PTR_ADD_OFFSET(memory, String->Length) = 0;

    GlobalUnlock(memory);

    PhpSetClipboardData(hWnd, CF_UNICODETEXT, data);
}

HWND PhCreateDialogFromTemplate(
    _In_ HWND Parent,
    _In_ ULONG Style,
    _In_ PVOID Instance,
    _In_ PWSTR Template,
    _In_ DLGPROC DialogProc,
    _In_ PVOID Parameter
    )
{
    PDLGTEMPLATEEX dialogTemplate;
    HWND dialogHandle;

    if (!PhLoadResource(Instance, Template, RT_DIALOG, NULL, &dialogTemplate))
        return NULL;

    if (dialogTemplate->signature == USHRT_MAX)
    {
        dialogTemplate->style = Style;
    }
    else
    {
        ((DLGTEMPLATE *)dialogTemplate)->style = Style;
    }

    dialogHandle = CreateDialogIndirectParam(
        Instance, 
        (DLGTEMPLATE *)dialogTemplate, 
        Parent, 
        DialogProc, 
        (LPARAM)Parameter
        );

    PhFree(dialogTemplate);

    return dialogHandle;
}

BOOLEAN PhModalPropertySheet(
    _Inout_ PROPSHEETHEADER *Header
    )
{
    // PropertySheet incorrectly discards WM_QUIT messages in certain cases, so we will use our own
    // message loop. An example of this is when GetMessage (called by PropertySheet's message loop)
    // dispatches a message directly from kernel-mode that causes the property sheet to close. In
    // that case PropertySheet will retrieve the WM_QUIT message but will ignore it because of its
    // buggy logic.

    // This is also a good opportunity to introduce an auto-pool.

    PH_AUTO_POOL autoPool;
    HWND oldFocus;
    HWND topLevelOwner;
    HWND hwnd;
    BOOL result;
    MSG message;

    PhInitializeAutoPool(&autoPool);

    oldFocus = GetFocus();
    topLevelOwner = Header->hwndParent;

    while (topLevelOwner && (GetWindowLongPtr(topLevelOwner, GWL_STYLE) & WS_CHILD))
        topLevelOwner = GetParent(topLevelOwner);

    if (topLevelOwner && (topLevelOwner == GetDesktopWindow() || EnableWindow(topLevelOwner, FALSE)))
        topLevelOwner = NULL;

    Header->dwFlags |= PSH_MODELESS;
    hwnd = (HWND)PropertySheet(Header);

    if (!hwnd)
    {
        if (topLevelOwner)
            EnableWindow(topLevelOwner, TRUE);

        return FALSE;
    }

    while (result = GetMessage(&message, NULL, 0, 0))
    {
        if (result == -1)
            break;

        if (!PropSheet_IsDialogMessage(hwnd, &message))
        {
            TranslateMessage(&message);
            DispatchMessage(&message);
        }

        PhDrainAutoPool(&autoPool);

        // Destroy the window when necessary.
        if (!PropSheet_GetCurrentPageHwnd(hwnd))
            break;
    }

    if (result == 0)
        PostQuitMessage((INT)message.wParam);
    if (Header->hwndParent && GetActiveWindow() == hwnd)
        SetActiveWindow(Header->hwndParent);
    if (topLevelOwner)
        EnableWindow(topLevelOwner, TRUE);
    if (oldFocus && IsWindow(oldFocus))
        SetFocus(oldFocus);

    DestroyWindow(hwnd);
    PhDeleteAutoPool(&autoPool);

    return TRUE;
}

VOID PhInitializeLayoutManager(
    _Out_ PPH_LAYOUT_MANAGER Manager,
    _In_ HWND RootWindowHandle
    )
{
    Manager->List = PhCreateList(4);
    Manager->LayoutNumber = 0;

    Manager->RootItem.Handle = RootWindowHandle;
    GetClientRect(Manager->RootItem.Handle, &Manager->RootItem.Rect);
    Manager->RootItem.OrigRect = Manager->RootItem.Rect;
    Manager->RootItem.ParentItem = NULL;
    Manager->RootItem.LayoutParentItem = NULL;
    Manager->RootItem.LayoutNumber = 0;
    Manager->RootItem.NumberOfChildren = 0;
    Manager->RootItem.DeferHandle = NULL;
}

VOID PhDeleteLayoutManager(
    _Inout_ PPH_LAYOUT_MANAGER Manager
    )
{
    ULONG i;

    for (i = 0; i < Manager->List->Count; i++)
        PhFree(Manager->List->Items[i]);

    PhDereferenceObject(Manager->List);
}

// HACK: The math below is all horribly broken, especially the HACK for multiline tab controls.

PPH_LAYOUT_ITEM PhAddLayoutItem(
    _Inout_ PPH_LAYOUT_MANAGER Manager,
    _In_ HWND Handle,
    _In_opt_ PPH_LAYOUT_ITEM ParentItem,
    _In_ ULONG Anchor
    )
{
    PPH_LAYOUT_ITEM layoutItem;
    RECT dummy = { 0 };

    layoutItem = PhAddLayoutItemEx(
        Manager,
        Handle,
        ParentItem,
        Anchor,
        dummy
        );

    layoutItem->Margin = layoutItem->Rect;
    PhConvertRect(&layoutItem->Margin, &layoutItem->ParentItem->Rect);

    if (layoutItem->ParentItem != layoutItem->LayoutParentItem)
    {
        // Fix the margin because the item has a dummy parent. They share the same layout parent
        // item.
        layoutItem->Margin.top -= layoutItem->ParentItem->Rect.top;
        layoutItem->Margin.left -= layoutItem->ParentItem->Rect.left;
        layoutItem->Margin.right = layoutItem->ParentItem->Margin.right;
        layoutItem->Margin.bottom = layoutItem->ParentItem->Margin.bottom;
    }

    return layoutItem;
}

PPH_LAYOUT_ITEM PhAddLayoutItemEx(
    _Inout_ PPH_LAYOUT_MANAGER Manager,
    _In_ HWND Handle,
    _In_opt_ PPH_LAYOUT_ITEM ParentItem,
    _In_ ULONG Anchor,
    _In_ RECT Margin
    )
{
    PPH_LAYOUT_ITEM item;

    if (!ParentItem)
        ParentItem = &Manager->RootItem;

    item = PhAllocate(sizeof(PH_LAYOUT_ITEM));
    item->Handle = Handle;
    item->ParentItem = ParentItem;
    item->LayoutNumber = Manager->LayoutNumber;
    item->NumberOfChildren = 0;
    item->DeferHandle = NULL;
    item->Anchor = Anchor;

    item->LayoutParentItem = item->ParentItem;

    while ((item->LayoutParentItem->Anchor & PH_LAYOUT_DUMMY_MASK) &&
        item->LayoutParentItem->LayoutParentItem)
    {
        item->LayoutParentItem = item->LayoutParentItem->LayoutParentItem;
    }

    item->LayoutParentItem->NumberOfChildren++;

    GetWindowRect(Handle, &item->Rect);
    MapWindowPoints(NULL, item->LayoutParentItem->Handle, (POINT *)&item->Rect, 2);

    if (item->Anchor & PH_LAYOUT_TAB_CONTROL)
    {
        // We want to convert the tab control rectangle to the tab page display rectangle.
        TabCtrl_AdjustRect(Handle, FALSE, &item->Rect);
    }

    item->Margin = Margin;

    item->OrigRect = item->Rect;

    PhAddItemList(Manager->List, item);

    return item;
}

VOID PhpLayoutItemLayout(
    _Inout_ PPH_LAYOUT_MANAGER Manager,
    _Inout_ PPH_LAYOUT_ITEM Item
    )
{
    RECT rect;
    BOOLEAN hasDummyParent;

    if (Item->NumberOfChildren > 0 && !Item->DeferHandle)
        Item->DeferHandle = BeginDeferWindowPos(Item->NumberOfChildren);

    if (Item->LayoutNumber == Manager->LayoutNumber)
        return;

    // If this is the root item we must stop here.
    if (!Item->ParentItem)
        return;

    PhpLayoutItemLayout(Manager, Item->ParentItem);

    if (Item->ParentItem != Item->LayoutParentItem)
    {
        PhpLayoutItemLayout(Manager, Item->LayoutParentItem);
        hasDummyParent = TRUE;
    }
    else
    {
        hasDummyParent = FALSE;
    }

    GetWindowRect(Item->Handle, &Item->Rect);
    MapWindowPoints(NULL, Item->LayoutParentItem->Handle, (POINT *)&Item->Rect, 2);

    if (Item->Anchor & PH_LAYOUT_TAB_CONTROL)
    {
        // We want to convert the tab control rectangle to the tab page display rectangle.
        TabCtrl_AdjustRect(Item->Handle, FALSE, &Item->Rect);
    }

    if (!(Item->Anchor & PH_LAYOUT_DUMMY_MASK))
    {
        // Convert right/bottom into margins to make the calculations
        // easier.
        rect = Item->Rect;
        PhConvertRect(&rect, &Item->LayoutParentItem->Rect);

        if (!(Item->Anchor & (PH_ANCHOR_LEFT | PH_ANCHOR_RIGHT)))
        {
            // TODO
            PhRaiseStatus(STATUS_NOT_IMPLEMENTED);
        }
        else if (Item->Anchor & PH_ANCHOR_RIGHT)
        {
            if (Item->Anchor & PH_ANCHOR_LEFT)
            {
                rect.left = (hasDummyParent ? Item->ParentItem->Rect.left : 0) + Item->Margin.left;
                rect.right = Item->Margin.right;
            }
            else
            {
                ULONG diff = Item->Margin.right - rect.right;

                rect.left -= diff;
                rect.right += diff;
            }
        }

        if (!(Item->Anchor & (PH_ANCHOR_TOP | PH_ANCHOR_BOTTOM)))
        {
            // TODO
            PhRaiseStatus(STATUS_NOT_IMPLEMENTED);
        }
        else if (Item->Anchor & PH_ANCHOR_BOTTOM)
        {
            if (Item->Anchor & PH_ANCHOR_TOP)
            {
                // tab control hack
                rect.top = (hasDummyParent ? Item->ParentItem->Rect.top : 0) + Item->Margin.top;
                rect.bottom = Item->Margin.bottom;
            }
            else
            {
                ULONG diff = Item->Margin.bottom - rect.bottom;

                rect.top -= diff;
                rect.bottom += diff;
            }
        }

        // Convert the right/bottom back into co-ordinates.
        PhConvertRect(&rect, &Item->LayoutParentItem->Rect);
        Item->Rect = rect;

        if (!(Item->Anchor & PH_LAYOUT_IMMEDIATE_RESIZE))
        {
            Item->LayoutParentItem->DeferHandle = DeferWindowPos(
                Item->LayoutParentItem->DeferHandle, Item->Handle,
                NULL, rect.left, rect.top,
                rect.right - rect.left, rect.bottom - rect.top,
                SWP_NOACTIVATE | SWP_NOZORDER
                );
        }
        else
        {
            // This is needed for tab controls, so that TabCtrl_AdjustRect will give us an
            // up-to-date result.
            SetWindowPos(
                Item->Handle,
                NULL, rect.left, rect.top,
                rect.right - rect.left, rect.bottom - rect.top,
                SWP_NOACTIVATE | SWP_NOZORDER
                );
        }
    }

    Item->LayoutNumber = Manager->LayoutNumber;
}

VOID PhLayoutManagerLayout(
    _Inout_ PPH_LAYOUT_MANAGER Manager
    )
{
    ULONG i;

    Manager->LayoutNumber++;

    GetClientRect(Manager->RootItem.Handle, &Manager->RootItem.Rect);

    for (i = 0; i < Manager->List->Count; i++)
    {
        PPH_LAYOUT_ITEM item = (PPH_LAYOUT_ITEM)Manager->List->Items[i];

        PhpLayoutItemLayout(Manager, item);
    }

    for (i = 0; i < Manager->List->Count; i++)
    {
        PPH_LAYOUT_ITEM item = (PPH_LAYOUT_ITEM)Manager->List->Items[i];

        if (item->DeferHandle)
        {
            EndDeferWindowPos(item->DeferHandle);
            item->DeferHandle = NULL;
        }

        if (item->Anchor & PH_LAYOUT_FORCE_INVALIDATE)
        {
            InvalidateRect(item->Handle, NULL, FALSE);
        }
    }

    if (Manager->RootItem.DeferHandle)
    {
        EndDeferWindowPos(Manager->RootItem.DeferHandle);
        Manager->RootItem.DeferHandle = NULL;
    }
}

static BOOLEAN NTAPI PhpWindowContextHashtableEqualFunction(
    _In_ PVOID Entry1,
    _In_ PVOID Entry2
    )
{
    PPH_WINDOW_PROPERTY_CONTEXT entry1 = Entry1;
    PPH_WINDOW_PROPERTY_CONTEXT entry2 = Entry2;

    return 
        entry1->WindowHandle == entry2->WindowHandle &&
        entry1->PropertyHash == entry2->PropertyHash;
}

static ULONG NTAPI PhpWindowContextHashtableHashFunction(
    _In_ PVOID Entry
    )
{
    PPH_WINDOW_PROPERTY_CONTEXT entry = Entry;

    return PhHashIntPtr((ULONG_PTR)entry->WindowHandle) ^ PhHashInt32(entry->PropertyHash);
}

PVOID PhGetWindowContext(
    _In_ HWND WindowHandle,
    _In_ ULONG PropertyHash
    )
{
    PH_WINDOW_PROPERTY_CONTEXT lookupEntry;
    PPH_WINDOW_PROPERTY_CONTEXT entry;

    lookupEntry.WindowHandle = WindowHandle;
    lookupEntry.PropertyHash = PropertyHash;

    PhAcquireQueuedLockShared(&WindowContextListLock);
    entry = PhFindEntryHashtable(WindowContextHashTable, &lookupEntry);
    PhReleaseQueuedLockShared(&WindowContextListLock);

    if (entry)
        return entry->Context;
    else
        return NULL;
}

VOID PhSetWindowContext(
    _In_ HWND WindowHandle,
    _In_ ULONG PropertyHash,
    _In_ PVOID Context
    )
{
    PH_WINDOW_PROPERTY_CONTEXT entry;

    entry.WindowHandle = WindowHandle;
    entry.PropertyHash = PropertyHash;
    entry.Context = Context;

    PhAcquireQueuedLockExclusive(&WindowContextListLock);
    PhAddEntryHashtable(WindowContextHashTable, &entry);
    PhReleaseQueuedLockExclusive(&WindowContextListLock);
}

VOID PhRemoveWindowContext(
    _In_ HWND WindowHandle,
    _In_ ULONG PropertyHash
    )
{
    PH_WINDOW_PROPERTY_CONTEXT lookupEntry;

    lookupEntry.WindowHandle = WindowHandle;
    lookupEntry.PropertyHash = PropertyHash;

    PhAcquireQueuedLockExclusive(&WindowContextListLock);
    PhRemoveEntryHashtable(WindowContextHashTable, &lookupEntry);
    PhReleaseQueuedLockExclusive(&WindowContextListLock);
}

VOID PhEnumChildWindows(
    _In_opt_ HWND WindowHandle,
    _In_ ULONG Limit,
    _In_ PH_CHILD_ENUM_CALLBACK Callback,
    _In_ PVOID Context
    )
{
    HWND childWindow = NULL;
    ULONG i = 0;

    while (i < Limit && (childWindow = FindWindowEx(WindowHandle, childWindow, NULL, NULL)))
    {
        if (!Callback(childWindow, Context))
            return;

        i++;
    }
}

typedef struct _GET_PROCESS_MAIN_WINDOW_CONTEXT
{
    HWND Window;
    HWND ImmersiveWindow;
    HANDLE ProcessId;
    BOOLEAN IsImmersive;
    BOOLEAN SkipInvisible;
} GET_PROCESS_MAIN_WINDOW_CONTEXT, *PGET_PROCESS_MAIN_WINDOW_CONTEXT;

BOOLEAN CALLBACK PhpGetProcessMainWindowEnumWindowsProc(
    _In_ HWND WindowHandle,
    _In_opt_ PVOID Context
    )
{
    PGET_PROCESS_MAIN_WINDOW_CONTEXT context = (PGET_PROCESS_MAIN_WINDOW_CONTEXT)Context;
    ULONG processId;
    HWND parentWindow;
    WINDOWINFO windowInfo;

    if (context->SkipInvisible && !IsWindowVisible(WindowHandle))
        return TRUE;

    GetWindowThreadProcessId(WindowHandle, &processId);

    if (UlongToHandle(processId) == context->ProcessId && (context->SkipInvisible ?
        !((parentWindow = GetParent(WindowHandle)) && IsWindowVisible(parentWindow)) && // skip windows with a visible parent
        PhGetWindowTextEx(WindowHandle, PH_GET_WINDOW_TEXT_INTERNAL | PH_GET_WINDOW_TEXT_LENGTH_ONLY, NULL) != 0 : TRUE)) // skip windows with no title
    {
        if (!context->ImmersiveWindow && context->IsImmersive &&
            GetProp(WindowHandle, L"Windows.ImmersiveShell.IdentifyAsMainCoreWindow"))
        {
            context->ImmersiveWindow = WindowHandle;
        }

        windowInfo.cbSize = sizeof(WINDOWINFO);

        if (!context->Window && GetWindowInfo(WindowHandle, &windowInfo) && (windowInfo.dwStyle & WS_DLGFRAME))
        {
            context->Window = WindowHandle;

            // If we're not looking at an immersive process, there's no need to search any more windows.
            if (!context->IsImmersive)
                return FALSE;
        }
    }

    return TRUE;
}

HWND PhGetProcessMainWindow(
    _In_ HANDLE ProcessId,
    _In_opt_ HANDLE ProcessHandle
    )
{
    return PhGetProcessMainWindowEx(ProcessId, ProcessHandle, TRUE);
}

HWND PhGetProcessMainWindowEx(
    _In_ HANDLE ProcessId,
    _In_opt_ HANDLE ProcessHandle,
    _In_ BOOLEAN SkipInvisible
    )
{
    GET_PROCESS_MAIN_WINDOW_CONTEXT context;
    HANDLE processHandle = NULL;

    memset(&context, 0, sizeof(GET_PROCESS_MAIN_WINDOW_CONTEXT));
    context.ProcessId = ProcessId;
    context.SkipInvisible = SkipInvisible;

    if (ProcessHandle)
        processHandle = ProcessHandle;
    else
        PhOpenProcess(&processHandle, PROCESS_QUERY_LIMITED_INFORMATION, ProcessId);

    if (processHandle && IsImmersiveProcess_I)
        context.IsImmersive = IsImmersiveProcess_I(processHandle);

    PhEnumChildWindows(NULL, 0x800, PhpGetProcessMainWindowEnumWindowsProc, &context);

    if (!ProcessHandle && processHandle)
        NtClose(processHandle);

    return context.ImmersiveWindow ? context.ImmersiveWindow : context.Window;
}

ULONG PhGetDialogItemValue(
    _In_ HWND WindowHandle,
    _In_ INT ControlID
    )
{
    ULONG64 controlValue = 0;
    HWND controlHandle;
    PPH_STRING controlText;

    if (controlHandle = GetDlgItem(WindowHandle, ControlID))
    {
        if (controlText = PhGetWindowText(controlHandle))
        {
            PhStringToInteger64(&controlText->sr, 10, &controlValue);
            PhDereferenceObject(controlText);
        }
    }

    return (ULONG)controlValue;
}

VOID PhSetDialogItemValue(
    _In_ HWND WindowHandle,
    _In_ INT ControlID,
    _In_ ULONG Value,
    _In_ BOOLEAN Signed
    )
{
    HWND controlHandle;
    WCHAR valueString[PH_INT32_STR_LEN_1];

    if (Signed)
        PhPrintInt32(valueString, (LONG)Value);
    else
        PhPrintUInt32(valueString, Value);

    if (controlHandle = GetDlgItem(WindowHandle, ControlID))
    {
        PhSetWindowText(controlHandle, valueString);
    }
}

VOID PhSetDialogItemText(
    _In_ HWND WindowHandle,
    _In_ INT ControlID,
    _In_ PCWSTR WindowText
    )
{
    HWND controlHandle;

    if (controlHandle = GetDlgItem(WindowHandle, ControlID))
    {
        PhSetWindowText(controlHandle, WindowText);
    }
}

VOID PhSetWindowText(
    _In_ HWND WindowHandle,
    _In_ PCWSTR WindowText
    )
{
    SendMessage(WindowHandle, WM_SETTEXT, 0, (LPARAM)WindowText); // TODO: DefWindowProc (dmex)
}

VOID PhSetWindowAlwaysOnTop(
    _In_ HWND WindowHandle,
    _In_ BOOLEAN AlwaysOnTop
    )
{
    SetFocus(WindowHandle); // HACK - SetWindowPos doesn't work properly without this (wj32)
    SetWindowPos(
        WindowHandle,
        AlwaysOnTop ? HWND_TOPMOST : HWND_NOTOPMOST, 
        0, 0, 0, 0,
        SWP_NOACTIVATE | SWP_NOMOVE | SWP_NOSIZE
        );
}

static BOOLEAN NTAPI PhpWindowCallbackHashtableEqualFunction(
    _In_ PVOID Entry1,
    _In_ PVOID Entry2
    )
{
    return
        (*(PPH_PLUGIN_WINDOW_CALLBACK_REGISTRATION *)Entry1)->WindowHandle ==
        (*(PPH_PLUGIN_WINDOW_CALLBACK_REGISTRATION *)Entry2)->WindowHandle;
}

static ULONG NTAPI PhpWindowCallbackHashtableHashFunction(
    _In_ PVOID Entry
    )
{
    return PhHashIntPtr((ULONG_PTR)(*(PPH_PLUGIN_WINDOW_CALLBACK_REGISTRATION *)Entry)->WindowHandle);
}

VOID PhRegisterWindowCallback(
    _In_ HWND WindowHandle,
    _In_ PH_PLUGIN_WINDOW_EVENT_TYPE Type,
    _In_ PVOID Context
    )
{
    PPH_PLUGIN_WINDOW_CALLBACK_REGISTRATION entry;

    entry = PhAllocate(sizeof(PH_PLUGIN_WINDOW_CALLBACK_REGISTRATION));
    entry->WindowHandle = WindowHandle;
    entry->Type = Type;

    switch (Type) // HACK
    {
    case PH_PLUGIN_WINDOW_EVENT_TYPE_TOPMOST:
        if (PhGetIntegerSetting(L"MainWindowAlwaysOnTop"))
            PhSetWindowAlwaysOnTop(WindowHandle, TRUE);
        break;
    }

    PhAcquireQueuedLockExclusive(&WindowCallbackListLock);
    PhAddEntryHashtable(WindowCallbackHashTable, &entry);
    PhReleaseQueuedLockExclusive(&WindowCallbackListLock);
}

VOID PhUnregisterWindowCallback(
    _In_ HWND WindowHandle
    )
{
    PH_PLUGIN_WINDOW_CALLBACK_REGISTRATION lookupEntry;
    PPH_PLUGIN_WINDOW_CALLBACK_REGISTRATION lookupEntryPtr = &lookupEntry;
    PPH_PLUGIN_WINDOW_CALLBACK_REGISTRATION *entryPtr;
    PPH_PLUGIN_WINDOW_CALLBACK_REGISTRATION entry;

    lookupEntry.WindowHandle = WindowHandle;

    PhAcquireQueuedLockExclusive(&WindowCallbackListLock);

    entryPtr = (PPH_PLUGIN_WINDOW_CALLBACK_REGISTRATION*)PhFindEntryHashtable(
        WindowCallbackHashTable,
        &lookupEntryPtr
        );

    assert(entryPtr);

    if (entryPtr && PhRemoveEntryHashtable(WindowCallbackHashTable, entryPtr))
    {
        entry = *entryPtr;
        PhFree(entry);
    }

    PhReleaseQueuedLockExclusive(&WindowCallbackListLock);
}

VOID PhWindowNotifyTopMostEvent(
    _In_ BOOLEAN TopMost
    )
{
    PPH_PLUGIN_WINDOW_CALLBACK_REGISTRATION *entry;
    ULONG i = 0;

    PhAcquireQueuedLockExclusive(&WindowCallbackListLock);

    while (PhEnumHashtable(WindowCallbackHashTable, (PVOID*)&entry, &i))
    {
        if ((*entry)->Type & PH_PLUGIN_WINDOW_EVENT_TYPE_TOPMOST)
        {
            PhSetWindowAlwaysOnTop((*entry)->WindowHandle, TopMost);
        }
    }

    PhReleaseQueuedLockExclusive(&WindowCallbackListLock);
}
