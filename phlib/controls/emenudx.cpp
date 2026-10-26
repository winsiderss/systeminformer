/*
 * Copyright (c) 2022 Winsider Seminars & Solutions, Inc.  All rights reserved.
 *
 * This file is part of System Informer.
 *
 * Authors:
 *
 *     dmex    2025-2026
 */

#include <ph.h>
#include <emenu.h>
#include <guisup.h>
#include <mapldr.h>
#include <treenew.h>
#include <settings.h>

// Native DirectComposition v2 menu rendering:
// D3D device -> D2D device -> IDCompositionDesktopDevice -> HWND target
//                                                       -> root visual -> menu visual -> surface
//
// Creating DirectComposition with the D2D device enables D2D batching. Each
// surface update supplies an ID2D1DeviceContext; DirectComposition owns its
// target and BeginDraw/EndDraw lifecycle. The menu is rendered at a fixed origin
// into a cached bitmap, copied to the supplied update offset in device pixels and
// committed after the surface update is complete.
//
// The DirectComposition backend builds the following visual tree:
//
//   Root Visual
//    `- Shadow Host Visual   (opacity + scale animation, shared by both children)
//        |- Shadow Visual    (rounded-rect mask blurred once with the D2D shadow effect)
//        `- Menu Visual      (D2D menu surface, offset by the shadow padding)
//
// The shadow visual is a sibling of the menu visual because an effect applies to
// the visual and its whole subtree. Both visuals live under one host so a single
// opacity/scale animation drives the shadow and the content in lock step. That
// animation is opt-in (PH_EMENU_DX_ENABLE_HOST_ANIMATION); by default the DWM
// window transition animates the whole window, shadow included.
//
// The optional WinRT backend supports backdrop brushes and drop shadows;
// the optional swap-chain backend presents its own D2D target bitmap.

//#define PH_EMENU_DX_ENABLE_WINRT_COMPOSITION
#define PH_EMENU_DX_ENABLE_HOSTBACKDROPBRUSH
//#define PH_EMENU_DX_ENABLE_SWAPCHAIN
//#define PH_EMENU_DX_ENABLE_DROPSHADOW
//#define PH_EMENU_DX_ENABLE_DROPSHADOW_DEBUG
//#define PH_EMENU_DX_ENABLE_D3D12
#define PH_EMENU_DX_ENABLE_D3D11
//#define PH_EMENU_DX_ENABLE_HOST_ANIMATION // DirectComposition open/close animation (default: DWM window transitions)

#ifdef PH_EMENU_DX_ENABLE_D3D12
#include <d3d12.h>
#include <d3d11on12.h>
#endif
#ifdef PH_EMENU_DX_ENABLE_D3D11
#include <d3d11.h>
#endif
#include <d3d10_1.h>
#include <d3d10.h>
#include <d3d11.h>
#include <d2d1_1.h>
#include <dcomp.h>
#include <dwrite.h>
#include <wincodec.h>
#include <dwmapi.h>

#include <wrl.h>
#include <algorithm>
#include <span>
#include <utility>

using Microsoft::WRL::ComPtr;

#ifdef PH_EMENU_DX_ENABLE_WINRT_COMPOSITION
#include <wrl/wrappers/corewrappers.h>
#include <windows.graphics.directx.h>
#include <windows.ui.composition.h>
#include <windows.ui.composition.desktop.h>
#include <windows.ui.composition.interop.h>
#include <DispatcherQueue.h>

using namespace Microsoft::WRL;
using namespace Microsoft::WRL::Wrappers;
using namespace ABI::Windows::UI::Composition;
using namespace ABI::Windows::UI::Composition::Desktop;
using namespace ABI::Windows::Graphics::DirectX;

#ifndef __ICompositorWithBlurredWallpaperBackdropBrush_INTERFACE_DEFINED__
#define __ICompositorWithBlurredWallpaperBackdropBrush_INTERFACE_DEFINED__
MIDL_INTERFACE("0d8fb190-f122-5b8d-9fdd-543b0d8eb7f3")
ICompositorWithBlurredWallpaperBackdropBrush : public IInspectable
{
public:
    virtual HRESULT STDMETHODCALLTYPE TryCreateBlurredWallpaperBackdropBrush(
        /* [out][retval] */ ICompositionBackdropBrush** result) = 0;
};
#endif
#endif

// PhShowEMenu links this file into every image that uses phlib menus, so the
// default D3D11/D3D10.1, D2D, DWrite, DWM and DComp entry points are resolved
// at runtime (see the Dynamic* helpers) rather than imported statically.
#ifdef PH_EMENU_DX_ENABLE_D3D12
#pragma comment(lib, "d3d12.lib")
#pragma comment(lib, "d3d11.lib")
#endif
#ifdef PH_EMENU_DX_ENABLE_WINRT_COMPOSITION
#pragma comment(lib, "windowsapp.lib")
#endif

// Private show flag; must not overlap PH_EMENU_SHOW_* in emenu.h.
#define PH_EMENU_SHOW_NO_ROUND_CORNERS 0x80000000

BOOLEAN PhEnableEMenuDxCompact = TRUE;

// Layout metrics in DIPs. The default set matches the modern Windows 11 context menu;
// the compact set (PhEnableEMenuDxCompact) matches the classic Windows 11 menu.
typedef struct _PH_EMENU_DX_METRICS
{
    FLOAT ItemHeight;
    FLOAT SeparatorHeight;
    FLOAT SeparatorWidth; // separator line thickness, in hairlines (device pixels at 100-199%)
    FLOAT SeparatorInset; // separator line inset from the left and right edges
    FLOAT VerticalPadding; // above the first and below the last item
    FLOAT MinWidth;
    FLOAT IconLeft; // left edge to the icon/check box
    FLOAT IconSize;
    FLOAT TextLeft; // left edge to the label
    FLOAT TextShortcutGap;
    FLOAT ShortcutRightMargin; // right edge to the end of the shortcut text
    FLOAT ArrowRightMargin; // right edge to the base of the submenu arrow
    FLOAT ArrowTipMargin; // right edge to the tip of the submenu arrow
    FLOAT RoundedCorner;
    FLOAT HoverMargin;
    FLOAT HoverCorner;
    FLOAT TextFontSize;
    FLOAT ShortcutFontSize;
    BOOLEAN ShortcutColorIsText; // shortcuts in the label color instead of grey
} PH_EMENU_DX_METRICS, *PPH_EMENU_DX_METRICS;

typedef enum _PH_EMENU_DX_BACKDROP_TYPE : ULONG
{
    PhEMenuDxBackdropNone,
    PhEMenuDxBackdropStandard, // CreateBackdropBrush (Win10 v1607+)
    PhEMenuDxBackdropHost,     // CreateHostBackdropBrush (Win10 v1703+)
    PhEMenuDxBackdropMica      // TryCreateBlurredWallpaperBackdropBrush (Win11 v21H2+)
} PH_EMENU_DX_BACKDROP_TYPE;

class PH_EMENU_DX;
typedef PH_EMENU_DX* PPH_EMENU_DX;

class PH_EMENU_DX
{
public:
    PH_EMENU_DX(_In_ const PH_EMENU_DX& Other) = delete;
    PH_EMENU_DX& operator=(_In_ const PH_EMENU_DX& Other) = delete;

    PH_EMENU_DX(
        _In_ PPH_EMENU Menu,
        _In_ ULONG Flags,
        _In_ ULONG Align,
        _In_ BOOLEAN DarkMode,
        _In_opt_ PPH_EMENU_DX ParentContext
        );

    ~PH_EMENU_DX();

    static BOOLEAN ShowInternal(
        _In_ PPH_EMENU Menu,
        _In_ HWND WindowHandle,
        _In_ ULONG Flags,
        _In_ ULONG Align,
        _In_ LONG X,
        _In_ LONG Y,
        _In_ BOOLEAN DarkMode,
        _In_opt_ PPH_EMENU_DX ParentContext,
        _Out_ PPH_EMENU_ITEM* SelectedItem
        );

    static VOID UnregisterWindowClass();

    static VOID CancelThreadMenus();

    static HBITMAP CreateGlyphBitmap(
        _In_ PCWSTR Glyph,
        _In_ BOOLEAN ColorFont,
        _In_ COLORREF Color,
        _In_ LONG Size
        );

    static BOOLEAN IsMenuSupported(
        _In_ PPH_EMENU Menu
        );

private:
    static LRESULT CALLBACK WindowProc(
        _In_ HWND WindowHandle,
        _In_ UINT WindowMessage,
        _In_ WPARAM wParam,
        _In_ LPARAM lParam
        );

    BOOLEAN Run(
        _In_ HWND OwnerWindow,
        _In_ LONG X,
        _In_ LONG Y,
        _Out_ PPH_EMENU_ITEM* SelectedItem
        );

    HRESULT BeforeEnteringMessageLoop(
        _In_ HWND OwnerWindow,
        _In_ LONG X,
        _In_ LONG Y
        );

    BOOLEAN EnterMessageLoop();

    VOID AfterLeavingMessageLoop();

    HRESULT CreateMainWindow(
        _In_ HWND OwnerWindow,
        _In_ LONG X,
        _In_ LONG Y
        );

    HRESULT ShowMainWindow();
    VOID DestroyMainWindow();

    HRESULT CreateD3DDevice();
    VOID DestroyD3DDevice();

    HRESULT CreateD2DDevice();
    VOID DestroyD2DDevice();

    HRESULT CreateDWriteFactory();
    VOID DestroyDWriteFactory();

    HRESULT CreateDCompDevice();
    VOID DestroyDCompDevice();

    HRESULT CreateResources();
    VOID ReleaseResources();

    HRESULT CreateDrawingResources();
    VOID DestroyDrawingResources();

    HRESULT InitializeTextFormats();
    HRESULT InitializeBrushes();

    HRESULT CreateDCompVisualTree();
    VOID DestroyDCompVisualTree();

    HRESULT CreateDCompTarget();
    VOID DestroyDCompTarget();

    HRESULT CreateDCompRootVisual();
    VOID DestroyDCompRootVisual();

    HRESULT CreateDCompMenuVisual();
    VOID DestroyDCompMenuVisual();

    HRESULT CreateDCompMenuSurface();
    VOID DestroyDCompMenuSurface();

    HRESULT DrawDCompMenuSurface();

    HRESULT BindDCompRootVisualToTarget();
    HRESULT BindDCompMenuSurfaceToVisual();

    HRESULT SetBackfaceVisibilityOnDCompMenuVisual();
    HRESULT AddDCompMenuVisualToRootVisual();

    VOID InitializeShadow();

    LONG GetSurfacePaddingPixels();

    HRESULT CreateDCompShadowTree();
    VOID DestroyDCompShadowTree();

    HRESULT DrawDCompShadowSurface();

    VOID ExcludeOccludersFromShadow(
        _In_ ID2D1DeviceContext* DrawingContext,
        _In_ POINT Offset
        );

    VOID StartOpenAnimation();
    VOID PlayCloseAnimation();

    static BOOLEAN IsPointInMenuBody(
        _In_ POINT Point
        );

    HRESULT CommitDCompDevice();

    VOID CommitUpdate();

    VOID Render();

    VOID CalculateLayout();

    VOID CalculatePosition(
        _In_ LONG X,
        _In_ LONG Y,
        _Out_ PLONG OutX,
        _Out_ PLONG OutY
        );

    VOID ReleaseCachedLayouts();

    HRESULT CreateTextLayout(
        _In_ PWSTR Text,
        _In_ SIZE_T TextLen,
        _In_ IDWriteTextFormat* Format,
        _In_ FLOAT MaxWidth,
        _In_ FLOAT MaxHeight,
        _Out_ IDWriteTextLayout** Layout
        ) const;

    HRESULT CreateBitmapFromIcon(
        _In_ HICON Icon,
        _Out_ ID2D1Bitmap** Bitmap
        ) const;

    HRESULT CreateBitmapFromHBitmap(
        _In_ HBITMAP BitmapHandle,
        _Out_ ID2D1Bitmap** Bitmap
        ) const;

    VOID RebuildTextLayouts();

    BOOLEAN CanScrollUp();
    BOOLEAN CanScrollDown();

    FLOAT GetItemTop(
        _In_ ULONG Index
        );

    VOID EnsureItemVisible(
        _In_ ULONG Index
        );

    VOID SetHoverIndex(
        _In_ ULONG Index
        );

    VOID ActivateHoverItem();

    static WCHAR GetItemMnemonic(
        _In_ PPH_EMENU_ITEM Item
        );

    HRESULT OpenSubMenu();

    VOID CloseSubMenuChain();
    VOID DestroySubMenuChain();

    VOID HandleMouseMove(
        _In_ HWND WindowHandle,
        _In_ POINT pt
        );

    static LRESULT OnNcCreate(
        _In_ HWND WindowHandle,
        _In_ CREATESTRUCT* CreateStruct
        );

    LRESULT OnCreate(
        _In_ HWND WindowHandle
        );

    VOID OnDestroy(
        _In_ HWND WindowHandle
        );

    VOID OnPaint(
        _In_ HWND WindowHandle
        );

    BOOLEAN OnSetCursor(
        _In_ HWND WindowHandle,
        _In_ HWND CursorWindowHandle,
        _In_ ULONG HitTest,
        _In_ ULONG WindowMessage
        );

    VOID OnMouseWheel(
        _In_ HWND WindowHandle,
        _In_ SHORT WheelDelta
        );

    VOID OnMouseMove(
        _In_ HWND WindowHandle,
        _In_ LONG CursorX,
        _In_ LONG CursorY
        );

    VOID OnMouseLeave(
        _In_ HWND WindowHandle
        );

    VOID OnTimer(
        _In_ HWND WindowHandle,
        _In_ ULONG TimerId
        );

    VOID OnLButtonUp(
        _In_ HWND WindowHandle,
        _In_ LONG CursorX,
        _In_ LONG CursorY
        );

    VOID OnKeyDown(
        _In_ HWND WindowHandle,
        _In_ ULONG VirtualKey
        );

    VOID OnSysKeyDown(
        _In_ HWND WindowHandle,
        _In_ ULONG VirtualKey,
        _In_ BOOLEAN Repeat
        );

    BOOLEAN OnChar(
        _In_ HWND WindowHandle,
        _In_ WCHAR Character
        );

    VOID OnActivateApp(
        _In_ HWND WindowHandle,
        _In_ BOOLEAN Active
        );

    LRESULT OnActivate(
        _In_ HWND WindowHandle,
        _In_ ULONG Type,
        _In_ HWND OtherWindow
        );

    VOID OnCancelMode(
        _In_ HWND WindowHandle
        );

    VOID OnUpdateUiState(
        _In_ HWND WindowHandle,
        _In_ USHORT Action,
        _In_ USHORT Flags
        );

    static LRESULT CALLBACK MouseHookCallback(
        _In_ INT nCode,
        _In_ WPARAM wParam,
        _In_ LPARAM lParam
        );

    static HRESULT CompositionCreateDevice(
        _In_opt_ IUnknown* RenderingDevice,
        _In_ REFIID Iid,
        _COM_Outptr_ void** DCompositionDevice
        );

    static PVOID GetDynamicProcedure(
        _Inout_ PVOID* CachedProcedure,
        _In_ PCWSTR LibraryName,
        _In_ PCSTR ProcedureName
        );

    static HRESULT DynamicD3D11CreateDevice(
        _In_opt_ IDXGIAdapter* Adapter,
        _In_ D3D_DRIVER_TYPE DriverType,
        _In_ UINT Flags,
        _Out_ ID3D11Device** Device,
        _Out_opt_ D3D_FEATURE_LEVEL* FeatureLevel,
        _Out_opt_ ID3D11DeviceContext** ImmediateContext
        );

    static HRESULT DynamicD3D10CreateDevice1(
        _In_ D3D10_DRIVER_TYPE DriverType,
        _In_ UINT Flags,
        _In_ D3D10_FEATURE_LEVEL1 HardwareLevel,
        _Out_ ID3D10Device1** Device
        );

    static HRESULT DynamicD2D1CreateFactory(
        _In_ D2D1_FACTORY_TYPE FactoryType,
        _In_ REFIID Iid,
        _In_opt_ const D2D1_FACTORY_OPTIONS* FactoryOptions,
        _Out_ void** Factory
        );

    static HRESULT DynamicDWriteCreateFactory(
        _In_ DWRITE_FACTORY_TYPE FactoryType,
        _In_ REFIID Iid,
        _Out_ IUnknown** Factory
        );

    static HRESULT DynamicDwmExtendFrameIntoClientArea(
        _In_ HWND WindowHandle,
        _In_ const MARGINS* Margins
        );

    static VOID ResetGlobalResources();

    static VOID ReleaseSharedResources();

    static HRESULT CreateSharedD3DDevice();
    static HRESULT CreateSharedD2DDevice();
    static HRESULT CreateSharedDCompDevice();
    static HRESULT CreateSharedTextAndImagingFactories();
    static HRESULT InitializeSharedResources();

    VOID RegisterMenuContext();
    VOID UnregisterMenuContext();

    static BOOL IsWindowInMenuChain(
        _In_opt_ HWND hwnd
        );

    static RTL_ATOM PhRegisterMenuClass();

    // Guards the Shared* objects. Menus can run on any GUI thread, and a
    // device-lost reset on one thread must not race initialization on another.
    static PH_QUEUED_LOCK SharedLock;
    static ULONG SharedGeneration;
#ifdef PH_EMENU_DX_ENABLE_D3D12
    static ID3D12Device* SharedD3D12Device;
    static ID3D12CommandQueue* SharedD3D12CommandQueue;
    static ID3D11Device* SharedD3D11Device12;
    static ID3D11On12Device* SharedD3D11On12Device;
#endif
#ifdef PH_EMENU_DX_ENABLE_D3D11
    static ID3D11Device* SharedD3D11Device;
#endif
    static ID3D10Device1* SharedD3DDevice;
    static ID2D1Factory1* SharedD2DFactory;
    static ID2D1Device* SharedD2DDevice;
#ifdef PH_EMENU_DX_ENABLE_WINRT_COMPOSITION
    static ICompositor* SharedCompositor;
#else
    static IDCompositionDevice2* SharedDCompDevice;
    static IDCompositionDesktopDevice* SharedDesktopDevice;
    static IDCompositionDevice3* SharedCompositionDevice3;
#endif
    static IDWriteFactory* SharedDWriteFactory;
    static IWICImagingFactory* SharedWicFactory;
#ifdef PH_EMENU_DX_ENABLE_WINRT_COMPOSITION
    static ICompositionGraphicsDevice* SharedCompositionGraphicsDevice;
#endif

    static PH_QUEUED_LOCK MenuListLock;
    static PPH_LIST MenuList;

    HWND WindowHandle{};
    HWND OwnerWindowHandle{}; // The window passed to PhShowEMenuDX (not GW_OWNER, which is the top-level ancestor).
    ULONG ThreadId{};
    ULONG ResourceGeneration{};

    PPH_EMENU Menu{};
    const PH_EMENU_DX_METRICS* Metrics{};

    union
    {
        ULONG States{};
        struct
        {
            ULONG ShowKeyboardCues : 1;
            ULONG IsSubMenu : 1;
            ULONG IsDarkMode : 1;
            ULONG IsAcrylic : 1;
            ULONG IsMica : 1;
            ULONG ExitLoop : 1;
            ULONG ShadowEnabled : 1;
            ULONG AnimationsEnabled : 1;
            ULONG OpenAnimationPending : 1;
            ULONG SubMenuSwitchPending : 1; // the open submenu is closing and HoverItem's submenu replaces it
            ULONG SubMenuSwitching : 1; // the submenu being created replaces a sibling
            ULONG Spare : 21;
        };
    };

    ULONG Flags{};
    ULONG Align{};

    FLOAT Width{};
    FLOAT Height{};
    FLOAT ItemHeight{}; // Metrics rounded to whole device pixels (CalculateLayout)
    FLOAT SeparatorHeight{};
    FLOAT VerticalPadding{};
    FLOAT HairlineWidth{}; // one device pixel or more, in DIPs (CalculateLayout)
    FLOAT MaxTextWidth{};
    FLOAT MaxShortcutWidth{};

    FLOAT Dpi{};
    FLOAT Scale{};
    FLOAT SurfacePadding{}; // DIPs of transparent window area reserved around the menu for the shadow

    PPH_EMENU_ITEM SelectedItem{};
    PPH_EMENU_ITEM HoverItem{};
    LONG HoverIndex{};

    BOOLEAN Registered{};
    BOOLEAN HoverUpArrow{};
    BOOLEAN HoverDownArrow{};

#ifdef PH_EMENU_DX_ENABLE_D3D12
    ID3D12Device* D3D12Device{};
    ID3D12CommandQueue* D3D12CommandQueue{};
    ID3D12CommandAllocator* D3D12CommandAllocator{};
    ID3D12GraphicsCommandList* D3D12CommandList{};
    ID3D12Fence* D3D12Fence{};
    HANDLE D3D12FenceEvent{};
    UINT64 D3D12FenceValue{};
    ID3D11Device* D3DDevice{};
    ID3D11On12Device* D3D11On12Device{};
    ID3D11DeviceContext* D3D11DeviceContext{};
#else
    IUnknown* D3DDevice{};
#endif
    ID2D1Factory1* D2DFactory{};
    ID2D1Device* D2DDevice{};
    ID2D1DeviceContext* D2DDeviceContext{};
#ifdef PH_EMENU_DX_ENABLE_SWAPCHAIN
    IDXGISwapChain1* SwapChain{};
#endif
#ifdef PH_EMENU_DX_ENABLE_WINRT_COMPOSITION
    ICompositor* Compositor{};
    IDesktopWindowTarget* CompositionTarget{};
#ifdef PH_EMENU_DX_ENABLE_SWAPCHAIN
    ICompositionSurface* CompositionSurface{};
#else
    ICompositionDrawingSurface* CompositionSurface{};
#endif
    IContainerVisual* RootVisual{};
    ISpriteVisual* MenuVisual{};
#ifdef PH_EMENU_DX_ENABLE_HOSTBACKDROPBRUSH
    ICompositionBrush* BackdropBrush{};
    ISpriteVisual* BackdropVisual{};
#endif
#ifdef PH_EMENU_DX_ENABLE_DROPSHADOW
    ISpriteVisual* ShadowVisual{};
    IUnknown* DropShadow{};
#endif
#else
    IDCompositionDevice2* DCompDevice{};
    IDCompositionTarget* DCompTarget{};
    IDCompositionSurface* DCompSurface{};
    ID2D1Bitmap1* MenuBitmap{}; // menu rendered at a fixed origin, copied into DCompSurface
    IDCompositionVisual2* RootVisual{};
    IDCompositionVisual2* MenuVisual{};
    IDCompositionVisual2* HostVisual{};
    IDCompositionVisual3* HostVisual3{};
    IDCompositionScaleTransform* HostScaleTransform{};
    IDCompositionVisual2* ShadowVisual{};
    IDCompositionSurface* ShadowSurface{};
    IDCompositionDesktopDevice* DesktopDevice{};
    IDCompositionDevice3* CompositionDevice3{};
#endif
    IDWriteFactory* DWriteFactory{};
    IDWriteTextFormat* TextFormat{};
    IDWriteTextFormat* ShortcutFormat{};

    ID2D1RoundedRectangleGeometry* ClipGeometry{};
    ID2D1Layer* ClipLayer{};

    ID2D1SolidColorBrush* TextBrush{};
    ID2D1SolidColorBrush* ShortcutBrush{};
    ID2D1SolidColorBrush* HoverBrush{};
    ID2D1SolidColorBrush* BorderBrush{};
    ID2D1SolidColorBrush* SeparatorBrush{};

    IDWriteTextLayout** TextLayouts{};
    IDWriteTextLayout** ShortcutLayouts{};
    DWRITE_TEXT_METRICS* TextMetrics{};
    DWRITE_TEXT_METRICS* ShortcutMetrics{};
    ID2D1Bitmap** ItemBitmaps{};
    IWICImagingFactory* WicFactory{};

    // Cached layouts to avoid repeated DWrite allocations during rendering.
    ULONG LayoutCount{}; // Number of entries in each cached array

    ULONG MenuShowDelay{};

    HHOOK MouseHookHandle{};

    // Scrolling members
    FLOAT ScrollOffset{}; // Current vertical scroll offset
    FLOAT ScrollViewTotalHeight{}; // Total height of all menu items, including hidden ones
    FLOAT ScrollViewVisibleHeight{}; // The maximum height the menu can occupy on screen

#if defined(PH_EMENU_DX_ENABLE_WINRT_COMPOSITION) && defined(PH_EMENU_DX_ENABLE_HOSTBACKDROPBRUSH) && !defined(PH_EMENU_DX_ENABLE_DROPSHADOW)
    PH_EMENU_DX_BACKDROP_TYPE BackdropType{};
#endif

    PH_EMENU_DX* SubMenuContext{};
    PH_EMENU_DX* ParentContext{};
};

#define PH_EMENU_DX_CLASSNAME L"PhEMenuDX"
#define PH_EMENU_DX_MIN_HEIGHT 20.0f

// DirectComposition shadow (DIPs). The effect's standard deviation is BLUR_RADIUS / 3,
// so the Gaussian is effectively zero at BLUR_RADIUS from the menu edge. PADDING must
// cover BLUR_RADIUS + OFFSET_Y so the shadow is never clipped by the window bounds.
#define PH_EMENU_DX_DC_SHADOW_BLUR_RADIUS 28.0f
#define PH_EMENU_DX_DC_SHADOW_OPACITY 0.80f
#define PH_EMENU_DX_DC_SHADOW_OPACITY_DARK 1.00f
#define PH_EMENU_DX_DC_SHADOW_OFFSET_Y 6.0f
// A Gaussian blur of a large shape is only half the source alpha at the shape edge.
// Growing the mask by SPREAD moves the 50% point outward so the shadow is dense
// right at the menu edge. PADDING must cover BLUR_RADIUS + OFFSET_Y + SPREAD.
#define PH_EMENU_DX_DC_SHADOW_SPREAD 4.0f
#define PH_EMENU_DX_DC_SHADOW_PADDING 40.0f

// Menu glyph bitmaps: the Fluent icon font (Windows 11) or MDL2 assets (Windows 10)
// for monochrome glyphs, and the emoji font for color glyphs.
#define PH_EMENU_DX_GLYPH_FONT_NAME L"Segoe Fluent Icons"
#define PH_EMENU_DX_GLYPH_FONT_NAME_LEGACY L"Segoe MDL2 Assets"
#define PH_EMENU_DX_GLYPH_EMOJI_FONT_NAME L"Segoe UI Emoji"

#define PH_EMENU_DX_DARK_BACKGROUND D2D1::ColorF(0.169f, 0.169f, 0.169f, 1.0f) // #2B2B2B, as the Windows 11 menus
#define PH_EMENU_DX_LIGHT_BACKGROUND D2D1::ColorF(0.98f, 0.98f, 0.98f, 1.0f)

#define PH_EMENU_DX_TEXT_BRUSH_OPACITY 1.0f
#define PH_EMENU_DX_SHORTCUT_BRUSH_OPACITY 1.0f
#define PH_EMENU_DX_HOVER_DARK_OPACITY 0.25f
#define PH_EMENU_DX_HOVER_LIGHT_OPACITY 0.15f
#define PH_EMENU_DX_BORDER_DARK_OPACITY 0.12f
#define PH_EMENU_DX_BORDER_LIGHT_OPACITY 0.08f
#define PH_EMENU_DX_SEPARATOR_DARK_OPACITY 0.15f
#define PH_EMENU_DX_SEPARATOR_LIGHT_OPACITY 0.15f

#define PH_EMENU_DX_TEXT_COLOR_DARK D2D1::ColorF(D2D1::ColorF::White, PH_EMENU_DX_TEXT_BRUSH_OPACITY)
#define PH_EMENU_DX_TEXT_COLOR_LIGHT D2D1::ColorF(D2D1::ColorF::Black, PH_EMENU_DX_TEXT_BRUSH_OPACITY)
#define PH_EMENU_DX_SHORTCUT_COLOR_DARK D2D1::ColorF(D2D1::ColorF::Gray, PH_EMENU_DX_SHORTCUT_BRUSH_OPACITY)
#define PH_EMENU_DX_SHORTCUT_COLOR_LIGHT D2D1::ColorF(D2D1::ColorF::DimGray, PH_EMENU_DX_SHORTCUT_BRUSH_OPACITY)
#define PH_EMENU_DX_HOVER_COLOR_DARK D2D1::ColorF(D2D1::ColorF::White, PH_EMENU_DX_HOVER_DARK_OPACITY)
#define PH_EMENU_DX_HOVER_COLOR_LIGHT D2D1::ColorF(D2D1::ColorF::Black, PH_EMENU_DX_HOVER_LIGHT_OPACITY)
#define PH_EMENU_DX_BORDER_COLOR_DARK D2D1::ColorF(D2D1::ColorF::White, PH_EMENU_DX_BORDER_DARK_OPACITY)
#define PH_EMENU_DX_BORDER_COLOR_LIGHT D2D1::ColorF(D2D1::ColorF::Black, PH_EMENU_DX_BORDER_LIGHT_OPACITY)
#define PH_EMENU_DX_SEPARATOR_COLOR_DARK D2D1::ColorF(D2D1::ColorF::White, PH_EMENU_DX_SEPARATOR_DARK_OPACITY)
#define PH_EMENU_DX_SEPARATOR_COLOR_LIGHT D2D1::ColorF(D2D1::ColorF::Black, PH_EMENU_DX_SEPARATOR_LIGHT_OPACITY)

#define PH_EMENU_DX_CHECK_LINE_WIDTH 1.5f
#define PH_EMENU_DX_RADIO_DOT_RADIUS 3.0f
#define PH_EMENU_DX_ARROW_SIZE 4.0f // check mark
#define PH_EMENU_DX_CHEVRON_SIZE 3.5f // submenu arrow half height
#define PH_EMENU_DX_SCROLL_ARROW_PADDING 8.0f
#define PH_WINDOW_TIMER_SCROLL 2

#define PH_EMENU_DX_DEFAULT_MENU_SHOW_DELAY 400
#define PH_EMENU_DX_DPI_BASE 96.0f

#define PH_EMENU_DX_DISABLED_ICON_OPACITY 0.5f
#define PH_EMENU_DX_ICON_CHECK_MARGIN 2.0f
#define PH_EMENU_DX_SUBMENU_MAX_HEIGHT_PERCENT 0.7f
#define PH_EMENU_DX_ROOT_MAX_HEIGHT_PERCENT 0.8f
#define PH_EMENU_DX_FONT_NAME L"Segoe UI"
#define PH_EMENU_DX_SCROLL_TIMER_INTERVAL 50
#define PH_EMENU_DX_MAX_TEXT_WIDTH 1000.0f

#define PH_EMENU_DX_CORNER_STYLE 2
#define PH_EMENU_DX_INITIAL_LIST_SIZE 2
#define PH_EMENU_DX_SCROLL_ARROW_SIZE 6.0f
#define PH_EMENU_DX_ARROW_SIZE_FACTOR 1.5f
#define PH_EMENU_DX_HALF_FACTOR 2.0f

#define PH_EMENU_DX_COLOR_TRANSPARENT D2D1::ColorF(0.0f, 0.0f, 0.0f, 0.0f)
#define PH_EMENU_DX_GPU_PRIORITY 16
#define PH_EMENU_DX_LOCALE_NAME L"en-us"
#define PH_EMENU_DX_RETRY_DELAY 50
#define PH_EMENU_DX_SCROLL_STEP 3
#define PH_EMENU_DX_SCROLL_SPEED_DIVISOR 2.0f
#define PH_EMENU_DX_SUBMENU_X_OFFSET 4.0f
#define PH_EMENU_DX_SUBMENU_Y_OFFSET 6.0f
#define PH_EMENU_DX_ALPHA_THRESHOLD 0.001f
#define PH_EMENU_DX_ACRYLIC_TINT_ALPHA_LEVEL 0.55f
#define PH_EMENU_DX_MICA_TINT_ALPHA_LEVEL 0.50f
#define PH_EMENU_DX_OPAQUE_ALPHA_LEVEL 1.0f
#define PH_EMENU_DX_FULL_OPACITY 1.0f
#define PH_EMENU_DX_NO_OPACITY 0.0f

// Both sets are measured from the Windows 11 menus at 100% and 150%; the classic
// menu rounds in pixels, so the compact offsets match 150% exactly and 100% within 1 px.
const PH_EMENU_DX_METRICS PhpEMenuDxDefaultMetrics =
{
    32.0f,  // ItemHeight
    9.0f,   // SeparatorHeight
    1.0f,   // SeparatorWidth
    4.0f,   // SeparatorInset
    4.0f,   // VerticalPadding
    268.0f, // MinWidth
    15.0f,  // IconLeft
    16.0f,  // IconSize
    43.0f,  // TextLeft
    32.0f,  // TextShortcutGap
    16.0f,  // ShortcutRightMargin
    21.5f,  // ArrowRightMargin
    18.0f,  // ArrowTipMargin
    8.0f,   // RoundedCorner
    4.0f,   // HoverMargin
    4.0f,   // HoverCorner
    14.0f,  // TextFontSize
    12.0f,  // ShortcutFontSize
    FALSE,  // ShortcutColorIsText
};

// The classic menu sizes itself to its content, so there is no minimum width, and
// shortcuts end in a column left of the submenu arrows in the label font and color.
const PH_EMENU_DX_METRICS PhpEMenuDxCompactMetrics =
{
    24.0f,  // ItemHeight
    7.0f,   // SeparatorHeight
    1.0f,   // SeparatorWidth
    4.0f,   // SeparatorInset
    4.0f,   // VerticalPadding
    0.0f,   // MinWidth
    10.0f,  // IconLeft
    16.0f,  // IconSize
    36.0f,  // TextLeft
    27.0f,  // TextShortcutGap
    24.0f,  // ShortcutRightMargin
    18.5f,  // ArrowRightMargin
    15.0f,  // ArrowTipMargin
    8.0f,   // RoundedCorner
    5.33f,  // HoverMargin
    4.0f,   // HoverCorner
    12.0f,  // TextFontSize (Segoe UI 9 pt)
    12.0f,  // ShortcutFontSize
    TRUE,   // ShortcutColorIsText
};

BOOLEAN PhShowEMenuDX(
    _In_ PPH_EMENU Menu,
    _In_ HWND WindowHandle,
    _In_ ULONG Flags,
    _In_ ULONG Align,
    _In_ LONG X,
    _In_ LONG Y,
    _In_ BOOLEAN DarkMode,
    _Out_ PPH_EMENU_ITEM* SelectedItem
    )
{
    *SelectedItem = nullptr;

    // Let PhShowEMenu fall back to TrackPopupMenu for menus this renderer can't
    // reproduce (columns, right-to-left order, menu bars) and for empty menus.
    if (!PH_EMENU_DX::IsMenuSupported(Menu))
        return FALSE;

    return PH_EMENU_DX::ShowInternal(Menu, WindowHandle, Flags, Align, X, Y, DarkMode, nullptr, SelectedItem);
}

VOID PhCancelEMenuDX(
    VOID
    )
{
    PH_EMENU_DX::CancelThreadMenus();
}

PH_QUEUED_LOCK PH_EMENU_DX::SharedLock = PH_QUEUED_LOCK_INIT;
ULONG PH_EMENU_DX::SharedGeneration = 0;
#ifdef PH_EMENU_DX_ENABLE_D3D12
ID3D12Device* PH_EMENU_DX::SharedD3D12Device = nullptr;
ID3D12CommandQueue* PH_EMENU_DX::SharedD3D12CommandQueue = nullptr;
ID3D11Device* PH_EMENU_DX::SharedD3D11Device12 = nullptr;
ID3D11On12Device* PH_EMENU_DX::SharedD3D11On12Device = nullptr;
#endif
#ifdef PH_EMENU_DX_ENABLE_D3D11
ID3D11Device* PH_EMENU_DX::SharedD3D11Device = nullptr;
#endif
ID3D10Device1* PH_EMENU_DX::SharedD3DDevice = nullptr;
ID2D1Factory1* PH_EMENU_DX::SharedD2DFactory = nullptr;
ID2D1Device* PH_EMENU_DX::SharedD2DDevice = nullptr;
#ifdef PH_EMENU_DX_ENABLE_WINRT_COMPOSITION
ICompositor* PH_EMENU_DX::SharedCompositor = nullptr;
#else
IDCompositionDevice2* PH_EMENU_DX::SharedDCompDevice = nullptr;
IDCompositionDesktopDevice* PH_EMENU_DX::SharedDesktopDevice = nullptr;
IDCompositionDevice3* PH_EMENU_DX::SharedCompositionDevice3 = nullptr;
#endif
IDWriteFactory* PH_EMENU_DX::SharedDWriteFactory = nullptr;
IWICImagingFactory* PH_EMENU_DX::SharedWicFactory = nullptr;
#ifdef PH_EMENU_DX_ENABLE_WINRT_COMPOSITION
ICompositionGraphicsDevice* PH_EMENU_DX::SharedCompositionGraphicsDevice = nullptr;
#endif

PH_QUEUED_LOCK PH_EMENU_DX::MenuListLock = PH_QUEUED_LOCK_INIT;
PPH_LIST PH_EMENU_DX::MenuList = nullptr;

LRESULT CALLBACK PH_EMENU_DX::WindowProc(
    _In_ HWND WindowHandle,
    _In_ UINT WindowMessage,
    _In_ WPARAM wParam,
    _In_ LPARAM lParam
    )
{
    PPH_EMENU_DX context = static_cast<PPH_EMENU_DX>(PhGetWindowContextEx(WindowHandle));

    if (WindowMessage == WM_NCCREATE)
    {
        return PH_EMENU_DX::OnNcCreate(WindowHandle, reinterpret_cast<CREATESTRUCT*>(lParam));
    }

    if (!context)
        return DefWindowProc(WindowHandle, WindowMessage, wParam, lParam);

    switch (WindowMessage)
    {
    case WM_CREATE:
        return context->OnCreate(WindowHandle);
    case WM_PAINT:
        context->OnPaint(WindowHandle);
        return 0;
    case WM_ERASEBKGND:
        return 1;
    case WM_DESTROY:
        context->OnDestroy(WindowHandle);
        break;
    case WM_NCDESTROY:
        context->WindowHandle = nullptr;
        PhRemoveWindowContextEx(WindowHandle);
        break;
    case WM_MOUSEACTIVATE:
        return MA_NOACTIVATE;
    case WM_SETCURSOR:
        if (context->OnSetCursor(WindowHandle, reinterpret_cast<HWND>(wParam), LOWORD(lParam), HIWORD(lParam)))
            return TRUE;
        break;
    case WM_NCHITTEST:
        {
            // The shadow padding is visually empty; let input fall through it.
            RECT windowRect;
            POINT point = { GET_X_LPARAM(lParam), GET_Y_LPARAM(lParam) };
            LONG paddingPixels = context->GetSurfacePaddingPixels();

            if (paddingPixels && GetWindowRect(WindowHandle, &windowRect))
            {
                windowRect.left += paddingPixels;
                windowRect.top += paddingPixels;
                windowRect.right -= paddingPixels;
                windowRect.bottom -= paddingPixels;

                if (!PtInRect(&windowRect, point))
                    return HTTRANSPARENT;
            }
        }
        return HTCLIENT;
    case WM_MOUSEWHEEL:
        context->OnMouseWheel(WindowHandle, GET_WHEEL_DELTA_WPARAM(wParam));
        break;
    case WM_MOUSEMOVE:
        context->OnMouseMove(WindowHandle, GET_X_LPARAM(lParam), GET_Y_LPARAM(lParam));
        break;
    case WM_MOUSELEAVE:
        context->OnMouseLeave(WindowHandle);
        break;
    case WM_TIMER:
        context->OnTimer(WindowHandle, static_cast<ULONG>(wParam));
        break;
    case WM_LBUTTONUP:
        context->OnLButtonUp(WindowHandle, GET_X_LPARAM(lParam), GET_Y_LPARAM(lParam));
        break;
    case WM_RBUTTONUP:
        {
            // PH_EMENU_SHOW_LEFTRIGHT (TPM_RIGHTBUTTON): either button selects.
            if (FlagOn(context->Flags, PH_EMENU_SHOW_LEFTRIGHT))
                context->OnLButtonUp(WindowHandle, GET_X_LPARAM(lParam), GET_Y_LPARAM(lParam));
        }
        break;
    case WM_KEYDOWN:
        context->OnKeyDown(WindowHandle, static_cast<ULONG>(wParam));
        break;
    case WM_SYSKEYDOWN:
        context->OnSysKeyDown(WindowHandle, static_cast<ULONG>(wParam), !!(lParam & 0x40000000));
        break;
    case WM_CHAR:
    case WM_SYSCHAR:
        if (context->OnChar(WindowHandle, static_cast<WCHAR>(wParam)))
            return 0;
        break;
    case WM_ACTIVATEAPP:
        context->OnActivateApp(WindowHandle, static_cast<BOOLEAN>(wParam));
        break;
    case WM_ACTIVATE:
        return context->OnActivate(WindowHandle, static_cast<ULONG>(wParam), reinterpret_cast<HWND>(lParam));
    case WM_CANCELMODE:
        context->OnCancelMode(WindowHandle);
        break;
    case WM_UPDATEUISTATE:
    case WM_CHANGEUISTATE:
        context->OnUpdateUiState(WindowHandle, LOWORD(wParam), HIWORD(wParam));
        break;
    }

    return DefWindowProc(WindowHandle, WindowMessage, wParam, lParam);
}

PH_EMENU_DX::PH_EMENU_DX(
    _In_ PPH_EMENU Menu,
    _In_ ULONG Flags,
    _In_ ULONG Align,
    _In_ BOOLEAN DarkMode,
    _In_opt_ PPH_EMENU_DX ParentContext
    )
{
    // The parameters shadow the members; qualify the members explicitly.
    this->Menu = Menu;
    this->Flags = Flags;
    this->Align = Align;
    this->IsDarkMode = DarkMode;
    this->HoverIndex = -1;
    this->ParentContext = ParentContext;
    this->IsSubMenu = ParentContext != nullptr;

    // Submenus keep the metrics of their root so a chain never mixes the two sizes.
    if (ParentContext)
        this->Metrics = ParentContext->Metrics;
    else
        this->Metrics = PhEnableEMenuDxCompact ? &PhpEMenuDxCompactMetrics : &PhpEMenuDxDefaultMetrics;
}

PH_EMENU_DX::~PH_EMENU_DX()
{
    this->AfterLeavingMessageLoop();
}

BOOLEAN PH_EMENU_DX::Run(
    _In_ HWND OwnerWindow,
    _In_ LONG X,
    _In_ LONG Y,
    _Out_ PPH_EMENU_ITEM* SelectedItem
    )
{
    BOOLEAN result = FALSE;

    if (SelectedItem)
        *SelectedItem = nullptr;

    if (HR_SUCCESS(this->BeforeEnteringMessageLoop(OwnerWindow, X, Y)))
    {
        result = this->EnterMessageLoop();

        if (result && SelectedItem)
        {
            *SelectedItem = this->SelectedItem;
        }
    }

    this->AfterLeavingMessageLoop();
    return result;
}

HRESULT PH_EMENU_DX::BeforeEnteringMessageLoop(
    _In_ HWND OwnerWindow,
    _In_ LONG X,
    _In_ LONG Y
    )
{
    // Determine DPI and scaling factors for the target monitor
    {
        RECT windowRect = { X, Y, X + 1, Y + 1 };

        Dpi = static_cast<FLOAT>(PhGetMonitorDpi(nullptr, &windowRect));

        if (Dpi == 0)
        {
            Dpi = PH_EMENU_DX_DPI_BASE;
        }

        Scale = Dpi / PH_EMENU_DX_DPI_BASE;
    }

    // Calculate maximum visible height (in DIPs) from the work area of the target monitor
    {
        POINT point = { X, Y };
        MONITORINFO monitorInfo = { sizeof(MONITORINFO) };

        if (GetMonitorInfo(MonitorFromPoint(point, MONITOR_DEFAULTTONEAREST), &monitorInfo))
        {
            FLOAT screenHeight = static_cast<FLOAT>(monitorInfo.rcWork.bottom - monitorInfo.rcWork.top) / Scale;

            ScrollViewVisibleHeight = screenHeight * (IsSubMenu ? PH_EMENU_DX_SUBMENU_MAX_HEIGHT_PERCENT : PH_EMENU_DX_ROOT_MAX_HEIGHT_PERCENT);

            ScrollViewVisibleHeight = std::max(ScrollViewVisibleHeight, PH_EMENU_DX_MIN_HEIGHT);
        }
    }

    // Load user preferences for keyboard cues and menu delay
    {
        BOOL keyboardCues = TRUE;

        if (PhGetSystemParametersInfo(SPI_GETKEYBOARDCUES, 0, &keyboardCues, 0))
            ShowKeyboardCues = keyboardCues || PhGetKeyState(VK_MENU);
        else
            ShowKeyboardCues = !!PhGetKeyState(VK_MENU);
    }

    {
        ULONG menuVisibleDelay = 0;

        if (PhGetSystemParametersInfo(SPI_GETMENUSHOWDELAY, 0, &menuVisibleDelay, 0))
            MenuShowDelay = menuVisibleDelay;
        else
            MenuShowDelay = PH_EMENU_DX_DEFAULT_MENU_SHOW_DELAY;
    }

    InitializeShadow();

    // Initialize DirectWrite formats and calculate final layout/position
    HRESULT status = InitializeTextFormats();
    if (HR_FAILED(status))
        return status;

    CalculateLayout();
    CalculatePosition(X, Y, &X, &Y);

    this->OwnerWindowHandle = OwnerWindow;

    status = CreateMainWindow(OwnerWindow, X, Y);
    if (HR_SUCCESS(status))
        status = CreateResources();
    if (HR_SUCCESS(status))
        status = InitializeBrushes();
    if (HR_SUCCESS(status))
        status = DrawDCompMenuSurface();
    if (HR_SUCCESS(status))
        status = CommitDCompDevice();
    if (HR_SUCCESS(status))
        status = ShowMainWindow();
    return status;
}

BOOLEAN PH_EMENU_DX::EnterMessageLoop()
{
    // Enter the modal message loop for the menu
    while (!ExitLoop)
    {
        MSG msg;

        BOOL result = GetMessage(&msg, nullptr, 0, 0);
        if (std::cmp_equal(result, -1))
            return FALSE;

        if (std::cmp_greater(result, 0))
        {
            // Route keyboard messages specifically to the menu window
            if (std::cmp_greater_equal(msg.message, WM_KEYFIRST) && std::cmp_less_equal(msg.message, WM_KEYLAST))
            {
                msg.hwnd = WindowHandle;
            }
            else if (msg.message == WM_MOUSEWHEEL)
            {
                // The menu never has focus, so wheel input is delivered to the owner.
                // Send it to the menu under the cursor, or the innermost menu.
                POINT point = { GET_X_LPARAM(msg.lParam), GET_Y_LPARAM(msg.lParam) };
                HWND pointWindowHandle = WindowFromPoint(point);

                if (PH_EMENU_DX::IsWindowInMenuChain(pointWindowHandle))
                    msg.hwnd = pointWindowHandle;
                else
                    msg.hwnd = WindowHandle;
            }

            if (!CallMsgFilter(&msg, MSGF_MENU))
            {
                TranslateMessage(&msg);
                DispatchMessage(&msg);
            }
        }
        else
        {
            PostQuitMessage((LONG)msg.wParam);
            break;
        }
    }

    return TRUE;
}

VOID PH_EMENU_DX::AfterLeavingMessageLoop()
{
    if (MouseHookHandle)
    {
        UnhookWindowsHookEx(MouseHookHandle);
        MouseHookHandle = nullptr;
    }

    DestroySubMenuChain();
    DestroyMainWindow();

    if (ParentContext && ParentContext->SubMenuContext == this)
    {
        ParentContext->SubMenuContext = nullptr;
    }

    if (Registered)
    {
        UnregisterMenuContext();
        Registered = FALSE;
    }

    ReleaseCachedLayouts();
    ReleaseResources();
}

HRESULT PH_EMENU_DX::CreateMainWindow(
    _In_ HWND OwnerWindow,
    _In_ LONG X,
    _In_ LONG Y
    )
{
    RTL_ATOM windowAtom = PhRegisterMenuClass();

    if (windowAtom == RTL_ATOM_INVALID_ATOM)
        return E_FAIL;

    // Create the menu window
    {
        LONG paddingPixels = GetSurfacePaddingPixels();
        LONG windowWidth = static_cast<LONG>(Width * Scale) + paddingPixels * 2;
        LONG windowHeight = static_cast<LONG>(Height * Scale) + paddingPixels * 2;

        WindowHandle = PhCreateWindowEx(
            MAKEINTATOM(windowAtom),
            nullptr,
#ifndef PH_EMENU_DX_ENABLE_WINRT_COMPOSITION
            // The shadow padding is transparent; a frame would be drawn around it.
            ShadowEnabled ? WS_POPUP : (WS_POPUP | WS_BORDER),
#else
            WS_POPUP,
#endif
            WS_EX_TOPMOST | WS_EX_TOOLWINDOW | WS_EX_NOACTIVATE | WS_EX_NOREDIRECTIONBITMAP,
            X,
            Y,
            windowWidth,
            windowHeight,
            OwnerWindow,
            nullptr,
            NtCurrentImageBase(),
            this
            );
    }

    if (!WindowHandle)
        return E_FAIL;

    RegisterMenuContext();
    Registered = TRUE;
    return S_OK;
}

HRESULT PH_EMENU_DX::ShowMainWindow()
{
    if (!WindowHandle)
        return E_UNEXPECTED;

#ifndef PH_EMENU_DX_ENABLE_WINRT_COMPOSITION
    if (IsAcrylic || IsMica)
    {
        MARGINS margins = { -1, -1, -1, -1 };
        DynamicDwmExtendFrameIntoClientArea(WindowHandle, &margins);
    }
#endif

    if (ParentContext)
        ParentContext->SubMenuContext = this;

    if (!IsSubMenu)
        MouseHookHandle = SetWindowsHookEx(WH_MOUSE_LL, MouseHookCallback, nullptr, 0);

#ifndef PH_EMENU_DX_ENABLE_WINRT_COMPOSITION
    // Make sure the committed tree (menu and shadow) reached the compositor before
    // the window is mapped, so the first frame on screen already has the shadow.
    if (DCompDevice)
        DCompDevice->WaitForCommitCompletion();
#endif

    ShowWindow(WindowHandle, SW_SHOWNOACTIVATE);

    // The tree was committed fully transparent; start the entrance only once the
    // window is visible so no animation frames are spent on an unmapped window.
    StartOpenAnimation();
    return S_OK;
}

VOID PH_EMENU_DX::DestroyMainWindow()
{
    if (WindowHandle)
    {
        PlayCloseAnimation();

        ShowWindow(WindowHandle, SW_HIDE);
        DestroyWindow(WindowHandle);
        WindowHandle = nullptr;
    }
}

HRESULT PH_EMENU_DX::CreateD3DDevice()
{
    HRESULT status = E_FAIL;

#ifdef PH_EMENU_DX_ENABLE_D3D12
    if (HR_FAILED(status) && SharedD3D12Device && SharedD3D11Device12)
    {
        D3D12Device = SharedD3D12Device;
        D3D12Device->AddRef();
        D3D12CommandQueue = SharedD3D12CommandQueue;
        D3D12CommandQueue->AddRef();
        D3DDevice = SharedD3D11Device12;
        D3DDevice->AddRef();
        D3D11On12Device = SharedD3D11On12Device;
        D3D11On12Device->AddRef();

        ((ID3D11Device*)D3DDevice)->GetImmediateContext(&D3D11DeviceContext);

        status = D3D12Device->CreateCommandAllocator(D3D12_COMMAND_LIST_TYPE_DIRECT, IID_PPV_ARGS(&D3D12CommandAllocator));

        if (HR_SUCCESS(status))
        {
            status = D3D12Device->CreateCommandList(0, D3D12_COMMAND_LIST_TYPE_DIRECT, D3D12CommandAllocator, nullptr, IID_PPV_ARGS(&D3D12CommandList));
        }

        if (HR_SUCCESS(status))
        {
            D3D12CommandList->Close();
        }

        if (HR_SUCCESS(status))
        {
            status = D3D12Device->CreateFence(0, D3D12_FENCE_FLAG_NONE, IID_PPV_ARGS(&D3D12Fence));
        }

        if (HR_SUCCESS(status))
        {
            D3D12FenceValue = 1;
            D3D12FenceEvent = CreateEvent(nullptr, FALSE, FALSE, nullptr);

            if (!D3D12FenceEvent)
            {
                status = HRESULT_FROM_WIN32(GetLastError());
            }
        }

        if (HR_SUCCESS(status))
            status = S_OK;
    }
#endif
#ifdef PH_EMENU_DX_ENABLE_D3D11
    if (HR_FAILED(status) && SharedD3D11Device)
    {
        D3DDevice = SharedD3D11Device;
        D3DDevice->AddRef();
        status = S_OK;
    }
#endif
    if (HR_FAILED(status) && SharedD3DDevice)
    {
        status = SharedD3DDevice->QueryInterface(IID_PPV_ARGS(&D3DDevice));
        if (HR_SUCCESS(status))
            status = S_OK;
    }

    return status;
}

VOID PH_EMENU_DX::DestroyD3DDevice()
{
#ifdef PH_EMENU_DX_ENABLE_D3D12
    if (D3D12FenceEvent)
    {
        CloseHandle(D3D12FenceEvent);
        D3D12FenceEvent = nullptr;
    }
    if (D3D12Fence)
    {
        D3D12Fence->Release();
        D3D12Fence = nullptr;
    }
    if (D3D12CommandList)
    {
        D3D12CommandList->Release();
        D3D12CommandList = nullptr;
    }
    if (D3D12CommandAllocator)
    {
        D3D12CommandAllocator->Release();
        D3D12CommandAllocator = nullptr;
    }
    if (D3D12CommandQueue)
    {
        D3D12CommandQueue->Release();
        D3D12CommandQueue = nullptr;
    }
    if (D3D11DeviceContext)
    {
        D3D11DeviceContext->Release();
        D3D11DeviceContext = nullptr;
    }
    if (D3D11On12Device)
    {
        D3D11On12Device->Release();
        D3D11On12Device = nullptr;
    }
    if (D3DDevice)
    {
        D3DDevice->Release();
        D3DDevice = nullptr;
    }
    if (D3D12Device)
    {
        D3D12Device->Release();
        D3D12Device = nullptr;
    }
#else
    if (D3DDevice)
    {
        D3DDevice->Release();
        D3DDevice = nullptr;
    }
#endif
}

HRESULT PH_EMENU_DX::CreateD2DDevice()
{
    HRESULT status = S_OK;

    if (!SharedD2DFactory || !SharedD2DDevice)
        return E_FAIL;

    D2DFactory = SharedD2DFactory;
    D2DFactory->AddRef();

    D2DDevice = SharedD2DDevice;
    D2DDevice->AddRef();

    status = D2DDevice->CreateDeviceContext(D2D1_DEVICE_CONTEXT_OPTIONS_NONE, &D2DDeviceContext);

    if (HR_FAILED(status))
        return status;

    D2DDeviceContext->SetDpi(Dpi, Dpi);
    D2DDeviceContext->SetAntialiasMode(D2D1_ANTIALIAS_MODE_PER_PRIMITIVE);
    D2DDeviceContext->SetTextAntialiasMode(D2D1_TEXT_ANTIALIAS_MODE_GRAYSCALE);

    return status;
}

VOID PH_EMENU_DX::DestroyD2DDevice()
{
    if (D2DDeviceContext)
    {
        D2DDeviceContext->Release();
        D2DDeviceContext = nullptr;
    }
    if (D2DDevice)
    {
        D2DDevice->Release();
        D2DDevice = nullptr;
    }
    if (D2DFactory)
    {
        D2DFactory->Release();
        D2DFactory = nullptr;
    }
}

HRESULT PH_EMENU_DX::CreateDWriteFactory()
{
    HRESULT status = S_OK;

    PhAcquireQueuedLockExclusive(&SharedLock);

    if (!SharedDWriteFactory)
    {
        status = DynamicDWriteCreateFactory(DWRITE_FACTORY_TYPE_SHARED, __uuidof(IDWriteFactory), reinterpret_cast<IUnknown**>(&SharedDWriteFactory));
    }

    if (HR_SUCCESS(status) && SharedDWriteFactory)
    {
        DWriteFactory = SharedDWriteFactory;
        DWriteFactory->AddRef();
    }
    else if (HR_SUCCESS(status))
    {
        status = E_FAIL;
    }

    PhReleaseQueuedLockExclusive(&SharedLock);

    return status;
}

VOID PH_EMENU_DX::DestroyDWriteFactory()
{
    if (TextFormat)
    {
        TextFormat->Release();
        TextFormat = nullptr;
    }
    if (ShortcutFormat)
    {
        ShortcutFormat->Release();
        ShortcutFormat = nullptr;
    }
    if (DWriteFactory)
    {
        DWriteFactory->Release();
        DWriteFactory = nullptr;
    }
}

HRESULT PH_EMENU_DX::CreateDCompDevice()
{
    HRESULT status = S_OK;

#ifdef PH_EMENU_DX_ENABLE_WINRT_COMPOSITION
    if (!SharedCompositor)
        return E_FAIL;

    Compositor = SharedCompositor;
    Compositor->AddRef();
#else
    if (!SharedDCompDevice || !SharedDesktopDevice)
        return E_FAIL;

    DCompDevice = SharedDCompDevice;
    DCompDevice->AddRef();
    DesktopDevice = SharedDesktopDevice;
    DesktopDevice->AddRef();
    CompositionDevice3 = SharedCompositionDevice3;
    if (CompositionDevice3)
        CompositionDevice3->AddRef();
#endif

    return status;
}

VOID PH_EMENU_DX::DestroyDCompDevice()
{
#ifdef PH_EMENU_DX_ENABLE_WINRT_COMPOSITION
    if (Compositor)
    {
        Compositor->Release();
        Compositor = nullptr;
    }
#else
    if (CompositionDevice3)
    {
        CompositionDevice3->Release();
        CompositionDevice3 = nullptr;
    }
    if (DesktopDevice)
    {
        DesktopDevice->Release();
        DesktopDevice = nullptr;
    }
    if (DCompDevice)
    {
        DCompDevice->Release();
        DCompDevice = nullptr;
    }
#endif
}

HRESULT PH_EMENU_DX::CreateResources()
{
    HRESULT status;

    // Take our own references to the shared objects while holding the lock so a
    // concurrent device-lost reset cannot release them underneath us.
    PhAcquireQueuedLockExclusive(&SharedLock);

    status = InitializeSharedResources();
    if (HR_SUCCESS(status))
        status = CreateD3DDevice();
    if (HR_SUCCESS(status))
        status = CreateD2DDevice();
    if (HR_SUCCESS(status))
        status = CreateDCompDevice();
    if (HR_SUCCESS(status))
    {
        if (!WicFactory && SharedWicFactory)
        {
            WicFactory = SharedWicFactory;
            WicFactory->AddRef();
        }

        ResourceGeneration = SharedGeneration;
    }

    PhReleaseQueuedLockExclusive(&SharedLock);

    if (HR_SUCCESS(status))
        status = CreateDrawingResources();
    if (HR_SUCCESS(status))
        status = CreateDCompVisualTree();
    return status;
}

VOID PH_EMENU_DX::ReleaseResources()
{
#ifdef PH_EMENU_DX_ENABLE_D3D12
    if (D3D12CommandQueue && D3D12Fence)
    {
        UINT64 fenceValue = D3D12FenceValue++;
        D3D12CommandQueue->Signal(D3D12Fence, fenceValue);
        if (std::cmp_less(D3D12Fence->GetCompletedValue(), fenceValue))
        {
            D3D12Fence->SetEventOnCompletion(fenceValue, D3D12FenceEvent);
            WaitForSingleObject(D3D12FenceEvent, INFINITE);
        }
    }
#endif

    DestroyDCompVisualTree();
    DestroyDrawingResources();
    DestroyDCompDevice();
    DestroyDWriteFactory();
    DestroyD2DDevice();
    DestroyD3DDevice();
}

HRESULT PH_EMENU_DX::CreateDrawingResources()
{
    HRESULT status = S_OK;

#ifdef PH_EMENU_DX_ENABLE_SWAPCHAIN
    {
        ComPtr<IDXGIDevice> dxgiDevice;
        ComPtr<IDXGIAdapter> dxgiAdapter;
        ComPtr<IDXGIFactory2> dxgiFactory;

        status = ((IUnknown*)D3DDevice)->QueryInterface(IID_PPV_ARGS(&dxgiDevice));

        if (HR_SUCCESS(status))
            status = dxgiDevice->GetAdapter(&dxgiAdapter);

        if (HR_SUCCESS(status))
            status = dxgiAdapter->GetParent(IID_PPV_ARGS(&dxgiFactory));

        if (HR_SUCCESS(status))
        {
            DXGI_SWAP_CHAIN_DESC1 description = { 0 };
            description.Width = static_cast<UINT>(Width * Scale);
            description.Height = static_cast<UINT>(Height * Scale);
            description.Format = DXGI_FORMAT_B8G8R8A8_UNORM;
            description.BufferUsage = DXGI_USAGE_RENDER_TARGET_OUTPUT;
            description.SampleDesc.Count = 1;
            description.BufferCount = 2;
            description.SwapEffect = DXGI_SWAP_EFFECT_FLIP_SEQUENTIAL;
            description.AlphaMode = DXGI_ALPHA_MODE_PREMULTIPLIED;

            status = dxgiFactory->CreateSwapChainForComposition(D3DDevice, &description, nullptr, &SwapChain);
        }
    }

    if (HR_FAILED(status))
        return status;
#endif

    {
        D2D1_ROUNDED_RECT roundRect = D2D1::RoundedRect(D2D1::RectF(0, 0, Width, Height), Metrics->RoundedCorner, Metrics->RoundedCorner);

        if (HR_SUCCESS(D2DFactory->CreateRoundedRectangleGeometry(&roundRect, &ClipGeometry)))
        {
            // D2D 1.1: pass nullptr to PushLayer; no pre-allocated layer needed
        }
    }

    return status;
}

VOID PH_EMENU_DX::DestroyDrawingResources()
{
    if (TextBrush)
    {
        TextBrush->Release();
        TextBrush = nullptr;
    }
    if (ShortcutBrush)
    {
        ShortcutBrush->Release();
        ShortcutBrush = nullptr;
    }
    if (HoverBrush)
    {
        HoverBrush->Release();
        HoverBrush = nullptr;
    }
    if (BorderBrush)
    {
        BorderBrush->Release();
        BorderBrush = nullptr;
    }
    if (SeparatorBrush)
    {
        SeparatorBrush->Release();
        SeparatorBrush = nullptr;
    }
    if (ClipLayer)
    {
        ClipLayer->Release();
        ClipLayer = nullptr;
    }
    if (ClipGeometry)
    {
        ClipGeometry->Release();
        ClipGeometry = nullptr;
    }
    if (WicFactory)
    {
        WicFactory->Release();
        WicFactory = nullptr;
    }
#ifdef PH_EMENU_DX_ENABLE_SWAPCHAIN
    if (SwapChain)
    {
        SwapChain->Release();
        SwapChain = nullptr;
    }
#endif
}

HRESULT PH_EMENU_DX::InitializeTextFormats()
{
    HRESULT hr = CreateDWriteFactory();
    if (HR_FAILED(hr))
        return hr;

    hr = DWriteFactory->CreateTextFormat(
        PH_EMENU_DX_FONT_NAME,
        nullptr,
        DWRITE_FONT_WEIGHT_NORMAL,
        DWRITE_FONT_STYLE_NORMAL,
        DWRITE_FONT_STRETCH_NORMAL,
        Metrics->TextFontSize,
        L"en-us",
        &TextFormat
    );
    if (HR_FAILED(hr))
        return hr;

    hr = DWriteFactory->CreateTextFormat(
        PH_EMENU_DX_FONT_NAME,
        nullptr,
        DWRITE_FONT_WEIGHT_NORMAL,
        DWRITE_FONT_STYLE_NORMAL,
        DWRITE_FONT_STRETCH_NORMAL,
        Metrics->ShortcutFontSize,
        L"en-us",
        &ShortcutFormat
    );
    if (HR_FAILED(hr))
        return hr;

    TextFormat->SetParagraphAlignment(DWRITE_PARAGRAPH_ALIGNMENT_NEAR);
    TextFormat->SetTextAlignment(DWRITE_TEXT_ALIGNMENT_LEADING);
    TextFormat->SetWordWrapping(DWRITE_WORD_WRAPPING_NO_WRAP);
    ShortcutFormat->SetParagraphAlignment(DWRITE_PARAGRAPH_ALIGNMENT_NEAR);
    ShortcutFormat->SetTextAlignment(DWRITE_TEXT_ALIGNMENT_LEADING);
    ShortcutFormat->SetWordWrapping(DWRITE_WORD_WRAPPING_NO_WRAP);

    return S_OK;
}

HRESULT PH_EMENU_DX::InitializeBrushes()
{
    HRESULT result;
    result = D2DDeviceContext->CreateSolidColorBrush(IsDarkMode ? PH_EMENU_DX_TEXT_COLOR_DARK : PH_EMENU_DX_TEXT_COLOR_LIGHT, &TextBrush);
    if (HR_FAILED(result)) return result;
    result = D2DDeviceContext->CreateSolidColorBrush(IsDarkMode ? PH_EMENU_DX_SHORTCUT_COLOR_DARK : PH_EMENU_DX_SHORTCUT_COLOR_LIGHT, &ShortcutBrush);
    if (HR_FAILED(result)) return result;
    result = D2DDeviceContext->CreateSolidColorBrush(IsDarkMode ? PH_EMENU_DX_HOVER_COLOR_DARK : PH_EMENU_DX_HOVER_COLOR_LIGHT, &HoverBrush);
    if (HR_FAILED(result)) return result;
    result = D2DDeviceContext->CreateSolidColorBrush(IsDarkMode ? PH_EMENU_DX_BORDER_COLOR_DARK : PH_EMENU_DX_BORDER_COLOR_LIGHT, &BorderBrush);
    if (HR_FAILED(result)) return result;
    result = D2DDeviceContext->CreateSolidColorBrush(IsDarkMode ? PH_EMENU_DX_SEPARATOR_COLOR_DARK : PH_EMENU_DX_SEPARATOR_COLOR_LIGHT, &SeparatorBrush);
    return result;
}

HRESULT PH_EMENU_DX::CreateDCompVisualTree()
{
    HRESULT status = CreateDCompTarget();
    if (HR_SUCCESS(status))
        status = CreateDCompRootVisual();
    if (HR_SUCCESS(status))
        status = BindDCompRootVisualToTarget();
    if (HR_SUCCESS(status))
        status = CreateDCompMenuVisual();
    if (HR_SUCCESS(status))
        status = CreateDCompMenuSurface();
    if (HR_SUCCESS(status))
        status = BindDCompMenuSurfaceToVisual();
    if (HR_SUCCESS(status))
        status = SetBackfaceVisibilityOnDCompMenuVisual();
    if (HR_SUCCESS(status))
        status = CreateDCompShadowTree();
    if (HR_SUCCESS(status))
        status = AddDCompMenuVisualToRootVisual();
    return status;
}

VOID PH_EMENU_DX::DestroyDCompVisualTree()
{
    DestroyDCompShadowTree();
    DestroyDCompMenuSurface();
    DestroyDCompMenuVisual();
#ifdef PH_EMENU_DX_ENABLE_WINRT_COMPOSITION
#ifdef PH_EMENU_DX_ENABLE_HOSTBACKDROPBRUSH
    if (BackdropVisual)
    {
        BackdropVisual->Release();
        BackdropVisual = nullptr;
    }
    if (BackdropBrush)
    {
        BackdropBrush->Release();
        BackdropBrush = nullptr;
    }
#endif
#ifdef PH_EMENU_DX_ENABLE_DROPSHADOW
    if (ShadowVisual)
    {
        ShadowVisual->Release();
        ShadowVisual = nullptr;
    }
    if (DropShadow)
    {
        DropShadow->Release();
        DropShadow = nullptr;
    }
#endif
#endif
    DestroyDCompRootVisual();
    DestroyDCompTarget();
}

HRESULT PH_EMENU_DX::CreateDCompTarget()
{
    BOOL topmost = TRUE;

#ifdef PH_EMENU_DX_ENABLE_WINRT_COMPOSITION
    ComPtr<ICompositorDesktopInterop> desktopInterop;
    HRESULT status = Compositor->QueryInterface(IID_PPV_ARGS(&desktopInterop));
    if (HR_SUCCESS(status))
        status = desktopInterop->CreateDesktopWindowTarget(WindowHandle, topmost, &CompositionTarget);
    return status;
#else
    return DesktopDevice->CreateTargetForHwnd(WindowHandle, topmost, &DCompTarget);
#endif
}

VOID PH_EMENU_DX::DestroyDCompTarget()
{
#ifdef PH_EMENU_DX_ENABLE_WINRT_COMPOSITION
    if (CompositionTarget)
    {
        CompositionTarget->Release();
        CompositionTarget = nullptr;
    }
#else
    if (DCompTarget)
    {
        DCompTarget->Release();
        DCompTarget = nullptr;
    }
#endif
}

HRESULT PH_EMENU_DX::CreateDCompRootVisual()
{
    HRESULT status = S_OK;

#ifdef PH_EMENU_DX_ENABLE_WINRT_COMPOSITION
    status = Compositor->CreateContainerVisual(&RootVisual);
#else
    status = DCompDevice->CreateVisual(&RootVisual);
#endif

    return status;
}

VOID PH_EMENU_DX::DestroyDCompRootVisual()
{
    if (RootVisual)
    {
        RootVisual->Release();
        RootVisual = nullptr;
    }
}

HRESULT PH_EMENU_DX::CreateDCompMenuVisual()
{
#ifdef PH_EMENU_DX_ENABLE_WINRT_COMPOSITION
    return Compositor->CreateSpriteVisual(&MenuVisual);
#else
    return DCompDevice->CreateVisual(&MenuVisual);
#endif
}

VOID PH_EMENU_DX::DestroyDCompMenuVisual()
{
    if (MenuVisual)
    {
        MenuVisual->Release();
        MenuVisual = nullptr;
    }
}

HRESULT PH_EMENU_DX::CreateDCompMenuSurface()
{
    HRESULT status = S_OK;

#ifdef PH_EMENU_DX_ENABLE_WINRT_COMPOSITION
    {
#ifdef PH_EMENU_DX_ENABLE_SWAPCHAIN
        ComPtr<ICompositorInterop> compositorInterop;
        if (HR_SUCCESS(Compositor->QueryInterface(IID_PPV_ARGS(&compositorInterop))))
        {
            status = compositorInterop->CreateCompositionSurfaceForSwapChain(SwapChain, &CompositionSurface);
        }

        if (HR_FAILED(status))
        {
            dprintf("PhEMenuDx: CreateCompositionSurfaceForSwapChain failed with 0x%08x\n", status);
        }
#else
        ABI::Windows::Foundation::Size size =
        {
            Width * Scale,
            Height * Scale
        };

        status = SharedCompositionGraphicsDevice->CreateDrawingSurface(
            size,
            DirectXPixelFormat_B8G8R8A8UIntNormalized,
            DirectXAlphaMode_Premultiplied,
            &CompositionSurface
            );
        if (HR_FAILED(status))
        {
            dprintf("PhEMenuDx: CreateDrawingSurface failed with 0x%08x (Size: %fx%f)\n", status, size.Width, size.Height);
        }
#endif
    }
#else
#ifdef PH_EMENU_DX_ENABLE_SWAPCHAIN
    status = S_OK;
#else
    status = DCompDevice->CreateSurface(
        static_cast<UINT>(Width * Scale),
        static_cast<UINT>(Height * Scale),
        DXGI_FORMAT_B8G8R8A8_UNORM,
        DXGI_ALPHA_MODE_PREMULTIPLIED,
        &DCompSurface
        );
    if (HR_FAILED(status))
    {
        dprintf("PhEMenuDx: DComp CreateSurface failed with 0x%08x\n", status);
    }
#endif
#endif

    return status;
}

VOID PH_EMENU_DX::DestroyDCompMenuSurface()
{
#ifdef PH_EMENU_DX_ENABLE_WINRT_COMPOSITION
    if (CompositionSurface)
    {
        CompositionSurface->Release();
        CompositionSurface = nullptr;
    }
#else
    if (MenuBitmap)
    {
        MenuBitmap->Release();
        MenuBitmap = nullptr;
    }
    if (DCompSurface)
    {
        DCompSurface->Release();
        DCompSurface = nullptr;
    }
#endif
}

HRESULT PH_EMENU_DX::DrawDCompMenuSurface()
{
    HRESULT status;
#if defined(PH_EMENU_DX_ENABLE_SWAPCHAIN) || defined(PH_EMENU_DX_ENABLE_WINRT_COMPOSITION)
    IDXGISurface* dxgiSurface = nullptr;
#endif

#ifdef PH_EMENU_DX_ENABLE_SWAPCHAIN
    if (SwapChain)
    {
        // Get the back buffer of the swap chain
        status = SwapChain->GetBuffer(0, IID_PPV_ARGS(&dxgiSurface));

        if (HR_SUCCESS(status))
        {
            ID2D1Bitmap1* targetBitmap = nullptr;
            D2D1_BITMAP_PROPERTIES1 bitmapProperties =
            {
                { DXGI_FORMAT_B8G8R8A8_UNORM, D2D1_ALPHA_MODE_PREMULTIPLIED },
                0.0f,
                0.0f,
                D2D1_BITMAP_OPTIONS_TARGET | D2D1_BITMAP_OPTIONS_CANNOT_DRAW,
                nullptr
            };

            status = D2DDeviceContext->CreateBitmapFromDxgiSurface(dxgiSurface, &bitmapProperties, &targetBitmap);

            if (HR_SUCCESS(status))
            {
                D2DDeviceContext->SetTarget(targetBitmap);
                D2DDeviceContext->SetDpi(Dpi, Dpi);
                D2DDeviceContext->BeginDraw();

                D2DDeviceContext->SetTransform(D2D1::Matrix3x2F::Identity());

                Render();

                D2DDeviceContext->Flush();
                status = D2DDeviceContext->EndDraw();

                if (HR_FAILED(status))
                {
                    dprintf("PhEMenuDx: D2D EndDraw failed with 0x%08x\n", status);
                }

                D2DDeviceContext->SetTarget(nullptr);
                targetBitmap->Release();
            }
            else
            {
                dprintf("PhEMenuDx: CreateBitmapFromDxgiSurface failed with 0x%08x\n", status);
            }

            dxgiSurface->Release();

            if (HR_SUCCESS(status))
            {
                status = SwapChain->Present(1, 0);

                if (HR_FAILED(status))
                {
                    dprintf("PhEMenuDx: SwapChain Present failed with 0x%08x\n", status);
                }
            }
        }
        else
        {
            dprintf("PhEMenuDx: SwapChain GetBuffer failed with 0x%08x\n", status);
        }
    }
#else
    POINT offset;
#ifdef PH_EMENU_DX_ENABLE_WINRT_COMPOSITION
    {
        // Handle WinRT Composition drawing lifecycle
        ComPtr<ICompositionDrawingSurfaceInterop> surfaceInterop;

        status = CompositionSurface->QueryInterface(IID_PPV_ARGS(&surfaceInterop));

        if (HR_SUCCESS(status))
        {
            // Begin drawing on the composition surface
            status = surfaceInterop->BeginDraw(nullptr, IID_PPV_ARGS(&dxgiSurface), &offset);
            if (HR_FAILED(status))
            {
                dprintf("PhEMenuDx: BeginDraw (WinRT) failed with 0x%08x\n", status);
            }
        }

        if (HR_SUCCESS(status))
        {
            ID2D1Bitmap1* targetBitmap = nullptr;
            D2D1_BITMAP_PROPERTIES1 bitmapProperties =
            {
                { DXGI_FORMAT_B8G8R8A8_UNORM, D2D1_ALPHA_MODE_PREMULTIPLIED },
                0.0f,
                0.0f,
                D2D1_BITMAP_OPTIONS_TARGET | D2D1_BITMAP_OPTIONS_CANNOT_DRAW,
                nullptr
            };

            // Wrap the DXGI surface in a D2D bitmap for rendering
            status = D2DDeviceContext->CreateBitmapFromDxgiSurface(dxgiSurface, &bitmapProperties, &targetBitmap);
            if (HR_SUCCESS(status))
            {
                D2DDeviceContext->SetTarget(targetBitmap);
                D2DDeviceContext->SetDpi(Dpi, Dpi);
                D2DDeviceContext->BeginDraw();

                // Apply composition surface offset to D2D transform
                D2DDeviceContext->SetTransform(D2D1::Matrix3x2F::Translation(offset.x / Scale, offset.y / Scale));

                // Perform the actual menu rendering
                Render();

                D2DDeviceContext->Flush();
                status = D2DDeviceContext->EndDraw();
                if (HR_FAILED(status))
                {
                    dprintf("PhEMenuDx: D2D EndDraw (WinRT) failed with 0x%08x\n", status);
                }

                D2DDeviceContext->SetTarget(nullptr);
                targetBitmap->Release();
            }

            // Signal completion of drawing to the composition engine
            status = surfaceInterop->EndDraw();
            if (HR_FAILED(status))
            {
                dprintf("PhEMenuDx: surfaceInterop EndDraw failed with 0x%08x\n", status);
            }

            dxgiSurface->Release();
        }
    }
#else
    ComPtr<ID2D1DeviceContext> drawingContext;
    ComPtr<ID2D1Image> previousTarget;
    D2D1_SIZE_U surfaceSize = D2D1::SizeU(static_cast<UINT32>(Width * Scale), static_cast<UINT32>(Height * Scale));

    // DirectComposition places every update at a different atlas offset. Rendering
    // at that offset translates the menu by offset / Scale DIPs, which is inexact at
    // fractional scales, so glyph snapping rounds differently from one hover redraw
    // to the next and the text jumps. Render at a fixed origin with the resource
    // context instead and copy the pixels to the whole-pixel update offset.
    if (MenuBitmap)
    {
        D2D1_SIZE_U bitmapSize = MenuBitmap->GetPixelSize();

        if (bitmapSize.width != surfaceSize.width || bitmapSize.height != surfaceSize.height)
        {
            MenuBitmap->Release();
            MenuBitmap = nullptr;
        }
    }

    if (!MenuBitmap)
    {
        status = D2DDeviceContext->CreateBitmap(
            surfaceSize,
            nullptr,
            0,
            D2D1::BitmapProperties1(D2D1_BITMAP_OPTIONS_TARGET, D2D1::PixelFormat(DXGI_FORMAT_B8G8R8A8_UNORM, D2D1_ALPHA_MODE_PREMULTIPLIED)),
            &MenuBitmap
            );

        if (HR_FAILED(status))
        {
            dprintf("PhEMenuDx: Menu bitmap creation failed with 0x%08x\n", status);
            return status;
        }
    }

    D2DDeviceContext->GetTarget(&previousTarget);
    D2DDeviceContext->SetTarget(MenuBitmap);
    D2DDeviceContext->SetDpi(Dpi, Dpi);
    D2DDeviceContext->BeginDraw();
    D2DDeviceContext->SetTransform(D2D1::Matrix3x2F::Identity());

    Render();

    status = D2DDeviceContext->EndDraw();
    D2DDeviceContext->SetTarget(previousTarget.Get());

    if (HR_FAILED(status))
    {
        dprintf("PhEMenuDx: Menu bitmap EndDraw failed with 0x%08x\n", status);
        return status;
    }

    status = DCompSurface->BeginDraw(nullptr, IID_PPV_ARGS(&drawingContext), &offset);
    if (HR_FAILED(status))
    {
        dprintf("PhEMenuDx: BeginDraw (DComp) failed with 0x%08x\n", status);
    }

    if (HR_SUCCESS(status))
    {
        D2D1_POINT_2F targetOffset = D2D1::Point2F(static_cast<FLOAT>(offset.x), static_cast<FLOAT>(offset.y));

        // Device pixels: a 1:1 copy that replaces the whole update rectangle.
        drawingContext->SetDpi(PH_EMENU_DX_DPI_BASE, PH_EMENU_DX_DPI_BASE);
        drawingContext->SetTransform(D2D1::Matrix3x2F::Identity());
        drawingContext->DrawImage(
            MenuBitmap,
            &targetOffset,
            nullptr,
            D2D1_INTERPOLATION_MODE_NEAREST_NEIGHBOR,
            D2D1_COMPOSITE_MODE_SOURCE_COPY
            );

        status = DCompSurface->EndDraw();
        if (HR_FAILED(status))
        {
            dprintf("PhEMenuDx: DCompSurface EndDraw failed with 0x%08x\n", status);
        }
    }
#endif
#endif

    return status;
}

HRESULT PH_EMENU_DX::BindDCompRootVisualToTarget()
{
#ifdef PH_EMENU_DX_ENABLE_WINRT_COMPOSITION
    ComPtr<ICompositionTarget> target;
    ComPtr<IVisual> visual;
    HRESULT status = CompositionTarget->QueryInterface(IID_PPV_ARGS(&target));
    if (HR_SUCCESS(status))
        status = RootVisual->QueryInterface(IID_PPV_ARGS(&visual));
    if (HR_SUCCESS(status))
        status = target->put_Root(visual.Get());
    return status;
#else
    return DCompTarget->SetRoot(RootVisual);
#endif
}

HRESULT PH_EMENU_DX::BindDCompMenuSurfaceToVisual()
{
    HRESULT status = S_OK;

#ifdef PH_EMENU_DX_ENABLE_WINRT_COMPOSITION
    FLOAT surfacePadding = (FLOAT)GetSurfacePaddingPixels() / Scale;

    {
        ComPtr<ISpriteVisual> spriteVisual = MenuVisual;
        ComPtr<ICompositionSurfaceBrush> surfaceBrush;
        ComPtr<ICompositionBrush> brush;
#ifndef PH_EMENU_DX_ENABLE_SWAPCHAIN
        ComPtr<ICompositionSurface> compositionSurface;
#endif

#ifndef PH_EMENU_DX_ENABLE_SWAPCHAIN
        if (HR_SUCCESS(status))
            status = CompositionSurface->QueryInterface(IID_PPV_ARGS(&compositionSurface));

        if (HR_SUCCESS(status))
            status = Compositor->CreateSurfaceBrushWithSurface(compositionSurface.Get(), &surfaceBrush);
#else
        if (HR_SUCCESS(status))
            status = Compositor->CreateSurfaceBrushWithSurface(CompositionSurface, &surfaceBrush);
#endif

        if (HR_SUCCESS(status))
            status = surfaceBrush->QueryInterface(IID_PPV_ARGS(&brush));

        if (HR_SUCCESS(status))
            status = spriteVisual->put_Brush(brush.Get());

        if (HR_SUCCESS(status))
        {
            ComPtr<IVisual> visual;

            if (HR_SUCCESS(spriteVisual->QueryInterface(IID_PPV_ARGS(&visual))))
            {
                ABI::Windows::Foundation::Numerics::Vector2 size = { Width * Scale, Height * Scale };
                ABI::Windows::Foundation::Numerics::Vector3 offset = { surfacePadding * Scale, surfacePadding * Scale, 0.0f };

                status = visual->put_Size(size);

                if (HR_SUCCESS(status))
                    status = visual->put_Offset(offset);
            }
        }

        if (HR_SUCCESS(status))
        {
            ComPtr<IVisualCollection> children;

            status = RootVisual->get_Children(&children);

            if (HR_SUCCESS(status))
            {
                ComPtr<IVisual> visual;

#ifndef PH_EMENU_DX_ENABLE_DROPSHADOW
#ifdef PH_EMENU_DX_ENABLE_HOSTBACKDROPBRUSH
                if (BackdropType != PhEMenuDxBackdropNone)
                {
                    ICompositionBackdropBrush* backdropBrush = nullptr;

                    status = E_FAIL;

                    if (BackdropType == PhEMenuDxBackdropMica)
                    {
                        __x_ABI_CWindows_CUI_CComposition_CICompositorWithBlurredWallpaperBackdropBrush* compositorMica;

                        if (HR_SUCCESS(Compositor->QueryInterface(IID_PPV_ARGS(&compositorMica))))
                        {
                            status = compositorMica->TryCreateBlurredWallpaperBackdropBrush(&backdropBrush);
                            compositorMica->Release();
                        }
                    }

                    if (HR_FAILED(status) || BackdropType == PhEMenuDxBackdropHost)
                    {
                        ComPtr<ICompositor3> compositor3;

                        if (HR_SUCCESS(Compositor->QueryInterface(IID_PPV_ARGS(&compositor3))))
                        {
                            status = compositor3->CreateHostBackdropBrush(&backdropBrush);
                        }
                    }

                    if (HR_FAILED(status) || BackdropType == PhEMenuDxBackdropStandard)
                    {
                        ComPtr<ICompositor2> compositor2;

                        if (HR_SUCCESS(Compositor->QueryInterface(IID_PPV_ARGS(&compositor2))))
                        {
                            status = compositor2->CreateBackdropBrush(&backdropBrush);
                        }
                    }

                    if (HR_SUCCESS(status) && backdropBrush)
                    {
                        status = backdropBrush->QueryInterface(IID_PPV_ARGS(&BackdropBrush));
                        backdropBrush->Release();
                    }

                    if (HR_SUCCESS(status))
                    {
                        status = Compositor->CreateSpriteVisual(&BackdropVisual);

                        if (HR_SUCCESS(status))
                        {
                            status = BackdropVisual->put_Brush(BackdropBrush);

                            if (HR_SUCCESS(status))
                            {
                                ComPtr<IVisual> backdropVisual;

                                if (HR_SUCCESS(BackdropVisual->QueryInterface(IID_PPV_ARGS(&backdropVisual))))
                                {
                                    ABI::Windows::Foundation::Numerics::Vector2 size = { Width * Scale, Height * Scale };
                                    ABI::Windows::Foundation::Numerics::Vector3 offset = { surfacePadding * Scale, surfacePadding * Scale, 0.0f };

                                    backdropVisual->put_Size(size);
                                    backdropVisual->put_Offset(offset);

                                    children->InsertAtBottom(backdropVisual.Get());
                                }
                            }
                        }
                    }
                }
#endif
#endif

#ifdef PH_EMENU_DX_ENABLE_DROPSHADOW
                {
                    ComPtr<ICompositor2> compositor2;

                    if (HR_SUCCESS(Compositor->QueryInterface(IID_PPV_ARGS(&compositor2))))
                    {
                        ComPtr<IDropShadow> dropShadow;

                        status = compositor2->CreateDropShadow(&dropShadow);

                        if (HR_SUCCESS(status))
                        {
                            DropShadow = dropShadow.Detach();
                        }
                    }

                    if (HR_SUCCESS(status))
                    {
                        status = Compositor->CreateSpriteVisual(&ShadowVisual);
                    }

                    if (HR_SUCCESS(status))
                    {
                        ComPtr<ISpriteVisual2> spriteVisual2;

                        if (HR_SUCCESS(ShadowVisual->QueryInterface(IID_PPV_ARGS(&spriteVisual2))))
                        {
                            status = spriteVisual2->put_Shadow(static_cast<ICompositionShadow*>(DropShadow));
                        }
                    }

                    if (HR_SUCCESS(status))
                    {
                        ComPtr<IVisual> shadowVisual;

                        if (HR_SUCCESS(ShadowVisual->QueryInterface(IID_PPV_ARGS(&shadowVisual))))
                        {
                            ABI::Windows::Foundation::Numerics::Vector2 size = { Width * Scale, Height * Scale };
                            ABI::Windows::Foundation::Numerics::Vector3 offset = { surfacePadding * Scale, surfacePadding * Scale, 0.0f };

                            shadowVisual->put_Size(size);
                            shadowVisual->put_Offset(offset);

                            ComPtr<IDropShadow> dropShadow;

                            if (HR_SUCCESS(DropShadow->QueryInterface(IID_PPV_ARGS(&dropShadow))))
                            {
#ifdef PH_EMENU_DX_ENABLE_DROPSHADOW_DEBUG
                                dropShadow->put_BlurRadius(PH_EMENU_DX_SHADOW_BLUR_RADIUS * Scale);
                                dropShadow->put_Opacity(1.0f);
                                dropShadow->put_Offset({ 0.0f, PH_EMENU_DX_SHADOW_OFFSET_Y * Scale, 0.0f });
                                dropShadow->put_Color({ 255, 255, 0, 0 }); // Red
#else
                                dropShadow->put_BlurRadius(PH_EMENU_DX_SHADOW_BLUR_RADIUS * Scale);
                                dropShadow->put_Opacity(PH_EMENU_DX_SHADOW_OPACITY);
                                dropShadow->put_Offset({ 0.0f, PH_EMENU_DX_SHADOW_OFFSET_Y * Scale, 0.0f });
                                dropShadow->put_Color({ 255, 0, 0, 0 }); // Black
#endif
                            }

                            children->InsertAtBottom(shadowVisual.Get());
                        }
                    }
                }
#endif

            }
        }
    }
#else
#ifdef PH_EMENU_DX_ENABLE_SWAPCHAIN
    status = MenuVisual->SetContent(SwapChain);
#else
    status = MenuVisual->SetContent(DCompSurface);
#endif

    // Whole pixels keep the menu text crisp inside the padded window.
    if (HR_SUCCESS(status))
        status = MenuVisual->SetOffsetX(static_cast<FLOAT>(GetSurfacePaddingPixels()));
    if (HR_SUCCESS(status))
        status = MenuVisual->SetOffsetY(static_cast<FLOAT>(GetSurfacePaddingPixels()));
#endif

    return status;
}

HRESULT PH_EMENU_DX::SetBackfaceVisibilityOnDCompMenuVisual()
{
#ifdef PH_EMENU_DX_ENABLE_WINRT_COMPOSITION
    return S_OK;
#else
    return MenuVisual->SetBackFaceVisibility(DCOMPOSITION_BACKFACE_VISIBILITY_HIDDEN);
#endif
}

HRESULT PH_EMENU_DX::AddDCompMenuVisualToRootVisual()
{
#ifdef PH_EMENU_DX_ENABLE_WINRT_COMPOSITION
    ComPtr<IVisualCollection> children;
    ComPtr<IVisual> visual;
    HRESULT status = RootVisual->get_Children(&children);
    if (HR_SUCCESS(status))
        status = MenuVisual->QueryInterface(IID_PPV_ARGS(&visual));
    if (HR_SUCCESS(status))
        status = children->InsertAtTop(visual.Get());
    return status;
#else
    if (!HostVisual)
        return RootVisual->AddVisual(MenuVisual, FALSE, nullptr);

    HRESULT status = RootVisual->AddVisual(HostVisual, FALSE, nullptr);

    // Z-order is explicit: with a nullptr reference visual, insertAbove=TRUE means
    // bottom-most and FALSE means top-most, so the menu is anchored to the shadow.
    if (HR_SUCCESS(status) && ShadowVisual)
        status = HostVisual->AddVisual(ShadowVisual, FALSE, nullptr);
    if (HR_SUCCESS(status))
        status = HostVisual->AddVisual(MenuVisual, TRUE, ShadowVisual);
    return status;
#endif
}

VOID PH_EMENU_DX::InitializeShadow()
{
    SurfacePadding = 0.0f;
    ShadowEnabled = FALSE;
    AnimationsEnabled = FALSE;
    OpenAnimationPending = FALSE;

#ifdef PH_EMENU_DX_ENABLE_WINRT_COMPOSITION
#ifdef PH_EMENU_DX_ENABLE_DROPSHADOW
    SurfacePadding = PH_EMENU_DX_SHADOW_PADDING;
#endif
#else
    BOOL dropShadow = TRUE;

    // Performance Options: "Show shadows under windows".
    if (!PhGetSystemParametersInfo(SPI_GETDROPSHADOW, 0, &dropShadow, 0))
        dropShadow = TRUE;

    if (dropShadow)
    {
        // The shadow is pre-rendered with D2D, so any DirectComposition device works.
        PhAcquireQueuedLockExclusive(&SharedLock);
        if (HR_SUCCESS(InitializeSharedResources()))
            ShadowEnabled = SharedDCompDevice != nullptr;
        PhReleaseQueuedLockExclusive(&SharedLock);
    }

    if (ShadowEnabled)
        SurfacePadding = PH_EMENU_DX_DC_SHADOW_PADDING;

#ifdef PH_EMENU_DX_ENABLE_HOST_ANIMATION
    {
        BOOL clientAnimations = TRUE;

        // Settings: "Animation effects".
        if (!PhGetSystemParametersInfo(SPI_GETCLIENTAREAANIMATION, 0, &clientAnimations, 0))
            clientAnimations = TRUE;

        AnimationsEnabled = !!clientAnimations;
        // A submenu replacing a sibling appears at once so the shadow doesn't blink.
        OpenAnimationPending = AnimationsEnabled && !(ParentContext && ParentContext->SubMenuSwitching);
    }
#endif
#endif
}

LONG PH_EMENU_DX::GetSurfacePaddingPixels()
{
    return static_cast<LONG>(SurfacePadding * Scale);
}

BOOLEAN PH_EMENU_DX::IsPointInMenuBody(
    _In_ POINT Point
    )
{
    ULONG currentThreadId = HandleToUlong(NtCurrentThreadId());
    BOOLEAN result = FALSE;

    PhAcquireQueuedLockShared(&MenuListLock);

    if (MenuList)
    {
        for (ULONG i = 0; std::cmp_less(i, MenuList->Count); i++)
        {
            PPH_EMENU_DX menuContext = static_cast<PPH_EMENU_DX>(MenuList->Items[i]);
            RECT windowRect;
            LONG paddingPixels;

            if (menuContext->ThreadId != currentThreadId || !menuContext->WindowHandle)
                continue;
            if (!GetWindowRect(menuContext->WindowHandle, &windowRect))
                continue;

            // The shadow padding belongs to the window but not to the menu.
            paddingPixels = menuContext->GetSurfacePaddingPixels();
            windowRect.left += paddingPixels;
            windowRect.top += paddingPixels;
            windowRect.right -= paddingPixels;
            windowRect.bottom -= paddingPixels;

            if (PtInRect(&windowRect, Point))
            {
                result = TRUE;
                break;
            }
        }
    }

    PhReleaseQueuedLockShared(&MenuListLock);

    return result;
}

#if !defined(PH_EMENU_DX_ENABLE_WINRT_COMPOSITION) && defined(PH_EMENU_DX_ENABLE_HOST_ANIMATION)
// Cubic Bezier easing with implicit end points (0,0) and (1,1), as used by CSS and
// the Fluent motion curves.
static double EMenuBezierCoordinate(
    _In_ double P1,
    _In_ double P2,
    _In_ double T
    )
{
    double u = 1.0 - T;

    return 3.0 * u * u * T * P1 + 3.0 * u * T * T * P2 + T * T * T;
}

static double EMenuBezierDerivative(
    _In_ double P1,
    _In_ double P2,
    _In_ double T
    )
{
    double u = 1.0 - T;

    return 3.0 * u * u * P1 + 6.0 * u * T * (P2 - P1) + 3.0 * T * T * (1.0 - P2);
}

// Evaluates the easing curve and its slope (dy/dx) at the given progress X.
static VOID EMenuEvaluateEase(
    _In_ double X,
    _Out_ double* Y,
    _Out_ double* Slope
    )
{
    const double x1 = PH_EMENU_DX_EASE_DECELERATE_X1;
    const double y1 = PH_EMENU_DX_EASE_DECELERATE_Y1;
    const double x2 = PH_EMENU_DX_EASE_DECELERATE_X2;
    const double y2 = PH_EMENU_DX_EASE_DECELERATE_Y2;
    double low = 0.0;
    double high = 1.0;
    double t = X;
    double dx;
    double dy;

    for (ULONG i = 0; std::cmp_less(i, 40); i++)
    {
        t = (low + high) / 2.0;

        if (EMenuBezierCoordinate(x1, x2, t) < X)
            low = t;
        else
            high = t;
    }

    dx = EMenuBezierDerivative(x1, x2, t);
    dy = EMenuBezierDerivative(y1, y2, t);

    *Y = EMenuBezierCoordinate(y1, y2, t);
    *Slope = dx > 1e-6 ? dy / dx : 0.0;
}

// DirectComposition animations are piecewise cubic polynomials in time. The Bezier
// easing is sampled into Hermite segments (exact value and slope at each knot) so
// the animation follows the real curve instead of a single-cubic approximation.
static HRESULT EMenuBuildAnimation(
    _In_ IDCompositionAnimation* Animation,
    _In_ FLOAT From,
    _In_ FLOAT To,
    _In_ double Duration,
    _In_ BOOLEAN Eased
    )
{
    HRESULT status;
    double delta = static_cast<double>(To) - static_cast<double>(From);

    if (!Eased)
    {
        status = Animation->AddCubic(0.0, From, static_cast<FLOAT>(delta / Duration), 0.0f, 0.0f);
    }
    else
    {
        double segment = Duration / PH_EMENU_DX_EASE_SEGMENTS;
        double y0;
        double slope0;

        status = S_OK;
        EMenuEvaluateEase(0.0, &y0, &slope0);

        for (ULONG i = 0; std::cmp_less(i, PH_EMENU_DX_EASE_SEGMENTS) && HR_SUCCESS(status); i++)
        {
            double y1;
            double slope1;
            double v0;
            double v1;
            double m0;
            double m1;

            EMenuEvaluateEase(static_cast<double>(i + 1) / PH_EMENU_DX_EASE_SEGMENTS, &y1, &slope1);

            v0 = From + delta * y0;
            v1 = From + delta * y1;
            m0 = delta * slope0 / Duration;
            m1 = delta * slope1 / Duration;

            status = Animation->AddCubic(
                segment * i,
                static_cast<FLOAT>(v0),
                static_cast<FLOAT>(m0),
                static_cast<FLOAT>(3.0 * (v1 - v0) / (segment * segment) - (2.0 * m0 + m1) / segment),
                static_cast<FLOAT>(-2.0 * (v1 - v0) / (segment * segment * segment) + (m0 + m1) / (segment * segment))
                );

            y0 = y1;
            slope0 = slope1;
        }
    }

    if (HR_SUCCESS(status))
        status = Animation->End(Duration, To);

    return status;
}
#endif

HRESULT PH_EMENU_DX::CreateDCompShadowTree()
{
#ifdef PH_EMENU_DX_ENABLE_WINRT_COMPOSITION
    return S_OK;
#else
    // Everything here is best effort: without the host or the shadow the menu
    // visual still renders, just without the elevation effect or entrance motion.
    LONG paddingPixels = GetSurfacePaddingPixels();

    if (HR_FAILED(DCompDevice->CreateVisual(&HostVisual)))
    {
        HostVisual = nullptr;
        OpenAnimationPending = FALSE;
        return S_OK;
    }

    HostVisual->QueryInterface(IID_PPV_ARGS(&HostVisual3));

#ifdef PH_EMENU_DX_ENABLE_HOST_ANIMATION
    if (OpenAnimationPending)
    {
        FLOAT windowWidth = static_cast<FLOAT>(static_cast<LONG>(Width * Scale) + paddingPixels * 2);
        FLOAT windowHeight = static_cast<FLOAT>(static_cast<LONG>(Height * Scale) + paddingPixels * 2);
        BOOLEAN prepared = FALSE;

        // Start fully transparent and slightly small; StartOpenAnimation plays it
        // once the window is on screen.
        if (HostVisual3 && HR_SUCCESS(DCompDevice->CreateScaleTransform(&HostScaleTransform)))
        {
            prepared =
                HR_SUCCESS(HostScaleTransform->SetCenterX(windowWidth / 2.0f)) &&
                HR_SUCCESS(HostScaleTransform->SetCenterY(windowHeight / 2.0f)) &&
                HR_SUCCESS(HostScaleTransform->SetScaleX(PH_EMENU_DX_OPEN_SCALE_FROM)) &&
                HR_SUCCESS(HostScaleTransform->SetScaleY(PH_EMENU_DX_OPEN_SCALE_FROM)) &&
                HR_SUCCESS(HostVisual->SetTransform(HostScaleTransform)) &&
                HR_SUCCESS(HostVisual3->SetOpacity(0.0f));
        }

        if (!prepared)
        {
            if (HostVisual3)
                HostVisual3->SetOpacity(1.0f);
            if (HostScaleTransform)
            {
                HostVisual->SetTransform(static_cast<IDCompositionTransform*>(nullptr));
                HostScaleTransform->Release();
                HostScaleTransform = nullptr;
            }

            OpenAnimationPending = FALSE;
        }
    }
#endif

    if (ShadowEnabled)
    {
        HRESULT status;
        LONG bodyWidth = static_cast<LONG>(Width * Scale);
        LONG bodyHeight = static_cast<LONG>(Height * Scale);

        status = DCompDevice->CreateVisual(&ShadowVisual);

        // The surface covers the menu plus the padding so the blurred output can
        // never be clipped by the bounds of the visual.
        if (HR_SUCCESS(status))
        {
            status = DCompDevice->CreateSurface(
                static_cast<UINT>(bodyWidth + paddingPixels * 2),
                static_cast<UINT>(bodyHeight + paddingPixels * 2),
                DXGI_FORMAT_B8G8R8A8_UNORM,
                DXGI_ALPHA_MODE_PREMULTIPLIED,
                &ShadowSurface
                );
        }

        if (HR_SUCCESS(status))
            status = DrawDCompShadowSurface();
        if (HR_SUCCESS(status))
            status = ShadowVisual->SetContent(ShadowSurface);
        if (HR_SUCCESS(status))
            status = ShadowVisual->SetOffsetX(0.0f);
        if (HR_SUCCESS(status))
            status = ShadowVisual->SetOffsetY(static_cast<FLOAT>(static_cast<LONG>(PH_EMENU_DX_DC_SHADOW_OFFSET_Y * Scale)));

        if (HR_FAILED(status))
        {
            dprintf("PhEMenuDx: Shadow setup failed with 0x%08x\n", status);

            if (ShadowSurface)
            {
                ShadowSurface->Release();
                ShadowSurface = nullptr;
            }
            if (ShadowVisual)
            {
                ShadowVisual->Release();
                ShadowVisual = nullptr;
            }
        }
    }

    return S_OK;
#endif
}

VOID PH_EMENU_DX::DestroyDCompShadowTree()
{
#ifndef PH_EMENU_DX_ENABLE_WINRT_COMPOSITION
    if (ShadowSurface)
    {
        ShadowSurface->Release();
        ShadowSurface = nullptr;
    }
    if (ShadowVisual)
    {
        ShadowVisual->Release();
        ShadowVisual = nullptr;
    }
    if (HostScaleTransform)
    {
        HostScaleTransform->Release();
        HostScaleTransform = nullptr;
    }
    if (HostVisual3)
    {
        HostVisual3->Release();
        HostVisual3 = nullptr;
    }
    if (HostVisual)
    {
        HostVisual->Release();
        HostVisual = nullptr;
    }
#endif
}

HRESULT PH_EMENU_DX::DrawDCompShadowSurface()
{
#ifdef PH_EMENU_DX_ENABLE_WINRT_COMPOSITION
    return S_OK;
#else
    HRESULT status;
    HRESULT endStatus;
    ComPtr<ID2D1Bitmap1> maskBitmap;
    ComPtr<ID2D1Image> previousTarget;
    ComPtr<ID2D1SolidColorBrush> maskBrush;
    ComPtr<ID2D1DeviceContext> drawingContext;
    ComPtr<ID2D1Effect> shadowEffect;
    FLOAT previousDpiX;
    FLOAT previousDpiY;
    POINT offset;
    LONG paddingPixels = GetSurfacePaddingPixels();
    LONG bodyWidth = static_cast<LONG>(Width * Scale);
    LONG bodyHeight = static_cast<LONG>(Height * Scale);
    FLOAT spreadPixels = static_cast<FLOAT>(static_cast<LONG>(PH_EMENU_DX_DC_SHADOW_SPREAD * Scale));
    D2D1_RECT_F maskRect = D2D1::RectF(
        static_cast<FLOAT>(paddingPixels) - spreadPixels,
        static_cast<FLOAT>(paddingPixels) - spreadPixels,
        static_cast<FLOAT>(paddingPixels + bodyWidth) + spreadPixels,
        static_cast<FLOAT>(paddingPixels + bodyHeight) + spreadPixels
        );
    // A dark desktop and dark menu swallow a light-theme shadow, so it needs more weight.
    D2D1_VECTOR_4F shadowColor = { 0.0f, 0.0f, 0.0f, IsDarkMode ? PH_EMENU_DX_DC_SHADOW_OPACITY_DARK : PH_EMENU_DX_DC_SHADOW_OPACITY };

    // The blur is rendered into the surface once instead of being a live composition
    // effect. DWM re-evaluates a visual effect for every dirty rect beneath it (hover
    // redraws of this menu or of the parent under the shadow), which makes the shadow
    // flicker and tear, and the effect can be missing from the first frames on screen.

    // Rasterize the menu-shaped mask with the resource context; it has no
    // DirectComposition drawing session of its own. Everything is in device pixels:
    // the visual offsets and the blur's standard deviation are pixel based too.
    status = D2DDeviceContext->CreateBitmap(
        D2D1::SizeU(static_cast<UINT32>(bodyWidth + paddingPixels * 2), static_cast<UINT32>(bodyHeight + paddingPixels * 2)),
        nullptr,
        0,
        D2D1::BitmapProperties1(D2D1_BITMAP_OPTIONS_TARGET, D2D1::PixelFormat(DXGI_FORMAT_B8G8R8A8_UNORM, D2D1_ALPHA_MODE_PREMULTIPLIED)),
        &maskBitmap
        );

    if (HR_FAILED(status))
        return status;

    D2DDeviceContext->GetTarget(&previousTarget);
    D2DDeviceContext->GetDpi(&previousDpiX, &previousDpiY);
    D2DDeviceContext->SetTarget(maskBitmap.Get());
    D2DDeviceContext->SetDpi(PH_EMENU_DX_DPI_BASE, PH_EMENU_DX_DPI_BASE);
    D2DDeviceContext->BeginDraw();
    D2DDeviceContext->SetAntialiasMode(D2D1_ANTIALIAS_MODE_PER_PRIMITIVE);
    D2DDeviceContext->SetTransform(D2D1::Matrix3x2F::Identity());
    D2DDeviceContext->Clear(PH_EMENU_DX_COLOR_TRANSPARENT);

    // Only the alpha channel feeds the shadow, so the mask is opaque and matches
    // the menu geometry exactly.
    status = D2DDeviceContext->CreateSolidColorBrush(D2D1::ColorF(D2D1::ColorF::White), &maskBrush);

    if (HR_SUCCESS(status))
    {
        if (FlagOn(Flags, PH_EMENU_SHOW_NO_ROUND_CORNERS))
        {
            D2DDeviceContext->FillRectangle(&maskRect, maskBrush.Get());
        }
        else
        {
            D2D1_ROUNDED_RECT maskRoundedRect = D2D1::RoundedRect(
                maskRect,
                Metrics->RoundedCorner * Scale + spreadPixels,
                Metrics->RoundedCorner * Scale + spreadPixels
                );

            D2DDeviceContext->FillRoundedRectangle(&maskRoundedRect, maskBrush.Get());
        }
    }

    endStatus = D2DDeviceContext->EndDraw();
    D2DDeviceContext->SetTarget(previousTarget.Get());
    D2DDeviceContext->SetDpi(previousDpiX, previousDpiY);

    if (HR_SUCCESS(status))
        status = endStatus;
    if (HR_FAILED(status))
    {
        dprintf("PhEMenuDx: Shadow mask draw failed with 0x%08x\n", status);
        return status;
    }

    // Blur the mask into the shadow surface.
    status = ShadowSurface->BeginDraw(nullptr, IID_PPV_ARGS(&drawingContext), &offset);

    if (HR_FAILED(status))
        return status;

    drawingContext->SetDpi(PH_EMENU_DX_DPI_BASE, PH_EMENU_DX_DPI_BASE);
    drawingContext->SetTransform(D2D1::Matrix3x2F::Identity());
    drawingContext->Clear(PH_EMENU_DX_COLOR_TRANSPARENT);

    status = drawingContext->CreateEffect(CLSID_D2D1Shadow, &shadowEffect);

    if (HR_SUCCESS(status))
    {
        shadowEffect->SetInput(0, maskBitmap.Get());
        status = shadowEffect->SetValue(D2D1_SHADOW_PROP_BLUR_STANDARD_DEVIATION, PH_EMENU_DX_DC_SHADOW_BLUR_RADIUS * Scale / 3.0f);
    }
    if (HR_SUCCESS(status))
        status = shadowEffect->SetValue(D2D1_SHADOW_PROP_COLOR, shadowColor);
    if (HR_SUCCESS(status))
        status = shadowEffect->SetValue(D2D1_SHADOW_PROP_OPTIMIZATION, D2D1_SHADOW_OPTIMIZATION_QUALITY);
    if (HR_SUCCESS(status))
    {
        drawingContext->DrawImage(shadowEffect.Get(), D2D1::Point2F(static_cast<FLOAT>(offset.x), static_cast<FLOAT>(offset.y)));
        ExcludeOccludersFromShadow(drawingContext.Get(), offset);
    }

    endStatus = ShadowSurface->EndDraw();

    if (HR_SUCCESS(status))
        status = endStatus;
    if (HR_FAILED(status))
    {
        dprintf("PhEMenuDx: Shadow surface draw failed with 0x%08x\n", status);
    }

    return status;
#endif
}

// A submenu window overlaps its parent menus, so its shadow would darken their edge.
// The shadow is a static bitmap, so the parent bodies are cut out of the bitmap itself
// (with their rounded corners) instead of with a window region, which DWM applies out
// of step with the composition. The submenu's own body is a separate visual above the
// shadow, so the part of it that overlaps a parent still sits on top. The surfaces the
// menu was opened from (the menu bar, toolbar menu bar or button) are cut out the same
// way, matching the shadow of the #32768 menu class (PhUpdateWindowShadow).
VOID PH_EMENU_DX::ExcludeOccludersFromShadow(
    _In_ ID2D1DeviceContext* DrawingContext,
    _In_ POINT Offset
    )
{
    ComPtr<ID2D1SolidColorBrush> clearBrush;
    PPH_EMENU_DX rootContext;
    RECT ownerRects[8];
    ULONG ownerCount;
    RECT windowRect;
    FLOAT originX;
    FLOAT originY;

    if (!GetWindowRect(WindowHandle, &windowRect))
        return;
    if (HR_FAILED(DrawingContext->CreateSolidColorBrush(PH_EMENU_DX_COLOR_TRANSPARENT, &clearBrush)))
        return;

    // Screen pixels to shadow surface pixels: the surface update starts at Offset and
    // the shadow visual is moved down by the shadow offset.
    originX = static_cast<FLOAT>(Offset.x - windowRect.left);
    originY = static_cast<FLOAT>(Offset.y - windowRect.top - static_cast<LONG>(PH_EMENU_DX_DC_SHADOW_OFFSET_Y * Scale));

    // Copy replaces the shadow pixels with transparency instead of blending over them.
    DrawingContext->SetPrimitiveBlend(D2D1_PRIMITIVE_BLEND_COPY);
    DrawingContext->SetTransform(D2D1::Matrix3x2F::Translation(originX, originY));

    // Submenus are owned by their parent menu, so the surfaces come from the root
    // menu's owner. The menu window never activates, so the button that opened the
    // menu keeps the focus.
    for (rootContext = this; rootContext->ParentContext; rootContext = rootContext->ParentContext)
        NOTHING;

    ownerCount = PhGetWindowShadowOwnerOccluders(
        rootContext->OwnerWindowHandle,
        GetFocus(),
        ownerRects,
        std::size(ownerRects)
        );

    for (ULONG i = 0; std::cmp_less(i, ownerCount); i++)
    {
        D2D1_RECT_F ownerRect = D2D1::RectF(
            static_cast<FLOAT>(ownerRects[i].left),
            static_cast<FLOAT>(ownerRects[i].top),
            static_cast<FLOAT>(ownerRects[i].right),
            static_cast<FLOAT>(ownerRects[i].bottom)
            );

        DrawingContext->FillRectangle(&ownerRect, clearBrush.Get());
    }

    for (PPH_EMENU_DX parent = ParentContext; parent; parent = parent->ParentContext)
    {
        RECT parentRect;
        LONG parentPadding;
        D2D1_RECT_F bodyRect;

        if (!parent->WindowHandle || !GetWindowRect(parent->WindowHandle, &parentRect))
            continue;

        // The shadow padding belongs to the parent window but not to its menu.
        parentPadding = parent->GetSurfacePaddingPixels();
        bodyRect = D2D1::RectF(
            static_cast<FLOAT>(parentRect.left + parentPadding),
            static_cast<FLOAT>(parentRect.top + parentPadding),
            static_cast<FLOAT>(parentRect.right - parentPadding),
            static_cast<FLOAT>(parentRect.bottom - parentPadding)
            );

        if (FlagOn(parent->Flags, PH_EMENU_SHOW_NO_ROUND_CORNERS))
        {
            DrawingContext->FillRectangle(&bodyRect, clearBrush.Get());
        }
        else
        {
            D2D1_ROUNDED_RECT bodyRoundedRect = D2D1::RoundedRect(
                bodyRect,
                parent->Metrics->RoundedCorner * parent->Scale,
                parent->Metrics->RoundedCorner * parent->Scale
                );

            DrawingContext->FillRoundedRectangle(&bodyRoundedRect, clearBrush.Get());
        }
    }

    DrawingContext->SetPrimitiveBlend(D2D1_PRIMITIVE_BLEND_SOURCE_OVER);
    DrawingContext->SetTransform(D2D1::Matrix3x2F::Identity());
}

VOID PH_EMENU_DX::StartOpenAnimation()
{
#if !defined(PH_EMENU_DX_ENABLE_WINRT_COMPOSITION) && defined(PH_EMENU_DX_ENABLE_HOST_ANIMATION)
    ComPtr<IDCompositionAnimation> opacityAnimation;
    ComPtr<IDCompositionAnimation> scaleXAnimation;
    ComPtr<IDCompositionAnimation> scaleYAnimation;
    HRESULT status;

    if (!OpenAnimationPending)
        return;

    OpenAnimationPending = FALSE;

    if (!HostVisual3 || !HostScaleTransform)
        return;

    status = DCompDevice->CreateAnimation(&opacityAnimation);
    if (HR_SUCCESS(status))
        status = DCompDevice->CreateAnimation(&scaleXAnimation);
    if (HR_SUCCESS(status))
        status = DCompDevice->CreateAnimation(&scaleYAnimation);

    // Opacity and scale are applied to the host, so the shadow and the menu
    // content always fade and scale together.
    if (HR_SUCCESS(status))
        status = EMenuBuildAnimation(opacityAnimation.Get(), 0.0f, 1.0f, PH_EMENU_DX_OPEN_FADE_DURATION, TRUE);
    if (HR_SUCCESS(status))
        status = EMenuBuildAnimation(scaleXAnimation.Get(), PH_EMENU_DX_OPEN_SCALE_FROM, 1.0f, PH_EMENU_DX_OPEN_SCALE_DURATION, TRUE);
    if (HR_SUCCESS(status))
        status = EMenuBuildAnimation(scaleYAnimation.Get(), PH_EMENU_DX_OPEN_SCALE_FROM, 1.0f, PH_EMENU_DX_OPEN_SCALE_DURATION, TRUE);
    if (HR_SUCCESS(status))
        status = HostVisual3->SetOpacity(opacityAnimation.Get());
    if (HR_SUCCESS(status))
        status = HostScaleTransform->SetScaleX(scaleXAnimation.Get());
    if (HR_SUCCESS(status))
        status = HostScaleTransform->SetScaleY(scaleYAnimation.Get());

    if (HR_FAILED(status))
    {
        // Never leave the menu invisible if the animation could not be set up.
        HostVisual3->SetOpacity(1.0f);
        HostScaleTransform->SetScaleX(1.0f);
        HostScaleTransform->SetScaleY(1.0f);
    }

    DCompDevice->Commit();
#endif
}

VOID PH_EMENU_DX::PlayCloseAnimation()
{
#if !defined(PH_EMENU_DX_ENABLE_WINRT_COMPOSITION) && defined(PH_EMENU_DX_ENABLE_HOST_ANIMATION)
    ComPtr<IDCompositionAnimation> opacityAnimation;

    // Only the root plays the exit: submenus are torn down before it and stacking
    // a fade per level would delay the selected command.
    if (IsSubMenu || !AnimationsEnabled || !HostVisual3 || !DCompDevice || !WindowHandle)
        return;
    if (!IsWindowVisible(WindowHandle))
        return;

    if (HR_SUCCESS(DCompDevice->CreateAnimation(&opacityAnimation)) &&
        HR_SUCCESS(EMenuBuildAnimation(opacityAnimation.Get(), 1.0f, 0.0f, PH_EMENU_DX_CLOSE_FADE_DURATION, FALSE)) &&
        HR_SUCCESS(HostVisual3->SetOpacity(opacityAnimation.Get())) &&
        HR_SUCCESS(DCompDevice->Commit()))
    {
        // The composition engine runs the animation; keep the window alive for its
        // duration (plus a frame) so the shadow and content fade out together.
        PhDelayExecution(static_cast<ULONG>(PH_EMENU_DX_CLOSE_FADE_DURATION * 1000.0) + 16);
    }
#endif
}

HRESULT PH_EMENU_DX::CommitDCompDevice()
{
#ifndef PH_EMENU_DX_ENABLE_WINRT_COMPOSITION
    return DCompDevice->Commit();
#else
    return S_OK;
#endif
}

VOID PH_EMENU_DX::CommitUpdate()
{
    HRESULT status = S_OK;

    if (!D2DDeviceContext || !TextBrush || !ShortcutBrush || !HoverBrush || !BorderBrush || !SeparatorBrush)
        return;

    // Check for device removal or reset
    {
#ifdef PH_EMENU_DX_ENABLE_D3D12
        status = D3D12Device->GetDeviceRemovedReason();
#else
        ComPtr<ID3D11Device> d3d11Device;
        if (HR_SUCCESS(((IUnknown*)D3DDevice)->QueryInterface(IID_PPV_ARGS(&d3d11Device))))
        {
            status = d3d11Device->GetDeviceRemovedReason();
        }
        else
        {
            ComPtr<ID3D10Device> d3d10Device;
            if (HR_SUCCESS(((IUnknown*)D3DDevice)->QueryInterface(IID_PPV_ARGS(&d3d10Device))))
            {
                status = d3d10Device->GetDeviceRemovedReason();
            }
        }
#endif
        if (status == DXGI_ERROR_DEVICE_REMOVED || status == DXGI_ERROR_DEVICE_RESET)
        {
            // Every open menu observes the removal. Only the first one (still on the
            // current generation) drops the shared objects; the rest just rebuild so
            // they don't tear down devices another menu has already recreated.
            PhAcquireQueuedLockExclusive(&SharedLock);
            if (ResourceGeneration == SharedGeneration)
                PH_EMENU_DX::ReleaseSharedResources();
            PhReleaseQueuedLockExclusive(&SharedLock);

            ReleaseCachedLayouts();
            ReleaseResources();
            status = InitializeTextFormats();
            if (HR_SUCCESS(status))
            {
                RebuildTextLayouts();
                status = CreateResources();
            }
            if (HR_SUCCESS(status))
                status = InitializeBrushes();
            if (HR_FAILED(status))
                return;
        }
    }

    status = DrawDCompMenuSurface();
    if (HR_SUCCESS(status))
        status = CommitDCompDevice();

    if (HR_FAILED(status))
    {
        dprintf("PhEMenuDx: Surface update failed with 0x%08x\n", status);
    }
}

VOID PH_EMENU_DX::Render()
{
    D2D1::Matrix3x2F baseTransform;
    D2DDeviceContext->GetTransform(&baseTransform);

    // Determine background transparency based on theme (Acrylic, Mica, or Opaque)
    FLOAT alpha = PH_EMENU_DX_OPAQUE_ALPHA_LEVEL;

#ifndef PH_EMENU_DX_ENABLE_DROPSHADOW
    if (IsAcrylic)
        alpha = PH_EMENU_DX_ACRYLIC_TINT_ALPHA_LEVEL;
    else if (IsMica)
        alpha = PH_EMENU_DX_MICA_TINT_ALPHA_LEVEL;
#endif

    D2D1_COLOR_F backgroundColor = IsDarkMode ? PH_EMENU_DX_DARK_BACKGROUND : PH_EMENU_DX_LIGHT_BACKGROUND;
    D2D1_RECT_F rect = D2D1::RectF(0.0f, 0.0f, Width, Height);
    D2D1_ROUNDED_RECT roundedRect = D2D1::RoundedRect(rect, Metrics->RoundedCorner, Metrics->RoundedCorner);
    BOOLEAN noRound = BooleanFlagOn(Flags, PH_EMENU_SHOW_NO_ROUND_CORNERS);

    backgroundColor.a = alpha;

    // Clear the render target with transparency
    D2DDeviceContext->SetTransform(D2D1::Matrix3x2F::Identity());
    D2DDeviceContext->Clear(PH_EMENU_DX_COLOR_TRANSPARENT);
    D2DDeviceContext->SetTransform(baseTransform);

    // Draw the menu background
    ID2D1SolidColorBrush* backgroundBrush = nullptr;
    if (alpha > PH_EMENU_DX_ALPHA_THRESHOLD && HR_SUCCESS(D2DDeviceContext->CreateSolidColorBrush(backgroundColor, &backgroundBrush)))
    {
        if (noRound)
        {
            D2DDeviceContext->FillRectangle(&rect, backgroundBrush);
        }
        else
        {
            if (ClipGeometry)
            {
                // Use a layer for high-quality rounded clipping if available
                D2D1_LAYER_PARAMETERS layerParams = D2D1::LayerParameters(D2D1::InfiniteRect(), ClipGeometry);
                D2DDeviceContext->PushLayer(&layerParams, nullptr);
                D2DDeviceContext->FillRectangle(&rect, backgroundBrush);
            }
            else
            {
                D2DDeviceContext->FillRoundedRectangle(&roundedRect, backgroundBrush);
            }
        }

        backgroundBrush->Release();
    }

    if (!noRound && ClipGeometry && !backgroundBrush)
    {
        D2D1_LAYER_PARAMETERS layerParams = D2D1::LayerParameters(D2D1::InfiniteRect(), ClipGeometry);
        D2DDeviceContext->PushLayer(&layerParams, nullptr);
    }

    // Set up clipping for the scrollable area
    FLOAT clipTop = 0.0f;
    FLOAT clipBottom = Height;

    if (ScrollViewTotalHeight > Height)
    {
        if (ScrollOffset > 0)
            clipTop = ItemHeight;
        if (ScrollOffset < (ScrollViewTotalHeight - Height))
            clipBottom = Height - ItemHeight;
    }

    D2D1_RECT_F clipRect = D2D1::RectF(0, clipTop, Width, clipBottom);
    D2DDeviceContext->PushAxisAlignedClip(clipRect, D2D1_ANTIALIAS_MODE_PER_PRIMITIVE);

    // Apply scroll translation for item rendering
    D2DDeviceContext->SetTransform(baseTransform * D2D1::Matrix3x2F::Translation(0.0f, -ScrollOffset));

    FLOAT y = VerticalPadding;

    // Render each menu item

    for (ULONG i = 0; std::cmp_less(i, Menu->Items->Count); i++)
    {
        PPH_EMENU_ITEM item = static_cast<PPH_EMENU_ITEM>(Menu->Items->Items[i]);
        FLOAT currentHeight = FlagOn(item->Flags, PH_EMENU_SEPARATOR) ? SeparatorHeight : ItemHeight;

        // Skip items that are outside the visible scroll region

        if (y + currentHeight >= ScrollOffset && y <= ScrollOffset + Height)
        {
            D2D1_RECT_F itemRect = D2D1::RectF(Metrics->HoverMargin, y, Width - Metrics->HoverMargin, y + currentHeight);

            // Draw hover highlight if applicable

            if (item == HoverItem && !FlagOn(item->Flags, PH_EMENU_DISABLED))
            {
                D2D1_ROUNDED_RECT roundedHover = D2D1::RoundedRect(itemRect, Metrics->HoverCorner, Metrics->HoverCorner);
                D2DDeviceContext->FillRoundedRectangle(&roundedHover, HoverBrush);
            }

            // Draw separator or item content

            if (FlagOn(item->Flags, PH_EMENU_SEPARATOR))
            {
                // Whole device pixels, centered on a pixel boundary, so the line stays sharp.
                FLOAT lineWidth = Metrics->SeparatorWidth * HairlineWidth;
                FLOAT lineY = (floorf((y + currentHeight / PH_EMENU_DX_HALF_FACTOR - lineWidth / PH_EMENU_DX_HALF_FACTOR) * Scale) / Scale) + lineWidth / PH_EMENU_DX_HALF_FACTOR;

                D2DDeviceContext->DrawLine(D2D1::Point2F(Metrics->SeparatorInset, lineY), D2D1::Point2F(Width - Metrics->SeparatorInset, lineY), SeparatorBrush, lineWidth);
            }
            else
            {
                // Handle icon/bitmap creation and rendering

                if (std::cmp_less(i, LayoutCount) && ItemBitmaps && !ItemBitmaps[i] && item->Bitmap)
                {
                    // Bitmap and Icon share storage in PH_EMENU_ITEM. PhShowEMenu callers
                    // supply HBITMAPs; anything that isn't a GDI bitmap is treated as an icon.
                    if (GetObjectType(item->Bitmap) == OBJ_BITMAP)
                    {
                        CreateBitmapFromHBitmap(item->Bitmap, &ItemBitmaps[i]);
                    }
                    else
                    {
                        CreateBitmapFromIcon(item->Icon, &ItemBitmaps[i]);
                    }
                }

                if (std::cmp_less(i, LayoutCount) && ItemBitmaps && ItemBitmaps[i])
                {
                    D2D1_RECT_F iconRect = D2D1::RectF(Metrics->IconLeft, y + (currentHeight - Metrics->IconSize) / PH_EMENU_DX_HALF_FACTOR, Metrics->IconLeft + Metrics->IconSize, y + (currentHeight + Metrics->IconSize) / PH_EMENU_DX_HALF_FACTOR);

                    // The icon takes the place of the check mark, so a checked item gets
                    // a box behind its icon like the built-in menu.
                    if (FlagOn(item->Flags, PH_EMENU_CHECKED))
                    {
                        D2D1_ROUNDED_RECT checkRect = D2D1::RoundedRect(
                            D2D1::RectF(iconRect.left - PH_EMENU_DX_ICON_CHECK_MARGIN, iconRect.top - PH_EMENU_DX_ICON_CHECK_MARGIN, iconRect.right + PH_EMENU_DX_ICON_CHECK_MARGIN, iconRect.bottom + PH_EMENU_DX_ICON_CHECK_MARGIN),
                            Metrics->HoverCorner,
                            Metrics->HoverCorner
                            );

                        D2DDeviceContext->FillRoundedRectangle(&checkRect, HoverBrush);
                        D2DDeviceContext->DrawRoundedRectangle(&checkRect, BorderBrush, HairlineWidth);
                    }

                    D2DDeviceContext->DrawBitmap(
                        ItemBitmaps[i],
                        &iconRect,
                        FlagOn(item->Flags, PH_EMENU_DISABLED) ? PH_EMENU_DX_DISABLED_ICON_OPACITY : PH_EMENU_DX_FULL_OPACITY
                        );
                }
                else if (FlagOn(item->Flags, PH_EMENU_CHECKED))
                {
                    // Draw checkmark or radio dot

                    ID2D1SolidColorBrush* checkBrush = FlagOn(item->Flags, PH_EMENU_DISABLED) ? ShortcutBrush : TextBrush;

                    if (FlagOn(item->Flags, PH_EMENU_RADIOCHECK))
                    {
                        D2D1_ELLIPSE dot = D2D1::Ellipse(D2D1::Point2F(Metrics->IconLeft + Metrics->IconSize / PH_EMENU_DX_HALF_FACTOR, y + currentHeight / PH_EMENU_DX_HALF_FACTOR), PH_EMENU_DX_RADIO_DOT_RADIUS, PH_EMENU_DX_RADIO_DOT_RADIUS);

                        D2DDeviceContext->FillEllipse(&dot, checkBrush);
                    }
                    else
                    {
                        // Centered on the icon box so checks line up with item icons.
                        FLOAT checkCenter = Metrics->IconLeft + Metrics->IconSize / PH_EMENU_DX_HALF_FACTOR;
                        D2D1_POINT_2F p1 = D2D1::Point2F(checkCenter - PH_EMENU_DX_ARROW_SIZE * 1.5f, y + currentHeight / PH_EMENU_DX_HALF_FACTOR);
                        D2D1_POINT_2F p2 = D2D1::Point2F(checkCenter - PH_EMENU_DX_ARROW_SIZE * 0.5f, y + currentHeight / PH_EMENU_DX_HALF_FACTOR + PH_EMENU_DX_ARROW_SIZE);
                        D2D1_POINT_2F p3 = D2D1::Point2F(checkCenter + PH_EMENU_DX_ARROW_SIZE * 1.5f, y + currentHeight / PH_EMENU_DX_HALF_FACTOR - PH_EMENU_DX_ARROW_SIZE);
                        D2DDeviceContext->DrawLine(p1, p2, checkBrush, PH_EMENU_DX_CHECK_LINE_WIDTH);
                        D2DDeviceContext->DrawLine(p2, p3, checkBrush, PH_EMENU_DX_CHECK_LINE_WIDTH);
                    }
                }

                // Render item text and shortcut layouts
                if (item->Text)
                {
                    if (std::cmp_less(i, LayoutCount) && TextLayouts && TextLayouts[i])
                    {
                        D2D1_POINT_2F origin = D2D1::Point2F(Metrics->TextLeft, y + (currentHeight - TextMetrics[i].height) / PH_EMENU_DX_HALF_FACTOR);

                        D2DDeviceContext->DrawTextLayout(
                            origin,
                            TextLayouts[i],
                            FlagOn(item->Flags, PH_EMENU_DISABLED) ? ShortcutBrush : TextBrush
                            );
                    }

                    if (std::cmp_less(i, LayoutCount) && ShortcutLayouts && ShortcutLayouts[i])
                    {
                        D2D1_POINT_2F origin = D2D1::Point2F(Width - Metrics->ShortcutRightMargin - MaxShortcutWidth,y + (currentHeight - ShortcutMetrics[i].height) / PH_EMENU_DX_HALF_FACTOR);

                        // ShortcutBrush is also the disabled text color.
                        D2DDeviceContext->DrawTextLayout(
                            origin,
                            ShortcutLayouts[i],
                            Metrics->ShortcutColorIsText && !FlagOn(item->Flags, PH_EMENU_DISABLED) ? TextBrush : ShortcutBrush
                            );
                    }
                }

                // Draw submenu indicator arrow
                if (item->Items && std::cmp_greater(item->Items->Count, 0) && !FlagOn(item->Flags, PH_EMENU_DISABLED))
                {
                    ID2D1SolidColorBrush* arrowBrush = TextBrush;
                    D2D1_POINT_2F p1 = D2D1::Point2F(Width - Metrics->ArrowRightMargin, y + currentHeight / PH_EMENU_DX_HALF_FACTOR - PH_EMENU_DX_CHEVRON_SIZE);
                    D2D1_POINT_2F p2 = D2D1::Point2F(Width - Metrics->ArrowTipMargin, y + currentHeight / PH_EMENU_DX_HALF_FACTOR);
                    D2D1_POINT_2F p3 = D2D1::Point2F(Width - Metrics->ArrowRightMargin, y + currentHeight / PH_EMENU_DX_HALF_FACTOR + PH_EMENU_DX_CHEVRON_SIZE);
                    D2DDeviceContext->DrawLine(p1, p2, arrowBrush, HairlineWidth);
                    D2DDeviceContext->DrawLine(p2, p3, arrowBrush, HairlineWidth);
                }
            }
        }
        y += currentHeight;
    }

    // Restore original transform and remove clipping
    D2DDeviceContext->SetTransform(baseTransform);
    D2DDeviceContext->PopAxisAlignedClip();

    // Draw scroll indicator arrows if the content is larger than the window
    if (ScrollViewTotalHeight > Height)
    {
        const FLOAT arrowSize = PH_EMENU_DX_SCROLL_ARROW_SIZE;
        const FLOAT arrowPadding = PH_EMENU_DX_SCROLL_ARROW_PADDING;
        ID2D1PathGeometry* pathGeometry = nullptr;
        ID2D1GeometrySink* geometrySink = nullptr;

        if (ScrollOffset > 0)
        {
            if (HoverUpArrow)
            {
                D2D1_RECT_F arrowRect = D2D1::RectF(0, 0, Width, ItemHeight);
                D2DDeviceContext->FillRectangle(&arrowRect, HoverBrush);
            }

            if (HR_SUCCESS(D2DFactory->CreatePathGeometry(&pathGeometry)))
            {
                if (HR_SUCCESS(pathGeometry->Open(&geometrySink)))
                {
                    D2D1_POINT_2F p1 = D2D1::Point2F(Width / PH_EMENU_DX_HALF_FACTOR, arrowPadding + arrowSize / PH_EMENU_DX_HALF_FACTOR);
                    D2D1_POINT_2F p2 = D2D1::Point2F(Width / PH_EMENU_DX_HALF_FACTOR - arrowSize, arrowPadding + arrowSize * PH_EMENU_DX_ARROW_SIZE_FACTOR);
                    D2D1_POINT_2F p3 = D2D1::Point2F(Width / PH_EMENU_DX_HALF_FACTOR + arrowSize, arrowPadding + arrowSize * PH_EMENU_DX_ARROW_SIZE_FACTOR);

                    geometrySink->BeginFigure(p1, D2D1_FIGURE_BEGIN_FILLED);
                    geometrySink->AddLine(p2);
                    geometrySink->AddLine(p3);
                    geometrySink->EndFigure(D2D1_FIGURE_END_CLOSED);
                    geometrySink->Close();
                    geometrySink->Release();
                }

                D2DDeviceContext->FillGeometry(pathGeometry, TextBrush);
                pathGeometry->Release();
            }
        }

        if (ScrollOffset < (ScrollViewTotalHeight - Height))
        {
            if (HoverDownArrow)
            {
                D2D1_RECT_F arrowRect = D2D1::RectF(0, Height - ItemHeight, Width, Height);
                D2DDeviceContext->FillRectangle(&arrowRect, HoverBrush);
            }

            if (HR_SUCCESS(D2DFactory->CreatePathGeometry(&pathGeometry)))
            {
                if (HR_SUCCESS(pathGeometry->Open(&geometrySink)))
                {
                    D2D1_POINT_2F p1 = D2D1::Point2F(Width / PH_EMENU_DX_HALF_FACTOR, Height - arrowPadding - arrowSize / PH_EMENU_DX_HALF_FACTOR);
                    D2D1_POINT_2F p2 = D2D1::Point2F(Width / PH_EMENU_DX_HALF_FACTOR - arrowSize, Height - arrowPadding - arrowSize * PH_EMENU_DX_ARROW_SIZE_FACTOR);
                    D2D1_POINT_2F p3 = D2D1::Point2F(Width / PH_EMENU_DX_HALF_FACTOR + arrowSize, Height - arrowPadding - arrowSize * PH_EMENU_DX_ARROW_SIZE_FACTOR);

                    geometrySink->BeginFigure(p1, D2D1_FIGURE_BEGIN_FILLED);
                    geometrySink->AddLine(p2);
                    geometrySink->AddLine(p3);
                    geometrySink->EndFigure(D2D1_FIGURE_END_CLOSED);
                    geometrySink->Close();
                    geometrySink->Release();
                }

                D2DDeviceContext->FillGeometry(pathGeometry, TextBrush);
                pathGeometry->Release();
            }
        }
    }

    if (!noRound && ClipGeometry)
    {
        D2DDeviceContext->PopLayer();
    }

    // Draw the final menu border
    FLOAT hairlineOffset = HairlineWidth / PH_EMENU_DX_HALF_FACTOR;
    D2D1_RECT_F borderRect = D2D1::RectF(
        hairlineOffset,
        hairlineOffset,
        Width - hairlineOffset,
        Height - hairlineOffset
        );

    if (noRound)
    {
        D2DDeviceContext->DrawRectangle(&borderRect, BorderBrush, HairlineWidth);
    }
    else
    {
        D2D1_ROUNDED_RECT roundedBorder = D2D1::RoundedRect(borderRect, Metrics->RoundedCorner - hairlineOffset, Metrics->RoundedCorner - hairlineOffset);
        D2DDeviceContext->DrawRoundedRectangle(&roundedBorder, BorderBrush, HairlineWidth);
    }
}

VOID PH_EMENU_DX::CalculateLayout()
{
    // Whole device pixels keep row, hover and separator edges crisp at every scale;
    // layout, drawing and hit-testing all use these rounded values.
    ItemHeight = roundf(Metrics->ItemHeight * Scale) / Scale;
    SeparatorHeight = roundf(Metrics->SeparatorHeight * Scale) / Scale;
    VerticalPadding = roundf(Metrics->VerticalPadding * Scale) / Scale;
    // Lines are one device pixel up to 199%, two up to 299%, and so on, like Windows.
    HairlineWidth = (Scale >= 2.0f ? floorf(Scale) : 1.0f) / Scale;
    MaxTextWidth = 0;
    MaxShortcutWidth = 0;

    // Clear existing layouts before recalculating
    ReleaseCachedLayouts();

    if (!Menu || !Menu->Items || std::cmp_equal(Menu->Items->Count, 0))
    {
        Width = Metrics->MinWidth;
        Height = ItemHeight;
        return;
    }

    ULONG count = Menu->Items->Count;

    // Allocate storage for text layouts and metrics for each menu item
    TextLayouts = static_cast<IDWriteTextLayout**>(PhAllocateZero(sizeof(IDWriteTextLayout*) * count));
    ShortcutLayouts = static_cast<IDWriteTextLayout**>(PhAllocateZero(sizeof(IDWriteTextLayout*) * count));
    TextMetrics = static_cast<DWRITE_TEXT_METRICS*>(PhAllocateZero(sizeof(DWRITE_TEXT_METRICS) * count));
    ShortcutMetrics = static_cast<DWRITE_TEXT_METRICS*>(PhAllocateZero(sizeof(DWRITE_TEXT_METRICS) * count));
    ItemBitmaps = static_cast<ID2D1Bitmap**>(PhAllocateZero(sizeof(ID2D1Bitmap*) * count));
    LayoutCount = count;

    std::span<IDWriteTextLayout*> textLayouts(TextLayouts, count);
    std::span<IDWriteTextLayout*> shortcutLayouts(ShortcutLayouts, count);
    std::span<DWRITE_TEXT_METRICS> textMetrics(TextMetrics, count);
    std::span<DWRITE_TEXT_METRICS> shortcutMetrics(ShortcutMetrics, count);

    // Iterate through items to create text layouts and calculate maximum widths
    for (ULONG i = 0; std::cmp_less(i, count); i++)
    {
        PPH_EMENU_ITEM item = static_cast<PPH_EMENU_ITEM>(Menu->Items->Items[i]);

        if (FlagOn(item->Flags, PH_EMENU_SEPARATOR))
            continue;

        if (item->Text)
        {
            IDWriteTextLayout* layout = nullptr;
            PWSTR shortcut = wcschr(item->Text, L'\b');
            SIZE_T textLen = shortcut ? (shortcut - item->Text) : PhCountStringZ(item->Text);

            // Create layout for the main item text
            if (HR_SUCCESS(CreateTextLayout(item->Text, textLen, TextFormat, PH_EMENU_DX_MAX_TEXT_WIDTH, ItemHeight, &layout)))
            {
                // Match MFS_DEFAULT: the default item is drawn in bold.
                if (FlagOn(item->Flags, PH_EMENU_DEFAULT))
                {
                    DWRITE_TEXT_RANGE range = { 0, UINT32_MAX };
                    layout->SetFontWeight(DWRITE_FONT_WEIGHT_BOLD, range);
                }

                if (HR_SUCCESS(layout->GetMetrics(&textMetrics[i])))
                {
                    textLayouts[i] = layout;

                    MaxTextWidth = std::max(MaxTextWidth, textMetrics[i].width);
                }
                else
                {
                    layout->Release();
                }

                layout = nullptr;
            }

            // Create layout for the shortcut text if present (separated by \b)
            if (shortcut)
            {
                shortcut++;

                if (HR_SUCCESS(DWriteFactory->CreateTextLayout(
                    shortcut,
                    static_cast<UINT32>(PhCountStringZ(shortcut)),
                    ShortcutFormat,
                    PH_EMENU_DX_MAX_TEXT_WIDTH,
                    ItemHeight,
                    &layout
                    )))
                {
                    if (HR_SUCCESS(layout->GetMetrics(&shortcutMetrics[i])))
                    {
                        shortcutLayouts[i] = layout;

                        MaxShortcutWidth = std::max(MaxShortcutWidth, shortcutMetrics[i].width);
                    }
                    else
                    {
                        layout->Release();
                    }
                }
            }
        }
    }

    // Determine final menu width based on column contents and padding
    Width = ceilf(
        Metrics->TextLeft +
        MaxTextWidth +
        Metrics->TextShortcutGap +
        MaxShortcutWidth +
        Metrics->ShortcutRightMargin
        );

    Width = std::max(Width, Metrics->MinWidth);

    // Calculate total height by summing up all items and separators
    Height = VerticalPadding * PH_EMENU_DX_HALF_FACTOR;

    for (ULONG i = 0; std::cmp_less(i, count); i++)
    {
        PPH_EMENU_ITEM item = static_cast<PPH_EMENU_ITEM>(Menu->Items->Items[i]);

        if (FlagOn(item->Flags, PH_EMENU_SEPARATOR))
            Height += SeparatorHeight;
        else
            Height += ItemHeight;
    }

    Height = ceilf(Height);

    ScrollViewTotalHeight = Height;

    // Cap menu height if it exceeds the visible screen area
    if (ScrollViewTotalHeight > ScrollViewVisibleHeight)
    {
        Height = ScrollViewVisibleHeight;
    }

    Height = std::max(Height, PH_EMENU_DX_MIN_HEIGHT);

    HoverItem = nullptr;
    HoverIndex = -1;
}

VOID PH_EMENU_DX::CalculatePosition(
    _In_ LONG X,
    _In_ LONG Y,
    _Out_ PLONG OutX,
    _Out_ PLONG OutY
    )
{
    LONG menuWidth = (LONG)(Width * Scale);
    LONG menuHeight = (LONG)(Height * Scale);
    LONG baseX = X;
    LONG baseY = Y;
    HMONITOR monitor;
    MONITORINFO monitorInfo;
    PH_RECTANGLE workArea;
    POINT point = { X, Y };

    monitor = MonitorFromPoint(point, MONITOR_DEFAULTTONEAREST);
    RtlZeroMemory(&monitorInfo, sizeof(MONITORINFO));
    monitorInfo.cbSize = sizeof(MONITORINFO);

    if (GetMonitorInfo(monitor, &monitorInfo))
    {
        PhRectToRectangle(&workArea, &monitorInfo.rcWork);

        if (FlagOn(Align, PH_ALIGN_RIGHT))
            baseX -= menuWidth;
        else if (!FlagOn(Align, PH_ALIGN_LEFT))
            baseX -= menuWidth / 2;

        // Match TrackPopupMenu: no vertical flag means TPM_VCENTERALIGN.
        if (FlagOn(Align, PH_ALIGN_BOTTOM))
            baseY -= menuHeight;
        else if (!FlagOn(Align, PH_ALIGN_TOP))
            baseY -= menuHeight / 2;

        // Flip edge-aligned menus to the other side of the point; centered
        // menus are shifted into the work area by PhAdjustRectangleToBounds.
        if (std::cmp_greater(baseX + menuWidth, workArea.Left + workArea.Width))
        {
            if (FlagOn(Align, PH_ALIGN_LEFT))
            {
                if (IsSubMenu && ParentContext)
                {
                    // Flip to the left side of the parent menu
                    baseX = X - static_cast<LONG>(ParentContext->Width * Scale) - menuWidth + static_cast<LONG>((PH_EMENU_DX_SUBMENU_X_OFFSET * Scale * 2.0f));
                }
                else
                {
                    baseX = X - menuWidth;
                }
            }
        }

        if (IsSubMenu)
        {
            if (std::cmp_greater(baseY + menuHeight, workArea.Top + workArea.Height))
            {
                LONG itemHeight = static_cast<LONG>(ItemHeight * Scale);
                LONG offset_y = static_cast<LONG>(PH_EMENU_DX_SUBMENU_Y_OFFSET * Scale);
                LONG flippedY = baseY + itemHeight + (offset_y * 2) - menuHeight;

                if (std::cmp_greater_equal(flippedY, workArea.Top))
                {
                    baseY = flippedY;
                }
                else
                {
                    baseY = workArea.Top + workArea.Height - menuHeight;
                }

                baseY = std::max(baseY, workArea.Top);
            }
            else if (std::cmp_less(baseY, workArea.Top))
            {
                baseY = workArea.Top;
            }
        }
        else
        {
            if (std::cmp_greater(baseY + menuHeight, workArea.Top + workArea.Height))
            {
                if (FlagOn(Align, PH_ALIGN_TOP))
                    baseY = Y - menuHeight;
            }

            if (std::cmp_less(baseY, workArea.Top))
            {
                if (FlagOn(Align, PH_ALIGN_BOTTOM))
                    baseY = Y;
            }
        }

        if (std::cmp_less(baseX, workArea.Left))
        {
            if (FlagOn(Align, PH_ALIGN_RIGHT))
                baseX = X;
        }

        PH_RECTANGLE rect;
        rect.Left = baseX;
        rect.Top = baseY;
        rect.Width = menuWidth;
        rect.Height = menuHeight;
        PhAdjustRectangleToBounds(&rect, &workArea);

        baseX = rect.Left;
        baseY = rect.Top;

        if (std::cmp_less(rect.Height, menuHeight))
            Height = static_cast<FLOAT>(rect.Height) / Scale;
        if (std::cmp_less(rect.Width, menuWidth))
            Width = static_cast<FLOAT>(rect.Width) / Scale;
    }
    else
    {
        if (FlagOn(Align, PH_ALIGN_RIGHT))
            baseX -= menuWidth;
        else if (!FlagOn(Align, PH_ALIGN_LEFT))
            baseX -= menuWidth / 2;

        if (FlagOn(Align, PH_ALIGN_BOTTOM))
            baseY -= menuHeight;
        else if (!FlagOn(Align, PH_ALIGN_TOP))
            baseY -= menuHeight / 2;
    }

    *OutX = baseX - GetSurfacePaddingPixels();
    *OutY = baseY - GetSurfacePaddingPixels();
}

VOID PH_EMENU_DX::ReleaseCachedLayouts()
{
    // Use the allocation count, not the live item count, in case the menu changed.
    ULONG count = LayoutCount;

    LayoutCount = 0;

    if (TextLayouts)
    {
        for (ULONG i = 0; std::cmp_less(i, count); i++)
        {
            if (TextLayouts[i])
            {
                TextLayouts[i]->Release();
                TextLayouts[i] = nullptr;
            }
        }

        PhFree(TextLayouts);
        TextLayouts = nullptr;
    }

    if (ShortcutLayouts)
    {
        for (ULONG i = 0; std::cmp_less(i, count); i++)
        {
            if (ShortcutLayouts[i])
            {
                ShortcutLayouts[i]->Release();
                ShortcutLayouts[i] = nullptr;
            }
        }

        PhFree(ShortcutLayouts);
        ShortcutLayouts = nullptr;
    }

    if (TextMetrics)
    {
        PhFree(TextMetrics);
        TextMetrics = nullptr;
    }

    if (ShortcutMetrics)
    {
        PhFree(ShortcutMetrics);
        ShortcutMetrics = nullptr;
    }

    if (ItemBitmaps)
    {
        for (ULONG i = 0; std::cmp_less(i, count); i++)
        {
            if (ItemBitmaps[i])
            {
                ItemBitmaps[i]->Release();
                ItemBitmaps[i] = nullptr;
            }
        }

        PhFree(ItemBitmaps);
        ItemBitmaps = nullptr;
    }
}

HRESULT PH_EMENU_DX::CreateTextLayout(
    _In_ PWSTR Text,
    _In_ SIZE_T TextLen,
    _In_ IDWriteTextFormat* Format,
    _In_ FLOAT MaxWidth,
    _In_ FLOAT MaxHeight,
    _Out_ IDWriteTextLayout** Layout
    ) const
{
    HRESULT status;
    PH_STRING_BUILDER stringBuilder;
    LONG underlineIndex = LONG_ERROR;
    PWSTR processedText;
    SIZE_T processedLength;

    PhInitializeStringBuilder(&stringBuilder, TextLen);

    for (SIZE_T i = 0; std::cmp_less(i, TextLen); i++)
    {
        if (Text[i] == L'&')
        {
            if (std::cmp_less(i + 1, TextLen) && Text[i + 1] == L'&')
            {
                PhAppendCharStringBuilder(&stringBuilder, L'&');
                i++;
            }
            else if (std::cmp_equal(underlineIndex, -1) && std::cmp_less(i + 1, TextLen))
            {
                underlineIndex = static_cast<LONG>(stringBuilder.String->Length / sizeof(WCHAR));
            }

            continue;
        }

        PhAppendCharStringBuilder(&stringBuilder, Text[i]);
    }

    processedText = stringBuilder.String->Buffer;
    processedLength = stringBuilder.String->Length / sizeof(WCHAR);

    status = DWriteFactory->CreateTextLayout(
        processedText,
        static_cast<UINT32>(processedLength),
        Format,
        MaxWidth,
        MaxHeight,
        Layout
        );

    if (HR_SUCCESS(status))
    {
        // Only show underlines if keyboard cues are enabled
        if (std::cmp_not_equal(underlineIndex, LONG_ERROR) && ShowKeyboardCues)
        {
            DWRITE_TEXT_RANGE range =
            {
                static_cast<UINT32>(underlineIndex),
                1
            };
            (*Layout)->SetUnderline(TRUE, range);
        }
    }

    PhDeleteStringBuilder(&stringBuilder);

    return status;
}

HRESULT PH_EMENU_DX::CreateBitmapFromIcon(
    _In_ HICON Icon,
    _Out_ ID2D1Bitmap** Bitmap
    ) const
{
    HRESULT status;
    IWICBitmap* wicBitmap = nullptr;

    // WicFactory is referenced from the shared factory in CreateResources.
    if (!WicFactory)
        return E_FAIL;

    status = WicFactory->CreateBitmapFromHICON(Icon, &wicBitmap);

    if (HR_SUCCESS(status))
    {
        status = D2DDeviceContext->CreateBitmapFromWicBitmap(wicBitmap, Bitmap);
        wicBitmap->Release();
    }

    return status;
}

HRESULT PH_EMENU_DX::CreateBitmapFromHBitmap(
    _In_ HBITMAP BitmapHandle,
    _Out_ ID2D1Bitmap** Bitmap
    ) const
{
    HRESULT status;
    IWICBitmap* wicBitmap = nullptr;

    // WicFactory is referenced from the shared factory in CreateResources.
    if (!WicFactory)
        return E_FAIL;

    status = WicFactory->CreateBitmapFromHBITMAP(
        BitmapHandle,
        nullptr,
        WICBitmapUsePremultipliedAlpha,
        &wicBitmap
        );

    if (HR_SUCCESS(status))
    {
        status = D2DDeviceContext->CreateBitmapFromWicBitmap(wicBitmap, Bitmap);
        wicBitmap->Release();
    }

    return status;
}

VOID PH_EMENU_DX::RebuildTextLayouts()
{
    // CalculateLayout recomputes the menu size from scratch, but the window, surface
    // and clip geometry were sized from the clamped geometry. Keep that geometry and
    // the hover state, and only replace the cached layouts and bitmaps.
    FLOAT width = Width;
    FLOAT height = Height;
    FLOAT totalHeight = ScrollViewTotalHeight;
    PPH_EMENU_ITEM hoverItem = HoverItem;
    LONG hoverIndex = HoverIndex;

    CalculateLayout();

    Width = width;
    Height = height;
    ScrollViewTotalHeight = totalHeight;
    HoverItem = hoverItem;
    HoverIndex = hoverIndex;
}

BOOLEAN PH_EMENU_DX::CanScrollUp()
{
    return ScrollViewTotalHeight > Height && ScrollOffset > 0;
}

BOOLEAN PH_EMENU_DX::CanScrollDown()
{
    return ScrollViewTotalHeight > Height && ScrollOffset < (ScrollViewTotalHeight - Height);
}

FLOAT PH_EMENU_DX::GetItemTop(
    _In_ ULONG Index
    )
{
    FLOAT itemTop = VerticalPadding;

    for (ULONG i = 0; std::cmp_less(i, Index) && std::cmp_less(i, Menu->Items->Count); i++)
    {
        PPH_EMENU_ITEM item = static_cast<PPH_EMENU_ITEM>(Menu->Items->Items[i]);
        itemTop += FlagOn(item->Flags, PH_EMENU_SEPARATOR) ? SeparatorHeight : ItemHeight;
    }

    return itemTop;
}

VOID PH_EMENU_DX::EnsureItemVisible(
    _In_ ULONG Index
    )
{
    PPH_EMENU_ITEM item;
    FLOAT maxOffset;
    FLOAT itemTop;
    FLOAT itemBottom;

    if (ScrollViewTotalHeight <= Height || std::cmp_greater_equal(Index, Menu->Items->Count))
        return;

    item = static_cast<PPH_EMENU_ITEM>(Menu->Items->Items[Index]);
    maxOffset = ScrollViewTotalHeight - Height;
    itemTop = GetItemTop(Index);
    itemBottom = itemTop + (FlagOn(item->Flags, PH_EMENU_SEPARATOR) ? SeparatorHeight : ItemHeight);

    // While visible, each scroll arrow covers one item height at its edge.
    if (itemTop < ScrollOffset + (CanScrollUp() ? ItemHeight : 0.0f))
        ScrollOffset = itemTop - ItemHeight;
    else if (itemBottom > ScrollOffset + Height - (CanScrollDown() ? ItemHeight : 0.0f))
        ScrollOffset = itemBottom + ItemHeight - Height;

    ScrollOffset = std::clamp(ScrollOffset, 0.0f, maxOffset);
}

VOID PH_EMENU_DX::SetHoverIndex(
    _In_ ULONG Index
    )
{
    // Any open submenu belongs to the previous hover item.
    CloseSubMenuChain();

    HoverIndex = static_cast<LONG>(Index);
    HoverItem = static_cast<PPH_EMENU_ITEM>(Menu->Items->Items[Index]);

    EnsureItemVisible(Index);
    CommitUpdate();
}

VOID PH_EMENU_DX::ActivateHoverItem()
{
    if (!HoverItem || FlagOn(HoverItem->Flags, PH_EMENU_DISABLED | PH_EMENU_SEPARATOR))
        return;

    if (HoverItem->Items && std::cmp_greater(HoverItem->Items->Count, 0))
    {
        OpenSubMenu();
    }
    else
    {
        SelectedItem = HoverItem;
        ExitLoop = TRUE;
        PostMessage(WindowHandle, WM_NULL, 0, 0);
    }
}

WCHAR PH_EMENU_DX::GetItemMnemonic(
    _In_ PPH_EMENU_ITEM Item
    )
{
    PWSTR text = Item->Text;

    if (!text)
        return UNICODE_NULL;

    // Same rules as CreateTextLayout: "&&" is a literal ampersand, the first
    // single '&' marks the mnemonic, and the shortcut text after '\b' is ignored.
    for (; *text != UNICODE_NULL && *text != L'\b'; text++)
    {
        if (*text == L'&')
        {
            text++;

            if (*text == L'&')
                continue;
            if (*text == UNICODE_NULL || *text == L'\b')
                break;

            return PhUpcaseUnicodeChar(*text);
        }
    }

    return UNICODE_NULL;
}

HRESULT PH_EMENU_DX::OpenSubMenu()
{
    if (SubMenuContext)
    {
        // The open submenu is still unwinding its message loop. Its window is shown
        // from the OpenSubMenu call further up the stack, which opens the next one
        // as soon as the old one returns.
        if (SubMenuContext->ExitLoop)
            SubMenuSwitchPending = TRUE;
        return S_OK;
    }

    while (HoverItem && HoverItem->Items && std::cmp_greater(HoverItem->Items->Count, 0) && !FlagOn(HoverItem->Flags, PH_EMENU_DISABLED))
    {
        PPH_EMENU_ITEM selected = nullptr;
        RECT windowRect;
        FLOAT itemY = PH_EMENU_DX_SUBMENU_Y_OFFSET;
        BOOLEAN shown;

        if (!PhGetWindowRect(WindowHandle, &windowRect))
        {
            SubMenuSwitching = FALSE;
            SubMenuSwitchPending = FALSE;
            return E_FAIL;
        }

        for (LONG i = 0; std::cmp_less(i, HoverIndex); i++)
        {
            PPH_EMENU_ITEM item = static_cast<PPH_EMENU_ITEM>(Menu->Items->Items[i]);
            itemY += FlagOn(item->Flags, PH_EMENU_SEPARATOR) ? SeparatorHeight : ItemHeight;
        }

        FLOAT surfacePadding = (FLOAT)GetSurfacePaddingPixels() / Scale;

        shown = PH_EMENU_DX::ShowInternal(HoverItem, WindowHandle, Flags, PH_ALIGN_LEFT | PH_ALIGN_TOP, windowRect.left + (LONG)((surfacePadding + Width) * Scale) - (LONG)(PH_EMENU_DX_SUBMENU_X_OFFSET * Scale), windowRect.top + (LONG)((surfacePadding + itemY - ScrollOffset) * Scale) - (LONG)(PH_EMENU_DX_SUBMENU_Y_OFFSET * Scale), !!IsDarkMode, this, &selected);
        SubMenuSwitching = FALSE;

        if (shown && selected)
        {
            SelectedItem = selected;
            ExitLoop = TRUE;
            PostMessage(WindowHandle, WM_NULL, 0, 0);
            break;
        }

        // The hover moved to another submenu item while the old submenu was open:
        // open its replacement now, without the show delay or an entrance fade, so
        // the shadow doesn't blink between the two.
        if (!SubMenuSwitchPending || ExitLoop)
            break;

        SubMenuSwitchPending = FALSE;
        SubMenuSwitching = TRUE;
    }

    SubMenuSwitching = FALSE;
    SubMenuSwitchPending = FALSE;
    return S_OK;
}

VOID PH_EMENU_DX::CloseSubMenuChain()
{
    PPH_EMENU_DX subContext = SubMenuContext;

    while (subContext)
    {
        subContext->ExitLoop = TRUE;
        PostMessage(subContext->WindowHandle, WM_NULL, 0, 0);
        subContext = subContext->SubMenuContext;
    }
}

VOID PH_EMENU_DX::DestroySubMenuChain()
{
    if (SubMenuContext)
    {
        PPH_EMENU_DX subContext = SubMenuContext;

        subContext->DestroySubMenuChain();
        subContext->ExitLoop = TRUE;

        if (IsWindow(subContext->WindowHandle))
            DestroyWindow(subContext->WindowHandle);

        SubMenuContext = nullptr;
    }
}

VOID PH_EMENU_DX::HandleMouseMove(
    _In_ HWND WindowHandle,
    _In_ POINT pt
    )
{
    FLOAT surfacePadding = (FLOAT)GetSurfacePaddingPixels() / Scale;

    PPH_EMENU_ITEM oldHover = HoverItem;
    FLOAT pt_x = (FLOAT)pt.x / Scale - surfacePadding;
    FLOAT pt_y = (FLOAT)pt.y / Scale - surfacePadding;
    FLOAT adjusted_pt_y = pt_y + ScrollOffset;
    BOOLEAN oldHoverUp = HoverUpArrow;
    BOOLEAN oldHoverDown = HoverDownArrow;

    HoverUpArrow = FALSE;
    HoverDownArrow = FALSE;

    // Arrow zones only exist while the arrow is drawn; otherwise the edge items
    // at the top and bottom of the scroll range would be unreachable.
    if (CanScrollUp() && pt_y < ItemHeight)
    {
        HoverUpArrow = TRUE;
        PhSetTimer(WindowHandle, PH_WINDOW_TIMER_SCROLL, PH_EMENU_DX_SCROLL_TIMER_INTERVAL, nullptr);
    }
    else if (CanScrollDown() && pt_y > Height - ItemHeight)
    {
        HoverDownArrow = TRUE;
        PhSetTimer(WindowHandle, PH_WINDOW_TIMER_SCROLL, PH_EMENU_DX_SCROLL_TIMER_INTERVAL, nullptr);
    }

    if (oldHoverUp != HoverUpArrow || oldHoverDown != HoverDownArrow)
        CommitUpdate();

    HoverItem = nullptr;
    HoverIndex = -1;

    if (!HoverUpArrow && !HoverDownArrow &&
        pt_x >= 0 && pt_x < Width &&
        pt_y >= VerticalPadding &&
        pt_y < Height - VerticalPadding)
    {
        FLOAT currentY = VerticalPadding;

        for (ULONG i = 0; std::cmp_less(i, Menu->Items->Count); i++)
        {
            PPH_EMENU_ITEM item = static_cast<PPH_EMENU_ITEM>(Menu->Items->Items[i]);
            FLOAT h = FlagOn(item->Flags, PH_EMENU_SEPARATOR) ? SeparatorHeight : ItemHeight;

            if (adjusted_pt_y >= currentY && adjusted_pt_y < currentY + h)
            {
                if (!FlagOn(item->Flags, PH_EMENU_SEPARATOR))
                {
                    HoverItem = item;
                    HoverIndex = (LONG)i;
                }

                break;
            }

            currentY += h;
        }
    }

    if (HoverItem != oldHover)
    {
        CommitUpdate();

        // Like native menus, an open submenu stays until the show delay elapses so it
        // isn't torn down while the cursor crosses other items on the way into it. The
        // timer then closes it and opens the replacement (if any) in one step.
        if (SubMenuContext && SubMenuContext->Menu == HoverItem && !SubMenuContext->ExitLoop)
        {
            PhKillTimer(WindowHandle, PH_WINDOW_TIMER_DEFAULT);
        }
        else if (SubMenuContext || (
            HoverItem &&
            HoverItem->Items &&
            std::cmp_greater(HoverItem->Items->Count, 0) &&
            !FlagOn(HoverItem->Flags, PH_EMENU_DISABLED)
            ))
        {
            PhSetTimer(WindowHandle, PH_WINDOW_TIMER_DEFAULT, std::max(MenuShowDelay, 1UL), nullptr);
        }
        else
        {
            PhKillTimer(WindowHandle, PH_WINDOW_TIMER_DEFAULT);
        }
    }

    {
        TRACKMOUSEEVENT trackEvent;

        trackEvent.cbSize = sizeof(TRACKMOUSEEVENT);
        trackEvent.dwFlags = TME_LEAVE;
        trackEvent.hwndTrack = WindowHandle;
        trackEvent.dwHoverTime = 0;

        TrackMouseEvent(&trackEvent);
    }
}

LRESULT PH_EMENU_DX::OnNcCreate(
    _In_ HWND WindowHandle,
    _In_ CREATESTRUCT* CreateStruct
    )
{
    PPH_EMENU_DX context = static_cast<PPH_EMENU_DX>(CreateStruct->lpCreateParams);
    context->WindowHandle = WindowHandle;
    PhSetWindowContextEx(WindowHandle, context);
    return TRUE;
}

LRESULT PH_EMENU_DX::OnCreate(
    _In_ HWND WindowHandle
    )
{
    // With the DirectComposition shadow the window is a transparent, padded surface
    // and the menu draws its own rounded corners; DWM must not round the window.
    if (std::cmp_greater_equal(WindowsVersion, WINDOWS_11) && !ShadowEnabled)
    {
#ifndef PH_EMENU_DX_ENABLE_DROPSHADOW
        ULONG ulongAttribute = PH_EMENU_DX_CORNER_STYLE;
        WINDOWCOMPOSITIONATTRIBUTEDATA data;

        data.Attribute = WCA_CORNER_STYLE;
        data.Data = &ulongAttribute;
        data.Length = sizeof(ULONG);
        PhSetWindowCompositionAttribute(WindowHandle, &data);

        //if (PhIsThemeTransparencyEnabled() && (IsAcrylic || IsMica))
        //{
        //    DWM_SYSTEMBACKDROPATTR_TYPE backdrop = DWMSBTATTR_TRANSIENTWINDOW;
        //    status = PhSetWindowThemeAttribute(WindowHandle, DWM_SYSTEMBACKDROP_ATTRTYPE, &backdrop, sizeof(backdrop));
        //}
#endif
    }

    // A submenu replacing a sibling appears at once; a DWM fade-in on every switch
    // makes the shadow blink.
    if (ParentContext && ParentContext->SubMenuSwitching)
    {
        BOOL transitionsDisabled = TRUE;

        PhSetWindowThemeAttribute(WindowHandle, DWMWA_TRANSITIONS_FORCEDISABLED, &transitionsDisabled, sizeof(BOOL));
    }

    return 0;
}

VOID PH_EMENU_DX::OnDestroy(
    _In_ HWND WindowHandle
    )
{
    ExitLoop = TRUE;
    DestroySubMenuChain();
    PhKillTimer(WindowHandle, PH_WINDOW_TIMER_DEFAULT);
    PhKillTimer(WindowHandle, PH_WINDOW_TIMER_SCROLL);
}

VOID PH_EMENU_DX::OnPaint(
    _In_ HWND WindowHandle
    )
{
    PAINTSTRUCT ps;

    if (BeginPaint(WindowHandle, &ps))
    {
        CommitUpdate();
        EndPaint(WindowHandle, &ps);
    }
}

BOOLEAN PH_EMENU_DX::OnSetCursor(
    _In_ HWND WindowHandle,
    _In_ HWND CursorWindowHandle,
    _In_ ULONG HitTest,
    _In_ ULONG WindowMessage
    )
{
    if (HitTest == HTCLIENT)
    {
        PhSetCursor(PhLoadCursor(nullptr, IDC_ARROW));
        return TRUE;
    }

    return FALSE;
}

VOID PH_EMENU_DX::OnMouseWheel(
    _In_ HWND WindowHandle,
    _In_ SHORT WheelDelta
    )
{
    if (ScrollViewTotalHeight > Height)
    {
        FLOAT steps = static_cast<FLOAT>(WheelDelta) / static_cast<FLOAT>(WHEEL_DELTA);
        FLOAT scrollAmount = steps * ItemHeight * PH_EMENU_DX_SCROLL_STEP;
        POINT pt;

        ScrollOffset -= scrollAmount;

        ScrollOffset = std::clamp(ScrollOffset, 0.0f, ScrollViewTotalHeight - Height);

        CommitUpdate();

        if (PhGetClientPos(WindowHandle, &pt))
        {
            HandleMouseMove(WindowHandle, pt);
        }
    }
}

VOID PH_EMENU_DX::OnMouseMove(
    _In_ HWND WindowHandle,
    _In_ LONG CursorX,
    _In_ LONG CursorY
    )
{
    POINT pt = { CursorX, CursorY };
    HandleMouseMove(WindowHandle, pt);
}

VOID PH_EMENU_DX::OnMouseLeave(
    _In_ HWND WindowHandle
    )
{
    if (SubMenuContext)
    {
        POINT pt;

        if (GetCursorPos(&pt))
        {
            HWND childWnd = SubMenuContext->WindowHandle;
            RECT childRect;

            if (GetWindowRect(childWnd, &childRect) && PhPtInRect(&childRect, &pt))
            {
                ULONG index;

                // The cursor reached the open submenu: cancel a pending switch to an
                // item crossed on the way and highlight the item that owns it again.
                if (!SubMenuContext->ExitLoop)
                {
                    PhKillTimer(WindowHandle, PH_WINDOW_TIMER_DEFAULT);

                    if (HoverItem != SubMenuContext->Menu &&
                        (index = PhFindItemList(Menu->Items, SubMenuContext->Menu)) != ULONG_MAX)
                    {
                        HoverItem = SubMenuContext->Menu;
                        HoverIndex = static_cast<LONG>(index);
                        CommitUpdate();
                    }
                }

                return;
            }
        }

        CloseSubMenuChain();
    }

    HoverItem = nullptr;
    HoverIndex = -1;
    CommitUpdate();
}

VOID PH_EMENU_DX::OnTimer(
    _In_ HWND WindowHandle,
    _In_ ULONG TimerId
    )
{
    if (TimerId == PH_WINDOW_TIMER_DEFAULT)
    {
        PhKillTimer(WindowHandle, PH_WINDOW_TIMER_DEFAULT);

        // The show delay elapsed on a different item than the one that owns the open
        // submenu. OpenSubMenu then marks the switch and the replacement is opened as
        // soon as the old submenu returns.
        if (SubMenuContext && SubMenuContext->Menu != HoverItem)
            CloseSubMenuChain();

        OpenSubMenu();
    }
    else if (TimerId == PH_WINDOW_TIMER_SCROLL)
    {
        const FLOAT surfacePadding = static_cast<FLOAT>(GetSurfacePaddingPixels()) / Scale;
        POINT pt;

        if (!PhGetClientPos(WindowHandle, &pt))
            return;

        const FLOAT pt_y = static_cast<FLOAT>(pt.y) / Scale - surfacePadding;
        const FLOAT scrollAmount = ItemHeight / PH_EMENU_DX_SCROLL_SPEED_DIVISOR;

        if (pt_y < ItemHeight && ScrollOffset > 0)
        {
            ScrollOffset -= scrollAmount;
            ScrollOffset = std::max(ScrollOffset, 0.0f);
            CommitUpdate();
            HandleMouseMove(WindowHandle, pt);
        }
        else if (pt_y > Height - ItemHeight && ScrollOffset < (ScrollViewTotalHeight - Height))
        {
            ScrollOffset += scrollAmount;
            ScrollOffset = std::min(ScrollOffset, ScrollViewTotalHeight - Height);
            CommitUpdate();
            HandleMouseMove(WindowHandle, pt);
        }
        else
        {
            PhKillTimer(WindowHandle, PH_WINDOW_TIMER_SCROLL);
        }
    }
}

VOID PH_EMENU_DX::OnLButtonUp(
    _In_ HWND WindowHandle,
    _In_ LONG CursorX,
    _In_ LONG CursorY
    )
{
    FLOAT surfacePadding = static_cast<FLOAT>(GetSurfacePaddingPixels()) / Scale;
    FLOAT pt_x = static_cast<FLOAT>(CursorX) / Scale - surfacePadding;
    FLOAT pt_y = static_cast<FLOAT>(CursorY) / Scale - surfacePadding;
    FLOAT adjusted_pt_y = pt_y + ScrollOffset;

    // Ignore clicks on a visible scroll arrow.
    if ((CanScrollUp() && pt_y < ItemHeight) ||
        (CanScrollDown() && pt_y > Height - ItemHeight))
    {
        return;
    }

    if (pt_x >= 0 && pt_x < Width && pt_y >= VerticalPadding && pt_y < Height - VerticalPadding)
    {
        FLOAT currentY = VerticalPadding;

        for (ULONG i = 0; std::cmp_less(i, Menu->Items->Count); i++)
        {
            PPH_EMENU_ITEM item = static_cast<PPH_EMENU_ITEM>(Menu->Items->Items[i]);
            FLOAT h = FlagOn(item->Flags, PH_EMENU_SEPARATOR) ? SeparatorHeight : ItemHeight;

            if (adjusted_pt_y >= currentY && adjusted_pt_y < currentY + h)
            {
                if (!FlagOn(item->Flags, PH_EMENU_DISABLED | PH_EMENU_SEPARATOR))
                {
                    if (item->Items && std::cmp_greater(item->Items->Count, 0))
                    {
                        if (!SubMenuContext)
                        {
                            PhKillTimer(WindowHandle, PH_WINDOW_TIMER_DEFAULT);
                            OpenSubMenu();
                        }
                    }
                    else
                    {
                        SelectedItem = item;
                        ExitLoop = TRUE;
                        PostMessage(WindowHandle, WM_NULL, 0, 0);
                    }
                }
                break;
            }

            currentY += h;
        }
    }
}

VOID PH_EMENU_DX::OnKeyDown(
    _In_ HWND WindowHandle,
    _In_ ULONG VirtualKey
    )
{
    PhKillTimer(WindowHandle, PH_WINDOW_TIMER_DEFAULT);

    if (std::cmp_equal(Menu->Items->Count, 0))
        return;

    switch (VirtualKey)
    {
    case VK_ESCAPE:
        {
            ExitLoop = TRUE;
            PostMessage(WindowHandle, WM_NULL, 0, 0);
        }
        break;
    case VK_RIGHT:
        {
            if (
                HoverItem &&
                HoverItem->Items &&
                std::cmp_greater(HoverItem->Items->Count, 0) &&
                !FlagOn(HoverItem->Flags, PH_EMENU_DISABLED)
                )
            {
                OpenSubMenu();
            }
        }
        break;
    case VK_LEFT:
        {
            if (IsSubMenu)
            {
                ExitLoop = TRUE;
                PostMessage(WindowHandle, WM_NULL, 0, 0);
            }
        }
        break;
    case VK_UP:
        {
            LONG next = HoverIndex - 1;

            if (std::cmp_less(next, 0L))
                next = static_cast<LONG>(Menu->Items->Count) - 1;

            LONG start = next;

            while (FlagOn((static_cast<PPH_EMENU_ITEM>(Menu->Items->Items[next]))->Flags, PH_EMENU_SEPARATOR | PH_EMENU_DISABLED))
            {
                next--;
                if (std::cmp_less(next, 0L))
                    next = static_cast<LONG>(Menu->Items->Count) - 1;
                if (std::cmp_equal(next, start))
                    break;
            }

            SetHoverIndex(static_cast<ULONG>(next));
        }
        break;
    case VK_DOWN:
        {
            LONG next = HoverIndex + 1;

            if (std::cmp_greater_equal(next, Menu->Items->Count))
                next = 0;

            LONG start = next;

            while (FlagOn((static_cast<PPH_EMENU_ITEM>(Menu->Items->Items[next]))->Flags, PH_EMENU_SEPARATOR | PH_EMENU_DISABLED))
            {
                next++;
                if (std::cmp_greater_equal(next, Menu->Items->Count))
                    next = 0;
                if (std::cmp_equal(next, start))
                    break;
            }

            SetHoverIndex(static_cast<ULONG>(next));
        }
        break;
    case VK_RETURN:
        {
            ActivateHoverItem();
        }
        break;
    }
}

VOID PH_EMENU_DX::OnSysKeyDown(
    _In_ HWND WindowHandle,
    _In_ ULONG VirtualKey,
    _In_ BOOLEAN Repeat
    )
{
    if (VirtualKey == VK_MENU)
    {
        // Ignore auto-repeat so holding Alt doesn't flicker the cues.
        if (Repeat)
            return;

        ShowKeyboardCues = !ShowKeyboardCues;
        RebuildTextLayouts();
        CommitUpdate();
    }
    else if (VirtualKey == VK_F10)
    {
        ExitLoop = TRUE;
        PostMessage(WindowHandle, WM_NULL, 0, 0);
    }
}

BOOLEAN PH_EMENU_DX::OnChar(
    _In_ HWND WindowHandle,
    _In_ WCHAR Character
    )
{
    ULONG count = Menu->Items->Count;
    ULONG matches = 0;
    LONG first = -1;
    LONG next = -1;
    WCHAR key;

    if (std::cmp_equal(count, 0) || Character < L' ')
        return FALSE;

    key = PhUpcaseUnicodeChar(Character);

    // Mnemonic selection: a unique match activates the item; several matches
    // cycle the hover through them, starting after the current item.
    for (ULONG i = 0; std::cmp_less(i, count); i++)
    {
        PPH_EMENU_ITEM item = static_cast<PPH_EMENU_ITEM>(Menu->Items->Items[i]);

        if (FlagOn(item->Flags, PH_EMENU_SEPARATOR | PH_EMENU_DISABLED))
            continue;
        if (GetItemMnemonic(item) != key)
            continue;

        matches++;

        if (std::cmp_equal(first, -1))
            first = static_cast<LONG>(i);
        if (std::cmp_equal(next, -1) && std::cmp_greater(i, HoverIndex))
            next = static_cast<LONG>(i);
    }

    if (std::cmp_equal(matches, 0))
        return FALSE;

    if (std::cmp_equal(next, -1))
        next = first;

    PhKillTimer(WindowHandle, PH_WINDOW_TIMER_DEFAULT);
    SetHoverIndex(static_cast<ULONG>(next));

    if (std::cmp_equal(matches, 1))
        ActivateHoverItem();

    return TRUE;
}

VOID PH_EMENU_DX::OnActivateApp(
    _In_ HWND WindowHandle,
    _In_ BOOLEAN Active
    )
{
    if (!Active)
    {
        ExitLoop = TRUE;
        PostMessage(WindowHandle, WM_NULL, 0, 0);
    }
}

LRESULT PH_EMENU_DX::OnActivate(
    _In_ HWND WindowHandle,
    _In_ ULONG Type,
    _In_ HWND OtherWindow
    )
{
    if (LOWORD(Type) == WA_INACTIVE)
    {
        HWND otherWnd = OtherWindow;
        HWND popup = otherWnd ? GetLastActivePopup(otherWnd) : nullptr;

        if (PH_EMENU_DX::IsWindowInMenuChain(otherWnd) || PH_EMENU_DX::IsWindowInMenuChain(popup))
            return 0;

        if ((SubMenuContext && otherWnd == SubMenuContext->WindowHandle) ||
            (ParentContext && otherWnd == ParentContext->WindowHandle))
            return 0;

        ExitLoop = TRUE;
        PostMessage(WindowHandle, WM_NULL, 0, 0);
    }

    return 0;
}

VOID PH_EMENU_DX::OnCancelMode(
    _In_ HWND WindowHandle
    )
{
    ExitLoop = TRUE;

    if (SubMenuContext)
    {
        SubMenuContext->ExitLoop = TRUE;
        PostMessage(SubMenuContext->WindowHandle, WM_NULL, 0, 0);
    }

    PostMessage(WindowHandle, WM_NULL, 0, 0);
}

VOID PH_EMENU_DX::OnUpdateUiState(
    _In_ HWND WindowHandle,
    _In_ USHORT Action,
    _In_ USHORT Flags
    )
{
    if (FlagOn(Flags, UISF_HIDEACCEL))
    {
        BOOL newShowCues = (Action == UIS_CLEAR);

        if (ShowKeyboardCues != newShowCues)
        {
            ShowKeyboardCues = newShowCues;
            RebuildTextLayouts();
            CommitUpdate();
        }
    }
}

LRESULT CALLBACK PH_EMENU_DX::MouseHookCallback(
    _In_ INT nCode,
    _In_ WPARAM wParam,
    _In_ LPARAM lParam
    )
{
    if (nCode == HC_ACTION && (wParam == WM_LBUTTONDOWN || wParam == WM_RBUTTONDOWN || wParam == WM_MBUTTONDOWN || wParam == WM_NCLBUTTONDOWN))
    {
        PMSLLHOOKSTRUCT hs = reinterpret_cast<PMSLLHOOKSTRUCT>(lParam);

        // Compare against the menu bodies, not the windows: the transparent shadow
        // padding around a menu is part of its window but is not a menu click.
        if (PH_EMENU_DX::IsPointInMenuBody(hs->pt))
            return CallNextHookEx(nullptr, nCode, wParam, lParam);

        // Low-level hooks run on the installing thread. Cancel only that thread's
        // menus; a cross-thread SendMessage here could stall system-wide mouse input.
        PH_EMENU_DX::CancelThreadMenus();
    }

    return CallNextHookEx(nullptr, nCode, wParam, lParam);
}

/**
 * Cancels every menu (including submenus) shown by the calling thread, innermost first.
 */
VOID PH_EMENU_DX::CancelThreadMenus()
{
    HWND* windows = nullptr;
    ULONG count = 0;
    ULONG currentThreadId = HandleToUlong(NtCurrentThreadId());

    PhAcquireQueuedLockShared(&MenuListLock);
    if (MenuList && std::cmp_greater(MenuList->Count, 0))
    {
        windows = static_cast<HWND*>(PhAllocate(MenuList->Count * sizeof(HWND)));

        for (ULONG i = 0; std::cmp_less(i, MenuList->Count); i++)
        {
            PPH_EMENU_DX menuContext = static_cast<PPH_EMENU_DX>(MenuList->Items[i]);

            if (menuContext->ThreadId == currentThreadId && menuContext->WindowHandle)
                windows[count++] = menuContext->WindowHandle;
        }
    }
    PhReleaseQueuedLockShared(&MenuListLock);

    if (windows)
    {
        for (ULONG i = count; std::cmp_greater(i, 0); i--)
            SendMessage(windows[i - 1], WM_CANCELMODE, 0, 0);
        PhFree(windows);
    }
}

BOOLEAN PH_EMENU_DX::IsMenuSupported(
    _In_ PPH_EMENU Menu
    )
{
    const ULONG unsupportedFlags = PH_EMENU_MENUBARBREAK | PH_EMENU_MENUBREAK | PH_EMENU_RIGHTORDER | PH_EMENU_MAINMENU;

    if (FlagOn(Menu->Flags, PH_EMENU_MAINMENU))
        return FALSE;
    if (!Menu->Items || std::cmp_equal(Menu->Items->Count, 0))
        return FALSE;

    for (ULONG i = 0; std::cmp_less(i, Menu->Items->Count); i++)
    {
        PPH_EMENU_ITEM item = static_cast<PPH_EMENU_ITEM>(Menu->Items->Items[i]);

        if (FlagOn(item->Flags, unsupportedFlags))
            return FALSE;

        // Empty submenus are drawn as plain items, so only recurse into populated ones.
        if (item->Items && std::cmp_not_equal(item->Items->Count, 0) && !IsMenuSupported(item))
            return FALSE;
    }

    return TRUE;
}

VOID PH_EMENU_DX::UnregisterWindowClass()
{
    PH_EMENU_DX::ResetGlobalResources();

    PhAcquireQueuedLockExclusive(&MenuListLock);
    if (MenuList)
    {
        PhDereferenceObject(MenuList);
        MenuList = nullptr;
    }
    PhReleaseQueuedLockExclusive(&MenuListLock);

    //UnregisterClass(PH_EMENU_DX_CLASSNAME, reinterpret_cast<HINSTANCE>(PhInstanceHandle));
}

HRESULT PH_EMENU_DX::CompositionCreateDevice(
    _In_opt_ IUnknown* RenderingDevice,
    _In_ REFIID Iid,
    _COM_Outptr_ void** DCompositionDevice
    )
{
    static decltype(&DCompositionCreateDevice2) createDevice = nullptr;
    static decltype(&DCompositionCreateDevice2) createDevice3 = nullptr;
    static PVOID compositionLibrary = nullptr;
    HRESULT status;

    if (!createDevice)
    {
        if (!compositionLibrary)
            compositionLibrary = PhLoadLibrary(L"dcomp.dll");
        if (!compositionLibrary)
            return HRESULT_FROM_WIN32(ERROR_MOD_NOT_FOUND);

        // DCompositionCreateDevice3 has the same signature as ...Device2.
        createDevice3 = reinterpret_cast<decltype(&DCompositionCreateDevice2)>(
            PhGetProcedureAddress(compositionLibrary, "DCompositionCreateDevice3", 0)
            );
        createDevice = reinterpret_cast<decltype(&DCompositionCreateDevice2)>(
            PhGetProcedureAddress(compositionLibrary, "DCompositionCreateDevice2", 0)
            );
        if (!createDevice)
            return HRESULT_FROM_WIN32(ERROR_PROC_NOT_FOUND);
    }

    // A device from ...Device2 cannot be queried for IDCompositionDevice3, which
    // owns CreateShadowEffect. Prefer ...Device3 and fall back to ...Device2.
    if (createDevice3)
    {
        status = createDevice3(RenderingDevice, Iid, DCompositionDevice);

        if (HR_SUCCESS(status))
            return status;
    }

    return createDevice(RenderingDevice, Iid, DCompositionDevice);
}

PVOID PH_EMENU_DX::GetDynamicProcedure(
    _Inout_ PVOID* CachedProcedure,
    _In_ PCWSTR LibraryName,
    _In_ PCSTR ProcedureName
    )
{
    PVOID procedure = ReadPointerAcquire(CachedProcedure);

    if (!procedure)
    {
        PVOID baseAddress;

        if (baseAddress = PhLoadLibrary(LibraryName))
        {
            if (procedure = PhGetProcedureAddress(baseAddress, ProcedureName, 0))
            {
                WritePointerRelease(CachedProcedure, procedure);
            }
        }
    }

    return procedure;
}

HRESULT PH_EMENU_DX::DynamicD3D11CreateDevice(
    _In_opt_ IDXGIAdapter* Adapter,
    _In_ D3D_DRIVER_TYPE DriverType,
    _In_ UINT Flags,
    _Out_ ID3D11Device** Device,
    _Out_opt_ D3D_FEATURE_LEVEL* FeatureLevel,
    _Out_opt_ ID3D11DeviceContext** ImmediateContext
    )
{
    using D3D11_CREATE_DEVICE_FN = HRESULT (WINAPI*)(IDXGIAdapter*, D3D_DRIVER_TYPE, HMODULE, UINT, const D3D_FEATURE_LEVEL*, UINT, UINT, ID3D11Device**, D3D_FEATURE_LEVEL*, ID3D11DeviceContext**);
    static PVOID procedure = nullptr;
    D3D11_CREATE_DEVICE_FN createDevice;

    *Device = nullptr;
    if (ImmediateContext)
        *ImmediateContext = nullptr;

    createDevice = reinterpret_cast<D3D11_CREATE_DEVICE_FN>(GetDynamicProcedure(&procedure, L"d3d11.dll", "D3D11CreateDevice"));

    if (!createDevice)
        return HRESULT_FROM_WIN32(ERROR_PROC_NOT_FOUND);

    return createDevice(Adapter, DriverType, nullptr, Flags, nullptr, 0, D3D11_SDK_VERSION, Device, FeatureLevel, ImmediateContext);
}

HRESULT PH_EMENU_DX::DynamicD3D10CreateDevice1(
    _In_ D3D10_DRIVER_TYPE DriverType,
    _In_ UINT Flags,
    _In_ D3D10_FEATURE_LEVEL1 HardwareLevel,
    _Out_ ID3D10Device1** Device
    )
{
    using D3D10_CREATE_DEVICE1_FN = HRESULT (WINAPI*)(IDXGIAdapter*, D3D10_DRIVER_TYPE, HMODULE, UINT, D3D10_FEATURE_LEVEL1, UINT, ID3D10Device1**);
    static PVOID procedure = nullptr;
    D3D10_CREATE_DEVICE1_FN createDevice;

    *Device = nullptr;

    createDevice = reinterpret_cast<D3D10_CREATE_DEVICE1_FN>(GetDynamicProcedure(&procedure, L"d3d10_1.dll", "D3D10CreateDevice1"));

    if (!createDevice)
        return HRESULT_FROM_WIN32(ERROR_PROC_NOT_FOUND);

    return createDevice(nullptr, DriverType, nullptr, Flags, HardwareLevel, D3D10_1_SDK_VERSION, Device);
}

HRESULT PH_EMENU_DX::DynamicD2D1CreateFactory(
    _In_ D2D1_FACTORY_TYPE FactoryType,
    _In_ REFIID Iid,
    _In_opt_ const D2D1_FACTORY_OPTIONS* FactoryOptions,
    _Out_ void** Factory
    )
{
    // D2D1CreateFactory has C++ overloads in d2d1.h, so spell out the export's type.
    using D2D1_CREATE_FACTORY_FN = HRESULT (WINAPI*)(D2D1_FACTORY_TYPE, REFIID, const D2D1_FACTORY_OPTIONS*, void**);
    static PVOID procedure = nullptr;
    D2D1_CREATE_FACTORY_FN createFactory;

    *Factory = nullptr;

    createFactory = reinterpret_cast<D2D1_CREATE_FACTORY_FN>(GetDynamicProcedure(&procedure, L"d2d1.dll", "D2D1CreateFactory"));

    if (!createFactory)
        return HRESULT_FROM_WIN32(ERROR_PROC_NOT_FOUND);

    return createFactory(FactoryType, Iid, FactoryOptions, Factory);
}

HRESULT PH_EMENU_DX::DynamicDWriteCreateFactory(
    _In_ DWRITE_FACTORY_TYPE FactoryType,
    _In_ REFIID Iid,
    _Out_ IUnknown** Factory
    )
{
    using DWRITE_CREATE_FACTORY_FN = HRESULT (WINAPI*)(DWRITE_FACTORY_TYPE, REFIID, IUnknown**);
    static PVOID procedure = nullptr;
    DWRITE_CREATE_FACTORY_FN createFactory;

    *Factory = nullptr;

    createFactory = reinterpret_cast<DWRITE_CREATE_FACTORY_FN>(GetDynamicProcedure(&procedure, L"dwrite.dll", "DWriteCreateFactory"));

    if (!createFactory)
        return HRESULT_FROM_WIN32(ERROR_PROC_NOT_FOUND);

    return createFactory(FactoryType, Iid, Factory);
}

HRESULT PH_EMENU_DX::DynamicDwmExtendFrameIntoClientArea(
    _In_ HWND WindowHandle,
    _In_ const MARGINS* Margins
    )
{
    using DWM_EXTEND_FRAME_FN = HRESULT (WINAPI*)(HWND, const MARGINS*);
    static PVOID procedure = nullptr;
    DWM_EXTEND_FRAME_FN extendFrame;

    extendFrame = reinterpret_cast<DWM_EXTEND_FRAME_FN>(GetDynamicProcedure(&procedure, L"dwmapi.dll", "DwmExtendFrameIntoClientArea"));

    if (!extendFrame)
        return HRESULT_FROM_WIN32(ERROR_PROC_NOT_FOUND);

    return extendFrame(WindowHandle, Margins);
}

VOID PH_EMENU_DX::ResetGlobalResources()
{
    PhAcquireQueuedLockExclusive(&SharedLock);
    PH_EMENU_DX::ReleaseSharedResources();
    PhReleaseQueuedLockExclusive(&SharedLock);
}

// Caller must hold SharedLock exclusively. Menus keep their own references,
// so objects in use by open menus stay alive until those menus release them.
VOID PH_EMENU_DX::ReleaseSharedResources()
{
#ifdef PH_EMENU_DX_ENABLE_D3D12
    if (SharedD3D11On12Device)
    {
        SharedD3D11On12Device->Release();
        SharedD3D11On12Device = nullptr;
    }
    if (SharedD3D11Device12)
    {
        SharedD3D11Device12->Release();
        SharedD3D11Device12 = nullptr;
    }
    if (SharedD3D12CommandQueue)
    {
        SharedD3D12CommandQueue->Release();
        SharedD3D12CommandQueue = nullptr;
    }
    if (SharedD3D12Device)
    {
        SharedD3D12Device->Release();
        SharedD3D12Device = nullptr;
    }
#endif
#ifdef PH_EMENU_DX_ENABLE_D3D11
    if (SharedD3D11Device)
    {
        SharedD3D11Device->Release();
        SharedD3D11Device = nullptr;
    }
#endif
    if (SharedD3DDevice)
    {
        SharedD3DDevice->Release();
        SharedD3DDevice = nullptr;
    }
    if (SharedD2DDevice)
    {
        SharedD2DDevice->Release();
        SharedD2DDevice = nullptr;
    }
    if (SharedD2DFactory)
    {
        SharedD2DFactory->Release();
        SharedD2DFactory = nullptr;
    }
#ifdef PH_EMENU_DX_ENABLE_WINRT_COMPOSITION
    if (SharedCompositionGraphicsDevice)
    {
        SharedCompositionGraphicsDevice->Release();
        SharedCompositionGraphicsDevice = nullptr;
    }
    if (SharedCompositor)
    {
        SharedCompositor->Release();
        SharedCompositor = nullptr;
    }
#else
    if (SharedCompositionDevice3)
    {
        SharedCompositionDevice3->Release();
        SharedCompositionDevice3 = nullptr;
    }
    if (SharedDesktopDevice)
    {
        SharedDesktopDevice->Release();
        SharedDesktopDevice = nullptr;
    }
    if (SharedDCompDevice)
    {
        SharedDCompDevice->Release();
        SharedDCompDevice = nullptr;
    }
#endif
    if (SharedWicFactory)
    {
        SharedWicFactory->Release();
        SharedWicFactory = nullptr;
    }
    if (SharedDWriteFactory)
    {
        SharedDWriteFactory->Release();
        SharedDWriteFactory = nullptr;
    }

    SharedGeneration++;
}

HRESULT PH_EMENU_DX::CreateSharedD3DDevice()
{
    HRESULT status = E_FAIL;

    // Keep whichever device the shared D2D device was built on. Retrying a
    // previously failed backend here would leave D3D and D2D on different devices.
#ifdef PH_EMENU_DX_ENABLE_D3D12
    if (SharedD3D11Device12)
        return S_OK;
#endif
#ifdef PH_EMENU_DX_ENABLE_D3D11
    if (SharedD3D11Device)
        return S_OK;
#endif
    if (SharedD3DDevice)
        return S_OK;

#if defined(PH_EMENU_DX_ENABLE_D3D12)
    if (HR_FAILED(status))
    {
        if (!SharedD3D12Device)
        {
            status = D3D12CreateDevice(nullptr, D3D_FEATURE_LEVEL_11_0, IID_PPV_ARGS(&SharedD3D12Device));

            if (HR_SUCCESS(status))
            {
                D3D12_COMMAND_QUEUE_DESC desc = { D3D12_COMMAND_LIST_TYPE_DIRECT };
                status = SharedD3D12Device->CreateCommandQueue(&desc, IID_PPV_ARGS(&SharedD3D12CommandQueue));
            }

            if (HR_SUCCESS(status))
            {
                IUnknown* commandQueues[] = { SharedD3D12CommandQueue };
                status = D3D11On12CreateDevice(
                    SharedD3D12Device,
                    D3D11_CREATE_DEVICE_BGRA_SUPPORT,
                    nullptr,
                    0,
                    commandQueues,
                    std::size(commandQueues),
                    0,
                    &SharedD3D11Device12,
                    nullptr,
                    nullptr
                    );
            }

            if (HR_SUCCESS(status))
            {
                status = SharedD3D11Device12->QueryInterface(IID_PPV_ARGS(&SharedD3D11On12Device));
            }
        }
        else
        {
            status = S_OK;
        }
    }
#endif
#if defined(PH_EMENU_DX_ENABLE_D3D11)
    if (HR_FAILED(status))
    {
        if (!SharedD3D11Device)
        {
            D3D_FEATURE_LEVEL featureLevel;
            ID3D11DeviceContext* d3dContext = nullptr;

            status = DynamicD3D11CreateDevice(
                nullptr,
                D3D_DRIVER_TYPE_HARDWARE,
                D3D11_CREATE_DEVICE_BGRA_SUPPORT,
                &SharedD3D11Device,
                &featureLevel,
                &d3dContext
                );

            if (d3dContext)
                d3dContext->Release();
        }
        else
        {
            status = S_OK;
        }
    }
#endif
    if (HR_FAILED(status))
    {
        if (!SharedD3DDevice)
        {
            status = DynamicD3D10CreateDevice1(
                D3D10_DRIVER_TYPE_HARDWARE,
                D3D10_CREATE_DEVICE_BGRA_SUPPORT,
                D3D10_FEATURE_LEVEL_10_1,
                &SharedD3DDevice
                );
        }
        else
        {
            status = S_OK;
        }
    }

    return status;
}

HRESULT PH_EMENU_DX::CreateSharedD2DDevice()
{
    HRESULT status = S_OK;
    IDXGIDevice* dxgiDevice = nullptr;

    if (HR_SUCCESS(status) && !SharedD2DFactory)
    {
        D2D1_FACTORY_OPTIONS options = { D2D1_DEBUG_LEVEL_NONE };

        status = DynamicD2D1CreateFactory(
            D2D1_FACTORY_TYPE_MULTI_THREADED, // Shared across GUI threads
            __uuidof(ID2D1Factory1),
            &options,
            (void**)&SharedD2DFactory
            );
    }

    if (HR_SUCCESS(status))
    {
        if (!dxgiDevice)
        {
#ifdef PH_EMENU_DX_ENABLE_D3D12
            if (SharedD3D11Device12 && !dxgiDevice)
                status = SharedD3D11Device12->QueryInterface(IID_PPV_ARGS(&dxgiDevice));
#endif
#ifdef PH_EMENU_DX_ENABLE_D3D11
            if (SharedD3D11Device && !dxgiDevice)
                status = SharedD3D11Device->QueryInterface(IID_PPV_ARGS(&dxgiDevice));
#endif
            if (SharedD3DDevice && !dxgiDevice)
                status = SharedD3DDevice->QueryInterface(IID_PPV_ARGS(&dxgiDevice));
        }

        if (!dxgiDevice)
            status = E_FAIL;
    }

    if (HR_SUCCESS(status) && dxgiDevice && SharedD2DFactory && !SharedD2DDevice)
    {
        status = SharedD2DFactory->CreateDevice(dxgiDevice, &SharedD2DDevice);
    }

    if (dxgiDevice)
        dxgiDevice->Release();

    return status;
}

HRESULT PH_EMENU_DX::CreateSharedDCompDevice()
{
    HRESULT status = S_OK;

#ifdef PH_EMENU_DX_ENABLE_WINRT_COMPOSITION
    if (HR_SUCCESS(status) && !SharedCompositor)
    {
        status = PhActivateInstance(
            L"dcomp.dll",
            RuntimeClass_Windows_UI_Composition_Compositor,
            IID_PPV_ARGS(&SharedCompositor)
            );
    }

    if (HR_SUCCESS(status) && !SharedCompositionGraphicsDevice && SharedCompositor)
    {
        ComPtr<ICompositorInterop> compositorInterop;
        ComPtr<IDXGIDevice> dxgiDeviceLocal;

        status = SharedCompositor->QueryInterface(IID_PPV_ARGS(&compositorInterop));

        if (HR_SUCCESS(status))
        {
#ifdef PH_EMENU_DX_ENABLE_D3D12
            if (SharedD3D11Device12 && !dxgiDeviceLocal)
                status = SharedD3D11Device12->QueryInterface(IID_PPV_ARGS(&dxgiDeviceLocal));
#endif
#ifdef PH_EMENU_DX_ENABLE_D3D11
            if (SharedD3D11Device && !dxgiDeviceLocal)
                status = SharedD3D11Device->QueryInterface(IID_PPV_ARGS(&dxgiDeviceLocal));
#endif
            if (SharedD3DDevice && !dxgiDeviceLocal)
                status = SharedD3DDevice->QueryInterface(IID_PPV_ARGS(&dxgiDeviceLocal));

            if (!dxgiDeviceLocal)
                status = E_FAIL;
        }

        if (HR_SUCCESS(status))
        {
            status = compositorInterop->CreateGraphicsDevice(dxgiDeviceLocal.Get(), &SharedCompositionGraphicsDevice);
        }
    }
#endif

#ifndef PH_EMENU_DX_ENABLE_WINRT_COMPOSITION
    // A D2D rendering device lets DirectComposition batch surface drawing.
    if (HR_SUCCESS(status) && !SharedDesktopDevice)
        status = PH_EMENU_DX::CompositionCreateDevice(SharedD2DDevice, IID_PPV_ARGS(&SharedDesktopDevice));

    if (HR_SUCCESS(status) && !SharedDCompDevice)
        status = SharedDesktopDevice->QueryInterface(IID_PPV_ARGS(&SharedDCompDevice));

    if (HR_SUCCESS(status) && !SharedCompositionDevice3)
        SharedDesktopDevice->QueryInterface(IID_PPV_ARGS(&SharedCompositionDevice3));
#endif

    return status;
}

HRESULT PH_EMENU_DX::CreateSharedTextAndImagingFactories()
{
    HRESULT status = S_OK;

    if (HR_SUCCESS(status) && !SharedWicFactory)
    {
        status = PhGetClassObject(L"windowscodecs.dll", CLSID_WICImagingFactory, IID_PPV_ARGS(&SharedWicFactory));
    }

    if (HR_SUCCESS(status) && !SharedDWriteFactory)
    {
        status = DynamicDWriteCreateFactory(DWRITE_FACTORY_TYPE_SHARED, __uuidof(IDWriteFactory), reinterpret_cast<IUnknown**>(&SharedDWriteFactory));
    }

    return status;
}

// Caller must hold SharedLock exclusively. Missing objects are created on demand,
// so a transient failure is retried by the next menu instead of being cached.
HRESULT PH_EMENU_DX::InitializeSharedResources()
{
    HRESULT status;

#ifdef PH_EMENU_DX_ENABLE_WINRT_COMPOSITION
    {
        PVOID controller = nullptr;
        PhCreateDispatcherQueueForCurrentThread(&controller);
    }
    RoInitialize(RO_INIT_MULTITHREADED);
#endif

    status = CreateSharedD3DDevice();
    if (HR_SUCCESS(status))
        status = CreateSharedD2DDevice();
    if (HR_SUCCESS(status))
        status = CreateSharedDCompDevice();
    if (HR_SUCCESS(status))
        status = CreateSharedTextAndImagingFactories();

    return status;
}

VOID PH_EMENU_DX::RegisterMenuContext()
{
    ThreadId = HandleToUlong(NtCurrentThreadId());

    PhAcquireQueuedLockExclusive(&MenuListLock);
    PhAddItemList(MenuList, this);
    PhReleaseQueuedLockExclusive(&MenuListLock);
}

VOID PH_EMENU_DX::UnregisterMenuContext()
{
    ULONG index;

    PhAcquireQueuedLockExclusive(&MenuListLock);

    index = PhFindItemList(MenuList, this);
    if (index != ULONG_MAX)
        PhRemoveItemList(MenuList, index);

    PhReleaseQueuedLockExclusive(&MenuListLock);
}

BOOL PH_EMENU_DX::IsWindowInMenuChain(
    _In_opt_ HWND hwnd
    )
{
    ULONG currentThreadId;

    if (!hwnd || !MenuList)
        return FALSE;

    // Only menus on the calling thread belong to its menu chain.
    currentThreadId = HandleToUlong(NtCurrentThreadId());

    PhAcquireQueuedLockShared(&MenuListLock);

    for (ULONG i = 0; std::cmp_less(i, MenuList->Count); i++)
    {
        PPH_EMENU_DX cur = static_cast<PPH_EMENU_DX>(MenuList->Items[i]);

        if (cur->ThreadId != currentThreadId)
            continue;

        if (cur->WindowHandle == hwnd || IsChild(cur->WindowHandle, hwnd))
        {
            PhReleaseQueuedLockShared(&MenuListLock);
            return TRUE;
        }
    }

    PhReleaseQueuedLockShared(&MenuListLock);

    return FALSE;
}

RTL_ATOM PH_EMENU_DX::PhRegisterMenuClass()
{
    static PH_INITONCE initOnce = PH_INITONCE_INIT;
    static RTL_ATOM windowAtom = RTL_ATOM_INVALID_ATOM;

    if (PhBeginInitOnce(&initOnce))
    {
        WNDCLASSEX wcex;

        RtlZeroMemory(&wcex, sizeof(WNDCLASSEX));
        wcex.cbSize = sizeof(WNDCLASSEX);
        wcex.style = CS_GLOBALCLASS;// | CS_DROPSHADOW;
        wcex.lpfnWndProc = PH_EMENU_DX::WindowProc;
        wcex.cbWndExtra = sizeof(PVOID);
        wcex.hInstance = reinterpret_cast<HINSTANCE>(PhInstanceHandle);
        wcex.hCursor = LoadCursor(nullptr, IDC_ARROW);
        wcex.lpszClassName = PH_EMENU_DX_CLASSNAME;
        windowAtom = RegisterClassEx(&wcex);

        MenuList = PhCreateList(PH_EMENU_DX_INITIAL_LIST_SIZE);

        PhEndInitOnce(&initOnce);
    }

    return windowAtom;
}

BOOLEAN PH_EMENU_DX::ShowInternal(
    _In_ PPH_EMENU Menu,
    _In_ HWND WindowHandle,
    _In_ ULONG Flags,
    _In_ ULONG Align,
    _In_ LONG X,
    _In_ LONG Y,
    _In_ BOOLEAN DarkMode,
    _In_opt_ PPH_EMENU_DX ParentContext,
    _Out_ PPH_EMENU_ITEM* SelectedItem
    )
{
    PH_EMENU_DX menuWindow(Menu, Flags, Align, DarkMode, ParentContext);
    return menuWindow.Run(WindowHandle, X, Y, SelectedItem);
}

VOID PhEMenuDxUnregisterClass()
{
    PH_EMENU_DX::UnregisterWindowClass();
}

// Renders a single font glyph into a 32bpp premultiplied top-down DIB. The result
// is an ordinary menu bitmap, so it works with both emenudx and TrackPopupMenu.
HBITMAP PH_EMENU_DX::CreateGlyphBitmap(
    _In_ PCWSTR Glyph,
    _In_ BOOLEAN ColorFont,
    _In_ COLORREF Color,
    _In_ LONG Size
    )
{
    HRESULT status = E_FAIL;
    HBITMAP bitmapHandle = nullptr;
    PVOID bits = nullptr;
    PCWSTR fontName = ColorFont ? PH_EMENU_DX_GLYPH_EMOJI_FONT_NAME : PH_EMENU_DX_GLYPH_FONT_NAME;
    ComPtr<ID2D1Factory1> d2dFactory;
    ComPtr<IDWriteFactory> dwriteFactory;
    ComPtr<IWICImagingFactory> wicFactory;
    ComPtr<IDWriteFontCollection> fontCollection;
    ComPtr<IDWriteTextFormat> textFormat;
    ComPtr<IDWriteTextLayout> textLayout;
    DWRITE_OVERHANG_METRICS overhang = {};
    ComPtr<IWICBitmap> wicBitmap;
    ComPtr<ID2D1RenderTarget> renderTarget;
    ComPtr<ID2D1SolidColorBrush> brush;

    if (!Glyph || Glyph[0] == UNICODE_NULL || std::cmp_less_equal(Size, 0))
        return nullptr;

    PhAcquireQueuedLockExclusive(&SharedLock);
    if (HR_SUCCESS(InitializeSharedResources()))
    {
        d2dFactory = SharedD2DFactory;
        dwriteFactory = SharedDWriteFactory;
        wicFactory = SharedWicFactory;
        status = d2dFactory && dwriteFactory && wicFactory ? S_OK : E_FAIL;
    }
    PhReleaseQueuedLockExclusive(&SharedLock);

    if (HR_FAILED(status))
        return nullptr;

    // Windows 10 has the MDL2 assets font, which shares most code points. A glyph the
    // icon font doesn't have would be drawn from a fallback font as a box or a letter,
    // so fail instead and let the caller pick another glyph.
    if (!ColorFont && HR_SUCCESS(dwriteFactory->GetSystemFontCollection(&fontCollection, FALSE)))
    {
        UINT32 index;
        BOOL exists = FALSE;
        ComPtr<IDWriteFontFamily> fontFamily;
        ComPtr<IDWriteFont> font;
        UINT32 codePoint = Glyph[0];

        if (HR_FAILED(fontCollection->FindFamilyName(fontName, &index, &exists)) || !exists)
        {
            fontName = PH_EMENU_DX_GLYPH_FONT_NAME_LEGACY;

            if (HR_FAILED(fontCollection->FindFamilyName(fontName, &index, &exists)) || !exists)
                return nullptr;
        }

        if (PH_UNICODE_UTF16_IS_HIGH_SURROGATE(Glyph[0]) && PH_UNICODE_UTF16_IS_LOW_SURROGATE(Glyph[1]))
            codePoint = PH_UNICODE_UTF16_TO_CODE_POINT(Glyph[0], Glyph[1]);

        if (HR_SUCCESS(fontCollection->GetFontFamily(index, &fontFamily)) &&
            HR_SUCCESS(fontFamily->GetFirstMatchingFont(DWRITE_FONT_WEIGHT_NORMAL, DWRITE_FONT_STRETCH_NORMAL, DWRITE_FONT_STYLE_NORMAL, &font)))
        {
            exists = FALSE;

            if (HR_FAILED(font->HasCharacter(codePoint, &exists)) || !exists)
                return nullptr;
        }
    }

    {
        FLOAT fontSize = static_cast<FLOAT>(Size);

        // Two passes: measure the drawn glyph (its ink, not the line box) and, for
        // emoji, grow or shrink the font so the ink fills the box. Fluent glyphs keep
        // the font size Windows draws them at. The second pass builds the final layout.
        for (ULONG pass = 0; std::cmp_less(pass, 2) && HR_SUCCESS(status); pass++)
        {
            textFormat.Reset();
            textLayout.Reset();

            status = dwriteFactory->CreateTextFormat(
                fontName,
                nullptr,
                DWRITE_FONT_WEIGHT_NORMAL,
                DWRITE_FONT_STYLE_NORMAL,
                DWRITE_FONT_STRETCH_NORMAL,
                fontSize,
                PH_EMENU_DX_LOCALE_NAME,
                &textFormat
                );

            if (HR_SUCCESS(status))
            {
                textFormat->SetTextAlignment(DWRITE_TEXT_ALIGNMENT_CENTER);
                textFormat->SetParagraphAlignment(DWRITE_PARAGRAPH_ALIGNMENT_CENTER);
                textFormat->SetWordWrapping(DWRITE_WORD_WRAPPING_NO_WRAP);

                status = dwriteFactory->CreateTextLayout(
                    Glyph,
                    static_cast<UINT32>(PhCountStringZ(Glyph)),
                    textFormat.Get(),
                    static_cast<FLOAT>(Size),
                    static_cast<FLOAT>(Size),
                    &textLayout
                    );
            }

            if (HR_SUCCESS(status))
                status = textLayout->GetOverhangMetrics(&overhang);

            if (HR_SUCCESS(status) && std::cmp_equal(pass, 0))
            {
                FLOAT inkWidth = static_cast<FLOAT>(Size) + overhang.left + overhang.right;
                FLOAT inkHeight = static_cast<FLOAT>(Size) + overhang.top + overhang.bottom;
                FLOAT inkExtent = std::max(inkWidth, inkHeight);

                if (!ColorFont || inkExtent <= 0.0f)
                    break;

                fontSize = fontSize * static_cast<FLOAT>(Size) / inkExtent;
            }
        }
    }

    if (HR_SUCCESS(status))
    {
        status = wicFactory->CreateBitmap(
            static_cast<UINT>(Size),
            static_cast<UINT>(Size),
            GUID_WICPixelFormat32bppPBGRA,
            WICBitmapCacheOnLoad,
            &wicBitmap
            );
    }

    if (HR_SUCCESS(status))
    {
        status = d2dFactory->CreateWicBitmapRenderTarget(
            wicBitmap.Get(),
            D2D1::RenderTargetProperties(
                D2D1_RENDER_TARGET_TYPE_DEFAULT,
                D2D1::PixelFormat(DXGI_FORMAT_B8G8R8A8_UNORM, D2D1_ALPHA_MODE_PREMULTIPLIED),
                PH_EMENU_DX_DPI_BASE,
                PH_EMENU_DX_DPI_BASE
                ),
            &renderTarget
            );
    }

    if (HR_SUCCESS(status))
    {
        status = renderTarget->CreateSolidColorBrush(
            D2D1::ColorF(GetRValue(Color) / 255.0f, GetGValue(Color) / 255.0f, GetBValue(Color) / 255.0f),
            &brush
            );
    }

    if (HR_SUCCESS(status))
    {
        // Overhangs are how far the ink reaches past each edge of the box; shifting by
        // half their difference centers the ink itself.
        D2D1_POINT_2F origin = D2D1::Point2F(
            (overhang.left - overhang.right) / PH_EMENU_DX_HALF_FACTOR,
            (overhang.top - overhang.bottom) / PH_EMENU_DX_HALF_FACTOR
            );

        // A transparent target can't take ClearType; the emoji font brings its own
        // colors and ignores the brush.
        renderTarget->BeginDraw();
        renderTarget->SetTextAntialiasMode(D2D1_TEXT_ANTIALIAS_MODE_GRAYSCALE);
        renderTarget->Clear(PH_EMENU_DX_COLOR_TRANSPARENT);
        renderTarget->DrawTextLayout(
            origin,
            textLayout.Get(),
            brush.Get(),
            ColorFont ? D2D1_DRAW_TEXT_OPTIONS_ENABLE_COLOR_FONT : D2D1_DRAW_TEXT_OPTIONS_NONE
            );
        status = renderTarget->EndDraw();
    }

    if (HR_SUCCESS(status))
    {
        bitmapHandle = PhCreateDIBSection(nullptr, PHBF_TOPDOWNDIB, Size, Size, &bits);

        if (!bitmapHandle || !bits)
            status = E_OUTOFMEMORY;
    }

    if (HR_SUCCESS(status))
    {
        status = wicBitmap->CopyPixels(
            nullptr,
            static_cast<UINT>(Size) * sizeof(ULONG),
            static_cast<UINT>(Size) * static_cast<UINT>(Size) * sizeof(ULONG),
            static_cast<PBYTE>(bits)
            );
    }

    if (HR_FAILED(status))
    {
        if (bitmapHandle)
        {
            DeleteBitmap(bitmapHandle);
            bitmapHandle = nullptr;
        }

        dprintf("PhEMenuDx: Glyph bitmap failed with 0x%08x\n", status);
    }

    return bitmapHandle;
}

HBITMAP PhCreateEMenuGlyphBitmap(
    _In_ PCWSTR Glyph,
    _In_ BOOLEAN ColorFont,
    _In_ COLORREF Color,
    _In_ LONG Size
    )
{
    return PH_EMENU_DX::CreateGlyphBitmap(Glyph, ColorFont, Color, Size);
}
