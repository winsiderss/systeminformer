/*
 * Copyright (c) 2022 Winsider Seminars & Solutions, Inc.  All rights reserved.
 *
 * This file is part of System Informer.
 *
 * Authors:
 *
 *     jxy-s   2021
 *
 */

#pragma once
#include <phapp.h>

EXTERN_C_START

typedef enum _PH_TOAST_REASON
{
    PhToastReasonUserCanceled,
    PhToastReasonApplicationHidden,
    PhToastReasonTimedOut,
    PhToastReasonActivated,
    PhToastReasonError,
    PhToastReasonUnknown
} PH_TOAST_REASON;

typedef VOID (NTAPI* PPH_TOAST_CALLBACK)(
    _In_ HRESULT Result,
    _In_ PH_TOAST_REASON Reason,
    _In_ PVOID Context
    );

_Must_inspect_result_
HRESULT
NTAPI
PhInitializeToastRuntime();

VOID
NTAPI
PhUninitializeToastRuntime();

_Must_inspect_result_
HRESULT
NTAPI
PhShowToast(
    _In_ PCWSTR ApplicationId,
    _In_ PCWSTR ToastXml,
    _In_opt_ ULONG TimeoutMilliseconds,
    _In_opt_ PPH_TOAST_CALLBACK ToastCallback,
    _In_opt_ PVOID Context
    );

_Must_inspect_result_
HRESULT
NTAPI
PhShowToastStringRef(
    _In_ PPH_STRINGREF ApplicationId,
    _In_ PPH_STRINGREF ToastXml,
    _In_opt_ ULONG TimeoutMilliseconds,
    _In_opt_ PPH_TOAST_CALLBACK ToastCallback,
    _In_opt_ PVOID Context
    );

EXTERN_C_END
