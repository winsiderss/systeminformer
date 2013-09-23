/*
 * Process Hacker Network Tools -
 *   Ping dialog
 *
 * Copyright (C) 2013 dmex
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

#include <winsock2.h>
#include <ws2tcpip.h>
#include <iphlpapi.h>
#include <icmpapi.h>

#pragma comment(lib, "iphlpapi.lib")
#pragma comment(lib, "ws2_32.lib")

#define WM_PING_UPDATE (WM_APP + 151)
static RECT NormalGraphTextMargin = { 5, 5, 5, 5 };
static RECT NormalGraphTextPadding = { 3, 3, 3, 3 };

static HFONT InitializeFont(
    __in HWND hwndDlg
    )
{
    LOGFONT logFont = { 0 };
    HFONT fontHandle = NULL;

    logFont.lfHeight = 20;
    logFont.lfWeight = FW_MEDIUM;
    logFont.lfQuality = CLEARTYPE_QUALITY | ANTIALIASED_QUALITY;

    wcscpy_s(logFont.lfFaceName, _countof(logFont.lfFaceName),
        WindowsVersion > WINDOWS_XP ? L"Segoe UI" : L"MS Shell Dlg 2"
        );

    fontHandle = CreateFontIndirect(&logFont);

    if (fontHandle)
    {
        SendMessage(hwndDlg, WM_SETFONT, (WPARAM)fontHandle, FALSE);
        return fontHandle;
    }

    return NULL;
}

static NTSTATUS PhPingNetworkPingThreadStart(
    __in PVOID Parameter
    )
{
    HANDLE icmpHandle = INVALID_HANDLE_VALUE;
    ULONG icmpReplyLength = 0;       
    ULONG icmpReplyCount = 0;
    ULONG icmpMaxPingTimeout = 0;
    LONG64 icmpCurrentPingMs = 0;
    PVOID icmpReplyBuffer = NULL;
    PNETWORK_OUTPUT_CONTEXT context = NULL;
    IP_OPTION_INFORMATION pingOptions = 
    { 
        255, 0, IP_FLAG_DF, 0, NULL 
    };
    
    icmpMaxPingTimeout = __max(PhGetIntegerSetting(L"ProcessHacker.NetTools.MaxPingTimeout"), 1);

    //Ping statistics for %s:
    //    Packets: Sent = 4, Received = 4, Lost = 0 (0% loss),
    //Approximate round trip times in milli-seconds:
    //    Minimum = 202ms, Maximum = 207ms, Average = 204ms

    __try
    {
        context = (PNETWORK_OUTPUT_CONTEXT)Parameter;
        if (context == NULL)
            __leave;

        if (context->IpAddress.Type == PH_IPV6_NETWORK_TYPE)
        {          
            SOCKADDR_IN6 icmp6LocalAddr = { 0 };
            SOCKADDR_IN6 icmp6RemoteAddr = { 0 };
            PICMPV6_ECHO_REPLY icmp6ReplyStruct = NULL;

            // Create ICMPv6 handle.
            if ((icmpHandle = Icmp6CreateFile()) == INVALID_HANDLE_VALUE)
                __leave;

            // Set Local IPv6 address.
            icmp6LocalAddr.sin6_addr = in6addr_any;
            icmp6LocalAddr.sin6_family = AF_INET6;

            // Set Remote IPv6 address.
            icmp6RemoteAddr.sin6_addr = context->IpAddress.In6Addr;
            icmp6RemoteAddr.sin6_port = _byteswap_ushort((USHORT)context->NetworkItem->RemoteEndpoint.Port);

            // Allocate ICMPv6 Ping buffer.
            icmpReplyLength = sizeof(ICMPV6_ECHO_REPLY);
            icmpReplyBuffer = PhAllocate(icmpReplyLength);
            memset(icmpReplyBuffer, 0, icmpReplyLength);

            InterlockedIncrement64(&context->PingSentCount);

            // Send ICMPv6 ping...
            icmpReplyCount = Icmp6SendEcho2(            
                icmpHandle,
                NULL,
                NULL,
                NULL, 
                &icmp6LocalAddr,
                &icmp6RemoteAddr,
                NULL, 
                0,
                &pingOptions,
                icmpReplyBuffer,
                icmpReplyLength,
                icmpMaxPingTimeout * 1000
                );

            icmp6ReplyStruct = (PICMPV6_ECHO_REPLY)icmpReplyBuffer;
            if (icmpReplyCount > 0 && icmp6ReplyStruct)
            {
                //if (icmpReplyStruct->Address.sin6_addr == context->IpAddress.In6Addr) 
                //if (icmpReplyStruct->DataSize < max_size)
                //if (icmpReplyStruct->RoundTripTime < 1)
                
                if (icmp6ReplyStruct->Status != IP_SUCCESS)
                {
                    InterlockedIncrement64(&context->PingLossCount);
                }

                icmpCurrentPingMs = icmp6ReplyStruct->RoundTripTime;
            }
            else
            {
                InterlockedIncrement64(&context->PingLossCount);
            }
        }
        else
        {
            IPAddr icmpSourceAddr = 0;
            PICMP_ECHO_REPLY icmpReplyStruct = NULL;
            
            // Create ICMPv4 handle.
            if ((icmpHandle = IcmpCreateFile()) == INVALID_HANDLE_VALUE)
                __leave;

            // Set Remote IPv4 address.
            icmpSourceAddr = context->IpAddress.InAddr.S_un.S_addr;

            // Allocate ICMPv4 Ping buffer.
            icmpReplyLength = sizeof(ICMP_ECHO_REPLY);
            icmpReplyBuffer = PhAllocate(icmpReplyLength);
            memset(icmpReplyBuffer, 0, icmpReplyLength);
   
            InterlockedIncrement64(&context->PingSentCount);

            // Send ICMPv4 ping...
            icmpReplyCount = IcmpSendEcho2(
                icmpHandle,
                NULL,
                NULL,
                NULL, 
                icmpSourceAddr,
                NULL, 
                0,
                &pingOptions,
                icmpReplyBuffer,
                icmpReplyLength,
                icmpMaxPingTimeout * 1000
                );

            icmpReplyStruct = (PICMP_ECHO_REPLY)icmpReplyBuffer;

            if (icmpReplyCount > 0 && icmpReplyStruct)
            {
                //if (icmpReplyStruct->Address == context->Address.InAddr.S_un.S_addr) 
                //if (icmpReplyStruct->DataSize < max_size)
                //if (icmpReplyStruct->RoundTripTime < 1)
               
                if (icmpReplyStruct->Status != IP_SUCCESS)
                {
                    InterlockedIncrement64(&context->PingLossCount);
                }

                icmpCurrentPingMs = icmpReplyStruct->RoundTripTime;
            }
            else
            {
                InterlockedIncrement64(&context->PingLossCount);
            }
        }

        InterlockedIncrement64(&context->PingRecvCount);

        if (context->PingMinMs == 0 || icmpCurrentPingMs < context->PingMinMs)
            context->PingMinMs = icmpCurrentPingMs;
        if (icmpCurrentPingMs > context->PingMaxMs)
            context->PingMaxMs = icmpCurrentPingMs;

        context->CurrentPingMs = icmpCurrentPingMs;

        PhAddItemCircularBuffer_ULONG64(&context->PingHistory, (ULONG64)icmpCurrentPingMs);                
    }
    __finally
    {
        if (icmpReplyBuffer)
        {
            PhFree(icmpReplyBuffer);
        }

        if (icmpHandle)
        {
            IcmpCloseHandle(icmpHandle);
        }
    }

    return STATUS_SUCCESS;
}

static VOID NTAPI NetworkPingUpdateHandler(
    __in_opt PVOID Parameter,
    __in_opt PVOID Context
    )
{
    PNETWORK_OUTPUT_CONTEXT context = (PNETWORK_OUTPUT_CONTEXT)Context;
    HANDLE dialogThread = INVALID_HANDLE_VALUE;

    // Queue up the next ping...
    // This needs to be done to prevent slow-links from blocking the interval update.
    if (dialogThread = PhCreateThread(0, (PUSER_THREAD_START_ROUTINE)PhPingNetworkPingThreadStart, (PVOID)context))
        NtClose(dialogThread);

    PostMessage(context->WindowHandle, WM_PING_UPDATE, 0, 0);
}

static INT_PTR CALLBACK NetworkPingWndProc(
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
            if (context->PingGraphHandle)
            {
                DestroyWindow(context->PingGraphHandle);
                context->PingGraphHandle = NULL;
            }

            PhSaveWindowPlacementToSetting(
                L"ProcessHacker.NetTools.NetToolsPingWindowPosition", 
                L"ProcessHacker.NetTools.NetToolsPingWindowSize", 
                hwndDlg
                ); 
            PhDeleteLayoutManager(&context->LayoutManager);
            RemoveProp(hwndDlg, L"Context");
            context = NULL;
        }
    }

    if (context == NULL)
        return FALSE;

    switch (uMsg)
    {
    case WM_INITDIALOG:
        {
            PH_RECTANGLE windowRectangle;
            PPH_LAYOUT_ITEM panelItem;

            windowRectangle.Position = PhGetIntegerPairSetting(L"ProcessHacker.NetTools.NetToolsPingWindowPosition");
            windowRectangle.Size = PhGetIntegerPairSetting(L"ProcessHacker.NetTools.NetToolsPingWindowSize");

            InitializeFont(GetDlgItem(hwndDlg, IDC_MAINTEXT));
            PhInitializeGraphState(&context->PingGraphState);
            PhInitializeLayoutManager(&context->LayoutManager, hwndDlg);
            PhInitializeCircularBuffer_ULONG64(&context->PingHistory, PhGetIntegerSetting(L"SampleCount"));

            PhAddLayoutItem(&context->LayoutManager, GetDlgItem(hwndDlg, IDC_STATIC3), NULL, PH_ANCHOR_BOTTOM | PH_ANCHOR_LEFT);
            PhAddLayoutItem(&context->LayoutManager, GetDlgItem(hwndDlg, IDC_STATIC5), NULL, PH_ANCHOR_BOTTOM | PH_ANCHOR_LEFT);
            PhAddLayoutItem(&context->LayoutManager, GetDlgItem(hwndDlg, IDC_STATIC6), NULL, PH_ANCHOR_BOTTOM | PH_ANCHOR_LEFT);
            PhAddLayoutItem(&context->LayoutManager, GetDlgItem(hwndDlg, IDC_STATIC7), NULL, PH_ANCHOR_BOTTOM | PH_ANCHOR_LEFT);
            PhAddLayoutItem(&context->LayoutManager, GetDlgItem(hwndDlg, IDC_STATIC8), NULL, PH_ANCHOR_BOTTOM | PH_ANCHOR_LEFT);
            PhAddLayoutItem(&context->LayoutManager, GetDlgItem(hwndDlg, IDOK), NULL, PH_ANCHOR_BOTTOM | PH_ANCHOR_RIGHT);
         
            panelItem = PhAddLayoutItem(&context->LayoutManager, GetDlgItem(hwndDlg, IDC_PING_LAYOUT), NULL, PH_ANCHOR_ALL);      
            context->PingGraphHandle = CreateWindow(
                PH_GRAPH_CLASSNAME,
                NULL,
                WS_VISIBLE | WS_CHILD | WS_BORDER,
                0,
                0,
                3,
                3,
                hwndDlg,
                (HMENU)9001,
                (HINSTANCE)PluginInstance->DllBase,
                NULL
                );
            Graph_SetTooltip(context->PingGraphHandle, TRUE);
            PhAddLayoutItemEx(&context->LayoutManager, context->PingGraphHandle, NULL, PH_ANCHOR_ALL, panelItem->Margin);

            // Load window settings.
            if (windowRectangle.Position.X == 0 || windowRectangle.Position.Y == 0)
                PhCenterWindow(hwndDlg, GetParent(hwndDlg));
            else
            {
                PhLoadWindowPlacementFromSetting(
                    L"ProcessHacker.NetTools.NetToolsPingWindowPosition", 
                    L"ProcessHacker.NetTools.NetToolsPingWindowSize", 
                    hwndDlg
                    );
            }
                  
            // Convert IP Address to string format.
            if (context->IpAddress.Type == PH_IPV4_NETWORK_TYPE)
            {
                RtlIpv4AddressToString(
                    &context->IpAddress.InAddr, 
                    context->addressString
                    );
            }
            else
            {
                RtlIpv6AddressToString(
                    &context->IpAddress.In6Addr, 
                    context->addressString
                    );
            }

            SetWindowText(hwndDlg, PhaFormatString(L"Ping (%s)", context->addressString)->Buffer);
            SetWindowText(GetDlgItem(hwndDlg, IDC_MAINTEXT), PhaFormatString(L"Pinging %s with 0 bytes of data:", context->addressString)->Buffer);
                        
            PhRegisterCallback(
                PhGetGeneralCallback(GeneralCallbackProcessesUpdated),
                NetworkPingUpdateHandler,
                context,
                &context->ProcessesUpdatedRegistration
                );
        }
        return TRUE;
    case WM_COMMAND:
        {
            switch (LOWORD(wParam))
            {
            case IDCANCEL:
            case IDOK:
                PostQuitMessage(0);
                break;
            }
        }
        break;
    case WM_DESTROY:
        {
            PhUnregisterCallback(
                PhGetGeneralCallback(GeneralCallbackProcessesUpdated), 
                &context->ProcessesUpdatedRegistration
                );
        }
        break;
    case WM_SIZE:
        PhLayoutManagerLayout(&context->LayoutManager);
        break;
    case WM_SIZING:
        PhResizingMinimumSize((PRECT)lParam, wParam, 100, 100);
        break;
    case WM_CTLCOLORBTN:
    case WM_CTLCOLORDLG:
    case WM_CTLCOLORSTATIC:
        {
            HDC hDC = (HDC)wParam;
            HWND hwndChild = (HWND)lParam;

            // Check for our static label and change the color.
            if (GetDlgCtrlID(hwndChild) == IDC_MAINTEXT)
            {
                SetTextColor(hDC, RGB(19, 112, 171));
            }

            // Set a transparent background for the control backcolor.
            SetBkMode(hDC, TRANSPARENT);

            // set window background color.
            return (INT_PTR)GetSysColorBrush(COLOR_WINDOW);
        }       
        break;
    case WM_PING_UPDATE:
        {
            context->PingGraphState.Valid = FALSE;
            context->PingGraphState.TooltipIndex = -1;
            Graph_MoveGrid(context->PingGraphHandle, 1);
            Graph_Draw(context->PingGraphHandle);
            Graph_UpdateTooltip(context->PingGraphHandle);
            InvalidateRect(context->PingGraphHandle, NULL, FALSE);
      
            //SetDlgItemText(hwndDlg, IDC_STATIC2, PhaFormatString(
            //    L"Sent: %I64u", context->PingSentCount)->Buffer);
            SetDlgItemText(hwndDlg, IDC_STATIC5, PhaFormatString(
                L"Loss: %I64u (0%% loss)", context->PingLossCount)->Buffer);
            SetDlgItemText(hwndDlg, IDC_STATIC6, PhaFormatString(
                L"Minimum: %I64ums", context->PingMinMs)->Buffer);
            SetDlgItemText(hwndDlg, IDC_STATIC7, PhaFormatString(
                L"Maximum: %I64ums", context->PingMaxMs)->Buffer);
            SetDlgItemText(hwndDlg, IDC_STATIC8, PhaFormatString(
                L"Average: %I64ums", context->PingAvgMs)->Buffer);
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

                    if (!!PhGetIntegerSetting(L"GraphColorMode"))
                    {                      
                        drawInfo->BackColor = RGB(0x00, 0x00, 0x00);
                        drawInfo->LineColor1 = PhGetIntegerSetting(L"ColorCpuKernel");
                        drawInfo->LineBackColor1 = PhHalveColorBrightness(drawInfo->LineColor1);
                        drawInfo->LineColor2 = PhGetIntegerSetting(L"ColorCpuUser");
                        drawInfo->LineBackColor2 = PhHalveColorBrightness(drawInfo->LineColor2);
                        drawInfo->GridColor = RGB(0x00, 0x57, 0x00);
                        drawInfo->TextColor = RGB(0x00, 0xff, 0x00);
                        drawInfo->TextBoxColor = RGB(0x00, 0x22, 0x00);
                    }
                    else
                    {              
                        drawInfo->BackColor = RGB(0xef, 0xef, 0xef);
                        drawInfo->LineColor1 = PhHalveColorBrightness(PhGetIntegerSetting(L"ColorCpuKernel"));
                        drawInfo->LineBackColor1 = PhMakeColorBrighter(drawInfo->LineColor1, 125);
                        drawInfo->LineColor2 = PhHalveColorBrightness(PhGetIntegerSetting(L"ColorCpuUser"));
                        drawInfo->LineBackColor2 = PhMakeColorBrighter(drawInfo->LineColor2, 125);
                        drawInfo->GridColor = RGB(0xc7, 0xc7, 0xc7);
                        drawInfo->TextColor = RGB(0x00, 0x00, 0x00);
                        drawInfo->TextBoxColor = RGB(0xe7, 0xe7, 0xe7);
                    }

                    if (header->hwndFrom == context->PingGraphHandle)
                    {
                        if (PhGetIntegerSetting(L"GraphShowText"))
                        {
                            HDC hdc = Graph_GetBufferedContext(context->PingGraphHandle);

                            PhSwapReference2(&context->PingGraphState.TooltipText, 
                                PhFormatString(L"Ping: %I64ums", context->CurrentPingMs)
                                );

                            SelectObject(hdc, PhApplicationFont);
                            PhSetGraphText(hdc, drawInfo, &context->PingGraphState.TooltipText->sr,
                                &NormalGraphTextMargin, &NormalGraphTextPadding, PH_ALIGN_TOP | PH_ALIGN_LEFT);
                        }
                        else
                        {
                            drawInfo->Text.Buffer = NULL;
                        }

                        PhGraphStateGetDrawInfo(
                            &context->PingGraphState,
                            getDrawInfo,
                            context->PingHistory.Count
                            );

                        if (!context->PingGraphState.Valid)
                        {
                            ULONG i = 0;

                            for (i = 0; i < drawInfo->LineDataCount; i++)
                            {
                                context->PingGraphState.Data1[i] =
                                    (FLOAT)PhGetItemCircularBuffer_ULONG64(&context->PingHistory, i);
                            }

                            // Scale the data.
                            PhxfDivideSingle2U(
                                context->PingGraphState.Data1,
                                (FLOAT)context->PingMaxMs + 5,
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
                                ULONG64 pingMs = PhGetItemCircularBuffer_ULONG64(&context->PingHistory, getTooltipText->Index);

                                PhSwapReference2(&context->PingGraphState.TooltipText, 
                                    PhFormatString(L"Ping: %I64ums", pingMs)
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

NTSTATUS PhNetworkPingDialogThreadStart(
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
        MAKEINTRESOURCE(IDD_PINGDIALOG),
        PhMainWndHandle,
        NetworkPingWndProc,
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