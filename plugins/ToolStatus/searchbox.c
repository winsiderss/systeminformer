/*
 * Process Hacker -
 *   Subclassed Edit control
 *
 * Copyright (C) 2012-2015 dmex
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
 */

#include "toolstatus.h"

static VOID NcAreaFreeTheme(
    _Inout_ PEDIT_CONTEXT Context
    )
{
    if (Context->BrushNormal)
        DeleteObject(Context->BrushNormal);

    if (Context->BrushHot)
        DeleteObject(Context->BrushHot);

    if (Context->BrushFocused)
        DeleteObject(Context->BrushFocused);

    if (Context->BrushFill)
        DeleteObject(Context->BrushFill);

    if (Context->WindowFont)
        DeleteObject(Context->WindowFont);
}

static VOID NcAreaInitializeFont(
    _Inout_ PEDIT_CONTEXT Context
    )
{
    NONCLIENTMETRICS metrics = { sizeof(NONCLIENTMETRICS) };

    // Cleanup existing Font handle.
    if (Context->WindowFont)
        DeleteObject(Context->WindowFont);

    if (SystemParametersInfo(SPI_GETNONCLIENTMETRICS, 0, &metrics, 0))
    {
        metrics.lfMessageFont.lfHeight = -11;

        Context->WindowFont = CreateFontIndirect(&metrics.lfMessageFont);
    }
    else
    {
        LOGFONT font;

        GetObject((HFONT)GetStockObject(DEFAULT_GUI_FONT), sizeof(LOGFONT), &font);

        font.lfHeight = -11;

        Context->WindowFont = CreateFontIndirect(&font);
    }

    SendMessage(Context->WindowHandle, WM_SETFONT, (WPARAM)Context->WindowFont, TRUE);
}

static VOID NcAreaInitializeTheme(
    _Inout_ PEDIT_CONTEXT Context
    )
{
    Context->CXBorder = GetSystemMetrics(SM_CXBORDER) * 2;
    //Context->CYBorder = GetSystemMetrics(SM_CYBORDER) * 2;

    Context->BackgroundColorRef = GetSysColor(COLOR_WINDOW);
    Context->BrushFill = GetSysColorBrush(COLOR_WINDOW);
    Context->BrushNormal = GetStockBrush(BLACK_BRUSH);
    Context->BrushHot = WindowsVersion < WINDOWS_VISTA ? CreateSolidBrush(RGB(50, 150, 255)) : GetSysColorBrush(COLOR_HIGHLIGHT);
    Context->BrushFocused = WindowsVersion < WINDOWS_VISTA ? CreateSolidBrush(RGB(50, 150, 255)) : GetSysColorBrush(COLOR_HIGHLIGHT);
}

static VOID NcAreaInitializeImageList(
    _Inout_ PEDIT_CONTEXT Context
    )
{
    HBITMAP bitmapActive = NULL;
    HBITMAP bitmapInactive = NULL;

    Context->ImageWidth = 23;
    Context->ImageHeight = 20;
    Context->ImageList = ImageList_Create(32, 32, ILC_COLOR32 | ILC_MASK, 0, 0);

    ImageList_SetBkColor(Context->ImageList, Context->BackgroundColorRef);
    ImageList_SetImageCount(Context->ImageList, 2);

    // Add the images to the imagelist
    if (bitmapActive = LoadImageFromResources(Context->ImageWidth, Context->ImageHeight, MAKEINTRESOURCE(IDB_SEARCH_ACTIVE)))
    {
        ImageList_Replace(Context->ImageList, 0, bitmapActive, NULL);
        DeleteObject(bitmapActive);
    }
    else
    {
        PhSetImageListBitmap(Context->ImageList, 0, PluginInstance->DllBase, MAKEINTRESOURCE(IDB_SEARCH_ACTIVE_BMP));
    }

    if (bitmapInactive = LoadImageFromResources(Context->ImageWidth, Context->ImageHeight, MAKEINTRESOURCE(IDB_SEARCH_INACTIVE)))
    {
        ImageList_Replace(Context->ImageList, 1, bitmapInactive, NULL);
        DeleteObject(bitmapInactive);
    }
    else
    {
        PhSetImageListBitmap(Context->ImageList, 1, PluginInstance->DllBase, MAKEINTRESOURCE(IDB_SEARCH_INACTIVE_BMP));
    }
}

static VOID NcAreaGetButtonRect(
    _Inout_ PEDIT_CONTEXT Context,
    _Inout_ PRECT ButtonRect
    )
{
    ButtonRect->left = (ButtonRect->right - Context->cxImgSize) - Context->CXBorder;
    ButtonRect->bottom -= 2;
    ButtonRect->right -= 2;
    ButtonRect->top += 2;
}

static VOID NcAreaDrawButton(
    _Inout_ PEDIT_CONTEXT Context,
    _In_ RECT ButtonRect
    )
{
    HDC hdc;

    if (!(hdc = GetWindowDC(Context->WindowHandle)))
        return;

    SetBkMode(hdc, TRANSPARENT);

    FillRect(hdc, &ButtonRect, Context->BrushFill);

//#ifdef _DEBUG
//    BOOL isFocused = (GetFocus() == Context->WindowHandle);
//    if (isFocused)
//    {
//        FrameRect(hdc, &ButtonRect, Context->BrushFocused);
//    }
//    else if (Context->MouseInClient)
//    {
//        FrameRect(hdc, &ButtonRect, Context->BrushHot);
//    }
//    else
//    {
//        FrameRect(hdc, &ButtonRect, Context->BrushNormal);
//    }
//#endif

    // Draw the image centered within the rect.
    if (SearchboxText->Length > 0)
    {
        ImageList_DrawEx(
            Context->ImageList,
            0,
            hdc,
            ButtonRect.left,
            ButtonRect.top + ((ButtonRect.bottom - ButtonRect.top) - Context->ImageHeight) / 2,
            0,
            0,
            Context->BackgroundColorRef,
            Context->BackgroundColorRef,
            ILD_NORMAL | ILD_TRANSPARENT
            );
    }
    else
    {
        ImageList_DrawEx(
            Context->ImageList,
            1,
            hdc,
            ButtonRect.left,
            ButtonRect.top + ((ButtonRect.bottom - ButtonRect.top) - (Context->ImageHeight - 1)) / 2, // Fix image offset by 1
            0,
            0,
            Context->BackgroundColorRef,
            Context->BackgroundColorRef,
            ILD_NORMAL | ILD_TRANSPARENT
            );
    }

    ReleaseDC(Context->WindowHandle, hdc);
}

static LRESULT CALLBACK NcAreaWndSubclassProc(
    _In_ HWND hwndDlg,
    _In_ UINT uMsg,
    _In_ WPARAM wParam,
    _In_ LPARAM lParam,
    _In_ UINT_PTR uIdSubclass,
    _In_ ULONG_PTR dwRefData
    )
{
    PEDIT_CONTEXT context;

    context = (PEDIT_CONTEXT)GetProp(hwndDlg, L"EditSubclassContext");

    switch (uMsg)
    {
    case WM_NCDESTROY:
        {
            NcAreaFreeTheme(context);

            if (context->ImageList)
                ImageList_Destroy(context->ImageList);

            RemoveWindowSubclass(hwndDlg, NcAreaWndSubclassProc, uIdSubclass);
            RemoveProp(hwndDlg, L"EditSubclassContext");
            PhFree(context);
        }
        break;
    case WM_ERASEBKGND:
        return 1;
    case WM_NCCALCSIZE:
        {
            LPNCCALCSIZE_PARAMS ncCalcSize = (NCCALCSIZE_PARAMS*)lParam;

            // Let Windows handle the non-client defaults.
            DefSubclassProc(hwndDlg, uMsg, wParam, lParam);

            // Deflate the client area to accommodate the custom button.
            ncCalcSize->rgrc[0].right -= context->cxImgSize;
        }
        return 0;
    case WM_NCPAINT:
        {
            RECT windowRect;

            // Let Windows handle the non-client defaults.
            DefSubclassProc(hwndDlg, uMsg, wParam, lParam);

            // Get the screen coordinates of the window.
            GetWindowRect(hwndDlg, &windowRect);

            // Adjust the coordinates (start from 0,0).
            OffsetRect(&windowRect, -windowRect.left, -windowRect.top);

            // Get the position of the inserted button.
            NcAreaGetButtonRect(context, &windowRect);

            // Draw the button.
            NcAreaDrawButton(context, windowRect);
        }
        return 0;
    case WM_NCHITTEST:
        {
            POINT windowPoint;
            RECT windowRect;

            // Get the screen coordinates of the mouse.
            windowPoint.x = GET_X_LPARAM(lParam);
            windowPoint.y = GET_Y_LPARAM(lParam);

            // Get the position of the inserted button.
            GetWindowRect(hwndDlg, &windowRect);
            NcAreaGetButtonRect(context, &windowRect);

            // Check that the mouse is within the inserted button.
            if (PtInRect(&windowRect, windowPoint))
            {
                return HTBORDER;
            }
        }
        break;
    case WM_NCLBUTTONDOWN:
        {
            POINT windowPoint;
            RECT windowRect;

            // Get the screen coordinates of the mouse.
            windowPoint.x = GET_X_LPARAM(lParam);
            windowPoint.y = GET_Y_LPARAM(lParam);

            // Get the position of the inserted button.
            GetWindowRect(hwndDlg, &windowRect);
            NcAreaGetButtonRect(context, &windowRect);

            // Check that the mouse is within the inserted button.
            if (PtInRect(&windowRect, windowPoint))
            {
                SetCapture(hwndDlg);

                RedrawWindow(hwndDlg, NULL, NULL, RDW_FRAME | RDW_INVALIDATE);
            }
        }
        break;
    case WM_LBUTTONUP:
        {
            POINT windowPoint;
            RECT windowRect;

            // Get the screen coordinates of the mouse.
            windowPoint.x = GET_X_LPARAM(lParam);
            windowPoint.y = GET_Y_LPARAM(lParam);

            // Get the screen coordinates of the window.
            GetWindowRect(hwndDlg, &windowRect);

            // Adjust the coordinates (start from 0,0).
            OffsetRect(&windowRect, -windowRect.left, -windowRect.top);

            // Get the position of the inserted button.
            NcAreaGetButtonRect(context, &windowRect);

            // Check that the mouse is within the inserted button.
            if (PtInRect(&windowRect, windowPoint))
            {
                // Forward click notification.
                SendMessage(PhMainWndHandle, WM_COMMAND, MAKEWPARAM(context->CommandID, BN_CLICKED), 0);
            }

            if (GetCapture() == hwndDlg)
            {
                ReleaseCapture();
            }

            RedrawWindow(hwndDlg, NULL, NULL, RDW_FRAME | RDW_INVALIDATE);
        }
        break;
    case WM_KEYDOWN:
        {
            if (wParam == '\t' || wParam == '\r')
            {
                HWND tnHandle;

                tnHandle = GetCurrentTreeNewHandle();

                if (tnHandle)
                {
                    SetFocus(tnHandle);

                    if (wParam == '\r')
                    {
                        if (TreeNew_GetFlatNodeCount(tnHandle) > 0)
                        {
                            TreeNew_DeselectRange(tnHandle, 0, -1);
                            TreeNew_SelectRange(tnHandle, 0, 0);
                            TreeNew_SetFocusNode(tnHandle, TreeNew_GetFlatNode(tnHandle, 0));
                            TreeNew_SetMarkNode(tnHandle, TreeNew_GetFlatNode(tnHandle, 0));
                        }
                    }
                }
                else
                {
                    PTOOLSTATUS_TAB_INFO tabInfo;

                    if ((tabInfo = FindTabInfo(SelectedTabIndex)) && tabInfo->ActivateContent)
                        tabInfo->ActivateContent(wParam == '\r');
                }

                return FALSE;
            }

            // Handle CTRL+A below Vista.
            if (WindowsVersion < WINDOWS_VISTA && (GetKeyState(VK_CONTROL) & VK_LCONTROL) && wParam == 'A')
            {
                Edit_SetSel(hwndDlg, 0, -1);
                return FALSE;
            }
        }
        break;
    case WM_CHAR:
        if (wParam == '\t' || wParam == '\r')
            return FALSE;
        break;
    case WM_CUT:
    case WM_CLEAR:
    case WM_PASTE:
    case WM_UNDO:
    case WM_KEYUP:
    case WM_SETTEXT:
    //case WM_SETFOCUS:
    case WM_KILLFOCUS:
        RedrawWindow(hwndDlg, NULL, NULL, RDW_FRAME | RDW_INVALIDATE);
        break;
    case WM_SETTINGCHANGE:
    case WM_SYSCOLORCHANGE:
    case WM_THEMECHANGED:
        {
            NcAreaFreeTheme(context);
            NcAreaInitializeTheme(context);
            NcAreaInitializeFont(context);

            // Reset the client area margins.
            SendMessage(hwndDlg, EM_SETMARGINS, EC_LEFTMARGIN, MAKELPARAM(0, 0));

            // Force the edit control to update its non-client area.
            RedrawWindow(hwndDlg, NULL, NULL, RDW_FRAME | RDW_INVALIDATE);
            //SetWindowPos(hwndDlg, 0, 0, 0, 0, 0, SWP_FRAMECHANGED | SWP_NOMOVE | SWP_NOSIZE | SWP_NOACTIVATE | SWP_NOZORDER);
        }
        break;
    case WM_SETFOCUS:
        {
            if (!RebarBandExists(BandID_SearchBox))
            {
                RebarBandInsert(BandID_SearchBox, SearchboxHandle, 20, 180);
            }
        }
        break;
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
    IWICImagingFactory* wicFactory = NULL;
    IWICBitmapScaler* wicScaler = NULL;
    WICPixelFormatGUID pixelFormat;

    WICRect rect = { 0, 0, Width, Height };

    __try
    {
        // Create the ImagingFactory
        if (FAILED(CoCreateInstance(&CLSID_WICImagingFactory1, NULL, CLSCTX_INPROC_SERVER, &IID_IWICImagingFactory, &wicFactory)))
            __leave;

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
           IWICImagingFactory_Release(wicFactory);
        }

        if (resourceHandle)
        {
            FreeResource(resourceHandle);
        }
    }

    return bitmapHandle;
}

HWND CreateSearchControl(
    _In_ UINT CommandID
    )
{
    PEDIT_CONTEXT context;

    context = (PEDIT_CONTEXT)PhAllocate(sizeof(EDIT_CONTEXT));
    memset(context, 0, sizeof(EDIT_CONTEXT));

    context->cxImgSize = 22; // GetSystemMetrics(SM_CXVSCROLL);
    context->CommandID = CommandID;

    // Create the SearchBox window.
    context->WindowHandle = CreateWindowEx(
        WS_EX_CLIENTEDGE,
        WC_EDIT,
        NULL,
        WS_CHILD | WS_CLIPSIBLINGS | WS_CLIPCHILDREN | ES_LEFT | ES_AUTOHSCROLL | (SearchBoxDisplayMode != SearchBoxDisplayHideInactive ? WS_VISIBLE : 0),
        CW_USEDEFAULT, CW_USEDEFAULT, CW_USEDEFAULT, CW_USEDEFAULT,
        RebarHandle,
        NULL,
        NULL,
        NULL
        );

    NcAreaInitializeTheme(context);
    NcAreaInitializeImageList(context);

    // Set initial text
    Edit_SetCueBannerText(context->WindowHandle, L"Search Processes (Ctrl+K)");

    // Set our window context data.
    SetProp(context->WindowHandle, L"EditSubclassContext", (HANDLE)context);

    // Subclass the Edit control window procedure.
    SetWindowSubclass(context->WindowHandle, NcAreaWndSubclassProc, 0, (ULONG_PTR)context);

    // Initialize the theme parameters.
    SendMessage(context->WindowHandle, WM_THEMECHANGED, 0, 0);

    return context->WindowHandle;
}