#include <phgui.h>

HWND PhMainWindowHandle;

BOOLEAN PhInitializeMainWindow(INT showCommand)
{
    PhMainWindowHandle = CreateWindow(
        PhWindowClassName,
        L"Process Hacker",
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

    if (!PhMainWindowHandle)
    {
        return FALSE;
    }

    ShowWindow(PhMainWindowHandle, showCommand);

    return TRUE;
}

INT PhMainMessageLoop()
{
    BOOL result;
    MSG message;
    HACCEL acceleratorTable;

    acceleratorTable = LoadAccelerators(PhInstanceHandle, MAKEINTRESOURCE(IDR_MAINWND));

    while (result = GetMessage(&message, NULL, 0, 0))
    {
        if (result == -1)
            return 1;

        if (!TranslateAccelerator(message.hwnd, acceleratorTable, &message))
        {
            TranslateMessage(&message);
            DispatchMessage(&message);
        }
    }

    return (INT)message.wParam;
}

LRESULT CALLBACK PhMainWndProc(      
    HWND hWnd,
    UINT uMsg,
    WPARAM wParam,
    LPARAM lParam
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
        {

        }
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
