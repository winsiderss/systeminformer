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

#ifndef _PH_HWSMU_H
#define _PH_HWSMU_H

EXTERN_C_START

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
    PhHwNamedSensorPmtTemperature, // PH_HW_CAP_TPMI_POWER, degrees Celsius, CPU component: Intel PMT SoC and core telemetry, Qualcomm TSENS die sensors
    PhHwNamedSensorAcpiVoltage, // PH_HW_CAP_ACPI_TEMP, volts, System component (ATK0110)
    PhHwNamedSensorZoneTemperature, // Windows counters, degrees Celsius, Windows component: ACPI thermal zones; network adapters from Id 0x20000
    PhHwNamedSensorZonePassiveLimit, // Windows counters, percent, Windows component
    PhHwNamedSensorMeterPower, // Windows counters, watts, Windows component: energy meters
    PhHwNamedSensorOsClock, // Windows counters, MHz, Windows component; Id is the flat processor index
    PhHwNamedSensorOsPerformanceLimit, // Windows counters, percent, Windows component
    PhHwNamedSensorCoreIpc, // instructions per cycle, CPU component; Id is the flat processor index
    PhHwNamedSensorCoreMemoryStall, // percent of cycles stalled on memory, CPU component; Id is the flat processor index
    PhHwNamedSensorCStateResidency, // percent of time, CPU component; Id is the state, plus 0x100 + 16 * core for a core's
    PhHwNamedSensorRaplPower, // watts, CPU component; Id is the RAPL domain (1 PP1, the integrated graphics)
    PhHwNamedSensorZoneTrip, // degrees Celsius, Windows component: ACPI thermal zone passive and critical trip points
    PhHwNamedSensorMaximum
} PH_HW_NAMED_SENSOR_KIND;

#define PH_HW_SMU_MAX_READINGS 160
#define PH_HW_SMU_MAX_CORE_SLOTS 16
#define PH_HW_SMU_NO_INDEX ULONG_MAX
#define PH_HW_HSMP_MAX_COMMANDS 32 // per-command state is one bit of a ULONG

// HSMP GET commands.
#define PH_HW_HSMP_TEST 0x01
#define PH_HW_HSMP_GET_SOCKET_POWER 0x04
#define PH_HW_HSMP_GET_SOCKET_POWER_LIMIT 0x06
#define PH_HW_HSMP_GET_SOCKET_POWER_LIMIT_MAX 0x07
#define PH_HW_HSMP_GET_FCLK_MCLK 0x0F
#define PH_HW_HSMP_GET_CCLK_THROTTLE_LIMIT 0x10
#define PH_HW_HSMP_GET_C0_PERCENT 0x11
#define PH_HW_HSMP_GET_DDR_BANDWIDTH 0x14
#define PH_HW_HSMP_GET_TEMP_MONITOR 0x15
#define PH_HW_HSMP_GET_DIMM_POWER 0x17 // argument: the DIMM address
#define PH_HW_HSMP_GET_DIMM_THERMAL 0x18 // argument: the DIMM address
#define PH_HW_HSMP_DIMM_CHANNELS 12 // SP5: UMC 0-11
#define PH_HW_HSMP_GET_SOCKET_FREQ_LIMIT 0x19
#define PH_HW_HSMP_GET_RAILS_SVI 0x1B
#define PH_HW_HSMP_GET_SOCKET_FMAX_FMIN 0x1C

// HSMP 19h socket frequency limit sources.
#define PH_HW_HSMP_LIMIT_CHTC 0x0001
#define PH_HW_HSMP_LIMIT_PROCHOT 0x0002
#define PH_HW_HSMP_LIMIT_TDC 0x0004
#define PH_HW_HSMP_LIMIT_PPT 0x0008
#define PH_HW_HSMP_LIMIT_OPN_MAX 0x0010
#define PH_HW_HSMP_LIMIT_RELIABILITY 0x0020
#define PH_HW_HSMP_LIMIT_APML 0x0040
#define PH_HW_HSMP_LIMIT_HSMP 0x0080

// Named sensor IDs of the SMU kinds; an HSMP reading has PH_HW_SMU_ID_HSMP set.
#define PH_HW_SMU_ID_HSMP 0x1000
typedef enum _PH_HW_SMU_ID
{
    PhHwSmuIdSocket = 0x00,
    PhHwSmuIdPpt,
    PhHwSmuIdPptSlow,
    PhHwSmuIdStapm,
    PhHwSmuIdTdc,
    PhHwSmuIdEdc,
    PhHwSmuIdThermal,
    PhHwSmuIdSocketMax,
    PhHwSmuIdPptFast,
    PhHwSmuIdPptApu,
    PhHwSmuIdTdcSoc,
    PhHwSmuIdEdcSoc,
    PhHwSmuIdGfxThermal,
    PhHwSmuIdSocThermal,
    PhHwSmuIdSkin,
    PhHwSmuIdTctl,
    PhHwSmuIdCoreRail = 0x10,
    PhHwSmuIdSocRail,
    PhHwSmuIdMiscRail,
    PhHwSmuIdSviRails,
    PhHwSmuIdMemRail,
    PhHwSmuIdCoreDomain,
    PhHwSmuIdCoreRailPeak,
    PhHwSmuIdCoreRailAverage,
    PhHwSmuIdCoreRail1, // second core rail (VDD1)
    PhHwSmuIdSoc = 0x20,
    PhHwSmuIdIod,
    PhHwSmuIdGfx,
    PhHwSmuIdHotspot,
    PhHwSmuIdVddp = 0x30,
    PhHwSmuIdVddg,
    PhHwSmuIdVddgIod,
    PhHwSmuIdVddgCcd,
    PhHwSmuIdVddm,
    PhHwSmuIdVsoc,
    PhHwSmuIdFclk = 0x40,
    PhHwSmuIdUclk,
    PhHwSmuIdMclk,
    PhHwSmuIdThrottleLimit,
    PhHwSmuIdSocketFrequencyLimit,
    PhHwSmuIdFmax,
    PhHwSmuIdFmin,
    PhHwSmuIdPackageC6 = 0x50,
    PhHwSmuIdSocketC0,
    PhHwSmuIdDramBandwidth,
    PhHwSmuIdDramBandwidthMax,
    PhHwSmuIdDramBandwidthUsed,
    PhHwSmuIdCoreRailVrm = 0x60,
    PhHwSmuIdSocRailVrm,
    PhHwSmuIdMiscRailVrm,
    PhHwSmuIdCoreRail1Vrm,
    PhHwSmuIdL3 = 0x100, // + CCX
    PhHwSmuIdCore = 0x200, // + core
    PhHwSmuIdDimm = 0x400 // + DIMM address
} PH_HW_SMU_ID;

// Sensor name is Name, then Index unless PH_HW_SMU_NO_INDEX, then Suffix.
typedef struct _PH_HW_SMU_READING
{
    PH_HW_NAMED_SENSOR_KIND Kind;
    ULONG Id; // unique within the kind and package; orders the sensors
    PCWSTR Name;
    ULONG Index;
    PCWSTR Suffix; // optional
    FLOAT Value;
} PH_HW_SMU_READING, *PPH_HW_SMU_READING;

typedef const PH_HW_SMU_READING* PCPH_HW_SMU_READING;

typedef struct _PH_HW_SMU_DECODE
{
    ULONG Count;
    BOOLEAN MemoryClockValid; // DDR MEMCLK, half the data rate; not in Readings
    BOOLEAN CoresEnded; // a populated core slot outside the locked set ended the per-core values
    BOOLEAN FrequencyLimitSourcesValid;
    FLOAT MemoryClock; // MHz
    ULONG FrequencyLimitSources; // HSMP 19h [15:0], PH_HW_HSMP_LIMIT_*; not in Readings
    PH_HW_SMU_READING Readings[PH_HW_SMU_MAX_READINGS];
} PH_HW_SMU_DECODE, *PPH_HW_SMU_DECODE;

typedef const PH_HW_SMU_DECODE* PCPH_HW_SMU_DECODE;

// Per-core slot state of a PM table session; zero-initialize at session start.
typedef struct _PH_HW_SMU_CORE_SLOTS
{
    ULONG Mask; // the populated slots, locked when their count first equals the core count
    BOOLEAN Locked;
    BOOLEAN Ended; // no per-core values for the rest of the session
} PH_HW_SMU_CORE_SLOTS, *PPH_HW_SMU_CORE_SLOTS;

PHLIBAPI
_Success_(return)
BOOLEAN
NTAPI
PhHwSmuDecodePmTable(
    _In_ ULONG Version,
    _In_reads_bytes_(Length) PCVOID Table,
    _In_ ULONG Length,
    _In_ ULONG CoreCount,
    _Inout_ PPH_HW_SMU_CORE_SLOTS CoreSlots,
    _Inout_ PPH_HW_SMU_DECODE Decode
    );

PHLIBAPI extern CONST UCHAR PhHwSmuHsmpCommands[];
PHLIBAPI extern CONST ULONG PhHwSmuHsmpCommandCount;

PHLIBAPI
VOID
NTAPI
PhHwSmuDecodeHsmpDimm(
    _In_ ULONG Opcode,
    _In_ ULONG Response,
    _Inout_ PPH_HW_SMU_DECODE Decode
    );

PHLIBAPI
VOID
NTAPI
PhHwSmuDecodeHsmp(
    _In_ ULONG Opcode,
    _In_reads_(2) PCULONG Response,
    _Inout_ PPH_HW_SMU_DECODE Decode
    );

EXTERN_C_END

#endif
