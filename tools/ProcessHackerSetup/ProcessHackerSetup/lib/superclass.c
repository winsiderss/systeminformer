#include <setup.h>

#pragma comment(lib, "Msimg32.lib")

//_Success_(return != NULL)

INT GetWindowWidth(HWND hwnd)
{
    RECT rect = { 0 };

    GetWindowRect(hwnd, &rect);

    return (rect.right - rect.left);
}

INT GetWindowHeight(HWND hwnd)
{
    RECT rect = { 0 };

    GetWindowRect(hwnd, &rect);

    return (rect.bottom - rect.top);
}

INT GetClientWindowWidth(HWND hwnd)
{
    RECT rect = { 0 };

    GetClientRect(hwnd, &rect);

    return (rect.right - rect.left);
}

INT GetClientWindowHeight(HWND hwnd)
{
    RECT rect = { 0 };

    GetClientRect(hwnd, &rect);

    return (rect.bottom - rect.top);
}


static VOID InflateClientRect(
    _Inout_ PRECT rect, 
    _In_ INT x, 
    _In_ INT y
    )
{
    rect->left -= x;
    rect->top -= y;
    rect->right += x;
    rect->bottom += y;
}

static VOID FrameClientRect(
    _In_ HDC hdc, 
    _In_ const PRECT rect, 
    _In_ HBRUSH hbrush
    )
{
    if ((rect->right <= rect->left) || (rect->bottom <= rect->top)) 
        return;

    HBRUSH prevBrush = SelectObject(hdc, hbrush);

    PatBlt(hdc, rect->left, rect->top, 1, rect->bottom - rect->top, PATCOPY);
    PatBlt(hdc, rect->right - 1, rect->top, 1, rect->bottom - rect->top, PATCOPY);
    PatBlt(hdc, rect->left, rect->top, rect->right - rect->left, 1, PATCOPY);
    PatBlt(hdc, rect->left, rect->bottom - 1, rect->right - rect->left, 1, PATCOPY);
       
    if (prevBrush)
    {
        SelectObject(hdc, prevBrush);
    }
}

static VOID FillClientRect(
    _In_ HDC hdc, 
    _In_ const PRECT rect, 
    _In_ HBRUSH hbrush
    )
{
    HBRUSH prevBrush = SelectObject(hdc, hbrush);

    PatBlt(
        hdc,
        rect->left,
        rect->top,
        rect->right - rect->left,
        rect->bottom - rect->top, 
        PATCOPY
        );

    if (prevBrush)
    {
        SelectObject(hdc, prevBrush);
    }
}







// http://msdn.microsoft.com/en-us/library/windows/desktop/bb760816.aspx
VOID DrawProgressBarControl(
    _In_ HWND WindowHandle, 
    _In_ HDC WindowDC, 
    _In_ RECT ClientRect
    )
{
    INT curValue = SendMessage(WindowHandle, PBM_GETPOS, 0, 0);
    INT maxValue = SendMessage(WindowHandle, PBM_GETRANGE, 0, 0);
    FLOAT percent = ((FLOAT)curValue / (FLOAT)maxValue) * 100.f;

    int oldBkMode = SetBkMode(WindowDC, TRANSPARENT);

    // Fill window background
    FillClientRect(WindowDC, &ClientRect, GetSysColorBrush(COLOR_3DFACE));
    // Draw window border
    FrameClientRect(WindowDC, &ClientRect, GetStockBrush(LTGRAY_BRUSH));

    if (curValue < 1)
    {
        return;
    }

    //if (maxValue < 1)
    //{
    //    return;
    //}

    InflateClientRect(
        &ClientRect,
        -GetSystemMetrics(SM_CXBORDER) * 2,
        -GetSystemMetrics(SM_CYBORDER) * 2
        );

    // Set progress fill length
    ClientRect.right = ((LONG)(ClientRect.left + ((ClientRect.right - ClientRect.left) * percent) / 100));

    // Draw progress fill border
    //FrameClientRect(WindowDC, &ClientRect, GetStockBrush(BLACK_BRUSH));

    //InflateClientRect(
    //    &ClientRect,
    //    -GetSystemMetrics(SM_CXBORDER),
    //    -GetSystemMetrics(SM_CYBORDER)
    //    );

    HBRUSH brush = GetSysColorBrush(COLOR_HIGHLIGHT);// CreateSolidBrush(RGB(48, 190, 50));
    FillClientRect(WindowDC, &ClientRect, brush);
    DeleteObject(brush);

    // draw background last?
    //SubtractRect()

    SetBkMode(WindowDC, oldBkMode);
}


LRESULT CALLBACK SubclassWindowProc(
    _In_ HWND hWnd,
    _In_ UINT uMsg,
    _In_ WPARAM wParam,
    _In_ LPARAM lParam,
    _In_ UINT_PTR uIdSubclass,
    _In_ DWORD_PTR dwRefData
    )
{
    switch (uMsg)
    {
    case WM_ERASEBKGND:
        return 1;
    case WM_PAINT:
        {
            PAINTSTRUCT paintStruct;
            RECT clientRect;
            
            GetClientRect(hWnd, &clientRect);

            if (BeginPaint(hWnd, &paintStruct))
            {
                SetBkMode(paintStruct.hdc, TRANSPARENT);

                //if (BeginBufferedPaint_I)
                //{
                //    HDC bufferedHdc = NULL;
                //    HPAINTBUFFER bufferedHandle = NULL;

                //    if (bufferedHandle = BeginBufferedPaint_I(
                //        paintStruct.hdc,
                //        &clientRect,
                //        BPBF_COMPATIBLEBITMAP,
                //        NULL,
                //        &bufferedHdc
                //        ))
                //    {
                //        if (uIdSubclass == IDC_PROGRESS1)
                //        {
                //            DrawProgressBarControl(hWnd, bufferedHdc, clientRect);
                //        }

                //        if (EndBufferedPaint_I)
                //        {
                //            EndBufferedPaint_I(bufferedHandle, TRUE);
                //        }
                //    }
                //    else
                //    {
                //        DrawProgressBarControl(hWnd, paintStruct.hdc, clientRect);
                //    }
                //}
                //else
                {
                    DrawProgressBarControl(hWnd, paintStruct.hdc, clientRect);
                }

                EndPaint(hWnd, &paintStruct);
            }
            return 1;
        }
        break;
    }

    return DefSubclassProc(hWnd, uMsg, wParam, lParam);
}

//VOID SetupSuperSubclass(VOID)
//{
//    WNDCLASSEX windowClassInfo = { sizeof(WNDCLASSEX) };
//
//    if (GetClassInfoEx(NULL, WC_BUTTON, &windowClassInfo))
//    {
//        parentWndProc = windowClassInfo.lpfnWndProc; // save the original message handler 
//        windowClassInfo.lpfnWndProc = WindowProc; // Set our new message handler
//
//        UnregisterClass(WC_BUTTON, NULL);
//        RegisterClassEx(&windowClassInfo);
//
//        return TRUE;
//    }
//
//    return FALSE;
//}