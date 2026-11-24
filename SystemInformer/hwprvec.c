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

#define PHP_HW_BOARDEC_SENSORS 32
#define PHP_HW_BOARDEC_ID_BASE 0x42454300 // named sensor ids of the board EC, apart from the Chrome EC's
#define PHP_HW_BOARDEC_STRIKE_LIMIT 3

static KPH_HW_EC_SENSOR PhpHwBoardEcSensors[PHP_HW_BOARDEC_SENSORS];
static ULONG PhpHwBoardEcCount = 0;
static ULONG PhpHwBoardEcNamedIndex[PHP_HW_BOARDEC_SENSORS]; // ULONG_MAX when not presented
static KPH_HW_EC_REGISTER PhpHwBoardEcRegisters[PHP_HW_BOARDEC_SENSORS * 2]; // each presented sensor's bytes
static ULONG PhpHwBoardEcRegisterCount = 0;
static KPH_HW_EC_IDENTITY PhpHwBoardEcIdentity;
static NTSTATUS PhpHwBoardEcListStatus = STATUS_NOT_SUPPORTED; // STATUS_NOT_SUPPORTED when not probed
static NTSTATUS PhpHwBoardEcStatus = STATUS_NOT_SUPPORTED; // the last read, for the report
static UCHAR PhpHwBoardEcStrikes = 0;
static BOOLEAN PhpHwBoardEcEnded = FALSE;
static BOOLEAN PhpHwBoardEcListed = FALSE;

/**
 * Lists the board EC's sensors the script has a row for and names those SI presents: temperatures, voltages, fans.
 *
 * \return STATUS_SUCCESS, STATUS_NOT_SUPPORTED on a board without a row or EC, or a failed call's status.
 */
static NTSTATUS PhpHwBoardEcList(
    VOID
    )
{
    NTSTATUS status;

    for (ULONG i = 0; i < RTL_NUMBER_OF(PhpHwBoardEcNamedIndex); i++)
        PhpHwBoardEcNamedIndex[i] = ULONG_MAX;

    PhpHwBoardEcRegisterCount = 0;
    status = KphHwEcSensors(PhpHwBoardEcSensors, RTL_NUMBER_OF(PhpHwBoardEcSensors), &PhpHwBoardEcCount);

    if (NT_SUCCESS(status))
        status = KphHwEcIdentify(0, &PhpHwBoardEcIdentity);

    PhpHwBoardEcListStatus = status;

    if (!NT_SUCCESS(status))
    {
        PhTraceInfo("Hardware sensors: board EC not listed: 0x%lx", status);

        if (status == STATUS_NOT_FOUND)
            return STATUS_NOT_SUPPORTED;

        return status;
    }

    for (ULONG i = 0; i < PhpHwBoardEcCount; i++)
    {
        PKPH_HW_EC_SENSOR sensor = &PhpHwBoardEcSensors[i];
        PH_HW_NAMED_SENSOR_KIND kind;
        PPH_STRING name;

        if (sensor->Kind == KphHwEcTemperature && sensor->Size == 1)
            kind = PhHwNamedSensorAcpiTemperature;
        else if (sensor->Kind == KphHwEcVoltage && sensor->Size == 2)
            kind = PhHwNamedSensorAcpiVoltage;
        else if (sensor->Kind == KphHwEcFan && sensor->Size == 2)
            kind = PhHwNamedSensorFanSpeed;
        else
            continue;

        name = PhConvertUtf8ToUtf16(sensor->Label);
        PhpHwBoardEcNamedIndex[i] = PhpHwAddNamedSensor(kind, 0, PHP_HW_BOARDEC_ID_BASE + i, name->Buffer);
        PhDereferenceObject(name);

        for (UCHAR b = 0; b < sensor->Size; b++)
        {
            PhpHwBoardEcRegisters[PhpHwBoardEcRegisterCount].Bank = sensor->Bank;
            PhpHwBoardEcRegisters[PhpHwBoardEcRegisterCount].Register = sensor->Register + b;
            PhpHwBoardEcRegisterCount++;
        }
    }

    PhpHwBoardEcListed = TRUE;

    return PhpHwBoardEcRegisterCount ? STATUS_SUCCESS : STATUS_NOT_SUPPORTED;
}

/**
 * Lists the board EC's sensors once a session.
 *
 * \return STATUS_SUCCESS, STATUS_NOT_SUPPORTED without a row, or the failed call's status.
 */
NTSTATUS PhpHwBoardEcProbe(
    VOID
    )
{
    NTSTATUS status = PhpHwBoardEcList();

    return status == STATUS_NOT_SUPPORTED || PhpHwStatusNotShipped(status) ? status : STATUS_SUCCESS;
}

/**
 * Publishes the board EC's sensors from one read of their bytes: a temperature is a signed byte of degrees C (-62,
 * -60 and -40 mark an unconnected probe), a voltage big-endian mV, a fan big-endian RPM. Contention is no value; three
 * failed reads in a row end the EC.
 *
 * \return STATUS_RETRY when the read met contention, STATUS_NOT_SUPPORTED once the EC ended, else STATUS_SUCCESS.
 */
NTSTATUS PhpHwBoardEcPass(
    VOID
    )
{
    KPH_HW_VALUE32 values[RTL_NUMBER_OF(PhpHwBoardEcRegisters)];
    ULONG capabilities = 0;
    ULONG at = 0;
    BOOLEAN read;

    if (PhpHwBoardEcEnded)
        return STATUS_NOT_SUPPORTED;

    if (!PhpHwBoardEcListed)
    {
        NTSTATUS status = PhpHwBoardEcList();

        if (status == STATUS_NOT_SUPPORTED)
            return STATUS_NOT_SUPPORTED;
        if (!PhpHwBoardEcListed)
            return ++PhpHwBoardEcStrikes >= PHP_HW_BOARDEC_STRIKE_LIMIT ? STATUS_NOT_SUPPORTED : STATUS_RETRY;
    }

    PhpHwBoardEcStatus = KphHwEcRead(0, PhpHwBoardEcRegisters, PhpHwBoardEcRegisterCount, values);
    read = NT_SUCCESS(PhpHwBoardEcStatus);

    if (read)
    {
        PhpHwBoardEcStrikes = 0;
    }
    else if (PhpHwBoardEcStatus != STATUS_RETRY && PhpHwBoardEcStatus != STATUS_DEVICE_BUSY &&
        ++PhpHwBoardEcStrikes >= PHP_HW_BOARDEC_STRIKE_LIMIT)
    {
        PhTraceInfo("Hardware sensors: board EC unavailable: 0x%lx", PhpHwBoardEcStatus);
        PhpHwBoardEcEnded = TRUE;
    }

    for (ULONG i = 0; i < PhpHwBoardEcCount; i++)
    {
        PKPH_HW_EC_SENSOR sensor = &PhpHwBoardEcSensors[i];
        BOOLEAN valid = read;
        ULONG raw = 0;
        FLOAT value = 0.0f;

        if (PhpHwBoardEcNamedIndex[i] == ULONG_MAX)
            continue;

        for (UCHAR b = 0; b < sensor->Size; b++, at++)
        {
            if (read && !NT_SUCCESS(values[at].Status))
                valid = FALSE;

            raw = (raw << 8) | (read ? (values[at].Value & 0xFF) : 0);
        }

        if (sensor->Kind == KphHwEcTemperature)
        {
            SCHAR degrees = (SCHAR)raw;

            valid = valid && degrees != -62 && degrees != -60 && degrees != -40;
            value = (FLOAT)degrees;
            capabilities |= PH_HW_CAP_ACPI_TEMP;
        }
        else if (sensor->Kind == KphHwEcVoltage)
        {
            value = (FLOAT)raw / 1000.0f;
            capabilities |= PH_HW_CAP_ACPI_TEMP;
        }
        else
        {
            value = (FLOAT)raw;
            capabilities |= PH_HW_CAP_FAN_SPEED;
        }

        PhpHwSetNamedSensor(PhpHwBoardEcNamedIndex[i], valid, value);
    }

    PhpHwFamilies[PhpHwFamilyBoardEc].Capabilities = PhpHwBoardEcEnded ? 0 : capabilities;

    if (PhpHwBoardEcEnded)
        return STATUS_NOT_SUPPORTED;

    if (!read && (PhpHwBoardEcStatus == STATUS_RETRY || PhpHwBoardEcStatus == STATUS_DEVICE_BUSY))
        return STATUS_RETRY;

    return STATUS_SUCCESS;
}

/**
 * Ends the session: the board EC is listed again at the next.
 */
VOID PhpHwBoardEcEnd(
    VOID
    )
{
    PhpHwBoardEcListStatus = STATUS_NOT_SUPPORTED;
    PhpHwBoardEcStatus = STATUS_NOT_SUPPORTED;
    PhpHwBoardEcCount = 0;
    PhpHwBoardEcRegisterCount = 0;
    PhpHwBoardEcStrikes = 0;
    PhpHwBoardEcEnded = FALSE;
    PhpHwBoardEcListed = FALSE;
    memset(&PhpHwBoardEcIdentity, 0, sizeof(PhpHwBoardEcIdentity));
}

/**
 * Writes the board EC's section of the hardware report: the list, the controller and the last read.
 *
 * \param Builder The report.
 */
VOID PhpHwBoardEcDiagnostics(
    _Inout_ PPH_STRING_BUILDER Builder
    )
{
    if (PhpHwBoardEcListStatus == STATUS_NOT_SUPPORTED)
    {
        PhAppendStringBuilder2(Builder, L"  Board EC: not listed\n");
        return;
    }

    PhAppendFormatStringBuilder(
        Builder,
        L"  Board EC: list 0x%08lx, %lu sensors, ports %02lx/%02lx%s%s; last read 0x%08lx; %lu bytes an update%s\n",
        PhpHwBoardEcListStatus,
        PhpHwBoardEcCount,
        PhpHwBoardEcIdentity.DataPort,
        PhpHwBoardEcIdentity.CommandPort,
        (PhpHwBoardEcIdentity.Flags & KPH_HW_EC_GLOBAL_LOCK) ? L", Global Lock" : L"",
        (PhpHwBoardEcIdentity.Flags & KPH_HW_EC_BANKED) ? L", banked" : L"",
        PhpHwBoardEcStatus,
        PhpHwBoardEcRegisterCount,
        PhpHwBoardEcEnded ? L", ended" : L""
        );
}

#define PHP_HW_GIGABYTE_ID_BASE 0x47420000 // named sensor ids of the Gigabyte firmware temperatures

static ULONG PhpHwGigabyteNamedIndex[KPH_HW_GIGABYTE_SENSORS]; // ULONG_MAX when not listed
static NTSTATUS PhpHwGigabyteStatus = STATUS_NOT_SUPPORTED; // the last read, for the report; not probed until set
static ULONG PhpHwGigabyteCount = 0;
static UCHAR PhpHwGigabyteStrikes = 0;
static BOOLEAN PhpHwGigabyteEnded = FALSE;

/**
 * Reads the Gigabyte firmware temperatures once a session and lists the sensors that are there (a value other than 0).
 *
 * \return STATUS_SUCCESS, STATUS_NOT_SUPPORTED on another vendor's board or without the method, or a failed call's
 * status.
 */
NTSTATUS PhpHwGigabyteProbe(
    VOID
    )
{
    KPH_HW_VALUE32 values[KPH_HW_GIGABYTE_SENSORS];
    NTSTATUS status;

    for (ULONG i = 0; i < RTL_NUMBER_OF(PhpHwGigabyteNamedIndex); i++)
        PhpHwGigabyteNamedIndex[i] = ULONG_MAX;

    PhpHwGigabyteCount = 0;
    status = KphHwGigabyteRead(values);
    PhpHwGigabyteStatus = status;

    if (status == STATUS_NOT_SUPPORTED || status == STATUS_NOT_FOUND)
        return STATUS_NOT_SUPPORTED;

    if (!NT_SUCCESS(status))
        return status;

    for (ULONG i = 0; i < KPH_HW_GIGABYTE_SENSORS; i++)
    {
        PPH_STRING name;

        if (!NT_SUCCESS(values[i].Status) || (values[i].Value & 0xFF) == 0)
            continue;

        name = PhFormatString(L"Board temperature %lu", i + 1);
        PhpHwGigabyteNamedIndex[i] = PhpHwAddNamedSensor(PhHwNamedSensorAcpiTemperature, 0, PHP_HW_GIGABYTE_ID_BASE + i, name->Buffer);
        PhDereferenceObject(name);
        PhpHwGigabyteCount++;
    }

    return PhpHwGigabyteCount ? STATUS_SUCCESS : STATUS_NOT_SUPPORTED;
}

/**
 * Publishes the listed temperatures from one read: the low byte is signed degrees C, 0 no value. Three failed reads in
 * a row end the source.
 *
 * \return STATUS_NOT_SUPPORTED once ended, else STATUS_SUCCESS.
 */
NTSTATUS PhpHwGigabytePass(
    VOID
    )
{
    KPH_HW_VALUE32 values[KPH_HW_GIGABYTE_SENSORS];
    BOOLEAN read;

    if (PhpHwGigabyteEnded || !PhpHwGigabyteCount)
        return STATUS_NOT_SUPPORTED;

    PhpHwGigabyteStatus = KphHwGigabyteRead(values);
    read = NT_SUCCESS(PhpHwGigabyteStatus);

    if (read)
        PhpHwGigabyteStrikes = 0;
    else if (++PhpHwGigabyteStrikes >= 3)
        PhpHwGigabyteEnded = TRUE;

    for (ULONG i = 0; i < KPH_HW_GIGABYTE_SENSORS; i++)
    {
        SCHAR degrees;

        if (PhpHwGigabyteNamedIndex[i] == ULONG_MAX)
            continue;

        degrees = read && NT_SUCCESS(values[i].Status) ? (SCHAR)(values[i].Value & 0xFF) : 0;
        PhpHwSetNamedSensor(PhpHwGigabyteNamedIndex[i], degrees != 0, (FLOAT)degrees);
    }

    PhpHwFamilies[PhpHwFamilyGigabyte].Capabilities = PhpHwGigabyteEnded ? 0 : PH_HW_CAP_ACPI_TEMP;

    return PhpHwGigabyteEnded ? STATUS_NOT_SUPPORTED : STATUS_SUCCESS;
}

/**
 * Ends the session: the temperatures are listed again at the next.
 */
VOID PhpHwGigabyteEnd(
    VOID
    )
{
    PhpHwGigabyteStatus = STATUS_NOT_SUPPORTED;
    PhpHwGigabyteCount = 0;
    PhpHwGigabyteStrikes = 0;
    PhpHwGigabyteEnded = FALSE;
}

/**
 * Writes the Gigabyte source's section of the hardware report.
 *
 * \param Builder The report.
 */
VOID PhpHwGigabyteDiagnostics(
    _Inout_ PPH_STRING_BUILDER Builder
    )
{
    PhAppendFormatStringBuilder(
        Builder,
        L"  Firmware temperatures: last read 0x%08lx, %lu listed%s\n",
        PhpHwGigabyteStatus,
        PhpHwGigabyteCount,
        PhpHwGigabyteEnded ? L", ended" : L""
        );
}

#define PHP_HW_ALIENWARE_ID_BASE 0x41570000 // named sensor ids: + kind << 8 + the firmware's id

static KPH_HW_ALIENWARE_IDENTITY PhpHwAlienwareIdentity;
static KPH_HW_ALIENWARE_SENSOR PhpHwAlienwareSensors[2 * KPH_HW_ALIENWARE_RESOURCES];
static ULONG PhpHwAlienwareNamedIndex[2 * KPH_HW_ALIENWARE_RESOURCES];
static NTSTATUS PhpHwAlienwareStatus = STATUS_NOT_SUPPORTED; // the last call, for the report; not probed until set
static ULONG PhpHwAlienwareCount = 0;
static UCHAR PhpHwAlienwareStrikes = 0;
static BOOLEAN PhpHwAlienwareEnded = FALSE;

/**
 * The name of an Alienware temperature sensor or fan by its firmware id.
 *
 * \param Kind KPH_HW_ALIENWARE_TEMPERATURE or KPH_HW_ALIENWARE_FAN.
 * \param Id The firmware's id.
 * \param Ordinal The sensor's place among its kind, from 1, for an id without a name.
 * \return The name.
 */
static PPH_STRING PhpHwAlienwareName(
    _In_ UCHAR Kind,
    _In_ UCHAR Id,
    _In_ ULONG Ordinal
    )
{
    static CONST PCWSTR fans[] =
    {
        L"CPU fan", L"GPU fan", L"PCI fan", L"Mid fan", L"Top fan", L"Side fan", L"U.2 fan", L"U.2 fan 2",
        L"Front fan", L"CPU fan 2", L"GPU fan 2", L"Top fan 2", L"Top fan 3", L"Front fan 2", L"Bottom fan",
        L"Bottom fan 2",
    };

    if (Kind == KPH_HW_ALIENWARE_TEMPERATURE)
    {
        switch (Id)
        {
        case 0x01:
            return PhCreateString(L"CPU");
        case 0x03:
            return PhCreateString(L"Front");
        case 0x06:
            return PhCreateString(L"GPU");
        }

        return PhFormatString(L"Temperature %lu", Ordinal);
    }

    if (Id >= 0x32 && Id < 0x32 + RTL_NUMBER_OF(fans))
        return PhCreateString(fans[Id - 0x32]);

    return PhFormatString(L"Fan %lu", Ordinal);
}

/**
 * Lists the Alienware firmware's temperature sensors and fans once a session.
 *
 * \return STATUS_SUCCESS, STATUS_NOT_SUPPORTED on a machine the script does not list or without the method, or
 * a failed call's status.
 */
NTSTATUS PhpHwAlienwareProbe(
    VOID
    )
{
    NTSTATUS status;

    PhpHwAlienwareCount = 0;
    status = KphHwAlienwareIdentify(&PhpHwAlienwareIdentity);
    PhpHwAlienwareStatus = status;

    if (status == STATUS_NOT_SUPPORTED || status == STATUS_NOT_FOUND)
        return STATUS_NOT_SUPPORTED;

    if (!NT_SUCCESS(status))
        return status;

    for (ULONG i = 0; i < PhpHwAlienwareIdentity.TemperatureCount; i++)
    {
        PhpHwAlienwareSensors[PhpHwAlienwareCount].Kind = KPH_HW_ALIENWARE_TEMPERATURE;
        PhpHwAlienwareSensors[PhpHwAlienwareCount].Id = PhpHwAlienwareIdentity.Temperatures[i];
        PhpHwAlienwareCount++;
    }

    for (ULONG i = 0; i < PhpHwAlienwareIdentity.FanCount; i++)
    {
        PhpHwAlienwareSensors[PhpHwAlienwareCount].Kind = KPH_HW_ALIENWARE_FAN;
        PhpHwAlienwareSensors[PhpHwAlienwareCount].Id = PhpHwAlienwareIdentity.Fans[i];
        PhpHwAlienwareCount++;
    }

    for (ULONG i = 0; i < PhpHwAlienwareCount; i++)
    {
        PKPH_HW_ALIENWARE_SENSOR sensor = &PhpHwAlienwareSensors[i];
        BOOLEAN temperature = sensor->Kind == KPH_HW_ALIENWARE_TEMPERATURE;
        ULONG ordinal = temperature ? i + 1 : i - PhpHwAlienwareIdentity.TemperatureCount + 1;
        PPH_STRING name = PhpHwAlienwareName(sensor->Kind, sensor->Id, ordinal);

        PhpHwAlienwareNamedIndex[i] = PhpHwAddNamedSensor(
            temperature ? PhHwNamedSensorAcpiTemperature : PhHwNamedSensorFanSpeed,
            0,
            PHP_HW_ALIENWARE_ID_BASE + ((ULONG)sensor->Kind << 8) + sensor->Id,
            name->Buffer
            );
        PhDereferenceObject(name);
    }

    return PhpHwAlienwareCount ? STATUS_SUCCESS : STATUS_NOT_SUPPORTED;
}

/**
 * Publishes the listed sensors from one read: a sensor the firmware refuses has no value. Three failed reads in a row
 * end the source.
 *
 * \return STATUS_NOT_SUPPORTED once ended, else STATUS_SUCCESS.
 */
NTSTATUS PhpHwAlienwarePass(
    VOID
    )
{
    KPH_HW_VALUE32 values[2 * KPH_HW_ALIENWARE_RESOURCES];
    ULONG capabilities = 0;
    BOOLEAN read;

    if (PhpHwAlienwareEnded || !PhpHwAlienwareCount)
        return STATUS_NOT_SUPPORTED;

    PhpHwAlienwareStatus = KphHwAlienwareRead(PhpHwAlienwareSensors, PhpHwAlienwareCount, values);
    read = NT_SUCCESS(PhpHwAlienwareStatus);

    if (read)
        PhpHwAlienwareStrikes = 0;
    else if (++PhpHwAlienwareStrikes >= 3)
        PhpHwAlienwareEnded = TRUE;

    for (ULONG i = 0; i < PhpHwAlienwareCount; i++)
    {
        BOOLEAN valid = read && NT_SUCCESS(values[i].Status);

        PhpHwSetNamedSensor(PhpHwAlienwareNamedIndex[i], valid, valid ? (FLOAT)values[i].Value : 0.0f);

        if (valid)
            capabilities |= PhpHwAlienwareSensors[i].Kind == KPH_HW_ALIENWARE_FAN ? PH_HW_CAP_FAN_SPEED : PH_HW_CAP_ACPI_TEMP;
    }

    PhpHwFamilies[PhpHwFamilyAlienware].Capabilities = PhpHwAlienwareEnded ? 0 : capabilities;

    return PhpHwAlienwareEnded ? STATUS_NOT_SUPPORTED : STATUS_SUCCESS;
}

/**
 * Ends the session: the sensors are listed again at the next.
 */
VOID PhpHwAlienwareEnd(
    VOID
    )
{
    PhpHwAlienwareStatus = STATUS_NOT_SUPPORTED;
    PhpHwAlienwareCount = 0;
    PhpHwAlienwareStrikes = 0;
    PhpHwAlienwareEnded = FALSE;
}

/**
 * Writes the Alienware source's section of the hardware report.
 *
 * \param Builder The report.
 */
VOID PhpHwAlienwareDiagnostics(
    _Inout_ PPH_STRING_BUILDER Builder
    )
{
    PhAppendFormatStringBuilder(
        Builder,
        L"  Firmware thermal: last call 0x%08lx, %lu listed%s\n",
        PhpHwAlienwareStatus,
        PhpHwAlienwareCount,
        PhpHwAlienwareEnded ? L", ended" : L""
        );
}
