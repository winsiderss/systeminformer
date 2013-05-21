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

static _IsThemeActive IsThemeActive_I;
static _OpenThemeData OpenThemeData_I;
static _CloseThemeData CloseThemeData_I;
static _IsThemePartDefined IsThemePartDefined_I;
static _DrawThemeBackground DrawThemeBackground_I;
static _DrawThemeText DrawThemeText_I;
static _GetThemeInt GetThemeInt_I;

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

    BOOL IsThemeActive;
    BOOL IsThemeBackgroundActive;
    HTHEME UxThemeData;
    HMODULE UxThemeModule;

    HIMAGELIST ImageList;
    HWND ParentWindow;
} NC_CONTROL, *PNCA_CONTROL;

static VOID GetButtonRect(
    __inout PNCA_CONTROL Context,
    __in RECT* rect
    )
{
    // retrieve the coordinates of an inserted button, given the specified window rectangle. 
    rect->right -= Context->cxRightEdge;
    rect->top += Context->cyTopEdge;
    rect->bottom -= Context->cyBottomEdge;
    rect->left = rect->right - Context->nButSize;

    if (Context->cxRightEdge > Context->cxLeftEdge)
        OffsetRect(rect, Context->cxRightEdge - Context->cxLeftEdge, 0);
}   

static VOID DrawInsertedButton(
    __in HWND hwndDlg, 
    __inout NC_CONTROL* Context, 
    __in HDC HdcHandle,
    __in RECT* prect
    )
{
    // Draw the search image...
    if (Context->TextLength > 0)
    {
        ImageList_DrawEx(
            Context->ImageList, 
            0,     
            HdcHandle, 
            prect->left + 1,
            prect->top + 1, 
            0, 0,
            CLR_NONE,
            CLR_NONE,
            ILD_NORMAL | ILD_SCALE
            );
    }
    else
    {
        ImageList_DrawEx(
            Context->ImageList, 
            1,
            HdcHandle, 
            prect->left + 1, 
            prect->top + 1, 
            0, 0,
            CLR_NONE,
            CLR_NONE,
            ILD_NORMAL | ILD_SCALE
            );
    }
}

static VOID PhTnpDrawThemedBorder(
    __in HWND hwnd,
    __in NC_CONTROL* Context,
    __in HDC hdc
    )
{
    static RECT clientRect = { 0, 0, 0, 0 };
    static RECT windowRect = { 0, 0, 0, 0 };

    // get the screen coordinates of the client area
    GetClientRect(hwnd, &clientRect);        
    // Exclude the client area
    ExcludeClipRect(hdc, clientRect.left, clientRect.top, clientRect.right, clientRect.bottom);

    // get the screen coordinates of the Window area
    GetWindowRect(hwnd, &windowRect);
    // adjust the coordinates - start from 0,0
    OffsetRect(&windowRect, -windowRect.left, -windowRect.top);   

    // Draw the themed background
    if (DrawThemeBackground_I && Context->IsThemeBackgroundActive)
    {
        DrawThemeBackground_I(Context->UxThemeData, hdc, EP_EDITBORDER_NOSCROLL, 0, &windowRect, NULL);
    }
    else
    {
        FillRect(hdc, &windowRect, (HBRUSH)GetStockObject(DC_BRUSH)); //GetSysColorBrush(COLOR_WINDOW)
    }

    // work out where to draw the button
    GetButtonRect(Context, &windowRect);
    DrawInsertedButton(hwnd, Context, hdc, &windowRect);

    // Restore the the client area
    IntersectClipRect(hdc, clientRect.left, clientRect.top, clientRect.right, clientRect.bottom);
}

static BOOLEAN PhTnpOnNcPaint(
    __in HWND hwnd,
    __in NC_CONTROL* Context,
    __in_opt HRGN UpdateRegion
    )
{
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

static LRESULT CALLBACK NcAreaWndSubclassProc(
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
        if (context->ImageList)
        {
            ImageList_Destroy(context->ImageList);
            context->ImageList = NULL;
        }

        if (context->UxThemeData)
        {
            if (CloseThemeData_I)
            {
                CloseThemeData_I(context->UxThemeData);
                context->UxThemeData = NULL;
            }
        }

        if (context->UxThemeModule)
        {
            FreeLibrary(context->UxThemeModule);
            context->UxThemeModule = NULL;
        }

        RemoveWindowSubclass(hwndDlg, NcAreaWndSubclassProc, 0);
        RemoveProp(hwndDlg, L"Context");
        PhFree(context);

        return FALSE;
    }

    switch (uMsg)
    {
    case WM_NCCALCSIZE:
        {
            NCCALCSIZE_PARAMS* nccsp = (NCCALCSIZE_PARAMS*)lParam;

            context->prect = (RECT*)lParam;
            context->oldrect = *context->prect;

            // calculate what the size of each window border is, so we can position the button...
            context->cxLeftEdge = context->prect->left - context->oldrect.left; 
            context->cxRightEdge = context->oldrect.right - context->prect->right;
            context->cyTopEdge = context->prect->top - context->oldrect.top;
            context->cyBottomEdge = context->oldrect.bottom - context->prect->bottom;   

            // now we can allocate additional space by deflating the
            // rectangle even further. Our button will go on the right-hand side,
            // and will be the same width as a scrollbar button
            context->prect->right -= context->nButSize; 
        }
        break;
    case WM_NCPAINT:
        {
            context->TextLength = Edit_GetTextLength(hwndDlg);

            if (!PhTnpOnNcPaint(hwndDlg, context, (HRGN)wParam))
                return FALSE;
        }
        break;
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

                // Invalidate the nonclient area...
                RedrawWindow(
                    hwndDlg, 
                    NULL, NULL,
                    RDW_FRAME | RDW_INVALIDATE | RDW_ERASENOW | RDW_NOCHILDREN
                    );
            }
        }
        return FALSE;
    case WM_LBUTTONUP:
        {
            // get the SCREEN coordinates of the mouse
            context->pt.x = GET_X_LPARAM(lParam);
            context->pt.y = GET_Y_LPARAM(lParam);

            // Always release
            ReleaseCapture();

            ClientToScreen(hwndDlg, &context->pt);
            GetWindowRect(hwndDlg, &context->rect);

            context->pt.x -= context->rect.left;
            context->pt.y -= context->rect.top;

            OffsetRect(&context->rect, -context->rect.left, -context->rect.top);
            GetButtonRect(context, &context->rect);

            // check that the mouse is within the region
            if (PtInRect(&context->rect, context->pt))
            {
                context->TextLength = Edit_GetTextLength(hwndDlg);

                // Invalidate the nonclient area...
                RedrawWindow(
                    hwndDlg, 
                    NULL, NULL, 
                    RDW_FRAME | RDW_INVALIDATE | RDW_ERASENOW | RDW_NOCHILDREN
                    );
            }
        }
        return FALSE;
    case WM_KEYUP:  
        {   
            context->TextLength = Edit_GetTextLength(hwndDlg);

            // Invalidate the nonclient area...
            RedrawWindow(
                hwndDlg, 
                NULL, NULL, 
                RDW_FRAME | RDW_INVALIDATE | RDW_ERASENOW | RDW_NOCHILDREN
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
    // search image sizes are 23x20
    context->nButSize = 22;
    context->ImageList = ImageList_Create(17, 17, ILC_COLOR32 | ILC_MASK, 0, 0);
    // Set the number of images
    ImageList_SetImageCount(context->ImageList, 2);
    PhSetImageListBitmap(context->ImageList, 0, (HINSTANCE)PluginInstance->DllBase, MAKEINTRESOURCE(IDB_SEARCH1));
    PhSetImageListBitmap(context->ImageList, 1, (HINSTANCE)PluginInstance->DllBase, MAKEINTRESOURCE(IDB_SEARCH2));;

    if ((context->UxThemeModule = LoadLibrary(L"uxtheme.dll")) != NULL)
    {
        IsThemeActive_I = (_IsThemeActive)GetProcAddress(context->UxThemeModule, "IsThemeActive");
        OpenThemeData_I = (_OpenThemeData)GetProcAddress(context->UxThemeModule, "OpenThemeData");
        CloseThemeData_I = (_CloseThemeData)GetProcAddress(context->UxThemeModule, "CloseThemeData");
        IsThemePartDefined_I = (_IsThemePartDefined)GetProcAddress(context->UxThemeModule, "IsThemePartDefined");
        DrawThemeBackground_I = (_DrawThemeBackground)GetProcAddress(context->UxThemeModule, "DrawThemeBackground");
        DrawThemeText_I = (_DrawThemeText)GetProcAddress(context->UxThemeModule, "DrawThemeText");
        GetThemeInt_I = (_GetThemeInt)GetProcAddress(context->UxThemeModule, "GetThemeInt");
    }

    if (IsThemeActive_I && 
        OpenThemeData_I && 
        CloseThemeData_I && 
        IsThemePartDefined_I && 
        DrawThemeBackground_I && 
        GetThemeInt_I)
    {
        context->IsThemeActive = IsThemeActive_I();
        context->UxThemeData = OpenThemeData_I(
            hwndDlg,
            VSCLASS_EDIT
            );

        // OpenThemeData_I: 
        //    SearchBox
        //    SearchEditBox, 
        //    Edit::SearchBox
        //    Edit::SearchEditBox
        // SetWindowTheme themes:  
        //    InactiveSearchBoxEdit
        //    InactiveSearchBoxEditComposited
        //    MaxInactiveSearchBoxEdit
        //    MaxInactiveSearchBoxEditComposited
        //    MaxSearchBoxEdit
        //    MaxSearchBoxEditComposited
        //    SearchBoxEdit
        //    SearchBoxEditComposited

        if (context->UxThemeData)
        {
            context->IsThemeBackgroundActive = IsThemePartDefined_I(
                context->UxThemeData, 
                EP_EDITBORDER_NOSCROLL, 
                0
                );
        }
        else
        {
            context->IsThemeBackgroundActive = FALSE;
        }
    }
    else
    {
        context->UxThemeData = NULL;
        context->IsThemeActive = FALSE;
        context->IsThemeBackgroundActive = FALSE;
    }

    // Set our window context data
    SetProp(hwndDlg, L"Context", (HANDLE)context);

    // Subclass the Edit control window procedure...
    SetWindowSubclass(hwndDlg, NcAreaWndSubclassProc, 0, 0);

    // force the edit control to update its non-client area...
    RedrawWindow(
        hwndDlg, 
        NULL, NULL, 
        RDW_FRAME | RDW_INVALIDATE | RDW_ERASENOW | RDW_NOCHILDREN
        );

    return TRUE;
}