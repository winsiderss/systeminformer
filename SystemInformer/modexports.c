/*
 * Process-wide module exports search.
 *
 * Enumerates all modules loaded into a process, parses each module's
 * PE export table and displays the results in a searchable list.
 */

#include <phapp.h>
#include <cpysave.h>
#include <emenu.h>
#include <mapimg.h>
#include <modprv.h>
#include <phsettings.h>
#include <settings.h>

#define WM_PH_EXPORT_SHOWDIALOG (WM_APP + 901)
#define WM_PH_EXPORT_FINISHED (WM_APP + 902)
#define WM_PH_EXPORT_SHOWMENU (WM_APP + 903)

#define PHP_MODEXPORT_COPY (WM_APP + 906)
#define PHP_MODEXPORT_LOCATION (WM_APP + 907)
#define PHP_MODEXPORT_PROPERTIES (WM_APP + 908)

static HANDLE PhpModuleExportsThreadHandle = NULL;
static HWND PhpModuleExportsWindowHandle = NULL;
static PH_EVENT PhpModuleExportsInitializedEvent = PH_EVENT_INIT;

typedef struct _PHP_MODULE_EXPORT_RESULT
{
    PPH_STRING ModuleName;
    PPH_STRING FunctionName;
    PPH_STRING ForwardedName;
    PPH_STRING OrdinalString;
    PPH_STRING FileName;
    ULONG Ordinal;
    PVOID Address;
} PHP_MODULE_EXPORT_RESULT, *PPHP_MODULE_EXPORT_RESULT;

typedef enum _PHP_MODULE_EXPORT_TREE_COLUMN_ITEM
{
    PHP_MODEXPORT_TREE_COLUMN_FUNCTION,
    PHP_MODEXPORT_TREE_COLUMN_MODULE,
    PHP_MODEXPORT_TREE_COLUMN_ORDINAL,
    PHP_MODEXPORT_TREE_COLUMN_ADDRESS,
    PHP_MODEXPORT_TREE_COLUMN_FORWARDNAME,
    PHP_MODEXPORT_TREE_COLUMN_MAXIMUM
} PHP_MODULE_EXPORT_TREE_COLUMN_ITEM;

typedef struct _PHP_MODULE_EXPORT_TREE_NODE
{
    PH_TREENEW_NODE Node;

    ULONG64 UniqueId; // used to stabilize sorting

    PPH_STRING ModuleName;
    PPH_STRING FunctionName;
    PPH_STRING ForwardedName;
    PPH_STRING OrdinalString;
    PPH_STRING FileName;
    PVOID Address;
    WCHAR AddressString[PH_PTR_STR_LEN_1];

    PH_STRINGREF TextCache[PHP_MODEXPORT_TREE_COLUMN_MAXIMUM];
} PHP_MODULE_EXPORT_TREE_NODE, *PPHP_MODULE_EXPORT_TREE_NODE;

typedef struct _PHP_MODULE_EXPORT_CONTEXT
{
    PH_LAYOUT_MANAGER LayoutManager;
    RECT MinimumSize;

    HWND WindowHandle;
    HWND TreeNewHandle;
    HWND SearchWindowHandle;
    HWND ParentWindowHandle;
    PPH_STRING WindowText;

    ULONG TreeNewSortColumn;
    PH_SORT_ORDER TreeNewSortOrder;
    PPH_LIST NodeList;
    PH_TN_FILTER_SUPPORT FilterSupport;

    HANDLE ProcessId;
    PPH_STRING ProcessName;

    HANDLE SearchThreadHandle;
    BOOLEAN SearchStop;

    ULONG_PTR SearchMatchHandle;
    PPH_LIST SearchResults;
    ULONG SearchResultsAddIndex;
    PH_QUEUED_LOCK SearchResultsLock;
} PHP_MODULE_EXPORT_CONTEXT, *PPHP_MODULE_EXPORT_CONTEXT;

typedef struct _PHP_MODULE_EXPORT_LAUNCH_PARAM
{
    HWND ParentWindowHandle;
    HANDLE ProcessId;
    PPH_STRING ProcessName;
} PHP_MODULE_EXPORT_LAUNCH_PARAM, *PPHP_MODULE_EXPORT_LAUNCH_PARAM;

#define SORT_FUNCTION(Column) PhpModuleExportTreeNewCompare##Column
#define BEGIN_SORT_FUNCTION(Column) static int __cdecl PhpModuleExportTreeNewCompare##Column( \
    _In_ void *_context, \
    _In_ const void *_elem1, \
    _In_ const void *_elem2 \
    ) \
{ \
    PPHP_MODULE_EXPORT_CONTEXT context = ((PPHP_MODULE_EXPORT_CONTEXT)_context); \
    PPHP_MODULE_EXPORT_TREE_NODE node1 = *(PPHP_MODULE_EXPORT_TREE_NODE*)_elem1; \
    PPHP_MODULE_EXPORT_TREE_NODE node2 = *(PPHP_MODULE_EXPORT_TREE_NODE*)_elem2; \
    int sortResult = 0;

#define END_SORT_FUNCTION \
    if (sortResult == 0) \
        sortResult = uintptrcmp((ULONG_PTR)node1->UniqueId, (ULONG_PTR)node2->UniqueId); \
    \
    return PhModifySort(sortResult, context->TreeNewSortOrder); \
}

BEGIN_SORT_FUNCTION(Function)
{
    sortResult = PhCompareStringWithNullSortOrder(node1->FunctionName, node2->FunctionName, context->TreeNewSortOrder, TRUE);
}
END_SORT_FUNCTION

BEGIN_SORT_FUNCTION(Module)
{
    sortResult = PhCompareStringWithNullSortOrder(node1->ModuleName, node2->ModuleName, context->TreeNewSortOrder, TRUE);
}
END_SORT_FUNCTION

BEGIN_SORT_FUNCTION(Ordinal)
{
    sortResult = uintptrcmp((ULONG_PTR)node1->Ordinal, (ULONG_PTR)node2->Ordinal);
}
END_SORT_FUNCTION

BEGIN_SORT_FUNCTION(Address)
{
    sortResult = uintptrcmp((ULONG_PTR)node1->Address, (ULONG_PTR)node2->Address);
}
END_SORT_FUNCTION

BEGIN_SORT_FUNCTION(ForwardName)
{
    sortResult = PhCompareStringWithNullSortOrder(node1->ForwardedName, node2->ForwardedName, context->TreeNewSortOrder, TRUE);
}
END_SORT_FUNCTION

VOID PhpModuleExportsLoadSettingsTreeList(
    _Inout_ PPHP_MODULE_EXPORT_CONTEXT Context
    )
{
    PPH_STRING settings;

    settings = PhGetStringSetting(SETTING_MODEXPORTS_TREE_LIST_COLUMNS);
    PhCmLoadSettings(Context->TreeNewHandle, &settings->sr);
    PhDereferenceObject(settings);
}

VOID PhpModuleExportsSaveSettingsTreeList(
    _Inout_ PPHP_MODULE_EXPORT_CONTEXT Context
    )
{
    PPH_STRING settings;

    settings = PhCmSaveSettings(Context->TreeNewHandle);
    PhSetStringSetting2(SETTING_MODEXPORTS_TREE_LIST_COLUMNS, &settings->sr);
    PhDereferenceObject(settings);
}

_Function_class_(PH_TYPE_DELETE_PROCEDURE)
VOID PhpModuleExportsDeleteProcedure(
    _In_ PVOID Object,
    _In_ ULONG Flags
    )
{
    PPHP_MODULE_EXPORT_CONTEXT context = Object;

    if (context->SearchResults)
    {
        for (ULONG i = 0; i < context->SearchResults->Count; i++)
        {
            PPHP_MODULE_EXPORT_RESULT result = context->SearchResults->Items[i];

            PhClearReference(&result->ModuleName);
            PhClearReference(&result->FunctionName);
            PhClearReference(&result->ForwardedName);
            PhClearReference(&result->OrdinalString);
            PhClearReference(&result->FileName);

            PhFree(result);
        }

        PhClearList(context->SearchResults);
        PhClearReference(&context->SearchResults);
    }

    PhClearReference(&context->ProcessName);
}

PPHP_MODULE_EXPORT_CONTEXT PhpCreateModuleExportsContext(
    VOID
    )
{
    static PH_INITONCE initOnce = PH_INITONCE_INIT;
    static PPH_OBJECT_TYPE PhpModuleExportsItemType = NULL;
    PPHP_MODULE_EXPORT_CONTEXT context;

    if (PhBeginInitOnce(&initOnce))
    {
        PhpModuleExportsItemType = PhCreateObjectType(L"ModuleExportsContext", 0, PhpModuleExportsDeleteProcedure);
        PhEndInitOnce(&initOnce);
    }

    context = PhCreateObject(sizeof(PHP_MODULE_EXPORT_CONTEXT), PhpModuleExportsItemType);
    memset(context, 0, sizeof(PHP_MODULE_EXPORT_CONTEXT));

    return context;
}

_Function_class_(PH_SEARCHCONTROL_CALLBACK)
VOID NTAPI PhpModuleExportsSearchControlCallback(
    _In_ ULONG_PTR MatchHandle,
    _In_opt_ PVOID Context
    )
{
    PPHP_MODULE_EXPORT_CONTEXT context = Context;

    assert(context);

    context->SearchMatchHandle = MatchHandle;

    PhApplyTreeNewFilters(&context->FilterSupport);
}

BOOLEAN NTAPI PhpModuleExportsTreeFilterCallback(
    _In_ PPH_TREENEW_NODE Node,
    _In_opt_ PVOID Context
    )
{
    PPHP_MODULE_EXPORT_CONTEXT context = Context;
    PPHP_MODULE_EXPORT_TREE_NODE node = (PPHP_MODULE_EXPORT_TREE_NODE)Node;

    if (!context->SearchMatchHandle)
        return TRUE;

    if (node->FunctionName)
    {
        if (PhSearchControlMatch(context->SearchMatchHandle, &node->FunctionName->sr))
            return TRUE;
    }

    if (node->ModuleName)
    {
        if (PhSearchControlMatch(context->SearchMatchHandle, &node->ModuleName->sr))
            return TRUE;
    }

    if (node->ForwardedName)
    {
        if (PhSearchControlMatch(context->SearchMatchHandle, &node->ForwardedName->sr))
            return TRUE;
    }

    return FALSE;
}

VOID PhpDestroyModuleExportNode(
    _In_ PPHP_MODULE_EXPORT_TREE_NODE Node
    )
{
    PhClearReference(&Node->ModuleName);
    PhClearReference(&Node->FunctionName);
    PhClearReference(&Node->ForwardedName);
    PhClearReference(&Node->OrdinalString);
    PhClearReference(&Node->FileName);

    PhFree(Node);
}

PPHP_MODULE_EXPORT_TREE_NODE PhpAddModuleExportNode(
    _Inout_ PPHP_MODULE_EXPORT_CONTEXT Context,
    _In_ PPHP_MODULE_EXPORT_RESULT Result
    )
{
    static ULONG64 NextUniqueId = 0;
    PPHP_MODULE_EXPORT_TREE_NODE node;

    node = PhAllocate(sizeof(PHP_MODULE_EXPORT_TREE_NODE));
    memset(node, 0, sizeof(PHP_MODULE_EXPORT_TREE_NODE));
    PhInitializeTreeNewNode(&node->Node);

    memset(node->TextCache, 0, sizeof(PH_STRINGREF) * PHP_MODEXPORT_TREE_COLUMN_MAXIMUM);
    node->Node.TextCache = node->TextCache;
    node->Node.TextCacheSize = PHP_MODEXPORT_TREE_COLUMN_MAXIMUM;

    PhSetReference(&node->ModuleName, Result->ModuleName);
    PhSetReference(&node->FunctionName, Result->FunctionName);
    PhSetReference(&node->ForwardedName, Result->ForwardedName);
    PhSetReference(&node->OrdinalString, Result->OrdinalString);
    PhSetReference(&node->FileName, Result->FileName);
    node->Address = Result->Address;
    PhPrintPointer(node->AddressString, Result->Address);

    node->UniqueId = ++NextUniqueId;

    if (Context->FilterSupport.FilterList)
        node->Node.Visible = PhApplyTreeNewFiltersToNode(&Context->FilterSupport, &node->Node);

    PhAddItemList(Context->NodeList, node);

    return node;
}

VOID PhpClearModuleExportTree(
    _In_ PPHP_MODULE_EXPORT_CONTEXT Context
    )
{
    for (ULONG i = 0; i < Context->NodeList->Count; i++)
        PhpDestroyModuleExportNode(Context->NodeList->Items[i]);

    PhClearList(Context->NodeList);

    TreeNew_NodesStructured(Context->TreeNewHandle);
}

BOOLEAN NTAPI PhpModuleExportsTreeNewCallback(
    _In_ HWND WindowHandle,
    _In_ PH_TREENEW_MESSAGE Message,
    _In_ PVOID Parameter1,
    _In_ PVOID Parameter2,
    _In_ PVOID Context
    )
{
    PPHP_MODULE_EXPORT_CONTEXT context = Context;
    PPHP_MODULE_EXPORT_TREE_NODE node;

    switch (Message)
    {
    case TreeNewGetChildren:
        {
            PPH_TREENEW_GET_CHILDREN getChildren = Parameter1;
            node = (PPHP_MODULE_EXPORT_TREE_NODE)getChildren->Node;

            if (!getChildren->Node)
            {
                static CONST _CoreCrtSecureSearchSortCompareFunction sortFunctions[] =
                {
                    SORT_FUNCTION(Function),
                    SORT_FUNCTION(Module),
                    SORT_FUNCTION(Ordinal),
                    SORT_FUNCTION(Address),
                    SORT_FUNCTION(ForwardName),
                };
                _CoreCrtSecureSearchSortCompareFunction sortFunction;

                static_assert(RTL_NUMBER_OF(sortFunctions) == PHP_MODEXPORT_TREE_COLUMN_MAXIMUM, "SortFunctions must equal maximum.");

                if (context->TreeNewSortColumn < PHP_MODEXPORT_TREE_COLUMN_MAXIMUM)
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
            node = (PPHP_MODULE_EXPORT_TREE_NODE)getCellText->Node;

            switch (getCellText->Id)
            {
            case PHP_MODEXPORT_TREE_COLUMN_FUNCTION:
                getCellText->Text = PhGetStringRef(node->FunctionName);
                break;
            case PHP_MODEXPORT_TREE_COLUMN_MODULE:
                getCellText->Text = PhGetStringRef(node->ModuleName);
                break;
            case PHP_MODEXPORT_TREE_COLUMN_ORDINAL:
                getCellText->Text = PhGetStringRef(node->OrdinalString);
                break;
            case PHP_MODEXPORT_TREE_COLUMN_ADDRESS:
                PhInitializeStringRefLongHint(&getCellText->Text, node->AddressString);
                break;
            case PHP_MODEXPORT_TREE_COLUMN_FORWARDNAME:
                getCellText->Text = PhGetStringRef(node->ForwardedName);
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
                    SendMessage(context->WindowHandle, WM_COMMAND, PHP_MODEXPORT_COPY, 0);
                break;
            }
        }
        return TRUE;
    case TreeNewLeftDoubleClick:
        {
            SendMessage(context->WindowHandle, WM_COMMAND, PHP_MODEXPORT_PROPERTIES, 0);
        }
        return TRUE;
    case TreeNewContextMenu:
        {
            PPH_TREENEW_CONTEXT_MENU contextMenuEvent = Parameter1;

            SendMessage(
                context->WindowHandle,
                WM_COMMAND,
                WM_PH_EXPORT_SHOWMENU,
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
            data.DefaultSortColumn = PHP_MODEXPORT_TREE_COLUMN_FUNCTION;
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

VOID PhpInitializeModuleExportTree(
    _Inout_ PPHP_MODULE_EXPORT_CONTEXT Context
    )
{
    Context->NodeList = PhCreateList(1024);

    PhSetControlTheme(Context->TreeNewHandle, L"explorer");
    TreeNew_SetCallback(Context->TreeNewHandle, PhpModuleExportsTreeNewCallback, Context);

    PhAddTreeNewColumn(Context->TreeNewHandle, PHP_MODEXPORT_TREE_COLUMN_FUNCTION, TRUE, L"Function", 220, PH_ALIGN_LEFT, 0, 0);
    PhAddTreeNewColumn(Context->TreeNewHandle, PHP_MODEXPORT_TREE_COLUMN_MODULE, TRUE, L"Module", 120, PH_ALIGN_LEFT, ULONG_MAX, 0);
    PhAddTreeNewColumn(Context->TreeNewHandle, PHP_MODEXPORT_TREE_COLUMN_ORDINAL, TRUE, L"Ordinal", 70, PH_ALIGN_LEFT, ULONG_MAX, 0);
    PhAddTreeNewColumn(Context->TreeNewHandle, PHP_MODEXPORT_TREE_COLUMN_ADDRESS, TRUE, L"Address", 110, PH_ALIGN_LEFT, ULONG_MAX, 0);
    PhAddTreeNewColumn(Context->TreeNewHandle, PHP_MODEXPORT_TREE_COLUMN_FORWARDNAME, FALSE, L"Forwarded To", 150, PH_ALIGN_LEFT, ULONG_MAX, 0);

    PhInitializeTreeNewFilterSupport(&Context->FilterSupport, Context->TreeNewHandle, Context->NodeList);

    TreeNew_SetSort(Context->TreeNewHandle, PHP_MODEXPORT_TREE_COLUMN_FUNCTION, AscendingSortOrder);

    PhpModuleExportsLoadSettingsTreeList(Context);
}

VOID PhpDeleteModuleExportTree(
    _Inout_ PPHP_MODULE_EXPORT_CONTEXT Context
    )
{
    PhDeleteTreeNewFilterSupport(&Context->FilterSupport);

    PhpClearModuleExportTree(Context);

    if (Context->NodeList)
        PhDereferenceObject(Context->NodeList);
}

PPHP_MODULE_EXPORT_TREE_NODE PhpGetSelectedModuleExportNode(
    _In_ PPHP_MODULE_EXPORT_CONTEXT Context
    )
{
    PPHP_MODULE_EXPORT_TREE_NODE node = NULL;

    for (ULONG i = 0; i < Context->NodeList->Count; i++)
    {
        node = Context->NodeList->Items[i];

        if (node->Node.Selected)
            return node;
    }

    return NULL;
}

VOID PhpModuleExportsFreeResult(
    _In_ PPHP_MODULE_EXPORT_RESULT Result
    )
{
    PhClearReference(&Result->ModuleName);
    PhClearReference(&Result->FunctionName);
    PhClearReference(&Result->ForwardedName);
    PhClearReference(&Result->OrdinalString);
    PhClearReference(&Result->FileName);

    PhFree(Result);
}

VOID PhpModuleExportsAddResultEntries(
    _In_ PPHP_MODULE_EXPORT_CONTEXT Context
    )
{
    ULONG i;

    PhAcquireQueuedLockExclusive(&Context->SearchResultsLock);

    if (Context->SearchResults->Count == 0 || Context->SearchResultsAddIndex == Context->SearchResults->Count)
    {
        PhReleaseQueuedLockExclusive(&Context->SearchResultsLock);
        return;
    }

    TreeNew_SetRedraw(Context->TreeNewHandle, FALSE);

    for (i = Context->SearchResultsAddIndex; i < Context->SearchResults->Count; i++)
    {
        PPHP_MODULE_EXPORT_RESULT result = Context->SearchResults->Items[i];

        PhpAddModuleExportNode(Context, result);
        PhpModuleExportsFreeResult(result);
    }

    TreeNew_NodesStructured(Context->TreeNewHandle);
    TreeNew_SetRedraw(Context->TreeNewHandle, TRUE);

    Context->SearchResultsAddIndex = i;

    PhReleaseQueuedLockExclusive(&Context->SearchResultsLock);
}

VOID PhpModuleExportsClearResultEntries(
    _In_ PPHP_MODULE_EXPORT_CONTEXT Context
    )
{
    PhpClearModuleExportTree(Context);

    Context->SearchResultsAddIndex = 0;

    for (ULONG i = 0; i < Context->SearchResults->Count; i++)
        PhpModuleExportsFreeResult(Context->SearchResults->Items[i]);

    PhClearList(Context->SearchResults);
}

_Function_class_(PH_ENUM_GENERIC_MODULES_CALLBACK)
static BOOLEAN NTAPI PhpModuleExportsEnumModulesCallback(
    _In_ PPH_MODULE_INFO Module,
    _In_opt_ PVOID Context
    )
{
    PPHP_MODULE_EXPORT_CONTEXT context = Context;
    PPH_STRING fileNameWin32;
    PPH_STRING moduleName;
    PH_MAPPED_IMAGE mappedImage;
    PH_MAPPED_IMAGE_EXPORTS exports;
    ULONG i;

    if (context->SearchStop)
        return FALSE;

    if (PhIsNullOrEmptyString(Module->FileName))
        return TRUE;

    fileNameWin32 = PhGetFileName(Module->FileName);

    if (!PhIsNullOrEmptyString(Module->Name))
        PhSetReference(&moduleName, Module->Name);
    else
        moduleName = PhGetBaseName(fileNameWin32);

    if (NT_SUCCESS(PhLoadMappedImage(fileNameWin32->Buffer, NULL, &mappedImage)))
    {
        if (NT_SUCCESS(PhGetMappedImageExports(&exports, &mappedImage)))
        {
            for (i = 0; i < exports.NumberOfEntries; i++)
            {
                PH_MAPPED_IMAGE_EXPORT_ENTRY exportEntry;
                PH_MAPPED_IMAGE_EXPORT_FUNCTION exportFunction;
                PPHP_MODULE_EXPORT_RESULT result;

                if (context->SearchStop)
                    break;

                if (
                    !NT_SUCCESS(PhGetMappedImageExportEntry(&exports, i, &exportEntry)) ||
                    !exportEntry.Name ||
                    !NT_SUCCESS(PhGetMappedImageExportFunction(&exports, NULL, exportEntry.Ordinal, &exportFunction))
                    )
                {
                    continue;
                }

                result = PhAllocateZero(sizeof(PHP_MODULE_EXPORT_RESULT));

                PhSetReference(&result->ModuleName, moduleName);
                result->FunctionName = PhConvertUtf8ToUtf16(exportEntry.Name);
                result->OrdinalString = PhFormatUInt64(exportEntry.Ordinal, FALSE);
                result->FileName = PhReferenceObject(fileNameWin32);
                result->Ordinal = exportEntry.Ordinal;
                result->Address = PTR_ADD_OFFSET(Module->BaseAddress, (ULONG_PTR)exportFunction.Function);

                if (exportFunction.ForwardedName)
                    result->ForwardedName = PhConvertUtf8ToUtf16(exportFunction.ForwardedName);

                PhAcquireQueuedLockExclusive(&context->SearchResultsLock);
                PhAddItemList(context->SearchResults, result);
                PhReleaseQueuedLockExclusive(&context->SearchResultsLock);
            }
        }

        PhUnloadMappedImage(&mappedImage);
    }

    if (moduleName)
        PhDereferenceObject(moduleName);
    PhDereferenceObject(fileNameWin32);

    return TRUE;
}

_Function_class_(USER_THREAD_START_ROUTINE)
NTSTATUS PhpModuleExportsSearchThreadStart(
    _In_ PVOID Parameter
    )
{
    PPHP_MODULE_EXPORT_CONTEXT context = Parameter;
    NTSTATUS status;

    status = PhEnumGenericModules(
        context->ProcessId,
        NULL,
        0,
        PhpModuleExportsEnumModulesCallback,
        context
        );

    PostMessage(context->WindowHandle, WM_PH_EXPORT_FINISHED, (WPARAM)status, 0);

    PhDereferenceObject(context);

    return STATUS_SUCCESS;
}

INT_PTR CALLBACK PhpModuleExportsDlgProc(
    _In_ HWND hwndDlg,
    _In_ UINT uMsg,
    _In_ WPARAM wParam,
    _In_ LPARAM lParam
    )
{
    PPHP_MODULE_EXPORT_CONTEXT context;

    if (uMsg == WM_INITDIALOG)
    {
        context = PhpCreateModuleExportsContext();
        PhSetDialogContext(hwndDlg, context);
    }
    else
    {
        context = PhGetDialogContext(hwndDlg);
    }

    if (!context)
        return FALSE;

    switch (uMsg)
    {
    case WM_INITDIALOG:
        {
            PPHP_MODULE_EXPORT_LAUNCH_PARAM launchParam = (PPHP_MODULE_EXPORT_LAUNCH_PARAM)lParam;

            context->ProcessId = launchParam->ProcessId;
            context->ParentWindowHandle = launchParam->ParentWindowHandle;
            PhSetReference(&context->ProcessName, launchParam->ProcessName);
            PhDereferenceObject(launchParam->ProcessName);
            PhFree(launchParam);

            PhSetApplicationWindowIcon(hwndDlg);

            context->WindowText = PhConcatStrings2(
                L"Search Exports - ",
                PhGetStringOrDefault(context->ProcessName, L"Unknown process")
                );
            PhSetWindowText(hwndDlg, PhGetStringOrEmpty(context->WindowText));

            context->TreeNewHandle = GetDlgItem(hwndDlg, IDC_MODEXPORTS_TREELIST);
            context->SearchWindowHandle = GetDlgItem(hwndDlg, IDC_MODEXPORTS_FILTER);

            PhInitializeLayoutManager(&context->LayoutManager, hwndDlg);
            PhAddLayoutItem(&context->LayoutManager, context->SearchWindowHandle, NULL, PH_ANCHOR_LEFT | PH_ANCHOR_TOP | PH_ANCHOR_RIGHT);
            PhAddLayoutItem(&context->LayoutManager, GetDlgItem(hwndDlg, IDOK), NULL, PH_ANCHOR_TOP | PH_ANCHOR_RIGHT);
            PhAddLayoutItem(&context->LayoutManager, context->TreeNewHandle, NULL, PH_ANCHOR_ALL);

            PhRegisterDialog(hwndDlg);
            PhCreateSearchControl(
                hwndDlg,
                context->SearchWindowHandle,
                L"Search Exports (Ctrl+K)",
                PhpModuleExportsSearchControlCallback,
                context
                );
            PhpInitializeModuleExportTree(context);

            context->MinimumSize.left = 0;
            context->MinimumSize.top = 0;
            context->MinimumSize.right = 300;
            context->MinimumSize.bottom = 100;
            MapDialogRect(hwndDlg, &context->MinimumSize);

            if (PhValidWindowPlacementFromSetting(SETTING_MODEXPORTS_WINDOW_POSITION))
                PhLoadWindowPlacementFromSetting(SETTING_MODEXPORTS_WINDOW_POSITION, SETTING_MODEXPORTS_WINDOW_SIZE, hwndDlg);
            else
                PhCenterWindow(hwndDlg, context->ParentWindowHandle);

            PhRegisterWindowCallback(hwndDlg, PH_PLUGIN_WINDOW_EVENT_TYPE_TOPMOST, NULL);

            context->SearchResults = PhCreateList(1024);
            context->SearchResultsAddIndex = 0;

            PhSetTimer(hwndDlg, PH_WINDOW_TIMER_DEFAULT, 1000, NULL);

            PhInitializeWindowTheme(hwndDlg, PhEnableThemeSupport);

            // Start enumerating the module exports. (dmex)
            SendMessage(hwndDlg, WM_COMMAND, IDOK, 0);
        }
        break;
    case WM_DESTROY:
        {
            PhRemoveDialogContext(hwndDlg);

            context->SearchStop = TRUE;

            PhKillTimer(hwndDlg, PH_WINDOW_TIMER_DEFAULT);

            if (context->SearchThreadHandle)
            {
                NtWaitForSingleObject(context->SearchThreadHandle, FALSE, NULL);
                NtClose(context->SearchThreadHandle);
                context->SearchThreadHandle = NULL;
            }

            PhpModuleExportsSaveSettingsTreeList(context);
            PhSaveWindowPlacementToSetting(SETTING_MODEXPORTS_WINDOW_POSITION, SETTING_MODEXPORTS_WINDOW_SIZE, hwndDlg);

            PhUnregisterWindowCallback(hwndDlg);

            PhDeleteLayoutManager(&context->LayoutManager);

            PhpDeleteModuleExportTree(context);

            if (context->WindowText)
                PhDereferenceObject(context->WindowText);

            PhDereferenceObject(context);

            PostQuitMessage(0);
        }
        break;
    case WM_PH_EXPORT_SHOWDIALOG:
        {
            if (IsMinimized(hwndDlg))
                ShowWindow(hwndDlg, SW_RESTORE);
            else
                ShowWindow(hwndDlg, SW_SHOW);

            SetForegroundWindow(hwndDlg);
        }
        break;
    case WM_SETCURSOR:
        {
            if (context->SearchThreadHandle)
            {
                PhSetCursor(PhLoadCursor(NULL, IDC_APPSTARTING));
                SetWindowLongPtr(hwndDlg, DWLP_MSGRESULT, TRUE);
                return TRUE;
            }
        }
        break;
    case WM_COMMAND:
        {
            switch (GET_WM_COMMAND_ID(wParam, lParam))
            {
            case IDOK:
                {
                    // Don't continue if the user requested cancellation.
                    if (context->SearchStop)
                        break;

                    // Restore the original window title. (dmex)
                    PhSetWindowText(hwndDlg, PhGetStringOrEmpty(context->WindowText));

                    if (!context->SearchThreadHandle)
                    {
                        // Clean up previous results.

                        PhpModuleExportsClearResultEntries(context);

                        // Start the search.

                        PhReferenceObject(context);

                        if (!NT_SUCCESS(PhCreateThreadEx(&context->SearchThreadHandle, PhpModuleExportsSearchThreadStart, context)))
                        {
                            PhDereferenceObject(context);
                            break;
                        }

                        PhSetDialogItemText(hwndDlg, IDOK, L"Cancel");

                        PhSetCursor(PhLoadCursor(NULL, IDC_APPSTARTING));
                    }
                    else
                    {
                        context->SearchStop = TRUE;
                        EnableWindow(GetDlgItem(hwndDlg, IDOK), FALSE);
                    }
                }
                break;
            case IDCANCEL:
                {
                    DestroyWindow(hwndDlg);
                }
                break;
            case WM_PH_EXPORT_SHOWMENU:
                {
                    PPH_TREENEW_CONTEXT_MENU contextMenuEvent = (PPH_TREENEW_CONTEXT_MENU)lParam;
                    PPH_EMENU menu;
                    PPH_EMENU_ITEM selectedItem;
                    PPHP_MODULE_EXPORT_TREE_NODE node;

                    if (!(node = PhpGetSelectedModuleExportNode(context)))
                        break;

                    menu = PhCreateEMenu();
                    PhInsertEMenuItem(menu, PhCreateEMenuItem(0, PHP_MODEXPORT_PROPERTIES, L"P&roperties", NULL, NULL), ULONG_MAX);
                    PhInsertEMenuItem(menu, PhCreateEMenuItem(0, PHP_MODEXPORT_LOCATION, L"Open &file location", NULL, NULL), ULONG_MAX);
                    PhInsertEMenuItem(menu, PhCreateEMenuSeparator(), ULONG_MAX);
                    PhInsertEMenuItem(menu, PhCreateEMenuItem(0, PHP_MODEXPORT_COPY, L"&Copy\bCtrl+C", NULL, NULL), ULONG_MAX);
                    PhInsertCopyCellEMenuItem(menu, PHP_MODEXPORT_COPY, context->TreeNewHandle, contextMenuEvent->Column);
                    PhSetFlagsEMenuItem(menu, PHP_MODEXPORT_PROPERTIES, PH_EMENU_DEFAULT, PH_EMENU_DEFAULT);

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
            case PHP_MODEXPORT_COPY:
                {
                    PPH_STRING text;

                    text = PhGetTreeNewText(context->TreeNewHandle, 0);
                    PhSetClipboardString(context->TreeNewHandle, &text->sr);
                    PhDereferenceObject(text);
                }
                break;
            case PHP_MODEXPORT_LOCATION:
                {
                    PPHP_MODULE_EXPORT_TREE_NODE node;

                    if (node = PhpGetSelectedModuleExportNode(context))
                    {
                        if (!PhIsNullOrEmptyString(node->FileName))
                            PhShellExploreFile(hwndDlg, node->FileName->Buffer);
                    }
                }
                break;
            case PHP_MODEXPORT_PROPERTIES:
                {
                    PPHP_MODULE_EXPORT_TREE_NODE node;

                    if (node = PhpGetSelectedModuleExportNode(context))
                    {
                        if (!PhIsNullOrEmptyString(node->FileName))
                            PhShellProperties(hwndDlg, node->FileName->Buffer);
                    }
                }
                break;
            }
        }
        break;
    case WM_DPICHANGED:
        {
            PhLayoutManagerUpdate(&context->LayoutManager, LOWORD(wParam));
            PhLayoutManagerLayout(&context->LayoutManager);

            context->MinimumSize.left = 0;
            context->MinimumSize.top = 0;
            context->MinimumSize.right = 300;
            context->MinimumSize.bottom = 100;
            MapDialogRect(hwndDlg, &context->MinimumSize);
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
    case WM_TIMER:
        {
            switch (wParam)
            {
            case PH_WINDOW_TIMER_DEFAULT:
                {
                    if (!context->SearchThreadHandle)
                        break;

                    // Update the search results.
                    PhpModuleExportsAddResultEntries(context);
                }
                break;
            }
        }
        break;
    case WM_PH_EXPORT_FINISHED:
        {
            // Add any un-added items.
            PhpModuleExportsAddResultEntries(context);

            // Add the result count to the window title. (dmex)
            PhSetWindowText(hwndDlg, PhaFormatString(
                L"%s (%lu results)",
                PhGetStringOrEmpty(context->WindowText),
                context->SearchResultsAddIndex
                )->Buffer);

            NtWaitForSingleObject(context->SearchThreadHandle, FALSE, NULL);
            NtClose(context->SearchThreadHandle);
            context->SearchThreadHandle = NULL;
            context->SearchStop = FALSE;

            PhSetDialogItemText(hwndDlg, IDOK, L"Refresh");
            EnableWindow(GetDlgItem(hwndDlg, IDOK), TRUE);
            PhSetCursor(PhLoadCursor(NULL, IDC_ARROW));
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
NTSTATUS PhpModuleExportsDialogThreadStart(
    _In_ PVOID Parameter
    )
{
    BOOL result;
    MSG message;
    PH_AUTO_POOL autoPool;

    PhInitializeAutoPool(&autoPool);

    PhpModuleExportsWindowHandle = PhCreateDialog(
        PhInstanceHandle,
        MAKEINTRESOURCE(IDD_MODEXPORTS),
        NULL,
        PhpModuleExportsDlgProc,
        Parameter
        );

    PhSetEvent(&PhpModuleExportsInitializedEvent);

    if (PhpModuleExportsWindowHandle)
    {
        while (result = GetMessage(&message, NULL, 0, 0))
        {
            if (result == INT_ERROR)
                break;

            if (!IsDialogMessage(PhpModuleExportsWindowHandle, &message))
            {
                TranslateMessage(&message);
                DispatchMessage(&message);
            }

            PhDrainAutoPool(&autoPool);
        }
    }

    PhDeleteAutoPool(&autoPool);
    PhResetEvent(&PhpModuleExportsInitializedEvent);

    if (PhpModuleExportsThreadHandle)
    {
        NtClose(PhpModuleExportsThreadHandle);
        PhpModuleExportsThreadHandle = NULL;
    }

    return STATUS_SUCCESS;
}

VOID PhShowModuleExportsDialog(
    _In_ HWND ParentWindowHandle,
    _In_ HANDLE ProcessId,
    _In_opt_ PPH_STRING ProcessName
    )
{
    if (!PhpModuleExportsThreadHandle)
    {
        PPHP_MODULE_EXPORT_LAUNCH_PARAM launchParam;

        launchParam = PhAllocateZero(sizeof(PHP_MODULE_EXPORT_LAUNCH_PARAM));
        launchParam->ParentWindowHandle = ParentWindowHandle;
        launchParam->ProcessId = ProcessId;
        PhSetReference(&launchParam->ProcessName, ProcessName);

        if (!NT_SUCCESS(PhCreateThreadEx(&PhpModuleExportsThreadHandle, PhpModuleExportsDialogThreadStart, launchParam)))
        {
            PhDereferenceObject(launchParam->ProcessName);
            PhFree(launchParam);
            PhShowStatus(ParentWindowHandle, L"Unable to create the window.", 0, ERROR_OUTOFMEMORY);
            return;
        }

        PhWaitForEvent(&PhpModuleExportsInitializedEvent, NULL);
    }

    PostMessage(PhpModuleExportsWindowHandle, WM_PH_EXPORT_SHOWDIALOG, 0, 0);
}
