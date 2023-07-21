/*
 * Copyright (c) 2022 Winsider Seminars & Solutions, Inc.  All rights reserved.
 *
 * This file is part of System Informer.
 *
 * Authors:
 *
 *     jxy-s    2023
 *
 */

#include <peview.h>

typedef struct _PVP_RELOCATIONS_CONTEXT
{
    HWND WindowHandle;
    HWND ListViewHandle;
    PH_LAYOUT_MANAGER LayoutManager;
    PPV_PROPPAGECONTEXT PropSheetContext;
} PVP_RELOCATIONS_CONTEXT, *PPVP_RELOCATIONS_CONTEXT;

VOID PvCreateSections(
    _Out_ PHANDLE ImageSection,
    _Out_ PHANDLE DataSection
    )
{
    NTSTATUS status;
    HANDLE fileHandle;

    *DataSection = NULL;
    *ImageSection = NULL;

    status = PhCreateFileWin32(
        &fileHandle,
        PhGetString(PvFileName),
        FILE_READ_ATTRIBUTES | FILE_READ_DATA | SYNCHRONIZE,
        FILE_ATTRIBUTE_NORMAL,
        FILE_SHARE_READ | FILE_SHARE_WRITE | FILE_SHARE_DELETE,
        FILE_OPEN,
        FILE_NON_DIRECTORY_FILE | FILE_SYNCHRONOUS_IO_NONALERT
        );
    if (!NT_SUCCESS(status))
        return;

    status = NtCreateSection(
        ImageSection,
        SECTION_QUERY | SECTION_MAP_READ,
        NULL,
        NULL,
        PAGE_READONLY,
        WindowsVersion < WINDOWS_8 ? SEC_IMAGE : SEC_IMAGE_NO_EXECUTE,
        fileHandle
        );
    if (!NT_SUCCESS(status))
        *ImageSection = NULL;

    status = NtCreateSection(
        DataSection,
        SECTION_QUERY | SECTION_MAP_READ,
        NULL,
        NULL,
        PAGE_READONLY,
        SEC_COMMIT,
        fileHandle
        );
    if (!NT_SUCCESS(status))
        *DataSection = NULL;

    NtClose(fileHandle);
}

VOID PvAddMappingsEntry(
    _In_ HWND ListViewHandle,
    _In_ PKPH_SECTION_MAP_ENTRY Entry,
    _In_ PWSTR Type
    )
{
    INT lvItemIndex;
    WCHAR value[PH_INT64_STR_LEN_1];

    if (Entry->ViewMapType == VIEW_MAP_TYPE_PROCESS)
    {
        CLIENT_ID clientId;

        clientId.UniqueProcess = Entry->ProcessId;
        clientId.UniqueThread = 0;
        lvItemIndex = PhAddListViewItem(ListViewHandle, MAXINT, PhStdGetClientIdName(&clientId)->Buffer, NULL);
    }
    else if (Entry->ViewMapType == VIEW_MAP_TYPE_SESSION)
    {
        lvItemIndex = PhAddListViewItem(ListViewHandle, MAXINT, L"Session", NULL);
    }
    else if (Entry->ViewMapType == VIEW_MAP_TYPE_SYSTEM_CACHE)
    {
        lvItemIndex = PhAddListViewItem(ListViewHandle, MAXINT, L"System", NULL);
    }
    else
    {
        lvItemIndex = PhAddListViewItem(ListViewHandle, MAXINT, L"Unknown", NULL);
    }

    PhSetListViewSubItem(ListViewHandle, lvItemIndex, 1, Type);

    PhPrintPointer(value, Entry->StartVa);
    PhSetListViewSubItem(ListViewHandle, lvItemIndex, 2, value);

    PhPrintPointer(value, Entry->EndVa);
    PhSetListViewSubItem(ListViewHandle, lvItemIndex, 3, value);
}

VOID PvEnumerateMappingsEntries(
    _In_ HWND ListViewHandle
    )
{
    PKPH_SECTION_MAPPINGS_INFORMATION sectionInfo;
    HANDLE imageSection;
    HANDLE dataSection;

    PvCreateSections(&imageSection, &dataSection);

    ExtendedListView_SetRedraw(ListViewHandle, FALSE);
    ListView_DeleteAllItems(ListViewHandle);

    if (imageSection)
    {
        if (NT_SUCCESS(KphQuerySectionMappingsInfo(imageSection, &sectionInfo)))
        {
            for (ULONG i = 0; i < sectionInfo->NumberOfMappings; i++)
            {
                PvAddMappingsEntry(ListViewHandle, &sectionInfo->Mappings[i], L"Image");
            }

            PhFree(sectionInfo);
        }

        NtClose(imageSection);
    }

    if (dataSection)
    {
        if (NT_SUCCESS(KphQuerySectionMappingsInfo(dataSection, &sectionInfo)))
        {
            for (ULONG i = 0; i < sectionInfo->NumberOfMappings; i++)
            {
                PvAddMappingsEntry(ListViewHandle, &sectionInfo->Mappings[i], L"Data");
            }

            PhFree(sectionInfo);
        }

        NtClose(dataSection);
    }

    //ExtendedListView_SortItems(ListViewHandle);
    ExtendedListView_SetRedraw(ListViewHandle, TRUE);
}

INT_PTR CALLBACK PvpMappingsDlgProc(
    _In_ HWND WindowHandle,
    _In_ UINT WindowMessage,
    _In_ WPARAM wParam,
    _In_ LPARAM lParam
    )
{
    PPVP_RELOCATIONS_CONTEXT context;

    if (WindowMessage == WM_INITDIALOG)
    {
        context = PhAllocateZero(sizeof(PVP_RELOCATIONS_CONTEXT));
        PhSetWindowContext(WindowHandle, PH_WINDOW_CONTEXT_DEFAULT, context);

        if (lParam)
        {
            const LPPROPSHEETPAGE propSheetPage = (LPPROPSHEETPAGE)lParam;
            context->PropSheetContext = (PPV_PROPPAGECONTEXT)propSheetPage->lParam;
        }
    }
    else
    {
        context = PhGetWindowContext(WindowHandle, PH_WINDOW_CONTEXT_DEFAULT);
    }

    if (!context)
        return FALSE;

    switch (WindowMessage)
    {
    case WM_INITDIALOG:
        {
            context->WindowHandle = WindowHandle;
            context->ListViewHandle = GetDlgItem(WindowHandle, IDC_LIST);

            PhSetListViewStyle(context->ListViewHandle, TRUE, TRUE);
            PhSetControlTheme(context->ListViewHandle, L"explorer");
            PhAddListViewColumn(context->ListViewHandle, 0, 0, 0, LVCFMT_LEFT, 200, L"View");
            PhAddListViewColumn(context->ListViewHandle, 1, 1, 1, LVCFMT_LEFT, 60, L"Type");
            PhAddListViewColumn(context->ListViewHandle, 2, 2, 2, LVCFMT_LEFT, 100, L"Start");
            PhAddListViewColumn(context->ListViewHandle, 3, 3, 3, LVCFMT_LEFT, 100, L"End");
            PhSetExtendedListView(context->ListViewHandle);
            PvConfigTreeBorders(context->ListViewHandle);

            PhInitializeLayoutManager(&context->LayoutManager, WindowHandle);
            PhAddLayoutItem(&context->LayoutManager, context->ListViewHandle, NULL, PH_ANCHOR_ALL);

            PvEnumerateMappingsEntries(context->ListViewHandle);

            PhInitializeWindowTheme(WindowHandle, PhEnableThemeSupport);
        }
        break;
    case WM_SHOWWINDOW:
        {
            if (context->PropSheetContext && !context->PropSheetContext->LayoutInitialized)
            {
                PvAddPropPageLayoutItem(WindowHandle, WindowHandle, PH_PROP_PAGE_TAB_CONTROL_PARENT, PH_ANCHOR_ALL);
                PvDoPropPageLayout(WindowHandle);

                context->PropSheetContext->LayoutInitialized = TRUE;
            }
        }
        break;
    case WM_DESTROY:
        {
            PhDeleteLayoutManager(&context->LayoutManager);

            PhRemoveWindowContext(WindowHandle, PH_WINDOW_CONTEXT_DEFAULT);
            PhFree(context);
        }
        break;
    case WM_SIZE:
        {
            PhLayoutManagerLayout(&context->LayoutManager);
        }
        break;
    case WM_NOTIFY:
        {
            PvHandleListViewNotifyForCopy(lParam, context->ListViewHandle);
        }
        break;
    case WM_CONTEXTMENU:
        {
            PvHandleListViewCommandCopy(WindowHandle, lParam, wParam, context->ListViewHandle);
        }
        break;
    case WM_CTLCOLORBTN:
    case WM_CTLCOLORDLG:
    case WM_CTLCOLORSTATIC:
    case WM_CTLCOLORLISTBOX:
        {
            SetBkMode((HDC)wParam, TRANSPARENT);
            SetTextColor((HDC)wParam, RGB(0, 0, 0));
            SetDCBrushColor((HDC)wParam, RGB(255, 255, 255));
            return (INT_PTR)GetStockBrush(DC_BRUSH);
        }
        break;
    }

    return FALSE;
}
