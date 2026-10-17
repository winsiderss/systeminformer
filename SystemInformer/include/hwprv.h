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

#ifndef PH_HWPRV_H
#define PH_HWPRV_H

// begin_phapppub
#define PH_HW_CAP_CORE_TEMP 0x00000001
#define PH_HW_CAP_PACKAGE_TEMP 0x00000002
#define PH_HW_CAP_CCD_TEMP 0x00000004
#define PH_HW_CAP_EFFECTIVE_CLOCK 0x00000008
#define PH_HW_CAP_MULTIPLIER 0x00000010
#define PH_HW_CAP_VOLTAGE 0x00000020
#define PH_HW_CAP_THROTTLE 0x00000040
#define PH_HW_CAP_RAPL_PACKAGE 0x00000080
#define PH_HW_CAP_RAPL_CORE 0x00000100
#define PH_HW_CAP_RAPL_DRAM 0x00000200
#define PH_HW_CAP_RAPL_PSYS 0x00000400
#define PH_HW_CAP_ACTIVE_CLOCK 0x00000800
#define PH_HW_CAP_MEMORY_CLOCK 0x00001000
#define PH_HW_CAP_MEMORY_BANDWIDTH 0x00002000
#define PH_HW_CAP_CORES_TEMP 0x00004000 // Intel PP0 (IA cores) domain
#define PH_HW_CAP_GRAPHICS_TEMP 0x00008000 // Intel PP1 (graphics) domain
#define PH_HW_CAP_POWER_LIMIT1 0x00010000
#define PH_HW_CAP_POWER_LIMIT2 0x00020000
#define PH_HW_CAP_CHIPSET_TEMP 0x00040000
#define PH_HW_CAP_DIMM_TEMP 0x00080000
#define PH_HW_CAP_UNCORE_CLOCK 0x00100000
#define PH_HW_CAP_TPMI_POWER 0x00200000 // TPMI RAPL domains not mapped to a package, PMT temperatures
#define PH_HW_CAP_SMU 0x00400000 // AMD SMU PM table and HSMP sensors
#define PH_HW_CAP_FAN_SPEED 0x00800000 // ACPI fan (PNP0C0B) and EC fan speeds
#define PH_HW_CAP_MEMORY_TIMINGS 0x01000000 // AMD DDR5 UMC, Intel MCHBAR timings
#define PH_HW_CAP_DIMM_VOLTAGE 0x02000000 // DDR5 PMIC set points
#define PH_HW_CAP_DIMM_PMIC 0x04000000 // DDR5 PMIC telemetry
#define PH_HW_CAP_ACPI_TEMP 0x08000000 // Intel DPTF participant, EC and ATK0110 temperatures, ATK0110 voltages
#define PH_HW_CAP_FAN_CONTROL 0x10000000 // ACPI fine-grain fan control
#define PH_HW_CAP_ACPI_POWER 0x20000000 // ACPI000D power meters
#define PH_HW_CAP_RESIDENCY 0x40000000 // Intel LPIT S0ix and MMIO RAPL power-limited residency
#define PH_HW_CAP_PLATFORM_LIMITS 0x80000000 // Intel PL4 and platform (PSYS) PL1/PL2

#define PH_HW_THROTTLE_THERMAL 0x00000001
#define PH_HW_THROTTLE_PROCHOT 0x00000002
#define PH_HW_THROTTLE_CRITICAL 0x00000004
#define PH_HW_THROTTLE_POWER_LIMIT 0x00000008
#define PH_HW_THROTTLE_PL1 0x00000010 // Intel core perf limit reasons from here
#define PH_HW_THROTTLE_PL2 0x00000020
#define PH_HW_THROTTLE_VR_THERMAL 0x00000040
#define PH_HW_THROTTLE_VR_CURRENT 0x00000080 // VR TDC
#define PH_HW_THROTTLE_ELECTRICAL 0x00000100 // EDP, other
#define PH_HW_THROTTLE_MAX_TURBO 0x00000200 // multi-core turbo limit
#define PH_HW_THROTTLE_TURBO_ATTENUATION 0x00000400
#define PH_HW_THROTTLE_RUNNING_AVERAGE_THERMAL 0x00000800
#define PH_HW_THROTTLE_RESIDENCY 0x00001000 // residency-state regulation
#define PH_HW_THROTTLE_GRAPHICS 0x00002000 // Haswell/Broadwell: graphics driver
#define PH_HW_THROTTLE_AUTONOMOUS 0x00004000 // Haswell/Broadwell: utilization-based frequency control

#define PH_HW_MAX_CCD 16
#define PH_HW_TEMPERATURE_FORMAT_COUNT 2

typedef struct _PH_HW_CPU_SENSOR
{
    PROCESSOR_NUMBER ProcessorNumber;
    ULONG CoreIndex;
    ULONG PackageIndex;
    ULONG Valid; // PH_HW_CAP_* bits whose values are current
    FLOAT Temperature;
    FLOAT EffectiveMhz; // APERF over the sample interval, including idle time
    FLOAT Multiplier;
    FLOAT Voltage;
    ULONG ThrottleFlags;
    FLOAT ActiveMhz; // P0 frequency scaled by APERF/MPERF, excluding idle time
} PH_HW_CPU_SENSOR, *PPH_HW_CPU_SENSOR;

// Primary timings; the cycle counts are in memory clock cycles.
typedef struct _PH_HW_MEMORY_TIMINGS
{
    ULONG CasLatency;
    ULONG RasToCasDelay; // tRCD, read
    ULONG RowPrecharge; // tRP
    ULONG RowActive; // tRAS
    ULONG RowCycle; // tRC, 0 when the controller has none
    ULONG RefreshCycle; // tRFC, 0 when unknown
    ULONG CommandRate; // 1, 2 or 3; 0 for N:1
    BOOLEAN GearDownValid;
    BOOLEAN GearDown;
} PH_HW_MEMORY_TIMINGS, *PPH_HW_MEMORY_TIMINGS;

typedef struct _PH_HW_PACKAGE_SENSOR
{
    ULONG Valid; // PH_HW_CAP_* bits whose values are current
    FLOAT Temperature;
    FLOAT TjMax;
    FLOAT MaxCoreTemperature;
    FLOAT PackagePower;
    FLOAT CorePower;
    FLOAT DramPower;
    FLOAT PlatformPower;
    FLOAT PowerLimit;
    FLOAT BaseMhz;
    FLOAT BusMhz;
    ULONG ThrottleFlags;
    ULONG CcdValidMask;
    FLOAT CcdTemperature[PH_HW_MAX_CCD];
    FLOAT MemoryClockMhz; // MEMCLK at the nominal 100 MHz reference clock
    FLOAT MemoryDataRate; // MT/s
    PH_CIRCULAR_BUFFER_FLOAT TemperatureHistory;
    PH_CIRCULAR_BUFFER_FLOAT PackagePowerHistory;
    ULONG MemoryGear; // 1, 2 or 4; 0 when the platform reports none
    FLOAT MemoryReadBandwidth; // MB/s (10^6 bytes)
    FLOAT MemoryWriteBandwidth; // MB/s (10^6 bytes)
    FLOAT CoresTemperature;
    FLOAT GraphicsTemperature;
    FLOAT ActivePowerLimit1; // the lower of the MSR and MMIO limits
    FLOAT ActivePowerLimit2;
    PH_HW_MEMORY_TIMINGS MemoryTimings;
} PH_HW_PACKAGE_SENSOR, *PPH_HW_PACKAGE_SENSOR;
// end_phapppub

extern PH_CIRCULAR_BUFFER_FLOAT PhHwCpuTemperatureHistory;
extern PH_CIRCULAR_BUFFER_FLOAT PhHwCpuPowerHistory;
extern FLOAT PhHwCpuTemperature;
extern FLOAT PhHwCpuPower;

VOID PhHwProviderInitialization(
    VOID
    );

VOID PhHwProviderUpdate(
    VOID
    );

VOID PhHwProviderUpdateHistory(
    VOID
    );

typedef enum _PH_HW_NAMED_SENSOR_KIND
{
    PhHwNamedSensorDimmTemperature, // PH_HW_CAP_DIMM_TEMP, degrees Celsius
    PhHwNamedSensorUncoreClock, // PH_HW_CAP_UNCORE_CLOCK, MHz
    PhHwNamedSensorTpmiPower, // PH_HW_CAP_TPMI_POWER, watts
    PhHwNamedSensorSmuTemperature, // PH_HW_CAP_SMU from here; degrees Celsius
    PhHwNamedSensorSmuPower, // watts
    PhHwNamedSensorSmuPowerLimit, // watts
    PhHwNamedSensorSmuCurrentLimit, // amperes
    PhHwNamedSensorSmuTemperatureLimit, // degrees Celsius
    PhHwNamedSensorSmuClock, // MHz
    PhHwNamedSensorSmuClockLimit, // MHz
    PhHwNamedSensorSmuMemoryClock, // MHz, Memory component
    PhHwNamedSensorSmuMemoryLoad, // percent, Memory component
    PhHwNamedSensorSmuVoltage, // volts
    PhHwNamedSensorSmuCurrent, // amperes
    PhHwNamedSensorSmuResidency, // percent
    PhHwNamedSensorFanSpeed, // PH_HW_CAP_FAN_SPEED, RPM, System component
    PhHwNamedSensorDimmVoltage, // PH_HW_CAP_DIMM_VOLTAGE, volts; Id is slot * PH_HW_PMIC_RAILS + rail
    PhHwNamedSensorDimmCurrent, // PH_HW_CAP_DIMM_PMIC, amperes; Id is slot * PH_HW_PMIC_RAILS + rail
    PhHwNamedSensorDimmPower, // PH_HW_CAP_DIMM_PMIC, watts; Id is slot * (PH_HW_PMIC_RAILS + 1) + rail, rail PH_HW_PMIC_RAILS the module total
    PhHwNamedSensorDimmPmicTemperature, // PH_HW_CAP_DIMM_PMIC, degrees Celsius, the lower bound of an 85 C or higher bucket; Id is slot
    PhHwNamedSensorAcpiTemperature, // PH_HW_CAP_ACPI_TEMP, degrees Celsius, System component
    PhHwNamedSensorFanControl, // PH_HW_CAP_FAN_CONTROL, percent, System component
    PhHwNamedSensorAcpiPower, // PH_HW_CAP_ACPI_POWER, watts, System component
    PhHwNamedSensorPlatformResidency, // PH_HW_CAP_RESIDENCY, percent, CPU component
    PhHwNamedSensorSmuBandwidth, // PH_HW_CAP_SMU, MB/s (10^6 bytes), Memory component
    PhHwNamedSensorPowerLimit, // PH_HW_CAP_PLATFORM_LIMITS, watts, CPU component
    PhHwNamedSensorPmtTemperature, // PH_HW_CAP_TPMI_POWER, degrees Celsius, CPU component: Intel PMT SoC and core telemetry
    PhHwNamedSensorAcpiVoltage, // PH_HW_CAP_ACPI_TEMP, volts, System component (ASUS ATK0110)
    PhHwNamedSensorZoneTemperature, // Windows counters, degrees Celsius, Windows component: ACPI thermal zones
    PhHwNamedSensorZonePassiveLimit, // Windows counters, percent, Windows component
    PhHwNamedSensorMeterPower, // Windows counters, watts, Windows component: energy meters
    PhHwNamedSensorOsClock, // Windows counters, MHz, Windows component; Id is the flat processor index
    PhHwNamedSensorOsPerformanceLimit, // Windows counters, percent, Windows component
    PhHwNamedSensorMaximum
} PH_HW_NAMED_SENSOR_KIND;

#define PH_HW_NAMED_SENSOR_NAME_LENGTH 48
#define PH_HW_PMIC_RAILS 3 // VDD (SWA), VDDQ (SWB), VPP (SWC)

// A sensor whose name comes from the hardware (SMBIOS locators, TPMI topology, SMU table layouts).
typedef struct _PH_HW_NAMED_SENSOR
{
    PH_HW_NAMED_SENSOR_KIND Kind;
    ULONG PackageIndex; // the component
    ULONG Id; // unique within the kind and package; orders the sensors
    BOOLEAN Active; // FALSE once dropped for the session
    BOOLEAN Valid; // Value is current
    FLOAT Value;
    WCHAR Name[PH_HW_NAMED_SENSOR_NAME_LENGTH];
} PH_HW_NAMED_SENSOR, *PPH_HW_NAMED_SENSOR;

ULONG PhHwGetNamedSensorCount(
    VOID
    );

BOOLEAN PhHwCounterSensorsPresent(
    VOID
    );

_Success_(return)
BOOLEAN PhHwGetNamedSensor(
    _In_ ULONG Index,
    _Out_ PPH_HW_NAMED_SENSOR Sensor
    );

_Success_(return)
BOOLEAN PhHwGetNamedSensorValue(
    _In_ PH_HW_NAMED_SENSOR_KIND Kind,
    _In_ ULONG PackageIndex,
    _In_ ULONG Id,
    _Out_ PFLOAT Value
    );

// A sensor published by a hardware sensor source (PhRegisterHardwareSensorSource).
typedef struct _PH_HW_SOURCE_SENSOR
{
    ULONG SourceId; // registration order, never reused
    ULONG ComponentId;
    PPH_STRING ComponentName;
    ULONG Type; // PH_HW_SENSOR_TYPE
    ULONG SensorId;
    PPH_STRING SensorName;
    ULONG Flags; // PH_HW_SENSOR_REPORT_*
    FLOAT Value;
    BOOLEAN Current; // reported in the last update
} PH_HW_SOURCE_SENSOR, *PPH_HW_SOURCE_SENSOR;
typedef const PH_HW_SOURCE_SENSOR* PCPH_HW_SOURCE_SENSOR;

_Function_class_(PH_HW_SOURCE_SENSOR_CALLBACK)
typedef VOID (NTAPI PH_HW_SOURCE_SENSOR_CALLBACK)(
    _In_ PCPH_HW_SOURCE_SENSOR Sensor,
    _In_opt_ PVOID Context
    );
typedef PH_HW_SOURCE_SENSOR_CALLBACK *PPH_HW_SOURCE_SENSOR_CALLBACK;

VOID PhHwEnumerateSourceSensors(
    _In_ PPH_HW_SOURCE_SENSOR_CALLBACK Callback,
    _In_opt_ PVOID Context
    );

BOOLEAN PhHwSourceSensorsPresent(
    VOID
    );

VOID PhHwSweepSensorSources(
    VOID
    );

ULONG PhHwGetPackageValidMask(
    VOID
    );

VOID PhHwWriteDiagnostics(
    _Inout_ PPH_STRING_BUILDER Builder
    );

VOID PhHwWriteSourceDiagnostics(
    _Inout_ PPH_STRING_BUILDER Builder
    );

_Ret_maybenull_
PCPH_STRINGREF PhHwGetSensorsUnavailableReason(
    VOID
    );

BOOLEAN PhHwSensorsProbed(
    VOID
    );

PPH_STRING PhHwCreateReport(
    _In_opt_ PCPH_STRINGREF Sensors
    );

// begin_phapppub
PHAPPAPI
BOOLEAN
NTAPI
PhHwSensorsEnabled(
    VOID
    );

PHAPPAPI
BOOLEAN
NTAPI
PhHwSensorsAvailable(
    VOID
    );

PHAPPAPI
ULONG
NTAPI
PhHwGetCapabilities(
    VOID
    );

PHAPPAPI
PPH_HW_CPU_SENSOR
NTAPI
PhHwGetCpuSensor(
    _In_ ULONG Index
    );

PHAPPAPI
PPH_HW_PACKAGE_SENSOR
NTAPI
PhHwGetPackageSensor(
    _In_ ULONG Index
    );

PHAPPAPI
ULONG
NTAPI
PhHwGetPackageCount(
    VOID
    );

PHAPPAPI
FLOAT
NTAPI
PhHwGetCpuTemperatureLimit(
    VOID
    );

_Success_(return)
PHAPPAPI
BOOLEAN
NTAPI
PhHwGetChipsetTemperature(
    _Out_ PFLOAT Celsius
    );

PHAPPAPI
VOID
NTAPI
PhHwFormatTemperature(
    _In_ FLOAT Celsius,
    _In_ USHORT Precision,
    _Out_writes_(PH_HW_TEMPERATURE_FORMAT_COUNT) PPH_FORMAT Format
    );
// end_phapppub

#endif
