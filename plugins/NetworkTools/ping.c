/*
 * Process Hacker Network Tools -
 *   Ping dialog
 *
 * Copyright (C) 2015 dmex
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

#define WM_PING_UPDATE (WM_APP + 151)

static RECT NormalGraphTextMargin = { 5, 5, 5, 5 };
static RECT NormalGraphTextPadding = { 3, 3, 3, 3 };

NTSTATUS NetworkPingThreadStart(
    _In_ PVOID Parameter
    )
{
    PNETWORK_PING_CONTEXT context = (PNETWORK_PING_CONTEXT)Parameter;
    HANDLE icmpHandle = INVALID_HANDLE_VALUE;
    ULONG icmpCurrentPingMs = 0;
    ULONG icmpReplyCount = 0;
    ULONG icmpReplyLength = 0;
    PVOID icmpReplyBuffer = NULL;
    PPH_BYTES icmpEchoBuffer = NULL;
    PPH_STRING icmpRandString = NULL;
    IP_OPTION_INFORMATION pingOptions =
    {
        255,         // Time To Live
        0,           // Type Of Service
        IP_FLAG_DF,  // IP header flags
        0            // Size of options data
    };
    //pingOptions.Flags |= IP_FLAG_REVERSE;

    if (icmpRandString = PhCreateStringEx(NULL, PhGetIntegerSetting(SETTING_NAME_PING_SIZE) * sizeof(WCHAR) + sizeof(UNICODE_NULL)))
    {
        PhGenerateRandomAlphaString(icmpRandString->Buffer, (ULONG)icmpRandString->Length / sizeof(WCHAR));

        icmpEchoBuffer = PhConvertUtf16ToMultiByte(icmpRandString->Buffer);
        PhDereferenceObject(icmpRandString);
    }

    if (context->RemoteEndpoint.Address.Type == PH_IPV6_NETWORK_TYPE)
    {
        SOCKADDR_IN6 icmp6LocalAddr = { 0 };
        SOCKADDR_IN6 icmp6RemoteAddr = { 0 };
        PICMPV6_ECHO_REPLY2 icmp6ReplyStruct = NULL;

        // TODO: Cache handle.
        if ((icmpHandle = Icmp6CreateFile()) == INVALID_HANDLE_VALUE)
            goto CleanupExit;

        // Set Local IPv6-ANY address.
        icmp6LocalAddr.sin6_addr = in6addr_any;
        icmp6LocalAddr.sin6_family = AF_INET6;

        // Set Remote IPv6 address.
        icmp6RemoteAddr.sin6_addr = context->RemoteEndpoint.Address.In6Addr;
        //icmp6RemoteAddr.sin6_port = _byteswap_ushort((USHORT)context->NetworkItem->RemoteEndpoint.Port);

        // Allocate ICMPv6 message.
        icmpReplyLength = ICMP_BUFFER_SIZE(sizeof(ICMPV6_ECHO_REPLY), icmpEchoBuffer);
        icmpReplyBuffer = PhAllocate(icmpReplyLength);
        memset(icmpReplyBuffer, 0, icmpReplyLength);

        InterlockedIncrement(&context->PingSentCount);

        // Send ICMPv6 ping...
        icmpReplyCount = Icmp6SendEcho2(
            icmpHandle,
            NULL,
            NULL,
            NULL,
            &icmp6LocalAddr,
            &icmp6RemoteAddr,
            icmpEchoBuffer->Buffer,
            (USHORT)icmpEchoBuffer->Length,
            &pingOptions,
            icmpReplyBuffer,
            icmpReplyLength,
            context->MaxPingTimeout
            );

        icmp6ReplyStruct = (PICMPV6_ECHO_REPLY2)icmpReplyBuffer;

        if (icmpReplyCount > 0 && icmp6ReplyStruct->Status == IP_SUCCESS)
        {
            BOOLEAN icmpPacketSignature = FALSE;

            if (!RtlEqualMemory(
                icmp6ReplyStruct->Address.sin6_addr,
                context->RemoteEndpoint.Address.In6Addr.u.Word,
                sizeof(icmp6ReplyStruct->Address.sin6_addr)
                ))
            {
                InterlockedIncrement(&context->UnknownAddrCount);
            }

            icmpPacketSignature = RtlEqualMemory(
                icmpEchoBuffer->Buffer,
                icmp6ReplyStruct->Data,
                icmpEchoBuffer->Length
                );

            if (!icmpPacketSignature)
            {
                InterlockedIncrement(&context->HashFailCount);
            }
        }
        else
        {
            InterlockedIncrement(&context->PingLossCount);
        }

        icmpCurrentPingMs = icmp6ReplyStruct->RoundTripTime;
    }
    else
    {
        IPAddr icmpLocalAddr = 0;
        IPAddr icmpRemoteAddr = 0;
        BOOLEAN icmpPacketSignature = FALSE;
        PICMP_ECHO_REPLY icmpReplyStruct = NULL;

        // TODO: Cache handle.
        if ((icmpHandle = IcmpCreateFile()) == INVALID_HANDLE_VALUE)
            goto CleanupExit;

        // Set Local IPv4-ANY address.
        icmpLocalAddr = in4addr_any.s_addr;

        // Set Remote IPv4 address.
        icmpRemoteAddr = context->RemoteEndpoint.Address.InAddr.s_addr;

        // Allocate ICMPv4 message.
        icmpReplyLength = ICMP_BUFFER_SIZE(sizeof(ICMP_ECHO_REPLY), icmpEchoBuffer);
        icmpReplyBuffer = PhAllocate(icmpReplyLength);
        memset(icmpReplyBuffer, 0, icmpReplyLength);

        InterlockedIncrement(&context->PingSentCount);

        // Send ICMPv4 ping...
        icmpReplyCount = IcmpSendEcho2Ex(
            icmpHandle,
            NULL,
            NULL,
            NULL,
            icmpLocalAddr,
            icmpRemoteAddr,
            NULL,
            0,
            &pingOptions,
            icmpReplyBuffer,
            icmpReplyLength,
            context->MaxPingTimeout
            );

        icmpReplyStruct = (PICMP_ECHO_REPLY)icmpReplyBuffer;

        if (icmpReplyCount > 0 && icmpReplyStruct->Status == IP_SUCCESS)
        {
            if (icmpReplyStruct->Address != context->RemoteEndpoint.Address.InAddr.s_addr)
            {
                InterlockedIncrement(&context->UnknownAddrCount);
            }

            if (icmpReplyStruct->DataSize == icmpEchoBuffer->Length)
            {
                icmpPacketSignature = RtlEqualMemory(
                    icmpEchoBuffer->Buffer,
                    icmpReplyStruct->Data,
                    icmpReplyStruct->DataSize);
            }

            if (!icmpPacketSignature)
            {
                InterlockedIncrement(&context->HashFailCount);
            }
        }
        else
        {
            InterlockedIncrement(&context->PingLossCount);
        }

        icmpCurrentPingMs = icmpReplyStruct->RoundTripTime;
    }  

    if (context->PingMinMs == 0 || icmpCurrentPingMs < context->PingMinMs)
        context->PingMinMs = icmpCurrentPingMs;
    if (icmpCurrentPingMs > context->PingMaxMs)
        context->PingMaxMs = icmpCurrentPingMs;

    context->CurrentPingMs = icmpCurrentPingMs;

    InterlockedIncrement(&context->PingRecvCount);

    PhAddItemCircularBuffer_ULONG(&context->PingHistory, icmpCurrentPingMs);

CleanupExit:

    if (icmpEchoBuffer)
    {
        PhDereferenceObject(icmpEchoBuffer);
    }

    if (icmpHandle != INVALID_HANDLE_VALUE)
    {
        IcmpCloseHandle(icmpHandle);
    }

    if (icmpReplyBuffer)
    {
        PhFree(icmpReplyBuffer);
    }

    PostMessage(context->WindowHandle, WM_PING_UPDATE, 0, 0);

    return STATUS_SUCCESS;
}

VOID NTAPI NetworkPingUpdateHandler(
    _In_opt_ PVOID Parameter,
    _In_opt_ PVOID Context
    )
{
    PNETWORK_PING_CONTEXT context = (PNETWORK_PING_CONTEXT)Context;

    if (!context)
        return;

    // Queue up the next ping request
    PhQueueItemWorkQueue(&context->PingWorkQueue, NetworkPingThreadStart, (PVOID)context);
}

VOID NetworkPingUpdateGraph(
    _In_ PNETWORK_PING_CONTEXT Context
    )
{
    Context->PingGraphState.Valid = FALSE;
    Context->PingGraphState.TooltipIndex = -1;
    Graph_MoveGrid(Context->PingGraphHandle, 1);
    Graph_Draw(Context->PingGraphHandle);
    Graph_UpdateTooltip(Context->PingGraphHandle);
    InvalidateRect(Context->PingGraphHandle, NULL, FALSE);
}

INT_PTR CALLBACK NetworkPingWndProc(
    _In_ HWND hwndDlg,
    _In_ UINT uMsg,
    _In_ WPARAM wParam,
    _In_ LPARAM lParam
    )
{
    PNETWORK_PING_CONTEXT context = NULL;

    if (uMsg == WM_INITDIALOG)
    {
        context = (PNETWORK_PING_CONTEXT)lParam;
        PhSetWindowContext(hwndDlg, PH_WINDOW_CONTEXT_DEFAULT, context);
    }
    else
    {
        context = PhGetWindowContext(hwndDlg, PH_WINDOW_CONTEXT_DEFAULT);
    }

    if (context == NULL)
        return FALSE;

    switch (uMsg)
    {
    case WM_INITDIALOG:
        {
            PPH_LAYOUT_ITEM panelItem;

            PhSetApplicationWindowIcon(hwndDlg);

            // We have already set the group boxes to have WS_EX_TRANSPARENT to fix
            // the drawing issue that arises when using WS_CLIPCHILDREN. However
            // in removing the flicker from the graphs the group boxes will now flicker.
            // It's a good tradeoff since no one stares at the group boxes.
            PhSetWindowStyle(hwndDlg, WS_CLIPCHILDREN, WS_CLIPCHILDREN);

            context->WindowHandle = hwndDlg;
            context->StatusHandle = GetDlgItem(hwndDlg, IDC_MAINTEXT);
            context->MaxPingTimeout = PhGetIntegerSetting(SETTING_NAME_PING_MINIMUM_SCALING);
            context->FontHandle = PhCreateCommonFont(-15, FW_MEDIUM, context->StatusHandle);
            context->PingGraphHandle = CreateWindow(
                PH_GRAPH_CLASSNAME,
                NULL,
                WS_VISIBLE | WS_CHILD | WS_BORDER,
                0,
                0,
                3,
                3,
                hwndDlg,
                NULL,
                NULL,
                NULL
                );
            Graph_SetTooltip(context->PingGraphHandle, TRUE);

            PhInitializeWorkQueue(&context->PingWorkQueue, 0, 20, 5000);
            PhInitializeGraphState(&context->PingGraphState);
            PhInitializeLayoutManager(&context->LayoutManager, hwndDlg);
            PhInitializeCircularBuffer_ULONG(&context->PingHistory, PhGetIntegerSetting(L"SampleCount"));

            PhAddLayoutItem(&context->LayoutManager, GetDlgItem(hwndDlg, IDC_ICMP_PANEL), NULL, PH_ANCHOR_BOTTOM | PH_ANCHOR_LEFT | PH_ANCHOR_RIGHT| PH_LAYOUT_FORCE_INVALIDATE);
            PhAddLayoutItem(&context->LayoutManager, GetDlgItem(hwndDlg, IDC_ICMP_AVG), NULL, PH_ANCHOR_BOTTOM | PH_ANCHOR_LEFT);
            PhAddLayoutItem(&context->LayoutManager, GetDlgItem(hwndDlg, IDC_ICMP_MIN), NULL, PH_ANCHOR_BOTTOM | PH_ANCHOR_LEFT);
            PhAddLayoutItem(&context->LayoutManager, GetDlgItem(hwndDlg, IDC_ICMP_MAX), NULL, PH_ANCHOR_BOTTOM | PH_ANCHOR_LEFT);
            PhAddLayoutItem(&context->LayoutManager, GetDlgItem(hwndDlg, IDC_PINGS_SENT), NULL, PH_ANCHOR_BOTTOM | PH_ANCHOR_LEFT);
            PhAddLayoutItem(&context->LayoutManager, GetDlgItem(hwndDlg, IDC_PINGS_LOST), NULL, PH_ANCHOR_BOTTOM | PH_ANCHOR_LEFT);
            PhAddLayoutItem(&context->LayoutManager, GetDlgItem(hwndDlg, IDC_BAD_HASH), NULL, PH_ANCHOR_BOTTOM | PH_ANCHOR_LEFT);
            PhAddLayoutItem(&context->LayoutManager, GetDlgItem(hwndDlg, IDC_ANON_ADDR), NULL, PH_ANCHOR_BOTTOM | PH_ANCHOR_LEFT);
            panelItem = PhAddLayoutItem(&context->LayoutManager, GetDlgItem(hwndDlg, IDC_PING_LAYOUT), NULL, PH_ANCHOR_ALL);
            PhAddLayoutItemEx(&context->LayoutManager, context->PingGraphHandle, NULL, PH_ANCHOR_ALL, panelItem->Margin);
            PhLayoutManagerLayout(&context->LayoutManager);

            if (PhGetIntegerPairSetting(SETTING_NAME_PING_WINDOW_POSITION).X != 0)
                PhLoadWindowPlacementFromSetting(SETTING_NAME_PING_WINDOW_POSITION, SETTING_NAME_PING_WINDOW_SIZE, hwndDlg);
            else
                PhCenterWindow(hwndDlg, PhMainWndHandle);

            PhSetWindowText(hwndDlg, PhaFormatString(L"Ping %s", context->IpAddressString)->Buffer);
            PhSetWindowText(context->StatusHandle, PhaFormatString(L"Pinging %s with %lu bytes of data...",
                context->IpAddressString,
                PhGetIntegerSetting(SETTING_NAME_PING_SIZE))->Buffer
                );

            PhRegisterCallback(
                PhGetGeneralCallback(GeneralCallbackProcessProviderUpdatedEvent),
                NetworkPingUpdateHandler,
                context,
                &context->ProcessesUpdatedRegistration
                );

            PhInitializeWindowTheme(hwndDlg, !!PhGetIntegerSetting(L"EnableThemeSupport"));
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
    case WM_DESTROY:
        {
            PhUnregisterCallback(
                PhGetGeneralCallback(GeneralCallbackProcessProviderUpdatedEvent),
                &context->ProcessesUpdatedRegistration
                );

            PhSaveWindowPlacementToSetting(
                SETTING_NAME_PING_WINDOW_POSITION,
                SETTING_NAME_PING_WINDOW_SIZE,
                hwndDlg
                );

            if (context->PingGraphHandle)
                DestroyWindow(context->PingGraphHandle);

            if (context->FontHandle)
                DeleteFont(context->FontHandle);

            PhDeleteWorkQueue(&context->PingWorkQueue);
            PhDeleteGraphState(&context->PingGraphState);
            PhDeleteLayoutManager(&context->LayoutManager);

            PhRemoveWindowContext(hwndDlg, PH_WINDOW_CONTEXT_DEFAULT);
            PhFree(context);

            PostQuitMessage(0);
        }
        break;
    case WM_SIZE:
        PhLayoutManagerLayout(&context->LayoutManager);
        break;
    case WM_SIZING:
        //PhResizingMinimumSize((PRECT)lParam, wParam, 420, 250);
        break;
    case WM_PING_UPDATE:
        {
            ULONG maxGraphHeight = 0;
            ULONG pingAvgValue = 0;

            NetworkPingUpdateGraph(context);

            for (ULONG i = 0; i < context->PingHistory.Count; i++)
            {
                maxGraphHeight = maxGraphHeight + PhGetItemCircularBuffer_ULONG(&context->PingHistory, i);
                pingAvgValue = maxGraphHeight / context->PingHistory.Count;
            }

            PhSetDialogItemText(hwndDlg, IDC_ICMP_AVG, PhaFormatString(
                L"Average: %lums", pingAvgValue)->Buffer);
            PhSetDialogItemText(hwndDlg, IDC_ICMP_MIN, PhaFormatString(
                L"Minimum: %lums", context->PingMinMs)->Buffer);
            PhSetDialogItemText(hwndDlg, IDC_ICMP_MAX, PhaFormatString(
                L"Maximum: %lums", context->PingMaxMs)->Buffer);

            PhSetDialogItemText(hwndDlg, IDC_PINGS_SENT, PhaFormatString(
                L"Pings sent: %lu", context->PingSentCount)->Buffer);
            PhSetDialogItemText(hwndDlg, IDC_PINGS_LOST, PhaFormatString(
                L"Pings lost: %lu (%.0f%%)", context->PingLossCount,
                ((DOUBLE)context->PingLossCount / context->PingSentCount * 100))->Buffer);

            //PhSetDialogItemText(hwndDlg, IDC_BAD_HASH, PhaFormatString(
            //    L"Bad hashes: %lu", context->HashFailCount)->Buffer);
            PhSetDialogItemText(hwndDlg, IDC_ANON_ADDR, PhaFormatString(
                L"Anon replies: %lu", context->UnknownAddrCount)->Buffer);
        }
        break;
    case WM_NOTIFY:
        {
            LPNMHDR header = (LPNMHDR)lParam;

            switch (header->code)
            {
            case GCN_GETDRAWINFO:
                {
                    PPH_GRAPH_GETDRAWINFO getDrawInfo = (PPH_GRAPH_GETDRAWINFO)header;
                    PPH_GRAPH_DRAW_INFO drawInfo = getDrawInfo->DrawInfo;

                    if (header->hwndFrom == context->PingGraphHandle)
                    {
                        if (PhGetIntegerSetting(L"GraphShowText"))
                        {
                            HDC hdc;

                            PhMoveReference(&context->PingGraphState.Text,
                                PhFormatString(L"%lu ms", context->CurrentPingMs)
                                );

                            hdc = Graph_GetBufferedContext(context->PingGraphHandle);
                            PhSetGraphText(hdc, drawInfo, &context->PingGraphState.Text->sr,
                                &NormalGraphTextMargin, &NormalGraphTextPadding, PH_ALIGN_TOP | PH_ALIGN_LEFT);
                        }
                        else
                        {
                            drawInfo->Text.Buffer = NULL;
                        }

                        drawInfo->Flags = PH_GRAPH_USE_GRID_X | PH_GRAPH_USE_GRID_Y;
                        PhSiSetColorsGraphDrawInfo(drawInfo, PhGetIntegerSetting(L"ColorCpuKernel"), 0);
                        PhGraphStateGetDrawInfo(&context->PingGraphState, getDrawInfo, context->PingHistory.Count);

                        if (!context->PingGraphState.Valid)
                        {
                            ULONG i;
                            FLOAT max = (FLOAT)context->MaxPingTimeout; // minimum scaling

                            for (i = 0; i < drawInfo->LineDataCount; i++)
                            {
                                FLOAT data1;

                                context->PingGraphState.Data1[i] = data1 = (FLOAT)PhGetItemCircularBuffer_ULONG(&context->PingHistory, i);

                                if (max < data1)
                                    max = data1;
                            }

                            // Scale the data.
                            PhDivideSinglesBySingle(
                                context->PingGraphState.Data1,
                                max,
                                drawInfo->LineDataCount
                                );

                            context->PingGraphState.Valid = TRUE;
                        }
                    }
                }
                break;
            case GCN_GETTOOLTIPTEXT:
                {
                    PPH_GRAPH_GETTOOLTIPTEXT getTooltipText = (PPH_GRAPH_GETTOOLTIPTEXT)lParam;

                    if (getTooltipText->Index < getTooltipText->TotalCount)
                    {
                        if (header->hwndFrom == context->PingGraphHandle)
                        {
                            if (context->PingGraphState.TooltipIndex != getTooltipText->Index)
                            {
                                ULONG pingMs = PhGetItemCircularBuffer_ULONG(&context->PingHistory, getTooltipText->Index);

                                PhMoveReference(&context->PingGraphState.TooltipText, PhFormatString(
                                    L"%lu ms\n%s", 
                                    pingMs,
                                    PH_AUTO_T(PH_STRING, PhGetStatisticsTimeString(NULL, getTooltipText->Index))->Buffer)
                                    );
                            }

                            getTooltipText->Text = context->PingGraphState.TooltipText->sr;
                        }
                    }
                }
                break;
            }
        }
        break;
    }

    return FALSE;
}

NTSTATUS NetworkPingDialogThreadStart(
    _In_ PVOID Parameter
    )
{
    BOOL result;
    MSG message;
    HWND windowHandle;
    PH_AUTO_POOL autoPool;

    PhInitializeAutoPool(&autoPool);

    windowHandle = CreateDialogParam(
        PluginInstance->DllBase,
        MAKEINTRESOURCE(IDD_PING),
        NULL,
        NetworkPingWndProc,
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

VOID ShowPingWindow(
    _In_ PPH_NETWORK_ITEM NetworkItem
    )
{
    PNETWORK_PING_CONTEXT context;

    context = (PNETWORK_PING_CONTEXT)PhAllocate(sizeof(NETWORK_PING_CONTEXT));
    memset(context, 0, sizeof(NETWORK_PING_CONTEXT));

    if (NetworkItem->RemoteEndpoint.Address.Type == PH_IPV4_NETWORK_TYPE)
        RtlIpv4AddressToString(&NetworkItem->RemoteEndpoint.Address.InAddr, context->IpAddressString);
    else
        RtlIpv6AddressToString(&NetworkItem->RemoteEndpoint.Address.In6Addr, context->IpAddressString);

    context->RemoteEndpoint = NetworkItem->RemoteEndpoint;

    PhCreateThread2(NetworkPingDialogThreadStart, (PVOID)context);
}
    
VOID ShowPingWindowFromAddress(
    _In_ PH_IP_ENDPOINT RemoteEndpoint
    )
{
    PNETWORK_PING_CONTEXT context;

    context = (PNETWORK_PING_CONTEXT)PhAllocate(sizeof(NETWORK_PING_CONTEXT));
    memset(context, 0, sizeof(NETWORK_PING_CONTEXT));

    if (RemoteEndpoint.Address.Type == PH_IPV4_NETWORK_TYPE)
    {
        RtlIpv4AddressToString(&RemoteEndpoint.Address.InAddr, context->IpAddressString);
    }
    else
    {
        RtlIpv6AddressToString(&RemoteEndpoint.Address.In6Addr, context->IpAddressString);
    }

    context->RemoteEndpoint = RemoteEndpoint;

    PhCreateThread2(NetworkPingDialogThreadStart, (PVOID)context);
}
