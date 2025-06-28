/*
 * Copyright (c) 2022 Winsider Seminars & Solutions, Inc.  All rights reserved.
 *
 * This file is part of System Informer.
 *
 * Authors:
 *
 *     wj32    2009-2016
 *     dmex    2017-2023
 *
 */

#include <phapp.h>
#include <procprp.h>
#include <procprpp.h>
#include <settings.h>

#include <cpysave.h>
#include <emenu.h>

#include <actions.h>
#include <extmgri.h>
#include <mainwnd.h>
#include <phplug.h>
#include <procprv.h>

static PPH_OBJECT_TYPE PhMemoryContextType = NULL;
static PH_STRINGREF EmptyMemoryText = PH_STRINGREF_INIT(L"There are no memory regions to display.");

_Function_class_(USER_THREAD_START_ROUTINE)
NTSTATUS PhpRefreshProcessMemoryThread(
    _In_ PVOID Context
    )
{
    PPH_MEMORY_CONTEXT memoryContext = Context;
    ULONG flags = PH_QUERY_MEMORY_REGION_TYPE | PH_QUERY_MEMORY_WS_COUNTERS;
    if (memoryContext->ListContext.ZeroPadAddresses)
        flags |= PH_QUERY_MEMORY_ZERO_PAD_ADDRESSES;

    if (PH_IS_REAL_PROCESS_ID(memoryContext->ProcessId))
    {
        memoryContext->LastRunStatus = PhQueryMemoryItemList(
            memoryContext->ProcessId,
            flags,
            &memoryContext->MemoryItemList
            );
    }

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
        PhInsertEMenuItem(menu, PhCreateEMenuItem(0, ID_MEMORY_READWRITEMEMORY, L"&Read/Write memory...", NULL, NULL), ULONG_MAX);
        PhInsertEMenuItem(menu, PhCreateEMenuItem(0, ID_MEMORY_CHANGEPROTECTION, L"Change &protection...", NULL, NULL), ULONG_MAX);
        PhInsertEMenuItem(menu, PhCreateEMenuItem(0, ID_MEMORY_EMPTYWORKINGSET, L"&Empty working set...", NULL, NULL), ULONG_MAX);
        PhInsertEMenuItem(menu, PhCreateEMenuSeparator(), ULONG_MAX);
        PhInsertEMenuItem(menu, PhCreateEMenuItem(0, ID_MEMORY_FREE, L"&Free", NULL, NULL), ULONG_MAX);
        PhInsertEMenuItem(menu, PhCreateEMenuItem(0, ID_MEMORY_DECOMMIT, L"&Decommit", NULL, NULL), ULONG_MAX);
        PhInsertEMenuItem(menu, PhCreateEMenuSeparator(), ULONG_MAX);
        PhInsertEMenuItem(menu, PhCreateEMenuItem(0, ID_MEMORY_SAVE, L"&Save...", NULL, NULL), ULONG_MAX);
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

BOOLEAN PhpMemoryTreeFilterCallback(
    _In_ PPH_TREENEW_NODE Node,
    _In_opt_ PVOID Context
    )
{
    PPH_MEMORY_CONTEXT memoryContext = Context;
    PPH_MEMORY_NODE memoryNode = (PPH_MEMORY_NODE)Node;
    PPH_MEMORY_ITEM memoryItem = memoryNode->MemoryItem;
    PCPH_STRINGREF string;
    PPH_STRING useText;

    if (memoryContext->ListContext.HideFreeRegions && FlagOn(memoryItem->State, MEM_FREE))
        return FALSE;
    if (memoryContext->ListContext.HideGuardRegions && FlagOn(memoryItem->Protect, PAGE_GUARD))
        return FALSE;

    if (
        memoryContext->ListContext.HideReservedRegions &&
        (FlagOn(memoryItem->Type, MEM_PRIVATE) || FlagOn(memoryItem->Type, MEM_MAPPED) || FlagOn(memoryItem->Type, MEM_IMAGE)) &&
        FlagOn(memoryItem->State, MEM_RESERVE) &&
        memoryItem->AllocationBaseItem // Ignore root nodes
        )
    {
        return FALSE;
    }

    if (!memoryContext->SearchMatchHandle)
        return TRUE;

    if (PhSearchControlMatchPointer(memoryContext->SearchMatchHandle, memoryNode->MemoryItem->AllocationBase))
        return TRUE;

    if (PhSearchControlMatchPointerRange(
        memoryContext->SearchMatchHandle,
        memoryNode->MemoryItem->AllocationBase,
        memoryNode->MemoryItem->RegionSize
        ))
        return TRUE;

    if (PhSearchControlMatchPointerRange(
        memoryContext->SearchMatchHandle,
        memoryNode->MemoryItem->BaseAddress,
        memoryNode->MemoryItem->RegionSize
        ))
        return TRUE;

    if (memoryItem->BaseAddressString[0])
    {
        if (PhSearchControlMatchLongHintZ(memoryContext->SearchMatchHandle, memoryItem->BaseAddressString))
            return TRUE;
    }

    string = PhGetMemoryTypeString(memoryItem->Type);
    if (string && string->Length)
    {
        if (PhSearchControlMatch(memoryContext->SearchMatchHandle, string))
            return TRUE;
    }

    string = PhGetMemoryStateString(memoryItem->State);
    if (string && string->Length)
    {
        if (PhSearchControlMatch(memoryContext->SearchMatchHandle, string))
            return TRUE;
    }

    useText = PH_AUTO(PhGetMemoryRegionUseText(memoryItem));
    if (!PhIsNullOrEmptyString(useText))
    {
        if (PhSearchControlMatch(memoryContext->SearchMatchHandle, &useText->sr))
            return TRUE;
    }

    return FALSE;
}

VOID PhpPopulateTableWithProcessMemoryNodes(
    _In_ HWND TreeListHandle,
    _In_ PPH_MEMORY_NODE Node,
    _In_ ULONG Level,
    _In_ PPH_STRING **Table,
    _Inout_ PULONG Index,
    _In_ PULONG DisplayToId,
    _In_ ULONG Columns
    )
{
    for (ULONG i = 0; i < Columns; i++)
    {
        PH_TREENEW_GET_CELL_TEXT getCellText;
        PPH_STRING text;

        getCellText.Node = &Node->Node;
        getCellText.Id = DisplayToId[i];
        PhInitializeEmptyStringRef(&getCellText.Text);
        TreeNew_GetCellText(TreeListHandle, &getCellText);

        if (i != 0)
        {
            if (getCellText.Text.Length == 0)
                text = PhReferenceEmptyString();
            else
                text = PhaCreateStringEx(getCellText.Text.Buffer, getCellText.Text.Length);
        }
        else
        {
            // If this is the first column in the row, add some indentation.
            text = PhaCreateStringEx(
                NULL,
                getCellText.Text.Length + UInt32x32To64(Level, 2) * sizeof(WCHAR)
                );
            wmemset(text->Buffer, L' ', UInt32x32To64(Level, 2));
            memcpy(&text->Buffer[UInt32x32To64(Level, 2)], getCellText.Text.Buffer, getCellText.Text.Length);
        }

        Table[*Index][i] = text;
    }

    (*Index)++;

    // Process the children.
    if (!Node->Children)
        return;

    for (ULONG i = 0; i < Node->Children->Count; i++)
    {
        PhpPopulateTableWithProcessMemoryNodes(
            TreeListHandle,
            Node->Children->Items[i],
            Level + 1,
            Table,
            Index,
            DisplayToId,
            Columns
            );
    }
}

PPH_LIST PhpGetProcessMemoryTreeListLines(
    _In_ HWND TreeListHandle,
    _In_ ULONG NumberOfNodes,
    _In_ PPH_LIST RootNodes,
    _In_ ULONG Mode
    )
{
    PH_AUTO_POOL autoPool;
    PPH_LIST lines;
    // The number of rows in the table (including +1 for the column headers).
    ULONG rows;
    // The number of columns.
    ULONG columns;
    // A column display index to ID map.
    PULONG displayToId;
    // A column display index to text map.
    PWSTR *displayToText;
    // The actual string table.
    PPH_STRING **table;
    ULONG i;
    ULONG j;

    // Use a local auto-pool to make memory management a bit less painful.
    PhInitializeAutoPool(&autoPool);

    rows = NumberOfNodes + 1;

    // Create the display index to ID map.
    PhMapDisplayIndexTreeNew(TreeListHandle, &displayToId, &displayToText, &columns);

    PhaCreateTextTable(&table, rows, columns);

    // Populate the first row with the column headers.
    for (i = 0; i < columns; i++)
    {
        table[0][i] = PhaCreateString(displayToText[i]);
    }

    // Go through the nodes in the process tree and populate each cell of the table.

    j = 1; // index starts at one because the first row contains the column headers.

    for (i = 0; i < RootNodes->Count; i++)
    {
        PhpPopulateTableWithProcessMemoryNodes(
            TreeListHandle,
            RootNodes->Items[i],
            0,
            table,
            &j,
            displayToId,
            columns
            );
    }

    PhFree(displayToId);
    PhFree(displayToText);

    lines = PhaFormatTextTable(table, rows, columns, Mode);

    PhDeleteAutoPool(&autoPool);

    return lines;
}

VOID PhpProcessMemorySave(
    _In_ PPH_MEMORY_CONTEXT MemoryContext
    )
{
    static PH_FILETYPE_FILTER filters[] =
    {
        { L"Text files (*.txt;*.log)", L"*.txt;*.log" },
        { L"Comma-separated values (*.csv)", L"*.csv" },
        { L"All files (*.*)", L"*.*" }
    };
    PVOID fileDialog = PhCreateSaveFileDialog();
    PH_FORMAT format[4];
    PPH_PROCESS_ITEM processItem;

    processItem = PhReferenceProcessItem(MemoryContext->ProcessId);
    PhInitFormatS(&format[0], L"System Informer (");
    PhInitFormatS(&format[1], processItem ? PhGetStringOrDefault(processItem->ProcessName, L"Unknown process") : L"Unknown process");
    PhInitFormatS(&format[2], L") Memory");
    PhInitFormatS(&format[3], L".txt");
    if (processItem) PhDereferenceObject(processItem);

    PhSetFileDialogFilter(fileDialog, filters, sizeof(filters) / sizeof(PH_FILETYPE_FILTER));
    PhSetFileDialogFileName(fileDialog, PH_AUTO_T(PH_STRING, PhFormat(format, 3, 60))->Buffer);

    if (PhShowFileDialog(MemoryContext->WindowHandle, fileDialog))
    {
        NTSTATUS status;
        PPH_STRING fileName;
        ULONG filterIndex;
        PPH_FILE_STREAM fileStream;

        fileName = PH_AUTO(PhGetFileDialogFileName(fileDialog));
        filterIndex = PhGetFileDialogFilterIndex(fileDialog);

        if (NT_SUCCESS(status = PhCreateFileStream(
            &fileStream,
            fileName->Buffer,
            FILE_GENERIC_WRITE,
            FILE_SHARE_READ,
            FILE_OVERWRITE_IF,
            0
            )))
        {
            ULONG mode;
            PPH_LIST lines;

            if (filterIndex == 2)
                mode = PH_EXPORT_MODE_CSV;
            else
                mode = PH_EXPORT_MODE_TABS;

            PhWriteStringAsUtf8FileStream(fileStream, (PPH_STRINGREF)&PhUnicodeByteOrderMark);

            if (mode != PH_EXPORT_MODE_CSV)
            {
                PhWritePhTextHeader(fileStream);
            }

            lines = PhpGetProcessMemoryTreeListLines(
                MemoryContext->TreeNewHandle,
                MemoryContext->ListContext.RegionNodeList->Count,
                MemoryContext->ListContext.RegionNodeList,
                mode
                );

            for (ULONG i = 0; i < lines->Count; i++)
            {
                PPH_STRING line;

                line = lines->Items[i];
                PhWriteStringAsUtf8FileStream(fileStream, &line->sr);
                PhDereferenceObject(line);
                PhWriteStringAsUtf8FileStream2(fileStream, L"\r\n");
            }

            PhDereferenceObject(lines);
            PhDereferenceObject(fileStream);
        }

        if (!NT_SUCCESS(status))
            PhShowStatus(MemoryContext->WindowHandle, L"Unable to create the file", status, 0);
    }

    PhFreeFileDialog(fileDialog);
}

_Function_class_(PH_TYPE_DELETE_PROCEDURE)
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

VOID NTAPI PhpProcessMemorySearchControlCallback(
    _In_ ULONG_PTR MatchHandle,
    _In_opt_ PVOID Context
    )
{
    PPH_MEMORY_CONTEXT memoryContext = Context;

    assert(memoryContext);

    memoryContext->SearchMatchHandle = MatchHandle;

    // Expand any hidden nodes to make search results visible.
    PhExpandAllMemoryNodes(&memoryContext->ListContext, TRUE);

    PhApplyTreeNewFilters(&memoryContext->ListContext.AllocationTreeFilterSupport);
    PhApplyTreeNewFilters(&memoryContext->ListContext.TreeFilterSupport);
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

            // Initialize the list.
            PhInitializeMemoryList(hwndDlg, memoryContext->TreeNewHandle, &memoryContext->ListContext);
            TreeNew_SetEmptyText(memoryContext->TreeNewHandle, &PhProcessPropPageLoadingText, 0);

            memoryContext->AllocationFilterEntry = PhAddTreeNewFilter(&memoryContext->ListContext.AllocationTreeFilterSupport, PhpMemoryTreeFilterCallback, memoryContext);
            memoryContext->FilterEntry = PhAddTreeNewFilter(&memoryContext->ListContext.TreeFilterSupport, PhpMemoryTreeFilterCallback, memoryContext);

            PhCreateSearchControl(
                hwndDlg,
                memoryContext->SearchboxHandle,
                L"Search Memory (Ctrl+K)",
                PhpProcessMemorySearchControlCallback,
                memoryContext
                );

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

                        SystemInformer_ShowMemoryEditor(showMemoryEditor);
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

                        if (PhUiFreeMemory(hwndDlg, processItem->ProcessId, memoryNode->MemoryItem, TRUE))
                        {
                            PhRemoveMemoryNode(&memoryContext->ListContext, &memoryContext->MemoryItemList, memoryNode);
                        }

                        PhDereferenceObject(memoryNode->MemoryItem);
                    }
                }
                break;
            case ID_MEMORY_DECOMMIT:
                {
                    PPH_MEMORY_NODE memoryNode = PhGetSelectedMemoryNode(&memoryContext->ListContext);

                    if (memoryNode)
                    {
                        PhReferenceObject(memoryNode->MemoryItem);

                        if (PhUiFreeMemory(hwndDlg, processItem->ProcessId, memoryNode->MemoryItem, FALSE))
                        {
                            PhRemoveMemoryNode(&memoryContext->ListContext, &memoryContext->MemoryItemList, memoryNode);
                        }

                        PhDereferenceObject(memoryNode->MemoryItem);
                    }
                }
                break;
            case ID_MEMORY_EMPTYWORKINGSET:
                {
                    PPH_MEMORY_NODE memoryNode = PhGetSelectedMemoryNode(&memoryContext->ListContext);

                    if (memoryNode)
                    {
                        PhReferenceObject(memoryNode->MemoryItem);

                        if (PhUiEmptyProcessMemoryWorkingSet(hwndDlg, processItem->ProcessId, memoryNode->MemoryItem))
                        {
                            // Refresh counters or statistics.
                        }

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
                    PPH_EMENU_ITEM zeroPadItem;
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
                        PH_MEMORY_FILTER_MENU_MODIFIED,
                        PH_MEMORY_FILTER_MENU_STRINGS,
                        PH_MEMORY_FILTER_MENU_SAVE,
                        PH_MEMORY_FILTER_MENU_ZERO_PAD_ADDRESSES
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
                    PhInsertEMenuItem(menu, zeroPadItem = PhCreateEMenuItem(0, PH_MEMORY_FILTER_MENU_ZERO_PAD_ADDRESSES, L"Zero pad addresses", NULL, NULL), ULONG_MAX);
                    PhInsertEMenuItem(menu, PhCreateEMenuSeparator(), ULONG_MAX);
                    PhInsertEMenuItem(menu, PhCreateEMenuItem(0, PH_MEMORY_FILTER_MENU_READ_ADDRESS, L"Read/Write &address...", NULL, NULL), ULONG_MAX);
                    PhInsertEMenuItem(menu, PhCreateEMenuItem(0, PH_MEMORY_FILTER_MENU_HEAPS, L"Heaps...", NULL, NULL), ULONG_MAX);
                    PhInsertEMenuItem(menu, PhCreateEMenuItem(0, PH_MEMORY_FILTER_MENU_MODIFIED, L"Modified...", NULL, NULL), ULONG_MAX);
                    PhInsertEMenuItem(menu, PhCreateEMenuItem(0, PH_MEMORY_FILTER_MENU_STRINGS, L"Strings...", NULL, NULL), ULONG_MAX);
                    PhInsertEMenuItem(menu, PhCreateEMenuItem(0, PH_MEMORY_FILTER_MENU_SAVE, L"Save...", NULL, NULL), ULONG_MAX);

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
                    if (memoryContext->ListContext.ZeroPadAddresses)
                        zeroPadItem->Flags |= PH_EMENU_CHECKED;

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
                            ULONG id = ULONG_MAX;

                            switch (selectedItem->Id)
                            {
                            case PH_MEMORY_FILTER_MENU_HIDE_FREE:
                                id = PH_MEMORY_FLAGS_FREE_OPTION;
                                break;
                            case PH_MEMORY_FILTER_MENU_HIDE_RESERVED:
                                id = PH_MEMORY_FLAGS_RESERVED_OPTION;
                                break;
                            case PH_MEMORY_FILTER_MENU_HIDE_GUARD:
                                id = PH_MEMORY_FLAGS_GUARD_OPTION;
                                break;
                            case PH_MEMORY_FILTER_MENU_HIGHLIGHT_PRIVATE:
                                id = PH_MEMORY_FLAGS_PRIVATE_OPTION;
                                break;
                            case PH_MEMORY_FILTER_MENU_HIGHLIGHT_SYSTEM:
                                id = PH_MEMORY_FLAGS_SYSTEM_OPTION;
                                break;
                            case PH_MEMORY_FILTER_MENU_HIGHLIGHT_CFG:
                                id = PH_MEMORY_FLAGS_CFG_OPTION;
                                break;
                            case PH_MEMORY_FILTER_MENU_HIGHLIGHT_EXECUTE:
                                id = PH_MEMORY_FLAGS_EXECUTE_OPTION;
                                break;
                            }

                            if (id != ULONG_MAX)
                            {
                                PhSetOptionsMemoryList(&memoryContext->ListContext, selectedItem->Id);
                                PhSaveSettingsMemoryList(&memoryContext->ListContext);

                                PhApplyTreeNewFilters(&memoryContext->ListContext.AllocationTreeFilterSupport);
                                PhApplyTreeNewFilters(&memoryContext->ListContext.TreeFilterSupport);
                            }
                        }
                        else if (selectedItem->Id == PH_MEMORY_FILTER_MENU_ZERO_PAD_ADDRESSES)
                        {
                            PhSetOptionsMemoryList(&memoryContext->ListContext, PH_MEMORY_FLAGS_ZERO_PAD_ADDRESSES);
                            PhSaveSettingsMemoryList(&memoryContext->ListContext);

                            PhInvalidateAllMemoryBaseAddressNodes(&memoryContext->ListContext);
                        }
                        else if (selectedItem->Id == PH_MEMORY_FILTER_MENU_HEAPS)
                        {
                            PhShowProcessHeapsDialog(hwndDlg, processItem);
                        }
                        else if (selectedItem->Id == PH_MEMORY_FILTER_MENU_MODIFIED)
                        {
                            PhShowImagePageModifiedDialog(hwndDlg, processItem);
                        }
                        else if (selectedItem->Id == PH_MEMORY_FILTER_MENU_STRINGS)
                        {
                            if (PhGetIntegerSetting(L"EnableMemStringsTreeDialog"))
                                PhShowMemoryStringTreeDialog(hwndDlg, processItem);
                            else
                                PhShowMemoryStringDialog(hwndDlg, processItem);
                        }
                        else if (selectedItem->Id == PH_MEMORY_FILTER_MENU_SAVE)
                        {
                            PhpProcessMemorySave(memoryContext);
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

                                        SystemInformer_ShowMemoryEditor(showMemoryEditor);
                                        break;
                                    }
                                    else
                                    {
                                        PhShowStatus(hwndDlg, L"Unable to find the memory region for the selected address.", STATUS_UNSUCCESSFUL, 0);
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
                SetWindowLongPtr(hwndDlg, DWLP_MSGRESULT, (LPARAM)memoryContext->TreeNewHandle);
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
            else if (memoryContext->LastRunStatus == ULONG_MAX)
            {
                TreeNew_SetEmptyText(memoryContext->ListContext.TreeNewHandle, &EmptyMemoryText, 0);
                TreeNew_NodesStructured(memoryContext->ListContext.TreeNewHandle);
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
