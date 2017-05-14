/*
 * Process Hacker -
 *   PE viewer
 *
 * Copyright (C) 2010-2011 wj32
 * Copyright (C) 2017 dmex
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
    UCHAR VersionString[1]; // dmex: added for convenience.
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
            PPH_STRING string;
            PH_STRING_BUILDER stringBuilder;
            PSTORAGESIGNATURE metaData;

            lvHandle = GetDlgItem(hwndDlg, IDC_LIST);
            PhSetListViewStyle(lvHandle, TRUE, TRUE);
            PhSetControlTheme(lvHandle, L"explorer");
            PhAddListViewColumn(lvHandle, 0, 0, 0, LVCFMT_LEFT, 80, L"Name");
            PhAddListViewColumn(lvHandle, 1, 1, 1, LVCFMT_LEFT, 80, L"VA");
            PhAddListViewColumn(lvHandle, 2, 2, 2, LVCFMT_LEFT, 80, L"Size");

            string = PhaFormatString(
                L"%u.%u", 
                PvImageCor20Header->MajorRuntimeVersion,
                PvImageCor20Header->MinorRuntimeVersion);
            SetDlgItemText(hwndDlg, IDC_RUNTIMEVERSION, string->Buffer);

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

            SetDlgItemText(hwndDlg, IDC_FLAGS, stringBuilder.String->Buffer);
            PhDeleteStringBuilder(&stringBuilder);

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

            if (metaData && metaData->VersionLength != 0)
            {
                string = PhZeroExtendToUtf16Ex((PCHAR)metaData->VersionString, metaData->VersionLength);
                SetDlgItemText(hwndDlg, IDC_VERSIONSTRING, string->Buffer);
                PhDereferenceObject(string);

                {
                    PSTORAGEHEADER storageHeader = PTR_ADD_OFFSET(metaData, (sizeof(STORAGESIGNATURE) - sizeof(UCHAR)) + metaData->VersionLength);
                    PSTORAGESTREAM streamHeader = PTR_ADD_OFFSET(storageHeader, sizeof(STORAGEHEADER));

                    for (USHORT i = 0; i < storageHeader->Streams; i++)
                    {
                        INT lvItemIndex;
                        WCHAR sectionName[65];
                        WCHAR pointer[PH_PTR_STR_LEN_1];

                        if (PhCopyStringZFromBytes(
                            streamHeader->Name, 
                            sizeof(streamHeader->Name), 
                            sectionName, 
                            ARRAYSIZE(sectionName),
                            NULL
                            ))
                        {
                            lvItemIndex = PhAddListViewItem(lvHandle, MAXINT, sectionName, NULL);

                            PhPrintPointer(pointer, UlongToPtr(streamHeader->Offset));

                            PhSetListViewSubItem(lvHandle, lvItemIndex, 1, pointer);
                            PhSetListViewSubItem(lvHandle, lvItemIndex, 2, PhaFormatSize(streamHeader->Size, -1)->Buffer);
                        }

                        // Stream headers don't have fixed sizes...
                        // The size is aligned up based on a variable length string at the end.
                        streamHeader = PTR_ADD_OFFSET(streamHeader, ALIGN_UP(FIELD_OFFSET(STORAGESTREAM, Name) + strlen(streamHeader->Name) + 1, 4));
                    }
                }
            }
            else
            {
                SetDlgItemText(hwndDlg, IDC_VERSIONSTRING, L"N/A");
            }
        }
        break;
    case WM_SHOWWINDOW:
        {
            if (!propPageContext->LayoutInitialized)
            {
                PPH_LAYOUT_ITEM dialogItem;

                dialogItem = PvAddPropPageLayoutItem(hwndDlg, hwndDlg,
                    PH_PROP_PAGE_TAB_CONTROL_PARENT, PH_ANCHOR_ALL);
                PvAddPropPageLayoutItem(hwndDlg, GetDlgItem(hwndDlg, IDC_LIST),
                    dialogItem, PH_ANCHOR_ALL);

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
        }
        break;
    }

    return FALSE;
}