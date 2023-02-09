/*
 * Copyright (c) 2022 Winsider Seminars & Solutions, Inc.  All rights reserved.
 *
 * This file is part of System Informer.
 *
 * Authors:
 *
 *     wj32    2010
 *     dmex    2016-2023
 *
 */

#include <phapp.h>
#include <phsettings.h>
#include <settings.h>

typedef struct _PH_INFORMATION_CONTEXT
{
    PWSTR String;
    ULONG Flags;
    PH_LAYOUT_MANAGER LayoutManager;
    RECT MinimumSize;
} PH_INFORMATION_CONTEXT, *PPH_INFORMATION_CONTEXT;

static INT_PTR CALLBACK PhpInformationDlgProc(
    _In_ HWND hwndDlg,
    _In_ UINT uMsg,
    _In_ WPARAM wParam,
    _In_ LPARAM lParam
    )
{
    PPH_INFORMATION_CONTEXT context;

    if (uMsg == WM_INITDIALOG)
    {
        context = (PPH_INFORMATION_CONTEXT)lParam;
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

            PhInitializeLayoutManager(&context->LayoutManager, hwndDlg);
            PhAddLayoutItem(&context->LayoutManager, GetDlgItem(hwndDlg, IDC_TEXT), NULL, PH_ANCHOR_ALL);
            PhAddLayoutItem(&context->LayoutManager, GetDlgItem(hwndDlg, IDOK), NULL, PH_ANCHOR_RIGHT | PH_ANCHOR_BOTTOM);
            PhAddLayoutItem(&context->LayoutManager, GetDlgItem(hwndDlg, IDC_COPY), NULL, PH_ANCHOR_RIGHT | PH_ANCHOR_BOTTOM);
            PhAddLayoutItem(&context->LayoutManager, GetDlgItem(hwndDlg, IDC_SAVE), NULL, PH_ANCHOR_RIGHT | PH_ANCHOR_BOTTOM);

            if (PhGetIntegerPairSetting(L"InformationWindowPosition").X)
                PhLoadWindowPlacementFromSetting(NULL, L"InformationWindowSize", hwndDlg);
            PhCenterWindow(hwndDlg, GetParent(hwndDlg));

            context->MinimumSize = (RECT){ -1, -1, -1, -1 };

            if (context->MinimumSize.left == -1)
            {
                RECT rect;

                rect.left = 0;
                rect.top = 0;
                rect.right = 200;
                rect.bottom = 140;
                MapDialogRect(hwndDlg, &rect);
                context->MinimumSize = rect;
                context->MinimumSize.left = 0;
            }

            PhSetDialogItemText(hwndDlg, IDC_TEXT, context->String);

            PhSetDialogFocus(hwndDlg, GetDlgItem(hwndDlg, IDOK));

            PhInitializeWindowTheme(hwndDlg, PhEnableThemeSupport);
        }
        break;
    case WM_DESTROY:
        {
            PhSaveWindowPlacementToSetting(L"InformationWindowPosition", L"InformationWindowSize", hwndDlg);

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
                            PhInitializeStringRefLongHint(&string, context->String);
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
            PhResizingMinimumSize((PRECT)lParam, wParam, context->MinimumSize.right, context->MinimumSize.bottom);
        }
        break;
    case WM_CTLCOLORBTN:
        return HANDLE_WM_CTLCOLORBTN(hwndDlg, wParam, lParam, PhWindowThemeControlColor);
    case WM_CTLCOLORDLG:
        return HANDLE_WM_CTLCOLORDLG(hwndDlg, wParam, lParam, PhWindowThemeControlColor);
    case WM_CTLCOLORSTATIC:
        return HANDLE_WM_CTLCOLORSTATIC(hwndDlg, wParam, lParam, PhWindowThemeControlColor);
    }

    return FALSE;
}

VOID PhShowInformationDialog(
    _In_ HWND ParentWindowHandle,
    _In_ PWSTR String,
    _Reserved_ ULONG Flags
    )
{
    PH_INFORMATION_CONTEXT context;

    memset(&context, 0, sizeof(PH_INFORMATION_CONTEXT));
    context.String = String;
    context.Flags = Flags;

    PhDialogBox(
        PhInstanceHandle,
        MAKEINTRESOURCE(IDD_INFORMATION),
        ParentWindowHandle,
        PhpInformationDlgProc,
        &context
        );
}
