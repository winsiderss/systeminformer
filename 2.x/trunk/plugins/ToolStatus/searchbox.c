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
*
* dmex: The non-client area subclassing code has been modified based on the following guide:
* http://www.catch22.net/tuts/insert-buttons-edit-control
*/

#include "toolstatus.h"

#ifdef _UXTHEME_ENABLED_
#include <uxtheme.h>
#include <vsstyle.h>
#include <vssym32.h>

typedef HRESULT (WINAPI *_GetThemeColor)(
    _In_ HTHEME hTheme,
    _In_ INT iPartId,
    _In_ INT iStateId,
    _In_ INT iPropId,
    _Out_ COLORREF *pColor
    );
typedef HRESULT (WINAPI *_SetWindowTheme)(
    _In_ HWND hwnd,
    _In_ LPCWSTR pszSubAppName,
    _In_ LPCWSTR pszSubIdList
    );

typedef HRESULT (WINAPI *_GetThemeFont)(
    _In_ HTHEME hTheme,
    _In_ HDC hdc,
    _In_ INT iPartId,
    _In_ INT iStateId,
    _In_ INT iPropId,
    _Out_ LOGFONTW *pFont
    );

typedef HRESULT (WINAPI *_GetThemeSysFont)(
    _In_ HTHEME hTheme,
    _In_ INT iFontId,
    _Out_ LOGFONTW *plf
    );

typedef BOOL (WINAPI *_IsThemeBackgroundPartiallyTransparent)(
    _In_ HTHEME hTheme,
    _In_ INT iPartId,
    _In_ INT iStateId
    );

typedef HRESULT (WINAPI *_DrawThemeParentBackground)(
    _In_ HWND hwnd,
    _In_ HDC hdc,
    _In_opt_ const RECT* prc
    );

typedef HRESULT (WINAPI *_GetThemeBackgroundContentRect)(
    _In_ HTHEME hTheme,
    _In_ HDC hdc,
    _In_ INT iPartId,
    _In_ INT iStateId,
    _Inout_ LPCRECT pBoundingRect,
    _Out_ LPRECT pContentRect
    );

static _IsThemeActive IsThemeActive_I;
static _OpenThemeData OpenThemeData_I;
static _SetWindowTheme SetWindowTheme_I;
static _CloseThemeData CloseThemeData_I;
static _IsThemePartDefined IsThemePartDefined_I;
static _DrawThemeBackground DrawThemeBackground_I;
static _DrawThemeParentBackground DrawThemeParentBackground_I;
static _IsThemeBackgroundPartiallyTransparent IsThemeBackgroundPartiallyTransparent_I;
static _GetThemeColor GetThemeColor_I;

static VOID NcAreaInitializeUxTheme(
    _Inout_ NC_CONTEXT* Context,
    _In_ HWND hwndDlg
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
        DrawThemeParentBackground_I = (_DrawThemeParentBackground)GetProcAddress(Context->UxThemeModule, "DrawThemeParentBackground");
        IsThemeBackgroundPartiallyTransparent_I = (_IsThemeBackgroundPartiallyTransparent)GetProcAddress(Context->UxThemeModule, "IsThemeBackgroundPartiallyTransparent");
    }

    if (IsThemeActive_I && OpenThemeData_I && CloseThemeData_I && IsThemePartDefined_I && DrawThemeBackground_I)
    {
        if (Context->UxThemeHandle)
            CloseThemeData_I(Context->UxThemeHandle);

        Context->IsThemeActive = IsThemeActive_I();
        Context->UxThemeHandle = OpenThemeData_I(hwndDlg, VSCLASS_EDIT);

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
#endif _UXTHEME_ENABLED_

static HFONT InitializeFont(
    _In_ HWND hwndDlg
    )
{
    HFONT fontHandle = NULL;
    NONCLIENTMETRICS metrics = { sizeof(NONCLIENTMETRICS) };

    SystemParametersInfo(SPI_GETNONCLIENTMETRICS, sizeof(NONCLIENTMETRICS), &metrics, 0);

    metrics.lfMenuFont.lfHeight = 14;
    metrics.lfMenuFont.lfWeight = FW_NORMAL;
    //metrics.lfMenuFont.lfQuality = CLEARTYPE_QUALITY | ANTIALIASED_QUALITY;

    fontHandle = CreateFontIndirect(&metrics.lfMenuFont);

    if (fontHandle)
    {
        SendMessage(hwndDlg, WM_SETFONT, (WPARAM)fontHandle, FALSE);
        return fontHandle;
    }

    return NULL;
}

static VOID NcAreaGetButtonRect(
    _Inout_ NC_CONTEXT* Context,
    _In_ RECT* rect
    )
{
    // retrieve the coordinates of an inserted button, given the specified window rectangle.
    rect->right -= Context->cxRightEdge;
    rect->top += Context->cyTopEdge; // GetSystemMetrics(SM_CYBORDER);
    rect->bottom -= Context->cyBottomEdge;
    rect->left = rect->right - Context->cxImgSize; // GetSystemMetrics(SM_CXBORDER)

    if (Context->cxRightEdge > Context->CxLeftEdge)
        OffsetRect(rect, Context->cxRightEdge - Context->CxLeftEdge, 0);
}

static LRESULT CALLBACK NcAreaWndSubclassProc(
    _In_ HWND hwndDlg,
    _In_ UINT uMsg,
    _In_ WPARAM wParam,
    _In_ LPARAM lParam,
    _In_ UINT_PTR uIdSubclass,
    _In_ DWORD_PTR dwRefData
    )
{
    NC_CONTEXT* context = (NC_CONTEXT*)GetProp(hwndDlg, L"Context");
    
    if (context == NULL)
        return DefSubclassProc(hwndDlg, uMsg, wParam, lParam);

    if (uMsg == WM_NCDESTROY)
    {
        RemoveWindowSubclass(hwndDlg, NcAreaWndSubclassProc, 0);

        if (context->ImageList)
            ImageList_Destroy(context->ImageList);

#ifdef _CUSTOM_THEME_
        if (context->FocusedBorderPen)
            DeleteObject(context->FocusedBorderPen);
 
        if (context->NormalBorderPen)
            DeleteObject(context->NormalBorderPen);       
#endif

#ifdef _UXTHEME_ENABLED_
        if (CloseThemeData_I && context->UxThemeHandle)
            CloseThemeData_I(context->UxThemeHandle);

        if (context->UxThemeModule)
            FreeLibrary(context->UxThemeModule);
#endif

        RemoveProp(hwndDlg, L"Context");
        PhFree(context);
    }

    switch (uMsg)
    {
#ifdef _UXTHEME_ENABLED_
    case WM_SYSCOLORCHANGE:
    case WM_STYLECHANGED:
    case WM_THEMECHANGED:
        NcAreaInitializeUxTheme(context, hwndDlg);
        break; 
#endif
    case WM_NCCALCSIZE:
        {
            PRECT newClientRect = (PRECT)lParam;

            newClientRect->right -= context->cxImgSize; // GetSystemMetrics(SM_CXVSCROLL);
        }
        break;
    case WM_NCPAINT:
        {
            HDC hdc = NULL;
            RECT clientRect = { 0 };
            RECT windowRect = { 0 };

            BOOL isFocused = (GetFocus() == hwndDlg);

            if (!(hdc = GetWindowDC(hwndDlg)))
                return FALSE;

            SetBkMode(hdc, TRANSPARENT);

            // Get the screen coordinates of the client window.
            GetClientRect(hwndDlg, &clientRect);      
            // Get the screen coordinates of the window.
            GetWindowRect(hwndDlg, &windowRect);
            // Adjust the coordinates (start from border edge).
            OffsetRect(&clientRect, context->CXBorder, context->CYBorder);
            // Adjust the coordinates (start from 0,0).
            OffsetRect(&windowRect, -windowRect.left, -windowRect.top);

            // Exclude the client area.
            ExcludeClipRect(
                hdc,
                clientRect.left,
                clientRect.top,
                clientRect.right,
                clientRect.bottom
                );      

            // Draw the themed background. 
#ifdef _UXTHEME_ENABLED_
            if (context->IsThemeActive && context->IsThemeBackgroundActive)
            {
                if (isFocused)
                {
                    if (IsThemeBackgroundPartiallyTransparent_I(
                        context->UxThemeHandle,
                        EP_EDITBORDER_NOSCROLL,
                        EPSN_FOCUSED
                        ))
                    {
                        DrawThemeParentBackground_I(hwndDlg, hdc, NULL);
                    }
                    DrawThemeBackground_I(
                        context->UxThemeHandle,
                        hdc,
                        EP_EDITBORDER_NOSCROLL,
                        EPSN_FOCUSED,
                        &windowRect,
                        NULL
                        );
                }
#ifdef _HOTTRACK_ENABLED_
                else if (context->MouseInClient)
                {            
                    if (IsThemeBackgroundPartiallyTransparent_I(
                        context->UxThemeHandle,
                        EP_EDITBORDER_NOSCROLL,
                        EPSN_HOT
                        ))
                    {
                        DrawThemeParentBackground_I(hwndDlg, hdc, NULL);
                    }
                    DrawThemeBackground_I(
                        context->UxThemeHandle,
                        hdc,
                        EP_EDITBORDER_NOSCROLL,
                        EPSN_HOT,
                        &windowRect,
                        NULL
                        );
                }
#endif _HOTTRACK_ENABLED_
                else
                {
                    if (IsThemeBackgroundPartiallyTransparent_I(
                        context->UxThemeHandle,
                        EP_EDITBORDER_NOSCROLL,
                        EPSN_NORMAL
                        ))
                    {
                        DrawThemeParentBackground_I(hwndDlg, hdc, NULL);
                    }
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
#endif _UXTHEME_ENABLED_
            {
#ifndef _CUSTOM_THEME_
                FillRect(hdc, &windowRect, GetSysColorBrush(COLOR_WINDOW));
#else
                SetDCBrushColor(hdc, RGB(0xff, 0xff, 0xff)); // GetSysColor(COLOR_WINDOW)
                FillRect(hdc, &windowRect, GetStockBrush(DC_BRUSH));
#endif _CUSTOM_THEME_
                if (isFocused)
                {
#ifndef _CUSTOM_THEME_
                    FrameRect(hdc, &windowRect, GetSysColorBrush(COLOR_HIGHLIGHT));
#else
                    HGDIOBJ oldPen = SelectObject(hdc, context->FocusedBorderPen);
                    Rectangle(hdc, windowRect.left, windowRect.top, windowRect.right, windowRect.bottom);
                    SelectObject(hdc, oldPen);
#endif _CUSTOM_THEME_
                }
#ifdef _HOTTRACK_ENABLED_
                else if (context->MouseInClient)
                {
#ifndef _CUSTOM_THEME_
                    FrameRect(hdc, &windowRect, GetSysColorBrush(COLOR_HIGHLIGHT));
#else
                    HGDIOBJ oldPen = SelectObject(hdc, context->FocusedBorderPen);
                    Rectangle(hdc, windowRect.left, windowRect.top, windowRect.right, windowRect.bottom);
                    SelectObject(hdc, oldPen);          
#endif
                }
#endif
                else
                {
#ifndef _CUSTOM_THEME_
                    FrameRect(hdc, &windowRect, GetSysColorBrush(COLOR_GRAYTEXT));
#else
                    HGDIOBJ oldPen = SelectObject(hdc, context->NormalBorderPen);
                    Rectangle(hdc, windowRect.left, windowRect.top, windowRect.right, windowRect.bottom);
                    SelectObject(hdc, oldPen); 
#endif
                }
            }

            // Get the position of the inserted button.
            NcAreaGetButtonRect(context, &windowRect);

            // Draw the button.
            if (GetWindowTextLength(hwndDlg) > 0)
            {
                ImageList_DrawEx(
                    context->ImageList,
                    0,
                    hdc,
                    windowRect.left,
                    windowRect.top,
                    0,
                    0,
                    RGB(0xff, 0xff, 0xff),
                    RGB(0xff, 0xff, 0xff),
                    ILD_NORMAL | ILD_TRANSPARENT
                    );
            }
            else
            {
                ImageList_DrawEx(
                    context->ImageList,
                    1,
                    hdc,
                    windowRect.left,
                    windowRect.top + 1,
                    0,
                    0,
                    RGB(0xff, 0xff, 0xff),
                    RGB(0xff, 0xff, 0xff),
                    ILD_NORMAL | ILD_TRANSPARENT
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
        return FALSE;
    case WM_NCHITTEST:
        {
            POINT windowPoint = { 0 };
            RECT windowRect = { 0 };

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
            POINT windowPoint = { 0 };
            RECT windowRect = { 0 };

            // Get the screen coordinates of the mouse.
            windowPoint.x = GET_X_LPARAM(lParam);
            windowPoint.y = GET_Y_LPARAM(lParam);

            // Get the position of the inserted button.
            GetWindowRect(hwndDlg, &windowRect);
            NcAreaGetButtonRect(context, &windowRect);

            // Check that the mouse is within the button rect.
            if (PtInRect(&windowRect, windowPoint))
            {
                SetCapture(hwndDlg);

                // Send click notification
                SendMessage(PhMainWndHandle, WM_COMMAND, MAKEWPARAM(context->CommandID, BN_CLICKED), 0);

                RedrawWindow(hwndDlg, NULL, NULL, RDW_FRAME | RDW_INVALIDATE);
                return FALSE;
            }
        }
        break;
    case WM_LBUTTONUP:
        {
            if (GetCapture() == hwndDlg)
            {
                ReleaseCapture();

                RedrawWindow(hwndDlg, NULL, NULL, RDW_FRAME | RDW_INVALIDATE);
                return FALSE;
            }
        }
        break;
    case WM_KEYDOWN:
        {
            if (WindowsVersion < WINDOWS_VISTA)
            {
                // Handle CTRL+A below Vista.
                if (GetKeyState(VK_CONTROL) & VK_LCONTROL && wParam == 'A')
                {
                    Edit_SetSel(hwndDlg, 0, -1);
                    return FALSE;
                }
            }
        }
        break;
    case WM_CUT:
    case WM_CLEAR:
    case WM_PASTE:
    case WM_UNDO:
    case WM_KEYUP:
    case WM_SETTEXT:
    case WM_SETFOCUS:
    case WM_KILLFOCUS:
        RedrawWindow(hwndDlg, NULL, NULL, RDW_FRAME | RDW_INVALIDATE);
        break;
#ifdef _HOTTRACK_ENABLED_
    case WM_MOUSELEAVE:
        {
            context->MouseInClient = FALSE;

            RedrawWindow(hwndDlg, NULL, NULL, RDW_FRAME | RDW_INVALIDATE);
        }
        break;
    case WM_MOUSEMOVE:
        {
            POINT windowPoint = { 0 };
            RECT windowRect = { 0 };

            // Get the screen coordinates of the mouse.
            windowPoint.x = GET_X_LPARAM(lParam);
            windowPoint.y = GET_Y_LPARAM(lParam);

            GetWindowRect(hwndDlg, &windowRect);
            OffsetRect(&windowRect, -windowRect.left, -windowRect.top);

            if (PtInRect(&windowRect, windowPoint))
            {
                TRACKMOUSEEVENT tme = { sizeof(TRACKMOUSEEVENT) };
                tme.dwFlags = TME_LEAVE;// | TME_NONCLIENT;
                tme.hwndTrack = hwndDlg;

                context->MouseInClient = TRUE;
                RedrawWindow(hwndDlg, NULL, NULL, RDW_FRAME | RDW_INVALIDATE);

                TrackMouseEvent(&tme);
            }
            else
            {
                //context->MouseInClient = FALSE;
                //RedrawWindow(hwndDlg, NULL, NULL, RDW_FRAME | RDW_INVALIDATE);
            }
        }
        break;
#endif _HOTTRACK_ENABLED_
    }

    return DefSubclassProc(hwndDlg, uMsg, wParam, lParam);
}



HBITMAP LoadImageFromResources(
    _In_ UINT Width,
    _In_ UINT Height,
    _In_ PCWSTR Name
    )
{
    UINT width = 0;
    UINT height = 0;
    UINT frameCount = 0;
    BOOLEAN isSuccess = FALSE;
    ULONG resourceLength = 0;
    HGLOBAL resourceHandle = NULL;
    HRSRC resourceHandleSource = NULL;
    WICInProcPointer resourceBuffer = NULL;

    BITMAPINFO bitmapInfo = { 0 };
    HBITMAP bitmapHandle = NULL;
    PBYTE bitmapBuffer = NULL;

    IWICStream* wicStream = NULL;
    IWICBitmapSource* wicBitmapSource = NULL;
    IWICBitmapDecoder* wicDecoder = NULL;
    IWICBitmapFrameDecode* wicFrame = NULL;
    
    IWICBitmapScaler* wicScaler = NULL;
    WICPixelFormatGUID pixelFormat;

    WICRect rect = { 0, 0, Width, Height };

    static PH_INITONCE initOnce = PH_INITONCE_INIT;
    static IWICImagingFactory* wicFactory = NULL;

    __try
    {
        if (PhBeginInitOnce(&initOnce))
        {
            // Create the ImagingFactory
            if (FAILED(CoCreateInstance(&CLSID_WICImagingFactory1, NULL, CLSCTX_INPROC_SERVER, &IID_IWICImagingFactory, (PVOID*)&wicFactory)))
            {
                PhEndInitOnce(&initOnce);
                __leave;
            }

            PhEndInitOnce(&initOnce);
        }

        // Find the resource
        if ((resourceHandleSource = FindResource(PluginInstance->DllBase, Name, L"PNG")) == NULL)
            __leave;

        // Get the resource length
        resourceLength = SizeofResource(PluginInstance->DllBase, resourceHandleSource);

        // Load the resource
        if ((resourceHandle = LoadResource(PluginInstance->DllBase, resourceHandleSource)) == NULL)
            __leave;

        if ((resourceBuffer = (WICInProcPointer)LockResource(resourceHandle)) == NULL)
            __leave;

        // Create the Stream
        if (FAILED(IWICImagingFactory_CreateStream(wicFactory, &wicStream)))
            __leave;

        // Initialize the Stream from Memory
        if (FAILED(IWICStream_InitializeFromMemory(wicStream, resourceBuffer, resourceLength)))
            __leave;

        if (FAILED(IWICImagingFactory_CreateDecoder(wicFactory, &GUID_ContainerFormatPng, NULL, &wicDecoder)))
            __leave;

        if (FAILED(IWICBitmapDecoder_Initialize(wicDecoder, (IStream*)wicStream, WICDecodeMetadataCacheOnLoad)))
            __leave;

        // Get the Frame count
        if (FAILED(IWICBitmapDecoder_GetFrameCount(wicDecoder, &frameCount)) || frameCount < 1)
            __leave;

        // Get the Frame
        if (FAILED(IWICBitmapDecoder_GetFrame(wicDecoder, 0, &wicFrame)))
            __leave;

        // Get the WicFrame image format
        if (FAILED(IWICBitmapFrameDecode_GetPixelFormat(wicFrame, &pixelFormat)))
            __leave;

        // Check if the image format is supported:
        if (IsEqualGUID(&pixelFormat, &GUID_WICPixelFormat32bppPBGRA)) // GUID_WICPixelFormat32bppRGB
        {
            wicBitmapSource = (IWICBitmapSource*)wicFrame;
        }
        else
        {
            // Convert the image to the correct format:
            if (FAILED(WICConvertBitmapSource(&GUID_WICPixelFormat32bppPBGRA, (IWICBitmapSource*)wicFrame, &wicBitmapSource)))
                __leave;

            IWICBitmapFrameDecode_Release(wicFrame);
        }

        bitmapInfo.bmiHeader.biSize = sizeof(BITMAPINFOHEADER);
        bitmapInfo.bmiHeader.biWidth = rect.Width;
        bitmapInfo.bmiHeader.biHeight = -((LONG)rect.Height);
        bitmapInfo.bmiHeader.biPlanes = 1;
        bitmapInfo.bmiHeader.biBitCount = 32;
        bitmapInfo.bmiHeader.biCompression = BI_RGB;

        HDC hdc = CreateCompatibleDC(NULL);
        bitmapHandle = CreateDIBSection(hdc, &bitmapInfo, DIB_RGB_COLORS, (PVOID*)&bitmapBuffer, NULL, 0);
        ReleaseDC(NULL, hdc);

        // Check if it's the same rect as the requested size.
        //if (width != rect.Width || height != rect.Height)
        if (FAILED(IWICImagingFactory_CreateBitmapScaler(wicFactory, &wicScaler)))
            __leave;
        if (FAILED(IWICBitmapScaler_Initialize(wicScaler, wicBitmapSource, rect.Width, rect.Height, WICBitmapInterpolationModeFant)))
            __leave;
        if (FAILED(IWICBitmapScaler_CopyPixels(wicScaler, &rect, rect.Width * 4, rect.Width * rect.Height * 4, bitmapBuffer)))
            __leave;

        isSuccess = TRUE;
    }
    __finally
    {
        if (wicScaler)
        {
            IWICBitmapScaler_Release(wicScaler);
        }

        if (wicBitmapSource)
        {
            IWICBitmapSource_Release(wicBitmapSource);
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
           // IWICImagingFactory_Release(wicFactory);
        }

        if (resourceHandle)
        {
            FreeResource(resourceHandle);
        }
    }

    return bitmapHandle;
}

BOOLEAN InsertButton(
    _In_ HWND hwndDlg,
    _In_ UINT CommandID
    )
{
    NC_CONTEXT* context = (NC_CONTEXT*)PhAllocate(sizeof(NC_CONTEXT));
    memset(context, 0, sizeof(NC_CONTEXT));

    context->cxImgSize = 22;
    context->CommandID = CommandID;
    context->CXBorder = GetSystemMetrics(SM_CXBORDER) * 2;
    context->CYBorder = GetSystemMetrics(SM_CYBORDER) * 2;

    context->ImageList = ImageList_Create(23, 20, ILC_COLOR32 | ILC_MASK, 2, 0);
    ImageList_SetImageCount(context->ImageList, 2);

    // Add the images to the imagelist
    if (context->ActiveBitmap = LoadImageFromResources(23, 20, MAKEINTRESOURCE(IDB_SEARCH_ACTIVE)))
    {
        ImageList_Replace(context->ImageList, 0, context->ActiveBitmap, NULL);
        DeleteObject(context->ActiveBitmap);
    }
    else
    {
        PhSetImageListBitmap(context->ImageList, 0, (HINSTANCE)PluginInstance->DllBase, MAKEINTRESOURCE(IDB_SEARCH_ACTIVE_BMP));
    }

    if (context->InactiveBitmap = LoadImageFromResources(23, 20, MAKEINTRESOURCE(IDB_SEARCH_INACTIVE)))
    {
        ImageList_Replace(context->ImageList, 1, context->InactiveBitmap, NULL);
        DeleteObject(context->InactiveBitmap);
    }
    else
    {
        PhSetImageListBitmap(context->ImageList, 1, (HINSTANCE)PluginInstance->DllBase, MAKEINTRESOURCE(IDB_SEARCH_INACTIVE_BMP));
    }

#ifdef _CUSTOM_THEME_
    ImageList_SetBkColor(context->ImageList, GetSysColor(COLOR_WINDOW));
    context->FocusedBorderPen = CreatePen(PS_SOLID, GetSystemMetrics(SM_CXBORDER), RGB(50, 150, 0xff));
    context->NormalBorderPen = CreatePen(PS_SOLID, GetSystemMetrics(SM_CXBORDER), RGB(160, 160, 160));
#else
    ImageList_SetBkColor(context->ImageList, RGB(0xff, 0xff, 0xff));   
#endif _CUSTOM_THEME_

    // Set Searchbox control font
    InitializeFont(hwndDlg);

    // Set our window context data.
    SetProp(hwndDlg, L"Context", (HANDLE)context);

    // Subclass the Edit control window procedure.
    SetWindowSubclass(hwndDlg, NcAreaWndSubclassProc, 0, (DWORD_PTR)context);

#ifdef _UXTHEME_ENABLED_
    SendMessage(hwndDlg, WM_THEMECHANGED, 0, 0);
#endif _UXTHEME_ENABLED_

    // Force the edit control to update its non-client area.
    RedrawWindow(hwndDlg, NULL, NULL, RDW_FRAME | RDW_INVALIDATE);

    return TRUE;
}