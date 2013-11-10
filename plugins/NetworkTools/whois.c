/*
 * Process Hacker Network Tools -
 *   Whois dialog
 *
 * Copyright (C) 2013 dmex
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
    _In_ HINTERNET Handle,
    _Out_ _Deref_post_z_cap_(*DataLength) PSTR *Data,
    _Out_ ULONG *DataLength
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
    _In_ PVOID Parameter
    )
{
    BOOLEAN isSuccess = FALSE;
    ULONG xmlLength = 0;
    PSTR xmlBuffer = NULL; 
    PPH_STRING phVersion = NULL;
    PPH_STRING userAgent = NULL;
    PPH_STRING whoisHttpGetString = NULL;
    HINTERNET connectionHandle = NULL;
    HINTERNET requestHandle = NULL;
    HINTERNET sessionHandle = NULL;
    mxml_node_t* xmlNode = NULL;
    WINHTTP_CURRENT_USER_IE_PROXY_CONFIG proxyConfig = { 0 };

    PNETWORK_OUTPUT_CONTEXT context = (PNETWORK_OUTPUT_CONTEXT)Parameter;
        
    Static_SetText(context->WindowHandle,   
        PhFormatString(L"Whois %s...", context->addressString)->Buffer);

    //4.4.3. IP Addresses and Networks
    // https://www.arin.net/resources/whoisrws/whois_api.html   
    //TODO: use REF string from /rest/ip/ lookup for querying the IP network: "/rest/net/NET-74-125-0-0-1?showDetails=true"
    // or use CIDR string from /rest/ip/ lookup for querying the IP network: "/rest/cidr/216.34.181.0/24?showDetails=true
    //WinHttpAddRequestHeaders(requestHandle, L"application/arin.whoisrws-v1+xml", -1L, 0);

    whoisHttpGetString = PhFormatString(L"/rest/ip/%s.txt", context->addressString);

    __try
    {
        // Create a user agent string.
        phVersion = PhGetPhVersion();
        userAgent = PhConcatStrings2(L"Process Hacker ", phVersion->Buffer);

        // Query the current system proxy
        WinHttpGetIEProxyConfigForCurrentUser(&proxyConfig);

        // Open the HTTP session with the system proxy configuration if available
        if (!(sessionHandle = WinHttpOpen(
            userAgent->Buffer,
            proxyConfig.lpszProxy != NULL ? WINHTTP_ACCESS_TYPE_NAMED_PROXY : WINHTTP_ACCESS_TYPE_DEFAULT_PROXY,
            proxyConfig.lpszProxy,
            proxyConfig.lpszProxyBypass,
            0
            )))
        {
            __leave;
        }

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
            NULL, // GET
            whoisHttpGetString->Buffer,
            NULL,
            WINHTTP_NO_REFERER,
            WINHTTP_DEFAULT_ACCEPT_TYPES,
            0 // WINHTTP_FLAG_REFRESH
            )))
        {
            __leave;
        }

        //WinHttpAddRequestHeaders(requestHandle, L"Accept: text/plain", -1L, 0);

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

        if (!ReadRequestString(requestHandle, &xmlBuffer, &xmlLength))
            __leave;

        SendMessage(context->WindowHandle, NTM_RECEIVEDWHOIS, (WPARAM)xmlLength, (LPARAM)xmlBuffer);

        isSuccess = TRUE;
    }
    __finally
    {    
        PhSwapReference2(&phVersion, NULL);
        PhSwapReference2(&userAgent, NULL);
        PhSwapReference2(&whoisHttpGetString, NULL);

        if (requestHandle)
            WinHttpCloseHandle(requestHandle);

        if (connectionHandle)
            WinHttpCloseHandle(connectionHandle);

        if (sessionHandle)
            WinHttpCloseHandle(sessionHandle);

        if (xmlNode)
            mxmlDelete(xmlNode);

        if (xmlBuffer)
            PhFree(xmlBuffer);
    }

    return STATUS_SUCCESS;
}