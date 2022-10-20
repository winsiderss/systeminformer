/*
 * Copyright (c) 2022 Winsider Seminars & Solutions, Inc.  All rights reserved.
 *
 * This file is part of System Informer.
 *
 * Authors:
 *
 *     wj32    2010-2011
 *     dmex    2017-2022
 *
 */

#include <peview.h>

#ifdef __has_include
#if __has_include (<metahost.h>)
#include <metahost.h>
#else
#include "metahost/metahost.h"
#endif
#else
#include "metahost/metahost.h"
#endif

typedef struct _PVP_PE_CLR_CONTEXT
{
    HWND WindowHandle;
    HWND ListViewHandle;
    PH_LAYOUT_MANAGER LayoutManager;
    PPV_PROPPAGECONTEXT PropSheetContext;
    PVOID PdbMetadataAddress;
} PVP_PE_CLR_CONTEXT, *PPVP_PE_CLR_CONTEXT;

// CLR structure reference:
// https://github.com/dotnet/coreclr/blob/master/src/md/inc/mdfileformat.h
// https://github.com/dotnet/coreclr/blob/master/src/utilcode/pedecoder.cpp
// https://github.com/dotnet/coreclr/blob/master/src/debug/daccess/nidump.cpp

#define STORAGE_MAGIC_SIG   0x424A5342  // BSJB

#include <pshpack1.h>
typedef struct _STORAGESIGNATURE
{
    ULONG Signature;
    USHORT MajorVersion;
    USHORT MinorVersion;
    ULONG ExtraData;        // Offset to next structure of information
    ULONG VersionLength;    // Length of version string
    //UCHAR VersionString[1];
} STORAGESIGNATURE, *PSTORAGESIGNATURE;

typedef struct _STORAGEHEADER
{
    UCHAR Flags;     // STGHDR_xxx flags.
    UCHAR Reserved;
    USHORT Streams; // How many streams are there.
} STORAGEHEADER, *PSTORAGEHEADER;

typedef struct _STORAGESTREAM
{
    ULONG Offset;  // Offset in file for this stream.
    ULONG Size;    // Size of the file.
    CHAR Name[32]; // Start of name, null terminated.
} STORAGESTREAM, *PSTORAGESTREAM;

typedef struct _MDSTREAMHEADER
{
    ULONG Reserved;
    UCHAR Major;
    UCHAR Minor;
    UCHAR Heaps;
    UCHAR Rid;
    ULONGLONG MaskValid;
    ULONGLONG Sorted;
} MDSTREAMHEADER, *PMDSTREAMHEADER;
#include <poppack.h>

PSTORAGESIGNATURE PvpPeGetClrMetaDataHeader(
    _In_opt_ PVOID PdbMetadataAddress
    )
{
    PSTORAGESIGNATURE metaData;

    if (PdbMetadataAddress)
    {
        metaData = PdbMetadataAddress;
    }
    else
    {
        metaData = PhMappedImageRvaToVa(&PvMappedImage, PvImageCor20Header->MetaData.VirtualAddress, NULL);

        if (metaData)
        {
            __try
            {
                PhProbeAddress(metaData, PvImageCor20Header->MetaData.Size, PvMappedImage.ViewBase, PvMappedImage.Size, 4);
            }
            __except (EXCEPTION_EXECUTE_HANDLER)
            {
                metaData = NULL;
            }
        }
    }

    return metaData;
}

PPH_STRING PvpPeGetClrFlagsText(
    VOID
    )
{
    PH_STRING_BUILDER stringBuilder;

    PhInitializeStringBuilder(&stringBuilder, 256);

    if (PvImageCor20Header->Flags & COMIMAGE_FLAGS_ILONLY)
        PhAppendStringBuilder2(&stringBuilder, L"IL only, ");
    if (PvImageCor20Header->Flags & COMIMAGE_FLAGS_32BITREQUIRED)
        PhAppendStringBuilder2(&stringBuilder, L"32-bit only, ");
    if (PvImageCor20Header->Flags & COMIMAGE_FLAGS_32BITPREFERRED)
        PhAppendStringBuilder2(&stringBuilder, L"32-bit preferred, ");
    if (PvImageCor20Header->Flags & COMIMAGE_FLAGS_IL_LIBRARY)
        PhAppendStringBuilder2(&stringBuilder, L"IL library, ");

    if (PvImageCor20Header->StrongNameSignature.VirtualAddress != 0 && PvImageCor20Header->StrongNameSignature.Size != 0)
    {
        if (PvImageCor20Header->Flags & COMIMAGE_FLAGS_STRONGNAMESIGNED)
            PhAppendStringBuilder2(&stringBuilder, L"Strong-name signed, ");
        else
            PhAppendStringBuilder2(&stringBuilder, L"Strong-name delay signed, ");
    }

    if (PvImageCor20Header->Flags & COMIMAGE_FLAGS_NATIVE_ENTRYPOINT)
        PhAppendStringBuilder2(&stringBuilder, L"Native entry-point, ");
    if (PvImageCor20Header->Flags & COMIMAGE_FLAGS_TRACKDEBUGDATA)
        PhAppendStringBuilder2(&stringBuilder, L"Track debug data, ");

    if (PhEndsWithString2(stringBuilder.String, L", ", FALSE))
        PhRemoveEndStringBuilder(&stringBuilder, 2);

    return PhFinalStringBuilderString(&stringBuilder);
}

PPH_STRING PvpPeGetClrVersionText(
    VOID
    )
{
    return PhFormatString(
        L"%hu.%hu",
        PvImageCor20Header->MajorRuntimeVersion,
        PvImageCor20Header->MinorRuntimeVersion
        );
}

PPH_STRING PvpPeGetClrStorageVersionText(
    _In_ PSTORAGESIGNATURE ClrMetaData
    )
{
    if (ClrMetaData && ClrMetaData->VersionLength != 0)
    {
        return PhZeroExtendToUtf16Ex(
            PTR_ADD_OFFSET(ClrMetaData, RTL_SIZEOF_THROUGH_FIELD(STORAGESIGNATURE, VersionLength)),
            ClrMetaData->VersionLength
            );
    }

    return PhCreateString(L"N/A");
}

PPH_STRING PvpPeClrGetMvid(
    _In_ PSTORAGESIGNATURE ClrMetaData
    )
{
    PPH_STRING guidMvidString = NULL;
    PSTORAGEHEADER storageHeader;
    PSTORAGESTREAM streamHeader;
    USHORT i;

    storageHeader = PTR_ADD_OFFSET(ClrMetaData, sizeof(STORAGESIGNATURE) + ClrMetaData->VersionLength);
    streamHeader = PTR_ADD_OFFSET(storageHeader, sizeof(STORAGEHEADER));

    for (i = 0; i < storageHeader->Streams; i++)
    {
        if (PhEqualBytesZ(streamHeader->Name, "#GUID", TRUE))
        {
            if (NT_SUCCESS(PhGetMappedImageDebugEntryByType(
                &PvMappedImage,
                IMAGE_DEBUG_TYPE_REPRO,
                NULL,
                NULL
                )))
            {
                PPH_STRING string;
                PPH_STRING hash;

                // The MVID is a hash of both the file and the PDB file for repro images. (dmex)
                string = PhFormatGuid(PTR_ADD_OFFSET(ClrMetaData, streamHeader->Offset));
                hash = PhBufferToHexStringEx(PTR_ADD_OFFSET(ClrMetaData, streamHeader->Offset), sizeof(GUID), FALSE);

                guidMvidString = PhFormatString(
                    L"%s (%s) (deterministic)",
                    PhGetStringOrEmpty(string),
                    PhGetStringOrEmpty(hash)
                    );

                PhDereferenceObject(string);
                PhDereferenceObject(hash);
            }
            else
            {
                guidMvidString = PhFormatGuid(PTR_ADD_OFFSET(ClrMetaData, streamHeader->Offset));
            }
            break;
        }

        streamHeader = PTR_ADD_OFFSET(streamHeader, ALIGN_UP(UFIELD_OFFSET(STORAGESTREAM, Name) + strlen(streamHeader->Name) + sizeof(ANSI_NULL), ULONG));
    }


    return guidMvidString;
}

VOID PvpPeClrEnumSections(
    _In_ PSTORAGESIGNATURE ClrMetaData,
    _In_ HWND ListViewHandle
    )
{
    PSTORAGEHEADER storageHeader;
    PSTORAGESTREAM streamHeader;
    ULONG count = 0;
    USHORT i;

    storageHeader = PTR_ADD_OFFSET(ClrMetaData, sizeof(STORAGESIGNATURE) + ClrMetaData->VersionLength);
    streamHeader = PTR_ADD_OFFSET(storageHeader, sizeof(STORAGEHEADER));

    for (i = 0; i < storageHeader->Streams; i++)
    {
        INT lvItemIndex;
        WCHAR sectionName[65];
        WCHAR value[PH_INT64_STR_LEN_1];

        PhPrintUInt32(value, ++count);
        lvItemIndex = PhAddListViewItem(ListViewHandle, MAXINT, value, NULL);

        if (PhCopyStringZFromBytes(
            streamHeader->Name,
            sizeof(streamHeader->Name),
            sectionName,
            RTL_NUMBER_OF(sectionName),
            NULL
            ))
        {
            PhSetListViewSubItem(ListViewHandle, lvItemIndex, 1, sectionName);
        }

        PhPrintPointer(value, UlongToPtr(streamHeader->Offset));
        PhSetListViewSubItem(ListViewHandle, lvItemIndex, 2, value);
        PhPrintPointer(value, PTR_ADD_OFFSET(streamHeader->Offset, streamHeader->Size));
        PhSetListViewSubItem(ListViewHandle, lvItemIndex, 3, value);
        PhSetListViewSubItem(ListViewHandle, lvItemIndex, 4, PhaFormatSize(streamHeader->Size, ULONG_MAX)->Buffer);

        if (streamHeader->Offset && streamHeader->Size)
        {
            __try
            {
                PH_HASH_CONTEXT hashContext;
                PPH_STRING hashString;
                UCHAR hash[32];

                PhInitializeHash(&hashContext, Md5HashAlgorithm); // PhGetIntegerSetting(L"HashAlgorithm")
                PhUpdateHash(&hashContext, PTR_ADD_OFFSET(ClrMetaData, streamHeader->Offset), streamHeader->Size);

                if (PhFinalHash(&hashContext, hash, 16, NULL))
                {
                    hashString = PhBufferToHexString(hash, 16);
                    PhSetListViewSubItem(ListViewHandle, lvItemIndex, 5, hashString->Buffer);
                    PhDereferenceObject(hashString);
                }
            }
            __except (EXCEPTION_EXECUTE_HANDLER)
            {
                PPH_STRING message;

                //message = PH_AUTO(PhGetNtMessage(GetExceptionCode()));
                message = PH_AUTO(PhGetWin32Message(PhNtStatusToDosError(GetExceptionCode()))); // WIN32_FROM_NTSTATUS

                PhSetListViewSubItem(ListViewHandle, lvItemIndex, 5, PhGetStringOrEmpty(message));
            }
        }

        // CLR stream headers don't have fixed sizes.
        // The size is aligned up based on a variable length string at the end. (dmex)
        streamHeader = PTR_ADD_OFFSET(streamHeader, ALIGN_UP(UFIELD_OFFSET(STORAGESTREAM, Name) + strlen(streamHeader->Name) + 1, ULONG));
    }
}

VOID PvpGetClrStrongNameToken(
    _In_ HWND WindowHandle
    )
{
    PPH_STRING clrStrongNameString = NULL;
    ICLRMetaHost* clrMetaHost = NULL;
    ICLRRuntimeInfo* clrRuntimeInfo = NULL;
    ICLRStrongName* clrStrongName = NULL;
    CLRCreateInstanceFnPtr CLRCreateInstance_I = NULL;
    PVOID mscoreeHandle = NULL;
    ULONG clrTokenLength = 0;
    ULONG clrKeyLength = 0;
    PBYTE clrToken = NULL;
    PBYTE clrKey = NULL;
    ULONG size = MAX_PATH;
    WCHAR version[MAX_PATH] = L"";

    if (mscoreeHandle = PhLoadLibrary(L"mscoree.dll"))
    {
        if (CLRCreateInstance_I = PhGetDllBaseProcedureAddress(mscoreeHandle, "CLRCreateInstance", 0))
        {
            if (!SUCCEEDED(CLRCreateInstance_I(
                &CLSID_CLRMetaHost,
                &IID_ICLRMetaHost,
                &clrMetaHost
                )))
            {
                goto CleanupExit;
            }
        }
        else
        {
            goto CleanupExit;
        }
    }
    else
    {
        goto CleanupExit;
    }

    if (!SUCCEEDED(ICLRMetaHost_GetVersionFromFile(
        clrMetaHost,
        PvFileName->Buffer,
        version,
        &size
        )))
    {
        goto CleanupExit;
    }

    if (!SUCCEEDED(ICLRMetaHost_GetRuntime(
        clrMetaHost,
        version,
        &IID_ICLRRuntimeInfo,
        &clrRuntimeInfo
        )))
    {
        goto CleanupExit;
    }

    if (!SUCCEEDED(ICLRRuntimeInfo_GetInterface(
        clrRuntimeInfo,
        &CLSID_CLRStrongName,
        &IID_ICLRStrongName,
        &clrStrongName
        )))
    {
        goto CleanupExit;
    }

    if (!SUCCEEDED(ICLRStrongName_StrongNameTokenFromAssembly(
        clrStrongName,
        PvFileName->Buffer,
        &clrToken,
        &clrTokenLength
        )))
    {
        goto CleanupExit;
    }

    //if (!SUCCEEDED(ICLRStrongName_StrongNameTokenFromAssemblyEx(
    //    clrStrongName,
    //    PvFileName->Buffer,
    //    &clrToken,
    //    &clrTokenLength,
    //    &clrKey,
    //    &clrKeyLength
    //    )))
    //{
    //    __leave;
    //}

CleanupExit:

    if (clrTokenLength)
    {
        if (clrStrongNameString = PhBufferToHexStringEx(clrToken, clrTokenLength, FALSE))
        {
            PhSetDialogItemText(WindowHandle, IDC_TOKENSTRING, clrStrongNameString->Buffer);
            PhDereferenceObject(clrStrongNameString);
        }
    }

    if (clrStrongName)
    {
        if (clrToken)
            ICLRStrongName_StrongNameFreeBuffer(clrStrongName, clrToken);

        ICLRStrongName_Release(clrStrongName);
    }

    if (clrRuntimeInfo)
        ICLRRuntimeInfo_Release(clrRuntimeInfo);
    if (clrMetaHost)
        ICLRMetaHost_Release(clrMetaHost);
    if (mscoreeHandle)
        FreeLibrary(mscoreeHandle);
}

INT_PTR CALLBACK PvpPeClrDlgProc(
    _In_ HWND hwndDlg,
    _In_ UINT uMsg,
    _In_ WPARAM wParam,
    _In_ LPARAM lParam
    )
{
    PPVP_PE_CLR_CONTEXT context;

    if (uMsg == WM_INITDIALOG)
    {
        context = PhAllocateZero(sizeof(PVP_PE_CLR_CONTEXT));
        PhSetWindowContext(hwndDlg, PH_WINDOW_CONTEXT_DEFAULT, context);

        if (lParam)
        {
            LPPROPSHEETPAGE propSheetPage = (LPPROPSHEETPAGE)lParam;
            context->PropSheetContext = (PPV_PROPPAGECONTEXT)propSheetPage->lParam;
            context->PdbMetadataAddress = context->PropSheetContext->Context;
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
            PSTORAGESIGNATURE clrMetaData;

            context->WindowHandle = hwndDlg;
            context->ListViewHandle = GetDlgItem(hwndDlg, IDC_LIST);

            PhSetListViewStyle(context->ListViewHandle, TRUE, TRUE);
            PhSetControlTheme(context->ListViewHandle, L"explorer");
            PhAddListViewColumn(context->ListViewHandle, 0, 0, 0, LVCFMT_LEFT, 40, L"#");
            PhAddListViewColumn(context->ListViewHandle, 1, 1, 1, LVCFMT_LEFT, 80, L"Name");
            PhAddListViewColumn(context->ListViewHandle, 2, 2, 2, LVCFMT_LEFT, 100, L"RVA (start)");
            PhAddListViewColumn(context->ListViewHandle, 3, 3, 3, LVCFMT_LEFT, 100, L"RVA (end)");
            PhAddListViewColumn(context->ListViewHandle, 4, 4, 4, LVCFMT_LEFT, 100, L"Size");
            PhAddListViewColumn(context->ListViewHandle, 5, 5, 5, LVCFMT_LEFT, 80, L"Hash");
            PhSetExtendedListView(context->ListViewHandle);
            PhLoadListViewColumnsFromSetting(L"ImageClrListViewColumns", context->ListViewHandle);
            PvSetListViewImageList(context->WindowHandle, context->ListViewHandle);

            PhInitializeLayoutManager(&context->LayoutManager, hwndDlg);
            PhAddLayoutItem(&context->LayoutManager, GetDlgItem(hwndDlg, IDC_CLRGROUP), NULL, PH_ANCHOR_LEFT | PH_ANCHOR_TOP | PH_ANCHOR_RIGHT);
            PhAddLayoutItem(&context->LayoutManager, GetDlgItem(hwndDlg, IDC_FLAGS), NULL, PH_ANCHOR_LEFT | PH_ANCHOR_TOP | PH_ANCHOR_RIGHT);
            PhAddLayoutItem(&context->LayoutManager, GetDlgItem(hwndDlg, IDC_MVIDSTRING), NULL, PH_ANCHOR_LEFT | PH_ANCHOR_TOP | PH_ANCHOR_RIGHT);
            PhAddLayoutItem(&context->LayoutManager, GetDlgItem(hwndDlg, IDC_TOKENSTRING), NULL, PH_ANCHOR_LEFT | PH_ANCHOR_TOP | PH_ANCHOR_RIGHT);
            PhAddLayoutItem(&context->LayoutManager, context->ListViewHandle, NULL, PH_ANCHOR_ALL);

            if (!context->PdbMetadataAddress)
            {
                PhSetDialogItemText(hwndDlg, IDC_RUNTIMEVERSION, PH_AUTO_T(PH_STRING, PvpPeGetClrVersionText())->Buffer);
                PhSetDialogItemText(hwndDlg, IDC_FLAGS, PH_AUTO_T(PH_STRING, PvpPeGetClrFlagsText())->Buffer);
            }
            else
            {
                PhSetDialogItemText(hwndDlg, IDC_RUNTIMEVERSION, L"");
                PhSetDialogItemText(hwndDlg, IDC_FLAGS, L"");
            }

            if (clrMetaData = PvpPeGetClrMetaDataHeader(context->PdbMetadataAddress))
            {
                PhSetDialogItemText(hwndDlg, IDC_VERSIONSTRING, PH_AUTO_T(PH_STRING, PvpPeGetClrStorageVersionText(clrMetaData))->Buffer);

                if (!context->PdbMetadataAddress)
                    PhSetDialogItemText(hwndDlg, IDC_MVIDSTRING, PH_AUTO_T(PH_STRING, PvpPeClrGetMvid(clrMetaData))->Buffer);
                else
                    PhSetDialogItemText(hwndDlg, IDC_MVIDSTRING, L"");

                PvpGetClrStrongNameToken(hwndDlg);

                PvpPeClrEnumSections(clrMetaData, context->ListViewHandle);
            }

            PhInitializeWindowTheme(hwndDlg, PeEnableThemeSupport);
        }
        break;
    case WM_DESTROY:
        {
            PhSaveListViewColumnsToSetting(L"ImageClrListViewColumns", context->ListViewHandle);
            PhDeleteLayoutManager(&context->LayoutManager);
            PhFree(context);
        }
        break;
    case WM_DPICHANGED:
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
            LPNMHDR header = (LPNMHDR)lParam;

            switch (header->code)
            {
            case PSN_QUERYINITIALFOCUS:
                SetWindowLongPtr(hwndDlg, DWLP_MSGRESULT, (LONG_PTR)GetDlgItem(hwndDlg, IDC_RUNTIMEVERSION));
                return TRUE;
            }

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
            return (INT_PTR)GetStockBrush(DC_BRUSH);
        }
        break;
    }

    return FALSE;
}
