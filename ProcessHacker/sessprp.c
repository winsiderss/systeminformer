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
#include <winsta.h>
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
    SIP(L"Active", State_Active),
    SIP(L"Connected", State_Connected),
    SIP(L"ConnectQuery", State_ConnectQuery),
    SIP(L"Shadow", State_Shadow),
    SIP(L"Disconnected", State_Disconnected),
    SIP(L"Idle", State_Idle),
    SIP(L"Listen", State_Listen),
    SIP(L"Reset", State_Reset),
    SIP(L"Down", State_Down),
    SIP(L"Init", State_Init)
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
            WINSTATIONINFORMATION winStationInfo;
            BOOLEAN haveWinStationInfo;
            WINSTATIONCLIENT clientInfo;
            BOOLEAN haveClientInfo;
            ULONG returnLength;
            PWSTR stateString;

            SetProp(hwndDlg, L"SessionId", (HANDLE)sessionId);
            PhCenterWindow(hwndDlg, GetParent(hwndDlg));

            // Query basic session information

            haveWinStationInfo = WinStationQueryInformationW(
                NULL,
                sessionId,
                WinStationInformation,
                &winStationInfo,
                sizeof(WINSTATIONINFORMATION),
                &returnLength
                );

            // Query client information

            haveClientInfo = WinStationQueryInformationW(
                NULL,
                sessionId,
                WinStationClient,
                &clientInfo,
                sizeof(WINSTATIONCLIENT),
                &returnLength
                );

            if (haveWinStationInfo)
            {
                SetDlgItemText(hwndDlg, IDC_USERNAME,
                    PhaFormatString(L"%s\\%s", winStationInfo.Domain, winStationInfo.UserName)->Buffer);
            }

            SetDlgItemInt(hwndDlg, IDC_SESSIONID, sessionId, FALSE);

            if (haveWinStationInfo)
            {
                if (PhFindStringSiKeyValuePairs(
                    PhpConnectStatePairs,
                    sizeof(PhpConnectStatePairs),
                    winStationInfo.ConnectState,
                    &stateString
                    ))
                {
                    SetDlgItemText(hwndDlg, IDC_STATE, stateString);
                }
            }

            if (haveWinStationInfo && winStationInfo.LogonTime.QuadPart != 0)
            {
                SYSTEMTIME systemTime;
                PPH_STRING time;

                PhLargeIntegerToLocalSystemTime(&systemTime, &winStationInfo.LogonTime);
                time = PhFormatDateTime(&systemTime);
                SetDlgItemText(hwndDlg, IDC_LOGONTIME, time->Buffer);
                PhDereferenceObject(time);
            }

            if (haveWinStationInfo && winStationInfo.ConnectTime.QuadPart != 0)
            {
                SYSTEMTIME systemTime;
                PPH_STRING time;

                PhLargeIntegerToLocalSystemTime(&systemTime, &winStationInfo.ConnectTime);
                time = PhFormatDateTime(&systemTime);
                SetDlgItemText(hwndDlg, IDC_CONNECTTIME, time->Buffer);
                PhDereferenceObject(time);
            }

            if (haveWinStationInfo && winStationInfo.DisconnectTime.QuadPart != 0)
            {
                SYSTEMTIME systemTime;
                PPH_STRING time;

                PhLargeIntegerToLocalSystemTime(&systemTime, &winStationInfo.DisconnectTime);
                time = PhFormatDateTime(&systemTime);
                SetDlgItemText(hwndDlg, IDC_DISCONNECTTIME, time->Buffer);
                PhDereferenceObject(time);
            }

            if (haveWinStationInfo && winStationInfo.LastInputTime.QuadPart != 0)
            {
                SYSTEMTIME systemTime;
                PPH_STRING time;

                PhLargeIntegerToLocalSystemTime(&systemTime, &winStationInfo.LastInputTime);
                time = PhFormatDateTime(&systemTime);
                SetDlgItemText(hwndDlg, IDC_LASTINPUTTIME, time->Buffer);
                PhDereferenceObject(time);
            }

            if (haveClientInfo && clientInfo.ClientName[0] != 0)
            {
                WCHAR addressString[65];

                SetDlgItemText(hwndDlg, IDC_CLIENTNAME, clientInfo.ClientName);

                if (clientInfo.ClientAddressFamily == AF_INET6)
                {
                    struct in6_addr address;
                    ULONG i;
                    PUSHORT in;
                    PUSHORT out;

                    // IPv6 is special - the client address data is a reversed version of
                    // the real address.

                    in = (PUSHORT)clientInfo.ClientAddress;
                    out = (PUSHORT)address.u.Word;

                    for (i = 8; i != 0; i--)
                    {
                        *out = _byteswap_ushort(*in);
                        in++;
                        out++;
                    }

                    RtlIpv6AddressToString(&address, addressString);
                }
                else
                {
                    wcscpy_s(addressString, 65, clientInfo.ClientAddress);
                }

                SetDlgItemText(hwndDlg, IDC_CLIENTADDRESS, addressString);

                SetDlgItemText(hwndDlg, IDC_CLIENTDISPLAY,
                    PhaFormatString(L"%ux%u@%u", clientInfo.HRes,
                    clientInfo.VRes, clientInfo.ColorDepth)->Buffer
                    );
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
