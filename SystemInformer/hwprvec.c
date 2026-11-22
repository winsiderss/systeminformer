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

#include <hwprvp.h>
#include <kphhw.h>
#include <trace.h>

// Collector crosec: the Chrome EC memory map of the Framework boards crosec.lua lists: its temperatures and fans,
// one KphHwCrosEcRead of the listed sensors' bytes every update.

#define PHP_HW_CROSEC_READ 0x20 // 00h-1Fh: temperatures, fans, second temperature bank
#define PHP_HW_CROSEC_FAN 0x10 // FAN 0-3, u16 LE
#define PHP_HW_CROSEC_TEMP_B 0x18 // TEMP_SENSOR_B 16-23
#define PHP_HW_CROSEC_TEMPS 16 // the first bank; the second has 8
#define PHP_HW_CROSEC_FANS 4
#define PHP_HW_CROSEC_SENSORS (PHP_HW_CROSEC_TEMPS + PHP_HW_CROSEC_FANS + 8) // temperatures 0-15, fans, temperatures 16-23
#define PHP_HW_CROSEC_TEMP_INVALID 0xFC // FCh not calibrated, FDh not powered, FEh error: listed, not a reading
#define PHP_HW_CROSEC_TEMP_ABSENT 0xFF
#define PHP_HW_CROSEC_ID_BASE 0x45430000 // named sensor ids of the EC, apart from the ACPI devices
#define PHP_HW_CROSEC_STRIKE_LIMIT 3 // failed reads in a row before the EC ends for the session

static KPH_HW_CROSEC_IDENTITY PhpHwCrosEcIdentity;
static NTSTATUS PhpHwCrosEcIdentifyStatus = STATUS_NOT_SUPPORTED; // STATUS_NOT_SUPPORTED when not probed
static NTSTATUS PhpHwCrosEcStatus = STATUS_NOT_SUPPORTED; // the last read, for the report
static ULONG PhpHwCrosEcNamedIndex[PHP_HW_CROSEC_SENSORS]; // ULONG_MAX when not listed
static ULONG PhpHwCrosEcOffsets[PHP_HW_CROSEC_READ]; // the bytes the listed sensors need
static ULONG PhpHwCrosEcOffsetCount = 0;
static UCHAR PhpHwCrosEcStrikes = 0;
static BOOLEAN PhpHwCrosEcEnded = FALSE;
static BOOLEAN PhpHwCrosEcListed = FALSE; // the identify and the probe read succeeded
static BOOLEAN PhpHwCrosEcProbedThisUpdate = FALSE; // a failed probe is tried again from the next pass

static BOOLEAN PhpHwCrosEcIsFan(
    _In_ ULONG Sensor
    )
{
    return Sensor >= PHP_HW_CROSEC_TEMPS && Sensor < PHP_HW_CROSEC_TEMPS + PHP_HW_CROSEC_FANS;
}

/**
 * \return The map offset of a sensor's (first) byte.
 */
static ULONG PhpHwCrosEcOffset(
    _In_ ULONG Sensor
    )
{
    if (PhpHwCrosEcIsFan(Sensor))
        return PHP_HW_CROSEC_FAN + (Sensor - PHP_HW_CROSEC_TEMPS) * 2;
    if (Sensor < PHP_HW_CROSEC_TEMPS)
        return Sensor;

    return PHP_HW_CROSEC_TEMP_B + Sensor - PHP_HW_CROSEC_TEMPS - PHP_HW_CROSEC_FANS;
}

/**
 * Reads map bytes; FALSE with the status when the call or any byte failed.
 */
static BOOLEAN PhpHwCrosEcRead(
    _In_reads_(Count) const ULONG* Offsets,
    _In_ ULONG Count,
    _Out_writes_(Count) PUCHAR Bytes,
    _Out_ PNTSTATUS Status
    )
{
    KPH_HW_VALUE32 values[PHP_HW_CROSEC_READ];
    NTSTATUS status;

    status = KphHwCrosEcRead(Offsets, Count, values);

    for (ULONG i = 0; NT_SUCCESS(status) && i < Count; i++)
    {
        status = values[i].Status;
        Bytes[i] = (UCHAR)values[i].Value;
    }

    *Status = status;
    return NT_SUCCESS(status);
}

/**
 * Identifies the EC and lists the temperatures and fans that read: the map version gates the second temperature bank.
 *
 * \return STATUS_SUCCESS, STATUS_NOT_SUPPORTED without a map to read, or a failed call's status.
 */
static NTSTATUS PhpHwCrosEcList(
    VOID
    )
{
    ULONG offsets[PHP_HW_CROSEC_READ];
    UCHAR bytes[PHP_HW_CROSEC_READ];
    ULONG count;

    for (ULONG i = 0; i < RTL_NUMBER_OF(PhpHwCrosEcNamedIndex); i++)
        PhpHwCrosEcNamedIndex[i] = ULONG_MAX;

    PhpHwCrosEcIdentifyStatus = KphHwCrosEcIdentify(&PhpHwCrosEcIdentity);

    if (!NT_SUCCESS(PhpHwCrosEcIdentifyStatus))
    {
        PhTraceInfo("Hardware sensors: Chrome EC not identified: 0x%lx", PhpHwCrosEcIdentifyStatus);

        // No row for the board, or no map at its port.
        if (PhpHwCrosEcIdentifyStatus == STATUS_NOT_FOUND)
            return STATUS_NOT_SUPPORTED;

        return PhpHwCrosEcIdentifyStatus;
    }

    if (!PhpHwCrosEcIdentity.ThermalVersion)
        return STATUS_NOT_SUPPORTED;

    count = PhpHwCrosEcIdentity.ThermalVersion < 2 ? PHP_HW_CROSEC_TEMP_B : PHP_HW_CROSEC_READ;

    for (ULONG i = 0; i < count; i++)
        offsets[i] = i;

    if (!PhpHwCrosEcRead(offsets, count, bytes, &PhpHwCrosEcStatus))
        return PhpHwCrosEcStatus;

    for (ULONG i = 0; i < PHP_HW_CROSEC_SENSORS; i++)
    {
        ULONG offset = PhpHwCrosEcOffset(i);
        PPH_STRING name;

        if (offset >= count)
            continue;

        if (PhpHwCrosEcIsFan(i))
        {
            if ((USHORT)(bytes[offset] | (bytes[offset + 1] << 8)) == 0xFFFF)
                continue;

            name = PhFormatString(L"EC fan %lu", i - PHP_HW_CROSEC_TEMPS + 1);
            PhpHwCrosEcNamedIndex[i] = PhpHwAddNamedSensor(PhHwNamedSensorFanSpeed, 0, PHP_HW_CROSEC_ID_BASE + i, name->Buffer);
            PhpHwCrosEcOffsets[PhpHwCrosEcOffsetCount++] = offset;
            PhpHwCrosEcOffsets[PhpHwCrosEcOffsetCount++] = offset + 1;
        }
        else
        {
            if (bytes[offset] == PHP_HW_CROSEC_TEMP_ABSENT)
                continue;

            name = PhFormatString(L"EC temperature %lu", i < PHP_HW_CROSEC_TEMPS ? i + 1 : i - PHP_HW_CROSEC_FANS + 1);
            PhpHwCrosEcNamedIndex[i] = PhpHwAddNamedSensor(PhHwNamedSensorAcpiTemperature, 0, PHP_HW_CROSEC_ID_BASE + i, name->Buffer);
            PhpHwCrosEcOffsets[PhpHwCrosEcOffsetCount++] = offset;
        }

        PhDereferenceObject(name);
    }

    PhpHwCrosEcListed = TRUE;

    return PhpHwCrosEcOffsetCount ? STATUS_SUCCESS : STATUS_NOT_SUPPORTED;
}

/**
 * Identifies the EC once a session and lists its sensors; a call that fails for another reason than a missing row or
 * map is made again from the next pass.
 *
 * \return STATUS_SUCCESS, STATUS_NOT_SUPPORTED without a map to read, or the failed call's status.
 */
NTSTATUS PhpHwCrosEcProbe(
    VOID
    )
{
    NTSTATUS status = PhpHwCrosEcList();

    PhpHwCrosEcProbedThisUpdate = TRUE;

    return status == STATUS_NOT_SUPPORTED || PhpHwStatusNotShipped(status) ? status : STATUS_SUCCESS;
}

/**
 * Publishes the EC temperatures (byte + 200 K; FCh-FFh are not readings) and fans (u16 LE RPM; FFFEh stalled) from one
 * read of the listed bytes. Contention is no value; three failed reads in a row end the EC.
 *
 * \return STATUS_RETRY when the read met contention, STATUS_NOT_SUPPORTED once the EC ended, else STATUS_SUCCESS.
 */
NTSTATUS PhpHwCrosEcPass(
    VOID
    )
{
    UCHAR bytes[PHP_HW_CROSEC_READ];
    UCHAR map[PHP_HW_CROSEC_READ];
    ULONG capabilities = 0;
    BOOLEAN read;

    if (PhpHwCrosEcEnded)
        return STATUS_NOT_SUPPORTED;

    if (!PhpHwCrosEcListed)
    {
        NTSTATUS status = STATUS_RETRY;

        if (!PhpHwCrosEcProbedThisUpdate)
            status = PhpHwCrosEcList();

        PhpHwCrosEcProbedThisUpdate = FALSE;

        if (status == STATUS_NOT_SUPPORTED)
            return STATUS_NOT_SUPPORTED;

        if (!PhpHwCrosEcListed)
        {
            if (status != STATUS_RETRY && ++PhpHwCrosEcStrikes >= PHP_HW_CROSEC_STRIKE_LIMIT)
                return STATUS_NOT_SUPPORTED;

            return STATUS_RETRY;
        }
    }

    PhpHwCrosEcProbedThisUpdate = FALSE;
    read = PhpHwCrosEcRead(PhpHwCrosEcOffsets, PhpHwCrosEcOffsetCount, bytes, &PhpHwCrosEcStatus);

    if (read)
    {
        PhpHwCrosEcStrikes = 0;

        for (ULONG i = 0; i < PhpHwCrosEcOffsetCount; i++)
            map[PhpHwCrosEcOffsets[i]] = bytes[i];
    }
    else if (PhpHwCrosEcStatus != STATUS_RETRY && ++PhpHwCrosEcStrikes >= PHP_HW_CROSEC_STRIKE_LIMIT)
    {
        PhTraceInfo("Hardware sensors: Chrome EC unavailable: 0x%lx", PhpHwCrosEcStatus);
        PhpHwCrosEcEnded = TRUE;
    }

    for (ULONG i = 0; i < PHP_HW_CROSEC_SENSORS; i++)
    {
        ULONG offset = PhpHwCrosEcOffset(i);
        BOOLEAN valid = FALSE;
        FLOAT value = 0.0f;

        if (PhpHwCrosEcNamedIndex[i] == ULONG_MAX)
            continue;

        if (read && PhpHwCrosEcIsFan(i))
        {
            USHORT rpm = (USHORT)(map[offset] | (map[offset + 1] << 8));

            if (rpm == 0xFFFE)
                rpm = 0;

            valid = rpm != 0xFFFF;
            value = (FLOAT)rpm;
        }
        else if (read)
        {
            valid = map[offset] < PHP_HW_CROSEC_TEMP_INVALID;
            value = (FLOAT)map[offset] + 200.0f - 273.15f;
        }

        PhpHwSetNamedSensor(PhpHwCrosEcNamedIndex[i], valid, value);

        if (!PhpHwCrosEcEnded)
            capabilities |= PhpHwCrosEcIsFan(i) ? PH_HW_CAP_FAN_SPEED : PH_HW_CAP_ACPI_TEMP;
    }

    PhpHwFamilies[PhpHwFamilyCrosEc].Capabilities = capabilities;

    if (PhpHwCrosEcEnded)
        return STATUS_NOT_SUPPORTED;

    return !read && PhpHwCrosEcStatus == STATUS_RETRY ? STATUS_RETRY : STATUS_SUCCESS;
}

/**
 * Ends the session: the EC is identified again at the next.
 */
VOID PhpHwCrosEcEnd(
    VOID
    )
{
    PhpHwCrosEcIdentifyStatus = STATUS_NOT_SUPPORTED;
    PhpHwCrosEcStatus = STATUS_NOT_SUPPORTED;
    PhpHwCrosEcOffsetCount = 0;
    PhpHwCrosEcStrikes = 0;
    PhpHwCrosEcEnded = FALSE;
    PhpHwCrosEcListed = FALSE;
    PhpHwCrosEcProbedThisUpdate = FALSE;
    memset(&PhpHwCrosEcIdentity, 0, sizeof(PhpHwCrosEcIdentity));
}

/**
 * Writes the collector's section of the hardware report: the identify and the last read.
 *
 * \param Builder The report.
 */
VOID PhpHwCrosEcDiagnostics(
    _Inout_ PPH_STRING_BUILDER Builder
    )
{
    if (PhpHwCrosEcIdentifyStatus == STATUS_NOT_SUPPORTED)
    {
        PhAppendStringBuilder2(Builder, L"  EC: not identified\n");
        return;
    }

    PhAppendFormatStringBuilder(
        Builder,
        L"  EC: identify 0x%08lx, ID version %u, thermal version %u; last read 0x%08lx; %lu bytes an update%s\n",
        PhpHwCrosEcIdentifyStatus,
        PhpHwCrosEcIdentity.IdVersion,
        PhpHwCrosEcIdentity.ThermalVersion,
        PhpHwCrosEcStatus,
        PhpHwCrosEcOffsetCount,
        PhpHwCrosEcEnded ? L", ended" : L""
        );
}
