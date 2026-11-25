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

// Collector vrm: PMBus controllers found on an SMBus host's port 0 by what they answer (the script permits only its
// identification range), each page's output voltage, temperature and output power read in one paged read an update.

#define PHP_HW_VRM_MAX_HOSTS 4
#define PHP_HW_VRM_MAX 8 // controllers
#define PHP_HW_VRM_PAGES 2
#define PHP_HW_VRM_ID_BASE 0x56520000 // named sensor ids: + controller << 8 + page << 4 + quantity

typedef struct _PHP_HW_VRM
{
    UCHAR Address;
    PHP_HW_PMBUS_FAMILY Family;
    ULONG Named[PHP_HW_VRM_PAGES][3]; // voltage, temperature, power
} PHP_HW_VRM, *PPHP_HW_VRM;

static CONST UCHAR PhpHwVrmAddresses[] =
{
    0x40, 0x41, 0x42, 0x43, 0x44, 0x45, 0x46, 0x47, 0x60, 0x61, 0x62, 0x63, 0x64, 0x65, 0x66, 0x67,
};
static PHP_HW_VRM PhpHwVrms[PHP_HW_VRM_MAX];
static ULONG PhpHwVrmCount = 0;
static ULONG PhpHwVrmHost = 0;
static ULONG PhpHwVrmUnidentified = 0; // devices that answered and were not identified
static NTSTATUS PhpHwVrmStatus = STATUS_NOT_SUPPORTED; // the last read, for the report
static UCHAR PhpHwVrmStrikes = 0;
static BOOLEAN PhpHwVrmEnded = FALSE;

/**
 * Lists a controller's sensors: per page its output voltage, temperature and output power.
 */
static VOID PhpHwVrmList(
    _In_ ULONG Index
    )
{
    static CONST PH_HW_NAMED_SENSOR_KIND kinds[3] = { PhHwNamedSensorAcpiVoltage, PhHwNamedSensorAcpiTemperature, PhHwNamedSensorAcpiPower };
    static CONST PCWSTR quantities[3] = { L"voltage", L"temperature", L"power" };
    PPHP_HW_VRM vrm = &PhpHwVrms[Index];

    for (ULONG page = 0; page < PHP_HW_VRM_PAGES; page++)
    {
        for (ULONG q = 0; q < 3; q++)
        {
            PPH_STRING name = PhFormatString(L"VR %02Xh rail %lu %s", vrm->Address, page + 1, quantities[q]);

            vrm->Named[page][q] = PhpHwAddNamedSensor(kinds[q], 0, PHP_HW_VRM_ID_BASE + (Index << 8) + (page << 4) + q, name->Buffer);
            PhDereferenceObject(name);
        }
    }
}

/**
 * Finds the PMBus controllers once a session: each SMBus host's port 0 is asked at every address of the range, and a
 * device is kept only when its answers identify a family SI decodes; the first host with one is the host.
 *
 * \return STATUS_SUCCESS, STATUS_RETRY when another tool held the bus, or STATUS_NOT_SUPPORTED when none is found.
 */
NTSTATUS PhpHwVrmProbe(
    VOID
    )
{
    ULONG hosts[PHP_HW_VRM_MAX_HOSTS];
    ULONG hostCount;

    PhpHwVrmCount = 0;
    PhpHwVrmUnidentified = 0;
    hostCount = PhpHwEnumeratePciFunctions(
        PHP_HW_ANY_VENDOR_ID,
        PHP_HW_ANY_VENDOR_ID,
        0x0C05,
        NULL,
        0,
        FALSE,
        hosts,
        RTL_NUMBER_OF(hosts)
        );

    for (ULONG h = 0; h < hostCount && !PhpHwVrmCount; h++)
    {
        for (ULONG i = 0; i < RTL_NUMBER_OF(PhpHwVrmAddresses) && PhpHwVrmCount < PHP_HW_VRM_MAX; i++)
        {
            KPH_HW_PMBUS_IDENTITY identity;
            PHP_HW_PMBUS_FAMILY family;
            NTSTATUS status;

            status = KphHwPmbusIdentify(hosts[h], 0, PhpHwVrmAddresses[i], &identity);

            if (status == STATUS_RETRY)
                return STATUS_RETRY;

            if (!NT_SUCCESS(status))
                continue;

            family = PhpHwPmbusIdentify(
                identity.Revision,
                identity.MfrId,
                identity.MfrIdLength,
                identity.Model,
                identity.ModelLength,
                identity.DeviceId,
                identity.DeviceIdLength
                );

            if (family == PhpHwPmbusFamilyNone || family == PhpHwPmbusFamilyMps)
            {
                PhpHwVrmUnidentified++;
                continue;
            }

            PhpHwVrms[PhpHwVrmCount].Address = PhpHwVrmAddresses[i];
            PhpHwVrms[PhpHwVrmCount].Family = family;
            PhpHwVrmHost = hosts[h];
            PhpHwVrmList(PhpHwVrmCount);
            PhpHwVrmCount++;
        }
    }

    return PhpHwVrmCount ? STATUS_SUCCESS : STATUS_NOT_SUPPORTED;
}

/**
 * Reads every controller's pages in one paged read and publishes what decodes. Three failed reads in a row end the
 * source; another tool holding the bus is a retry.
 *
 * \return STATUS_RETRY when another tool held the bus, STATUS_NOT_SUPPORTED once ended, else STATUS_SUCCESS.
 */
NTSTATUS PhpHwVrmPass(
    VOID
    )
{
    static CONST UCHAR commands[4] = { 0x20, 0x8B, 0x8D, 0x96 }; // VOUT_MODE, READ_VOUT, READ_TEMPERATURE_1, READ_POUT
    KPH_HW_SMBUS_ITEM items[PHP_HW_VRM_MAX * PHP_HW_VRM_PAGES * 4];
    KPH_HW_VALUE32 values[RTL_NUMBER_OF(items)];
    ULONG count = 0;
    ULONG capabilities = 0;
    BOOLEAN read;

    if (PhpHwVrmEnded || !PhpHwVrmCount)
        return STATUS_NOT_SUPPORTED;

    for (ULONG i = 0; i < PhpHwVrmCount; i++)
    {
        for (ULONG page = 0; page < PHP_HW_VRM_PAGES; page++)
        {
            for (ULONG c = 0; c < 4; c++)
            {
                items[count].Address = PhpHwVrms[i].Address;
                items[count].Command = commands[c];
                items[count].Protocol = c ? KPH_HW_SMBUS_PAGED_WORD : KPH_HW_SMBUS_PAGED_BYTE;
                items[count].Reserved = (UCHAR)page;
                count++;
            }
        }
    }

    PhpHwVrmStatus = KphHwSmbusRead(PhpHwVrmHost, 0, items, count, values);

    if (PhpHwVrmStatus == STATUS_RETRY)
        return STATUS_RETRY;

    read = NT_SUCCESS(PhpHwVrmStatus);

    if (read)
        PhpHwVrmStrikes = 0;
    else if (++PhpHwVrmStrikes >= 3)
        PhpHwVrmEnded = TRUE;

    for (ULONG i = 0, at = 0; i < PhpHwVrmCount; i++)
    {
        PPHP_HW_VRM vrm = &PhpHwVrms[i];

        for (ULONG page = 0; page < PHP_HW_VRM_PAGES; page++, at += 4)
        {
            static CONST PHP_HW_PMBUS_QUANTITY quantities[3] = { PhpHwPmbusVout, PhpHwPmbusTemperature, PhpHwPmbusPout };
            BOOLEAN modeRead = read && NT_SUCCESS(values[at].Status);

            for (ULONG q = 0; q < 3; q++)
            {
                DOUBLE value = 0;
                BOOLEAN valid = read && NT_SUCCESS(values[at + 1 + q].Status) && (q != 0 || modeRead) &&
                    PhpHwPmbusDecode(vrm->Family, quantities[q], (UCHAR)values[at].Value, (USHORT)values[at + 1 + q].Value, &value);

                PhpHwSetNamedSensor(vrm->Named[page][q], valid, valid ? (FLOAT)value : 0.0f);

                if (valid)
                    capabilities |= q == 2 ? PH_HW_CAP_ACPI_POWER : PH_HW_CAP_ACPI_TEMP;
            }
        }
    }

    PhpHwFamilies[PhpHwFamilyVrm].Capabilities = PhpHwVrmEnded ? 0 : capabilities;

    return PhpHwVrmEnded ? STATUS_NOT_SUPPORTED : STATUS_SUCCESS;
}

/**
 * Ends the session: the controllers are found again at the next.
 */
VOID PhpHwVrmEnd(
    VOID
    )
{
    PhpHwVrmStatus = STATUS_NOT_SUPPORTED;
    PhpHwVrmCount = 0;
    PhpHwVrmStrikes = 0;
    PhpHwVrmEnded = FALSE;
}

/**
 * Writes the VRM source's section of the hardware report.
 *
 * \param Builder The report.
 */
VOID PhpHwVrmDiagnostics(
    _Inout_ PPH_STRING_BUILDER Builder
    )
{
    PhAppendFormatStringBuilder(
        Builder,
        L"  VR controllers: last read 0x%08lx, %lu found, %lu unidentified%s\n",
        PhpHwVrmStatus,
        PhpHwVrmCount,
        PhpHwVrmUnidentified,
        PhpHwVrmEnded ? L", ended" : L""
        );

    for (ULONG i = 0; i < PhpHwVrmCount; i++)
        PhAppendFormatStringBuilder(Builder, L"    %02Xh: family %d\n", PhpHwVrms[i].Address, PhpHwVrms[i].Family);
}
