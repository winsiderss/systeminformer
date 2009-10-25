#include <phgui.h>

HWND PhMainWndHandle;
static HWND TabControlHandle;

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

    PhMainWndOnCreate();
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
    case WM_DESTROY:
        {
            PostQuitMessage(0);
        }
        break;
    default:
        return DefWindowProc(hWnd, uMsg, wParam, lParam);
    }

    return 0;
}

VOID PhMainWndOnCreate()
{
    TabControlHandle = PhCreateTabControl(PhMainWndHandle);
    PhAddTabControlTab(TabControlHandle, 0, L"Processes");
    PhAddTabControlTab(TabControlHandle, 1, L"Services");
    PhAddTabControlTab(TabControlHandle, 2, L"Network");
}

VOID PhMainWndOnLayout()
{
    RECT rect;

    GetClientRect(PhMainWndHandle, &rect);
    PhSetControlPosition(TabControlHandle, 0, 0, rect.right, rect.bottom);
}
