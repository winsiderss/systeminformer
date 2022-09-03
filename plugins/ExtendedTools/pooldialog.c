/*
 * Copyright (c) 2022 Winsider Seminars & Solutions, Inc.  All rights reserved.
 *
 * This file is part of System Informer.
 *
 * Authors:
 *
 *     dmex    2016
 *
 */

#include "exttools.h"
#include "poolmon.h"
#include "commonutil.h"

static HWND PoolTagDialogHandle = NULL;
static HANDLE PoolTagDialogThreadHandle = NULL;
static PH_EVENT PoolTagDialogInitializedEvent = PH_EVENT_INIT;

VOID UpdatePoolTagTable(
    _Inout_ PPOOLTAG_CONTEXT Context
    )
{
    PSYSTEM_POOLTAG_INFORMATION poolTagTable;
    ULONG i;
    
    if (!NT_SUCCESS(EnumPoolTagTable(&poolTagTable)))
    {
        PhDereferenceObject(Context);
        return;
    }
    
    for (i = 0; i < poolTagTable->Count; i++)
    {
        PPOOLTAG_ROOT_NODE node;
        SYSTEM_POOLTAG poolTagInfo;

        poolTagInfo = poolTagTable->TagInfo[i];
    
        if (node = PmFindPoolTagNode(Context, poolTagInfo.TagUlong))
        {
            PhUpdateDelta(&node->PoolItem->PagedAllocsDelta, poolTagInfo.PagedAllocs);
            PhUpdateDelta(&node->PoolItem->PagedFreesDelta, poolTagInfo.PagedFrees);
            PhUpdateDelta(&node->PoolItem->PagedCurrentDelta, poolTagInfo.PagedAllocs - poolTagInfo.PagedFrees);
            PhUpdateDelta(&node->PoolItem->PagedTotalSizeDelta, poolTagInfo.PagedUsed);     
            PhUpdateDelta(&node->PoolItem->NonPagedAllocsDelta, poolTagInfo.NonPagedAllocs);
            PhUpdateDelta(&node->PoolItem->NonPagedFreesDelta, poolTagInfo.NonPagedFrees);
            PhUpdateDelta(&node->PoolItem->NonPagedCurrentDelta, poolTagInfo.NonPagedAllocs - poolTagInfo.NonPagedFrees);
            PhUpdateDelta(&node->PoolItem->NonPagedTotalSizeDelta, poolTagInfo.NonPagedUsed);

            PmUpdatePoolTagNode(Context, node);
        } 
        else
        {
            PPOOL_ITEM entry;

            entry = PhCreateAlloc(sizeof(POOL_ITEM));
            memset(entry, 0, sizeof(POOL_ITEM));

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

            UpdatePoolTagBinaryName(Context, entry, poolTagInfo.TagUlong);

            PmAddPoolTagNode(Context, entry);
        }
    }

    TreeNew_NodesStructured(Context->TreeNewHandle);
    PhDereferenceObject(Context);

    PhFree(poolTagTable);
}

BOOLEAN WordMatchStringRef(
    _Inout_ PPOOLTAG_CONTEXT Context,
    _In_ PPH_STRINGREF Text
    )
{
    PH_STRINGREF part;
    PH_STRINGREF remainingPart;

    remainingPart = Context->SearchboxText->sr;

    while (remainingPart.Length != 0)
    {
        PhSplitStringRefAtChar(&remainingPart, '|', &part, &remainingPart);

        if (part.Length != 0)
        {
            if (PhFindStringInStringRef(Text, &part, TRUE) != -1)
                return TRUE;
        }
    }

    return FALSE;
}

BOOLEAN WordMatchStringZ(
    _Inout_ PPOOLTAG_CONTEXT Context,
    _In_ PWSTR Text
    )
{
    PH_STRINGREF text;

    PhInitializeStringRef(&text, Text);
    return WordMatchStringRef(Context, &text);
}

BOOLEAN PoolTagTreeFilterCallback(
    _In_ PPH_TREENEW_NODE Node,
    _In_opt_ PVOID Context
    )
{
    PPOOLTAG_CONTEXT context = Context;
    PPOOLTAG_ROOT_NODE poolNode = (PPOOLTAG_ROOT_NODE)Node;

    if (PhIsNullOrEmptyString(context->SearchboxText))
        return TRUE;

    if (poolNode->PoolItem->TagString[0] != 0)
    {
        if (WordMatchStringZ(context, poolNode->PoolItem->TagString))
            return TRUE;
    }

    if (!PhIsNullOrEmptyString(poolNode->PoolItem->BinaryNameString))
    {
        if (WordMatchStringRef(context, &poolNode->PoolItem->BinaryNameString->sr))
            return TRUE;
    }

    if (!PhIsNullOrEmptyString(poolNode->PoolItem->DescriptionString))
    {
        if (WordMatchStringRef(context, &poolNode->PoolItem->DescriptionString->sr))
            return TRUE;
    }

    return FALSE;
}

VOID NTAPI PoolmonProcessesUpdatedCallback(
    _In_opt_ PVOID Parameter,
    _In_opt_ PVOID Context
    )
{
    PPOOLTAG_CONTEXT context = Context;

    context->ProcessesUpdatedCount++;

    if (context->ProcessesUpdatedCount < 2)
        return;
 
    PhReferenceObject(context);
    UpdatePoolTagTable(Context);
}

INT_PTR CALLBACK PoolMonDlgProc(
    _In_ HWND hwndDlg,
    _In_ UINT uMsg,
    _In_ WPARAM wParam,
    _In_ LPARAM lParam
    )
{
    PPOOLTAG_CONTEXT context;

    if (uMsg == WM_INITDIALOG)
    {
        context = PhCreateAlloc(sizeof(POOLTAG_CONTEXT));
        memset(context, 0, sizeof(POOLTAG_CONTEXT));

        PhSetWindowContext(hwndDlg, PH_WINDOW_CONTEXT_DEFAULT, context);
    }
    else
    {
        context = PhGetWindowContext(hwndDlg, PH_WINDOW_CONTEXT_DEFAULT);

        if (uMsg == WM_DESTROY)
        {
            PhRemoveWindowContext(hwndDlg, PH_WINDOW_CONTEXT_DEFAULT);
            PhDereferenceObject(context);
        }
    }

    if (!context)
        return FALSE;

    switch (uMsg)
    {
    case WM_INITDIALOG:
        {
            PhSetApplicationWindowIcon(hwndDlg);

            context->ParentWindowHandle = hwndDlg;
            context->TreeNewHandle = GetDlgItem(hwndDlg, IDC_POOLTREE);
            context->SearchboxHandle = GetDlgItem(hwndDlg, IDC_POOLSEARCH);

            PhRegisterDialog(hwndDlg);
            PhCenterWindow(hwndDlg, PhMainWndHandle);

            PhCreateSearchControl(hwndDlg, context->SearchboxHandle, L"Search Pool Tags (Ctrl+K)");
            PmInitializePoolTagTree(context);

            PhInitializeLayoutManager(&context->LayoutManager, hwndDlg);
            PhAddLayoutItem(&context->LayoutManager, context->TreeNewHandle, NULL, PH_ANCHOR_ALL);
            PhAddLayoutItem(&context->LayoutManager, context->SearchboxHandle, NULL, PH_ANCHOR_BOTTOM | PH_ANCHOR_LEFT);         
            PhAddLayoutItem(&context->LayoutManager, GetDlgItem(hwndDlg, IDCANCEL), NULL, PH_ANCHOR_BOTTOM | PH_ANCHOR_RIGHT);
            PhLoadWindowPlacementFromSetting(SETTING_NAME_POOL_WINDOW_POSITION, SETTING_NAME_POOL_WINDOW_SIZE, hwndDlg);
            
            context->SearchboxText = PhReferenceEmptyString();
            context->TreeFilterEntry = PhAddTreeNewFilter(
                &context->FilterSupport,
                PoolTagTreeFilterCallback,
                context
                );

            LoadPoolTagDatabase(context);

            PhReferenceObject(context);
            UpdatePoolTagTable(context);
            TreeNew_AutoSizeColumn(context->TreeNewHandle, TREE_COLUMN_ITEM_DESCRIPTION, TN_AUTOSIZE_REMAINING_SPACE);

            PhRegisterCallback(
                PhGetGeneralCallback(GeneralCallbackProcessProviderUpdatedEvent),
                PoolmonProcessesUpdatedCallback,
                context,
                &context->ProcessesUpdatedCallbackRegistration
                );          
                
            SendMessage(hwndDlg, WM_NEXTDLGCTL, (WPARAM)context->TreeNewHandle, TRUE);
        }
        break;
    case WM_SIZE:
        PhLayoutManagerLayout(&context->LayoutManager);
        TreeNew_AutoSizeColumn(context->TreeNewHandle, TREE_COLUMN_ITEM_DESCRIPTION, TN_AUTOSIZE_REMAINING_SPACE);
        break;
    case WM_DESTROY:
        {
            PhUnregisterCallback(
                PhGetGeneralCallback(GeneralCallbackProcessProviderUpdatedEvent),
                &context->ProcessesUpdatedCallbackRegistration
                );

            PhSaveWindowPlacementToSetting(SETTING_NAME_POOL_WINDOW_POSITION, SETTING_NAME_POOL_WINDOW_SIZE, hwndDlg);
            PmSaveSettingsTreeList(context);

            PhDeleteTreeNewFilterSupport(&context->FilterSupport);

            PmDeletePoolTagTree(context);
            FreePoolTagDatabase(context);

            PhDeleteLayoutManager(&context->LayoutManager);
            PhUnregisterDialog(hwndDlg);

            PostQuitMessage(0);
        }
        break;
    case POOL_TABLE_SHOWDIALOG:
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
            case POOL_TABLE_SHOWCONTEXTMENU:
                {
                    PPH_EMENU menu;
                    PPOOLTAG_ROOT_NODE selectedNode;
                    PPH_EMENU_ITEM selectedItem;
                    PPH_TREENEW_CONTEXT_MENU contextMenuEvent = (PPH_TREENEW_CONTEXT_MENU)lParam;

                    if (selectedNode = PmGetSelectedPoolTagNode(context))
                    {
                        menu = PhCreateEMenu();
                        PhInsertEMenuItem(menu, PhCreateEMenuItem(0, 1, L"Show allocations", NULL, NULL), -1);
                        PhInsertEMenuItem(menu, PhCreateEMenuSeparator(), -1);
                        PhInsertEMenuItem(menu, PhCreateEMenuItem(0, 2, L"Edit description...", NULL, NULL), -1);
                        
                        selectedItem = PhShowEMenu(
                            menu,
                            hwndDlg,
                            PH_EMENU_SHOW_LEFTRIGHT,
                            PH_ALIGN_LEFT | PH_ALIGN_TOP,
                            contextMenuEvent->Location.x,
                            contextMenuEvent->Location.y
                            );

                        if (selectedItem && selectedItem->Id != -1)
                        {
                            switch (selectedItem->Id)
                            {
                            case 1:
                                ShowBigPoolDialog(selectedNode->PoolItem);
                                break;
                            }
                        }

                        PhDestroyEMenu(menu);
                    }
                }
                break;
            }
        }
        break;
    }

    return FALSE;
}

NTSTATUS ShowPoolMonDialogThread(
    _In_ PVOID Parameter
    )
{
    BOOL result;
    MSG message;
    PH_AUTO_POOL autoPool;

    PhInitializeAutoPool(&autoPool);

    PoolTagDialogHandle = CreateDialog(
        PluginInstance->DllBase,
        MAKEINTRESOURCE(IDD_POOL),
        NULL,
        PoolMonDlgProc
        );

    PhSetEvent(&PoolTagDialogInitializedEvent);

    PostMessage(PoolTagDialogHandle, POOL_TABLE_SHOWDIALOG, 0, 0);

    while (result = GetMessage(&message, NULL, 0, 0))
    {
        if (result == -1)
            break;

        if (!IsDialogMessage(PoolTagDialogHandle, &message))
        {
            TranslateMessage(&message);
            DispatchMessage(&message);
        }

        PhDrainAutoPool(&autoPool);
    }

    PhDeleteAutoPool(&autoPool);

    if (PoolTagDialogThreadHandle)
    {
        NtClose(PoolTagDialogThreadHandle);
        PoolTagDialogThreadHandle = NULL;
    }

    PhResetEvent(&PoolTagDialogInitializedEvent);

    return STATUS_SUCCESS;
}

VOID ShowPoolMonDialog(
    VOID
    )
{
    if (!PoolTagDialogThreadHandle)
    {
        if (!NT_SUCCESS(PhCreateThreadEx(&PoolTagDialogThreadHandle, ShowPoolMonDialogThread, NULL)))
        {
            PhShowError(PhMainWndHandle, L"%s", L"Unable to create the pool monitor window.");
            return;
        }

        PhWaitForEvent(&PoolTagDialogInitializedEvent, NULL);
    }

    PostMessage(PoolTagDialogHandle, POOL_TABLE_SHOWDIALOG, 0, 0);
}
