/*
 * Process Hacker -
 *   Subclassed Edit control
 *
 * Copyright (C) 2012-2016 dmex
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
#include "commonutil.h"
#include <uxtheme.h>
#include <vssym32.h>

VOID NcAreaInitializeTheme(
    _Inout_ PEDIT_CONTEXT Context
    )
{
    HBITMAP bitmap;
    LOGFONT logFont;

    Context->CXWidth = PhMultiplyDivide(20, PhGlobalDpi, 96);
    Context->ImageWidth = GetSystemMetrics(SM_CXSMICON) + 4;
    Context->ImageHeight = GetSystemMetrics(SM_CYSMICON) + 4;
    Context->BrushNormal = GetSysColorBrush(COLOR_WINDOW);
    Context->BrushHot = CreateSolidBrush(RGB(205, 232, 255));
    Context->BrushPushed = CreateSolidBrush(RGB(153, 209, 255));

    if (GetObject((HFONT)SendMessage(ToolBarHandle, WM_GETFONT, 0, 0), sizeof(LOGFONT), &logFont))
    {
        logFont.lfHeight = -11;

        Context->WindowFont = CreateFontIndirect(&logFont);
        SendMessage(SearchboxHandle, WM_SETFONT, (WPARAM)Context->WindowFont, TRUE);
    }

    if (bitmap = LoadImageFromResources(Context->ImageWidth, Context->ImageHeight, MAKEINTRESOURCE(IDB_SEARCH_ACTIVE), TRUE))
    {
        Context->BitmapActive = CommonBitmapToIcon(bitmap, Context->ImageWidth, Context->ImageHeight);
        DeleteObject(bitmap);
    }
    else if (bitmap = LoadImage(PluginInstance->DllBase, MAKEINTRESOURCE(IDB_SEARCH_ACTIVE_BMP), IMAGE_BITMAP, 0, 0, 0))
    {
        Context->BitmapActive = CommonBitmapToIcon(bitmap, Context->ImageWidth, Context->ImageHeight);
        DeleteObject(bitmap);
    }

    if (bitmap = LoadImageFromResources(Context->ImageWidth, Context->ImageHeight, MAKEINTRESOURCE(IDB_SEARCH_INACTIVE), TRUE))
    {
        Context->BitmapInactive = CommonBitmapToIcon(bitmap, Context->ImageWidth, Context->ImageHeight);
        DeleteObject(bitmap);
    }
    else if (bitmap = LoadImage(PluginInstance->DllBase, MAKEINTRESOURCE(IDB_SEARCH_INACTIVE_BMP), IMAGE_BITMAP, 0, 0, 0))
    {
        Context->BitmapInactive = CommonBitmapToIcon(bitmap, Context->ImageWidth, Context->ImageHeight);
        DeleteObject(bitmap);
    }

    if (IsThemeActive())
    {
        HTHEME themeDataHandle;

        if (themeDataHandle = OpenThemeData(SearchboxHandle, VSCLASS_COMBOBOX))
        {
            if (!SUCCEEDED(GetThemeInt(
                themeDataHandle,
                CP_DROPDOWNBUTTON,
                CBXS_NORMAL,
                TMT_BORDERSIZE,
                &Context->CXBorder
                )))
            {
                Context->CXBorder = GetSystemMetrics(SM_CXBORDER) * 2;
            }

            CloseThemeData(themeDataHandle);
        }
        else
        {
            Context->CXBorder = GetSystemMetrics(SM_CXBORDER) * 2;
        }
    }
    else
    {
        Context->CXBorder = GetSystemMetrics(SM_CXBORDER) * 2;
    }
}

VOID NcAreaFreeTheme(
    _Inout_ PEDIT_CONTEXT Context
    )
{
    if (Context->BrushNormal)
    {
        DeleteObject(Context->BrushNormal);
        Context->BrushNormal = NULL;
    }

    if (Context->BrushHot)
    {
        DeleteObject(Context->BrushHot);
        Context->BrushHot = NULL;
    }

    if (Context->BrushPushed)
    {
        DeleteObject(Context->BrushPushed);
        Context->BrushPushed = NULL;
    }

    if (Context->WindowFont)
    {
        DeleteObject(Context->WindowFont);
        Context->WindowFont = NULL;
    }

    if (Context->BitmapActive)
    {
        DeleteObject(Context->BitmapActive);
        Context->BitmapActive = NULL;
    }

    if (Context->BitmapInactive)
    {
        DeleteObject(Context->BitmapInactive);
        Context->BitmapInactive = NULL;
    }
}

VOID NcAreaGetButtonRect(
    _Inout_ PEDIT_CONTEXT Context,
    _Inout_ PRECT ButtonRect
    )
{
    ButtonRect->left = (ButtonRect->right - Context->CXWidth) - Context->CXBorder + 4;
    ButtonRect->bottom -= Context->CXBorder - 6;
    ButtonRect->right -= Context->CXBorder - 6;
    ButtonRect->top += Context->CXBorder;
}

VOID NcAreaDrawButton(
    _Inout_ PEDIT_CONTEXT Context, 
    _In_ HWND WindowHandle,
    _In_ RECT ButtonRect
    )
{
    HDC hdc;
    HDC bufferDc;
    HBITMAP bufferBitmap;
    HBITMAP oldBufferBitmap;
    RECT bufferRect =
    {
        0, 0,
        ButtonRect.right - ButtonRect.left,
        ButtonRect.bottom - ButtonRect.top
    };

    if (!(hdc = GetWindowDC(GetParent(WindowHandle)))) // Window DC of the parent combobox
        return;

    bufferDc = CreateCompatibleDC(hdc);
    bufferBitmap = CreateCompatibleBitmap(hdc, bufferRect.right, bufferRect.bottom);
    oldBufferBitmap = SelectObject(bufferDc, bufferBitmap);

    if (Context->Pushed)
    {
        FillRect(bufferDc, &bufferRect, Context->BrushPushed);
        //FrameRect(bufferDc, &bufferRect, CreateSolidBrush(RGB(0xff, 0, 0)));
    }
    else if (Context->Hot)
    {
        FillRect(bufferDc, &bufferRect, Context->BrushHot);
        //FrameRect(bufferDc, &bufferRect, CreateSolidBrush(RGB(38, 160, 218)));
    }
    else
    {
        FillRect(bufferDc, &bufferRect, Context->BrushNormal);
        //FrameRect(bufferDc, &bufferRect, GetSysColorBrush(COLOR_WINDOWTEXT));
    }

    if (Edit_GetTextLength(WindowHandle) > 0)
    {
        if (Context->BitmapActive)
        {
            DrawIconEx(
                bufferDc,
                bufferRect.left + ((bufferRect.right - bufferRect.left) - Context->ImageWidth) / 2,
                bufferRect.top + ((bufferRect.bottom - bufferRect.top) - Context->ImageHeight) / 2,
                Context->BitmapActive,
                Context->ImageWidth,
                Context->ImageHeight,
                0,
                NULL,
                DI_NORMAL
                );
        }
    }
    else
    {
        if (Context->BitmapInactive)
        {
            DrawIconEx(
                bufferDc,
                bufferRect.left + ((bufferRect.right - bufferRect.left) - (Context->ImageWidth - 2)) / 2,
                bufferRect.top + ((bufferRect.bottom - bufferRect.top) - (Context->ImageHeight - 2)) / 2, // (ImageHeight - 2) offset image
                Context->BitmapInactive,
                Context->ImageWidth,
                Context->ImageHeight,
                0,
                NULL,
                DI_NORMAL
                );
        }
    }

    BitBlt(hdc, ButtonRect.left, ButtonRect.top, ButtonRect.right, ButtonRect.bottom, bufferDc, 0, 0, SRCCOPY);

    SelectObject(bufferDc, oldBufferBitmap);
    DeleteObject(bufferBitmap);
    DeleteDC(bufferDc);
    ReleaseDC(GetParent(WindowHandle), hdc);
}

LRESULT CALLBACK NcAreaWndSubclassProc(
    _In_ HWND hWnd,
    _In_ UINT uMsg,
    _In_ WPARAM wParam,
    _In_ LPARAM lParam,
    _In_ UINT_PTR uIdSubclass,
    _In_ ULONG_PTR dwRefData
    )
{
    PEDIT_CONTEXT context;

    context = (PEDIT_CONTEXT)GetProp(hWnd, L"EditSubclassContext");

    switch (uMsg)
    {
    case WM_NCDESTROY:
        {
            NcAreaFreeTheme(context);

            RemoveWindowSubclass(hWnd, NcAreaWndSubclassProc, uIdSubclass);
            RemoveProp(hWnd, L"EditSubclassContext");
            PhDereferenceObject(context);
        }
        break;
    case WM_ERASEBKGND:
        return 1;
    case WM_NCCALCSIZE:
        {
            LPNCCALCSIZE_PARAMS ncCalcSize = (NCCALCSIZE_PARAMS*)lParam;

            // Let Windows handle the non-client defaults.
            DefSubclassProc(hWnd, uMsg, wParam, lParam);

            // Deflate the client area to accommodate the custom button.
            ncCalcSize->rgrc[0].right -= context->CXWidth;
        }
        return 0;
    case WM_NCPAINT:
        {
            RECT windowRect;

            // Let Windows handle the non-client defaults.
            DefSubclassProc(hWnd, uMsg, wParam, lParam);

            // Get the screen coordinates of the window.
            GetWindowRect(hWnd, &windowRect);

            // Adjust the coordinates (start from 0,0).
            OffsetRect(&windowRect, -windowRect.left, -windowRect.top);

            // Get the position of the inserted button.
            NcAreaGetButtonRect(context, &windowRect);

            // Draw the button.
            NcAreaDrawButton(context, hWnd, windowRect);
        }
        return 0;
    case WM_NCHITTEST:
        {
            POINT windowPoint;
            RECT windowRect;

            // Get the screen coordinates of the mouse.
            if (!GetCursorPos(&windowPoint))
                break;

            // Get the screen coordinates of the window.
            GetWindowRect(hWnd, &windowRect);

            // Get the position of the inserted button.
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
            if (!GetCursorPos(&windowPoint))
                break;

            // Get the screen coordinates of the window.
            GetWindowRect(hWnd, &windowRect);

            // Get the position of the inserted button.
            NcAreaGetButtonRect(context, &windowRect);

            // Check that the mouse is within the inserted button.
            if (PtInRect(&windowRect, windowPoint))
            {
                context->Pushed = TRUE;

                SetCapture(hWnd);

                RedrawWindow(hWnd, NULL, NULL, RDW_FRAME | RDW_INVALIDATE);
            }
        }
        break;
    case WM_LBUTTONUP:
        {
            POINT windowPoint;
            RECT windowRect;

            // Get the screen coordinates of the mouse.
            if (!GetCursorPos(&windowPoint))
                break;

            // Get the screen coordinates of the window.
            GetWindowRect(hWnd, &windowRect);

            // Get the position of the inserted button.
            NcAreaGetButtonRect(context, &windowRect);

            // Check that the mouse is within the inserted button.
            if (PtInRect(&windowRect, windowPoint))
            {
                // Forward click notification.
                SendMessage(PhMainWndHandle, WM_COMMAND, MAKEWPARAM(context->CommandID, BN_CLICKED), 0);
            }

            if (GetCapture() == hWnd)
            {
                context->Pushed = FALSE;
                ReleaseCapture();
            }

            RedrawWindow(hWnd, NULL, NULL, RDW_FRAME | RDW_INVALIDATE);
        }
        break;
    case WM_KEYDOWN:
        {
            if (wParam == VK_RETURN)
            {
                SendMessage(PhMainWndHandle, WM_COMMAND, MAKEWPARAM(0, EN_CHANGE), (LPARAM)GetParent(hWnd));
                return FALSE;
            }

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
        }
        break;
    case WM_CUT:
    case WM_CLEAR:
    case WM_PASTE:
    case WM_UNDO:
    case WM_KEYUP:
    case WM_SETTEXT:
    case WM_KILLFOCUS:
        RedrawWindow(hWnd, NULL, NULL, RDW_FRAME | RDW_INVALIDATE);
        break;
    case WM_SETTINGCHANGE:
    case WM_SYSCOLORCHANGE:
    case WM_THEMECHANGED:
        {
            NcAreaFreeTheme(context);
            NcAreaInitializeTheme(context);

            // Reset the client area margins.
            SendMessage(hWnd, EM_SETMARGINS, EC_LEFTMARGIN, MAKELPARAM(0, 0));

            // Refresh the non-client area.
            SetWindowPos(hWnd, NULL, 0, 0, 0, 0, SWP_NOACTIVATE | SWP_NOMOVE | SWP_NOSIZE | SWP_FRAMECHANGED);

            // Force the edit control to update its non-client area.
            RedrawWindow(hWnd, NULL, NULL, RDW_FRAME | RDW_INVALIDATE);
        }
        break;
    case WM_SETFOCUS:
        {
            if (SearchBoxDisplayMode != SEARCHBOX_DISPLAY_MODE_HIDEINACTIVE)
                break;

            if (!RebarBandExists(REBAR_BAND_ID_SEARCHBOX))
            {
                UINT height = (UINT)SendMessage(RebarHandle, RB_GETROWHEIGHT, 0, 0);

                RebarBandInsert(
                    REBAR_BAND_ID_SEARCHBOX, 
                    SearchboxHandle,
                    PhMultiplyDivide(180, PhGlobalDpi, 96), 
                    height
                    );
            }
        }
        break;
    case WM_NCMOUSEMOVE:
        {
            POINT windowPoint;
            RECT windowRect;

            // Get the screen coordinates of the mouse.
            if (!GetCursorPos(&windowPoint))
                break;

            // Get the screen coordinates of the window.
            GetWindowRect(hWnd, &windowRect);

            // Get the position of the inserted button.
            NcAreaGetButtonRect(context, &windowRect);

            // Check that the mouse is within the inserted button.
            if (PtInRect(&windowRect, windowPoint) && !context->Hot)
            {
                TRACKMOUSEEVENT trackMouseEvent;

                trackMouseEvent.cbSize = sizeof(TRACKMOUSEEVENT);
                trackMouseEvent.dwFlags = TME_LEAVE | TME_NONCLIENT;
                trackMouseEvent.hwndTrack = hWnd;
                trackMouseEvent.dwHoverTime = 0;

                context->Hot = TRUE;

                TrackMouseEvent(&trackMouseEvent);

                RedrawWindow(hWnd, NULL, NULL, RDW_FRAME | RDW_INVALIDATE);
            }
        }
        break;
    case WM_NCMOUSELEAVE:
        {
            if (context->Hot)
            {
                context->Hot = FALSE;
                RedrawWindow(hWnd, NULL, NULL, RDW_FRAME | RDW_INVALIDATE);
            }
        }
        break;
    case WM_MOUSEMOVE:
        {
            if ((wParam & MK_LBUTTON) && GetCapture() == hWnd)
            {
                POINT windowPoint;
                RECT windowRect;

                // Get the screen coordinates of the mouse.
                if (!GetCursorPos(&windowPoint))
                    break;

                // Get the screen coordinates of the window.
                GetWindowRect(hWnd, &windowRect);

                // Get the position of the inserted button.
                NcAreaGetButtonRect(context, &windowRect);

                // Check that the mouse is within the inserted button.
                context->Pushed = PtInRect(&windowRect, windowPoint);

                RedrawWindow(hWnd, NULL, NULL, RDW_FRAME | RDW_INVALIDATE);
            }
        }
        break;
    case WM_CHAR:
        { 
            if (wParam == '\t' || wParam == '\r')
                return FALSE;

            if (!ToolStatusConfig.AutoComplete)
                break;

            if (GetKeyState(VK_LCONTROL) & 0x8000)
                break;
            if (GetKeyState(VK_BACK) & 0x8000)
                break;
            if (GetKeyState(VK_RETURN) & 0x8000)
                break;

            PostMessage(PhMainWndHandle, WM_COMMAND, MAKEWPARAM(TIDC_SEARCH_STRING, EN_CHANGE), (LPARAM)SearchboxHandle);

            if (!iswcntrl((WCHAR)wParam))
            {
                int index;
                WCHAR buffer[DOS_MAX_PATH_LENGTH];
                PPH_STRING string;

                //if (GetWindowLongPtr(SearchboxHandle, GWL_STYLE) & CBS_DROPDOWN)
                //{  
                //    ComboBox_ShowDropdown(SearchboxHandle, TRUE);
                //}

                // Get the substring from 0 to start of selection
                ComboBox_GetText(SearchboxHandle, buffer, ARRAYSIZE(buffer));
                buffer[LOWORD(ComboBox_GetEditSel(SearchboxHandle))] = 0;

                string = PhFormatString(
                    L"%ls%lc",
                    buffer,
                    (WCHAR)wParam
                    );

                if ((index = ComboBox_FindStringExact(SearchboxHandle, -1, string->Buffer)) == CB_ERR)
                    index = ComboBox_FindString(SearchboxHandle, -1, string->Buffer);

                if (index != CB_ERR)
                {
                    ComboBox_SetCurSel(SearchboxHandle, index);
                    ComboBox_SetEditSel(SearchboxHandle, string->Length / 2, -1);
                }
                else
                {
                    ComboBox_SetText(SearchboxHandle, string->Buffer);
                    ComboBox_SetEditSel(SearchboxHandle, string->Length / 2, -1);
                }

                PhDereferenceObject(string);
                return FALSE;
            }
        }
    }

    return DefSubclassProc(hWnd, uMsg, wParam, lParam);
}

PEDIT_CONTEXT CreateSearchControl(
    _In_ UINT CommandID
    )
{
    PEDIT_CONTEXT context;
    COMBOBOXINFO comboInfo = { sizeof(COMBOBOXINFO) };

    context = (PEDIT_CONTEXT)PhCreateAlloc(sizeof(EDIT_CONTEXT));
    memset(context, 0, sizeof(EDIT_CONTEXT));

    context->CommandID = CommandID;
    
    SearchboxHandle = CreateWindowEx(
        0,
        WC_COMBOBOX,
        NULL,
        WS_VISIBLE | WS_CHILD | WS_CLIPSIBLINGS | WS_CLIPCHILDREN | CBS_DROPDOWN | CBS_HASSTRINGS | CBS_AUTOHSCROLL | CBS_OWNERDRAWFIXED,
        CW_USEDEFAULT, CW_USEDEFAULT, CW_USEDEFAULT, CW_USEDEFAULT,
        RebarHandle,
        NULL,
        NULL,
        NULL
        );

    if (GetComboBoxInfo(SearchboxHandle, &comboInfo))
    {
        // Set initial height
        ListBox_SetItemHeight(comboInfo.hwndList, 0, 18);

        SearchEditHandle = comboInfo.hwndItem;

        // Set initial text
        Edit_SetCueBannerText(SearchEditHandle, L"Search Processes (Ctrl+K)");

        // Set our window context data.
        SetProp(SearchEditHandle, L"EditSubclassContext", (HANDLE)context);

        // Subclass the Edit control window procedure.
        SetWindowSubclass(SearchEditHandle, NcAreaWndSubclassProc, 0, (ULONG_PTR)context);

        // Initialize the theme parameters.
        SendMessage(SearchEditHandle, WM_THEMECHANGED, 0, 0);
    }

    if (SearchBoxDisplayMode == SEARCHBOX_DISPLAY_MODE_HIDEINACTIVE)
        ShowWindow(SearchboxHandle, SW_HIDE);

    return context;
}