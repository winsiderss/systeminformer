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

typedef struct _PV_PE_VERSIONINFO_CONTEXT
{
    HWND WindowHandle;
    HWND ListViewHandle;
    HIMAGELIST ListViewImageList;
    PH_LAYOUT_MANAGER LayoutManager;
    PPV_PROPPAGECONTEXT PropSheetContext;
} PV_PE_VERSIONINFO_CONTEXT, *PPV_PE_VERSIONINFO_CONTEXT;

VOID PvAddVersionInfoItem(
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

PVOID PvGetFileVersionInfoValue(
    _In_ PVS_VERSION_INFO_STRUCT32 VersionInfo
    )
{
    PWSTR offset = VersionInfo->Key + PhCountStringZ(VersionInfo->Key) + 1;

    return PTR_ADD_OFFSET(VersionInfo, ALIGN_UP(PTR_SUB_OFFSET(offset, VersionInfo), ULONG));
}

BOOLEAN PvDumpFileVersionInfoKey(
    _In_ PVS_VERSION_INFO_STRUCT32 VersionInfo,
    _In_ HWND ListViewHandle,
    _Inout_ PULONG Count
    )
{
    PVOID value;
    ULONG valueOffset;
    PVS_VERSION_INFO_STRUCT32 child;

    if (!(value = PvGetFileVersionInfoValue(VersionInfo)))
        return FALSE;

    valueOffset = VersionInfo->ValueLength * (VersionInfo->Type ? sizeof(WCHAR) : sizeof(BYTE));
    child = PTR_ADD_OFFSET(value, ALIGN_UP(valueOffset, ULONG));

    while ((ULONG_PTR)child < (ULONG_PTR)PTR_ADD_OFFSET(VersionInfo, VersionInfo->Length))
    {
        if (child->Length == 0)
            break;

        PvAddVersionInfoItem(
            ListViewHandle,
            Count,
            1,
            child->Key,
            child->ValueLength ? PvGetFileVersionInfoValue(child) : L""
            );

        child = PTR_ADD_OFFSET(child, ALIGN_UP(child->Length, ULONG));
    }

    return FALSE;
}

BOOLEAN PvDumpFileVersionInfo(
    _In_ PVOID VersionInfo,
    _In_ ULONG LangCodePage,
    _In_ HWND ListViewHandle,
    _Inout_ PULONG Count
    )
{
    static PH_STRINGREF blockInfoName = PH_STRINGREF_INIT(L"StringFileInfo");
    PVS_VERSION_INFO_STRUCT32 blockStringInfo;
    PVS_VERSION_INFO_STRUCT32 blockLangInfo;
    SIZE_T returnLength;
    PH_FORMAT format[3];
    WCHAR langNameString[65];

    if (!PhGetFileVersionInfoKey(
        VersionInfo,
        blockInfoName.Length / sizeof(WCHAR),
        blockInfoName.Buffer,
        &blockStringInfo
        ))
    {
        return FALSE;
    }

    PhInitFormatX(&format[0], LangCodePage);
    format[0].Type |= FormatPadZeros | FormatUpperCase;
    format[0].Width = 8;

    if (!PhFormatToBuffer(format, 1, langNameString, sizeof(langNameString), &returnLength))
        return FALSE;

    if (!PhGetFileVersionInfoKey(
        blockStringInfo,
        returnLength / sizeof(WCHAR) - sizeof(ANSI_NULL),
        langNameString,
        &blockLangInfo
        ))
    {
        return FALSE;
    }

    PvDumpFileVersionInfoKey(blockLangInfo, ListViewHandle, Count);

    return TRUE;
}

PPH_STRING PvVersionInfoFlagsToString(
    _In_ ULONG Flags
    )
{
    PH_STRING_BUILDER stringBuilder;
    WCHAR pointer[PH_PTR_STR_LEN_1];

    if (Flags == 0)
        return PhCreateString(L"0");

    PhInitializeStringBuilder(&stringBuilder, 10);

    if (BooleanFlagOn(Flags, VS_FF_DEBUG))
        PhAppendStringBuilder2(&stringBuilder, L"Debug, ");
    if (BooleanFlagOn(Flags, VS_FF_PRERELEASE))
        PhAppendStringBuilder2(&stringBuilder, L"Pre-release, ");
    if (BooleanFlagOn(Flags, VS_FF_PATCHED))
        PhAppendStringBuilder2(&stringBuilder, L"Patched, ");
    if (BooleanFlagOn(Flags, VS_FF_PRIVATEBUILD))
        PhAppendStringBuilder2(&stringBuilder, L"Private build, ");
    if (BooleanFlagOn(Flags, VS_FF_INFOINFERRED))
        PhAppendStringBuilder2(&stringBuilder, L"INFOINFERRED, ");
    if (BooleanFlagOn(Flags, VS_FF_SPECIALBUILD))
        PhAppendStringBuilder2(&stringBuilder, L"Special build, ");

    if (PhEndsWithString2(stringBuilder.String, L", ", FALSE))
        PhRemoveEndStringBuilder(&stringBuilder, 2);

    PhPrintPointer(pointer, UlongToPtr(Flags));
    PhAppendFormatStringBuilder(&stringBuilder, L" (%s)", pointer);

    return PhFinalStringBuilderString(&stringBuilder);
}

PPH_STRING PvVersionInfoFileOSToString(
    _In_ ULONG Flags
    )
{
    PH_STRING_BUILDER stringBuilder;
    WCHAR pointer[PH_PTR_STR_LEN_1];

    if (Flags == 0)
        return PhCreateString(L"0");

    PhInitializeStringBuilder(&stringBuilder, 10);

    if (BooleanFlagOn(Flags, VOS_DOS_WINDOWS32))
        PhAppendStringBuilder2(&stringBuilder, L"DOS, ");
    if (BooleanFlagOn(Flags, VOS_NT_WINDOWS32))
        PhAppendStringBuilder2(&stringBuilder, L"Windows NT, ");

    if (PhEndsWithString2(stringBuilder.String, L", ", FALSE))
        PhRemoveEndStringBuilder(&stringBuilder, 2);

    PhPrintPointer(pointer, UlongToPtr(Flags));
    PhAppendFormatStringBuilder(&stringBuilder, L" (%s)", pointer);

    return PhFinalStringBuilderString(&stringBuilder);
}

PPH_STRING PvVersionInfoFileTypeToString(
    _In_ ULONG Flags
    )
{
    PH_STRING_BUILDER stringBuilder;
    WCHAR pointer[PH_PTR_STR_LEN_1];

    if (Flags == 0)
        return PhCreateString(L"0");

    PhInitializeStringBuilder(&stringBuilder, 10);

    if (Flags == VFT_UNKNOWN)
        PhAppendStringBuilder2(&stringBuilder, L"Unknown, ");
    if (Flags == VFT_APP)
        PhAppendStringBuilder2(&stringBuilder, L"Application, ");
    if (Flags == VFT_DLL)
        PhAppendStringBuilder2(&stringBuilder, L"DLL, ");
    if (Flags == VFT_DRV)
        PhAppendStringBuilder2(&stringBuilder, L"Driver, ");
    if (Flags == VFT_FONT)
        PhAppendStringBuilder2(&stringBuilder, L"Font, ");
    if (Flags == VFT_VXD)
        PhAppendStringBuilder2(&stringBuilder, L"VXD, ");
    if (Flags == VFT_STATIC_LIB)
        PhAppendStringBuilder2(&stringBuilder, L"Static library, ");

    if (PhEndsWithString2(stringBuilder.String, L", ", FALSE))
        PhRemoveEndStringBuilder(&stringBuilder, 2);

    PhPrintPointer(pointer, UlongToPtr(Flags));
    PhAppendFormatStringBuilder(&stringBuilder, L" (%s)", pointer);

    return PhFinalStringBuilderString(&stringBuilder);
}

// Enumerate any custom strings in the StringFileInfo (dmex)
VOID PvEnumVersionInfo(
    _In_ HWND ListViewHandle
    )
{
    ULONG count = 0;
    VS_FIXEDFILEINFO* rootBlock;
    PVOID versionInfo;
    ULONG langCodePage;

    versionInfo = PhGetFileVersionInfo(PvFileName->Buffer);

    if (!versionInfo)
        return;

    if (rootBlock = PhGetFileVersionFixedInfo(versionInfo))
    {
        PvAddVersionInfoItem(ListViewHandle, &count, 0, L"Signature", PhaFormatString(L"0x%lx", rootBlock->dwSignature)->Buffer);
        PvAddVersionInfoItem(ListViewHandle, &count, 0, L"Struct version", PhaFormatString(L"%lu.%lu", HIWORD(rootBlock->dwStrucVersion), LOWORD(rootBlock->dwStrucVersion))->Buffer);
        PvAddVersionInfoItem(ListViewHandle, &count, 0, L"File version", PhaFormatString(L"%lu.%lu.%lu.%lu", HIWORD(rootBlock->dwFileVersionMS), LOWORD(rootBlock->dwFileVersionMS), HIWORD(rootBlock->dwFileVersionLS), LOWORD(rootBlock->dwFileVersionLS))->Buffer);
        PvAddVersionInfoItem(ListViewHandle, &count, 0, L"Product version", PhaFormatString(L"%lu.%lu.%lu.%lu", HIWORD(rootBlock->dwProductVersionMS), LOWORD(rootBlock->dwProductVersionMS), HIWORD(rootBlock->dwProductVersionLS), LOWORD(rootBlock->dwProductVersionLS))->Buffer);
        PvAddVersionInfoItem(ListViewHandle, &count, 0, L"FileFlagsMask", PhaFormatString(L"0x%lx", rootBlock->dwFileFlagsMask)->Buffer);
        PvAddVersionInfoItem(ListViewHandle, &count, 0, L"FileFlags", PH_AUTO_T(PH_STRING, PvVersionInfoFlagsToString(rootBlock->dwFileFlags))->Buffer);
        PvAddVersionInfoItem(ListViewHandle, &count, 0, L"FileOS", PH_AUTO_T(PH_STRING, PvVersionInfoFileOSToString(rootBlock->dwFileOS))->Buffer);
        PvAddVersionInfoItem(ListViewHandle, &count, 0, L"FileType", PH_AUTO_T(PH_STRING, PvVersionInfoFileTypeToString(rootBlock->dwFileType))->Buffer);
        PvAddVersionInfoItem(ListViewHandle, &count, 0, L"FileSubtype", PhaFormatString(L"%lu", rootBlock->dwFileSubtype)->Buffer); // VFT2_DRV_KEYBOARD
        PvAddVersionInfoItem(ListViewHandle, &count, 0, L"FileDate", PhaFormatString(L"%lu.%lu.%lu.%lu", HIWORD(rootBlock->dwFileDateMS), LOWORD(rootBlock->dwFileDateMS), HIWORD(rootBlock->dwFileDateLS), LOWORD(rootBlock->dwFileDateLS))->Buffer);
    }

    // TODO: Enumerate the language block.
    langCodePage = PhGetFileVersionInfoLangCodePage(versionInfo);

    // Use the default language code page.
    if (!PvDumpFileVersionInfo(versionInfo, langCodePage, ListViewHandle, &count))
    {
        // Use the windows-1252 code page.
        if (!PvDumpFileVersionInfo(versionInfo, (langCodePage & 0xffff0000) + 1252, ListViewHandle, &count))
        {
            // Use the default language (US English).
            if (!PvDumpFileVersionInfo(versionInfo, (MAKELANGID(LANG_ENGLISH, SUBLANG_ENGLISH_US) << 16) + 1252, ListViewHandle, &count))
            {
                // Use the default language (US English).
                PvDumpFileVersionInfo(versionInfo, (MAKELANGID(LANG_ENGLISH, SUBLANG_ENGLISH_US) << 16) + 0, ListViewHandle, &count);
            }
        }
    }

    PhFree(versionInfo);
}

INT_PTR CALLBACK PvpPeVersionInfoDlgProc(
    _In_ HWND hwndDlg,
    _In_ UINT uMsg,
    _In_ WPARAM wParam,
    _In_ LPARAM lParam
    )
{
    PPV_PE_VERSIONINFO_CONTEXT context;

    if (uMsg == WM_INITDIALOG)
    {
        context = PhAllocateZero(sizeof(PV_PE_VERSIONINFO_CONTEXT));
        PhSetWindowContext(hwndDlg, PH_WINDOW_CONTEXT_DEFAULT, context);

        if (lParam)
        {
            LPPROPSHEETPAGE propSheetPage = (LPPROPSHEETPAGE)lParam;
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
            PhAddListViewColumn(context->ListViewHandle, 1, 1, 1, LVCFMT_LEFT, 200, L"Name");
            PhAddListViewColumn(context->ListViewHandle, 2, 2, 2, LVCFMT_LEFT, 250, L"Value");
            PhSetExtendedListView(context->ListViewHandle);
            PhLoadListViewColumnsFromSetting(L"ImageVersionInfoListViewColumns", context->ListViewHandle);
            PvConfigTreeBorders(context->ListViewHandle);

            PhInitializeLayoutManager(&context->LayoutManager, hwndDlg);
            PhAddLayoutItem(&context->LayoutManager, context->ListViewHandle, NULL, PH_ANCHOR_ALL);

            ListView_EnableGroupView(context->ListViewHandle, TRUE);
            PhAddListViewGroup(context->ListViewHandle, 0, L"FixedFileInfo");
            PhAddListViewGroup(context->ListViewHandle, 1, L"StringFileInfo");

            PvEnumVersionInfo(context->ListViewHandle);

            PhInitializeWindowTheme(hwndDlg, PhEnableThemeSupport);
        }
        break;
    case WM_DESTROY:
        {
            PhSaveListViewColumnsToSetting(L"ImageVersionInfoListViewColumns", context->ListViewHandle);

            PhDeleteLayoutManager(&context->LayoutManager);

            PhRemoveWindowContext(hwndDlg, PH_WINDOW_CONTEXT_DEFAULT);
            PhFree(context);
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
            PvHandleListViewCommandCopy(hwndDlg, lParam, wParam, context->ListViewHandle);
        }
        break;
    case WM_NOTIFY:
        {
            LPNMHDR header = (LPNMHDR)lParam;

            switch (header->code)
            {
            case PSN_QUERYINITIALFOCUS:
                SetWindowLongPtr(hwndDlg, DWLP_MSGRESULT, (LONG_PTR)context->ListViewHandle);
                return TRUE;
            }

            PvHandleListViewNotifyForCopy(lParam, context->ListViewHandle);
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
