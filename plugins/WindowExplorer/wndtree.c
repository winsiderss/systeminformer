/*
 * Copyright (c) 2022 Winsider Seminars & Solutions, Inc.  All rights reserved.
 *
 * This file is part of System Informer.
 *
 * Authors:
 *
 *     wj32    2011
 *     dmex    2017-2023
 *
 */

#include "wndexp.h"

_Function_class_(PH_HASHTABLE_EQUAL_FUNCTION)
BOOLEAN WepWindowNodeHashtableEqualFunction(
    _In_ PVOID Entry1,
    _In_ PVOID Entry2
    );

_Function_class_(PH_HASHTABLE_HASH_FUNCTION)
ULONG WepWindowNodeHashtableHashFunction(
    _In_ PVOID Entry
    );

VOID WepDestroyWindowNode(
    _In_ PWE_WINDOW_NODE WindowNode
    );

BOOLEAN NTAPI WepWindowTreeNewCallback(
    _In_ HWND WindowHandle,
    _In_ PH_TREENEW_MESSAGE Message,
    _In_ PVOID Parameter1,
    _In_ PVOID Parameter2,
    _In_ PVOID Context
    );

/**
 * Callback for filtering window tree nodes.
 *
 * \param[in] Node The tree node to filter.
 * \param[in] Context The window tree context.
 * \return TRUE if the node should be visible, FALSE otherwise.
 */
_Function_class_(PH_TN_FILTER_FUNCTION)
BOOLEAN WeWindowTreeFilterCallback(
    _In_ PPH_TREENEW_NODE Node,
    _In_ PVOID Context
    )
{
    PWE_WINDOW_NODE windowNode = (PWE_WINDOW_NODE)Node;
    PWE_WINDOW_TREE_CONTEXT context = Context;

    if (!context->SearchMatchHandle)
        return TRUE;

    if (windowNode->WindowHandle)
    {
        if (PhSearchControlMatchPointer(context->SearchMatchHandle, windowNode->WindowHandle))
            return TRUE;
    }

    if (windowNode->WindowItem->WindowClassText)
    {
        if (PhSearchControlMatch(context->SearchMatchHandle, &windowNode->WindowItem->WindowClassText->sr))
            return TRUE;
    }

    if (windowNode->WindowHandleString[0])
    {
        if (PhSearchControlMatchLongHintZ(context->SearchMatchHandle, windowNode->WindowHandleString))
            return TRUE;
    }

    if (!PhIsNullOrEmptyString(windowNode->WindowItem->WindowText))
    {
        if (PhSearchControlMatch(context->SearchMatchHandle, &windowNode->WindowItem->WindowText->sr))
            return TRUE;
    }

    if (!PhIsNullOrEmptyString(windowNode->ThreadString))
    {
        if (PhSearchControlMatch(context->SearchMatchHandle, &windowNode->ThreadString->sr))
            return TRUE;
    }

    if (!PhIsNullOrEmptyString(windowNode->ModuleString))
    {
        if (PhSearchControlMatch(context->SearchMatchHandle, &windowNode->ModuleString->sr))
            return TRUE;
    }

    return FALSE;
}

/**
 * Initializes a window tree.
 *
 * \param[in] ParentWindowHandle The parent window handle.
 * \param[in] TreeNewHandle The tree-new handle.
 * \param[out] Context The window tree context.
 */
VOID WeInitializeWindowTree(
    _In_ HWND ParentWindowHandle,
    _In_ HWND TreeNewHandle,
    _Out_ PWE_WINDOW_TREE_CONTEXT Context
    )
{
    PPH_STRING settings;

    memset(Context, 0, sizeof(WE_WINDOW_TREE_CONTEXT));

    Context->NodeHashtable = PhCreateHashtable(
        sizeof(PWE_WINDOW_NODE),
        WepWindowNodeHashtableEqualFunction,
        WepWindowNodeHashtableHashFunction,
        100
        );
    Context->NodeList = PhCreateList(100);
    Context->NodeRootList = PhCreateList(30);
    Context->EnableStateHighlighting = TRUE;

    Context->ParentWindowHandle = ParentWindowHandle;
    Context->TreeNewHandle = TreeNewHandle;
    PhSetControlTheme(TreeNewHandle, L"explorer");

    TreeNew_SetRedraw(TreeNewHandle, FALSE);
    TreeNew_SetCallback(TreeNewHandle, WepWindowTreeNewCallback, Context);

    PhAddTreeNewColumn(TreeNewHandle, WEWNTLC_CLASS, TRUE, L"Class", 180, PH_ALIGN_LEFT, 0, 0);
    PhAddTreeNewColumn(TreeNewHandle, WEWNTLC_HANDLE, TRUE, L"Handle", 70, PH_ALIGN_LEFT, 1, 0);
    PhAddTreeNewColumn(TreeNewHandle, WEWNTLC_TEXT, TRUE, L"Text", 220, PH_ALIGN_LEFT, 2, 0);
    PhAddTreeNewColumn(TreeNewHandle, WEWNTLC_PROCESS, TRUE, L"Process", 150, PH_ALIGN_LEFT, 3, 0);
    PhAddTreeNewColumn(TreeNewHandle, WEWNTLC_THREAD, TRUE, L"Thread", 150, PH_ALIGN_LEFT, 4, 0);
    PhAddTreeNewColumn(TreeNewHandle, WEWNTLC_MODULE, TRUE, L"Module", 150, PH_ALIGN_LEFT, 5, 0);

    PhInitializeTreeNewFilterSupport(&Context->FilterSupport, Context->TreeNewHandle, Context->NodeList);
    Context->TreeFilterEntry = PhAddTreeNewFilter(&Context->FilterSupport, WeWindowTreeFilterCallback, Context);

    TreeNew_SetTriState(TreeNewHandle, TRUE);
    TreeNew_SetRedraw(TreeNewHandle, TRUE);

    settings = PhGetStringSetting(SETTING_NAME_WINDOW_TREE_LIST_COLUMNS);
    PhCmLoadSettings(TreeNewHandle, &settings->sr);
    PhDereferenceObject(settings);
}

/**
 * Deletes a window tree.
 *
 * \param[in] Context The window tree context.
 */
VOID WeDeleteWindowTree(
    _In_ PWE_WINDOW_TREE_CONTEXT Context
    )
{
    PPH_STRING settings;
    ULONG i;

    PhRemoveTreeNewFilter(&Context->FilterSupport, Context->TreeFilterEntry);
    PhDeleteTreeNewFilterSupport(&Context->FilterSupport);

    settings = PhCmSaveSettings(Context->TreeNewHandle);
    PhSetStringSetting2(SETTING_NAME_WINDOW_TREE_LIST_COLUMNS, &settings->sr);
    PhDereferenceObject(settings);

    if (Context->NodeList)
    {
        for (i = 0; i < Context->NodeList->Count; i++)
            WepDestroyWindowNode(Context->NodeList->Items[i]);

        PhDereferenceObject(Context->NodeList);
    }

    if (Context->NodeImageList)
        PhImageListDestroy(Context->NodeImageList);

    if (Context->NodeStateList)
        PhDereferenceObject(Context->NodeStateList);

    PhDereferenceObject(Context->NodeHashtable);
    PhDereferenceObject(Context->NodeRootList);
}

/**
 * Initializes the image list for a window tree.
 *
 * \param[in,out] Context The window tree context.
 * \param[in] Selector The window selector.
 */
VOID WeInitializeWindowTreeImageList(
    _In_ PWE_WINDOW_TREE_CONTEXT Context,
    _In_ PWE_WINDOW_SELECTOR Selector
    )
{
    Context->EnableIcons = !!PhGetIntegerSetting(SETTING_NAME_WINDOW_ENABLE_ICONS);
    Context->EnableIconsInternal = !!PhGetIntegerSetting(SETTING_NAME_WINDOW_ENABLE_ICONS_INTERNAL);
    Context->EnableMessageOnly = !!PhGetIntegerSetting(SETTING_NAME_WINDOW_ENUM_MESSAGEONLY);
    Context->EnableWindowVisible = !!PhGetIntegerSetting(SETTING_NAME_WINDOW_ENUM_NONVISIBLE);
    Context->HighlightMessageOnly = !!PhGetIntegerSetting(SETTING_NAME_WINDOW_HIGHLIGHT_MESSAGEONLY);
    Context->MessageOnlyWindowColor = PhGetIntegerSetting(SETTING_NAME_WINDOW_HIGHLIGHT_MESSAGEONLY_COLOR);
    Context->SelectorType = Selector->Type;

    if (Context->EnableIcons)
    {
        if (Context->EnableIconsInternal)
        {
            HICON iconSmall;
            LONG dpiValue;

            dpiValue = PhGetWindowDpi(Context->ParentWindowHandle);
            Context->NodeImageList = PhImageListCreate(
                PhGetSystemMetrics(SM_CXSMICON, dpiValue),
                PhGetSystemMetrics(SM_CYSMICON, dpiValue),
                ILC_MASK | ILC_COLOR32,
                100,
                1
                );

            if (Context->NodeImageList)
            {
                PhImageListSetBkColor(Context->NodeImageList, CLR_NONE);
                TreeNew_SetImageList(Context->TreeNewHandle, Context->NodeImageList);

                PhGetStockApplicationIcon(&iconSmall, NULL, dpiValue);
                PhImageListAddIcon(Context->NodeImageList, iconSmall);
            }
        }
        else
        {
            TreeNew_SetImageList(Context->TreeNewHandle, PhGetProcessSmallImageList());
        }
    }
}

/**
 * Updates the window tree image list on DPI change.
 *
 * \param[in,out] Context The window tree context.
 * \param[in] Selector The window selector.
 */
VOID WeDpiChangeUpdateWindowTree(
    _In_ PWE_WINDOW_TREE_CONTEXT Context,
    _In_ PWE_WINDOW_SELECTOR Selector
    )
{
    if (Context->EnableIconsInternal)
    {
        LONG dpiValue;

        dpiValue = PhGetWindowDpi(Context->ParentWindowHandle);
        //Context->NodeImageList = PhImageListCreate(
        //    PhGetSystemMetrics(SM_CXSMICON, dpiValue),
        //    PhGetSystemMetrics(SM_CYSMICON, dpiValue),
        //    ILC_MASK | ILC_COLOR32,
        //    100,
        //    1
        //);

        if (Context->NodeImageList)
        {
            PhImageListSetIconSize(Context->NodeImageList, PhGetSystemMetrics(SM_CXSMICON, dpiValue), PhGetSystemMetrics(SM_CYSMICON, dpiValue));

            //PhGetStockApplicationIcon(&iconSmall, NULL);
            //PhImageListAddIcon(Context->NodeImageList, iconSmall);
        }
    }
    else
    {
        //TreeNew_SetImageList(Context->TreeNewHandle, PhGetProcessSmallImageList());
    }
}

/**
 * Hashtable equal function for window nodes.
 *
 * \param[in] Entry1 The first entry.
 * \param[in] Entry2 The second entry.
 * \return TRUE if equal, FALSE otherwise.
 */
_Use_decl_annotations_
BOOLEAN WepWindowNodeHashtableEqualFunction(
    _In_ PVOID Entry1,
    _In_ PVOID Entry2
    )
{
    PWE_WINDOW_NODE windowNode1 = *(PWE_WINDOW_NODE *)Entry1;
    PWE_WINDOW_NODE windowNode2 = *(PWE_WINDOW_NODE *)Entry2;

    return windowNode1->WindowHandle == windowNode2->WindowHandle;
}

/**
 * Hashtable hash function for window nodes.
 *
 * \param[in] Entry The entry to hash.
 * \return The hash value.
 */
_Use_decl_annotations_
ULONG WepWindowNodeHashtableHashFunction(
    _In_ PVOID Entry
    )
{
    return PhHashIntPtr((ULONG_PTR)(*(PWE_WINDOW_NODE *)Entry)->WindowHandle);
}

/**
 * Gets the client ID name for a process or thread.
 *
 * \param[in] ClientId The client ID.
 * \return The client ID name.
 */
PPH_STRING WeGetClientIdName(
    _In_ PCLIENT_ID ClientId
    )
{
    PPH_STRING result;
    PPH_STRING processName = NULL;
    PPH_STRING threadName = NULL;
    PH_STRINGREF processNameStringRef;
    PH_STRINGREF threadNameStringRef;
    BOOLEAN processIsTerminated = FALSE;
    BOOLEAN threadIsTerminated = FALSE;

    PhInitializeStringRef(&processNameStringRef, L"terminated process");
    PhInitializeStringRef(&threadNameStringRef, L"terminated thread");

    if (ClientId->UniqueProcess)
    {
        HANDLE processHandle;

        if (ClientId->UniqueProcess == SYSTEM_PROCESS_ID)
        {
            if (processName = PhGetKernelFileName())
            {
                PhMoveReference(&processName, PhGetBaseName(processName));
                processNameStringRef.Length = processName->Length;
                processNameStringRef.Buffer = processName->Buffer;
            }
        }
        else
        {
            // Determine the name of the process ourselves (diversenok)
            if (ClientId->UniqueProcess && NT_SUCCESS(PhGetProcessImageFileNameById(
                ClientId->UniqueProcess,
                NULL,
                &processName
                )))
            {
                processNameStringRef.Length = processName->Length;
                processNameStringRef.Buffer = processName->Buffer;
            }
        }

        if (NT_SUCCESS(PhOpenProcessClientId(
            &processHandle,
            SYNCHRONIZE, // PROCESS_QUERY_LIMITED_INFORMATION
            ClientId
            )))
        {
            // Check if the process is alive
            PhGetProcessIsTerminated(processHandle, &processIsTerminated);

            // Use the name of the process if available
            //if (PhIsNullOrEmptyString(ProcessName) && NT_SUCCESS(PhGetProcessImageFileName(processHandle, &processName)))
            //{
            //    processNameStringRef.Length = processName->Length;
            //    processNameStringRef.Buffer = processName->Buffer;
            //}

            NtClose(processHandle);
        }
    }

    if (ClientId->UniqueThread)
    {
        HANDLE threadHandle;

        if (NT_SUCCESS(PhOpenThreadClientId(
            &threadHandle,
            THREAD_QUERY_LIMITED_INFORMATION,
            ClientId
            )))
        {
            // Check if the thread is alive
            PhGetThreadIsTerminated(threadHandle, &threadIsTerminated);

            // Use the name of the thread if available
            if (NT_SUCCESS(PhGetThreadName(threadHandle, &threadName)))
            {
                threadNameStringRef.Length = threadName->Length;
                threadNameStringRef.Buffer = threadName->Buffer;
            }

            NtClose(threadHandle);
        }
    }

    // Combine everything

    if (ClientId->UniqueProcess && ClientId->UniqueThread)
    {
        PH_FORMAT format[10];

        // L"%s%.*s (%lu): %s%.*s (%lu)"
        PhInitFormatS(&format[0], processIsTerminated ? L"Terminated " : L"");
        PhInitFormatSR(&format[1], processNameStringRef);
        PhInitFormatS(&format[2], L" (");
        PhInitFormatU(&format[3], HandleToUlong(ClientId->UniqueProcess));
        PhInitFormatS(&format[4], L"): ");
        PhInitFormatS(&format[5], threadIsTerminated ? L"Terminated " : L"");
        PhInitFormatSR(&format[6], threadNameStringRef);
        PhInitFormatS(&format[7], L" (");
        PhInitFormatU(&format[8], HandleToUlong(ClientId->UniqueThread));
        PhInitFormatC(&format[9], L')');

        result = PhFormat(format, RTL_NUMBER_OF(format), 0);
    }
    else if (ClientId->UniqueThread)
    {
        PH_FORMAT format[5];

        // %s%.*s (%lu)"
        PhInitFormatS(&format[0], threadIsTerminated ? L"Terminated " : L"");
        PhInitFormatSR(&format[1], threadNameStringRef);
        PhInitFormatS(&format[2], L" (");
        PhInitFormatU(&format[3], HandleToUlong(ClientId->UniqueThread));
        PhInitFormatC(&format[4], L')');

        result = PhFormat(format, RTL_NUMBER_OF(format), 0);
    }
    else
    {
        PH_FORMAT format[5];

        // L"%s%.*s (%lu)"
        PhInitFormatS(&format[0], processIsTerminated ? L"Terminated " : L"");
        PhInitFormatSR(&format[1], processNameStringRef);
        PhInitFormatS(&format[2], L" (");
        PhInitFormatU(&format[3], HandleToUlong(ClientId->UniqueProcess));
        PhInitFormatC(&format[4], L')');

        result = PhFormat(format, RTL_NUMBER_OF(format), 0);
    }

    //result = PhFormatString(
    //    ClientId->UniqueThread ? L"%s%.*s (%lu): %s%.*s (%lu)" : L"%s%.*s (%lu)",
    //    processIsTerminated ? L"Terminated " : L"",
    //    processNameStringRef.Length / sizeof(WCHAR),
    //    processNameStringRef.Buffer,
    //    HandleToUlong(ClientId->UniqueProcess),
    //    threadIsTerminated ? L"terminated " : L"",
    //    threadNameStringRef.Length / sizeof(WCHAR),
    //    threadNameStringRef.Buffer,
    //    HandleToUlong(ClientId->UniqueThread)
    //    );

    PhClearReference(&processName);
    PhClearReference(&threadName);

    return result;
}

BOOLEAN NeedsNodesStructured = FALSE;

/**
 * Adds a window node to the window tree.
 *
 * \param[in,out] Context The window tree context.
 * \param[in] WindowItem The window item.
 * \param[in] RunId The run ID.
 * \return The added window node.
 */
PWE_WINDOW_NODE WeAddWindowNode(
    _Inout_ PWE_WINDOW_TREE_CONTEXT Context,
    _In_ PWE_WINDOW_ITEM WindowItem,
    _In_ ULONG RunId
    )
{
    PWE_WINDOW_NODE windowNode;
    PWE_WINDOW_NODE parentNode = NULL;


    windowNode = PhAllocateZero(sizeof(WE_WINDOW_NODE));
    PhInitializeTreeNewNode(&windowNode->Node);
    windowNode->WindowItem = PhReferenceObject(WindowItem);
    windowNode->Children = PhCreateList(1);

    if (Context->EnableStateHighlighting && RunId != 0)
    {
        PhChangeShStateTn(
            &windowNode->Node,
            &windowNode->ShState,
            &Context->NodeStateList,
            NewItemState,
            Context->ColorNew,
            NULL
            );
    }

    memset(windowNode->TextCache, 0, sizeof(PH_STRINGREF) * WEWNTLC_MAXIMUM);
    windowNode->Node.TextCache = windowNode->TextCache;
    windowNode->Node.TextCacheSize = WEWNTLC_MAXIMUM;

    // Populate window information
    windowNode->WindowHandle = WindowItem->WindowHandle;
    PhPrintPointer(windowNode->WindowHandleString, WindowItem->WindowHandle);

    if (WindowItem->ParentHandle)
    {
        parentNode = WeFindWindowNode(Context, WindowItem->ParentHandle);
    }

    if (parentNode)
    {
        windowNode->Parent = parentNode;
        if (Context->ViewMode == WeWindowViewModeZOrder)
            WepInsertWindowNodeByZOrder(Context, parentNode, windowNode);
        else
            PhAddItemList(parentNode->Children, windowNode);
        parentNode->HasChildren = TRUE;
        //windowNode->HasChildren = !!FindWindowEx(WindowItem->WindowHandle, NULL, NULL, NULL);
    }
    else
    {
        if (Context->ViewMode == WeWindowViewModeZOrder)
            WepInsertWindowNodeByZOrder(Context, NULL, windowNode);
        else
            PhAddItemList(Context->NodeRootList, windowNode);
    }

    WepFillWindowInfo(Context, windowNode);

    //if (windowNode->ClientId.UniqueProcess)
    //{
    //    CLIENT_ID clientId;
    //
    //    clientId.UniqueProcess = windowNode->ClientId.UniqueProcess;
    //    clientId.UniqueThread = NULL;
    //
    //    windowNode->ProcessString = WeGetClientIdName(&clientId);
    //}
    //
    //if (windowNode->ClientId.UniqueThread)
    //{
    //    CLIENT_ID clientId;
    //
    //    clientId.UniqueProcess = NULL;
    //    clientId.UniqueThread = windowNode->ClientId.UniqueThread;
    //
    //    windowNode->ThreadString = WeGetClientIdName(&clientId);
    //}

    //windowNode->WindowIndex = WindowItem->WindowIndex;
    //PhPrintUInt32(windowNode->WindowIndexString, windowNode->WindowIndex);

    //windowNode->WindowGeneration = WindowItem->WindowGeneration;
    //PhPrintUInt32(windowNode->WindowGenerationString, windowNode->WindowGeneration);

    // Get process item and module information
    /*if (windowNode->ClientId.UniqueProcess)
    {
        PPH_PROCESS_ITEM processItem;

        if (processItem = PhReferenceProcessItem(windowNode->ClientId.UniqueProcess))
        {
            if (!windowNode->ProcessItem)
                windowNode->ProcessItem = PhReferenceObject(processItem);

            if (!PhIsNullOrEmptyString(processItem->FileName))
            {
                windowNode->ModuleString = PhReferenceObject(processItem->FileName);
            }

            PhDereferenceObject(processItem);
        }
    }*/

    if (Context->EnableIcons && Context->EnableIconsInternal)
    {
        if (Context->NodeImageList)
        {
            HICON windowIcon;

            if (windowIcon = WeGetInternalWindowIcon(WindowItem->WindowHandle, ICON_SMALL))
            {
                windowNode->WindowIconIndex = PhImageListAddIcon(Context->NodeImageList, windowIcon);
                DestroyIcon(windowIcon);
            }
        }
    }

    PhAddEntryHashtable(Context->NodeHashtable, &windowNode);
    PhAddItemList(Context->NodeList, windowNode);

    if (Context->FilterSupport.FilterList)
    {
        windowNode->Node.Visible = PhApplyTreeNewFiltersToNode(&Context->FilterSupport, &windowNode->Node);
    }

    //TreeNew_NodesStructured(Context->TreeNewHandle);
    NeedsNodesStructured = TRUE;

    return windowNode;
}

/**
 * Finds a window node by window handle.
 *
 * \param[in] Context The window tree context.
 * \param[in] WindowHandle The window handle.
 * \return The window node, or NULL if not found.
 */
PWE_WINDOW_NODE WeFindWindowNode(
    _In_ PWE_WINDOW_TREE_CONTEXT Context,
    _In_ HWND WindowHandle
    )
{
    WE_WINDOW_NODE lookupWindowNode;
    PWE_WINDOW_NODE lookupWindowNodePtr = &lookupWindowNode;
    PWE_WINDOW_NODE *windowNode;

    lookupWindowNode.WindowHandle = WindowHandle;

    windowNode = (PWE_WINDOW_NODE *)PhFindEntryHashtable(
        Context->NodeHashtable,
        &lookupWindowNodePtr
        );

    if (windowNode)
        return *windowNode;
    else
        return NULL;
}

/**
 * Updates a window node.
 *
 * \param[in] WindowNode The window node.
 * \param[in] Context The window tree context.
 */
VOID WeUpdateWindowNode(
    _In_ PWE_WINDOW_NODE WindowNode,
    _In_ PWE_WINDOW_TREE_CONTEXT Context
    )
{
    memset(WindowNode->TextCache, 0, sizeof(PH_STRINGREF) * WEWNTLC_MAXIMUM);

    PhInvalidateTreeNewNode(&WindowNode->Node, TN_CACHE_COLOR);
    TreeNew_InvalidateNode(Context->TreeNewHandle, &WindowNode->Node);
    //TreeNew_NodesStructured(Context->TreeNewHandle);
    //Context->NodesNeedRestructure = TRUE;
}

VOID WeInvalidateWindowTreeColors(
    _In_ PWE_WINDOW_TREE_CONTEXT Context
    )
{
    ULONG i;

    for (i = 0; i < Context->NodeList->Count; i++)
    {
        PWE_WINDOW_NODE windowNode = Context->NodeList->Items[i];

        PhInvalidateTreeNewNode(&windowNode->Node, TN_CACHE_COLOR);
    }

    InvalidateRect(Context->TreeNewHandle, NULL, FALSE);
}

VOID WeSetWindowTreeIconsEnabled(
    _In_ PWE_WINDOW_TREE_CONTEXT Context,
    _In_ BOOLEAN EnableIcons
    )
{
    ULONG i;

    Context->EnableIcons = EnableIcons;

    if (!EnableIcons)
    {
        TreeNew_SetImageList(Context->TreeNewHandle, NULL);
    }
    else if (Context->EnableIconsInternal)
    {
        if (!Context->NodeImageList)
        {
            HICON iconSmall;
            LONG dpiValue = PhGetWindowDpi(Context->ParentWindowHandle);

            Context->NodeImageList = PhImageListCreate(
                PhGetSystemMetrics(SM_CXSMICON, dpiValue),
                PhGetSystemMetrics(SM_CYSMICON, dpiValue),
                ILC_MASK | ILC_COLOR32,
                100,
                1
                );

            if (Context->NodeImageList)
            {
                PhImageListSetBkColor(Context->NodeImageList, CLR_NONE);
                PhGetStockApplicationIcon(&iconSmall, NULL, dpiValue);
                PhImageListAddIcon(Context->NodeImageList, iconSmall);

                for (i = 0; i < Context->NodeList->Count; i++)
                {
                    PWE_WINDOW_NODE windowNode = Context->NodeList->Items[i];
                    HICON windowIcon;

                    if (windowIcon = WeGetInternalWindowIcon(windowNode->WindowHandle, ICON_SMALL))
                    {
                        windowNode->WindowIconIndex = PhImageListAddIcon(Context->NodeImageList, windowIcon);
                        DestroyIcon(windowIcon);
                    }
                }
            }
        }

        TreeNew_SetImageList(Context->TreeNewHandle, Context->NodeImageList);
    }
    else
    {
        TreeNew_SetImageList(Context->TreeNewHandle, PhGetProcessSmallImageList());

        for (i = 0; i < Context->NodeList->Count; i++)
        {
            PWE_WINDOW_NODE windowNode = Context->NodeList->Items[i];

            if (windowNode->ProcessItem && PhTestEvent(&windowNode->ProcessItem->Stage1Event))
            {
                windowNode->WindowIconIndex = windowNode->ProcessItem->SmallIconIndex;
                windowNode->ProcessIconValid = TRUE;
            }
        }
    }

    TreeNew_NodesStructured(Context->TreeNewHandle);
    InvalidateRect(Context->TreeNewHandle, NULL, FALSE);
}

/**
 * Internal function to remove a window node.
 *
 * \param[in] WindowNode The window node.
 * \param[in,opt] Context The window tree context.
 */
VOID WepRemoveWindowNode(
    _In_ PWE_WINDOW_NODE WindowNode,
    _In_opt_ PWE_WINDOW_TREE_CONTEXT Context
    )
{
    ULONG index;
    ULONG i;

    // Remove from parent/children hashtable/list and cleanup.

    if (WindowNode->Parent)
    {
        if ((index = PhFindItemList(WindowNode->Parent->Children, WindowNode)) != ULONG_MAX)
        {
            PhRemoveItemList(WindowNode->Parent->Children, index);
        }
    }
    else
    {
        if ((index = PhFindItemList(Context->NodeRootList, WindowNode)) != ULONG_MAX)
        {
            PhRemoveItemList(Context->NodeRootList, index);
        }
    }

    // Move the node's children to the root list.
    for (i = 0; i < WindowNode->Children->Count; i++)
    {
        PWE_WINDOW_NODE node = WindowNode->Children->Items[i];

        node->Parent = NULL;
        PhAddItemList(Context->NodeRootList, node);
    }

    // Remove from hashtable/list and cleanup.

    if ((index = PhFindItemList(Context->NodeList, WindowNode)) != ULONG_MAX)
    {
        PhRemoveItemList(Context->NodeList, index);
    }

    WepDestroyWindowNode(WindowNode);

    //TreeNew_NodesStructured(Context->TreeNewHandle);
    NeedsNodesStructured = TRUE;
}

/**
 * Removes a window node from the tree.
 *
 * \param[in] WindowNode The window node.
 * \param[in] Context The window tree context.
 */
VOID WeRemoveWindowNode(
    _In_ PWE_WINDOW_NODE WindowNode,
    _In_ PWE_WINDOW_TREE_CONTEXT Context
    )
{
    // Remove from the hashtable here to avoid problems in case the key is re-used.
    PhRemoveEntryHashtable(Context->NodeHashtable, &WindowNode);

    if (Context->EnableStateHighlighting)
    {
        PhChangeShStateTn(
            &WindowNode->Node,
            &WindowNode->ShState,
            &Context->NodeStateList,
            RemovingItemState,
            Context->ColorRemoved,
            Context->TreeNewHandle
            );
    }
    else
    {
        WepRemoveWindowNode(WindowNode, Context);
    }
}

/**
 * Ticks the window nodes for updates and state changes.
 *
 * \param[in] Context The windows context.
 * \param[in] TreeContext The window tree context.
 */
VOID WeTickWindowNodes(
    _In_ PWINDOWS_CONTEXT Context,
    _In_ PWE_WINDOW_TREE_CONTEXT TreeContext
    )
{
    BOOLEAN fullyInvalidated = FALSE;
    BOOLEAN needsInvalidate = FALSE;
    ULONG i;

    if (TreeContext->TreeNewSortOrder != NoSortOrder)
    {
        //TreeNew_NodesStructured(TreeContext->TreeNewHandle);
        NeedsNodesStructured = TRUE;
        fullyInvalidated = TRUE;
    }

    PH_TICK_SH_STATE_TN(
        WE_WINDOW_NODE,
        ShState,
        TreeContext->NodeStateList,
        WepRemoveWindowNode,
        Context->HighlightingDuration,
        TreeContext->TreeNewHandle,
        TRUE,
        &fullyInvalidated,
        TreeContext
        );

    if (TreeContext->EnableIcons && !TreeContext->EnableIconsInternal && TreeContext->NodeList)
    {
        for (i = 0; i < TreeContext->NodeList->Count; i++)
        {
            PWE_WINDOW_NODE windowNode = TreeContext->NodeList->Items[i];

            if (windowNode->ProcessItem && !windowNode->ProcessIconValid && PhTestEvent(&windowNode->ProcessItem->Stage1Event))
            {
                if (windowNode->WindowIconIndex != windowNode->ProcessItem->SmallIconIndex)
                {
                    windowNode->WindowIconIndex = windowNode->ProcessItem->SmallIconIndex;
                    TreeNew_InvalidateNode(TreeContext->TreeNewHandle, &windowNode->Node);
                    needsInvalidate = TRUE;
                }

                windowNode->ProcessIconValid = TRUE;
            }
        }
    }

    if (NeedsNodesStructured)
    {
        NeedsNodesStructured = FALSE;
        TreeNew_NodesStructured(TreeContext->TreeNewHandle);
    }

    if (!fullyInvalidated)
    {
        if (needsInvalidate)
            InvalidateRect(TreeContext->TreeNewHandle, NULL, FALSE);
        else
            InvalidateRect(TreeContext->TreeNewHandle, NULL, FALSE);
    }
}

/**
 * Destroys a window node.
 *
 * \param[in] WindowNode The window node.
 */
VOID WepDestroyWindowNode(
    _In_ PWE_WINDOW_NODE WindowNode
    )
{
    if (WindowNode->Children) PhDereferenceObject(WindowNode->Children);

    if (WindowNode->ThreadString) PhDereferenceObject(WindowNode->ThreadString);
    if (WindowNode->ModuleString) PhDereferenceObject(WindowNode->ModuleString);
    if (WindowNode->FileNameWin32) PhDereferenceObject(WindowNode->FileNameWin32);

    if (WindowNode->ProcessItem) PhDereferenceObject(WindowNode->ProcessItem);
    if (WindowNode->WindowItem) PhDereferenceObject(WindowNode->WindowItem);

    PhFree(WindowNode);
}

#define SORT_FUNCTION(Column) WepWindowTreeNewCompare##Column
#define BEGIN_SORT_FUNCTION(Column) static int __cdecl WepWindowTreeNewCompare##Column( \
    _In_ void *_context, \
    _In_ const void *_elem1, \
    _In_ const void *_elem2 \
    ) \
{ \
    PWE_WINDOW_TREE_CONTEXT context = (PWE_WINDOW_TREE_CONTEXT)_context; \
    PWE_WINDOW_NODE node1 = *(PWE_WINDOW_NODE *)_elem1; \
    PWE_WINDOW_NODE node2 = *(PWE_WINDOW_NODE *)_elem2; \
    int sortResult = 0;

#define END_SORT_FUNCTION \
    if (sortResult == 0) \
        sortResult = uintcmp(node1->WindowIndex, node2->WindowIndex); \
    \
    return PhModifySort(sortResult, context->TreeNewSortOrder); \
}

BEGIN_SORT_FUNCTION(Class)
{
    sortResult = PhCompareStringWithNullSortOrder(node1->WindowItem->WindowClassText, node2->WindowItem->WindowClassText, context->TreeNewSortOrder, TRUE);
}
END_SORT_FUNCTION

BEGIN_SORT_FUNCTION(Handle)
{
    sortResult = uintptrcmp((ULONG_PTR)node1->WindowHandle, (ULONG_PTR)node2->WindowHandle);
}
END_SORT_FUNCTION

BEGIN_SORT_FUNCTION(Index)
{
    sortResult = ushortcmp(node1->WindowIndex, node2->WindowIndex);
}
END_SORT_FUNCTION

BEGIN_SORT_FUNCTION(Generation)
{
    sortResult = ushortcmp(node1->WindowGeneration, node2->WindowGeneration);
}
END_SORT_FUNCTION

BEGIN_SORT_FUNCTION(Text)
{
    sortResult = PhCompareStringWithNullSortOrder(node1->WindowItem->WindowText, node2->WindowItem->WindowText, context->TreeNewSortOrder, TRUE);
}
END_SORT_FUNCTION

BEGIN_SORT_FUNCTION(Process)
{
    sortResult = uintptrcmp((ULONG_PTR)node1->WindowItem->ClientId.UniqueProcess, (ULONG_PTR)node2->WindowItem->ClientId.UniqueProcess);
}
END_SORT_FUNCTION

BEGIN_SORT_FUNCTION(Thread)
{
    sortResult = uintptrcmp((ULONG_PTR)node1->WindowItem->ClientId.UniqueThread, (ULONG_PTR)node2->WindowItem->ClientId.UniqueThread);
}
END_SORT_FUNCTION

BEGIN_SORT_FUNCTION(Module)
{
    sortResult = PhCompareStringWithNullSortOrder(node1->ModuleString, node2->ModuleString, context->TreeNewSortOrder, TRUE);
}
END_SORT_FUNCTION

/**
 * Callback for tree-new messages.
 *
 * \param[in] WindowHandle The tree-new window handle.
 * \param[in] Message The tree-new message.
 * \param[in] Parameter1 Message parameter 1.
 * \param[in] Parameter2 Message parameter 2.
 * \param[in] Context The window tree context.
 * \return TRUE if handled, FALSE otherwise.
 */
BOOLEAN NTAPI WepWindowTreeNewCallback(
    _In_ HWND WindowHandle,
    _In_ PH_TREENEW_MESSAGE Message,
    _In_ PVOID Parameter1,
    _In_ PVOID Parameter2,
    _In_ PVOID Context
    )
{
    PWE_WINDOW_TREE_CONTEXT context = Context;
    PWE_WINDOW_NODE node;

    switch (Message)
    {
    case TreeNewGetChildren:
        {
            PPH_TREENEW_GET_CHILDREN getChildren = Parameter1;
            node = (PWE_WINDOW_NODE)getChildren->Node;

            if (context->TreeNewSortOrder == NoSortOrder)
            {
                if (!node) // Root level
                {
                    getChildren->Children = (PPH_TREENEW_NODE *)context->NodeRootList->Items;
                    getChildren->NumberOfChildren = context->NodeRootList->Count;

                    if (context->ViewMode == WeWindowViewModeParentChild)
                    {
                        qsort_s(
                            context->NodeRootList->Items,
                            context->NodeRootList->Count,
                            sizeof(PVOID),
                            WepWindowTreeNewCompareHandle,
                            context
                            );
                    }
                }
                else // Child level
                {
                    switch (context->ViewMode)
                    {
                    case WeWindowViewModeParentChild:
                    case WeWindowViewModeOwner: // For owner chain, children list is populated by owned windows
                    case WeWindowViewModeZOrder:
                        getChildren->Children = (PPH_TREENEW_NODE *)node->Children->Items;
                        getChildren->NumberOfChildren = node->Children->Count;
                        break;
                    }
                }
            }
            else // Sorted case (applies to all view modes equally for flat sorting)
            {
                if (!node)
                {
                    static CONST _CoreCrtSecureSearchSortCompareFunction sortFunctions[] =
                    {
                        SORT_FUNCTION(Class),
                        SORT_FUNCTION(Handle),
                        SORT_FUNCTION(Text),
                        SORT_FUNCTION(Process),
                        SORT_FUNCTION(Thread),
                        SORT_FUNCTION(Module)
                    };
                    _CoreCrtSecureSearchSortCompareFunction sortFunction;

                    static_assert(RTL_NUMBER_OF(sortFunctions) == WEWNTLC_MAXIMUM, "SortFunctions must equal maximum.");

                    if (context->TreeNewSortColumn < WEWNTLC_MAXIMUM)
                        sortFunction = sortFunctions[context->TreeNewSortColumn];
                    else
                        sortFunction = NULL;

                    if (sortFunction)
                    {
                        qsort_s(context->NodeList->Items, context->NodeList->Count, sizeof(PVOID), sortFunction, context);
                    }

                    getChildren->Children = (PPH_TREENEW_NODE *)context->NodeList->Items;
                    getChildren->NumberOfChildren = context->NodeList->Count;
                }
                else
                {
                    // For sorted view, children are not typically displayed hierarchically.
                    // This path might need to be re-evaluated if hierarchical sorting is desired.
                    getChildren->Children = NULL;
                    getChildren->NumberOfChildren = 0;
                }
            }
        }
        return TRUE;
    case TreeNewIsLeaf:
        {
            PPH_TREENEW_IS_LEAF isLeaf = Parameter1;
            node = (PWE_WINDOW_NODE)isLeaf->Node;

            if (context->TreeNewSortOrder == NoSortOrder)
            {
                switch (context->ViewMode)
                {
                case WeWindowViewModeParentChild:
                case WeWindowViewModeOwner:
                case WeWindowViewModeZOrder:
                    isLeaf->IsLeaf = !node->Children->Count;
                    break;
                }
            }
            else
            {
                // In sorted view, it's a flat list
                isLeaf->IsLeaf = TRUE;
            }
        }
        return TRUE;
    case TreeNewGetCellText:
        {
            PPH_TREENEW_GET_CELL_TEXT getCellText = Parameter1;
            node = (PWE_WINDOW_NODE)getCellText->Node;

            switch (getCellText->Id)
            {
            case WEWNTLC_CLASS:
                getCellText->Text = PhGetStringRef(node->WindowItem->WindowClassText);
                break;
            case WEWNTLC_HANDLE:
                PhInitializeStringRefLongHint(&getCellText->Text, node->WindowHandleString);
                break;
            case WEWNTLC_TEXT:
                getCellText->Text = PhGetStringRef(node->WindowItem->WindowText);
                break;
            case WEWNTLC_PROCESS:
                getCellText->Text = PhGetStringRef(node->ProcessString);
                break;
            case WEWNTLC_THREAD:
                getCellText->Text = PhGetStringRef(node->ThreadString);
                break;
            case WEWNTLC_MODULE:
                getCellText->Text = PhGetStringRef(node->ModuleString);
                break;
            default:
                return FALSE;
            }

            getCellText->Flags = TN_CACHE;
        }
        return TRUE;
    case TreeNewGetNodeColor:
        {
            PPH_TREENEW_GET_NODE_COLOR getNodeColor = Parameter1;
            node = (PWE_WINDOW_NODE)getNodeColor->Node;

            if (!node->WindowItem->WindowVisible)
            {
                getNodeColor->ForeColor = RGB(0x55, 0x55, 0x55);
            }

            if (node->WindowItem->WindowMessageOnly && context->HighlightMessageOnly)
            {
                getNodeColor->BackColor = context->MessageOnlyWindowColor;
            }

            getNodeColor->Flags = TN_CACHE;
        }
        return TRUE;
    case TreeNewGetNodeIcon:
        {
            PPH_TREENEW_GET_NODE_ICON getNodeIcon = Parameter1;

            if (!context->EnableIcons)
                break;

            node = (PWE_WINDOW_NODE)getNodeIcon->Node;
            getNodeIcon->Icon = (HICON)node->WindowIconIndex;
            //getNodeIcon->Flags = TN_CACHE;
        }
        return TRUE;
    case TreeNewSortChanged:
        {
            PPH_TREENEW_SORT_CHANGED_EVENT sorting = Parameter1;

            context->TreeNewSortColumn = sorting->SortColumn;
            context->TreeNewSortOrder = sorting->SortOrder;

            // Force a rebuild to sort the items.
            TreeNew_NodesStructured(WindowHandle);
        }
        return TRUE;
    case TreeNewKeyDown:
        {
            PPH_TREENEW_KEY_EVENT keyEvent = Parameter1;

            switch (keyEvent->VirtualKey)
            {
            case 'C':
                if (GetKeyState(VK_CONTROL) < 0)
                    SendMessage(context->ParentWindowHandle, WM_COMMAND, ID_WINDOW_COPY, 0);
                break;
            }
        }
        return TRUE;
    case TreeNewLeftDoubleClick:
        {
            SendMessage(context->ParentWindowHandle, WM_COMMAND, ID_WINDOW_PROPERTIES, 0);
        }
        return TRUE;
    case TreeNewContextMenu:
        {
            PPH_TREENEW_CONTEXT_MENU contextMenuEvent = Parameter1;

            SendMessage(context->ParentWindowHandle, WM_COMMAND, ID_SHOWCONTEXTMENU, (LPARAM)contextMenuEvent);
        }
        return TRUE;
    case TreeNewHeaderRightClick:
        {
            PH_TN_COLUMN_MENU_DATA data;

            data.TreeNewHandle = WindowHandle;
            data.MouseEvent = Parameter1;
            data.DefaultSortColumn = WEWNTLC_CLASS;
            data.DefaultSortOrder = NoSortOrder;
            PhInitializeTreeNewColumnMenuEx(&data, PH_TN_COLUMN_MENU_SHOW_RESET_SORT);

            data.Selection = PhShowEMenu(data.Menu, WindowHandle, PH_EMENU_SHOW_LEFTRIGHT,
                PH_ALIGN_LEFT | PH_ALIGN_TOP, data.MouseEvent->ScreenLocation.x, data.MouseEvent->ScreenLocation.y);
            PhHandleTreeNewColumnMenu(&data);
            PhDeleteTreeNewColumnMenu(&data);
        }
        return TRUE;
    }

    return FALSE;
}

/**
 * Clears the window tree.
 *
 * \param[in] Context The window tree context.
 */
VOID WeClearWindowTree(
    _In_ PWE_WINDOW_TREE_CONTEXT Context
    )
{
    ULONG i;

    if (Context->NodeStateList)
    {
        PhDereferenceObject(Context->NodeStateList);
        Context->NodeStateList = NULL;
    }

    for (i = 0; i < Context->NodeList->Count; i++)
        WepDestroyWindowNode(Context->NodeList->Items[i]);

    PhClearHashtable(Context->NodeHashtable);
    PhClearList(Context->NodeList);
    PhClearList(Context->NodeRootList);
}

/**
 * Gets the selected window node.
 *
 * \param[in] Context The window tree context.
 * \return The selected window node, or NULL if none selected.
 */
PWE_WINDOW_NODE WeGetSelectedWindowNode(
    _In_ PWE_WINDOW_TREE_CONTEXT Context
    )
{
    PWE_WINDOW_NODE windowNode = NULL;
    ULONG i;

    for (i = 0; i < Context->NodeList->Count; i++)
    {
        windowNode = Context->NodeList->Items[i];

        if (windowNode->Node.Selected)
            return windowNode;
    }

    return NULL;
}

/**
 * Gets all selected window nodes.
 *
 * \param[in] Context The window tree context.
 * \param[out] Nodes A pointer to an array of window nodes.
 * \param[out] NumberOfNodes The number of nodes in the array.
 * \return TRUE if nodes were selected, FALSE otherwise.
 */
_Success_(return)
BOOLEAN WeGetSelectedWindowNodes(
    _In_ PWE_WINDOW_TREE_CONTEXT Context,
    _Out_ PWE_WINDOW_NODE **Nodes,
    _Out_ PULONG NumberOfNodes
    )
{
    PPH_LIST list = PhCreateList(2);
    ULONG i;

    for (i = 0; i < Context->NodeList->Count; i++)
    {
        PWE_WINDOW_NODE node = Context->NodeList->Items[i];

        if (node->Node.Selected)
        {
            PhAddItemList(list, node);
        }
    }

    if (list->Count)
    {
        *Nodes = PhAllocateCopy(list->Items, sizeof(PVOID) * list->Count);
        *NumberOfNodes = list->Count;

        PhDereferenceObject(list);
        return TRUE;
    }

    PhDereferenceObject(list);
    return FALSE;
}

/**
 * Expands or collapses all window nodes.
 *
 * \param[in] Context The window tree context.
 * \param[in] Expand TRUE to expand, FALSE to collapse.
 */
VOID WeExpandAllWindowNodes(
    _In_ PWE_WINDOW_TREE_CONTEXT Context,
    _In_ BOOLEAN Expand
    )
{
    ULONG i;
    BOOLEAN needsRestructure = FALSE;

    for (i = 0; i < Context->NodeList->Count; i++)
    {
        PWE_WINDOW_NODE node = Context->NodeList->Items[i];

        if (node->Children->Count != 0 && node->Node.Expanded != Expand)
        {
            node->Node.Expanded = Expand;
            needsRestructure = TRUE;
        }
    }

    if (needsRestructure)
        TreeNew_NodesStructured(Context->TreeNewHandle);
}

/**
 * Deselects all window nodes.
 *
 * \param[in] Context The window tree context.
 */
VOID WeDeselectAllWindowNodes(
    _In_ PWE_WINDOW_TREE_CONTEXT Context
    )
{
    TreeNew_DeselectRange(Context->TreeNewHandle, 0, -1);
}

/**
 * Selects and ensures window nodes are visible.
 *
 * \param[in] Context The window tree context.
 * \param[in] WindowNodes An array of window nodes.
 * \param[in] NumberOfWindowNodes The number of nodes in the array.
 */
VOID WeSelectAndEnsureVisibleWindowNodes(
    _In_ PWE_WINDOW_TREE_CONTEXT Context,
    _In_ PWE_WINDOW_NODE* WindowNodes,
    _In_ ULONG NumberOfWindowNodes
    )
{
    ULONG i;
    PWE_WINDOW_NODE leader = NULL;
    PWE_WINDOW_NODE node;
    BOOLEAN needsRestructure = FALSE;

    WeDeselectAllWindowNodes(Context);

    for (i = 0; i < NumberOfWindowNodes; i++)
    {
        if (WindowNodes[i]->Node.Visible)
        {
            leader = WindowNodes[i];
            break;
        }
    }

    if (!leader)
        return;

    // Expand recursively upwards, and select the nodes.

    for (i = 0; i < NumberOfWindowNodes; i++)
    {
        if (!WindowNodes[i]->Node.Visible)
            continue;

        node = WindowNodes[i]->Parent;

        while (node)
        {
            if (!node->Node.Expanded)
                needsRestructure = TRUE;

            node->Node.Expanded = TRUE;
            node = node->Parent;
        }

        WindowNodes[i]->Node.Selected = TRUE;
    }

    if (needsRestructure)
        TreeNew_NodesStructured(Context->TreeNewHandle);

    TreeNew_FocusMarkSelectNode(Context->TreeNewHandle, &leader->Node);
}
