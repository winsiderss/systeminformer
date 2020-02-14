/*
 * Process Hacker Network Tools -
 *   Whois dialog
 *
 * Copyright (C) 2013-2019 dmex
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
#include <commonutil.h>
#include <richedit.h>

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
    ULONG allocatedLength;
    ULONG dataLength;
    ULONG returnLength;
    BYTE buffer[PAGE_SIZE];

    allocatedLength = sizeof(buffer);
    data = (PSTR)PhAllocate(allocatedLength);
    dataLength = 0;

    while ((returnLength = recv(Handle, buffer, PAGE_SIZE, 0)) != SOCKET_ERROR)
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
BOOLEAN WhoisExtractServerUrl(
    _In_ PPH_STRING WhoisResponce,
    _Out_opt_ PPH_STRING *WhoisServerAddress
    )
{
    ULONG_PTR whoisServerHostnameIndex;
    ULONG_PTR whoisServerHostnameLength;
    PPH_STRING whoisServerName;

    if ((whoisServerHostnameIndex = PhFindStringInString(WhoisResponce, 0, L"whois:")) == -1)
        return FALSE;
    if ((whoisServerHostnameLength = PhFindStringInString(WhoisResponce, whoisServerHostnameIndex, L"\n")) == -1)
        return FALSE;
    if ((whoisServerHostnameLength = whoisServerHostnameLength - whoisServerHostnameIndex) == 0)
        return FALSE;
 
    whoisServerName = PhSubstring(
        WhoisResponce,
        whoisServerHostnameIndex + wcslen(L"whois:"),
        (ULONG)whoisServerHostnameLength - wcslen(L"whois:")
        );

    if (WhoisServerAddress)
        *WhoisServerAddress = TrimString(whoisServerName);

    PhDereferenceObject(whoisServerName);
    return TRUE;
}

_Success_(return)
BOOLEAN WhoisExtractReferralServer(
    _In_ PPH_STRING WhoisResponce,
    _Out_opt_ PPH_STRING *WhoisServerAddress,
    _Out_opt_ PPH_STRING *WhoisServerPort
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

    if ((whoisServerHostnameIndex = PhFindStringInString(WhoisResponce, 0, L"ReferralServer:")) == -1)
        return FALSE;
    if ((whoisServerHostnameLength = PhFindStringInString(WhoisResponce, whoisServerHostnameIndex, L"\n")) == -1)
        return FALSE;
    if ((whoisServerHostnameLength = whoisServerHostnameLength - whoisServerHostnameIndex) == 0)
        return FALSE;

    whoisServerName = PhSubstring(
        WhoisResponce,
        whoisServerHostnameIndex + wcslen(L"ReferralServer:"),
        (ULONG)whoisServerHostnameLength - wcslen(L"ReferralServer:")
        );

    whoisServerHostname = TrimString(whoisServerName);
    
    if (swscanf_s(
        whoisServerHostname->Buffer,
        L"%[^:]://%[^:]:%[^/]/%s",
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

        if (PhCountStringZ(urlPort) >= 2)
        {
            if (WhoisServerPort)
            {
                *WhoisServerPort = PhCreateString(urlPort);
            }
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
BOOLEAN WhoisQueryServer(
    _In_ PWSTR WhoisServerAddress,
    _In_ PWSTR WhoisServerPort,
    _In_ PWSTR WhoisQueryAddress, 
    _Out_ PPH_STRING* WhoisQueryResponse
    )
{
    PADDRINFOW result = NULL;
    PADDRINFOW addrInfo = NULL;
    ADDRINFOW hints;
    ULONG whoisResponceLength = 0;
    PSTR whoisResponce = NULL;
    PPH_BYTES whoisQuery = NULL;

    memset(&hints, 0, sizeof(ADDRINFOW));
    hints.ai_family = AF_UNSPEC;
    hints.ai_socktype = SOCK_STREAM;
    hints.ai_protocol = IPPROTO_TCP;

    if (GetAddrInfo(WhoisServerAddress, WhoisServerPort ? WhoisServerPort : L"43", &hints, &result))
        return FALSE;

    if (PhEqualStringZ(WhoisServerAddress, L"whois.arin.net", TRUE))
        whoisQuery = FormatAnsiString("n %S\r\n", WhoisQueryAddress);
    else
        whoisQuery = FormatAnsiString("%S\r\n", WhoisQueryAddress);

    for (addrInfo = result; addrInfo; addrInfo = addrInfo->ai_next)
    {
        SOCKET socketHandle = socket(
            addrInfo->ai_family, 
            addrInfo->ai_socktype, 
            addrInfo->ai_protocol
            );

        if (socketHandle == INVALID_SOCKET)
            continue;

        if (connect(socketHandle, addrInfo->ai_addr, (INT)addrInfo->ai_addrlen) == SOCKET_ERROR)
        {
            closesocket(socketHandle);
            continue;
        }

        if (send(socketHandle, whoisQuery->Buffer, (INT)whoisQuery->Length, 0) == SOCKET_ERROR)
        {
            closesocket(socketHandle);
            continue;
        }

        if (ReadSocketString(socketHandle, &whoisResponce, &whoisResponceLength))
        {
            closesocket(socketHandle);
            break;
        }
    }

    FreeAddrInfo(result);

    PhDereferenceObject(whoisQuery);

    if (whoisResponce)
    {
        if (WhoisQueryResponse)
            *WhoisQueryResponse = PhConvertUtf8ToUtf16(whoisResponce);
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
    PH_STRING_BUILDER sb;
    PPH_STRING whoisResponse = NULL;
    PPH_STRING whoisReferralResponse = NULL;
    PPH_STRING whoisServerName = NULL;
    PPH_STRING whoisReferralServerName = NULL;
    PPH_STRING whoisReferralServerPort = NULL;

    if (WSAStartup(WINSOCK_VERSION, &winsockStartup) != ERROR_SUCCESS)
        return STATUS_FAIL_CHECK;

    PhInitializeStringBuilder(&sb, 0x100);

    SendMessage(context->WindowHandle, NTM_RECEIVEDWHOIS, 0, (LPARAM)PhCreateString(L"Connecting to whois.iana.org..."));

    if (!WhoisQueryServer(L"whois.iana.org", L"43", context->IpAddressString, &whoisResponse))
    {
        PhAppendFormatStringBuilder(&sb, L"Connection to whois.iana.org failed.\n");
        goto CleanupExit;
    }

    if (!WhoisExtractServerUrl(whoisResponse, &whoisServerName))
    {
        PhAppendFormatStringBuilder(&sb, L"Error parsing whois.iana.org response:\n%s\n", whoisResponse->Buffer);
        goto CleanupExit;
    }

    SendMessage(context->WindowHandle, NTM_RECEIVEDWHOIS, 0, (LPARAM)PhFormatString(L"Connecting to %s...", PhGetStringOrEmpty(whoisServerName)));

    if (WhoisQueryServer(
        PhGetString(whoisServerName),
        L"43",
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
            SendMessage(context->WindowHandle, NTM_RECEIVEDWHOIS, 0, (LPARAM)PhFormatString(
                L"%s referred the request to %s\n",
                PhGetString(whoisServerName),
                PhGetString(whoisReferralServerName)
                ));

            PhAppendFormatStringBuilder(
                &sb, 
                L"%s referred the request to %s\n", 
                PhGetString(whoisServerName), 
                PhGetString(whoisReferralServerName)
                );

            if (WhoisQueryServer(
                PhGetString(whoisReferralServerName),
                PhGetString(whoisReferralServerPort),
                context->IpAddressString,
                &whoisReferralResponse
                ))
            {
                PhAppendFormatStringBuilder(&sb, L"\n%s\n", PhGetString(whoisReferralResponse));
                PhAppendFormatStringBuilder(&sb, L"\nOriginal request to %s:\n%s\n", PhGetString(whoisServerName), PhGetString(whoisResponse));

                PostMessage(context->WindowHandle, NTM_RECEIVEDWHOIS, 0, (LPARAM)PhFinalStringBuilderString(&sb));

                goto CleanupExit;
            }
        }
    }

    PhAppendFormatStringBuilder(&sb, L"\n%s", PhGetString(whoisResponse));

    PostMessage(context->WindowHandle, NTM_RECEIVEDWHOIS, 0, (LPARAM)PhFinalStringBuilderString(&sb));

CleanupExit:
    WSACleanup();
    PhClearReference(&whoisResponse);
    PhClearReference(&whoisReferralResponse);
    PhClearReference(&whoisServerName);
    PhClearReference(&whoisReferralServerName);
    PhClearReference(&whoisReferralServerPort);

    return STATUS_SUCCESS;
}

INT_PTR CALLBACK NetworkOutputDlgProc(
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

            SendMessage(hwndDlg, WM_SETICON, ICON_SMALL, (LPARAM)PH_LOAD_SHARED_ICON_SMALL(PhInstanceHandle, MAKEINTRESOURCE(PHAPP_IDI_PROCESSHACKER)));
            SendMessage(hwndDlg, WM_SETICON, ICON_BIG, (LPARAM)PH_LOAD_SHARED_ICON_LARGE(PhInstanceHandle, MAKEINTRESOURCE(PHAPP_IDI_PROCESSHACKER)));

            if (context->RemoteEndpoint.Address.Type == PH_IPV4_NETWORK_TYPE)
            {
                RtlIpv4AddressToString(&context->RemoteEndpoint.Address.InAddr, context->IpAddressString);
            }
            else if (context->RemoteEndpoint.Address.Type == PH_IPV6_NETWORK_TYPE)
            {
                RtlIpv6AddressToString(&context->RemoteEndpoint.Address.In6Addr, context->IpAddressString);
            }

            SetWindowText(hwndDlg, PhaFormatString(L"Whois %s...", context->IpAddressString)->Buffer);

            //SendMessage(context->RichEditHandle, EM_SETBKGNDCOLOR, RGB(0, 0, 0), 0);
            SendMessage(context->RichEditHandle, EM_SETEVENTMASK, 0, SendMessage(context->RichEditHandle, EM_GETEVENTMASK, 0, 0) | ENM_LINK);
            SendMessage(context->RichEditHandle, EM_AUTOURLDETECT, AURL_ENABLEURL, 0);
            SendMessage(context->RichEditHandle, EM_SETWORDWRAPMODE, WBF_WORDWRAP, 0);
            context->FontHandle = PhCreateCommonFont(-11, FW_MEDIUM, context->RichEditHandle);

            PhInitializeLayoutManager(&context->LayoutManager, hwndDlg);
            PhAddLayoutItem(&context->LayoutManager, context->RichEditHandle, NULL, PH_ANCHOR_ALL);

            if (PhGetIntegerPairSetting(SETTING_NAME_WHOIS_WINDOW_POSITION).X != 0)
                PhLoadWindowPlacementFromSetting(SETTING_NAME_WHOIS_WINDOW_POSITION, SETTING_NAME_WHOIS_WINDOW_SIZE, hwndDlg);
            else
                PhCenterWindow(hwndDlg, PhMainWndHandle);

            PhInitializeWindowTheme(hwndDlg, !!PhGetIntegerSetting(L"EnableThemeSupport"));

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
    case WM_TRACERT_UPDATE:
        {
            PPH_STRING windowText;

            if (windowText = PhGetWindowText(context->WindowHandle))
            {
                PPH_STRING text;

                text = PhConcatStrings2(windowText->Buffer, L" Finished.");
                Static_SetText(context->WindowHandle, text->Buffer);

                PhDereferenceObject(text);
                PhDereferenceObject(windowText);

            }
        }
        break;
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
        LoadLibrary(L"msftedit.dll");
        PhEndInitOnce(&initOnce);
    }

    PhInitializeAutoPool(&autoPool);

    windowHandle = CreateDialogParam(
        PluginInstance->DllBase,
        MAKEINTRESOURCE(IDD_WHOIS),
        NULL,
        NetworkOutputDlgProc,
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


VOID ShowWhoisWindow(
    _In_ PPH_NETWORK_ITEM NetworkItem
    )
{
    PNETWORK_WHOIS_CONTEXT context;

    context = (PNETWORK_WHOIS_CONTEXT)PhCreateAlloc(sizeof(NETWORK_WHOIS_CONTEXT));
    memset(context, 0, sizeof(NETWORK_WHOIS_CONTEXT));

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
    PNETWORK_WHOIS_CONTEXT context;

    context = (PNETWORK_WHOIS_CONTEXT)PhCreateAlloc(sizeof(NETWORK_WHOIS_CONTEXT));
    memset(context, 0, sizeof(NETWORK_WHOIS_CONTEXT));

    RtlCopyMemory(
        &context->RemoteEndpoint,
        &RemoteEndpoint,
        sizeof(PH_IP_ENDPOINT)
        );

    PhCreateThread2(NetworkWhoisDialogThreadStart, (PVOID)context);
}
