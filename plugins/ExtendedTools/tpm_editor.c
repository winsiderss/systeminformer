/*
 * Copyright (c) 2022 Winsider Seminars & Solutions, Inc.  All rights reserved.
 *
 * This file is part of System Informer.
 *
 * Authors:
 *
 *     jxy-s   2024
 *
 */

#include "exttools.h"
#include <hexedit.h>

typedef struct _ET_TPM_EDITOR_CONTEXT
{
    HWND WindowHandle;
    HWND ParentWindowHandle;
    HWND HexEditHandle;
    HWND BytesPerRowHandle;

    PH_LAYOUT_MANAGER LayoutManager;

    TPM_NV_INDEX Index;
    PBYTE Data;
    USHORT DataSize;
} ET_TPM_EDITOR_CONTEXT, *PET_TPM_EDITOR_CONTEXT;

NTSTATUS EtTpmEditorRead(
    _In_ PET_TPM_EDITOR_CONTEXT Context
    )
{
    NTSTATUS status;
    TBS_HCONTEXT tbsContextHandle;
    TPMA_NV attributes;
    USHORT dataSize;
    PBYTE data = NULL;

    if (!NT_SUCCESS(status = EtTpmOpen(&tbsContextHandle)))
        return status;

    if (!NT_SUCCESS(status = EtTpmReadPublic(tbsContextHandle, Context->Index, &attributes, &dataSize)))
        goto CleanupExit;

    data = PhAllocateZero(dataSize);

    if (!NT_SUCCESS(status = EtTpmRead(tbsContextHandle, Context->Index, data, dataSize)))
        goto CleanupExit;

    if (Context->Data)
        PhFree(Context->Data);

    Context->DataSize = dataSize;
    Context->Data = data;
    data = NULL;

CleanupExit:

    if (data)
        PhFree(data);

    EtTpmClose(tbsContextHandle);

    return status;
}

INT_PTR CALLBACK EtTpmEditorDlgProc(
    _In_ HWND hwndDlg,
    _In_ UINT uMsg,
    _In_ WPARAM wParam,
    _In_ LPARAM lParam
    )
{
    PET_TPM_EDITOR_CONTEXT context;

    if (uMsg == WM_INITDIALOG)
    {
        context = (PET_TPM_EDITOR_CONTEXT)lParam;
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

            context->HexEditHandle = GetDlgItem(hwndDlg, IDC_TPM_HEXEDIT);
            context->BytesPerRowHandle = GetDlgItem(hwndDlg, IDC_TPM_BYTESPERROW);

            PhSetApplicationWindowIcon(hwndDlg);
            PhSetWindowText(hwndDlg, PhaFormatString(L"TPM index 0x%08x", context->Index.Value)->Buffer);

            PhInitializeLayoutManager(&context->LayoutManager, hwndDlg);
            PhAddLayoutItem(&context->LayoutManager, GetDlgItem(hwndDlg, IDC_TPM_HEXEDIT), NULL, PH_ANCHOR_ALL);
            PhAddLayoutItem(&context->LayoutManager, GetDlgItem(hwndDlg, IDC_TPM_REREAD), NULL, PH_ANCHOR_BOTTOM | PH_ANCHOR_LEFT);
            PhAddLayoutItem(&context->LayoutManager, GetDlgItem(hwndDlg, IDC_TPM_WRITE), NULL, PH_ANCHOR_BOTTOM | PH_ANCHOR_LEFT);
            PhAddLayoutItem(&context->LayoutManager, context->BytesPerRowHandle, NULL, PH_ANCHOR_BOTTOM | PH_ANCHOR_LEFT);
            PhAddLayoutItem(&context->LayoutManager, GetDlgItem(hwndDlg, IDC_TPM_SAVE), NULL, PH_ANCHOR_RIGHT | PH_ANCHOR_BOTTOM);
            PhAddLayoutItem(&context->LayoutManager, GetDlgItem(hwndDlg, IDOK), NULL, PH_ANCHOR_RIGHT | PH_ANCHOR_BOTTOM);

            if (!NT_SUCCESS(status = EtTpmEditorRead(context)))
            {
                PhShowStatus(context->ParentWindowHandle, L"Failed to read TPM", status, 0);
                DestroyWindow(hwndDlg);
                return TRUE;
            }

            HexEdit_SetData(context->HexEditHandle, context->Data, context->DataSize);

            {
                PH_RECTANGLE windowRectangle = { 0 };

                windowRectangle.Position = PhGetIntegerPairSetting(L"MemEditPosition");

                if (windowRectangle.Position.X == 0)
                {
                    PhCenterWindow(hwndDlg, context->ParentWindowHandle);
                }
                else
                {
                    RECT rect;
                    LONG dpiValue;

                    PhRectangleToRect(&rect, &windowRectangle);
                    dpiValue = PhGetMonitorDpi(&rect);

                    windowRectangle.Size = PhGetScalableIntegerPairSetting(L"MemEditSize", TRUE, dpiValue)->Pair;
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
                ULONG bytesPerRow;

                for (ULONG i = 0; i < ARRAYSIZE(bytesPerRowStrings); i++)
                    bytesPerRowStrings[i] = PhaFormatString(L"%u bytes per row", 1 << (2 + i))->Buffer;

                PhAddComboBoxStrings(context->BytesPerRowHandle, bytesPerRowStrings, ARRAYSIZE(bytesPerRowStrings));

                bytesPerRow = PhGetIntegerSetting(L"MemEditBytesPerRow");

                if (bytesPerRow >= 4)
                {
                    HexEdit_SetBytesPerRow(context->HexEditHandle, bytesPerRow);
                    PhSelectComboBoxString(context->BytesPerRowHandle, PhaFormatString(
                        L"%u bytes per row", bytesPerRow)->Buffer, FALSE);
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

            PhDeleteLayoutManager(&context->LayoutManager);

            if (context->Data)
                PhFree(context->Data);

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
            case IDC_TPM_SAVE:
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
                        L"TPM-%08x.bin",
                        context->Index.Value
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
                            status = PhWriteFileStream(fileStream, context->Data, context->DataSize);
                            PhDereferenceObject(fileStream);
                        }

                        if (!NT_SUCCESS(status))
                            PhShowStatus(hwndDlg, L"Unable to create the file", status, 0);
                    }

                    PhFreeFileDialog(fileDialog);
                }
                break;
            case IDC_TPM_WRITE:
                {
                    PhShowWarning(NULL, L"Unable to write to the TPM", L"TPM write not yet implemented");
                    return TRUE;
                }
                break;
            case IDC_TPM_REREAD:
                {
                    NTSTATUS status;

                    if (!NT_SUCCESS(status = EtTpmEditorRead(context)))
                    {
                        PhShowStatus(NULL, L"Unable to read TPM", status, 0);
                        return TRUE;
                    }

                    HexEdit_SetData(context->HexEditHandle, context->Data, context->DataSize);
                    InvalidateRect(context->HexEditHandle, NULL, TRUE);
                }
                break;
            case IDC_TPM_BYTESPERROW:
                {
                    if (GET_WM_COMMAND_CMD(wParam, lParam) == CBN_SELCHANGE)
                    {
                        PPH_STRING bytesPerRowString = PhaGetDlgItemText(hwndDlg, IDC_TPM_BYTESPERROW);
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

NTSTATUS EtTpmEditorDialogThreadStart(
    _In_ PVOID Parameter
    )
{
    PET_TPM_EDITOR_CONTEXT context = Parameter;
    BOOL result;
    MSG message;
    HWND windowHandle;
    PH_AUTO_POOL autoPool;

    PhInitializeAutoPool(&autoPool);

    windowHandle = PhCreateDialog(
        PluginInstance->DllBase,
        MAKEINTRESOURCE(IDD_TPM_EDIT),
        !!PhGetIntegerSetting(L"ForceNoParent") ? NULL : context->ParentWindowHandle,
        EtTpmEditorDlgProc,
        context
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

VOID EtShowTpmEditDialog(
    _In_ HWND ParentWindowHandle,
    _In_ TPM_NV_INDEX Index
    )
{
    PET_TPM_EDITOR_CONTEXT context;

    context = PhAllocateZero(sizeof(ET_TPM_EDITOR_CONTEXT));
    context->ParentWindowHandle = ParentWindowHandle;
    context->Index.Value = Index.Value;

    PhCreateThread2(EtTpmEditorDialogThreadStart, context);
}
