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
#include <hwdimm.h>
#include <trace.h>

// Collector smbus: the memory modules' thermal sensors on the first SMBus host that has them, one KphHwSmbusRead
// every 3 seconds (the family's interval) carrying every module.

#define PHP_HW_SMBUS_MAX_HOSTS 8
#define PHP_HW_SMBUS_STRIKE_LIMIT 2 // consecutive timed-out or not-ready reads that end the SMBus for the session
#define PHP_HW_DIMM_COUNT 8 // SPD 50h-57h, TSOD 18h-1Fh
#define PHP_HW_DIMM_FAILURE_LIMIT 3 // consecutive reads without a value that drop a module
#define PHP_HW_SPD_NVM_BYTES 14 // SPD bytes 2, 3, 20-21, 30-39 of NVM page 0
#define PHP_HW_TSOD_ADDRESS 0x18
#define PHP_HW_SPD_ADDRESS 0x50
#define PHP_HW_SMBUS_BYTE 0
#define PHP_HW_SMBUS_WORD 1
#define PHP_HW_DIMM_MIN_CELSIUS -20.0f
#define PHP_HW_DIMM_MAX_CELSIUS 120.0f
#define PHP_HW_SMBUS_MAX_ITEMS (PHP_HW_DIMM_COUNT * PHP_HW_SPD_NVM_BYTES)

typedef enum _PHP_HW_DIMM_TYPE
{
    PhpHwDimmNone,
    PhpHwDimmTsod, // JC-42.4 thermal sensor (DDR3/DDR4)
    PhpHwDimmSpd5118 // SPD5118 hub thermal sensor (DDR5)
} PHP_HW_DIMM_TYPE;

typedef enum _PHP_HW_SMBUS_RESULT
{
    PhpHwSmbusOk,
    PhpHwSmbusAbsent, // no acknowledge: an empty slot
    PhpHwSmbusBusy, // another agent holds the host or the bus, or the item was not attempted
    PhpHwSmbusNotReady, // the host stopped decoding or moved
    PhpHwSmbusTimeout, // the transaction was killed; the device is never asked again this session
    PhpHwSmbusRefused // not requested again this session
} PHP_HW_SMBUS_RESULT;

typedef enum _PHP_HW_DIMM_PROBE
{
    PhpHwDimmProbeFound,
    PhpHwDimmProbeNone, // no thermal sensor, or the host is refused or not ready: the next host
    PhpHwDimmProbeRetry, // a transaction timed out: a strike, probed again
    PhpHwDimmProbeContended // another agent held the bus or the host: probed again
} PHP_HW_DIMM_PROBE;

typedef struct _PHP_HW_DIMM
{
    PHP_HW_DIMM_TYPE Type;
    BOOLEAN Disabled;
    UCHAR Failures;
    ULONG Index; // in the named-sensor store
} PHP_HW_DIMM, *PPHP_HW_DIMM;

// The SPD of a slot as probed, for the report.
typedef struct _PHP_HW_SPD_REPORT
{
    BOOLEAN Ddr4Read; // DramType and Ddr4Thermal hold SPD bytes 2 and 14
    BOOLEAN HubRead; // Mr11 holds the SPD5118 register
    BOOLEAN Mr18Read;
    BOOLEAN Locked; // an SPD5118 hub left on a non-zero page; nothing of it is readable
    BOOLEAN NvmRead; // Nvm holds the NVM page 0 bytes
    UCHAR DramType;
    UCHAR Ddr4Thermal;
    UCHAR Mr11;
    UCHAR Mr18;
    UCHAR Nvm[PHP_HW_SPD_NVM_BYTES];
} PHP_HW_SPD_REPORT, *PPHP_HW_SPD_REPORT;

typedef struct _PHP_HW_DIMM_PROBE_STATE
{
    BOOLEAN Busy; // a transaction timed out
    BOOLEAN Contended; // another agent held the host or the bus
    BOOLEAN NotReady; // the host stopped decoding or moved: the host is passed over
} PHP_HW_DIMM_PROBE_STATE, *PPHP_HW_DIMM_PROBE_STATE;

static ULONG PhpHwSmbusHosts[PHP_HW_SMBUS_MAX_HOSTS];
static ULONG PhpHwSmbusHostCount = 0;
static ULONG PhpHwSmbusHost = ULONG_MAX; // the host with the modules
static ULONG PhpHwSmbusProbeHost = ULONG_MAX; // the host the probe goes on with; ULONG_MAX when it is done
static BOOLEAN PhpHwSmbusProbeIdentified = FALSE; // the probe host answered identify
static BOOLEAN PhpHwSmbusProbedThisUpdate = FALSE; // the probe's answers stand until the next update
static BOOLEAN PhpHwSmbusEnded = FALSE;
static UCHAR PhpHwSmbusStrikes = 0;
static UCHAR PhpHwSmbusTimedOutSpd = 0; // slots never asked again
static UCHAR PhpHwSmbusTimedOutTsod = 0;
static KPH_HW_SMBUS_HOST PhpHwSmbusIdentity;
static PHP_HW_DIMM PhpHwDimms[PHP_HW_DIMM_COUNT];
static PHP_HW_SPD_REPORT PhpHwSpdReports[PHP_HW_DIMM_COUNT];
static ULONG PhpHwSpdReportHost = ULONG_MAX;
static KPH_HW_SMBUS_ITEM PhpHwSmbusItems[PHP_HW_SMBUS_MAX_ITEMS];
static KPH_HW_VALUE32 PhpHwSmbusValues[PHP_HW_SMBUS_MAX_ITEMS];

static PHP_HW_SMBUS_RESULT PhpHwSmbusClassify(
    _In_ NTSTATUS Status
    )
{
    if (NT_SUCCESS(Status))
        return PhpHwSmbusOk;

    switch (Status)
    {
    case STATUS_NO_SUCH_DEVICE:
        return PhpHwSmbusAbsent;
    case STATUS_RETRY:
    case STATUS_CANCELLED:
    case STATUS_INSUFFICIENT_RESOURCES:
    case STATUS_NO_MEMORY:
        return PhpHwSmbusBusy;
    case STATUS_DEVICE_NOT_READY:
        return PhpHwSmbusNotReady;
    case STATUS_IO_TIMEOUT:
        return PhpHwSmbusTimeout;
    }

    return PhpHwSmbusRefused;
}

static VOID PhpHwSmbusItem(
    _In_ ULONG Index,
    _In_ UCHAR Address,
    _In_ UCHAR Command,
    _In_ UCHAR Protocol
    )
{
    PhpHwSmbusItems[Index].Address = Address;
    PhpHwSmbusItems[Index].Command = Command;
    PhpHwSmbusItems[Index].Protocol = Protocol;
    PhpHwSmbusItems[Index].Reserved = 0;
}

/**
 * Reads the first Count items on a host; a call that fails gives its status to every item.
 *
 * \return The call's status.
 */
static NTSTATUS PhpHwSmbusSend(
    _In_ ULONG Locator,
    _In_ ULONG Count
    )
{
    NTSTATUS status;

    status = KphHwSmbusRead(Locator, 0, PhpHwSmbusItems, Count, PhpHwSmbusValues);

    for (ULONG i = 0; !NT_SUCCESS(status) && i < Count; i++)
    {
        PhpHwSmbusValues[i].Status = status;
        PhpHwSmbusValues[i].Value = 0;
    }

    return status;
}

static BOOLEAN PhpHwDimmCelsiusValid(
    _In_ FLOAT Celsius
    )
{
    return Celsius >= PHP_HW_DIMM_MIN_CELSIUS && Celsius <= PHP_HW_DIMM_MAX_CELSIUS;
}

static UCHAR PhpHwDimmAddress(
    _In_ PHP_HW_DIMM_TYPE Type,
    _In_range_(0, PHP_HW_DIMM_COUNT - 1) ULONG Slot
    )
{
    return (UCHAR)((Type == PhpHwDimmTsod ? PHP_HW_TSOD_ADDRESS : PHP_HW_SPD_ADDRESS) + Slot);
}

/**
 * Applies the status of one probe item; a timed-out address is recorded in *TimedOut.
 */
static BOOLEAN PhpHwSmbusProbeItem(
    _In_ ULONG Index,
    _In_range_(0, PHP_HW_DIMM_COUNT - 1) ULONG Slot,
    _Inout_ PUCHAR TimedOut,
    _Inout_ PPHP_HW_DIMM_PROBE_STATE State
    )
{
    switch (PhpHwSmbusClassify(PhpHwSmbusValues[Index].Status))
    {
    case PhpHwSmbusOk:
        return TRUE;
    case PhpHwSmbusBusy:
        State->Contended = TRUE;
        break;
    case PhpHwSmbusNotReady:
        State->NotReady = TRUE;
        break;
    case PhpHwSmbusTimeout:
        *TimedOut |= (UCHAR)(1 << Slot);
        State->Busy = TRUE;
        break;
    }

    return FALSE;
}

static BOOLEAN PhpHwDimmProbeStopped(
    _In_ const PHP_HW_DIMM_PROBE_STATE* State
    )
{
    return State->Busy || State->Contended || State->NotReady;
}

static PHP_HW_DIMM_PROBE PhpHwDimmProbeOutcome(
    _In_ const PHP_HW_DIMM_PROBE_STATE* State
    )
{
    if (State->NotReady)
        return PhpHwDimmProbeNone;
    if (State->Busy)
        return PhpHwDimmProbeRetry;

    return PhpHwDimmProbeContended;
}

/**
 * Finds the modules' thermal sensors on a host and records what was read of each SPD for the report.
 *
 * \param Locator The host.
 * \param Reports Receives what was read of each SPD.
 * \return The outcome.
 */
static PHP_HW_DIMM_PROBE PhpHwProbeDimmSensorsOnHost(
    _In_ ULONG Locator,
    _Inout_updates_(PHP_HW_DIMM_COUNT) PPHP_HW_SPD_REPORT Reports
    )
{
    static CONST UCHAR tsodCommands[5] = { 0x00, 0x01, 0x06, 0x07, 0x05 }; // CAP, CONFIG, MANID, DEVID, TEMP
    static CONST UCHAR nvmBytes[PHP_HW_SPD_NVM_BYTES] = { 2, 3, 20, 21, 30, 31, 32, 33, 34, 35, 36, 37, 38, 39 };
    ULONG slots[PHP_HW_DIMM_COUNT];
    BOOLEAN present[PHP_HW_DIMM_COUNT] = { 0 };
    BOOLEAN ddr5[PHP_HW_DIMM_COUNT] = { 0 };
    BOOLEAN noTsod[PHP_HW_DIMM_COUNT] = { 0 };
    USHORT identity[PHP_HW_DIMM_COUNT] = { 0 };
    PHP_HW_DIMM_PROBE_STATE state = { 0 };
    BOOLEAN spdAnswered = FALSE;
    BOOLEAN found = FALSE;
    ULONG count;

    for (ULONG i = 0; i < PHP_HW_DIMM_COUNT; i++)
        PhpHwDimms[i].Type = PhpHwDimmNone;

    // An SPD5118 hub reads 51h, 18h from MR0:MR1; a DDR3/DDR4 EEPROM answers with SPD bytes 0 and 1.
    count = 0;

    for (ULONG i = 0; i < PHP_HW_DIMM_COUNT; i++)
    {
        if (PhpHwSmbusTimedOutSpd & (1 << i))
            continue;

        PhpHwSmbusItem(count, (UCHAR)(PHP_HW_SPD_ADDRESS + i), 0x00, PHP_HW_SMBUS_WORD);
        slots[count++] = i;
    }

    if (count)
        PhpHwSmbusSend(Locator, count);

    for (ULONG i = 0; i < count; i++)
    {
        if (PhpHwSmbusProbeItem(i, slots[i], &PhpHwSmbusTimedOutSpd, &state))
        {
            present[slots[i]] = TRUE;
            identity[slots[i]] = (USHORT)PhpHwSmbusValues[i].Value;
            ddr5[slots[i]] = PhpHwSmbusValues[i].Value == 0x1851;
            spdAnswered = TRUE;
        }
        else if (PhpHwSmbusClassify(PhpHwSmbusValues[i].Status) == PhpHwSmbusAbsent)
        {
            spdAnswered = TRUE;
        }
    }

    if (PhpHwDimmProbeStopped(&state))
        return PhpHwDimmProbeOutcome(&state);

    // A zero identity is an SPD5118 hub left on a non-zero NVM page when MR3:MR4 is zero too (MR11[2:0] the page):
    // it is never moved, and neither it nor a TSOD is read. A DDR4 SPD says in byte 14 bit 7 whether it has a
    // thermal sensor; the byte is trusted only when byte 2 says DDR4, as the bus may be on page 1.
    count = 0;

    for (ULONG i = 0; i < PHP_HW_DIMM_COUNT; i++)
    {
        UCHAR address = (UCHAR)(PHP_HW_SPD_ADDRESS + i);

        if (!present[i] || ddr5[i])
            continue;

        if (!identity[i])
        {
            PhpHwSmbusItem(count * 2 + 0, address, 0x03, PHP_HW_SMBUS_WORD);
            PhpHwSmbusItem(count * 2 + 1, address, 0x0B, PHP_HW_SMBUS_BYTE);
        }
        else
        {
            PhpHwSmbusItem(count * 2 + 0, address, 0x02, PHP_HW_SMBUS_BYTE);
            PhpHwSmbusItem(count * 2 + 1, address, 0x0E, PHP_HW_SMBUS_BYTE);
        }

        slots[count++] = i;
    }

    if (count)
    {
        PhpHwSmbusSend(Locator, count * 2);

        for (ULONG i = 0; i < count; i++)
        {
            PPHP_HW_SPD_REPORT report = &Reports[slots[i]];
            BOOLEAN valid = PhpHwSmbusProbeItem(i * 2, slots[i], &PhpHwSmbusTimedOutSpd, &state);
            ULONG first = PhpHwSmbusValues[i * 2].Value;
            ULONG second = PhpHwSmbusValues[i * 2 + 1].Value;

            if (!PhpHwSmbusProbeItem(i * 2 + 1, slots[i], &PhpHwSmbusTimedOutSpd, &state) || !valid)
                continue;

            if (!identity[slots[i]])
            {
                report->HubRead = TRUE;
                report->Mr11 = (UCHAR)second;
                report->Locked = !first && (second & 0x07);
                noTsod[slots[i]] = report->Locked;
            }
            else
            {
                report->Ddr4Read = TRUE;
                report->DramType = (UCHAR)first;
                report->Ddr4Thermal = (UCHAR)second;
                noTsod[slots[i]] = (report->DramType == 0x0C || report->DramType == 0x0E) && !(report->Ddr4Thermal & 0x80);
            }
        }

        if (PhpHwDimmProbeStopped(&state))
            return PhpHwDimmProbeOutcome(&state);
    }

    // DDR5: MR5 bit 1 is the thermal sensor capability, MR26 bit 0 disables the sensor, MR49:MR50 is the temperature;
    // MR11 is the NVM page and addressing mode, MR18 bit 5 I3C mode.
    count = 0;

    for (ULONG i = 0; i < PHP_HW_DIMM_COUNT; i++)
    {
        UCHAR address = (UCHAR)(PHP_HW_SPD_ADDRESS + i);

        if (!ddr5[i])
            continue;

        PhpHwSmbusItem(count * 5 + 0, address, 0x05, PHP_HW_SMBUS_BYTE);
        PhpHwSmbusItem(count * 5 + 1, address, 0x1A, PHP_HW_SMBUS_BYTE);
        PhpHwSmbusItem(count * 5 + 2, address, 0x31, PHP_HW_SMBUS_WORD);
        PhpHwSmbusItem(count * 5 + 3, address, 0x0B, PHP_HW_SMBUS_BYTE);
        PhpHwSmbusItem(count * 5 + 4, address, 0x12, PHP_HW_SMBUS_BYTE);
        slots[count++] = i;
    }

    if (count)
    {
        ULONG nvmCount = 0;
        ULONG nvmSlots[PHP_HW_DIMM_COUNT];

        PhpHwSmbusSend(Locator, count * 5);

        for (ULONG i = 0; i < count; i++)
        {
            PKPH_HW_VALUE32 value = &PhpHwSmbusValues[i * 5];
            PPHP_HW_SPD_REPORT report = &Reports[slots[i]];
            BOOLEAN valid = TRUE;

            for (ULONG j = 0; j < 5; j++)
            {
                if (!PhpHwSmbusProbeItem(i * 5 + j, slots[i], &PhpHwSmbusTimedOutSpd, &state))
                    valid = FALSE;
            }

            if (!valid)
                continue;

            report->HubRead = TRUE;
            report->Mr18Read = TRUE;
            report->Mr11 = (UCHAR)value[3].Value;
            report->Mr18 = (UCHAR)value[4].Value;

            if (
                (value[0].Value & 0x02) &&
                !(value[1].Value & 0x01) &&
                PhpHwDimmCelsiusValid(PhHwSpd5118ToCelsius((USHORT)value[2].Value))
                )
            {
                PhpHwDimms[slots[i]].Type = PhpHwDimmSpd5118;
                found = TRUE;
            }

            // NVM page 0 is SPD bytes 0-127 at commands 80h-FFh, only with one-byte addressing (MR11[3] clear).
            if (!(report->Mr11 & 0x0F) && !(report->Mr18 & 0x20))
                nvmSlots[nvmCount++] = slots[i];
        }

        if (PhpHwDimmProbeStopped(&state))
            return PhpHwDimmProbeOutcome(&state);

        for (ULONG i = 0; i < nvmCount; i++)
        {
            for (ULONG j = 0; j < PHP_HW_SPD_NVM_BYTES; j++)
                PhpHwSmbusItem(i * PHP_HW_SPD_NVM_BYTES + j, (UCHAR)(PHP_HW_SPD_ADDRESS + nvmSlots[i]), (UCHAR)(0x80 + nvmBytes[j]), PHP_HW_SMBUS_BYTE);
        }

        if (nvmCount)
        {
            PhpHwSmbusSend(Locator, nvmCount * PHP_HW_SPD_NVM_BYTES);

            for (ULONG i = 0; i < nvmCount; i++)
            {
                PPHP_HW_SPD_REPORT report = &Reports[nvmSlots[i]];
                BOOLEAN valid = TRUE;

                for (ULONG j = 0; j < PHP_HW_SPD_NVM_BYTES; j++)
                {
                    if (!PhpHwSmbusProbeItem(i * PHP_HW_SPD_NVM_BYTES + j, nvmSlots[i], &PhpHwSmbusTimedOutSpd, &state))
                        valid = FALSE;

                    report->Nvm[j] = (UCHAR)PhpHwSmbusValues[i * PHP_HW_SPD_NVM_BYTES + j].Value;
                }

                report->NvmRead = valid;
            }

            if (PhpHwDimmProbeStopped(&state))
                return PhpHwDimmProbeOutcome(&state);
        }
    }

    // DDR3/DDR4: a JC-42.4 sensor at 18h + slot; CAP [15:8] and CONFIG [15:11] are reserved zero.
    // Without an SPD answer every slot is tried.
    count = 0;

    for (ULONG i = 0; i < PHP_HW_DIMM_COUNT; i++)
    {
        if (ddr5[i] || noTsod[i] || (spdAnswered && !present[i]) || (PhpHwSmbusTimedOutTsod & (1 << i)))
            continue;

        for (ULONG j = 0; j < RTL_NUMBER_OF(tsodCommands); j++)
            PhpHwSmbusItem(count * 5 + j, (UCHAR)(PHP_HW_TSOD_ADDRESS + i), tsodCommands[j], PHP_HW_SMBUS_WORD);

        slots[count++] = i;
    }

    if (count)
    {
        PhpHwSmbusSend(Locator, count * 5);

        for (ULONG i = 0; i < count; i++)
        {
            PKPH_HW_VALUE32 value = &PhpHwSmbusValues[i * 5];
            BOOLEAN valid = TRUE;
            USHORT manufacturer;

            for (ULONG j = 0; j < 5; j++)
            {
                if (!PhpHwSmbusProbeItem(i * 5 + j, slots[i], &PhpHwSmbusTimedOutTsod, &state))
                    valid = FALSE;
            }

            manufacturer = _byteswap_ushort((USHORT)value[2].Value);

            if (
                valid &&
                !(_byteswap_ushort((USHORT)value[0].Value) & 0xFF00) &&
                !(_byteswap_ushort((USHORT)value[1].Value) & 0xF800) &&
                manufacturer != 0 && manufacturer != 0xFFFF &&
                PhpHwDimmCelsiusValid(PhHwTsodToCelsius((USHORT)value[4].Value))
                )
            {
                PhpHwDimms[slots[i]].Type = PhpHwDimmTsod;
                found = TRUE;
            }
        }

        if (PhpHwDimmProbeStopped(&state))
            return PhpHwDimmProbeOutcome(&state);
    }

    return found ? PhpHwDimmProbeFound : PhpHwDimmProbeNone;
}

/**
 * Lists the found modules as named sensors. SMBIOS locators label them only when the populated memory device count
 * matches the sensor count.
 */
static VOID PhpHwSmbusNameDimms(
    VOID
    )
{
    PHP_HW_MEMORY_DEVICES devices;
    ULONG dimmCount = 0;
    ULONG index = 0;

    for (ULONG i = 0; i < PHP_HW_DIMM_COUNT; i++)
    {
        if (PhpHwDimms[i].Type != PhpHwDimmNone)
            dimmCount++;
    }

    // The populated memory devices are taken in the 50h + slot order (A1, A2, B1, B2).
    PhpHwQueryMemoryDevices(&devices);

    // Boards that repeat the device locator per channel ("DIMM 1") are told apart by the bank locator.
    for (ULONG i = 0; i < PHP_HW_MEMORY_DEVICE_MAX; i++)
    {
        for (ULONG j = i + 1; j < PHP_HW_MEMORY_DEVICE_MAX && devices.Locators[i]; j++)
        {
            if (!devices.Locators[j] || !PhEqualString(devices.Locators[i], devices.Locators[j], TRUE))
                continue;

            for (ULONG k = 0; k < PHP_HW_MEMORY_DEVICE_MAX; k++)
            {
                if (devices.Locators[k] && devices.Banks[k] && devices.Banks[k]->Length)
                    PhMoveReference(&devices.Locators[k], PhFormatString(L"%s %s", devices.Banks[k]->Buffer, devices.Locators[k]->Buffer));
            }

            i = PHP_HW_MEMORY_DEVICE_MAX;
            break;
        }
    }

    for (ULONG i = 0; i < PHP_HW_DIMM_COUNT; i++)
    {
        PPHP_HW_DIMM dimm = &PhpHwDimms[i];
        PPH_STRING slot;
        PPH_STRING name;

        dimm->Disabled = FALSE;
        dimm->Failures = 0;
        dimm->Index = ULONG_MAX;

        if (dimm->Type == PhpHwDimmNone)
            continue;

        if (devices.Count == dimmCount && index < PHP_HW_MEMORY_DEVICE_MAX && devices.Locators[index] && devices.Locators[index]->Length)
            slot = PhReferenceObject(devices.Locators[index]);
        else
            slot = PhFormatString(L"DIMM %lu", i);

        index++;

        name = PhFormatString(L"%s (0x%02X)", slot->Buffer, PhpHwDimmAddress(dimm->Type, i));
        dimm->Index = PhpHwAddNamedSensor(PhHwNamedSensorDimmTemperature, 0, i, name->Buffer);
        PhDereferenceObject(name);
        PhDereferenceObject(slot);
    }

    PhpHwFreeMemoryDevices(&devices);
}

/**
 * Ends the SMBus for the session: every module's sensor has no value.
 */
static VOID PhpHwSmbusEnd(
    VOID
    )
{
    for (ULONG i = 0; i < PHP_HW_DIMM_COUNT; i++)
    {
        if (PhpHwDimms[i].Type != PhpHwDimmNone)
            PhpHwSetNamedSensor(PhpHwDimms[i].Index, FALSE, 0.0f);

        PhpHwDimms[i].Disabled = TRUE;
    }

    PhpHwSmbusEnded = TRUE;
}

/**
 * Counts a timed-out or not-ready read; PHP_HW_SMBUS_STRIKE_LIMIT in a row end the SMBus for the session.
 */
static VOID PhpHwSmbusStrike(
    VOID
    )
{
    if (++PhpHwSmbusStrikes >= PHP_HW_SMBUS_STRIKE_LIMIT)
    {
        PhTraceInfo("Hardware sensors: SMBus unavailable after %lu failed reads", (ULONG)PhpHwSmbusStrikes);
        PhpHwSmbusEnd();
    }
}

/**
 * Probes the hosts in order, from where the last probe stopped, until one has modules' thermal sensors.
 *
 * \return The status of the last call, STATUS_RETRY when the probe met contention.
 */
static NTSTATUS PhpHwSmbusProbeHosts(
    VOID
    )
{
    NTSTATUS status = STATUS_NOT_SUPPORTED;

    while (PhpHwSmbusProbeHost < PhpHwSmbusHostCount)
    {
        ULONG locator = PhpHwSmbusHosts[PhpHwSmbusProbeHost];
        PHP_HW_SPD_REPORT reports[PHP_HW_DIMM_COUNT];
        PHP_HW_DIMM_PROBE probe;

        if (!PhpHwSmbusProbeIdentified)
        {
            status = KphHwSmbusIdentify(locator, &PhpHwSmbusIdentity);

            if (status == STATUS_RETRY)
                return status;

            if (!NT_SUCCESS(status))
            {
                PhpHwSmbusProbeHost++;
                continue;
            }

            PhpHwSmbusProbeIdentified = TRUE;
        }

        memset(reports, 0, sizeof(reports));
        probe = PhpHwProbeDimmSensorsOnHost(locator, reports);

        // A host without SPD answers keeps what an earlier host showed (a locked hub, say).
        for (ULONG i = 0; i < PHP_HW_DIMM_COUNT; i++)
        {
            if (reports[i].Ddr4Read || reports[i].HubRead)
            {
                memcpy(PhpHwSpdReports, reports, sizeof(PhpHwSpdReports));
                PhpHwSpdReportHost = PhpHwSmbusProbeHost;
                break;
            }
        }

        switch (probe)
        {
        case PhpHwDimmProbeRetry:
        case PhpHwDimmProbeContended:
            // Nothing is kept from a partial probe; it is repeated in full. Contention is not a strike.
            for (ULONG i = 0; i < PHP_HW_DIMM_COUNT; i++)
                PhpHwDimms[i].Type = PhpHwDimmNone;

            if (probe == PhpHwDimmProbeRetry)
                PhpHwSmbusStrike();

            return probe == PhpHwDimmProbeRetry ? STATUS_IO_TIMEOUT : STATUS_RETRY;
        case PhpHwDimmProbeFound:
            PhpHwSmbusHost = PhpHwSmbusProbeHost;
            PhpHwSmbusProbeHost = ULONG_MAX;
            PhpHwSmbusStrikes = 0;
            PhpHwSmbusNameDimms();
            return STATUS_SUCCESS;
        }

        // A clean answer without sensors, or a host that is not ready: the next host starts afresh.
        PhpHwSmbusProbeHost++;
        PhpHwSmbusProbeIdentified = FALSE;
        PhpHwSmbusStrikes = 0;
        PhpHwSmbusTimedOutSpd = 0;
        PhpHwSmbusTimedOutTsod = 0;
        status = STATUS_SUCCESS;
    }

    PhpHwSmbusProbeHost = ULONG_MAX;

    return status;
}

/**
 * Lists the SMBus hosts and probes them for the modules' thermal sensors. Called once a session.
 *
 * \return STATUS_SUCCESS with modules or a probe to go on with, STATUS_NOT_SUPPORTED without, or the first host's
 * identify status when no host answered.
 */
NTSTATUS PhpHwSmbusProbe(
    VOID
    )
{
    NTSTATUS status;

    PhpHwSmbusHostCount = PhpHwEnumeratePciFunctions(
        PHP_HW_ANY_VENDOR_ID,
        PHP_HW_ANY_VENDOR_ID,
        0x0C05,
        NULL,
        0,
        FALSE,
        PhpHwSmbusHosts,
        PHP_HW_SMBUS_MAX_HOSTS
        );

    if (!PhpHwSmbusHostCount)
        return STATUS_NOT_SUPPORTED;

    PhpHwSmbusProbeHost = 0;
    PhpHwSmbusProbedThisUpdate = TRUE;
    status = PhpHwSmbusProbeHosts();

    if (PhpHwSmbusHost != ULONG_MAX || PhpHwSmbusProbeHost != ULONG_MAX)
        return STATUS_SUCCESS;

    return NT_SUCCESS(status) ? STATUS_NOT_SUPPORTED : status;
}

/**
 * Reads every module's temperature in one call: an SPD5118 hub's page (MR11) and MR49:MR50, a TSOD's TEMP.
 *
 * \return STATUS_RETRY when the call met contention, else STATUS_SUCCESS.
 */
static NTSTATUS PhpHwSmbusRead(
    VOID
    )
{
    ULONG slots[PHP_HW_DIMM_COUNT];
    ULONG firsts[PHP_HW_DIMM_COUNT];
    BOOLEAN answered = FALSE;
    BOOLEAN busy = FALSE;
    ULONG count = 0;
    ULONG items = 0;
    NTSTATUS status;

    for (ULONG i = 0; i < PHP_HW_DIMM_COUNT; i++)
    {
        PPHP_HW_DIMM dimm = &PhpHwDimms[i];
        UCHAR address;

        if (dimm->Type == PhpHwDimmNone || dimm->Disabled)
            continue;

        address = PhpHwDimmAddress(dimm->Type, i);
        firsts[count] = items;
        slots[count++] = i;

        if (dimm->Type == PhpHwDimmSpd5118)
        {
            PhpHwSmbusItem(items++, address, 0x0B, PHP_HW_SMBUS_BYTE);
            PhpHwSmbusItem(items++, address, 0x31, PHP_HW_SMBUS_WORD);
        }
        else
        {
            PhpHwSmbusItem(items++, address, 0x05, PHP_HW_SMBUS_WORD);
        }
    }

    if (!count)
    {
        PhpHwSmbusEnd();
        return STATUS_SUCCESS;
    }

    status = PhpHwSmbusSend(PhpHwSmbusHosts[PhpHwSmbusHost], items);

    for (ULONG i = 0; i < count; i++)
    {
        PPHP_HW_DIMM dimm = &PhpHwDimms[slots[i]];
        PKPH_HW_VALUE32 value = &PhpHwSmbusValues[firsts[i]];
        BOOLEAN valid = FALSE;
        FLOAT celsius = 0.0f;

        if (dimm->Type == PhpHwDimmSpd5118)
        {
            // A hub another agent left on a non-zero NVM page (MR11[2:0]) does not read its other registers: no
            // value this pass, nothing counted.
            if (PhpHwSmbusClassify(value->Status) == PhpHwSmbusOk && (value->Value & 0x07))
            {
                answered = TRUE;
                PhpHwSetNamedSensor(dimm->Index, FALSE, 0.0f);
                continue;
            }

            value++;
        }

        switch (PhpHwSmbusClassify(value->Status))
        {
        case PhpHwSmbusOk:
            answered = TRUE;
            celsius = dimm->Type == PhpHwDimmSpd5118 ? PhHwSpd5118ToCelsius((USHORT)value->Value) : PhHwTsodToCelsius((USHORT)value->Value);
            valid = PhpHwDimmCelsiusValid(celsius);
            break;
        case PhpHwSmbusAbsent:
            answered = TRUE;
            break;
        case PhpHwSmbusBusy:
            // Contention, or an item not attempted: no value this pass, nothing counted.
            PhpHwSetNamedSensor(dimm->Index, FALSE, 0.0f);
            continue;
        case PhpHwSmbusNotReady:
            busy = TRUE;
            break;
        case PhpHwSmbusTimeout:
            // A device that hangs the bus is never asked again.
            busy = TRUE;
            dimm->Disabled = TRUE;
            break;
        case PhpHwSmbusRefused:
            dimm->Disabled = TRUE;
            break;
        }

        // A sensor that stops acknowledging, is not ready or reads out of range is dropped after PHP_HW_DIMM_FAILURE_LIMIT reads.
        if (valid)
            dimm->Failures = 0;
        else if (!dimm->Disabled && ++dimm->Failures >= PHP_HW_DIMM_FAILURE_LIMIT)
            dimm->Disabled = TRUE;

        if (dimm->Disabled)
            PhTraceInfo("Hardware sensors: DIMM %lu unavailable: 0x%lx", slots[i], value->Status);

        PhpHwSetNamedSensor(dimm->Index, valid, celsius);
    }

    if (busy)
        PhpHwSmbusStrike();
    else if (answered)
        PhpHwSmbusStrikes = 0;

    return status == STATUS_RETRY ? STATUS_RETRY : STATUS_SUCCESS;
}

/**
 * Goes on with a probe that met contention, from the next update; reads the modules once they are found.
 *
 * \return STATUS_RETRY when the pass met contention, else STATUS_SUCCESS.
 */
NTSTATUS PhpHwSmbusPass(
    VOID
    )
{
    NTSTATUS status = STATUS_SUCCESS;
    BOOLEAN any = FALSE;

    if (!PhpHwSmbusEnded && PhpHwSmbusHost == ULONG_MAX && PhpHwSmbusProbeHost != ULONG_MAX && !PhpHwSmbusProbedThisUpdate)
        status = PhpHwSmbusProbeHosts();

    if (!PhpHwSmbusEnded && PhpHwSmbusHost != ULONG_MAX)
        status = PhpHwSmbusRead();

    for (ULONG i = 0; !PhpHwSmbusEnded && i < PHP_HW_DIMM_COUNT; i++)
        any |= PhpHwDimms[i].Type != PhpHwDimmNone && !PhpHwDimms[i].Disabled;

    PhpHwSmbusProbedThisUpdate = FALSE;
    PhpHwFamilies[PhpHwFamilySmbus].Capabilities = any ? PH_HW_CAP_DIMM_TEMP : 0;

    return status == STATUS_RETRY ? STATUS_RETRY : STATUS_SUCCESS;
}

/**
 * Ends the session: the hosts are listed and probed again at the next probe.
 */
VOID PhpHwSmbusEndSession(
    VOID
    )
{
    PhpHwSmbusHostCount = 0;
    PhpHwSmbusHost = ULONG_MAX;
    PhpHwSmbusProbeHost = ULONG_MAX;
    PhpHwSmbusProbeIdentified = FALSE;
    PhpHwSmbusProbedThisUpdate = FALSE;
    PhpHwSmbusEnded = FALSE;
    PhpHwSmbusStrikes = 0;
    PhpHwSmbusTimedOutSpd = 0;
    PhpHwSmbusTimedOutTsod = 0;
    PhpHwSpdReportHost = ULONG_MAX;
    memset(&PhpHwSmbusIdentity, 0, sizeof(PhpHwSmbusIdentity));
    memset(PhpHwDimms, 0, sizeof(PhpHwDimms));
    memset(PhpHwSpdReports, 0, sizeof(PhpHwSpdReports));
}

/**
 * Writes an SPD5118's NVM page 0 bytes and their decode.
 */
static VOID PhpHwAppendSpd5118Nvm(
    _Inout_ PPH_STRING_BUILDER Builder,
    _In_reads_(PHP_HW_SPD_NVM_BYTES) const UCHAR* Nvm
    )
{
    static CONST PCWSTR moduleTypes[16] =
    {
        NULL, L"RDIMM", L"UDIMM", L"SODIMM", L"LRDIMM", L"CUDIMM", L"CSODIMM", L"MRDIMM",
        L"CAMM2", NULL, L"DDIMM", L"Solder down", NULL, NULL, NULL, NULL
    };
    USHORT timings[5];
    USHORT cycle = Nvm[2] | (Nvm[3] << 8);
    PCWSTR moduleType = moduleTypes[Nvm[1] & 0x0F];

    for (ULONG i = 0; i < RTL_NUMBER_OF(timings); i++)
        timings[i] = Nvm[4 + i * 2] | (Nvm[5 + i * 2] << 8);

    PhAppendFormatStringBuilder(
        Builder,
        L"\n    SPD NVM: key 0x%02x%s, module type 0x%02x%s%s, tCKAVGmin %u ps",
        Nvm[0],
        Nvm[0] == 0x12 ? L" (DDR5)" : Nvm[0] == 0x13 ? L" (LPDDR5)" : L"",
        Nvm[1],
        moduleType ? L" " : L"",
        moduleType ? moduleType : L"",
        cycle
        );

    if (cycle)
        PhAppendFormatStringBuilder(Builder, L" (%.0f MT/s)", 2000000.0 / cycle);

    PhAppendFormatStringBuilder(
        Builder,
        L", tAA %u ps, tRCD %u ps, tRP %u ps, tRAS %u ps, tRC %u ps",
        timings[0],
        timings[1],
        timings[2],
        timings[3],
        timings[4]
        );
}

/**
 * Writes the collector's section of the hardware report: the hosts, the host with the modules, and what was read of
 * each SPD.
 *
 * \param Builder The report.
 */
VOID PhpHwSmbusDiagnostics(
    _Inout_ PPH_STRING_BUILDER Builder
    )
{
    static CONST PCWSTR dimmTypes[] = { L"none", L"TSOD", L"SPD5118" };

    PhAppendFormatStringBuilder(Builder, L"  Hosts: %lu, DIMMs on ", PhpHwSmbusHostCount);

    if (PhpHwSmbusHost != ULONG_MAX)
    {
        PhAppendFormatStringBuilder(
            Builder,
            L"host %lu (0x%05lx, %04x:%04x revision 0x%02lx, kind %lu, base 0x%04lx)%s\n",
            PhpHwSmbusHost,
            PhpHwSmbusHosts[PhpHwSmbusHost],
            PhpHwSmbusIdentity.Vendor,
            PhpHwSmbusIdentity.Device,
            PhpHwSmbusIdentity.Revision,
            PhpHwSmbusIdentity.Kind,
            PhpHwSmbusIdentity.Base,
            PhpHwSmbusEnded ? L", ended" : L""
            );
    }
    else
    {
        PhAppendStringBuilder2(Builder, L"none\n");
    }

    if (PhpHwSpdReportHost != ULONG_MAX)
        PhAppendFormatStringBuilder(Builder, L"  SPD probe: host %lu\n", PhpHwSpdReportHost);

    for (ULONG i = 0; i < PHP_HW_DIMM_COUNT; i++)
    {
        PPHP_HW_SPD_REPORT report = &PhpHwSpdReports[i];
        PHP_HW_DIMM_TYPE type = PhpHwDimms[i].Type;

        if (type == PhpHwDimmNone && !report->Ddr4Read && !report->HubRead)
            continue;

        PhAppendFormatStringBuilder(Builder, L"  Slot %lu: %s", i, dimmTypes[type < RTL_NUMBER_OF(dimmTypes) ? type : 0]);

        if (report->Ddr4Read)
            PhAppendFormatStringBuilder(Builder, L", SPD byte 2 0x%02x, byte 14 0x%02x", report->DramType, report->Ddr4Thermal);

        if (report->HubRead)
        {
            PhAppendFormatStringBuilder(Builder, L", MR11 0x%02x", report->Mr11);

            if (report->Mr18Read)
                PhAppendFormatStringBuilder(Builder, L", MR18 0x%02x", report->Mr18);

            if (report->Locked)
                PhAppendFormatStringBuilder(Builder, L" (hub on page %u, locked)", report->Mr11 & 0x07);
            if (report->Mr11 & 0x08)
                PhAppendStringBuilder2(Builder, L" (2-byte addressing)");
            if (report->Mr18 & 0x20)
                PhAppendStringBuilder2(Builder, L" (I3C)");
        }

        if (report->NvmRead)
            PhpHwAppendSpd5118Nvm(Builder, report->Nvm);

        PhAppendCharStringBuilder(Builder, L'\n');
    }
}
