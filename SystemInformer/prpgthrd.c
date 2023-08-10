/*
 * Copyright (c) 2022 Winsider Seminars & Solutions, Inc.  All rights reserved.
 *
 * This file is part of System Informer.
 *
 * Authors:
 *
 *     wj32    2009-2016
 *     dmex    2018-2023
 *
 */

#include <phapp.h>
#include <procprp.h>
#include <procprpp.h>
#include <cpysave.h>
#include <emenu.h>
#include <secedit.h>
#include <settings.h>
#include <symprv.h>

#include <actions.h>
#include <extmgri.h>
#include <phplug.h>
#include <phsettings.h>
#include <procprv.h>
#include <thrdprv.h>

static PH_STRINGREF EmptyThreadsText = PH_STRINGREF_INIT(L"There are no threads to display.");

static VOID NTAPI ThreadAddedHandler(
    _In_ PVOID Parameter,
    _In_ PVOID Context
    )
{
    PPH_THREADS_CONTEXT threadsContext = (PPH_THREADS_CONTEXT)Context;

    // Parameter contains a pointer to the added thread item.
    PhReferenceObject(Parameter);
    PhPushProviderEventQueue(&threadsContext->EventQueue, ProviderAddedEvent, Parameter, (ULONG)threadsContext->Provider->RunId);
}

static VOID NTAPI ThreadModifiedHandler(
    _In_ PVOID Parameter,
    _In_ PVOID Context
    )
{
    PPH_THREADS_CONTEXT threadsContext = (PPH_THREADS_CONTEXT)Context;

    PhPushProviderEventQueue(&threadsContext->EventQueue, ProviderModifiedEvent, Parameter, (ULONG)threadsContext->Provider->RunId);
}

static VOID NTAPI ThreadRemovedHandler(
    _In_ PVOID Parameter,
    _In_ PVOID Context
    )
{
    PPH_THREADS_CONTEXT threadsContext = (PPH_THREADS_CONTEXT)Context;

    PhPushProviderEventQueue(&threadsContext->EventQueue, ProviderRemovedEvent, Parameter, (ULONG)threadsContext->Provider->RunId);
}

static VOID NTAPI ThreadsUpdatedHandler(
    _In_ PVOID Parameter,
    _In_ PVOID Context
    )
{
    PPH_THREADS_CONTEXT threadsContext = (PPH_THREADS_CONTEXT)Context;

    PostMessage(threadsContext->WindowHandle, WM_PH_THREADS_UPDATED, (ULONG)threadsContext->Provider->RunId, threadsContext->Provider->RunId == 1);
}

static VOID NTAPI ThreadsLoadingStateChangedHandler(
    _In_ PVOID Parameter,
    _In_ PVOID Context
    )
{
    PPH_THREADS_CONTEXT threadsContext = (PPH_THREADS_CONTEXT)Context;

    //PostMessage(
    //    threadsContext->ListContext.TreeNewHandle,
    //    TNM_SETCURSOR,
    //    0,
    //    // Parameter contains TRUE if loading symbols
    //    (LPARAM)(Parameter ? PhLoadCursor(NULL, IDC_APPSTARTING) : NULL)
    //    );
}

VOID PhpInitializeThreadMenu(
    _In_ PPH_EMENU Menu,
    _In_ HANDLE ProcessId,
    _In_ PPH_THREAD_ITEM *Threads,
    _In_ ULONG NumberOfThreads
    )
{
    if (NumberOfThreads == 0)
    {
        PhSetFlagsAllEMenuItems(Menu, PH_EMENU_DISABLED, PH_EMENU_DISABLED);
    }
    else if (NumberOfThreads == 1)
    {
        // All menu items are enabled by default.
    }
    else
    {
        ULONG menuItemsMultiEnabled[] =
        {
            ID_THREAD_TERMINATE,
            ID_THREAD_SUSPEND,
            ID_THREAD_RESUME,
            ID_THREAD_COPY,
            ID_THREAD_AFFINITY,
        };
        ULONG i;
        PPH_EMENU_ITEM menuItem;

        PhSetFlagsAllEMenuItems(Menu, PH_EMENU_DISABLED, PH_EMENU_DISABLED);

        // These menu items are capable of manipulating
        // multiple threads.
        for (i = 0; i < sizeof(menuItemsMultiEnabled) / sizeof(ULONG); i++)
        {
            PhEnableEMenuItem(Menu, menuItemsMultiEnabled[i], TRUE);
        }

        if (menuItem = PhFindEMenuItem(Menu, PH_EMENU_FIND_DESCEND, L"&Priority", 0))
        {
            PhSetEnabledEMenuItem(menuItem, TRUE);
        }
    }

    PhEnableEMenuItem(Menu, ID_THREAD_TOKEN, FALSE);

    // Critical
    if (NumberOfThreads == 1)
    {
        HANDLE threadHandle;

        if (NT_SUCCESS(PhOpenThread(
            &threadHandle,
            THREAD_QUERY_INFORMATION,
            Threads[0]->ThreadId
            )))
        {
            BOOLEAN breakOnTermination;

            if (NT_SUCCESS(PhGetThreadBreakOnTermination(
                threadHandle,
                &breakOnTermination
                )))
            {
                if (breakOnTermination)
                {
                    PhSetFlagsEMenuItem(Menu, ID_THREAD_CRITICAL, PH_EMENU_CHECKED, PH_EMENU_CHECKED);
                }
            }

            NtClose(threadHandle);
        }
    }

    // Priority
    if (NumberOfThreads == 1)
    {
        HANDLE threadHandle;
        ULONG threadPriority = THREAD_PRIORITY_ERROR_RETURN;
        IO_PRIORITY_HINT ioPriority = ULONG_MAX;
        ULONG pagePriority = ULONG_MAX;
        BOOLEAN threadPriorityBoostDisabled = FALSE;
        ULONG id = 0;

        if (NT_SUCCESS(PhOpenThread(
            &threadHandle,
            THREAD_QUERY_LIMITED_INFORMATION,
            Threads[0]->ThreadId
            )))
        {
            HANDLE tokenHandle;

            PhGetThreadBasePriority(threadHandle, &threadPriority);
            PhGetThreadIoPriority(threadHandle, &ioPriority);
            PhGetThreadPagePriority(threadHandle, &pagePriority);
            PhGetThreadPriorityBoost(threadHandle, &threadPriorityBoostDisabled);

            if (NT_SUCCESS(PhOpenThreadToken(
                threadHandle,
                TOKEN_QUERY,
                TRUE,
                &tokenHandle
                )))
            {
                PhEnableEMenuItem(Menu, ID_THREAD_TOKEN, TRUE);
                NtClose(tokenHandle);
            }

            NtClose(threadHandle);
        }

        switch (threadPriority)
        {
        case THREAD_PRIORITY_TIME_CRITICAL + 1:
        case THREAD_PRIORITY_TIME_CRITICAL:
            id = ID_PRIORITY_TIMECRITICAL;
            break;
        case THREAD_PRIORITY_HIGHEST:
            id = ID_PRIORITY_HIGHEST;
            break;
        case THREAD_PRIORITY_ABOVE_NORMAL:
            id = ID_PRIORITY_ABOVENORMAL;
            break;
        case THREAD_PRIORITY_NORMAL:
            id = ID_PRIORITY_NORMAL;
            break;
        case THREAD_PRIORITY_BELOW_NORMAL:
            id = ID_PRIORITY_BELOWNORMAL;
            break;
        case THREAD_PRIORITY_LOWEST:
            id = ID_PRIORITY_LOWEST;
            break;
        case THREAD_PRIORITY_IDLE:
        case THREAD_PRIORITY_IDLE - 1:
            id = ID_PRIORITY_IDLE;
            break;
        }

        if (id != 0)
        {
            PhSetFlagsEMenuItem(Menu, id,
                PH_EMENU_CHECKED | PH_EMENU_RADIOCHECK,
                PH_EMENU_CHECKED | PH_EMENU_RADIOCHECK);
        }

        if (ioPriority != ULONG_MAX)
        {
            id = 0;

            switch (ioPriority)
            {
            case IoPriorityVeryLow:
                id = ID_IOPRIORITY_VERYLOW;
                break;
            case IoPriorityLow:
                id = ID_IOPRIORITY_LOW;
                break;
            case IoPriorityNormal:
                id = ID_IOPRIORITY_NORMAL;
                break;
            case IoPriorityHigh:
                id = ID_IOPRIORITY_HIGH;
                break;
            }

            if (id != 0)
            {
                PhSetFlagsEMenuItem(Menu, id,
                    PH_EMENU_CHECKED | PH_EMENU_RADIOCHECK,
                    PH_EMENU_CHECKED | PH_EMENU_RADIOCHECK);
            }
        }

        if (pagePriority != ULONG_MAX)
        {
            id = 0;

            switch (pagePriority)
            {
            case MEMORY_PRIORITY_VERY_LOW:
                id = ID_PAGEPRIORITY_VERYLOW;
                break;
            case MEMORY_PRIORITY_LOW:
                id = ID_PAGEPRIORITY_LOW;
                break;
            case MEMORY_PRIORITY_MEDIUM:
                id = ID_PAGEPRIORITY_MEDIUM;
                break;
            case MEMORY_PRIORITY_BELOW_NORMAL:
                id = ID_PAGEPRIORITY_BELOWNORMAL;
                break;
            case MEMORY_PRIORITY_NORMAL:
                id = ID_PAGEPRIORITY_NORMAL;
                break;
            }

            if (id != 0)
            {
                PhSetFlagsEMenuItem(Menu, id,
                    PH_EMENU_CHECKED | PH_EMENU_RADIOCHECK,
                    PH_EMENU_CHECKED | PH_EMENU_RADIOCHECK);
            }
        }

        if (!threadPriorityBoostDisabled)
        {
            PhSetFlagsEMenuItem(Menu, ID_THREAD_BOOST, PH_EMENU_CHECKED, PH_EMENU_CHECKED);
        }
    }
}

static NTSTATUS NTAPI PhpThreadPermissionsOpenThread(
    _Out_ PHANDLE Handle,
    _In_ ACCESS_MASK DesiredAccess,
    _In_opt_ PVOID Context
    )
{
    if (Context)
        return PhOpenThread(Handle, DesiredAccess, (HANDLE)Context);

    return STATUS_UNSUCCESSFUL;
}

static NTSTATUS NTAPI PhpOpenThreadTokenObject(
    _Out_ PHANDLE Handle,
    _In_ ACCESS_MASK DesiredAccess,
    _In_opt_ PVOID Context
    )
{
    if (Context)
        return PhOpenThreadToken((HANDLE)Context, DesiredAccess, TRUE, Handle);

    return STATUS_UNSUCCESSFUL;
}

BOOLEAN PhpThreadTreeFilterCallback(
    _In_ PPH_TREENEW_NODE Node,
    _In_opt_ PPH_THREADS_CONTEXT Context
    )
{
    PPH_THREAD_NODE threadNode = (PPH_THREAD_NODE)Node;
    PPH_THREAD_ITEM threadItem = threadNode->ThreadItem;

    if (Context->ListContext.HideSuspended && threadItem->WaitReason == Suspended)
        return FALSE;
    if (Context->ListContext.HideGuiThreads && threadItem->IsGuiThread)
        return FALSE;

    if (PhIsNullOrEmptyString(Context->SearchboxText))
        return TRUE;

    // thread properties

    if (threadItem->ThreadIdString[0])
    {
        if (PhWordMatchStringLongHintZ(Context->SearchboxText, threadItem->ThreadIdString))
            return TRUE;
    }

    if (threadNode->PriorityText[0])
    {
        if (PhWordMatchStringLongHintZ(Context->SearchboxText, threadNode->PriorityText))
            return TRUE;
    }

    if (threadNode->BasePriorityText[0])
    {
        if (PhWordMatchStringLongHintZ(Context->SearchboxText, threadNode->BasePriorityText))
            return TRUE;
    }

    if (threadNode->IdealProcessorText[0])
    {
        if (PhWordMatchStringLongHintZ(Context->SearchboxText, threadNode->IdealProcessorText))
            return TRUE;
    }

    if (threadNode->ThreadIdHexText[0])
    {
        if (PhWordMatchStringLongHintZ(Context->SearchboxText, threadNode->ThreadIdHexText))
            return TRUE;
    }

    if (!PhIsNullOrEmptyString(threadNode->StartAddressText))
    {
        if (PhWordMatchStringRef(&Context->SearchboxText->sr, &threadNode->StartAddressText->sr))
            return TRUE;
    }

    if (!PhIsNullOrEmptyString(threadNode->PrioritySymbolicText))
    {
        if (PhWordMatchStringRef(&Context->SearchboxText->sr, &threadNode->PrioritySymbolicText->sr))
            return TRUE;
    }

    if (!PhIsNullOrEmptyString(threadNode->CreatedText))
    {
        if (PhWordMatchStringRef(&Context->SearchboxText->sr, &threadNode->CreatedText->sr))
            return TRUE;
    }

    if (!PhIsNullOrEmptyString(threadNode->NameText))
    {
        if (PhWordMatchStringRef(&Context->SearchboxText->sr, &threadNode->NameText->sr))
            return TRUE;
    }

    if (!PhIsNullOrEmptyString(threadNode->StateText))
    {
        if (PhWordMatchStringRef(&Context->SearchboxText->sr, &threadNode->StateText->sr))
            return TRUE;
    }

    if (!PhIsNullOrEmptyString(threadNode->LastSystemCallText))
    {
        if (PhWordMatchStringRef(&Context->SearchboxText->sr, &threadNode->LastSystemCallText->sr))
            return TRUE;
    }

    if (!PhIsNullOrEmptyString(threadNode->LastErrorCodeText))
    {
        if (PhWordMatchStringRef(&Context->SearchboxText->sr, &threadNode->LastErrorCodeText->sr))
            return TRUE;
    }

    if (!PhIsNullOrEmptyString(threadNode->ThreadItem->ServiceName))
    {
        if (PhWordMatchStringRef(&Context->SearchboxText->sr, &threadNode->ThreadItem->ServiceName->sr))
            return TRUE;
    }

    if (!PhIsNullOrEmptyString(threadNode->ThreadItem->StartAddressString))
    {
        if (PhWordMatchStringRef(&Context->SearchboxText->sr, &threadNode->ThreadItem->StartAddressString->sr))
            return TRUE;
    }

    if (!PhIsNullOrEmptyString(threadNode->ThreadItem->StartAddressFileName))
    {
        if (PhWordMatchStringRef(&Context->SearchboxText->sr, &threadNode->ThreadItem->StartAddressFileName->sr))
            return TRUE;
    }

    return FALSE;
}

PPH_EMENU PhpCreateThreadMenu(
    VOID
    )
{
    PPH_EMENU menu;
    PPH_EMENU_ITEM menuItem;

    menu = PhCreateEMenu();
    PhInsertEMenuItem(menu, PhCreateEMenuItem(0, ID_THREAD_INSPECT, L"&Inspect\bEnter", NULL, NULL), ULONG_MAX);
    PhInsertEMenuItem(menu, PhCreateEMenuItem(0, ID_THREAD_TERMINATE, L"T&erminate\bDel", NULL, NULL), ULONG_MAX);
    PhInsertEMenuItem(menu, PhCreateEMenuItem(0, ID_THREAD_SUSPEND, L"&Suspend", NULL, NULL), ULONG_MAX);
    PhInsertEMenuItem(menu, PhCreateEMenuItem(0, ID_THREAD_RESUME, L"Res&ume", NULL, NULL), ULONG_MAX);
    PhInsertEMenuItem(menu, PhCreateEMenuSeparator(), ULONG_MAX);
    PhInsertEMenuItem(menu, PhCreateEMenuItem(0, ID_ANALYZE_WAIT, L"Analy&ze", NULL, NULL), ULONG_MAX);
    PhInsertEMenuItem(menu, PhCreateEMenuItem(0, ID_THREAD_AFFINITY, L"&Affinity", NULL, NULL), ULONG_MAX);
    PhInsertEMenuItem(menu, PhCreateEMenuItem(0, ID_THREAD_BOOST, L"&Boost", NULL, NULL), ULONG_MAX);
    PhInsertEMenuItem(menu, PhCreateEMenuItem(0, ID_THREAD_CRITICAL, L"Critical", NULL, NULL), ULONG_MAX);
    PhInsertEMenuItem(menu, PhCreateEMenuItem(0, ID_THREAD_PERMISSIONS, L"Per&missions", NULL, NULL), ULONG_MAX);
    PhInsertEMenuItem(menu, PhCreateEMenuItem(0, ID_THREAD_TOKEN, L"&Token", NULL, NULL), ULONG_MAX);

    menuItem = PhCreateEMenuItem(0, 0, L"&Priority", NULL, NULL);
    PhInsertEMenuItem(menuItem, PhCreateEMenuItem(0, ID_PRIORITY_TIMECRITICAL, L"Time &critical", NULL, NULL), ULONG_MAX);
    PhInsertEMenuItem(menuItem, PhCreateEMenuItem(0, ID_PRIORITY_HIGHEST, L"&Highest", NULL, NULL), ULONG_MAX);
    PhInsertEMenuItem(menuItem, PhCreateEMenuItem(0, ID_PRIORITY_ABOVENORMAL, L"&Above normal", NULL, NULL), ULONG_MAX);
    PhInsertEMenuItem(menuItem, PhCreateEMenuItem(0, ID_PRIORITY_NORMAL, L"&Normal", NULL, NULL), ULONG_MAX);
    PhInsertEMenuItem(menuItem, PhCreateEMenuItem(0, ID_PRIORITY_BELOWNORMAL, L"&Below normal", NULL, NULL), ULONG_MAX);
    PhInsertEMenuItem(menuItem, PhCreateEMenuItem(0, ID_PRIORITY_LOWEST, L"&Lowest", NULL, NULL), ULONG_MAX);
    PhInsertEMenuItem(menuItem, PhCreateEMenuItem(0, ID_PRIORITY_IDLE, L"&Idle", NULL, NULL), ULONG_MAX);
    PhInsertEMenuItem(menu, menuItem, ULONG_MAX);

    menuItem = PhCreateEMenuItem(0, 0, L"&I/O priority", NULL, NULL);
    PhInsertEMenuItem(menuItem, PhCreateEMenuItem(0, ID_IOPRIORITY_HIGH, L"&High", NULL, NULL), ULONG_MAX);
    PhInsertEMenuItem(menuItem, PhCreateEMenuItem(0, ID_IOPRIORITY_NORMAL, L"&Normal", NULL, NULL), ULONG_MAX);
    PhInsertEMenuItem(menuItem, PhCreateEMenuItem(0, ID_IOPRIORITY_LOW, L"&Low", NULL, NULL), ULONG_MAX);
    PhInsertEMenuItem(menuItem, PhCreateEMenuItem(0, ID_IOPRIORITY_VERYLOW, L"&Very low", NULL, NULL), ULONG_MAX);
    PhInsertEMenuItem(menu, menuItem, ULONG_MAX);

    menuItem = PhCreateEMenuItem(0, 0, L"Pa&ge priority", NULL, NULL);
    PhInsertEMenuItem(menuItem, PhCreateEMenuItem(0, ID_PAGEPRIORITY_NORMAL, L"&Normal", NULL, NULL), ULONG_MAX);
    PhInsertEMenuItem(menuItem, PhCreateEMenuItem(0, ID_PAGEPRIORITY_BELOWNORMAL, L"&Below normal", NULL, NULL), ULONG_MAX);
    PhInsertEMenuItem(menuItem, PhCreateEMenuItem(0, ID_PAGEPRIORITY_MEDIUM, L"&Medium", NULL, NULL), ULONG_MAX);
    PhInsertEMenuItem(menuItem, PhCreateEMenuItem(0, ID_PAGEPRIORITY_LOW, L"&Low", NULL, NULL), ULONG_MAX);
    PhInsertEMenuItem(menuItem, PhCreateEMenuItem(0, ID_PAGEPRIORITY_VERYLOW, L"&Very low", NULL, NULL), ULONG_MAX);
    PhInsertEMenuItem(menu, menuItem, ULONG_MAX);

    PhInsertEMenuItem(menu, PhCreateEMenuSeparator(), ULONG_MAX);
    PhInsertEMenuItem(menu, PhCreateEMenuItem(0, ID_THREAD_COPY, L"&Copy\bCtrl+C", NULL, NULL), ULONG_MAX);

    return menu;
}

VOID PhShowThreadContextMenu(
    _In_ HWND hwndDlg,
    _In_ PPH_PROCESS_ITEM ProcessItem,
    _In_ PPH_THREADS_CONTEXT Context,
    _In_ PPH_TREENEW_CONTEXT_MENU ContextMenu
    )
{
    PPH_THREAD_ITEM *threads;
    ULONG numberOfThreads;

    PhGetSelectedThreadItems(&Context->ListContext, &threads, &numberOfThreads);

    if (numberOfThreads != 0)
    {
        PPH_EMENU menu;
        PPH_EMENU_ITEM item;
        PH_PLUGIN_MENU_INFORMATION menuInfo;

        menu = PhpCreateThreadMenu();
        PhSetFlagsEMenuItem(menu, ID_THREAD_INSPECT, PH_EMENU_DEFAULT, PH_EMENU_DEFAULT);

        PhpInitializeThreadMenu(menu, ProcessItem->ProcessId, threads, numberOfThreads);
        PhInsertCopyCellEMenuItem(menu, ID_THREAD_COPY, Context->ListContext.TreeNewHandle, ContextMenu->Column);

        if (PhPluginsEnabled)
        {
            PhPluginInitializeMenuInfo(&menuInfo, menu, hwndDlg, 0);
            menuInfo.u.Thread.ProcessId = ProcessItem->ProcessId;
            menuInfo.u.Thread.Threads = threads;
            menuInfo.u.Thread.NumberOfThreads = numberOfThreads;

            PhInvokeCallback(PhGetGeneralCallback(GeneralCallbackThreadMenuInitializing), &menuInfo);
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

    PhFree(threads);
}

VOID PhpPopulateTableWithProcessThreadNodes(
    _In_ HWND TreeListHandle,
    _In_ PPH_THREAD_NODE Node,
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
}

PPH_LIST PhpGetProcessThreadTreeListLines(
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
        PhpPopulateTableWithProcessThreadNodes(
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

VOID PhpProcessThreadsSave(
    _In_ PPH_THREADS_CONTEXT ThreadsContext
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

    processItem = PhReferenceProcessItem(ThreadsContext->Provider->ProcessId);
    PhInitFormatS(&format[0], L"System Informer (");
    PhInitFormatS(&format[1], PhGetStringOrDefault(processItem->ProcessName, L"Unknown process"));
    PhInitFormatS(&format[2], L") Threads");
    PhInitFormatS(&format[3], L".txt");
    if (processItem) PhDereferenceObject(processItem);

    PhSetFileDialogFilter(fileDialog, filters, sizeof(filters) / sizeof(PH_FILETYPE_FILTER));
    PhSetFileDialogFileName(fileDialog, PH_AUTO_T(PH_STRING, PhFormat(format, 3, 60))->Buffer);

    if (PhShowFileDialog(ThreadsContext->WindowHandle, fileDialog))
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
            PhWritePhTextHeader(fileStream);

            lines = PhpGetProcessThreadTreeListLines(
                ThreadsContext->TreeNewHandle,
                ThreadsContext->ListContext.NodeList->Count,
                ThreadsContext->ListContext.NodeList,
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
            PhShowStatus(ThreadsContext->WindowHandle, L"Unable to create the file", status, 0);
    }

    PhFreeFileDialog(fileDialog);
}

VOID PhpSymbolProviderEventCallbackThreadStatus(
    _In_ PVOID Parameter,
    _In_ PVOID Context
    )
{
    PPH_SYMBOL_EVENT_DATA event = Parameter;
    PPH_THREADS_CONTEXT context = Context;
    PPH_STRING statusMessage = NULL;

    switch (event->EventType)
    {
    case PH_SYMBOL_EVENT_TYPE_LOAD_START:
        statusMessage = PhReferenceObject(event->EventMessage);
        break;
    case PH_SYMBOL_EVENT_TYPE_LOAD_END:
        statusMessage = PhReferenceEmptyString();
        break;
    case PH_SYMBOL_EVENT_TYPE_PROGRESS:
        statusMessage = PhReferenceObject(event->EventMessage);
        break;
    }

    if (statusMessage)
    {
        PhSetWindowText(context->StatusHandle, PhGetStringOrEmpty(statusMessage));
        PhDereferenceObject(statusMessage);
    }
}

INT_PTR CALLBACK PhpProcessThreadsDlgProc(
    _In_ HWND hwndDlg,
    _In_ UINT uMsg,
    _In_ WPARAM wParam,
    _In_ LPARAM lParam
    )
{
    LPPROPSHEETPAGE propSheetPage;
    PPH_PROCESS_PROPPAGECONTEXT propPageContext;
    PPH_PROCESS_ITEM processItem;
    PPH_THREADS_CONTEXT threadsContext;

    if (PhPropPageDlgProcHeader(hwndDlg, uMsg, lParam, &propSheetPage, &propPageContext, &processItem))
    {
        threadsContext = (PPH_THREADS_CONTEXT)propPageContext->Context;
    }
    else
    {
        return FALSE;
    }

    switch (uMsg)
    {
    case WM_INITDIALOG:
        {
            threadsContext = propPageContext->Context = PhAllocate(PhEmGetObjectSize(EmThreadsContextType, sizeof(PH_THREADS_CONTEXT)));
            memset(threadsContext, 0, sizeof(PH_THREADS_CONTEXT));

            threadsContext->WindowHandle = hwndDlg;
            threadsContext->TreeNewHandle = GetDlgItem(hwndDlg, IDC_LIST);
            threadsContext->SearchboxHandle = GetDlgItem(hwndDlg, IDC_SEARCH);
            threadsContext->StatusHandle = GetDlgItem(hwndDlg, IDC_STATE);

            // The thread provider has a special registration mechanism.
            threadsContext->Provider = PhCreateThreadProvider(
                processItem->ProcessId
                );
            PhRegisterCallback(
                &threadsContext->Provider->ThreadAddedEvent,
                ThreadAddedHandler,
                threadsContext,
                &threadsContext->AddedEventRegistration
                );
            PhRegisterCallback(
                &threadsContext->Provider->ThreadModifiedEvent,
                ThreadModifiedHandler,
                threadsContext,
                &threadsContext->ModifiedEventRegistration
                );
            PhRegisterCallback(
                &threadsContext->Provider->ThreadRemovedEvent,
                ThreadRemovedHandler,
                threadsContext,
                &threadsContext->RemovedEventRegistration
                );
            PhRegisterCallback(
                &threadsContext->Provider->UpdatedEvent,
                ThreadsUpdatedHandler,
                threadsContext,
                &threadsContext->UpdatedEventRegistration
                );
            PhRegisterCallback(
                &threadsContext->Provider->LoadingStateChangedEvent,
                ThreadsLoadingStateChangedHandler,
                threadsContext,
                &threadsContext->LoadingStateChangedEventRegistration
                );

            // Initialize the list. (wj32)
            PhInitializeThreadList(hwndDlg, threadsContext->TreeNewHandle, &threadsContext->ListContext);
            TreeNew_SetEmptyText(threadsContext->TreeNewHandle, &EmptyThreadsText, 0);
            PhInitializeProviderEventQueue(&threadsContext->EventQueue, 100);
            threadsContext->SearchboxText = PhReferenceEmptyString();
            threadsContext->FilterEntry = PhAddTreeNewFilter(&threadsContext->ListContext.TreeFilterSupport, PhpThreadTreeFilterCallback, threadsContext);
            threadsContext->ListContext.ProcessId = processItem->ProcessId;
            threadsContext->ListContext.ProcessCreateTime = processItem->CreateTime;

            // Initialize the search box. (dmex)
            PhCreateSearchControl(hwndDlg, threadsContext->SearchboxHandle, L"Search Threads (Ctrl+K)");

            // Use Cycles instead of Context Switches on Vista and above, but only when we can
            // open the process, since cycle time information requires sufficient access to the
            // threads. (wj32)
            {
                HANDLE processHandle;

                // We make a distinction between PROCESS_QUERY_INFORMATION and PROCESS_QUERY_LIMITED_INFORMATION since
                // the latter can be used when opening audiodg.exe even though we can't access its threads using
                // THREAD_QUERY_LIMITED_INFORMATION. (wj32)

                if (processItem->ProcessId == SYSTEM_IDLE_PROCESS_ID)
                {
                    threadsContext->ListContext.UseCycleTime = TRUE;
                }
                else if (NT_SUCCESS(PhOpenProcess(&processHandle, PROCESS_QUERY_INFORMATION, processItem->ProcessId)))
                {
                    threadsContext->ListContext.UseCycleTime = TRUE;
                    NtClose(processHandle);
                }
                else if (NT_SUCCESS(PhOpenProcess(&processHandle, PROCESS_QUERY_LIMITED_INFORMATION, processItem->ProcessId)))
                {
                    threadsContext->ListContext.UseCycleTime = TRUE;

                    // We can't use cycle time for protected processes (without KSystemInformer). (wj32)
                    if (processItem->IsProtectedProcess)
                    {
                        // Windows 10 allows elevated processes to query cycle time for protected processes (dmex)
                        if (WindowsVersion < WINDOWS_10 || !PhGetOwnTokenAttributes().Elevated)
                        {
                            threadsContext->ListContext.UseCycleTime = FALSE;
                        }
                    }

                    NtClose(processHandle);
                }
            }

            if (processItem->ServiceList && processItem->ServiceList->Count != 0)
                threadsContext->ListContext.HasServices = TRUE;

            PhEmCallObjectOperation(EmThreadsContextType, threadsContext, EmObjectCreate);

            if (PhPluginsEnabled)
            {
                PH_PLUGIN_TREENEW_INFORMATION treeNewInfo;

                treeNewInfo.TreeNewHandle = threadsContext->TreeNewHandle;
                treeNewInfo.CmData = &threadsContext->ListContext.Cm;
                treeNewInfo.SystemContext = threadsContext;
                PhInvokeCallback(PhGetGeneralCallback(GeneralCallbackThreadTreeNewInitializing), &treeNewInfo);
            }

            PhLoadSettingsThreadList(&threadsContext->ListContext);

            PhThreadProviderInitialUpdate(threadsContext->Provider);
            PhRegisterThreadProvider(threadsContext->Provider, &threadsContext->ProviderRegistration);

            if (threadsContext->Provider->SymbolProvider)
            {
                PhRegisterCallback(
                    &PhSymbolEventCallback,
                    PhpSymbolProviderEventCallbackThreadStatus,
                    threadsContext,
                    &threadsContext->SymbolProviderEventRegistration
                    );
            }

            PhInitializeWindowTheme(hwndDlg, PhEnableThemeSupport);
        }
        break;
    case WM_DESTROY:
        {
            if (threadsContext->Provider->SymbolProvider)
            {
                PhUnregisterCallback(
                    &PhSymbolEventCallback,
                    &threadsContext->SymbolProviderEventRegistration
                    );
            }

            PhRemoveTreeNewFilter(&threadsContext->ListContext.TreeFilterSupport, threadsContext->FilterEntry);
            if (threadsContext->SearchboxText) PhDereferenceObject(threadsContext->SearchboxText);

            PhEmCallObjectOperation(EmThreadsContextType, threadsContext, EmObjectDelete);

            PhUnregisterCallback(
                &threadsContext->Provider->ThreadAddedEvent,
                &threadsContext->AddedEventRegistration
                );
            PhUnregisterCallback(
                &threadsContext->Provider->ThreadModifiedEvent,
                &threadsContext->ModifiedEventRegistration
                );
            PhUnregisterCallback(
                &threadsContext->Provider->ThreadRemovedEvent,
                &threadsContext->RemovedEventRegistration
                );
            PhUnregisterCallback(
                &threadsContext->Provider->UpdatedEvent,
                &threadsContext->UpdatedEventRegistration
                );
            PhUnregisterCallback(
                &threadsContext->Provider->LoadingStateChangedEvent,
                &threadsContext->LoadingStateChangedEventRegistration
                );
            PhUnregisterThreadProvider(threadsContext->Provider, &threadsContext->ProviderRegistration);
            PhSetTerminatingThreadProvider(threadsContext->Provider);
            PhDereferenceObject(threadsContext->Provider);
            PhDeleteProviderEventQueue(&threadsContext->EventQueue);

            if (PhPluginsEnabled)
            {
                PH_PLUGIN_TREENEW_INFORMATION treeNewInfo;

                treeNewInfo.TreeNewHandle = threadsContext->TreeNewHandle;
                treeNewInfo.CmData = &threadsContext->ListContext.Cm;
                PhInvokeCallback(PhGetGeneralCallback(GeneralCallbackThreadTreeNewUninitializing), &treeNewInfo);
            }

            PhSaveSettingsThreadList(&threadsContext->ListContext);
            PhDeleteThreadList(&threadsContext->ListContext);

            PhFree(threadsContext);
        }
        break;
    case WM_SHOWWINDOW:
        {
            PPH_LAYOUT_ITEM dialogItem;

            if (dialogItem = PhBeginPropPageLayout(hwndDlg, propPageContext))
            {
                PhAddPropPageLayoutItem(hwndDlg, threadsContext->StatusHandle, dialogItem, PH_ANCHOR_LEFT | PH_ANCHOR_TOP | PH_ANCHOR_RIGHT | PH_LAYOUT_FORCE_INVALIDATE);
                PhAddPropPageLayoutItem(hwndDlg, threadsContext->SearchboxHandle, dialogItem, PH_ANCHOR_TOP | PH_ANCHOR_RIGHT);
                PhAddPropPageLayoutItem(hwndDlg, threadsContext->TreeNewHandle, dialogItem, PH_ANCHOR_ALL);
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

                    if (GET_WM_COMMAND_HWND(wParam, lParam) != threadsContext->SearchboxHandle)
                        break;

                    newSearchboxText = PH_AUTO(PhGetWindowText(threadsContext->SearchboxHandle));

                    if (!PhEqualString(threadsContext->SearchboxText, newSearchboxText, FALSE))
                    {
                        // Cache the current search text for our callback.
                        PhSwapReference(&threadsContext->SearchboxText, newSearchboxText);

                        PhApplyTreeNewFilters(&threadsContext->ListContext.TreeFilterSupport);
                    }
                }
                break;
            }

            switch (GET_WM_COMMAND_ID(wParam, lParam))
            {
            case ID_SHOWCONTEXTMENU:
                {
                    PhShowThreadContextMenu(hwndDlg, processItem, threadsContext, (PPH_TREENEW_CONTEXT_MENU)lParam);
                }
                break;
            case ID_THREAD_INSPECT:
                {
                    PPH_THREAD_ITEM threadItem = PhGetSelectedThreadItem(&threadsContext->ListContext);

                    if (threadItem)
                    {
                        PhReferenceObject(threadsContext->Provider);
                        PhShowThreadStackDialog(
                            hwndDlg,
                            threadsContext->Provider->ProcessId,
                            threadItem->ThreadId,
                            threadsContext->Provider
                            );
                        PhDereferenceObject(threadsContext->Provider);
                    }
                }
                break;
            case ID_THREAD_TERMINATE:
                {
                    PPH_THREAD_ITEM *threads;
                    ULONG numberOfThreads;

                    PhGetSelectedThreadItems(&threadsContext->ListContext, &threads, &numberOfThreads);
                    PhReferenceObjects(threads, numberOfThreads);

                    if (PhUiTerminateThreads(hwndDlg, threads, numberOfThreads))
                        PhDeselectAllThreadNodes(&threadsContext->ListContext);

                    PhDereferenceObjects(threads, numberOfThreads);
                    PhFree(threads);
                }
                break;
            case ID_THREAD_SUSPEND:
                {
                    PPH_THREAD_ITEM *threads;
                    ULONG numberOfThreads;

                    PhGetSelectedThreadItems(&threadsContext->ListContext, &threads, &numberOfThreads);
                    PhReferenceObjects(threads, numberOfThreads);
                    PhUiSuspendThreads(hwndDlg, threads, numberOfThreads);
                    PhDereferenceObjects(threads, numberOfThreads);
                    PhFree(threads);
                }
                break;
            case ID_THREAD_RESUME:
                {
                    PPH_THREAD_ITEM *threads;
                    ULONG numberOfThreads;

                    PhGetSelectedThreadItems(&threadsContext->ListContext, &threads, &numberOfThreads);
                    PhReferenceObjects(threads, numberOfThreads);
                    PhUiResumeThreads(hwndDlg, threads, numberOfThreads);
                    PhDereferenceObjects(threads, numberOfThreads);
                    PhFree(threads);
                }
                break;
            case ID_THREAD_AFFINITY:
                {
                    ULONG numberOfThreads;
                    PPH_THREAD_ITEM* threads;

                    PhGetSelectedThreadItems(&threadsContext->ListContext, &threads, &numberOfThreads);
                    PhReferenceObjects(threads, numberOfThreads);

                    if (numberOfThreads == 1) // HACK
                    {
                        PhShowProcessAffinityDialog(hwndDlg, NULL, threads[0]);
                    }
                    else
                    {
                        PhShowThreadAffinityDialog(hwndDlg, threads, numberOfThreads);
                    }

                    PhDereferenceObjects(threads, numberOfThreads);
                    PhFree(threads);
                }
                break;
            case ID_THREAD_BOOST:
                {
                    PPH_THREAD_ITEM threadItem = PhGetSelectedThreadItem(&threadsContext->ListContext);

                    if (threadItem)
                    {
                        NTSTATUS status;
                        HANDLE threadHandle;

                        if (NT_SUCCESS(status = PhOpenThread(
                            &threadHandle,
                            THREAD_QUERY_LIMITED_INFORMATION,
                            threadItem->ThreadId
                            )))
                        {
                            BOOLEAN threadPriorityBoostDisabled = FALSE;
                            ULONG numberOfThreads;
                            PPH_THREAD_ITEM* threads;

                            PhGetThreadPriorityBoost(threadHandle, &threadPriorityBoostDisabled);

                            PhGetSelectedThreadItems(&threadsContext->ListContext, &threads, &numberOfThreads);
                            PhReferenceObjects(threads, numberOfThreads);
                            PhUiSetBoostPriorityThreads(hwndDlg, threads, numberOfThreads, !threadPriorityBoostDisabled);
                            PhDereferenceObjects(threads, numberOfThreads);
                            PhFree(threads);

                            NtClose(threadHandle);
                        }
                        else
                        {
                            PhShowStatus(hwndDlg, PhaFormatString(
                                L"Unable to %s thread %lu", // string pooling optimization (dmex)
                                L"set the boost priority of",
                                HandleToUlong(threadItem->ThreadId)
                                )->Buffer, status, 0);
                        }
                    }
                }
                break;
            case ID_THREAD_CRITICAL:
                {
                    PPH_THREAD_ITEM threadItem = PhGetSelectedThreadItem(&threadsContext->ListContext);

                    if (threadItem)
                    {
                        NTSTATUS status;
                        HANDLE threadHandle;
                        BOOLEAN breakOnTermination;

                        status = PhOpenThread(
                            &threadHandle,
                            THREAD_QUERY_INFORMATION | THREAD_SET_INFORMATION,
                            threadItem->ThreadId
                            );

                        if (NT_SUCCESS(status))
                        {
                            status = PhGetThreadBreakOnTermination(
                                threadHandle,
                                &breakOnTermination
                                );

                            if (NT_SUCCESS(status))
                            {
                                if (!breakOnTermination && (!PhGetIntegerSetting(L"EnableWarnings") || PhShowConfirmMessage(
                                    hwndDlg,
                                    L"enable",
                                    L"critical status on the thread",
                                    L"If the process ends, the operating system will shut down immediately.",
                                    TRUE
                                    )))
                                {
                                    status = PhSetThreadBreakOnTermination(threadHandle, TRUE);
                                }
                                else if (breakOnTermination && (!PhGetIntegerSetting(L"EnableWarnings") || PhShowConfirmMessage(
                                    hwndDlg,
                                    L"disable",
                                    L"critical status on the thread",
                                    NULL,
                                    FALSE
                                    )))
                                {
                                    status = PhSetThreadBreakOnTermination(threadHandle, FALSE);
                                }
                            }

                            NtClose(threadHandle);
                        }

                        if (!NT_SUCCESS(status))
                        {
                            PhShowStatus(hwndDlg, L"Unable to change the thread critical status.", status, 0);
                        }
                    }
                }
                break;
            case ID_THREAD_PERMISSIONS:
                {
                    PPH_THREAD_ITEM threadItem = PhGetSelectedThreadItem(&threadsContext->ListContext);

                    if (threadItem)
                    {
                        PhEditSecurity(
                            PhCsForceNoParent ? NULL : hwndDlg,
                            PhaFormatString(L"Thread %u", HandleToUlong(threadItem->ThreadId))->Buffer,
                            L"Thread",
                            PhpThreadPermissionsOpenThread,
                            NULL,
                            threadItem->ThreadId
                            );
                    }
                }
                break;
            case ID_THREAD_TOKEN:
                {
                    NTSTATUS status;
                    PPH_THREAD_ITEM threadItem = PhGetSelectedThreadItem(&threadsContext->ListContext);
                    HANDLE threadHandle;

                    if (threadItem)
                    {
                        if (NT_SUCCESS(status = PhOpenThread(
                            &threadHandle,
                            THREAD_QUERY_LIMITED_INFORMATION,
                            threadItem->ThreadId
                            )))
                        {
                            PhShowTokenProperties(
                                hwndDlg,
                                PhpOpenThreadTokenObject,
                                threadsContext->Provider->ProcessId,
                                (PVOID)threadHandle,
                                NULL
                                );

                            NtClose(threadHandle);
                        }
                        else
                        {
                            PhShowStatus(hwndDlg, L"Unable to open the thread", status, 0);
                        }
                    }
                }
                break;
            case ID_ANALYZE_WAIT:
                {
                    PPH_THREAD_ITEM threadItem = PhGetSelectedThreadItem(&threadsContext->ListContext);

                    if (threadItem)
                    {
                        PhReferenceObject(threadsContext->Provider->SymbolProvider);
                        PhUiAnalyzeWaitThread(
                            hwndDlg,
                            processItem->ProcessId,
                            threadItem->ThreadId,
                            threadsContext->Provider->SymbolProvider
                            );
                        PhDereferenceObject(threadsContext->Provider->SymbolProvider);
                    }
                }
                break;
            case ID_PRIORITY_TIMECRITICAL:
            case ID_PRIORITY_HIGHEST:
            case ID_PRIORITY_ABOVENORMAL:
            case ID_PRIORITY_NORMAL:
            case ID_PRIORITY_BELOWNORMAL:
            case ID_PRIORITY_LOWEST:
            case ID_PRIORITY_IDLE:
                {
                    LONG threadPriorityWin32 = LONG_MAX;

                    switch (GET_WM_COMMAND_ID(wParam, lParam))
                    {
                    case ID_PRIORITY_TIMECRITICAL:
                        threadPriorityWin32 = THREAD_PRIORITY_TIME_CRITICAL;
                        break;
                    case ID_PRIORITY_HIGHEST:
                        threadPriorityWin32 = THREAD_PRIORITY_HIGHEST;
                        break;
                    case ID_PRIORITY_ABOVENORMAL:
                        threadPriorityWin32 = THREAD_PRIORITY_ABOVE_NORMAL;
                        break;
                    case ID_PRIORITY_NORMAL:
                        threadPriorityWin32 = THREAD_PRIORITY_NORMAL;
                        break;
                    case ID_PRIORITY_BELOWNORMAL:
                        threadPriorityWin32 = THREAD_PRIORITY_BELOW_NORMAL;
                        break;
                    case ID_PRIORITY_LOWEST:
                        threadPriorityWin32 = THREAD_PRIORITY_LOWEST;
                        break;
                    case ID_PRIORITY_IDLE:
                        threadPriorityWin32 = THREAD_PRIORITY_IDLE;
                        break;
                    }

                    if (threadPriorityWin32 != LONG_MAX)
                    {
                        ULONG numberOfThreads;
                        PPH_THREAD_ITEM* threads;

                        PhGetSelectedThreadItems(&threadsContext->ListContext, &threads, &numberOfThreads);
                        PhReferenceObjects(threads, numberOfThreads);
                        PhUiSetPriorityThreads(hwndDlg, threads, numberOfThreads, threadPriorityWin32);
                        PhDereferenceObjects(threads, numberOfThreads);
                        PhFree(threads);
                    }
                }
                break;
            case ID_IOPRIORITY_VERYLOW:
            case ID_IOPRIORITY_LOW:
            case ID_IOPRIORITY_NORMAL:
            case ID_IOPRIORITY_HIGH:
                {
                    PPH_THREAD_ITEM threadItem = PhGetSelectedThreadItem(&threadsContext->ListContext);

                    if (threadItem)
                    {
                        IO_PRIORITY_HINT ioPriority = ULONG_MAX;

                        switch (GET_WM_COMMAND_ID(wParam, lParam))
                        {
                        case ID_IOPRIORITY_VERYLOW:
                            ioPriority = IoPriorityVeryLow;
                            break;
                        case ID_IOPRIORITY_LOW:
                            ioPriority = IoPriorityLow;
                            break;
                        case ID_IOPRIORITY_NORMAL:
                            ioPriority = IoPriorityNormal;
                            break;
                        case ID_IOPRIORITY_HIGH:
                            ioPriority = IoPriorityHigh;
                            break;
                        }

                        if (ioPriority != ULONG_MAX)
                        {
                            PhReferenceObject(threadItem);
                            PhUiSetIoPriorityThread(hwndDlg, threadItem, ioPriority);
                            PhDereferenceObject(threadItem);
                        }
                    }
                }
                break;
            case ID_PAGEPRIORITY_VERYLOW:
            case ID_PAGEPRIORITY_LOW:
            case ID_PAGEPRIORITY_MEDIUM:
            case ID_PAGEPRIORITY_BELOWNORMAL:
            case ID_PAGEPRIORITY_NORMAL:
                {
                    PPH_THREAD_ITEM threadItem = PhGetSelectedThreadItem(&threadsContext->ListContext);

                    if (threadItem)
                    {
                        ULONG pagePriority = ULONG_MAX;

                        switch (GET_WM_COMMAND_ID(wParam, lParam))
                        {
                        case ID_PAGEPRIORITY_VERYLOW:
                            pagePriority = MEMORY_PRIORITY_VERY_LOW;
                            break;
                        case ID_PAGEPRIORITY_LOW:
                            pagePriority = MEMORY_PRIORITY_LOW;
                            break;
                        case ID_PAGEPRIORITY_MEDIUM:
                            pagePriority = MEMORY_PRIORITY_MEDIUM;
                            break;
                        case ID_PAGEPRIORITY_BELOWNORMAL:
                            pagePriority = MEMORY_PRIORITY_BELOW_NORMAL;
                            break;
                        case ID_PAGEPRIORITY_NORMAL:
                            pagePriority = MEMORY_PRIORITY_NORMAL;
                            break;
                        }

                        if (pagePriority != ULONG_MAX)
                        {
                            PhReferenceObject(threadItem);
                            PhUiSetPagePriorityThread(hwndDlg, threadItem, pagePriority);
                            PhDereferenceObject(threadItem);
                        }
                    }
                }
                break;
            case ID_THREAD_COPY:
                {
                    PPH_STRING text;

                    text = PhGetTreeNewText(threadsContext->TreeNewHandle, 0);
                    PhSetClipboardString(threadsContext->TreeNewHandle, &text->sr);
                    PhDereferenceObject(text);
                }
                break;
            //case IDC_OPENSTARTMODULE:
            //    {
            //        PPH_THREAD_ITEM threadItem = PhGetSelectedThreadItem(&threadsContext->ListContext);
            //
            //        if (threadItem && threadItem->StartAddressFileName)
            //        {
            //            PhShellExecuteUserString(
            //                hwndDlg,
            //                L"FileBrowseExecutable",
            //                threadItem->StartAddressFileName->Buffer,
            //                FALSE,
            //                L"Make sure the Explorer executable file is present."
            //                );
            //        }
            //    }
            //    break;
            case IDC_OPTIONS:
                {
                    RECT rect;
                    PPH_EMENU menu;
                    PPH_EMENU_ITEM hideSuspendedMenuItem;
                    PPH_EMENU_ITEM hideGuiMenuItem;
                    PPH_EMENU_ITEM highlightSuspendedMenuItem;
                    PPH_EMENU_ITEM highlightGuiMenuItem;
                    PPH_EMENU_ITEM saveMenuItem;
                    PPH_EMENU_ITEM selectedItem;

                    if (!GetWindowRect(GetDlgItem(hwndDlg, IDC_OPTIONS), &rect))
                        break;

                    hideSuspendedMenuItem = PhCreateEMenuItem(0, PH_THREAD_TREELIST_MENUITEM_HIDE_SUSPENDED, L"Hide suspended", NULL, NULL);
                    hideGuiMenuItem = PhCreateEMenuItem(0, PH_THREAD_TREELIST_MENUITEM_HIDE_GUITHREADS, L"Hide gui", NULL, NULL);
                    highlightSuspendedMenuItem = PhCreateEMenuItem(0, PH_THREAD_TREELIST_MENUITEM_HIGHLIGHT_SUSPENDED, L"Highlight suspended", NULL, NULL);
                    highlightGuiMenuItem = PhCreateEMenuItem(0, PH_THREAD_TREELIST_MENUITEM_HIGHLIGHT_GUITHREADS, L"Highlight gui", NULL, NULL);
                    saveMenuItem = PhCreateEMenuItem(0, PH_THREAD_TREELIST_MENUITEM_SAVE, L"Save...", NULL, NULL);

                    menu = PhCreateEMenu();
                    PhInsertEMenuItem(menu, hideSuspendedMenuItem, ULONG_MAX);
                    PhInsertEMenuItem(menu, hideGuiMenuItem, ULONG_MAX);
                    PhInsertEMenuItem(menu, PhCreateEMenuSeparator(), ULONG_MAX);
                    PhInsertEMenuItem(menu, highlightSuspendedMenuItem, ULONG_MAX);
                    PhInsertEMenuItem(menu, highlightGuiMenuItem, ULONG_MAX);
                    PhInsertEMenuItem(menu, PhCreateEMenuSeparator(), ULONG_MAX);
                    PhInsertEMenuItem(menu, saveMenuItem, ULONG_MAX);

                    if (threadsContext->ListContext.HideSuspended)
                        hideSuspendedMenuItem->Flags |= PH_EMENU_CHECKED;
                    if (threadsContext->ListContext.HideGuiThreads)
                        hideGuiMenuItem->Flags |= PH_EMENU_CHECKED;
                    if (threadsContext->ListContext.HighlightSuspended)
                        highlightSuspendedMenuItem->Flags |= PH_EMENU_CHECKED;
                    if (threadsContext->ListContext.HighlightGuiThreads)
                        highlightGuiMenuItem->Flags |= PH_EMENU_CHECKED;

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
                        if (selectedItem->Id == PH_THREAD_TREELIST_MENUITEM_SAVE)
                        {
                            PhpProcessThreadsSave(threadsContext);
                        }
                        else
                        {
                            PhSetOptionsThreadList(&threadsContext->ListContext, selectedItem->Id);
                            PhSaveSettingsThreadList(&threadsContext->ListContext);
                            PhApplyTreeNewFilters(&threadsContext->ListContext.TreeFilterSupport);
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
            case PSN_SETACTIVE:
                break;
            case PSN_KILLACTIVE:
                // Can't disable, it screws up the deltas.
                break;
            case PSN_QUERYINITIALFOCUS:
                SetWindowLongPtr(hwndDlg, DWLP_MSGRESULT, (LPARAM)threadsContext->TreeNewHandle);
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
                    SetFocus(threadsContext->SearchboxHandle);
                    return TRUE;
                }
            }
        }
        break;
    case WM_PH_THREADS_UPDATED:
        {
            ULONG upToRunId = (ULONG)wParam;
            BOOLEAN firstRun = !!lParam;
            PPH_PROVIDER_EVENT events;
            ULONG count;
            ULONG i;

            events = PhFlushProviderEventQueue(&threadsContext->EventQueue, upToRunId, &count);

            if (events)
            {
                TreeNew_SetRedraw(threadsContext->TreeNewHandle, FALSE);

                for (i = 0; i < count; i++)
                {
                    PH_PROVIDER_EVENT_TYPE type = PH_PROVIDER_EVENT_TYPE(events[i]);
                    PPH_THREAD_ITEM threadItem = PH_PROVIDER_EVENT_OBJECT(events[i]);

                    switch (type)
                    {
                    case ProviderAddedEvent:
                        PhAddThreadNode(&threadsContext->ListContext, threadItem, firstRun);
                        PhDereferenceObject(threadItem);
                        break;
                    case ProviderModifiedEvent:
                        PhUpdateThreadNode(&threadsContext->ListContext, PhFindThreadNode(&threadsContext->ListContext, threadItem->ThreadId));
                        break;
                    case ProviderRemovedEvent:
                        PhRemoveThreadNode(&threadsContext->ListContext, PhFindThreadNode(&threadsContext->ListContext, threadItem->ThreadId));
                        break;
                    }
                }

                PhFree(events);
            }

            PhTickThreadNodes(&threadsContext->ListContext);

            // Refresh the visible nodes.
            PhApplyTreeNewFilters(&threadsContext->ListContext.TreeFilterSupport);

            if (count != 0)
                TreeNew_SetRedraw(threadsContext->TreeNewHandle, TRUE);

            if (propPageContext->PropContext->SelectThreadId)
            {
                PPH_THREAD_NODE threadNode;

                if (threadNode = PhFindThreadNode(&threadsContext->ListContext, propPageContext->PropContext->SelectThreadId))
                {
                    if (threadNode->Node.Visible)
                    {
                        TreeNew_SetFocusNode(threadsContext->TreeNewHandle, &threadNode->Node);
                        TreeNew_SetMarkNode(threadsContext->TreeNewHandle, &threadNode->Node);
                        TreeNew_SelectRange(threadsContext->TreeNewHandle, threadNode->Node.Index, threadNode->Node.Index);
                        TreeNew_EnsureVisible(threadsContext->TreeNewHandle, &threadNode->Node);
                    }
                }

                propPageContext->PropContext->SelectThreadId = NULL;
            }
        }
        break;
    }

    return FALSE;
}
