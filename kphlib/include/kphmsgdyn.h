/*
 * Copyright (c) 2022 Winsider Seminars & Solutions, Inc.  All rights reserved.
 *
 * This file is part of System Informer.
 *
 * Authors:
 *
 *     jxy-s   2022
 *
 */

#pragma once
#include <kphmsg.h>

#ifdef __cplusplus
extern "C"
{
#endif

VOID KphMsgDynClear(
    _Inout_ PKPH_MESSAGE Message
    );

_Must_inspect_result_
NTSTATUS KphMsgDynAddUnicodeString(
    _Inout_ PKPH_MESSAGE Message,
    _In_ KPH_MESSAGE_FIELD_ID FieldId,
    _In_ PCUNICODE_STRING String
    );

_Must_inspect_result_
NTSTATUS KphMsgDynAddUnicodeString(
    _Inout_ PKPH_MESSAGE Message,
    _In_ KPH_MESSAGE_FIELD_ID FieldId,
    _In_ PCUNICODE_STRING String
    );

_Must_inspect_result_
NTSTATUS KphMsgDynGetUnicodeString(
    _In_ PCKPH_MESSAGE Message,
    _In_ KPH_MESSAGE_FIELD_ID FieldId,
    _Out_ PUNICODE_STRING String
    );

_Must_inspect_result_
NTSTATUS KphMsgDynAddAnsiString(
    _Inout_ PKPH_MESSAGE Message,
    _In_ KPH_MESSAGE_FIELD_ID FieldId,
    _In_ PCANSI_STRING String
    );

_Must_inspect_result_
NTSTATUS KphMsgDynGetAnsiString(
    _In_ PCKPH_MESSAGE Message,
    _In_ KPH_MESSAGE_FIELD_ID FieldId,
    _Out_ PANSI_STRING String
    );

#ifdef __cplusplus
}
#endif
