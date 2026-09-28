/*
 * Startup Apps.
 *
 * Displays and manages startup entries from the Run/RunOnce registry keys,
 * the per-user and common startup folders and their StartupApproved state,
 * similar to the Startup tab in Task Manager.
 */

#include "exttools.h"

#include <emenu.h>

#define ETP_RUN_KEY_NAME L"Software\\Microsoft\\Windows\\CurrentVersion\\Run"
#define ETP_RUNONCE_KEY_NAME L"Software\\Microsoft\\Windows\\CurrentVersion\\RunOnce"
#define ETP_APPROVED_KEY_NAME L"Software\\Microsoft\\Windows\\CurrentVersion\\Explorer\\StartupApproved"
#define ETP_STARTUP_FOLDER_USER L"%APPDATA%\\Microsoft\\Windows\\Start Menu\\Programs\\Startup"
#define ETP_STARTUP_FOLDER_COMMON L"%ALLUSERSPROFILE%\\Microsoft\\Windows\\Start Menu\\Programs\\Startup"

#define WM_PH_STARTAPPS_SHOWDIALOG (WM_APP + 131)

typedef enum _ETP_STARTUP_ENTRY_TYPE
{
    ETP_STARTUP_ENTRY_REGISTRY,
    ETP_STARTUP_ENTRY_FOLDER
} ETP_STARTUP_ENTRY_TYPE;

typedef struct _ETP_STARTUP_ENTRY
{
    ETP_STARTUP_ENTRY_TYPE Type;
    BOOLEAN RootHiveIsMachine;
    BOOLEAN Wow64Entry;
    BOOLEAN RunOnce;
    BOOLEAN Disabled;
    BOOLEAN ApprovedStateExists;
    PPH_STRING SourceKeyPath; // The registry key path or the startup folder path.
    PPH_STRING EntryName; // The value name or the file name.
    PPH_STRING Command; // The expanded command or the full file path.
} ETP_STARTUP_ENTRY, *PETP_STARTUP_ENTRY;

typedef enum _ETP_STARTUP_TREE_COLUMN_ITEM
{
    ETP_STARTUP_COLUMN_NAME,
    ETP_STARTUP_COLUMN_COMMAND,
    ETP_STARTUP_COLUMN_LOCATION,
    ETP_STARTUP_COLUMN_STATUS,
    ETP_STARTUP_COLUMN_MAXIMUM
} ETP_STARTUP_TREE_COLUMN_ITEM;

typedef struct _ETP_STARTUP_TREE_NODE
{
    PH_TREENEW_NODE Node;

    ULONG64 UniqueId; // used to stabilize sorting

    PETP_STARTUP_ENTRY Entry;

    PH_STRINGREF TextCache[ETP_STARTUP_COLUMN_MAXIMUM];
} ETP_STARTUP_TREE_NODE, *PETP_STARTUP_TREE_NODE;

typedef struct _ETP_STARTUP_CONTEXT
{
    PH_LAYOUT_MANAGER LayoutManager;
    RECT MinimumSize;

    HWND WindowHandle;
    HWND TreeNewHandle;
    HWND SearchWindowHandle;

    ULONG TreeNewSortColumn;
    PH_SORT_ORDER TreeNewSortOrder;
    PPH_LIST NodeList;
    PH_TN_FILTER_SUPPORT FilterSupport;

    ULONG_PTR SearchMatchHandle;
} ETP_STARTUP_CONTEXT, *PETP_STARTUP_CONTEXT;

typedef struct _ETP_ENUM_RUN_CONTEXT
{
    PETP_STARTUP_CONTEXT WindowContext;
    HANDLE ApprovedKeyHandle;
    PPH_STRING SourceKeyPath;
    BOOLEAN RootHiveIsMachine;
    BOOLEAN Wow64Entry;
    BOOLEAN RunOnce;
    PCWSTR LocationString;
} ETP_ENUM_RUN_CONTEXT, *PETP_ENUM_RUN_CONTEXT;

static HANDLE EtStartupAppsDialogThreadHandle = NULL;
static HWND EtStartupAppsDialogHandle = NULL;
static PH_EVENT EtStartupAppsDialogInitializedEvent = PH_EVENT_INIT;

#define SORT_FUNCTION(Column) EtStartupTreeNewCompare##Column
#define BEGIN_SORT_FUNCTION(Column) static int __cdecl EtStartupTreeNewCompare##Column( \
    _In_ void *_context, \
    _In_ const void *_elem1, \
    _In_ const void *_elem2 \
    ) \
{ \
    PETP_STARTUP_CONTEXT context = ((PETP_STARTUP_CONTEXT)_context); \
    PETP_STARTUP_TREE_NODE node1 = *(PETP_STARTUP_TREE_NODE*)_elem1; \
    PETP_STARTUP_TREE_NODE node2 = *(PETP_STARTUP_TREE_NODE*)_elem2; \
    int sortResult = 0;

#define END_SORT_FUNCTION \
    if (sortResult == 0) \
        sortResult = uintptrcmp((ULONG_PTR)node1->UniqueId, (ULONG_PTR)node2->UniqueId); \
    \
    return PhModifySort(sortResult, context->TreeNewSortOrder); \
}

BEGIN_SORT_FUNCTION(Name)
{
    sortResult = PhCompareStringWithNullSortOrder(node1->Entry->EntryName, node2->Entry->EntryName, context->TreeNewSortOrder, TRUE);
}
END_SORT_FUNCTION

BEGIN_SORT_FUNCTION(Command)
{
    sortResult = PhCompareStringWithNullSortOrder(node1->Entry->Command, node2->Entry->Command, context->TreeNewSortOrder, TRUE);
}
END_SORT_FUNCTION

BEGIN_SORT_FUNCTION(Location)
{
    sortResult = uintcmp(node1->Entry->Type, node2->Entry->Type);

    if (sortResult == 0)
        sortResult = wcscmp(
            PhGetStringOrEmpty(node1->Entry->SourceKeyPath),
            PhGetStringOrEmpty(node2->Entry->SourceKeyPath)
            );
}
END_SORT_FUNCTION

BEGIN_SORT_FUNCTION(Status)
{
    sortResult = uintcmp(node1->Entry->Disabled, node2->Entry->Disabled);
}
END_SORT_FUNCTION

_Function_class_(PH_TREENEW_CALLBACK)
BOOLEAN NTAPI EtStartupTreeNewCallback(
    _In_ HWND WindowHandle,
    _In_ PH_TREENEW_MESSAGE Message,
    _In_ PVOID Parameter1,
    _In_ PVOID Parameter2,
    _In_ PVOID Context
    )
{
    PETP_STARTUP_CONTEXT context = Context;
    PETP_STARTUP_TREE_NODE node;

    switch (Message)
    {
    case TreeNewGetChildren:
        {
            PPH_TREENEW_GET_CHILDREN getChildren = Parameter1;
            node = (PETP_STARTUP_TREE_NODE)getChildren->Node;

            if (!getChildren->Node)
            {
                static CONST _CoreCrtSecureSearchSortCompareFunction sortFunctions[] =
                {
                    SORT_FUNCTION(Name),
                    SORT_FUNCTION(Command),
                    SORT_FUNCTION(Location),
                    SORT_FUNCTION(Status),
                };
                _CoreCrtSecureSearchSortCompareFunction sortFunction;

                static_assert(RTL_NUMBER_OF(sortFunctions) == ETP_STARTUP_COLUMN_MAXIMUM, "SortFunctions must equal maximum.");

                if (context->TreeNewSortColumn < ETP_STARTUP_COLUMN_MAXIMUM)
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
        }
        return TRUE;
    case TreeNewIsLeaf:
        {
            PPH_TREENEW_IS_LEAF isLeaf = (PPH_TREENEW_IS_LEAF)Parameter1;

            isLeaf->IsLeaf = TRUE;
        }
        return TRUE;
    case TreeNewGetCellText:
        {
            PPH_TREENEW_GET_CELL_TEXT getCellText = (PPH_TREENEW_GET_CELL_TEXT)Parameter1;
            node = (PETP_STARTUP_TREE_NODE)getCellText->Node;

            switch (getCellText->Id)
            {
            case ETP_STARTUP_COLUMN_NAME:
                getCellText->Text = PhGetStringRef(node->Entry->EntryName);
                break;
            case ETP_STARTUP_COLUMN_COMMAND:
                getCellText->Text = PhGetStringRef(node->Entry->Command);
                break;
            case ETP_STARTUP_COLUMN_LOCATION:
                {
                    PCWSTR location;

                    if (node->Entry->Type == ETP_STARTUP_ENTRY_FOLDER)
                        location = node->Entry->RootHiveIsMachine ? L"Common Startup Folder" : L"User Startup Folder";
                    else if (node->Entry->RootHiveIsMachine)
                        location = node->Entry->Wow64Entry ? L"HKLM\\...\\Run (32-bit)" : L"HKLM\\...\\Run";
                    else
                        location = L"HKCU\\...\\Run";

                    PhInitializeStringRefLongHint(&getCellText->Text, location);
                }
                break;
            case ETP_STARTUP_COLUMN_STATUS:
                PhInitializeStringRefLongHint(&getCellText->Text,
                    node->Entry->Disabled ? L"Disabled" : L"Enabled");
                break;
            default:
                return FALSE;
            }

            getCellText->Flags = TN_CACHE;
        }
        return TRUE;
    case TreeNewSortChanged:
        {
            PPH_TREENEW_SORT_CHANGED_EVENT sorting = Parameter1;

            context->TreeNewSortColumn = sorting->SortColumn;
            context->TreeNewSortOrder = sorting->SortOrder;

            // Force a rebuild to sort the items. (dmex)
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
                    SendMessage(context->WindowHandle, WM_COMMAND, IDC_STARTUP_COPY, 0);
                break;
            }
        }
        return TRUE;
    case TreeNewLeftDoubleClick:
        {
            SendMessage(context->WindowHandle, WM_COMMAND, IDC_STARTUP_OPENLOCATION, 0);
        }
        return TRUE;
    case TreeNewContextMenu:
        {
            PPH_TREENEW_CONTEXT_MENU contextMenuEvent = Parameter1;

            SendMessage(
                context->WindowHandle,
                WM_COMMAND,
                IDC_STARTUP_GOTOENTRY,
                (LPARAM)contextMenuEvent
                );
        }
        return TRUE;
    case TreeNewHeaderRightClick:
        {
            PH_TN_COLUMN_MENU_DATA data;

            memset(&data, 0, sizeof(PH_TN_COLUMN_MENU_DATA));
            data.TreeNewHandle = WindowHandle;
            data.MouseEvent = Parameter1;
            data.DefaultSortColumn = ETP_STARTUP_COLUMN_NAME;
            data.DefaultSortOrder = AscendingSortOrder;
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

VOID EtStartupLoadSettingsTreeList(
    _Inout_ PETP_STARTUP_CONTEXT Context
    )
{
    PPH_STRING settings;

    settings = PhGetStringSetting(SETTING_NAME_STARTUP_TASKS_LISTVIEW_COLUMNS);
    PhCmLoadSettings(Context->TreeNewHandle, &settings->sr);
    PhDereferenceObject(settings);
}

VOID EtStartupSaveSettingsTreeList(
    _Inout_ PETP_STARTUP_CONTEXT Context
    )
{
    PPH_STRING settings;

    settings = PhCmSaveSettings(Context->TreeNewHandle);
    PhSetStringSetting2(SETTING_NAME_STARTUP_TASKS_LISTVIEW_COLUMNS, &settings->sr);
    PhDereferenceObject(settings);
}

_Function_class_(PH_SEARCHCONTROL_CALLBACK)
VOID NTAPI EtStartupSearchControlCallback(
    _In_ ULONG_PTR MatchHandle,
    _In_opt_ PVOID Context
    )
{
    PETP_STARTUP_CONTEXT context = Context;

    assert(context);

    context->SearchMatchHandle = MatchHandle;

    PhApplyTreeNewFilters(&context->FilterSupport);
}

BOOLEAN NTAPI EtStartupTreeFilterCallback(
    _In_ PPH_TREENEW_NODE Node,
    _In_opt_ PVOID Context
    )
{
    PETP_STARTUP_CONTEXT context = Context;
    PETP_STARTUP_TREE_NODE node = (PETP_STARTUP_TREE_NODE)Node;

    if (!context->SearchMatchHandle)
        return TRUE;

    if (!PhIsNullOrEmptyString(node->Entry->EntryName))
    {
        if (PhSearchControlMatch(context->SearchMatchHandle, &node->Entry->EntryName->sr))
            return TRUE;
    }

    if (!PhIsNullOrEmptyString(node->Entry->Command))
    {
        if (PhSearchControlMatch(context->SearchMatchHandle, &node->Entry->Command->sr))
            return TRUE;
    }

    return FALSE;
}

VOID EtStartupDestroyNode(
    _In_ PETP_STARTUP_TREE_NODE Node
    )
{
    PhClearReference(&Node->Entry->SourceKeyPath);
    PhClearReference(&Node->Entry->EntryName);
    PhClearReference(&Node->Entry->Command);

    PhFree(Node->Entry);
    PhFree(Node);
}

VOID EtStartupInitializeTree(
    _Inout_ PETP_STARTUP_CONTEXT Context
    )
{
    Context->NodeList = PhCreateList(32);

    PhSetControlTheme(Context->TreeNewHandle, L"explorer");
    TreeNew_SetCallback(Context->TreeNewHandle, EtStartupTreeNewCallback, Context);

    PhAddTreeNewColumn(Context->TreeNewHandle, ETP_STARTUP_COLUMN_NAME, TRUE, L"Name", 180, PH_ALIGN_LEFT, 0, 0);
    PhAddTreeNewColumn(Context->TreeNewHandle, ETP_STARTUP_COLUMN_COMMAND, TRUE, L"Command", 280, PH_ALIGN_LEFT, ULONG_MAX, 0);
    PhAddTreeNewColumn(Context->TreeNewHandle, ETP_STARTUP_COLUMN_LOCATION, TRUE, L"Location", 150, PH_ALIGN_LEFT, ULONG_MAX, 0);
    PhAddTreeNewColumn(Context->TreeNewHandle, ETP_STARTUP_COLUMN_STATUS, TRUE, L"Status", 80, PH_ALIGN_LEFT, ULONG_MAX, 0);

    PhInitializeTreeNewFilterSupport(&Context->FilterSupport, Context->TreeNewHandle, Context->NodeList);

    TreeNew_SetSort(Context->TreeNewHandle, ETP_STARTUP_COLUMN_NAME, AscendingSortOrder);

    EtStartupLoadSettingsTreeList(Context);
}

VOID EtStartupDeleteTree(
    _Inout_ PETP_STARTUP_CONTEXT Context
    )
{
    PhDeleteTreeNewFilterSupport(&Context->FilterSupport);

    for (ULONG i = 0; i < Context->NodeList->Count; i++)
        EtStartupDestroyNode(Context->NodeList->Items[i]);

    PhClearList(Context->NodeList);

    if (Context->NodeList)
        PhDereferenceObject(Context->NodeList);
}

PETP_STARTUP_TREE_NODE EtStartupGetSelectedNode(
    _In_ PETP_STARTUP_CONTEXT Context
    )
{
    PETP_STARTUP_TREE_NODE node = NULL;

    for (ULONG i = 0; i < Context->NodeList->Count; i++)
    {
        node = Context->NodeList->Items[i];

        if (node->Node.Selected)
            return node;
    }

    return NULL;
}

VOID EtStartupAddEntry(
    _In_ PETP_STARTUP_CONTEXT Context,
    _In_ ETP_STARTUP_ENTRY_TYPE Type,
    _In_ BOOLEAN RootHiveIsMachine,
    _In_ BOOLEAN Wow64Entry,
    _In_ BOOLEAN RunOnce,
    _In_ BOOLEAN Disabled,
    _In_ BOOLEAN ApprovedStateExists,
    _In_opt_ PPH_STRING SourceKeyPath,
    _In_ PPH_STRING EntryName,
    _In_ PPH_STRING Command
    )
{
    static ULONG64 nextUniqueId = 0;
    PETP_STARTUP_TREE_NODE node;
    PETP_STARTUP_ENTRY entry;

    entry = PhAllocateZero(sizeof(ETP_STARTUP_ENTRY));
    entry->Type = Type;
    entry->RootHiveIsMachine = RootHiveIsMachine;
    entry->Wow64Entry = Wow64Entry;
    entry->RunOnce = RunOnce;
    entry->Disabled = Disabled;
    entry->ApprovedStateExists = ApprovedStateExists;
    PhSetReference(&entry->SourceKeyPath, SourceKeyPath);
    PhSetReference(&entry->EntryName, EntryName);
    PhSetReference(&entry->Command, Command);

    node = PhAllocate(sizeof(ETP_STARTUP_TREE_NODE));
    memset(node, 0, sizeof(ETP_STARTUP_TREE_NODE));
    PhInitializeTreeNewNode(&node->Node);

    memset(node->TextCache, 0, sizeof(PH_STRINGREF) * ETP_STARTUP_COLUMN_MAXIMUM);
    node->Node.TextCache = node->TextCache;
    node->Node.TextCacheSize = ETP_STARTUP_COLUMN_MAXIMUM;

    node->Entry = entry;
    node->UniqueId = ++nextUniqueId;

    if (Context->FilterSupport.FilterList)
        node->Node.Visible = PhApplyTreeNewFiltersToNode(&Context->FilterSupport, &node->Node);

    PhAddItemList(Context->NodeList, node);
}

static BOOLEAN EtpQueryApprovedState(
    _In_ HANDLE ApprovedKeyHandle,
    _In_ PCWSTR ValueName,
    _Out_ PBOOLEAN Disabled,
    _Out_ PBOOLEAN Exists
    )
{
    PVOID buffer;

    *Exists = FALSE;
    *Disabled = FALSE;

    if (!ApprovedKeyHandle)
        return FALSE;

    if (NT_SUCCESS(PhQueryValueKeyZ(
        ApprovedKeyHandle,
        ValueName,
        KeyValueFullInformation,
        &buffer
        )))
    {
        PKEY_VALUE_FULL_INFORMATION keyValueInfo = buffer;

        if (
            keyValueInfo->Type == REG_BINARY &&
            keyValueInfo->DataLength >= sizeof(ULONG)
            )
        {
            PUCHAR data = PTR_ADD_OFFSET(keyValueInfo, keyValueInfo->DataOffset);

            // The first byte contains the enabled state. (Task Manager compatibility)
            *Disabled = !!(data[0] & 0x1);
            *Exists = TRUE;
        }

        PhFree(buffer);
        return TRUE;
    }

    return FALSE;
}

static VOID EtpOpenApprovedKey(
    _In_ BOOLEAN RootHiveIsMachine,
    _In_ BOOLEAN Wow64Entry,
    _In_ PCWSTR Suffix,
    _Out_ PHANDLE ApprovedKeyHandle
    )
{
    PPH_STRING keyPath;
    ACCESS_MASK accessMask = KEY_READ;

    keyPath = PhConcatStrings2(ETP_APPROVED_KEY_NAME L"\\", Suffix);

#ifdef _WIN64
    if (Wow64Entry)
        accessMask |= KEY_WOW64_32KEY;
#endif

    if (!NT_SUCCESS(PhOpenKey(
        ApprovedKeyHandle,
        accessMask,
        RootHiveIsMachine ? PH_KEY_LOCAL_MACHINE : PH_KEY_CURRENT_USER,
        &keyPath->sr,
        0
        )))
    {
        *ApprovedKeyHandle = NULL;
    }

    PhDereferenceObject(keyPath);
}

_Function_class_(PH_ENUM_KEY_CALLBACK)
static BOOLEAN NTAPI EtpEnumRunValuesCallback(
    _In_ HANDLE RootDirectory,
    _In_ PVOID Information,
    _In_opt_ PVOID Context
    )
{
    PKEY_VALUE_FULL_INFORMATION keyValueInfo = Information;
    PETP_ENUM_RUN_CONTEXT enumContext = Context;

    if (keyValueInfo->Type == REG_SZ || keyValueInfo->Type == REG_EXPAND_SZ)
    {
        PPH_STRING command;
        BOOLEAN disabled = FALSE;
        BOOLEAN approvedExists = FALSE;

        command = PhCreateStringEx(
            (PWSTR)PTR_ADD_OFFSET(keyValueInfo, keyValueInfo->DataOffset),
            keyValueInfo->DataLength
            );

        if (keyValueInfo->Type == REG_EXPAND_SZ)
        {
            PPH_STRING expandedCommand;

            if (expandedCommand = PhExpandEnvironmentStrings(&command->sr))
                PhMoveReference(&command, expandedCommand);
        }

        EtpQueryApprovedState(enumContext->ApprovedKeyHandle, keyValueInfo->Name, &disabled, &approvedExists);

        EtStartupAddEntry(
            enumContext->WindowContext,
            ETP_STARTUP_ENTRY_REGISTRY,
            enumContext->RootHiveIsMachine,
            enumContext->Wow64Entry,
            enumContext->RunOnce,
            disabled,
            approvedExists,
            enumContext->SourceKeyPath,
            PhCreateString(keyValueInfo->Name),
            command
            );

        PhDereferenceObject(command);
    }

    return TRUE;
}

static VOID EtpEnumerateRunKey(
    _In_ PETP_STARTUP_CONTEXT Context,
    _In_ BOOLEAN RootHiveIsMachine,
    _In_ BOOLEAN Wow64Entry,
    _In_ BOOLEAN RunOnce
    )
{
    HANDLE keyHandle = NULL;
    HANDLE approvedKeyHandle = NULL;
    ACCESS_MASK accessMask = KEY_READ;
    PPH_STRING keyPath;
    ETP_ENUM_RUN_CONTEXT enumContext;

#ifdef _WIN64
    if (Wow64Entry)
        accessMask |= KEY_WOW64_32KEY;
#endif

    keyPath = PhCreateString(RunOnce ? ETP_RUNONCE_KEY_NAME : ETP_RUN_KEY_NAME);

    if (NT_SUCCESS(PhOpenKey(
        &keyHandle,
        accessMask,
        RootHiveIsMachine ? PH_KEY_LOCAL_MACHINE : PH_KEY_CURRENT_USER,
        &keyPath->sr,
        0
        )))
    {
        EtpOpenApprovedKey(
            RootHiveIsMachine,
            Wow64Entry,
            RunOnce ? L"RunOnce" : L"Run",
            &approvedKeyHandle
            );

        enumContext.WindowContext = Context;
        enumContext.ApprovedKeyHandle = approvedKeyHandle;
        enumContext.SourceKeyPath = keyPath;
        enumContext.RootHiveIsMachine = RootHiveIsMachine;
        enumContext.Wow64Entry = Wow64Entry;
        enumContext.RunOnce = RunOnce;
        enumContext.LocationString = NULL;

        PhEnumerateValueKey(
            keyHandle,
            KeyValueFullInformation,
            EtpEnumRunValuesCallback,
            &enumContext
            );

        if (approvedKeyHandle)
            NtClose(approvedKeyHandle);
        NtClose(keyHandle);
    }

    PhDereferenceObject(keyPath);
}

static VOID EtpEnumerateStartupFolder(
    _In_ PETP_STARTUP_CONTEXT Context,
    _In_ BOOLEAN RootHiveIsMachine
    )
    {
    HANDLE approvedKeyHandle = NULL;
    PPH_STRING folderPath;
    PPH_STRING searchPath;
    WIN32_FIND_DATA findData;
    HANDLE findHandle;

    folderPath = PhExpandEnvironmentStrings(
        RootHiveIsMachine ? ETP_STARTUP_FOLDER_COMMON : ETP_STARTUP_FOLDER_USER
        );

    EtpOpenApprovedKey(RootHiveIsMachine, FALSE, L"StartupFolder", &approvedKeyHandle);

    searchPath = PhConcatStrings2(folderPath->Buffer, L"\\*");

    if ((findHandle = FindFirstFile(searchPath->Buffer, &findData)) != INVALID_HANDLE_VALUE)
    {
        do
        {
            BOOLEAN disabled = FALSE;
            BOOLEAN approvedExists = FALSE;

            if (findData.dwFileAttributes & FILE_ATTRIBUTE_DIRECTORY)
                continue;
            if (PhEqualStringZ(findData.cFileName, L"desktop.ini", FALSE))
                continue;

            EtpQueryApprovedState(approvedKeyHandle, findData.cFileName, &disabled, &approvedExists);

            EtStartupAddEntry(
                Context,
                ETP_STARTUP_ENTRY_FOLDER,
                RootHiveIsMachine,
                FALSE,
                FALSE,
                disabled,
                approvedExists,
                folderPath,
                PhCreateString(findData.cFileName),
                PhConcatStrings(3, folderPath->Buffer, L"\\", findData.cFileName)
                );
        } while (FindNextFile(findHandle, &findData));

        FindClose(findHandle);
    }

    PhDereferenceObject(searchPath);

    if (approvedKeyHandle)
        NtClose(approvedKeyHandle);
    PhDereferenceObject(folderPath);
}

static VOID EtStartupRefreshEntries(
    _In_ PETP_STARTUP_CONTEXT Context
    )
{
    TreeNew_SetRedraw(Context->TreeNewHandle, FALSE);

    for (ULONG i = 0; i < Context->NodeList->Count; i++)
        EtStartupDestroyNode(Context->NodeList->Items[i]);

    PhClearList(Context->NodeList);

    EtpEnumerateRunKey(Context, FALSE, FALSE, FALSE);
    EtpEnumerateRunKey(Context, FALSE, FALSE, TRUE);
    EtpEnumerateRunKey(Context, TRUE, FALSE, FALSE);
    EtpEnumerateRunKey(Context, TRUE, FALSE, TRUE);
#ifdef _WIN64
    EtpEnumerateRunKey(Context, TRUE, TRUE, FALSE);
    EtpEnumerateRunKey(Context, TRUE, TRUE, TRUE);
#endif

    EtpEnumerateStartupFolder(Context, FALSE);
    EtpEnumerateStartupFolder(Context, TRUE);

    TreeNew_NodesStructured(Context->TreeNewHandle);
    TreeNew_SetRedraw(Context->TreeNewHandle, TRUE);
}

static PPH_STRING EtpGetCommandExecutable(
    _In_ PPH_STRING Command
    )
{
    SIZE_T commandLength;

    if (PhIsNullOrEmptyString(Command))
        return NULL;

    commandLength = PhCountStringZ(Command->Buffer);

    if (Command->Buffer[0] == L'"')
    {
        for (SIZE_T i = 1; i < commandLength; i++)
        {
            if (Command->Buffer[i] == L'"')
                return PhSubstring(Command, 1, i - 1);
        }

        return PhSubstring(Command, 1, commandLength - 1);
    }

    for (SIZE_T i = 0; i < commandLength; i++)
    {
        if (Command->Buffer[i] == L' ')
            return PhSubstring(Command, 0, i);
    }

    return PhReferenceObject(Command);
}

static BOOLEAN EtStartupToggleEntryState(
    _In_ PETP_STARTUP_CONTEXT Context,
    _In_ PETP_STARTUP_TREE_NODE Node
    )
{
    HANDLE approvedKeyHandle;
    PPH_STRING approvedSuffix;
    ACCESS_MASK accessMask = KEY_READ | KEY_SET_VALUE;
    NTSTATUS status;

    if (Node->Entry->Type == ETP_STARTUP_ENTRY_FOLDER)
        approvedSuffix = PhCreateString(L"StartupFolder");
    else
        approvedSuffix = PhCreateString(Node->Entry->RunOnce ? L"RunOnce" : L"Run");

#ifdef _WIN64
    if (Node->Entry->Wow64Entry)
        accessMask |= KEY_WOW64_32KEY;
#endif

    {
        PPH_STRING approvedKeyPath;

        approvedKeyPath = PhConcatStrings2(ETP_APPROVED_KEY_NAME L"\\", approvedSuffix->Buffer);

        status = PhOpenKey(
            &approvedKeyHandle,
            accessMask,
            Node->Entry->RootHiveIsMachine ? PH_KEY_LOCAL_MACHINE : PH_KEY_CURRENT_USER,
            &approvedKeyPath->sr,
            0
            );

        PhDereferenceObject(approvedKeyPath);
    }

    if (!NT_SUCCESS(status))
    {
        // Create the key if it doesn't exist yet. Only needed when disabling. (dmex)
        if (!Node->Entry->Disabled)
        {
            PhDereferenceObject(approvedSuffix);
            return FALSE;
        }

        {
            PPH_STRING approvedKeyPath;

            approvedKeyPath = PhConcatStrings2(ETP_APPROVED_KEY_NAME L"\\", approvedSuffix->Buffer);

            status = PhCreateKey(
                &approvedKeyHandle,
                accessMask,
                Node->Entry->RootHiveIsMachine ? PH_KEY_LOCAL_MACHINE : PH_KEY_CURRENT_USER,
                &approvedKeyPath->sr,
                0,
                0,
                NULL
                );

            PhDereferenceObject(approvedKeyPath);
        }

        if (!NT_SUCCESS(status))
        {
            PhDereferenceObject(approvedSuffix);
            return FALSE;
        }
    }

    if (Node->Entry->Disabled)
    {
        // Re-enable the entry by deleting the StartupApproved value. (dmex)
        status = PhDeleteValueKeyZ(approvedKeyHandle, Node->Entry->EntryName->Buffer);
    }
    else
    {
        // Disable the entry by writing the StartupApproved value. (Task Manager compatibility)
        UCHAR data[sizeof(LARGE_INTEGER) + sizeof(ULONG)];
        LARGE_INTEGER systemTime;
        ULONG valueSize = sizeof(data);

        NtQuerySystemTime(&systemTime);

        data[0] = 0x03; // disabled
        data[1] = 0x00;
        data[2] = 0x00;
        data[3] = 0x00;
        memcpy(&data[sizeof(ULONG)], &systemTime.QuadPart, sizeof(LARGE_INTEGER));

        {
            PPH_STRINGREF valueName;

            valueName = &Node->Entry->EntryName->sr;

            status = PhSetValueKey(
                approvedKeyHandle,
                valueName,
                REG_BINARY,
                data,
                valueSize
                );
        }
    }

    NtClose(approvedKeyHandle);
    PhDereferenceObject(approvedSuffix);

    return NT_SUCCESS(status);
}

static BOOLEAN EtStartupDeleteEntry(
    _In_ PETP_STARTUP_CONTEXT Context,
    _In_ PETP_STARTUP_TREE_NODE Node
    )
{
    if (Node->Entry->Type == ETP_STARTUP_ENTRY_FOLDER)
    {
        return DeleteFile(Node->Entry->Command->Buffer) != 0;
    }
    else
    {
        HANDLE keyHandle;
        ACCESS_MASK accessMask = KEY_SET_VALUE;
        NTSTATUS status;

#ifdef _WIN64
        if (Node->Entry->Wow64Entry)
            accessMask |= KEY_WOW64_32KEY;
#endif

        if (!NT_SUCCESS(PhOpenKey(
            &keyHandle,
            accessMask,
            Node->Entry->RootHiveIsMachine ? PH_KEY_LOCAL_MACHINE : PH_KEY_CURRENT_USER,
            &Node->Entry->SourceKeyPath->sr,
            0
            )))
        {
            return FALSE;
        }

        status = PhDeleteValueKeyZ(keyHandle, Node->Entry->EntryName->Buffer);
        NtClose(keyHandle);

        return NT_SUCCESS(status);
    }
}

INT_PTR CALLBACK EtStartupAppsDlgProc(
    _In_ HWND hwndDlg,
    _In_ UINT uMsg,
    _In_ WPARAM wParam,
    _In_ LPARAM lParam
    )
{
    PETP_STARTUP_CONTEXT context = NULL;

    if (uMsg == WM_INITDIALOG)
    {
        context = PhAllocateZero(sizeof(ETP_STARTUP_CONTEXT));
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
            context->TreeNewHandle = GetDlgItem(hwndDlg, IDC_STARTUP_TREE);
            context->SearchWindowHandle = GetDlgItem(hwndDlg, IDC_STARTUP_SEARCH);

            PhSetApplicationWindowIcon(hwndDlg);

            EtStartupInitializeTree(context);

            PhInitializeLayoutManager(&context->LayoutManager, hwndDlg);
            PhAddLayoutItem(&context->LayoutManager, context->SearchWindowHandle, NULL, PH_ANCHOR_TOP | PH_ANCHOR_RIGHT);
            PhAddLayoutItem(&context->LayoutManager, GetDlgItem(hwndDlg, IDC_STARTUP_ENABLEDISABLE), NULL, PH_ANCHOR_BOTTOM | PH_ANCHOR_LEFT);
            PhAddLayoutItem(&context->LayoutManager, GetDlgItem(hwndDlg, IDC_STARTUP_DELETE), NULL, PH_ANCHOR_BOTTOM | PH_ANCHOR_LEFT);
            PhAddLayoutItem(&context->LayoutManager, GetDlgItem(hwndDlg, IDC_STARTUP_REFRESH), NULL, PH_ANCHOR_BOTTOM | PH_ANCHOR_LEFT);
            PhAddLayoutItem(&context->LayoutManager, GetDlgItem(hwndDlg, IDOK), NULL, PH_ANCHOR_BOTTOM | PH_ANCHOR_RIGHT);
            PhAddLayoutItem(&context->LayoutManager, context->TreeNewHandle, NULL, PH_ANCHOR_ALL);

            PhCreateSearchControl(
                hwndDlg,
                context->SearchWindowHandle,
                L"Search Startup Apps (Ctrl+K)",
                EtStartupSearchControlCallback,
                context
                );

            context->MinimumSize.left = 0;
            context->MinimumSize.top = 0;
            context->MinimumSize.right = 400;
            context->MinimumSize.bottom = 200;
            MapDialogRect(hwndDlg, &context->MinimumSize);

            if (PhValidWindowPlacementFromSetting(SETTING_NAME_STARTUP_TASKS_WINDOW_POSITION))
                PhLoadWindowPlacementFromSetting(SETTING_NAME_STARTUP_TASKS_WINDOW_POSITION, SETTING_NAME_STARTUP_TASKS_WINDOW_SIZE, hwndDlg);
            else
                PhCenterWindow(hwndDlg, (HWND)lParam);

            PhInitializeWindowTheme(hwndDlg, !!PhGetIntegerSetting(SETTING_ENABLE_THEME_SUPPORT));

            EtStartupAppsDialogHandle = hwndDlg;

            EtStartupRefreshEntries(context);
        }
        break;
    case WM_DESTROY:
        {
            EtStartupAppsDialogHandle = NULL;

            EtStartupSaveSettingsTreeList(context);
            PhSaveWindowPlacementToSetting(SETTING_NAME_STARTUP_TASKS_WINDOW_POSITION, SETTING_NAME_STARTUP_TASKS_WINDOW_SIZE, hwndDlg);

            PhDeleteLayoutManager(&context->LayoutManager);
            EtStartupDeleteTree(context);

            PhRemoveWindowContext(hwndDlg, PH_WINDOW_CONTEXT_DEFAULT);
            PhFree(context);

            PostQuitMessage(0);
        }
        break;
    case WM_SIZE:
        {
            PhLayoutManagerLayout(&context->LayoutManager);
        }
        break;
    case WM_SIZING:
        {
            PhResizingMinimumSize((PRECT)lParam, wParam, context->MinimumSize.right, context->MinimumSize.bottom);
        }
        break;
    case WM_PH_STARTAPPS_SHOWDIALOG:
        {
            if (IsMinimized(hwndDlg))
                ShowWindow(hwndDlg, SW_RESTORE);
            else
                ShowWindow(hwndDlg, SW_SHOW);

            SetForegroundWindow(hwndDlg);
        }
        break;
    case WM_DPICHANGED:
        {
            PhLayoutManagerUpdate(&context->LayoutManager, LOWORD(wParam));
            PhLayoutManagerLayout(&context->LayoutManager);
        }
        break;
    case WM_COMMAND:
        {
            switch (GET_WM_COMMAND_ID(wParam, lParam))
            {
            case IDOK:
                {
                    DestroyWindow(hwndDlg);
                }
                break;
            case IDC_STARTUP_REFRESH:
                {
                    EtStartupRefreshEntries(context);
                }
                break;
            case IDC_STARTUP_ENABLEDISABLE:
                {
                    PETP_STARTUP_TREE_NODE node;

                    if (node = EtStartupGetSelectedNode(context))
                    {
                        if (EtStartupToggleEntryState(context, node))
                            EtStartupRefreshEntries(context);
                        else
                            PhShowStatus(hwndDlg, L"Unable to change the startup entry state.", 0, GetLastError());
                    }
                }
                break;
            case IDC_STARTUP_DELETE:
                {
                    PETP_STARTUP_TREE_NODE node;

                    if (node = EtStartupGetSelectedNode(context))
                    {
                        if (PhShowConfirmMessage(
                            hwndDlg,
                            L"delete",
                            PhGetStringOrEmpty(node->Entry->EntryName),
                            L"Deleting a startup entry will permanently remove it.",
                            FALSE
                            ))
                        {
                            if (EtStartupDeleteEntry(context, node))
                                EtStartupRefreshEntries(context);
                            else
                                PhShowStatus(hwndDlg, L"Unable to delete the startup entry.", 0, GetLastError());
                        }
                    }
                }
                break;
            case IDC_STARTUP_OPENLOCATION:
                {
                    PETP_STARTUP_TREE_NODE node;

                    if (node = EtStartupGetSelectedNode(context))
                    {
                        PPH_STRING fileName;

                        if (fileName = EtpGetCommandExecutable(node->Entry->Command))
                        {
                            PhShellExecuteUserString(
                                hwndDlg,
                                SETTING_FILE_BROWSE_EXECUTABLE,
                                fileName->Buffer,
                                FALSE,
                                L"Make sure the Explorer executable file is present."
                                );
                            PhDereferenceObject(fileName);
                        }
                    }
                }
                break;
            case IDC_STARTUP_COPY:
                {
                    PPH_STRING text;

                    text = PhGetTreeNewText(context->TreeNewHandle, 0);
                    PhSetClipboardString(context->TreeNewHandle, &text->sr);
                    PhDereferenceObject(text);
                }
                break;
            case IDC_STARTUP_GOTOENTRY:
                {
                    PPH_TREENEW_CONTEXT_MENU contextMenuEvent = (PPH_TREENEW_CONTEXT_MENU)lParam;
                    PPH_EMENU menu;
                    PPH_EMENU_ITEM selectedItem;
                    PETP_STARTUP_TREE_NODE node;

                    if (!(node = EtStartupGetSelectedNode(context)))
                        break;

                    menu = PhCreateEMenu();
                    PhInsertEMenuItem(menu, PhCreateEMenuItem(0, IDC_STARTUP_ENABLEDISABLE,
                        node->Entry->Disabled ? L"&Enable" : L"&Disable", NULL, NULL), ULONG_MAX);
                    PhInsertEMenuItem(menu, PhCreateEMenuItem(0, IDC_STARTUP_DELETE, L"D&elete", NULL, NULL), ULONG_MAX);
                    PhInsertEMenuItem(menu, PhCreateEMenuSeparator(), ULONG_MAX);
                    PhInsertEMenuItem(menu, PhCreateEMenuItem(0, IDC_STARTUP_OPENLOCATION, L"Open &file location", NULL, NULL), ULONG_MAX);
                    PhInsertEMenuItem(menu, PhCreateEMenuSeparator(), ULONG_MAX);
                    PhInsertEMenuItem(menu, PhCreateEMenuItem(0, IDC_STARTUP_COPY, L"&Copy\bCtrl+C", NULL, NULL), ULONG_MAX);
                    PhInsertCopyCellEMenuItem(menu, IDC_STARTUP_COPY, context->TreeNewHandle, contextMenuEvent->Column);
                    PhSetFlagsEMenuItem(menu, IDC_STARTUP_ENABLEDISABLE, PH_EMENU_DEFAULT, PH_EMENU_DEFAULT);

                    selectedItem = PhShowEMenu(
                        menu,
                        hwndDlg,
                        PH_EMENU_SHOW_SEND_COMMAND | PH_EMENU_SHOW_LEFTRIGHT,
                        PH_ALIGN_LEFT | PH_ALIGN_TOP,
                        contextMenuEvent->Location.x,
                        contextMenuEvent->Location.y
                        );

                    if (selectedItem && selectedItem->Id != ULONG_MAX)
                    {
                        switch (selectedItem->Id)
                        {
                        case IDC_STARTUP_ENABLEDISABLE:
                            SendMessage(hwndDlg, WM_COMMAND, MAKEWPARAM(IDC_STARTUP_ENABLEDISABLE, 0), 0);
                            break;
                        case IDC_STARTUP_DELETE:
                            SendMessage(hwndDlg, WM_COMMAND, MAKEWPARAM(IDC_STARTUP_DELETE, 0), 0);
                            break;
                        case IDC_STARTUP_OPENLOCATION:
                            SendMessage(hwndDlg, WM_COMMAND, MAKEWPARAM(IDC_STARTUP_OPENLOCATION, 0), 0);
                            break;
                        case IDC_STARTUP_COPY:
                            SendMessage(hwndDlg, WM_COMMAND, MAKEWPARAM(IDC_STARTUP_COPY, 0), 0);
                            break;
                        }

                        PhHandleCopyCellEMenuItem(selectedItem);
                    }

                    PhDestroyEMenu(menu);
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

_Function_class_(USER_THREAD_START_ROUTINE)
NTSTATUS EtShowStartupAppsDialogThread(
    _In_ PVOID Parameter
    )
{
    BOOL result;
    MSG message;
    PH_AUTO_POOL autoPool;

    PhInitializeAutoPool(&autoPool);

    EtStartupAppsDialogHandle = PhCreateDialog(
        NtCurrentImageBase(),
        MAKEINTRESOURCE(IDD_STARTUP_TASKS),
        NULL,
        EtStartupAppsDlgProc,
        Parameter
        );

    PhSetEvent(&EtStartupAppsDialogInitializedEvent);

    if (EtStartupAppsDialogHandle)
    {
        PostMessage(EtStartupAppsDialogHandle, WM_PH_STARTAPPS_SHOWDIALOG, 0, 0);

        while (result = GetMessage(&message, NULL, 0, 0))
        {
            if (result == INT_ERROR)
                break;

            if (!IsDialogMessage(EtStartupAppsDialogHandle, &message))
            {
                TranslateMessage(&message);
                DispatchMessage(&message);
            }

            PhDrainAutoPool(&autoPool);
        }
    }

    PhDeleteAutoPool(&autoPool);
    PhResetEvent(&EtStartupAppsDialogInitializedEvent);

    if (EtStartupAppsDialogThreadHandle)
    {
        NtClose(EtStartupAppsDialogThreadHandle);
        EtStartupAppsDialogThreadHandle = NULL;
    }

    return STATUS_SUCCESS;
}

VOID EtShowStartupAppsDialog(
    _In_ HWND ParentWindowHandle
    )
{
    if (!EtStartupAppsDialogThreadHandle)
    {
        if (!NT_SUCCESS(PhCreateThreadEx(&EtStartupAppsDialogThreadHandle, EtShowStartupAppsDialogThread, ParentWindowHandle)))
        {
            PhShowError2(ParentWindowHandle, L"Unable to create the window.", L"%s", L"");
            return;
        }

        PhWaitForEvent(&EtStartupAppsDialogInitializedEvent, NULL);
    }

    PostMessage(EtStartupAppsDialogHandle, WM_PH_STARTAPPS_SHOWDIALOG, 0, 0);
}
