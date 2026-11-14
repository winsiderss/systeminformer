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

#include "agenttools.h"

static CONST PCSTR AtpHwComponentKinds[] =
{
    "cpu",
    "memory",
    "chipset",
    "system",
    "windows",
    "source",
};
C_ASSERT(RTL_NUMBER_OF(AtpHwComponentKinds) == PhHwComponentMaximum);

static CONST PCSTR AtpHwGroups[] =
{
    "temperatures",
    "powers",
    "limits",
    "clocks",
    "bandwidth",
    "active_clocks",
    "multipliers",
    "voltages",
    "currents",
    "residency",
    "fans",
    "controls",
    "throttling",
};
C_ASSERT(RTL_NUMBER_OF(AtpHwGroups) == PhHwGroupMaximum);

static CONST PCSTR AtpHwUnits[] =
{
    "celsius",
    "watts",
    "mhz",
    "mt_per_s",
    "multiplier",
    "volts",
    "throttle",
    "mb_per_s",
    "gear",
    "amperes",
    "rpm",
    "percent",
};
C_ASSERT(RTL_NUMBER_OF(AtpHwUnits) == PhHwUnitMaximum);

static CONST ULONG AtpHwThrottleFlags[] =
{
    PH_HW_THROTTLE_THERMAL,
    PH_HW_THROTTLE_PROCHOT,
    PH_HW_THROTTLE_CRITICAL,
    PH_HW_THROTTLE_POWER_LIMIT,
    PH_HW_THROTTLE_PL1,
    PH_HW_THROTTLE_PL2,
    PH_HW_THROTTLE_VR_THERMAL,
    PH_HW_THROTTLE_VR_CURRENT,
    PH_HW_THROTTLE_ELECTRICAL,
    PH_HW_THROTTLE_MAX_TURBO,
    PH_HW_THROTTLE_TURBO_ATTENUATION,
    PH_HW_THROTTLE_RUNNING_AVERAGE_THERMAL,
    PH_HW_THROTTLE_RESIDENCY,
    PH_HW_THROTTLE_GRAPHICS,
    PH_HW_THROTTLE_AUTONOMOUS,
};

static CONST PWSTR AtpHwThrottleNames[] =
{
    L"thermal",
    L"prochot",
    L"critical",
    L"power_limit",
    L"pl1",
    L"pl2",
    L"vr_thermal",
    L"vr_current",
    L"electrical",
    L"max_turbo",
    L"turbo_attenuation",
    L"running_average_thermal",
    L"residency",
    L"graphics",
    L"autonomous",
};
C_ASSERT(RTL_NUMBER_OF(AtpHwThrottleFlags) == RTL_NUMBER_OF(AtpHwThrottleNames));

// The same names as JSON keys, which must be literals.
static CONST PCSTR AtpHwThrottleKeys[] =
{
    "thermal",
    "prochot",
    "critical",
    "power_limit",
    "pl1",
    "pl2",
    "vr_thermal",
    "vr_current",
    "electrical",
    "max_turbo",
    "turbo_attenuation",
    "running_average_thermal",
    "residency",
    "graphics",
    "autonomous",
};
C_ASSERT(RTL_NUMBER_OF(AtpHwThrottleFlags) == RTL_NUMBER_OF(AtpHwThrottleKeys));

#define AT_HW_HISTORY_DEFAULT_WINDOW_SECONDS 60
#define AT_HW_HISTORY_MAXIMUM_SAMPLES 65536 // per reply, with include_samples
#define AT_HW_SENSOR_ID_LENGTH (16 + 1 + 2 + 1 + 16)

// A copy of a PH_HW_SENSOR_ENTRY that outlives the enumeration, or of a stored history's info.
typedef struct _AT_HW_SENSOR
{
    PH_HW_COMPONENT_KIND ComponentKind;
    ULONG ComponentIndex;
    ULONG SourceId;
    ULONG64 ComponentKey;
    PPH_STRING ComponentName; // NULL only for a stored history without a name
    PH_HW_SENSOR_GROUP Group;
    PH_HW_SENSOR_UNIT Unit;
    ULONG64 SensorKey;
    PPH_STRING SensorName; // NULL only for a stored history without a name
    BOOLEAN Listed;
    BOOLEAN Current;
    FLOAT Value;
    ULONG ThrottleFlags;
} AT_HW_SENSOR, *PAT_HW_SENSOR;

typedef struct _AT_HW_HISTORY_KEY
{
    ULONG64 ComponentKey;
    PH_HW_SENSOR_GROUP Group;
    ULONG64 SensorKey;
} AT_HW_HISTORY_KEY, *PAT_HW_HISTORY_KEY;

_Function_class_(PH_HW_SENSOR_ENUM_CALLBACK)
static VOID NTAPI AtpHwCollectSensor(
    _In_ PCPH_HW_SENSOR_ENTRY Entry,
    _In_opt_ PVOID Context
    )
{
    PPH_LIST sensors = Context;
    PAT_HW_SENSOR sensor;

    if (!sensors ||
        (ULONG)Entry->ComponentKind >= PhHwComponentMaximum ||
        (ULONG)Entry->Group >= PhHwGroupMaximum ||
        (ULONG)Entry->Unit >= PhHwUnitMaximum)
    {
        return;
    }

    sensor = PhAllocate(sizeof(AT_HW_SENSOR));
    sensor->ComponentKind = Entry->ComponentKind;
    sensor->ComponentIndex = Entry->ComponentIndex;
    sensor->SourceId = Entry->SourceId;
    sensor->ComponentKey = Entry->ComponentKey;
    sensor->ComponentName = PhReferenceObject(Entry->ComponentName);
    sensor->Group = Entry->Group;
    sensor->Unit = Entry->Unit;
    sensor->SensorKey = Entry->SensorKey;
    sensor->SensorName = PhReferenceObject(Entry->SensorName);
    sensor->Listed = TRUE;
    sensor->Current = Entry->Current;
    sensor->Value = Entry->Value;
    sensor->ThrottleFlags = Entry->ThrottleFlags;

    PhAddItemList(sensors, sensor);
}

static VOID AtpHwFreeSensors(
    _In_ PPH_LIST Sensors
    )
{
    for (ULONG i = 0; i < Sensors->Count; i++)
    {
        PAT_HW_SENSOR sensor = Sensors->Items[i];

        PhClearReference(&sensor->ComponentName);
        PhClearReference(&sensor->SensorName);
        PhFree(sensor);
    }

    PhDereferenceObject(Sensors);
}

static int __cdecl AtpHwCompareSensors(
    _In_ void* Context,
    _In_ CONST void* Left,
    _In_ CONST void* Right
    )
{
    PAT_HW_SENSOR left = *(PAT_HW_SENSOR*)Left;
    PAT_HW_SENSOR right = *(PAT_HW_SENSOR*)Right;

    UNREFERENCED_PARAMETER(Context);

    if (left->ComponentKey != right->ComponentKey)
        return left->ComponentKey < right->ComponentKey ? -1 : 1;
    if (left->Group != right->Group)
        return left->Group < right->Group ? -1 : 1;
    if (left->SensorKey != right->SensorKey)
        return left->SensorKey < right->SensorKey ? -1 : 1;

    return 0;
}

// Value receives the index of the name, or ULONG_MAX when the argument is absent.
_Success_(return)
static BOOLEAN AtpHwGetEnumArgument(
    _In_opt_ PVOID Arguments,
    _In_ PCSTR Key,
    _In_reads_(Count) CONST PCSTR* Names,
    _In_ ULONG Count,
    _In_ PCWSTR Allowed,
    _Out_ PULONG Value,
    _Inout_ PAT_TOOL_RESULT Result
    )
{
    PPH_STRING string;
    PPH_BYTES utf8;
    ULONG i;

    *Value = ULONG_MAX;

    if (!Arguments || !PhGetJsonObject(Arguments, Key))
        return TRUE;

    if (!(string = AtGetArgumentString(Arguments, Key)))
    {
        AtSetToolError(Result, "invalid_arguments", STATUS_INVALID_PARAMETER, L"%hs must be one of %s.", Key, Allowed);
        return FALSE;
    }

    utf8 = PhConvertUtf16ToUtf8Ex(string->Buffer, string->Length);
    PhDereferenceObject(string);

    for (i = 0; utf8 && i < Count; i++)
    {
        if (strcmp(utf8->Buffer, Names[i]) == 0)
        {
            *Value = i;
            break;
        }
    }

    PhClearReference(&utf8);

    if (*Value == ULONG_MAX)
    {
        AtSetToolError(Result, "invalid_arguments", STATUS_INVALID_PARAMETER, L"%hs must be one of %s.", Key, Allowed);
        return FALSE;
    }

    return TRUE;
}

static VOID AtpHwAddSensorIdentity(
    _In_ PVOID Row,
    _In_ PAT_HW_SENSOR Sensor
    )
{
    PVOID component;
    PPH_STRING id;

    id = PhFormatString(L"%016I64x.%02lx.%016I64x", Sensor->ComponentKey, (ULONG)Sensor->Group, Sensor->SensorKey);
    AtJsonAddString(Row, "id", id);
    PhDereferenceObject(id);

    component = PhCreateJsonObject();
    PhAddJsonObject(component, "kind", AtpHwComponentKinds[Sensor->ComponentKind]);
    PhAddJsonObjectUInt64(component, "index", Sensor->ComponentIndex);

    if (Sensor->ComponentKind == PhHwComponentSource)
        PhAddJsonObjectUInt64(component, "source_id", Sensor->SourceId);
    else
        AtJsonAddNull(component, "source_id");

    AtJsonAddString(component, "name", Sensor->ComponentName);
    PhAddJsonObjectValue(Row, "component", component);

    PhAddJsonObject(Row, "group", AtpHwGroups[Sensor->Group]);
    AtJsonAddString(Row, "name", Sensor->SensorName);
    PhAddJsonObject(Row, "unit", AtpHwUnits[Sensor->Unit]);
}

static PVOID AtpHwCreateSensorRow(
    _In_ PAT_HW_SENSOR Sensor
    )
{
    PVOID row;

    row = PhCreateJsonObject();
    AtpHwAddSensorIdentity(row, Sensor);

    if (Sensor->Current && Sensor->Unit != PhHwUnitThrottle)
        PhAddJsonObjectDouble(row, "value", Sensor->Value);
    else
        AtJsonAddNull(row, "value");

    if (Sensor->Current && Sensor->Unit == PhHwUnitThrottle)
    {
        AtJsonAddFlagStrings(
            row,
            "throttle",
            Sensor->ThrottleFlags,
            AtpHwThrottleFlags,
            AtpHwThrottleNames,
            RTL_NUMBER_OF(AtpHwThrottleFlags)
            );
    }
    else
    {
        AtJsonAddNull(row, "throttle");
    }

    return row;
}

static VOID AtpHwAddCollector(
    _In_ PVOID Object
    )
{
    PVOID collector;

    collector = PhCreateJsonObject();
    PhAddJsonObjectBoolean(collector, "sensors_enabled", !!PhGetIntegerSetting(L"EnableHardwareSensors"));
    PhAddJsonObjectBoolean(collector, "kernel_sensors_available", PhHwSensorsAvailable());
    PhAddJsonObjectBoolean(collector, "windows_counters_present", PhHwCounterSensorsPresent());
    PhAddJsonObjectBoolean(collector, "source_sensors_present", PhHwSourceSensorsPresent());
    AtJsonAddStringRef(collector, "unavailable_reason", PhHwGetSensorsUnavailableReason());
    PhAddJsonObjectValue(Object, "collector", collector);
}

VOID AtpGetHardwareSensors(
    _In_ PAT_TOOL_CALL Call,
    _Inout_ PAT_TOOL_RESULT Result
    )
{
    ULONG componentFilter;
    ULONG groupFilter;
    PPH_STRING nameContains;
    PPH_LIST sensors;
    AT_ROWS rows;
    PVOID structured;
    ULONG i;

    if (!AtpHwGetEnumArgument(
        Call->Arguments,
        "component",
        AtpHwComponentKinds,
        RTL_NUMBER_OF(AtpHwComponentKinds),
        L"cpu, memory, chipset, system, windows or source",
        &componentFilter,
        Result
        ))
    {
        return;
    }

    if (!AtpHwGetEnumArgument(
        Call->Arguments,
        "group",
        AtpHwGroups,
        RTL_NUMBER_OF(AtpHwGroups),
        L"temperatures, powers, limits, clocks, bandwidth, active_clocks, multipliers, voltages, currents, residency, fans, controls or throttling",
        &groupFilter,
        Result
        ))
    {
        return;
    }

    nameContains = AtGetArgumentString(Call->Arguments, "name_contains");

    // Copied out first so that no row is built while the provider holds a lock.
    sensors = PhCreateList(64);
    PhHwEnumerateSensors(AtpHwCollectSensor, sensors);
    qsort_s(sensors->Items, sensors->Count, sizeof(PVOID), AtpHwCompareSensors, NULL);

    structured = PhCreateJsonObject();
    AtInitializeRows(&rows, Call->Arguments);

    for (i = 0; i < sensors->Count; i++)
    {
        PAT_HW_SENSOR sensor = sensors->Items[i];

        if (componentFilter != ULONG_MAX && (ULONG)sensor->ComponentKind != componentFilter)
            continue;
        if (groupFilter != ULONG_MAX && (ULONG)sensor->Group != groupFilter)
            continue;
        if (nameContains && !AtContainsString(sensor->SensorName, nameContains) && !AtContainsString(sensor->ComponentName, nameContains))
            continue;

        AtAddRow(&rows, AtpHwCreateSensorRow(sensor));
    }

    AtAddRows(structured, "sensors", &rows);

    AtpHwAddCollector(structured);

    AtAddSnapshot(structured);

    Result->StructuredContent = structured;

    AtDeleteRows(&rows);
    AtpHwFreeSensors(sensors);
    PhClearReference(&nameContains);
}

_Success_(return)
static BOOLEAN AtpHwParseHex(
    _In_reads_(Digits) PCWCH Buffer,
    _In_ ULONG Digits,
    _Out_ PULONG64 Value
    )
{
    ULONG64 value = 0;

    for (ULONG i = 0; i < Digits; i++)
    {
        WCHAR c = Buffer[i];
        ULONG digit;

        if (c >= L'0' && c <= L'9')
            digit = c - L'0';
        else if (c >= L'a' && c <= L'f')
            digit = c - L'a' + 10;
        else if (c >= L'A' && c <= L'F')
            digit = c - L'A' + 10;
        else
            return FALSE;

        value = (value << 4) | digit;
    }

    *Value = value;
    return TRUE;
}

// Accepts exactly the id get_hardware_sensors formats: 16 hex digits, '.', 2, '.', 16.
_Success_(return)
BOOLEAN AtpHwParseSensorId(
    _In_ PPH_STRING Id,
    _Out_ PULONG64 ComponentKey,
    _Out_ PH_HW_SENSOR_GROUP* Group,
    _Out_ PULONG64 SensorKey
    )
{
    ULONG64 componentKey;
    ULONG64 group;
    ULONG64 sensorKey;

    if (Id->Length / sizeof(WCHAR) != AT_HW_SENSOR_ID_LENGTH)
        return FALSE;
    if (Id->Buffer[16] != L'.' || Id->Buffer[19] != L'.')
        return FALSE;
    if (!AtpHwParseHex(&Id->Buffer[0], 16, &componentKey))
        return FALSE;
    if (!AtpHwParseHex(&Id->Buffer[17], 2, &group) || group >= PhHwGroupMaximum)
        return FALSE;
    if (!AtpHwParseHex(&Id->Buffer[20], 16, &sensorKey))
        return FALSE;

    *ComponentKey = componentKey;
    *Group = (PH_HW_SENSOR_GROUP)group;
    *SensorKey = sensorKey;
    return TRUE;
}

_Function_class_(PH_HW_SENSOR_HISTORY_ENUM_CALLBACK)
static VOID NTAPI AtpHwCollectHistoryKey(
    _In_ ULONG64 ComponentKey,
    _In_ PH_HW_SENSOR_GROUP Group,
    _In_ ULONG64 SensorKey,
    _In_ BOOLEAN Listed,
    _In_opt_ PVOID Context
    )
{
    PPH_ARRAY keys = Context;
    AT_HW_HISTORY_KEY key;

    UNREFERENCED_PARAMETER(Listed);

    if (!keys)
        return;

    key.ComponentKey = ComponentKey;
    key.Group = Group;
    key.SensorKey = SensorKey;
    PhAddItemArray(keys, &key);
}

// Searches the first Count sensors, which are sorted by key.
static PAT_HW_SENSOR AtpHwFindSensor(
    _In_ PPH_LIST Sensors,
    _In_ ULONG Count,
    _In_ ULONG64 ComponentKey,
    _In_ PH_HW_SENSOR_GROUP Group,
    _In_ ULONG64 SensorKey
    )
{
    AT_HW_SENSOR key;
    PAT_HW_SENSOR keyPointer = &key;
    PAT_HW_SENSOR* found;

    key.ComponentKey = ComponentKey;
    key.Group = Group;
    key.SensorKey = SensorKey;

    found = bsearch_s(&keyPointer, Sensors->Items, Count, sizeof(PVOID), AtpHwCompareSensors, NULL);

    return found ? *found : NULL;
}

// A sensor described by what the history store kept, for one the provider no longer lists.
static PAT_HW_SENSOR AtpHwCreateStoredSensor(
    _In_ ULONG64 ComponentKey,
    _In_ PH_HW_SENSOR_GROUP Group,
    _In_ ULONG64 SensorKey
    )
{
    PH_HW_SENSOR_HISTORY_INFO info;
    PAT_HW_SENSOR sensor;
    FLOAT value;
    ULONG count;

    if (!PhHwCopySensorHistory(ComponentKey, Group, SensorKey, 0, &value, NULL, &count, &info))
        return NULL;

    if ((ULONG)info.ComponentKind >= PhHwComponentMaximum || (ULONG)info.Unit >= PhHwUnitMaximum)
    {
        PhClearReference(&info.ComponentName);
        PhClearReference(&info.SensorName);
        return NULL;
    }

    sensor = PhAllocateZero(sizeof(AT_HW_SENSOR));
    sensor->ComponentKind = info.ComponentKind;
    sensor->ComponentIndex = info.ComponentIndex;
    sensor->SourceId = info.SourceId;
    sensor->ComponentKey = ComponentKey;
    sensor->ComponentName = info.ComponentName;
    sensor->Group = Group;
    sensor->Unit = info.Unit;
    sensor->SensorKey = SensorKey;
    sensor->SensorName = info.SensorName;
    sensor->Listed = info.Listed;

    return sensor;
}

static PVOID AtpHwCreateHistoryRow(
    _In_ PAT_HW_SENSOR Sensor,
    _In_ ULONG WindowSamples,
    _In_ BOOLEAN IncludeSamples,
    _Out_writes_(WindowSamples) PFLOAT Values,
    _Out_writes_(WindowSamples) PLARGE_INTEGER Times,
    _Out_ PULONG SampleCount
    )
{
    BOOLEAN throttle = Sensor->Unit == PhHwUnitThrottle;
    ULONG reasonCounts[RTL_NUMBER_OF(AtpHwThrottleFlags)] = { 0 };
    ULONG throttledCount = 0;
    ULONG readings = 0;
    DOUBLE total = 0.0;
    FLOAT minimum = 0.0f;
    FLOAT maximum = 0.0f;
    FLOAT last = 0.0f;
    ULONG count;
    PVOID row;

    if (!PhHwCopySensorHistory(Sensor->ComponentKey, Sensor->Group, Sensor->SensorKey, WindowSamples, Values, Times, &count, NULL))
        count = 0;

    *SampleCount = count;

    for (ULONG i = 0; i < count; i++)
    {
        FLOAT value = Values[i];

        if (isnan(value))
            continue;

        if (throttle)
        {
            ULONG flags = (ULONG)value;

            if (flags)
                throttledCount++;

            for (ULONG j = 0; j < RTL_NUMBER_OF(AtpHwThrottleFlags); j++)
            {
                if (flags & AtpHwThrottleFlags[j])
                    reasonCounts[j]++;
            }
        }
        else if (readings == 0)
        {
            minimum = value;
            maximum = value;
            last = value; // most recent first
        }
        else
        {
            minimum = min(minimum, value);
            maximum = max(maximum, value);
        }

        total += value;
        readings++;
    }

    row = PhCreateJsonObject();
    AtpHwAddSensorIdentity(row, Sensor);
    PhAddJsonObjectBoolean(row, "listed", Sensor->Listed);
    PhAddJsonObjectUInt64(row, "sample_count", count);

    if (!throttle && readings)
    {
        PVOID stats = PhCreateJsonObject();

        PhAddJsonObjectDouble(stats, "average", total / readings);
        PhAddJsonObjectDouble(stats, "minimum", minimum);
        PhAddJsonObjectDouble(stats, "maximum", maximum);
        PhAddJsonObjectDouble(stats, "last", last);
        PhAddJsonObjectUInt64(stats, "reading_count", readings);
        PhAddJsonObjectValue(row, "stats", stats);
    }
    else
    {
        AtJsonAddNull(row, "stats");
    }

    if (throttle)
    {
        PVOID summary = PhCreateJsonObject();
        PVOID reasons = PhCreateJsonObject();

        PhAddJsonObjectUInt64(summary, "throttled_count", throttledCount);

        for (ULONG j = 0; j < RTL_NUMBER_OF(AtpHwThrottleFlags); j++)
        {
            if (reasonCounts[j])
                PhAddJsonObjectUInt64(reasons, AtpHwThrottleKeys[j], reasonCounts[j]);
        }

        PhAddJsonObjectValue(summary, "reasons", reasons);
        PhAddJsonObjectValue(row, "throttle", summary);
    }
    else
    {
        AtJsonAddNull(row, "throttle");
    }

    if (IncludeSamples)
    {
        PVOID samples = PhCreateJsonArray();

        for (ULONG i = 0; i < count; i++)
        {
            PVOID sample = PhCreateJsonObject();

            AtJsonAddTime(sample, "time", &Times[i]);

            if (isnan(Values[i]))
                AtJsonAddNull(sample, throttle ? "throttle" : "value");
            else if (throttle)
                AtJsonAddFlagStrings(sample, "throttle", (ULONG)Values[i], AtpHwThrottleFlags, AtpHwThrottleNames, RTL_NUMBER_OF(AtpHwThrottleFlags));
            else
                PhAddJsonObjectDouble(sample, "value", Values[i]);

            PhAddJsonArrayObject(samples, sample);
        }

        PhAddJsonObjectValue(row, "samples", samples);
    }
    else
    {
        AtJsonAddNull(row, "samples");
    }

    return row;
}

VOID AtpGetHardwareSensorHistory(
    _In_ PAT_TOOL_CALL Call,
    _Inout_ PAT_TOOL_RESULT Result
    )
{
    ULONG componentFilter;
    ULONG groupFilter;
    PPH_STRING nameContains = NULL;
    ULONG64 windowSeconds;
    ULONG interval;
    ULONG slots;
    ULONG windowSamples;
    ULONG coveredSamples = 0;
    BOOLEAN includeSamples;
    PVOID ids;
    PPH_LIST sensors;
    ULONG listedCount;
    PPH_LIST selected;
    PFLOAT values;
    PLARGE_INTEGER times;
    AT_ROWS rows;
    PVOID structured;
    ULONG i;

    if (!AtpHwGetEnumArgument(
        Call->Arguments,
        "component",
        AtpHwComponentKinds,
        RTL_NUMBER_OF(AtpHwComponentKinds),
        L"cpu, memory, chipset, system, windows or source",
        &componentFilter,
        Result
        ))
    {
        return;
    }

    if (!AtpHwGetEnumArgument(
        Call->Arguments,
        "group",
        AtpHwGroups,
        RTL_NUMBER_OF(AtpHwGroups),
        L"temperatures, powers, limits, clocks, bandwidth, active_clocks, multipliers, voltages, currents, residency, fans, controls or throttling",
        &groupFilter,
        Result
        ))
    {
        return;
    }

    interval = AtGetUpdateInterval();
    slots = PhHwGetSensorHistorySlots();

    if (!AtGetArgumentUInt64(Call->Arguments, "window_seconds", &windowSeconds) || windowSeconds == 0)
        windowSeconds = AT_HW_HISTORY_DEFAULT_WINDOW_SECONDS;

    windowSamples = (ULONG)min(windowSeconds * 1000 / interval, slots);

    if (windowSamples == 0)
        windowSamples = 1;

    includeSamples = AtJsonGetObjectBoolean(Call->Arguments, "include_samples");
    ids = AtJsonGetObjectMember(Call->Arguments, "ids", PH_JSON_OBJECT_TYPE_ARRAY);

    if (ids && PhGetJsonArrayLength(ids) > AT_ROWS_MAXIMUM_LIMIT)
    {
        AtSetToolError(Result, "invalid_arguments", STATUS_INVALID_PARAMETER, L"ids has more than %lu entries.", AT_ROWS_MAXIMUM_LIMIT);
        return;
    }

    // Copied out first so that no row is built while the provider or the store holds a lock.
    sensors = PhCreateList(64);
    PhHwEnumerateSensors(AtpHwCollectSensor, sensors);
    qsort_s(sensors->Items, sensors->Count, sizeof(PVOID), AtpHwCompareSensors, NULL);
    listedCount = sensors->Count;

    // The rows in the order they are returned; sensors owns them.
    selected = PhCreateList(64);

    if (ids)
    {
        ULONG count = PhGetJsonArrayLength(ids);

        for (i = 0; i < count; i++)
        {
            PPH_STRING id = PhGetJsonObjectString(PhGetJsonArrayIndexObject(ids, i));
            ULONG64 componentKey;
            PH_HW_SENSOR_GROUP group;
            ULONG64 sensorKey;
            PAT_HW_SENSOR sensor;

            if (!id || !AtpHwParseSensorId(id, &componentKey, &group, &sensorKey))
            {
                AtSetToolError(Result, "invalid_arguments", STATUS_INVALID_PARAMETER, L"ids[%lu] is not a sensor id from get_hardware_sensors.", i);
                PhClearReference(&id);
                goto CleanupExit;
            }

            if (!(sensor = AtpHwFindSensor(sensors, listedCount, componentKey, group, sensorKey)))
            {
                if (!(sensor = AtpHwCreateStoredSensor(componentKey, group, sensorKey)))
                {
                    AtSetToolError(Result, "not_found", STATUS_NOT_FOUND, L"No sensor or history has the id %s.", id->Buffer);
                    PhDereferenceObject(id);
                    goto CleanupExit;
                }

                PhAddItemList(sensors, sensor);
            }

            PhAddItemList(selected, sensor);
            PhDereferenceObject(id);
        }
    }
    else
    {
        PH_ARRAY keys;

        nameContains = AtGetArgumentString(Call->Arguments, "name_contains");

        // The histories of sensors that are no longer listed are rows too.
        PhInitializeArray(&keys, sizeof(AT_HW_HISTORY_KEY), 64);
        PhHwEnumerateSensorHistories(AtpHwCollectHistoryKey, &keys);

        for (i = 0; i < keys.Count; i++)
        {
            PAT_HW_HISTORY_KEY key = PhItemArray(&keys, i);
            PAT_HW_SENSOR sensor;

            if ((ULONG)key->Group >= PhHwGroupMaximum)
                continue;
            if (AtpHwFindSensor(sensors, listedCount, key->ComponentKey, key->Group, key->SensorKey))
                continue;

            if (sensor = AtpHwCreateStoredSensor(key->ComponentKey, key->Group, key->SensorKey))
                PhAddItemList(sensors, sensor);
        }

        PhDeleteArray(&keys);
        qsort_s(sensors->Items, sensors->Count, sizeof(PVOID), AtpHwCompareSensors, NULL);

        for (i = 0; i < sensors->Count; i++)
        {
            PAT_HW_SENSOR sensor = sensors->Items[i];

            if (componentFilter != ULONG_MAX && (ULONG)sensor->ComponentKind != componentFilter)
                continue;
            if (groupFilter != ULONG_MAX && (ULONG)sensor->Group != groupFilter)
                continue;
            if (nameContains && !AtContainsString(sensor->SensorName, nameContains) && !AtContainsString(sensor->ComponentName, nameContains))
                continue;

            PhAddItemList(selected, sensor);
        }
    }

    values = PhAllocate(windowSamples * sizeof(FLOAT));
    times = PhAllocate(windowSamples * sizeof(LARGE_INTEGER));

    structured = PhCreateJsonObject();
    AtInitializeRows(&rows, Call->Arguments);

    // The rows keep their order, so the page is known before a row is built: only its rows carry samples, and it
    // holds no more of them than AT_HW_HISTORY_MAXIMUM_SAMPLES.
    PhClearReference(&rows.SortBy);

    if (includeSamples)
        rows.Limit = min(rows.Limit, max(AT_HW_HISTORY_MAXIMUM_SAMPLES / windowSamples, 1));

    for (i = 0; i < selected->Count; i++)
    {
        BOOLEAN samples = includeSamples && i >= rows.Offset && i - rows.Offset < rows.Limit;
        ULONG count;

        AtAddRow(&rows, AtpHwCreateHistoryRow(selected->Items[i], windowSamples, samples, values, times, &count));
        coveredSamples = max(coveredSamples, count);
    }

    PhAddJsonObjectUInt64(structured, "update_interval_ms", interval);
    PhAddJsonObjectUInt64(structured, "history_slots", slots);
    PhAddJsonObjectUInt64(structured, "window_seconds", (ULONG64)coveredSamples * interval / 1000);
    AtAddRows(structured, "sensors", &rows);
    AtpHwAddCollector(structured);
    AtAddSnapshot(structured);

    Result->StructuredContent = structured;

    AtDeleteRows(&rows);
    PhFree(times);
    PhFree(values);

CleanupExit:
    PhDereferenceObject(selected);
    AtpHwFreeSensors(sensors);
    PhClearReference(&nameContains);
}

VOID AtpGetHardwareReport(
    _In_ PAT_TOOL_CALL Call,
    _Inout_ PAT_TOOL_RESULT Result
    )
{
    PPH_STRING report;
    PVOID structured;

    UNREFERENCED_PARAMETER(Call);

    report = PhHwCreateReport(NULL);

    structured = PhCreateJsonObject();
    AtJsonAddString(structured, "report", report);
    AtAddSnapshot(structured);

    Result->StructuredContent = structured;

    PhDereferenceObject(report);
}

VOID AtHardwareInvokeTool(
    _In_ PCAT_TOOL Tool,
    _In_ PAT_TOOL_CALL Call,
    _Inout_ PAT_TARGET Target,
    _Inout_ PAT_TOOL_RESULT Result
    )
{
    UNREFERENCED_PARAMETER(Target);

    switch (Tool->Action)
    {
    case AtActionGetHardwareSensors:
        AtpGetHardwareSensors(Call, Result);
        break;
    case AtActionGetHardwareReport:
        AtpGetHardwareReport(Call, Result);
        break;
    case AtActionGetHardwareSensorHistory:
        AtpGetHardwareSensorHistory(Call, Result);
        break;
    default:
        AtSetToolError(Result, "failed", STATUS_NOT_IMPLEMENTED, L"This tool is not implemented.");
        break;
    }
}
