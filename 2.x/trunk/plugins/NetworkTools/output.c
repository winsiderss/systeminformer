/*
 * Process Hacker Network Tools -
 *   output dialog
 *
 * Copyright (C) 2010-2015 wj32
 * Copyright (C) 2012-2015 dmex
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
            PhSaveWindowPlacementToSetting(SETTING_NAME_TRACERT_WINDOW_POSITION, SETTING_NAME_TRACERT_WINDOW_SIZE, hwndDlg);
            PhDeleteLayoutManager(&context->LayoutManager);

            if (context->ProcessHandle)
            {
                // Terminate the child process.
                PhTerminateProcess(context->ProcessHandle, STATUS_SUCCESS);

                // Close the child process handle.
                NtClose(context->ProcessHandle);
            }

            // Close the pipe handle.
            if (context->PipeReadHandle)
                NtClose(context->PipeReadHandle);

            RemoveProp(hwndDlg, L"Context");
            PhFree(context);
        }
    }

    if (!context)
        return FALSE;

    switch (uMsg)
    {
    case WM_INITDIALOG:
        {
            PH_RECTANGLE windowRectangle;

            context->WindowHandle = hwndDlg;
            context->OutputHandle = GetDlgItem(hwndDlg, IDC_NETOUTPUTEDIT);

            PhInitializeLayoutManager(&context->LayoutManager, hwndDlg);
            PhAddLayoutItem(&context->LayoutManager, context->OutputHandle, NULL, PH_ANCHOR_ALL);
            PhAddLayoutItem(&context->LayoutManager, GetDlgItem(hwndDlg, IDC_MORE_INFO), NULL, PH_ANCHOR_BOTTOM | PH_ANCHOR_LEFT);
            PhAddLayoutItem(&context->LayoutManager, GetDlgItem(hwndDlg, IDOK), NULL, PH_ANCHOR_BOTTOM | PH_ANCHOR_RIGHT);

            windowRectangle.Position = PhGetIntegerPairSetting(SETTING_NAME_TRACERT_WINDOW_POSITION);
            windowRectangle.Size = PhGetIntegerPairSetting(SETTING_NAME_TRACERT_WINDOW_SIZE);

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
                PhLoadWindowPlacementFromSetting(SETTING_NAME_TRACERT_WINDOW_POSITION, SETTING_NAME_TRACERT_WINDOW_SIZE, hwndDlg);
            }

            if (context->IpAddress.Type == PH_IPV4_NETWORK_TYPE)
            {
                RtlIpv4AddressToString(&context->IpAddress.InAddr, context->IpAddressString);
            }
            else
            {
                RtlIpv6AddressToString(&context->IpAddress.In6Addr, context->IpAddressString);
            }

            switch (context->Action)
            {
            case NETWORK_ACTION_TRACEROUTE:
                {
                    HANDLE dialogThread = INVALID_HANDLE_VALUE;

                    Static_SetText(context->WindowHandle,
                        PhaFormatString(L"Tracing route to %s...", context->IpAddressString)->Buffer
                        );

                    if (dialogThread = PhCreateThread(0, NetworkTracertThreadStart, (PVOID)context))
                        NtClose(dialogThread);
                }
                break;
            case NETWORK_ACTION_WHOIS:
                {
                    HANDLE dialogThread = INVALID_HANDLE_VALUE;

                    Static_SetText(context->WindowHandle,
                        PhaFormatString(L"Whois %s...", context->IpAddressString)->Buffer
                        );

                    ShowWindow(GetDlgItem(hwndDlg, IDC_MORE_INFO), SW_SHOW);

                    if (dialogThread = PhCreateThread(0, NetworkWhoisThreadStart, (PVOID)context))
                        NtClose(dialogThread);
                }
                break;
            case NETWORK_ACTION_PATHPING:
                {
                    HANDLE dialogThread = INVALID_HANDLE_VALUE;

                    Static_SetText(context->WindowHandle,
                        PhaFormatString(L"Pathing route to %s...", context->IpAddressString)->Buffer
                        );

                    if (dialogThread = PhCreateThread(0, NetworkTracertThreadStart, (PVOID)context))
                        NtClose(dialogThread);
                }
                break;
            }
        }
        break;
    case WM_COMMAND:
        {
            switch (GET_WM_COMMAND_ID(wParam, lParam))
            {
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

            // Check if old graph colors are enabled.
            if (!PhGetIntegerSetting(L"GraphColorMode"))
                break;

            // Check for our edit control and change the color.
            if (hwndChild == context->OutputHandle)
            {
                // Set a transparent background for the control backcolor.
                //SetBkMode(hDC, TRANSPARENT);

                // Set the Edit control background.
                SetBkColor(hDC, RGB(0x0, 0x0, 0x0));

                // Set text color as the Green PH graph text color.
                SetTextColor(hDC, RGB(124, 252, 0));

                // Set a black control backcolor.
                return (INT_PTR)GetStockBrush(BLACK_BRUSH);
            }
        }
        break;
    case WM_NOTIFY:
        {
            switch (((LPNMHDR)lParam)->code)
            {
            case NM_CLICK:
            case NM_RETURN:
                {
                    PNMLINK syslink = (PNMLINK)lParam;

                    if (syslink->hdr.idFrom == IDC_MORE_INFO)
                    {
                        PhShellExecute(
                            PhMainWndHandle,
                            PhaConcatStrings2(L"http://wq.apnic.net/apnic-bin/whois.pl?searchtext=", context->IpAddressString)->Buffer,
                            NULL
                            );
                    }
                }
                break;
            }
        }
        break;
    case NTM_RECEIVEDTRACE:
        {
            OEM_STRING inputString;
            UNICODE_STRING convertedString;
            PH_STRING_BUILDER receivedString;

            if (wParam != 0)
            {
                inputString.Buffer = (PCHAR)lParam;
                inputString.Length = (USHORT)wParam;

                if (NT_SUCCESS(RtlOemStringToUnicodeString(&convertedString, &inputString, TRUE)))
                {
                    PPH_STRING windowText = NULL;

                    PhInitializeStringBuilder(&receivedString, PAGE_SIZE);

                    // Get the current output text.
                    windowText = PhGetWindowText(context->OutputHandle);

                    // Append the current output text to the New string.
                    if (!PhIsNullOrEmptyString(windowText))
                        PhAppendStringBuilder(&receivedString, &windowText->sr);

                    PhAppendFormatStringBuilder(&receivedString, L"%s", convertedString.Buffer);

                    // Remove leading newlines.
                    if (receivedString.String->Length >= 2 * 2 &&
                        receivedString.String->Buffer[0] == '\r' &&
                        receivedString.String->Buffer[1] == '\n')
                    {
                        PhRemoveStringBuilder(&receivedString, 0, 2);
                    }

                    SetWindowText(context->OutputHandle, receivedString.String->Buffer);
                    SendMessage(
                        context->OutputHandle,
                        EM_SETSEL,
                        receivedString.String->Length / 2 - 1,
                        receivedString.String->Length / 2 - 1
                        );
                    SendMessage(context->OutputHandle, WM_VSCROLL, SB_BOTTOM, 0);

                    PhDereferenceObject(windowText);
                    PhDeleteStringBuilder(&receivedString);
                    RtlFreeUnicodeString(&convertedString);
                }
            }
        }
        break;
    case NTM_RECEIVEDWHOIS:
        {
            OEM_STRING inputString;
            UNICODE_STRING convertedString;
            PH_STRING_BUILDER receivedString;

            if (lParam != 0)
            {
                inputString.Buffer = (PCHAR)lParam;
                inputString.Length = (USHORT)wParam;

                if (NT_SUCCESS(RtlOemStringToUnicodeString(&convertedString, &inputString, TRUE)))
                {
                    USHORT i;

                    PhInitializeStringBuilder(&receivedString, PAGE_SIZE);

                    // Convert carriage returns.
                    for (i = 0; i < convertedString.Length; i++)
                    {
                        if (convertedString.Buffer[i] == '\n')
                        {
                            PhAppendStringBuilder2(&receivedString, L"\r\n");
                        }
                        else
                        {
                            PhAppendCharStringBuilder(&receivedString, convertedString.Buffer[i]);
                        }
                    }

                    // Remove leading newlines.
                    if (receivedString.String->Length >= 2 * 2 &&
                        receivedString.String->Buffer[0] == '\r' &&
                        receivedString.String->Buffer[1] == '\n')
                    {
                        PhRemoveStringBuilder(&receivedString, 0, 2);
                    }

                    SetWindowText(context->OutputHandle, receivedString.String->Buffer);
                    SendMessage(
                        context->OutputHandle,
                        EM_SETSEL,
                        receivedString.String->Length / 2 - 1,
                        receivedString.String->Length / 2 - 1
                        );
                    SendMessage(context->OutputHandle, WM_VSCROLL, SB_TOP, 0);

                    PhDeleteStringBuilder(&receivedString);
                    RtlFreeUnicodeString(&convertedString);
                }

                PhFree((PVOID)lParam);
            }
        }
        break;
    case NTM_RECEIVEDFINISH:
        {
            PPH_STRING windowText = PhGetWindowText(context->WindowHandle);

            if (windowText)
            {
                Static_SetText(
                    context->WindowHandle,
                    PhaFormatString(L"%s Finished.", windowText->Buffer)->Buffer
                    );
                PhDereferenceObject(windowText);
            }
        }
        break;
    }

    return FALSE;
}

static NTSTATUS PhNetworkOutputDialogThreadStart(
    _In_ PVOID Parameter
    )
{
    BOOL result;
    MSG message;
    HWND windowHandle;
    PH_AUTO_POOL autoPool;
    PNETWORK_OUTPUT_CONTEXT context = (PNETWORK_OUTPUT_CONTEXT)Parameter;

    PhInitializeAutoPool(&autoPool);

    windowHandle = CreateDialogParam(
        (HINSTANCE)PluginInstance->DllBase,
        MAKEINTRESOURCE(IDD_OUTPUT),
        PhMainWndHandle,
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
    DestroyWindow(windowHandle);

    return STATUS_SUCCESS;
}

VOID PerformNetworkAction(
    _In_ PH_NETWORK_ACTION Action,
    _In_ PPH_NETWORK_ITEM NetworkItem
    )
{
    HANDLE dialogThread = INVALID_HANDLE_VALUE;
    PNETWORK_OUTPUT_CONTEXT context;

    context = (PNETWORK_OUTPUT_CONTEXT)PhAllocate(sizeof(NETWORK_OUTPUT_CONTEXT));
    memset(context, 0, sizeof(NETWORK_OUTPUT_CONTEXT));

    context->Action = Action;
    context->NetworkItem = NetworkItem;
    context->IpAddress = NetworkItem->RemoteEndpoint.Address;

    if (context->Action == NETWORK_ACTION_PING)
    {
        if (dialogThread = PhCreateThread(0, PhNetworkPingDialogThreadStart, (PVOID)context))
            NtClose(dialogThread);
    }
    else
    {
        if (dialogThread = PhCreateThread(0, PhNetworkOutputDialogThreadStart, (PVOID)context))
            NtClose(dialogThread);
    }
}