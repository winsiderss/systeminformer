#include <phgui.h>
#include <wchar.h>

VOID PhMainWndOnCreate();
VOID PhMainWndOnLayout();
VOID PhMainWndTabControlOnLayout();
VOID PhMainWndTabControlOnNotify(
    __in LPNMHDR Header
    );
VOID PhMainWndTabControlOnSelectionChanged();

VOID PhMainWndOnProcessAdded(
    __in PPH_PROCESS_ITEM ProcessItem
    );

VOID PhMainWndOnProcessRemoved(
    __in PPH_PROCESS_ITEM ProcessItem
    );

HWND PhMainWndHandle;
static HWND TabControlHandle;
static INT ProcessesTabIndex;
static INT ServicesTabIndex;
static INT NetworkTabIndex;
static HWND ProcessListViewHandle;
static HWND ServiceListViewHandle;
static HWND NetworkListViewHandle;

static PH_PROVIDER_THREAD PrimaryProviderThread;

static PH_CALLBACK_REGISTRATION ProcessProviderRegistration;
static PH_CALLBACK_REGISTRATION ProcessAddedRegistration;
static PH_CALLBACK_REGISTRATION ProcessRemovedRegistration;

BOOLEAN PhMainWndInitialization(
    __in INT ShowCommand
    )
{
    // Enable debug privilege if possible.
    {
        HANDLE tokenHandle;

        if (NT_SUCCESS(PhOpenProcessToken(
            &tokenHandle,
            TOKEN_ADJUST_PRIVILEGES,
            NtCurrentProcess()
            )))
        {
            PhSetTokenPrivilege(tokenHandle, L"SeDebugPrivilege", NULL, SE_PRIVILEGE_ENABLED);
            CloseHandle(tokenHandle);
        }
    }

    // Initialize the providers.
    PhInitializeProviderThread(&PrimaryProviderThread, 1000);
    PhRegisterProvider(&PrimaryProviderThread, PhProcessProviderUpdate, &ProcessProviderRegistration);

    PhMainWndHandle = CreateWindow(
        PhWindowClassName,
        PH_APP_NAME,
        WS_OVERLAPPEDWINDOW,
        CW_USEDEFAULT,
        0,
        CW_USEDEFAULT,
        0,
        NULL,
        NULL,
        PhInstanceHandle,
        NULL
        );

    if (!PhMainWndHandle)
        return FALSE;

    PhInitializeFont(PhMainWndHandle);

    // Initialize child controls.
    PhMainWndOnCreate();

    PhMainWndTabControlOnSelectionChanged();

    // Perform a layout.
    PhMainWndOnLayout();

    PhStartProviderThread(&PrimaryProviderThread);

    ShowWindow(PhMainWndHandle, ShowCommand);

    return TRUE;
}

LRESULT CALLBACK PhMainWndProc(      
    __in HWND hWnd,
    __in UINT uMsg,
    __in WPARAM wParam,
    __in LPARAM lParam
    )
{
    switch (uMsg)
    { 
    case WM_DESTROY:
        {
            PostQuitMessage(0);
        }
        break;
    case WM_COMMAND:
        {
            INT id = LOWORD(wParam);

            switch (id)
            {
            case ID_HACKER_EXIT:
                DestroyWindow(hWnd);
                break;
            default:
                return DefWindowProc(hWnd, uMsg, wParam, lParam);
            }
        }
        break;
    case WM_PAINT:
        {
            HDC hdc;
            PAINTSTRUCT paintStruct;

            hdc = BeginPaint(hWnd, &paintStruct);
            EndPaint(hWnd, &paintStruct);
        }
        break;
    case WM_SIZE:
        PhMainWndOnLayout();
        InvalidateRect(hWnd, NULL, TRUE);
        break;
    case WM_NOTIFY:
        {
            LPNMHDR header = (LPNMHDR)lParam;

            if (header->hwndFrom == TabControlHandle)
            {
                PhMainWndTabControlOnNotify(header);
            }
        }
        break;
    case WM_PH_PROCESS_ADDED:
        {
            PPH_PROCESS_ITEM processItem = (PPH_PROCESS_ITEM)lParam;

            PhMainWndOnProcessAdded(processItem);
            PhDereferenceObject(processItem);
        }
        break;
    case WM_PH_PROCESS_REMOVED:
        {
            PhMainWndOnProcessRemoved((PPH_PROCESS_ITEM)lParam);
        }
        break;
    default:
        return DefWindowProc(hWnd, uMsg, wParam, lParam);
    }

    return 0;
}

VOID NTAPI ProcessAddedHandler(
    __in PVOID Parameter,
    __in PVOID Context
    )
{
    PPH_PROCESS_ITEM processItem = (PPH_PROCESS_ITEM)Parameter;

    // Reference the process item so it doesn't get deleted before 
    // we handle the event in the main thread.
    PhReferenceObject(processItem);
    PostMessage(PhMainWndHandle, WM_PH_PROCESS_ADDED, 0, (LPARAM)processItem);
}

VOID NTAPI ProcessRemovedHandler(
    __in PVOID Parameter,
    __in PVOID Context
    )
{
    PPH_PROCESS_ITEM processItem = (PPH_PROCESS_ITEM)Parameter;

    // We already have a reference to the process item, so we don't need to 
    // reference it here.
    PostMessage(PhMainWndHandle, WM_PH_PROCESS_REMOVED, 0, (LPARAM)processItem);
}

VOID PhMainWndOnCreate()
{
    TabControlHandle = PhCreateTabControl(PhMainWndHandle);
    ProcessesTabIndex = PhAddTabControlTab(TabControlHandle, 0, L"Processes");
    ServicesTabIndex = PhAddTabControlTab(TabControlHandle, 1, L"Services");
    NetworkTabIndex = PhAddTabControlTab(TabControlHandle, 2, L"Network");

    ProcessListViewHandle = PhCreateListViewControl(PhMainWndHandle, ID_MAINWND_PROCESSLV);
    ListView_SetExtendedListViewStyleEx(ProcessListViewHandle, LVS_EX_FULLROWSELECT | LVS_EX_DOUBLEBUFFER, -1);
    ServiceListViewHandle = PhCreateListViewControl(PhMainWndHandle, ID_MAINWND_SERVICELV);
    ListView_SetExtendedListViewStyleEx(ServiceListViewHandle, LVS_EX_FULLROWSELECT | LVS_EX_DOUBLEBUFFER, -1);
    NetworkListViewHandle = PhCreateListViewControl(PhMainWndHandle, ID_MAINWND_NETWORKLV);
    ListView_SetExtendedListViewStyleEx(NetworkListViewHandle, LVS_EX_FULLROWSELECT | LVS_EX_DOUBLEBUFFER, -1);

    PhAddListViewColumn(ProcessListViewHandle, 0, 0, 0, LVCFMT_LEFT, 100, L"Name");
    PhAddListViewColumn(ProcessListViewHandle, 1, 1, 1, LVCFMT_LEFT, 80, L"PID");
    PhAddListViewColumn(ProcessListViewHandle, 2, 2, 2, LVCFMT_LEFT, 140, L"User Name");
    PhAddListViewColumn(ProcessListViewHandle, 3, 3, 3, LVCFMT_LEFT, 300, L"File Name");
    PhAddListViewColumn(ProcessListViewHandle, 4, 4, 4, LVCFMT_LEFT, 300, L"Command Line");
    PhAddListViewColumn(
        ServiceListViewHandle,
        0,
        0,
        0,
        LVCFMT_LEFT,
        100,
        L"Name"
        );
    PhAddListViewColumn(
        NetworkListViewHandle,
        0,
        0,
        0,
        LVCFMT_LEFT,
        100,
        L"Process Name"
        );

    PhRegisterCallback(
        &PhProcessAddedEvent,
        ProcessAddedHandler,
        NULL,
        &ProcessAddedRegistration
        );
    PhRegisterCallback(
        &PhProcessRemovedEvent,
        ProcessRemovedHandler,
        NULL,
        &ProcessRemovedRegistration
        );
}

VOID PhMainWndOnLayout()
{
    RECT rect;

    // Resize the tab control.
    GetClientRect(PhMainWndHandle, &rect);
    PhSetControlPosition(TabControlHandle, rect.left, rect.top, rect.right, rect.bottom);

    PhMainWndTabControlOnLayout();
}

VOID PhMainWndTabControlOnLayout()
{
    RECT rect;
    INT selectedIndex;

    GetClientRect(PhMainWndHandle, &rect);
    TabCtrl_AdjustRect(TabControlHandle, FALSE, &rect);

    selectedIndex = TabCtrl_GetCurSel(TabControlHandle);

    if (selectedIndex == ProcessesTabIndex)
    {
        PhSetControlPosition(ProcessListViewHandle, rect.left, rect.top, rect.right, rect.bottom);
    }
    else if (selectedIndex == ServicesTabIndex)
    {
        PhSetControlPosition(ServiceListViewHandle, rect.left, rect.top, rect.right, rect.bottom);
    }
    else if (selectedIndex == NetworkTabIndex)
    {
        PhSetControlPosition(NetworkListViewHandle, rect.left, rect.top, rect.right, rect.bottom);
    }
}

VOID PhMainWndTabControlOnNotify(
    __in LPNMHDR Header
    )
{
    if (Header->code == TCN_SELCHANGE)
    {
        PhMainWndTabControlOnSelectionChanged();
    }
}

VOID PhMainWndTabControlOnSelectionChanged()
{
    INT selectedIndex;

    selectedIndex = TabCtrl_GetCurSel(TabControlHandle);
    ShowWindow(ProcessListViewHandle, selectedIndex == ProcessesTabIndex ? SW_SHOW : SW_HIDE);
    ShowWindow(ServiceListViewHandle, selectedIndex == ServicesTabIndex ? SW_SHOW : SW_HIDE);
    ShowWindow(NetworkListViewHandle, selectedIndex == NetworkTabIndex ? SW_SHOW : SW_HIDE);

    PhMainWndTabControlOnLayout();
}

VOID PhMainWndOnProcessAdded(
    __in PPH_PROCESS_ITEM ProcessItem
    )
{
    INT lvItemIndex;

    // Add a reference for the pointer being stored in the list view item.
    PhReferenceObject(ProcessItem);

    lvItemIndex = PhAddListViewItem(
        ProcessListViewHandle,
        MAXINT,
        ProcessItem->ProcessName->Buffer,
        ProcessItem
        );
    PhSetListViewSubItem(ProcessListViewHandle, lvItemIndex, 1, ProcessItem->ProcessIdString);
    PhSetListViewSubItem(ProcessListViewHandle, lvItemIndex, 2, PhGetString(ProcessItem->UserName));
    PhSetListViewSubItem(ProcessListViewHandle, lvItemIndex, 3, PhGetString(ProcessItem->FileName));
    PhSetListViewSubItem(ProcessListViewHandle, lvItemIndex, 4, PhGetString(ProcessItem->CommandLine));
}

VOID PhMainWndOnProcessRemoved(
    __in PPH_PROCESS_ITEM ProcessItem
    )
{
    PhRemoveListViewItem(
        ProcessListViewHandle,
        PhFindListViewItemByParam(ProcessListViewHandle, -1, ProcessItem)
        );
}
