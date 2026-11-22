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

ULONG PhpHwProcessorCount = 0;
ULONG PhpHwPackageCount = 0;
PPH_HW_CPU_SENSOR PhpHwCpuSensors = NULL;
PPH_HW_PACKAGE_SENSOR PhpHwPackageSensors = NULL;

BOOLEAN PhpHwGateOpen = FALSE;
BOOLEAN PhpHwProbed = FALSE;
BOOLEAN PhpHwAvailable = FALSE;
ULONG PhpHwCapabilities = 0;

BOOLEAN PhpHwChipsetTemperatureValid = FALSE;
FLOAT PhpHwChipsetTemperature = 0.0f;

PH_QUEUED_LOCK PhpHwNamedSensorLock = PH_QUEUED_LOCK_INIT;
PPH_HW_NAMED_SENSOR PhpHwNamedSensors = NULL;
ULONG PhpHwNamedSensorCount = 0;

FLOAT PhHwCpuTemperature = 0.0f;
FLOAT PhHwCpuPower = 0.0f;

/**
 * Copies the active named sensors under one hold of the lock.
 *
 * \param Count Receives the number of sensors copied.
 * \return The sensors, freed with PhFree, or NULL when there are none.
 */
_Ret_maybenull_
PPH_HW_NAMED_SENSOR PhHwCopyNamedSensors(
    _Out_ PULONG Count
    )
{
    PPH_HW_NAMED_SENSOR copy = NULL;
    ULONG copied = 0;

    PhAcquireQueuedLockShared(&PhpHwNamedSensorLock);

    if (PhpHwAvailable && PhpHwNamedSensorCount)
    {
        copy = PhAllocate(sizeof(PH_HW_NAMED_SENSOR) * PhpHwNamedSensorCount);

        for (ULONG i = 0; i < PhpHwNamedSensorCount; i++)
        {
            if (PhpHwNamedSensors[i].Active)
                copy[copied++] = PhpHwNamedSensors[i];
        }
    }

    PhReleaseQueuedLockShared(&PhpHwNamedSensorLock);

    if (copy && !copied)
    {
        PhFree(copy);
        copy = NULL;
    }

    *Count = copied;

    return copy;
}

/**
 * Retrieves the value of a named sensor.
 *
 * \param Kind The kind of the sensor.
 * \param PackageIndex The component of the sensor.
 * \param Id The ID of the sensor within the kind and component.
 * \param Value Receives the value.
 * \return TRUE when the sensor is active and its value is current.
 */
_Success_(return)
BOOLEAN PhHwGetNamedSensorValue(
    _In_ PH_HW_NAMED_SENSOR_KIND Kind,
    _In_ ULONG PackageIndex,
    _In_ ULONG Id,
    _Out_ PFLOAT Value
    )
{
    BOOLEAN found = FALSE;

    PhAcquireQueuedLockShared(&PhpHwNamedSensorLock);

    for (ULONG i = 0; PhpHwAvailable && i < PhpHwNamedSensorCount; i++)
    {
        if (
            PhpHwNamedSensors[i].Active &&
            PhpHwNamedSensors[i].Valid &&
            PhpHwNamedSensors[i].Kind == Kind &&
            PhpHwNamedSensors[i].PackageIndex == PackageIndex &&
            PhpHwNamedSensors[i].Id == Id
            )
        {
            *Value = PhpHwNamedSensors[i].Value;
            found = TRUE;
            break;
        }
    }

    PhReleaseQueuedLockShared(&PhpHwNamedSensorLock);

    return found;
}

/**
 * Retrieves the chipset (PCH) temperature of the last update.
 *
 * \param Celsius Receives the temperature.
 * \return TRUE when the temperature is current.
 */
_Success_(return)
BOOLEAN NTAPI PhHwGetChipsetTemperature(
    _Out_ PFLOAT Celsius
    )
{
    if (!PhpHwAvailable || !PhpHwChipsetTemperatureValid)
        return FALSE;

    *Celsius = PhpHwChipsetTemperature;
    return TRUE;
}

/**
 * Determines whether hardware sensors may be read.
 *
 * \return TRUE when the gate is open.
 */
BOOLEAN NTAPI PhHwSensorsEnabled(
    VOID
    )
{
    return PhpHwGateOpen;
}

/**
 * Retrieves the capabilities at least one processor package has a current value for.
 *
 * \return PH_HW_CAP_* flags of the package sensors, or 0 when hardware sensors are unavailable.
 */
ULONG PhHwGetPackageValidMask(
    VOID
    )
{
    ULONG valid = 0;

    if (!PhHwSensorsAvailable())
        return 0;

    for (ULONG i = 0; i < PhHwGetPackageCount(); i++)
        valid |= PhpHwPackageSensors[i].Valid;

    return valid;
}

/**
 * Determines whether every driver family has finished its probe in this session.
 */
BOOLEAN PhHwSensorsProbed(
    VOID
    )
{
    return PhpHwGateOpen && PhpHwProbed;
}

/**
 * Determines whether hardware sensors are available.
 *
 * \return TRUE when the gate is open and a driver family has published a sensor in this session.
 */
BOOLEAN NTAPI PhHwSensorsAvailable(
    VOID
    )
{
    return PhpHwGateOpen && PhpHwAvailable;
}

/**
 * Retrieves the available hardware sensor capabilities.
 *
 * \return PH_HW_CAP_* flags, or 0 when hardware sensors are unavailable.
 */
ULONG NTAPI PhHwGetCapabilities(
    VOID
    )
{
    if (!PhHwSensorsAvailable())
        return 0;

    return PhpHwCapabilities;
}

/**
 * Retrieves the sensor of a logical processor.
 *
 * \param Index The flat processor index, as used by PhCpusKernelUsage.
 * \return The sensor, or NULL when the index is out of range or the provider has no session.
 */
PPH_HW_CPU_SENSOR NTAPI PhHwGetCpuSensor(
    _In_ ULONG Index
    )
{
    if (!PhpHwCpuSensors || Index >= PhpHwProcessorCount)
        return NULL;

    return &PhpHwCpuSensors[Index];
}

/**
 * Retrieves the sensor of a processor package.
 *
 * \param Index The package index.
 * \return The sensor, or NULL when the index is out of range or the provider has no session.
 */
PPH_HW_PACKAGE_SENSOR NTAPI PhHwGetPackageSensor(
    _In_ ULONG Index
    )
{
    if (!PhpHwPackageSensors || Index >= PhpHwPackageCount)
        return NULL;

    return &PhpHwPackageSensors[Index];
}

/**
 * Retrieves the number of processor packages.
 *
 * \return The number of packages, or 0 when the provider has no session.
 */
ULONG NTAPI PhHwGetPackageCount(
    VOID
    )
{
    return PhpHwPackageSensors ? PhpHwPackageCount : 0;
}

/**
 * Retrieves the upper bound of CPU temperature graphs.
 *
 * \return The highest package TjMax, or 100 degrees Celsius when no package reports one.
 */
FLOAT NTAPI PhHwGetCpuTemperatureLimit(
    VOID
    )
{
    FLOAT limit = 0.0f;

    for (ULONG i = 0; i < PhHwGetPackageCount(); i++)
    {
        if (PhpHwPackageSensors[i].TjMax > limit)
            limit = PhpHwPackageSensors[i].TjMax;
    }

    return limit > 0.0f ? limit : 100.0f;
}

/**
 * Determines whether the Windows performance counters have published a sensor.
 *
 * \return TRUE when at least one counter sensor exists.
 */
BOOLEAN NTAPI PhHwCounterSensorsPresent(
    VOID
    )
{
    return FALSE;
}
