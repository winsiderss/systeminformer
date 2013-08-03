/*
 * Process Hacker Network Tools -
 *   Whois dialog
 *
 * Copyright (C) 2010-2013 wj32
 * Copyright (C) 2012-2013 dmex
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

#pragma comment(lib, "Winhttp.lib")

#include "nettools.h"

#include <mxml.h>
#include <winhttp.h>

static BOOLEAN ReadRequestString(
    __in HINTERNET Handle,
    __out_z PSTR* Data,
    __out ULONG* DataLength
    )
{
    BYTE buffer[PAGE_SIZE];
    PSTR data;
    ULONG allocatedLength;
    ULONG dataLength;
    ULONG returnLength;

    allocatedLength = sizeof(buffer);
    data = (PSTR)PhAllocate(allocatedLength);
    dataLength = 0;

    // Zero the buffer
    memset(buffer, 0, PAGE_SIZE);

    while (WinHttpReadData(Handle, buffer, PAGE_SIZE, &returnLength))
    {
        if (returnLength == 0)
            break;

        if (allocatedLength < dataLength + returnLength)
        {
            allocatedLength *= 2;
            data = (PSTR)PhReAllocate(data, allocatedLength);
        }

        // Copy the returned buffer into our pointer
        memcpy(data + dataLength, buffer, returnLength);
        // Zero the returned buffer for the next loop
        //memset(buffer, 0, returnLength);

        dataLength += returnLength;
    }

    if (allocatedLength < dataLength + 1)
    {
        allocatedLength++;
        data = (PSTR)PhReAllocate(data, allocatedLength);
    }

    // Ensure that the buffer is null-terminated.
    data[dataLength] = 0;

    *DataLength = dataLength;
    *Data = data;

    return TRUE;
}

NTSTATUS NetworkWhoisThreadStart(
    __in PVOID Parameter
    )
{
    BOOLEAN isSuccess = FALSE;
    mxml_node_t* xmlNode = NULL;
    ULONG xmlBufferLength = 0;
    PSTR xmlStringBuffer = NULL;
    HINTERNET connectionHandle = NULL;
    HINTERNET requestHandle = NULL;
    HINTERNET sessionHandle = NULL;

    PNETWORK_OUTPUT_CONTEXT context = (PNETWORK_OUTPUT_CONTEXT)Parameter;
        
    Static_SetText(context->WindowHandle,   
        PhFormatString(L"Whois %s...", context->addressString)->Buffer);

    __try
    {
        // Reuse existing session?
        //if (!Context->HttpSessionHandle)
        PPH_STRING phVersion = NULL;
        PPH_STRING userAgent = NULL;
        WINHTTP_CURRENT_USER_IE_PROXY_CONFIG proxyConfig = { 0 };

        // Create a user agent string.
        phVersion = PhGetPhVersion();
        userAgent = PhConcatStrings2(L"PH_", phVersion->Buffer);

        // Query the current system proxy
        WinHttpGetIEProxyConfigForCurrentUser(&proxyConfig);

        // Open the HTTP session with the system proxy configuration if available
        sessionHandle = WinHttpOpen(
            userAgent->Buffer,
            proxyConfig.lpszProxy != NULL ? WINHTTP_ACCESS_TYPE_NAMED_PROXY : WINHTTP_ACCESS_TYPE_DEFAULT_PROXY,
            proxyConfig.lpszProxy,
            proxyConfig.lpszProxyBypass,
            0
            );

        PhSwapReference(&phVersion, NULL);
        PhSwapReference(&userAgent, NULL);

        if (!sessionHandle)
            __leave;

        if (!(connectionHandle = WinHttpConnect(
            sessionHandle,
            L"whois.arin.net",
            INTERNET_DEFAULT_HTTP_PORT,
            0
            )))
        {
            __leave;
        }

        if (!(requestHandle = WinHttpOpenRequest(
            connectionHandle,
            L"GET",
            L"/rest/ip/203.161.102.1",
            NULL,
            WINHTTP_NO_REFERER,
            WINHTTP_DEFAULT_ACCEPT_TYPES,
            0 // WINHTTP_FLAG_REFRESH
            )))
        {
            __leave;
        }

        if (!WinHttpSendRequest(
            requestHandle,
            WINHTTP_NO_ADDITIONAL_HEADERS, 0,
            WINHTTP_NO_REQUEST_DATA, 0,
            WINHTTP_IGNORE_REQUEST_TOTAL_LENGTH, 0
            ))
        {
            __leave;
        }

        if (!WinHttpReceiveResponse(requestHandle, NULL))
            __leave;
        if (!ReadRequestString(requestHandle, &xmlStringBuffer, &xmlBufferLength))
            __leave;

        {
            PPH_STRING buffer = PhFormatString(
                L"%hs\r\n\r\n", 
                xmlStringBuffer
                );
            SendMessage(context->WindowHandle, NTM_RECEIVEDPING, (WPARAM)buffer, 0);
            isSuccess = TRUE;
        }
    }
    __finally
    {
        if (requestHandle)
            WinHttpCloseHandle(requestHandle);

        if (connectionHandle)
            WinHttpCloseHandle(connectionHandle);

        if (xmlNode)
            mxmlDelete(xmlNode);

        if (xmlStringBuffer)
            PhFree(xmlStringBuffer);
    }

    return STATUS_SUCCESS;
}