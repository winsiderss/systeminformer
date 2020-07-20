/*
 * Process Hacker Network Tools -
 *   Tracert dialog
 *
 * Copyright (C) 2015-2020 dmex
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

#include "nettools.h"
#include "tracert.h"

PPH_STRING TracertGetErrorMessage(
    _In_ IP_STATUS Result
    )
{
    PPH_STRING message;
    ULONG messageLength;

    messageLength = 128;
    message = PhCreateStringEx(NULL, 128 * sizeof(WCHAR));

    if (GetIpErrorString(Result, message->Buffer, &messageLength) == ERROR_INSUFFICIENT_BUFFER)
    {
        PhDereferenceObject(message);
        message = PhCreateStringEx(NULL, messageLength * sizeof(WCHAR));

        if (GetIpErrorString(Result, message->Buffer, &messageLength) != ERROR_SUCCESS)
        {
            PhDereferenceObject(message);
            message = PhGetWin32Message(Result);
        }
    }
    else
    {
        message = PhGetWin32Message(Result);
    }

    return message;
}

PPH_STRING PhpGetDnsReverseNameFromAddress(
    _In_ PTRACERT_RESOLVE_WORKITEM Address
    )
{
    switch (Address->Type)
    {
    case PH_IPV4_NETWORK_TYPE:
        {
            IN_ADDR inAddr4 = ((PSOCKADDR_IN)&Address->SocketAddress)->sin_addr;
            PH_STRING_BUILDER stringBuilder;

            PhInitializeStringBuilder(&stringBuilder, DNS_MAX_IP4_REVERSE_NAME_LENGTH);

            PhAppendFormatStringBuilder(
                &stringBuilder,
                L"%hhu.%hhu.%hhu.%hhu.",
                inAddr4.s_impno,
                inAddr4.s_lh,
                inAddr4.s_host,
                inAddr4.s_net
                );

            PhAppendStringBuilder2(&stringBuilder, DNS_IP4_REVERSE_DOMAIN_STRING);

            return PhFinalStringBuilderString(&stringBuilder);
        }
    case PH_IPV6_NETWORK_TYPE:
        {
            IN6_ADDR inAddr6 = ((PSOCKADDR_IN6)&Address->SocketAddress)->sin6_addr;
            PH_STRING_BUILDER stringBuilder;

            PhInitializeStringBuilder(&stringBuilder, DNS_MAX_IP6_REVERSE_NAME_LENGTH);

            for (INT i = sizeof(IN6_ADDR) - 1; i >= 0; i--)
            {
                PhAppendFormatStringBuilder(
                    &stringBuilder,
                    L"%hhx.%hhx.",
                    inAddr6.s6_addr[i] & 0xF,
                    (inAddr6.s6_addr[i] >> 4) & 0xF
                    );
            }

            PhAppendStringBuilder2(&stringBuilder, DNS_IP6_REVERSE_DOMAIN_STRING);

            return PhFinalStringBuilderString(&stringBuilder);
        }
    default:
        return NULL;
    }
}

NTSTATUS TracertHostnameLookupCallback(
    _In_ PVOID Parameter
    )
{
    PTRACERT_RESOLVE_WORKITEM workitem = Parameter;
    PPH_STRING dnsHostNameString = NULL;
    PPH_STRING dnsReverseNameString = NULL;
    PDNS_RECORD dnsRecordList = NULL;

    if (workitem->Type == PH_IPV4_NETWORK_TYPE)
    {
        IN_ADDR inAddr4 = ((PSOCKADDR_IN)&workitem->SocketAddress)->sin_addr;

        if (IN4_IS_ADDR_UNSPECIFIED(&inAddr4))
        {
            PhFree(workitem);
            return STATUS_SUCCESS;
        }
    }
    else if (workitem->Type == PH_IPV6_NETWORK_TYPE)
    {
        IN6_ADDR inAddr6 = ((PSOCKADDR_IN6)&workitem->SocketAddress)->sin6_addr;

        if (IN6_IS_ADDR_UNSPECIFIED(&inAddr6))
        {
            PhFree(workitem);
            return STATUS_SUCCESS;
        }
    }

    if (!(dnsReverseNameString = PhpGetDnsReverseNameFromAddress(workitem)))
    {
        PhFree(workitem);
        return STATUS_FAIL_CHECK;
    }

    if (dnsRecordList = PhDnsQuery(
        NULL,
        dnsReverseNameString->Buffer,
        DNS_TYPE_PTR
        ))
    {
        PH_STRING_BUILDER stringBuilder;

        PhInitializeStringBuilder(&stringBuilder, 0x80);

        for (PDNS_RECORD dnsRecord = dnsRecordList; dnsRecord; dnsRecord = dnsRecord->pNext)
        {
            if (dnsRecord->wType == DNS_TYPE_PTR)
            {
                PhAppendFormatStringBuilder(&stringBuilder, L"%s, ", dnsRecord->Data.PTR.pNameHost);
            }
        }

        if (stringBuilder.String->Length != 0)
            PhRemoveEndStringBuilder(&stringBuilder, 2);

        dnsHostNameString = PhFinalStringBuilderString(&stringBuilder);

        PhDnsFree(dnsRecordList);
    }

    if (dnsHostNameString)
    {
        SendMessage(
            workitem->WindowHandle,
            WM_TRACERT_HOSTNAME,
            workitem->Index,
            (LPARAM)dnsHostNameString
            );
    }
    else
    {
        //if (status != DNS_ERROR_RCODE_NAME_ERROR)
        //{
        //    SendMessage(
        //        workitem->WindowHandle,
        //        WM_TRACERT_HOSTNAME,
        //        workitem->Index,
        //        (LPARAM)PhGetWin32Message(status)
        //        );
        //}
    }

    PhDereferenceObject(dnsReverseNameString);

    PhFree(workitem);

    return STATUS_SUCCESS;
}

VOID TracertQueueHostLookup(
    _In_ PNETWORK_TRACERT_CONTEXT Context,
    _In_ PTRACERT_ROOT_NODE Node,
    _In_ PVOID SocketAddress
    ) 
{
    PPH_STRING remoteCountryCode;
    PPH_STRING remoteCountryName;
    ULONG addressStringLength = INET6_ADDRSTRLEN;
    WCHAR addressString[INET6_ADDRSTRLEN] = L"";

    if (Context->RemoteEndpoint.Address.Type == PH_IPV4_NETWORK_TYPE)
    {
        IN_ADDR sockAddrIn;
        PTRACERT_RESOLVE_WORKITEM resolve;

        memset(&sockAddrIn, 0, sizeof(IN_ADDR));
        memcpy(&sockAddrIn, SocketAddress, sizeof(IN_ADDR));

        if (NT_SUCCESS(RtlIpv4AddressToStringEx(&sockAddrIn, 0, addressString, &addressStringLength)))
        {
            if (PhIsNullOrEmptyString(Node->IpAddressString))
            {
                PhMoveReference(&Node->IpAddressString, PhCreateString(addressString));     
            }
            else
            {
                // Make sure we don't append the same address. (dmex)
                if (PhFindStringInString(Node->IpAddressString, 0, addressString) == -1)
                {
                    // Append multiple address routes for the same ping or 'hop', to the node. (dmex)
                    PhMoveReference(
                        &Node->IpAddressString,
                        PhConcatStrings(3, PhGetStringOrEmpty(Node->IpAddressString), L", ", addressString)
                        );
                }
            }
        }

        resolve = PhAllocateZero(sizeof(TRACERT_RESOLVE_WORKITEM));
        resolve->Type = PH_IPV4_NETWORK_TYPE;
        resolve->WindowHandle = Context->WindowHandle;
        resolve->Index = Node->TTL;

        ((PSOCKADDR_IN)&resolve->SocketAddress)->sin_family = AF_INET;
        ((PSOCKADDR_IN)&resolve->SocketAddress)->sin_addr = sockAddrIn;

        PhQueueItemWorkQueue(&Context->WorkQueue, TracertHostnameLookupCallback, resolve);

        if (LookupSockInAddr4CountryCode(
            sockAddrIn,
            &remoteCountryCode,
            &remoteCountryName
            ))
        {
            PhMoveReference(&Node->RemoteCountryCode, remoteCountryCode);
            PhMoveReference(&Node->RemoteCountryName, remoteCountryName);
        }
    }
    else if (Context->RemoteEndpoint.Address.Type == PH_IPV6_NETWORK_TYPE)
    {
        IN6_ADDR sockAddrIn6;
        PTRACERT_RESOLVE_WORKITEM resolve;

        memset(&sockAddrIn6, 0, sizeof(IN6_ADDR));
        memcpy(&sockAddrIn6, SocketAddress, sizeof(IN6_ADDR));

        if (NT_SUCCESS(RtlIpv6AddressToStringEx(&sockAddrIn6, 0, 0, addressString, &addressStringLength)))
        {
            if (PhIsNullOrEmptyString(Node->IpAddressString))
            {
                PhMoveReference(&Node->IpAddressString, PhCreateString(addressString));
            }
            else
            {
                // Make sure we don't append the same address.
                if (PhFindStringInString(Node->IpAddressString, 0, addressString) == -1)
                {
                    // Append multiple address routes for the same ping or 'hop', to the node. (dmex)
                    PhMoveReference(
                        &Node->IpAddressString,
                        PhConcatStrings(3, PhGetStringOrEmpty(Node->IpAddressString), L", ", addressString)
                        );
                }
            }
        }

        resolve = PhAllocateZero(sizeof(TRACERT_RESOLVE_WORKITEM));
        resolve->Type = PH_IPV6_NETWORK_TYPE;
        resolve->WindowHandle = Context->WindowHandle;
        resolve->Index = Node->TTL;

        ((PSOCKADDR_IN6)&resolve->SocketAddress)->sin6_family = AF_INET6;
        ((PSOCKADDR_IN6)&resolve->SocketAddress)->sin6_addr = sockAddrIn6;

        PhQueueItemWorkQueue(&Context->WorkQueue, TracertHostnameLookupCallback, resolve);

        if (LookupSockInAddr6CountryCode(
            sockAddrIn6,
            &remoteCountryCode,
            &remoteCountryName
            ))
        {
            PhMoveReference(&Node->RemoteCountryCode, remoteCountryCode);
            PhMoveReference(&Node->RemoteCountryName, remoteCountryName);
        }
    }
}

NTSTATUS NetworkTracertThreadStart(
    _In_ PVOID Parameter
    )
{
    PNETWORK_TRACERT_CONTEXT context = (PNETWORK_TRACERT_CONTEXT)Parameter;
    HANDLE icmpHandle = INVALID_HANDLE_VALUE;
    SOCKADDR_STORAGE sourceAddress = { 0 };
    SOCKADDR_STORAGE destinationAddress = { 0 };
    ULONG icmpReplyCount = 0;
    ULONG icmpReplyLength = 0;
    PVOID icmpReplyBuffer = NULL;
    PPH_BYTES icmpEchoBuffer = NULL;
    PPH_STRING icmpRandString = NULL;
    IP_OPTION_INFORMATION pingOptions =
    {
        1,
        0,
        IP_FLAG_DF,
        0
    };

    if (icmpRandString = PhCreateStringEx(NULL, PhGetIntegerSetting(SETTING_NAME_PING_SIZE) * sizeof(WCHAR) + sizeof(UNICODE_NULL)))
    {
        PhGenerateRandomAlphaString(icmpRandString->Buffer, (ULONG)icmpRandString->Length / sizeof(WCHAR));

        icmpEchoBuffer = PhConvertUtf16ToMultiByte(icmpRandString->Buffer);
        PhDereferenceObject(icmpRandString);
    }

    if (!icmpEchoBuffer)
        goto CleanupExit;

    switch (context->RemoteEndpoint.Address.Type)
    {
    case PH_IPV4_NETWORK_TYPE:
        icmpHandle = IcmpCreateFile();
        break;
    case PH_IPV6_NETWORK_TYPE:
        icmpHandle = Icmp6CreateFile();
        break;
    }

    if (icmpHandle == INVALID_HANDLE_VALUE)
        goto CleanupExit;

    if (context->RemoteEndpoint.Address.Type == PH_IPV4_NETWORK_TYPE)
    {
        ((PSOCKADDR_IN)&destinationAddress)->sin_family = AF_INET;
        ((PSOCKADDR_IN)&destinationAddress)->sin_addr = context->RemoteEndpoint.Address.InAddr;
        //((PSOCKADDR_IN)&destinationAddress)->sin_port = (USHORT)context->RemoteEndpoint.Port;
    }
    else if (context->RemoteEndpoint.Address.Type == PH_IPV6_NETWORK_TYPE)
    {
        ((PSOCKADDR_IN6)&destinationAddress)->sin6_family = AF_INET6;
        ((PSOCKADDR_IN6)&destinationAddress)->sin6_addr = context->RemoteEndpoint.Address.In6Addr;
        //((PSOCKADDR_IN6)&destinationAddress)->sin6_port = (USHORT)context->RemoteEndpoint.Port;
    }

    for (ULONG i = 0; i < context->MaximumHops; i++)
    {
        PTRACERT_ROOT_NODE node;
        IN_ADDR last4ReplyAddress = in4addr_any;
        IN6_ADDR last6ReplyAddress = in6addr_any;

        if (context->Cancel)
            break;

        node = AddTracertNode(context, pingOptions.Ttl);
        PhReferenceObject(node);

        for (ULONG ii = 0; ii < DEFAULT_MAXIMUM_PINGS; ii++)
        {
            if (context->Cancel)
                break;

            if (context->RemoteEndpoint.Address.Type == PH_IPV4_NETWORK_TYPE)
            {
                icmpReplyLength = ICMP_BUFFER_SIZE(sizeof(ICMP_ECHO_REPLY), icmpEchoBuffer);
                icmpReplyBuffer = PhAllocate(icmpReplyLength);
                memset(icmpReplyBuffer, 0, icmpReplyLength);

                icmpReplyCount = IcmpSendEcho2Ex(
                    icmpHandle,
                    0,
                    NULL,
                    NULL,
                    ((PSOCKADDR_IN)&sourceAddress)->sin_addr.s_addr,
                    ((PSOCKADDR_IN)&destinationAddress)->sin_addr.s_addr,
                    icmpEchoBuffer->Buffer,
                    (USHORT)icmpEchoBuffer->Length,
                    &pingOptions,
                    icmpReplyBuffer,
                    icmpReplyLength,
                    DEFAULT_TIMEOUT
                    );

                if (icmpReplyCount > 0)
                {
                    PICMP_ECHO_REPLY reply4 = (PICMP_ECHO_REPLY)icmpReplyBuffer;

                    memcpy_s(&last4ReplyAddress, sizeof(IN_ADDR), &reply4->Address, sizeof(IN_ADDR));

                    TracertQueueHostLookup(
                        context,
                        node,
                        &reply4->Address
                        );

                    node->PingStatus[ii] = reply4->Status;
                    node->PingList[ii] = reply4->RoundTripTime;
                    UpdateTracertNode(context, node);

                    if (reply4->Status == IP_HOP_LIMIT_EXCEEDED && reply4->RoundTripTime < MIN_INTERVAL)
                    {
                        //PhDelayExecution(MIN_INTERVAL - reply4->RoundTripTime);
                    }

                    //if (reply4->Status != IP_REQ_TIMED_OUT)
                    //{
                    //    PPH_STRING errorMessage;
                    //
                    //    if (errorMessage = TracertGetErrorMessage(reply4->Status))
                    //    {
                    //        node->IpAddressString = errorMessage;
                    //    }
                    //}
                }
                else
                {
                    node->PingStatus[ii] = GetLastError(); // IP_REQ_TIMED_OUT;
                    UpdateTracertNode(context, node);
                }

                PhFree(icmpReplyBuffer);
            }
            else
            {
                icmpReplyLength = ICMP_BUFFER_SIZE(sizeof(ICMPV6_ECHO_REPLY), icmpEchoBuffer);
                icmpReplyBuffer = PhAllocate(icmpReplyLength);
                memset(icmpReplyBuffer, 0, icmpReplyLength);

                icmpReplyCount = Icmp6SendEcho2(
                    icmpHandle,
                    0,
                    NULL,
                    NULL,
                    ((PSOCKADDR_IN6)&sourceAddress),
                    ((PSOCKADDR_IN6)&destinationAddress),
                    icmpEchoBuffer->Buffer,
                    (USHORT)icmpEchoBuffer->Length,
                    &pingOptions,
                    icmpReplyBuffer,
                    icmpReplyLength,
                    DEFAULT_TIMEOUT
                    );

                if (icmpReplyCount > 0)
                {
                    PICMPV6_ECHO_REPLY reply6 = (PICMPV6_ECHO_REPLY)icmpReplyBuffer;

                    memcpy(&last6ReplyAddress, &reply6->Address.sin6_addr, sizeof(IN6_ADDR));

                    TracertQueueHostLookup(
                        context,
                        node,
                        &reply6->Address.sin6_addr
                        );
              
                    node->PingStatus[ii] = reply6->Status;
                    node->PingList[ii] = reply6->RoundTripTime;
                    UpdateTracertNode(context, node);

                    if (reply6->Status == IP_HOP_LIMIT_EXCEEDED)
                    {
                        if (reply6->RoundTripTime < MIN_INTERVAL)
                        {
                            //PhDelayExecution(MIN_INTERVAL - reply6->RoundTripTime);
                        }
                    }
                }
                else
                {
                    node->PingStatus[ii] = GetLastError(); // IP_REQ_TIMED_OUT;
                    UpdateTracertNode(context, node);
                }

                PhFree(icmpReplyBuffer);
            }
        }

        PhDereferenceObject(node);

        if (context->RemoteEndpoint.Address.Type == PH_IPV4_NETWORK_TYPE)
        {
            if (RtlEqualMemory(&last4ReplyAddress, &((PSOCKADDR_IN)&destinationAddress)->sin_addr, sizeof(IN_ADDR)))
                break;
        }
        else if (context->RemoteEndpoint.Address.Type == PH_IPV6_NETWORK_TYPE)
        {
            if (RtlEqualMemory(&last6ReplyAddress, &((PSOCKADDR_IN6)&destinationAddress)->sin6_addr, sizeof(IN6_ADDR)))
                break;
        }

        pingOptions.Ttl++;
    }

CleanupExit:

    if (icmpHandle != INVALID_HANDLE_VALUE)
        IcmpCloseHandle(icmpHandle);

    PostMessage(context->WindowHandle, NTM_RECEIVEDFINISH, 0, 0);
    PhDereferenceObject(context);

    if (icmpEchoBuffer)
        PhDereferenceObject(icmpEchoBuffer);

    return STATUS_SUCCESS;
}

VOID TracertMenuActionCallback(
    _In_ PNETWORK_TRACERT_CONTEXT Context, 
    _In_ ULONG Id
    )
{
    switch (Id)
    {
    case MAINMENU_ACTION_PING:
        {
            PH_IP_ENDPOINT RemoteEndpoint;
            PWSTR terminator = UNICODE_NULL;
            PTRACERT_ROOT_NODE node;

            if ((node = GetSelectedTracertNode(Context)) && !PhIsNullOrEmptyString(node->IpAddressString))
            {
                if (NT_SUCCESS(RtlIpv4StringToAddress(
                    node->IpAddressString->Buffer, 
                    TRUE, 
                    &terminator, 
                    &RemoteEndpoint.Address.InAddr
                    )))
                {
                    RemoteEndpoint.Address.Type = PH_IPV4_NETWORK_TYPE;
                    ShowPingWindowFromAddress(RemoteEndpoint);
                    break;
                }

                if (NT_SUCCESS(RtlIpv6StringToAddress(
                    node->IpAddressString->Buffer, 
                    &terminator, 
                    &RemoteEndpoint.Address.In6Addr
                    )))
                {
                    RemoteEndpoint.Address.Type = PH_IPV6_NETWORK_TYPE;
                    ShowPingWindowFromAddress(RemoteEndpoint);
                    break;
                }
            }
        }
        break;
    case NETWORK_ACTION_TRACEROUTE:
        {
            PH_IP_ENDPOINT RemoteEndpoint;
            PWSTR terminator = UNICODE_NULL;
            PTRACERT_ROOT_NODE node;

            if ((node = GetSelectedTracertNode(Context)) && !PhIsNullOrEmptyString(node->IpAddressString))
            {
                if (NT_SUCCESS(RtlIpv4StringToAddress(
                    node->IpAddressString->Buffer, 
                    TRUE, 
                    &terminator, 
                    &RemoteEndpoint.Address.InAddr
                    )))
                {
                    RemoteEndpoint.Address.Type = PH_IPV4_NETWORK_TYPE;
                    ShowTracertWindowFromAddress(RemoteEndpoint);
                    break;
                }
    
                if (NT_SUCCESS(RtlIpv6StringToAddress(
                    node->IpAddressString->Buffer, 
                    &terminator, 
                    &RemoteEndpoint.Address.In6Addr
                    )))
                {
                    RemoteEndpoint.Address.Type = PH_IPV6_NETWORK_TYPE;
                    ShowTracertWindowFromAddress(RemoteEndpoint);
                    break;
                }
            }
        }
        break;
    case NETWORK_ACTION_WHOIS:
        {
            PH_IP_ENDPOINT RemoteEndpoint;
            PWSTR terminator = UNICODE_NULL;
            PTRACERT_ROOT_NODE node;

            if ((node = GetSelectedTracertNode(Context)) && !PhIsNullOrEmptyString(node->IpAddressString))
            {
                if (NT_SUCCESS(RtlIpv4StringToAddress(
                    node->IpAddressString->Buffer, 
                    TRUE, 
                    &terminator, 
                    &RemoteEndpoint.Address.InAddr
                    )))
                {
                    RemoteEndpoint.Address.Type = PH_IPV4_NETWORK_TYPE;
                    ShowWhoisWindowFromAddress(RemoteEndpoint);
                    break;
                }
    
                if (NT_SUCCESS(RtlIpv6StringToAddress(
                    node->IpAddressString->Buffer,
                    &terminator, 
                    &RemoteEndpoint.Address.In6Addr
                    )))
                {
                    RemoteEndpoint.Address.Type = PH_IPV6_NETWORK_TYPE;
                    ShowWhoisWindowFromAddress(RemoteEndpoint);
                    break;
                }
            }
        }
        break;
    case MENU_ACTION_COPY:
        {
            PPH_STRING text;

            text = PhGetTreeNewText(Context->TreeNewHandle, 0);
            PhSetClipboardString(Context->TreeNewHandle, &text->sr);
            PhDereferenceObject(text);
        }
        break;
    }
}

INT_PTR CALLBACK TracertDlgProc(
    _In_ HWND hwndDlg,
    _In_ UINT uMsg,
    _In_ WPARAM wParam,
    _In_ LPARAM lParam
    )
{
    PNETWORK_TRACERT_CONTEXT context;

    if (uMsg == WM_INITDIALOG)
    {
        context = (PNETWORK_TRACERT_CONTEXT)lParam;
        PhSetWindowContext(hwndDlg, PH_WINDOW_CONTEXT_DEFAULT, context);
    }
    else
    {
        context = PhGetWindowContext(hwndDlg, PH_WINDOW_CONTEXT_DEFAULT);

        if (uMsg == WM_DESTROY)
        {
            context->Cancel = TRUE;

            PhSaveWindowPlacementToSetting(SETTING_NAME_TRACERT_WINDOW_POSITION, SETTING_NAME_TRACERT_WINDOW_SIZE, hwndDlg);

            if (context->FontHandle)
                DeleteFont(context->FontHandle);

            DeleteTracertTree(context);

            PhDeleteWorkQueue(&context->WorkQueue);
            PhDeleteLayoutManager(&context->LayoutManager);
            PhRemoveWindowContext(hwndDlg, PH_WINDOW_CONTEXT_DEFAULT);
            PhDereferenceObject(context);

            PostQuitMessage(0);
        }
    }

    if (!context)
        return FALSE;

    switch (uMsg)
    {
    case WM_INITDIALOG:
        {
            Static_SetText(hwndDlg,
                PhaFormatString(L"Tracing %s...", context->IpAddressString)->Buffer
                );
            Static_SetText(GetDlgItem(hwndDlg, IDC_STATUS),
                PhaFormatString(L"Tracing route to %s with %lu bytes of data...", context->IpAddressString, PhGetIntegerSetting(SETTING_NAME_PING_SIZE))->Buffer
                );

            context->WindowHandle = hwndDlg;
            context->TreeNewHandle = GetDlgItem(hwndDlg, IDC_LIST_TRACERT);
            context->FontHandle = PhCreateCommonFont(-15, FW_MEDIUM, GetDlgItem(hwndDlg, IDC_STATUS));

            PhSetApplicationWindowIcon(hwndDlg);

            InitializeTracertTree(context);

            PhInitializeWorkQueue(&context->WorkQueue, 0, 40, 5000);
            PhInitializeLayoutManager(&context->LayoutManager, hwndDlg);
            PhAddLayoutItem(&context->LayoutManager, GetDlgItem(hwndDlg, IDC_STATUS), NULL, PH_ANCHOR_TOP | PH_ANCHOR_LEFT | PH_ANCHOR_RIGHT | PH_LAYOUT_FORCE_INVALIDATE);
            PhAddLayoutItem(&context->LayoutManager, context->TreeNewHandle, NULL, PH_ANCHOR_ALL);
            PhAddLayoutItem(&context->LayoutManager, GetDlgItem(hwndDlg, IDCANCEL), NULL, PH_ANCHOR_BOTTOM | PH_ANCHOR_RIGHT);
            PhAddLayoutItem(&context->LayoutManager, GetDlgItem(hwndDlg, IDC_REFRESH), NULL, PH_ANCHOR_BOTTOM | PH_ANCHOR_RIGHT);

            if (PhGetIntegerPairSetting(SETTING_NAME_TRACERT_WINDOW_POSITION).X != 0)
                PhLoadWindowPlacementFromSetting(SETTING_NAME_TRACERT_WINDOW_POSITION, SETTING_NAME_TRACERT_WINDOW_SIZE, hwndDlg);
            else
                PhCenterWindow(hwndDlg, PhMainWndHandle);

            EnableWindow(GetDlgItem(hwndDlg, IDC_REFRESH), FALSE);

            PhReferenceObject(context);
            PhCreateThread2(NetworkTracertThreadStart, (PVOID)context);

            PhInitializeWindowTheme(hwndDlg, !!PhGetIntegerSetting(L"EnableThemeSupport"));
        }
        break;
    case WM_COMMAND:
        {
            switch (GET_WM_COMMAND_ID(wParam, lParam))
            {
            case IDCANCEL:
                {
                    context->Cancel = TRUE;

                    DestroyWindow(hwndDlg);
                }
                break;
            case IDC_REFRESH:
                {
                    Static_SetText(context->WindowHandle, PhaFormatString(
                        L"Tracing %s...",
                        context->IpAddressString
                        )->Buffer);
                    Static_SetText(GetDlgItem(hwndDlg, IDC_STATUS), PhaFormatString(
                        L"Tracing route to %s with %lu bytes of data....",
                        context->IpAddressString,
                        PhGetIntegerSetting(SETTING_NAME_PING_SIZE)
                        )->Buffer);

                    EnableWindow(GetDlgItem(hwndDlg, IDC_REFRESH), FALSE);

                    ClearTracertTree(context);

                    PhReferenceObject(context);
                    PhCreateThread2(NetworkTracertThreadStart, (PVOID)context);
                }
                break;
            case TRACERT_SHOWCONTEXTMENU:
                {
                    PPH_EMENU menu;
                    PTRACERT_ROOT_NODE selectedNode;
                    PPH_EMENU_ITEM selectedItem;
                    PPH_TREENEW_CONTEXT_MENU contextMenuEvent = (PPH_TREENEW_CONTEXT_MENU)lParam;

                    if (selectedNode = GetSelectedTracertNode(context))
                    {
                        menu = PhCreateEMenu();
                        PhInsertEMenuItem(menu, PhCreateEMenuItem(0, MAINMENU_ACTION_PING, L"Ping", NULL, NULL), ULONG_MAX);
                        PhInsertEMenuItem(menu, PhCreateEMenuItem(0, NETWORK_ACTION_TRACEROUTE, L"Traceroute", NULL, NULL), ULONG_MAX);
                        PhInsertEMenuItem(menu, PhCreateEMenuItem(0, NETWORK_ACTION_WHOIS, L"Whois", NULL, NULL), ULONG_MAX);
                        PhInsertEMenuItem(menu, PhCreateEMenuSeparator(), ULONG_MAX);
                        PhInsertEMenuItem(menu, PhCreateEMenuItem(0, MENU_ACTION_COPY, L"Copy", NULL, NULL), ULONG_MAX);
                        PhInsertCopyCellEMenuItem(menu, MENU_ACTION_COPY, context->TreeNewHandle, contextMenuEvent->Column);

                        if (PhIsNullOrEmptyString(selectedNode->IpAddressString))
                        {
                            PhSetFlagsEMenuItem(menu, MAINMENU_ACTION_PING, PH_EMENU_DISABLED, PH_EMENU_DISABLED);
                            PhSetFlagsEMenuItem(menu, NETWORK_ACTION_TRACEROUTE, PH_EMENU_DISABLED, PH_EMENU_DISABLED);
                            PhSetFlagsEMenuItem(menu, NETWORK_ACTION_WHOIS, PH_EMENU_DISABLED, PH_EMENU_DISABLED);
                        }

                        selectedItem = PhShowEMenu(
                            menu,
                            hwndDlg,
                            PH_EMENU_SHOW_LEFTRIGHT,
                            PH_ALIGN_LEFT | PH_ALIGN_TOP,
                            contextMenuEvent->Location.x,
                            contextMenuEvent->Location.y
                            );

                        if (selectedItem && selectedItem->Id != ULONG_MAX)
                        {
                            if (!PhHandleCopyCellEMenuItem(selectedItem))
                            {
                                TracertMenuActionCallback(context, selectedItem->Id);
                            }
                        }

                        PhDestroyEMenu(menu);
                    }
                }
                break;
            }
        }
        break;
    case WM_SIZE:
        PhLayoutManagerLayout(&context->LayoutManager);
        break;
    case NTM_RECEIVEDFINISH:
        {
            EnableWindow(GetDlgItem(hwndDlg, IDC_REFRESH), TRUE);

            Static_SetText(context->WindowHandle, PhaFormatString(
                L"Tracing %s... complete", 
                context->IpAddressString
                )->Buffer);
            Static_SetText(GetDlgItem(hwndDlg, IDC_STATUS), PhaFormatString(
                L"Tracing route to %s with %lu bytes of data... complete.", 
                context->IpAddressString, 
                PhGetIntegerSetting(SETTING_NAME_PING_SIZE)
                )->Buffer);

            TreeNew_NodesStructured(context->TreeNewHandle);
        }
        break;
    case WM_TRACERT_HOSTNAME:
        {
            ULONG index = (ULONG)wParam;
            PPH_STRING hostName = (PPH_STRING)lParam;
            PTRACERT_ROOT_NODE traceNode;

            if (traceNode = FindTracertNode(context, index))
            {
                if (PhIsNullOrEmptyString(traceNode->HostnameString))
                {
                    PhSwapReference(&traceNode->HostnameString, hostName);

                    UpdateTracertNode(context, traceNode);
                }
                else
                {
                    // Make sure we don't append the same address. (dmex)
                    if (PhFindStringInString(traceNode->HostnameString, 0, PhGetStringOrEmpty(hostName)) == -1)
                    {
                        // Append multiple address routes for the same ping or 'hop', to the node. (dmex)
                        PhMoveReference(
                            &traceNode->HostnameString,
                            PhConcatStrings(3, PhGetStringOrEmpty(traceNode->HostnameString), L", ", PhGetStringOrEmpty(hostName))
                            );

                        UpdateTracertNode(context, traceNode);
                    }
                }
            }

            PhClearReference(&hostName);
        }
        break;
    }

    return FALSE;
}

NTSTATUS TracertDialogThreadStart(
    _In_ PVOID Parameter
    )
{
    BOOL result;
    MSG message;
    HWND windowHandle;
    PH_AUTO_POOL autoPool;
    PNETWORK_TRACERT_CONTEXT context = (PNETWORK_TRACERT_CONTEXT)Parameter;

    PhInitializeAutoPool(&autoPool);

    windowHandle = CreateDialogParam(
        (HINSTANCE)PluginInstance->DllBase,
        MAKEINTRESOURCE(IDD_TRACERT),
        NULL,
        TracertDlgProc,
        (LPARAM)Parameter
        );

    ShowWindow(windowHandle, SW_SHOW);
    SetForegroundWindow(windowHandle);

    while (result = GetMessage(&message, NULL, 0, 0))
    {
        if (result == -1)
            break;

        if (!IsDialogMessage(windowHandle, &message))
        {
            TranslateMessage(&message);
            DispatchMessage(&message);
        }

        PhDrainAutoPool(&autoPool);
    }

    PhDeleteAutoPool(&autoPool);

    return STATUS_SUCCESS;
}

VOID ShowTracertWindow(
    _In_ PPH_NETWORK_ITEM NetworkItem
    )
{
    PNETWORK_TRACERT_CONTEXT context;

    context = (PNETWORK_TRACERT_CONTEXT)PhCreateAlloc(sizeof(NETWORK_TRACERT_CONTEXT));
    memset(context, 0, sizeof(NETWORK_TRACERT_CONTEXT));

    context->MaximumHops = PhGetIntegerSetting(SETTING_NAME_TRACERT_MAX_HOPS);

    RtlCopyMemory(
        &context->RemoteEndpoint,
        &NetworkItem->RemoteEndpoint,
        sizeof(PH_IP_ENDPOINT)
        );

    if (NetworkItem->RemoteEndpoint.Address.Type == PH_IPV4_NETWORK_TYPE)
    {
        RtlIpv4AddressToString(&NetworkItem->RemoteEndpoint.Address.InAddr, context->IpAddressString);
    }
    else if (NetworkItem->RemoteEndpoint.Address.Type == PH_IPV6_NETWORK_TYPE)
    {
        RtlIpv6AddressToString(&NetworkItem->RemoteEndpoint.Address.In6Addr, context->IpAddressString);
    }

    PhCreateThread2(TracertDialogThreadStart, (PVOID)context);
}

VOID ShowTracertWindowFromAddress(
    _In_ PH_IP_ENDPOINT RemoteEndpoint
    )
{
    PNETWORK_TRACERT_CONTEXT context;

    context = (PNETWORK_TRACERT_CONTEXT)PhCreateAlloc(sizeof(NETWORK_TRACERT_CONTEXT));
    memset(context, 0, sizeof(NETWORK_TRACERT_CONTEXT));

    context->MaximumHops = PhGetIntegerSetting(SETTING_NAME_TRACERT_MAX_HOPS);

    RtlCopyMemory(
        &context->RemoteEndpoint,
        &RemoteEndpoint,
        sizeof(PH_IP_ENDPOINT)
        );

    if (RemoteEndpoint.Address.Type == PH_IPV4_NETWORK_TYPE)
    {
        RtlIpv4AddressToString(&RemoteEndpoint.Address.InAddr, context->IpAddressString);
    }
    else if (RemoteEndpoint.Address.Type == PH_IPV6_NETWORK_TYPE)
    {
        RtlIpv6AddressToString(&RemoteEndpoint.Address.In6Addr, context->IpAddressString);
    }

    PhCreateThread2(TracertDialogThreadStart, (PVOID)context);
}
