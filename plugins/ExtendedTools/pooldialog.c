/*
 * Copyright (c) 2022 Winsider Seminars & Solutions, Inc.  All rights reserved.
 *
 * This file is part of System Informer.
 *
 * Authors:
 *
 *     dmex    2016-2023
 *
 */

#include "exttools.h"
#include "poolmon.h"

static HWND EtPoolTagDialogHandle = NULL;
static HANDLE EtPoolTagDialogThreadHandle = NULL;
static PH_EVENT EtPoolTagDialogInitializedEvent = PH_EVENT_INIT;

NTSTATUS EtEnumPoolTagTable(
    _Out_ PVOID* Buffer
    )
{
    NTSTATUS status;
    PVOID buffer;
    ULONG bufferSize;
    ULONG attempts;

    bufferSize = 0x100;
    buffer = PhAllocate(bufferSize);

    status = NtQuerySystemInformation(
        SystemPoolTagInformation,
        buffer,
        bufferSize,
        &bufferSize
        );
    attempts = 0;

    while (status == STATUS_INFO_LENGTH_MISMATCH && attempts < 8)
    {
        PhFree(buffer);
        buffer = PhAllocate(bufferSize);

        status = NtQuerySystemInformation(
            SystemPoolTagInformation,
            buffer,
            bufferSize,
            &bufferSize
            );
        attempts++;
    }

    if (NT_SUCCESS(status))
        *Buffer = buffer;
    else
        PhFree(buffer);

    return status;
}

VOID EtUpdatePoolTagTable(
    _Inout_ PPOOLTAG_CONTEXT Context
    )
{
    PSYSTEM_POOLTAG_INFORMATION poolTagTable;

    if (!NT_SUCCESS(EtEnumPoolTagTable(&poolTagTable)))
        return;

    for (ULONG i = 0; i < poolTagTable->Count; i++)
    {
        PPOOLTAG_ROOT_NODE node;
        SYSTEM_POOLTAG poolTagInfo;

        poolTagInfo = poolTagTable->TagInfo[i];

        if (node = EtFindPoolTagNode(Context, poolTagInfo.TagUlong))
        {
            PhUpdateDelta(&node->PoolItem->PagedAllocsDelta, poolTagInfo.PagedAllocs);
            PhUpdateDelta(&node->PoolItem->PagedFreesDelta, poolTagInfo.PagedFrees);
            PhUpdateDelta(&node->PoolItem->PagedCurrentDelta, poolTagInfo.PagedAllocs - poolTagInfo.PagedFrees);
            PhUpdateDelta(&node->PoolItem->PagedTotalSizeDelta, poolTagInfo.PagedUsed);
            PhUpdateDelta(&node->PoolItem->NonPagedAllocsDelta, poolTagInfo.NonPagedAllocs);
            PhUpdateDelta(&node->PoolItem->NonPagedFreesDelta, poolTagInfo.NonPagedFrees);
            PhUpdateDelta(&node->PoolItem->NonPagedCurrentDelta, poolTagInfo.NonPagedAllocs - poolTagInfo.NonPagedFrees);
            PhUpdateDelta(&node->PoolItem->NonPagedTotalSizeDelta, poolTagInfo.NonPagedUsed);
            node->PoolItem->HaveFirstSample = TRUE;

            EtUpdatePoolTagNode(Context, node);
        }
        else
        {
            PPOOL_ITEM entry;

            entry = PhAllocateZero(sizeof(POOL_ITEM));
            entry->TagUlong = poolTagInfo.TagUlong;
            PhZeroExtendToUtf16Buffer(poolTagInfo.Tag, sizeof(poolTagInfo.Tag), entry->TagString);

            PhUpdateDelta(&entry->PagedAllocsDelta, poolTagInfo.PagedAllocs);
            PhUpdateDelta(&entry->PagedFreesDelta, poolTagInfo.PagedFrees);
            PhUpdateDelta(&entry->PagedCurrentDelta, poolTagInfo.PagedAllocs - poolTagInfo.PagedFrees);
            PhUpdateDelta(&entry->PagedTotalSizeDelta, poolTagInfo.PagedUsed);
            PhUpdateDelta(&entry->NonPagedAllocsDelta, poolTagInfo.NonPagedAllocs);
            PhUpdateDelta(&entry->NonPagedFreesDelta, poolTagInfo.NonPagedFrees);
            PhUpdateDelta(&entry->NonPagedCurrentDelta, poolTagInfo.NonPagedAllocs - poolTagInfo.NonPagedFrees);
            PhUpdateDelta(&entry->NonPagedTotalSizeDelta, poolTagInfo.NonPagedUsed);

            EtUpdatePoolTagBinaryName(Context, entry, poolTagInfo.TagUlong);

            EtAddPoolTagNode(Context, entry);
        }
    }

    TreeNew_NodesStructured(Context->TreeNewHandle);

    PhFree(poolTagTable);
}

BOOLEAN EtPoolTagTreeFilterCallback(
    _In_ PPH_TREENEW_NODE Node,
    _In_ PVOID Context
    )
{
    PPOOLTAG_ROOT_NODE poolNode = (PPOOLTAG_ROOT_NODE)Node;
    PPOOLTAG_CONTEXT context = Context;

    if (PhIsNullOrEmptyString(context->SearchboxText))
        return TRUE;

    if (poolNode->PoolItem->TagString[0] != 0)
    {
        if (PhWordMatchStringZ(context->SearchboxText, poolNode->PoolItem->TagString))
            return TRUE;
    }

    if (!PhIsNullOrEmptyString(poolNode->PoolItem->BinaryNameString))
    {
        if (PhWordMatchStringRef(&context->SearchboxText->sr, &poolNode->PoolItem->BinaryNameString->sr))
            return TRUE;
    }

    if (!PhIsNullOrEmptyString(poolNode->PoolItem->DescriptionString))
    {
        if (PhWordMatchStringRef(&context->SearchboxText->sr, &poolNode->PoolItem->DescriptionString->sr))
            return TRUE;
    }

    return FALSE;
}

VOID NTAPI EtPoolMonProcessesUpdatedCallback(
    _In_opt_ PVOID Parameter,
    _In_ PVOID Context
    )
{
    PPOOLTAG_CONTEXT context = Context;

    if (context->ProcessesUpdatedCount < 2)
    {
        context->ProcessesUpdatedCount++;
        return;
    }

    EtUpdatePoolTagTable(Context);
}

INT_PTR CALLBACK EtPoolMonDlgProc(
    _In_ HWND hwndDlg,
    _In_ UINT uMsg,
    _In_ WPARAM wParam,
    _In_ LPARAM lParam
    )
{
    PPOOLTAG_CONTEXT context;

    if (uMsg == WM_INITDIALOG)
    {
        context = PhAllocateZero(sizeof(POOLTAG_CONTEXT));
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
            context->WindowHandle = hwndDlg;
            context->ParentWindowHandle = (HWND)lParam;
            context->TreeNewHandle = GetDlgItem(hwndDlg, IDC_POOLTREE);
            context->SearchboxHandle = GetDlgItem(hwndDlg, IDC_POOLSEARCH);

            PhSetApplicationWindowIcon(hwndDlg);

            PhCreateSearchControl(hwndDlg, context->SearchboxHandle, L"Search Pool Tags (Ctrl+K)");
            EtInitializePoolTagTree(context);

            PhInitializeLayoutManager(&context->LayoutManager, hwndDlg);
            PhAddLayoutItem(&context->LayoutManager, context->TreeNewHandle, NULL, PH_ANCHOR_ALL);
            PhAddLayoutItem(&context->LayoutManager, context->SearchboxHandle, NULL, PH_ANCHOR_BOTTOM | PH_ANCHOR_LEFT);
            PhAddLayoutItem(&context->LayoutManager, GetDlgItem(hwndDlg, IDC_POOL_AUTOSIZE_COLUMNS), NULL, PH_ANCHOR_BOTTOM | PH_ANCHOR_LEFT);
            PhAddLayoutItem(&context->LayoutManager, GetDlgItem(hwndDlg, IDCANCEL), NULL, PH_ANCHOR_BOTTOM | PH_ANCHOR_RIGHT);

            if (PhGetIntegerPairSetting(SETTING_NAME_POOL_WINDOW_POSITION).X != 0)
                PhLoadWindowPlacementFromSetting(SETTING_NAME_POOL_WINDOW_POSITION, SETTING_NAME_POOL_WINDOW_SIZE, hwndDlg);
            else
                PhCenterWindow(hwndDlg, context->ParentWindowHandle);

            context->SearchboxText = PhReferenceEmptyString();
            context->TreeFilterEntry = PhAddTreeNewFilter(
                &context->FilterSupport,
                EtPoolTagTreeFilterCallback,
                context
                );

            EtLoadPoolTagDatabase(context);
            EtUpdatePoolTagTable(context);

            //TreeNew_AutoSizeColumn(context->TreeNewHandle, TREE_COLUMN_ITEM_DESCRIPTION, TN_AUTOSIZE_REMAINING_SPACE);

            PhRegisterCallback(
                PhGetGeneralCallback(GeneralCallbackProcessProviderUpdatedEvent),
                EtPoolMonProcessesUpdatedCallback,
                context,
                &context->ProcessesUpdatedCallbackRegistration
                );

            PhInitializeWindowTheme(hwndDlg, !!PhGetIntegerSetting(L"EnableThemeSupport"));

            SendMessage(hwndDlg, WM_NEXTDLGCTL, (WPARAM)context->TreeNewHandle, TRUE);
        }
        break;
    case WM_SIZE:
        {
            PhLayoutManagerLayout(&context->LayoutManager);

            if (context->TreeNewAutoSize)
            {
                TreeNew_AutoSizeColumn(context->TreeNewHandle, TREE_COLUMN_ITEM_DESCRIPTION, TN_AUTOSIZE_REMAINING_SPACE);
            }
        }
        break;
    case WM_DESTROY:
        {
            PhRemoveWindowContext(hwndDlg, PH_WINDOW_CONTEXT_DEFAULT);

            PhUnregisterCallback(PhGetGeneralCallback(GeneralCallbackProcessProviderUpdatedEvent), &context->ProcessesUpdatedCallbackRegistration);

            PhSaveWindowPlacementToSetting(SETTING_NAME_POOL_WINDOW_POSITION, SETTING_NAME_POOL_WINDOW_SIZE, hwndDlg);

            EtSaveSettingsPoolTagTreeList(context);

            PhDeleteLayoutManager(&context->LayoutManager);
            PhDeleteTreeNewFilterSupport(&context->FilterSupport);

            EtDeletePoolTagTree(context);
            EtFreePoolTagDatabase(context);

            if (context->SearchboxText)
                PhDereferenceObject(context->SearchboxText);

            PhFree(context);

            PostQuitMessage(0);
        }
        break;
    case WM_PH_SHOW_DIALOG:
        {
            if (IsMinimized(hwndDlg))
                ShowWindow(hwndDlg, SW_RESTORE);
            else
                ShowWindow(hwndDlg, SW_SHOW);

            SetForegroundWindow(hwndDlg);
        }
        break;
    case WM_COMMAND:
        {
            switch (GET_WM_COMMAND_ID(wParam, lParam))
            {
            case IDCANCEL:
                DestroyWindow(hwndDlg);
                break;
            case IDC_POOLCLEAR:
                {
                    SetFocus(context->SearchboxHandle);
                    Static_SetText(context->SearchboxHandle, L"");
                }
                break;
            case IDC_POOLSEARCH:
                {
                    PPH_STRING newSearchboxText;

                    if (GET_WM_COMMAND_CMD(wParam, lParam) != EN_CHANGE)
                        break;

                    newSearchboxText = PH_AUTO(PhGetWindowText(context->SearchboxHandle));

                    if (!PhEqualString(context->SearchboxText, newSearchboxText, FALSE))
                    {
                        // Cache the current search text for our callback.
                        PhSwapReference(&context->SearchboxText, newSearchboxText);

                        PhApplyTreeNewFilters(&context->FilterSupport);
                    }
                }
                break;
            case IDC_POOL_AUTOSIZE_COLUMNS:
                {
                    context->TreeNewAutoSize = !context->TreeNewAutoSize;
                }
                break;
            case WM_PH_UPDATE_DIALOG:
                {
                    PPH_TREENEW_CONTEXT_MENU contextMenuEvent = (PPH_TREENEW_CONTEXT_MENU)lParam;
                    PPH_EMENU menu;
                    PPOOLTAG_ROOT_NODE selectedNode;
                    PPH_EMENU_ITEM selectedItem;

                    if (selectedNode = EtGetSelectedPoolTagNode(context))
                    {
                        menu = PhCreateEMenu();
                        PhInsertEMenuItem(menu, PhCreateEMenuItem(0, 1, L"Show allocations", NULL, NULL), ULONG_MAX);
                        PhInsertEMenuItem(menu, PhCreateEMenuSeparator(), ULONG_MAX);
                        PhInsertEMenuItem(menu, PhCreateEMenuItem(0, 2, L"&Copy\bCtrl+C", NULL, NULL), ULONG_MAX);
                        PhInsertCopyCellEMenuItem(menu, 2, context->TreeNewHandle, contextMenuEvent->Column);

                        selectedItem = PhShowEMenu(
                            menu,
                            hwndDlg,
                            PH_EMENU_SHOW_LEFTRIGHT,
                            PH_ALIGN_LEFT | PH_ALIGN_TOP,
                            contextMenuEvent->Location.x,
                            contextMenuEvent->Location.y
                            );

                        if (selectedItem && selectedItem->Id != ULONG_MAX)
                        {
                            if (!PhHandleCopyCellEMenuItem(selectedItem))
                            {
                                switch (selectedItem->Id)
                                {
                                case 1:
                                    EtShowBigPoolDialog(selectedNode->PoolItem);
                                    break;
                                case 2:
                                    EtCopyPoolTagTree(context);
                                    break;
                                }
                            }
                        }

                        PhDestroyEMenu(menu);
                    }
                }
                break;
            case WM_PH_SHOW_DIALOG:
                {
                    PPOOLTAG_ROOT_NODE selectedNode;

                    if (selectedNode = EtGetSelectedPoolTagNode(context))
                    {
                        EtShowBigPoolDialog(selectedNode->PoolItem);
                    }
                }
                break;
            }
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

NTSTATUS EtShowPoolMonDialogThread(
    _In_ PVOID Parameter
    )
{
    BOOL result;
    MSG message;
    PH_AUTO_POOL autoPool;

    PhInitializeAutoPool(&autoPool);

    EtPoolTagDialogHandle = PhCreateDialog(
        PluginInstance->DllBase,
        MAKEINTRESOURCE(IDD_POOL),
        NULL,
        EtPoolMonDlgProc,
        Parameter
        );

    PhSetEvent(&EtPoolTagDialogInitializedEvent);

    PostMessage(EtPoolTagDialogHandle, WM_PH_SHOW_DIALOG, 0, 0);

    while (result = GetMessage(&message, NULL, 0, 0))
    {
        if (result == -1)
            break;

        if (!IsDialogMessage(EtPoolTagDialogHandle, &message))
        {
            TranslateMessage(&message);
            DispatchMessage(&message);
        }

        PhDrainAutoPool(&autoPool);
    }

    PhDeleteAutoPool(&autoPool);

    if (EtPoolTagDialogThreadHandle)
    {
        NtClose(EtPoolTagDialogThreadHandle);
        EtPoolTagDialogThreadHandle = NULL;
    }

    PhResetEvent(&EtPoolTagDialogInitializedEvent);

    return STATUS_SUCCESS;
}

VOID EtShowPoolTableDialog(
    _In_ HWND ParentWindowHandle
    )
{
    if (!EtPoolTagDialogThreadHandle)
    {
        if (!NT_SUCCESS(PhCreateThreadEx(&EtPoolTagDialogThreadHandle, EtShowPoolMonDialogThread, ParentWindowHandle)))
        {
            PhShowError(ParentWindowHandle, L"%s", L"Unable to create the window.");
            return;
        }

        PhWaitForEvent(&EtPoolTagDialogInitializedEvent, NULL);
    }

    PostMessage(EtPoolTagDialogHandle, WM_PH_SHOW_DIALOG, 0, 0);
}
