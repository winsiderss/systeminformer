/*
 * Copyright (c) 2022 Winsider Seminars & Solutions, Inc.  All rights reserved.
 *
 * This file is part of System Informer.
 *
 * Authors:
 *
 *     wj32    2010-2016
 *     dmex    2011-2023
 *
 */

#include "toolstatus.h"
#include <toolstatusintf.h>

_Function_class_(PH_HASHTABLE_EQUAL_FUNCTION)
BOOLEAN PluginInterfaceTabInfoHashtableEqualFunction(
    _In_ PVOID Entry1,
    _In_ PVOID Entry2
    );

_Function_class_(PH_HASHTABLE_HASH_FUNCTION)
ULONG PluginInterfaceTabInfoHashtableHashFunction(
    _In_ PVOID Entry
    );

ULONG_PTR PluginInterfaceGetSearchMatchHandle(
    VOID
    );

VOID PluginInterfaceRegisterTabSearch(
    _In_ LONG TabIndex,
    _In_ PCPH_STRINGREF BannerText
    );

PTOOLSTATUS_TAB_INFO PluginInterfaceRegisterTabInfo(
    _In_ LONG TabIndex,
    _In_opt_ PCPH_STRINGREF BannerText
    );

static PPH_HASHTABLE TabInfoHashtable = NULL;
static PH_CALLBACK_DECLARE(PluginInterfaceSearchChangedEvent);

CONST TOOLSTATUS_INTERFACE PluginInterface =
{
    TOOLSTATUS_INTERFACE_VERSION,
    PluginInterfaceGetSearchMatchHandle,
    SearchWordMatchStringRefCallback,
    PluginInterfaceRegisterTabSearch,
    &PluginInterfaceSearchChangedEvent,
    PluginInterfaceRegisterTabInfo,
    ToolbarRegisterGraph
};

VOID PluginInterfaceInitialize(
    VOID
    )
{
    TabInfoHashtable = PhCreateHashtable(
        sizeof(TOOLSTATUS_TAB_INFO),
        PluginInterfaceTabInfoHashtableEqualFunction,
        PluginInterfaceTabInfoHashtableHashFunction,
        3
        );
}

_Function_class_(PH_HASHTABLE_EQUAL_FUNCTION)
BOOLEAN PluginInterfaceTabInfoHashtableEqualFunction(
    _In_ PVOID Entry1,
    _In_ PVOID Entry2
    )
{
    PTOOLSTATUS_TAB_INFO entry1 = (PTOOLSTATUS_TAB_INFO)Entry1;
    PTOOLSTATUS_TAB_INFO entry2 = (PTOOLSTATUS_TAB_INFO)Entry2;

    return entry1->Index == entry2->Index;
}

_Function_class_(PH_HASHTABLE_HASH_FUNCTION)
ULONG PluginInterfaceTabInfoHashtableHashFunction(
    _In_ PVOID Entry
    )
{
    PTOOLSTATUS_TAB_INFO entry = (PTOOLSTATUS_TAB_INFO)Entry;

    return PhHashInt32(entry->Index);
}

ULONG_PTR PluginInterfaceGetSearchMatchHandle(
    VOID
    )
{
    return SearchMatchHandle;
}

VOID PluginInterfaceInvokeSearchChangedEvent(
    _In_ ULONG_PTR Context
    )
{
    PhInvokeCallback(&PluginInterfaceSearchChangedEvent, (PVOID)Context);
}

VOID PluginInterfaceRegisterTabSearch(
    _In_ LONG TabIndex,
    _In_ PCPH_STRINGREF BannerText
    )
{
    PluginInterfaceRegisterTabInfo(TabIndex, BannerText);
}

PTOOLSTATUS_TAB_INFO PluginInterfaceRegisterTabInfo(
    _In_ LONG TabIndex,
    _In_opt_ PCPH_STRINGREF BannerText
    )
{
    TOOLSTATUS_TAB_INFO lookupEntry;
    PTOOLSTATUS_TAB_INFO entry;

    RtlZeroMemory(&lookupEntry, sizeof(TOOLSTATUS_TAB_INFO));
    lookupEntry.Index = TabIndex;
    lookupEntry.BannerText = BannerText;

    if (entry = PhAddEntryHashtable(TabInfoHashtable, &lookupEntry))
        return entry;

    return NULL;
}

PTOOLSTATUS_TAB_INFO FindTabInfo(
    _In_ LONG TabIndex
    )
{
    TOOLSTATUS_TAB_INFO lookupEntry;
    PTOOLSTATUS_TAB_INFO entry;

    lookupEntry.Index = TabIndex;

    if (entry = PhFindEntryHashtable(TabInfoHashtable, &lookupEntry))
        return entry;

    return NULL;
}

HWND GetTabIndexTreeNewHandle(
    _In_ LONG TabIndex
    )
{
    PTOOLSTATUS_TAB_INFO tabInfo;

    if (tabInfo = FindTabInfo(TabIndex))
    {
        return tabInfo->GetTreeNewHandle();
    }

    return NULL;
}

PPH_STRING GetTabIndexBannerText(
    _In_ LONG TabIndex,
    _In_opt_ PCPH_STRINGREF AppendString
    )
{
    PTOOLSTATUS_TAB_INFO tabInfo;

    if ((tabInfo = FindTabInfo(TabIndex)) && tabInfo->BannerText)
    {
        if (AppendString)
            return PhConcatStringRef2(tabInfo->BannerText, AppendString);
        else
            return PhCreateString2(tabInfo->BannerText);
    }

    return NULL;
}
