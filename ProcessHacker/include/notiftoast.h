/*
 * Process Hacker -
 *   Toast Notification
 *
 * Copyright (C) 2021 jxy-s
 *
 * This file is part of Process Hacker.
 *
 * Process Hacker is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation, either version 3 of the License, or
 * (at your option) any later version.
 *
 * Process Hacker is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with Process Hacker.  If not, see <http://www.gnu.org/licenses/>.
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
