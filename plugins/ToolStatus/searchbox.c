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
#include <vssym32.h>

#define HRGN_FULL ((HRGN)1) // passed by WM_NCPAINT even though it's completely undocumented

_IsThemeActive IsThemeActive_I;
_OpenThemeData OpenThemeData_I;
_CloseThemeData CloseThemeData_I;
_IsThemePartDefined IsThemePartDefined_I;
_DrawThemeBackground DrawThemeBackground_I;
_DrawThemeText DrawThemeText_I;
_GetThemeInt GetThemeInt_I;

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
    HMODULE uxthemeHandle;
    HIMAGELIST ImageList;
    HWND ParentWindow;
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
            ILD_NORMAL | ILD_SCALE
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
            ILD_NORMAL | ILD_SCALE
            );
    }
}

VOID PhTnpDrawThemedBorder(
    __in HWND hwnd,
    __in NC_CONTROL* Context,
    __in HDC hdc
    )
{
    RECT windowRect;
    RECT clientRect;
    LONG sizingBorderWidth;
    LONG borderX;
    LONG borderY;

    GetWindowRect(hwnd, &windowRect);
    windowRect.right -= windowRect.left;
    windowRect.bottom -= windowRect.top;
    windowRect.left = 0;
    windowRect.top = 0;

    clientRect.left = windowRect.left + GetSystemMetrics(SM_CXEDGE);
    clientRect.top = windowRect.top + GetSystemMetrics(SM_CYEDGE);
    clientRect.right = windowRect.right - GetSystemMetrics(SM_CXEDGE);
    clientRect.bottom = windowRect.bottom - GetSystemMetrics(SM_CYEDGE);

    // Make sure we don't paint in the client area.
    //ExcludeClipRect(hdc, clientRect.left, clientRect.top, clientRect.right, clientRect.bottom);

    // Draw the themed border.
    DrawThemeBackground_I(Context->WndHTheme, hdc, 0, 0, &windowRect, NULL);

    // Calculate the size of the border we just drew, and fill in the rest of the space if we didn't fully paint the region.
    if (SUCCEEDED(GetThemeInt_I(Context->WndHTheme, 0, 0, TMT_SIZINGBORDERWIDTH, &sizingBorderWidth)))
    {
        borderX = sizingBorderWidth;
        borderY = sizingBorderWidth;
    }
    else
    {
        borderX = GetSystemMetrics(SM_CXBORDER);
        borderY = GetSystemMetrics(SM_CYBORDER);
    }

    if (borderX < GetSystemMetrics(SM_CXEDGE) || borderY < GetSystemMetrics(SM_CYEDGE))
    {
        windowRect.left += GetSystemMetrics(SM_CXEDGE) - borderX;
        windowRect.top += GetSystemMetrics(SM_CYEDGE) - borderY;
        windowRect.right -= GetSystemMetrics(SM_CXEDGE) - borderX;
        windowRect.bottom -= GetSystemMetrics(SM_CYEDGE) - borderY;

        FillRect(hdc, &windowRect, GetSysColorBrush(COLOR_WINDOW));
    }

    //IntersectClipRect(hdc, clientRect.left, clientRect.top, clientRect.right, clientRect.bottom);

    {
        // get the screen coordinates of the window
        GetWindowRect(hwnd, &Context->rect);
        // adjust the coordinates - start from 0,0
        OffsetRect(&Context->rect, -Context->rect.left, -Context->rect.top);    

        // work out where to draw the button
        GetButtonRect(Context, &Context->rect);
        DrawInsertedButton(hwnd, Context, hdc, &Context->rect);   
    }
}

BOOLEAN PhTnpOnNcPaint(
    __in HWND hwnd,
    __in NC_CONTROL* Context,
    __in_opt HRGN UpdateRegion
    )
{
    // Themed border
    //if ((Context->ExtendedStyle & WS_EX_CLIENTEDGE) && Context->ThemeData)
    HDC hdc;
    ULONG flags;

    if (UpdateRegion == HRGN_FULL)
        UpdateRegion = NULL;

    // Note the use of undocumented flags below. GetDCEx doesn't work without these.
    flags = DCX_WINDOW | DCX_LOCKWINDOWUPDATE | 0x10000;

    if (UpdateRegion)
        flags |= DCX_INTERSECTRGN | 0x40000;

    if (hdc = GetDCEx(hwnd, UpdateRegion, flags))
    {
        PhTnpDrawThemedBorder(hwnd, Context, hdc);
        ReleaseDC(hwnd, hdc);
        return TRUE;
    }

    return FALSE;
}

LRESULT CALLBACK NcAreaWndSubclassProc(
    __in HWND hwndDlg,
    __in UINT uMsg,
    __in WPARAM wParam,
    __in LPARAM lParam, 
    __in UINT_PTR uIdSubclass, 
    __in DWORD_PTR dwRefData
    )
{
    NC_CONTROL* context = (NC_CONTROL*)GetProp(hwndDlg, L"Context");

    if (!context)
        return FALSE;

    if (uMsg == WM_DESTROY)
    {
        if (context->BorderBrush)
        {
            DeleteObject(context->BorderBrush);
            context->BorderBrush = NULL;
        }

        if (context->DcBrush)
        {
            DeleteObject(context->DcBrush);
            context->DcBrush = NULL;
        }

        if (context->ImageList)
        {
            ImageList_Destroy(context->ImageList);
            context->ImageList = NULL;
        }

        if (CloseThemeData_I && context->WndHTheme)
        {
            CloseThemeData_I(context->WndHTheme);
            context->WndHTheme = NULL;
        }

        if (context->uxthemeHandle)
        {
            FreeLibrary(context->uxthemeHandle);
            context->uxthemeHandle = NULL;
        }

        RemoveProp(hwndDlg, L"Context");
        RemoveWindowSubclass(hwndDlg, NcAreaWndSubclassProc, 0);

        PhFree(context);
        return FALSE;
    }

    switch (uMsg)
    {  
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
            //CallWindowProc(context->NCAreaWndProc, hwndDlg, uMsg, wParam, lParam);
            DefSubclassProc(hwndDlg, uMsg, wParam, lParam);

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
            if (!PhTnpOnNcPaint(hwndDlg, context, (HRGN)wParam))
                return FALSE;
        }
        break;
        //case WM_NCPAINT:
        //    {      
        //        HDC hdc;
        //        if (hdc = GetWindowDC(hwndDlg))
        //        {
        //            // get the screen coordinates of the window
        //            GetWindowRect(hwndDlg, &context->rect);
        //            // adjust the coordinates - start from 0,0
        //            OffsetRect(&context->rect, -context->rect.left, -context->rect.top);    

        //            if (context->WndHTheme)
        //            {
        //                DrawThemeBackground(
        //                    context->WndHTheme, 
        //                    hdc, 
        //                    EP_BACKGROUNDWITHBORDER, 
        //                    EBWBS_NORMAL, 
        //                    &context->rect, 
        //                    0
        //                    );
        //            }
        //            else
        //            {
        //                // Clear the draw region 
        //                FillRect(hdc, &context->rect, context->DcBrush);
        //                // Set border color
        //                FrameRect(hdc, &context->rect, context->BorderBrush);
        //            }

        //            // work out where to draw the button
        //            GetButtonRect(context, &context->rect);
        //            DrawInsertedButton(hwndDlg, context, hdc, &context->rect);   

        //            // cleanup
        //            ReleaseDC(hwndDlg, hdc);
        //        }
        //    }
        //    return FALSE;
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
                SetWindowPos(
                    hwndDlg,
                    0, 0, 0, 0, 0,
                    SWP_FRAMECHANGED | SWP_NOMOVE | SWP_NOSIZE | SWP_NOACTIVATE | SWP_NOZORDER
                    );
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

                SetWindowPos(
                    hwndDlg,
                    0, 0, 0, 0, 0,
                    SWP_FRAMECHANGED | SWP_NOMOVE | SWP_NOSIZE | SWP_NOACTIVATE | SWP_NOZORDER
                    );
            }
        }
        return FALSE;
    case WM_KEYUP:
        {   
            context->TextLength = Edit_GetTextLength(hwndDlg) > 0;

            // invalidate the nonclient area
            SetWindowPos(
                hwndDlg,
                0, 0, 0, 0, 0,
                SWP_FRAMECHANGED | SWP_NOMOVE | SWP_NOSIZE | SWP_NOACTIVATE | SWP_NOZORDER
                );
        }
        break;
    }

    return DefSubclassProc(hwndDlg, uMsg, wParam, lParam);
}

BOOLEAN InsertButton(
    __in HWND hwndDlg, 
    __in UINT CommandID
    )
{
    NC_CONTROL* context = (NC_CONTROL*)PhAllocate(sizeof(NC_CONTROL));
    
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

    context->uxthemeHandle = LoadLibrary(L"uxtheme.dll");
    IsThemeActive_I = (_IsThemeActive)GetProcAddress(context->uxthemeHandle, "IsThemeActive");
    OpenThemeData_I = (_OpenThemeData)GetProcAddress(context->uxthemeHandle, "OpenThemeData");
    CloseThemeData_I = (_CloseThemeData)GetProcAddress(context->uxthemeHandle, "CloseThemeData");
    IsThemePartDefined_I = (_IsThemePartDefined)GetProcAddress(context->uxthemeHandle, "IsThemePartDefined");
    DrawThemeBackground_I = (_DrawThemeBackground)GetProcAddress(context->uxthemeHandle, "DrawThemeBackground");
    DrawThemeText_I = (_DrawThemeText)GetProcAddress(context->uxthemeHandle, "DrawThemeText");
    GetThemeInt_I = (_GetThemeInt)GetProcAddress(context->uxthemeHandle, "GetThemeInt");
  
    context->WndHTheme = OpenThemeData_I(hwndDlg, VSCLASS_EDIT); // SearchBox, SearchEditBox, Edit::SearchBox, Edit::SearchEditBox
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
    SetWindowSubclass(hwndDlg, NcAreaWndSubclassProc, 0, 0);

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