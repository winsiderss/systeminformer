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
#include <phsettings.h>
#include <settings.h>
#include <colmgr.h>
#include <cpysave.h>
#include <emenu.h>
#include <hwprv.h>
#include <procprv.h>
#include <sysinfo.h>
#include <sysinfop.h>

#define WM_PH_HW_SENSORS_UPDATE (WM_APP + 1)
#define PHP_HW_COLLAPSE_PROCESSOR_COUNT 16
#define PHP_HW_POWER_GRAPH_MINIMUM 10.0f

typedef enum _PHP_HW_SENSORS_COLUMN
{
    PhpHwSensorsColumnName,
    PhpHwSensorsColumnValue,
    PhpHwSensorsColumnMinimum,
    PhpHwSensorsColumnMaximum,
    PhpHwSensorsColumnHistory,
    PhpHwSensorsColumnMaximumId
} PHP_HW_SENSORS_COLUMN;

typedef enum _PHP_HW_COMPONENT_KIND
{
    PhpHwComponentCpu, // one per package
    PhpHwComponentMemory, // one per package
    PhpHwComponentChipset,
    PhpHwComponentSystem,
    PhpHwComponentWindows, // Windows performance counters
    PhpHwComponentSource, // published by a hardware sensor source
    PhpHwComponentMaximum
} PHP_HW_COMPONENT_KIND;

typedef enum _PHP_HW_GROUP
{
    PhpHwGroupTemperatures,
    PhpHwGroupPowers,
    PhpHwGroupLimits,
    PhpHwGroupClocks,
    PhpHwGroupBandwidth,
    PhpHwGroupActiveClocks,
    PhpHwGroupMultipliers,
    PhpHwGroupVoltages,
    PhpHwGroupCurrents,
    PhpHwGroupResidency,
    PhpHwGroupFans,
    PhpHwGroupControls,
    PhpHwGroupThrottling,
    PhpHwGroupMaximum
} PHP_HW_GROUP;

typedef struct _PHP_HW_GROUP_INFO
{
    PCWSTR Name;
    BOOLEAN PerProcessor; // collapsed by default on large systems
} PHP_HW_GROUP_INFO, *PPHP_HW_GROUP_INFO;
typedef CONST PHP_HW_GROUP_INFO *PCPHP_HW_GROUP_INFO;

static CONST PHP_HW_GROUP_INFO PhpHwGroupInfo[PhpHwGroupMaximum] =
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
    PhpHwSensorMaximum
} PHP_HW_SENSOR_KIND;

typedef enum _PHP_HW_SENSOR_UNIT
{
    PhpHwUnitTemperature,
    PhpHwUnitPower,
    PhpHwUnitClock,
    PhpHwUnitDataRate,
    PhpHwUnitMultiplier,
    PhpHwUnitVoltage,
    PhpHwUnitThrottle,
    PhpHwUnitBandwidth,
    PhpHwUnitGear,
    PhpHwUnitCurrent,
    PhpHwUnitFan,
    PhpHwUnitPercent
} PHP_HW_SENSOR_UNIT;

typedef struct _PHP_HW_SENSOR_INFO
{
    PHP_HW_COMPONENT_KIND Component;
    PHP_HW_GROUP Group;
    ULONG Capability;
    PHP_HW_SENSOR_UNIT Unit;
    PCWSTR Name; // followed by the index when Indexed; NULL for named sensors
    BOOLEAN Indexed;
    BOOLEAN History;
} PHP_HW_SENSOR_INFO, *PPHP_HW_SENSOR_INFO;
typedef CONST PHP_HW_SENSOR_INFO *PCPHP_HW_SENSOR_INFO;

static CONST PHP_HW_SENSOR_INFO PhpHwSensorInfo[PhpHwSensorMaximum] =
{
    { PhpHwComponentCpu, PhpHwGroupTemperatures, PH_HW_CAP_PACKAGE_TEMP, PhpHwUnitTemperature, L"Package", FALSE, TRUE },
    { PhpHwComponentCpu, PhpHwGroupTemperatures, PH_HW_CAP_CORES_TEMP, PhpHwUnitTemperature, L"IA cores", FALSE, TRUE },
    { PhpHwComponentCpu, PhpHwGroupTemperatures, PH_HW_CAP_GRAPHICS_TEMP, PhpHwUnitTemperature, L"Graphics", FALSE, TRUE },
    { PhpHwComponentCpu, PhpHwGroupTemperatures, PH_HW_CAP_CCD_TEMP, PhpHwUnitTemperature, L"CCD ", TRUE, TRUE },
    { PhpHwComponentCpu, PhpHwGroupTemperatures, PH_HW_CAP_CORE_TEMP, PhpHwUnitTemperature, L"Core ", TRUE, TRUE },
    { PhpHwComponentCpu, PhpHwGroupTemperatures, PH_HW_CAP_SMU, PhpHwUnitTemperature, NULL, FALSE, TRUE },
    { PhpHwComponentCpu, PhpHwGroupTemperatures, PH_HW_CAP_TPMI_POWER, PhpHwUnitTemperature, NULL, FALSE, TRUE },
    { PhpHwComponentChipset, PhpHwGroupTemperatures, PH_HW_CAP_CHIPSET_TEMP, PhpHwUnitTemperature, L"PCH", FALSE, TRUE },
    { PhpHwComponentMemory, PhpHwGroupTemperatures, PH_HW_CAP_DIMM_TEMP, PhpHwUnitTemperature, NULL, FALSE, TRUE },
    { PhpHwComponentMemory, PhpHwGroupTemperatures, PH_HW_CAP_DIMM_PMIC, PhpHwUnitTemperature, NULL, FALSE, FALSE },
    { PhpHwComponentSystem, PhpHwGroupTemperatures, PH_HW_CAP_ACPI_TEMP, PhpHwUnitTemperature, NULL, FALSE, TRUE },
    { PhpHwComponentWindows, PhpHwGroupTemperatures, 0, PhpHwUnitTemperature, NULL, FALSE, TRUE },
    { PhpHwComponentCpu, PhpHwGroupPowers, PH_HW_CAP_RAPL_PACKAGE, PhpHwUnitPower, L"Package", FALSE, TRUE },
    { PhpHwComponentCpu, PhpHwGroupPowers, PH_HW_CAP_RAPL_CORE, PhpHwUnitPower, L"Core", FALSE, TRUE },
    { PhpHwComponentCpu, PhpHwGroupPowers, PH_HW_CAP_RAPL_DRAM, PhpHwUnitPower, L"DRAM", FALSE, TRUE },
    { PhpHwComponentCpu, PhpHwGroupPowers, PH_HW_CAP_RAPL_PSYS, PhpHwUnitPower, L"Platform", FALSE, TRUE },
    { PhpHwComponentCpu, PhpHwGroupPowers, PH_HW_CAP_TPMI_POWER, PhpHwUnitPower, NULL, FALSE, TRUE },
    { PhpHwComponentCpu, PhpHwGroupPowers, PH_HW_CAP_SMU, PhpHwUnitPower, NULL, FALSE, TRUE },
    { PhpHwComponentMemory, PhpHwGroupPowers, PH_HW_CAP_DIMM_PMIC, PhpHwUnitPower, NULL, FALSE, TRUE },
    { PhpHwComponentSystem, PhpHwGroupPowers, PH_HW_CAP_ACPI_POWER, PhpHwUnitPower, NULL, FALSE, TRUE },
    { PhpHwComponentWindows, PhpHwGroupPowers, 0, PhpHwUnitPower, NULL, FALSE, TRUE },
    { PhpHwComponentCpu, PhpHwGroupLimits, PH_HW_CAP_POWER_LIMIT1, PhpHwUnitPower, L"PL1 (active)", FALSE, FALSE },
    { PhpHwComponentCpu, PhpHwGroupLimits, PH_HW_CAP_POWER_LIMIT2, PhpHwUnitPower, L"PL2 (active)", FALSE, FALSE },
    { PhpHwComponentCpu, PhpHwGroupLimits, PH_HW_CAP_SMU, PhpHwUnitPower, NULL, FALSE, FALSE },
    { PhpHwComponentCpu, PhpHwGroupLimits, PH_HW_CAP_PLATFORM_LIMITS, PhpHwUnitPower, NULL, FALSE, FALSE },
    { PhpHwComponentCpu, PhpHwGroupLimits, PH_HW_CAP_SMU, PhpHwUnitCurrent, NULL, FALSE, FALSE },
    { PhpHwComponentCpu, PhpHwGroupLimits, PH_HW_CAP_SMU, PhpHwUnitTemperature, NULL, FALSE, FALSE },
    { PhpHwComponentCpu, PhpHwGroupLimits, PH_HW_CAP_SMU, PhpHwUnitClock, NULL, FALSE, FALSE },
    { PhpHwComponentCpu, PhpHwGroupClocks, PH_HW_CAP_UNCORE_CLOCK, PhpHwUnitClock, NULL, FALSE, FALSE },
    { PhpHwComponentCpu, PhpHwGroupClocks, PH_HW_CAP_SMU, PhpHwUnitClock, NULL, FALSE, FALSE },
    { PhpHwComponentCpu, PhpHwGroupClocks, PH_HW_CAP_EFFECTIVE_CLOCK, PhpHwUnitClock, L"CPU ", TRUE, FALSE },
    { PhpHwComponentCpu, PhpHwGroupActiveClocks, PH_HW_CAP_ACTIVE_CLOCK, PhpHwUnitClock, L"CPU ", TRUE, FALSE },
    { PhpHwComponentWindows, PhpHwGroupClocks, 0, PhpHwUnitClock, NULL, FALSE, FALSE },
    { PhpHwComponentMemory, PhpHwGroupClocks, PH_HW_CAP_MEMORY_CLOCK, PhpHwUnitClock, L"Memory clock", FALSE, FALSE },
    { PhpHwComponentMemory, PhpHwGroupClocks, PH_HW_CAP_MEMORY_CLOCK, PhpHwUnitDataRate, L"Data rate", FALSE, FALSE },
    { PhpHwComponentMemory, PhpHwGroupClocks, PH_HW_CAP_MEMORY_CLOCK, PhpHwUnitGear, L"Gear", FALSE, FALSE },
    { PhpHwComponentMemory, PhpHwGroupClocks, PH_HW_CAP_SMU, PhpHwUnitClock, NULL, FALSE, FALSE },
    { PhpHwComponentMemory, PhpHwGroupBandwidth, PH_HW_CAP_MEMORY_BANDWIDTH, PhpHwUnitBandwidth, L"DRAM read", FALSE, TRUE },
    { PhpHwComponentMemory, PhpHwGroupBandwidth, PH_HW_CAP_MEMORY_BANDWIDTH, PhpHwUnitBandwidth, L"DRAM write", FALSE, TRUE },
    { PhpHwComponentMemory, PhpHwGroupBandwidth, PH_HW_CAP_SMU, PhpHwUnitPercent, NULL, FALSE, TRUE },
    { PhpHwComponentMemory, PhpHwGroupBandwidth, PH_HW_CAP_SMU, PhpHwUnitBandwidth, NULL, FALSE, TRUE },
    { PhpHwComponentCpu, PhpHwGroupMultipliers, PH_HW_CAP_MULTIPLIER, PhpHwUnitMultiplier, L"CPU ", TRUE, FALSE },
    { PhpHwComponentCpu, PhpHwGroupVoltages, PH_HW_CAP_VOLTAGE, PhpHwUnitVoltage, L"CPU ", TRUE, FALSE },
    { PhpHwComponentCpu, PhpHwGroupVoltages, PH_HW_CAP_SMU, PhpHwUnitVoltage, NULL, FALSE, FALSE },
    { PhpHwComponentMemory, PhpHwGroupVoltages, PH_HW_CAP_DIMM_VOLTAGE, PhpHwUnitVoltage, NULL, FALSE, FALSE },
    { PhpHwComponentSystem, PhpHwGroupVoltages, PH_HW_CAP_ACPI_TEMP, PhpHwUnitVoltage, NULL, FALSE, FALSE },
    { PhpHwComponentCpu, PhpHwGroupCurrents, PH_HW_CAP_SMU, PhpHwUnitCurrent, NULL, FALSE, FALSE },
    { PhpHwComponentMemory, PhpHwGroupCurrents, PH_HW_CAP_DIMM_PMIC, PhpHwUnitCurrent, NULL, FALSE, TRUE },
    { PhpHwComponentCpu, PhpHwGroupResidency, PH_HW_CAP_SMU, PhpHwUnitPercent, NULL, FALSE, FALSE },
    { PhpHwComponentCpu, PhpHwGroupResidency, PH_HW_CAP_RESIDENCY, PhpHwUnitPercent, NULL, FALSE, FALSE },
    { PhpHwComponentSystem, PhpHwGroupFans, PH_HW_CAP_FAN_SPEED, PhpHwUnitFan, NULL, FALSE, TRUE },
    { PhpHwComponentSystem, PhpHwGroupControls, PH_HW_CAP_FAN_CONTROL, PhpHwUnitPercent, NULL, FALSE, TRUE },
    { PhpHwComponentCpu, PhpHwGroupThrottling, PH_HW_CAP_THROTTLE, PhpHwUnitThrottle, L"Package", FALSE, FALSE },
    { PhpHwComponentWindows, PhpHwGroupThrottling, 0, PhpHwUnitPercent, NULL, FALSE, FALSE },
    { PhpHwComponentWindows, PhpHwGroupThrottling, 0, PhpHwUnitPercent, NULL, FALSE, FALSE },
};

// Group and unit of each PH_HW_SENSOR_TYPE; source sensors use PhpHwSensorMaximum as their kind.
static CONST struct
{
    PHP_HW_GROUP Group;
    PHP_HW_SENSOR_UNIT Unit;
} PhpHwSourceTypeInfo[PhHwSensorTypeMaximum] =
{
    { PhpHwGroupTemperatures, PhpHwUnitTemperature },
    { PhpHwGroupPowers, PhpHwUnitPower },
    { PhpHwGroupClocks, PhpHwUnitClock },
    { PhpHwGroupVoltages, PhpHwUnitVoltage },
    { PhpHwGroupCurrents, PhpHwUnitCurrent },
    { PhpHwGroupFans, PhpHwUnitFan },
    { PhpHwGroupControls, PhpHwUnitPercent },
    { PhpHwGroupBandwidth, PhpHwUnitBandwidth },
};

typedef enum _PHP_HW_NODE_TYPE
{
    PhpHwNodeComponent,
    PhpHwNodeGroup,
    PhpHwNodeSensor
} PHP_HW_NODE_TYPE;

typedef struct _PHP_HW_NODE
{
    PH_TREENEW_NODE Node;
    PHP_HW_NODE_TYPE Type;
    ULONG64 Key; // order among siblings
    PPH_STRING Name;
    PPH_LIST Children; // component: groups with sensors; group: all sensors ordered by key

    union
    {
        struct
        {
            struct _PHP_HW_NODE *Groups[PhpHwGroupMaximum];
            PHP_HW_COMPONENT_KIND Kind;
        } Component;
        struct
        {
            struct _PHP_HW_NODE *Group;
            PHP_HW_SENSOR_KIND Kind; // PhpHwSensorMaximum for a source sensor
            PHP_HW_SENSOR_UNIT Unit;
            ULONG Capability; // core sensors: removed when the capability is lost
            BOOLEAN History;
            BOOLEAN Listed; // source and named sensors: removed when no longer listed by the provider
            BOOLEAN Current;
            BOOLEAN RangeValid;
            FLOAT Value;
            FLOAT Minimum;
            FLOAT Maximum;
            ULONG ThrottleFlags;
            PPH_STRING ValueText;
            PPH_STRING MinimumText;
            PPH_STRING MaximumText;
            PH_CIRCULAR_BUFFER_FLOAT HistoryBuffer; // valid when History
            PH_GRAPH_BUFFERS GraphBuffers;
        } Sensor;
    };
} PHP_HW_NODE, *PPHP_HW_NODE;

typedef struct _PHP_HW_SENSORS_CONTEXT
{
    HWND WindowHandle;
    HWND TreeNewHandle;
    HWND SearchWindowHandle;
    ULONG_PTR SearchMatchHandle;
    PCPH_STRINGREF EmptyText;
    PH_LAYOUT_MANAGER LayoutManager;
    PH_CALLBACK_REGISTRATION ProcessesUpdatedRegistration;
    PPH_LIST ComponentList; // all components ordered by key
    PPH_LIST RootList; // components with sensors
    PPH_LIST SensorList; // all sensors, unordered
    PPH_STRING CpuBrandString;
    ULONG ProcessorCount;
    BOOLEAN StructureChanged;

    HDC GraphContext;
    HBITMAP GraphBitmap;
    HBITMAP GraphOldBitmap;
    PVOID GraphBits;
    LONG GraphWidth;
    LONG GraphHeight;
} PHP_HW_SENSORS_CONTEXT, *PPHP_HW_SENSORS_CONTEXT;

static HWND PhpHwSensorsWindowHandle = NULL;

INT_PTR CALLBACK PhpHwSensorsDlgProc(
    _In_ HWND hwndDlg,
    _In_ UINT uMsg,
    _In_ WPARAM wParam,
    _In_ LPARAM lParam
    );

/**
 * Determines whether the hardware sensors window can show sensors: the provider gate is open or a
 * sensor source reports sensors.
 */
BOOLEAN PhHwSensorsWindowAvailable(
    VOID
    )
{
    return PhHwSensorsEnabled() || PhHwSourceSensorsPresent() || PhHwCounterSensorsPresent();
}

VOID PhShowHardwareSensorsWindow(
    _In_ HWND ParentWindowHandle
    )
{
    if (!PhpHwSensorsWindowHandle)
    {
        PhpHwSensorsWindowHandle = PhCreateDialog(
            NtCurrentImageBase(),
            MAKEINTRESOURCE(IDD_HWSENSORS),
            PhCsForceNoParent ? NULL : ParentWindowHandle,
            PhpHwSensorsDlgProc,
            NULL
            );
        PhRegisterDialog(PhpHwSensorsWindowHandle);
        ShowWindow(PhpHwSensorsWindowHandle, SW_SHOW);
    }

    if (IsMinimized(PhpHwSensorsWindowHandle))
        ShowWindow(PhpHwSensorsWindowHandle, SW_RESTORE);
    else
        SetForegroundWindow(PhpHwSensorsWindowHandle);
}

_Function_class_(PH_CALLBACK_FUNCTION)
static VOID NTAPI PhpHwSensorsProcessesUpdatedHandler(
    _In_opt_ PVOID Parameter,
    _In_opt_ PVOID Context
    )
{
    PPHP_HW_SENSORS_CONTEXT context = Context;

    if (context)
        PostMessage(context->WindowHandle, WM_PH_HW_SENSORS_UPDATE, 0, 0);
}

static PPH_STRING PhpHwSensorFormatValue(
    _In_ PHP_HW_SENSOR_UNIT Unit,
    _In_ FLOAT Value
    )
{
    PH_FORMAT format[PH_HW_TEMPERATURE_FORMAT_COUNT];

    C_ASSERT(PH_HW_TEMPERATURE_FORMAT_COUNT == 2);

    switch (Unit)
    {
    case PhpHwUnitTemperature:
        PhHwFormatTemperature(Value, 1, format);
        break;
    case PhpHwUnitPower:
        PhInitFormatF(&format[0], Value, 1);
        PhInitFormatS(&format[1], L" W");
        break;
    case PhpHwUnitClock:
        PhInitFormatF(&format[0], Value, 0);
        PhInitFormatS(&format[1], L" MHz");
        break;
    case PhpHwUnitDataRate:
        PhInitFormatF(&format[0], Value, 0);
        PhInitFormatS(&format[1], L" MT/s");
        break;
    case PhpHwUnitMultiplier:
        PhInitFormatF(&format[0], Value, 2);
        PhInitFormatS(&format[1], L"x");
        break;
    case PhpHwUnitVoltage:
        PhInitFormatF(&format[0], Value, 3);
        PhInitFormatS(&format[1], L" V");
        break;
    case PhpHwUnitBandwidth:
        PhInitFormatF(&format[0], Value, 0);
        PhInitFormatS(&format[1], L" MB/s");
        break;
    case PhpHwUnitGear:
        PhInitFormatF(&format[0], Value, 0);
        PhInitFormatS(&format[1], L"");
        break;
    case PhpHwUnitCurrent:
        PhInitFormatF(&format[0], Value, 2);
        PhInitFormatS(&format[1], L" A");
        break;
    case PhpHwUnitFan:
        PhInitFormatF(&format[0], Value, 0);
        PhInitFormatS(&format[1], L" RPM");
        break;
    case PhpHwUnitPercent:
        PhInitFormatF(&format[0], Value, 1);
        PhInitFormatS(&format[1], L"%");
        break;
    default:
        return NULL;
    }

    return PhFormat(format, RTL_NUMBER_OF(format), 0);
}

static PPH_STRING PhpHwSensorFormatThrottle(
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

static PPH_STRING PhpHwFormatComponentName(
    _In_ PPHP_HW_SENSORS_CONTEXT Context,
    _In_ PHP_HW_COMPONENT_KIND Kind,
    _In_ ULONG Index
    )
{
    switch (Kind)
    {
    case PhpHwComponentCpu:
        {
            PH_FORMAT format[4];

            if (PhHwGetPackageCount() <= 1)
                return PhReferenceObject(Context->CpuBrandString);

            PhInitFormatS(&format[0], L"CPU #");
            PhInitFormatU(&format[1], Index);
            PhInitFormatS(&format[2], L": ");
            PhInitFormatSR(&format[3], Context->CpuBrandString->sr);

            return PhFormat(format, RTL_NUMBER_OF(format), 0);
        }
    case PhpHwComponentMemory:
        {
            PH_FORMAT format[2];

            if (PhHwGetPackageCount() <= 1)
                return PhCreateString(L"Memory");

            PhInitFormatS(&format[0], L"Memory #");
            PhInitFormatU(&format[1], Index);

            return PhFormat(format, RTL_NUMBER_OF(format), 0);
        }
    case PhpHwComponentChipset:
        return PhCreateString(L"Chipset");
    case PhpHwComponentSystem:
        return PhCreateString(L"System");
    case PhpHwComponentWindows:
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

static PPHP_HW_NODE PhpHwCreateNode(
    _In_ PHP_HW_NODE_TYPE Type,
    _In_ ULONG64 Key,
    _In_ _Assume_refs_(1) PPH_STRING Name
    )
{
    PPHP_HW_NODE node;

    node = PhAllocateZero(sizeof(PHP_HW_NODE));
    PhInitializeTreeNewNode(&node->Node);
    node->Type = Type;
    node->Key = Key;
    node->Name = Name;

    if (Type != PhpHwNodeSensor)
        node->Children = PhCreateList(8);

    return node;
}

static VOID PhpHwDestroyNode(
    _In_ _Post_invalid_ PPHP_HW_NODE Node
    )
{
    if (Node->Type == PhpHwNodeSensor)
    {
        PhClearReference(&Node->Sensor.ValueText);
        PhClearReference(&Node->Sensor.MinimumText);
        PhClearReference(&Node->Sensor.MaximumText);

        if (Node->Sensor.History)
            PhDeleteCircularBuffer_FLOAT(&Node->Sensor.HistoryBuffer);

        PhDeleteGraphBuffers(&Node->Sensor.GraphBuffers);
    }

    if (Node->Children)
        PhDereferenceObject(Node->Children);

    PhClearReference(&Node->Name);
    PhFree(Node);
}

// Position receives the node's position, or the insertion point when not found.
static PPHP_HW_NODE PhpHwFindNode(
    _In_ PPH_LIST List,
    _In_ ULONG64 Key,
    _Out_ PULONG Position
    )
{
    ULONG low = 0;
    ULONG high = List->Count;

    while (low < high)
    {
        ULONG middle = low + (high - low) / 2;
        PPHP_HW_NODE node = List->Items[middle];

        if (node->Key == Key)
        {
            *Position = middle;
            return node;
        }

        if (node->Key < Key)
            low = middle + 1;
        else
            high = middle;
    }

    *Position = low;
    return NULL;
}

// Ordered by kind, source registration, then index; SourceId and Name apply to source components only.
static PPHP_HW_NODE PhpHwGetComponent(
    _In_ PPHP_HW_SENSORS_CONTEXT Context,
    _In_ PHP_HW_COMPONENT_KIND Kind,
    _In_ ULONG SourceId,
    _In_ ULONG Index,
    _In_opt_ PPH_STRING Name
    )
{
    ULONG64 key = ((ULONG64)Kind << 56) | ((ULONG64)(SourceId & 0xFFFFFF) << 32) | Index;
    PPHP_HW_NODE component;
    ULONG position;

    component = PhpHwFindNode(Context->ComponentList, key, &position);

    if (component)
        return component;

    component = PhpHwCreateNode(
        PhpHwNodeComponent,
        key,
        Name ? PhReferenceObject(Name) : PhpHwFormatComponentName(Context, Kind, Index)
        );
    component->Component.Kind = Kind;
    PhInsertItemList(Context->ComponentList, position, component);

    return component;
}

static PPHP_HW_NODE PhpHwGetGroup(
    _In_ PPHP_HW_SENSORS_CONTEXT Context,
    _In_ PPHP_HW_NODE Component,
    _In_ PHP_HW_GROUP Group
    )
{
    PCPHP_HW_GROUP_INFO info = &PhpHwGroupInfo[Group];
    PPHP_HW_NODE group;

    group = Component->Component.Groups[Group];

    if (group)
        return group;

    group = PhpHwCreateNode(PhpHwNodeGroup, Group, PhCreateString(info->Name));
    group->Node.Expanded = !(
        info->PerProcessor &&
        Component->Component.Kind == PhpHwComponentCpu &&
        Context->ProcessorCount > PHP_HW_COLLAPSE_PROCESSOR_COUNT
        );
    Component->Component.Groups[Group] = group;

    return group;
}

static PPHP_HW_NODE PhpHwInsertSensor(
    _In_ PPHP_HW_SENSORS_CONTEXT Context,
    _In_ PPHP_HW_NODE Group,
    _In_ ULONG Position,
    _In_ ULONG64 Key,
    _In_ _Assume_refs_(1) PPH_STRING Name,
    _In_ PHP_HW_SENSOR_KIND Kind,
    _In_ PHP_HW_SENSOR_UNIT Unit,
    _In_ ULONG Capability,
    _In_ BOOLEAN History
    )
{
    PPHP_HW_NODE sensor;

    sensor = PhpHwCreateNode(PhpHwNodeSensor, Key, Name);
    sensor->Sensor.Group = Group;
    sensor->Sensor.Kind = Kind;
    sensor->Sensor.Unit = Unit;
    sensor->Sensor.Capability = Capability;
    sensor->Sensor.History = History;

    if (History)
        PhInitializeCircularBuffer_FLOAT(&sensor->Sensor.HistoryBuffer, PhStatisticsSampleCount);

    PhInitializeGraphBuffers(&sensor->Sensor.GraphBuffers);

    PhInsertItemList(Group->Children, Position, sensor);
    PhAddItemList(Context->SensorList, sensor);
    Context->StructureChanged = TRUE;

    return sensor;
}

// Creates the node and its parents as needed; a NULL Name is formatted from the kind and index.
static PPHP_HW_NODE PhpHwGetSensor(
    _In_ PPHP_HW_SENSORS_CONTEXT Context,
    _In_ ULONG ComponentIndex,
    _In_ PHP_HW_SENSOR_KIND Kind,
    _In_ ULONG Index,
    _In_opt_ PCWSTR Name
    )
{
    PCPHP_HW_SENSOR_INFO info = &PhpHwSensorInfo[Kind];
    ULONG64 key = ((ULONG64)Kind << 32) | Index;
    PPHP_HW_NODE group;
    PPHP_HW_NODE sensor;
    ULONG position;

    group = PhpHwGetGroup(Context, PhpHwGetComponent(Context, info->Component, 0, ComponentIndex, NULL), info->Group);

    sensor = PhpHwFindNode(group->Children, key, &position);

    if (sensor)
        return sensor;

    return PhpHwInsertSensor(
        Context,
        group,
        position,
        key,
        Name ? PhCreateString((PWSTR)Name) : PhpHwFormatSensorName(Kind, Index),
        Kind,
        info->Unit,
        info->Capability,
        info->History
        );
}

static VOID PhpHwSensorUpdateValue(
    _Inout_ PPHP_HW_NODE Sensor,
    _In_ FLOAT Value
    )
{
    Sensor->Sensor.Current = TRUE;
    Sensor->Sensor.Value = Value;

    if (!Sensor->Sensor.RangeValid)
    {
        Sensor->Sensor.Minimum = Value;
        Sensor->Sensor.Maximum = Value;
        Sensor->Sensor.RangeValid = TRUE;
    }
    else
    {
        if (Value < Sensor->Sensor.Minimum)
            Sensor->Sensor.Minimum = Value;
        if (Value > Sensor->Sensor.Maximum)
            Sensor->Sensor.Maximum = Value;
    }
}

static VOID PhpHwSensorSetValue(
    _In_ PPHP_HW_SENSORS_CONTEXT Context,
    _In_ ULONG Package,
    _In_ PHP_HW_SENSOR_KIND Kind,
    _In_ ULONG Index,
    _In_ FLOAT Value
    )
{
    PhpHwSensorUpdateValue(PhpHwGetSensor(Context, Package, Kind, Index, NULL), Value);
}

static VOID PhpHwSensorSetThrottle(
    _In_ PPHP_HW_SENSORS_CONTEXT Context,
    _In_ ULONG Package,
    _In_ ULONG ThrottleFlags
    )
{
    PPHP_HW_NODE sensor = PhpHwGetSensor(Context, Package, PhpHwSensorThrottle, 0, NULL);

    sensor->Sensor.Current = TRUE;
    sensor->Sensor.ThrottleFlags = ThrottleFlags;
}

static VOID PhpHwCollectCpu(
    _In_ PPHP_HW_SENSORS_CONTEXT Context
    )
{
    ULONG packageCount = PhHwGetPackageCount();
    PPH_HW_PACKAGE_SENSOR package;
    PPH_HW_CPU_SENSOR sensor;
    ULONG processorCount = 0;

    while (PhHwGetCpuSensor(processorCount))
        processorCount++;

    for (ULONG i = 0; i < packageCount; i++)
    {
        if (!(package = PhHwGetPackageSensor(i)))
            break;

        if (package->Valid & PH_HW_CAP_PACKAGE_TEMP)
            PhpHwSensorSetValue(Context, i, PhpHwSensorPackageTemperature, 0, package->Temperature);
        if (package->Valid & PH_HW_CAP_CORES_TEMP)
            PhpHwSensorSetValue(Context, i, PhpHwSensorCoresTemperature, 0, package->CoresTemperature);
        if (package->Valid & PH_HW_CAP_GRAPHICS_TEMP)
            PhpHwSensorSetValue(Context, i, PhpHwSensorGraphicsTemperature, 0, package->GraphicsTemperature);

        if (package->Valid & PH_HW_CAP_CCD_TEMP)
        {
            for (ULONG ccd = 0; ccd < PH_HW_MAX_CCD; ccd++)
            {
                if (package->CcdValidMask & (1UL << ccd))
                    PhpHwSensorSetValue(Context, i, PhpHwSensorCcdTemperature, ccd, package->CcdTemperature[ccd]);
            }
        }

        if (package->Valid & PH_HW_CAP_RAPL_PACKAGE)
            PhpHwSensorSetValue(Context, i, PhpHwSensorPackagePower, 0, package->PackagePower);
        if (package->Valid & PH_HW_CAP_RAPL_CORE)
            PhpHwSensorSetValue(Context, i, PhpHwSensorCorePower, 0, package->CorePower);
        if (package->Valid & PH_HW_CAP_RAPL_DRAM)
            PhpHwSensorSetValue(Context, i, PhpHwSensorDramPower, 0, package->DramPower);
        if (package->Valid & PH_HW_CAP_RAPL_PSYS)
            PhpHwSensorSetValue(Context, i, PhpHwSensorPlatformPower, 0, package->PlatformPower);
        if (package->Valid & PH_HW_CAP_POWER_LIMIT1)
            PhpHwSensorSetValue(Context, i, PhpHwSensorPowerLimit1, 0, package->ActivePowerLimit1);
        if (package->Valid & PH_HW_CAP_POWER_LIMIT2)
            PhpHwSensorSetValue(Context, i, PhpHwSensorPowerLimit2, 0, package->ActivePowerLimit2);
        if (package->Valid & PH_HW_CAP_THROTTLE)
            PhpHwSensorSetThrottle(Context, i, package->ThrottleFlags);
    }

    for (ULONG i = 0; i < processorCount; i++)
    {
        ULONG packageIndex;

        if (!(sensor = PhHwGetCpuSensor(i)))
            break;

        packageIndex = sensor->PackageIndex;

        // Logical processors of a core share its temperature.
        if (sensor->Valid & PH_HW_CAP_CORE_TEMP)
            PhpHwSensorSetValue(Context, packageIndex, PhpHwSensorCoreTemperature, sensor->CoreIndex, sensor->Temperature);
        if (sensor->Valid & PH_HW_CAP_EFFECTIVE_CLOCK)
            PhpHwSensorSetValue(Context, packageIndex, PhpHwSensorEffectiveClock, i, sensor->EffectiveMhz);
        if (sensor->Valid & PH_HW_CAP_ACTIVE_CLOCK)
            PhpHwSensorSetValue(Context, packageIndex, PhpHwSensorActiveClock, i, sensor->ActiveMhz);
        if (sensor->Valid & PH_HW_CAP_MULTIPLIER)
            PhpHwSensorSetValue(Context, packageIndex, PhpHwSensorMultiplier, i, sensor->Multiplier);
        if (sensor->Valid & PH_HW_CAP_VOLTAGE)
            PhpHwSensorSetValue(Context, packageIndex, PhpHwSensorVoltage, i, sensor->Voltage);
    }
}

static VOID PhpHwCollectMemory(
    _In_ PPHP_HW_SENSORS_CONTEXT Context
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
            PhpHwSensorSetValue(Context, i, PhpHwSensorMemoryClock, 0, package->MemoryClockMhz);
            PhpHwSensorSetValue(Context, i, PhpHwSensorMemoryDataRate, 0, package->MemoryDataRate);

            if (package->MemoryGear)
                PhpHwSensorSetValue(Context, i, PhpHwSensorMemoryGear, 0, (FLOAT)package->MemoryGear);
        }

        if (package->Valid & PH_HW_CAP_MEMORY_BANDWIDTH)
        {
            PhpHwSensorSetValue(Context, i, PhpHwSensorMemoryReadBandwidth, 0, package->MemoryReadBandwidth);
            PhpHwSensorSetValue(Context, i, PhpHwSensorMemoryWriteBandwidth, 0, package->MemoryWriteBandwidth);
        }
    }
}

static VOID PhpHwCollectChipset(
    _In_ PPHP_HW_SENSORS_CONTEXT Context
    )
{
    FLOAT celsius;

    if (PhHwGetChipsetTemperature(&celsius))
        PhpHwSensorSetValue(Context, 0, PhpHwSensorChipsetTemperature, 0, celsius);
}

static PPHP_HW_NODE PhpHwFindSensor(
    _In_ PPHP_HW_SENSORS_CONTEXT Context,
    _In_ ULONG ComponentIndex,
    _In_ PHP_HW_SENSOR_KIND Kind,
    _In_ ULONG Index
    )
{
    PCPHP_HW_SENSOR_INFO info = &PhpHwSensorInfo[Kind];
    PPHP_HW_NODE component;
    PPHP_HW_NODE group;
    ULONG position;

    if (!(component = PhpHwFindNode(Context->ComponentList, ((ULONG64)info->Component << 56) | ComponentIndex, &position)))
        return NULL;
    if (!(group = component->Component.Groups[info->Group]))
        return NULL;

    return PhpHwFindNode(group->Children, ((ULONG64)Kind << 32) | Index, &position);
}

static BOOLEAN PhpHwSensorIsNamed(
    _In_ PHP_HW_SENSOR_KIND Kind
    )
{
    return PhpHwSensorInfo[Kind].Name == NULL;
}

// A named sensor node appears on its first current value.
static VOID PhpHwCollectNamed(
    _In_ PPHP_HW_SENSORS_CONTEXT Context
    )
{
    static CONST PHP_HW_SENSOR_KIND kinds[] =
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
    ULONG count = PhHwGetNamedSensorCount();

    C_ASSERT(RTL_NUMBER_OF(kinds) == PhHwNamedSensorMaximum);

    for (ULONG i = 0; i < count; i++)
    {
        PH_HW_NAMED_SENSOR sensor;
        PHP_HW_SENSOR_KIND kind;
        PPHP_HW_NODE node;

        if (!PhHwGetNamedSensor(i, &sensor) || sensor.Kind >= PhHwNamedSensorMaximum)
            continue;

        kind = kinds[sensor.Kind];
        sensor.Name[RTL_NUMBER_OF(sensor.Name) - 1] = UNICODE_NULL;

        if (sensor.Valid)
        {
            node = PhpHwGetSensor(Context, sensor.PackageIndex, kind, sensor.Id, sensor.Name);
            PhpHwSensorUpdateValue(node, sensor.Value);
        }
        else
        {
            node = PhpHwFindSensor(Context, sensor.PackageIndex, kind, sensor.Id);
        }

        if (node)
            node->Sensor.Listed = TRUE;
    }
}

_Function_class_(PH_HW_SOURCE_SENSOR_CALLBACK)
static VOID NTAPI PhpHwCollectSourceSensor(
    _In_ PCPH_HW_SOURCE_SENSOR Sensor,
    _In_opt_ PVOID Context
    )
{
    PPHP_HW_SENSORS_CONTEXT context = Context;
    PPHP_HW_NODE component;
    PPHP_HW_NODE group;
    PPHP_HW_NODE sensor;
    ULONG64 key;
    ULONG position;

    if (!context || Sensor->Type >= RTL_NUMBER_OF(PhpHwSourceTypeInfo))
        return;

    component = PhpHwGetComponent(context, PhpHwComponentSource, Sensor->SourceId, Sensor->ComponentId, Sensor->ComponentName);
    group = PhpHwGetGroup(context, component, PhpHwSourceTypeInfo[Sensor->Type].Group);
    key = ((ULONG64)PhpHwSensorMaximum << 32) | Sensor->SensorId;
    sensor = PhpHwFindNode(group->Children, key, &position);

    if (!sensor)
    {
        sensor = PhpHwInsertSensor(
            context,
            group,
            position,
            key,
            PhReferenceObject(Sensor->SensorName),
            PhpHwSensorMaximum,
            PhpHwSourceTypeInfo[Sensor->Type].Unit,
            0,
            !!(Sensor->Flags & PH_HW_SENSOR_REPORT_HISTORY)
            );
    }

    sensor->Sensor.Listed = TRUE;

    if (Sensor->Current)
        PhpHwSensorUpdateValue(sensor, Sensor->Value);
}

static VOID PhpHwSensorUpdateText(
    _In_ PPHP_HW_NODE Sensor
    )
{
    if (Sensor->Sensor.Unit == PhpHwUnitThrottle)
    {
        if (Sensor->Sensor.Current)
            PhMoveReference(&Sensor->Sensor.ValueText, PhpHwSensorFormatThrottle(Sensor->Sensor.ThrottleFlags));
        else
            PhClearReference(&Sensor->Sensor.ValueText);

        return;
    }

    if (Sensor->Sensor.Current)
        PhMoveReference(&Sensor->Sensor.ValueText, PhpHwSensorFormatValue(Sensor->Sensor.Unit, Sensor->Sensor.Value));
    else
        PhClearReference(&Sensor->Sensor.ValueText);

    // The gear is a mode, not a measurement; it has no range.
    if (Sensor->Sensor.RangeValid && Sensor->Sensor.Unit != PhpHwUnitGear)
    {
        PhMoveReference(&Sensor->Sensor.MinimumText, PhpHwSensorFormatValue(Sensor->Sensor.Unit, Sensor->Sensor.Minimum));
        PhMoveReference(&Sensor->Sensor.MaximumText, PhpHwSensorFormatValue(Sensor->Sensor.Unit, Sensor->Sensor.Maximum));
    }
    else
    {
        PhClearReference(&Sensor->Sensor.MinimumText);
        PhClearReference(&Sensor->Sensor.MaximumText);
    }
}

// A source that unregisters leaves empty groups and components behind.
static VOID PhpHwSensorsRemoveEmptySourceComponents(
    _In_ PPHP_HW_SENSORS_CONTEXT Context
    )
{
    for (ULONG i = 0; i < Context->ComponentList->Count; )
    {
        PPHP_HW_NODE component = Context->ComponentList->Items[i];
        BOOLEAN empty = TRUE;

        if (component->Component.Kind != PhpHwComponentSource)
        {
            i++;
            continue;
        }

        for (ULONG group = 0; group < PhpHwGroupMaximum; group++)
        {
            if (!component->Component.Groups[group])
                continue;

            if (component->Component.Groups[group]->Children->Count)
            {
                empty = FALSE;
                continue;
            }

            PhpHwDestroyNode(component->Component.Groups[group]);
            component->Component.Groups[group] = NULL;
        }

        if (empty)
        {
            PhRemoveItemList(Context->ComponentList, i);
            PhpHwDestroyNode(component);
            continue;
        }

        i++;
    }
}

static BOOLEAN PhpHwSensorsMatch(
    _In_ PPHP_HW_SENSORS_CONTEXT Context,
    _In_ PPHP_HW_NODE Node
    )
{
    return Node->Name && PhSearchControlMatch(Context->SearchMatchHandle, &Node->Name->sr);
}

// A matching component or group shows all its sensors; a matching sensor shows its ancestors.
static VOID PhpHwSensorsApplySearch(
    _In_ PPHP_HW_SENSORS_CONTEXT Context
    )
{
    for (ULONG i = 0; i < Context->RootList->Count; i++)
    {
        PPHP_HW_NODE component = Context->RootList->Items[i];
        BOOLEAN componentMatch = !Context->SearchMatchHandle || PhpHwSensorsMatch(Context, component);

        component->Node.Visible = componentMatch;

        for (ULONG j = 0; j < component->Children->Count; j++)
        {
            PPHP_HW_NODE group = component->Children->Items[j];
            BOOLEAN groupMatch = componentMatch || PhpHwSensorsMatch(Context, group);

            group->Node.Visible = groupMatch;

            for (ULONG k = 0; k < group->Children->Count; k++)
            {
                PPHP_HW_NODE sensor = group->Children->Items[k];

                sensor->Node.Visible = groupMatch || PhpHwSensorsMatch(Context, sensor);

                if (sensor->Node.Visible)
                {
                    group->Node.Visible = TRUE;
                    component->Node.Visible = TRUE;
                }
            }
        }
    }
}

static VOID PhpHwSensorsUpdateEmptyText(
    _In_ PPHP_HW_SENSORS_CONTEXT Context
    )
{
    static CONST PH_STRINGREF noMatch = PH_STRINGREF_INIT(L"No sensors match the search.");
    static CONST PH_STRINGREF unavailable = PH_STRINGREF_INIT(L"Hardware sensors are unavailable.");
    static CONST PH_STRINGREF unsupported = PH_STRINGREF_INIT(L"No supported hardware sensors were found. Use Save... to create a hardware report and attach it to a new issue to request support.");
    static CONST PH_STRINGREF waiting = PH_STRINGREF_INIT(L"Waiting for sensor readings...");
    PCPH_STRINGREF text;

    if (Context->RootList->Count)
        text = &noMatch;
    else if (!PhHwSensorsEnabled())
        text = &unavailable;
    else if (PhHwSensorsProbed() && !PhHwSensorsAvailable() && !PhHwSourceSensorsPresent())
        text = &unsupported;
    else
        text = &waiting;

    if (text == Context->EmptyText)
        return;

    Context->EmptyText = text;
    TreeNew_SetEmptyText(Context->TreeNewHandle, text, 0);
}

static VOID PhpHwSensorsRestructure(
    _In_ PPHP_HW_SENSORS_CONTEXT Context
    )
{
    PhpHwSensorsRemoveEmptySourceComponents(Context);
    PhClearList(Context->RootList);

    for (ULONG i = 0; i < Context->ComponentList->Count; i++)
    {
        PPHP_HW_NODE component = Context->ComponentList->Items[i];

        PhClearList(component->Children);

        for (ULONG group = 0; group < PhpHwGroupMaximum; group++)
        {
            if (component->Component.Groups[group] && component->Component.Groups[group]->Children->Count)
                PhAddItemList(component->Children, component->Component.Groups[group]);
        }

        if (component->Children->Count)
            PhAddItemList(Context->RootList, component);
    }

    PhpHwSensorsApplySearch(Context);
    TreeNew_NodesStructured(Context->TreeNewHandle);
}

// Sensors appear on their first current value and show an empty value while stale.
static VOID PhpHwSensorsUpdate(
    _In_ PPHP_HW_SENSORS_CONTEXT Context
    )
{
    ULONG capabilities = PhHwGetCapabilities();

    for (ULONG i = 0; i < Context->SensorList->Count; i++)
    {
        PPHP_HW_NODE sensor = Context->SensorList->Items[i];

        sensor->Sensor.Current = FALSE;
        sensor->Sensor.Listed = FALSE;
    }

    if (capabilities)
    {
        PhpHwCollectCpu(Context);
        PhpHwCollectMemory(Context);
        PhpHwCollectChipset(Context);
    }

    PhpHwCollectNamed(Context);

    PhHwEnumerateSourceSensors(PhpHwCollectSourceSensor, Context);

    for (ULONG i = 0; i < Context->SensorList->Count; )
    {
        PPHP_HW_NODE sensor = Context->SensorList->Items[i];
        BOOLEAN available;

        if (sensor->Sensor.Kind == PhpHwSensorMaximum || PhpHwSensorIsNamed(sensor->Sensor.Kind))
            available = sensor->Sensor.Listed;
        else
            available = !!(capabilities & sensor->Sensor.Capability);

        if (!available)
        {
            PPH_LIST siblings = sensor->Sensor.Group->Children;
            ULONG position = PhFindItemList(siblings, sensor);

            if (position != ULONG_MAX)
                PhRemoveItemList(siblings, position);

            PhRemoveItemList(Context->SensorList, i);
            PhpHwDestroyNode(sensor);
            Context->StructureChanged = TRUE;
            continue;
        }

        if (sensor->Sensor.History)
        {
            PhAddItemCircularBuffer_FLOAT(&sensor->Sensor.HistoryBuffer, sensor->Sensor.Current ? sensor->Sensor.Value : 0.0f);
            sensor->Sensor.GraphBuffers.Valid = FALSE;
        }

        PhpHwSensorUpdateText(sensor);
        i++;
    }

    if (Context->StructureChanged)
    {
        Context->StructureChanged = FALSE;
        PhpHwSensorsRestructure(Context);
    }
    else
    {
        InvalidateRect(Context->TreeNewHandle, NULL, FALSE);
    }

    PhpHwSensorsUpdateEmptyText(Context);
}

static VOID PhpHwSensorsResetRange(
    _In_ PPHP_HW_SENSORS_CONTEXT Context
    )
{
    for (ULONG i = 0; i < Context->SensorList->Count; i++)
    {
        PPHP_HW_NODE sensor = Context->SensorList->Items[i];

        sensor->Sensor.RangeValid = sensor->Sensor.Current;
        sensor->Sensor.Minimum = sensor->Sensor.Value;
        sensor->Sensor.Maximum = sensor->Sensor.Value;

        PhpHwSensorUpdateText(sensor);
    }

    InvalidateRect(Context->TreeNewHandle, NULL, FALSE);
}

static VOID PhpHwSensorsInvalidateGraphs(
    _In_ PPHP_HW_SENSORS_CONTEXT Context
    )
{
    for (ULONG i = 0; i < Context->SensorList->Count; i++)
    {
        PPHP_HW_NODE sensor = Context->SensorList->Items[i];

        sensor->Sensor.GraphBuffers.Valid = FALSE;
    }
}

static VOID PhpHwSensorsDeleteGraphContext(
    _In_ PPHP_HW_SENSORS_CONTEXT Context
    )
{
    if (Context->GraphContext)
    {
        if (Context->GraphOldBitmap)
        {
            SelectBitmap(Context->GraphContext, Context->GraphOldBitmap);
            Context->GraphOldBitmap = NULL;
        }

        if (Context->GraphBitmap)
        {
            DeleteBitmap(Context->GraphBitmap);
            Context->GraphBitmap = NULL;
        }

        DeleteDC(Context->GraphContext);
        Context->GraphContext = NULL;
    }

    Context->GraphBits = NULL;
    Context->GraphWidth = 0;
    Context->GraphHeight = 0;
}

static VOID PhpHwSensorsNeedGraphContext(
    _In_ PPHP_HW_SENSORS_CONTEXT Context,
    _In_ HDC hdc,
    _In_ LONG Width,
    _In_ LONG Height
    )
{
    if (Context->GraphContext && Context->GraphWidth == Width && Context->GraphHeight == Height)
        return;

    PhpHwSensorsDeleteGraphContext(Context);

    Context->GraphContext = CreateCompatibleDC(hdc);

    if (!Context->GraphContext)
        return;

    Context->GraphBitmap = PhCreateDIBSection(hdc, PHBF_DIB, Width, Height, &Context->GraphBits);

    if (!Context->GraphBitmap)
    {
        DeleteDC(Context->GraphContext);
        Context->GraphContext = NULL;
        Context->GraphBits = NULL;
        return;
    }

    Context->GraphOldBitmap = SelectBitmap(Context->GraphContext, Context->GraphBitmap);
    Context->GraphWidth = Width;
    Context->GraphHeight = Height;
}

static VOID PhpHwSensorsDrawHistory(
    _In_ PPHP_HW_SENSORS_CONTEXT Context,
    _In_ PPH_TREENEW_CUSTOM_DRAW CustomDraw
    )
{
    PPHP_HW_NODE node = (PPHP_HW_NODE)CustomDraw->Node;
    RECT rect = CustomDraw->CellRect;
    PH_GRAPH_DRAW_INFO drawInfo;
    COLORREF color;

    if (node->Type != PhpHwNodeSensor)
        return;
    if (!node->Sensor.History || !node->Sensor.HistoryBuffer.Count)
        return;
    if (rect.right - rect.left <= 1 || rect.bottom - rect.top <= 1)
        return;

    color = node->Sensor.Unit == PhpHwUnitTemperature ? PhCsColorTemperature : PhCsColorPowerUsage;

    memset(&drawInfo, 0, sizeof(PH_GRAPH_DRAW_INFO));
    drawInfo.Width = rect.right - rect.left - 1; // leave a small gap
    drawInfo.Height = rect.bottom - rect.top - 1; // leave a small gap
    drawInfo.Step = 2;
    drawInfo.BackColor = RGB(0x00, 0x00, 0x00);
    drawInfo.LineColor1 = color;
    drawInfo.LineBackColor1 = PhHalveColorBrightness(color);

    PhGetDrawInfoGraphBuffers(&node->Sensor.GraphBuffers, &drawInfo, node->Sensor.HistoryBuffer.Count);

    if (!node->Sensor.GraphBuffers.Valid)
    {
        FLOAT scale;

        PhCopyCircularBuffer_FLOAT(&node->Sensor.HistoryBuffer, node->Sensor.GraphBuffers.Data1, drawInfo.LineDataCount);

        if (
            node->Sensor.Unit == PhpHwUnitTemperature &&
            node->Sensor.Kind != PhpHwSensorMaximum &&
            node->Sensor.Kind != PhpHwSensorChipsetTemperature &&
            node->Sensor.Kind != PhpHwSensorDimmTemperature &&
            node->Sensor.Kind != PhpHwSensorDimmPmicTemperature &&
            node->Sensor.Kind != PhpHwSensorAcpiTemperature &&
            node->Sensor.Kind != PhpHwSensorZoneTemperature
            )
        {
            scale = PhHwGetCpuTemperatureLimit();
        }
        else if (node->Sensor.Unit == PhpHwUnitTemperature)
        {
            // A source, chipset, zone or DIMM temperature is not bounded by the CPU TjMax.
            scale = max(PhMaxMemorySingles(node->Sensor.GraphBuffers.Data1, drawInfo.LineDataCount), 100.0f);
        }
        else if (node->Sensor.Unit == PhpHwUnitPercent)
        {
            scale = 100.0f;
        }
        else
        {
            scale = PhMaxMemorySingles(node->Sensor.GraphBuffers.Data1, drawInfo.LineDataCount);

            if (scale < PHP_HW_POWER_GRAPH_MINIMUM)
                scale = PHP_HW_POWER_GRAPH_MINIMUM;
        }

        PhDivideSinglesBySingle(node->Sensor.GraphBuffers.Data1, scale, drawInfo.LineDataCount);
        node->Sensor.GraphBuffers.Valid = TRUE;
    }

    PhpHwSensorsNeedGraphContext(Context, CustomDraw->Dc, drawInfo.Width, drawInfo.Height);

    if (Context->GraphBits)
    {
        PhDrawGraphDirect(Context->GraphContext, Context->GraphBits, &drawInfo);
        BitBlt(
            CustomDraw->Dc,
            rect.left,
            rect.top + 1, // + 1 for small gap
            drawInfo.Width,
            drawInfo.Height,
            Context->GraphContext,
            0,
            0,
            SRCCOPY
            );
    }
}

static VOID PhpHwSensorsCopy(
    _In_ PPHP_HW_SENSORS_CONTEXT Context
    )
{
    PPH_STRING text;

    text = PhGetTreeNewText(Context->TreeNewHandle, 0);
    PhSetClipboardString(Context->TreeNewHandle, &text->sr);
    PhDereferenceObject(text);
}

// Every sensor, regardless of the search and collapsed nodes.
static PPH_STRING PhpHwSensorsGetText(
    _In_ PPHP_HW_SENSORS_CONTEXT Context
    )
{
    PH_STRING_BUILDER builder;

    PhInitializeStringBuilder(&builder, 0x1000);

    for (ULONG i = 0; i < Context->RootList->Count; i++)
    {
        PPHP_HW_NODE component = Context->RootList->Items[i];

        PhAppendFormatStringBuilder(&builder, L"%s\n", PhGetStringOrEmpty(component->Name));

        for (ULONG j = 0; j < component->Children->Count; j++)
        {
            PPHP_HW_NODE group = component->Children->Items[j];

            PhAppendFormatStringBuilder(&builder, L"  %s\n", PhGetStringOrEmpty(group->Name));

            for (ULONG k = 0; k < group->Children->Count; k++)
            {
                PPHP_HW_NODE sensor = group->Children->Items[k];

                PhAppendFormatStringBuilder(
                    &builder,
                    L"    %s: %s (min %s, max %s)\n",
                    PhGetStringOrEmpty(sensor->Name),
                    PhGetStringOrEmpty(sensor->Sensor.ValueText),
                    PhGetStringOrEmpty(sensor->Sensor.MinimumText),
                    PhGetStringOrEmpty(sensor->Sensor.MaximumText)
                    );
            }
        }
    }

    return PhFinalStringBuilderString(&builder);
}

static VOID PhpHwSensorsSave(
    _In_ PPHP_HW_SENSORS_CONTEXT Context
    )
{
    static PH_FILETYPE_FILTER filters[] =
    {
        { L"Text files (*.txt)", L"*.txt" },
        { L"All files (*.*)", L"*.*" }
    };
    PVOID fileDialog;

    fileDialog = PhCreateSaveFileDialog();
    PhSetFileDialogFilter(fileDialog, filters, RTL_NUMBER_OF(filters));
    PhSetFileDialogFileName(fileDialog, L"Hardware.txt");

    if (PhShowFileDialog(Context->WindowHandle, fileDialog))
    {
        NTSTATUS status;
        PPH_STRING fileName;
        PPH_FILE_STREAM fileStream;
        PPH_STRING sensors;
        PPH_STRING report;

        fileName = PH_AUTO(PhGetFileDialogFileName(fileDialog));
        sensors = PhpHwSensorsGetText(Context);
        report = PhHwCreateReport(&sensors->sr);

        if (NT_SUCCESS(status = PhCreateFileStream(
            &fileStream,
            fileName->Buffer,
            FILE_GENERIC_WRITE,
            FILE_SHARE_READ,
            FILE_OVERWRITE_IF,
            0
            )))
        {
            PhWriteStringAsUtf8FileStream(fileStream, (PPH_STRINGREF)&PhUnicodeByteOrderMark);
            PhWriteStringAsUtf8FileStream(fileStream, &report->sr);
            PhDereferenceObject(fileStream);
        }

        if (!NT_SUCCESS(status))
            PhShowStatus(Context->WindowHandle, L"Unable to create the file", status, 0);

        PhDereferenceObject(report);
        PhDereferenceObject(sensors);
    }

    PhFreeFileDialog(fileDialog);
}

_Function_class_(PH_SEARCHCONTROL_CALLBACK)
static VOID NTAPI PhpHwSensorsSearchControlCallback(
    _In_ ULONG_PTR MatchHandle,
    _In_opt_ PVOID Context
    )
{
    PPHP_HW_SENSORS_CONTEXT context = Context;

    if (!context)
        return;

    context->SearchMatchHandle = MatchHandle;
    PhpHwSensorsApplySearch(context);
    TreeNew_NodesStructured(context->TreeNewHandle);
}

static VOID PhpHwSensorsShowContextMenu(
    _In_ PPHP_HW_SENSORS_CONTEXT Context,
    _In_ PPH_TREENEW_CONTEXT_MENU ContextMenu
    )
{
    PPH_EMENU menu;
    PPH_EMENU_ITEM item;

    menu = PhCreateEMenu();
    PhInsertEMenuItem(menu, PhCreateEMenuItem(0, IDC_RESET, L"&Reset min/max", NULL, NULL), ULONG_MAX);
    PhInsertEMenuItem(menu, PhCreateEMenuSeparator(), ULONG_MAX);
    PhInsertEMenuItem(menu, PhCreateEMenuItem(0, IDC_COPY, L"&Copy", NULL, NULL), ULONG_MAX);
    PhInsertCopyCellEMenuItem(menu, IDC_COPY, Context->TreeNewHandle, ContextMenu->Column);

    item = PhShowEMenu(
        menu,
        Context->WindowHandle,
        PH_EMENU_SHOW_LEFTRIGHT,
        PH_ALIGN_LEFT | PH_ALIGN_TOP,
        ContextMenu->Location.x,
        ContextMenu->Location.y
        );

    if (item && !PhHandleCopyCellEMenuItem(item))
    {
        switch (item->Id)
        {
        case IDC_RESET:
            PhpHwSensorsResetRange(Context);
            break;
        case IDC_COPY:
            PhpHwSensorsCopy(Context);
            break;
        }
    }

    PhDestroyEMenu(menu);
}

_Function_class_(PH_TREENEW_CALLBACK)
static BOOLEAN NTAPI PhpHwSensorsTreeNewCallback(
    _In_ HWND WindowHandle,
    _In_ PH_TREENEW_MESSAGE Message,
    _In_ PVOID Parameter1,
    _In_ PVOID Parameter2,
    _In_ PVOID Context
    )
{
    PPHP_HW_SENSORS_CONTEXT context = Context;
    PPHP_HW_NODE node;

    switch (Message)
    {
    case TreeNewGetChildren:
        {
            PPH_TREENEW_GET_CHILDREN getChildren = Parameter1;
            node = (PPHP_HW_NODE)getChildren->Node;

            if (!node)
            {
                getChildren->Children = (PPH_TREENEW_NODE *)context->RootList->Items;
                getChildren->NumberOfChildren = context->RootList->Count;
            }
            else if (node->Children)
            {
                getChildren->Children = (PPH_TREENEW_NODE *)node->Children->Items;
                getChildren->NumberOfChildren = node->Children->Count;
            }
        }
        return TRUE;
    case TreeNewIsLeaf:
        {
            PPH_TREENEW_IS_LEAF isLeaf = Parameter1;
            node = (PPHP_HW_NODE)isLeaf->Node;

            isLeaf->IsLeaf = node->Type == PhpHwNodeSensor;
        }
        return TRUE;
    case TreeNewGetCellText:
        {
            PPH_TREENEW_GET_CELL_TEXT getCellText = Parameter1;
            node = (PPHP_HW_NODE)getCellText->Node;

            switch (getCellText->Id)
            {
            case PhpHwSensorsColumnName:
                getCellText->Text = PhGetStringRef(node->Name);
                break;
            case PhpHwSensorsColumnValue:
                if (node->Type == PhpHwNodeSensor)
                    getCellText->Text = PhGetStringRef(node->Sensor.ValueText);
                break;
            case PhpHwSensorsColumnMinimum:
                if (node->Type == PhpHwNodeSensor)
                    getCellText->Text = PhGetStringRef(node->Sensor.MinimumText);
                break;
            case PhpHwSensorsColumnMaximum:
                if (node->Type == PhpHwNodeSensor)
                    getCellText->Text = PhGetStringRef(node->Sensor.MaximumText);
                break;
            default:
                return FALSE;
            }
        }
        return TRUE;
    case TreeNewGetNodeColor:
        {
            PPH_TREENEW_GET_NODE_COLOR getNodeColor = Parameter1;

            getNodeColor->Flags = TN_CACHE | TN_AUTO_FORECOLOR;
        }
        return TRUE;
    case TreeNewCustomDraw:
        {
            PPH_TREENEW_CUSTOM_DRAW customDraw = Parameter1;

            if (customDraw->Column->Id == PhpHwSensorsColumnHistory)
                PhpHwSensorsDrawHistory(context, customDraw);
        }
        return TRUE;
    case TreeNewColumnResized:
        {
            PPH_TREENEW_COLUMN column = Parameter1;

            if (column->Id == PhpHwSensorsColumnHistory)
                PhpHwSensorsInvalidateGraphs(context);
        }
        return TRUE;
    case TreeNewKeyDown:
        {
            PPH_TREENEW_KEY_EVENT keyEvent = Parameter1;

            switch (keyEvent->VirtualKey)
            {
            case 'C':
                if (GetKeyState(VK_CONTROL) < 0)
                    PhpHwSensorsCopy(context);
                break;
            case 'A':
                if (GetKeyState(VK_CONTROL) < 0)
                    TreeNew_SelectRange(WindowHandle, 0, -1);
                break;
            }
        }
        return TRUE;
    case TreeNewContextMenu:
        {
            PhpHwSensorsShowContextMenu(context, Parameter1);
        }
        return TRUE;
    case TreeNewHeaderRightClick:
        {
            PH_TN_COLUMN_MENU_DATA data;

            data.TreeNewHandle = WindowHandle;
            data.MouseEvent = Parameter1;
            data.DefaultSortColumn = 0;
            data.DefaultSortOrder = NoSortOrder;
            PhInitializeTreeNewColumnMenu(&data);

            data.Selection = PhShowEMenu(data.Menu, WindowHandle, PH_EMENU_SHOW_LEFTRIGHT,
                PH_ALIGN_LEFT | PH_ALIGN_TOP, data.MouseEvent->ScreenLocation.x, data.MouseEvent->ScreenLocation.y);
            PhHandleTreeNewColumnMenu(&data);
            PhDeleteTreeNewColumnMenu(&data);
        }
        return TRUE;
    }

    return FALSE;
}

static VOID PhpHwSensorsInitializeTree(
    _In_ PPHP_HW_SENSORS_CONTEXT Context
    )
{
    PPH_STRING settings;

    PhSetControlTheme(Context->TreeNewHandle, L"explorer");

    TreeNew_SetCallback(Context->TreeNewHandle, PhpHwSensorsTreeNewCallback, Context);
    TreeNew_SetExtendedFlags(Context->TreeNewHandle, TN_FLAG_ITEM_DRAG_SELECT, TN_FLAG_ITEM_DRAG_SELECT);

    TreeNew_SetRedraw(Context->TreeNewHandle, FALSE);

    PhAddTreeNewColumn(Context->TreeNewHandle, PhpHwSensorsColumnName, TRUE, L"Sensor", 280, PH_ALIGN_LEFT, 0, 0);
    PhAddTreeNewColumn(Context->TreeNewHandle, PhpHwSensorsColumnValue, TRUE, L"Value", 90, PH_ALIGN_RIGHT, 1, DT_RIGHT);
    PhAddTreeNewColumn(Context->TreeNewHandle, PhpHwSensorsColumnMinimum, TRUE, L"Min", 90, PH_ALIGN_RIGHT, 2, DT_RIGHT);
    PhAddTreeNewColumn(Context->TreeNewHandle, PhpHwSensorsColumnMaximum, TRUE, L"Max", 90, PH_ALIGN_RIGHT, 3, DT_RIGHT);
    PhAddTreeNewColumnEx2(Context->TreeNewHandle, PhpHwSensorsColumnHistory, TRUE, L"History", 140, PH_ALIGN_LEFT, 4, 0, TN_COLUMN_FLAG_CUSTOMDRAW);

    TreeNew_SetRedraw(Context->TreeNewHandle, TRUE);

    settings = PhGetStringSetting(SETTING_HW_SENSORS_TREE_LIST_COLUMNS);
    PhCmLoadSettings(Context->TreeNewHandle, &settings->sr);
    PhDereferenceObject(settings);
}

static VOID PhpHwSensorsDeleteTree(
    _In_ PPHP_HW_SENSORS_CONTEXT Context
    )
{
    PPH_STRING settings;

    settings = PhCmSaveSettings(Context->TreeNewHandle);
    PhSetStringSetting2(SETTING_HW_SENSORS_TREE_LIST_COLUMNS, &settings->sr);
    PhDereferenceObject(settings);

    for (ULONG i = 0; i < Context->SensorList->Count; i++)
        PhpHwDestroyNode(Context->SensorList->Items[i]);

    for (ULONG i = 0; i < Context->ComponentList->Count; i++)
    {
        PPHP_HW_NODE component = Context->ComponentList->Items[i];

        for (ULONG group = 0; group < PhpHwGroupMaximum; group++)
        {
            if (component->Component.Groups[group])
                PhpHwDestroyNode(component->Component.Groups[group]);
        }

        PhpHwDestroyNode(component);
    }

    PhDereferenceObject(Context->SensorList);
    PhDereferenceObject(Context->RootList);
    PhDereferenceObject(Context->ComponentList);

    PhpHwSensorsDeleteGraphContext(Context);
}

INT_PTR CALLBACK PhpHwSensorsDlgProc(
    _In_ HWND hwndDlg,
    _In_ UINT uMsg,
    _In_ WPARAM wParam,
    _In_ LPARAM lParam
    )
{
    PPHP_HW_SENSORS_CONTEXT context;

    if (uMsg == WM_INITDIALOG)
    {
        context = PhAllocateZero(sizeof(PHP_HW_SENSORS_CONTEXT));
        context->WindowHandle = hwndDlg;

        PhSetWindowContext(hwndDlg, PH_WINDOW_CONTEXT_DEFAULT, context);
    }
    else
    {
        context = PhGetWindowContext(hwndDlg, PH_WINDOW_CONTEXT_DEFAULT);
    }

    if (!context)
        return FALSE;

    switch (uMsg)
    {
    case WM_INITDIALOG:
        {
            context->TreeNewHandle = GetDlgItem(hwndDlg, IDC_TREELIST);
            context->SearchWindowHandle = GetDlgItem(hwndDlg, IDC_FILTER);
            context->ComponentList = PhCreateList(2);
            context->RootList = PhCreateList(2);
            context->SensorList = PhCreateList(64);
            context->CpuBrandString = PhSipGetCpuBrandString();
            context->ProcessorCount = PhSystemProcessorInformation.NumberOfProcessors;

            PhSetApplicationWindowIcon(hwndDlg);

            PhCreateSearchControl2(
                hwndDlg,
                context->SearchWindowHandle,
                L"Search Hardware Sensors",
                SETTING_SEARCH_HW_SENSORS_REGEX,
                SETTING_SEARCH_HW_SENSORS_CASE_SENSITIVE,
                PhpHwSensorsSearchControlCallback,
                context
                );

            PhpHwSensorsInitializeTree(context);

            PhInitializeLayoutManager(&context->LayoutManager, hwndDlg);
            PhAddLayoutItem(&context->LayoutManager, context->SearchWindowHandle, NULL, PH_ANCHOR_LEFT | PH_ANCHOR_TOP | PH_ANCHOR_RIGHT);
            PhAddLayoutItem(&context->LayoutManager, context->TreeNewHandle, NULL, PH_ANCHOR_ALL);
            PhAddLayoutItem(&context->LayoutManager, GetDlgItem(hwndDlg, IDC_RESET), NULL, PH_ANCHOR_LEFT | PH_ANCHOR_BOTTOM);
            PhAddLayoutItem(&context->LayoutManager, GetDlgItem(hwndDlg, IDC_SAVE), NULL, PH_ANCHOR_LEFT | PH_ANCHOR_BOTTOM);
            PhAddLayoutItem(&context->LayoutManager, GetDlgItem(hwndDlg, IDOK), NULL, PH_ANCHOR_RIGHT | PH_ANCHOR_BOTTOM);

            PhpHwSensorsUpdate(context);

            PhRegisterCallback(
                PhGetGeneralCallback(GeneralCallbackProcessProviderUpdatedEvent),
                PhpHwSensorsProcessesUpdatedHandler,
                context,
                &context->ProcessesUpdatedRegistration
                );

            if (PhValidWindowPlacementFromSetting(SETTING_HW_SENSORS_WINDOW_POSITION))
                PhLoadWindowPlacementFromSetting(SETTING_HW_SENSORS_WINDOW_POSITION, SETTING_HW_SENSORS_WINDOW_SIZE, hwndDlg);
            else
                PhCenterWindow(hwndDlg, GetParent(hwndDlg));

            PhSetDialogFocus(hwndDlg, context->TreeNewHandle);

            PhInitializeWindowTheme(hwndDlg, PhEnableThemeSupport);
        }
        break;
    case WM_DESTROY:
        {
            PhUnregisterCallback(
                PhGetGeneralCallback(GeneralCallbackProcessProviderUpdatedEvent),
                &context->ProcessesUpdatedRegistration
                );

            PhSaveWindowPlacementToSetting(SETTING_HW_SENSORS_WINDOW_POSITION, SETTING_HW_SENSORS_WINDOW_SIZE, hwndDlg);

            PhUnregisterDialog(PhpHwSensorsWindowHandle);
            PhpHwSensorsWindowHandle = NULL;

            PhRemoveWindowContext(hwndDlg, PH_WINDOW_CONTEXT_DEFAULT);

            PhpHwSensorsDeleteTree(context);
            PhClearReference(&context->CpuBrandString);
            PhDeleteLayoutManager(&context->LayoutManager);
            PhFree(context);
        }
        break;
    case WM_COMMAND:
        {
            switch (GET_WM_COMMAND_ID(wParam, lParam))
            {
            case IDCANCEL:
            case IDOK:
                DestroyWindow(hwndDlg);
                break;
            case IDC_RESET:
                PhpHwSensorsResetRange(context);
                break;
            case IDC_SAVE:
                PhpHwSensorsSave(context);
                break;
            }
        }
        break;
    case WM_PH_HW_SENSORS_UPDATE:
        {
            PhpHwSensorsUpdate(context);
        }
        break;
    case WM_DPICHANGED:
        {
            PhLayoutManagerUpdate(&context->LayoutManager, LOWORD(wParam));
            PhLayoutManagerLayout(&context->LayoutManager);
        }
        break;
    case WM_SIZE:
        {
            PhLayoutManagerLayout(&context->LayoutManager);
        }
        break;
    case WM_CTLCOLORBTN:
        return HANDLE_WM_CTLCOLORBTN(hwndDlg, wParam, lParam, PhWindowThemeControlColor);
    case WM_CTLCOLORDLG:
        return HANDLE_WM_CTLCOLORDLG(hwndDlg, wParam, lParam, PhWindowThemeControlColor);
    case WM_CTLCOLORSTATIC:
        return HANDLE_WM_CTLCOLORSTATIC(hwndDlg, wParam, lParam, PhWindowThemeControlColor);
    }

    return FALSE;
}
