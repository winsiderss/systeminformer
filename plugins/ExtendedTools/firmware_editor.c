/*
 * Copyright (c) 2022 Winsider Seminars & Solutions, Inc.  All rights reserved.
 *
 * This file is part of System Informer.
 *
 * Authors:
 *
 *     dmex    2016-2022
 *
 */

#include "exttools.h"
#include "firmware.h"

typedef struct _EFI_EDITOR_CONTEXT
{
    HWND WindowHandle;
    HWND HexEditHandle;
    PH_LAYOUT_MANAGER LayoutManager;

    PPH_STRING Title;

    PPH_STRING Name;
    PPH_STRING GuidString;

    PVOID VariableValue;
    ULONG VariableValueLength;
} EFI_EDITOR_CONTEXT, *PEFI_EDITOR_CONTEXT;

NTSTATUS UefiQueryVariable(
    _Inout_ PEFI_EDITOR_CONTEXT Context,
    _In_ PPH_STRING Name,
    _In_ PPH_STRING Guid
    )
{
    NTSTATUS status;
    UNICODE_STRING variableNameUs;
    UNICODE_STRING guidStringUs;
    GUID variableGuid;
    PVOID variableValue = NULL;
    ULONG variableValueLength = 0;

    PhStringRefToUnicodeString(&Name->sr, &variableNameUs);
    PhStringRefToUnicodeString(&Guid->sr, &guidStringUs);

    status = RtlGUIDFromString(
        &guidStringUs,
        &variableGuid
        );

    if (!NT_SUCCESS(status))
        return status;

    status = NtQuerySystemEnvironmentValueEx(
        &variableNameUs,
        &variableGuid,
        variableValue,
        &variableValueLength,
        NULL
        );

    if (status != STATUS_BUFFER_TOO_SMALL)
        return status;

    variableValue = PhAllocate(variableValueLength);
    memset(variableValue, 0, variableValueLength);

    status = NtQuerySystemEnvironmentValueEx(
        &variableNameUs,
        &variableGuid,
        variableValue,
        &variableValueLength,
        NULL
        );

    if (NT_SUCCESS(status))
    {
        if (Context->VariableValue)
            PhFree(Context->VariableValue);

        Context->VariableValue = variableValue;
        Context->VariableValueLength = variableValueLength;
    }
    else
    {
        if (Context->VariableValue)
            PhFree(Context->VariableValue);
        Context->VariableValueLength = 0;
    }

    return status;
}

INT_PTR CALLBACK UefiEditorDlgProc(
    _In_ HWND hwndDlg,
    _In_ UINT uMsg,
    _In_ WPARAM wParam,
    _In_ LPARAM lParam
    )
{
    PEFI_EDITOR_CONTEXT context;

    if (uMsg == WM_INITDIALOG)
    {
        context = (PEFI_EDITOR_CONTEXT)lParam;
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

            context->HexEditHandle = GetDlgItem(hwndDlg, IDC_FIRMWARE_HEXEDITVAR);

            PhSetApplicationWindowIcon(hwndDlg);
            PhSetWindowText(hwndDlg, PhGetString(context->Name));

            if (!NT_SUCCESS(status = UefiQueryVariable(context, context->Name, context->GuidString)))
            {
                PhShowStatus(NULL, L"Unable to query the EFI variable.", status, 0);
                return TRUE;
            }

            HexEdit_SetData(context->HexEditHandle, context->VariableValue, context->VariableValueLength);

            PhInitializeLayoutManager(&context->LayoutManager, hwndDlg);
            PhAddLayoutItem(&context->LayoutManager, GetDlgItem(hwndDlg, IDC_FIRMWARE_HEXEDITVAR), NULL, PH_ANCHOR_ALL);
            PhAddLayoutItem(&context->LayoutManager, GetDlgItem(hwndDlg, IDC_FIRMWARE_REREAD), NULL, PH_ANCHOR_BOTTOM | PH_ANCHOR_LEFT);
            PhAddLayoutItem(&context->LayoutManager, GetDlgItem(hwndDlg, IDC_FIRMWARE_WRITE), NULL, PH_ANCHOR_BOTTOM | PH_ANCHOR_LEFT);
            PhAddLayoutItem(&context->LayoutManager, GetDlgItem(hwndDlg, IDC_FIRMWARE_BYTESPERROW), NULL, PH_ANCHOR_BOTTOM | PH_ANCHOR_LEFT);
            PhAddLayoutItem(&context->LayoutManager, GetDlgItem(hwndDlg, IDC_FIRMWARE_SAVE), NULL, PH_ANCHOR_RIGHT | PH_ANCHOR_BOTTOM);
            PhAddLayoutItem(&context->LayoutManager, GetDlgItem(hwndDlg, IDOK), NULL, PH_ANCHOR_RIGHT | PH_ANCHOR_BOTTOM);

            {
                PH_RECTANGLE windowRectangle = {0};
                RECT rect;
                LONG dpiValue;

                windowRectangle.Position = PhGetIntegerPairSetting(L"MemEditPosition");

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

            {
                PWSTR bytesPerRowStrings[7];
                ULONG bytesPerRow;

                for (ULONG i = 0; i < ARRAYSIZE(bytesPerRowStrings); i++)
                    bytesPerRowStrings[i] = PhaFormatString(L"%u bytes per row", 1 << (2 + i))->Buffer;

                PhAddComboBoxStrings(GetDlgItem(hwndDlg, IDC_FIRMWARE_BYTESPERROW), bytesPerRowStrings, ARRAYSIZE(bytesPerRowStrings));

                bytesPerRow = PhGetIntegerSetting(L"MemEditBytesPerRow");

                if (bytesPerRow >= 4)
                {
                    HexEdit_SetBytesPerRow(context->HexEditHandle, bytesPerRow);
                    PhSelectComboBoxString(GetDlgItem(hwndDlg, IDC_FIRMWARE_BYTESPERROW),
                        PhaFormatString(L"%u bytes per row", bytesPerRow)->Buffer, FALSE
                        );
                }
            }

            PhInitializeWindowTheme(hwndDlg, !!PhGetIntegerSetting(L"EnableThemeSupport"));

            SendMessage(hwndDlg, WM_NEXTDLGCTL, (WPARAM)context->HexEditHandle, TRUE);
        }
        break;
    case WM_DESTROY:
        {
            PhRemoveWindowContext(hwndDlg, PH_WINDOW_CONTEXT_DEFAULT);

            PhSaveWindowPlacementToSetting(L"MemEditPosition", L"MemEditSize", hwndDlg);
            PhUnregisterDialog(hwndDlg);

            PhDeleteLayoutManager(&context->LayoutManager);

            PhClearReference(&context->Title);
            PhClearReference(&context->Name);
            PhClearReference(&context->GuidString);

            if (context->VariableValue)
                PhFree(context->VariableValue);

            PhFree(context);
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
            case IDC_FIRMWARE_SAVE:
                {
                    static PH_FILETYPE_FILTER filters[] =
                    {
                        { L"Binary files (*.bin)", L"*.bin" },
                        { L"All files (*.*)", L"*.*" }
                    };
                    PVOID fileDialog;

                    fileDialog = PhCreateSaveFileDialog();

                    PhSetFileDialogFilter(fileDialog, filters, ARRAYSIZE(filters));
                    PhSetFileDialogFileName(fileDialog, PhaFormatString(
                        L"%s.bin",
                        PhGetString(context->Name)
                        )->Buffer);

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
                            status = PhWriteFileStream(fileStream, context->VariableValue, context->VariableValueLength);
                            PhDereferenceObject(fileStream);
                        }

                        if (!NT_SUCCESS(status))
                            PhShowStatus(hwndDlg, L"Unable to create the file", status, 0);
                    }

                    PhFreeFileDialog(fileDialog);
                }
                break;
            case IDC_FIRMWARE_WRITE:
                {
                    NOTHING;
                }
                break;
            case IDC_FIRMWARE_REREAD:
                {
                    NTSTATUS status;

                    if (!NT_SUCCESS(status = UefiQueryVariable(context, context->Name, context->GuidString)))
                    {
                        PhShowStatus(NULL, L"Unable to query the EFI variable.", status, 0);
                        return TRUE;
                    }

                    HexEdit_SetData(context->HexEditHandle, context->VariableValue, context->VariableValueLength);
                    InvalidateRect(context->HexEditHandle, NULL, TRUE);
                }
                break;
            case IDC_FIRMWARE_BYTESPERROW:
                {
                    if (GET_WM_COMMAND_CMD(wParam, lParam) == CBN_SELCHANGE)
                    {
                        PPH_STRING bytesPerRowString = PhaGetDlgItemText(hwndDlg, IDC_FIRMWARE_BYTESPERROW);
                        PH_STRINGREF firstPart;
                        PH_STRINGREF secondPart;
                        ULONG64 bytesPerRow64;

                        if (PhSplitStringRefAtChar(&bytesPerRowString->sr, ' ', &firstPart, &secondPart))
                        {
                            if (PhStringToInteger64(&firstPart, 10, &bytesPerRow64))
                            {
                                PhSetIntegerSetting(L"MemEditBytesPerRow", (ULONG)bytesPerRow64);
                                HexEdit_SetBytesPerRow(context->HexEditHandle, (ULONG)bytesPerRow64);
                                SendMessage(hwndDlg, WM_NEXTDLGCTL, (WPARAM)context->HexEditHandle, TRUE);
                            }
                        }
                    }
                    break;
                }
           }
        }
        break;
    case WM_SIZE:
        {
            PhLayoutManagerLayout(&context->LayoutManager);
        }
        break;
    }

    return FALSE;
}

NTSTATUS UefiEditorDialogThreadStart(
    _In_ PVOID Parameter
    )
{
    BOOL result;
    MSG message;
    HWND windowHandle;
    PH_AUTO_POOL autoPool;

    PhInitializeAutoPool(&autoPool);

    windowHandle = CreateDialogParam(
        PluginInstance->DllBase,
        MAKEINTRESOURCE(IDD_FIRMWARE_EDITVAR),
        NULL,
        UefiEditorDlgProc,
        (LPARAM)Parameter
        );

    ShowWindow(windowHandle, SW_SHOW);
    SetForegroundWindow(windowHandle);

    while (result = GetMessage(&message, NULL, 0, 0))
    {
        if (result == -1)
            break;

        if (!IsDialogMessage(windowHandle, &message))
        {
            TranslateMessage(&message);
            DispatchMessage(&message);
        }

        PhDrainAutoPool(&autoPool);
    }

    PhDeleteAutoPool(&autoPool);

    return STATUS_SUCCESS;
}

VOID ShowUefiEditorDialog(
    _In_ PEFI_ENTRY Entry
    )
{
    PEFI_EDITOR_CONTEXT context;

    context = PhAllocateZero(sizeof(EFI_EDITOR_CONTEXT));
    context->Name = PhDuplicateString(Entry->Name);
    context->GuidString = PhDuplicateString(Entry->GuidString);

    PhCreateThread2(UefiEditorDialogThreadStart, context);
}
