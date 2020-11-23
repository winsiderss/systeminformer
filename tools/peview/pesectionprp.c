/*
 * Process Hacker -
 *   PE viewer
 *
 * Copyright (C) 2019-2020 dmex
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

#include <peview.h>

typedef struct _PV_PE_SECTION_CONTEXT
{
    HWND WindowHandle;
    HWND ListViewHandle;
    HIMAGELIST ListViewImageList;
} PV_PE_SECTION_CONTEXT, *PPV_PE_SECTION_CONTEXT;

COLORREF NTAPI PvPeCharacteristicsColorFunction(
    _In_ INT Index,
    _In_ PVOID Param,
    _In_opt_ PVOID Context
    )
{
    PIMAGE_SECTION_HEADER imageSection = Param;

    if (imageSection->Characteristics & IMAGE_SCN_MEM_WRITE)
        return RGB(0xf0, 0xa0, 0xa0);
    if (imageSection->Characteristics & IMAGE_SCN_MEM_EXECUTE)
        return RGB(0xff, 0x93, 0x14);
    if (imageSection->Characteristics & IMAGE_SCN_CNT_CODE)
        return RGB(0xe0, 0xf0, 0xe0);
    if (imageSection->Characteristics & IMAGE_SCN_MEM_READ)
        return RGB(0xc0, 0xf0, 0xc0);

    return RGB(0xff, 0xff, 0xff);
}

INT NTAPI PvPeVirtualAddressCompareFunction(
    _In_ PVOID Item1,
    _In_ PVOID Item2,
    _In_opt_ PVOID Context
    )
{
    PIMAGE_SECTION_HEADER entry1 = Item1;
    PIMAGE_SECTION_HEADER entry2 = Item2;

    return uintcmp(entry1->VirtualAddress, entry2->VirtualAddress);
}

INT NTAPI PvPeSizeOfRawDataCompareFunction(
    _In_ PVOID Item1,
    _In_ PVOID Item2,
    _In_opt_ PVOID Context
    )
{
    PIMAGE_SECTION_HEADER entry1 = Item1;
    PIMAGE_SECTION_HEADER entry2 = Item2;

    return uintcmp(entry1->SizeOfRawData, entry2->SizeOfRawData);
}

INT NTAPI PvPeCharacteristicsCompareFunction(
    _In_ PVOID Item1,
    _In_ PVOID Item2,
    _In_opt_ PVOID Context
    )
{
    PIMAGE_SECTION_HEADER entry1 = Item1;
    PIMAGE_SECTION_HEADER entry2 = Item2;

    return uintcmp(entry1->Characteristics, entry2->Characteristics);
}

PPH_STRING PvGetSectionCharacteristics(
    _In_ ULONG Characteristics
    )
{
    PH_STRING_BUILDER stringBuilder;
    WCHAR pointer[PH_PTR_STR_LEN_1];

    if (Characteristics == 0)
        return PhCreateString(L"Associative (0x0)");

    PhInitializeStringBuilder(&stringBuilder, 10);

    if (Characteristics & IMAGE_SCN_TYPE_NO_PAD)
        PhAppendStringBuilder2(&stringBuilder, L"No Padding, ");
    if (Characteristics & IMAGE_SCN_CNT_CODE)
        PhAppendStringBuilder2(&stringBuilder, L"Code, ");
    if (Characteristics & IMAGE_SCN_CNT_INITIALIZED_DATA)
        PhAppendStringBuilder2(&stringBuilder, L"Initialized data, ");
    if (Characteristics & IMAGE_SCN_CNT_UNINITIALIZED_DATA)
        PhAppendStringBuilder2(&stringBuilder, L"Uninitialized data, ");
    if (Characteristics & IMAGE_SCN_LNK_INFO)
        PhAppendStringBuilder2(&stringBuilder, L"Comments, ");
    if (Characteristics & IMAGE_SCN_LNK_REMOVE)
        PhAppendStringBuilder2(&stringBuilder, L"Excluded, ");
    if (Characteristics & IMAGE_SCN_LNK_COMDAT)
        PhAppendStringBuilder2(&stringBuilder, L"COMDAT, ");
    if (Characteristics & IMAGE_SCN_NO_DEFER_SPEC_EXC)
        PhAppendStringBuilder2(&stringBuilder, L"Speculative exceptions, ");
    if (Characteristics & IMAGE_SCN_GPREL)
        PhAppendStringBuilder2(&stringBuilder, L"GP relative, ");
    if (Characteristics & IMAGE_SCN_MEM_PURGEABLE)
        PhAppendStringBuilder2(&stringBuilder, L"Purgeable, ");
    if (Characteristics & IMAGE_SCN_MEM_LOCKED)
        PhAppendStringBuilder2(&stringBuilder, L"Locked, ");
    if (Characteristics & IMAGE_SCN_MEM_PRELOAD)
        PhAppendStringBuilder2(&stringBuilder, L"Preload, ");
    if (Characteristics & IMAGE_SCN_LNK_NRELOC_OVFL)
        PhAppendStringBuilder2(&stringBuilder, L"Extended relocations, ");
    if (Characteristics & IMAGE_SCN_MEM_DISCARDABLE)
        PhAppendStringBuilder2(&stringBuilder, L"Discardable, ");
    if (Characteristics & IMAGE_SCN_MEM_NOT_CACHED)
        PhAppendStringBuilder2(&stringBuilder, L"Not cachable, ");
    if (Characteristics & IMAGE_SCN_MEM_NOT_PAGED)
        PhAppendStringBuilder2(&stringBuilder, L"Not pageable, ");
    if (Characteristics & IMAGE_SCN_MEM_SHARED)
        PhAppendStringBuilder2(&stringBuilder, L"Shareable, ");
    if (Characteristics & IMAGE_SCN_MEM_EXECUTE)
        PhAppendStringBuilder2(&stringBuilder, L"Executable, ");
    if (Characteristics & IMAGE_SCN_MEM_READ)
        PhAppendStringBuilder2(&stringBuilder, L"Readable, ");
    if (Characteristics & IMAGE_SCN_MEM_WRITE)
        PhAppendStringBuilder2(&stringBuilder, L"Writeable, ");

    if (PhEndsWithString2(stringBuilder.String, L", ", FALSE))
        PhRemoveEndStringBuilder(&stringBuilder, 2);

    PhPrintPointer(pointer, UlongToPtr(Characteristics));
    PhAppendFormatStringBuilder(&stringBuilder, L" (%s)", pointer);

    return PhFinalStringBuilderString(&stringBuilder);
}

VOID PvSetPeImageSections(
    _In_ HWND ListViewHandle
    )
{
    ExtendedListView_SetRedraw(ListViewHandle, FALSE);
    ListView_DeleteAllItems(ListViewHandle);

    for (ULONG i = 0; i < PvMappedImage.NumberOfSections; i++)
    {
        INT lvItemIndex;
        WCHAR sectionName[IMAGE_SIZEOF_SHORT_NAME + 1];
        WCHAR value[PH_INT64_STR_LEN_1];

        if (PhGetMappedImageSectionName(
            &PvMappedImage.Sections[i],
            sectionName,
            RTL_NUMBER_OF(sectionName),
            NULL
            ))
        {
            PhPrintUInt32(value, i + 1);
            lvItemIndex = PhAddListViewItem(ListViewHandle, MAXINT, value, &PvMappedImage.Sections[i]);
            PhSetListViewSubItem(ListViewHandle, lvItemIndex, 1, sectionName);
            PhPrintPointer(value, UlongToPtr(PvMappedImage.Sections[i].VirtualAddress));
            PhSetListViewSubItem(ListViewHandle, lvItemIndex, 2, value);
            PhPrintPointer(value, PTR_ADD_OFFSET(PvMappedImage.Sections[i].VirtualAddress, PvMappedImage.Sections[i].SizeOfRawData));
            PhSetListViewSubItem(ListViewHandle, lvItemIndex, 3, value);
            PhSetListViewSubItem(ListViewHandle, lvItemIndex, 4, PhaFormatSize(PvMappedImage.Sections[i].SizeOfRawData, ULONG_MAX)->Buffer);
            PhSetListViewSubItem(ListViewHandle, lvItemIndex, 5, PH_AUTO_T(PH_STRING, PvGetSectionCharacteristics(PvMappedImage.Sections[i].Characteristics))->Buffer);

            if (PvMappedImage.Sections[i].VirtualAddress && PvMappedImage.Sections[i].SizeOfRawData)
            {
                __try
                {
                    PVOID imageSectionData;
                    PH_HASH_CONTEXT hashContext;
                    PPH_STRING hashString;
                    UCHAR hash[32];

                    if (imageSectionData = PhMappedImageRvaToVa(&PvMappedImage, PvMappedImage.Sections[i].VirtualAddress, NULL))
                    {
                        PhInitializeHash(&hashContext, Md5HashAlgorithm); // PhGetIntegerSetting(L"HashAlgorithm")
                        PhUpdateHash(&hashContext, imageSectionData, PvMappedImage.Sections[i].SizeOfRawData);

                        if (PhFinalHash(&hashContext, hash, 16, NULL))
                        {
                            hashString = PhBufferToHexString(hash, 16);
                            PhSetListViewSubItem(ListViewHandle, lvItemIndex, 6, hashString->Buffer);
                            PhDereferenceObject(hashString);
                        }
                    }
                }
                __except (EXCEPTION_EXECUTE_HANDLER)
                {
                    PPH_STRING message;

                    //message = PH_AUTO(PhGetNtMessage(GetExceptionCode()));
                    message = PH_AUTO(PhGetWin32Message(RtlNtStatusToDosError(GetExceptionCode()))); // WIN32_FROM_NTSTATUS

                    PhSetListViewSubItem(ListViewHandle, lvItemIndex, 6, PhGetStringOrEmpty(message));
                }
            }
        }
    }

    ExtendedListView_SortItems(ListViewHandle);
    ExtendedListView_SetRedraw(ListViewHandle, TRUE);
}

INT_PTR CALLBACK PvPeSectionsDlgProc(
    _In_ HWND hwndDlg,
    _In_ UINT uMsg,
    _In_ WPARAM wParam,
    _In_ LPARAM lParam
    )
{
    LPPROPSHEETPAGE propSheetPage;
    PPV_PROPPAGECONTEXT propPageContext;
    PPV_PE_SECTION_CONTEXT context = NULL;

    if (!PvPropPageDlgProcHeader(hwndDlg, uMsg, lParam, &propSheetPage, &propPageContext))
        return FALSE;

    if (uMsg == WM_INITDIALOG)
    {
        context = propPageContext->Context = PhAllocate(sizeof(PV_PE_SECTION_CONTEXT));
        memset(context, 0, sizeof(PV_PE_SECTION_CONTEXT));
    }
    else
    {
        context = propPageContext->Context;
    }

    switch (uMsg)
    {
    case WM_INITDIALOG:
        {
            context->ListViewHandle = GetDlgItem(hwndDlg, IDC_LIST);

            PhSetExtendedListView(context->ListViewHandle);
            PhSetListViewStyle(context->ListViewHandle, TRUE, TRUE);
            PhSetControlTheme(context->ListViewHandle, L"explorer");
            PhAddListViewColumn(context->ListViewHandle, 0, 0, 0, LVCFMT_LEFT, 40, L"#");
            PhAddListViewColumn(context->ListViewHandle, 1, 1, 1, LVCFMT_LEFT, 80, L"Name");
            PhAddListViewColumn(context->ListViewHandle, 2, 2, 2, LVCFMT_LEFT, 100, L"RVA (start)");
            PhAddListViewColumn(context->ListViewHandle, 3, 3, 3, LVCFMT_LEFT, 100, L"RVA (end)");
            PhAddListViewColumn(context->ListViewHandle, 4, 4, 4, LVCFMT_LEFT, 80, L"Size");
            PhAddListViewColumn(context->ListViewHandle, 5, 5, 5, LVCFMT_LEFT, 250, L"Characteristics");
            PhAddListViewColumn(context->ListViewHandle, 6, 6, 6, LVCFMT_LEFT, 80, L"Hash");
            //ExtendedListView_SetItemColorFunction(context->ListViewHandle, PvPeCharacteristicsColorFunction);
            ExtendedListView_SetCompareFunction(context->ListViewHandle, 1, PvPeVirtualAddressCompareFunction);
            ExtendedListView_SetCompareFunction(context->ListViewHandle, 2, PvPeSizeOfRawDataCompareFunction);
            ExtendedListView_SetCompareFunction(context->ListViewHandle, 3, PvPeCharacteristicsCompareFunction);
            PhLoadListViewColumnsFromSetting(L"ImageSectionsListViewColumns", context->ListViewHandle);
            PhLoadListViewSortColumnsFromSetting(L"ImageSectionsListViewSort", context->ListViewHandle);

            if (context->ListViewImageList = ImageList_Create(2, 20, ILC_MASK | ILC_COLOR, 1, 1))
                ListView_SetImageList(context->ListViewHandle, context->ListViewImageList, LVSIL_SMALL);

            PvSetPeImageSections(context->ListViewHandle);

            PhInitializeWindowTheme(hwndDlg, PeEnableThemeSupport);
        }
        break;
    case WM_DESTROY:
        {
            PhSaveListViewSortColumnsToSetting(L"ImageSectionsListViewSort", context->ListViewHandle);
            PhSaveListViewColumnsToSetting(L"ImageSectionsListViewColumns", context->ListViewHandle);

            if (context->ListViewImageList)
                ImageList_Destroy(context->ListViewImageList);

            PhFree(context);
            context = NULL;
        }
        break;
    case WM_SHOWWINDOW:
        {
            if (!propPageContext->LayoutInitialized)
            {
                PPH_LAYOUT_ITEM dialogItem;

                dialogItem = PvAddPropPageLayoutItem(hwndDlg, hwndDlg, PH_PROP_PAGE_TAB_CONTROL_PARENT, PH_ANCHOR_ALL);
                PvAddPropPageLayoutItem(hwndDlg, context->ListViewHandle, dialogItem, PH_ANCHOR_ALL);

                PvDoPropPageLayout(hwndDlg);

                propPageContext->LayoutInitialized = TRUE;
            }
        }
        break;
    case WM_NOTIFY:
        {
            PvHandleListViewNotifyForCopy(lParam, context->ListViewHandle);
        }
        break;
    case WM_CONTEXTMENU:
        {
            PvHandleListViewCommandCopy(hwndDlg, lParam, wParam, context->ListViewHandle);
        }
        break;
    }

    if (context)
    {
        REFLECT_MESSAGE_DLG(hwndDlg, context->ListViewHandle, uMsg, wParam, lParam);
    }

    return FALSE;
}
