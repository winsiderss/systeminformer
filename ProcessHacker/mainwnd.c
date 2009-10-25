#include <phgui.h>

VOID PhMainWndOnCreate();
VOID PhMainWndOnLayout();
VOID PhMainWndTabControlOnLayout();
VOID PhMainWndTabControlOnNotify(
    __in LPNMHDR Header
    );
VOID PhMainWndTabControlOnSelectionChanged();

HWND PhMainWndHandle;
static HWND TabControlHandle;
static INT ProcessesTabIndex;
static INT ServicesTabIndex;
static INT NetworkTabIndex;
static HWND ProcessListViewHandle;
static HWND ServiceListViewHandle;
static HWND NetworkListViewHandle;

BOOLEAN PhMainWndInitialization(
    __in INT ShowCommand
    )
{
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
        return DefWindowProc(hWnd, uMsg, wParam, lParam);
    case WM_SIZE:
        PhMainWndOnLayout();
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
    default:
        return DefWindowProc(hWnd, uMsg, wParam, lParam);
    }

    return 0;
}

BOOLEAN ProcessEnumCallback(PSYSTEM_PROCESS_INFORMATION Process)
{
    PWSTR buffer;

    // LEAK
    buffer = PhAllocate(Process->ImageName.Length + sizeof(WCHAR));
    memcpy(buffer, Process->ImageName.Buffer, Process->ImageName.Length);
    buffer[Process->ImageName.Length / sizeof(WCHAR)] = 0;

    PhAddListViewItem(
        ProcessListViewHandle,
        MAXINT,
        buffer
        );

    return FALSE;
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

    PhAddListViewColumn(
        ProcessListViewHandle,
        0,
        0,
        0,
        LVCFMT_LEFT,
        100,
        L"Name"
        );
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

    PhEnumProcesses(ProcessEnumCallback, NULL);
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
