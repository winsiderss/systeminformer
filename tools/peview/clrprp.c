/*
 * Process Hacker -
 *   PE viewer
 *
 * Copyright (C) 2010-2011 wj32
 * Copyright (C) 2017-2019 dmex
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
#include "metahost.h"

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
    BYTE Flags;     // STGHDR_xxx flags.
    BYTE Reserved;
    USHORT Streams; // How many streams are there.
} STORAGEHEADER, *PSTORAGEHEADER;

typedef struct _STORAGESTREAM
{
    ULONG Offset;  // Offset in file for this stream.
    ULONG Size;    // Size of the file.
    CHAR Name[32]; // Start of name, null terminated.
} STORAGESTREAM, *PSTORAGESTREAM;
#include <poppack.h>

PSTORAGESIGNATURE PvpPeGetClrMetaDataHeader(
    VOID
    )
{
    PSTORAGESIGNATURE metaData;

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
            guidMvidString = PhFormatGuid(PTR_ADD_OFFSET(ClrMetaData, streamHeader->Offset));
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
    USHORT i;

    storageHeader = PTR_ADD_OFFSET(ClrMetaData, sizeof(STORAGESIGNATURE) + ClrMetaData->VersionLength);
    streamHeader = PTR_ADD_OFFSET(storageHeader, sizeof(STORAGEHEADER));

    for (i = 0; i < storageHeader->Streams; i++)
    {
        INT lvItemIndex;
        WCHAR sectionName[65];
        WCHAR pointer[PH_PTR_STR_LEN_1];

        if (PhCopyStringZFromBytes(
            streamHeader->Name,
            sizeof(streamHeader->Name),
            sectionName,
            RTL_NUMBER_OF(sectionName),
            NULL
            ))
        {
            lvItemIndex = PhAddListViewItem(ListViewHandle, MAXINT, sectionName, NULL);
            PhPrintPointer(pointer, UlongToPtr(streamHeader->Offset));
            PhSetListViewSubItem(ListViewHandle, lvItemIndex, 1, pointer);
            PhSetListViewSubItem(ListViewHandle, lvItemIndex, 2, PhaFormatSize(streamHeader->Size, ULONG_MAX)->Buffer);
        }

        // Stream headers don't have fixed sizes...
        // The size is aligned up based on a variable length string at the end.
        streamHeader = PTR_ADD_OFFSET(streamHeader, ALIGN_UP(UFIELD_OFFSET(STORAGESTREAM, Name) + strlen(streamHeader->Name) + 1, ULONG));
    }
}

VOID PvpGetClrStrongNameToken(
    _In_ HWND WindowHandle
    )
{
    PPH_STRING clrStrongNameString = NULL;
    ICLRMetaHost* clrMetaHost = NULL;
    ICLRRuntimeInfo* clrRuntimInfo = NULL;
    ICLRStrongName* clrStrongName = NULL;
    CLRCreateInstanceFnPtr CLRCreateInstance_I = NULL;
    PVOID mscoreeHandle = NULL;
    ULONG clrTokenLength = 0;
    ULONG clrKeyLength = 0;
    PBYTE clrToken = NULL;
    PBYTE clrKey = NULL;
    ULONG size = MAX_PATH;
    WCHAR version[MAX_PATH] = L"";

    if (mscoreeHandle = LoadLibrary(L"mscoree.dll"))
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
        &clrRuntimInfo
        )))
    {
        goto CleanupExit;
    }

    if (!SUCCEEDED(ICLRRuntimeInfo_GetInterface(
        clrRuntimInfo,
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

    if (clrRuntimInfo)
        ICLRRuntimeInfo_Release(clrRuntimInfo);
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
    LPPROPSHEETPAGE propSheetPage;
    PPV_PROPPAGECONTEXT propPageContext;

    if (!PvPropPageDlgProcHeader(hwndDlg, uMsg, lParam, &propSheetPage, &propPageContext))
        return FALSE;

    switch (uMsg)
    {
    case WM_INITDIALOG:
        {
            HWND lvHandle;
            PSTORAGESIGNATURE clrMetaData;

            lvHandle = GetDlgItem(hwndDlg, IDC_LIST);
            PhSetListViewStyle(lvHandle, TRUE, TRUE);
            PhSetControlTheme(lvHandle, L"explorer");
            PhAddListViewColumn(lvHandle, 0, 0, 0, LVCFMT_LEFT, 80, L"Name");
            PhAddListViewColumn(lvHandle, 1, 1, 1, LVCFMT_LEFT, 80, L"VA");
            PhAddListViewColumn(lvHandle, 2, 2, 2, LVCFMT_LEFT, 80, L"Size");

            PhSetDialogItemText(hwndDlg, IDC_RUNTIMEVERSION, PH_AUTO_T(PH_STRING, PvpPeGetClrVersionText())->Buffer);
            PhSetDialogItemText(hwndDlg, IDC_FLAGS, PH_AUTO_T(PH_STRING, PvpPeGetClrFlagsText())->Buffer);

            if (clrMetaData = PvpPeGetClrMetaDataHeader())
            {
                PhSetDialogItemText(hwndDlg, IDC_VERSIONSTRING, PH_AUTO_T(PH_STRING, PvpPeGetClrStorageVersionText(clrMetaData))->Buffer);
                PhSetDialogItemText(hwndDlg, IDC_MVIDSTRING, PH_AUTO_T(PH_STRING, PvpPeClrGetMvid(clrMetaData))->Buffer);

                PvpGetClrStrongNameToken(hwndDlg);

                PvpPeClrEnumSections(clrMetaData, lvHandle);
            }

            PhInitializeWindowTheme(hwndDlg, PeEnableThemeSupport);
        }
        break;
    case WM_SHOWWINDOW:
        {
            if (!propPageContext->LayoutInitialized)
            {
                PPH_LAYOUT_ITEM dialogItem;

                dialogItem = PvAddPropPageLayoutItem(hwndDlg, hwndDlg, PH_PROP_PAGE_TAB_CONTROL_PARENT, PH_ANCHOR_ALL);
                PvAddPropPageLayoutItem(hwndDlg, GetDlgItem(hwndDlg, IDC_LIST), dialogItem, PH_ANCHOR_ALL);
                PvDoPropPageLayout(hwndDlg);

                propPageContext->LayoutInitialized = TRUE;
            }
        }
        break;
    case WM_NOTIFY:
        {
            LPNMHDR header = (LPNMHDR)lParam;

            switch (header->code)
            {
            case PSN_QUERYINITIALFOCUS:
                {
                    SetWindowLongPtr(hwndDlg, DWLP_MSGRESULT, (LONG_PTR)GetDlgItem(hwndDlg, IDC_RUNTIMEVERSION));
                }
                return TRUE;
            }

            PvHandleListViewNotifyForCopy(lParam, GetDlgItem(hwndDlg, IDC_LIST));
        }
        break;
    case WM_CONTEXTMENU:
        {
            PvHandleListViewCommandCopy(hwndDlg, lParam, wParam, GetDlgItem(hwndDlg, IDC_LIST));
        }
        break;
    }

    return FALSE;
}
