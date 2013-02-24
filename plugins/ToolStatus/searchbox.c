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

#include <uxtheme.h>
#include <vsstyle.h>
#pragma comment(lib, "uxtheme.lib")

typedef struct _NC_CONTROL
{
    UINT CommandID; // sent in a WM_COMMAND message
    UINT uState;

    INT nButSize; // 22 horizontal size of button   
    INT cxLeftEdge; // size of the current window borders.
    INT cxRightEdge;  // size of the current window borders.
    INT cyTopEdge; 
    INT cyBottomEdge; 
    INT TextLength;

    POINT pt;
    RECT rect;
    RECT oldrect;
    RECT* prect;
         
    HBRUSH BorderBrush;
    HBRUSH DcBrush;
    HTHEME WndHTheme;
    HIMAGELIST ImageList;
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
    __in HWND hwndDlg, 
    __inout NC_CONTROL* context, 
    __in HDC HdcHandle,
    __in RECT* prect
    )
{
    // Draw the image - with some bad offsets..
    // Move the draw region +3 right and up -2
    OffsetRect(prect, 0, -2);

    if (context->TextLength > 0)
    {
        ImageList_DrawEx(
            context->ImageList, 
            0,     
            HdcHandle, 
            prect->left, 
            prect->top,
            0, 0,
            CLR_NONE,
            CLR_NONE,
            ILD_NORMAL | ILD_TRANSPARENT | ILD_SCALE
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
            0, 0,
            CLR_NONE,
            CLR_NONE,
            ILD_NORMAL | ILD_TRANSPARENT | ILD_SCALE
            );
    }
}

LRESULT CALLBACK NcAreaWndSubclassProc(
    __in HWND hwndDlg,
    __in UINT uMsg,
    __in WPARAM wParam,
    __in LPARAM lParam
    )
{
    NC_CONTROL* context = (NC_CONTROL*)GetProp(hwndDlg, L"Context");

    if (!context || !context->NCAreaWndProc)
        return FALSE;

    if (uMsg == WM_DESTROY)
    {
        if (context->ImageList)
        {
            ImageList_Destroy(context->ImageList);
            context->ImageList = NULL;
        }

        RemoveProp(hwndDlg, L"Context");     
        PhFree(context);

        return FALSE;
    }
    
    switch (uMsg)
    {  
    case WM_NCACTIVATE:
    case WM_ERASEBKGND:
        return TRUE;
    case WM_NCCALCSIZE:
        {
            NCCALCSIZE_PARAMS* nccsp = (NCCALCSIZE_PARAMS*)lParam;

            context->prect = (RECT*)lParam;
            context->oldrect = *context->prect;

            // Adjust (shrink) the edit control client rectangle to accommodate the border:
            nccsp->rgrc[0].left += 1;
            nccsp->rgrc[0].right -= 1;
            nccsp->rgrc[0].top += 3;
            nccsp->rgrc[0].bottom -= 1;

            // let the old wndproc allocate space for the borders, or any other non-client space.
            CallWindowProc(context->NCAreaWndProc, hwndDlg, uMsg, wParam, lParam);

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
     
            if (hdc = GetWindowDC(hwndDlg))
            {
                // get the screen coordinates of the window
                GetWindowRect(hwndDlg, &context->rect);
                // adjust the coordinates - start from 0,0
                OffsetRect(&context->rect, -context->rect.left, -context->rect.top);    
   
                if (context->WndHTheme)
                {
                    DrawThemeBackground(
                        context->WndHTheme, 
                        hdc, 
                        EP_BACKGROUNDWITHBORDER, 
                        EBWBS_NORMAL, 
                        &context->rect, 
                        0
                        );
                }
                else
                {
                    // Clear the draw region 
                    FillRect(hdc, &context->rect, context->DcBrush);
                    // Set border color
                    FrameRect(hdc, &context->rect, context->BorderBrush);
                }

                // work out where to draw the button
                GetButtonRect(context, &context->rect);
                DrawInsertedButton(hwndDlg, context, hdc, &context->rect);   

                // cleanup
                ReleaseDC(hwndDlg, hdc);
            }
        }
        return FALSE;
    case WM_NCHITTEST:
        {
            // get the screen coordinates of the mouse
            context->pt.x = GET_X_LPARAM(lParam);
            context->pt.y = GET_Y_LPARAM(lParam);

            // get the position of the inserted button
            GetWindowRect(hwndDlg, &context->rect);
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
            GetWindowRect(hwndDlg, &context->rect);
            
            context->pt.x -= context->rect.left;
            context->pt.y -= context->rect.top;

            // adjust the coordinates so they start from 0,0
            OffsetRect(&context->rect, -context->rect.left, -context->rect.top);

            GetButtonRect(context, &context->rect);

            // check that the mouse is within the inserted button
            if (PtInRect(&context->rect, context->pt))
            {
                SetCapture(hwndDlg);
                               
                // Send the click notification to the parent window
                PostMessage(
                    context->ParentWindow,
                    WM_COMMAND, 
                    MAKEWPARAM(context->CommandID, BN_CLICKED), 
                    0
                    );

                // invalidate the nonclient area
                //RedrawWindow(
                //    hwndDlg, 
                //    NULL, NULL, 
                //    RDW_FRAME | RDW_INVALIDATE | RDW_UPDATENOW | RDW_NOCHILDREN
                //    );
            }
        }
        return FALSE;
    //case WM_NCMOUSEMOVE:
    //case WM_NCMOUSELEAVE:
    //    {
    //        // get the screen coordinates of the mouse
    //        context->pt.x = GET_X_LPARAM(lParam);
    //        context->pt.y = GET_Y_LPARAM(lParam);
    //        // get the position of the inserted button
    //        GetWindowRect(hwndDlg, &context->rect);
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
    //            DrawInsertedButton(hwndDlg, context, NULL, &context->rect);
    //    }
    //    break;
    case WM_LBUTTONUP:
        {
            // get the SCREEN coordinates of the mouse
            context->pt.x = GET_X_LPARAM(lParam);
            context->pt.y = GET_Y_LPARAM(lParam);

            // Always release
            ReleaseCapture();

            ClientToScreen(hwndDlg, &context->pt);
            // get the position of the inserted button
            GetWindowRect(hwndDlg, &context->rect);

            context->pt.x -= context->rect.left;
            context->pt.y -= context->rect.top;

            OffsetRect(&context->rect, -context->rect.left, -context->rect.top);
            GetButtonRect(context, &context->rect);

            // check that the mouse is within the region
            if (PtInRect(&context->rect, context->pt))
            {
                context->TextLength = Edit_GetTextLength(hwndDlg) > 0;

                //RedrawWindow(
                //    hwndDlg, 
                //    NULL, NULL, 
                //    RDW_FRAME | RDW_INVALIDATE | RDW_UPDATENOW | RDW_NOCHILDREN
                //    );
            }
        }
        return FALSE;
    case WM_KEYUP:
        {   
            context->TextLength = Edit_GetTextLength(hwndDlg) > 0;

            // invalidate the nonclient area
            RedrawWindow(
                hwndDlg, 
                NULL, NULL, 
                RDW_FRAME | RDW_INVALIDATE | RDW_UPDATENOW | RDW_NOCHILDREN
                );
        }
        break;
    }

    return CallWindowProc(context->NCAreaWndProc, hwndDlg, uMsg, wParam, lParam);
}

BOOLEAN InsertButton(
    __in HWND hwndDlg, 
    __in UINT CommandID
    )
{
    NC_CONTROL* context = (NC_CONTROL*)PhAllocate(
        sizeof(NC_CONTROL)
        );

    memset(context, 0, sizeof(NC_CONTROL));
       
    context->CommandID = CommandID;
    context->ParentWindow = GetParent(hwndDlg);
    context->BorderBrush = (HBRUSH)CreateSolidBrush(RGB(0x8f, 0x8f, 0x8f));
    context->DcBrush = (HBRUSH)GetStockObject(DC_BRUSH);
    // search image sizes are 23x20
    context->nButSize = 21;
    context->ImageList = ImageList_Create(18, 18, ILC_COLOR32 | ILC_MASK, 0, 0);

    // Set the number of images
    ImageList_SetImageCount(context->ImageList, 2);
    PhSetImageListBitmap(context->ImageList, 0, (HINSTANCE)PluginInstance->DllBase, MAKEINTRESOURCE(IDB_SEARCH1));
    PhSetImageListBitmap(context->ImageList, 1, (HINSTANCE)PluginInstance->DllBase, MAKEINTRESOURCE(IDB_SEARCH2));;

    context->WndHTheme = OpenThemeData(hwndDlg, VSCLASS_EDIT); // SearchBox, SearchEditBox, Edit::SearchBox, Edit::SearchEditBox
    //We can also SetWindowTheme:
    //    InactiveSearchBoxEdit
    //    InactiveSearchBoxEditComposited
    //    MaxInactiveSearchBoxEdit
    //    MaxInactiveSearchBoxEditComposited
    //    MaxSearchBoxEdit
    //    MaxSearchBoxEditComposited
    //    SearchBoxEdit
    //    SearchBoxEditComposited

    // associate our button state structure with the window before subclassing the WndProc
    SetProp(hwndDlg, L"Context", (HANDLE)context);

    // replace the old window procedure with our new one
    context->NCAreaWndProc = SubclassWindow(hwndDlg, NcAreaWndSubclassProc);

    if (!context->WndHTheme)
    {
        //force the edit control to update its non-client area
        SetWindowPos(
            hwndDlg,
            0, 0, 0, 0, 0,
            SWP_FRAMECHANGED | SWP_NOMOVE | SWP_NOSIZE | SWP_NOACTIVATE | SWP_NOZORDER
            );
    }

    return TRUE;
}