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

// Additional CLSIDs observed in DllGetClassObject
// {49EB6558-C09C-46DC-8668-1F848C290D0B}
DEFINE_GUID(CLSID_GUID_49EB6558_C09C_46DC_8668_1F848C290D0B, 0x49EB6558, 0xC09C, 0x46DC, 0x86, 0x68, 0x1F, 0x84, 0x8C, 0x29, 0x0D, 0x0B);
// {169A0691-8DF9-11D1-A1C4-00C04FD75D13}
DEFINE_GUID(CLSID_GUID_169A0691_8DF9_11D1_A1C4_00C04FD75D13, 0x169A0691, 0x8DF9, 0x11D1, 0xA1, 0xC4, 0x00, 0xC0, 0x4F, 0xD7, 0x5D, 0x13);
// {DD313E04-FEFF-11D1-8ECD-0000F87A470C}
DEFINE_GUID(CLSID_GUID_DD313E04_FEFF_11D1_8ECD_0000F87A470C, 0xDD313E04, 0xFEFF, 0x11D1, 0x8E, 0xCD, 0x00, 0x00, 0xF8, 0x7A, 0x47, 0x0C);
// {E9711A2F-350F-4EC1-8EBD-21245A8B9376}
DEFINE_GUID(CLSID_GUID_E9711A2F_350F_4EC1_8EBD_21245A8B9376, 0xE9711A2F, 0x350F, 0x4EC1, 0x8E, 0xBD, 0x21, 0x24, 0x5A, 0x8B, 0x93, 0x76);
// {6D48E7F7-8ECD-404C-8E30-81C49E8E36EE}
DEFINE_GUID(CLSID_GUID_6D48E7F7_8ECD_404C_8E30_81C49E8E36EE, 0x6D48E7F7, 0x8ECD, 0x404C, 0x8E, 0x30, 0x81, 0xC4, 0x9E, 0x8E, 0x36, 0xEE);
// {E0DF7408-44FF-47D8-BE3B-79729980CAD8}
DEFINE_GUID(CLSID_GUID_E0DF7408_44FF_47D8_BE3B_79729980CAD8, 0xE0DF7408, 0x44FF, 0x47D8, 0xBE, 0x3B, 0x79, 0x72, 0x99, 0x80, 0xCA, 0xD8);

typedef enum tagPROPDESC_CONTROL_TYPE
{
    PROP_DESC_CONTROL_TYPE_DISPLAY = 0,
    PROP_DESC_CONTROL_TYPE_EDIT = 1,
    PROP_DESC_CONTROL_TYPE_CALENDAR = 2,
    PROP_DESC_CONTROL_TYPE_TEXTBOX = 3
} PROPCTL_CONTROL_TYPE;

_Enum_is_bitflag_
typedef enum _LV_SUBITEM_EDIT_MODE
{
    LV_SUBITEM_EDIT_MODE_NONE = 0x00000000,
    LV_SUBITEM_EDIT_MODE_BEGIN_EDIT = 0x00000001,
    LV_SUBITEM_EDIT_MODE_COMMIT = 0x00000002,
    LV_SUBITEM_EDIT_MODE_CANCEL = 0x00000004
} LV_SUBITEM_EDIT_MODE;
DEFINE_ENUM_FLAG_OPERATORS(LV_SUBITEM_EDIT_MODE);

_Enum_is_bitflag_
typedef enum _LV_PROPERTY_CONTROL_FLAGS
{
    LV_PROPERTY_CONTROL_FLAG_NONE = 0x00000000,
    LV_PROPERTY_CONTROL_FLAG_VISIBLE = 0x00000001,
    LV_PROPERTY_CONTROL_FLAG_READONLY = 0x00000002,
    LV_PROPERTY_CONTROL_FLAG_HIGHLIGHT = 0x00000004
} LV_PROPERTY_CONTROL_FLAGS;
DEFINE_ENUM_FLAG_OPERATORS(LV_PROPERTY_CONTROL_FLAGS);

_Enum_is_bitflag_
typedef enum _LV_PROPERTY_CONTROL_FORMAT_FLAGS
{
    LV_PROPERTY_CONTROL_FORMAT_FLAG_NONE = 0x00000000,
    LV_PROPERTY_CONTROL_FORMAT_FLAG_PREFIX_NAME = 0x00000001,
    LV_PROPERTY_CONTROL_FORMAT_FLAG_FILENAME = 0x00000002,
    LV_PROPERTY_CONTROL_FORMAT_FLAG_ALWAYS_KB = 0x00000004
} LV_PROPERTY_CONTROL_FORMAT_FLAGS;
DEFINE_ENUM_FLAG_OPERATORS(LV_PROPERTY_CONTROL_FORMAT_FLAGS);

_Enum_is_bitflag_
typedef enum _LV_DRAWPCTL_WINDOWLESS_DRAW_MODE
{
    LV_DRAWPCTL_WINDOWLESS_DRAW_MODE_NONE = 0x00000000,
    LV_DRAWPCTL_WINDOWLESS_DRAW_MODE_HOT = 0x00000001
} LV_DRAWPCTL_WINDOWLESS_DRAW_MODE;
DEFINE_ENUM_FLAG_OPERATORS(LV_DRAWPCTL_WINDOWLESS_DRAW_MODE);

_Enum_is_bitflag_
typedef enum _LV_DRAWPCTL_TOOLTIP_FLAGS
{
    LV_DRAWPCTL_TOOLTIP_FLAG_NONE = 0x00000000,
    LV_DRAWPCTL_TOOLTIP_FLAG_CLIPPED = 0x00000001,
    LV_DRAWPCTL_TOOLTIP_FLAG_CUSTOM = 0x00000002
} LV_DRAWPCTL_TOOLTIP_FLAGS;
DEFINE_ENUM_FLAG_OPERATORS(LV_DRAWPCTL_TOOLTIP_FLAGS);

_Enum_is_bitflag_
typedef enum _LV_LISTVIEW_SELECTION_FLAGS
{
    LV_LISTVIEW_SELECTION_FLAG_NONE = 0x00000000,
    LV_LISTVIEW_SELECTION_FLAG_KEEP_SELECTION_VISIBLE = 0x00000001
} LV_LISTVIEW_SELECTION_FLAGS;
DEFINE_ENUM_FLAG_OPERATORS(LV_LISTVIEW_SELECTION_FLAGS);

_Enum_is_bitflag_
typedef enum _LV_LISTVIEW_TYPEAHEAD_FLAGS
{
    LV_LISTVIEW_TYPEAHEAD_FLAG_NONE = 0x00000000,
    LV_LISTVIEW_TYPEAHEAD_FLAG_ENABLED = 0x00000001,
    LV_LISTVIEW_TYPEAHEAD_FLAG_PREFIX_MATCH = 0x00000002
} LV_LISTVIEW_TYPEAHEAD_FLAGS;
DEFINE_ENUM_FLAG_OPERATORS(LV_LISTVIEW_TYPEAHEAD_FLAGS);

_Enum_is_bitflag_
typedef enum _LV_LISTVIEW_ICON_BULLYING_MODE
{
    LV_LISTVIEW_ICON_BULLYING_DISABLED = 0,
    LV_LISTVIEW_ICON_BULLYING_ENABLED = 1
} LV_LISTVIEW_ICON_BULLYING_MODE;
DEFINE_ENUM_FLAG_OPERATORS(LV_LISTVIEW_ICON_BULLYING_MODE);

_Enum_is_bitflag_
typedef enum _LV_LISTVIEW_QUIRKS_FLAGS
{
    LV_LISTVIEW_QUIRK_NONE = 0x00000000,
    LV_LISTVIEW_QUIRK_WORKAREAS = 0x00000001,
    LV_LISTVIEW_QUIRK_WORKAREAS_DPI = 0x00000002,
    LV_LISTVIEW_QUIRK_LEGACY_COMPAT = 0x00000004
} LV_LISTVIEW_QUIRKS_FLAGS;
DEFINE_ENUM_FLAG_OPERATORS(LV_LISTVIEW_QUIRKS_FLAGS);

_Enum_is_bitflag_
typedef enum _LV_PROPERTY_CONTROL_CREATE_FLAGS
{
    LV_PROPERTY_CONTROL_CREATE_FLAG_NONE = 0x00000000,
    LV_PROPERTY_CONTROL_CREATE_FLAG_VISIBLE = 0x00000001,
    LV_PROPERTY_CONTROL_CREATE_FLAG_TOOLTIP = 0x00000002,
    LV_PROPERTY_CONTROL_CREATE_FLAG_HIGHLIGHT = 0x00000004,
    LV_PROPERTY_CONTROL_CREATE_FLAG_ACTIVE = 0x00000008, // triggers v_CreateActiveWindow (in-place edit)
    LV_PROPERTY_CONTROL_CREATE_FLAG_TRACKING = 0x00000010 // mouse tracking / focus timer
} LV_PROPERTY_CONTROL_CREATE_FLAGS;
DEFINE_ENUM_FLAG_OPERATORS(LV_PROPERTY_CONTROL_CREATE_FLAGS);

_Enum_is_bitflag_
typedef enum _LV_LISTVIEW_SETITEMCOUNT_FLAGS
{
    LV_LISTVIEW_SETITEMCOUNT_FLAG_NONE = 0x00000000,
    LV_LISTVIEW_SETITEMCOUNT_FLAG_NOINVALIDATEALL = 0x00000001, // LVSICF_NOINVALIDATEALL
    LV_LISTVIEW_SETITEMCOUNT_FLAG_NOSCROLL = 0x00000002         // LVSICF_NOSCROLL
} LV_LISTVIEW_SETITEMCOUNT_FLAGS;
DEFINE_ENUM_FLAG_OPERATORS(LV_LISTVIEW_SETITEMCOUNT_FLAGS);

_Enum_is_bitflag_
typedef enum _LV_LISTVIEW_ITEM_STATE_FLAGS
{
    LV_LISTVIEW_ITEM_STATE_NONE = 0x00000000,
    LV_LISTVIEW_ITEM_STATE_SELECTED = 0x00000001,   // LVIS_SELECTED
    LV_LISTVIEW_ITEM_STATE_FOCUSED = 0x00000002,    // LVIS_FOCUSED
    LV_LISTVIEW_ITEM_STATE_CURRENT = 0x00000004,    // item is the current focused item
    LV_LISTVIEW_ITEM_STATE_CHECKED = 0x00000008,    // state image mask 0x2000 (checked)
    LV_LISTVIEW_ITEM_STATE_CHECKED2 = 0x00000010,   // state image mask 0x3000 (extended check state)
    LV_LISTVIEW_ITEM_STATE_ACTIVATED = 0x00000020   // item activation state
} LV_LISTVIEW_ITEM_STATE_FLAGS;
DEFINE_ENUM_FLAG_OPERATORS(LV_LISTVIEW_ITEM_STATE_FLAGS);

#undef INTERFACE
#define INTERFACE IOwnerDataCallback
#ifndef __IOwnerDataCallback_INTERFACE_DEFINED__
#define __IOwnerDataCallback_INTERFACE_DEFINED__
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
    STDMETHOD(QueryInterface)(THIS_ _In_ REFIID riid, _COM_Outptr_ void** ppvObject) PURE;

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
     * \param ItemIndex The zero-based index of the item.
     * \param pPosition Receives the item's position in view coordinates.
     * \return HRESULT indicating success or failure.
     */
    DECLSPEC_XFGVIRT(IOwnerDataCallback, GetItemPosition)
    STDMETHOD(GetItemPosition)(THIS_ _In_ LONG ItemIndex, _Out_ LPPOINT Position) PURE;

    /**
     * Sets the position of an item.
     *
     * \param ItemIndex The zero-based index of the item.
     * \param Position The desired item position in view coordinates.
     * \return HRESULT indicating success or failure.
     */
    DECLSPEC_XFGVIRT(IOwnerDataCallback, SetItemPosition)
    STDMETHOD(SetItemPosition)(THIS_ _In_ LONG ItemIndex, _In_ POINT Position) PURE;

    /**
     * Maps a group-wide index to the control-wide item index.
     *
     * \param GroupIndex Zero-based list view group index.
     * \param GroupWideItemIndex Zero-based index of the item within the group.
     * \param TotalItemIndex Receives the control-wide item index.
     * \return HRESULT indicating success or failure.
     */
    DECLSPEC_XFGVIRT(IOwnerDataCallback, GetItemInGroup)
    STDMETHOD(GetItemInGroup)(THIS_ _In_ LONG GroupIndex, _In_ LONG GroupWideItemIndex, _Out_ PLONG TotalItemIndex) PURE;

    /**
     * Retrieves the group for a specific occurrence of an item.
     *
     * \param ItemIndex Control-wide zero-based item index.
     * \param OccurrenceIndex Zero-based occurrence index of the item.
     * \param GroupIndex Receives the group's zero-based index.
     * \return HRESULT indicating success or failure.
     */
    DECLSPEC_XFGVIRT(IOwnerDataCallback, GetItemGroup)
    STDMETHOD(GetItemGroup)(THIS_ _In_ LONG ItemIndex, _In_ LONG OccurrenceIndex, _Out_ PLONG GroupIndex) PURE;

    /**
     * Retrieves how often an item occurs in the control.
     *
     * \param ItemIndex Control-wide zero-based item index.
     * \param OccurrenceCount Receives the number of occurrences.
     * \return HRESULT indicating success or failure.
     */
    DECLSPEC_XFGVIRT(IOwnerDataCallback, GetItemGroupCount)
    STDMETHOD(GetItemGroupCount)(THIS_ _In_ LONG ItemIndex, _Out_ PLONG OccurrenceCount) PURE;

    /**
     * Provides a cache hint for a range of items.
     *
     * \param firstItem First item to cache (group-wide index).
     * \param lastItem Last item to cache (group-wide index).
     * \return HRESULT indicating success or failure.
     */
    DECLSPEC_XFGVIRT(IOwnerDataCallback, OnCacheHint)
    STDMETHOD(OnCacheHint)(THIS_ _In_ LVITEMINDEX FirstItem, _In_ LVITEMINDEX LastItem) PURE;

    END_INTERFACE
};

#endif // __IOwnerDataCallback_INTERFACE_DEFINED__

#undef INTERFACE
#define INTERFACE ISubItemCallback
#ifndef __ISubItemCallback_INTERFACE_DEFINED__
#define __ISubItemCallback_INTERFACE_DEFINED__
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
    STDMETHOD(QueryInterface)(THIS_ _In_ REFIID riid, _COM_Outptr_ void** ppvObject) PURE;

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
     * \param SubItemIndex Zero-based subitem index.
     * \param Buffer Wide-character buffer to receive the title.
     * \param BufferSize Buffer size in characters.
     * \return HRESULT indicating success or failure.
     */
    DECLSPEC_XFGVIRT(ISubItemCallback, GetSubItemTitle)
    STDMETHOD(GetSubItemTitle)(THIS_ _In_ LONG SubItemIndex, _Out_writes_(BufferSize) PWSTR Buffer, _In_ LONG BufferSize) PURE;

    /**
     * Retrieves a control interface for a specific subitem.
     *
     * \param ItemIndex Zero-based item index.
     * \param SubItemIndex Zero-based subitem index.
     * \param RequiredInterface IID of the required control interface.
     * \param ppObject Receives the interface pointer for the subitem control.
     * \return HRESULT indicating success or failure.
     */
    DECLSPEC_XFGVIRT(ISubItemCallback, GetSubItemControl)
    STDMETHOD(GetSubItemControl)(THIS_ _In_ LONG ItemIndex, _In_ LONG SubItemIndex, _In_ REFIID RequiredInterface, _COM_Outptr_ void** ppObject) PURE; // IPropertyControl

    /**
     * Begins editing a subitem.
     *
     * \param ItemIndex Zero-based item index.
     * \param SubItemIndex Zero-based subitem index.
     * \param Mode Edit mode flags.
     * \param RequiredInterface IID of the required editing control interface.
     * \param ppObject Receives the interface pointer for the editing control.
     * \return HRESULT indicating success or failure.
     */
    DECLSPEC_XFGVIRT(ISubItemCallback, BeginSubItemEdit)
    STDMETHOD(BeginSubItemEdit)(THIS_ _In_ LONG ItemIndex, _In_ LONG SubItemIndex, _In_ LV_SUBITEM_EDIT_MODE Mode, _In_ REFIID RequiredInterface, _COM_Outptr_ void** ppObject) PURE; // IPropertyControl

    /**
     * Ends editing of a subitem.
     *
     * \param ItemIndex Zero-based item index.
     * \param SubItemIndex Zero-based subitem index.
     * \param Mode Edit mode flags.
     * \param pPropertyControl The property control used during editing.
     * \return HRESULT indicating success or failure.
     */
    DECLSPEC_XFGVIRT(ISubItemCallback, EndSubItemEdit)
    STDMETHOD(EndSubItemEdit)(THIS_ _In_ LONG ItemIndex, _In_ LONG SubItemIndex, _In_ LV_SUBITEM_EDIT_MODE Mode, _Inout_opt_ void** ppObject) PURE; // IPropertyControl

    /**
     * Begins editing a group.
     *
     * \param GroupIndex Zero-based group index.
     * \param RequiredInterface IID of the required editing control interface.
     * \param ppObject Receives the interface pointer for the editing control.
     * \return HRESULT indicating success or failure.
     */
    DECLSPEC_XFGVIRT(ISubItemCallback, BeginGroupEdit)
    STDMETHOD(BeginGroupEdit)(THIS_ _In_ LONG GroupIndex, _In_ REFIID RequiredInterface, _COM_Outptr_ void** ppObject) PURE; // IPropertyControl

    /**
     * Ends editing of a group.
     *
     * \param GroupIndex Zero-based group index.
     * \param Mode Edit mode flags.
     * \param pPropertyControl The property control used during editing.
     * \return HRESULT indicating success or failure.
     */
    DECLSPEC_XFGVIRT(ISubItemCallback, EndGroupEdit)
    STDMETHOD(EndGroupEdit)(THIS_ _In_ LONG GroupIndex, _In_ LV_SUBITEM_EDIT_MODE Mode, _Inout_opt_ void** ppObject) PURE; // IPropertyControl

    /**
     * Invokes a custom verb on a subitem.
     *
     * \param ItemIndex Zero-based item index.
     * \param Verb The verb to invoke (wide string).
     * \return HRESULT indicating success or failure.
     */
    DECLSPEC_XFGVIRT(ISubItemCallback, OnInvokeVerb)
    STDMETHOD(OnInvokeVerb)(THIS_ _In_ LONG ItemIndex, _In_ LPCWSTR Verb) PURE;

    END_INTERFACE
};

#endif // __ISubItemCallback_INTERFACE_DEFINED__

#undef INTERFACE
#define INTERFACE IPropertyValue
#ifndef __IPropertyValue_INTERFACE_DEFINED__
#define __IPropertyValue_INTERFACE_DEFINED__
DECLARE_INTERFACE_(IPropertyValue, IUnknown)
{
    BEGIN_INTERFACE

    //
    // IUnknown
    //

    DECLSPEC_XFGVIRT(IPropertyValue, QueryInterface)
    STDMETHOD(QueryInterface)(THIS_ _In_ REFIID riid, _COM_Outptr_ void** ppvObject) PURE;

    DECLSPEC_XFGVIRT(IPropertyValue, AddRef)
    STDMETHOD_(ULONG, AddRef)(THIS) PURE;

    DECLSPEC_XFGVIRT(IPropertyValue, Release)
    STDMETHOD_(ULONG, Release)(THIS) PURE;

    //
    // IPropertyValue
    //

    DECLSPEC_XFGVIRT(IPropertyValue, SetPropertyKey)
    STDMETHOD(SetPropertyKey)(THIS_ _In_ PROPERTYKEY const* PropertyKey) PURE;

    DECLSPEC_XFGVIRT(IPropertyValue, GetPropertyKey)
    STDMETHOD(GetPropertyKey)(THIS_ _Out_ PROPERTYKEY* PropertyKey) PURE;

    DECLSPEC_XFGVIRT(IPropertyValue, GetValue)
    STDMETHOD(GetValue)(THIS_ _Out_ PROPVARIANT* Value) PURE;

    DECLSPEC_XFGVIRT(IPropertyValue, InitValue)
    STDMETHOD(InitValue)(THIS_ _In_ PROPVARIANT const* Value) PURE;

    DECLSPEC_XFGVIRT(IPropertyValue, HasValue)
    STDMETHOD(HasValue)(THIS_ _In_ PROPVARIANT const* Value, _Out_ PLONG ValueIndex) PURE;

    DECLSPEC_XFGVIRT(IPropertyValue, InsertValue)
    STDMETHOD(InsertValue)(THIS_ _In_ LONG ValueIndex, _In_ PROPVARIANT const* Value) PURE;

    DECLSPEC_XFGVIRT(IPropertyValue, AppendValue)
    STDMETHOD(AppendValue)(THIS_ _In_ PROPVARIANT const* Value) PURE;

    DECLSPEC_XFGVIRT(IPropertyValue, DeleteValue)
    STDMETHOD(DeleteValue)(THIS_ _In_ PROPVARIANT const* Value) PURE;

    DECLSPEC_XFGVIRT(IPropertyValue, GetValueAt)
    STDMETHOD(GetValueAt)(THIS_ _In_ LONG ValueIndex, _Out_ PROPVARIANT* Value) PURE;

    DECLSPEC_XFGVIRT(IPropertyValue, GetCount)
    STDMETHOD(GetCount)(THIS_ _Out_ PLONG Count) PURE;

    END_INTERFACE
};

#endif // __IPropertyValue_INTERFACE_DEFINED__

#undef INTERFACE
#define INTERFACE IPropertyControlBase
#ifndef __IPropertyControlBase_INTERFACE_DEFINED__
#define __IPropertyControlBase_INTERFACE_DEFINED__
DECLARE_INTERFACE_(IPropertyControlBase, IUnknown)
{
    BEGIN_INTERFACE

    //
    // IUnknown
    //

    DECLSPEC_XFGVIRT(IPropertyControlBase, QueryInterface)
    STDMETHOD(QueryInterface)(THIS_ _In_ REFIID riid, _COM_Outptr_ void** ppvObject) PURE;

    DECLSPEC_XFGVIRT(IPropertyControlBase, AddRef)
    STDMETHOD_(ULONG, AddRef)(THIS) PURE;

    DECLSPEC_XFGVIRT(IPropertyControlBase, Release)
    STDMETHOD_(ULONG, Release)(THIS) PURE;

    //
    // IPropertyControlBase
    //

    DECLSPEC_XFGVIRT(IPropertyControlBase, Initialize)
    STDMETHOD(Initialize)(THIS_ _In_ IUnknown* Site, _In_ enum tagPROPDESC_CONTROL_TYPE Type) PURE;

    DECLSPEC_XFGVIRT(IPropertyControlBase, GetSize)
    STDMETHOD(GetSize)(THIS_ _In_ enum PROPCTL_RECT_TYPE RectType, _In_ HDC hdc, _In_opt_ SIZE const* Proposed, _Out_ SIZE* Size) PURE;

    DECLSPEC_XFGVIRT(IPropertyControlBase, SetWindowTheme)
    STDMETHOD(SetWindowTheme)(THIS_ _In_opt_ PCWSTR SubAppName, _In_opt_ PCWSTR SubIdList) PURE;

    DECLSPEC_XFGVIRT(IPropertyControlBase, SetFont)
    STDMETHOD(SetFont)(THIS_ _In_opt_ HFONT Font) PURE;

    DECLSPEC_XFGVIRT(IPropertyControlBase, SetForeColor)
    STDMETHOD(SetForeColor)(THIS_ _In_ COLORREF ColorRef) PURE;

    DECLSPEC_XFGVIRT(IPropertyControlBase, GetFlags)
    STDMETHOD(GetFlags)(THIS_ _Out_ LV_PROPERTY_CONTROL_FLAGS* Flags) PURE;

    DECLSPEC_XFGVIRT(IPropertyControlBase, SetFlags)
    STDMETHOD(SetFlags)(THIS_ _In_ LV_PROPERTY_CONTROL_FLAGS Mask, _In_ LV_PROPERTY_CONTROL_FLAGS Flags) PURE;

    DECLSPEC_XFGVIRT(IPropertyControlBase, AdjustWindowRectPCB)
    STDMETHOD(AdjustWindowRectPCB)(THIS_ _In_ HWND WindowHandle, _Out_ RECT* WindowRect, _In_opt_ RECT const* ClientRect, _In_ LONG Flags) PURE;

    DECLSPEC_XFGVIRT(IPropertyControlBase, SetValue)
    STDMETHOD(SetValue)(THIS_ _In_opt_ IUnknown* Value) PURE;

    DECLSPEC_XFGVIRT(IPropertyControlBase, InvokeDefaultAction)
    STDMETHOD(InvokeDefaultAction)(THIS) PURE;

    DECLSPEC_XFGVIRT(IPropertyControlBase, Destroy)
    STDMETHOD(Destroy)(THIS) PURE;

    DECLSPEC_XFGVIRT(IPropertyControlBase, SetFormatFlags)
    STDMETHOD(SetFormatFlags)(THIS_ _In_ LV_PROPERTY_CONTROL_FORMAT_FLAGS Flags) PURE;

    DECLSPEC_XFGVIRT(IPropertyControlBase, GetFormatFlags)
    STDMETHOD(GetFormatFlags)(THIS_ _Out_ LV_PROPERTY_CONTROL_FORMAT_FLAGS* Flags) PURE;

    END_INTERFACE
};

#endif // __IPropertyControlBase_INTERFACE_DEFINED__

#undef INTERFACE
#define INTERFACE IListViewFooterCallback
#ifndef __IListViewFooterCallback_INTERFACE_DEFINED__
#define __IListViewFooterCallback_INTERFACE_DEFINED__
DECLARE_INTERFACE_(IListViewFooterCallback, IUnknown)
{
    BEGIN_INTERFACE

    //
    // IUnknown
    //

    DECLSPEC_XFGVIRT(IListViewFooterCallback, QueryInterface)
    STDMETHOD(QueryInterface)(THIS_ _In_ REFIID riid, _COM_Outptr_ void** ppvObject) PURE;

    DECLSPEC_XFGVIRT(IListViewFooterCallback, AddRef)
    STDMETHOD_(ULONG, AddRef)(THIS) PURE;

    DECLSPEC_XFGVIRT(IListViewFooterCallback, Release)
    STDMETHOD_(ULONG, Release)(THIS) PURE;

    //
    // IListViewFooterCallback
    //

    DECLSPEC_XFGVIRT(IListViewFooterCallback, OnButtonClicked)
    STDMETHOD(OnButtonClicked)(THIS_ _In_ LONG ItemIndex, _In_ LPARAM ItemLParam, _Out_ PLONG RemoveFooter) PURE;

    DECLSPEC_XFGVIRT(IListViewFooterCallback, OnDestroyButton)
    STDMETHOD(OnDestroyButton)(THIS_ _In_ LONG ItemIndex, _In_ LPARAM ItemLParam) PURE;

    END_INTERFACE
};

#endif // __IListViewFooterCallback_INTERFACE_DEFINED__

#undef INTERFACE
#define INTERFACE IListViewFooter
#ifndef __IListViewFooter_INTERFACE_DEFINED__
#define __IListViewFooter_INTERFACE_DEFINED__
DECLARE_INTERFACE_(IListViewFooter, IUnknown)
{
    BEGIN_INTERFACE

    //
    // IUnknown
    //

    DECLSPEC_XFGVIRT(IListViewFooter, QueryInterface)
    STDMETHOD(QueryInterface)(THIS_ _In_ REFIID riid, _COM_Outptr_ void** ppvObject) PURE;

    DECLSPEC_XFGVIRT(IListViewFooter, AddRef)
    STDMETHOD_(ULONG, AddRef)(THIS) PURE;

    DECLSPEC_XFGVIRT(IListViewFooter, Release)
    STDMETHOD_(ULONG, Release)(THIS) PURE;

    //
    // IListViewFooter
    //

    DECLSPEC_XFGVIRT(IListViewFooter, IsVisible)
    STDMETHOD(IsVisible)(THIS_ _Out_ PLONG Visible) PURE;

    DECLSPEC_XFGVIRT(IListViewFooter, GetFooterFocus)
    STDMETHOD(GetFooterFocus)(THIS_ _Out_ PLONG ItemIndex) PURE;

    DECLSPEC_XFGVIRT(IListViewFooter, SetFooterFocus)
    STDMETHOD(SetFooterFocus)(THIS_ _In_ LONG ItemIndex) PURE;

    DECLSPEC_XFGVIRT(IListViewFooter, SetIntroText)
    STDMETHOD(SetIntroText)(THIS_ _In_ PCWSTR Text) PURE;

    DECLSPEC_XFGVIRT(IListViewFooter, Show)
    STDMETHOD(Show)(THIS_ _In_ BOOL ShowFooter) PURE;

    DECLSPEC_XFGVIRT(IListViewFooter, RemoveAllButtons)
    STDMETHOD(RemoveAllButtons)(THIS) PURE;

    DECLSPEC_XFGVIRT(IListViewFooter, InsertButton)
    STDMETHOD(InsertButton)(THIS_ _In_ LONG InsertAt, _In_ PCWSTR ButtonText, _In_opt_ PCWSTR SubText, _In_ LONG ImageIndex, _In_opt_ IUnknown* Context) PURE;

    END_INTERFACE
};

#endif // __IListViewFooter_INTERFACE_DEFINED__

#undef INTERFACE
#define INTERFACE IDrawPropertyControl
#ifndef __IDrawPropertyControl_INTERFACE_DEFINED__
#define __IDrawPropertyControl_INTERFACE_DEFINED__
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
    STDMETHOD(QueryInterface)(THIS_ _In_ REFIID riid, _COM_Outptr_ void** ppvObject) PURE;

    DECLSPEC_XFGVIRT(IDrawPropertyControl, AddRef)
    STDMETHOD_(ULONG, AddRef)(THIS) PURE;

    DECLSPEC_XFGVIRT(IDrawPropertyControl, Release)
    STDMETHOD_(ULONG, Release)(THIS) PURE;

    //
    // IPropertyControlBase
    //

    DECLSPEC_XFGVIRT(IDrawPropertyControl, Initialize)
    STDMETHOD(Initialize)(THIS_ _In_ IUnknown* Site, _In_ enum tagPROPDESC_CONTROL_TYPE Type) PURE; // 2/calendar, 3/textbox

    DECLSPEC_XFGVIRT(IDrawPropertyControl, GetSize)
    STDMETHOD(GetSize)(THIS_ _In_ enum PROPCTL_RECT_TYPE RectType, _In_ HDC DeviceContext, _In_opt_ SIZE const* ProposedSize, _Out_ SIZE* Size) PURE;

    DECLSPEC_XFGVIRT(IDrawPropertyControl, SetWindowTheme)
    STDMETHOD(SetWindowTheme)(THIS_ _In_opt_ PCWSTR SubAppName, _In_opt_ PCWSTR SubIdList) PURE;

    DECLSPEC_XFGVIRT(IDrawPropertyControl, SetFont)
    STDMETHOD(SetFont)(THIS_ _In_opt_ HFONT Font) PURE;

    DECLSPEC_XFGVIRT(IDrawPropertyControl, SetForeColor)
    STDMETHOD(SetForeColor)(THIS_ _In_ ULONG ColorRef) PURE;

    DECLSPEC_XFGVIRT(IDrawPropertyControl, GetFlags)
    STDMETHOD(GetFlags)(THIS_ _Out_ PLONG Flags) PURE;

    DECLSPEC_XFGVIRT(IDrawPropertyControl, SetFlags)
    STDMETHOD(SetFlags)(THIS_ _In_ LV_PROPERTY_CONTROL_FLAGS Mask, _In_ LV_PROPERTY_CONTROL_FLAGS Flags) PURE;

    DECLSPEC_XFGVIRT(IDrawPropertyControl, AdjustWindowRectPCB)
    STDMETHOD(AdjustWindowRectPCB)(THIS_ _In_ HWND WindowHandle, _Out_ RECT* WindowRect, _In_opt_ RECT const* ClientRect, _In_ LONG Flags) PURE;

    DECLSPEC_XFGVIRT(IDrawPropertyControl, SetValue)
    STDMETHOD(SetValue)(THIS_ _In_opt_ IUnknown* Value) PURE;

    DECLSPEC_XFGVIRT(IDrawPropertyControl, InvokeDefaultAction)
    STDMETHOD(InvokeDefaultAction)(THIS) PURE;

    DECLSPEC_XFGVIRT(IDrawPropertyControl, Destroy)
    STDMETHOD(Destroy)(THIS) PURE;

    DECLSPEC_XFGVIRT(IDrawPropertyControl, SetFormatFlags)
    STDMETHOD(SetFormatFlags)(THIS_ _In_ LV_PROPERTY_CONTROL_FORMAT_FLAGS Flags) PURE;

    DECLSPEC_XFGVIRT(IDrawPropertyControl, GetFormatFlags)
    STDMETHOD(GetFormatFlags)(THIS_ _Out_ LV_PROPERTY_CONTROL_FORMAT_FLAGS* Flags) PURE;

    //
    // IDrawPropertyControl
    //

    DECLSPEC_XFGVIRT(IDrawPropertyControl, GetDrawFlags)
    STDMETHOD(GetDrawFlags)(THIS_ _Out_ PLONG Flags) PURE;

    DECLSPEC_XFGVIRT(IDrawPropertyControl, WindowlessDraw)
    STDMETHOD(WindowlessDraw)(THIS_ _In_ HDC DeviceContext, _In_ RECT DrawingRectangle, _In_ LV_DRAWPCTL_WINDOWLESS_DRAW_MODE DrawMode) PURE;

    DECLSPEC_XFGVIRT(IDrawPropertyControl, HasVisibleContent)
    STDMETHOD(HasVisibleContent)(THIS) PURE;

    DECLSPEC_XFGVIRT(IDrawPropertyControl, GetDisplayText)
    STDMETHOD(GetDisplayText)(THIS_ _Outptr_ PWSTR* Text) PURE;

    DECLSPEC_XFGVIRT(IDrawPropertyControl, GetTooltipInfo)
    STDMETHOD(GetTooltipInfo)(THIS_ _In_ HDC DeviceContext, _In_opt_ SIZE const* ProposedSize, _Out_ PLONG TooltipFlags) PURE;

    END_INTERFACE
};

#endif // __IDrawPropertyControl_INTERFACE_DEFINED__

#undef INTERFACE
#define INTERFACE IDrawPropertyControlWin10
#ifndef __IDrawPropertyControlWin10_INTERFACE_DEFINED__
#define __IDrawPropertyControlWin10_INTERFACE_DEFINED__
DECLARE_INTERFACE_(IDrawPropertyControlWin10, IDrawPropertyControl)
{
    BEGIN_INTERFACE

    //
    // IUnknown
    //

    DECLSPEC_XFGVIRT(IDrawPropertyControlWin10, QueryInterface)
    STDMETHOD(QueryInterface)(THIS_ _In_ REFIID riid, _COM_Outptr_ void** ppvObject) PURE;

    DECLSPEC_XFGVIRT(IDrawPropertyControlWin10, AddRef)
    STDMETHOD_(ULONG, AddRef)(THIS) PURE;

    DECLSPEC_XFGVIRT(IDrawPropertyControlWin10, Release)
    STDMETHOD_(ULONG, Release)(THIS) PURE;

    //
    // IPropertyControlBase
    //

    DECLSPEC_XFGVIRT(IDrawPropertyControlWin10, Initialize)
    STDMETHOD(Initialize)(THIS_ _In_ IUnknown* Site, _In_ enum tagPROPDESC_CONTROL_TYPE Type) PURE;

    DECLSPEC_XFGVIRT(IDrawPropertyControlWin10, GetSize)
    STDMETHOD(GetSize)(THIS_ _In_ enum PROPCTL_RECT_TYPE RectType, _In_ HDC DeviceContext, _In_opt_ SIZE const* ProposedSize, _Out_ SIZE* Size) PURE;

    DECLSPEC_XFGVIRT(IDrawPropertyControlWin10, SetWindowTheme)
    STDMETHOD(SetWindowTheme)(THIS_ _In_opt_ PCWSTR SubAppName, _In_opt_ PCWSTR SubIdList) PURE;

    DECLSPEC_XFGVIRT(IDrawPropertyControlWin10, SetFont)
    STDMETHOD(SetFont)(THIS_ _In_opt_ HFONT Font) PURE;

    DECLSPEC_XFGVIRT(IDrawPropertyControlWin10, SetForeColor)
    STDMETHOD(SetForeColor)(THIS_ _In_ ULONG ColorRef) PURE;

    DECLSPEC_XFGVIRT(IDrawPropertyControlWin10, GetFlags)
    STDMETHOD(GetFlags)(THIS_ _Out_ PLONG Flags) PURE;

    DECLSPEC_XFGVIRT(IDrawPropertyControlWin10, SetFlags)
    STDMETHOD(SetFlags)(THIS_ _In_ LV_PROPERTY_CONTROL_FLAGS Mask, _In_ LV_PROPERTY_CONTROL_FLAGS Flags) PURE;

    DECLSPEC_XFGVIRT(IDrawPropertyControlWin10, AdjustWindowRectPCB)
    STDMETHOD(AdjustWindowRectPCB)(THIS_ _In_ HWND WindowHandle, _Out_ RECT* WindowRect, _In_opt_ RECT const* ClientRect, _In_ LONG Flags) PURE;

    DECLSPEC_XFGVIRT(IDrawPropertyControlWin10, SetValue)
    STDMETHOD(SetValue)(THIS_ _In_opt_ IUnknown* Value) PURE;

    DECLSPEC_XFGVIRT(IDrawPropertyControlWin10, InvokeDefaultAction)
    STDMETHOD(InvokeDefaultAction)(THIS) PURE;

    DECLSPEC_XFGVIRT(IDrawPropertyControlWin10, Destroy)
    STDMETHOD(Destroy)(THIS) PURE;

    DECLSPEC_XFGVIRT(IDrawPropertyControlWin10, SetFormatFlags)
    STDMETHOD(SetFormatFlags)(THIS_ _In_ LV_PROPERTY_CONTROL_FORMAT_FLAGS Flags) PURE;

    DECLSPEC_XFGVIRT(IDrawPropertyControlWin10, GetFormatFlags)
    STDMETHOD(GetFormatFlags)(THIS_ _Out_ PLONG Flags) PURE;

    //
    // IDrawPropertyControl
    //

    DECLSPEC_XFGVIRT(IDrawPropertyControlWin10, GetDrawFlags)
    STDMETHOD(GetDrawFlags)(THIS_ _Out_ PLONG Flags) PURE;

    DECLSPEC_XFGVIRT(IDrawPropertyControlWin10, WindowlessDraw)
    STDMETHOD(WindowlessDraw)(THIS_ _In_ HDC DeviceContext, _In_ RECT DrawingRectangle, _In_ LV_DRAWPCTL_WINDOWLESS_DRAW_MODE DrawMode) PURE;

    DECLSPEC_XFGVIRT(IDrawPropertyControlWin10, HasVisibleContent)
    STDMETHOD(HasVisibleContent)(THIS) PURE;

    DECLSPEC_XFGVIRT(IDrawPropertyControlWin10, GetDisplayText)
    STDMETHOD(GetDisplayText)(THIS_ _Outptr_ PWSTR* Text) PURE;

    DECLSPEC_XFGVIRT(IDrawPropertyControlWin10, GetTooltipInfo)
    STDMETHOD(GetTooltipInfo)(THIS_ _In_ HDC DeviceContext, _In_opt_ SIZE const* ProposedSize, _Out_ PLONG TooltipFlags) PURE;

    //
    // IDrawPropertyControlWin10
    //

    DECLSPEC_XFGVIRT(IDrawPropertyControlWin10, SetWindowlessParentWindow)
    STDMETHOD(SetWindowlessParentWindow)(THIS_ _In_ HWND WindowHandle) PURE;

    // Win11

    DECLSPEC_XFGVIRT(IDrawPropertyControlWin10, OnDPIChanged)
    STDMETHOD(OnDPIChanged)(THIS) PURE;

    END_INTERFACE
};

#endif // __IDrawPropertyControlWin10_INTERFACE_DEFINED__

#undef INTERFACE
#define INTERFACE IPropertyControl
#ifndef __IPropertyControl_INTERFACE_DEFINED__
#define __IPropertyControl_INTERFACE_DEFINED__
DECLARE_INTERFACE_(IPropertyControl, IUnknown)
{
    BEGIN_INTERFACE

    //
    // IUnknown
    //

    DECLSPEC_XFGVIRT(IPropertyControl, QueryInterface)
    STDMETHOD(QueryInterface)(THIS_ _In_ REFIID riid, _COM_Outptr_ void** ppvObject) PURE;

    DECLSPEC_XFGVIRT(IPropertyControl, AddRef)
    STDMETHOD_(ULONG, AddRef)(THIS) PURE;

    DECLSPEC_XFGVIRT(IPropertyControl, Release)
    STDMETHOD_(ULONG, Release)(THIS) PURE;

    //
    // IPropertyControl
    //

    DECLSPEC_XFGVIRT(IPropertyControl, GetValue)
    STDMETHOD(GetValue)(THIS_ _In_ REFIID riid, _COM_Outptr_ void** ppvObject) PURE;

    DECLSPEC_XFGVIRT(IPropertyControl, Create)
    STDMETHOD(Create)(THIS_ _In_ HWND ParentWindowHandle, _In_ RECT const* ItemRectangle, _In_opt_ RECT const* ClipRectangle, _In_ LV_PROPERTY_CONTROL_CREATE_FLAGS CreateFlags) PURE;

    DECLSPEC_XFGVIRT(IPropertyControl, SetPosition)
    STDMETHOD(SetPosition)(THIS_ _In_ RECT const* ItemRectangle, _In_opt_ RECT const* ClipRectangle) PURE;

    DECLSPEC_XFGVIRT(IPropertyControl, IsModified)
    STDMETHOD(IsModified)(THIS_ _Out_ PBOOL Modified) PURE;

    DECLSPEC_XFGVIRT(IPropertyControl, SetModified)
    STDMETHOD(SetModified)(THIS_ _In_ BOOL Modified) PURE;

    DECLSPEC_XFGVIRT(IPropertyControl, ValidationFailed)
    STDMETHOD(ValidationFailed)(THIS_ _In_ PCWSTR ValidationText) PURE;

    DECLSPEC_XFGVIRT(IPropertyControl, GetState)
    STDMETHOD(GetState)(THIS_ _Out_ PULONG State) PURE;

    END_INTERFACE
};

#endif // __IPropertyControl_INTERFACE_DEFINED__

#undef INTERFACE
#define INTERFACE IListView
#ifndef __IListView_INTERFACE_DEFINED__
#define __IListView_INTERFACE_DEFINED__
DECLARE_INTERFACE_(IListView, IUnknown) // real name is IListView2
{
    BEGIN_INTERFACE

    //
    // IUnknown
    //

    DECLSPEC_XFGVIRT(IListView, QueryInterface)
    STDMETHOD(QueryInterface)(THIS_ _In_ REFIID riid, _COM_Outptr_ void** ppvObject) PURE;

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
    STDMETHOD(GetImageList)(THIS_ _In_ LONG ImageListType, _Out_ HIMAGELIST* ImageList) PURE;

    /**
     * Sets a new image list for the list view and optionally retrieves the handle to the previous image list.
     *
     * \param ImageListType The type of image list to set (e.g., normal, small, state).
     * \param NewImageList Handle to the new image list to associate with the list view.
     * \param OldImageList Pointer to a variable that receives the handle to the previous image list.
     * \return HRESULT indicating success or failure.
     */
    DECLSPEC_XFGVIRT(IListView, SetImageList)
    STDMETHOD(SetImageList)(THIS_ _In_ LONG ImageListType, _In_opt_ HIMAGELIST NewImageList, _Out_opt_ HIMAGELIST* OldImageList) PURE;

    /**
     * Retrieves the current background color of the list view.
     *
     * \param Color Pointer to a variable that receives the background color (COLORREF).
     * \return HRESULT indicating success or failure.
     */
    DECLSPEC_XFGVIRT(IListView, GetBackgroundColor)
    STDMETHOD(GetBackgroundColor)(THIS_ _Out_ COLORREF* Color) PURE;

    /**
     * Sets the background color of the list view.
     *
     * \param color The new background color (COLORREF) to set.
     * \return HRESULT indicating success or failure.
     */
    DECLSPEC_XFGVIRT(IListView, SetBackgroundColor)
    STDMETHOD(SetBackgroundColor)(THIS_ _In_ COLORREF Color) PURE;

    /**
     * \brief Retrieves the current text color used by the list view.
     *
     * \param[out] Color Pointer to a COLORREF variable that receives the current text color.
     * \return HRESULT indicating success or failure.
     */
    DECLSPEC_XFGVIRT(IListView, GetForegroundColor)
    STDMETHOD(GetForegroundColor)(THIS_ _Out_ COLORREF* Color) PURE;

    /**
     * \brief Sets the foreground (text) color for the list view.
     *
     * \param[in] Color The COLORREF value specifying the new text color.
     * \return HRESULT indicating success or failure.
     */
    DECLSPEC_XFGVIRT(IListView, SetForegroundColor)
    STDMETHOD(SetForegroundColor)(THIS_ _In_ COLORREF Color) PURE;

    /**
     * \brief Retrieves the current background color used for the text in the list view.
     *
     * \param[out] Color Pointer to a COLORREF variable that receives the current text background color.
     * \return HRESULT indicating success or failure.
     */
    DECLSPEC_XFGVIRT(IListView, GetTextBackgroundColor)
    STDMETHOD(GetTextBackgroundColor)(THIS_ _Out_ COLORREF* Color) PURE;

    /**
     * \brief Sets the background color for the text in the list view.
     *
     * \param[in] color The COLORREF value specifying the new text background color.
     * \return HRESULT indicating success or failure.
     */
    DECLSPEC_XFGVIRT(IListView, SetTextBackgroundColor)
    STDMETHOD(SetTextBackgroundColor)(THIS_ _In_ COLORREF Color) PURE;

    DECLSPEC_XFGVIRT(IListView, GetHotLightColor)
    STDMETHOD(GetHotLightColor)(THIS_ _Out_ COLORREF* Color) PURE;

    DECLSPEC_XFGVIRT(IListView, SetHotLightColor)
    STDMETHOD(SetHotLightColor)(THIS_ _In_ COLORREF Color) PURE;

    DECLSPEC_XFGVIRT(IListView, GetItemCount)
    STDMETHOD(GetItemCount)(THIS_ _Out_ PLONG ItemCount) PURE;

    DECLSPEC_XFGVIRT(IListView, SetItemCount)
    STDMETHOD(SetItemCount)(THIS_ _In_ LONG ItemCount, _In_ LV_LISTVIEW_SETITEMCOUNT_FLAGS Flags) PURE;

    DECLSPEC_XFGVIRT(IListView, GetItem)
    STDMETHOD(GetItem)(THIS_ _Inout_ LVITEMW* Item) PURE;

    DECLSPEC_XFGVIRT(IListView, SetItem)
    STDMETHOD(SetItem)(THIS_ _In_ LVITEMW* const Item) PURE;

    DECLSPEC_XFGVIRT(IListView, GetItemState)
    STDMETHOD(GetItemState)(THIS_ _In_ LONG ItemIndex, _In_ LONG SubItemIndex, _In_ LV_LISTVIEW_ITEM_STATE_FLAGS Mask, _Out_ LV_LISTVIEW_ITEM_STATE_FLAGS* State) PURE;

    DECLSPEC_XFGVIRT(IListView, SetItemState)
    STDMETHOD(SetItemState)(THIS_ _In_ LONG ItemIndex, _In_ LONG SubItemIndex, _In_ LV_LISTVIEW_ITEM_STATE_FLAGS Mask, _In_ LV_LISTVIEW_ITEM_STATE_FLAGS State) PURE;

    DECLSPEC_XFGVIRT(IListView, GetItemText)
    STDMETHOD(GetItemText)(THIS_ _In_ LONG ItemIndex, _In_ LONG SubItemIndex, _Out_writes_(BufferSize) PWSTR Buffer, _In_ LONG BufferSize) PURE;

    DECLSPEC_XFGVIRT(IListView, SetItemText)
    STDMETHOD(SetItemText)(THIS_ _In_ LONG ItemIndex, _In_ LONG SubItemIndex, _In_ PCWSTR Text) PURE;

    DECLSPEC_XFGVIRT(IListView, GetBackgroundImage)
    STDMETHOD(GetBackgroundImage)(THIS_ _Inout_ LVBKIMAGEW* pBkImage) PURE;

    DECLSPEC_XFGVIRT(IListView, SetBackgroundImage)
    STDMETHOD(SetBackgroundImage)(THIS_ _In_ LVBKIMAGEW* const BkImage) PURE;

    DECLSPEC_XFGVIRT(IListView, GetFocusedColumn)
    STDMETHOD(GetFocusedColumn)(THIS_ _Out_ PLONG ColumnIndex) PURE;

    DECLSPEC_XFGVIRT(IListView, SetSelectionFlags)
    STDMETHOD(SetSelectionFlags)(THIS_ _In_ LV_LISTVIEW_SELECTION_FLAGS Mask, _In_ LV_LISTVIEW_SELECTION_FLAGS Flags) PURE;

    DECLSPEC_XFGVIRT(IListView, GetSelectedColumn)
    STDMETHOD(GetSelectedColumn)(THIS_ _Out_ PLONG ColumnIndex) PURE;

    DECLSPEC_XFGVIRT(IListView, SetSelectedColumn)
    STDMETHOD(SetSelectedColumn)(THIS_ _In_ LONG ColumnIndex) PURE;

    DECLSPEC_XFGVIRT(IListView, GetView)
    STDMETHOD(GetView)(THIS_ _Out_ PULONG View) PURE;

    DECLSPEC_XFGVIRT(IListView, SetView)
    STDMETHOD(SetView)(THIS_ _In_ ULONG View) PURE;

    DECLSPEC_XFGVIRT(IListView, InsertItem)
    STDMETHOD(InsertItem)(THIS_ _In_ LVITEMW* const Item, _Out_ PLONG ItemIndex) PURE;

    DECLSPEC_XFGVIRT(IListView, DeleteItem)
    STDMETHOD(DeleteItem)(THIS_ _In_ LONG ItemIndex) PURE;

    DECLSPEC_XFGVIRT(IListView, DeleteAllItems)
    STDMETHOD(DeleteAllItems)(THIS) PURE;

    DECLSPEC_XFGVIRT(IListView, UpdateItem)
    STDMETHOD(UpdateItem)(THIS_ _In_ LONG ItemIndex) PURE;

    DECLSPEC_XFGVIRT(IListView, GetItemRect)
    STDMETHOD(GetItemRect)(THIS_ _In_ LVITEMINDEX ItemIndex, _In_ LONG RectangleType, _Out_ PRECT Rectangle) PURE;

    DECLSPEC_XFGVIRT(IListView, GetSubItemRect)
    STDMETHOD(GetSubItemRect)(THIS_ _In_ LVITEMINDEX ItemIndex, _In_ LONG SubItemIndex, _In_ LONG RectangleType, _Out_ PRECT Rectangle) PURE;

    DECLSPEC_XFGVIRT(IListView, HitTestSubItem)
    STDMETHOD(HitTestSubItem)(THIS_ _Inout_ LVHITTESTINFO* HitTestData) PURE;

    DECLSPEC_XFGVIRT(IListView, GetIncrSearchString)
    STDMETHOD(GetIncrSearchString)(THIS_ _Out_writes_(BufferSize) PWSTR Buffer, _In_ LONG BufferSize, _Out_ PLONG CopiedChars) PURE;

    DECLSPEC_XFGVIRT(IListView, GetItemSpacing)
    STDMETHOD(GetItemSpacing)(THIS_ _In_ BOOL SmallIconView, _Out_ PLONG HorizontalSpacing, _Out_ PLONG VerticalSpacing) PURE;

    DECLSPEC_XFGVIRT(IListView, SetIconSpacing)
    STDMETHOD(SetIconSpacing)(THIS_ _In_ LONG HorizontalSpacing, _In_ LONG VerticalSpacing, _Out_opt_ PLONG OldHorizontalSpacing, _Out_opt_ PLONG OldVerticalSpacing) PURE;

    DECLSPEC_XFGVIRT(IListView, GetNextItem)
    STDMETHOD(GetNextItem)(THIS_ _In_ LVITEMINDEX ItemIndex, _In_ ULONG Flags, _Out_ LVITEMINDEX* NextItemIndex) PURE;

    DECLSPEC_XFGVIRT(IListView, FindItem)
    STDMETHOD(FindItem)(THIS_ _In_ LVITEMINDEX StartItemIndex, _In_ LVFINDINFOW const* FindInfo, _Out_ LVITEMINDEX* FoundItemIndex) PURE;

    DECLSPEC_XFGVIRT(IListView, GetSelectionMark)
    STDMETHOD(GetSelectionMark)(THIS_ _Out_ LVITEMINDEX* SelectionMark) PURE;

    DECLSPEC_XFGVIRT(IListView, SetSelectionMark)
    STDMETHOD(SetSelectionMark)(THIS_ _In_ LVITEMINDEX NewSelectionMark, _Out_opt_ LVITEMINDEX* OldSelectionMark) PURE;

    DECLSPEC_XFGVIRT(IListView, GetItemPosition)
    STDMETHOD(GetItemPosition)(THIS_ _In_ LVITEMINDEX ItemIndex, _Out_ POINT* Position) PURE;

    DECLSPEC_XFGVIRT(IListView, SetItemPosition)
    STDMETHOD(SetItemPosition)(THIS_ _In_ LONG ItemIndex, _In_ POINT const* Position) PURE;

    DECLSPEC_XFGVIRT(IListView, ScrollView)
    STDMETHOD(ScrollView)(THIS_ _In_ ULONG HorizontalScrollDistance, _In_ ULONG VerticalScrollDistance) PURE;

    DECLSPEC_XFGVIRT(IListView, EnsureItemVisible)
    STDMETHOD(EnsureItemVisible)(THIS_ _In_ LVITEMINDEX ItemIndex, _In_ BOOL PartialOk) PURE;

    DECLSPEC_XFGVIRT(IListView, EnsureSubItemVisible)
    STDMETHOD(EnsureSubItemVisible)(THIS_ _In_ LVITEMINDEX ItemIndex, _In_ LONG SubItemIndex) PURE;

    DECLSPEC_XFGVIRT(IListView, EditSubItem)
    STDMETHOD(EditSubItem)(THIS_ _In_ LVITEMINDEX ItemIndex, _In_ LONG SubItemIndex) PURE;

    DECLSPEC_XFGVIRT(IListView, RedrawItems)
    STDMETHOD(RedrawItems)(THIS_ _In_ LONG FirstItemIndex, _In_ LONG LastItemIndex) PURE;

    DECLSPEC_XFGVIRT(IListView, ArrangeItems)
    STDMETHOD(ArrangeItems)(THIS_ _In_ LONG Mode) PURE;

    DECLSPEC_XFGVIRT(IListView, RecomputeItems)
    STDMETHOD(RecomputeItems)(THIS_ _In_ LONG Mode) PURE;

    DECLSPEC_XFGVIRT(IListView, GetEditControl)
    STDMETHOD(GetEditControl)(THIS_ _Out_ HWND* EditWindowHandle) PURE;

    DECLSPEC_XFGVIRT(IListView, EditLabel)
    STDMETHOD(EditLabel)(THIS_ _In_ LVITEMINDEX ItemIndex, _In_opt_ PCWSTR InitialEditText, _Out_ HWND* EditWindowHandle) PURE;

    DECLSPEC_XFGVIRT(IListView, EditGroupLabel)
    STDMETHOD(EditGroupLabel)(THIS_ _In_ LONG GroupIndex) PURE;

    DECLSPEC_XFGVIRT(IListView, CancelEditLabel)
    STDMETHOD(CancelEditLabel)(THIS) PURE;

    DECLSPEC_XFGVIRT(IListView, GetEditItem)
    STDMETHOD(GetEditItem)(THIS_ _Out_ LVITEMINDEX* ItemIndex, _Out_ PLONG SubItemIndex) PURE;

    DECLSPEC_XFGVIRT(IListView, HitTest)
    STDMETHOD(HitTest)(THIS_ _Inout_ LVHITTESTINFO* HitTestData) PURE;

    DECLSPEC_XFGVIRT(IListView, GetStringWidth)
    STDMETHOD(GetStringWidth)(THIS_ _In_ PCWSTR String, _Out_ PLONG Width) PURE;

    DECLSPEC_XFGVIRT(IListView, GetColumn)
    STDMETHOD(GetColumn)(THIS_ _In_ LONG ColumnIndex, _Inout_ LVCOLUMNW* Column) PURE;

    DECLSPEC_XFGVIRT(IListView, SetColumn)
    STDMETHOD(SetColumn)(THIS_ _In_ LONG ColumnIndex, _In_ LVCOLUMNW* const Column) PURE;

    DECLSPEC_XFGVIRT(IListView, GetColumnOrderArray)
    STDMETHOD(GetColumnOrderArray)(THIS_ _In_ LONG NumberOfColumns, _Out_ PVOID* Columns) PURE;

    DECLSPEC_XFGVIRT(IListView, SetColumnOrderArray)
    STDMETHOD(SetColumnOrderArray)(THIS_ _In_ LONG NumberOfColumns, _In_ PVOID Columns) PURE;

    DECLSPEC_XFGVIRT(IListView, GetHeaderControl)
    STDMETHOD(GetHeaderControl)(THIS_ _Out_ HWND* HeaderWindowHandle) PURE;

    DECLSPEC_XFGVIRT(IListView, InsertColumn)
    STDMETHOD(InsertColumn)(THIS_ _In_ LONG InsertAt, _In_ LVCOLUMNW* const Column, _Out_ PLONG ColumnIndex) PURE;

    DECLSPEC_XFGVIRT(IListView, DeleteColumn)
    STDMETHOD(DeleteColumn)(THIS_ _In_ LONG ColumnIndex) PURE;

    DECLSPEC_XFGVIRT(IListView, CreateDragImage)
    STDMETHOD(CreateDragImage)(THIS_ _In_ LONG ItemIndex, _In_opt_ POINT const* UpperLeft, _Out_ HIMAGELIST* ImageList) PURE;

    DECLSPEC_XFGVIRT(IListView, GetViewRect)
    STDMETHOD(GetViewRect)(THIS_ _Out_ RECT* Rectangle) PURE;

    DECLSPEC_XFGVIRT(IListView, GetClientRectangle)
    STDMETHOD(GetClientRectangle)(THIS_ _In_ BOOL StyleAndClientRect, _Out_ RECT* ClientRectangle) PURE;

    DECLSPEC_XFGVIRT(IListView, GetColumnWidth)
    STDMETHOD(GetColumnWidth)(THIS_ _In_ LONG ColumnIndex, _Out_ PLONG Width) PURE;

    DECLSPEC_XFGVIRT(IListView, SetColumnWidth)
    STDMETHOD(SetColumnWidth)(THIS_ _In_ LONG ColumnIndex, _In_ LONG Width) PURE;

    /*
     * Gets the callback mask for a list-view control. You can send this message explicitly or by using the ListView_GetCallbackMask macro.
     * Mask directly correspond to the LVIS_* bitmask flags from the underlying LVM_GETCALLBACKMASK/LVM_SETCALLBACKMASK.
     * \sa https://learn.microsoft.com/en-us/windows/win32/controls/lvm-getcallbackmask
     */
    DECLSPEC_XFGVIRT(IListView, GetCallbackMask)
    STDMETHOD(GetCallbackMask)(THIS_ _Out_ PULONG Mask) PURE;

    /*
     * Changes the callback mask for a list-view control. You can send this message explicitly or by using the ListView_SetCallbackMask macro.
     * Mask directly correspond to the LVIS_* bitmask flags from the underlying LVM_GETCALLBACKMASK/LVM_SETCALLBACKMASK.
     * \sa https://learn.microsoft.com/en-us/windows/win32/controls/lvm-getcallbackmask
     */
    DECLSPEC_XFGVIRT(IListView, SetCallbackMask)
    STDMETHOD(SetCallbackMask)(THIS_ _In_ ULONG Mask) PURE;

    DECLSPEC_XFGVIRT(IListView, GetTopIndex)
    STDMETHOD(GetTopIndex)(THIS_ _Out_ PLONG TopIndex) PURE;

    DECLSPEC_XFGVIRT(IListView, GetCountPerPage)
    STDMETHOD(GetCountPerPage)(THIS_ _Out_ PLONG CountPerPage) PURE;

    DECLSPEC_XFGVIRT(IListView, GetOrigin)
    STDMETHOD(GetOrigin)(THIS_ _Out_ POINT* Origin) PURE;

    DECLSPEC_XFGVIRT(IListView, GetSelectedCount)
    STDMETHOD(GetSelectedCount)(THIS_ _Out_ PLONG SelectedCount) PURE;

    DECLSPEC_XFGVIRT(IListView, SortItems)
    STDMETHOD(SortItems)(THIS_ _In_ BOOL SortingByIndex, _In_ LPARAM lParam, _In_ PFNLVCOMPARE ComparisonFunction) PURE;

    /*
     * Gets the extended styles that are currently in use for a given list-view control.
     * You can send this message explicitly or use the ListView_GetExtendedListViewStyle macro.
     * \sa https://learn.microsoft.com/en-us/windows/win32/controls/lvm-getextendedlistviewstyle
     */
    DECLSPEC_XFGVIRT(IListView, GetExtendedStyle)
    STDMETHOD(GetExtendedStyle)(THIS_ _Out_ PULONG Style) PURE;

    /*
     * Sets extended styles in list-view controls.
     * You can send this message explicitly or use the ListView_SetExtendedListViewStyle or ListView_SetExtendedListViewStyleEx macro.
     * \sa https://learn.microsoft.com/en-us/windows/win32/controls/lvm-setextendedlistviewstyle
     */
    DECLSPEC_XFGVIRT(IListView, SetExtendedStyle)
    STDMETHOD(SetExtendedStyle)(THIS_ _In_ ULONG Mask, _In_ ULONG Style, _Out_opt_ PULONG OldStyle) PURE;

    DECLSPEC_XFGVIRT(IListView, GetHoverTime)
    STDMETHOD(GetHoverTime)(THIS_ _Out_ PULONG Time) PURE;

    DECLSPEC_XFGVIRT(IListView, SetHoverTime)
    STDMETHOD(SetHoverTime)(THIS_ _In_ ULONG Time, _Out_opt_ PULONG OldSetting) PURE;

    DECLSPEC_XFGVIRT(IListView, GetToolTip)
    STDMETHOD(GetToolTip)(THIS_ _Out_ HWND* ToolTipWindowHandle) PURE;

    DECLSPEC_XFGVIRT(IListView, SetToolTip)
    STDMETHOD(SetToolTip)(THIS_ _In_ HWND ToolTipWindowHandle, _Out_opt_ HWND* OldToolTipWindowHandle) PURE;

    DECLSPEC_XFGVIRT(IListView, GetHotItem)
    STDMETHOD(GetHotItem)(THIS_ _Out_ LVITEMINDEX* HotItem) PURE;

    DECLSPEC_XFGVIRT(IListView, SetHotItem)
    STDMETHOD(SetHotItem)(THIS_ _In_ LVITEMINDEX NewHotItem, _Out_opt_ LVITEMINDEX* OldHotItem) PURE;

    DECLSPEC_XFGVIRT(IListView, GetHotCursor)
    STDMETHOD(GetHotCursor)(THIS_ _Out_ HCURSOR* Cursor) PURE;

    DECLSPEC_XFGVIRT(IListView, SetHotCursor)
    STDMETHOD(SetHotCursor)(THIS_ _In_ HCURSOR Cursor, _Out_opt_ HCURSOR* OldCursor) PURE;

    DECLSPEC_XFGVIRT(IListView, ApproximateViewRect)
    STDMETHOD(ApproximateViewRect)(THIS_ _In_ LONG ItemCount, _Out_opt_ PLONG Width, _Out_opt_ PLONG Height) PURE;

    DECLSPEC_XFGVIRT(IListView, SetRangeObject)
    STDMETHOD(SetRangeObject)(THIS_ _In_ LONG RangeObjectType, _Inout_ void** Object) PURE;

    DECLSPEC_XFGVIRT(IListView, GetWorkAreas)
    STDMETHOD(GetWorkAreas)(THIS_ _In_ LONG NumberOfWorkAreas, _Out_writes_(NumberOfWorkAreas) PRECT WorkAreas) PURE;

    DECLSPEC_XFGVIRT(IListView, SetWorkAreas)
    STDMETHOD(SetWorkAreas)(THIS_ _In_ LONG NumberOfWorkAreas, _In_reads_(NumberOfWorkAreas) RECT const* WorkAreas) PURE;

    DECLSPEC_XFGVIRT(IListView, GetWorkAreaCount)
    STDMETHOD(GetWorkAreaCount)(THIS_ _Out_ PLONG NumberOfWorkAreas) PURE;

    DECLSPEC_XFGVIRT(IListView, ResetEmptyText)
    STDMETHOD(ResetEmptyText)(THIS) PURE;

    DECLSPEC_XFGVIRT(IListView, EnableGroupView)
    STDMETHOD(EnableGroupView)(THIS_ _In_ BOOL Enable) PURE;

    DECLSPEC_XFGVIRT(IListView, IsGroupViewEnabled)
    STDMETHOD(IsGroupViewEnabled)(THIS_ _Out_ PBOOL IsEnabled) PURE;

    DECLSPEC_XFGVIRT(IListView, SortGroups)
    STDMETHOD(SortGroups)(THIS_ _In_ PFNLVGROUPCOMPARE pComparisonFunction, _In_opt_ PVOID lParam) PURE;

    DECLSPEC_XFGVIRT(IListView, GetGroupInfo)
    STDMETHOD(GetGroupInfo)(THIS_ _In_ BOOL GetGroupInfoByIndex, _In_ LONG GroupIDOrIndex, _Inout_ LVGROUP* Group) PURE;

    DECLSPEC_XFGVIRT(IListView, SetGroupInfo)
    STDMETHOD(SetGroupInfo)(THIS_ _In_ BOOL SetGroupInfoByIndex, _In_ LONG GroupIDOrIndex, _In_ LVGROUP* const Group) PURE;

    DECLSPEC_XFGVIRT(IListView, GetGroupRect)
    STDMETHOD(GetGroupRect)(THIS_ _In_ BOOL GetGroupRectByIndex, _In_ LONG GroupIDOrIndex, _In_ LONG RectangleType, _Out_ PRECT pRectangle) PURE;

    DECLSPEC_XFGVIRT(IListView, GetGroupState)
    STDMETHOD(GetGroupState)(THIS_ _In_ LONG GroupID, _In_ ULONG Mask, _Out_ PULONG State) PURE;

    DECLSPEC_XFGVIRT(IListView, HasGroup)
    STDMETHOD(HasGroup)(THIS_ _In_ LONG GroupID, _Out_ PBOOL HasGroup) PURE;

    DECLSPEC_XFGVIRT(IListView, InsertGroup)
    STDMETHOD(InsertGroup)(THIS_ _In_ LONG InsertAt, _In_ LVGROUP* const Group, _Out_ PLONG GroupID) PURE;

    DECLSPEC_XFGVIRT(IListView, RemoveGroup)
    STDMETHOD(RemoveGroup)(THIS_ _In_ LONG GroupID) PURE;

    DECLSPEC_XFGVIRT(IListView, InsertGroupSorted)
    STDMETHOD(InsertGroupSorted)(THIS_ _In_ LVINSERTGROUPSORTED const* Group, _Out_ PLONG GroupID) PURE;

    DECLSPEC_XFGVIRT(IListView, GetGroupMetrics)
    STDMETHOD(GetGroupMetrics)(THIS_ _Inout_ LVGROUPMETRICS* Metrics) PURE;

    DECLSPEC_XFGVIRT(IListView, SetGroupMetrics)
    STDMETHOD(SetGroupMetrics)(THIS_ _In_ LVGROUPMETRICS* const Metrics) PURE;

    DECLSPEC_XFGVIRT(IListView, RemoveAllGroups)
    STDMETHOD(RemoveAllGroups)(THIS) PURE;

    DECLSPEC_XFGVIRT(IListView, GetFocusedGroup)
    STDMETHOD(GetFocusedGroup)(THIS_ _Out_ PLONG GroupID) PURE;

    DECLSPEC_XFGVIRT(IListView, GetGroupCount)
    STDMETHOD(GetGroupCount)(THIS_ _Out_ PLONG Count) PURE;

    DECLSPEC_XFGVIRT(IListView, SetOwnerDataCallback)
    STDMETHOD(SetOwnerDataCallback)(THIS_ _In_opt_ IOwnerDataCallback* Callback) PURE;

    DECLSPEC_XFGVIRT(IListView, GetTileViewInfo)
    STDMETHOD(GetTileViewInfo)(THIS_ _Inout_ LVTILEVIEWINFO* Info) PURE;

    DECLSPEC_XFGVIRT(IListView, SetTileViewInfo)
    STDMETHOD(SetTileViewInfo)(THIS_ _In_ LVTILEVIEWINFO* const Info) PURE;

    DECLSPEC_XFGVIRT(IListView, GetTileInfo)
    STDMETHOD(GetTileInfo)(THIS_ _Inout_ LVTILEINFO* TileInfo) PURE;

    DECLSPEC_XFGVIRT(IListView, SetTileInfo)
    STDMETHOD(SetTileInfo)(THIS_ _In_ LVTILEINFO* const TileInfo) PURE;

    DECLSPEC_XFGVIRT(IListView, GetInsertMark)
    STDMETHOD(GetInsertMark)(THIS_ _Out_ LVINSERTMARK* InsertMarkDetails) PURE;

    DECLSPEC_XFGVIRT(IListView, SetInsertMark)
    STDMETHOD(SetInsertMark)(THIS_ _In_ LVINSERTMARK const* InsertMarkDetails) PURE;

    DECLSPEC_XFGVIRT(IListView, GetInsertMarkRect)
    STDMETHOD(GetInsertMarkRect)(THIS_ _Out_ LPRECT pInsertMarkRectangle) PURE;

    DECLSPEC_XFGVIRT(IListView, GetInsertMarkColor)
    STDMETHOD(GetInsertMarkColor)(THIS_ _Out_ COLORREF* Color) PURE;

    DECLSPEC_XFGVIRT(IListView, SetInsertMarkColor)
    STDMETHOD(SetInsertMarkColor)(THIS_ _In_ COLORREF Color, _Out_opt_ COLORREF* OldColor) PURE;

    DECLSPEC_XFGVIRT(IListView, HitTestInsertMark)
    STDMETHOD(HitTestInsertMark)(THIS_ _In_ POINT const* Point, _Out_ LVINSERTMARK* InsertMarkDetails) PURE;

    DECLSPEC_XFGVIRT(IListView, SetInfoTip)
    STDMETHOD(SetInfoTip)(THIS_ _In_ LVSETINFOTIP* const InfoTip) PURE;

    DECLSPEC_XFGVIRT(IListView, GetOutlineColor)
    STDMETHOD(GetOutlineColor)(THIS_ _Out_ COLORREF* Color) PURE;

    DECLSPEC_XFGVIRT(IListView, SetOutlineColor)
    STDMETHOD(SetOutlineColor)(THIS_ _In_ COLORREF Color, _Out_opt_ COLORREF* OldColor) PURE;

    DECLSPEC_XFGVIRT(IListView, GetFrozenItem)
    STDMETHOD(GetFrozenItem)(THIS_ _Out_ PLONG ItemIndex) PURE;

    DECLSPEC_XFGVIRT(IListView, SetFrozenItem)
    STDMETHOD(SetFrozenItem)(THIS_ _In_ LONG ItemIndexStart, _In_ LONG ItemIndexEnd) PURE;

    DECLSPEC_XFGVIRT(IListView, GetFrozenSlot)
    STDMETHOD(GetFrozenSlot)(THIS_ _Out_ RECT* Rect) PURE;

    DECLSPEC_XFGVIRT(IListView, SetFrozenSlot)
    STDMETHOD(SetFrozenSlot)(THIS_ _In_ LONG Slot, _In_ POINT const* Rect) PURE;

    DECLSPEC_XFGVIRT(IListView, GetViewMargin)
    STDMETHOD(GetViewMargin)(THIS_ _Out_ RECT* Margin) PURE;

    DECLSPEC_XFGVIRT(IListView, SetViewMargin)
    STDMETHOD(SetViewMargin)(THIS_ _In_ RECT const* Margin) PURE;

    DECLSPEC_XFGVIRT(IListView, SetKeyboardSelected)
    STDMETHOD(SetKeyboardSelected)(THIS_ _In_ LVITEMINDEX ItemIndex) PURE;

    DECLSPEC_XFGVIRT(IListView, MapIndexToId)
    STDMETHOD(MapIndexToId)(THIS_ _In_ LONG ItemIndex, _Out_ PLONG ItemID) PURE;

    DECLSPEC_XFGVIRT(IListView, MapIdToIndex)
    STDMETHOD(MapIdToIndex)(THIS_ _In_ LONG ItemID, _Out_ PLONG ItemIndex) PURE;

    DECLSPEC_XFGVIRT(IListView, IsItemVisible)
    STDMETHOD(IsItemVisible)(THIS_ _In_ LVITEMINDEX ItemIndex, _Out_ BOOL* Visible) PURE;

    DECLSPEC_XFGVIRT(IListView, EnableAlphaShadow)
    STDMETHOD(EnableAlphaShadow)(THIS_ _In_ BOOL Enable) PURE;

    DECLSPEC_XFGVIRT(IListView, GetGroupSubsetCount)
    STDMETHOD(GetGroupSubsetCount)(THIS_ _Out_ PLONG NumberOfRowsDisplayed) PURE;

    DECLSPEC_XFGVIRT(IListView, SetGroupSubsetCount)
    STDMETHOD(SetGroupSubsetCount)(THIS_ _In_ LONG NumberOfRowsToDisplay) PURE;

    DECLSPEC_XFGVIRT(IListView, GetVisibleSlotCount)
    STDMETHOD(GetVisibleSlotCount)(THIS_ _Out_ PLONG Count) PURE;

    DECLSPEC_XFGVIRT(IListView, GetColumnMargin)
    STDMETHOD(GetColumnMargin)(THIS_ _Out_ RECT* Margin) PURE;

    DECLSPEC_XFGVIRT(IListView, SetSubItemCallback)
    STDMETHOD(SetSubItemCallback)(THIS_ _In_opt_ LPVOID Callback) PURE;

    DECLSPEC_XFGVIRT(IListView, GetVisibleItemRange)
    STDMETHOD(GetVisibleItemRange)(THIS_ _Out_ LVITEMINDEX* FirstItem, _Out_ LVITEMINDEX* LastItem) PURE;

    DECLSPEC_XFGVIRT(IListView, SetTypeAheadFlags)
    STDMETHOD(SetTypeAheadFlags)(THIS_ _In_ LV_LISTVIEW_TYPEAHEAD_FLAGS Mask, _In_ LV_LISTVIEW_TYPEAHEAD_FLAGS Flags) PURE;

    // Windows 10

    /**
     * \brief Sets the work areas with DPI awareness for the view.
     *
     * \param [in] LONG The number of work areas to set.
     * \param [in] struct tagLVWORKAREAWITHDPI const* Pointer to an array of LVWORKAREAWITHDPI structures that define the work areas and their associated DPI settings.
     * \return HRESULT Returns S_OK if successful, or an error code otherwise.
     */
    DECLSPEC_XFGVIRT(IListView, SetWorkAreasWithDpi)
    STDMETHOD(SetWorkAreasWithDpi)(THIS_ _In_ LONG Index, _In_ struct tagLVWORKAREAWITHDPI const* WorkAreasWithDpi) PURE;

    /**
     * \brief Retrieves the work areas along with their associated DPI settings.
     *
     * \param [in] LONG The index or identifier for the work area.
     * \param [out] tagLVWORKAREAWITHDPI* Pointer to a structure that receives the work area information and its DPI.
     *
     * \return HRESULT indicating success or failure of the operation.
     */
    DECLSPEC_XFGVIRT(IListView, GetWorkAreasWithDpi)
    STDMETHOD(GetWorkAreasWithDpi)(THIS_ _In_ LONG Index, _Out_ struct tagLVWORKAREAWITHDPI* WorkAreasWithDpi) PURE;

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
    STDMETHOD(SetWorkAreaImageList)(THIS_ _In_ LONG WorkAreaIndex, _In_ LONG ImageListType, _In_opt_ HIMAGELIST ImageListHandle, _Out_opt_ HIMAGELIST* OldImageListHandle) PURE;

    /**
     * Retrieves the image list associated with a specified work area.
     *
     * \param [in] param1 The first LONG parameter, typically representing the work area index or identifier.
     * \param [in] param2 The second LONG parameter, usage depends on implementation context.
     * \param [out] phImageList Pointer to a HIMAGELIST that receives the handle to the image list.
     * \return HRESULT Returns S_OK on success, or an error code otherwise.
     */
    DECLSPEC_XFGVIRT(IListView, GetWorkAreaImageList)
    STDMETHOD(GetWorkAreaImageList)(THIS_ _In_ LONG WorkAreaIndex, _In_ LONG ImageListType, _Out_ HIMAGELIST* ImageListHandle) PURE;

    /**
     * Enables or disables the "Icon Bullying" feature.
     *
     * This method controls the behavior of icon manipulation within the interface.
     *
     * \param Mode Specifies the mode for icon bullying.
     * \return HRESULT indicating success or failure of the operation.
     */
    DECLSPEC_XFGVIRT(IListView, EnableIconBullying)
    STDMETHOD(EnableIconBullying)(THIS_ _In_ LV_LISTVIEW_ICON_BULLYING_MODE Mode) PURE;

    /**
     * \brief Enables or configures specific quirks or compatibility modes.
     *
     * \param Flags A bitmask of flags specifying which quirks to enable.
     * \return HRESULT indicating success or failure of the operation.
     */
    DECLSPEC_XFGVIRT(IListView, EnableQuirks)
    STDMETHOD(EnableQuirks)(THIS_ _In_ LV_LISTVIEW_QUIRKS_FLAGS Flags) PURE;

    END_INTERFACE
};

#endif // __IListView_INTERFACE_DEFINED__

#undef INTERFACE

#ifdef COBJMACROS

#define IListView_QueryInterface(This,riid,ppvObject) \
    ((This)->lpVtbl->QueryInterface(This,riid,ppvObject))
#define IListView_AddRef(This) \
    ((This)->lpVtbl->AddRef(This))
#define IListView_Release(This) \
    ((This)->lpVtbl->Release(This))
#define IListView_GetWindow(This, OleWindow, WindowHandle) \
    ((This)->lpVtbl->GetWindow(This, OleWindow, WindowHandle))
#define IListView_ContextSensitiveHelp(This, EnterMode) \
    ((This)->lpVtbl->ContextSensitiveHelp(This, EnterMode))
#define IListView_GetImageList(This, ImageList, NewImageList) \
    ((This)->lpVtbl->GetImageList(This, ImageList, ImageList))
#define IListView_SetImageList(This, ImageList, NewImageList, OldImageList) \
    ((This)->lpVtbl->SetImageList(This, ImageList, NewImageList, OldImageList))
#define IListView_GetBackgroundColor(This, Color) \
    ((This)->lpVtbl->GetBackgroundColor(This, Color))
#define IListView_SetBackgroundColor(This, Color) \
    ((This)->lpVtbl->SetBackgroundColor(This, Color))
#define IListView_GetForegroundColor(This, Color) \
    ((This)->lpVtbl->GetForegroundColor(This, Color))
#define IListView_SetForegroundColor(This, Color) \
    ((This)->lpVtbl->SetForegroundColor(This, Color))
#define IListView_GetTextBackgroundColor(This, Color) \
    ((This)->lpVtbl->GetTextBackgroundColor(This, Color))
#define IListView_SetTextBackgroundColor(This, Color) \
    ((This)->lpVtbl->SetTextBackgroundColor(This, Color))
#define IListView_GetHotLightColor(This, Color) \
    ((This)->lpVtbl->GetHotLightColor(This, Color))
#define IListView_SetHotLightColor(This, Color) \
    ((This)->lpVtbl->SetHotLightColor(This, Color))
#define IListView_GetItemCount(This, ItemCount) \
    ((This)->lpVtbl->GetItemCount(This, ItemCount))
#define IListView_SetItemCount(This, ItemCount, Dlags) \
    ((This)->lpVtbl->SetItemCount(This, ItemCount, Flags))
#define IListView_GetItem(This, Item) \
    ((This)->lpVtbl->GetItem(This, Item))
#define IListView_SetItem(This, Item) \
    ((This)->lpVtbl->SetItem(This, Item))
#define IListView_GetItemState(This, ItemCount, SubItemIndex, Mask, State) \
    ((This)->lpVtbl->GetItemState(This, ItemCount, SubItemIndex, Mask, State))
#define IListView_SetItemState(This, ItemIndex, SubItemIndex, Mask, State) \
    ((This)->lpVtbl->SetItemState(This, ItemIndex, SubItemIndex, Mask, State))
#define IListView_GetItemText(This, ItemCount, SbItemIndex, Buffer, BufferSize) \
    ((This)->lpVtbl->GetItemText(This, ItemCount, SubItemIndex, Buffer, BufferSize))
#define IListView_SetItemText(This, ItemIndex, SubItemIndex, Text) \
    ((This)->lpVtbl->SetItemText(This, ItemIndex, SubItemIndex, Text))
#define IListView_GetBackgroundImage(This, BkImage) \
    ((This)->lpVtbl->GetBackgroundImage(This, BkImage))
#define IListView_SetBackgroundImage(This, BkImage) \
    ((This)->lpVtbl->SetBackgroundImage(This, BkImage))
#define IListView_GetFocusedColumn(This, ColumnIndex) \
    ((This)->lpVtbl->GetFocusedColumn(This, ColumnIndex))
#define IListView_SetSelectionFlags(This, Mask, flags) \
    ((This)->lpVtbl->SetSelectionFlags(This, Mask, flags))
#define IListView_GetSelectedColumn(This, ColumnIndex) \
    ((This)->lpVtbl->GetSelectedColumn(This, ColumnIndex))
#define IListView_SetSelectedColumn(This, ColumnIndex) \
    ((This)->lpVtbl->SetSelectedColumn(This, ColumnIndex))
#define IListView_GetView(This, View) \
    ((This)->lpVtbl->GetView(This, View))
#define IListView_SetView(This, View) \
    ((This)->lpVtbl->SetView(This, View))
#define IListView_InsertItem(This,Item, ItemIndex) \
    ((This)->lpVtbl->InsertItem(This, Item, ItemIndex))
#define IListView_DeleteItem(This, ItemIndex) \
    ((This)->lpVtbl->DeleteItem(This, ItemIndex))
#define IListView_DeleteAllItems(This) \
    ((This)->lpVtbl->DeleteAllItems(This))
#define IListView_UpdateItem(This, ItemIndex) \
    ((This)->lpVtbl->UpdateItem(This, ItemIndex))
#define IListView_GetItemRect(This, ItemIndex, rectangleType, Rectangle) \
    ((This)->lpVtbl->GetItemRect(This, ItemIndex, rectangleType, Rectangle))
#define IListView_GetSubItemRect(This, ItemIndex, SubItemIndex, rectangleType, Rectangle) \
    ((This)->lpVtbl->GetSubItemRect(This, ItemIndex, SubItemIndex, rectangleType, Rectangle))
#define IListView_HitTestSubItem(This, HitTestData) \
    ((This)->lpVtbl->HitTestSubItem(This, HitTestData))
#define IListView_GetIncrSearchString(This, Buffer, bufferSize, CopiedChars) \
    ((This)->lpVtbl->GetIncrSearchString(This, Buffer, bufferSize, CopiedChars))
#define IListView_GetItemSpacing(This, smallIconView, HorizontalSpacing, VerticalSpacing) \
    ((This)->lpVtbl->GetItemSpacing(This, smallIconView, HorizontalSpacing, VerticalSpacing))
#define IListView_SetIconSpacing(This, horizontalSpacing, verticalSpacing, HorizontalSpacing, VerticalSpacing) \
    ((This)->lpVtbl->SetIconSpacing(This, horizontalSpacing, verticalSpacing, HorizontalSpacing, VerticalSpacing))
#define IListView_GetNextItem(This, ItemIndex, flags, NextItemIndex) \
    ((This)->lpVtbl->GetNextItem(This, ItemIndex, flags, NextItemIndex))
#define IListView_FindItem(This, startItemIndex, FindInfo, FoundItemIndex) \
    ((This)->lpVtbl->FindItem(This, startItemIndex, FindInfo, FoundItemIndex))
#define IListView_GetSelectionMark(This, SelectionMark) \
    ((This)->lpVtbl->GetSelectionMark(This, SelectionMark))
#define IListView_SetSelectionMark(This, newSelectionMark, OldSelectionMark) \
    ((This)->lpVtbl->SetSelectionMark(This, newSelectionMark, OldSelectionMark))
#define IListView_GetItemPosition(This, ItemIndex, osition) \
    ((This)->lpVtbl->GetItemPosition(This, ItemIndex, osition))
#define IListView_SetItemPosition(This, ItemIndex, osition) \
    ((This)->lpVtbl->SetItemPosition(This, ItemIndex, osition))
#define IListView_ScrollView(This, HorizontalScrollDistance, VerticalScrollDistance) \
    ((This)->lpVtbl->ScrollView(This, HorizontalScrollDistance, VerticalScrollDistance))
#define IListView_EnsureItemVisible(This, ItemIndex, artialOk) \
    ((This)->lpVtbl->EnsureItemVisible(This, ItemIndex, artialOk))
#define IListView_EnsureSubItemVisible(This, ItemIndex, SubItemIndex) \
    ((This)->lpVtbl->EnsureSubItemVisible(This, ItemIndex, SubItemIndex))
#define IListView_EditSubItem(This, ItemIndex, SubItemIndex) \
    ((This)->lpVtbl->EditSubItem(This, ItemIndex, SubItemIndex))
#define IListView_RedrawItems(This, FirstItemIndex, LastItemIndex) \
    ((This)->lpVtbl->RedrawItems(This, FirstItemIndex, LastItemIndex))
#define IListView_ArrangeItems(This, mode) \
    ((This)->lpVtbl->ArrangeItems(This, mode))
#define IListView_RecomputeItems(This, unknown) \
    ((This)->lpVtbl->RecomputeItems(This, unknown))
#define IListView_GetEditControl(This, EditWindowHandle) \
    ((This)->lpVtbl->GetEditControl(This, EditWindowHandle))
#define IListView_EditLabel(This, ItemIndex, initialEditText, EditWindowHandle) \
    ((This)->lpVtbl->EditLabel(This, ItemIndex, initialEditText, EditWindowHandle))
#define IListView_EditGroupLabel(This, groupIndex) \
    ((This)->lpVtbl->EditGroupLabel(This, groupIndex))
#define IListView_CancelEditLabel(This) \
    ((This)->lpVtbl->CancelEditLabel(This))
#define IListView_GetEditItem(This, ItemIndex, SubItemIndex) \
    ((This)->lpVtbl->GetEditItem(This, ItemIndex, SubItemIndex))
#define IListView_HitTest(This, HitTestData) \
    ((This)->lpVtbl->HitTest(This, HitTestData))
#define IListView_GetStringWidth(This, String, Width) \
    ((This)->lpVtbl->GetStringWidth(This, String, Width))
#define IListView_GetColumn(This, columnIndex, Column) \
    ((This)->lpVtbl->GetColumn(This, columnIndex, Column))
#define IListView_SetColumn(This, columnIndex, Column) \
    ((This)->lpVtbl->SetColumn(This, columnIndex, Column))
#define IListView_GetColumnOrderArray(This, numberOfColumns, Columns) \
    ((This)->lpVtbl->GetColumnOrderArray(This, numberOfColumns, Columns))
#define IListView_SetColumnOrderArray(This, numberOfColumns, Columns) \
    ((This)->lpVtbl->SetColumnOrderArray(This, numberOfColumns, Columns))
#define IListView_GetHeaderControl(This, HeaderWindowHandle) \
    ((This)->lpVtbl->GetHeaderControl(This, HeaderWindowHandle))
#define IListView_InsertColumn(This, insertAt, Column, ColumnIndex) \
    ((This)->lpVtbl->InsertColumn(This, insertAt, Column, ColumnIndex))
#define IListView_DeleteColumn(This, columnIndex) \
    ((This)->lpVtbl->DeleteColumn(This, columnIndex))
#define IListView_CreateDragImage(This, ItemIndex, UpperLeft, HImageList) \
    ((This)->lpVtbl->CreateDragImage(This, ItemIndex, UpperLeft, HImageList))
#define IListView_GetViewRect(This, Rectangle) \
    ((This)->lpVtbl->GetViewRect(This, Rectangle))
#define IListView_GetClientRectangle(This, unknown, ClientRectangle) \
    ((This)->lpVtbl->GetClientRectangle(This, unknown, ClientRectangle))
#define IListView_GetColumnWidth(This, columnIndex, Width) \
    ((This)->lpVtbl->GetColumnWidth(This, columnIndex, Width))
#define IListView_SetColumnWidth(This, columnIndex, width) \
    ((This)->lpVtbl->SetColumnWidth(This, columnIndex, width))
#define IListView_GetCallbackMask(This, Mask) \
    ((This)->lpVtbl->GetCallbackMask(This, Mask))
#define IListView_SetCallbackMask(This, mask) \
    ((This)->lpVtbl->SetCallbackMask(This, mask))
#define IListView_GetTopIndex(This, TopIndex) \
    ((This)->lpVtbl->GetTopIndex(This, TopIndex))
#define IListView_GetCountPerPage(This, CountPerPage) \
    ((This)->lpVtbl->GetCountPerPage(This, CountPerPage))
#define IListView_GetOrigin(This, Origin) \
    ((This)->lpVtbl->GetOrigin(This, Origin))
#define IListView_GetSelectedCount(This, SelectedCount) \
    ((This)->lpVtbl->GetSelectedCount(This, SelectedCount))
#define IListView_SortItems(This, SortingByIndex, lParam, ComparisonFunction) \
    ((This)->lpVtbl->SortItems(This, SortingByIndex, lParam, ComparisonFunction))
#define IListView_GetExtendedStyle(This, Style) \
    ((This)->lpVtbl->GetExtendedStyle(This, Style))
#define IListView_SetExtendedStyle(This, mask, style, OldStyle) \
    ((This)->lpVtbl->SetExtendedStyle(This, mask, style, OldStyle))
#define IListView_GetHoverTime(This, Time) \
    ((This)->lpVtbl->GetHoverTime(This, Time))
#define IListView_SetHoverTime(This, time, OldSetting) \
    ((This)->lpVtbl->SetHoverTime(This, time, OldSetting))
#define IListView_GetToolTip(This, ToolTipWindowHandle) \
    ((This)->lpVtbl->GetToolTip(This, ToolTipWindowHandle))
#define IListView_SetToolTip(This, ToolTipWindowHandle, OldToolTipWindowHandle) \
    ((This)->lpVtbl->SetToolTip(This, ToolTipWindowHandle, OldToolTipWindowHandle))
#define IListView_GetHotItem(This, HotItem) \
    ((This)->lpVtbl->GetHotItem(This, HotItem))
#define IListView_SetHotItem(This, newHotItem, OldHotItem) \
    ((This)->lpVtbl->SetHotItem(This, newHotItem, OldHotItem))
#define IListView_GetHotCursor(This, HCursor) \
    ((This)->lpVtbl->GetHotCursor(This, HCursor))
#define IListView_SetHotCursor(This, hCursor, HOldCursor) \
    ((This)->lpVtbl->SetHotCursor(This, hCursor, HOldCursor))
#define IListView_ApproximateViewRect(This, itemCount, Width, Height) \
    ((This)->lpVtbl->ApproximateViewRect(This, itemCount, Width, Height))
#define IListView_SetRangeObject(This, unknown, Object) \
    ((This)->lpVtbl->SetRangeObject(This, unknown, Object))
#define IListView_GetWorkAreas(This, numberOfWorkAreas, WorkAreas) \
    ((This)->lpVtbl->GetWorkAreas(This, numberOfWorkAreas, WorkAreas))
#define IListView_SetWorkAreas(This, numberOfWorkAreas, WorkAreas) \
    ((This)->lpVtbl->SetWorkAreas(This, numberOfWorkAreas, WorkAreas))
#define IListView_GetWorkAreaCount(This, NumberOfWorkAreas) \
    ((This)->lpVtbl->GetWorkAreaCount(This, NumberOfWorkAreas))
#define IListView_ResetEmptyText(This) \
    ((This)->lpVtbl->ResetEmptyText(This))
#define IListView_EnableGroupView(This, enable) \
    ((This)->lpVtbl->EnableGroupView(This, enable))
#define IListView_IsGroupViewEnabled(This, IsEnabled) \
    ((This)->lpVtbl->IsGroupViewEnabled(This, IsEnabled))
#define IListView_SortGroups(This, ComparisonFunction, lParam) \
    ((This)->lpVtbl->SortGroups(This, ComparisonFunction, lParam))
#define IListView_GetGroupInfo(This, GetGroupInfoByIndex, GroupIDOrIndex, Group) \
    ((This)->lpVtbl->GetGroupInfo(This, GetGroupInfoByIndex, GroupIDOrIndex, Group))
#define IListView_SetGroupInfo(This, SetGroupInfoByIndex, GroupIDOrIndex, Group) \
    ((This)->lpVtbl->SetGroupInfo(This, SetGroupInfoByIndex, GroupIDOrIndex, Group))
#define IListView_GetGroupRect(This, GetGroupRectByIndex, GroupIDOrIndex, rectangleType, Rectangle) \
    ((This)->lpVtbl->GetGroupRect(This, GetGroupRectByIndex, GroupIDOrIndex, rectangleType, Rectangle))
#define IListView_GetGroupState(This, groupID, mask, State) \
    ((This)->lpVtbl->GetGroupState(This, groupID, mask, State))
#define IListView_HasGroup(This, groupID, HasGroup) \
    ((This)->lpVtbl->HasGroup(This, groupID, HasGroup))
#define IListView_InsertGroup(This, insertAt, Group, GroupID) \
    ((This)->lpVtbl->InsertGroup(This, insertAt, Group, GroupID))
#define IListView_RemoveGroup(This, groupID) \
    ((This)->lpVtbl->RemoveGroup(This, groupID))
#define IListView_InsertGroupSorted(This, Group, GroupID) \
    ((This)->lpVtbl->InsertGroupSorted(This, Group, GroupID))
#define IListView_GetGroupMetrics(This, Metrics) \
    ((This)->lpVtbl->GetGroupMetrics(This, Metrics))
#define IListView_SetGroupMetrics(This, Metrics) \
    ((This)->lpVtbl->SetGroupMetrics(This, Metrics))
#define IListView_RemoveAllGroups(This) \
    ((This)->lpVtbl->RemoveAllGroups(This))
#define IListView_GetFocusedGroup(This, GroupID) \
    ((This)->lpVtbl->GetFocusedGroup(This, GroupID))
#define IListView_GetGroupCount(This, Count) \
    ((This)->lpVtbl->GetGroupCount(This, Count))
#define IListView_SetOwnerDataCallback(This, Callback) \
    ((This)->lpVtbl->SetOwnerDataCallback(This, Callback))
#define IListView_GetTileViewInfo(This, Info) \
    ((This)->lpVtbl->GetTileViewInfo(This, Info))
#define IListView_SetTileViewInfo(This, Info) \
    ((This)->lpVtbl->SetTileViewInfo(This, Info))
#define IListView_GetTileInfo(This, TileInfo) \
    ((This)->lpVtbl->GetTileInfo(This, TileInfo))
#define IListView_SetTileInfo(This, TileInfo) \
    ((This)->lpVtbl->SetTileInfo(This, TileInfo))
#define IListView_GetInsertMark(This, InsertMarkDetails) \
    ((This)->lpVtbl->GetInsertMark(This, InsertMarkDetails))
#define IListView_SetInsertMark(This, InsertMarkDetails) \
    ((This)->lpVtbl->SetInsertMark(This, InsertMarkDetails))
#define IListView_GetInsertMarkRect(This, InsertMarkRectangle) \
    ((This)->lpVtbl->GetInsertMarkRect(This, InsertMarkRectangle))
#define IListView_GetInsertMarkColor(This, Color) \
    ((This)->lpVtbl->GetInsertMarkColor(This, Color))
#define IListView_SetInsertMarkColor(This, color, OldColor) \
    ((This)->lpVtbl->SetInsertMarkColor(This, color, OldColor))
#define IListView_HitTestInsertMark(This, Point, InsertMarkDetails) \
    ((This)->lpVtbl->HitTestInsertMark(This, Point, InsertMarkDetails))
#define IListView_SetInfoTip(This, InfoTip) \
    ((This)->lpVtbl->SetInfoTip(This, InfoTip))
#define IListView_GetOutlineColor(This, Color) \
    ((This)->lpVtbl->GetOutlineColor(This, Color))
#define IListView_SetOutlineColor(This, color, OldColor) \
    ((This)->lpVtbl->SetOutlineColor(This, color, OldColor))
#define IListView_GetFrozenItem(This, ItemIndex) \
    ((This)->lpVtbl->GetFrozenItem(This, ItemIndex))
#define IListView_SetFrozenItem(This, unknown1, unknown2) \
    ((This)->lpVtbl->SetFrozenItem(This, unknown1, unknown2))
#define IListView_GetFrozenSlot(This, Unknown) \
    ((This)->lpVtbl->GetFrozenSlot(This, Unknown))
#define IListView_SetFrozenSlot(This, unknown1, Unknown2) \
    ((This)->lpVtbl->SetFrozenSlot(This, unknown1, Unknown2))
#define IListView_GetViewMargin(This, Margin) \
    ((This)->lpVtbl->GetViewMargin(This, Margin))
#define IListView_SetViewMargin(This, Margin) \
    ((This)->lpVtbl->SetViewMargin(This, Margin))
#define IListView_SetKeyboardSelected(This, ItemIndex) \
    ((This)->lpVtbl->SetKeyboardSelected(This, ItemIndex))
#define IListView_MapIndexToId(This, ItemIndex, ItemID) \
    ((This)->lpVtbl->MapIndexToId(This, ItemIndex, ItemID))
#define IListView_MapIdToIndex(This, itemID, ItemIndex) \
    ((This)->lpVtbl->MapIdToIndex(This, itemID, ItemIndex))
#define IListView_IsItemVisible(This, ItemIndex, Visible) \
    ((This)->lpVtbl->IsItemVisible(This, ItemIndex, Visible))
#define IListView_EnableAlphaShadow(This, enable) \
    ((This)->lpVtbl->EnableAlphaShadow(This, enable))
#define IListView_GetGroupSubsetCount(This, NumberOfRowsDisplayed) \
    ((This)->lpVtbl->GetGroupSubsetCount(This, NumberOfRowsDisplayed))
#define IListView_SetGroupSubsetCount(This, numberOfRowsToDisplay) \
    ((This)->lpVtbl->SetGroupSubsetCount(This, numberOfRowsToDisplay))
#define IListView_GetVisibleSlotCount(This, Count) \
    ((This)->lpVtbl->GetVisibleSlotCount(This, Count))
#define IListView_GetColumnMargin(This, Margin) \
    ((This)->lpVtbl->GetColumnMargin(This, Margin))
#define IListView_SetSubItemCallback(This, Callback) \
    ((This)->lpVtbl->SetSubItemCallback(This, Callback))
#define IListView_GetVisibleItemRange(This, FirstItem, LastItem) \
    ((This)->lpVtbl->GetVisibleItemRange(This, FirstItem, LastItem))
#define IListView_SetTypeAheadFlags(This, mask, flags) \
    ((This)->lpVtbl->SetTypeAheadFlags(This, mask, flags))

#endif // COBJMACROS

#endif
