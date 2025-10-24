/*
 * Copyright (c) 2022 Winsider Seminars & Solutions, Inc.  All rights reserved.
 *
 * This file is part of System Informer.
 *
 * Authors:
 *
 *     jxy-s   2022-2025
 *     dmex    2022
 *
 */

#include <kphlibbase.h>
#include <kphmsg.h>

#define KPH_MESSAGE_VERSION 4

/**
 * Gets the current system time (UTC).
 *
 * \remarks Use this function instead of NtQuerySystemTime() because no system calls are involved.
 */
VOID KphMsgQuerySystemTime(
    _Out_ PLARGE_INTEGER SystemTime
    )
{
#ifdef _KERNEL_MODE
    KeQuerySystemTime(SystemTime);
#else
    while (TRUE)
    {
        SystemTime->HighPart = USER_SHARED_DATA->SystemTime.High1Time;
        SystemTime->LowPart = USER_SHARED_DATA->SystemTime.LowPart;

        if (SystemTime->HighPart == USER_SHARED_DATA->SystemTime.High2Time)
            break;

        YieldProcessor();
    }
#endif
}

/**
 * \brief Initializes a message.
 *
 * \param MessageId Message identifier to initialize with.
 */
VOID KphMsgInit(
    _Out_writes_bytes_(KPH_MESSAGE_MIN_SIZE) PKPH_MESSAGE Message,
    _In_ KPH_MESSAGE_ID MessageId
    )
{
    RtlZeroMemory(Message, KPH_MESSAGE_MIN_SIZE);
    Message->Header.Version = KPH_MESSAGE_VERSION;
    Message->Header.Size = KPH_MESSAGE_MIN_SIZE;
    Message->Header.MessageId = MessageId;
    KphMsgQuerySystemTime(&Message->Header.TimeStamp);
}

/**
 * \brief Validates a message.
 *
 * \param Message The message to validate.
 *
 * \return Successful or errant status.
 */
_Must_inspect_result_
NTSTATUS KphMsgValidate(
    _In_ PCKPH_MESSAGE Message
    )
{
    if (Message->Header.Version != KPH_MESSAGE_VERSION)
    {
        return STATUS_REVISION_MISMATCH;
    }

    if ((Message->Header.Size < KPH_MESSAGE_MIN_SIZE) ||
        (Message->Header.Size > sizeof(KPH_MESSAGE)))
    {
        return STATUS_INVALID_MESSAGE;
    }

    if (Message->Header.MessageId != KphMsgUnhandled)
    {
        if ((Message->Header.MessageId <= InvalidKphMsg) ||
            (Message->Header.MessageId >= MaxKphMsg))
        {
            return STATUS_INVALID_MESSAGE;
        }
    }

    return STATUS_SUCCESS;
}
