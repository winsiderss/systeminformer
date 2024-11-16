/*
 * Copyright (c) 2022 Winsider Seminars & Solutions, Inc.  All rights reserved.
 *
 * This file is part of System Informer.
 *
 * Authors:
 *
 *     dmex    2012-2023
 *     jxy-s   2023-2024
 *
 */

#ifndef _PH_SEARCHBOX_H
#define _PH_SEARCHBOX_H

#ifdef __cplusplus
extern "C" {
#endif

typedef
VOID
NTAPI
PH_SEARCHCONTROL_CALLBACK(
    _In_ ULONG_PTR MatchHandle,
    _In_opt_ PVOID Context
    );
typedef PH_SEARCHCONTROL_CALLBACK* PPH_SEARCHCONTROL_CALLBACK;

PHLIBAPI
VOID
NTAPI
PhCreateSearchControlEx(
    _In_ HWND ParentWindowHandle,
    _In_ HWND WindowHandle,
    _In_opt_ PCWSTR BannerText,
    _In_ PCWSTR SearchButtonResource,
    _In_ PCWSTR SearchButtonActiveResource,
    _In_ PCWSTR CaseButtonResource,
    _In_ PCWSTR RegexButtonResource,
    _In_ PCWSTR RegexSetting,
    _In_ PCWSTR CaseSetting,
    _In_ PPH_SEARCHCONTROL_CALLBACK Callback,
    _In_opt_ PVOID Context
    );

PHLIBAPI
BOOLEAN
NTAPI
PhSearchControlMatch(
    _In_ ULONG_PTR MatchHandle,
    _In_ PPH_STRINGREF Text
    );

PHLIBAPI
BOOLEAN
NTAPI
PhSearchControlMatchZ(
    _In_ ULONG_PTR MatchHandle,
    _In_ PCWSTR Text
    );

PHLIBAPI
BOOLEAN
NTAPI
PhSearchControlMatchLongHintZ(
    _In_ ULONG_PTR MatchHandle,
    _In_ PCWSTR Text
    );

PHLIBAPI
BOOLEAN
NTAPI
PhSearchControlMatchPointer(
    _In_ ULONG_PTR MatchHandle,
    _In_ PVOID Pointer
    );

PHLIBAPI
BOOLEAN
NTAPI
PhSearchControlMatchPointerRange(
    _In_ ULONG_PTR MatchHandle,
    _In_ PVOID Pointer,
    _In_ SIZE_T Size
    );

#ifdef __cplusplus
}
#endif

#endif
