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

typedef HRESULT (WINAPI *_GetThemeColor)(
    __in HTHEME hTheme,
    __in INT iPartId,
    __in INT iStateId,
    __in INT iPropId,
    __out COLORREF *pColor
    );
typedef HRESULT (WINAPI *_SetWindowTheme)(
    __in HWND hwnd,
    __in LPCWSTR pszSubAppName,
    __in LPCWSTR pszSubIdList
    );

typedef HRESULT (WINAPI *_GetThemeFont)(
    HTHEME hTheme,
    HDC hdc,
    int iPartId,
    int iStateId,
    int iPropId,
    _Out_ LOGFONTW *pFont
    );

typedef struct _NC_CONTROL
{
    UINT CommandID; // sent in a WM_COMMAND message
    UINT uState;

    INT cxLeftEdge; // size of the current window borders.
    INT cxRightEdge;  // size of the current window borders.
    INT cyTopEdge; 
    INT cyBottomEdge;

    SIZE ImgSize;
    RECT oldrect;
    RECT* prect;

    BOOL IsThemeActive;
    BOOL IsThemeBackgroundActive;
    HTHEME UxThemeData;
    HMODULE UxThemeModule;

    HIMAGELIST ImageList;
    HWND ParentWindow;
} *NC_CONTROL;

static _IsThemeActive IsThemeActive_I;
static _OpenThemeData OpenThemeData_I;
static _SetWindowTheme SetWindowTheme_I;
static _CloseThemeData CloseThemeData_I;
static _IsThemePartDefined IsThemePartDefined_I;
static _DrawThemeBackground DrawThemeBackground_I;
static _DrawThemeText DrawThemeText_I;
static _GetThemeInt GetThemeInt_I;
static _GetThemeColor GetThemeColor_I;
static _GetThemeFont GetThemeFont_I;

static VOID NcAreaInitializeUxTheme(
    __inout NC_CONTROL Context,
    __in HWND hwndDlg
    )
{
    if (!Context->UxThemeModule)
    {
        if ((Context->UxThemeModule = LoadLibrary(L"uxtheme.dll")) != NULL)
        {
            IsThemeActive_I = (_IsThemeActive)GetProcAddress(Context->UxThemeModule, "IsThemeActive");
            OpenThemeData_I = (_OpenThemeData)GetProcAddress(Context->UxThemeModule, "OpenThemeData");
            SetWindowTheme_I = (_SetWindowTheme)GetProcAddress(Context->UxThemeModule, "SetWindowTheme");
            CloseThemeData_I = (_CloseThemeData)GetProcAddress(Context->UxThemeModule, "CloseThemeData");
            IsThemePartDefined_I = (_IsThemePartDefined)GetProcAddress(Context->UxThemeModule, "IsThemePartDefined");
            DrawThemeBackground_I = (_DrawThemeBackground)GetProcAddress(Context->UxThemeModule, "DrawThemeBackground");
            DrawThemeText_I = (_DrawThemeText)GetProcAddress(Context->UxThemeModule, "DrawThemeText");
            GetThemeInt_I = (_GetThemeInt)GetProcAddress(Context->UxThemeModule, "GetThemeInt");
            GetThemeColor_I = (_GetThemeColor)GetProcAddress(Context->UxThemeModule, "GetThemeColor");
            GetThemeFont_I = (_GetThemeFont)GetProcAddress(Context->UxThemeModule, "GetThemeFont");
        }
    }

    if (IsThemeActive_I && 
        OpenThemeData_I && 
        CloseThemeData_I && 
        IsThemePartDefined_I && 
        DrawThemeBackground_I && 
        GetThemeInt_I)
    {
        Context->IsThemeActive = IsThemeActive_I();
 
        if (Context->UxThemeData)
        {
            CloseThemeData_I(Context->UxThemeData);
            Context->UxThemeData = NULL;
        }

        // UxTheme classes and themes:
        // OpenThemeData_I: 
        //    SearchBox
        //    SearchEditBox, 
        //    Edit::SearchBox
        //    Edit::SearchEditBox
        Context->UxThemeData = OpenThemeData_I(hwndDlg, VSCLASS_EDIT);

        // SetWindowTheme_I:  
        //    InactiveSearchBoxEdit
        //    InactiveSearchBoxEditComposited
        //    MaxInactiveSearchBoxEdit
        //    MaxInactiveSearchBoxEditComposited
        //    MaxSearchBoxEdit
        //    MaxSearchBoxEditComposited
        //    SearchBoxEdit
        //    SearchBoxEditComposited
        // SetWindowTheme_I(hwndDlg, L"SearchBoxEdit", NULL);

        if (Context->UxThemeData)
        {            
            Context->IsThemeBackgroundActive = IsThemePartDefined_I(
                Context->UxThemeData, 
                EP_BACKGROUND, 
                0
                );

            //COLORREF clrBackgroundRef;
            //HFONT hFont = NULL;
            //LOGFONT lf = { 0 };

            //if (SUCCEEDED(GetThemeFont_I(
            //    Context->UxThemeData,
            //    NULL,
            //    EP_EDITTEXT, 
            //    3,
            //    TMT_FONT,
            //    &lf
            //    )))
            //{
            //    hFont = CreateFontIndirect(&lf);
            //}

            //GetThemeColor_I(
            //    Context->UxThemeData, 
            //    EP_BACKGROUND,
            //    EBS_NORMAL, 
            //    TMT_BORDERCOLOR, 
            //    &clrBackgroundRef
            //    );
        }
        else
        {
            Context->IsThemeBackgroundActive = FALSE;
        }
    }
    else
    {
        Context->UxThemeData = NULL;
        Context->IsThemeActive = FALSE;
        Context->IsThemeBackgroundActive = FALSE;
    }
}

static VOID NcAreaGetButtonRect(
    __inout NC_CONTROL Context,
    __in RECT* rect
    )
{
    // retrieve the coordinates of an inserted button, given the specified window rectangle. 
    rect->right -= Context->cxRightEdge;
    rect->top += Context->cyTopEdge; // GetSystemMetrics(SM_CYBORDER);
    rect->bottom -= Context->cyBottomEdge;
    rect->left = rect->right - Context->ImgSize.cx; // GetSystemMetrics(SM_CXBORDER)

    if (Context->cxRightEdge > Context->cxLeftEdge)
        OffsetRect(rect, Context->cxRightEdge - Context->cxLeftEdge, 0);
}

static BOOLEAN NcAreaOnNcPaint(
    __in HWND hwndDlg,
    __in NC_CONTROL Context,
    __in_opt HRGN UpdateRegion
    )
{
    HDC hdc; 
    static RECT clientRect = { 0, 0, 0, 0 };
    static RECT windowRect = { 0, 0, 0, 0 };
    
    // Note the use of undocumented flags below. GetDCEx doesn't work without these.
    ULONG flags = DCX_WINDOW | DCX_LOCKWINDOWUPDATE | 0x10000;

    if (UpdateRegion == HRGN_FULL)
        UpdateRegion = NULL;

    if (UpdateRegion)
        flags |= DCX_INTERSECTRGN | 0x40000;

    if (hdc = GetDCEx(hwndDlg, UpdateRegion, flags))
    {
        // Get the screen coordinates of the client window
        GetClientRect(hwndDlg, &clientRect); 

        // Exclude the client area...
        ExcludeClipRect(
            hdc, 
            clientRect.left, 
            clientRect.top,
            clientRect.right,
            clientRect.bottom
            );

        // Get the screen coordinates of the window
        GetWindowRect(hwndDlg, &windowRect);
        // Adjust the coordinates - start from 0,0 
        OffsetRect(&windowRect, -windowRect.left, -windowRect.top); 

        // Draw the themed background.
        if (Context->IsThemeActive && Context->IsThemeBackgroundActive)
        {
            // Works better without??
            //DrawThemeBackground_I(
            //    Context->UxThemeData, 
            //    hdc, 
            //    EP_BACKGROUND, 
            //    EBS_NORMAL, 
            //    &windowRect, 
            //    NULL
            //    );
        }
        else
        {   
            FillRect(hdc, &windowRect, (HBRUSH)GetStockObject(DC_BRUSH));
        }

        SelectClipRgn(hdc, UpdateRegion);

        // get the position of the inserted button
        NcAreaGetButtonRect(Context, &windowRect);

        // Draw the button
        if (Edit_GetTextLength(hwndDlg) > 0)
        {
            ImageList_Draw(
                Context->ImageList, 
                0,     
                hdc, 
                windowRect.left,
                windowRect.top, 
                0
                );
        }
        else
        {
            ImageList_Draw(
                Context->ImageList, 
                1,
                hdc, 
                windowRect.left, 
                windowRect.top, 
                0
                );
        }

        // Restore the the client area...
        IntersectClipRect(
            hdc, 
            clientRect.left, 
            clientRect.top,
            clientRect.right, 
            clientRect.bottom
            );

        ReleaseDC(hwndDlg, hdc);
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
    NC_CONTROL context = (NC_CONTROL)GetProp(hwndDlg, L"Context");
              
    static POINT pt;   
    static RECT windowRect = { 0, 0, 0, 0 };

    if (context == NULL)
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

            // allocate space for the image by deflating the client window rectangle.
            context->prect->right -= context->ImgSize.cx; 
        }
        break;
    case WM_NCPAINT:
        {
            if (!NcAreaOnNcPaint(hwndDlg, context, (HRGN)wParam))
                return FALSE;
        }
        break;
    case WM_NCHITTEST:
        {
            // get the screen coordinates of the mouse
            pt.x = GET_X_LPARAM(lParam);
            pt.y = GET_Y_LPARAM(lParam);

            // get the position of the inserted button
            GetWindowRect(hwndDlg, &windowRect);
            NcAreaGetButtonRect(context, &windowRect);

            // check that the mouse is within the inserted button
            if (PtInRect(&windowRect, pt))
                return HTBORDER;
        }
        break;
    case WM_NCLBUTTONDOWN:
        {
            // get the screen coordinates of the mouse
            pt.x = GET_X_LPARAM(lParam);
            pt.y = GET_Y_LPARAM(lParam);

            // get the position of the inserted button
            GetWindowRect(hwndDlg, &windowRect);
            NcAreaGetButtonRect(context, &windowRect);

            // check that the mouse is within the inserted button
            if (PtInRect(&windowRect, pt))
            {
                SetCapture(hwndDlg);

                // Send the click notification to the parent window..
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
            pt.x = GET_X_LPARAM(lParam);
            pt.y = GET_Y_LPARAM(lParam);

            // Always release
            ReleaseCapture();

            // XP compat fix??
            //if (WindowsVersion < WINDOWS_VISTA)
            {
                ClientToScreen(hwndDlg, &pt);

                // get the position of the inserted button
                GetWindowRect(hwndDlg, &windowRect);
                NcAreaGetButtonRect(context, &windowRect);

                // check that the mouse is within the region
                if (PtInRect(&windowRect, pt))
                {
                    // Invalidate the nonclient area...
                    RedrawWindow(
                        hwndDlg, 
                        NULL, NULL, 
                        RDW_FRAME | RDW_INVALIDATE | RDW_ERASENOW | RDW_NOCHILDREN
                        );

                }
            }
        }
        return FALSE;
    case WM_KEYUP:  
        {   
            // Invalidate the nonclient area...
            RedrawWindow(
                hwndDlg, 
                NULL, NULL, 
                RDW_FRAME | RDW_INVALIDATE | RDW_ERASENOW | RDW_NOCHILDREN
                );
        }
        break;
    case WM_STYLECHANGED: 
    case WM_THEMECHANGED:
        NcAreaInitializeUxTheme(context, hwndDlg);
        break;
    }

    return DefSubclassProc(hwndDlg, uMsg, wParam, lParam);
}

BOOLEAN InsertButton(
    __in HWND hwndDlg, 
    __in UINT CommandID
    )
{
    NC_CONTROL context = (NC_CONTROL)PhAllocate(sizeof(struct _NC_CONTROL));
    memset(context, 0, sizeof(struct _NC_CONTROL));
     
    context->CommandID = CommandID;
    context->ImgSize.cx = 23;
    context->ImgSize.cy = 20;
    context->ImageList = ImageList_Create(24, 24, ILC_COLOR32 | ILC_MASK, 0, 0);
    context->ParentWindow = GetParent(hwndDlg);

    ImageList_SetImageCount(context->ImageList, 2);
    PhSetImageListBitmap(context->ImageList, 0, (HINSTANCE)PluginInstance->DllBase, MAKEINTRESOURCE(IDB_SEARCH1));
    PhSetImageListBitmap(context->ImageList, 1, (HINSTANCE)PluginInstance->DllBase, MAKEINTRESOURCE(IDB_SEARCH2));;

    NcAreaInitializeUxTheme(context, hwndDlg);

    // Set our window context data
    SetProp(hwndDlg, L"Context", (HANDLE)context);

    // Subclass the Edit control window procedure...
    SetWindowSubclass(hwndDlg, NcAreaWndSubclassProc, 0, (DWORD_PTR)context);

    // force the edit control to update its non-client area...
    RedrawWindow(
        hwndDlg, 
        NULL, NULL, 
        RDW_FRAME | RDW_INVALIDATE | RDW_ERASENOW | RDW_NOCHILDREN
        );

    return TRUE;
}