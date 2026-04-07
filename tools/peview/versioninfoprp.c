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
#include <json.h>

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
    _In_ PCWSTR Name,
    _In_ PCWSTR Value
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
    PCWSTR offset = VersionInfo->Key + PhCountStringZ(VersionInfo->Key) + 1;

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

    if (!NT_SUCCESS(PhGetFileVersionInfoKey(
        VersionInfo,
        blockInfoName.Length / sizeof(WCHAR),
        blockInfoName.Buffer,
        &blockStringInfo
        )))
    {
        return FALSE;
    }

    PhInitFormatX(&format[0], LangCodePage);
    format[0].Type |= FormatPadZeros | FormatUpperCase;
    format[0].Width = 8;

    if (!PhFormatToBuffer(format, 1, langNameString, sizeof(langNameString), &returnLength))
        return FALSE;

    if (!NT_SUCCESS(PhGetFileVersionInfoKey(
        blockStringInfo,
        returnLength / sizeof(WCHAR) - sizeof(ANSI_NULL),
        langNameString,
        &blockLangInfo
        )))
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

static PCWSTR PvVersionInfoFileOSHostToString(
    _In_ ULONG FileOS
    )
{
    switch (HIWORD(FileOS))
    {
    case 0:  return L"";
    case 1:  return L"DOS";
    case 2:  return L"16-bit OS/2";
    case 3:  return L"32-bit OS/2";
    case 4:  return L"Windows NT";
    case 5:  return L"Windows CE";
    default: return L"Unknown";
    }
}

static PCWSTR PvVersionInfoFileOSGuestToString(
    _In_ ULONG FileOS
    )
{
    switch (LOWORD(FileOS))
    {
    case 0:  return L"";
    case 1:  return L"16-bit Windows";
    case 2:  return L"16-bit Presentation Manager";
    case 3:  return L"32-bit Presentation Manager";
    case 4:  return L"32-bit Windows";
    default: return L"Unknown";
    }
}

VOID PvLoadVersionBuildMetadata(
     _In_ HWND ListViewHandle,
    _Inout_ PULONG Count
    )
{
    static CONST PH_STRINGREF string = PH_STRINGREF_INIT(L"\\AppxManifest.xml");
    NTSTATUS status;
    HANDLE fileHandle;
    LARGE_INTEGER fileSize;
    PPH_BYTES fileContent;
    PVOID topNode;
    PPH_STRING fileName;
    PH_STRINGREF BasePathName;

    if (!PhGetBasePath(&PvFileName->sr, &BasePathName, NULL))
        return;

    fileName = PhConcatStringRef2(&BasePathName, &string);
    fileName = PhDosPathNameToNtPathName(&fileName->sr);

    status = PhCreateFile(
        &fileHandle,
        &fileName->sr,
        FILE_GENERIC_READ,
        FILE_ATTRIBUTE_NORMAL,
        FILE_SHARE_READ,
        FILE_OPEN,
        FILE_NON_DIRECTORY_FILE | FILE_SYNCHRONOUS_IO_NONALERT
        );

    if (!NT_SUCCESS(status))
        return;

    if (NT_SUCCESS(PhGetFileSize(fileHandle, &fileSize)) && fileSize.QuadPart == 0)
    {
        NtClose(fileHandle);
        return;
    }

    if (NT_SUCCESS(status = PhGetFileText(&fileContent, fileHandle, FALSE)))
    {
        if (topNode = PhLoadXmlObjectFromString2(fileContent->Buffer))
        {
            PVOID propertiesNode;
            PVOID buildMetadata;

            if (propertiesNode = PhFindXmlObject(topNode, topNode, "Properties", NULL, NULL))
            {
                for (PVOID child = PhGetXmlNodeFirstChild(propertiesNode); child; child = PhGetXmlNodeNextChild(child))
                {
                    PCSTR name = PhGetXmlNodeElementText(child);
                    PPH_STRING value = PhGetXmlNodeOpaqueText(child);

                    if (name && value && (
                        PhEqualBytesZ(name, "DisplayName", TRUE) ||
                        PhEqualBytesZ(name, "PublisherDisplayName", TRUE) ||
                        PhEqualBytesZ(name, "Logo", TRUE)
                        ))
                    {
                        PPH_STRING nameUtf16 = PhZeroExtendToUtf16(name);

                        PvAddVersionInfoItem(
                            ListViewHandle,
                            Count,
                            3,
                            PhGetString(nameUtf16),
                            PhGetString(value)
                            );

                        PhClearReference(&nameUtf16);
                    }

                    PhClearReference(&value);
                }
            }

            if (buildMetadata = PhFindXmlObject(topNode, topNode, "build:Metadata", NULL, NULL))
            {
                for (PVOID child = PhGetXmlNodeFirstChild(buildMetadata); child; child = PhGetXmlNodeNextChild(child))
                {
                    SIZE_T attributeCount = PhGetXmlNodeAttributeCount(child);
                    const char* name = PhGetXmlNodeElementText(child);
                    PPH_STRING value = PhGetXmlNodeOpaqueText(child);

                    if (name && value)
                    {
                        PPH_STRING attributeName = PhGetXmlNodeAttributeText(child, "Name");
                        PPH_STRING attributeValue = PhGetXmlNodeAttributeText(child, "Value");
                        PPH_STRING attributeVersion = PhGetXmlNodeAttributeText(child, "Version");

                        PvAddVersionInfoItem(
                            ListViewHandle,
                            Count,
                            3,
                            PhGetString(attributeName),
                            attributeValue ? PhGetString(attributeValue) : PhGetString(attributeVersion)
                            );

                        PhClearReference(&attributeVersion);
                        PhClearReference(&attributeValue);
                        PhClearReference(&attributeName);
                    }

                    PhClearReference(&value);
                }
            }

            PhDereferenceObject(fileContent);

            PhFreeXmlObject(topNode);
        }
    }
}

PPH_STRING PvVersionInfoFileOSToString(
    _In_ ULONG FileOS
    )
{
    PH_STRING_BUILDER stringBuilder;
    WCHAR pointer[PH_PTR_STR_LEN_1];

    if (FileOS == 0)
        return PhCreateString(L"Unknown (0)");

    PhInitializeStringBuilder(&stringBuilder, 10);

    if (LOWORD(FileOS))
    {
        PhAppendStringBuilder2(&stringBuilder, PvVersionInfoFileOSGuestToString(FileOS));
        if (HIWORD(FileOS))
            PhAppendStringBuilder2(&stringBuilder, L" on ");
    }

    PhAppendStringBuilder2(&stringBuilder, PvVersionInfoFileOSHostToString(FileOS));

    PhPrintPointer(pointer, UlongToPtr(FileOS));
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

PVOID PvGetFileVersionInfoValue2(
    _In_ PVS_VERSION_INFO_STRUCT32 VersionInfo
    )
{
    const PCWSTR keyOffset = VersionInfo->Key + PhCountStringZ(VersionInfo->Key) + 1;

    return PTR_ADD_OFFSET(VersionInfo, ALIGN_UP(PTR_SUB_OFFSET(keyOffset, VersionInfo), ULONG));
}

VOID PvEnumFileVersionInfo(
    _In_ HWND ListViewHandle,
    _Inout_ PULONG Count,
    _In_ PVOID VersionBlock,
    _In_ PPH_STRING CurrentName,
    _In_ ULONG Depth)
{
    PVOID value;
    ULONG valueOffset;
    PVS_VERSION_INFO_STRUCT32 child;
    PVS_VERSION_INFO_STRUCT32 VersionInfo = VersionBlock;

    if (!(value = PvGetFileVersionInfoValue2(VersionInfo)))
        return;

    valueOffset = VersionInfo->ValueLength * (VersionInfo->Type ? sizeof(WCHAR) : sizeof(CHAR));
    child = PTR_ADD_OFFSET(value, ALIGN_UP(valueOffset, ULONG));

    while ((ULONG_PTR)child < (ULONG_PTR)PTR_ADD_OFFSET(VersionInfo, VersionInfo->Length))
    {
        if (CurrentName)
        {
            PPH_STRING string;

            string = PhFormatString(
                L"\\%s\\%s",
                PhGetString(CurrentName),
                child->Key
                );

            PvEnumFileVersionInfo(
                ListViewHandle,
                Count,
                child,
                string,
                Depth + 1
                );

            PvAddVersionInfoItem(
                ListViewHandle,
                Count,
                1,
                PhGetString(string),
                child->ValueLength ? PvGetFileVersionInfoValue(child) : L""
                );

            PhDereferenceObject(string);
        }
        else
        {
            PPH_STRING nameString = PhFormatString(L"%s", child->Key);
            PvEnumFileVersionInfo(ListViewHandle, Count, child, nameString, Depth + 1);
            PhDereferenceObject(nameString);
        }

        if (child->Length == 0)
            break;

        child = PTR_ADD_OFFSET(child, ALIGN_UP(child->Length, ULONG));
    }
}

// Enumerate any custom strings in the StringFileInfo (dmex)
VOID PvEnumVersionInfo(
    _In_ HWND ListViewHandle
    )
{
    NTSTATUS status;
    ULONG count = 0;
    VS_FIXEDFILEINFO* rootBlock;
    PVOID versionInfo;

    status = PhGetFileVersionInfo(PvFileName->Buffer, &versionInfo);

    if (!NT_SUCCESS(status))
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

    static PH_STRINGREF blockStringInfoName = PH_STRINGREF_INIT(L"StringFileInfo");
    PVS_VERSION_INFO_STRUCT32 blockStringInfo;

    //PvEnumFileVersionInfo(ListViewHandle, &count, versionInfo, NULL, 0);

    if (NT_SUCCESS(PhGetFileVersionInfoKey(
        versionInfo,
        blockStringInfoName.Length / sizeof(WCHAR),
        blockStringInfoName.Buffer,
        &blockStringInfo
        )))
    {
        PvEnumFileVersionInfo(ListViewHandle, &count, blockStringInfo, NULL, 0);
    }

    {
        PLANGANDCODEPAGE codePage;
        ULONG codePageLength;

        if (NT_SUCCESS(PhGetFileVersionVarFileInfoValueZ(versionInfo, L"Translation", &codePage, &codePageLength)))
        {
            for (ULONG i = 0; i < (codePageLength / sizeof(LANGANDCODEPAGE)); i++)
            {
                SIZE_T returnLength;
                PH_FORMAT format[3];
                WCHAR langNameString[65];

                PhInitFormatX(&format[0], ((ULONG)codePage[i].Language << 16) + codePage[i].CodePage);
                format[0].Type |= FormatPadZeros | FormatUpperCase;
                format[0].Width = 8;

                if (PhFormatToBuffer(format, 1, langNameString, sizeof(langNameString), &returnLength))
                {
                    PvAddVersionInfoItem(ListViewHandle, &count, 2, L"Translation", langNameString);
                }
            }
        }
    }

    PhFree(versionInfo);

    PvLoadVersionBuildMetadata(ListViewHandle, &count);
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
            PhAddListViewGroup(context->ListViewHandle, 2, L"VarFileInfo");
            PhAddListViewGroup(context->ListViewHandle, 3, L"AppxManifest");

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
    case WM_DPICHANGED:
        {
            PhLayoutManagerUpdate(&context->LayoutManager, LOWORD(wParam));
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
            return (INT_PTR)PhGetStockBrush(DC_BRUSH);
        }
        break;
    }

    return FALSE;
}
