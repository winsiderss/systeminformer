/*
 * Process Hacker -
 *   PE viewer
 *
 * Copyright (C) 2019 dmex
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

VOID PvpSetRichEditText(
    _In_ HWND WindowHandle,
    _In_ PWSTR Text
    )
{
    SetFocus(WindowHandle);
    SendMessage(WindowHandle, WM_SETREDRAW, FALSE, 0);
    //SendMessage(WindowHandle, EM_SETSEL, 0, -1); // -2
    SendMessage(WindowHandle, EM_REPLACESEL, FALSE, (LPARAM)Text);
    SendMessage(WindowHandle, WM_VSCROLL, SB_TOP, 0); // requires SetFocus()    
    SendMessage(WindowHandle, WM_SETREDRAW, TRUE, 0);

    PostMessage(WindowHandle, EM_SETSEL, -1, 0);

    InvalidateRect(WindowHandle, NULL, FALSE);
}

VOID PvpShowFilePreview(
    _In_ HWND WindowHandle
    )
{
    PPH_STRING fileText;
    PH_STRING_BUILDER sb;

    if (fileText = PhFileReadAllText(PvFileName->Buffer, TRUE))
    {
        PhInitializeStringBuilder(&sb, 0x100);

        for (SIZE_T i = 0; i < fileText->Length / sizeof(WCHAR); i++)
        {
            if (iswprint(fileText->Buffer[i]))
            {
                PhAppendCharStringBuilder(&sb, fileText->Buffer[i]);
            }
            else
            {
                PhAppendCharStringBuilder(&sb, L' ');
            }
        }

        PhMoveReference(&fileText, PhFinalStringBuilderString(&sb));

        PvpSetRichEditText(GetDlgItem(WindowHandle, IDC_PREVIEW), fileText->Buffer);

        PhDereferenceObject(fileText);
    }
}

INT_PTR CALLBACK PvpPePreviewDlgProc(
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
            SendMessage(GetDlgItem(hwndDlg, IDC_PREVIEW), EM_SETLIMITTEXT, ULONG_MAX, 0);

            PvpShowFilePreview(hwndDlg);
            
            PhInitializeWindowTheme(hwndDlg, PeEnableThemeSupport);
        }
        break;
    case WM_DESTROY:
        {

        }
        break;
    case WM_SHOWWINDOW:
        {
            if (!propPageContext->LayoutInitialized)
            {
                PPH_LAYOUT_ITEM dialogItem;

                dialogItem = PvAddPropPageLayoutItem(hwndDlg, hwndDlg, PH_PROP_PAGE_TAB_CONTROL_PARENT, PH_ANCHOR_ALL);
                PvAddPropPageLayoutItem(hwndDlg, GetDlgItem(hwndDlg, IDC_PREVIEW), dialogItem, PH_ANCHOR_ALL);
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
                    SetWindowLongPtr(hwndDlg, DWLP_MSGRESULT, (LONG_PTR)GetDlgItem(hwndDlg, IDC_PREVIEW));
                }
                return TRUE;
            }
        }
        break;
    }

    return FALSE;
}
