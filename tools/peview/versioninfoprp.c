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
    HWND EditWindow;
    HIMAGELIST ListViewImageList;
    PH_LAYOUT_MANAGER LayoutManager;
    PPV_PROPPAGECONTEXT PropSheetContext;
} PV_PE_VERSIONINFO_CONTEXT, *PPV_PE_VERSIONINFO_CONTEXT;

VOID PvpSetRichEditVersionInfoText(
    _In_ HWND WindowHandle,
    _In_ PWSTR Text
    )
{
    //SetFocus(WindowHandle);
    SendMessage(WindowHandle, WM_SETREDRAW, FALSE, 0);
    //SendMessage(WindowHandle, EM_SETSEL, 0, -1); // -2
    SendMessage(WindowHandle, WM_SETTEXT, FALSE, (LPARAM)Text);
    //SendMessage(WindowHandle, WM_VSCROLL, SB_TOP, 0); // requires SetFocus()
    SendMessage(WindowHandle, WM_SETREDRAW, TRUE, 0);
    //PostMessage(WindowHandle, EM_SETSEL, -1, 0);
    //InvalidateRect(WindowHandle, NULL, FALSE);
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
    _In_ PPH_STRING_BUILDER StringBuilder
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

        PhAppendFormatStringBuilder(StringBuilder, L"%s:\t%s\r\n", child->Key, (PWSTR)PvGetFileVersionInfoValue(child));

        child = PTR_ADD_OFFSET(child, ALIGN_UP(child->Length, ULONG));
    }

    return FALSE;
}

BOOLEAN PvDumpFileVersionInfo(
    _In_ PVOID VersionInfo, 
    _In_ ULONG LangCodePage,
    _In_ PPH_STRING_BUILDER StringBuilder
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

    PvDumpFileVersionInfoKey(blockLangInfo, StringBuilder);

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
    _In_ HWND WindowHandle
    )
{
    PPH_STRING fileText;
    PH_STRING_BUILDER sb;
    VS_FIXEDFILEINFO* rootBlock;
    PVOID versionInfo;
    ULONG langCodePage;

    versionInfo = PhGetFileVersionInfo(PvFileName->Buffer);

    if (!versionInfo)
        return;

    PhInitializeStringBuilder(&sb, 260);

    if (rootBlock = PhGetFileVersionFixedInfo(versionInfo))
    {
        PhAppendFormatStringBuilder(&sb, L"Signature:\t0x%lx\r\n", rootBlock->dwSignature);
        PhAppendFormatStringBuilder(&sb, L"Struct version:\t%lu.%lu\r\n", HIWORD(rootBlock->dwStrucVersion), LOWORD(rootBlock->dwStrucVersion));
        PhAppendFormatStringBuilder(&sb, L"File version:\t%lu.%lu.%lu.%lu\r\n", HIWORD(rootBlock->dwFileVersionMS), LOWORD(rootBlock->dwFileVersionMS), HIWORD(rootBlock->dwFileVersionLS), LOWORD(rootBlock->dwFileVersionLS));
        PhAppendFormatStringBuilder(&sb, L"Product version:\t%lu.%lu.%lu.%lu\r\n", HIWORD(rootBlock->dwProductVersionMS), LOWORD(rootBlock->dwProductVersionMS), HIWORD(rootBlock->dwProductVersionLS), LOWORD(rootBlock->dwProductVersionLS));
        PhAppendFormatStringBuilder(&sb, L"FileFlagsMask:\t0x%lx\r\n", rootBlock->dwFileFlagsMask);
        PhAppendFormatStringBuilder(&sb, L"FileFlags:\t\t%s\r\n", PH_AUTO_T(PH_STRING, PvVersionInfoFlagsToString(rootBlock->dwFileFlags))->Buffer);
        PhAppendFormatStringBuilder(&sb, L"FileOS:\t\t%s\r\n", PH_AUTO_T(PH_STRING, PvVersionInfoFileOSToString(rootBlock->dwFileOS))->Buffer);
        PhAppendFormatStringBuilder(&sb, L"FileType:\t\t%s\r\n", PH_AUTO_T(PH_STRING, PvVersionInfoFileTypeToString(rootBlock->dwFileType))->Buffer);
        PhAppendFormatStringBuilder(&sb, L"FileSubtype:\t%lu\r\n", rootBlock->dwFileSubtype); // VFT2_DRV_KEYBOARD
        PhAppendFormatStringBuilder(&sb, L"FileDate:\t\t%lu.%lu.%lu.%lu\r\n", HIWORD(rootBlock->dwFileDateMS), LOWORD(rootBlock->dwFileDateMS), HIWORD(rootBlock->dwFileDateLS), LOWORD(rootBlock->dwFileDateLS));
    }

    // TODO: Enumerate the language block.
    langCodePage = PhGetFileVersionInfoLangCodePage(versionInfo);
    PhAppendStringBuilder2(&sb, L"\r\nBLOCK \"StringFileInfo\"\r\n");

    // Use the default language code page.
    if (!PvDumpFileVersionInfo(versionInfo, langCodePage, &sb))
    {
        // Use the windows-1252 code page.
        if (!PvDumpFileVersionInfo(versionInfo, (langCodePage & 0xffff0000) + 1252, &sb))
        {
            // Use the default language (US English).
            if (!PvDumpFileVersionInfo(versionInfo, (MAKELANGID(LANG_ENGLISH, SUBLANG_ENGLISH_US) << 16) + 1252, &sb))
            {
                // Use the default language (US English).
                PvDumpFileVersionInfo(versionInfo, (MAKELANGID(LANG_ENGLISH, SUBLANG_ENGLISH_US) << 16) + 0, &sb);
            }
        }
    }

    PhFree(versionInfo);

    fileText = PhFinalStringBuilderString(&sb);
    PvpSetRichEditVersionInfoText(GetDlgItem(WindowHandle, IDC_PREVIEW), fileText->Buffer);
    PhDereferenceObject(fileText);
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
            context->EditWindow = GetDlgItem(hwndDlg, IDC_PREVIEW);

            SendMessage(context->EditWindow, EM_SETLIMITTEXT, ULONG_MAX, 0);
            PvConfigTreeBorders(context->EditWindow);

            PhInitializeLayoutManager(&context->LayoutManager, hwndDlg);
            PhAddLayoutItem(&context->LayoutManager, context->EditWindow, NULL, PH_ANCHOR_ALL);

            PvEnumVersionInfo(hwndDlg);

            PhInitializeWindowTheme(hwndDlg, PeEnableThemeSupport);
        }
        break;
    case WM_DESTROY:
        {
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
    case WM_NOTIFY:
        {
            LPNMHDR header = (LPNMHDR)lParam;

            switch (header->code)
            {
            case PSN_QUERYINITIALFOCUS:
                SetWindowLongPtr(hwndDlg, DWLP_MSGRESULT, (LONG_PTR)context->EditWindow);
                return TRUE;
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
