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

// Collector tsens: the Qualcomm TSENS die temperature sensors of the Snapdragon parts tsens.lua lists, one
// KphHwTsensRead of the enabled sensors every update. Each core's temperature is the hotter of its two sensors and the
// package's the hottest CPU sensor (after the cpu collector, which leaves the package the hottest thermal zone); the
// other sensors are named sensors.

#define PHP_HW_TSENS_ID_BASE 0x54530000 // named sensor ids of the controllers: base + controller * 16 + sensor
#define PHP_HW_TSENS_STRIKE_LIMIT 3 // failed reads in a row before the sensors end for the session
#define PHP_HW_TSENS_VALID 0x00200000 // TM_Sn_STATUS [21]
#define PHP_HW_TSENS_SENSORS_ENABLED(Control) (((Control) >> 3) & 0xFFFF) // SROT control [18:3]

typedef struct _PHP_HW_TSENS_CORE
{
    FLOAT Maximum;
    BOOLEAN Valid; // this pass
    BOOLEAN Listed; // a sensor of the core is listed: the collector owns the core's temperature
} PHP_HW_TSENS_CORE, *PPHP_HW_TSENS_CORE;

static KPH_HW_TSENS_IDENTITY PhpHwTsensIdentity;
static NTSTATUS PhpHwTsensIdentifyStatus = STATUS_NOT_SUPPORTED; // STATUS_NOT_SUPPORTED when not probed
static NTSTATUS PhpHwTsensStatus = STATUS_NOT_SUPPORTED; // the last read, for the report
static ULONG PhpHwTsensValues[KPH_HW_TSENS_CONTROLLERS * KPH_HW_TSENS_SENSORS]; // the last read, for the report
static ULONG PhpHwTsensNamedIndex[KPH_HW_TSENS_CONTROLLERS * KPH_HW_TSENS_SENSORS]; // ULONG_MAX when not named
static ULONG PhpHwTsensProcessor[KPH_HW_TSENS_CONTROLLERS * KPH_HW_TSENS_SENSORS]; // ULONG_MAX when not a core's
static ULONG PhpHwTsensCoreSensors = 0; // sensors of cores, for the report
static PPHP_HW_TSENS_CORE PhpHwTsensCores = NULL; // per processor, the hottest sensor of this pass; when listed
static UCHAR PhpHwTsensStrikes = 0;
static BOOLEAN PhpHwTsensEnded = FALSE;
static BOOLEAN PhpHwTsensListed = FALSE; // the identify succeeded
static BOOLEAN PhpHwTsensProbedThisUpdate = FALSE; // a failed probe is tried again from the next pass

/**
 * \return The display name of a sensor that is not a core's, or NULL for a role that is not listed.
 */
static PPH_STRING PhpHwTsensName(
    _In_ PKPH_HW_TSENS_SENSOR Sensor
    )
{
    switch (Sensor->Role)
    {
    case KphHwTsensRoleClusterTop:
        return PhFormatString(L"CPU cluster %u top", Sensor->Index);
    case KphHwTsensRoleClusterBottom:
        return PhFormatString(L"CPU cluster %u bottom", Sensor->Index);
    case KphHwTsensRoleAoss:
        return PhFormatString(L"Always-on subsystem %u", Sensor->Index);
    case KphHwTsensRoleMemory:
        return PhCreateString(L"Memory controller");
    case KphHwTsensRoleVideo:
        return PhCreateString(L"Video");
    case KphHwTsensRoleNpu:
        return PhFormatString(L"NPU %u", Sensor->Index);
    case KphHwTsensRoleGpu:
        return PhFormatString(L"GPU %u", Sensor->Index);
    case KphHwTsensRoleCamera:
        return PhFormatString(L"Camera %u", Sensor->Index);
    }

    return NULL;
}

/**
 * \return The flat index of a processor, or ULONG_MAX when no processor has the group and number.
 */
static ULONG PhpHwTsensFindProcessor(
    _In_ UCHAR Group,
    _In_ UCHAR Number
    )
{
    for (ULONG i = 0; i < PhpHwProcessorCount; i++)
    {
        if (PhpHwCpuSensors[i].ProcessorNumber.Group == Group && PhpHwCpuSensors[i].ProcessorNumber.Number == Number)
            return i;
    }

    return ULONG_MAX;
}

/**
 * Identifies the controllers and lists the enabled sensors: a core's goes to its processor, the rest are named.
 *
 * \return STATUS_SUCCESS, STATUS_NOT_SUPPORTED without a part to read, or a failed call's status.
 */
static NTSTATUS PhpHwTsensList(
    VOID
    )
{
    for (ULONG i = 0; i < RTL_NUMBER_OF(PhpHwTsensNamedIndex); i++)
    {
        PhpHwTsensNamedIndex[i] = ULONG_MAX;
        PhpHwTsensProcessor[i] = ULONG_MAX;
    }

    PhpHwTsensCoreSensors = 0;

    if (!PhpHwTsensCores)
        PhpHwTsensCores = PhAllocateZero(sizeof(PHP_HW_TSENS_CORE) * PhpHwProcessorCount);

    PhpHwTsensIdentifyStatus = KphHwTsensIdentify(&PhpHwTsensIdentity);

    if (!NT_SUCCESS(PhpHwTsensIdentifyStatus))
    {
        PhTraceInfo("Hardware sensors: TSENS not identified: 0x%lx", PhpHwTsensIdentifyStatus);
        return PhpHwTsensIdentifyStatus;
    }

    for (ULONG c = 0; c < PhpHwTsensIdentity.Count; c++)
    {
        PKPH_HW_TSENS_CONTROLLER controller = &PhpHwTsensIdentity.Controllers[c];
        ULONG enabled = PHP_HW_TSENS_SENSORS_ENABLED(controller->Control);

        if (!(controller->Control & 1))
            continue;

        for (ULONG s = 0; s < KPH_HW_TSENS_SENSORS; s++)
        {
            PKPH_HW_TSENS_SENSOR sensor = &controller->Sensors[s];
            ULONG index = c * KPH_HW_TSENS_SENSORS + s;
            PPH_STRING name;

            if (!(enabled & (1UL << s)) || sensor->Role == KphHwTsensRoleNone || sensor->Role >= KphHwTsensRoleMaximum)
                continue;

            if (sensor->Role == KphHwTsensRoleCpuTop || sensor->Role == KphHwTsensRoleCpuBottom)
            {
                if (sensor->Group != KPH_HW_TSENS_NO_PROCESSOR)
                {
                    PhpHwTsensProcessor[index] = PhpHwTsensFindProcessor(sensor->Group, sensor->Number);

                    if (PhpHwTsensProcessor[index] != ULONG_MAX)
                    {
                        PhpHwTsensCores[PhpHwTsensProcessor[index]].Listed = TRUE;
                        PhpHwTsensCoreSensors++;
                    }
                }

                continue;
            }

            if (!(name = PhpHwTsensName(sensor)))
                continue;

            PhpHwTsensNamedIndex[index] = PhpHwAddNamedSensor(PhHwNamedSensorPmtTemperature, 0, PHP_HW_TSENS_ID_BASE + index, name->Buffer);
            PhDereferenceObject(name);
        }
    }

    PhpHwTsensListed = TRUE;

    return STATUS_SUCCESS;
}

/**
 * Identifies the controllers once a session and lists their sensors; a call that fails for another reason than a
 * missing row is made again from the next pass.
 *
 * \return STATUS_SUCCESS, STATUS_NOT_SUPPORTED without a part to read, or the failed call's status.
 */
NTSTATUS PhpHwTsensProbe(
    VOID
    )
{
    NTSTATUS status;

    if (PhpHwQueryMachine() != IMAGE_FILE_MACHINE_ARM64 || !PhpHwProcessorCount)
        return STATUS_NOT_SUPPORTED;

    status = PhpHwTsensList();
    PhpHwTsensProbedThisUpdate = TRUE;

    return status == STATUS_NOT_SUPPORTED || PhpHwStatusNotShipped(status) ? status : STATUS_SUCCESS;
}

/**
 * Publishes the sensors from one read: TM_Sn_STATUS [11:0] is tenths of a degree, signed, and [21] says it is valid.
 * A core takes the hotter of its sensors; the package of the processors the hottest CPU sensor. Three failed reads in
 * a row end the sensors.
 *
 * \return STATUS_NOT_SUPPORTED once the sensors ended, else STATUS_SUCCESS.
 */
NTSTATUS PhpHwTsensPass(
    VOID
    )
{
    ULONG values[KPH_HW_TSENS_CONTROLLERS * KPH_HW_TSENS_SENSORS];
    ULONG capabilities = 0;
    BOOLEAN read;

    if (PhpHwTsensEnded)
        return STATUS_NOT_SUPPORTED;

    if (!PhpHwTsensListed)
    {
        NTSTATUS status = STATUS_RETRY;

        if (!PhpHwTsensProbedThisUpdate)
            status = PhpHwTsensList();

        PhpHwTsensProbedThisUpdate = FALSE;

        if (status == STATUS_NOT_SUPPORTED)
            return STATUS_NOT_SUPPORTED;

        if (!PhpHwTsensListed)
        {
            if (status != STATUS_RETRY && ++PhpHwTsensStrikes >= PHP_HW_TSENS_STRIKE_LIMIT)
                return STATUS_NOT_SUPPORTED;

            return STATUS_RETRY;
        }
    }

    PhpHwTsensProbedThisUpdate = FALSE;
    read = NT_SUCCESS(PhpHwTsensStatus = KphHwTsensRead(PhpHwTsensIdentity.Count, values));

    if (read)
    {
        PhpHwTsensStrikes = 0;
        memcpy(PhpHwTsensValues, values, PhpHwTsensIdentity.Count * KPH_HW_TSENS_SENSORS * sizeof(ULONG));
    }
    else if (++PhpHwTsensStrikes >= PHP_HW_TSENS_STRIKE_LIMIT)
    {
        PhTraceInfo("Hardware sensors: TSENS unavailable: 0x%lx", PhpHwTsensStatus);
        PhpHwTsensEnded = TRUE;
    }

    for (ULONG i = 0; i < PhpHwProcessorCount; i++)
        PhpHwTsensCores[i].Valid = FALSE;

    for (ULONG i = 0; i < PhpHwTsensIdentity.Count * KPH_HW_TSENS_SENSORS; i++)
    {
        ULONG value = read ? values[i] : 0;
        BOOLEAN valid = !!(value & PHP_HW_TSENS_VALID);
        LONG tenths = (LONG)(value & 0x7FF) - (LONG)(value & 0x800);
        FLOAT celsius = (FLOAT)tenths / 10.0f;

        if (PhpHwTsensNamedIndex[i] != ULONG_MAX)
            PhpHwSetNamedSensor(PhpHwTsensNamedIndex[i], valid, celsius);

        if (PhpHwTsensProcessor[i] < PhpHwProcessorCount)
        {
            PPHP_HW_TSENS_CORE core = &PhpHwTsensCores[PhpHwTsensProcessor[i]];

            capabilities |= PH_HW_CAP_CORE_TEMP | PH_HW_CAP_PACKAGE_TEMP;

            if (valid && (!core->Valid || celsius > core->Maximum))
            {
                core->Maximum = celsius;
                core->Valid = TRUE;
            }
        }
    }

    for (ULONG p = 0; p < PhpHwPackageCount; p++)
    {
        PPH_HW_PACKAGE_SENSOR package = &PhpHwPackageSensors[p];
        FLOAT packageMaximum = 0.0f;
        BOOLEAN packageValid = FALSE;

        for (ULONG i = 0; i < PhpHwProcessorCount; i++)
        {
            PPH_HW_CPU_SENSOR sensor = &PhpHwCpuSensors[i];
            PPHP_HW_TSENS_CORE core = &PhpHwTsensCores[i];
            ULONG coreValid = 0;

            if (sensor->PackageIndex != p || !core->Listed)
                continue;

            // The collector owns the temperature of a core it lists: set or cleared every pass.
            if (core->Valid)
                sensor->Temperature = core->Maximum;

            if (core->Valid)
                coreValid = PH_HW_CAP_CORE_TEMP;

            WriteULongRelease(&sensor->Valid, (ReadULongAcquire(&sensor->Valid) & ~PH_HW_CAP_CORE_TEMP) | coreValid);

            if (!core->Valid)
                continue;

            if (!packageValid || core->Maximum > packageMaximum)
                packageMaximum = core->Maximum;

            packageValid = TRUE;
        }

        // The cores' sensors replace the thermal zone the cpu collector left as the package's.
        if (packageValid)
        {
            package->Temperature = packageMaximum;
            WriteULongRelease(&package->Valid, ReadULongAcquire(&package->Valid) | PH_HW_CAP_PACKAGE_TEMP);
        }
        else if (PhpHwTsensCoreSensors)
        {
            // The cpu collector sets the zone again on its next pass, when it runs.
            WriteULongRelease(&package->Valid, ReadULongAcquire(&package->Valid) & ~PH_HW_CAP_PACKAGE_TEMP);
        }
    }

    PhpHwFamilies[PhpHwFamilyTsens].Capabilities = PhpHwTsensEnded ? 0 : capabilities;

    return PhpHwTsensEnded ? STATUS_NOT_SUPPORTED : STATUS_SUCCESS;
}

/**
 * Ends the session: the controllers are identified again at the next.
 */
VOID PhpHwTsensEnd(
    VOID
    )
{
    PhpHwTsensIdentifyStatus = STATUS_NOT_SUPPORTED;
    PhpHwTsensStatus = STATUS_NOT_SUPPORTED;
    PhpHwTsensCoreSensors = 0;
    PhpHwTsensStrikes = 0;
    PhpHwTsensEnded = FALSE;
    PhpHwTsensListed = FALSE;
    PhpHwTsensProbedThisUpdate = FALSE;
    memset(&PhpHwTsensIdentity, 0, sizeof(PhpHwTsensIdentity));
    memset(PhpHwTsensValues, 0, sizeof(PhpHwTsensValues));

    if (PhpHwTsensCores)
    {
        PhFree(PhpHwTsensCores);
        PhpHwTsensCores = NULL;
    }
}

/**
 * Writes the collector's section of the hardware report: the identify, each controller's version, control and last
 * read status registers.
 *
 * \param Builder The report.
 */
VOID PhpHwTsensDiagnostics(
    _Inout_ PPH_STRING_BUILDER Builder
    )
{
    PCWSTR ended = L"";

    if (PhpHwTsensIdentifyStatus == STATUS_NOT_SUPPORTED)
    {
        PhAppendStringBuilder2(Builder, L"  Controllers: not identified\n");
        return;
    }

    if (PhpHwTsensEnded)
        ended = L", ended";

    PhAppendFormatStringBuilder(
        Builder,
        L"  Controllers: identify 0x%08lx, %lu controllers, %lu core sensors; last read 0x%08lx%s\n",
        PhpHwTsensIdentifyStatus,
        PhpHwTsensIdentity.Count,
        PhpHwTsensCoreSensors,
        PhpHwTsensStatus,
        ended
        );

    for (ULONG c = 0; c < PhpHwTsensIdentity.Count && c < KPH_HW_TSENS_CONTROLLERS; c++)
    {
        PhAppendFormatStringBuilder(
            Builder,
            L"  tsens%lu: version 0x%08lx control 0x%08lx; status",
            c,
            PhpHwTsensIdentity.Controllers[c].Version,
            PhpHwTsensIdentity.Controllers[c].Control
            );

        for (ULONG s = 0; s < KPH_HW_TSENS_SENSORS; s++)
            PhAppendFormatStringBuilder(Builder, L" %08lx", PhpHwTsensValues[c * KPH_HW_TSENS_SENSORS + s]);

        PhAppendStringBuilder2(Builder, L"\n");
    }
}
