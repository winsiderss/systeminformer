/*
 * Copyright (c) 2022 Winsider Seminars & Solutions, Inc.  All rights reserved.
 *
 * This file is part of System Informer.
 *
 * Authors:
 *
 *     dmex    2015-2023
 *
 */

#include "nettools.h"
#include <math.h>

static RECT NormalGraphTextMargin = { 5, 5, 5, 5 };
static RECT NormalGraphTextPadding = { 3, 3, 3, 3 };

PPH_OBJECT_TYPE PingContextType = NULL;
PH_INITONCE PingContextTypeInitOnce = PH_INITONCE_INIT;

_Function_class_(PH_TYPE_DELETE_PROCEDURE)
VOID PingContextDeleteProcedure(
    _In_ PVOID Object,
    _In_ ULONG Flags
    )
{
    PNETWORK_PING_CONTEXT context = Object;

    PhDeleteCircularBuffer_FLOAT(&context->PingSuccessHistory);
    PhDeleteCircularBuffer_FLOAT(&context->PingFailureHistory);
}

PNETWORK_PING_CONTEXT CreatePingContext(
    VOID
    )
{
    PNETWORK_PING_CONTEXT context;

    if (PhBeginInitOnce(&PingContextTypeInitOnce))
    {
        PingContextType = PhCreateObjectType(L"PingContextObjectType", 0, PingContextDeleteProcedure);
        PhEndInitOnce(&PingContextTypeInitOnce);
    }

    context = PhCreateObject(sizeof(NETWORK_PING_CONTEXT), PingContextType);
    memset(context, 0, sizeof(NETWORK_PING_CONTEXT));

    return context;
}

NTSTATUS NetworkPingThreadStart(
    _In_ PVOID Parameter
    )
{
    PNETWORK_PING_CONTEXT context = (PNETWORK_PING_CONTEXT)Parameter;
    HANDLE icmpHandle = INVALID_HANDLE_VALUE;
    BOOLEAN icmpSuccess = FALSE;
    FLOAT icmpCurrentPingMs = 0.0f;
    ULONG icmpCurrentOverhead = 0;
    ULONG icmpReplyCount = 0;
    ULONG icmpReplyLength = 0;
    PVOID icmpReplyBuffer = NULL;
    ULONG icmpEchoBufferLength = 0;
    PPH_BYTES icmpEchoBuffer = NULL;
    PPH_STRING icmpRandString = NULL;
    LARGE_INTEGER performanceCounterStart;
    LARGE_INTEGER performanceCounterEnd;
    LARGE_INTEGER performanceCounterFrequency;
    FLOAT performanceCounterTime = 0.0f;
    IP_OPTION_INFORMATION pingOptions =
    {
        UCHAR_MAX,   // Time To Live
        0,           // Type Of Service
        IP_FLAG_DF,  // IP header flags
        0            // Size of options data
    };
    //pingOptions.Flags |= IP_FLAG_REVERSE;

    PhQueryPerformanceFrequency(&performanceCounterFrequency);
    icmpEchoBufferLength = PhGetIntegerSetting(SETTING_NAME_PING_SIZE);

    if (icmpRandString = PhCreateStringEx(NULL, icmpEchoBufferLength * sizeof(WCHAR) + sizeof(UNICODE_NULL)))
    {
        PhGenerateRandomAlphaString(icmpRandString->Buffer, icmpRandString->Length / sizeof(WCHAR));
        icmpEchoBuffer = PhConvertUtf16ToUtf8Ex(icmpRandString->Buffer, icmpRandString->Length - sizeof(UNICODE_NULL));
        PhDereferenceObject(icmpRandString);
    }

    if (!icmpEchoBuffer)
        goto CleanupExit;
    if (icmpEchoBuffer->Length != icmpEchoBufferLength)
        goto CleanupExit;

    if (context->RemoteEndpoint.Address.Type == PH_NETWORK_TYPE_IPV6)
    {
        SOCKADDR_IN6 icmp6LocalAddr = { 0 };
        SOCKADDR_IN6 icmp6RemoteAddr = { 0 };
        PICMPV6_ECHO_REPLY2 icmp6ReplyStruct = NULL;

        if ((icmpHandle = Icmp6CreateFile()) == INVALID_HANDLE_VALUE)
            goto CleanupExit;

        // Set Local IPv6-ANY address.
        icmp6LocalAddr.sin6_addr = in6addr_any;
        icmp6LocalAddr.sin6_family = AF_INET6;

        // Set Remote IPv6 address.
        icmp6RemoteAddr.sin6_addr = context->RemoteEndpoint.Address.In6Addr;
        //icmp6RemoteAddr.sin6_port = _byteswap_ushort((USHORT)context->NetworkItem->RemoteEndpoint.Port);

        // Allocate ICMPv6 message.
        icmpReplyLength = ICMP_BUFFER_SIZE(sizeof(ICMPV6_ECHO_REPLY), icmpEchoBuffer->Length);
        icmpReplyBuffer = PhAllocateZero(icmpReplyLength);

        InterlockedIncrement(&context->PingSentCount);
        PhQueryPerformanceCounter(&performanceCounterStart);

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
            context->Timeout
            );

        PhQueryPerformanceCounter(&performanceCounterEnd);
        icmp6ReplyStruct = (PICMPV6_ECHO_REPLY2)icmpReplyBuffer;

        performanceCounterTime = (FLOAT)(performanceCounterEnd.QuadPart - performanceCounterStart.QuadPart);
        performanceCounterTime *= 1000000;
        performanceCounterTime /= performanceCounterFrequency.QuadPart;
        performanceCounterTime /= 1000;

        if (icmpReplyCount && icmp6ReplyStruct->Status == IP_SUCCESS)
        {
            if (!RtlEqualMemory(
                icmp6ReplyStruct->Address.sin6_addr,
                icmp6RemoteAddr.sin6_addr.u.Word,
                sizeof(icmp6ReplyStruct->Address.sin6_addr)
                ))
            {
                InterlockedIncrement(&context->UnknownAddrCount);
            }

            if (!RtlEqualMemory(
                icmpEchoBuffer->Buffer,
                icmp6ReplyStruct->Data,
                icmpEchoBuffer->Length
                ))
            {
                InterlockedIncrement(&context->HashFailCount);
            }

            icmpSuccess = TRUE;
        }
        else
        {
            InterlockedIncrement(&context->PingLossCount);
        }

        if (performanceCounterTime != 0.0f)
            icmpCurrentOverhead = (ULONG)performanceCounterTime - icmp6ReplyStruct->RoundTripTime;
        icmpCurrentPingMs = performanceCounterTime - (FLOAT)icmpCurrentOverhead;
    }
    else
    {
        IPAddr icmpLocalAddr = 0;
        IPAddr icmpRemoteAddr = 0;
        BOOLEAN icmpPacketSignature = FALSE;
        PICMP_ECHO_REPLY icmpReplyStruct = NULL;

        if ((icmpHandle = IcmpCreateFile()) == INVALID_HANDLE_VALUE)
            goto CleanupExit;

        // Set Local IPv4-ANY address.
        icmpLocalAddr = in4addr_any.s_addr;

        // Set Remote IPv4 address.
        icmpRemoteAddr = context->RemoteEndpoint.Address.InAddr.s_addr;

        // Allocate ICMPv4 message.
        icmpReplyLength = ICMP_BUFFER_SIZE(sizeof(ICMP_ECHO_REPLY), icmpEchoBuffer->Length);
        icmpReplyBuffer = PhAllocateZero(icmpReplyLength);

        InterlockedIncrement(&context->PingSentCount);
        PhQueryPerformanceCounter(&performanceCounterStart);

        icmpReplyCount = IcmpSendEcho2Ex(
            icmpHandle,
            NULL,
            NULL,
            NULL,
            icmpLocalAddr,
            icmpRemoteAddr,
            icmpEchoBuffer->Buffer,
            (USHORT)icmpEchoBuffer->Length,
            &pingOptions,
            icmpReplyBuffer,
            icmpReplyLength,
            context->Timeout
            );

        PhQueryPerformanceCounter(&performanceCounterEnd);
        icmpReplyStruct = (PICMP_ECHO_REPLY)icmpReplyBuffer;

        performanceCounterTime = (FLOAT)(performanceCounterEnd.QuadPart - performanceCounterStart.QuadPart);
        performanceCounterTime *= 1000000;
        performanceCounterTime /= performanceCounterFrequency.QuadPart;
        performanceCounterTime /= 1000;

        if (icmpReplyCount && icmpReplyStruct->Status == IP_SUCCESS)
        {
            if (icmpReplyStruct->Address != icmpRemoteAddr)
            {
                InterlockedIncrement(&context->UnknownAddrCount);
            }

            if (icmpReplyStruct->DataSize == icmpEchoBuffer->Length)
            {
                icmpPacketSignature = RtlEqualMemory(
                    icmpEchoBuffer->Buffer,
                    icmpReplyStruct->Data,
                    icmpEchoBuffer->Length);
            }

            if (!icmpPacketSignature)
            {
                InterlockedIncrement(&context->HashFailCount);
            }

            icmpSuccess = TRUE;
        }
        else
        {
            InterlockedIncrement(&context->PingLossCount);
        }

        if (performanceCounterTime != 0.0f)
            icmpCurrentOverhead = (ULONG)performanceCounterTime - icmpReplyStruct->RoundTripTime;
        icmpCurrentPingMs = performanceCounterTime - (FLOAT)icmpCurrentOverhead;
    }

    if (context->PingMinMs == 0.0f || icmpCurrentPingMs < context->PingMinMs)
        context->PingMinMs = icmpCurrentPingMs;
    if (icmpCurrentPingMs > context->PingMaxMs)
        context->PingMaxMs = icmpCurrentPingMs;

    context->CurrentPingMs = icmpCurrentPingMs;

    InterlockedIncrement(&context->PingRecvCount);

    if (icmpSuccess)
    {
        PhAddItemCircularBuffer_FLOAT(&context->PingSuccessHistory, icmpCurrentPingMs);
        PhAddItemCircularBuffer_FLOAT(&context->PingFailureHistory, 0.0f);
    }
    else
    {
        PhAddItemCircularBuffer_FLOAT(&context->PingSuccessHistory, 0.0f);
        PhAddItemCircularBuffer_FLOAT(&context->PingFailureHistory, icmpCurrentPingMs);
    }

CleanupExit:

    if (icmpHandle && icmpHandle != INVALID_HANDLE_VALUE)
    {
        IcmpCloseHandle(icmpHandle);
    }

    if (icmpEchoBuffer)
    {
        PhDereferenceObject(icmpEchoBuffer);
    }

    if (icmpReplyBuffer)
    {
        PhFree(icmpReplyBuffer);
    }

    PostMessage(context->WindowHandle, WM_PH_UPDATE_DIALOG, 0, 0);
    PhDereferenceObject(context);

    return STATUS_SUCCESS;
}

_Function_class_(PH_CALLBACK_FUNCTION)
VOID NTAPI NetworkPingUpdateHandler(
    _In_opt_ PVOID Parameter,
    _In_ PVOID Context
    )
{
    PNETWORK_PING_CONTEXT context = (PNETWORK_PING_CONTEXT)Context;

    PhReferenceObject(context);
    PhQueueItemWorkQueue(&context->PingWorkQueue, NetworkPingThreadStart, (PVOID)context);
}

VOID NetworkPingUpdateGraph(
    _In_ PNETWORK_PING_CONTEXT Context
    )
{
    Context->PingGraphState.Valid = FALSE;
    Context->PingGraphState.TooltipIndex = ULONG_MAX;
    Graph_MoveGrid(Context->PingGraphHandle, 1);
    Graph_Draw(Context->PingGraphHandle);
    Graph_UpdateTooltip(Context->PingGraphHandle);
    InvalidateRect(Context->PingGraphHandle, NULL, FALSE);
}

PPH_STRING NetworkPingLabelYFunction(
    _In_ PPH_GRAPH_DRAW_INFO DrawInfo,
    _In_ ULONG DataIndex,
    _In_ FLOAT Value,
    _In_ FLOAT Parameter
    )
{
    FLOAT value;

    value = (FLOAT)(Parameter);

    if (value != 0)
    {
        PH_FORMAT format[2];

        PhInitFormatF(&format[0], value, 2); // (USHORT)PhGetIntegerSetting(L"MaxPrecisionUnit"));
        PhInitFormatS(&format[1], L" ms");

        return PhFormat(format, RTL_NUMBER_OF(format), 0);
    }
    else
    {
        return PhReferenceEmptyString();
    }
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
            context->WindowDpi = PhGetWindowDpi(hwndDlg);
            context->MinPingScaling = PhGetIntegerSetting(SETTING_NAME_PING_MINIMUM_SCALING);
            context->Timeout = PhGetIntegerSetting(SETTING_NAME_PING_TIMEOUT);
            context->FontHandle = PhCreateCommonFont(-15, FW_MEDIUM, context->StatusHandle, context->WindowDpi);
            context->PingGraphHandle = CreateWindow(
                PH_GRAPH_CLASSNAME,
                NULL,
                WS_VISIBLE | WS_CHILD | WS_BORDER,
                0,
                0,
                0,
                0,
                hwndDlg,
                NULL,
                PluginInstance->DllBase,
                NULL
                );
            Graph_SetTooltip(context->PingGraphHandle, TRUE);

            PhInitializeWorkQueue(&context->PingWorkQueue, 0, 20, 5000);
            PhInitializeGraphState(&context->PingGraphState);
            PhInitializeLayoutManager(&context->LayoutManager, hwndDlg);
            PhInitializeCircularBuffer_FLOAT(&context->PingSuccessHistory, PhGetIntegerSetting(L"SampleCount"));
            PhInitializeCircularBuffer_FLOAT(&context->PingFailureHistory, PhGetIntegerSetting(L"SampleCount"));

            PhAddLayoutItem(&context->LayoutManager, GetDlgItem(hwndDlg, IDC_ICMP_PANEL), NULL, PH_ANCHOR_BOTTOM | PH_ANCHOR_LEFT | PH_ANCHOR_RIGHT| PH_LAYOUT_FORCE_INVALIDATE);
            PhAddLayoutItem(&context->LayoutManager, GetDlgItem(hwndDlg, IDC_ICMP_AVG), NULL, PH_ANCHOR_BOTTOM | PH_ANCHOR_LEFT);
            PhAddLayoutItem(&context->LayoutManager, GetDlgItem(hwndDlg, IDC_ICMP_MIN), NULL, PH_ANCHOR_BOTTOM | PH_ANCHOR_LEFT);
            PhAddLayoutItem(&context->LayoutManager, GetDlgItem(hwndDlg, IDC_ICMP_MAX), NULL, PH_ANCHOR_BOTTOM | PH_ANCHOR_LEFT);
            PhAddLayoutItem(&context->LayoutManager, GetDlgItem(hwndDlg, IDC_PINGS_SENT), NULL, PH_ANCHOR_BOTTOM | PH_ANCHOR_LEFT);
            PhAddLayoutItem(&context->LayoutManager, GetDlgItem(hwndDlg, IDC_PINGS_LOST), NULL, PH_ANCHOR_BOTTOM | PH_ANCHOR_LEFT);
            PhAddLayoutItem(&context->LayoutManager, GetDlgItem(hwndDlg, IDC_ICMP_STDEV), NULL, PH_ANCHOR_BOTTOM | PH_ANCHOR_LEFT);
            PhAddLayoutItem(&context->LayoutManager, GetDlgItem(hwndDlg, IDC_BAD_HASH), NULL, PH_ANCHOR_BOTTOM | PH_ANCHOR_LEFT);
            PhAddLayoutItem(&context->LayoutManager, GetDlgItem(hwndDlg, IDC_ANON_ADDR), NULL, PH_ANCHOR_BOTTOM | PH_ANCHOR_LEFT);
            panelItem = PhAddLayoutItem(&context->LayoutManager, GetDlgItem(hwndDlg, IDC_PING_LAYOUT), NULL, PH_ANCHOR_ALL);
            PhAddLayoutItemEx(&context->LayoutManager, context->PingGraphHandle, NULL, PH_ANCHOR_ALL, &panelItem->Margin);
            PhLayoutManagerLayout(&context->LayoutManager);

            if (PhValidWindowPlacementFromSetting(SETTING_NAME_PING_WINDOW_POSITION))
                PhLoadWindowPlacementFromSetting(SETTING_NAME_PING_WINDOW_POSITION, SETTING_NAME_PING_WINDOW_SIZE, hwndDlg);
            else
                PhCenterWindow(hwndDlg, context->ParentWindowHandle);

            PhSetWindowText(hwndDlg, PhaFormatString(L"Ping %s", context->RemoteAddressString)->Buffer);
            PhSetWindowText(context->StatusHandle, PhaFormatString(L"Pinging %s with %lu bytes of data...",
                context->RemoteAddressString,
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
            PhDereferenceObject(context);

            PostQuitMessage(0);
        }
        break;
    case WM_SIZE:
        {
            PhLayoutManagerLayout(&context->LayoutManager);

            context->PingGraphState.Valid = FALSE;
            context->PingGraphState.TooltipIndex = ULONG_MAX;
            if (context->PingGraphHandle)
                Graph_Draw(context->PingGraphHandle);
        }
        break;
    case WM_SIZING:
        //PhResizingMinimumSize((PRECT)lParam, wParam, 420, 250);
        break;
    case WM_PH_UPDATE_DIALOG:
        {
            FLOAT pingSumValue = 0.0f;
            FLOAT pingAvgMeanValue = 0.0f;
            FLOAT pingDeviationValue = 0.0f;

            NetworkPingUpdateGraph(context);

            for (ULONG i = 0; i < context->PingSuccessHistory.Count; i++)
            {
                pingSumValue += PhGetItemCircularBuffer_FLOAT(&context->PingSuccessHistory, i);
            }

            if (context->PingSuccessHistory.Count)
            {
                pingAvgMeanValue = (FLOAT)pingSumValue / context->PingSuccessHistory.Count;
            }

            for (ULONG i = 0; i < context->PingSuccessHistory.Count; i++)
            {
                FLOAT pingHistoryValue = PhGetItemCircularBuffer_FLOAT(&context->PingSuccessHistory, i);

                pingDeviationValue += powf(pingHistoryValue - pingAvgMeanValue, 2);
            }

            if (context->PingSuccessHistory.Count)
            {
                FLOAT pingVarianceValue = pingDeviationValue / context->PingSuccessHistory.Count;

                pingDeviationValue = sqrtf(pingVarianceValue);
            }

            PhSetDialogItemText(hwndDlg, IDC_ICMP_AVG, PhaFormatString(
                L"Average: %.2f ms", pingAvgMeanValue)->Buffer);
            PhSetDialogItemText(hwndDlg, IDC_ICMP_MIN, PhaFormatString(
                L"Minimum: %.2f ms", context->PingMinMs)->Buffer);
            PhSetDialogItemText(hwndDlg, IDC_ICMP_MAX, PhaFormatString(
                L"Maximum: %.2f ms", context->PingMaxMs)->Buffer);

            PhSetDialogItemText(hwndDlg, IDC_PINGS_SENT, PhaFormatString(
                L"Pings sent: %lu", context->PingSentCount)->Buffer);
            PhSetDialogItemText(hwndDlg, IDC_PINGS_LOST, PhaFormatString(
                L"Pings lost: %lu (%.0f%%)", context->PingLossCount,
                ((FLOAT)context->PingLossCount / context->PingSentCount * 100))->Buffer);

            PhSetDialogItemText(hwndDlg, IDC_ICMP_STDEV, PhaFormatString(
                L"Deviation: %.2f ms", pingDeviationValue)->Buffer);
            //PhSetDialogItemText(hwndDlg, IDC_ICMP_STVAR, PhaFormatString(
            //    L"Variance: %.2f ms", pingVarianceValue)->Buffer);

            PhSetDialogItemText(hwndDlg, IDC_BAD_HASH, PhaFormatString(
                L"Bad replies: %lu", context->HashFailCount)->Buffer);
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
                        drawInfo->Flags = PH_GRAPH_USE_GRID_X | PH_GRAPH_USE_GRID_Y | PH_GRAPH_USE_LINE_2 | PH_GRAPH_LABEL_MAX_Y;
                        PhSiSetColorsGraphDrawInfo(drawInfo, PhGetIntegerSetting(L"ColorCpuKernel"), PhGetIntegerSetting(L"ColorCpuUser"), context->WindowDpi);
                        PhGraphStateGetDrawInfo(&context->PingGraphState, getDrawInfo, context->PingSuccessHistory.Count);

                        if (!context->PingGraphState.Valid)
                        {
                            PhCopyCircularBuffer_FLOAT(&context->PingSuccessHistory, context->PingGraphState.Data1, drawInfo->LineDataCount);
                            PhCopyCircularBuffer_FLOAT(&context->PingFailureHistory, context->PingGraphState.Data2, drawInfo->LineDataCount);

                            {
                                FLOAT max = 0;

                                if (PhGetIntegerSetting(L"EnableAvxSupport") && drawInfo->LineDataCount > 128)
                                {
                                    max = PhAddPlusMaxMemorySingles(
                                        context->PingGraphState.Data1,
                                        context->PingGraphState.Data2,
                                        drawInfo->LineDataCount
                                        );
                                }
                                else
                                {
                                    for (ULONG i = 0; i < drawInfo->LineDataCount; i++)
                                    {
                                        FLOAT data = context->PingGraphState.Data1[i] + context->PingGraphState.Data2[i];

                                        if (max < data)
                                            max = data;
                                    }
                                }

                                if (max != 0)
                                {
                                    FLOAT min = max;

                                    if (!PhGetIntegerSetting(L"EnableGraphMaxScale"))
                                    {
                                        if (min < (FLOAT)context->MinPingScaling)
                                            min = (FLOAT)context->MinPingScaling;
                                    }

                                    PhDivideSinglesBySingle(context->PingGraphState.Data1, min, drawInfo->LineDataCount);
                                    PhDivideSinglesBySingle(context->PingGraphState.Data2, min, drawInfo->LineDataCount);
                                }

                                drawInfo->LabelYFunction = NetworkPingLabelYFunction;
                                drawInfo->LabelYFunctionParameter = max;
                            }

                            context->PingGraphState.Valid = TRUE;
                        }

                        if (PhGetIntegerSetting(L"GraphShowText"))
                        {
                            HDC hdc;
                            PH_FORMAT format[2];

                            // %.2f ms
                            PhInitFormatF(&format[0], context->CurrentPingMs, 2);
                            PhInitFormatS(&format[1], L" ms");

                            PhMoveReference(&context->PingGraphState.Text, PhFormat(format, RTL_NUMBER_OF(format), 0));

                            hdc = Graph_GetBufferedContext(context->PingGraphHandle);
                            PhSetGraphText(hdc, drawInfo, &context->PingGraphState.Text->sr,
                                &NormalGraphTextMargin, &NormalGraphTextPadding, PH_ALIGN_TOP | PH_ALIGN_LEFT);
                        }
                        else
                        {
                            drawInfo->Text.Buffer = NULL;
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
                                FLOAT pingMs;
                                PH_FORMAT format[3];

                                if (!(pingMs = PhGetItemCircularBuffer_FLOAT(&context->PingSuccessHistory, getTooltipText->Index)))
                                    pingMs = PhGetItemCircularBuffer_FLOAT(&context->PingFailureHistory, getTooltipText->Index);

                                // %.2f ms\n%s
                                PhInitFormatF(&format[0], pingMs, 2);
                                PhInitFormatS(&format[1], L" ms\n");
                                PhInitFormatSR(&format[2], PH_AUTO_T(PH_STRING, PhGetStatisticsTimeString(NULL, getTooltipText->Index))->sr);

                                PhMoveReference(&context->PingGraphState.TooltipText, PhFormat(format, RTL_NUMBER_OF(format), 0));
                            }

                            getTooltipText->Text = PhGetStringRef(context->PingGraphState.TooltipText);
                        }
                    }
                }
                break;
            }
        }
        break;
    case WM_DPICHANGED:
        {
            context->WindowDpi = PhGetWindowDpi(hwndDlg);
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

NTSTATUS NetworkPingDialogThreadStart(
    _In_ PVOID Parameter
    )
{
    BOOL result;
    MSG message;
    HWND windowHandle;
    PH_AUTO_POOL autoPool;

    PhInitializeAutoPool(&autoPool);

    windowHandle = PhCreateDialog(
        PluginInstance->DllBase,
        MAKEINTRESOURCE(IDD_PING),
        NULL,
        NetworkPingWndProc,
        Parameter
        );

    ShowWindow(windowHandle, SW_SHOW);
    SetForegroundWindow(windowHandle);

    while (result = GetMessage(&message, NULL, 0, 0))
    {
        if (result == INT_ERROR)
            break;

        if (!IsDialogMessage(windowHandle, &message))
        {
            TranslateMessage(&message);
            DispatchMessage(&message);
        }

        if ((
            message.hwnd == windowHandle ||
            message.hwnd && IsChild(windowHandle, message.hwnd)) &&
            message.message == WM_KEYDOWN
            )
        {
            if (message.wParam == VK_F5)
            {
                SystemInformer_Refresh();  // forward key messages (dmex)
            }
        }

        PhDrainAutoPool(&autoPool);
    }

    PhDeleteAutoPool(&autoPool);

    return STATUS_SUCCESS;
}

VOID ShowPingWindow(
    _In_ HWND ParentWindowHandle,
    _In_ PPH_NETWORK_ITEM NetworkItem
    )
{
    PNETWORK_PING_CONTEXT context;

    context = CreatePingContext();
    context->ParentWindowHandle = ParentWindowHandle;

    memcpy_s(
        &context->RemoteEndpoint,
        sizeof(context->RemoteEndpoint),
        &NetworkItem->RemoteEndpoint,
        sizeof(NetworkItem->RemoteEndpoint)
        );

    if (NetworkItem->RemoteEndpoint.Address.Type == PH_NETWORK_TYPE_IPV4)
    {
        ULONG remoteAddressStringLength = RTL_NUMBER_OF(context->RemoteAddressString);

        if (NT_SUCCESS(PhIpv4AddressToString(
            &NetworkItem->RemoteEndpoint.Address.InAddr,
            0,
            context->RemoteAddressString,
            &remoteAddressStringLength
            )))
        {
            context->RemoteAddressStringLength = (remoteAddressStringLength - 1) * sizeof(WCHAR);
        }
    }
    else
    {
        ULONG remoteAddressStringLength = RTL_NUMBER_OF(context->RemoteAddressString);

        if (NT_SUCCESS(RtlIpv6AddressToStringEx(
            &NetworkItem->RemoteEndpoint.Address.In6Addr,
            0,
            0,
            context->RemoteAddressString,
            &remoteAddressStringLength
            )))
        {
            context->RemoteAddressStringLength = (remoteAddressStringLength - 1) * sizeof(WCHAR);
        }
    }

    PhCreateThread2(NetworkPingDialogThreadStart, context);
}

VOID ShowPingWindowFromAddress(
    _In_ HWND ParentWindowHandle,
    _In_ PH_IP_ENDPOINT RemoteEndpoint
    )
{
    PNETWORK_PING_CONTEXT context;

    context = CreatePingContext();
    context->ParentWindowHandle = ParentWindowHandle;

    memcpy_s(
        &context->RemoteEndpoint,
        sizeof(context->RemoteEndpoint),
        &RemoteEndpoint,
        sizeof(RemoteEndpoint)
        );

    if (RemoteEndpoint.Address.Type == PH_NETWORK_TYPE_IPV4)
    {
        ULONG remoteAddressStringLength = RTL_NUMBER_OF(context->RemoteAddressString);

        if (NT_SUCCESS(PhIpv4AddressToString(
            &RemoteEndpoint.Address.InAddr,
            0,
            context->RemoteAddressString,
            &remoteAddressStringLength
            )))
        {
            context->RemoteAddressStringLength = (remoteAddressStringLength - 1) * sizeof(WCHAR);
        }
    }
    else
    {
        ULONG remoteAddressStringLength = RTL_NUMBER_OF(context->RemoteAddressString);

        if (NT_SUCCESS(RtlIpv6AddressToStringEx(
            &RemoteEndpoint.Address.In6Addr,
            0,
            0,
            context->RemoteAddressString,
            &remoteAddressStringLength
            )))
        {
            context->RemoteAddressStringLength = (remoteAddressStringLength - 1) * sizeof(WCHAR);
        }
    }

    PhCreateThread2(NetworkPingDialogThreadStart, context);
}
