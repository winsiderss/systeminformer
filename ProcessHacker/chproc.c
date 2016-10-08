/*
 * Process Hacker -
 *   choose process dialog
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

#include <lsasup.h>

typedef struct _CHOOSE_PROCESS_DIALOG_CONTEXT
{
    PWSTR Message;
    HANDLE ProcessId;

    PH_LAYOUT_MANAGER LayoutManager;
    RECT MinimumSize;
    HIMAGELIST ImageList;
    HWND ListViewHandle;
} CHOOSE_PROCESS_DIALOG_CONTEXT, *PCHOOSE_PROCESS_DIALOG_CONTEXT;

INT_PTR CALLBACK PhpChooseProcessDlgProc(
    _In_ HWND hwndDlg,
    _In_ UINT uMsg,
    _In_ WPARAM wParam,
    _In_ LPARAM lParam
    );

BOOLEAN PhShowChooseProcessDialog(
    _In_ HWND ParentWindowHandle,
    _In_ PWSTR Message,
    _Out_ PHANDLE ProcessId
    )
{
    CHOOSE_PROCESS_DIALOG_CONTEXT context;

    context.Message = Message;
    context.ProcessId = NULL;

    if (DialogBoxParam(
        PhInstanceHandle,
        MAKEINTRESOURCE(IDD_CHOOSEPROCESS),
        ParentWindowHandle,
        PhpChooseProcessDlgProc,
        (LPARAM)&context
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
    _In_ PCHOOSE_PROCESS_DIALOG_CONTEXT Context
    )
{
    NTSTATUS status;
    HWND lvHandle;
    PVOID processes;
    PSYSTEM_PROCESS_INFORMATION process;

    lvHandle = Context->ListViewHandle;

    if (!NT_SUCCESS(status = PhEnumProcesses(&processes)))
    {
        PhShowStatus(hwndDlg, L"Unable to enumerate processes", status, 0);
        return;
    }

    ExtendedListView_SetRedraw(lvHandle, FALSE);

    ListView_DeleteAllItems(lvHandle);
    ImageList_RemoveAll(Context->ImageList);

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
        INT imageIndex;

        if (process->UniqueProcessId != SYSTEM_IDLE_PROCESS_ID)
            name = PhCreateStringFromUnicodeString(&process->ImageName);
        else
            name = PhCreateString(SYSTEM_IDLE_PROCESS_NAME);

        lvItemIndex = PhAddListViewItem(lvHandle, MAXINT, name->Buffer, process->UniqueProcessId);
        PhDereferenceObject(name);

        if (NT_SUCCESS(PhOpenProcess(&processHandle, ProcessQueryAccess, process->UniqueProcessId)))
        {
            HANDLE tokenHandle;
            PTOKEN_USER user;

            if (!WINDOWS_HAS_IMAGE_FILE_NAME_BY_PROCESS_ID && process->UniqueProcessId != SYSTEM_PROCESS_ID)
                PhGetProcessImageFileName(processHandle, &fileName);

            if (NT_SUCCESS(PhOpenProcessToken(processHandle, TOKEN_QUERY, &tokenHandle)))
            {
                if (NT_SUCCESS(PhGetTokenUser(tokenHandle, &user)))
                {
                    userName = PhGetSidFullName(user->User.Sid, TRUE, NULL);
                    PhFree(user);
                }

                NtClose(tokenHandle);
            }

            NtClose(processHandle);
        }

        if (process->UniqueProcessId == SYSTEM_IDLE_PROCESS_ID && !userName && PhLocalSystemName)
            PhSetReference(&userName, PhLocalSystemName);

        if (WINDOWS_HAS_IMAGE_FILE_NAME_BY_PROCESS_ID && process->UniqueProcessId != SYSTEM_PROCESS_ID)
            PhGetProcessImageFileNameByProcessId(process->UniqueProcessId, &fileName);

        if (process->UniqueProcessId == SYSTEM_PROCESS_ID)
            fileName = PhGetKernelFileName();

        if (fileName)
            PhMoveReference(&fileName, PhGetFileName(fileName));

        icon = PhGetFileShellIcon(PhGetString(fileName), L".exe", FALSE);

        // Icon
        if (icon)
        {
            imageIndex = ImageList_AddIcon(Context->ImageList, icon);
            PhSetListViewItemImageIndex(Context->ListViewHandle, lvItemIndex, imageIndex);
            DestroyIcon(icon);
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

    ExtendedListView_SortItems(lvHandle);
    ExtendedListView_SetRedraw(lvHandle, TRUE);
}

INT_PTR CALLBACK PhpChooseProcessDlgProc(
    _In_ HWND hwndDlg,
    _In_ UINT uMsg,
    _In_ WPARAM wParam,
    _In_ LPARAM lParam
    )
{
    PCHOOSE_PROCESS_DIALOG_CONTEXT context = NULL;

    if (uMsg == WM_INITDIALOG)
    {
        context = (PCHOOSE_PROCESS_DIALOG_CONTEXT)lParam;
        SetProp(hwndDlg, PhMakeContextAtom(), (HANDLE)context);
    }
    else
    {
        context = (PCHOOSE_PROCESS_DIALOG_CONTEXT)GetProp(hwndDlg, PhMakeContextAtom());

        if (uMsg == WM_DESTROY)
        {
            RemoveProp(hwndDlg, PhMakeContextAtom());
        }
    }

    if (!context)
        return FALSE;

    switch (uMsg)
    {
    case WM_INITDIALOG:
        {
            HWND lvHandle;

            PhCenterWindow(hwndDlg, GetParent(hwndDlg));

            SetDlgItemText(hwndDlg, IDC_MESSAGE, context->Message);

            PhInitializeLayoutManager(&context->LayoutManager, hwndDlg);
            PhAddLayoutItem(&context->LayoutManager, GetDlgItem(hwndDlg, IDC_MESSAGE), NULL,
                PH_ANCHOR_LEFT | PH_ANCHOR_TOP | PH_ANCHOR_RIGHT | PH_LAYOUT_FORCE_INVALIDATE);
            PhAddLayoutItem(&context->LayoutManager, GetDlgItem(hwndDlg, IDC_LIST), NULL,
                PH_ANCHOR_ALL);
            PhAddLayoutItem(&context->LayoutManager, GetDlgItem(hwndDlg, IDOK), NULL,
                PH_ANCHOR_RIGHT | PH_ANCHOR_BOTTOM);
            PhAddLayoutItem(&context->LayoutManager, GetDlgItem(hwndDlg, IDCANCEL), NULL,
                PH_ANCHOR_RIGHT | PH_ANCHOR_BOTTOM);
            PhAddLayoutItem(&context->LayoutManager, GetDlgItem(hwndDlg, IDC_REFRESH), NULL,
                PH_ANCHOR_BOTTOM | PH_ANCHOR_LEFT);
            PhLayoutManagerLayout(&context->LayoutManager);

            context->MinimumSize.left = 0;
            context->MinimumSize.top = 0;
            context->MinimumSize.right = 280;
            context->MinimumSize.bottom = 170;
            MapDialogRect(hwndDlg, &context->MinimumSize);

            context->ListViewHandle = lvHandle = GetDlgItem(hwndDlg, IDC_LIST);
            context->ImageList = ImageList_Create(PhSmallIconSize.X, PhSmallIconSize.Y, ILC_COLOR32 | ILC_MASK, 0, 40);

            PhSetListViewStyle(lvHandle, FALSE, TRUE);
            PhSetControlTheme(lvHandle, L"explorer");
            PhAddListViewColumn(lvHandle, 0, 0, 0, LVCFMT_LEFT, 180, L"Name");
            PhAddListViewColumn(lvHandle, 1, 1, 1, LVCFMT_LEFT, 60, L"PID");
            PhAddListViewColumn(lvHandle, 2, 2, 2, LVCFMT_LEFT, 160, L"User name");
            PhSetExtendedListView(lvHandle);

            ListView_SetImageList(lvHandle, context->ImageList, LVSIL_SMALL);

            PhpRefreshProcessList(hwndDlg, context);

            EnableWindow(GetDlgItem(hwndDlg, IDOK), FALSE);
        }
        break;
    case WM_DESTROY:
        {
            ImageList_Destroy(context->ImageList);
            PhDeleteLayoutManager(&context->LayoutManager);
        }
        break;
    case WM_COMMAND:
        {
            switch (LOWORD(wParam))
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
    }

    return FALSE;
}
