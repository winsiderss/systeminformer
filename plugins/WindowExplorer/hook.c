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

BOOLEAN WepCreateServerObjects();

BOOLEAN WepOpenServerObjects();

VOID WepCloseServerObjects();

VOID WepWriteClientData(
    __in HWND hwnd
    );

LRESULT CALLBACK WepCallWndProc(
    __in int nCode,
    __in WPARAM wParam,
    __in LPARAM lParam
    );

// Shared
ULONG WeServerMessage;
HANDLE WeServerSharedSection;
PWE_HOOK_SHARED_DATA WeServerSharedData;
HANDLE WeServerSharedSectionLock;
HANDLE WeServerSharedSectionEvent;
// Server
HHOOK WeHookHandle = NULL;
// The current message ID is used to detect out-of-sync clients. If a client processes a 
// message after our timeout has expired and sends us late messages, we can identify and 
// reject them by looking at the message ID.
ULONG WeCurrentMessageId = 0;
PH_QUEUED_LOCK WeRequestLock = PH_QUEUED_LOCK_INIT;

// Server

VOID WeHookServerInitialization()
{
    if (WeHookHandle)
        return;

    WeServerMessage = RegisterWindowMessage(WE_SERVER_MESSAGE_NAME);

    if (!WepCreateServerObjects())
        return;

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

BOOLEAN WepCreateServerObjects()
{
    OBJECT_ATTRIBUTES objectAttributes;
    WCHAR buffer[256];
    UNICODE_STRING objectName;

    if (!WeServerSharedSection)
    {
        LARGE_INTEGER maximumSize;

        WeFormatLocalObjectName(WE_SERVER_SHARED_SECTION_NAME, buffer, &objectName);
        InitializeObjectAttributes(&objectAttributes, &objectName, OBJ_CASE_INSENSITIVE, NULL, NULL);
        maximumSize.QuadPart = sizeof(WE_HOOK_SHARED_DATA);

        if (!NT_SUCCESS(NtCreateSection(
            &WeServerSharedSection,
            SECTION_ALL_ACCESS,
            &objectAttributes,
            &maximumSize,
            PAGE_READWRITE,
            SEC_COMMIT,
            NULL
            )))
        {
            return FALSE;
        }
    }

    if (!WeServerSharedData)
    {
        PVOID viewBase;
        SIZE_T viewSize;

        viewBase = NULL;
        viewSize = sizeof(WE_HOOK_SHARED_DATA);

        if (!NT_SUCCESS(NtMapViewOfSection(
            WeServerSharedSection,
            NtCurrentProcess(),
            &viewBase,
            0,
            0,
            NULL,
            &viewSize,
            ViewShare,
            0,
            PAGE_READWRITE
            )))
        {
            WepCloseServerObjects();
            return FALSE;
        }

        WeServerSharedData = viewBase;
    }

    if (!WeServerSharedSectionLock)
    {
        WeFormatLocalObjectName(WE_SERVER_SHARED_SECTION_LOCK_NAME, buffer, &objectName);
        InitializeObjectAttributes(&objectAttributes, &objectName, OBJ_CASE_INSENSITIVE, NULL, NULL);

        if (!NT_SUCCESS(NtCreateMutant(
            &WeServerSharedSectionLock,
            MUTANT_ALL_ACCESS,
            &objectAttributes,
            FALSE
            )))
        {
            WepCloseServerObjects();
            return FALSE;
        }
    }

    if (!WeServerSharedSectionEvent)
    {
        WeFormatLocalObjectName(WE_SERVER_SHARED_SECTION_EVENT_NAME, buffer, &objectName);
        InitializeObjectAttributes(&objectAttributes, &objectName, OBJ_CASE_INSENSITIVE, NULL, NULL);

        if (!NT_SUCCESS(NtCreateEvent(
            &WeServerSharedSectionEvent,
            EVENT_ALL_ACCESS,
            &objectAttributes,
            NotificationEvent,
            FALSE
            )))
        {
            WepCloseServerObjects();
            return FALSE;
        }
    }

    return TRUE;
}

BOOLEAN WepOpenServerObjects()
{
    OBJECT_ATTRIBUTES objectAttributes;
    WCHAR buffer[256];
    UNICODE_STRING objectName;

    if (!WeServerSharedSection)
    {
        WeFormatLocalObjectName(WE_SERVER_SHARED_SECTION_NAME, buffer, &objectName);
        InitializeObjectAttributes(&objectAttributes, &objectName, OBJ_CASE_INSENSITIVE, NULL, NULL);

        if (!NT_SUCCESS(NtOpenSection(
            &WeServerSharedSection,
            SECTION_ALL_ACCESS,
            &objectAttributes
            )))
        {
            return FALSE;
        }
    }

    if (!WeServerSharedData)
    {
        PVOID viewBase;
        SIZE_T viewSize;

        viewBase = NULL;
        viewSize = sizeof(WE_HOOK_SHARED_DATA);

        if (!NT_SUCCESS(NtMapViewOfSection(
            WeServerSharedSection,
            NtCurrentProcess(),
            &viewBase,
            0,
            0,
            NULL,
            &viewSize,
            ViewShare,
            0,
            PAGE_READWRITE
            )))
        {
            WepCloseServerObjects();
            return FALSE;
        }

        WeServerSharedData = viewBase;
    }

    if (!WeServerSharedSectionLock)
    {
        WeFormatLocalObjectName(WE_SERVER_SHARED_SECTION_LOCK_NAME, buffer, &objectName);
        InitializeObjectAttributes(&objectAttributes, &objectName, OBJ_CASE_INSENSITIVE, NULL, NULL);

        if (!NT_SUCCESS(NtOpenMutant(
            &WeServerSharedSectionLock,
            MUTANT_ALL_ACCESS,
            &objectAttributes
            )))
        {
            WepCloseServerObjects();
            return FALSE;
        }
    }

    if (!WeServerSharedSectionEvent)
    {
        WeFormatLocalObjectName(WE_SERVER_SHARED_SECTION_EVENT_NAME, buffer, &objectName);
        InitializeObjectAttributes(&objectAttributes, &objectName, OBJ_CASE_INSENSITIVE, NULL, NULL);

        if (!NT_SUCCESS(NtOpenEvent(
            &WeServerSharedSectionEvent,
            EVENT_ALL_ACCESS,
            &objectAttributes
            )))
        {
            WepCloseServerObjects();
            return FALSE;
        }
    }

    return TRUE;
}

VOID WepCloseServerObjects()
{
    if (WeServerSharedSection)
    {
        NtClose(WeServerSharedSection);
        WeServerSharedSection = NULL;
    }

    if (WeServerSharedData)
    {
        NtUnmapViewOfSection(NtCurrentProcess(), WeServerSharedData);
        WeServerSharedData = NULL;
    }

    if (WeServerSharedSectionLock)
    {
        NtClose(WeServerSharedSectionLock);
        WeServerSharedSectionLock = NULL;
    }

    if (WeServerSharedSectionEvent)
    {
        NtClose(WeServerSharedSectionEvent);
        WeServerSharedSectionEvent = NULL;
    }
}

VOID WeLockServerSharedData(
    __out PWE_HOOK_SHARED_DATA *Data
    )
{
    PhAcquireQueuedLockExclusive(&WeRequestLock);
    *Data = WeServerSharedData;
}

VOID WeUnlockServerSharedData()
{
    PhReleaseQueuedLockExclusive(&WeRequestLock);
}

BOOLEAN WeSendServerRequest(
    __in HWND hWnd
    )
{
    ULONG threadId;
    ULONG processId;
    LARGE_INTEGER timeout;

    if (!WeServerSharedData || !WeServerSharedSectionLock || !WeServerSharedSectionEvent)
        return FALSE;

    threadId = GetWindowThreadProcessId(hWnd, &processId);

    if (UlongToHandle(processId) == NtCurrentProcessId())
    {
        // We are trying to get information about the server. Call the procedure directly.
        WepWriteClientData(hWnd);
        return TRUE;
    }

    // Call the client and wait for the client to finish.

    WeCurrentMessageId++;
    NtResetEvent(WeServerSharedSectionEvent, NULL);

    if (!SendNotifyMessage(hWnd, WeServerMessage, (WPARAM)NtCurrentProcessId(), WeCurrentMessageId))
        return FALSE;

    timeout.QuadPart = -WE_CLIENT_MESSAGE_TIMEOUT * PH_TIMEOUT_MS;

    if (NtWaitForSingleObject(WeServerSharedSectionEvent, FALSE, &timeout) != STATUS_WAIT_0)
        return FALSE;

    if (WeServerSharedData->MessageId != WeCurrentMessageId)
        return FALSE;

    return TRUE;
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

VOID WepWriteClientData(
    __in HWND hwnd
    )
{
    WCHAR className[256];

    memset(&WeServerSharedData->c, 0, sizeof(WeServerSharedData->c));

    if (IsWindowUnicode(hwnd))
        WeServerSharedData->c.WndProc = GetWindowLongPtrW(hwnd, GWLP_WNDPROC);
    else
        WeServerSharedData->c.WndProc = GetWindowLongPtrA(hwnd, GWLP_WNDPROC);

    WeServerSharedData->c.ClassInfo.cbSize = sizeof(WNDCLASSEX);
                
    if (!GetClassName(hwnd, className, sizeof(className) / sizeof(WCHAR)))
        className[0] = 0;

    GetClassInfoEx(NULL, className, &WeServerSharedData->c.ClassInfo);
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
        HANDLE serverProcessId;
        ULONG messageId;

        serverProcessId = (HANDLE)info->wParam;
        messageId = (ULONG)info->lParam;

        if (serverProcessId != NtCurrentProcessId())
        {
            if (WepOpenServerObjects())
            {
                LARGE_INTEGER timeout;

                timeout.QuadPart = -WE_CLIENT_MESSAGE_TIMEOUT * PH_TIMEOUT_MS;

                if (NtWaitForSingleObject(WeServerSharedSectionLock, FALSE, &timeout) == WAIT_OBJECT_0)
                {
                    WeServerSharedData->MessageId = messageId;
                    WepWriteClientData(info->hwnd);

                    NtReleaseMutant(WeServerSharedSectionLock, NULL);
                    NtSetEvent(WeServerSharedSectionEvent, NULL);
                }

                WepCloseServerObjects();
            }
        }
    }

    return result;
}
