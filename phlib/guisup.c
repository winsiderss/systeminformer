/*
 * Copyright (c) 2022 Winsider Seminars & Solutions, Inc.  All rights reserved.
 *
 * This file is part of System Informer.
 *
 * Authors:
 *
 *     wj32    2009-2016
 *     dmex    2017-2024
 *
 */

#include <ph.h>
#include <apiimport.h>
#include <guisup.h>
#include <mapimg.h>
#include <mapldr.h>
#include <settings.h>
#include <guisupp.h>

#include <commoncontrols.h>
#include <shellscalingapi.h>
#include <wincodec.h>
#include <uxtheme.h>

BOOLEAN NTAPI PhpWindowContextHashtableEqualFunction(
    _In_ PVOID Entry1,
    _In_ PVOID Entry2
    );
ULONG NTAPI PhpWindowContextHashtableHashFunction(
    _In_ PVOID Entry
    );
BOOLEAN NTAPI PhpWindowCallbackHashtableEqualFunction(
    _In_ PVOID Entry1,
    _In_ PVOID Entry2
    );
ULONG NTAPI PhpWindowCallbackHashtableHashFunction(
    _In_ PVOID Entry
    );

typedef struct _PH_WINDOW_PROPERTY_CONTEXT
{
    ULONG PropertyHash;
    HWND WindowHandle;
    PVOID Context;
} PH_WINDOW_PROPERTY_CONTEXT, *PPH_WINDOW_PROPERTY_CONTEXT;

HFONT PhApplicationFont = NULL;
HFONT PhTreeWindowFont = NULL;
HFONT PhMonospaceFont = NULL;
LONG PhFontQuality = 0;
LONG PhSystemDpi = 96;
PH_INTEGER_PAIR PhSmallIconSize = { 16, 16 };
PH_INTEGER_PAIR PhLargeIconSize = { 32, 32 };

static PH_INITONCE SharedIconCacheInitOnce = PH_INITONCE_INIT;
static PPH_HASHTABLE SharedIconCacheHashtable;
static PH_QUEUED_LOCK SharedIconCacheLock = PH_QUEUED_LOCK_INIT;

static PPH_HASHTABLE WindowCallbackHashTable = NULL;
static PH_QUEUED_LOCK WindowCallbackListLock = PH_QUEUED_LOCK_INIT;
static PPH_HASHTABLE WindowContextHashTable = NULL;
static PH_QUEUED_LOCK WindowContextListLock = PH_QUEUED_LOCK_INIT;

static __typeof__(&OpenThemeDataForDpi) OpenThemeDataForDpi_I = NULL;
static __typeof__(&OpenThemeData) OpenThemeData_I = NULL;
static __typeof__(&CloseThemeData) CloseThemeData_I = NULL;
static __typeof__(&SetWindowTheme) SetWindowTheme_I = NULL;
static __typeof__(&IsThemeActive) IsThemeActive_I = NULL;
static __typeof__(&IsThemePartDefined) IsThemePartDefined_I = NULL;
static _GetThemeClass GetThemeClass_I = NULL;
static __typeof__(&GetThemeColor) GetThemeColor_I = NULL;
static __typeof__(&GetThemeInt) GetThemeInt_I = NULL;
static __typeof__(&GetThemePartSize) GetThemePartSize_I = NULL;
static __typeof__(&DrawThemeBackground) DrawThemeBackground_I = NULL;
static __typeof__(&DrawThemeTextEx) DrawThemeTextEx_I = NULL;
static _AllowDarkModeForWindow AllowDarkModeForWindow_I = NULL; // Win10-RS5 (uxtheme.dll ordinal 133)
static _IsDarkModeAllowedForWindow IsDarkModeAllowedForWindow_I = NULL; // Win10-RS5 (uxtheme.dll ordinal 137)
static __typeof__(&GetDpiForMonitor) GetDpiForMonitor_I = NULL; // win81+
static __typeof__(&GetDpiForWindow) GetDpiForWindow_I = NULL; // win10rs1+
static __typeof__(&GetDpiForSystem) GetDpiForSystem_I = NULL; // win10rs1+
//static _GetDpiForSession GetDpiForSession_I = NULL; // ordinal 2713
static __typeof__(&GetSystemMetricsForDpi) GetSystemMetricsForDpi_I = NULL;
static __typeof__(&SystemParametersInfoForDpi) SystemParametersInfoForDpi_I = NULL;
static _CreateMRUList CreateMRUList_I = NULL;
static _AddMRUString AddMRUString_I = NULL;
static _EnumMRUList EnumMRUList_I = NULL;
static _FreeMRUList FreeMRUList_I = NULL;

VOID PhGuiSupportInitialization(
    VOID
    )
{
    PVOID baseAddress;

    WindowCallbackHashTable = PhCreateHashtable(
        sizeof(PH_PLUGIN_WINDOW_CALLBACK_REGISTRATION),
        PhpWindowCallbackHashtableEqualFunction,
        PhpWindowCallbackHashtableHashFunction,
        10
        );
    WindowContextHashTable = PhCreateHashtable(
        sizeof(PH_WINDOW_PROPERTY_CONTEXT),
        PhpWindowContextHashtableEqualFunction,
        PhpWindowContextHashtableHashFunction,
        10
        );

    if (baseAddress = PhLoadLibrary(L"uxtheme.dll"))
    {
        OpenThemeDataForDpi_I = PhGetDllBaseProcedureAddress(baseAddress, "OpenThemeDataForDpi", 0);
        OpenThemeData_I = PhGetDllBaseProcedureAddress(baseAddress, "OpenThemeData", 0);
        CloseThemeData_I = PhGetDllBaseProcedureAddress(baseAddress, "CloseThemeData", 0);
        SetWindowTheme_I = PhGetDllBaseProcedureAddress(baseAddress, "SetWindowTheme", 0);
        IsThemeActive_I = PhGetDllBaseProcedureAddress(baseAddress, "IsThemeActive", 0);
        IsThemePartDefined_I = PhGetDllBaseProcedureAddress(baseAddress, "IsThemePartDefined", 0);
        GetThemeColor_I = PhGetDllBaseProcedureAddress(baseAddress, "GetThemeColor", 0);
        GetThemeInt_I = PhGetDllBaseProcedureAddress(baseAddress, "GetThemeInt", 0);
        GetThemePartSize_I = PhGetDllBaseProcedureAddress(baseAddress, "GetThemePartSize", 0);
        DrawThemeBackground_I = PhGetDllBaseProcedureAddress(baseAddress, "DrawThemeBackground", 0);

        if (WindowsVersion >= WINDOWS_11)
        {
            GetThemeClass_I = PhGetDllBaseProcedureAddress(baseAddress, NULL, 74);
        }
        if (WindowsVersion >= WINDOWS_10_RS5)
        {
            AllowDarkModeForWindow_I = PhGetDllBaseProcedureAddress(baseAddress, NULL, 133);
            IsDarkModeAllowedForWindow_I = PhGetDllBaseProcedureAddress(baseAddress, NULL, 137);
        }
    }

    if (WindowsVersion >= WINDOWS_8_1)
    {
        if (baseAddress = PhLoadLibrary(L"shcore.dll"))
        {
            GetDpiForMonitor_I = PhGetDllBaseProcedureAddress(baseAddress, "GetDpiForMonitor", 0);
        }
    }

    if (WindowsVersion >= WINDOWS_10_RS1)
    {
        if (baseAddress = PhLoadLibrary(L"user32.dll"))
        {
            GetDpiForWindow_I = PhGetDllBaseProcedureAddress(baseAddress, "GetDpiForWindow", 0);
            GetDpiForSystem_I = PhGetDllBaseProcedureAddress(baseAddress, "GetDpiForSystem", 0);
            GetSystemMetricsForDpi_I = PhGetDllBaseProcedureAddress(baseAddress, "GetSystemMetricsForDpi", 0);
            SystemParametersInfoForDpi_I = PhGetDllBaseProcedureAddress(baseAddress, "SystemParametersInfoForDpi", 0);
        }
    }

    PhGuiSupportUpdateSystemMetrics(NULL, 0);
}

VOID PhGuiSupportUpdateSystemMetrics(
    _In_opt_ HWND WindowHandle,
    _In_opt_ LONG WindowDpi
    )
{
    PhSystemDpi = WindowDpi ? WindowDpi : (WindowHandle ? PhGetWindowDpi(WindowHandle) : PhGetSystemDpi());
    PhSmallIconSize.X = PhGetSystemMetrics(SM_CXSMICON, PhSystemDpi);
    PhSmallIconSize.Y = PhGetSystemMetrics(SM_CYSMICON, PhSystemDpi);
    PhLargeIconSize.X = PhGetSystemMetrics(SM_CXICON, PhSystemDpi);
    PhLargeIconSize.Y = PhGetSystemMetrics(SM_CYICON, PhSystemDpi);
}

LONG PhGetFontQualitySetting(
    _In_ LONG FontQuality
    )
{
    switch (FontQuality)
    {
    case 0: return DEFAULT_QUALITY;
    case 1: return DRAFT_QUALITY;
    case 2: return PROOF_QUALITY;
    case 3: return NONANTIALIASED_QUALITY;
    case 4: return ANTIALIASED_QUALITY;
    case 5: return CLEARTYPE_QUALITY;
    case 6: return CLEARTYPE_NATURAL_QUALITY;
    }

    return DEFAULT_QUALITY;
}

HFONT PhCreateFont(
    _In_opt_ PCWSTR Name,
    _In_ LONG Size,
    _In_ LONG Weight,
    _In_ LONG PitchAndFamily,
    _In_ LONG Dpi
    )
{
    return CreateFont(
        -(LONG)PhMultiplyDivide(Size, Dpi, 72),
        0,
        0,
        0,
        Weight,
        FALSE,
        FALSE,
        FALSE,
        ANSI_CHARSET,
        OUT_DEFAULT_PRECIS,
        CLIP_DEFAULT_PRECIS,
        PhFontQuality,
        PitchAndFamily,
        Name
        );
}

HFONT PhCreateCommonFont(
    _In_ LONG Size,
    _In_ LONG Weight,
    _In_opt_ HWND WindowHandle,
    _In_ LONG WindowDpi
    )
{
    HFONT fontHandle;
    LOGFONT logFont;

    if (!PhGetSystemParametersInfo(SPI_GETICONTITLELOGFONT, sizeof(LOGFONT), &logFont, WindowDpi))
        return NULL;

    fontHandle = CreateFont(
        -PhMultiplyDivideSigned(Size, WindowDpi, 72),
        0,
        0,
        0,
        Weight,
        FALSE,
        FALSE,
        FALSE,
        ANSI_CHARSET,
        OUT_DEFAULT_PRECIS,
        CLIP_DEFAULT_PRECIS,
        PhFontQuality,
        DEFAULT_PITCH,
        logFont.lfFaceName
        );

    if (!fontHandle)
        return NULL;

    if (WindowHandle)
    {
        SetWindowFont(WindowHandle, fontHandle, TRUE);
    }

    return fontHandle;
}

HFONT PhCreateIconTitleFont(
    _In_opt_ LONG WindowDpi
    )
{
    LOGFONT logFont;

    if (PhGetSystemParametersInfo(SPI_GETICONTITLELOGFONT, sizeof(LOGFONT), &logFont, WindowDpi))
    {
        logFont.lfQuality = (UCHAR)PhFontQuality;
        return CreateFontIndirect(&logFont);
    }

    return NULL;
}

HFONT PhCreateMessageFont(
    _In_opt_ LONG WindowDpi
    )
{
    NONCLIENTMETRICS metrics = { sizeof(metrics) };

    if (PhGetSystemParametersInfo(SPI_GETNONCLIENTMETRICS, sizeof(metrics), &metrics, WindowDpi))
    {
        metrics.lfMessageFont.lfQuality = (UCHAR)PhFontQuality;
        return CreateFontIndirect(&metrics.lfMessageFont);
    }

    return NULL;
}

HFONT PhDuplicateFont(
    _In_ HFONT Font
    )
{
    LOGFONT logFont;

    if (GetObject(Font, sizeof(LOGFONT), &logFont))
    {
        logFont.lfQuality = (UCHAR)PhFontQuality;
        return CreateFontIndirect(&logFont);
    }

    return NULL;
}

HFONT PhDuplicateFontWithNewWeight(
    _In_ HFONT Font,
    _In_ LONG NewWeight
    )
{
    LOGFONT logFont;

    if (GetObject(Font, sizeof(LOGFONT), &logFont))
    {
        logFont.lfWeight = NewWeight;
        logFont.lfQuality = (UCHAR)PhFontQuality;
        return CreateFontIndirect(&logFont);
    }

    return NULL;
}

HFONT PhDuplicateFontWithNewHeight(
    _In_ HFONT Font,
    _In_ LONG NewHeight,
    _In_ LONG dpiValue
    )
{
    LOGFONT logFont;

    if (GetObject(Font, sizeof(LOGFONT), &logFont))
    {
        logFont.lfHeight = PhGetDpi(NewHeight, dpiValue);
        logFont.lfQuality = (UCHAR)PhFontQuality;
        return CreateFontIndirect(&logFont);
    }

    return NULL;
}


HFONT PhInitializeFont(
    _In_ LONG WindowDpi
    )
{
    HFONT fontHandle;

    if (fontHandle = PhCreateFont(L"Microsoft Sans Serif", 8, FW_NORMAL, DEFAULT_PITCH, WindowDpi))
        return fontHandle;
    if (fontHandle = PhCreateFont(L"Tahoma", 8, FW_NORMAL, DEFAULT_PITCH, WindowDpi))
        return fontHandle;
    if (fontHandle = PhCreateMessageFont(WindowDpi))
        return fontHandle;

    return GetStockFont(DEFAULT_GUI_FONT);
}

HFONT PhInitializeMonospaceFont(
    _In_ LONG WindowDpi
    )
{
    HFONT fontHandle;

    if (fontHandle = PhCreateFont(L"Lucida Console", 9, FW_DONTCARE, FF_MODERN, WindowDpi))
        return fontHandle;
    if (fontHandle = PhCreateFont(L"Courier New", 9, FW_DONTCARE, FF_MODERN, WindowDpi))
        return fontHandle;
    if (fontHandle = PhCreateFont(NULL, 9, FW_DONTCARE, FF_MODERN, WindowDpi))
        return fontHandle;

    //{
    //    NONCLIENTMETRICS metrics;
    //
    //    memset(&metrics, 0, sizeof(NONCLIENTMETRICS));
    //    metrics.cbSize = sizeof(NONCLIENTMETRICS);
    //
    //    if (PhGetSystemParametersInfo(SPI_GETNONCLIENTMETRICS, sizeof(NONCLIENTMETRICS), &metrics, WindowDpi))
    //    {
    //        return CreateFontIndirect(&metrics.lfMessageFont);
    //    }
    //}

    LOGFONT logFont;

    if (GetObject(GetStockFont(SYSTEM_FIXED_FONT), sizeof(LOGFONT), &logFont))
    {
        logFont.lfWeight = -(LONG)PhMultiplyDivide(logFont.lfWeight, WindowDpi, 72);
        return CreateFontIndirect(&logFont);
    }

    return GetStockFont(SYSTEM_FIXED_FONT);
}

/**
 * The PhGetScreenDC function retrieves a handle to a device context (DC) for the entire screen.
 *
 * \return The handle to the requested stock object.
 */
HDC PhGetScreenDC(
    VOID
    )
{
    static HDC hdc = NULL;

    if (hdc == NULL)
    {
        hdc = GetDC(NULL);
    }

    return hdc;
}

/**
 * The PhGetStockBrush function retrieves a handle to one of the stock pens, brushes, fonts, or palettes.
 *
 * \param Index The type of stock object.
 * \return The handle to the requested stock object.
 */
HGDIOBJ PhGetStockObject(
    _In_ LONG Index
    )
{
    static HBRUSH brush[STOCK_LAST + 1] = { NULL };

    assert(Index <= STOCK_LAST);

    if (brush[Index] == NULL)
    {
        brush[Index] = GetStockObject(Index);
    }

    return brush[Index];
}

/**
 * Opens the theme data for the specified window handle, class list, and window DPI.
 *
 * \param WindowHandle The handle to the window for which to open the theme data. Can be NULL.
 * \param ClassList The class list of the theme to open.
 * \param WindowDpi The DPI of the window.
 * \return The handle to the opened theme data, or NULL if the theme data could not be opened.
 */
HTHEME PhOpenThemeData(
    _In_opt_ HWND WindowHandle,
    _In_ PCWSTR ClassList,
    _In_ LONG WindowDpi
    )
{
    if (OpenThemeDataForDpi_I && WindowDpi)
    {
        return OpenThemeDataForDpi_I(WindowHandle, ClassList, WindowDpi);
    }

    if (OpenThemeData_I)
    {
        return OpenThemeData_I(WindowHandle, ClassList);
    }

    return NULL;
}

VOID PhCloseThemeData(
    _In_ HTHEME ThemeHandle
    )
{
    if (CloseThemeData_I)
    {
        CloseThemeData_I(ThemeHandle);
    }
}

VOID PhSetControlTheme(
    _In_ HWND Handle,
    _In_opt_ PCWSTR Theme
    )
{
    if (SetWindowTheme_I)
    {
        SetWindowTheme_I(Handle, Theme, NULL);
    }
}

BOOLEAN PhIsThemeActive(
    VOID
    )
{
    if (!IsThemeActive_I)
        return FALSE;

    return !!IsThemeActive_I();
}

BOOLEAN PhIsThemePartDefined(
    _In_ HTHEME ThemeHandle,
    _In_ LONG PartId,
    _In_ LONG StateId
    )
{
    if (!IsThemePartDefined_I)
        return FALSE;

    return !!IsThemePartDefined_I(ThemeHandle, PartId, StateId);
}

_Success_(return)
BOOLEAN PhGetThemeClass(
    _In_ HTHEME ThemeHandle,
    _Out_writes_z_(ClassLength) PWSTR Class,
    _In_ ULONG ClassLength
    )
{
    if (!GetThemeClass_I)
        return FALSE;

    return SUCCEEDED(GetThemeClass_I(ThemeHandle, Class, ClassLength));
}

_Success_(return)
BOOLEAN PhGetThemeColor(
    _In_ HTHEME ThemeHandle,
    _In_ LONG PartId,
    _In_ LONG StateId,
    _In_ LONG PropId,
    _Out_ COLORREF* Color
    )
{
    if (!GetThemeColor_I)
        return FALSE;

    return SUCCEEDED(GetThemeColor_I(ThemeHandle, PartId, StateId, PropId, Color));
}

_Success_(return)
BOOLEAN PhGetThemeInt(
    _In_ HTHEME ThemeHandle,
    _In_ LONG PartId,
    _In_ LONG StateId,
    _In_ LONG PropId,
    _Out_ PLONG Value
    )
{
    if (!GetThemeInt_I)
        return FALSE;

    return SUCCEEDED(GetThemeInt_I(ThemeHandle, PartId, StateId, PropId, Value));
}

_Success_(return)
BOOLEAN PhGetThemePartSize(
    _In_ HTHEME ThemeHandle,
    _In_opt_ HDC hdc,
    _In_ LONG PartId,
    _In_ LONG StateId,
    _In_opt_ LPCRECT Rect,
    _In_ THEMEPARTSIZE Flags,
    _Out_ PSIZE Size
    )
{
    if (!GetThemePartSize_I)
        return FALSE;

    return SUCCEEDED(GetThemePartSize_I(ThemeHandle, hdc, PartId, StateId, Rect, (enum THEMESIZE)Flags, Size));
}

BOOLEAN PhDrawThemeBackground(
    _In_ HTHEME ThemeHandle,
    _In_ HDC hdc,
    _In_ LONG PartId,
    _In_ LONG StateId,
    _In_ LPCRECT Rect,
    _In_opt_ LPCRECT ClipRect
    )
{
    if (!DrawThemeBackground_I)
        return FALSE;

    return SUCCEEDED(DrawThemeBackground_I(ThemeHandle, hdc, PartId, StateId, Rect, ClipRect));
}

BOOLEAN PhDrawThemeTextEx(
    _In_ HTHEME ThemeHandle,
    _In_ HDC hdc,
    _In_ LONG PartId,
    _In_ LONG StateId,
    _In_reads_(cchText) LPCWSTR Text,
    _In_ LONG cchText,
    _In_ ULONG TextFlags,
    _Inout_ LPRECT Rect,
    _In_opt_ const PVOID Options // DTTOPTS*
    )
{
    if (!DrawThemeTextEx_I)
        DrawThemeTextEx_I = PhGetModuleProcAddress(L"uxtheme.dll", "DrawThemeTextEx");

    if (!DrawThemeTextEx_I)
        return FALSE;

    return SUCCEEDED(DrawThemeTextEx_I(ThemeHandle, hdc, PartId, StateId, Text, cchText, TextFlags, Rect, Options));
}

BOOLEAN PhAllowDarkModeForWindow(
    _In_ HWND WindowHandle,
    _In_ BOOL Enabled
    )
{
    if (!AllowDarkModeForWindow_I)
        return FALSE;

    return !!AllowDarkModeForWindow_I(WindowHandle, Enabled);
}

BOOLEAN PhIsDarkModeAllowedForWindow(
    _In_ HWND WindowHandle
    )
{
    if (!IsDarkModeAllowedForWindow_I)
        return FALSE;

    return !!IsDarkModeAllowedForWindow_I(WindowHandle);
}

// rev from EtwRundown.dll!EtwpLogDPISettingsInfo (dmex)
//LONG PhGetUserOrMachineDpi(
//    VOID
//    )
//{
//    static PH_STRINGREF machineKeyName = PH_STRINGREF_INIT(L"System\\CurrentControlSet\\Hardware Profiles\\Current\\Software\\Fonts");
//    static PH_STRINGREF userKeyName = PH_STRINGREF_INIT(L"Control Panel\\Desktop");
//    HANDLE keyHandle;
//    LONG dpi = 0;
//
//    if (NT_SUCCESS(PhOpenKey(&keyHandle, KEY_QUERY_VALUE, PH_KEY_USERS, &userKeyName, 0)))
//    {
//        dpi = PhQueryRegistryUlongZ(keyHandle, L"LogPixels");
//        NtClose(keyHandle);
//    }
//
//    if (dpi == 0)
//    {
//        if (NT_SUCCESS(PhOpenKey(&keyHandle, KEY_QUERY_VALUE, PH_KEY_LOCAL_MACHINE, &machineKeyName, 0)))
//        {
//            dpi = PhQueryRegistryUlongZ(keyHandle, L"LogPixels");
//            NtClose(keyHandle);
//        }
//    }
//
//    return dpi;
//}

BOOLEAN PhGetWindowRect(
    _In_ HWND WindowHandle,
    _Out_ PRECT WindowRect
    )
{
    // Note: GetWindowRect can return success with either invalid (0,0) or empty rects (40,40) and in some cases
    // this results in unwanted clipping, performance issues with the CreateCompatibleBitmap double buffering and
    // issues with MonitorFromRect layout and DPI queries, so ignore the return status and check the rect (dmex)

    GetWindowRect(WindowHandle, WindowRect);

    if (PhRectEmpty(WindowRect))
        return FALSE;

    return TRUE;
}

BOOLEAN PhGetClientRect(
    _In_ HWND WindowHandle,
    _Out_ PRECT ClientRect
    )
{
    GetClientRect(WindowHandle, ClientRect);

    if (!(ClientRect->right && ClientRect->bottom))
        return FALSE;

    return TRUE;
}

BOOLEAN PhIsHungAppWindow(
    _In_ HWND WindowHandle
    )
{
    return !!IsHungAppWindow(WindowHandle);
}

HWND PhGetShellWindow(
    VOID
    )
{
    return GetShellWindow();
}

BOOLEAN PhSetChildWindowNoActivate(
    _In_ HWND WindowHandle,
    _In_ HANDLE ThreadId
    )
{
    typedef ULONG (WINAPI* SetChildWindowNoActivate)(
        _In_ HWND WindowHandle
        );
    static PH_INITONCE initOnce = PH_INITONCE_INIT;
    static __typeof__(SetChildWindowNoActivate) SetChildWindowNoActivate_I = NULL; // NtUserSetChildWindowNoActivate

    if (PhBeginInitOnce(&initOnce))
    {
        PVOID baseAddress;

        if (baseAddress = PhLoadLibrary(L"user32.dll"))
        {
            SetChildWindowNoActivate_I = PhGetDllBaseProcedureAddress(baseAddress, NULL, 2005);
        }

        PhEndInitOnce(&initOnce);
    }

    if (!SetChildWindowNoActivate_I)
        return FALSE;

    return !!SetChildWindowNoActivate_I(WindowHandle);
}

BOOLEAN PhCheckWindowThreadDesktop(
    _In_ HWND WindowHandle,
    _In_ HANDLE ThreadId
    )
{
    typedef LOGICAL (WINAPI* CheckWindowThreadDesktop)(
        _In_ HWND WindowHandle,
        _In_ ULONG ThreadId
        );
    static PH_INITONCE initOnce = PH_INITONCE_INIT;
    static CheckWindowThreadDesktop CheckWindowThreadDesktop_I = NULL;

    if (PhBeginInitOnce(&initOnce))
    {
        PVOID baseAddress;

        if (baseAddress = PhLoadLibrary(L"user32.dll"))
        {
            CheckWindowThreadDesktop_I = PhGetDllBaseProcedureAddress(baseAddress, "CheckWindowThreadDesktop", 0);
        }

        PhEndInitOnce(&initOnce);
    }

    if (!CheckWindowThreadDesktop_I)
        return FALSE;

    return !!CheckWindowThreadDesktop_I(WindowHandle, HandleToUlong(ThreadId));
}

LONG PhGetDpi(
    _In_ LONG Number,
    _In_ LONG DpiValue
    )
{
    return PhMultiplyDivideSigned(Number, DpiValue, USER_DEFAULT_SCREEN_DPI);
}

LONG PhGetMonitorDpi(
    _In_ LPCRECT rect
    )
{
    return PhGetDpiValue(NULL, rect);
}

LONG PhGetSystemDpi(
    VOID
    )
{
    LONG dpi;

    dpi = PhGetTaskbarDpi();

    if (dpi == 0)
    {
        dpi = PhGetDpiValue(NULL, NULL);
    }

    if (dpi == 0)
    {
        dpi = USER_DEFAULT_SCREEN_DPI;
    }

    return dpi;
}

// rev from GetDpiForShellUIComponent (dmex)
//LONG PhGetShellDpi(
//    VOID
//    )
//{
//    static HWND trayWindow = NULL;
//    HWND windowHandle;
//    LONG dpi = 0;
//
//    if (IsWindow(trayWindow))
//    {
//        windowHandle = trayWindow;
//    }
//    else
//    {
//        windowHandle = trayWindow = FindWindow(L"Shell_TrayWnd", NULL);
//    }
//
//    if (windowHandle)
//    {
//        dpi = HandleToLong(GetProp(windowHandle, L"TaskbarDPI_NotificationArea"));
//    }
//
//    //if (dpi == 0)
//    //{
//    //    dpi = GetDpiForShellUIComponent(SHELL_UI_COMPONENT_NOTIFICATIONAREA);
//    //}
//
//    if (dpi == 0)
//    {
//        dpi = USER_DEFAULT_SCREEN_DPI;
//    }
//
//    return dpi;
//}

LONG PhGetTaskbarDpi(
    VOID
    )
{
    LONG dpi = 0;
    HWND windowHandle;
    RECT windowRect = { 0 };

    if (windowHandle = PhGetShellWindow())
    {
        GetWindowRect(windowHandle, &windowRect);
    }

    if (PhRectEmpty(&windowRect))
    {
        if (windowHandle = GetDesktopWindow())
        {
            GetWindowRect(windowHandle, &windowRect);
        }
    }

    //if (PhRectEmpty(&windowRect))
    //{
    //    APPBARDATA appbarData = { sizeof(APPBARDATA) };
    //
    //    if (SHAppBarMessage(ABM_GETTASKBARPOS, &appbarData))
    //    {
    //        windowRect = appbarData.rc;
    //    }
    //}

    if (!PhRectEmpty(&windowRect))
    {
        dpi = PhGetMonitorDpi(&windowRect);
    }

    if (dpi == 0)
    {
        dpi = PhGetDpiValue(NULL, NULL);
    }

    //if (dpi == 0)
    //{
    //    dpi = PhGetShellDpi();
    //}

    if (dpi == 0)
    {
        dpi = USER_DEFAULT_SCREEN_DPI;
    }

    return dpi;
}

LONG PhGetWindowDpi(
    _In_ HWND WindowHandle
    )
{
    LONG dpi;

    if (WindowsVersion >= WINDOWS_10)
    {
        RECT windowRect;

        dpi = PhGetDpiValue(WindowHandle, NULL);

        if (dpi == 0)
        {
            if (PhGetWindowRect(WindowHandle, &windowRect))
            {
                dpi = PhGetDpiValue(NULL, &windowRect);
            }
        }
    }
    else
    {
        HDC screenHdc;

        if (screenHdc = GetDC(WindowHandle))
        {
            dpi = GetDeviceCaps(screenHdc, LOGPIXELSX);
            ReleaseDC(WindowHandle, screenHdc);
        }
        else
        {
            dpi = USER_DEFAULT_SCREEN_DPI;
        }
    }

    if (dpi == 0)
    {
        dpi = USER_DEFAULT_SCREEN_DPI;
    }

    return dpi;
}

LONG PhGetDpiValue(
    _In_opt_ HWND WindowHandle,
    _In_opt_ LPCRECT Rect
    )
{
    if (Rect || WindowHandle)
    {
        // Windows 10 (RS1)

        if (GetDpiForWindow_I && WindowHandle)
        {
            LONG dpi;

            if (dpi = GetDpiForWindow_I(WindowHandle))
            {
                return dpi;
            }
        }

        // Windows 8.1

        if (GetDpiForMonitor_I)
        {
            HMONITOR monitor;
            LONG dpi_x;
            LONG dpi_y;

            if (Rect)
                monitor = MonitorFromRect(Rect, MONITOR_DEFAULTTONEAREST);
            else
                monitor = MonitorFromWindow(WindowHandle, MONITOR_DEFAULTTONEAREST);

            if (HR_SUCCESS(GetDpiForMonitor_I(monitor, MDT_EFFECTIVE_DPI, &dpi_x, &dpi_y)))
            {
                return dpi_x;
            }
        }
    }

    // Windows 10 (RS1)

    if (GetDpiForSystem_I)
    {
        return GetDpiForSystem_I();
    }

    // Windows 7 and Windows 8
    {
        HDC screenHdc;
        LONG dpi_x;

        if (screenHdc = GetDC(NULL))
        {
            dpi_x = GetDeviceCaps(screenHdc, LOGPIXELSX);
            ReleaseDC(NULL, screenHdc);

            if (dpi_x != 0)
            {
                return dpi_x;
            }
        }
    }

    return USER_DEFAULT_SCREEN_DPI;
}

/**
 * Retrieves the system metrics for the specified index.
 *
 * \param Index The index of the system metric to retrieve.
 * \param DpiValue The DPI value to use for retrieving the system metric. If not provided, the default DPI value will be used.
 * \return The value of the system metric.
 */
LONG PhGetSystemMetrics(
    _In_ LONG Index,
    _In_opt_ LONG DpiValue
    )
{
    if (GetSystemMetricsForDpi_I && DpiValue)
    {
        return GetSystemMetricsForDpi_I(Index, DpiValue);
    }

    return GetSystemMetrics(Index);
}

BOOLEAN PhGetSystemSafeBootMode(
    VOID
    )
{
    if (WindowsVersion < WINDOWS_NEW)
    {
        return !!USER_SHARED_DATA->SafeBootMode;
    }

    return !!PhGetSystemMetrics(SM_CLEANBOOT, 0);
}

BOOL PhGetSystemParametersInfo(
    _In_ LONG Action,
    _In_ ULONG Param1,
    _Pre_maybenull_ _Post_valid_ PVOID Param2,
    _In_opt_ LONG DpiValue
    )
{
    if (SystemParametersInfoForDpi_I && DpiValue)
    {
        return SystemParametersInfoForDpi_I(Action, Param1, Param2, 0, DpiValue);
    }

    return SystemParametersInfo(Action, Param1, Param2, 0);
}

VOID PhGetSizeDpiValue(
    _Inout_ PRECT rect,
    _In_ LONG DpiValue,
    _In_ BOOLEAN isUnpack
    )
{
    PH_RECTANGLE rectangle;
    LONG numerator;
    LONG denominator;

    if (DpiValue == USER_DEFAULT_SCREEN_DPI)
        return;

    if (isUnpack)
    {
        numerator = DpiValue;
        denominator = USER_DEFAULT_SCREEN_DPI;
    }
    else
    {
        numerator = USER_DEFAULT_SCREEN_DPI;
        denominator = DpiValue;
    }

    rectangle.Left = rect->left;
    rectangle.Top = rect->top;
    rectangle.Width = rect->right - rect->left;
    rectangle.Height = rect->bottom - rect->top;

    if (rectangle.Left)
        rectangle.Left = PhMultiplyDivideSigned(rectangle.Left, numerator, denominator);
    if (rectangle.Top)
        rectangle.Top = PhMultiplyDivideSigned(rectangle.Top, numerator, denominator);
    if (rectangle.Width)
        rectangle.Width = PhMultiplyDivideSigned(rectangle.Width, numerator, denominator);
    if (rectangle.Height)
        rectangle.Height = PhMultiplyDivideSigned(rectangle.Height, numerator, denominator);

    rect->left = rectangle.Left;
    rect->top = rectangle.Top;
    rect->right = rectangle.Left + rectangle.Width;
    rect->bottom = rectangle.Top + rectangle.Height;
}

LONG PhAddTabControlTab(
    _In_ HWND TabControlHandle,
    _In_ LONG Index,
    _In_ PCWSTR Text
    )
{
    TCITEM item;

    item.mask = TCIF_TEXT;
    item.pszText = (PWSTR)Text;

    return TabCtrl_InsertItem(TabControlHandle, Index, &item);
}

PPH_STRING PhGetWindowText(
    _In_ HWND WindowHandle
    )
{
    PPH_STRING text;

    PhGetWindowTextEx(WindowHandle, 0, &text);
    return text;
}

ULONG PhGetWindowTextEx(
    _In_ HWND WindowHandle,
    _In_ ULONG Flags,
    _Out_opt_ PPH_STRING *Text
    )
{
    PPH_STRING string;
    ULONG length;

    if (Flags & PH_GET_WINDOW_TEXT_INTERNAL)
    {
        if (Flags & PH_GET_WINDOW_TEXT_LENGTH_ONLY)
        {
            WCHAR buffer[32];
            length = InternalGetWindowText(WindowHandle, buffer, sizeof(buffer) / sizeof(WCHAR));
            if (Text) *Text = NULL;
        }
        else
        {
            // TODO: Resize the buffer until we get the entire thing.
            string = PhCreateStringEx(NULL, 256 * sizeof(WCHAR));
            length = InternalGetWindowText(WindowHandle, string->Buffer, (ULONG)string->Length / sizeof(WCHAR) + 1);
            string->Length = length * sizeof(WCHAR);

            if (Text)
                *Text = string;
            else
                PhDereferenceObject(string);
        }

        return length;
    }
    else
    {
        length = GetWindowTextLength(WindowHandle);

        if (length == 0 || (Flags & PH_GET_WINDOW_TEXT_LENGTH_ONLY))
        {
            if (Text)
                *Text = PhReferenceEmptyString();

            return length;
        }

        string = PhCreateStringEx(NULL, length * sizeof(WCHAR));

        if (GetWindowText(WindowHandle, string->Buffer, (ULONG)string->Length / sizeof(WCHAR) + 1))
        {
            if (Text)
                *Text = string;
            else
                PhDereferenceObject(string);

            return length;
        }
        else
        {
            if (Text)
                *Text = PhReferenceEmptyString();

            PhDereferenceObject(string);

            return 0;
        }
    }
}

NTSTATUS PhGetWindowTextToBuffer(
    _In_ HWND WindowHandle,
    _In_ ULONG Flags,
    _Out_writes_bytes_(BufferLength) PWSTR Buffer,
    _In_opt_ ULONG BufferLength,
    _Out_opt_ PULONG ReturnLength
    )
{
    NTSTATUS status;
    ULONG length;

    if (Flags & PH_GET_WINDOW_TEXT_INTERNAL)
        length = InternalGetWindowText(WindowHandle, Buffer, BufferLength);
    else
        length = GetWindowText(WindowHandle, Buffer, BufferLength);

    if (length == 0)
        status = PhGetLastWin32ErrorAsNtStatus();
    else
        status = STATUS_SUCCESS;

    if (ReturnLength)
        *ReturnLength = length;

    return status;
}

PPH_STRING PhGetComboBoxString(
    _In_ HWND WindowHandle,
    _In_ LONG Index
    )
{
    PPH_STRING string;
    LONG length;

    if (Index == INT_ERROR)
    {
        Index = ComboBox_GetCurSel(WindowHandle);

        if (Index == CB_ERR)
            return NULL;
    }

    length = ComboBox_GetLBTextLen(WindowHandle, Index);

    if (length == CB_ERR)
        return NULL;
    if (length == 0)
        return PhReferenceEmptyString();

    string = PhCreateStringEx(NULL, length * sizeof(WCHAR));

    if (ComboBox_GetLBText(WindowHandle, Index, string->Buffer) != CB_ERR)
    {
        return string;
    }
    else
    {
        PhDereferenceObject(string);
        return NULL;
    }
}

LONG PhSelectComboBoxString(
    _In_ HWND WindowHandle,
    _In_ PCWSTR String,
    _In_ BOOLEAN Partial
    )
{
    if (Partial)
    {
        return ComboBox_SelectString(WindowHandle, INT_ERROR, String);
    }
    else
    {
        LONG index;

        index = ComboBox_FindStringExact(WindowHandle, INT_ERROR, String);

        if (index == CB_ERR)
            return CB_ERR;

        ComboBox_SetCurSel(WindowHandle, index);

        InvalidateRect(WindowHandle, NULL, TRUE);

        return index;
    }
}

VOID PhDeleteComboBoxStrings(
    _In_ HWND ComboBoxHandle,
    _In_ BOOLEAN ResetContent
    )
{
    LONG total;

    if ((total = ComboBox_GetCount(ComboBoxHandle)) == CB_ERR)
        return;

    for (LONG i = 0; i < total; i++)
    {
        ComboBox_DeleteString(ComboBoxHandle, i);
    }

    if (ResetContent)
    {
        ComboBox_ResetContent(ComboBoxHandle);
    }
}


PPH_STRING PhGetListBoxString(
    _In_ HWND WindowHandle,
    _In_ LONG Index
    )
{
    PPH_STRING string;
    LONG length;

    if (Index == INT_ERROR)
    {
        Index = ListBox_GetCurSel(WindowHandle);

        if (Index == LB_ERR)
            return NULL;
    }

    length = ListBox_GetTextLen(WindowHandle, Index);

    if (length == LB_ERR)
        return NULL;
    if (length == 0)
        return NULL;

    string = PhCreateStringEx(NULL, length * sizeof(WCHAR));

    if (ListBox_GetText(WindowHandle, Index, string->Buffer) != LB_ERR)
    {
        return string;
    }
    else
    {
        PhDereferenceObject(string);
        return NULL;
    }
}

VOID PhSetImageListBitmap(
    _In_ HIMAGELIST ImageList,
    _In_ LONG Index,
    _In_ HINSTANCE InstanceHandle,
    _In_ PCWSTR BitmapName
    )
{
    HBITMAP bitmap;

    bitmap = LoadImage(InstanceHandle, BitmapName, IMAGE_BITMAP, 0, 0, 0);

    if (bitmap)
    {
        PhImageListReplace(ImageList, Index, bitmap, NULL);
        DeleteBitmap(bitmap);
    }
}

static BOOLEAN SharedIconCacheHashtableEqualFunction(
    _In_ PVOID Entry1,
    _In_ PVOID Entry2
    )
{
    PPHP_ICON_ENTRY entry1 = Entry1;
    PPHP_ICON_ENTRY entry2 = Entry2;

    if (entry1->InstanceHandle != entry2->InstanceHandle ||
        entry1->Width != entry2->Width ||
        entry1->Height != entry2->Height ||
        entry1->DpiValue != entry2->DpiValue)
    {
        return FALSE;
    }

    if (IS_INTRESOURCE(entry1->Name))
    {
        if (IS_INTRESOURCE(entry2->Name))
            return PtrToUlong(entry1->Name) == PtrToUlong(entry2->Name);
        else
            return FALSE;
    }
    else
    {
        if (!IS_INTRESOURCE(entry2->Name))
            return PhEqualStringZ(entry1->Name, entry2->Name, FALSE);
        else
            return FALSE;
    }
}

static ULONG SharedIconCacheHashtableHashFunction(
    _In_ PVOID Entry
    )
{
    PPHP_ICON_ENTRY entry = Entry;
    ULONG nameHash;

    if (IS_INTRESOURCE(entry->Name))
        nameHash = PtrToUlong(entry->Name);
    else
        nameHash = PhHashBytes((PUCHAR)entry->Name, PhCountStringZ(entry->Name));

    return nameHash ^ (PtrToUlong(entry->InstanceHandle) >> 5) ^ (entry->Width << 3) ^ entry->Height ^ entry->DpiValue;
}

HICON PhLoadIcon(
    _In_opt_ PVOID ImageBaseAddress,
    _In_ PCWSTR Name,
    _In_ ULONG Flags,
    _In_opt_ LONG Width,
    _In_opt_ LONG Height,
    _In_opt_ LONG SystemDpi
    )
{
    PHP_ICON_ENTRY entry;
    PPHP_ICON_ENTRY actualEntry;
    HICON icon = NULL;
    LONG width;
    LONG height;

    if (PhBeginInitOnce(&SharedIconCacheInitOnce))
    {
        SharedIconCacheHashtable = PhCreateHashtable(sizeof(PHP_ICON_ENTRY),
            SharedIconCacheHashtableEqualFunction, SharedIconCacheHashtableHashFunction, 10);
        PhEndInitOnce(&SharedIconCacheInitOnce);
    }

    if (Flags & PH_LOAD_ICON_SHARED)
    {
        PhAcquireQueuedLockExclusive(&SharedIconCacheLock);

        entry.InstanceHandle = ImageBaseAddress;
        entry.Name = Name;
        entry.Width = PhpGetIconEntrySize(Width, Flags);
        entry.Height = PhpGetIconEntrySize(Height, Flags);
        entry.DpiValue = SystemDpi;
        actualEntry = PhFindEntryHashtable(SharedIconCacheHashtable, &entry);

        if (actualEntry)
        {
            icon = actualEntry->Icon;
            PhReleaseQueuedLockExclusive(&SharedIconCacheLock);
            return icon;
        }
    }

    if (Flags & (PH_LOAD_ICON_SIZE_SMALL | PH_LOAD_ICON_SIZE_LARGE))
    {
        if (Flags & PH_LOAD_ICON_SIZE_SMALL)
        {
            width = PhGetSystemMetrics(SM_CXSMICON, SystemDpi);
            height = PhGetSystemMetrics(SM_CYSMICON, SystemDpi);
        }
        else
        {
            width = PhGetSystemMetrics(SM_CXICON, SystemDpi);
            height = PhGetSystemMetrics(SM_CYICON, SystemDpi);
        }

        LoadIconWithScaleDown(ImageBaseAddress, Name, width, height, &icon);
    }
    else
    {
        LoadIconWithScaleDown(ImageBaseAddress, Name, Width, Height, &icon);
    }

    if (!icon && !(Flags & PH_LOAD_ICON_STRICT))
    {
        if (Flags & PH_LOAD_ICON_SIZE_SMALL)
        {
            width = PhGetSystemMetrics(SM_CXSMICON, SystemDpi);
            height = PhGetSystemMetrics(SM_CYSMICON, SystemDpi);
        }
        else
        {
            width = PhGetSystemMetrics(SM_CXICON, SystemDpi);
            height = PhGetSystemMetrics(SM_CYICON, SystemDpi);
        }

        icon = LoadImage(ImageBaseAddress, Name, IMAGE_ICON, width, height, 0);
    }

    if (Flags & PH_LOAD_ICON_SHARED)
    {
        if (icon)
        {
            if (!IS_INTRESOURCE(Name))
                entry.Name = PhDuplicateStringZ(Name);
            entry.Icon = icon;
            PhAddEntryHashtable(SharedIconCacheHashtable, &entry);
        }

        PhReleaseQueuedLockExclusive(&SharedIconCacheLock);
    }

    return icon;
}

/**
 * Gets the default icon used for executable files.
 *
 * \param SmallIcon A variable which receives the small default executable icon. Do not destroy the
 * icon using DestroyIcon(); it is shared between callers.
 * \param LargeIcon A variable which receives the large default executable icon. Do not destroy the
 * icon using DestroyIcon(); it is shared between callers.
 */
VOID PhGetStockApplicationIcon(
    _Out_opt_ HICON *SmallIcon,
    _Out_opt_ HICON *LargeIcon
    )
{
    static HICON smallIcon = NULL;
    static HICON largeIcon = NULL;
    static LONG systemDpi = 0;

    if (systemDpi != PhSystemDpi)
    {
        if (smallIcon)
        {
            DestroyIcon(smallIcon);
            smallIcon = NULL;
        }
        if (largeIcon)
        {
            DestroyIcon(largeIcon);
            largeIcon = NULL;
        }

        systemDpi = PhSystemDpi;
    }

    // This no longer uses SHGetFileInfo because it is *very* slow and causes many other DLLs to be
    // loaded, increasing memory usage. The worst thing about it, however, is that it is horribly
    // incompatible with multi-threading. The first time it is called, it tries to perform some
    // one-time initialization. It guards this with a lock, but when multiple threads try to call
    // the function at the same time, instead of waiting for initialization to finish it simply
    // fails the other threads.

    if (!smallIcon || !largeIcon)
    {
        if (WindowsVersion < WINDOWS_10)
        {
            PPH_STRING systemDirectory;
            PPH_STRING dllFileName;

            // imageres,11 (Windows 10 and above), user32,0 (Vista and above) or shell32,2 (XP) contains
            // the default application icon.

            if (systemDirectory = PhGetSystemDirectory())
            {
                dllFileName = PhConcatStringRefZ(&systemDirectory->sr, L"\\user32.dll");

                PhExtractIcon(
                    dllFileName->Buffer,
                    &largeIcon,
                    &smallIcon
                    );

                PhDereferenceObject(dllFileName);
                PhDereferenceObject(systemDirectory);
            }
        }
        else
        {
            static CONST PH_STRINGREF imageFileName = PH_STRINGREF_INIT(L"\\SystemRoot\\System32\\imageres.dll");

            PhExtractIconEx(
                &imageFileName,
                TRUE,
                11,
                PhGetSystemMetrics(SM_CXICON, systemDpi),
                PhGetSystemMetrics(SM_CYICON, systemDpi),
                PhGetSystemMetrics(SM_CXSMICON, systemDpi),
                PhGetSystemMetrics(SM_CYSMICON, systemDpi),
                &largeIcon,
                &smallIcon
                );
        }
    }

    if (!smallIcon)
        smallIcon = PhLoadIcon(NULL, IDI_APPLICATION, PH_LOAD_ICON_SIZE_SMALL, 0, 0, systemDpi);
    if (!largeIcon)
        largeIcon = PhLoadIcon(NULL, IDI_APPLICATION, PH_LOAD_ICON_SIZE_LARGE, 0, 0, systemDpi);

    if (SmallIcon)
        *SmallIcon = smallIcon;
    if (LargeIcon)
        *LargeIcon = largeIcon;
}

//HICON PhGetFileShellIcon(
//    _In_opt_ PWSTR FileName,
//    _In_opt_ PWSTR DefaultExtension,
//    _In_ BOOLEAN LargeIcon
//    )
//{
//    SHFILEINFO fileInfo;
//    ULONG iconFlag;
//    HICON icon;
//
//    if (DefaultExtension && PhEqualStringZ(DefaultExtension, L".exe", TRUE))
//    {
//        // Special case for executable files (see above for reasoning).
//
//        icon = NULL;
//
//        if (FileName)
//        {
//            PhExtractIcon(
//                FileName,
//                LargeIcon ? &icon : NULL,
//                !LargeIcon ? &icon : NULL
//                );
//        }
//
//        if (!icon)
//        {
//            PhGetStockApplicationIcon(
//                !LargeIcon ? &icon : NULL,
//                LargeIcon ? &icon : NULL
//                );
//
//            if (icon)
//                icon = CopyIcon(icon);
//        }
//
//        return icon;
//    }
//
//    iconFlag = LargeIcon ? SHGFI_LARGEICON : SHGFI_SMALLICON;
//    icon = NULL;
//    memset(&fileInfo, 0, sizeof(SHFILEINFO));
//
//    if (FileName && SHGetFileInfo(
//        FileName,
//        0,
//        &fileInfo,
//        sizeof(SHFILEINFO),
//        SHGFI_ICON | iconFlag
//        ))
//    {
//        icon = fileInfo.hIcon;
//    }
//
//    if (!icon && DefaultExtension)
//    {
//        memset(&fileInfo, 0, sizeof(SHFILEINFO));
//
//        if (SHGetFileInfo(
//            DefaultExtension,
//            FILE_ATTRIBUTE_NORMAL,
//            &fileInfo,
//            sizeof(SHFILEINFO),
//            SHGFI_ICON | iconFlag | SHGFI_USEFILEATTRIBUTES
//            ))
//            icon = fileInfo.hIcon;
//    }
//
//    return icon;
//}

VOID PhpSetClipboardData(
    _In_ HWND WindowHandle,
    _In_ ULONG Format,
    _In_ HANDLE Data
    )
{
    if (OpenClipboard(WindowHandle))
    {
        if (!EmptyClipboard())
            goto Fail;

        if (!SetClipboardData(Format, Data))
            goto Fail;

        CloseClipboard();

        return;
    }

Fail:
    GlobalFree(Data);
}

VOID PhSetClipboardString(
    _In_ HWND WindowHandle,
    _In_ PCPH_STRINGREF String
    )
{
    HANDLE data;
    PVOID memory;

    data = GlobalAlloc(GMEM_MOVEABLE, String->Length + sizeof(UNICODE_NULL));
    memory = GlobalLock(data);

    memcpy(memory, String->Buffer, String->Length);
    *(PWCHAR)PTR_ADD_OFFSET(memory, String->Length) = UNICODE_NULL;

    GlobalUnlock(memory);

    PhpSetClipboardData(WindowHandle, CF_UNICODETEXT, data);
}

PPH_STRING PhGetClipboardString(
    _In_ HWND WindowHandle
    )
{
    PPH_STRING string;
    HGLOBAL data;

    string = PhReferenceEmptyString();

    if (!IsClipboardFormatAvailable(CF_UNICODETEXT))
        return string;

    if (!OpenClipboard(WindowHandle))
        return string;

    data = GetClipboardData(CF_UNICODETEXT);
    if (data)
    {
        PVOID str;

        str = GlobalLock(data);
        if (str)
        {
            PhMoveReference(&string, PhCreateString(str));
            GlobalUnlock(data);
        }
    }

    CloseClipboard();

    return string;
}

HWND PhCreateDialogFromTemplate(
    _In_ HWND Parent,
    _In_ ULONG Style,
    _In_ PVOID Instance,
    _In_ PCWSTR Template,
    _In_ DLGPROC DialogProc,
    _In_ PVOID Parameter
    )
{
    PDLGTEMPLATEEX dialogTemplate;
    HWND dialogHandle;

    if (!NT_SUCCESS(PhLoadResourceCopy(Instance, Template, RT_DIALOG, NULL, &dialogTemplate)))
        return NULL;

    if (dialogTemplate->signature == USHRT_MAX)
    {
        dialogTemplate->style = Style;
    }
    else
    {
        ((DLGTEMPLATE *)dialogTemplate)->style = Style;
    }

    dialogHandle = CreateDialogIndirectParam(
        Instance,
        (DLGTEMPLATE *)dialogTemplate,
        Parent,
        DialogProc,
        (LPARAM)Parameter
        );

    PhFree(dialogTemplate);

    return dialogHandle;
}

HWND PhCreateDialog(
    _In_ PVOID Instance,
    _In_ PCWSTR Template,
    _In_opt_ HWND ParentWindow,
    _In_ DLGPROC DialogProc,
    _In_opt_ PVOID Parameter
    )
{
    PDLGTEMPLATEEX dialogTemplate;
    HWND dialogHandle;

    if (!NT_SUCCESS(PhLoadResource(Instance, Template, RT_DIALOG, NULL, &dialogTemplate)))
        return NULL;

    dialogHandle = CreateDialogIndirectParam(
        Instance,
        (LPDLGTEMPLATE)dialogTemplate,
        ParentWindow,
        DialogProc,
        (LPARAM)Parameter
        );

    return dialogHandle;
}

HWND PhCreateWindowEx(
    _In_ PCWSTR ClassName,
    _In_opt_ PCWSTR WindowName,
    _In_ ULONG Style,
    _In_ ULONG ExStyle,
    _In_ LONG X,
    _In_ LONG Y,
    _In_ LONG Width,
    _In_ LONG Height,
    _In_opt_ HWND ParentWindow,
    _In_opt_ HMENU MenuHandle,
    _In_opt_ PVOID InstanceHandle,
    _In_opt_ PVOID Parameter
    )
{
    HWND windowHandle;

    windowHandle = CreateWindowEx(
        ExStyle,
        ClassName,
        WindowName,
        Style,
        X,
        Y,
        Width,
        Height,
        ParentWindow,
        MenuHandle,
        InstanceHandle,
        Parameter
        );

    return windowHandle;
}

HWND PhCreateMessageWindow(
    VOID
    )
{
    HWND windowHandle;

    windowHandle = CreateWindowEx(
        0,
        L"Message",
        NULL,
        0,
        0, 0, 0, 0,
        HWND_MESSAGE,
        NULL,
        NULL,
        NULL
        );

    return windowHandle;
}

INT_PTR PhDialogBox(
    _In_ PVOID Instance,
    _In_ PCWSTR Template,
    _In_opt_ HWND ParentWindow,
    _In_ DLGPROC DialogProc,
    _In_opt_ PVOID Parameter
    )
{
    PDLGTEMPLATEEX dialogTemplate;
    INT_PTR dialogResult;

    if (!NT_SUCCESS(PhLoadResource(Instance, Template, RT_DIALOG, NULL, &dialogTemplate)))
        return INT_ERROR;

    dialogResult = DialogBoxIndirectParam(
        Instance,
        (LPDLGTEMPLATE)dialogTemplate,
        ParentWindow,
        DialogProc,
        (LPARAM)Parameter
        );

    return dialogResult;
}

// rev from LoadMenuW
HMENU PhLoadMenu(
    _In_ PVOID DllBase,
    _In_ PCWSTR MenuName
    )
{
    LPMENUTEMPLATE templateBuffer;

    if (NT_SUCCESS(PhLoadResource(
        DllBase,
        MenuName,
        RT_MENU,
        NULL,
        &templateBuffer
        )))
    {
        return LoadMenuIndirect(templateBuffer);
    }

    return NULL;
}

LRESULT CALLBACK PhpGeneralPropSheetWndProc(
    _In_ HWND hwnd,
    _In_ UINT uMsg,
    _In_ WPARAM wParam,
    _In_ LPARAM lParam
    )
{
    WNDPROC oldWndProc;

    oldWndProc = PhGetWindowContext(hwnd, 0xF);

    if (!oldWndProc)
        return 0;

    switch (uMsg)
    {
    case WM_NCDESTROY:
        {
            PhRemoveWindowContext(hwnd, 0xF);
            PhSetWindowProcedure(hwnd, oldWndProc);
        }
        break;
    case WM_SYSCOMMAND:
        {
            switch (wParam & 0xFFF0)
            {
            case SC_CLOSE:
                {
                    PostMessage(hwnd, WM_CLOSE, 0, 0);
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
                // Prevent the OK button from working (even though
                // it's already hidden). This prevents the Enter
                // key from closing the dialog box.
                return 0;
            }
        }
        break;
    case WM_KEYDOWN: // forward key messages
        {
            HWND pageWindowHandle;

            if (pageWindowHandle = PropSheet_GetCurrentPageHwnd(hwnd))
            {
                if (SendMessage(pageWindowHandle, uMsg, wParam, lParam))
                {
                    return TRUE;
                }
            }
        }
        break;
    }

    return CallWindowProc(oldWndProc, hwnd, uMsg, wParam, lParam);
}

INT CALLBACK PhpGeneralPropSheetProc(
    _In_ HWND hwndDlg,
    _In_ UINT uMsg,
    _In_ LPARAM lParam
    )
{
    switch (uMsg)
    {
    case PSCB_INITIALIZED:
        {
            PhSetWindowContext(hwndDlg, 0xF, (PVOID)PhGetWindowProcedure(hwndDlg));
            PhSetWindowProcedure(hwndDlg, PhpGeneralPropSheetWndProc);

            // Hide the OK button.
            ShowWindow(GetDlgItem(hwndDlg, IDOK), SW_HIDE);
            // Set the Cancel button's text to "Close".
            PhSetDialogItemText(hwndDlg, IDCANCEL, L"Close");
        }
        break;
    }

    return 0;
}

BOOLEAN PhModalPropertySheet(
    _Inout_ PROPSHEETHEADER *Header
    )
{
    // PropertySheet incorrectly discards WM_QUIT messages in certain cases, so we will use our own
    // message loop. An example of this is when GetMessage (called by PropertySheet's message loop)
    // dispatches a message directly from kernel-mode that causes the property sheet to close. In
    // that case PropertySheet will retrieve the WM_QUIT message but will ignore it because of its
    // buggy logic.

    // This is also a good opportunity to introduce an auto-pool.

    PH_AUTO_POOL autoPool;
    HWND oldFocus;
    HWND topLevelOwner;
    HWND hwnd;
    BOOL result;
    MSG message;

    PhInitializeAutoPool(&autoPool);

    oldFocus = GetFocus();
    topLevelOwner = Header->hwndParent;

    while (topLevelOwner && (GetWindowLongPtr(topLevelOwner, GWL_STYLE) & WS_CHILD))
        topLevelOwner = GetParent(topLevelOwner);

    if (topLevelOwner && (topLevelOwner == GetDesktopWindow() || EnableWindow(topLevelOwner, FALSE)))
        topLevelOwner = NULL;

    Header->dwFlags |= PSH_MODELESS;
    // Allow to close other modeless property sheets (ex. Handle properties) by clicking the X on
    // the taskbar window thumbnail, also forward key messages (Dart Vanya)
    if (!Header->pfnCallback)
    {
        Header->dwFlags |= PSH_USECALLBACK;
        Header->pfnCallback = PhpGeneralPropSheetProc;
    }
    hwnd = (HWND)PropertySheet(Header);

    if (!hwnd)
    {
        if (topLevelOwner)
            EnableWindow(topLevelOwner, TRUE);

        return FALSE;
    }

    while (result = GetMessage(&message, NULL, 0, 0))
    {
        BOOLEAN processed = FALSE;

        if (result == INT_ERROR)
            break;

        if (!processed)
        {
            if (!PropSheet_IsDialogMessage(hwnd, &message))
            {
                TranslateMessage(&message);
                DispatchMessage(&message);
            }
        }

        PhDrainAutoPool(&autoPool);

        // Destroy the window when necessary.
        if (!PropSheet_GetCurrentPageHwnd(hwnd))
            break;
    }

    if (result == 0)
        PostQuitMessage((INT)message.wParam);
    if (Header->hwndParent && GetActiveWindow() == hwnd)
        SetActiveWindow(Header->hwndParent);
    if (topLevelOwner)
        EnableWindow(topLevelOwner, TRUE);
    if (oldFocus && IsWindow(oldFocus))
        SetFocus(oldFocus);

    DestroyWindow(hwnd);
    PhDeleteAutoPool(&autoPool);

    return TRUE;
}

VOID PhInitializeLayoutManager(
    _Out_ PPH_LAYOUT_MANAGER Manager,
    _In_ HWND RootWindowHandle
    )
{
    RECT rect;
    LONG dpiValue;

    dpiValue = PhGetWindowDpi(RootWindowHandle);

    PhGetClientRect(RootWindowHandle, &rect);

    PhGetSizeDpiValue(&rect, dpiValue, FALSE);

    Manager->List = PhCreateList(4);

    Manager->dpiValue = dpiValue;
    Manager->LayoutNumber = 0;

    Manager->RootItem.Handle = RootWindowHandle;
    Manager->RootItem.Rect = rect;
    Manager->RootItem.ParentItem = NULL;
    Manager->RootItem.LayoutParentItem = NULL;
    Manager->RootItem.LayoutNumber = 0;
    Manager->RootItem.NumberOfChildren = 0;
    Manager->RootItem.DeferHandle = NULL;
}

VOID PhDeleteLayoutManager(
    _Inout_ PPH_LAYOUT_MANAGER Manager
    )
{
    ULONG i;

    for (i = 0; i < Manager->List->Count; i++)
        PhFree(Manager->List->Items[i]);

    PhDereferenceObject(Manager->List);
}

// HACK: The math below is all horribly broken, especially the HACK for multiline tab controls.

PPH_LAYOUT_ITEM PhAddLayoutItem(
    _Inout_ PPH_LAYOUT_MANAGER Manager,
    _In_ HWND Handle,
    _In_opt_ PPH_LAYOUT_ITEM ParentItem,
    _In_ ULONG Anchor
    )
{
    PPH_LAYOUT_ITEM layoutItem;
    RECT dummy = { 0 };

    layoutItem = PhAddLayoutItemEx(
        Manager,
        Handle,
        ParentItem,
        Anchor,
        dummy
        );

    layoutItem->Margin = layoutItem->Rect;
    PhConvertRect(&layoutItem->Margin, &layoutItem->ParentItem->Rect);

    if (layoutItem->ParentItem != layoutItem->LayoutParentItem)
    {
        // Fix the margin because the item has a dummy parent. They share the same layout parent
        // item.
        layoutItem->Margin.top -= layoutItem->ParentItem->Rect.top;
        layoutItem->Margin.left -= layoutItem->ParentItem->Rect.left;
        layoutItem->Margin.right = layoutItem->ParentItem->Margin.right;
        layoutItem->Margin.bottom = layoutItem->ParentItem->Margin.bottom;
    }

    return layoutItem;
}

PPH_LAYOUT_ITEM PhAddLayoutItemEx(
    _Inout_ PPH_LAYOUT_MANAGER Manager,
    _In_ HWND Handle,
    _In_opt_ PPH_LAYOUT_ITEM ParentItem,
    _In_ ULONG Anchor,
    _In_ RECT Margin
    )
{
    PPH_LAYOUT_ITEM item;

    if (!ParentItem)
        ParentItem = &Manager->RootItem;

    item = PhAllocate(sizeof(PH_LAYOUT_ITEM));
    item->Handle = Handle;
    item->ParentItem = ParentItem;
    item->LayoutNumber = Manager->LayoutNumber;
    item->NumberOfChildren = 0;
    item->DeferHandle = NULL;
    item->Anchor = Anchor;

    item->LayoutParentItem = item->ParentItem;

    while ((item->LayoutParentItem->Anchor & PH_LAYOUT_DUMMY_MASK) &&
        item->LayoutParentItem->LayoutParentItem)
    {
        item->LayoutParentItem = item->LayoutParentItem->LayoutParentItem;
    }

    item->LayoutParentItem->NumberOfChildren++;

    GetWindowRect(Handle, &item->Rect);
    MapWindowRect(HWND_DESKTOP, item->LayoutParentItem->Handle, &item->Rect);

    if (item->Anchor & PH_LAYOUT_TAB_CONTROL)
    {
        // We want to convert the tab control rectangle to the tab page display rectangle.
        TabCtrl_AdjustRect(Handle, FALSE, &item->Rect);
    }

    PhGetSizeDpiValue(&item->Rect, Manager->dpiValue, FALSE);

    item->Margin = Margin;
    PhGetSizeDpiValue(&item->Margin, Manager->dpiValue, FALSE);

    PhAddItemList(Manager->List, item);

    return item;
}

VOID PhpLayoutItemLayout(
    _Inout_ PPH_LAYOUT_MANAGER Manager,
    _Inout_ PPH_LAYOUT_ITEM Item
    )
{
    RECT margin;
    RECT rect;
    ULONG diff;
    BOOLEAN hasDummyParent;

    if (Item->NumberOfChildren > 0 && !Item->DeferHandle)
        Item->DeferHandle = BeginDeferWindowPos(Item->NumberOfChildren);

    if (Item->LayoutNumber == Manager->LayoutNumber)
        return;

    // If this is the root item we must stop here.
    if (!Item->ParentItem)
        return;

    PhpLayoutItemLayout(Manager, Item->ParentItem);

    if (Item->ParentItem != Item->LayoutParentItem)
    {
        PhpLayoutItemLayout(Manager, Item->LayoutParentItem);
        hasDummyParent = TRUE;
    }
    else
    {
        hasDummyParent = FALSE;
    }

    GetWindowRect(Item->Handle, &Item->Rect);
    MapWindowRect(HWND_DESKTOP, Item->LayoutParentItem->Handle, &Item->Rect);

    if (Item->Anchor & PH_LAYOUT_TAB_CONTROL)
    {
        // We want to convert the tab control rectangle to the tab page display rectangle.
        TabCtrl_AdjustRect(Item->Handle, FALSE, &Item->Rect);
    }

    PhGetSizeDpiValue(&Item->Rect, Manager->dpiValue, FALSE);

    if (!(Item->Anchor & PH_LAYOUT_DUMMY_MASK))
    {
        margin = Item->Margin;
        rect = Item->Rect;

        // Convert right/bottom into margins to make the calculations
        // easier.
        PhConvertRect(&rect, &Item->LayoutParentItem->Rect);

        if (!(Item->Anchor & (PH_ANCHOR_LEFT | PH_ANCHOR_RIGHT)))
        {
            // TODO
            PhRaiseStatus(STATUS_NOT_IMPLEMENTED);
        }
        else if (Item->Anchor & PH_ANCHOR_RIGHT)
        {
            if (Item->Anchor & PH_ANCHOR_LEFT)
            {
                rect.left = (hasDummyParent ? Item->ParentItem->Rect.left : 0) + margin.left;
                rect.right = margin.right;
            }
            else
            {
                diff = margin.right - rect.right;

                rect.left -= diff;
                rect.right += diff;
            }
        }

        if (!(Item->Anchor & (PH_ANCHOR_TOP | PH_ANCHOR_BOTTOM)))
        {
            // TODO
            PhRaiseStatus(STATUS_NOT_IMPLEMENTED);
        }
        else if (Item->Anchor & PH_ANCHOR_BOTTOM)
        {
            if (Item->Anchor & PH_ANCHOR_TOP)
            {
                // tab control hack
                rect.top = (hasDummyParent ? Item->ParentItem->Rect.top : 0) + margin.top;
                rect.bottom = margin.bottom;
            }
            else
            {
                diff = margin.bottom - rect.bottom;

                rect.top -= diff;
                rect.bottom += diff;
            }
        }

        // Convert the right/bottom back into co-ordinates.
        PhConvertRect(&rect, &Item->LayoutParentItem->Rect);
        Item->Rect = rect;
        PhGetSizeDpiValue(&rect, Manager->dpiValue, TRUE);

        if (!(Item->Anchor & PH_LAYOUT_IMMEDIATE_RESIZE))
        {
            Item->LayoutParentItem->DeferHandle = DeferWindowPos(
                Item->LayoutParentItem->DeferHandle, Item->Handle,
                NULL, rect.left, rect.top,
                rect.right - rect.left, rect.bottom - rect.top,
                SWP_NOACTIVATE | SWP_NOZORDER
                );
        }
        else
        {
            // This is needed for tab controls, so that TabCtrl_AdjustRect will give us an
            // up-to-date result.
            SetWindowPos(
                Item->Handle,
                NULL, rect.left, rect.top,
                rect.right - rect.left, rect.bottom - rect.top,
                SWP_NOACTIVATE | SWP_NOZORDER
                );
        }
    }

    Item->LayoutNumber = Manager->LayoutNumber;
}

VOID PhLayoutManagerLayout(
    _Inout_ PPH_LAYOUT_MANAGER Manager
    )
{
    PPH_LAYOUT_ITEM item;
    LONG dpiValue;
    ULONG i;

    Manager->LayoutNumber++;

    dpiValue = PhGetWindowDpi(Manager->RootItem.Handle);
    Manager->dpiValue = dpiValue;

    if (!PhGetClientRect(Manager->RootItem.Handle, &Manager->RootItem.Rect))
        return;

    PhGetSizeDpiValue(&Manager->RootItem.Rect, dpiValue, FALSE);

    for (i = 0; i < Manager->List->Count; i++)
    {
        item = (PPH_LAYOUT_ITEM)Manager->List->Items[i];

        PhpLayoutItemLayout(Manager, item);
    }

    for (i = 0; i < Manager->List->Count; i++)
    {
        item = (PPH_LAYOUT_ITEM)Manager->List->Items[i];

        if (item->DeferHandle)
        {
            EndDeferWindowPos(item->DeferHandle);
            item->DeferHandle = NULL;
        }

        if (item->Anchor & PH_LAYOUT_FORCE_INVALIDATE)
        {
            InvalidateRect(item->Handle, NULL, FALSE);
        }
    }

    if (Manager->RootItem.DeferHandle)
    {
        EndDeferWindowPos(Manager->RootItem.DeferHandle);
        Manager->RootItem.DeferHandle = NULL;
    }
}

BOOLEAN NTAPI PhpWindowContextHashtableEqualFunction(
    _In_ PVOID Entry1,
    _In_ PVOID Entry2
    )
{
    PPH_WINDOW_PROPERTY_CONTEXT entry1 = Entry1;
    PPH_WINDOW_PROPERTY_CONTEXT entry2 = Entry2;

    return
        entry1->WindowHandle == entry2->WindowHandle &&
        entry1->PropertyHash == entry2->PropertyHash;
}

ULONG NTAPI PhpWindowContextHashtableHashFunction(
    _In_ PVOID Entry
    )
{
    PPH_WINDOW_PROPERTY_CONTEXT entry = Entry;

    return PhHashIntPtr((ULONG_PTR)entry->WindowHandle) ^ entry->PropertyHash; // PhHashInt32
}

PVOID PhGetWindowContext(
    _In_ HWND WindowHandle,
    _In_ ULONG PropertyHash
    )
{
    PH_WINDOW_PROPERTY_CONTEXT lookupEntry;
    PPH_WINDOW_PROPERTY_CONTEXT entry;

    lookupEntry.WindowHandle = WindowHandle;
    lookupEntry.PropertyHash = PropertyHash;

    PhAcquireQueuedLockShared(&WindowContextListLock);
    entry = PhFindEntryHashtable(WindowContextHashTable, &lookupEntry);
    PhReleaseQueuedLockShared(&WindowContextListLock);

    if (entry)
        return entry->Context;
    else
        return NULL;
}

VOID PhSetWindowContext(
    _In_ HWND WindowHandle,
    _In_ ULONG PropertyHash,
    _In_ PVOID Context
    )
{
    PH_WINDOW_PROPERTY_CONTEXT entry;

    entry.WindowHandle = WindowHandle;
    entry.PropertyHash = PropertyHash;
    entry.Context = Context;

    PhAcquireQueuedLockExclusive(&WindowContextListLock);
    PhAddEntryHashtable(WindowContextHashTable, &entry);
    PhReleaseQueuedLockExclusive(&WindowContextListLock);
}

VOID PhRemoveWindowContext(
    _In_ HWND WindowHandle,
    _In_ ULONG PropertyHash
    )
{
    PH_WINDOW_PROPERTY_CONTEXT lookupEntry;

    lookupEntry.WindowHandle = WindowHandle;
    lookupEntry.PropertyHash = PropertyHash;

    PhAcquireQueuedLockExclusive(&WindowContextListLock);
    PhRemoveEntryHashtable(WindowContextHashTable, &lookupEntry);
    PhReleaseQueuedLockExclusive(&WindowContextListLock);
}

VOID PhEnumWindows(
    _In_ PH_ENUM_CALLBACK Callback,
    _In_opt_ PVOID Context
    )
{
    EnumWindows((WNDENUMPROC)Callback, (LPARAM)Context);
}

/**
 * Enumerates the child windows of the specified window handle.
 *
 * \param WindowHandle The handle of the parent window.
 * \param Limit The maximum number of child windows to enumerate.
 * \param Callback The callback function to be called for each child window.
 * \param Context An optional context parameter to be passed to the callback function.
 */
VOID PhEnumChildWindows(
    _In_opt_ HWND WindowHandle,
    _In_ ULONG Limit,
    _In_ PH_CHILD_ENUM_CALLBACK Callback,
    _In_opt_ PVOID Context
    )
{
    EnumChildWindows(WindowHandle, (WNDENUMPROC)Callback, (LPARAM)Context);

    //HWND childWindow = NULL;
    //ULONG i = 0;
    //
    //while (i < Limit && (childWindow = FindWindowEx(WindowHandle, childWindow, NULL, NULL)))
    //{
    //    if (!Callback(childWindow, Context))
    //        return;
    //
    //    i++;
    //}
}

typedef struct _GET_PROCESS_MAIN_WINDOW_CONTEXT
{
    HWND Window;
    HWND ImmersiveWindow;
    HANDLE ProcessId;
    BOOLEAN IsImmersive;
    BOOLEAN SkipInvisible;
} GET_PROCESS_MAIN_WINDOW_CONTEXT, *PGET_PROCESS_MAIN_WINDOW_CONTEXT;

BOOL CALLBACK PhpGetProcessMainWindowEnumWindowsProc(
    _In_ HWND WindowHandle,
    _In_ PVOID Context
    )
{
    PGET_PROCESS_MAIN_WINDOW_CONTEXT context = (PGET_PROCESS_MAIN_WINDOW_CONTEXT)Context;
    ULONG processId;
    WINDOWINFO windowInfo;

    if (context->SkipInvisible && !IsWindowVisible(WindowHandle))
        return TRUE;

    GetWindowThreadProcessId(WindowHandle, &processId);

    //if (UlongToHandle(processId) == context->ProcessId && (context->SkipInvisible ?
    //    !((parentWindow = GetParent(WindowHandle)) && IsWindowVisible(parentWindow)) && // skip windows with a visible parent
    //    PhGetWindowTextEx(WindowHandle, PH_GET_WINDOW_TEXT_INTERNAL | PH_GET_WINDOW_TEXT_LENGTH_ONLY, NULL) != 0 : TRUE)) // skip windows with no title

    if (UlongToHandle(processId) == context->ProcessId)
    {
        if (!context->ImmersiveWindow && context->IsImmersive &&
            GetProp(WindowHandle, L"Windows.ImmersiveShell.IdentifyAsMainCoreWindow"))
        {
            context->ImmersiveWindow = WindowHandle;
        }

        windowInfo.cbSize = sizeof(WINDOWINFO);

        if (!context->Window && GetWindowInfo(WindowHandle, &windowInfo) && (windowInfo.dwStyle & WS_DLGFRAME))
        {
            context->Window = WindowHandle;

            // If we're not looking at an immersive process, there's no need to search any more windows.
            if (!context->IsImmersive)
                return FALSE;
        }
    }

    return TRUE;
}

HWND PhGetProcessMainWindow(
    _In_opt_ HANDLE ProcessId,
    _In_opt_ HANDLE ProcessHandle
    )
{
    return PhGetProcessMainWindowEx(ProcessId, ProcessHandle, TRUE);
}

HWND PhGetProcessMainWindowEx(
    _In_opt_ HANDLE ProcessId,
    _In_opt_ HANDLE ProcessHandle,
    _In_ BOOLEAN SkipInvisible
    )
{
    GET_PROCESS_MAIN_WINDOW_CONTEXT context;
    HANDLE processHandle = NULL;

    memset(&context, 0, sizeof(GET_PROCESS_MAIN_WINDOW_CONTEXT));
    context.ProcessId = ProcessId;
    context.SkipInvisible = SkipInvisible;

    if (ProcessHandle)
        processHandle = ProcessHandle;
    else
        PhOpenProcess(&processHandle, PROCESS_QUERY_LIMITED_INFORMATION, ProcessId);

    if (processHandle && WindowsVersion >= WINDOWS_8)
        context.IsImmersive = PhIsImmersiveProcess(processHandle);

    PhEnumWindows(PhpGetProcessMainWindowEnumWindowsProc, &context);
    //PhEnumChildWindows(NULL, 0x800, PhpGetProcessMainWindowEnumWindowsProc, &context);

    if (!ProcessHandle && processHandle)
        NtClose(processHandle);

    return context.ImmersiveWindow ? context.ImmersiveWindow : context.Window;
}

ULONG PhGetDialogItemValue(
    _In_ HWND WindowHandle,
    _In_ LONG ControlID
    )
{
    ULONG64 controlValue = 0;
    HWND controlHandle;
    PPH_STRING controlText;

    if (controlHandle = GetDlgItem(WindowHandle, ControlID))
    {
        if (controlText = PhGetWindowText(controlHandle))
        {
            PhStringToInteger64(&controlText->sr, 10, &controlValue);
            PhDereferenceObject(controlText);
        }
    }

    return (ULONG)controlValue;
}

VOID PhSetDialogItemValue(
    _In_ HWND WindowHandle,
    _In_ LONG ControlID,
    _In_ ULONG Value,
    _In_ BOOLEAN Signed
    )
{
    HWND controlHandle;
    WCHAR valueString[PH_INT32_STR_LEN_1];

    if (Signed)
        PhPrintInt32(valueString, (LONG)Value);
    else
        PhPrintUInt32(valueString, Value);

    if (controlHandle = GetDlgItem(WindowHandle, ControlID))
    {
        PhSetWindowText(controlHandle, valueString);
    }
}

VOID PhSetDialogItemText(
    _In_ HWND WindowHandle,
    _In_ LONG ControlID,
    _In_ PCWSTR WindowText
    )
{
    HWND controlHandle;

    if (controlHandle = GetDlgItem(WindowHandle, ControlID))
    {
        PhSetWindowText(controlHandle, WindowText);
    }
}

VOID PhSetWindowText(
    _In_ HWND WindowHandle,
    _In_ PCWSTR WindowText
    )
{
    SendMessage(WindowHandle, WM_SETTEXT, 0, (LPARAM)WindowText); // TODO: DefWindowProc (dmex)
}

VOID PhSetGroupBoxText(
    _In_ HWND WindowHandle,
    _In_ PCWSTR WindowText
    )
{
    // Suspend the groupbox when setting the text otherwise
    // the groupbox doesn't paint the text with dark theme colors. (dmex)

    SendMessage(WindowHandle, WM_SETREDRAW, FALSE, 0);
    PhSetWindowText(WindowHandle, WindowText);
    SendMessage(WindowHandle, WM_SETREDRAW, TRUE, 0);

    InvalidateRect(WindowHandle, NULL, FALSE);
}

VOID PhSetWindowAlwaysOnTop(
    _In_ HWND WindowHandle,
    _In_ BOOLEAN AlwaysOnTop
    )
{
    SetFocus(WindowHandle); // HACK - SetWindowPos doesn't work properly without this (wj32)
    SetWindowPos(
        WindowHandle,
        AlwaysOnTop ? HWND_TOPMOST : HWND_NOTOPMOST,
        0, 0, 0, 0,
        SWP_NOACTIVATE | SWP_NOMOVE | SWP_NOSIZE
        );
}

BOOLEAN NTAPI PhpWindowCallbackHashtableEqualFunction(
    _In_ PVOID Entry1,
    _In_ PVOID Entry2
    )
{
    return
        ((PPH_PLUGIN_WINDOW_CALLBACK_REGISTRATION)Entry1)->WindowHandle ==
        ((PPH_PLUGIN_WINDOW_CALLBACK_REGISTRATION)Entry2)->WindowHandle;
}

ULONG NTAPI PhpWindowCallbackHashtableHashFunction(
    _In_ PVOID Entry
    )
{
    return PhHashIntPtr((ULONG_PTR)((PPH_PLUGIN_WINDOW_CALLBACK_REGISTRATION)Entry)->WindowHandle);
}

VOID PhRegisterWindowCallback(
    _In_ HWND WindowHandle,
    _In_ PH_PLUGIN_WINDOW_EVENT_TYPE Type,
    _In_opt_ PVOID Context
    )
{
    PH_PLUGIN_WINDOW_CALLBACK_REGISTRATION entry;

    entry.WindowHandle = WindowHandle;
    entry.Type = Type;

    switch (Type) // HACK
    {
    case PH_PLUGIN_WINDOW_EVENT_TYPE_TOPMOST:
        if (PhGetIntegerSetting(L"MainWindowAlwaysOnTop"))
            PhSetWindowAlwaysOnTop(WindowHandle, TRUE);
        break;
    }

    PhAcquireQueuedLockExclusive(&WindowCallbackListLock);
    PhAddEntryHashtable(WindowCallbackHashTable, &entry);
    PhReleaseQueuedLockExclusive(&WindowCallbackListLock);
}

VOID PhUnregisterWindowCallback(
    _In_ HWND WindowHandle
    )
{
    PH_PLUGIN_WINDOW_CALLBACK_REGISTRATION lookupEntry;
    PPH_PLUGIN_WINDOW_CALLBACK_REGISTRATION entry;

    lookupEntry.WindowHandle = WindowHandle;

    PhAcquireQueuedLockExclusive(&WindowCallbackListLock);
    entry = PhFindEntryHashtable(WindowCallbackHashTable, &lookupEntry);
    assert(entry);
    PhRemoveEntryHashtable(WindowCallbackHashTable, entry);
    PhReleaseQueuedLockExclusive(&WindowCallbackListLock);
}

VOID PhWindowNotifyTopMostEvent(
    _In_ BOOLEAN TopMost
    )
{
    PPH_PLUGIN_WINDOW_CALLBACK_REGISTRATION entry;
    ULONG i = 0;

    PhAcquireQueuedLockExclusive(&WindowCallbackListLock);

    while (PhEnumHashtable(WindowCallbackHashTable, &entry, &i))
    {
        if (entry->Type & PH_PLUGIN_WINDOW_EVENT_TYPE_TOPMOST)
        {
            PhSetWindowAlwaysOnTop(entry->WindowHandle, TopMost);
        }
    }

    PhReleaseQueuedLockExclusive(&WindowCallbackListLock);
}

/**
 * Retrieves the environment variables for the specified user.
 *
 * @param Environment A pointer to the new environment block. 
 * @param TokenHandle Token to query for user environment variables.
 * If this is a primary token, the token must have TOKEN_QUERY and TOKEN_DUPLICATE access.
 * If the token is an impersonation token, it must have TOKEN_QUERY access.
 * If this parameter is NULL, the returned environment block contains system variables only.
 * @param Inherit Specifies whether to inherit variables from the current process' environment. If this value is TRUE, the process inherits the current process' environment. 
 * @return A pointer to the imported procedure, or NULL if the procedure could not be imported.
 * @remarks User-specific environment variables such as %USERPROFILE% are set only when the user's profile is loaded. To load a user's profile, call the LoadUserProfile function.
 */
NTSTATUS PhCreateEnvironmentBlock(
    _Out_ PVOID* Environment,
    _In_opt_ HANDLE TokenHandle,
    _In_ BOOLEAN Inherit
    )
{
    //#include <UserEnv.h>
    //HANDLE profileHandle;
    //
    //if (TokenHandle)
    //{
    //    PROFILEINFO profileInfo = { sizeof(PROFILEINFO) };
    //    LoadUserProfile(TokenHandle, &profileInfo);
    //    profileHandle = profileInfo.hProfile;
    //}

    *Environment = NULL;

    if (CreateEnvironmentBlock_Import()(Environment, TokenHandle, Inherit))
    {
        return STATUS_SUCCESS;
    }

    //if (TokenHandle && profileHandle)
    //{
    //    UnloadUserProfile(TokenHandle, profileHandle);
    //}

    return PhGetLastWin32ErrorAsNtStatus();
}

/**
 * Frees environment variables created by the CreateEnvironmentBlock function.
 *
 * @param Environment A pointer to the new environment block.
 */
VOID PhDestroyEnvironmentBlock(
    _In_ _Post_invalid_ PVOID Environment
    )
{
    DestroyEnvironmentBlock_Import()(Environment);
}

_Success_(return)
BOOLEAN PhRegenerateUserEnvironment(
    _Out_opt_ PVOID* NewEnvironment,
    _In_ BOOLEAN UpdateCurrentEnvironment
    )
{
    static PH_INITONCE initOnce = PH_INITONCE_INIT;
    static BOOL (WINAPI *RegenerateUserEnvironment_I)(
        _Out_ PVOID* NewEnvironment,
        _In_ BOOL UpdateCurrentEnvironment
        ) = NULL;
    PVOID environment;

    if (PhBeginInitOnce(&initOnce))
    {
        PVOID shell32Handle;

        if (shell32Handle = PhLoadLibrary(L"shell32.dll"))
        {
            RegenerateUserEnvironment_I = PhGetDllBaseProcedureAddress(shell32Handle, "RegenerateUserEnvironment", 0);
        }

        PhEndInitOnce(&initOnce);
    }

    if (!RegenerateUserEnvironment_I)
        return FALSE;

    if (RegenerateUserEnvironment_I(&environment, UpdateCurrentEnvironment))
    {
        if (NewEnvironment)
        {
            *NewEnvironment = environment;
        }
        else
        {
            if (DestroyEnvironmentBlock_Import() && !UpdateCurrentEnvironment)
            {
                DestroyEnvironmentBlock_Import()(environment);
            }
        }

        return TRUE;
    }

    return FALSE;
}

HICON PhGetInternalWindowIcon(
    _In_ HWND WindowHandle,
    _In_ UINT Type
    )
{
    static PH_INITONCE initOnce = PH_INITONCE_INIT;
    static HICON (WINAPI *InternalGetWindowIcon_I)(
        _In_ HWND WindowHandle,
        _In_ ULONG Type
        ) = NULL;

    if (PhBeginInitOnce(&initOnce))
    {
        PVOID shell32Handle;

        if (shell32Handle = PhLoadLibrary(L"shell32.dll"))
        {
            InternalGetWindowIcon_I = PhGetDllBaseProcedureAddress(shell32Handle, "InternalGetWindowIcon", 0);
        }

        PhEndInitOnce(&initOnce);
    }

    if (!InternalGetWindowIcon_I)
        return NULL;

    return InternalGetWindowIcon_I(WindowHandle, Type);
}

BOOLEAN PhIsImmersiveProcess(
    _In_ HANDLE ProcessHandle
    )
{
    static PH_INITONCE initOnce = PH_INITONCE_INIT;
    static __typeof__(&IsImmersiveProcess) IsImmersiveProcess_I = NULL;

    if (PhBeginInitOnce(&initOnce))
    {
        if (WindowsVersion >= WINDOWS_8)
            IsImmersiveProcess_I = PhGetDllProcedureAddress(L"user32.dll", "IsImmersiveProcess", 0);
        PhEndInitOnce(&initOnce);
    }

    if (!IsImmersiveProcess_I)
        return FALSE;

    return !!IsImmersiveProcess_I(ProcessHandle);
}

_Success_(return)
BOOLEAN PhGetProcessUIContextInformation(
    _In_ HANDLE ProcessHandle,
    _Out_ PPROCESS_UICONTEXT_INFORMATION UIContext
    )
{
    static PH_INITONCE initOnce = PH_INITONCE_INIT;
    static BOOL (WINAPI* GetProcessUIContextInformation_I)(
        _In_ HANDLE ProcessHandle,
        _Out_ PPROCESS_UICONTEXT_INFORMATION UIContext
        ) = NULL;

    if (PhBeginInitOnce(&initOnce))
    {
        if (WindowsVersion >= WINDOWS_8)
            GetProcessUIContextInformation_I = PhGetDllProcedureAddress(L"user32.dll", "GetProcessUIContextInformation", 0);
        PhEndInitOnce(&initOnce);
    }

    if (!GetProcessUIContextInformation_I)
        return FALSE;

    return !!GetProcessUIContextInformation_I(ProcessHandle, UIContext);
}

_Success_(return)
BOOLEAN PhGetProcessDpiAwareness(
    _In_ HANDLE ProcessHandle,
    _Out_ PPH_PROCESS_DPI_AWARENESS ProcessDpiAwareness
    )
{
    static PH_INITONCE initOnce = PH_INITONCE_INIT;
    static __typeof__(&GetDpiAwarenessContextForProcess) GetDpiAwarenessContextForProcess_I = NULL;
    static __typeof__(&AreDpiAwarenessContextsEqual) AreDpiAwarenessContextsEqual_I = NULL;
    static BOOL (WINAPI* GetProcessDpiAwarenessInternal_I)(_In_ HANDLE hprocess, _Out_ PROCESS_DPI_AWARENESS* value) = NULL;

    if (PhBeginInitOnce(&initOnce))
    {
        PVOID baseAddress;

        if (baseAddress = PhGetLoaderEntryDllBaseZ(L"user32.dll"))
        {
            if (WindowsVersion >= WINDOWS_10_RS1)
            {
                GetDpiAwarenessContextForProcess_I = PhGetDllBaseProcedureAddress(baseAddress, "GetDpiAwarenessContextForProcess", 0);
                AreDpiAwarenessContextsEqual_I = PhGetDllBaseProcedureAddress(baseAddress, "AreDpiAwarenessContextsEqual", 0);
            }

            if (!(GetDpiAwarenessContextForProcess_I && AreDpiAwarenessContextsEqual_I))
            {
                GetProcessDpiAwarenessInternal_I = PhGetDllBaseProcedureAddress(baseAddress, "GetProcessDpiAwarenessInternal", 0);
            }
        }

        PhEndInitOnce(&initOnce);
    }

    if (GetDpiAwarenessContextForProcess_I && AreDpiAwarenessContextsEqual_I)
    {
        DPI_AWARENESS_CONTEXT context = GetDpiAwarenessContextForProcess_I(ProcessHandle);

        if (AreDpiAwarenessContextsEqual_I(context, DPI_AWARENESS_CONTEXT_UNAWARE))
        {
            *ProcessDpiAwareness = PH_PROCESS_DPI_AWARENESS_UNAWARE;
            return TRUE;
        }

        if (AreDpiAwarenessContextsEqual_I(context, DPI_AWARENESS_CONTEXT_SYSTEM_AWARE))
        {
            *ProcessDpiAwareness = PH_PROCESS_DPI_AWARENESS_SYSTEM_DPI_AWARE;
            return TRUE;
        }

        if (AreDpiAwarenessContextsEqual_I(context, DPI_AWARENESS_CONTEXT_PER_MONITOR_AWARE))
        {
            *ProcessDpiAwareness = PH_PROCESS_DPI_AWARENESS_PER_MONITOR_DPI_AWARE;
            return TRUE;
        }

        if (AreDpiAwarenessContextsEqual_I(context, DPI_AWARENESS_CONTEXT_PER_MONITOR_AWARE_V2))
        {
            *ProcessDpiAwareness = PH_PROCESS_DPI_AWARENESS_PER_MONITOR_AWARE_V2;
            return TRUE;
        }

        if (AreDpiAwarenessContextsEqual_I(context, DPI_AWARENESS_CONTEXT_UNAWARE_GDISCALED))
        {
            *ProcessDpiAwareness = PH_PROCESS_DPI_AWARENESS_UNAWARE_GDISCALED;
            return TRUE;
        }
    }

    if (GetProcessDpiAwarenessInternal_I)
    {
        PROCESS_DPI_AWARENESS dpiAwareness = PROCESS_DPI_UNAWARE;

        if (GetProcessDpiAwarenessInternal_I(ProcessHandle, &dpiAwareness))
        {
            switch (dpiAwareness)
            {
            case PROCESS_DPI_UNAWARE:
                *ProcessDpiAwareness = PH_PROCESS_DPI_AWARENESS_UNAWARE;
                return TRUE;
            case PROCESS_SYSTEM_DPI_AWARE:
                *ProcessDpiAwareness = PH_PROCESS_DPI_AWARENESS_SYSTEM_DPI_AWARE;
                return TRUE;
            case PROCESS_PER_MONITOR_DPI_AWARE:
                *ProcessDpiAwareness = PH_PROCESS_DPI_AWARENESS_PER_MONITOR_DPI_AWARE;
                return TRUE;
            }
        }
    }

    return FALSE;
}

NTSTATUS PhGetPhysicallyInstalledSystemMemory(
    _Out_ PULONGLONG TotalMemory,
    _Out_ PULONGLONG ReservedMemory
    )
{
    static PH_INITONCE initOnce = PH_INITONCE_INIT;
    static __typeof__(&GetPhysicallyInstalledSystemMemory) GetPhysicallyInstalledSystemMemory_I = NULL;
    ULONGLONG physicallyInstalledSystemMemory = 0;

    if (PhBeginInitOnce(&initOnce))
    {
        GetPhysicallyInstalledSystemMemory_I = PhGetDllProcedureAddress(L"kernel32.dll", "GetPhysicallyInstalledSystemMemory", 0);
        PhEndInitOnce(&initOnce);
    }

    if (!GetPhysicallyInstalledSystemMemory_I)
    {
        return STATUS_PROCEDURE_NOT_FOUND;
    }

    if (GetPhysicallyInstalledSystemMemory_I(&physicallyInstalledSystemMemory))
    {
        *TotalMemory = physicallyInstalledSystemMemory * 1024ULL;
        *ReservedMemory = physicallyInstalledSystemMemory * 1024ULL - UInt32x32To64(PhSystemBasicInformation.NumberOfPhysicalPages, PAGE_SIZE);
        return STATUS_SUCCESS;
    }

    *TotalMemory = 0;
    *ReservedMemory = 0;
    return PhGetLastWin32ErrorAsNtStatus();
}

/**
 * Retrieves the GUI resources used by a session.
 *
 * \param Flags The flags to be used for retrieving the GUI resources.
 * \param Total Pointer to a variable that will receive the total number of GUI resources.
 * \return Returns the status code indicating the success or failure of the operation.
 */
NTSTATUS PhGetSessionGuiResources(
    _In_ ULONG Flags,
    _Out_ PULONG Total
    )
{
    return PhGetProcessGuiResources(GR_GLOBAL, Flags, Total);
}

/**
 * Retrieves the GUI resources used by a process.
 *
 * \param ProcessHandle The handle to the process for which to retrieve the GUI resources.
 * \param Flags The flags to be used for retrieving the GUI resources.
 * \param Total Pointer to a variable that will receive the total number of GUI resources.
 * \return Returns the status code indicating the success or failure of the operation.
 */
NTSTATUS PhGetProcessGuiResources(
    _In_ HANDLE ProcessHandle,
    _In_ ULONG Flags,
    _Out_ PULONG Total
    )
{
    if (*Total = GetGuiResources(ProcessHandle, Flags))
        return STATUS_SUCCESS;

    return PhGetLastWin32ErrorAsNtStatus();
}

/**
 * Retrieves information about the active window or a specified GUI thread.
 *
 * \param ThreadId The identifier for the thread for which information is to be retrieved. If this parameter is NULL, the function returns information for the foreground thread.
 * \param ThreadInfo A pointer to a GUITHREADINFO structure that receives information describing the thread.
 * \return Returns the status code indicating the success or failure of the operation.
 */
NTSTATUS PhGetGUIThreadInfo(
    _In_opt_ HANDLE ThreadId,
    _Out_ PGUITHREADINFO ThreadInfo
    )
{
    ThreadInfo->cbSize = sizeof(GUITHREADINFO);

    if (GetGUIThreadInfo(HandleToUlong(ThreadId), ThreadInfo))
    {
        return STATUS_SUCCESS;
    }

    return PhGetLastWin32ErrorAsNtStatus();
}

BOOLEAN PhGetThreadWin32Thread(
    _In_ HANDLE ThreadId
    )
{
    GUITHREADINFO info;

    return NT_SUCCESS(PhGetGUIThreadInfo(ThreadId, &info));
}

NTSTATUS PhGetSendMessageReceiver(
    _In_ HANDLE ThreadId,
    _Out_ HWND *WindowHandle
    )
{
    static PH_INITONCE initOnce = PH_INITONCE_INIT;
    static __typeof__(&GetSendMessageReceiver) GetSendMessageReceiver_I = NULL;
    HWND windowHandle;

    // GetSendMessageReceiver is an undocumented function exported by
    // user32.dll. It retrieves the handle of the window which a thread
    // is sending a message to. (wj32)

    if (PhBeginInitOnce(&initOnce))
    {
        GetSendMessageReceiver_I = PhGetDllProcedureAddress(L"user32.dll", "GetSendMessageReceiver", 0);
        PhEndInitOnce(&initOnce);
    }

    if (!GetSendMessageReceiver_I)
    {
        return STATUS_PROCEDURE_NOT_FOUND;
    }

    if (windowHandle = GetSendMessageReceiver_I(ThreadId))
    {
        *WindowHandle = windowHandle;
        return STATUS_SUCCESS;
    }

    return PhGetLastWin32ErrorAsNtStatus();
}

// rev from ExtractIconExW
_Success_(return)
BOOLEAN PhExtractIcon(
    _In_ PCWSTR FileName,
    _Out_opt_ HICON *IconLarge,
    _Out_opt_ HICON *IconSmall
    )
{
    static PH_INITONCE initOnce = PH_INITONCE_INIT;
    static LONG (WINAPI *PrivateExtractIconExW)(
        _In_ PCWSTR FileName,
        _In_ LONG IconIndex,
        _Out_opt_ HICON* IconLarge,
        _Out_opt_ HICON* IconSmall,
        _In_ LONG IconCount
        ) = NULL;
    HICON iconLarge = NULL;
    HICON iconSmall = NULL;

    if (PhBeginInitOnce(&initOnce))
    {
        PrivateExtractIconExW = PhGetDllProcedureAddress(L"user32.dll", "PrivateExtractIconExW", 0);
        PhEndInitOnce(&initOnce);
    }

    if (!PrivateExtractIconExW)
        return FALSE;

    if (PrivateExtractIconExW(
        FileName,
        0,
        IconLarge ? &iconLarge : NULL,
        IconSmall ? &iconSmall : NULL,
        1
        ) > 0)
    {
        if (IconLarge)
            *IconLarge = iconLarge;
        if (IconSmall)
            *IconSmall = iconSmall;

        return TRUE;
    }

    if (iconLarge)
        DestroyIcon(iconLarge);
    if (iconSmall)
        DestroyIcon(iconSmall);

    return FALSE;
}

NTSTATUS PhLoadIconFromResourceDirectory(
    _In_ PPH_MAPPED_IMAGE MappedImage,
    _In_ PIMAGE_RESOURCE_DIRECTORY ResourceDirectory,
    _In_ LONG ResourceIndex,
    _In_ PCWSTR ResourceType,
    _Out_opt_ ULONG* ResourceLength,
    _Out_opt_ PVOID* ResourceBuffer
    )
{
    ULONG resourceIndex;
    ULONG resourceCount;
    PVOID resourceBuffer;
    PIMAGE_RESOURCE_DIRECTORY nameDirectory;
    PIMAGE_RESOURCE_DIRECTORY languageDirectory;
    PIMAGE_RESOURCE_DIRECTORY_ENTRY resourceType;
    PIMAGE_RESOURCE_DIRECTORY_ENTRY resourceName;
    PIMAGE_RESOURCE_DIRECTORY_ENTRY resourceLanguage;
    PIMAGE_RESOURCE_DATA_ENTRY resourceData;

    // Find the type
    resourceCount = ResourceDirectory->NumberOfIdEntries + ResourceDirectory->NumberOfNamedEntries;
    resourceType = PTR_ADD_OFFSET(ResourceDirectory, sizeof(IMAGE_RESOURCE_DIRECTORY));

    for (resourceIndex = 0; resourceIndex < resourceCount; resourceIndex++)
    {
        if (resourceType[resourceIndex].NameIsString)
            continue;
        if (resourceType[resourceIndex].Name == PtrToUlong(ResourceType))
            break;
    }

    if (resourceIndex == resourceCount)
        return STATUS_RESOURCE_TYPE_NOT_FOUND;
    if (!resourceType[resourceIndex].DataIsDirectory)
        return STATUS_RESOURCE_TYPE_NOT_FOUND;

    // Find the name
    nameDirectory = PTR_ADD_OFFSET(ResourceDirectory, resourceType[resourceIndex].OffsetToDirectory);
    resourceCount = nameDirectory->NumberOfIdEntries + nameDirectory->NumberOfNamedEntries;
    resourceName = PTR_ADD_OFFSET(nameDirectory, sizeof(IMAGE_RESOURCE_DIRECTORY));

    if (ResourceIndex < 0) // RT_ICON and DEVPKEY_DeviceClass_IconPath
    {
        for (resourceIndex = 0; resourceIndex < resourceCount; resourceIndex++)
        {
            if (resourceName[resourceIndex].NameIsString)
                continue;
            if (resourceName[resourceIndex].Name == (ULONG)-ResourceIndex)
                break;
        }
    }
    else // RT_GROUP_ICON
    {
        resourceIndex = ResourceIndex;
    }

    if (resourceIndex >= resourceCount)
        return STATUS_RESOURCE_NAME_NOT_FOUND;
    if (!resourceName[resourceIndex].DataIsDirectory)
        return STATUS_RESOURCE_NAME_NOT_FOUND;

    // Find the language
    languageDirectory = PTR_ADD_OFFSET(ResourceDirectory, resourceName[resourceIndex].OffsetToDirectory);
    //resourceCount = languageDirectory->NumberOfIdEntries + languageDirectory->NumberOfNamedEntries;
    resourceLanguage = PTR_ADD_OFFSET(languageDirectory, sizeof(IMAGE_RESOURCE_DIRECTORY));
    resourceIndex = 0; // use the first entry

    if (resourceLanguage[resourceIndex].DataIsDirectory)
        return STATUS_RESOURCE_LANG_NOT_FOUND;

    resourceData = PTR_ADD_OFFSET(ResourceDirectory, resourceLanguage[resourceIndex].OffsetToData);

    if (!resourceData)
        return STATUS_RESOURCE_DATA_NOT_FOUND;

    resourceBuffer = PhMappedImageRvaToVa(MappedImage, resourceData->OffsetToData, NULL);

    if (!resourceBuffer)
        return STATUS_RESOURCE_DATA_NOT_FOUND;

    if (ResourceLength)
        *ResourceLength = resourceData->Size;
    if (ResourceBuffer)
        *ResourceBuffer = resourceBuffer;

    // if (LDR_IS_IMAGEMAPPING(ImageBaseAddress))
    // PhLoaderEntryImageRvaToVa(ImageBaseAddress, resourceData->OffsetToData, resourceBuffer);
    // PhLoadResource(ImageBaseAddress, MAKEINTRESOURCE(ResourceIndex), ResourceType, &resourceLength, &resourceBuffer);

    return STATUS_SUCCESS;
}

#ifndef MAKEFOURCC
#define MAKEFOURCC(ch0, ch1, ch2, ch3) \
 ((ULONG)(BYTE)(ch0) | ((ULONG)(BYTE)(ch1) << 8) | \
 ((ULONG)(BYTE)(ch2) << 16) | ((ULONG)(BYTE)(ch3) << 24 ))
#endif

// https://docs.microsoft.com/en-us/windows/win32/menurc/newheader
// One or more RESDIR structures immediately follow the NEWHEADER structure.
typedef struct _NEWHEADER
{
    USHORT Reserved;
    USHORT ResourceType;
    USHORT ResourceCount;
} NEWHEADER, *PNEWHEADER;

NTSTATUS PhCreateIconFromResourceDirectory(
    _In_ PPH_MAPPED_IMAGE MappedImage,
    _In_ PVOID ResourceDirectory,
    _In_ PVOID IconDirectory,
    _In_ LONG Width,
    _In_ LONG Height,
    _In_ ULONG Flags,
    _Out_opt_ HICON* IconHandle
    )
{
    NTSTATUS status;
    HICON iconHandle;
    LONG iconResourceId;
    ULONG iconResourceLength;
    PVOID iconResourceBuffer;

    if (!(iconResourceId = LookupIconIdFromDirectoryEx(
        IconDirectory,
        TRUE,
        Width,
        Height,
        Flags
        )))
    {
        return PhGetLastWin32ErrorAsNtStatus();
    }

    status = PhLoadIconFromResourceDirectory(
        MappedImage,
        ResourceDirectory,
        -iconResourceId,
        RT_ICON,
        &iconResourceLength,
        &iconResourceBuffer
        );

    if (!NT_SUCCESS(status))
        return status;

    if (
        ((PBITMAPINFOHEADER)iconResourceBuffer)->biSize != sizeof(BITMAPINFOHEADER) &&
        ((PBITMAPCOREHEADER)iconResourceBuffer)->bcSize != sizeof(BITMAPCOREHEADER) &&
        ((PBITMAPCOREHEADER)iconResourceBuffer)->bcSize != MAKEFOURCC(137, 'P', 'N', 'G') &&
        ((PBITMAPCOREHEADER)iconResourceBuffer)->bcSize != MAKEFOURCC('J', 'P', 'E', 'G')
        )
    {
        return STATUS_RESOURCE_TYPE_NOT_FOUND;
    }

    if (!(iconHandle = CreateIconFromResourceEx(
        iconResourceBuffer,
        iconResourceLength,
        TRUE,
        0x30000,
        Width,
        Height,
        Flags
        )))
    {
        return PhGetLastWin32ErrorAsNtStatus();
    }

    *IconHandle = iconHandle;
    return STATUS_SUCCESS;
}

// rev from LdrLoadAlternateResourceModuleEx and GetMunResourceModuleForEnumIfExist (dmex)
/**
 * Retrieves the filename of the \SystemResources\.mun alternate resource (mandatory on 19H1 and later).
 *
 * \param FileName A string containing a file name.
 * \param NativeFileName The type of name format.
 * \param FilePathType
 * \param ResourceFileName A pointer to the MUN filename.
 *
 * \return Successful or errant status.
 *
 * \remarks LdrLoadAlternateResourceModuleEx and GetMunResourceModuleForEnumIfExist always search the parent directory
 * and this function has the same logic and semantics. For example: C:\Windows\explorer.exe -> C:\SystemResources\explorer.exe.mun
 */
NTSTATUS PhGetSystemResourcesFileName(
    _In_ PCPH_STRINGREF FileName,
    _In_ BOOLEAN NativeFileName,
    _In_ RTL_PATH_TYPE FilePathType,
    _Out_ PPH_STRING* ResourceFileName
    )
{
    static CONST PH_STRINGREF directoryName = PH_STRINGREF_INIT(L"\\SystemResources\\");
    static CONST PH_STRINGREF extensionName = PH_STRINGREF_INIT(L".mun");
    PPH_STRING fileName;
    PH_STRINGREF directoryPart;
    PH_STRINGREF fileNamePart;
    PH_STRINGREF baseNamePart;

    if (WindowsVersion < WINDOWS_10_19H1)
        return STATUS_UNSUCCESSFUL;
    if (FilePathType == RtlPathTypeUncAbsolute)
        return STATUS_OBJECT_PATH_SYNTAX_BAD;
    if (!PhGetBasePath(FileName, &directoryPart, &fileNamePart))
        return STATUS_OBJECT_PATH_INVALID;

    if (directoryPart.Length && fileNamePart.Length)
    {
        if (!PhGetBasePath(&directoryPart, &baseNamePart, NULL))
            return STATUS_OBJECT_PATH_INVALID;

        fileName = PhConcatStringRef4(
            &baseNamePart,
            &directoryName,
            &fileNamePart,
            &extensionName
            );

        if (NativeFileName)
        {
            if (PhDoesFileExist(&fileName->sr))
            {
                *ResourceFileName = fileName;
                return STATUS_SUCCESS;
            }
        }
        else
        {
            if (PhDoesFileExistWin32(PhGetString(fileName)))
            {
                *ResourceFileName = fileName;
                return STATUS_SUCCESS;
            }
        }

        PhClearReference(&fileName);
    }

    return STATUS_UNSUCCESSFUL;
}

/**
 * \brief Extracts icons from the specified executable file.
 *
 * \param FileName A string containing a file name.
 * \param NativeFileName The type of name format.
 * \param IconIndex The zero-based index of the icon within the group or a negative number for a specific resource identifier.
 * \param IconLargeWidth
 * \param IconLargeHeight
 * \param IconSmallWidth
 * \param IconSmallHeight
 * \param IconLarge A handle to the large icon within the group or handle to the an icon from the resource identifier.
 * \param IconSmall A handle to the small icon within the group or handle to the an icon from the resource identifier.
 *
 * \return Successful or errant status.
 *
 * \remarks Use this function instead of PrivateExtractIconExW() because images are mapped with SEC_COMMIT and READONLY
 * while PrivateExtractIconExW loads images with EXECUTE and SEC_IMAGE (section allocations and relocation processing).
 */
NTSTATUS PhExtractIconEx(
    _In_ PCPH_STRINGREF FileName,
    _In_ BOOLEAN NativeFileName,
    _In_ LONG IconIndex,
    _In_ LONG IconLargeWidth,
    _In_ LONG IconLargeHeight,
    _In_ LONG IconSmallWidth,
    _In_ LONG IconSmallHeight,
    _Out_opt_ HICON *IconLarge,
    _Out_opt_ HICON *IconSmall
    )
{
    NTSTATUS status;
    HICON iconLarge = NULL;
    HICON iconSmall = NULL;
    PPH_STRING resourceFileName = NULL;
    PH_STRINGREF fileName;
    PH_MAPPED_IMAGE mappedImage;
    PIMAGE_DATA_DIRECTORY dataDirectory;
    PIMAGE_RESOURCE_DIRECTORY resourceDirectory;
    ULONG iconDirectoryResourceLength;
    PNEWHEADER iconDirectoryResource;
    RTL_PATH_TYPE fileNameType;

    fileNameType = PhDetermineDosPathNameType(FileName);

    if (!(fileNameType == RtlPathTypeRooted || fileNameType == RtlPathTypeDriveAbsolute))
        return STATUS_OBJECT_PATH_SYNTAX_BAD;

    status = PhGetSystemResourcesFileName(
        FileName,
        NativeFileName,
        fileNameType,
        &resourceFileName
        );

    if (NT_SUCCESS(status))
    {
        fileName.Buffer = resourceFileName->Buffer;
        fileName.Length = resourceFileName->Length;
    }
    else
    {
        fileName.Buffer = FileName->Buffer;
        fileName.Length = FileName->Length;
    }

    if (PhIsNullOrEmptyStringRef(&fileName))
    {
        PhClearReference(&resourceFileName);
        return FALSE;
    }

    __try
    {
        if (NativeFileName)
        {
            status = PhLoadMappedImageEx(
                &fileName,
                NULL,
                &mappedImage
                );
        }
        else
        {
            status = PhLoadMappedImage(
                PhGetStringRefZ(&fileName),
                NULL,
                &mappedImage
                );
        }

        if (!NT_SUCCESS(status))
        {
            PhClearReference(&resourceFileName);
            return FALSE;
        }

        status = PhGetMappedImageDataDirectory(
            &mappedImage,
            IMAGE_DIRECTORY_ENTRY_RESOURCE,
            &dataDirectory
            );

        if (!NT_SUCCESS(status))
            goto CleanupExit;

        resourceDirectory = PhMappedImageRvaToVa(
            &mappedImage,
            dataDirectory->VirtualAddress,
            NULL
            );

        if (!resourceDirectory)
            goto CleanupExit;


        status = PhLoadIconFromResourceDirectory(
            &mappedImage,
            resourceDirectory,
            IconIndex,
            RT_GROUP_ICON,
            &iconDirectoryResourceLength,
            &iconDirectoryResource
            );

        if (!NT_SUCCESS(status))
            goto CleanupExit;

        if (iconDirectoryResource->ResourceType != RES_ICON)
            goto CleanupExit;

        if (IconLarge)
        {
            status = PhCreateIconFromResourceDirectory(
                &mappedImage,
                resourceDirectory,
                iconDirectoryResource,
                IconLargeWidth,
                IconLargeHeight,
                LR_DEFAULTCOLOR,
                &iconLarge
                );

            if (!NT_SUCCESS(status))
                goto CleanupExit;
        }

        if (IconSmall)
        {
            status = PhCreateIconFromResourceDirectory(
                &mappedImage,
                resourceDirectory,
                iconDirectoryResource,
                IconSmallWidth,
                IconSmallHeight,
                LR_DEFAULTCOLOR,
                &iconSmall
                );

            if (!NT_SUCCESS(status))
                goto CleanupExit;
        }
    }
    __except (EXCEPTION_EXECUTE_HANDLER)
    {
        status = GetExceptionCode();
    }

CleanupExit:

    PhUnloadMappedImage(&mappedImage);
    PhClearReference(&resourceFileName);

    if (IconLarge && IconSmall)
    {
        if (iconLarge && iconSmall)
        {
            *IconLarge = iconLarge;
            *IconSmall = iconSmall;
            return TRUE;
        }

        if (iconLarge)
            DestroyIcon(iconLarge);
        if (iconSmall)
            DestroyIcon(iconSmall);

        return FALSE;
    }

    if (IconLarge && iconLarge)
    {
        *IconLarge = iconLarge;
        return TRUE;
    }

    if (IconSmall && iconSmall)
    {
        *IconSmall = iconSmall;
        return TRUE;
    }

    if (iconLarge)
        DestroyIcon(iconLarge);
    if (iconSmall)
        DestroyIcon(iconSmall);

    return FALSE;
}

// Imagelist support

HIMAGELIST PhImageListCreate(
    _In_ LONG Width,
    _In_ LONG Height,
    _In_ LONG Flags,
    _In_ LONG InitialCount,
    _In_ LONG GrowCount
    )
{
    HRESULT status;
    IImageList2* imageList;

    status = ImageList_CoCreateInstance(
        &CLSID_ImageList,
        NULL,
        &IID_IImageList2,
        &imageList
        );

    if (SUCCEEDED(status))
    {
        status = IImageList2_Initialize(
            imageList,
            Width,
            Height,
            Flags,
            InitialCount,
            GrowCount
            );
    }

    //if (FAILED(status))
    //{
    //    status = ImageList_CoCreateInstance(
    //        &CLSID_ImageList,
    //        NULL,
    //        &IID_IImageList,
    //        &imageList
    //        );
    //}

    if (FAILED(status))
        return NULL;

    return IImageListToHIMAGELIST(imageList);
}

BOOLEAN PhImageListDestroy(
    _In_opt_ HIMAGELIST ImageListHandle
    )
{
    if (!ImageListHandle)
        return TRUE;

    return SUCCEEDED(IImageList2_Release((IImageList2*)ImageListHandle));
}

BOOLEAN PhImageListSetImageCount(
    _In_ HIMAGELIST ImageListHandle,
    _In_ ULONG Count
    )
{
    return SUCCEEDED(IImageList2_SetImageCount((IImageList2*)ImageListHandle, Count));
}

BOOLEAN PhImageListGetImageCount(
    _In_ HIMAGELIST ImageListHandle,
    _Out_ PLONG Count
    )
{
    return SUCCEEDED(IImageList2_GetImageCount((IImageList2*)ImageListHandle, Count));
}

BOOLEAN PhImageListSetBkColor(
    _In_ HIMAGELIST ImageListHandle,
    _In_ COLORREF BackgroundColor
    )
{
    COLORREF previousColor = 0;

    return SUCCEEDED(IImageList2_SetBkColor(
        (IImageList2*)ImageListHandle,
        BackgroundColor,
        &previousColor
        ));
}

LONG PhImageListAddIcon(
    _In_ HIMAGELIST ImageListHandle,
    _In_ HICON IconHandle
    )
{
    LONG index = INT_ERROR;

    IImageList2_ReplaceIcon(
        (IImageList2*)ImageListHandle,
        INT_ERROR,
        IconHandle,
        &index
        );

    return index;
}

LONG PhImageListAddBitmap(
    _In_ HIMAGELIST ImageListHandle,
    _In_ HBITMAP BitmapImage,
    _In_opt_ HBITMAP BitmapMask
    )
{
    LONG index = INT_ERROR;

    IImageList2_Add(
        (IImageList2*)ImageListHandle,
        BitmapImage,
        BitmapMask,
        &index
        );

    return index;
}

BOOLEAN PhImageListRemoveIcon(
    _In_ HIMAGELIST ImageListHandle,
    _In_ LONG Index
    )
{
    return SUCCEEDED(IImageList2_Remove(
        (IImageList2*)ImageListHandle,
        Index
        ));
}

HICON PhImageListGetIcon(
    _In_ HIMAGELIST ImageListHandle,
    _In_ LONG Index,
    _In_ ULONG Flags
    )
{
    HICON iconhandle = NULL;

    IImageList2_GetIcon(
        (IImageList2*)ImageListHandle,
        Index,
        Flags,
        &iconhandle
        );

    return iconhandle;
}

BOOLEAN PhImageListGetIconSize(
    _In_ HIMAGELIST ImageListHandle,
    _Out_ PLONG cx,
    _Out_ PLONG cy
    )
{
    return SUCCEEDED(IImageList2_GetIconSize(
        (IImageList2*)ImageListHandle,
        cx,
        cy
        ));
}

BOOLEAN PhImageListReplace(
    _In_ HIMAGELIST ImageListHandle,
    _In_ LONG Index,
    _In_ HBITMAP BitmapImage,
    _In_opt_ HBITMAP BitmapMask
    )
{
    return SUCCEEDED(IImageList2_Replace(
        (IImageList2*)ImageListHandle,
        Index,
        BitmapImage,
        BitmapMask
        ));
}

BOOLEAN PhImageListDrawIcon(
    _In_ HIMAGELIST ImageListHandle,
    _In_ LONG Index,
    _In_ HDC Hdc,
    _In_ LONG x,
    _In_ LONG y,
    _In_ UINT32 Style,
    _In_ BOOLEAN Disabled
    )
{
    return PhImageListDrawEx(
        ImageListHandle,
        Index,
        Hdc,
        x,
        y,
        0,
        0,
        CLR_DEFAULT,
        CLR_DEFAULT,
        Style,
        Disabled ? ILS_SATURATE : ILS_NORMAL
        );
}

BOOLEAN PhImageListDrawEx(
    _In_ HIMAGELIST ImageListHandle,
    _In_ LONG Index,
    _In_ HDC Hdc,
    _In_ LONG x,
    _In_ LONG y,
    _In_ LONG dx,
    _In_ LONG dy,
    _In_ COLORREF BackColor,
    _In_ COLORREF ForeColor,
    _In_ UINT32 Style,
    _In_ ULONG State
    )
{
    IMAGELISTDRAWPARAMS imagelistDraw;

    memset(&imagelistDraw, 0, sizeof(IMAGELISTDRAWPARAMS));
    imagelistDraw.cbSize = sizeof(IMAGELISTDRAWPARAMS);
    imagelistDraw.himl = ImageListHandle;
    imagelistDraw.hdcDst = Hdc;
    imagelistDraw.i = Index;
    imagelistDraw.x = x;
    imagelistDraw.y = y;
    imagelistDraw.cx = dx;
    imagelistDraw.cy = dy;
    imagelistDraw.rgbBk = BackColor;
    imagelistDraw.rgbFg = ForeColor;
    imagelistDraw.fStyle = Style;
    imagelistDraw.fState = State;

    return SUCCEEDED(IImageList2_Draw((IImageList2*)ImageListHandle, &imagelistDraw));
}

BOOLEAN PhImageListSetIconSize(
    _In_ HIMAGELIST ImageListHandle,
    _In_ LONG cx,
    _In_ LONG cy
    )
{
    return SUCCEEDED(IImageList2_SetIconSize((IImageList2*)ImageListHandle, cx, cy));
}

static const PH_FLAG_MAPPING PhpInitiateShutdownMappings[] =
{
    { PH_SHUTDOWN_RESTART, SHUTDOWN_RESTART },
    { PH_SHUTDOWN_POWEROFF, SHUTDOWN_POWEROFF },
    { PH_SHUTDOWN_INSTALL_UPDATES, SHUTDOWN_INSTALL_UPDATES },
    { PH_SHUTDOWN_HYBRID, SHUTDOWN_HYBRID },
    { PH_SHUTDOWN_RESTART_BOOTOPTIONS, SHUTDOWN_RESTART_BOOTOPTIONS },
    { PH_CREATE_PROCESS_EXTENDED_STARTUPINFO, EXTENDED_STARTUPINFO_PRESENT },
};

ULONG PhInitiateShutdown(
    _In_ ULONG Flags
    )
{
    ULONG status;
    ULONG newFlags;

    newFlags = 0;
    PhMapFlags1(&newFlags, Flags, PhpInitiateShutdownMappings, ARRAYSIZE(PhpInitiateShutdownMappings));

    status = InitiateShutdown(
        NULL,
        NULL,
        0,
        SHUTDOWN_FORCE_OTHERS | newFlags,
        SHTDN_REASON_FLAG_PLANNED
        );

    return status;
}

/**
 * Sets shutdown parameters for the current process relative to the other processes in the system.
 *
 * \param Level The shutdown priority for the current process.
 * \param Flags Optional flags for terminating the current process.
 *
 * \return Successful or errant status.
 */
BOOLEAN PhSetProcessShutdownParameters(
    _In_ ULONG Level,
    _In_ ULONG Flags
    )
{
    static PH_INITONCE initOnce = PH_INITONCE_INIT;
    static BOOL (WINAPI *SetProcessShutdownParameters_I)(
        _In_ ULONG dwLevel,
        _In_ ULONG dwFlags
        ) = NULL;

    if (PhBeginInitOnce(&initOnce))
    {
        PVOID baseAddress;

        if (baseAddress = PhLoadLibrary(L"kernel32.dll"))
        {
            SetProcessShutdownParameters_I = PhGetDllBaseProcedureAddress(baseAddress, "SetProcessShutdownParameters", 0);
        }

        PhEndInitOnce(&initOnce);
    }

    if (!SetProcessShutdownParameters_I)
        return FALSE;

    return !!SetProcessShutdownParameters_I(Level, Flags);
}

// Timeline drawing support

VOID PhCustomDrawTreeTimeLine(
    _In_ HDC Hdc,
    _In_ RECT CellRect,
    _In_ ULONG Flags,
    _In_opt_ PLARGE_INTEGER StartTime,
    _In_ PLARGE_INTEGER CreateTime
    )
{
    RECT rect = CellRect;
    RECT borderRect = CellRect;
    FLOAT percent;
    ULONG flags = 0;
    LARGE_INTEGER systemTime;
    LARGE_INTEGER startTime;
    LARGE_INTEGER createTime;

    if (StartTime)
    {
        PhQuerySystemTime(&systemTime);
        startTime.QuadPart = systemTime.QuadPart - StartTime->QuadPart;
        createTime.QuadPart = systemTime.QuadPart - CreateTime->QuadPart;
    }
    else
    {
        static LARGE_INTEGER bootTime = { 0 };

        if (bootTime.QuadPart == 0)
        {
            PhGetSystemBootTime(&bootTime);
        }

        PhQuerySystemTime(&systemTime);
        startTime.QuadPart = systemTime.QuadPart - bootTime.QuadPart;
        createTime.QuadPart = systemTime.QuadPart - CreateTime->QuadPart;
    }

    if (createTime.QuadPart > startTime.QuadPart)
    {
        SetFlag(flags, PH_DRAW_TIMELINE_OVERFLOW);
    }

    percent = (FLOAT)createTime.QuadPart / (FLOAT)startTime.QuadPart * 100.f;

    if (percent > 100.f)
        percent = 100.f;
    if (percent < 0.0005f)
        percent = 0.0f;

    if (FlagOn(Flags, PH_DRAW_TIMELINE_DARKTHEME))
        FillRect(Hdc, &rect, PhThemeWindowBackgroundBrush);
    else
        FillRect(Hdc, &rect, (HBRUSH)(COLOR_WINDOW + 1));

    PhInflateRect(&rect, -1, -1);
    rect.bottom += 1;
    PhInflateRect(&borderRect, -1, -1);
    borderRect.bottom += 1;

    if (FlagOn(Flags, PH_DRAW_TIMELINE_DARKTHEME))
    {
        FillRect(Hdc, &rect, PhThemeWindowBackgroundBrush);

        if (FlagOn(flags, PH_DRAW_TIMELINE_OVERFLOW))
            SetDCBrushColor(Hdc, RGB(128, 128, 128));
        else
            SetDCBrushColor(Hdc, RGB(0, 130, 135));

        SelectBrush(Hdc, PhGetStockBrush(DC_BRUSH));
    }
    else
    {
        FillRect(Hdc, &rect, (HBRUSH)(COLOR_BTNFACE + 1));

        if (FlagOn(flags, PH_DRAW_TIMELINE_OVERFLOW))
            SetDCBrushColor(Hdc, RGB(128, 128, 128));
        else
            SetDCBrushColor(Hdc, RGB(158, 202, 158));

        SelectBrush(Hdc, PhGetStockBrush(DC_BRUSH));
    }

    rect.left = (LONG)((LONG)rect.right + ((LONG)(rect.left - rect.right) * (percent / 100.f)));

    if (rect.left < borderRect.left)
        rect.left = borderRect.left;

    PatBlt(
        Hdc,
        rect.left,
        rect.top,
        rect.right - rect.left,
        rect.bottom - rect.top,
        PATCOPY
        );

    FrameRect(Hdc, &borderRect, PhGetStockBrush(GRAY_BRUSH));
}

// Windows Imaging Component (WIC) bitmap support

HBITMAP PhCreateDIBSection(
    _In_ HDC Hdc,
    _In_ PH_BUFFERFORMAT Format,
    _In_ LONG Width,
    _In_ LONG Height,
    _Outptr_opt_ _When_(return != NULL, _Notnull_) PVOID* Bits
    )
{
    switch (Format)
    {
    case PHBF_COMPATIBLEBITMAP:
        {
            if (Bits)
            {
                *Bits = NULL;
            }
            return CreateCompatibleBitmap(Hdc, Width, Height);
        }
        break;
    case PHBF_DIB:
    case PHBF_TOPDOWNDIB:
    case PHBF_TOPDOWNMONODIB:
        {
            BITMAPINFO bitmapInfo;

            memset(&bitmapInfo, 0, sizeof(BITMAPINFOHEADER));
            bitmapInfo.bmiHeader.biSize = sizeof(BITMAPINFOHEADER);
            bitmapInfo.bmiHeader.biWidth = Width;
            bitmapInfo.bmiHeader.biHeight = Format == PHBF_TOPDOWNDIB ? -Height : Height;
            bitmapInfo.bmiHeader.biPlanes = 1;
            bitmapInfo.bmiHeader.biBitCount = Format == PHBF_TOPDOWNMONODIB ? 1 : 32;
            bitmapInfo.bmiHeader.biCompression = BI_RGB;

            return CreateDIBSection(Hdc, &bitmapInfo, DIB_RGB_COLORS, Bits, NULL, 0);
        }
        break;
    }

    return NULL;
}

HBITMAP PhCreateBitmapHandle(
    _In_ LONG Width,
    _In_ LONG Height,
    _Outptr_opt_ _When_(return != NULL, _Notnull_) PVOID* Bits
    )
{
    HBITMAP bitmapHandle;
    BITMAPINFO bitmapInfo;

    memset(&bitmapInfo, 0, sizeof(BITMAPINFO));
    bitmapInfo.bmiHeader.biSize = sizeof(BITMAPINFOHEADER);
    bitmapInfo.bmiHeader.biWidth = Width;
    bitmapInfo.bmiHeader.biHeight = -Height;
    bitmapInfo.bmiHeader.biPlanes = 1;
    bitmapInfo.bmiHeader.biBitCount = 32;
    bitmapInfo.bmiHeader.biCompression = BI_RGB;

    bitmapHandle = CreateDIBSection(PhGetScreenDC(), &bitmapInfo, DIB_RGB_COLORS, Bits, NULL, 0);

    return bitmapHandle;
}

static PGUID PhpGetImageFormatDecoderType(
    _In_ PH_IMAGE_FORMAT_TYPE Format
    )
{
    switch (Format)
    {
    case PH_IMAGE_FORMAT_TYPE_ICO:
        return (PGUID)&GUID_ContainerFormatIco;
    case PH_IMAGE_FORMAT_TYPE_BMP:
        return (PGUID)&GUID_ContainerFormatBmp;
    case PH_IMAGE_FORMAT_TYPE_JPG:
        return (PGUID)&GUID_ContainerFormatJpeg;
    case PH_IMAGE_FORMAT_TYPE_PNG:
        return (PGUID)&GUID_ContainerFormatPng;
    }

    return (PGUID)&GUID_ContainerFormatRaw;
}

HBITMAP PhLoadImageFormatFromResource(
    _In_ PVOID DllBase,
    _In_ PCWSTR Name,
    _In_ PCWSTR Type,
    _In_ PH_IMAGE_FORMAT_TYPE Format,
    _In_ LONG Width,
    _In_ LONG Height
    )
{
    BOOLEAN success = FALSE;
    HBITMAP bitmapHandle = NULL;
    ULONG resourceLength = 0;
    WICInProcPointer resourceBuffer = NULL;
    PVOID bitmapBuffer = NULL;
    IWICImagingFactory* wicImagingFactory = NULL;
    IWICStream* wicBitmapStream = NULL;
    IWICBitmapSource* wicBitmapSource = NULL;
    IWICBitmapDecoder* wicBitmapDecoder = NULL;
    IWICBitmapFrameDecode* wicBitmapFrame = NULL;
    WICPixelFormatGUID pixelFormat;
    UINT sourceWidth = 0;
    UINT sourceHeight = 0;

    if (!NT_SUCCESS(PhLoadResource(DllBase, Name, Type, &resourceLength, &resourceBuffer)))
        goto CleanupExit;

    if (FAILED(PhGetClassObject(L"windowscodecs.dll", &CLSID_WICImagingFactory1, &IID_IWICImagingFactory, &wicImagingFactory)))
        goto CleanupExit;
    if (FAILED(IWICImagingFactory_CreateStream(wicImagingFactory, &wicBitmapStream)))
        goto CleanupExit;
    if (FAILED(IWICStream_InitializeFromMemory(wicBitmapStream, resourceBuffer, resourceLength)))
        goto CleanupExit;
    if (FAILED(IWICImagingFactory_CreateDecoder(wicImagingFactory, PhpGetImageFormatDecoderType(Format), &GUID_VendorMicrosoft, &wicBitmapDecoder)))
        goto CleanupExit;
    if (FAILED(IWICBitmapDecoder_Initialize(wicBitmapDecoder, (IStream*)wicBitmapStream, WICDecodeMetadataCacheOnDemand)))
        goto CleanupExit;
    if (FAILED(IWICBitmapDecoder_GetFrame(wicBitmapDecoder, 0, &wicBitmapFrame)))
        goto CleanupExit;
    if (FAILED(IWICBitmapFrameDecode_GetPixelFormat(wicBitmapFrame, &pixelFormat)))
        goto CleanupExit;

    if (IsEqualGUID(&pixelFormat, &GUID_WICPixelFormat32bppBGRA)) // CreateDIBSection format
    {
        if (FAILED(IWICBitmapFrameDecode_QueryInterface(wicBitmapFrame, &IID_IWICBitmapSource, &wicBitmapSource)))
            goto CleanupExit;
    }
    else
    {
        IWICFormatConverter* wicFormatConverter = NULL;

        if (FAILED(IWICImagingFactory_CreateFormatConverter(wicImagingFactory, &wicFormatConverter)))
            goto CleanupExit;

        if (FAILED(IWICFormatConverter_Initialize(
            wicFormatConverter,
            (IWICBitmapSource*)wicBitmapFrame,
            &GUID_WICPixelFormat32bppBGRA,
            WICBitmapDitherTypeNone,
            NULL,
            0.0f,
            WICBitmapPaletteTypeCustom
            )))
        {
            IWICFormatConverter_Release(wicFormatConverter);
            goto CleanupExit;
        }

        if (FAILED(IWICFormatConverter_QueryInterface(wicFormatConverter, &IID_IWICBitmapSource, &wicBitmapSource)))
            goto CleanupExit;

        IWICFormatConverter_Release(wicFormatConverter);
    }

    if (!(bitmapHandle = PhCreateBitmapHandle(Width, Height, &bitmapBuffer)))
        goto CleanupExit;

    if (SUCCEEDED(IWICBitmapSource_GetSize(wicBitmapSource, &sourceWidth, &sourceHeight)) && sourceWidth == Width && sourceHeight == Height)
    {
        if (SUCCEEDED(IWICBitmapSource_CopyPixels(
            wicBitmapSource,
            NULL,
            Width * sizeof(RGBQUAD),
            Width * Height * sizeof(RGBQUAD),
            bitmapBuffer
            )))
        {
            success = TRUE;
        }
    }
    else
    {
        IWICBitmapScaler* wicBitmapScaler = NULL;

        if (SUCCEEDED(IWICImagingFactory_CreateBitmapScaler(wicImagingFactory, &wicBitmapScaler)))
        {
            if (SUCCEEDED(IWICBitmapScaler_Initialize(
                wicBitmapScaler,
                wicBitmapSource,
                Width,
                Height,
                WindowsVersion < WINDOWS_10 ? WICBitmapInterpolationModeFant : WICBitmapInterpolationModeHighQualityCubic
                )))
            {
                if (SUCCEEDED(IWICBitmapScaler_CopyPixels(
                    wicBitmapScaler,
                    NULL,
                    Width * sizeof(RGBQUAD),
                    Width * Height * sizeof(RGBQUAD),
                    bitmapBuffer
                    )))
                {
                    success = TRUE;
                }
            }

            IWICBitmapScaler_Release(wicBitmapScaler);
        }
    }

CleanupExit:

    if (wicBitmapSource)
        IWICBitmapSource_Release(wicBitmapSource);
    if (wicBitmapDecoder)
        IWICBitmapDecoder_Release(wicBitmapDecoder);
    if (wicBitmapFrame)
        IWICBitmapFrameDecode_Release(wicBitmapFrame);
    if (wicBitmapStream)
        IWICStream_Release(wicBitmapStream);
    if (wicImagingFactory)
        IWICImagingFactory_Release(wicImagingFactory);

    if (!success)
    {
        if (bitmapHandle) DeleteBitmap(bitmapHandle);
        return NULL;
    }

    return bitmapHandle;
}

HBITMAP PhLoadImageFromAddress(
    _In_ PVOID Buffer,
    _In_ ULONG BufferLength,
    _In_ LONG Width,
    _In_ LONG Height
    )
{
    BOOLEAN success = FALSE;
    HBITMAP bitmapHandle = NULL;
    PVOID bitmapBuffer = NULL;
    IWICImagingFactory* wicImagingFactory = NULL;
    IWICStream* wicBitmapStream = NULL;
    IWICBitmapSource* wicBitmapSource = NULL;
    IWICBitmapDecoder* wicBitmapDecoder = NULL;
    IWICBitmapFrameDecode* wicBitmapFrame = NULL;
    WICPixelFormatGUID pixelFormat;
    UINT sourceWidth = 0;
    UINT sourceHeight = 0;

    if (FAILED(PhGetClassObject(L"windowscodecs.dll", &CLSID_WICImagingFactory1, &IID_IWICImagingFactory, &wicImagingFactory)))
        goto CleanupExit;
    if (FAILED(IWICImagingFactory_CreateStream(wicImagingFactory, &wicBitmapStream)))
        goto CleanupExit;
    if (FAILED(IWICStream_InitializeFromMemory(wicBitmapStream, Buffer, BufferLength)))
        goto CleanupExit;
    if (FAILED(IWICImagingFactory_CreateDecoderFromStream(wicImagingFactory, (IStream*)wicBitmapStream, &GUID_VendorMicrosoft, WICDecodeMetadataCacheOnDemand, &wicBitmapDecoder)))
        goto CleanupExit;
    if (FAILED(IWICBitmapDecoder_GetFrame(wicBitmapDecoder, 0, &wicBitmapFrame)))
        goto CleanupExit;
    if (FAILED(IWICBitmapFrameDecode_GetPixelFormat(wicBitmapFrame, &pixelFormat)))
        goto CleanupExit;

    if (IsEqualGUID(&pixelFormat, &GUID_WICPixelFormat32bppBGRA))
    {
        if (FAILED(IWICBitmapFrameDecode_QueryInterface(wicBitmapFrame, &IID_IWICBitmapSource, &wicBitmapSource)))
            goto CleanupExit;
    }
    else
    {
        IWICFormatConverter* wicFormatConverter = NULL;

        if (FAILED(IWICImagingFactory_CreateFormatConverter(wicImagingFactory, &wicFormatConverter)))
            goto CleanupExit;

        if (FAILED(IWICFormatConverter_Initialize(
            wicFormatConverter,
            (IWICBitmapSource*)wicBitmapFrame,
            &GUID_WICPixelFormat32bppBGRA,
            WICBitmapDitherTypeNone,
            NULL,
            0.0f,
            WICBitmapPaletteTypeCustom
            )))
        {
            IWICFormatConverter_Release(wicFormatConverter);
            goto CleanupExit;
        }

        if (FAILED(IWICFormatConverter_QueryInterface(wicFormatConverter, &IID_IWICBitmapSource, &wicBitmapSource)))
            goto CleanupExit;

        IWICFormatConverter_Release(wicFormatConverter);
    }

    if (!(bitmapHandle = PhCreateBitmapHandle(Width, Height, &bitmapBuffer)))
        goto CleanupExit;

    if (SUCCEEDED(IWICBitmapSource_GetSize(wicBitmapSource, &sourceWidth, &sourceHeight)) && sourceWidth == Width && sourceHeight == Height)
    {
        if (SUCCEEDED(IWICBitmapSource_CopyPixels(
            wicBitmapSource,
            NULL,
            Width * sizeof(RGBQUAD),
            Width * Height * sizeof(RGBQUAD),
            bitmapBuffer
            )))
        {
            success = TRUE;
        }
    }
    else
    {
        IWICBitmapScaler* wicBitmapScaler = NULL;

        if (SUCCEEDED(IWICImagingFactory_CreateBitmapScaler(wicImagingFactory, &wicBitmapScaler)))
        {
            if (SUCCEEDED(IWICBitmapScaler_Initialize(
                wicBitmapScaler,
                wicBitmapSource,
                Width,
                Height,
                WindowsVersion < WINDOWS_10 ? WICBitmapInterpolationModeFant : WICBitmapInterpolationModeHighQualityCubic
                )))
            {
                if (SUCCEEDED(IWICBitmapScaler_CopyPixels(
                    wicBitmapScaler,
                    NULL,
                    Width * sizeof(RGBQUAD),
                    Width * Height * sizeof(RGBQUAD),
                    bitmapBuffer
                    )))
                {
                    success = TRUE;
                }
            }

            IWICBitmapScaler_Release(wicBitmapScaler);
        }
    }

CleanupExit:

    if (wicBitmapSource)
        IWICBitmapSource_Release(wicBitmapSource);
    if (wicBitmapDecoder)
        IWICBitmapDecoder_Release(wicBitmapDecoder);
    if (wicBitmapFrame)
        IWICBitmapFrameDecode_Release(wicBitmapFrame);
    if (wicBitmapStream)
        IWICStream_Release(wicBitmapStream);
    if (wicImagingFactory)
        IWICImagingFactory_Release(wicImagingFactory);

    if (!success)
    {
        if (bitmapHandle) DeleteBitmap(bitmapHandle);
        return NULL;
    }

    return bitmapHandle;
}

// Load image and auto-detect the format (dmex)
HBITMAP PhLoadImageFromResource(
    _In_ PVOID DllBase,
    _In_ PCWSTR Name,
    _In_ PCWSTR Type,
    _In_ LONG Width,
    _In_ LONG Height
    )
{
    ULONG resourceLength;
    WICInProcPointer resourceBuffer;

    if (NT_SUCCESS(PhLoadResource(DllBase, Name, Type, &resourceLength, &resourceBuffer)))
    {
        return PhLoadImageFromAddress(resourceBuffer, resourceLength, Width, Height);
    }

    return NULL;
}

// Load image and auto-detect the format (dmex)
HBITMAP PhLoadImageFromFile(
    _In_ PCWSTR FileName,
    _In_ LONG Width,
    _In_ LONG Height
    )
{
    BOOLEAN success = FALSE;
    HBITMAP bitmapHandle = NULL;
    PVOID bitmapBuffer = NULL;
    IWICImagingFactory* wicImagingFactory = NULL;
    IWICBitmapSource* wicBitmapSource = NULL;
    IWICBitmapDecoder* wicBitmapDecoder = NULL;
    IWICBitmapFrameDecode* wicBitmapFrame = NULL;
    WICPixelFormatGUID pixelFormat;
    UINT sourceWidth = 0;
    UINT sourceHeight = 0;

    if (FAILED(PhGetClassObject(L"windowscodecs.dll", &CLSID_WICImagingFactory1, &IID_IWICImagingFactory, &wicImagingFactory)))
        goto CleanupExit;
    if (FAILED(IWICImagingFactory_CreateDecoderFromFilename(wicImagingFactory, FileName, &GUID_VendorMicrosoft, GENERIC_READ, WICDecodeMetadataCacheOnDemand, &wicBitmapDecoder)))
        goto CleanupExit;
    if (FAILED(IWICBitmapDecoder_GetFrame(wicBitmapDecoder, 0, &wicBitmapFrame)))
        goto CleanupExit;
    if (FAILED(IWICBitmapFrameDecode_GetPixelFormat(wicBitmapFrame, &pixelFormat)))
        goto CleanupExit;

    if (IsEqualGUID(&pixelFormat, &GUID_WICPixelFormat32bppBGRA))
    {
        if (FAILED(IWICBitmapFrameDecode_QueryInterface(wicBitmapFrame, &IID_IWICBitmapSource, &wicBitmapSource)))
            goto CleanupExit;
    }
    else
    {
        IWICFormatConverter* wicFormatConverter = NULL;

        if (FAILED(IWICImagingFactory_CreateFormatConverter(wicImagingFactory, &wicFormatConverter)))
            goto CleanupExit;

        if (FAILED(IWICFormatConverter_Initialize(
            wicFormatConverter,
            (IWICBitmapSource*)wicBitmapFrame,
            &GUID_WICPixelFormat32bppBGRA,
            WICBitmapDitherTypeNone,
            NULL,
            0.0f,
            WICBitmapPaletteTypeCustom
            )))
        {
            IWICFormatConverter_Release(wicFormatConverter);
            goto CleanupExit;
        }

        if (FAILED(IWICFormatConverter_QueryInterface(wicFormatConverter, &IID_IWICBitmapSource, &wicBitmapSource)))
            goto CleanupExit;

        IWICFormatConverter_Release(wicFormatConverter);
    }

    if (!(bitmapHandle = PhCreateBitmapHandle(Width, Height, &bitmapBuffer)))
        goto CleanupExit;

    if (SUCCEEDED(IWICBitmapSource_GetSize(wicBitmapSource, &sourceWidth, &sourceHeight)) && sourceWidth == Width && sourceHeight == Height)
    {
        if (SUCCEEDED(IWICBitmapSource_CopyPixels(
            wicBitmapSource,
            NULL,
            Width * sizeof(RGBQUAD),
            Width * Height * sizeof(RGBQUAD),
            bitmapBuffer
            )))
        {
            success = TRUE;
        }
    }
    else
    {
        IWICBitmapScaler* wicBitmapScaler = NULL;

        if (SUCCEEDED(IWICImagingFactory_CreateBitmapScaler(wicImagingFactory, &wicBitmapScaler)))
        {
            if (SUCCEEDED(IWICBitmapScaler_Initialize(
                wicBitmapScaler,
                wicBitmapSource,
                Width,
                Height,
                WindowsVersion < WINDOWS_10 ? WICBitmapInterpolationModeFant : WICBitmapInterpolationModeHighQualityCubic
                )))
            {
                if (SUCCEEDED(IWICBitmapScaler_CopyPixels(
                    wicBitmapScaler,
                    NULL,
                    Width * sizeof(RGBQUAD),
                    Width * Height * sizeof(RGBQUAD),
                    bitmapBuffer
                    )))
                {
                    success = TRUE;
                }
            }

            IWICBitmapScaler_Release(wicBitmapScaler);
        }
    }

CleanupExit:

    if (wicBitmapSource)
        IWICBitmapSource_Release(wicBitmapSource);
    if (wicBitmapDecoder)
        IWICBitmapDecoder_Release(wicBitmapDecoder);
    if (wicBitmapFrame)
        IWICBitmapFrameDecode_Release(wicBitmapFrame);
    if (wicImagingFactory)
        IWICImagingFactory_Release(wicImagingFactory);

    if (!success)
    {
        if (bitmapHandle) DeleteBitmap(bitmapHandle);
        return NULL;
    }

    return bitmapHandle;
}

NTSTATUS PhGetWindowCompositionAttribute(
    _In_ HWND WindowHandle,
    _Inout_ PWINDOWCOMPOSITIONATTRIBUTEDATA AttributeData
    )
{
    static PH_INITONCE initOnce = PH_INITONCE_INIT;
    static BOOL (WINAPI* GetWindowCompositionAttribute_I)(
        _In_ HWND WindowHandle,
        _Inout_ PWINDOWCOMPOSITIONATTRIBUTEDATA AttributeData
        ) = NULL;

    if (PhBeginInitOnce(&initOnce))
    {
        GetWindowCompositionAttribute_I = PhGetDllProcedureAddress(L"user32.dll", "GetWindowCompositionAttribute", 0);
        PhEndInitOnce(&initOnce);
    }

    if (GetWindowCompositionAttribute_I)
    {
        if (GetWindowCompositionAttribute_I(WindowHandle, AttributeData))
        {
            return STATUS_SUCCESS;
        }
    }

    return PhGetLastWin32ErrorAsNtStatus();
}

NTSTATUS PhSetWindowCompositionAttribute(
    _In_ HWND WindowHandle,
    _In_ PWINDOWCOMPOSITIONATTRIBUTEDATA AttributeData
    )
{
    static PH_INITONCE initOnce = PH_INITONCE_INIT;
    static BOOL (WINAPI* SetWindowCompositionAttribute_I)(
        _In_ HWND WindowHandle,
        _In_ PWINDOWCOMPOSITIONATTRIBUTEDATA AttributeData
        ) = NULL;

    if (PhBeginInitOnce(&initOnce))
    {
        SetWindowCompositionAttribute_I = PhGetDllProcedureAddress(L"user32.dll", "SetWindowCompositionAttribute", 0);
        PhEndInitOnce(&initOnce);
    }

    if (SetWindowCompositionAttribute_I)
    {
        if (SetWindowCompositionAttribute_I(WindowHandle, AttributeData))
        {
            return STATUS_SUCCESS;
        }
    }

    return PhGetLastWin32ErrorAsNtStatus();
}

NTSTATUS PhSetWindowAcrylicCompositionColor(
    _In_ HWND WindowHandle,
    _In_ ULONG GradientColor
    )
{
    ACCENT_POLICY policy =
    {
        ACCENT_ENABLE_ACRYLICBLURBEHIND,
        ACCENT_WINDOWS11_LUMINOSITY | ACCENT_BORDER_ALL,
        GradientColor,
        0
    };
    WINDOWCOMPOSITIONATTRIBUTEDATA attribute =
    {
        WCA_ACCENT_POLICY,
        &policy,
        sizeof(ACCENT_POLICY)
    };

    return PhSetWindowCompositionAttribute(WindowHandle, &attribute);
}

HCURSOR PhLoadArrowCursor(
    VOID
    )
{
    static PH_INITONCE initOnce = PH_INITONCE_INIT;
    static HCURSOR arrowCursorHandle = NULL;

    if (PhBeginInitOnce(&initOnce))
    {
        arrowCursorHandle = PhLoadCursor(NULL, IDC_ARROW);
        PhEndInitOnce(&initOnce);
    }

    return arrowCursorHandle;
}

HCURSOR PhLoadDividerCursor(
    VOID
    )
{
    static PH_INITONCE initOnce = PH_INITONCE_INIT;
    static HCURSOR dividerCursorHandle = NULL;

    if (PhBeginInitOnce(&initOnce))
    {
        PVOID baseAddress;

        //dividerCursorHandle = PhLoadCursor(NULL, IDC_SIZEWE);

        if (baseAddress = PhGetLoaderEntryDllBaseZ(L"comctl32.dll"))
        {
            dividerCursorHandle = PhLoadCursor(baseAddress, IDC_DIVIDER);
        }

        PhEndInitOnce(&initOnce);
    }

    return dividerCursorHandle;
}

NTSTATUS PhIsInteractiveUserSession(
    VOID
    )
{
    USEROBJECTFLAGS flags;

    memset(&flags, 0, sizeof(USEROBJECTFLAGS));

    if (GetUserObjectInformation(
        GetProcessWindowStation(),
        UOI_FLAGS,
        &flags,
        sizeof(USEROBJECTFLAGS),
        NULL
        ))
    {
        if (BooleanFlagOn(flags.dwFlags, WSF_VISIBLE))
            return STATUS_SUCCESS;

        return STATUS_NOT_GUI_PROCESS;
    }

    return PhGetLastWin32ErrorAsNtStatus();
}

PPH_STRING PhGetCurrentWindowStationName(
    VOID
    )
{
    PPH_STRING string;

    string = PhCreateStringEx(NULL, 0x200);

    if (GetUserObjectInformation(
        GetProcessWindowStation(),
        UOI_NAME,
        string->Buffer,
        (ULONG)string->Length + sizeof(UNICODE_NULL),
        NULL
        ))
    {
        PhTrimToNullTerminatorString(string);
        return string;
    }
    else
    {
        PhDereferenceObject(string);
        return PhCreateString(L"WinSta0"); // assume the current window station is WinSta0
    }
}

PPH_STRING PhGetCurrentThreadDesktopName(
    VOID
    )
{
    PPH_STRING string;

    string = PhCreateStringEx(NULL, 0x200);

    if (GetUserObjectInformation(
        GetThreadDesktop(HandleToUlong(NtCurrentThreadId())),
        UOI_NAME,
        string->Buffer,
        (ULONG)string->Length + sizeof(UNICODE_NULL),
        NULL
        ))
    {
        PhTrimToNullTerminatorString(string);
        return string;
    }
    else
    {
        PhDereferenceObject(string);
        return PhCreateString(L"Default");
    }
}

BOOLEAN PhpInitializeMRUList(VOID)
{
    static PH_INITONCE initOnce = PH_INITONCE_INIT;
    static PVOID comctl32ModuleHandle = NULL;

    if (PhBeginInitOnce(&initOnce))
    {
        if (comctl32ModuleHandle = PhLoadLibrary(L"comctl32.dll"))
        {
            CreateMRUList_I = PhGetDllBaseProcedureAddress(comctl32ModuleHandle, "CreateMRUListW", 0);
            AddMRUString_I = PhGetDllBaseProcedureAddress(comctl32ModuleHandle, "AddMRUStringW", 0);
            EnumMRUList_I = PhGetDllBaseProcedureAddress(comctl32ModuleHandle, "EnumMRUListW", 0);
            FreeMRUList_I = PhGetDllBaseProcedureAddress(comctl32ModuleHandle, "FreeMRUList", 0);

            if (!(CreateMRUList_I && AddMRUString_I && EnumMRUList_I && FreeMRUList_I))
            {
                PhFreeLibrary(comctl32ModuleHandle);
                comctl32ModuleHandle = NULL;
            }
        }

        PhEndInitOnce(&initOnce);
    }

    if (comctl32ModuleHandle)
        return TRUE;

    return FALSE;
}

_Success_(return)
BOOLEAN PhRecentListCreate(
    _Out_ PHANDLE RecentHandle
    )
{
    HANDLE handle;
    MRUINFO info;

    if (!PhpInitializeMRUList())
        return FALSE;

    memset(&info, 0, sizeof(MRUINFO));
    info.cbSize = sizeof(MRUINFO);
    info.uMaxItems = UINT_MAX;
    info.hKey = HKEY_CURRENT_USER;
    info.lpszSubKey = L"Software\\Microsoft\\Windows\\CurrentVersion\\Explorer\\RunMRU";

    if (handle = CreateMRUList_I(&info))
    {
        *RecentHandle = handle;
        return TRUE;
    }

    return FALSE;
}

VOID PhRecentListDestroy(
    _In_ HANDLE RecentHandle
    )
{
    if (!PhpInitializeMRUList())
        return;

    if (RecentHandle)
    {
        FreeMRUList_I(RecentHandle);
    }
}

BOOLEAN PhRecentListAddString(
    _In_ HANDLE RecentHandle,
    _In_ PCWSTR String
    )
{
    if (!PhpInitializeMRUList())
        return FALSE;

    return AddMRUString_I(RecentHandle, String) != INT_ERROR;
}

BOOLEAN PhRecentListAddCommand(
    _In_ PPH_STRINGREF Command
    )
{
    static CONST PH_STRINGREF prefixSr = PH_STRINGREF_INIT(L"\\1");
    BOOLEAN status;
    HANDLE listHandle;
    PPH_STRING command;

    if (!PhpInitializeMRUList())
        return FALSE;
    if (!PhRecentListCreate(&listHandle))
        return FALSE;

    command = PhConcatStringRef2(Command, &prefixSr);

    status = PhRecentListAddString(listHandle, PhGetString(command));

    PhDereferenceObject(command);

    PhRecentListDestroy(listHandle);

    return status;
}

VOID PhEnumerateRecentList(
    _In_ PPH_ENUM_MRULIST_CALLBACK Callback,
    _In_opt_ PVOID Context
    )
{
    HANDLE listHandle;
    LONG listCount;

    if (!PhpInitializeMRUList())
        return;
    if (!PhRecentListCreate(&listHandle))
        return;

    listCount = EnumMRUList_I(
        listHandle,
        MAXINT,
        NULL,
        0
        );

    for (LONG i = 0; i < listCount; i++)
    {
        PH_STRINGREF string;
        SIZE_T returnLength;
        WCHAR buffer[DOS_MAX_PATH_LENGTH] = L"";

        returnLength = EnumMRUList_I(
            listHandle,
            i,
            buffer,
            RTL_NUMBER_OF(buffer)
            );

        if (returnLength >= RTL_NUMBER_OF(buffer))
            continue;
        if (returnLength < sizeof(UNICODE_NULL))
            continue;

        buffer[returnLength] = UNICODE_NULL;

        string.Buffer = buffer;
        string.Length = returnLength * sizeof(WCHAR);

        // trim \\1 (dmex)
        if (string.Buffer[returnLength - 1] == L'1' && string.Buffer[returnLength - 2] == L'\\')
        {
            string.Buffer[returnLength - 2] = UNICODE_NULL;
            string.Length -= 4;
        }

        if (!Callback(&string, Context))
            break;
    }

    FreeMRUList_I(listHandle);
}

/**
 * Forcibly closes the specified window.
 *
 * @param WindowHandle A handle to the window to be closed.
 * @param Force If TRUE, force the destruction of the window if an initial attempt to gently close the window using WM_CLOSE fails. If FALSE, only WM_CLOSE is attempted.
 * @return NTSTATUS Successful or errant status.
 * @remarks https://learn.microsoft.com/en-us/windows/win32/api/winuser/nf-winuser-endtask
 */
NTSTATUS PhTerminateWindow(
    _In_ HWND WindowHandle,
    _In_ BOOLEAN Force
    )
{
    static PH_INITONCE initOnce = PH_INITONCE_INIT;
    static BOOL (WINAPI* EndTask_I)(_In_ HWND hWnd, _In_ BOOL fShutDown, _In_ BOOL fForce) = NULL;

    if (PhBeginInitOnce(&initOnce))
    {
        EndTask_I = PhGetDllProcedureAddress(L"user32.dll", "EndTask", 0);
        PhEndInitOnce(&initOnce);
    }

    if (!EndTask_I)
        return STATUS_PROCEDURE_NOT_FOUND;

    if (EndTask_I(WindowHandle, FALSE, !!Force))
        return STATUS_SUCCESS;

    return PhGetLastWin32ErrorAsNtStatus();
}
