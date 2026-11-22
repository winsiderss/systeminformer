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
// by a KphHwAcpiEval of its own every 2 seconds (the family's interval), so one slow method costs only its device.

#define PHP_HW_ACPI_MAX_DEVICES 64 // identify's first answer; more are asked for once
#define PHP_HW_ACPI_EVAL_MS 500 // the most an evaluation may take
#define PHP_HW_ACPI_CALL_MS 900 // the most a call's evaluations may take together
#define PHP_HW_ACPI_STRIKE_LIMIT 2 // consecutive timed-out evaluations that end a device for the session
#define PHP_HW_ACPI_BUSY_LIMIT 5 // consecutive busy evaluations that end a device for the session
#define PHP_HW_ACPI_IMPLAUSIBLE_LIMIT 3 // consecutive implausible readings that end a device for the session
#define PHP_HW_ACPI_STATIC_LENGTH 0x1000 // the static methods' answers: _FIF (0x40) and _FPS (0x800) with their headers
#define PHP_HW_ACPI_READ_LENGTH 0x100 // a reading's answer

typedef struct _PHP_HW_ACPI_DEVICE
{
    ULONG Row;
    ULONG Ordinal;
    CHAR Id[9];
    CHAR Path[RTL_FIELD_SIZE(KPH_HW_ACPI_DEVICE, Path)];
    PH_HW_ACPI_KIND Kind;
    BOOLEAN Disabled;
    BOOLEAN Answered; // a reading decoded, known or not: the sensors are listed
    BOOLEAN FineGrain;
    BOOLEAN StaticRead;
    UCHAR Strikes;
    UCHAR Busy;
    UCHAR Implausible;
    ULONG SensorId;
    ULONG SpeedIndex; // in the named-sensor store
    ULONG ControlIndex;
    PPH_BYTES Reading; // the evaluation record of the reading, built once
    WCHAR Name[PH_HW_NAMED_SENSOR_NAME_LENGTH];
    ULONG FifCount;
    ULONG Fif[4];
    ULONG StateCount;
    PH_HW_FAN_STATE States[PH_HW_FAN_MAX_STATES];
} PHP_HW_ACPI_DEVICE, *PPHP_HW_ACPI_DEVICE;

static ULONG PhpHwAcpiIdentified = 0; // the devices identify listed this session
static PPHP_HW_ACPI_DEVICE PhpHwAcpiDevices = NULL; // the devices read
static ULONG PhpHwAcpiDeviceCount = 0;
static UCHAR PhpHwAcpiReadBuffer[PHP_HW_ACPI_READ_LENGTH];

/**
 * Applies the session failure policy to a device: contention and an evaluation not run are no value; two timeouts or
 * five busy answers in a row end it; anything else ends it at once.
 *
 * \return TRUE when the evaluation answered.
 */
static BOOLEAN PhpHwAcpiApplyStatus(
    _Inout_ PPHP_HW_ACPI_DEVICE Device,
    _In_ NTSTATUS Status
    )
{
    BOOLEAN disable;

    if (NT_SUCCESS(Status))
    {
        Device->Strikes = 0;
        Device->Busy = 0;
        return TRUE;
    }

    switch (Status)
    {
    case STATUS_DEVICE_BUSY:
        disable = ++Device->Busy >= PHP_HW_ACPI_BUSY_LIMIT;
        break;
    case STATUS_RETRY:
    case STATUS_INSUFFICIENT_RESOURCES:
    case STATUS_NO_MEMORY:
        disable = FALSE;
        break;
    case STATUS_IO_TIMEOUT:
        Device->Busy = 0;
        disable = ++Device->Strikes >= PHP_HW_ACPI_STRIKE_LIMIT;
        break;
    default:
        disable = TRUE;
        break;
    }

    if (disable)
    {
        Device->Disabled = TRUE;
        PhTraceInfo("Hardware sensors: ACPI device %hs unavailable: 0x%lx", Device->Path, Status);
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
 * The time each of a call's evaluations may take: PHP_HW_ACPI_EVAL_MS, or PHP_HW_ACPI_CALL_MS shared by the records
 * when that is less; never 0.
 */
static ULONG PhpHwAcpiMilliseconds(
    _In_ ULONG Records
    )
{
    return max(min(PHP_HW_ACPI_EVAL_MS, PHP_HW_ACPI_CALL_MS / max(Records, 1)), 1);
}

static VOID PhpHwAcpiAppend(
    _Inout_ PPH_BYTES_BUILDER Evals,
    _In_ PPHP_HW_ACPI_DEVICE Device,
    _In_ PCSTR Name
    )
{
    PH_BYTESREF name;

    PhInitializeBytesRef(&name, (PSTR)Name);
    KphHwAcpiAppendEval(Evals, Device->Row, Device->Ordinal, &name, NULL);
}

/**
 * Reads a fan's static methods in one call: _FIF (fine-grain control and its step) and _FPS (its states, for the
 * report). Both are optional, so a record that fails is not counted; a call that fails whole is.
 */
static VOID PhpHwAcpiReadFanStatic(
    _Inout_ PPHP_HW_ACPI_DEVICE Device
    )
{
    PH_BYTES_BUILDER evals;
    PUCHAR output;
    ULONG returnLength = 0;
    NTSTATUS status;
    NTSTATUS recordStatus;
    const UCHAR* data;
    ULONG dataLength;

    PhInitializeBytesBuilder(&evals, 64);
    PhpHwAcpiAppend(&evals, Device, "_FIF");
    PhpHwAcpiAppend(&evals, Device, "_FPS");
    output = PhAllocate(PHP_HW_ACPI_STATIC_LENGTH);

    status = KphHwAcpiEval(PhpHwAcpiMilliseconds(2), evals.Bytes->Buffer, (ULONG)evals.Bytes->Length, output, PHP_HW_ACPI_STATIC_LENGTH, &returnLength);

    if (!NT_SUCCESS(status))
    {
        PhpHwAcpiApplyStatus(Device, status);
    }
    else
    {
        Device->StaticRead = TRUE;

        if (PhpHwAcpiRecord(output, returnLength, 0, &recordStatus, &data, &dataLength) && NT_SUCCESS(recordStatus))
        {
            ULONG64 fif[4];

            Device->FifCount = PhHwAcpiReadIntegers(data, dataLength, fif, RTL_NUMBER_OF(fif));

            for (ULONG i = 0; i < Device->FifCount; i++)
                Device->Fif[i] = fif[i] > ULONG_MAX ? ULONG_MAX : (ULONG)fif[i];

            Device->FineGrain = Device->FifCount >= 2 && fif[1] != 0;
        }

        if (PhpHwAcpiRecord(output, returnLength, 1, &recordStatus, &data, &dataLength) && NT_SUCCESS(recordStatus))
            Device->StateCount = PhHwDecodeFanStates(data, dataLength, Device->States, RTL_NUMBER_OF(Device->States));
    }

    PhFree(output);
    PhDeleteBytesBuilder(&evals);
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
 * Names the devices (decision D7): the device tree's name of the device at the namespace path, else the ID and the
 * ordinal; devices that share a name are told apart by their last path segment, or the whole path when that is shared
 * too. Each device's id follows its path across sessions; a colliding hash takes the next free value.
 */
static VOID PhpHwAcpiNameDevices(
    VOID
    )
{
    PCSTR* paths;
    PPH_STRING* names;

    paths = PhAllocate(sizeof(PCSTR) * PhpHwAcpiDeviceCount);
    names = PhAllocate(sizeof(PPH_STRING) * PhpHwAcpiDeviceCount);

    for (ULONG i = 0; i < PhpHwAcpiDeviceCount; i++)
        paths[i] = PhpHwAcpiDevices[i].Path;

    PhpHwQueryAcpiDeviceNames(paths, PhpHwAcpiDeviceCount, names);

    for (ULONG i = 0; i < PhpHwAcpiDeviceCount; i++)
    {
        PPHP_HW_ACPI_DEVICE device = &PhpHwAcpiDevices[i];

        if (!names[i])
            names[i] = PhFormatString(L"%hs %lu", device->Id, device->Ordinal);

        if (device->Path[0])
            device->SensorId = PhHashBytes((PUCHAR)device->Path, strlen(device->Path));
        else
            device->SensorId = PhHashBytes((PUCHAR)device->Id, strlen(device->Id)) ^ (device->Ordinal * 0x9E3779B9);

        for (ULONG j = 0; j < i; )
        {
            if (PhpHwAcpiDevices[j].SensorId == device->SensorId)
            {
                device->SensorId++;
                j = 0;
            }
            else
            {
                j++;
            }
        }
    }

    for (ULONG i = 0; i < PhpHwAcpiDeviceCount; i++)
    {
        PPHP_HW_ACPI_DEVICE device = &PhpHwAcpiDevices[i];
        PCSTR segment = PhpHwAcpiLastSegment(device->Path);
        BOOLEAN sharedName = FALSE;
        BOOLEAN sharedSegment = FALSE;
        PPH_STRING name;

        for (ULONG j = 0; j < PhpHwAcpiDeviceCount; j++)
        {
            if (j == i || !PhEqualString(names[j], names[i], TRUE))
                continue;

            sharedName = TRUE;

            if (strcmp(PhpHwAcpiLastSegment(PhpHwAcpiDevices[j].Path), segment) == 0)
                sharedSegment = TRUE;
        }

        if (sharedName && device->Path[0])
            name = PhFormatString(L"%s (%hs)", names[i]->Buffer, sharedSegment ? device->Path : segment);
        else
            name = PhReferenceObject(names[i]);

        PhCopyStringZ(name->Buffer, name->Length / sizeof(WCHAR), device->Name, RTL_NUMBER_OF(device->Name), NULL);
        PhDereferenceObject(name);
    }

    for (ULONG i = 0; i < PhpHwAcpiDeviceCount; i++)
        PhDereferenceObject(names[i]);

    PhFree(names);
    PhFree(paths);
}

/**
 * Lists the devices of the rows read (fans), names them and reads their static methods. Called once a session.
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

        if (devices[i].Row != KPH_HW_ACPI_ROW_PNP0C0B)
            continue;

        device = &PhpHwAcpiDevices[PhpHwAcpiDeviceCount++];
        device->Row = devices[i].Row;
        device->Ordinal = devices[i].Ordinal;
        device->Kind = PhHwAcpiFan;
        device->SpeedIndex = ULONG_MAX;
        device->ControlIndex = ULONG_MAX;
        memcpy(device->Id, devices[i].Id, sizeof(devices[i].Id));
        memcpy(device->Path, devices[i].Path, sizeof(device->Path));
        device->Path[sizeof(device->Path) - 1] = ANSI_NULL;
    }

    PhFree(devices);

    if (PhpHwAcpiDeviceCount)
        PhpHwAcpiNameDevices();

    for (ULONG i = 0; i < PhpHwAcpiDeviceCount; i++)
    {
        PPHP_HW_ACPI_DEVICE device = &PhpHwAcpiDevices[i];
        PH_BYTES_BUILDER evals;

        PhpHwAcpiReadFanStatic(device);

        PhInitializeBytesBuilder(&evals, 32);
        PhpHwAcpiAppend(&evals, device, "_FST");
        device->Reading = PhFinalBytesBuilderBytes(&evals);
    }

    return STATUS_SUCCESS;
}

/**
 * Lists a device's sensors on its first answer.
 */
static VOID PhpHwAcpiListSensors(
    _Inout_ PPHP_HW_ACPI_DEVICE Device
    )
{
    Device->Answered = TRUE;
    Device->SpeedIndex = PhpHwAddNamedSensor(PhHwNamedSensorFanSpeed, 0, Device->SensorId, Device->Name);

    if (Device->FineGrain)
        Device->ControlIndex = PhpHwAddNamedSensor(PhHwNamedSensorFanControl, 0, Device->SensorId, Device->Name);
}

/**
 * Reads each device: a fan's _FST, one call a device. A malformed result ends the device for the session; an unknown
 * reading is no value.
 *
 * \return STATUS_RETRY when every call met contention, else STATUS_SUCCESS.
 */
NTSTATUS PhpHwAcpiPass(
    VOID
    )
{
    BOOLEAN answered = FALSE;
    BOOLEAN retried = FALSE;
    ULONG capabilities = 0;

    for (ULONG i = 0; i < PhpHwAcpiDeviceCount; i++)
    {
        PPHP_HW_ACPI_DEVICE device = &PhpHwAcpiDevices[i];
        ULONG returnLength = 0;
        NTSTATUS status;
        NTSTATUS recordStatus;
        const UCHAR* data;
        ULONG dataLength;
        ULONG rpm = 0;
        ULONG64 control = 0;
        BOOLEAN valid = FALSE;

        if (device->Disabled)
            continue;

        status = KphHwAcpiEval(
            PhpHwAcpiMilliseconds(1),
            device->Reading->Buffer,
            (ULONG)device->Reading->Length,
            PhpHwAcpiReadBuffer,
            sizeof(PhpHwAcpiReadBuffer),
            &returnLength
            );

        if (status == STATUS_RETRY)
            retried = TRUE;
        else
            answered = TRUE;

        // A call that fails whole counts against its device; the record's status counts otherwise.
        if (!NT_SUCCESS(status))
        {
            PhpHwAcpiApplyStatus(device, status);
        }
        else
        {
            if (!PhpHwAcpiRecord(PhpHwAcpiReadBuffer, returnLength, 0, &recordStatus, &data, &dataLength))
            {
                recordStatus = STATUS_INFO_LENGTH_MISMATCH;
                data = NULL;
                dataLength = 0;
            }

            if (PhpHwAcpiApplyStatus(device, recordStatus))
            {
                switch (PhHwDecodeFanStatus(data, dataLength, &rpm, &control))
                {
                case PhHwFanDecodeOk:
                    valid = TRUE;
                    device->Implausible = 0;
                    break;
                case PhHwFanDecodeUnknown:
                    // Firmware may report an unknown speed while the fan is spun down.
                    break;
                case PhHwFanDecodeImplausible:
                    if (++device->Implausible >= PHP_HW_ACPI_IMPLAUSIBLE_LIMIT)
                    {
                        device->Disabled = TRUE;
                        PhTraceInfo("Hardware sensors: ACPI device %hs unavailable: implausible reading", device->Path);
                    }
                    break;
                case PhHwFanDecodeMalformed:
                    device->Disabled = TRUE;
                    PhTraceInfo("Hardware sensors: ACPI device %hs unavailable: malformed result of %lu bytes", device->Path, dataLength);
                    break;
                }

                if (!device->Answered && !device->Disabled)
                    PhpHwAcpiListSensors(device);
            }
        }

        PhpHwSetNamedSensor(device->SpeedIndex, valid && !device->Disabled, (FLOAT)rpm);
        PhpHwSetNamedSensor(device->ControlIndex, valid && !device->Disabled && device->FineGrain && control <= 100, (FLOAT)control);
    }

    for (ULONG i = 0; i < PhpHwAcpiDeviceCount; i++)
    {
        PPHP_HW_ACPI_DEVICE device = &PhpHwAcpiDevices[i];

        if (device->Disabled || !device->Answered)
            continue;

        capabilities |= PH_HW_CAP_FAN_SPEED;

        if (device->FineGrain)
            capabilities |= PH_HW_CAP_FAN_CONTROL;
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
        if (PhpHwAcpiDevices[i].Reading)
            PhDereferenceObject(PhpHwAcpiDevices[i].Reading);
    }

    if (PhpHwAcpiDevices)
    {
        PhFree(PhpHwAcpiDevices);
        PhpHwAcpiDevices = NULL;
    }

    PhpHwAcpiDeviceCount = 0;
    PhpHwAcpiIdentified = 0;
}

/**
 * Writes the collector's section of the hardware report: the devices identify listed and each device read with its
 * static methods.
 *
 * \param Builder The report.
 */
VOID PhpHwAcpiDiagnostics(
    _Inout_ PPH_STRING_BUILDER Builder
    )
{
    PhAppendFormatStringBuilder(Builder, L"  Devices: %lu identified\n", PhpHwAcpiIdentified);

    for (ULONG i = 0; i < PhpHwAcpiDeviceCount; i++)
    {
        PPHP_HW_ACPI_DEVICE device = &PhpHwAcpiDevices[i];

        if (!device->StaticRead)
            continue;

        PhAppendFormatStringBuilder(Builder, L"  %hs: fan", device->Path);

        if (device->FifCount >= 3)
            PhAppendFormatStringBuilder(Builder, L", _FIF fine-grain %lu step %lu%%", device->Fif[1], device->Fif[2]);

        // _FPS states: control/trip point/RPM/noise in 0.1 dBA/power in mW; '-' is not supported.
        for (ULONG j = 0; j < device->StateCount; j++)
        {
            PhAppendStringBuilder2(Builder, j ? L" " : L", _FPS ");

            for (ULONG k = 0; k < RTL_NUMBER_OF(device->States[j].Values); k++)
            {
                ULONG value = device->States[j].Values[k];

                if (k)
                    PhAppendCharStringBuilder(Builder, L'/');

                if (value == ULONG_MAX)
                    PhAppendCharStringBuilder(Builder, L'-');
                else
                    PhAppendFormatStringBuilder(Builder, L"%lu", value);
            }
        }

        PhAppendCharStringBuilder(Builder, L'\n');
    }
}
