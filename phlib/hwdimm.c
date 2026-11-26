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
#include <hwdimm.h>

CONST UCHAR PhHwPmicCommands[] =
{
    0x04, 0x05, 0x06, // error log of the previous power cycle
    0x08, 0x09, 0x0A, 0x0B, 0x0C, 0x0D, 0x0E, 0x0F, // status, SWA/SWB/SWC current or power
    0x1A, 0x1B, // power mode, telemetry mode
    0x20, 0x21, 0x22, 0x23, 0x24, 0x25, 0x26, 0x27, 0x28, 0x29, 0x2A, 0x2B, 0x2C, 0x2D, 0x2E, 0x2F, // limits, set points, LDO, protection
    0x30, 0x31, 0x32, 0x33, // ADC select and value, VR enable and I3C, temperature
    0x3B, 0x3C, 0x3D, // revision, vendor
};

/**
 * Decodes a JC-42.4 temperature: [12:0] two's complement in 1/16 degrees Celsius, read byte-swapped.
 */
FLOAT NTAPI PhHwTsodToCelsius(
    _In_ USHORT Value
    )
{
    LONG raw = _byteswap_ushort(Value) & 0x1FFF;

    if (raw & 0x1000)
        raw -= 0x2000;

    return (FLOAT)raw * 0.0625f;
}

/**
 * Decodes the SPD5118 MR49:MR50 temperature: [12:2] two's complement in 1/4 degrees Celsius.
 */
FLOAT NTAPI PhHwSpd5118ToCelsius(
    _In_ USHORT Value
    )
{
    LONG raw = (Value >> 2) & 0x7FF;

    if (raw & 0x400)
        raw -= 0x800;

    return (FLOAT)raw * 0.25f;
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

    assert(FALSE);
    return 0;
}

/**
 * Decides whether a PMIC set point is on the default range: 2Bh's range bit for it (bit 5 for 21h, 3 for 25h, 0 for
 * 27h) clear, or claimed by a vendor rule.
 */
BOOLEAN NTAPI PhHwPmicDefaultRange(
    _In_reads_(PH_HW_PMIC_REPORT_REGISTERS) const UCHAR* Registers,
    _In_ UCHAR SetPoint
    )
{
    UCHAR claimed = 0;
    UCHAR bit;

    // Vendor 8A8Ch: 2Bh[5:4] is its high-voltage mode.
    if (PhpHwPmicRegister(Registers, 0x3C) == 0x8A && PhpHwPmicRegister(Registers, 0x3D) == 0x8C)
        claimed = 0x30;

    switch (SetPoint)
    {
    case 0x21:
        bit = 0x20;
        break;
    case 0x25:
        bit = 0x08;
        break;
    case 0x27:
        bit = 0x01;
        break;
    default:
        return TRUE;
    }

    return (bit & claimed) || !(PhpHwPmicRegister(Registers, 0x2B) & bit);
}

/**
 * Decides whether a PMIC encodes the VDD and VDDQ set points in the whole byte (800 + 5n mV, a vendor high-voltage
 * mode) instead of the JEDEC [7:1].
 */
BOOLEAN NTAPI PhHwPmicHighVoltage(
    _In_reads_(PH_HW_PMIC_REPORT_REGISTERS) const UCHAR* Registers
    )
{
    UCHAR select = PhpHwPmicRegister(Registers, 0x30);
    UCHAR input = (select >> 3) & 0xF;
    UCHAR vid;
    FLOAT measured;
    FLOAT jedec;
    FLOAT wide;

    // Vendor 8A8Ch: 2Bh[5:4] = 11b.
    if (PhpHwPmicRegister(Registers, 0x3C) == 0x8A && PhpHwPmicRegister(Registers, 0x3D) == 0x8C)
        return (PhpHwPmicRegister(Registers, 0x2B) & 0x30) == 0x30;

    // Otherwise only an ADC another agent left enabled on SWA or SWB tells; the decode closer to it wins.
    if (!(select & 0x80) || (input != 0 && input != 2))
        return FALSE;

    vid = PhpHwPmicRegister(Registers, input == 0 ? 0x21 : 0x25);
    measured = (FLOAT)PhpHwPmicRegister(Registers, 0x31) * 0.015f;
    jedec = 0.8f + (FLOAT)((vid >> 1) & 0x7F) * 0.005f - measured;
    wide = 0.8f + (FLOAT)vid * 0.005f - measured;

    return wide * wide < jedec * jedec;
}

/**
 * Decodes the PMIC telemetry: SWA/SWB/SWC current, or power when 1Bh[6], or the module total in 0Ch when also
 * 1Ah[1]; the die temperature is a bucket, published only from 85 C.
 */
VOID NTAPI PhHwDecodePmicTelemetry(
    _In_ UCHAR Swa,
    _In_ UCHAR Swb,
    _In_ UCHAR Swc,
    _In_ UCHAR PowerMode,
    _In_ UCHAR TelemetryMode,
    _In_ UCHAR Temperature,
    _Out_writes_(PH_HW_PMIC_SENSORS) PBOOLEAN Valid,
    _Out_writes_(PH_HW_PMIC_SENSORS) PFLOAT Values
    )
{
    static CONST FLOAT buckets[8] = { 0.0f, 85.0f, 95.0f, 105.0f, 115.0f, 125.0f, 135.0f, 140.0f };
    FLOAT rails[PH_HW_PMIC_RAILS];
    UCHAR bucket = Temperature >> 5;

    memset(Valid, 0, PH_HW_PMIC_SENSORS * sizeof(BOOLEAN));
    memset(Values, 0, PH_HW_PMIC_SENSORS * sizeof(FLOAT));

    // 125 mA or 125 mW steps; SWB and SWC are 6 bits; a zero 0Ch is undefined (the meter is off), and so is the snapshot.
    rails[0] = (FLOAT)Swa * 0.125f;
    rails[1] = (FLOAT)(Swb & 0x3F) * 0.125f;
    rails[2] = (FLOAT)(Swc & 0x3F) * 0.125f;

    if (!(TelemetryMode & 0x40))
    {
        for (ULONG rail = 0; rail < PH_HW_PMIC_RAILS; rail++)
        {
            Valid[PH_HW_PMIC_SENSOR_CURRENT + rail] = !!Swa;
            Values[PH_HW_PMIC_SENSOR_CURRENT + rail] = rails[rail];
        }
    }
    else if (PowerMode & 0x02)
    {
        Valid[PH_HW_PMIC_SENSOR_POWER + PH_HW_PMIC_RAILS] = !!Swa;
        Values[PH_HW_PMIC_SENSOR_POWER + PH_HW_PMIC_RAILS] = rails[0];
    }
    else
    {
        for (ULONG rail = 0; rail < PH_HW_PMIC_RAILS; rail++)
        {
            Valid[PH_HW_PMIC_SENSOR_POWER + rail] = rail || Swa;
            Values[PH_HW_PMIC_SENSOR_POWER + rail] = rails[rail];
        }
    }

    if (bucket)
    {
        Valid[PH_HW_PMIC_SENSOR_TEMPERATURE] = TRUE;
        Values[PH_HW_PMIC_SENSOR_TEMPERATURE] = buckets[bucket];
    }
}
