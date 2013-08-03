/*
 * Process Hacker Network Tools -
 *   Ping dialog
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

#pragma comment(lib, "IPHLPAPI.lib")
#pragma comment(lib, "Ws2_32.lib")

#include "nettools.h"

#include <winsock2.h>
#include <ws2tcpip.h>
#include <iphlpapi.h>
#include <icmpapi.h>

NTSTATUS NetworkPingThreadStart(
    __in PVOID Parameter
    )
{
    PNETWORK_OUTPUT_CONTEXT context = (PNETWORK_OUTPUT_CONTEXT)Parameter;
    
    ULONG i = 0;
    
    WSADATA wsaData = { 0 };
    PPH_ANSI_STRING addrString;
    SOCKADDR_IN sock_in;
   
    HANDLE icmpHandle = INVALID_HANDLE_VALUE;
    PVOID replyBuffer = NULL;
    ULONG replyLength = 0;
    ULONG pingReplyCount = 0;
    PICMP_ECHO_REPLY icmpReplyStruct = NULL;
    IP_OPTION_INFORMATION pingOptions = { 0 };

    WCHAR hostname[NI_MAXHOST];
    WCHAR servInfo[NI_MAXSERV];

    BOOLEAN isLookupEnabled = !!PhGetIntegerSetting(L"ProcessHacker.NetTools.EnableHostnameLookup");
    ULONG maxPingCount = max(PhGetIntegerSetting(L"ProcessHacker.NetTools.MaxPingCount"), 1);
   
    addrString = PhCreateAnsiStringFromUnicode(context->addressString);
    
    Static_SetText(context->WindowHandle,   
        PhFormatString(L"Pinging %s...", context->addressString)->Buffer);
   
    WSAStartup(WINSOCK_VERSION, &wsaData);
    
    if (context->NetworkItem->RemoteEndpoint.Address.Type == PH_IPV4_NETWORK_TYPE)
    {
        icmpHandle = IcmpCreateFile();

        sock_in.sin_family = AF_INET;
        sock_in.sin_addr.s_addr = inet_addr(addrString->Buffer);
    }
    else
    {
        icmpHandle = Icmp6CreateFile();

        sock_in.sin_family = AF_INET6;
    }

    sock_in.sin_port = htons(_wtoi(context->NetworkItem->RemotePortString));

    // This value indicates that the packet should not be fragmented.
    pingOptions.Flags = IP_FLAG_DF;
    // Set the TTL field if we are doing a traceroute step.
    pingOptions.Ttl = 128;
    
    //Pinging 74.125.31.125 with 32 bytes of data:
    //Reply from 74.125.31.125: bytes=32 time=207ms TTL=37
    //Reply from 74.125.31.125: bytes=32 time=202ms TTL=38
    //Reply from 74.125.31.125: bytes=32 time=206ms TTL=37
    //Reply from 74.125.31.125: bytes=32 time=204ms TTL=38
    //Ping statistics for 74.125.31.125:
    //    Packets: Sent = 4, Received = 4, Lost = 0 (0% loss),
    //Approximate round trip times in milli-seconds:
    //    Minimum = 202ms, Maximum = 207ms, Average = 204ms

    if (isLookupEnabled && GetNameInfo(
        (const PSOCKADDR)&sock_in, sizeof(SOCKADDR_IN),               
        hostname, _countof(hostname), 
        servInfo, _countof(servInfo),
        0    
        ) == 0) 
    {
        PPH_STRING buffer = PhFormatString(
            L"Pinging %s:%s [%s] with 32 bytes of data...\r\n\r\n", 
            context->addressString, 
            servInfo, 
            hostname
            );
        SendMessage(context->WindowHandle, NTM_RECEIVEDPING, (WPARAM)buffer, 0);
    }
    else  
    {
        PPH_STRING buffer = PhFormatString(
            L"Pinging %s with 32 bytes of data...\r\n\r\n", 
            context->addressString
            );
        SendMessage(context->WindowHandle, NTM_RECEIVEDPING, (WPARAM)buffer, 0);
    }

    // Loop through the 3 sets of pings
    for (i = 0; i < maxPingCount; i++)
    {  
        replyLength = sizeof(ICMP_ECHO_REPLY);
        replyBuffer = PhAllocate(sizeof(ICMP_ECHO_REPLY));

        memset(replyBuffer, '0xf2', replyLength);

        // First ping with no data
        if ((pingReplyCount = IcmpSendEcho(
            icmpHandle,
            context->NetworkItem->RemoteEndpoint.Address.InAddr.S_un.S_addr,
            NULL,
            0,
            &pingOptions,
            replyBuffer,
            replyLength,
            1000
            )) == 0)
        {
            PPH_STRING buffer = NULL;

            if (WSAGetLastError() == IP_REQ_TIMED_OUT)
            {
                buffer = PhFormatString(L"Reply from %s: Timed out...\r\n" ,
                    context->addressString
                    );
                SendMessage(context->WindowHandle, NTM_RECEIVEDPING, (WPARAM)buffer, 0);
            }
            else
            {         
                buffer = PhFormatString(L"IcmpSendEcho failed: %d\r\n", WSAGetLastError());
                SendMessage(context->WindowHandle, NTM_RECEIVEDPING, (WPARAM)buffer, 0);
            }

            if (replyBuffer)
            {
                PhFree(replyBuffer);
                replyBuffer = NULL;
            }

            continue;
        }

        icmpReplyStruct = (PICMP_ECHO_REPLY)replyBuffer;

        if (icmpReplyStruct)
        {
            // Reply from differnt address??
            // if (icmpReplyStruct->Address == context->Address.InAddr.S_un.S_addr) 
            // if (icmpReplyStruct->DataSize < max_size
            //if (!icmpReplyStruct->RoundTripTime) PPH_STRING buffer = PhFormatString(L"<1 ms  ");  

            switch (icmpReplyStruct->Status)
            {
            case IP_SUCCESS:
                {
                    PPH_STRING buffer = PhFormatString(
                        L"Reply from %s: Time=%dms TTL=%u\r\n",
                        context->addressString,
                        icmpReplyStruct->RoundTripTime,
                        icmpReplyStruct->Options.Ttl
                        );
                    SendMessage(context->WindowHandle, NTM_RECEIVEDPING, (WPARAM)buffer, 0); 
                }
                break;
            case IP_REQ_TIMED_OUT:
                {
                    PPH_STRING buffer = PhFormatString(L"Reply from %s [%s]: Timed out...\r\n",
                        context->addressString,
                        hostname
                        );
                    SendMessage(context->WindowHandle, NTM_RECEIVEDPING, (WPARAM)buffer, 0);
                }
                break;
            default:
                {
                    PPH_STRING buffer = PhFormatString(
                        L"icmpReplyStruct failed: %d\r\n", 
                        icmpReplyStruct->Status
                        );
                    SendMessage(context->WindowHandle, NTM_RECEIVEDPING, (WPARAM)buffer, 0);
                }
                break;
            } 
        }
    }

    if (replyBuffer)
    {
        PhFree(replyBuffer);
        replyBuffer = NULL;
    }

    IcmpCloseHandle(icmpHandle);
    WSACleanup();

    {
        PPH_STRING buffer = PhFormatString(L"\r\n");
        SendMessage(context->WindowHandle, NTM_RECEIVEDPING, (WPARAM)buffer, 0);
        Button_Enable(GetDlgItem(context->WindowHandle, IDC_NETRETRY), TRUE);
    }

    return STATUS_SUCCESS;
}