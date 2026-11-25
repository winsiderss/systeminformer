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
#include <math.h>

// PMBus voltage regulator controllers: the families SI decodes and how each one's readings decode. LINEAR11 is a 5-bit
// signed exponent over an 11-bit signed mantissa; LINEAR16 (VOUT) an unsigned mantissa with VOUT_MODE's exponent;
// DIRECT is (Y x 10^-R - b) / m; a VID is a code of the scheme the family names.

typedef enum _PHP_HW_PMBUS_VID
{
    PhpHwPmbusVidNone,
    PhpHwPmbusVidVr12, // 250 mV + 5 mV a step from code 1
    PhpHwPmbusVidVr13, // 500 mV + 10 mV a step from code 1
    PhpHwPmbusVidImvp9, // 200 mV + 10 mV a step from code 1
    PhpHwPmbusVidAmd625 // 1550 mV - 6.25 mV a step from code 0 to D8h
} PHP_HW_PMBUS_VID;

/**
 * The value of a LINEAR11 reading.
 */
static DOUBLE PhpHwPmbusLinear11(
    _In_ USHORT Raw
    )
{
    LONG exponent = (SHORT)Raw >> 11;
    LONG mantissa = ((LONG)(Raw & 0x7FF) ^ 0x400) - 0x400;

    return ldexp((DOUBLE)mantissa, exponent);
}

/**
 * The volts of a VID code under a scheme; FALSE when the code is outside it.
 */
static BOOLEAN PhpHwPmbusVidVolts(
    _In_ PHP_HW_PMBUS_VID Scheme,
    _In_ USHORT Code,
    _Out_ PDOUBLE Volts
    )
{
    switch (Scheme)
    {
    case PhpHwPmbusVidVr12:
        if (Code < 1)
            return FALSE;
        *Volts = (250.0 + (Code - 1) * 5.0) / 1000.0;
        return TRUE;
    case PhpHwPmbusVidVr13:
        if (Code < 1)
            return FALSE;
        *Volts = (500.0 + (Code - 1) * 10.0) / 1000.0;
        return TRUE;
    case PhpHwPmbusVidImvp9:
        if (Code < 1)
            return FALSE;
        *Volts = (200.0 + (Code - 1) * 10.0) / 1000.0;
        return TRUE;
    case PhpHwPmbusVidAmd625:
        if (Code > 0xD8)
            return FALSE;
        *Volts = (1550.0 - Code * 6.25) / 1000.0;
        return TRUE;
    }

    return FALSE;
}

/**
 * The VID scheme a family's VOUT_MODE parameter (bits 4:0) names.
 */
static PHP_HW_PMBUS_VID PhpHwPmbusVidScheme(
    _In_ PHP_HW_PMBUS_FAMILY Family,
    _In_ UCHAR Parameter
    )
{
    switch (Family)
    {
    case PhpHwPmbusFamilyInfineonXdpe122:
        switch (Parameter)
        {
        case 0x01:
            return PhpHwPmbusVidVr12;
        case 0x02:
            return PhpHwPmbusVidVr13;
        case 0x03:
            return PhpHwPmbusVidImvp9;
        case 0x10:
            return PhpHwPmbusVidAmd625;
        }
        break;
    case PhpHwPmbusFamilyTiVid:
        switch (Parameter)
        {
        case 0x02:
        case 0x04:
            return PhpHwPmbusVidVr13;
        case 0x01:
        case 0x05:
        case 0x07:
            return PhpHwPmbusVidVr12;
        }
        break;
    }

    return PhpHwPmbusVidNone;
}

/**
 * Decodes a reading of a PMBus controller.
 *
 * \param Family The controller's family.
 * \param Quantity What the reading is.
 * \param VoutMode The page's VOUT_MODE, for an output voltage.
 * \param Raw The reading's word.
 * \param Value Receives volts, amperes, watts or degrees C.
 * \return TRUE when the family's reading of the quantity decodes.
 */
_Success_(return)
BOOLEAN PhpHwPmbusDecode(
    _In_ PHP_HW_PMBUS_FAMILY Family,
    _In_ PHP_HW_PMBUS_QUANTITY Quantity,
    _In_ UCHAR VoutMode,
    _In_ USHORT Raw,
    _Out_ PDOUBLE Value
    )
{
    static CONST CHAR directR[PhpHwPmbusQuantityMaximum] = { 2, 3, 2, 1, 0, 0, 0 }; // Renesas: VIN, VOUT, IIN, IOUT, PIN, POUT, temperature

    switch (Family)
    {
    case PhpHwPmbusFamilyRenesasRaa:
        *Value = (SHORT)Raw * pow(10.0, -directR[Quantity]);
        return TRUE;
    case PhpHwPmbusFamilyMps:
        if (Quantity != PhpHwPmbusVin)
            return FALSE;
        *Value = PhpHwPmbusLinear11(Raw);
        return TRUE;
    case PhpHwPmbusFamilyInfineonXdpe122:
    case PhpHwPmbusFamilyTiVid:
    case PhpHwPmbusFamilyTiLinear:
    case PhpHwPmbusFamilyIr35221:
        break;
    default:
        return FALSE;
    }

    if (Quantity != PhpHwPmbusVout)
    {
        *Value = PhpHwPmbusLinear11(Raw);
        return TRUE;
    }

    switch (VoutMode >> 5)
    {
    case 0:
        *Value = ldexp((DOUBLE)Raw, (LONG)((VoutMode & 0x1F) ^ 0x10) - 0x10);
        return TRUE;
    case 1:
        return PhpHwPmbusVidVolts(PhpHwPmbusVidScheme(Family, VoutMode & 0x1F), Raw, Value);
    }

    return FALSE;
}

/**
 * Identifies a PMBus controller from what it answers; only families whose answers are known are identified here.
 *
 * \param Revision PMBUS_REVISION (98h).
 * \param MfrId The MFR_ID (99h) block.
 * \param MfrIdLength The block's length.
 * \param Model The MFR_MODEL (9Ah) block.
 * \param ModelLength The block's length.
 * \param DeviceId The IC_DEVICE_ID (ADh) block.
 * \param DeviceIdLength The block's length.
 * \return The family, or PhpHwPmbusFamilyNone.
 */
PHP_HW_PMBUS_FAMILY PhpHwPmbusIdentify(
    _In_ UCHAR Revision,
    _In_reads_bytes_(MfrIdLength) const UCHAR* MfrId,
    _In_ ULONG MfrIdLength,
    _In_reads_bytes_(ModelLength) const UCHAR* Model,
    _In_ ULONG ModelLength,
    _In_reads_bytes_(DeviceIdLength) const UCHAR* DeviceId,
    _In_ ULONG DeviceIdLength
    )
{
    static CONST struct
    {
        UCHAR Revision; // 0 any
        UCHAR Length;
        UCHAR Id[6];
        PHP_HW_PMBUS_FAMILY Family;
    } devices[] =
    {
        { 0x33, 1, { 0x81 }, PhpHwPmbusFamilyTiVid }, // TPS53681
        { 0x33, 5, { 'T', 'I', 'S', 'h', 'P' }, PhpHwPmbusFamilyTiLinear }, // TPS53685
        { 0, 5, { 'T', 'I', 0x53, 0x67, 0x60 }, PhpHwPmbusFamilyTiLinear }, // TPS53676
        { 0, 5, { 'T', 'I', 0x53, 0x6C, 0x70 }, PhpHwPmbusFamilyTiLinear }, // TPS536C7
    };
    ULONG length = DeviceIdLength;

    if (MfrIdLength == 2 && MfrId[0] == 'R' && MfrId[1] == 'I' && ModelLength == 2 && Model[0] == 0x6C && Model[1] == 0)
        return PhpHwPmbusFamilyIr35221;

    if (length && DeviceId[length - 1] == 0)
        length--;

    for (ULONG i = 0; i < RTL_NUMBER_OF(devices); i++)
    {
        if (devices[i].Revision && devices[i].Revision != Revision)
            continue;
        if (length == devices[i].Length && memcmp(DeviceId, devices[i].Id, length) == 0)
            return devices[i].Family;
    }

    return PhpHwPmbusFamilyNone;
}
