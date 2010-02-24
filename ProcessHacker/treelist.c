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
    c.hCursor = LoadCursor(NULL, IDC_ARROW);
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
                WS_CHILD | WS_VISIBLE | HDS_BUTTONS | HDS_FULLDRAG | HDS_HORZ,
                0,
                0,
                0,
                0,
                hwnd,
                (HMENU)PH_TREELIST_HEADER_ID,
                PhInstanceHandle,
                NULL
                );

            {
                HDITEM item;

                item.mask = HDI_FORMAT | HDI_TEXT | HDI_WIDTH;
                item.cxy = 200;
                item.pszText = L"Name";
                item.fmt = HDF_LEFT | HDF_STRING;

                Header_InsertItem(context->HeaderHandle, 0, &item);
            }

            SendMessage(hwnd, WM_SETFONT, (WPARAM)PhIconTitleFont, FALSE);
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
            HDLAYOUT layout;
            WINDOWPOS pos;

            GetClientRect(hwnd, &clientRect);

            layout.prc = &clientRect;
            layout.pwpos = &pos;

            if (Header_Layout(context->HeaderHandle, &layout))
            {
                SetWindowPos(context->HeaderHandle, NULL, pos.x, pos.y,
                    pos.cx, pos.cy, SWP_NOACTIVATE | SWP_NOZORDER);
            }
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

            //FillRect(hdc, &paintStruct.rcPaint, GetSysColorBrush(COLOR_WINDOW));

            SelectObject(hdc, GetSysColorBrush(COLOR_WINDOWTEXT));
            rect.left = 0;
            rect.top = 20;
            rect.right = 200;
            rect.bottom = 100;

            DrawText(hdc, L"Test", -1, &rect, DT_LEFT | DT_NOCLIP);

            EndPaint(hwnd, &paintStruct);
        }
        break;
    case WM_SETFONT:
        {
            SendMessage(context->HeaderHandle, WM_SETFONT, wParam, lParam);
        }
        break;
    }

    return DefWindowProc(hwnd, uMsg, wParam, lParam);
}
