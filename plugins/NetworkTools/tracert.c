/*
 * Copyright (c) 2022 Winsider Seminars & Solutions, Inc.  All rights reserved.
 *
 * This file is part of System Informer.
 *
 * Authors:
 *
 *     dmex    2015-2023
 *
 */

#include "nettools.h"
#include "tracert.h"

PPH_OBJECT_TYPE TracertContextType = NULL;
PH_INITONCE TracertContextTypeInitOnce = PH_INITONCE_INIT;

_Function_class_(PH_TYPE_DELETE_PROCEDURE)
VOID TracertContextDeleteProcedure(
    _In_ PVOID Object,
    _In_ ULONG Flags
    )
{
    PNETWORK_TRACERT_CONTEXT context = Object;

    PhDeleteWorkQueue(&context->WorkQueue);
    DeleteTracertTree(context);
}

PNETWORK_TRACERT_CONTEXT CreateTracertContext(
    VOID
    )
{
    PNETWORK_TRACERT_CONTEXT context;

    if (PhBeginInitOnce(&TracertContextTypeInitOnce))
    {
        TracertContextType = PhCreateObjectType(L"PingContextObjectType", 0, TracertContextDeleteProcedure);
        PhEndInitOnce(&TracertContextTypeInitOnce);
    }

    context = PhCreateObject(sizeof(NETWORK_TRACERT_CONTEXT), TracertContextType);
    memset(context, 0, sizeof(NETWORK_TRACERT_CONTEXT));

    return context;
}

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

_Function_class_(USER_THREAD_START_ROUTINE)
NTSTATUS TracertHostnameLookupCallback(
    _In_ PVOID Parameter
    )
{
    PTRACERT_RESOLVE_WORKITEM workitem = Parameter;
    BOOLEAN dnsLocalQuery = FALSE;
    PPH_STRING dnsHostNameString = NULL;
    PPH_STRING dnsReverseNameString = NULL;
    PDNS_RECORD dnsRecordList;

    if (workitem->Type == PH_NETWORK_TYPE_IPV4)
    {
        IN_ADDR inAddr4 = ((PSOCKADDR_IN)&workitem->SocketAddress)->sin_addr;

        if (IN4_IS_ADDR_LOOPBACK(&inAddr4) ||
            IN4_IS_ADDR_BROADCAST(&inAddr4) ||
            IN4_IS_ADDR_MULTICAST(&inAddr4) ||
            IN4_IS_ADDR_LINKLOCAL(&inAddr4) ||
            IN4_IS_ADDR_MC_LINKLOCAL(&inAddr4) ||
            IN4_IS_ADDR_RFC1918(&inAddr4))
        {
            dnsLocalQuery = TRUE;
        }

        dnsReverseNameString = PhDnsReverseLookupNameFromAddress(PH_NETWORK_TYPE_IPV4, &inAddr4);
    }
    else if (workitem->Type == PH_NETWORK_TYPE_IPV6)
    {
        IN6_ADDR inAddr6 = ((PSOCKADDR_IN6)&workitem->SocketAddress)->sin6_addr;

        if (IN6_IS_ADDR_LOOPBACK(&inAddr6) ||
            IN6_IS_ADDR_MULTICAST(&inAddr6) ||
            IN6_IS_ADDR_LINKLOCAL(&inAddr6) ||
            IN6_IS_ADDR_MC_LINKLOCAL(&inAddr6))
        {
            dnsLocalQuery = TRUE;
        }

        dnsReverseNameString = PhDnsReverseLookupNameFromAddress(PH_NETWORK_TYPE_IPV6, &inAddr6);
    }

    if (PhIsNullOrEmptyString(dnsReverseNameString))
    {
        PhFree(workitem);
        return STATUS_FAIL_CHECK;
    }

    if (PhGetIntegerSetting(L"EnableNetworkResolveDoH") && !dnsLocalQuery)
    {
        dnsRecordList = PhDnsQuery(
            NULL,
            dnsReverseNameString->Buffer,
            DNS_TYPE_PTR
            );
    }
    else
    {
        dnsRecordList = PhDnsQuery2(
            NULL,
            dnsReverseNameString->Buffer,
            DNS_TYPE_PTR,
            DNS_QUERY_NO_HOSTS_FILE // DNS_QUERY_BYPASS_CACHE
            );
    }

    if (dnsRecordList)
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
    ULONG remoteCountryCode;
    PPH_STRING remoteCountryName;
    ULONG addressStringLength = INET6_ADDRSTRLEN;
    WCHAR addressString[INET6_ADDRSTRLEN] = L"";

    if (Context->RemoteEndpoint.Address.Type == PH_NETWORK_TYPE_IPV4)
    {
        IN_ADDR sockAddrIn;

        memset(&sockAddrIn, 0, sizeof(IN_ADDR));
        memcpy(&sockAddrIn, SocketAddress, sizeof(IN_ADDR));

        if (NT_SUCCESS(PhIpv4AddressToString(&sockAddrIn, 0, addressString, &addressStringLength)))
        {
            if (PhIsNullOrEmptyString(Node->IpAddressString))
            {
                PhMoveReference(&Node->IpAddressString, PhCreateString(addressString));
            }
            else
            {
                // Make sure we don't append the same address. (dmex)
                if (PhFindStringInString(Node->IpAddressString, 0, addressString) == SIZE_MAX)
                {
                    // Append multiple address routes for the same ping or 'hop', to the node. (dmex)
                    PhMoveReference(
                        &Node->IpAddressString,
                        PhConcatStrings(3, PhGetStringOrEmpty(Node->IpAddressString), L", ", addressString)
                        );
                }
            }
        }

        if (!IN4_IS_ADDR_UNSPECIFIED(&sockAddrIn))
        {
            PTRACERT_RESOLVE_WORKITEM resolve;

            resolve = PhAllocateZero(sizeof(TRACERT_RESOLVE_WORKITEM));
            resolve->Type = PH_NETWORK_TYPE_IPV4;
            resolve->WindowHandle = Context->WindowHandle;
            resolve->Index = Node->TTL;

            ((PSOCKADDR_IN)&resolve->SocketAddress)->sin_family = AF_INET;
            ((PSOCKADDR_IN)&resolve->SocketAddress)->sin_addr = sockAddrIn;

            PhQueueItemWorkQueue(&Context->WorkQueue, TracertHostnameLookupCallback, resolve);
        }

        if (LookupSockInAddr4CountryCode(
            sockAddrIn,
            &remoteCountryCode,
            &remoteCountryName
            ))
        {
            Node->RemoteCountryCode = remoteCountryCode;
            PhMoveReference(&Node->RemoteCountryName, remoteCountryName);
        }
    }
    else if (Context->RemoteEndpoint.Address.Type == PH_NETWORK_TYPE_IPV6)
    {
        IN6_ADDR sockAddrIn6;

        memset(&sockAddrIn6, 0, sizeof(IN6_ADDR));
        memcpy(&sockAddrIn6, SocketAddress, sizeof(IN6_ADDR));

        if (NT_SUCCESS(PhIpv6AddressToString(&sockAddrIn6, 0, 0, addressString, &addressStringLength)))
        {
            if (PhIsNullOrEmptyString(Node->IpAddressString))
            {
                PhMoveReference(&Node->IpAddressString, PhCreateString(addressString));
            }
            else
            {
                // Make sure we don't append the same address.
                if (PhFindStringInString(Node->IpAddressString, 0, addressString) == SIZE_MAX)
                {
                    // Append multiple address routes for the same ping or 'hop', to the node. (dmex)
                    PhMoveReference(
                        &Node->IpAddressString,
                        PhConcatStrings(3, PhGetStringOrEmpty(Node->IpAddressString), L", ", addressString)
                        );
                }
            }
        }

        if (!IN6_IS_ADDR_UNSPECIFIED(&sockAddrIn6))
        {
            PTRACERT_RESOLVE_WORKITEM resolve;

            resolve = PhAllocateZero(sizeof(TRACERT_RESOLVE_WORKITEM));
            resolve->Type = PH_NETWORK_TYPE_IPV6;
            resolve->WindowHandle = Context->WindowHandle;
            resolve->Index = Node->TTL;

            ((PSOCKADDR_IN6)&resolve->SocketAddress)->sin6_family = AF_INET6;
            ((PSOCKADDR_IN6)&resolve->SocketAddress)->sin6_addr = sockAddrIn6;

            PhQueueItemWorkQueue(&Context->WorkQueue, TracertHostnameLookupCallback, resolve);
        }

        if (LookupSockInAddr6CountryCode(
            sockAddrIn6,
            &remoteCountryCode,
            &remoteCountryName
            ))
        {
            Node->RemoteCountryCode = remoteCountryCode;
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
    BOOLEAN icmpReplyStatusFatal = FALSE;
    FLOAT icmpCurrentPingMs = 0.0f;
    ULONG icmpCurrentOverhead = 0;
    ULONG icmpReplyCount = 0;
    ULONG icmpReplyLength = 0;
    PVOID icmpReplyBuffer = NULL;
    ULONG icmpEchoBufferLength = 0;
    PPH_BYTES icmpEchoBuffer = NULL;
    PPH_STRING icmpRandString = NULL;
    LARGE_INTEGER performanceCounterStart;
    LARGE_INTEGER performanceCounterEnd;
    LARGE_INTEGER performanceCounterFrequency;
    FLOAT performanceCounterTime = 0.0f;
    IP_OPTION_INFORMATION pingOptions =
    {
        1,
        0,
        IP_FLAG_DF,
        0
    };

    PhQueryPerformanceFrequency(&performanceCounterFrequency);
    icmpEchoBufferLength = PhGetIntegerSetting(SETTING_NAME_PING_SIZE);

    if (icmpRandString = PhCreateStringEx(NULL, icmpEchoBufferLength * sizeof(WCHAR) + sizeof(UNICODE_NULL)))
    {
        PhGenerateRandomAlphaString(icmpRandString->Buffer, icmpRandString->Length / sizeof(WCHAR));
        icmpEchoBuffer = PhConvertUtf16ToUtf8Ex(icmpRandString->Buffer, icmpRandString->Length - sizeof(UNICODE_NULL));
        PhDereferenceObject(icmpRandString);
    }

    if (!icmpEchoBuffer)
        goto CleanupExit;
    if (icmpEchoBuffer->Length != icmpEchoBufferLength)
        goto CleanupExit;

    switch (context->RemoteEndpoint.Address.Type)
    {
    case PH_NETWORK_TYPE_IPV4:
        icmpHandle = IcmpCreateFile();
        break;
    case PH_NETWORK_TYPE_IPV6:
        icmpHandle = Icmp6CreateFile();
        break;
    }

    if (icmpHandle == INVALID_HANDLE_VALUE)
        goto CleanupExit;

    if (context->RemoteEndpoint.Address.Type == PH_NETWORK_TYPE_IPV4)
    {
        ((PSOCKADDR_IN)&destinationAddress)->sin_family = AF_INET;
        ((PSOCKADDR_IN)&destinationAddress)->sin_addr = context->RemoteEndpoint.Address.InAddr;
        //((PSOCKADDR_IN)&destinationAddress)->sin_port = (USHORT)context->RemoteEndpoint.Port;
    }
    else if (context->RemoteEndpoint.Address.Type == PH_NETWORK_TYPE_IPV6)
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

            if (context->RemoteEndpoint.Address.Type == PH_NETWORK_TYPE_IPV4)
            {
                icmpReplyLength = ICMP_BUFFER_SIZE(sizeof(ICMP_ECHO_REPLY), icmpEchoBuffer->Length);
                icmpReplyBuffer = PhAllocateZero(icmpReplyLength);

                PhQueryPerformanceCounter(&performanceCounterStart);

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
                    context->Timeout
                    );

                PhQueryPerformanceCounter(&performanceCounterEnd);

                if (icmpReplyCount > 0)
                {
                    PICMP_ECHO_REPLY reply4 = (PICMP_ECHO_REPLY)icmpReplyBuffer;

                    performanceCounterTime = (FLOAT)(performanceCounterEnd.QuadPart - performanceCounterStart.QuadPart);
                    performanceCounterTime *= 1000000;
                    performanceCounterTime /= performanceCounterFrequency.QuadPart;
                    performanceCounterTime /= 1000;
                    icmpCurrentOverhead = (ULONG)performanceCounterTime - reply4->RoundTripTime;
                    icmpCurrentPingMs = performanceCounterTime - (FLOAT)icmpCurrentOverhead;

                    memcpy_s(&last4ReplyAddress, sizeof(IN_ADDR), &reply4->Address, sizeof(IN_ADDR));

                    TracertQueueHostLookup(
                        context,
                        node,
                        &reply4->Address
                        );

                    node->PingStatus[ii] = reply4->Status;
                    node->PingList[ii] = icmpCurrentPingMs;
                    UpdateTracertNode(context, node);

                    if (reply4->Status == IP_SUCCESS)
                    {
                        icmpReplyStatusFatal = FALSE; // reset failure
                    }

                    if (reply4->Status == IP_DEST_NO_ROUTE)
                    {
                        icmpReplyStatusFatal = TRUE;
                        //break; // add break for instant failure
                    }

                    if (reply4->Status == IP_HOP_LIMIT_EXCEEDED && reply4->RoundTripTime < DEFAULT_MINIMUM_INTERVAL)
                    {
                        //PhDelayExecution(DEFAULT_MINIMUM_INTERVAL - reply4->RoundTripTime);
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
                icmpReplyLength = ICMP_BUFFER_SIZE(sizeof(ICMPV6_ECHO_REPLY), icmpEchoBuffer->Length);
                icmpReplyBuffer = PhAllocateZero(icmpReplyLength);

                PhQueryPerformanceCounter(&performanceCounterStart);

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
                    context->Timeout
                    );

                PhQueryPerformanceCounter(&performanceCounterEnd);

                if (icmpReplyCount > 0)
                {
                    PICMPV6_ECHO_REPLY reply6 = (PICMPV6_ECHO_REPLY)icmpReplyBuffer;

                    performanceCounterTime = (FLOAT)(performanceCounterEnd.QuadPart - performanceCounterStart.QuadPart);
                    performanceCounterTime *= 1000000;
                    performanceCounterTime /= performanceCounterFrequency.QuadPart;
                    performanceCounterTime /= 1000;
                    icmpCurrentOverhead = (ULONG)performanceCounterTime - reply6->RoundTripTime;
                    icmpCurrentPingMs = performanceCounterTime - (FLOAT)icmpCurrentOverhead;

                    memcpy(&last6ReplyAddress, &reply6->Address.sin6_addr, sizeof(IN6_ADDR));

                    TracertQueueHostLookup(
                        context,
                        node,
                        &reply6->Address.sin6_addr
                        );

                    node->PingStatus[ii] = reply6->Status;
                    node->PingList[ii] = icmpCurrentPingMs;
                    UpdateTracertNode(context, node);

                    if (reply6->Status == IP_SUCCESS)
                    {
                        icmpReplyStatusFatal = FALSE; // reset failure
                    }

                    if (reply6->Status == IP_DEST_NO_ROUTE)
                    {
                        icmpReplyStatusFatal = TRUE;
                        //break; // add break for instant fail
                    }

                    if (reply6->Status == IP_HOP_LIMIT_EXCEEDED)
                    {
                        if (reply6->RoundTripTime < DEFAULT_MINIMUM_INTERVAL)
                        {
                            //PhDelayExecution(DEFAULT_MINIMUM_INTERVAL - reply6->RoundTripTime);
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

        if (icmpReplyStatusFatal)
        {
            // If the route becomes unavailable the error response is IP_DEST_NO_ROUTE, since this node cannot
            // forward packets the responses will all be from this same address. We need to end the trace
            // and wait for the route to become available (tracert does the same in this case). (dmex)
            break;
        }

        if (context->RemoteEndpoint.Address.Type == PH_NETWORK_TYPE_IPV4)
        {
            if (RtlEqualMemory(&last4ReplyAddress, &((PSOCKADDR_IN)&destinationAddress)->sin_addr, sizeof(IN_ADDR)))
                break;
        }
        else if (context->RemoteEndpoint.Address.Type == PH_NETWORK_TYPE_IPV6)
        {
            if (RtlEqualMemory(&last6ReplyAddress, &((PSOCKADDR_IN6)&destinationAddress)->sin6_addr, sizeof(IN6_ADDR)))
                break;
        }

        pingOptions.Ttl++;
    }

CleanupExit:

    if (icmpHandle && icmpHandle != INVALID_HANDLE_VALUE)
        IcmpCloseHandle(icmpHandle);

    PostMessage(context->WindowHandle, NTM_RECEIVEDFINISH, 0, (LPARAM)icmpReplyStatusFatal);
    PhDereferenceObject(context);
    PhClearReference(&icmpEchoBuffer);

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
            PH_IP_ENDPOINT remoteEndpoint = { 0 };
            USHORT remoteEndpointPort = 0;
            ULONG remoteEndpointScope = 0;
            PTRACERT_ROOT_NODE node;

            if ((node = GetSelectedTracertNode(Context)) && !PhIsNullOrEmptyString(node->IpAddressString))
            {
                if (NT_SUCCESS(PhIpv4StringToAddress(
                    PhGetString(node->IpAddressString),
                    TRUE,
                    &remoteEndpoint.Address.InAddr,
                    &remoteEndpointPort
                    )))
                {
                    remoteEndpoint.Address.Type = PH_NETWORK_TYPE_IPV4;
                    ShowPingWindowFromAddress(Context->WindowHandle, remoteEndpoint);
                    break;
                }

                if (NT_SUCCESS(PhIpv6StringToAddress(
                    PhGetString(node->IpAddressString),
                    &remoteEndpoint.Address.In6Addr,
                    &remoteEndpointScope,
                    &remoteEndpointPort
                    )))
                {
                    remoteEndpoint.Address.Type = PH_NETWORK_TYPE_IPV6;
                    ShowPingWindowFromAddress(Context->WindowHandle, remoteEndpoint);
                    break;
                }
            }
        }
        break;
    case NETWORK_ACTION_TRACEROUTE:
        {
            PH_IP_ENDPOINT remoteEndpoint = { 0 };
            USHORT remoteEndpointPort = 0;
            ULONG remoteEndpointScope = 0;
            PTRACERT_ROOT_NODE node;

            if ((node = GetSelectedTracertNode(Context)) && !PhIsNullOrEmptyString(node->IpAddressString))
            {
                if (NT_SUCCESS(PhIpv4StringToAddress(
                    PhGetString(node->IpAddressString),
                    TRUE,
                    &remoteEndpoint.Address.InAddr,
                    &remoteEndpointPort
                    )))
                {
                    remoteEndpoint.Address.Type = PH_NETWORK_TYPE_IPV4;
                    ShowTracertWindowFromAddress(Context->WindowHandle, remoteEndpoint);
                    break;
                }

                if (NT_SUCCESS(PhIpv6StringToAddress(
                    PhGetString(node->IpAddressString),
                    &remoteEndpoint.Address.In6Addr,
                    &remoteEndpointScope,
                    &remoteEndpointPort
                    )))
                {
                    remoteEndpoint.Address.Type = PH_NETWORK_TYPE_IPV6;
                    ShowTracertWindowFromAddress(Context->WindowHandle, remoteEndpoint);
                    break;
                }
            }
        }
        break;
    case NETWORK_ACTION_WHOIS:
        {
            PH_IP_ENDPOINT remoteEndpoint = { 0 };
            USHORT remoteEndpointPort = 0;
            ULONG remoteEndpointScope = 0;
            PTRACERT_ROOT_NODE node;

            if ((node = GetSelectedTracertNode(Context)) && !PhIsNullOrEmptyString(node->IpAddressString))
            {
                if (NT_SUCCESS(PhIpv4StringToAddress(
                    PhGetString(node->IpAddressString),
                    TRUE,
                    &remoteEndpoint.Address.InAddr,
                    &remoteEndpointPort
                    )))
                {
                    remoteEndpoint.Address.Type = PH_NETWORK_TYPE_IPV4;
                    ShowWhoisWindowFromAddress(Context->WindowHandle, remoteEndpoint);
                    break;
                }

                if (NT_SUCCESS(PhIpv6StringToAddress(
                    PhGetString(node->IpAddressString),
                    &remoteEndpoint.Address.In6Addr,
                    &remoteEndpointScope,
                    &remoteEndpointPort
                    )))
                {
                    remoteEndpoint.Address.Type = PH_NETWORK_TYPE_IPV6;
                    ShowWhoisWindowFromAddress(Context->WindowHandle, remoteEndpoint);
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

VOID TracertSetTreeNewFont(
    _In_ PNETWORK_TRACERT_CONTEXT Context
    )
{
    PPH_STRING fontHexString;
    LOGFONT font;

    fontHexString = PhaGetStringSetting(L"Font");

    if (
        fontHexString->Length / sizeof(WCHAR) / 2 == sizeof(LOGFONT) &&
        PhHexStringToBuffer(&fontHexString->sr, (PUCHAR)&font)
        )
    {
        if (Context->TreeNewFont = CreateFontIndirect(&font))
        {
            SetWindowFont(Context->TreeNewHandle, Context->TreeNewFont, TRUE);
        }
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
    }

    if (!context)
        return FALSE;

    switch (uMsg)
    {
    case WM_INITDIALOG:
        {
            LONG dpiValue;

            PhSetWindowText(hwndDlg,
                PhaFormatString(L"Tracing %s...", context->RemoteAddressString)->Buffer
                );
            PhSetWindowText(GetDlgItem(hwndDlg, IDC_STATUS),
                PhaFormatString(L"Tracing route to %s with %lu bytes of data...", context->RemoteAddressString, PhGetIntegerSetting(SETTING_NAME_PING_SIZE))->Buffer
                );

            dpiValue = PhGetWindowDpi(hwndDlg);

            context->WindowHandle = hwndDlg;
            context->TreeNewHandle = GetDlgItem(hwndDlg, IDC_LIST_TRACERT);
            context->FontHandle = PhCreateCommonFont(-15, FW_MEDIUM, GetDlgItem(hwndDlg, IDC_STATUS), dpiValue);
            context->Timeout = PhGetIntegerSetting(SETTING_NAME_PING_TIMEOUT);

            PhSetApplicationWindowIcon(hwndDlg);

            InitializeTracertTree(context);
            TracertSetTreeNewFont(context);

            PhInitializeWorkQueue(&context->WorkQueue, 0, 40, 5000);
            PhInitializeLayoutManager(&context->LayoutManager, hwndDlg);
            PhAddLayoutItem(&context->LayoutManager, GetDlgItem(hwndDlg, IDC_STATUS), NULL, PH_ANCHOR_TOP | PH_ANCHOR_LEFT | PH_ANCHOR_RIGHT | PH_LAYOUT_FORCE_INVALIDATE);
            PhAddLayoutItem(&context->LayoutManager, context->TreeNewHandle, NULL, PH_ANCHOR_ALL);
            PhAddLayoutItem(&context->LayoutManager, GetDlgItem(hwndDlg, IDCANCEL), NULL, PH_ANCHOR_BOTTOM | PH_ANCHOR_RIGHT);
            PhAddLayoutItem(&context->LayoutManager, GetDlgItem(hwndDlg, IDC_REFRESH), NULL, PH_ANCHOR_BOTTOM | PH_ANCHOR_RIGHT);

            if (PhValidWindowPlacementFromSetting(SETTING_NAME_TRACERT_WINDOW_POSITION))
                PhLoadWindowPlacementFromSetting(SETTING_NAME_TRACERT_WINDOW_POSITION, SETTING_NAME_TRACERT_WINDOW_SIZE, hwndDlg);
            else
                PhCenterWindow(hwndDlg, context->ParentWindowHandle);

            EnableWindow(GetDlgItem(hwndDlg, IDC_REFRESH), FALSE);

            PhReferenceObject(context);
            PhCreateThread2(NetworkTracertThreadStart, context);

            PhInitializeWindowTheme(hwndDlg, !!PhGetIntegerSetting(L"EnableThemeSupport"));
        }
        break;
    case WM_DESTROY:
        {
            context->Cancel = TRUE;

            PhRemoveWindowContext(hwndDlg, PH_WINDOW_CONTEXT_DEFAULT);

            PhSaveWindowPlacementToSetting(SETTING_NAME_TRACERT_WINDOW_POSITION, SETTING_NAME_TRACERT_WINDOW_SIZE, hwndDlg);

            TracertSaveSettingsTreeList(context);

            if (context->FontHandle)
                DeleteFont(context->FontHandle);
            if (context->TreeNewFont)
                DeleteFont(context->TreeNewFont);

            PhDeleteLayoutManager(&context->LayoutManager);
            PhDereferenceObject(context);

            PostQuitMessage(0);
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
                    PhSetWindowText(context->WindowHandle, PhaFormatString(
                        L"Tracing %s...",
                        context->RemoteAddressString
                        )->Buffer);
                    PhSetWindowText(GetDlgItem(hwndDlg, IDC_STATUS), PhaFormatString(
                        L"Tracing route to %s with %lu bytes of data...",
                        context->RemoteAddressString,
                        PhGetIntegerSetting(SETTING_NAME_PING_SIZE)
                        )->Buffer);

                    EnableWindow(GetDlgItem(hwndDlg, IDC_REFRESH), FALSE);

                    ClearTracertTree(context);

                    PhReferenceObject(context);
                    PhCreateThread2(NetworkTracertThreadStart, context);
                }
                break;
            case TRACERT_SHOWCONTEXTMENU:
                {
                    PPH_TREENEW_CONTEXT_MENU contextMenuEvent = (PPH_TREENEW_CONTEXT_MENU)lParam;
                    PPH_EMENU menu;
                    PTRACERT_ROOT_NODE selectedNode;
                    PPH_EMENU_ITEM selectedItem;
                    PH_IP_ENDPOINT remoteEndpoint;
                    USHORT remoteEndpointPort = 0;
                    ULONG remoteEndpointScope = 0;

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

                        if (!PhIsNullOrEmptyString(selectedNode->IpAddressString))
                        {
                            if (NT_SUCCESS(PhIpv4StringToAddress(
                                PhGetString(selectedNode->IpAddressString),
                                TRUE,
                                &remoteEndpoint.Address.InAddr,
                                &remoteEndpointPort
                                )))
                            {
                                if (
                                    IN4_IS_ADDR_UNSPECIFIED(&remoteEndpoint.Address.InAddr) ||
                                    IN4_IS_ADDR_LOOPBACK(&remoteEndpoint.Address.InAddr)
                                    )
                                {
                                    PhSetFlagsEMenuItem(menu, MAINMENU_ACTION_PING, PH_EMENU_DISABLED, PH_EMENU_DISABLED);
                                    PhSetFlagsEMenuItem(menu, NETWORK_ACTION_TRACEROUTE, PH_EMENU_DISABLED, PH_EMENU_DISABLED);
                                    PhSetFlagsEMenuItem(menu, NETWORK_ACTION_WHOIS, PH_EMENU_DISABLED, PH_EMENU_DISABLED);
                                }

                                if (IN4_IS_ADDR_BROADCAST(&remoteEndpoint.Address.InAddr) ||
                                    IN4_IS_ADDR_MULTICAST(&remoteEndpoint.Address.InAddr) ||
                                    IN4_IS_ADDR_LINKLOCAL(&remoteEndpoint.Address.InAddr) ||
                                    IN4_IS_ADDR_MC_LINKLOCAL(&remoteEndpoint.Address.InAddr) ||
                                    IN4_IS_ADDR_RFC1918(&remoteEndpoint.Address.InAddr))
                                {
                                    PhSetFlagsEMenuItem(menu, NETWORK_ACTION_WHOIS, PH_EMENU_DISABLED, PH_EMENU_DISABLED);
                                }
                            }

                            if (NT_SUCCESS(PhIpv6StringToAddress(
                                PhGetString(selectedNode->IpAddressString),
                                &remoteEndpoint.Address.In6Addr,
                                &remoteEndpointScope,
                                &remoteEndpointPort
                                )))
                            {
                                if (
                                    IN6_IS_ADDR_UNSPECIFIED(&remoteEndpoint.Address.In6Addr) ||
                                    IN6_IS_ADDR_LOOPBACK(&remoteEndpoint.Address.In6Addr)
                                    )
                                {
                                    PhSetFlagsEMenuItem(menu, MAINMENU_ACTION_PING, PH_EMENU_DISABLED, PH_EMENU_DISABLED);
                                    PhSetFlagsEMenuItem(menu, NETWORK_ACTION_TRACEROUTE, PH_EMENU_DISABLED, PH_EMENU_DISABLED);
                                    PhSetFlagsEMenuItem(menu, NETWORK_ACTION_WHOIS, PH_EMENU_DISABLED, PH_EMENU_DISABLED);
                                }

                                if (IN6_IS_ADDR_MULTICAST(&remoteEndpoint.Address.In6Addr) ||
                                    IN6_IS_ADDR_LINKLOCAL(&remoteEndpoint.Address.In6Addr) ||
                                    IN6_IS_ADDR_MC_LINKLOCAL(&remoteEndpoint.Address.In6Addr) ||
                                    IN6_IS_ADDR_MC_LINKLOCAL(&remoteEndpoint.Address.In6Addr))
                                {
                                    PhSetFlagsEMenuItem(menu, NETWORK_ACTION_WHOIS, PH_EMENU_DISABLED, PH_EMENU_DISABLED);
                                }
                            }
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
        {
            PhLayoutManagerLayout(&context->LayoutManager);
        }
        break;
    case WM_DPICHANGED:
        {
            PhLayoutManagerUpdate(&context->LayoutManager, LOWORD(wParam));
            PhLayoutManagerLayout(&context->LayoutManager);
        }
        break;
    case NTM_RECEIVEDFINISH:
        {
            BOOLEAN failed = (BOOLEAN)lParam;

            EnableWindow(GetDlgItem(hwndDlg, IDC_REFRESH), TRUE);

            PhSetWindowText(context->WindowHandle, PhaFormatString(
                L"Tracing %s... %s",
                context->RemoteAddressString,
                failed ? L"error" : L"complete"
                )->Buffer);
            PhSetWindowText(GetDlgItem(hwndDlg, IDC_STATUS), PhaFormatString(
                L"Tracing route to %s with %lu bytes of data... %s.",
                context->RemoteAddressString,
                PhGetIntegerSetting(SETTING_NAME_PING_SIZE),
                failed ? L"error" : L"complete"
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
                    if (PhFindStringInString(traceNode->HostnameString, 0, PhGetStringOrEmpty(hostName)) == SIZE_MAX)
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
    case WM_CTLCOLORBTN:
        return HANDLE_WM_CTLCOLORBTN(hwndDlg, wParam, lParam, PhWindowThemeControlColor);
    case WM_CTLCOLORDLG:
        return HANDLE_WM_CTLCOLORDLG(hwndDlg, wParam, lParam, PhWindowThemeControlColor);
    case WM_CTLCOLORSTATIC:
        return HANDLE_WM_CTLCOLORSTATIC(hwndDlg, wParam, lParam, PhWindowThemeControlColor);
    }

    return FALSE;
}

_Function_class_(USER_THREAD_START_ROUTINE)
NTSTATUS TracertDialogThreadStart(
    _In_ PVOID Parameter
    )
{
    PNETWORK_TRACERT_CONTEXT context = (PNETWORK_TRACERT_CONTEXT)Parameter;
    BOOL result;
    MSG message;
    HWND windowHandle;
    PH_AUTO_POOL autoPool;

    PhInitializeAutoPool(&autoPool);

    windowHandle = PhCreateDialog(
        PluginInstance->DllBase,
        MAKEINTRESOURCE(IDD_TRACERT),
        NULL,
        TracertDlgProc,
        Parameter
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
    _In_ HWND ParentWindowHandle,
    _In_ PPH_NETWORK_ITEM NetworkItem
    )
{
    PNETWORK_TRACERT_CONTEXT context;

    context = CreateTracertContext();
    context->MaximumHops = PhGetIntegerSetting(SETTING_NAME_TRACERT_MAX_HOPS);
    context->ParentWindowHandle = ParentWindowHandle;

    memcpy_s(
        &context->RemoteEndpoint,
        sizeof(context->RemoteEndpoint),
        &NetworkItem->RemoteEndpoint,
        sizeof(NetworkItem->RemoteEndpoint)
        );

    if (NetworkItem->RemoteEndpoint.Address.Type == PH_NETWORK_TYPE_IPV4)
    {
        ULONG remoteAddressStringLength = RTL_NUMBER_OF(context->RemoteAddressString);

        if (NT_SUCCESS(PhIpv4AddressToString(
            &NetworkItem->RemoteEndpoint.Address.InAddr,
            0,
            context->RemoteAddressString,
            &remoteAddressStringLength
            )))
        {
            context->RemoteAddressStringLength = (remoteAddressStringLength - 1) * sizeof(WCHAR);
        }
    }
    else
    {
        ULONG remoteAddressStringLength = RTL_NUMBER_OF(context->RemoteAddressString);

        if (NT_SUCCESS(PhIpv6AddressToString(
            &NetworkItem->RemoteEndpoint.Address.In6Addr,
            0,
            0,
            context->RemoteAddressString,
            &remoteAddressStringLength
            )))
        {
            context->RemoteAddressStringLength = (remoteAddressStringLength - 1) * sizeof(WCHAR);
        }
    }

    PhCreateThread2(TracertDialogThreadStart, context);
}

VOID ShowTracertWindowFromAddress(
    _In_ HWND ParentWindowHandle,
    _In_ PH_IP_ENDPOINT RemoteEndpoint
    )
{
    PNETWORK_TRACERT_CONTEXT context;

    context = CreateTracertContext();
    context->MaximumHops = PhGetIntegerSetting(SETTING_NAME_TRACERT_MAX_HOPS);
    context->ParentWindowHandle = ParentWindowHandle;

    memcpy_s(
        &context->RemoteEndpoint,
        sizeof(context->RemoteEndpoint),
        &RemoteEndpoint,
        sizeof(RemoteEndpoint)
        );

    if (RemoteEndpoint.Address.Type == PH_NETWORK_TYPE_IPV4)
    {
        ULONG remoteAddressStringLength = RTL_NUMBER_OF(context->RemoteAddressString);

        if (NT_SUCCESS(PhIpv4AddressToString(
            &RemoteEndpoint.Address.InAddr,
            0,
            context->RemoteAddressString,
            &remoteAddressStringLength
            )))
        {
            context->RemoteAddressStringLength = (remoteAddressStringLength - 1) * sizeof(WCHAR);
        }
    }
    else
    {
        ULONG remoteAddressStringLength = RTL_NUMBER_OF(context->RemoteAddressString);

        if (NT_SUCCESS(PhIpv6AddressToString(
            &RemoteEndpoint.Address.In6Addr,
            0,
            0,
            context->RemoteAddressString,
            &remoteAddressStringLength
            )))
        {
            context->RemoteAddressStringLength = (remoteAddressStringLength - 1) * sizeof(WCHAR);
        }
    }

    PhCreateThread2(TracertDialogThreadStart, context);
}
