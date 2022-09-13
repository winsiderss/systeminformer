/*
 * Copyright (c) 2022 Winsider Seminars & Solutions, Inc.  All rights reserved.
 *
 * This file is part of System Informer.
 *
 * Authors:
 *
 *     wj32    2011
 *     dmex    2011-2022
 *
 */

#include "exttools.h"
#include <netfw.h>

LONG EtpProcessTreeNewSortFunction(
    _In_ PVOID Node1,
    _In_ PVOID Node2,
    _In_ ULONG SubId,
    _In_ PH_SORT_ORDER SortOrder,
    _In_ PVOID Context
    );

LONG EtpNetworkTreeNewSortFunction(
    _In_ PVOID Node1,
    _In_ PVOID Node2,
    _In_ ULONG SubId,
    _In_ PH_SORT_ORDER SortOrder,
    _In_ PVOID Context
    );

typedef struct _COLUMN_INFO
{
    ULONG SubId;
    PWSTR Text;
    ULONG Width;
    ULONG Alignment;
    ULONG TextFlags;
    BOOLEAN SortDescending;
} COLUMN_INFO, *PCOLUMN_INFO;

typedef enum _PHP_AGGREGATE_TYPE
{
    AggregateTypeFloat,
    AggregateTypeInt32,
    AggregateTypeInt64,
    AggregateTypeIntPtr
} PHP_AGGREGATE_TYPE;

typedef enum _PHP_AGGREGATE_LOCATION
{
    AggregateLocationProcessNode,
    AggregateLocationProcessItem
} PHP_AGGREGATE_LOCATION;

static ULONG ProcessTreeListSortColumn;
static PH_SORT_ORDER ProcessTreeListSortOrder;

static GUID IID_INetFwMgr_I = { 0xf7898af5, 0xcac4, 0x4632, { 0xa2, 0xec, 0xda, 0x06, 0xe5, 0x11, 0x1a, 0xf2 } };
static GUID CLSID_NetFwMgr_I = { 0x304ce942, 0x6e39, 0x40d8, { 0x94, 0x3a, 0xb9, 0x13, 0xc4, 0x0c, 0x9c, 0xd4 } };

VOID EtpAddTreeNewColumn(
    _In_ PPH_PLUGIN_TREENEW_INFORMATION TreeNewInfo,
    _In_ ULONG SubId,
    _In_ PWSTR Text,
    _In_ ULONG Width,
    _In_ ULONG Alignment,
    _In_ ULONG TextFlags,
    _In_ BOOLEAN SortDescending,
    _In_ PPH_PLUGIN_TREENEW_SORT_FUNCTION SortFunction
    )
{
    PH_TREENEW_COLUMN column;

    memset(&column, 0, sizeof(PH_TREENEW_COLUMN));
    column.SortDescending = SortDescending;
    column.Text = Text;
    column.Width = Width;
    column.Alignment = Alignment;
    column.TextFlags = TextFlags;

    PhPluginAddTreeNewColumn(
        PluginInstance,
        TreeNewInfo->CmData,
        &column,
        SubId,
        NULL,
        SortFunction
        );
}

VOID EtProcessTreeNewInitializing(
    _In_ PVOID Parameter
    )
{
    static COLUMN_INFO columns[] =
    {
        { ETPRTNC_DISKREADS, L"Disk reads", 70, PH_ALIGN_RIGHT, DT_RIGHT, TRUE },
        { ETPRTNC_DISKWRITES, L"Disk writes", 70, PH_ALIGN_RIGHT, DT_RIGHT, TRUE },
        { ETPRTNC_DISKREADBYTES, L"Disk read bytes", 70, PH_ALIGN_RIGHT, DT_RIGHT, TRUE },
        { ETPRTNC_DISKWRITEBYTES, L"Disk write bytes", 70, PH_ALIGN_RIGHT, DT_RIGHT, TRUE },
        { ETPRTNC_DISKTOTALBYTES, L"Disk total bytes", 70, PH_ALIGN_RIGHT, DT_RIGHT, TRUE },
        { ETPRTNC_DISKREADSDELTA, L"Disk reads delta", 70, PH_ALIGN_RIGHT, DT_RIGHT, TRUE },
        { ETPRTNC_DISKWRITESDELTA, L"Disk writes delta", 70, PH_ALIGN_RIGHT, DT_RIGHT, TRUE },
        { ETPRTNC_DISKREADBYTESDELTA, L"Disk read bytes delta", 70, PH_ALIGN_RIGHT, DT_RIGHT, TRUE },
        { ETPRTNC_DISKWRITEBYTESDELTA, L"Disk write bytes delta", 70, PH_ALIGN_RIGHT, DT_RIGHT, TRUE },
        { ETPRTNC_DISKTOTALBYTESDELTA, L"Disk total bytes delta", 70, PH_ALIGN_RIGHT, DT_RIGHT, TRUE },
        { ETPRTNC_NETWORKRECEIVES, L"Network receives", 70, PH_ALIGN_RIGHT, DT_RIGHT, TRUE },
        { ETPRTNC_NETWORKSENDS, L"Network sends", 70, PH_ALIGN_RIGHT, DT_RIGHT, TRUE },
        { ETPRTNC_NETWORKRECEIVEBYTES, L"Network receive bytes", 70, PH_ALIGN_RIGHT, DT_RIGHT, TRUE },
        { ETPRTNC_NETWORKSENDBYTES, L"Network send bytes", 70, PH_ALIGN_RIGHT, DT_RIGHT, TRUE },
        { ETPRTNC_NETWORKTOTALBYTES, L"Network total bytes", 70, PH_ALIGN_RIGHT, DT_RIGHT, TRUE },
        { ETPRTNC_NETWORKRECEIVESDELTA, L"Network receives delta", 70, PH_ALIGN_RIGHT, DT_RIGHT, TRUE },
        { ETPRTNC_NETWORKSENDSDELTA, L"Network sends delta", 70, PH_ALIGN_RIGHT, DT_RIGHT, TRUE },
        { ETPRTNC_NETWORKRECEIVEBYTESDELTA, L"Network receive bytes delta", 70, PH_ALIGN_RIGHT, DT_RIGHT, TRUE },
        { ETPRTNC_NETWORKSENDBYTESDELTA, L"Network send bytes delta", 70, PH_ALIGN_RIGHT, DT_RIGHT, TRUE },
        { ETPRTNC_NETWORKTOTALBYTESDELTA, L"Network total bytes delta", 70, PH_ALIGN_RIGHT, DT_RIGHT, TRUE },
        { ETPRTNC_HARDFAULTS, L"Hard faults", 70, PH_ALIGN_RIGHT, DT_RIGHT, TRUE },
        { ETPRTNC_HARDFAULTSDELTA, L"Hard faults delta", 70, PH_ALIGN_RIGHT, DT_RIGHT, TRUE },
        { ETPRTNC_PEAKTHREADS, L"Peak threads", 45, PH_ALIGN_RIGHT, DT_RIGHT, TRUE },
        { ETPRTNC_GPU, L"GPU", 45, PH_ALIGN_RIGHT, DT_RIGHT, TRUE },
        { ETPRTNC_GPUDEDICATEDBYTES, L"GPU dedicated bytes", 70, PH_ALIGN_RIGHT, DT_RIGHT, TRUE },
        { ETPRTNC_GPUSHAREDBYTES, L"GPU shared bytes", 70, PH_ALIGN_RIGHT, DT_RIGHT, TRUE },
        { ETPRTNC_DISKREADRATE, L"Disk read rate", 70, PH_ALIGN_RIGHT, DT_RIGHT, TRUE },
        { ETPRTNC_DISKWRITERATE, L"Disk write rate", 70, PH_ALIGN_RIGHT, DT_RIGHT, TRUE },
        { ETPRTNC_DISKTOTALRATE, L"Disk total rate", 70, PH_ALIGN_RIGHT, DT_RIGHT, TRUE },
        { ETPRTNC_NETWORKRECEIVERATE, L"Network receive rate", 70, PH_ALIGN_RIGHT, DT_RIGHT, TRUE },
        { ETPRTNC_NETWORKSENDRATE, L"Network send rate", 70, PH_ALIGN_RIGHT, DT_RIGHT, TRUE },
        { ETPRTNC_NETWORKTOTALRATE, L"Network total rate", 70, PH_ALIGN_RIGHT, DT_RIGHT, TRUE },
        { ETPRTNC_FPS, L"FPS", 50, PH_ALIGN_RIGHT, DT_RIGHT, TRUE },
    };

    PPH_PLUGIN_TREENEW_INFORMATION treeNewInfo = Parameter;
    ULONG i;

    for (i = 0; i < RTL_NUMBER_OF(columns); i++)
    {
        EtpAddTreeNewColumn(treeNewInfo, columns[i].SubId, columns[i].Text, columns[i].Width, columns[i].Alignment,
            columns[i].TextFlags, columns[i].SortDescending, EtpProcessTreeNewSortFunction);
    }

    PhPluginEnableTreeNewNotify(PluginInstance, treeNewInfo->CmData);
}

// Copied from ProcessHacker\proctree.c
FORCEINLINE PVOID PhpFieldForAggregate(
    _In_ PPH_PROCESS_NODE ProcessNode,
    _In_ PHP_AGGREGATE_LOCATION Location,
    _In_ SIZE_T FieldOffset
    )
{
    PVOID object;

    switch (Location)
    {
    case AggregateLocationProcessNode:
        object = PhPluginGetObjectExtension(PluginInstance, ProcessNode->ProcessItem, EmProcessNodeType);
        break;
    case AggregateLocationProcessItem:
        object = PhPluginGetObjectExtension(PluginInstance, ProcessNode->ProcessItem, EmProcessItemType);
        break;
    default:
        PhRaiseStatus(STATUS_INVALID_PARAMETER);
    }

    return PTR_ADD_OFFSET(object, FieldOffset);
}

// Copied from ProcessHacker\proctree.c
FORCEINLINE VOID PhpAccumulateField(
    _Inout_ PVOID Accumulator,
    _In_ PVOID Value,
    _In_ PHP_AGGREGATE_TYPE Type
    )
{
    switch (Type)
    {
    case AggregateTypeFloat:
        *(PFLOAT)Accumulator += *(PFLOAT)Value;
        break;
    case AggregateTypeInt32:
        *(PULONG)Accumulator += *(PULONG)Value;
        break;
    case AggregateTypeInt64:
        *(PULONG64)Accumulator += *(PULONG64)Value;
        break;
    case AggregateTypeIntPtr:
        *(PULONG_PTR)Accumulator += *(PULONG_PTR)Value;
        break;
    }
}

// Copied from ProcessHacker\proctree.c
static VOID PhpAggregateField(
    _In_ PPH_PROCESS_NODE ProcessNode,
    _In_ PHP_AGGREGATE_TYPE Type,
    _In_ PHP_AGGREGATE_LOCATION Location,
    _In_ SIZE_T FieldOffset,
    _Inout_ PVOID AggregatedValue
    )
{
    PhpAccumulateField(AggregatedValue, PhpFieldForAggregate(ProcessNode, Location, FieldOffset), Type);

    for (ULONG i = 0; i < ProcessNode->Children->Count; i++)
    {
        PhpAggregateField(ProcessNode->Children->Items[i], Type, Location, FieldOffset, AggregatedValue);
    }
}

// Copied from ProcessHacker\proctree.c
static VOID PhpAggregateFieldIfNeeded(
    _In_ PPH_PROCESS_NODE ProcessNode,
    _In_ PHP_AGGREGATE_TYPE Type,
    _In_ PHP_AGGREGATE_LOCATION Location,
    _In_ SIZE_T FieldOffset,
    _Inout_ PVOID AggregatedValue
    )
{
    if (!EtPropagateCpuUsage || ProcessNode->Node.Expanded || ProcessTreeListSortOrder != NoSortOrder)
    {
        PhpAccumulateField(AggregatedValue, PhpFieldForAggregate(ProcessNode, Location, FieldOffset), Type);
    }
    else
    {
        PhpAggregateField(ProcessNode, Type, Location, FieldOffset, AggregatedValue);
    }
}

VOID EtProcessTreeNewMessage(
    _In_ PVOID Parameter
    )
{
    PPH_PLUGIN_TREENEW_MESSAGE message = Parameter;
    PPH_PROCESS_NODE processNode;
    PET_PROCESS_BLOCK block;

    if (message->Message == TreeNewGetCellText)
    {
        PPH_TREENEW_GET_CELL_TEXT getCellText = message->Parameter1;
        processNode = (PPH_PROCESS_NODE)getCellText->Node;
        block = EtGetProcessBlock(processNode->ProcessItem);

        PhAcquireQueuedLockExclusive(&block->TextCacheLock);

        if (block->TextCacheValid[message->SubId])
        {
            if (block->TextCacheLength[message->SubId])
            {
                getCellText->Text.Length = block->TextCacheLength[message->SubId];
                getCellText->Text.Buffer = block->TextCache[message->SubId];
            }
        }
        else
        {
            switch (message->SubId)
            {
            case ETPRTNC_DISKREADS:
                EtFormatInt64(block->DiskReadCount, block, message);
                break;
            case ETPRTNC_DISKWRITES:
                EtFormatInt64(block->DiskWriteCount, block, message);
                break;
            case ETPRTNC_DISKREADBYTES:
                EtFormatSize(block->DiskReadRaw, block, message);
                break;
            case ETPRTNC_DISKWRITEBYTES:
                EtFormatSize(block->DiskWriteRaw, block, message);
                break;
            case ETPRTNC_DISKTOTALBYTES:
                {
                    ULONG64 number = 0;

                    PhpAggregateFieldIfNeeded(processNode, AggregateTypeInt64, AggregateLocationProcessItem, FIELD_OFFSET(ET_PROCESS_BLOCK, DiskReadRaw), &number);
                    PhpAggregateFieldIfNeeded(processNode, AggregateTypeInt64, AggregateLocationProcessItem, FIELD_OFFSET(ET_PROCESS_BLOCK, DiskWriteRaw), &number);

                    EtFormatSize(number, block, message);
                }
                break;
            case ETPRTNC_DISKREADSDELTA:
                EtFormatInt64(block->DiskReadDelta.Delta, block, message);
                break;
            case ETPRTNC_DISKWRITESDELTA:
                EtFormatInt64(block->DiskWriteDelta.Delta, block, message);
                break;
            case ETPRTNC_DISKREADBYTESDELTA:
                EtFormatSize(block->DiskReadRawDelta.Delta, block, message);
                break;
            case ETPRTNC_DISKWRITEBYTESDELTA:
                EtFormatSize(block->DiskWriteRawDelta.Delta, block, message);
                break;
            case ETPRTNC_DISKTOTALBYTESDELTA:
                {
                    ULONG64 number = 0;

                    PhpAggregateFieldIfNeeded(processNode, AggregateTypeInt64, AggregateLocationProcessItem, FIELD_OFFSET(ET_PROCESS_BLOCK, DiskReadRawDelta.Delta), &number);
                    PhpAggregateFieldIfNeeded(processNode, AggregateTypeInt64, AggregateLocationProcessItem, FIELD_OFFSET(ET_PROCESS_BLOCK, DiskWriteRawDelta.Delta), &number);

                    EtFormatSize(number, block, message);
                }
                break;
            case ETPRTNC_NETWORKRECEIVES:
                EtFormatInt64(block->NetworkReceiveCount, block, message);
                break;
            case ETPRTNC_NETWORKSENDS:
                EtFormatInt64(block->NetworkSendCount, block, message);
                break;
            case ETPRTNC_NETWORKRECEIVEBYTES:
                EtFormatSize(block->NetworkReceiveRaw, block, message);
                break;
            case ETPRTNC_NETWORKSENDBYTES:
                EtFormatSize(block->NetworkSendRaw, block, message);
                break;
            case ETPRTNC_NETWORKTOTALBYTES:
                {
                    ULONG64 number = 0;

                    PhpAggregateFieldIfNeeded(processNode, AggregateTypeInt64, AggregateLocationProcessItem, FIELD_OFFSET(ET_PROCESS_BLOCK, NetworkReceiveRaw), &number);
                    PhpAggregateFieldIfNeeded(processNode, AggregateTypeInt64, AggregateLocationProcessItem, FIELD_OFFSET(ET_PROCESS_BLOCK, NetworkSendRaw), &number);

                    EtFormatSize(number, block, message);
                }
                break;
            case ETPRTNC_NETWORKRECEIVESDELTA:
                EtFormatInt64(block->NetworkReceiveDelta.Delta, block, message);
                break;
            case ETPRTNC_NETWORKSENDSDELTA:
                EtFormatInt64(block->NetworkSendDelta.Delta, block, message);
                break;
            case ETPRTNC_NETWORKRECEIVEBYTESDELTA:
                EtFormatSize(block->NetworkReceiveRawDelta.Delta, block, message);
                break;
            case ETPRTNC_NETWORKSENDBYTESDELTA:
                EtFormatSize(block->NetworkSendRawDelta.Delta, block, message);
                break;
            case ETPRTNC_NETWORKTOTALBYTESDELTA:
                {
                    ULONG64 number = 0;

                    PhpAggregateFieldIfNeeded(processNode, AggregateTypeInt64, AggregateLocationProcessItem, FIELD_OFFSET(ET_PROCESS_BLOCK, NetworkReceiveRawDelta.Delta), &number);
                    PhpAggregateFieldIfNeeded(processNode, AggregateTypeInt64, AggregateLocationProcessItem, FIELD_OFFSET(ET_PROCESS_BLOCK, NetworkSendRawDelta.Delta), &number);

                    EtFormatSize(number, block, message);
                }
                break;
            case ETPRTNC_HARDFAULTS:
                EtFormatInt64(processNode->ProcessItem->HardFaultsDelta.Value, block, message);
                break;
            case ETPRTNC_HARDFAULTSDELTA:
                if (processNode->ProcessItem->HardFaultsDelta.Delta != 0)
                    EtFormatInt64(processNode->ProcessItem->HardFaultsDelta.Delta, block, message);
                break;
            case ETPRTNC_PEAKTHREADS:
                EtFormatInt64(block->ProcessItem->PeakNumberOfThreads, block, message);
                break;
            case ETPRTNC_GPU:
                {
                    FLOAT gpuUsage = 0;

                    PhpAggregateFieldIfNeeded(
                        processNode,
                        AggregateTypeFloat,
                        AggregateLocationProcessItem,
                        FIELD_OFFSET(ET_PROCESS_BLOCK, GpuNodeUtilization),
                        &gpuUsage
                        );

                    gpuUsage *= 100;
                    EtFormatDouble(gpuUsage, block, message);
                }
                break;
            case ETPRTNC_GPUDEDICATEDBYTES:
                {
                    ULONG64 gpuDedicatedUsage = 0;

                    PhpAggregateFieldIfNeeded(
                        processNode,
                        AggregateTypeInt64,
                        AggregateLocationProcessItem,
                        FIELD_OFFSET(ET_PROCESS_BLOCK, GpuDedicatedUsage),
                        &gpuDedicatedUsage
                        );

                    EtFormatSize(gpuDedicatedUsage, block, message);
                }
                break;
            case ETPRTNC_GPUSHAREDBYTES:
                {
                    ULONG64 gpuSharedUsage = 0;

                    PhpAggregateFieldIfNeeded(
                        processNode,
                        AggregateTypeInt64,
                        AggregateLocationProcessItem,
                        FIELD_OFFSET(ET_PROCESS_BLOCK, GpuSharedUsage),
                        &gpuSharedUsage
                        );

                    EtFormatSize(gpuSharedUsage, block, message);
                }
                break;
            case ETPRTNC_DISKREADRATE:
                EtFormatRate(block->DiskReadRawDelta.Delta, block, message);
                break;
            case ETPRTNC_DISKWRITERATE:
                EtFormatRate(block->DiskWriteRawDelta.Delta, block, message);
                break;
            case ETPRTNC_DISKTOTALRATE:
                {
                    ULONG64 number = 0;

                    PhpAggregateFieldIfNeeded(processNode, AggregateTypeInt64, AggregateLocationProcessItem, FIELD_OFFSET(ET_PROCESS_BLOCK, DiskReadRawDelta.Delta), &number);
                    PhpAggregateFieldIfNeeded(processNode, AggregateTypeInt64, AggregateLocationProcessItem, FIELD_OFFSET(ET_PROCESS_BLOCK, DiskWriteRawDelta.Delta), &number);

                    EtFormatRate(number, block, message);
                }
                break;
            case ETPRTNC_NETWORKRECEIVERATE:
                EtFormatRate(block->NetworkReceiveRawDelta.Delta, block, message);
                break;
            case ETPRTNC_NETWORKSENDRATE:
                EtFormatRate(block->NetworkSendRawDelta.Delta, block, message);
                break;
            case ETPRTNC_NETWORKTOTALRATE:
                {
                    ULONG64 number = 0;

                    PhpAggregateFieldIfNeeded(processNode, AggregateTypeInt64, AggregateLocationProcessItem, FIELD_OFFSET(ET_PROCESS_BLOCK, NetworkReceiveRawDelta.Delta), &number);
                    PhpAggregateFieldIfNeeded(processNode, AggregateTypeInt64, AggregateLocationProcessItem, FIELD_OFFSET(ET_PROCESS_BLOCK, NetworkSendRawDelta.Delta), &number);

                    EtFormatRate(number, block, message);
                }
                break;
            case ETPRTNC_FPS:
                {
                    FLOAT frames = 0;

                    PhpAggregateFieldIfNeeded(
                        processNode,
                        AggregateTypeFloat,
                        AggregateLocationProcessItem,
                        FIELD_OFFSET(ET_PROCESS_BLOCK, FramesPerSecond),
                        &frames
                        );

                    EtFormatDouble(frames, block, message);
                }
                break;
            }

            if (block->TextCacheLength[message->SubId])
            {
                getCellText->Text.Length = block->TextCacheLength[message->SubId];
                getCellText->Text.Buffer = block->TextCache[message->SubId];
            }

            block->TextCacheValid[message->SubId] = TRUE;
        }

        PhReleaseQueuedLockExclusive(&block->TextCacheLock);
    }
    else if (message->Message == TreeNewSortChanged)
    {
        TreeNew_GetSort(message->TreeNewHandle, &ProcessTreeListSortColumn, &ProcessTreeListSortOrder);
    }
    else if (message->Message == TreeNewNodeExpanding)
    {
        processNode = message->Parameter1;
        block = EtGetProcessBlock(processNode->ProcessItem);

        if (EtPropagateCpuUsage)
        {
            block->TextCacheValid[ETPRTNC_DISKTOTALBYTES] = FALSE;
            block->TextCacheValid[ETPRTNC_NETWORKTOTALBYTES] = FALSE;

            block->TextCacheValid[ETPRTNC_DISKTOTALBYTESDELTA] = FALSE;
            block->TextCacheValid[ETPRTNC_NETWORKTOTALBYTESDELTA] = FALSE;

            block->TextCacheValid[ETPRTNC_GPU] = FALSE;
            block->TextCacheValid[ETPRTNC_GPUDEDICATEDBYTES] = FALSE;
            block->TextCacheValid[ETPRTNC_GPUSHAREDBYTES] = FALSE;

            block->TextCacheValid[ETPRTNC_DISKTOTALRATE] = FALSE;
            block->TextCacheValid[ETPRTNC_NETWORKTOTALRATE] = FALSE;

            block->TextCacheValid[ETPRTNC_FPS] = FALSE;
        }
    }
    else if (message->Message == TreeNewGetHeaderText)
    {
        PPH_TREENEW_GET_HEADER_TEXT getHeaderText = message->Parameter1;
        PPH_TREENEW_COLUMN column = getHeaderText->Column;
        PLIST_ENTRY listEntry;
        SIZE_T returnLength;
        FLOAT decimal = 0;
        ULONG64 number = 0;

        if (ProcessesUpdatedCount != 3)
            return;

        switch (message->SubId)
        {
        case ETPRTNC_DISKREADS:
        case ETPRTNC_DISKWRITES:
        case ETPRTNC_DISKREADBYTES:
        case ETPRTNC_DISKWRITEBYTES:
        case ETPRTNC_DISKTOTALBYTES:
        case ETPRTNC_DISKREADSDELTA:
        case ETPRTNC_DISKWRITESDELTA:
        case ETPRTNC_DISKREADBYTESDELTA:
        case ETPRTNC_DISKWRITEBYTESDELTA:
        case ETPRTNC_DISKTOTALBYTESDELTA:
        case ETPRTNC_NETWORKRECEIVES:
        case ETPRTNC_NETWORKSENDS:
        case ETPRTNC_NETWORKRECEIVEBYTES:
        case ETPRTNC_NETWORKSENDBYTES:
        case ETPRTNC_NETWORKTOTALBYTES:
        case ETPRTNC_NETWORKRECEIVESDELTA:
        case ETPRTNC_NETWORKSENDSDELTA:
        case ETPRTNC_NETWORKRECEIVEBYTESDELTA:
        case ETPRTNC_NETWORKSENDBYTESDELTA:
        case ETPRTNC_NETWORKTOTALBYTESDELTA:
        case ETPRTNC_HARDFAULTS:
        case ETPRTNC_HARDFAULTSDELTA:
        case ETPRTNC_PEAKTHREADS:
        case ETPRTNC_GPU:
        case ETPRTNC_GPUDEDICATEDBYTES:
        case ETPRTNC_GPUSHAREDBYTES:
        case ETPRTNC_DISKREADRATE:
        case ETPRTNC_DISKWRITERATE:
        case ETPRTNC_DISKTOTALRATE:
        case ETPRTNC_NETWORKRECEIVERATE:
        case ETPRTNC_NETWORKSENDRATE:
        case ETPRTNC_NETWORKTOTALRATE:
        case ETPRTNC_FPS:
            break;
        default:
            return;
        }

        listEntry = EtProcessBlockListHead.Flink;

        while (listEntry != &EtProcessBlockListHead)
        {
            PET_PROCESS_BLOCK block = CONTAINING_RECORD(listEntry, ET_PROCESS_BLOCK, ListEntry);

            if (block->ProcessItem->State & PH_PROCESS_ITEM_REMOVED)
            {
                listEntry = listEntry->Flink;
                continue; // Skip terminated.
            }

            if (block->ProcessNode)
            {
                if (!block->ProcessNode->Node.Visible)
                {
                    listEntry = listEntry->Flink;
                    continue; // Skip filtered nodes.
                }
            }
            else
            {
                listEntry = listEntry->Flink;
                continue; // Skip filtered nodes.
            }

            switch (message->SubId)
            {
            case ETPRTNC_DISKREADS:
                number += block->DiskReadCount;
                break;
            case ETPRTNC_DISKWRITES:
                number += block->DiskWriteCount;
                break;
            case ETPRTNC_DISKREADBYTES:
                number += block->DiskReadRaw;
                break;
            case ETPRTNC_DISKWRITEBYTES:
                number += block->DiskWriteRaw;
                break;
            case ETPRTNC_DISKTOTALBYTES:
                number += block->DiskReadRaw + block->DiskWriteRaw;
                break;
            case ETPRTNC_DISKREADSDELTA:
                number += block->DiskReadDelta.Delta;
                break;
            case ETPRTNC_DISKWRITESDELTA:
                number += block->DiskWriteDelta.Delta;
                break;
            case ETPRTNC_DISKREADBYTESDELTA:
                number += block->DiskReadRawDelta.Delta;
                break;
            case ETPRTNC_DISKWRITEBYTESDELTA:
                number += block->DiskWriteRawDelta.Delta;
                break;
            case ETPRTNC_DISKTOTALBYTESDELTA:
                number += block->DiskReadRawDelta.Delta + block->DiskWriteRawDelta.Delta;
                break;
            case ETPRTNC_NETWORKRECEIVES:
                number += block->NetworkReceiveCount;
                break;
            case ETPRTNC_NETWORKSENDS:
                number += block->NetworkSendCount;
                break;
            case ETPRTNC_NETWORKRECEIVEBYTES:
                number += block->NetworkReceiveRaw;
                break;
            case ETPRTNC_NETWORKSENDBYTES:
                number += block->NetworkSendRaw;
                break;
            case ETPRTNC_NETWORKTOTALBYTES:
                number += block->NetworkReceiveRaw + block->NetworkSendRaw;
                break;
            case ETPRTNC_NETWORKRECEIVESDELTA:
                number += block->NetworkReceiveDelta.Delta;
                break;
            case ETPRTNC_NETWORKSENDSDELTA:
                number += block->NetworkSendDelta.Delta;
                break;
            case ETPRTNC_NETWORKRECEIVEBYTESDELTA:
                number += block->NetworkReceiveRawDelta.Delta;
                break;
            case ETPRTNC_NETWORKSENDBYTESDELTA:
                number += block->NetworkSendRawDelta.Delta;
                break;
            case ETPRTNC_NETWORKTOTALBYTESDELTA:
                number += block->NetworkReceiveRawDelta.Delta + block->NetworkSendRawDelta.Delta;
                break;
            case ETPRTNC_HARDFAULTS:
                number += block->ProcessItem->HardFaultsDelta.Value;
                break;
            case ETPRTNC_HARDFAULTSDELTA:
                number += block->ProcessItem->HardFaultsDelta.Delta;
                break;
            case ETPRTNC_PEAKTHREADS:
                number += block->ProcessItem->PeakNumberOfThreads;
                break;
            case ETPRTNC_GPU:
                decimal += block->GpuNodeUtilization;
                break;
            case ETPRTNC_GPUDEDICATEDBYTES:
                number += block->GpuDedicatedUsage;
                break;
            case ETPRTNC_GPUSHAREDBYTES:
                number += block->GpuSharedUsage;
                break;
            case ETPRTNC_DISKREADRATE:
                number += block->DiskReadRawDelta.Delta;
                break;
            case ETPRTNC_DISKWRITERATE:
                number += block->DiskWriteRawDelta.Delta;
                break;
            case ETPRTNC_DISKTOTALRATE:
                number += block->DiskReadRawDelta.Delta + block->DiskWriteRawDelta.Delta;
                break;
            case ETPRTNC_NETWORKRECEIVERATE:
                number += block->NetworkReceiveRawDelta.Delta;
                break;
            case ETPRTNC_NETWORKSENDRATE:
                number += block->NetworkSendRawDelta.Delta;
                break;
            case ETPRTNC_NETWORKTOTALRATE:
                number += block->NetworkReceiveRawDelta.Delta + block->NetworkSendRawDelta.Delta;
                break;
            case ETPRTNC_FPS:
                decimal += block->FramesPerSecond;
                break;
            }

            listEntry = listEntry->Flink;
        }

        switch (message->SubId)
        {
        case ETPRTNC_DISKREADS:
        case ETPRTNC_DISKREADSDELTA:
        case ETPRTNC_DISKWRITES:
        case ETPRTNC_DISKWRITESDELTA:
        case ETPRTNC_HARDFAULTS:
        case ETPRTNC_HARDFAULTSDELTA:
        case ETPRTNC_NETWORKRECEIVES:
        case ETPRTNC_NETWORKRECEIVESDELTA:
        case ETPRTNC_NETWORKSENDS:
        case ETPRTNC_NETWORKSENDSDELTA:
        case ETPRTNC_PEAKTHREADS:
            {
                PH_FORMAT format[1];

                if (number == 0)
                    break;

                PhInitFormatI64UGroupDigits(&format[0], number);

                if (PhFormatToBuffer(format, RTL_NUMBER_OF(format), getHeaderText->TextCache, getHeaderText->TextCacheSize, &returnLength))
                {
                    getHeaderText->Text.Buffer = getHeaderText->TextCache;
                    getHeaderText->Text.Length = returnLength - sizeof(UNICODE_NULL);
                }
            }
            break;
        case ETPRTNC_DISKREADBYTES:
        case ETPRTNC_DISKWRITEBYTES:
        case ETPRTNC_DISKTOTALBYTES:
        case ETPRTNC_DISKREADBYTESDELTA:
        case ETPRTNC_DISKWRITEBYTESDELTA:
        case ETPRTNC_DISKTOTALBYTESDELTA:
        case ETPRTNC_NETWORKRECEIVEBYTES:
        case ETPRTNC_NETWORKSENDBYTES:
        case ETPRTNC_NETWORKTOTALBYTES:
        case ETPRTNC_NETWORKRECEIVEBYTESDELTA:
        case ETPRTNC_NETWORKSENDBYTESDELTA:
        case ETPRTNC_NETWORKTOTALBYTESDELTA:
        case ETPRTNC_GPUDEDICATEDBYTES:
        case ETPRTNC_GPUSHAREDBYTES:
            {
                PH_FORMAT format[1];

                if (number == 0)
                    break;

                PhInitFormatSize(&format[0], number);

                if (PhFormatToBuffer(format, RTL_NUMBER_OF(format), getHeaderText->TextCache, getHeaderText->TextCacheSize, &returnLength))
                {
                    getHeaderText->Text.Buffer = getHeaderText->TextCache;
                    getHeaderText->Text.Length = returnLength - sizeof(UNICODE_NULL);
                }
            }
            break;
        case ETPRTNC_DISKREADRATE:
        case ETPRTNC_DISKWRITERATE:
        case ETPRTNC_DISKTOTALRATE:
        case ETPRTNC_NETWORKRECEIVERATE:
        case ETPRTNC_NETWORKSENDRATE:
        case ETPRTNC_NETWORKTOTALRATE:
            {
                PH_FORMAT format[2];
                ULONG64 value;

                value = number;
                value *= 1000;
                value /= EtUpdateInterval;

                if (value == 0)
                    break;

                PhInitFormatSize(&format[0], value);
                PhInitFormatS(&format[1], L"/s");

                if (PhFormatToBuffer(format, RTL_NUMBER_OF(format), getHeaderText->TextCache, getHeaderText->TextCacheSize, &returnLength))
                {
                    getHeaderText->Text.Buffer = getHeaderText->TextCache;
                    getHeaderText->Text.Length = returnLength - sizeof(UNICODE_NULL);
                }
            }
            break;
        case ETPRTNC_GPU:
            {
                PH_FORMAT format[2];

                if (decimal == 0.0)
                    break;

                decimal *= 100;
                PhInitFormatF(&format[0], decimal, 2);
                PhInitFormatC(&format[1], L'%');

                if (PhFormatToBuffer(format, RTL_NUMBER_OF(format), getHeaderText->TextCache, getHeaderText->TextCacheSize, &returnLength))
                {
                    getHeaderText->Text.Buffer = getHeaderText->TextCache;
                    getHeaderText->Text.Length = returnLength - sizeof(UNICODE_NULL);
                }
            }
            break;
        case ETPRTNC_FPS:
            {
                PH_FORMAT format[1];

                if (decimal == 0.0)
                    break;

                PhInitFormatF(&format[0], decimal, 2);

                if (PhFormatToBuffer(format, RTL_NUMBER_OF(format), getHeaderText->TextCache, getHeaderText->TextCacheSize, &returnLength))
                {
                    getHeaderText->Text.Buffer = getHeaderText->TextCache;
                    getHeaderText->Text.Length = returnLength - sizeof(UNICODE_NULL);
                }
            }
            break;
        }
    }
}

LONG EtpProcessTreeNewSortFunction(
    _In_ PVOID Node1,
    _In_ PVOID Node2,
    _In_ ULONG SubId,
    _In_ PH_SORT_ORDER SortOrder,
    _In_ PVOID Context
    )
{
    LONG result;
    PPH_PROCESS_NODE node1 = Node1;
    PPH_PROCESS_NODE node2 = Node2;
    PET_PROCESS_BLOCK block1;
    PET_PROCESS_BLOCK block2;

    block1 = EtGetProcessBlock(node1->ProcessItem);
    block2 = EtGetProcessBlock(node2->ProcessItem);

    result = 0;

    switch (SubId)
    {
    case ETPRTNC_DISKREADS:
        result = uint64cmp(block1->DiskReadCount, block2->DiskReadCount);
        break;
    case ETPRTNC_DISKWRITES:
        result = uint64cmp(block1->DiskWriteCount, block2->DiskWriteCount);
        break;
    case ETPRTNC_DISKREADBYTES:
        result = uint64cmp(block1->DiskReadRaw, block2->DiskReadRaw);
        break;
    case ETPRTNC_DISKWRITEBYTES:
        result = uint64cmp(block1->DiskWriteRaw, block2->DiskWriteRaw);
        break;
    case ETPRTNC_DISKTOTALBYTES:
        result = uint64cmp(block1->DiskReadRaw + block1->DiskWriteRaw, block2->DiskReadRaw + block2->DiskWriteRaw);
        break;
    case ETPRTNC_DISKREADSDELTA:
        result = uint64cmp(block1->DiskReadDelta.Delta, block2->DiskReadDelta.Delta);
        break;
    case ETPRTNC_DISKWRITESDELTA:
        result = uint64cmp(block1->DiskWriteDelta.Delta, block2->DiskWriteDelta.Delta);
        break;
    case ETPRTNC_DISKREADBYTESDELTA:
        result = uint64cmp(block1->DiskReadRawDelta.Delta, block2->DiskReadRawDelta.Delta);
        break;
    case ETPRTNC_DISKWRITEBYTESDELTA:
        result = uint64cmp(block1->DiskWriteRawDelta.Delta, block2->DiskWriteRawDelta.Delta);
        break;
    case ETPRTNC_DISKTOTALBYTESDELTA:
        result = uint64cmp(block1->DiskReadRawDelta.Delta + block1->DiskWriteRawDelta.Delta, block2->DiskReadRawDelta.Delta + block2->DiskWriteRawDelta.Delta);
        break;
    case ETPRTNC_NETWORKRECEIVES:
        result = uint64cmp(block1->NetworkReceiveCount, block2->NetworkReceiveCount);
        break;
    case ETPRTNC_NETWORKSENDS:
        result = uint64cmp(block1->NetworkSendCount, block2->NetworkSendCount);
        break;
    case ETPRTNC_NETWORKRECEIVEBYTES:
        result = uint64cmp(block1->NetworkReceiveRaw, block2->NetworkReceiveRaw);
        break;
    case ETPRTNC_NETWORKSENDBYTES:
        result = uint64cmp(block1->NetworkSendRaw, block2->NetworkSendRaw);
        break;
    case ETPRTNC_NETWORKTOTALBYTES:
        result = uint64cmp(block1->NetworkReceiveRaw + block1->NetworkSendRaw, block2->NetworkReceiveRaw + block2->NetworkSendRaw);
        break;
    case ETPRTNC_NETWORKRECEIVESDELTA:
        result = uint64cmp(block1->NetworkReceiveDelta.Delta, block2->NetworkReceiveDelta.Delta);
        break;
    case ETPRTNC_NETWORKSENDSDELTA:
        result = uint64cmp(block1->NetworkSendDelta.Delta, block2->NetworkSendDelta.Delta);
        break;
    case ETPRTNC_NETWORKRECEIVEBYTESDELTA:
        result = uint64cmp(block1->NetworkReceiveRawDelta.Delta, block2->NetworkReceiveRawDelta.Delta);
        break;
    case ETPRTNC_NETWORKSENDBYTESDELTA:
        result = uint64cmp(block1->NetworkSendRawDelta.Delta, block2->NetworkSendRawDelta.Delta);
        break;
    case ETPRTNC_NETWORKTOTALBYTESDELTA:
        result = uint64cmp(block1->NetworkReceiveRawDelta.Delta + block1->NetworkSendRawDelta.Delta, block2->NetworkReceiveRawDelta.Delta + block2->NetworkSendRawDelta.Delta);
        break;
    case ETPRTNC_HARDFAULTS:
        result = uintcmp(block1->ProcessItem->HardFaultsDelta.Value, block2->ProcessItem->HardFaultsDelta.Value);
        break;
    case ETPRTNC_HARDFAULTSDELTA:
        result = uintcmp(block1->ProcessItem->HardFaultsDelta.Delta, block2->ProcessItem->HardFaultsDelta.Delta);
        break;
    case ETPRTNC_PEAKTHREADS:
        result = uintcmp(block1->ProcessItem->PeakNumberOfThreads, block2->ProcessItem->PeakNumberOfThreads);
        break;
    case ETPRTNC_GPU:
        result = singlecmp(block1->GpuNodeUtilization, block2->GpuNodeUtilization);
        break;
    case ETPRTNC_GPUDEDICATEDBYTES:
        result = uint64cmp(block1->GpuDedicatedUsage, block2->GpuDedicatedUsage);
        break;
    case ETPRTNC_GPUSHAREDBYTES:
        result = uint64cmp(block1->GpuSharedUsage, block2->GpuSharedUsage);
        break;
    case ETPRTNC_DISKREADRATE:
        result = uint64cmp(block1->DiskReadRawDelta.Delta, block2->DiskReadRawDelta.Delta);
        break;
    case ETPRTNC_DISKWRITERATE:
        result = uint64cmp(block1->DiskWriteRawDelta.Delta, block2->DiskWriteRawDelta.Delta);
        break;
    case ETPRTNC_DISKTOTALRATE:
        result = uint64cmp(block1->DiskReadRawDelta.Delta + block1->DiskWriteRawDelta.Delta, block2->DiskReadRawDelta.Delta + block2->DiskWriteRawDelta.Delta);
        break;
    case ETPRTNC_NETWORKRECEIVERATE:
        result = uint64cmp(block1->NetworkReceiveRawDelta.Delta, block2->NetworkReceiveRawDelta.Delta);
        break;
    case ETPRTNC_NETWORKSENDRATE:
        result = uint64cmp(block1->NetworkSendRawDelta.Delta, block2->NetworkSendRawDelta.Delta);
        break;
    case ETPRTNC_NETWORKTOTALRATE:
        result = uint64cmp(block1->NetworkReceiveRawDelta.Delta + block1->NetworkSendRawDelta.Delta, block2->NetworkReceiveRawDelta.Delta + block2->NetworkSendRawDelta.Delta);
        break;
    case ETPRTNC_FPS:
        result = singlecmp(block1->FramesPerSecond, block2->FramesPerSecond);
        break;
    }

    return result;
}

VOID EtNetworkTreeNewInitializing(
    _In_ PVOID Parameter
    )
{
    static COLUMN_INFO columns[] =
    {
        { ETNETNC_RECEIVES, L"Receives", 70, PH_ALIGN_RIGHT, DT_RIGHT, TRUE },
        { ETNETNC_SENDS, L"Sends", 70, PH_ALIGN_RIGHT, DT_RIGHT, TRUE },
        { ETNETNC_RECEIVEBYTES, L"Receive bytes", 70, PH_ALIGN_RIGHT, DT_RIGHT, TRUE },
        { ETNETNC_SENDBYTES, L"Send bytes", 70, PH_ALIGN_RIGHT, DT_RIGHT, TRUE },
        { ETNETNC_TOTALBYTES, L"Total bytes", 70, PH_ALIGN_RIGHT, DT_RIGHT, TRUE },
        { ETNETNC_RECEIVESDELTA, L"Receives delta", 70, PH_ALIGN_RIGHT, DT_RIGHT, TRUE },
        { ETNETNC_SENDSDELTA, L"Sends delta", 70, PH_ALIGN_RIGHT, DT_RIGHT, TRUE },
        { ETNETNC_RECEIVEBYTESDELTA, L"Receive bytes delta", 70, PH_ALIGN_RIGHT, DT_RIGHT, TRUE },
        { ETNETNC_SENDBYTESDELTA, L"Send bytes delta", 70, PH_ALIGN_RIGHT, DT_RIGHT, TRUE },
        { ETNETNC_TOTALBYTESDELTA, L"Total bytes delta", 70, PH_ALIGN_RIGHT, DT_RIGHT, TRUE },
        { ETNETNC_FIREWALLSTATUS, L"Firewall status", 170, PH_ALIGN_LEFT, 0, FALSE },
        { ETNETNC_RECEIVERATE, L"Receive rate", 70, PH_ALIGN_RIGHT, DT_RIGHT, TRUE },
        { ETNETNC_SENDRATE, L"Send rate", 70, PH_ALIGN_RIGHT, DT_RIGHT, TRUE },
        { ETNETNC_TOTALRATE, L"Total rate", 70, PH_ALIGN_RIGHT, DT_RIGHT, TRUE }
    };

    PPH_PLUGIN_TREENEW_INFORMATION treeNewInfo = Parameter;
    ULONG i;

    for (i = 0; i < sizeof(columns) / sizeof(COLUMN_INFO); i++)
    {
        EtpAddTreeNewColumn(treeNewInfo, columns[i].SubId, columns[i].Text, columns[i].Width, columns[i].Alignment,
            columns[i].TextFlags, columns[i].SortDescending, EtpNetworkTreeNewSortFunction);
    }
}

VOID EtpUpdateFirewallStatus(
    _Inout_ PET_NETWORK_BLOCK Block
    )
{
    if (!Block->FirewallStatusValid)
    {
        Block->FirewallStatus = EtQueryFirewallStatus(Block->NetworkItem);
        Block->FirewallStatusValid = TRUE;
    }
}

VOID EtNetworkTreeNewMessage(
    _In_ PVOID Parameter
    )
{
    PPH_PLUGIN_TREENEW_MESSAGE message = Parameter;

    if (message->Message == TreeNewGetCellText)
    {
        PPH_TREENEW_GET_CELL_TEXT getCellText = message->Parameter1;
        PPH_NETWORK_NODE networkNode = (PPH_NETWORK_NODE)getCellText->Node;
        PET_NETWORK_BLOCK block = EtGetNetworkBlock(networkNode->NetworkItem);

        PhAcquireQueuedLockExclusive(&block->TextCacheLock);

        if (block->TextCacheValid[message->SubId])
        {
            if (block->TextCacheLength[message->SubId])
            {
                getCellText->Text.Length = block->TextCacheLength[message->SubId];
                getCellText->Text.Buffer = block->TextCache[message->SubId];
            }
        }
        else
        {
            switch (message->SubId)
            {
            case ETNETNC_RECEIVES:
                EtFormatNetworkInt64(block->ReceiveCount, block, message);
                break;
            case ETNETNC_SENDS:
                EtFormatNetworkInt64(block->SendCount, block, message);
                break;
            case ETNETNC_RECEIVEBYTES:
                EtFormatNetworkSize(block->ReceiveRaw, block, message);
                break;
            case ETNETNC_SENDBYTES:
                EtFormatNetworkSize(block->SendRaw, block, message);
                break;
            case ETNETNC_TOTALBYTES:
                EtFormatNetworkSize(block->ReceiveRaw + block->SendRaw, block, message);
                break;
            case ETNETNC_RECEIVESDELTA:
                EtFormatNetworkInt64(block->ReceiveDelta.Delta, block, message);
                break;
            case ETNETNC_SENDSDELTA:
                EtFormatNetworkInt64(block->SendDelta.Delta, block, message);
                break;
            case ETNETNC_RECEIVEBYTESDELTA:
                EtFormatNetworkSize(block->ReceiveRawDelta.Delta, block, message);
                break;
            case ETNETNC_SENDBYTESDELTA:
                EtFormatNetworkSize(block->SendRawDelta.Delta, block, message);
                break;
            case ETNETNC_TOTALBYTESDELTA:
                EtFormatNetworkSize(block->ReceiveRawDelta.Delta + block->SendRawDelta.Delta, block, message);
                break;
            case ETNETNC_FIREWALLSTATUS:
                {
                    EtpUpdateFirewallStatus(block);

                    if (block->FirewallStatus >= FirewallUnknownStatus && block->FirewallStatus < FirewallMaximumStatus)
                    {
                        static PH_STRINGREF strings[FirewallMaximumStatus] =
                        {
                            PH_STRINGREF_INIT(L"Unknown"),
                            PH_STRINGREF_INIT(L"Allowed, not restricted"),
                            PH_STRINGREF_INIT(L"Allowed, restricted"),
                            PH_STRINGREF_INIT(L"Not allowed, not restricted"),
                            PH_STRINGREF_INIT(L"Not allowed, restricted")
                        };

                        block->TextCacheLength[message->SubId] = strings[block->FirewallStatus].Length;
                        memcpy_s(
                            block->TextCache[message->SubId],
                            sizeof(block->TextCache[message->SubId]),
                            strings[block->FirewallStatus].Buffer,
                            strings[block->FirewallStatus].Length
                            );
                    }
                }
                break;
            case ETNETNC_RECEIVERATE:
                EtFormatNetworkRate(block->ReceiveRawDelta.Delta, block, message);
                break;
            case ETNETNC_SENDRATE:
                EtFormatNetworkRate(block->SendRawDelta.Delta, block, message);
                break;
            case ETNETNC_TOTALRATE:
                EtFormatNetworkRate(block->ReceiveRawDelta.Delta + block->SendRawDelta.Delta, block, message);
                break;
            }

            if (block->TextCacheLength[message->SubId])
            {
                getCellText->Text.Length = block->TextCacheLength[message->SubId];
                getCellText->Text.Buffer = block->TextCache[message->SubId];
            }

            block->TextCacheValid[message->SubId] = TRUE;
        }

        PhReleaseQueuedLockExclusive(&block->TextCacheLock);
    }
}

LONG EtpNetworkTreeNewSortFunction(
    _In_ PVOID Node1,
    _In_ PVOID Node2,
    _In_ ULONG SubId,
    _In_ PH_SORT_ORDER SortOrder,
    _In_ PVOID Context
    )
{
    LONG result;
    PPH_NETWORK_NODE node1 = Node1;
    PPH_NETWORK_NODE node2 = Node2;
    PET_NETWORK_BLOCK block1;
    PET_NETWORK_BLOCK block2;

    block1 = EtGetNetworkBlock(node1->NetworkItem);
    block2 = EtGetNetworkBlock(node2->NetworkItem);

    result = 0;

    switch (SubId)
    {
    case ETNETNC_RECEIVES:
        result = uint64cmp(block1->ReceiveCount, block2->ReceiveCount);
        break;
    case ETNETNC_SENDS:
        result = uint64cmp(block1->SendCount, block2->SendCount);
        break;
    case ETNETNC_RECEIVEBYTES:
        result = uint64cmp(block1->ReceiveRaw, block2->ReceiveRaw);
        break;
    case ETNETNC_SENDBYTES:
        result = uint64cmp(block1->SendRaw, block2->SendRaw);
        break;
    case ETNETNC_TOTALBYTES:
        result = uint64cmp(block1->ReceiveRaw + block1->SendRaw, block2->ReceiveRaw + block2->SendRaw);
        break;
    case ETNETNC_RECEIVESDELTA:
        result = uint64cmp(block1->ReceiveDelta.Delta, block2->ReceiveDelta.Delta);
        break;
    case ETNETNC_SENDSDELTA:
        result = uint64cmp(block1->SendDelta.Delta, block2->SendDelta.Delta);
        break;
    case ETNETNC_RECEIVEBYTESDELTA:
        result = uint64cmp(block1->ReceiveRawDelta.Delta, block2->ReceiveRawDelta.Delta);
        break;
    case ETNETNC_SENDBYTESDELTA:
        result = uint64cmp(block1->SendRawDelta.Delta, block2->SendRawDelta.Delta);
        break;
    case ETNETNC_TOTALBYTESDELTA:
        result = uint64cmp(block1->ReceiveRawDelta.Delta + block1->SendRawDelta.Delta, block2->ReceiveRawDelta.Delta + block2->SendRawDelta.Delta);
        break;
    case ETNETNC_FIREWALLSTATUS:
        EtpUpdateFirewallStatus(block1);
        EtpUpdateFirewallStatus(block2);
        result = intcmp(block1->FirewallStatus, block2->FirewallStatus);
        break;
    case ETNETNC_RECEIVERATE:
        result = uint64cmp(block1->ReceiveRawDelta.Delta, block2->ReceiveRawDelta.Delta);
        break;
    case ETNETNC_SENDRATE:
        result = uint64cmp(block1->SendRawDelta.Delta, block2->SendRawDelta.Delta);
        break;
    case ETNETNC_TOTALRATE:
        result = uint64cmp(block1->ReceiveRawDelta.Delta + block1->SendRawDelta.Delta, block2->ReceiveRawDelta.Delta + block2->SendRawDelta.Delta);
        break;
    }

    return result;
}

ET_FIREWALL_STATUS EtQueryFirewallStatus(
    _In_ PPH_NETWORK_ITEM NetworkItem
    )
{
    static INetFwMgr* manager = NULL;
    ET_FIREWALL_STATUS result;
    PPH_PROCESS_ITEM processItem;
    BSTR imageFileNameBStr;
    BSTR localAddressBStr;
    VARIANT allowed;
    VARIANT restricted;

    if (!manager)
    {
        if (!SUCCEEDED(PhGetClassObject(L"firewallapi.dll", &CLSID_NetFwMgr_I, &IID_INetFwMgr_I, &manager)))
            return FirewallUnknownStatus;

        if (!manager)
            return FirewallUnknownStatus;
    }

    processItem = PhReferenceProcessItem(NetworkItem->ProcessId);

    if (!processItem)
        return FirewallUnknownStatus;

    if (!processItem->FileNameWin32)
    {
        PhDereferenceObject(processItem);
        return FirewallUnknownStatus;
    }

    result = FirewallUnknownStatus;

    if (imageFileNameBStr = SysAllocStringLen(processItem->FileNameWin32->Buffer, (ULONG)processItem->FileNameWin32->Length / sizeof(WCHAR)))
    {
        localAddressBStr = NULL;

        if (!PhIsNullIpAddress(&NetworkItem->LocalEndpoint.Address))
            localAddressBStr = SysAllocString(NetworkItem->LocalAddressString);

        memset(&allowed, 0, sizeof(VARIANT)); // VariantInit
        memset(&restricted, 0, sizeof(VARIANT)); // VariantInit

        if (SUCCEEDED(INetFwMgr_IsPortAllowed(
            manager,
            imageFileNameBStr,
            (NetworkItem->ProtocolType & PH_IPV6_NETWORK_TYPE) ? NET_FW_IP_VERSION_V6 : NET_FW_IP_VERSION_V4,
            NetworkItem->LocalEndpoint.Port,
            localAddressBStr,
            (NetworkItem->ProtocolType & PH_UDP_PROTOCOL_TYPE) ? NET_FW_IP_PROTOCOL_UDP : NET_FW_IP_PROTOCOL_TCP,
            &allowed,
            &restricted
            )))
        {
            if (V_BOOL(&allowed))
            {
                if (V_BOOL(&restricted))
                    result = FirewallAllowedRestricted;
                else
                    result = FirewallAllowedNotRestricted;
            }
            else
            {
                if (V_BOOL(&restricted))
                    result = FirewallNotAllowedRestricted;
                else
                    result = FirewallNotAllowedNotRestricted;
            }
        }

        if (localAddressBStr)
            SysFreeString(localAddressBStr);

        SysFreeString(imageFileNameBStr);
    }

    PhDereferenceObject(processItem);

    return result;
}
