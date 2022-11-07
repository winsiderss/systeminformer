/*
 * Copyright (c) 2022 Winsider Seminars & Solutions, Inc.  All rights reserved.
 *
 * This file is part of System Informer.
 *
 * Authors:
 *
 *     dmex    2013-2022
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
    if (!!PhGetIntegerSetting(L"EnableThemeSupport"))
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
    PH_STRINGREF sr = String->sr;
    PhTrimStringRef(&sr, &whitespace, 0);
    return PhCreateString2(&sr);
}

PPH_STRING TrimString2(
    _In_ PPH_STRING String
    )
{
    static PH_STRINGREF whitespace = PH_STRINGREF_INIT(L"\n\n");
    PH_STRINGREF sr = String->sr;
    PhTrimStringRef(&sr, &whitespace, 0);
    return PhCreateString2(&sr);
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
        whoisServerHostnameIndex + wcslen(L"whois:"),
        whoisServerHostnameLength - wcslen(L"whois:")
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
        whoisServerHostnameIndex + wcslen(L"ReferralServer:"),
        whoisServerHostnameLength - wcslen(L"ReferralServer:")
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

_Success_(return)
BOOLEAN WhoisConnectServer(
    _In_ PWSTR WhoisServerAddress,
    _In_ USHORT WhoisServerPort,
    _In_ USHORT DnsQueryMessageType,
    _Out_ SOCKET* WhoisServerSocketHandle
    )
{
    SOCKET whoisSocketHandle = INVALID_SOCKET;
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

    for (PDNS_RECORD dnsRecord = dnsRecordList; dnsRecord; dnsRecord = dnsRecord->pNext)
    {
        if (dnsRecord->wType == DNS_TYPE_A)
        {
            SOCKADDR_IN remoteAddr;

            memset(&remoteAddr, 0, sizeof(SOCKADDR_IN));
            remoteAddr.sin_family = AF_INET;
            remoteAddr.sin_port = _byteswap_ushort(WhoisServerPort);
            memcpy_s(
                &remoteAddr.sin_addr.s_addr,
                sizeof(remoteAddr.sin_addr.s_addr),
                &dnsRecord->Data.A.IpAddress,
                sizeof(dnsRecord->Data.A.IpAddress)
                );

            if ((whoisSocketHandle = WSASocket(AF_INET, SOCK_STREAM, IPPROTO_TCP, NULL, 0, WSA_FLAG_OVERLAPPED | WSA_FLAG_NO_HANDLE_INHERIT)) != INVALID_SOCKET)
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
                        bind(whoisSocketHandle, (PSOCKADDR)&bestSourceAddress.Ipv4, sizeof(bestSourceAddress.Ipv4));
                    }
                }

                if (WSAConnect(whoisSocketHandle, (PSOCKADDR)&remoteAddr, sizeof(SOCKADDR_IN), NULL, NULL, NULL, NULL) != SOCKET_ERROR)
                    break;

                closesocket(whoisSocketHandle);
                whoisSocketHandle = INVALID_SOCKET;
            }
        }
        else if (dnsRecord->wType == DNS_TYPE_AAAA)
        {
            SOCKADDR_IN6 remoteAddr;

            memset(&remoteAddr, 0, sizeof(SOCKADDR_IN6));
            remoteAddr.sin6_family = AF_INET6;
            remoteAddr.sin6_port = _byteswap_ushort(WhoisServerPort);
            memcpy_s(
                remoteAddr.sin6_addr.s6_addr,
                sizeof(remoteAddr.sin6_addr.s6_addr),
                dnsRecord->Data.AAAA.Ip6Address.IP6Byte,
                sizeof(dnsRecord->Data.AAAA.Ip6Address.IP6Byte)
                );

            if ((whoisSocketHandle = WSASocket(AF_INET6, SOCK_STREAM, IPPROTO_TCP, NULL, 0, WSA_FLAG_OVERLAPPED | WSA_FLAG_NO_HANDLE_INHERIT)) != INVALID_SOCKET)
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
                        bind(whoisSocketHandle, (PSOCKADDR)&bestSourceAddress.Ipv6, sizeof(bestSourceAddress.Ipv6));
                    }
                }

                if (WSAConnect(whoisSocketHandle, (PSOCKADDR)&remoteAddr, sizeof(SOCKADDR_IN6), NULL, NULL, NULL, NULL) != SOCKET_ERROR)
                    break;

                closesocket(whoisSocketHandle);
                whoisSocketHandle = INVALID_SOCKET;
            }
        }
    }

    PhDnsFree(dnsRecordList);

    if (whoisSocketHandle != INVALID_SOCKET)
    {
        *WhoisServerSocketHandle = whoisSocketHandle;
        return TRUE;
    }

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

    if (!WhoisQueryServer(L"whois.iana.org", IPPORT_WHOIS, context->IpAddressString, &whoisResponse))
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
        context->IpAddressString,
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
                context->IpAddressString,
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

        if (uMsg == WM_DESTROY)
        {
            PhSaveWindowPlacementToSetting(SETTING_NAME_WHOIS_WINDOW_POSITION, SETTING_NAME_WHOIS_WINDOW_SIZE, hwndDlg);
            PhDeleteLayoutManager(&context->LayoutManager);
            PhRemoveWindowContext(hwndDlg, PH_WINDOW_CONTEXT_DEFAULT);

            if (context->FontHandle)
                DeleteFont(context->FontHandle);

            context->WindowHandle = NULL;
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
            context->WindowHandle = hwndDlg;
            context->RichEditHandle = GetDlgItem(hwndDlg, IDC_NETOUTPUTEDIT);

            PhSetApplicationWindowIcon(hwndDlg);
            WhoisSetTextFont(context);

            if (context->RemoteEndpoint.Address.Type == PH_IPV4_NETWORK_TYPE)
            {
                RtlIpv4AddressToString(&context->RemoteEndpoint.Address.InAddr, context->IpAddressString);
            }
            else if (context->RemoteEndpoint.Address.Type == PH_IPV6_NETWORK_TYPE)
            {
                RtlIpv6AddressToString(&context->RemoteEndpoint.Address.In6Addr, context->IpAddressString);
            }

            PhSetWindowText(hwndDlg, PhaFormatString(L"Whois %s...", context->IpAddressString)->Buffer);

            //SendMessage(context->RichEditHandle, EM_SETBKGNDCOLOR, RGB(0, 0, 0), 0);
            SendMessage(context->RichEditHandle, EM_SETEVENTMASK, 0, SendMessage(context->RichEditHandle, EM_GETEVENTMASK, 0, 0) | ENM_LINK);
            SendMessage(context->RichEditHandle, EM_AUTOURLDETECT, AURL_ENABLEURL, 0);
            SendMessage(context->RichEditHandle, EM_SETWORDWRAPMODE, WBF_WORDWRAP, 0);
            //context->FontHandle = PhCreateCommonFont(-11, FW_MEDIUM, context->RichEditHandle);
            SendMessage(context->RichEditHandle, EM_SETMARGINS, EC_LEFTMARGIN, MAKELONG(4, 0));

            PhInitializeLayoutManager(&context->LayoutManager, hwndDlg);
            PhAddLayoutItem(&context->LayoutManager, context->RichEditHandle, NULL, PH_ANCHOR_ALL);

            if (PhGetIntegerPairSetting(SETTING_NAME_WHOIS_WINDOW_POSITION).X != 0)
                PhLoadWindowPlacementFromSetting(SETTING_NAME_WHOIS_WINDOW_POSITION, SETTING_NAME_WHOIS_WINDOW_SIZE, hwndDlg);
            else
                PhCenterWindow(hwndDlg, PhMainWndHandle);

            PhInitializeWindowTheme(hwndDlg, !!PhGetIntegerSetting(L"EnableThemeSupport"));

            PhReferenceObject(context);
            PhCreateThread2(NetworkWhoisThreadStart, (PVOID)context);
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
    _In_ PPH_NETWORK_ITEM NetworkItem
    )
{
    PNETWORK_WHOIS_CONTEXT context = CreateWhoisContext();

    RtlCopyMemory(
        &context->RemoteEndpoint,
        &NetworkItem->RemoteEndpoint,
        sizeof(PH_IP_ENDPOINT)
        );

    PhCreateThread2(NetworkWhoisDialogThreadStart, (PVOID)context);
}

VOID ShowWhoisWindowFromAddress(
    _In_ PH_IP_ENDPOINT RemoteEndpoint
    )
{
    PNETWORK_WHOIS_CONTEXT context = CreateWhoisContext();

    RtlCopyMemory(
        &context->RemoteEndpoint,
        &RemoteEndpoint,
        sizeof(PH_IP_ENDPOINT)
        );

    PhCreateThread2(NetworkWhoisDialogThreadStart, (PVOID)context);
}
