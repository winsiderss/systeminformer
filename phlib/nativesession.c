/*
 * Copyright (c) 2022 Winsider Seminars & Solutions, Inc.  All rights reserved.
 *
 * This file is part of System Informer.
 *
 * Authors:
 *
 *     wj32    2009-2016
 *     dmex    2017-2024
 *
 */

#include <ph.h>
#include <winsta.h>

/**
 * Starts the remote control of another Terminal Services session.
 *
 * \param TargetServerName The name of the server where the session is located.
 * \param TargetSessionId The session ID of the session to remote control.
 * \param HotKeyVk The virtual key code that represents the hotkey to use to stop remote control.
 * \param HotkeyModifiers The virtual key modifiers for the hotkey.
 * \return NTSTATUS Successful or errant status.
 */
NTSTATUS NTAPI PhWinStationShadow(
    _In_ PCWSTR TargetServerName,
    _In_ ULONG TargetSessionId,
    _In_ UCHAR HotKeyVk,
    _In_ USHORT HotkeyModifiers
    )
{
    if (WinStationShadow(
        WINSTATION_CURRENT_SERVER,
        TargetServerName,
        TargetSessionId,
        HotKeyVk,
        HotkeyModifiers
        ))
    {
        return STATUS_SUCCESS;
    }

    return PhGetLastWin32ErrorAsNtStatus();
}

/**
 * Queries information for a specified window station.
 *
 * \param SessionId The session ID.
 * \param WinStationInformationClass The type of information to query.
 * \param WinStationInformation A buffer that receives the requested information.
 * \param WinStationInformationLength The size of the buffer, in bytes.
 * \param ReturnLength A variable that receives the number of bytes returned.
 * \return NTSTATUS Successful or errant status.
 */
NTSTATUS NTAPI PhWindowStationQueryInformation(
    _In_ ULONG SessionId,
    _In_ WINSTATIONINFOCLASS WinStationInformationClass,
    _Out_writes_bytes_(WinStationInformationLength) PVOID WinStationInformation,
    _In_ ULONG WinStationInformationLength,
    _Out_ PULONG ReturnLength
    )
{
    if (WinStationQueryInformationW(
        WINSTATION_CURRENT_SERVER,
        SessionId,
        WinStationInformationClass,
        WinStationInformation,
        WinStationInformationLength,
        ReturnLength))
    {
        return STATUS_SUCCESS;
    }

    return PhGetLastWin32ErrorAsNtStatus();
}

/**
 * Retrieves session information for a specified session.
 *
 * \param SessionId The session ID.
 * \param SessionInformation A buffer that receives the session information.
 * \return NTSTATUS Successful or errant status.
 */
NTSTATUS NTAPI PhGetWindowStationSessionInformation(
    _In_ ULONG SessionId,
    _Out_ PWINSTATIONINFORMATION SessionInformation
    )
{
    ULONG returnLength;

    return PhWindowStationQueryInformation(
        SessionId,
        WinStationInformation,
        SessionInformation,
        sizeof(WINSTATIONINFORMATION),
        &returnLength
        );
}

/**
 * Enumerates all sessions on the current server.
 *
 * \param SessionIds A variable that receives a pointer to an array of session IDs.
 * \param Count A variable that receives the number of sessions returned.
 * \return NTSTATUS Successful or errant status.
 */
NTSTATUS NTAPI PhWinStationEnumerate(
    _Out_ PSESSIONIDW* SessionIds,
    _Out_ PULONG Count
    )
{
    if (WinStationEnumerateW(
        WINSTATION_CURRENT_SERVER,
        SessionIds,
        Count
        ))
    {
        return STATUS_SUCCESS;
    }

    return PhGetLastWin32ErrorAsNtStatus();
}

/**
 * Sends a message to a specified session.
 *
 * \param SessionId The session ID.
 * \param Title The title of the message.
 * \param TitleLength The length of the title, in bytes.
 * \param Message The message text.
 * \param MessageLength The length of the message text, in bytes.
 * \param Style The style of the message box.
 * \param Timeout The timeout for the message box, in seconds.
 * \param Response A variable that receives the user's response.
 * \param DoNotWait Whether to wait for the user's response.
 * \return NTSTATUS Successful or errant status.
 */
NTSTATUS NTAPI PhWinStationSendMessage(
    _In_ ULONG SessionId,
    _In_ PCWSTR Title,
    _In_ ULONG TitleLength,
    _In_ PCWSTR Message,
    _In_ ULONG MessageLength,
    _In_ ULONG Style,
    _In_ ULONG Timeout,
    _Out_ PULONG Response,
    _In_ BOOLEAN DoNotWait
    )
{
    if (WinStationSendMessageW(
        WINSTATION_CURRENT_SERVER,
        SessionId,
        Title,
        TitleLength,
        Message,
        MessageLength,
        Style,
        Timeout,
        Response,
        DoNotWait
        ))
    {
        return STATUS_SUCCESS;
    }

    return PhGetLastWin32ErrorAsNtStatus();
}

/**
 * Connects to another Terminal Services session.
 *
 * \param SessionId The session ID.
 * \param TargetSessionId The session ID to connect to.
 * \param Password The password for the session.
 * \param Wait Whether to wait for the connection to complete.
 * \return NTSTATUS Successful or errant status.
 */
NTSTATUS NTAPI PhWinStationConnect(
    _In_ ULONG SessionId,
    _In_ ULONG TargetSessionId,
    _In_opt_ PCWSTR Password,
    _In_ BOOLEAN Wait
    )
{
    if (WinStationConnectW(
        WINSTATION_CURRENT_SERVER,
        SessionId,
        TargetSessionId,
        Password ? Password : L"",
        Wait
        ))
    {
        return STATUS_SUCCESS;
    }

    return PhGetLastWin32ErrorAsNtStatus();
}

/**
 * Disconnects a specified session.
 *
 * \param SessionId The session ID.
 * \param Wait Whether to wait for the disconnection to complete.
 * \return NTSTATUS Successful or errant status.
 */
NTSTATUS NTAPI PhWinStationDisconnect(
    _In_ ULONG SessionId,
    _In_ BOOLEAN Wait
    )
{
    if (WinStationDisconnect(
        WINSTATION_CURRENT_SERVER,
        SessionId,
        Wait
        ))
    {
        return STATUS_SUCCESS;
    }

    return PhGetLastWin32ErrorAsNtStatus();
}

/**
 * Resets a specified session.
 *
 * \param SessionId The session ID.
 * \param Wait Whether to wait for the reset to complete.
 * \return NTSTATUS Successful or errant status.
 */
NTSTATUS NTAPI PhWinStationReset(
    _In_ ULONG SessionId,
    _In_ BOOLEAN Wait
    )
{
    if (WinStationReset(
        WINSTATION_CURRENT_SERVER,
        SessionId,
        Wait
        ))
    {
        return STATUS_SUCCESS;
    }

    return PhGetLastWin32ErrorAsNtStatus();
}

/**
 * Frees memory allocated by Terminal Services functions.
 *
 * \param Buffer The buffer to free.
 */
VOID NTAPI PhWinStationFreeMemory(
    _In_ PVOID Buffer
    )
{
    WinStationFreeMemory(Buffer);
}
