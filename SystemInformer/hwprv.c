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
#include <trace.h>

#define PHP_HW_SOURCE_MAX_SENSORS 1024 // per source
#define PHP_HW_SOURCE_MAX_NAME 128 // characters

typedef struct _PHP_HW_SOURCE_SENSOR
{
    ULONG ComponentId;
    ULONG Type;
    ULONG SensorId;
    PPH_STRING ComponentName;
    PPH_STRING SensorName;
    ULONG Flags;
    FLOAT Value;
    ULONG UpdateId; // PhpHwSourceUpdateId of the last report
    BOOLEAN HasComponentClass;
    GUID ComponentClass;
} PHP_HW_SOURCE_SENSOR, *PPHP_HW_SOURCE_SENSOR;

typedef struct _PHP_HW_REPORT_REQUEST
{
    LIST_ENTRY ListEntry;
    PH_EVENT ServedEvent;
    BOOLEAN Served;
} PHP_HW_REPORT_REQUEST, *PPHP_HW_REPORT_REQUEST;

typedef struct _PH_HW_SENSOR_SOURCE_REGISTRATION
{
    LIST_ENTRY ListEntry;
    ULONG Id;
    PPH_PLUGIN Plugin;
    PH_HW_SENSOR_SOURCE Source;
    PPH_LIST Sensors; // PPHP_HW_SOURCE_SENSOR in first-report order
    ULONG NextIndex; // where the next report is expected
    BOOLEAN CapTraced;
} PH_HW_SENSOR_SOURCE_REGISTRATION;

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

PH_CIRCULAR_BUFFER_FLOAT PhHwCpuTemperatureHistory;
PH_CIRCULAR_BUFFER_FLOAT PhHwCpuPowerHistory;
FLOAT PhHwCpuTemperature = 0.0f;
FLOAT PhHwCpuPower = 0.0f;
static ULONG PhpHwCpuFallbackValid = 0; // PH_HW_CAP_PACKAGE_TEMP and PH_HW_CAP_RAPL_PACKAGE from the Windows counters
static FLOAT PhpHwCpuFallbackTemperature = 0.0f; // the hottest zone
static FLOAT PhpHwCpuFallbackPower = 0.0f; // the package power fallback, watts

PH_PROVIDER_REGISTRATION PhHwProviderRegistration;

PHP_HW_FAMILY PhpHwFamilies[PhpHwFamilyMaximum] =
{
    { L"cpu", 0, PhpHwCpuProbe, PhpHwCpuPass, PhpHwCpuEnd, PhpHwCpuDiagnostics },
    { L"smn", 0, PhpHwSmnProbe, PhpHwSmnPass, PhpHwSmnEnd, PhpHwSmnDiagnostics },
    { L"mmio", 0, PhpHwMmioProbe, PhpHwMmioPass, PhpHwMmioEnd, PhpHwMmioDiagnostics },
    { L"platform", 0, PhpHwPlatformProbe, PhpHwPlatformPass, PhpHwPlatformEnd, PhpHwPlatformDiagnostics },
    { L"pciconfig", 0, PhpHwPciProbe, PhpHwPciPass, PhpHwPciEnd, PhpHwPciDiagnostics },
    { L"vsec", 0, PhpHwVsecProbe, PhpHwVsecPass, PhpHwVsecEnd, PhpHwVsecDiagnostics },
    { L"crosec", 0, PhpHwCrosEcProbe, PhpHwCrosEcPass, PhpHwCrosEcEnd, PhpHwCrosEcDiagnostics },
    { L"boardec", PHP_HW_SECOND, PhpHwBoardEcProbe, PhpHwBoardEcPass, PhpHwBoardEcEnd, PhpHwBoardEcDiagnostics },
    { L"gigabyte", 2 * PHP_HW_SECOND, PhpHwGigabyteProbe, PhpHwGigabytePass, PhpHwGigabyteEnd, PhpHwGigabyteDiagnostics },
    { L"alienware", 2 * PHP_HW_SECOND, PhpHwAlienwareProbe, PhpHwAlienwarePass, PhpHwAlienwareEnd, PhpHwAlienwareDiagnostics },
    { L"hp", 2 * PHP_HW_SECOND, PhpHwHpProbe, PhpHwHpPass, PhpHwHpEnd, PhpHwHpDiagnostics },
    { L"prom21", 2 * PHP_HW_SECOND, PhpHwProm21Probe, PhpHwProm21Pass, PhpHwProm21End, PhpHwProm21Diagnostics },
    { L"aqtion", 5 * PHP_HW_SECOND, PhpHwAqtionProbe, PhpHwAqtionPass, PhpHwAqtionEnd, PhpHwAqtionDiagnostics },
    { L"vrm", 2 * PHP_HW_SECOND, PhpHwVrmProbe, PhpHwVrmPass, PhpHwVrmEnd, PhpHwVrmDiagnostics },
    { L"sch5027", 2 * PHP_HW_SECOND, PhpHwSch5027Probe, PhpHwSch5027Pass, PhpHwSch5027End, PhpHwSch5027Diagnostics },
    { L"smu", PHP_HW_SECOND, PhpHwSmuProbe, PhpHwSmuPass, PhpHwSmuEnd, PhpHwSmuDiagnostics },
    { L"hsmp", PHP_HW_SECOND, PhpHwHsmpProbe, PhpHwHsmpPass, PhpHwHsmpEnd, PhpHwHsmpDiagnostics },
    { L"smbus", 3 * PHP_HW_SECOND, PhpHwSmbusProbe, PhpHwSmbusPass, PhpHwSmbusEndSession, PhpHwSmbusDiagnostics },
    { L"acpi", 2 * PHP_HW_SECOND, PhpHwAcpiProbe, PhpHwAcpiPass, PhpHwAcpiEnd, PhpHwAcpiDiagnostics },
    { L"tsens", 0, PhpHwTsensProbe, PhpHwTsensPass, PhpHwTsensEnd, PhpHwTsensDiagnostics },
    { L"spbm", 0, PhpHwSpbmProbe, PhpHwSpbmPass, PhpHwSpbmEnd, PhpHwSpbmDiagnostics },
};

BOOLEAN PhpHwReportReads = FALSE;
ULONG PhpHwReportWaitMilliseconds = 5000;

static ULONG PhpHwRunCount = 0;
static BOOLEAN PhpHwTopologyQueried = FALSE;
static BOOLEAN PhpHwHistoryInitialized = FALSE;

// The update lock serializes source callbacks with registration; the data lock guards the source
// list and the reported sensors. The list changes under both.
static PH_QUEUED_LOCK PhpHwSourceUpdateLock = PH_QUEUED_LOCK_INIT;
static PH_QUEUED_LOCK PhpHwSourceDataLock = PH_QUEUED_LOCK_INIT;
static LIST_ENTRY PhpHwSourceListHead = { &PhpHwSourceListHead, &PhpHwSourceListHead };
static ULONG PhpHwSourceNextId = 1;
static ULONG PhpHwSourceUpdateId = 0;
static ULONG PhpHwSourceCompletedUpdateId = 0; // the last update whose source loop finished
static HANDLE PhpHwSourceUpdateThread = NULL; // the thread in the source loop, for assertions

// Report requests waiting for an update to make the report-only reads (D11).
static PH_QUEUED_LOCK PhpHwReportLock = PH_QUEUED_LOCK_INIT;
static LIST_ENTRY PhpHwReportRequests = { &PhpHwReportRequests, &PhpHwReportRequests };
static LONG PhpHwRefreshRequested = FALSE; // the next update runs every family regardless of its interval
static BOOLEAN PhpHwOffUpdated = FALSE; // an update ran with the setting off since it was last on

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
        PhpHwFamilies[i].NotShipped = FALSE;
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

    for (ULONG i = 0; i < topology.PackageCount; i++)
    {
        PhInitializeCircularBuffer_FLOAT(&PhpHwPackageSensors[i].TemperatureHistory, PhStatisticsSampleCount);
        PhInitializeCircularBuffer_FLOAT(&PhpHwPackageSensors[i].PackagePowerHistory, PhStatisticsSampleCount);
    }

    PhpHwProcessorCount = topology.ProcessorCount;
    WriteULongRelease(&PhpHwPackageCount, topology.PackageCount);

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

    // Without the driver only the processor values were published.
    PhpHwAvailable = FALSE;

    for (ULONG i = 0; i < PhpHwFamilyMaximum; i++)
    {
        PPHP_HW_FAMILY family = &PhpHwFamilies[i];

        family->Absent = FALSE;
        family->NotShipped = FALSE;
        family->Ran = FALSE;
        family->RetryStreak = 0;
        family->BackoffUpdates = 0;

        if (family->Probe)
        {
            NTSTATUS status = family->Probe();

            if (status == STATUS_NOT_SUPPORTED)
            {
                family->Absent = TRUE;
            }
            else if (PhpHwStatusNotShipped(status))
            {
                family->Absent = TRUE;
                family->NotShipped = TRUE;
            }
        }
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

        PhpHwFamilies[i].Capabilities = 0;
    }

    PhpHwGateOpen = FALSE;
    PhpHwProbed = FALSE;
    PhpHwAvailable = FALSE;
    PhpHwCapabilities = 0;
    PhpHwChipsetTemperatureValid = FALSE;

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
 * \param Force Run the pass regardless of the interval and the back-off, for a report request.
 */
static VOID PhpHwRunFamily(
    _Inout_ PPHP_HW_FAMILY Family,
    _In_ ULONG64 Now,
    _In_ BOOLEAN Force
    )
{
    NTSTATUS status;

    if (Family->Absent || !Family->Pass)
        return;

    if (!Force)
    {
        if (Family->Ran && Now - Family->LastRun < Family->Interval)
            return;
        if (Family->RetryStreak >= 2 && ++Family->BackoffUpdates < PHP_HW_RETRY_BACKOFF)
            return;
    }

    Family->BackoffUpdates = 0;
    Family->Ran = TRUE;
    Family->LastRun = Now;

    status = Family->Pass();

    if (status == STATUS_RETRY)
    {
        Family->RetryStreak++;
    }
    else if (status == STATUS_NOT_SUPPORTED)
    {
        Family->Absent = TRUE;
    }
    else if (PhpHwStatusNotShipped(status))
    {
        Family->Absent = TRUE;
        Family->NotShipped = TRUE;
    }
    else if (NT_SUCCESS(status))
    {
        Family->RetryStreak = 0;
    }
}

/**
 * Takes the waiting report requests for this update to serve.
 *
 * \param Serving Receives the requests; empty when none waits.
 */
static VOID PhpHwTakeReportRequests(
    _Out_ PLIST_ENTRY Serving
    )
{
    InitializeListHead(Serving);

    PhAcquireQueuedLockExclusive(&PhpHwReportLock);

    if (!IsListEmpty(&PhpHwReportRequests))
    {
        Serving->Flink = PhpHwReportRequests.Flink;
        Serving->Blink = PhpHwReportRequests.Blink;
        Serving->Flink->Blink = Serving;
        Serving->Blink->Flink = Serving;
        InitializeListHead(&PhpHwReportRequests);
    }

    PhReleaseQueuedLockExclusive(&PhpHwReportLock);
}

/**
 * Wakes the report requests this update served.
 *
 * \param Serving The requests from PhpHwTakeReportRequests; a request that gave up has removed itself.
 */
static VOID PhpHwSignalReportRequests(
    _Inout_ PLIST_ENTRY Serving
    )
{
    PhAcquireQueuedLockExclusive(&PhpHwReportLock);

    while (!IsListEmpty(Serving))
    {
        PPHP_HW_REPORT_REQUEST request = CONTAINING_RECORD(RemoveHeadList(Serving), PHP_HW_REPORT_REQUEST, ListEntry);

        InitializeListHead(&request->ListEntry);
        request->Served = TRUE;
        PhSetEvent(&request->ServedEvent);
    }

    PhReleaseQueuedLockExclusive(&PhpHwReportLock);
}

/**
 * Asks the next update for the report-only reads and waits for it.
 *
 * \return TRUE when an update made the reads, FALSE when none came within PhpHwReportWaitMilliseconds.
 */
static BOOLEAN PhpHwRequestReportReads(
    VOID
    )
{
    PHP_HW_REPORT_REQUEST request;
    LARGE_INTEGER timeout;
    BOOLEAN served;

    PhInitializeEvent(&request.ServedEvent);
    request.Served = FALSE;

    PhAcquireQueuedLockExclusive(&PhpHwReportLock);
    InsertTailList(&PhpHwReportRequests, &request.ListEntry);
    PhReleaseQueuedLockExclusive(&PhpHwReportLock);

    PhBoostProvider(&PhHwProviderRegistration, NULL);

    PhWaitForEvent(&request.ServedEvent, PhTimeoutFromMilliseconds(&timeout, PhpHwReportWaitMilliseconds));

    PhAcquireQueuedLockExclusive(&PhpHwReportLock);

    if (!(served = request.Served))
        RemoveEntryList(&request.ListEntry);

    PhReleaseQueuedLockExclusive(&PhpHwReportLock);

    // An event that was waited on and never set still holds its wait handle.
    if (!served)
        PhSetEvent(&request.ServedEvent);

    return served;
}

/**
 * Adds a named sensor for the session. Called on the provider thread.
 *
 * \param Kind The kind of the sensor.
 * \param PackageIndex The component of the sensor.
 * \param Id The ID of the sensor within the kind and component.
 * \param Name The name of the sensor; at most PH_HW_NAMED_SENSOR_NAME_LENGTH - 1 characters are kept.
 * \return The index of the sensor, or ULONG_MAX when the store is full.
 */
ULONG PhpHwAddNamedSensor(
    _In_ PH_HW_NAMED_SENSOR_KIND Kind,
    _In_ ULONG PackageIndex,
    _In_ ULONG Id,
    _In_ PCWSTR Name
    )
{
    PPH_HW_NAMED_SENSOR sensor;
    ULONG index = ULONG_MAX;

    PhAcquireQueuedLockExclusive(&PhpHwNamedSensorLock);

    if (!PhpHwNamedSensors)
        PhpHwNamedSensors = PhAllocate(sizeof(PH_HW_NAMED_SENSOR) * PHP_HW_MAX_NAMED_SENSORS);

    if (PhpHwNamedSensorCount < PHP_HW_MAX_NAMED_SENSORS)
    {
        index = PhpHwNamedSensorCount++;
        sensor = &PhpHwNamedSensors[index];
        memset(sensor, 0, sizeof(PH_HW_NAMED_SENSOR));
        sensor->Kind = Kind;
        sensor->PackageIndex = PackageIndex;
        sensor->Id = Id;
        sensor->Active = TRUE;
        PhCopyStringZ(Name, RTL_NUMBER_OF(sensor->Name) - 1, sensor->Name, RTL_NUMBER_OF(sensor->Name), NULL);
    }

    PhReleaseQueuedLockExclusive(&PhpHwNamedSensorLock);

    return index;
}

/**
 * Sets the value of a named sensor. Called on the provider thread.
 *
 * \param Index The index from PhpHwAddNamedSensor; ULONG_MAX is ignored.
 * \param Valid Whether the value is current; a value that is not keeps the last one.
 * \param Value The value.
 */
VOID PhpHwSetNamedSensor(
    _In_ ULONG Index,
    _In_ BOOLEAN Valid,
    _In_ FLOAT Value
    )
{
    PhAcquireQueuedLockExclusive(&PhpHwNamedSensorLock);

    if (Index < PhpHwNamedSensorCount)
    {
        if (Valid)
            PhpHwNamedSensors[Index].Value = Value;

        PhpHwNamedSensors[Index].Valid = Valid;
    }

    PhReleaseQueuedLockExclusive(&PhpHwNamedSensorLock);
}

/**
 * Determines whether the session found nothing because a script is not shipped.
 *
 * \return TRUE when the session has probed, no family with a probe is present and one was not shipped with this
 * build; a family absent for its hardware (no root complex, another processor) does not count against it.
 */
BOOLEAN PhpHwFamiliesNotShipped(
    VOID
    )
{
    BOOLEAN notShipped = FALSE;

    if (!PhpHwGateOpen || !PhpHwProbed)
        return FALSE;

    for (ULONG i = 0; i < PhpHwFamilyMaximum; i++)
    {
        if (!PhpHwFamilies[i].Probe)
            continue;
        if (!PhpHwFamilies[i].Absent)
            return FALSE;
        if (PhpHwFamilies[i].NotShipped)
            notShipped = TRUE;
    }

    return notShipped;
}

static BOOLEAN PhpHwSourceSensorMatches(
    _In_ PPHP_HW_SOURCE_SENSOR Sensor,
    _In_ PCPH_HW_SENSOR_REPORT Report
    )
{
    return Sensor->ComponentId == Report->ComponentId && Sensor->Type == Report->Type && Sensor->SensorId == Report->SensorId;
}

static PPH_STRING PhpHwCreateSourceName(
    _In_ PCWSTR Name
    )
{
    SIZE_T count = 0;

    while (count < PHP_HW_SOURCE_MAX_NAME && Name[count] != UNICODE_NULL)
        count++;

    return PhCreateStringEx((PWSTR)Name, count * sizeof(WCHAR));
}

_Function_class_(PH_HW_SENSOR_REPORT_FUNCTION)
static VOID NTAPI PhpHwSourceReport(
    _In_ PVOID ReportContext,
    _In_ PCPH_HW_SENSOR_REPORT Report
    )
{
    PPH_HW_SENSOR_SOURCE_REGISTRATION source = ReportContext;
    PPHP_HW_SOURCE_SENSOR sensor = NULL;
    ULONG index;

    if (Report->Size < RTL_SIZEOF_THROUGH_FIELD(PH_HW_SENSOR_REPORT, Value))
        return;
    if (Report->Type >= PhHwSensorTypeMaximum || (Report->Flags & ~PH_HW_SENSOR_REPORT_HISTORY))
        return;
    if (!Report->ComponentName || !Report->SensorName)
        return;
    if (!(Report->Value >= -FLT_MAX && Report->Value <= FLT_MAX))
        return;

    PhAcquireQueuedLockExclusive(&PhpHwSourceDataLock);

    // Reports usually arrive in the same order on every update.
    index = source->NextIndex;

    if (index >= source->Sensors->Count || !PhpHwSourceSensorMatches(source->Sensors->Items[index], Report))
    {
        for (index = 0; index < source->Sensors->Count; index++)
        {
            if (PhpHwSourceSensorMatches(source->Sensors->Items[index], Report))
                break;
        }
    }

    if (index < source->Sensors->Count)
    {
        sensor = source->Sensors->Items[index];
    }
    else if (source->Sensors->Count < PHP_HW_SOURCE_MAX_SENSORS)
    {
        sensor = PhAllocateZero(sizeof(PHP_HW_SOURCE_SENSOR));
        sensor->ComponentId = Report->ComponentId;
        sensor->Type = Report->Type;
        sensor->SensorId = Report->SensorId;
        sensor->ComponentName = PhpHwCreateSourceName(Report->ComponentName);
        sensor->SensorName = PhpHwCreateSourceName(Report->SensorName);
        sensor->Flags = Report->Flags;
        PhAddItemList(source->Sensors, sensor);
    }
    else if (!source->CapTraced)
    {
        PhTraceWarning("Hardware sensor source %lu exceeds %lu sensors", source->Id, PHP_HW_SOURCE_MAX_SENSORS);
        source->CapTraced = TRUE;
    }

    if (sensor)
    {
        if (Report->Size >= RTL_SIZEOF_THROUGH_FIELD(PH_HW_SENSOR_REPORT, ComponentClass) && Report->ComponentClass)
        {
            sensor->ComponentClass = *Report->ComponentClass;
            sensor->HasComponentClass = TRUE;
        }

        sensor->Value = Report->Value;
        sensor->UpdateId = PhpHwSourceUpdateId;
        source->NextIndex = index + 1;
    }

    PhReleaseQueuedLockExclusive(&PhpHwSourceDataLock);
}

/**
 * Invokes the callback of each registered source; while hardware sensors are off the sources' sensors are dropped.
 *
 * \param Enabled Whether hardware sensors are on. Sources run in user mode: the driver is not needed.
 */
static VOID PhpHwUpdateSources(
    _In_ BOOLEAN Enabled
    )
{
    PhAcquireQueuedLockExclusive(&PhpHwSourceUpdateLock);

    if (!IsListEmpty(&PhpHwSourceListHead))
    {
        PhAcquireQueuedLockExclusive(&PhpHwSourceDataLock);
        PhpHwSourceUpdateId++;
        PhReleaseQueuedLockExclusive(&PhpHwSourceDataLock);

        if (Enabled)
        {
            WritePointerRelease(&PhpHwSourceUpdateThread, NtCurrentThreadId());

            for (PLIST_ENTRY entry = PhpHwSourceListHead.Flink; entry != &PhpHwSourceListHead; entry = entry->Flink)
            {
                PPH_HW_SENSOR_SOURCE_REGISTRATION source = CONTAINING_RECORD(entry, PH_HW_SENSOR_SOURCE_REGISTRATION, ListEntry);

                source->NextIndex = 0;
                source->Source.Callback(PhpHwSourceReport, source, source->Source.Context);
            }

            WritePointerRelease(&PhpHwSourceUpdateThread, NULL);
        }
        else
        {
            PhAcquireQueuedLockExclusive(&PhpHwSourceDataLock);

            for (PLIST_ENTRY entry = PhpHwSourceListHead.Flink; entry != &PhpHwSourceListHead; entry = entry->Flink)
            {
                PPH_HW_SENSOR_SOURCE_REGISTRATION source = CONTAINING_RECORD(entry, PH_HW_SENSOR_SOURCE_REGISTRATION, ListEntry);

                for (ULONG i = 0; i < source->Sensors->Count; i++)
                {
                    PPHP_HW_SOURCE_SENSOR sensor = source->Sensors->Items[i];

                    PhDereferenceObject(sensor->ComponentName);
                    PhDereferenceObject(sensor->SensorName);
                    PhFree(sensor);
                }

                PhClearList(source->Sensors);
            }

            PhReleaseQueuedLockExclusive(&PhpHwSourceDataLock);
        }

        // A sensor not reported by this update goes stale once it finishes.
        PhAcquireQueuedLockExclusive(&PhpHwSourceDataLock);
        PhpHwSourceCompletedUpdateId = PhpHwSourceUpdateId;
        PhReleaseQueuedLockExclusive(&PhpHwSourceDataLock);
    }

    PhReleaseQueuedLockExclusive(&PhpHwSourceUpdateLock);
}

/**
 * Recomputes the capability bits from what currently has a value; the first published sensor of the session makes
 * the sensors available.
 */
static VOID PhpHwUpdateCapabilities(
    VOID
    )
{
    ULONG capabilities = 0;

    for (ULONG i = 0; i < PhpHwFamilyMaximum; i++)
        capabilities |= PhpHwFamilies[i].Capabilities;
    for (ULONG i = 0; i < PhpHwProcessorCount; i++)
        capabilities |= PhpHwCpuSensors[i].Valid;
    for (ULONG i = 0; i < PhpHwPackageCount; i++)
        capabilities |= PhpHwPackageSensors[i].Valid;

    PhpHwCapabilities = capabilities;

    if (capabilities || PhpHwNamedSensorCount)
        PhpHwAvailable = TRUE;
}

/**
 * Computes the processor temperature (the hottest package) and power (the packages together), completed from the
 * Windows counters where no package has one.
 *
 * \param Temperature Receives the temperature, 0 without one.
 * \param Power Receives the power, 0 without one.
 * \return PH_HW_CAP_PACKAGE_TEMP and PH_HW_CAP_RAPL_PACKAGE flags for the values given.
 */
static ULONG PhpHwQueryCpuValues(
    _Out_ PFLOAT Temperature,
    _Out_ PFLOAT Power
    )
{
    ULONG valid = 0;
    FLOAT temperature = 0.0f;
    FLOAT power = 0.0f;

    for (ULONG i = 0; i < PhpHwPackageCount; i++)
    {
        PPH_HW_PACKAGE_SENSOR package = &PhpHwPackageSensors[i];

        if ((package->Valid & PH_HW_CAP_PACKAGE_TEMP) && (!(valid & PH_HW_CAP_PACKAGE_TEMP) || package->Temperature > temperature))
        {
            temperature = package->Temperature;
            valid |= PH_HW_CAP_PACKAGE_TEMP;
        }

        if (package->Valid & PH_HW_CAP_RAPL_PACKAGE)
        {
            power += package->PackagePower;
            valid |= PH_HW_CAP_RAPL_PACKAGE;
        }
    }

    if (!(valid & PH_HW_CAP_PACKAGE_TEMP) && (PhpHwCpuFallbackValid & PH_HW_CAP_PACKAGE_TEMP))
    {
        temperature = PhpHwCpuFallbackTemperature;
        valid |= PH_HW_CAP_PACKAGE_TEMP;
    }

    if (!(valid & PH_HW_CAP_RAPL_PACKAGE) && (PhpHwCpuFallbackValid & PH_HW_CAP_RAPL_PACKAGE))
    {
        power = PhpHwCpuFallbackPower;
        valid |= PH_HW_CAP_RAPL_PACKAGE;
    }

    *Temperature = temperature;
    *Power = power;

    return valid;
}

/**
 * Reads the Windows counters' stand-ins for the processor values (the hottest zone and the package power fallback)
 * and publishes PhHwCpuTemperature and PhHwCpuPower, their only writer: the history samples them on another thread
 * mid-update. The package sensors keep only their own readings.
 *
 * \param Fallback Whether the counters may stand in.
 */
static VOID PhpHwUpdateCpuValues(
    _In_ BOOLEAN Fallback
    )
{
    ULONG valid = 0;
    FLOAT celsius;
    FLOAT watts;

    if (Fallback && PhpHwCounterHottestZone(&celsius))
    {
        PhpHwCpuFallbackTemperature = celsius;
        valid |= PH_HW_CAP_PACKAGE_TEMP;
    }

    if (Fallback && PhpHwCounterPackagePower(&watts))
    {
        PhpHwCpuFallbackPower = watts;
        valid |= PH_HW_CAP_RAPL_PACKAGE;
    }

    PhpHwCpuFallbackValid = valid;
    PhpHwQueryCpuValues(&PhHwCpuTemperature, &PhHwCpuPower);
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
    BOOLEAN refresh;

    UNREFERENCED_PARAMETER(Object);

    // With the setting off, the first update ends the session and drops the sources; the rest have nothing to do.
    if (!PhCsEnableHardwareSensors && PhpHwOffUpdated)
        return;

    PhpHwOffUpdated = !PhCsEnableHardwareSensors;

    now = PhpHwQueryTime();
    open = PhCsEnableHardwareSensors && PhpHwDriverReady();

    if (open && !PhpHwGateOpen)
        PhpHwBeginSession();
    else if (!open && PhpHwGateOpen)
        PhpHwEndSession();

    refresh = !!InterlockedExchange(&PhpHwRefreshRequested, FALSE);

    if (PhpHwGateOpen)
    {
        LIST_ENTRY serving;

        PhpHwTakeReportRequests(&serving);
        PhpHwReportReads = !IsListEmpty(&serving);

        for (ULONG i = 0; i < PhpHwFamilyMaximum; i++)
            PhpHwRunFamily(&PhpHwFamilies[i], now, PhpHwReportReads || refresh);

        PhpHwReportReads = FALSE;

        PhpHwUpdateCounters(TRUE);
        PhpHwUpdateSources(TRUE);
        PhpHwUpdateCapabilities();
        // No ARM64 part has a package temperature or energy register; on x64 a zone is often a board sensor.
        PhpHwUpdateCpuValues(PhpHwCpuIsArm64());
        PhpHwSignalReportRequests(&serving);
    }
    else
    {
        LIST_ENTRY serving;

        PhpHwUpdateCounters(!!PhCsEnableHardwareSensors);
        PhpHwUpdateSources(!!PhCsEnableHardwareSensors);
        PhpHwUpdateCpuValues(!!PhCsEnableHardwareSensors);
        PhpHwAvailable = !!PhpHwCpuFallbackValid;

        // A request queued before the gate closed gets what is held.
        PhpHwTakeReportRequests(&serving);
        PhpHwSignalReportRequests(&serving);
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
 * Runs an update now that reads every family regardless of its interval.
 */
VOID PhHwProviderRefresh(
    VOID
    )
{
    InterlockedExchange(&PhpHwRefreshRequested, TRUE);
    PhBoostProvider(&PhHwProviderRegistration, NULL);
}

/**
 * Appends the published values to the histories; called by the process provider after its system history.
 */
VOID PhHwProviderUpdateHistory(
    VOID
    )
{
    ULONG packageCount;

    if (!PhpHwHistoryInitialized)
    {
        PhInitializeCircularBuffer_FLOAT(&PhHwCpuTemperatureHistory, PhStatisticsSampleCount);
        PhInitializeCircularBuffer_FLOAT(&PhHwCpuPowerHistory, PhStatisticsSampleCount);
        WriteBooleanRelease(&PhpHwHistoryInitialized, TRUE);
    }

    PhAddItemCircularBuffer_FLOAT(&PhHwCpuTemperatureHistory, PhHwCpuTemperature);
    PhAddItemCircularBuffer_FLOAT(&PhHwCpuPowerHistory, PhHwCpuPower);

    packageCount = ReadULongAcquire(&PhpHwPackageCount);

    for (ULONG i = 0; i < packageCount; i++)
    {
        PPH_HW_PACKAGE_SENSOR sensor = &PhpHwPackageSensors[i];
        FLOAT temperature = 0.0f;
        FLOAT power = 0.0f;

        if (sensor->Valid & PH_HW_CAP_PACKAGE_TEMP)
            temperature = sensor->Temperature;
        if (sensor->Valid & PH_HW_CAP_RAPL_PACKAGE)
            power = sensor->PackagePower;

        PhAddItemCircularBuffer_FLOAT(&sensor->TemperatureHistory, temperature);
        PhAddItemCircularBuffer_FLOAT(&sensor->PackagePowerHistory, power);
    }

    PhpHwHistoryAppend();
}

/**
 * Retrieves the CPU temperature and power histories, which are sampled with the system histories.
 *
 * \param TemperatureHistory Receives the hottest package temperature at each sample, 0 without a reading.
 * \param PowerHistory Receives the total package power at each sample, 0 without a reading.
 * \return TRUE once the histories exist, otherwise FALSE.
 */
_Success_(return)
BOOLEAN NTAPI PhHwGetCpuHistory(
    _Out_ PPH_CIRCULAR_BUFFER_FLOAT* TemperatureHistory,
    _Out_ PPH_CIRCULAR_BUFFER_FLOAT* PowerHistory
    )
{
    if (!ReadBooleanAcquire(&PhpHwHistoryInitialized))
        return FALSE;

    *TemperatureHistory = &PhHwCpuTemperatureHistory;
    *PowerHistory = &PhHwCpuPowerHistory;

    return TRUE;
}

static VOID PhpHwAppendFlags(
    _Inout_ PPH_STRING_BUILDER Builder,
    _In_ ULONG Flags,
    _In_reads_(Count) const PH_KEY_VALUE_PAIR* Names,
    _In_ ULONG Count
    )
{
    BOOLEAN first = TRUE;

    PhAppendFormatStringBuilder(Builder, L"0x%08lx", Flags);

    for (ULONG i = 0; i < Count; i++)
    {
        if (!(Flags & PtrToUlong(Names[i].Value)))
            continue;

        if (first)
            PhAppendStringBuilder2(Builder, L" (");
        else
            PhAppendStringBuilder2(Builder, L", ");
        PhAppendStringBuilder2(Builder, (PWSTR)Names[i].Key);
        first = FALSE;
    }

    if (!first)
        PhAppendCharStringBuilder(Builder, L')');

    PhAppendCharStringBuilder(Builder, L'\n');
}

/**
 * Writes the state of the hardware sensors provider and each family's section for the hardware report.
 *
 * \details With the gate open, the next update first makes the report-only reads (D11).
 *
 * \param Builder The report.
 */
VOID PhHwWriteDiagnostics(
    _Inout_ PPH_STRING_BUILDER Builder
    )
{
    static CONST PH_KEY_VALUE_PAIR capabilityNames[] =
    {
        { L"core temperature", (PVOID)PH_HW_CAP_CORE_TEMP },
        { L"package temperature", (PVOID)PH_HW_CAP_PACKAGE_TEMP },
        { L"CCD temperature", (PVOID)PH_HW_CAP_CCD_TEMP },
        { L"effective clock", (PVOID)PH_HW_CAP_EFFECTIVE_CLOCK },
        { L"multiplier", (PVOID)PH_HW_CAP_MULTIPLIER },
        { L"voltage", (PVOID)PH_HW_CAP_VOLTAGE },
        { L"throttle", (PVOID)PH_HW_CAP_THROTTLE },
        { L"RAPL package", (PVOID)PH_HW_CAP_RAPL_PACKAGE },
        { L"RAPL core", (PVOID)PH_HW_CAP_RAPL_CORE },
        { L"RAPL DRAM", (PVOID)PH_HW_CAP_RAPL_DRAM },
        { L"RAPL platform", (PVOID)PH_HW_CAP_RAPL_PSYS },
        { L"active clock", (PVOID)PH_HW_CAP_ACTIVE_CLOCK },
        { L"memory clock", (PVOID)PH_HW_CAP_MEMORY_CLOCK },
        { L"memory bandwidth", (PVOID)PH_HW_CAP_MEMORY_BANDWIDTH },
        { L"cores temperature", (PVOID)PH_HW_CAP_CORES_TEMP },
        { L"graphics temperature", (PVOID)PH_HW_CAP_GRAPHICS_TEMP },
        { L"PL1", (PVOID)PH_HW_CAP_POWER_LIMIT1 },
        { L"PL2", (PVOID)PH_HW_CAP_POWER_LIMIT2 },
        { L"chipset temperature", (PVOID)PH_HW_CAP_CHIPSET_TEMP },
        { L"DIMM temperature", (PVOID)PH_HW_CAP_DIMM_TEMP },
        { L"uncore clock", (PVOID)PH_HW_CAP_UNCORE_CLOCK },
        { L"TPMI/PMT", (PVOID)PH_HW_CAP_TPMI_POWER },
        { L"SMU", (PVOID)PH_HW_CAP_SMU },
        { L"fan speed", (PVOID)PH_HW_CAP_FAN_SPEED },
        { L"memory timings", (PVOID)PH_HW_CAP_MEMORY_TIMINGS },
        { L"DIMM voltage", (PVOID)PH_HW_CAP_DIMM_VOLTAGE },
        { L"DIMM PMIC", (PVOID)PH_HW_CAP_DIMM_PMIC },
        { L"ACPI temperature", (PVOID)PH_HW_CAP_ACPI_TEMP },
        { L"fan control", (PVOID)PH_HW_CAP_FAN_CONTROL },
        { L"ACPI power", (PVOID)PH_HW_CAP_ACPI_POWER },
        { L"S0ix residency", (PVOID)PH_HW_CAP_RESIDENCY },
        { L"platform limits", (PVOID)(ULONG_PTR)PH_HW_CAP_PLATFORM_LIMITS },
    };
    PCWSTR reads;
    PCWSTR setting = L"disabled";
    PCWSTR driverReady = L"no";
    PCWSTR gateOpen = L"no";
    PCWSTR probed = L"no";
    PCWSTR available = L"no";

    if (PhpHwGateOpen && !PhpHwRequestReportReads())
        reads = L"the provider did not update within the wait; the sections show what is held";
    else if (!PhpHwGateOpen)
        reads = L"none, hardware sensors are off; the sections show what is held";
    else
        reads = L"made by the update";

    if (PhCsEnableHardwareSensors)
        setting = L"enabled";
    if (PhpHwDriverReady())
        driverReady = L"yes";
    if (PhpHwGateOpen)
        gateOpen = L"yes";
    if (PhpHwProbed)
        probed = L"yes";
    if (PhpHwAvailable)
        available = L"yes";

    PhAppendFormatStringBuilder(
        Builder,
        L"Setting: %s\nDriver ready: %s\nGate open: %s\nProbed: %s\nAvailable: %s\nCapabilities: ",
        setting,
        driverReady,
        gateOpen,
        probed,
        available
        );
    PhpHwAppendFlags(Builder, PhpHwCapabilities, capabilityNames, RTL_NUMBER_OF(capabilityNames));
    PhAppendFormatStringBuilder(Builder, L"Report reads: %s\n", reads);
    PhpHwHistoryWriteDiagnostics(Builder);

    for (ULONG i = 0; i < PhpHwFamilyMaximum; i++)
    {
        PPHP_HW_FAMILY family = &PhpHwFamilies[i];
        PCWSTR state;

        if (!PhpHwProbed)
            state = L"not probed";
        else if (family->NotShipped)
            state = L"not shipped with this build";
        else if (family->Absent)
            state = L"absent";
        else if (family->RetryStreak >= 2)
            state = L"backing off after retries";
        else
            state = L"present";

        PhAppendFormatStringBuilder(Builder, L"\n%s\n  State: %s\n", family->Name, state);

        if (family->Diagnostics)
            family->Diagnostics(Builder);
        else
            PhAppendStringBuilder2(Builder, L"  Not collected\n");
    }

    PhAppendCharStringBuilder(Builder, L'\n');
    PhpHwWriteCounterDiagnostics(Builder);
}

/**
 * Registers a hardware sensor source.
 *
 * \param Plugin The plugin that owns the source.
 * \param Source The source; copied. Size must reach Context (fields past Size are zero) and Flags must be zero.
 * \param Registration Receives the registration; unregister it at the latest from PluginCallbackUnload.
 * \return STATUS_SUCCESS, or STATUS_INVALID_PARAMETER for a malformed source.
 * \remarks Must not be called from within a source callback.
 */
NTSTATUS NTAPI PhRegisterHardwareSensorSource(
    _In_ PPH_PLUGIN Plugin,
    _In_ PCPH_HW_SENSOR_SOURCE Source,
    _Out_ PPH_HW_SENSOR_SOURCE_REGISTRATION* Registration
    )
{
    PPH_HW_SENSOR_SOURCE_REGISTRATION registration;

    assert(ReadPointerAcquire(&PhpHwSourceUpdateThread) != NtCurrentThreadId());

    if (!Plugin || Source->Size < RTL_SIZEOF_THROUGH_FIELD(PH_HW_SENSOR_SOURCE, Context) || Source->Flags || !Source->Callback)
        return STATUS_INVALID_PARAMETER;

    registration = PhAllocateZero(sizeof(PH_HW_SENSOR_SOURCE_REGISTRATION));
    registration->Plugin = Plugin;
    memcpy(&registration->Source, Source, min(Source->Size, sizeof(PH_HW_SENSOR_SOURCE)));
    registration->Source.Size = sizeof(PH_HW_SENSOR_SOURCE);
    registration->Sensors = PhCreateList(16);

    PhAcquireQueuedLockExclusive(&PhpHwSourceUpdateLock);
    PhAcquireQueuedLockExclusive(&PhpHwSourceDataLock);
    registration->Id = PhpHwSourceNextId++;
    InsertTailList(&PhpHwSourceListHead, &registration->ListEntry);
    PhReleaseQueuedLockExclusive(&PhpHwSourceDataLock);
    PhReleaseQueuedLockExclusive(&PhpHwSourceUpdateLock);

    *Registration = registration;
    return STATUS_SUCCESS;
}

/**
 * Writes the diagnostics of each hardware sensor source for the hardware report.
 *
 * \details The update lock serializes the diagnostics with the source callbacks and registration.
 *
 * \param Builder The report.
 */
VOID PhHwWriteSourceDiagnostics(
    _Inout_ PPH_STRING_BUILDER Builder
    )
{
    ULONG count = 0;

    assert(ReadPointerAcquire(&PhpHwSourceUpdateThread) != NtCurrentThreadId());

    PhAcquireQueuedLockExclusive(&PhpHwSourceUpdateLock);
    WritePointerRelease(&PhpHwSourceUpdateThread, NtCurrentThreadId());

    for (PLIST_ENTRY entry = PhpHwSourceListHead.Flink; entry != &PhpHwSourceListHead; entry = entry->Flink)
    {
        PPH_HW_SENSOR_SOURCE_REGISTRATION registration = CONTAINING_RECORD(entry, PH_HW_SENSOR_SOURCE_REGISTRATION, ListEntry);

        PhAppendFormatStringBuilder(Builder, L"%.*s: %lu sensors\n", (ULONG)(registration->Plugin->Name.Length / sizeof(WCHAR)), registration->Plugin->Name.Buffer, registration->Sensors->Count);

        if (registration->Source.Diagnostics)
            registration->Source.Diagnostics(Builder, registration->Source.Context);

        count++;
    }

    WritePointerRelease(&PhpHwSourceUpdateThread, NULL);
    PhReleaseQueuedLockExclusive(&PhpHwSourceUpdateLock);

    if (!count)
        PhAppendStringBuilder2(Builder, L"None\n");
}

/**
 * Unregisters a hardware sensor source and removes its sensors.
 *
 * \details Waits for a callback in progress; afterwards its context may be freed.
 *
 * \param Registration The registration from PhRegisterHardwareSensorSource.
 * \remarks Must not be called from within a source callback.
 */
VOID NTAPI PhUnregisterHardwareSensorSource(
    _In_ _Post_invalid_ PPH_HW_SENSOR_SOURCE_REGISTRATION Registration
    )
{
    assert(ReadPointerAcquire(&PhpHwSourceUpdateThread) != NtCurrentThreadId());

    PhAcquireQueuedLockExclusive(&PhpHwSourceUpdateLock);
    PhAcquireQueuedLockExclusive(&PhpHwSourceDataLock);
    RemoveEntryList(&Registration->ListEntry);
    PhReleaseQueuedLockExclusive(&PhpHwSourceDataLock);
    PhReleaseQueuedLockExclusive(&PhpHwSourceUpdateLock);

    for (ULONG i = 0; i < Registration->Sensors->Count; i++)
    {
        PPHP_HW_SOURCE_SENSOR sensor = Registration->Sensors->Items[i];

        PhDereferenceObject(sensor->ComponentName);
        PhDereferenceObject(sensor->SensorName);
        PhFree(sensor);
    }

    PhDereferenceObject(Registration->Sensors);
    PhFree(Registration);
}

/**
 * Removes the sources plugins left registered. Called after the plugin unload callbacks.
 */
VOID PhHwSweepSensorSources(
    VOID
    )
{
    ULONG count = 0;

    while (TRUE)
    {
        PPH_HW_SENSOR_SOURCE_REGISTRATION registration = NULL;

        PhAcquireQueuedLockShared(&PhpHwSourceDataLock);

        if (!IsListEmpty(&PhpHwSourceListHead))
            registration = CONTAINING_RECORD(PhpHwSourceListHead.Flink, PH_HW_SENSOR_SOURCE_REGISTRATION, ListEntry);

        PhReleaseQueuedLockShared(&PhpHwSourceDataLock);

        if (!registration)
            break;

        PhUnregisterHardwareSensorSource(registration);
        count++;
    }

    if (count)
        PhTraceWarning("Hardware sensors: %lu sources left registered after plugin unload", count);
}

/**
 * Enumerates the sensors of the registered sources in registration and first-report order.
 *
 * \details The callback runs under the source data lock and must not register or unregister a source.
 *
 * \param Callback Called once per sensor.
 * \param Context Passed to the callback.
 */
VOID PhHwEnumerateSourceSensors(
    _In_ PPH_HW_SOURCE_SENSOR_CALLBACK Callback,
    _In_opt_ PVOID Context
    )
{
    PhAcquireQueuedLockShared(&PhpHwSourceDataLock);

    for (PLIST_ENTRY entry = PhpHwSourceListHead.Flink; entry != &PhpHwSourceListHead; entry = entry->Flink)
    {
        PPH_HW_SENSOR_SOURCE_REGISTRATION source = CONTAINING_RECORD(entry, PH_HW_SENSOR_SOURCE_REGISTRATION, ListEntry);

        for (ULONG i = 0; i < source->Sensors->Count; i++)
        {
            PPHP_HW_SOURCE_SENSOR sensor = source->Sensors->Items[i];
            PH_HW_SOURCE_SENSOR info;

            info.SourceId = source->Id;
            info.ComponentId = sensor->ComponentId;
            info.ComponentName = sensor->ComponentName;
            info.Type = sensor->Type;
            info.SensorId = sensor->SensorId;
            info.SensorName = sensor->SensorName;
            info.Flags = sensor->Flags;
            info.Value = sensor->Value;
            // Reported in the last finished update or the running one (history samples on another thread).
            info.Current = sensor->UpdateId - PhpHwSourceCompletedUpdateId <= 1;
            info.ComponentClass = sensor->HasComponentClass ? &sensor->ComponentClass : NULL;

            Callback(&info, Context);
        }
    }

    PhReleaseQueuedLockShared(&PhpHwSourceDataLock);
}

/**
 * Determines whether a source has reported a sensor.
 *
 * \return TRUE when a registered source has a sensor, listed current or not.
 */
BOOLEAN NTAPI PhHwSourceSensorsPresent(
    VOID
    )
{
    BOOLEAN present = FALSE;

    PhAcquireQueuedLockShared(&PhpHwSourceDataLock);

    for (PLIST_ENTRY entry = PhpHwSourceListHead.Flink; entry != &PhpHwSourceListHead; entry = entry->Flink)
    {
        PPH_HW_SENSOR_SOURCE_REGISTRATION source = CONTAINING_RECORD(entry, PH_HW_SENSOR_SOURCE_REGISTRATION, ListEntry);

        if (source->Sensors->Count)
        {
            present = TRUE;
            break;
        }
    }

    PhReleaseQueuedLockShared(&PhpHwSourceDataLock);

    return present;
}

/**
 * Copies the active named sensors under one hold of the lock: the driver families' while they are available, and
 * the Windows counters'.
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
    ULONG driverCount;
    ULONG copied = 0;

    PhAcquireQueuedLockShared(&PhpHwNamedSensorLock);

    driverCount = PhpHwAvailable ? PhpHwNamedSensorCount : 0;

    if (driverCount + PhpHwCounterSensorCount)
    {
        copy = PhAllocate(sizeof(PH_HW_NAMED_SENSOR) * (driverCount + PhpHwCounterSensorCount));

        for (ULONG i = 0; i < driverCount; i++)
        {
            if (PhpHwNamedSensors[i].Active)
                copy[copied++] = PhpHwNamedSensors[i];
        }

        for (ULONG i = 0; i < PhpHwCounterSensorCount; i++)
        {
            if (PhpHwCounterSensors[i].Active)
                copy[copied++] = PhpHwCounterSensors[i];
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

    for (ULONG i = 0; !found && i < PhpHwCounterSensorCount; i++)
    {
        if (
            PhpHwCounterSensors[i].Active &&
            PhpHwCounterSensors[i].Valid &&
            PhpHwCounterSensors[i].Kind == Kind &&
            PhpHwCounterSensors[i].PackageIndex == PackageIndex &&
            PhpHwCounterSensors[i].Id == Id
            )
        {
            *Value = PhpHwCounterSensors[i].Value;
            found = TRUE;
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
 * Retrieves the processor temperature (the hottest package) and power (the packages together); the Windows counters
 * stand in where no package has one (on ARM64, or without the driver).
 *
 * \param Temperature Receives the temperature, 0 without one.
 * \param Power Receives the power, 0 without one.
 * \return PH_HW_CAP_PACKAGE_TEMP and PH_HW_CAP_RAPL_PACKAGE flags for the values given, or 0 when hardware sensors
 * are unavailable.
 */
ULONG PhHwGetCpuValues(
    _Out_ PFLOAT Temperature,
    _Out_ PFLOAT Power
    )
{
    if (!PhHwSensorsAvailable())
    {
        *Temperature = 0.0f;
        *Power = 0.0f;
        return 0;
    }

    return PhpHwQueryCpuValues(Temperature, Power);
}

/**
 * Retrieves which of the processor temperature and power have a value.
 *
 * \return PH_HW_CAP_PACKAGE_TEMP and PH_HW_CAP_RAPL_PACKAGE flags, or 0 when hardware sensors are unavailable.
 */
ULONG PhHwGetCpuValidMask(
    VOID
    )
{
    FLOAT temperature;
    FLOAT power;

    return PhHwGetCpuValues(&temperature, &power);
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
 * \return TRUE when a driver family has published a sensor in this session, or without the driver when the Windows
 * counters give the processor a temperature or power.
 */
BOOLEAN NTAPI PhHwSensorsAvailable(
    VOID
    )
{
    return PhpHwAvailable;
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

