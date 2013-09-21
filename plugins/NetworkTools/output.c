/*
 * Process Hacker Network Tools -
 *   output dialog
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

#include "nettools.h"

static RECT MinimumSize = { -1, -1, -1, -1 };

static INT_PTR CALLBACK NetworkOutputDlgProc(
    __in HWND hwndDlg,
    __in UINT uMsg,
    __in WPARAM wParam,
    __in LPARAM lParam
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

        if (uMsg == WM_NCDESTROY)
        {
            PhSaveWindowPlacementToSetting(L"ProcessHacker.NetTools.NetToolsWindowPosition", L"ProcessHacker.NetTools.NetToolsWindowSize", hwndDlg); 
            PhDeleteStringBuilder(&context->ReceivedString);
            RemoveProp(hwndDlg, L"Context");
        }
    }

    if (!context)
        return FALSE;

    switch (uMsg)
    {
    case WM_INITDIALOG:
        {
            PH_RECTANGLE windowRectangle;
            HANDLE dialogThread = INVALID_HANDLE_VALUE;

            PhInitializeStringBuilder(&context->ReceivedString, PAGE_SIZE);
            PhInitializeLayoutManager(&context->LayoutManager, hwndDlg);
            PhAddLayoutItem(&context->LayoutManager, GetDlgItem(hwndDlg, IDC_NETOUTPUTEDIT), NULL, PH_ANCHOR_ALL);
            PhAddLayoutItem(&context->LayoutManager, GetDlgItem(hwndDlg, IDC_NETRETRY), NULL, PH_ANCHOR_BOTTOM | PH_ANCHOR_RIGHT);
            PhAddLayoutItem(&context->LayoutManager, GetDlgItem(hwndDlg, IDOK), NULL, PH_ANCHOR_BOTTOM | PH_ANCHOR_RIGHT);
             
            context->UseOldColors = !!PhGetIntegerSetting(L"GraphColorMode");
            windowRectangle.Position = PhGetIntegerPairSetting(L"ProcessHacker.NetTools.NetToolsWindowPosition");
            windowRectangle.Size = PhGetIntegerPairSetting(L"ProcessHacker.NetTools.NetToolsWindowSize");

            if (MinimumSize.left == -1)
            {
                RECT rect;

                rect.left = 0;
                rect.top = 0;
                rect.right = 190;
                rect.bottom = 120;
                MapDialogRect(hwndDlg, &rect);
                MinimumSize = rect;
                MinimumSize.left = 0;
            }

            // Check for first-run default position.
            if (windowRectangle.Position.X == 0 || windowRectangle.Position.Y == 0)
            {
                PhCenterWindow(hwndDlg, GetParent(hwndDlg));
            }
            else
            {
                PhLoadWindowPlacementFromSetting(L"ProcessHacker.NetTools.NetToolsWindowPosition", L"ProcessHacker.NetTools.NetToolsWindowSize", hwndDlg);
            }

            if (context->NetworkItem->RemoteEndpoint.Address.Type == PH_IPV4_NETWORK_TYPE)
            {
                RtlIpv4AddressToString(&context->NetworkItem->RemoteEndpoint.Address.InAddr, context->addressString);
            }
            else
            {
                RtlIpv6AddressToString(&context->NetworkItem->RemoteEndpoint.Address.In6Addr, context->addressString);
            }

            switch (context->Action)
            {
            case NETWORK_ACTION_TRACEROUTE:
                {
                    if (dialogThread = PhCreateThread(0, (PUSER_THREAD_START_ROUTINE)NetworkTracertThreadStart, (PVOID)context))
                        NtClose(dialogThread);
                }
                break;
            case NETWORK_ACTION_WHOIS:
                {
                    if (dialogThread = PhCreateThread(0, (PUSER_THREAD_START_ROUTINE)NetworkWhoisThreadStart, (PVOID)context))
                        NtClose(dialogThread);
                }
                break;
            }
        }
        break;
    case WM_COMMAND:
        {
            switch (LOWORD(wParam))
            {
            case IDC_NETRETRY:
                Button_Enable(GetDlgItem(hwndDlg, IDC_NETRETRY), FALSE);
                break;
            case IDCANCEL:
            case IDOK:
                PostQuitMessage(0);
                break;
            }
        }
        break;
    case WM_SIZE:
        PhLayoutManagerLayout(&context->LayoutManager);
        break;   
    case WM_SIZING:
        PhResizingMinimumSize((PRECT)lParam, wParam, MinimumSize.right, MinimumSize.bottom);
        break;
    case WM_CTLCOLORDLG: 
    case WM_CTLCOLORSTATIC:    
        {
            HDC hDC = (HDC)wParam;
            HWND hwndChild = (HWND)lParam;
            ULONG hwndChildID = 0;
            
            // Check if OldColors are enabled.
            if (!context->UseOldColors)
                break;

            // Get the control ID.
            hwndChildID = GetDlgCtrlID(hwndChild);

            // Set a transparent background for the control backcolor.
            SetBkMode(hDC, TRANSPARENT);

            // Check for our edit control and change the color.
            if (hwndChildID == IDC_NETOUTPUTEDIT)
            {
                // Set a Vista style text color.
                SetTextColor(hDC, RGB(124, 252, 0));
                
                // Set a black control backcolor.
                return (INT_PTR)GetStockBrush(BLACK_BRUSH);
            }
        }
        break;
    case NTM_RECEIVEDTRACE:
        {
            OEM_STRING inputString;
            UNICODE_STRING convertedString;

            if (wParam != 0)
            {
                inputString.Buffer = (PCHAR)lParam;
                inputString.Length = (USHORT)wParam;

                if (NT_SUCCESS(RtlOemStringToUnicodeString(&convertedString, &inputString, TRUE)))
                {
                    PhAppendStringBuilderEx(&context->ReceivedString, convertedString.Buffer, convertedString.Length);
                    RtlFreeUnicodeString(&convertedString);

                    // Remove leading newlines.
                    if (context->ReceivedString.String->Length >= 2 * 2 &&
                        context->ReceivedString.String->Buffer[0] == '\r' && 
                        context->ReceivedString.String->Buffer[1] == '\n')
                    {
                        PhRemoveStringBuilder(&context->ReceivedString, 0, 2);
                    }

                    SetDlgItemText(hwndDlg, IDC_NETOUTPUTEDIT, context->ReceivedString.String->Buffer);
                    SendMessage(
                        GetDlgItem(hwndDlg, IDC_NETOUTPUTEDIT),
                        EM_SETSEL,
                        context->ReceivedString.String->Length / 2 - 1,
                        context->ReceivedString.String->Length / 2 - 1
                        );
                    SendMessage(GetDlgItem(hwndDlg, IDC_NETOUTPUTEDIT), WM_VSCROLL, SB_BOTTOM, 0);
                    return TRUE;
                }
            }
        }
        break;   
    case NTM_RECEIVEDPING:
        {
            PPH_STRING inputString;

            if (wParam != 0)
            {
                inputString = (PPH_STRING)wParam;
                
                //PhReferenceObject(inputString);
                PhAppendStringBuilderEx(&context->ReceivedString, inputString->Buffer, inputString->Length);
                PhDereferenceObject(inputString);

                // Remove leading newlines.
                if (context->ReceivedString.String->Length >= 2 * 2 &&
                    context->ReceivedString.String->Buffer[0] == '\r' && 
                    context->ReceivedString.String->Buffer[1] == '\n')
                {
                    PhRemoveStringBuilder(&context->ReceivedString, 0, 2);
                }

                SetDlgItemText(hwndDlg, IDC_NETOUTPUTEDIT, context->ReceivedString.String->Buffer);
                SendMessage(
                    GetDlgItem(hwndDlg, IDC_NETOUTPUTEDIT),
                    EM_SETSEL,
                    context->ReceivedString.String->Length / 2 - 1,
                    context->ReceivedString.String->Length / 2 - 1
                    );
                SendMessage(GetDlgItem(hwndDlg, IDC_NETOUTPUTEDIT), WM_VSCROLL, SB_BOTTOM, 0);
                return TRUE;
            }
        }
        break;
    case NTM_RECEIVEDWHOIS:
        {
            OEM_STRING inputString;
            UNICODE_STRING convertedString;
            PH_STRING_BUILDER receivedString;

            if (wParam != 0)
            {
                inputString.Buffer = (PCHAR)lParam;
                inputString.Length = (USHORT)wParam;

                PhInitializeStringBuilder(&receivedString, PAGE_SIZE);

                if (NT_SUCCESS(RtlOemStringToUnicodeString(&convertedString, &inputString, TRUE)))
                {
                    SIZE_T i;

                    // Remove leading newlines.                  
                    for (i = 0; i < inputString.Length; i++)
                    {
                        if (inputString.Buffer[i] == '\n')
                        {
                            PhAppendStringBuilder(&receivedString, PhaCreateString(L"\r\n"));
                        }
                        else if (inputString.Buffer[i] == '#')
                        {
                            // Skip
                        }
                        else
                        {
                            PhAppendCharStringBuilder(&receivedString, inputString.Buffer[i]);
                        }
                    }

                    if (receivedString.String->Length >= 2 * 2 &&
                        receivedString.String->Buffer[0] == '\r' && 
                        receivedString.String->Buffer[1] == '\n')
                    {
                        PhRemoveStringBuilder(&receivedString, 0, 4);
                    }

                    SetDlgItemText(hwndDlg, IDC_NETOUTPUTEDIT, receivedString.String->Buffer);
                    SendMessage(
                        GetDlgItem(hwndDlg, IDC_NETOUTPUTEDIT),
                        EM_SETSEL,
                        receivedString.String->Length / 2 - 1,
                        receivedString.String->Length / 2 - 1
                        );
                    SendMessage(GetDlgItem(hwndDlg, IDC_NETOUTPUTEDIT), WM_VSCROLL, SB_BOTTOM, 0);

                    PhDeleteStringBuilder(&receivedString);
                    return TRUE;
                }
            }
        }
        break;
    }

    return FALSE;
}

static NTSTATUS PhNetworkOutputDialogThreadStart(
    __in PVOID Parameter
    )
{
    BOOL result;
    MSG message;
    PH_AUTO_POOL autoPool;
    PNETWORK_OUTPUT_CONTEXT context = (PNETWORK_OUTPUT_CONTEXT)Parameter;

    PhInitializeAutoPool(&autoPool);

    context->WindowHandle = CreateDialogParam(
        (HINSTANCE)PluginInstance->DllBase,
        MAKEINTRESOURCE(IDD_OUTPUT),
        PhMainWndHandle,
        NetworkOutputDlgProc,
        (LPARAM)Parameter
        );

    ShowWindow(context->WindowHandle, SW_SHOW);
    SetForegroundWindow(context->WindowHandle);

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
    DestroyWindow(context->WindowHandle);
    PhFree(context);
    return STATUS_SUCCESS;
}

static HFONT InitializeFont(
    __in HWND hwndDlg
    )
{
    LOGFONT logFont = { 0 };
    HFONT fontHandle = NULL;

    logFont.lfHeight = 14;
    logFont.lfWeight = FW_NORMAL;//FW_MEDIUM;
    logFont.lfQuality = CLEARTYPE_QUALITY | ANTIALIASED_QUALITY;
    
    // GDI uses the first font that matches the above attributes.
    fontHandle = CreateFontIndirect(&logFont);

    if (fontHandle)
    {
        SendMessage(hwndDlg, WM_SETFONT, (WPARAM)fontHandle, FALSE);
        return fontHandle;
    }

    return NULL;
}

VOID PerformNetworkAction(
    __in ULONG Action,
    __in PPH_NETWORK_ITEM NetworkItem
    )
{ 
    HANDLE dialogThread = INVALID_HANDLE_VALUE;
    PNETWORK_OUTPUT_CONTEXT context = (PNETWORK_OUTPUT_CONTEXT)PhAllocate(
        sizeof(NETWORK_OUTPUT_CONTEXT)
        );
    memset(context, 0, sizeof(NETWORK_OUTPUT_CONTEXT));

    context->Action = Action;
    context->NetworkItem = NetworkItem;

    if (context->Action == NETWORK_ACTION_PING)
    {
        if (dialogThread = PhCreateThread(0, (PUSER_THREAD_START_ROUTINE)PhNetworkPingDialogThreadStart, (PVOID)context))
            NtClose(dialogThread);
    }
    else
    {
        if (dialogThread = PhCreateThread(0, (PUSER_THREAD_START_ROUTINE)PhNetworkOutputDialogThreadStart, (PVOID)context)) 
            NtClose(dialogThread);
    }
}