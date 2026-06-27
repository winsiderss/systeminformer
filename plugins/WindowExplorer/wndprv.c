/*
 * Copyright (c) 2022 Winsider Seminars & Solutions, Inc.  All rights reserved.
 *
 * This file is part of System Informer.
 *
 * Authors:
 *
 *     dmex    2025
 *
 */

#include "wndexp.h"

#include <trace.h>

PPH_OBJECT_TYPE WeProviderWindowItemType = NULL;

_Function_class_(PH_HASHTABLE_EQUAL_FUNCTION)
BOOLEAN WepWindowItemHashtableEqualFunction(
    _In_ PVOID Entry1,
    _In_ PVOID Entry2
    );

_Function_class_(PH_HASHTABLE_HASH_FUNCTION)
ULONG WepWindowItemHashtableHashFunction(
    _In_ PVOID Entry
    );

/**
 * Delete procedure for window item objects.
 *
 * \param[in] Object The window item object.
 * \param[in] Flags Flags.
 */
_Function_class_(PH_TYPE_DELETE_PROCEDURE)
VOID NTAPI WeWindowItemDeleteProcedure(
    _In_ PVOID Object,
    _In_ ULONG Flags
    )
{
    PWE_WINDOW_ITEM windowItem = Object;

    if (windowItem->WindowText)
        PhDereferenceObject(windowItem->WindowText);
    if (windowItem->WindowClassText)
        PhDereferenceObject(windowItem->WindowClassText);
}

/**
 * Creates a window provider.
 *
 * \return The created window provider.
 */
PWE_WINDOW_PROVIDER WeCreateWindowProvider(
    VOID
    )
{
    PWE_WINDOW_PROVIDER provider;

    provider = PhAllocateZero(sizeof(WE_WINDOW_PROVIDER));

    provider->WindowHashtable = PhCreateHashtable(
        sizeof(PWE_WINDOW_ITEM),
        WepWindowItemHashtableEqualFunction,
        WepWindowItemHashtableHashFunction,
        100
        );
    PhInitializeQueuedLock(&provider->WindowHashtableLock);

    PhInitializeCallback(&provider->WindowAddedEvent);
    PhInitializeCallback(&provider->WindowModifiedEvent);
    PhInitializeCallback(&provider->WindowRemovedEvent);
    PhInitializeCallback(&provider->WindowUpdatedEvent);

    return provider;
}

/**
 * Deletes a window provider.
 *
 * \param[in] Provider The window provider.
 */
VOID WeDeleteWindowProvider(
    _In_ PWE_WINDOW_PROVIDER Provider
    )
{
    PWE_WINDOW_ITEM* windowItems;
    ULONG numberOfWindowItems;
    ULONG i;

    WeEnumWindowItems(Provider, &windowItems, &numberOfWindowItems);

    if (windowItems)
    {
        for (i = 0; i < numberOfWindowItems; i++)
        {
            WeDeleteWindowItem(windowItems[i]);
        }
        PhFree(windowItems);
    }

    PhDeleteCallback(&Provider->WindowAddedEvent);
    PhDeleteCallback(&Provider->WindowModifiedEvent);
    PhDeleteCallback(&Provider->WindowRemovedEvent);
    PhDeleteCallback(&Provider->WindowUpdatedEvent);

    PhDereferenceObject(Provider->WindowHashtable);
    PhFree(Provider);
}

/**
 * Creates a window item.
 *
 * \param[in] WindowHandle The window handle.
 * \return The created window item.
 */
PWE_WINDOW_ITEM WeCreateWindowItem(
    _In_ HWND WindowHandle
    )
{
    PWE_WINDOW_ITEM windowItem;

    if (!WeProviderWindowItemType)
    {
        WeProviderWindowItemType = PhCreateObjectType(L"WeProviderWindowItem", 0, WeWindowItemDeleteProcedure);
    }

    windowItem = PhCreateObjectZero(sizeof(WE_WINDOW_ITEM), WeProviderWindowItemType);
    windowItem->WindowHandle = WindowHandle;

    return windowItem;
}

/**
 * Deletes a window item.
 *
 * \param[in] WindowItem The window item.
 */
VOID WeDeleteWindowItem(
    _In_ PWE_WINDOW_ITEM WindowItem
    )
{
    PhDereferenceObject(WindowItem);
}

/**
 * Hashtable equal function for window items.
 *
 * \param[in] Entry1 The first entry.
 * \param[in] Entry2 The second entry.
 * \return TRUE if equal, FALSE otherwise.
 */
_Function_class_(PH_HASHTABLE_EQUAL_FUNCTION)
BOOLEAN WepWindowItemHashtableEqualFunction(
    _In_ PVOID Entry1,
    _In_ PVOID Entry2
    )
{
    PWE_WINDOW_ITEM windowItem1 = *(PWE_WINDOW_ITEM *)Entry1;
    PWE_WINDOW_ITEM windowItem2 = *(PWE_WINDOW_ITEM *)Entry2;

    return windowItem1->WindowHandle == windowItem2->WindowHandle;
}

/**
 * Hashtable hash function for window items.
 *
 * \param[in] Entry The entry to hash.
 * \return The hash value.
 */
_Function_class_(PH_HASHTABLE_HASH_FUNCTION)
ULONG WepWindowItemHashtableHashFunction(
    _In_ PVOID Entry
    )
{
    PWE_WINDOW_ITEM windowItem = *(PWE_WINDOW_ITEM *)Entry;

    return PhHashIntPtr((ULONG_PTR)windowItem->WindowHandle);
}

/**
 * References a window item by window handle.
 *
 * \param[in] Provider The window provider.
 * \param[in] WindowHandle The window handle.
 * \return The window item, or NULL if not found.
 */
PWE_WINDOW_ITEM WeReferenceWindowItem(
    _In_ PWE_WINDOW_PROVIDER Provider,
    _In_ HWND WindowHandle
    )
{
    WE_WINDOW_ITEM lookupWindowItem;
    PWE_WINDOW_ITEM lookupWindowItemPtr = &lookupWindowItem;
    PWE_WINDOW_ITEM *windowItemPtr;
    PWE_WINDOW_ITEM windowItem;

    lookupWindowItem.WindowHandle = WindowHandle;

    PhAcquireQueuedLockShared(&Provider->WindowHashtableLock);

    windowItemPtr = (PWE_WINDOW_ITEM *)PhFindEntryHashtable(
        Provider->WindowHashtable,
        &lookupWindowItemPtr
        );

    if (windowItemPtr)
    {
        windowItem = *windowItemPtr;
        PhReferenceObject(windowItem);
    }
    else
    {
        windowItem = NULL;
    }

    PhReleaseQueuedLockShared(&Provider->WindowHashtableLock);

    return windowItem;
}

/**
 * Enumerates window items in a provider.
 *
 * \param[in] Provider The window provider.
 * \param[out,opt] WindowItems A pointer to an array of window items.
 * \param[out] NumberOfWindowItems The number of items in the array.
 */
VOID WeEnumWindowItems(
    _In_ PWE_WINDOW_PROVIDER Provider,
    _Out_opt_ PWE_WINDOW_ITEM **WindowItems,
    _Out_ PULONG NumberOfWindowItems
    )
{
    PWE_WINDOW_ITEM* windowItems;
    PH_HASHTABLE_ENUM_CONTEXT enumContext;
    PWE_WINDOW_ITEM* windowItem;
    ULONG numberOfWindowItems;
    ULONG count = 0;

    PhAcquireQueuedLockShared(&Provider->WindowHashtableLock);

    PhBeginEnumHashtable(Provider->WindowHashtable, &enumContext);

    while (windowItem = PhNextEnumHashtable(&enumContext))
    {
        count++;
    }

    if (count == 0)
    {
        PhReleaseQueuedLockShared(&Provider->WindowHashtableLock);

        if (WindowItems) *WindowItems = NULL;
        *NumberOfWindowItems = count;
        return;
    }

    numberOfWindowItems = count;
    windowItems = PhAllocate(sizeof(PWE_WINDOW_ITEM) * numberOfWindowItems);
    count = 0;

    PhBeginEnumHashtable(Provider->WindowHashtable, &enumContext);

    while (windowItem = PhNextEnumHashtable(&enumContext))
    {
        windowItems[count++] = (*windowItem);
    }

    PhReleaseQueuedLockShared(&Provider->WindowHashtableLock);

    if (WindowItems)
        *WindowItems = windowItems;
    else
        PhFree(windowItems);
    *NumberOfWindowItems = numberOfWindowItems;
}

/**
 * Internal function to remove a window item.
 *
 * \param[in] Provider The window provider.
 * \param[in] WindowItem The window item to remove.
 */
static VOID WepRemoveWindowItem(
    _In_ PWE_WINDOW_PROVIDER Provider,
    _In_ PWE_WINDOW_ITEM WindowItem
    )
{
    PhRemoveEntryHashtable(Provider->WindowHashtable, &WindowItem);
    WeDeleteWindowItem(WindowItem);
}

typedef struct _WE_WINDOW_ENUM_ENTRY
{
    HWND WindowHandle;
    HWND ParentHandle;
    CLIENT_ID ClientId;
    BOOLEAN IsMessageOnly;
    BOOLEAN IsVisible;
} WE_WINDOW_ENUM_ENTRY, *PWE_WINDOW_ENUM_ENTRY;

typedef struct _WE_WINDOW_ENUM_CONTEXT
{
    PPH_LIST WindowList;
    PWE_WINDOW_SELECTOR Selector;
    BOOLEAN EnumMessageOnly;
    BOOLEAN EnumNonVisible;
} WE_WINDOW_ENUM_CONTEXT, *PWE_WINDOW_ENUM_CONTEXT;

typedef struct _WE_WINDOW_TREE_ENUM_CONTEXT
{
    PWINDOWS_CONTEXT WindowContext;
    PWE_WINDOW_NODE RootNode;
    HANDLE FilterProcessId;
    HANDLE FilterThreadId;
    BOOLEAN IsMessageOnlyWindow;
} WE_WINDOW_TREE_ENUM_CONTEXT, *PWE_WINDOW_TREE_ENUM_CONTEXT;

NTSTATUS WeEnumerateWindows(
    _In_opt_ PWE_WINDOW_SELECTOR Selector,
    _Out_ PWE_WINDOW_ENUM_ENTRY *Windows,
    _Out_ PULONG NumberOfWindows
    );

_Success_(return)
BOOLEAN WepGetWindowInfo(
    _In_ HWND WindowHandle,
    _Out_ PWE_WINDOW_ITEM WindowItem
    );

VOID WepAddEnumChildWindows(
    _In_ PWINDOWS_CONTEXT Context,
    _In_opt_ PWE_WINDOW_NODE ParentNode,
    _In_ HWND WindowHandle,
    _In_opt_ HANDLE FilterProcessId,
    _In_opt_ HANDLE FilterThreadId
    );

static VOID WepSetWindowNodeParent(
    _In_ PWE_WINDOW_TREE_CONTEXT Context,
    _In_ PWE_WINDOW_NODE WindowNode,
    _In_opt_ PWE_WINDOW_NODE ParentNode
    )
{
    ULONG index;

    if (WindowNode->Parent == ParentNode)
    {
        if (ParentNode)
            ParentNode->HasChildren = TRUE;
        return;
    }

    if (WindowNode->Parent)
    {
        if ((index = PhFindItemList(WindowNode->Parent->Children, WindowNode)) != ULONG_MAX)
            PhRemoveItemList(WindowNode->Parent->Children, index);

        WindowNode->Parent->HasChildren = WindowNode->Parent->Children->Count != 0;
    }
    else
    {
        if ((index = PhFindItemList(Context->NodeRootList, WindowNode)) != ULONG_MAX)
            PhRemoveItemList(Context->NodeRootList, index);
    }

    WindowNode->Parent = ParentNode;
    WindowNode->WindowItem->ParentHandle = ParentNode ? ParentNode->WindowHandle : NULL;

    if (ParentNode)
    {
        if (PhFindItemList(ParentNode->Children, WindowNode) == ULONG_MAX)
        {
            if (Context->ViewMode == WeWindowViewModeZOrder)
                WepInsertWindowNodeByZOrder(Context, ParentNode, WindowNode);
            else
                PhAddItemList(ParentNode->Children, WindowNode);
        }

        ParentNode->HasChildren = TRUE;
    }
    else if (PhFindItemList(Context->NodeRootList, WindowNode) == ULONG_MAX)
    {
        if (Context->ViewMode == WeWindowViewModeZOrder)
            WepInsertWindowNodeByZOrder(Context, NULL, WindowNode);
        else
            PhAddItemList(Context->NodeRootList, WindowNode);
    }
}

VOID WepInsertWindowNodeByZOrder(
    _In_ PWE_WINDOW_TREE_CONTEXT Context,
    _In_opt_ PWE_WINDOW_NODE ParentNode,
    _In_ PWE_WINDOW_NODE WindowNode
    )
{
    PPH_LIST list;
    HWND previousWindow;

    list = ParentNode ? ParentNode->Children : Context->NodeRootList;

    if (!list)
        return;

    if (WindowNode->WindowItem->WindowMessageOnly)
    {
        PhAddItemList(list, WindowNode);
        return;
    }

    previousWindow = GetWindow(WindowNode->WindowHandle, GW_HWNDPREV);

    while (previousWindow)
    {
        PWE_WINDOW_NODE previousNode;
        ULONG index;

        previousNode = WeFindWindowNode(Context, previousWindow);

        if (previousNode && previousNode->Parent == ParentNode)
        {
            if ((index = PhFindItemList(list, previousNode)) != ULONG_MAX)
            {
                PhInsertItemList(list, index + 1, WindowNode);
                return;
            }
        }

        previousWindow = GetWindow(previousWindow, GW_HWNDPREV);
    }

    PhInsertItemList(list, 0, WindowNode);
}

VOID WepFillWindowInfo(
    _In_ PWE_WINDOW_TREE_CONTEXT Context,
    _In_ PWE_WINDOW_NODE Node
    )
{
    CLIENT_ID clientId;

    PhClearReference(&Node->ProcessString);
    PhClearReference(&Node->ThreadString);

    PhPrintPointer(Node->WindowHandleString, Node->WindowHandle);
    Node->WindowItem->WindowVisible = !!IsWindowVisible(Node->WindowHandle);

    clientId.UniqueProcess = Node->WindowItem->ClientId.UniqueProcess;
    clientId.UniqueThread = NULL;

    if (clientId.UniqueProcess)
        Node->ProcessString = WeGetClientIdName(&clientId);

    clientId.UniqueProcess = Node->WindowItem->ClientId.UniqueProcess;
    clientId.UniqueThread = Node->WindowItem->ClientId.UniqueThread;

    if (clientId.UniqueThread)
        Node->ThreadString = WeGetClientIdName(&clientId);

    if (Node->WindowItem->ClientId.UniqueProcess && !Node->ProcessItem)
    {
        PPH_PROCESS_ITEM processItem;

        if (processItem = PhReferenceProcessItem(Node->WindowItem->ClientId.UniqueProcess))
        {
            Node->ProcessItem = PhReferenceObject(processItem);

            if (!Node->ModuleString && !PhIsNullOrEmptyString(processItem->FileName))
                Node->ModuleString = PhReferenceObject(processItem->FileName);

            if (processItem->QueryHandle)
            {
                PVOID instanceHandle;
                PPH_STRING fileName;
                PPH_STRING fileNameWin32;

                if (!(instanceHandle = (PVOID)GetWindowLongPtr(Node->WindowHandle, GWLP_HINSTANCE)))
                    instanceHandle = (PVOID)GetClassLongPtr(Node->WindowHandle, GCLP_HMODULE);

                if (instanceHandle && NT_SUCCESS(PhGetProcessMappedFileName(processItem->QueryHandle, instanceHandle, &fileName)))
                {
                    fileNameWin32 = PhGetFileName(fileName);
                    PhMoveReference(&Node->FileNameWin32, fileNameWin32);

                    PhMoveReference(&Node->ModuleString, PhGetBaseName(fileNameWin32));
                    PhDereferenceObject(fileName);
                }
            }

            if (!Node->ProcessIconValid && Context->EnableIcons && !Context->EnableIconsInternal)
            {
                if (PhTestEvent(&processItem->Stage1Event))
                {
                    Node->WindowIconIndex = processItem->SmallIconIndex;
                    Node->ProcessIconValid = TRUE;
                }
            }

            PhDereferenceObject(processItem);
        }
    }

    /*
    if (Context->FilterSupport.FilterList)
        Node->Node.Visible = PhApplyTreeNewFiltersToNode(&Context->FilterSupport, &Node->Node);*/
}

PWE_WINDOW_NODE WepAddChildWindowNode(
    _In_ PWE_WINDOW_TREE_CONTEXT Context,
    _In_opt_ PWE_WINDOW_NODE ParentNode,
    _In_ HWND WindowHandle
    )
{
    PWE_WINDOW_NODE childNode;
    PWE_WINDOW_ITEM windowItem;

    if (childNode = WeFindWindowNode(Context, WindowHandle))
    {
        WepSetWindowNodeParent(Context, childNode, ParentNode);
        WepFillWindowInfo(Context, childNode);
        return childNode;
    }

    windowItem = WeCreateWindowItem(WindowHandle);

    if (!WepGetWindowInfo(WindowHandle, windowItem))
    {
        WeDeleteWindowItem(windowItem);
        return NULL;
    }

    if (ParentNode)
        windowItem->ParentHandle = ParentNode->WindowHandle;
    else if (WindowHandle == GetDesktopWindow() || WindowHandle == HWND_MESSAGE)
        windowItem->ParentHandle = NULL;

    childNode = WeAddWindowNode(Context, windowItem, 0);

    WeDeleteWindowItem(windowItem);

    return childNode;
}

_Function_class_(PH_WINDOW_ENUM_CALLBACK)
BOOLEAN CALLBACK WepEnumChildWindowsProc(
    _In_ HWND WindowHandle,
    _In_ PVOID Context
    )
{
    PWE_WINDOW_TREE_ENUM_CONTEXT context = Context;
    CLIENT_ID clientId;

    if (!NT_SUCCESS(PhGetWindowClientId(WindowHandle, &clientId)))
        return TRUE;

    if ((!context->FilterProcessId || clientId.UniqueProcess == context->FilterProcessId) &&
        (!context->FilterThreadId || clientId.UniqueThread == context->FilterThreadId))
    {
        PWE_WINDOW_NODE childNode;

        childNode = WepAddChildWindowNode(
            &context->WindowContext->TreeContext,
            context->RootNode,
            WindowHandle
            );

        if (childNode)
        {
            if (context->IsMessageOnlyWindow)
                childNode->WindowItem->WindowMessageOnly = TRUE;

            if (FindWindowEx(WindowHandle, NULL, NULL, NULL))
                childNode->HasChildren = TRUE;
        }
    }

    return TRUE;
}

VOID WepAddEnumChildWindows(
    _In_ PWINDOWS_CONTEXT Context,
    _In_opt_ PWE_WINDOW_NODE ParentNode,
    _In_ HWND WindowHandle,
    _In_opt_ HANDLE FilterProcessId,
    _In_opt_ HANDLE FilterThreadId
    )
{
    BOOLEAN enumMessageOnly;
    BOOLEAN isMessageOnlyWindow;

    enumMessageOnly = !!PhGetIntegerSetting(SETTING_NAME_WINDOW_ENUM_MESSAGEONLY);
    isMessageOnlyWindow = WindowHandle == HWND_MESSAGE ||
        (ParentNode && ParentNode->WindowItem->WindowMessageOnly);

    if (isMessageOnlyWindow && !enumMessageOnly)
        return;

    if (PhGetIntegerSetting(SETTING_NAME_WINDOW_ENUM_ALTERNATE))
    {
        WE_WINDOW_TREE_ENUM_CONTEXT enumContext;

        enumContext.WindowContext = Context;
        enumContext.RootNode = ParentNode;
        enumContext.FilterProcessId = FilterProcessId;
        enumContext.FilterThreadId = FilterThreadId;
        enumContext.IsMessageOnlyWindow = isMessageOnlyWindow;

        PhEnumChildWindows(WindowHandle, WepEnumChildWindowsProc, &enumContext);
    }
    else
    {
        HWND childWindow = NULL;
        ULONG i = 0;

        while (i < 0x4000 && (childWindow = FindWindowEx(WindowHandle, childWindow, NULL, NULL)))
        {
            CLIENT_ID clientId;

            if (!NT_SUCCESS(PhGetWindowClientId(childWindow, &clientId)))
            {
                i++;
                continue;
            }

            if ((!FilterProcessId || clientId.UniqueProcess == FilterProcessId) &&
                (!FilterThreadId || clientId.UniqueThread == FilterThreadId))
            {
                PWE_WINDOW_NODE childNode;

                childNode = WepAddChildWindowNode(&Context->TreeContext, ParentNode, childWindow);

                if (childNode)
                {
                    if (isMessageOnlyWindow)
                        childNode->WindowItem->WindowMessageOnly = TRUE;

                    if (FindWindowEx(childWindow, NULL, NULL, NULL))
                    {
                        childNode->HasChildren = TRUE;
                        WepAddEnumChildWindows(Context, childNode, childWindow, FilterProcessId, FilterThreadId);
                    }
                }
            }

            i++;
        }
    }
}

BOOL CALLBACK WepEnumDesktopWindowsProc(
    _In_ HWND WindowHandle,
    _In_ LPARAM lParam
    )
{
    PWINDOWS_CONTEXT context = (PWINDOWS_CONTEXT)lParam;

    WepAddChildWindowNode(&context->TreeContext, NULL, WindowHandle);

    return TRUE;
}

VOID WepAddDesktopWindows(
    _In_ PWINDOWS_CONTEXT Context,
    _In_ PWSTR DesktopName
    )
{
    HDESK desktopHandle;

    if (desktopHandle = OpenDesktop(DesktopName, 0, FALSE, DESKTOP_ENUMERATE))
    {
        EnumDesktopWindows(desktopHandle, WepEnumDesktopWindowsProc, (LPARAM)Context);
        CloseDesktop(desktopHandle);
    }
}

PWE_WINDOW_NODE WepGetOrCreateWindowNode(
    _In_ PWINDOWS_CONTEXT Context,
    _In_ HWND WindowHandle
    )
{
    PWE_WINDOW_NODE node;

    if (node = WeFindWindowNode(&Context->TreeContext, WindowHandle))
        return node;

    return WepAddChildWindowNode(&Context->TreeContext, NULL, WindowHandle);
}

VOID WepEnumerateParentChildWindows(
    _In_ PWINDOWS_CONTEXT Context,
    _In_opt_ HANDLE FilterProcessId,
    _In_opt_ HANDLE FilterThreadId
    )
{
    switch (Context->Selector.Type)
    {
    case WeWindowSelectorAll:
        WepAddEnumChildWindows(Context, NULL, GetDesktopWindow(), NULL, NULL);
        WepAddEnumChildWindows(Context, NULL, HWND_MESSAGE, NULL, NULL);
        break;
    case WeWindowSelectorThread:
        WepAddEnumChildWindows(Context, NULL, GetDesktopWindow(), NULL, Context->Selector.Thread.ThreadId);
        WepAddEnumChildWindows(Context, NULL, HWND_MESSAGE, NULL, Context->Selector.Thread.ThreadId);
        break;
    case WeWindowSelectorProcess:
        WepAddEnumChildWindows(Context, NULL, GetDesktopWindow(), Context->Selector.Process.ProcessId, NULL);
        WepAddEnumChildWindows(Context, NULL, HWND_MESSAGE, Context->Selector.Process.ProcessId, NULL);
        break;
    case WeWindowSelectorDesktop:
        WepAddDesktopWindows(Context, PhGetString(Context->Selector.Desktop.DesktopName));
        break;
    default:
        WepAddEnumChildWindows(Context, NULL, GetDesktopWindow(), FilterProcessId, FilterThreadId);
        break;
    }
}

VOID WepEnumerateZOrderWindows(
    _In_ PWINDOWS_CONTEXT Context,
    _In_opt_ HANDLE FilterProcessId,
    _In_opt_ HANDLE FilterThreadId
    )
{
    HWND currentWindow;

    currentWindow = GetWindow(GetDesktopWindow(), GW_CHILD);

    while (currentWindow)
    {
        CLIENT_ID clientId;

        if (NT_SUCCESS(PhGetWindowClientId(currentWindow, &clientId)))
        {
            if ((!FilterProcessId || clientId.UniqueProcess == FilterProcessId) &&
                (!FilterThreadId || clientId.UniqueThread == FilterThreadId))
            {
                PWE_WINDOW_NODE windowNode;

                windowNode = WepAddChildWindowNode(&Context->TreeContext, NULL, currentWindow);

                if (windowNode && FindWindowEx(currentWindow, NULL, NULL, NULL))
                {
                    windowNode->HasChildren = TRUE;
                    WepAddEnumChildWindows(Context, windowNode, currentWindow, FilterProcessId, FilterThreadId);
                }
            }
        }

        currentWindow = GetWindow(currentWindow, GW_HWNDNEXT);
    }

    if (PhGetIntegerSetting(SETTING_NAME_WINDOW_ENUM_MESSAGEONLY))
    {
        WepAddEnumChildWindows(Context, NULL, HWND_MESSAGE, FilterProcessId, FilterThreadId);
    }
}

VOID WepEnumerateOwnerChainWindows(
    _In_ PWINDOWS_CONTEXT Context,
    _In_opt_ HANDLE FilterProcessId,
    _In_opt_ HANDLE FilterThreadId
    )
{
    PPH_LIST allTopLevelWindows;
    HWND currentWindow;
    ULONG i;

    allTopLevelWindows = PhCreateList(100);
    currentWindow = GetWindow(GetDesktopWindow(), GW_CHILD);

    while (currentWindow)
    {
        CLIENT_ID clientId;

        if (NT_SUCCESS(PhGetWindowClientId(currentWindow, &clientId)))
        {
            if ((!FilterProcessId || clientId.UniqueProcess == FilterProcessId) &&
                (!FilterThreadId || clientId.UniqueThread == FilterThreadId))
            {
                PhAddItemList(allTopLevelWindows, currentWindow);
            }
        }

        currentWindow = GetWindow(currentWindow, GW_HWNDNEXT);
    }

    for (i = 0; i < allTopLevelWindows->Count; i++)
    {
        HWND windowHandle;
        HWND ownerHandle;
        PWE_WINDOW_NODE windowNode;

        windowHandle = (HWND)allTopLevelWindows->Items[i];
        ownerHandle = GetWindow(windowHandle, GW_OWNER);
        windowNode = WepGetOrCreateWindowNode(Context, windowHandle);

        if (!windowNode)
            continue;

        if (ownerHandle && ownerHandle != GetDesktopWindow())
        {
            PWE_WINDOW_NODE ownerNode;

            ownerNode = WepGetOrCreateWindowNode(Context, ownerHandle);

            if (ownerNode)
                WepSetWindowNodeParent(&Context->TreeContext, windowNode, ownerNode);
        }
        else
        {
            WepSetWindowNodeParent(&Context->TreeContext, windowNode, NULL);
        }
    }

    PhDereferenceObject(allTopLevelWindows);
}

VOID WepAddEnumWindowsByViewMode(
    _In_ PWINDOWS_CONTEXT Context
    )
{
    HANDLE filterProcessId = NULL;
    HANDLE filterThreadId = NULL;

    switch (Context->Selector.Type)
    {
    case WeWindowSelectorProcess:
        filterProcessId = Context->Selector.Process.ProcessId;
        break;
    case WeWindowSelectorThread:
        filterThreadId = Context->Selector.Thread.ThreadId;
        break;
    }

    if (Context->Selector.Type == WeWindowSelectorDesktop)
    {
        WepAddDesktopWindows(Context, PhGetString(Context->Selector.Desktop.DesktopName));
        return;
    }

    switch (Context->TreeContext.ViewMode)
    {
    case WeWindowViewModeZOrder:
        WepEnumerateZOrderWindows(Context, filterProcessId, filterThreadId);
        break;
    case WeWindowViewModeOwner:
        WepEnumerateOwnerChainWindows(Context, filterProcessId, filterThreadId);
        break;
    case WeWindowViewModeParentChild:
    default:
        WepEnumerateParentChildWindows(Context, filterProcessId, filterThreadId);
        break;
    }
}

BOOLEAN WeAddWindowToList(
    _In_ PWE_WINDOW_ENUM_CONTEXT EnumContext,
    _In_ HWND WindowHandle,
    _In_opt_ HWND ParentHandle,
    _In_ PCLIENT_ID ClientId,
    _In_ BOOLEAN IsMessageOnly
    )
{
    PWE_WINDOW_ENUM_ENTRY entry;
    BOOLEAN windowVisible;

    windowVisible = !!IsWindowVisible(WindowHandle);

    // Apply filters during enumeration
    if (!IsMessageOnly && !windowVisible && !EnumContext->EnumNonVisible)
    {
        return FALSE; // Skip invisible window
    }

    if (EnumContext->Selector)
    {
        switch (EnumContext->Selector->Type)
        {
        case WeWindowSelectorProcess:
            if (ClientId->UniqueProcess != EnumContext->Selector->Process.ProcessId)
                return FALSE;
            break;
        case WeWindowSelectorThread:
            if (ClientId->UniqueThread != EnumContext->Selector->Thread.ThreadId)
                return FALSE;
            break;
        }
    }

    entry = PhAllocateZero(sizeof(WE_WINDOW_ENUM_ENTRY));
    entry->WindowHandle = WindowHandle;
    entry->ParentHandle = ParentHandle;
    entry->ClientId = *ClientId;
    entry->IsMessageOnly = IsMessageOnly;
    entry->IsVisible = windowVisible;

    PhAddItemList(EnumContext->WindowList, entry);
    return TRUE;
}

_Success_(return)
BOOLEAN WepGetWindowInfo(
    _In_ HWND WindowHandle,
    _Out_ PWE_WINDOW_ITEM WindowItem
    )
{
    ULONG windowTextLength = 0;
    WCHAR windowText[0x100];

    if (!IsWindow(WindowHandle))
        return FALSE;

    WindowItem->ParentHandle = GetParent(WindowHandle);
    WindowItem->WindowVisible = !!IsWindowVisible(WindowHandle);

    if (NT_SUCCESS(PhGetClassName(
        WindowHandle,
        windowText,
        RTL_NUMBER_OF(windowText),
        &windowTextLength
        )) && windowTextLength != 0)
    {
        WindowItem->WindowClassText = PhCreateStringEx(windowText, windowTextLength * sizeof(WCHAR));
    }

    if (NT_SUCCESS(PhGetWindowTextToBuffer(
        WindowHandle,
        PH_GET_WINDOW_TEXT_INTERNAL,
        windowText,
        RTL_NUMBER_OF(windowText),
        &windowTextLength
        )) && windowTextLength != 0)
    {
        WindowItem->WindowText = PhCreateStringEx(windowText, windowTextLength * sizeof(WCHAR));
    }

    if (!NT_SUCCESS(PhGetWindowClientId(
        WindowHandle,
        &WindowItem->ClientId
        )))
    {
        WindowItem->ClientId.UniqueProcess = NULL;
        WindowItem->ClientId.UniqueThread = NULL;
    }

    WindowItem->WindowIndex = LOWORD(WindowHandle);
    WindowItem->WindowGeneration = HIWORD(WindowHandle) & 0x7FFF;

    return TRUE;
}

VOID WepEnumerateChildWindowsRecursive(
    _In_ PWE_WINDOW_ENUM_CONTEXT EnumContext,
    _In_opt_ HWND ParentHandle,
    _In_ BOOLEAN IsMessageOnly
    )
{
    HWND childWindow = NULL;
    ULONG i = 0;
    BOOLEAN isMessageOnly = IsMessageOnly || ParentHandle == HWND_MESSAGE;

    while (i < 0x4000 && (childWindow = FindWindowEx(ParentHandle, childWindow, NULL, NULL)))
    {
        CLIENT_ID clientId;

        if (NT_SUCCESS(PhGetWindowClientId(childWindow, &clientId)))
        {
            WeAddWindowToList(EnumContext, childWindow, ParentHandle, &clientId, isMessageOnly);
            WepEnumerateChildWindowsRecursive(EnumContext, childWindow, isMessageOnly);
        }

        i++;
    }
}

_Function_class_(PH_WINDOW_ENUM_CALLBACK)
BOOLEAN CALLBACK WepEnumerateWindowsCallback(
    _In_ HWND WindowHandle,
    _In_ PVOID Context
    )
{
    PWE_WINDOW_ENUM_CONTEXT enumContext = (PWE_WINDOW_ENUM_CONTEXT)Context;
    CLIENT_ID clientId;

    if (NT_SUCCESS(PhGetWindowClientId(WindowHandle, &clientId)))
    {
        WeAddWindowToList(enumContext, WindowHandle, NULL, &clientId, FALSE);
        WepEnumerateChildWindowsRecursive(enumContext, WindowHandle, FALSE);
    }

    return TRUE;
}

NTSTATUS WeEnumerateWindows(
    _In_opt_ PWE_WINDOW_SELECTOR Selector,
    _Out_ PWE_WINDOW_ENUM_ENTRY *Windows,
    _Out_ PULONG NumberOfWindows
    )
{
    WE_WINDOW_ENUM_CONTEXT enumContext;

    enumContext.WindowList = PhCreateList(100);
    enumContext.EnumMessageOnly = !!PhGetIntegerSetting(SETTING_NAME_WINDOW_ENUM_MESSAGEONLY);
    enumContext.EnumNonVisible = !!PhGetIntegerSetting(SETTING_NAME_WINDOW_ENUM_NONVISIBLE);
    enumContext.Selector = Selector;

    // Enumerate top-level windows using PhEnumWindowsEx instead of EnumWindows
    // because EnumWindows doesn't return Metro/UWP app windows.
    // Each top-level window callback will recursively find its children.
    PhEnumWindowsEx(NULL, WepEnumerateWindowsCallback, &enumContext);

    // Enumerate message-only windows last if enabled
    if (enumContext.EnumMessageOnly)
    {
        WepEnumerateChildWindowsRecursive(&enumContext, HWND_MESSAGE, TRUE);
    }

    *NumberOfWindows = enumContext.WindowList->Count;

    if (*NumberOfWindows > 0)
    {
        // Allocate array and copy the entries (not pointers)
        *Windows = PhAllocate(sizeof(WE_WINDOW_ENUM_ENTRY) * (*NumberOfWindows));
        
        for (ULONG i = 0; i < enumContext.WindowList->Count; i++)
        {
            PWE_WINDOW_ENUM_ENTRY entry = enumContext.WindowList->Items[i];
            (*Windows)[i] = *entry;
            PhFree(entry);
        }
    }
    else
    {
        *Windows = NULL;
    }

    PhDereferenceObject(enumContext.WindowList);

    return STATUS_SUCCESS;
}

_Function_class_(PH_PROVIDER_FUNCTION)
VOID WeWindowProviderUpdate(
    _In_ PVOID Object
    )
{
    PWINDOWS_CONTEXT context = (PWINDOWS_CONTEXT)Object;
    PWE_WINDOW_PROVIDER provider = context->WindowProvider;
    NTSTATUS status;
    PWE_WINDOW_ENUM_ENTRY windows;
    ULONG numberOfWindows;
    ULONG i;

    PhTraceFuncEnter("Window provider run count: %lu", context->WindowProviderRunCount);

    // Enumerate all windows
    status = WeEnumerateWindows(&context->Selector, &windows, &numberOfWindows);

    if (!NT_SUCCESS(status))
    {
        PhTraceFuncExit("Failed to enumerate windows: %!STATUS!", status);
        return;
    }

    // Look for dead windows.
    {
        PPH_LIST windowsToRemove = NULL;
        PH_HASHTABLE_ENUM_CONTEXT enumContext;
        PWE_WINDOW_ITEM *windowItem;

        PhAcquireQueuedLockShared(&provider->WindowHashtableLock);
        PhBeginEnumHashtable(provider->WindowHashtable, &enumContext);

        while (windowItem = PhNextEnumHashtable(&enumContext))
        {
            BOOLEAN found = FALSE;

            for (i = 0; i < numberOfWindows; i++)
            {
                // Check if the window still exists in the enumeration.
                if (windows[i].WindowHandle == (*windowItem)->WindowHandle)
                {
                    found = TRUE;
                    break;
                }
            }

            if (!found)
            {
                // Raise the window removed event.
                PhInvokeCallback(&provider->WindowRemovedEvent, *windowItem);

                if (!windowsToRemove)
                    windowsToRemove = PhCreateList(2);

                PhAddItemList(windowsToRemove, *windowItem);
            }
        }

        PhReleaseQueuedLockShared(&provider->WindowHashtableLock);

        if (windowsToRemove)
        {
            PhAcquireQueuedLockExclusive(&provider->WindowHashtableLock);

            for (i = 0; i < windowsToRemove->Count; i++)
            {
                WepRemoveWindowItem(provider, windowsToRemove->Items[i]);
            }

            PhReleaseQueuedLockExclusive(&provider->WindowHashtableLock);
            PhDereferenceObject(windowsToRemove);
        }
    }

    // Look for new windows and update existing ones.
    for (i = 0; i < numberOfWindows; i++)
    {
        PWE_WINDOW_ITEM windowItem;
        PWE_WINDOW_ENUM_ENTRY windowEntry;

        windowEntry = &windows[i];
        windowItem = WeReferenceWindowItem(provider, windowEntry->WindowHandle);

        if (!windowItem)
        {
            // Create the window item and fill in basic information.
            windowItem = WeCreateWindowItem(windowEntry->WindowHandle);

            if (WepGetWindowInfo(windowEntry->WindowHandle, windowItem))
            {
                windowItem->WindowMessageOnly = windowEntry->IsMessageOnly;

                // Add the window item to the hashtable.
                PhAcquireQueuedLockExclusive(&provider->WindowHashtableLock);
                PhAddEntryHashtable(provider->WindowHashtable, &windowItem);
                PhReleaseQueuedLockExclusive(&provider->WindowHashtableLock);

                // Raise the window added event.
                PhInvokeCallback(&provider->WindowAddedEvent, windowItem);
            }
            else
            {
                WeDeleteWindowItem(windowItem);
            }
        }
        else
        {
            WE_WINDOW_ITEM tempItem = { 0 };

            // Check for modifications.
            if (WepGetWindowInfo(windowEntry->WindowHandle, &tempItem))
            {
                BOOLEAN changed = FALSE;

                if (windowItem->ParentHandle != tempItem.ParentHandle)
                {
                    windowItem->ParentHandle = tempItem.ParentHandle;
                    changed = TRUE;
                }

                if (windowItem->WindowClassText && tempItem.WindowClassText && !PhEqualString(windowItem->WindowClassText, tempItem.WindowClassText, FALSE))
                {
                    PhSwapReference(&windowItem->WindowClassText, tempItem.WindowClassText);
                    changed = TRUE;
                }

                if (windowItem->WindowText && tempItem.WindowText && !PhEqualString(windowItem->WindowText, tempItem.WindowText, FALSE))
                {
                    PhSwapReference(&windowItem->WindowText, tempItem.WindowText);
                    changed = TRUE;
                }

                if (windowItem->ClientId.UniqueProcess != tempItem.ClientId.UniqueProcess)
                {
                    windowItem->ClientId.UniqueProcess = tempItem.ClientId.UniqueProcess;
                    changed = TRUE;
                }

                if (windowItem->ClientId.UniqueThread != tempItem.ClientId.UniqueThread)
                {
                    windowItem->ClientId.UniqueThread = tempItem.ClientId.UniqueThread;
                    changed = TRUE;
                }

                if (windowItem->WindowVisible != tempItem.WindowVisible)
                {
                    windowItem->WindowVisible = tempItem.WindowVisible;
                    changed = TRUE;
                }

                // Raise the window modified event.
                if (changed)
                {
                    PhInvokeCallback(&provider->WindowModifiedEvent, windowItem);
                }

                PhClearReference(&tempItem.WindowClassText);
                PhClearReference(&tempItem.WindowText);
            }
            else
            {
                // Invalid window handle. Raise the window removed event.
                //PhInvokeCallback(&provider->WindowRemovedEvent, windowItem);
                //...
            }

            // Release the reference taken by WeReferenceWindowItem.
            PhDereferenceObject(windowItem);
        }
    }

    // Free enumeration data
    if (windows)
        PhFree(windows);

    PhInvokeCallback(&provider->WindowUpdatedEvent, UlongToPtr(context->WindowProviderRunCount));

    PhTraceFuncExit("Window provider completed: %lu", context->WindowProviderRunCount);

    InterlockedIncrement(&context->WindowProviderRunCount);
}
