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
#include <SetupAPI.h>
#include <trace.h>

// IOCTL_THERMAL_QUERY_INFORMATION and the THERMAL_INFORMATION_EX that ACPI.sys returns (92 bytes; poclass.h has only the
// older layout). A stamp equal to the zone's current one holds the request until the zone changes.
#define PHP_HW_IOCTL_THERMAL_QUERY_INFORMATION CTL_CODE(FILE_DEVICE_BATTERY, 0x20, METHOD_BUFFERED, FILE_READ_ACCESS)
#define PHP_HW_THERMAL_STAMP_NONE 0xFFFFFFFF

typedef struct _PHP_HW_THERMAL_INFORMATION_EX
{
    ULONG ThermalStamp;
    ULONG ThermalConstant1;
    ULONG ThermalConstant2;
    GROUP_AFFINITY Processors;
    ULONG SamplingPeriod;
    ULONG CurrentTemperature;
    ULONG PassiveTripPoint;
    ULONG CriticalTripPoint;
    UCHAR ActiveTripPointCount;
    ULONG ActiveTripPoint[10];
} PHP_HW_THERMAL_INFORMATION_EX, *PPHP_HW_THERMAL_INFORMATION_EX;
#ifdef _WIN64
C_ASSERT(FIELD_OFFSET(PHP_HW_THERMAL_INFORMATION_EX, PassiveTripPoint) == 40);
C_ASSERT(FIELD_OFFSET(PHP_HW_THERMAL_INFORMATION_EX, ActiveTripPoint) + sizeof(ULONG) * 10 == 92);
#endif

// Collector counters: the Windows performance counters (Perflib V2) of ACPI thermal zones, energy meters and processor
// limits. Sampled on the provider thread once per update; the setting alone enables them, no driver.

#define PHP_HW_COUNTER_MAX_ITEMS 256
#define PHP_HW_COUNTER_MAX_SENSORS (PHP_HW_COUNTER_MAX_ITEMS * 2)
#define PHP_HW_COUNTER_VALUES 4
#define PHP_HW_COUNTER_ZONE_MINIMUM 2732 // tenths of a kelvin; at or below 0 C is a placeholder (0, 2330 = -40 C)
#define PHP_HW_COUNTER_ZONE_MAXIMUM 4500 // tenths of a kelvin; above 177 C is not a temperature
#define PHP_HW_COUNTER_ZONE_DUMMY 3000 // 300.0 K, the constant that firmware dummy zones return
#define PHP_HW_COUNTER_RETRY_TICKS 30 // updates between attempts to open the query after a failure
#define PHP_HW_COUNTER_NIC_ID_BASE 0x20000 // network adapter temperatures: this plus the item index
#define PHP_HW_COUNTER_NIC_MAXIMUM 150 // degrees Celsius; 0 or above this is not a reading

typedef enum _PHP_HW_COUNTER_SET
{
    PhpHwCounterSetZone, // Thermal Zone Information
    PhpHwCounterSetProcessor, // Processor Information
    PhpHwCounterSetMeter, // Energy Meter
    PhpHwCounterSetNic, // Mellanox WinOF-2 Device Diagnostic; found by name
    PhpHwCounterSetMaximum
} PHP_HW_COUNTER_SET;

// {52BC5412-DAC2-449C-8BC2-96443888FE6B}
static CONST GUID PhpHwZoneCounterSet = { 0x52bc5412, 0xdac2, 0x449c, { 0x8b, 0xc2, 0x96, 0x44, 0x38, 0x88, 0xfe, 0x6b } };
// {B4FC721A-0378-476F-89BA-A5A79F810B36}
static CONST GUID PhpHwProcessorCounterSet = { 0xb4fc721a, 0x0378, 0x476f, { 0x89, 0xba, 0xa5, 0xa7, 0x9f, 0x81, 0x0b, 0x36 } };
// {06EBF20D-17FB-4338-A08D-7A99F17CA678}
static CONST GUID PhpHwMeterCounterSet = { 0x06ebf20d, 0x17fb, 0x4338, { 0xa0, 0x8d, 0x7a, 0x99, 0xf1, 0x7c, 0xa6, 0x78 } };
// Registered without a published GUID; looked up by name when the query opens.
static GUID PhpHwNicCounterSet = { 0 };
static ULONG PhpHwNicCounterId = ULONG_MAX; // ULONG_MAX when the set or its temperature counter is not registered

static CONST GUID* CONST PhpHwCounterSetGuids[PhpHwCounterSetMaximum] =
{
    &PhpHwZoneCounterSet,
    &PhpHwProcessorCounterSet,
    &PhpHwMeterCounterSet,
    &PhpHwNicCounterSet
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
    { PhpHwCounterSetNic, ULONG_MAX, 0 }, // Nic temperature in Celsius degrees unit; the ID is PhpHwNicCounterId
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
    ULONG TripIndex[2]; // a zone's passive and critical trip point sensors; ULONG_MAX when none
} PHP_HW_COUNTER_ITEM, *PPHP_HW_COUNTER_ITEM;

// A thermal zone's trip points, read once a query from its device interface.
typedef struct _PHP_HW_COUNTER_TRIP
{
    WCHAR Name[PH_HW_NAMED_SENSOR_NAME_LENGTH]; // the zone's BIOS device name
    ULONG Trip[2]; // passive and critical, tenths of a kelvin; 0 when none
} PHP_HW_COUNTER_TRIP, *PPHP_HW_COUNTER_TRIP;

#define PHP_HW_COUNTER_MAX_TRIPS 32

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
static PHP_HW_COUNTER_TRIP PhpHwCounterTrips[PHP_HW_COUNTER_MAX_TRIPS];
static ULONG PhpHwCounterTripCount = 0;
static BOOLEAN PhpHwCounterTripsRead = FALSE; // since the query opened
// Under PhpHwNamedSensorLock: interfaces, opened, read, read at the exact length, short replies; the last failure.
static ULONG PhpHwCounterTripReport[6] = { 0 };

// Under PhpHwNamedSensorLock.
ULONG PhpHwCounterSensorCount = 0;
PH_HW_NAMED_SENSOR PhpHwCounterSensors[PHP_HW_COUNTER_MAX_SENSORS];
static BOOLEAN PhpHwCounterZoneValid = FALSE; // a zone has a current temperature
static BOOLEAN PhpHwCounterZoneSeen = FALSE; // a zone had a temperature since the query opened
static FLOAT PhpHwCounterZoneMaximum = 0.0f; // the hottest current zone, degrees Celsius
static BOOLEAN PhpHwCounterClusterValid = FALSE; // a CPU_CLUSTER energy meter has a current reading
static FLOAT PhpHwCounterClusterWatts = 0.0f; // the CPU_CLUSTER energy meters together, watts
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
    PhpHwCounterClusterValid = FALSE;
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
    PhpHwCounterTripCount = 0;
    PhpHwCounterTripsRead = FALSE;

    PhAcquireQueuedLockExclusive(&PhpHwNamedSensorLock);
    PhpHwCounterClearSensorsLocked();
    PhReleaseQueuedLockExclusive(&PhpHwNamedSensorLock);
}

/**
 * Returns the ID of a counter in its set; ULONG_MAX when it is not registered.
 */
static ULONG PhpHwCounterId(
    _In_ PCPHP_HW_COUNTER Counter
    )
{
    return Counter->Set == PhpHwCounterSetNic ? PhpHwNicCounterId : Counter->CounterId;
}

/**
 * Queries registration information of a counter set, growing the buffer as needed.
 *
 * \return TRUE when the information is in the buffer; Length receives its size.
 */
static BOOLEAN PhpHwCounterQueryRegistration(
    _In_ PCGUID Set,
    _In_ PerfRegInfoType Request,
    _Inout_ PVOID* Buffer,
    _Inout_ PULONG BufferLength,
    _Out_ PULONG Length
    )
{
    ULONG status;
    ULONG length = 0;

    *Length = 0;
    status = PerfQueryCounterSetRegistrationInfo(NULL, Set, Request, 0, *Buffer, *BufferLength, &length);

    if (status == ERROR_NOT_ENOUGH_MEMORY && length > *BufferLength)
    {
        PhFree(*Buffer);
        *BufferLength = length;
        *Buffer = PhAllocate(length);
        status = PerfQueryCounterSetRegistrationInfo(NULL, Set, Request, 0, *Buffer, *BufferLength, &length);
    }

    if (status != ERROR_SUCCESS || length > *BufferLength)
        return FALSE;

    *Length = length;
    return TRUE;
}

/**
 * Finds the network adapter diagnostic set and its temperature counter by their English names.
 */
static VOID PhpHwCounterFindNicSet(
    VOID
    )
{
    static CONST PH_STRINGREF setName = PH_STRINGREF_INIT(L"Mellanox WinOF-2 Device Diagnostic");
    static CONST PH_STRINGREF counterName = PH_STRINGREF_INIT(L"Nic temperature in Celsius degrees unit");
    ULONG count = 0;
    ULONG setsLength;
    PGUID sets;
    PVOID buffer;
    ULONG bufferLength = 0x1000;
    ULONG length;

    memset(&PhpHwNicCounterSet, 0, sizeof(GUID));
    PhpHwNicCounterId = ULONG_MAX;

    if (PerfEnumerateCounterSet(NULL, NULL, 0, &count) != ERROR_NOT_ENOUGH_MEMORY || !count)
        return;
    if (!NT_SUCCESS(RtlULongMult(count, sizeof(GUID), &setsLength)))
        return;

    sets = PhAllocate(setsLength);
    buffer = PhAllocate(bufferLength);

    if (PerfEnumerateCounterSet(NULL, sets, count, &count) == ERROR_SUCCESS && count <= setsLength / sizeof(GUID))
    {
        for (ULONG i = 0; i < count; i++)
        {
            PH_STRINGREF name;
            PPERF_STRING_BUFFER_HEADER header;
            PPERF_STRING_COUNTER_HEADER counters;

            if (!PhpHwCounterQueryRegistration(&sets[i], PERF_REG_COUNTERSET_ENGLISH_NAME, &buffer, &bufferLength, &length))
                continue;

            name.Buffer = buffer;
            name.Length = wcsnlen(name.Buffer, length / sizeof(WCHAR)) * sizeof(WCHAR);

            if (!PhEqualStringRef(&name, &setName, TRUE))
                continue;

            if (PhpHwCounterQueryRegistration(&sets[i], PERF_REG_COUNTER_ENGLISH_NAMES, &buffer, &bufferLength, &length) &&
                length >= sizeof(PERF_STRING_BUFFER_HEADER))
            {
                header = buffer;
                counters = PTR_ADD_OFFSET(header, sizeof(PERF_STRING_BUFFER_HEADER));

                for (ULONG j = 0; j < header->dwCounters && j < (length - sizeof(PERF_STRING_BUFFER_HEADER)) / sizeof(PERF_STRING_COUNTER_HEADER); j++)
                {
                    ULONG offset = counters[j].dwOffset;

                    if (offset >= length)
                        continue;

                    name.Buffer = PTR_ADD_OFFSET(buffer, offset);
                    name.Length = wcsnlen(name.Buffer, (length - offset) / sizeof(WCHAR)) * sizeof(WCHAR);

                    if (PhEqualStringRef(&name, &counterName, TRUE))
                    {
                        PhpHwNicCounterSet = sets[i];
                        PhpHwNicCounterId = counters[j].dwCounterId;
                        break;
                    }
                }
            }

            break;
        }
    }

    PhFree(buffer);
    PhFree(sets);
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

    PhpHwCounterFindNicSet();
    identifier = PhAllocateZero(length);

    for (ULONG i = 0; i < RTL_NUMBER_OF(PhpHwCounters); i++)
    {
        PCPHP_HW_COUNTER counter = &PhpHwCounters[i];

        if (PhpHwCounterId(counter) == ULONG_MAX)
            continue;

        memset(identifier, 0, length);
        identifier->Size = length;
        identifier->CounterSetGuid = *PhpHwCounterSetGuids[counter->Set];
        identifier->CounterId = PhpHwCounterId(counter);
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
                            identifier->CounterId == PhpHwCounterId(&PhpHwCounters[i]))
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
    item->TripIndex[0] = ULONG_MAX;
    item->TripIndex[1] = ULONG_MAX;
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
 * Compares two ACPI namespace names, ignoring each segment's trailing underscore padding (\_TZ_.TZ0_ is \_TZ.TZ0).
 */
static BOOLEAN PhpHwCounterSameZone(
    _In_ PCWSTR Name1,
    _In_ PCWSTR Name2
    )
{
    while (TRUE)
    {
        while (*Name1 == L'_' && (Name1[1] == L'.' || !Name1[1]))
            Name1++;
        while (*Name2 == L'_' && (Name2[1] == L'.' || !Name2[1]))
            Name2++;

        if (towupper(*Name1) != towupper(*Name2))
            return FALSE;
        if (!*Name1)
            return TRUE;

        Name1++;
        Name2++;
    }
}

/**
 * Reads the passive and critical trip points of every thermal zone from its device interface
 * (IOCTL_THERMAL_QUERY_INFORMATION), once a query.
 */
static VOID PhpHwCounterReadTrips(
    VOID
    )
{
    // GUID_DEVICE_THERMAL_ZONE
    static CONST GUID thermalZone = { 0x4afa3d51, 0x74a7, 0x11d0, { 0xbe, 0x5e, 0x00, 0xa0, 0xc9, 0x06, 0x28, 0x57 } };
    HDEVINFO deviceInfoSet;
    SP_DEVICE_INTERFACE_DATA interfaceData;
    ULONG report[6] = { 0 };

    PhpHwCounterTripsRead = TRUE;
    PhpHwCounterTripCount = 0;

    deviceInfoSet = SetupDiGetClassDevsW(&thermalZone, NULL, NULL, DIGCF_PRESENT | DIGCF_DEVICEINTERFACE);

    if (deviceInfoSet == INVALID_HANDLE_VALUE)
    {
        report[5] = PhGetLastWin32ErrorAsNtStatus();
        PhAcquireQueuedLockExclusive(&PhpHwNamedSensorLock);
        memcpy(PhpHwCounterTripReport, report, sizeof(report));
        PhReleaseQueuedLockExclusive(&PhpHwNamedSensorLock);
        return;
    }

    memset(&interfaceData, 0, sizeof(SP_DEVICE_INTERFACE_DATA));
    interfaceData.cbSize = sizeof(SP_DEVICE_INTERFACE_DATA);

    for (ULONG i = 0; PhpHwCounterTripCount < PHP_HW_COUNTER_MAX_TRIPS && SetupDiEnumDeviceInterfaces(deviceInfoSet, NULL, &thermalZone, i, &interfaceData); i++)
    {
        NTSTATUS status;

        UCHAR detailBuffer[sizeof(SP_DEVICE_INTERFACE_DETAIL_DATA_W) + 512 * sizeof(WCHAR)];
        PSP_DEVICE_INTERFACE_DETAIL_DATA_W detail = (PSP_DEVICE_INTERFACE_DETAIL_DATA_W)detailBuffer;
        SP_DEVINFO_DATA deviceInfoData;
        DEVPROPTYPE propertyType;
        WCHAR name[256];
        HANDLE handle;
        PHP_HW_THERMAL_INFORMATION_EX thermal;
        ULONG returnLength = 0;
        PPHP_HW_COUNTER_TRIP trip;

        memset(detailBuffer, 0, sizeof(detailBuffer));
        detail->cbSize = sizeof(SP_DEVICE_INTERFACE_DETAIL_DATA_W);
        memset(&deviceInfoData, 0, sizeof(SP_DEVINFO_DATA));
        deviceInfoData.cbSize = sizeof(SP_DEVINFO_DATA);
        ULONG exact;

        report[0]++;

        if (!SetupDiGetDeviceInterfaceDetailW(deviceInfoSet, &interfaceData, detail, sizeof(detailBuffer), NULL, &deviceInfoData))
        {
            report[5] = PhGetLastWin32ErrorAsNtStatus();
            continue;
        }
        if (!SetupDiGetDevicePropertyW(deviceInfoSet, &deviceInfoData, &DEVPKEY_Device_BiosDeviceName, &propertyType, (PBYTE)name, sizeof(name) - sizeof(WCHAR), NULL, 0) || propertyType != DEVPROP_TYPE_STRING)
        {
            report[5] = STATUS_OBJECT_NAME_NOT_FOUND;
            continue;
        }

        name[RTL_NUMBER_OF(name) - 1] = UNICODE_NULL;

        if (!NT_SUCCESS(status = PhCreateFileWin32(
            &handle,
            detail->DevicePath,
            FILE_READ_DATA | SYNCHRONIZE,
            FILE_ATTRIBUTE_NORMAL,
            FILE_SHARE_READ | FILE_SHARE_WRITE,
            FILE_OPEN,
            FILE_NON_DIRECTORY_FILE | FILE_SYNCHRONOUS_IO_NONALERT
            )))
        {
            report[5] = status;
            continue;
        }

        report[1]++;
        memset(&thermal, 0, sizeof(PHP_HW_THERMAL_INFORMATION_EX));
        thermal.ThermalStamp = PHP_HW_THERMAL_STAMP_NONE;

        status = PhDeviceIoControlFile(handle, PHP_HW_IOCTL_THERMAL_QUERY_INFORMATION, &thermal, sizeof(thermal), &thermal, sizeof(thermal), &returnLength);

        // A driver that wants the structure's exact size (92 bytes, not the padded sizeof) is asked again.
        if (!NT_SUCCESS(status) || returnLength < RTL_SIZEOF_THROUGH_FIELD(PHP_HW_THERMAL_INFORMATION_EX, CriticalTripPoint))
        {
            exact = RTL_SIZEOF_THROUGH_FIELD(PHP_HW_THERMAL_INFORMATION_EX, ActiveTripPoint);
            memset(&thermal, 0, sizeof(PHP_HW_THERMAL_INFORMATION_EX));
            thermal.ThermalStamp = PHP_HW_THERMAL_STAMP_NONE;
            returnLength = 0;
            status = PhDeviceIoControlFile(handle, PHP_HW_IOCTL_THERMAL_QUERY_INFORMATION, &thermal, exact, &thermal, exact, &returnLength);

            if (NT_SUCCESS(status) && returnLength >= RTL_SIZEOF_THROUGH_FIELD(PHP_HW_THERMAL_INFORMATION_EX, CriticalTripPoint))
                report[3]++;
        }

        returnLength = 0;
        if (!NT_SUCCESS(status))
        {
            report[5] = status;
        }
        else if (returnLength < RTL_SIZEOF_THROUGH_FIELD(PHP_HW_THERMAL_INFORMATION_EX, CriticalTripPoint))
        {
            // A zone a vendor driver owns (Snapdragon) can complete it with no data.
            report[4]++;
        }
        else
        {
            report[2]++;
            trip = &PhpHwCounterTrips[PhpHwCounterTripCount++];
            PhCopyStringZ(name, SIZE_MAX, trip->Name, RTL_NUMBER_OF(trip->Name), NULL);
            trip->Trip[0] = thermal.PassiveTripPoint;
            trip->Trip[1] = thermal.CriticalTripPoint;
        }

        NtClose(handle);
    }

    SetupDiDestroyDeviceInfoList(deviceInfoSet);

    PhAcquireQueuedLockExclusive(&PhpHwNamedSensorLock);
    memcpy(PhpHwCounterTripReport, report, sizeof(report));
    PhReleaseQueuedLockExclusive(&PhpHwNamedSensorLock);
}

/**
 * Determines whether a thermal zone is an MPTF power participant (\_TZ.PSOC, \_TZ.PSYS), whose temperature counter
 * carries milliwatts.
 */
static BOOLEAN PhpHwCounterZoneIsPower(
    _In_ PCWSTR Name
    )
{
    SIZE_T length = wcslen(Name);

    return length >= 5 && (_wcsicmp(Name + length - 5, L".PSOC") == 0 || _wcsicmp(Name + length - 5, L".PSYS") == 0);
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
    BOOLEAN clusterValid = FALSE;
    FLOAT clusterWatts = 0.0f;

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
            if (PhpHwCounterZoneIsPower(item->Name))
            {
                valid = !!(item->Seen & 1);
                value = (FLOAT)item->Values[0] / 1000.0f;

                if (valid && item->SensorIndex == ULONG_MAX)
                    item->SensorIndex = PhpHwCounterAddSensor(PhHwNamedSensorMeterPower, PhpHwCounterMeterId(item->Name, i), item->Name);
                break;
            }

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

                if (!PhpHwCounterTripsRead)
                    PhpHwCounterReadTrips();

                for (ULONG t = 0; t < PhpHwCounterTripCount; t++)
                {
                    static CONST PCWSTR suffixes[2] = { L" passive", L" critical" };

                    if (!PhpHwCounterSameZone(PhpHwCounterTrips[t].Name, item->Name))
                        continue;

                    for (ULONG k = 0; k < 2; k++)
                    {
                        WCHAR tripName[PH_HW_NAMED_SENSOR_NAME_LENGTH];
                        PH_FORMAT format[2];

                        // A trip point below 0 C or the zone maximum is firmware's "none".
                        if (PhpHwCounterTrips[t].Trip[k] <= 2732 || PhpHwCounterTrips[t].Trip[k] > PHP_HW_COUNTER_ZONE_MAXIMUM)
                            continue;

                        PhInitFormatS(&format[0], item->Name);
                        PhInitFormatS(&format[1], suffixes[k]);
                        PhFormatToBuffer(format, RTL_NUMBER_OF(format), tripName, sizeof(tripName), NULL);
                        item->TripIndex[k] = PhpHwCounterAddSensor(PhHwNamedSensorZoneTrip, id * 2 + k, tripName);

                        PhAcquireQueuedLockExclusive(&PhpHwNamedSensorLock);

                        if (item->TripIndex[k] < PhpHwCounterSensorCount)
                        {
                            PhpHwCounterSensors[item->TripIndex[k]].Valid = TRUE;
                            PhpHwCounterSensors[item->TripIndex[k]].Value = (FLOAT)PhpHwCounterTrips[t].Trip[k] / 10.0f - 273.15f;
                        }

                        PhReleaseQueuedLockExclusive(&PhpHwNamedSensorLock);
                    }

                    break;
                }
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
                PH_STRINGREF meterName;

                valid = PhpHwCounterRatio(item, 0, &ratio);
                value = (FLOAT)(ratio / 1000.0);

                // The Snapdragon cluster rails, the processor's power on ARM64 (no RAPL there).
                PhInitializeStringRefLongHint(&meterName, item->Name);

                if (valid && PhStartsWithStringRef2(&meterName, L"CPU_CLUSTER", TRUE))
                {
                    clusterWatts += value;
                    clusterValid = TRUE;
                }

                // A meter appears once it has counted; RAPL DRAM lists on parts that never do.
                if (valid && item->SensorIndex == ULONG_MAX && ratio > 0.0)
                    item->SensorIndex = PhpHwCounterAddSensor(PhHwNamedSensorMeterPower, PhpHwCounterMeterId(item->Name, i), item->Name);
            }
            break;
        case PhpHwCounterSetNic:
            valid = (item->Seen & 1) && item->Values[0] && item->Values[0] <= PHP_HW_COUNTER_NIC_MAXIMUM;
            value = (FLOAT)item->Values[0];

            if (valid && item->SensorIndex == ULONG_MAX)
                item->SensorIndex = PhpHwCounterAddSensor(PhHwNamedSensorZoneTemperature, PHP_HW_COUNTER_NIC_ID_BASE + i, item->Name);
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
    PhpHwCounterClusterValid = clusterValid;
    PhpHwCounterClusterWatts = clusterWatts;
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
 * Retrieves the CPU_CLUSTER energy meters together, the processor power fallback on ARM64.
 *
 * \param Watts Receives the power.
 * \return TRUE when a cluster meter has a current reading.
 */
_Success_(return)
BOOLEAN PhpHwCounterClusterPower(
    _Out_ PFLOAT Watts
    )
{
    BOOLEAN valid;

    PhAcquireQueuedLockShared(&PhpHwNamedSensorLock);

    valid = PhpHwCounterClusterValid;

    if (valid)
        *Watts = PhpHwCounterClusterWatts;

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
    static CONST PCWSTR setNames[PhpHwCounterSetMaximum] = { L"thermal zones", L"processors", L"energy meters", L"network adapters" };
    PPHP_HW_COUNTER_ITEM items = NULL;
    PHP_HW_COUNTER_BLOCK blocks[RTL_NUMBER_OF(PhpHwCounters)];
    ULONG count;
    ULONG blockCount;
    ULONG status;
    ULONG sets;
    ULONG trips[6];

    PhAcquireQueuedLockShared(&PhpHwNamedSensorLock);
    memcpy(trips, PhpHwCounterTripReport, sizeof(trips));
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

    if (PhpHwCounterTripsRead)
    {
        PhAppendFormatStringBuilder(
            Builder,
            L"  Trip points: %lu zone interfaces, %lu opened, %lu read (%lu at 92 bytes), %lu short replies; last failure 0x%08lx\n",
            trips[0],
            trips[1],
            trips[2],
            trips[3],
            trips[4],
            trips[5]
            );
    }

    for (ULONG i = 0; i < blockCount; i++)
    {
        PPHP_HW_COUNTER_BLOCK block = &blocks[i];

        if (block->Counter < RTL_NUMBER_OF(PhpHwCounters))
        {
            PhAppendFormatStringBuilder(
                Builder,
                L"  Counter %s %lu: status 0x%08lx, type %lu, %lu instances\n",
                setNames[PhpHwCounters[block->Counter].Set],
                PhpHwCounterId(&PhpHwCounters[block->Counter]),
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
        case PhpHwCounterSetNic:
            PhAppendFormatStringBuilder(Builder, L"  Network adapter %s: %I64u C\n", item->Name, item->Values[0]);
            break;
        }
    }

    if (items)
        PhFree(items);
}
