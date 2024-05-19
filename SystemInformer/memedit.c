/*
 * Copyright (c) 2022 Winsider Seminars & Solutions, Inc.  All rights reserved.
 *
 * This file is part of System Informer.
 *
 * Authors:
 *
 *     wj32    2010-2016
 *
 */

#include <phapp.h>
#include <hexedit.h>

#include <procprv.h>
#include <settings.h>

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
    HWND OwnerHandle;
    HWND HexEditHandle;
    PH_LAYOUT_MANAGER LayoutManager;

    PUCHAR Buffer;
    ULONG SelectOffset;
    PPH_STRING Title;
    ULONG Flags;

    BOOLEAN LoadCompleted;
    BOOLEAN WriteAccess;
} MEMORY_EDITOR_CONTEXT, *PMEMORY_EDITOR_CONTEXT;

INT NTAPI PhpMemoryEditorCompareFunction(
    _In_ PPH_AVL_LINKS Links1,
    _In_ PPH_AVL_LINKS Links2
    );

INT_PTR CALLBACK PhpMemoryEditorDlgProc(
    _In_ HWND hwndDlg,
    _In_ UINT uMsg,
    _In_ WPARAM wParam,
    _In_ LPARAM lParam
    );

PH_AVL_TREE PhMemoryEditorSet = PH_AVL_TREE_INIT(PhpMemoryEditorCompareFunction);
static RECT MinimumSize = { -1, -1, -1, -1 };

VOID PhShowMemoryEditorDialog(
    _In_ HWND OwnerWindow,
    _In_ HANDLE ProcessId,
    _In_ PVOID BaseAddress,
    _In_ SIZE_T RegionSize,
    _In_ ULONG SelectOffset,
    _In_ ULONG SelectLength,
    _In_opt_ PPH_STRING Title,
    _In_ ULONG Flags
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
        context = PhAllocateZero(sizeof(MEMORY_EDITOR_CONTEXT));
        context->OwnerHandle = OwnerWindow;
        context->ProcessId = ProcessId;
        context->BaseAddress = BaseAddress;
        context->RegionSize = RegionSize;
        context->SelectOffset = SelectOffset;
        PhSwapReference(&context->Title, Title);
        context->Flags = Flags;

        context->WindowHandle = PhCreateDialog(
            PhInstanceHandle,
            MAKEINTRESOURCE(IDD_MEMEDIT),
            NULL,
            PhpMemoryEditorDlgProc,
            context
            );

        if (!context->LoadCompleted)
        {
            DestroyWindow(context->WindowHandle);
            return;
        }

        if (SelectOffset != ULONG_MAX)
            PostMessage(context->WindowHandle, WM_PH_SELECT_OFFSET, SelectOffset, SelectLength);

        PhRegisterDialog(context->WindowHandle);
        PhAddElementAvlTree(&PhMemoryEditorSet, &context->Links);

        ShowWindow(context->WindowHandle, SW_SHOW);
        SetForegroundWindow(context->WindowHandle);
    }
    else
    {
        context = CONTAINING_RECORD(links, MEMORY_EDITOR_CONTEXT, Links);

        if (IsMinimized(context->WindowHandle))
            ShowWindow(context->WindowHandle, SW_RESTORE);
        else
            SetForegroundWindow(context->WindowHandle);

        if (SelectOffset != ULONG_MAX)
            PostMessage(context->WindowHandle, WM_PH_SELECT_OFFSET, SelectOffset, SelectLength);

        // Just in case.
        if ((Flags & PH_MEMORY_EDITOR_UNMAP_VIEW_OF_SECTION) && ProcessId == NtCurrentProcessId())
            NtUnmapViewOfSection(NtCurrentProcess(), BaseAddress);
    }
}

INT NTAPI PhpMemoryEditorCompareFunction(
    _In_ PPH_AVL_LINKS Links1,
    _In_ PPH_AVL_LINKS Links2
    )
{
    PMEMORY_EDITOR_CONTEXT context1 = CONTAINING_RECORD(Links1, MEMORY_EDITOR_CONTEXT, Links);
    PMEMORY_EDITOR_CONTEXT context2 = CONTAINING_RECORD(Links2, MEMORY_EDITOR_CONTEXT, Links);

    return memcmp(context1->Key, context2->Key, sizeof(context1->Key));
}

INT_PTR CALLBACK PhpMemoryEditorDlgProc(
    _In_ HWND hwndDlg,
    _In_ UINT uMsg,
    _In_ WPARAM wParam,
    _In_ LPARAM lParam
    )
{
    PMEMORY_EDITOR_CONTEXT context;

    if (uMsg == WM_INITDIALOG)
    {
        context = (PMEMORY_EDITOR_CONTEXT)lParam;
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
            NTSTATUS status;

            PhSetApplicationWindowIcon(hwndDlg);

            if (context->Title)
            {
                PhSetWindowText(hwndDlg, context->Title->Buffer);
            }
            else
            {
                PPH_PROCESS_ITEM processItem;

                if (processItem = PhReferenceProcessItem(context->ProcessId))
                {
                    PhSetWindowText(hwndDlg, PhaFormatString(L"%s (%u) (0x%Ix - 0x%Ix)",
                        processItem->ProcessName->Buffer, HandleToUlong(context->ProcessId),
                        (ULONG_PTR)context->BaseAddress, (ULONG_PTR)context->BaseAddress + context->RegionSize)->Buffer);
                    PhDereferenceObject(processItem);
                }
            }

            PhInitializeLayoutManager(&context->LayoutManager, hwndDlg);

            if (context->RegionSize > 1024 * 1024 * 1024) // 1 GB
            {
                PhShowError(context->OwnerHandle, L"%s", L"Unable to edit the memory region because it is too large.");
                return TRUE;
            }

            if (!NT_SUCCESS(status = PhOpenProcess(
                &context->ProcessHandle,
                PROCESS_VM_READ,
                context->ProcessId
                )))
            {
                PhShowStatus(context->OwnerHandle, L"Unable to open the process", status, 0);
                return TRUE;
            }

            context->Buffer = PhAllocatePage(context->RegionSize, NULL);

            if (!context->Buffer)
            {
                PhShowError(context->OwnerHandle, L"%s", L"Unable to allocate memory for the buffer.");
                return TRUE;
            }

            if (!NT_SUCCESS(status = NtReadVirtualMemory(
                context->ProcessHandle,
                context->BaseAddress,
                context->Buffer,
                context->RegionSize,
                NULL
                )))
            {
                PhShowStatus(context->OwnerHandle, L"Unable to read memory", status, 0);
                return TRUE;
            }

            PhAddLayoutItem(&context->LayoutManager, GetDlgItem(hwndDlg, IDOK), NULL,
                PH_ANCHOR_RIGHT | PH_ANCHOR_BOTTOM);
            PhAddLayoutItem(&context->LayoutManager, GetDlgItem(hwndDlg, IDC_SAVE), NULL,
                PH_ANCHOR_RIGHT | PH_ANCHOR_BOTTOM);
            PhAddLayoutItem(&context->LayoutManager, GetDlgItem(hwndDlg, IDC_BYTESPERROW), NULL,
                PH_ANCHOR_BOTTOM | PH_ANCHOR_LEFT);
            PhAddLayoutItem(&context->LayoutManager, GetDlgItem(hwndDlg, IDC_GOTO), NULL,
                PH_ANCHOR_BOTTOM | PH_ANCHOR_LEFT);
            PhAddLayoutItem(&context->LayoutManager, GetDlgItem(hwndDlg, IDC_WRITE), NULL,
                PH_ANCHOR_BOTTOM | PH_ANCHOR_LEFT);
            PhAddLayoutItem(&context->LayoutManager, GetDlgItem(hwndDlg, IDC_REREAD), NULL,
                PH_ANCHOR_BOTTOM | PH_ANCHOR_LEFT);

            if (MinimumSize.left == -1)
            {
                RECT rect;

                rect.left = 0;
                rect.top = 0;
                rect.right = 290;
                rect.bottom = 140;
                MapDialogRect(hwndDlg, &rect);
                MinimumSize = rect;
                MinimumSize.left = 0;
            }

            context->HexEditHandle = GetDlgItem(hwndDlg, IDC_MEMORY);
            PhAddLayoutItem(&context->LayoutManager, context->HexEditHandle, NULL, PH_ANCHOR_ALL);
            HexEdit_SetBuffer(context->HexEditHandle, context->Buffer, (ULONG)context->RegionSize);

            {
                PH_RECTANGLE windowRectangle = { 0 };

                windowRectangle.Position = PhGetIntegerPairSetting(L"MemEditPosition");

                if (windowRectangle.Position.X == 0)
                {
                    PhCenterWindow(hwndDlg, context->OwnerHandle);
                }
                else
                {
                    RECT rect;
                    LONG dpiValue;

                    rect = PhRectangleToRect(windowRectangle);
                    dpiValue = PhGetMonitorDpi(&rect);

                    windowRectangle.Size = PhGetScalableIntegerPairSetting(L"MemEditSize", TRUE, dpiValue).Pair;
                    PhAdjustRectangleToWorkingArea(NULL, &windowRectangle);

                    MoveWindow(hwndDlg, windowRectangle.Left, windowRectangle.Top,
                        windowRectangle.Width, windowRectangle.Height, FALSE);

                    // Implement cascading by saving an offsetted rectangle.
                    windowRectangle.Left += 20;
                    windowRectangle.Top += 20;

                    PhSetIntegerPairSetting(L"MemEditPosition", windowRectangle.Position);
                    PhSetScalableIntegerPairSetting2(L"MemEditSize", windowRectangle.Size, dpiValue);
                }
            }

            {
                PWSTR bytesPerRowStrings[7];
                ULONG i;
                ULONG bytesPerRow;

                for (i = 0; i < sizeof(bytesPerRowStrings) / sizeof(PWSTR); i++)
                    bytesPerRowStrings[i] = PhaFormatString(L"%u bytes per row", 1 << (2 + i))->Buffer;

                PhAddComboBoxStrings(GetDlgItem(hwndDlg, IDC_BYTESPERROW),
                    bytesPerRowStrings, sizeof(bytesPerRowStrings) / sizeof(PWSTR));

                bytesPerRow = PhGetIntegerSetting(L"MemEditBytesPerRow");

                if (bytesPerRow >= 4)
                {
                    HexEdit_SetBytesPerRow(context->HexEditHandle, bytesPerRow);
                    PhSelectComboBoxString(GetDlgItem(hwndDlg, IDC_BYTESPERROW),
                        PhaFormatString(L"%u bytes per row", bytesPerRow)->Buffer, FALSE);
                }
            }

            PhInitializeWindowTheme(hwndDlg, !!PhGetIntegerSetting(L"EnableThemeSupport"));

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

            PhRemoveWindowContext(hwndDlg, PH_WINDOW_CONTEXT_DEFAULT);

            PhDeleteLayoutManager(&context->LayoutManager);

            if (context->Buffer) PhFreePage(context->Buffer);
            if (context->ProcessHandle) NtClose(context->ProcessHandle);
            PhClearReference(&context->Title);

            if ((context->Flags & PH_MEMORY_EDITOR_UNMAP_VIEW_OF_SECTION) && context->ProcessId == NtCurrentProcessId())
                NtUnmapViewOfSection(NtCurrentProcess(), context->BaseAddress);

            PhFree(context);
        }
        break;
    case WM_SHOWWINDOW:
        {
            PhSetDialogFocus(hwndDlg, context->HexEditHandle);
        }
        break;
    case WM_COMMAND:
        {
            switch (GET_WM_COMMAND_ID(wParam, lParam))
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

                    if (!context->Title && (processItem = PhReferenceProcessItem(context->ProcessId)))
                    {
                        PhSetFileDialogFileName(fileDialog,
                            PhaFormatString(L"%s_0x%Ix-0x%Ix.bin", processItem->ProcessName->Buffer,
                            context->BaseAddress, context->RegionSize)->Buffer);
                        PhDereferenceObject(processItem);
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
                        L"Go to Offset",
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
                                PhShowError(hwndDlg, L"%s", L"The offset is too large.");
                                continue;
                            }

                            PhSetDialogFocus(hwndDlg, context->HexEditHandle);
                            HexEdit_SetSel(context->HexEditHandle, (LONG)offset, (LONG)offset);
                            break;
                        }
                    }
                }
                break;
            case IDC_WRITE:
                {
                    NTSTATUS status;

                    if (PhGetIntegerSetting(L"EnableWarnings") && !PhShowConfirmMessage(
                        hwndDlg,
                        L"write",
                        L"process memory",
                        L"Some programs may restrict access or ban your account when editing the memory of the process.",
                        FALSE
                        ))
                    {
                        break;
                    }

                    if (!context->WriteAccess)
                    {
                        HANDLE processHandle;

                        if (!NT_SUCCESS(status = PhOpenProcess(
                            &processHandle,
                            PROCESS_VM_READ | PROCESS_VM_WRITE,
                            context->ProcessId
                            )))
                        {
                            PhShowStatus(hwndDlg, L"Unable to open the process", status, 0);
                            break;
                        }

                        if (context->ProcessHandle) NtClose(context->ProcessHandle);
                        context->ProcessHandle = processHandle;
                        context->WriteAccess = TRUE;
                    }

                    if (!NT_SUCCESS(status = NtWriteVirtualMemory(
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

                    if (!NT_SUCCESS(status = NtReadVirtualMemory(
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
            case IDC_BYTESPERROW:
                if (HIWORD(wParam) == CBN_SELCHANGE)
                {
                    PPH_STRING bytesPerRowString = PhaGetDlgItemText(hwndDlg, IDC_BYTESPERROW);
                    PH_STRINGREF firstPart;
                    PH_STRINGREF secondPart;
                    ULONG64 bytesPerRow64;

                    if (PhSplitStringRefAtChar(&bytesPerRowString->sr, L' ', &firstPart, &secondPart))
                    {
                        if (PhStringToInteger64(&firstPart, 10, &bytesPerRow64))
                        {
                            PhSetIntegerSetting(L"MemEditBytesPerRow", (ULONG)bytesPerRow64);
                            HexEdit_SetBytesPerRow(context->HexEditHandle, (ULONG)bytesPerRow64);
                            PhSetDialogFocus(hwndDlg, context->HexEditHandle);
                        }
                    }
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
    case WM_PH_SELECT_OFFSET:
        {
            HexEdit_SetEditMode(context->HexEditHandle, EDIT_ASCII);
            HexEdit_SetSel(context->HexEditHandle, (ULONG)wParam, PtrToUlong(PTR_ADD_OFFSET(wParam, lParam)));
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
