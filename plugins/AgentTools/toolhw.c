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

// A copy of a PH_HW_SENSOR_ENTRY that outlives the enumeration.
typedef struct _AT_HW_SENSOR
{
    PH_HW_COMPONENT_KIND ComponentKind;
    ULONG ComponentIndex;
    ULONG SourceId;
    ULONG64 ComponentKey;
    PPH_STRING ComponentName;
    PH_HW_SENSOR_GROUP Group;
    PH_HW_SENSOR_UNIT Unit;
    ULONG64 SensorKey;
    PPH_STRING SensorName;
    BOOLEAN Current;
    FLOAT Value;
    ULONG ThrottleFlags;
} AT_HW_SENSOR, *PAT_HW_SENSOR;

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
    sensor->Current = Entry->Current;
    sensor->Value = Entry->Value;
    sensor->ThrottleFlags = Entry->ThrottleFlags;

    PhAddItemList(sensors, sensor);
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

static PVOID AtpHwCreateSensorRow(
    _In_ PAT_HW_SENSOR Sensor
    )
{
    PVOID row;
    PVOID component;
    PPH_STRING id;

    row = PhCreateJsonObject();

    id = PhFormatString(L"%016I64x.%02lx.%016I64x", Sensor->ComponentKey, (ULONG)Sensor->Group, Sensor->SensorKey);
    AtJsonAddString(row, "id", id);
    PhDereferenceObject(id);

    component = PhCreateJsonObject();
    PhAddJsonObject(component, "kind", AtpHwComponentKinds[Sensor->ComponentKind]);
    PhAddJsonObjectUInt64(component, "index", Sensor->ComponentIndex);

    if (Sensor->ComponentKind == PhHwComponentSource)
        PhAddJsonObjectUInt64(component, "source_id", Sensor->SourceId);
    else
        AtJsonAddNull(component, "source_id");

    AtJsonAddString(component, "name", Sensor->ComponentName);
    PhAddJsonObjectValue(row, "component", component);

    PhAddJsonObject(row, "group", AtpHwGroups[Sensor->Group]);
    AtJsonAddString(row, "name", Sensor->SensorName);
    PhAddJsonObject(row, "unit", AtpHwUnits[Sensor->Unit]);

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
    PVOID collector;
    PCPH_STRINGREF reason;
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

    collector = PhCreateJsonObject();
    PhAddJsonObjectBoolean(collector, "sensors_enabled", !!PhGetIntegerSetting(L"EnableHardwareSensors"));
    PhAddJsonObjectBoolean(collector, "kernel_sensors_available", PhHwSensorsAvailable());
    PhAddJsonObjectBoolean(collector, "windows_counters_present", PhHwCounterSensorsPresent());
    PhAddJsonObjectBoolean(collector, "source_sensors_present", PhHwSourceSensorsPresent());
    reason = PhHwGetSensorsUnavailableReason();
    AtJsonAddStringRef(collector, "unavailable_reason", reason);
    PhAddJsonObjectValue(structured, "collector", collector);

    AtAddSnapshot(structured);

    Result->StructuredContent = structured;

    AtDeleteRows(&rows);

    for (i = 0; i < sensors->Count; i++)
    {
        PAT_HW_SENSOR sensor = sensors->Items[i];

        PhDereferenceObject(sensor->ComponentName);
        PhDereferenceObject(sensor->SensorName);
        PhFree(sensor);
    }

    PhDereferenceObject(sensors);
    PhClearReference(&nameContains);
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
    default:
        AtSetToolError(Result, "failed", STATUS_NOT_IMPLEMENTED, L"This tool is not implemented.");
        break;
    }
}
