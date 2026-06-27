/*
 * Copyright (c) 2022 Winsider Seminars & Solutions, Inc.  All rights reserved.
 *
 * This file is part of System Informer.
 *
 * Authors:
 *
 *     dmex    2017-2023
 *
 */

#include "wndexp.h"

PPH_OBJECT_TYPE PvpPropContextType = NULL;
PPH_OBJECT_TYPE PvpPropPageContextType = NULL;

_Function_class_(PH_TYPE_DELETE_PROCEDURE)
VOID NTAPI PvpPropContextDeleteProcedure(
    _In_ PVOID Object,
    _In_ ULONG Flags
    );

_Function_class_(PH_TYPE_DELETE_PROCEDURE)
VOID NTAPI PvpPropPageContextDeleteProcedure(
    _In_ PVOID Object,
    _In_ ULONG Flags
    );

VOID NTAPI PvpDereferencePropPageContext(
    _In_opt_ PVOID Context
    )
{
    if (Context)
        PhDereferenceObject(Context);
}

PPV_PROPCONTEXT HdCreatePropContext(
    _In_ PWSTR Caption,
    _In_opt_ PVOID Context,
    _In_opt_ PPH_PROPSHEETNEW_INITIALIZED_CALLBACK InitializedCallback
    )
{
    PPV_PROPCONTEXT propContext;
    PH_PROPSHEETNEW sheet;

    if (!PvpPropContextType)
        PvpPropContextType = PhCreateObjectType(L"HdPropContext2", 0, PvpPropContextDeleteProcedure);

    propContext = PhCreateObject(sizeof(PV_PROPCONTEXT), PvpPropContextType);
    memset(propContext, 0, sizeof(PV_PROPCONTEXT));

    memset(&sheet, 0, sizeof(PH_PROPSHEETNEW));
    sheet.Caption = Caption;
    sheet.Layout = PhPropSheetNewLayoutTop;
    sheet.Skin = PhTabNewSkinUxTheme;
    sheet.Flags = PH_PROPSHEETNEW_RESIZABLE |
        PH_PROPSHEETNEW_CENTER |
        PH_PROPSHEETNEW_SAVE_PLACEMENT |
        PH_PROPSHEETNEW_SAVE_ACTIVE_PAGE |
        PH_PROPSHEETNEW_CLOSE_BUTTON;
    sheet.SettingNamePosition = SETTING_NAME_WINDOWS_PROPERTY_POSITION;
    sheet.SettingNameSize = SETTING_NAME_WINDOWS_PROPERTY_SIZE;
    sheet.SettingNameActivePage = SETTING_NAME_WINDOWS_PROPERTY_PAGE;
    sheet.Context = Context;
    sheet.Initialized = InitializedCallback;
    propContext->Builder = PhPropSheetNewBuilderCreate(&sheet);

    return propContext;
}

_Function_class_(PH_TYPE_DELETE_PROCEDURE)
VOID NTAPI PvpPropContextDeleteProcedure(
    _In_ PVOID Object,
    _In_ ULONG Flags
    )
{
    PPV_PROPCONTEXT propContext = (PPV_PROPCONTEXT)Object;

    PhPropSheetNewBuilderDestroy(propContext->Builder);
}

_Function_class_(PH_TYPE_DELETE_PROCEDURE)
VOID NTAPI PvpPropPageContextDeleteProcedure(
    _In_ PVOID Object,
    _In_ ULONG Flags
    )
{
    PPV_PROPPAGECONTEXT propPageContext = (PPV_PROPPAGECONTEXT)Object;

    if (propPageContext->Context)
        PhDereferenceObject(propPageContext->Context);
}

_Function_class_(PH_WINDOW_ENUM_CALLBACK)
BOOLEAN CALLBACK PvRefreshButtonWindowEnumCallback(
    _In_ HWND WindowHandle,
    _In_opt_ PVOID Context
    )
{
    WCHAR className[256];

    if (!NT_SUCCESS(PhGetClassName(WindowHandle, className, RTL_NUMBER_OF(className), NULL)))
        className[0] = UNICODE_NULL;

    if (PhEqualStringZ(className, L"#32770", TRUE))
    {
        SendMessage(WindowHandle, WM_PH_UPDATE_DIALOG, 0, 0);
    }

    return TRUE;
}

VOID PvRefreshChildWindows(
    _In_ HWND WindowHandle
    )
{
    HWND propSheetHandle;

    if (propSheetHandle = GetAncestor(WindowHandle, GA_ROOT))
    {
        PhEnumChildWindows(propSheetHandle, PvRefreshButtonWindowEnumCallback, NULL);
    }
}

BOOLEAN PvAddPropPage(
    _Inout_ PPV_PROPCONTEXT PropContext,
    _In_ _Assume_refs_(1) PPV_PROPPAGECONTEXT PropPageContext
    )
{
    PH_PROPSHEETNEW_PAGE_DESCRIPTOR descriptor;

    memset(&descriptor, 0, sizeof(PH_PROPSHEETNEW_PAGE_DESCRIPTOR));
    descriptor.Id = PropPageContext->Id;
    descriptor.Name = PropPageContext->Name;
    descriptor.Instance = PropPageContext->Instance;
    descriptor.Template = PropPageContext->Template;
    descriptor.DialogProc = PropPageContext->DialogProc;
    descriptor.Context = PropPageContext;
    descriptor.DeleteContext = PvpDereferencePropPageContext;

    return PhPropSheetNewBuilderAddPage(PropContext->Builder, &descriptor);
}

PPV_PROPPAGECONTEXT PvCreatePropPageContext(
    _In_ PCWSTR Id,
    _In_ PCWSTR Name,
    _In_ LPCWSTR Template,
    _In_ DLGPROC DlgProc,
    _In_opt_ PVOID Context
    )
{
    return PvCreatePropPageContextEx(PluginInstance->DllBase, Id, Name, Template, DlgProc, Context);
}

PPV_PROPPAGECONTEXT PvCreatePropPageContextEx(
    _In_opt_ PVOID InstanceHandle,
    _In_ PCWSTR Id,
    _In_ PCWSTR Name,
    _In_ LPCWSTR Template,
    _In_ DLGPROC DlgProc,
    _In_opt_ PVOID Context
    )
{
    PPV_PROPPAGECONTEXT propPageContext;

    if (!PvpPropPageContextType)
        PvpPropPageContextType = PhCreateObjectType(L"HdPropPageContext2", 0, PvpPropPageContextDeleteProcedure);

    propPageContext = PhCreateObject(sizeof(PV_PROPPAGECONTEXT), PvpPropPageContextType);
    memset(propPageContext, 0, sizeof(PV_PROPPAGECONTEXT));

    propPageContext->Context = Context;
    propPageContext->Instance = InstanceHandle;
    propPageContext->Template = Template;
    propPageContext->DialogProc = DlgProc;
    propPageContext->Id = Id;
    propPageContext->Name = Name;

    return propPageContext;
}

#ifdef DEBUG
static VOID ASSERT_DIALOGRECT(
    _In_ PVOID DllBase,
    _In_ PCWSTR Name,
    _In_ SHORT Width,
    _In_ SHORT Height
    )
{
    PDLGTEMPLATEEX dialogTemplate = NULL;

    PhLoadResource(DllBase, Name, RT_DIALOG, NULL, &dialogTemplate);

    assert(dialogTemplate && dialogTemplate->cx == Width && dialogTemplate->cy == Height);
}
#endif

PPH_LAYOUT_ITEM PvAddPropPageLayoutItem(
    _In_ HWND WindowHandle,
    _In_ HWND Handle,
    _In_ PPH_LAYOUT_ITEM ParentItem,
    _In_ ULONG Anchor
    )
{
    PPV_PROPPAGECONTEXT propPageContext;

    propPageContext = PhGetWindowContext(WindowHandle, 0xfff);

    return PhPropSheetNewPageAddLayoutItemEx(
        WindowHandle,
        Handle,
        ParentItem == PH_PROP_PAGE_TAB_CONTROL_PARENT ? PH_PROPSHEETNEW_PAGE_LAYOUT_PARENT : ParentItem,
        Anchor,
        propPageContext ? propPageContext->Instance : NULL,
        propPageContext ? propPageContext->Template : NULL
        );
}

/**
 * \brief PvDoPropPageLayout
 *
 * \param WindowHandle
 */

VOID PvDoPropPageLayout(
    _In_ HWND WindowHandle
    )
{
    PhPropSheetNewPageLayout(WindowHandle);
}
