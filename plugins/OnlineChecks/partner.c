/*
 * Copyright (c) 2022 Winsider Seminars & Solutions, Inc.  All rights reserved.
 *
 * This file is part of System Informer.
 *
 * Authors:
 *
 *     jxy-s   2026
 *
 */

#include "onlnchk.h"

typedef struct _PARTNER_CONTEXT
{
    HBITMAP BannerBitmap;
    HFONT TitleFont;
} PARTNER_CONTEXT, *PPARTNER_CONTEXT;

static HBITMAP PartnerLoadImage(
    _In_ PCWSTR Name,
    _In_ LONG Width,
    _In_ LONG Height
    )
{
    HBITMAP bitmap;
    DIBSECTION dib;

    bitmap = PhLoadImageFormatFromResource(PluginInstance->DllBase, Name, L"PNG", PH_IMAGE_FORMAT_TYPE_PNG, Width, Height);

    if (bitmap && GetObject(bitmap, sizeof(DIBSECTION), &dib) == sizeof(DIBSECTION) && dib.dsBm.bmBits)
    {
        PBYTE bits = dib.dsBm.bmBits;
        SIZE_T count = (SIZE_T)dib.dsBm.bmWidth * dib.dsBm.bmHeight;

        for (SIZE_T i = 0; i < count; i++)
        {
            BYTE alpha = bits[i * 4 + 3];
            bits[i * 4 + 0] = (BYTE)((bits[i * 4 + 0] * alpha) / 255);
            bits[i * 4 + 1] = (BYTE)((bits[i * 4 + 1] * alpha) / 255);
            bits[i * 4 + 2] = (BYTE)((bits[i * 4 + 2] * alpha) / 255);
        }
    }

    return bitmap;
}

static VOID PartnerDrawImage(
    _In_ LPDRAWITEMSTRUCT DrawInfo,
    _In_opt_ HBITMAP Bitmap
    )
{
    BITMAP bitmap;
    HDC bufferDc;
    HBITMAP oldBitmap;
    BLENDFUNCTION blend = { AC_SRC_OVER, 0, 255, AC_SRC_ALPHA };
    LONG x;
    LONG y;

    if (!Bitmap)
        return;
    if (!GetObject(Bitmap, sizeof(BITMAP), &bitmap))
        return;

    x = DrawInfo->rcItem.left + ((DrawInfo->rcItem.right - DrawInfo->rcItem.left) - bitmap.bmWidth) / 2;
    y = DrawInfo->rcItem.top + ((DrawInfo->rcItem.bottom - DrawInfo->rcItem.top) - bitmap.bmHeight) / 2;

    bufferDc = CreateCompatibleDC(DrawInfo->hDC);
    oldBitmap = SelectObject(bufferDc, Bitmap);
    GdiAlphaBlend(DrawInfo->hDC, x, y, bitmap.bmWidth, bitmap.bmHeight, bufferDc, 0, 0, bitmap.bmWidth, bitmap.bmHeight, blend);
    SelectObject(bufferDc, oldBitmap);
    DeleteDC(bufferDc);
}

static INT_PTR CALLBACK PartnerDialogProc(
    _In_ HWND WindowHandle,
    _In_ UINT WindowMessage,
    _In_ WPARAM wParam,
    _In_ LPARAM lParam
    )
{
    PPARTNER_CONTEXT context;

    if (WindowMessage == WM_INITDIALOG)
    {
        context = PhAllocateZero(sizeof(PARTNER_CONTEXT));
        PhSetWindowContext(WindowHandle, MAXCHAR, context);
    }
    else
    {
        context = PhGetWindowContext(WindowHandle, MAXCHAR);
    }

    if (!context)
        return FALSE;

    switch (WindowMessage)
    {
    case WM_INITDIALOG:
        {
            RECT rect;
            LONG width;
            LONG height;

            PhCenterWindow(WindowHandle, GetParent(WindowHandle));

            GetClientRect(GetDlgItem(WindowHandle, IDC_PARTNER_BANNER), &rect);
            width = rect.right;
            height = rect.bottom;

            if (width > height * 3)
            {
                width = height * 3;
            }
            else
            {
                height = width / 3;
            }

            context->BannerBitmap = PartnerLoadImage(MAKEINTRESOURCE(IDB_PARTNERBANNER), width, height);

            context->TitleFont = PhCreateCommonFont(12, FW_SEMIBOLD, GetDlgItem(WindowHandle, IDC_PARTNER_TITLE), SystemInformer_GetWindowDpi());
            if (context->TitleFont)
                SetWindowFont(GetDlgItem(WindowHandle, IDC_PARTNER_TITLE), context->TitleFont, TRUE);

            PhInitializeWindowTheme(WindowHandle, !!PhGetIntegerSetting(L"EnableThemeSupport"));

            PhSetDialogItemText(WindowHandle, IDC_PARTNER_LINK, L"<a href=\"https://www.hybrid-analysis.com/\">hybrid-analysis.com</a>");
            PhSetDialogFocus(WindowHandle, GetDlgItem(WindowHandle, IDOK));
        }
        return FALSE;
    case WM_DRAWITEM:
        {
            LPDRAWITEMSTRUCT drawInfo = (LPDRAWITEMSTRUCT)lParam;

            if (drawInfo->CtlID == IDC_PARTNER_BANNER)
            {
                PartnerDrawImage(drawInfo, context->BannerBitmap);
                return TRUE;
            }
        }
        break;
    case WM_NOTIFY:
        {
            LPNMHDR header = (LPNMHDR)lParam;

            switch (header->code)
            {
            case NM_CLICK:
                {
                    switch (header->idFrom)
                    {
                    case IDC_PARTNER_LINK:
                        PhShellExecute(WindowHandle, ((PNMLINK)header)->item.szUrl, NULL);
                        break;
                    }
                }
                break;
            }
        }
        break;
    case WM_COMMAND:
        {
            switch (GET_WM_COMMAND_ID(wParam, lParam))
            {
            case IDOK:
                EndDialog(WindowHandle, IDOK);
                break;
            case IDC_PARTNER_CONFIGURE:
                EndDialog(WindowHandle, IDC_PARTNER_CONFIGURE);
                break;
            case IDCANCEL:
                EndDialog(WindowHandle, IDCANCEL);
                break;
            }
        }
        break;
    case WM_DESTROY:
        {
            if (context->BannerBitmap)
                DeleteObject(context->BannerBitmap);
            if (context->TitleFont)
                DeleteObject(context->TitleFont);

            PhRemoveWindowContext(WindowHandle, MAXCHAR);
            PhFree(context);
        }
        break;
    case WM_CTLCOLORBTN:
        return HANDLE_WM_CTLCOLORBTN(WindowHandle, wParam, lParam, PhWindowThemeControlColor);
    case WM_CTLCOLORDLG:
        return HANDLE_WM_CTLCOLORDLG(WindowHandle, wParam, lParam, PhWindowThemeControlColor);
    case WM_CTLCOLORSTATIC:
        return HANDLE_WM_CTLCOLORSTATIC(WindowHandle, wParam, lParam, PhWindowThemeControlColor);
    }

    return FALSE;
}

static VOID OnlineChecksShowPartnerOptions(
    _In_ PVOID InvokeContext
    )
{
    OnlineChecksPreEnableUi = TRUE;
    SystemInformer_ShowOptions(L"OnlineChecks");
}

static VOID OnlineChecksRestartForPartner(
    _In_ PVOID InvokeContext
    )
{
    SystemInformer_PrepareForEarlyShutdown();

    if (NT_SUCCESS(PhShellProcessHacker(
        SystemInformer_GetWindowHandle(),
        L"-v -newinstance",
        SW_SHOW,
        PH_SHELL_EXECUTE_DEFAULT,
        PH_SHELL_APP_PROPAGATE_PARAMETERS | PH_SHELL_APP_PROPAGATE_PARAMETERS_IGNORE_VISIBILITY,
        0,
        NULL
        )))
    {
        SystemInformer_Destroy();
    }
    else
    {
        SystemInformer_CancelEarlyShutdown();
    }
}

_Function_class_(PH_CALLBACK_FUNCTION)
VOID NTAPI MainWindowShowingCallback(
    _In_ PVOID Parameter,
    _In_ PVOID Context
    )
{
    //INT_PTR result;

    //if (PhGetIntegerSetting(SETTING_NAME_PARTNER_PROMPT_SHOWN))
    //    return;

    //result = PhDialogBox(
    //    PluginInstance->DllBase,
    //    MAKEINTRESOURCE(IDD_PARTNER),
    //    SystemInformer_GetWindowHandle(),
    //    PartnerDialogProc,
    //    NULL
    //    );

    //PhSetIntegerSetting(SETTING_NAME_PARTNER_PROMPT_SHOWN, TRUE);

    //if (result == IDC_PARTNER_CONFIGURE)
    //{
    //    SystemInformer_Invoke(OnlineChecksShowPartnerOptions, NULL);
    //}
    //else
    //{
    //    PhSetIntegerSetting(SETTING_NAME_SCAN_ENABLED, TRUE);
    //    PhSetIntegerSetting(SETTING_NAME_HYBRIDANALYSIS_LOOKUPS_ENABLED, TRUE);
    //    PhSetIntegerSetting(SETTING_NAME_HYBRIDANALYSIS_SUBMIT_ENABLED, TRUE);
    //    SystemInformer_Invoke(OnlineChecksRestartForPartner, NULL);
    //}
}
