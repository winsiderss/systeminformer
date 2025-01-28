/*
 * Copyright (c) 2022 Winsider Seminars & Solutions, Inc.  All rights reserved.
 *
 * This file is part of System Informer.
 *
 * Authors:
 *
 *     jxy-s   2022
 *     dmex    2022-2023
 *
 */

#ifndef PH_KSISUP_H
#define PH_KSISUP_H

VOID
PhShowKsiStatus(
    VOID
    );

VOID
PhInitializeKsi(
    VOID
    );

NTSTATUS
PhCleanupKsi(
    VOID
    );

PPH_STRING
PhGetKsiMessage(
    _In_opt_ NTSTATUS Status,
    _In_ BOOLEAN Force,
    _In_ PCWSTR Format,
    ...
    );

LONG
PhShowKsiMessage2(
    _In_opt_ HWND WindowHandle,
    _In_ ULONG Buttons,
    _In_opt_ PCWSTR Icon,
    _In_opt_ NTSTATUS Status,
    _In_ BOOLEAN Force,
    _In_ PCWSTR Title,
    _In_ PCWSTR Format,
    ...
    );

VOID
PhShowKsiMessageEx(
    _In_opt_ HWND WindowHandle,
    _In_opt_ PCWSTR Icon,
    _In_opt_ NTSTATUS Status,
    _In_ BOOLEAN Force,
    _In_ PCWSTR Title,
    _In_ PCWSTR Format,
    ...
    );

VOID
PhShowKsiMessage(
    _In_opt_ HWND WindowHandle,
    _In_opt_ PCWSTR Icon,
    _In_ PCWSTR Title,
    _In_ PCWSTR Format,
    ...
    );

PWSTR PhKsiStandardHelpText(
    VOID
    );

FORCEINLINE
VOID
PhShowKsiNotConnected(
    _In_opt_ HWND WindowHandle,
    _In_ PWSTR Message
    )
{
    PhShowKsiMessage(
        WindowHandle,
        TD_INFORMATION_ICON,
        L"Kernel driver not connected",
        L"%s\r\n\r\n"
        L"System Informer is not connected to the kernel driver or lacks the required state "
        L"necessary for this feature. Make sure that the \"Enable kernel-mode driver\" option is "
        L"enabled and that System Informer is running with administrator privileges.",
        Message
        );
}

FORCEINLINE
PPH_STRING
PhGetKsiNotConnectedString(
    _In_ PWSTR Message
    )
{
    return PhGetKsiMessage(
        0,
        FALSE,
        L"%s\r\n\r\n%s\r\n\r\n%s",
        L"Kernel driver not connected",
        L"System Informer is not connected to the kernel driver or lacks the required state "
        L"necessary for this feature. Make sure that the \"Enable kernel-mode driver\" option is "
        L"enabled and that System Informer is running with administrator privileges.",
        Message
        );
}

#endif
