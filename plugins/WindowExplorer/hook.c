/*
 * Process Hacker Window Explorer - 
 *   hook procedure
 * 
 * Copyright (C) 2011 wj32
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

#include "wndexp.h"

VOID WepCreateServerWindow();

LRESULT CALLBACK WepServerWndProc(
    __in HWND hWnd,
    __in UINT uMsg,
    __in WPARAM wParam,
    __in LPARAM lParam
    );

LRESULT CALLBACK WepCallWndProc(
    __in int nCode,
    __in WPARAM wParam,
    __in LPARAM lParam
    );

// Shared
ULONG WeServerMessage;
// Server
HHOOK WeHookHandle = NULL;
HWND WeServerWindowHandle;
// The current message ID is used to detect out-of-sync clients. If a client processes a 
// message after our timeout has expired and sends us late messages, we can identify and 
// reject them by looking at the message ID.
ULONG WeCurrentMessageId = 0;
HANDLE WeCurrentProcessId;
HANDLE WeCurrentProcessHandle;
WE_HOOK_REQUEST WeCurrentRequest;
WE_HOOK_REPLY WeCurrentReply;
HANDLE WeCurrentRequestEndEvent = NULL;
PH_QUEUED_LOCK WeRequestLock = PH_QUEUED_LOCK_INIT;

// Server

VOID WeHookServerInitialization()
{
    if (WeHookHandle)
        return;

    WeServerMessage = RegisterWindowMessage(WE_SERVER_MESSAGE_NAME);

    if (!WeServerWindowHandle)
        WepCreateServerWindow();

    WeHookHandle = SetWindowsHookEx(WH_CALLWNDPROC, WepCallWndProc, PluginInstance->DllBase, 0);
}

VOID WeHookServerUninitialization()
{
    if (WeHookHandle)
    {
        UnhookWindowsHookEx(WeHookHandle);
        WeHookHandle = NULL;
    }
}

VOID WepCreateServerWindow()
{
    WNDCLASSEX wcex;

    memset(&wcex, 0, sizeof(WNDCLASSEX));
    wcex.cbSize = sizeof(WNDCLASSEX);
    wcex.lpfnWndProc = WepServerWndProc;
    wcex.hInstance = PluginInstance->DllBase;
    wcex.lpszClassName = L"WeServerWindow";

    RegisterClassEx(&wcex);

    WeServerWindowHandle = CreateWindow(
        L"WeServerWindow",
        L"",
        0,
        0,
        0,
        0,
        0,
        HWND_MESSAGE,
        NULL,
        PluginInstance->DllBase,
        NULL
        );

    // Allow client messages through UIPI.
    if (*(PULONG)WeGetProcedureAddress("WindowsVersion") >= WINDOWS_VISTA)
    {
        _ChangeWindowMessageFilter changeWindowMessageFilter;

        if (changeWindowMessageFilter = PhGetProcAddress(L"user32.dll", "ChangeWindowMessageFilter"))
        {
            changeWindowMessageFilter(WM_WE_RECEIVE_REQUEST, MSGFLT_ADD);
            changeWindowMessageFilter(WM_WE_SEND_REPLY, MSGFLT_ADD);
        }
    }
}

LRESULT CALLBACK WepServerWndProc(
    __in HWND hWnd,
    __in UINT uMsg,
    __in WPARAM wParam,
    __in LPARAM lParam
    )
{
    LRESULT result;

    result = 0;

    switch (uMsg)
    {
    case WM_CREATE:
        return 0;
    case WM_NCCREATE:
        return TRUE;
    case WM_WE_RECEIVE_REQUEST:
    case WM_WE_SEND_REPLY:
        {
            ULONG messageId;
            ULONG_PTR parameter;

            messageId = (ULONG)wParam;
            parameter = lParam;

            if (messageId != WeCurrentMessageId)
                return 0;

            switch (uMsg)
            {
            case WM_WE_RECEIVE_REQUEST:
                {
                    if (NT_SUCCESS(PhWriteVirtualMemory(WeCurrentProcessHandle, (PVOID)parameter, &WeCurrentRequest, sizeof(WE_HOOK_REQUEST), NULL)))
                        result = WE_CLIENT_MESSAGE_MAGIC;
                }
                break;
            case WM_WE_SEND_REPLY:
                {
                    if (NT_SUCCESS(PhReadVirtualMemory(WeCurrentProcessHandle, (PVOID)parameter, &WeCurrentReply, sizeof(WE_HOOK_REPLY), NULL)))
                        result = WE_CLIENT_MESSAGE_MAGIC;

                    NtSetEvent(WeCurrentRequestEndEvent, NULL);
                }
                break;
            }
        }
        break;
    }

    return result;
}

BOOLEAN WeSendServerRequest(
    __in HWND hWnd,
    __in PWE_HOOK_REQUEST Request,
    __out PWE_HOOK_REPLY Reply
    )
{
    BOOLEAN result;
    ULONG threadId;
    ULONG processId;
    HANDLE processHandle;

    threadId = GetWindowThreadProcessId(hWnd, &processId);

    if (threadId == 0)
        return FALSE;

    if (!NT_SUCCESS(PhOpenProcess(&processHandle, PROCESS_VM_READ | PROCESS_VM_WRITE, UlongToHandle(processId))))
        return FALSE;

    result = TRUE;

    PhAcquireQueuedLockExclusive(&WeRequestLock);

    if (WeCurrentRequestEndEvent)
    {
        NtResetEvent(WeCurrentRequestEndEvent, NULL);
    }
    else
    {
        if (!NT_SUCCESS(NtCreateEvent(&WeCurrentRequestEndEvent, EVENT_ALL_ACCESS, NULL, NotificationEvent, FALSE)))
        {
            PhReleaseQueuedLockExclusive(&WeRequestLock);
            NtClose(processHandle);
            return FALSE;
        }
    }

    WeCurrentMessageId++;
    WeCurrentProcessId = UlongToHandle(processId);
    WeCurrentProcessHandle = processHandle;
    memcpy(&WeCurrentRequest, Request, sizeof(WE_HOOK_REQUEST));
    memset(&WeCurrentReply, 0, sizeof(WE_HOOK_REPLY));

    // Call the client and begin processing client messages.

    SendNotifyMessage(hWnd, WeServerMessage, (WPARAM)WeServerWindowHandle, WeCurrentMessageId);

    if (PhWaitForMultipleObjectsAndPump(WeServerWindowHandle, 1, &WeCurrentRequestEndEvent, WE_CLIENT_MESSAGE_TIMEOUT) == STATUS_WAIT_0)
    {
        memcpy(Reply, &WeCurrentReply, sizeof(WE_HOOK_REPLY));
    }
    else
    {
        result = FALSE;
    }

    PhReleaseQueuedLockExclusive(&WeRequestLock);

    NtClose(processHandle);

    return result;
}

// Client

VOID WeHookClientInitialization()
{
    WeServerMessage = RegisterWindowMessage(WE_SERVER_MESSAGE_NAME);
}

VOID WeHookClientUninitialization()
{
    NOTHING;
}

BOOLEAN WepSendClientMessage(
    __in HWND hWnd,
    __in UINT Msg,
    __in ULONG MessageId,
    __in LPARAM lParam
    )
{
    ULONG_PTR result;

    if (!SendMessageTimeout(hWnd, Msg, MessageId, lParam, SMTO_BLOCK, WE_CLIENT_MESSAGE_TIMEOUT, &result))
        return FALSE;

    if (result != WE_CLIENT_MESSAGE_MAGIC)
        return FALSE;

    return TRUE;
}

LRESULT CALLBACK WepCallWndProc(
    __in int nCode,
    __in WPARAM wParam,
    __in LPARAM lParam
    )
{
    LRESULT result;
    PCWPSTRUCT info;

    result = CallNextHookEx(NULL, nCode, wParam, lParam);

    info = (PCWPSTRUCT)lParam;

    if (info->message == WeServerMessage)
    {
        HWND serverWindow;
        ULONG messageId;
        WE_HOOK_REQUEST request;
        WE_HOOK_REPLY reply;

        serverWindow = (HWND)info->wParam;
        messageId = (ULONG)info->lParam;

        // Read the server's request.
        if (!WepSendClientMessage(serverWindow, WM_WE_RECEIVE_REQUEST, messageId, (LPARAM)&request))
            goto ExitProc;

        memset(&reply, 0, sizeof(WE_HOOK_REPLY));

        switch (request.Type)
        {
        case GetWindowLongHookRequest:
            {
                reply.u.Data = GetWindowLongPtr(info->hwnd, (LONG)request.Parameter1);
            }
            break;
        }

        // Send our data back to the server.
        WepSendClientMessage(serverWindow, WM_WE_SEND_REPLY, messageId, (LPARAM)&reply);
    }

ExitProc:
    return result;
}
