/*
 * Copyright (c) 2022 Winsider Seminars & Solutions, Inc.  All rights reserved.
 *
 * This file is part of System Informer.
 *
 * Authors:
 *
 *     wj32    2009-2016
 *     dmex    2017-2022
 *
 */

#include <ph.h>
#include <apiimport.h>
#include <guisup.h>
#include <mapimg.h>
#include <settings.h>
#include <fastlock.h>
#include <guisupp.h>

#include <math.h>
#include <commoncontrols.h>
#include <shellapi.h>
#include <uxtheme.h>
#include <wincodec.h>

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

HFONT PhApplicationFont = NULL;
HFONT PhTreeWindowFont = NULL;
HFONT PhMonospaceFont = NULL;

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
    LONG dpiValue;

    dpiValue = PhGetDpiValue(ListViewHandle, NULL);

    memset(&column, 0, sizeof(LVCOLUMN));
    column.mask = LVCF_FMT | LVCF_WIDTH | LVCF_TEXT | LVCF_SUBITEM | LVCF_ORDER;
    column.fmt = Format;
    column.cx = Width < 0 ? -Width : PhGetDpi(Width, dpiValue);
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

_Success_(return)
BOOLEAN PhGetListViewItemImageIndex(
    _In_ HWND ListViewHandle,
    _In_ INT Index,
    _Out_ PINT ImageIndex
    )
{
    LVITEM item;

    item.mask = LVIF_IMAGE;
    item.iItem = Index;
    item.iSubItem = 0;

    if (!ListView_GetItem(ListViewHandle, &item))
        return FALSE;

    *ImageIndex = item.iImage;

    return TRUE;
}

_Success_(return)
BOOLEAN PhGetListViewItemParam(
    _In_ HWND ListViewHandle,
    _In_ INT Index,
    _Out_ PVOID *Param
    )
{
    LVITEM item;

    item.mask = LVIF_PARAM;
    item.iItem = Index;
    item.iSubItem = 0;

    if (!ListView_GetItem(ListViewHandle, &item))
        return FALSE;

    *Param = (PVOID)item.lParam;

    return TRUE;
}

BOOLEAN PhSetListViewItemParam(
    _In_ HWND ListViewHandle,
    _In_ INT Index,
    _In_ PVOID Param
    )
{
    LVITEM item;

    item.mask = LVIF_PARAM;
    item.iItem = Index;
    item.lParam = (LPARAM)Param;

    return !!ListView_SetItem(ListViewHandle, &item);
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

        if (Index == CB_ERR)
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

        InvalidateRect(hwnd, NULL, TRUE);

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
        return NULL;

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
    INT i;
    INT count;

    count = ListView_GetItemCount(hWnd);

    if (count <= 0)
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
    INT index;
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
        PhImageListReplace(ImageList, Index, bitmap, NULL);
        DeleteBitmap(bitmap);
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
        entry1->Height != entry2->Height ||
        entry1->DpiValue != entry2->DpiValue)
    {
        return FALSE;
    }

    if (IS_INTRESOURCE(entry1->Name))
    {
        if (IS_INTRESOURCE(entry2->Name))
            return PtrToUlong(entry1->Name) == PtrToUlong(entry2->Name);
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

    return nameHash ^ (PtrToUlong(entry->InstanceHandle) >> 5) ^ (entry->Width << 3) ^ entry->Height ^ entry->DpiValue;
}

HICON PhLoadIcon(
    _In_opt_ PVOID ImageBaseAddress,
    _In_ PWSTR Name,
    _In_ ULONG Flags,
    _In_opt_ ULONG Width,
    _In_opt_ ULONG Height,
    _In_opt_ LONG DpiValue
    )
{
    PHP_ICON_ENTRY entry;
    PPHP_ICON_ENTRY actualEntry;
    HICON icon = NULL;
    INT width;
    INT height;

    if (PhBeginInitOnce(&SharedIconCacheInitOnce))
    {
        SharedIconCacheHashtable = PhCreateHashtable(sizeof(PHP_ICON_ENTRY),
            SharedIconCacheHashtableEqualFunction, SharedIconCacheHashtableHashFunction, 10);
        PhEndInitOnce(&SharedIconCacheInitOnce);
    }

    if (Flags & PH_LOAD_ICON_SHARED)
    {
        PhAcquireQueuedLockExclusive(&SharedIconCacheLock);

        entry.InstanceHandle = ImageBaseAddress;
        entry.Name = Name;
        entry.Width = PhpGetIconEntrySize(Width, Flags);
        entry.Height = PhpGetIconEntrySize(Height, Flags);
        entry.DpiValue = DpiValue;
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
        if (Flags & PH_LOAD_ICON_SIZE_SMALL)
        {
            width = PhGetSystemMetrics(SM_CXSMICON, DpiValue);
            height = PhGetSystemMetrics(SM_CXSMICON, DpiValue);
        }
        else
        {
            width = PhGetSystemMetrics(SM_CXICON, DpiValue);
            height = PhGetSystemMetrics(SM_CYICON, DpiValue);
        }

        if (LoadIconWithScaleDown)
            LoadIconWithScaleDown(ImageBaseAddress, Name, width, height, &icon);
        //if (LoadIconMetric)
        //    LoadIconMetric(ImageBaseAddress, Name, (Flags & PH_LOAD_ICON_SIZE_SMALL) ? LIM_SMALL : LIM_LARGE, &icon);
    }
    else
    {
        if (LoadIconWithScaleDown)
            LoadIconWithScaleDown(ImageBaseAddress, Name, Width, Height, &icon);
    }

    if (!icon && !(Flags & PH_LOAD_ICON_STRICT))
    {
        if (Flags & PH_LOAD_ICON_SIZE_SMALL)
        {
            width = PhGetSystemMetrics(SM_CXSMICON, DpiValue);
            height = PhGetSystemMetrics(SM_CXSMICON, DpiValue);
        }
        else
        {
            width = PhGetSystemMetrics(SM_CXICON, DpiValue);
            height = PhGetSystemMetrics(SM_CYICON, DpiValue);
        }

        icon = LoadImage(ImageBaseAddress, Name, IMAGE_ICON, width, height, 0);
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
        LONG dpiValue = PhGetSystemDpi();

        if (WindowsVersion < WINDOWS_10)
        {
            PPH_STRING systemDirectory;
            PPH_STRING dllFileName;

            // imageres,11 (Windows 10 and above), user32,0 (Vista and above) or shell32,2 (XP) contains
            // the default application icon.

            if (systemDirectory = PhGetSystemDirectory())
            {
                dllFileName = PhConcatStringRefZ(&systemDirectory->sr, L"\\user32.dll");

                PhExtractIcon(
                    dllFileName->Buffer,
                    &largeIcon,
                    &smallIcon
                    );

                PhDereferenceObject(dllFileName);
                PhDereferenceObject(systemDirectory);
            }
        }

        // Fallback icons
        if (!smallIcon)
            smallIcon = PhLoadIcon(NULL, IDI_APPLICATION, PH_LOAD_ICON_SIZE_SMALL, 0, 0, dpiValue);
        if (!largeIcon)
            largeIcon = PhLoadIcon(NULL, IDI_APPLICATION, PH_LOAD_ICON_SIZE_LARGE, 0, 0, dpiValue);

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
    memset(&fileInfo, 0, sizeof(SHFILEINFO));

    if (FileName && SHGetFileInfoW_Import() && SHGetFileInfoW_Import()(
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
        memset(&fileInfo, 0, sizeof(SHFILEINFO));

        if (SHGetFileInfoW_Import() && SHGetFileInfoW_Import()(
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

    data = GlobalAlloc(GMEM_MOVEABLE, String->Length + sizeof(UNICODE_NULL));
    memory = GlobalLock(data);

    memcpy(memory, String->Buffer, String->Length);
    *(PWCHAR)PTR_ADD_OFFSET(memory, String->Length) = UNICODE_NULL;

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

    if (!PhLoadResourceCopy(Instance, Template, RT_DIALOG, NULL, &dialogTemplate))
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

HWND PhCreateDialog(
    _In_ PVOID Instance,
    _In_ PWSTR Template,
    _In_opt_ HWND ParentWindow,
    _In_ DLGPROC DialogProc,
    _In_opt_ PVOID Parameter
    )
{
    PDLGTEMPLATEEX dialogTemplate;
    HWND dialogHandle;

    if (!PhLoadResource(Instance, Template, RT_DIALOG, NULL, &dialogTemplate))
        return NULL;

    dialogHandle = CreateDialogIndirectParam(
        Instance,
        (LPDLGTEMPLATE)dialogTemplate,
        ParentWindow,
        DialogProc,
        (LPARAM)Parameter
        );

    return dialogHandle;
}

HWND PhCreateWindow(
    _In_ ULONG ExStyle,
    _In_opt_ PCWSTR ClassName,
    _In_opt_ PCWSTR WindowName,
    _In_ ULONG Style,
    _In_ INT X,
    _In_ INT Y,
    _In_ INT Width,
    _In_ INT Height,
    _In_opt_ HWND ParentWindow,
    _In_opt_ HMENU MenuHandle,
    _In_opt_ PVOID InstanceHandle,
    _In_opt_ PVOID Parameter
    )
{
    return CreateWindowEx(
        ExStyle,
        ClassName,
        WindowName,
        Style,
        X,
        Y,
        Width,
        Height,
        ParentWindow,
        MenuHandle,
        InstanceHandle,
        Parameter
        );
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

        if (message.message == WM_KEYDOWN /*|| message.message == WM_KEYUP*/) // forward key messages (dmex)
        {
            SendMessage(hwnd, message.message, message.wParam, message.lParam);
        }

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
    RECT rect;
    LONG dpiValue;

    dpiValue = PhGetWindowDpi(RootWindowHandle);

    GetClientRect(RootWindowHandle, &rect);

    PhGetSizeDpiValue(&rect, dpiValue, FALSE);

    Manager->List = PhCreateList(4);

    Manager->dpiValue = dpiValue;
    Manager->LayoutNumber = 0;

    Manager->RootItem.Handle = RootWindowHandle;
    Manager->RootItem.Rect = rect;
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
    MapWindowPoints(HWND_DESKTOP, item->LayoutParentItem->Handle, (PPOINT)&item->Rect, 2);

    if (item->Anchor & PH_LAYOUT_TAB_CONTROL)
    {
        // We want to convert the tab control rectangle to the tab page display rectangle.
        TabCtrl_AdjustRect(Handle, FALSE, &item->Rect);
    }

    PhGetSizeDpiValue(&item->Rect, Manager->dpiValue, FALSE);

    item->Margin = Margin;
    PhGetSizeDpiValue(&item->Margin, Manager->dpiValue, FALSE);

    PhAddItemList(Manager->List, item);

    return item;
}

VOID PhpLayoutItemLayout(
    _Inout_ PPH_LAYOUT_MANAGER Manager,
    _Inout_ PPH_LAYOUT_ITEM Item
    )
{
    RECT margin;
    RECT rect;
    ULONG diff;
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
    MapWindowPoints(HWND_DESKTOP, Item->LayoutParentItem->Handle, (PPOINT)&Item->Rect, 2);

    if (Item->Anchor & PH_LAYOUT_TAB_CONTROL)
    {
        // We want to convert the tab control rectangle to the tab page display rectangle.
        TabCtrl_AdjustRect(Item->Handle, FALSE, &Item->Rect);
    }

    PhGetSizeDpiValue(&Item->Rect, Manager->dpiValue, FALSE);

    if (!(Item->Anchor & PH_LAYOUT_DUMMY_MASK))
    {
        margin = Item->Margin;
        rect = Item->Rect;

        // Convert right/bottom into margins to make the calculations
        // easier.
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
                rect.left = (hasDummyParent ? Item->ParentItem->Rect.left : 0) + margin.left;
                rect.right = margin.right;
            }
            else
            {
                diff = margin.right - rect.right;

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
                rect.top = (hasDummyParent ? Item->ParentItem->Rect.top : 0) + margin.top;
                rect.bottom = margin.bottom;
            }
            else
            {
                diff = margin.bottom - rect.bottom;

                rect.top -= diff;
                rect.bottom += diff;
            }
        }

        // Convert the right/bottom back into co-ordinates.
        PhConvertRect(&rect, &Item->LayoutParentItem->Rect);
        Item->Rect = rect;
        PhGetSizeDpiValue(&rect, Manager->dpiValue, TRUE);

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
    PPH_LAYOUT_ITEM item;
    LONG dpiValue;
    ULONG i;

    Manager->LayoutNumber++;

    dpiValue = PhGetWindowDpi(Manager->RootItem.Handle);
    Manager->dpiValue = dpiValue;

    GetClientRect(Manager->RootItem.Handle, &Manager->RootItem.Rect);
    PhGetSizeDpiValue(&Manager->RootItem.Rect, dpiValue, FALSE);

    for (i = 0; i < Manager->List->Count; i++)
    {
        item = (PPH_LAYOUT_ITEM)Manager->List->Items[i];

        PhpLayoutItemLayout(Manager, item);
    }

    for (i = 0; i < Manager->List->Count; i++)
    {
        item = (PPH_LAYOUT_ITEM)Manager->List->Items[i];

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

VOID PhEnumWindows(
    _In_ PH_ENUM_CALLBACK Callback,
    _In_opt_ PVOID Context
    )
{
    EnumWindows((WNDENUMPROC)Callback, (LPARAM)Context);
}

VOID PhEnumChildWindows(
    _In_opt_ HWND WindowHandle,
    _In_ ULONG Limit,
    _In_ PH_CHILD_ENUM_CALLBACK Callback,
    _In_opt_ PVOID Context
    )
{
    EnumChildWindows(WindowHandle, (WNDENUMPROC)Callback, (LPARAM)Context);

    //HWND childWindow = NULL;
    //ULONG i = 0;
    //
    //while (i < Limit && (childWindow = FindWindowEx(WindowHandle, childWindow, NULL, NULL)))
    //{
    //    if (!Callback(childWindow, Context))
    //        return;
    //
    //    i++;
    //}
}

typedef struct _GET_PROCESS_MAIN_WINDOW_CONTEXT
{
    HWND Window;
    HWND ImmersiveWindow;
    HANDLE ProcessId;
    BOOLEAN IsImmersive;
    BOOLEAN SkipInvisible;
} GET_PROCESS_MAIN_WINDOW_CONTEXT, *PGET_PROCESS_MAIN_WINDOW_CONTEXT;

BOOL CALLBACK PhpGetProcessMainWindowEnumWindowsProc(
    _In_ HWND WindowHandle,
    _In_opt_ PVOID Context
    )
{
    PGET_PROCESS_MAIN_WINDOW_CONTEXT context = (PGET_PROCESS_MAIN_WINDOW_CONTEXT)Context;
    ULONG processId;
    WINDOWINFO windowInfo;

    if (!context)
        return TRUE;
    if (context->SkipInvisible && !IsWindowVisible(WindowHandle))
        return TRUE;

    GetWindowThreadProcessId(WindowHandle, &processId);

    //if (UlongToHandle(processId) == context->ProcessId && (context->SkipInvisible ?
    //    !((parentWindow = GetParent(WindowHandle)) && IsWindowVisible(parentWindow)) && // skip windows with a visible parent
    //    PhGetWindowTextEx(WindowHandle, PH_GET_WINDOW_TEXT_INTERNAL | PH_GET_WINDOW_TEXT_LENGTH_ONLY, NULL) != 0 : TRUE)) // skip windows with no title

    if (UlongToHandle(processId) == context->ProcessId)
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

    if (processHandle && WindowsVersion >= WINDOWS_8)
        context.IsImmersive = PhIsImmersiveProcess(processHandle);

    PhEnumWindows(PhpGetProcessMainWindowEnumWindowsProc, &context);
    //PhEnumChildWindows(NULL, 0x800, PhpGetProcessMainWindowEnumWindowsProc, &context);

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

VOID PhSetGroupBoxText(
    _In_ HWND WindowHandle,
    _In_ PCWSTR WindowText
    )
{
    // Suspend the groupbox when setting the text otherwise
    // the groupbox doesn't paint the text with dark theme colors. (dmex)

    SendMessage(WindowHandle, WM_SETREDRAW, FALSE, 0);
    PhSetWindowText(WindowHandle, WindowText);
    SendMessage(WindowHandle, WM_SETREDRAW, TRUE, 0);

    InvalidateRect(WindowHandle, NULL, FALSE);
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
        ((PPH_PLUGIN_WINDOW_CALLBACK_REGISTRATION)Entry1)->WindowHandle ==
        ((PPH_PLUGIN_WINDOW_CALLBACK_REGISTRATION)Entry2)->WindowHandle;
}

static ULONG NTAPI PhpWindowCallbackHashtableHashFunction(
    _In_ PVOID Entry
    )
{
    return PhHashIntPtr((ULONG_PTR)((PPH_PLUGIN_WINDOW_CALLBACK_REGISTRATION)Entry)->WindowHandle);
}

VOID PhRegisterWindowCallback(
    _In_ HWND WindowHandle,
    _In_ PH_PLUGIN_WINDOW_EVENT_TYPE Type,
    _In_opt_ PVOID Context
    )
{
    PH_PLUGIN_WINDOW_CALLBACK_REGISTRATION entry;

    entry.WindowHandle = WindowHandle;
    entry.Type = Type;

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
    PPH_PLUGIN_WINDOW_CALLBACK_REGISTRATION entry;

    lookupEntry.WindowHandle = WindowHandle;

    PhAcquireQueuedLockExclusive(&WindowCallbackListLock);
    entry = PhFindEntryHashtable(WindowCallbackHashTable, &lookupEntry);
    assert(entry);
    PhRemoveEntryHashtable(WindowCallbackHashTable, entry);
    PhReleaseQueuedLockExclusive(&WindowCallbackListLock);
}

VOID PhWindowNotifyTopMostEvent(
    _In_ BOOLEAN TopMost
    )
{
    PPH_PLUGIN_WINDOW_CALLBACK_REGISTRATION entry;
    ULONG i = 0;

    PhAcquireQueuedLockExclusive(&WindowCallbackListLock);

    while (PhEnumHashtable(WindowCallbackHashTable, &entry, &i))
    {
        if (entry->Type & PH_PLUGIN_WINDOW_EVENT_TYPE_TOPMOST)
        {
            PhSetWindowAlwaysOnTop(entry->WindowHandle, TopMost);
        }
    }

    PhReleaseQueuedLockExclusive(&WindowCallbackListLock);
}

HICON PhGetInternalWindowIcon(
    _In_ HWND WindowHandle,
    _In_ UINT Type
    )
{
    static PH_INITONCE initOnce = PH_INITONCE_INIT;
    static HICON (WINAPI *InternalGetWindowIcon_I)(
        _In_ HWND WindowHandle,
        _In_ ULONG Type
        ) = NULL;

    if (PhBeginInitOnce(&initOnce))
    {
        PVOID shell32Handle;

        if (shell32Handle = PhLoadLibrary(L"shell32.dll"))
        {
            InternalGetWindowIcon_I = PhGetDllBaseProcedureAddress(shell32Handle, "InternalGetWindowIcon", 0);
        }

        PhEndInitOnce(&initOnce);
    }

    if (!InternalGetWindowIcon_I)
        return NULL;

    return InternalGetWindowIcon_I(WindowHandle, Type);
}

HANDLE PhGetGlobalTimerQueue(
    VOID
    )
{
    static HANDLE PhTimerQueueHandle = NULL;
    static PH_INITONCE PhTimerQueueHandleInitOnce = PH_INITONCE_INIT;

    if (PhBeginInitOnce(&PhTimerQueueHandleInitOnce))
    {
        RtlCreateTimerQueue(&PhTimerQueueHandle);

        PhEndInitOnce(&PhTimerQueueHandleInitOnce);
    }

    return PhTimerQueueHandle;
}

BOOLEAN PhIsImmersiveProcess(
    _In_ HANDLE ProcessHandle
    )
{
    static PH_INITONCE initOnce = PH_INITONCE_INIT;
    static BOOL (WINAPI* IsImmersiveProcess_I)(
        _In_ HANDLE ProcessHandle
        ) = NULL;

    if (PhBeginInitOnce(&initOnce))
    {
        if (WindowsVersion >= WINDOWS_8)
            IsImmersiveProcess_I = PhGetDllProcedureAddress(L"user32.dll", "IsImmersiveProcess", 0);
        PhEndInitOnce(&initOnce);
    }

    if (!IsImmersiveProcess_I)
        return FALSE;

    return !!IsImmersiveProcess_I(ProcessHandle);
}

_Success_(return)
BOOLEAN PhGetProcessDpiAwareness(
    _In_ HANDLE ProcessHandle,
    _Out_ PPH_PROCESS_DPI_AWARENESS ProcessDpiAwareness
    )
{
    static PH_INITONCE initOnce = PH_INITONCE_INIT;
    static DPI_AWARENESS_CONTEXT (WINAPI* GetDpiAwarenessContextForProcess_I)(
        _In_ HANDLE hprocess) = NULL;
    static BOOL (WINAPI* AreDpiAwarenessContextsEqual_I)(
        _In_ DPI_AWARENESS_CONTEXT dpiContextA,
        _In_ DPI_AWARENESS_CONTEXT dpiContextB) = NULL;
    static BOOL (WINAPI* GetProcessDpiAwarenessInternal_I)(
        _In_ HANDLE hprocess,
        _Out_ ULONG* value) = NULL;

    if (PhBeginInitOnce(&initOnce))
    {
        PVOID baseAddress;

        if (baseAddress = PhGetLoaderEntryDllBase(L"user32.dll"))
        {
            if (WindowsVersion >= WINDOWS_10_RS1)
            {
                GetDpiAwarenessContextForProcess_I = PhGetDllBaseProcedureAddress(baseAddress, "GetDpiAwarenessContextForProcess", 0);
                AreDpiAwarenessContextsEqual_I = PhGetDllBaseProcedureAddress(baseAddress, "AreDpiAwarenessContextsEqual", 0);
            }

            if (!(GetDpiAwarenessContextForProcess_I && AreDpiAwarenessContextsEqual_I))
            {
                GetProcessDpiAwarenessInternal_I = PhGetDllBaseProcedureAddress(baseAddress, "GetProcessDpiAwarenessInternal", 0);
            }
        }

        PhEndInitOnce(&initOnce);
    }

    if (GetDpiAwarenessContextForProcess_I && AreDpiAwarenessContextsEqual_I)
    {
        DPI_AWARENESS_CONTEXT context = GetDpiAwarenessContextForProcess_I(ProcessHandle);

        if (AreDpiAwarenessContextsEqual_I(context, DPI_AWARENESS_CONTEXT_UNAWARE))
        {
            *ProcessDpiAwareness = PH_PROCESS_DPI_AWARENESS_UNAWARE;
            return TRUE;
        }

        if (AreDpiAwarenessContextsEqual_I(context, DPI_AWARENESS_CONTEXT_SYSTEM_AWARE))
        {
            *ProcessDpiAwareness = PH_PROCESS_DPI_AWARENESS_SYSTEM_DPI_AWARE;
            return TRUE;
        }

        if (AreDpiAwarenessContextsEqual_I(context, DPI_AWARENESS_CONTEXT_PER_MONITOR_AWARE))
        {
            *ProcessDpiAwareness = PH_PROCESS_DPI_AWARENESS_PER_MONITOR_DPI_AWARE;
            return TRUE;
        }

        if (AreDpiAwarenessContextsEqual_I(context, DPI_AWARENESS_CONTEXT_PER_MONITOR_AWARE_V2))
        {
            *ProcessDpiAwareness = PH_PROCESS_DPI_AWARENESS_PER_MONITOR_AWARE_V2;
            return TRUE;
        }

        if (AreDpiAwarenessContextsEqual_I(context, DPI_AWARENESS_CONTEXT_UNAWARE_GDISCALED))
        {
            *ProcessDpiAwareness = PH_PROCESS_DPI_AWARENESS_UNAWARE_GDISCALED;
            return TRUE;
        }
    }

    if (GetProcessDpiAwarenessInternal_I)
    {
        ULONG dpiAwareness = 0;

        if (GetProcessDpiAwarenessInternal_I(ProcessHandle, &dpiAwareness))
        {
            switch (dpiAwareness)
            {
            case PROCESS_DPI_UNAWARE:
                *ProcessDpiAwareness = PH_PROCESS_DPI_AWARENESS_UNAWARE;
                return TRUE;
            case PROCESS_SYSTEM_DPI_AWARE:
                *ProcessDpiAwareness = PH_PROCESS_DPI_AWARENESS_SYSTEM_DPI_AWARE;
                return TRUE;
            case PROCESS_PER_MONITOR_DPI_AWARE:
                *ProcessDpiAwareness = PH_PROCESS_DPI_AWARENESS_PER_MONITOR_DPI_AWARE;
                return TRUE;
            }
        }
    }

    return FALSE;
}

_Success_(return)
BOOLEAN PhGetPhysicallyInstalledSystemMemory(
    _Out_ PULONGLONG TotalMemory,
    _Out_ PULONGLONG ReservedMemory
    )
{
    static PH_INITONCE initOnce = PH_INITONCE_INIT;
    static BOOL (WINAPI *GetPhysicallyInstalledSystemMemory_I)(_Out_ PULONGLONG TotalMemoryInKilobytes) = NULL;
    ULONGLONG physicallyInstalledSystemMemory = 0;

    if (PhBeginInitOnce(&initOnce))
    {
        GetPhysicallyInstalledSystemMemory_I = PhGetDllProcedureAddress(L"kernel32.dll", "GetPhysicallyInstalledSystemMemory", 0);
        PhEndInitOnce(&initOnce);
    }

    if (!GetPhysicallyInstalledSystemMemory_I)
        return FALSE;

    if (GetPhysicallyInstalledSystemMemory_I(&physicallyInstalledSystemMemory))
    {
        *TotalMemory = physicallyInstalledSystemMemory * 1024ULL;
        *ReservedMemory = physicallyInstalledSystemMemory * 1024ULL - UInt32x32To64(PhSystemBasicInformation.NumberOfPhysicalPages, PAGE_SIZE);
        return TRUE;
    }

    return FALSE;
}

_Success_(return)
BOOLEAN PhGetSendMessageReceiver(
    _In_ HANDLE ThreadId,
    _Out_ HWND *WindowHandle
    )
{
    static PH_INITONCE initOnce = PH_INITONCE_INIT;
    static HWND (WINAPI *GetSendMessageReceiver_I)(
        _In_ HANDLE ThreadId
        );
    HWND windowHandle;

    // GetSendMessageReceiver is an undocumented function exported by
    // user32.dll. It retrieves the handle of the window which a thread
    // is sending a message to. (wj32)

    if (PhBeginInitOnce(&initOnce))
    {
        GetSendMessageReceiver_I = PhGetDllProcedureAddress(L"user32.dll", "GetSendMessageReceiver", 0);
        PhEndInitOnce(&initOnce);
    }

    if (!GetSendMessageReceiver_I)
        return FALSE;

    if (windowHandle = GetSendMessageReceiver_I(ThreadId)) // && GetLastError() == ERROR_SUCCESS
    {
        *WindowHandle = windowHandle;
        return TRUE;
    }

    return FALSE;
}

// rev from ExtractIconExW
_Success_(return)
BOOLEAN PhExtractIcon(
    _In_ PWSTR FileName,
    _Out_opt_ HICON *IconLarge,
    _Out_opt_ HICON *IconSmall
    )
{
    static PH_INITONCE initOnce = PH_INITONCE_INIT;
    static INT (WINAPI *PrivateExtractIconExW)(
        _In_ PCWSTR FileName,
        _In_ INT IconIndex,
        _Out_opt_ HICON* IconLarge,
        _Out_opt_ HICON* IconSmall,
        _In_ INT IconCount
        ) = NULL;
    HICON iconLarge = NULL;
    HICON iconSmall = NULL;

    if (PhBeginInitOnce(&initOnce))
    {
        PrivateExtractIconExW = PhGetDllProcedureAddress(L"user32.dll", "PrivateExtractIconExW", 0);
        PhEndInitOnce(&initOnce);
    }

    if (!PrivateExtractIconExW)
        return FALSE;

    if (PrivateExtractIconExW(
        FileName,
        0,
        IconLarge ? &iconLarge : NULL,
        IconSmall ? &iconSmall : NULL,
        1
        ) > 0)
    {
        if (IconLarge)
            *IconLarge = iconLarge;
        if (IconSmall)
            *IconSmall = iconSmall;

        return TRUE;
    }

    if (iconLarge)
        DestroyIcon(iconLarge);
    if (iconSmall)
        DestroyIcon(iconSmall);

    return FALSE;
}

_Success_(return)
BOOLEAN PhLoadIconFromResourceDirectory(
    _In_ PPH_MAPPED_IMAGE MappedImage,
    _In_ PIMAGE_RESOURCE_DIRECTORY ResourceDirectory,
    _In_ INT32 ResourceIndex,
    _In_ PCWSTR ResourceType,
    _Out_opt_ ULONG* ResourceLength,
    _Out_opt_ PVOID* ResourceBuffer
    )
{
    ULONG resourceIndex;
    ULONG resourceCount;
    PVOID resourceBuffer;
    PIMAGE_RESOURCE_DIRECTORY nameDirectory;
    PIMAGE_RESOURCE_DIRECTORY languageDirectory;
    PIMAGE_RESOURCE_DIRECTORY_ENTRY resourceType;
    PIMAGE_RESOURCE_DIRECTORY_ENTRY resourceName;
    PIMAGE_RESOURCE_DIRECTORY_ENTRY resourceLanguage;
    PIMAGE_RESOURCE_DATA_ENTRY resourceData;

    // Find the type
    resourceCount = ResourceDirectory->NumberOfIdEntries + ResourceDirectory->NumberOfNamedEntries;
    resourceType = PTR_ADD_OFFSET(ResourceDirectory, sizeof(IMAGE_RESOURCE_DIRECTORY));

    for (resourceIndex = 0; resourceIndex < resourceCount; resourceIndex++)
    {
        if (resourceType[resourceIndex].NameIsString)
            continue;
        if (resourceType[resourceIndex].Name == PtrToUlong(ResourceType))
            break;
    }

    if (resourceIndex == resourceCount)
        return FALSE;
    if (!resourceType[resourceIndex].DataIsDirectory)
        return FALSE;

    // Find the name
    nameDirectory = PTR_ADD_OFFSET(ResourceDirectory, resourceType[resourceIndex].OffsetToDirectory);
    resourceCount = nameDirectory->NumberOfIdEntries + nameDirectory->NumberOfNamedEntries;
    resourceName = PTR_ADD_OFFSET(nameDirectory, sizeof(IMAGE_RESOURCE_DIRECTORY));

    if (ResourceIndex < 0) // RT_ICON and DEVPKEY_DeviceClass_IconPath
    {
        for (resourceIndex = 0; resourceIndex < resourceCount; resourceIndex++)
        {
            if (resourceName[resourceIndex].NameIsString)
                continue;
            if (resourceName[resourceIndex].Name == (ULONG)-ResourceIndex)
                break;
        }
    }
    else // RT_GROUP_ICON
    {
        resourceIndex = ResourceIndex;
    }

    if (resourceIndex >= resourceCount)
        return FALSE;
    if (!resourceName[resourceIndex].DataIsDirectory)
        return FALSE;

    // Find the language
    languageDirectory = PTR_ADD_OFFSET(ResourceDirectory, resourceName[resourceIndex].OffsetToDirectory);
    //resourceCount = languageDirectory->NumberOfIdEntries + languageDirectory->NumberOfNamedEntries;
    resourceLanguage = PTR_ADD_OFFSET(languageDirectory, sizeof(IMAGE_RESOURCE_DIRECTORY));
    resourceIndex = 0; // use the first entry

    if (resourceLanguage[resourceIndex].DataIsDirectory)
        return FALSE;

    resourceData = PTR_ADD_OFFSET(ResourceDirectory, resourceLanguage[resourceIndex].OffsetToData);

    if (!resourceData)
        return FALSE;

    resourceBuffer = PhMappedImageRvaToVa(MappedImage, resourceData->OffsetToData, NULL);

    if (!resourceBuffer)
        return FALSE;

    if (ResourceLength)
        *ResourceLength = resourceData->Size;
    if (ResourceBuffer)
        *ResourceBuffer = resourceBuffer;

    // if (LDR_IS_IMAGEMAPPING(ImageBaseAddress))
    // PhLoaderEntryImageRvaToVa(ImageBaseAddress, resourceData->OffsetToData, resourceBuffer);
    // PhLoadResource(ImageBaseAddress, MAKEINTRESOURCE(ResourceIndex), ResourceType, &resourceLength, &resourceBuffer);

    return TRUE;
}

#ifndef MAKEFOURCC
#define MAKEFOURCC(ch0, ch1, ch2, ch3) \
 ((ULONG)(BYTE)(ch0) | ((ULONG)(BYTE)(ch1) << 8) | \
 ((ULONG)(BYTE)(ch2) << 16) | ((ULONG)(BYTE)(ch3) << 24 ))
#endif

// https://docs.microsoft.com/en-us/windows/win32/menurc/newheader
// One or more RESDIR structures immediately follow the NEWHEADER structure.
typedef struct _NEWHEADER
{
    USHORT Reserved;
    USHORT ResourceType;
    USHORT ResourceCount;
} NEWHEADER, *PNEWHEADER;

HICON PhCreateIconFromResourceDirectory(
    _In_ PPH_MAPPED_IMAGE MappedImage,
    _In_ PVOID ResourceDirectory,
    _In_ PVOID IconDirectory,
    _In_ INT32 Width,
    _In_ INT32 Height,
    _In_ UINT32 Flags
    )
{
    INT32 iconResourceId;
    ULONG iconResourceLength;
    PVOID iconResourceBuffer;

    if (!(iconResourceId = LookupIconIdFromDirectoryEx(
        IconDirectory,
        TRUE,
        Width,
        Height,
        Flags
        )))
    {
        return NULL;
    }

    if (!PhLoadIconFromResourceDirectory(
        MappedImage,
        ResourceDirectory,
        -iconResourceId,
        RT_ICON,
        &iconResourceLength,
        &iconResourceBuffer
        ))
    {
        return NULL;
    }

    if (
        ((PBITMAPINFOHEADER)iconResourceBuffer)->biSize != sizeof(BITMAPINFOHEADER) &&
        ((PBITMAPCOREHEADER)iconResourceBuffer)->bcSize != sizeof(BITMAPCOREHEADER) &&
        ((PBITMAPCOREHEADER)iconResourceBuffer)->bcSize != MAKEFOURCC(137, 'P', 'N', 'G') &&
        ((PBITMAPCOREHEADER)iconResourceBuffer)->bcSize != MAKEFOURCC('J', 'P', 'E', 'G')
        )
    {
        // CreateIconFromResourceEx seems to know what formats are supported so these
        // size/format checks are probably redundant and not required? (dmex)
        return NULL;
    }

    return CreateIconFromResourceEx(
        iconResourceBuffer,
        iconResourceLength,
        TRUE,
        0x30000,
        Width,
        Height,
        Flags
        );
}

PPH_STRING PhpGetImageMunResourcePath(
    _In_ PPH_STRING FileName,
    _In_ BOOLEAN NativeFileName
    )
{
    static PH_STRINGREF systemResourcePathSr = PH_STRINGREF_INIT(L"\\SystemResources\\");
    static PH_STRINGREF systemResourceExtensionSr = PH_STRINGREF_INIT(L".mun");
    PPH_STRING filePath = NULL;
    PPH_STRING directory;
    PPH_STRING fileName;

    if (WindowsVersion < WINDOWS_10_19H1)
    {
        PhReferenceObject(FileName);
        return FileName;
    }

    if (PhDetermineDosPathNameType(PhGetString(FileName)) == RtlPathTypeUncAbsolute)
    {
        PhReferenceObject(FileName);
        return FileName;
    }

    // 19H1 and above relocated binary resources into the \SystemResources\ directory.
    // This is implemented as a hook inside EnumResourceNamesExW:
    // PrivateExtractIconExW -> EnumResourceNamesExW -> GetMunResourceModuleForEnumIfExist.
    //
    // GetMunResourceModuleForEnumIfExist trims the path and inserts '\SystemResources\' and '.mun'
    // to locate the binary with the icon resources. For example:
    // From: C:\Windows\system32\notepad.exe
    // To: C:\Windows\SystemResources\notepad.exe.mun
    //
    // It doesn't currently hard-code the \SystemResources\ path and ends up accessing other directories:
    // From: C:\Windows\explorer.exe
    // To: C:\SystemResources\explorer.exe.mun
    //
    // The below code has the same logic and semantics. (dmex)

    directory = PhGetBaseDirectory(FileName);
    fileName = PhGetBaseName(FileName);

    if (directory)
    {
        PhMoveReference(&directory, PhGetBaseDirectory(directory));
    }

    if (directory && fileName)
    {
        PhMoveReference(&fileName, PhConcatStringRef3(&directory->sr, &systemResourcePathSr, &fileName->sr));
        PhMoveReference(&fileName, PhConcatStringRef2(&fileName->sr, &systemResourceExtensionSr));

        if (NativeFileName)
        {
            if (PhDoesFileExist(&fileName->sr))
                PhMoveReference(&filePath, fileName);
            else
                PhDereferenceObject(fileName);
        }
        else
        {
            if (PhDoesFileExistWin32(PhGetString(fileName)))
                PhMoveReference(&filePath, fileName);
            else
                PhDereferenceObject(fileName);
        }
    }

    if (PhIsNullOrEmptyString(filePath))
    {
        PhSetReference(&filePath, FileName);
    }

    PhClearReference(&directory);

    return filePath;
}

// rev from PrivateExtractIconExW with changes
// for using SEC_COMMIT instead of SEC_IMAGE. (dmex)
_Success_(return)
BOOLEAN PhExtractIconEx(
    _In_ PPH_STRING FileName,
    _In_ BOOLEAN NativeFileName,
    _In_ INT32 IconIndex,
    _Out_opt_ HICON *IconLarge,
    _Out_opt_ HICON *IconSmall,
    _In_ LONG dpiValue
    )
{
    NTSTATUS status;
    HICON iconLarge = NULL;
    HICON iconSmall = NULL;
    PPH_STRING fileName;
    PH_MAPPED_IMAGE mappedImage;
    PIMAGE_DATA_DIRECTORY dataDirectory;
    PIMAGE_RESOURCE_DIRECTORY resourceDirectory;
    ULONG iconDirectoryResourceLength;
    PNEWHEADER iconDirectoryResource;

    if (!(fileName = PhpGetImageMunResourcePath(
        FileName,
        NativeFileName
        )))
    {
        return FALSE;
    }

    if (NativeFileName)
    {
        status = PhLoadMappedImageEx(
            &fileName->sr,
            NULL,
            &mappedImage
            );
    }
    else
    {
        status = PhLoadMappedImage(
            PhGetString(fileName),
            NULL,
            &mappedImage
            );
    }

    if (!NT_SUCCESS(status))
    {
        PhDereferenceObject(fileName);
        return FALSE;
    }

    status = PhGetMappedImageDataEntry(
        &mappedImage,
        IMAGE_DIRECTORY_ENTRY_RESOURCE,
        &dataDirectory
        );

    if (!NT_SUCCESS(status))
        goto CleanupExit;

    resourceDirectory = PhMappedImageRvaToVa(
        &mappedImage,
        dataDirectory->VirtualAddress,
        NULL
        );

    if (!resourceDirectory)
        goto CleanupExit;

    __try
    {
        if (!PhLoadIconFromResourceDirectory(
            &mappedImage,
            resourceDirectory,
            IconIndex,
            RT_GROUP_ICON,
            &iconDirectoryResourceLength,
            &iconDirectoryResource
            ))
        {
            goto CleanupExit;
        }

        if (iconDirectoryResource->ResourceType != RES_ICON)
            goto CleanupExit;

        if (IconLarge)
        {
            iconLarge = PhCreateIconFromResourceDirectory(
                &mappedImage,
                resourceDirectory,
                iconDirectoryResource,
                PhGetSystemMetrics(SM_CXICON, dpiValue),
                PhGetSystemMetrics(SM_CYICON, dpiValue),
                LR_DEFAULTCOLOR
                );
        }

        if (IconSmall)
        {
            iconSmall = PhCreateIconFromResourceDirectory(
                &mappedImage,
                resourceDirectory,
                iconDirectoryResource,
                PhGetSystemMetrics(SM_CXSMICON, dpiValue),
                PhGetSystemMetrics(SM_CYSMICON, dpiValue),
                LR_DEFAULTCOLOR
                );
        }
    }
    __except (EXCEPTION_EXECUTE_HANDLER)
    {
        NOTHING;
    }

CleanupExit:

    PhUnloadMappedImage(&mappedImage);
    PhDereferenceObject(fileName);

    if (IconLarge && IconSmall)
    {
        if (iconLarge && iconSmall)
        {
            *IconLarge = iconLarge;
            *IconSmall = iconSmall;
            return TRUE;
        }

        if (iconLarge)
            DestroyIcon(iconLarge);
        if (iconSmall)
            DestroyIcon(iconSmall);

        return FALSE;
    }

    if (IconLarge && iconLarge)
    {
        *IconLarge = iconLarge;
        return TRUE;
    }

    if (IconSmall && iconSmall)
    {
        *IconSmall = iconSmall;
        return TRUE;
    }

    if (iconLarge)
        DestroyIcon(iconLarge);
    if (iconSmall)
        DestroyIcon(iconSmall);

    return FALSE;
}

// Imagelist support

HIMAGELIST PhImageListCreate(
    _In_ UINT Width,
    _In_ UINT Height,
    _In_ UINT Flags,
    _In_ UINT InitialCount,
    _In_ UINT GrowCount
    )
{
    HRESULT status;
    IImageList2* imageList;

    status = ImageList_CoCreateInstance(
        &CLSID_ImageList,
        NULL,
        &IID_IImageList2,
        &imageList
        );

    if (FAILED(status))
        return NULL;

    status = IImageList2_Initialize(
        imageList,
        Width,
        Height,
        Flags,
        InitialCount,
        GrowCount
        );

    if (FAILED(status))
        return NULL;

    // Win32 HIMAGELIST is always a pointer to the IImageList interface. (dmex)
    return (HIMAGELIST)imageList;
}

BOOLEAN PhImageListDestroy(
    _In_ HIMAGELIST ImageListHandle
    )
{
    return SUCCEEDED(IImageList2_Release((IImageList2*)ImageListHandle));
}

BOOLEAN PhImageListSetImageCount(
    _In_ HIMAGELIST ImageListHandle,
    _In_ UINT Count
    )
{
    return SUCCEEDED(IImageList2_SetImageCount((IImageList2*)ImageListHandle, Count));
}

BOOLEAN PhImageListGetImageCount(
    _In_ HIMAGELIST ImageListHandle,
    _Out_ PUINT Count
    )
{
    return SUCCEEDED(IImageList2_GetImageCount((IImageList2*)ImageListHandle, Count));
}

BOOLEAN PhImageListSetBkColor(
    _In_ HIMAGELIST ImageListHandle,
    _In_ COLORREF BackgroundColor
    )
{
    COLORREF previousColor = 0;

    return SUCCEEDED(IImageList2_SetBkColor(
        (IImageList2*)ImageListHandle,
        BackgroundColor,
        &previousColor
        ));
}

UINT PhImageListAddIcon(
    _In_ HIMAGELIST ImageListHandle,
    _In_ HICON IconHandle
    )
{
    INT index = -1;

    IImageList2_ReplaceIcon(
        (IImageList2*)ImageListHandle,
        UINT_MAX,
        IconHandle,
        &index
        );

    return index;
}

UINT PhImageListAddBitmap(
    _In_ HIMAGELIST ImageListHandle,
    _In_ HBITMAP BitmapImage,
    _In_opt_ HBITMAP BitmapMask
    )
{
    INT index = -1;

    IImageList2_Add(
        (IImageList2*)ImageListHandle,
        BitmapImage,
        BitmapMask,
        &index
        );

    return index;
}

BOOLEAN PhImageListRemoveIcon(
    _In_ HIMAGELIST ImageListHandle,
    _In_ UINT Index
    )
{
    return SUCCEEDED(IImageList2_Remove(
        (IImageList2*)ImageListHandle,
        Index
        ));
}

HICON PhImageListGetIcon(
    _In_ HIMAGELIST ImageListHandle,
    _In_ UINT Index,
    _In_ UINT Flags
    )
{
    HICON iconhandle = NULL;

    IImageList2_GetIcon(
        (IImageList2*)ImageListHandle,
        Index,
        Flags,
        &iconhandle
        );

    return iconhandle;
}

BOOLEAN PhImageListGetIconSize(
    _In_ HIMAGELIST ImageListHandle,
    _Out_ PINT cx,
    _Out_ PINT cy
    )
{
    return SUCCEEDED(IImageList2_GetIconSize(
        (IImageList2*)ImageListHandle,
        cx,
        cy
        ));
}

BOOLEAN PhImageListReplace(
    _In_ HIMAGELIST ImageListHandle,
    _In_ UINT Index,
    _In_ HBITMAP BitmapImage,
    _In_opt_ HBITMAP BitmapMask
    )
{
    return SUCCEEDED(IImageList2_Replace(
        (IImageList2*)ImageListHandle,
        Index,
        BitmapImage,
        BitmapMask
        ));
}

BOOLEAN PhImageListDrawIcon(
    _In_ HIMAGELIST ImageListHandle,
    _In_ INT Index,
    _In_ HDC Hdc,
    _In_ INT x,
    _In_ INT y,
    _In_ UINT Style,
    _In_ BOOLEAN Disabled
    )
{
    return PhImageListDrawEx(
        ImageListHandle,
        Index,
        Hdc,
        x,
        y,
        0,
        0,
        CLR_DEFAULT,
        CLR_DEFAULT,
        Style,
        Disabled ? ILS_SATURATE : ILS_NORMAL
        );
}

BOOLEAN PhImageListDrawEx(
    _In_ HIMAGELIST ImageListHandle,
    _In_ INT Index,
    _In_ HDC Hdc,
    _In_ INT x,
    _In_ INT y,
    _In_ INT dx,
    _In_ INT dy,
    _In_ COLORREF BackColor,
    _In_ COLORREF ForeColor,
    _In_ UINT Style,
    _In_ DWORD State
    )
{
    IMAGELISTDRAWPARAMS imagelistDraw;

    memset(&imagelistDraw, 0, sizeof(IMAGELISTDRAWPARAMS));
    imagelistDraw.cbSize = sizeof(IMAGELISTDRAWPARAMS);
    imagelistDraw.himl = ImageListHandle;
    imagelistDraw.hdcDst = Hdc;
    imagelistDraw.i = Index;
    imagelistDraw.x = x;
    imagelistDraw.y = y;
    imagelistDraw.cx = dx;
    imagelistDraw.cy = dy;
    imagelistDraw.rgbBk = BackColor;
    imagelistDraw.rgbFg = ForeColor;
    imagelistDraw.fStyle = Style;
    imagelistDraw.fState = State;

    return SUCCEEDED(IImageList2_Draw((IImageList2*)ImageListHandle, &imagelistDraw));
}

BOOLEAN PhImageListSetIconSize(
    _In_ HIMAGELIST ImageListHandle,
    _In_ INT cx,
    _In_ INT cy
    )
{
    return SUCCEEDED(IImageList2_SetIconSize((IImageList2*)ImageListHandle, cx, cy));
}

static BOOLEAN CALLBACK PhpDpiChangedForwardEnumChildWindows(
    _In_ HWND WindowHandle,
    _In_opt_ PVOID Context
    )
{
    SendMessage(WindowHandle, WM_DPICHANGED, 0, 0);
    return TRUE;
}

VOID PhDpiChangedForwardChildWindows(
    _In_ HWND WindowHandle
    )
{
    PhEnumChildWindows(WindowHandle, 0x1000, PhpDpiChangedForwardEnumChildWindows, NULL);
}

static const PH_FLAG_MAPPING PhpInitiateShutdownMappings[] =
{
    { PH_SHUTDOWN_RESTART, SHUTDOWN_RESTART },
    { PH_SHUTDOWN_POWEROFF, SHUTDOWN_POWEROFF },
    { PH_SHUTDOWN_INSTALL_UPDATES, SHUTDOWN_INSTALL_UPDATES },
    { PH_SHUTDOWN_HYBRID, SHUTDOWN_HYBRID },
    { PH_SHUTDOWN_RESTART_BOOTOPTIONS, SHUTDOWN_RESTART_BOOTOPTIONS },
    { PH_CREATE_PROCESS_EXTENDED_STARTUPINFO, EXTENDED_STARTUPINFO_PRESENT },
};

ULONG PhInitiateShutdown(
    _In_ ULONG Flags
    )
{
    ULONG status;
    ULONG newFlags;

    newFlags = 0;
    PhMapFlags1(&newFlags, Flags, PhpInitiateShutdownMappings, ARRAYSIZE(PhpInitiateShutdownMappings));

    status = InitiateShutdown(
        NULL,
        NULL,
        0,
        SHUTDOWN_FORCE_OTHERS | newFlags,
        SHTDN_REASON_FLAG_PLANNED
        );

    return status;
}

VOID PhCustomDrawTreeTimeLine(
    _In_ HDC Hdc,
    _In_ RECT CellRect,
    _In_ ULONG Flags,
    _In_opt_ PLARGE_INTEGER StartTime,
    _In_ PLARGE_INTEGER CreateTime
    )
{
    HBRUSH previousBrush = NULL;
    RECT rect = CellRect;
    RECT borderRect = CellRect;
    FLOAT percent = 0;
    LARGE_INTEGER systemTime;
    LARGE_INTEGER startTime;
    LARGE_INTEGER createTime;

    if (StartTime)
    {
        PhQuerySystemTime(&systemTime);
        startTime.QuadPart = systemTime.QuadPart - StartTime->QuadPart;
        createTime.QuadPart = systemTime.QuadPart - CreateTime->QuadPart;
    }
    else
    {
        static LARGE_INTEGER bootTime = { 0 };

        if (bootTime.QuadPart == 0)
        {
            SYSTEM_TIMEOFDAY_INFORMATION timeOfDayInfo;

            if (NT_SUCCESS(NtQuerySystemInformation(
                SystemTimeOfDayInformation,
                &timeOfDayInfo,
                sizeof(SYSTEM_TIMEOFDAY_INFORMATION),
                NULL
                )))
            {
                bootTime.LowPart = timeOfDayInfo.BootTime.LowPart;
                bootTime.HighPart = timeOfDayInfo.BootTime.HighPart;
                bootTime.QuadPart -= timeOfDayInfo.BootTimeBias;
            }

            //if (NT_SUCCESS(NtQuerySystemInformation(
            //    SystemTimeOfDayInformation,
            //    &timeOfDayInfo,
            //    RTL_SIZEOF_THROUGH_FIELD(SYSTEM_TIMEOFDAY_INFORMATION, CurrentTime),
            //    NULL
            //    )))
            //{
            //    startTime.QuadPart = timeOfDayInfo.CurrentTime.QuadPart - timeOfDayInfo.BootTime.QuadPart;
            //    createTime.QuadPart = timeOfDayInfo.CurrentTime.QuadPart - processItem->CreateTime.QuadPart;
            //    percent = round((DOUBLE)((FLOAT)createTime.QuadPart / (FLOAT)startTime.QuadPart * 100));
            //}
        }

        PhQuerySystemTime(&systemTime);
        startTime.QuadPart = systemTime.QuadPart - bootTime.QuadPart;
        createTime.QuadPart = systemTime.QuadPart - CreateTime->QuadPart;
    }

    // Note: Time is 8 bytes, Float is 4 bytes. Use DOUBLE type at some stage. (dmex)
    percent = (FLOAT)createTime.QuadPart / (FLOAT)startTime.QuadPart * 100.f;

    if (!(Flags & PH_DRAW_TIMELINE_OVERFLOW))
    {
        // Prevent overflow from changing the system time to an earlier date. (dmex)
        if (fabsf(percent) > 100.f)
            percent = 100.f;
        if (fabsf(percent) < 0.0005f)
            percent = 0.f;
    }

    if (Flags & PH_DRAW_TIMELINE_DARKTHEME)
        FillRect(Hdc, &rect, PhMenuBackgroundBrush);
    else
        FillRect(Hdc, &rect, GetSysColorBrush(COLOR_WINDOW));

    PhInflateRect(&rect, -1, -1);
    rect.bottom += 1;

    if (Flags & PH_DRAW_TIMELINE_DARKTHEME)
    {
        FillRect(Hdc, &rect, PhMenuBackgroundBrush);

        if (Flags & PH_DRAW_TIMELINE_OVERFLOW) // System threads created before startup. (dmex)
            SetDCBrushColor(Hdc, percent > 100.f ? RGB(128, 128, 128) : RGB(0, 130, 135));
        else
            SetDCBrushColor(Hdc, RGB(0, 130, 135));

        previousBrush = SelectBrush(Hdc, GetStockBrush(DC_BRUSH));
    }
    else
    {
        FillRect(Hdc, &rect, GetSysColorBrush(COLOR_3DFACE));

        if (Flags & PH_DRAW_TIMELINE_OVERFLOW) // System threads created before startup. (dmex)
            SetDCBrushColor(Hdc, percent > 100.f ? RGB(128, 128, 128) : RGB(158, 202, 158));
        else
            SetDCBrushColor(Hdc, RGB(158, 202, 158));

        previousBrush = SelectBrush(Hdc, GetStockBrush(DC_BRUSH));
    }

    if (Flags & PH_DRAW_TIMELINE_OVERFLOW)
    {
        // Prevent overflow from changing the system time to an earlier date. (dmex)
        if (fabsf(percent) > 100.f)
            percent = 100.f;
        if (fabsf(percent) < 0.0005f)
            percent = 0.f;
    }

    //rect.right = ((LONG)(rect.left + ((rect.right - rect.left) * (LONG)percent) / 100));
    //rect.left = ((LONG)(rect.right + ((rect.left - rect.right) * (LONG)percent) / 100));
    rect.left = (LONG)(rect.right + ((rect.left - rect.right) * percent / 100));

    PatBlt(
        Hdc,
        rect.left,
        rect.top,
        rect.right - rect.left,
        rect.bottom - rect.top,
        PATCOPY
        );

    if (previousBrush) SelectBrush(Hdc, previousBrush);

    PhInflateRect(&borderRect, -1, -1);
    borderRect.bottom += 1;
    FrameRect(Hdc, &borderRect, GetStockBrush(GRAY_BRUSH));
}

// Windows Imaging Component (WIC) bitmap support 

static PVOID PhpGetWicImagingFactoryInterface(
    VOID
    )
{
    static PH_INITONCE initOnce = PH_INITONCE_INIT;
    static PVOID wicImagingFactory = NULL;

    if (PhBeginInitOnce(&initOnce))
    {
        if (WindowsVersion > WINDOWS_8)
            PhGetClassObject(L"windowscodecs.dll", &CLSID_WICImagingFactory2, &IID_IWICImagingFactory, &wicImagingFactory);
        else
            PhGetClassObject(L"windowscodecs.dll", &CLSID_WICImagingFactory1, &IID_IWICImagingFactory, &wicImagingFactory);

        PhEndInitOnce(&initOnce);
    }

    return wicImagingFactory;
}

static PGUID PhpGetImageFormatDecoderType(
    _In_ PH_IMAGE_FORMAT_TYPE Format
    )
{
    switch (Format)
    {
    case PH_IMAGE_FORMAT_TYPE_ICO:
        return (PGUID)&GUID_ContainerFormatIco;
    case PH_IMAGE_FORMAT_TYPE_BMP:
        return (PGUID)&GUID_ContainerFormatBmp;
    case PH_IMAGE_FORMAT_TYPE_JPG:
        return (PGUID)&GUID_ContainerFormatJpeg;
    case PH_IMAGE_FORMAT_TYPE_PNG:
        return (PGUID)&GUID_ContainerFormatPng;
    }

    return (PGUID)&GUID_ContainerFormatRaw;
}

HBITMAP PhLoadImageFormatFromResource(
    _In_ PVOID DllBase,
    _In_ PCWSTR Name,
    _In_ PCWSTR Type,
    _In_ PH_IMAGE_FORMAT_TYPE Format,
    _In_ UINT Width,
    _In_ UINT Height
    )
{
    BOOLEAN success = FALSE;
    HBITMAP bitmapHandle = NULL;
    ULONG resourceLength = 0;
    WICInProcPointer resourceBuffer = NULL;
    PVOID bitmapBuffer = NULL;
    IWICImagingFactory* wicImageFactory = NULL;
    IWICStream* wicBitmapStream = NULL;
    IWICBitmapSource* wicBitmapSource = NULL;
    IWICBitmapDecoder* wicBitmapDecoder = NULL;
    IWICBitmapFrameDecode* wicBitmapFrame = NULL;
    WICPixelFormatGUID pixelFormat;
    UINT sourceWidth = 0;
    UINT sourceHeight = 0;

    if (!PhLoadResource(DllBase, Name, Type, &resourceLength, &resourceBuffer))
        goto CleanupExit;

    if (!(wicImageFactory = PhpGetWicImagingFactoryInterface()))
        goto CleanupExit;
    if (FAILED(IWICImagingFactory_CreateStream(wicImageFactory, &wicBitmapStream)))
        goto CleanupExit;
    if (FAILED(IWICStream_InitializeFromMemory(wicBitmapStream, resourceBuffer, resourceLength)))
        goto CleanupExit;
    if (FAILED(IWICImagingFactory_CreateDecoder(wicImageFactory, PhpGetImageFormatDecoderType(Format), &GUID_VendorMicrosoft, &wicBitmapDecoder)))
        goto CleanupExit;
    if (FAILED(IWICBitmapDecoder_Initialize(wicBitmapDecoder, (IStream*)wicBitmapStream, WICDecodeMetadataCacheOnDemand)))
        goto CleanupExit;
    if (FAILED(IWICBitmapDecoder_GetFrame(wicBitmapDecoder, 0, &wicBitmapFrame)))
        goto CleanupExit;
    if (FAILED(IWICBitmapFrameDecode_GetPixelFormat(wicBitmapFrame, &pixelFormat)))
        goto CleanupExit;

    if (IsEqualGUID(&pixelFormat, &GUID_WICPixelFormat32bppBGRA)) // CreateDIBSection format
    {
        if (FAILED(IWICBitmapFrameDecode_QueryInterface(wicBitmapFrame, &IID_IWICBitmapSource, &wicBitmapSource)))
            goto CleanupExit;
    }
    else
    {
        IWICFormatConverter* wicFormatConverter = NULL;

        if (FAILED(IWICImagingFactory_CreateFormatConverter(wicImageFactory, &wicFormatConverter)))
            goto CleanupExit;

        if (FAILED(IWICFormatConverter_Initialize(
            wicFormatConverter,
            (IWICBitmapSource*)wicBitmapFrame,
            &GUID_WICPixelFormat32bppBGRA,
            WICBitmapDitherTypeNone,
            NULL,
            0.0f,
            WICBitmapPaletteTypeCustom
            )))
        {
            IWICFormatConverter_Release(wicFormatConverter);
            goto CleanupExit;
        }

        if (FAILED(IWICFormatConverter_QueryInterface(wicFormatConverter, &IID_IWICBitmapSource, &wicBitmapSource)))
            goto CleanupExit;

        IWICFormatConverter_Release(wicFormatConverter);
    }

    {
        BITMAPINFO bitmapInfo;
        HDC screenHdc;

        memset(&bitmapInfo, 0, sizeof(BITMAPINFO));
        bitmapInfo.bmiHeader.biSize = sizeof(BITMAPINFOHEADER);
        bitmapInfo.bmiHeader.biPlanes = 1;
        bitmapInfo.bmiHeader.biCompression = BI_RGB;
        bitmapInfo.bmiHeader.biWidth = Width;
        bitmapInfo.bmiHeader.biHeight = -((LONG)Height);
        bitmapInfo.bmiHeader.biBitCount = 32;

        screenHdc = GetDC(NULL);
        bitmapHandle = CreateDIBSection(screenHdc, &bitmapInfo, DIB_RGB_COLORS, &bitmapBuffer, NULL, 0);
        ReleaseDC(NULL, screenHdc);
    }

    if (SUCCEEDED(IWICBitmapSource_GetSize(wicBitmapSource, &sourceWidth, &sourceHeight)) && sourceWidth == Width && sourceHeight == Height)
    {
        if (SUCCEEDED(IWICBitmapSource_CopyPixels(
            wicBitmapSource,
            NULL,
            Width * sizeof(RGBQUAD),
            Width * Height * sizeof(RGBQUAD),
            bitmapBuffer
            )))
        {
            success = TRUE;
        }
    }
    else
    {
        IWICBitmapScaler* wicBitmapScaler = NULL;

        if (SUCCEEDED(IWICImagingFactory_CreateBitmapScaler(wicImageFactory, &wicBitmapScaler)))
        {
            if (SUCCEEDED(IWICBitmapScaler_Initialize(
                wicBitmapScaler,
                wicBitmapSource,
                Width,
                Height,
                WindowsVersion < WINDOWS_10 ? WICBitmapInterpolationModeFant : WICBitmapInterpolationModeHighQualityCubic
                )))
            {
                if (SUCCEEDED(IWICBitmapScaler_CopyPixels(
                    wicBitmapScaler,
                    NULL,
                    Width * sizeof(RGBQUAD),
                    Width * Height * sizeof(RGBQUAD),
                    bitmapBuffer
                    )))
                {
                    success = TRUE;
                }
            }

            IWICBitmapScaler_Release(wicBitmapScaler);
        }
    }

CleanupExit:

    if (wicBitmapSource)
        IWICBitmapSource_Release(wicBitmapSource);
    if (wicBitmapDecoder)
        IWICBitmapDecoder_Release(wicBitmapDecoder);
    if (wicBitmapFrame)
        IWICBitmapFrameDecode_Release(wicBitmapFrame);
    if (wicBitmapStream)
        IWICStream_Release(wicBitmapStream);

    if (!success)
    {
        if (bitmapHandle) DeleteBitmap(bitmapHandle);
        return NULL;
    }

    return bitmapHandle;
}

// Load image and auto-detect the format (dmex)
HBITMAP PhLoadImageFromResource(
    _In_ PVOID DllBase,
    _In_ PCWSTR Name,
    _In_ PCWSTR Type,
    _In_ UINT Width,
    _In_ UINT Height
    )
{
    BOOLEAN success = FALSE;
    HBITMAP bitmapHandle = NULL;
    ULONG resourceLength = 0;
    WICInProcPointer resourceBuffer = NULL;
    PVOID bitmapBuffer = NULL;
    IWICImagingFactory* wicImageFactory = NULL;
    IWICStream* wicBitmapStream = NULL;
    IWICBitmapSource* wicBitmapSource = NULL;
    IWICBitmapDecoder* wicBitmapDecoder = NULL;
    IWICBitmapFrameDecode* wicBitmapFrame = NULL;
    WICPixelFormatGUID pixelFormat;
    UINT sourceWidth = 0;
    UINT sourceHeight = 0;

    if (!PhLoadResource(DllBase, Name, Type, &resourceLength, &resourceBuffer))
        goto CleanupExit;

    if (!(wicImageFactory = PhpGetWicImagingFactoryInterface()))
        goto CleanupExit;
    if (FAILED(IWICImagingFactory_CreateStream(wicImageFactory, &wicBitmapStream)))
        goto CleanupExit;
    if (FAILED(IWICStream_InitializeFromMemory(wicBitmapStream, resourceBuffer, resourceLength)))
        goto CleanupExit;
    if (FAILED(IWICImagingFactory_CreateDecoderFromStream(wicImageFactory, (IStream*)wicBitmapStream, &GUID_VendorMicrosoft, WICDecodeMetadataCacheOnDemand, &wicBitmapDecoder)))
        goto CleanupExit;
    if (FAILED(IWICBitmapDecoder_GetFrame(wicBitmapDecoder, 0, &wicBitmapFrame)))
        goto CleanupExit;
    if (FAILED(IWICBitmapFrameDecode_GetPixelFormat(wicBitmapFrame, &pixelFormat)))
        goto CleanupExit;

    if (IsEqualGUID(&pixelFormat, &GUID_WICPixelFormat32bppBGRA))
    {
        if (FAILED(IWICBitmapFrameDecode_QueryInterface(wicBitmapFrame, &IID_IWICBitmapSource, &wicBitmapSource)))
            goto CleanupExit;
    }
    else
    {
        IWICFormatConverter* wicFormatConverter = NULL;

        if (FAILED(IWICImagingFactory_CreateFormatConverter(wicImageFactory, &wicFormatConverter)))
            goto CleanupExit;

        if (FAILED(IWICFormatConverter_Initialize(
            wicFormatConverter,
            (IWICBitmapSource*)wicBitmapFrame,
            &GUID_WICPixelFormat32bppBGRA,
            WICBitmapDitherTypeNone,
            NULL,
            0.0f,
            WICBitmapPaletteTypeCustom
            )))
        {
            IWICFormatConverter_Release(wicFormatConverter);
            goto CleanupExit;
        }

        if (FAILED(IWICFormatConverter_QueryInterface(wicFormatConverter, &IID_IWICBitmapSource, &wicBitmapSource)))
            goto CleanupExit;

        IWICFormatConverter_Release(wicFormatConverter);
    }

    {
        BITMAPINFO bitmapInfo;
        HDC screenHdc;

        memset(&bitmapInfo, 0, sizeof(BITMAPINFO));
        bitmapInfo.bmiHeader.biSize = sizeof(BITMAPINFOHEADER);
        bitmapInfo.bmiHeader.biPlanes = 1;
        bitmapInfo.bmiHeader.biCompression = BI_RGB;
        bitmapInfo.bmiHeader.biWidth = Width;
        bitmapInfo.bmiHeader.biHeight = -((LONG)Height);
        bitmapInfo.bmiHeader.biBitCount = 32;

        screenHdc = GetDC(NULL);
        bitmapHandle = CreateDIBSection(screenHdc, &bitmapInfo, DIB_RGB_COLORS, &bitmapBuffer, NULL, 0);
        ReleaseDC(NULL, screenHdc);
    }

    if (SUCCEEDED(IWICBitmapSource_GetSize(wicBitmapSource, &sourceWidth, &sourceHeight)) && sourceWidth == Width && sourceHeight == Height)
    {
        if (SUCCEEDED(IWICBitmapSource_CopyPixels(
            wicBitmapSource, 
            NULL, 
            Width * sizeof(RGBQUAD), 
            Width * Height * sizeof(RGBQUAD),
            bitmapBuffer
            )))
        {
            success = TRUE;
        }
    }
    else
    {
        IWICBitmapScaler* wicBitmapScaler = NULL;

        if (SUCCEEDED(IWICImagingFactory_CreateBitmapScaler(wicImageFactory, &wicBitmapScaler)))
        {
            if (SUCCEEDED(IWICBitmapScaler_Initialize(
                wicBitmapScaler,
                wicBitmapSource,
                Width, 
                Height, 
                WindowsVersion < WINDOWS_10 ? WICBitmapInterpolationModeFant : WICBitmapInterpolationModeHighQualityCubic
                )))
            {
                if (SUCCEEDED(IWICBitmapScaler_CopyPixels(
                    wicBitmapScaler, 
                    NULL, 
                    Width * sizeof(RGBQUAD), 
                    Width * Height * sizeof(RGBQUAD),
                    bitmapBuffer
                    )))
                {
                    success = TRUE;
                }
            }

            IWICBitmapScaler_Release(wicBitmapScaler);
        }
    }

CleanupExit:

    if (wicBitmapSource)
        IWICBitmapSource_Release(wicBitmapSource);
    if (wicBitmapDecoder)
        IWICBitmapDecoder_Release(wicBitmapDecoder);
    if (wicBitmapFrame)
        IWICBitmapFrameDecode_Release(wicBitmapFrame);
    if (wicBitmapStream)
        IWICStream_Release(wicBitmapStream);

    if (!success)
    {
        if (bitmapHandle) DeleteBitmap(bitmapHandle);
        return NULL;
    }

    return bitmapHandle;
}

// Load image and auto-detect the format (dmex)
HBITMAP PhLoadImageFromFile(
    _In_ PWSTR FileName,
    _In_ UINT Width,
    _In_ UINT Height
    )
{
    BOOLEAN success = FALSE;
    HBITMAP bitmapHandle = NULL;
    PVOID bitmapBuffer = NULL;
    IWICImagingFactory* wicFactory = NULL;
    IWICBitmapSource* wicBitmapSource = NULL;
    IWICBitmapDecoder* wicBitmapDecoder = NULL;
    IWICBitmapFrameDecode* wicBitmapFrame = NULL;
    WICPixelFormatGUID pixelFormat;
    UINT sourceWidth = 0;
    UINT sourceHeight = 0;

    if (!(wicFactory = PhpGetWicImagingFactoryInterface()))
        goto CleanupExit;
    if (FAILED(IWICImagingFactory_CreateDecoderFromFilename(wicFactory, FileName, &GUID_VendorMicrosoft, GENERIC_READ, WICDecodeMetadataCacheOnDemand, &wicBitmapDecoder)))
        goto CleanupExit;
    if (FAILED(IWICBitmapDecoder_GetFrame(wicBitmapDecoder, 0, &wicBitmapFrame)))
        goto CleanupExit;
    if (FAILED(IWICBitmapFrameDecode_GetPixelFormat(wicBitmapFrame, &pixelFormat)))
        goto CleanupExit;

    if (IsEqualGUID(&pixelFormat, &GUID_WICPixelFormat32bppBGRA))
    {
        if (FAILED(IWICBitmapFrameDecode_QueryInterface(wicBitmapFrame, &IID_IWICBitmapSource, &wicBitmapSource)))
            goto CleanupExit;
    }
    else
    {
        IWICFormatConverter* wicFormatConverter = NULL;

        if (FAILED(IWICImagingFactory_CreateFormatConverter(wicFactory, &wicFormatConverter)))
            goto CleanupExit;

        if (FAILED(IWICFormatConverter_Initialize(
            wicFormatConverter,
            (IWICBitmapSource*)wicBitmapFrame,
            &GUID_WICPixelFormat32bppBGRA,
            WICBitmapDitherTypeNone,
            NULL,
            0.0f,
            WICBitmapPaletteTypeCustom
            )))
        {
            IWICFormatConverter_Release(wicFormatConverter);
            goto CleanupExit;
        }

        if (FAILED(IWICFormatConverter_QueryInterface(wicFormatConverter, &IID_IWICBitmapSource, &wicBitmapSource)))
            goto CleanupExit;

        IWICFormatConverter_Release(wicFormatConverter);
    }

    {
        BITMAPINFO bitmapInfo;
        HDC screenHdc;

        memset(&bitmapInfo, 0, sizeof(BITMAPINFO));
        bitmapInfo.bmiHeader.biSize = sizeof(BITMAPINFOHEADER);
        bitmapInfo.bmiHeader.biPlanes = 1;
        bitmapInfo.bmiHeader.biCompression = BI_RGB;
        bitmapInfo.bmiHeader.biWidth = Width;
        bitmapInfo.bmiHeader.biHeight = -((LONG)Height);
        bitmapInfo.bmiHeader.biBitCount = 32;

        screenHdc = GetDC(NULL);
        bitmapHandle = CreateDIBSection(screenHdc, &bitmapInfo, DIB_RGB_COLORS, &bitmapBuffer, NULL, 0);
        ReleaseDC(NULL, screenHdc);
    }

    if (SUCCEEDED(IWICBitmapSource_GetSize(wicBitmapSource, &sourceWidth, &sourceHeight)) && sourceWidth == Width && sourceHeight == Height)
    {
        if (SUCCEEDED(IWICBitmapSource_CopyPixels(
            wicBitmapSource,
            NULL,
            Width * sizeof(RGBQUAD),
            Width * Height * sizeof(RGBQUAD),
            bitmapBuffer
            )))
        {
            success = TRUE;
        }
    }
    else
    {
        IWICBitmapScaler* wicBitmapScaler = NULL;

        if (SUCCEEDED(IWICImagingFactory_CreateBitmapScaler(wicFactory, &wicBitmapScaler)))
        {
            if (SUCCEEDED(IWICBitmapScaler_Initialize(
                wicBitmapScaler,
                wicBitmapSource,
                Width,
                Height,
                WindowsVersion < WINDOWS_10 ? WICBitmapInterpolationModeFant : WICBitmapInterpolationModeHighQualityCubic
                )))
            {
                if (SUCCEEDED(IWICBitmapScaler_CopyPixels(
                    wicBitmapScaler,
                    NULL,
                    Width * sizeof(RGBQUAD),
                    Width * Height * sizeof(RGBQUAD),
                    bitmapBuffer
                    )))
                {
                    success = TRUE;
                }
            }

            IWICBitmapScaler_Release(wicBitmapScaler);
        }
    }

CleanupExit:

    if (wicBitmapSource)
        IWICBitmapSource_Release(wicBitmapSource);
    if (wicBitmapDecoder)
        IWICBitmapDecoder_Release(wicBitmapDecoder);
    if (wicBitmapFrame)
        IWICBitmapFrameDecode_Release(wicBitmapFrame);

    if (!success)
    {
        if (bitmapHandle) DeleteBitmap(bitmapHandle);
        return NULL;
    }

    return bitmapHandle;
}
