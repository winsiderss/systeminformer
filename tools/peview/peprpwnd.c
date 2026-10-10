/*
 * Copyright (c) 2022 Winsider Seminars & Solutions, Inc.  All rights reserved.
 *
 * This file is part of System Informer.
 *
 * Authors:
 *
 *     dmex    2021-2026
 *
 */

#include <peview.h>
#include <secedit.h>

typedef struct _PV_WINDOW_SECTION
{
    PH_STRINGREF Name;

    PVOID Instance;
    PWSTR Template;
    DLGPROC DialogProc;
    PVOID Parameter;

    HWND DialogHandle;
    HTREEITEM TreeItemHandle;
    INT IconIndex;
} PV_WINDOW_SECTION, *PPV_WINDOW_SECTION;

INT_PTR CALLBACK PvTabWindowDialogProc(
    _In_ HWND hwndDlg,
    _In_ UINT uMsg,
    _In_ WPARAM wParam,
    _In_ LPARAM lParam
    );

#define SWP_NO_ACTIVATE_MOVE_SIZE_ZORDER (SWP_NOACTIVATE | SWP_NOMOVE | SWP_NOSIZE | SWP_NOZORDER | SWP_NOOWNERZORDER)
#define SWP_SHOWWINDOW_ONLY (SWP_NO_ACTIVATE_MOVE_SIZE_ZORDER | SWP_SHOWWINDOW)
#define SWP_HIDEWINDOW_ONLY (SWP_NO_ACTIVATE_MOVE_SIZE_ZORDER | SWP_HIDEWINDOW)

VOID PvDestroyTabSection(
    _In_ PPV_WINDOW_SECTION Section
    );

VOID PvEnterTabSectionView(
    _In_ PPV_WINDOW_SECTION NewSection
    );

VOID PvLayoutTabSectionView(
    VOID
    );

VOID PvEnterTabSectionViewInner(
    _In_ PPV_WINDOW_SECTION Section,
    _Inout_opt_ HDWP *ContainerDeferHandle
    );

VOID PvCreateTabSectionDialog(
    _In_ PPV_WINDOW_SECTION Section
    );

VOID PvTabWindowOnSize(
    VOID
    );

PPV_WINDOW_SECTION PvFindTabSectionByName(
    _In_ PPH_STRINGREF Name
    );

PPV_WINDOW_SECTION PvGetSelectedTabSection(
    _In_opt_ PVOID TreeItemHandle
    );

HTREEITEM PvTreeViewInsertItem(
    _In_opt_ HTREEITEM HandleInsertAfter,
    _In_ PWSTR Text,
    _In_ PVOID Context,
    _In_ INT IconIndex
    );

PPV_WINDOW_SECTION PvCreateTabSection(
    _In_ PWSTR Name,
    _In_ INT IconIndex,
    _In_ PVOID Instance,
    _In_ PWSTR Template,
    _In_ DLGPROC DialogProc,
    _In_opt_ PVOID Parameter
    );

static HWND PvPropertiesWindowHandle = NULL;
static HWND PvTabTreeControl = NULL;
static HWND PvTabSplitterControl = NULL;
static HWND PvTabContainerControl = NULL;
static WNDPROC PvTabContainerDefaultWindowProc = NULL;
static INT PvPropertiesWindowShowCommand = SW_SHOW;
static PH_LAYOUT_MANAGER PvTabWindowLayoutManager;
static PPH_LIST PvTabSectionList = NULL;
static PPV_WINDOW_SECTION PvTabCurrentSection = NULL;
static LONG PvTabSidebarWidth = 210;
static BOOLEAN PvTabSplitterDragging = FALSE;
static BOOLEAN PvTabSplitterHot = FALSE;
static LONG PvTabSplitterDragOffset = 0;
static LONG PvTabWindowDpi = USER_DEFAULT_SCREEN_DPI;

#define WM_PV_SPLITTER (WM_APP + 120)
#define PV_SPLITTER_LINE_WIDTH 1
#define PV_SPLITTER_HIT_WIDTH 7
#define PV_SPLITTER_HOT_WIDTH 2
#define PV_SIDEBAR_MINIMUM_WIDTH 100
#define PV_SIDEBAR_DEFAULT_WIDTH 210
#define PV_SIDEBAR_MINIMUM_CONTENT 250

// One clamp for every path that can set the width: the settings load, the native
// splitter drag and the width the document reports after its own drag. Without this
// the native chrome, the document and the next session each normalize differently and
// end up showing three different sidebars. ClientWidth of zero skips the upper bound,
// which is what the load path wants before the window has a client area. (dmex)
LONG PvTabClampSidebarWidth(
    _In_ LONG Width,
    _In_ LONG Dpi,
    _In_ LONG ClientWidth
    )
{
    LONG minimumWidth;

    minimumWidth = PhMultiplyDivideSigned(PV_SIDEBAR_MINIMUM_WIDTH, Dpi, USER_DEFAULT_SCREEN_DPI);

    if (Width < minimumWidth)
        Width = minimumWidth;

    if (ClientWidth > 0)
    {
        LONG maximumWidth = ClientWidth - PhMultiplyDivideSigned(PV_SIDEBAR_MINIMUM_CONTENT, Dpi, USER_DEFAULT_SCREEN_DPI);

        Width = min(Width, max(minimumWidth, maximumWidth));
    }

    return Width;
}

VOID PvInvalidateSplitter(
    VOID
    )
{
    if (PvTabSplitterControl)
    {
        InvalidateRect(PvTabSplitterControl, NULL, FALSE);
    }
}

VOID PvLayoutTabWindow(
    _In_ HWND WindowHandle
    )
{
    RECT rect;
    LONG width;
    LONG paddingWidth;
    LONG splitterWidth;
    LONG splitterHitWidth;
    LONG contentLeft;
    LONG paddingLeft;
    LONG paddingRight;
    LONG paddingTop;
    LONG windowDpi;

    windowDpi = PhGetWindowDpi(WindowHandle);

    GetClientRect(WindowHandle, &rect);

    splitterWidth = max(PV_SPLITTER_LINE_WIDTH, PhMultiplyDivideSigned(PV_SPLITTER_LINE_WIDTH, windowDpi, USER_DEFAULT_SCREEN_DPI));
    splitterHitWidth = max(PV_SPLITTER_HIT_WIDTH, PhMultiplyDivideSigned(PV_SPLITTER_HIT_WIDTH, windowDpi, USER_DEFAULT_SCREEN_DPI));

    // Clamp for layout only; the user's preferred width is left untouched so
    // that a temporarily narrow client area doesn't destroy it. (dmex)
    paddingWidth = PhMultiplyDivideSigned(100, windowDpi, USER_DEFAULT_SCREEN_DPI);
    width = max(paddingWidth, PvTabSidebarWidth);
    width = min(width, max(paddingWidth, rect.right - PhMultiplyDivideSigned(250, windowDpi, USER_DEFAULT_SCREEN_DPI)));
    contentLeft = PhMultiplyDivideSigned(12, windowDpi, USER_DEFAULT_SCREEN_DPI) + width;
    paddingLeft = PhMultiplyDivideSigned(8, windowDpi, USER_DEFAULT_SCREEN_DPI);
    paddingRight = PhMultiplyDivideSigned(30, windowDpi, USER_DEFAULT_SCREEN_DPI);
    paddingTop = PhMultiplyDivideSigned(4, windowDpi, USER_DEFAULT_SCREEN_DPI);

    // Move the three controls in a single atomic update.(dmex)
    {
        HDWP deferHandle;

        if (deferHandle = BeginDeferWindowPos(3))
        {
            deferHandle = DeferWindowPos(
                deferHandle,
                PvTabTreeControl,
                NULL,
                paddingLeft,
                paddingLeft,
                width, max(0, rect.bottom - PhMultiplyDivideSigned(34, windowDpi, USER_DEFAULT_SCREEN_DPI)),
                SWP_NOACTIVATE | SWP_NOZORDER
                );

            deferHandle = DeferWindowPos(
                deferHandle,
                PvTabSplitterControl,
                NULL,
                contentLeft - splitterWidth - (splitterHitWidth - splitterWidth) / 2,
                paddingTop,
                splitterHitWidth,
                max(0, rect.bottom - paddingRight),
                SWP_NOACTIVATE | SWP_NOZORDER
                );

            deferHandle = DeferWindowPos(
                deferHandle,
                PvTabContainerControl,
                NULL,
                contentLeft,
                paddingTop,
                max(0, rect.right - contentLeft - paddingLeft),
                max(0, rect.bottom - paddingRight),
                SWP_NOACTIVATE | SWP_NOZORDER
                );

            EndDeferWindowPos(deferHandle);
        }
    }

    PvLayoutTabSectionView();
}

LRESULT CALLBACK PvSplitterWindowProc(
    _In_ HWND WindowHandle,
    _In_ UINT Message,
    _In_ WPARAM wParam,
    _In_ LPARAM lParam
    )
{
    switch (Message)
    {
    case WM_NCHITTEST:
        return HTCLIENT;
    case WM_ERASEBKGND:
        return TRUE;
    case WM_PAINT:
        {
            PAINTSTRUCT paintStruct;
            HDC hdc;
            HWND parentHandle;
            HBRUSH brush;
            RECT rect;
            RECT lineRect;
            LONG lineWidth;
            LONG windowDpi;
            BOOLEAN active;

            if (hdc = BeginPaint(WindowHandle, &paintStruct))
            {
                //INT savedDC = SaveDC(hdc);

                GetClientRect(WindowHandle, &rect);

                windowDpi = PhGetWindowDpi(WindowHandle);
                parentHandle = GetParent(WindowHandle);

                active = PvTabSplitterDragging || PvTabSplitterHot;

                // Thicken the divider while the user is hovering or dragging. (dmex)
                if (active)
                    lineWidth = max(PV_SPLITTER_HOT_WIDTH, PhMultiplyDivideSigned(PV_SPLITTER_HOT_WIDTH, windowDpi, USER_DEFAULT_SCREEN_DPI));
                else
                    lineWidth = max(PV_SPLITTER_LINE_WIDTH, PhMultiplyDivideSigned(PV_SPLITTER_LINE_WIDTH, windowDpi, USER_DEFAULT_SCREEN_DPI));

                lineWidth = min(lineWidth, max(1, rect.right - rect.left));
                lineRect = rect;
                lineRect.left = rect.left + max(0, ((rect.right - rect.left) - lineWidth) / 2);
                lineRect.right = lineRect.left + lineWidth;

                // Fill the grab area with the theme background so the splitter blends
                // in. When theming is disabled fall back to the parent's static brush,
                // and to a system brush when that comes back NULL - passing NULL to
                // FillRect silently no-ops and leaves the previous frame on screen. (dmex)
                if (!PvThemeEraseBackground(WindowHandle, hdc))
                {
                    brush = (HBRUSH)SendMessage(parentHandle, WM_CTLCOLORSTATIC, (WPARAM)hdc, (LPARAM)WindowHandle);

                    if (brush)
                        FillRect(hdc, &paintStruct.rcPaint, brush);
                    else
                        FillRect(hdc, &paintStruct.rcPaint, GetSysColorBrush(COLOR_BTNFACE));
                }

                if (!PvThemeFillRect(hdc, &lineRect, active ?
                    PvGetThemeAccentColor(PvThemeAccentPrimary) : PvGetThemeColors()->SplitterColor))
                {
                    if (active)
                        FillRect(hdc, &lineRect, GetSysColorBrush(COLOR_HIGHLIGHT));
                    else
                        DrawEdge(hdc, &lineRect, EDGE_ETCHED, BF_LEFT);
                }

                //if (savedDC)
                //    RestoreDC(hdc, savedDC);

                EndPaint(WindowHandle, &paintStruct);
            }
        }
        return 0;
    case WM_SETCURSOR:
        {
            SetCursor(LoadCursor(NULL, IDC_SIZEWE));
        }
        return TRUE;
    case WM_LBUTTONDOWN:
        {
            POINT point;

            GetCursorPos(&point);
            ScreenToClient(GetParent(WindowHandle), &point);

            // Remember where inside the splitter the drag started so the
            // sidebar doesn't jump to the cursor. (dmex)
            PvTabSplitterDragOffset = point.x - PvTabSidebarWidth;
            PvTabSplitterDragging = TRUE;

            SetCapture(WindowHandle);

            PvInvalidateSplitter();
        }
        return 0;
    case WM_MOUSEMOVE:
        {
            if (PvTabSplitterDragging && GetCapture() == WindowHandle)
            {
                POINT point;
                HWND parentHandle;

                parentHandle = GetParent(WindowHandle);

                GetCursorPos(&point);
                ScreenToClient(parentHandle, &point);

                SendMessage(parentHandle, WM_PV_SPLITTER, 0, MAKELPARAM(point.x - PvTabSplitterDragOffset, 0));
            }
            else if (!PvTabSplitterHot)
            {
                TRACKMOUSEEVENT trackMouseEvent;

                // The static has no hover state of its own, so ask for WM_MOUSELEAVE
                // to know when to drop the highlight again. (dmex)
                memset(&trackMouseEvent, 0, sizeof(TRACKMOUSEEVENT));
                trackMouseEvent.cbSize = sizeof(TRACKMOUSEEVENT);
                trackMouseEvent.dwFlags = TME_LEAVE;
                trackMouseEvent.hwndTrack = WindowHandle;

                PvTabSplitterHot = TRUE;
                TrackMouseEvent(&trackMouseEvent);

                PvInvalidateSplitter();
            }
        }
        return 0;
    case WM_MOUSELEAVE:
        {
            if (PvTabSplitterHot)
            {
                PvTabSplitterHot = FALSE;
                PvInvalidateSplitter();
            }
        }
        return 0;
    case WM_LBUTTONUP:
        {
            if (GetCapture() == WindowHandle)
            {
                ReleaseCapture();
            }

            PvTabSplitterDragging = FALSE;

            PvInvalidateSplitter();
        }
        return 0;
    case WM_KEYDOWN:
        {
            if (wParam == VK_ESCAPE && PvTabSplitterDragging)
            {
                if (GetCapture() == WindowHandle)
                {
                    ReleaseCapture();
                }

                PvTabSplitterDragging = FALSE;
                PvTabSplitterHot = FALSE;

                PvInvalidateSplitter();
                return 0;
            }
        }
        break;
    case WM_CAPTURECHANGED:
        {
            // Capture can be lost without a button-up (Alt-Tab, a modal dialog), so
            // clear the hover state too - the cursor may no longer be over us and
            // WM_MOUSELEAVE won't necessarily arrive. (dmex)
            PvTabSplitterDragging = FALSE;
            PvTabSplitterHot = FALSE;

            PvInvalidateSplitter();
        }
        return 0;
    case WM_NCDESTROY:
        {
            PvTabSplitterDragging = FALSE;
            PvTabSplitterHot = FALSE;
            PvTabSplitterControl = NULL;
        }
        break;
    }

    return DefWindowProc(WindowHandle, Message, wParam, lParam);
}

// The IDD_CONTAINER tab host is a plain "#32770" control with no dialog procedure
// of its own, so DefDlgProc would erase it with COLOR_3DFACE. Paint the theme
// background here (and hand out the theme brush for its own child controls) so the
// area behind and around the property pages never flashes the system color during
// page switches or resizes. (dmex)
LRESULT CALLBACK PvContainerWindowProc(
    _In_ HWND WindowHandle,
    _In_ UINT Message,
    _In_ WPARAM wParam,
    _In_ LPARAM lParam
    )
{
    switch (Message)
    {
    case WM_ERASEBKGND:
        {
            if (PvThemeEraseBackground(WindowHandle, (HDC)wParam))
                return TRUE;
        }
        break;
    case WM_CTLCOLORDLG:
    case WM_CTLCOLORSTATIC:
    case WM_CTLCOLORBTN:
        {
            HBRUSH brush;

            if (brush = PvThemeHandleCtlColor((HDC)wParam, Message != WM_CTLCOLORBTN))
                return (LRESULT)brush;
        }
        break;
    }

    return CallWindowProc(PvTabContainerDefaultWindowProc, WindowHandle, Message, wParam, lParam);
}

VOID PvConfigureTabSidebar(
    _In_ HWND WindowHandle
    )
{
    PvThemeApplyTreeView(PvTabTreeControl);
    PvSetTreeViewImageList(WindowHandle, PvTabTreeControl);

    // The buttons are deliberately left alone: PhInitializeWindowTheme already walked
    // the children and set them up the way phlib's button drawing expects. Re-theming
    // them here only risks undoing that. (dmex)
    PvThemeApplyControl(PvTabContainerControl);

    // The glyphs are only registered here; PvThemeDrawButton reads them back while
    // painting, so the buttons themselves are still left to phlib's setup. (dmex)
    PvThemeSetButtonGlyph(GetDlgItem(WindowHandle, IDC_OPTIONS), PvThemeButtonGlyphOptions);
    PvThemeSetButtonGlyph(GetDlgItem(WindowHandle, IDC_SECURITY), PvThemeButtonGlyphSecurity);

    PvThemeApplyWindowFrame(WindowHandle);
}

_Ret_maybenull_
HWND PvGetPePropertiesWindowHandle(
    VOID
    )
{
    return PvPropertiesWindowHandle;
}

VOID PvShowPePropertiesWindow(
    VOID
    )
{
    BOOL result;
    MSG message;
    PH_AUTO_POOL autoPool;

    PhInitializeAutoPool(&autoPool);

    PvPropertiesWindowHandle = PhCreateDialog(
        PhInstanceHandle,
        MAKEINTRESOURCE(IDD_TABWINDOW),
        NULL,
        PvTabWindowDialogProc,
        NULL
        );

    if (PhGetIntegerSetting(L"MainWindowState") == SW_MAXIMIZE)
    {
        PvPropertiesWindowShowCommand = SW_MAXIMIZE;
    }

    ShowWindow(PvPropertiesWindowHandle, PvPropertiesWindowShowCommand);
    SetForegroundWindow(PvPropertiesWindowHandle);
    PvStartPageFinishLoading();

    while (result = GetMessage(&message, NULL, 0, 0))
    {
        if (result == -1)
            break;

        if (!IsDialogMessage(PvPropertiesWindowHandle, &message))
        {
            TranslateMessage(&message);
            DispatchMessage(&message);
        }

        PhDrainAutoPool(&autoPool);
    }

    PhDeleteAutoPool(&autoPool);
}

VOID PvAddTreeViewSections(
    VOID
    )
{
    PPV_WINDOW_SECTION section;
    PH_MAPPED_IMAGE_IMPORTS imports;
    PH_MAPPED_IMAGE_EXPORTS exports;
    PIMAGE_LOAD_CONFIG_DIRECTORY32 config32;
    PIMAGE_LOAD_CONFIG_DIRECTORY64 config64;
    PIMAGE_DATA_DIRECTORY entry;

    PvTabSectionList = PhCreateList(30);
    PvTabCurrentSection = NULL;

    // General page
    section = PvCreateTabSection(
        L"General",
        PV_SECTION_ICON_GENERAL,
        PhInstanceHandle,
        MAKEINTRESOURCE(IDD_PEGENERAL),
        PvPeGeneralDlgProc,
        NULL
        );

    // Headers page
    PvCreateTabSection(
        L"Headers",
        PV_SECTION_ICON_HEADERS,
        PhInstanceHandle,
        MAKEINTRESOURCE(IDD_PEHEADERS),
        PvPeHeadersDlgProc,
        NULL
        );

    // Load Config page
    if (NT_SUCCESS(PhGetMappedImageDataDirectory(&PvMappedImage, IMAGE_DIRECTORY_ENTRY_LOAD_CONFIG, &entry)))
    {
        PvCreateTabSection(
            L"Load Config",
            PV_SECTION_ICON_LOAD_CONFIG,
            PhInstanceHandle,
            MAKEINTRESOURCE(IDD_PELOADCONFIG),
            PvPeLoadConfigDlgProc,
            NULL
            );
    }

    // Sections page
    PvCreateTabSection(
        L"Sections",
        PV_SECTION_ICON_SECTIONS,
        PhInstanceHandle,
        MAKEINTRESOURCE(IDD_PESECTIONS),
        PvPeSectionsDlgProc,
        NULL
        );

    // Directories page
    PvCreateTabSection(
        L"Directories",
        PV_SECTION_ICON_DIRECTORIES,
        PhInstanceHandle,
        MAKEINTRESOURCE(IDD_PEDIRECTORY),
        PvPeDirectoryDlgProc,
        NULL
        );

    // Imports page
    if ((NT_SUCCESS(PhGetMappedImageImports(&imports, &PvMappedImage)) && imports.NumberOfDlls != 0) ||
        (NT_SUCCESS(PhGetMappedImageDelayImports(&imports, &PvMappedImage)) && imports.NumberOfDlls != 0))
    {
        PvCreateTabSection(
            L"Imports",
            PV_SECTION_ICON_IMPORTS,
            PhInstanceHandle,
            MAKEINTRESOURCE(IDD_PEIMPORTS),
            PvPeImportsDlgProc,
            NULL
            );
    }

    // Exports page
    if (NT_SUCCESS(PhGetMappedImageExports(&exports, &PvMappedImage)) && exports.NumberOfEntries != 0)
    {
        PPV_EXPORTS_PAGECONTEXT exportsPageContext;
        PPV_PROPPAGECONTEXT propPageContext;
        LPPROPSHEETPAGE propSheetPage;

        exportsPageContext = PhAllocateZero(sizeof(PV_EXPORTS_PAGECONTEXT));
        exportsPageContext->FreePropPageContext = TRUE;
        exportsPageContext->Context = ULongToPtr(0); // PhGetMappedImageExportsEx with no flags

        propPageContext = PhAllocateZero(sizeof(PV_PROPPAGECONTEXT));
        propPageContext->Context = exportsPageContext;
        propSheetPage = PhAllocateZero(sizeof(PROPSHEETPAGE));
        propSheetPage->lParam = (LPARAM)propPageContext;

        PvCreateTabSection(
            L"Exports",
            PV_SECTION_ICON_EXPORTS,
            PhInstanceHandle,
            MAKEINTRESOURCE(IDD_PEEXPORTS),
            PvPeExportsDlgProc,
            propSheetPage
            );


        // NativeAOT runtime debug header export.
        {
            PH_MAPPED_IMAGE_EXPORT_FUNCTION runtimeDebugFunction;

            if (NT_SUCCESS(PhGetMappedImageExportFunction(
                &exports,
                "DotNetRuntimeDebugHeader",
                0,
                &runtimeDebugFunction
                )) && runtimeDebugFunction.Function)
            {
                PvCreateTabSection(
                    L"NativeAOT",
                    PV_SECTION_ICON_EXPORTS,
                    PhInstanceHandle,
                    MAKEINTRESOURCE(IDD_PERUNTIMEDEBUG),
                    PvPeRuntimeDebugDlgProc,
                    propSheetPage
                    );
            }
        }
    }

    // Exports ARM64X page
    if (NT_SUCCESS(PhGetMappedImageExportsEx(&exports, &PvMappedImage, PH_GET_IMAGE_EXPORTS_ARM64X)) && exports.NumberOfEntries != 0)
    {
        PPV_EXPORTS_PAGECONTEXT exportsPageContext;
        PPV_PROPPAGECONTEXT propPageContext;
        LPPROPSHEETPAGE propSheetPage;

        exportsPageContext = PhAllocateZero(sizeof(PV_EXPORTS_PAGECONTEXT));
        exportsPageContext->FreePropPageContext = TRUE;
        exportsPageContext->Context = ULongToPtr(PH_GET_IMAGE_EXPORTS_ARM64X);

        propPageContext = PhAllocateZero(sizeof(PV_PROPPAGECONTEXT));
        propPageContext->Context = exportsPageContext;
        propSheetPage = PhAllocateZero(sizeof(PROPSHEETPAGE));
        propSheetPage->lParam = (LPARAM)propPageContext;

        PvCreateTabSection(
            L"Exports ARM64X",
            PV_SECTION_ICON_EXPORTS,
            PhInstanceHandle,
            MAKEINTRESOURCE(IDD_PEEXPORTS),
            PvPeExportsDlgProc,
            propSheetPage
            );
    }

    // Resources page
    if (NT_SUCCESS(PhGetMappedImageDataDirectory(&PvMappedImage, IMAGE_DIRECTORY_ENTRY_RESOURCE, &entry)))
    {
        PvCreateTabSection(
            L"Resources",
            PV_SECTION_ICON_RESOURCES,
            PhInstanceHandle,
            MAKEINTRESOURCE(IDD_PERESOURCES),
            PvPeResourcesDlgProc,
            NULL
            );

        // Manifest page
        if (NT_SUCCESS(PhGetMappedImageResource(&PvMappedImage, MAKEINTRESOURCEW(1), RT_MANIFEST, 0, NULL, NULL)))
        {
            PvCreateTabSection(
                L"Manifest",
                PV_SECTION_ICON_GENERAL,
                PhInstanceHandle,
                MAKEINTRESOURCE(IDD_PEPREVIEW),
                PvPeAppManifestDlgProc,
                NULL
                );
        }
    }

    // CLR page
    if (NT_SUCCESS(PhGetMappedImageDataDirectory(&PvMappedImage, IMAGE_DIRECTORY_ENTRY_COM_DESCRIPTOR, &entry)) &&
        NT_SUCCESS(PhMappedImageRvaToVa(&PvMappedImage, entry->VirtualAddress, (PVOID *)&PvImageCor20Header)))
    {
        NTSTATUS status = STATUS_SUCCESS;

        __try
        {
            PhProbeAddress(
                PvImageCor20Header,
                sizeof(IMAGE_COR20_HEADER),
                PvMappedImage.ViewBase,
                PvMappedImage.ViewSize,
                4
                );
        }
        __except (EXCEPTION_EXECUTE_HANDLER)
        {
            status = GetExceptionCode();
        }

        if (NT_SUCCESS(status))
        {
            PvCreateTabSection(
                L"CLR",
                PV_SECTION_ICON_CRT,
                PhInstanceHandle,
                MAKEINTRESOURCE(IDD_PECLR),
                PvpPeClrDlgProc,
                NULL
                );

            PvCreateTabSection(
                L"CLR Imports",
                PV_SECTION_ICON_IMPORTS,
                PhInstanceHandle,
                MAKEINTRESOURCE(IDD_PECLRIMPORTS),
                PvpPeClrImportsDlgProc,
                NULL
                );

            PvCreateTabSection(
                L"CLR Tables",
                PV_SECTION_ICON_HEADERS,
                PhInstanceHandle,
                MAKEINTRESOURCE(IDD_PECLRTABLES),
                PvPeClrTablesDlgProc,
                NULL
                );
        }
    }

    // CFG page
    if (PvMappedImage.NtHeaders->OptionalHeader.DllCharacteristics & IMAGE_DLLCHARACTERISTICS_GUARD_CF)
    {
        PvCreateTabSection(
            L"CFG",
            PV_SECTION_ICON_CFG,
            PhInstanceHandle,
            MAKEINTRESOURCE(IDD_PECFG),
            PvpPeCgfDlgProc,
            NULL
            );
    }

    // TLS page
    if (NT_SUCCESS(PhGetMappedImageDataDirectory(&PvMappedImage, IMAGE_DIRECTORY_ENTRY_TLS, &entry)))
    {
        PvCreateTabSection(
            L"TLS",
            PV_SECTION_ICON_VOLATILE,
            PhInstanceHandle,
            MAKEINTRESOURCE(IDD_TLS),
            PvpPeTlsDlgProc,
            NULL
            );
    }

    // ProdId page
    {
        ULONG imageDosStubLength = ((PIMAGE_DOS_HEADER)PvMappedImage.ViewBase)->e_lfanew - RTL_SIZEOF_THROUGH_FIELD(IMAGE_DOS_HEADER, e_lfanew);

        if (imageDosStubLength != 0)// && imageDosStubLength != 64)
        {
            PvCreateTabSection(
                L"ProdID",
                PV_SECTION_ICON_PDBID,
                PhInstanceHandle,
                MAKEINTRESOURCE(IDD_PEPRODID),
                PvpPeProdIdDlgProc,
                NULL
                );
        }
    }

    {
        BOOLEAN hasExceptions = FALSE;
        BOOLEAN hasExceptionsArm64X = FALSE;

        if (PvMappedImage.Magic == IMAGE_NT_OPTIONAL_HDR32_MAGIC)
        {
            if (NT_SUCCESS(PhGetMappedImageLoadConfig32(&PvMappedImage, &config32)) &&
                RTL_CONTAINS_FIELD(config32, config32->Size, SEHandlerCount))
            {
                if (config32->SEHandlerCount && config32->SEHandlerTable)
                    hasExceptions = TRUE;
            }
        }
        else
        {
            if (NT_SUCCESS(PhGetMappedImageDataDirectory(&PvMappedImage, IMAGE_DIRECTORY_ENTRY_EXCEPTION, &entry)))
            {
                IMAGE_DATA_DIRECTORY entryArm64X;

                hasExceptions = TRUE;

                if (NT_SUCCESS(PhRelocateMappedImageDataEntryARM64X(&PvMappedImage, entry, &entryArm64X)))
                    hasExceptionsArm64X = TRUE;
            }
        }

        // Exceptions page
        if (hasExceptions)
        {
            PPV_EXCEPTIONS_PAGECONTEXT exceptionsPageContext;
            PPV_PROPPAGECONTEXT propPageContext;
            LPPROPSHEETPAGE propSheetPage;

            exceptionsPageContext = PhAllocateZero(sizeof(PV_EXCEPTIONS_PAGECONTEXT));
            exceptionsPageContext->FreePropPageContext = TRUE;
            exceptionsPageContext->Context = ULongToPtr(0); // PhGetMappedImageExceptionsEx with no flags

            propPageContext = PhAllocateZero(sizeof(PV_PROPPAGECONTEXT));
            propPageContext->Context = exceptionsPageContext;
            propSheetPage = PhAllocateZero(sizeof(PROPSHEETPAGE));
            propSheetPage->lParam = (LPARAM)propPageContext;

            PvCreateTabSection(
                L"Exceptions",
                PV_SECTION_ICON_EXCEPTIONS,
                PhInstanceHandle,
                MAKEINTRESOURCE(IDD_PEEXCEPTIONS),
                PvpPeExceptionDlgProc,
                propSheetPage
                );
        }

        // Exceptions ARM64X page
        if (hasExceptionsArm64X)
        {
            PPV_EXCEPTIONS_PAGECONTEXT exceptionsPageContext;
            PPV_PROPPAGECONTEXT propPageContext;
            LPPROPSHEETPAGE propSheetPage;

            exceptionsPageContext = PhAllocateZero(sizeof(PV_EXCEPTIONS_PAGECONTEXT));
            exceptionsPageContext->FreePropPageContext = TRUE;
            exceptionsPageContext->Context = ULongToPtr(PH_GET_IMAGE_EXCEPTIONS_ARM64X);

            propPageContext = PhAllocateZero(sizeof(PV_PROPPAGECONTEXT));
            propPageContext->Context = exceptionsPageContext;
            propSheetPage = PhAllocateZero(sizeof(PROPSHEETPAGE));
            propSheetPage->lParam = (LPARAM)propPageContext;

            PvCreateTabSection(
                L"Exceptions ARM64X",
                PV_SECTION_ICON_EXCEPTIONS,
                PhInstanceHandle,
                MAKEINTRESOURCE(IDD_PEEXCEPTIONS),
                PvpPeExceptionDlgProc,
                propSheetPage
                );
        }
    }

    // Relocations page
    if (NT_SUCCESS(PhGetMappedImageDataDirectory(&PvMappedImage, IMAGE_DIRECTORY_ENTRY_BASERELOC, &entry)))
    {
        PvCreateTabSection(
            L"Relocations",
            PV_SECTION_ICON_RELOCATIONS,
            PhInstanceHandle,
            MAKEINTRESOURCE(IDD_PERELOCATIONS),
            PvpPeRelocationDlgProc,
            NULL
            );
    }

    // Dynmic Relocations page
    if (NT_SUCCESS(PhGetMappedImageDynamicRelocationsTable(&PvMappedImage, NULL)))
    {
        PvCreateTabSection(
            L"Dynamic Relocations",
            PV_SECTION_ICON_RELOCATIONS,
            PhInstanceHandle,
            MAKEINTRESOURCE(IDD_PEDYNAMICRELOCATIONS),
            PvpPeDynamicRelocationDlgProc,
            NULL
            );
    }

    // Hybrid Metadata page
    if (PhGetMappedImageCHPEVersion(&PvMappedImage))
    {
        PvCreateTabSection(
            L"Hybrid Metadata",
            PV_SECTION_ICON_VOLATILE,
            PhInstanceHandle,
            MAKEINTRESOURCE(IDD_PELOADCONFIG),
            PvpPeCHPEDlgProc,
            NULL
            );
    }

    // Certificates page
    if (NT_SUCCESS(PhGetMappedImageDataDirectory(&PvMappedImage, IMAGE_DIRECTORY_ENTRY_SECURITY, &entry)))
    {
        PvCreateTabSection(
            L"Certificates",
            PV_SECTION_ICON_CERTIFICATES,
            PhInstanceHandle,
            MAKEINTRESOURCE(IDD_PESECURITY),
            PvpPeSecurityDlgProc,
            NULL
            );
    }

    // Debug page
    if (NT_SUCCESS(PhGetMappedImageDataDirectory(&PvMappedImage, IMAGE_DIRECTORY_ENTRY_DEBUG, &entry)))
    {
        PvCreateTabSection(
            L"Debug",
            PV_SECTION_ICON_DEBUG,
            PhInstanceHandle,
            MAKEINTRESOURCE(IDD_PEDEBUG),
            PvpPeDebugDlgProc,
            NULL
            );
    }

    // Volatile page
    {
        BOOLEAN valid = FALSE;

        if (PvMappedImage.Magic == IMAGE_NT_OPTIONAL_HDR32_MAGIC)
        {
            if (NT_SUCCESS(PhGetMappedImageLoadConfig32(&PvMappedImage, &config32)) &&
                RTL_CONTAINS_FIELD(config32, config32->Size, VolatileMetadataPointer))
            {
                if (config32->VolatileMetadataPointer)
                    valid = TRUE;
            }
        }
        else
        {
            if (NT_SUCCESS(PhGetMappedImageLoadConfig64(&PvMappedImage, &config64)) &&
                RTL_CONTAINS_FIELD(config64, config64->Size, VolatileMetadataPointer))
            {
                if (config64->VolatileMetadataPointer)
                    valid = TRUE;
            }
        }

        if (valid)
        {
            PvCreateTabSection(
                L"Volatile Metadata",
                PV_SECTION_ICON_VOLATILE,
                PhInstanceHandle,
                MAKEINTRESOURCE(IDD_PEVOLATILE),
                PvpPeVolatileDlgProc,
                NULL
                );
        }
    }

    // EH continuation page
    {
        BOOLEAN has_ehcont = FALSE;

        if (PvMappedImage.Magic == IMAGE_NT_OPTIONAL_HDR32_MAGIC)
        {
            if (NT_SUCCESS(PhGetMappedImageLoadConfig32(&PvMappedImage, &config32)) &&
                RTL_CONTAINS_FIELD(config32, config32->Size, GuardEHContinuationCount))
            {
                if (config32->GuardEHContinuationTable && config32->GuardEHContinuationCount)
                    has_ehcont = TRUE;
            }
        }
        else
        {
            if (NT_SUCCESS(PhGetMappedImageLoadConfig64(&PvMappedImage, &config64)) &&
                RTL_CONTAINS_FIELD(config64, config64->Size, GuardEHContinuationCount))
            {
                if (config64->GuardEHContinuationTable && config64->GuardEHContinuationCount)
                    has_ehcont = TRUE;
            }
        }

        if (has_ehcont)
        {
            PvCreateTabSection(
                L"EH Continuation",
                PV_SECTION_ICON_EHCONT,
                PhInstanceHandle,
                MAKEINTRESOURCE(IDD_PEEHCONT),
                PvpPeEhContDlgProc,
                NULL
                );
        }
    }

    // Debug POGO page
    {
        BOOLEAN debugPogoValid = FALSE;
        PVOID debugEntry;

        if (NT_SUCCESS(PhGetMappedImageDebugEntryByType(
            &PvMappedImage,
            IMAGE_DEBUG_TYPE_POGO,
            NULL,
            &debugEntry
            )))
        {
            debugPogoValid = TRUE;
        }

        if (debugPogoValid)
        {
            PvCreateTabSection(
                L"POGO",
                PV_SECTION_ICON_POGO,
                PhInstanceHandle,
                MAKEINTRESOURCE(IDD_PEDEBUGPOGO),
                PvpPeDebugPogoDlgProc,
                NULL
                );

            PvCreateTabSection(
                L"CRT",
                PV_SECTION_ICON_CRT,
                PhInstanceHandle,
                MAKEINTRESOURCE(IDD_PEDEBUGCRT),
                PvpPeDebugCrtDlgProc,
                NULL
                );
        }
    }

    // Properties page
    PvCreateTabSection(
        L"Properties",
        PV_SECTION_ICON_PROPERTIES,
        PhInstanceHandle,
        MAKEINTRESOURCE(IDD_PEPROPSTORAGE),
        PvpPePropStoreDlgProc,
        NULL
        );

    // Extended attributes page
    PvCreateTabSection(
        L"Attributes",
        PV_SECTION_ICON_ATTRIBUTES,
        PhInstanceHandle,
        MAKEINTRESOURCE(IDD_PEATTR),
        PvpPeExtendedAttributesDlgProc,
        NULL
        );

    // Streams page
    PvCreateTabSection(
        L"Streams",
        PV_SECTION_ICON_STREAMS,
        PhInstanceHandle,
        MAKEINTRESOURCE(IDD_PESTREAMS),
        PvpPeStreamsDlgProc,
        NULL
        );

    // Layout page
    PvCreateTabSection(
        L"Layout",
        PV_SECTION_ICON_LAYOUT,
        PhInstanceHandle,
        MAKEINTRESOURCE(IDD_PELAYOUT),
        PvpPeLayoutDlgProc,
        NULL
        );

    // Links page
    PvCreateTabSection(
        L"Links",
        PV_SECTION_ICON_LINKS,
        PhInstanceHandle,
        MAKEINTRESOURCE(IDD_PELINKS),
        PvpPeLinksDlgProc,
        NULL
        );

    // Processes page
    PvCreateTabSection(
        L"Processes",
        PV_SECTION_ICON_PROCESSES,
        PhInstanceHandle,
        MAKEINTRESOURCE(IDD_PIDS),
        PvpPeProcessesDlgProc,
        NULL
        );

    // Hashes page
    PvCreateTabSection(
        L"Hashes",
        PV_SECTION_ICON_HASHES,
        PhInstanceHandle,
        MAKEINTRESOURCE(IDD_PEHASHES),
        PvpPeHashesDlgProc,
        NULL
        );

    // Text preview page
    //PvCreateTabSection(
    //    L"Preview",
    //    PV_SECTION_ICON_PREVIEW,
    //    PhInstanceHandle,
    //    MAKEINTRESOURCE(IDD_PEPREVIEW),
    //    PvpPePreviewDlgProc,
    //    NULL
    //    );

    // Symbols page
    PvCreateTabSection(
        L"Symbols",
        PV_SECTION_ICON_SYMBOLS,
        PhInstanceHandle,
        MAKEINTRESOURCE(IDD_PESYMBOLS),
        PvpSymbolsDlgProc,
        NULL
        );

    // Strings page
    PvCreateTabSection(
        L"Strings",
        PV_SECTION_ICON_STRINGS,
        PhInstanceHandle,
        MAKEINTRESOURCE(IDD_STRINGS),
        PvStringsDlgProc,
        NULL
        );

    // VS_VERSIONINFO page
    PvCreateTabSection(
        L"Version",
        PV_SECTION_ICON_VERSION,
        PhInstanceHandle,
        MAKEINTRESOURCE(IDD_PEVERSIONINFO),
        PvpPeVersionInfoDlgProc,
        NULL
        );

    // Mappings page
    if (KphLevelEx(FALSE) >= KphLevelMed)
    {
        PvCreateTabSection(
            L"Mappings",
            PV_SECTION_ICON_LAYOUT,
            PhInstanceHandle,
            MAKEINTRESOURCE(IDD_PERELOCATIONS),
            PvpMappingsDlgProc,
            NULL
            );
    }

    // MUI page
    PvCreateTabSection(
        L"MUI",
        PV_SECTION_ICON_MUI,
        PhInstanceHandle,
        MAKEINTRESOURCE(IDD_PEVERSIONINFO),
        PvpPeMuiResourceDlgProc,
        NULL
        );

    // LoadLibrary page
    PvCreateTabSection(
        L"GetLoadLibrary",
        PV_SECTION_ICON_GETLOADLIBRARY,
        PhInstanceHandle,
        MAKEINTRESOURCE(IDD_GETLOADLIBRARY),
        PvGetLoadLibraryDlgProc,
        NULL
        );

    // ProcAddress page
    PvCreateTabSection(
        L"GetProcAddress",
        PV_SECTION_ICON_GETPROCADDR,
        PhInstanceHandle,
        MAKEINTRESOURCE(IDD_GETPROCADDR),
        PvGetProcAddressDlgProc,
        NULL
        );

    if (PhGetIntegerSetting(L"MainWindowPageRestoreEnabled"))
    {
        PPH_STRING startPage;
        PPV_WINDOW_SECTION startSection;
        BOOLEAN foundStartPage = FALSE;

        if (startPage = PhGetStringSetting(L"MainWindowPage"))
        {
            if (startSection = PvFindTabSectionByName(&startPage->sr))
            {
                TreeView_SelectItem(PvTabTreeControl, startSection->TreeItemHandle);
                foundStartPage = TRUE;
            }

            PhDereferenceObject(startPage);
        }

        if (!foundStartPage)
        {
            TreeView_SelectItem(PvTabTreeControl, section->TreeItemHandle);
        }

        SetFocus(PvTabTreeControl);
    }
    else
    {
        TreeView_SelectItem(PvTabTreeControl, section->TreeItemHandle);
        SetFocus(PvTabTreeControl);
        //PvEnterTabSectionView(section);
    }

    PvTabWindowOnSize();
}

INT_PTR CALLBACK PvTabWindowDialogProc(
    _In_ HWND hwndDlg,
    _In_ UINT uMsg,
    _In_ WPARAM wParam,
    _In_ LPARAM lParam
    )
{
    switch (uMsg)
    {
    case WM_INITDIALOG:
        {
            PvTabTreeControl = GetDlgItem(hwndDlg, IDC_SECTIONTREE);
            PvTabSplitterControl = GetDlgItem(hwndDlg, IDC_SECTION_SPLITTER);
            PvTabContainerControl = GetDlgItem(hwndDlg, IDD_CONTAINER);

            // The setting is stored DPI-independent (96dpi); scale it for this window. (dmex)
            PvTabSidebarWidth = (LONG)PhGetIntegerSetting(L"PeViewSidebarWidth");
            if (PvTabSidebarWidth < PV_SIDEBAR_MINIMUM_WIDTH)
                PvTabSidebarWidth = PV_SIDEBAR_DEFAULT_WIDTH;
            PvTabWindowDpi = PhGetWindowDpi(hwndDlg);
            PvTabSidebarWidth = PhMultiplyDivideSigned(PvTabSidebarWidth, PvTabWindowDpi, USER_DEFAULT_SCREEN_DPI);
            PhSetWindowProcedure(PvTabSplitterControl, PvSplitterWindowProc);

            PvTabContainerDefaultWindowProc = PhGetWindowProcedure(PvTabContainerControl);
            PhSetWindowProcedure(PvTabContainerControl, PvContainerWindowProc);

            PhSetWindowText(hwndDlg, PhaFormatString(L"%s Properties", PhGetString(PvFileName))->Buffer);

            // Without WS_CLIPCHILDREN the dialog erases its whole client area first
            // and the sidebar, splitter and container then paint over it, so every
            // splitter move flashes the background through. The clip styles now come
            // from the IDD_TABWINDOW template. (dmex)

            PvThemeInitializePageDialog(hwndDlg, PhEnableThemeSupport);
            PvConfigureTabSidebar(hwndDlg);

            PhInitializeLayoutManager(&PvTabWindowLayoutManager, hwndDlg);
            PhAddLayoutItem(&PvTabWindowLayoutManager, GetDlgItem(hwndDlg, IDC_OPTIONS), NULL, PH_ANCHOR_LEFT | PH_ANCHOR_BOTTOM);
            PhAddLayoutItem(&PvTabWindowLayoutManager, GetDlgItem(hwndDlg, IDC_SECURITY), NULL, PH_ANCHOR_LEFT | PH_ANCHOR_BOTTOM);
            PhAddLayoutItem(&PvTabWindowLayoutManager, GetDlgItem(hwndDlg, IDOK), NULL, PH_ANCHOR_RIGHT | PH_ANCHOR_BOTTOM);

            PvLayoutTabWindow(hwndDlg);

            {
                if (!PhExtractIcon(PvFileName->Buffer, &PvImageLargeIcon, &PvImageSmallIcon))
                {
                    PhGetStockApplicationIcon(&PvImageSmallIcon, &PvImageLargeIcon, PhGetWindowDpi(hwndDlg));
                }

                //SendMessage(hwndDlg, WM_SETICON, ICON_SMALL, (LPARAM)PvImageSmallIcon);
                //SendMessage(hwndDlg, WM_SETICON, ICON_BIG, (LPARAM)PvImageLargeIcon);
            }

            if (PvpLoadDbgHelp(&PvSymbolProvider))
            {
                PPH_STRING fileName;

                if (NT_SUCCESS(PhGetProcessMappedFileName(NtCurrentProcess(), PvMappedImage.ViewBase, &fileName)))
                {
                    if (PvMappedImage.Magic == IMAGE_NT_OPTIONAL_HDR32_MAGIC)
                    {
                        PhLoadModuleSymbolProvider(
                            PvSymbolProvider,
                            fileName,
                            PTR_ADD_OFFSET(UlongToPtr(PvMappedImage.NtHeaders32->OptionalHeader.ImageBase), 0),
                            PvMappedImage.NtHeaders32->OptionalHeader.SizeOfImage
                            );
                    }
                    else
                    {
                        PhLoadModuleSymbolProvider(
                            PvSymbolProvider,
                            fileName,
                            PTR_ADD_OFFSET((ULONG_PTR)PvMappedImage.NtHeaders->OptionalHeader.ImageBase, 0),
                            PvMappedImage.NtHeaders->OptionalHeader.SizeOfImage
                            );
                    }

                    PhDereferenceObject(fileName);
                }

                PhLoadModulesForVirtualSymbolProvider(PvSymbolProvider, NtCurrentProcessId(), NtCurrentProcess());
            }

            PvAddTreeViewSections();

            if (PhGetIntegerPairSetting(L"MainWindowPosition").X)
                PhLoadWindowPlacementFromSetting(L"MainWindowPosition", L"MainWindowSize", hwndDlg);
            else
                PhCenterWindow(hwndDlg, NULL);

            PvLayoutTabWindow(hwndDlg);
        }
        break;
    case WM_DESTROY:
        {
            ULONG i;
            PPV_WINDOW_SECTION section;

            PhSaveWindowPlacementToSetting(L"MainWindowPosition", L"MainWindowSize", hwndDlg);
            PhSetIntegerSetting(L"PeViewSidebarWidth", PhMultiplyDivideSigned(
                PvTabSidebarWidth, USER_DEFAULT_SCREEN_DPI, PvTabWindowDpi));
            PvSaveWindowState(hwndDlg);

            if (PhGetIntegerSetting(L"MainWindowPageRestoreEnabled"))
                PhSetStringSetting2(L"MainWindowPage", &PvTabCurrentSection->Name);

            PhDeleteLayoutManager(&PvTabWindowLayoutManager);

            PvThemeRemoveButtonGlyph(GetDlgItem(hwndDlg, IDC_OPTIONS));
            PvThemeRemoveButtonGlyph(GetDlgItem(hwndDlg, IDC_SECURITY));

            for (i = 0; i < PvTabSectionList->Count; i++)
            {
                section = PvTabSectionList->Items[i];
                PvDestroyTabSection(section);
            }

            PhDereferenceObject(PvTabSectionList);
            PvTabSectionList = NULL;

            PvDeleteTheme();

            PostQuitMessage(0);
        }
        break;
    case WM_DPICHANGED_AFTERPARENT:
        {
            LONG newDpi = LOWORD(wParam);

            if (PvTabWindowDpi && newDpi && newDpi != PvTabWindowDpi)
            {
                PvTabSidebarWidth = PhMultiplyDivideSigned(PvTabSidebarWidth, newDpi, PvTabWindowDpi);
                PvTabWindowDpi = newDpi;
            }

            PhLayoutManagerUpdate(&PvTabWindowLayoutManager, LOWORD(wParam));
            PhLayoutManagerLayout(&PvTabWindowLayoutManager);
            PvLayoutTabWindow(hwndDlg);

            PvConfigureTabSidebar(hwndDlg);
        }
        break;
    case WM_ERASEBKGND:
        {
            if (PvThemeEraseBackground(hwndDlg, (HDC)wParam))
                return TRUE;
        }
        break;
    case WM_CTLCOLORDLG:
    case WM_CTLCOLORBTN:
    case WM_CTLCOLORSTATIC:
        {
            HBRUSH brush;

            if (brush = PvThemeHandleCtlColor((HDC)wParam, uMsg == WM_CTLCOLORSTATIC ? TRUE : FALSE))
                return (INT_PTR)brush;
        }
        break;
    case WM_SETTINGCHANGE:
    case WM_THEMECHANGED:
        {
            // Re-resolve the palette so automatic mode follows the new
            // Windows preference, then refresh the chrome.
            PvReapplyTheme(hwndDlg);

            // PvReapplyTheme doesn't know about the splitter and the parent's
            // InvalidateRect doesn't reach child windows, so repaint it here. (dmex)
            PvInvalidateSplitter();
        }
        break;
    case WM_SIZE:
        {
            PvTabWindowOnSize();
        }
        break;
    case WM_PV_SPLITTER:
        {
            RECT rect;
            LONG dpi;

            dpi = PhGetWindowDpi(hwndDlg);

            GetClientRect(hwndDlg, &rect);

            // Signed integer; the cursor can leave the client area. (dmex)
            PvTabSidebarWidth = PvTabClampSidebarWidth(GET_X_LPARAM(lParam), dpi, rect.right);
            PvLayoutTabWindow(hwndDlg);

            // Flush the paints the moves above just queued so the split tracks the
            // cursor instead of lagging behind it. Deliberately no RDW_INVALIDATE or
            // RDW_ERASE: invalidating the whole window would repaint everything on
            // every mouse move, and the erase would flash the dialog background
            // underneath the controls before they redraw. Only what the moves
            // actually invalidated needs to come back. (dmex)
            RedrawWindow(
                hwndDlg,
                NULL,
                NULL,
                RDW_UPDATENOW | RDW_ALLCHILDREN
                );
        }
        break;
    case WM_COMMAND:
        {
            switch (GET_WM_COMMAND_ID(wParam, lParam))
            {
            case IDCANCEL:
            case IDOK:
                DestroyWindow(hwndDlg);
                break;
            case IDC_OPTIONS:
                {
                    PvShowOptionsWindow(hwndDlg);
                }
                break;
            case IDC_SECURITY:
                {
                    PhEditSecurity(
                        hwndDlg,
                        PhGetString(PvFileName),
                        L"FileObject",
                        PhpOpenFileSecurity,
                        PhpCloseFileSecurity,
                        NULL
                        );
                }
                break;
            }
        }
        break;
    case WM_DRAWITEM:
        {
            PDRAWITEMSTRUCT drawInfo = (PDRAWITEMSTRUCT)lParam;

            //if (drawInfo->CtlID == IDC_SEPARATOR)
            //{
            //    RECT rect;
            //
            //    rect = drawInfo->rcItem;
            //    rect.right = 2;
            //
            //    if (PhEnableThemeSupport)
            //    {
            //        switch (PhCsGraphColorMode)
            //        {
            //        case 0: // New colors
            //            {
            //                FillRect(drawInfo->hDC, &rect, GetSysColorBrush(COLOR_3DHIGHLIGHT));
            //                rect.left += 1;
            //                FillRect(drawInfo->hDC, &rect, GetSysColorBrush(COLOR_3DSHADOW));
            //            }
            //            break;
            //        case 1: // Old colors
            //            {
            //                SetDCBrushColor(drawInfo->hDC, RGB(0, 0, 0));
            //                FillRect(drawInfo->hDC, &rect, PhGetStockBrush(DC_BRUSH));
            //            }
            //            break;
            //        }
            //    }
            //    else
            //    {
            //        FillRect(drawInfo->hDC, &rect, GetSysColorBrush(COLOR_3DHIGHLIGHT));
            //        rect.left += 1;
            //        FillRect(drawInfo->hDC, &rect, GetSysColorBrush(COLOR_3DSHADOW));
            //    }
            //
            //    return TRUE;
            //}
        }
        break;
    case WM_NOTIFY:
        {
            LPNMHDR header = (LPNMHDR)lParam;

            switch (header->code)
            {
            case TVN_KEYDOWN:
                {
                    LPNMTVKEYDOWN keydown = (LPNMTVKEYDOWN)lParam;

                    if (keydown->wVKey == 'K' && GetKeyState(VK_CONTROL) < 0)
                    {
                        PPV_WINDOW_SECTION section;

                        if (section = PvGetSelectedTabSection(NULL))
                            SendMessage(section->DialogHandle, WM_KEYDOWN, keydown->wVKey, 0);
                    }
                }
                break;
            case TVN_SELCHANGED:
                {
                    LPNMTREEVIEW treeview = (LPNMTREEVIEW)lParam;
                    PPV_WINDOW_SECTION section;

                    if (section = PvGetSelectedTabSection(treeview->itemNew.hItem))
                    {
                        PvEnterTabSectionView(section);
                    }
                }
                break;
            case NM_CUSTOMDRAW:
                {
                    // phlib's theme procedure ignores SysTreeView32, so the sidebar
                    // custom draw reaches the dialog procedure normally. (dmex)
                    if (header->hwndFrom == PvTabTreeControl && PvThemeEnabled())
                    {
                        LRESULT result = PvThemeDrawSidebarItem((LPNMTVCUSTOMDRAW)lParam);

                        SetWindowLongPtr(hwndDlg, DWLP_MSGRESULT, result);
                        return TRUE;
                    }
                }
                break;
            case NM_SETCURSOR:
                {
                    if (header->hwndFrom == PvTabTreeControl)
                    {
                        PhSetCursor(PhLoadArrowCursor());

                        SetWindowLongPtr(hwndDlg, DWLP_MSGRESULT, TRUE);
                        return TRUE;
                    }
                }
                break;
            }
        }
        break;
    }

    return FALSE;
}

VOID PvTabWindowOnSize(
    VOID
    )
{
    PhLayoutManagerLayout(&PvTabWindowLayoutManager);
    PvLayoutTabWindow(PvPropertiesWindowHandle);

    /* Keep the shell controls visible after theme/layout initialization. */
    ShowWindow(PvTabTreeControl, SW_SHOW);
    ShowWindow(PvTabContainerControl, SW_SHOW);
    ShowWindow(GetDlgItem(PvPropertiesWindowHandle, IDC_OPTIONS), SW_SHOW);
    ShowWindow(GetDlgItem(PvPropertiesWindowHandle, IDC_SECURITY), SW_SHOW);
    ShowWindow(GetDlgItem(PvPropertiesWindowHandle, IDOK), SW_SHOW);

    if (PvTabSectionList && PvTabSectionList->Count != 0)
    {
        PvLayoutTabSectionView();
    }
}

VOID PvLayoutTabSectionView(
    VOID
    )
{
    if (PvTabCurrentSection && PvTabCurrentSection->DialogHandle)
    {
        RECT clientRect;

        GetClientRect(PvTabContainerControl, &clientRect);

        SetWindowPos(
            PvTabCurrentSection->DialogHandle,
            NULL,
            0,
            0,
            clientRect.right - clientRect.left,
            clientRect.bottom - clientRect.top,
            SWP_NOACTIVATE | SWP_NOZORDER
            );
    }
}

VOID PvEnterTabSectionView(
    _In_ PPV_WINDOW_SECTION NewSection
    )
{
    ULONG i;
    PPV_WINDOW_SECTION section;
    PPV_WINDOW_SECTION oldSection;
    HDWP containerDeferHandle;

    if (PvTabCurrentSection == NewSection)
        return;

    oldSection = PvTabCurrentSection;
    PvTabCurrentSection = NewSection;

    // Create and lay out the new section while it is still hidden so the first
    // visible frame is final, then hide the previous sections in a single batch.
    PvEnterTabSectionViewInner(NewSection, NULL);
    PvLayoutTabSectionView();

    containerDeferHandle = BeginDeferWindowPos(PvTabSectionList->Count);

    for (i = 0; i < PvTabSectionList->Count; i++)
    {
        section = PvTabSectionList->Items[i];

        if (section != NewSection)
            PvEnterTabSectionViewInner(section, &containerDeferHandle);
    }

    EndDeferWindowPos(containerDeferHandle);

    // Present the first frame in a single synchronous pass. WS_CLIPCHILDREN stops
    // the page priming the area under its controls, so an ungated SW_SHOW lets each
    // child's default erase-on-show reach the screen before its content paints.
    // Suppressing redraw across the show keeps that transient off-screen; the
    // RedrawWindow below then flushes one clean erase+paint cascade. (dmex)
    if (NewSection->DialogHandle)
    {
        SendMessage(NewSection->DialogHandle, WM_SETREDRAW, FALSE, 0);
        ShowWindow(NewSection->DialogHandle, SW_SHOW);
        SendMessage(NewSection->DialogHandle, WM_SETREDRAW, TRUE, 0);

        RedrawWindow(NewSection->DialogHandle, NULL, NULL, RDW_ERASE | RDW_INVALIDATE | RDW_ALLCHILDREN | RDW_UPDATENOW);
    }
}

VOID PvEnterTabSectionViewInner(
    _In_ PPV_WINDOW_SECTION Section,
    _Inout_opt_ HDWP *ContainerDeferHandle
    )
{
    if (Section == PvTabCurrentSection && !Section->DialogHandle)
        PvCreateTabSectionDialog(Section);

    // The current section is shown by the caller once it has been laid out.
    if (Section != PvTabCurrentSection && Section->DialogHandle && ContainerDeferHandle)
    {
        *ContainerDeferHandle = DeferWindowPos(*ContainerDeferHandle, Section->DialogHandle, NULL, 0, 0, 0, 0, SWP_HIDEWINDOW_ONLY | SWP_NOREDRAW);
    }
}

VOID PvCreateTabSectionDialog(
    _In_ PPV_WINDOW_SECTION Section
    )
{
    // WS_CLIPCHILDREN so a page never erases its client area underneath its own
    // controls. Pages that are a single full-size list never showed this, but the
    // General page has a group box overlapping its edit controls and flashed the
    // background between them on every resize. WS_CLIPSIBLINGS because every
    // section dialog stays created and they are all siblings inside the container.
    // The page is created hidden so the caller can lay it out before the first
    // frame reaches the screen. (dmex)
    Section->DialogHandle = PhCreateDialogFromTemplate(
        PvTabContainerControl,
        DS_SETFONT | DS_FIXEDSYS | DS_CONTROL | WS_CHILD | WS_CLIPSIBLINGS | WS_CLIPCHILDREN,
        Section->Instance,
        Section->Template,
        Section->DialogProc,
        Section->Parameter
        );

    if (!Section->DialogHandle)
        return;

    // WS_EX_CONTROLPARENT makes tab navigation traverse into the page.
    PhSetWindowExStyle(Section->DialogHandle, WS_EX_CONTROLPARENT, WS_EX_CONTROLPARENT);

    PvThemeInitializePageDialog(Section->DialogHandle, TRUE);
}

HTREEITEM PvTreeViewInsertItem(
    _In_opt_ HTREEITEM HandleInsertAfter,
    _In_ PWSTR Text,
    _In_ PVOID Context,
    _In_ INT IconIndex
    )
{
    TV_INSERTSTRUCT insert;

    memset(&insert, 0, sizeof(TV_INSERTSTRUCT));
    insert.hParent = TVI_ROOT;
    insert.hInsertAfter = HandleInsertAfter;
    insert.item.mask = TVIF_TEXT | TVIF_PARAM | TVIF_IMAGE | TVIF_SELECTEDIMAGE;
    insert.item.pszText = Text;
    insert.item.lParam = (LPARAM)Context;
    insert.item.iImage = IconIndex;
    insert.item.iSelectedImage = IconIndex;

    return TreeView_InsertItem(PvTabTreeControl, &insert);
}

PPV_WINDOW_SECTION PvGetSelectedTabSection(
    _In_opt_ PVOID TreeItemHandle
    )
{
    TVITEM item;
    HTREEITEM itemHandle;

    if (TreeItemHandle)
        itemHandle = TreeItemHandle;
    else
        itemHandle = TreeView_GetSelection(PvTabTreeControl);

    memset(&item, 0, sizeof(TVITEM));
    item.mask = TVIF_PARAM | TVIF_HANDLE;
    item.hItem = itemHandle;

    if (!TreeView_GetItem(PvTabTreeControl, &item))
        return NULL;

    return (PPV_WINDOW_SECTION)item.lParam;
}

PPV_WINDOW_SECTION PvCreateTabSection(
    _In_ PWSTR Name,
    _In_ INT IconIndex,
    _In_ PVOID Instance,
    _In_ PWSTR Template,
    _In_ DLGPROC DialogProc,
    _In_opt_ PVOID Parameter
    )
{
    PPV_WINDOW_SECTION section;

    section = PhAllocateZero(sizeof(PV_WINDOW_SECTION));
    PhInitializeStringRefLongHint(&section->Name, Name);
    section->Instance = Instance;
    section->Template = Template;
    section->DialogProc = DialogProc;
    section->Parameter = Parameter;
    section->IconIndex = IconIndex;
    section->TreeItemHandle = PvTreeViewInsertItem(TVI_LAST, Name, section, IconIndex);

    PhAddItemList(PvTabSectionList, section);

    return section;
}

VOID PvDestroyTabSection(
    _In_ PPV_WINDOW_SECTION Section
    )
{
    PhFree(Section);
}

PPV_WINDOW_SECTION PvFindTabSectionByName(
    _In_ PPH_STRINGREF Name
    )
{
    ULONG i;
    PPV_WINDOW_SECTION section;

    for (i = 0; i < PvTabSectionList->Count; i++)
    {
        section = PvTabSectionList->Items[i];

        if (PhEqualStringRef(&section->Name, Name, TRUE))
            return section;
    }

    return NULL;
}
