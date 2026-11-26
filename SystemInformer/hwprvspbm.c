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

// Collector spbm: the NVIDIA N1X power budget page, one KphHwSpbmRead every update. A power with an energy count is
// the count's rate over the update.

#define PHP_HW_SPBM_ID_BASE 0x53500000 // named sensor ids: base + kind * 0x100 + channel
#define PHP_HW_SPBM_STRIKE_LIMIT 3 // failed reads in a row before the sensors end for the session
#define PHP_HW_SPBM_ENERGY_MAX_WATTS 10000.0f // above this an energy count restarted
#define PHP_HW_SPBM_MAX_INTERVAL 10.0f // seconds; a longer interval is a gap
#define PHP_HW_SPBM_PACKAGE_CAPS (PH_HW_CAP_PACKAGE_TEMP | PH_HW_CAP_RAPL_PACKAGE | PH_HW_CAP_RAPL_CORE | \
    PH_HW_CAP_RAPL_PSYS | PH_HW_CAP_POWER_LIMIT1 | PH_HW_CAP_POWER_LIMIT2) // package bits this collector owns

typedef struct _PHP_HW_SPBM_NAMED
{
    PH_HW_NAMED_SENSOR_KIND Kind;
    ULONG Channel;
    PCWSTR Name;
} PHP_HW_SPBM_NAMED, *PPHP_HW_SPBM_NAMED;
typedef CONST PHP_HW_SPBM_NAMED *PCPHP_HW_SPBM_NAMED;

static CONST PHP_HW_SPBM_NAMED PhpHwSpbmTemperatures[] =
{
    { PhHwNamedSensorPmtTemperature, KphHwSpbmTemperatureSoc, L"SoC" },
    { PhHwNamedSensorPmtTemperature, KphHwSpbmTemperatureCluster0E, L"CPU cluster 0 E-cores" },
    { PhHwNamedSensorPmtTemperature, KphHwSpbmTemperatureCluster0P, L"CPU cluster 0 P-cores" },
    { PhHwNamedSensorPmtTemperature, KphHwSpbmTemperatureCluster1E, L"CPU cluster 1 E-cores" },
    { PhHwNamedSensorPmtTemperature, KphHwSpbmTemperatureCluster1P, L"CPU cluster 1 P-cores" },
    { PhHwNamedSensorPmtTemperature, KphHwSpbmTemperatureGpu, L"GPU" },
    { PhHwNamedSensorPmtTemperature, KphHwSpbmTemperatureUncore, L"Uncore" },
    { PhHwNamedSensorPmtTemperature, KphHwSpbmTemperatureDla, L"DLA" },
};

static CONST PHP_HW_SPBM_NAMED PhpHwSpbmPowers[] =
{
    { PhHwNamedSensorRaplPower, KphHwSpbmPowerCpuP, L"CPU P-cores" },
    { PhHwNamedSensorRaplPower, KphHwSpbmPowerCpuE, L"CPU E-cores" },
    { PhHwNamedSensorRaplPower, KphHwSpbmPowerGpu, L"GPU" },
    { PhHwNamedSensorRaplPower, KphHwSpbmPowerCpuGpu, L"CPU + GPU" },
    { PhHwNamedSensorRaplPower, KphHwSpbmPowerMemoryA, L"Memory rail A" },
    { PhHwNamedSensorRaplPower, KphHwSpbmPowerMemoryB, L"Memory rail B" },
    { PhHwNamedSensorRaplPower, KphHwSpbmPowerMemoryC, L"Memory rail C" },
    { PhHwNamedSensorRaplPower, KphHwSpbmPowerDcInput, L"DC input" },
    { PhHwNamedSensorRaplPower, KphHwSpbmPowerPackageAverage, L"Package average" },
    { PhHwNamedSensorRaplPower, KphHwSpbmPowerSystemAverage, L"System average" },
    { PhHwNamedSensorRaplPower, KphHwSpbmPowerSystemShortAverage, L"System short average" },
};

static CONST PHP_HW_SPBM_NAMED PhpHwSpbmVoltages[] =
{
    { PhHwNamedSensorSmuVoltage, KphHwSpbmVoltageCpuP, L"CPU P-cluster" },
    { PhHwNamedSensorSmuVoltage, KphHwSpbmVoltageCpuE, L"CPU E-cluster" },
    { PhHwNamedSensorSmuVoltage, KphHwSpbmVoltageMemoryC, L"Memory rail C" },
};

static CONST PHP_HW_SPBM_NAMED PhpHwSpbmLimits[] =
{
    { PhHwNamedSensorPowerLimit, KphHwSpbmLimitPl1Request, L"PL1 (requested)" },
    { PhHwNamedSensorPowerLimit, KphHwSpbmLimitPl2Request, L"PL2 (requested)" },
    { PhHwNamedSensorPowerLimit, KphHwSpbmLimitInForce, L"Limit in force" },
};

// The energy count of a power channel, KphHwSpbmEnergyMaximum when it has none.
static CONST ULONG PhpHwSpbmPowerEnergy[KphHwSpbmPowerMaximum] =
{
    KphHwSpbmEnergySystem, // System
    KphHwSpbmEnergyPackage, // Package
    KphHwSpbmEnergyMaximum, // CpuGpu
    KphHwSpbmEnergyCpuP, // CpuP
    KphHwSpbmEnergyCpuE, // CpuE
    KphHwSpbmEnergyGpu, // Gpu
    KphHwSpbmEnergyMaximum, // MemoryA
    KphHwSpbmEnergyMaximum, // MemoryB
    KphHwSpbmEnergyMaximum, // MemoryC
    KphHwSpbmEnergyMaximum, // DcInput
    KphHwSpbmEnergyMaximum, // PackageAverage
    KphHwSpbmEnergyMaximum, // SystemAverage
    KphHwSpbmEnergyMaximum, // SystemShortAverage
};

static KPH_HW_SPBM_SAMPLE PhpHwSpbmSample; // the last read, for the report
static KPH_HW_SPBM_SAMPLE PhpHwSpbmLastSample; // the last read that succeeded
static ULONG64 PhpHwSpbmLastTime = 0; // of PhpHwSpbmLastSample; 0 without one
static NTSTATUS PhpHwSpbmStatus = STATUS_NOT_SUPPORTED; // the last read; STATUS_NOT_SUPPORTED when not probed
static ULONG PhpHwSpbmTemperatureIndex[RTL_NUMBER_OF(PhpHwSpbmTemperatures)];
static ULONG PhpHwSpbmPowerIndex[RTL_NUMBER_OF(PhpHwSpbmPowers)];
static ULONG PhpHwSpbmLimitIndex[RTL_NUMBER_OF(PhpHwSpbmLimits)];
static ULONG PhpHwSpbmVoltageIndex[RTL_NUMBER_OF(PhpHwSpbmVoltages)];
static UCHAR PhpHwSpbmStrikes = 0;
static BOOLEAN PhpHwSpbmEnded = FALSE;
static BOOLEAN PhpHwSpbmListed = FALSE; // the first read succeeded and the named sensors are listed
static BOOLEAN PhpHwSpbmProbedThisUpdate = FALSE; // a failed probe is tried again from the next pass

/**
 * Forgets the named sensors' indexes.
 */
static VOID PhpHwSpbmClearIndexes(
    VOID
    )
{
    for (ULONG i = 0; i < RTL_NUMBER_OF(PhpHwSpbmTemperatureIndex); i++)
        PhpHwSpbmTemperatureIndex[i] = ULONG_MAX;
    for (ULONG i = 0; i < RTL_NUMBER_OF(PhpHwSpbmPowerIndex); i++)
        PhpHwSpbmPowerIndex[i] = ULONG_MAX;
    for (ULONG i = 0; i < RTL_NUMBER_OF(PhpHwSpbmLimitIndex); i++)
        PhpHwSpbmLimitIndex[i] = ULONG_MAX;
    for (ULONG i = 0; i < RTL_NUMBER_OF(PhpHwSpbmVoltageIndex); i++)
        PhpHwSpbmVoltageIndex[i] = ULONG_MAX;
}

/**
 * Adds the named sensors of the channels the part has.
 *
 * \param Named The sensors.
 * \param Count The number of sensors.
 * \param Mask The part's channels of their kind.
 * \param Index Receives each sensor's index, ULONG_MAX for a channel the part does not have.
 */
static VOID PhpHwSpbmAddNamed(
    _In_reads_(Count) PCPHP_HW_SPBM_NAMED Named,
    _In_ ULONG Count,
    _In_ ULONG Mask,
    _Out_writes_(Count) PULONG Index
    )
{
    for (ULONG i = 0; i < Count; i++)
    {
        Index[i] = ULONG_MAX;

        if (!(Mask & (1UL << Named[i].Channel)))
            continue;

        Index[i] = PhpHwAddNamedSensor(Named[i].Kind, 0, PHP_HW_SPBM_ID_BASE + (ULONG)Named[i].Kind * 0x100 + Named[i].Channel, Named[i].Name);
    }
}

/**
 * Reads the page and, on the first read that succeeds, lists the named sensors of the part's channels.
 *
 * \return STATUS_SUCCESS, STATUS_NOT_SUPPORTED without a part to read, or the failed read's status.
 */
static NTSTATUS PhpHwSpbmRead(
    VOID
    )
{
    PhpHwSpbmStatus = KphHwSpbmRead(&PhpHwSpbmSample);

    if (!NT_SUCCESS(PhpHwSpbmStatus))
    {
        PhTraceInfo("Hardware sensors: SPBM not read: 0x%lx", PhpHwSpbmStatus);
        return PhpHwSpbmStatus;
    }

    if (!PhpHwSpbmListed)
    {
        PhpHwSpbmAddNamed(PhpHwSpbmTemperatures, RTL_NUMBER_OF(PhpHwSpbmTemperatures), PhpHwSpbmSample.TemperatureMask, PhpHwSpbmTemperatureIndex);
        PhpHwSpbmAddNamed(PhpHwSpbmPowers, RTL_NUMBER_OF(PhpHwSpbmPowers), PhpHwSpbmSample.PowerMask, PhpHwSpbmPowerIndex);
        PhpHwSpbmAddNamed(PhpHwSpbmLimits, RTL_NUMBER_OF(PhpHwSpbmLimits), PhpHwSpbmSample.LimitMask, PhpHwSpbmLimitIndex);
        PhpHwSpbmAddNamed(PhpHwSpbmVoltages, RTL_NUMBER_OF(PhpHwSpbmVoltages), PhpHwSpbmSample.VoltageMask, PhpHwSpbmVoltageIndex);
        PhpHwSpbmListed = TRUE;
    }

    return STATUS_SUCCESS;
}

/**
 * Reads the page once a session to see whether the part has a row; a read that fails for another reason than a
 * missing row is made again from the next pass.
 *
 * \return STATUS_SUCCESS, STATUS_NOT_SUPPORTED without a part to read, or the failed read's status.
 */
NTSTATUS PhpHwSpbmProbe(
    VOID
    )
{
    NTSTATUS status;

    PhpHwSpbmClearIndexes();

    if (PhpHwQueryMachine() != IMAGE_FILE_MACHINE_ARM64 || !PhpHwPackageCount)
        return STATUS_NOT_SUPPORTED;

    status = PhpHwSpbmRead();
    PhpHwSpbmProbedThisUpdate = TRUE;

    if (status == STATUS_NOT_SUPPORTED || PhpHwStatusNotShipped(status))
        return status;

    return STATUS_SUCCESS;
}

/**
 * The power of a channel in watts: its energy count's rate since the last sample, else its instant word.
 *
 * \param Channel The power channel.
 * \param Seconds The time since the last sample, 0 without one.
 * \param Watts Receives the power.
 * \return TRUE when the part has the channel.
 */
_Success_(return)
static BOOLEAN PhpHwSpbmPower(
    _In_ ULONG Channel,
    _In_ FLOAT Seconds,
    _Out_ PFLOAT Watts
    )
{
    ULONG energy = PhpHwSpbmPowerEnergy[Channel];

    if (!(PhpHwSpbmSample.PowerMask & (1UL << Channel)))
        return FALSE;

    if (energy < KphHwSpbmEnergyMaximum && (PhpHwSpbmSample.EnergyMask & (1UL << energy)) && Seconds > 0.0f && Seconds <= PHP_HW_SPBM_MAX_INTERVAL)
    {
        ULONG millijoules = PhpHwSpbmSample.Energy[energy] - PhpHwSpbmLastSample.Energy[energy];
        FLOAT watts = (FLOAT)millijoules / 1000.0f / Seconds;

        if (watts <= PHP_HW_SPBM_ENERGY_MAX_WATTS)
        {
            *Watts = watts;
            return TRUE;
        }
    }

    *Watts = (FLOAT)PhpHwSpbmSample.Power[Channel] / 1000.0f;
    return TRUE;
}

/**
 * Publishes one read. Three failed reads in a row end the sensors.
 *
 * \return STATUS_NOT_SUPPORTED once the sensors ended, else STATUS_SUCCESS.
 */
NTSTATUS PhpHwSpbmPass(
    VOID
    )
{
    PPH_HW_PACKAGE_SENSOR package = &PhpHwPackageSensors[0];
    ULONG valid = 0;
    ULONG64 now;
    FLOAT seconds = 0.0f;
    FLOAT watts;
    FLOAT pWatts;
    FLOAT eWatts;
    BOOLEAN read;

    if (PhpHwSpbmEnded)
        return STATUS_NOT_SUPPORTED;

    if (PhpHwSpbmProbedThisUpdate)
    {
        read = NT_SUCCESS(PhpHwSpbmStatus);
        PhpHwSpbmProbedThisUpdate = FALSE;
    }
    else
    {
        NTSTATUS status = PhpHwSpbmRead();

        if (status == STATUS_NOT_SUPPORTED)
            return STATUS_NOT_SUPPORTED;

        read = NT_SUCCESS(status);
    }

    now = PhpHwQueryTime();

    if (read)
    {
        PhpHwSpbmStrikes = 0;

        if (PhpHwSpbmLastTime && now > PhpHwSpbmLastTime)
            seconds = (FLOAT)(now - PhpHwSpbmLastTime) / (FLOAT)PHP_HW_SECOND;
    }
    else if (++PhpHwSpbmStrikes >= PHP_HW_SPBM_STRIKE_LIMIT)
    {
        PhTraceInfo("Hardware sensors: SPBM unavailable: 0x%lx", PhpHwSpbmStatus);
        PhpHwSpbmEnded = TRUE;
    }

    for (ULONG i = 0; i < RTL_NUMBER_OF(PhpHwSpbmTemperatures); i++)
    {
        LONG milli = PhpHwSpbmSample.Temperature[PhpHwSpbmTemperatures[i].Channel];

        // A word the firmware never wrote reads 0 K.
        PhpHwSetNamedSensor(PhpHwSpbmTemperatureIndex[i], read && milli > -273000, (FLOAT)milli / 1000.0f);
    }

    for (ULONG i = 0; i < RTL_NUMBER_OF(PhpHwSpbmPowers); i++)
    {
        BOOLEAN have = FALSE;

        watts = 0.0f;

        if (read)
            have = PhpHwSpbmPower(PhpHwSpbmPowers[i].Channel, seconds, &watts);

        PhpHwSetNamedSensor(PhpHwSpbmPowerIndex[i], have, watts);
    }

    for (ULONG i = 0; i < RTL_NUMBER_OF(PhpHwSpbmLimits); i++)
    {
        ULONG milliwatts = PhpHwSpbmSample.Limit[PhpHwSpbmLimits[i].Channel];

        PhpHwSetNamedSensor(PhpHwSpbmLimitIndex[i], read && milliwatts, (FLOAT)milliwatts / 1000.0f);
    }

    for (ULONG i = 0; i < RTL_NUMBER_OF(PhpHwSpbmVoltages); i++)
    {
        PhpHwSetNamedSensor(PhpHwSpbmVoltageIndex[i], read, (FLOAT)PhpHwSpbmSample.Voltage[PhpHwSpbmVoltages[i].Channel] / 1000.0f);
    }

    if (read)
    {
        LONG hottest = PhpHwSpbmSample.Temperature[KphHwSpbmTemperatureHottest];

        if ((PhpHwSpbmSample.TemperatureMask & (1UL << KphHwSpbmTemperatureHottest)) && hottest > -273000)
        {
            package->Temperature = (FLOAT)hottest / 1000.0f;
            valid |= PH_HW_CAP_PACKAGE_TEMP;
        }

        if (PhpHwSpbmPower(KphHwSpbmPowerPackage, seconds, &watts))
        {
            package->PackagePower = watts;
            valid |= PH_HW_CAP_RAPL_PACKAGE;
        }

        if (PhpHwSpbmPower(KphHwSpbmPowerCpuP, seconds, &pWatts) && PhpHwSpbmPower(KphHwSpbmPowerCpuE, seconds, &eWatts))
        {
            package->CorePower = pWatts + eWatts;
            valid |= PH_HW_CAP_RAPL_CORE;
        }

        if (PhpHwSpbmPower(KphHwSpbmPowerSystem, seconds, &watts))
        {
            package->PlatformPower = watts;
            valid |= PH_HW_CAP_RAPL_PSYS;
        }

        if (PhpHwSpbmSample.LimitMask & (1UL << KphHwSpbmLimitPl1))
        {
            package->ActivePowerLimit1 = (FLOAT)PhpHwSpbmSample.Limit[KphHwSpbmLimitPl1] / 1000.0f;
            valid |= PH_HW_CAP_POWER_LIMIT1;
        }

        if (PhpHwSpbmSample.LimitMask & (1UL << KphHwSpbmLimitPl2))
        {
            package->ActivePowerLimit2 = (FLOAT)PhpHwSpbmSample.Limit[KphHwSpbmLimitPl2] / 1000.0f;
            valid |= PH_HW_CAP_POWER_LIMIT2;
        }

        PhpHwSpbmLastSample = PhpHwSpbmSample;
        PhpHwSpbmLastTime = now;
    }

    // The collector owns these bits of the package: set or cleared every pass.
    WriteULongRelease(&package->Valid, (ReadULongAcquire(&package->Valid) & ~PHP_HW_SPBM_PACKAGE_CAPS) | valid);
    PhpHwFamilies[PhpHwFamilySpbm].Capabilities = PhpHwSpbmEnded ? 0 : PHP_HW_SPBM_PACKAGE_CAPS;

    return PhpHwSpbmEnded ? STATUS_NOT_SUPPORTED : STATUS_SUCCESS;
}

/**
 * Ends the session: the page is read and the sensors listed again at the next.
 */
VOID PhpHwSpbmEnd(
    VOID
    )
{
    PhpHwSpbmStatus = STATUS_NOT_SUPPORTED;
    PhpHwSpbmStrikes = 0;
    PhpHwSpbmEnded = FALSE;
    PhpHwSpbmListed = FALSE;
    PhpHwSpbmProbedThisUpdate = FALSE;
    PhpHwSpbmLastTime = 0;
    PhpHwSpbmClearIndexes();
    memset(&PhpHwSpbmSample, 0, sizeof(PhpHwSpbmSample));
    memset(&PhpHwSpbmLastSample, 0, sizeof(PhpHwSpbmLastSample));
}

/**
 * Writes the collector's section of the hardware report: the last read's status, the part's channels and the raw
 * values.
 *
 * \param Builder The report.
 */
VOID PhpHwSpbmDiagnostics(
    _Inout_ PPH_STRING_BUILDER Builder
    )
{
    if (PhpHwSpbmStatus == STATUS_NOT_SUPPORTED)
    {
        PhAppendStringBuilder2(Builder, L"  Page: not read\n");
        return;
    }

    PhAppendFormatStringBuilder(
        Builder,
        L"  Page: last read 0x%08lx%s; channels: temperatures 0x%lx, powers 0x%lx, energies 0x%lx, limits 0x%lx, "
        L"voltages 0x%lx\n",
        PhpHwSpbmStatus,
        PhpHwSpbmEnded ? L", ended" : L"",
        PhpHwSpbmSample.TemperatureMask,
        PhpHwSpbmSample.PowerMask,
        PhpHwSpbmSample.EnergyMask,
        PhpHwSpbmSample.LimitMask,
        PhpHwSpbmSample.VoltageMask
        );

    PhAppendStringBuilder2(Builder, L"  Temperatures (milli-C):");
    for (ULONG i = 0; i < KphHwSpbmTemperatureMaximum; i++)
        PhAppendFormatStringBuilder(Builder, L" %ld", PhpHwSpbmSample.Temperature[i]);

    PhAppendStringBuilder2(Builder, L"\n  Powers (mW):");
    for (ULONG i = 0; i < KphHwSpbmPowerMaximum; i++)
        PhAppendFormatStringBuilder(Builder, L" %lu", PhpHwSpbmSample.Power[i]);

    PhAppendStringBuilder2(Builder, L"\n  Energies (mJ):");
    for (ULONG i = 0; i < KphHwSpbmEnergyMaximum; i++)
        PhAppendFormatStringBuilder(Builder, L" %lu", PhpHwSpbmSample.Energy[i]);

    PhAppendStringBuilder2(Builder, L"\n  Limits (mW):");
    for (ULONG i = 0; i < KphHwSpbmLimitMaximum; i++)
        PhAppendFormatStringBuilder(Builder, L" %lu", PhpHwSpbmSample.Limit[i]);

    PhAppendStringBuilder2(Builder, L"\n  Voltages (mV):");
    for (ULONG i = 0; i < KphHwSpbmVoltageMaximum; i++)
        PhAppendFormatStringBuilder(Builder, L" %lu", PhpHwSpbmSample.Voltage[i]);

    PhAppendStringBuilder2(Builder, L"\n");
}
