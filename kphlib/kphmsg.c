/*
 * Copyright (c) 2022 Winsider Seminars & Solutions, Inc.  All rights reserved.
 *
 * This file is part of System Informer.
 *
 * Authors:
 *
 *     jxy-s   2022-2026
 *     dmex    2022
 *
 */

#include <kphlibbase.h>
#include <kphmsg.h>

#define KPH_MESSAGE_VERSION 9

/**
 * \brief Gets the current system time (UTC).
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
        SystemTime->HighPart = ReadAcquire(&USER_SHARED_DATA->SystemTime.High1Time);
        SystemTime->LowPart = (ULONG)ReadAcquire((volatile LONG*)&USER_SHARED_DATA->SystemTime.LowPart);

        if (SystemTime->HighPart == ReadAcquire(&USER_SHARED_DATA->SystemTime.High2Time))
            break;

        YieldProcessor();
    }
#endif
}

/**
 * \brief Gets the current unbiased interrupt time, which excludes time spent asleep.
 */
VOID KphMsgQueryUnbiasedInterruptTime(
    _Out_ PLARGE_INTEGER UnbiasedInterruptTime
    )
{
#ifdef _KERNEL_MODE
    UnbiasedInterruptTime->QuadPart = KeQueryUnbiasedInterruptTime();
#else
    LARGE_INTEGER interruptTime;
    ULONGLONG interruptTimeBias;

    while (TRUE)
    {
        interruptTimeBias = ReadULong64Acquire(&USER_SHARED_DATA->InterruptTimeBias);
        interruptTime.HighPart = ReadAcquire(&USER_SHARED_DATA->InterruptTime.High1Time);
        interruptTime.LowPart = (ULONG)ReadAcquire((volatile LONG*)&USER_SHARED_DATA->InterruptTime.LowPart);

        if ((interruptTime.HighPart == ReadAcquire(&USER_SHARED_DATA->InterruptTime.High2Time)) &&
            (interruptTimeBias == ReadULong64Acquire(&USER_SHARED_DATA->InterruptTimeBias)))
            break;

        YieldProcessor();
    }

    UnbiasedInterruptTime->QuadPart = interruptTime.QuadPart - (LONG64)interruptTimeBias;
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
    KphMsgQueryUnbiasedInterruptTime(&Message->Header.UnbiasedInterruptTime);
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

    if (Message->_Dyn.Count > ARRAYSIZE(Message->_Dyn.Entries))
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
