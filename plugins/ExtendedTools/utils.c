/*
 * Copyright (c) 2022 Winsider Seminars & Solutions, Inc.  All rights reserved.
 *
 * This file is part of System Informer.
 *
 * Authors:
 *
 *     wj32    2011
 *     dmex    2020
 *
 */

#include "exttools.h"

VOID EtFormatToBuffer(
    _In_reads_(Count) PPH_FORMAT Format,
    _In_ ULONG Count,
    _In_ PET_PROCESS_BLOCK Block,
    _In_ PPH_PLUGIN_TREENEW_MESSAGE Message
    )
{
    SIZE_T returnLength;

    if (PhFormatToBuffer(Format, Count, Block->TextCache[Message->SubId], sizeof(Block->TextCache[Message->SubId]), &returnLength))
        Block->TextCacheLength[Message->SubId] = returnLength - sizeof(UNICODE_NULL);
    else
        Block->TextCacheLength[Message->SubId] = 0;
}

VOID EtFormatToBufferNetwork(
    _In_reads_(Count) PPH_FORMAT Format,
    _In_ ULONG Count,
    _In_ PET_NETWORK_BLOCK Block,
    _In_ PPH_PLUGIN_TREENEW_MESSAGE Message
    )
{
    SIZE_T returnLength;

    if (PhFormatToBuffer(Format, Count, Block->TextCache[Message->SubId], sizeof(Block->TextCache[Message->SubId]), &returnLength))
        Block->TextCacheLength[Message->SubId] = returnLength - sizeof(UNICODE_NULL);
    else
        Block->TextCacheLength[Message->SubId] = 0;
}

VOID EtFormatInt64(
    _In_ ULONG64 Value,
    _In_ PET_PROCESS_BLOCK Block,
    _In_ PPH_PLUGIN_TREENEW_MESSAGE Message
    )
{
    if (Value != 0)
    {
        PH_FORMAT format[1];

        PhInitFormatI64U(&format[0], Value);

        EtFormatToBuffer(format, 1, Block, Message);
    }
    else
    {
        Block->TextCacheLength[Message->SubId] = 0;
    }
}

VOID EtFormatNetworkInt64(
    _In_ ULONG64 Value,
    _In_ PET_NETWORK_BLOCK Block,
    _In_ PPH_PLUGIN_TREENEW_MESSAGE Message
    )
{
    if (Value != 0)
    {
        PH_FORMAT format[1];

        PhInitFormatI64U(&format[0], Value);

        EtFormatToBufferNetwork(format, 1, Block, Message);
    }
    else
    {
        Block->TextCacheLength[Message->SubId] = 0;
    }
}

VOID EtFormatSize(
    _In_ ULONG64 Value,
    _In_ PET_PROCESS_BLOCK Block,
    _In_ PPH_PLUGIN_TREENEW_MESSAGE Message
    )
{
    if (Value != 0)
    {
        PH_FORMAT format[1];

        PhInitFormatSize(&format[0], Value);

        EtFormatToBuffer(format, 1, Block, Message);
    }
    else
    {
        Block->TextCacheLength[Message->SubId] = 0;
    }
}

VOID EtFormatNetworkSize(
    _In_ ULONG64 Value,
    _In_ PET_NETWORK_BLOCK Block,
    _In_ PPH_PLUGIN_TREENEW_MESSAGE Message
    )
{
    if (Value != 0)
    {
        PH_FORMAT format[1];

        PhInitFormatSize(&format[0], Value);

        EtFormatToBufferNetwork(format, 1, Block, Message);
    }
    else
    {
        Block->TextCacheLength[Message->SubId] = 0;
    }
}

VOID EtFormatDouble(
    _In_ DOUBLE Value,
    _In_ PET_PROCESS_BLOCK Block,
    _In_ PPH_PLUGIN_TREENEW_MESSAGE Message
    )
{
    if (Value >= 0.01)
    {
        PH_FORMAT format[1];

        PhInitFormatF(&format[0], Value, 2);

        EtFormatToBuffer(format, 1, Block, Message);
    }
    else
    {
        Block->TextCacheLength[Message->SubId] = 0;
    }
}

VOID EtFormatRate(
    _In_ ULONG64 ValuePerPeriod,
    _In_ PET_PROCESS_BLOCK Block,
    _In_ PPH_PLUGIN_TREENEW_MESSAGE Message
    )
{
    ULONG64 number;

    number = ValuePerPeriod;
    number *= 1000;
    number /= EtUpdateInterval;

    if (number != 0)
    {
        PH_FORMAT format[2];

        PhInitFormatSize(&format[0], number);
        PhInitFormatS(&format[1], L"/s");

        EtFormatToBuffer(format, 2, Block, Message);
    }
    else
    {
        Block->TextCacheLength[Message->SubId] = 0;
    }
}

VOID EtFormatNetworkRate(
    _In_ ULONG64 ValuePerPeriod,
    _In_ PET_NETWORK_BLOCK Block,
    _In_ PPH_PLUGIN_TREENEW_MESSAGE Message
    )
{
    ULONG64 number;

    number = ValuePerPeriod;
    number *= 1000;
    number /= EtUpdateInterval;

    if (number != 0)
    {
        PH_FORMAT format[2];

        PhInitFormatSize(&format[0], number);
        PhInitFormatS(&format[1], L"/s");

        EtFormatToBufferNetwork(format, 2, Block, Message);
    }
    else
    {
        Block->TextCacheLength[Message->SubId] = 0;
    }
}
