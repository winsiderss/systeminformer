/*
 * Copyright (c) 2022 Winsider Seminars & Solutions, Inc.  All rights reserved.
 *
 * This file is part of System Informer.
 *
 * Authors:
 *
 *     dmex    2018-2022
 *
 */

#include "devices.h"

#define GRAPH_PADDING 5
static RECT NormalGraphTextMargin = { 5, 5, 5, 5 };
static RECT NormalGraphTextPadding = { 3, 3, 3, 3 };

INT_PTR CALLBACK GraphicsDeviceNodesDlgProc(
    _In_ HWND WindowHandle,
    _In_ UINT WindowMessage,
    _In_ WPARAM wParam,
    _In_ LPARAM lParam
    );

_Function_class_(PH_GRAPH_MESSAGE_CALLBACK)
BOOLEAN GraphicsDeviceNodesMessageCallback(
    _In_ HWND WindowHandle,
    _In_ ULONG Message,
    _In_ PVOID Parameter1,
    _In_ PVOID Parameter2,
    _In_ PVOID Context
    );

_Function_class_(USER_THREAD_START_ROUTINE)
NTSTATUS EtpGpuNodesDialogThreadStart(
    _In_ PDV_GPU_SYSINFO_CONTEXT Context
    )
{
    BOOL result;
    MSG message;
    PH_AUTO_POOL autoPool;

    PhInitializeAutoPool(&autoPool);

    Context->NodeWindowHandle = PhCreateDialog(
        PluginInstance->DllBase,
        MAKEINTRESOURCE(IDD_GPUDEVICE_NODES),
        NULL,
        GraphicsDeviceNodesDlgProc,
        Context
        );

    PhSetEvent(&Context->NodeWindowInitializedEvent);

    while (result = GetMessage(&message, NULL, 0, 0))
    {
        if (result == INT_ERROR)
            break;

        if (!IsDialogMessage(Context->NodeWindowHandle, &message))
        {
            TranslateMessage(&message);
            DispatchMessage(&message);
        }

        PhDrainAutoPool(&autoPool);
    }

    PhDeleteAutoPool(&autoPool);

    if (Context->NodeWindowThreadHandle)
        NtClose(Context->NodeWindowThreadHandle);

    Context->NodeWindowThreadHandle = NULL;
    Context->NodeWindowHandle = NULL;

    PhResetEvent(&Context->NodeWindowInitializedEvent);

    PhDereferenceObject(Context);

    return STATUS_SUCCESS;
}

VOID GraphicsDeviceShowNodesDialog(
    _In_ PDV_GPU_SYSINFO_CONTEXT Context,
    _In_ HWND ParentWindowHandle
    )
{
    if (Context->DeviceEntry->NumberOfNodes == 0)
    {
        PhShowError2(ParentWindowHandle, L"There are no graphics nodes to display.", L"%s", L"");
        return;
    }

    if (!Context->NodeWindowThreadHandle)
    {
        PhReferenceObject(Context);

        if (!NT_SUCCESS(PhCreateThreadEx(&Context->NodeWindowThreadHandle, EtpGpuNodesDialogThreadStart, Context)))
        {
            PhShowError2(ParentWindowHandle, L"Unable to create the window.", L"%s", L"");
            PhDereferenceObject(Context);
            return;
        }

        PhWaitForEvent(&Context->NodeWindowInitializedEvent, NULL);
    }

    PostMessage(Context->NodeWindowHandle, WM_PH_SHOW_DIALOG, 0, 0);
}

_Function_class_(PH_CALLBACK_FUNCTION)
static VOID ProcessesUpdatedCallback(
    _In_opt_ PVOID Parameter,
    _In_opt_ PVOID Context
    )
{
    PDV_GPU_NODES_WINDOW_CONTEXT context = Context;
    if (!context) return;

    //D3DKMT_HANDLE adapterHandle;
    //LUID adapterLuid;
    //
    //if (NT_SUCCESS(GraphicsOpenAdapterFromDeviceName(&adapterHandle, &adapterLuid, PhGetString(context->DeviceEntry->Id.DevicePath))))
    //{
    //    for (ULONG i = 0; i < context->DeviceEntry->NumberOfNodes; i++)
    //    {
    //        ULONG64 frequency;
    //        ULONG64 maxFrequency;
    //        ULONG64 maxFrequencyOC;
    //        ULONG voltage;
    //        ULONG voltageMax;
    //        ULONG voltageMaxOC;
    //        ULONG64 maxTransitionLatency;
    //
    //        if (NT_SUCCESS(GraphicsQueryAdapterDeviceNodePerfData(
    //            adapterHandle,
    //            i,
    //            &frequency,
    //            &maxFrequency,
    //            &maxFrequencyOC,
    //            &voltage,
    //            &voltageMax,
    //            &voltageMaxOC,
    //            &maxTransitionLatency
    //            )))
    //        {
    //            dprintf("Node %lu: frequency=%llu, maxFrequency=%llu, maxFrequencyOC=%llu, voltage=%lu, voltageMax=%lu, voltageMaxOC=%lu, maxTransitionLatency=%llu\n",
    //                i, frequency, maxFrequency, maxFrequencyOC, voltage, voltageMax, voltageMaxOC, maxTransitionLatency);
    //        }
    //    }
    //
    //    GraphicsCloseAdapterHandle(adapterHandle);
    //}

    PostMessage(context->WindowHandle, WM_PH_UPDATE_DIALOG, 0, 0);
}

INT_PTR CALLBACK GraphicsDeviceNodesDlgProc(
    _In_ HWND WindowHandle,
    _In_ UINT WindowMessage,
    _In_ WPARAM wParam,
    _In_ LPARAM lParam
    )
{
    PDV_GPU_NODES_WINDOW_CONTEXT context = NULL;

    if (WindowMessage == WM_INITDIALOG)
    {
        context = PhAllocateZero(sizeof(DV_GPU_NODES_WINDOW_CONTEXT));
        context->NumberOfNodes = ((PDV_GPU_SYSINFO_CONTEXT)lParam)->DeviceEntry->NumberOfNodes;
        context->DeviceEntry = PhReferenceObject(((PDV_GPU_SYSINFO_CONTEXT)lParam)->DeviceEntry);

        PhSetWindowContext(WindowHandle, PH_WINDOW_CONTEXT_DEFAULT, context);
    }
    else
    {
        context = PhGetWindowContext(WindowHandle, PH_WINDOW_CONTEXT_DEFAULT);
    }

    if (context == NULL)
        return FALSE;

    switch (WindowMessage)
    {
    case WM_INITDIALOG:
        {
            ULONG i;
            ULONG numberOfRows;
            ULONG numberOfColumns;

            context->WindowHandle = WindowHandle;

            PhSetApplicationWindowIcon(WindowHandle);

            PhInitializeLayoutManager(&context->LayoutManager, WindowHandle);
            context->LayoutMargin = PhAddLayoutItem(&context->LayoutManager, GetDlgItem(WindowHandle, IDC_LAYOUT), NULL, PH_ANCHOR_ALL)->Margin;

            context->GraphHandle = PhAllocateZero(sizeof(HWND) * context->NumberOfNodes);
            context->GraphState = PhAllocateZero(sizeof(PH_GRAPH_STATE) * context->NumberOfNodes);

            // Calculate the minimum size.

            numberOfRows = (ULONG)sqrt(context->NumberOfNodes);
            numberOfColumns = (context->NumberOfNodes + numberOfRows - 1) / numberOfRows;
            context->MinimumSize.left = 0;
            context->MinimumSize.top = 0;
            context->MinimumSize.right = 55;
            context->MinimumSize.bottom = 60;
            MapDialogRect(WindowHandle, &context->MinimumSize);
            context->MinimumSize.right += (context->MinimumSize.right + GRAPH_PADDING) * numberOfColumns;
            context->MinimumSize.bottom += (context->MinimumSize.bottom + GRAPH_PADDING) * numberOfRows;
            SetWindowPos(WindowHandle, NULL, 0, 0, context->MinimumSize.right, context->MinimumSize.bottom, SWP_NOACTIVATE | SWP_NOMOVE | SWP_NOZORDER);

            {
                D3DKMT_HANDLE adapterHandle;
                LUID adapterLuid;

                if (NT_SUCCESS(GraphicsOpenAdapterFromDeviceName(&adapterHandle, &adapterLuid, PhGetString(context->DeviceEntry->Id.DevicePath))))
                {
                    context->NodeNameList = GraphicsQueryDeviceNodeList(adapterHandle, context->NumberOfNodes);
                    GraphicsCloseAdapterHandle(adapterHandle);
                }
            }

            for (i = 0; i < context->NumberOfNodes; i++)
            {
                PH_GRAPH_CREATEPARAMS graphCreateParams;

                memset(&graphCreateParams, 0, sizeof(PH_GRAPH_CREATEPARAMS));
                graphCreateParams.Size = sizeof(PH_GRAPH_CREATEPARAMS);
                graphCreateParams.Callback = GraphicsDeviceNodesMessageCallback;
                graphCreateParams.Context = context;

                context->GraphHandle[i] = PhCreateWindow(
                    PH_GRAPH_CLASSNAME,
                    NULL,
                    WS_VISIBLE | WS_CHILD | WS_BORDER,
                    0,
                    0,
                    0,
                    0,
                    WindowHandle,
                    UlongToPtr(i),
                    NULL,
                    &graphCreateParams
                    );
                Graph_SetTooltip(context->GraphHandle[i], TRUE);
                PhInitializeGraphState(&context->GraphState[i]);
            }

            // Note: This dialog must be centered after all other graphs and controls have been added.
            if (PhValidWindowPlacementFromSetting(SETTING_NAME_GRAPHICS_NODES_WINDOW_POSITION))
                PhLoadWindowPlacementFromSetting(SETTING_NAME_GRAPHICS_NODES_WINDOW_POSITION, SETTING_NAME_GRAPHICS_NODES_WINDOW_SIZE, WindowHandle);
            else
                PhCenterWindow(WindowHandle, NULL);

            PhRegisterCallback(
                PhGetGeneralCallback(GeneralCallbackProcessProviderUpdatedEvent),
                ProcessesUpdatedCallback,
                context,
                &context->ProcessesUpdatedCallbackRegistration
                );

            PhInitializeWindowTheme(WindowHandle, !!PhGetIntegerSetting(SETTING_ENABLE_THEME_SUPPORT));
        }
        break;
    case WM_DESTROY:
        {
            PhUnregisterCallback(PhGetGeneralCallback(GeneralCallbackProcessProviderUpdatedEvent), &context->ProcessesUpdatedCallbackRegistration);

            PhSaveWindowPlacementToSetting(SETTING_NAME_GRAPHICS_NODES_WINDOW_POSITION, SETTING_NAME_GRAPHICS_NODES_WINDOW_SIZE, WindowHandle);

            for (ULONG i = 0; i < context->NumberOfNodes; i++)
            {
                PhDeleteGraphState(&context->GraphState[i]);
            }

            if (context->NodeNameList)
            {
                PhDereferenceObjects(context->NodeNameList->Items, context->NodeNameList->Count);
                PhDereferenceObject(context->NodeNameList);
            }

            if (context->Description)
            {
                PhDereferenceObject(context->Description);
            }

            PhFree(context->GraphHandle);
            PhFree(context->GraphState);

            PhDeleteLayoutManager(&context->LayoutManager);

            if (context->DeviceEntry)
            {
                PhDereferenceObject(context->DeviceEntry);
            }

            PostQuitMessage(0);
        }
        break;
    case WM_NCDESTROY:
        {
            PhRemoveWindowContext(WindowHandle, PH_WINDOW_CONTEXT_DEFAULT);
            PhFree(context);
        }
        break;
    case WM_SIZE:
        {
            HDWP deferHandle;
            RECT clientRect;
            ULONG numberOfRows = (ULONG)sqrt(context->NumberOfNodes);
            ULONG numberOfColumns = (context->NumberOfNodes + numberOfRows - 1) / numberOfRows;
            ULONG numberOfYPaddings = numberOfRows - 1;
            ULONG numberOfXPaddings = numberOfColumns - 1;
            ULONG cellHeight;
            ULONG y;
            ULONG cellWidth;
            ULONG x;
            ULONG i;

            if (!(context->GraphState && context->GraphHandle))
                break;

            for (i = 0; i < context->NumberOfNodes; i++)
            {
                context->GraphState[i].Valid = FALSE;
                context->GraphState[i].TooltipIndex = ULONG_MAX;
            }

            PhLayoutManagerLayout(&context->LayoutManager);

            deferHandle = BeginDeferWindowPos(context->NumberOfNodes);

            PhGetClientRect(WindowHandle, &clientRect);
            cellHeight = (clientRect.bottom - context->LayoutMargin.top - context->LayoutMargin.bottom - GRAPH_PADDING * numberOfYPaddings) / numberOfRows;
            y = context->LayoutMargin.top;
            i = 0;

            for (ULONG row = 0; row < numberOfRows; ++row)
            {
                // Give the last row the remaining space; the height we calculated might be off by a few
                // pixels due to integer division.
                if (row == numberOfRows - 1)
                    cellHeight = clientRect.bottom - context->LayoutMargin.bottom - y;

                cellWidth = (clientRect.right - context->LayoutMargin.left - context->LayoutMargin.right - GRAPH_PADDING * numberOfXPaddings) / numberOfColumns;
                x = context->LayoutMargin.left;

                for (ULONG column = 0; column < numberOfColumns; column++)
                {
                    // Give the last cell the remaining space; the width we calculated might be off by a few
                    // pixels due to integer division.
                    if (column == numberOfColumns - 1)
                        cellWidth = clientRect.right - context->LayoutMargin.right - x;

                    if (i < context->NumberOfNodes)
                    {
                        deferHandle = DeferWindowPos(
                            deferHandle,
                            context->GraphHandle[i],
                            NULL,
                            x,
                            y,
                            cellWidth,
                            cellHeight,
                            SWP_NOACTIVATE | SWP_NOZORDER
                            );
                        i++;
                    }
                    x += cellWidth + GRAPH_PADDING;
                }

                y += cellHeight + GRAPH_PADDING;
            }

            EndDeferWindowPos(deferHandle);
        }
        break;
    case WM_SIZING:
        {
            PhResizingMinimumSize((PRECT)lParam, wParam, context->MinimumSize.right, context->MinimumSize.bottom);
        }
        break;
    case WM_COMMAND:
        {
            switch (GET_WM_COMMAND_ID(wParam, lParam))
            {
            case IDCANCEL:
                DestroyWindow(WindowHandle);
                break;
            }
        }
        break;
    case WM_PH_SHOW_DIALOG:
        {
            for (ULONG i = 0; i < context->NumberOfNodes; i++)
            {
                if (context->GraphState)
                {
                    context->GraphState[i].Valid = FALSE;
                    context->GraphState[i].TooltipIndex = ULONG_MAX;
                }

                if (context->GraphHandle)
                {
                    Graph_Draw(context->GraphHandle[i]);
                }
            }

            if (IsMinimized(WindowHandle))
                ShowWindow(WindowHandle, SW_RESTORE);
            else
                ShowWindow(WindowHandle, SW_SHOW);

            SetForegroundWindow(WindowHandle);
        }
        break;
    case WM_PH_UPDATE_DIALOG:
        {
            for (ULONG i = 0; i < context->NumberOfNodes; i++)
            {
                if (context->GraphState)
                {
                    context->GraphState[i].Valid = FALSE;
                    context->GraphState[i].TooltipIndex = ULONG_MAX;
                }

                if (context->GraphHandle)
                {
                    Graph_MoveGrid(context->GraphHandle[i], 1);
                    Graph_Draw(context->GraphHandle[i]);
                    Graph_UpdateTooltip(context->GraphHandle[i]);
                    InvalidateRect(context->GraphHandle[i], NULL, FALSE);
                }
            }
        }
        break;
    case WM_CTLCOLORBTN:
        return HANDLE_WM_CTLCOLORBTN(WindowHandle, wParam, lParam, PhWindowThemeControlColor);
    case WM_CTLCOLORDLG:
        return HANDLE_WM_CTLCOLORDLG(WindowHandle, wParam, lParam, PhWindowThemeControlColor);
    case WM_CTLCOLORSTATIC:
        return HANDLE_WM_CTLCOLORSTATIC(WindowHandle, wParam, lParam, PhWindowThemeControlColor);
    }

    return FALSE;
}

_Function_class_(PH_GRAPH_MESSAGE_CALLBACK)
BOOLEAN GraphicsDeviceNodesMessageCallback(
    _In_ HWND WindowHandle,
    _In_ ULONG Message,
    _In_ PVOID Parameter1,
    _In_ PVOID Parameter2,
    _In_ PVOID Context
    )
{
    PDV_GPU_NODES_WINDOW_CONTEXT context = (PDV_GPU_NODES_WINDOW_CONTEXT)Context;

    switch (Message)
    {
    case GCN_GETDRAWINFO:
        {
            PPH_GRAPH_GETDRAWINFO getDrawInfo = (PPH_GRAPH_GETDRAWINFO)Parameter1;
            PPH_GRAPH_DRAW_INFO drawInfo = getDrawInfo->DrawInfo;
            LONG dpiValue;

            if (!(context->GraphState && context->GraphHandle))
                break;
            if (context->NumberOfNodes != context->DeviceEntry->NumberOfNodes)
                break;

            ULONG index = GetDlgCtrlID(WindowHandle);
            if (index > context->NumberOfNodes)
                break;

            PPH_GRAPH_STATE graphState = &context->GraphState[index];
            if (!graphState)
                break;

            dpiValue = PhGetWindowDpi(context->WindowHandle);
            drawInfo->Flags = PH_GRAPH_USE_GRID_X | PH_GRAPH_USE_GRID_Y | (GraphicsEnableScaleText ? PH_GRAPH_LABEL_MAX_Y : 0);
            PhSiSetColorsGraphDrawInfo(drawInfo, PhGetIntegerSetting(SETTING_COLOR_CPU_KERNEL), 0, dpiValue);

            PhGraphStateGetDrawInfo(
                graphState,
                getDrawInfo,
                context->DeviceEntry->GpuNodesHistory[index].Count
                );

            if (!graphState->Valid)
            {
                PhCopyCircularBuffer_FLOAT(&context->DeviceEntry->GpuNodesHistory[index], graphState->Data1, drawInfo->LineDataCount);

                {
                    FLOAT max = 0;

                    for (ULONG ii = 0; ii < drawInfo->LineDataCount; ii++)
                    {
                        FLOAT data = graphState->Data1[ii]; // HACK

                        if (max < data)
                            max = data;
                    }

                    if (GraphicsEnableScaleGraph)
                    {
                        if (max != 0)
                        {
                            PhDivideSinglesBySingle(graphState->Data1, max, drawInfo->LineDataCount);
                        }
                    }

                    if (GraphicsEnableScaleText)
                    {
                        drawInfo->LabelYFunction = PhSiDoubleLabelYFunction;
                        drawInfo->LabelYFunctionParameter = max;
                    }
                }

                graphState->Valid = TRUE;
            }

            if (GraphicsGraphShowText)
            {
                HDC hdc;
                FLOAT gpu;
                PPH_STRING engineName = NULL;

                gpu = PhGetItemCircularBuffer_FLOAT(&context->DeviceEntry->GpuNodesHistory[index], 0);

                if (context->NodeNameList)
                {
                    engineName = context->NodeNameList->Items[index];
                }

                if (!PhIsNullOrEmptyString(engineName))
                {
                    PH_FORMAT format[4];

                    // %.2f%% (%s)
                    PhInitFormatF(&format[0], gpu * 100, 2);
                    PhInitFormatS(&format[1], L"% (");
                    PhInitFormatSR(&format[2], engineName->sr);
                    PhInitFormatC(&format[3], L')');

                    PhMoveReference(&graphState->Text, PhFormat(format, RTL_NUMBER_OF(format), 0));
                }
                else
                {
                    PH_FORMAT format[4];

                    // %.2f%% (Node %lu)
                    PhInitFormatF(&format[0], gpu * 100, 2);
                    PhInitFormatS(&format[1], L"% (Node ");
                    PhInitFormatU(&format[2], index);
                    PhInitFormatC(&format[3], L')');

                    PhMoveReference(&graphState->Text, PhFormat(format, RTL_NUMBER_OF(format), 0));
                }

                hdc = Graph_GetBufferedContext(context->GraphHandle[index]);
                PhSetGraphText(
                    hdc,
                    drawInfo,
                    &graphState->Text->sr,
                    &NormalGraphTextMargin,
                    &NormalGraphTextPadding,
                    PH_ALIGN_TOP | PH_ALIGN_LEFT
                    );
            }
            else
            {
                drawInfo->Text.Buffer = NULL;
            }
        }
        break;
    case GCN_GETTOOLTIPTEXT:
        {
            PPH_GRAPH_GETTOOLTIPTEXT getTooltipText = (PPH_GRAPH_GETTOOLTIPTEXT)Parameter1;

            if (getTooltipText->Index < getTooltipText->TotalCount)
            {
                ULONG index = GetDlgCtrlID(WindowHandle);
                if (index > context->NumberOfNodes)
                    break;
          
                PPH_GRAPH_STATE graphState = &context->GraphState[index];
                if (!graphState)
                    break;

                if (graphState->TooltipIndex != getTooltipText->Index)
                {
                    FLOAT gpu;
                    PPH_STRING engineName = NULL;

                    gpu = PhGetItemCircularBuffer_FLOAT(
                        &context->DeviceEntry->GpuNodesHistory[index],
                        getTooltipText->Index
                        );

                    if (context->NodeNameList)
                    {
                        engineName = context->NodeNameList->Items[index];
                    }

                    if (PhIsNullOrEmptyString(context->Description))
                    {
                        context->Description = GraphicsQueryDeviceInterfaceDescription(
                            PhGetString(context->DeviceEntry->Id.DevicePath)
                            );
                    }

                    if (!PhIsNullOrEmptyString(engineName) && !PhIsNullOrEmptyString(context->Description))
                    {
                        PH_FORMAT format[9];

                        // %.2f%%\nNode %lu (%s) on %s\n%s
                        PhInitFormatF(&format[0], gpu * 100, 2);
                        PhInitFormatS(&format[1], L"%\nNode ");
                        PhInitFormatU(&format[2], index);
                        PhInitFormatS(&format[3], L" (");
                        PhInitFormatSR(&format[4], engineName->sr);
                        PhInitFormatS(&format[5], L") on ");
                        PhInitFormatSR(&format[6], context->Description->sr);
                        PhInitFormatC(&format[7], L'\n');
                        PhInitFormatSR(&format[8], PH_AUTO_T(PH_STRING, PhGetStatisticsTimeString(NULL, getTooltipText->Index))->sr);

                        PhMoveReference(&graphState->TooltipText, PhFormat(format, RTL_NUMBER_OF(format), 0));
                    }
                    else
                    {
                        PH_FORMAT format[5];

                        // %.2f%%\nNode %lu on %s\n%s
                        PhInitFormatF(&format[0], gpu * 100, 2);
                        PhInitFormatS(&format[1], L"%\nNode ");
                        PhInitFormatU(&format[2], index);
                        PhInitFormatC(&format[3], L'\n');
                        PhInitFormatSR(&format[4], PH_AUTO_T(PH_STRING, PhGetStatisticsTimeString(NULL, getTooltipText->Index))->sr);

                        PhMoveReference(&graphState->TooltipText, PhFormat(format, RTL_NUMBER_OF(format), 0));
                    }
                }

                getTooltipText->Text = PhGetStringRef(graphState->TooltipText);
            }
        }
        break;
    case GCN_MOUSEEVENT:
        {
            PPH_GRAPH_MOUSEEVENT mouseEvent = (PPH_GRAPH_MOUSEEVENT)Parameter1;
            PPH_PROCESS_RECORD record = NULL;

            if (mouseEvent->Message == WM_LBUTTONDBLCLK)
            {
                //if (PhGetIntegerSetting(SETTING_NAME_SHOWSYSINFOGRAPH))
                //{
                //    PhShowSystemInformationDialog(L"CPU");
                //}
                //else
                //{
                //    if (mouseEvent->Index < mouseEvent->TotalCount)
                //    {
                //        record = PhSipReferenceMaxCpuRecord(mouseEvent->Index);
                //    }
                //
                //    if (record)
                //    {
                //        PhShowProcessRecordDialog(MainWindowHandle, record);
                //        PhDereferenceProcessRecord(record);
                //    }
                //}
            }
            else if (mouseEvent->Message == WM_RBUTTONUP)
            {
                //ShowCustomizeMenu(WindowHandle);
            }
        }
        break;
    }

    return TRUE;
}
