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
#define CINTERFACE
#define COBJMACROS
#define INITGUID

#include "toolstatus.h"
#include "searchbox.h"

#include <Wincodec.h>
#include <uxtheme.h>
#include <vsstyle.h>
#include <vssym32.h>

#pragma comment(lib, "windowscodecs.lib")

#define HRGN_FULL ((HRGN)1) // passed by WM_NCPAINT even though it's completely undocumented

static _IsThemeActive IsThemeActive_I;
static _OpenThemeData OpenThemeData_I;
static _SetWindowTheme SetWindowTheme_I;
static _CloseThemeData CloseThemeData_I;
static _IsThemePartDefined IsThemePartDefined_I;
static _DrawThemeBackground DrawThemeBackground_I;
static _GetThemeBackgroundContentRect GetThemeBackgroundContentRect_I;
static _IsThemeBackgroundPartiallyTransparent IsThemeBackgroundPartiallyTransparent_I;
static _DrawThemeText DrawThemeText_I;
static _GetThemeInt GetThemeInt_I;
static _GetThemeColor GetThemeColor_I;
static _GetThemeFont GetThemeFont_I;
static _GetThemeSysFont GetThemeSysFont_I;

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
            IsThemeBackgroundPartiallyTransparent_I = (_IsThemeBackgroundPartiallyTransparent)GetProcAddress(Context->UxThemeModule, "IsThemeBackgroundPartiallyTransparent");
            GetThemeBackgroundContentRect_I = (_GetThemeBackgroundContentRect)GetProcAddress(Context->UxThemeModule, "GetThemeBackgroundContentRect"); 
            DrawThemeText_I = (_DrawThemeText)GetProcAddress(Context->UxThemeModule, "DrawThemeText");
            GetThemeInt_I = (_GetThemeInt)GetProcAddress(Context->UxThemeModule, "GetThemeInt");
            GetThemeColor_I = (_GetThemeColor)GetProcAddress(Context->UxThemeModule, "GetThemeColor");
            GetThemeFont_I = (_GetThemeFont)GetProcAddress(Context->UxThemeModule, "GetThemeFont");
            GetThemeSysFont_I = (_GetThemeSysFont)GetProcAddress(Context->UxThemeModule, "GetThemeSysFont");
        }
    }

    if (IsThemeActive_I && OpenThemeData_I && 
        CloseThemeData_I && IsThemePartDefined_I && 
        DrawThemeBackground_I && GetThemeInt_I
        )
    {
        // UxTheme classes and themes (not all listed):
        // OpenThemeData_I: 
        //    SearchBox
        //    SearchEditBox, 
        //    Edit::SearchBox
        //    Edit::SearchEditBox
        if (Context->UxThemeData)
            CloseThemeData_I(Context->UxThemeData);

        Context->IsThemeActive = IsThemeActive_I();
        Context->UxThemeData = OpenThemeData_I(hwndDlg, VSCLASS_EDIT);

        // Edit control classes and themes (not all listed):
        // SetWindowTheme_I:  
        //    InactiveSearchBoxEdit
        //    InactiveSearchBoxEditComposited
        //    MaxInactiveSearchBoxEdit
        //    MaxInactiveSearchBoxEditComposited
        //    MaxSearchBoxEdit
        //    MaxSearchBoxEditComposited
        //    SearchBoxEdit
        //    SearchBoxEditComposited
        //SetWindowTheme_I(hwndDlg, L"SearchBoxEdit", NULL);

        if (Context->UxThemeData)
        {
            Context->IsThemeBackgroundActive = IsThemePartDefined_I(
                Context->UxThemeData, 
                EP_EDITBORDER_NOSCROLL, 
                0
                );

            GetThemeColor_I(
                Context->UxThemeData, 
                EP_EDITBORDER_NOSCROLL,
                EPSN_NORMAL, 
                TMT_FILLCOLOR, 
                &Context->clrUxThemeFillRef
                );

            GetThemeColor_I(
                Context->UxThemeData, 
                EP_EDITBORDER_NOSCROLL,
                EPSN_NORMAL, 
                TMT_BORDERCOLOR, 
                &Context->clrUxThemeBackgroundRef
                );
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
            HDC hdc = NULL;
            HRGN UpdateRegion = (HRGN)wParam;
            static RECT clientRect = { 0, 0, 0, 0 };
            static RECT windowRect = { 0, 0, 0, 0 };

            // Note the use of undocumented flags below. GetDCEx doesn't work without these.
            ULONG flags = DCX_WINDOW | DCX_LOCKWINDOWUPDATE | 0x10000;

            if (UpdateRegion == HRGN_FULL)
                UpdateRegion = NULL;

            if (UpdateRegion)
                flags |= DCX_INTERSECTRGN | 0x40000;

            if (!(hdc = GetDCEx(hwndDlg, UpdateRegion, flags)))
                return FALSE;

            SelectClipRgn(hdc, UpdateRegion);

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
            //if (context->IsThemeActive && context->IsThemeBackgroundActive)
            //{
            //    // Works better without?
            DrawThemeBackground_I(
                context->UxThemeData, 
                hdc, 
                EP_BACKGROUND, 
                EBS_NORMAL,
                &windowRect,
                &clientRect
                );
            //}

            //else
            FillRect(hdc, &windowRect, (HBRUSH)GetStockObject(DC_BRUSH));

            // get the position of the inserted button
            NcAreaGetButtonRect(context, &windowRect);

            // Draw the button
            if (GetWindowTextLength(hwndDlg) > 0)
            {
                ImageList_Draw(
                    context->ImageList, 
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
                    context->ImageList, 
                    1,
                    hdc, 
                    windowRect.left, 
                    windowRect.top + 1, 
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
        }
        break;
    case WM_NCHITTEST:
        {
            // get the screen coordinates of the mouse
            context->pt.x = GET_X_LPARAM(lParam);
            context->pt.y = GET_Y_LPARAM(lParam);

            // get the position of the inserted button
            GetWindowRect(hwndDlg, &context->windowRect);
            NcAreaGetButtonRect(context, &context->windowRect);

            // check that the mouse is within the inserted button
            if (PtInRect(&context->windowRect, context->pt))
                return HTBORDER;
        }
        break;
    case WM_NCLBUTTONDOWN:
        {
            // get the screen coordinates of the mouse
            context->pt.x = GET_X_LPARAM(lParam);
            context->pt.y = GET_Y_LPARAM(lParam);

            // get the position of the inserted button
            GetWindowRect(hwndDlg, &context->windowRect);
            NcAreaGetButtonRect(context, &context->windowRect);

            // check that the mouse is within the button rect
            if (PtInRect(&context->windowRect, context->pt))
            {
                context->hasCapture = TRUE;
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
                return FALSE;
            }
        }
        break;
    case WM_LBUTTONUP:
        {
            if (context->hasCapture)
            {
                ReleaseCapture();
                context->hasCapture = FALSE;

                // Invalidate the nonclient area...
                RedrawWindow(
                    hwndDlg, 
                    NULL, NULL, 
                    RDW_FRAME | RDW_INVALIDATE | RDW_ERASENOW | RDW_NOCHILDREN
                    );

                return FALSE;
            }
        }
        break;
    case EN_CHANGE:
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
   
HBITMAP LoadImageFromResources(
    __in LPCTSTR lpName, 
    __in LPCTSTR lpType
    )
{
    UINT nFrameCount = 0;
    UINT width = 0;
    UINT height = 0;
    DWORD dwResourceSize = 0;
    BITMAPINFO bminfo = { 0 };

    HRSRC resHandleRef = NULL;
    HGLOBAL resHandle = NULL;

    HBITMAP bitmapHandle = NULL;
    PVOID pvImageBits = NULL; 

    IWICStream* wicStream = NULL;
    IWICBitmapSource* wicBitmap = NULL;
    IWICBitmapDecoder* wicDecoder = NULL;
    IWICBitmapFrameDecode* wicFrame = NULL;
    IWICImagingFactory* wicFactory = NULL;
    IWICBitmapScaler* wicScaler = NULL;
    WICInProcPointer pvSourceResourceData = NULL;

    HDC hdcScreen = GetDC(NULL);

    __try
    {   
        if ((resHandleRef = FindResource((HINSTANCE)PluginInstance->DllBase, lpName, lpType)) == NULL)
            __leave;
        if ((resHandle = LoadResource((HINSTANCE)PluginInstance->DllBase, resHandleRef)) == NULL)
            __leave;
        if ((pvSourceResourceData = (WICInProcPointer)LockResource(resHandle)) == NULL)
            __leave;

        dwResourceSize = SizeofResource((HINSTANCE)PluginInstance->DllBase, resHandleRef);

        if (FAILED(CoCreateInstance(&CLSID_WICImagingFactory1, NULL, CLSCTX_INPROC_SERVER, &IID_IWICImagingFactory, (void**)&wicFactory)))
            __leave;
        if (FAILED(CoCreateInstance(&CLSID_WICPngDecoder1, NULL, CLSCTX_INPROC_SERVER, &IID_IWICBitmapDecoder, (void**)&wicDecoder)))
            __leave;
        if (FAILED(IWICImagingFactory_CreateStream(wicFactory, &wicStream)))
            __leave;
        if (FAILED(IWICStream_InitializeFromMemory(wicStream, pvSourceResourceData, dwResourceSize)))
            __leave;
        if (FAILED(IWICBitmapDecoder_Initialize(wicDecoder, (IStream*)wicStream, WICDecodeMetadataCacheOnLoad)))
            __leave;
        if (FAILED(IWICBitmapDecoder_GetFrameCount(wicDecoder, &nFrameCount)) || nFrameCount != 1)
            __leave;
        if (FAILED(IWICBitmapDecoder_GetFrame(wicDecoder, 0, &wicFrame)))
            __leave;

        if (FAILED(WICConvertBitmapSource(&GUID_WICPixelFormat32bppPBGRA, (IWICBitmapSource*)wicFrame, &wicBitmap)))
            __leave;
        if (FAILED(IWICBitmapSource_GetSize(wicBitmap, &width, &height)) || width == 0 || height == 0)
            __leave;

        bminfo.bmiHeader.biSize = sizeof(BITMAPINFOHEADER);
        bminfo.bmiHeader.biWidth = width;
        bminfo.bmiHeader.biHeight = -((LONG)height);
        bminfo.bmiHeader.biPlanes = 1;
        bminfo.bmiHeader.biBitCount = 32;
        bminfo.bmiHeader.biCompression = BI_RGB;

        if ((bitmapHandle = CreateDIBSection(hdcScreen, &bminfo, DIB_RGB_COLORS, &pvImageBits, NULL, 0)) != NULL)
        {  
            WICRect rect = { 0, 0, width, height };          

            if (FAILED(IWICImagingFactory_CreateBitmapScaler(wicFactory, &wicScaler)))
                __leave;

            if (FAILED(IWICBitmapScaler_Initialize(wicScaler, (IWICBitmapSource*)wicFrame, width, height, WICBitmapInterpolationModeFant)))
                __leave;
            if (SUCCEEDED(IWICBitmapScaler_CopyPixels(wicScaler, &rect, width * 4, width * height * 4, (BYTE*)pvImageBits)))
                __leave;
        }
   
        DeleteObject(bitmapHandle);
        bitmapHandle = NULL;
    }
    __finally
    {
        ReleaseDC(NULL, hdcScreen);

        IWICBitmapScaler_Release(wicScaler);
        IWICBitmapSource_Release(wicBitmap);
        IWICBitmapFrameDecode_Release(wicFrame);
        IWICStream_Release(wicStream);
        IWICBitmapDecoder_Release(wicDecoder);
        IWICImagingFactory_Release(wicFactory);
    }

    return bitmapHandle;
}

BOOLEAN InsertButton(
    __in HWND hwndDlg, 
    __in UINT CommandID
    )
{
    NC_CONTROL context = (NC_CONTROL)PhAllocate(sizeof(struct _NC_CONTROL));

    memset(context, 0, sizeof(struct _NC_CONTROL));

    context->CommandID = CommandID;
    context->ImgSize.cx = 22;
    context->ImgSize.cy = 22;
    context->ImageList = ImageList_Create(22, 22, ILC_COLOR32 | ILC_MASK, 0, 0);
    context->ParentWindow = GetParent(hwndDlg);

    ImageList_SetImageCount(context->ImageList, 2);
    ImageList_Replace(context->ImageList, 0, LoadImageFromResources(MAKEINTRESOURCE(IDB_SEARCH1), L"PNG"), NULL);
    ImageList_Replace(context->ImageList, 1, LoadImageFromResources(MAKEINTRESOURCE(IDB_SEARCH2), L"PNG"), NULL);

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