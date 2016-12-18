/*
 * Process Hacker Network Tools -
 *   Tracert dialog
 *
 * Copyright (C) 2015-2016 dmex
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
#include <commonutil.h>
#include <math.h>

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

VOID TracertUpdateTime(
    _In_ PNETWORK_TRACERT_CONTEXT Context,
    _In_ PPOOLTAG_ROOT_NODE Node,
    _In_ INT SubIndex,
    _In_ ULONG RoundTripTime
    ) 
{ 
    if (RoundTripTime)
    {
        Node->PingList[SubIndex] = RoundTripTime;

        UpdateTracertNode(Context, Node);
    } 
    else 
    { 
        Node->PingList[SubIndex] = ULONG_MAX;

        switch (SubIndex)
        {
        case 0:
            Node->Ping1String = PhFormatString(L"<1 ms");
            break;
        case 1:
            Node->Ping2String = PhFormatString(L"<1 ms");
            break;
        case 2:
            Node->Ping3String = PhFormatString(L"<1 ms");
            break;
        case 3:
            Node->Ping4String = PhFormatString(L"<1 ms");
            break;
        }

        UpdateTracertNode(Context, Node);
    } 
} 

NTSTATUS TracertHostnameLookupCallback(
    _In_ PVOID Parameter
    )
{
    PTRACERT_RESOLVE_WORKITEM resolve = Parameter;

    if (resolve->Type == PH_IPV4_NETWORK_TYPE)
    {
        if (!GetNameInfo(
            (PSOCKADDR)&resolve->SocketAddress,
            sizeof(SOCKADDR_IN),
            resolve->SocketAddressHostname,
            sizeof(resolve->SocketAddressHostname),
            NULL,
            0,
            NI_NAMEREQD
            ))
        {
            resolve->Node->HostnameString = PhCreateString(resolve->SocketAddressHostname);
            resolve->Node->HostnameValid = TRUE;
        }
        else
        {
            ULONG errorCode = WSAGetLastError();

            if (errorCode != WSAHOST_NOT_FOUND && errorCode != WSATRY_AGAIN)
            {
                //PPH_STRING errorMessage = PhGetWin32Message(errorCode);
                //PhSetListViewSubItem(work->LvHandle, work->LvItemIndex, HOSTNAME_COLUMN, errorMessage->Buffer);
                //PhDereferenceObject(errorMessage);
            }

            PhDereferenceObject(resolve);
        }
    }
    else if (resolve->Type == PH_IPV6_NETWORK_TYPE)
    {
        if (!GetNameInfo(
            (PSOCKADDR)&resolve->SocketAddress,
            sizeof(SOCKADDR_IN6),
            resolve->SocketAddressHostname,
            sizeof(resolve->SocketAddressHostname),
            NULL,
            0,
            NI_NAMEREQD
            ))
        {
            resolve->Node->HostnameString = PhCreateString(resolve->SocketAddressHostname);
            resolve->Node->HostnameValid = TRUE;
        }
        else
        {
            ULONG errorCode = WSAGetLastError();

            if (errorCode != WSAHOST_NOT_FOUND && errorCode != WSATRY_AGAIN)
            {
                //PPH_STRING errorMessage = PhGetWin32Message(errorCode);
                //PhSetListViewSubItem(work->LvHandle, work->LvItemIndex, HOSTNAME_COLUMN, errorMessage->Buffer);
                //PhDereferenceObject(errorMessage);
            }

            PhDereferenceObject(resolve);
        }
    }

    return STATUS_SUCCESS;
}

NTSTATUS TracertGeoIpLookupCallback(
    _In_ PVOID Parameter
    )
{
    PTRACERT_RESOLVE_WORKITEM resolve = Parameter;
    DOUBLE currentLatitude = 0.0;
    DOUBLE currentLongitude = 0.0;

    LookupGeoIpCurrentCity(&currentLatitude, &currentLongitude);

    PhSwapReference(&resolve->Node->RemoteCityDistance, GeoLookupCityDistance(
        resolve->CityLatitude,
        resolve->CityLongitude,
        currentLatitude,
        currentLongitude
        ));

    return STATUS_SUCCESS;
}

VOID TracertQueueHostLookup(
    _In_ PNETWORK_TRACERT_CONTEXT Context,
    _In_ PPOOLTAG_ROOT_NODE Node,
    _In_ PVOID SocketAddress
    ) 
{
    if (Context->RemoteEndpoint.Address.Type == PH_IPV4_NETWORK_TYPE)
    {
        IN_ADDR sockAddrIn;
        PTRACERT_RESOLVE_WORKITEM resolve;
        PPH_STRING remoteCountryCode;
        PPH_STRING remoteCountryName;
        ULONG addressStringLength = INET_ADDRSTRLEN;
        WCHAR addressString[INET_ADDRSTRLEN] = L"";

        memset(&sockAddrIn, 0, sizeof(IN_ADDR));
        memcpy(&sockAddrIn, SocketAddress, sizeof(IN_ADDR));

        if (NT_SUCCESS(RtlIpv4AddressToStringEx(&sockAddrIn, 0, addressString, &addressStringLength)))
        {
            Node->IpAddressString = PhCreateString(addressString);
        }

        resolve = PhCreateAlloc(sizeof(TRACERT_RESOLVE_WORKITEM));
        memset(resolve, 0, sizeof(TRACERT_RESOLVE_WORKITEM));

        resolve->Type = PH_IPV4_NETWORK_TYPE;
        resolve->WindowHandle = Context->WindowHandle;
        resolve->Node = Node;

        ((PSOCKADDR_IN)&resolve->SocketAddress)->sin_family = AF_INET;
        ((PSOCKADDR_IN)&resolve->SocketAddress)->sin_addr = sockAddrIn;

        if (LookupSockAddrCountryCode(
            sockAddrIn,
            &remoteCountryCode,
            &remoteCountryName,
            &resolve->CityLatitude,
            &resolve->CityLongitude
            ))
        {
            PhSwapReference(&Node->RemoteCountryCode, remoteCountryCode);
            PhSwapReference(&Node->RemoteCountryName, remoteCountryName);

            if (resolve->CityLatitude != 0.0 && resolve->CityLongitude != 0.0)
            {
                PhQueueItemWorkQueue(&Context->WorkQueue, TracertGeoIpLookupCallback, resolve);
            }
        }

        PhQueueItemWorkQueue(&Context->WorkQueue, TracertHostnameLookupCallback, resolve);
    }
    else if (Context->RemoteEndpoint.Address.Type == PH_IPV6_NETWORK_TYPE)
    {
        IN6_ADDR sockAddrIn6;
        PTRACERT_RESOLVE_WORKITEM resolve;
        ULONG addressStringLength = INET6_ADDRSTRLEN;
        WCHAR addressString[INET6_ADDRSTRLEN] = L"";

        memset(&sockAddrIn6, 0, sizeof(IN6_ADDR));
        memcpy(&sockAddrIn6, SocketAddress, sizeof(IN6_ADDR));
       
        if (NT_SUCCESS(RtlIpv6AddressToStringEx(&sockAddrIn6, 0, 0, addressString, &addressStringLength)))
        {
            Node->IpAddressString = PhCreateString(addressString);
        }

        resolve = PhCreateAlloc(sizeof(TRACERT_RESOLVE_WORKITEM));
        memset(resolve, 0, sizeof(TRACERT_RESOLVE_WORKITEM));

        resolve->Type = PH_IPV6_NETWORK_TYPE;
        resolve->WindowHandle = Context->WindowHandle;
        resolve->Node = Node;

        ((PSOCKADDR_IN6)&resolve->SocketAddress)->sin6_family = AF_INET6;
        ((PSOCKADDR_IN6)&resolve->SocketAddress)->sin6_addr = sockAddrIn6;

        PhQueueItemWorkQueue(&Context->WorkQueue, TracertHostnameLookupCallback, resolve);
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

    if (icmpRandString = PhCreateStringEx(NULL, PhGetIntegerSetting(SETTING_NAME_PING_SIZE) * 2 + 2))
    {
        PhGenerateRandomAlphaString(icmpRandString->Buffer, (ULONG)icmpRandString->Length / sizeof(WCHAR));

        icmpEchoBuffer = PhConvertUtf16ToMultiByte(icmpRandString->Buffer);
        PhDereferenceObject(icmpRandString);
    }

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
        //((PSOCKADDR_IN)&destinationAddress)->sin_port = (USHORT)context->RemoteEndpoint.Port;//_byteswap_ushort((USHORT)Context->RemoteEndpoint.Port);
    }
    else if (context->RemoteEndpoint.Address.Type == PH_IPV6_NETWORK_TYPE)
    {
        ((PSOCKADDR_IN6)&destinationAddress)->sin6_family = AF_INET6;
        ((PSOCKADDR_IN6)&destinationAddress)->sin6_addr = context->RemoteEndpoint.Address.In6Addr;
        //((PSOCKADDR_IN6)&destinationAddress)->sin6_port = (USHORT)context->RemoteEndpoint.Port;//_byteswap_ushort((USHORT)Context->RemoteEndpoint.Port);
    }

    for (INT i = 0; i < DEFAULT_MAXIMUM_HOPS; i++)
    {
        IN_ADDR last4ReplyAddress = in4addr_any;
        IN6_ADDR last6ReplyAddress = in6addr_any;

        if (context->Cancel)
            break;

        PPOOLTAG_ROOT_NODE node = AddTracertNode(context, pingOptions.Ttl);

        TreeNew_NodesStructured(context->TreeNewHandle);

        for (INT ii = 0; ii < MAX_PINGS; ii++)
        {
            if (context->Cancel)
                break;

            if (context->RemoteEndpoint.Address.Type == PH_IPV4_NETWORK_TYPE)
            {
                icmpReplyLength = ICMP_BUFFER_SIZE(sizeof(ICMP_ECHO_REPLY), icmpEchoBuffer);
                icmpReplyBuffer = PhAllocate(icmpReplyLength);
                memset(icmpReplyBuffer, 0, icmpReplyLength);

                if (IcmpSendEcho2Ex(
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
                    ))
                {
                    PICMP_ECHO_REPLY reply4 = (PICMP_ECHO_REPLY)icmpReplyBuffer;

                    TracertUpdateTime(
                        context,
                        node,
                        ii,
                        reply4->RoundTripTime
                        );

                    TracertQueueHostLookup(
                        context,
                        node,
                        &reply4->Address
                        );

                    memcpy(&last4ReplyAddress, &reply4->Address, sizeof(IN_ADDR));

                    if (reply4->Status == IP_HOP_LIMIT_EXCEEDED)
                    {
                        if (reply4->RoundTripTime < MIN_INTERVAL)
                        {
                            //LARGE_INTEGER interval;
                            //NtDelayExecution(FALSE, PhTimeoutFromMilliseconds(&interval, MIN_INTERVAL - reply4->RoundTripTime));
                        }
                    }
                    else if (reply4->Status != IP_SUCCESS)
                    {
                        node->PingList[ii] = ULONG_MAX;
                    }
                }
                else
                {
                    node->PingList[ii] = ULONG_MAX;
                }

                PhFree(icmpReplyBuffer);
            }
            else
            {
                icmpReplyLength = ICMP_BUFFER_SIZE(sizeof(ICMPV6_ECHO_REPLY), icmpEchoBuffer);
                icmpReplyBuffer = PhAllocate(icmpReplyLength);
                memset(icmpReplyBuffer, 0, icmpReplyLength);

                if (Icmp6SendEcho2(
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
                    ))
                {
                    PICMPV6_ECHO_REPLY reply6 = (PICMPV6_ECHO_REPLY)icmpReplyBuffer;

                    TracertUpdateTime(
                        context,
                        node,
                        ii,
                        reply6->RoundTripTime
                        );

                    TracertQueueHostLookup(
                        context,
                        node,
                        &reply6->Address.sin6_addr
                        );

                    memcpy(&last6ReplyAddress, &reply6->Address.sin6_addr, sizeof(IN6_ADDR));

                    if (reply6->Status == IP_HOP_LIMIT_EXCEEDED)
                    {
                        if (reply6->RoundTripTime < MIN_INTERVAL)
                        {
                            //LARGE_INTEGER interval;
                            //NtDelayExecution(FALSE, PhTimeoutFromMilliseconds(&interval, MIN_INTERVAL - reply6->RoundTripTime));
                        }
                    }
                    else if (reply6->Status != IP_SUCCESS)
                    {
                        node->PingList[ii] = ULONG_MAX;

                        if (reply6->Status != IP_REQ_TIMED_OUT)
                        {
                            PPH_STRING errorMessage;

                            if (errorMessage = PH_AUTO(TracertGetErrorMessage(reply6->Status)))
                            {
                                node->IpAddressString = errorMessage;
                            }
                        }
                    }
                }
                else
                {
                    ULONG errorCode = GetLastError();

                    node->PingList[ii] = ULONG_MAX;

                    if (errorCode != IP_REQ_TIMED_OUT)
                    {
                        PPH_STRING errorMessage;

                        if (errorMessage = PH_AUTO(TracertGetErrorMessage(errorCode)))
                        {
                            node->IpAddressString = errorMessage;
                        }
                    }
                }

                PhFree(icmpReplyBuffer);
            }
        }

        if (context->RemoteEndpoint.Address.Type == PH_IPV4_NETWORK_TYPE)
        {
            if (!memcmp(&last4ReplyAddress, &((PSOCKADDR_IN)&destinationAddress)->sin_addr, sizeof(IN_ADDR)))
                break;
        }
        else if (context->RemoteEndpoint.Address.Type == PH_IPV6_NETWORK_TYPE)
        {
            if (!memcmp(&last6ReplyAddress, &((PSOCKADDR_IN6)&destinationAddress)->sin6_addr, sizeof(IN6_ADDR)))
                break;
        }

        pingOptions.Ttl++;
    }

CleanupExit:

    if (icmpHandle != INVALID_HANDLE_VALUE)
    {
        IcmpCloseHandle(icmpHandle);
    }

    PhDereferenceObject(context);

    PostMessage(context->WindowHandle, NTM_RECEIVEDFINISH, 0, 0);
    return STATUS_SUCCESS;
}

VOID ShowMenu(
    _In_ PNETWORK_TRACERT_CONTEXT Context, 
    _In_ ULONG Id
    )
{
    switch (Id)
    {
    case MAINMENU_ACTION_PING:
        {
            PH_IP_ENDPOINT RemoteEndpoint;
            PWSTR terminator = NULL;
            PPOOLTAG_ROOT_NODE node;

            if (node = GetSelectedTracertNode(Context))
            {
                if (NT_SUCCESS(RtlIpv4StringToAddress(node->IpAddressString->Buffer, TRUE, &terminator, &RemoteEndpoint.Address.InAddr)))
                {
                    RemoteEndpoint.Address.Type = PH_IPV4_NETWORK_TYPE;
                    ShowPingWindowFromAddress(RemoteEndpoint);
                    break;
                }

                if (NT_SUCCESS(RtlIpv6StringToAddress(node->IpAddressString->Buffer, &terminator, &RemoteEndpoint.Address.In6Addr)))
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
            PWSTR terminator = NULL;
            PPOOLTAG_ROOT_NODE node;

            if (node = GetSelectedTracertNode(Context))
            {
                if (NT_SUCCESS(RtlIpv4StringToAddress(node->IpAddressString->Buffer, TRUE, &terminator, &RemoteEndpoint.Address.InAddr)))
                {
                    RemoteEndpoint.Address.Type = PH_IPV4_NETWORK_TYPE;
                    ShowTracertWindowFromAddress(RemoteEndpoint);
                    break;
                }
    
                if (NT_SUCCESS(RtlIpv6StringToAddress(node->IpAddressString->Buffer, &terminator, &RemoteEndpoint.Address.In6Addr)))
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
            PWSTR terminator = NULL;
            PPOOLTAG_ROOT_NODE node;

            if (node = GetSelectedTracertNode(Context))
            {
                if (NT_SUCCESS(RtlIpv4StringToAddress(node->IpAddressString->Buffer, TRUE, &terminator, &RemoteEndpoint.Address.InAddr)))
                {
                    RemoteEndpoint.Address.Type = PH_IPV4_NETWORK_TYPE;
                    ShowWhoisWindowFromAddress(RemoteEndpoint);
                    break;
                }
    
                if (NT_SUCCESS(RtlIpv6StringToAddress(node->IpAddressString->Buffer, &terminator, &RemoteEndpoint.Address.In6Addr)))
                {
                    RemoteEndpoint.Address.Type = PH_IPV6_NETWORK_TYPE;
                    ShowWhoisWindowFromAddress(RemoteEndpoint);
                    break;
                }
            }
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
        SetProp(hwndDlg, L"Context", (HANDLE)context);
    }
    else
    {
        context = (PNETWORK_TRACERT_CONTEXT)GetProp(hwndDlg, L"Context");

        if (uMsg == WM_DESTROY)
        {
            context->Cancel = TRUE;

            PhSaveWindowPlacementToSetting(SETTING_NAME_TRACERT_WINDOW_POSITION, SETTING_NAME_TRACERT_WINDOW_SIZE, hwndDlg);

            if (context->FontHandle)
                DeleteObject(context->FontHandle);

            DeleteTracertTree(context);

            PhDeleteWorkQueue(&context->WorkQueue);
            PhDeleteLayoutManager(&context->LayoutManager);
            RemoveProp(hwndDlg, L"Context");
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
            HANDLE tracertThread;

            PhCenterWindow(hwndDlg, PhMainWndHandle);            

            Static_SetText(hwndDlg,
                PhaFormatString(L"Tracing %s...", context->IpAddressString)->Buffer
                );
            Static_SetText(GetDlgItem(hwndDlg, IDC_STATUS),
                PhaFormatString(L"Tracing route to %s with %lu bytes of data...", context->IpAddressString, PhGetIntegerSetting(SETTING_NAME_PING_SIZE))->Buffer
                );

            context->WindowHandle = hwndDlg;
            context->TreeNewHandle = GetDlgItem(hwndDlg, IDC_LIST_TRACERT);
            context->FontHandle = CommonCreateFont(-15, GetDlgItem(hwndDlg, IDC_STATUS));

            InitializeTracertTree(context);

            PhInitializeWorkQueue(&context->WorkQueue, 0, 40, 5000);
            PhInitializeLayoutManager(&context->LayoutManager, hwndDlg);
            PhAddLayoutItem(&context->LayoutManager, GetDlgItem(hwndDlg, IDC_STATUS), NULL, PH_ANCHOR_TOP | PH_ANCHOR_LEFT | PH_ANCHOR_RIGHT | PH_LAYOUT_FORCE_INVALIDATE);
            PhAddLayoutItem(&context->LayoutManager, context->TreeNewHandle, NULL, PH_ANCHOR_ALL);
            PhAddLayoutItem(&context->LayoutManager, GetDlgItem(hwndDlg, IDCANCEL), NULL, PH_ANCHOR_BOTTOM | PH_ANCHOR_RIGHT);
            PhLoadWindowPlacementFromSetting(SETTING_NAME_TRACERT_WINDOW_POSITION, SETTING_NAME_TRACERT_WINDOW_SIZE, hwndDlg);

            PhReferenceObject(context);

            if (tracertThread = PhCreateThread(0, NetworkTracertThreadStart, (PVOID)context))
                NtClose(tracertThread);

            EnableThemeDialogTexture(hwndDlg, ETDT_ENABLETAB);
        }
        break;
    case WM_COMMAND:
        {
            switch (GET_WM_COMMAND_ID(wParam, lParam))
            {
            case IDCANCEL:
                DestroyWindow(hwndDlg);
                break;
            case POOL_TABLE_SHOWCONTEXTMENU:
                {
                    PPH_EMENU menu;
                    PPOOLTAG_ROOT_NODE selectedNode;
                    PPH_EMENU_ITEM selectedItem;
                    PPH_TREENEW_MOUSE_EVENT mouseEvent = (PPH_TREENEW_MOUSE_EVENT)lParam;

                    if (selectedNode = GetSelectedTracertNode(context))
                    {
                        menu = PhCreateEMenu();
                        PhInsertEMenuItem(menu, PhCreateEMenuItem(0, MAINMENU_ACTION_PING, L"Ping", NULL, NULL), -1);
                        PhInsertEMenuItem(menu, PhCreateEMenuItem(0, NETWORK_ACTION_TRACEROUTE, L"Traceroute", NULL, NULL), -1);
                        PhInsertEMenuItem(menu, PhCreateEMenuItem(0, NETWORK_ACTION_WHOIS, L"Whois", NULL, NULL), -1);

                        selectedItem = PhShowEMenu(
                            menu,
                            hwndDlg,
                            PH_EMENU_SHOW_LEFTRIGHT,
                            PH_ALIGN_LEFT | PH_ALIGN_TOP,
                            mouseEvent->Location.x,
                            mouseEvent->Location.y
                            );

                        if (selectedItem && selectedItem->Id != -1)
                        {
                            ShowMenu(context, selectedItem->Id);
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
            PPH_STRING windowText;

            if (windowText = PH_AUTO(PhGetWindowText(context->WindowHandle)))
            {
                Static_SetText(
                    context->WindowHandle,
                    PhaFormatString(L"%s complete", windowText->Buffer)->Buffer
                    );
            }

            if (windowText = PH_AUTO(PhGetWindowText(GetDlgItem(hwndDlg, IDC_STATUS))))
            {
                Static_SetText(
                    GetDlgItem(hwndDlg, IDC_STATUS),
                    PhaFormatString(L"%s complete", windowText->Buffer)->Buffer
                    );
            }
            
            TreeNew_NodesStructured(context->TreeNewHandle);
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

        if (!IsDialogMessage(context->WindowHandle, &message))
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
    HANDLE dialogThread;
    PNETWORK_TRACERT_CONTEXT context;

    context = (PNETWORK_TRACERT_CONTEXT)PhCreateAlloc(sizeof(NETWORK_TRACERT_CONTEXT));
    memset(context, 0, sizeof(NETWORK_TRACERT_CONTEXT));

    context->RemoteEndpoint = NetworkItem->RemoteEndpoint;

    if (NetworkItem->RemoteEndpoint.Address.Type == PH_IPV4_NETWORK_TYPE)
    {
        RtlIpv4AddressToString(&NetworkItem->RemoteEndpoint.Address.InAddr, context->IpAddressString);
    }
    else if (NetworkItem->RemoteEndpoint.Address.Type == PH_IPV6_NETWORK_TYPE)
    {
        RtlIpv6AddressToString(&NetworkItem->RemoteEndpoint.Address.In6Addr, context->IpAddressString);
    }

    if (dialogThread = PhCreateThread(0, TracertDialogThreadStart, (PVOID)context))
    {
        NtClose(dialogThread);
    }
}

VOID ShowTracertWindowFromAddress(
    _In_ PH_IP_ENDPOINT RemoteEndpoint
    )
{
    HANDLE dialogThread;
    PNETWORK_TRACERT_CONTEXT context;

    context = (PNETWORK_TRACERT_CONTEXT)PhCreateAlloc(sizeof(NETWORK_TRACERT_CONTEXT));
    memset(context, 0, sizeof(NETWORK_TRACERT_CONTEXT));

    context->RemoteEndpoint = RemoteEndpoint;

    if (RemoteEndpoint.Address.Type == PH_IPV4_NETWORK_TYPE)
    {
        RtlIpv4AddressToString(&RemoteEndpoint.Address.InAddr, context->IpAddressString);
    }
    else if (RemoteEndpoint.Address.Type == PH_IPV6_NETWORK_TYPE)
    {
        RtlIpv6AddressToString(&RemoteEndpoint.Address.In6Addr, context->IpAddressString);
    }

    if (dialogThread = PhCreateThread(0, TracertDialogThreadStart, (PVOID)context))
    {
        NtClose(dialogThread);
    }
}