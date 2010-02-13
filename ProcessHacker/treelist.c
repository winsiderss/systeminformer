#include <phgui.h>
#include <treelist.h>
#include <treelistp.h>

BOOLEAN PhTreeListInitialization()
{
    WNDCLASSEX c = { sizeof(c) };

    c.style = 0;
    c.lpfnWndProc = PhpTreeListWndProc;
    c.cbClsExtra = 0;
    c.cbWndExtra = sizeof(PVOID);
    c.hInstance = PhInstanceHandle;
    c.hIcon = NULL;
    c.hCursor = NULL;
    c.hbrBackground = NULL;
    c.lpszMenuName = NULL;
    c.lpszClassName = PH_TREELIST_CLASSNAME;
    c.hIconSm = NULL;

    return !!RegisterClassEx(&c);
}

HWND PhCreateTreeListControl(
    __in HWND ParentHandle,
    __in INT_PTR Id
    )
{
    return CreateWindow(
        PH_TREELIST_CLASSNAME,
        L"",
        WS_CHILD | WS_VISIBLE | WS_BORDER | WS_CLIPSIBLINGS,
        0,
        0,
        3,
        3,
        ParentHandle,
        (HMENU)Id,
        PhInstanceHandle,
        NULL
        );
}

VOID PhpInitializeTreeListContext(
    __out PPHP_TREELIST_CONTEXT Context
    )
{

}

VOID PhpDeleteTreeListContext(
    __inout PPHP_TREELIST_CONTEXT Context
    )
{

}

LRESULT CALLBACK PhpTreeListWndProc(
    __in HWND hwnd,
    __in UINT uMsg,
    __in WPARAM wParam,
    __in LPARAM lParam
    )
{
    PPHP_TREELIST_CONTEXT context;

    context = (PPHP_TREELIST_CONTEXT)GetWindowLongPtr(hwnd, 0);

    if (uMsg == WM_CREATE)
    {
        context = PhAllocate(sizeof(PHP_TREELIST_CONTEXT));
        PhpInitializeTreeListContext(context);
        SetWindowLongPtr(hwnd, 0, (LONG_PTR)context);
    }

    if (!context)
        return DefWindowProc(hwnd, uMsg, wParam, lParam);

    switch (uMsg)
    {
    case WM_CREATE:
        {
            LPCREATESTRUCT createStruct = (LPCREATESTRUCT)lParam;

            context->Handle = hwnd;

            context->HeaderHandle = CreateWindow(
                WC_HEADER,
                L"",
                WS_CHILD | WS_BORDER | HDS_BUTTONS | HDS_HORZ,
                0,
                0,
                createStruct->cx,
                CW_USEDEFAULT,
                hwnd,
                (HMENU)PH_TREELIST_HEADER_ID,
                PhInstanceHandle,
                NULL
                );
        }
        break;
    case WM_DESTROY:
        {
            PhpDeleteTreeListContext(context);
            PhFree(context);
            SetWindowLongPtr(hwnd, 0, (LONG_PTR)NULL);
        }
        break;
    case WM_SIZE:
        {
            RECT clientRect;

            GetClientRect(hwnd, &clientRect);
            SetWindowPos(context->HeaderHandle, NULL, 0, 0,
                clientRect.right - clientRect.left, 20, SWP_NOACTIVATE | SWP_NOZORDER);
        }
        break;
    case WM_ERASEBKGND:
        return TRUE;
    case WM_PAINT:
        {
            HDC hdc;
            PAINTSTRUCT paintStruct;
            RECT clientRect;
            RECT rect;

            hdc = BeginPaint(hwnd, &paintStruct);

            GetClientRect(hwnd, &clientRect);

            FillRect(hdc, &paintStruct.rcPaint, GetSysColorBrush(COLOR_WINDOW));

            SelectObject(hdc, GetSysColorBrush(COLOR_WINDOWTEXT));
            rect.left = 0;
            rect.top = 0;
            rect.right = 200;
            rect.bottom = 100;

            DrawText(hdc, L"Test", -1, &rect, DT_LEFT | DT_NOCLIP);

            EndPaint(hwnd, &paintStruct);
        }
        break;
    }

    return DefWindowProc(hwnd, uMsg, wParam, lParam);
}
