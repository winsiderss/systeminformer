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

#ifdef __cplusplus
extern "C"
{
#endif

typedef enum _PH_TOAST_REASON
{
    PhToastReasonUserCanceled,
    PhToastReasonApplicationHidden,
    PhToastReasonTimedOut,
    PhToastReasonActivated,
    PhToastReasonError,
    PhToastReasonUnknown

} PH_TOAST_REASON;

typedef VOID(NTAPI* PPH_TOAST_CALLBACK)(
    _In_ HRESULT Result,
    _In_ PH_TOAST_REASON Reason,
    _In_ PVOID Context
    );

_Must_inspect_result_
HRESULT
PhInitializeToastRuntime();

VOID
PhUninitializeToastRuntime();

_Must_inspect_result_
HRESULT PhShowToast(
    _In_ PCWSTR ApplicationId,
    _In_ PCWSTR ToastXml,
    _In_opt_ ULONG TimeoutMilliseconds,
    _In_opt_ PPH_TOAST_CALLBACK ToastCallback,
    _In_opt_ PVOID Context
    );

#ifdef __cplusplus
}
#endif
