/*
 * Copyright (c) 2022 Winsider Seminars & Solutions, Inc.  All rights reserved.
 *
 * This file is part of System Informer.
 *
 * Authors:
 *
 *     jxy-s   2026
 *
 */

#include <ph.h>
#include <kphhw.h>

/**
 * Identifies the processor.
 *
 * \param Identity Receives the identity.
 * \return Successful or errant status.
 */
NTSTATUS KphHwCpuIdentify(
    _Out_ PKPH_HW_CPU_IDENTITY Identity
    )
{
    NTSTATUS status;
    ANSI_STRING script;
    ANSI_STRING routine;
    KPH_HW_CPU_IDENTITY identity;
    ULONG returnLength;

    RtlInitAnsiString(&script, "cpu");
    RtlInitAnsiString(&routine, "identify");

    status = KphScriptCall(
        &script,
        &routine,
        NULL,
        0,
        &identity,
        sizeof(identity),
        &returnLength
        );

    if (!NT_SUCCESS(status))
        return status;

    if (returnLength != sizeof(identity))
        return STATUS_INFO_LENGTH_MISMATCH;

    *Identity = identity;

    return status;
}

/**
 * Reads model-specific registers, each on the processor its item names.
 *
 * \param Items The registers to read.
 * \param Count The number of items.
 * \param Values Receives a status and a value per item, in the order of the
 * items.
 * \return Successful or errant status.
 */
NTSTATUS KphHwCpuRead(
    _In_reads_(Count) PCKPH_HW_CPU_ITEM Items,
    _In_ ULONG Count,
    _Out_writes_(Count) PKPH_HW_VALUE64 Values
    )
{
    NTSTATUS status;
    ANSI_STRING script;
    ANSI_STRING routine;
    ULONG inputLength;
    ULONG outputLength;
    ULONG returnLength;
    PKPH_HW_VALUE64 values;

    if (!NT_SUCCESS(status = RtlULongMult(Count, sizeof(KPH_HW_CPU_ITEM), &inputLength)))
        return status;
    if (!NT_SUCCESS(status = RtlULongMult(Count, sizeof(KPH_HW_VALUE64), &outputLength)))
        return status;

    RtlInitAnsiString(&script, "cpu");
    RtlInitAnsiString(&routine, "read");

    values = PhAllocate(outputLength ? outputLength : 1);

    status = KphScriptCall(
        &script,
        &routine,
        (PVOID)Items,
        inputLength,
        values,
        outputLength,
        &returnLength
        );

    if (NT_SUCCESS(status) && returnLength != outputLength)
        status = STATUS_INFO_LENGTH_MISMATCH;

    if (NT_SUCCESS(status))
        RtlCopyMemory(Values, values, outputLength);

    PhFree(values);

    return status;
}
