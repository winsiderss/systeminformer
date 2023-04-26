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

#include "usernotes.h"

BOOLEAN IsCollapseServicesOnStartEnabled(
    VOID
    )
{
    static PH_STRINGREF servicesBaseName = PH_STRINGREF_INIT(L"services.exe");
    PDB_OBJECT object;

    object = FindDbObject(FILE_TAG, &servicesBaseName);

    if (object && object->Collapse)
    {
        return TRUE;
    }

    return FALSE;
}

VOID AddOrRemoveCollapseServicesOnStart(
    _In_ BOOLEAN CollapseServicesOnStart
    )
{
    // This is for backwards compat with PhCsCollapseServicesOnStart (dmex)
    // https://github.com/winsiderss/systeminformer/issues/519

    if (CollapseServicesOnStart)
    {
        static PH_STRINGREF servicesBaseName = PH_STRINGREF_INIT(L"services.exe");
        PDB_OBJECT object;

        if (object = FindDbObject(FILE_TAG, &servicesBaseName))
        {
            object->Collapse = TRUE;
        }
        else
        {
            object = CreateDbObject(FILE_TAG, &servicesBaseName, NULL);
            object->Collapse = TRUE;
        }
    }
    else
    {
        static PH_STRINGREF servicesBaseName = PH_STRINGREF_INIT(L"services.exe");
        PDB_OBJECT object;

        object = FindDbObject(FILE_TAG, &servicesBaseName);

        if (object && object->Collapse)
        {
            object->Collapse = FALSE;
            DeleteDbObjectForProcessIfUnused(object);
        }
    }
}

VOID OptionsDeleteDbObject(
    _In_ PDB_OBJECT Object
    )
{
    PhMoveReference(&Object->Comment, PhReferenceEmptyString());
    Object->PriorityClass = 0;
    Object->IoPriorityPlusOne = 0;
    Object->BackColor = ULONG_MAX;
    Object->Collapse = FALSE;
    Object->AffinityMask = 0;
    Object->PagePriorityPlusOne = 0;
    Object->Boost = FALSE;
    DeleteDbObjectForProcessIfUnused(Object);
}

typedef struct _DB_ENUM_CONTEXT
{
    ULONG Count;
    HWND ListViewHandle;
} DB_ENUM_CONTEXT, *PDB_ENUM_CONTEXT;

BOOLEAN OptionsEnumDbCallback(
    _In_ PDB_OBJECT Object,
    _In_ PDB_ENUM_CONTEXT Context
    )
{
    INT lvItemIndex;
    WCHAR value[PH_INT64_STR_LEN_1];

    PhPrintUInt32(value, ++Context->Count);
    lvItemIndex = PhAddListViewItem(Context->ListViewHandle, MAXINT, value, Object);

    if (Object->Tag == FILE_TAG)
    {
        PhSetListViewSubItem(Context->ListViewHandle, lvItemIndex, 1, L"File");
    }
    else if (Object->Tag == SERVICE_TAG)
    {
        PhSetListViewSubItem(Context->ListViewHandle, lvItemIndex, 1, L"Service");
    }
    else if (Object->Tag == COMMAND_LINE_TAG)
    {
        PhSetListViewSubItem(Context->ListViewHandle, lvItemIndex, 1, L"Commandline");
    }

    PhSetListViewSubItem(Context->ListViewHandle, lvItemIndex, 2, PhGetString(Object->Name));
    PhSetListViewSubItem(Context->ListViewHandle, lvItemIndex, 3, PhGetString(Object->Comment));

    if (Object->PriorityClass != PROCESS_PRIORITY_CLASS_UNKNOWN)
    {
        PhPrintUInt32(value, Object->PriorityClass);
        PhSetListViewSubItem(Context->ListViewHandle, lvItemIndex, 4, value);
    }

    if (Object->IoPriorityPlusOne - 1 != ULONG_MAX)
    {
        PhPrintUInt32(value, Object->IoPriorityPlusOne - 1);
        PhSetListViewSubItem(Context->ListViewHandle, lvItemIndex, 5, value);
    }

    if (Object->BackColor != ULONG_MAX)
    {
        PhPrintUInt32(value, Object->BackColor);
        PhSetListViewSubItem(Context->ListViewHandle, lvItemIndex, 6, value);
    }

    if (Object->Collapse)
    {
        PhSetListViewSubItem(Context->ListViewHandle, lvItemIndex, 7, L"True");
    }

    if (Object->AffinityMask != 0)
    {
        PhPrintPointer(value, (PVOID)Object->AffinityMask);
        PhSetListViewSubItem(Context->ListViewHandle, lvItemIndex, 8, value);
    }

    if (Object->PagePriorityPlusOne - 1 != ULONG_MAX)
    {
        PhPrintUInt32(value, Object->PagePriorityPlusOne - 1);
        PhSetListViewSubItem(Context->ListViewHandle, lvItemIndex, 9, value);
    }

    return TRUE;
}

INT_PTR CALLBACK OptionsDlgProc(
    _In_ HWND hwndDlg,
    _In_ UINT uMsg,
    _In_ WPARAM wParam,
    _In_ LPARAM lParam
    )
{
    static PH_LAYOUT_MANAGER LayoutManager;

    switch (uMsg)
    {
    case WM_INITDIALOG:
        {
            HWND listview = GetDlgItem(hwndDlg, IDC_DBLIST);

            PhSetListViewStyle(listview, FALSE, TRUE);
            PhSetControlTheme(listview, L"explorer");
            PhAddListViewColumn(listview, 0, 0, 0, LVCFMT_LEFT, 40, L"#");
            PhAddListViewColumn(listview, 1, 1, 1, LVCFMT_LEFT, 100, L"Type");
            PhAddListViewColumn(listview, 2, 2, 2, LVCFMT_LEFT, 100, L"Name");
            PhAddListViewColumn(listview, 3, 3, 3, LVCFMT_LEFT, 100, L"Comment");
            PhAddListViewColumn(listview, 4, 4, 4, LVCFMT_LEFT, 80, L"Priority");
            PhAddListViewColumn(listview, 5, 5, 5, LVCFMT_LEFT, 80, L"IO priority");
            PhAddListViewColumn(listview, 6, 6, 6, LVCFMT_LEFT, 80, L"BackColor");
            PhAddListViewColumn(listview, 7, 7, 7, LVCFMT_LEFT, 80, L"Collapse");
            PhAddListViewColumn(listview, 8, 8, 8, LVCFMT_LEFT, 80, L"Affinity");
            PhAddListViewColumn(listview, 8, 8, 8, LVCFMT_LEFT, 80, L"Page priority");
            PhSetExtendedListView(listview);
            PhLoadListViewColumnsFromSetting(SETTING_NAME_OPTIONS_DB_COLUMNS, listview);

            PhInitializeLayoutManager(&LayoutManager, hwndDlg);
            PhAddLayoutItem(&LayoutManager, listview, NULL, PH_ANCHOR_ALL);

            Button_SetCheck(GetDlgItem(hwndDlg, IDC_COLLAPSE_SERVICES_CHECK), IsCollapseServicesOnStartEnabled());

            {
                DB_ENUM_CONTEXT enumContext = { 0 };

                enumContext.Count = 0;
                enumContext.ListViewHandle = listview;

                EnumDb(OptionsEnumDbCallback, &enumContext);
            }
        }
        break;
    case WM_DESTROY:
        {
            PhSaveListViewColumnsToSetting(SETTING_NAME_OPTIONS_DB_COLUMNS, GetDlgItem(hwndDlg, IDC_DBLIST));

            PhDeleteLayoutManager(&LayoutManager);
        }
        break;
    case WM_SIZE:
        {
            PhLayoutManagerLayout(&LayoutManager);
        }
        break;
    case WM_COMMAND:
        {
            switch (GET_WM_COMMAND_ID(wParam, lParam))
            {
            case IDC_COLLAPSE_SERVICES_CHECK:
                {
                    AddOrRemoveCollapseServicesOnStart(
                        Button_GetCheck(GET_WM_COMMAND_HWND(wParam, lParam)) == BST_CHECKED);

                    SaveDb();

                    // uncomment for realtime toggle
                    //LoadCollapseServicesOnStart();
                    //PhExpandAllProcessNodes(TRUE);
                    //if (ToolStatusInterface)
                    //    PhInvokeCallback(ToolStatusInterface->SearchChangedEvent, PH_AUTO(PhReferenceEmptyString()));
                }
                break;
            }
        }
        break;
    case WM_CONTEXTMENU:
        {
            HWND listviewHandle = GetDlgItem(hwndDlg, IDC_DBLIST);

            if ((HWND)wParam == listviewHandle)
            {
                POINT point;
                PPH_EMENU menu;
                PPH_EMENU item;
                PVOID* listviewItems;
                ULONG numberOfItems;

                point.x = GET_X_LPARAM(lParam);
                point.y = GET_Y_LPARAM(lParam);

                if (point.x == -1 && point.y == -1)
                    PhGetListViewContextMenuPoint(listviewHandle, &point);

                PhGetSelectedListViewItemParams(listviewHandle, &listviewItems, &numberOfItems);

                if (numberOfItems != 0)
                {
                    menu = PhCreateEMenu();
                    PhInsertEMenuItem(menu, PhCreateEMenuItem(0, 1, L"&Delete", NULL, NULL), ULONG_MAX);
                    PhInsertEMenuItem(menu, PhCreateEMenuSeparator(), ULONG_MAX);
                    PhInsertEMenuItem(menu, PhCreateEMenuItem(0, 2, L"&Copy", NULL, NULL), ULONG_MAX);
                    PhInsertCopyListViewEMenuItem(menu, 2, listviewHandle);

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
                            case 1:
                                {
                                    SendMessage(listviewHandle, WM_SETREDRAW, FALSE, 0);

                                    for (ULONG i = 0; i < numberOfItems; i++)
                                    {
                                        OptionsDeleteDbObject(listviewItems[i]);
                                    }

                                    for (LONG i = numberOfItems - 1; i >= 0; i--)
                                    {
                                        PhRemoveListViewItem(listviewHandle, i);
                                    }

                                    SendMessage(listviewHandle, WM_SETREDRAW, TRUE, 0);

                                    SaveDb();
                                }
                                break;
                            case 2:
                                PhCopyListView(listviewHandle);
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
