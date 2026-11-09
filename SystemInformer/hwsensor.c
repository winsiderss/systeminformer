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

#include <phapp.h>
#include <phplug.h>
#include <hwprv.h>
#include <sysinfo.h>
#include <sysinfop.h>

// The display mapping of the hardware sensors: components, groups, units, names and ordering keys.

typedef struct _PHP_HW_GROUP_INFO
{
    PCWSTR Name;
    BOOLEAN PerProcessor; // collapsed by default on large systems
} PHP_HW_GROUP_INFO, *PPHP_HW_GROUP_INFO;
typedef CONST PHP_HW_GROUP_INFO *PCPHP_HW_GROUP_INFO;

static CONST PHP_HW_GROUP_INFO PhpHwGroupInfo[PhHwGroupMaximum] =
{
    { L"Temperatures", FALSE },
    { L"Powers", FALSE },
    { L"Limits", FALSE },
    { L"Clocks", TRUE },
    { L"Bandwidth", FALSE },
    { L"Active clocks", TRUE },
    { L"Multipliers", TRUE },
    { L"Voltages", TRUE },
    { L"Currents", FALSE },
    { L"Residency", FALSE },
    { L"Fans", FALSE },
    { L"Controls", FALSE },
    { L"Throttling", FALSE },
};

// Ordered as displayed within a group.
typedef enum _PHP_HW_SENSOR_KIND
{
    PhpHwSensorPackageTemperature,
    PhpHwSensorCoresTemperature,
    PhpHwSensorGraphicsTemperature,
    PhpHwSensorCcdTemperature,
    PhpHwSensorCoreTemperature,
    PhpHwSensorSmuTemperature, // named
    PhpHwSensorPmtTemperature, // named
    PhpHwSensorChipsetTemperature,
    PhpHwSensorDimmTemperature, // named
    PhpHwSensorDimmPmicTemperature, // named
    PhpHwSensorAcpiTemperature, // named
    PhpHwSensorZoneTemperature, // named
    PhpHwSensorPackagePower,
    PhpHwSensorCorePower,
    PhpHwSensorDramPower,
    PhpHwSensorPlatformPower,
    PhpHwSensorTpmiPower, // named
    PhpHwSensorSmuPower, // named
    PhpHwSensorDimmPower, // named
    PhpHwSensorAcpiPower, // named
    PhpHwSensorMeterPower, // named
    PhpHwSensorPowerLimit1,
    PhpHwSensorPowerLimit2,
    PhpHwSensorSmuPowerLimit, // named
    PhpHwSensorPowerLimit, // named
    PhpHwSensorSmuCurrentLimit, // named
    PhpHwSensorSmuTemperatureLimit, // named
    PhpHwSensorSmuClockLimit, // named
    PhpHwSensorUncoreClock, // named
    PhpHwSensorSmuClock, // named
    PhpHwSensorEffectiveClock,
    PhpHwSensorActiveClock,
    PhpHwSensorOsClock, // named
    PhpHwSensorMemoryClock,
    PhpHwSensorMemoryDataRate,
    PhpHwSensorMemoryGear,
    PhpHwSensorSmuMemoryClock, // named
    PhpHwSensorMemoryReadBandwidth,
    PhpHwSensorMemoryWriteBandwidth,
    PhpHwSensorSmuMemoryLoad, // named
    PhpHwSensorSmuBandwidth, // named
    PhpHwSensorMultiplier,
    PhpHwSensorVoltage,
    PhpHwSensorSmuVoltage, // named
    PhpHwSensorDimmVoltage, // named
    PhpHwSensorAcpiVoltage, // named
    PhpHwSensorSmuCurrent, // named
    PhpHwSensorDimmCurrent, // named
    PhpHwSensorSmuResidency, // named
    PhpHwSensorPlatformResidency, // named
    PhpHwSensorFanSpeed, // named
    PhpHwSensorFanControl, // named
    PhpHwSensorThrottle,
    PhpHwSensorZonePassiveLimit, // named
    PhpHwSensorOsPerformanceLimit, // named
    PhpHwSensorMaximum // a source sensor
} PHP_HW_SENSOR_KIND;

typedef struct _PHP_HW_SENSOR_INFO
{
    PH_HW_COMPONENT_KIND Component;
    PH_HW_SENSOR_GROUP Group;
    ULONG Capability;
    PH_HW_SENSOR_UNIT Unit;
    PCWSTR Name; // followed by the index when Indexed; NULL for named sensors
    BOOLEAN Indexed;
    BOOLEAN History;
} PHP_HW_SENSOR_INFO, *PPHP_HW_SENSOR_INFO;
typedef CONST PHP_HW_SENSOR_INFO *PCPHP_HW_SENSOR_INFO;

static CONST PHP_HW_SENSOR_INFO PhpHwSensorInfo[PhpHwSensorMaximum] =
{
    { PhHwComponentCpu, PhHwGroupTemperatures, PH_HW_CAP_PACKAGE_TEMP, PhHwUnitTemperature, L"Package", FALSE, TRUE },
    { PhHwComponentCpu, PhHwGroupTemperatures, PH_HW_CAP_CORES_TEMP, PhHwUnitTemperature, L"IA cores", FALSE, TRUE },
    { PhHwComponentCpu, PhHwGroupTemperatures, PH_HW_CAP_GRAPHICS_TEMP, PhHwUnitTemperature, L"Graphics", FALSE, TRUE },
    { PhHwComponentCpu, PhHwGroupTemperatures, PH_HW_CAP_CCD_TEMP, PhHwUnitTemperature, L"CCD ", TRUE, TRUE },
    { PhHwComponentCpu, PhHwGroupTemperatures, PH_HW_CAP_CORE_TEMP, PhHwUnitTemperature, L"Core ", TRUE, TRUE },
    { PhHwComponentCpu, PhHwGroupTemperatures, PH_HW_CAP_SMU, PhHwUnitTemperature, NULL, FALSE, TRUE },
    { PhHwComponentCpu, PhHwGroupTemperatures, PH_HW_CAP_TPMI_POWER, PhHwUnitTemperature, NULL, FALSE, TRUE },
    { PhHwComponentChipset, PhHwGroupTemperatures, PH_HW_CAP_CHIPSET_TEMP, PhHwUnitTemperature, L"PCH", FALSE, TRUE },
    { PhHwComponentMemory, PhHwGroupTemperatures, PH_HW_CAP_DIMM_TEMP, PhHwUnitTemperature, NULL, FALSE, TRUE },
    { PhHwComponentMemory, PhHwGroupTemperatures, PH_HW_CAP_DIMM_PMIC, PhHwUnitTemperature, NULL, FALSE, FALSE },
    { PhHwComponentSystem, PhHwGroupTemperatures, PH_HW_CAP_ACPI_TEMP, PhHwUnitTemperature, NULL, FALSE, TRUE },
    { PhHwComponentWindows, PhHwGroupTemperatures, 0, PhHwUnitTemperature, NULL, FALSE, TRUE },
    { PhHwComponentCpu, PhHwGroupPowers, PH_HW_CAP_RAPL_PACKAGE, PhHwUnitPower, L"Package", FALSE, TRUE },
    { PhHwComponentCpu, PhHwGroupPowers, PH_HW_CAP_RAPL_CORE, PhHwUnitPower, L"Core", FALSE, TRUE },
    { PhHwComponentCpu, PhHwGroupPowers, PH_HW_CAP_RAPL_DRAM, PhHwUnitPower, L"DRAM", FALSE, TRUE },
    { PhHwComponentCpu, PhHwGroupPowers, PH_HW_CAP_RAPL_PSYS, PhHwUnitPower, L"Platform", FALSE, TRUE },
    { PhHwComponentCpu, PhHwGroupPowers, PH_HW_CAP_TPMI_POWER, PhHwUnitPower, NULL, FALSE, TRUE },
    { PhHwComponentCpu, PhHwGroupPowers, PH_HW_CAP_SMU, PhHwUnitPower, NULL, FALSE, TRUE },
    { PhHwComponentMemory, PhHwGroupPowers, PH_HW_CAP_DIMM_PMIC, PhHwUnitPower, NULL, FALSE, TRUE },
    { PhHwComponentSystem, PhHwGroupPowers, PH_HW_CAP_ACPI_POWER, PhHwUnitPower, NULL, FALSE, TRUE },
    { PhHwComponentWindows, PhHwGroupPowers, 0, PhHwUnitPower, NULL, FALSE, TRUE },
    { PhHwComponentCpu, PhHwGroupLimits, PH_HW_CAP_POWER_LIMIT1, PhHwUnitPower, L"PL1 (active)", FALSE, FALSE },
    { PhHwComponentCpu, PhHwGroupLimits, PH_HW_CAP_POWER_LIMIT2, PhHwUnitPower, L"PL2 (active)", FALSE, FALSE },
    { PhHwComponentCpu, PhHwGroupLimits, PH_HW_CAP_SMU, PhHwUnitPower, NULL, FALSE, FALSE },
    { PhHwComponentCpu, PhHwGroupLimits, PH_HW_CAP_PLATFORM_LIMITS, PhHwUnitPower, NULL, FALSE, FALSE },
    { PhHwComponentCpu, PhHwGroupLimits, PH_HW_CAP_SMU, PhHwUnitCurrent, NULL, FALSE, FALSE },
    { PhHwComponentCpu, PhHwGroupLimits, PH_HW_CAP_SMU, PhHwUnitTemperature, NULL, FALSE, FALSE },
    { PhHwComponentCpu, PhHwGroupLimits, PH_HW_CAP_SMU, PhHwUnitClock, NULL, FALSE, FALSE },
    { PhHwComponentCpu, PhHwGroupClocks, PH_HW_CAP_UNCORE_CLOCK, PhHwUnitClock, NULL, FALSE, TRUE },
    { PhHwComponentCpu, PhHwGroupClocks, PH_HW_CAP_SMU, PhHwUnitClock, NULL, FALSE, TRUE },
    { PhHwComponentCpu, PhHwGroupClocks, PH_HW_CAP_EFFECTIVE_CLOCK, PhHwUnitClock, L"CPU ", TRUE, TRUE },
    { PhHwComponentCpu, PhHwGroupActiveClocks, PH_HW_CAP_ACTIVE_CLOCK, PhHwUnitClock, L"CPU ", TRUE, TRUE },
    { PhHwComponentWindows, PhHwGroupClocks, 0, PhHwUnitClock, NULL, FALSE, TRUE },
    { PhHwComponentMemory, PhHwGroupClocks, PH_HW_CAP_MEMORY_CLOCK, PhHwUnitClock, L"Memory clock", FALSE, TRUE },
    { PhHwComponentMemory, PhHwGroupClocks, PH_HW_CAP_MEMORY_CLOCK, PhHwUnitDataRate, L"Data rate", FALSE, FALSE },
    { PhHwComponentMemory, PhHwGroupClocks, PH_HW_CAP_MEMORY_CLOCK, PhHwUnitGear, L"Gear", FALSE, FALSE },
    { PhHwComponentMemory, PhHwGroupClocks, PH_HW_CAP_SMU, PhHwUnitClock, NULL, FALSE, TRUE },
    { PhHwComponentMemory, PhHwGroupBandwidth, PH_HW_CAP_MEMORY_BANDWIDTH, PhHwUnitBandwidth, L"DRAM read", FALSE, TRUE },
    { PhHwComponentMemory, PhHwGroupBandwidth, PH_HW_CAP_MEMORY_BANDWIDTH, PhHwUnitBandwidth, L"DRAM write", FALSE, TRUE },
    { PhHwComponentMemory, PhHwGroupBandwidth, PH_HW_CAP_SMU, PhHwUnitPercent, NULL, FALSE, TRUE },
    { PhHwComponentMemory, PhHwGroupBandwidth, PH_HW_CAP_SMU, PhHwUnitBandwidth, NULL, FALSE, TRUE },
    { PhHwComponentCpu, PhHwGroupMultipliers, PH_HW_CAP_MULTIPLIER, PhHwUnitMultiplier, L"CPU ", TRUE, TRUE },
    { PhHwComponentCpu, PhHwGroupVoltages, PH_HW_CAP_VOLTAGE, PhHwUnitVoltage, L"CPU ", TRUE, TRUE },
    { PhHwComponentCpu, PhHwGroupVoltages, PH_HW_CAP_SMU, PhHwUnitVoltage, NULL, FALSE, TRUE },
    { PhHwComponentMemory, PhHwGroupVoltages, PH_HW_CAP_DIMM_VOLTAGE, PhHwUnitVoltage, NULL, FALSE, FALSE },
    { PhHwComponentSystem, PhHwGroupVoltages, PH_HW_CAP_ACPI_TEMP, PhHwUnitVoltage, NULL, FALSE, TRUE },
    { PhHwComponentCpu, PhHwGroupCurrents, PH_HW_CAP_SMU, PhHwUnitCurrent, NULL, FALSE, TRUE },
    { PhHwComponentMemory, PhHwGroupCurrents, PH_HW_CAP_DIMM_PMIC, PhHwUnitCurrent, NULL, FALSE, TRUE },
    { PhHwComponentCpu, PhHwGroupResidency, PH_HW_CAP_SMU, PhHwUnitPercent, NULL, FALSE, TRUE },
    { PhHwComponentCpu, PhHwGroupResidency, PH_HW_CAP_RESIDENCY, PhHwUnitPercent, NULL, FALSE, TRUE },
    { PhHwComponentSystem, PhHwGroupFans, PH_HW_CAP_FAN_SPEED, PhHwUnitFan, NULL, FALSE, TRUE },
    { PhHwComponentSystem, PhHwGroupControls, PH_HW_CAP_FAN_CONTROL, PhHwUnitPercent, NULL, FALSE, TRUE },
    { PhHwComponentCpu, PhHwGroupThrottling, PH_HW_CAP_THROTTLE, PhHwUnitThrottle, L"Package", FALSE, FALSE },
    { PhHwComponentWindows, PhHwGroupThrottling, 0, PhHwUnitPercent, NULL, FALSE, TRUE },
    { PhHwComponentWindows, PhHwGroupThrottling, 0, PhHwUnitPercent, NULL, FALSE, TRUE },
};

// Group and unit of each PH_HW_SENSOR_TYPE.
static CONST struct
{
    PH_HW_SENSOR_GROUP Group;
    PH_HW_SENSOR_UNIT Unit;
} PhpHwSourceTypeInfo[PhHwSensorTypeMaximum] =
{
    { PhHwGroupTemperatures, PhHwUnitTemperature },
    { PhHwGroupPowers, PhHwUnitPower },
    { PhHwGroupClocks, PhHwUnitClock },
    { PhHwGroupVoltages, PhHwUnitVoltage },
    { PhHwGroupCurrents, PhHwUnitCurrent },
    { PhHwGroupFans, PhHwUnitFan },
    { PhHwGroupControls, PhHwUnitPercent },
    { PhHwGroupBandwidth, PhHwUnitBandwidth },
};

// The display kind of each PH_HW_NAMED_SENSOR_KIND.
static CONST PHP_HW_SENSOR_KIND PhpHwNamedSensorKinds[] =
{
    PhpHwSensorDimmTemperature,
    PhpHwSensorUncoreClock,
    PhpHwSensorTpmiPower,
    PhpHwSensorSmuTemperature,
    PhpHwSensorSmuPower,
    PhpHwSensorSmuPowerLimit,
    PhpHwSensorSmuCurrentLimit,
    PhpHwSensorSmuTemperatureLimit,
    PhpHwSensorSmuClock,
    PhpHwSensorSmuClockLimit,
    PhpHwSensorSmuMemoryClock,
    PhpHwSensorSmuMemoryLoad,
    PhpHwSensorSmuVoltage,
    PhpHwSensorSmuCurrent,
    PhpHwSensorSmuResidency,
    PhpHwSensorFanSpeed,
    PhpHwSensorDimmVoltage,
    PhpHwSensorDimmCurrent,
    PhpHwSensorDimmPower,
    PhpHwSensorDimmPmicTemperature,
    PhpHwSensorAcpiTemperature,
    PhpHwSensorFanControl,
    PhpHwSensorAcpiPower,
    PhpHwSensorPlatformResidency,
    PhpHwSensorSmuBandwidth,
    PhpHwSensorPowerLimit,
    PhpHwSensorPmtTemperature,
    PhpHwSensorAcpiVoltage,
    PhpHwSensorZoneTemperature,
    PhpHwSensorZonePassiveLimit,
    PhpHwSensorMeterPower,
    PhpHwSensorOsClock,
    PhpHwSensorOsPerformanceLimit,
};
C_ASSERT(RTL_NUMBER_OF(PhpHwNamedSensorKinds) == PhHwNamedSensorMaximum);

typedef struct _PHP_HW_ENUM_CONTEXT
{
    PPH_HW_SENSOR_ENUM_CALLBACK Callback;
    PVOID Context;
    ULONG64 ComponentKey; // of ComponentName
    PPH_STRING ComponentName;
} PHP_HW_ENUM_CONTEXT, *PPHP_HW_ENUM_CONTEXT;

static ULONG64 PhpHwComponentKey(
    _In_ PH_HW_COMPONENT_KIND Kind,
    _In_ ULONG SourceId,
    _In_ ULONG Index
    )
{
    return ((ULONG64)Kind << 56) | ((ULONG64)(SourceId & 0xFFFFFF) << 32) | Index;
}

static ULONG64 PhpHwSensorKey(
    _In_ PHP_HW_SENSOR_KIND Kind,
    _In_ ULONG Index
    )
{
    return ((ULONG64)Kind << 32) | Index;
}

static PHP_HW_SENSOR_KIND PhpHwSensorKindFromKey(
    _In_ ULONG64 SensorKey
    )
{
    return (PHP_HW_SENSOR_KIND)(SensorKey >> 32);
}

static PPH_STRING PhpHwGetCpuBrandString(
    VOID
    )
{
    static PH_INITONCE initOnce = PH_INITONCE_INIT;
    static PPH_STRING brandString = NULL;

    if (PhBeginInitOnce(&initOnce))
    {
        if (!(brandString = PhSipGetCpuBrandString()))
            brandString = PhReferenceEmptyString();

        PhEndInitOnce(&initOnce);
    }

    return brandString;
}

static PPH_STRING PhpHwFormatComponentName(
    _In_ PH_HW_COMPONENT_KIND Kind,
    _In_ ULONG Index
    )
{
    switch (Kind)
    {
    case PhHwComponentCpu:
        {
            PH_FORMAT format[4];

            if (PhHwGetPackageCount() <= 1)
                return PhReferenceObject(PhpHwGetCpuBrandString());

            PhInitFormatS(&format[0], L"CPU #");
            PhInitFormatU(&format[1], Index);
            PhInitFormatS(&format[2], L": ");
            PhInitFormatSR(&format[3], PhpHwGetCpuBrandString()->sr);

            return PhFormat(format, RTL_NUMBER_OF(format), 0);
        }
    case PhHwComponentMemory:
        {
            PH_FORMAT format[2];

            if (PhHwGetPackageCount() <= 1)
                return PhCreateString(L"Memory");

            PhInitFormatS(&format[0], L"Memory #");
            PhInitFormatU(&format[1], Index);

            return PhFormat(format, RTL_NUMBER_OF(format), 0);
        }
    case PhHwComponentChipset:
        return PhCreateString(L"Chipset");
    case PhHwComponentSystem:
        return PhCreateString(L"System");
    case PhHwComponentWindows:
        return PhCreateString(L"Windows");
    }

    return PhReferenceEmptyString();
}

static PPH_STRING PhpHwFormatSensorName(
    _In_ PHP_HW_SENSOR_KIND Kind,
    _In_ ULONG Index
    )
{
    PCPHP_HW_SENSOR_INFO info = &PhpHwSensorInfo[Kind];
    PH_FORMAT format[2];

    if (!info->Indexed)
        return PhCreateString(info->Name);

    PhInitFormatS(&format[0], info->Name);
    PhInitFormatU(&format[1], Index);

    return PhFormat(format, RTL_NUMBER_OF(format), 0);
}

static PPH_STRING PhpHwGetComponentName(
    _Inout_ PPHP_HW_ENUM_CONTEXT Context,
    _In_ PH_HW_COMPONENT_KIND Kind,
    _In_ ULONG Index
    )
{
    ULONG64 key = PhpHwComponentKey(Kind, 0, Index);

    if (!Context->ComponentName || Context->ComponentKey != key)
    {
        PhMoveReference(&Context->ComponentName, PhpHwFormatComponentName(Kind, Index));
        Context->ComponentKey = key;
    }

    return Context->ComponentName;
}

// Reports a sensor of the PhpHwSensorInfo table; a NULL Name is formatted from the kind and index.
static VOID PhpHwEmitSensor(
    _Inout_ PPHP_HW_ENUM_CONTEXT Context,
    _In_ ULONG ComponentIndex,
    _In_ PHP_HW_SENSOR_KIND Kind,
    _In_ ULONG Index,
    _In_opt_ PPH_STRING Name,
    _In_ BOOLEAN Current,
    _In_ FLOAT Value,
    _In_ ULONG ThrottleFlags
    )
{
    PCPHP_HW_SENSOR_INFO info = &PhpHwSensorInfo[Kind];
    PH_HW_SENSOR_ENTRY entry;
    PPH_STRING sensorName;

    sensorName = Name ? PhReferenceObject(Name) : PhpHwFormatSensorName(Kind, Index);

    entry.ComponentKind = info->Component;
    entry.ComponentIndex = ComponentIndex;
    entry.SourceId = 0;
    entry.ComponentKey = PhpHwComponentKey(info->Component, 0, ComponentIndex);
    entry.ComponentName = PhpHwGetComponentName(Context, info->Component, ComponentIndex);
    entry.Group = info->Group;
    entry.Unit = info->Unit;
    entry.SensorKey = PhpHwSensorKey(Kind, Index);
    entry.SensorName = sensorName;
    entry.Capability = info->Name ? info->Capability : 0;
    entry.Current = Current;
    entry.History = info->History;
    entry.Value = Value;
    entry.ThrottleFlags = ThrottleFlags;

    Context->Callback(&entry, Context->Context);

    PhDereferenceObject(sensorName);
}

static VOID PhpHwEmitValue(
    _Inout_ PPHP_HW_ENUM_CONTEXT Context,
    _In_ ULONG ComponentIndex,
    _In_ PHP_HW_SENSOR_KIND Kind,
    _In_ ULONG Index,
    _In_ FLOAT Value
    )
{
    PhpHwEmitSensor(Context, ComponentIndex, Kind, Index, NULL, TRUE, Value, 0);
}

static VOID PhpHwEnumerateCpu(
    _Inout_ PPHP_HW_ENUM_CONTEXT Context
    )
{
    ULONG packageCount = PhHwGetPackageCount();
    PPH_HW_PACKAGE_SENSOR package;
    PPH_HW_CPU_SENSOR sensor;
    ULONG processorCount = 0;
    PBOOLEAN coreReported;

    while (PhHwGetCpuSensor(processorCount))
        processorCount++;

    for (ULONG i = 0; i < packageCount; i++)
    {
        if (!(package = PhHwGetPackageSensor(i)))
            break;

        if (package->Valid & PH_HW_CAP_PACKAGE_TEMP)
            PhpHwEmitValue(Context, i, PhpHwSensorPackageTemperature, 0, package->Temperature);
        if (package->Valid & PH_HW_CAP_CORES_TEMP)
            PhpHwEmitValue(Context, i, PhpHwSensorCoresTemperature, 0, package->CoresTemperature);
        if (package->Valid & PH_HW_CAP_GRAPHICS_TEMP)
            PhpHwEmitValue(Context, i, PhpHwSensorGraphicsTemperature, 0, package->GraphicsTemperature);

        if (package->Valid & PH_HW_CAP_CCD_TEMP)
        {
            for (ULONG ccd = 0; ccd < PH_HW_MAX_CCD; ccd++)
            {
                if (package->CcdValidMask & (1UL << ccd))
                    PhpHwEmitValue(Context, i, PhpHwSensorCcdTemperature, ccd, package->CcdTemperature[ccd]);
            }
        }

        if (package->Valid & PH_HW_CAP_RAPL_PACKAGE)
            PhpHwEmitValue(Context, i, PhpHwSensorPackagePower, 0, package->PackagePower);
        if (package->Valid & PH_HW_CAP_RAPL_CORE)
            PhpHwEmitValue(Context, i, PhpHwSensorCorePower, 0, package->CorePower);
        if (package->Valid & PH_HW_CAP_RAPL_DRAM)
            PhpHwEmitValue(Context, i, PhpHwSensorDramPower, 0, package->DramPower);
        if (package->Valid & PH_HW_CAP_RAPL_PSYS)
            PhpHwEmitValue(Context, i, PhpHwSensorPlatformPower, 0, package->PlatformPower);
        if (package->Valid & PH_HW_CAP_POWER_LIMIT1)
            PhpHwEmitValue(Context, i, PhpHwSensorPowerLimit1, 0, package->ActivePowerLimit1);
        if (package->Valid & PH_HW_CAP_POWER_LIMIT2)
            PhpHwEmitValue(Context, i, PhpHwSensorPowerLimit2, 0, package->ActivePowerLimit2);
        if (package->Valid & PH_HW_CAP_THROTTLE)
            PhpHwEmitSensor(Context, i, PhpHwSensorThrottle, 0, NULL, TRUE, 0.0f, package->ThrottleFlags);
    }

    // Logical processors of a core share its temperature; it is reported once, by the first of them.
    coreReported = PhAllocateZero(sizeof(BOOLEAN) * max(processorCount, 1));

    for (ULONG i = 0; i < processorCount; i++)
    {
        ULONG packageIndex;

        if (!(sensor = PhHwGetCpuSensor(i)))
            break;

        packageIndex = sensor->PackageIndex;

        if ((sensor->Valid & PH_HW_CAP_CORE_TEMP) && sensor->CoreIndex < processorCount && !coreReported[sensor->CoreIndex])
        {
            coreReported[sensor->CoreIndex] = TRUE;
            PhpHwEmitValue(Context, packageIndex, PhpHwSensorCoreTemperature, sensor->CoreIndex, sensor->Temperature);
        }
        if (sensor->Valid & PH_HW_CAP_EFFECTIVE_CLOCK)
            PhpHwEmitValue(Context, packageIndex, PhpHwSensorEffectiveClock, i, sensor->EffectiveMhz);
        if (sensor->Valid & PH_HW_CAP_ACTIVE_CLOCK)
            PhpHwEmitValue(Context, packageIndex, PhpHwSensorActiveClock, i, sensor->ActiveMhz);
        if (sensor->Valid & PH_HW_CAP_MULTIPLIER)
            PhpHwEmitValue(Context, packageIndex, PhpHwSensorMultiplier, i, sensor->Multiplier);
        if (sensor->Valid & PH_HW_CAP_VOLTAGE)
            PhpHwEmitValue(Context, packageIndex, PhpHwSensorVoltage, i, sensor->Voltage);
    }

    PhFree(coreReported);
}

static VOID PhpHwEnumerateMemory(
    _Inout_ PPHP_HW_ENUM_CONTEXT Context
    )
{
    ULONG packageCount = PhHwGetPackageCount();
    PPH_HW_PACKAGE_SENSOR package;

    for (ULONG i = 0; i < packageCount; i++)
    {
        if (!(package = PhHwGetPackageSensor(i)))
            break;

        if (package->Valid & PH_HW_CAP_MEMORY_CLOCK)
        {
            PhpHwEmitValue(Context, i, PhpHwSensorMemoryClock, 0, package->MemoryClockMhz);
            PhpHwEmitValue(Context, i, PhpHwSensorMemoryDataRate, 0, package->MemoryDataRate);

            if (package->MemoryGear)
                PhpHwEmitValue(Context, i, PhpHwSensorMemoryGear, 0, (FLOAT)package->MemoryGear);
        }

        if (package->Valid & PH_HW_CAP_MEMORY_BANDWIDTH)
        {
            PhpHwEmitValue(Context, i, PhpHwSensorMemoryReadBandwidth, 0, package->MemoryReadBandwidth);
            PhpHwEmitValue(Context, i, PhpHwSensorMemoryWriteBandwidth, 0, package->MemoryWriteBandwidth);
        }
    }
}

static VOID PhpHwEnumerateChipset(
    _Inout_ PPHP_HW_ENUM_CONTEXT Context
    )
{
    FLOAT celsius;

    if (PhHwGetChipsetTemperature(&celsius))
        PhpHwEmitValue(Context, 0, PhpHwSensorChipsetTemperature, 0, celsius);
}

static VOID PhpHwEnumerateNamed(
    _Inout_ PPHP_HW_ENUM_CONTEXT Context
    )
{
    ULONG count = PhHwGetNamedSensorCount();

    for (ULONG i = 0; i < count; i++)
    {
        PH_HW_NAMED_SENSOR sensor;
        PPH_STRING name;

        if (!PhHwGetNamedSensor(i, &sensor) || sensor.Kind >= PhHwNamedSensorMaximum)
            continue;

        sensor.Name[RTL_NUMBER_OF(sensor.Name) - 1] = UNICODE_NULL;
        name = PhCreateString(sensor.Name);

        PhpHwEmitSensor(
            Context,
            sensor.PackageIndex,
            PhpHwNamedSensorKinds[sensor.Kind],
            sensor.Id,
            name,
            sensor.Valid,
            sensor.Value,
            0
            );

        PhDereferenceObject(name);
    }
}

_Function_class_(PH_HW_SOURCE_SENSOR_CALLBACK)
static VOID NTAPI PhpHwEnumerateSourceSensor(
    _In_ PCPH_HW_SOURCE_SENSOR Sensor,
    _In_opt_ PVOID Context
    )
{
    PPHP_HW_ENUM_CONTEXT context = Context;
    PH_HW_SENSOR_ENTRY entry;

    if (!context || Sensor->Type >= RTL_NUMBER_OF(PhpHwSourceTypeInfo))
        return;

    entry.ComponentKind = PhHwComponentSource;
    entry.ComponentIndex = Sensor->ComponentId;
    entry.SourceId = Sensor->SourceId;
    entry.ComponentKey = PhpHwComponentKey(PhHwComponentSource, Sensor->SourceId, Sensor->ComponentId);
    entry.ComponentName = Sensor->ComponentName;
    entry.Group = PhpHwSourceTypeInfo[Sensor->Type].Group;
    entry.Unit = PhpHwSourceTypeInfo[Sensor->Type].Unit;
    entry.SensorKey = PhpHwSensorKey(PhpHwSensorMaximum, Sensor->SensorId);
    entry.SensorName = Sensor->SensorName;
    entry.Capability = 0;
    entry.Current = Sensor->Current;
    entry.History = !!(Sensor->Flags & PH_HW_SENSOR_REPORT_HISTORY);
    entry.Value = Sensor->Value;
    entry.ThrottleFlags = 0;

    context->Callback(&entry, context->Context);
}

/**
 * Enumerates the hardware sensors as the sensors window shows them.
 *
 * \param Callback Called once per sensor, in no particular order.
 * \param Context Passed to the callback.
 */
VOID NTAPI PhHwEnumerateSensors(
    _In_ PPH_HW_SENSOR_ENUM_CALLBACK Callback,
    _In_opt_ PVOID Context
    )
{
    PHP_HW_ENUM_CONTEXT context;

    context.Callback = Callback;
    context.Context = Context;
    context.ComponentKey = 0;
    context.ComponentName = NULL;

    if (PhHwGetCapabilities())
    {
        PhpHwEnumerateCpu(&context);
        PhpHwEnumerateMemory(&context);
        PhpHwEnumerateChipset(&context);
    }

    PhpHwEnumerateNamed(&context);
    PhHwEnumerateSourceSensors(PhpHwEnumerateSourceSensor, &context);

    PhClearReference(&context.ComponentName);
}

/**
 * Determines whether a temperature sensor is bounded by the CPU temperature limit.
 */
BOOLEAN PhHwSensorScalesToCpuLimit(
    _In_ PCPH_HW_SENSOR_ENTRY Entry
    )
{
    PHP_HW_SENSOR_KIND kind;

    if (Entry->Unit != PhHwUnitTemperature || Entry->ComponentKind == PhHwComponentSource)
        return FALSE;

    kind = PhpHwSensorKindFromKey(Entry->SensorKey);

    // A chipset, zone or DIMM temperature is not bounded by the CPU TjMax.
    return
        kind != PhpHwSensorChipsetTemperature &&
        kind != PhpHwSensorDimmTemperature &&
        kind != PhpHwSensorDimmPmicTemperature &&
        kind != PhpHwSensorAcpiTemperature &&
        kind != PhpHwSensorZoneTemperature;
}

PCWSTR PhHwGetSensorGroupName(
    _In_ PH_HW_SENSOR_GROUP Group
    )
{
    return Group < PhHwGroupMaximum ? PhpHwGroupInfo[Group].Name : L"";
}

BOOLEAN PhHwIsSensorGroupPerProcessor(
    _In_ PH_HW_SENSOR_GROUP Group
    )
{
    return Group < PhHwGroupMaximum && PhpHwGroupInfo[Group].PerProcessor;
}

_Ret_maybenull_
PPH_STRING PhHwFormatSensorValue(
    _In_ PH_HW_SENSOR_UNIT Unit,
    _In_ FLOAT Value
    )
{
    PH_FORMAT format[PH_HW_TEMPERATURE_FORMAT_COUNT];

    C_ASSERT(PH_HW_TEMPERATURE_FORMAT_COUNT == 2);

    switch (Unit)
    {
    case PhHwUnitTemperature:
        PhHwFormatTemperature(Value, 1, format);
        break;
    case PhHwUnitPower:
        PhInitFormatF(&format[0], Value, 1);
        PhInitFormatS(&format[1], L" W");
        break;
    case PhHwUnitClock:
        PhInitFormatF(&format[0], Value, 0);
        PhInitFormatS(&format[1], L" MHz");
        break;
    case PhHwUnitDataRate:
        PhInitFormatF(&format[0], Value, 0);
        PhInitFormatS(&format[1], L" MT/s");
        break;
    case PhHwUnitMultiplier:
        PhInitFormatF(&format[0], Value, 2);
        PhInitFormatS(&format[1], L"x");
        break;
    case PhHwUnitVoltage:
        PhInitFormatF(&format[0], Value, 3);
        PhInitFormatS(&format[1], L" V");
        break;
    case PhHwUnitBandwidth:
        PhInitFormatF(&format[0], Value, 0);
        PhInitFormatS(&format[1], L" MB/s");
        break;
    case PhHwUnitGear:
        PhInitFormatF(&format[0], Value, 0);
        PhInitFormatS(&format[1], L"");
        break;
    case PhHwUnitCurrent:
        PhInitFormatF(&format[0], Value, 2);
        PhInitFormatS(&format[1], L" A");
        break;
    case PhHwUnitFan:
        PhInitFormatF(&format[0], Value, 0);
        PhInitFormatS(&format[1], L" RPM");
        break;
    case PhHwUnitPercent:
        PhInitFormatF(&format[0], Value, 1);
        PhInitFormatS(&format[1], L"%");
        break;
    default:
        return NULL;
    }

    return PhFormat(format, RTL_NUMBER_OF(format), 0);
}

PPH_STRING PhHwFormatThrottleFlags(
    _In_ ULONG ThrottleFlags
    )
{
    static CONST struct { ULONG Flag; PCWSTR Name; } throttleNames[] =
    {
        { PH_HW_THROTTLE_THERMAL, L"Thermal" },
        { PH_HW_THROTTLE_PROCHOT, L"PROCHOT" },
        { PH_HW_THROTTLE_CRITICAL, L"Critical" },
        { PH_HW_THROTTLE_POWER_LIMIT, L"Power limit" },
        { PH_HW_THROTTLE_PL1, L"PL1" },
        { PH_HW_THROTTLE_PL2, L"PL2" },
        { PH_HW_THROTTLE_VR_THERMAL, L"VR thermal" },
        { PH_HW_THROTTLE_VR_CURRENT, L"VR current" },
        { PH_HW_THROTTLE_ELECTRICAL, L"Electrical" },
        { PH_HW_THROTTLE_MAX_TURBO, L"Max turbo" },
        { PH_HW_THROTTLE_TURBO_ATTENUATION, L"Turbo attenuation" },
        { PH_HW_THROTTLE_RUNNING_AVERAGE_THERMAL, L"Running average thermal" },
        { PH_HW_THROTTLE_RESIDENCY, L"Residency" },
        { PH_HW_THROTTLE_GRAPHICS, L"Graphics" },
        { PH_HW_THROTTLE_AUTONOMOUS, L"Autonomous" },
    };
    PH_FORMAT format[RTL_NUMBER_OF(throttleNames) * 2];
    ULONG count = 0;

    for (ULONG i = 0; i < RTL_NUMBER_OF(throttleNames); i++)
    {
        if (!(ThrottleFlags & throttleNames[i].Flag))
            continue;

        if (count)
            PhInitFormatS(&format[count++], L", ");

        PhInitFormatS(&format[count++], throttleNames[i].Name);
    }

    if (!count)
        PhInitFormatS(&format[count++], L"None");

    return PhFormat(format, count, 0);
}

typedef struct _PHP_HW_TEXT_ROW
{
    ULONG64 ComponentKey;
    PH_HW_SENSOR_GROUP Group;
    ULONG64 SensorKey;
    PPH_STRING ComponentName;
    PPH_STRING SensorName;
    PPH_STRING ValueText;
} PHP_HW_TEXT_ROW, *PPHP_HW_TEXT_ROW;

_Function_class_(PH_HW_SENSOR_ENUM_CALLBACK)
static VOID NTAPI PhpHwCollectTextRow(
    _In_ PCPH_HW_SENSOR_ENTRY Entry,
    _In_opt_ PVOID Context
    )
{
    PPH_LIST rows = Context;
    PPHP_HW_TEXT_ROW row;

    if (!rows)
        return;

    // A named sensor is shown from its first current value.
    if (!Entry->Current && !Entry->Capability && Entry->ComponentKind != PhHwComponentSource)
        return;

    row = PhAllocate(sizeof(PHP_HW_TEXT_ROW));
    row->ComponentKey = Entry->ComponentKey;
    row->Group = Entry->Group;
    row->SensorKey = Entry->SensorKey;
    row->ComponentName = PhReferenceObject(Entry->ComponentName);
    row->SensorName = PhReferenceObject(Entry->SensorName);

    if (!Entry->Current)
        row->ValueText = NULL;
    else if (Entry->Unit == PhHwUnitThrottle)
        row->ValueText = PhHwFormatThrottleFlags(Entry->ThrottleFlags);
    else
        row->ValueText = PhHwFormatSensorValue(Entry->Unit, Entry->Value);

    PhAddItemList(rows, row);
}

static int __cdecl PhpHwTextRowCompare(
    _In_ void* Context,
    _In_ CONST void* Left,
    _In_ CONST void* Right
    )
{
    PPHP_HW_TEXT_ROW left = *(PPHP_HW_TEXT_ROW*)Left;
    PPHP_HW_TEXT_ROW right = *(PPHP_HW_TEXT_ROW*)Right;

    UNREFERENCED_PARAMETER(Context);

    if (left->ComponentKey != right->ComponentKey)
        return left->ComponentKey < right->ComponentKey ? -1 : 1;
    if (left->Group != right->Group)
        return left->Group < right->Group ? -1 : 1;
    if (left->SensorKey != right->SensorKey)
        return left->SensorKey < right->SensorKey ? -1 : 1;

    return 0;
}

/**
 * Formats the current sensors as the sensors window lists them, without the ranges.
 *
 * \return The text, empty when there are no sensors.
 */
PPH_STRING PhHwFormatSensorsText(
    VOID
    )
{
    PH_STRING_BUILDER builder;
    PPH_LIST rows;
    PPHP_HW_TEXT_ROW previous = NULL;

    rows = PhCreateList(64);
    PhHwEnumerateSensors(PhpHwCollectTextRow, rows);
    qsort_s(rows->Items, rows->Count, sizeof(PVOID), PhpHwTextRowCompare, NULL);

    PhInitializeStringBuilder(&builder, 0x1000);

    for (ULONG i = 0; i < rows->Count; i++)
    {
        PPHP_HW_TEXT_ROW row = rows->Items[i];

        if (!previous || previous->ComponentKey != row->ComponentKey)
        {
            PhAppendFormatStringBuilder(&builder, L"%s\n", PhGetStringOrEmpty(row->ComponentName));
            previous = NULL;
        }

        if (!previous || previous->Group != row->Group)
            PhAppendFormatStringBuilder(&builder, L"  %s\n", PhHwGetSensorGroupName(row->Group));

        PhAppendFormatStringBuilder(
            &builder,
            L"    %s: %s\n",
            PhGetStringOrEmpty(row->SensorName),
            PhGetStringOrEmpty(row->ValueText)
            );

        previous = row;
    }

    for (ULONG i = 0; i < rows->Count; i++)
    {
        PPHP_HW_TEXT_ROW row = rows->Items[i];

        PhDereferenceObject(row->ComponentName);
        PhDereferenceObject(row->SensorName);
        PhClearReference(&row->ValueText);
        PhFree(row);
    }

    PhDereferenceObject(rows);

    return PhFinalStringBuilderString(&builder);
}
