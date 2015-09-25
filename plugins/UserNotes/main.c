/*
 * Process Hacker User Notes -
 *   main program
 *
 * Copyright (C) 2011-2015 wj32
 * Copyright (C) 2015 dmex
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

#include <phdk.h>
#include <phappresource.h>
#include <windowsx.h>
#include "db.h"
#include "resource.h"

#define INTENT_PROCESS_COMMENT 0x1
#define INTENT_PROCESS_PRIORITY_CLASS 0x2
#define INTENT_PROCESS_IO_PRIORITY 0x4

#define PROCESS_PRIORITY_SAVE_ID 1
#define PROCESS_PRIORITY_SAVE_FOR_THIS_COMMAND_LINE_ID 2
#define PROCESS_IO_PRIORITY_SAVE_ID 3
#define PROCESS_IO_PRIORITY_SAVE_FOR_THIS_COMMAND_LINE_ID 4
#define PROCESS_ADD_PROCESS_HIGHLIGHT_ID 5
#define PROCESS_REMOVE_PROCESS_HIGHLIGHT_ID 6

#define COMMENT_COLUMN_ID 1

typedef struct _PROCESS_EXTENSION
{
    LIST_ENTRY ListEntry;
    PPH_PROCESS_ITEM ProcessItem;
    BOOLEAN Valid;
    PPH_STRING Comment;
} PROCESS_EXTENSION, *PPROCESS_EXTENSION;

typedef struct _PROCESS_COMMENT_PAGE_CONTEXT
{
    PPH_STRING OriginalComment;
} PROCESS_COMMENT_PAGE_CONTEXT, *PPROCESS_COMMENT_PAGE_CONTEXT;

typedef struct _SERVICE_EXTENSION
{
    LIST_ENTRY ListEntry;
    BOOLEAN Valid;
    PPH_STRING Comment;
} SERVICE_EXTENSION, *PSERVICE_EXTENSION;

typedef struct _SERVICE_COMMENT_PAGE_CONTEXT
{
    PPH_SERVICE_ITEM ServiceItem;
    PH_LAYOUT_MANAGER LayoutManager;
} SERVICE_COMMENT_PAGE_CONTEXT, *PSERVICE_COMMENT_PAGE_CONTEXT;

INT_PTR CALLBACK OptionsDlgProc(
    _In_ HWND hwndDlg,
    _In_ UINT uMsg,
    _In_ WPARAM wParam,
    _In_ LPARAM lParam
    );

INT_PTR CALLBACK ProcessCommentPageDlgProc(
    _In_ HWND hwndDlg,
    _In_ UINT uMsg,
    _In_ WPARAM wParam,
    _In_ LPARAM lParam
    );

INT_PTR CALLBACK ServiceCommentPageDlgProc(
    _In_ HWND hwndDlg,
    _In_ UINT uMsg,
    _In_ WPARAM wParam,
    _In_ LPARAM lParam
    );

UINT_PTR CALLBACK ColorDlgHookProc(
    _In_ HWND hwndDlg,
    _In_ UINT uMsg,
    _In_ WPARAM wParam,
    _In_ LPARAM lParam
    );

PPH_PLUGIN PluginInstance;
PH_CALLBACK_REGISTRATION PluginLoadCallbackRegistration;
PH_CALLBACK_REGISTRATION PluginUnloadCallbackRegistration;
PH_CALLBACK_REGISTRATION PluginShowOptionsCallbackRegistration;
PH_CALLBACK_REGISTRATION PluginMenuItemCallbackRegistration;
PH_CALLBACK_REGISTRATION PluginMenuHookCallbackRegistration;
PH_CALLBACK_REGISTRATION TreeNewMessageCallbackRegistration;
PH_CALLBACK_REGISTRATION MainWindowShowingCallbackRegistration;
PH_CALLBACK_REGISTRATION ProcessPropertiesInitializingCallbackRegistration;
PH_CALLBACK_REGISTRATION ServicePropertiesInitializingCallbackRegistration;
PH_CALLBACK_REGISTRATION ProcessMenuInitializingCallbackRegistration;
PH_CALLBACK_REGISTRATION ProcessTreeNewInitializingCallbackRegistration;
PH_CALLBACK_REGISTRATION GetProcessHighlightingColorCallbackRegistration;
PH_CALLBACK_REGISTRATION ServiceTreeNewInitializingCallbackRegistration;
PH_CALLBACK_REGISTRATION MiListSectionMenuInitializingCallbackRegistration;
PH_CALLBACK_REGISTRATION ProcessModifiedCallbackRegistration;
PH_CALLBACK_REGISTRATION ProcessesUpdatedCallbackRegistration;

HWND ProcessTreeNewHandle;
LIST_ENTRY ProcessListHead = { &ProcessListHead, &ProcessListHead };
PH_QUEUED_LOCK ProcessListLock = PH_QUEUED_LOCK_INIT;

HWND ServiceTreeNewHandle;
LIST_ENTRY ServiceListHead = { &ServiceListHead, &ServiceListHead };
PH_QUEUED_LOCK ServiceListLock = PH_QUEUED_LOCK_INIT;

static COLORREF ProcessCustomColors[16] =
{
    RGB(255, 255, 255),
    RGB(255, 255, 255),
    RGB(255, 255, 255),
    RGB(255, 255, 255),
    RGB(255, 255, 255),
    RGB(255, 255, 255),
    RGB(255, 255, 255),
    RGB(255, 255, 255),
    RGB(255, 255, 255),
    RGB(255, 255, 255),
    RGB(255, 255, 255),
    RGB(255, 255, 255),
    RGB(255, 255, 255),
    RGB(255, 255, 255),
    RGB(255, 255, 255),
    RGB(255, 255, 255)
};

static BOOLEAN MatchDbObjectIntent(
    _In_ PDB_OBJECT Object,
    _In_ ULONG Intent
    )
{
    return (!(Intent & INTENT_PROCESS_COMMENT) || Object->Comment->Length != 0) &&
        (!(Intent & INTENT_PROCESS_PRIORITY_CLASS) || Object->PriorityClass != 0) &&
        (!(Intent & INTENT_PROCESS_IO_PRIORITY) || Object->IoPriorityPlusOne != 0);
}

static PDB_OBJECT FindDbObjectForProcess(
    _In_ PPH_PROCESS_ITEM ProcessItem,
    _In_ ULONG Intent
    )
{
    PDB_OBJECT object;

    if (
        ProcessItem->CommandLine && (object = FindDbObject(COMMAND_LINE_TAG, &ProcessItem->CommandLine->sr)) &&
        MatchDbObjectIntent(object, Intent)
        )
        return object;

    if ((object = FindDbObject(FILE_TAG, &ProcessItem->ProcessName->sr)) && MatchDbObjectIntent(object, Intent))
        return object;

    return NULL;
}

static VOID DeleteDbObjectForProcessIfUnused(
    _In_ PDB_OBJECT Object
    )
{
    if (Object->Comment->Length == 0 && Object->PriorityClass == 0 && Object->IoPriorityPlusOne == 0 && Object->BackColor == ULONG_MAX)
    {
        DeleteDbObject(Object);
    }
}

static VOID LoadCustomColors(
    _In_ PPH_STRING String
    )
{
    PH_STRINGREF part;
    PH_STRINGREF remaining = String->sr;

    for (SIZE_T i = 0; i < ARRAYSIZE(ProcessCustomColors); i++)
    {
        ULONG64 integer = 0;

        PhSplitStringRefAtChar(&remaining, ',', &part, &remaining);

        if (PhStringToInteger64(&part, 10, &integer))
        {
            ProcessCustomColors[i] = (COLORREF)integer;
        }
    }
}

static PPH_STRING SaveCustomColors(
    VOID
    )
{
    PH_STRING_BUILDER stringBuilder;

    PhInitializeStringBuilder(&stringBuilder, 100);

    for (SIZE_T i = 0; i < ARRAYSIZE(ProcessCustomColors); i++)
    {
        PhAppendFormatStringBuilder(&stringBuilder, L"%lu,", ProcessCustomColors[i]);
    }

    if (stringBuilder.String->Length != 0)
        PhRemoveEndStringBuilder(&stringBuilder, 1);

    return PhFinalStringBuilderString(&stringBuilder);
}

static ULONG GetProcessIoPriority(
    _In_ HANDLE ProcessId
    )
{
    HANDLE processHandle;
    ULONG ioPriority = -1;

    if (NT_SUCCESS(PhOpenProcess(
        &processHandle,
        ProcessQueryAccess,
        ProcessId
        )))
    {
        if (!NT_SUCCESS(PhGetProcessIoPriority(
            processHandle,
            &ioPriority
            )))
        {
            ioPriority = -1;
        }

        NtClose(processHandle);
    }

    return ioPriority;
}

static ULONG GetPriorityClassFromId(
    _In_ ULONG Id
    )
{
    switch (Id)
    {
    case PHAPP_ID_PRIORITY_REALTIME:
        return PROCESS_PRIORITY_CLASS_REALTIME;
    case PHAPP_ID_PRIORITY_HIGH:
        return PROCESS_PRIORITY_CLASS_HIGH;
    case PHAPP_ID_PRIORITY_ABOVENORMAL:
        return PROCESS_PRIORITY_CLASS_ABOVE_NORMAL;
    case PHAPP_ID_PRIORITY_NORMAL:
        return PROCESS_PRIORITY_CLASS_NORMAL;
    case PHAPP_ID_PRIORITY_BELOWNORMAL:
        return PROCESS_PRIORITY_CLASS_BELOW_NORMAL;
    case PHAPP_ID_PRIORITY_IDLE:
        return PROCESS_PRIORITY_CLASS_IDLE;
    }

    return 0;
}

static ULONG GetIoPriorityFromId(
    _In_ ULONG Id
    )
{
    switch (Id)
    {
    case PHAPP_ID_I_0:
        return 0;
    case PHAPP_ID_I_1:
        return 1;
    case PHAPP_ID_I_2:
        return 2;
    case PHAPP_ID_I_3:
        return 3;
    }

    return -1;
}

static VOID NTAPI LoadCallback(
    _In_opt_ PVOID Parameter,
    _In_opt_ PVOID Context
    )
{
    PPH_STRING path;
    PPH_STRING realPath;
    PPH_STRING customColors;

    path = PhGetStringSetting(SETTING_NAME_DATABASE_PATH);
    realPath = PhExpandEnvironmentStrings(&path->sr);
    PhDereferenceObject(path);
    path = realPath;

    customColors = PhGetStringSetting(SETTING_NAME_CUSTOM_COLOR_LIST);
    LoadCustomColors(customColors);
    PhDereferenceObject(customColors);

    if (RtlDetermineDosPathNameType_U(path->Buffer) == RtlPathTypeRelative)
    {
        PPH_STRING directory;

        directory = PhGetApplicationDirectory();
        realPath = PhConcatStringRef2(&directory->sr, &path->sr);
        PhDereferenceObject(directory);
        PhDereferenceObject(path);
        path = realPath;
    }

    SetDbPath(path);
    PhDereferenceObject(path);

    LoadDb();
}

static VOID NTAPI UnloadCallback(
    _In_opt_ PVOID Parameter,
    _In_opt_ PVOID Context
    )
{
    PPH_STRING customColors;

    customColors = SaveCustomColors();
    PhSetStringSetting2(SETTING_NAME_CUSTOM_COLOR_LIST, &customColors->sr);
    PhDereferenceObject(customColors);

    SaveDb();
}

VOID NTAPI ShowOptionsCallback(
    _In_opt_ PVOID Parameter,
    _In_opt_ PVOID Context
    )
{
    DialogBox(
        PluginInstance->DllBase,
        MAKEINTRESOURCE(IDD_OPTIONS),
        (HWND)Parameter,
        OptionsDlgProc
        );
}

VOID NTAPI MenuItemCallback(
    _In_opt_ PVOID Parameter,
    _In_opt_ PVOID Context
    )
{
    PPH_PLUGIN_MENU_ITEM menuItem = Parameter;
    PPH_PROCESS_ITEM processItem = PhGetSelectedProcessItem();
    PDB_OBJECT object;

    if (!processItem)
        return;

    switch (menuItem->Id)
    {
    case PROCESS_PRIORITY_SAVE_ID:
        {
            LockDb();

            if ((object = FindDbObject(FILE_TAG, &processItem->ProcessName->sr)) && object->PriorityClass != 0)
            {
                object->PriorityClass = 0;
                DeleteDbObjectForProcessIfUnused(object);
            }
            else
            {
                object = CreateDbObject(FILE_TAG, &processItem->ProcessName->sr, NULL);
                object->PriorityClass = processItem->PriorityClass;
            }

            UnlockDb();
            SaveDb();
        }
        break;
    case PROCESS_PRIORITY_SAVE_FOR_THIS_COMMAND_LINE_ID:
        {
            if (processItem->CommandLine)
            {
                LockDb();

                if ((object = FindDbObject(COMMAND_LINE_TAG, &processItem->CommandLine->sr)) && object->PriorityClass != 0)
                {
                    object->PriorityClass = 0;
                    DeleteDbObjectForProcessIfUnused(object);
                }
                else
                {
                    object = CreateDbObject(COMMAND_LINE_TAG, &processItem->CommandLine->sr, NULL);
                    object->PriorityClass = processItem->PriorityClass;
                }

                UnlockDb();
                SaveDb();
            }
        }
        break;
    case PROCESS_IO_PRIORITY_SAVE_ID:
        {
            LockDb();

            if ((object = FindDbObject(FILE_TAG, &processItem->ProcessName->sr)) && object->IoPriorityPlusOne != 0)
            {
                object->IoPriorityPlusOne = 0;
                DeleteDbObjectForProcessIfUnused(object);
            }
            else
            {
                object = CreateDbObject(FILE_TAG, &processItem->ProcessName->sr, NULL);
                object->IoPriorityPlusOne = GetProcessIoPriority(processItem->ProcessId) + 1;
            }

            UnlockDb();
            SaveDb();
        }
        break;
    case PROCESS_IO_PRIORITY_SAVE_FOR_THIS_COMMAND_LINE_ID:
        {
            if (processItem->CommandLine)
            {
                LockDb();

                if ((object = FindDbObject(COMMAND_LINE_TAG, &processItem->CommandLine->sr)) && object->IoPriorityPlusOne != 0)
                {
                    object->IoPriorityPlusOne = 0;
                    DeleteDbObjectForProcessIfUnused(object);
                }
                else
                {
                    object = CreateDbObject(COMMAND_LINE_TAG, &processItem->CommandLine->sr, NULL);
                    object->IoPriorityPlusOne = GetProcessIoPriority(processItem->ProcessId) + 1;
                }

                UnlockDb();
                SaveDb();
            }
        }
        break;
    case PROCESS_ADD_PROCESS_HIGHLIGHT_ID:
        {
            CHOOSECOLOR chooseColor = { sizeof(CHOOSECOLOR) };
            chooseColor.hwndOwner = PhMainWndHandle;
            chooseColor.lpCustColors = ProcessCustomColors;
            chooseColor.lpfnHook = ColorDlgHookProc;
            chooseColor.Flags = CC_ANYCOLOR | CC_FULLOPEN | CC_SOLIDCOLOR | CC_ENABLEHOOK;

            if (ChooseColor(&chooseColor))
            {
                LockDb();

                if ((object = FindDbObject(FILE_TAG, &processItem->ProcessName->sr)) && object->BackColor != ULONG_MAX)
                {
                    object->BackColor = chooseColor.rgbResult;
                }
                else
                {
                    object = CreateDbObject(FILE_TAG, &processItem->ProcessName->sr, NULL);
                    object->BackColor = chooseColor.rgbResult;
                }

                UnlockDb();
                SaveDb();
            }

            PhInvalidateAllProcessNodes();
        }
        break;
    case PROCESS_REMOVE_PROCESS_HIGHLIGHT_ID:
        {
            LockDb();

            if ((object = FindDbObject(FILE_TAG, &processItem->ProcessName->sr)) && object->BackColor != ULONG_MAX)
            {
                object->BackColor = ULONG_MAX;
                DeleteDbObjectForProcessIfUnused(object);
            }

            UnlockDb();
            SaveDb();

            PhInvalidateAllProcessNodes();
        }
        break;
    }
}

VOID NTAPI MenuHookCallback(
    _In_opt_ PVOID Parameter,
    _In_opt_ PVOID Context
    )
{
    PPH_PLUGIN_MENU_HOOK_INFORMATION menuHookInfo = Parameter;
    ULONG id = menuHookInfo->SelectedItem->Id;

    switch (id)
    {
    case PHAPP_ID_PRIORITY_REALTIME:
    case PHAPP_ID_PRIORITY_HIGH:
    case PHAPP_ID_PRIORITY_ABOVENORMAL:
    case PHAPP_ID_PRIORITY_NORMAL:
    case PHAPP_ID_PRIORITY_BELOWNORMAL:
    case PHAPP_ID_PRIORITY_IDLE:
        {
            BOOLEAN changed = FALSE;
            PPH_PROCESS_ITEM *processes;
            ULONG numberOfProcesses;
            ULONG i;

            PhGetSelectedProcessItems(&processes, &numberOfProcesses);
            LockDb();

            for (i = 0; i < numberOfProcesses; i++)
            {
                PDB_OBJECT object;

                if (object = FindDbObjectForProcess(processes[i], INTENT_PROCESS_PRIORITY_CLASS))
                {
                    ULONG newPriorityClass = GetPriorityClassFromId(id);

                    if (object->PriorityClass != newPriorityClass)
                    {
                        object->PriorityClass = newPriorityClass;
                        changed = TRUE;
                    }
                }
            }

            UnlockDb();
            PhFree(processes);

            if (changed)
                SaveDb();
        }
        break;
    case PHAPP_ID_I_0:
    case PHAPP_ID_I_1:
    case PHAPP_ID_I_2:
    case PHAPP_ID_I_3:
        {
            BOOLEAN changed = FALSE;
            PPH_PROCESS_ITEM *processes;
            ULONG numberOfProcesses;
            ULONG i;

            PhGetSelectedProcessItems(&processes, &numberOfProcesses);
            LockDb();

            for (i = 0; i < numberOfProcesses; i++)
            {
                PDB_OBJECT object;

                if (object = FindDbObjectForProcess(processes[i], INTENT_PROCESS_IO_PRIORITY))
                {
                    ULONG newIoPriorityPlusOne = GetIoPriorityFromId(id) + 1;

                    if (object->IoPriorityPlusOne != newIoPriorityPlusOne)
                    {
                        object->IoPriorityPlusOne = newIoPriorityPlusOne;
                        changed = TRUE;
                    }
                }
            }

            UnlockDb();
            PhFree(processes);

            if (changed)
                SaveDb();
        }
        break;
    }
}

VOID InvalidateProcessComments(
    VOID
    )
{
    PLIST_ENTRY listEntry;

    PhAcquireQueuedLockExclusive(&ProcessListLock);

    listEntry = ProcessListHead.Flink;

    while (listEntry != &ProcessListHead)
    {
        PPROCESS_EXTENSION extension;

        extension = CONTAINING_RECORD(listEntry, PROCESS_EXTENSION, ListEntry);

        extension->Valid = FALSE;

        listEntry = listEntry->Flink;
    }

    PhReleaseQueuedLockExclusive(&ProcessListLock);
}

VOID UpdateProcessComment(
    _In_ PPH_PROCESS_NODE Node,
    _In_ PPROCESS_EXTENSION Extension
    )
{
    if (!Extension->Valid)
    {
        PDB_OBJECT object;

        LockDb();

        if (object = FindDbObjectForProcess(Node->ProcessItem, INTENT_PROCESS_COMMENT))
        {
            PhSwapReference(&Extension->Comment, object->Comment);
        }
        else
        {
            PhClearReference(&Extension->Comment);
        }

        UnlockDb();

        Extension->Valid = TRUE;
    }
}

VOID InvalidateServiceComments(
    VOID
    )
{
    PLIST_ENTRY listEntry;

    PhAcquireQueuedLockExclusive(&ServiceListLock);

    listEntry = ServiceListHead.Flink;

    while (listEntry != &ServiceListHead)
    {
        PSERVICE_EXTENSION extension;

        extension = CONTAINING_RECORD(listEntry, SERVICE_EXTENSION, ListEntry);

        extension->Valid = FALSE;

        listEntry = listEntry->Flink;
    }

    PhReleaseQueuedLockExclusive(&ServiceListLock);
}

VOID UpdateServiceComment(
    _In_ PPH_SERVICE_NODE Node,
    _In_ PSERVICE_EXTENSION Extension
    )
{
    if (!Extension->Valid)
    {
        PDB_OBJECT object;

        LockDb();

        if (object = FindDbObject(SERVICE_TAG, &Node->ServiceItem->Name->sr))
        {
            PhSwapReference(&Extension->Comment, object->Comment);
        }
        else
        {
            PhClearReference(&Extension->Comment);
        }

        UnlockDb();

        Extension->Valid = TRUE;
    }
}

VOID TreeNewMessageCallback(
    _In_opt_ PVOID Parameter,
    _In_opt_ PVOID Context
    )
{
    PPH_PLUGIN_TREENEW_MESSAGE message = Parameter;

    switch (message->Message)
    {
    case TreeNewGetCellText:
        {
            PPH_TREENEW_GET_CELL_TEXT getCellText = message->Parameter1;

            if (message->TreeNewHandle == ProcessTreeNewHandle)
            {
                PPH_PROCESS_NODE node;

                node = (PPH_PROCESS_NODE)getCellText->Node;

                switch (message->SubId)
                {
                case COMMENT_COLUMN_ID:
                    {
                        PPROCESS_EXTENSION extension;

                        extension = PhPluginGetObjectExtension(PluginInstance, node->ProcessItem, EmProcessItemType);
                        UpdateProcessComment(node, extension);
                        getCellText->Text = PhGetStringRef(extension->Comment);
                    }
                    break;
                }
            }
            else if (message->TreeNewHandle == ServiceTreeNewHandle)
            {
                PPH_SERVICE_NODE node;

                node = (PPH_SERVICE_NODE)getCellText->Node;

                switch (message->SubId)
                {
                case COMMENT_COLUMN_ID:
                    {
                        PSERVICE_EXTENSION extension;

                        extension = PhPluginGetObjectExtension(PluginInstance, node->ServiceItem, EmServiceItemType);
                        UpdateServiceComment(node, extension);
                        getCellText->Text = PhGetStringRef(extension->Comment);
                    }
                    break;
                }
            }
        }
        break;
    }
}

VOID MainWindowShowingCallback(
    _In_opt_ PVOID Parameter,
    _In_opt_ PVOID Context
    )
{
    NOTHING;
}

VOID ProcessPropertiesInitializingCallback(
    _In_opt_ PVOID Parameter,
    _In_opt_ PVOID Context
    )
{
    PPH_PLUGIN_PROCESS_PROPCONTEXT propContext = Parameter;

    PhAddProcessPropPage(
        propContext->PropContext,
        PhCreateProcessPropPageContextEx(PluginInstance->DllBase, MAKEINTRESOURCE(IDD_PROCCOMMENT), ProcessCommentPageDlgProc, NULL)
        );
}

VOID AddSavePriorityMenuItemsAndHook(
    _In_ PPH_PLUGIN_MENU_INFORMATION MenuInfo,
    _In_ PPH_PROCESS_ITEM ProcessItem,
    _In_ BOOLEAN UseSelectionForHook
    )
{
    PPH_EMENU_ITEM priorityMenuItem;
    PPH_EMENU_ITEM ioPriorityMenuItem;
    PPH_EMENU_ITEM saveMenuItem;
    PPH_EMENU_ITEM saveForCommandLineMenuItem;
    PDB_OBJECT object;

    // Priority
    if (priorityMenuItem = PhFindEMenuItem(MenuInfo->Menu, 0, L"Priority", 0))
    {
        PhInsertEMenuItem(priorityMenuItem, PhPluginCreateEMenuItem(PluginInstance, PH_EMENU_SEPARATOR, 0, L"", NULL), -1);
        PhInsertEMenuItem(priorityMenuItem, saveMenuItem = PhPluginCreateEMenuItem(PluginInstance, 0, PROCESS_PRIORITY_SAVE_ID, PhaFormatString(L"Save for %s", ProcessItem->ProcessName->Buffer)->Buffer, NULL), -1);
        PhInsertEMenuItem(priorityMenuItem, saveForCommandLineMenuItem = PhPluginCreateEMenuItem(PluginInstance, 0, PROCESS_PRIORITY_SAVE_FOR_THIS_COMMAND_LINE_ID, L"Save for this command line", NULL), -1);

        if (!ProcessItem->CommandLine)
            saveForCommandLineMenuItem->Flags |= PH_EMENU_DISABLED;

        LockDb();

        if ((object = FindDbObject(FILE_TAG, &ProcessItem->ProcessName->sr)) && object->PriorityClass != 0)
            saveMenuItem->Flags |= PH_EMENU_CHECKED;
        if (ProcessItem->CommandLine && (object = FindDbObject(COMMAND_LINE_TAG, &ProcessItem->CommandLine->sr)) && object->PriorityClass != 0)
            saveForCommandLineMenuItem->Flags |= PH_EMENU_CHECKED;

        UnlockDb();
    }

    // I/O Priority
    if (ioPriorityMenuItem = PhFindEMenuItem(MenuInfo->Menu, 0, L"I/O Priority", 0))
    {
        PhInsertEMenuItem(ioPriorityMenuItem, PhPluginCreateEMenuItem(PluginInstance, PH_EMENU_SEPARATOR, 0, L"", NULL), -1);
        PhInsertEMenuItem(ioPriorityMenuItem, saveMenuItem = PhPluginCreateEMenuItem(PluginInstance, 0, PROCESS_IO_PRIORITY_SAVE_ID, PhaFormatString(L"Save for %s", ProcessItem->ProcessName->Buffer)->Buffer, NULL), -1);
        PhInsertEMenuItem(ioPriorityMenuItem, saveForCommandLineMenuItem = PhPluginCreateEMenuItem(PluginInstance, 0, PROCESS_IO_PRIORITY_SAVE_FOR_THIS_COMMAND_LINE_ID, L"Save for this command line", NULL), -1);

        if (!ProcessItem->CommandLine)
            saveForCommandLineMenuItem->Flags |= PH_EMENU_DISABLED;

        LockDb();

        if ((object = FindDbObject(FILE_TAG, &ProcessItem->ProcessName->sr)) && object->IoPriorityPlusOne != 0)
            saveMenuItem->Flags |= PH_EMENU_CHECKED;
        if (ProcessItem->CommandLine && (object = FindDbObject(COMMAND_LINE_TAG, &ProcessItem->CommandLine->sr)) && object->IoPriorityPlusOne != 0)
            saveForCommandLineMenuItem->Flags |= PH_EMENU_CHECKED;

        UnlockDb();
    }

    PhPluginAddMenuHook(MenuInfo, PluginInstance, UseSelectionForHook ? NULL : ProcessItem->ProcessId);
}

VOID ProcessMenuInitializingCallback(
    _In_opt_ PVOID Parameter,
    _In_opt_ PVOID Context
    )
{
    PPH_PLUGIN_MENU_INFORMATION menuInfo = Parameter;
    PPH_EMENU_ITEM miscMenuItem;
    PPH_EMENU_ITEM highlightMenuItem;
    PDB_OBJECT object;

    if (menuInfo->u.Process.NumberOfProcesses != 1)
        return;

    AddSavePriorityMenuItemsAndHook(menuInfo, menuInfo->u.Process.Processes[0], TRUE);

    if (!(miscMenuItem = PhFindEMenuItem(menuInfo->Menu, 0, L"Miscellaneous", 0)))
        return;

    LockDb();

    if ((object = FindDbObject(FILE_TAG, &menuInfo->u.Process.Processes[0]->ProcessName->sr)) && object->BackColor != ULONG_MAX)
    {
        PhInsertEMenuItem(miscMenuItem, highlightMenuItem = PhPluginCreateEMenuItem(PluginInstance, 0, PROCESS_REMOVE_PROCESS_HIGHLIGHT_ID, L"Highlight Process", menuInfo->u.Process.Processes[0]), 0);
        highlightMenuItem->Flags |= PH_EMENU_CHECKED;
    }
    else
    {
        PhInsertEMenuItem(miscMenuItem, highlightMenuItem = PhPluginCreateEMenuItem(PluginInstance, 0, PROCESS_ADD_PROCESS_HIGHLIGHT_ID, L"Highlight Process", menuInfo->u.Process.Processes[0]), 0);
    }

    UnlockDb();
}

LONG NTAPI ProcessCommentSortFunction(
    _In_ PVOID Node1,
    _In_ PVOID Node2,
    _In_ ULONG SubId,
    _In_ PVOID Context
    )
{
    PPH_PROCESS_NODE node1 = Node1;
    PPH_PROCESS_NODE node2 = Node2;
    PPROCESS_EXTENSION extension1 = PhPluginGetObjectExtension(PluginInstance, node1->ProcessItem, EmProcessItemType);
    PPROCESS_EXTENSION extension2 = PhPluginGetObjectExtension(PluginInstance, node2->ProcessItem, EmProcessItemType);

    UpdateProcessComment(node1, extension1);
    UpdateProcessComment(node2, extension2);

    return PhCompareStringWithNull(extension1->Comment, extension2->Comment, TRUE);
}

VOID ProcessTreeNewInitializingCallback(
    _In_opt_ PVOID Parameter,
    _In_opt_ PVOID Context
    )
{
    PPH_PLUGIN_TREENEW_INFORMATION info = Parameter;
    PH_TREENEW_COLUMN column;

    ProcessTreeNewHandle = info->TreeNewHandle;

    memset(&column, 0, sizeof(PH_TREENEW_COLUMN));
    column.Text = L"Comment";
    column.Width = 120;
    column.Alignment = PH_ALIGN_LEFT;

    PhPluginAddTreeNewColumn(PluginInstance, info->CmData, &column, COMMENT_COLUMN_ID, NULL, ProcessCommentSortFunction);
}

VOID GetProcessHighlightingColorCallback(
    _In_opt_ PVOID Parameter,
    _In_opt_ PVOID Context
    )
{
    PPH_PLUGIN_GET_HIGHLIGHTING_COLOR getHighlightingColor = Parameter;
    PPH_PROCESS_ITEM processItem = (PPH_PROCESS_ITEM)getHighlightingColor->Parameter;
    PDB_OBJECT object;

    if (getHighlightingColor->Handled)
        return;

    LockDb();

    if ((object = FindDbObject(FILE_TAG, &processItem->ProcessName->sr)) && object->BackColor != ULONG_MAX)
    {
        getHighlightingColor->BackColor = object->BackColor;
        getHighlightingColor->Cache = TRUE;
        getHighlightingColor->Handled = TRUE;
    }

    UnlockDb();
}

VOID ServicePropertiesInitializingCallback(
    _In_opt_ PVOID Parameter,
    _In_opt_ PVOID Context
    )
{
    PPH_PLUGIN_OBJECT_PROPERTIES objectProperties = Parameter;
    PROPSHEETPAGE propSheetPage;

    if (objectProperties->NumberOfPages < objectProperties->MaximumNumberOfPages)
    {
        memset(&propSheetPage, 0, sizeof(PROPSHEETPAGE));
        propSheetPage.dwSize = sizeof(PROPSHEETPAGE);
        propSheetPage.dwFlags = PSP_USETITLE;
        propSheetPage.hInstance = PluginInstance->DllBase;
        propSheetPage.pszTemplate = MAKEINTRESOURCE(IDD_SRVCOMMENT);
        propSheetPage.pszTitle = L"Comment";
        propSheetPage.pfnDlgProc = ServiceCommentPageDlgProc;
        propSheetPage.lParam = (LPARAM)objectProperties->Parameter;
        objectProperties->Pages[objectProperties->NumberOfPages++] = CreatePropertySheetPage(&propSheetPage);
    }
}

LONG NTAPI ServiceCommentSortFunction(
    _In_ PVOID Node1,
    _In_ PVOID Node2,
    _In_ ULONG SubId,
    _In_ PVOID Context
    )
{
    PPH_SERVICE_NODE node1 = Node1;
    PPH_SERVICE_NODE node2 = Node2;
    PSERVICE_EXTENSION extension1 = PhPluginGetObjectExtension(PluginInstance, node1->ServiceItem, EmServiceItemType);
    PSERVICE_EXTENSION extension2 = PhPluginGetObjectExtension(PluginInstance, node2->ServiceItem, EmServiceItemType);

    UpdateServiceComment(node1, extension1);
    UpdateServiceComment(node2, extension2);

    return PhCompareStringWithNull(extension1->Comment, extension2->Comment, TRUE);
}

VOID ServiceTreeNewInitializingCallback(
    _In_opt_ PVOID Parameter,
    _In_opt_ PVOID Context
    )
{
    PPH_PLUGIN_TREENEW_INFORMATION info = Parameter;
    PH_TREENEW_COLUMN column;

    ServiceTreeNewHandle = info->TreeNewHandle;

    memset(&column, 0, sizeof(PH_TREENEW_COLUMN));
    column.Text = L"Comment";
    column.Width = 120;
    column.Alignment = PH_ALIGN_LEFT;

    PhPluginAddTreeNewColumn(PluginInstance, info->CmData, &column, COMMENT_COLUMN_ID, NULL, ServiceCommentSortFunction);
}

VOID MiListSectionMenuInitializingCallback(
    _In_opt_ PVOID Parameter,
    _In_opt_ PVOID Context
    )
{
    PPH_PLUGIN_MENU_INFORMATION menuInfo = Parameter;

    AddSavePriorityMenuItemsAndHook(menuInfo, menuInfo->u.MiListSection.ProcessGroup->Representative, FALSE);
}

VOID ProcessModifiedCallback(
    _In_opt_ PVOID Parameter,
    _In_opt_ PVOID Context
    )
{
    PPH_PROCESS_ITEM processItem = Parameter;
    PPROCESS_EXTENSION extension = PhPluginGetObjectExtension(PluginInstance, processItem, EmProcessItemType);

    extension->Valid = FALSE;
}

VOID ProcessesUpdatedCallback(
    _In_opt_ PVOID Parameter,
    _In_opt_ PVOID Context
    )
{
    PLIST_ENTRY listEntry;

    if (GetNumberOfDbObjects() == 0)
        return;

    PhAcquireQueuedLockExclusive(&ProcessListLock);
    LockDb();

    listEntry = ProcessListHead.Flink;

    while (listEntry != &ProcessListHead)
    {
        PPROCESS_EXTENSION extension;
        PPH_PROCESS_ITEM processItem;
        PDB_OBJECT object;

        extension = CONTAINING_RECORD(listEntry, PROCESS_EXTENSION, ListEntry);
        processItem = extension->ProcessItem;

        if (object = FindDbObjectForProcess(processItem, INTENT_PROCESS_PRIORITY_CLASS))
        {
            if (processItem->PriorityClass != object->PriorityClass)
            {
                HANDLE processHandle;
                PROCESS_PRIORITY_CLASS priorityClass;

                if (NT_SUCCESS(PhOpenProcess(
                    &processHandle,
                    PROCESS_SET_INFORMATION,
                    processItem->ProcessId
                    )))
                {
                    priorityClass.Foreground = FALSE;
                    priorityClass.PriorityClass = (UCHAR)object->PriorityClass;
                    NtSetInformationProcess(processHandle, ProcessPriorityClass, &priorityClass, sizeof(PROCESS_PRIORITY_CLASS));

                    NtClose(processHandle);
                }
            }
        }

        if (object = FindDbObjectForProcess(processItem, INTENT_PROCESS_IO_PRIORITY))
        {
            if (object->IoPriorityPlusOne != 0)
            {
                HANDLE processHandle;

                if (NT_SUCCESS(PhOpenProcess(
                    &processHandle,
                    PROCESS_SET_INFORMATION,
                    processItem->ProcessId
                    )))
                {
                    PhSetProcessIoPriority(processHandle, object->IoPriorityPlusOne - 1);
                    NtClose(processHandle);
                }
            }
        }

        listEntry = listEntry->Flink;
    }

    UnlockDb();
    PhReleaseQueuedLockExclusive(&ProcessListLock);
}

VOID ProcessItemCreateCallback(
    _In_ PVOID Object,
    _In_ PH_EM_OBJECT_TYPE ObjectType,
    _In_ PVOID Extension
    )
{
    PPH_PROCESS_ITEM processItem = Object;
    PPROCESS_EXTENSION extension = Extension;

    memset(extension, 0, sizeof(PROCESS_EXTENSION));
    extension->ProcessItem = processItem;

    PhAcquireQueuedLockExclusive(&ProcessListLock);
    InsertTailList(&ProcessListHead, &extension->ListEntry);
    PhReleaseQueuedLockExclusive(&ProcessListLock);
}

VOID ProcessItemDeleteCallback(
    _In_ PVOID Object,
    _In_ PH_EM_OBJECT_TYPE ObjectType,
    _In_ PVOID Extension
    )
{
    PPH_PROCESS_ITEM processItem = Object;
    PPROCESS_EXTENSION extension = Extension;

    PhClearReference(&extension->Comment);
    PhAcquireQueuedLockExclusive(&ProcessListLock);
    RemoveEntryList(&extension->ListEntry);
    PhReleaseQueuedLockExclusive(&ProcessListLock);
}

VOID ServiceItemCreateCallback(
    _In_ PVOID Object,
    _In_ PH_EM_OBJECT_TYPE ObjectType,
    _In_ PVOID Extension
    )
{
    PPH_SERVICE_ITEM processItem = Object;
    PSERVICE_EXTENSION extension = Extension;

    memset(extension, 0, sizeof(SERVICE_EXTENSION));
    PhAcquireQueuedLockExclusive(&ServiceListLock);
    InsertTailList(&ServiceListHead, &extension->ListEntry);
    PhReleaseQueuedLockExclusive(&ServiceListLock);
}

VOID ServiceItemDeleteCallback(
    _In_ PVOID Object,
    _In_ PH_EM_OBJECT_TYPE ObjectType,
    _In_ PVOID Extension
    )
{
    PPH_SERVICE_ITEM processItem = Object;
    PSERVICE_EXTENSION extension = Extension;

    PhClearReference(&extension->Comment);
    PhAcquireQueuedLockExclusive(&ServiceListLock);
    RemoveEntryList(&extension->ListEntry);
    PhReleaseQueuedLockExclusive(&ServiceListLock);
}

LOGICAL DllMain(
    _In_ HINSTANCE Instance,
    _In_ ULONG Reason,
    _Reserved_ PVOID Reserved
    )
{
    if (Reason == DLL_PROCESS_ATTACH)
    {
        PPH_PLUGIN_INFORMATION info;
        PH_SETTING_CREATE settings[] =
        {
            { StringSettingType, SETTING_NAME_DATABASE_PATH, L"%APPDATA%\\Process Hacker 2\\usernotesdb.xml" },
            { StringSettingType, SETTING_NAME_CUSTOM_COLOR_LIST, L"" }
        };

        PluginInstance = PhRegisterPlugin(PLUGIN_NAME, Instance, &info);

        if (!PluginInstance)
            return FALSE;

        info->DisplayName = L"User Notes";
        info->Author = L"dmex, wj32";
        info->Description = L"Allows the user to add comments for processes and services. Also allows the user to save process priority. Also allows the user to highlight individual processes.";
        info->Url = L"http://processhacker.sf.net/forums/viewtopic.php?t=1120";
        info->HasOptions = TRUE;

        InitializeDb();

        PhRegisterCallback(PhGetPluginCallback(PluginInstance, PluginCallbackLoad),
            LoadCallback, NULL, &PluginLoadCallbackRegistration);
        PhRegisterCallback(PhGetPluginCallback(PluginInstance, PluginCallbackUnload),
            UnloadCallback, NULL, &PluginUnloadCallbackRegistration);
        PhRegisterCallback(PhGetPluginCallback(PluginInstance, PluginCallbackShowOptions),
            ShowOptionsCallback, NULL, &PluginShowOptionsCallbackRegistration);
        PhRegisterCallback(PhGetPluginCallback(PluginInstance, PluginCallbackMenuItem),
            MenuItemCallback, NULL, &PluginMenuItemCallbackRegistration);
        PhRegisterCallback(PhGetPluginCallback(PluginInstance, PluginCallbackMenuHook),
            MenuHookCallback, NULL, &PluginMenuHookCallbackRegistration);
        PhRegisterCallback(PhGetPluginCallback(PluginInstance, PluginCallbackTreeNewMessage),
            TreeNewMessageCallback, NULL, &TreeNewMessageCallbackRegistration);
        PhRegisterCallback(PhGetGeneralCallback(GeneralCallbackMainWindowShowing),
            MainWindowShowingCallback, NULL, &MainWindowShowingCallbackRegistration);
        PhRegisterCallback(PhGetGeneralCallback(GeneralCallbackProcessPropertiesInitializing),
            ProcessPropertiesInitializingCallback, NULL, &ProcessPropertiesInitializingCallbackRegistration);
        PhRegisterCallback(PhGetGeneralCallback(GeneralCallbackServicePropertiesInitializing),
            ServicePropertiesInitializingCallback, NULL, &ServicePropertiesInitializingCallbackRegistration);
        PhRegisterCallback(PhGetGeneralCallback(GeneralCallbackProcessMenuInitializing),
            ProcessMenuInitializingCallback, NULL, &ProcessMenuInitializingCallbackRegistration);
        PhRegisterCallback(PhGetGeneralCallback(GeneralCallbackProcessTreeNewInitializing),
            ProcessTreeNewInitializingCallback, NULL, &ProcessTreeNewInitializingCallbackRegistration);
        PhRegisterCallback(PhGetGeneralCallback(GeneralCallbackGetProcessHighlightingColor), 
            GetProcessHighlightingColorCallback, NULL, &GetProcessHighlightingColorCallbackRegistration);
        PhRegisterCallback(PhGetGeneralCallback(GeneralCallbackServiceTreeNewInitializing),
            ServiceTreeNewInitializingCallback, NULL, &ServiceTreeNewInitializingCallbackRegistration);
        PhRegisterCallback(PhGetGeneralCallback(GeneralCallbackMiListSectionMenuInitializing),
            MiListSectionMenuInitializingCallback, NULL, &MiListSectionMenuInitializingCallbackRegistration);
        PhRegisterCallback(&PhProcessModifiedEvent,
            ProcessModifiedCallback, NULL, &ProcessModifiedCallbackRegistration);
        PhRegisterCallback(&PhProcessesUpdatedEvent,
            ProcessesUpdatedCallback, NULL, &ProcessesUpdatedCallbackRegistration);

        PhPluginSetObjectExtension(PluginInstance, EmProcessItemType, sizeof(PROCESS_EXTENSION),
            ProcessItemCreateCallback, ProcessItemDeleteCallback);
        PhPluginSetObjectExtension(PluginInstance, EmServiceItemType, sizeof(SERVICE_EXTENSION),
            ServiceItemCreateCallback, ServiceItemDeleteCallback);

        PhAddSettings(settings, ARRAYSIZE(settings));
    }

    return TRUE;
}

INT_PTR CALLBACK OptionsDlgProc(
    _In_ HWND hwndDlg,
    _In_ UINT uMsg,
    _In_ WPARAM wParam,
    _In_ LPARAM lParam
    )
{
    switch (uMsg)
    {
    case WM_INITDIALOG:
        {
            PPH_STRING path;

            path = PhGetStringSetting(SETTING_NAME_DATABASE_PATH);
            SetDlgItemText(hwndDlg, IDC_DATABASE, path->Buffer);
            PhDereferenceObject(path);
        }
        break;
    case WM_COMMAND:
        {
            switch (LOWORD(wParam))
            {
            case IDCANCEL:
                EndDialog(hwndDlg, IDCANCEL);
                break;
            case IDOK:
                {
                    PhSetStringSetting2(SETTING_NAME_DATABASE_PATH,
                        &PhaGetDlgItemText(hwndDlg, IDC_DATABASE)->sr);

                    EndDialog(hwndDlg, IDOK);
                }
                break;
            case IDC_BROWSE:
                {
                    static PH_FILETYPE_FILTER filters[] =
                    {
                        { L"XML files (*.xml)", L"*.xml" },
                        { L"All files (*.*)", L"*.*" }
                    };
                    PVOID fileDialog;
                    PPH_STRING fileName;

                    fileDialog = PhCreateOpenFileDialog();
                    PhSetFileDialogFilter(fileDialog, filters, sizeof(filters) / sizeof(PH_FILETYPE_FILTER));

                    fileName = PhGetFileName(PhaGetDlgItemText(hwndDlg, IDC_DATABASE));
                    PhSetFileDialogFileName(fileDialog, fileName->Buffer);
                    PhDereferenceObject(fileName);

                    if (PhShowFileDialog(hwndDlg, fileDialog))
                    {
                        fileName = PhGetFileDialogFileName(fileDialog);
                        SetDlgItemText(hwndDlg, IDC_DATABASE, fileName->Buffer);
                        PhDereferenceObject(fileName);
                    }

                    PhFreeFileDialog(fileDialog);
                }
                break;
            }
        }
        break;
    }

    return FALSE;
}

INT_PTR CALLBACK ProcessCommentPageDlgProc(
    _In_ HWND hwndDlg,
    _In_ UINT uMsg,
    _In_ WPARAM wParam,
    _In_ LPARAM lParam
    )
{
    LPPROPSHEETPAGE propSheetPage;
    PPH_PROCESS_PROPPAGECONTEXT propPageContext;
    PPH_PROCESS_ITEM processItem;
    PPROCESS_COMMENT_PAGE_CONTEXT context;

    if (PhPropPageDlgProcHeader(hwndDlg, uMsg, lParam, &propSheetPage, &propPageContext, &processItem))
    {
        context = propPageContext->Context;
    }
    else
    {
        return FALSE;
    }

    switch (uMsg)
    {
    case WM_INITDIALOG:
        {
            PDB_OBJECT object;
            PPH_STRING comment;

            context = PhAllocate(sizeof(PROCESS_COMMENT_PAGE_CONTEXT));
            propPageContext->Context = context;

            // Load the comment.

            SendMessage(GetDlgItem(hwndDlg, IDC_COMMENT), EM_SETLIMITTEXT, UNICODE_STRING_MAX_CHARS, 0);

            LockDb();

            if (object = FindDbObjectForProcess(processItem, INTENT_PROCESS_COMMENT))
            {
                PhSetReference(&comment, object->Comment);

                if (processItem->CommandLine && (object = FindDbObject(COMMAND_LINE_TAG, &processItem->CommandLine->sr)) && object->Comment->Length != 0)
                {
                    Button_SetCheck(GetDlgItem(hwndDlg, IDC_MATCHCOMMANDLINE), BST_CHECKED);
                }
            }
            else
            {
                comment = PhReferenceEmptyString();
            }

            UnlockDb();

            SetDlgItemText(hwndDlg, IDC_COMMENT, comment->Buffer);
            context->OriginalComment = comment;

            if (!processItem->CommandLine)
                EnableWindow(GetDlgItem(hwndDlg, IDC_MATCHCOMMANDLINE), FALSE);
        }
        break;
    case WM_DESTROY:
        {
            PDB_OBJECT object;
            PPH_STRING comment;
            BOOLEAN matchCommandLine;
            BOOLEAN done = FALSE;

            comment = PhGetWindowText(GetDlgItem(hwndDlg, IDC_COMMENT));
            matchCommandLine = Button_GetCheck(GetDlgItem(hwndDlg, IDC_MATCHCOMMANDLINE)) == BST_CHECKED;

            if (!processItem->CommandLine)
                matchCommandLine = FALSE;

            LockDb();

            if (processItem->CommandLine && !matchCommandLine)
            {
                PDB_OBJECT objectForProcessName;
                PPH_STRING message = NULL;

                object = FindDbObject(COMMAND_LINE_TAG, &processItem->CommandLine->sr);
                objectForProcessName = FindDbObject(FILE_TAG, &processItem->ProcessName->sr);

                if (object && objectForProcessName && object->Comment->Length != 0 && objectForProcessName->Comment->Length != 0 &&
                    !PhEqualString(comment, objectForProcessName->Comment, FALSE))
                {
                    message = PhFormatString(
                        L"Do you want to replace the comment for %s which is currently\n    \"%s\"\n"
                        L"with\n    \"%s\"?",
                        processItem->ProcessName->Buffer,
                        objectForProcessName->Comment->Buffer,
                        comment->Buffer
                        );
                }

                if (object)
                {
                    PhMoveReference(&object->Comment, PhReferenceEmptyString());
                    DeleteDbObjectForProcessIfUnused(object);
                }

                if (message)
                {
                    // Prevent deadlocks.
                    UnlockDb();

                    if (MessageBox(hwndDlg, message->Buffer, L"Comment", MB_ICONQUESTION | MB_YESNO) == IDNO)
                    {
                        done = TRUE;
                    }

                    LockDb();
                    PhDereferenceObject(message);
                }
            }

            if (!done)
            {
                if (comment->Length != 0)
                {
                    if (matchCommandLine)
                        CreateDbObject(COMMAND_LINE_TAG, &processItem->CommandLine->sr, comment);
                    else
                        CreateDbObject(FILE_TAG, &processItem->ProcessName->sr, comment);
                }
                else
                {
                    if (
                        (!matchCommandLine && (object = FindDbObject(FILE_TAG, &processItem->ProcessName->sr))) ||
                        (matchCommandLine && (object = FindDbObject(COMMAND_LINE_TAG, &processItem->CommandLine->sr)))
                        )
                    {
                        PhMoveReference(&object->Comment, PhReferenceEmptyString());
                        DeleteDbObjectForProcessIfUnused(object);
                    }
                }
            }

            UnlockDb();

            PhDereferenceObject(comment);

            PhDereferenceObject(context->OriginalComment);
            PhFree(context);

            PhPropPageDlgProcDestroy(hwndDlg);

            SaveDb();
            InvalidateProcessComments();
        }
        break;
    case WM_SHOWWINDOW:
        {
            PPH_LAYOUT_ITEM dialogItem;

            if (dialogItem = PhBeginPropPageLayout(hwndDlg, propPageContext))
            {
                PhAddPropPageLayoutItem(hwndDlg, GetDlgItem(hwndDlg, IDC_COMMENT), dialogItem, PH_ANCHOR_ALL);
                PhAddPropPageLayoutItem(hwndDlg, GetDlgItem(hwndDlg, IDC_REVERT), dialogItem, PH_ANCHOR_LEFT | PH_ANCHOR_BOTTOM);
                PhAddPropPageLayoutItem(hwndDlg, GetDlgItem(hwndDlg, IDC_MATCHCOMMANDLINE), dialogItem, PH_ANCHOR_RIGHT | PH_ANCHOR_BOTTOM);
                PhEndPropPageLayout(hwndDlg, propPageContext);
            }
        }
        break;
    case WM_COMMAND:
        {
            switch (LOWORD(wParam))
            {
            case IDC_COMMENT:
                {
                    if (HIWORD(wParam) == EN_CHANGE)
                        EnableWindow(GetDlgItem(hwndDlg, IDC_REVERT), TRUE);
                }
                break;
            case IDC_REVERT:
                {
                    SetDlgItemText(hwndDlg, IDC_COMMENT, context->OriginalComment->Buffer);
                    SendMessage(GetDlgItem(hwndDlg, IDC_COMMENT), EM_SETSEL, 0, -1);
                    SetFocus(GetDlgItem(hwndDlg, IDC_COMMENT));
                    EnableWindow(GetDlgItem(hwndDlg, IDC_REVERT), FALSE);
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
                {
                    SetWindowLongPtr(hwndDlg, DWLP_MSGRESULT, (LONG_PTR)GetDlgItem(hwndDlg, IDC_MATCHCOMMANDLINE));
                }
                return TRUE;
            }
        }
        break;
    }

    return FALSE;
}

INT_PTR CALLBACK ServiceCommentPageDlgProc(
    _In_ HWND hwndDlg,
    _In_ UINT uMsg,
    _In_ WPARAM wParam,
    _In_ LPARAM lParam
    )
{
    PSERVICE_COMMENT_PAGE_CONTEXT context;

    if (uMsg == WM_INITDIALOG)
    {
        context = PhAllocate(sizeof(SERVICE_COMMENT_PAGE_CONTEXT));
        memset(context, 0, sizeof(SERVICE_COMMENT_PAGE_CONTEXT));

        SetProp(hwndDlg, L"Context", (HANDLE)context);
    }
    else
    {
        context = (PSERVICE_COMMENT_PAGE_CONTEXT)GetProp(hwndDlg, L"Context");

        if (uMsg == WM_DESTROY)
            RemoveProp(hwndDlg, L"Context");
    }

    if (!context)
        return FALSE;

    switch (uMsg)
    {
    case WM_INITDIALOG:
        {
            LPPROPSHEETPAGE propSheetPage = (LPPROPSHEETPAGE)lParam;
            PPH_SERVICE_ITEM serviceItem = (PPH_SERVICE_ITEM)propSheetPage->lParam;
            PDB_OBJECT object;
            PPH_STRING comment;

            context->ServiceItem = serviceItem;

            PhInitializeLayoutManager(&context->LayoutManager, hwndDlg);
            PhAddLayoutItem(&context->LayoutManager, GetDlgItem(hwndDlg, IDC_COMMENT), NULL, PH_ANCHOR_ALL);

            // Load the comment.

            SendMessage(GetDlgItem(hwndDlg, IDC_COMMENT), EM_SETLIMITTEXT, UNICODE_STRING_MAX_CHARS, 0);

            LockDb();

            if (object = FindDbObject(SERVICE_TAG, &serviceItem->Name->sr))
                PhSetReference(&comment, object->Comment);
            else
                comment = PhReferenceEmptyString();

            UnlockDb();

            SetDlgItemText(hwndDlg, IDC_COMMENT, comment->Buffer);
            PhDereferenceObject(comment);
        }
        break;
    case WM_DESTROY:
        {
            PhDeleteLayoutManager(&context->LayoutManager);
            PhFree(context);
        }
        break;
    case WM_SIZE:
        {
            PhLayoutManagerLayout(&context->LayoutManager);
        }
        break;
    case WM_COMMAND:
        {
            switch (LOWORD(wParam))
            {
            case IDC_COMMENT:
                {
                    if (HIWORD(wParam) == EN_CHANGE)
                        EnableWindow(GetDlgItem(hwndDlg, IDC_REVERT), TRUE);
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
            case PSN_KILLACTIVE:
                {
                    SetWindowLongPtr(hwndDlg, DWLP_MSGRESULT, FALSE);
                }
                return TRUE;
            case PSN_APPLY:
                {
                    PDB_OBJECT object;
                    PPH_STRING comment;

                    comment = PhGetWindowText(GetDlgItem(hwndDlg, IDC_COMMENT));

                    LockDb();

                    if (comment->Length != 0)
                    {
                        CreateDbObject(SERVICE_TAG, &context->ServiceItem->Name->sr, comment);
                    }
                    else
                    {
                        if (object = FindDbObject(SERVICE_TAG, &context->ServiceItem->Name->sr))
                            DeleteDbObject(object);
                    }

                    UnlockDb();

                    PhDereferenceObject(comment);

                    SaveDb();
                    InvalidateServiceComments();

                    SetWindowLongPtr(hwndDlg, DWLP_MSGRESULT, PSNRET_NOERROR);
                }
                return TRUE;
            }
        }
        break;
    }

    return FALSE;
}

UINT_PTR CALLBACK ColorDlgHookProc(
    _In_ HWND hwndDlg,
    _In_ UINT uMsg,
    _In_ WPARAM wParam,
    _In_ LPARAM lParam
    )
{
    switch (uMsg)
    {
    case WM_INITDIALOG:
        PhCenterWindow(hwndDlg, PhMainWndHandle);
        break;
    case WM_CTLCOLORBTN:
    case WM_CTLCOLORDLG:
    case WM_CTLCOLORSTATIC:
        {
            HDC hDC = (HDC)wParam;
            HWND hwndChild = (HWND)lParam;

            // Set a transparent background for the control.
            SetBkMode(hDC, TRANSPARENT);

            // Set window background color.
            return (INT_PTR)GetSysColorBrush(COLOR_WINDOW);
        }
        break;
    }

    return FALSE;
}