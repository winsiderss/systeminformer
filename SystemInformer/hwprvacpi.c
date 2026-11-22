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
#include <hwacpi.h>
#include <trace.h>

// Collector acpi: the ACPI devices the acpi script covers, listed by one KphHwAcpiIdentify a session; each device read
// by a KphHwAcpiEval of its own every 2 seconds (the family's interval; a power meter at its own), with a record per
// value, so one slow method costs only its device.

#define PHP_HW_ACPI_MAX_DEVICES 64 // identify's first answer; more are asked for once
#define PHP_HW_ACPI_EVAL_MS 500 // the most an evaluation may take
#define PHP_HW_ACPI_CALL_MS 900 // the most a call's evaluations may take together
#define PHP_HW_ACPI_STRIKE_LIMIT 2 // consecutive timed-out evaluations that end a value for the session
#define PHP_HW_ACPI_BUSY_LIMIT 5 // consecutive busy evaluations that end a value for the session
#define PHP_HW_ACPI_IMPLAUSIBLE_LIMIT 3 // consecutive implausible readings that end a value for the session
#define PHP_HW_ACPI_STATIC_LENGTH 0x2000 // a static call's answers: at most ATK's three lists of 0x800
#define PHP_HW_ACPI_READ_LENGTH 0x1000 // a reading call's answers: a record of 0x20 a value
#define PHP_HW_ACPI_METER_MIN_MS 2000
#define PHP_HW_ACPI_METER_MAX_MS 60000 // _GAI above this is read at this interval
#define PHP_HW_ACPI_TYPE_POWER 0x11 // DPTF PTYP of a power participant
#define PHP_HW_ACPI_TYPE_BATTERY 0xC // DPTF PTYP of a battery
#define PHP_HW_ACPI_ATK_FIELDS 5 // id, name, limit1, limit2, enable
#define PHP_HW_ACPI_THINKPAD_SENSORS 8 // TMP0-TMP7
#define PHP_HW_ACPI_DYTC_FC_MMC 27 // DYTC function capabilities: MMC

typedef struct _PHP_HW_ACPI_STATE
{
    BOOLEAN Disabled;
    UCHAR Strikes;
    UCHAR Busy;
    UCHAR Implausible;
} PHP_HW_ACPI_STATE, *PPHP_HW_ACPI_STATE;

// A reading of a device: a fan's speed, a participant's temperature, a meter's or a getter's power.
typedef struct _PHP_HW_ACPI_VALUE
{
    PCSTR Method;
    UCHAR Op; // sets the value apart in its id; an ATK reader's PH_HW_ATK_OP_*
    BOOLEAN HasArgument;
    ULONG Argument; // an ATK reader's sensor id
    PH_HW_NAMED_SENSOR_KIND Kind;
    PHP_HW_ACPI_STATE State;
    BOOLEAN Listed; // a reading decoded, known or not: the sensors are listed
    BOOLEAN Named;
    ULONG SensorId;
    ULONG Index; // in the named-sensor store
    ULONG ControlIndex; // a fine-grain fan's control
    WCHAR Label[PH_HW_NAMED_SENSOR_NAME_LENGTH]; // the value's own name; empty for the device's name
    WCHAR Name[PH_HW_NAMED_SENSOR_NAME_LENGTH];
} PHP_HW_ACPI_VALUE, *PPHP_HW_ACPI_VALUE;

typedef struct _PHP_HW_ACPI_DEVICE
{
    ULONG Row;
    ULONG Ordinal;
    CHAR Id[9];
    CHAR Path[RTL_FIELD_SIZE(KPH_HW_ACPI_DEVICE, Path)];
    PH_HW_ACPI_KIND Kind;
    PHP_HW_ACPI_STATE State; // of the static call
    BOOLEAN StaticRead;
    BOOLEAN FineGrain;
    ULONG64 IntervalTicks; // a meter's; 0 for the family's
    ULONG64 LastRead;
    PPH_STRING TreeName; // the device tree's
    PPH_STRING FirmwareName; // _STR, or a meter's model
    ULONG ValueCount;
    ULONG ValueCapacity;
    PPHP_HW_ACPI_VALUE Values;
    UCHAR EcSlots; // the ThinkPad EC's TMPn that answered, for the report
    PPH_BYTES Reading; // the records of the values still read, built when they change
    PH_HW_ACPI_REPORT Report;
} PHP_HW_ACPI_DEVICE, *PPHP_HW_ACPI_DEVICE;

static ULONG PhpHwAcpiIdentified = 0; // the devices identify listed this session
static PPHP_HW_ACPI_DEVICE PhpHwAcpiDevices = NULL; // the devices of the rows read
static ULONG PhpHwAcpiDeviceCount = 0;
static BOOLEAN PhpHwAcpiProbedThisUpdate = FALSE; // the probe's static answers stand until the next update
static UCHAR PhpHwAcpiReadBuffer[PHP_HW_ACPI_READ_LENGTH];

/**
 * Applies the session failure policy to a device's static call or a value: contention and an evaluation not run are
 * no value; two timeouts or five busy answers in a row end it; anything else ends it at once.
 *
 * \return TRUE when the evaluation answered.
 */
static BOOLEAN PhpHwAcpiApplyStatus(
    _Inout_ PPHP_HW_ACPI_STATE State,
    _In_ PCSTR Path,
    _In_ NTSTATUS Status
    )
{
    BOOLEAN disable;

    if (NT_SUCCESS(Status))
    {
        State->Strikes = 0;
        State->Busy = 0;
        return TRUE;
    }

    switch (Status)
    {
    case STATUS_DEVICE_BUSY:
        disable = ++State->Busy >= PHP_HW_ACPI_BUSY_LIMIT;
        break;
    case STATUS_RETRY:
    case STATUS_INSUFFICIENT_RESOURCES:
    case STATUS_NO_MEMORY:
        disable = FALSE;
        break;
    case STATUS_IO_TIMEOUT:
        State->Busy = 0;
        disable = ++State->Strikes >= PHP_HW_ACPI_STRIKE_LIMIT;
        break;
    default:
        disable = TRUE;
        break;
    }

    if (disable)
    {
        State->Disabled = TRUE;
        PhTraceInfo("Hardware sensors: ACPI device %hs unavailable: 0x%lx", Path, Status);
    }

    return FALSE;
}

/**
 * Finds a record's answer in a KphHwAcpiEval output.
 *
 * \param Output The output.
 * \param Length The length of the output.
 * \param Index The record.
 * \param Status Receives the record's status.
 * \param Data Receives the record's output buffer.
 * \param DataLength Receives the length of the output buffer.
 * \return TRUE when the output holds the record.
 */
_Success_(return)
static BOOLEAN PhpHwAcpiRecord(
    _In_reads_bytes_(Length) const UCHAR* Output,
    _In_ ULONG Length,
    _In_ ULONG Index,
    _Out_ PNTSTATUS Status,
    _Out_ const UCHAR** Data,
    _Out_ PULONG DataLength
    )
{
    ULONG at = 0;

    for (ULONG i = 0; at <= Length && Length - at >= 8; i++)
    {
        LONG status;
        ULONG length;

        memcpy(&status, Output + at, sizeof(LONG));
        memcpy(&length, Output + at + 4, sizeof(ULONG));

        if (length > Length - at - 8)
            return FALSE;

        if (i == Index)
        {
            *Status = status;
            *Data = Output + at + 8;
            *DataLength = length;
            return TRUE;
        }

        at += 8 + length + (8 - length % 8) % 8;
    }

    return FALSE;
}

/**
 * Reads a record's answer as an integer.
 *
 * \return TRUE when the record answered with an integer; Status receives the record's status either way.
 */
_Success_(return)
static BOOLEAN PhpHwAcpiRecordInteger(
    _In_reads_bytes_(Length) const UCHAR* Output,
    _In_ ULONG Length,
    _In_ ULONG Index,
    _Out_ PULONG64 Value,
    _Out_ PNTSTATUS Status
    )
{
    const UCHAR* data;
    ULONG dataLength;

    if (!PhpHwAcpiRecord(Output, Length, Index, Status, &data, &dataLength))
    {
        *Status = STATUS_INFO_LENGTH_MISMATCH;
        return FALSE;
    }

    return NT_SUCCESS(*Status) && PhHwAcpiReadInteger(data, dataLength, Value);
}

/**
 * The time each of a call's evaluations may take: PHP_HW_ACPI_EVAL_MS, or PHP_HW_ACPI_CALL_MS shared by the records
 * when that is less; never 0.
 */
static ULONG PhpHwAcpiMilliseconds(
    _In_ ULONG Records
    )
{
    return max(min(PHP_HW_ACPI_EVAL_MS, PHP_HW_ACPI_CALL_MS / max(Records, 1)), 1);
}

static VOID PhpHwAcpiAppendArgument(
    _Inout_ PPH_BYTES_BUILDER Evals,
    _In_ PPHP_HW_ACPI_DEVICE Device,
    _In_ PCSTR Name,
    _In_ BOOLEAN HasArgument,
    _In_ ULONG Argument
    )
{
    PH_BYTESREF name;
    PH_BYTESREF arguments;
    UCHAR argument[PH_HW_ACPI_ARGUMENT_HEADER + sizeof(ULONG)];
    USHORT type = PH_HW_ACPI_ARGUMENT_INTEGER;
    USHORT length = sizeof(ULONG);

    // One integer argument as the ACPI driver takes it: its type, its length, the value.
    memcpy(argument, &type, sizeof(USHORT));
    memcpy(argument + sizeof(USHORT), &length, sizeof(USHORT));
    memcpy(argument + PH_HW_ACPI_ARGUMENT_HEADER, &Argument, sizeof(ULONG));
    arguments.Buffer = (PCH)argument;
    arguments.Length = sizeof(argument);

    PhInitializeBytesRef(&name, (PSTR)Name);
    KphHwAcpiAppendEval(Evals, Device->Row, Device->Ordinal, &name, HasArgument ? &arguments : NULL);
}

static VOID PhpHwAcpiAppend(
    _Inout_ PPH_BYTES_BUILDER Evals,
    _In_ PPHP_HW_ACPI_DEVICE Device,
    _In_ PCSTR Name
    )
{
    PhpHwAcpiAppendArgument(Evals, Device, Name, FALSE, 0);
}

/**
 * Builds the records of a device's values still read.
 */
static VOID PhpHwAcpiBuildReading(
    _Inout_ PPHP_HW_ACPI_DEVICE Device
    )
{
    PH_BYTES_BUILDER evals;

    PhInitializeBytesBuilder(&evals, 64);

    for (ULONG i = 0; i < Device->ValueCount; i++)
    {
        if (!Device->Values[i].State.Disabled)
            PhpHwAcpiAppendArgument(&evals, Device, Device->Values[i].Method, Device->Values[i].HasArgument, Device->Values[i].Argument);
    }

    PhMoveReference(&Device->Reading, PhFinalBytesBuilderBytes(&evals));
}

static PPHP_HW_ACPI_VALUE PhpHwAcpiAddValue(
    _Inout_ PPHP_HW_ACPI_DEVICE Device,
    _In_ PCSTR Method,
    _In_ UCHAR Op,
    _In_ PH_HW_NAMED_SENSOR_KIND Kind,
    _In_opt_ PCWSTR Label
    )
{
    PPHP_HW_ACPI_VALUE value;

    if (Device->ValueCount == Device->ValueCapacity)
    {
        Device->ValueCapacity = Device->ValueCapacity ? Device->ValueCapacity * 2 : 4;
        Device->Values = PhReAllocate(Device->Values, sizeof(PHP_HW_ACPI_VALUE) * Device->ValueCapacity);
    }

    value = &Device->Values[Device->ValueCount++];
    memset(value, 0, sizeof(PHP_HW_ACPI_VALUE));
    value->Method = Method;
    value->Op = Op;
    value->Kind = Kind;
    value->Index = ULONG_MAX;
    value->ControlIndex = ULONG_MAX;

    if (Label)
        PhCopyStringZ(Label, SIZE_MAX, value->Label, RTL_NUMBER_OF(value->Label), NULL);

    return value;
}

/**
 * Decides from a static call whose first record is _STA whether a device is present: _STA with Present and
 * Functioning, or no _STA (ACPI 6.5 6.3.7).
 *
 * \return STATUS_SUCCESS, STATUS_NOT_SUPPORTED when it is not, or the record's status to count.
 */
static NTSTATUS PhpHwAcpiPresent(
    _In_reads_bytes_(Length) const UCHAR* Output,
    _In_ ULONG Length
    )
{
    ULONG64 value;
    NTSTATUS status;

    if (PhpHwAcpiRecordInteger(Output, Length, 0, &value, &status))
        return (value & 0x9) == 0x9 ? STATUS_SUCCESS : STATUS_NOT_SUPPORTED;
    if (status == STATUS_OBJECT_NAME_NOT_FOUND)
        return STATUS_SUCCESS;
    if (NT_SUCCESS(status))
        return STATUS_NOT_SUPPORTED;

    return status;
}

/**
 * Reads a fan's static methods: _FIF (fine-grain control and its step) and _FPS (its states, for the report). Both are
 * optional, so a record that fails is not counted.
 */
static NTSTATUS PhpHwAcpiFanStatic(
    _Inout_ PPHP_HW_ACPI_DEVICE Device,
    _In_reads_bytes_(Length) const UCHAR* Output,
    _In_ ULONG Length
    )
{
    NTSTATUS status;
    const UCHAR* data;
    ULONG dataLength;

    if (PhpHwAcpiRecord(Output, Length, 0, &status, &data, &dataLength) && NT_SUCCESS(status))
    {
        ULONG64 fif[4];

        Device->Report.FifCount = PhHwAcpiReadIntegers(data, dataLength, fif, RTL_NUMBER_OF(fif));

        for (ULONG i = 0; i < Device->Report.FifCount; i++)
            Device->Report.Fif[i] = fif[i] > ULONG_MAX ? ULONG_MAX : (ULONG)fif[i];

        Device->FineGrain = Device->Report.FifCount >= 2 && fif[1] != 0;
    }

    if (PhpHwAcpiRecord(Output, Length, 1, &status, &data, &dataLength) && NT_SUCCESS(status))
        Device->Report.StateCount = PhHwDecodeFanStates(data, dataLength, Device->Report.States, RTL_NUMBER_OF(Device->Report.States));

    PhpHwAcpiAddValue(Device, "_FST", 0, PhHwNamedSensorFanSpeed, NULL);

    return STATUS_SUCCESS;
}

/**
 * Reads a DPTF participant's static methods: _STA and PTYP (only a sensor, a memory or a battery participant has a
 * participant temperature), _STR for its name, and the trip points for the report.
 */
static NTSTATUS PhpHwAcpiParticipantStatic(
    _Inout_ PPHP_HW_ACPI_DEVICE Device,
    _In_reads_bytes_(Length) const UCHAR* Output,
    _In_ ULONG Length
    )
{
    NTSTATUS status;
    ULONG64 value;
    const UCHAR* data;
    ULONG dataLength;

    if (!NT_SUCCESS(status = PhpHwAcpiPresent(Output, Length)))
        return status;

    // The IETM (0), fans (4), the PCH (5) and power participants (11h) carry no participant temperature.
    if (!PhpHwAcpiRecordInteger(Output, Length, 1, &value, &status))
        return NT_SUCCESS(status) || status == STATUS_OBJECT_NAME_NOT_FOUND ? STATUS_NOT_SUPPORTED : status;
    if (value != 0x3 && value != 0xB && value != 0xC)
        return STATUS_NOT_SUPPORTED;

    Device->Report.Type = (ULONG)value;

    if (PhpHwAcpiRecord(Output, Length, 2, &status, &data, &dataLength) && NT_SUCCESS(status))
        PhMoveReference(&Device->FirmwareName, PhHwAcpiReadString(data, dataLength));

    for (ULONG i = 0; i < PH_HW_DPTF_TRIPS; i++)
    {
        if (PhpHwAcpiRecordInteger(Output, Length, 3 + i, &value, &status) && value < ULONG_MAX)
            Device->Report.Trips[i] = (ULONG)value;
    }

    PhpHwAcpiAddValue(Device, "_TMP", 3, PhHwNamedSensorAcpiTemperature, NULL);

    return STATUS_SUCCESS;
}

/**
 * Reads a power meter's static methods: _STA, _PMC (it measures, in mW; its model names it) and _GAI (its
 * averaging interval, which paces its reading).
 */
static NTSTATUS PhpHwAcpiMeterStatic(
    _Inout_ PPHP_HW_ACPI_DEVICE Device,
    _In_reads_bytes_(Length) const UCHAR* Output,
    _In_ ULONG Length
    )
{
    NTSTATUS status;
    ULONG64 value;
    const UCHAR* data;
    ULONG dataLength;

    if (!NT_SUCCESS(status = PhpHwAcpiPresent(Output, Length)))
        return status;

    if (!PhpHwAcpiRecord(Output, Length, 1, &status, &data, &dataLength))
        return STATUS_INFO_LENGTH_MISMATCH;
    if (!NT_SUCCESS(status))
        return status == STATUS_OBJECT_NAME_NOT_FOUND ? STATUS_NOT_SUPPORTED : status;

    PhHwDecodePowerMeterCapabilities(data, dataLength, &Device->Report);

    // [0] bit 0 measurement; [1] unit 0 is mW, the only unit accepted.
    if (Device->Report.MeterCount < 2 || !(Device->Report.Meter[0] & 0x1) || Device->Report.Meter[1] != 0)
        return STATUS_NOT_SUPPORTED;

    if (Device->Report.MeterStrings[0][0])
        PhMoveReference(&Device->FirmwareName, PhZeroExtendToUtf16(Device->Report.MeterStrings[0]));
    else if (Device->Report.MeterStrings[2][0])
        PhMoveReference(&Device->FirmwareName, PhZeroExtendToUtf16(Device->Report.MeterStrings[2]));

    Device->IntervalTicks = PHP_HW_ACPI_METER_MIN_MS * (PHP_HW_SECOND / 1000);

    if (PhpHwAcpiRecordInteger(Output, Length, 2, &value, &status) && value < ULONG_MAX)
    {
        Device->Report.AveragingMs = (ULONG)value;
        Device->IntervalTicks = max(min(value, PHP_HW_ACPI_METER_MAX_MS), PHP_HW_ACPI_METER_MIN_MS) * (PHP_HW_SECOND / 1000);
    }

    PhpHwAcpiAddValue(Device, "_PMM", 2, PhHwNamedSensorAcpiPower, NULL);

    return STATUS_SUCCESS;
}

/**
 * Reads a DPTF power participant's static methods: _STA, PTYP (a power participant, 11h, or a battery, Ch) and the
 * getters of its type for the report; its readings are PMAX, PROP (not a battery's) and PBSS.
 */
static NTSTATUS PhpHwAcpiPowerStatic(
    _Inout_ PPHP_HW_ACPI_DEVICE Device,
    _In_reads_bytes_(Length) const UCHAR* Output,
    _In_ ULONG Length
    )
{
    static CONST BOOLEAN battery[PH_HW_POWER_STATICS] = { FALSE, FALSE, FALSE, TRUE, TRUE, TRUE };
    NTSTATUS status;
    ULONG64 value;

    if (!NT_SUCCESS(status = PhpHwAcpiPresent(Output, Length)))
        return status;

    if (!PhpHwAcpiRecordInteger(Output, Length, 1, &value, &status))
        return NT_SUCCESS(status) || status == STATUS_OBJECT_NAME_NOT_FOUND ? STATUS_NOT_SUPPORTED : status;
    if (value != PHP_HW_ACPI_TYPE_POWER && value != PHP_HW_ACPI_TYPE_BATTERY)
        return STATUS_NOT_SUPPORTED;

    Device->Report.Type = (ULONG)value;

    for (ULONG i = 0; i < PH_HW_POWER_STATICS; i++)
    {
        if (battery[i] != (Device->Report.Type == PHP_HW_ACPI_TYPE_BATTERY))
            continue;

        if (PhpHwAcpiRecordInteger(Output, Length, 2 + i, &value, &status) && value < ULONG_MAX)
            Device->Report.Power[i] = (ULONG)value;
    }

    PhpHwAcpiAddValue(Device, "PMAX", 3, PhHwNamedSensorAcpiPower, L"Maximum power");

    if (Device->Report.Type != PHP_HW_ACPI_TYPE_BATTERY)
        PhpHwAcpiAddValue(Device, "PROP", 6, PhHwNamedSensorAcpiPower, L"Rest of platform");

    PhpHwAcpiAddValue(Device, "PBSS", 5, PhHwNamedSensorAcpiPower, L"Steady-state power");

    return STATUS_SUCCESS;
}

static PCSTR PhpHwAcpiLastSegment(
    _In_ PCSTR Path
    )
{
    PCSTR segment = Path;

    for (PCSTR p = Path; *p; p++)
    {
        if (*p == '.' || *p == '\\')
            segment = p + 1;
    }

    return segment;
}

/**
 * Expands an ATK0110 device's sensor lists into its values: each list a count, then packages of { id, name, limit1,
 * limit2, enable }; an enabled sensor of the list's class is read by its reader with its id.
 */
static NTSTATUS PhpHwAcpiAtkStatic(
    _Inout_ PPHP_HW_ACPI_DEVICE Device,
    _In_reads_bytes_(Length) const UCHAR* Output,
    _In_ ULONG Length
    )
{
    static CONST struct { PCSTR Reader; UCHAR Op; ULONG Class; PH_HW_NAMED_SENSOR_KIND Kind; } lists[] =
    {
        { "RTMP", PH_HW_ATK_OP_RTMP, 0x0603, PhHwNamedSensorAcpiTemperature },
        { "RVLT", PH_HW_ATK_OP_RVLT, 0x0602, PhHwNamedSensorAcpiVoltage },
        { "RFAN", PH_HW_ATK_OP_RFAN, 0x0604, PhHwNamedSensorFanSpeed },
    };
    NTSTATUS firstStatus = STATUS_SUCCESS;
    BOOLEAN answered = FALSE;

    for (ULONG l = 0; l < RTL_NUMBER_OF(lists); l++)
    {
        NTSTATUS status;
        const UCHAR* data;
        ULONG dataLength;
        ULONG offset;
        ULONG end;
        ULONG elements;
        USHORT type;
        USHORT length;
        ULONG size;

        if (!PhpHwAcpiRecord(Output, Length, l, &status, &data, &dataLength))
            return STATUS_INFO_LENGTH_MISMATCH;

        if (!NT_SUCCESS(status))
        {
            if (NT_SUCCESS(firstStatus))
                firstStatus = status;

            continue;
        }

        answered = TRUE;

        if (!PhHwAcpiGetElements(data, dataLength, &offset, &end, &elements) || !elements)
            continue;

        // The count, then the sensor packages.
        if (!PhHwAcpiReadArgument(data, end, offset, &type, &length, &size) || type != PH_HW_ACPI_ARGUMENT_INTEGER)
            continue;

        offset += size;

        for (ULONG e = 1; e < elements; e++)
        {
            ULONG fields[PHP_HW_ACPI_ATK_FIELDS];
            USHORT fieldTypes[PHP_HW_ACPI_ATK_FIELDS];
            USHORT fieldLengths[PHP_HW_ACPI_ATK_FIELDS];
            ULONG fieldEnd;
            ULONG field;
            ULONG64 id;
            ULONG64 enable;
            PPH_STRING name = NULL;
            PPHP_HW_ACPI_VALUE value;
            ULONG f;

            if (!PhHwAcpiReadArgument(data, end, offset, &type, &length, &size) || type != PH_HW_ACPI_ARGUMENT_PACKAGE)
                break;

            // { id, name, limit1, limit2, enable }; a short package or a zero enable is skipped.
            field = offset + PH_HW_ACPI_ARGUMENT_HEADER;
            fieldEnd = field + length;

            for (f = 0; f < PHP_HW_ACPI_ATK_FIELDS; f++)
            {
                ULONG fieldSize;

                if (!PhHwAcpiReadArgument(data, fieldEnd, field, &fieldTypes[f], &fieldLengths[f], &fieldSize))
                    break;

                fields[f] = field;
                field += fieldSize;
            }

            offset += size;

            if (
                f < PHP_HW_ACPI_ATK_FIELDS ||
                PhHwAcpiReadIntegersAt(data, fields[0], fieldEnd, &id, 1) != 1 ||
                (id >> 16) != lists[l].Class ||
                PhHwAcpiReadIntegersAt(data, fields[4], fieldEnd, &enable, 1) != 1 ||
                !enable
                )
            {
                continue;
            }

            if (fieldTypes[1] == PH_HW_ACPI_ARGUMENT_STRING && fieldLengths[1] > 1)
            {
                name = PhZeroExtendToUtf16Ex((PCSTR)(data + fields[1] + PH_HW_ACPI_ARGUMENT_HEADER), fieldLengths[1]);
                PhTrimToNullTerminatorString(name);
            }

            if (!name || !name->Length)
                PhMoveReference(&name, PhFormatString(L"ATK 0x%08I64x", id));

            value = PhpHwAcpiAddValue(Device, lists[l].Reader, lists[l].Op, lists[l].Kind, name->Buffer);
            value->HasArgument = TRUE;
            value->Argument = (ULONG)id;
            PhDereferenceObject(name);
        }
    }

    if (!answered)
        return firstStatus == STATUS_OBJECT_NAME_NOT_FOUND ? STATUS_NOT_SUPPORTED : firstStatus;

    return Device->ValueCount ? STATUS_SUCCESS : STATUS_NOT_SUPPORTED;
}

/**
 * Reads a ThinkPad HKEY's DYTC query, get, function capabilities and MMC get, for the report; the profile is a label,
 * so the device is not read after. Without bit 8 of the query DYTC is not enabled; MMC get needs its function
 * capability.
 */
static NTSTATUS PhpHwAcpiDytcStatic(
    _Inout_ PPHP_HW_ACPI_DEVICE Device,
    _In_reads_bytes_(Length) const UCHAR* Output,
    _In_ ULONG Length
    )
{
    for (ULONG i = 0; i < PH_HW_DYTC_COMMANDS; i++)
    {
        ULONG64 value;
        NTSTATUS status;

        Device->Report.Dytc[i] = ULONG_MAX;

        if (PhpHwAcpiRecordInteger(Output, Length, i, &value, &status) && value < ULONG_MAX)
            Device->Report.Dytc[i] = (ULONG)value;

        if (i == 0 && (Device->Report.Dytc[0] == ULONG_MAX || !(Device->Report.Dytc[0] & 0x100)))
            break;
        if (i == 2 && (Device->Report.Dytc[2] == ULONG_MAX || !(Device->Report.Dytc[2] & (1UL << PHP_HW_ACPI_DYTC_FC_MMC))))
            break;
    }

    return STATUS_SUCCESS;
}

/**
 * Reads a ThinkPad EC's TMP0-TMP7: each slot that answers is read after. The row answers STATUS_NOT_FOUND without a
 * ThinkPad hotkey device: then the EC has no reading.
 */
static NTSTATUS PhpHwAcpiThinkPadStatic(
    _Inout_ PPHP_HW_ACPI_DEVICE Device,
    _In_reads_bytes_(Length) const UCHAR* Output,
    _In_ ULONG Length
    )
{
    static CONST PCSTR methods[PHP_HW_ACPI_THINKPAD_SENSORS] = { "TMP0", "TMP1", "TMP2", "TMP3", "TMP4", "TMP5", "TMP6", "TMP7" };
    PCSTR segment = PhpHwAcpiLastSegment(Device->Path);
    NTSTATUS firstStatus = STATUS_SUCCESS;

    for (ULONG i = 0; i < PHP_HW_ACPI_THINKPAD_SENSORS; i++)
    {
        NTSTATUS status;
        const UCHAR* data;
        ULONG dataLength;
        PPH_STRING label;

        if (!PhpHwAcpiRecord(Output, Length, i, &status, &data, &dataLength))
            return STATUS_INFO_LENGTH_MISMATCH;

        if (!NT_SUCCESS(status))
        {
            if (NT_SUCCESS(firstStatus))
                firstStatus = status;

            continue;
        }

        label = PhFormatString(L"%hs TMP%lu", segment, i);
        PhpHwAcpiAddValue(Device, methods[i], (UCHAR)i, PhHwNamedSensorAcpiTemperature, label->Buffer);
        PhDereferenceObject(label);
        Device->EcSlots |= (UCHAR)(1 << i);
    }

    if (Device->ValueCount)
        return STATUS_SUCCESS;
    if (firstStatus == STATUS_NOT_FOUND || firstStatus == STATUS_OBJECT_NAME_NOT_FOUND)
        return STATUS_NOT_SUPPORTED;

    return firstStatus;
}

/**
 * Decodes a ThinkPad EC TMPn reading: signed degrees C; outside -127 to 127 the sensor is absent.
 */
_Success_(return == PhHwFanDecodeOk)
static PH_HW_FAN_DECODE PhpHwDecodeThinkPadTemperature(
    _In_reads_bytes_(Length) const UCHAR* Buffer,
    _In_ ULONG Length,
    _Out_ PFLOAT Celsius
    )
{
    ULONG64 value;
    LONG celsius;

    if (!PhHwAcpiReadInteger(Buffer, Length, &value))
        return PhHwFanDecodeMalformed;

    celsius = (LONG)(ULONG)value;

    // 00h is a slot the EC does not implement, 80h one without a sensor.
    if (!celsius || celsius > 127 || celsius < -127)
        return PhHwFanDecodeUnknown;

    *Celsius = (FLOAT)celsius;
    return PhHwFanDecodeOk;
}

/**
 * Reads a device's static methods in one call and decides what it reads. A device the static methods show has no
 * reading ends for the session; a call that fails whole counts against the device and is made again.
 */
static VOID PhpHwAcpiReadStatic(
    _Inout_ PPHP_HW_ACPI_DEVICE Device
    )
{
    static CONST PCSTR fan[] = { "_FIF", "_FPS" };
    static CONST PCSTR participant[] = { "_STA", "PTYP", "_STR", "_PSV", "_CRT", "_AC0", "GTSH" };
    static CONST PCSTR meter[] = { "_STA", "_PMC", "_GAI" };
    static CONST PCSTR power[] = { "_STA", "PTYP", "PSRC", "ARTG", "CTYP", "RBHF", "VBNL", "CMPP" };
    static CONST PCSTR atk[] = { "TSIF", "VSIF", "FSIF" };
    static CONST PCSTR dytc[] = { "DYTC", "DYTC", "DYTC", "DYTC" };
    static CONST ULONG dytcCommands[PH_HW_DYTC_COMMANDS] = { 0x0, 0x2, 0x3, 0x8 }; // query, get, capabilities, MMC get
    static CONST PCSTR thinkPad[] = { "TMP0", "TMP1", "TMP2", "TMP3", "TMP4", "TMP5", "TMP6", "TMP7" };
    const PCSTR* methods;
    ULONG count;
    PH_BYTES_BUILDER evals;
    PUCHAR output;
    ULONG returnLength = 0;
    NTSTATUS status;

    switch (Device->Kind)
    {
    case PhHwAcpiFan:
        methods = fan;
        count = RTL_NUMBER_OF(fan);
        break;
    case PhHwAcpiDptfTemperature:
        methods = participant;
        count = RTL_NUMBER_OF(participant);
        break;
    case PhHwAcpiPowerMeter:
        methods = meter;
        count = RTL_NUMBER_OF(meter);
        break;
    case PhHwAcpiAtk:
        methods = atk;
        count = RTL_NUMBER_OF(atk);
        break;
    case PhHwAcpiDytc:
        methods = dytc;
        count = RTL_NUMBER_OF(dytc);
        break;
    case PhHwAcpiHotkeyTemperature:
        methods = thinkPad;
        count = RTL_NUMBER_OF(thinkPad);
        break;
    default:
        methods = power;
        count = RTL_NUMBER_OF(power);
        break;
    }

    PhInitializeBytesBuilder(&evals, 128);

    for (ULONG i = 0; i < count; i++)
        PhpHwAcpiAppendArgument(&evals, Device, methods[i], Device->Kind == PhHwAcpiDytc, Device->Kind == PhHwAcpiDytc ? dytcCommands[i] : 0);

    output = PhAllocate(PHP_HW_ACPI_STATIC_LENGTH);
    status = KphHwAcpiEval(PhpHwAcpiMilliseconds(count), evals.Bytes->Buffer, (ULONG)evals.Bytes->Length, output, PHP_HW_ACPI_STATIC_LENGTH, &returnLength);

    if (NT_SUCCESS(status))
    {
        switch (Device->Kind)
        {
        case PhHwAcpiFan:
            status = PhpHwAcpiFanStatic(Device, output, returnLength);
            break;
        case PhHwAcpiDptfTemperature:
            status = PhpHwAcpiParticipantStatic(Device, output, returnLength);
            break;
        case PhHwAcpiPowerMeter:
            status = PhpHwAcpiMeterStatic(Device, output, returnLength);
            break;
        case PhHwAcpiAtk:
            status = PhpHwAcpiAtkStatic(Device, output, returnLength);
            break;
        case PhHwAcpiDytc:
            status = PhpHwAcpiDytcStatic(Device, output, returnLength);
            break;
        case PhHwAcpiHotkeyTemperature:
            status = PhpHwAcpiThinkPadStatic(Device, output, returnLength);
            break;
        default:
            status = PhpHwAcpiPowerStatic(Device, output, returnLength);
            break;
        }
    }

    if (NT_SUCCESS(status))
    {
        Device->StaticRead = TRUE;
        Device->State.Strikes = 0;
        Device->State.Busy = 0;

        // A device without a reading (DYTC) is in the report only.
        if (Device->ValueCount)
            PhpHwAcpiBuildReading(Device);
        else
            Device->State.Disabled = TRUE;
    }
    else if (status == STATUS_NOT_SUPPORTED)
    {
        Device->State.Disabled = TRUE;
        PhTraceInfo("Hardware sensors: ACPI device %hs has no reading", Device->Path);
    }
    else
    {
        PhpHwAcpiApplyStatus(&Device->State, Device->Path, status);
    }

    PhFree(output);
    PhDeleteBytesBuilder(&evals);
}

static PPH_STRING PhpHwAcpiBaseName(
    _In_ PPHP_HW_ACPI_DEVICE Device,
    _In_ PPHP_HW_ACPI_VALUE Value
    )
{
    if (Value->Label[0])
        return PhCreateString(Value->Label);
    if (Device->FirmwareName && Device->FirmwareName->Length)
        return PhReferenceObject(Device->FirmwareName);
    if (Device->TreeName)
        return PhReferenceObject(Device->TreeName);

    return PhFormatString(L"%hs %lu", Device->Id, Device->Ordinal);
}

static BOOLEAN PhpHwAcpiSensorIdUsed(
    _In_ PPHP_HW_ACPI_VALUE Value,
    _In_ ULONG SensorId
    )
{
    for (ULONG i = 0; i < PhpHwAcpiDeviceCount; i++)
    {
        for (ULONG v = 0; v < PhpHwAcpiDevices[i].ValueCount; v++)
        {
            PPHP_HW_ACPI_VALUE other = &PhpHwAcpiDevices[i].Values[v];

            if (other != Value && other->Named && other->SensorId == SensorId)
                return TRUE;
        }
    }

    return FALSE;
}

/**
 * Names the values of the devices whose static methods were read (decision D7): the firmware's name (_STR, a meter's
 * model), else the device tree's name of the device at the namespace path, else the ID and the ordinal; a getter by
 * its label. Values that share a name are told apart by their device's last path segment, or the whole path when
 * that is shared too. A value's id follows its path and method across sessions; a colliding hash takes the next free
 * value.
 */
static VOID PhpHwAcpiNameValues(
    VOID
    )
{
    for (ULONG i = 0; i < PhpHwAcpiDeviceCount; i++)
    {
        PPHP_HW_ACPI_DEVICE device = &PhpHwAcpiDevices[i];
        PCSTR segment = PhpHwAcpiLastSegment(device->Path);

        for (ULONG v = 0; v < device->ValueCount; v++)
        {
            PPHP_HW_ACPI_VALUE value = &device->Values[v];
            PPH_STRING base;
            PPH_STRING name;
            BOOLEAN sharedName = FALSE;
            BOOLEAN sharedSegment = FALSE;
            ULONG sensorId;

            if (value->Named)
                continue;

            if (device->Path[0])
                sensorId = PhHashBytes((PUCHAR)device->Path, strlen(device->Path));
            else
                sensorId = PhHashBytes((PUCHAR)device->Id, strlen(device->Id)) ^ (device->Ordinal * 0x85EBCA6B);

            sensorId ^= value->Op * 0x9E3779B9;

            while (PhpHwAcpiSensorIdUsed(value, sensorId))
                sensorId++;

            value->SensorId = sensorId;
            base = PhpHwAcpiBaseName(device, value);

            for (ULONG j = 0; j < PhpHwAcpiDeviceCount; j++)
            {
                PPHP_HW_ACPI_DEVICE other = &PhpHwAcpiDevices[j];

                for (ULONG w = 0; w < other->ValueCount; w++)
                {
                    PPH_STRING otherBase;

                    if (other == device && w == v)
                        continue;

                    otherBase = PhpHwAcpiBaseName(other, &other->Values[w]);

                    if (PhEqualString(otherBase, base, TRUE))
                    {
                        sharedName = TRUE;

                        if (other != device && strcmp(PhpHwAcpiLastSegment(other->Path), segment) == 0)
                            sharedSegment = TRUE;
                    }

                    PhDereferenceObject(otherBase);
                }
            }

            if (sharedName && device->Path[0])
                name = PhFormatString(L"%s (%hs)", base->Buffer, sharedSegment ? device->Path : segment);
            else
                name = PhReferenceObject(base);

            PhCopyStringZ(name->Buffer, name->Length / sizeof(WCHAR), value->Name, RTL_NUMBER_OF(value->Name), NULL);
            value->Named = TRUE;
            PhDereferenceObject(name);
            PhDereferenceObject(base);
        }
    }
}

/**
 * Lists the devices of the rows read, finds their device tree names and reads their static methods. Called once a
 * session.
 *
 * \return STATUS_SUCCESS, STATUS_NOT_SUPPORTED without a device, or identify's status.
 */
NTSTATUS PhpHwAcpiProbe(
    VOID
    )
{
    PKPH_HW_ACPI_DEVICE devices;
    ULONG capacity = PHP_HW_ACPI_MAX_DEVICES;
    ULONG count = 0;
    NTSTATUS status;
    PCSTR* paths;
    PPH_STRING* names;

    // Identify numbers the devices anew: it is asked once, or twice when the first answer did not fit.
    devices = PhAllocate(sizeof(KPH_HW_ACPI_DEVICE) * capacity);
    status = KphHwAcpiIdentify(devices, capacity, &count);

    if (status == STATUS_BUFFER_TOO_SMALL && count > capacity)
    {
        PhFree(devices);
        capacity = count;
        devices = PhAllocate(sizeof(KPH_HW_ACPI_DEVICE) * capacity);
        status = KphHwAcpiIdentify(devices, capacity, &count);
    }

    if (!NT_SUCCESS(status) || !count)
    {
        PhFree(devices);
        return NT_SUCCESS(status) ? STATUS_NOT_SUPPORTED : status;
    }

    count = min(count, capacity);
    PhpHwAcpiIdentified = count;
    PhpHwAcpiDevices = PhAllocateZero(sizeof(PHP_HW_ACPI_DEVICE) * count);

    for (ULONG i = 0; i < count; i++)
    {
        PPHP_HW_ACPI_DEVICE device;
        PH_HW_ACPI_KIND kind;

        switch (devices[i].Row)
        {
        case KPH_HW_ACPI_ROW_PNP0C0B:
            kind = PhHwAcpiFan;
            break;
        case KPH_HW_ACPI_ROW_INT3403:
            kind = PhHwAcpiDptfTemperature;
            break;
        case KPH_HW_ACPI_ROW_ACPI000D:
            kind = PhHwAcpiPowerMeter;
            break;
        case KPH_HW_ACPI_ROW_INT3407:
            kind = PhHwAcpiDptfPower;
            break;
        case KPH_HW_ACPI_ROW_ATK0110:
            kind = PhHwAcpiAtk;
            break;
        case KPH_HW_ACPI_ROW_LEN0068:
            kind = PhHwAcpiDytc;
            break;
        case KPH_HW_ACPI_ROW_PNP0C09:
            kind = PhHwAcpiHotkeyTemperature;
            break;
        default:
            continue;
        }

        device = &PhpHwAcpiDevices[PhpHwAcpiDeviceCount++];
        device->Row = devices[i].Row;
        device->Ordinal = devices[i].Ordinal;
        device->Kind = kind;
        memcpy(device->Id, devices[i].Id, sizeof(devices[i].Id));
        memcpy(device->Path, devices[i].Path, sizeof(device->Path));
        device->Path[sizeof(device->Path) - 1] = ANSI_NULL;
        device->Report.Type = ULONG_MAX;
        device->Report.AveragingMs = ULONG_MAX;

        for (ULONG j = 0; j < PH_HW_DPTF_TRIPS; j++)
            device->Report.Trips[j] = ULONG_MAX;
        for (ULONG j = 0; j < PH_HW_POWER_STATICS; j++)
            device->Report.Power[j] = ULONG_MAX;
        for (ULONG j = 0; j < PH_HW_DYTC_COMMANDS; j++)
            device->Report.Dytc[j] = ULONG_MAX;
    }

    PhFree(devices);

    if (!PhpHwAcpiDeviceCount)
        return STATUS_SUCCESS;

    paths = PhAllocate(sizeof(PCSTR) * PhpHwAcpiDeviceCount);
    names = PhAllocate(sizeof(PPH_STRING) * PhpHwAcpiDeviceCount);

    for (ULONG i = 0; i < PhpHwAcpiDeviceCount; i++)
        paths[i] = PhpHwAcpiDevices[i].Path;

    PhpHwQueryAcpiDeviceNames(paths, PhpHwAcpiDeviceCount, names);

    for (ULONG i = 0; i < PhpHwAcpiDeviceCount; i++)
        PhpHwAcpiDevices[i].TreeName = names[i];

    PhFree(names);
    PhFree(paths);

    for (ULONG i = 0; i < PhpHwAcpiDeviceCount; i++)
        PhpHwAcpiReadStatic(&PhpHwAcpiDevices[i]);

    PhpHwAcpiNameValues();
    PhpHwAcpiProbedThisUpdate = TRUE;

    return STATUS_SUCCESS;
}

/**
 * Lists a value's sensors on its first answer.
 */
static VOID PhpHwAcpiListSensors(
    _In_ PPHP_HW_ACPI_DEVICE Device,
    _Inout_ PPHP_HW_ACPI_VALUE Value
    )
{
    Value->Listed = TRUE;
    Value->Index = PhpHwAddNamedSensor(Value->Kind, 0, Value->SensorId, Value->Name);

    if (Device->Kind == PhHwAcpiFan && Device->FineGrain)
        Value->ControlIndex = PhpHwAddNamedSensor(PhHwNamedSensorFanControl, 0, Value->SensorId, Value->Name);
}

/**
 * Reads a device's values in one call. A malformed result ends a value for the session; an unknown reading is no
 * value.
 *
 * \return The call's status.
 */
static NTSTATUS PhpHwAcpiReadDevice(
    _Inout_ PPHP_HW_ACPI_DEVICE Device
    )
{
    ULONG records = 0;
    ULONG record = 0;
    ULONG returnLength = 0;
    BOOLEAN dropped = FALSE;
    NTSTATUS status;

    for (ULONG v = 0; v < Device->ValueCount; v++)
        records += !Device->Values[v].State.Disabled;

    status = KphHwAcpiEval(
        PhpHwAcpiMilliseconds(records),
        Device->Reading->Buffer,
        (ULONG)Device->Reading->Length,
        PhpHwAcpiReadBuffer,
        sizeof(PhpHwAcpiReadBuffer),
        &returnLength
        );

    for (ULONG v = 0; v < Device->ValueCount; v++)
    {
        PPHP_HW_ACPI_VALUE value = &Device->Values[v];
        NTSTATUS recordStatus = status;
        const UCHAR* data = NULL;
        ULONG dataLength = 0;
        FLOAT reading = 0.0f;
        ULONG64 control = 0;
        BOOLEAN valid = FALSE;

        if (value->State.Disabled)
            continue;

        // A call that fails whole counts against each of its values; a record's own status counts otherwise.
        if (NT_SUCCESS(status) && !PhpHwAcpiRecord(PhpHwAcpiReadBuffer, returnLength, record, &recordStatus, &data, &dataLength))
            recordStatus = STATUS_INFO_LENGTH_MISMATCH;

        record++;

        if (PhpHwAcpiApplyStatus(&value->State, Device->Path, recordStatus))
        {
            PH_HW_FAN_DECODE decode;

            switch (Device->Kind)
            {
            case PhHwAcpiFan:
                {
                    ULONG rpm = 0;

                    decode = PhHwDecodeFanStatus(data, dataLength, &rpm, &control);
                    reading = (FLOAT)rpm;
                }
                break;
            case PhHwAcpiDptfTemperature:
                decode = PhHwDecodeDptfTemperature(data, dataLength, &reading);
                break;
            case PhHwAcpiAtk:
                decode = PhHwDecodeAtkReading(value->Op, data, dataLength, &reading);
                break;
            case PhHwAcpiHotkeyTemperature:
                decode = PhpHwDecodeThinkPadTemperature(data, dataLength, &reading);
                break;
            default:
                decode = PhHwDecodePowerMeterReading(data, dataLength, &reading);
                break;
            }

            switch (decode)
            {
            case PhHwFanDecodeOk:
                valid = TRUE;
                value->State.Implausible = 0;
                break;
            case PhHwFanDecodeUnknown:
                // Firmware may report an unknown speed while the fan is spun down, or no reading yet.
                break;
            case PhHwFanDecodeImplausible:
                if (++value->State.Implausible >= PHP_HW_ACPI_IMPLAUSIBLE_LIMIT)
                {
                    value->State.Disabled = TRUE;
                    PhTraceInfo("Hardware sensors: ACPI device %hs %hs unavailable: implausible reading", Device->Path, value->Method);
                }
                break;
            case PhHwFanDecodeMalformed:
                value->State.Disabled = TRUE;
                PhTraceInfo("Hardware sensors: ACPI device %hs %hs unavailable: malformed result of %lu bytes", Device->Path, value->Method, dataLength);
                break;
            }

            if (!value->Listed && !value->State.Disabled)
                PhpHwAcpiListSensors(Device, value);
        }

        dropped |= value->State.Disabled;
        PhpHwSetNamedSensor(value->Index, valid && !value->State.Disabled, reading);
        PhpHwSetNamedSensor(value->ControlIndex, valid && !value->State.Disabled && Device->FineGrain && control <= 100, (FLOAT)control);
    }

    if (dropped)
    {
        BOOLEAN any = FALSE;

        for (ULONG v = 0; v < Device->ValueCount; v++)
            any |= !Device->Values[v].State.Disabled;

        if (any)
            PhpHwAcpiBuildReading(Device);
        else
            Device->State.Disabled = TRUE;
    }

    return status;
}

/**
 * Reads the static methods of a device whose static call met contention (from the update after the probe's), and
 * each device that is due: a fan's _FST, a participant's _TMP, a meter's _PMM at its interval, a power participant's
 * getters.
 *
 * \return STATUS_RETRY when every call met contention, else STATUS_SUCCESS.
 */
NTSTATUS PhpHwAcpiPass(
    VOID
    )
{
    BOOLEAN answered = FALSE;
    BOOLEAN retried = FALSE;
    BOOLEAN named = FALSE;
    ULONG capabilities = 0;
    ULONG64 now = 0;

    for (ULONG i = 0; i < PhpHwAcpiDeviceCount; i++)
    {
        PPHP_HW_ACPI_DEVICE device = &PhpHwAcpiDevices[i];
        NTSTATUS status;

        if (device->State.Disabled)
            continue;

        if (!device->StaticRead)
        {
            if (PhpHwAcpiProbedThisUpdate)
                continue;

            PhpHwAcpiReadStatic(device);
            named |= device->StaticRead;
            continue;
        }

        if (device->IntervalTicks)
        {
            if (!now)
                now = PhpHwQueryTime();

            if (device->LastRead && now - device->LastRead < device->IntervalTicks)
                continue;

            device->LastRead = now;
        }

        status = PhpHwAcpiReadDevice(device);

        if (status == STATUS_RETRY)
            retried = TRUE;
        else
            answered = TRUE;
    }

    if (named)
        PhpHwAcpiNameValues();

    PhpHwAcpiProbedThisUpdate = FALSE;

    for (ULONG i = 0; i < PhpHwAcpiDeviceCount; i++)
    {
        PPHP_HW_ACPI_DEVICE device = &PhpHwAcpiDevices[i];

        for (ULONG v = 0; !device->State.Disabled && v < device->ValueCount; v++)
        {
            PPHP_HW_ACPI_VALUE value = &device->Values[v];

            if (value->State.Disabled || !value->Listed)
                continue;

            switch (value->Kind)
            {
            case PhHwNamedSensorFanSpeed:
                capabilities |= PH_HW_CAP_FAN_SPEED;

                if (device->FineGrain)
                    capabilities |= PH_HW_CAP_FAN_CONTROL;
                break;
            case PhHwNamedSensorAcpiTemperature:
            case PhHwNamedSensorAcpiVoltage:
                capabilities |= PH_HW_CAP_ACPI_TEMP;
                break;
            default:
                capabilities |= PH_HW_CAP_ACPI_POWER;
                break;
            }
        }
    }

    PhpHwFamilies[PhpHwFamilyAcpi].Capabilities = capabilities;

    return retried && !answered ? STATUS_RETRY : STATUS_SUCCESS;
}

/**
 * Ends the session: identify is asked again at the next probe.
 */
VOID PhpHwAcpiEnd(
    VOID
    )
{
    for (ULONG i = 0; i < PhpHwAcpiDeviceCount; i++)
    {
        PhClearReference(&PhpHwAcpiDevices[i].Reading);
        PhClearReference(&PhpHwAcpiDevices[i].TreeName);
        PhClearReference(&PhpHwAcpiDevices[i].FirmwareName);

        if (PhpHwAcpiDevices[i].Values)
            PhFree(PhpHwAcpiDevices[i].Values);
    }

    if (PhpHwAcpiDevices)
    {
        PhFree(PhpHwAcpiDevices);
        PhpHwAcpiDevices = NULL;
    }

    PhpHwAcpiDeviceCount = 0;
    PhpHwAcpiIdentified = 0;
    PhpHwAcpiProbedThisUpdate = FALSE;
}

/**
 * Writes the collector's section of the hardware report: the devices identify listed and each device whose static
 * methods were read, with them.
 *
 * \param Builder The report.
 */
VOID PhpHwAcpiDiagnostics(
    _Inout_ PPH_STRING_BUILDER Builder
    )
{
    static CONST PCWSTR kindNames[] =
    {
        L"fan", L"DPTF participant", L"power meter", L"DPTF power participant", L"ATK0110", L"ATK0110 sensor", L"DYTC",
        L"ThinkPad EC temperature"
    };
    static CONST PCWSTR dytcNames[PH_HW_DYTC_COMMANDS] = { L"query", L"get", L"capabilities", L"MMC" };
    static CONST PCWSTR tripNames[PH_HW_DPTF_TRIPS] = { L"_PSV", L"_CRT", L"_AC0", L"GTSH" };
    static CONST PCWSTR getterNames[PH_HW_POWER_STATICS] = { L"PSRC", L"ARTG", L"CTYP", L"RBHF", L"VBNL", L"CMPP" };
    static CONST PCWSTR stringNames[] = { L"model", L"serial", L"OEM" };

    PhAppendFormatStringBuilder(Builder, L"  Devices: %lu identified\n", PhpHwAcpiIdentified);

    for (ULONG i = 0; i < PhpHwAcpiDeviceCount; i++)
    {
        PPHP_HW_ACPI_DEVICE device = &PhpHwAcpiDevices[i];
        PPH_HW_ACPI_REPORT report = &device->Report;

        if (!device->StaticRead)
            continue;

        PhAppendFormatStringBuilder(Builder, L"  %hs: %s", device->Path, kindNames[min((ULONG)device->Kind, RTL_NUMBER_OF(kindNames) - 1)]);

        if (device->Kind == PhHwAcpiFan)
        {
            if (report->FifCount >= 3)
                PhAppendFormatStringBuilder(Builder, L", _FIF fine-grain %lu step %lu%%", report->Fif[1], report->Fif[2]);

            // _FPS states: control/trip point/RPM/noise in 0.1 dBA/power in mW; '-' is not supported.
            for (ULONG j = 0; j < report->StateCount; j++)
            {
                PhAppendStringBuilder2(Builder, j ? L" " : L", _FPS ");

                for (ULONG k = 0; k < RTL_NUMBER_OF(report->States[j].Values); k++)
                {
                    ULONG value = report->States[j].Values[k];

                    if (k)
                        PhAppendCharStringBuilder(Builder, L'/');

                    if (value == ULONG_MAX)
                        PhAppendCharStringBuilder(Builder, L'-');
                    else
                        PhAppendFormatStringBuilder(Builder, L"%lu", value);
                }
            }
        }

        if (device->Kind == PhHwAcpiAtk)
        {
            for (ULONG v = 0; v < device->ValueCount; v++)
                PhAppendFormatStringBuilder(Builder, L", %hs 0x%08lx", device->Values[v].Method, device->Values[v].Argument);
        }

        if (device->Kind == PhHwAcpiDytc)
        {
            for (ULONG j = 0; j < PH_HW_DYTC_COMMANDS; j++)
            {
                if (report->Dytc[j] != ULONG_MAX)
                    PhAppendFormatStringBuilder(Builder, L", %s 0x%08lx", dytcNames[j], report->Dytc[j]);
            }

            // GET: [11:8] function, [15:12] mode, bit 17 lap mode.
            if (report->Dytc[1] != ULONG_MAX)
                PhAppendFormatStringBuilder(Builder, L" (function %lu, mode %lu, lap %lu)", (report->Dytc[1] >> 8) & 0xF, (report->Dytc[1] >> 12) & 0xF, (report->Dytc[1] >> 17) & 1);
        }

        if (device->Kind == PhHwAcpiHotkeyTemperature)
        {
            for (ULONG j = 0; j < PHP_HW_ACPI_THINKPAD_SENSORS; j++)
            {
                if (device->EcSlots & (1 << j))
                    PhAppendFormatStringBuilder(Builder, j && (device->EcSlots & ((1 << j) - 1)) ? L" TMP%lu" : L", TMP%lu", j);
            }
        }

        if ((device->Kind == PhHwAcpiDptfPower || device->Kind == PhHwAcpiDptfTemperature) && report->Type != ULONG_MAX)
            PhAppendFormatStringBuilder(Builder, L", PTYP 0x%lx", report->Type);

        if (device->Kind == PhHwAcpiDptfPower)
        {
            for (ULONG j = 0; j < PH_HW_POWER_STATICS; j++)
            {
                if (report->Power[j] != ULONG_MAX)
                    PhAppendFormatStringBuilder(Builder, L", %s %lu", getterNames[j], report->Power[j]);
            }
        }

        if (device->Kind == PhHwAcpiPowerMeter)
        {
            if (report->MeterCount)
            {
                PhAppendStringBuilder2(Builder, L", _PMC");

                for (ULONG j = 0; j < report->MeterCount; j++)
                    PhAppendFormatStringBuilder(Builder, L" 0x%lx", report->Meter[j]);
            }

            for (ULONG j = 0; j < RTL_NUMBER_OF(report->MeterStrings); j++)
            {
                if (report->MeterStrings[j][0])
                    PhAppendFormatStringBuilder(Builder, L", %s \"%hs\"", stringNames[j], report->MeterStrings[j]);
            }

            if (report->AveragingMs != ULONG_MAX)
                PhAppendFormatStringBuilder(Builder, L", _GAI %lu ms", report->AveragingMs);
        }

        if (device->Kind == PhHwAcpiDptfTemperature)
        {
            for (ULONG j = 0; j < PH_HW_DPTF_TRIPS; j++)
            {
                if (report->Trips[j] == ULONG_MAX)
                    continue;

                // Trip points are tenths of a kelvin; GTSH is a hysteresis, in tenths of a degree.
                if (j == PH_HW_DPTF_TRIPS - 1)
                    PhAppendFormatStringBuilder(Builder, L", %s %.1f C", tripNames[j], (FLOAT)report->Trips[j] / 10.0f);
                else
                    PhAppendFormatStringBuilder(Builder, L", %s %.1f C", tripNames[j], (FLOAT)report->Trips[j] / 10.0f - 273.15f);
            }
        }

        PhAppendCharStringBuilder(Builder, L'\n');
    }
}
