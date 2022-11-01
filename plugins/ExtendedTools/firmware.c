/*
 * Copyright (c) 2022 Winsider Seminars & Solutions, Inc.  All rights reserved.
 *
 * This file is part of System Informer.
 *
 * Authors:
 *
 *     dmex    2016-2022
 *
 */

#include "exttools.h"
#include "efi_guid_list.h"
#include "Efi\EfiTypes.h"
#include "Efi\EfiDevicePath.h"

PPH_STRING EtFirmwareAttributeToString(
    _In_ ULONG Attribute
    )
{
    PH_STRING_BUILDER sb;

    PhInitializeStringBuilder(&sb, 0x100);

    if (Attribute & EFI_VARIABLE_NON_VOLATILE)
        PhAppendStringBuilder2(&sb, L"Non Volatile, ");

    if (Attribute & EFI_VARIABLE_BOOTSERVICE_ACCESS)
        PhAppendStringBuilder2(&sb, L"Boot Service, ");

    if (Attribute & EFI_VARIABLE_RUNTIME_ACCESS)
        PhAppendStringBuilder2(&sb, L"Runtime Access, ");

    if (Attribute & EFI_VARIABLE_HARDWARE_ERROR_RECORD)
        PhAppendStringBuilder2(&sb, L"Hardware Error Record, ");

    if (Attribute & EFI_VARIABLE_AUTHENTICATED_WRITE_ACCESS)
        PhAppendStringBuilder2(&sb, L"Authenticated Write Access, ");

    if (Attribute & EFI_VARIABLE_TIME_BASED_AUTHENTICATED_WRITE_ACCESS)
        PhAppendStringBuilder2(&sb, L"Authenticated Write Access (Time Based), ");

    if (Attribute & EFI_VARIABLE_APPEND_WRITE)
        PhAppendStringBuilder2(&sb, L"Append Write, ");

    if (PhEndsWithStringRef2(&sb.String->sr, L", ", FALSE))
        PhRemoveEndStringBuilder(&sb, 2);

    return PhFinalStringBuilderString(&sb);
}

PWSTR EtFirmwareGuidToNameString(
    _In_ PGUID VendorGuid
    )
{
    for (ULONG i = 0; i < ARRAYSIZE(table); i++)
    {
        if (IsEqualGUID(VendorGuid, &table[i].Guid))
            return table[i].Name;
    }

    return L"";
}

VOID FreeListViewFirmwareEntries(
    _In_ PUEFI_WINDOW_CONTEXT Context
    )
{
    ULONG index = ULONG_MAX;

    while ((index = PhFindListViewItemByFlags(
        Context->ListViewHandle,
        index,
        LVNI_ALL
        )) != -1)
    {
        PEFI_ENTRY param;

        if (PhGetListViewItemParam(Context->ListViewHandle, index, &param))
        {
            PhClearReference(&param->Name);
            PhClearReference(&param->GuidString);
            PhFree(param);
        }
    }
}

NTSTATUS EtEnumerateFirmwareEntries(
    _In_ PUEFI_WINDOW_CONTEXT Context
    )
{
    NTSTATUS status;
    PVOID variables;

    ExtendedListView_SetRedraw(Context->ListViewHandle, FALSE);
    FreeListViewFirmwareEntries(Context);
    ListView_DeleteAllItems(Context->ListViewHandle);

    status = PhEnumFirmwareEnvironmentValues(
        SystemEnvironmentValueInformation,
        &variables
        );

    if (NT_SUCCESS(status))
    {
        PVARIABLE_NAME_AND_VALUE i;

        for (i = PH_FIRST_FIRMWARE_VALUE(variables); i; i = PH_NEXT_FIRMWARE_VALUE(i))
        {
            INT index;
            PEFI_ENTRY entry;

            entry = PhAllocateZero(sizeof(EFI_ENTRY));
            entry->Length = i->ValueLength;
            entry->Name = PhCreateString(i->Name);
            entry->GuidString = PhFormatGuid(&i->VendorGuid);

            index = PhAddListViewItem(
                Context->ListViewHandle,
                MAXINT,
                PhGetStringOrEmpty(entry->Name),
                entry
                );
            PhSetListViewSubItem(
                Context->ListViewHandle,
                index,
                1,
                PH_AUTO_T(PH_STRING, EtFirmwareAttributeToString(i->Attributes))->Buffer
                );
            PhSetListViewSubItem(
                Context->ListViewHandle,
                index,
                2,
                EtFirmwareGuidToNameString(&i->VendorGuid)
                );
            PhSetListViewSubItem(
                Context->ListViewHandle,
                index,
                3,
                PhGetStringOrEmpty(entry->GuidString)
                );
            PhSetListViewSubItem(
                Context->ListViewHandle,
                index,
                4,
                PhaFormatSize(entry->Length, ULONG_MAX)->Buffer
                );
        }

        PhFree(variables);
    }

    ExtendedListView_SortItems(Context->ListViewHandle);
    ExtendedListView_SetRedraw(Context->ListViewHandle, TRUE);

    return status;
}

VOID EtShowFirmwareMenu(
    _In_ PUEFI_WINDOW_CONTEXT Context,
    _In_ HWND hwndDlg
    )
{
    PPH_STRING name;

    if (name = PhGetSelectedListViewItemText(Context->ListViewHandle))
    {
        // Nothing
        PhDereferenceObject(name);
    }
}

INT NTAPI EtFirmwareNameCompareFunction(
    _In_ PVOID Item1,
    _In_ PVOID Item2,
    _In_opt_ PVOID Context
    )
{
    PEFI_ENTRY item1 = Item1;
    PEFI_ENTRY item2 = Item2;

    return PhCompareStringZ(PhGetStringOrEmpty(item1->Name), PhGetStringOrEmpty(item2->Name), TRUE);
}

INT NTAPI EtFirmwareEntryLengthCompareFunction(
    _In_ PVOID Item1,
    _In_ PVOID Item2,
    _In_opt_ PVOID Context
    )
{
    PEFI_ENTRY item1 = Item1;
    PEFI_ENTRY item2 = Item2;

    return uintcmp(item1->Length, item2->Length);
}

INT_PTR CALLBACK EtFirmwareDlgProc(
    _In_ HWND hwndDlg,
    _In_ UINT uMsg,
    _In_ WPARAM wParam,
    _In_ LPARAM lParam
    )
{
    PUEFI_WINDOW_CONTEXT context = NULL;

    if (uMsg == WM_INITDIALOG)
    {
        context = PhAllocateZero(sizeof(UEFI_WINDOW_CONTEXT));
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
            PhSetApplicationWindowIcon(hwndDlg);

            context->ListViewHandle = GetDlgItem(hwndDlg, IDC_FIRMWARE_BOOT_LIST);
            context->ParentWindowHandle = (HWND)lParam;

            PhSetListViewStyle(context->ListViewHandle, FALSE, TRUE);
            PhSetControlTheme(context->ListViewHandle, L"explorer");
            PhAddListViewColumn(context->ListViewHandle, 0, 0, 0, LVCFMT_LEFT, 100, L"Name");
            PhAddListViewColumn(context->ListViewHandle, 1, 1, 1, LVCFMT_LEFT, 140, L"Attributes");
            PhAddListViewColumn(context->ListViewHandle, 2, 2, 2, LVCFMT_LEFT, 140, L"Guid Name");
            PhAddListViewColumn(context->ListViewHandle, 3, 3, 3, LVCFMT_LEFT, 140, L"Guid");
            PhAddListViewColumn(context->ListViewHandle, 4, 4, 4, LVCFMT_LEFT, 50, L"Data Length");
            PhSetExtendedListView(context->ListViewHandle);

            //ExtendedListView_SetSortFast(context->ListViewHandle, TRUE);
            ExtendedListView_SetCompareFunction(context->ListViewHandle, 0, EtFirmwareNameCompareFunction);
            ExtendedListView_SetCompareFunction(context->ListViewHandle, 1, EtFirmwareNameCompareFunction);
            ExtendedListView_SetCompareFunction(context->ListViewHandle, 4, EtFirmwareEntryLengthCompareFunction);
            PhLoadListViewColumnsFromSetting(SETTING_NAME_FIRMWARE_LISTVIEW_COLUMNS, context->ListViewHandle);

            PhInitializeLayoutManager(&context->LayoutManager, hwndDlg);
            PhAddLayoutItem(&context->LayoutManager, context->ListViewHandle, NULL, PH_ANCHOR_ALL);
            PhAddLayoutItem(&context->LayoutManager, GetDlgItem(hwndDlg, IDC_FIRMWARE_BOOT_REFRESH), NULL, PH_ANCHOR_BOTTOM | PH_ANCHOR_LEFT);
            PhAddLayoutItem(&context->LayoutManager, GetDlgItem(hwndDlg, IDOK), NULL, PH_ANCHOR_BOTTOM | PH_ANCHOR_RIGHT);

            if (PhGetIntegerPairSetting(SETTING_NAME_FIRMWARE_WINDOW_POSITION).X != 0)
                PhLoadWindowPlacementFromSetting(SETTING_NAME_FIRMWARE_WINDOW_POSITION, SETTING_NAME_FIRMWARE_WINDOW_SIZE, hwndDlg);
            else
                PhCenterWindow(hwndDlg, context->ParentWindowHandle);

            PhInitializeWindowTheme(hwndDlg, !!PhGetIntegerSetting(L"EnableThemeSupport"));

            EtEnumerateFirmwareEntries(context);
        }
        break;
    case WM_DESTROY:
        {
            FreeListViewFirmwareEntries(context);

            PhSaveListViewColumnsToSetting(SETTING_NAME_FIRMWARE_LISTVIEW_COLUMNS, context->ListViewHandle);
            PhSaveWindowPlacementToSetting(SETTING_NAME_FIRMWARE_WINDOW_POSITION, SETTING_NAME_FIRMWARE_WINDOW_SIZE, hwndDlg);
            PhDeleteLayoutManager(&context->LayoutManager);

            PhRemoveWindowContext(hwndDlg, PH_WINDOW_CONTEXT_DEFAULT);
            PhFree(context);
        }
        break;
    case WM_SIZE:
        PhLayoutManagerLayout(&context->LayoutManager);
        break;
    case WM_COMMAND:
        {
            switch (LOWORD(wParam))
            {
            case IDC_FIRMWARE_BOOT_REFRESH:
                {
                    EtEnumerateFirmwareEntries(context);
                }
                break;
            case IDCANCEL:
            case IDOK:
                EndDialog(hwndDlg, IDOK);
                break;
            }
        }
        break;
    case WM_NOTIFY:
        {
            LPNMHDR hdr = (LPNMHDR)lParam;

            PhHandleListViewNotifyForCopy(lParam, context->ListViewHandle);

            switch (hdr->code)
            {
            case NM_RCLICK:
                {
                    if (hdr->hwndFrom == context->ListViewHandle)
                        EtShowFirmwareMenu(context, hwndDlg);
                }
                break;
            case NM_DBLCLK:
                {
                    if (hdr->hwndFrom == context->ListViewHandle)
                    {
                        PEFI_ENTRY entry;

                        if (entry = PhGetSelectedListViewItemParam(context->ListViewHandle))
                        {
                            EtShowFirmwareEditDialog(hwndDlg, entry);
                        }
                    }
                }
                break;
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

VOID EtShowFirmwareDialog(
    _In_ HWND ParentWindowHandle
    )
{
    DialogBoxParam(
        PluginInstance->DllBase,
        MAKEINTRESOURCE(IDD_FIRMWARE),
        NULL,
        EtFirmwareDlgProc,
        (LPARAM)ParentWindowHandle
        );
}
