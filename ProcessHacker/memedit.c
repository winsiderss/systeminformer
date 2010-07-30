/*
 * Process Hacker - 
 *   memory editor window
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
#include <settings.h>
#include <hexedit.h>
#include <windowsx.h>

#define WM_PH_SELECT_OFFSET (WM_APP + 300)

typedef struct _MEMORY_EDITOR_CONTEXT
{
    PH_AVL_LINKS Links;
    union
    {
        struct
        {
            HANDLE ProcessId;
            PVOID BaseAddress;
            SIZE_T RegionSize;
        };
        ULONG_PTR Key[3];
    };
    HANDLE ProcessHandle;
    HWND WindowHandle;
    PH_LAYOUT_MANAGER LayoutManager;
    HWND HexEditHandle;
    PUCHAR Buffer;
    ULONG SelectOffset;

    BOOLEAN LoadCompleted;
    BOOLEAN CreateFailed;
} MEMORY_EDITOR_CONTEXT, *PMEMORY_EDITOR_CONTEXT;

INT NTAPI PhpMemoryEditorCompareFunction(
    __in PPH_AVL_LINKS Links1,
    __in PPH_AVL_LINKS Links2
    );

INT_PTR CALLBACK PhpMemoryEditorDlgProc(      
    __in HWND hwndDlg,
    __in UINT uMsg,
    __in WPARAM wParam,
    __in LPARAM lParam
    );

PH_AVL_TREE PhMemoryEditorSet = PH_AVL_TREE_INIT(PhpMemoryEditorCompareFunction);

VOID PhShowMemoryEditorDialog(
    __in HANDLE ProcessId,
    __in PVOID BaseAddress,
    __in SIZE_T RegionSize,
    __in ULONG SelectOffset,
    __in ULONG SelectLength
    )
{
    PMEMORY_EDITOR_CONTEXT context;
    MEMORY_EDITOR_CONTEXT lookupContext;
    PPH_AVL_LINKS links;

    lookupContext.ProcessId = ProcessId;
    lookupContext.BaseAddress = BaseAddress;
    lookupContext.RegionSize = RegionSize;

    links = PhFindElementAvlTree(&PhMemoryEditorSet, &lookupContext.Links);

    if (!links)
    {
        context = PhAllocate(sizeof(MEMORY_EDITOR_CONTEXT));
        memset(context, 0, sizeof(MEMORY_EDITOR_CONTEXT));

        context->ProcessId = ProcessId;
        context->BaseAddress = BaseAddress;
        context->RegionSize = RegionSize;
        context->SelectOffset = SelectOffset;

        context->WindowHandle = CreateDialogParam(
            PhInstanceHandle,
            MAKEINTRESOURCE(IDD_MEMEDIT),
            NULL,
            PhpMemoryEditorDlgProc,
            (LPARAM)context
            );

        if (context->CreateFailed)
        {
            DestroyWindow(context->WindowHandle);
            context->WindowHandle = NULL;
            return;
        }

        if (SelectOffset != -1)
            PostMessage(context->WindowHandle, WM_PH_SELECT_OFFSET, SelectOffset, SelectLength);

        PhRegisterDialog(context->WindowHandle);
        PhAddElementAvlTree(&PhMemoryEditorSet, &context->Links);

        ShowWindow(context->WindowHandle, SW_SHOW);
    }
    else
    {
        context = CONTAINING_RECORD(links, MEMORY_EDITOR_CONTEXT, Links);

        if (IsIconic(context->WindowHandle))
            ShowWindow(context->WindowHandle, SW_RESTORE);
        else
            SetForegroundWindow(context->WindowHandle);

        if (SelectOffset != -1)
            PostMessage(context->WindowHandle, WM_PH_SELECT_OFFSET, SelectOffset, SelectLength);
    }
}

INT NTAPI PhpMemoryEditorCompareFunction(
    __in PPH_AVL_LINKS Links1,
    __in PPH_AVL_LINKS Links2
    )
{
    PMEMORY_EDITOR_CONTEXT context1 = CONTAINING_RECORD(Links1, MEMORY_EDITOR_CONTEXT, Links);
    PMEMORY_EDITOR_CONTEXT context2 = CONTAINING_RECORD(Links2, MEMORY_EDITOR_CONTEXT, Links);

    return memcmp(context1->Key, context2->Key, sizeof(context1->Key));
}

INT_PTR CALLBACK PhpMemoryEditorDlgProc(      
    __in HWND hwndDlg,
    __in UINT uMsg,
    __in WPARAM wParam,
    __in LPARAM lParam
    )
{
    PMEMORY_EDITOR_CONTEXT context;

    if (uMsg != WM_INITDIALOG)
    {
        context = GetProp(hwndDlg, L"Context");
    }
    else
    {
        context = (PMEMORY_EDITOR_CONTEXT)lParam;
        SetProp(hwndDlg, L"Context", (HANDLE)context);
    }

    if (!context)
        return FALSE;

    switch (uMsg)
    {
    case WM_INITDIALOG:
        {
            NTSTATUS status;
            RECT rect;

            {
                PPH_PROCESS_ITEM processItem;

                if (processItem = PhReferenceProcessItem(context->ProcessId))
                {
                    SetWindowText(hwndDlg, PhaFormatString(L"%s (%u) (0x%Ix - 0x%Ix)",
                        processItem->ProcessName->Buffer, (ULONG)context->ProcessId,
                        context->BaseAddress, (ULONG_PTR)context->BaseAddress + context->RegionSize)->Buffer);
                    PhDereferenceObject(processItem);
                }
            }

            PhInitializeLayoutManager(&context->LayoutManager, hwndDlg);

            if (context->RegionSize > 1024 * 1024 * 1024) // 1 GB
            {
                PhShowError(NULL, L"Unable to edit the memory region because it is too large.");
                context->CreateFailed = TRUE;
                return TRUE;
            }

            if (!NT_SUCCESS(status = PhOpenProcess(
                &context->ProcessHandle,
                PROCESS_VM_READ | PROCESS_VM_WRITE,
                context->ProcessId
                )))
            {
                if (!NT_SUCCESS(status = PhOpenProcess(
                    &context->ProcessHandle,
                    PROCESS_VM_READ,
                    context->ProcessId
                    )))
                {
                    PhShowStatus(NULL, L"Unable to open the process", status, 0);
                    context->CreateFailed = TRUE;
                    return TRUE;
                }
            }

            context->Buffer = PhAllocatePage(context->RegionSize, NULL);

            if (!context->Buffer)
            {
                PhShowError(NULL, L"Unable to allocate memory for the buffer.");
                context->CreateFailed = TRUE;
                return TRUE;
            }

            if (!NT_SUCCESS(status = PhReadVirtualMemory(
                context->ProcessHandle,
                context->BaseAddress,
                context->Buffer,
                context->RegionSize,
                NULL
                )))
            {
                PhShowStatus(PhMainWndHandle, L"Unable to read memory", status, 0);
                context->CreateFailed = TRUE;
                return TRUE;
            }

            PhAddLayoutItem(&context->LayoutManager, GetDlgItem(hwndDlg, IDOK), NULL,
                PH_ANCHOR_RIGHT | PH_ANCHOR_BOTTOM);
            PhAddLayoutItem(&context->LayoutManager, GetDlgItem(hwndDlg, IDC_SAVE), NULL,
                PH_ANCHOR_RIGHT | PH_ANCHOR_BOTTOM);
            PhAddLayoutItem(&context->LayoutManager, GetDlgItem(hwndDlg, IDC_GOTO), NULL,
                PH_ANCHOR_BOTTOM | PH_ANCHOR_LEFT);
            PhAddLayoutItem(&context->LayoutManager, GetDlgItem(hwndDlg, IDC_WRITE), NULL,
                PH_ANCHOR_BOTTOM | PH_ANCHOR_LEFT);
            PhAddLayoutItem(&context->LayoutManager, GetDlgItem(hwndDlg, IDC_REREAD), NULL,
                PH_ANCHOR_BOTTOM | PH_ANCHOR_LEFT);

            GetWindowRect(GetDlgItem(hwndDlg, IDC_MEMORY_LAYOUT), &rect);
            MapWindowPoints(NULL, hwndDlg, (POINT *)&rect, 2);

            context->HexEditHandle = PhCreateHexEditControl(hwndDlg, IDC_MEMORY);
            BringWindowToTop(context->HexEditHandle);
            MoveWindow(context->HexEditHandle, rect.left, rect.top,
                rect.right - rect.left, rect.bottom - rect.top, FALSE);
            PhAddLayoutItem(&context->LayoutManager, context->HexEditHandle, NULL,
                PH_ANCHOR_ALL);
            ShowWindow(context->HexEditHandle, SW_SHOW);
            HexEdit_SetBuffer(context->HexEditHandle, context->Buffer, (ULONG)context->RegionSize);

            {
                PH_RECTANGLE windowRectangle;

                windowRectangle.Position = PhGetIntegerPairSetting(L"MemEditPosition");
                windowRectangle.Size = PhGetIntegerPairSetting(L"MemEditSize");
                PhAdjustRectangleToWorkingArea(hwndDlg, &windowRectangle);

                MoveWindow(hwndDlg, windowRectangle.Left, windowRectangle.Top,
                    windowRectangle.Width, windowRectangle.Height, FALSE);

                // Implement cascading by saving an offsetted rectangle.
                windowRectangle.Left += 20;
                windowRectangle.Top += 20;

                PhSetIntegerPairSetting(L"MemEditPosition", windowRectangle.Position);
                PhSetIntegerPairSetting(L"MemEditSize", windowRectangle.Size);
            }

            context->LoadCompleted = TRUE;
        }
        break;
    case WM_DESTROY:
        {
            if (context->LoadCompleted)
            {
                PhSaveWindowPlacementToSetting(L"MemEditPosition", L"MemEditSize", hwndDlg);
                PhRemoveElementAvlTree(&PhMemoryEditorSet, &context->Links);
                PhUnregisterDialog(hwndDlg);
            }

            RemoveProp(hwndDlg, L"Context");

            PhDeleteLayoutManager(&context->LayoutManager);

            if (context->Buffer) PhFreePage(context->Buffer);
            if (context->ProcessHandle) NtClose(context->ProcessHandle);

            PhFree(context);
        }
        break;
    case WM_SHOWWINDOW:
        {
            SetFocus(context->HexEditHandle);
        }
        break;
    case WM_COMMAND:
        {
            switch (LOWORD(wParam))
            {
            case IDCANCEL:
            case IDOK:
                DestroyWindow(hwndDlg);
                break;
            case IDC_SAVE:
                {
                    static PH_FILETYPE_FILTER filters[] =
                    {
                        { L"Binary files (*.bin)", L"*.bin" },
                        { L"All files (*.*)", L"*.*" }
                    };
                    PVOID fileDialog;
                    PPH_PROCESS_ITEM processItem;

                    fileDialog = PhCreateSaveFileDialog();

                    PhSetFileDialogFilter(fileDialog, filters, sizeof(filters) / sizeof(PH_FILETYPE_FILTER));

                    if (processItem = PhReferenceProcessItem(context->ProcessId))
                    {
                        PhSetFileDialogFileName(fileDialog,
                            PhaFormatString(L"%s_0x%Ix-0x%Ix.bin", processItem->ProcessName->Buffer,
                            context->BaseAddress, context->RegionSize)->Buffer);
                    }
                    else
                    {
                        PhSetFileDialogFileName(fileDialog, L"Memory.bin");
                    }

                    if (PhShowFileDialog(hwndDlg, fileDialog))
                    {
                        NTSTATUS status;
                        PPH_STRING fileName;
                        PPH_FILE_STREAM fileStream;

                        fileName = PhGetFileDialogFileName(fileDialog);
                        PhaDereferenceObject(fileName);

                        if (NT_SUCCESS(status = PhCreateFileStream(
                            &fileStream,
                            fileName->Buffer,
                            FILE_GENERIC_WRITE,
                            FILE_SHARE_READ,
                            FILE_OVERWRITE_IF,
                            0
                            )))
                        {
                            status = PhWriteFileStream(fileStream, context->Buffer, (ULONG)context->RegionSize);
                            PhDereferenceObject(fileStream);
                        }

                        if (!NT_SUCCESS(status))
                            PhShowStatus(hwndDlg, L"Unable to create the file", status, 0);
                    }

                    PhFreeFileDialog(fileDialog);
                }
                break;
            case IDC_GOTO:
                {
                    PPH_STRING selectedChoice = NULL;

                    while (PhaChoiceDialog(
                        hwndDlg,
                        L"Goto Offset",
                        L"Enter an offset:",
                        NULL,
                        0,
                        NULL,
                        PH_CHOICE_DIALOG_USER_CHOICE,
                        &selectedChoice,
                        NULL,
                        L"MemEditGotoChoices"
                        ))
                    {
                        ULONG64 offset;

                        if (selectedChoice->Length == 0)
                            continue;

                        if (PhStringToInteger64(&selectedChoice->sr, 0, &offset))
                        {
                            if (offset >= context->RegionSize)
                            {
                                PhShowError(hwndDlg, L"The offset is too large.");
                                continue;
                            }

                            SetFocus(context->HexEditHandle);
                            HexEdit_SetSel(context->HexEditHandle, (LONG)offset, (LONG)offset);
                            break;
                        }
                    }
                }
                break;
            case IDC_WRITE:
                {
                    NTSTATUS status;

                    if (!NT_SUCCESS(status = PhWriteVirtualMemory(
                        context->ProcessHandle,
                        context->BaseAddress,
                        context->Buffer,
                        context->RegionSize,
                        NULL
                        )))
                    {
                        PhShowStatus(hwndDlg, L"Unable to write memory", status, 0);
                    }
                }
                break;
            case IDC_REREAD:
                {
                    NTSTATUS status;

                    if (!NT_SUCCESS(status = PhReadVirtualMemory(
                        context->ProcessHandle,
                        context->BaseAddress,
                        context->Buffer,
                        context->RegionSize,
                        NULL
                        )))
                    {
                        PhShowStatus(hwndDlg, L"Unable to read memory", status, 0);
                    }

                    InvalidateRect(context->HexEditHandle, NULL, TRUE);
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
            PhResizingMinimumSize((PRECT)lParam, wParam, 450, 300);
        }
        break;
    case WM_PH_SELECT_OFFSET:
        {
            HexEdit_SetEditMode(context->HexEditHandle, EDIT_ASCII);
            HexEdit_SetSel(context->HexEditHandle, (ULONG)wParam, (ULONG)wParam + (ULONG)lParam);
        }
        break;
    }

    return FALSE;
}
