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
#include <procprv.h>
#include <lsasup.h>

typedef struct _PH_CHOOSE_PROCESS_DIALOG_CONTEXT
{
    PWSTR Message;
    HANDLE ProcessId;

    PH_LAYOUT_MANAGER LayoutManager;
    RECT MinimumSize;
    HIMAGELIST ImageList;
    HWND ListViewHandle;
} PH_CHOOSE_PROCESS_DIALOG_CONTEXT, *PPH_CHOOSE_PROCESS_DIALOG_CONTEXT;

INT_PTR CALLBACK PhpChooseProcessDlgProc(
    _In_ HWND hwndDlg,
    _In_ UINT uMsg,
    _In_ WPARAM wParam,
    _In_ LPARAM lParam
    );

_Success_(return)
BOOLEAN PhShowChooseProcessDialog(
    _In_ HWND ParentWindowHandle,
    _In_ PWSTR Message,
    _Out_ PHANDLE ProcessId
    )
{
    PH_CHOOSE_PROCESS_DIALOG_CONTEXT context;

    memset(&context, 0, sizeof(PH_CHOOSE_PROCESS_DIALOG_CONTEXT));
    context.Message = Message;
    context.ProcessId = NULL;

    if (PhDialogBox(
        PhInstanceHandle,
        MAKEINTRESOURCE(IDD_CHOOSEPROCESS),
        ParentWindowHandle,
        PhpChooseProcessDlgProc,
        &context
        ) == IDOK)
    {
        *ProcessId = context.ProcessId;

        return TRUE;
    }
    else
    {
        return FALSE;
    }
}

static VOID PhpRefreshProcessList(
    _In_ HWND hwndDlg,
    _In_ PPH_CHOOSE_PROCESS_DIALOG_CONTEXT Context
    )
{
    NTSTATUS status;
    PVOID processes;
    PSYSTEM_PROCESS_INFORMATION process;

    if (!NT_SUCCESS(status = PhEnumProcesses(&processes)))
    {
        PhShowStatus(hwndDlg, L"Unable to enumerate processes", status, 0);
        return;
    }

    ExtendedListView_SetRedraw(Context->ListViewHandle, FALSE);
    ListView_DeleteAllItems(Context->ListViewHandle);
    PhImageListRemoveAll(Context->ImageList);

    process = PH_FIRST_PROCESS(processes);

    do
    {
        INT lvItemIndex;
        PPH_STRING name;
        HANDLE processHandle;
        PPH_STRING fileName = NULL;
        HICON icon = NULL;
        WCHAR processIdString[PH_INT32_STR_LEN_1];
        PPH_STRING userName = NULL;
        INT imageIndex = INT_MAX;

        if (process->UniqueProcessId != SYSTEM_IDLE_PROCESS_ID)
            name = PhCreateStringFromUnicodeString(&process->ImageName);
        else
            name = PhCreateStringFromUnicodeString(&SYSTEM_IDLE_PROCESS_NAME);

        lvItemIndex = PhAddListViewItem(Context->ListViewHandle, MAXINT, name->Buffer, process->UniqueProcessId);
        PhDereferenceObject(name);

        if (NT_SUCCESS(PhOpenProcess(&processHandle, PROCESS_QUERY_LIMITED_INFORMATION, process->UniqueProcessId)))
        {
            HANDLE tokenHandle;
            PH_TOKEN_USER tokenUser;

            if (NT_SUCCESS(PhOpenProcessToken(processHandle, TOKEN_QUERY, &tokenHandle)))
            {
                if (NT_SUCCESS(PhGetTokenUser(tokenHandle, &tokenUser)))
                {
                    userName = PhGetSidFullName(tokenUser.User.Sid, TRUE, NULL);
                }

                NtClose(tokenHandle);
            }

            NtClose(processHandle);
        }

        if (process->UniqueProcessId == SYSTEM_IDLE_PROCESS_ID && !userName)
        {
            PhSetReference(&userName, PhGetSidFullName((PSID)&PhSeLocalSystemSid, TRUE, NULL));
        }

        if (process->UniqueProcessId == SYSTEM_PROCESS_ID)
            fileName = PhGetKernelFileName2();
        else if (PH_IS_REAL_PROCESS_ID(process->UniqueProcessId))
            PhGetProcessImageFileNameByProcessId(process->UniqueProcessId, &fileName);

        if (fileName)
            PhMoveReference(&fileName, PhGetFileName(fileName));

        // Icon
        if (!PhIsNullOrEmptyString(fileName))
        {
            PhExtractIcon(PhGetString(fileName), &icon, NULL);
        }

        if (icon)
        {
            imageIndex = PhImageListAddIcon(Context->ImageList, icon);
            PhSetListViewItemImageIndex(Context->ListViewHandle, lvItemIndex, imageIndex);
            DestroyIcon(icon);
        }
        else
        {
            PhGetStockApplicationIcon(NULL, &icon);
            imageIndex = PhImageListAddIcon(Context->ImageList, icon);
            PhSetListViewItemImageIndex(Context->ListViewHandle, lvItemIndex, imageIndex);
        }

        // PID
        PhPrintUInt32(processIdString, HandleToUlong(process->UniqueProcessId));
        PhSetListViewSubItem(Context->ListViewHandle, lvItemIndex, 1, processIdString);

        // User Name
        PhSetListViewSubItem(Context->ListViewHandle, lvItemIndex, 2, PhGetString(userName));

        if (userName) PhDereferenceObject(userName);
        if (fileName) PhDereferenceObject(fileName);
    } while (process = PH_NEXT_PROCESS(process));

    PhFree(processes);

    ExtendedListView_SortItems(Context->ListViewHandle);
    ExtendedListView_SetRedraw(Context->ListViewHandle, TRUE);
}

static VOID PhpChooseProcessSetImagelist(
    _Inout_ PPH_CHOOSE_PROCESS_DIALOG_CONTEXT context
    )
{
    LONG dpiValue;

    dpiValue = PhGetWindowDpi(context->ListViewHandle);

    if (context->ImageList)
    {
        PhImageListSetIconSize(
            context->ImageList,
            PhGetSystemMetrics(SM_CXSMICON, dpiValue),
            PhGetSystemMetrics(SM_CYSMICON, dpiValue)
            );
    }
    else
    {
        context->ImageList = PhImageListCreate(
            PhGetSystemMetrics(SM_CXSMICON, dpiValue),
            PhGetSystemMetrics(SM_CYSMICON, dpiValue),
            ILC_MASK | ILC_COLOR32,
            0, 40
            );
    }

    ListView_SetImageList(context->ListViewHandle, context->ImageList, LVSIL_SMALL);
}

INT_PTR CALLBACK PhpChooseProcessDlgProc(
    _In_ HWND hwndDlg,
    _In_ UINT uMsg,
    _In_ WPARAM wParam,
    _In_ LPARAM lParam
    )
{
    PPH_CHOOSE_PROCESS_DIALOG_CONTEXT context = NULL;

    if (uMsg == WM_INITDIALOG)
    {
        context = (PPH_CHOOSE_PROCESS_DIALOG_CONTEXT)lParam;
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
            HWND lvHandle;

            PhSetApplicationWindowIcon(hwndDlg);

            PhCenterWindow(hwndDlg, GetParent(hwndDlg));

            PhSetDialogItemText(hwndDlg, IDC_MESSAGE, context->Message);

            PhInitializeLayoutManager(&context->LayoutManager, hwndDlg);
            PhAddLayoutItem(&context->LayoutManager, GetDlgItem(hwndDlg, IDC_MESSAGE), NULL, PH_ANCHOR_LEFT | PH_ANCHOR_TOP | PH_ANCHOR_RIGHT | PH_LAYOUT_FORCE_INVALIDATE);
            PhAddLayoutItem(&context->LayoutManager, GetDlgItem(hwndDlg, IDC_LIST), NULL, PH_ANCHOR_ALL);
            PhAddLayoutItem(&context->LayoutManager, GetDlgItem(hwndDlg, IDOK), NULL, PH_ANCHOR_RIGHT | PH_ANCHOR_BOTTOM);
            PhAddLayoutItem(&context->LayoutManager, GetDlgItem(hwndDlg, IDCANCEL), NULL, PH_ANCHOR_RIGHT | PH_ANCHOR_BOTTOM);
            PhAddLayoutItem(&context->LayoutManager, GetDlgItem(hwndDlg, IDC_REFRESH), NULL, PH_ANCHOR_BOTTOM | PH_ANCHOR_LEFT);
            PhLayoutManagerLayout(&context->LayoutManager);

            context->MinimumSize.left = 0;
            context->MinimumSize.top = 0;
            context->MinimumSize.right = 280;
            context->MinimumSize.bottom = 170;
            MapDialogRect(hwndDlg, &context->MinimumSize);

            context->ListViewHandle = lvHandle = GetDlgItem(hwndDlg, IDC_LIST);

            PhSetListViewStyle(lvHandle, FALSE, TRUE);
            PhSetControlTheme(lvHandle, L"explorer");
            PhAddListViewColumn(lvHandle, 0, 0, 0, LVCFMT_LEFT, 180, L"Name");
            PhAddListViewColumn(lvHandle, 1, 1, 1, LVCFMT_LEFT, 60, L"PID");
            PhAddListViewColumn(lvHandle, 2, 2, 2, LVCFMT_LEFT, 160, L"User name");
            PhSetExtendedListView(lvHandle);

            PhpChooseProcessSetImagelist(context);

            PhpRefreshProcessList(hwndDlg, context);

            EnableWindow(GetDlgItem(hwndDlg, IDOK), FALSE);
        }
        break;
    case WM_DESTROY:
        {
            PhDeleteLayoutManager(&context->LayoutManager);

            if (context->ImageList)
            {
                PhImageListDestroy(context->ImageList);
                context->ImageList = NULL;
            }

            PhRemoveWindowContext(hwndDlg, PH_WINDOW_CONTEXT_DEFAULT);
        }
        break;
    case WM_DPICHANGED:
        {
            PhpChooseProcessSetImagelist(context);

            PhpRefreshProcessList(hwndDlg, context);
        }
        break;
    case WM_COMMAND:
        {
            switch (GET_WM_COMMAND_ID(wParam, lParam))
            {
            case IDCANCEL:
                {
                    EndDialog(hwndDlg, IDCANCEL);
                }
                break;
            case IDOK:
                {
                    if (ListView_GetSelectedCount(context->ListViewHandle) == 1)
                    {
                        context->ProcessId = (HANDLE)PhGetSelectedListViewItemParam(context->ListViewHandle);
                        EndDialog(hwndDlg, IDOK);
                    }
                }
                break;
            case IDC_REFRESH:
                {
                    PhpRefreshProcessList(hwndDlg, context);
                }
                break;
            }
        }
        break;
    case WM_NOTIFY:
        {
            LPNMHDR header = (LPNMHDR)lParam;

            switch (header->code)
            {
            case LVN_ITEMCHANGED:
                {
                    EnableWindow(GetDlgItem(hwndDlg, IDOK), ListView_GetSelectedCount(context->ListViewHandle) == 1);
                }
                break;
            case NM_DBLCLK:
                {
                    SendMessage(hwndDlg, WM_COMMAND, IDOK, 0);
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
