/*
 * Process Hacker Network Tools -
 *   Whois dialog
 *
 * Copyright (C) 2013-2016 dmex
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

VOID RichEditAppendText(
    _In_ HWND RichEditHandle,
    _In_ PWSTR Text
)
{
    //CHARFORMAT2 cf;
    //memset(&cf, 0, sizeof(CHARFORMAT2));
    //cf.cbSize = sizeof(CHARFORMAT2);
    //cf.dwMask = CFM_COLOR | CFM_EFFECTS;
    //cf.dwEffects = CFE_BOLD;
    //cf.crTextColor = RGB(0xff, 0xff, 0xff);
    //cf.crBackColor = RGB(0, 0, 0);

    //SendMessage(RichEditHandle, EM_SETCHARFORMAT, SCF_SELECTION, (LPARAM)&cf);
    SendMessage(RichEditHandle, EM_REPLACESEL, FALSE, (LPARAM)Text);
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

BOOLEAN ReadSocketString(
    _In_ SOCKET Handle,
    _Out_ _Deref_post_z_cap_(*DataLength) PSTR *Data,
    _Out_ ULONG *DataLength
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

    // Zero the buffer
    memset(buffer, 0, PAGE_SIZE);

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

    // Ensure that the buffer is null-terminated.
    data[dataLength] = 0;

    *DataLength = dataLength;
    *Data = data;

    return TRUE;
}

BOOLEAN WhoisExtractServerUrl(
    _In_ PPH_STRING WhoisResponce,
    _Out_ PPH_STRING *WhoisServerAddress
    )
{
    ULONG_PTR whoisServerHostnameIndex;
    ULONG_PTR whoisServerHostnameLength;
    PPH_STRING whoisServerName;

    if ((whoisServerHostnameIndex = PhFindStringInString(WhoisResponce, 0, L"whois:")) == -1)
        return FALSE;
    if ((whoisServerHostnameLength = PhFindStringInString(WhoisResponce, whoisServerHostnameIndex, L"\n") - whoisServerHostnameIndex) == -1)
        return FALSE;

    whoisServerName = PhSubstring(
        WhoisResponce,
        whoisServerHostnameIndex + PhCountStringZ(L"whois:"),
        (ULONG)whoisServerHostnameLength - PhCountStringZ(L"whois:")
        );

    *WhoisServerAddress = TrimString(whoisServerName);

    PhDereferenceObject(whoisServerName);
    return TRUE;
}

BOOLEAN WhoisExtractReferralServer(
    _In_ PPH_STRING WhoisResponce,
    _Out_ PPH_STRING *WhoisServerAddress,
    _Out_ PPH_STRING *WhoisServerPort
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
    if ((whoisServerHostnameLength = PhFindStringInString(WhoisResponce, whoisServerHostnameIndex, L"\n") - whoisServerHostnameIndex) == -1)
        return FALSE;

    whoisServerName = PhSubstring(
        WhoisResponce,
        whoisServerHostnameIndex + PhCountStringZ(L"ReferralServer:"),
        (ULONG)whoisServerHostnameLength - PhCountStringZ(L"ReferralServer:")
        );

    whoisServerHostname = TrimString(whoisServerName);
    
    if (swscanf(
        whoisServerHostname->Buffer,
        L"%[^:]://%[^:]:%[^/]/%s",
        urlProtocal,
        urlHost,
        urlPort,
        urlPath
        ))
    {
        *WhoisServerAddress = PhCreateString(urlHost);
        *WhoisServerPort = PhCreateString(urlPort);

        PhDereferenceObject(whoisServerName);
        PhDereferenceObject(whoisServerHostname);
        return TRUE;
    }

    PhDereferenceObject(whoisServerName);
    PhDereferenceObject(whoisServerHostname);
    return FALSE;
}

BOOLEAN WhoisQueryServer(
    _In_ PWSTR WhoisServerAddress,
    _In_ PWSTR WhoisServerPort,
    _In_ PWSTR QueryString, 
    _In_ PPH_STRING* response
    )
{
    WSADATA winsockStartup;
    PADDRINFOW result = NULL;
    PADDRINFOW ptr = NULL;
    ADDRINFOW hints;
    ULONG whoisResponceLength = 0;
    PSTR whoisResponce = NULL;
    CHAR whoisQuery[PAGE_SIZE] = "";

    if (PhEqualStringZ(WhoisServerAddress, L"whois.arin.net", TRUE))
    {
        _snprintf_s(whoisQuery, sizeof(whoisQuery), _TRUNCATE, "n %S\r\n", QueryString);
    }
    else
    {
        _snprintf_s(whoisQuery, sizeof(whoisQuery), _TRUNCATE, "%S\r\n", QueryString);
    }

    if (WSAStartup(WINSOCK_VERSION, &winsockStartup) != ERROR_SUCCESS)
        return FALSE;

    memset(&hints, 0, sizeof(ADDRINFOW));
    hints.ai_family = AF_UNSPEC;
    hints.ai_socktype = SOCK_STREAM;
    hints.ai_protocol = IPPROTO_TCP;

    if (GetAddrInfo(WhoisServerAddress, WhoisServerPort, &hints, &result))
    {
        WSACleanup();
        return FALSE;
    }

    for (ptr = result; ptr; ptr = ptr->ai_next)
    {
        SOCKET socketHandle = socket(ptr->ai_family, ptr->ai_socktype, ptr->ai_protocol);

        if (socketHandle == INVALID_SOCKET)
            continue;

        if (connect(socketHandle, ptr->ai_addr, (INT)ptr->ai_addrlen) == SOCKET_ERROR)
        {
            closesocket(socketHandle);
            continue;
        }

        if (send(socketHandle, whoisQuery, (INT)strlen(whoisQuery), 0) == SOCKET_ERROR)
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
    WSACleanup();

    if (whoisResponce)
    {
        *response = PhConvertUtf8ToUtf16(whoisResponce);
        return TRUE;
    }

    return FALSE;
}

BOOLEAN WhoisQueryLookup(
    _In_ PWSTR IpAddress, 
    _Out_ PPH_STRING *WhoisQueryResult
    )
{
    BOOLEAN success = FALSE;
    PPH_STRING whoisResponse = NULL;
    PPH_STRING whoisReferralResponse = NULL;
    PPH_STRING whoisServerName = NULL;
    PPH_STRING whoisReferralServerName = NULL;
    PPH_STRING whoisReferralServerPort = NULL;
    PH_STRING_BUILDER sb;

    PhInitializeStringBuilder(&sb, 0x100);

    if (!WhoisQueryServer(L"whois.iana.org", L"43", IpAddress, &whoisResponse))
    {
        PhAppendFormatStringBuilder(&sb, L"Connection to whois.iana.org failed.\n");
        goto CleanupExit;
    }

    if (!WhoisExtractServerUrl(whoisResponse, &whoisServerName))
    {
        PhAppendFormatStringBuilder(&sb, L"Error parsing whois.iana.org response:\n%s\n", whoisResponse->Buffer);
        goto CleanupExit;
    }

    PhAppendFormatStringBuilder(&sb, L"%s\n", whoisServerName->Buffer);

    if (WhoisQueryServer(
        whoisServerName->Buffer,
        L"43",
        IpAddress,
        &whoisResponse
        ))
    {
        if (WhoisExtractReferralServer(
            whoisResponse,
            &whoisReferralServerName,
            &whoisReferralServerPort
            ))
        {
            PhAppendFormatStringBuilder(&sb, L"%s referred the request to: %s\n", whoisServerName->Buffer, whoisReferralServerName->Buffer);

            if (WhoisQueryServer(
                whoisReferralServerName->Buffer,
                whoisReferralServerPort->Buffer,
                IpAddress,
                &whoisReferralResponse
                ))
            {
                PhAppendFormatStringBuilder(&sb, L"\n%s\n", whoisReferralResponse->Buffer);
                PhAppendFormatStringBuilder(&sb, L"\nOriginal request to %s:\n%s\n", whoisServerName->Buffer, whoisResponse->Buffer);
                success = TRUE;
                goto CleanupExit;
            }
        }
    }

    PhAppendFormatStringBuilder(&sb, L"\n%s", whoisResponse->Buffer);

    success = TRUE;

CleanupExit:
    PhClearReference(&whoisResponse);
    PhClearReference(&whoisReferralResponse);
    PhClearReference(&whoisServerName);
    PhClearReference(&whoisReferralServerName);
    PhClearReference(&whoisReferralServerPort);

    *WhoisQueryResult = PhFinalStringBuilderString(&sb);

    return success;
}

NTSTATUS NetworkWhoisThreadStart(
    _In_ PVOID Parameter
    )
{
    PNETWORK_OUTPUT_CONTEXT context = NULL;
    PPH_STRING whoisReply = NULL;

    context = (PNETWORK_OUTPUT_CONTEXT)Parameter;

    WhoisQueryLookup(context->IpAddressString, &whoisReply);

    PostMessage(context->WindowHandle, NTM_RECEIVEDWHOIS, 0, (LPARAM)whoisReply);
    PostMessage(context->WindowHandle, NTM_RECEIVEDFINISH, 0, 0);

    return STATUS_SUCCESS;
}

INT_PTR CALLBACK NetworkOutputDlgProc(
    _In_ HWND hwndDlg,
    _In_ UINT uMsg,
    _In_ WPARAM wParam,
    _In_ LPARAM lParam
    )
{
    PNETWORK_OUTPUT_CONTEXT context;

    if (uMsg == WM_INITDIALOG)
    {
        context = (PNETWORK_OUTPUT_CONTEXT)lParam;
        SetProp(hwndDlg, L"Context", (HANDLE)context);
    }
    else
    {
        context = (PNETWORK_OUTPUT_CONTEXT)GetProp(hwndDlg, L"Context");

        if (uMsg == WM_DESTROY)
        {
            PhSaveWindowPlacementToSetting(SETTING_NAME_OUTPUT_WINDOW_POSITION, SETTING_NAME_OUTPUT_WINDOW_SIZE, hwndDlg);
            PhDeleteLayoutManager(&context->LayoutManager);
            RemoveProp(hwndDlg, L"Context");
            PhFree(context);

            PostQuitMessage(0);
        }
    }

    if (!context)
        return FALSE;

    switch (uMsg)
    {
    case WM_INITDIALOG:
        {
            PH_RECTANGLE windowRectangle;
            HANDLE dialogThread;

            context->WindowHandle = hwndDlg;
            context->WhoisHandle = GetDlgItem(hwndDlg, IDC_NETOUTPUTEDIT);

            //SendMessage(context->OutputHandle, EM_SETBKGNDCOLOR, RGB(0, 0, 0), 0);
            SendMessage(context->WhoisHandle, EM_SETEVENTMASK, 0, SendMessage(context->WhoisHandle, EM_GETEVENTMASK, 0, 0) | ENM_LINK);
            SendMessage(context->WhoisHandle, EM_AUTOURLDETECT, AURL_ENABLEURL, 0);
            SendMessage(context->WhoisHandle, EM_SETWORDWRAPMODE, WBF_WORDWRAP, 0);

            PhInitializeLayoutManager(&context->LayoutManager, hwndDlg);
            PhAddLayoutItem(&context->LayoutManager, context->WhoisHandle, NULL, PH_ANCHOR_ALL);
            PhAddLayoutItem(&context->LayoutManager, GetDlgItem(hwndDlg, IDCANCEL), NULL, PH_ANCHOR_BOTTOM | PH_ANCHOR_RIGHT);

            windowRectangle.Position = PhGetIntegerPairSetting(SETTING_NAME_OUTPUT_WINDOW_POSITION);
            windowRectangle.Size = PhGetScalableIntegerPairSetting(SETTING_NAME_OUTPUT_WINDOW_SIZE, TRUE).Pair;
            if (windowRectangle.Position.X != 0 || windowRectangle.Position.Y != 0)
                PhLoadWindowPlacementFromSetting(SETTING_NAME_OUTPUT_WINDOW_POSITION, SETTING_NAME_OUTPUT_WINDOW_SIZE, hwndDlg);
            else
                PhCenterWindow(hwndDlg, GetParent(hwndDlg));

            if (context->RemoteEndpoint.Address.Type == PH_IPV4_NETWORK_TYPE)
                RtlIpv4AddressToString(&context->RemoteEndpoint.Address.InAddr, context->IpAddressString);
            else
                RtlIpv6AddressToString(&context->RemoteEndpoint.Address.In6Addr, context->IpAddressString);

            SetWindowText(context->WindowHandle, PhaFormatString(L"Whois %s...", context->IpAddressString)->Buffer);
            RichEditAppendText(context->WhoisHandle, L"whois.iana.org found the following authoritative answer from: ");

            if (dialogThread = PhCreateThread(0, NetworkWhoisThreadStart, (PVOID)context))
                NtClose(dialogThread);
        }
        break;
    case WM_COMMAND:
        {
            switch (GET_WM_COMMAND_ID(wParam, lParam))
            {
            case IDCANCEL:
            case IDOK:
                DestroyWindow(hwndDlg);
                break;
            }
        }
        break;
    case WM_SIZE:
        PhLayoutManagerLayout(&context->LayoutManager);
        break;
    case NTM_RECEIVEDWHOIS:
        {
            PH_STRING_BUILDER receivedString;
            PPH_STRING convertedString = (PPH_STRING)lParam;

            PhInitializeStringBuilder(&receivedString, PAGE_SIZE);

            for (SIZE_T i = 0; i < convertedString->Length / sizeof(WCHAR); i++)
            {
                if (convertedString->Buffer[i] == '\n' && convertedString->Buffer[i + 1] == '\n')
                {
                    if (i < convertedString->Length - 2 &&
                        convertedString->Buffer[i] == '\n' &&
                        convertedString->Buffer[i + 1] == '\n' &&
                        convertedString->Buffer[i + 2] == '\n')
                    {
                        NOTHING;
                    }
                    else
                    {
                        PhAppendStringBuilder2(&receivedString, L"\r\n");
                    }
                }
                else
                {
                    PhAppendCharStringBuilder(&receivedString, convertedString->Buffer[i]);
                }
            }

            if (receivedString.String->Length >= 2 * 2 &&
                receivedString.String->Buffer[0] == '\n' &&
                receivedString.String->Buffer[1] == '\n')
            {
                PhRemoveStringBuilder(&receivedString, 0, 2);
            }

            RichEditAppendText(context->WhoisHandle, receivedString.String->Buffer);

            PhDeleteStringBuilder(&receivedString);
            PhDereferenceObject(convertedString);
        }
        break;
    case NTM_RECEIVEDFINISH:
        {
            PPH_STRING windowText = PH_AUTO(PhGetWindowText(context->WindowHandle));

            if (windowText)
            {
                Static_SetText(
                    context->WindowHandle,
                    PhaFormatString(L"%s Finished.", windowText->Buffer)->Buffer
                    );
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
    PNETWORK_OUTPUT_CONTEXT context;
    BOOL result;
    MSG message;
    HWND windowHandle;
    PH_AUTO_POOL autoPool;

    context = (PNETWORK_OUTPUT_CONTEXT)Parameter;

    if (PhBeginInitOnce(&initOnce))
    {
        LoadLibrary(L"msftedit.dll");
        PhEndInitOnce(&initOnce);
    }

    PhInitializeAutoPool(&autoPool);

    windowHandle = CreateDialogParam(
        (HINSTANCE)PluginInstance->DllBase,
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
