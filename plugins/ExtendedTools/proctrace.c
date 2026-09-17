/*
 * Process Tracker (ETW).
 *
 * Tracks process creation and termination events in real time using a
 * private ETW session with the Microsoft-Windows-Kernel-Process provider,
 * capturing short-lived processes missed by periodic enumeration.
 */

#include "exttools.h"

#include <tdh.h>

#define PROCESS_TRACE_KEYWORD_PROCESS 0x10

#define PHP_PROCTRACE_MAX_EVENTS 10000

#define WM_PH_PROCTRACE_SHOWDIALOG (WM_APP + 121)
#define WM_PH_PROCTRACE_SHOWMENU (WM_APP + 122)

// 22fb2cd6-0e7b-4e8f-9d10-4a8a7c0c0f2f
DEFINE_GUID(EtProcessTraceProviderGuid, 0x22fb2cd6, 0x0e7b, 0x4e8f, 0x9d, 0x10, 0x4a, 0x8a, 0x7c, 0x0c, 0x0f, 0x2f);
// 9d2bf3e7-0b45-4c8a-a428-1f6a9b7751d0
DEFINE_GUID(EtProcessTraceSessionGuid, 0x9d2bf3e7, 0x0b45, 0x4c8a, 0xa4, 0x28, 0x1f, 0x6a, 0x9b, 0x77, 0x51, 0xd0);

static UNICODE_STRING EtpProcessTraceLoggerName = RTL_CONSTANT_STRING(L"SiProcessTraceSession");
static UCHAR EtpProcessTracePropertiesBuffer[sizeof(EVENT_TRACE_PROPERTIES) + sizeof(L"SiProcessTraceSession")];
static PEVENT_TRACE_PROPERTIES EtpProcessTraceProperties = (PEVENT_TRACE_PROPERTIES)EtpProcessTracePropertiesBuffer;
static TRACEHANDLE EtpProcessTraceSessionHandle = INVALID_PROCESSTRACE_HANDLE;
static BOOLEAN EtpProcessTraceStartedSession = FALSE;
static BOOLEAN EtpProcessTraceExiting = FALSE;

static HANDLE EtProcessTraceDialogThreadHandle = NULL;
static HANDLE EtProcessTraceConsumerThreadHandle = NULL;
static HWND EtProcessTraceDialogHandle = NULL;
static PH_EVENT EtProcessTraceDialogInitializedEvent = PH_EVENT_INIT;

typedef enum _PHP_PROCESS_TRACE_EVENT_TYPE
{
    ProcessTraceStartEvent,
    ProcessTraceStopEvent
} PHP_PROCESS_TRACE_EVENT_TYPE;

typedef struct _PHP_PROCESS_TRACE_RESULT
{
    LARGE_INTEGER TimeStamp;
    PHP_PROCESS_TRACE_EVENT_TYPE EventType;
    ULONG ProcessId;
    ULONG ParentProcessId;
    PPH_STRING ImageName;
    PPH_STRING CommandLine;
} PHP_PROCESS_TRACE_RESULT, *PPHP_PROCESS_TRACE_RESULT;

typedef enum _PHP_PROCESS_TRACE_TREE_COLUMN_ITEM
{
    PROCESS_TRACE_COLUMN_TIME,
    PROCESS_TRACE_COLUMN_EVENT,
    PROCESS_TRACE_COLUMN_PID,
    PROCESS_TRACE_COLUMN_PPID,
    PROCESS_TRACE_COLUMN_NAME,
    PROCESS_TRACE_COLUMN_COMMANDLINE,
    PROCESS_TRACE_COLUMN_MAXIMUM
} PHP_PROCESS_TRACE_TREE_COLUMN_ITEM;

typedef struct _PHP_PROCESS_TRACE_TREE_NODE
{
    PH_TREENEW_NODE Node;

    ULONG64 UniqueId; // used to stabilize sorting

    LARGE_INTEGER TimeStamp;
    PHP_PROCESS_TRACE_EVENT_TYPE EventType;
    ULONG ProcessId;
    ULONG ParentProcessId;
    PPH_STRING TimeString;
    PPH_STRING ImageName;
    PPH_STRING CommandLine;
    WCHAR ProcessIdString[PH_INT32_STR_LEN_1];
    WCHAR ParentProcessIdString[PH_INT32_STR_LEN_1];

    PH_STRINGREF TextCache[PROCESS_TRACE_COLUMN_MAXIMUM];
} PHP_PROCESS_TRACE_TREE_NODE, *PPHP_PROCESS_TRACE_TREE_NODE;

typedef struct _PHP_PROCESS_TRACE_CONTEXT
{
    PH_LAYOUT_MANAGER LayoutManager;
    RECT MinimumSize;

    HWND WindowHandle;
    HWND TreeNewHandle;

    ULONG TreeNewSortColumn;
    PH_SORT_ORDER TreeNewSortOrder;
    PPH_LIST NodeList;

    PPH_LIST EventList;
    ULONG EventListAddIndex;
    PH_QUEUED_LOCK EventListLock;
    BOOLEAN EventListOverflowed;
} PHP_PROCESS_TRACE_CONTEXT, *PPHP_PROCESS_TRACE_CONTEXT;

#define SORT_FUNCTION(Column) PhpProcessTraceTreeNewCompare##Column
#define BEGIN_SORT_FUNCTION(Column) static int __cdecl PhpProcessTraceTreeNewCompare##Column( \
    _In_ void *_context, \
    _In_ const void *_elem1, \
    _In_ const void *_elem2 \
    ) \
{ \
    PPHP_PROCESS_TRACE_CONTEXT context = ((PPHP_PROCESS_TRACE_CONTEXT)_context); \
    PPHP_PROCESS_TRACE_TREE_NODE node1 = *(PPHP_PROCESS_TRACE_TREE_NODE*)_elem1; \
    PPHP_PROCESS_TRACE_TREE_NODE node2 = *(PPHP_PROCESS_TRACE_TREE_NODE*)_elem2; \
    int sortResult = 0;

#define END_SORT_FUNCTION \
    if (sortResult == 0) \
        sortResult = uintptrcmp((ULONG_PTR)node1->UniqueId, (ULONG_PTR)node2->UniqueId); \
    \
    return PhModifySort(sortResult, context->TreeNewSortOrder); \
}

BEGIN_SORT_FUNCTION(Time)
{
    sortResult = uintptrcmp(node1->TimeStamp.QuadPart, node2->TimeStamp.QuadPart);
}
END_SORT_FUNCTION

BEGIN_SORT_FUNCTION(Event)
{
    sortResult = uintcmp(node1->EventType, node2->EventType);
}
END_SORT_FUNCTION

BEGIN_SORT_FUNCTION(Pid)
{
    sortResult = uintcmp(node1->ProcessId, node2->ProcessId);
}
END_SORT_FUNCTION

BEGIN_SORT_FUNCTION(Ppid)
{
    sortResult = uintcmp(node1->ParentProcessId, node2->ParentProcessId);
}
END_SORT_FUNCTION

BEGIN_SORT_FUNCTION(Name)
{
    sortResult = PhCompareStringWithNullSortOrder(node1->ImageName, node2->ImageName, context->TreeNewSortOrder, TRUE);
}
END_SORT_FUNCTION

BEGIN_SORT_FUNCTION(CommandLine)
{
    sortResult = PhCompareStringWithNullSortOrder(node1->CommandLine, node2->CommandLine, context->TreeNewSortOrder, TRUE);
}
END_SORT_FUNCTION

_Function_class_(PH_TREENEW_CALLBACK)
BOOLEAN NTAPI PhpProcessTraceTreeNewCallback(
    _In_ HWND WindowHandle,
    _In_ PH_TREENEW_MESSAGE Message,
    _In_ PVOID Parameter1,
    _In_ PVOID Parameter2,
    _In_ PVOID Context
    )
{
    PPHP_PROCESS_TRACE_CONTEXT context = Context;
    PPHP_PROCESS_TRACE_TREE_NODE node;

    switch (Message)
    {
    case TreeNewGetChildren:
        {
            PPH_TREENEW_GET_CHILDREN getChildren = Parameter1;
            node = (PPHP_PROCESS_TRACE_TREE_NODE)getChildren->Node;

            if (!getChildren->Node)
            {
                static CONST _CoreCrtSecureSearchSortCompareFunction sortFunctions[] =
                {
                    SORT_FUNCTION(Time),
                    SORT_FUNCTION(Event),
                    SORT_FUNCTION(Pid),
                    SORT_FUNCTION(Ppid),
                    SORT_FUNCTION(Name),
                    SORT_FUNCTION(CommandLine),
                };
                _CoreCrtSecureSearchSortCompareFunction sortFunction;

                static_assert(RTL_NUMBER_OF(sortFunctions) == PROCESS_TRACE_COLUMN_MAXIMUM, "SortFunctions must equal maximum.");

                if (context->TreeNewSortColumn < PROCESS_TRACE_COLUMN_MAXIMUM)
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
            node = (PPHP_PROCESS_TRACE_TREE_NODE)getCellText->Node;

            switch (getCellText->Id)
            {
            case PROCESS_TRACE_COLUMN_TIME:
                getCellText->Text = PhGetStringRef(node->TimeString);
                break;
            case PROCESS_TRACE_COLUMN_EVENT:
                PhInitializeStringRefLongHint(&getCellText->Text,
                    node->EventType == ProcessTraceStartEvent ? L"Started" : L"Stopped");
                break;
            case PROCESS_TRACE_COLUMN_PID:
                PhInitializeStringRefLongHint(&getCellText->Text, node->ProcessIdString);
                break;
            case PROCESS_TRACE_COLUMN_PPID:
                PhInitializeStringRefLongHint(&getCellText->Text, node->ParentProcessIdString);
                break;
            case PROCESS_TRACE_COLUMN_NAME:
                getCellText->Text = PhGetStringRef(node->ImageName);
                break;
            case PROCESS_TRACE_COLUMN_COMMANDLINE:
                getCellText->Text = PhGetStringRef(node->CommandLine);
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
                    SendMessage(context->WindowHandle, WM_COMMAND, IDC_PROCTRACE_COPY, 0);
                break;
            }
        }
        return TRUE;
    case TreeNewContextMenu:
        {
            PPH_TREENEW_CONTEXT_MENU contextMenuEvent = Parameter1;

            SendMessage(
                context->WindowHandle,
                WM_COMMAND,
                WM_PH_PROCTRACE_SHOWMENU,
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
            data.DefaultSortColumn = PROCESS_TRACE_COLUMN_TIME;
            data.DefaultSortOrder = DescendingSortOrder;
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

VOID PhpProcessTraceDestroyNode(
    _In_ PPHP_PROCESS_TRACE_TREE_NODE Node
    )
{
    PhClearReference(&Node->TimeString);
    PhClearReference(&Node->ImageName);
    PhClearReference(&Node->CommandLine);

    PhFree(Node);
}

VOID PhpProcessTraceLoadSettingsTreeList(
    _Inout_ PPHP_PROCESS_TRACE_CONTEXT Context
    )
{
    PPH_STRING settings;

    settings = PhGetStringSetting(SETTING_NAME_PROCTRACE_COLUMNS);
    PhCmLoadSettings(Context->TreeNewHandle, &settings->sr);
    PhDereferenceObject(settings);
}

VOID PhpProcessTraceSaveSettingsTreeList(
    _Inout_ PPHP_PROCESS_TRACE_CONTEXT Context
    )
{
    PPH_STRING settings;

    settings = PhCmSaveSettings(Context->TreeNewHandle);
    PhSetStringSetting2(SETTING_NAME_PROCTRACE_COLUMNS, &settings->sr);
    PhDereferenceObject(settings);
}

VOID PhpProcessTraceInitializeTree(
    _Inout_ PPHP_PROCESS_TRACE_CONTEXT Context
    )
{
    Context->NodeList = PhCreateList(128);

    PhSetControlTheme(Context->TreeNewHandle, L"explorer");
    TreeNew_SetCallback(Context->TreeNewHandle, PhpProcessTraceTreeNewCallback, Context);

    PhAddTreeNewColumn(Context->TreeNewHandle, PROCESS_TRACE_COLUMN_TIME, TRUE, L"Time", 130, PH_ALIGN_LEFT, 0, 0);
    PhAddTreeNewColumn(Context->TreeNewHandle, PROCESS_TRACE_COLUMN_EVENT, TRUE, L"Event", 70, PH_ALIGN_LEFT, ULONG_MAX, 0);
    PhAddTreeNewColumn(Context->TreeNewHandle, PROCESS_TRACE_COLUMN_PID, TRUE, L"PID", 70, PH_ALIGN_LEFT, ULONG_MAX, 0);
    PhAddTreeNewColumn(Context->TreeNewHandle, PROCESS_TRACE_COLUMN_PPID, TRUE, L"Parent PID", 80, PH_ALIGN_LEFT, ULONG_MAX, 0);
    PhAddTreeNewColumn(Context->TreeNewHandle, PROCESS_TRACE_COLUMN_NAME, TRUE, L"Name", 150, PH_ALIGN_LEFT, ULONG_MAX, 0);
    PhAddTreeNewColumn(Context->TreeNewHandle, PROCESS_TRACE_COLUMN_COMMANDLINE, TRUE, L"Command Line", 300, PH_ALIGN_LEFT, ULONG_MAX, 0);

    TreeNew_SetSort(Context->TreeNewHandle, PROCESS_TRACE_COLUMN_TIME, DescendingSortOrder);

    PhpProcessTraceLoadSettingsTreeList(Context);
}

VOID PhpProcessTraceDeleteTree(
    _Inout_ PPHP_PROCESS_TRACE_CONTEXT Context
    )
{
    for (ULONG i = 0; i < Context->NodeList->Count; i++)
        PhpProcessTraceDestroyNode(Context->NodeList->Items[i]);

    PhClearList(Context->NodeList);

    if (Context->NodeList)
        PhDereferenceObject(Context->NodeList);
}

PPHP_PROCESS_TRACE_TREE_NODE PhpGetSelectedProcessTraceNodes(
    _In_ PPHP_PROCESS_TRACE_CONTEXT Context,
    _Out_ PULONG NumberOfSelectedNodes
    )
{
    PPHP_PROCESS_TRACE_TREE_NODE selectedNode = NULL;
    ULONG numberOfSelectedNodes = 0;

    for (ULONG i = 0; i < Context->NodeList->Count; i++)
    {
        PPHP_PROCESS_TRACE_TREE_NODE node = Context->NodeList->Items[i];

        if (node->Node.Selected)
        {
            selectedNode = node;
            numberOfSelectedNodes++;
        }
    }

    *NumberOfSelectedNodes = numberOfSelectedNodes;

    return selectedNode;
}

VOID PhpProcessTraceFreeResult(
    _In_ PPHP_PROCESS_TRACE_RESULT Result
    )
{
    PhClearReference(&Result->ImageName);
    PhClearReference(&Result->CommandLine);

    PhFree(Result);
}

VOID PhpProcessTraceAddResultEntries(
    _In_ PPHP_PROCESS_TRACE_CONTEXT Context
    )
{
    ULONG i;

    PhAcquireQueuedLockExclusive(&Context->EventListLock);

    if (Context->EventList->Count == 0 || Context->EventListAddIndex == Context->EventList->Count)
    {
        PhReleaseQueuedLockExclusive(&Context->EventListLock);
        return;
    }

    TreeNew_SetRedraw(Context->TreeNewHandle, FALSE);

    for (i = Context->EventListAddIndex; i < Context->EventList->Count; i++)
    {
        static ULONG64 nextUniqueId = 0;
        PPHP_PROCESS_TRACE_RESULT result = Context->EventList->Items[i];
        PPHP_PROCESS_TRACE_TREE_NODE node;
        SYSTEMTIME systemTime;

        node = PhAllocate(sizeof(PHP_PROCESS_TRACE_TREE_NODE));
        memset(node, 0, sizeof(PHP_PROCESS_TRACE_TREE_NODE));
        PhInitializeTreeNewNode(&node->Node);

        memset(node->TextCache, 0, sizeof(PH_STRINGREF) * PROCESS_TRACE_COLUMN_MAXIMUM);
        node->Node.TextCache = node->TextCache;
        node->Node.TextCacheSize = PROCESS_TRACE_COLUMN_MAXIMUM;

        node->TimeStamp = result->TimeStamp;
        node->EventType = result->EventType;
        node->ProcessId = result->ProcessId;
        node->ParentProcessId = result->ParentProcessId;

        PhLargeIntegerToLocalSystemTime(&systemTime, &result->TimeStamp);
        node->TimeString = PhFormatDateTime(&systemTime);
        PhSetReference(&node->ImageName, result->ImageName);
        PhSetReference(&node->CommandLine, result->CommandLine);
        PhPrintUInt32(node->ProcessIdString, result->ProcessId);
        PhPrintUInt32(node->ParentProcessIdString, result->ParentProcessId);

        node->UniqueId = ++nextUniqueId;

        PhAddItemList(Context->NodeList, node);
        PhpProcessTraceFreeResult(result);
    }

    TreeNew_NodesStructured(Context->TreeNewHandle);
    TreeNew_SetRedraw(Context->TreeNewHandle, TRUE);

    Context->EventListAddIndex = i;

    PhReleaseQueuedLockExclusive(&Context->EventListLock);
}

VOID PhpProcessTraceClearResults(
    _In_ PPHP_PROCESS_TRACE_CONTEXT Context
    )
{
    PhAcquireQueuedLockExclusive(&Context->EventListLock);

    TreeNew_SetRedraw(Context->TreeNewHandle, FALSE);

    for (ULONG i = Context->EventListAddIndex; i < Context->EventList->Count; i++)
        PhpProcessTraceFreeResult(Context->EventList->Items[i]);

    PhClearList(Context->EventList);
    Context->EventListAddIndex = 0;
    Context->EventListOverflowed = FALSE;

    for (ULONG i = 0; i < Context->NodeList->Count; i++)
        PhpProcessTraceDestroyNode(Context->NodeList->Items[i]);

    PhClearList(Context->NodeList);

    TreeNew_NodesStructured(Context->TreeNewHandle);
    TreeNew_SetRedraw(Context->TreeNewHandle, TRUE);

    PhReleaseQueuedLockExclusive(&Context->EventListLock);
}

static BOOLEAN EtpMatchPropertyName(
    _In_ PCWSTR PropertyName,
    _In_ PCWSTR Candidate
    )
{
    return _wcsicmp(PropertyName, Candidate) == 0;
}

static BOOLEAN EtpQueryEventPropertyUint32(
    _In_ PEVENT_RECORD EventRecord,
    _In_ PTRACE_EVENT_INFO Info,
    _In_ ULONG Index,
    _Out_ PULONG Value
    )
{
    PROPERTY_DATA_DESCRIPTOR descriptor;
    UCHAR buffer[sizeof(ULONG)];
    ULONG status;
    ULONG size;

    memset(&descriptor, 0, sizeof(PROPERTY_DATA_DESCRIPTOR));
    descriptor.PropertyName = (ULONGLONG)((PBYTE)Info + Info->EventPropertyInfoArray[Index].nonStructProperty.NameOffset);
    descriptor.ArrayIndex = ULONG_MAX;

    size = sizeof(buffer);
    status = TdhGetProperty(
        EventRecord,
        0,
        NULL,
        1,
        &descriptor,
        size,
        buffer
        );

    if (status != ERROR_SUCCESS || size != sizeof(ULONG))
        return FALSE;

    *Value = *(PULONG)buffer;
    return TRUE;
}

static PPH_STRING EtpQueryEventPropertyString(
    _In_ PEVENT_RECORD EventRecord,
    _In_ PTRACE_EVENT_INFO Info,
    _In_ ULONG Index,
    _In_ USHORT InType
    )
{
    PROPERTY_DATA_DESCRIPTOR descriptor;
    PVOID buffer;
    ULONG status;
    ULONG size;

    memset(&descriptor, 0, sizeof(PROPERTY_DATA_DESCRIPTOR));
    descriptor.PropertyName = (ULONGLONG)((PBYTE)Info + Info->EventPropertyInfoArray[Index].nonStructProperty.NameOffset);
    descriptor.ArrayIndex = ULONG_MAX;

    status = TdhGetPropertySize(
        EventRecord,
        0,
        NULL,
        1,
        &descriptor,
        &size
        );

    if (status != ERROR_SUCCESS || size == 0 || size > 0x8000)
        return NULL;

    buffer = PhAllocate(size);

    status = TdhGetProperty(
        EventRecord,
        0,
        NULL,
        1,
        &descriptor,
        size,
        (PBYTE)buffer
        );

    if (status == ERROR_SUCCESS)
    {
        PPH_STRING string;

        // The payload for these types is null terminated. (TDH)
        if (InType == TDH_INTYPE_ANSISTRING)
            string = PhConvertUtf8ToUtf16((PSTR)buffer);
        else if (InType == TDH_INTYPE_UNICODESTRING)
            string = PhCreateString((PWSTR)buffer);
        else
            string = NULL;

        PhFree(buffer);
        return string;
    }

    PhFree(buffer);
    return NULL;
}

VOID NTAPI EtpProcessTraceEventCallback(
    _In_ PEVENT_RECORD EventRecord
    )
{
    PPHP_PROCESS_TRACE_CONTEXT context;
    PTRACE_EVENT_INFO info = NULL;
    ULONG infoSize = 0;
    ULONG status;
    PHP_PROCESS_TRACE_EVENT_TYPE eventType;
    ULONG processId = 0;
    ULONG parentProcessId = 0;
    PPH_STRING imageName = NULL;
    PPH_STRING commandLine = NULL;
    PHP_PROCESS_TRACE_RESULT result;

    if (
        !EtProcessTraceDialogHandle ||
        !IsEqualGUID(&EventRecord->EventHeader.ProviderId, &EtProcessTraceProviderGuid) ||
        (EventRecord->EventHeader.EventDescriptor.Id != 1 &&
        EventRecord->EventHeader.EventDescriptor.Id != 2)
        )
    {
        return;
    }

    context = PhGetWindowContext(
        EtProcessTraceDialogHandle,
        PH_WINDOW_CONTEXT_DEFAULT
        );

    if (!context)
        return;

    eventType = EventRecord->EventHeader.EventDescriptor.Id == 1 ? ProcessTraceStartEvent : ProcessTraceStopEvent;

    status = TdhGetEventInformation(EventRecord, 0, NULL, NULL, &infoSize);

    if (status != ERROR_INSUFFICIENT_BUFFER)
        return;

    info = PhAllocate(infoSize);

    status = TdhGetEventInformation(EventRecord, 0, NULL, info, &infoSize);

    if (status == ERROR_SUCCESS)
    {
        for (ULONG i = 0; i < info->TopLevelPropertyCount; i++)
        {
            PEVENT_PROPERTY_INFO property = &info->EventPropertyInfoArray[i];

            if (FlagOn(property->Flags, PropertyStruct))
                continue;

            {
                PCWSTR propertyName = (PCWSTR)((PBYTE)info + property->nonStructProperty.NameOffset);

                if (
                    EtpMatchPropertyName(propertyName, L"NewProcessId") ||
                    EtpMatchPropertyName(propertyName, L"ProcessId") ||
                    EtpMatchPropertyName(propertyName, L"ProcessID")
                    )
                {
                    ULONG value;

                    if (EtpQueryEventPropertyUint32(EventRecord, info, i, &value))
                        processId = value;
                }
                else if (
                    EtpMatchPropertyName(propertyName, L"ParentProcessId") ||
                    EtpMatchPropertyName(propertyName, L"CreatorProcessId")
                    )
                {
                    ULONG value;

                    if (EtpQueryEventPropertyUint32(EventRecord, info, i, &value))
                        parentProcessId = value;
                }
                else if (
                    EtpMatchPropertyName(propertyName, L"ImageName") ||
                    EtpMatchPropertyName(propertyName, L"ImageFileName")
                    )
                {
                    if (!imageName)
                        imageName = EtpQueryEventPropertyString(EventRecord, info, i, property->nonStructType.InType);
                }
                else if (EtpMatchPropertyName(propertyName, L"CommandLine"))
                {
                    commandLine = EtpQueryEventPropertyString(EventRecord, info, i, property->nonStructType.InType);
                }
            }
        }
    }

    PhFree(info);

    {
        BOOLEAN overflowed;

        result.TimeStamp = EventRecord->EventHeader.TimeStamp;
        result.EventType = eventType;
        result.ProcessId = processId;
        result.ParentProcessId = parentProcessId;
        result.ImageName = imageName;
        result.CommandLine = commandLine;

        // Take ownership of the strings on success. (dmex)
        imageName = NULL;
        commandLine = NULL;

        PhAcquireQueuedLockExclusive(&context->EventListLock);

        if (context->EventListOverflowed || context->EventList->Count >= PHP_PROCTRACE_MAX_EVENTS)
        {
            context->EventListOverflowed = TRUE;
            overflowed = TRUE;
        }
        else
        {
            overflowed = FALSE;
            PhAddItemList(context->EventList, PhAllocateCopy(&result, sizeof(PHP_PROCESS_TRACE_RESULT)));
        }

        PhReleaseQueuedLockExclusive(&context->EventListLock);

        if (overflowed)
        {
            if (result.ImageName)
                PhDereferenceObject(result.ImageName);
            if (result.CommandLine)
                PhDereferenceObject(result.CommandLine);
        }
    }
}

ULONG NTAPI EtpProcessTraceBufferCallback(
    _In_ PEVENT_TRACE_LOGFILE Buffer
    )
{
    return !EtpProcessTraceExiting;
}

ULONG EtpStartProcessTraceSession(
    VOID
    )
{
    TRACEHANDLE traceHandle = INVALID_PROCESSTRACE_HANDLE;
    ULONG bufferSize;
    ULONG status;

    bufferSize = sizeof(EVENT_TRACE_PROPERTIES) + EtpProcessTraceLoggerName.Length + sizeof(UNICODE_NULL);

    memset(EtpProcessTraceProperties, 0, sizeof(EtpProcessTracePropertiesBuffer));
    EtpProcessTraceProperties->Wnode.BufferSize = bufferSize;
    EtpProcessTraceProperties->Wnode.Guid = EtProcessTraceSessionGuid;
    EtpProcessTraceProperties->Wnode.ClientContext = 1; // QPC clocks
    EtpProcessTraceProperties->Wnode.Flags = WNODE_FLAG_TRACED_GUID;
    EtpProcessTraceProperties->MinimumBuffers = 1;
    EtpProcessTraceProperties->LogFileMode = EVENT_TRACE_REAL_TIME_MODE;
    EtpProcessTraceProperties->FlushTimer = 1;
    EtpProcessTraceProperties->LogFileNameOffset = 0;
    EtpProcessTraceProperties->LoggerNameOffset = sizeof(EVENT_TRACE_PROPERTIES);

    // Get the existing session handle.
    status = ControlTrace(
        0,
        EtpProcessTraceLoggerName.Buffer,
        EtpProcessTraceProperties,
        EVENT_TRACE_CONTROL_QUERY
        );

    if (status == ERROR_SUCCESS)
    {
        traceHandle = EtpProcessTraceProperties->Wnode.HistoricalContext;
        EtpProcessTraceStartedSession = FALSE; // Started by another program. (dmex)
    }
    else
    {
        status = StartTrace(
            &traceHandle,
            EtpProcessTraceLoggerName.Buffer,
            EtpProcessTraceProperties
            );

        EtpProcessTraceStartedSession = status == ERROR_SUCCESS;

        if (status == ERROR_ALREADY_EXISTS)
        {
            // A previous instance didn't stop the session. Stop and retry. (dmex)
            ControlTrace(0, EtpProcessTraceLoggerName.Buffer, EtpProcessTraceProperties, EVENT_TRACE_CONTROL_STOP);

            status = StartTrace(
                &traceHandle,
                EtpProcessTraceLoggerName.Buffer,
                EtpProcessTraceProperties
                );

            EtpProcessTraceStartedSession = status == ERROR_SUCCESS;
        }
    }

    if (status == ERROR_SUCCESS)
        EtpProcessTraceSessionHandle = traceHandle;
    else
        EtpProcessTraceSessionHandle = INVALID_PROCESSTRACE_HANDLE; // StartTrace set the handle 0 on failure. (dmex)

    return status;
}

ULONG EtpStopProcessTraceSession(
    VOID
    )
{
    if (EtpProcessTraceStartedSession)
    {
        // If we have a session handle, we use that instead of the logger name.
        EtpProcessTraceProperties->LogFileNameOffset = 0; // make sure it is 0, otherwise ControlTrace crashes

        return ControlTrace(
            EtpProcessTraceSessionHandle,
            NULL,
            EtpProcessTraceProperties,
            EVENT_TRACE_CONTROL_STOP
            );
    }

    return ERROR_SUCCESS;
}

_Function_class_(USER_THREAD_START_ROUTINE)
NTSTATUS EtpProcessTraceConsumerThreadStart(
    _In_ PVOID Parameter
    )
{
    EVENT_TRACE_LOGFILE logFile;
    TRACEHANDLE traceHandle;
    ULONG result;
    BOOLEAN enabledProvider = FALSE;

    PhSetThreadName(NtCurrentThread(), L"SiProcessTraceThread");

    memset(&logFile, 0, sizeof(EVENT_TRACE_LOGFILE));
    logFile.LoggerName = EtpProcessTraceLoggerName.Buffer;
    logFile.ProcessTraceMode = PROCESS_TRACE_MODE_REAL_TIME | PROCESS_TRACE_MODE_EVENT_RECORD;
    logFile.BufferCallback = EtpProcessTraceBufferCallback;
    logFile.EventRecordCallback = EtpProcessTraceEventCallback;

    traceHandle = OpenTrace(&logFile);

    if (traceHandle == INVALID_PROCESSTRACE_HANDLE)
        return STATUS_UNSUCCESSFUL;

    // Note: The provider must only be enabled after we've opened the trace otherwise
    // the provider will generate eventlog warnings about no realtime listeners. (dmex)
    if (EtpProcessTraceSessionHandle != INVALID_PROCESSTRACE_HANDLE)
    {
        result = EnableTraceEx2(
            EtpProcessTraceSessionHandle,
            &EtProcessTraceProviderGuid,
            EVENT_CONTROL_CODE_ENABLE_PROVIDER,
            TRACE_LEVEL_INFORMATION,
            PROCESS_TRACE_KEYWORD_PROCESS,
            0,
            INFINITE,
            NULL
            );

        if (result == ERROR_SUCCESS)
            enabledProvider = TRUE;
    }

    result = ProcessTrace(&traceHandle, 1, NULL, NULL);

    if (enabledProvider && EtpProcessTraceSessionHandle != INVALID_PROCESSTRACE_HANDLE)
    {
        EnableTraceEx2(
            EtpProcessTraceSessionHandle,
            &EtProcessTraceProviderGuid,
            EVENT_CONTROL_CODE_DISABLE_PROVIDER,
            0,
            0,
            0,
            0,
            NULL
            );
    }

    CloseTrace(traceHandle);

    return STATUS_SUCCESS;
}

INT_PTR CALLBACK EtpProcessTraceDlgProc(
    _In_ HWND hwndDlg,
    _In_ UINT uMsg,
    _In_ WPARAM wParam,
    _In_ LPARAM lParam
    )
{
    PPHP_PROCESS_TRACE_CONTEXT context = NULL;

    if (uMsg == WM_INITDIALOG)
    {
        context = PhAllocateZero(sizeof(PHP_PROCESS_TRACE_CONTEXT));
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
            context->TreeNewHandle = GetDlgItem(hwndDlg, IDC_PROCTRACE_TREE);

            PhSetApplicationWindowIcon(hwndDlg);

            PhpProcessTraceInitializeTree(context);

            context->EventList = PhCreateList(128);
            context->EventListAddIndex = 0;

            PhInitializeLayoutManager(&context->LayoutManager, hwndDlg);
            PhAddLayoutItem(&context->LayoutManager, context->TreeNewHandle, NULL, PH_ANCHOR_ALL);
            PhAddLayoutItem(&context->LayoutManager, GetDlgItem(hwndDlg, IDC_PROCTRACE_CLEAR), NULL, PH_ANCHOR_BOTTOM | PH_ANCHOR_LEFT);
            PhAddLayoutItem(&context->LayoutManager, GetDlgItem(hwndDlg, IDCANCEL), NULL, PH_ANCHOR_BOTTOM | PH_ANCHOR_RIGHT);

            context->MinimumSize.left = 0;
            context->MinimumSize.top = 0;
            context->MinimumSize.right = 400;
            context->MinimumSize.bottom = 200;
            MapDialogRect(hwndDlg, &context->MinimumSize);

            if (PhValidWindowPlacementFromSetting(SETTING_NAME_PROCTRACE_WINDOW_POSITION))
                PhLoadWindowPlacementFromSetting(SETTING_NAME_PROCTRACE_WINDOW_POSITION, SETTING_NAME_PROCTRACE_WINDOW_SIZE, hwndDlg);
            else
                PhCenterWindow(hwndDlg, (HWND)lParam);

            PhRegisterDialog(hwndDlg);
            PhInitializeWindowTheme(hwndDlg, !!PhGetIntegerSetting(SETTING_ENABLE_THEME_SUPPORT));

            EtProcessTraceDialogHandle = hwndDlg;

            // Start consuming events.
            EtpStartProcessTraceSession();

            PhCreateThreadEx(&EtProcessTraceConsumerThreadHandle, EtpProcessTraceConsumerThreadStart, NULL);

            PhSetTimer(hwndDlg, PH_WINDOW_TIMER_DEFAULT, 500, NULL);
        }
        break;
    case WM_DESTROY:
        {
            PhKillTimer(hwndDlg, PH_WINDOW_TIMER_DEFAULT);

            EtProcessTraceDialogHandle = NULL;
            EtpProcessTraceExiting = TRUE;

            // Stop the session to unblock ProcessTrace. (dmex)
            EtpStopProcessTraceSession();

            if (EtProcessTraceConsumerThreadHandle)
            {
                NtWaitForSingleObject(EtProcessTraceConsumerThreadHandle, FALSE, NULL);
                NtClose(EtProcessTraceConsumerThreadHandle);
                EtProcessTraceConsumerThreadHandle = NULL;
            }

            EtpProcessTraceExiting = FALSE;

            PhSaveWindowPlacementToSetting(SETTING_NAME_PROCTRACE_WINDOW_POSITION, SETTING_NAME_PROCTRACE_WINDOW_SIZE, hwndDlg);
            PhpProcessTraceSaveSettingsTreeList(context);

            PhpProcessTraceDeleteTree(context);
            PhDeleteLayoutManager(&context->LayoutManager);

            if (context->EventList)
            {
                for (ULONG i = 0; i < context->EventList->Count; i++)
                    PhpProcessTraceFreeResult(context->EventList->Items[i]);

                PhDereferenceObject(context->EventList);
            }

            PhRemoveWindowContext(hwndDlg, PH_WINDOW_CONTEXT_DEFAULT);
            PhFree(context);

            PostQuitMessage(0);
        }
        break;
    case WM_PH_PROCTRACE_SHOWDIALOG:
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
                {
                    DestroyWindow(hwndDlg);
                }
                break;
            case IDC_PROCTRACE_CLEAR:
                {
                    PhpProcessTraceClearResults(context);
                }
                break;
            case IDC_PROCTRACE_COPY:
                {
                    PPH_STRING text;

                    text = PhGetTreeNewText(context->TreeNewHandle, 0);
                    PhSetClipboardString(context->TreeNewHandle, &text->sr);
                    PhDereferenceObject(text);
                }
                break;
            case WM_PH_PROCTRACE_SHOWMENU:
                {
                    PPH_TREENEW_CONTEXT_MENU contextMenuEvent = (PPH_TREENEW_CONTEXT_MENU)lParam;
                    PPH_EMENU menu;
                    PPH_EMENU_ITEM selectedItem;
                    ULONG numberOfSelectedNodes = 0;

                    if (!PhpGetSelectedProcessTraceNodes(context, &numberOfSelectedNodes))
                        break;

                    menu = PhCreateEMenu();
                    PhInsertEMenuItem(menu, PhCreateEMenuItem(0, IDC_PROCTRACE_GOTOPROCESS, L"&Go to process", NULL, NULL), ULONG_MAX);
                    PhInsertEMenuItem(menu, PhCreateEMenuSeparator(), ULONG_MAX);
                    PhInsertEMenuItem(menu, PhCreateEMenuItem(0, IDC_PROCTRACE_COPY, L"&Copy\bCtrl+C", NULL, NULL), ULONG_MAX);
                    PhInsertCopyCellEMenuItem(menu, IDC_PROCTRACE_COPY, context->TreeNewHandle, contextMenuEvent->Column);
                    PhSetFlagsEMenuItem(menu, IDC_PROCTRACE_GOTOPROCESS, PH_EMENU_DEFAULT, PH_EMENU_DEFAULT);

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
                        PhHandleCopyCellEMenuItem(selectedItem);
                    }

                    PhDestroyEMenu(menu);
                }
                break;
            case IDC_PROCTRACE_GOTOPROCESS:
                {
                    ULONG numberOfSelectedNodes = 0;
                    PPHP_PROCESS_TRACE_TREE_NODE node;

                    if (node = PhpGetSelectedProcessTraceNodes(context, &numberOfSelectedNodes))
                    {
                        PPH_PROCESS_ITEM processItem;

                        if (processItem = PhReferenceProcessItem(node->ProcessId))
                        {
                            PhShowProcessProperties(processItem);
                            PhDereferenceObject(processItem);
                        }
                        else
                        {
                            PhShowStatus(hwndDlg, L"The process does not exist.", STATUS_INVALID_CID, 0);
                        }
                    }
                }
                break;
            }
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
    case WM_DPICHANGED:
        {
            PhLayoutManagerUpdate(&context->LayoutManager, LOWORD(wParam));
            PhLayoutManagerLayout(&context->LayoutManager);

            context->MinimumSize.left = 0;
            context->MinimumSize.top = 0;
            context->MinimumSize.right = 400;
            context->MinimumSize.bottom = 200;
            MapDialogRect(hwndDlg, &context->MinimumSize);
        }
        break;
    case WM_TIMER:
        {
            switch (wParam)
            {
            case PH_WINDOW_TIMER_DEFAULT:
                {
                    PhpProcessTraceAddResultEntries(context);

                    if (context->EventListOverflowed)
                    {
                        context->EventListOverflowed = FALSE;
                        PhShowInformation(hwndDlg, L"The maximum number of events (%lu) has been reached.", PHP_PROCTRACE_MAX_EVENTS);
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

_Function_class_(USER_THREAD_START_ROUTINE)
NTSTATUS EtpProcessTraceDialogThreadStart(
    _In_ PVOID Parameter
    )
{
    BOOL result;
    MSG message;
    PH_AUTO_POOL autoPool;

    PhInitializeAutoPool(&autoPool);

    EtProcessTraceDialogHandle = PhCreateDialog(
        NtCurrentImageBase(),
        MAKEINTRESOURCE(IDD_PROCTRACE),
        NULL,
        EtpProcessTraceDlgProc,
        Parameter
        );

    PhSetEvent(&EtProcessTraceDialogInitializedEvent);

    if (EtProcessTraceDialogHandle)
    {
        PostMessage(EtProcessTraceDialogHandle, WM_PH_PROCTRACE_SHOWDIALOG, 0, 0);

        while (result = GetMessage(&message, NULL, 0, 0))
        {
            if (result == INT_ERROR)
                break;

            if (!IsDialogMessage(EtProcessTraceDialogHandle, &message))
            {
                TranslateMessage(&message);
                DispatchMessage(&message);
            }

            PhDrainAutoPool(&autoPool);
        }
    }

    PhDeleteAutoPool(&autoPool);
    PhResetEvent(&EtProcessTraceDialogInitializedEvent);

    if (EtProcessTraceDialogThreadHandle)
    {
        NtClose(EtProcessTraceDialogThreadHandle);
        EtProcessTraceDialogThreadHandle = NULL;
    }

    return STATUS_SUCCESS;
}

VOID EtShowProcessTraceDialog(
    _In_ HWND ParentWindowHandle
    )
{
    if (!PhGetOwnTokenAttributes().Elevated)
    {
        PhShowError2(
            ParentWindowHandle,
            L"Unable to start the process trace session.",
            L"%s",
            L"Make sure System Informer is running with administrative privileges."
            );
        return;
    }

    if (!EtProcessTraceDialogThreadHandle)
    {
        if (!NT_SUCCESS(PhCreateThreadEx(&EtProcessTraceDialogThreadHandle, EtpProcessTraceDialogThreadStart, ParentWindowHandle)))
        {
            PhShowError2(ParentWindowHandle, L"Unable to create the window.", L"%s", L"");
            return;
        }

        PhWaitForEvent(&EtProcessTraceDialogInitializedEvent, NULL);
    }

    PostMessage(EtProcessTraceDialogHandle, WM_PH_PROCTRACE_SHOWDIALOG, 0, 0);
}
