/*
 * Process Hacker -
 *   Process properties: Memory page
 *
 * Copyright (C) 2009-2016 wj32
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
#include <cpysave.h>
#include <emenu.h>
#include <procprv.h>
#include <memprv.h>
#include <memlist.h>
#include <actions.h>
#include <phplug.h>
#include <extmgri.h>
#include <windowsx.h>
#include <procprpp.h>

static PH_STRINGREF EmptyMemoryText = PH_STRINGREF_INIT(L"There are no memory regions to display.");

VOID PhpRefreshProcessMemoryList(
    _In_ HWND hwndDlg,
    _In_ PPH_PROCESS_PROPPAGECONTEXT PropPageContext
    )
{
    PPH_MEMORY_CONTEXT memoryContext = PropPageContext->Context;

    if (memoryContext->MemoryItemListValid)
    {
        PhDeleteMemoryItemList(&memoryContext->MemoryItemList);
        memoryContext->MemoryItemListValid = FALSE;
    }

    memoryContext->LastRunStatus = PhQueryMemoryItemList(
        memoryContext->ProcessId,
        PH_QUERY_MEMORY_REGION_TYPE | PH_QUERY_MEMORY_WS_COUNTERS,
        &memoryContext->MemoryItemList
        );

    if (NT_SUCCESS(memoryContext->LastRunStatus))
    {
        if (PhPluginsEnabled)
        {
            PH_PLUGIN_MEMORY_ITEM_LIST_CONTROL control;

            control.Type = PluginMemoryItemListInitialized;
            control.u.Initialized.List = &memoryContext->MemoryItemList;

            PhInvokeCallback(PhGetGeneralCallback(GeneralCallbackMemoryItemListControl), &control);
        }

        memoryContext->MemoryItemListValid = TRUE;
        TreeNew_SetEmptyText(memoryContext->ListContext.TreeNewHandle, &EmptyMemoryText, 0);
        PhReplaceMemoryList(&memoryContext->ListContext, &memoryContext->MemoryItemList);
    }
    else
    {
        PPH_STRING message;

        message = PhGetStatusMessage(memoryContext->LastRunStatus, 0);
        PhMoveReference(&memoryContext->ErrorMessage, PhFormatString(L"Unable to query memory information:\n%s", PhGetStringOrDefault(message, L"Unknown error.")));
        PhClearReference(&message);
        TreeNew_SetEmptyText(memoryContext->ListContext.TreeNewHandle, &memoryContext->ErrorMessage->sr, 0);

        PhReplaceMemoryList(&memoryContext->ListContext, NULL);
    }
}

VOID PhpInitializeMemoryMenu(
    _In_ PPH_EMENU Menu,
    _In_ HANDLE ProcessId,
    _In_ PPH_MEMORY_NODE *MemoryNodes,
    _In_ ULONG NumberOfMemoryNodes
    )
{
    if (NumberOfMemoryNodes == 0)
    {
        PhSetFlagsAllEMenuItems(Menu, PH_EMENU_DISABLED, PH_EMENU_DISABLED);
    }
    else if (NumberOfMemoryNodes == 1 && !MemoryNodes[0]->IsAllocationBase)
    {
        if (MemoryNodes[0]->MemoryItem->State & MEM_FREE)
        {
            PhEnableEMenuItem(Menu, ID_MEMORY_CHANGEPROTECTION, FALSE);
            PhEnableEMenuItem(Menu, ID_MEMORY_FREE, FALSE);
            PhEnableEMenuItem(Menu, ID_MEMORY_DECOMMIT, FALSE);
        }
        else if (MemoryNodes[0]->MemoryItem->Type & (MEM_MAPPED | MEM_IMAGE))
        {
            PhEnableEMenuItem(Menu, ID_MEMORY_DECOMMIT, FALSE);
        }
    }
    else
    {
        ULONG i;
        ULONG numberOfAllocationBase = 0;

        PhSetFlagsAllEMenuItems(Menu, PH_EMENU_DISABLED, PH_EMENU_DISABLED);
        PhEnableEMenuItem(Menu, ID_MEMORY_COPY, TRUE);

        for (i = 0; i < NumberOfMemoryNodes; i++)
        {
            if (MemoryNodes[i]->IsAllocationBase)
                numberOfAllocationBase++;
        }

        if (numberOfAllocationBase == 0 || numberOfAllocationBase == NumberOfMemoryNodes)
            PhEnableEMenuItem(Menu, ID_MEMORY_SAVE, TRUE);
    }

    PhEnableEMenuItem(Menu, ID_MEMORY_READWRITEADDRESS, TRUE);
}

VOID PhShowMemoryContextMenu(
    _In_ HWND hwndDlg,
    _In_ PPH_PROCESS_ITEM ProcessItem,
    _In_ PPH_MEMORY_CONTEXT Context,
    _In_ PPH_TREENEW_CONTEXT_MENU ContextMenu
    )
{
    PPH_MEMORY_NODE *memoryNodes;
    ULONG numberOfMemoryNodes;

    PhGetSelectedMemoryNodes(&Context->ListContext, &memoryNodes, &numberOfMemoryNodes);

    //if (numberOfMemoryNodes != 0)
    {
        PPH_EMENU menu;
        PPH_EMENU_ITEM item;
        PH_PLUGIN_MENU_INFORMATION menuInfo;

        menu = PhCreateEMenu();
        PhLoadResourceEMenuItem(menu, PhInstanceHandle, MAKEINTRESOURCE(IDR_MEMORY), 0);
        PhSetFlagsEMenuItem(menu, ID_MEMORY_READWRITEMEMORY, PH_EMENU_DEFAULT, PH_EMENU_DEFAULT);

        PhpInitializeMemoryMenu(menu, ProcessItem->ProcessId, memoryNodes, numberOfMemoryNodes);
        PhInsertCopyCellEMenuItem(menu, ID_MEMORY_COPY, Context->ListContext.TreeNewHandle, ContextMenu->Column);

        if (PhPluginsEnabled)
        {
            PhPluginInitializeMenuInfo(&menuInfo, menu, hwndDlg, 0);
            menuInfo.u.Memory.ProcessId = ProcessItem->ProcessId;
            menuInfo.u.Memory.MemoryNodes = memoryNodes;
            menuInfo.u.Memory.NumberOfMemoryNodes = numberOfMemoryNodes;

            PhInvokeCallback(PhGetGeneralCallback(GeneralCallbackMemoryMenuInitializing), &menuInfo);
        }

        item = PhShowEMenu(
            menu,
            hwndDlg,
            PH_EMENU_SHOW_LEFTRIGHT,
            PH_ALIGN_LEFT | PH_ALIGN_TOP,
            ContextMenu->Location.x,
            ContextMenu->Location.y
            );

        if (item)
        {
            BOOLEAN handled = FALSE;

            handled = PhHandleCopyCellEMenuItem(item);

            if (!handled && PhPluginsEnabled)
                handled = PhPluginTriggerEMenuItem(&menuInfo, item);

            if (!handled)
                SendMessage(hwndDlg, WM_COMMAND, item->Id, 0);
        }

        PhDestroyEMenu(menu);
    }

    PhFree(memoryNodes);
}

INT_PTR CALLBACK PhpProcessMemoryDlgProc(
    _In_ HWND hwndDlg,
    _In_ UINT uMsg,
    _In_ WPARAM wParam,
    _In_ LPARAM lParam
    )
{
    LPPROPSHEETPAGE propSheetPage;
    PPH_PROCESS_PROPPAGECONTEXT propPageContext;
    PPH_PROCESS_ITEM processItem;
    PPH_MEMORY_CONTEXT memoryContext;
    HWND tnHandle;

    if (PhpPropPageDlgProcHeader(hwndDlg, uMsg, lParam,
        &propSheetPage, &propPageContext, &processItem))
    {
        memoryContext = (PPH_MEMORY_CONTEXT)propPageContext->Context;

        if (memoryContext)
            tnHandle = memoryContext->ListContext.TreeNewHandle;
    }
    else
    {
        return FALSE;
    }

    switch (uMsg)
    {
    case WM_INITDIALOG:
        {
            memoryContext = propPageContext->Context =
                PhAllocate(PhEmGetObjectSize(EmMemoryContextType, sizeof(PH_MEMORY_CONTEXT)));
            memset(memoryContext, 0, sizeof(PH_MEMORY_CONTEXT));
            memoryContext->ProcessId = processItem->ProcessId;

            // Initialize the list.
            tnHandle = GetDlgItem(hwndDlg, IDC_LIST);
            BringWindowToTop(tnHandle);
            PhInitializeMemoryList(hwndDlg, tnHandle, &memoryContext->ListContext);
            TreeNew_SetEmptyText(tnHandle, &PhpLoadingText, 0);
            memoryContext->LastRunStatus = -1;
            memoryContext->ErrorMessage = NULL;

            PhEmCallObjectOperation(EmMemoryContextType, memoryContext, EmObjectCreate);

            if (PhPluginsEnabled)
            {
                PH_PLUGIN_TREENEW_INFORMATION treeNewInfo;

                treeNewInfo.TreeNewHandle = tnHandle;
                treeNewInfo.CmData = &memoryContext->ListContext.Cm;
                treeNewInfo.SystemContext = memoryContext;
                PhInvokeCallback(PhGetGeneralCallback(GeneralCallbackMemoryTreeNewInitializing), &treeNewInfo);
            }

            PhLoadSettingsMemoryList(&memoryContext->ListContext);
            PhSetOptionsMemoryList(&memoryContext->ListContext, TRUE);
            Button_SetCheck(GetDlgItem(hwndDlg, IDC_HIDEFREEREGIONS),
                memoryContext->ListContext.HideFreeRegions ? BST_CHECKED : BST_UNCHECKED);

            PhpRefreshProcessMemoryList(hwndDlg, propPageContext);
        }
        break;
    case WM_DESTROY:
        {
            PhEmCallObjectOperation(EmMemoryContextType, memoryContext, EmObjectDelete);

            if (PhPluginsEnabled)
            {
                PH_PLUGIN_TREENEW_INFORMATION treeNewInfo;

                treeNewInfo.TreeNewHandle = tnHandle;
                treeNewInfo.CmData = &memoryContext->ListContext.Cm;
                PhInvokeCallback(PhGetGeneralCallback(GeneralCallbackMemoryTreeNewUninitializing), &treeNewInfo);
            }

            PhSaveSettingsMemoryList(&memoryContext->ListContext);
            PhDeleteMemoryList(&memoryContext->ListContext);

            if (memoryContext->MemoryItemListValid)
                PhDeleteMemoryItemList(&memoryContext->MemoryItemList);

            PhClearReference(&memoryContext->ErrorMessage);
            PhFree(memoryContext);

            PhpPropPageDlgProcDestroy(hwndDlg);
        }
        break;
    case WM_SHOWWINDOW:
        {
            if (!propPageContext->LayoutInitialized)
            {
                PPH_LAYOUT_ITEM dialogItem;

                dialogItem = PhAddPropPageLayoutItem(hwndDlg, hwndDlg,
                    PH_PROP_PAGE_TAB_CONTROL_PARENT, PH_ANCHOR_ALL);
                PhAddPropPageLayoutItem(hwndDlg, GetDlgItem(hwndDlg, IDC_STRINGS),
                    dialogItem, PH_ANCHOR_TOP | PH_ANCHOR_RIGHT);
                PhAddPropPageLayoutItem(hwndDlg, GetDlgItem(hwndDlg, IDC_REFRESH),
                    dialogItem, PH_ANCHOR_TOP | PH_ANCHOR_RIGHT);
                PhAddPropPageLayoutItem(hwndDlg, memoryContext->ListContext.TreeNewHandle,
                    dialogItem, PH_ANCHOR_ALL);

                PhDoPropPageLayout(hwndDlg);

                propPageContext->LayoutInitialized = TRUE;
            }
        }
        break;
    case WM_COMMAND:
        {
            switch (LOWORD(wParam))
            {
            case ID_SHOWCONTEXTMENU:
                {
                    PhShowMemoryContextMenu(hwndDlg, processItem, memoryContext, (PPH_TREENEW_CONTEXT_MENU)lParam);
                }
                break;
            case ID_MEMORY_READWRITEMEMORY:
                {
                    PPH_MEMORY_NODE memoryNode = PhGetSelectedMemoryNode(&memoryContext->ListContext);

                    if (memoryNode && !memoryNode->IsAllocationBase)
                    {
                        if (memoryNode->MemoryItem->State & MEM_COMMIT)
                        {
                            PPH_SHOWMEMORYEDITOR showMemoryEditor = PhAllocate(sizeof(PH_SHOWMEMORYEDITOR));

                            memset(showMemoryEditor, 0, sizeof(PH_SHOWMEMORYEDITOR));
                            showMemoryEditor->ProcessId = processItem->ProcessId;
                            showMemoryEditor->BaseAddress = memoryNode->MemoryItem->BaseAddress;
                            showMemoryEditor->RegionSize = memoryNode->MemoryItem->RegionSize;
                            showMemoryEditor->SelectOffset = -1;
                            showMemoryEditor->SelectLength = 0;
                            ProcessHacker_ShowMemoryEditor(PhMainWndHandle, showMemoryEditor);
                        }
                        else
                        {
                            PhShowError(hwndDlg, L"Unable to edit the memory region because it is not committed.");
                        }
                    }
                }
                break;
            case ID_MEMORY_SAVE:
                {
                    NTSTATUS status;
                    HANDLE processHandle;
                    PPH_MEMORY_NODE *memoryNodes;
                    ULONG numberOfMemoryNodes;

                    if (!NT_SUCCESS(status = PhOpenProcess(
                        &processHandle,
                        PROCESS_VM_READ,
                        processItem->ProcessId
                        )))
                    {
                        PhShowStatus(hwndDlg, L"Unable to open the process", status, 0);
                        break;
                    }

                    PhGetSelectedMemoryNodes(&memoryContext->ListContext, &memoryNodes, &numberOfMemoryNodes);

                    if (numberOfMemoryNodes != 0)
                    {
                        static PH_FILETYPE_FILTER filters[] =
                        {
                            { L"Binary files (*.bin)", L"*.bin" },
                            { L"All files (*.*)", L"*.*" }
                        };
                        PVOID fileDialog;

                        fileDialog = PhCreateSaveFileDialog();

                        PhSetFileDialogFilter(fileDialog, filters, sizeof(filters) / sizeof(PH_FILETYPE_FILTER));
                        PhSetFileDialogFileName(fileDialog, PhaConcatStrings2(processItem->ProcessName->Buffer, L".bin")->Buffer);

                        if (PhShowFileDialog(hwndDlg, fileDialog))
                        {
                            PPH_STRING fileName;
                            PPH_FILE_STREAM fileStream;
                            PVOID buffer;
                            ULONG i;
                            ULONG_PTR offset;

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
                                buffer = PhAllocatePage(PAGE_SIZE, NULL);

                                // Go through each selected memory item and append the region contents
                                // to the file.
                                for (i = 0; i < numberOfMemoryNodes; i++)
                                {
                                    PPH_MEMORY_NODE memoryNode = memoryNodes[i];
                                    PPH_MEMORY_ITEM memoryItem = memoryNode->MemoryItem;

                                    if (!memoryNode->IsAllocationBase && !(memoryItem->State & MEM_COMMIT))
                                        continue;

                                    for (offset = 0; offset < memoryItem->RegionSize; offset += PAGE_SIZE)
                                    {
                                        if (NT_SUCCESS(NtReadVirtualMemory(
                                            processHandle,
                                            PTR_ADD_OFFSET(memoryItem->BaseAddress, offset),
                                            buffer,
                                            PAGE_SIZE,
                                            NULL
                                            )))
                                        {
                                            PhWriteFileStream(fileStream, buffer, PAGE_SIZE);
                                        }
                                    }
                                }

                                PhFreePage(buffer);

                                PhDereferenceObject(fileStream);
                            }

                            if (!NT_SUCCESS(status))
                                PhShowStatus(hwndDlg, L"Unable to create the file", status, 0);
                        }

                        PhFreeFileDialog(fileDialog);
                    }

                    PhFree(memoryNodes);
                    NtClose(processHandle);
                }
                break;
            case ID_MEMORY_CHANGEPROTECTION:
                {
                    PPH_MEMORY_NODE memoryNode = PhGetSelectedMemoryNode(&memoryContext->ListContext);

                    if (memoryNode)
                    {
                        PhReferenceObject(memoryNode->MemoryItem);

                        PhShowMemoryProtectDialog(hwndDlg, processItem, memoryNode->MemoryItem);
                        PhUpdateMemoryNode(&memoryContext->ListContext, memoryNode);

                        PhDereferenceObject(memoryNode->MemoryItem);
                    }
                }
                break;
            case ID_MEMORY_FREE:
                {
                    PPH_MEMORY_NODE memoryNode = PhGetSelectedMemoryNode(&memoryContext->ListContext);

                    if (memoryNode)
                    {
                        PhReferenceObject(memoryNode->MemoryItem);
                        PhUiFreeMemory(hwndDlg, processItem->ProcessId, memoryNode->MemoryItem, TRUE);
                        PhDereferenceObject(memoryNode->MemoryItem);
                        // TODO: somehow update the list
                    }
                }
                break;
            case ID_MEMORY_DECOMMIT:
                {
                    PPH_MEMORY_NODE memoryNode = PhGetSelectedMemoryNode(&memoryContext->ListContext);

                    if (memoryNode)
                    {
                        PhReferenceObject(memoryNode->MemoryItem);
                        PhUiFreeMemory(hwndDlg, processItem->ProcessId, memoryNode->MemoryItem, FALSE);
                        PhDereferenceObject(memoryNode->MemoryItem);
                    }
                }
                break;
            case ID_MEMORY_READWRITEADDRESS:
                {
                    PPH_STRING selectedChoice = NULL;

                    if (!memoryContext->MemoryItemListValid)
                        break;

                    while (PhaChoiceDialog(
                        hwndDlg,
                        L"Read/Write Address",
                        L"Enter an address:",
                        NULL,
                        0,
                        NULL,
                        PH_CHOICE_DIALOG_USER_CHOICE,
                        &selectedChoice,
                        NULL,
                        L"MemoryReadWriteAddressChoices"
                        ))
                    {
                        ULONG64 address64;
                        PVOID address;

                        if (selectedChoice->Length == 0)
                            continue;

                        if (PhStringToInteger64(&selectedChoice->sr, 0, &address64))
                        {
                            PPH_MEMORY_ITEM memoryItem;

                            address = (PVOID)address64;
                            memoryItem = PhLookupMemoryItemList(&memoryContext->MemoryItemList, address);

                            if (memoryItem)
                            {
                                PPH_SHOWMEMORYEDITOR showMemoryEditor = PhAllocate(sizeof(PH_SHOWMEMORYEDITOR));

                                memset(showMemoryEditor, 0, sizeof(PH_SHOWMEMORYEDITOR));
                                showMemoryEditor->ProcessId = processItem->ProcessId;
                                showMemoryEditor->BaseAddress = memoryItem->BaseAddress;
                                showMemoryEditor->RegionSize = memoryItem->RegionSize;
                                showMemoryEditor->SelectOffset = (ULONG)((ULONG_PTR)address - (ULONG_PTR)memoryItem->BaseAddress);
                                showMemoryEditor->SelectLength = 0;
                                ProcessHacker_ShowMemoryEditor(PhMainWndHandle, showMemoryEditor);
                                break;
                            }
                            else
                            {
                                PhShowError(hwndDlg, L"Unable to find the memory region for the selected address.");
                            }
                        }
                    }
                }
                break;
            case ID_MEMORY_COPY:
                {
                    PPH_STRING text;

                    text = PhGetTreeNewText(tnHandle, 0);
                    PhSetClipboardString(tnHandle, &text->sr);
                    PhDereferenceObject(text);
                }
                break;
            case IDC_HIDEFREEREGIONS:
                {
                    BOOLEAN hide;

                    hide = Button_GetCheck(GetDlgItem(hwndDlg, IDC_HIDEFREEREGIONS)) == BST_CHECKED;
                    PhSetOptionsMemoryList(&memoryContext->ListContext, hide);
                }
                break;
            case IDC_STRINGS:
                PhShowMemoryStringDialog(hwndDlg, processItem);
                break;
            case IDC_REFRESH:
                PhpRefreshProcessMemoryList(hwndDlg, propPageContext);
                break;
            }
        }
        break;
    }

    return FALSE;
}
