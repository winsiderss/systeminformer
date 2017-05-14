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
            PH_STRING_BUILDER stringBuilder;
            PPH_STRING string;
            PVOID metaData;
            ULONG versionStringLength;

            string = PhaFormatString(L"%u.%u", PvImageCor20Header->MajorRuntimeVersion,
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

            versionStringLength = 0;

            if (metaData)
            {
                // Skip 12 bytes.
                // First 4 bytes contains the length of the version string.
                // The version string follows.
                versionStringLength = *(PULONG)((PCHAR)metaData + 12);

                // Make sure the length is valid.
                if (versionStringLength >= 0x100)
                    versionStringLength = 0;
            }

            if (versionStringLength != 0)
            {
                string = PhZeroExtendToUtf16Ex((PCHAR)metaData + 12 + 4, versionStringLength);
                SetDlgItemText(hwndDlg, IDC_VERSIONSTRING, string->Buffer);
                PhDereferenceObject(string);
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
                PvAddPropPageLayoutItem(hwndDlg, hwndDlg,
                    PH_PROP_PAGE_TAB_CONTROL_PARENT, PH_ANCHOR_ALL);

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