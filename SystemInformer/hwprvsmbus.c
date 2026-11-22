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

// Collector smbus: the memory modules' thermal sensors on the first SMBus host that has them, and their DDR5 PMICs,
// one KphHwSmbusRead every 3 seconds (the family's interval) carrying every module.

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
#define PHP_HW_PMIC_ADDRESS 0x48
#define PHP_HW_PMIC_RAIL_VPP 2 // rails are VDD, VDDQ, VPP
#define PHP_HW_PMIC_TELEMETRY 6 // SWA, SWB, SWC, power mode, telemetry mode, temperature
#define PHP_HW_SMBUS_MAX_ITEMS (PHP_HW_DIMM_COUNT * PH_HW_PMIC_REPORT_REGISTERS)

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
    BOOLEAN Pmic; // the PMIC answered the probe; its telemetry is read with the temperature
    BOOLEAN PmicDisabled;
    UCHAR PmicFailures;
    ULONG Index; // in the named-sensor store
    ULONG VoltageIndex[PH_HW_PMIC_RAILS];
    ULONG PmicIndex[PH_HW_PMIC_SENSORS]; // ULONG_MAX until the sensor's first reading
    FLOAT Voltages[PH_HW_PMIC_RAILS]; // the set points
    WCHAR Slot[PH_HW_NAMED_SENSOR_NAME_LENGTH]; // the slot's name, for the PMIC sensors
    BOOLEAN PmicTelemetryRead; // PmicTelemetry and PmicTelemetryStatus hold the last read, for the report
    UCHAR PmicTelemetry[PHP_HW_PMIC_TELEMETRY];
    NTSTATUS PmicTelemetryStatus;
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
static UCHAR PhpHwSmbusTimedOutPmic = 0;
static UCHAR PhpHwPmicReportMask = 0; // the slots whose PMIC was read for the report
static NTSTATUS PhpHwPmicReportStatus[PHP_HW_DIMM_COUNT];
static UCHAR PhpHwPmicReportRegisters[PHP_HW_DIMM_COUNT][PH_HW_PMIC_REPORT_REGISTERS];
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

static UCHAR PhpHwPmicRegister(
    _In_reads_(PH_HW_PMIC_REPORT_REGISTERS) const UCHAR* Registers,
    _In_ UCHAR Command
    )
{
    for (ULONG i = 0; i < PH_HW_PMIC_REPORT_REGISTERS; i++)
    {
        if (PhHwPmicCommands[i] == Command)
            return Registers[i];
    }

    return 0;
}

/**
 * Reads the DDR5 PMIC registers of the slots whose SPD5118 hub answered, and their set points, in one call. A PMIC
 * that times out, is not ready or refuses has no set points or telemetry for the session.
 *
 * \return FALSE when another agent held the bus; the probe is made again.
 */
static BOOLEAN PhpHwProbePmics(
    _In_ ULONG Locator
    )
{
    static CONST FLOAT bases[PH_HW_PMIC_RAILS] = { 0.8f, 0.8f, 1.5f }; // VDD, VDDQ, VPP
    static CONST UCHAR setPoints[PH_HW_PMIC_RAILS] = { 0x21, 0x25, 0x27 }; // SWA, SWB, SWC
    ULONG slots[PHP_HW_DIMM_COUNT];
    ULONG count = 0;
    BOOLEAN contended = FALSE;

    for (ULONG slot = 0; slot < PHP_HW_DIMM_COUNT; slot++)
    {
        PhpHwDimms[slot].Pmic = FALSE;

        if (PhpHwDimms[slot].Type != PhpHwDimmSpd5118 || (PhpHwSmbusTimedOutPmic & (1 << slot)))
            continue;

        for (ULONG j = 0; j < PH_HW_PMIC_REPORT_REGISTERS; j++)
            PhpHwSmbusItem(count * PH_HW_PMIC_REPORT_REGISTERS + j, (UCHAR)(PHP_HW_PMIC_ADDRESS + slot), PhHwPmicCommands[j], PHP_HW_SMBUS_BYTE);

        slots[count++] = slot;
    }

    if (!count)
        return TRUE;

    PhpHwSmbusSend(Locator, count * PH_HW_PMIC_REPORT_REGISTERS);

    for (ULONG i = 0; i < count; i++)
    {
        PKPH_HW_VALUE32 value = &PhpHwSmbusValues[i * PH_HW_PMIC_REPORT_REGISTERS];
        PPHP_HW_DIMM dimm = &PhpHwDimms[slots[i]];
        UCHAR registers[PH_HW_PMIC_REPORT_REGISTERS];
        NTSTATUS status = STATUS_SUCCESS;
        BOOLEAN timedOut = FALSE;
        BOOLEAN busy = FALSE;
        BOOLEAN highVoltage;

        for (ULONG j = 0; j < PH_HW_PMIC_REPORT_REGISTERS; j++)
        {
            switch (PhpHwSmbusClassify(value[j].Status))
            {
            case PhpHwSmbusBusy:
                busy = TRUE;
                break;
            case PhpHwSmbusTimeout:
                timedOut = TRUE;
                break;
            }

            if (NT_SUCCESS(status) && !NT_SUCCESS(value[j].Status))
                status = value[j].Status;

            registers[j] = (UCHAR)value[j].Value;
        }

        if (timedOut)
        {
            // A PMIC that hangs the bus is never asked again this session.
            PhpHwSmbusTimedOutPmic |= (UCHAR)(1 << slots[i]);
            status = STATUS_IO_TIMEOUT;
        }
        else if (busy)
        {
            // Contention does not use up the set points.
            contended = TRUE;
            continue;
        }

        memcpy(PhpHwPmicReportRegisters[slots[i]], registers, sizeof(registers));
        PhpHwPmicReportMask |= (UCHAR)(1 << slots[i]);
        PhpHwPmicReportStatus[slots[i]] = status;

        if (!NT_SUCCESS(status))
        {
            PhTraceInfo("Hardware sensors: PMIC 0x%02X: 0x%lx", PHP_HW_PMIC_ADDRESS + slots[i], status);
            continue;
        }

        highVoltage = PhHwPmicHighVoltage(registers);

        for (ULONG rail = 0; rail < PH_HW_PMIC_RAILS; rail++)
        {
            UCHAR vid = PhpHwPmicRegister(registers, setPoints[rail]);

            // JEDEC set point: [7:1] in 5 mV steps above the rail base; VPP has no high-voltage mode.
            if (highVoltage && rail != PHP_HW_PMIC_RAIL_VPP)
                dimm->Voltages[rail] = bases[rail] + (FLOAT)vid * 0.005f;
            else
                dimm->Voltages[rail] = bases[rail] + (FLOAT)((vid >> 1) & 0x7F) * 0.005f;
        }

        dimm->Pmic = TRUE;
    }

    return !contended;
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
    {
        PhpHwDimms[i].Type = PhpHwDimmNone;
        PhpHwDimms[i].Pmic = FALSE;
    }

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

        if (found && !PhpHwProbePmics(Locator))
        {
            state.Contended = TRUE;
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
        static CONST PCWSTR rails[PH_HW_PMIC_RAILS] = { L"VDD", L"VDDQ", L"VPP" };
        PPHP_HW_DIMM dimm = &PhpHwDimms[i];
        PPH_STRING slot;
        PPH_STRING name;

        dimm->Disabled = FALSE;
        dimm->Failures = 0;
        dimm->PmicDisabled = FALSE;
        dimm->PmicFailures = 0;
        dimm->Index = ULONG_MAX;

        for (ULONG j = 0; j < PH_HW_PMIC_RAILS; j++)
            dimm->VoltageIndex[j] = ULONG_MAX;
        for (ULONG j = 0; j < PH_HW_PMIC_SENSORS; j++)
            dimm->PmicIndex[j] = ULONG_MAX;

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

        for (ULONG rail = 0; dimm->Pmic && rail < PH_HW_PMIC_RAILS; rail++)
        {
            name = PhFormatString(L"%s %s", slot->Buffer, rails[rail]);
            dimm->VoltageIndex[rail] = PhpHwAddNamedSensor(PhHwNamedSensorDimmVoltage, 0, i * PH_HW_PMIC_RAILS + rail, name->Buffer);
            PhpHwSetNamedSensor(dimm->VoltageIndex[rail], TRUE, dimm->Voltages[rail]);
            PhDereferenceObject(name);
        }

        PhCopyStringZ(slot->Buffer, SIZE_MAX, dimm->Slot, RTL_NUMBER_OF(dimm->Slot), NULL);
        PhDereferenceObject(slot);
    }

    PhpHwFreeMemoryDevices(&devices);
}

/**
 * Lists a PMIC sensor of a slot on its first reading.
 *
 * \param Dimm The slot.
 * \param Sensor The PH_HW_PMIC_SENSOR_* index.
 */
static VOID PhpHwListPmicSensor(
    _Inout_ PPHP_HW_DIMM Dimm,
    _In_ ULONG Sensor
    )
{
    static CONST PCWSTR rails[PH_HW_PMIC_RAILS] = { L"VDD", L"VDDQ", L"VPP" };
    ULONG i = (ULONG)(Dimm - PhpHwDimms);
    PH_HW_NAMED_SENSOR_KIND kind;
    ULONG id;
    PPH_STRING name;

    if (Sensor < PH_HW_PMIC_SENSOR_POWER)
    {
        kind = PhHwNamedSensorDimmCurrent;
        id = i * PH_HW_PMIC_RAILS + Sensor;
        name = PhFormatString(L"%s %s", Dimm->Slot, rails[Sensor]);
    }
    else if (Sensor < PH_HW_PMIC_SENSOR_POWER + PH_HW_PMIC_RAILS)
    {
        kind = PhHwNamedSensorDimmPower;
        id = i * (PH_HW_PMIC_RAILS + 1) + Sensor - PH_HW_PMIC_SENSOR_POWER;
        name = PhFormatString(L"%s %s", Dimm->Slot, rails[Sensor - PH_HW_PMIC_SENSOR_POWER]);
    }
    else if (Sensor < PH_HW_PMIC_SENSOR_TEMPERATURE)
    {
        kind = PhHwNamedSensorDimmPower;
        id = i * (PH_HW_PMIC_RAILS + 1) + PH_HW_PMIC_RAILS;
        name = PhFormatString(L"%s total", Dimm->Slot);
    }
    else
    {
        kind = PhHwNamedSensorDimmPmicTemperature;
        id = i;
        name = PhFormatString(L"%s PMIC (coarse)", Dimm->Slot);
    }

    Dimm->PmicIndex[Sensor] = PhpHwAddNamedSensor(kind, 0, id, name->Buffer);
    PhDereferenceObject(name);
}

/**
 * Sets the PMIC telemetry of a slot; without values every one has no value. A sensor is listed on its first reading.
 */
static VOID PhpHwSetPmicSensors(
    _Inout_ PPHP_HW_DIMM Dimm,
    _In_reads_opt_(PH_HW_PMIC_SENSORS) const BOOLEAN* Valid,
    _In_reads_opt_(PH_HW_PMIC_SENSORS) const FLOAT* Values
    )
{
    for (ULONG i = 0; Dimm->Pmic && i < PH_HW_PMIC_SENSORS; i++)
    {
        BOOLEAN valid = Valid && Values && Valid[i];

        if (valid && Dimm->PmicIndex[i] == ULONG_MAX)
            PhpHwListPmicSensor(Dimm, i);

        PhpHwSetNamedSensor(Dimm->PmicIndex[i], valid, Values ? Values[i] : 0.0f);
    }
}

/**
 * Drops a module: its temperature, set points and PMIC telemetry have no value.
 */
static VOID PhpHwSmbusDropDimm(
    _In_ PPHP_HW_DIMM Dimm
    )
{
    Dimm->Disabled = TRUE;
    PhpHwSetNamedSensor(Dimm->Index, FALSE, 0.0f);

    for (ULONG rail = 0; Dimm->Pmic && rail < PH_HW_PMIC_RAILS; rail++)
        PhpHwSetNamedSensor(Dimm->VoltageIndex[rail], FALSE, 0.0f);

    PhpHwSetPmicSensors(Dimm, NULL, NULL);
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
            PhpHwSmbusDropDimm(&PhpHwDimms[i]);

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
        PhpHwSmbusTimedOutPmic = 0;
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
 * Reads every module's temperature and PMIC telemetry in one call: an SPD5118 hub's page (MR11) and MR49:MR50, a
 * TSOD's TEMP, a PMIC's SWA, SWB, SWC, power and telemetry modes and temperature.
 *
 * \return STATUS_RETRY when the call met contention, else STATUS_SUCCESS.
 */
static NTSTATUS PhpHwSmbusRead(
    VOID
    )
{
    static CONST UCHAR pmicCommands[PHP_HW_PMIC_TELEMETRY] = { 0x0C, 0x0E, 0x0F, 0x1A, 0x1B, 0x33 };
    ULONG slots[PHP_HW_DIMM_COUNT];
    ULONG firsts[PHP_HW_DIMM_COUNT];
    ULONG pmicSlots[PHP_HW_DIMM_COUNT];
    ULONG pmicFirst;
    ULONG pmicCount = 0;
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

    pmicFirst = items;

    for (ULONG i = 0; i < PHP_HW_DIMM_COUNT; i++)
    {
        PPHP_HW_DIMM dimm = &PhpHwDimms[i];

        if (!dimm->Pmic || dimm->PmicDisabled || dimm->Type == PhpHwDimmNone || dimm->Disabled)
            continue;

        for (ULONG j = 0; j < PHP_HW_PMIC_TELEMETRY; j++)
            PhpHwSmbusItem(items++, (UCHAR)(PHP_HW_PMIC_ADDRESS + i), pmicCommands[j], PHP_HW_SMBUS_BYTE);

        pmicSlots[pmicCount++] = i;
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
        {
            PhTraceInfo("Hardware sensors: DIMM %lu unavailable: 0x%lx", slots[i], value->Status);
            PhpHwSmbusDropDimm(dimm);
            continue;
        }

        PhpHwSetNamedSensor(dimm->Index, valid, celsius);
    }

    for (ULONG i = 0; i < pmicCount; i++)
    {
        static CONST UCHAR severity[] = { 0, 1, 2, 3, 5, 4 }; // by PHP_HW_SMBUS_RESULT: Ok, Absent, Busy, NotReady, Timeout, Refused
        PKPH_HW_VALUE32 value = &PhpHwSmbusValues[pmicFirst + i * PHP_HW_PMIC_TELEMETRY];
        PPHP_HW_DIMM dimm = &PhpHwDimms[pmicSlots[i]];
        PHP_HW_SMBUS_RESULT result = PhpHwSmbusOk;
        NTSTATUS pmicStatus = STATUS_SUCCESS;
        BOOLEAN valid[PH_HW_PMIC_SENSORS];
        FLOAT values[PH_HW_PMIC_SENSORS];

        C_ASSERT(RTL_NUMBER_OF(severity) == PhpHwSmbusRefused + 1);

        // A module dropped in this pass takes its PMIC with it.
        if (dimm->Disabled)
            continue;

        // The most severe result decides.
        for (ULONG j = 0; j < PHP_HW_PMIC_TELEMETRY; j++)
        {
            PHP_HW_SMBUS_RESULT itemResult = PhpHwSmbusClassify(value[j].Status);

            if (severity[itemResult] > severity[result])
            {
                result = itemResult;
                pmicStatus = value[j].Status;
            }

            dimm->PmicTelemetry[j] = (UCHAR)value[j].Value;
        }

        dimm->PmicTelemetryRead = TRUE;
        dimm->PmicTelemetryStatus = pmicStatus;

        switch (result)
        {
        case PhpHwSmbusOk:
        case PhpHwSmbusAbsent:
            answered = TRUE;
            break;
        case PhpHwSmbusBusy:
            // Contention, or an item not attempted: no value this pass, nothing counted.
            PhpHwSetPmicSensors(dimm, NULL, NULL);
            continue;
        case PhpHwSmbusNotReady:
            busy = TRUE;
            break;
        case PhpHwSmbusTimeout:
            busy = TRUE;
            dimm->PmicDisabled = TRUE;
            break;
        case PhpHwSmbusRefused:
            dimm->PmicDisabled = TRUE;
            break;
        }

        if (result == PhpHwSmbusOk)
            dimm->PmicFailures = 0;
        else if (!dimm->PmicDisabled && ++dimm->PmicFailures >= PHP_HW_DIMM_FAILURE_LIMIT)
            dimm->PmicDisabled = TRUE;

        if (dimm->PmicDisabled)
            PhTraceInfo("Hardware sensors: DIMM %lu PMIC unavailable: 0x%lx", pmicSlots[i], pmicStatus);

        if (result != PhpHwSmbusOk)
        {
            PhpHwSetPmicSensors(dimm, NULL, NULL);
            continue;
        }

        PhHwDecodePmicTelemetry(
            (UCHAR)value[0].Value,
            (UCHAR)value[1].Value,
            (UCHAR)value[2].Value,
            (UCHAR)value[3].Value,
            (UCHAR)value[4].Value,
            (UCHAR)value[5].Value,
            valid,
            values
            );
        PhpHwSetPmicSensors(dimm, valid, values);
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
    ULONG capabilities = 0;

    if (!PhpHwSmbusEnded && PhpHwSmbusHost == ULONG_MAX && PhpHwSmbusProbeHost != ULONG_MAX && !PhpHwSmbusProbedThisUpdate)
        status = PhpHwSmbusProbeHosts();

    if (!PhpHwSmbusEnded && PhpHwSmbusHost != ULONG_MAX)
        status = PhpHwSmbusRead();

    for (ULONG i = 0; !PhpHwSmbusEnded && i < PHP_HW_DIMM_COUNT; i++)
    {
        PPHP_HW_DIMM dimm = &PhpHwDimms[i];

        if (dimm->Type == PhpHwDimmNone || dimm->Disabled)
            continue;

        capabilities |= PH_HW_CAP_DIMM_TEMP;

        if (dimm->Pmic)
            capabilities |= PH_HW_CAP_DIMM_VOLTAGE;
        if (dimm->Pmic && !dimm->PmicDisabled)
            capabilities |= PH_HW_CAP_DIMM_PMIC;
    }

    PhpHwSmbusProbedThisUpdate = FALSE;
    PhpHwFamilies[PhpHwFamilySmbus].Capabilities = capabilities;

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
    PhpHwSmbusTimedOutPmic = 0;
    PhpHwPmicReportMask = 0;
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

static BOOLEAN PhpHwOddParity(
    _In_ UCHAR Value
    )
{
    Value ^= Value >> 4;
    Value ^= Value >> 2;
    Value ^= Value >> 1;

    return Value & 1;
}

/**
 * Writes the PMIC5100 registers of a slot and their decode.
 */
static VOID PhpHwAppendPmic(
    _Inout_ PPH_STRING_BUILDER Builder,
    _In_reads_(PH_HW_PMIC_REPORT_REGISTERS) const UCHAR* Registers
    )
{
    static CONST struct
    {
        UCHAR Bank;
        UCHAR Id;
        PCWSTR Name;
    } vendors[] =
    {
        { 0x8A, 0x8C, L"Richtek" },
        { 0x04, 0x23, L"Renesas" },
        { 0x80, 0xB3, L"Renesas (IDT)" },
        { 0x0B, 0x2A, L"Monolithic Power Systems" },
        { 0x86, 0x32, L"Montage" },
        { 0x0B, 0x10, L"Anpec" },
        { 0x80, 0x97, L"Texas Instruments" },
        { 0x86, 0x9D, L"Rambus" },
    };
    UCHAR bank = PhpHwPmicRegister(Registers, 0x3C);
    UCHAR id = PhpHwPmicRegister(Registers, 0x3D);
    UCHAR revision = PhpHwPmicRegister(Registers, 0x3B);
    PCWSTR vendor = NULL;

    PhAppendStringBuilder2(Builder, L"\n    PMIC");

    for (ULONG i = 0; i < PH_HW_PMIC_REPORT_REGISTERS; i++)
    {
        if (!i || PhHwPmicCommands[i] != PhHwPmicCommands[i - 1] + 1)
            PhAppendFormatStringBuilder(Builder, L" %02Xh:", PhHwPmicCommands[i]);

        PhAppendFormatStringBuilder(Builder, L" %02x", Registers[i]);
    }

    // JEP106 bank and id, each with odd parity in bit 7.
    if (PhpHwOddParity(bank) && PhpHwOddParity(id) && (id & 0x7F) && (id & 0x7F) != 0x7F)
    {
        for (ULONG i = 0; i < RTL_NUMBER_OF(vendors); i++)
        {
            if (vendors[i].Bank == bank && vendors[i].Id == id)
            {
                vendor = vendors[i].Name;
                break;
            }
        }

        if (!vendor)
            vendor = L"unknown";
    }
    else
    {
        vendor = L"invalid";
    }

    PhAppendFormatStringBuilder(
        Builder,
        L"\n    PMIC vendor %s (0x%02x 0x%02x), revision %u.%u, %s mode, %s, %s, high voltage %s",
        vendor,
        bank,
        id,
        ((revision >> 4) & 0x3) + 1,
        (revision >> 1) & 0x7,
        (PhpHwPmicRegister(Registers, 0x2F) & 0x04) ? L"programmable" : L"secure",
        (PhpHwPmicRegister(Registers, 0x32) & 0x40) ? L"I3C" : L"I2C",
        (PhpHwPmicRegister(Registers, 0x1B) & 0x40) ? ((PhpHwPmicRegister(Registers, 0x1A) & 0x02) ? L"total power meter" : L"power meter") : L"current meter",
        PhHwPmicHighVoltage(Registers) ? L"yes" : L"no"
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

        if (type == PhpHwDimmNone && !(PhpHwPmicReportMask & (1 << i)) && !report->Ddr4Read && !report->HubRead)
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

        if (PhpHwPmicReportMask & (1 << i))
        {
            if (NT_SUCCESS(PhpHwPmicReportStatus[i]))
                PhpHwAppendPmic(Builder, PhpHwPmicReportRegisters[i]);
            else
                PhAppendFormatStringBuilder(Builder, L"\n    PMIC 0x%08lx", PhpHwPmicReportStatus[i]);
        }

        if (PhpHwDimms[i].PmicTelemetryRead)
        {
            PUCHAR telemetry = PhpHwDimms[i].PmicTelemetry;

            // A zero SWA (0Ch) is the meter off: no current or power has a value.
            PhAppendFormatStringBuilder(
                Builder,
                L"\n    PMIC telemetry: status 0x%08lx, 0Ch %02x 0Eh %02x 0Fh %02x 1Ah %02x 1Bh %02x 33h %02x%s",
                PhpHwDimms[i].PmicTelemetryStatus,
                telemetry[0],
                telemetry[1],
                telemetry[2],
                telemetry[3],
                telemetry[4],
                telemetry[5],
                NT_SUCCESS(PhpHwDimms[i].PmicTelemetryStatus) && !telemetry[0] ? L", meter off" : L""
                );
        }

        PhAppendCharStringBuilder(Builder, L'\n');
    }
}
