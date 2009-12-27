#include <phgui.h>
#include <wchar.h>

VOID PhMainWndOnCreate();
VOID PhMainWndOnLayout();
VOID PhMainWndTabControlOnLayout();
VOID PhMainWndTabControlOnNotify(
    __in LPNMHDR Header
    );
VOID PhMainWndTabControlOnSelectionChanged();

VOID FillProcessInfo(
    __inout PPH_PROCESS_ITEM ProcessItem
    );

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
    default:
        return DefWindowProc(hWnd, uMsg, wParam, lParam);
    }

    return 0;
}

VOID EnumerateProcesses()
{
    PVOID processes;
    PSYSTEM_PROCESS_INFORMATION process;

    if (!NT_SUCCESS(PhEnumProcesses(&processes)))
        return;

    process = PH_FIRST_PROCESS(processes);

    do
    {
        PPH_PROCESS_ITEM processItem;
        INT lvItemIndex;

        if (process->UniqueProcessId == (HANDLE)0)
            RtlInitUnicodeString(&process->ImageName, L"System Idle Process");

        processItem = PhCreateProcessItem(process->UniqueProcessId);
        processItem->ProcessName = PhCreateStringEx(process->ImageName.Buffer, process->ImageName.Length);
        _snwprintf(processItem->ProcessIdString, PH_INT_STR_LEN, L"%d", processItem->ProcessId);
        FillProcessInfo(processItem);

        lvItemIndex = PhAddListViewItem(
            ProcessListViewHandle,
            MAXINT,
            processItem->ProcessName->Buffer,
            processItem
            );
        PhSetListViewSubItem(ProcessListViewHandle, lvItemIndex, 1, processItem->ProcessIdString);
        PhSetListViewSubItem(ProcessListViewHandle, lvItemIndex, 2, PhGetString(processItem->UserName));
        PhSetListViewSubItem(ProcessListViewHandle, lvItemIndex, 3, PhGetString(processItem->FileName));
        PhSetListViewSubItem(ProcessListViewHandle, lvItemIndex, 4, PhGetString(processItem->CommandLine));
    } while (process = PH_NEXT_PROCESS(process));

    PhFree(processes);
}

VOID FillProcessInfo(
    __inout PPH_PROCESS_ITEM ProcessItem
    )
{
    NTSTATUS status;
    HANDLE processHandle;

    status = PhOpenProcess(&processHandle, PROCESS_QUERY_INFORMATION, ProcessItem->ProcessId);

    if (!NT_SUCCESS(status))
        return;

    // Process information
    {
        PPH_STRING fileName;

        status = PhGetProcessImageFileName(processHandle, &fileName);

        if (NT_SUCCESS(status))
        {
            PPH_STRING newFileName;
            
            newFileName = PhGetFileName(fileName);
            PhSwapReference(&ProcessItem->FileName, newFileName);

            PhDereferenceObject(fileName);
            PhDereferenceObject(newFileName);
        }
    }

    {
        HANDLE processHandle2;

        status = PhOpenProcess(
            &processHandle2,
            PROCESS_QUERY_INFORMATION | PROCESS_VM_READ,
            ProcessItem->ProcessId
            );

        if (NT_SUCCESS(status))
        {
            PPH_STRING commandLine;

            status = PhGetProcessCommandLine(processHandle2, &commandLine);

            if (NT_SUCCESS(status))
            {
                PhSwapReference(&ProcessItem->CommandLine, commandLine);
                PhDereferenceObject(commandLine);
            }

            CloseHandle(processHandle2);
        }
    }

    // Token-related information
    {
        HANDLE tokenHandle;

        status = PhOpenProcessToken(&tokenHandle, TOKEN_QUERY, processHandle);

        if (NT_SUCCESS(status))
        {
            // User name
            {
                PTOKEN_USER user;

                status = PhGetTokenUser(tokenHandle, &user);

                if (NT_SUCCESS(status))
                {
                    PPH_STRING userName;
                    PPH_STRING domainName;

                    if (PhLookupSid(user->User.Sid, &userName, &domainName, NULL))
                    {
                        PPH_STRING fullName;

                        fullName = PhCreateStringEx(NULL, domainName->Length + 2 + userName->Length);
                        memcpy(fullName->Buffer, domainName->Buffer, domainName->Length);
                        fullName->Buffer[domainName->Length / 2] = '\\';
                        memcpy(&fullName->Buffer[domainName->Length / 2 + 1], userName->Buffer, userName->Length);

                        PhSwapReference(&ProcessItem->UserName, fullName);

                        PhDereferenceObject(userName);
                        PhDereferenceObject(domainName);
                        PhDereferenceObject(fullName);
                    }

                    PhFree(user);
                }
            }

            CloseHandle(tokenHandle);
        }
    }

    CloseHandle(processHandle);
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

    EnumerateProcesses();
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
