/*
 * Process Hacker -
 *   Subclassed Edit control
 *
 * Copyright (C) 2012-2013 dmex
 *
 * This file is part of Process Hacker.
 *
 * Process Hacker is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation, either version 3 of the License, or
 * (at your option) any later version.
 *
 * Process Hacker is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with Process Hacker.  If not, see <http://www.gnu.org/licenses/>.
 */

// dmex: The non-client area subclassing code has been modified based on the following guide:
// http://www.catch22.net/tuts/insert-buttons-edit-control

#include "toolstatus.h"
#include "searchbox.h"

typedef struct _NC_CONTROL
{
    UINT CommandID; // sent in a WM_COMMAND message
    UINT uState;
    BOOLEAN ShowSearchIcon; // is the button up/down?
    BOOLEAN IsMouseDown; // is the mouse activating the button?

    INT nButSize; // horizontal size of button   
    INT cxLeftEdge; // size of the current window borders.
    INT cxRightEdge;  // size of the current window borders.
    INT cyTopEdge; 
    INT cyBottomEdge; 
   
    POINT pt;
    RECT rect;
    RECT oldrect;
    RECT* prect;
      
    HIMAGELIST ImageList;
    HBRUSH BorderBrush;
    HBRUSH DcBrush;
    HINSTANCE DllBase;
    HWND ParentWindow;
    WNDPROC NCAreaWndProc;
} NC_CONTROL;

static VOID GetButtonRect(
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

static VOID DrawInsertedButton(
    __in HWND WindowHandle, 
    __inout NC_CONTROL* context, 
    __in HDC HdcHandle,
    __in RECT* prect
    )
{
    // Draw the image - with some bad offsets..
    // Move the draw region 3 right and up 3
    OffsetRect(prect, 5, -3);

    if (context->ShowSearchIcon)
    {
        ImageList_DrawEx(
            context->ImageList, 
            0,     
            HdcHandle, 
            prect->left, 
            prect->top,
            18,
            18,
            CLR_NONE,
            CLR_NONE,
            ILD_NORMAL | ILD_TRANSPARENT   
            );
    }
    else
    {
        ImageList_DrawEx(
            context->ImageList, 
            1, 
            HdcHandle, 
            prect->left, 
            prect->top,
            18,
            18,
            CLR_NONE,
            CLR_NONE,
            ILD_NORMAL | ILD_TRANSPARENT
            );
    }
}

LRESULT CALLBACK NcAreaWndSubclassProc(
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
    
    switch (uMsg)
    {
    case WM_ERASEBKGND:
        return 1;
    case WM_NCCALCSIZE:
        {
            NCCALCSIZE_PARAMS* nccsp = (NCCALCSIZE_PARAMS*)lParam;

            context->prect = (RECT*)lParam;
            context->oldrect = *context->prect;

            // Adjust (shrink) the edit control client rectangle to accommodate the border:
            nccsp->rgrc[0].left += 3;
            nccsp->rgrc[0].right -= 3;
            nccsp->rgrc[0].top += 4;
            nccsp->rgrc[0].bottom -= 4;

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
            HDC hdc;

            // let the old window procedure draw the borders other non-client bits-and-pieces for us.
            CallWindowProc(context->NCAreaWndProc, WindowHandle, uMsg, wParam, lParam);

            // ProcessHacker Themed border
            // Note the use of undocumented flags below. GetDCEx doesn't work without these
            if (hdc = GetDCEx(WindowHandle, NULL, DCX_WINDOW | DCX_LOCKWINDOWUPDATE | 0x10000))
            {
                // get the screen coordinates of the window
                GetWindowRect(WindowHandle, &context->rect);
                // adjust the coordinates so they start from 0,0
                OffsetRect(&context->rect, -context->rect.left, -context->rect.top);    

                // Clear the region
                FillRect(hdc, &context->rect, context->DcBrush);

                // Set border color
                FrameRect(hdc, &context->rect, context->BorderBrush);

                // work out where to draw the button
                GetButtonRect(context, &context->rect);
                DrawInsertedButton(WindowHandle, context, hdc, &context->rect);   

                // cleanup
                ReleaseDC(WindowHandle, hdc);
            }

            // HACK - invalidate the edit control client region (XP only??)
            if (WindowsVersion < WINDOWS_VISTA)
                InvalidateRect(WindowHandle, NULL, FALSE);
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

            // adjust the coordinates so they start from 0,0
            OffsetRect(&context->rect, -context->rect.left, -context->rect.top);

            GetButtonRect(context, &context->rect);

            // check that the mouse is within the inserted button
            if (PtInRect(&context->rect, context->pt))
            {
                HDC hdc;

                context->IsMouseDown = TRUE;
                SetCapture(WindowHandle);

                if (hdc = GetDCEx(WindowHandle, NULL, DCX_WINDOW | DCX_LOCKWINDOWUPDATE | 0x10000))
                {         
                    // redraw the non-client area to reflect the change
                    DrawInsertedButton(WindowHandle, context, hdc, &context->rect);
                    // cleanup
                    ReleaseDC(WindowHandle, hdc);
                }
            }
        }
        break;
    //case WM_NCMOUSEMOVE:
    //case WM_NCMOUSELEAVE:
    //    {
    //        // get the screen coordinates of the mouse
    //        context->pt.x = GET_X_LPARAM(lParam);
    //        context->pt.y = GET_Y_LPARAM(lParam);
    //        // get the position of the inserted button
    //        GetWindowRect(WindowHandle, &context->rect);
    //        context->pt.x -= context->rect.left;
    //        context->pt.y -= context->rect.top;
    //        // adjust the coordinates so they start from 0,0
    //        OffsetRect(&context->rect, -context->rect.left, -context->rect.top);
    //        GetButtonRect(context, &context->rect);
    //        context->oldstate = context->IsMouseActive;
    //        //check that the mouse is within the inserted button
    //        if (PtInRect(&context->rect, context->pt))
    //        {
    //            context->IsMouseActive = TRUE;
    //        }
    //        else
    //        {
    //            context->IsMouseActive = FALSE;
    //        }
    //        // to prevent flicker, we only redraw the button if its state has changed    
    //        if (context->oldstate != context->IsMouseActive)      
    //            DrawInsertedButton(WindowHandle, context, NULL, &context->rect);
    //    }
    //    break;
    case WM_LBUTTONUP:
        {
            // get the SCREEN coordinates of the mouse
            context->pt.x = GET_X_LPARAM(lParam);
            context->pt.y = GET_Y_LPARAM(lParam);
        
            context->IsMouseDown = FALSE;
            ReleaseCapture();

            ClientToScreen(WindowHandle, &context->pt);
            // get the position of the inserted button
            GetWindowRect(WindowHandle, &context->rect);

            context->pt.x -= context->rect.left;
            context->pt.y -= context->rect.top;

            OffsetRect(&context->rect, -context->rect.left, -context->rect.top);
            GetButtonRect(context, &context->rect);

            // check that the mouse is within the region
            if (PtInRect(&context->rect, context->pt))
            {
                HDC hdc;

                // Send the click notification to the parent window
                PostMessage(context->ParentWindow, WM_COMMAND, MAKEWPARAM(context->CommandID, BN_CLICKED), 0);

                context->ShowSearchIcon = FALSE;

                if (hdc = GetDCEx(WindowHandle, NULL, DCX_WINDOW | DCX_LOCKWINDOWUPDATE | 0x10000))
                {
                    //redraw the non-client area to reflect the change
                    DrawInsertedButton(WindowHandle, context, hdc, &context->rect);
                    // cleanup
                    ReleaseDC(WindowHandle, hdc);
                }
            }
        }
        break;
    case WM_KEYUP:
    case WM_KILLFOCUS:
        {            
            HDC hdc;

            context->ShowSearchIcon = Edit_GetTextLength(WindowHandle) > 0;

            if (hdc = GetDCEx(WindowHandle, NULL, DCX_WINDOW | DCX_LOCKWINDOWUPDATE | 0x10000))
            {                  
                // get the screen coordinates of the window
                GetWindowRect(WindowHandle, &context->rect);

                // adjust the coordinates so they start from 0,0
                OffsetRect(&context->rect, -context->rect.left, -context->rect.top); 

                // work out where to draw the button
                GetButtonRect(context, &context->rect);           

                // only clear the button region
                FillRect(hdc, &context->rect, context->DcBrush);

                // redraw the non-client area to reflect the change
                DrawInsertedButton(WindowHandle, context, hdc, &context->rect);

                // cleanup
                ReleaseDC(WindowHandle, hdc);
            }
        }
        break;
    }

    return CallWindowProc(context->NCAreaWndProc, WindowHandle, uMsg, wParam, lParam);
}

BOOLEAN InsertButton(
    __in HWND WindowHandle, 
    __in UINT CommandID, 
    __in INT nSize
    )
{
    NC_CONTROL* context;
    
    context = (NC_CONTROL*)PhAllocate(sizeof(NC_CONTROL));
    memset(context, 0, sizeof(NC_CONTROL));

    context->CommandID = CommandID;
    context->nButSize = nSize;
    context->DllBase = (HINSTANCE)PluginInstance->DllBase;
    context->ParentWindow = GetParent(WindowHandle);

    context->DcBrush = (HBRUSH)GetStockObject(DC_BRUSH);
    context->BorderBrush = (HBRUSH)CreateSolidBrush(RGB(0x8f, 0x8f, 0x8f));
    context->ImageList = ImageList_Create(22, 22, ILC_COLOR32 | ILC_MASK, 0, 0);

    // Set the number of images
    ImageList_SetImageCount(context->ImageList, 2);
    PhSetImageListBitmap(context->ImageList, 0, context->DllBase, MAKEINTRESOURCE(IDB_SEARCH1));
    PhSetImageListBitmap(context->ImageList, 1, context->DllBase, MAKEINTRESOURCE(IDB_SEARCH2));;

    // associate our button state structure with the window before subclassing the WndProc
    SetProp(WindowHandle, L"Context", (HANDLE)context);

    // replace the old window procedure with our new one
    context->NCAreaWndProc = SubclassWindow(WindowHandle, NcAreaWndSubclassProc);

    // force the edit control to update its non-client area
    SetWindowPos(
        WindowHandle,
        0, 0, 0, 0, 0,
        SWP_FRAMECHANGED | SWP_NOMOVE | SWP_NOSIZE | SWP_NOACTIVATE | SWP_NOZORDER
        );

    return TRUE;
}