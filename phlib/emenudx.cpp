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
// target and BeginDraw/EndDraw lifecycle. Drawing uses the supplied update
// offset in DIPs and commits after the surface update is complete.
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
#define PH_EMENU_DX_ENABLE_BORDER

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
#include <span>

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

typedef enum _PH_EMENU_DX_BACKDROP_TYPE
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

    static VOID UnregisterWindowClass(VOID);

    static BOOLEAN IsMenuSupported(
        _In_ PPH_EMENU Menu
        );

private:
    PH_EMENU_DX(_In_ const PH_EMENU_DX& Other) = delete;
    PH_EMENU_DX& operator=(_In_ const PH_EMENU_DX& Other) = delete;

    static LRESULT CALLBACK WindowProc(
        _In_ HWND WindowHandle,
        _In_ UINT WindowMessage,
        _In_ WPARAM wParam,
        _In_ LPARAM lParam
        );

    BOOLEAN Run(
        _In_ HWND OwnerWindowHandle,
        _In_ LONG X,
        _In_ LONG Y,
        _Out_ PPH_EMENU_ITEM* SelectedItem
        );

    HRESULT BeforeEnteringMessageLoop(
        _In_ HWND OwnerWindowHandle,
        _In_ LONG X,
        _In_ LONG Y
        );

    BOOLEAN EnterMessageLoop(VOID);

    VOID AfterLeavingMessageLoop(VOID) noexcept;

    HRESULT CreateMainWindow(
        _In_ HWND OwnerWindowHandle,
        _In_ LONG X,
        _In_ LONG Y
        );

    HRESULT ShowMainWindow(VOID);

    VOID DestroyMainWindow(VOID);

    HRESULT CreateD3DDevice(VOID);

    VOID DestroyD3DDevice(VOID);

    HRESULT CreateD2DDevice(VOID);

    VOID DestroyD2DDevice(VOID);

    HRESULT CreateDWriteFactory(VOID);

    VOID DestroyDWriteFactory(VOID);

    HRESULT CreateDCompDevice(VOID);

    VOID DestroyDCompDevice(VOID);

    HRESULT CreateResources(VOID);

    VOID ReleaseResources(VOID);

    HRESULT CreateDrawingResources(VOID);

    VOID DestroyDrawingResources(VOID);

    HRESULT InitializeTextFormats(VOID);

    HRESULT InitializeBrushes(VOID);

    HRESULT CreateDCompVisualTree(VOID);

    VOID DestroyDCompVisualTree(VOID);

    HRESULT CreateDCompTarget(VOID);

    VOID DestroyDCompTarget(VOID);

    HRESULT CreateDCompRootVisual(VOID);

    VOID DestroyDCompRootVisual(VOID);

    HRESULT CreateDCompMenuVisual(VOID);

    VOID DestroyDCompMenuVisual(VOID);

    HRESULT CreateDCompMenuSurface(VOID);

    VOID DestroyDCompMenuSurface(VOID);

    HRESULT DrawDCompMenuSurface(VOID);

    HRESULT BindDCompRootVisualToTarget(VOID);

    HRESULT BindDCompMenuSurfaceToVisual(VOID);

    HRESULT SetBackfaceVisibilityOnDCompMenuVisual(VOID);

    HRESULT AddDCompMenuVisualToRootVisual(VOID);

    HRESULT CommitDCompDevice(VOID);

    VOID CommitUpdate(VOID);

    VOID Render(VOID);

    VOID CalculateLayout(VOID);

    VOID CalculatePosition(
        _In_ LONG X,
        _In_ LONG Y,
        _Out_ PLONG OutX,
        _Out_ PLONG OutY
        );

    VOID ReleaseCachedLayouts(VOID);

    HRESULT CreateTextLayout(
        _In_ PWSTR Text,
        _In_ SIZE_T TextLen,
        _In_ IDWriteTextFormat* Format,
        _In_ FLOAT MaxWidth,
        _In_ FLOAT MaxHeight,
        _Out_ IDWriteTextLayout** Layout
        );

    HRESULT CreateBitmapFromIcon(
        _In_ HICON Icon,
        _Out_ ID2D1Bitmap** Bitmap
        );

    HRESULT CreateBitmapFromHBitmap(
        _In_ HBITMAP BitmapHandle,
        _Out_ ID2D1Bitmap** Bitmap
        );

    VOID RebuildTextLayouts(VOID);

    BOOLEAN CanScrollUp(VOID);

    BOOLEAN CanScrollDown(VOID);

    FLOAT GetItemTop(
        _In_ ULONG Index
        );

    VOID EnsureItemVisible(
        _In_ ULONG Index
        );

    VOID SetHoverIndex(
        _In_ ULONG Index
        );

    VOID ActivateHoverItem(VOID);

    static WCHAR GetItemMnemonic(
        _In_ PPH_EMENU_ITEM Item
        );

    HRESULT OpenSubMenu(VOID);

    VOID DestroySubMenuChain(VOID);

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

    static VOID ResetGlobalResources(VOID);

    static VOID ReleaseSharedResources(VOID);

    static HRESULT CreateSharedD3DDevice(VOID);

    static HRESULT CreateSharedD2DDevice(VOID);

    static HRESULT CreateSharedDCompDevice(VOID);

    static HRESULT CreateSharedTextAndImagingFactories(VOID);

    static HRESULT InitializeSharedResources(VOID);

    VOID RegisterMenuContext(VOID);

    VOID UnregisterMenuContext(VOID);

    static BOOL IsWindowInMenuChain(
        _In_opt_ HWND hwnd
        );

    static RTL_ATOM PhRegisterMenuClass(VOID);

private:
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

private:
    HWND WindowHandle{};
    BOOLEAN Registered{};
    ULONG ThreadId{};
    ULONG ResourceGeneration{};

    PPH_EMENU Menu{};

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
            ULONG Spare : 26;
        };
    };

    ULONG Flags{};
    ULONG Align{};

    FLOAT Width{};
    FLOAT Height{};
    FLOAT ItemHeight{};
    FLOAT IconWidth{};
    FLOAT MaxTextWidth{};
    FLOAT MaxShortcutWidth{};

    FLOAT Dpi{};
    FLOAT Scale{};

    PPH_EMENU_ITEM SelectedItem{};
    PPH_EMENU_ITEM HoverItem{};
    LONG HoverIndex{};
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
    IDCompositionVisual2* RootVisual{};
    IDCompositionVisual2* MenuVisual{};
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

    // Cached layouts to avoid repeated DWrite allocations during rendering.
    ULONG LayoutCount{}; // Number of entries in each cached array
    IDWriteTextLayout** TextLayouts{};
    IDWriteTextLayout** ShortcutLayouts{};
    DWRITE_TEXT_METRICS* TextMetrics{};
    DWRITE_TEXT_METRICS* ShortcutMetrics{};
    ID2D1Bitmap** ItemBitmaps{};
    IWICImagingFactory* WicFactory{};

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
#define PH_EMENU_DX_ITEM_HEIGHT 28.0f
#define PH_EMENU_DX_SEPARATOR_HEIGHT 6.0f
#define PH_EMENU_DX_MIN_WIDTH 280.0f
#define PH_EMENU_DX_PADDING 8.0f
#define PH_EMENU_DX_ICON_SIZE 18.0f
#define PH_EMENU_DX_ICON_COLUMN_WIDTH 24.0f
#define PH_EMENU_DX_TEXT_SHORTCUT_GAP 32.0f
#define PH_EMENU_DX_ROUNDED_CORNER 10.0f
#define PH_EMENU_DX_HOVER_CORNER 4.0f
#define PH_EMENU_DX_HOVER_MARGIN 5.0f
#define PH_EMENU_DX_MENU_VERTICAL_PADDING 6.0f
#define PH_EMENU_DX_MIN_HEIGHT 20.0f

#define PH_EMENU_DX_SHADOW_BLUR_RADIUS 20.0f
#define PH_EMENU_DX_SHADOW_OPACITY 0.80f
#define PH_EMENU_DX_SHADOW_OFFSET_Y 4.0f
#define PH_EMENU_DX_SHADOW_COLOR D2D1::ColorF(D2D1::ColorF::Black)
#define PH_EMENU_DX_SHADOW_PADDING 32.0f

#define PH_EMENU_DX_DARK_BACKGROUND D2D1::ColorF(0.12f, 0.12f, 0.12f, 1.0f)
#define PH_EMENU_DX_LIGHT_BACKGROUND D2D1::ColorF(0.98f, 0.98f, 0.98f, 1.0f)

#define PH_EMENU_DX_TEXT_BRUSH_OPACITY 1.0f
#define PH_EMENU_DX_SHORTCUT_BRUSH_OPACITY 1.0f
#define PH_EMENU_DX_HOVER_DARK_OPACITY 0.25f
#define PH_EMENU_DX_HOVER_LIGHT_OPACITY 0.15f
#define PH_EMENU_DX_BORDER_DARK_OPACITY 0.18f
#define PH_EMENU_DX_BORDER_LIGHT_OPACITY 0.12f
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
#define PH_EMENU_DX_ARROW_LINE_WIDTH 1.2f
#define PH_EMENU_DX_ARROW_SIZE 4.0f
#define PH_EMENU_DX_ARROW_RIGHT_MARGIN 16.0f
#define PH_EMENU_DX_ARROW_TIP_MARGIN 12.0f
#define PH_EMENU_DX_SCROLL_ARROW_PADDING 8.0f
#define PH_WINDOW_TIMER_SCROLL 2

#define PH_EMENU_DX_DEFAULT_MENU_SHOW_DELAY 400
#define PH_EMENU_DX_DPI_BASE 96.0f

#define PH_EMENU_DX_DISABLED_ICON_OPACITY 0.5f
#define PH_EMENU_DX_SUBMENU_MAX_HEIGHT_PERCENT 0.7f
#define PH_EMENU_DX_ROOT_MAX_HEIGHT_PERCENT 0.8f
#define PH_EMENU_DX_TEXT_FONT_SIZE 13.0f
#define PH_EMENU_DX_SHORTCUT_FONT_SIZE 10.0f
#define PH_EMENU_DX_FONT_NAME L"Segoe UI"
#define PH_EMENU_DX_RETRY_TIMER_INTERVAL 50
#define PH_EMENU_DX_SCROLL_TIMER_INTERVAL 50
#define PH_EMENU_DX_MAX_TEXT_WIDTH 1000.0f
#define PH_EMENU_DX_BORDER_WIDTH 2.0f
#define PH_EMENU_DX_BORDER_OFFSET (PH_EMENU_DX_BORDER_WIDTH / PH_EMENU_DX_HALF_FACTOR)

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
    *SelectedItem = NULL;

    // Let PhShowEMenu fall back to TrackPopupMenu for menus this renderer can't
    // reproduce (columns, right-to-left order, menu bars) and for empty menus.
    if (!PH_EMENU_DX::IsMenuSupported(Menu))
        return FALSE;

    return PH_EMENU_DX::ShowInternal(Menu, WindowHandle, Flags, Align, X, Y, DarkMode, NULL, SelectedItem);
}

PH_QUEUED_LOCK PH_EMENU_DX::SharedLock = PH_QUEUED_LOCK_INIT;
ULONG PH_EMENU_DX::SharedGeneration = 0;
#ifdef PH_EMENU_DX_ENABLE_D3D12
ID3D12Device* PH_EMENU_DX::SharedD3D12Device = NULL;
ID3D12CommandQueue* PH_EMENU_DX::SharedD3D12CommandQueue = NULL;
ID3D11Device* PH_EMENU_DX::SharedD3D11Device12 = NULL;
ID3D11On12Device* PH_EMENU_DX::SharedD3D11On12Device = NULL;
#endif
#ifdef PH_EMENU_DX_ENABLE_D3D11
ID3D11Device* PH_EMENU_DX::SharedD3D11Device = NULL;
#endif
ID3D10Device1* PH_EMENU_DX::SharedD3DDevice = NULL;
ID2D1Factory1* PH_EMENU_DX::SharedD2DFactory = NULL;
ID2D1Device* PH_EMENU_DX::SharedD2DDevice = NULL;
#ifdef PH_EMENU_DX_ENABLE_WINRT_COMPOSITION
ICompositor* PH_EMENU_DX::SharedCompositor = NULL;
#else
IDCompositionDevice2* PH_EMENU_DX::SharedDCompDevice = NULL;
IDCompositionDesktopDevice* PH_EMENU_DX::SharedDesktopDevice = NULL;
IDCompositionDevice3* PH_EMENU_DX::SharedCompositionDevice3 = NULL;
#endif
IDWriteFactory* PH_EMENU_DX::SharedDWriteFactory = NULL;
IWICImagingFactory* PH_EMENU_DX::SharedWicFactory = NULL;
#ifdef PH_EMENU_DX_ENABLE_WINRT_COMPOSITION
ICompositionGraphicsDevice* PH_EMENU_DX::SharedCompositionGraphicsDevice = NULL;
#endif

PH_QUEUED_LOCK PH_EMENU_DX::MenuListLock = PH_QUEUED_LOCK_INIT;
PPH_LIST PH_EMENU_DX::MenuList = NULL;

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
        context->WindowHandle = NULL;
        PhRemoveWindowContextEx(WindowHandle);
        break;
    case WM_MOUSEACTIVATE:
        return MA_NOACTIVATE;
    case WM_SETCURSOR:
        if (context->OnSetCursor(WindowHandle, (HWND)wParam, LOWORD(lParam), HIWORD(lParam)))
            return TRUE;
        break;
    case WM_NCHITTEST:
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
        context->OnTimer(WindowHandle, (ULONG)wParam);
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
        context->OnKeyDown(WindowHandle, (ULONG)wParam);
        break;
    case WM_SYSKEYDOWN:
        context->OnSysKeyDown(WindowHandle, (ULONG)wParam, !!(lParam & 0x40000000));
        break;
    case WM_CHAR:
    case WM_SYSCHAR:
        if (context->OnChar(WindowHandle, (WCHAR)wParam))
            return 0;
        break;
    case WM_ACTIVATEAPP:
        context->OnActivateApp(WindowHandle, (BOOLEAN)wParam);
        break;
    case WM_ACTIVATE:
        return context->OnActivate(WindowHandle, (ULONG)wParam, (HWND)lParam);
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
    this->IsSubMenu = ParentContext != NULL;
}

PH_EMENU_DX::~PH_EMENU_DX()
{
    this->AfterLeavingMessageLoop();
}

BOOLEAN PH_EMENU_DX::Run(
    _In_ HWND OwnerWindowHandle,
    _In_ LONG X,
    _In_ LONG Y,
    _Out_ PPH_EMENU_ITEM* SelectedItem
    )
{
    BOOLEAN result = FALSE;

    if (SelectedItem)
        *SelectedItem = NULL;

    if (HR_SUCCESS(this->BeforeEnteringMessageLoop(OwnerWindowHandle, X, Y)))
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
    _In_ HWND OwnerWindowHandle,
    _In_ LONG X,
    _In_ LONG Y
    )
{
    // Determine DPI and scaling factors for the target monitor
    {
        RECT windowRect = { X, Y, X + 1, Y + 1 };

        Dpi = (FLOAT)PhGetMonitorDpi(NULL, &windowRect);

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
            FLOAT screenHeight = (FLOAT)(monitorInfo.rcWork.bottom - monitorInfo.rcWork.top) / Scale;

            ScrollViewVisibleHeight = screenHeight * (IsSubMenu ? PH_EMENU_DX_SUBMENU_MAX_HEIGHT_PERCENT : PH_EMENU_DX_ROOT_MAX_HEIGHT_PERCENT);

            if (ScrollViewVisibleHeight < PH_EMENU_DX_MIN_HEIGHT)
                ScrollViewVisibleHeight = PH_EMENU_DX_MIN_HEIGHT;
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

    // Initialize DirectWrite formats and calculate final layout/position
    HRESULT status = InitializeTextFormats();
    if (HR_FAILED(status))
        return status;

    CalculateLayout();
    CalculatePosition(X, Y, &X, &Y);

    status = CreateMainWindow(OwnerWindowHandle, X, Y);
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

BOOLEAN PH_EMENU_DX::EnterMessageLoop(VOID)
{
    // Enter the modal message loop for the menu
    while (!ExitLoop)
    {
        MSG msg;

        BOOL result = GetMessage(&msg, NULL, 0, 0);
        if (result == -1)
            return FALSE;

        if (result > 0)
        {
            // Route keyboard messages specifically to the menu window
            if (msg.message >= WM_KEYFIRST && msg.message <= WM_KEYLAST)
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

VOID PH_EMENU_DX::AfterLeavingMessageLoop(VOID) noexcept
{
    if (MouseHookHandle)
    {
        UnhookWindowsHookEx(MouseHookHandle);
        MouseHookHandle = NULL;
    }

    DestroySubMenuChain();
    DestroyMainWindow();

    if (ParentContext && ParentContext->SubMenuContext == this)
    {
        ParentContext->SubMenuContext = NULL;
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
    _In_ HWND OwnerWindowHandle,
    _In_ LONG X,
    _In_ LONG Y
    )
{
    RTL_ATOM windowAtom = PhRegisterMenuClass();
    if (windowAtom == RTL_ATOM_INVALID_ATOM)
        return E_FAIL;

    // Create the menu window
    {
        FLOAT surfacePadding = 0.0f;
#ifdef PH_EMENU_DX_ENABLE_DROPSHADOW
        surfacePadding = PH_EMENU_DX_SHADOW_PADDING;
#endif
        FLOAT windowWidth = Width + surfacePadding * PH_EMENU_DX_HALF_FACTOR;
        FLOAT windowHeight = Height + surfacePadding * PH_EMENU_DX_HALF_FACTOR;

        WindowHandle = PhCreateWindowEx(
            MAKEINTATOM(windowAtom),
            NULL,
#ifndef PH_EMENU_DX_ENABLE_WINRT_COMPOSITION
            WS_POPUP | WS_BORDER,
#else
            WS_POPUP,
#endif
            WS_EX_TOPMOST | WS_EX_TOOLWINDOW | WS_EX_NOACTIVATE | WS_EX_NOREDIRECTIONBITMAP,
            X,
            Y,
            (LONG)(windowWidth * Scale),
            (LONG)(windowHeight * Scale),
            OwnerWindowHandle,
            NULL,
            NULL,
            this
            );
    }

    if (!WindowHandle)
        return E_FAIL;

    RegisterMenuContext();
    Registered = TRUE;
    return S_OK;
}

HRESULT PH_EMENU_DX::ShowMainWindow(VOID)
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
        MouseHookHandle = SetWindowsHookEx(WH_MOUSE_LL, MouseHookCallback, NULL, 0);

    ShowWindow(WindowHandle, SW_SHOWNOACTIVATE);
    return S_OK;
}

VOID PH_EMENU_DX::DestroyMainWindow(VOID)
{
    if (WindowHandle)
    {
        ShowWindow(WindowHandle, SW_HIDE);
        DestroyWindow(WindowHandle);
        WindowHandle = NULL;
    }
}

HRESULT PH_EMENU_DX::CreateD3DDevice(VOID)
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
            status = D3D12Device->CreateCommandList(0, D3D12_COMMAND_LIST_TYPE_DIRECT, D3D12CommandAllocator, NULL, IID_PPV_ARGS(&D3D12CommandList));
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
            D3D12FenceEvent = CreateEvent(NULL, FALSE, FALSE, NULL);

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

VOID PH_EMENU_DX::DestroyD3DDevice(VOID)
{
#ifdef PH_EMENU_DX_ENABLE_D3D12
    if (D3D12FenceEvent)
    {
        CloseHandle(D3D12FenceEvent);
        D3D12FenceEvent = NULL;
    }
    if (D3D12Fence)
    {
        D3D12Fence->Release();
        D3D12Fence = NULL;
    }
    if (D3D12CommandList)
    {
        D3D12CommandList->Release();
        D3D12CommandList = NULL;
    }
    if (D3D12CommandAllocator)
    {
        D3D12CommandAllocator->Release();
        D3D12CommandAllocator = NULL;
    }
    if (D3D12CommandQueue)
    {
        D3D12CommandQueue->Release();
        D3D12CommandQueue = NULL;
    }
    if (D3D11DeviceContext)
    {
        D3D11DeviceContext->Release();
        D3D11DeviceContext = NULL;
    }
    if (D3D11On12Device)
    {
        D3D11On12Device->Release();
        D3D11On12Device = NULL;
    }
    if (D3DDevice)
    {
        D3DDevice->Release();
        D3DDevice = NULL;
    }
    if (D3D12Device)
    {
        D3D12Device->Release();
        D3D12Device = NULL;
    }
#else
    if (D3DDevice)
    {
        D3DDevice->Release();
        D3DDevice = NULL;
    }
#endif
}

HRESULT PH_EMENU_DX::CreateD2DDevice(VOID)
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

VOID PH_EMENU_DX::DestroyD2DDevice(VOID)
{
    if (D2DDeviceContext)
    {
        D2DDeviceContext->Release();
        D2DDeviceContext = NULL;
    }
    if (D2DDevice)
    {
        D2DDevice->Release();
        D2DDevice = NULL;
    }
    if (D2DFactory)
    {
        D2DFactory->Release();
        D2DFactory = NULL;
    }
}

HRESULT PH_EMENU_DX::CreateDWriteFactory(VOID)
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

VOID PH_EMENU_DX::DestroyDWriteFactory(VOID)
{
    if (TextFormat)
    {
        TextFormat->Release();
        TextFormat = NULL;
    }
    if (ShortcutFormat)
    {
        ShortcutFormat->Release();
        ShortcutFormat = NULL;
    }
    if (DWriteFactory)
    {
        DWriteFactory->Release();
        DWriteFactory = NULL;
    }
}

HRESULT PH_EMENU_DX::CreateDCompDevice(VOID)
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

VOID PH_EMENU_DX::DestroyDCompDevice(VOID)
{
#ifdef PH_EMENU_DX_ENABLE_WINRT_COMPOSITION
    if (Compositor)
    {
        Compositor->Release();
        Compositor = NULL;
    }
#else
    if (CompositionDevice3)
    {
        CompositionDevice3->Release();
        CompositionDevice3 = NULL;
    }
    if (DesktopDevice)
    {
        DesktopDevice->Release();
        DesktopDevice = NULL;
    }
    if (DCompDevice)
    {
        DCompDevice->Release();
        DCompDevice = NULL;
    }
#endif
}

HRESULT PH_EMENU_DX::CreateResources(VOID)
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

VOID PH_EMENU_DX::ReleaseResources(VOID)
{
#ifdef PH_EMENU_DX_ENABLE_D3D12
    if (D3D12CommandQueue && D3D12Fence)
    {
        UINT64 fenceValue = D3D12FenceValue++;
        D3D12CommandQueue->Signal(D3D12Fence, fenceValue);
        if (D3D12Fence->GetCompletedValue() < fenceValue)
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

HRESULT PH_EMENU_DX::CreateDrawingResources(VOID)
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

            status = dxgiFactory->CreateSwapChainForComposition(D3DDevice, &description, NULL, &SwapChain);
        }
    }

    if (HR_FAILED(status))
        return status;
#endif

    {
        D2D1_ROUNDED_RECT roundRect = D2D1::RoundedRect(D2D1::RectF(0, 0, Width, Height), PH_EMENU_DX_ROUNDED_CORNER, PH_EMENU_DX_ROUNDED_CORNER);

        if (HR_SUCCESS(D2DFactory->CreateRoundedRectangleGeometry(&roundRect, &ClipGeometry)))
        {
            // D2D 1.1: pass NULL to PushLayer; no pre-allocated layer needed
        }
    }

    return status;
}

VOID PH_EMENU_DX::DestroyDrawingResources(VOID)
{
    if (TextBrush)
    {
        TextBrush->Release();
        TextBrush = NULL;
    }
    if (ShortcutBrush)
    {
        ShortcutBrush->Release();
        ShortcutBrush = NULL;
    }
    if (HoverBrush)
    {
        HoverBrush->Release();
        HoverBrush = NULL;
    }
    if (BorderBrush)
    {
        BorderBrush->Release();
        BorderBrush = NULL;
    }
    if (SeparatorBrush)
    {
        SeparatorBrush->Release();
        SeparatorBrush = NULL;
    }
    if (ClipLayer)
    {
        ClipLayer->Release();
        ClipLayer = NULL;
    }
    if (ClipGeometry)
    {
        ClipGeometry->Release();
        ClipGeometry = NULL;
    }
    if (WicFactory)
    {
        WicFactory->Release();
        WicFactory = NULL;
    }
#ifdef PH_EMENU_DX_ENABLE_SWAPCHAIN
    if (SwapChain)
    {
        SwapChain->Release();
        SwapChain = NULL;
    }
#endif
}

HRESULT PH_EMENU_DX::InitializeTextFormats(VOID)
{
    HRESULT hr = CreateDWriteFactory();
    if (HR_FAILED(hr))
        return hr;

    hr = DWriteFactory->CreateTextFormat(
        PH_EMENU_DX_FONT_NAME,
        NULL,
        DWRITE_FONT_WEIGHT_NORMAL,
        DWRITE_FONT_STYLE_NORMAL,
        DWRITE_FONT_STRETCH_NORMAL,
        PH_EMENU_DX_TEXT_FONT_SIZE,
        L"en-us",
        &TextFormat
    );
    if (HR_FAILED(hr))
        return hr;

    hr = DWriteFactory->CreateTextFormat(
        PH_EMENU_DX_FONT_NAME,
        NULL,
        DWRITE_FONT_WEIGHT_NORMAL,
        DWRITE_FONT_STYLE_NORMAL,
        DWRITE_FONT_STRETCH_NORMAL,
        PH_EMENU_DX_SHORTCUT_FONT_SIZE,
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

HRESULT PH_EMENU_DX::InitializeBrushes(VOID)
{
    HRESULT result;
    result = D2DDeviceContext->CreateSolidColorBrush(IsDarkMode ? PH_EMENU_DX_TEXT_COLOR_DARK : PH_EMENU_DX_TEXT_COLOR_LIGHT, &TextBrush);
    if (HR_FAILED(result)) return result;
    result = D2DDeviceContext->CreateSolidColorBrush(IsDarkMode ? PH_EMENU_DX_SHORTCUT_COLOR_DARK : PH_EMENU_DX_SHORTCUT_COLOR_LIGHT, &ShortcutBrush);
    if (HR_FAILED(result)) return result;
    result = D2DDeviceContext->CreateSolidColorBrush(IsDarkMode ? PH_EMENU_DX_HOVER_COLOR_DARK : PH_EMENU_DX_HOVER_COLOR_LIGHT, &HoverBrush);
    if (HR_FAILED(result)) return result;
#ifdef PH_EMENU_DX_ENABLE_BORDER
    result = D2DDeviceContext->CreateSolidColorBrush(D2D1::ColorF(D2D1::ColorF::Black), &BorderBrush);
#else
    result = D2DDeviceContext->CreateSolidColorBrush(IsDarkMode ? PH_EMENU_DX_BORDER_COLOR_DARK : PH_EMENU_DX_BORDER_COLOR_LIGHT, &BorderBrush);
#endif
    if (HR_FAILED(result)) return result;
    result = D2DDeviceContext->CreateSolidColorBrush(IsDarkMode ? PH_EMENU_DX_SEPARATOR_COLOR_DARK : PH_EMENU_DX_SEPARATOR_COLOR_LIGHT, &SeparatorBrush);
    return result;
}

HRESULT PH_EMENU_DX::CreateDCompVisualTree(VOID)
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
        status = AddDCompMenuVisualToRootVisual();
    return status;
}

VOID PH_EMENU_DX::DestroyDCompVisualTree(VOID)
{
    DestroyDCompMenuSurface();
    DestroyDCompMenuVisual();
#ifdef PH_EMENU_DX_ENABLE_WINRT_COMPOSITION
#ifdef PH_EMENU_DX_ENABLE_HOSTBACKDROPBRUSH
    if (BackdropVisual)
    {
        BackdropVisual->Release();
        BackdropVisual = NULL;
    }
    if (BackdropBrush)
    {
        BackdropBrush->Release();
        BackdropBrush = NULL;
    }
#endif
#ifdef PH_EMENU_DX_ENABLE_DROPSHADOW
    if (ShadowVisual)
    {
        ShadowVisual->Release();
        ShadowVisual = NULL;
    }
    if (DropShadow)
    {
        DropShadow->Release();
        DropShadow = NULL;
    }
#endif
#endif
    DestroyDCompRootVisual();
    DestroyDCompTarget();
}

HRESULT PH_EMENU_DX::CreateDCompTarget(VOID)
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

VOID PH_EMENU_DX::DestroyDCompTarget(VOID)
{
#ifdef PH_EMENU_DX_ENABLE_WINRT_COMPOSITION
    if (CompositionTarget)
    {
        CompositionTarget->Release();
        CompositionTarget = NULL;
    }
#else
    if (DCompTarget)
    {
        DCompTarget->Release();
        DCompTarget = NULL;
    }
#endif
}

HRESULT PH_EMENU_DX::CreateDCompRootVisual(VOID)
{
    HRESULT status = S_OK;

#ifdef PH_EMENU_DX_ENABLE_WINRT_COMPOSITION
    status = Compositor->CreateContainerVisual(&RootVisual);
#else
    status = DCompDevice->CreateVisual(&RootVisual);
#endif

    return status;
}

VOID PH_EMENU_DX::DestroyDCompRootVisual(VOID)
{
    if (RootVisual)
    {
        RootVisual->Release();
        RootVisual = NULL;
    }
}

HRESULT PH_EMENU_DX::CreateDCompMenuVisual(VOID)
{
#ifdef PH_EMENU_DX_ENABLE_WINRT_COMPOSITION
    return Compositor->CreateSpriteVisual(&MenuVisual);
#else
    return DCompDevice->CreateVisual(&MenuVisual);
#endif
}

VOID PH_EMENU_DX::DestroyDCompMenuVisual(VOID)
{
    if (MenuVisual)
    {
        MenuVisual->Release();
        MenuVisual = NULL;
    }
}

HRESULT PH_EMENU_DX::CreateDCompMenuSurface(VOID)
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

VOID PH_EMENU_DX::DestroyDCompMenuSurface(VOID)
{
#ifdef PH_EMENU_DX_ENABLE_WINRT_COMPOSITION
    if (CompositionSurface)
    {
        CompositionSurface->Release();
        CompositionSurface = NULL;
    }
#else
    if (DCompSurface)
    {
        DCompSurface->Release();
        DCompSurface = NULL;
    }
#endif
}

HRESULT PH_EMENU_DX::DrawDCompMenuSurface(VOID)
{
    HRESULT status = S_OK;
#if defined(PH_EMENU_DX_ENABLE_SWAPCHAIN) || defined(PH_EMENU_DX_ENABLE_WINRT_COMPOSITION)
    IDXGISurface* dxgiSurface = NULL;
#endif

#ifdef PH_EMENU_DX_ENABLE_SWAPCHAIN
    if (SwapChain)
    {
        // Get the back buffer of the swap chain
        status = SwapChain->GetBuffer(0, IID_PPV_ARGS(&dxgiSurface));

        if (HR_SUCCESS(status))
        {
            ID2D1Bitmap1* targetBitmap = NULL;
            D2D1_BITMAP_PROPERTIES1 bitmapProperties =
            {
                { DXGI_FORMAT_B8G8R8A8_UNORM, D2D1_ALPHA_MODE_PREMULTIPLIED },
                0.0f,
                0.0f,
                D2D1_BITMAP_OPTIONS_TARGET | D2D1_BITMAP_OPTIONS_CANNOT_DRAW,
                NULL
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

                D2DDeviceContext->SetTarget(NULL);
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
            status = surfaceInterop->BeginDraw(NULL, IID_PPV_ARGS(&dxgiSurface), &offset);
            if (HR_FAILED(status))
            {
                dprintf("PhEMenuDx: BeginDraw (WinRT) failed with 0x%08x\n", status);
            }
        }

        if (HR_SUCCESS(status))
        {
            ID2D1Bitmap1* targetBitmap = NULL;
            D2D1_BITMAP_PROPERTIES1 bitmapProperties =
            {
                { DXGI_FORMAT_B8G8R8A8_UNORM, D2D1_ALPHA_MODE_PREMULTIPLIED },
                0.0f,
                0.0f,
                D2D1_BITMAP_OPTIONS_TARGET | D2D1_BITMAP_OPTIONS_CANNOT_DRAW,
                NULL
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

                D2DDeviceContext->SetTarget(NULL);
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

    status = DCompSurface->BeginDraw(NULL, IID_PPV_ARGS(&drawingContext), &offset);
    if (HR_FAILED(status))
    {
        dprintf("PhEMenuDx: BeginDraw (DComp) failed with 0x%08x\n", status);
    }

    if (HR_SUCCESS(status))
    {
        // DirectComposition owns the target and the D2D drawing session.
        // Cached brushes and bitmaps share the same D2D device.
        ID2D1DeviceContext* resourceContext = D2DDeviceContext;
        D2DDeviceContext = drawingContext.Get();
        drawingContext->SetDpi(Dpi, Dpi);
        drawingContext->SetAntialiasMode(D2D1_ANTIALIAS_MODE_PER_PRIMITIVE);
        drawingContext->SetTextAntialiasMode(D2D1_TEXT_ANTIALIAS_MODE_GRAYSCALE);
        drawingContext->SetTransform(D2D1::Matrix3x2F::Translation(offset.x / Scale, offset.y / Scale));

        Render();

        D2DDeviceContext = resourceContext;
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

HRESULT PH_EMENU_DX::BindDCompRootVisualToTarget(VOID)
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

HRESULT PH_EMENU_DX::BindDCompMenuSurfaceToVisual(VOID)
{
    HRESULT status = S_OK;

    FLOAT surfacePadding = 0.0f;
#ifdef PH_EMENU_DX_ENABLE_DROPSHADOW
    surfacePadding = PH_EMENU_DX_SHADOW_PADDING;
#endif

#ifdef PH_EMENU_DX_ENABLE_WINRT_COMPOSITION
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
                    ICompositionBackdropBrush* backdropBrush = NULL;

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
#endif

    return status;
}

HRESULT PH_EMENU_DX::SetBackfaceVisibilityOnDCompMenuVisual(VOID)
{
#ifdef PH_EMENU_DX_ENABLE_WINRT_COMPOSITION
    return S_OK;
#else
    return MenuVisual->SetBackFaceVisibility(DCOMPOSITION_BACKFACE_VISIBILITY_HIDDEN);
#endif
}

HRESULT PH_EMENU_DX::AddDCompMenuVisualToRootVisual(VOID)
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
    return RootVisual->AddVisual(MenuVisual, FALSE, NULL);
#endif
}

HRESULT PH_EMENU_DX::CommitDCompDevice(VOID)
{
#ifndef PH_EMENU_DX_ENABLE_WINRT_COMPOSITION
    return DCompDevice->Commit();
#else
    return S_OK;
#endif
}

VOID PH_EMENU_DX::CommitUpdate(VOID)
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

VOID PH_EMENU_DX::Render(VOID)
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
    backgroundColor.a = alpha;
    D2D1_RECT_F rect = D2D1::RectF(0.0f, 0.0f, Width, Height);
    D2D1_ROUNDED_RECT roundedRect = D2D1::RoundedRect(rect, PH_EMENU_DX_ROUNDED_CORNER, PH_EMENU_DX_ROUNDED_CORNER);

    BOOLEAN noRound = BooleanFlagOn(Flags, PH_EMENU_SHOW_NO_ROUND_CORNERS);

    // Clear the render target with transparency
    D2DDeviceContext->SetTransform(D2D1::Matrix3x2F::Identity());
    D2DDeviceContext->Clear(PH_EMENU_DX_COLOR_TRANSPARENT);
    D2DDeviceContext->SetTransform(baseTransform);

    // Draw the menu background
    ID2D1SolidColorBrush* backgroundBrush = NULL;
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
                D2DDeviceContext->PushLayer(&layerParams, NULL);
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
        D2DDeviceContext->PushLayer(&layerParams, NULL);
    }

    // Set up clipping for the scrollable area
    FLOAT clipTop = 0.0f;
    FLOAT clipBottom = Height;

    if (ScrollViewTotalHeight > Height)
    {
        if (ScrollOffset > 0)
            clipTop = PH_EMENU_DX_ITEM_HEIGHT;
        if (ScrollOffset < (ScrollViewTotalHeight - Height))
            clipBottom = Height - PH_EMENU_DX_ITEM_HEIGHT;
    }

    D2D1_RECT_F clipRect = D2D1::RectF(0, clipTop, Width, clipBottom);
    D2DDeviceContext->PushAxisAlignedClip(clipRect, D2D1_ANTIALIAS_MODE_PER_PRIMITIVE);

    // Apply scroll translation for item rendering
    D2DDeviceContext->SetTransform(baseTransform * D2D1::Matrix3x2F::Translation(0.0f, -ScrollOffset));

    FLOAT y = PH_EMENU_DX_MENU_VERTICAL_PADDING;

    // Render each menu item

    for (ULONG i = 0; i < Menu->Items->Count; i++)
    {
        PPH_EMENU_ITEM item = static_cast<PPH_EMENU_ITEM>(Menu->Items->Items[i]);
        FLOAT currentHeight = FlagOn(item->Flags, PH_EMENU_SEPARATOR) ? PH_EMENU_DX_SEPARATOR_HEIGHT : PH_EMENU_DX_ITEM_HEIGHT;

        // Skip items that are outside the visible scroll region

        if (y + currentHeight >= ScrollOffset && y <= ScrollOffset + Height)
        {
            D2D1_RECT_F itemRect = D2D1::RectF(PH_EMENU_DX_HOVER_MARGIN, y, Width - PH_EMENU_DX_HOVER_MARGIN, y + currentHeight);

            // Draw hover highlight if applicable

            if (item == HoverItem && !FlagOn(item->Flags, PH_EMENU_DISABLED))
            {
                D2D1_ROUNDED_RECT roundedHover = D2D1::RoundedRect(itemRect, PH_EMENU_DX_HOVER_CORNER, PH_EMENU_DX_HOVER_CORNER);
                D2DDeviceContext->FillRoundedRectangle(&roundedHover, HoverBrush);
            }

            // Draw separator or item content

            if (FlagOn(item->Flags, PH_EMENU_SEPARATOR))
            {
                D2DDeviceContext->DrawLine(D2D1::Point2F(PH_EMENU_DX_PADDING, y + currentHeight / PH_EMENU_DX_HALF_FACTOR), D2D1::Point2F(Width - PH_EMENU_DX_PADDING, y + currentHeight / PH_EMENU_DX_HALF_FACTOR), SeparatorBrush, PH_EMENU_DX_BORDER_WIDTH);
            }
            else
            {
                // Handle icon/bitmap creation and rendering

                if (i < LayoutCount && ItemBitmaps && !ItemBitmaps[i] && item->Bitmap)
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

                if (i < LayoutCount && ItemBitmaps && ItemBitmaps[i])
                {
                    D2D1_RECT_F iconRect = D2D1::RectF(PH_EMENU_DX_PADDING + (IconWidth - PH_EMENU_DX_ICON_SIZE) / PH_EMENU_DX_HALF_FACTOR, y + (currentHeight - PH_EMENU_DX_ICON_SIZE) / PH_EMENU_DX_HALF_FACTOR, PH_EMENU_DX_PADDING + (IconWidth + PH_EMENU_DX_ICON_SIZE) / PH_EMENU_DX_HALF_FACTOR, y + (currentHeight + PH_EMENU_DX_ICON_SIZE) / PH_EMENU_DX_HALF_FACTOR);

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
                        D2D1_ELLIPSE dot = D2D1::Ellipse(D2D1::Point2F(PH_EMENU_DX_PADDING + IconWidth / PH_EMENU_DX_HALF_FACTOR, y + currentHeight / PH_EMENU_DX_HALF_FACTOR), PH_EMENU_DX_RADIO_DOT_RADIUS, PH_EMENU_DX_RADIO_DOT_RADIUS);

                        D2DDeviceContext->FillEllipse(&dot, checkBrush);
                    }
                    else
                    {
                        D2D1_POINT_2F p1 = D2D1::Point2F(PH_EMENU_DX_PADDING + PH_EMENU_DX_ARROW_SIZE, y + currentHeight / PH_EMENU_DX_HALF_FACTOR);
                        D2D1_POINT_2F p2 = D2D1::Point2F(PH_EMENU_DX_PADDING + PH_EMENU_DX_ARROW_SIZE * PH_EMENU_DX_HALF_FACTOR, y + currentHeight / PH_EMENU_DX_HALF_FACTOR + PH_EMENU_DX_ARROW_SIZE);
                        D2D1_POINT_2F p3 = D2D1::Point2F(PH_EMENU_DX_PADDING + PH_EMENU_DX_ARROW_SIZE * 4.0f, y + currentHeight / PH_EMENU_DX_HALF_FACTOR - PH_EMENU_DX_ARROW_SIZE);
                        D2DDeviceContext->DrawLine(p1, p2, checkBrush, PH_EMENU_DX_CHECK_LINE_WIDTH);
                        D2DDeviceContext->DrawLine(p2, p3, checkBrush, PH_EMENU_DX_CHECK_LINE_WIDTH);
                    }
                }

                // Render item text and shortcut layouts
                if (item->Text)
                {
                    if (i < LayoutCount && TextLayouts && TextLayouts[i])
                    {
                        D2D1_POINT_2F origin = D2D1::Point2F(PH_EMENU_DX_PADDING + IconWidth, y + (currentHeight - TextMetrics[i].height) / PH_EMENU_DX_HALF_FACTOR);

                        D2DDeviceContext->DrawTextLayout(
                            origin,
                            TextLayouts[i],
                            FlagOn(item->Flags, PH_EMENU_DISABLED) ? ShortcutBrush : TextBrush
                            );
                    }

                    if (i < LayoutCount && ShortcutLayouts && ShortcutLayouts[i])
                    {
                        D2D1_POINT_2F origin = D2D1::Point2F(Width - PH_EMENU_DX_PADDING - MaxShortcutWidth, y + (currentHeight - ShortcutMetrics[i].height) / PH_EMENU_DX_HALF_FACTOR);

                        D2DDeviceContext->DrawTextLayout(
                            origin,
                            ShortcutLayouts[i],
                            ShortcutBrush
                            );
                    }
                }

                // Draw submenu indicator arrow
                if (item->Items && item->Items->Count > 0 && !FlagOn(item->Flags, PH_EMENU_DISABLED))
                {
                    ID2D1SolidColorBrush* arrowBrush = TextBrush;
                    D2D1_POINT_2F p1 = D2D1::Point2F(Width - PH_EMENU_DX_ARROW_RIGHT_MARGIN, y + currentHeight / PH_EMENU_DX_HALF_FACTOR - PH_EMENU_DX_ARROW_SIZE);
                    D2D1_POINT_2F p2 = D2D1::Point2F(Width - PH_EMENU_DX_ARROW_TIP_MARGIN, y + currentHeight / PH_EMENU_DX_HALF_FACTOR);
                    D2D1_POINT_2F p3 = D2D1::Point2F(Width - PH_EMENU_DX_ARROW_RIGHT_MARGIN, y + currentHeight / PH_EMENU_DX_HALF_FACTOR + PH_EMENU_DX_ARROW_SIZE);
                    D2DDeviceContext->DrawLine(p1, p2, arrowBrush, PH_EMENU_DX_ARROW_LINE_WIDTH);
                    D2DDeviceContext->DrawLine(p2, p3, arrowBrush, PH_EMENU_DX_ARROW_LINE_WIDTH);
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
        ID2D1PathGeometry* pathGeometry = NULL;
        ID2D1GeometrySink* geometrySink = NULL;

        if (ScrollOffset > 0)
        {
            if (HoverUpArrow)
            {
                D2D1_RECT_F arrowRect = D2D1::RectF(0, 0, Width, PH_EMENU_DX_ITEM_HEIGHT);
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
                D2D1_RECT_F arrowRect = D2D1::RectF(0, Height - PH_EMENU_DX_ITEM_HEIGHT, Width, Height);
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
    D2D1_RECT_F borderRect = D2D1::RectF(
        PH_EMENU_DX_BORDER_OFFSET,
        PH_EMENU_DX_BORDER_OFFSET,
        Width - PH_EMENU_DX_BORDER_OFFSET,
        Height - PH_EMENU_DX_BORDER_OFFSET
        );

    if (noRound)
    {
        D2DDeviceContext->DrawRectangle(&borderRect, BorderBrush, PH_EMENU_DX_BORDER_WIDTH);
    }
    else
    {
        D2D1_ROUNDED_RECT roundedBorder = D2D1::RoundedRect(borderRect, PH_EMENU_DX_ROUNDED_CORNER, PH_EMENU_DX_ROUNDED_CORNER);
        D2DDeviceContext->DrawRoundedRectangle(&roundedBorder, BorderBrush, PH_EMENU_DX_BORDER_WIDTH);
    }
}

VOID PH_EMENU_DX::CalculateLayout(VOID)
{
    // Initialize base dimensions
    ItemHeight = PH_EMENU_DX_ITEM_HEIGHT;
    IconWidth = PH_EMENU_DX_ICON_COLUMN_WIDTH;
    MaxTextWidth = 0;
    MaxShortcutWidth = 0;

    // Clear existing layouts before recalculating
    ReleaseCachedLayouts();

    if (!Menu || !Menu->Items || Menu->Items->Count == 0)
    {
        Width = PH_EMENU_DX_MIN_WIDTH;
        Height = PH_EMENU_DX_ITEM_HEIGHT;
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
    for (ULONG i = 0; i < count; i++)
    {
        PPH_EMENU_ITEM item = static_cast<PPH_EMENU_ITEM>(Menu->Items->Items[i]);

        if (FlagOn(item->Flags, PH_EMENU_SEPARATOR))
            continue;

        if (item->Text)
        {
            IDWriteTextLayout* layout = NULL;
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

                    if (textMetrics[i].width > MaxTextWidth)
                    {
                        MaxTextWidth = textMetrics[i].width;
                    }
                }
                else
                {
                    layout->Release();
                }

                layout = NULL;
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

                        if (shortcutMetrics[i].width > MaxShortcutWidth)
                        {
                            MaxShortcutWidth = shortcutMetrics[i].width;
                        }
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
        PH_EMENU_DX_PADDING +
        IconWidth +
        MaxTextWidth +
        PH_EMENU_DX_TEXT_SHORTCUT_GAP +
        MaxShortcutWidth +
        PH_EMENU_DX_PADDING
        );

    if (Width < PH_EMENU_DX_MIN_WIDTH)
        Width = PH_EMENU_DX_MIN_WIDTH;

    // Calculate total height by summing up all items and separators
    Height = PH_EMENU_DX_MENU_VERTICAL_PADDING * PH_EMENU_DX_HALF_FACTOR;

    for (ULONG i = 0; i < count; i++)
    {
        PPH_EMENU_ITEM item = static_cast<PPH_EMENU_ITEM>(Menu->Items->Items[i]);

        if (FlagOn(item->Flags, PH_EMENU_SEPARATOR))
            Height += PH_EMENU_DX_SEPARATOR_HEIGHT;
        else
            Height += PH_EMENU_DX_ITEM_HEIGHT;
    }

    Height = ceilf(Height);

    ScrollViewTotalHeight = Height;

    // Cap menu height if it exceeds the visible screen area
    if (ScrollViewTotalHeight > ScrollViewVisibleHeight)
    {
        Height = ScrollViewVisibleHeight;
    }

    if (Height < PH_EMENU_DX_MIN_HEIGHT)
    {
        Height = PH_EMENU_DX_MIN_HEIGHT;
    }

    HoverItem = NULL;
    HoverIndex = -1;
}

VOID PH_EMENU_DX::CalculatePosition(
    _In_ LONG X,
    _In_ LONG Y,
    _Out_ PLONG OutX,
    _Out_ PLONG OutY
    )
{
    FLOAT surfacePadding = 0.0f;
#ifdef PH_EMENU_DX_ENABLE_DROPSHADOW
    surfacePadding = PH_EMENU_DX_SHADOW_PADDING;
#endif

    LONG menuWidth = (LONG)(Width * Scale);
    LONG menuHeight = (LONG)(Height * Scale);
    LONG windowWidth = (LONG)((Width + surfacePadding * PH_EMENU_DX_HALF_FACTOR) * Scale);
    LONG windowHeight = (LONG)((Height + surfacePadding * PH_EMENU_DX_HALF_FACTOR) * Scale);
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
        if (baseX + menuWidth > workArea.Left + workArea.Width)
        {
            if (FlagOn(Align, PH_ALIGN_LEFT))
            {
                if (IsSubMenu && ParentContext)
                {
                    // Flip to the left side of the parent menu
                    baseX = X - (LONG)(ParentContext->Width * Scale) - menuWidth + (LONG)(PH_EMENU_DX_SUBMENU_X_OFFSET * Scale * 2.0f);
                }
                else
                {
                    baseX = X - menuWidth;
                }
            }
        }

        if (IsSubMenu)
        {
            if (baseY + menuHeight > workArea.Top + workArea.Height)
            {
                LONG itemHeight = (LONG)(PH_EMENU_DX_ITEM_HEIGHT * Scale);
                LONG offset_y = (LONG)(PH_EMENU_DX_SUBMENU_Y_OFFSET * Scale);
                LONG flippedY = baseY + itemHeight + (offset_y * 2) - menuHeight;

                if (flippedY >= workArea.Top)
                {
                    baseY = flippedY;
                }
                else
                {
                    baseY = workArea.Top + workArea.Height - menuHeight;
                }

                if (baseY < workArea.Top)
                    baseY = workArea.Top;
            }
            else if (baseY < workArea.Top)
            {
                baseY = workArea.Top;
            }
        }
        else
        {
            if (baseY + menuHeight > workArea.Top + workArea.Height)
            {
                if (FlagOn(Align, PH_ALIGN_TOP))
                    baseY = Y - menuHeight;
            }

            if (baseY < workArea.Top)
            {
                if (FlagOn(Align, PH_ALIGN_BOTTOM))
                    baseY = Y;
            }
        }

        if (baseX < workArea.Left)
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

        if (rect.Height < menuHeight)
            Height = (FLOAT)rect.Height / Scale;
        if (rect.Width < menuWidth)
            Width = (FLOAT)rect.Width / Scale;
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

    *OutX = baseX - (LONG)(surfacePadding * Scale);
    *OutY = baseY - (LONG)(surfacePadding * Scale);
}

VOID PH_EMENU_DX::ReleaseCachedLayouts(VOID)
{
    // Use the allocation count, not the live item count, in case the menu changed.
    ULONG count = LayoutCount;

    LayoutCount = 0;

    if (TextLayouts)
    {
        for (ULONG i = 0; i < count; i++)
        {
            if (TextLayouts[i])
            {
                TextLayouts[i]->Release();
                TextLayouts[i] = NULL;
            }
        }

        PhFree(TextLayouts);
        TextLayouts = NULL;
    }

    if (ShortcutLayouts)
    {
        for (ULONG i = 0; i < count; i++)
        {
            if (ShortcutLayouts[i])
            {
                ShortcutLayouts[i]->Release();
                ShortcutLayouts[i] = NULL;
            }
        }

        PhFree(ShortcutLayouts);
        ShortcutLayouts = NULL;
    }

    if (TextMetrics)
    {
        PhFree(TextMetrics);
        TextMetrics = NULL;
    }

    if (ShortcutMetrics)
    {
        PhFree(ShortcutMetrics);
        ShortcutMetrics = NULL;
    }

    if (ItemBitmaps)
    {
        for (ULONG i = 0; i < count; i++)
        {
            if (ItemBitmaps[i])
            {
                ItemBitmaps[i]->Release();
                ItemBitmaps[i] = NULL;
            }
        }

        PhFree(ItemBitmaps);
        ItemBitmaps = NULL;
    }
}

HRESULT PH_EMENU_DX::CreateTextLayout(
    _In_ PWSTR Text,
    _In_ SIZE_T TextLen,
    _In_ IDWriteTextFormat* Format,
    _In_ FLOAT MaxWidth,
    _In_ FLOAT MaxHeight,
    _Out_ IDWriteTextLayout** Layout
    )
{
    HRESULT status;
    PH_STRING_BUILDER stringBuilder;
    LONG underlineIndex = LONG_ERROR;
    PWSTR processedText;
    SIZE_T processedLength;

    PhInitializeStringBuilder(&stringBuilder, TextLen);

    for (SIZE_T i = 0; i < TextLen; i++)
    {
        if (Text[i] == L'&')
        {
            if (i + 1 < TextLen && Text[i + 1] == L'&')
            {
                PhAppendCharStringBuilder(&stringBuilder, L'&');
                i++;
            }
            else if (underlineIndex == -1 && i + 1 < TextLen)
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
        if (underlineIndex != LONG_ERROR && ShowKeyboardCues)
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
    )
{
    HRESULT status;
    IWICBitmap* wicBitmap = NULL;

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
    )
{
    HRESULT status;
    IWICBitmap* wicBitmap = NULL;

    // WicFactory is referenced from the shared factory in CreateResources.
    if (!WicFactory)
        return E_FAIL;

    status = WicFactory->CreateBitmapFromHBITMAP(
        BitmapHandle,
        NULL,
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

VOID PH_EMENU_DX::RebuildTextLayouts(VOID)
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

BOOLEAN PH_EMENU_DX::CanScrollUp(VOID)
{
    return ScrollViewTotalHeight > Height && ScrollOffset > 0;
}

BOOLEAN PH_EMENU_DX::CanScrollDown(VOID)
{
    return ScrollViewTotalHeight > Height && ScrollOffset < (ScrollViewTotalHeight - Height);
}

FLOAT PH_EMENU_DX::GetItemTop(
    _In_ ULONG Index
    )
{
    FLOAT itemTop = PH_EMENU_DX_MENU_VERTICAL_PADDING;

    for (ULONG i = 0; i < Index && i < Menu->Items->Count; i++)
    {
        PPH_EMENU_ITEM item = static_cast<PPH_EMENU_ITEM>(Menu->Items->Items[i]);
        itemTop += FlagOn(item->Flags, PH_EMENU_SEPARATOR) ? PH_EMENU_DX_SEPARATOR_HEIGHT : PH_EMENU_DX_ITEM_HEIGHT;
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

    if (ScrollViewTotalHeight <= Height || Index >= Menu->Items->Count)
        return;

    item = static_cast<PPH_EMENU_ITEM>(Menu->Items->Items[Index]);
    maxOffset = ScrollViewTotalHeight - Height;
    itemTop = GetItemTop(Index);
    itemBottom = itemTop + (FlagOn(item->Flags, PH_EMENU_SEPARATOR) ? PH_EMENU_DX_SEPARATOR_HEIGHT : PH_EMENU_DX_ITEM_HEIGHT);

    // While visible, each scroll arrow covers one item height at its edge.
    if (itemTop < ScrollOffset + (CanScrollUp() ? PH_EMENU_DX_ITEM_HEIGHT : 0.0f))
        ScrollOffset = itemTop - PH_EMENU_DX_ITEM_HEIGHT;
    else if (itemBottom > ScrollOffset + Height - (CanScrollDown() ? PH_EMENU_DX_ITEM_HEIGHT : 0.0f))
        ScrollOffset = itemBottom + PH_EMENU_DX_ITEM_HEIGHT - Height;

    if (ScrollOffset < 0)
        ScrollOffset = 0;
    if (ScrollOffset > maxOffset)
        ScrollOffset = maxOffset;
}

VOID PH_EMENU_DX::SetHoverIndex(
    _In_ ULONG Index
    )
{
    PPH_EMENU_DX subContext = SubMenuContext;

    // Any open submenu belongs to the previous hover item.
    while (subContext)
    {
        subContext->ExitLoop = TRUE;
        PostMessage(subContext->WindowHandle, WM_NULL, 0, 0);
        subContext = subContext->SubMenuContext;
    }

    HoverIndex = static_cast<LONG>(Index);
    HoverItem = static_cast<PPH_EMENU_ITEM>(Menu->Items->Items[Index]);

    EnsureItemVisible(Index);
    CommitUpdate();
}

VOID PH_EMENU_DX::ActivateHoverItem(VOID)
{
    if (!HoverItem || FlagOn(HoverItem->Flags, PH_EMENU_DISABLED | PH_EMENU_SEPARATOR))
        return;

    if (HoverItem->Items && HoverItem->Items->Count > 0)
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

HRESULT PH_EMENU_DX::OpenSubMenu(VOID)
{
    if (SubMenuContext)
    {
        if (SubMenuContext->ExitLoop)
            PhSetTimer(WindowHandle, PH_WINDOW_TIMER_DEFAULT, PH_EMENU_DX_RETRY_TIMER_INTERVAL, NULL);
        return S_OK;
    }

    if (HoverItem && HoverItem->Items && HoverItem->Items->Count > 0 && !FlagOn(HoverItem->Flags, PH_EMENU_DISABLED))
    {
        PPH_EMENU_ITEM selected;
        RECT windowRect;
        FLOAT itemY = PH_EMENU_DX_SUBMENU_Y_OFFSET;

        if (!PhGetWindowRect(WindowHandle, &windowRect))
            return E_FAIL;

        for (LONG i = 0; i < HoverIndex; i++)
        {
            PPH_EMENU_ITEM item = static_cast<PPH_EMENU_ITEM>(Menu->Items->Items[i]);
            itemY += FlagOn(item->Flags, PH_EMENU_SEPARATOR) ? PH_EMENU_DX_SEPARATOR_HEIGHT : PH_EMENU_DX_ITEM_HEIGHT;
        }

        FLOAT surfacePadding = 0.0f;
#ifdef PH_EMENU_DX_ENABLE_DROPSHADOW
        surfacePadding = PH_EMENU_DX_SHADOW_PADDING;
#endif

        if (PH_EMENU_DX::ShowInternal(HoverItem, WindowHandle, Flags, PH_ALIGN_LEFT | PH_ALIGN_TOP, windowRect.left + (LONG)((surfacePadding + Width) * Scale) - (LONG)(PH_EMENU_DX_SUBMENU_X_OFFSET * Scale), windowRect.top + (LONG)((surfacePadding + itemY - ScrollOffset) * Scale) - (LONG)(PH_EMENU_DX_SUBMENU_Y_OFFSET * Scale), !!IsDarkMode, this, &selected))
        {
            if (selected)
            {
                SelectedItem = selected;
                ExitLoop = TRUE;
                PostMessage(WindowHandle, WM_NULL, 0, 0);
            }
        }
    }

    return S_OK;
}

VOID PH_EMENU_DX::DestroySubMenuChain(VOID)
{
    if (SubMenuContext)
    {
        PPH_EMENU_DX subContext = SubMenuContext;

        subContext->DestroySubMenuChain();
        subContext->ExitLoop = TRUE;

        if (IsWindow(subContext->WindowHandle))
            DestroyWindow(subContext->WindowHandle);

        SubMenuContext = NULL;
    }
}

VOID PH_EMENU_DX::HandleMouseMove(
    _In_ HWND WindowHandle,
    _In_ POINT pt
    )
{
    FLOAT surfacePadding = 0.0f;
#ifdef PH_EMENU_DX_ENABLE_DROPSHADOW
    surfacePadding = PH_EMENU_DX_SHADOW_PADDING;
#endif

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
    if (CanScrollUp() && pt_y < PH_EMENU_DX_ITEM_HEIGHT)
    {
        HoverUpArrow = TRUE;
        PhSetTimer(WindowHandle, PH_WINDOW_TIMER_SCROLL, PH_EMENU_DX_SCROLL_TIMER_INTERVAL, NULL);
    }
    else if (CanScrollDown() && pt_y > Height - PH_EMENU_DX_ITEM_HEIGHT)
    {
        HoverDownArrow = TRUE;
        PhSetTimer(WindowHandle, PH_WINDOW_TIMER_SCROLL, PH_EMENU_DX_SCROLL_TIMER_INTERVAL, NULL);
    }

    if (oldHoverUp != HoverUpArrow || oldHoverDown != HoverDownArrow)
        CommitUpdate();

    HoverItem = NULL;
    HoverIndex = -1;

    if (!HoverUpArrow && !HoverDownArrow &&
        pt_x >= 0 && pt_x < Width &&
        pt_y >= PH_EMENU_DX_MENU_VERTICAL_PADDING &&
        pt_y < Height - PH_EMENU_DX_MENU_VERTICAL_PADDING)
    {
        FLOAT currentY = PH_EMENU_DX_MENU_VERTICAL_PADDING;

        for (ULONG i = 0; i < Menu->Items->Count; i++)
        {
            PPH_EMENU_ITEM item = static_cast<PPH_EMENU_ITEM>(Menu->Items->Items[i]);
            FLOAT h = FlagOn(item->Flags, PH_EMENU_SEPARATOR) ? PH_EMENU_DX_SEPARATOR_HEIGHT : PH_EMENU_DX_ITEM_HEIGHT;

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
        if (SubMenuContext)
        {
            PPH_EMENU_DX subContext = SubMenuContext;

            while (subContext)
            {
                subContext->ExitLoop = TRUE;
                PostMessage(subContext->WindowHandle, WM_NULL, 0, 0);
                subContext = subContext->SubMenuContext;
            }
        }

        CommitUpdate();

        if (HoverItem &&
            HoverItem->Items &&
            HoverItem->Items->Count > 0 &&
            !FlagOn(HoverItem->Flags, PH_EMENU_DISABLED))
        {
            PhSetTimer(WindowHandle, PH_WINDOW_TIMER_DEFAULT, MenuShowDelay > 0 ? MenuShowDelay : 1, NULL);
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
    if (WindowsVersion >= WINDOWS_11)
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
        PhSetCursor(PhLoadCursor(NULL, IDC_ARROW));
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
        FLOAT steps = (FLOAT)WheelDelta / (FLOAT)WHEEL_DELTA;
        FLOAT scrollAmount = steps * ItemHeight * PH_EMENU_DX_SCROLL_STEP;
        POINT pt;

        ScrollOffset -= scrollAmount;

        if (ScrollOffset < 0)
            ScrollOffset = 0;
        if (ScrollOffset > (ScrollViewTotalHeight - Height))
            ScrollOffset = (ScrollViewTotalHeight - Height);

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
                return;
        }

        PPH_EMENU_DX subContext = SubMenuContext;
        while (subContext)
        {
            subContext->ExitLoop = TRUE;
            PostMessage(subContext->WindowHandle, WM_NULL, 0, 0);
            subContext = subContext->SubMenuContext;
        }
    }

    HoverItem = NULL;
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
        OpenSubMenu();
    }
    else if (TimerId == PH_WINDOW_TIMER_SCROLL)
    {
        POINT pt;
        FLOAT surfacePadding = 0.0f;
#ifdef PH_EMENU_DX_ENABLE_DROPSHADOW
        surfacePadding = PH_EMENU_DX_SHADOW_PADDING;
#endif

        if (!PhGetClientPos(WindowHandle, &pt))
            return;

        FLOAT pt_y = (FLOAT)pt.y / Scale - surfacePadding;
        FLOAT scrollAmount = ItemHeight / PH_EMENU_DX_SCROLL_SPEED_DIVISOR;

        if (pt_y < PH_EMENU_DX_ITEM_HEIGHT && ScrollOffset > 0)
        {
            ScrollOffset -= scrollAmount;
            if (ScrollOffset < 0)
                ScrollOffset = 0;
            CommitUpdate();
            HandleMouseMove(WindowHandle, pt);
        }
        else if (pt_y > Height - PH_EMENU_DX_ITEM_HEIGHT && ScrollOffset < (ScrollViewTotalHeight - Height))
        {
            ScrollOffset += scrollAmount;
            if (ScrollOffset > (ScrollViewTotalHeight - Height))
                ScrollOffset = (ScrollViewTotalHeight - Height);
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
    FLOAT surfacePadding = 0.0f;
#ifdef PH_EMENU_DX_ENABLE_DROPSHADOW
    surfacePadding = PH_EMENU_DX_SHADOW_PADDING;
#endif

    FLOAT pt_x = (FLOAT)CursorX / Scale - surfacePadding;
    FLOAT pt_y = (FLOAT)CursorY / Scale - surfacePadding;

    FLOAT adjusted_pt_y = pt_y + ScrollOffset;

    // Ignore clicks on a visible scroll arrow.
    if ((CanScrollUp() && pt_y < PH_EMENU_DX_ITEM_HEIGHT) ||
        (CanScrollDown() && pt_y > Height - PH_EMENU_DX_ITEM_HEIGHT))
    {
        return;
    }

    if (pt_x >= 0 && pt_x < Width && pt_y >= PH_EMENU_DX_MENU_VERTICAL_PADDING && pt_y < Height - PH_EMENU_DX_MENU_VERTICAL_PADDING)
    {
        FLOAT currentY = PH_EMENU_DX_MENU_VERTICAL_PADDING;

        for (ULONG i = 0; i < Menu->Items->Count; i++)
        {
            PPH_EMENU_ITEM item = static_cast<PPH_EMENU_ITEM>(Menu->Items->Items[i]);
            FLOAT h = FlagOn(item->Flags, PH_EMENU_SEPARATOR) ? PH_EMENU_DX_SEPARATOR_HEIGHT : PH_EMENU_DX_ITEM_HEIGHT;

            if (adjusted_pt_y >= currentY && adjusted_pt_y < currentY + h)
            {
                if (!FlagOn(item->Flags, PH_EMENU_DISABLED | PH_EMENU_SEPARATOR))
                {
                    if (item->Items && item->Items->Count > 0)
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

    if (Menu->Items->Count == 0)
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
                HoverItem->Items->Count > 0 &&
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

            if (next < 0)
                next = static_cast<LONG>(Menu->Items->Count) - 1;

            LONG start = next;

            while (FlagOn((static_cast<PPH_EMENU_ITEM>(Menu->Items->Items[next]))->Flags, PH_EMENU_SEPARATOR | PH_EMENU_DISABLED))
            {
                next--;
                if (next < 0)
                    next = static_cast<LONG>(Menu->Items->Count) - 1;
                if (next == start)
                    break;
            }

            SetHoverIndex(static_cast<ULONG>(next));
        }
        break;
    case VK_DOWN:
        {
            LONG next = HoverIndex + 1;

            if (next >= (LONG)Menu->Items->Count)
                next = 0;

            LONG start = next;

            while (FlagOn((static_cast<PPH_EMENU_ITEM>(Menu->Items->Items[next]))->Flags, PH_EMENU_SEPARATOR | PH_EMENU_DISABLED))
            {
                next++;
                if (next >= (LONG)Menu->Items->Count)
                    next = 0;
                if (next == start)
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

    if (count == 0 || Character < L' ')
        return FALSE;

    key = PhUpcaseUnicodeChar(Character);

    // Mnemonic selection: a unique match activates the item; several matches
    // cycle the hover through them, starting after the current item.
    for (ULONG i = 0; i < count; i++)
    {
        PPH_EMENU_ITEM item = static_cast<PPH_EMENU_ITEM>(Menu->Items->Items[i]);

        if (FlagOn(item->Flags, PH_EMENU_SEPARATOR | PH_EMENU_DISABLED))
            continue;
        if (GetItemMnemonic(item) != key)
            continue;

        matches++;

        if (first == -1)
            first = static_cast<LONG>(i);
        if (next == -1 && static_cast<LONG>(i) > HoverIndex)
            next = static_cast<LONG>(i);
    }

    if (matches == 0)
        return FALSE;

    if (next == -1)
        next = first;

    PhKillTimer(WindowHandle, PH_WINDOW_TIMER_DEFAULT);
    SetHoverIndex(static_cast<ULONG>(next));

    if (matches == 1)
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
        HWND popup = otherWnd ? GetLastActivePopup(otherWnd) : NULL;

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
        HWND windowHandle = WindowFromPoint(hs->pt);

        if (PH_EMENU_DX::IsWindowInMenuChain(windowHandle))
            return CallNextHookEx(NULL, nCode, wParam, lParam);

        HWND* windows = NULL;
        ULONG count = 0;
        ULONG currentThreadId = HandleToUlong(NtCurrentThreadId());

        // Low-level hooks run on the installing thread. Cancel only that thread's
        // menus; a cross-thread SendMessage here could stall system-wide mouse input.
        PhAcquireQueuedLockShared(&MenuListLock);
        if (MenuList && MenuList->Count > 0)
        {
            windows = static_cast<HWND*>(PhAllocate(MenuList->Count * sizeof(HWND)));

            for (ULONG i = 0; i < MenuList->Count; i++)
            {
                PPH_EMENU_DX menuContext = static_cast<PPH_EMENU_DX>(MenuList->Items[i]);

                if (menuContext->ThreadId == currentThreadId && menuContext->WindowHandle)
                    windows[count++] = menuContext->WindowHandle;
            }
        }
        PhReleaseQueuedLockShared(&MenuListLock);

        if (windows)
        {
            for (ULONG i = count; i > 0; i--)
                SendMessage(windows[i - 1], WM_CANCELMODE, 0, 0);
            PhFree(windows);
        }
    }

    return CallNextHookEx(NULL, nCode, wParam, lParam);
}

BOOLEAN PH_EMENU_DX::IsMenuSupported(
    _In_ PPH_EMENU Menu
    )
{
    const ULONG unsupportedFlags = PH_EMENU_MENUBARBREAK | PH_EMENU_MENUBREAK | PH_EMENU_RIGHTORDER | PH_EMENU_MAINMENU;

    if (FlagOn(Menu->Flags, PH_EMENU_MAINMENU))
        return FALSE;
    if (!Menu->Items || Menu->Items->Count == 0)
        return FALSE;

    for (ULONG i = 0; i < Menu->Items->Count; i++)
    {
        PPH_EMENU_ITEM item = static_cast<PPH_EMENU_ITEM>(Menu->Items->Items[i]);

        if (FlagOn(item->Flags, unsupportedFlags))
            return FALSE;

        // Empty submenus are drawn as plain items, so only recurse into populated ones.
        if (item->Items && item->Items->Count != 0 && !IsMenuSupported(item))
            return FALSE;
    }

    return TRUE;
}

VOID PH_EMENU_DX::UnregisterWindowClass(VOID)
{
    PH_EMENU_DX::ResetGlobalResources();

    PhAcquireQueuedLockExclusive(&MenuListLock);
    if (MenuList)
    {
        PhDereferenceObject(MenuList);
        MenuList = NULL;
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
    static decltype(&DCompositionCreateDevice2) createDevice = NULL;
    static PVOID compositionLibrary = NULL;

    if (!createDevice)
    {
        if (!compositionLibrary)
            compositionLibrary = PhLoadLibrary(L"dcomp.dll");
        if (!compositionLibrary)
            return HRESULT_FROM_WIN32(ERROR_MOD_NOT_FOUND);

        createDevice = reinterpret_cast<decltype(&DCompositionCreateDevice2)>(
            PhGetProcedureAddress(compositionLibrary, "DCompositionCreateDevice2", 0)
            );
        if (!createDevice)
            return HRESULT_FROM_WIN32(ERROR_PROC_NOT_FOUND);
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
    static PVOID procedure = NULL;
    D3D11_CREATE_DEVICE_FN createDevice;

    *Device = NULL;
    if (ImmediateContext)
        *ImmediateContext = NULL;

    createDevice = reinterpret_cast<D3D11_CREATE_DEVICE_FN>(GetDynamicProcedure(&procedure, L"d3d11.dll", "D3D11CreateDevice"));

    if (!createDevice)
        return HRESULT_FROM_WIN32(ERROR_PROC_NOT_FOUND);

    return createDevice(Adapter, DriverType, NULL, Flags, NULL, 0, D3D11_SDK_VERSION, Device, FeatureLevel, ImmediateContext);
}

HRESULT PH_EMENU_DX::DynamicD3D10CreateDevice1(
    _In_ D3D10_DRIVER_TYPE DriverType,
    _In_ UINT Flags,
    _In_ D3D10_FEATURE_LEVEL1 HardwareLevel,
    _Out_ ID3D10Device1** Device
    )
{
    using D3D10_CREATE_DEVICE1_FN = HRESULT (WINAPI*)(IDXGIAdapter*, D3D10_DRIVER_TYPE, HMODULE, UINT, D3D10_FEATURE_LEVEL1, UINT, ID3D10Device1**);
    static PVOID procedure = NULL;
    D3D10_CREATE_DEVICE1_FN createDevice;

    *Device = NULL;

    createDevice = reinterpret_cast<D3D10_CREATE_DEVICE1_FN>(GetDynamicProcedure(&procedure, L"d3d10_1.dll", "D3D10CreateDevice1"));

    if (!createDevice)
        return HRESULT_FROM_WIN32(ERROR_PROC_NOT_FOUND);

    return createDevice(NULL, DriverType, NULL, Flags, HardwareLevel, D3D10_1_SDK_VERSION, Device);
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
    static PVOID procedure = NULL;
    D2D1_CREATE_FACTORY_FN createFactory;

    *Factory = NULL;

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
    static PVOID procedure = NULL;
    DWRITE_CREATE_FACTORY_FN createFactory;

    *Factory = NULL;

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
    static PVOID procedure = NULL;
    DWM_EXTEND_FRAME_FN extendFrame;

    extendFrame = reinterpret_cast<DWM_EXTEND_FRAME_FN>(GetDynamicProcedure(&procedure, L"dwmapi.dll", "DwmExtendFrameIntoClientArea"));

    if (!extendFrame)
        return HRESULT_FROM_WIN32(ERROR_PROC_NOT_FOUND);

    return extendFrame(WindowHandle, Margins);
}

VOID PH_EMENU_DX::ResetGlobalResources(VOID)
{
    PhAcquireQueuedLockExclusive(&SharedLock);
    PH_EMENU_DX::ReleaseSharedResources();
    PhReleaseQueuedLockExclusive(&SharedLock);
}

// Caller must hold SharedLock exclusively. Menus keep their own references,
// so objects in use by open menus stay alive until those menus release them.
VOID PH_EMENU_DX::ReleaseSharedResources(VOID)
{
#ifdef PH_EMENU_DX_ENABLE_D3D12
    if (SharedD3D11On12Device)
    {
        SharedD3D11On12Device->Release();
        SharedD3D11On12Device = NULL;
    }
    if (SharedD3D11Device12)
    {
        SharedD3D11Device12->Release();
        SharedD3D11Device12 = NULL;
    }
    if (SharedD3D12CommandQueue)
    {
        SharedD3D12CommandQueue->Release();
        SharedD3D12CommandQueue = NULL;
    }
    if (SharedD3D12Device)
    {
        SharedD3D12Device->Release();
        SharedD3D12Device = NULL;
    }
#endif
#ifdef PH_EMENU_DX_ENABLE_D3D11
    if (SharedD3D11Device)
    {
        SharedD3D11Device->Release();
        SharedD3D11Device = NULL;
    }
#endif
    if (SharedD3DDevice)
    {
        SharedD3DDevice->Release();
        SharedD3DDevice = NULL;
    }
    if (SharedD2DDevice)
    {
        SharedD2DDevice->Release();
        SharedD2DDevice = NULL;
    }
    if (SharedD2DFactory)
    {
        SharedD2DFactory->Release();
        SharedD2DFactory = NULL;
    }
#ifdef PH_EMENU_DX_ENABLE_WINRT_COMPOSITION
    if (SharedCompositionGraphicsDevice)
    {
        SharedCompositionGraphicsDevice->Release();
        SharedCompositionGraphicsDevice = NULL;
    }
    if (SharedCompositor)
    {
        SharedCompositor->Release();
        SharedCompositor = NULL;
    }
#else
    if (SharedCompositionDevice3)
    {
        SharedCompositionDevice3->Release();
        SharedCompositionDevice3 = NULL;
    }
    if (SharedDesktopDevice)
    {
        SharedDesktopDevice->Release();
        SharedDesktopDevice = NULL;
    }
    if (SharedDCompDevice)
    {
        SharedDCompDevice->Release();
        SharedDCompDevice = NULL;
    }
#endif
    if (SharedWicFactory)
    {
        SharedWicFactory->Release();
        SharedWicFactory = NULL;
    }
    if (SharedDWriteFactory)
    {
        SharedDWriteFactory->Release();
        SharedDWriteFactory = NULL;
    }

    SharedGeneration++;
}

HRESULT PH_EMENU_DX::CreateSharedD3DDevice(VOID)
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
            status = D3D12CreateDevice(NULL, D3D_FEATURE_LEVEL_11_0, IID_PPV_ARGS(&SharedD3D12Device));

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
                    NULL,
                    0,
                    commandQueues,
                    _countof(commandQueues),
                    0,
                    &SharedD3D11Device12,
                    NULL,
                    NULL
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
            ID3D11DeviceContext* d3dContext = NULL;

            status = DynamicD3D11CreateDevice(
                NULL,
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

HRESULT PH_EMENU_DX::CreateSharedD2DDevice(VOID)
{
    HRESULT status = S_OK;
    IDXGIDevice* dxgiDevice = NULL;

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

HRESULT PH_EMENU_DX::CreateSharedDCompDevice(VOID)
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

HRESULT PH_EMENU_DX::CreateSharedTextAndImagingFactories(VOID)
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
HRESULT PH_EMENU_DX::InitializeSharedResources(VOID)
{
    HRESULT status;

#ifdef PH_EMENU_DX_ENABLE_WINRT_COMPOSITION
    {
        PVOID controller = NULL;
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

VOID PH_EMENU_DX::RegisterMenuContext(VOID)
{
    ThreadId = HandleToUlong(NtCurrentThreadId());

    PhAcquireQueuedLockExclusive(&MenuListLock);
    PhAddItemList(MenuList, this);
    PhReleaseQueuedLockExclusive(&MenuListLock);
}

VOID PH_EMENU_DX::UnregisterMenuContext(VOID)
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

    for (ULONG i = 0; i < MenuList->Count; i++)
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

RTL_ATOM PH_EMENU_DX::PhRegisterMenuClass(VOID)
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
        wcex.hCursor = LoadCursor(NULL, IDC_ARROW);
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

VOID PhEMenuDxUnregisterClass(VOID)
{
    PH_EMENU_DX::UnregisterWindowClass();
}
