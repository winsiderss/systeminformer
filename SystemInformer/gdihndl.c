/*
 * Copyright (c) 2022 Winsider Seminars & Solutions, Inc.  All rights reserved.
 *
 * This file is part of System Informer.
 *
 * Authors:
 *
 *     wj32    2010
 *     dmex    2021-2023
 *
 */

#include <phapp.h>
#include <procprv.h>
#include <phsettings.h>
#include <emenu.h>

#include <ntgdi.h>

typedef struct _PH_GDI_HANDLES_CONTEXT
{
    HWND ListViewHandle;
    HWND ParentWindowHandle;
    PPH_PROCESS_ITEM ProcessItem;
    PPH_LIST List;
    PH_LAYOUT_MANAGER LayoutManager;
} PH_GDI_HANDLES_CONTEXT, *PPH_GDI_HANDLES_CONTEXT;

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
    PPH_GDI_HANDLES_CONTEXT context;
    HWND windowHandle;

    context = PhAllocateZero(sizeof(PH_GDI_HANDLES_CONTEXT));
    context->ProcessItem = PhReferenceObject(ProcessItem);
    context->List = PhCreateList(20);
    context->ParentWindowHandle = ParentWindowHandle;

    windowHandle = PhCreateDialog(
        PhInstanceHandle,
        MAKEINTRESOURCE(IDD_GDIHANDLES),
        PhCsForceNoParent ? NULL : ParentWindowHandle,
        PhpGdiHandlesDlgProc,
        context
        );

    ShowWindow(windowHandle, SW_SHOW);
    SetForegroundWindow(windowHandle);
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
    _In_ PPH_GDI_HANDLES_CONTEXT Context
    )
{
    ULONG i;
    PGDI_SHARED_MEMORY gdiShared;
    USHORT processId;
    ULONG handleCount;
    PGDI_HANDLE_ENTRY handle;
    PPH_GDI_HANDLE_ITEM gdiHandleItem;
    MEMORY_BASIC_INFORMATION basicInfo;

    memset(&basicInfo, 0, sizeof(MEMORY_BASIC_INFORMATION));

    ExtendedListView_SetRedraw(Context->ListViewHandle, FALSE);
    ListView_DeleteAllItems(Context->ListViewHandle);

    for (i = 0; i < Context->List->Count; i++)
    {
        gdiHandleItem = Context->List->Items[i];

        if (gdiHandleItem->Information)
            PhDereferenceObject(gdiHandleItem->Information);

        PhFree(Context->List->Items[i]);
        Context->List->Items[i] = NULL;
    }

    PhClearList(Context->List);

    gdiShared = (PGDI_SHARED_MEMORY)NtCurrentPeb()->GdiSharedHandleTable;
    processId = (USHORT)HandleToUlong(Context->ProcessItem->ProcessId);
    handleCount = GDI_MAX_HANDLE_COUNT;

    if (NT_SUCCESS(NtQueryVirtualMemory(
        NtCurrentProcess(),
        gdiShared,
        MemoryBasicInformation,
        &basicInfo,
        sizeof(MEMORY_BASIC_INFORMATION),
        NULL
        )))
    {
        handleCount = (ULONG)(basicInfo.RegionSize / sizeof(GDI_HANDLE_ENTRY));
        handleCount = __min(GDI_MAX_HANDLE_COUNT, handleCount);
    }

    for (i = 0; i < handleCount; i++)
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

        gdiHandleItem = PhAllocateZero(sizeof(PH_GDI_HANDLE_ITEM));
        gdiHandleItem->Entry = handle;
        gdiHandleItem->Handle = GDI_MAKE_HANDLE(i, handle->Unique);
        gdiHandleItem->Object = handle->Object;
        gdiHandleItem->TypeName = typeName;
        gdiHandleItem->Information = PhpGetGdiHandleInformation(gdiHandleItem->Handle);
        PhAddItemList(Context->List, gdiHandleItem);

        lvItemIndex = PhAddListViewItem(Context->ListViewHandle, MAXINT, gdiHandleItem->TypeName, gdiHandleItem);
        PhPrintPointer(pointer, UlongToPtr(gdiHandleItem->Handle));
        PhSetListViewSubItem(Context->ListViewHandle, lvItemIndex, 1, pointer);
        PhPrintPointer(pointer, gdiHandleItem->Object);
        PhSetListViewSubItem(Context->ListViewHandle, lvItemIndex, 2, pointer);
        PhSetListViewSubItem(Context->ListViewHandle, lvItemIndex, 3, PhGetString(gdiHandleItem->Information));
    }

    ExtendedListView_SortItems(Context->ListViewHandle);
    ExtendedListView_SetRedraw(Context->ListViewHandle, TRUE);
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
    PPH_GDI_HANDLES_CONTEXT context = NULL;

    if (uMsg == WM_INITDIALOG)
    {
        context = (PPH_GDI_HANDLES_CONTEXT)lParam;

        PhSetWindowContext(hwndDlg, PH_WINDOW_CONTEXT_DEFAULT, context);
    }
    else
    {
        context = PhGetWindowContext(hwndDlg, PH_WINDOW_CONTEXT_DEFAULT);
    }

    if (!context)
        return FALSE;

    switch (uMsg)
    {
    case WM_INITDIALOG:
        {
            context->ListViewHandle = GetDlgItem(hwndDlg, IDC_LIST);

            PhSetApplicationWindowIcon(hwndDlg);

            PhCenterWindow(hwndDlg, context->ParentWindowHandle);

            PhRegisterDialog(hwndDlg);

            PhInitializeLayoutManager(&context->LayoutManager, hwndDlg);
            PhAddLayoutItem(&context->LayoutManager, context->ListViewHandle, NULL, PH_ANCHOR_ALL);
            PhAddLayoutItem(&context->LayoutManager, GetDlgItem(hwndDlg, IDC_REFRESH), NULL, PH_ANCHOR_BOTTOM | PH_ANCHOR_RIGHT);
            PhAddLayoutItem(&context->LayoutManager, GetDlgItem(hwndDlg, IDOK), NULL, PH_ANCHOR_BOTTOM | PH_ANCHOR_RIGHT);

            PhSetListViewStyle(context->ListViewHandle, TRUE, TRUE);
            PhSetControlTheme(context->ListViewHandle, L"explorer");
            PhAddListViewColumn(context->ListViewHandle, 0, 0, 0, LVCFMT_LEFT, 100, L"Type");
            PhAddListViewColumn(context->ListViewHandle, 1, 1, 1, LVCFMT_LEFT, 80, L"Handle");
            PhAddListViewColumn(context->ListViewHandle, 2, 2, 2, LVCFMT_LEFT, 102, L"Object");
            PhAddListViewColumn(context->ListViewHandle, 3, 3, 3, LVCFMT_LEFT, 200, L"Information");

            PhSetExtendedListView(context->ListViewHandle);
            ExtendedListView_SetCompareFunction(context->ListViewHandle, 1, PhpGdiHandleHandleCompareFunction);
            ExtendedListView_SetCompareFunction(context->ListViewHandle, 2, PhpGdiHandleObjectCompareFunction);
            ExtendedListView_AddFallbackColumn(context->ListViewHandle, 0);
            ExtendedListView_AddFallbackColumn(context->ListViewHandle, 1);

            {
                PPH_STRING windowTitle;

                windowTitle = PhGetWindowText(hwndDlg);
                PhMoveReference(&windowTitle, PhFormatString(
                    L"%s: %s (%lu)",
                    PhGetStringOrEmpty(windowTitle),
                    PhGetStringOrEmpty(context->ProcessItem->ProcessName),
                    HandleToUlong(context->ProcessItem->ProcessId)
                    ));

                PhSetWindowText(hwndDlg, PhGetStringOrEmpty(windowTitle));
                PhDereferenceObject(windowTitle);
            }

            PhpRefreshGdiHandles(context);

            PhInitializeWindowTheme(hwndDlg, PhEnableThemeSupport);
        }
        break;
    case WM_DESTROY:
        {
            PhUnregisterDialog(hwndDlg);

            PhDeleteLayoutManager(&context->LayoutManager);

            if (context->List)
            {
                for (ULONG i = 0; i < context->List->Count; i++)
                {
                    PPH_GDI_HANDLE_ITEM gdiHandleItem = context->List->Items[i];

                    if (gdiHandleItem->Information)
                        PhDereferenceObject(gdiHandleItem->Information);

                    PhFree(context->List->Items[i]);
                    context->List->Items[i] = NULL;
                }

                PhDereferenceObject(context->List);
            }

            if (context->ProcessItem)
            {
                PhDereferenceObject(context->ProcessItem);
            }

            PhRemoveWindowContext(hwndDlg, PH_WINDOW_CONTEXT_DEFAULT);
            PhFree(context);
        }
        break;
    case WM_COMMAND:
        {
            switch (GET_WM_COMMAND_ID(wParam, lParam))
            {
            case IDCANCEL:
            case IDOK:
                DestroyWindow(hwndDlg);
                break;
            case IDC_REFRESH:
                PhpRefreshGdiHandles(context);
                break;
            }
        }
        break;
    case WM_SIZE:
        {
            PhLayoutManagerLayout(&context->LayoutManager);
        }
        break;
    case WM_NOTIFY:
        {
            PhHandleListViewNotifyForCopy(lParam, context->ListViewHandle);
        }
        break;
    case WM_CONTEXTMENU:
        {
            if ((HWND)wParam == context->ListViewHandle)
            {
                POINT point;
                PPH_EMENU menu;
                PPH_EMENU item;
                PVOID* listviewItems;
                ULONG numberOfItems;

                point.x = GET_X_LPARAM(lParam);
                point.y = GET_Y_LPARAM(lParam);

                if (point.x == -1 && point.y == -1)
                    PhGetListViewContextMenuPoint((HWND)wParam, &point);

                PhGetSelectedListViewItemParams(context->ListViewHandle, &listviewItems, &numberOfItems);

                if (numberOfItems != 0)
                {
                    menu = PhCreateEMenu();
                    PhInsertEMenuItem(menu, PhCreateEMenuItem(0, IDC_COPY, L"&Copy", NULL, NULL), ULONG_MAX);
                    PhInsertCopyListViewEMenuItem(menu, IDC_COPY, context->ListViewHandle);

                    item = PhShowEMenu(
                        menu,
                        hwndDlg,
                        PH_EMENU_SHOW_SEND_COMMAND | PH_EMENU_SHOW_LEFTRIGHT,
                        PH_ALIGN_LEFT | PH_ALIGN_TOP,
                        point.x,
                        point.y
                        );

                    if (item)
                    {
                        if (!PhHandleCopyListViewEMenuItem(item))
                        {
                            switch (item->Id)
                            {
                            case IDC_COPY:
                                PhCopyListView(context->ListViewHandle);
                                break;
                            }
                        }
                    }

                    PhDestroyEMenu(menu);
                }

                PhFree(listviewItems);
            }
        }
        break;
    case WM_CTLCOLORBTN:
        return HANDLE_WM_CTLCOLORBTN(hwndDlg, wParam, lParam, PhWindowThemeControlColor);
    case WM_CTLCOLORDLG:
        return HANDLE_WM_CTLCOLORDLG(hwndDlg, wParam, lParam, PhWindowThemeControlColor);
    case WM_CTLCOLORSTATIC:
        return HANDLE_WM_CTLCOLORSTATIC(hwndDlg, wParam, lParam, PhWindowThemeControlColor);
    }

    return FALSE;
}
