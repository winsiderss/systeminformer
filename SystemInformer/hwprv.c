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
#include <phplug.h>

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

PH_PROVIDER_REGISTRATION PhHwProviderRegistration;

PHP_HW_FAMILY PhpHwFamilies[PhpHwFamilyMaximum] =
{
    { L"cpu", 0 },
    { L"smn", 0 },
    { L"mmio", 0 },
    { L"platform", 0 },
    { L"pciconfig", 0 },
    { L"vsec", 0 },
    { L"crosec", 0 },
    { L"smu", PHP_HW_SECOND },
    { L"hsmp", PHP_HW_SECOND },
    { L"smbus", 3 * PHP_HW_SECOND },
    { L"acpi", 2 * PHP_HW_SECOND },
};

static ULONG PhpHwRunCount = 0;
static BOOLEAN PhpHwTopologyQueried = FALSE;

/**
 * Determines whether a status says the family's script is not shipped with this build.
 *
 * \param Status A status a helper returned.
 * \return TRUE for STATUS_OBJECT_NAME_NOT_FOUND, STATUS_OBJECT_PATH_NOT_FOUND and STATUS_NO_SUCH_FILE.
 */
BOOLEAN PhpHwStatusNotShipped(
    _In_ NTSTATUS Status
    )
{
    return Status == STATUS_OBJECT_NAME_NOT_FOUND || Status == STATUS_OBJECT_PATH_NOT_FOUND || Status == STATUS_NO_SUCH_FILE;
}

/**
 * Initializes the hardware provider.
 */
VOID PhHwProviderInitialization(
    VOID
    )
{
    for (ULONG i = 0; i < PhpHwFamilyMaximum; i++)
    {
        PhpHwFamilies[i].Absent = FALSE;
        PhpHwFamilies[i].Ran = FALSE;
        PhpHwFamilies[i].RetryStreak = 0;
        PhpHwFamilies[i].BackoffUpdates = 0;
    }
}

/**
 * Allocates the per-processor and per-package arrays once; the topology does not change while the process runs.
 */
static VOID PhpHwInitializeTopology(
    VOID
    )
{
    PHP_HW_TOPOLOGY topology;

    if (PhpHwTopologyQueried)
        return;

    PhpHwTopologyQueried = TRUE;

    if (!PhpHwQueryTopology(&topology))
        return;

    PhpHwCpuSensors = PhAllocateZero(sizeof(PH_HW_CPU_SENSOR) * topology.ProcessorCount);
    PhpHwPackageSensors = PhAllocateZero(sizeof(PH_HW_PACKAGE_SENSOR) * topology.PackageCount);

    for (ULONG i = 0; i < topology.ProcessorCount; i++)
    {
        PhpHwCpuSensors[i].ProcessorNumber = topology.Processors[i].Number;
        PhpHwCpuSensors[i].CoreIndex = topology.Processors[i].CoreIndex;
        PhpHwCpuSensors[i].PackageIndex = topology.Processors[i].PackageIndex;
    }

    PhpHwPackageCount = topology.PackageCount;
    PhpHwProcessorCount = topology.ProcessorCount;

    PhpHwFreeTopology(&topology);
}

/**
 * Starts a session: the gate has opened. Every family probes.
 */
static VOID PhpHwBeginSession(
    VOID
    )
{
    PhpHwInitializeTopology();
    PhpHwGateOpen = TRUE;

    for (ULONG i = 0; i < PhpHwFamilyMaximum; i++)
    {
        PPHP_HW_FAMILY family = &PhpHwFamilies[i];

        family->Absent = FALSE;
        family->Ran = FALSE;
        family->RetryStreak = 0;
        family->BackoffUpdates = 0;

        if (family->Probe && family->Probe() == STATUS_NOT_SUPPORTED)
            family->Absent = TRUE;
    }

    PhpHwProbed = TRUE;
}

/**
 * Ends a session: the gate has closed. Every family forgets what it learned and nothing stays published.
 */
static VOID PhpHwEndSession(
    VOID
    )
{
    for (ULONG i = 0; i < PhpHwFamilyMaximum; i++)
    {
        if (PhpHwFamilies[i].End)
            PhpHwFamilies[i].End();
    }

    PhpHwGateOpen = FALSE;
    PhpHwProbed = FALSE;
    PhpHwAvailable = FALSE;
    PhpHwCapabilities = 0;
    PhpHwChipsetTemperatureValid = FALSE;
    PhHwCpuTemperature = 0.0f;
    PhHwCpuPower = 0.0f;

    for (ULONG i = 0; i < PhpHwProcessorCount; i++)
        PhpHwCpuSensors[i].Valid = 0;
    for (ULONG i = 0; i < PhpHwPackageCount; i++)
        PhpHwPackageSensors[i].Valid = 0;

    PhAcquireQueuedLockExclusive(&PhpHwNamedSensorLock);

    if (PhpHwNamedSensors)
    {
        PhFree(PhpHwNamedSensors);
        PhpHwNamedSensors = NULL;
    }

    PhpHwNamedSensorCount = 0;

    PhReleaseQueuedLockExclusive(&PhpHwNamedSensorLock);
}

/**
 * Runs a family's pass when its interval has passed and it is not backing off after STATUS_RETRY.
 *
 * \param Family The family.
 * \param Now The provider's time of this update.
 */
static VOID PhpHwRunFamily(
    _Inout_ PPHP_HW_FAMILY Family,
    _In_ ULONG64 Now
    )
{
    NTSTATUS status;

    if (Family->Absent || !Family->Pass)
        return;
    if (Family->Ran && Now - Family->LastRun < Family->Interval)
        return;

    if (Family->RetryStreak >= 2 && ++Family->BackoffUpdates < PHP_HW_RETRY_BACKOFF)
        return;

    Family->BackoffUpdates = 0;
    Family->Ran = TRUE;
    Family->LastRun = Now;

    status = Family->Pass();

    if (status == STATUS_RETRY)
        Family->RetryStreak++;
    else if (status == STATUS_NOT_SUPPORTED)
        Family->Absent = TRUE;
    else if (NT_SUCCESS(status))
        Family->RetryStreak = 0;
}

/**
 * Recomputes the capability bits from what currently has a value.
 */
static VOID PhpHwUpdateCapabilities(
    VOID
    )
{
    ULONG capabilities = 0;

    for (ULONG i = 0; i < PhpHwProcessorCount; i++)
        capabilities |= PhpHwCpuSensors[i].Valid;
    for (ULONG i = 0; i < PhpHwPackageCount; i++)
        capabilities |= PhpHwPackageSensors[i].Valid;

    PhpHwCapabilities = capabilities;
}

/**
 * Runs one update of the hardware provider on the provider thread.
 *
 * \param Object The registration's object, unused.
 */
_Function_class_(PH_PROVIDER_FUNCTION)
VOID PhHwProviderUpdate(
    _In_ PVOID Object
    )
{
    PH_PROVIDER_UPDATED_EVENT updatedEvent;
    ULONG64 now;
    BOOLEAN open;

    UNREFERENCED_PARAMETER(Object);

    now = PhpHwQueryTime();
    open = PhCsEnableHardwareSensors && PhpHwDriverReady();

    if (open && !PhpHwGateOpen)
        PhpHwBeginSession();
    else if (!open && PhpHwGateOpen)
        PhpHwEndSession();

    if (PhpHwGateOpen)
    {
        for (ULONG i = 0; i < PhpHwFamilyMaximum; i++)
            PhpHwRunFamily(&PhpHwFamilies[i], now);

        PhpHwUpdateCapabilities();
    }

    memset(&updatedEvent, 0, sizeof(PH_PROVIDER_UPDATED_EVENT));
    updatedEvent.RunCount = PhpHwRunCount++;
    updatedEvent.UpdateInterval = PhCsUpdateInterval;
    updatedEvent.ElapsedMilliseconds = PhGetProviderElapsedMilliseconds(&PhHwProviderRegistration);

    if (!PhGetProviderElapsed(&PhHwProviderRegistration, &updatedEvent.Elapsed))
        memset(&updatedEvent.Elapsed, 0, sizeof(PH_PROVIDER_ELAPSED));

    PhInvokeCallback(PhGetGeneralCallback(GeneralCallbackHardwareProviderUpdatedEvent), &updatedEvent);
}

/**
 * Appends the published values to the histories; called by the process provider after its system history.
 */
VOID PhHwProviderUpdateHistory(
    VOID
    )
{
    NOTHING;
}

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
