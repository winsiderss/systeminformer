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
#include <perflib.h>
#include <trace.h>

// Collector counters: the Windows performance counters (Perflib V2) of ACPI thermal zones, energy meters and processor
// limits. Sampled on the provider thread once per update; the setting alone enables them, no driver.

#define PHP_HW_COUNTER_MAX_ITEMS 256
#define PHP_HW_COUNTER_MAX_SENSORS (PHP_HW_COUNTER_MAX_ITEMS * 2)
#define PHP_HW_COUNTER_VALUES 4
#define PHP_HW_COUNTER_ZONE_MINIMUM 2732 // tenths of a kelvin; at or below 0 C is a placeholder (0, 2330 = -40 C)
#define PHP_HW_COUNTER_ZONE_MAXIMUM 4500 // tenths of a kelvin; above 177 C is not a temperature (MPTF \_TZ.PSOC, PSYS)
#define PHP_HW_COUNTER_ZONE_DUMMY 3000 // 300.0 K, the constant that firmware dummy zones return
#define PHP_HW_COUNTER_RETRY_TICKS 30 // updates between attempts to open the query after a failure

typedef enum _PHP_HW_COUNTER_SET
{
    PhpHwCounterSetZone, // Thermal Zone Information
    PhpHwCounterSetProcessor, // Processor Information
    PhpHwCounterSetMeter, // Energy Meter
    PhpHwCounterSetMaximum
} PHP_HW_COUNTER_SET;

// {52BC5412-DAC2-449C-8BC2-96443888FE6B}
static CONST GUID PhpHwZoneCounterSet = { 0x52bc5412, 0xdac2, 0x449c, { 0x8b, 0xc2, 0x96, 0x44, 0x38, 0x88, 0xfe, 0x6b } };
// {B4FC721A-0378-476F-89BA-A5A79F810B36}
static CONST GUID PhpHwProcessorCounterSet = { 0xb4fc721a, 0x0378, 0x476f, { 0x89, 0xba, 0xa5, 0xa7, 0x9f, 0x81, 0x0b, 0x36 } };
// {06EBF20D-17FB-4338-A08D-7A99F17CA678}
static CONST GUID PhpHwMeterCounterSet = { 0x06ebf20d, 0x17fb, 0x4338, { 0xa0, 0x8d, 0x7a, 0x99, 0xf1, 0x7c, 0xa6, 0x78 } };

static CONST GUID* CONST PhpHwCounterSetGuids[PhpHwCounterSetMaximum] =
{
    &PhpHwZoneCounterSet,
    &PhpHwProcessorCounterSet,
    &PhpHwMeterCounterSet
};

typedef struct _PHP_HW_COUNTER
{
    PHP_HW_COUNTER_SET Set;
    ULONG CounterId;
    UCHAR Slot; // in PHP_HW_COUNTER_ITEM.Values
} PHP_HW_COUNTER, *PPHP_HW_COUNTER;
typedef const PHP_HW_COUNTER* PCPHP_HW_COUNTER;

// Counter IDs from the registered counter sets; a ratio counter is followed by its base.
static CONST PHP_HW_COUNTER PhpHwCounters[] =
{
    { PhpHwCounterSetZone, 3, 0 }, // High Precision Temperature, tenths of a kelvin
    { PhpHwCounterSetZone, 1, 1 }, // % Passive Limit
    { PhpHwCounterSetZone, 2, 2 }, // Throttle Reasons
    { PhpHwCounterSetProcessor, 33, 0 }, // Actual Frequency, MHz over the base
    { PhpHwCounterSetProcessor, 34, 1 },
    { PhpHwCounterSetProcessor, 30, 2 }, // % Performance Limit
    { PhpHwCounterSetProcessor, 31, 3 }, // Performance Limit Flags
    { PhpHwCounterSetMeter, 3, 0 }, // Power, mW over the base
    { PhpHwCounterSetMeter, 4, 1 },
};

typedef struct _PHP_HW_COUNTER_ITEM
{
    PHP_HW_COUNTER_SET Set;
    WCHAR Name[PH_HW_NAMED_SENSOR_NAME_LENGTH];
    ULONG64 Values[PHP_HW_COUNTER_VALUES];
    ULONG64 Previous[PHP_HW_COUNTER_VALUES];
    UCHAR Sizes[PHP_HW_COUNTER_VALUES]; // bytes of each value; 32-bit counters wrap
    ULONG Seen; // slots read by the last sample
    ULONG PreviousSeen; // slots read by the sample before
    ULONG SensorIndex; // the temperature, clock or power sensor; ULONG_MAX when none
    ULONG SensorId; // the Id of that sensor
    ULONG LimitIndex; // the processor performance limit sensor; ULONG_MAX when none
} PHP_HW_COUNTER_ITEM, *PPHP_HW_COUNTER_ITEM;

// One counter of the last sample, for the report.
typedef struct _PHP_HW_COUNTER_BLOCK
{
    ULONG Counter; // PhpHwCounters index; ULONG_MAX when unknown
    ULONG Status;
    ULONG Type;
    ULONG Instances;
} PHP_HW_COUNTER_BLOCK, *PPHP_HW_COUNTER_BLOCK;

// Provider thread state.
static HANDLE PhpHwCounterQuery = NULL;
static ULONG PhpHwCounterAdded[RTL_NUMBER_OF(PhpHwCounters)]; // PhpHwCounters index of each sample block; ULONG_MAX when unknown
static ULONG PhpHwCounterAddedCount = 0;
static ULONG PhpHwCounterItemCount = 0;
static PHP_HW_COUNTER_ITEM PhpHwCounterItems[PHP_HW_COUNTER_MAX_ITEMS];
static ULONG PhpHwCounterBlockCount = 0;
static PHP_HW_COUNTER_BLOCK PhpHwCounterBlocks[RTL_NUMBER_OF(PhpHwCounters)];
static ULONG PhpHwCounterZoneLimitIndex = ULONG_MAX; // the lowest zone passive limit sensor

// Under PhpHwNamedSensorLock.
ULONG PhpHwCounterSensorCount = 0;
PH_HW_NAMED_SENSOR PhpHwCounterSensors[PHP_HW_COUNTER_MAX_SENSORS];
static BOOLEAN PhpHwCounterZoneValid = FALSE; // a zone has a current temperature
static BOOLEAN PhpHwCounterZoneSeen = FALSE; // a zone had a temperature since the query opened
static FLOAT PhpHwCounterZoneMaximum = 0.0f; // the hottest current zone, degrees Celsius
static ULONG PhpHwCounterStatus = ERROR_NOT_READY; // the last open, add or query result, for the report
static ULONG PhpHwCounterSetsAdded = 0; // bit per PHP_HW_COUNTER_SET with an added counter
static ULONG PhpHwCounterReportCount = 0;
static PHP_HW_COUNTER_ITEM PhpHwCounterReport[PHP_HW_COUNTER_MAX_ITEMS];
static ULONG PhpHwCounterReportBlockCount = 0;
static PHP_HW_COUNTER_BLOCK PhpHwCounterReportBlocks[RTL_NUMBER_OF(PhpHwCounters)];

/**
 * Drops every counter sensor. The caller holds the named sensor lock exclusively.
 */
_Requires_exclusive_lock_held_(PhpHwNamedSensorLock)
static VOID PhpHwCounterClearSensorsLocked(
    VOID
    )
{
    PhpHwCounterSensorCount = 0;
    PhpHwCounterZoneValid = FALSE;
    PhpHwCounterZoneSeen = FALSE;
    PhpHwCounterReportCount = 0;
    PhpHwCounterReportBlockCount = 0;
}

/**
 * Adds a counter sensor.
 *
 * \return The index of the sensor, or ULONG_MAX when the table is full.
 */
static ULONG PhpHwCounterAddSensor(
    _In_ PH_HW_NAMED_SENSOR_KIND Kind,
    _In_ ULONG Id,
    _In_ PCWSTR Name
    )
{
    PPH_HW_NAMED_SENSOR sensor;
    ULONG index = ULONG_MAX;

    PhAcquireQueuedLockExclusive(&PhpHwNamedSensorLock);

    if (PhpHwCounterSensorCount < PHP_HW_COUNTER_MAX_SENSORS)
    {
        index = PhpHwCounterSensorCount++;
        sensor = &PhpHwCounterSensors[index];
        memset(sensor, 0, sizeof(PH_HW_NAMED_SENSOR));
        sensor->Kind = Kind;
        sensor->Id = Id;
        sensor->Active = TRUE;
        PhCopyStringZ(Name, SIZE_MAX, sensor->Name, RTL_NUMBER_OF(sensor->Name), NULL);
    }

    PhReleaseQueuedLockExclusive(&PhpHwNamedSensorLock);

    return index;
}

/**
 * Closes the query, forgets the instances and drops their sensors.
 */
static VOID PhpHwCounterClose(
    VOID
    )
{
    if (PhpHwCounterQuery)
    {
        PerfCloseQueryHandle(PhpHwCounterQuery);
        PhpHwCounterQuery = NULL;
    }

    PhpHwCounterAddedCount = 0;
    PhpHwCounterItemCount = 0;
    PhpHwCounterZoneLimitIndex = ULONG_MAX;

    PhAcquireQueuedLockExclusive(&PhpHwNamedSensorLock);
    PhpHwCounterClearSensorsLocked();
    PhReleaseQueuedLockExclusive(&PhpHwNamedSensorLock);
}

/**
 * Opens the query and adds every counter whose set is registered.
 *
 * \return TRUE when a counter was added.
 */
static BOOLEAN PhpHwCounterOpen(
    VOID
    )
{
    ULONG status;
    ULONG setsAdded = 0;
    ULONG length = ALIGN_UP_BY(sizeof(PERF_COUNTER_IDENTIFIER) + sizeof(PERF_WILDCARD_INSTANCE), 8);
    PPERF_COUNTER_IDENTIFIER identifier;

    PhpHwCounterClose();

    status = PerfOpenQueryHandle(NULL, &PhpHwCounterQuery);

    if (status != ERROR_SUCCESS)
    {
        PhpHwCounterQuery = NULL;
        goto Exit;
    }

    identifier = PhAllocateZero(length);

    for (ULONG i = 0; i < RTL_NUMBER_OF(PhpHwCounters); i++)
    {
        PCPHP_HW_COUNTER counter = &PhpHwCounters[i];

        memset(identifier, 0, length);
        identifier->Size = length;
        identifier->CounterSetGuid = *PhpHwCounterSetGuids[counter->Set];
        identifier->CounterId = counter->CounterId;
        identifier->InstanceId = PERF_WILDCARD_COUNTER;
        memcpy(PTR_ADD_OFFSET(identifier, sizeof(PERF_COUNTER_IDENTIFIER)), PERF_WILDCARD_INSTANCE, sizeof(PERF_WILDCARD_INSTANCE));

        if (PerfAddCounters(PhpHwCounterQuery, identifier, length) == ERROR_SUCCESS && identifier->Status == ERROR_SUCCESS)
            setsAdded |= 1UL << counter->Set;
    }

    PhFree(identifier);

    // Samples list the counters in the order PerfQueryCounterInfo reports, not the order they were added.
    if (setsAdded)
    {
        ULONG infoLength = 0;

        if (PerfQueryCounterInfo(PhpHwCounterQuery, NULL, 0, &infoLength) == ERROR_NOT_ENOUGH_MEMORY && infoLength)
        {
            PPERF_COUNTER_IDENTIFIER info = PhAllocate(infoLength);
            PVOID end = PTR_ADD_OFFSET(info, infoLength);

            if (PerfQueryCounterInfo(PhpHwCounterQuery, info, infoLength, &infoLength) == ERROR_SUCCESS)
            {
                for (identifier = info;
                    (ULONG_PTR)identifier + sizeof(PERF_COUNTER_IDENTIFIER) <= (ULONG_PTR)end &&
                    identifier->Size >= sizeof(PERF_COUNTER_IDENTIFIER) &&
                    PhpHwCounterAddedCount < RTL_NUMBER_OF(PhpHwCounterAdded);
                    identifier = PTR_ADD_OFFSET(identifier, identifier->Size))
                {
                    ULONG index = ULONG_MAX;

                    for (ULONG i = 0; i < RTL_NUMBER_OF(PhpHwCounters); i++)
                    {
                        if (IsEqualGUID(&identifier->CounterSetGuid, PhpHwCounterSetGuids[PhpHwCounters[i].Set]) &&
                            identifier->CounterId == PhpHwCounters[i].CounterId)
                        {
                            index = i;
                            break;
                        }
                    }

                    PhpHwCounterAdded[PhpHwCounterAddedCount++] = index;
                }
            }

            PhFree(info);
        }
    }

    status = PhpHwCounterAddedCount ? ERROR_SUCCESS : ERROR_NOT_FOUND;

    if (!PhpHwCounterAddedCount)
    {
        setsAdded = 0;
        PerfCloseQueryHandle(PhpHwCounterQuery);
        PhpHwCounterQuery = NULL;
    }

Exit:
    PhAcquireQueuedLockExclusive(&PhpHwNamedSensorLock);
    PhpHwCounterStatus = status;
    PhpHwCounterSetsAdded = setsAdded;
    PhReleaseQueuedLockExclusive(&PhpHwNamedSensorLock);

    PhTraceInfo("Hardware sensors: Windows counters 0x%lx, sets 0x%lx", status, setsAdded);

    return !!PhpHwCounterQuery;
}

/**
 * Finds or adds the instance of a set.
 */
static PPHP_HW_COUNTER_ITEM PhpHwCounterFindItem(
    _In_ PHP_HW_COUNTER_SET Set,
    _In_ PCPH_STRINGREF Name
    )
{
    PPHP_HW_COUNTER_ITEM item;

    for (ULONG i = 0; i < PhpHwCounterItemCount; i++)
    {
        PH_STRINGREF itemName;

        item = &PhpHwCounterItems[i];
        PhInitializeStringRefLongHint(&itemName, item->Name);

        if (item->Set == Set && PhEqualStringRef(&itemName, Name, FALSE))
            return item;
    }

    if (PhpHwCounterItemCount >= PHP_HW_COUNTER_MAX_ITEMS || Name->Length / sizeof(WCHAR) >= PH_HW_NAMED_SENSOR_NAME_LENGTH)
        return NULL;

    item = &PhpHwCounterItems[PhpHwCounterItemCount++];
    memset(item, 0, sizeof(PHP_HW_COUNTER_ITEM));
    item->Set = Set;
    memcpy(item->Name, Name->Buffer, Name->Length);
    item->SensorIndex = ULONG_MAX;
    item->LimitIndex = ULONG_MAX;

    return item;
}

/**
 * Stores the instances of one counter of a sample.
 */
static VOID PhpHwCounterReadBlock(
    _In_ PCPHP_HW_COUNTER Counter,
    _In_ PPERF_COUNTER_HEADER Header,
    _In_ PVOID End
    )
{
    PPERF_MULTI_INSTANCES instances;
    PPERF_INSTANCE_HEADER instance;

    if (Header->dwType != PERF_MULTIPLE_INSTANCES)
        return;

    instances = PTR_ADD_OFFSET(Header, sizeof(PERF_COUNTER_HEADER));

    if ((ULONG_PTR)instances + sizeof(PERF_MULTI_INSTANCES) > (ULONG_PTR)End)
        return;

    instance = PTR_ADD_OFFSET(instances, sizeof(PERF_MULTI_INSTANCES));

    for (ULONG i = 0; i < instances->dwInstances; i++)
    {
        PPERF_COUNTER_DATA data;
        PPHP_HW_COUNTER_ITEM item;
        PH_STRINGREF name;
        ULONG64 value;

        if ((ULONG_PTR)instance + sizeof(PERF_INSTANCE_HEADER) > (ULONG_PTR)End || instance->Size < sizeof(PERF_INSTANCE_HEADER) ||
            instance->Size > (ULONG_PTR)End - (ULONG_PTR)instance)
        {
            break;
        }

        data = PTR_ADD_OFFSET(instance, instance->Size);

        if ((ULONG_PTR)data + sizeof(PERF_COUNTER_DATA) > (ULONG_PTR)End || data->dwSize < sizeof(PERF_COUNTER_DATA) ||
            data->dwSize > (ULONG_PTR)End - (ULONG_PTR)data || sizeof(PERF_COUNTER_DATA) + data->dwDataSize > data->dwSize)
        {
            break;
        }

        name.Buffer = PTR_ADD_OFFSET(instance, sizeof(PERF_INSTANCE_HEADER));
        name.Length = wcsnlen(name.Buffer, (instance->Size - sizeof(PERF_INSTANCE_HEADER)) / sizeof(WCHAR)) * sizeof(WCHAR);

        if (data->dwDataSize == sizeof(ULONG64))
            value = *(PULONG64)PTR_ADD_OFFSET(data, sizeof(PERF_COUNTER_DATA));
        else if (data->dwDataSize == sizeof(ULONG))
            value = *(PULONG)PTR_ADD_OFFSET(data, sizeof(PERF_COUNTER_DATA));
        else
            value = 0;

        if (data->dwDataSize && (item = PhpHwCounterFindItem(Counter->Set, &name)))
        {
            item->Values[Counter->Slot] = value;
            item->Sizes[Counter->Slot] = (UCHAR)data->dwDataSize;
            item->Seen |= 1UL << Counter->Slot;
        }

        instance = PTR_ADD_OFFSET(data, data->dwSize);
    }
}

/**
 * Returns the change of a counter since the last sample, modulo its width.
 */
static ULONG64 PhpHwCounterDelta(
    _In_ const PHP_HW_COUNTER_ITEM* Item,
    _In_ UCHAR Slot
    )
{
    ULONG64 delta = Item->Values[Slot] - Item->Previous[Slot];

    return Item->Sizes[Slot] == sizeof(ULONG) ? (ULONG)delta : delta;
}

/**
 * Returns the change of a ratio counter over the change of its base, or FALSE before two samples.
 */
_Success_(return)
static BOOLEAN PhpHwCounterRatio(
    _In_ const PHP_HW_COUNTER_ITEM* Item,
    _In_ UCHAR Slot,
    _Out_ PDOUBLE Value
    )
{
    ULONG mask = 3UL << Slot;
    ULONG64 base;

    if ((Item->Seen & mask) != mask || (Item->PreviousSeen & mask) != mask)
        return FALSE;

    if (!(base = PhpHwCounterDelta(Item, Slot + 1)))
        return FALSE;

    *Value = (DOUBLE)PhpHwCounterDelta(Item, Slot) / (DOUBLE)base;
    return TRUE;
}

/**
 * Maps a Processor Information instance ("group,number") to the flat processor index; ULONG_MAX for totals.
 */
static ULONG PhpHwCounterProcessorIndex(
    _In_ PCWSTR Name
    )
{
    PH_STRINGREF remaining;
    PH_STRINGREF groupText;
    PH_STRINGREF numberText;
    ULONG64 group;
    ULONG64 number;
    ULONG index = 0;

    PhInitializeStringRefLongHint(&remaining, (PWSTR)Name); // read only

    if (!PhSplitStringRefAtChar(&remaining, L',', &groupText, &numberText) ||
        !PhStringToInteger64(&groupText, 10, (PLONG64)&group) ||
        !PhStringToInteger64(&numberText, 10, (PLONG64)&number) ||
        group >= PhSystemProcessorInformation.NumberOfProcessorGroups ||
        number >= PhGetActiveProcessorCount((USHORT)group))
    {
        return ULONG_MAX;
    }

    for (USHORT i = 0; i < (USHORT)group; i++)
        index += PhGetActiveProcessorCount(i);

    return index + (ULONG)number;
}

/**
 * Orders a zone by the number its name ends in (\_SB.TZ2 before \_SB.TZ10); other names follow, by discovery.
 */
static ULONG PhpHwCounterZoneId(
    _In_ PCWSTR Name,
    _In_ ULONG Index
    )
{
    SIZE_T length = PhCountStringZ(Name);
    SIZE_T start = length;
    ULONG number = 0;

    while (start && Name[start - 1] >= L'0' && Name[start - 1] <= L'9')
        start--;

    if (start == length || length - start > 4)
        return 0x10000 + Index;

    for (SIZE_T i = start; i < length; i++)
        number = number * 10 + (Name[i] - L'0');

    return number;
}

/**
 * Orders a RAPL meter by package, then PKG, PP0, PP1, DRAM, PSYS, then RAPL_PackageP_CoreN_CORE by N; other
 * names follow, by discovery.
 */
static ULONG PhpHwCounterMeterId(
    _In_ PCWSTR Name,
    _In_ ULONG Index
    )
{
    static PCWSTR domains[] = { L"PKG", L"PP0", L"PP1", L"DRAM", L"PSYS" };
    PCWSTR name = Name;
    ULONG package = 0;
    ULONG core = 0;

    if (_wcsnicmp(name, L"RAPL_Package", 12))
        return 0x10000 + Index;

    name += 12;

    if (*name < L'0' || *name > L'9')
        return 0x10000 + Index;

    for (; *name >= L'0' && *name <= L'9'; name++)
    {
        package = package * 10 + (*name - L'0');

        if (package > 0xF)
            return 0x10000 + Index;
    }

    if (*name++ != L'_')
        return 0x10000 + Index;

    for (ULONG i = 0; i < RTL_NUMBER_OF(domains); i++)
    {
        if (PhEqualStringZ(name, domains[i], TRUE))
            return package * 0x1000 + i;
    }

    if (_wcsnicmp(name, L"Core", 4))
        return 0x10000 + Index;

    name += 4;

    if (*name < L'0' || *name > L'9')
        return 0x10000 + Index;

    for (; *name >= L'0' && *name <= L'9'; name++)
    {
        core = core * 10 + (*name - L'0');

        if (core > 0xEFF)
            return 0x10000 + Index;
    }

    if (!PhEqualStringZ(name, L"_CORE", TRUE))
        return 0x10000 + Index;

    return package * 0x1000 + 0x100 + core;
}

/**
 * Publishes one sample.
 */
static VOID PhpHwCounterPublish(
    VOID
    )
{
    BOOLEAN zoneValid = FALSE;
    FLOAT zoneMaximum = 0.0f;
    BOOLEAN zoneLimitValid = FALSE;
    FLOAT zoneLimit = 0.0f;

    for (ULONG i = 0; i < PhpHwCounterItemCount; i++)
    {
        PPHP_HW_COUNTER_ITEM item = &PhpHwCounterItems[i];
        BOOLEAN valid = FALSE;
        FLOAT value = 0.0f;
        BOOLEAN limitValid = FALSE;
        FLOAT limit = 0.0f;
        DOUBLE ratio = 0.0;

        switch (item->Set)
        {
        case PhpHwCounterSetZone:
            // 300.0 K is a dummy only until the zone has shown something else; a live zone may read it.
            if ((item->Seen & 1) && item->Values[0] > PHP_HW_COUNTER_ZONE_MINIMUM &&
                item->Values[0] <= PHP_HW_COUNTER_ZONE_MAXIMUM &&
                (item->SensorIndex != ULONG_MAX || item->Values[0] != PHP_HW_COUNTER_ZONE_DUMMY))
            {
                valid = TRUE;
                value = (FLOAT)item->Values[0] / 10.0f - 273.15f;

                if (!zoneValid || value > zoneMaximum)
                    zoneMaximum = value;

                zoneValid = TRUE;

                // One passive limit for all zones: the lowest.
                if ((item->Seen & 2) && (!zoneLimitValid || (FLOAT)item->Values[1] < zoneLimit))
                {
                    zoneLimit = (FLOAT)item->Values[1];
                    zoneLimitValid = TRUE;
                }
            }

            // A zone appears on its first real temperature; placeholder-only zones never do.
            if (valid && item->SensorIndex == ULONG_MAX)
            {
                ULONG id = PhpHwCounterZoneId(item->Name, i);

                // Names that end in the same number (\_TZ.TZ0, \_SB.TZ0, TZ01) keep their discovery order instead.
                for (ULONG j = 0; j < PhpHwCounterItemCount; j++)
                {
                    PPHP_HW_COUNTER_ITEM other = &PhpHwCounterItems[j];

                    if (j != i && other->Set == PhpHwCounterSetZone && other->SensorIndex != ULONG_MAX && other->SensorId == id)
                    {
                        id = 0x10000 + i;
                        break;
                    }
                }

                item->SensorIndex = PhpHwCounterAddSensor(PhHwNamedSensorZoneTemperature, id, item->Name);
                item->SensorId = id;
            }
            break;
        case PhpHwCounterSetProcessor:
            {
                ULONG processor = PhpHwCounterProcessorIndex(item->Name);

                if (processor != ULONG_MAX)
                {
                    valid = PhpHwCounterRatio(item, 0, &ratio) && ratio > 0.0;
                    value = (FLOAT)ratio;

                    if (valid && item->SensorIndex == ULONG_MAX)
                    {
                        WCHAR name[PH_HW_NAMED_SENSOR_NAME_LENGTH];
                        PH_FORMAT format[2];

                        PhInitFormatS(&format[0], L"CPU ");
                        PhInitFormatU(&format[1], processor);

                        if (PhFormatToBuffer(format, RTL_NUMBER_OF(format), name, sizeof(name), NULL))
                            item->SensorIndex = PhpHwCounterAddSensor(PhHwNamedSensorOsClock, processor, name);
                    }
                }
                else if (PhEqualStringZ(item->Name, L"_Total", TRUE))
                {
                    limitValid = !!(item->Seen & 4);
                    limit = (FLOAT)item->Values[2];

                    if (limitValid && item->LimitIndex == ULONG_MAX)
                        item->LimitIndex = PhpHwCounterAddSensor(PhHwNamedSensorOsPerformanceLimit, 0, L"Performance limit");
                }
            }
            break;
        case PhpHwCounterSetMeter:
            if (!PhEqualStringZ(item->Name, L"_Total", TRUE))
            {
                valid = PhpHwCounterRatio(item, 0, &ratio);
                value = (FLOAT)(ratio / 1000.0);

                // A meter appears once it has counted; RAPL DRAM lists on parts that never do.
                if (valid && item->SensorIndex == ULONG_MAX && ratio > 0.0)
                    item->SensorIndex = PhpHwCounterAddSensor(PhHwNamedSensorMeterPower, PhpHwCounterMeterId(item->Name, i), item->Name);
            }
            break;
        }

        PhAcquireQueuedLockExclusive(&PhpHwNamedSensorLock);

        if (item->SensorIndex < PhpHwCounterSensorCount)
        {
            PhpHwCounterSensors[item->SensorIndex].Valid = valid;

            if (valid)
                PhpHwCounterSensors[item->SensorIndex].Value = value;
        }

        if (item->LimitIndex < PhpHwCounterSensorCount)
        {
            PhpHwCounterSensors[item->LimitIndex].Valid = limitValid;

            if (limitValid)
                PhpHwCounterSensors[item->LimitIndex].Value = limit;
        }

        PhReleaseQueuedLockExclusive(&PhpHwNamedSensorLock);

        memcpy(item->Previous, item->Values, sizeof(item->Previous));
        item->PreviousSeen = item->Seen;
    }

    if (zoneLimitValid && PhpHwCounterZoneLimitIndex == ULONG_MAX)
        PhpHwCounterZoneLimitIndex = PhpHwCounterAddSensor(PhHwNamedSensorZonePassiveLimit, 0, L"Passive limit");

    PhAcquireQueuedLockExclusive(&PhpHwNamedSensorLock);

    if (PhpHwCounterZoneLimitIndex < PhpHwCounterSensorCount)
    {
        PhpHwCounterSensors[PhpHwCounterZoneLimitIndex].Valid = zoneLimitValid;

        if (zoneLimitValid)
            PhpHwCounterSensors[PhpHwCounterZoneLimitIndex].Value = zoneLimit;
    }

    PhpHwCounterZoneValid = zoneValid;
    PhpHwCounterZoneMaximum = zoneMaximum;
    PhpHwCounterZoneSeen |= zoneValid;
    PhpHwCounterReportCount = PhpHwCounterItemCount;
    memcpy(PhpHwCounterReport, PhpHwCounterItems, sizeof(PHP_HW_COUNTER_ITEM) * PhpHwCounterItemCount);
    PhpHwCounterReportBlockCount = PhpHwCounterBlockCount;
    memcpy(PhpHwCounterReportBlocks, PhpHwCounterBlocks, sizeof(PHP_HW_COUNTER_BLOCK) * PhpHwCounterBlockCount);

    PhReleaseQueuedLockExclusive(&PhpHwNamedSensorLock);
}

/**
 * Takes one sample of the counters.
 */
static VOID PhpHwCounterSample(
    VOID
    )
{
    static ULONG bufferSize = 0x4000;
    PPERF_DATA_HEADER buffer;
    ULONG returnSize = 0;
    ULONG status;
    PPERF_COUNTER_HEADER header;
    PVOID end;

    buffer = PhAllocate(bufferSize);
    status = PerfQueryCounterData(PhpHwCounterQuery, buffer, bufferSize, &returnSize);

    if (status == ERROR_NOT_ENOUGH_MEMORY && returnSize > bufferSize)
    {
        PhFree(buffer);
        bufferSize = returnSize;
        buffer = PhAllocate(bufferSize);
        status = PerfQueryCounterData(PhpHwCounterQuery, buffer, bufferSize, &returnSize);
    }

    for (ULONG i = 0; i < PhpHwCounterItemCount; i++)
        PhpHwCounterItems[i].Seen = 0;

    PhpHwCounterBlockCount = 0;

    if (status == ERROR_SUCCESS && returnSize >= sizeof(PERF_DATA_HEADER) && returnSize <= bufferSize)
    {
        end = PTR_ADD_OFFSET(buffer, returnSize);
        header = PTR_ADD_OFFSET(buffer, sizeof(PERF_DATA_HEADER));

        for (ULONG i = 0; i < buffer->dwNumCounters && i < PhpHwCounterAddedCount; i++)
        {
            PPHP_HW_COUNTER_BLOCK block;

            if ((ULONG_PTR)header + sizeof(PERF_COUNTER_HEADER) > (ULONG_PTR)end || header->dwSize < sizeof(PERF_COUNTER_HEADER) ||
                header->dwSize > (ULONG_PTR)end - (ULONG_PTR)header)
            {
                break;
            }

            block = &PhpHwCounterBlocks[PhpHwCounterBlockCount++];
            block->Counter = PhpHwCounterAdded[i];
            block->Status = header->dwStatus;
            block->Type = header->dwType;
            block->Instances = 0;

            if (header->dwType == PERF_MULTIPLE_INSTANCES && header->dwSize >= sizeof(PERF_COUNTER_HEADER) + sizeof(PERF_MULTI_INSTANCES))
                block->Instances = ((PPERF_MULTI_INSTANCES)PTR_ADD_OFFSET(header, sizeof(PERF_COUNTER_HEADER)))->dwInstances;

            if (header->dwStatus == ERROR_SUCCESS && PhpHwCounterAdded[i] < RTL_NUMBER_OF(PhpHwCounters))
                PhpHwCounterReadBlock(&PhpHwCounters[PhpHwCounterAdded[i]], header, PTR_ADD_OFFSET(header, header->dwSize));

            header = PTR_ADD_OFFSET(header, header->dwSize);
        }
    }

    PhFree(buffer);

    if (PhpHwCounterStatus != status)
    {
        PhAcquireQueuedLockExclusive(&PhpHwNamedSensorLock);
        PhpHwCounterStatus = status;
        PhReleaseQueuedLockExclusive(&PhpHwNamedSensorLock);
    }

    PhpHwCounterPublish();
}

/**
 * Opens, samples or closes the Windows counters. Called on every update; the setting alone enables them.
 *
 * \param Enabled Whether hardware sensors are enabled in the settings.
 */
VOID PhpHwUpdateCounters(
    _In_ BOOLEAN Enabled
    )
{
    static BOOLEAN started = FALSE;
    static ULONG retryTicks = 0;

    if (!Enabled)
    {
        if (started)
        {
            PhpHwCounterClose();
            started = FALSE;
        }

        return;
    }

    // A failed open is tried again every PHP_HW_COUNTER_RETRY_TICKS updates.
    if (!started || (!PhpHwCounterQuery && ++retryTicks >= PHP_HW_COUNTER_RETRY_TICKS))
    {
        started = TRUE;
        retryTicks = 0;
        PhpHwCounterOpen();
    }

    if (PhpHwCounterQuery)
        PhpHwCounterSample();
}

/**
 * Retrieves the hottest current ACPI thermal zone, the package temperature fallback on ARM64.
 *
 * \param Celsius Receives the temperature.
 * \return TRUE when a zone has a current temperature.
 */
_Success_(return)
BOOLEAN PhpHwCounterHottestZone(
    _Out_ PFLOAT Celsius
    )
{
    BOOLEAN valid;

    PhAcquireQueuedLockShared(&PhpHwNamedSensorLock);

    valid = PhpHwCounterZoneValid;

    if (valid)
        *Celsius = PhpHwCounterZoneMaximum;

    PhReleaseQueuedLockShared(&PhpHwNamedSensorLock);

    return valid;
}

/**
 * Determines whether a zone had a temperature since the query opened; the ARM64 package capability stays set across
 * a missed sample.
 */
BOOLEAN PhpHwCounterZoneEverSeen(
    VOID
    )
{
    BOOLEAN seen;

    PhAcquireQueuedLockShared(&PhpHwNamedSensorLock);
    seen = PhpHwCounterZoneSeen;
    PhReleaseQueuedLockShared(&PhpHwNamedSensorLock);

    return seen;
}

/**
 * Determines whether a Windows counter sensor is listed.
 *
 * \return TRUE when the counters publish a sensor.
 */
BOOLEAN NTAPI PhHwCounterSensorsPresent(
    VOID
    )
{
    BOOLEAN present = FALSE;

    PhAcquireQueuedLockShared(&PhpHwNamedSensorLock);

    for (ULONG i = 0; i < PhpHwCounterSensorCount; i++)
    {
        if (PhpHwCounterSensors[i].Active)
        {
            present = TRUE;
            break;
        }
    }

    PhReleaseQueuedLockShared(&PhpHwNamedSensorLock);

    return present;
}

/**
 * Appends the Windows counter state to the hardware report: the raw values of every zone and meter, and the
 * processor limits.
 *
 * \param Builder The report.
 */
VOID PhpHwWriteCounterDiagnostics(
    _Inout_ PPH_STRING_BUILDER Builder
    )
{
    static CONST PCWSTR setNames[PhpHwCounterSetMaximum] = { L"thermal zones", L"processors", L"energy meters" };
    PPHP_HW_COUNTER_ITEM items = NULL;
    PHP_HW_COUNTER_BLOCK blocks[RTL_NUMBER_OF(PhpHwCounters)];
    ULONG count;
    ULONG blockCount;
    ULONG status;
    ULONG sets;

    PhAcquireQueuedLockShared(&PhpHwNamedSensorLock);
    count = PhpHwCounterReportCount;
    blockCount = PhpHwCounterReportBlockCount;
    status = PhpHwCounterStatus;
    sets = PhpHwCounterSetsAdded;

    if (count)
    {
        items = PhAllocate(sizeof(PHP_HW_COUNTER_ITEM) * count);
        memcpy(items, PhpHwCounterReport, sizeof(PHP_HW_COUNTER_ITEM) * count);
    }

    memcpy(blocks, PhpHwCounterReportBlocks, sizeof(PHP_HW_COUNTER_BLOCK) * blockCount);

    PhReleaseQueuedLockShared(&PhpHwNamedSensorLock);

    PhAppendFormatStringBuilder(Builder, L"Windows counters: status 0x%08lx, sets", status);

    for (ULONG i = 0; i < PhpHwCounterSetMaximum; i++)
    {
        if (sets & (1UL << i))
            PhAppendFormatStringBuilder(Builder, L" %s", setNames[i]);
    }

    if (!sets)
        PhAppendStringBuilder2(Builder, L" none");

    PhAppendCharStringBuilder(Builder, L'\n');

    for (ULONG i = 0; i < blockCount; i++)
    {
        PPHP_HW_COUNTER_BLOCK block = &blocks[i];

        if (block->Counter < RTL_NUMBER_OF(PhpHwCounters))
        {
            PhAppendFormatStringBuilder(
                Builder,
                L"  Counter %s %lu: status 0x%08lx, type %lu, %lu instances\n",
                setNames[PhpHwCounters[block->Counter].Set],
                PhpHwCounters[block->Counter].CounterId,
                block->Status,
                block->Type,
                block->Instances
                );
        }
        else
        {
            PhAppendFormatStringBuilder(Builder, L"  Counter unknown: status 0x%08lx, type %lu, %lu instances\n", block->Status, block->Type, block->Instances);
        }
    }

    for (ULONG i = 0; i < count; i++)
    {
        PPHP_HW_COUNTER_ITEM item = &items[i];

        switch (item->Set)
        {
        case PhpHwCounterSetZone:
            PhAppendFormatStringBuilder(
                Builder,
                L"  Zone %s: %I64u tenths K, passive limit %I64u%%, throttle reasons 0x%I64x\n",
                item->Name,
                item->Values[0],
                item->Values[1],
                item->Values[2]
                );
            break;
        case PhpHwCounterSetProcessor:
            if (PhEqualStringZ(item->Name, L"_Total", TRUE))
            {
                PhAppendFormatStringBuilder(
                    Builder,
                    L"  Processors: performance limit %I64u%%, limit flags 0x%I64x\n",
                    item->Values[2],
                    item->Values[3]
                    );
            }
            break;
        case PhpHwCounterSetMeter:
            PhAppendFormatStringBuilder(Builder, L"  Meter %s: power %I64u over %I64u\n", item->Name, item->Values[0], item->Values[1]);
            break;
        }
    }

    if (items)
        PhFree(items);
}
