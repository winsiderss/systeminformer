/*
 * Process Hacker Extended Tools - 
 *   ETW process tree support
 * 
 * Copyright (C) 2011 wj32
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

#include "exttools.h"

LONG EtpProcessTreeNewSortFunction(
    __in PVOID Node1,
    __in PVOID Node2,
    __in ULONG SubId,
    __in PVOID Context
    );

VOID EtpAddTreeNewColumn(
    __in PPH_PLUGIN_TREENEW_INFORMATION TreeNewInfo,
    __in ULONG SubId,
    __in PWSTR Text,
    __in ULONG Width,
    __in ULONG Alignment,
    __in ULONG TextFlags
    )
{
    PH_TREENEW_COLUMN column;

    memset(&column, 0, sizeof(PH_TREENEW_COLUMN));
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
        EtpProcessTreeNewSortFunction
        );
}

VOID EtEtwProcessTreeNewInitializing(
    __in PVOID Parameter
    )
{
    PPH_PLUGIN_TREENEW_INFORMATION treeNewInfo = Parameter;

    EtpAddTreeNewColumn(treeNewInfo, ETETWTNC_DISKREADS, L"Disk Reads", 70, PH_ALIGN_RIGHT, DT_RIGHT);
    EtpAddTreeNewColumn(treeNewInfo, ETETWTNC_DISKWRITES, L"Disk Writes", 70, PH_ALIGN_RIGHT, DT_RIGHT);
    EtpAddTreeNewColumn(treeNewInfo, ETETWTNC_DISKREADBYTES, L"Disk Read Bytes", 70, PH_ALIGN_RIGHT, DT_RIGHT);
    EtpAddTreeNewColumn(treeNewInfo, ETETWTNC_DISKWRITEBYTES, L"Disk Write Bytes", 70, PH_ALIGN_RIGHT, DT_RIGHT);
    EtpAddTreeNewColumn(treeNewInfo, ETETWTNC_DISKTOTALBYTES, L"Disk Total Bytes", 70, PH_ALIGN_RIGHT, DT_RIGHT);
    EtpAddTreeNewColumn(treeNewInfo, ETETWTNC_DISKREADSDELTA, L"Disk Reads Delta", 70, PH_ALIGN_RIGHT, DT_RIGHT);
    EtpAddTreeNewColumn(treeNewInfo, ETETWTNC_DISKWRITESDELTA, L"Disk Writes Delta", 70, PH_ALIGN_RIGHT, DT_RIGHT);
    EtpAddTreeNewColumn(treeNewInfo, ETETWTNC_DISKREADBYTESDELTA, L"Disk Read Bytes Delta", 70, PH_ALIGN_RIGHT, DT_RIGHT);
    EtpAddTreeNewColumn(treeNewInfo, ETETWTNC_DISKWRITEBYTESDELTA, L"Disk Write Bytes Delta", 70, PH_ALIGN_RIGHT, DT_RIGHT);
    EtpAddTreeNewColumn(treeNewInfo, ETETWTNC_DISKTOTALBYTESDELTA, L"Disk Total Bytes Delta", 70, PH_ALIGN_RIGHT, DT_RIGHT);
    EtpAddTreeNewColumn(treeNewInfo, ETETWTNC_NETWORKRECEIVES, L"Network Receives", 70, PH_ALIGN_RIGHT, DT_RIGHT);
    EtpAddTreeNewColumn(treeNewInfo, ETETWTNC_NETWORKSENDS, L"Network Sends", 70, PH_ALIGN_RIGHT, DT_RIGHT);
    EtpAddTreeNewColumn(treeNewInfo, ETETWTNC_NETWORKRECEIVEBYTES, L"Network Receive Bytes", 70, PH_ALIGN_RIGHT, DT_RIGHT);
    EtpAddTreeNewColumn(treeNewInfo, ETETWTNC_NETWORKSENDBYTES, L"Network Send Bytes", 70, PH_ALIGN_RIGHT, DT_RIGHT);
    EtpAddTreeNewColumn(treeNewInfo, ETETWTNC_NETWORKTOTALBYTES, L"Network Total Bytes", 70, PH_ALIGN_RIGHT, DT_RIGHT);
    EtpAddTreeNewColumn(treeNewInfo, ETETWTNC_NETWORKRECEIVESDELTA, L"Network Receives Delta", 70, PH_ALIGN_RIGHT, DT_RIGHT);
    EtpAddTreeNewColumn(treeNewInfo, ETETWTNC_NETWORKSENDSDELTA, L"Network Sends Delta", 70, PH_ALIGN_RIGHT, DT_RIGHT);
    EtpAddTreeNewColumn(treeNewInfo, ETETWTNC_NETWORKRECEIVEBYTESDELTA, L"Network Receive Bytes Delta", 70, PH_ALIGN_RIGHT, DT_RIGHT);
    EtpAddTreeNewColumn(treeNewInfo, ETETWTNC_NETWORKSENDBYTESDELTA, L"Network Send Bytes Delta", 70, PH_ALIGN_RIGHT, DT_RIGHT);
    EtpAddTreeNewColumn(treeNewInfo, ETETWTNC_NETWORKTOTALBYTESDELTA, L"Network Total Bytes Delta", 70, PH_ALIGN_RIGHT, DT_RIGHT);
}

VOID EtEtwProcessTreeNewMessage(
    __in PVOID Parameter
    )
{
    PPH_PLUGIN_TREENEW_MESSAGE message = Parameter;

    if (message->Message == TreeNewGetCellText)
    {
        PPH_TREENEW_GET_CELL_TEXT getCellText = message->Parameter1;
        PPH_PROCESS_NODE processNode = (PPH_PROCESS_NODE)getCellText->Node;
        PET_PROCESS_ETW_BLOCK block;
        PPH_STRING text;

        if (!EtEtwEnabled)
            return;

        if (!(block = EtFindProcessEtwBlock(processNode->ProcessItem)))
            return;

        PhAcquireQueuedLockExclusive(&block->TextCacheLock);

        if (block->TextCacheValid[message->SubId])
        {
            if (block->TextCache[message->SubId])
                getCellText->Text = block->TextCache[message->SubId]->sr;
        }
        else
        {
            text = NULL;

            switch (message->SubId)
            {
            case ETETWTNC_DISKREADS:
                if (block->DiskReadCount != 0)
                    text = PhFormatUInt64(block->DiskReadCount, TRUE);
                break;
            case ETETWTNC_DISKWRITES:
                if (block->DiskWriteCount != 0)
                    text = PhFormatUInt64(block->DiskWriteCount, TRUE);
                break;
            case ETETWTNC_DISKREADBYTES:
                if (block->DiskReadRaw != 0)
                    text = PhFormatSize(block->DiskReadRaw, -1);
                break;
            case ETETWTNC_DISKWRITEBYTES:
                if (block->DiskWriteRaw != 0)
                    text = PhFormatSize(block->DiskWriteRaw, -1);
                break;
            case ETETWTNC_DISKTOTALBYTES:
                if (block->DiskReadRaw + block->DiskWriteRaw != 0)
                    text = PhFormatSize(block->DiskReadRaw + block->DiskWriteRaw, -1);
                break;
            case ETETWTNC_DISKREADSDELTA:
                if (block->DiskReadDelta.Delta != 0)
                    text = PhFormatUInt64(block->DiskReadDelta.Delta, TRUE);
                break;
            case ETETWTNC_DISKWRITESDELTA:
                if (block->DiskWriteDelta.Delta != 0)
                    text = PhFormatUInt64(block->DiskWriteDelta.Delta, TRUE);
                break;
            case ETETWTNC_DISKREADBYTESDELTA:
                if (block->DiskReadRawDelta.Delta != 0)
                    text = PhFormatSize(block->DiskReadRawDelta.Delta, -1);
                break;
            case ETETWTNC_DISKWRITEBYTESDELTA:
                if (block->DiskWriteRawDelta.Delta != 0)
                    text = PhFormatSize(block->DiskWriteRawDelta.Delta, -1);
                break;
            case ETETWTNC_DISKTOTALBYTESDELTA:
                if (block->DiskReadRawDelta.Delta + block->DiskWriteRawDelta.Delta != 0)
                    text = PhFormatSize(block->DiskReadRawDelta.Delta + block->DiskWriteRawDelta.Delta, -1);
                break;
            case ETETWTNC_NETWORKRECEIVES:
                if (block->NetworkReceiveCount != 0)
                    text = PhFormatUInt64(block->NetworkReceiveCount, TRUE);
                break;
            case ETETWTNC_NETWORKSENDS:
                if (block->NetworkSendCount != 0)
                    text = PhFormatUInt64(block->NetworkSendCount, TRUE);
                break;
            case ETETWTNC_NETWORKRECEIVEBYTES:
                if (block->NetworkReceiveRaw != 0)
                    text = PhFormatSize(block->NetworkReceiveRaw, -1);
                break;
            case ETETWTNC_NETWORKSENDBYTES:
                if (block->NetworkSendRaw != 0)
                    text = PhFormatSize(block->NetworkSendRaw, -1);
                break;
            case ETETWTNC_NETWORKTOTALBYTES:
                if (block->NetworkReceiveRaw + block->NetworkSendRaw != 0)
                    text = PhFormatSize(block->NetworkReceiveRaw + block->NetworkSendRaw, -1);
                break;
            case ETETWTNC_NETWORKRECEIVESDELTA:
                if (block->NetworkReceiveDelta.Delta != 0)
                    text = PhFormatUInt64(block->NetworkReceiveDelta.Delta, TRUE);
                break;
            case ETETWTNC_NETWORKSENDSDELTA:
                if (block->NetworkSendDelta.Delta != 0)
                    text = PhFormatUInt64(block->NetworkSendDelta.Delta, TRUE);
                break;
            case ETETWTNC_NETWORKRECEIVEBYTESDELTA:
                if (block->NetworkReceiveRawDelta.Delta != 0)
                    text = PhFormatSize(block->NetworkReceiveRawDelta.Delta, -1);
                break;
            case ETETWTNC_NETWORKSENDBYTESDELTA:
                if (block->NetworkSendRawDelta.Delta != 0)
                    text = PhFormatSize(block->NetworkSendRawDelta.Delta, -1);
                break;
            case ETETWTNC_NETWORKTOTALBYTESDELTA:
                if (block->NetworkReceiveRawDelta.Delta + block->NetworkSendRawDelta.Delta != 0)
                    text = PhFormatSize(block->NetworkReceiveRawDelta.Delta + block->NetworkSendRawDelta.Delta, -1);
                break;
            }

            if (text)
            {
                getCellText->Text = text->sr;
            }

            PhSwapReference(&block->TextCache[message->SubId], text);
            block->TextCacheValid[message->SubId] = TRUE;
        }

        PhReleaseQueuedLockExclusive(&block->TextCacheLock);

        EtDereferenceProcessEtwBlock(block);
    }
}

LONG EtpProcessTreeNewSortFunction(
    __in PVOID Node1,
    __in PVOID Node2,
    __in ULONG SubId,
    __in PVOID Context
    )
{
    LONG result;
    PPH_PROCESS_NODE node1 = Node1;
    PPH_PROCESS_NODE node2 = Node2;
    PET_PROCESS_ETW_BLOCK block1;
    PET_PROCESS_ETW_BLOCK block2;

    if (!EtEtwEnabled)
        return 0;

    if (!(block1 = EtFindProcessEtwBlock(node1->ProcessItem)))
    {
        return 0;
    }

    if (!(block2 = EtFindProcessEtwBlock(node2->ProcessItem)))
    {
        EtDereferenceProcessEtwBlock(block1);
        return 0;
    }

    result = 0;

    switch (SubId)
    {
    case ETETWTNC_DISKREADS:
        result = uintcmp(block1->DiskReadCount, block2->DiskReadCount);
        break;
    case ETETWTNC_DISKWRITES:
        result = uintcmp(block1->DiskWriteCount, block2->DiskWriteCount);
        break;
    case ETETWTNC_DISKREADBYTES:
        result = uintcmp(block1->DiskReadRaw, block2->DiskReadRaw);
        break;
    case ETETWTNC_DISKWRITEBYTES:
        result = uintcmp(block1->DiskWriteRaw, block2->DiskWriteRaw);
        break;
    case ETETWTNC_DISKTOTALBYTES:
        result = uintcmp(block1->DiskReadRaw + block1->DiskWriteRaw, block2->DiskReadRaw + block2->DiskWriteRaw);
        break;
    case ETETWTNC_DISKREADSDELTA:
        result = uintcmp(block1->DiskReadDelta.Delta, block2->DiskReadDelta.Delta);
        break;
    case ETETWTNC_DISKWRITESDELTA:
        result = uintcmp(block1->DiskWriteDelta.Delta, block2->DiskWriteDelta.Delta);
        break;
    case ETETWTNC_DISKREADBYTESDELTA:
        result = uintcmp(block1->DiskReadRawDelta.Delta, block2->DiskReadRawDelta.Delta);
        break;
    case ETETWTNC_DISKWRITEBYTESDELTA:
        result = uintcmp(block1->DiskWriteRawDelta.Delta, block2->DiskWriteRawDelta.Delta);
        break;
    case ETETWTNC_DISKTOTALBYTESDELTA:
        result = uintcmp(block1->DiskReadRawDelta.Delta + block1->DiskWriteRawDelta.Delta, block2->DiskReadRawDelta.Delta + block2->DiskWriteRawDelta.Delta);
        break;
    case ETETWTNC_NETWORKRECEIVES:
        result = uintcmp(block1->NetworkReceiveCount, block2->NetworkReceiveCount);
        break;
    case ETETWTNC_NETWORKSENDS:
        result = uintcmp(block1->NetworkSendCount, block2->NetworkSendCount);
        break;
    case ETETWTNC_NETWORKRECEIVEBYTES:
        result = uintcmp(block1->NetworkReceiveRaw, block2->NetworkReceiveRaw);
        break;
    case ETETWTNC_NETWORKSENDBYTES:
        result = uintcmp(block1->NetworkSendRaw, block2->NetworkSendRaw);
        break;
    case ETETWTNC_NETWORKTOTALBYTES:
        result = uintcmp(block1->NetworkReceiveRaw + block1->NetworkSendRaw, block2->NetworkReceiveRaw + block2->NetworkSendRaw);
        break;
    case ETETWTNC_NETWORKRECEIVESDELTA:
        result = uintcmp(block1->NetworkReceiveDelta.Delta, block2->NetworkReceiveDelta.Delta);
        break;
    case ETETWTNC_NETWORKSENDSDELTA:
        result = uintcmp(block1->NetworkSendDelta.Delta, block2->NetworkSendDelta.Delta);
        break;
    case ETETWTNC_NETWORKRECEIVEBYTESDELTA:
        result = uintcmp(block1->NetworkReceiveRawDelta.Delta, block2->NetworkReceiveRawDelta.Delta);
        break;
    case ETETWTNC_NETWORKSENDBYTESDELTA:
        result = uintcmp(block1->NetworkSendRawDelta.Delta, block2->NetworkSendRawDelta.Delta);
        break;
    case ETETWTNC_NETWORKTOTALBYTESDELTA:
        result = uintcmp(block1->NetworkReceiveRawDelta.Delta + block1->NetworkSendRawDelta.Delta, block2->NetworkReceiveRawDelta.Delta + block2->NetworkSendRawDelta.Delta);
        break;
    }

    EtDereferenceProcessEtwBlock(block1);
    EtDereferenceProcessEtwBlock(block2);

    return result;
}
