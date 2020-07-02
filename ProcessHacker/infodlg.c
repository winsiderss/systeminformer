/*
 * Process Hacker -
 *   information dialog
 *
 * Copyright (C) 2010 wj32
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

#include <phapp.h>
#include <phsettings.h>

typedef struct _INFORMATION_CONTEXT
{
    PWSTR String;
    ULONG Flags;
    PH_LAYOUT_MANAGER LayoutManager;
} INFORMATION_CONTEXT, *PINFORMATION_CONTEXT;

static RECT MinimumSize = { -1, -1, -1, -1 };

static INT_PTR CALLBACK PhpInformationDlgProc(
    _In_ HWND hwndDlg,
    _In_ UINT uMsg,
    _In_ WPARAM wParam,
    _In_ LPARAM lParam
    )
{
    PINFORMATION_CONTEXT context;

    if (uMsg == WM_INITDIALOG)
    {
        context = (PINFORMATION_CONTEXT)lParam;
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

            PhCenterWindow(hwndDlg, GetParent(hwndDlg));

            PhInitializeLayoutManager(&context->LayoutManager, hwndDlg);
            PhAddLayoutItem(&context->LayoutManager, GetDlgItem(hwndDlg, IDC_TEXT), NULL, PH_ANCHOR_ALL);
            PhAddLayoutItem(&context->LayoutManager, GetDlgItem(hwndDlg, IDOK), NULL, PH_ANCHOR_RIGHT | PH_ANCHOR_BOTTOM);
            PhAddLayoutItem(&context->LayoutManager, GetDlgItem(hwndDlg, IDC_COPY), NULL, PH_ANCHOR_RIGHT | PH_ANCHOR_BOTTOM);
            PhAddLayoutItem(&context->LayoutManager, GetDlgItem(hwndDlg, IDC_SAVE), NULL, PH_ANCHOR_RIGHT | PH_ANCHOR_BOTTOM);

            if (MinimumSize.left == -1)
            {
                RECT rect;

                rect.left = 0;
                rect.top = 0;
                rect.right = 200;
                rect.bottom = 140;
                MapDialogRect(hwndDlg, &rect);
                MinimumSize = rect;
                MinimumSize.left = 0;
            }

            PhSetDialogItemText(hwndDlg, IDC_TEXT, context->String);

            PhSetDialogFocus(hwndDlg, GetDlgItem(hwndDlg, IDOK));

            PhInitializeWindowTheme(hwndDlg, PhEnableThemeSupport);
        }
        break;
    case WM_DESTROY:
        {
            PhDeleteLayoutManager(&context->LayoutManager);

            PhRemoveWindowContext(hwndDlg, PH_WINDOW_CONTEXT_DEFAULT);
        }
        break;
    case WM_COMMAND:
        {
            switch (GET_WM_COMMAND_ID(wParam, lParam))
            {
            case IDCANCEL:
            case IDOK:
                EndDialog(hwndDlg, IDOK);
                break;
            case IDC_COPY:
                {
                    HWND editControl;
                    LONG selStart;
                    LONG selEnd;
                    PH_STRINGREF string;

                    editControl = GetDlgItem(hwndDlg, IDC_TEXT);
                    SendMessage(editControl, EM_GETSEL, (WPARAM)&selStart, (LPARAM)&selEnd);

                    if (selStart == selEnd)
                    {
                        // Select and copy the entire string.
                        PhInitializeStringRefLongHint(&string, context->String);
                        Edit_SetSel(editControl, 0, -1);
                    }
                    else
                    {
                        string.Buffer = context->String + selStart;
                        string.Length = (selEnd - selStart) * 2;
                    }

                    PhSetClipboardString(hwndDlg, &string);

                    PhSetDialogFocus(hwndDlg, editControl);
                }
                break;
            case IDC_SAVE:
                {
                    static PH_FILETYPE_FILTER filters[] =
                    {
                        { L"Text files (*.txt)", L"*.txt" },
                        { L"All files (*.*)", L"*.*" }
                    };
                    PVOID fileDialog;

                    fileDialog = PhCreateSaveFileDialog();

                    PhSetFileDialogFilter(fileDialog, filters, sizeof(filters) / sizeof(PH_FILETYPE_FILTER));
                    PhSetFileDialogFileName(fileDialog, L"Information.txt");

                    if (PhShowFileDialog(hwndDlg, fileDialog))
                    {
                        NTSTATUS status;
                        PPH_STRING fileName;
                        PPH_FILE_STREAM fileStream;

                        fileName = PH_AUTO(PhGetFileDialogFileName(fileDialog));

                        if (NT_SUCCESS(status = PhCreateFileStream(
                            &fileStream,
                            fileName->Buffer,
                            FILE_GENERIC_WRITE,
                            FILE_SHARE_READ,
                            FILE_OVERWRITE_IF,
                            0
                            )))
                        {
                            PH_STRINGREF string;

                            PhWriteStringAsUtf8FileStream(fileStream, &PhUnicodeByteOrderMark);
                            PhInitializeStringRef(&string, context->String);
                            PhWriteStringAsUtf8FileStream(fileStream, &string);
                            PhDereferenceObject(fileStream);
                        }

                        if (!NT_SUCCESS(status))
                            PhShowStatus(hwndDlg, L"Unable to create the file", status, 0);
                    }

                    PhFreeFileDialog(fileDialog);
                }
                break;
            }
        }
        break;
    case WM_SIZE:
        {
            PhLayoutManagerLayout(&context->LayoutManager);
        }
        break;
    case WM_SIZING:
        {
            PhResizingMinimumSize((PRECT)lParam, wParam, MinimumSize.right, MinimumSize.bottom);
        }
        break;
    }

    return FALSE;
}

VOID PhShowInformationDialog(
    _In_ HWND ParentWindowHandle,
    _In_ PWSTR String,
    _Reserved_ ULONG Flags
    )
{
    INFORMATION_CONTEXT context;

    context.String = String;
    context.Flags = Flags;

    DialogBoxParam(
        PhInstanceHandle,
        MAKEINTRESOURCE(IDD_INFORMATION),
        ParentWindowHandle,
        PhpInformationDlgProc,
        (LPARAM)&context
        );
}
