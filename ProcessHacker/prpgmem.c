/*
 * Process Hacker -
 *   Process properties: Memory page
 *
 * Copyright (C) 2009-2016 wj32
 * Copyright (C) 2017-2019 dmex
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
#include <procprp.h>
#include <procprpp.h>

#include <cpysave.h>
#include <emenu.h>

#include <actions.h>
#include <extmgri.h>
#include <mainwnd.h>
#include <phplug.h>
#include <phsettings.h>
#include <procprv.h>

static PPH_OBJECT_TYPE PhMemoryContextType = NULL;
static PH_STRINGREF EmptyMemoryText = PH_STRINGREF_INIT(L"There are no memory regions to display.");

NTSTATUS PhpRefreshProcessMemoryThread(
    _In_ PVOID Context
    )
{
    PPH_MEMORY_CONTEXT memoryContext = Context;

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
    }

    PostMessage(memoryContext->WindowHandle, WM_PH_INVOKE, 0, 0);

    PhDereferenceObject(memoryContext);

    return STATUS_SUCCESS;
}

VOID PhpRefreshProcessMemoryList(
    _In_ PPH_PROCESS_PROPPAGECONTEXT PropPageContext
    )
{
    PPH_MEMORY_CONTEXT memoryContext = PropPageContext->Context;

    if (memoryContext->MemoryItemListValid)
    {
        PhDeleteMemoryItemList(&memoryContext->MemoryItemList);
        memoryContext->MemoryItemListValid = FALSE;
    }

    PhReferenceObject(memoryContext);
    PhCreateThread2(PhpRefreshProcessMemoryThread, memoryContext);
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
        PhInsertEMenuItem(menu, PhCreateEMenuItem(0, ID_MEMORY_READWRITEMEMORY, L"&Read/Write memory", NULL, NULL), ULONG_MAX);
        PhInsertEMenuItem(menu, PhCreateEMenuItem(0, ID_MEMORY_SAVE, L"&Save...", NULL, NULL), ULONG_MAX);
        PhInsertEMenuItem(menu, PhCreateEMenuItem(0, ID_MEMORY_CHANGEPROTECTION, L"Change &protection...", NULL, NULL), ULONG_MAX);
        PhInsertEMenuItem(menu, PhCreateEMenuItem(0, ID_MEMORY_FREE, L"&Free", NULL, NULL), ULONG_MAX);
        PhInsertEMenuItem(menu, PhCreateEMenuItem(0, ID_MEMORY_DECOMMIT, L"&Decommit", NULL, NULL), ULONG_MAX);
        PhInsertEMenuItem(menu, PhCreateEMenuSeparator(), ULONG_MAX);
        PhInsertEMenuItem(menu, PhCreateEMenuItem(0, ID_MEMORY_COPY, L"&Copy\bCtrl+C", NULL, NULL), ULONG_MAX);
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

static BOOLEAN PhpWordMatchHandleStringRef(
    _In_ PPH_STRING SearchText,
    _In_ PPH_STRINGREF Text
    )
{
    PH_STRINGREF part;
    PH_STRINGREF remainingPart;

    remainingPart = SearchText->sr;

    while (remainingPart.Length)
    {
        PhSplitStringRefAtChar(&remainingPart, L'|', &part, &remainingPart);

        if (part.Length)
        {
            if (PhFindStringInStringRef(Text, &part, TRUE) != -1)
                return TRUE;
        }
    }

    return FALSE;
}

static BOOLEAN PhpWordMatchHandleStringZ(
    _In_ PPH_STRING SearchText,
    _In_ PWSTR Text
    )
{
    PH_STRINGREF text;

    PhInitializeStringRef(&text, Text);

    return PhpWordMatchHandleStringRef(SearchText, &text);
}

BOOLEAN PhpMemoryTreeFilterCallback(
    _In_ PPH_TREENEW_NODE Node,
    _In_opt_ PVOID Context
    )
{
    PPH_MEMORY_CONTEXT memoryContext = Context;
    PPH_MEMORY_NODE memoryNode = (PPH_MEMORY_NODE)Node;
    PPH_MEMORY_ITEM memoryItem = memoryNode->MemoryItem;
    PPH_STRING useText;
    PWSTR tempString;

    if (memoryContext->ListContext.HideFreeRegions && memoryItem->State & MEM_FREE)
        return FALSE;
    if (memoryContext->ListContext.HideGuardRegions && memoryItem->Protect & PAGE_GUARD)
        return FALSE;

    if (memoryContext->ListContext.HideReservedRegions &&
        (memoryItem->Type & MEM_PRIVATE || memoryItem->Type & MEM_MAPPED || memoryItem->Type & MEM_IMAGE) &&
        memoryItem->State & MEM_RESERVE &&
        memoryItem->AllocationBaseItem // Ignore root nodes
        )
    {
        return FALSE;
    }

    if (PhIsNullOrEmptyString(memoryContext->SearchboxText))
        return TRUE;

    if (memoryContext->UseSearchPointer)
    {
        // Show all the nodes for which the specified pointer is the allocation base
        if ((ULONG64)memoryNode->MemoryItem->AllocationBase == memoryContext->SearchPointer)
        {
            return TRUE;
        }

        // Show the AllocationBaseNode for which the specified pointer is within its range
        if (
            memoryNode->IsAllocationBase &&
            (ULONG64)memoryNode->MemoryItem->AllocationBase <= memoryContext->SearchPointer &&
            (ULONG64)memoryNode->MemoryItem->AllocationBase + (ULONG64)memoryNode->MemoryItem->RegionSize > memoryContext->SearchPointer
            )
        {
            return TRUE;
        }

        // Show the RegionNode for which the specified pointer is within its range
        if (
            (ULONG64)memoryNode->MemoryItem->BaseAddress <= memoryContext->SearchPointer &&
            (ULONG64)memoryNode->MemoryItem->BaseAddress + (ULONG64)memoryNode->MemoryItem->RegionSize > memoryContext->SearchPointer
            )
        {
            return TRUE;
        }
    }

    if (memoryNode->BaseAddressText[0])
    {
        if (PhpWordMatchHandleStringZ(memoryContext->SearchboxText, memoryNode->BaseAddressText))
            return TRUE;
    }

    useText = PH_AUTO(PhGetMemoryRegionUseText(memoryItem));
    if (!PhIsNullOrEmptyString(useText))
    {
        if (PhpWordMatchHandleStringRef(memoryContext->SearchboxText, &useText->sr))
            return TRUE;
    }
    
    tempString = PhGetMemoryTypeString(memoryItem->Type);
    if (tempString[0])
    {
        if (PhpWordMatchHandleStringZ(memoryContext->SearchboxText, tempString))
            return TRUE;
    }

    tempString = PhGetMemoryStateString(memoryItem->State);
    if (tempString[0])
    {
        if (PhpWordMatchHandleStringZ(memoryContext->SearchboxText, tempString))
            return TRUE;
    }

    return FALSE;
}

VOID PhpMemoryContextDeleteProcedure(
    _In_ PVOID Object,
    _In_ ULONG Flags
    )
{
    PPH_MEMORY_CONTEXT memoryContext = (PPH_MEMORY_CONTEXT)Object;

    PhDeleteMemoryList(&memoryContext->ListContext);

    if (memoryContext->MemoryItemListValid)
        PhDeleteMemoryItemList(&memoryContext->MemoryItemList);

    PhClearReference(&memoryContext->ErrorMessage);
}

PPH_MEMORY_CONTEXT PhCreateMemoryContext(
    VOID
    )
{
    static PH_INITONCE initOnce = PH_INITONCE_INIT;
    PPH_MEMORY_CONTEXT memoryContext;

    if (PhBeginInitOnce(&initOnce))
    {
        PhMemoryContextType = PhCreateObjectType(L"ProcessMemoryContext", 0, PhpMemoryContextDeleteProcedure);
        PhEndInitOnce(&initOnce);
    }

    memoryContext = PhCreateObject(
        PhEmGetObjectSize(EmMemoryContextType, sizeof(PH_MEMORY_CONTEXT)),
        PhMemoryContextType
        );
    memset(memoryContext, 0, sizeof(PH_MEMORY_CONTEXT));

    return memoryContext;
}

INT_PTR CALLBACK PhpProcessMemoryDlgProc(
    _In_ HWND hwndDlg,
    _In_ UINT uMsg,
    _In_ WPARAM wParam,
    _In_ LPARAM lParam
    )
{
    PPH_MEMORY_CONTEXT memoryContext;
    LPPROPSHEETPAGE propSheetPage;
    PPH_PROCESS_PROPPAGECONTEXT propPageContext;
    PPH_PROCESS_ITEM processItem;

    if (PhPropPageDlgProcHeader(hwndDlg, uMsg, lParam, &propSheetPage, &propPageContext, &processItem))
    {
        memoryContext = (PPH_MEMORY_CONTEXT)propPageContext->Context;
    }
    else
    {
        return FALSE;
    }

    switch (uMsg)
    {
    case WM_INITDIALOG:
        {
            memoryContext = propPageContext->Context = PhCreateMemoryContext();
            memoryContext->WindowHandle = hwndDlg;
            memoryContext->ProcessId = processItem->ProcessId;
            memoryContext->TreeNewHandle = GetDlgItem(hwndDlg, IDC_LIST);
            memoryContext->SearchboxHandle = GetDlgItem(hwndDlg, IDC_SEARCH);
            memoryContext->LastRunStatus = ULONG_MAX;
            memoryContext->ErrorMessage = NULL;
            memoryContext->SearchboxText = PhReferenceEmptyString();

            // Initialize the list.
            PhInitializeMemoryList(hwndDlg, memoryContext->TreeNewHandle, &memoryContext->ListContext);
            TreeNew_SetEmptyText(memoryContext->TreeNewHandle, &PhpLoadingText, 0);

            memoryContext->AllocationFilterEntry = PhAddTreeNewFilter(&memoryContext->ListContext.AllocationTreeFilterSupport, PhpMemoryTreeFilterCallback, memoryContext);
            memoryContext->FilterEntry = PhAddTreeNewFilter(&memoryContext->ListContext.TreeFilterSupport, PhpMemoryTreeFilterCallback, memoryContext);

            PhCreateSearchControl(hwndDlg, memoryContext->SearchboxHandle, L"Search Memory (Ctrl+K)");

            PhEmCallObjectOperation(EmMemoryContextType, memoryContext, EmObjectCreate);

            if (PhPluginsEnabled)
            {
                PH_PLUGIN_TREENEW_INFORMATION treeNewInfo;

                treeNewInfo.TreeNewHandle = memoryContext->TreeNewHandle;
                treeNewInfo.CmData = &memoryContext->ListContext.Cm;
                treeNewInfo.SystemContext = memoryContext;
                PhInvokeCallback(PhGetGeneralCallback(GeneralCallbackMemoryTreeNewInitializing), &treeNewInfo);
            }

            PhLoadSettingsMemoryList(&memoryContext->ListContext);

            PhpRefreshProcessMemoryList(propPageContext);

            PhInitializeWindowTheme(hwndDlg, PhEnableThemeSupport);
        }
        break;
    case WM_DESTROY:
        {
            PhRemoveTreeNewFilter(&memoryContext->ListContext.TreeFilterSupport, memoryContext->FilterEntry);
            PhRemoveTreeNewFilter(&memoryContext->ListContext.TreeFilterSupport, memoryContext->AllocationFilterEntry);
            if (memoryContext->SearchboxText) PhDereferenceObject(memoryContext->SearchboxText);

            PhEmCallObjectOperation(EmMemoryContextType, memoryContext, EmObjectDelete);

            if (PhPluginsEnabled)
            {
                PH_PLUGIN_TREENEW_INFORMATION treeNewInfo;

                treeNewInfo.TreeNewHandle = memoryContext->TreeNewHandle;
                treeNewInfo.CmData = &memoryContext->ListContext.Cm;
                PhInvokeCallback(PhGetGeneralCallback(GeneralCallbackMemoryTreeNewUninitializing), &treeNewInfo);
            }

            PhSaveSettingsMemoryList(&memoryContext->ListContext);

            PhDereferenceObject(memoryContext);
        }
        break;
    case WM_SHOWWINDOW:
        {
            PPH_LAYOUT_ITEM dialogItem;

            if (dialogItem = PhBeginPropPageLayout(hwndDlg, propPageContext))
            {
                PhAddPropPageLayoutItem(hwndDlg, memoryContext->SearchboxHandle, dialogItem, PH_ANCHOR_TOP | PH_ANCHOR_RIGHT);
                PhAddPropPageLayoutItem(hwndDlg, memoryContext->ListContext.TreeNewHandle, dialogItem, PH_ANCHOR_ALL);
                PhEndPropPageLayout(hwndDlg, propPageContext);
            }
        }
        break;
    case WM_COMMAND:
        {
            switch (GET_WM_COMMAND_CMD(wParam, lParam))
            {
            case EN_CHANGE:
                {
                    PPH_STRING newSearchboxText;

                    if (GET_WM_COMMAND_HWND(wParam, lParam) != memoryContext->SearchboxHandle)
                        break;

                    newSearchboxText = PH_AUTO(PhGetWindowText(memoryContext->SearchboxHandle));

                    if (!PhEqualString(memoryContext->SearchboxText, newSearchboxText, FALSE))
                    {
                        // Try to get a search pointer from the search string.
                        memoryContext->UseSearchPointer = PhStringToInteger64(&newSearchboxText->sr, 0, &memoryContext->SearchPointer);

                        // Cache the current search text for our callback.
                        PhSwapReference(&memoryContext->SearchboxText, newSearchboxText);

                        // Expand any hidden nodes to make search results visible.
                        PhExpandAllMemoryNodes(&memoryContext->ListContext, TRUE);

                        PhApplyTreeNewFilters(&memoryContext->ListContext.AllocationTreeFilterSupport);
                        PhApplyTreeNewFilters(&memoryContext->ListContext.TreeFilterSupport);
                    }
                }
                break;
            }

            switch (GET_WM_COMMAND_ID(wParam, lParam))
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
                        PPH_SHOW_MEMORY_EDITOR showMemoryEditor;

                        showMemoryEditor = PhAllocateZero(sizeof(PH_SHOW_MEMORY_EDITOR));
                        showMemoryEditor->OwnerWindow = hwndDlg;
                        showMemoryEditor->ProcessId = processItem->ProcessId;
                        showMemoryEditor->BaseAddress = memoryNode->MemoryItem->BaseAddress;
                        showMemoryEditor->RegionSize = memoryNode->MemoryItem->RegionSize;
                        showMemoryEditor->SelectOffset = ULONG_MAX;
                        showMemoryEditor->SelectLength = 0;

                        ProcessHacker_ShowMemoryEditor(showMemoryEditor);
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
                                if (buffer = PhAllocatePage(PAGE_SIZE, NULL))
                                {
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
                                }

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
            case ID_MEMORY_COPY:
                {
                    PPH_STRING text;

                    text = PhGetTreeNewText(memoryContext->TreeNewHandle, 0);
                    PhSetClipboardString(memoryContext->TreeNewHandle, &text->sr);
                    PhDereferenceObject(text);
                }
                break;
            case IDC_REFRESH:
                PhpRefreshProcessMemoryList(propPageContext);
                break;
            case IDC_FILTEROPTIONS:
                {
                    RECT rect;
                    PPH_EMENU menu;
                    PPH_EMENU_ITEM freeItem;
                    PPH_EMENU_ITEM reservedItem;
                    PPH_EMENU_ITEM guardItem;
                    PPH_EMENU_ITEM privateItem;
                    PPH_EMENU_ITEM systemItem;
                    PPH_EMENU_ITEM cfgItem;
                    PPH_EMENU_ITEM typeItem;
                    PPH_EMENU_ITEM selectedItem;

                    GetWindowRect(GetDlgItem(hwndDlg, IDC_FILTEROPTIONS), &rect);

                    typedef enum _PH_MEMORY_FILTER_MENU_ITEM
                    {
                        PH_MEMORY_FILTER_MENU_HIDE_FREE = 1,
                        PH_MEMORY_FILTER_MENU_HIDE_RESERVED,
                        PH_MEMORY_FILTER_MENU_HIGHLIGHT_PRIVATE,
                        PH_MEMORY_FILTER_MENU_HIGHLIGHT_SYSTEM,
                        PH_MEMORY_FILTER_MENU_HIGHLIGHT_CFG,
                        PH_MEMORY_FILTER_MENU_HIGHLIGHT_EXECUTE,
                        PH_MEMORY_FILTER_MENU_HIDE_GUARD,
                        // Non-standard PH_MEMORY_FLAG options.
                        PH_MEMORY_FILTER_MENU_READ_ADDRESS,
                        PH_MEMORY_FILTER_MENU_HEAPS,
                        PH_MEMORY_FILTER_MENU_STRINGS,
                    } PH_MEMORY_FILTER_MENU_ITEM;

                    menu = PhCreateEMenu();
                    PhInsertEMenuItem(menu, freeItem = PhCreateEMenuItem(0, PH_MEMORY_FILTER_MENU_HIDE_FREE, L"Hide free pages", NULL, NULL), ULONG_MAX);
                    PhInsertEMenuItem(menu, reservedItem = PhCreateEMenuItem(0, PH_MEMORY_FILTER_MENU_HIDE_RESERVED, L"Hide reserved pages", NULL, NULL), ULONG_MAX);
                    PhInsertEMenuItem(menu, guardItem = PhCreateEMenuItem(0, PH_MEMORY_FILTER_MENU_HIDE_GUARD, L"Hide guard pages", NULL, NULL), ULONG_MAX);
                    PhInsertEMenuItem(menu, PhCreateEMenuSeparator(), ULONG_MAX);
                    PhInsertEMenuItem(menu, privateItem = PhCreateEMenuItem(0, PH_MEMORY_FILTER_MENU_HIGHLIGHT_PRIVATE, L"Highlight private pages", NULL, NULL), ULONG_MAX);
                    PhInsertEMenuItem(menu, systemItem = PhCreateEMenuItem(0, PH_MEMORY_FILTER_MENU_HIGHLIGHT_SYSTEM, L"Highlight system pages", NULL, NULL), ULONG_MAX);
                    PhInsertEMenuItem(menu, cfgItem = PhCreateEMenuItem(0, PH_MEMORY_FILTER_MENU_HIGHLIGHT_CFG, L"Highlight CFG pages", NULL, NULL), ULONG_MAX);
                    PhInsertEMenuItem(menu, typeItem = PhCreateEMenuItem(0, PH_MEMORY_FILTER_MENU_HIGHLIGHT_EXECUTE, L"Highlight executable pages", NULL, NULL), ULONG_MAX);
                    PhInsertEMenuItem(menu, PhCreateEMenuSeparator(), ULONG_MAX);
                    PhInsertEMenuItem(menu, PhCreateEMenuItem(0, PH_MEMORY_FILTER_MENU_READ_ADDRESS, L"Read/Write &address...", NULL, NULL), ULONG_MAX);
                    PhInsertEMenuItem(menu, PhCreateEMenuItem(0, PH_MEMORY_FILTER_MENU_HEAPS, L"Heaps...", NULL, NULL), ULONG_MAX);
                    PhInsertEMenuItem(menu, PhCreateEMenuItem(0, PH_MEMORY_FILTER_MENU_STRINGS, L"Strings...", NULL, NULL), ULONG_MAX);

                    if (memoryContext->ListContext.HideFreeRegions)
                        freeItem->Flags |= PH_EMENU_CHECKED;
                    if (memoryContext->ListContext.HideReservedRegions)
                        reservedItem->Flags |= PH_EMENU_CHECKED;
                    if (memoryContext->ListContext.HideGuardRegions)
                        guardItem->Flags |= PH_EMENU_CHECKED;
                    if (memoryContext->ListContext.HighlightPrivatePages)
                        privateItem->Flags |= PH_EMENU_CHECKED;
                    if (memoryContext->ListContext.HighlightSystemPages)
                        systemItem->Flags |= PH_EMENU_CHECKED;
                    if (memoryContext->ListContext.HighlightCfgPages)
                        cfgItem->Flags |= PH_EMENU_CHECKED;
                    if (memoryContext->ListContext.HighlightExecutePages)
                        typeItem->Flags |= PH_EMENU_CHECKED;

                    selectedItem = PhShowEMenu(
                        menu,
                        hwndDlg,
                        PH_EMENU_SHOW_LEFTRIGHT,
                        PH_ALIGN_LEFT | PH_ALIGN_TOP,
                        rect.left,
                        rect.bottom
                        );

                    if (selectedItem && selectedItem->Id)
                    {
                        if (selectedItem->Id == PH_MEMORY_FILTER_MENU_HIDE_FREE || 
                            selectedItem->Id == PH_MEMORY_FILTER_MENU_HIDE_RESERVED ||
                            selectedItem->Id == PH_MEMORY_FILTER_MENU_HIDE_GUARD ||
                            selectedItem->Id == PH_MEMORY_FILTER_MENU_HIGHLIGHT_PRIVATE ||
                            selectedItem->Id == PH_MEMORY_FILTER_MENU_HIGHLIGHT_SYSTEM ||
                            selectedItem->Id == PH_MEMORY_FILTER_MENU_HIGHLIGHT_CFG ||
                            selectedItem->Id == PH_MEMORY_FILTER_MENU_HIGHLIGHT_EXECUTE)
                        {
                            PhSetOptionsMemoryList(&memoryContext->ListContext, selectedItem->Id);
                            PhSaveSettingsMemoryList(&memoryContext->ListContext);

                            PhApplyTreeNewFilters(&memoryContext->ListContext.AllocationTreeFilterSupport);
                            PhApplyTreeNewFilters(&memoryContext->ListContext.TreeFilterSupport);
                        }
                        else if (selectedItem->Id == PH_MEMORY_FILTER_MENU_STRINGS)
                        {
                            PhShowMemoryStringDialog(hwndDlg, processItem);
                        }
                        else if (selectedItem->Id == PH_MEMORY_FILTER_MENU_HEAPS)
                        {
                            PhShowProcessHeapsDialog(hwndDlg, processItem);
                        }
                        else if (selectedItem->Id == PH_MEMORY_FILTER_MENU_READ_ADDRESS)
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
                                        PPH_SHOW_MEMORY_EDITOR showMemoryEditor;

                                        showMemoryEditor = PhAllocateZero(sizeof(PH_SHOW_MEMORY_EDITOR));
                                        showMemoryEditor->ProcessId = processItem->ProcessId;
                                        showMemoryEditor->BaseAddress = memoryItem->BaseAddress;
                                        showMemoryEditor->RegionSize = memoryItem->RegionSize;
                                        showMemoryEditor->SelectOffset = (ULONG)((ULONG_PTR)address - (ULONG_PTR)memoryItem->BaseAddress);
                                        showMemoryEditor->SelectLength = 0;

                                        ProcessHacker_ShowMemoryEditor(showMemoryEditor);
                                        break;
                                    }
                                    else
                                    {
                                        PhShowError(hwndDlg, L"%s", L"Unable to find the memory region for the selected address.");
                                    }
                                }
                            }
                        }
                    }

                    PhDestroyEMenu(menu);
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
            case PSN_QUERYINITIALFOCUS:
                SetWindowLongPtr(hwndDlg, DWLP_MSGRESULT, (LPARAM)GetDlgItem(hwndDlg, IDC_LIST));
                return TRUE;
            }
        }
        break;
    case WM_KEYDOWN:
        {
            if (LOWORD(wParam) == 'K')
            {
                if (GetKeyState(VK_CONTROL) < 0)
                {
                    SetFocus(memoryContext->SearchboxHandle);
                    return TRUE;
                }
            }
        }
        break;
    case WM_PH_INVOKE:
        {
            if (memoryContext->MemoryItemListValid)
            {
                TreeNew_SetEmptyText(memoryContext->ListContext.TreeNewHandle, &EmptyMemoryText, 0);

                PhReplaceMemoryList(&memoryContext->ListContext, &memoryContext->MemoryItemList);
            }
            else
            {
                PPH_STRING message;

                message = PhGetStatusMessage(memoryContext->LastRunStatus, 0);
                PhMoveReference(&memoryContext->ErrorMessage, PhFormatString(L"Unable to query memory information:\n%s", PhGetStringOrDefault(message, L"Unknown error.")));
                TreeNew_SetEmptyText(memoryContext->ListContext.TreeNewHandle, &memoryContext->ErrorMessage->sr, 0);

                PhReplaceMemoryList(&memoryContext->ListContext, NULL);

                PhClearReference(&message);
            }
        }
        break;
    }

    return FALSE;
}
