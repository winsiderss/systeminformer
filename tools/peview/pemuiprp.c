/*
 * Copyright (c) 2022 Winsider Seminars & Solutions, Inc.  All rights reserved.
 *
 * This file is part of System Informer.
 *
 * Authors:
 *
 *     dmex    2023
 *
 */

#include <peview.h>

typedef struct _PVP_PE_MUI_CONTEXT
{
    HWND WindowHandle;
    HWND ListViewHandle;
    PH_LAYOUT_MANAGER LayoutManager;
    PPV_PROPPAGECONTEXT PropSheetContext;
} PVP_PE_MUI_CONTEXT, *PPVP_PE_MUI_CONTEXT;

#define MUI_TYPE TEXT("MUI")
#define MUI_NAME MAKEINTRESOURCE(1)
#define MUI_SIGNATURE 0xFECDFECD
#define MUI_VERSION 0x10000
#define MUI_FALLBACK_LOCATION_INTERNAL 1
#define MUI_FALLBACK_LOCATION_EXTERNAL 2
#define MUI_FILETYPE_MAIN 17
#define MUI_FILETYPE_MUI 16

// rev from MUIRCT.EXE!DumpRCManifest (Windows SDK) (dmex)
typedef struct _MUI_MANIFEST
{
    ULONG Signature;
    ULONG Length;
    ULONG RCConfigVersion;
    ULONG PathType;
    ULONG FileType;
    ULONG SystemAttributes;
    ULONG FallbackLocation;
    UCHAR ServiceChecksum[16];
    UCHAR Checksum[16];
    UCHAR MUIFilePath[24];
    ULONG MainNameTypesOffset;
    ULONG MainNameTypesLength;
    ULONG MainIDTypesOffset;
    ULONG MainIDTypesLength;
    ULONG MuiNameTypesOffset;
    ULONG MuiNameTypesLength;
    ULONG MuiIDTypesOffset;
    ULONG MuiIDTypesLength;
    ULONG LanguageOffset;
    ULONG LanguageLength;
    ULONG UltimateFallbackLanguageOffset;
    ULONG UltimateFallbackLanguageLength;
} MUI_MANIFEST, *PMUI_MANIFEST;

VOID PvAddMuiInfoItem(
    _In_ HWND ListViewHandle,
    _Inout_ PULONG Count,
    _In_ INT GroupId,
    _In_ PWSTR Name,
    _In_ PWSTR Value
    )
{
    INT lvItemIndex;
    WCHAR number[PH_PTR_STR_LEN_1];

    PhPrintUInt32(number, ++(*Count));
    lvItemIndex = PhAddListViewGroupItem(ListViewHandle, GroupId, MAXINT, number, NULL);
    PhSetListViewSubItem(ListViewHandle, lvItemIndex, 1, Name);
    PhSetListViewSubItem(ListViewHandle, lvItemIndex, 2, Value);
}

VOID PvAddMuiResourceInfo(
    _In_ HWND ListViewHandle,
    _In_ PMUI_MANIFEST Manifest
    )
{
    ULONG count = 0;

    if (Manifest->Signature != MUI_SIGNATURE)
        return;

    PvAddMuiInfoItem(ListViewHandle, &count, 0, L"Signature", PhaFormatString(L"0x%lx", Manifest->Signature)->Buffer);
    PvAddMuiInfoItem(ListViewHandle, &count, 0, L"Length", PhaFormatString(L"%lu (%s)", Manifest->Length, PhaFormatSize(Manifest->Length, ULONG_MAX)->Buffer)->Buffer);
    PvAddMuiInfoItem(ListViewHandle, &count, 0, L"RC config version", PhaFormatString(L"0x%lx", Manifest->RCConfigVersion)->Buffer);
    PvAddMuiInfoItem(ListViewHandle, &count, 0, L"PathType", PhaFormatString(L"%lu", Manifest->PathType)->Buffer);
    PvAddMuiInfoItem(ListViewHandle, &count, 0, L"FileType", PhaFormatString(L"%lu", Manifest->FileType)->Buffer);
    PvAddMuiInfoItem(ListViewHandle, &count, 0, L"SystemAttributes", PhaFormatString(L"%lu", Manifest->SystemAttributes)->Buffer);
    PvAddMuiInfoItem(ListViewHandle, &count, 0, L"UltimateFallback location", PhaFormatString(L"%lu", Manifest->FallbackLocation)->Buffer);
    PvAddMuiInfoItem(ListViewHandle, &count, 0, L"MainNameTypesOffset", PhaFormatString(L"%lu (0x%lx)", Manifest->MainNameTypesOffset, Manifest->MainNameTypesOffset)->Buffer);
    PvAddMuiInfoItem(ListViewHandle, &count, 0, L"MainNameTypesLength", PhaFormatString(L"%lu (%s)", Manifest->MainNameTypesLength, PhaFormatSize(Manifest->MainNameTypesLength, ULONG_MAX)->Buffer)->Buffer);
    PvAddMuiInfoItem(ListViewHandle, &count, 0, L"MainIDTypesOffset", PhaFormatString(L"%lu (0x%lx)", Manifest->MainIDTypesOffset, Manifest->MainIDTypesOffset)->Buffer);
    PvAddMuiInfoItem(ListViewHandle, &count, 0, L"MainIDTypesLength", PhaFormatString(L"%lu (%s)", Manifest->MainIDTypesLength, PhaFormatSize(Manifest->MainIDTypesLength, ULONG_MAX)->Buffer)->Buffer);
    PvAddMuiInfoItem(ListViewHandle, &count, 0, L"MuiNameTypesOffset", PhaFormatString(L"%lu (0x%lx)", Manifest->MuiNameTypesOffset, Manifest->MuiNameTypesOffset)->Buffer);
    PvAddMuiInfoItem(ListViewHandle, &count, 0, L"MuiNameTypesLength", PhaFormatString(L"%lu (%s)", Manifest->MuiNameTypesLength, PhaFormatSize(Manifest->MuiNameTypesLength, ULONG_MAX)->Buffer)->Buffer);
    PvAddMuiInfoItem(ListViewHandle, &count, 0, L"MuiIDTypesOffset", PhaFormatString(L"%lu (0x%lx)", Manifest->MuiIDTypesOffset, Manifest->MuiIDTypesOffset)->Buffer);
    PvAddMuiInfoItem(ListViewHandle, &count, 0, L"MuiIDTypesLength", PhaFormatString(L"%lu (%s)", Manifest->MuiIDTypesLength, PhaFormatSize(Manifest->MuiIDTypesLength, ULONG_MAX)->Buffer)->Buffer);
    PvAddMuiInfoItem(ListViewHandle, &count, 0, L"LanguageOffset", PhaFormatString(L"%lu (0x%lx)", Manifest->LanguageOffset, Manifest->LanguageOffset)->Buffer);
    PvAddMuiInfoItem(ListViewHandle, &count, 0, L"LanguageLength", PhaFormatString(L"%lu (0x%lx)", Manifest->LanguageLength, Manifest->LanguageLength)->Buffer);
    PvAddMuiInfoItem(ListViewHandle, &count, 0, L"FallbackLanguageOffset", PhaFormatString(L"%lu (0x%lx)", Manifest->UltimateFallbackLanguageOffset, Manifest->UltimateFallbackLanguageOffset)->Buffer);
    PvAddMuiInfoItem(ListViewHandle, &count, 0, L"FallbackLanguageLength", PhaFormatString(L"%lu (%s)", Manifest->UltimateFallbackLanguageLength, PhaFormatSize(Manifest->UltimateFallbackLanguageLength, ULONG_MAX)->Buffer)->Buffer);
    PvAddMuiInfoItem(ListViewHandle, &count, 0, L"UltimateFallbackLanguage", Manifest->UltimateFallbackLanguageOffset ? PTR_ADD_OFFSET(Manifest, Manifest->UltimateFallbackLanguageOffset) : L"");
    PvAddMuiInfoItem(ListViewHandle, &count, 1, L"Service Checksum", PH_AUTO_T(PH_STRING, PhBufferToHexString(Manifest->ServiceChecksum, sizeof(Manifest->ServiceChecksum)))->Buffer);
    //PvAddMuiInfoItem(ListViewHandle, &count, 1, L"MUIFilePath", PH_AUTO_T(PH_STRING, PhBufferToHexString(Manifest->MUIFilePath, sizeof(Block->MUIFilePath)))->Buffer);
    PvAddMuiInfoItem(ListViewHandle, &count, 1, L"Checksum", PH_AUTO_T(PH_STRING, PhBufferToHexString(Manifest->Checksum, sizeof(Manifest->Checksum)))->Buffer);

    {
        const PVOID nameTypesList = PTR_ADD_OFFSET(Manifest, Manifest->MainNameTypesOffset);
        const PULONG identifierTypesList = PTR_ADD_OFFSET(Manifest, Manifest->MainIDTypesOffset);
        const ULONG identifierTypesCount = Manifest->MainIDTypesLength / sizeof(ULONG);
        PH_STRING_BUILDER stringBuilder;

        PhInitializeStringBuilder(&stringBuilder, 10);
        for (PWSTR name = nameTypesList; *name; name += PhCountStringZ(name) + 1)
            PhAppendFormatStringBuilder(&stringBuilder, L"%s, ", name);
        if (PhEndsWithString2(stringBuilder.String, L", ", FALSE))
            PhRemoveEndStringBuilder(&stringBuilder, 2);
        PvAddMuiInfoItem(ListViewHandle, &count, 2, L"Names", PhGetString(PhFinalStringBuilderString(&stringBuilder)));
        PhDeleteStringBuilder(&stringBuilder);

        PhInitializeStringBuilder(&stringBuilder, 10);
        for (ULONG i = 0; i < identifierTypesCount; i++)
            PhAppendFormatStringBuilder(&stringBuilder, L"%s, ", PhaFormatUInt64(identifierTypesList[i], TRUE)->Buffer);
        if (PhEndsWithString2(stringBuilder.String, L", ", FALSE))
            PhRemoveEndStringBuilder(&stringBuilder, 2);
        PvAddMuiInfoItem(ListViewHandle, &count, 3, L"Types", PhGetString(PhFinalStringBuilderString(&stringBuilder)));
        PhDeleteStringBuilder(&stringBuilder);
    }

    {
        const PVOID nameTypesList = PTR_ADD_OFFSET(Manifest, Manifest->MuiNameTypesOffset);
        const PULONG identifierTypesList = PTR_ADD_OFFSET(Manifest, Manifest->MuiIDTypesOffset);
        const ULONG identifierTypesCount = Manifest->MuiIDTypesLength / sizeof(ULONG);
        PH_STRING_BUILDER stringBuilder;

        PhInitializeStringBuilder(&stringBuilder, 10);
        for (PWSTR name = nameTypesList; *name; name += PhCountStringZ(name) + 1)
            PhAppendFormatStringBuilder(&stringBuilder, L"%s, ", name);
        if (PhEndsWithString2(stringBuilder.String, L", ", FALSE))
            PhRemoveEndStringBuilder(&stringBuilder, 2);
        PvAddMuiInfoItem(ListViewHandle, &count, 4, L"Names", PhGetString(PhFinalStringBuilderString(&stringBuilder)));
        PhDeleteStringBuilder(&stringBuilder);

        PhInitializeStringBuilder(&stringBuilder, 10);
        for (ULONG i = 0; i < identifierTypesCount; i++)
            PhAppendFormatStringBuilder(&stringBuilder, L"%s, ", PhaFormatUInt64(identifierTypesList[i], TRUE)->Buffer);
        if (PhEndsWithString2(stringBuilder.String, L", ", FALSE))
            PhRemoveEndStringBuilder(&stringBuilder, 2);
        PvAddMuiInfoItem(ListViewHandle, &count, 5, L"Types", PhGetString(PhFinalStringBuilderString(&stringBuilder)));
        PhDeleteStringBuilder(&stringBuilder);
    }
}

VOID PvPeGetMuiInfo(
    _In_ HWND ListViewHandle
    )
{
    ULONG resourceLength;
    PVOID resourceBuffer;

    ExtendedListView_SetRedraw(ListViewHandle, FALSE);
    ListView_DeleteAllItems(ListViewHandle);
    ListView_EnableGroupView(ListViewHandle, TRUE);
    PhAddListViewGroup(ListViewHandle, 0, L"MUI");
    PhAddListViewGroup(ListViewHandle, 1, L"Checksums");
    PhAddListViewGroup(ListViewHandle, 2, L"MainNameTypes");
    PhAddListViewGroup(ListViewHandle, 3, L"MainTypeIDs");
    PhAddListViewGroup(ListViewHandle, 4, L"TypeNames");
    PhAddListViewGroup(ListViewHandle, 5, L"TypeIDs");

    if (PhLoadResource(
        PvMappedImage.ViewBase,
        MUI_NAME,
        MUI_TYPE,
        &resourceLength,
        &resourceBuffer
        ))
    {
        PvAddMuiResourceInfo(ListViewHandle, resourceBuffer);
    }
    else
    {
        PH_MAPPED_IMAGE_RESOURCES resources;
        PH_IMAGE_RESOURCE_ENTRY entry;
        ULONG i;

        if (NT_SUCCESS(PhGetMappedImageResources(&resources, &PvMappedImage)))
        {
            for (i = 0; i < resources.NumberOfEntries; i++)
            {
                entry = resources.ResourceEntries[i];

                if (IS_INTRESOURCE(entry.Name) && (PtrToUshort((const void*)entry.Name) == PtrToUshort(MUI_NAME)))
                {
                    if (!IS_INTRESOURCE(entry.Type))
                    {
                        const PIMAGE_RESOURCE_DIR_STRING_U resourceString = (PIMAGE_RESOURCE_DIR_STRING_U)entry.Type;
                        PH_STRINGREF string;

                        string.Buffer = resourceString->NameString;
                        string.Length = resourceString->Length * sizeof(WCHAR);

                        if (PhEqualStringRef2(&string, MUI_TYPE, TRUE))
                        {
                            PVOID resourceData = PhMappedImageRvaToVa(&PvMappedImage, entry.Offset, NULL);

                            PvAddMuiResourceInfo(ListViewHandle, resourceData);
                        }
                    }
                }
            }

            PhFree(resources.ResourceEntries);
        }
    }

    //ExtendedListView_SortItems(context->ListViewHandle);
    ExtendedListView_SetRedraw(ListViewHandle, TRUE);
}

INT_PTR CALLBACK PvpPeMuiResourceDlgProc(
    _In_ HWND hwndDlg,
    _In_ UINT uMsg,
    _In_ WPARAM wParam,
    _In_ LPARAM lParam
    )
{
    PPVP_PE_MUI_CONTEXT context;

    if (uMsg == WM_INITDIALOG)
    {
        context = PhAllocateZero(sizeof(PVP_PE_MUI_CONTEXT));
        PhSetWindowContext(hwndDlg, PH_WINDOW_CONTEXT_DEFAULT, context);

        if (lParam)
        {
            const LPPROPSHEETPAGE propSheetPage = (LPPROPSHEETPAGE)lParam;
            context->PropSheetContext = (PPV_PROPPAGECONTEXT)propSheetPage->lParam;
        }
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
            context->WindowHandle = hwndDlg;
            context->ListViewHandle = GetDlgItem(hwndDlg, IDC_LIST);

            PhSetListViewStyle(context->ListViewHandle, TRUE, TRUE);
            PhSetControlTheme(context->ListViewHandle, L"explorer");
            PhAddListViewColumn(context->ListViewHandle, 0, 0, 0, LVCFMT_LEFT, 40, L"#");
            PhAddListViewColumn(context->ListViewHandle, 1, 1, 1, LVCFMT_LEFT, 150, L"Name");
            PhAddListViewColumn(context->ListViewHandle, 2, 2, 2, LVCFMT_LEFT, 250, L"Value");
            PhSetExtendedListView(context->ListViewHandle);
            PhLoadListViewColumnsFromSetting(L"ImageMuiListViewColumns", context->ListViewHandle);
            PvConfigTreeBorders(context->ListViewHandle);
            //PvSetListViewImageList(context->WindowHandle, context->ListViewHandle);

            PhInitializeLayoutManager(&context->LayoutManager, hwndDlg);
            PhAddLayoutItem(&context->LayoutManager, context->ListViewHandle, NULL, PH_ANCHOR_ALL);

            PvPeGetMuiInfo(context->ListViewHandle);

            PhInitializeWindowTheme(hwndDlg, PhEnableThemeSupport);
        }
        break;
    case WM_DESTROY:
        {
            PhSaveListViewColumnsToSetting(L"ImageMuiListViewColumns", context->ListViewHandle);
            PhDeleteLayoutManager(&context->LayoutManager);
        }
        break;
    case WM_NCDESTROY:
        {
            PhRemoveWindowContext(hwndDlg, PH_WINDOW_CONTEXT_DEFAULT);
            PhFree(context);
        }
        break;
    case WM_DPICHANGED:
        {
            //PvSetListViewImageList(context->WindowHandle, context->ListViewHandle);
        }
        break;
    case WM_SHOWWINDOW:
        {
            if (context->PropSheetContext && !context->PropSheetContext->LayoutInitialized)
            {
                PvAddPropPageLayoutItem(hwndDlg, hwndDlg, PH_PROP_PAGE_TAB_CONTROL_PARENT, PH_ANCHOR_ALL);
                PvDoPropPageLayout(hwndDlg);

                context->PropSheetContext->LayoutInitialized = TRUE;
            }
        }
        break;
    case WM_SIZE:
        {
            PhLayoutManagerLayout(&context->LayoutManager);
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
                    PvGetListViewContextMenuPoint(context->ListViewHandle, &point);

                PhGetSelectedListViewItemParams(context->ListViewHandle, &listviewItems, &numberOfItems);

                if (numberOfItems != 0)
                {
                    menu = PhCreateEMenu();
                    PhInsertEMenuItem(menu, PhCreateEMenuItem(0, USHRT_MAX, L"&Copy", NULL, NULL), ULONG_MAX);
                    PvInsertCopyListViewEMenuItem(menu, USHRT_MAX, context->ListViewHandle);

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
                        if (!PvHandleCopyListViewEMenuItem(item))
                        {
                            switch (item->Id)
                            {
                            case USHRT_MAX:
                                PvCopyListView(context->ListViewHandle);
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
