/*
 * Copyright (c) 2022 Winsider Seminars & Solutions, Inc.  All rights reserved.
 *
 * This file is part of System Informer.
 *
 * Authors:
 *
 *     wj32    2010-2016
 *     dmex    2016-2023
 *
 */

#ifndef _PH_GUISUPPVIEW_H
#define _PH_GUISUPPVIEW_H

//
// Listview2 (Undocumented)
//

#ifndef LVM_QUERYINTERFACE
#define LVM_QUERYINTERFACE (LVM_FIRST + 189) // IListView, IListView2, IOleWindow, IVisualProperties, IPropertyControlSite, IListViewFooter
#endif

// {E5B16AF2-3990-4681-A609-1F060CD14269}
DEFINE_GUID(IID_IListView, 0xE5B16AF2, 0x3990, 0x4681, 0xA6, 0x09, 0x1F, 0x06, 0x0C, 0xD1, 0x42, 0x69);
// {E6DFF6FD-BCD5-4162-9C65-A3B18C616FDB}
DEFINE_GUID(IID_IDrawPropertyControl, 0xE6DFF6FD, 0xBCD5, 0x4162, 0x9C, 0x65, 0xA3, 0xB1, 0x8C, 0x61, 0x6F, 0xDB);
// {1572DD51-443C-44B0-ACE4-38A005FC697E}
DEFINE_GUID(IID_IDrawPropertyControlWin10, 0x1572DD51, 0x443C, 0x44B0, 0xAC, 0xE4, 0x38, 0xA0, 0x05, 0xFC, 0x69, 0x7E);
// {F0034DA8-8A22-4151-8F16-2EBA76565BCC}
DEFINE_GUID(IID_IListViewFooter, 0xF0034DA8, 0x8A22, 0x4151, 0x8F, 0x16, 0x2E, 0xBA, 0x76, 0x56, 0x5B, 0xCC);
// {88EB9442-913B-4AB4-A741-DD99DCB7558B}
DEFINE_GUID(IID_IListViewFooterCallback, 0x88EB9442, 0x913B, 0x4AB4, 0xA7, 0x41, 0xDD, 0x99, 0xDC, 0xB7, 0x55, 0x8B);
// {44C09D56-8D3B-419D-A462-7B956B105B47}
DEFINE_GUID(IID_IOwnerDataCallback, 0x44C09D56, 0x8D3B, 0x419D, 0xA4, 0x62, 0x7B, 0x95, 0x6B, 0x10, 0x5B, 0x47);
// {5E82A4DD-9561-476A-8634-1BEBACBA4A38}
DEFINE_GUID(IID_IPropertyControl, 0x5E82A4DD, 0x9561, 0x476A, 0x86, 0x34, 0x1B, 0xEB, 0xAC, 0xBA, 0x4A, 0x38);
// {6E71A510-732A-4557-9596-A827E36DAF8F}
DEFINE_GUID(IID_IPropertyControlBase, 0x6E71A510, 0x732A, 0x4557, 0x95, 0x96, 0xA8, 0x27, 0xE3, 0x6D, 0xAF, 0x8F);
// {7AF7F355-1066-4E17-B1F2-19FE2F099CD2}
DEFINE_GUID(IID_IPropertyValue, 0x7AF7F355, 0x1066, 0x4E17, 0xB1, 0xF2, 0x19, 0xFE, 0x2F, 0x09, 0x9C, 0xD2);
// {11A66240-5489-42C2-AEBF-286FC831524C}
DEFINE_GUID(IID_ISubItemCallback, 0x11A66240, 0x5489, 0x42C2, 0xAE, 0xBF, 0x28, 0x6F, 0xC8, 0x31, 0x52, 0x4C);

// CLSID_CAsyncSubItemControlsView: {A1DCCC29-7C70-4821-97AE-67F04105EC91}
DEFINE_GUID(CLSID_CAsyncSubItemControlsView, 0xa1dccc29, 0x7c70, 0x4821, 0x97, 0xae, 0x67, 0xf0, 0x41, 0x5, 0xec, 0x91);
// CLSID_CFooterAreaView: {EBE684BF-3301-4A6D-83CE-E4D6851BE881}
DEFINE_GUID(CLSID_CFooterAreaView, 0xebe684bf, 0x3301, 0x4a6d, 0x83, 0xce, 0xe4, 0xd6, 0x85, 0x1b, 0xe8, 0x81);
// CLSID_CGroupedVirtualModeView: {A08A0F2D-0647-4443-9450-C460F4791046}
DEFINE_GUID(CLSID_CGroupedVirtualModeView, 0xa08a0f2d, 0x647, 0x4443, 0x94, 0x50, 0xc4, 0x60, 0xf4, 0x79, 0x10, 0x46);
// CLSID_CGroupSubsetView: {64A11699-104A-482A-9D85-CCFEA2EE3C94}
DEFINE_GUID(CLSID_CGroupSubsetView, 0x64a11699, 0x104a, 0x482a, 0x9d, 0x85, 0xcc, 0xfe, 0xa2, 0xee, 0x3c, 0x94);
// CLSID_CSubItemControlsView: {13E88673-D30C-46BA-8F2E-97C5CD024E73}
DEFINE_GUID(CLSID_CSubItemControlsView, 0x13e88673, 0xd30c, 0x46ba, 0x8f, 0x2e, 0x97, 0xc5, 0xcd, 0x2, 0x4e, 0x73);

// {1E8F0D70-7399-41BF-8598-7949A2DEC898}
DEFINE_GUID(CLSID_CBooleanControl, 0x1E8F0D70, 0x7399, 0x41BF, 0x85, 0x98, 0x79, 0x49, 0xA2, 0xDE, 0xC8, 0x98);
// {e2183960-9d58-4e9c-878a-4acc06ca564a}
DEFINE_GUID(CLSID_CCustomDrawMultiValuePropertyControl, 0xE2183960, 0x9D58, 0x4E9C, 0x87, 0x8A, 0x4A, 0xCC, 0x06, 0xCA, 0x56, 0x4A);
// {AB517586-73CF-489c-8D8C-5AE0EAD0613A}
DEFINE_GUID(CLSID_CCustomDrawPercentFullControl, 0xAB517586, 0x73CF, 0x489c, 0x8D, 0x8C, 0x5A, 0xE0, 0xEA, 0xD0, 0x61, 0x3A);
// {0d81ea0d-13bf-44b2-af1c-fcdf6be7927c}
DEFINE_GUID(CLSID_CCustomDrawProgressControl, 0x0d81ea0d, 0x13bf, 0x44B2, 0xAF, 0x1C, 0xFC, 0xDF, 0x6B, 0xE7, 0x92, 0x7C);
// {15756be1-a4ad-449c-b576-df3df0e068d3}
DEFINE_GUID(CLSID_CHyperlinkControl, 0x15756BE1, 0xA4AD, 0x449C, 0xB5, 0x76, 0xDF, 0x3D, 0xF0, 0xE0, 0x68, 0xD3);
// {53a01e9d-61cc-4cb0-83b1-31bc8df63156}
DEFINE_GUID(CLSID_CIconListControl, 0x53A01E9D, 0x61CC, 0x4CB0, 0x83, 0xB1, 0x31, 0xBC, 0x8D, 0xF6, 0x31, 0x56);
// {6A205B57-2567-4a2c-B881-F787FAB579A3}
DEFINE_GUID(CLSID_CInPlaceCalendarControl, 0x6A205B57, 0x2567, 0x4A2C, 0xB8, 0x81, 0xF7, 0x87, 0xFA, 0xB5, 0x79, 0xA3);
// {0EEA25CC-4362-4a12-850B-86EE61B0D3EB}
DEFINE_GUID(CLSID_CInPlaceDropListComboControl, 0x0EEA25CC, 0x4362, 0x4A12, 0x85, 0x0B, 0x86, 0xEE, 0x61, 0xB0, 0xD3, 0xEB);
// {A9CF0EAE-901A-4739-A481-E35B73E47F6D}
DEFINE_GUID(CLSID_CInPlaceEditBoxControl, 0xA9CF0EAE, 0x901A, 0x4739, 0xA4, 0x81, 0xE3, 0x5B, 0x73, 0xE4, 0x7F, 0x6D);
// {8EE97210-FD1F-4b19-91DA-67914005F020}
DEFINE_GUID(CLSID_CInPlaceMLEditBoxControl, 0x8EE97210, 0xFD1F, 0x4B19, 0x91, 0xDA, 0x67, 0x91, 0x40, 0x05, 0xF0, 0x20);
// {8e85d0ce-deaf-4ea1-9410-fd1a2105ceb5}
DEFINE_GUID(CLSID_CInPlaceMultiValuePropertyControl, 0x8E85D0CE, 0xDEAF, 0x4EA1, 0x94, 0x10, 0xFD, 0x1A, 0x21, 0x05, 0xCE, 0xB5);
// {85e94d25-0712-47ed-8cde-b0971177c6a1}
DEFINE_GUID(CLSID_CRatingControl, 0x85e94d25, 0x0712, 0x47ed, 0x8C, 0xDE, 0xB0, 0x97, 0x11, 0x77, 0xC6, 0xA1);
// {527c9a9b-b9a2-44b0-84f9-f0dc11c2bcfb}
DEFINE_GUID(CLSID_CStaticPropertyControl, 0x527C9A9B, 0xB9A2, 0x44B0, 0x84, 0xF9, 0xF0, 0xDC, 0x11, 0xC2, 0xBC, 0xFB);

#undef INTERFACE
#define INTERFACE IOwnerDataCallback
/**
 * IOwnerDataCallback
 *
 * Callback interface used by owner-data list views to query and set
 * positions and grouping relations of items, and to receive cache hints.
 */
DECLARE_INTERFACE_(IOwnerDataCallback, IUnknown)
{
    BEGIN_INTERFACE

    //
    // IUnknown
    //

    DECLSPEC_XFGVIRT(IOwnerDataCallback, QueryInterface)
    STDMETHOD(QueryInterface)(THIS_ REFIID riid, void** ppvObject) PURE;

    DECLSPEC_XFGVIRT(IOwnerDataCallback, AddRef)
    STDMETHOD_(ULONG, AddRef)(THIS) PURE;

    DECLSPEC_XFGVIRT(IOwnerDataCallback, Release)
    STDMETHOD_(ULONG, Release)(THIS) PURE;

    //
    // IOwnerDataCallback
    //

    /**
     * Retrieves the current position of an item.
     *
     * \param itemIndex The zero-based index of the item.
     * \param pPosition Receives the item's position in view coordinates.
     * \return HRESULT indicating success or failure.
     */
    DECLSPEC_XFGVIRT(IOwnerDataCallback, GetItemPosition)
    STDMETHOD(GetItemPosition)(THIS_ LONG itemIndex, LPPOINT pPosition) PURE;

    /**
     * Sets the position of an item.
     *
     * \param itemIndex The zero-based index of the item.
     * \param position The desired item position in view coordinates.
     * \return HRESULT indicating success or failure.
     */
    DECLSPEC_XFGVIRT(IOwnerDataCallback, SetItemPosition)
    STDMETHOD(SetItemPosition)(THIS_ LONG itemIndex, POINT position) PURE;

    /**
     * Maps a group-wide index to the control-wide item index.
     *
     * \param groupIndex Zero-based list view group index.
     * \param groupWideItemIndex Zero-based index of the item within the group.
     * \param pTotalItemIndex Receives the control-wide item index.
     * \return HRESULT indicating success or failure.
     */
    DECLSPEC_XFGVIRT(IOwnerDataCallback, GetItemInGroup)
    STDMETHOD(GetItemInGroup)(THIS_ LONG groupIndex, LONG groupWideItemIndex, PLONG pTotalItemIndex) PURE;

    /**
     * Retrieves the group for a specific occurrence of an item.
     *
     * \param itemIndex Control-wide zero-based item index.
     * \param occurenceIndex Zero-based occurrence index of the item.
     * \param pGroupIndex Receives the group's zero-based index.
     * \return HRESULT indicating success or failure.
     */
    DECLSPEC_XFGVIRT(IOwnerDataCallback, GetItemGroup)
    STDMETHOD(GetItemGroup)(THIS_ LONG itemIndex, LONG occurenceIndex, PLONG pGroupIndex) PURE;

    /**
     * Retrieves how often an item occurs in the control.
     *
     * \param itemIndex Control-wide zero-based item index.
     * \param pOccurenceCount Receives the number of occurrences.
     * \return HRESULT indicating success or failure.
     */
    DECLSPEC_XFGVIRT(IOwnerDataCallback, GetItemGroupCount)
    STDMETHOD(GetItemGroupCount)(THIS_ LONG itemIndex, PLONG pOccurenceCount) PURE;

    /**
     * Provides a cache hint for a range of items.
     *
     * \param firstItem First item to cache (group-wide index).
     * \param lastItem Last item to cache (group-wide index).
     * \return HRESULT indicating success or failure.
     */
    DECLSPEC_XFGVIRT(IOwnerDataCallback, OnCacheHint)
    STDMETHOD(OnCacheHint)(THIS_ LVITEMINDEX firstItem, LVITEMINDEX lastItem) PURE;

    END_INTERFACE
};

#undef INTERFACE
#define INTERFACE ISubItemCallback
/**
 * ISubItemCallback
 *
 * Callback interface for subitem operations in a custom list view. Allows
 * retrieving subitem titles, providing editing/controls per subitem or group,
 * and invoking custom verbs.
 */
DECLARE_INTERFACE_(ISubItemCallback, IUnknown)
{
    BEGIN_INTERFACE

    //
    // IUnknown
    //

    DECLSPEC_XFGVIRT(ISubItemCallback, QueryInterface)
    STDMETHOD(QueryInterface)(THIS_ REFIID riid, void** ppvObject) PURE;

    DECLSPEC_XFGVIRT(ISubItemCallback, AddRef)
    STDMETHOD_(ULONG, AddRef)(THIS) PURE;

    DECLSPEC_XFGVIRT(ISubItemCallback, Release)
    STDMETHOD_(ULONG, Release)(THIS) PURE;

    //
    // ISubItemCallback
    //

    /**
     * Retrieves the title of a subitem.
     *
     * \param subItemIndex Zero-based subitem index.
     * \param pBuffer Wide-character buffer to receive the title.
     * \param bufferSize Buffer size in characters.
     * \return HRESULT indicating success or failure.
     */
    DECLSPEC_XFGVIRT(ISubItemCallback, GetSubItemTitle)
    STDMETHOD(GetSubItemTitle)(THIS_ LONG subItemIndex, PWSTR pBuffer, LONG bufferSize) PURE;

    /**
     * Retrieves a control interface for a specific subitem.
     *
     * \param itemIndex Zero-based item index.
     * \param subItemIndex Zero-based subitem index.
     * \param requiredInterface IID of the required control interface.
     * \param ppObject Receives the interface pointer for the subitem control.
     * \return HRESULT indicating success or failure.
     */
    DECLSPEC_XFGVIRT(ISubItemCallback, GetSubItemControl)
    STDMETHOD(GetSubItemControl)(THIS_ LONG itemIndex, LONG subItemIndex, REFIID requiredInterface, void** ppObject) PURE; // IPropertyControl

    /**
     * Begins editing a subitem.
     *
     * \param itemIndex Zero-based item index.
     * \param subItemIndex Zero-based subitem index.
     * \param mode Edit mode flags.
     * \param requiredInterface IID of the required editing control interface.
     * \param ppObject Receives the interface pointer for the editing control.
     * \return HRESULT indicating success or failure.
     */
    DECLSPEC_XFGVIRT(ISubItemCallback, BeginSubItemEdit)
    STDMETHOD(BeginSubItemEdit)(THIS_ LONG itemIndex, LONG subItemIndex, LONG mode, REFIID requiredInterface, void** ppObject) PURE; // IPropertyControl

    /**
     * Ends editing of a subitem.
     *
     * \param itemIndex Zero-based item index.
     * \param subItemIndex Zero-based subitem index.
     * \param mode Edit mode flags.
     * \param pPropertyControl The property control used during editing.
     * \return HRESULT indicating success or failure.
     */
    DECLSPEC_XFGVIRT(ISubItemCallback, EndSubItemEdit)
    STDMETHOD(EndSubItemEdit)(THIS_ LONG itemIndex, LONG subItemIndex, LONG mode, void** ppObject) PURE; // IPropertyControl

    /**
     * Begins editing a group.
     *
     * \param groupIndex Zero-based group index.
     * \param requiredInterface IID of the required editing control interface.
     * \param ppObject Receives the interface pointer for the editing control.
     * \return HRESULT indicating success or failure.
     */
    DECLSPEC_XFGVIRT(ISubItemCallback, BeginGroupEdit)
    STDMETHOD(BeginGroupEdit)(THIS_ LONG groupIndex, REFIID iid, void** ppObject) PURE; // IPropertyControl

    /**
     * Ends editing of a group.
     *
     * \param groupIndex Zero-based group index.
     * \param mode Edit mode flags.
     * \param pPropertyControl The property control used during editing.
     * \return HRESULT indicating success or failure.
     */
    DECLSPEC_XFGVIRT(ISubItemCallback, EndGroupEdit)
    STDMETHOD(EndGroupEdit)(THIS_ LONG groupIndex, LONG mode, void** ppObject) PURE; // IPropertyControl

    /**
     * Invokes a custom verb on a subitem.
     *
     * \param itemIndex Zero-based item index.
     * \param pVerb The verb to invoke (wide string).
     * \return HRESULT indicating success or failure.
     */
    DECLSPEC_XFGVIRT(ISubItemCallback, OnInvokeVerb)
    STDMETHOD(OnInvokeVerb)(THIS_ LONG itemIndex, LPCWSTR pVerb) PURE;

    END_INTERFACE
};

#undef INTERFACE
#define INTERFACE IDrawPropertyControl
/**
 * IDrawPropertyControl
 *
 * This interface defines methods for custom drawing of property controls within the application's
 * graphical user interface. Implementers provide the logic necessary to render the visuals.
 */
DECLARE_INTERFACE_(IDrawPropertyControl, IUnknown)
{
    BEGIN_INTERFACE

    //
    // IUnknown
    //

    DECLSPEC_XFGVIRT(IDrawPropertyControl, QueryInterface)
    STDMETHOD(QueryInterface)(THIS_ REFIID riid, void** ppvObject) PURE;

    DECLSPEC_XFGVIRT(IDrawPropertyControl, AddRef)
    STDMETHOD_(ULONG, AddRef)(THIS) PURE;

    DECLSPEC_XFGVIRT(IDrawPropertyControl, Release)
    STDMETHOD_(ULONG, Release)(THIS) PURE;

    //
    // IPropertyControlBase
    //

    DECLSPEC_XFGVIRT(IDrawPropertyControl, Initialize)
    STDMETHOD(Initialize)(THIS_ IUnknown*, ULONG) PURE; // 2/calendar, 3/textbox

    DECLSPEC_XFGVIRT(IDrawPropertyControl, GetSize)
    STDMETHOD(GetSize)(THIS_ enum PROPCTL_RECT_TYPE, HDC, SIZE const*, SIZE*) PURE;

    DECLSPEC_XFGVIRT(IDrawPropertyControl, SetWindowTheme)
    STDMETHOD(SetWindowTheme)(THIS_ PCWSTR, PCWSTR) PURE;

    DECLSPEC_XFGVIRT(IDrawPropertyControl, SetFont)
    STDMETHOD(SetFont)(THIS_ HFONT) PURE;

    DECLSPEC_XFGVIRT(IDrawPropertyControl, SetForeColor)
    STDMETHOD(SetForeColor)(THIS_ ULONG) PURE;

    DECLSPEC_XFGVIRT(IDrawPropertyControl, GetFlags)
    STDMETHOD(GetFlags)(THIS_ LONG*) PURE;

    DECLSPEC_XFGVIRT(IDrawPropertyControl, SetFlags)
    STDMETHOD(SetFlags)(THIS_ LONG, LONG) PURE;

    DECLSPEC_XFGVIRT(IDrawPropertyControl, AdjustWindowRectPCB)
    STDMETHOD(AdjustWindowRectPCB)(THIS_ HWND, RECT*, RECT const*, LONG) PURE;

    DECLSPEC_XFGVIRT(IDrawPropertyControl, SetValue)
    STDMETHOD(SetValue)(THIS_ IUnknown*) PURE;

    DECLSPEC_XFGVIRT(IDrawPropertyControl, InvokeDefaultAction)
    STDMETHOD(InvokeDefaultAction)(THIS) PURE;

    DECLSPEC_XFGVIRT(IDrawPropertyControl, Destroy)
    STDMETHOD(Destroy)(THIS) PURE;

    DECLSPEC_XFGVIRT(IDrawPropertyControl, SetFormatFlags)
    STDMETHOD(SetFormatFlags)(THIS_ LONG) PURE;

    DECLSPEC_XFGVIRT(IDrawPropertyControl, GetFormatFlags)
    STDMETHOD(GetFormatFlags)(THIS_ LONG*) PURE;

    //
    // IDrawPropertyControl
    //

    DECLSPEC_XFGVIRT(IDrawPropertyControl, GetDrawFlags)
    STDMETHOD(GetDrawFlags)(THIS_ PLONG Flags) PURE;

    DECLSPEC_XFGVIRT(IDrawPropertyControl, WindowlessDraw)
    STDMETHOD(WindowlessDraw)(THIS_ HDC hDC, RECT pDrawingRectangle, LONG a) PURE;

    DECLSPEC_XFGVIRT(IDrawPropertyControl, HasVisibleContent)
    STDMETHOD(HasVisibleContent)(THIS) PURE;

    DECLSPEC_XFGVIRT(IDrawPropertyControl, GetDisplayText)
    STDMETHOD(GetDisplayText)(THIS_ PWSTR *Text) PURE;

    DECLSPEC_XFGVIRT(IDrawPropertyControl, GetTooltipInfo)
    STDMETHOD(GetTooltipInfo)(THIS_ HDC, SIZE const*, PLONG) PURE;

    END_INTERFACE
};

#undef INTERFACE
#define INTERFACE IPropertyControl
DECLARE_INTERFACE_(IPropertyControl, IUnknown)
{
    BEGIN_INTERFACE

    //
    // IUnknown
    //

    DECLSPEC_XFGVIRT(IPropertyControl, QueryInterface)
    STDMETHOD(QueryInterface)(THIS_ REFIID riid, void** ppvObject) PURE;

    DECLSPEC_XFGVIRT(IPropertyControl, AddRef)
    STDMETHOD_(ULONG, AddRef)(THIS) PURE;

    DECLSPEC_XFGVIRT(IPropertyControl, Release)
    STDMETHOD_(ULONG, Release)(THIS) PURE;

    //
    // IPropertyControl
    //

    DECLSPEC_XFGVIRT(IPropertyControl, GetValue)
    STDMETHOD(GetValue)(THIS_ REFIID riid, void** ppvObject) PURE;

    DECLSPEC_XFGVIRT(IPropertyControl, Create)
    STDMETHOD(Create)(THIS_ HWND Parent, RECT const* ItemRectangle, RECT const* Unknown1, int Unknown2) PURE;

    DECLSPEC_XFGVIRT(IPropertyControl, SetPosition)
    STDMETHOD(SetPosition)(THIS_ RECT const*, RECT const*) PURE;

    DECLSPEC_XFGVIRT(IPropertyControl, IsModified)
    STDMETHOD(IsModified)(THIS_ PBOOL Modified) PURE;

    DECLSPEC_XFGVIRT(IPropertyControl, SetModified)
    STDMETHOD(SetModified)(THIS_ BOOL Modified) PURE;

    DECLSPEC_XFGVIRT(IPropertyControl, ValidationFailed)
    STDMETHOD(ValidationFailed)(THIS_ PCWSTR) PURE;

    DECLSPEC_XFGVIRT(IPropertyControl, GetState)
    STDMETHOD(GetState)(THIS_ PULONG State) PURE;

    END_INTERFACE
};

#undef INTERFACE
#define INTERFACE IListView
DECLARE_INTERFACE_(IListView, IUnknown) // real name is IListView2
{
    BEGIN_INTERFACE

    //
    // IUnknown
    //

    DECLSPEC_XFGVIRT(IListView, QueryInterface)
    STDMETHOD(QueryInterface)(THIS_ REFIID riid, void** ppvObject) PURE;

    DECLSPEC_XFGVIRT(IListView, AddRef)
    STDMETHOD_(ULONG, AddRef)(THIS) PURE;

    DECLSPEC_XFGVIRT(IListView, Release)
    STDMETHOD_(ULONG, Release)(THIS) PURE;

    //
    // IOleWindow
    //

    DECLSPEC_XFGVIRT(IListView, GetWindow)
    STDMETHOD(GetWindow)(THIS_ __RPC__in IOleWindow* window, __RPC__deref_out_opt HWND* WindowHandle) PURE;

    DECLSPEC_XFGVIRT(IListView, ContextSensitiveHelp)
    STDMETHOD(ContextSensitiveHelp)(THIS_ __RPC__in IOleWindow* window, _In_ BOOL fEnterMode) PURE;

    //
    // IListView
    //

    /**
     * Retrieves the handle to the image list used by the list view.
     *
     * \param ImageListType The type of image list to retrieve (e.g., normal, small, state).
     * \param ImageList Pointer to a variable that receives the handle to the image list.
     * \return HRESULT indicating success or failure.
     */
    DECLSPEC_XFGVIRT(IListView, GetImageList)
    STDMETHOD(GetImageList)(THIS_ LONG ImageListType, HIMAGELIST* ImageList) PURE;

    /**
     * Sets a new image list for the list view and optionally retrieves the handle to the previous image list.
     *
     * \param ImageListType The type of image list to set (e.g., normal, small, state).
     * \param NewImageList Handle to the new image list to associate with the list view.
     * \param OldImageList Pointer to a variable that receives the handle to the previous image list.
     * \return HRESULT indicating success or failure.
     */
    DECLSPEC_XFGVIRT(IListView, SetImageList)
    STDMETHOD(SetImageList)(THIS_ LONG ImageListType, HIMAGELIST NewImageList, HIMAGELIST* OldImageList) PURE;

    /**
     * Retrieves the current background color of the list view.
     *
     * \param Color Pointer to a variable that receives the background color (COLORREF).
     * \return HRESULT indicating success or failure.
     */
    DECLSPEC_XFGVIRT(IListView, GetBackgroundColor)
    STDMETHOD(GetBackgroundColor)(THIS_ COLORREF* Color) PURE;

    /**
     * Sets the background color of the list view.
     *
     * \param color The new background color (COLORREF) to set.
     * \return HRESULT indicating success or failure.
     */
    DECLSPEC_XFGVIRT(IListView, SetBackgroundColor)
    STDMETHOD(SetBackgroundColor)(THIS_ COLORREF Color) PURE;

    /**
     * \brief Retrieves the current text color used by the list view.
     *
     * \param[out] Color Pointer to a COLORREF variable that receives the current text color.
     * \return HRESULT indicating success or failure.
     */
    DECLSPEC_XFGVIRT(IListView, GetTextColor)
    STDMETHOD(GetTextColor)(THIS_ COLORREF* Color) PURE;

    /**
     * \brief Sets the foreground (text) color for the list view.
     *
     * \param[in] Color The COLORREF value specifying the new text color.
     * \return HRESULT indicating success or failure.
     */
    DECLSPEC_XFGVIRT(IListView, SetForeColor)
    STDMETHOD(SetForeColor)(THIS_ COLORREF Color) PURE;

    /**
     * \brief Retrieves the current background color used for the text in the list view.
     *
     * \param[out] Color Pointer to a COLORREF variable that receives the current text background color.
     * \return HRESULT indicating success or failure.
     */
    DECLSPEC_XFGVIRT(IListView, GetTextBackgroundColor)
    STDMETHOD(GetTextBackgroundColor)(THIS_ COLORREF* Color) PURE;

    /**
     * \brief Sets the background color for the text in the list view.
     *
     * \param[in] color The COLORREF value specifying the new text background color.
     * \return HRESULT indicating success or failure.
     */
    DECLSPEC_XFGVIRT(IListView, SetTextBackgroundColor)
    STDMETHOD(SetTextBackgroundColor)(THIS_ COLORREF Color) PURE;

    DECLSPEC_XFGVIRT(IListView, GetHotLightColor)
    STDMETHOD(GetHotLightColor)(THIS_ COLORREF* Color) PURE;

    DECLSPEC_XFGVIRT(IListView, SetHotLightColor)
    STDMETHOD(SetHotLightColor)(THIS_ COLORREF Color) PURE;

    DECLSPEC_XFGVIRT(IListView, GetItemCount)
    STDMETHOD(GetItemCount)(THIS_ PLONG ItemCount) PURE;

    DECLSPEC_XFGVIRT(IListView, SetItemCount)
    STDMETHOD(SetItemCount)(THIS_ LONG ItemCount, ULONG Flags) PURE;

    DECLSPEC_XFGVIRT(IListView, GetItem)
    STDMETHOD(GetItem)(THIS_ LVITEMW* Item) PURE;

    DECLSPEC_XFGVIRT(IListView, SetItem)
    STDMETHOD(SetItem)(THIS_ LVITEMW* const Item) PURE;

    DECLSPEC_XFGVIRT(IListView, GetItemState)
    STDMETHOD(GetItemState)(THIS_ LONG ItemIndex, LONG SubItemIndex, ULONG Mask, PULONG State) PURE;

    DECLSPEC_XFGVIRT(IListView, SetItemState)
    STDMETHOD(SetItemState)(THIS_ LONG ItemIndex, LONG SubItemIndex, ULONG Mask, ULONG State) PURE;

    DECLSPEC_XFGVIRT(IListView, GetItemText)
    STDMETHOD(GetItemText)(THIS_ LONG ItemIndex, LONG SubItemIndex, PWSTR Buffer, LONG BufferSize) PURE;

    DECLSPEC_XFGVIRT(IListView, SetItemText)
    STDMETHOD(SetItemText)(THIS_ LONG ItemIndex, LONG SubItemIndex, PCWSTR Text) PURE;

    DECLSPEC_XFGVIRT(IListView, GetBackgroundImage)
    STDMETHOD(GetBackgroundImage)(THIS_ LVBKIMAGEW* pBkImage) PURE;

    DECLSPEC_XFGVIRT(IListView, SetBackgroundImage)
    STDMETHOD(SetBackgroundImage)(THIS_ LVBKIMAGEW* const BkImage) PURE;

    DECLSPEC_XFGVIRT(IListView, GetFocusedColumn)
    STDMETHOD(GetFocusedColumn)(THIS_ PLONG ColumnIndex) PURE;

    DECLSPEC_XFGVIRT(IListView, SetSelectionFlags)
    STDMETHOD(SetSelectionFlags)(THIS_ LONG Mask, LONG Flags) PURE; // HRESULT SetSelectionFlags (SELECTION_FLAGS, SELECTION_FLAGS);

    DECLSPEC_XFGVIRT(IListView, GetSelectedColumn)
    STDMETHOD(GetSelectedColumn)(THIS_ PLONG ColumnIndex) PURE;

    DECLSPEC_XFGVIRT(IListView, SetSelectedColumn)
    STDMETHOD(SetSelectedColumn)(THIS_ LONG ColumnIndex) PURE;

    DECLSPEC_XFGVIRT(IListView, GetView)
    STDMETHOD(GetView)(THIS_ PULONG View) PURE;

    DECLSPEC_XFGVIRT(IListView, SetView)
    STDMETHOD(SetView)(THIS_ ULONG View) PURE;

    DECLSPEC_XFGVIRT(IListView, InsertItem)
    STDMETHOD(InsertItem)(THIS_ LVITEMW* const Item, PLONG ItemIndex) PURE;

    DECLSPEC_XFGVIRT(IListView, DeleteItem)
    STDMETHOD(DeleteItem)(THIS_ LONG ItemIndex) PURE;

    DECLSPEC_XFGVIRT(IListView, DeleteAllItems)
    STDMETHOD(DeleteAllItems)(THIS) PURE;

    DECLSPEC_XFGVIRT(IListView, UpdateItem)
    STDMETHOD(UpdateItem)(THIS_ LONG ItemIndex) PURE;

    DECLSPEC_XFGVIRT(IListView, GetItemRect)
    STDMETHOD(GetItemRect)(THIS_ LVITEMINDEX ItemIndex, LONG RectangleType, PRECT Rectangle) PURE;

    DECLSPEC_XFGVIRT(IListView, GetSubItemRect)
    STDMETHOD(GetSubItemRect)(THIS_ LVITEMINDEX ItemIndex, LONG SubItemIndex, LONG RectangleType, PRECT Rectangle) PURE;

    DECLSPEC_XFGVIRT(IListView, HitTestSubItem)
    STDMETHOD(HitTestSubItem)(THIS_ LVHITTESTINFO* HitTestData) PURE;

    DECLSPEC_XFGVIRT(IListView, GetIncrSearchString)
    STDMETHOD(GetIncrSearchString)(THIS_ PWSTR Buffer, LONG BufferSize, PLONG CopiedChars) PURE;

    DECLSPEC_XFGVIRT(IListView, GetItemSpacing)
    STDMETHOD(GetItemSpacing)(THIS_ BOOL SmallIconView, PLONG HorizontalSpacing, PLONG VerticalSpacing) PURE;

    DECLSPEC_XFGVIRT(IListView, SetIconSpacing)
    STDMETHOD(SetIconSpacing)(THIS_ LONG HorizontalSpacing, LONG VerticalSpacing, PLONG OldHorizontalSpacing, PLONG OldVerticalSpacing) PURE;

    DECLSPEC_XFGVIRT(IListView, GetNextItem)
    STDMETHOD(GetNextItem)(THIS_ LVITEMINDEX ItemIndex, ULONG Flags, LVITEMINDEX* NextItemIndex) PURE;

    DECLSPEC_XFGVIRT(IListView, FindItem)
    STDMETHOD(FindItem)(THIS_ LVITEMINDEX startItemIndex, LVFINDINFOW const* FindInfo, LVITEMINDEX* FoundItemIndex) PURE;

    DECLSPEC_XFGVIRT(IListView, GetSelectionMark)
    STDMETHOD(GetSelectionMark)(THIS_ LVITEMINDEX* SelectionMark) PURE;

    DECLSPEC_XFGVIRT(IListView, SetSelectionMark)
    STDMETHOD(SetSelectionMark)(THIS_ LVITEMINDEX NewSelectionMark, LVITEMINDEX* OldSelectionMark) PURE;

    DECLSPEC_XFGVIRT(IListView, GetItemPosition)
    STDMETHOD(GetItemPosition)(THIS_ LVITEMINDEX ItemIndex, POINT* Position) PURE;

    DECLSPEC_XFGVIRT(IListView, SetItemPosition)
    STDMETHOD(SetItemPosition)(THIS_ LONG ItemIndex, POINT const* Position) PURE;

    DECLSPEC_XFGVIRT(IListView, ScrollView)
    STDMETHOD(ScrollView)(THIS_ ULONG HorizontalScrollDistance, ULONG VerticalScrollDistance) PURE;

    DECLSPEC_XFGVIRT(IListView, EnsureItemVisible)
    STDMETHOD(EnsureItemVisible)(THIS_ LVITEMINDEX ItemIndex, BOOL PartialOk) PURE;

    DECLSPEC_XFGVIRT(IListView, EnsureSubItemVisible)
    STDMETHOD(EnsureSubItemVisible)(THIS_ LVITEMINDEX ItemIndex, LONG SubItemIndex) PURE;

    DECLSPEC_XFGVIRT(IListView, EditSubItem)
    STDMETHOD(EditSubItem)(THIS_ LVITEMINDEX ItemIndex, LONG SubItemIndex) PURE;

    DECLSPEC_XFGVIRT(IListView, RedrawItems)
    STDMETHOD(RedrawItems)(THIS_ LONG FirstItemIndex, LONG LastItemIndex) PURE;

    DECLSPEC_XFGVIRT(IListView, ArrangeItems)
    STDMETHOD(ArrangeItems)(THIS_ LONG Mode) PURE;

    DECLSPEC_XFGVIRT(IListView, RecomputeItems)
    STDMETHOD(RecomputeItems)(THIS_ LONG Mode) PURE;

    DECLSPEC_XFGVIRT(IListView, GetEditControl)
    STDMETHOD(GetEditControl)(THIS_ HWND* EditWindowHandle) PURE;

    DECLSPEC_XFGVIRT(IListView, EditLabel)
    STDMETHOD(EditLabel)(THIS_ LVITEMINDEX ItemIndex, PCWSTR InitialEditText, HWND* EditWindowHandle) PURE;

    DECLSPEC_XFGVIRT(IListView, EditGroupLabel)
    STDMETHOD(EditGroupLabel)(THIS_ LONG GroupIndex) PURE;

    DECLSPEC_XFGVIRT(IListView, CancelEditLabel)
    STDMETHOD(CancelEditLabel)(THIS) PURE;

    DECLSPEC_XFGVIRT(IListView, GetEditItem)
    STDMETHOD(GetEditItem)(THIS_ LVITEMINDEX* ItemIndex, PLONG SubItemIndex) PURE;

    DECLSPEC_XFGVIRT(IListView, HitTest)
    STDMETHOD(HitTest)(THIS_ LVHITTESTINFO* HitTestData) PURE;

    DECLSPEC_XFGVIRT(IListView, GetStringWidth)
    STDMETHOD(GetStringWidth)(THIS_ PCWSTR String, PLONG Width) PURE;

    DECLSPEC_XFGVIRT(IListView, GetColumn)
    STDMETHOD(GetColumn)(THIS_ LONG ColumnIndex, LVCOLUMNW* Column) PURE;

    DECLSPEC_XFGVIRT(IListView, SetColumn)
    STDMETHOD(SetColumn)(THIS_ LONG ColumnIndex, LVCOLUMNW* const Column) PURE;

    DECLSPEC_XFGVIRT(IListView, GetColumnOrderArray)
    STDMETHOD(GetColumnOrderArray)(THIS_ LONG NumberOfColumns, _Out_ PVOID* Columns) PURE;

    DECLSPEC_XFGVIRT(IListView, SetColumnOrderArray)
    STDMETHOD(SetColumnOrderArray)(THIS_ LONG NumberOfColumns, _In_ PVOID Columns) PURE;

    DECLSPEC_XFGVIRT(IListView, GetHeaderControl)
    STDMETHOD(GetHeaderControl)(THIS_ HWND* HeaderWindowHandle) PURE;

    DECLSPEC_XFGVIRT(IListView, InsertColumn)
    STDMETHOD(InsertColumn)(THIS_ LONG InsertAt, LVCOLUMNW* const Column, PLONG ColumnIndex) PURE;

    DECLSPEC_XFGVIRT(IListView, DeleteColumn)
    STDMETHOD(DeleteColumn)(THIS_ LONG ColumnIndex) PURE;

    DECLSPEC_XFGVIRT(IListView, CreateDragImage)
    STDMETHOD(CreateDragImage)(THIS_ LONG ItemIndex, POINT const* UpperLeft, HIMAGELIST* ImageList) PURE;

    DECLSPEC_XFGVIRT(IListView, GetViewRect)
    STDMETHOD(GetViewRect)(THIS_ RECT* Rectangle) PURE;

    DECLSPEC_XFGVIRT(IListView, GetClientRect)
    STDMETHOD(GetClientRect)(THIS_ BOOL StyleAndClientRect, RECT* ClientRectangle) PURE;

    DECLSPEC_XFGVIRT(IListView, GetColumnWidth)
    STDMETHOD(GetColumnWidth)(THIS_ LONG ColumnIndex, PLONG Width) PURE;

    DECLSPEC_XFGVIRT(IListView, SetColumnWidth)
    STDMETHOD(SetColumnWidth)(THIS_ LONG ColumnIndex, LONG Width) PURE;

    DECLSPEC_XFGVIRT(IListView, GetCallbackMask)
    STDMETHOD(GetCallbackMask)(THIS_ PULONG Mask) PURE;

    DECLSPEC_XFGVIRT(IListView, SetCallbackMask)
    STDMETHOD(SetCallbackMask)(THIS_ ULONG Mask) PURE;

    DECLSPEC_XFGVIRT(IListView, GetTopIndex)
    STDMETHOD(GetTopIndex)(THIS_ PLONG TopIndex) PURE;

    DECLSPEC_XFGVIRT(IListView, GetCountPerPage)
    STDMETHOD(GetCountPerPage)(THIS_ PLONG CountPerPage) PURE;

    DECLSPEC_XFGVIRT(IListView, GetOrigin)
    STDMETHOD(GetOrigin)(THIS_ POINT* Origin) PURE;

    DECLSPEC_XFGVIRT(IListView, GetSelectedCount)
    STDMETHOD(GetSelectedCount)(THIS_ PLONG SelectedCount) PURE;

    DECLSPEC_XFGVIRT(IListView, SortItems)
    STDMETHOD(SortItems)(THIS_ BOOL SortingByIndex, LPARAM lParam, PFNLVCOMPARE ComparisonFunction) PURE;

    DECLSPEC_XFGVIRT(IListView, GetExtendedStyle)
    STDMETHOD(GetExtendedStyle)(THIS_ PULONG Style) PURE;

    DECLSPEC_XFGVIRT(IListView, SetExtendedStyle)
    STDMETHOD(SetExtendedStyle)(THIS_ ULONG Mask, ULONG Style, PULONG OldStyle) PURE;

    DECLSPEC_XFGVIRT(IListView, GetHoverTime)
    STDMETHOD(GetHoverTime)(THIS_ PULONG Time) PURE;

    DECLSPEC_XFGVIRT(IListView, SetHoverTime)
    STDMETHOD(SetHoverTime)(THIS_ ULONG Time, PULONG OldSetting) PURE;

    DECLSPEC_XFGVIRT(IListView, GetToolTip)
    STDMETHOD(GetToolTip)(THIS_ HWND* ToolTipWindowHandle) PURE;

    DECLSPEC_XFGVIRT(IListView, SetToolTip)
    STDMETHOD(SetToolTip)(THIS_ HWND ToolTipWindowHandle, HWND* OldToolTipWindowHandle) PURE;

    DECLSPEC_XFGVIRT(IListView, GetHotItem)
    STDMETHOD(GetHotItem)(THIS_ LVITEMINDEX* HotItem) PURE;

    DECLSPEC_XFGVIRT(IListView, SetHotItem)
    STDMETHOD(SetHotItem)(THIS_ LVITEMINDEX NewHotItem, LVITEMINDEX* OldHotItem) PURE;

    DECLSPEC_XFGVIRT(IListView, GetHotCursor)
    STDMETHOD(GetHotCursor)(THIS_ HCURSOR* Cursor) PURE;

    DECLSPEC_XFGVIRT(IListView, SetHotCursor)
    STDMETHOD(SetHotCursor)(THIS_ HCURSOR Cursor, HCURSOR* OldCursor) PURE;

    DECLSPEC_XFGVIRT(IListView, ApproximateViewRect)
    STDMETHOD(ApproximateViewRect)(THIS_ LONG ItemCount, PLONG Width, PLONG Height) PURE;

    DECLSPEC_XFGVIRT(IListView, SetRangeObject)
    STDMETHOD(SetRangeObject)(THIS_ LONG unknown, void** Object) PURE;

    DECLSPEC_XFGVIRT(IListView, GetWorkAreas)
    STDMETHOD(GetWorkAreas)(THIS_ LONG NumberOfWorkAreas, RECT* WorkAreas) PURE;

    DECLSPEC_XFGVIRT(IListView, SetWorkAreas)
    STDMETHOD(SetWorkAreas)(THIS_ LONG NumberOfWorkAreas, RECT const* WorkAreas) PURE;

    DECLSPEC_XFGVIRT(IListView, GetWorkAreaCount)
    STDMETHOD(GetWorkAreaCount)(THIS_ PLONG NumberOfWorkAreas) PURE;

    DECLSPEC_XFGVIRT(IListView, ResetEmptyText)
    STDMETHOD(ResetEmptyText)(THIS) PURE;

    DECLSPEC_XFGVIRT(IListView, EnableGroupView)
    STDMETHOD(EnableGroupView)(THIS_ BOOL Enable) PURE;

    DECLSPEC_XFGVIRT(IListView, IsGroupViewEnabled)
    STDMETHOD(IsGroupViewEnabled)(THIS_ PBOOL IsEnabled) PURE;

    DECLSPEC_XFGVIRT(IListView, SortGroups)
    STDMETHOD(SortGroups)(THIS_ PFNLVGROUPCOMPARE pComparisonFunction, PVOID lParam) PURE;

    DECLSPEC_XFGVIRT(IListView, GetGroupInfo)
    STDMETHOD(GetGroupInfo)(THIS_ BOOL GetGroupInfoByIndex, LONG GroupIDOrIndex, LVGROUP* Group) PURE;

    DECLSPEC_XFGVIRT(IListView, SetGroupInfo)
    STDMETHOD(SetGroupInfo)(THIS_ BOOL SetGroupInfoByIndex, LONG GroupIDOrIndex, LVGROUP* const Group) PURE;

    DECLSPEC_XFGVIRT(IListView, GetGroupRect)
    STDMETHOD(GetGroupRect)(THIS_ BOOL GetGroupRectByIndex, LONG GroupIDOrIndex, LONG RectangleType, RECT* pRectangle) PURE;

    DECLSPEC_XFGVIRT(IListView, GetGroupState)
    STDMETHOD(GetGroupState)(THIS_ LONG GroupID, ULONG Mask, PULONG State) PURE;

    DECLSPEC_XFGVIRT(IListView, HasGroup)
    STDMETHOD(HasGroup)(THIS_ LONG GroupID, BOOL* HasGroup) PURE;

    DECLSPEC_XFGVIRT(IListView, InsertGroup)
    STDMETHOD(InsertGroup)(THIS_ LONG InsertAt, LVGROUP* const Group, PLONG GroupID) PURE;

    DECLSPEC_XFGVIRT(IListView, RemoveGroup)
    STDMETHOD(RemoveGroup)(THIS_ LONG GroupID) PURE;

    DECLSPEC_XFGVIRT(IListView, InsertGroupSorted)
    STDMETHOD(InsertGroupSorted)(THIS_ LVINSERTGROUPSORTED const* Group, PLONG GroupID) PURE;

    DECLSPEC_XFGVIRT(IListView, GetGroupMetrics)
    STDMETHOD(GetGroupMetrics)(THIS_ LVGROUPMETRICS* Metrics) PURE;

    DECLSPEC_XFGVIRT(IListView, SetGroupMetrics)
    STDMETHOD(SetGroupMetrics)(THIS_ LVGROUPMETRICS* const Metrics) PURE;

    DECLSPEC_XFGVIRT(IListView, RemoveAllGroups)
    STDMETHOD(RemoveAllGroups)(THIS) PURE;

    DECLSPEC_XFGVIRT(IListView, GetFocusedGroup)
    STDMETHOD(GetFocusedGroup)(THIS_ PLONG pGroupID) PURE;

    DECLSPEC_XFGVIRT(IListView, GetGroupCount)
    STDMETHOD(GetGroupCount)(THIS_ PLONG pCount) PURE;

    DECLSPEC_XFGVIRT(IListView, SetOwnerDataCallback)
    STDMETHOD(SetOwnerDataCallback)(THIS_ IOwnerDataCallback* pCallback) PURE;

    DECLSPEC_XFGVIRT(IListView, GetTileViewInfo)
    STDMETHOD(GetTileViewInfo)(THIS_ LVTILEVIEWINFO* pInfo) PURE;

    DECLSPEC_XFGVIRT(IListView, SetTileViewInfo)
    STDMETHOD(SetTileViewInfo)(THIS_ LVTILEVIEWINFO* const pInfo) PURE;

    DECLSPEC_XFGVIRT(IListView, GetTileInfo)
    STDMETHOD(GetTileInfo)(THIS_ LVTILEINFO* pTileInfo) PURE;

    DECLSPEC_XFGVIRT(IListView, SetTileInfo)
    STDMETHOD(SetTileInfo)(THIS_ LVTILEINFO* const pTileInfo) PURE;

    DECLSPEC_XFGVIRT(IListView, GetInsertMark)
    STDMETHOD(GetInsertMark)(THIS_ LVINSERTMARK* pInsertMarkDetails) PURE;

    DECLSPEC_XFGVIRT(IListView, SetInsertMark)
    STDMETHOD(SetInsertMark)(THIS_ LVINSERTMARK const* pInsertMarkDetails) PURE;

    DECLSPEC_XFGVIRT(IListView, GetInsertMarkRect)
    STDMETHOD(GetInsertMarkRect)(THIS_ LPRECT pInsertMarkRectangle) PURE;

    DECLSPEC_XFGVIRT(IListView, GetInsertMarkColor)
    STDMETHOD(GetInsertMarkColor)(THIS_ COLORREF* pColor) PURE;

    DECLSPEC_XFGVIRT(IListView, SetInsertMarkColor)
    STDMETHOD(SetInsertMarkColor)(THIS_ COLORREF color, COLORREF* pOldColor) PURE;

    DECLSPEC_XFGVIRT(IListView, HitTestInsertMark)
    STDMETHOD(HitTestInsertMark)(THIS_ POINT const* Point, LVINSERTMARK* pInsertMarkDetails) PURE;

    DECLSPEC_XFGVIRT(IListView, SetInfoTip)
    STDMETHOD(SetInfoTip)(THIS_ LVSETINFOTIP* const InfoTip) PURE;

    DECLSPEC_XFGVIRT(IListView, GetOutlineColor)
    STDMETHOD(GetOutlineColor)(THIS_ COLORREF* Color) PURE;

    DECLSPEC_XFGVIRT(IListView, SetOutlineColor)
    STDMETHOD(SetOutlineColor)(THIS_ COLORREF Color, COLORREF* OldColor) PURE;

    DECLSPEC_XFGVIRT(IListView, GetFrozenItem)
    STDMETHOD(GetFrozenItem)(THIS_ PLONG ItemIndex) PURE;

    DECLSPEC_XFGVIRT(IListView, SetFrozenItem)
    STDMETHOD(SetFrozenItem)(THIS_ LONG ItemIndexStart, LONG ItemIndexEnd) PURE;

    DECLSPEC_XFGVIRT(IListView, GetFrozenSlot)
    STDMETHOD(GetFrozenSlot)(THIS_ RECT* Rect) PURE;

    DECLSPEC_XFGVIRT(IListView, SetFrozenSlot)
    STDMETHOD(SetFrozenSlot)(THIS_ LONG Slot, POINT const* Rect) PURE;

    DECLSPEC_XFGVIRT(IListView, GetViewMargin)
    STDMETHOD(GetViewMargin)(THIS_ RECT* Margin) PURE;

    DECLSPEC_XFGVIRT(IListView, SetViewMargin)
    STDMETHOD(SetViewMargin)(THIS_ RECT const* Margin) PURE;

    DECLSPEC_XFGVIRT(IListView, SetKeyboardSelected)
    STDMETHOD(SetKeyboardSelected)(THIS_ LVITEMINDEX ItemIndex) PURE;

    DECLSPEC_XFGVIRT(IListView, MapIndexToId)
    STDMETHOD(MapIndexToId)(THIS_ LONG ItemIndex, PLONG ItemID) PURE;

    DECLSPEC_XFGVIRT(IListView, MapIdToIndex)
    STDMETHOD(MapIdToIndex)(THIS_ LONG ItemID, PLONG ItemIndex) PURE;

    DECLSPEC_XFGVIRT(IListView, IsItemVisible)
    STDMETHOD(IsItemVisible)(THIS_ LVITEMINDEX ItemIndex, BOOL* Visible) PURE;

    DECLSPEC_XFGVIRT(IListView, EnableAlphaShadow)
    STDMETHOD(EnableAlphaShadow)(THIS_ BOOL Enable) PURE;

    DECLSPEC_XFGVIRT(IListView, GetGroupSubsetCount)
    STDMETHOD(GetGroupSubsetCount)(THIS_ PLONG NumberOfRowsDisplayed) PURE;

    DECLSPEC_XFGVIRT(IListView, SetGroupSubsetCount)
    STDMETHOD(SetGroupSubsetCount)(THIS_ LONG NumberOfRowsToDisplay) PURE;

    DECLSPEC_XFGVIRT(IListView, GetVisibleSlotCount)
    STDMETHOD(GetVisibleSlotCount)(THIS_ PLONG Count) PURE;

    DECLSPEC_XFGVIRT(IListView, GetColumnMargin)
    STDMETHOD(GetColumnMargin)(THIS_ RECT* Margin) PURE;

    DECLSPEC_XFGVIRT(IListView, SetSubItemCallback)
    STDMETHOD(SetSubItemCallback)(THIS_ LPVOID Callback) PURE;

    DECLSPEC_XFGVIRT(IListView, GetVisibleItemRange)
    STDMETHOD(GetVisibleItemRange)(THIS_ LVITEMINDEX* FirstItem, LVITEMINDEX* LastItem) PURE;

    DECLSPEC_XFGVIRT(IListView, SetTypeAheadFlags)
    STDMETHOD(SetTypeAheadFlags)(THIS_ ULONG Mask, ULONG Flags) PURE; // HRESULT SetTypeAheadFlags (TYPEAHEAD_FLAGS, TYPEAHEAD_FLAGS);

    // Windows 10

    /**
     * \brief Sets the work areas with DPI awareness for the view.
     *
     * \param [in] LONG The number of work areas to set.
     * \param [in] struct tagLVWORKAREAWITHDPI const* Pointer to an array of LVWORKAREAWITHDPI structures that define the work areas and their associated DPI settings.
     * \return HRESULT Returns S_OK if successful, or an error code otherwise.
     */
    DECLSPEC_XFGVIRT(IListView, SetWorkAreasWithDpi)
    STDMETHOD(SetWorkAreasWithDpi)(THIS_ LONG index, struct tagLVWORKAREAWITHDPI const*) PURE;

    /**
     * \brief Retrieves the work areas along with their associated DPI settings.
     *
     * \param [in] LONG The index or identifier for the work area.
     * \param [out] tagLVWORKAREAWITHDPI* Pointer to a structure that receives the work area information and its DPI.
     *
     * \return HRESULT indicating success or failure of the operation.
     */
    DECLSPEC_XFGVIRT(IListView, GetWorkAreasWithDpi)
    STDMETHOD(GetWorkAreasWithDpi)(THIS_ LONG index, struct tagLVWORKAREAWITHDPI*) PURE;

    /**
     * Sets the image list for the work area.
     *
     * \param [in] param1 The first LONG parameter (purpose depends on implementation).
     * \param [in] param2 The second LONG parameter (purpose depends on implementation).
     * \param [in] hImageList Handle to the image list to be set.
     * \param [out] phOldImageList Pointer to a variable that receives the handle to the previous image list.
     * \return HRESULT Returns S_OK on success, or an error code otherwise.
     */
    DECLSPEC_XFGVIRT(IListView, SetWorkAreaImageList)
    STDMETHOD(SetWorkAreaImageList)(THIS_ LONG param1, LONG param2, HIMAGELIST hImageList, HIMAGELIST* phOldImageList) PURE;

    /**
     * Retrieves the image list associated with a specified work area.
     *
     * \param [in] param1 The first LONG parameter, typically representing the work area index or identifier.
     * \param [in] param2 The second LONG parameter, usage depends on implementation context.
     * \param [out] phImageList Pointer to a HIMAGELIST that receives the handle to the image list.
     * \return HRESULT Returns S_OK on success, or an error code otherwise.
     */
    DECLSPEC_XFGVIRT(IListView, GetWorkAreaImageList)
    STDMETHOD(GetWorkAreaImageList)(THIS_ LONG param1, LONG param2, HIMAGELIST* phImageList) PURE;

    /**
     * Enables or disables the "Icon Bullying" feature.
     *
     * This method controls the behavior of icon manipulation within the interface.
     *
     * \param Mode Specifies the mode for icon bullying.
     * \return HRESULT indicating success or failure of the operation.
     */
    DECLSPEC_XFGVIRT(IListView, EnableIconBullying)
    STDMETHOD(EnableIconBullying)(THIS_ LONG Mode) PURE;

    /**
     * \brief Enables or configures specific quirks or compatibility modes.
     *
     * \param Flags A bitmask of flags specifying which quirks to enable.
     * \return HRESULT indicating success or failure of the operation.
     */
    DECLSPEC_XFGVIRT(IListView, EnableQuirks)
    STDMETHOD(EnableQuirks)(THIS_ ULONG Flags) PURE;

    END_INTERFACE
};

#undef INTERFACE

#define IListViewQueryInterface(This,riid,ppvObject) \
    ((This)->lpVtbl->QueryInterface(This,riid,ppvObject))
#define IListView_AddRef(This) \
    ((This)->lpVtbl->AddRef(This))
#define IListView_Release(This) \
    ((This)->lpVtbl->Release(This))
#define IListView_GetWindow(This, OleWindow, WindowHandle) \
    ((This)->lpVtbl->GetWindow(This, OleWindow, WindowHandle))
#define IListView_ContextSensitiveHelp(This, fEnterMode) \
    ((This)->lpVtbl->ContextSensitiveHelp(This, fEnterMode))
#define IListView_GetImageList(This, imageList, pHImageList) \
    ((This)->lpVtbl->GetImageList(This, imageList, pHImageList))
#define IListView_SetImageList(This, imageList, hNewImageList, pHOldImageList) \
    ((This)->lpVtbl->SetImageList(This, imageList, hNewImageList, pHOldImageList))
#define IListView_GetBackgroundColor(This, pColor) \
    ((This)->lpVtbl->GetBackgroundColor(This, pColor))
#define IListView_SetBackgroundColor(This, color) \
    ((This)->lpVtbl->SetBackgroundColor(This, color))
#define IListView_GetTextColor(This, pColor) \
    ((This)->lpVtbl->GetTextColor(This, pColor))
#define IListView_SetTextColor(This, color) \
    ((This)->lpVtbl->SetTextColor(This, color))
#define IListView_GetTextBackgroundColor(This, pColor) \
    ((This)->lpVtbl->GetTextBackgroundColor(This, pColor))
#define IListView_SetTextBackgroundColor(This, color) \
    ((This)->lpVtbl->SetTextBackgroundColor(This, color))
#define IListView_GetHotLightColor(This, pColor) \
    ((This)->lpVtbl->GetHotLightColor(This, pColor))
#define IListView_SetHotLightColor(This, color) \
    ((This)->lpVtbl->SetHotLightColor(This, color))
#define IListView_GetItemCount(This, pItemCount) \
    ((This)->lpVtbl->GetItemCount(This, pItemCount))
#define IListView_SetItemCount(This, itemCount, flags) \
    ((This)->lpVtbl->SetItemCount(This, itemCount, flags))
#define IListView_GetItem(This, pItem) \
    ((This)->lpVtbl->GetItem(This, pItem))
#define IListView_SetItem(This, pItem) \
    ((This)->lpVtbl->SetItem(This, pItem))
#define IListView_GetItemState(This, itemIndex, subItemIndex, mask, pState) \
    ((This)->lpVtbl->GetItemState(This, itemIndex, subItemIndex, mask, pState))
#define IListView_SetItemState(This, itemIndex, subItemIndex, mask, state) \
    ((This)->lpVtbl->SetItemState(This, itemIndex, subItemIndex, mask, state))
#define IListView_GetItemText(This, itemIndex, subItemIndex, pBuffer, bufferSize) \
    ((This)->lpVtbl->GetItemText(This, itemIndex, subItemIndex, pBuffer, bufferSize))
#define IListView_SetItemText(This, itemIndex, subItemIndex, pText) \
    ((This)->lpVtbl->SetItemText(This, itemIndex, subItemIndex, pText))
#define IListView_GetBackgroundImage(This, pBkImage) \
    ((This)->lpVtbl->GetBackgroundImage(This, pBkImage))
#define IListView_SetBackgroundImage(This, pBkImage) \
    ((This)->lpVtbl->SetBackgroundImage(This, pBkImage))
#define IListView_GetFocusedColumn(This, pColumnIndex) \
    ((This)->lpVtbl->GetFocusedColumn(This, pColumnIndex))
#define IListView_SetSelectionFlags(This, mask, flags) \
    ((This)->lpVtbl->SetSelectionFlags(This, mask, flags))
#define IListView_GetSelectedColumn(This, pColumnIndex) \
    ((This)->lpVtbl->GetSelectedColumn(This, pColumnIndex))
#define IListView_SetSelectedColumn(This, columnIndex) \
    ((This)->lpVtbl->SetSelectedColumn(This, columnIndex))
#define IListView_GetView(This, pView) \
    ((This)->lpVtbl->GetView(This, pView))
#define IListView_SetView(This, view) \
    ((This)->lpVtbl->SetView(This, view))
#define IListView_InsertItem(This, pItem, pItemIndex) \
    ((This)->lpVtbl->InsertItem(This, pItem, pItemIndex))
#define IListView_DeleteItem(This, itemIndex) \
    ((This)->lpVtbl->DeleteItem(This, itemIndex))
#define IListView_DeleteAllItems(This) \
    ((This)->lpVtbl->DeleteAllItems(This))
#define IListView_UpdateItem(This, itemIndex) \
    ((This)->lpVtbl->UpdateItem(This, itemIndex))
#define IListView_GetItemRect(This, itemIndex, rectangleType, pRectangle) \
    ((This)->lpVtbl->GetItemRect(This, itemIndex, rectangleType, pRectangle))
#define IListView_GetSubItemRect(This, itemIndex, subItemIndex, rectangleType, pRectangle) \
    ((This)->lpVtbl->GetSubItemRect(This, itemIndex, subItemIndex, rectangleType, pRectangle))
#define IListView_HitTestSubItem(This, pHitTestData) \
    ((This)->lpVtbl->HitTestSubItem(This, pHitTestData))
#define IListView_GetIncrSearchString(This, pBuffer, bufferSize, pCopiedChars) \
    ((This)->lpVtbl->GetIncrSearchString(This, pBuffer, bufferSize, pCopiedChars))
#define IListView_GetItemSpacing(This, smallIconView, pHorizontalSpacing, pVerticalSpacing) \
    ((This)->lpVtbl->GetItemSpacing(This, smallIconView, pHorizontalSpacing, pVerticalSpacing))
#define IListView_SetIconSpacing(This, horizontalSpacing, verticalSpacing, pHorizontalSpacing, pVerticalSpacing) \
    ((This)->lpVtbl->SetIconSpacing(This, horizontalSpacing, verticalSpacing, pHorizontalSpacing, pVerticalSpacing))
#define IListView_GetNextItem(This, itemIndex, flags, pNextItemIndex) \
    ((This)->lpVtbl->GetNextItem(This, itemIndex, flags, pNextItemIndex))
#define IListView_FindItem(This, startItemIndex, pFindInfo, pFoundItemIndex) \
    ((This)->lpVtbl->FindItem(This, startItemIndex, pFindInfo, pFoundItemIndex))
#define IListView_GetSelectionMark(This, pSelectionMark) \
    ((This)->lpVtbl->GetSelectionMark(This, pSelectionMark))
#define IListView_SetSelectionMark(This, newSelectionMark, pOldSelectionMark) \
    ((This)->lpVtbl->SetSelectionMark(This, newSelectionMark, pOldSelectionMark))
#define IListView_GetItemPosition(This, itemIndex, pPosition) \
    ((This)->lpVtbl->GetItemPosition(This, itemIndex, pPosition))
#define IListView_SetItemPosition(This, itemIndex, pPosition) \
    ((This)->lpVtbl->SetItemPosition(This, itemIndex, pPosition))
#define IListView_ScrollView(This, horizontalScrollDistance, verticalScrollDistance) \
    ((This)->lpVtbl->ScrollView(This, horizontalScrollDistance, verticalScrollDistance))
#define IListView_EnsureItemVisible(This, itemIndex, partialOk) \
    ((This)->lpVtbl->EnsureItemVisible(This, itemIndex, partialOk))
#define IListView_EnsureSubItemVisible(This, itemIndex, subItemIndex) \
    ((This)->lpVtbl->EnsureSubItemVisible(This, itemIndex, subItemIndex))
#define IListView_EditSubItem(This, itemIndex, subItemIndex) \
    ((This)->lpVtbl->EditSubItem(This, itemIndex, subItemIndex))
#define IListView_RedrawItems(This, firstItemIndex, lastItemIndex) \
    ((This)->lpVtbl->RedrawItems(This, firstItemIndex, lastItemIndex))
#define IListView_ArrangeItems(This, mode) \
    ((This)->lpVtbl->ArrangeItems(This, mode))
#define IListView_RecomputeItems(This, unknown) \
    ((This)->lpVtbl->RecomputeItems(This, unknown))
#define IListView_GetEditControl(This, EditWindowHandle) \
    ((This)->lpVtbl->GetEditControl(This, EditWindowHandle))
#define IListView_EditLabel(This, itemIndex, initialEditText, EditWindowHandle) \
    ((This)->lpVtbl->EditLabel(This, itemIndex, initialEditText, EditWindowHandle))
#define IListView_EditGroupLabel(This, groupIndex) \
    ((This)->lpVtbl->EditGroupLabel(This, groupIndex))
#define IListView_CancelEditLabel(This) \
    ((This)->lpVtbl->CancelEditLabel(This))
#define IListView_GetEditItem(This, itemIndex, subItemIndex) \
    ((This)->lpVtbl->GetEditItem(This, itemIndex, subItemIndex))
#define IListView_HitTest(This, pHitTestData) \
    ((This)->lpVtbl->HitTest(This, pHitTestData))
#define IListView_GetStringWidth(This, pString, pWidth) \
    ((This)->lpVtbl->GetStringWidth(This, pString, pWidth))
#define IListView_GetColumn(This, columnIndex, pColumn) \
    ((This)->lpVtbl->GetColumn(This, columnIndex, pColumn))
#define IListView_SetColumn(This, columnIndex, pColumn) \
    ((This)->lpVtbl->SetColumn(This, columnIndex, pColumn))
#define IListView_GetColumnOrderArray(This, numberOfColumns, pColumns) \
    ((This)->lpVtbl->GetColumnOrderArray(This, numberOfColumns, pColumns))
#define IListView_SetColumnOrderArray(This, numberOfColumns, pColumns) \
    ((This)->lpVtbl->SetColumnOrderArray(This, numberOfColumns, pColumns))
#define IListView_GetHeaderControl(This, HeaderWindowHandle) \
    ((This)->lpVtbl->GetHeaderControl(This, HeaderWindowHandle))
#define IListView_InsertColumn(This, insertAt, pColumn, pColumnIndex) \
    ((This)->lpVtbl->InsertColumn(This, insertAt, pColumn, pColumnIndex))
#define IListView_DeleteColumn(This, columnIndex) \
    ((This)->lpVtbl->DeleteColumn(This, columnIndex))
#define IListView_CreateDragImage(This, itemIndex, pUpperLeft, pHImageList) \
    ((This)->lpVtbl->CreateDragImage(This, itemIndex, pUpperLeft, pHImageList))
#define IListView_GetViewRect(This, pRectangle) \
    ((This)->lpVtbl->GetViewRect(This, pRectangle))
#define IListView_GetClientRect(This, unknown, pClientRectangle) \
    ((This)->lpVtbl->GetClientRect(This, unknown, pClientRectangle))
#define IListView_GetColumnWidth(This, columnIndex, pWidth) \
    ((This)->lpVtbl->GetColumnWidth(This, columnIndex, pWidth))
#define IListView_SetColumnWidth(This, columnIndex, width) \
    ((This)->lpVtbl->SetColumnWidth(This, columnIndex, width))
#define IListView_GetCallbackMask(This, pMask) \
    ((This)->lpVtbl->GetCallbackMask(This, pMask))
#define IListView_SetCallbackMask(This, mask) \
    ((This)->lpVtbl->SetCallbackMask(This, mask))
#define IListView_GetTopIndex(This, pTopIndex) \
    ((This)->lpVtbl->GetTopIndex(This, pTopIndex))
#define IListView_GetCountPerPage(This, pCountPerPage) \
    ((This)->lpVtbl->GetCountPerPage(This, pCountPerPage))
#define IListView_GetOrigin(This, pOrigin) \
    ((This)->lpVtbl->GetOrigin(This, pOrigin))
#define IListView_GetSelectedCount(This, pSelectedCount) \
    ((This)->lpVtbl->GetSelectedCount(This, pSelectedCount))
#define IListView_SortItems(This, SortingByIndex, lParam, pComparisonFunction) \
    ((This)->lpVtbl->SortItems(This, SortingByIndex, lParam, pComparisonFunction))
#define IListView_GetExtendedStyle(This, pStyle) \
    ((This)->lpVtbl->GetExtendedStyle(This, pStyle))
#define IListView_SetExtendedStyle(This, mask, style, pOldStyle) \
    ((This)->lpVtbl->SetExtendedStyle(This, mask, style, pOldStyle))
#define IListView_GetHoverTime(This, pTime) \
    ((This)->lpVtbl->GetHoverTime(This, pTime))
#define IListView_SetHoverTime(This, time, pOldSetting) \
    ((This)->lpVtbl->SetHoverTime(This, time, pOldSetting))
#define IListView_GetToolTip(This, ToolTipWindowHandle) \
    ((This)->lpVtbl->GetToolTip(This, ToolTipWindowHandle))
#define IListView_SetToolTip(This, ToolTipWindowHandle, OldToolTipWindowHandle) \
    ((This)->lpVtbl->SetToolTip(This, ToolTipWindowHandle, OldToolTipWindowHandle))
#define IListView_GetHotItem(This, pHotItem) \
    ((This)->lpVtbl->GetHotItem(This, pHotItem))
#define IListView_SetHotItem(This, newHotItem, pOldHotItem) \
    ((This)->lpVtbl->SetHotItem(This, newHotItem, pOldHotItem))
#define IListView_GetHotCursor(This, pHCursor) \
    ((This)->lpVtbl->GetHotCursor(This, pHCursor))
#define IListView_SetHotCursor(This, hCursor, pHOldCursor) \
    ((This)->lpVtbl->SetHotCursor(This, hCursor, pHOldCursor))
#define IListView_ApproximateViewRect(This, itemCount, pWidth, pHeight) \
    ((This)->lpVtbl->ApproximateViewRect(This, itemCount, pWidth, pHeight))
#define IListView_SetRangeObject(This, unknown, pObject) \
    ((This)->lpVtbl->SetRangeObject(This, unknown, pObject))
#define IListView_GetWorkAreas(This, numberOfWorkAreas, pWorkAreas) \
    ((This)->lpVtbl->GetWorkAreas(This, numberOfWorkAreas, pWorkAreas))
#define IListView_SetWorkAreas(This, numberOfWorkAreas, pWorkAreas) \
    ((This)->lpVtbl->SetWorkAreas(This, numberOfWorkAreas, pWorkAreas))
#define IListView_GetWorkAreaCount(This, pNumberOfWorkAreas) \
    ((This)->lpVtbl->GetWorkAreaCount(This, pNumberOfWorkAreas))
#define IListView_ResetEmptyText(This) \
    ((This)->lpVtbl->ResetEmptyText(This))
#define IListView_EnableGroupView(This, enable) \
    ((This)->lpVtbl->EnableGroupView(This, enable))
#define IListView_IsGroupViewEnabled(This, pIsEnabled) \
    ((This)->lpVtbl->IsGroupViewEnabled(This, pIsEnabled))
#define IListView_SortGroups(This, pComparisonFunction, lParam) \
    ((This)->lpVtbl->SortGroups(This, pComparisonFunction, lParam))
#define IListView_GetGroupInfo(This, GetGroupInfoByIndex, GroupIDOrIndex, pGroup) \
    ((This)->lpVtbl->GetGroupInfo(This, GetGroupInfoByIndex, GroupIDOrIndex, pGroup))
#define IListView_SetGroupInfo(This, SetGroupInfoByIndex, GroupIDOrIndex, pGroup) \
    ((This)->lpVtbl->SetGroupInfo(This, SetGroupInfoByIndex, GroupIDOrIndex, pGroup))
#define IListView_GetGroupRect(This, GetGroupRectByIndex, GroupIDOrIndex, rectangleType, pRectangle) \
    ((This)->lpVtbl->GetGroupRect(This, GetGroupRectByIndex, GroupIDOrIndex, rectangleType, pRectangle))
#define IListView_GetGroupState(This, groupID, mask, pState) \
    ((This)->lpVtbl->GetGroupState(This, groupID, mask, pState))
#define IListView_HasGroup(This, groupID, pHasGroup) \
    ((This)->lpVtbl->HasGroup(This, groupID, pHasGroup))
#define IListView_InsertGroup(This, insertAt, pGroup, pGroupID) \
    ((This)->lpVtbl->InsertGroup(This, insertAt, pGroup, pGroupID))
#define IListView_RemoveGroup(This, groupID) \
    ((This)->lpVtbl->RemoveGroup(This, groupID))
#define IListView_InsertGroupSorted(This, pGroup, pGroupID) \
    ((This)->lpVtbl->InsertGroupSorted(This, pGroup, pGroupID))
#define IListView_GetGroupMetrics(This, pMetrics) \
    ((This)->lpVtbl->GetGroupMetrics(This, pMetrics))
#define IListView_SetGroupMetrics(This, pMetrics) \
    ((This)->lpVtbl->SetGroupMetrics(This, pMetrics))
#define IListView_RemoveAllGroups(This) \
    ((This)->lpVtbl->RemoveAllGroups(This))
#define IListView_GetFocusedGroup(This, pGroupID) \
    ((This)->lpVtbl->GetFocusedGroup(This, pGroupID))
#define IListView_GetGroupCount(This, pCount) \
    ((This)->lpVtbl->GetGroupCount(This, pCount))
#define IListView_SetOwnerDataCallback(This, pCallback) \
    ((This)->lpVtbl->SetOwnerDataCallback(This, pCallback))
#define IListView_GetTileViewInfo(This, pInfo) \
    ((This)->lpVtbl->GetTileViewInfo(This, pInfo))
#define IListView_SetTileViewInfo(This, pInfo) \
    ((This)->lpVtbl->SetTileViewInfo(This, pInfo))
#define IListView_GetTileInfo(This, pTileInfo) \
    ((This)->lpVtbl->GetTileInfo(This, pTileInfo))
#define IListView_SetTileInfo(This, pTileInfo) \
    ((This)->lpVtbl->SetTileInfo(This, pTileInfo))
#define IListView_GetInsertMark(This, pInsertMarkDetails) \
    ((This)->lpVtbl->GetInsertMark(This, pInsertMarkDetails))
#define IListView_SetInsertMark(This, pInsertMarkDetails) \
    ((This)->lpVtbl->SetInsertMark(This, pInsertMarkDetails))
#define IListView_GetInsertMarkRect(This, pInsertMarkRectangle) \
    ((This)->lpVtbl->GetInsertMarkRect(This, pInsertMarkRectangle))
#define IListView_GetInsertMarkColor(This, pColor) \
    ((This)->lpVtbl->GetInsertMarkColor(This, pColor))
#define IListView_SetInsertMarkColor(This, color, pOldColor) \
    ((This)->lpVtbl->SetInsertMarkColor(This, color, pOldColor))
#define IListView_HitTestInsertMark(This, pPoint, pInsertMarkDetails) \
    ((This)->lpVtbl->HitTestInsertMark(This, pPoint, pInsertMarkDetails))
#define IListView_SetInfoTip(This, pInfoTip) \
    ((This)->lpVtbl->SetInfoTip(This, pInfoTip))
#define IListView_GetOutlineColor(This, pColor) \
    ((This)->lpVtbl->GetOutlineColor(This, pColor))
#define IListView_SetOutlineColor(This, color, pOldColor) \
    ((This)->lpVtbl->SetOutlineColor(This, color, pOldColor))
#define IListView_GetFrozenItem(This, pItemIndex) \
    ((This)->lpVtbl->GetFrozenItem(This, pItemIndex))
#define IListView_SetFrozenItem(This, unknown1, unknown2) \
    ((This)->lpVtbl->SetFrozenItem(This, unknown1, unknown2))
#define IListView_GetFrozenSlot(This, pUnknown) \
    ((This)->lpVtbl->GetFrozenSlot(This, pUnknown))
#define IListView_SetFrozenSlot(This, unknown1, pUnknown2) \
    ((This)->lpVtbl->SetFrozenSlot(This, unknown1, pUnknown2))
#define IListView_GetViewMargin(This, pMargin) \
    ((This)->lpVtbl->GetViewMargin(This, pMargin))
#define IListView_SetViewMargin(This, pMargin) \
    ((This)->lpVtbl->SetViewMargin(This, pMargin))
#define IListView_SetKeyboardSelected(This, itemIndex) \
    ((This)->lpVtbl->SetKeyboardSelected(This, itemIndex))
#define IListView_MapIndexToId(This, itemIndex, pItemID) \
    ((This)->lpVtbl->MapIndexToId(This, itemIndex, pItemID))
#define IListView_MapIdToIndex(This, itemID, pItemIndex) \
    ((This)->lpVtbl->MapIdToIndex(This, itemID, pItemIndex))
#define IListView_IsItemVisible(This, itemIndex, pVisible) \
    ((This)->lpVtbl->IsItemVisible(This, itemIndex, pVisible))
#define IListView_EnableAlphaShadow(This, enable) \
    ((This)->lpVtbl->EnableAlphaShadow(This, enable))
#define IListView_GetGroupSubsetCount(This, pNumberOfRowsDisplayed) \
    ((This)->lpVtbl->GetGroupSubsetCount(This, pNumberOfRowsDisplayed))
#define IListView_SetGroupSubsetCount(This, numberOfRowsToDisplay) \
    ((This)->lpVtbl->SetGroupSubsetCount(This, numberOfRowsToDisplay))
#define IListView_GetVisibleSlotCount(This, pCount) \
    ((This)->lpVtbl->GetVisibleSlotCount(This, pCount))
#define IListView_GetColumnMargin(This, pMargin) \
    ((This)->lpVtbl->GetColumnMargin(This, pMargin))
#define IListView_SetSubItemCallback(This, pCallback) \
    ((This)->lpVtbl->SetSubItemCallback(This, pCallback))
#define IListView_GetVisibleItemRange(This, pFirstItem, pLastItem) \
    ((This)->lpVtbl->GetVisibleItemRange(This, pFirstItem, pLastItem))
#define IListView_SetTypeAheadFlags(This, mask, flags) \
    ((This)->lpVtbl->SetTypeAheadFlags(This, mask, flags))

#endif
