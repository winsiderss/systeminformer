/*
 * Process Hacker -
 *   GDI handles dialog
 *
 * Copyright (C) 2010 wj32
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

#include <phapp.h>

#include <ntgdi.h>

#include <procprv.h>

typedef struct _GDI_HANDLES_CONTEXT
{
    PPH_PROCESS_ITEM ProcessItem;
    PPH_LIST List;
} GDI_HANDLES_CONTEXT, *PGDI_HANDLES_CONTEXT;

typedef struct _PH_GDI_HANDLE_ITEM
{
    PGDI_HANDLE_ENTRY Entry;
    ULONG Handle;
    PVOID Object;
    PWSTR TypeName;
    PPH_STRING Information;
} PH_GDI_HANDLE_ITEM, *PPH_GDI_HANDLE_ITEM;

INT_PTR CALLBACK PhpGdiHandlesDlgProc(
    _In_ HWND hwndDlg,
    _In_ UINT uMsg,
    _In_ WPARAM wParam,
    _In_ LPARAM lParam
    );

VOID PhShowGdiHandlesDialog(
    _In_ HWND ParentWindowHandle,
    _In_ PPH_PROCESS_ITEM ProcessItem
    )
{
    GDI_HANDLES_CONTEXT context;
    ULONG i;

    context.ProcessItem = ProcessItem;
    context.List = PhCreateList(20);

    DialogBoxParam(
        PhInstanceHandle,
        MAKEINTRESOURCE(IDD_GDIHANDLES),
        ParentWindowHandle,
        PhpGdiHandlesDlgProc,
        (LPARAM)&context
        );

    for (i = 0; i < context.List->Count; i++)
    {
        PPH_GDI_HANDLE_ITEM gdiHandleItem = context.List->Items[i];

        if (gdiHandleItem->Information)
            PhDereferenceObject(gdiHandleItem->Information);

        PhFree(context.List->Items[i]);
    }

    PhDereferenceObject(context.List);
}

PWSTR PhpGetGdiHandleTypeName(
    _In_ ULONG Unique
    )
{
    switch (GDI_CLIENT_TYPE_FROM_UNIQUE(Unique))
    {
    case GDI_CLIENT_ALTDC_TYPE:
        return L"Alt. DC";
    case GDI_CLIENT_BITMAP_TYPE:
        return L"Bitmap";
    case GDI_CLIENT_BRUSH_TYPE:
        return L"Brush";
    case GDI_CLIENT_CLIENTOBJ_TYPE:
        return L"Client Object";
    case GDI_CLIENT_DIBSECTION_TYPE:
        return L"DIB Section";
    case GDI_CLIENT_DC_TYPE:
        return L"DC";
    case GDI_CLIENT_EXTPEN_TYPE:
        return L"ExtPen";
    case GDI_CLIENT_FONT_TYPE:
        return L"Font";
    case GDI_CLIENT_METADC16_TYPE:
        return L"Metafile DC";
    case GDI_CLIENT_METAFILE_TYPE:
        return L"Enhanced Metafile";
    case GDI_CLIENT_METAFILE16_TYPE:
        return L"Metafile";
    case GDI_CLIENT_PALETTE_TYPE:
        return L"Palette";
    case GDI_CLIENT_PEN_TYPE:
        return L"Pen";
    case GDI_CLIENT_REGION_TYPE:
        return L"Region";
    default:
        return NULL;
    }
}

PPH_STRING PhpGetGdiHandleInformation(
    _In_ ULONG Handle
    )
{
    HGDIOBJ handle;

    handle = (HGDIOBJ)UlongToPtr(Handle);

    switch (GDI_CLIENT_TYPE_FROM_HANDLE(Handle))
    {
    case GDI_CLIENT_BITMAP_TYPE:
    case GDI_CLIENT_DIBSECTION_TYPE:
        {
            BITMAP bitmap;

            if (GetObject(handle, sizeof(BITMAP), &bitmap))
            {
                return PhFormatString(
                    L"Width: %u, Height: %u, Depth: %u",
                    bitmap.bmWidth,
                    bitmap.bmHeight,
                    bitmap.bmBitsPixel
                    );
            }
        }
        break;
    case GDI_CLIENT_BRUSH_TYPE:
        {
            LOGBRUSH brush;

            if (GetObject(handle, sizeof(LOGBRUSH), &brush))
            {
                return PhFormatString(
                    L"Style: %u, Color: 0x%08x, Hatch: 0x%Ix",
                    brush.lbStyle,
                    _byteswap_ulong(brush.lbColor),
                    brush.lbHatch
                    );
            }
        }
        break;
    case GDI_CLIENT_EXTPEN_TYPE:
        {
            EXTLOGPEN pen;

            if (GetObject(handle, sizeof(EXTLOGPEN), &pen))
            {
                return PhFormatString(
                    L"Style: 0x%x, Width: %u, Color: 0x%08x",
                    pen.elpPenStyle,
                    pen.elpWidth,
                    _byteswap_ulong(pen.elpColor)
                    );
            }
        }
        break;
    case GDI_CLIENT_FONT_TYPE:
        {
            LOGFONT font;

            if (GetObject(handle, sizeof(LOGFONT), &font))
            {
                return PhFormatString(
                    L"Face: %s, Height: %d",
                    font.lfFaceName,
                    font.lfHeight
                    );
            }
        }
        break;
    case GDI_CLIENT_PALETTE_TYPE:
        {
            USHORT count;

            if (GetObject(handle, sizeof(USHORT), &count))
            {
                return PhFormatString(
                    L"Entries: %u",
                    (ULONG)count
                    );
            }
        }
        break;
    case GDI_CLIENT_PEN_TYPE:
        {
            LOGPEN pen;

            if (GetObject(handle, sizeof(LOGPEN), &pen))
            {
                return PhFormatString(
                    L"Style: %u, Width: %u, Color: 0x%08x",
                    pen.lopnStyle,
                    pen.lopnWidth.x,
                    _byteswap_ulong(pen.lopnColor)
                    );
            }
        }
        break;
    }

    return NULL;
}

VOID PhpRefreshGdiHandles(
    _In_ HWND hwndDlg,
    _In_ PGDI_HANDLES_CONTEXT Context
    )
{
    HWND lvHandle;
    ULONG i;
    PGDI_SHARED_MEMORY gdiShared;
    USHORT processId;
    PGDI_HANDLE_ENTRY handle;
    PPH_GDI_HANDLE_ITEM gdiHandleItem;

    lvHandle = GetDlgItem(hwndDlg, IDC_LIST);

    ExtendedListView_SetRedraw(lvHandle, FALSE);
    ListView_DeleteAllItems(lvHandle);

    for (i = 0; i < Context->List->Count; i++)
    {
        gdiHandleItem = Context->List->Items[i];

        if (gdiHandleItem->Information)
            PhDereferenceObject(gdiHandleItem->Information);

        PhFree(Context->List->Items[i]);
    }

    PhClearList(Context->List);

    gdiShared = (PGDI_SHARED_MEMORY)NtCurrentPeb()->GdiSharedHandleTable;
    processId = (USHORT)Context->ProcessItem->ProcessId;

    for (i = 0; i < GDI_MAX_HANDLE_COUNT; i++)
    {
        PWSTR typeName;
        INT lvItemIndex;
        WCHAR pointer[PH_PTR_STR_LEN_1];

        handle = &gdiShared->Handles[i];

        if (handle->Owner.ProcessId != processId)
            continue;

        typeName = PhpGetGdiHandleTypeName(handle->Unique);

        if (!typeName)
            continue;

        gdiHandleItem = PhAllocate(sizeof(PH_GDI_HANDLE_ITEM));
        gdiHandleItem->Entry = handle;
        gdiHandleItem->Handle = GDI_MAKE_HANDLE(i, handle->Unique);
        gdiHandleItem->Object = handle->Object;
        gdiHandleItem->TypeName = typeName;
        gdiHandleItem->Information = PhpGetGdiHandleInformation(gdiHandleItem->Handle);
        PhAddItemList(Context->List, gdiHandleItem);

        lvItemIndex = PhAddListViewItem(lvHandle, MAXINT, gdiHandleItem->TypeName, gdiHandleItem);
        PhPrintPointer(pointer, UlongToPtr(gdiHandleItem->Handle));
        PhSetListViewSubItem(lvHandle, lvItemIndex, 1, pointer);
        PhPrintPointer(pointer, gdiHandleItem->Object);
        PhSetListViewSubItem(lvHandle, lvItemIndex, 2, pointer);
        PhSetListViewSubItem(lvHandle, lvItemIndex, 3, PhGetString(gdiHandleItem->Information));
    }

    ExtendedListView_SortItems(lvHandle);
    ExtendedListView_SetRedraw(lvHandle, TRUE);
}

INT NTAPI PhpGdiHandleHandleCompareFunction(
    _In_ PVOID Item1,
    _In_ PVOID Item2,
    _In_opt_ PVOID Context
    )
{
    PPH_GDI_HANDLE_ITEM item1 = Item1;
    PPH_GDI_HANDLE_ITEM item2 = Item2;

    return uintcmp(item1->Handle, item2->Handle);
}

INT NTAPI PhpGdiHandleObjectCompareFunction(
    _In_ PVOID Item1,
    _In_ PVOID Item2,
    _In_opt_ PVOID Context
    )
{
    PPH_GDI_HANDLE_ITEM item1 = Item1;
    PPH_GDI_HANDLE_ITEM item2 = Item2;

    return uintptrcmp((ULONG_PTR)item1->Object, (ULONG_PTR)item2->Object);
}

INT_PTR CALLBACK PhpGdiHandlesDlgProc(
    _In_ HWND hwndDlg,
    _In_ UINT uMsg,
    _In_ WPARAM wParam,
    _In_ LPARAM lParam
    )
{
    switch (uMsg)
    {
    case WM_INITDIALOG:
        {
            PGDI_HANDLES_CONTEXT context = (PGDI_HANDLES_CONTEXT)lParam;
            HWND lvHandle;

            PhCenterWindow(hwndDlg, GetParent(hwndDlg));

            SetProp(hwndDlg, PhMakeContextAtom(), (HANDLE)context);

            lvHandle = GetDlgItem(hwndDlg, IDC_LIST);

            PhSetListViewStyle(lvHandle, FALSE, TRUE);
            PhSetControlTheme(lvHandle, L"explorer");
            PhAddListViewColumn(lvHandle, 0, 0, 0, LVCFMT_LEFT, 100, L"Type");
            PhAddListViewColumn(lvHandle, 1, 1, 1, LVCFMT_LEFT, 80, L"Handle");
            PhAddListViewColumn(lvHandle, 2, 2, 2, LVCFMT_LEFT, 102, L"Object");
            PhAddListViewColumn(lvHandle, 3, 3, 3, LVCFMT_LEFT, 200, L"Information");

            PhSetExtendedListView(lvHandle);
            ExtendedListView_SetCompareFunction(lvHandle, 1, PhpGdiHandleHandleCompareFunction);
            ExtendedListView_SetCompareFunction(lvHandle, 2, PhpGdiHandleObjectCompareFunction);
            ExtendedListView_AddFallbackColumn(lvHandle, 0);
            ExtendedListView_AddFallbackColumn(lvHandle, 1);

            PhpRefreshGdiHandles(hwndDlg, context);
        }
        break;
    case WM_DESTROY:
        {
            RemoveProp(hwndDlg, PhMakeContextAtom());
        }
        break;
    case WM_COMMAND:
        {
            switch (LOWORD(wParam))
            {
            case IDCANCEL:
            case IDOK:
                EndDialog(hwndDlg, IDOK);
                break;
            case IDC_REFRESH:
                {
                    PhpRefreshGdiHandles(hwndDlg, (PGDI_HANDLES_CONTEXT)GetProp(hwndDlg, PhMakeContextAtom()));
                }
                break;
            }
        }
        break;
    case WM_NOTIFY:
        {
            PhHandleListViewNotifyForCopy(lParam, GetDlgItem(hwndDlg, IDC_LIST));
        }
        break;
    }

    return FALSE;
}
