/*
 * Copyright (c) 2022 Winsider Seminars & Solutions, Inc.  All rights reserved.
 *
 * This file is part of System Informer.
 *
 * Authors:
 *
 *     dmex    2013-2023
 *
 */

#include "nettools.h"
#include <richedit.h>

PPH_OBJECT_TYPE WhoisContextType = NULL;
PH_INITONCE WhoisContextTypeInitOnce = PH_INITONCE_INIT;

VOID RichEditSetText(
    _In_ HWND RichEditHandle,
    _In_ PWSTR Text
    )
{
    if (PhGetIntegerSetting(L"EnableThemeSupport"))
    {
        CHARFORMAT cf;

        memset(&cf, 0, sizeof(CHARFORMAT));
        cf.cbSize = sizeof(CHARFORMAT);
        cf.dwMask = CFM_COLOR;

        switch (PhGetIntegerSetting(L"GraphColorMode"))
        {
        case 0: // New colors
            cf.crTextColor = RGB(0x0, 0x0, 0x0);
            break;
        case 1: // Old colors
            cf.crTextColor = RGB(0xff, 0xff, 0xff);
            break;
        }

        SendMessage(RichEditHandle, EM_SETCHARFORMAT, SCF_DEFAULT, (LPARAM)&cf);
    }

    SetFocus(RichEditHandle);
    SendMessage(RichEditHandle, WM_SETREDRAW, FALSE, 0);
    SendMessage(RichEditHandle, EM_SETSEL, 0, -1); // -2
    SendMessage(RichEditHandle, EM_REPLACESEL, FALSE, (LPARAM)Text);
    SendMessage(RichEditHandle, WM_VSCROLL, SB_TOP, 0); // requires SetFocus()
    SendMessage(RichEditHandle, WM_SETREDRAW, TRUE, 0);

    InvalidateRect(RichEditHandle, NULL, FALSE);
}

PPH_STRING TrimString(
    _In_ PPH_STRING String
    )
{
    static PH_STRINGREF whitespace = PH_STRINGREF_INIT(L"  ");
    return PhCreateString3(&String->sr, 0, &whitespace);
}

PPH_STRING TrimString2(
    _In_ PPH_STRING String
    )
{
    static PH_STRINGREF whitespace = PH_STRINGREF_INIT(L"\n\n");
    return PhCreateString3(&String->sr, 0, &whitespace);
}

_Success_(return)
BOOLEAN ReadSocketString(
    _In_ SOCKET Handle,
    _Out_opt_ _Deref_post_z_cap_(*DataLength) PSTR* Data,
    _Out_opt_ ULONG* DataLength
    )
{
    PSTR data;
    WSABUF socketBuffer;
    ULONG socketFlags;
    ULONG allocatedLength;
    ULONG dataLength;
    ULONG returnLength;
    BYTE buffer[PAGE_SIZE];

    allocatedLength = sizeof(buffer);
    data = (PSTR)PhAllocate(allocatedLength);
    dataLength = 0;

    socketBuffer.buf = buffer;
    socketBuffer.len = PAGE_SIZE;
    socketFlags = 0;

    while (WSARecv(Handle, &socketBuffer, 1, &returnLength, &socketFlags, NULL, NULL) != SOCKET_ERROR)
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

    if (allocatedLength < dataLength + sizeof(ANSI_NULL))
    {
        allocatedLength++;
        data = (PSTR)PhReAllocate(data, allocatedLength);
    }

    data[dataLength] = ANSI_NULL;

    if (dataLength)
    {
        if (Data)
            *Data = data;
        else
            PhFree(data);

        if (DataLength)
            *DataLength = dataLength;

        return TRUE;
    }

    PhFree(data);

    return FALSE;
}

_Success_(return)
BOOLEAN WriteSocketString(
    _In_ SOCKET Handle,
    _In_ PSTR Data,
    _In_ ULONG DataLength
    )
{
    ULONG socketBufferSent;
    WSABUF socketBuffer;

    socketBufferSent = 0;
    socketBuffer.buf = Data;
    socketBuffer.len = DataLength;

    if (WSASend(Handle, &socketBuffer, 1, &socketBufferSent, 0, NULL, NULL) != SOCKET_ERROR)
    {
        if (socketBufferSent == DataLength)
            return TRUE;
    }

    return FALSE;
}

_Success_(return)
BOOLEAN WhoisExtractServerUrl(
    _In_ PPH_STRING WhoisResponse,
    _Out_opt_ PPH_STRING *WhoisServerAddress
    )
{
    ULONG_PTR whoisServerHostnameIndex;
    ULONG_PTR whoisServerHostnameLength;
    PPH_STRING whoisServerName;

    if ((whoisServerHostnameIndex = PhFindStringInString(WhoisResponse, 0, L"whois:")) == SIZE_MAX)
        return FALSE;
    if ((whoisServerHostnameLength = PhFindStringInString(WhoisResponse, whoisServerHostnameIndex, L"\n")) == SIZE_MAX)
        return FALSE;
    if ((whoisServerHostnameLength = whoisServerHostnameLength - whoisServerHostnameIndex) == 0)
        return FALSE;

    whoisServerName = PhSubstring(
        WhoisResponse,
        whoisServerHostnameIndex + RTL_NUMBER_OF(L"whois:") - 1,
        whoisServerHostnameLength - RTL_NUMBER_OF(L"whois:") - 1
        );

    if (WhoisServerAddress)
        *WhoisServerAddress = TrimString(whoisServerName);

    PhDereferenceObject(whoisServerName);
    return TRUE;
}

_Success_(return)
BOOLEAN WhoisExtractReferralServer(
    _In_ PPH_STRING WhoisResponse,
    _Out_opt_ PPH_STRING *WhoisServerAddress,
    _Out_opt_ USHORT *WhoisServerPort
    )
{
    ULONG_PTR whoisServerHostnameIndex;
    ULONG_PTR whoisServerHostnameLength;
    PPH_STRING whoisServerName;
    PPH_STRING whoisServerHostname;
    WCHAR urlProtocal[0x100] = L"";
    WCHAR urlHost[0x100] = L"";
    WCHAR urlPort[0x100] = L"";
    WCHAR urlPath[0x100] = L"";

    if ((whoisServerHostnameIndex = PhFindStringInString(WhoisResponse, 0, L"ReferralServer:")) == SIZE_MAX)
        return FALSE;
    if ((whoisServerHostnameLength = PhFindStringInString(WhoisResponse, whoisServerHostnameIndex, L"\n")) == SIZE_MAX)
        return FALSE;
    if ((whoisServerHostnameLength = whoisServerHostnameLength - whoisServerHostnameIndex) == 0)
        return FALSE;

    whoisServerName = PhSubstring(
        WhoisResponse,
        whoisServerHostnameIndex + RTL_NUMBER_OF(L"ReferralServer:") - 1,
        whoisServerHostnameLength - RTL_NUMBER_OF(L"ReferralServer:") - 1
        );

    whoisServerHostname = TrimString(whoisServerName);

    if (swscanf_s(
        whoisServerHostname->Buffer,
        L"%255[^:]://%255[^:]:%255[^/]/%255s",
        urlProtocal,
        (UINT)RTL_NUMBER_OF(urlProtocal),
        urlHost,
        (UINT)RTL_NUMBER_OF(urlHost),
        urlPort,
        (UINT)RTL_NUMBER_OF(urlPort),
        urlPath,
        (UINT)RTL_NUMBER_OF(urlPath)
        ))
    {
        if (WhoisServerAddress)
        {
            *WhoisServerAddress = PhCreateString(urlHost);
        }

        if (WhoisServerPort)
        {
            USHORT serverPort = IPPORT_WHOIS;

            if (PhCountStringZ(urlPort) >= 2)
            {
                ULONG64 serverPortInteger;
                PH_STRINGREF serverPortSr;

                PhInitializeStringRefLongHint(&serverPortSr, urlPort);

                if (PhStringToInteger64(&serverPortSr, 10, &serverPortInteger))
                {
                    serverPort = (USHORT)serverPortInteger;
                }
            }

            *WhoisServerPort = serverPort;
        }

        PhDereferenceObject(whoisServerName);
        PhDereferenceObject(whoisServerHostname);

        return TRUE;
    }

    PhDereferenceObject(whoisServerName);
    PhDereferenceObject(whoisServerHostname);

    return FALSE;
}

SOCKET WhoisDualStackSocketCreate(
    VOID
    )
{
    SOCKET socketHandle;

    socketHandle = WSASocket(
        AF_INET6,
        SOCK_STREAM,
        IPPROTO_TCP,
        NULL,
        0,
        WSA_FLAG_OVERLAPPED | WSA_FLAG_NO_HANDLE_INHERIT
        );

    if (socketHandle == INVALID_SOCKET)
        return INVALID_SOCKET;

    if (setsockopt(
        socketHandle,
        IPPROTO_IPV6,
        IPV6_V6ONLY,
        (PCSTR)&(INT){ FALSE },
        sizeof(INT)
        ) == SOCKET_ERROR)
    {
        closesocket(socketHandle);
        socketHandle = INVALID_SOCKET;
    }

    return socketHandle;
}

_Success_(return)
BOOLEAN WhoisDualStackSocketConnectByAddressList(
    _In_ PDNS_RECORD DnsRecordList,
    _In_ USHORT ServerPort,
    _Out_ SOCKET* SocketHandle
    )
{
    BOOLEAN success = FALSE;
    SOCKET socketHandle = INVALID_SOCKET;
    ULONG socketAddressListSize = 0;
    ULONG socketAddressListBytes = 0;
    PSOCKADDR_IN6 socketAddressArray = NULL;
    PSOCKET_ADDRESS_LIST socketAddressList = NULL;
    INT dnsRecordListCount = 0;
    INT socketAddressCount = 0;

    for (PDNS_RECORD i = DnsRecordList; i; i = i->pNext)
    {
        dnsRecordListCount += 1;
    }

    socketAddressArray = PhAllocate(dnsRecordListCount * sizeof(SOCKADDR_IN6));
    memset(socketAddressArray, 0, dnsRecordListCount * sizeof(SOCKADDR_IN6));

    socketAddressListSize = SIZEOF_SOCKET_ADDRESS_LIST(dnsRecordListCount) + (dnsRecordListCount * sizeof(SOCKADDR_STORAGE));
    socketAddressList = PhAllocate(socketAddressListSize);
    memset(socketAddressList, 0, socketAddressListSize);

    for (PDNS_RECORD i = DnsRecordList; i; i = i->pNext)
    {
        if (i->wType == DNS_TYPE_A)
        {
            SOCKADDR_IN remoteAddress;

            memset(&remoteAddress, 0, sizeof(SOCKADDR_IN));
            remoteAddress.sin_family = AF_INET;
            remoteAddress.sin_port = _byteswap_ushort(ServerPort);
            memcpy_s(
                &remoteAddress.sin_addr.s_addr,
                sizeof(remoteAddress.sin_addr.s_addr),
                &i->Data.A.IpAddress,
                sizeof(i->Data.A.IpAddress)
                );

            IN6ADDR_SETV4MAPPED(
                &socketAddressArray[socketAddressCount],
                &remoteAddress.sin_addr,
                IN4ADDR_SCOPE_ID(&remoteAddress),
                remoteAddress.sin_port
                );

            socketAddressList->Address[socketAddressCount].lpSockaddr = (PSOCKADDR)&socketAddressArray[socketAddressCount];
            socketAddressList->Address[socketAddressCount].iSockaddrLength = sizeof(SOCKADDR_IN6);
            socketAddressCount++;
        }
        else if (i->wType == DNS_TYPE_AAAA)
        {
            SOCKADDR_IN6 remoteAddress;

            memset(&remoteAddress, 0, sizeof(SOCKADDR_IN6));
            remoteAddress.sin6_family = AF_INET6;
            remoteAddress.sin6_port = _byteswap_ushort(ServerPort);
            memcpy_s(
                remoteAddress.sin6_addr.s6_addr,
                sizeof(remoteAddress.sin6_addr.s6_addr),
                i->Data.AAAA.Ip6Address.IP6Byte,
                sizeof(i->Data.AAAA.Ip6Address.IP6Byte)
                );

            IN6ADDR_SETSOCKADDR(
                &socketAddressArray[socketAddressCount],
                &remoteAddress.sin6_addr,
                remoteAddress.sin6_scope_struct,
                remoteAddress.sin6_port
                );

            socketAddressList->Address[socketAddressCount].lpSockaddr = (PSOCKADDR)&socketAddressArray[socketAddressCount];
            socketAddressList->Address[socketAddressCount].iSockaddrLength = sizeof(SOCKADDR_IN6);
            socketAddressCount++;
        }
    }

    socketAddressList->iAddressCount = socketAddressCount;

    if ((socketHandle = WhoisDualStackSocketCreate()) == INVALID_SOCKET)
        goto CleanupExit;

    if (WSAIoctl(
        socketHandle,
        SIO_ADDRESS_LIST_SORT,
        socketAddressList,
        socketAddressListSize,
        socketAddressList,
        socketAddressListSize,
        &socketAddressListBytes,
        NULL,
        NULL
        ) == SOCKET_ERROR)
    {
        goto CleanupExit;
    }

    if (!WSAConnectByList(
        socketHandle,
        socketAddressList,
        NULL,
        NULL,
        NULL,
        NULL,
        NULL,
        NULL
        ))
    {
        goto CleanupExit;
    }

    success = TRUE;

CleanupExit:
    if (socketAddressList)
    {
        PhFree(socketAddressList);
    }

    if (socketAddressArray)
    {
        PhFree(socketAddressArray);
    }

    if (success)
    {
        *SocketHandle = socketHandle;
    }
    else
    {
        if (socketHandle)
            closesocket(socketHandle);
    }

    return success;
}

_Success_(return)
BOOLEAN WhoisConnectByAddressList(
    _In_ PDNS_RECORD DnsRecordList,
    _In_ USHORT ServerPort,
    _Out_ SOCKET* SocketHandle
    )
{
    SOCKET socketHandle = INVALID_SOCKET;

    for (PDNS_RECORD i = DnsRecordList; i; i = i->pNext)
    {
        if (i->wType == DNS_TYPE_A)
        {
            SOCKADDR_IN remoteAddr;

            memset(&remoteAddr, 0, sizeof(SOCKADDR_IN));
            remoteAddr.sin_family = AF_INET;
            remoteAddr.sin_port = _byteswap_ushort(ServerPort);
            memcpy_s(
                &remoteAddr.sin_addr.s_addr,
                sizeof(remoteAddr.sin_addr.s_addr),
                &i->Data.A.IpAddress,
                sizeof(i->Data.A.IpAddress)
                );

            if ((socketHandle = WSASocket(AF_INET, SOCK_STREAM, IPPROTO_TCP, NULL, 0, WSA_FLAG_OVERLAPPED | WSA_FLAG_NO_HANDLE_INHERIT)) != INVALID_SOCKET)
            {
                ULONG bestInterfaceIndex;

                if (GetBestInterfaceEx((PSOCKADDR)&remoteAddr, &bestInterfaceIndex) == ERROR_SUCCESS)
                {
                    MIB_IPFORWARD_ROW2 bestAddressRoute = { 0 };
                    SOCKADDR_INET destinationAddress = { 0 };
                    SOCKADDR_INET bestSourceAddress = { 0 };

                    destinationAddress.si_family = AF_INET;
                    destinationAddress.Ipv4 = remoteAddr;

                    if (GetBestRoute2(
                        NULL,
                        bestInterfaceIndex,
                        NULL,
                        &destinationAddress,
                        0,
                        &bestAddressRoute,
                        &bestSourceAddress
                        ) == ERROR_SUCCESS)
                    {
                        bind(socketHandle, (PSOCKADDR)&bestSourceAddress.Ipv4, sizeof(bestSourceAddress.Ipv4));
                    }
                }

                if (WSAConnect(socketHandle, (PSOCKADDR)&remoteAddr, sizeof(SOCKADDR_IN), NULL, NULL, NULL, NULL) != SOCKET_ERROR)
                    break;

                closesocket(socketHandle);
                socketHandle = INVALID_SOCKET;
            }
        }
        else if (i->wType == DNS_TYPE_AAAA)
        {
            SOCKADDR_IN6 remoteAddr;

            memset(&remoteAddr, 0, sizeof(SOCKADDR_IN6));
            remoteAddr.sin6_family = AF_INET6;
            remoteAddr.sin6_port = _byteswap_ushort(ServerPort);
            memcpy_s(
                remoteAddr.sin6_addr.s6_addr,
                sizeof(remoteAddr.sin6_addr.s6_addr),
                i->Data.AAAA.Ip6Address.IP6Byte,
                sizeof(i->Data.AAAA.Ip6Address.IP6Byte)
                );

            if ((socketHandle = WSASocket(AF_INET6, SOCK_STREAM, IPPROTO_TCP, NULL, 0, WSA_FLAG_OVERLAPPED | WSA_FLAG_NO_HANDLE_INHERIT)) != INVALID_SOCKET)
            {
                ULONG bestInterfaceIndex;

                if (GetBestInterfaceEx((PSOCKADDR)&remoteAddr, &bestInterfaceIndex) == ERROR_SUCCESS)
                {
                    MIB_IPFORWARD_ROW2 bestAddressRoute = { 0 };
                    SOCKADDR_INET destinationAddress = { 0 };
                    SOCKADDR_INET bestSourceAddress = { 0 };

                    destinationAddress.si_family = AF_INET6;
                    destinationAddress.Ipv6 = remoteAddr;

                    if (GetBestRoute2(
                        NULL,
                        bestInterfaceIndex,
                        NULL,
                        &destinationAddress,
                        0,
                        &bestAddressRoute,
                        &bestSourceAddress
                        ) == ERROR_SUCCESS)
                    {
                        bind(socketHandle, (PSOCKADDR)&bestSourceAddress.Ipv6, sizeof(bestSourceAddress.Ipv6));
                    }
                }

                if (WSAConnect(socketHandle, (PSOCKADDR)&remoteAddr, sizeof(SOCKADDR_IN6), NULL, NULL, NULL, NULL) != SOCKET_ERROR)
                    break;

                closesocket(socketHandle);
                socketHandle = INVALID_SOCKET;
            }
        }
    }

    if (socketHandle != INVALID_SOCKET)
    {
        *SocketHandle = socketHandle;
        return TRUE;
    }

    return FALSE;
}

_Success_(return)
BOOLEAN WhoisConnectServer(
    _In_ PWSTR WhoisServerAddress,
    _In_ USHORT WhoisServerPort,
    _In_ USHORT DnsQueryMessageType,
    _Out_ SOCKET* WhoisServerSocketHandle
    )
{
    SOCKET socketHandle;
    PDNS_RECORD dnsRecordList;

    if (PhGetIntegerSetting(L"EnableNetworkResolveDoH"))
    {
        dnsRecordList = PhDnsQuery(
            NULL,
            WhoisServerAddress,
            DnsQueryMessageType
            );
    }
    else
    {
        dnsRecordList = PhDnsQuery2(
            NULL,
            WhoisServerAddress,
            DnsQueryMessageType,
            DNS_QUERY_NO_HOSTS_FILE // DNS_QUERY_BYPASS_CACHE
            );
    }

    if (!dnsRecordList)
        return FALSE;

    if (WhoisDualStackSocketConnectByAddressList(dnsRecordList, WhoisServerPort, &socketHandle))
    {
        PhDnsFree(dnsRecordList);
        *WhoisServerSocketHandle = socketHandle;
        return TRUE;
    }

    if (WhoisConnectByAddressList(dnsRecordList, WhoisServerPort, &socketHandle))
    {
        PhDnsFree(dnsRecordList);
        *WhoisServerSocketHandle = socketHandle;
        return TRUE;
    }

    PhDnsFree(dnsRecordList);

    return FALSE;
}

_Success_(return)
BOOLEAN WhoisQueryServer(
    _In_ PWSTR WhoisServerAddress,
    _In_ USHORT WhoisServerPort,
    _In_ PWSTR WhoisQueryAddress,
    _Out_ PPH_STRING* WhoisQueryResponse
    )
{
    ULONG whoisResponseLength = 0;
    PSTR whoisResponse = NULL;
    PPH_BYTES whoisServerQuery = NULL;
    SOCKET whoisSocketHandle = INVALID_SOCKET;

    if (!WhoisServerAddress)
        return FALSE;
    if (WhoisServerPort == 0)
        return FALSE;

    if (!WhoisConnectServer(WhoisServerAddress, WhoisServerPort, DNS_TYPE_AAAA, &whoisSocketHandle))
    {
        if (!WhoisConnectServer(WhoisServerAddress, WhoisServerPort, DNS_TYPE_A, &whoisSocketHandle))
            return FALSE;
    }

    if (PhEqualStringZ(WhoisServerAddress, L"whois.arin.net", TRUE))
        whoisServerQuery = PhFormatBytes("n %S\r\n", WhoisQueryAddress);
    else
        whoisServerQuery = PhFormatBytes("%S\r\n", WhoisQueryAddress);

    if (!WriteSocketString(whoisSocketHandle, whoisServerQuery->Buffer, (ULONG)whoisServerQuery->Length))
    {
        PhDereferenceObject(whoisServerQuery);
        return FALSE;
    }

    if (!ReadSocketString(whoisSocketHandle, &whoisResponse, &whoisResponseLength))
    {
        PhDereferenceObject(whoisServerQuery);
        return FALSE;
    }

    PhDereferenceObject(whoisServerQuery);
    closesocket(whoisSocketHandle);

    if (whoisResponse)
    {
        if (WhoisQueryResponse)
            *WhoisQueryResponse = PhZeroExtendToUtf16Ex(whoisResponse, whoisResponseLength);

        PhFree(whoisResponse);
        return TRUE;
    }

    return FALSE;
}

NTSTATUS NetworkWhoisThreadStart(
    _In_ PVOID Parameter
    )
{
    PNETWORK_WHOIS_CONTEXT context = (PNETWORK_WHOIS_CONTEXT)Parameter;
    WSADATA winsockStartup;
    PH_STRING_BUILDER stringBuilder;
    PPH_STRING whoisResponse = NULL;
    PPH_STRING whoisReferralResponse = NULL;
    PPH_STRING whoisServerName = NULL;
    PPH_STRING whoisReferralServerName = NULL;
    USHORT whoisReferralServerPort = IPPORT_WHOIS;

    if (WSAStartup(WINSOCK_VERSION, &winsockStartup) != ERROR_SUCCESS)
    {
        PhDereferenceObject(context);
        return STATUS_FAIL_CHECK;
    }

    PhInitializeStringBuilder(&stringBuilder, 0x100);

    if (context->WindowHandle)
    {
        SendMessage(context->WindowHandle, NTM_RECEIVEDWHOIS, 0, (LPARAM)PhCreateString(L"Connecting to whois.iana.org..."));
    }

    if (!WhoisQueryServer(L"whois.iana.org", IPPORT_WHOIS, context->RemoteAddressString, &whoisResponse))
    {
        PhAppendFormatStringBuilder(&stringBuilder, L"Connection to whois.iana.org failed.\n");
        goto CleanupExit;
    }

    if (!WhoisExtractServerUrl(whoisResponse, &whoisServerName))
    {
        PhAppendFormatStringBuilder(&stringBuilder, L"Error parsing whois.iana.org response:\n%s\n", whoisResponse->Buffer);
        goto CleanupExit;
    }

    if (context->WindowHandle)
    {
        SendMessage(context->WindowHandle, NTM_RECEIVEDWHOIS, 0, (LPARAM)PhFormatString(L"Connecting to %s...", PhGetStringOrEmpty(whoisServerName)));
    }

    if (WhoisQueryServer(
        PhGetString(whoisServerName),
        IPPORT_WHOIS,
        context->RemoteAddressString,
        &whoisResponse
        ))
    {
        if (WhoisExtractReferralServer(
            whoisResponse,
            &whoisReferralServerName,
            &whoisReferralServerPort
            ))
        {
            if (context->WindowHandle)
            {
                SendMessage(context->WindowHandle, NTM_RECEIVEDWHOIS, 0, (LPARAM)PhFormatString(
                    L"%s referred the request to %s\n",
                    PhGetString(whoisServerName),
                    PhGetString(whoisReferralServerName)
                    ));
            }

            PhAppendFormatStringBuilder(
                &stringBuilder,
                L"%s referred the request to %s\n",
                PhGetString(whoisServerName),
                PhGetString(whoisReferralServerName)
                );

            if (WhoisQueryServer(
                PhGetString(whoisReferralServerName),
                whoisReferralServerPort,
                context->RemoteAddressString,
                &whoisReferralResponse
                ))
            {
                PhAppendFormatStringBuilder(&stringBuilder, L"\n%s\n", PhGetString(whoisReferralResponse));
                PhAppendFormatStringBuilder(&stringBuilder, L"\nOriginal request to %s:\n%s\n", PhGetString(whoisServerName), PhGetString(whoisResponse));
                goto CleanupExit;
            }
        }
    }
    else
    {
        PhAppendFormatStringBuilder(&stringBuilder, L"Connection to %s failed.\n", PhGetStringOrEmpty(whoisServerName));
        goto CleanupExit;
    }

    PhAppendFormatStringBuilder(&stringBuilder, L"\n%s", PhGetString(whoisResponse));

CleanupExit:

    if (context->WindowHandle)
        PostMessage(context->WindowHandle, NTM_RECEIVEDWHOIS, 0, (LPARAM)PhFinalStringBuilderString(&stringBuilder));
    else
        PhDeleteStringBuilder(&stringBuilder);

    WSACleanup();

    if (whoisResponse)
        PhDereferenceObject(whoisResponse);
    if (whoisReferralResponse)
        PhDereferenceObject(whoisReferralResponse);
    if (whoisServerName)
        PhDereferenceObject(whoisServerName);
    if (whoisReferralServerName)
        PhDereferenceObject(whoisReferralServerName);

    PhDereferenceObject(context);

    return STATUS_SUCCESS;
}

VOID WhoisSetTextFont(
    _In_ PNETWORK_WHOIS_CONTEXT Context
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
        if (Context->FontHandle = CreateFontIndirect(&font))
        {
            SetWindowFont(Context->RichEditHandle, Context->FontHandle, TRUE);
        }
    }
}

VOID WhoisParseAddressString(
    _In_ PNETWORK_WHOIS_CONTEXT Context
    )
{
    ULONG remoteAddressStringLength = RTL_NUMBER_OF(Context->RemoteAddressString);

    if (Context->RemoteEndpoint.Address.Type == PH_IPV4_NETWORK_TYPE)
    {
        if (NT_SUCCESS(RtlIpv4AddressToStringEx(
            &Context->RemoteEndpoint.Address.InAddr,
            0,
            Context->RemoteAddressString,
            &remoteAddressStringLength
            )))
        {
            Context->RemoteAddressStringLength = (remoteAddressStringLength - 1) * sizeof(WCHAR);
        }
    }
    else
    {
        if (NT_SUCCESS(RtlIpv6AddressToStringEx(
            &Context->RemoteEndpoint.Address.In6Addr,
            0,
            0,
            Context->RemoteAddressString,
            &remoteAddressStringLength
            )))
        {
            Context->RemoteAddressStringLength = (remoteAddressStringLength - 1) * sizeof(WCHAR);
        }
    }
}

INT_PTR CALLBACK WhoisDlgProc(
    _In_ HWND hwndDlg,
    _In_ UINT uMsg,
    _In_ WPARAM wParam,
    _In_ LPARAM lParam
    )
{
    PNETWORK_WHOIS_CONTEXT context;

    if (uMsg == WM_INITDIALOG)
    {
        context = (PNETWORK_WHOIS_CONTEXT)lParam;
        PhSetWindowContext(hwndDlg, PH_WINDOW_CONTEXT_DEFAULT, context);
    }
    else
    {
        context = PhGetWindowContext(hwndDlg, PH_WINDOW_CONTEXT_DEFAULT);

        if (uMsg == WM_NCDESTROY)
        {
            PhSaveWindowPlacementToSetting(SETTING_NAME_WHOIS_WINDOW_POSITION, SETTING_NAME_WHOIS_WINDOW_SIZE, hwndDlg);
            PhDeleteLayoutManager(&context->LayoutManager);
            PhRemoveWindowContext(hwndDlg, PH_WINDOW_CONTEXT_DEFAULT);

            if (context->FontHandle)
                DeleteFont(context->FontHandle);

            context->WindowHandle = NULL;
            PhDereferenceObject(context);
        }
    }

    if (!context)
        return FALSE;

    switch (uMsg)
    {
    case WM_INITDIALOG:
        {
            context->WindowHandle = hwndDlg;
            context->RichEditHandle = GetDlgItem(hwndDlg, IDC_NETOUTPUTEDIT);

            PhSetApplicationWindowIcon(hwndDlg);
            WhoisSetTextFont(context);
            WhoisParseAddressString(context);

            PhSetWindowText(hwndDlg, PhaFormatString(L"Whois %s...", context->RemoteAddressString)->Buffer);

            //SendMessage(context->RichEditHandle, EM_SETBKGNDCOLOR, RGB(0, 0, 0), 0);
            SendMessage(context->RichEditHandle, EM_SETEVENTMASK, 0, SendMessage(context->RichEditHandle, EM_GETEVENTMASK, 0, 0) | ENM_LINK);
            SendMessage(context->RichEditHandle, EM_AUTOURLDETECT, AURL_ENABLEURL, 0);
            SendMessage(context->RichEditHandle, EM_SETWORDWRAPMODE, WBF_WORDWRAP, 0);
            //context->FontHandle = PhCreateCommonFont(-11, FW_MEDIUM, context->RichEditHandle);
            SendMessage(context->RichEditHandle, EM_SETMARGINS, EC_LEFTMARGIN, MAKELONG(4, 0));
            SendMessage(context->RichEditHandle, EM_SETREADONLY, TRUE, 0);

            PhInitializeLayoutManager(&context->LayoutManager, hwndDlg);
            PhAddLayoutItem(&context->LayoutManager, context->RichEditHandle, NULL, PH_ANCHOR_ALL);

            if (PhGetIntegerPairSetting(SETTING_NAME_WHOIS_WINDOW_POSITION).X != 0)
                PhLoadWindowPlacementFromSetting(SETTING_NAME_WHOIS_WINDOW_POSITION, SETTING_NAME_WHOIS_WINDOW_SIZE, hwndDlg);
            else
                PhCenterWindow(hwndDlg, context->ParentWindowHandle);

            PhInitializeWindowTheme(hwndDlg, !!PhGetIntegerSetting(L"EnableThemeSupport"));

            PhReferenceObject(context);
            PhCreateThread2(NetworkWhoisThreadStart, (PVOID)context);
        }
        break;
    case WM_DESTROY:
        {
            PostQuitMessage(0);
        }
        break;
    case WM_COMMAND:
        {
            switch (GET_WM_COMMAND_ID(wParam, lParam))
            {
            case IDCANCEL:
                DestroyWindow(hwndDlg);
                break;
            }
        }
        break;
    case WM_SIZE:
        PhLayoutManagerLayout(&context->LayoutManager);
        break;
    case WM_NOTIFY:
        {
            LPNMHDR header = (LPNMHDR)lParam;

            switch (header->code)
            {
            case EN_LINK:
                {
                    ENLINK* link = (ENLINK*)lParam;

                    if (link->msg == WM_LBUTTONUP)
                    {
                        ULONG length;
                        PWSTR buffer;
                        TEXTRANGE textRange;

                        length = (link->chrg.cpMax - link->chrg.cpMin) * sizeof(WCHAR);
                        buffer = PhAllocateZero(length + sizeof(UNICODE_NULL));

                        textRange.chrg = link->chrg;
                        textRange.lpstrText = buffer;

                        if (SendMessage(context->RichEditHandle, EM_GETTEXTRANGE, 0, (LPARAM)&textRange))
                        {
                            if (PhCountStringZ(buffer) > 4)
                            {
                                PhShellExecute(context->WindowHandle, buffer, NULL);
                            }
                        }

                        PhFree(buffer);
                    }
                }
                break;
            }
        }
        break;
    case NTM_RECEIVEDWHOIS:
        {
            PPH_STRING whoisString = PH_AUTO((PPH_STRING)lParam);
            PPH_STRING trimString = PH_AUTO(TrimString2(whoisString));

            RichEditSetText(context->RichEditHandle, trimString->Buffer);
        }
        break;
    //case WM_TRACERT_UPDATE:
    //    {
    //        PPH_STRING windowText;
    //
    //        if (windowText = PhGetWindowText(context->WindowHandle))
    //        {
    //            PPH_STRING text;
    //
    //            text = PhConcatStrings2(windowText->Buffer, L" Finished.");
    //            PhSetWindowText(context->WindowHandle, text->Buffer);
    //
    //            PhDereferenceObject(text);
    //            PhDereferenceObject(windowText);
    //        }
    //    }
    //    break;
    case WM_CONTEXTMENU:
        {
            POINT point;
            PPH_EMENU menu;
            PPH_EMENU item;
            CHARRANGE range;

            point.x = GET_X_LPARAM(lParam);
            point.y = GET_Y_LPARAM(lParam);

            menu = PhCreateEMenu();
            PhInsertEMenuItem(menu, PhCreateEMenuItem(0, 10, L"&Copy", NULL, NULL), ULONG_MAX);
            PhInsertEMenuItem(menu, PhCreateEMenuSeparator(), ULONG_MAX);
            PhInsertEMenuItem(menu, PhCreateEMenuItem(0, 11, L"&Select all", NULL, NULL), ULONG_MAX);

            memset(&range, 0, sizeof(CHARRANGE));
            SendMessage(context->RichEditHandle, EM_EXGETSEL, 0, (LPARAM)&range);

            if (range.cpMin == range.cpMax)
            {
                if (item = PhFindEMenuItem(menu, PH_EMENU_FIND_DESCEND, NULL, 10))
                {
                    PhSetEnabledEMenuItem(item, FALSE);
                }
            }

            item = PhShowEMenu(
                menu,
                hwndDlg,
                PH_EMENU_SHOW_SEND_COMMAND | PH_EMENU_SHOW_LEFTRIGHT,
                PH_ALIGN_LEFT | PH_ALIGN_TOP,
                point.x,
                point.y
                );

            if (item)
            {
                switch (item->Id)
                {
                case 10:
                    SendMessage(context->RichEditHandle, WM_COPY, 0, 0);
                    break;
                case 11:
                    SendMessage(context->RichEditHandle, EM_SETSEL, 0, -1);
                    break;
                }
            }

            PhDestroyEMenu(menu);
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

NTSTATUS NetworkWhoisDialogThreadStart(
    _In_ PVOID Parameter
    )
{
    static PH_INITONCE initOnce = PH_INITONCE_INIT;
    BOOL result;
    MSG message;
    HWND windowHandle;
    PH_AUTO_POOL autoPool;

    if (PhBeginInitOnce(&initOnce))
    {
        PhLoadLibrary(L"msftedit.dll");
        PhEndInitOnce(&initOnce);
    }

    PhInitializeAutoPool(&autoPool);

    windowHandle = PhCreateDialog(
        PluginInstance->DllBase,
        MAKEINTRESOURCE(IDD_WHOIS),
        NULL,
        WhoisDlgProc,
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

VOID WhoisContextDeleteProcedure(
    _In_ PVOID Object,
    _In_ ULONG Flags
    )
{
    NOTHING;
}

PNETWORK_WHOIS_CONTEXT CreateWhoisContext(
    VOID
    )
{
    PNETWORK_WHOIS_CONTEXT context;

    if (PhBeginInitOnce(&WhoisContextTypeInitOnce))
    {
        WhoisContextType = PhCreateObjectType(L"WhoisContextObjectType", 0, WhoisContextDeleteProcedure);
        PhEndInitOnce(&WhoisContextTypeInitOnce);
    }

    context = PhCreateObject(sizeof(NETWORK_WHOIS_CONTEXT), WhoisContextType);
    memset(context, 0, sizeof(NETWORK_WHOIS_CONTEXT));

    return context;
}

VOID ShowWhoisWindow(
    _In_ HWND ParentWindowHandle,
    _In_ PPH_NETWORK_ITEM NetworkItem
    )
{
    PNETWORK_WHOIS_CONTEXT context;

    context = CreateWhoisContext();
    context->ParentWindowHandle = ParentWindowHandle;

    memcpy_s(
        &context->RemoteEndpoint,
        sizeof(context->RemoteEndpoint),
        &NetworkItem->RemoteEndpoint,
        sizeof(NetworkItem->RemoteEndpoint)
        );

    PhCreateThread2(NetworkWhoisDialogThreadStart, context);
}

VOID ShowWhoisWindowFromAddress(
    _In_ HWND ParentWindowHandle,
    _In_ PH_IP_ENDPOINT RemoteEndpoint
    )
{
    PNETWORK_WHOIS_CONTEXT context;

    context = CreateWhoisContext();
    context->ParentWindowHandle = ParentWindowHandle;

    memcpy_s(
        &context->RemoteEndpoint,
        sizeof(context->RemoteEndpoint),
        &RemoteEndpoint,
        sizeof(RemoteEndpoint)
        );

    PhCreateThread2(NetworkWhoisDialogThreadStart, context);
}
