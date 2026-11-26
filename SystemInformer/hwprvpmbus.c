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
    _In_reads_bytes_(MfrIdLength) PCUCHAR MfrId,
    _In_ ULONG MfrIdLength,
    _In_reads_bytes_(ModelLength) PCUCHAR Model,
    _In_ ULONG ModelLength,
    _In_reads_bytes_(DeviceIdLength) PCUCHAR DeviceId,
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
#define PHP_HW_VRM_MODE_INTERVAL 30 // passes between reads of a page's VOUT_MODE once it is known
#define PHP_HW_VRM_ID_BASE 0x56520000 // named sensor ids: + controller << 8 + page << 4 + quantity

typedef struct _PHP_HW_VRM
{
    UCHAR Address;
    PHP_HW_PMBUS_FAMILY Family;
    ULONG Named[PHP_HW_VRM_PAGES][3]; // voltage, temperature, power
    BOOLEAN ModeKnown[PHP_HW_VRM_PAGES];
    UCHAR Mode[PHP_HW_VRM_PAGES]; // VOUT_MODE
    UCHAR Strikes; // reads in a row in which it was asked and nothing of it answered
    BOOLEAN Ended;
} PHP_HW_VRM, *PPHP_HW_VRM;

static CONST UCHAR PhpHwVrmAddresses[] =
{
    0x40, 0x41, 0x42, 0x43, 0x44, 0x45, 0x46, 0x47, 0x60, 0x61, 0x62, 0x63, 0x64, 0x65, 0x66, 0x67,
};
// Boards whose VRM telemetry is read elsewhere (the board's embedded controller): no discovery on them.
static CONST struct
{
    PCWSTR Manufacturer;
    PCWSTR Product;
} PhpHwVrmBoardRows[] =
{
    { L"ASUSTeK COMPUTER INC.", L"ROG CROSSHAIR X870E APEX" },
};

static PHP_HW_VRM PhpHwVrms[PHP_HW_VRM_MAX];
static BOOLEAN PhpHwVrmRowBoard = FALSE; // the board has a row: discovery does not run
static ULONG PhpHwVrmCount = 0;
static ULONG PhpHwVrmHost = 0;
static ULONG PhpHwVrmUnidentified = 0; // devices that answered and were not identified
static NTSTATUS PhpHwVrmStatus = STATUS_NOT_SUPPORTED; // the last read, for the report
static ULONG PhpHwVrmPasses = 0;
static BOOLEAN PhpHwVrmEnded = FALSE;
static ULONG PhpHwVrmHosts[PHP_HW_VRM_MAX_HOSTS];
static ULONG PhpHwVrmHostCount = 0;
static BOOLEAN PhpHwVrmProbing = FALSE; // another tool held the bus: the probe goes on from the host and address below
static ULONG PhpHwVrmProbeHost = 0;
static ULONG PhpHwVrmProbeAddress = 0;

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
 * Whether the board has a row: its VRM telemetry is read elsewhere.
 */
static BOOLEAN PhpHwVrmBoardHasRow(
    VOID
    )
{
    PPH_STRING manufacturer;
    PPH_STRING product;
    BOOLEAN found = FALSE;

    if (!PhpHwQueryBaseboard(&manufacturer, &product))
        return FALSE;

    for (ULONG i = 0; i < RTL_NUMBER_OF(PhpHwVrmBoardRows) && !found; i++)
    {
        found = PhEqualString2(manufacturer, PhpHwVrmBoardRows[i].Manufacturer, TRUE) &&
            PhEqualString2(product, PhpHwVrmBoardRows[i].Product, TRUE);
    }

    PhDereferenceObject(manufacturer);
    PhDereferenceObject(product);

    return found;
}

/**
 * Asks the addresses from the probe's host and address on: each SMBus host's port 0 at every address of the range,
 * a device kept only when its answers identify a family SI decodes; the first host with one is the host. Another tool
 * holding the bus stops it there.
 */
static VOID PhpHwVrmProbeAddresses(
    VOID
    )
{
    PhpHwVrmProbing = FALSE;

    while (PhpHwVrmProbeHost < PhpHwVrmHostCount)
    {
        ULONG host = PhpHwVrmHosts[PhpHwVrmProbeHost];

        for (; PhpHwVrmProbeAddress < RTL_NUMBER_OF(PhpHwVrmAddresses) && PhpHwVrmCount < PHP_HW_VRM_MAX; PhpHwVrmProbeAddress++)
        {
            ULONG i = PhpHwVrmProbeAddress;
            KPH_HW_PMBUS_IDENTITY identity;
            PHP_HW_PMBUS_FAMILY family;
            NTSTATUS status;

            status = KphHwPmbusIdentify(host, 0, PhpHwVrmAddresses[i], &identity);

            if (status == STATUS_RETRY)
            {
                PhpHwVrmProbing = TRUE;
                return;
            }

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
            memset(PhpHwVrms[PhpHwVrmCount].ModeKnown, 0, sizeof(PhpHwVrms[PhpHwVrmCount].ModeKnown));
            PhpHwVrms[PhpHwVrmCount].Strikes = 0;
            PhpHwVrms[PhpHwVrmCount].Ended = FALSE;
            PhpHwVrmHost = host;
            PhpHwVrmList(PhpHwVrmCount);
            PhpHwVrmCount++;
        }

        if (PhpHwVrmCount)
            break;

        PhpHwVrmProbeHost++;
        PhpHwVrmProbeAddress = 0;
    }
}

/**
 * Finds the PMBus controllers once a session on a board without a row.
 *
 * \return STATUS_SUCCESS (also when another tool held the bus: the pass goes on with the probe), or
 * STATUS_NOT_SUPPORTED when none is found or the board has a row.
 */
_Function_class_(PHP_HW_FAMILY_PROBE)
NTSTATUS NTAPI PhpHwVrmProbe(
    VOID
    )
{
    PhpHwVrmCount = 0;
    PhpHwVrmUnidentified = 0;
    PhpHwVrmRowBoard = PhpHwVrmBoardHasRow();

    if (PhpHwVrmRowBoard)
        return STATUS_NOT_SUPPORTED;

    PhpHwVrmHostCount = PhpHwEnumeratePciFunctions(
        PHP_HW_ANY_VENDOR_ID,
        PHP_HW_ANY_VENDOR_ID,
        0x0C05,
        NULL,
        0,
        FALSE,
        PhpHwVrmHosts,
        RTL_NUMBER_OF(PhpHwVrmHosts)
        );
    PhpHwVrmProbeHost = 0;
    PhpHwVrmProbeAddress = 0;
    PhpHwVrmProbeAddresses();

    if (PhpHwVrmProbing)
        return STATUS_SUCCESS;

    return PhpHwVrmCount ? STATUS_SUCCESS : STATUS_NOT_SUPPORTED;
}

/**
 * Reads every controller's pages in one paged read and publishes what decodes. Three failed reads in a row end the
 * source; another tool holding the bus is a retry.
 *
 * \return STATUS_RETRY when another tool held the bus, STATUS_NOT_SUPPORTED once ended, else STATUS_SUCCESS.
 */
_Function_class_(PHP_HW_FAMILY_PASS)
NTSTATUS NTAPI PhpHwVrmPass(
    VOID
    )
{
    static CONST UCHAR commands[4] = { 0x20, 0x8B, 0x8D, 0x96 }; // VOUT_MODE, READ_VOUT, READ_TEMPERATURE_1, READ_POUT
    KPH_HW_SMBUS_ITEM items[PHP_HW_VRM_MAX * PHP_HW_VRM_PAGES * 4];
    KPH_HW_VALUE32 values[RTL_NUMBER_OF(items)];
    BOOLEAN asked[PHP_HW_VRM_MAX];
    ULONG count = 0;
    ULONG capabilities = 0;
    BOOLEAN read;
    BOOLEAN ended = TRUE;
    BOOLEAN modePass;

    if (PhpHwVrmProbing)
    {
        PhpHwVrmProbeAddresses();

        if (PhpHwVrmProbing)
            return STATUS_RETRY;
    }

    if (PhpHwVrmEnded || !PhpHwVrmCount)
        return STATUS_NOT_SUPPORTED;

    modePass = PhpHwVrmPasses++ % PHP_HW_VRM_MODE_INTERVAL == 0;

    for (ULONG i = 0; i < PhpHwVrmCount; i++)
    {
        asked[i] = !PhpHwVrms[i].Ended;

        for (ULONG page = 0; asked[i] && page < PHP_HW_VRM_PAGES; page++)
        {
            ULONG first = 1;

            if (modePass || !PhpHwVrms[i].ModeKnown[page])
                first = 0;

            for (ULONG c = first; c < 4; c++)
            {
                items[count].Address = PhpHwVrms[i].Address;
                items[count].Command = commands[c];
                items[count].Protocol = c ? KPH_HW_SMBUS_PAGED_WORD : KPH_HW_SMBUS_PAGED_BYTE;
                items[count].Page = (UCHAR)page;
                count++;
            }
        }
    }

    PhpHwVrmStatus = KphHwSmbusRead(PhpHwVrmHost, 0, items, count, values);

    if (PhpHwVrmStatus == STATUS_RETRY)
        return STATUS_RETRY;

    read = NT_SUCCESS(PhpHwVrmStatus);

    for (ULONG i = 0, at = 0; i < PhpHwVrmCount; i++)
    {
        PPHP_HW_VRM vrm = &PhpHwVrms[i];
        ULONG first = at;

        for (ULONG page = 0; asked[i] && page < PHP_HW_VRM_PAGES; page++, at += 3)
        {
            static CONST PHP_HW_PMBUS_QUANTITY quantities[3] = { PhpHwPmbusVout, PhpHwPmbusTemperature, PhpHwPmbusPout };

            if (modePass || !vrm->ModeKnown[page])
            {
                vrm->ModeKnown[page] = read && NT_SUCCESS(values[at].Status);
                vrm->Mode[page] = (UCHAR)values[at].Value;
                at++;
            }

            for (ULONG q = 0; q < 3; q++)
            {
                DOUBLE value = 0;
                BOOLEAN valid = read && NT_SUCCESS(values[at + q].Status) && (q != 0 || vrm->ModeKnown[page]) &&
                    PhpHwPmbusDecode(vrm->Family, quantities[q], vrm->Mode[page], (USHORT)values[at + q].Value, &value);

                FLOAT published = 0.0f;

                if (valid)
                    published = (FLOAT)value;

                PhpHwSetNamedSensor(vrm->Named[page][q], valid, published);

                if (valid)
                    capabilities |= q == 2 ? PH_HW_CAP_ACPI_POWER : PH_HW_CAP_ACPI_TEMP;
            }
        }

        // A controller strikes when its items were reached and none answered: contention is an answer, and an item
        // cancelled after another's failure was not reached. A read that failed counts against every one asked.
        if (asked[i])
        {
            BOOLEAN answered = FALSE;
            BOOLEAN reached = !read;

            for (ULONG item = first; read && item < at; item++)
            {
                NTSTATUS status = values[item].Status;

                if (NT_SUCCESS(status) || status == STATUS_RETRY || status == STATUS_DEVICE_BUSY)
                    answered = TRUE;
                if (status != STATUS_CANCELLED)
                    reached = TRUE;
            }

            if (answered)
                vrm->Strikes = 0;
            else if (reached && ++vrm->Strikes >= 3)
                vrm->Ended = TRUE;
        }

        if (!vrm->Ended)
            ended = FALSE;
    }

    PhpHwVrmEnded = ended;
    PhpHwFamilies[PhpHwFamilyVrm].Capabilities = PhpHwVrmEnded ? 0 : capabilities;

    return PhpHwVrmEnded ? STATUS_NOT_SUPPORTED : STATUS_SUCCESS;
}

/**
 * Ends the session: the controllers are found again at the next.
 */
_Function_class_(PHP_HW_FAMILY_END)
VOID NTAPI PhpHwVrmEnd(
    VOID
    )
{
    PhpHwVrmStatus = STATUS_NOT_SUPPORTED;
    PhpHwVrmCount = 0;
    PhpHwVrmPasses = 0;
    PhpHwVrmEnded = FALSE;
    PhpHwVrmProbing = FALSE;
}

/**
 * Writes the VRM source's section of the hardware report.
 *
 * \param Builder The report.
 */
_Function_class_(PHP_HW_FAMILY_DIAGNOSTICS)
VOID NTAPI PhpHwVrmDiagnostics(
    _Inout_ PPH_STRING_BUILDER Builder
    )
{
    PCWSTR ended = L"";
    PCWSTR rowBoard = L"";

    if (PhpHwVrmEnded)
        ended = L", ended";
    if (PhpHwVrmRowBoard)
        rowBoard = L", the board's row (no discovery)";

    PhAppendFormatStringBuilder(
        Builder,
        L"  VR controllers: last read 0x%08lx, %lu found, %lu unidentified%s%s\n",
        PhpHwVrmStatus,
        PhpHwVrmCount,
        PhpHwVrmUnidentified,
        ended,
        rowBoard
        );

    for (ULONG i = 0; i < PhpHwVrmCount; i++)
    {
        PCWSTR controllerEnded = L"";

        if (PhpHwVrms[i].Ended)
            controllerEnded = L", ended";

        PhAppendFormatStringBuilder(
            Builder,
            L"    %02Xh: family %d%s\n",
            PhpHwVrms[i].Address,
            PhpHwVrms[i].Family,
            controllerEnded
            );
    }
}

// Collector sch5027: an SCH5027 hardware monitor on an Intel SMBus host at 2Ch-2Eh, accepted on its company and stepping;
// voltages, temperatures and fans read in one SMBus read an update (the high bytes before the low-bit registers, which
// latch when the high bytes are read).

#define PHP_HW_SCH5027_ID_BASE 0x53430000 // named sensor ids: + kind << 8 + index
#define PHP_HW_SCH5027_ITEMS 24

static CONST UCHAR PhpHwSch5027Commands[PHP_HW_SCH5027_ITEMS] =
{
    0x20, 0x21, 0x22, 0x23, 0x24, 0x99, 0x9A, // in0-in6
    0x25, 0x26, 0x27, // temperatures 1-3
    0x84, 0x85, 0x86, 0x87, 0x88, 0x89, // low bits
    0x28, 0x29, 0x2A, 0x2B, 0x2C, 0x2D, 0x2E, 0x2F, // fans 1-4, low byte first
};
static CONST USHORT PhpHwSch5027Nominal[7] = { 5000, 2250, 3300, 1125, 1125, 3300, 3300 }; // mV at full scale
static CONST UCHAR PhpHwSch5027InLsb[7] = { 3, 4, 4, 3, 2, 0, 0 }; // low-bit register (84h + n) of in0-in6
static CONST UCHAR PhpHwSch5027InShift[7] = { 4, 4, 0, 0, 0, 0, 4 };
static CONST UCHAR PhpHwSch5027TempLsb[3] = { 1, 2, 1 };
static CONST UCHAR PhpHwSch5027TempShift[3] = { 4, 4, 0 };

static ULONG PhpHwSch5027Host = 0;
static UCHAR PhpHwSch5027Address = 0;
static ULONG PhpHwSch5027Named[7 + 3 + 4];
static NTSTATUS PhpHwSch5027Status = STATUS_NOT_SUPPORTED; // the last read, for the report
static UCHAR PhpHwSch5027Strikes = 0;
static BOOLEAN PhpHwSch5027Ended = FALSE;
static BOOLEAN PhpHwSch5027Found = FALSE;
static ULONG PhpHwSch5027Hosts[PHP_HW_VRM_MAX_HOSTS];
static ULONG PhpHwSch5027HostCount = 0;
static BOOLEAN PhpHwSch5027Probing = FALSE; // another tool held the bus: the probe goes on from the host below
static ULONG PhpHwSch5027ProbeHost = 0;

/**
 * Asks the hosts from the probe's host on: each Intel SMBus host's port 0 at 2Ch-2Eh for the identification bytes,
 * the chip accepted only on company 5Ch and stepping 69h, and its sensors listed. Another tool holding the bus stops
 * it there.
 */
static VOID PhpHwSch5027ProbeHosts(
    VOID
    )
{
    static CONST UCHAR addresses[3] = { 0x2C, 0x2D, 0x2E };

    PhpHwSch5027Probing = FALSE;

    for (; PhpHwSch5027ProbeHost < PhpHwSch5027HostCount && !PhpHwSch5027Found; PhpHwSch5027ProbeHost++)
    {
        ULONG h = PhpHwSch5027ProbeHost;
        KPH_HW_SMBUS_ITEM items[9];
        KPH_HW_VALUE32 values[9];
        NTSTATUS status;

        for (ULONG i = 0; i < 9; i++)
        {
            items[i].Address = addresses[i / 3];
            items[i].Command = (UCHAR)(0x3D + i % 3);
            items[i].Protocol = KPH_HW_SMBUS_BYTE;
            items[i].Page = 0;
        }

        status = KphHwSmbusRead(PhpHwSch5027Hosts[h], 0, items, RTL_NUMBER_OF(items), values);

        // Another tool on the bus at an item leaves the rest unread: the host is asked again.
        for (ULONG i = 0; NT_SUCCESS(status) && i < RTL_NUMBER_OF(items); i++)
        {
            if (values[i].Status == STATUS_RETRY)
                status = STATUS_RETRY;
        }

        if (status == STATUS_RETRY)
        {
            PhpHwSch5027Probing = TRUE;
            return;
        }

        if (!NT_SUCCESS(status))
            continue;

        for (ULONG a = 0; a < 3 && !PhpHwSch5027Found; a++)
        {
            PKPH_HW_VALUE32 id = &values[a * 3];

            if (NT_SUCCESS(id[1].Status) && NT_SUCCESS(id[2].Status) && id[1].Value == 0x5C && id[2].Value == 0x69)
            {
                PhpHwSch5027Host = PhpHwSch5027Hosts[h];
                PhpHwSch5027Address = addresses[a];
                PhpHwSch5027Found = TRUE;
            }
        }
    }

    if (!PhpHwSch5027Found)
        return;

    for (ULONG i = 0; i < RTL_NUMBER_OF(PhpHwSch5027Named); i++)
    {
        PH_HW_NAMED_SENSOR_KIND kind;
        PPH_STRING name;
        ULONG index;

        if (i < 7)
        {
            kind = PhHwNamedSensorAcpiVoltage;
            index = i;
            name = PhFormatString(L"SCH5027 in%lu", index);
        }
        else if (i < 10)
        {
            kind = PhHwNamedSensorAcpiTemperature;
            index = i - 7;
            name = PhFormatString(L"SCH5027 temperature %lu", index + 1);
        }
        else
        {
            kind = PhHwNamedSensorFanSpeed;
            index = i - 10;
            name = PhFormatString(L"SCH5027 fan %lu", index + 1);
        }

        PhpHwSch5027Named[i] = PhpHwAddNamedSensor(kind, 0, PHP_HW_SCH5027_ID_BASE + ((ULONG)kind << 8) + index, name->Buffer);
        PhDereferenceObject(name);
    }
}

/**
 * Finds an SCH5027 once a session.
 *
 * \return STATUS_SUCCESS (also when another tool held the bus: the pass goes on with the probe), or
 * STATUS_NOT_SUPPORTED when there is none.
 */
_Function_class_(PHP_HW_FAMILY_PROBE)
NTSTATUS NTAPI PhpHwSch5027Probe(
    VOID
    )
{
    PhpHwSch5027Found = FALSE;
    PhpHwSch5027HostCount = PhpHwEnumeratePciFunctions(0x8086, 0x8086, 0x0C05, NULL, 0, FALSE, PhpHwSch5027Hosts, RTL_NUMBER_OF(PhpHwSch5027Hosts));
    PhpHwSch5027ProbeHost = 0;
    PhpHwSch5027ProbeHosts();

    if (PhpHwSch5027Probing)
        return STATUS_SUCCESS;

    return PhpHwSch5027Found ? STATUS_SUCCESS : STATUS_NOT_SUPPORTED;
}

/**
 * Reads and publishes the monitor: a voltage is its 12-bit reading of the input's nominal full scale, a temperature
 * the 12-bit two's complement degrees C (-128 no value), a fan 5400000 / count RPM (0 and FFFFh no value). Three
 * failed reads in a row end the source; another tool holding the bus is a retry.
 *
 * \return STATUS_RETRY when another tool held the bus, STATUS_NOT_SUPPORTED once ended, else STATUS_SUCCESS.
 */
_Function_class_(PHP_HW_FAMILY_PASS)
NTSTATUS NTAPI PhpHwSch5027Pass(
    VOID
    )
{
    KPH_HW_SMBUS_ITEM items[PHP_HW_SCH5027_ITEMS];
    KPH_HW_VALUE32 values[PHP_HW_SCH5027_ITEMS];
    ULONG capabilities = 0;
    BOOLEAN read;
    BOOLEAN contended = FALSE;

    if (PhpHwSch5027Probing)
    {
        PhpHwSch5027ProbeHosts();

        if (PhpHwSch5027Probing)
            return STATUS_RETRY;
    }

    if (PhpHwSch5027Ended || !PhpHwSch5027Found)
        return STATUS_NOT_SUPPORTED;

    for (ULONG i = 0; i < PHP_HW_SCH5027_ITEMS; i++)
    {
        items[i].Address = PhpHwSch5027Address;
        items[i].Command = PhpHwSch5027Commands[i];
        items[i].Protocol = KPH_HW_SMBUS_BYTE;
        items[i].Page = 0;
    }

    PhpHwSch5027Status = KphHwSmbusRead(PhpHwSch5027Host, 0, items, PHP_HW_SCH5027_ITEMS, values);

    if (PhpHwSch5027Status == STATUS_RETRY)
        return STATUS_RETRY;

    read = NT_SUCCESS(PhpHwSch5027Status);

    for (ULONG i = 0; i < PHP_HW_SCH5027_ITEMS; i++)
    {
        if (read && !NT_SUCCESS(values[i].Status))
        {
            read = FALSE;
            contended = values[i].Status == STATUS_RETRY || values[i].Status == STATUS_DEVICE_BUSY;
        }
    }

    if (read)
        PhpHwSch5027Strikes = 0;
    else if (!contended && ++PhpHwSch5027Strikes >= 3)
        PhpHwSch5027Ended = TRUE;

    for (ULONG i = 0; i < 7; i++)
    {
        ULONG reading = read ? ((values[i].Value & 0xFF) << 8) | ((values[10 + PhpHwSch5027InLsb[i]].Value << PhpHwSch5027InShift[i]) & 0xF0) : 0;

        FLOAT volts = 0.0f;

        if (read)
            volts = (FLOAT)reading * PhpHwSch5027Nominal[i] / 49152.0f / 1000.0f;

        PhpHwSetNamedSensor(PhpHwSch5027Named[i], read, volts);
    }

    for (ULONG i = 0; i < 3; i++)
    {
        SHORT reading = read ? (SHORT)(((values[7 + i].Value & 0xFF) << 8) | ((values[10 + PhpHwSch5027TempLsb[i]].Value << PhpHwSch5027TempShift[i]) & 0xF0)) : 0;
        BOOLEAN valid = read && (values[7 + i].Value & 0xFF) != 0x80;
        FLOAT celsius = 0.0f;

        if (valid)
            celsius = reading / 256.0f;

        PhpHwSetNamedSensor(PhpHwSch5027Named[7 + i], valid, celsius);
    }

    for (ULONG i = 0; i < 4; i++)
    {
        ULONG count = read ? (values[16 + i * 2].Value & 0xFF) | ((values[17 + i * 2].Value & 0xFF) << 8) : 0;
        BOOLEAN valid = read && count != 0 && count != 0xFFFF;
        FLOAT rpm = 0.0f;

        if (valid)
            rpm = 5400000.0f / count;

        PhpHwSetNamedSensor(PhpHwSch5027Named[10 + i], valid, rpm);

        if (valid)
            capabilities |= PH_HW_CAP_FAN_SPEED;
    }

    if (read)
        capabilities |= PH_HW_CAP_ACPI_TEMP;

    PhpHwFamilies[PhpHwFamilySch5027].Capabilities = PhpHwSch5027Ended ? 0 : capabilities;

    return PhpHwSch5027Ended ? STATUS_NOT_SUPPORTED : STATUS_SUCCESS;
}

/**
 * Ends the session: the monitor is found again at the next.
 */
_Function_class_(PHP_HW_FAMILY_END)
VOID NTAPI PhpHwSch5027End(
    VOID
    )
{
    PhpHwSch5027Status = STATUS_NOT_SUPPORTED;
    PhpHwSch5027Found = FALSE;
    PhpHwSch5027Strikes = 0;
    PhpHwSch5027Ended = FALSE;
    PhpHwSch5027Probing = FALSE;
}

/**
 * Writes the SCH5027 source's section of the hardware report.
 *
 * \param Builder The report.
 */
_Function_class_(PHP_HW_FAMILY_DIAGNOSTICS)
VOID NTAPI PhpHwSch5027Diagnostics(
    _Inout_ PPH_STRING_BUILDER Builder
    )
{
    PCWSTR found = L"not found";
    PCWSTR ended = L"";

    if (PhpHwSch5027Found)
        found = L"found";
    if (PhpHwSch5027Ended)
        ended = L", ended";

    PhAppendFormatStringBuilder(
        Builder,
        L"  SCH5027: last read 0x%08lx, %s%s\n",
        PhpHwSch5027Status,
        found,
        ended
        );
}
