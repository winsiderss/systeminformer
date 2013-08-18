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
static _IsThemeBackgroundPartiallyTransparent IsThemeBackgroundPartiallyTransparent_I;
static _GetThemeColor GetThemeColor_I;

static VOID NcAreaInitializeUxTheme(
    __inout NC_CONTEXT* Context,
    __in HWND hwndDlg
    )
{
    if (!Context->UxThemeModule)
        Context->UxThemeModule = LoadLibrary(L"uxtheme.dll");

    if (Context->UxThemeModule)
    {
        IsThemeActive_I = (_IsThemeActive)GetProcAddress(Context->UxThemeModule, "IsThemeActive");
        OpenThemeData_I = (_OpenThemeData)GetProcAddress(Context->UxThemeModule, "OpenThemeData");
        SetWindowTheme_I = (_SetWindowTheme)GetProcAddress(Context->UxThemeModule, "SetWindowTheme");
        CloseThemeData_I = (_CloseThemeData)GetProcAddress(Context->UxThemeModule, "CloseThemeData");
        GetThemeColor_I = (_GetThemeColor)GetProcAddress(Context->UxThemeModule, "GetThemeColor");
        IsThemePartDefined_I = (_IsThemePartDefined)GetProcAddress(Context->UxThemeModule, "IsThemePartDefined");
        DrawThemeBackground_I = (_DrawThemeBackground)GetProcAddress(Context->UxThemeModule, "DrawThemeBackground");
        IsThemeBackgroundPartiallyTransparent_I = (_IsThemeBackgroundPartiallyTransparent)GetProcAddress(Context->UxThemeModule, "IsThemeBackgroundPartiallyTransparent");
    }

    if (IsThemeActive_I && OpenThemeData_I && CloseThemeData_I && IsThemePartDefined_I && DrawThemeBackground_I)
    {
        // UxTheme classes and themes (not all listed):
        // OpenThemeData_I:
        //    SearchBox
        //    SearchEditBox,
        //    Edit::SearchBox
        //    Edit::SearchEditBox
        if (Context->UxThemeHandle)
            CloseThemeData_I(Context->UxThemeHandle);

        Context->IsThemeActive = IsThemeActive_I();
        Context->UxThemeHandle = OpenThemeData_I(hwndDlg, VSCLASS_EDIT);

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

        if (Context->UxThemeHandle)
        {
            Context->IsThemeBackgroundActive = IsThemePartDefined_I(
                Context->UxThemeHandle,
                EP_EDITBORDER_NOSCROLL,
                0
                );

            GetThemeColor_I(
                Context->UxThemeHandle,
                EP_EDITBORDER_NOSCROLL,
                EPSN_NORMAL,
                TMT_FILLCOLOR,
                &Context->clrUxThemeFillRef
                );

            GetThemeColor_I(
                Context->UxThemeHandle,
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
        Context->UxThemeHandle = NULL;
        Context->IsThemeActive = FALSE;
        Context->IsThemeBackgroundActive = FALSE;
    }
}

static VOID NcAreaGetButtonRect(
    __inout NC_CONTEXT* Context,
    __in RECT* rect
    )
{
    // retrieve the coordinates of an inserted button, given the specified window rectangle.
    rect->right -= Context->cxRightEdge;
    rect->top += Context->cyTopEdge; // GetSystemMetrics(SM_CYBORDER);
    rect->bottom -= Context->cyBottomEdge;
    rect->left = rect->right - Context->cxImgSize; // GetSystemMetrics(SM_CXBORDER)

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
    NC_CONTEXT* context = (NC_CONTEXT*)GetProp(hwndDlg, L"Context");

    if (context == NULL)
        return FALSE;

    if (uMsg == WM_NCDESTROY)
    {
        // Cleanup the subclass data...

        if (context->ImageList)
        {
            ImageList_Destroy(context->ImageList);
            context->ImageList = NULL;
        }

        if (context->UxThemeHandle)
        {
            if (CloseThemeData_I)
            {
                CloseThemeData_I(context->UxThemeHandle);
                context->UxThemeHandle = NULL;
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
    case WM_STYLECHANGED:
    case WM_THEMECHANGED:
        NcAreaInitializeUxTheme(context, hwndDlg);
        break;
    case WM_NCCALCSIZE:
        {
            RECT* newClientRect = NULL;
            RECT oldClientRect = { 0, 0, 0, 0 };

            newClientRect = (RECT*)lParam;
            oldClientRect = *newClientRect;

            // calculate what the size of each window border is, so we can position the button...
            context->cxLeftEdge = newClientRect->left - oldClientRect.left;
            context->cxRightEdge = oldClientRect.right - newClientRect->right;
            context->cyTopEdge = newClientRect->top - oldClientRect.top;
            context->cyBottomEdge = oldClientRect.bottom - newClientRect->bottom;

            // allocate space for the image by deflating the client window rectangle.
            newClientRect->right -= context->cxImgSize;
        }
        break;
    case WM_NCPAINT:
        {
            HDC hdc = NULL;
            HRGN updateRegion = (HRGN)wParam;
            RECT clientRect = { 0, 0, 0, 0 };
            RECT windowRect = { 0, 0, 0, 0 };

            // Note the use of undocumented flags below. GetDCEx doesn't work without these.
            ULONG flags = DCX_WINDOW | DCX_LOCKWINDOWUPDATE | 0x10000;

            if (updateRegion == HRGN_FULL)
                updateRegion = NULL;

            if (updateRegion)
                flags |= DCX_INTERSECTRGN | 0x40000;

            if (!(hdc = GetDCEx(hwndDlg, updateRegion, flags)))
                return FALSE;

            //SelectClipRgn(hdc, updateRegion);
            SetBkMode(hdc, TRANSPARENT);

            // Get the screen coordinates of the client window.
            GetClientRect(hwndDlg, &clientRect);

            // Exclude the client area.
            ExcludeClipRect(
                hdc,
                clientRect.left,
                clientRect.top,
                clientRect.right,
                clientRect.bottom
                );

            // Get the screen coordinates of the window.
            GetWindowRect(hwndDlg, &windowRect);
            // Adjust the coordinates (start from 0,0).
            OffsetRect(&windowRect, -windowRect.left, -windowRect.top);

            // Draw the themed background.
            if (context->IsThemeActive && context->IsThemeBackgroundActive)
            {
                BOOL isFocused = (GetFocus() == hwndDlg);

                if (isFocused)
                {
                    DrawThemeBackground_I(
                        context->UxThemeHandle,
                        hdc,
                        EP_EDITBORDER_NOSCROLL,
                        EPSN_FOCUSED,
                        &windowRect,
                        NULL
                        );
                }
                else
                {
                    DrawThemeBackground_I(
                        context->UxThemeHandle,
                        hdc,
                        EP_EDITBORDER_NOSCROLL,
                        EPSN_NORMAL,
                        &windowRect,
                        NULL
                        );
                }
            }
            else
            {
                // Fill in the text box.
                SetDCBrushColor(hdc, RGB(0xff, 0xff, 0xff));
                FillRect(hdc, &windowRect, GetStockBrush(DC_BRUSH));
            }

            // Get the position of the inserted button.
            NcAreaGetButtonRect(context, &windowRect);

            // Draw the button.
            if (GetWindowTextLength(hwndDlg) > 0)
            {
                ImageList_Draw(
                    context->ImageList,
                    0,
                    hdc,
                    windowRect.left,
                    windowRect.top,
                    ILD_TRANSPARENT
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
                    ILD_TRANSPARENT
                    );
            }

            // Restore the the client area.
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
            POINT windowPoint = { 0, 0 };
            RECT windowRect = { 0, 0, 0, 0 };

            // Get the screen coordinates of the mouse.
            windowPoint.x = GET_X_LPARAM(lParam);
            windowPoint.y = GET_Y_LPARAM(lParam);

            // Get the position of the inserted button.
            GetWindowRect(hwndDlg, &windowRect);
            NcAreaGetButtonRect(context, &windowRect);

            // Check that the mouse is within the inserted button.
            if (PtInRect(&windowRect, windowPoint))
                return HTBORDER;
        }
        break;
    case WM_NCLBUTTONDOWN:
        {
            POINT windowPoint = { 0, 0 };
            RECT windowRect = { 0, 0, 0, 0 };

            // Get the screen coordinates of the mouse.
            windowPoint.x = GET_X_LPARAM(lParam);
            windowPoint.y = GET_Y_LPARAM(lParam);

            // Get the position of the inserted button.
            GetWindowRect(hwndDlg, &windowRect);
            NcAreaGetButtonRect(context, &windowRect);

            // Check that the mouse is within the button rect.
            if (PtInRect(&windowRect, windowPoint))
            {
                context->HasCapture = TRUE;
                SetCapture(hwndDlg);

                // Send the click notification to the parent window.
                PostMessage(
                    context->ParentWindow,
                    WM_COMMAND,
                    MAKEWPARAM(context->CommandID, BN_CLICKED),
                    0
                    );

                // Invalidate the nonclient area.
                RedrawWindow(hwndDlg, NULL, NULL, RDW_FRAME | RDW_INVALIDATE);
                return FALSE;
            }
        }
        break;
    case WM_LBUTTONUP:
        {
            if (context->HasCapture)
            {
                ReleaseCapture();
                context->HasCapture = FALSE;

                // Invalidate the nonclient area.
                RedrawWindow(hwndDlg, NULL, NULL, RDW_FRAME | RDW_INVALIDATE);
                return FALSE;
            }
        }
        break;
    case WM_UNDO:
    case WM_PASTE:
    case EN_CHANGE: // Same value as WM_CUT...
    case WM_CLEAR:
        RedrawWindow(hwndDlg, NULL, NULL, RDW_FRAME | RDW_INVALIDATE);
        break;
    case WM_KEYUP:
        RedrawWindow(hwndDlg, NULL, NULL, RDW_FRAME | RDW_INVALIDATE);
        return FALSE;
    }

    return DefSubclassProc(hwndDlg, uMsg, wParam, lParam);
}



static HBITMAP LoadImageFromResources(
    __in UINT Width,
    __in UINT Height,
    __in LPCTSTR Name,
    __in LPCTSTR Type
    )
{
    UINT width = 0;
    UINT height = 0;
    UINT frameCount = 0;
    ULONG resLength = 0;
    HRSRC resHandleSrc = NULL;
    HGLOBAL resHandle = NULL;
    BITMAPINFO bitmapInfo = { 0 };
    HBITMAP bitmapHandle = NULL;
    BYTE* bitmapBuffer = NULL;
    WICInProcPointer resBuffer = NULL;

    IWICStream* wicStream = NULL;
    IWICBitmapSource* wicBitmap = NULL;
    IWICBitmapDecoder* wicDecoder = NULL;
    IWICBitmapFrameDecode* wicFrame = NULL;
    IWICImagingFactory* wicFactory = NULL;
    IWICBitmapScaler* wicScaler = NULL;

    HDC hdcScreen = GetDC(NULL);

    __try
    {
        if ((resHandleSrc = FindResource((HINSTANCE)PluginInstance->DllBase, Name, Type)) == NULL)
            __leave;
        if ((resHandle = LoadResource((HINSTANCE)PluginInstance->DllBase, resHandleSrc)) == NULL)
            __leave;
        if ((resBuffer = (WICInProcPointer)LockResource(resHandle)) == NULL)
            __leave;

        resLength = SizeofResource((HINSTANCE)PluginInstance->DllBase, resHandleSrc);

        if (FAILED(CoCreateInstance(&CLSID_WICImagingFactory1, NULL, CLSCTX_INPROC_SERVER, &IID_IWICImagingFactory, (PVOID*)&wicFactory)))
            __leave;
        if (FAILED(CoCreateInstance(&CLSID_WICPngDecoder1, NULL, CLSCTX_INPROC_SERVER, &IID_IWICBitmapDecoder, (PVOID*)&wicDecoder)))
            __leave;
        if (FAILED(IWICImagingFactory_CreateStream(wicFactory, &wicStream)))
            __leave;
        if (FAILED(IWICStream_InitializeFromMemory(wicStream, resBuffer, resLength)))
            __leave;
        if (FAILED(IWICBitmapDecoder_Initialize(wicDecoder, (IStream*)wicStream, WICDecodeMetadataCacheOnLoad)))
            __leave;
        if (FAILED(IWICBitmapDecoder_GetFrameCount(wicDecoder, &frameCount)) || frameCount != 1)
            __leave;
        if (FAILED(IWICBitmapDecoder_GetFrame(wicDecoder, 0, &wicFrame)))
            __leave;
        if (FAILED(WICConvertBitmapSource(&GUID_WICPixelFormat32bppBGR, (IWICBitmapSource*)wicFrame, &wicBitmap)))
            __leave;
        if (FAILED(IWICBitmapSource_GetSize(wicBitmap, &width, &height)) || width == 0 || height == 0)
            __leave;

        bitmapInfo.bmiHeader.biSize = sizeof(BITMAPINFOHEADER);
        bitmapInfo.bmiHeader.biWidth = Width;
        bitmapInfo.bmiHeader.biHeight = -((LONG)Height);
        bitmapInfo.bmiHeader.biPlanes = 1;
        bitmapInfo.bmiHeader.biBitCount = 32;
        bitmapInfo.bmiHeader.biCompression = BI_RGB;

        if ((bitmapHandle = CreateDIBSection(hdcScreen, &bitmapInfo, DIB_RGB_COLORS, (PVOID*)&bitmapBuffer, NULL, 0)) != NULL)
        {
            WICRect rect = { 0, 0, Width, Height };

            if (FAILED(IWICImagingFactory_CreateBitmapScaler(wicFactory, &wicScaler)))
                __leave;
            if (FAILED(IWICBitmapScaler_Initialize(wicScaler, wicBitmap, Width, Height, WICBitmapInterpolationModeFant)))
                __leave;
            if (SUCCEEDED(IWICBitmapScaler_CopyPixels(wicScaler, &rect, Width * 4, Width * Height * 4, bitmapBuffer)))
                __leave;
        }
    }
    __finally
    {
        ReleaseDC(NULL, hdcScreen);

        if (wicScaler)
        {
            IWICBitmapScaler_Release(wicScaler);
        }

        if (wicBitmap)
        {
            IWICBitmapSource_Release(wicBitmap);
        }

        if (wicFrame)
        {
            IWICBitmapFrameDecode_Release(wicFrame);
        }

        if (wicStream)
        {
            IWICStream_Release(wicStream);
        }

        if (wicDecoder)
        {
            IWICBitmapDecoder_Release(wicDecoder);
        }

        if (wicFactory)
        {
            IWICImagingFactory_Release(wicFactory);
        }
    }

    return bitmapHandle;
}

static HFONT InitializeFont(
    __in HWND hwndDlg
    )
{
    LOGFONT logFont = { 0 };
    HFONT fontHandle = NULL;

    logFont.lfHeight = 14;
    logFont.lfWeight = FW_NORMAL;
    logFont.lfQuality = CLEARTYPE_QUALITY | ANTIALIASED_QUALITY;
    
    // GDI uses the first font that matches the above attributes.
    fontHandle = CreateFontIndirect(&logFont);

    if (fontHandle)
    {
        SendMessage(hwndDlg, WM_SETFONT, (WPARAM)fontHandle, FALSE);
        return fontHandle;
    }

    return NULL;
}


BOOLEAN InsertButton(
    __in HWND hwndDlg,
    __in UINT CommandID
    )
{
    NC_CONTEXT* context = (NC_CONTEXT*)PhAllocate(sizeof(NC_CONTEXT));
    memset(context, 0, sizeof(NC_CONTEXT));

    context->cxImgSize = 22;
    context->CommandID = CommandID;
    context->ParentWindow = GetParent(hwndDlg);
    context->ImageList = ImageList_Create(32, 32, ILC_COLOR32 | ILC_MASK, 0, 0);

    ImageList_SetImageCount(context->ImageList, 2);
    ImageList_Replace(context->ImageList, 0, LoadImageFromResources(23, 20, MAKEINTRESOURCE(IDB_SEARCH1), L"PNG"), NULL);
    ImageList_Replace(context->ImageList, 1, LoadImageFromResources(23, 20, MAKEINTRESOURCE(IDB_SEARCH2), L"PNG"), NULL);

    // Initialize the window UxTheme data.
    NcAreaInitializeUxTheme(context, hwndDlg);

            
    // Set Searchbox control font
    SearchboxFontHandle = InitializeFont(hwndDlg);

    // Set our window context data.
    SetProp(hwndDlg, L"Context", (HANDLE)context);

    // Subclass the Edit control window procedure.
    SetWindowSubclass(hwndDlg, NcAreaWndSubclassProc, 0, (DWORD_PTR)context);

    // Force the edit control to update its non-client area.
    RedrawWindow(hwndDlg, NULL, NULL, RDW_FRAME | RDW_INVALIDATE);
    return TRUE;
}