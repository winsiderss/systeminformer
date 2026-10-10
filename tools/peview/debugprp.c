/*
 * Copyright (c) 2022 Winsider Seminars & Solutions, Inc.  All rights reserved.
 *
 * This file is part of System Informer.
 *
 * Authors:
 *
 *     dmex    2021-2026
 *
 */

#include <peview.h>

typedef struct _PVP_PE_DEBUG_CONTEXT
{
    HWND WindowHandle;
    HWND ListViewHandle;
    PH_LAYOUT_MANAGER LayoutManager;
    PPV_PROPPAGECONTEXT PropSheetContext;
} PVP_PE_DEBUG_CONTEXT, *PPVP_PE_DEBUG_CONTEXT;

typedef struct _IMAGE_DEBUG_VC_FEATURE_ENTRY
{
    ULONG PreVCPlusPlusCount;
    ULONG CAndCPlusPlusCount;
    ULONG GuardStackCount;
    ULONG SdlCount;
    ULONG GuardCount;
} IMAGE_DEBUG_VC_FEATURE_ENTRY, *PIMAGE_DEBUG_VC_FEATURE_ENTRY;

#ifndef IMAGE_DLLCHARACTERISTICS_EX_CET_COMPAT
#define IMAGE_DLLCHARACTERISTICS_EX_CET_COMPAT 0x01
#endif
#ifndef IMAGE_DLLCHARACTERISTICS_EX_CET_COMPAT_STRICT_MODE
#define IMAGE_DLLCHARACTERISTICS_EX_CET_COMPAT_STRICT_MODE 0x02
#endif
#ifndef IMAGE_DLLCHARACTERISTICS_EX_CET_SET_CONTEXT_IP_VALIDATION_RELAXED_MODE
#define IMAGE_DLLCHARACTERISTICS_EX_CET_SET_CONTEXT_IP_VALIDATION_RELAXED_MODE 0x04
#endif
#ifndef IMAGE_DLLCHARACTERISTICS_EX_CET_DYNAMIC_APIS_ALLOW_IN_PROC
#define IMAGE_DLLCHARACTERISTICS_EX_CET_DYNAMIC_APIS_ALLOW_IN_PROC 0x08
#endif
#ifndef IMAGE_DLLCHARACTERISTICS_EX_CET_RESERVED_1
#define IMAGE_DLLCHARACTERISTICS_EX_CET_RESERVED_1 0x10
#endif
#ifndef IMAGE_DLLCHARACTERISTICS_EX_CET_RESERVED_2
#define IMAGE_DLLCHARACTERISTICS_EX_CET_RESERVED_2 0x20
#endif

PWSTR PvpGetDebugTypeString(
    _In_ ULONG Type
    )
{
    switch (Type)
    {
    case IMAGE_DEBUG_TYPE_COFF:
        return L"COFF";
    case IMAGE_DEBUG_TYPE_CODEVIEW:
        return L"CODEVIEW";
    case IMAGE_DEBUG_TYPE_FPO:
        return L"FPO";
    case IMAGE_DEBUG_TYPE_MISC:
        return L"MISC";
    case IMAGE_DEBUG_TYPE_EXCEPTION:
        return L"EXCEPTION";
    case IMAGE_DEBUG_TYPE_FIXUP:
        return L"FIXUP";
    case IMAGE_DEBUG_TYPE_OMAP_TO_SRC:
        return L"OMAP_TO_SRC";
    case IMAGE_DEBUG_TYPE_OMAP_FROM_SRC:
        return L"OMAP_FROM_SRC";
    case IMAGE_DEBUG_TYPE_BORLAND:
        return L"BORLAND";
    case IMAGE_DEBUG_TYPE_RESERVED10: // coreclr
        return L"RESERVED10";
    case IMAGE_DEBUG_TYPE_CLSID:
        return L"CLSID";
    case IMAGE_DEBUG_TYPE_VC_FEATURE:
        return L"VC_FEATURE";
    case IMAGE_DEBUG_TYPE_POGO:
        return L"POGO";
    case IMAGE_DEBUG_TYPE_ILTCG:
        return L"ILTCG";
    case IMAGE_DEBUG_TYPE_MPX:
        return L"MPX";
    case IMAGE_DEBUG_TYPE_REPRO:
        return L"REPRO";
    case IMAGE_DEBUG_TYPE_EMBEDDEDPORTABLEPDB:
        return L"EMBEDDED_PDB";
    // Note: missing 18.
    case IMAGE_DEBUG_TYPE_PDBCHECKSUM:
        return L"PDB_CHECKSUM";
    case IMAGE_DEBUG_TYPE_EX_DLLCHARACTERISTICS:
        return L"EX_DLLCHARACTERISTICS";
    case IMAGE_DEBUG_TYPE_PERFMAP:
        return L"PERFMAP";
    }

    return PhaFormatString(L"%lu", Type)->Buffer;
}

PVOID PvpGetDebugEntryData(
    _In_ PPH_IMAGE_DEBUG_ENTRY Entry
    )
{
    PVOID data = NULL;

    if (Entry->SizeOfData == 0)
        return NULL;

    if (Entry->AddressOfRawData)
    {
        if (!NT_SUCCESS(PhMappedImageRvaToVa(&PvMappedImage, Entry->AddressOfRawData, &data)))
            data = NULL;
    }

    if (!data && Entry->PointerToRawData)
    {
        if ((ULONG64)Entry->PointerToRawData + Entry->SizeOfData <= PvMappedImage.ViewSize)
        {
            data = PTR_ADD_OFFSET(PvMappedImage.ViewBase, Entry->PointerToRawData);
        }
    }

    if (data && (ULONG64)PTR_SUB_OFFSET(data, PvMappedImage.ViewBase) + Entry->SizeOfData > PvMappedImage.ViewSize)
    {
        data = NULL;
    }

    return data;
}

LONG PvpAddDebugGroupItem(
    _In_ PPVP_PE_DEBUG_CONTEXT Context,
    _In_ LONG GroupId,
    _In_ PWSTR Name,
    _In_opt_ PWSTR Value
    )
{
    LONG lvItemIndex;

    lvItemIndex = PhAddListViewGroupItem(Context->ListViewHandle, GroupId, MAXINT, Name, NULL);

    if (Value)
    {
        PhSetListViewSubItem(Context->ListViewHandle, lvItemIndex, 1, Value);
    }

    return lvItemIndex;
}

VOID PvpAddDebugCodeViewItems(
    _In_ PPVP_PE_DEBUG_CONTEXT Context,
    _In_ LONG GroupId,
    _In_ PVOID Data,
    _In_ ULONG DataLength
    )
{
    ULONG signature;

    if (DataLength < sizeof(ULONG))
        return;

    signature = *(PULONG)Data;

    if (signature == CODEVIEW_SIGNATURE_RSDS && DataLength >= UFIELD_OFFSET(CODEVIEW_INFO_PDB70, ImageName))
    {
        PCODEVIEW_INFO_PDB70 codeview = Data;
        ULONG maximumLength = DataLength - UFIELD_OFFSET(CODEVIEW_INFO_PDB70, ImageName);
        SIZE_T nameLength = strnlen(codeview->ImageName, maximumLength);
        PPH_STRING guidString;
        PPH_STRING nameString;

        PvpAddDebugGroupItem(Context, GroupId, L"Signature", L"RSDS (PDB 7.0)");

        if (guidString = PhFormatGuid(&codeview->PdbGuid))
        {
            PvpAddDebugGroupItem(Context, GroupId, L"PDB GUID", guidString->Buffer);
            PhDereferenceObject(guidString);
        }

        PvpAddDebugGroupItem(Context, GroupId, L"PDB age", PhaFormatUInt64(codeview->PdbAge, FALSE)->Buffer);

        if (nameLength && (nameString = PhConvertUtf8ToUtf16Ex(codeview->ImageName, nameLength)))
        {
            PvpAddDebugGroupItem(Context, GroupId, L"PDB file name", nameString->Buffer);
            PhDereferenceObject(nameString);
        }
    }
    else if (signature == CODEVIEW_SIGNATURE_NB10 && DataLength >= UFIELD_OFFSET(CODEVIEW_INFO_PDB20, PdbFileName))
    {
        PCODEVIEW_INFO_PDB20 codeview = Data;
        ULONG maximumLength = DataLength - UFIELD_OFFSET(CODEVIEW_INFO_PDB20, PdbFileName);
        SIZE_T nameLength = strnlen(codeview->PdbFileName, maximumLength);
        LARGE_INTEGER time;
        SYSTEMTIME systemTime;
        PPH_STRING nameString;

        PvpAddDebugGroupItem(Context, GroupId, L"Signature", L"NB10 (PDB 2.0)");

        RtlSecondsSince1970ToTime(codeview->Timestamp, &time);
        PhLargeIntegerToLocalSystemTime(&systemTime, &time);
        PvpAddDebugGroupItem(Context, GroupId, L"Timestamp", PhaFormatDateTime(&systemTime)->Buffer);
        PvpAddDebugGroupItem(Context, GroupId, L"PDB age", PhaFormatUInt64(codeview->Age, FALSE)->Buffer);

        if (nameLength && (nameString = PhConvertUtf8ToUtf16Ex(codeview->PdbFileName, nameLength)))
        {
            PvpAddDebugGroupItem(Context, GroupId, L"PDB file name", nameString->Buffer);
            PhDereferenceObject(nameString);
        }
    }
    else
    {
        PvpAddDebugGroupItem(Context, GroupId, L"Signature", PhaFormatString(L"0x%lx", signature)->Buffer);
    }
}

VOID PvpAddDebugVcFeatureItems(
    _In_ PPVP_PE_DEBUG_CONTEXT Context,
    _In_ LONG GroupId,
    _In_ PVOID Data,
    _In_ ULONG DataLength
    )
{
    PIMAGE_DEBUG_VC_FEATURE_ENTRY features = Data;

    if (DataLength < sizeof(IMAGE_DEBUG_VC_FEATURE_ENTRY))
        return;

    PvpAddDebugGroupItem(Context, GroupId, L"Pre-VC++ 11.0", PhaFormatUInt64(features->PreVCPlusPlusCount, TRUE)->Buffer);
    PvpAddDebugGroupItem(Context, GroupId, L"C/C++", PhaFormatUInt64(features->CAndCPlusPlusCount, TRUE)->Buffer);
    PvpAddDebugGroupItem(Context, GroupId, L"/GS", PhaFormatUInt64(features->GuardStackCount, TRUE)->Buffer);
    PvpAddDebugGroupItem(Context, GroupId, L"/sdl", PhaFormatUInt64(features->SdlCount, TRUE)->Buffer);
    PvpAddDebugGroupItem(Context, GroupId, L"guardN", PhaFormatUInt64(features->GuardCount, TRUE)->Buffer);
}

VOID PvpAddDebugReproItems(
    _In_ PPVP_PE_DEBUG_CONTEXT Context,
    _In_ LONG GroupId,
    _In_opt_ PVOID Data,
    _In_ ULONG DataLength
    )
{
    PH_MAPPED_IMAGE_REPRO repro;
    PPH_STRING string;

    if (DataLength == 0 || !Data)
    {
        PvpAddDebugGroupItem(Context, GroupId, L"Reproducible", L"Yes (timestamp is a hash)");
        return;
    }

    PvpAddDebugGroupItem(Context, GroupId, L"Reproducible", L"Yes");

    if (DataLength >= sizeof(ULONG))
    {
        ULONG hashLength = *(PULONG)Data;

        if (hashLength && hashLength <= DataLength - sizeof(ULONG))
        {
            PPH_STRING hashString;

            if (hashString = PhBufferToHexString(PTR_ADD_OFFSET(Data, sizeof(ULONG)), hashLength))
            {
                PvpAddDebugGroupItem(Context, GroupId, L"Build hash", hashString->Buffer);
                PhDereferenceObject(hashString);
            }
        }
    }

    if (!NT_SUCCESS(PhGetMappedImageReproHash(&PvMappedImage, &repro)) || !repro.StoredHashValid)
        return;

    PvpAddDebugGroupItem(
        Context,
        GroupId,
        L"Expected timestamp",
        PhaFormatString(
            L"0x%lx (%s)",
            repro.TimeDateStamp,
            repro.TimeStampValid ? L"matches" : L"mismatch"
            )->Buffer
        );

    if (string = PhFormatGuid(&repro.PdbGuid))
    {
        PvpAddDebugGroupItem(
            Context,
            GroupId,
            L"Expected PDB GUID",
            PhaFormatString(
                L"%s (%s)",
                string->Buffer,
                !repro.PdbSignaturePresent ? L"no CodeView entry" :
                repro.PdbSignatureValid ? L"matches" : L"mismatch"
                )->Buffer
            );
        PhDereferenceObject(string);
    }

    // The linker writes the stored hash and the PDB GUID after hashing, so this
    // recomputation assumes they were zero at the time and is informational only.

    if (repro.HashComputed && (string = PhBufferToHexString(repro.ComputedHash, PH_IMAGE_REPRO_HASH_SIZE)))
    {
        PvpAddDebugGroupItem(
            Context,
            GroupId,
            L"Recomputed hash",
            PhaFormatString(
                L"%s (best-effort, %s)",
                string->Buffer,
                repro.HashValid ? L"matches" : L"mismatch"
                )->Buffer
            );
        PhDereferenceObject(string);
    }
    else
    {
        PvpAddDebugGroupItem(Context, GroupId, L"Recomputed hash", L"Not available");
    }
}

VOID PvpAddDebugExDllCharacteristicsItems(
    _In_ PPVP_PE_DEBUG_CONTEXT Context,
    _In_ LONG GroupId,
    _In_ PVOID Data,
    _In_ ULONG DataLength
    )
{
    ULONG characteristics;
    PH_STRING_BUILDER stringBuilder;

    if (DataLength < sizeof(ULONG))
        return;

    characteristics = *(PULONG)Data;

    PhInitializeStringBuilder(&stringBuilder, 0x100);

    if (characteristics & IMAGE_DLLCHARACTERISTICS_EX_CET_COMPAT)
        PhAppendStringBuilder2(&stringBuilder, L"CET compatible, ");
    if (characteristics & IMAGE_DLLCHARACTERISTICS_EX_CET_COMPAT_STRICT_MODE)
        PhAppendStringBuilder2(&stringBuilder, L"CET strict mode, ");
    if (characteristics & IMAGE_DLLCHARACTERISTICS_EX_CET_SET_CONTEXT_IP_VALIDATION_RELAXED_MODE)
        PhAppendStringBuilder2(&stringBuilder, L"CET context IP validation relaxed mode, ");
    if (characteristics & IMAGE_DLLCHARACTERISTICS_EX_CET_DYNAMIC_APIS_ALLOW_IN_PROC)
        PhAppendStringBuilder2(&stringBuilder, L"CET dynamic APIs allowed in-proc, ");
    if (characteristics & IMAGE_DLLCHARACTERISTICS_EX_CET_RESERVED_1)
        PhAppendStringBuilder2(&stringBuilder, L"CET reserved 1, ");
    if (characteristics & IMAGE_DLLCHARACTERISTICS_EX_CET_RESERVED_2)
        PhAppendStringBuilder2(&stringBuilder, L"CET reserved 2, ");
    if (characteristics & IMAGE_DLLCHARACTERISTICS_EX_FORWARD_CFI_COMPAT)
        PhAppendStringBuilder2(&stringBuilder, L"Forward CFI compatible, ");
    if (characteristics & IMAGE_DLLCHARACTERISTICS_EX_HOTPATCH_COMPATIBLE)
        PhAppendStringBuilder2(&stringBuilder, L"Hotpatch compatible, ");

    if (PhEndsWithString2(stringBuilder.String, L", ", FALSE))
        PhRemoveEndStringBuilder(&stringBuilder, 2);

    PvpAddDebugGroupItem(Context, GroupId, L"Characteristics", PhaFormatString(L"0x%lx", characteristics)->Buffer);

    if (stringBuilder.String->Length)
        PvpAddDebugGroupItem(Context, GroupId, L"Flags", stringBuilder.String->Buffer);

    PhDeleteStringBuilder(&stringBuilder);
}

VOID PvpAddDebugPdbChecksumItems(
    _In_ PPVP_PE_DEBUG_CONTEXT Context,
    _In_ LONG GroupId,
    _In_ PVOID Data,
    _In_ ULONG DataLength
    )
{
    SIZE_T nameLength;
    ULONG checksumOffset;
    PPH_STRING nameString;
    PPH_STRING checksumString;

    nameLength = strnlen((PCSTR)Data, DataLength);

    if (nameLength == 0 || nameLength == DataLength)
        return;

    if (nameString = PhConvertUtf8ToUtf16Ex((PCSTR)Data, nameLength))
    {
        PvpAddDebugGroupItem(Context, GroupId, L"Algorithm", nameString->Buffer);
        PhDereferenceObject(nameString);
    }

    checksumOffset = (ULONG)nameLength + sizeof(ANSI_NULL);

    if (checksumOffset < DataLength)
    {
        if (checksumString = PhBufferToHexString(PTR_ADD_OFFSET(Data, checksumOffset), DataLength - checksumOffset))
        {
            PvpAddDebugGroupItem(Context, GroupId, L"Checksum", checksumString->Buffer);
            PhDereferenceObject(checksumString);
        }
    }
}

INT_PTR CALLBACK PvpPeDebugDlgProc(
    _In_ HWND hwndDlg,
    _In_ UINT uMsg,
    _In_ WPARAM wParam,
    _In_ LPARAM lParam
    )
{
    PPVP_PE_DEBUG_CONTEXT context;

    if (uMsg == WM_INITDIALOG)
    {
        context = PhAllocateZero(sizeof(PVP_PE_DEBUG_CONTEXT));
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
            PH_MAPPED_IMAGE_DEBUG debug;
            PH_IMAGE_DEBUG_ENTRY entry;
            ULONG count = 0;
            ULONG i;

            context->WindowHandle = hwndDlg;
            context->ListViewHandle = GetDlgItem(hwndDlg, IDC_LIST);

            PhSetListViewStyle(context->ListViewHandle, TRUE, TRUE);
            PvConfigListViewFont(hwndDlg, context->ListViewHandle);
            PhAddListViewColumn(context->ListViewHandle, 0, 0, 0, LVCFMT_LEFT, 220, L"Name");
            PhAddListViewColumn(context->ListViewHandle, 1, 1, 1, LVCFMT_LEFT, 300, L"Value");
            PhSetExtendedListView(context->ListViewHandle);
            ListView_EnableGroupView(context->ListViewHandle, TRUE);
            PhLoadListViewColumnsFromSetting(L"ImageDebugListViewGroupColumns", context->ListViewHandle);
            PvConfigTreeBorders(context->ListViewHandle);
            PvSetListViewImageList(context->WindowHandle, context->ListViewHandle);

            PhInitializeLayoutManager(&context->LayoutManager, hwndDlg);
            PhAddLayoutItem(&context->LayoutManager, context->ListViewHandle, NULL, PH_ANCHOR_ALL);

            if (NT_SUCCESS(PhGetMappedImageDebug(&PvMappedImage, &debug)))
            {
                ExtendedListView_SetRedraw(context->ListViewHandle, FALSE);

                for (i = 0; i < debug.NumberOfEntries; i++)
                {
                    INT groupId = (INT)i;
                    WCHAR value[PH_INT64_STR_LEN_1];
                    PVOID entryData;

                    entry = debug.DebugEntries[i];

                    PhAddListViewGroup(context->ListViewHandle, groupId, PhaFormatString(
                        L"#%lu %s",
                        ++count,
                        PvpGetDebugTypeString(entry.Type)
                        )->Buffer);

                    PhPrintPointer(value, UlongToPtr(entry.AddressOfRawData));
                    PvpAddDebugGroupItem(context, groupId, L"RVA (start)", value);
                    PhPrintPointer(value, UlongToPtr(UInt32Add32To64(entry.AddressOfRawData, entry.SizeOfData)));
                    PvpAddDebugGroupItem(context, groupId, L"RVA (end)", value);
                    PvpAddDebugGroupItem(context, groupId, L"Size", PhaFormatSize(entry.SizeOfData, ULONG_MAX)->Buffer);

                    entryData = PvpGetDebugEntryData(&entry);

                    if (entryData)
                    {
                        PPH_STRING hashString;

                        if (hashString = PvHashBuffer(entryData, entry.SizeOfData))
                        {
                            PvpAddDebugGroupItem(context, groupId, L"Hash", hashString->Buffer);
                            PhDereferenceObject(hashString);
                        }
                    }

                    switch (entry.Type)
                    {
                    case IMAGE_DEBUG_TYPE_CODEVIEW:
                        {
                            if (entryData)
                                PvpAddDebugCodeViewItems(context, groupId, entryData, entry.SizeOfData);
                        }
                        break;
                    case IMAGE_DEBUG_TYPE_VC_FEATURE:
                        {
                            if (entryData)
                                PvpAddDebugVcFeatureItems(context, groupId, entryData, entry.SizeOfData);
                        }
                        break;
                    case IMAGE_DEBUG_TYPE_REPRO:
                        {
                            PvpAddDebugReproItems(context, groupId, entryData, entryData ? entry.SizeOfData : 0);
                        }
                        break;
                    case IMAGE_DEBUG_TYPE_EX_DLLCHARACTERISTICS:
                        {
                            if (entryData)
                                PvpAddDebugExDllCharacteristicsItems(context, groupId, entryData, entry.SizeOfData);
                        }
                        break;
                    case IMAGE_DEBUG_TYPE_PDBCHECKSUM:
                        {
                            if (entryData)
                                PvpAddDebugPdbChecksumItems(context, groupId, entryData, entry.SizeOfData);
                        }
                        break;
                    }
                }

                ExtendedListView_SetRedraw(context->ListViewHandle, TRUE);

                PhFree(debug.DebugEntries);
            }

            // Must run before PvThemeApplyListView: phlib's initializer resets the
            // listview colors to its own palette. (dmex)
            PvThemeInitializePageDialog(hwndDlg, PhEnableThemeSupport);
            PvThemeApplyListView(context->ListViewHandle);
        }
        break;
    case WM_THEMECHANGED:
        {
            PvThemeApplyListView(context->ListViewHandle);
        }
        break;
    case WM_DESTROY:
        {
            PhSaveListViewColumnsToSetting(L"ImageDebugListViewGroupColumns", context->ListViewHandle);
            PhDeleteLayoutManager(&context->LayoutManager);
            PhRemoveWindowContext(hwndDlg, PH_WINDOW_CONTEXT_DEFAULT);
            PhFree(context);
        }
        break;
    case WM_DPICHANGED_AFTERPARENT:
        {
            PvSetListViewImageList(context->WindowHandle, context->ListViewHandle);
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
            PvHandleListViewNotifyForCopy(lParam, context->ListViewHandle);
        }
        break;
    case WM_CONTEXTMENU:
        {
            PvHandleListViewCommandCopy(hwndDlg, lParam, wParam, context->ListViewHandle);
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
