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

// Listview2 (Undocumented)

#ifndef LVM_QUERYINTERFACE
#define LVM_QUERYINTERFACE (LVM_FIRST + 189) // IListView, IListView2, IOleWindow, IVisualProperties, IPropertyControlSite, IListViewFooter
#endif

DEFINE_GUID(IID_IListView, 0xE5B16AF2, 0x3990, 0x4681, 0xA6, 0x09, 0x1F, 0x06, 0x0C, 0xD1, 0x42, 0x69);
DEFINE_GUID(IID_IDrawPropertyControl, 0xE6DFF6FD, 0xBCD5, 0x4162, 0x9C, 0x65, 0xA3, 0xB1, 0x8C, 0x61, 0x6F, 0xDB);
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

DEFINE_GUID(CLSID_CAsyncSubItemControlsView, 0xa1dccc29, 0x7c70, 0x4821, 0x97, 0xae, 0x67, 0xf0, 0x41, 0x5, 0xec, 0x91);
DEFINE_GUID(CLSID_CFooterAreaView, 0xebe684bf, 0x3301, 0x4a6d, 0x83, 0xce, 0xe4, 0xd6, 0x85, 0x1b, 0xe8, 0x81);
DEFINE_GUID(CLSID_CGroupedVirtualModeView, 0xa08a0f2d, 0x647, 0x4443, 0x94, 0x50, 0xc4, 0x60, 0xf4, 0x79, 0x10, 0x46);
DEFINE_GUID(CLSID_CGroupSubsetView, 0x64a11699, 0x104a, 0x482a, 0x9d, 0x85, 0xcc, 0xfe, 0xa2, 0xee, 0x3c, 0x94);
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
DECLARE_INTERFACE_(IOwnerDataCallback, IUnknown)
{
    STDMETHOD(QueryInterface)(THIS_ REFIID riid, void** ppvObject) PURE;
    STDMETHOD_(ULONG, AddRef)(THIS) PURE;
    STDMETHOD_(ULONG, Release)(THIS) PURE;
    // IOwnerDataCallback
    STDMETHOD(OnDataAvailable)(THIS_ LPARAM lParam, ULONG dwItemCount) PURE;
    STDMETHOD(OnDataUnavailable)(THIS_ LPARAM lParam) PURE;
    STDMETHOD(OnDataChanged)(THIS_ LPARAM lParam, ULONG dwItemCount) PURE;
    STDMETHOD(OnDataReset)(THIS_ LPARAM lParam) PURE;
};

#undef INTERFACE
#define INTERFACE ISubItemCallback
DECLARE_INTERFACE_(ISubItemCallback, IUnknown)
{
    STDMETHOD(QueryInterface)(THIS_ REFIID riid, void** ppvObject) PURE;
    STDMETHOD_(ULONG, AddRef)(THIS) PURE;
    STDMETHOD_(ULONG, Release)(THIS) PURE;
    // ISubItemCallback
    STDMETHOD(GetSubItemTitle)(THIS_ INT subItemIndex, PWSTR pBuffer, INT bufferSize) PURE;
    STDMETHOD(GetSubItemControl)(THIS_ INT itemIndex, INT subItemIndex, REFIID requiredInterface, PPVOID ppObject) PURE;
    STDMETHOD(BeginSubItemEdit)(THIS_ INT itemIndex, INT subItemIndex, INT mode, REFIID requiredInterface, PPVOID ppObject) PURE;
    STDMETHOD(EndSubItemEdit)(THIS_ INT itemIndex, INT subItemIndex, int mode, struct IPropertyControl* pPropertyControl) PURE;
    STDMETHOD(BeginGroupEdit)(THIS_ INT groupIndex, REFIID INT, PPVOID ppObject) PURE;
    STDMETHOD(EndGroupEdit)(THIS_ INT groupIndex, INT mode, struct IPropertyControl* pPropertyControl) PURE;
    STDMETHOD(OnInvokeVerb)(THIS_ INT itemIndex, LPCWSTR pVerb) PURE;
};

#undef INTERFACE
#define INTERFACE IDrawPropertyControl
DECLARE_INTERFACE_(IDrawPropertyControl, IUnknown)
{
    STDMETHOD(QueryInterface)(THIS_ REFIID riid, void** ppvObject) PURE;
    STDMETHOD_(ULONG, AddRef)(THIS) PURE;
    STDMETHOD_(ULONG, Release)(THIS) PURE;

    // IPropertyControlBase
    STDMETHOD(Initialize)(IUnknown*, ULONG) PURE; // 2/calendar, 3/textbox
    STDMETHOD(GetSize)(enum PROPCTL_RECT_TYPE, HDC, SIZE const*, SIZE*) PURE;
    STDMETHOD(SetWindowTheme)(PCWSTR, PCWSTR) PURE;
    STDMETHOD(SetFont)(HFONT) PURE;
    STDMETHOD(SetTextColor)(ULONG) PURE;
    STDMETHOD(GetFlags)(INT*) PURE;
    STDMETHOD(SetFlags)(INT, INT) PURE;
    STDMETHOD(AdjustWindowRectPCB)(HWND, RECT*, RECT const*, INT) PURE;
    STDMETHOD(SetValue)(IUnknown*) PURE;
    STDMETHOD(InvokeDefaultAction)(VOID) PURE;
    STDMETHOD(Destroy)(VOID) PURE;
    STDMETHOD(SetFormatFlags)(INT) PURE;
    STDMETHOD(GetFormatFlags)(INT*) PURE;

    // IDrawPropertyControl
    STDMETHOD(GetDrawFlags)(PINT Flags) PURE;
    STDMETHOD(WindowlessDraw)(HDC hDC, RECT pDrawingRectangle, int a) PURE;
    STDMETHOD(HasVisibleContent)(VOID) PURE;
    STDMETHOD(GetDisplayText)(LPWSTR * Text) PURE;
    STDMETHOD(GetTooltipInfo)(HDC, SIZE const*, PINT) PURE;
};

#undef INTERFACE
#define INTERFACE IListView
DECLARE_INTERFACE_(IListView, IUnknown) // real name is IListView2
{
    BEGIN_INTERFACE

    STDMETHOD(QueryInterface)(THIS_ REFIID riid, void** ppvObject) PURE;
    STDMETHOD_(ULONG, AddRef)(THIS) PURE;
    STDMETHOD_(ULONG, Release)(THIS) PURE;
    // IOleWindow
    STDMETHOD(GetWindow)(THIS_ __RPC__in IOleWindow* window, __RPC__deref_out_opt HWND* WindowHandle) PURE;
    STDMETHOD(ContextSensitiveHelp)(THIS_ __RPC__in IOleWindow* window, _In_ BOOL fEnterMode) PURE;
    // IListView
    STDMETHOD(GetImageList)(THIS_ int imageList, HIMAGELIST* pHImageList) PURE;
    STDMETHOD(SetImageList)(THIS_ int imageList, HIMAGELIST hNewImageList, HIMAGELIST* pHOldImageList) PURE;
    STDMETHOD(GetBackgroundColor)(THIS_ COLORREF* pColor) PURE;
    STDMETHOD(SetBackgroundColor)(THIS_ COLORREF color) PURE;
    STDMETHOD(GetTextColor)(THIS_ COLORREF* pColor) PURE;
    STDMETHOD(SetTextColor)(THIS_ COLORREF color) PURE;
    STDMETHOD(GetTextBackgroundColor)(THIS_ COLORREF* pColor) PURE;
    STDMETHOD(SetTextBackgroundColor)(THIS_ COLORREF color) PURE;
    STDMETHOD(GetHotLightColor)(THIS_ COLORREF* pColor) PURE;
    STDMETHOD(SetHotLightColor)(THIS_ COLORREF color) PURE;
    STDMETHOD(GetItemCount)(THIS_ PINT pItemCount) PURE;
    STDMETHOD(SetItemCount)(THIS_ int itemCount, ULONG flags) PURE;
    STDMETHOD(GetItem)(THIS_ LVITEMW* pItem) PURE;
    STDMETHOD(SetItem)(THIS_ LVITEMW* const pItem) PURE;
    STDMETHOD(GetItemState)(THIS_ int itemIndex, int subItemIndex, ULONG mask, ULONG* pState) PURE;
    STDMETHOD(SetItemState)(THIS_ int itemIndex, int subItemIndex, ULONG mask, ULONG state) PURE;
    STDMETHOD(GetItemText)(THIS_ int itemIndex, int subItemIndex, LPWSTR pBuffer, int bufferSize) PURE;
    STDMETHOD(SetItemText)(THIS_ int itemIndex, int subItemIndex, LPCWSTR pText) PURE;
    STDMETHOD(GetBackgroundImage)(THIS_ LVBKIMAGEW* pBkImage) PURE;
    STDMETHOD(SetBackgroundImage)(THIS_ LVBKIMAGEW* const pBkImage) PURE;
    STDMETHOD(GetFocusedColumn)(THIS_ PINT pColumnIndex) PURE;
    STDMETHOD(SetSelectionFlags)(THIS_ ULONG mask, ULONG flags) PURE; // HRESULT SetSelectionFlags (SELECTION_FLAGS, SELECTION_FLAGS);
    STDMETHOD(GetSelectedColumn)(THIS_ PULONG pColumnIndex) PURE;
    STDMETHOD(SetSelectedColumn)(THIS_ ULONG columnIndex) PURE;
    STDMETHOD(GetView)(THIS_ ULONG* pView) PURE;
    STDMETHOD(SetView)(THIS_ ULONG view) PURE;
    STDMETHOD(InsertItem)(THIS_ LVITEMW* const pItem, PULONG pItemIndex) PURE;
    STDMETHOD(DeleteItem)(THIS_ ULONG itemIndex) PURE;
    STDMETHOD(DeleteAllItems)(THIS) PURE;
    STDMETHOD(UpdateItem)(THIS_ ULONG itemIndex) PURE;
    STDMETHOD(GetItemRect)(THIS_ LVITEMINDEX itemIndex, int rectangleType, LPRECT pRectangle) PURE;
    STDMETHOD(GetSubItemRect)(THIS_ LVITEMINDEX itemIndex, int subItemIndex, int rectangleType, LPRECT pRectangle) PURE;
    STDMETHOD(HitTestSubItem)(THIS_ LVHITTESTINFO* pHitTestData) PURE;
    STDMETHOD(GetIncrSearchString)(THIS_ PWSTR pBuffer, int bufferSize, PINT pCopiedChars) PURE;
    STDMETHOD(GetItemSpacing)(THIS_ BOOL smallIconView, PINT pHorizontalSpacing, PINT pVerticalSpacing) PURE;
    STDMETHOD(SetIconSpacing)(THIS_ int horizontalSpacing, int verticalSpacing, PINT pHorizontalSpacing, PINT pVerticalSpacing) PURE;
    STDMETHOD(GetNextItem)(THIS_ LVITEMINDEX itemIndex, ULONG flags, LVITEMINDEX* pNextItemIndex) PURE;
    STDMETHOD(FindItem)(THIS_ LVITEMINDEX startItemIndex, LVFINDINFOW const* pFindInfo, LVITEMINDEX* pFoundItemIndex) PURE;
    STDMETHOD(GetSelectionMark)(THIS_ LVITEMINDEX* pSelectionMark) PURE;
    STDMETHOD(SetSelectionMark)(THIS_ LVITEMINDEX newSelectionMark, LVITEMINDEX* pOldSelectionMark) PURE;
    STDMETHOD(GetItemPosition)(THIS_ LVITEMINDEX itemIndex, POINT* pPosition) PURE;
    STDMETHOD(SetItemPosition)(THIS_ ULONG itemIndex, POINT const* pPosition) PURE;
    STDMETHOD(ScrollView)(THIS_ ULONG horizontalScrollDistance, ULONG verticalScrollDistance) PURE;
    STDMETHOD(EnsureItemVisible)(THIS_ LVITEMINDEX itemIndex, BOOL partialOk) PURE;
    STDMETHOD(EnsureSubItemVisible)(THIS_ LVITEMINDEX itemIndex, ULONG subItemIndex) PURE;
    STDMETHOD(EditSubItem)(THIS_ LVITEMINDEX itemIndex, ULONG subItemIndex) PURE;
    STDMETHOD(RedrawItems)(THIS_ ULONG firstItemIndex, ULONG lastItemIndex) PURE;
    STDMETHOD(ArrangeItems)(THIS_ ULONG mode) PURE;
    STDMETHOD(RecomputeItems)(THIS_ int unknown) PURE;
    STDMETHOD(GetEditControl)(THIS_ HWND* EditWindowHandle) PURE;
    STDMETHOD(EditLabel)(THIS_ LVITEMINDEX itemIndex, LPCWSTR initialEditText, HWND* EditWindowHandle) PURE;
    STDMETHOD(EditGroupLabel)(THIS_ ULONG groupIndex) PURE;
    STDMETHOD(CancelEditLabel)(THIS) PURE;
    STDMETHOD(GetEditItem)(THIS_ LVITEMINDEX* itemIndex, PINT subItemIndex) PURE;
    STDMETHOD(HitTest)(THIS_ LVHITTESTINFO* pHitTestData) PURE;
    STDMETHOD(GetStringWidth)(THIS_ PCWSTR pString, PINT pWidth) PURE;
    STDMETHOD(GetColumn)(THIS_ ULONG columnIndex, LVCOLUMNW* pColumn) PURE;
    STDMETHOD(SetColumn)(THIS_ ULONG columnIndex, LVCOLUMNW* const pColumn) PURE;
    STDMETHOD(GetColumnOrderArray)(THIS_ ULONG numberOfColumns, PINT pColumns) PURE;
    STDMETHOD(SetColumnOrderArray)(THIS_ ULONG numberOfColumns, int const* pColumns) PURE;
    STDMETHOD(GetHeaderControl)(THIS_ HWND* HeaderWindowHandle) PURE;
    STDMETHOD(InsertColumn)(THIS_ ULONG insertAt, LVCOLUMNW* const pColumn, PINT pColumnIndex) PURE;
    STDMETHOD(DeleteColumn)(THIS_ ULONG columnIndex) PURE;
    STDMETHOD(CreateDragImage)(THIS_ ULONG itemIndex, POINT const* pUpperLeft, HIMAGELIST* pHImageList) PURE;
    STDMETHOD(GetViewRect)(THIS_ RECT* pRectangle) PURE;
    STDMETHOD(GetClientRect)(THIS_ BOOL unknown, RECT* pClientRectangle) PURE;
    STDMETHOD(GetColumnWidth)(THIS_ ULONG columnIndex, PINT pWidth) PURE;
    STDMETHOD(SetColumnWidth)(THIS_ ULONG columnIndex, ULONG width) PURE;
    STDMETHOD(GetCallbackMask)(THIS_ ULONG* pMask) PURE;
    STDMETHOD(SetCallbackMask)(THIS_ ULONG mask) PURE;
    STDMETHOD(GetTopIndex)(THIS_ PULONG pTopIndex) PURE;
    STDMETHOD(GetCountPerPage)(THIS_ PULONG pCountPerPage) PURE;
    STDMETHOD(GetOrigin)(THIS_ POINT* pOrigin) PURE;
    STDMETHOD(GetSelectedCount)(THIS_ PULONG pSelectedCount) PURE;
    STDMETHOD(SortItems)(THIS_ BOOL unknown, LPARAM lParam, PFNLVCOMPARE pComparisonFunction) PURE;
    STDMETHOD(GetExtendedStyle)(THIS_ ULONG* pStyle) PURE;
    STDMETHOD(SetExtendedStyle)(THIS_ ULONG mask, ULONG style, ULONG* pOldStyle) PURE;
    STDMETHOD(GetHoverTime)(THIS_ PULONG pTime) PURE;
    STDMETHOD(SetHoverTime)(THIS_ UINT time, PULONG pOldSetting) PURE;
    STDMETHOD(GetToolTip)(THIS_ HWND* ToolTipWindowHandle) PURE;
    STDMETHOD(SetToolTip)(THIS_ HWND ToolTipWindowHandle, HWND* OldToolTipWindowHandle) PURE;
    STDMETHOD(GetHotItem)(THIS_ LVITEMINDEX* pHotItem) PURE;
    STDMETHOD(SetHotItem)(THIS_ LVITEMINDEX newHotItem, LVITEMINDEX* pOldHotItem) PURE;
    STDMETHOD(GetHotCursor)(THIS_ HCURSOR* pHCursor) PURE;
    STDMETHOD(SetHotCursor)(THIS_ HCURSOR hCursor, HCURSOR* pHOldCursor) PURE;
    STDMETHOD(ApproximateViewRect)(THIS_ int itemCount, PINT pWidth, PINT pHeight) PURE;
    STDMETHOD(SetRangeObject)(THIS_ int unknown, LPVOID pObject) PURE;
    STDMETHOD(GetWorkAreas)(THIS_ int numberOfWorkAreas, RECT* pWorkAreas) PURE;
    STDMETHOD(SetWorkAreas)(THIS_ int numberOfWorkAreas, RECT const* pWorkAreas) PURE;
    STDMETHOD(GetWorkAreaCount)(THIS_ PINT pNumberOfWorkAreas) PURE;
    STDMETHOD(ResetEmptyText)(THIS) PURE;
    STDMETHOD(EnableGroupView)(THIS_ BOOL Eable) PURE;
    STDMETHOD(IsGroupViewEnabled)(THIS_ PBOOL IsEnabled) PURE;
    STDMETHOD(SortGroups)(THIS_ PFNLVGROUPCOMPARE pComparisonFunction, PVOID lParam) PURE;
    STDMETHOD(GetGroupInfo)(THIS_ ULONG unknown1, ULONG groupID, LVGROUP* pGroup) PURE;
    STDMETHOD(SetGroupInfo)(THIS_ ULONG unknown, ULONG groupID, LVGROUP* const pGroup) PURE;
    STDMETHOD(GetGroupRect)(THIS_ BOOL unknown, ULONG groupID, ULONG rectangleType, RECT* pRectangle) PURE;
    STDMETHOD(GetGroupState)(THIS_ ULONG groupID, ULONG mask, ULONG* pState) PURE;
    STDMETHOD(HasGroup)(THIS_ INT groupID, BOOL* pHasGroup) PURE;
    STDMETHOD(InsertGroup)(THIS_ INT insertAt, LVGROUP* const pGroup, PINT pGroupID) PURE;
    STDMETHOD(RemoveGroup)(THIS_ INT groupID) PURE;
    STDMETHOD(InsertGroupSorted)(THIS_ LVINSERTGROUPSORTED const* pGroup, PINT pGroupID) PURE;
    STDMETHOD(GetGroupMetrics)(THIS_ LVGROUPMETRICS* pMetrics) PURE;
    STDMETHOD(SetGroupMetrics)(THIS_ LVGROUPMETRICS* const pMetrics) PURE;
    STDMETHOD(RemoveAllGroups)(THIS) PURE;
    STDMETHOD(GetFocusedGroup)(THIS_ PINT pGroupID) PURE;
    STDMETHOD(GetGroupCount)(THIS_ PINT pCount) PURE;
    STDMETHOD(SetOwnerDataCallback)(THIS_ IOwnerDataCallback* pCallback) PURE;
    STDMETHOD(GetTileViewInfo)(THIS_ LVTILEVIEWINFO* pInfo) PURE;
    STDMETHOD(SetTileViewInfo)(THIS_ LVTILEVIEWINFO* const pInfo) PURE;
    STDMETHOD(GetTileInfo)(THIS_ LVTILEINFO* pTileInfo) PURE;
    STDMETHOD(SetTileInfo)(THIS_ LVTILEINFO* const pTileInfo) PURE;
    STDMETHOD(GetInsertMark)(THIS_ LVINSERTMARK* pInsertMarkDetails) PURE;
    STDMETHOD(SetInsertMark)(THIS_ LVINSERTMARK const* pInsertMarkDetails) PURE;
    STDMETHOD(GetInsertMarkRect)(THIS_ LPRECT pInsertMarkRectangle) PURE;
    STDMETHOD(GetInsertMarkColor)(THIS_ COLORREF* pColor) PURE;
    STDMETHOD(SetInsertMarkColor)(THIS_ COLORREF color, COLORREF* pOldColor) PURE;
    STDMETHOD(HitTestInsertMark)(THIS_ POINT const* pPoint, LVINSERTMARK* pInsertMarkDetails) PURE;
    STDMETHOD(SetInfoTip)(THIS_ LVSETINFOTIP* const pInfoTip) PURE;
    STDMETHOD(GetOutlineColor)(THIS_ COLORREF* pColor) PURE;
    STDMETHOD(SetOutlineColor)(THIS_ COLORREF color, COLORREF* pOldColor) PURE;
    STDMETHOD(GetFrozenItem)(THIS_ PINT pItemIndex) PURE;
    STDMETHOD(SetFrozenItem)(THIS_ int unknown1, int unknown2) PURE;
    STDMETHOD(GetFrozenSlot)(THIS_ RECT* pUnknown) PURE;
    STDMETHOD(SetFrozenSlot)(THIS_ int unknown1, POINT const* pUnknown2) PURE;
    STDMETHOD(GetViewMargin)(THIS_ RECT* pMargin) PURE;
    STDMETHOD(SetViewMargin)(THIS_ RECT const* pMargin) PURE;
    STDMETHOD(SetKeyboardSelected)(THIS_ LVITEMINDEX itemIndex) PURE;
    STDMETHOD(MapIndexToId)(THIS_ int itemIndex, PINT pItemID) PURE;
    STDMETHOD(MapIdToIndex)(THIS_ int itemID, PINT pItemIndex) PURE;
    STDMETHOD(IsItemVisible)(THIS_ LVITEMINDEX itemIndex, BOOL* pVisible) PURE;
    STDMETHOD(EnableAlphaShadow)(THIS_ BOOL enable) PURE;
    STDMETHOD(GetGroupSubsetCount)(THIS_ PINT pNumberOfRowsDisplayed) PURE;
    STDMETHOD(SetGroupSubsetCount)(THIS_ int numberOfRowsToDisplay) PURE;
    STDMETHOD(GetVisibleSlotCount)(THIS_ PINT pCount) PURE;
    STDMETHOD(GetColumnMargin)(THIS_ RECT* pMargin) PURE;
    STDMETHOD(SetSubItemCallback)(THIS_ LPVOID pCallback) PURE;
    STDMETHOD(GetVisibleItemRange)(THIS_ LVITEMINDEX* pFirstItem, LVITEMINDEX* pLastItem) PURE;
    STDMETHOD(SetTypeAheadFlags)(THIS_ UINT mask, UINT flags) PURE; // HRESULT SetTypeAheadFlags (TYPEAHEAD_FLAGS, TYPEAHEAD_FLAGS);

    // Windows 10
    STDMETHOD(SetWorkAreasWithDpi)(THIS_ int, struct tagLVWORKAREAWITHDPI const*) PURE;
    STDMETHOD(GetWorkAreasWithDpi)(THIS_ int, struct tagLVWORKAREAWITHDPI*) PURE;
    STDMETHOD(SetWorkAreaImageList)(THIS_ int, int, HIMAGELIST, HIMAGELIST*) PURE;
    STDMETHOD(GetWorkAreaImageList)(THIS_ int, int, HIMAGELIST*) PURE;
    STDMETHOD(EnableIconBullying)(THIS_ INT Mode) PURE;
    STDMETHOD(EnableQuirks)(THIS_ ULONG Flags) PURE;

    END_INTERFACE
};

#define IListViewQueryInterface(This,riid,ppvObject) \
    ((This)->lpVtbl->QueryInterface(This,riid,ppvObject))
#define IListView_AddRef(This) \
    ((This)->lpVtbl->AddRef(This))
#define IListView_Release(This) \
    ((This)->lpVtbl->Release(This))
#define IListView_GetWindow(This, WindowHandle) \
    ((This)->lpVtbl->AddRef(This))
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
#define IListView_SortItems(This, unknown, lParam, pComparisonFunction) \
    ((This)->lpVtbl->SortItems(This, unknown, lParam, pComparisonFunction))
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
#define IListView_GetGroupInfo(This, unknown1, unknown2, pGroup) \
    ((This)->lpVtbl->GetGroupInfo(This, unknown1, unknown2, pGroup))
#define IListView_SetGroupInfo(This, unknown, groupID, pGroup) \
    ((This)->lpVtbl->SetGroupInfo(This, unknown, groupID, pGroup))
#define IListView_GetGroupRect(This, unknown, groupID, rectangleType, pRectangle) \
    ((This)->lpVtbl->GetGroupRect(This, unknown, groupID, rectangleType, pRectangle))
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
