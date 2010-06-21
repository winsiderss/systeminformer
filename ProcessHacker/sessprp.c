/*
 * Process Hacker - 
 *   session properties
 * 
 * Copyright (C) 2010 wj32
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

#include <phapp.h>
#include <wtsapi32.h>
#include <ws2tcpip.h>

INT_PTR CALLBACK PhpSessionPropertiesDlgProc(
    __in HWND hwndDlg,
    __in UINT uMsg,
    __in WPARAM wParam,
    __in LPARAM lParam
    );

#define SIP(String, Integer) { (String), (PVOID)(Integer) }

static PH_KEY_VALUE_PAIR PhpConnectStatePairs[] =
{
    SIP(L"Active", WTSActive),
    SIP(L"Connected", WTSConnected),
    SIP(L"ConnectQuery", WTSConnectQuery),
    SIP(L"Shadow", WTSShadow),
    SIP(L"Disconnected", WTSDisconnected),
    SIP(L"Idle", WTSIdle),
    SIP(L"Listen", WTSListen),
    SIP(L"Reset", WTSReset),
    SIP(L"Down", WTSDown),
    SIP(L"Init", WTSInit)
};

VOID PhShowSessionProperties(
    __in HWND ParentWindowHandle,
    __in ULONG SessionId
    )
{
    DialogBoxParam(
        PhInstanceHandle,
        MAKEINTRESOURCE(IDD_SESSION),
        ParentWindowHandle,
        PhpSessionPropertiesDlgProc,
        (LPARAM)SessionId
        );
}

INT_PTR CALLBACK PhpSessionPropertiesDlgProc(
    __in HWND hwndDlg,
    __in UINT uMsg,
    __in WPARAM wParam,
    __in LPARAM lParam
    )
{
    switch (uMsg)
    {
    case WM_INITDIALOG:
        {
            ULONG sessionId = (ULONG)lParam;
            PPH_STRING domainName = NULL;
            PPH_STRING userName = NULL;
            PULONG state = NULL;
            PWSTR stateString;
            PPH_STRING clientName = NULL;
            PWTS_CLIENT_ADDRESS clientAddress = NULL;
            WCHAR clientAddressString[65] = L"N/A";
            PWTS_CLIENT_DISPLAY clientDisplay = NULL;
            ULONG length;

            SetProp(hwndDlg, L"SessionId", (HANDLE)sessionId);
            PhCenterWindow(hwndDlg, GetParent(hwndDlg));

            domainName = PHA_DEREFERENCE(PhGetSessionInformationString(
                WTS_CURRENT_SERVER_HANDLE, sessionId, WTSDomainName));
            userName = PHA_DEREFERENCE(PhGetSessionInformationString(
                WTS_CURRENT_SERVER_HANDLE, sessionId, WTSUserName));
            clientName = PHA_DEREFERENCE(PhGetSessionInformationString(
                WTS_CURRENT_SERVER_HANDLE, sessionId, WTSClientName));

            SetDlgItemText(hwndDlg, IDC_USERNAME,
                PhaFormatString(L"%s\\%s", PhGetStringOrEmpty(domainName), PhGetStringOrEmpty(userName))->Buffer);
            SetDlgItemInt(hwndDlg, IDC_SESSIONID, sessionId, FALSE);

            WTSQuerySessionInformation(
                WTS_CURRENT_SERVER_HANDLE,
                sessionId,
                WTSConnectState,
                (PWSTR *)&state,
                &length
                );

            if (state)
            {
                if (PhFindStringSiKeyValuePairs(
                    PhpConnectStatePairs,
                    sizeof(PhpConnectStatePairs),
                    *state,
                    &stateString
                    ))
                {
                    SetDlgItemText(hwndDlg, IDC_STATE, stateString);
                }

                WTSFreeMemory(state);
            }

            if (!PhIsStringNullOrEmpty(clientName))
                SetDlgItemText(hwndDlg, IDC_CLIENTNAME, clientName->Buffer);

            WTSQuerySessionInformation(
                WTS_CURRENT_SERVER_HANDLE,
                sessionId,
                WTSClientAddress,
                (PWSTR *)&clientAddress,
                &length
                );

            if (clientAddress)
            {
                if (!PhIsStringNullOrEmpty(clientName))
                {
                    switch (clientAddress->AddressFamily)
                    {
                    case AF_INET:
                        RtlIpv4AddressToString((struct in_addr *)&clientAddress->Address[2], clientAddressString);
                        break;
                    case AF_INET6:
                        RtlIpv6AddressToString((struct in6_addr *)&clientAddress->Address[2], clientAddressString);
                        break;
                    }

                    SetDlgItemText(hwndDlg, IDC_CLIENTADDRESS, clientAddressString);
                }

                WTSFreeMemory(clientAddress);
            }

            WTSQuerySessionInformation(
                WTS_CURRENT_SERVER_HANDLE,
                sessionId,
                WTSClientDisplay,
                (PWSTR *)&clientDisplay,
                &length
                );

            if (clientDisplay)
            {
                if (!PhIsStringNullOrEmpty(clientName))
                {
                    SetDlgItemText(hwndDlg, IDC_CLIENTDISPLAY,
                        PhaFormatString(L"%ux%u@%u", clientDisplay->HorizontalResolution,
                        clientDisplay->VerticalResolution, clientDisplay->ColorDepth)->Buffer
                        );
                }

                WTSFreeMemory(clientDisplay);
            }

            SetFocus(GetDlgItem(hwndDlg, IDOK));
        }
        break;
    case WM_DESTROY:
        {
            RemoveProp(hwndDlg, L"SessionId");
        }
        break;
    case WM_COMMAND:
        {
            switch (LOWORD(wParam))
            {
            case IDCANCEL:
            case IDOK:
                EndDialog(hwndDlg, IDOK);
                break;
            }
        }
        break;
    }

    return FALSE;
}
