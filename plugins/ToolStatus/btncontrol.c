#include "toolstatus.h"
#include "btncontrol.h"

BOOLEAN InsertButton(
    __in HWND WindowHandle, 
    __in UINT uCmdId, 
    __in INT nSize
    )
{
    NC_CONTROL* context;
    
    context = (NC_CONTROL*)PhAllocate(sizeof(NC_CONTROL));
    memset(context, 0, sizeof(NC_CONTROL));

    context->uCmdId = uCmdId;
    context->nButSize = nSize;
    context->IsButtonDown = FALSE;
    context->ParentWindow = GetParent(WindowHandle);
    context->DllBase = (HINSTANCE)PluginInstance->DllBase;
    context->ImageList = ImageList_Create(18, 18, ILC_COLOR32 | ILC_MASK, 0, 0);
    
    // Set the number of images.
    ImageList_SetImageCount(context->ImageList, 2);
    PhSetImageListBitmap(context->ImageList, 0, context->DllBase, MAKEINTRESOURCE(IDB_SEARCH1));
    PhSetImageListBitmap(context->ImageList, 1, context->DllBase, MAKEINTRESOURCE(IDB_SEARCH2));;

    // associate our button state structure with the window before subclassing the WndProc
    SetProp(WindowHandle, L"Context", (HANDLE)context);

    // replace the old window procedure with our new one
    context->NCAreaWndProc = SubclassWindow(WindowHandle, InsButProc);

    // force the edit control to update its non-client area
    SetWindowPos(
        WindowHandle,
        0, 0, 0, 0, 0,
        SWP_FRAMECHANGED | SWP_NOMOVE | SWP_NOSIZE | SWP_NOACTIVATE | SWP_NOZORDER
        );

    return TRUE;
}

VOID GetButtonRect(
    __inout NC_CONTROL* nc,
    __in RECT* rect
    )
{
    // retrieve the coordinates of an inserted button, given the specified window rectangle. 
    rect->right -= nc->cxRightEdge;
    rect->top += nc->cyTopEdge;
    rect->bottom -= nc->cyBottomEdge;
    rect->left = rect->right - nc->nButSize;

    if (nc->cxRightEdge > nc->cxLeftEdge)
        OffsetRect(rect, nc->cxRightEdge - nc->cxLeftEdge, 0);
}   

VOID RedrawNC(
    __in HWND WindowHandle
    )
{
    SetWindowPos(
        WindowHandle, 
        0, 0, 0, 0, 0, 
        SWP_DRAWFRAME | SWP_NOMOVE | SWP_NOSIZE | SWP_NOACTIVATE | SWP_NOZORDER
        );
}

VOID DrawInsertedButton(
    __in HWND WindowHandle, 
    __inout NC_CONTROL* nc, 
    __in RECT* prect
    )
{
    HDC hdc;
    HBRUSH brush;
  
    hdc = GetWindowDC(WindowHandle);
    brush = CreateSolidBrush(RGB(0, 0, 0));

    //SetBkMode(hdc, TRANSPARENT);
    
    FillRect(hdc, prect, (HBRUSH)(COLOR_WINDOW+1));

    if (nc->ImageList)
    {
        // Draw the image - with some bad offsets..

        // Move the rect right
        OffsetRect(prect, 3, 0);

        if (nc->IsMouseDown)
        {
            ImageList_Draw(
                nc->ImageList, 
                0,     
                hdc, 
                prect->left, 
                prect->top - 3,
                ILD_NORMAL | ILD_TRANSPARENT
                );
        }
        else
        {
            ImageList_Draw(
                nc->ImageList, 
                1, 
                hdc, 
                prect->left, 
                prect->top - 2,
                ILD_NORMAL | ILD_TRANSPARENT
                );
        }
    }

    DeleteObject(brush);
    ReleaseDC(WindowHandle, hdc);
}

LRESULT CALLBACK InsButProc(
    __in HWND WindowHandle,
    __in UINT uMsg,
    __in WPARAM wParam,
    __in LPARAM lParam
    )
{
    NC_CONTROL* context = (NC_CONTROL*)GetProp(WindowHandle, L"Context");

    if (!context || !context->NCAreaWndProc)
        return FALSE;

    if (uMsg == WM_DESTROY)
    {
        if (context->ImageList)
        {
            ImageList_Destroy(context->ImageList);
            context->ImageList = NULL;
        }

        RemoveProp(WindowHandle, L"Context");     
        PhFree(context);

        return FALSE;
    }
    
    if (!context || !context->NCAreaWndProc)
        return FALSE;

    switch (uMsg)
    {
    case WM_NCCALCSIZE:
        {
            NCCALCSIZE_PARAMS* nccsp = (NCCALCSIZE_PARAMS*)lParam;

            context->prect = (RECT*)lParam;
            context->oldrect = *context->prect;

            // Adjust (shrink) the client rectangle to accommodate the border:
            nccsp->rgrc[0].top += 3;

            // let the old wndproc allocate space for the borders, or any other non-client space.
            CallWindowProc(context->NCAreaWndProc, WindowHandle, uMsg, wParam, lParam);

            // calculate what the size of each window border is,
            // we need to know where the button is going to live.
            context->cxLeftEdge = context->prect->left - context->oldrect.left; 
            context->cxRightEdge = context->oldrect.right - context->prect->right;
            context->cyTopEdge = context->prect->top - context->oldrect.top;
            context->cyBottomEdge = context->oldrect.bottom - context->prect->bottom;   

            // now we can allocate additional space by deflating the
            // rectangle even further. Our button will go on the right-hand side,
            // and will be the same width as a scrollbar button
            context->prect->right -= context->nButSize; 
        }
        return FALSE;
    case WM_NCPAINT:
        {  
            // let the old window procedure draw the borders other non-client bits-and-pieces for us.
            CallWindowProc(context->NCAreaWndProc, WindowHandle, uMsg, wParam, lParam);

            // get the screen coordinates of the window.
            GetWindowRect(WindowHandle, &context->rect);
            // adjust the coordinates so they start from 0,0
            OffsetRect(&context->rect, -context->rect.left, -context->rect.top);    
            
            // border - requires the edit control have the WS_EX_STATICEDGE style
            {
                HDC hdc;
                HBRUSH brush;

                hdc = GetWindowDC(WindowHandle);
                brush = CreateSolidBrush(RGB(0, 0, 0));

                FillRect(hdc, &context->rect, (HBRUSH)(COLOR_WINDOW+1));

                // Draw a single line around the outside
                FrameRect(hdc, &context->rect, brush);

                DeleteObject(brush);
                ReleaseDC(WindowHandle, hdc);
            }

            // work out where to draw the button
            GetButtonRect(context, &context->rect);

            DrawInsertedButton(WindowHandle, context, &context->rect);
        }
        return FALSE;
    case WM_NCHITTEST:
        {
            // get the screen coordinates of the mouse
            context->pt.x = GET_X_LPARAM(lParam);
            context->pt.y = GET_Y_LPARAM(lParam);

            // get the position of the inserted button
            GetWindowRect(WindowHandle, &context->rect);
            GetButtonRect(context, &context->rect);

            // check that the mouse is within the inserted button
            if (PtInRect(&context->rect, context->pt))
                return HTBORDER;
        }
        break;
    case WM_NCLBUTTONDBLCLK:
    case WM_NCLBUTTONDOWN:
        {
            // get the screen coordinates of the mouse
            context->pt.x = GET_X_LPARAM(lParam);
            context->pt.y = GET_Y_LPARAM(lParam);

            // get the position of the inserted button
            GetWindowRect(WindowHandle, &context->rect);
            
            context->pt.x -= context->rect.left;
            context->pt.y -= context->rect.top;

            OffsetRect(&context->rect, -context->rect.left, -context->rect.top);

            GetButtonRect(context, &context->rect);

            // check that the mouse is within the inserted button
            if (PtInRect(&context->rect, context->pt))
            {
                SetCapture(WindowHandle);

                context->IsButtonDown = TRUE;
                context->IsMouseDown = TRUE;

                //redraw the non-client area to reflect the change
                DrawInsertedButton(WindowHandle, context, &context->rect);
            }
        }
        break;
    case WM_MOUSEMOVE:
        {
            if (!context->IsMouseDown)
                break;

            // get the SCREEN coordinates of the mouse
            context->pt.x = GET_X_LPARAM(lParam);
            context->pt.y = GET_Y_LPARAM(lParam);

            ClientToScreen(WindowHandle, &context->pt);

            // get the position of the inserted button
            GetWindowRect(WindowHandle, &context->rect);

            context->pt.x -= context->rect.left;
            context->pt.y -= context->rect.top;

            OffsetRect(
                &context->rect, 
                -context->rect.left, 
                -context->rect.top
                );

            GetButtonRect(context, &context->rect);

            context->oldstate = context->IsButtonDown;

            // check that the mouse is within the inserted button
            if (PtInRect(&context->rect, context->pt))
            {
                context->IsButtonDown = TRUE;
            }
            else
            {
                context->IsButtonDown = FALSE;
            }

            // redraw the non-client area to reflect the change.
            // to prevent flicker, we only redraw the button if its state has changed
            if (context->oldstate != context->IsButtonDown)
                DrawInsertedButton(WindowHandle, context, &context->rect);
        }
        break;
    case WM_LBUTTONUP:
        {
            if (!context->IsMouseDown)
                break;

            // get the SCREEN coordinates of the mouse
            context->pt.x = GET_X_LPARAM(lParam);
            context->pt.y = GET_Y_LPARAM(lParam);
            
            ClientToScreen(WindowHandle, &context->pt);

            // get the position of the inserted button
            GetWindowRect(WindowHandle, &context->rect);

            context->pt.x -= context->rect.left;
            context->pt.y -= context->rect.top;

            OffsetRect(
                &context->rect, 
                -context->rect.left, 
                -context->rect.top
                );

            GetButtonRect(context, &context->rect);
            
            // check that the mouse is within the inserted button
            if (PtInRect(&context->rect, context->pt))
            {
                PostMessage(
                    context->ParentWindow, 
                    WM_COMMAND, 
                    MAKEWPARAM(context->uCmdId, BN_CLICKED), 
                    0
                    );
            }
                    
            ReleaseCapture();

            context->IsButtonDown = FALSE;
            context->IsMouseDown = FALSE;

            // redraw the non-client area to reflect the change.
            DrawInsertedButton(WindowHandle, context, &context->rect);
        }
        break;
    }

    return CallWindowProc(context->NCAreaWndProc, WindowHandle, uMsg, wParam, lParam);
}