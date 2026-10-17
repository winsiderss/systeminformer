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
#include <hwprv.h>
#include <procprv.h>

#include <hwsmu.h>

#include <cpusensor.h>
#include <phfirmware.h>
#include <kphuser.h>
#include <kphdyn.h>

#include <SetupAPI.h>
#include <perflib.h>
// devpkey.h has no include guard and may already be included via netioapi.h
#ifndef DEVPKEY_H_INCLUDED
#include <devpkey.h>
#endif

#include <trace.h>

#define PHP_HW_RETRY_LIMIT 3
#define PHP_HW_STRIKE_LIMIT 2 // consecutive transient failures that end an item for the session
#define PHP_HW_PCI_CONTENDED_LIMIT 2 // consecutive timed-out Access_PCI waits after which ticks stop waiting
#define PHP_HW_MAX_ROOTS 32
#define PHP_HW_AMD_VENDOR_ID 0x1022
#define PHP_HW_HYGON_VENDOR_ID 0x1D94
#define PHP_HW_INTEL_VENDOR_ID 0x8086
#define PHP_HW_ANY_VENDOR_ID 0xFFFF // no PCI function has it
#define PHP_HW_MAX_WINDOWS 8
#define PHP_HW_REGISTER_BATCH 256 // MSR and system register entries per request
#define PHP_HW_IMC_COUNT_BYTES 64 // bytes per IMC data read/write count
#define PHP_HW_IMC_BYTES_PER_TRANSFER_BOUND 32 // twice the 16 bytes per transfer of two 64-bit client channels
#define PHP_HW_IMC_DATA_RATE_BOUND 12000.0f // MT/s when the data rate is unknown
#define PHP_HW_IMC_DATA_RATE_BOUND_32 4800.0f // MT/s when unknown, parts with 32-bit counters (DDR3/DDR4)
#define PHP_HW_IMC_MAX_INTERVAL 10.0f // seconds; a longer interval is a gap
#define PHP_HW_SOURCE_MAX_SENSORS 1024 // per source
#define PHP_HW_SOURCE_MAX_NAME 128 // characters
#define PHP_HW_SOURCE_BUDGET_MS 10 // callback time above which a source is traced
#define PHP_HW_ENTRY_UNSET STATUS_UNSUCCESSFUL
#define PHP_HW_MEMORY_CLOCK_INTERVAL 5 // ticks between memory clock reads
#define PHP_HW_PROBE_RETRY_TICKS 10 // ticks between unanswered capability probes
#define PHP_HW_MAX_NAMED_SENSORS 128
#define PHP_HW_MAX_SMBUS_HOSTS 4
#define PHP_HW_DIMM_COUNT 8 // SPD 50h-57h, TSOD 18h-1Fh
#define PHP_HW_DIMM_INTERVAL_MS 3000 // between DIMM temperature reads on the SMBus worker
#define PHP_HW_SMBUS_STRIKE_LIMIT 2 // consecutive timed-out or not-ready requests that end the SMBus for the session
#define PHP_HW_SMBUS_TIMEOUT_MS 250 // per SMBus request
#define PHP_HW_SMBUS_OP_TSOD 0 // smbus_read op: TSOD 18h-1Fh, word reads of 00h-07h
#define PHP_HW_SMBUS_OP_SPD 1 // smbus_read op: SPD 50h-57h, byte/word reads of 00h-FFh
#define PHP_HW_SMBUS_OP_PMIC 2 // smbus_read op: DDR5 PMIC 48h-4Fh, byte reads of 04h-06h, 08h-0Fh, 1Ah-1Bh, 20h-33h, 3Bh-3Dh
#define PHP_HW_PMIC_ADDRESS 0x48
#define PHP_HW_PMIC_RAIL_VPP 2 // rails are VDD, VDDQ, VPP
#define PHP_HW_PMIC_TRIES 3 // PMIC batches while the bus is busy
#define PHP_HW_PMIC_SENSOR_CURRENT 0 // + rail
#define PHP_HW_PMIC_SENSOR_POWER (PHP_HW_PMIC_SENSOR_CURRENT + PH_HW_PMIC_RAILS) // + rail; + PH_HW_PMIC_RAILS is the module total
#define PHP_HW_PMIC_SENSOR_TEMPERATURE (PHP_HW_PMIC_SENSOR_POWER + PH_HW_PMIC_RAILS + 1)
#define PHP_HW_PMIC_SENSORS (PHP_HW_PMIC_SENSOR_TEMPERATURE + 1)
#define PHP_HW_SPD_NVM_BYTES 14 // SPD bytes 2, 3, 20-21, 30-39 of NVM page 0
#define PHP_HW_TSOD_ADDRESS 0x18
#define PHP_HW_SPD_ADDRESS 0x50
#define PHP_HW_DIMM_MIN_CELSIUS -20.0f
#define PHP_HW_DIMM_MAX_CELSIUS 120.0f
#define PHP_HW_MAX_VSEC_DEVICES 16
#define PHP_HW_MAX_TPMI_REGIONS 64
#define PHP_HW_MAX_TPMI_ENTRIES 256
#define PHP_HW_TPMI_REGION_ENTRIES 32 // RAPL domains of a 4 KiB instance
#define PHP_HW_TPMI_UNMAPPED_ID 0x80000000 // named sensor id bit of domains not mapped to a package
#define PHP_HW_LIMIT_PL4 0 // named limit sensors of a package
#define PHP_HW_LIMIT_PLATFORM_PL1 1
#define PHP_HW_LIMIT_PLATFORM_PL2 2
#define PHP_HW_LIMIT_SENSORS 3
#define PHP_HW_GT_CLOCK_ID 0x47540000 // named sensor id of the MCHBAR graphics clock
#define PHP_HW_RAPL_LIMITED_ID 1 // named residency sensor id of PACKAGE_RAPL_PERF_STATUS; LPIT S0ix is 0
#define PHP_HW_PMT_ID_BASE 0x50000000 // named sensor ids of the PMT samples: | device << 16 | instance << 8 | sample
#define PHP_HW_MAX_PMT_DEVICES 4
#define PHP_HW_MAX_PMT_ENTRIES 64
#define PHP_HW_MSR_UNCORE_ID 0x40000000 // named sensor id of the MSR uncore clock, apart from the TPMI domains
#define PHP_HW_SMU_INTERVAL_MS 1000 // minimum interval between SMU passes
#define PHP_HW_SMU_STRIKE_LIMIT 2 // consecutive busy or timed-out requests that end an SMU source for the session
#define PHP_HW_MAILBOX_TIMEOUT_MS 250 // per mailbox batch
#define PHP_HW_SMU_TABLE_TIMEOUT_MS 250 // per PM table copy
#define PHP_HW_SMU_MAX_PACKAGES 8
#define PHP_HW_SMU_MAX_SENSORS 256
#define PHP_HW_SMU_MAX_MAILBOXES 4 // mailbox ops of a root probed for HSMP
#define PHP_HW_SMU_TEST_ARGUMENT 0x5A // HSMP TEST answers the argument plus one
#define PHP_HW_SMU_ORIGIN_TABLE 0
#define PHP_HW_SMU_ORIGIN_HSMP 1
#define PHP_HW_ARM_PROBE_REGISTERS 6 // ARM64 thread-scope registers of the plan, probed on every LP
#define PHP_HW_ARM_AMU_COUNTERS 0x3 // AMCNTENSET0_EL0: CPU_CYCLES and CNT_CYCLES enabled
#define PHP_HW_MAX_FANS 32 // entries of the fan worker: fans, DPTF participants, power meters, DPTF power getters
#define PHP_HW_FAN_INTERVAL_MS 2000 // at least this long between evaluations of a device
#define PHP_HW_FAN_TIMEOUT_MS 500 // per evaluation
#define PHP_HW_FAN_STRIKE_LIMIT 2 // consecutive timed-out evaluations that end a device for the session
#define PHP_HW_FAN_BUSY_LIMIT 5 // consecutive busy evaluations that end a device for the session
#define PHP_HW_FAN_OP_FST 0 // acpi_eval ops of the PNP0C0B row
#define PHP_HW_FAN_OP_FIF 1
#define PHP_HW_FAN_OP_FPS 2
#define PHP_HW_FAN_FPS_OUTPUT_LENGTH 0x800 // the row allows 0x800 for _FPS
#define PHP_HW_FAN_MAX_STATES 16 // _FPS states kept for the report
#define PHP_HW_FAN_OUTPUT_LENGTH 0x100 // ACPI_EVAL_OUTPUT_BUFFER bytes of every op but _FPS
#define PHP_HW_FAN_MAX_RPM 30000
#define PHP_HW_LPIT_WINDOW 0 // platform window op of the Intel client CPU rows: entry="0", the first usable LPIT counter
#define PHP_HW_LPIT_HEADER 36 // ACPI table header
#define PHP_HW_PWRM_WINDOW_FIRST 1 // platform window ops of the Intel client CPU rows: the PWRM block at SLP_S0 193Ch, then LPM 1098h
#define PHP_HW_PWRM_WINDOW_COUNT 2
#define PHP_HW_CROS_EC_WINDOW 0 // platform window op of the system board rows, which are reserved for the ChromeOS EC map
#define PHP_HW_CROS_EC_READ 0x20 // 00h-1Fh: temperatures, fans, second temperature bank
#define PHP_HW_CROS_EC_FAN 0x10 // FAN 0-3, u16 LE
#define PHP_HW_CROS_EC_TEMP_B 0x18 // TEMP_SENSOR_B 16-23
#define PHP_HW_CROS_EC_ID 0x20 // 'E', 'C', ID_VERSION, THERMAL_VERSION
#define PHP_HW_CROS_EC_ID_LENGTH 4
#define PHP_HW_CROS_EC_TEMPS 16 // the first bank; the second has 8
#define PHP_HW_CROS_EC_FANS 4
#define PHP_HW_CROS_EC_SENSORS (PHP_HW_CROS_EC_TEMPS + PHP_HW_CROS_EC_FANS + 8) // temperatures 0-15, fans, temperatures 16-23
#define PHP_HW_CROS_EC_TEMP_INVALID 0xFC // FCh not calibrated, FDh not powered, FEh error: listed, not a reading
#define PHP_HW_CROS_EC_TEMP_ABSENT 0xFF
#define PHP_HW_CROS_EC_ID_BASE 0x45430000 // named sensor ids of the EC, apart from the ACPI devices
#define PHP_HW_LPIT_NATIVE_LENGTH 56
#define PHP_HW_DPTF_OP_STA 0 // acpi_eval ops of the DPTF generic participant row
#define PHP_HW_DPTF_OP_STR 1
#define PHP_HW_DPTF_OP_PTYP 2
#define PHP_HW_DPTF_OP_TMP 3
#define PHP_HW_DPTF_OP_PSV 4 // the trip points follow: _PSV, _CRT, _AC0, GTSH
#define PHP_HW_DPTF_TRIPS 4
#define PHP_HW_METER_OP_STA 0 // acpi_eval ops of the ACPI000D row
#define PHP_HW_METER_OP_PMC 1
#define PHP_HW_METER_OP_PMM 2
#define PHP_HW_METER_OP_GAI 3
#define PHP_HW_METER_PMC_OUTPUT_LENGTH 0x200 // the row allows 0x200 for _PMC
#define PHP_HW_METER_PMC_INTEGERS 11 // _PMC capabilities .. maximum cap
#define PHP_HW_METER_PMC_ELEMENTS 14 // then the model, serial and OEM strings
#define PHP_HW_METER_MAX_INTERVAL_MS 60000 // _GAI above this is polled at this interval
#define PHP_HW_METER_MAX_MILLIWATTS 10000000 // 10 kW
#define PHP_HW_ATK_OP_TSIF 0 // acpi_eval ops of the ATK0110 row
#define PHP_HW_ATK_OP_VSIF 1
#define PHP_HW_ATK_OP_FSIF 2
#define PHP_HW_ATK_OP_RTMP 3
#define PHP_HW_ATK_OP_RVLT 4
#define PHP_HW_ATK_OP_RFAN 5
#define PHP_HW_ATK_LIST_OUTPUT_LENGTH 0x800 // the row allows 0x800 for the sensor lists
#define PHP_HW_ATK_SENSOR_FIELDS 5 // id, name, limit1, limit2, enable
#define PHP_HW_DYTC_COMMANDS 4 // acpi_eval ops of the HKEY row: query, get, function capabilities, MMC get
#define PHP_HW_DYTC_FC_MMC 27 // function capabilities: MMC
#define PHP_HW_POWER_OP_STA 0 // acpi_eval ops of the DPTF power participant row
#define PHP_HW_POWER_OP_PTYP 1
#define PHP_HW_POWER_OP_PSRC 2 // the static getters follow: PSRC, ARTG, CTYP, RBHF, VBNL, CMPP
#define PHP_HW_POWER_OP_PMAX 3
#define PHP_HW_POWER_OP_ARTG 4
#define PHP_HW_POWER_OP_PBSS 5
#define PHP_HW_POWER_OP_PROP 6
#define PHP_HW_POWER_OP_CTYP 7
#define PHP_HW_POWER_OP_RBHF 8
#define PHP_HW_POWER_OP_VBNL 9
#define PHP_HW_POWER_OP_CMPP 10
#define PHP_HW_POWER_STATICS 6
#define PHP_HW_DPTF_MIN_DECIKELVIN 2000 // -73 C
#define PHP_HW_DPTF_MAX_DECIKELVIN 4500 // 177 C
// ACPI_EVAL_OUTPUT_BUFFER_V1 and ACPI_METHOD_ARGUMENT_V1 of acpiioct.h (WDK-only, not in the SDK include path).
#define PHP_HW_ACPI_OUTPUT_SIGNATURE 'BoeA'
#define PHP_HW_ACPI_OUTPUT_HEADER 12 // Signature, Length, Count
#define PHP_HW_ACPI_ARGUMENT_HEADER 4 // Type, DataLength
#define PHP_HW_ACPI_ARGUMENT_INTEGER 0
#define PHP_HW_ACPI_ARGUMENT_STRING 1
#define PHP_HW_ACPI_ARGUMENT_BUFFER 2
#define PHP_HW_ACPI_ARGUMENT_PACKAGE 3

typedef enum _PHP_HW_REG
{
    PhpHwRegAperf,
    PhpHwRegMperf,
    PhpHwRegPerfStatus, // Intel IA32_PERF_STATUS (thread), AMD HwPstateStatus (core)
    PhpHwRegThermStatus,
    PhpHwRegCoreEnergy,
    PhpHwRegPackageThermStatus,
    PhpHwRegPackageEnergy,
    PhpHwRegPp0Energy,
    PhpHwRegDramEnergy,
    PhpHwRegPlatformEnergy,
    PhpHwRegArmCounter, // ARM64 CNTVCT_EL0
    PhpHwRegArmCycles, // ARM64 AMEVCNTR00_EL0 (CPU_CYCLES)
    PhpHwRegArmConstantCycles, // ARM64 AMEVCNTR01_EL0 (CNT_CYCLES)
    PhpHwRegArmPmuCycles, // ARM64 PMCCNTR_EL0, without the AMU
    PhpHwRegUncorePerfStatus, // Intel MSR_UNCORE_PERF_STATUS (pkg)
    PhpHwRegLimitReasons, // Intel MSR_CORE_PERF_LIMIT_REASONS (pkg)
    PhpHwRegVrCurrentConfig, // Intel MSR_VR_CURRENT_CONFIG, PL4 (pkg)
    PhpHwRegPlatformPowerLimit, // Intel MSR_PLATFORM_POWER_LIMIT (pkg)
    PhpHwRegTemperatureTarget, // probe only from here
    PhpHwRegPlatformInfo,
    PhpHwRegRaplUnit,
    PhpHwRegPowerLimit,
    PhpHwRegPStateDef0,
    PhpHwRegArmFrequency, // ARM64 CNTFRQ_EL0
    PhpHwRegArmAmuEnable, // ARM64 AMCNTENSET0_EL0
    PhpHwRegMaximum
} PHP_HW_REG;
C_ASSERT(PhpHwRegMaximum <= 32);

typedef enum _PHP_HW_ARM_SOURCE
{
    PhpHwArmSourceNone,
    PhpHwArmSourceAmu, // AMEVCNTR00_EL0 and AMEVCNTR01_EL0
    PhpHwArmSourcePmu // PMCCNTR_EL0
} PHP_HW_ARM_SOURCE;

typedef enum _PHP_HW_SCOPE
{
    PhpHwScopeThread,
    PhpHwScopeCore,
    PhpHwScopePackage
} PHP_HW_SCOPE;

typedef struct _PHP_HW_ITEM
{
    BOOLEAN Disabled;
    UCHAR RetryCount;
} PHP_HW_ITEM, *PPHP_HW_ITEM;

typedef struct _PHP_HW_REGISTER
{
    ULONG Register; // 0 when not in the plan
    PHP_HW_SCOPE Scope;
    BOOLEAN ProbeOnly;
    PHP_HW_ITEM Item;
} PHP_HW_REGISTER, *PPHP_HW_REGISTER;

typedef struct _PHP_HW_SAMPLE
{
    ULONG Valid;
    ULONG LastValid;
    ULONG64 Value[PhpHwRegMaximum];
    ULONG64 LastValue[PhpHwRegMaximum];
} PHP_HW_SAMPLE, *PPHP_HW_SAMPLE;

typedef struct _PHP_HW_CORE
{
    ULONG FirstProcessor;
    ULONG PackageIndex;
    PHP_HW_SAMPLE Sample;
} PHP_HW_CORE, *PPHP_HW_CORE;

typedef enum _PHP_HW_TPMI_ENTRY_KIND
{
    PhpHwTpmiPackageEnergy,
    PhpHwTpmiDramEnergy,
    PhpHwTpmiPlatformEnergy,
    PhpHwTpmiEnergyMaximum,
    PhpHwTpmiUncoreStatus = PhpHwTpmiEnergyMaximum
} PHP_HW_TPMI_ENTRY_KIND;

typedef struct _PHP_HW_PACKAGE
{
    ULONG FirstProcessor;
    ULONG CoreCount;
    ULONG TjMax;
    PHP_HW_SAMPLE Sample;

    FLOAT MsrPowerLimit1; // watts, 0 when unavailable
    FLOAT MsrPowerLimit2;
    ULONG UncoreNamedIndex; // the MSR uncore clock sensor, ULONG_MAX when TPMI or nothing reports it
    ULONG LimitNamedIndex[PHP_HW_LIMIT_SENSORS]; // PHP_HW_LIMIT_*; ULONG_MAX when not listed

    BOOLEAN HasRoot;
    KPH_HW_PCI_LOCATOR Root;
    PHP_HW_ITEM Tctl;
    ULONG CcdMask;
    PHP_HW_ITEM Ccd[PH_HW_MAX_CCD];
    PHP_HW_ITEM MemoryClock;
    ULONG MemoryChannel; // SMN address of the first active UMC channel, ULONG_MAX until found
    BOOLEAN MemoryClockValid; // UmcMemoryClockMhz and UmcMemoryDataRate hold the last UMC decode
    FLOAT UmcMemoryClockMhz;
    FLOAT UmcMemoryDataRate;
    BOOLEAN MemoryTimingsRead; // read once per probe
    BOOLEAN MemoryTimingsValid;
    PH_HW_MEMORY_TIMINGS MemoryTimings;

    UCHAR DramIdleTicks; // samples whose DRAM energy did not move, until it does
    UCHAR PlatformIdleTicks;
    BOOLEAN DramEnergySeen; // the DRAM energy counter moved
    BOOLEAN PlatformEnergySeen;

    // Per-tick aggregation.
    ULONG TickValid;
    ULONG TickThrottleFlags;
    ULONG TickCoreCount;
    FLOAT TickMaxCoreTemperature;
    FLOAT TickCorePower;
    BOOLEAN TickMmioTemperatureValid;
    FLOAT TickMmioTemperature;
    FLOAT TickMmioPowerLimit1; // watts, 0 when unavailable or disabled
    FLOAT TickMmioPowerLimit2;

    ULONG TickTpmiEnergyRequired[PhpHwTpmiEnergyMaximum]; // TPMI RAPL domains read for the package this tick
    ULONG TickTpmiEnergyCount[PhpHwTpmiEnergyMaximum];
    FLOAT TickTpmiPower[PhpHwTpmiEnergyMaximum];
} PHP_HW_PACKAGE, *PPHP_HW_PACKAGE;

typedef struct _PHP_HW_BATCH_TAG
{
    PHP_HW_REG Reg;
    ULONG Index; // processor, core or package index by the register scope
} PHP_HW_BATCH_TAG, *PPHP_HW_BATCH_TAG;

// Registers of the Intel host bridge (MCHBAR), processor-thermal (BAR0) and PCH thermal (BAR0) windows.
typedef enum _PHP_HW_WINDOW_REG
{
    PhpHwWinMcBiosReq,
    PhpHwWinMcBiosData,
    PhpHwWinSaPerfStatus,
    PhpHwWinBclkFreq,
    PhpHwWinMemssBiosData,
    PhpHwWinImcReads32,
    PhpHwWinImcWrites32,
    PhpHwWinImc0Reads,
    PhpHwWinImc0Writes,
    PhpHwWinImc0ReadsAlt,
    PhpHwWinImc0WritesAlt,
    PhpHwWinImc1Reads,
    PhpHwWinImc1Writes,
    PhpHwWinImc1ReadsAlt,
    PhpHwWinImc1WritesAlt,
    PhpHwWinPowerUnit,
    PhpHwWinPackageTemperature,
    PhpHwWinPp0Temperature,
    PhpHwWinPp1Temperature,
    PhpHwWinTemperatureTarget,
    PhpHwWinPowerLimit1,
    PhpHwWinPowerLimit2,
    PhpHwWinPchTemp,
    PhpHwWinPchTsel,
    PhpHwWinGtPerfStatus,
    PhpHwWinRpStateCap,
    PhpHwWinFreqInfo,
    PhpHwWinRaplPerfStatus,
    PhpHwWinImc0Requests,
    PhpHwWinImc1Requests,
    PhpHwWinMc0Ch0RankTemperature,
    PhpHwWinMc0Ch1RankTemperature,
    PhpHwWinMc1Ch0RankTemperature,
    PhpHwWinMc1Ch1RankTemperature,
    PhpHwWinMaximum
} PHP_HW_WINDOW_REG;
C_ASSERT(PhpHwWinMaximum <= 64);

typedef struct _PHP_HW_WINDOW_REG_INFO
{
    ULONG Offset;
    UCHAR Width;
    CHAR Imc; // 'R' data reads, 'W' data writes, 0 otherwise
} PHP_HW_WINDOW_REG_INFO;

static CONST PHP_HW_WINDOW_REG_INFO PhpHwWindowRegInfo[PhpHwWinMaximum] =
{
    { PH_INTEL_MCHBAR_MC_BIOS_REQ, 4, 0 },
    { PH_INTEL_MCHBAR_MC_BIOS_DATA, 4, 0 },
    { PH_INTEL_MCHBAR_SA_PERF_STATUS, 4, 0 },
    { PH_INTEL_MCHBAR_BCLK_FREQ, 4, 0 },
    { PH_INTEL_MCHBAR_MEMSS_BIOS_DATA, 4, 0 },
    { PH_INTEL_MCHBAR_IMC_DATA_READS, 4, 'R' },
    { PH_INTEL_MCHBAR_IMC_DATA_WRITES, 4, 'W' },
    { PH_INTEL_MCHBAR_IMC_READS, 8, 'R' },
    { PH_INTEL_MCHBAR_IMC_WRITES, 8, 'W' },
    { PH_INTEL_MCHBAR_IMC_READS_ALT, 8, 'R' },
    { PH_INTEL_MCHBAR_IMC_WRITES_ALT, 8, 'W' },
    { PH_INTEL_MCHBAR_IMC_STRIDE + PH_INTEL_MCHBAR_IMC_READS, 8, 'R' },
    { PH_INTEL_MCHBAR_IMC_STRIDE + PH_INTEL_MCHBAR_IMC_WRITES, 8, 'W' },
    { PH_INTEL_MCHBAR_IMC_STRIDE + PH_INTEL_MCHBAR_IMC_READS_ALT, 8, 'R' },
    { PH_INTEL_MCHBAR_IMC_STRIDE + PH_INTEL_MCHBAR_IMC_WRITES_ALT, 8, 'W' },
    { PH_INTEL_MCHBAR_PKG_POWER_UNIT, 4, 0 },
    { PH_INTEL_MCHBAR_PKG_TEMPERATURE, 4, 0 },
    { PH_INTEL_MCHBAR_PP0_TEMPERATURE, 4, 0 },
    { PH_INTEL_MCHBAR_PP1_TEMPERATURE, 4, 0 },
    { PH_INTEL_MCHBAR_TEMPERATURE_TARGET, 4, 0 },
    { PH_INTEL_MCHBAR_PKG_POWER_LIMIT, 4, 0 },
    { PH_INTEL_MCHBAR_PKG_POWER_LIMIT + 4, 4, 0 },
    { PH_INTEL_PCH_THERMAL_TEMP, 2, 0 },
    { PH_INTEL_PCH_THERMAL_TSEL, 1, 0 },
    { PH_INTEL_MCHBAR_GT_PERF_STATUS, 4, 0 },
    { PH_INTEL_MCHBAR_RP_STATE_CAP, 4, 0 },
    { PH_INTEL_MCHBAR_FREQ_INFO_REC, 4, 0 },
    { PH_INTEL_MCHBAR_RAPL_PERF_STATUS, 4, 0 },
    { PH_INTEL_MCHBAR_IMC_REQUESTS, 8, 0 },
    { PH_INTEL_MCHBAR_IMC_STRIDE + PH_INTEL_MCHBAR_IMC_REQUESTS, 8, 0 },
    { PH_INTEL_MCHBAR_RANK_TEMPERATURE, 4, 0 },
    { PH_INTEL_MCHBAR_RANK_TEMPERATURE + 0x800, 4, 0 },
    { PH_INTEL_MCHBAR_IMC_STRIDE + PH_INTEL_MCHBAR_RANK_TEMPERATURE, 4, 0 },
    { PH_INTEL_MCHBAR_IMC_STRIDE + PH_INTEL_MCHBAR_RANK_TEMPERATURE + 0x800, 4, 0 },
};

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
} PHP_HW_SOURCE_SENSOR, *PPHP_HW_SOURCE_SENSOR;

typedef struct _PH_HW_SENSOR_SOURCE_REGISTRATION
{
    LIST_ENTRY ListEntry;
    ULONG Id;
    PPH_PLUGIN Plugin;
    PH_HW_SENSOR_SOURCE Source;
    PPH_LIST Sensors; // PPHP_HW_SOURCE_SENSOR in first-report order
    ULONG NextIndex; // where the next report is expected
    BOOLEAN CapTraced;
    BOOLEAN SlowTraced;
} PH_HW_SENSOR_SOURCE_REGISTRATION;

typedef enum _PHP_HW_DIMM_TYPE
{
    PhpHwDimmNone,
    PhpHwDimmTsod, // JC-42.4 thermal sensor (DDR3/DDR4)
    PhpHwDimmSpd5118 // SPD5118 hub thermal sensor (DDR5)
} PHP_HW_DIMM_TYPE;

typedef struct _PHP_HW_DIMM
{
    PHP_HW_DIMM_TYPE Type;
    PHP_HW_ITEM Item;
    BOOLEAN Pmic; // the PMIC answered the probe; its telemetry is read with the temperature
    PHP_HW_ITEM PmicItem;
} PHP_HW_DIMM, *PPHP_HW_DIMM;

// The SPD of a slot as probed, for the report.
typedef struct _PHP_HW_SPD_REPORT
{
    BOOLEAN Ddr4Read; // DramType and Ddr4Thermal hold SPD bytes 2 and 14
    BOOLEAN HubRead; // Mr11 holds the SPD5118 register
    BOOLEAN Mr18Read;
    BOOLEAN Locked; // an SPD5118 hub left on a non-zero page; nothing of it is readable
    BOOLEAN NvmRead; // Nvm holds the NVM page 0 bytes
    UCHAR DramType;
    UCHAR Ddr4Thermal;
    UCHAR Mr11;
    UCHAR Mr18;
    UCHAR Nvm[PHP_HW_SPD_NVM_BYTES];
} PHP_HW_SPD_REPORT, *PPHP_HW_SPD_REPORT;

typedef enum _PHP_HW_DIMM_PROBE
{
    PhpHwDimmProbeFound,
    PhpHwDimmProbeNone, // no DIMM sensor, or the host is refused
    PhpHwDimmProbeRetry, // the bus or the host timed out
    PhpHwDimmProbeContended // another agent held the bus or the host
} PHP_HW_DIMM_PROBE;

// A TPMI feature instance read once per tick.
typedef struct _PHP_HW_TPMI_REGION
{
    KPH_HW_PCI_LOCATOR Locator;
    UCHAR OpIndex;
    USHORT Instance;
    PHP_HW_ITEM Item;
    ULONG FirstEntry;
    ULONG EntryCount;
    LARGE_INTEGER SampleTime;
    LARGE_INTEGER LastSampleTime;
} PHP_HW_TPMI_REGION, *PPHP_HW_TPMI_REGION;

typedef struct _PHP_HW_TPMI_ENTRY
{
    ULONG Offset; // qword, relative to the instance
    PHP_HW_TPMI_ENTRY_KIND Kind;
    ULONG PackageIndex; // ULONG_MAX when not mapped to a package
    ULONG NamedIndex; // ULONG_MAX when the value feeds a package sensor
    FLOAT EnergyUnit; // joules
    PHP_HW_ITEM Item;
    BOOLEAN Valid;
    BOOLEAN LastValid;
    ULONG64 Value;
    ULONG64 LastValue;
} PHP_HW_TPMI_ENTRY, *PPHP_HW_TPMI_ENTRY;

// An SMU telemetry source of a package: the PM table or the HSMP mailbox.
typedef struct _PHP_HW_SMU_SOURCE
{
    BOOLEAN Disabled; // for the session
    UCHAR Strikes; // consecutive transient failures
} PHP_HW_SMU_SOURCE, *PPHP_HW_SMU_SOURCE;

typedef struct _PHP_HW_SMU_PACKAGE
{
    BOOLEAN HasRoot;
    KPH_HW_PCI_LOCATOR Root;
    ULONG CoreCount;
    PHP_HW_SMU_SOURCE Table;
    PHP_HW_SMU_SOURCE Hsmp;
    UCHAR HsmpOpIndex; // UCHAR_MAX until found
    UCHAR HsmpProbeOpIndex; // the next mailbox op to probe
    BOOLEAN HsmpAcpi; // HSMP uses the MMIO mailbox of AcpiDevice instead of the SMN pair of Root
    PPH_BYTES AcpiDevice; // the namespace path of the AMDI0097 device whose _UID is the package
    ULONG HsmpCommandDisabled; // bit per PhHwSmuHsmpCommands entry
    UCHAR HsmpCommandFailures[PH_HW_HSMP_MAX_COMMANDS];
    PH_HW_SMU_CORE_SLOTS CoreSlots;
    BOOLEAN FirmwareQueried;
    UCHAR FirmwareTries;
    ULONG FirmwareVersions[PHP_HW_SMU_MAX_MAILBOXES]; // per mailbox op of Root, for the report; 0 when unknown
} PHP_HW_SMU_PACKAGE, *PPHP_HW_SMU_PACKAGE;

typedef enum _PHP_HW_ACPI_KIND
{
    PhpHwAcpiFan, // PNP0C0B: _FST speed
    PhpHwAcpiDptfTemperature, // Intel DPTF generic participant: _TMP
    PhpHwAcpiPowerMeter, // ACPI000D: _PMM
    PhpHwAcpiDptfPower, // Intel DPTF power (PTYP 11h) or battery (0Ch) participant: one entry per getter (PMAX, PBSS, PROP)
    PhpHwAcpiAtk, // ASUS ATK0110: expanded into PhpHwAcpiAtkReading entries at session start
    PhpHwAcpiAtkReading, // one ATK0110 sensor: RTMP, RVLT or RFAN of an id
    PhpHwAcpiDytc // Lenovo ThinkPad HKEY: DYTC read once for the report
} PHP_HW_ACPI_KIND;

// An ACPI device the fan worker evaluates periodically.
typedef struct _PHP_HW_FAN
{
    PPH_BYTES Path; // the namespace path of the device
    PHP_HW_ACPI_KIND Kind;
    UCHAR Op; // DPTF power and ATK0110: the method of this entry
    UCHAR ArgCount; // ATK0110 readers and DYTC take one argument
    ULONG Arg; // ATK0110: the element id; the row adds the class and type
    PHP_HW_ITEM Item; // RetryCount counts consecutive implausible readings
    UCHAR Strikes; // consecutive timed-out evaluations
    UCHAR BusyCount; // consecutive busy evaluations
    BOOLEAN Answered; // a well-formed reading was read; the sensor is listed from then on
    BOOLEAN Probed; // the static methods of the device were read
    BOOLEAN FineGrain; // fan: _FIF fine-grain control, so _FST.Control is a percentage
    ULONG IntervalMs; // at least this long between readings, never below PHP_HW_FAN_INTERVAL_MS
    ULONG MeterIntervalMs; // power meter: _GAI, applied from the first reading on
    LARGE_INTEGER LastEvaluation; // performance counter at the last request
} PHP_HW_FAN, *PPHP_HW_FAN;

// An _FPS fan performance state: Control, TripPoint, Speed (RPM), NoiseLevel (0.1 dBA), Power (mW).
typedef struct _PHP_HW_FAN_STATE
{
    ULONG Values[5];
} PHP_HW_FAN_STATE, *PPHP_HW_FAN_STATE;

// An ACPI device of the fan session, for the report.
typedef struct _PHP_HW_ACPI_REPORT
{
    CHAR Path[64];
    PHP_HW_ACPI_KIND Kind;
    ULONG Type; // DPTF PTYP; ULONG_MAX until read
    ULONG Trips[PHP_HW_DPTF_TRIPS]; // DPTF _PSV, _CRT, _AC0, GTSH in tenths K; ULONG_MAX when absent
    ULONG FifCount; // fan _FIF integers read
    ULONG Fif[4]; // Revision, FineGrainControl, StepSize, LowSpeedNotificationSupport
    ULONG StateCount;
    PHP_HW_FAN_STATE States[PHP_HW_FAN_MAX_STATES];
    ULONG MeterCount; // power meter _PMC integers read
    ULONG Meter[PHP_HW_METER_PMC_INTEGERS]; // _PMC: capabilities, unit, type, accuracy, sampling ms, min/max averaging ms, hysteresis, hardware limit, min/max cap
    CHAR MeterStrings[PHP_HW_METER_PMC_ELEMENTS - PHP_HW_METER_PMC_INTEGERS][64]; // _PMC model, serial, OEM
    ULONG AveragingMs; // power meter _GAI, ULONG_MAX when absent
    ULONG Power[PHP_HW_POWER_STATICS]; // DPTF power PSRC, ARTG mW, CTYP; battery RBHF mOhm, VBNL mV, CMPP mA; ULONG_MAX when absent
    UCHAR Op; // DPTF power and ATK0110: the method of the entry
    ULONG Arg; // ATK0110: the element id
    ULONG Dytc[PHP_HW_DYTC_COMMANDS]; // DYTC query, get, function capabilities, MMC get; ULONG_MAX when not read
} PHP_HW_ACPI_REPORT, *PPHP_HW_ACPI_REPORT;

typedef struct _PHP_HW_WINDOW
{
    KPH_HW_PCI_LOCATOR Locator;
    PHP_HW_ITEM Item; // the whole window
    PHP_HW_ITEM Registers[PhpHwWinMaximum];
    ULONG64 Valid; // PHP_HW_WINDOW_REG bits
    ULONG64 LastValid;
    ULONG64 Value[PhpHwWinMaximum];
    ULONG64 LastValue[PhpHwWinMaximum];
    LARGE_INTEGER SampleTime;
    LARGE_INTEGER LastSampleTime;
} PHP_HW_WINDOW, *PPHP_HW_WINDOW;

PH_CIRCULAR_BUFFER_FLOAT PhHwCpuTemperatureHistory;
PH_CIRCULAR_BUFFER_FLOAT PhHwCpuPowerHistory;
FLOAT PhHwCpuTemperature = 0.0f;
FLOAT PhHwCpuPower = 0.0f;

static BOOLEAN PhpHwAvailable = FALSE;
static ULONG PhpHwCapabilities = 0;
static ULONG PhpHwCoreReadoutMask = 0x7F; // IA32_THERM_STATUS readout width
static ULONG PhpHwPackageReadoutMask = 0x7F; // IA32_PACKAGE_THERM_STATUS readout width
static BOOLEAN PhpHwGateOpen = FALSE;
static BOOLEAN PhpHwProbeAttempted = FALSE;
static ULONG PhpHwProbeRetryTicks = 0;
static BOOLEAN PhpHwNoDataTraced = FALSE;
static KPH_HW_CAPABILITIES PhpHwProbeCapabilities;

static PH_CPU_IDENTITY PhpHwIdentity;
static BOOLEAN PhpHwIntel = FALSE;
static PHP_HW_REGISTER PhpHwRegisters[PhpHwRegMaximum];
static BOOLEAN PhpHwVoltageSupported = FALSE;
static FLOAT PhpHwTscMhz = 0.0f;

// LPIT S0ix residency, provider state.
static ULONG PhpHwLpitNamedIndex = ULONG_MAX; // the named sensor, ULONG_MAX when not listed
static DOUBLE PhpHwLpitFrequency = 0.0; // counter Hz
static BOOLEAN PhpHwLpitSampled = FALSE;
static ULONG PhpHwLpitCount = 0;
static LARGE_INTEGER PhpHwLpitTime = { 0 };
static UCHAR PhpHwLpitFailures = 0; // consecutive failed reads
static NTSTATUS PhpHwLpitStatus = STATUS_NOT_SUPPORTED; // the last read, for the report

// PMC PWRM PCH temperature, provider state.
static UCHAR PhpHwPwrmWindow = 0; // the platform window op, 0 when not read
static UCHAR PhpHwPwrmFailures = 0;
static NTSTATUS PhpHwPwrmStatus[PHP_HW_PWRM_WINDOW_COUNT] = { STATUS_NOT_SUPPORTED, STATUS_NOT_SUPPORTED }; // the probe read of each op, for the report
static ULONG PhpHwPwrmMinMax[2]; // MINTEMP, MAXTEMP at the probe

// ChromeOS EC memory map (Framework NPCX boards), provider state.
static ULONG PhpHwCrosEcNamedIndex[PHP_HW_CROS_EC_SENSORS]; // ULONG_MAX when not listed
static UCHAR PhpHwCrosEcThermalVersion = 0;
static UCHAR PhpHwCrosEcFailures = 0;
static NTSTATUS PhpHwCrosEcStatus = STATUS_NOT_SUPPORTED; // the last read, for the report

static ULONG PhpHwCrosEcCapabilities(
    VOID
    );

static VOID PhpHwSetNamedSensor(
    _In_ ULONG Index,
    _In_ BOOLEAN Valid,
    _In_ FLOAT Value
    );

static FLOAT PhpHwEnergyUnit = 0.0f;
static FLOAT PhpHwDramEnergyUnit = 0.0f;
static FLOAT PhpHwPlatformEnergyUnit = 0.0f;
static FLOAT PhpHwTctlOffset = 0.0f;
static ULONG PhpHwCcdBase = 0;
static ULONG PhpHwCcdCount = 0;
static ULONG PhpHwUmcChannelCount = 0;
static ULONG PhpHwMemoryClockTicks = 0;

static ULONG PhpHwProcessorCount = 0;
static PPH_HW_CPU_SENSOR PhpHwCpuSensors = NULL;
static PPHP_HW_SAMPLE PhpHwProcessorSamples = NULL;
static PUSHORT PhpHwGroupBase = NULL;
static ULONG PhpHwCoreCount = 0;
static PPHP_HW_CORE PhpHwCores = NULL;
static ULONG PhpHwPackageCount = 0;
static PPH_HW_PACKAGE_SENSOR PhpHwPackageSensors = NULL;
static PPHP_HW_PACKAGE PhpHwPackages = NULL;

static ULONG PhpHwBatchCapacity = 0;
static PKPH_HW_MSR_BATCH_ENTRY PhpHwBatch = NULL; // on ARM64 Register is the KPH_DYN_HW_SYSREG_*
static PPHP_HW_BATCH_TAG PhpHwBatchTags = NULL;

static BOOLEAN PhpHwArm64 = FALSE; // the plan is the ARM64 system registers (sysreg_read)
static PKPH_HW_SYSREG_BATCH_ENTRY PhpHwSysregBatch = NULL; // PHP_HW_REGISTER_BATCH entries
static ULONG64 PhpHwArmCounterHz = 0; // CNTFRQ_EL0, 0 when unknown
static PULONG PhpHwArmMidr = NULL; // per processor, 0 until the driver reports it
static PUCHAR PhpHwArmStalls = NULL; // per processor, consecutive ticks a cycle counter did not advance
static PULONG PhpHwArmDisabled = NULL; // per processor, bit n: PHP_HW_REG n is not read on the processor this session

static LARGE_INTEGER PhpHwSampleTime = { 0 };
static LARGE_INTEGER PhpHwLastSampleTime = { 0 };
static LARGE_INTEGER PhpHwFrequency = { 0 };

static BOOLEAN PhpHwHistoryInitialized = FALSE;

static BOOLEAN PhpHwRootsEnumerated = FALSE;
static BOOLEAN PhpHwSmnPending = FALSE;
static UCHAR PhpHwPciTimeouts = 0; // consecutive timed-out waits for Access_PCI

static BOOLEAN PhpHwWindowsEnumerated = FALSE;
static ULONG PhpHwWindowLocatorCount = 0;
static KPH_HW_PCI_LOCATOR PhpHwWindowLocators[PHP_HW_MAX_WINDOWS];
static BOOLEAN PhpHwWindowHostBridge = FALSE; // PhpHwWindowLocators[0] is the host bridge
static ULONG PhpHwWindowCount = 0;
static PHP_HW_WINDOW PhpHwWindows[PHP_HW_MAX_WINDOWS];
static FLOAT PhpHwPowerUnit = 0.0f; // MSR_RAPL_POWER_UNIT watts, 0 when unavailable
static FLOAT PhpHwMmioPowerUnit = 0.0f; // MMIO package power unit watts, 0 when unavailable
static BOOLEAN PhpHwChipsetTemperatureValid = FALSE;
static ULONG PhpHwGtNamedIndex = ULONG_MAX; // the MCHBAR graphics clock sensor, ULONG_MAX when not listed
static ULONG PhpHwGtCaps = 0; // RP_STATE_CAP at probe, for the report
static ULONG PhpHwGtRpe = 0; // FREQ_INFO_REC at probe, for the report
static ULONG PhpHwGtPerfStatusLast = 0; // GT_PERF_STATUS, for the report
static ULONG PhpHwGtPerfStatusPeak = 0; // the GT_PERF_STATUS with the highest decoded clock, for the report
static ULONG PhpHwRaplLimitedNamedIndex = ULONG_MAX; // the MMIO RAPL limited residency sensor, ULONG_MAX when not listed

// MEMSS_PMA_CR_MEM_CONFIG and the MAD registers of each IMC at probe (Meteor Lake and later), for the report.
typedef struct _PHP_HW_INTEL_MEMORY_CONFIG
{
    ULONG Valid; // bit 0 MemConfig, bit 1 + 3 * IMC + i Mad[IMC][i]
    ULONG MemConfig;
    ULONG Mad[2][3]; // MAD_INTER_CHANNEL, MAD_DIMM_CH0, MAD_DIMM_CH1
} PHP_HW_INTEL_MEMORY_CONFIG;

static PHP_HW_INTEL_MEMORY_CONFIG PhpHwMemoryConfig = { 0 };

// RAPL DRAM and platform (PSYS) energy: some parts and boards list the MSR but never count; the register is dropped
// when no package's counter has moved after PHP_HW_ENERGY_IDLE_TICKS samples.
#define PHP_HW_ENERGY_IDLE_TICKS 5
static FLOAT PhpHwChipsetTemperature = 0.0f;
static ULONG PhpHwSmbiosDataRate = 0; // MT/s, 0 when unknown or the SMBIOS version is before 2.8
static FLOAT PhpHwDecodedDataRate = 0.0f; // MT/s of the last MCHBAR decode, for the report
static BOOLEAN PhpHwDataRateRejected = FALSE; // the last decode disagreed with SMBIOS

// The named sensor lock guards the TPMI and MSR named sensors (written by the provider), the DIMM sensors
// (written by the SMBus worker), the SMU sensors (written by the SMU worker) and the session requests.
static PH_QUEUED_LOCK PhpHwNamedSensorLock = PH_QUEUED_LOCK_INIT;
static ULONG PhpHwNamedSensorCount = 0;
static PH_HW_NAMED_SENSOR PhpHwNamedSensors[PHP_HW_MAX_NAMED_SENSORS];
static PH_HW_NAMED_SENSOR PhpHwDimmSensors[PHP_HW_DIMM_COUNT]; // by slot; Active when listed
static PH_HW_NAMED_SENSOR PhpHwDimmVoltageSensors[PHP_HW_DIMM_COUNT * PH_HW_PMIC_RAILS]; // by Id; Active when listed
static PH_HW_NAMED_SENSOR PhpHwDimmPmicSensors[PHP_HW_DIMM_COUNT * PHP_HW_PMIC_SENSORS]; // slot * PHP_HW_PMIC_SENSORS + PHP_HW_PMIC_SENSOR_*
static BOOLEAN PhpHwSmbusRequested = FALSE;
static ULONG PhpHwSmbusRequestId = 0; // a new id makes the worker probe again
static HANDLE PhpHwSmbusEvent = NULL;

// SMBus worker state.
static BOOLEAN PhpHwSmbusEnumerated = FALSE;
static ULONG PhpHwSmbusLocatorCount = 0;
static KPH_HW_PCI_LOCATOR PhpHwSmbusLocators[PHP_HW_MAX_SMBUS_HOSTS];
static ULONG PhpHwSmbusHost = ULONG_MAX; // the locator whose DIMMs are read, ULONG_MAX when none
static ULONG PhpHwSmbusProbeHost = ULONG_MAX; // the next locator to probe, ULONG_MAX when done
static ULONG PhpHwSmbusStrikes = 0;
static UCHAR PhpHwSmbusTimedOutSpd = 0; // slots whose SPD address timed out
static UCHAR PhpHwSmbusTimedOutTsod = 0; // slots whose TSOD address timed out
static UCHAR PhpHwSmbusTimedOutPmic = 0; // slots whose PMIC address timed out
static PHP_HW_DIMM PhpHwDimms[PHP_HW_DIMM_COUNT];
static ULONG PhpHwDimmVoltageMask = 0; // bit n set when PhpHwDimmVoltages[n / PH_HW_PMIC_RAILS][n % PH_HW_PMIC_RAILS] holds a set point
static FLOAT PhpHwDimmVoltages[PHP_HW_DIMM_COUNT][PH_HW_PMIC_RAILS];
C_ASSERT(PHP_HW_DIMM_COUNT * PH_HW_PMIC_RAILS <= sizeof(ULONG) * 8);

// A listed PMT sample, provider state.
typedef struct _PHP_HW_PMT_ENTRY
{
    ULONG LocatorIndex;
    UCHAR OpIndex;
    USHORT Instance;
    ULONG Sample; // index in PhpHwPmtSamples
    ULONG NamedIndex;
    PHP_HW_ITEM Item;
} PHP_HW_PMT_ENTRY, *PPHP_HW_PMT_ENTRY;

static BOOLEAN PhpHwPmtEnumerated = FALSE;
static ULONG PhpHwPmtLocatorCount = 0;
static KPH_HW_PCI_LOCATOR PhpHwPmtLocators[PHP_HW_MAX_PMT_DEVICES];
static ULONG PhpHwPmtEntryCount = 0;
static PHP_HW_PMT_ENTRY PhpHwPmtEntries[PHP_HW_MAX_PMT_ENTRIES];

// A TPMI SST or PLR instance read at probe for the report.
typedef struct _PHP_HW_TPMI_REPORT
{
    UCHAR Feature; // PH_INTEL_TPMI_ID_SST, PH_INTEL_TPMI_ID_PLR
    UCHAR Version; // SST interface version
    UCHAR Level; // SST-PP current level
    BOOLEAN Locked;
    USHORT Instance;
    ULONG PackageIndex; // ULONG_MAX when not mapped to a package
    ULONG64 Value; // SST: level INFO_1 (TDP, TjMax); PLR: die-level reasons
    ULONG64 Ratios; // SST: level INFO_11
} PHP_HW_TPMI_REPORT, *PPHP_HW_TPMI_REPORT;

#define PHP_HW_MAX_TPMI_REPORTS PHP_HW_MAX_TPMI_REGIONS

static ULONG PhpHwTpmiReportCount = 0;
static ULONG PhpHwTpmiReportsOmitted = 0;
static PHP_HW_TPMI_REPORT PhpHwTpmiReports[PHP_HW_MAX_TPMI_REPORTS];

static BOOLEAN PhpHwTpmiEnumerated = FALSE;
static ULONG PhpHwTpmiLocatorCount = 0;
static KPH_HW_PCI_LOCATOR PhpHwTpmiLocators[PHP_HW_MAX_VSEC_DEVICES];
static ULONG PhpHwTpmiRegionCount = 0;
static PHP_HW_TPMI_REGION PhpHwTpmiRegions[PHP_HW_MAX_TPMI_REGIONS];
static ULONG PhpHwTpmiEntryCount = 0;
static PHP_HW_TPMI_ENTRY PhpHwTpmiEntries[PHP_HW_MAX_TPMI_ENTRIES];

// SMU session request, written by the provider under the named sensor lock.
static BOOLEAN PhpHwSmuRequested = FALSE;
static ULONG PhpHwSmuRequestId = 0; // a new id makes the worker start over
static ULONG PhpHwSmuRequestPackageCount = 0;
static BOOLEAN PhpHwSmuRequestHasRoot[PHP_HW_SMU_MAX_PACKAGES];
static KPH_HW_PCI_LOCATOR PhpHwSmuRequestRoots[PHP_HW_SMU_MAX_PACKAGES];
static ULONG PhpHwSmuRequestCoreCounts[PHP_HW_SMU_MAX_PACKAGES];
static BOOLEAN PhpHwSmuRequestAcpi = FALSE; // the driver can reach ACPI devices
static HANDLE PhpHwSmuEvent = NULL;

// SMU sensors, written by the SMU worker under the named sensor lock; cleared when a session ends.
static ULONG PhpHwSmuSensorCount = 0;
static PH_HW_NAMED_SENSOR PhpHwSmuSensors[PHP_HW_SMU_MAX_SENSORS];
static UCHAR PhpHwSmuSensorOrigins[PHP_HW_SMU_MAX_SENSORS]; // PHP_HW_SMU_ORIGIN_*

// The last PM table read of each package for the report, written by the SMU worker under the named sensor lock.
typedef struct _PHP_HW_SMU_REPORT
{
    BOOLEAN Read;
    BOOLEAN Decoded;
    NTSTATUS Status;
    ULONG Version;
    ULONG Length; // bytes in Table
    ULONG Capacity;
    PVOID Table;
    ULONG FirmwareVersions[PHP_HW_SMU_MAX_MAILBOXES];
    BOOLEAN LimitSourcesValid;
    ULONG LimitSources; // the last HSMP 19h limit sources, PH_HW_HSMP_LIMIT_*
} PHP_HW_SMU_REPORT, *PPHP_HW_SMU_REPORT;

static PHP_HW_SMU_REPORT PhpHwSmuReports[PHP_HW_SMU_MAX_PACKAGES];

// The DIMM probe for the report, written by the SMBus worker under the named sensor lock.
// PMIC5100 registers read by the probe; 10h-19h are write-1-to-clear, 37h-39h the vendor password.
static CONST UCHAR PhpHwPmicCommands[] =
{
    0x04, 0x05, 0x06, // error log of the previous power cycle
    0x08, 0x09, 0x0A, 0x0B, 0x0C, 0x0D, 0x0E, 0x0F, // status, SWA/SWB/SWC current or power
    0x1A, 0x1B, // power mode, telemetry mode
    0x20, 0x21, 0x22, 0x23, 0x24, 0x25, 0x26, 0x27, 0x28, 0x29, 0x2A, 0x2B, 0x2C, 0x2D, 0x2E, 0x2F, // limits, set points, LDO, protection
    0x30, 0x31, 0x32, 0x33, // ADC select and value, VR enable and I3C, temperature
    0x3B, 0x3C, 0x3D, // revision, vendor
};

#define PHP_HW_PMIC_REPORT_REGISTERS RTL_NUMBER_OF(PhpHwPmicCommands)

static ULONG PhpHwDimmReportHostCount = 0;
static ULONG PhpHwDimmReportHost = ULONG_MAX;
static UCHAR PhpHwDimmReportTypes[PHP_HW_DIMM_COUNT]; // PHP_HW_DIMM_TYPE
static UCHAR PhpHwPmicReportMask = 0; // slots whose PMIC was read
static NTSTATUS PhpHwPmicReportStatus[PHP_HW_DIMM_COUNT];
static UCHAR PhpHwPmicReportRegisters[PHP_HW_DIMM_COUNT][PHP_HW_PMIC_REPORT_REGISTERS];
static PHP_HW_SPD_REPORT PhpHwSpdReports[PHP_HW_DIMM_COUNT];
static ULONG PhpHwSpdReportHost = ULONG_MAX; // the host PhpHwSpdReports came from
static BOOLEAN PhpHwSmuMemoryClockValid[PHP_HW_SMU_MAX_PACKAGES];
static FLOAT PhpHwSmuMemoryClock[PHP_HW_SMU_MAX_PACKAGES]; // DDR MEMCLK, MHz

// SMU worker state.
static ULONG PhpHwSmuPackageCount = 0;
static PHP_HW_SMU_PACKAGE PhpHwSmuPackages[PHP_HW_SMU_MAX_PACKAGES];
static PVOID PhpHwSmuTableBuffer = NULL; // KPH_DYN_HW_SMU_TABLE_MAX_SIZE bytes
static PPH_HW_SMU_DECODE PhpHwSmuDecode = NULL;
static BOOLEAN PhpHwSmuAcpiAvailable = FALSE;
static BOOLEAN PhpHwSmuAcpiEnumerated = FALSE; // the AMDI0097 devices of the session were looked for

// ACPI fan session request, written by the provider under the named sensor lock.
static BOOLEAN PhpHwFanRequested = FALSE;
static ULONG PhpHwFanRequestId = 0; // a new id makes the worker find the fans again
static HANDLE PhpHwFanEvent = NULL;
static BOOLEAN PhpHwFansOnly = FALSE; // the probe found nothing but the ACPI devices of the fan worker or the board EC; the processor is not read

// ACPI fan and DPTF sensors and their report, written by the fan worker under the named sensor lock; cleared when a session ends.
static ULONG PhpHwFanSensorCount = 0;
static PH_HW_NAMED_SENSOR PhpHwFanSensors[PHP_HW_MAX_FANS];
static PH_HW_NAMED_SENSOR PhpHwFanControlSensors[PHP_HW_MAX_FANS]; // by device, PhpHwFanSensorCount entries; fine-grain fans only
static PHP_HW_ACPI_REPORT PhpHwAcpiReports[PHP_HW_MAX_FANS];

// Fan worker state.
static ULONG PhpHwFanCount = 0;
static PHP_HW_FAN PhpHwFans[PHP_HW_MAX_FANS];
static LARGE_INTEGER PhpHwFanLastEvaluation = { 0 }; // the last ACPI request of the fan worker, of any session

#define PHP_HW_DIAGNOSTICS_MAX_PACKAGES 16 // packages past this are left out of the report

typedef struct _PHP_HW_DIAGNOSTICS_PACKAGE
{
    BOOLEAN HasRoot;
    BOOLEAN TctlOff;
    BOOLEAN MemoryClockOff;
    BOOLEAN MemoryTimingsValid;
    KPH_HW_PCI_LOCATOR Root;
    ULONG CcdMask;
    ULONG MemoryChannel;
} PHP_HW_DIAGNOSTICS_PACKAGE, *PPHP_HW_DIAGNOSTICS_PACKAGE;

// Provider state for the hardware report, published under the named sensor lock after each update.
typedef struct _PHP_HW_DIAGNOSTICS
{
    BOOLEAN GateOpen;
    BOOLEAN ProbeAttempted;
    BOOLEAN Available;
    BOOLEAN FansOnly;
    ULONG Capabilities;
    FLOAT TscMhz;
    ULONG SmbiosDataRate;
    FLOAT DecodedDataRate;
    BOOLEAN DataRateRejected;
    BOOLEAN LpitListed;
    ULONG PmtLocatorCount;
    ULONG PmtEntryCount;
    BOOLEAN GtListed;
    ULONG GtCaps; // RP_STATE_CAP
    ULONG GtRpe; // FREQ_INFO_REC
    ULONG GtPerfStatusLast;
    ULONG GtPerfStatusPeak;
    PHP_HW_INTEL_MEMORY_CONFIG MemoryConfig;
    ULONG RankTemperatureValid; // RANK_TEMPERATURE bits, MC0 ch0 first
    ULONG RankTemperature[4];
    ULONG RequestsValid; // PWM_TOTAL_REQCOUNT bits by IMC
    ULONG64 Requests[2];
    UCHAR CrosEcThermalVersion;
    NTSTATUS CrosEcStatus; // the last EC read; STATUS_NOT_SUPPORTED when not probed
    NTSTATUS LpitStatus; // the last LPIT counter read; STATUS_NOT_SUPPORTED when not probed
    DOUBLE LpitFrequency;
    UCHAR PwrmWindow;
    NTSTATUS PwrmStatus[PHP_HW_PWRM_WINDOW_COUNT]; // STATUS_NOT_SUPPORTED when not tried
    ULONG PwrmMinMax[2];
    KPH_HW_CAPABILITIES Driver;
    ULONG Registers[PhpHwRegMaximum];
    ULONG RegistersOff[(PhpHwRegMaximum + 31) / 32];
    ULONG WindowCount;
    ULONG WindowLocatorCount;
    ULONG TpmiRegionCount;
    ULONG TpmiEntryCount;
    ULONG TpmiReportCount;
    ULONG TpmiReportsOmitted;
    PHP_HW_TPMI_REPORT TpmiReports[PHP_HW_MAX_TPMI_REPORTS];
    ULONG PackageCount;
    PHP_HW_DIAGNOSTICS_PACKAGE Packages[PHP_HW_DIAGNOSTICS_MAX_PACKAGES];
} PHP_HW_DIAGNOSTICS, *PPHP_HW_DIAGNOSTICS;

static PHP_HW_DIAGNOSTICS PhpHwDiagnostics = { 0 };

// The update lock serializes source callbacks with registration; the data lock guards the source
// list and the reported sensors. The list changes under both.
static PH_QUEUED_LOCK PhpHwSourceUpdateLock = PH_QUEUED_LOCK_INIT;
static PH_QUEUED_LOCK PhpHwSourceDataLock = PH_QUEUED_LOCK_INIT;
static LIST_ENTRY PhpHwSourceListHead = { &PhpHwSourceListHead, &PhpHwSourceListHead };
static ULONG PhpHwSourceNextId = 1;
static ULONG PhpHwSourceUpdateId = 0;
static ULONG PhpHwSourceCompletedUpdateId = 0; // the last update whose source loop finished
static HANDLE PhpHwSourceUpdateThread = NULL; // the thread in the source loop, for assertions

/**
 * Maps a processor to the PhCpusKernelUsage flat index; ULONG_MAX when it is not active.
 */
static ULONG PhpHwFlatProcessorIndex(
    _In_ USHORT Group,
    _In_ ULONG Number
    )
{
    if (Group >= PhSystemProcessorInformation.NumberOfProcessorGroups)
        return ULONG_MAX;
    if (Number >= PhGetActiveProcessorCount(Group))
        return ULONG_MAX;

    return PhpHwGroupBase[Group] + Number;
}

/**
 * Assigns each processor to a core or package relationship by flat index.
 *
 * \param FirstProcessor Receives the lowest flat index of each relationship.
 */
_Success_(return)
static BOOLEAN PhpHwMapRelationship(
    _In_ LOGICAL_PROCESSOR_RELATIONSHIP Relationship,
    _Out_ PULONG Count,
    _Out_ PULONG* FirstProcessor
    )
{
    PSYSTEM_LOGICAL_PROCESSOR_INFORMATION_EX information;
    PSYSTEM_LOGICAL_PROCESSOR_INFORMATION_EX entry;
    ULONG informationLength;
    ULONG count;
    PULONG firstProcessor;

    if (!NT_SUCCESS(PhGetSystemLogicalProcessorInformation(Relationship, &information, &informationLength)))
        return FALSE;

    count = 0;

    for (
        entry = information;
        (ULONG_PTR)entry < (ULONG_PTR)PTR_ADD_OFFSET(information, informationLength);
        entry = PTR_ADD_OFFSET(entry, entry->Size)
        )
    {
        if (entry->Relationship == Relationship)
            count++;
    }

    if (count == 0)
    {
        PhFree(information);
        return FALSE;
    }

    firstProcessor = PhAllocate(sizeof(ULONG) * count);
    count = 0;

    for (
        entry = information;
        (ULONG_PTR)entry < (ULONG_PTR)PTR_ADD_OFFSET(information, informationLength);
        entry = PTR_ADD_OFFSET(entry, entry->Size)
        )
    {
        if (entry->Relationship != Relationship)
            continue;

        firstProcessor[count] = ULONG_MAX;

        for (USHORT i = 0; i < entry->Processor.GroupCount; i++)
        {
            PGROUP_AFFINITY groupMask = &entry->Processor.GroupMask[i];

            for (ULONG number = 0; number < sizeof(KAFFINITY) * 8; number++)
            {
                ULONG index;

                if (!(groupMask->Mask & AFFINITY_MASK(number)))
                    continue;

                index = PhpHwFlatProcessorIndex(groupMask->Group, number);

                if (index == ULONG_MAX)
                    continue;

                if (Relationship == RelationProcessorCore)
                    PhpHwCpuSensors[index].CoreIndex = count;
                else
                    PhpHwCpuSensors[index].PackageIndex = count;

                if (index < firstProcessor[count])
                    firstProcessor[count] = index;
            }
        }

        if (firstProcessor[count] == ULONG_MAX)
        {
            PhFree(firstProcessor);
            PhFree(information);
            return FALSE;
        }

        count++;
    }

    PhFree(information);

    *Count = count;
    *FirstProcessor = firstProcessor;
    return TRUE;
}

/**
 * Builds the processor, core and package maps and allocates the sensor arrays.
 */
static BOOLEAN PhpHwInitializeTopology(
    VOID
    )
{
    PULONG coreFirst = NULL;
    PULONG packageFirst = NULL;
    ULONG coreCount;
    ULONG packageCount;
    USHORT base;
    ULONG index;
    BOOLEAN arm64 = FALSE;

#if defined(_M_ARM64)
    arm64 = TRUE;
#else
    if (!PhGetCpuIdentity(&PhpHwIdentity))
        return FALSE;

    if (PhCpuIdentityIsIntel(&PhpHwIdentity))
    {
        PhpHwIntel = TRUE;
    }
    else if (PhCpuIdentityIsAmd(&PhpHwIdentity))
    {
        if (PhpHwIdentity.DisplayFamily < 0x17)
            return FALSE;
    }
    else
    {
        return FALSE;
    }
#endif

    PhpHwProcessorCount = PhSystemProcessorInformation.NumberOfProcessors;
    PhpHwGroupBase = PhAllocate(sizeof(USHORT) * PhSystemProcessorInformation.NumberOfProcessorGroups);
    base = 0;

    for (USHORT group = 0; group < PhSystemProcessorInformation.NumberOfProcessorGroups; group++)
    {
        PhpHwGroupBase[group] = base;
        base += PhGetActiveProcessorCount(group);
    }

    PhpHwCpuSensors = PhAllocateZero(sizeof(PH_HW_CPU_SENSOR) * PhpHwProcessorCount);
    PhpHwProcessorSamples = PhAllocateZero(sizeof(PHP_HW_SAMPLE) * PhpHwProcessorCount);
    index = 0;

    for (USHORT group = 0; group < PhSystemProcessorInformation.NumberOfProcessorGroups; group++)
    {
        USHORT activeCount = PhGetActiveProcessorCount(group);

        for (USHORT number = 0; number < activeCount && index < PhpHwProcessorCount; number++)
        {
            PhpHwCpuSensors[index].ProcessorNumber.Group = group;
            PhpHwCpuSensors[index].ProcessorNumber.Number = (UCHAR)number;
            PhpHwCpuSensors[index].CoreIndex = ULONG_MAX;
            PhpHwCpuSensors[index].PackageIndex = ULONG_MAX;
            index++;
        }
    }

    if (
        index != PhpHwProcessorCount ||
        !PhpHwMapRelationship(RelationProcessorCore, &coreCount, &coreFirst) ||
        !PhpHwMapRelationship(RelationProcessorPackage, &packageCount, &packageFirst)
        )
    {
        goto CleanupExit;
    }

    for (ULONG i = 0; i < PhpHwProcessorCount; i++)
    {
        if (PhpHwCpuSensors[i].CoreIndex == ULONG_MAX || PhpHwCpuSensors[i].PackageIndex == ULONG_MAX)
            goto CleanupExit;
    }

    PhpHwCoreCount = coreCount;
    PhpHwCores = PhAllocateZero(sizeof(PHP_HW_CORE) * coreCount);
    PhpHwPackageCount = packageCount;
    PhpHwPackages = PhAllocateZero(sizeof(PHP_HW_PACKAGE) * packageCount);
    PhpHwPackageSensors = PhAllocateZero(sizeof(PH_HW_PACKAGE_SENSOR) * packageCount);

    for (ULONG i = 0; i < packageCount; i++)
    {
        PhpHwPackages[i].UncoreNamedIndex = ULONG_MAX;
        memset(PhpHwPackages[i].LimitNamedIndex, 0xFF, sizeof(PhpHwPackages[i].LimitNamedIndex));
        PhpHwPackages[i].FirstProcessor = packageFirst[i];
    }

    for (ULONG i = 0; i < coreCount; i++)
    {
        PhpHwCores[i].FirstProcessor = coreFirst[i];
        PhpHwCores[i].PackageIndex = PhpHwCpuSensors[coreFirst[i]].PackageIndex;
        PhpHwPackages[PhpHwCores[i].PackageIndex].CoreCount++;
    }

    // Per-tick registers (3 thread, 2 core, 10 package) plus the probe-only package registers (13 with them).
    PhpHwBatchCapacity = PhpHwProcessorCount * (arm64 ? PHP_HW_ARM_PROBE_REGISTERS : 3) + coreCount * 2 + packageCount * 14 + 8;
    PhpHwBatch = PhAllocate(sizeof(KPH_HW_MSR_BATCH_ENTRY) * PhpHwBatchCapacity);
    PhpHwBatchTags = PhAllocate(sizeof(PHP_HW_BATCH_TAG) * PhpHwBatchCapacity);

    if (arm64)
    {
        PhpHwSysregBatch = PhAllocate(sizeof(KPH_HW_SYSREG_BATCH_ENTRY) * PHP_HW_REGISTER_BATCH);
        PhpHwArmMidr = PhAllocateZero(sizeof(ULONG) * PhpHwProcessorCount);
        PhpHwArmStalls = PhAllocateZero(sizeof(UCHAR) * PhpHwProcessorCount);
        PhpHwArmDisabled = PhAllocateZero(sizeof(ULONG) * PhpHwProcessorCount);
        PhpHwArm64 = TRUE;
    }

    PhQueryPerformanceFrequency(&PhpHwFrequency);

    PhFree(coreFirst);
    PhFree(packageFirst);

    PhTraceInfo(
        "Hardware sensors topology: %lu processors, %lu cores, %lu packages",
        PhpHwProcessorCount,
        PhpHwCoreCount,
        PhpHwPackageCount
        );

    return TRUE;

CleanupExit:
    if (coreFirst)
        PhFree(coreFirst);
    if (packageFirst)
        PhFree(packageFirst);

    PhFree(PhpHwProcessorSamples);
    PhpHwProcessorSamples = NULL;
    PhFree(PhpHwCpuSensors);
    PhpHwCpuSensors = NULL;
    PhFree(PhpHwGroupBase);
    PhpHwGroupBase = NULL;
    PhpHwProcessorCount = 0;

    PhTraceWarning("Hardware sensors topology unavailable");

    return FALSE;
}

/**
 * Initializes the hardware sensor provider.
 */
VOID PhHwProviderInitialization(
    VOID
    )
{
    PhpHwInitializeTopology();
}

static VOID PhpHwSetRegister(
    _In_ PHP_HW_REG Reg,
    _In_ ULONG Register,
    _In_ PHP_HW_SCOPE Scope
    )
{
    PhpHwRegisters[Reg].Register = Register;
    PhpHwRegisters[Reg].Scope = Scope;
    // The power limit is read every tick; firmware and DTT change it at runtime.
    PhpHwRegisters[Reg].ProbeOnly = (Reg >= PhpHwRegTemperatureTarget) && Reg != PhpHwRegPowerLimit;
    PhpHwRegisters[Reg].Item.Disabled = FALSE;
    PhpHwRegisters[Reg].Item.RetryCount = 0;
}

static VOID PhpHwInitializeRegisterPlan(
    VOID
    )
{
    memset(PhpHwRegisters, 0, sizeof(PhpHwRegisters));

    for (ULONG i = 0; i < PhpHwPackageCount; i++)
    {
        PhpHwPackages[i].DramIdleTicks = 0;
        PhpHwPackages[i].PlatformIdleTicks = 0;
        PhpHwPackages[i].DramEnergySeen = FALSE;
        PhpHwPackages[i].PlatformEnergySeen = FALSE;
    }

    if (PhpHwArm64)
    {
        PhpHwSetRegister(PhpHwRegArmCounter, KPH_DYN_HW_SYSREG_CNTVCT_EL0, PhpHwScopeThread);
        PhpHwSetRegister(PhpHwRegArmCycles, KPH_DYN_HW_SYSREG_AMEVCNTR00_EL0, PhpHwScopeThread);
        PhpHwSetRegister(PhpHwRegArmConstantCycles, KPH_DYN_HW_SYSREG_AMEVCNTR01_EL0, PhpHwScopeThread);
        PhpHwSetRegister(PhpHwRegArmPmuCycles, KPH_DYN_HW_SYSREG_PMCCNTR_EL0, PhpHwScopeThread);
        PhpHwSetRegister(PhpHwRegArmFrequency, KPH_DYN_HW_SYSREG_CNTFRQ_EL0, PhpHwScopeThread);
        PhpHwSetRegister(PhpHwRegArmAmuEnable, KPH_DYN_HW_SYSREG_AMCNTENSET0_EL0, PhpHwScopeThread);
    }
    else if (PhpHwIntel)
    {
        PhpHwSetRegister(PhpHwRegAperf, PH_INTEL_MSR_APERF, PhpHwScopeThread);
        PhpHwSetRegister(PhpHwRegMperf, PH_INTEL_MSR_MPERF, PhpHwScopeThread);
        PhpHwSetRegister(PhpHwRegPerfStatus, PH_INTEL_MSR_PERF_STATUS, PhpHwScopeThread);
        PhpHwSetRegister(PhpHwRegThermStatus, PH_INTEL_MSR_THERM_STATUS, PhpHwScopeCore);
        PhpHwSetRegister(PhpHwRegPackageThermStatus, PH_INTEL_MSR_PACKAGE_THERM_STATUS, PhpHwScopePackage);
        PhpHwSetRegister(PhpHwRegPackageEnergy, PH_INTEL_MSR_PKG_ENERGY_STATUS, PhpHwScopePackage);
        PhpHwSetRegister(PhpHwRegPp0Energy, PH_INTEL_MSR_PP0_ENERGY_STATUS, PhpHwScopePackage);
        PhpHwSetRegister(PhpHwRegDramEnergy, PH_INTEL_MSR_DRAM_ENERGY_STATUS, PhpHwScopePackage);
        PhpHwSetRegister(PhpHwRegPlatformEnergy, PH_INTEL_MSR_PLATFORM_ENERGY_STATUS, PhpHwScopePackage);
        PhpHwSetRegister(PhpHwRegTemperatureTarget, PH_INTEL_MSR_TEMPERATURE_TARGET, PhpHwScopePackage);
        PhpHwSetRegister(PhpHwRegPlatformInfo, PH_INTEL_MSR_PLATFORM_INFO, PhpHwScopePackage);
        PhpHwSetRegister(PhpHwRegRaplUnit, PH_INTEL_MSR_RAPL_POWER_UNIT, PhpHwScopePackage);
        PhpHwSetRegister(PhpHwRegPowerLimit, PH_INTEL_MSR_PKG_POWER_LIMIT, PhpHwScopePackage);
        PhpHwSetRegister(PhpHwRegUncorePerfStatus, PH_INTEL_MSR_UNCORE_PERF_STATUS, PhpHwScopePackage);
        PhpHwSetRegister(PhpHwRegLimitReasons, PhIntelLimitReasonsMsr(&PhpHwIdentity), PhpHwScopePackage);
        PhpHwSetRegister(PhpHwRegPlatformPowerLimit, PH_INTEL_MSR_PLATFORM_POWER_LIMIT, PhpHwScopePackage);

        if (PhIntelHasPl4(&PhpHwIdentity))
            PhpHwSetRegister(PhpHwRegVrCurrentConfig, PH_INTEL_MSR_VR_CURRENT_CONFIG, PhpHwScopePackage);

        PhpHwCoreReadoutMask = PhIntelThermReadoutMask(&PhpHwIdentity, FALSE);
        PhpHwPackageReadoutMask = PhIntelThermReadoutMask(&PhpHwIdentity, TRUE);
    }
    else
    {
        PhpHwSetRegister(PhpHwRegAperf, PH_AMD_MSR_APERF_READONLY, PhpHwScopeThread);
        PhpHwSetRegister(PhpHwRegMperf, PH_AMD_MSR_MPERF_READONLY, PhpHwScopeThread);
        PhpHwSetRegister(PhpHwRegPerfStatus, PH_AMD_MSR_HW_PSTATE_STATUS, PhpHwScopeCore);
        PhpHwSetRegister(PhpHwRegCoreEnergy, PH_AMD_MSR_CORE_ENERGY_STAT, PhpHwScopeCore);
        PhpHwSetRegister(PhpHwRegPackageEnergy, PH_AMD_MSR_PKG_ENERGY_STAT, PhpHwScopePackage);
        PhpHwSetRegister(PhpHwRegRaplUnit, PH_AMD_MSR_RAPL_PWR_UNIT, PhpHwScopePackage);
        PhpHwSetRegister(PhpHwRegPStateDef0, PH_AMD_MSR_PSTATE_DEF0, PhpHwScopePackage);
    }
}

static BOOLEAN PhpHwRegisterActive(
    _In_ PHP_HW_REG Reg
    )
{
    return PhpHwRegisters[Reg].Register && !PhpHwRegisters[Reg].Item.Disabled;
}

static VOID PhpHwDisableRegister(
    _In_ PHP_HW_REG Reg
    )
{
    PhpHwRegisters[Reg].Item.Disabled = TRUE;
}

/**
 * Applies the session failure policy: a missing or refused resource ends the item at once,
 * contention (STATUS_RETRY) is skipped, any other failure counts a strike.
 *
 * \return TRUE when the value of the request is usable.
 */
static BOOLEAN PhpHwApplyStatus(
    _Inout_ PPHP_HW_ITEM Item,
    _In_ ULONG Id,
    _In_ NTSTATUS Status
    )
{
    if (NT_SUCCESS(Status))
    {
        Item->RetryCount = 0;
        return TRUE;
    }

    switch (Status)
    {
    case STATUS_RETRY:
        break;
    case STATUS_NOINTERFACE:
    case STATUS_NOT_SUPPORTED:
    case STATUS_NOT_FOUND:
    case STATUS_INVALID_PARAMETER:
    case STATUS_ACCESS_DENIED:
    case STATUS_DEVICE_NOT_READY:
    case STATUS_DEVICE_CONFIGURATION_ERROR:
    case STATUS_INTEGER_OVERFLOW:
    case STATUS_CONFLICTING_ADDRESSES:
        Item->Disabled = TRUE;
        PhTraceInfo("Hardware sensor 0x%lx unavailable: %!STATUS!", Id, Status);
        break;
    default:
        if (++Item->RetryCount >= PHP_HW_STRIKE_LIMIT)
        {
            Item->Disabled = TRUE;
            PhTraceInfo("Hardware sensor 0x%lx unavailable: %!STATUS!", Id, Status);
        }
        break;
    }

    return FALSE;
}

static PPHP_HW_SAMPLE PhpHwGetSample(
    _In_ PHP_HW_REG Reg,
    _In_ ULONG Index
    )
{
    switch (PhpHwRegisters[Reg].Scope)
    {
    case PhpHwScopeThread:
        return &PhpHwProcessorSamples[Index];
    case PhpHwScopeCore:
        return &PhpHwCores[Index].Sample;
    default:
        return &PhpHwPackages[Index].Sample;
    }
}

static VOID PhpHwRollSample(
    _Inout_ PPHP_HW_SAMPLE Sample
    )
{
    Sample->LastValid = Sample->Valid;
    memcpy(Sample->LastValue, Sample->Value, sizeof(Sample->Value));
    Sample->Valid = 0;
}

static VOID PhpHwResetSamples(
    VOID
    )
{
    for (ULONG i = 0; i < PhpHwProcessorCount; i++)
        PhpHwProcessorSamples[i].Valid = PhpHwProcessorSamples[i].LastValid = 0;
    for (ULONG i = 0; i < PhpHwCoreCount; i++)
        PhpHwCores[i].Sample.Valid = PhpHwCores[i].Sample.LastValid = 0;
    for (ULONG i = 0; i < PhpHwPackageCount; i++)
        PhpHwPackages[i].Sample.Valid = PhpHwPackages[i].Sample.LastValid = 0;
}

_Success_(return)
static BOOLEAN PhpHwSampleValue(
    _In_ PPHP_HW_SAMPLE Sample,
    _In_ PHP_HW_REG Reg,
    _Out_ PULONG64 Value
    )
{
    if (!(Sample->Valid & (1UL << Reg)))
        return FALSE;

    *Value = Sample->Value[Reg];
    return TRUE;
}

_Success_(return)
static BOOLEAN PhpHwSampleDelta(
    _In_ PPHP_HW_SAMPLE Sample,
    _In_ PHP_HW_REG Reg,
    _Out_ PULONG64 Delta
    )
{
    if (!(Sample->Valid & Sample->LastValid & (1UL << Reg)))
        return FALSE;

    *Delta = Sample->Value[Reg] - Sample->LastValue[Reg];
    return TRUE;
}

_Success_(return)
static BOOLEAN PhpHwSampleSeconds(
    _Out_ PFLOAT Seconds
    )
{
    if (PhpHwLastSampleTime.QuadPart == 0 || PhpHwFrequency.QuadPart == 0)
        return FALSE;
    if (PhpHwSampleTime.QuadPart <= PhpHwLastSampleTime.QuadPart)
        return FALSE;

    *Seconds = (FLOAT)(PhpHwSampleTime.QuadPart - PhpHwLastSampleTime.QuadPart) / (FLOAT)PhpHwFrequency.QuadPart;
    return TRUE;
}

/**
 * Converts a 32-bit energy counter delta to watts over the sample interval.
 */
_Success_(return)
static BOOLEAN PhpHwSampleWatts(
    _In_ PPHP_HW_SAMPLE Sample,
    _In_ PHP_HW_REG Reg,
    _In_ FLOAT EnergyUnit,
    _Out_ PFLOAT Watts
    )
{
    ULONG64 delta;
    FLOAT seconds;

    if (EnergyUnit <= 0.0f)
        return FALSE;
    if (!PhpHwSampleSeconds(&seconds))
        return FALSE;
    if (!PhpHwSampleDelta(Sample, Reg, &delta))
        return FALSE;

    *Watts = (FLOAT)(ULONG)delta * EnergyUnit / seconds;

    return TRUE;
}

static VOID PhpHwAddEntry(
    _Inout_ PULONG Count,
    _In_ PHP_HW_REG Reg,
    _In_ ULONG Processor,
    _In_ ULONG Index
    )
{
    PKPH_HW_MSR_BATCH_ENTRY entry;

    if (*Count >= PhpHwBatchCapacity)
        return;

    entry = &PhpHwBatch[*Count];
    entry->Register = PhpHwRegisters[Reg].Register;
    entry->Processor = PhpHwCpuSensors[Processor].ProcessorNumber;
    entry->Value = 0;
    entry->Status = PHP_HW_ENTRY_UNSET;
    PhpHwBatchTags[*Count].Reg = Reg;
    PhpHwBatchTags[*Count].Index = Index;

    (*Count)++;
}

/**
 * Reads a chunk of the batch through KphHwReadSysregBatch and keeps the reported MIDR per processor.
 */
static NTSTATUS PhpHwSendSysregBatch(
    _Inout_updates_(Count) PKPH_HW_MSR_BATCH_ENTRY Entries,
    _In_ ULONG Count
    )
{
    NTSTATUS status;

    for (ULONG i = 0; i < Count; i++)
    {
        PKPH_HW_SYSREG_BATCH_ENTRY entry = &PhpHwSysregBatch[i];

        memset(entry, 0, sizeof(KPH_HW_SYSREG_BATCH_ENTRY));
        entry->Register = (UCHAR)Entries[i].Register;
        entry->Processor = Entries[i].Processor;
        entry->Status = Entries[i].Status;
    }

    status = KphHwReadSysregBatch(PhpHwSysregBatch, Count);

    for (ULONG i = 0; i < Count; i++)
    {
        PKPH_HW_SYSREG_BATCH_ENTRY entry = &PhpHwSysregBatch[i];
        ULONG index;

        Entries[i].Value = entry->Value;
        Entries[i].Status = entry->Status;

        if (entry->Midr)
        {
            index = PhpHwFlatProcessorIndex(entry->Processor.Group, entry->Processor.Number);

            if (index != ULONG_MAX)
                PhpHwArmMidr[index] = entry->Midr;
        }
    }

    return status;
}

static BOOLEAN PhpHwArmRegisterUsable(
    _In_ PHP_HW_REG Reg,
    _In_ ULONG Processor
    )
{
    return PhpHwRegisterActive(Reg) && !(PhpHwArmDisabled[Processor] & (1UL << Reg));
}

/**
 * Stops reading an ARM64 register on a processor; disabled for the session once no processor reads it.
 */
static VOID PhpHwArmDisableRegister(
    _In_ PHP_HW_REG Reg,
    _In_ ULONG Processor
    )
{
    PhpHwArmDisabled[Processor] |= 1UL << Reg;

    for (ULONG i = 0; i < PhpHwProcessorCount; i++)
    {
        if (!(PhpHwArmDisabled[i] & (1UL << Reg)))
            return;
    }

    if (PhpHwRegisterActive(Reg))
    {
        PhTraceInfo("Hardware sensor 0x%lx unavailable on every processor", PhpHwRegisters[Reg].Register);
        PhpHwDisableRegister(Reg);
    }
}

/**
 * Applies the session failure policy to an ARM64 entry on its processor only.
 *
 * \return TRUE when the value of the entry is usable.
 */
static BOOLEAN PhpHwArmApplyStatus(
    _In_ PHP_HW_REG Reg,
    _In_ ULONG Processor,
    _In_ NTSTATUS Status
    )
{
    PHP_HW_ITEM item = { 0 };

    if (NT_SUCCESS(Status))
        return TRUE;

    PhpHwApplyStatus(&item, PhpHwRegisters[Reg].Register, Status);

    if (item.Disabled)
        PhpHwArmDisableRegister(Reg, Processor);

    return FALSE;
}

/**
 * Selects the AMU when the processor reads both of its counters, else PMCCNTR_EL0.
 */
static PHP_HW_ARM_SOURCE PhpHwArmSource(
    _In_ ULONG Processor
    )
{
    if (!PhpHwArmCounterHz || !PhpHwArmRegisterUsable(PhpHwRegArmCounter, Processor))
        return PhpHwArmSourceNone;
    if (PhpHwArmRegisterUsable(PhpHwRegArmCycles, Processor) && PhpHwArmRegisterUsable(PhpHwRegArmConstantCycles, Processor))
        return PhpHwArmSourceAmu;
    if (PhpHwArmRegisterUsable(PhpHwRegArmPmuCycles, Processor))
        return PhpHwArmSourcePmu;

    return PhpHwArmSourceNone;
}

/**
 * Sends the batch in chunks of PHP_HW_REGISTER_BATCH.
 *
 * \return TRUE when the driver answered at least one entry.
 */
static BOOLEAN PhpHwSendBatch(
    _In_ ULONG Count
    )
{
    BOOLEAN answered = FALSE;

    for (ULONG i = 0; i < Count; i += PHP_HW_REGISTER_BATCH)
    {
        NTSTATUS status;

        if (PhpHwArm64)
            status = PhpHwSendSysregBatch(&PhpHwBatch[i], min(Count - i, PHP_HW_REGISTER_BATCH));
        else
            status = KphHwReadMsrBatch(&PhpHwBatch[i], min(Count - i, PHP_HW_REGISTER_BATCH));

        if (!NT_SUCCESS(status))
            PhTrace("Hardware sensor batch failed: %!STATUS!", status);
    }

    PhQueryPerformanceCounter(&PhpHwSampleTime);

    for (ULONG i = 0; i < Count; i++)
    {
        PKPH_HW_MSR_BATCH_ENTRY entry = &PhpHwBatch[i];
        PPHP_HW_BATCH_TAG tag = &PhpHwBatchTags[i];
        PPHP_HW_SAMPLE sample;

        // An unanswered entry is invalid for this tick only.
        if (entry->Status == PHP_HW_ENTRY_UNSET)
            continue;

        answered = TRUE;

        // An inactive processor fails its thread-scope entries; skipped without a strike.
        if (entry->Status == STATUS_INVALID_PARAMETER && PhpHwRegisters[tag->Reg].Scope == PhpHwScopeThread)
            continue;

        // Every ARM64 register is thread scope: Index is the processor.
        if (PhpHwArm64 ? !PhpHwArmApplyStatus(tag->Reg, tag->Index, entry->Status) : !PhpHwApplyStatus(&PhpHwRegisters[tag->Reg].Item, entry->Register, entry->Status))
            continue;

        sample = PhpHwGetSample(tag->Reg, tag->Index);
        sample->Value[tag->Reg] = entry->Value;
        sample->Valid |= 1UL << tag->Reg;
    }

    return answered;
}

static BOOLEAN PhpHwIsIntelNehalem(
    VOID
    )
{
    static const UCHAR models[] = { 0x1A, 0x1E, 0x1F, 0x25, 0x2C, 0x2E, 0x2F };

    if (PhpHwIdentity.DisplayFamily != 0x06)
        return FALSE;

    for (ULONG i = 0; i < RTL_NUMBER_OF(models); i++)
    {
        if (PhpHwIdentity.DisplayModel == models[i])
            return TRUE;
    }

    return FALSE;
}

/**
 * Parses the digit groups of a PCI location string as the driver does.
 *
 * \return The number of groups (3, or 4 with a segment), or 0 when malformed.
 */
static ULONG PhpHwParseLocation(
    _In_reads_(Count) PCWSTR Location,
    _In_ ULONG Count,
    _Out_writes_(4) PULONG Numbers
    )
{
    ULONG numberCount = 0;
    BOOLEAN inNumber = FALSE;

    memset(Numbers, 0, sizeof(ULONG) * 4);

    for (ULONG i = 0; i < Count && Location[i] != UNICODE_NULL; i++)
    {
        WCHAR c = Location[i];

        if (c < L'0' || c > L'9')
        {
            inNumber = FALSE;
            continue;
        }

        if (!inNumber)
        {
            if (numberCount == 4)
                return 0;

            inNumber = TRUE;
            numberCount++;
        }

        Numbers[numberCount - 1] = Numbers[numberCount - 1] * 10 + (c - L'0');

        if (Numbers[numberCount - 1] > USHRT_MAX)
            return 0;
    }

    return numberCount;
}

static ULONG64 PhpHwLocatorKey(
    _In_ PKPH_HW_PCI_LOCATOR Locator
    )
{
    return ((ULONG64)Locator->Segment << 24) | ((ULONG64)Locator->Bus << 16) | ((ULONG64)Locator->Device << 8) | Locator->Function;
}

_Success_(return)
static BOOLEAN PhpHwParseHardwareIdField(
    _In_ PCPH_STRINGREF HardwareId,
    _In_ PCWSTR Field,
    _Out_ PUSHORT Value
    )
{
    PH_STRINGREF part;
    ULONG64 value;
    SIZE_T index;

    if ((index = PhFindStringInStringRefZ(HardwareId, Field, TRUE)) == SIZE_MAX)
        return FALSE;
    if (HardwareId->Length < (index + 8) * sizeof(WCHAR))
        return FALSE;

    part.Buffer = &HardwareId->Buffer[index + 4];
    part.Length = 4 * sizeof(WCHAR);

    if (!PhStringToUInt64(&part, 16, &value))
        return FALSE;

    *Value = (USHORT)value;
    return TRUE;
}

static BOOLEAN PhpHwHasCompatibleClass(
    _In_reads_(Count) PCWSTR CompatibleIds,
    _In_ ULONG Count,
    _In_ PCPH_STRINGREF Class
    )
{
    for (ULONG i = 0; i < Count && CompatibleIds[i] != UNICODE_NULL; )
    {
        PH_STRINGREF id;

        id.Buffer = (PWSTR)&CompatibleIds[i];
        id.Length = PhCountStringZ(id.Buffer) * sizeof(WCHAR);

        if (PhStartsWithStringRef(&id, Class, TRUE))
            return TRUE;

        i += (ULONG)(id.Length / sizeof(WCHAR)) + 1;
    }

    return FALSE;
}

/**
 * Finds the present PCI functions of a vendor and class, sorted by location.
 *
 * \param VendorId The vendor ID to accept, or PHP_HW_ANY_VENDOR_ID for any.
 * \param Class The class compatible ID prefix, e.g. PCI\CC_0600.
 * \param DeviceIds The device IDs to accept, or NULL for any.
 * \param DeviceZeroOnly TRUE to accept device 0, function 0 only.
 */
static ULONG PhpHwFindPciDevices(
    _In_ USHORT VendorId,
    _In_ USHORT AlternateVendorId,
    _In_ PCPH_STRINGREF Class,
    _In_reads_opt_(DeviceIdCount) const USHORT* DeviceIds,
    _In_ ULONG DeviceIdCount,
    _In_ BOOLEAN DeviceZeroOnly,
    _Out_writes_to_(Count, return) PKPH_HW_PCI_LOCATOR Locators,
    _In_ ULONG Count
    )
{
    HDEVINFO deviceInfoSet;
    SP_DEVINFO_DATA deviceInfoData;
    ULONG found = 0;

    deviceInfoSet = SetupDiGetClassDevsW(NULL, L"PCI", NULL, DIGCF_ALLCLASSES | DIGCF_PRESENT);

    if (deviceInfoSet == INVALID_HANDLE_VALUE)
        return 0;

    memset(&deviceInfoData, 0, sizeof(SP_DEVINFO_DATA));
    deviceInfoData.cbSize = sizeof(SP_DEVINFO_DATA);

    for (ULONG i = 0; found < Count && SetupDiEnumDeviceInfo(deviceInfoSet, i, &deviceInfoData); i++)
    {
        WCHAR ids[512];
        WCHAR location[128];
        PH_STRINGREF hardwareId;
        DEVPROPTYPE propertyType;
        ULONG busNumber;
        ULONG address;
        ULONG numbers[4];
        ULONG numberCount;
        USHORT vendorId;
        KPH_HW_PCI_LOCATOR locator;
        ULONG j;

        memset(ids, 0, sizeof(ids));

        if (!SetupDiGetDevicePropertyW(deviceInfoSet, &deviceInfoData, &DEVPKEY_Device_HardwareIds, &propertyType, (PBYTE)ids, sizeof(ids) - sizeof(WCHAR) * 2, NULL, 0))
            continue;

        PhInitializeStringRefLongHint(&hardwareId, ids);

        if (!PhpHwParseHardwareIdField(&hardwareId, L"VEN_", &vendorId))
            continue;
        if (VendorId != PHP_HW_ANY_VENDOR_ID && vendorId != VendorId && vendorId != AlternateVendorId)
            continue;

        if (DeviceIds)
        {
            USHORT deviceId;

            if (!PhpHwParseHardwareIdField(&hardwareId, L"DEV_", &deviceId))
                continue;

            for (j = 0; j < DeviceIdCount; j++)
            {
                if (DeviceIds[j] == deviceId)
                    break;
            }

            if (j == DeviceIdCount)
                continue;
        }

        memset(ids, 0, sizeof(ids));

        if (!SetupDiGetDevicePropertyW(deviceInfoSet, &deviceInfoData, &DEVPKEY_Device_CompatibleIds, &propertyType, (PBYTE)ids, sizeof(ids) - sizeof(WCHAR) * 2, NULL, 0))
            continue;
        if (!PhpHwHasCompatibleClass(ids, RTL_NUMBER_OF(ids), Class))
            continue;

        if (!SetupDiGetDevicePropertyW(deviceInfoSet, &deviceInfoData, &DEVPKEY_Device_BusNumber, &propertyType, (PBYTE)&busNumber, sizeof(ULONG), NULL, 0))
            continue;
        if (!SetupDiGetDevicePropertyW(deviceInfoSet, &deviceInfoData, &DEVPKEY_Device_Address, &propertyType, (PBYTE)&address, sizeof(ULONG), NULL, 0))
            continue;
        if ((DeviceZeroOnly && address != 0) || busNumber > UCHAR_MAX || HIWORD(address) > 0x1F || LOWORD(address) > 0x7)
            continue;

        memset(location, 0, sizeof(location));

        if (!SetupDiGetDevicePropertyW(deviceInfoSet, &deviceInfoData, &DEVPKEY_Device_LocationInfo, &propertyType, (PBYTE)location, sizeof(location) - sizeof(WCHAR), NULL, 0))
            continue;

        numberCount = PhpHwParseLocation(location, RTL_NUMBER_OF(location), numbers);

        memset(&locator, 0, sizeof(KPH_HW_PCI_LOCATOR));

        if (numberCount == 4)
            locator.Segment = (USHORT)numbers[0];
        else if (numberCount != 3)
            continue;

        locator.Bus = (UCHAR)busNumber;
        locator.Device = (UCHAR)HIWORD(address);
        locator.Function = (UCHAR)LOWORD(address);

        for (j = found; j > 0; j--)
        {
            if (PhpHwLocatorKey(&Locators[j - 1]) <= PhpHwLocatorKey(&locator))
                break;

            Locators[j] = Locators[j - 1];
        }

        Locators[j] = locator;
        found++;
    }

    SetupDiDestroyDeviceInfoList(deviceInfoSet);

    return found;
}

/**
 * Splits the root complexes evenly over the packages in bus order.
 */
static VOID PhpHwAssignRootComplexes(
    VOID
    )
{
    static CONST PH_STRINGREF hostBridgeClass = PH_STRINGREF_INIT(L"PCI\\CC_0600");
    KPH_HW_PCI_LOCATOR roots[PHP_HW_MAX_ROOTS];
    ULONG rootCount;

    rootCount = PhpHwFindPciDevices(PHP_HW_AMD_VENDOR_ID, PHP_HW_HYGON_VENDOR_ID, &hostBridgeClass, NULL, 0, TRUE, roots, RTL_NUMBER_OF(roots));

    PhTraceInfo("Hardware sensors: %lu root complexes", rootCount);

    if (rootCount == 0)
        return;

    if (rootCount % PhpHwPackageCount == 0)
    {
        ULONG step = rootCount / PhpHwPackageCount;

        for (ULONG i = 0; i < PhpHwPackageCount; i++)
        {
            PhpHwPackages[i].HasRoot = TRUE;
            PhpHwPackages[i].Root = roots[i * step];
        }
    }
    else
    {
        PhpHwPackages[0].HasRoot = TRUE;
        PhpHwPackages[0].Root = roots[0];
    }
}

/**
 * Acquires Global\Access_PCI, held by hardware monitors around PCI index/data pairs.
 *
 * \return TRUE when the mutex is held.
 */
static BOOLEAN PhpHwAcquirePciMutant(
    VOID
    )
{
    NTSTATUS status;

    status = PhAcquireHardwareBus(PhHardwareBusPci, PhpHwPciTimeouts >= PHP_HW_PCI_CONTENDED_LIMIT ? 0 : 100);

    if (NT_SUCCESS(status))
    {
        PhpHwPciTimeouts = 0;
        return TRUE;
    }

    if (status == STATUS_RETRY && PhpHwPciTimeouts < PHP_HW_PCI_CONTENDED_LIMIT)
        PhpHwPciTimeouts++;

    return FALSE;
}

static VOID PhpHwReleasePciMutant(
    VOID
    )
{
    PhReleaseHardwareBus(PhHardwareBusPci);
}

_Success_(return)
static BOOLEAN PhpHwReadSmn(
    _In_ PPHP_HW_PACKAGE Package,
    _Inout_ PPHP_HW_ITEM Item,
    _In_ ULONG Target,
    _Out_ PULONG Value
    )
{
    NTSTATUS status;
    ULONG value;

    if (Item->Disabled)
        return FALSE;

    status = KphHwReadPciIndirect(&Package->Root, Target, &value);

    if (!PhpHwApplyStatus(Item, Target, status))
        return FALSE;

    *Value = value;
    return TRUE;
}

static BOOLEAN PhpHwSmnSupported(
    _In_ const KPH_HW_CAPABILITIES* Capabilities
    )
{
    return !PhpHwIntel &&
        FlagOn(Capabilities->Flags, KPH_HW_CAP_PCI_AVAILABLE) &&
        FlagOn(Capabilities->ImplementedKinds, 1ULL << KPH_DYN_HW_KIND_PCI_INDIRECT_READ);
}

static VOID PhpHwPrepareSmn(
    _In_ const KPH_HW_CAPABILITIES* Capabilities
    )
{
    PhpHwSmnPending = FALSE;

    for (ULONG i = 0; i < PhpHwPackageCount; i++)
    {
        PhpHwPackages[i].Tctl.Disabled = TRUE;
        PhpHwPackages[i].CcdMask = 0;
        PhpHwPackages[i].MemoryClock.Disabled = TRUE;
        PhpHwPackages[i].MemoryChannel = ULONG_MAX;
        PhpHwPackages[i].MemoryClockValid = FALSE;
        PhpHwPackages[i].MemoryTimingsRead = FALSE;
        PhpHwPackages[i].MemoryTimingsValid = FALSE;
    }

    if (!PhpHwSmnSupported(Capabilities))
        return;

    if (!PhpHwRootsEnumerated)
    {
        PhpHwRootsEnumerated = TRUE;
        PhpHwAssignRootComplexes();
    }

    if (!PhAmdTccdAddressRange(&PhpHwIdentity, &PhpHwCcdBase, &PhpHwCcdCount))
    {
        PhpHwCcdBase = 0;
        PhpHwCcdCount = 0;
    }

    if (!PhAmdUmcChannelCount(&PhpHwIdentity, &PhpHwUmcChannelCount))
        PhpHwUmcChannelCount = 0;

    if (PhpHwCcdCount > PH_HW_MAX_CCD)
        PhpHwCcdCount = PH_HW_MAX_CCD;

    for (ULONG i = 0; i < PhpHwPackageCount; i++)
    {
        if (PhpHwPackages[i].HasRoot)
            PhpHwSmnPending = TRUE;
    }
}

/**
 * Finds the first enabled UMC channel with a DIMM. The caller holds Global\Access_PCI.
 *
 * \param Probe TRUE to disable the item when no channel is active.
 */
static BOOLEAN PhpHwFindMemoryChannelLocked(
    _Inout_ PPHP_HW_PACKAGE Package,
    _In_ BOOLEAN Probe
    )
{
    for (ULONG i = 0; i < PhpHwUmcChannelCount; i++)
    {
        ULONG channel = PH_AMD_SMN_UMC_BASE + (i << PH_AMD_SMN_UMC_CHANNEL_SHIFT);
        ULONG umcCap;
        ULONG cs0;
        ULONG cs2;

        if (
            !PhpHwReadSmn(Package, &Package->MemoryClock, channel + PH_AMD_UMC_CAP, &umcCap) ||
            !PhpHwReadSmn(Package, &Package->MemoryClock, channel + PH_AMD_UMC_BASE_ADDR_CS0, &cs0) ||
            !PhpHwReadSmn(Package, &Package->MemoryClock, channel + PH_AMD_UMC_BASE_ADDR_CS2, &cs2)
            )
        {
            return FALSE;
        }

        if (!(umcCap & (1UL << 19)) && ((cs0 | cs2) & 1))
        {
            Package->MemoryChannel = channel;
            return TRUE;
        }
    }

    if (Probe)
        Package->MemoryClock.Disabled = TRUE;

    return FALSE;
}

/**
 * Reads the DDR5 timings of the memory channel of a package. The caller holds Global\Access_PCI.
 */
static VOID PhpHwReadMemoryTimingsLocked(
    _Inout_ PPHP_HW_PACKAGE Package,
    _In_ ULONG DramConfiguration
    )
{
    ULONG channel = Package->MemoryChannel;
    ULONG configuration;
    ULONG timing1;
    ULONG timing2;
    ULONG refresh = 0;

    if (!PhAmdUmcIsDdr5Layout(&PhpHwIdentity))
        return;

    if (
        !PhpHwReadSmn(Package, &Package->MemoryClock, channel + PH_AMD_UMC_CONFIGURATION, &configuration) ||
        (configuration & 0x7) != 1 ||
        !PhpHwReadSmn(Package, &Package->MemoryClock, channel + PH_AMD_UMC_DRAM_TIMING1, &timing1) ||
        !PhpHwReadSmn(Package, &Package->MemoryClock, channel + PH_AMD_UMC_DRAM_TIMING2, &timing2)
        )
    {
        return;
    }

    // An unused refresh register reads zero or the reset value 00C00138h.
    for (ULONG i = 0; i < 4; i++)
    {
        ULONG value;

        if (!PhpHwReadSmn(Package, &Package->MemoryClock, channel + PH_AMD_UMC_DRAM_TIMING_RFC + i * 4, &value))
            return;

        if (value && value != 0x00C00138)
        {
            refresh = value & 0xFFFF;
            break;
        }
    }

    Package->MemoryTimings.CasLatency = timing1 & 0x3F;
    Package->MemoryTimings.RasToCasDelay = (timing1 >> 16) & 0x3F;
    Package->MemoryTimings.RowActive = (timing1 >> 8) & 0x7F;
    Package->MemoryTimings.RowPrecharge = (timing2 >> 16) & 0x3F;
    Package->MemoryTimings.RowCycle = timing2 & 0xFF;
    Package->MemoryTimings.RefreshCycle = refresh;
    Package->MemoryTimings.CommandRate = (DramConfiguration & (1UL << 17)) ? 2 : 1;
    Package->MemoryTimings.GearDownValid = TRUE;
    Package->MemoryTimings.GearDown = !!(DramConfiguration & (1UL << 18));
    Package->MemoryTimingsValid = Package->MemoryTimings.CasLatency != 0;
}

/**
 * Reads the memory clock of a package. The caller holds Global\Access_PCI.
 *
 * \param Probe TRUE to disable the item on a failed decode; otherwise the last value is kept.
 */
static VOID PhpHwReadMemoryClockLocked(
    _In_ ULONG Index,
    _In_ BOOLEAN Probe
    )
{
    PPHP_HW_PACKAGE package = &PhpHwPackages[Index];
    ULONG value;
    FLOAT mhz;
    FLOAT dataRate;

    if (!package->HasRoot || package->MemoryClock.Disabled)
        return;

    if (
        (package->MemoryChannel != ULONG_MAX || PhpHwFindMemoryChannelLocked(package, Probe)) &&
        PhpHwReadSmn(package, &package->MemoryClock, package->MemoryChannel + PH_AMD_UMC_DRAM_CONFIGURATION, &value)
        )
    {
        if (PhAmdDramConfigurationToMhz(value, &PhpHwIdentity, &mhz, &dataRate))
        {
            package->UmcMemoryClockMhz = mhz;
            package->UmcMemoryDataRate = dataRate;
            package->MemoryClockValid = TRUE;

            if (!package->MemoryTimingsRead)
            {
                package->MemoryTimingsRead = TRUE;
                PhpHwReadMemoryTimingsLocked(package, value);
            }
        }
        else if (Probe)
        {
            package->MemoryClock.Disabled = TRUE;
        }
    }

    if (package->MemoryClock.Disabled)
    {
        package->MemoryClockValid = FALSE;
        package->MemoryTimingsValid = FALSE;
    }
}

/**
 * The caller holds Global\Access_PCI.
 */
static VOID PhpHwProbeSmnLocked(
    VOID
    )
{
    PhpHwSmnPending = FALSE;
    PhpHwMemoryClockTicks = 0;

    for (ULONG i = 0; i < PhpHwPackageCount; i++)
    {
        PPHP_HW_PACKAGE package = &PhpHwPackages[i];
        ULONG value;
        FLOAT celsius;

        if (!package->HasRoot)
            continue;

        package->Tctl.Disabled = FALSE;
        package->Tctl.RetryCount = 0;

        if (PhpHwReadSmn(package, &package->Tctl, PH_AMD_SMN_THM_TCON_CUR_TMP, &value) && !PhAmdTctlToCelsius(value, &celsius))
            package->Tctl.Disabled = TRUE;

        for (ULONG j = 0; j < PhpHwCcdCount; j++)
        {
            package->Ccd[j].Disabled = FALSE;
            package->Ccd[j].RetryCount = 0;

            // An unpopulated CCD reads with the valid bit clear.
            if (PhpHwReadSmn(package, &package->Ccd[j], PhpHwCcdBase + j * 4, &value))
            {
                if (PhAmdTccdToCelsius(value, &celsius))
                    package->CcdMask |= 1UL << j;
                else
                    package->Ccd[j].Disabled = TRUE;
            }
            else if (!package->Ccd[j].Disabled)
            {
                package->CcdMask |= 1UL << j;
            }
        }

        if (PhpHwUmcChannelCount)
        {
            package->MemoryClock.Disabled = FALSE;
            package->MemoryClock.RetryCount = 0;
            package->MemoryTimingsRead = FALSE;
            PhpHwReadMemoryClockLocked(i, TRUE);
        }
    }
}

static BOOLEAN PhpHwWindowsSupported(
    _In_ const KPH_HW_CAPABILITIES* Capabilities
    )
{
    return PhpHwIntel &&
        FlagOn(Capabilities->Flags, KPH_HW_CAP_PCI_AVAILABLE) &&
        FlagOn(Capabilities->ImplementedKinds, 1ULL << KPH_DYN_HW_KIND_WINDOW_READ);
}

/**
 * Applies the session failure policy; STATUS_DEVICE_NOT_READY counts a strike instead of ending the item.
 */
static BOOLEAN PhpHwApplyWindowStatus(
    _Inout_ PPHP_HW_ITEM Item,
    _In_ ULONG Id,
    _In_ NTSTATUS Status
    )
{
    return PhpHwApplyStatus(Item, Id, Status == STATUS_DEVICE_NOT_READY ? STATUS_DEVICE_BUSY : Status);
}

/**
 * Reads a window in one batch; entry statuses apply to registers, a batch status to the window.
 */
static VOID PhpHwReadWindow(
    _Inout_ PPHP_HW_WINDOW Window
    )
{
    KPH_HW_WINDOW_BATCH_ENTRY entries[PhpHwWinMaximum];
    PHP_HW_WINDOW_REG regs[PhpHwWinMaximum];
    ULONG count = 0;
    BOOLEAN answered = FALSE;
    NTSTATUS status;

    Window->LastValid = Window->Valid;
    memcpy(Window->LastValue, Window->Value, sizeof(Window->Value));
    Window->LastSampleTime = Window->SampleTime;
    Window->Valid = 0;

    if (Window->Item.Disabled)
        return;

    for (PHP_HW_WINDOW_REG reg = 0; reg < PhpHwWinMaximum; reg++)
    {
        if (Window->Registers[reg].Disabled)
            continue;

        memset(&entries[count], 0, sizeof(KPH_HW_WINDOW_BATCH_ENTRY));
        entries[count].Offset = PhpHwWindowRegInfo[reg].Offset;
        entries[count].Width = PhpHwWindowRegInfo[reg].Width;
        regs[count] = reg;
        count++;
    }

    if (count == 0)
    {
        Window->Item.Disabled = TRUE;
        return;
    }

    status = KphHwReadWindowBatch(&Window->Locator, 0, entries, count);
    PhQueryPerformanceCounter(&Window->SampleTime);

    for (ULONG i = 0; i < count; i++)
    {
        // A whole-batch failure leaves every entry unset.
        if (entries[i].Status == PHP_HW_ENTRY_UNSET)
            continue;

        answered = TRUE;

        if (!PhpHwApplyWindowStatus(&Window->Registers[regs[i]], entries[i].Offset, entries[i].Status))
            continue;

        Window->Value[regs[i]] = entries[i].Value;
        Window->Valid |= 1ULL << regs[i];
    }

    if (answered)
        Window->Item.RetryCount = 0;
    else
        PhpHwApplyWindowStatus(&Window->Item, (ULONG)PhpHwLocatorKey(&Window->Locator), status);
}

static BOOLEAN PhpHwWindowRegisterValid(
    _In_ PHP_HW_WINDOW_REG Reg
    )
{
    for (ULONG i = 0; i < PhpHwWindowCount; i++)
    {
        if (PhpHwWindows[i].Valid & (1ULL << Reg))
            return TRUE;
    }

    return FALSE;
}

/**
 * Retrieves a register from the first window that has it.
 */
_Success_(return)
static BOOLEAN PhpHwWindowValue64(
    _In_ PHP_HW_WINDOW_REG Reg,
    _Out_ PULONG64 Value
    )
{
    for (ULONG i = 0; i < PhpHwWindowCount; i++)
    {
        if (PhpHwWindows[i].Valid & (1ULL << Reg))
        {
            *Value = PhpHwWindows[i].Value[Reg];
            return TRUE;
        }
    }

    return FALSE;
}

/**
 * Retrieves a register up to 4 bytes wide from the first window that has it.
 */
_Success_(return)
static BOOLEAN PhpHwWindowValue(
    _In_ PHP_HW_WINDOW_REG Reg,
    _Out_ PULONG Value
    )
{
    ULONG64 value;

    if (!PhpHwWindowValue64(Reg, &value))
        return FALSE;

    *Value = (ULONG)value;
    return TRUE;
}

static BOOLEAN PhpHwWindowRegisterActive(
    _In_ PHP_HW_WINDOW_REG Reg
    )
{
    for (ULONG i = 0; i < PhpHwWindowCount; i++)
    {
        if (!PhpHwWindows[i].Item.Disabled && !PhpHwWindows[i].Registers[Reg].Disabled)
            return TRUE;
    }

    return FALSE;
}

static VOID PhpHwDisableWindowRegister(
    _In_ PHP_HW_WINDOW_REG Reg
    )
{
    for (ULONG i = 0; i < PhpHwWindowCount; i++)
        PhpHwWindows[i].Registers[Reg].Disabled = TRUE;
}

_Function_class_(PH_ENUM_SMBIOS_CALLBACK)
static BOOLEAN NTAPI PhpHwSmbiosMemoryCallback(
    _In_ ULONG_PTR EnumHandle,
    _In_ UCHAR MajorVersion,
    _In_ UCHAR MinorVersion,
    _In_ PPH_SMBIOS_ENTRY Entry,
    _In_opt_ PVOID Context
    )
{
    PULONG configuredSpeed = Context;
    ULONG speed = 0;

    // Before 2.8 the configured speed may be a clock in MHz rather than MT/s.
    if (!configuredSpeed || MajorVersion < 2 || (MajorVersion == 2 && MinorVersion < 8))
        return TRUE;
    if (Entry->Header.Type != SMBIOS_MEMORY_DEVICE_INFORMATION_TYPE || !Entry->MemoryDevice.Size.Value)
        return FALSE;

    if (PH_SMBIOS_CONTAINS_FIELD(Entry, MemoryDevice, ConfiguredSpeed))
        speed = Entry->MemoryDevice.ConfiguredSpeed;
    if (speed == MAXUSHORT && PH_SMBIOS_CONTAINS_FIELD(Entry, MemoryDevice, ExtendedConfiguredSpeed))
        speed = Entry->MemoryDevice.ExtendedConfiguredSpeed;
    if (speed != MAXUSHORT && speed > *configuredSpeed)
        *configuredSpeed = speed;

    return FALSE;
}

/**
 * Rejects a data rate within 7% of half or twice the SMBIOS configured speed (wrong unit).
 */
_Success_(return)
static BOOLEAN PhpHwDecodeWindowMemoryClock(
    _Out_ PFLOAT MemoryClockMhz,
    _Out_ PFLOAT DataRate,
    _Out_ PULONG Gear
    )
{
    static BOOLEAN mismatchTraced = FALSE;
    ULONG req;
    ULONG data;
    ULONG saPerfStatus;
    ULONG bclk;
    ULONG memss;
    BOOLEAN reqValid = PhpHwWindowValue(PhpHwWinMcBiosReq, &req);
    BOOLEAN dataValid = PhpHwWindowValue(PhpHwWinMcBiosData, &data);
    BOOLEAN saPerfStatusValid = PhpHwWindowValue(PhpHwWinSaPerfStatus, &saPerfStatus);
    BOOLEAN bclkValid = PhpHwWindowValue(PhpHwWinBclkFreq, &bclk);
    BOOLEAN memssValid = PhpHwWindowValue(PhpHwWinMemssBiosData, &memss);
    FLOAT mhz;
    FLOAT dataRate;
    ULONG gear;

    if (!PhIntelMchbarToMemoryClock(
        &PhpHwIdentity,
        reqValid ? &req : NULL,
        dataValid ? &data : NULL,
        saPerfStatusValid ? &saPerfStatus : NULL,
        bclkValid ? &bclk : NULL,
        memssValid ? &memss : NULL,
        &mhz,
        &dataRate,
        &gear
        ))
    {
        return FALSE;
    }

    PhpHwDecodedDataRate = dataRate;
    PhpHwDataRateRejected = FALSE;

    if (PhpHwSmbiosDataRate)
    {
        FLOAT ratio = dataRate / (FLOAT)PhpHwSmbiosDataRate;

        if ((ratio >= 0.5f * 0.93f && ratio <= 0.5f * 1.07f) || (ratio >= 2.0f * 0.93f && ratio <= 2.0f * 1.07f))
        {
            if (!mismatchTraced)
            {
                PhTraceWarning("Hardware sensors: memory data rate %lu MT/s disagrees with SMBIOS %lu MT/s", (ULONG)dataRate, PhpHwSmbiosDataRate);
                mismatchTraced = TRUE;
            }

            PhpHwDataRateRejected = TRUE;
            return FALSE;
        }
    }

    *MemoryClockMhz = mhz;
    *DataRate = dataRate;
    *Gear = gear;
    return TRUE;
}

/**
 * Converts the IMC data read/write count deltas to MB/s (10^6 bytes).
 *
 * \param DataRate The data rate in MT/s, or 0 when unknown.
 * \return FALSE for the tick when an interval or a counter delta is implausible.
 */
_Success_(return)
static BOOLEAN PhpHwDecodeWindowBandwidth(
    _In_ FLOAT DataRate,
    _Out_ PFLOAT ReadBandwidth,
    _Out_ PFLOAT WriteBandwidth
    )
{
    FLOAT readBandwidth = 0.0f;
    FLOAT writeBandwidth = 0.0f;
    BOOLEAN valid = FALSE;

    for (ULONG i = 0; i < PhpHwWindowCount; i++)
    {
        PPHP_HW_WINDOW window = &PhpHwWindows[i];
        ULONG64 reads = 0;
        ULONG64 writes = 0;
        BOOLEAN counted = FALSE;
        FLOAT seconds;

        if (
            PhpHwFrequency.QuadPart == 0 ||
            window->LastSampleTime.QuadPart == 0 ||
            window->SampleTime.QuadPart <= window->LastSampleTime.QuadPart
            )
        {
            continue;
        }

        seconds = (FLOAT)(window->SampleTime.QuadPart - window->LastSampleTime.QuadPart) / (FLOAT)PhpHwFrequency.QuadPart;

        // The first sample after a gap is dropped.
        if (seconds > PHP_HW_IMC_MAX_INTERVAL)
            return FALSE;

        for (PHP_HW_WINDOW_REG reg = 0; reg < PhpHwWinMaximum; reg++)
        {
            FLOAT ceiling;
            ULONG64 delta;

            if (!PhpHwWindowRegInfo[reg].Imc || !(window->Valid & window->LastValid & (1ULL << reg)))
                continue;

            delta = window->Value[reg] - window->LastValue[reg];

            if (PhpHwWindowRegInfo[reg].Width == 4)
            {
                // 32-bit counters are DDR3/DDR4 parts.
                ceiling = (DataRate > 0.0f ? DataRate : PHP_HW_IMC_DATA_RATE_BOUND_32) * 1000000.0f * PHP_HW_IMC_BYTES_PER_TRANSFER_BOUND;

                if (seconds >= 4294967296.0f * PHP_HW_IMC_COUNT_BYTES / ceiling)
                    return FALSE;

                delta = (ULONG)delta;
            }
            else
            {
                ceiling = (DataRate > 0.0f ? DataRate : PHP_HW_IMC_DATA_RATE_BOUND) * 1000000.0f * PHP_HW_IMC_BYTES_PER_TRANSFER_BOUND;
            }

            if ((FLOAT)delta * PHP_HW_IMC_COUNT_BYTES / seconds > ceiling)
                return FALSE;

            if (PhpHwWindowRegInfo[reg].Imc == 'R')
                reads += delta;
            else
                writes += delta;

            counted = TRUE;
        }

        if (!counted)
            continue;

        readBandwidth += (FLOAT)reads * PHP_HW_IMC_COUNT_BYTES / seconds / 1000000.0f;
        writeBandwidth += (FLOAT)writes * PHP_HW_IMC_COUNT_BYTES / seconds / 1000000.0f;
        valid = TRUE;
    }

    if (!valid)
        return FALSE;

    *ReadBandwidth = readBandwidth;
    *WriteBandwidth = writeBandwidth;
    return TRUE;
}

/**
 * Decodes an MMIO power limit dword (limit [14:0], enable bit 15).
 *
 * \return The limit in watts, or 0 when disabled or the unit is unknown.
 */
static FLOAT PhpHwDecodeWindowPowerLimit(
    _In_ PHP_HW_WINDOW_REG Reg
    )
{
    FLOAT unit = PhpHwMmioPowerUnit > 0.0f ? PhpHwMmioPowerUnit : PhpHwPowerUnit;
    ULONG value;

    if (unit <= 0.0f || !PhpHwWindowValue(Reg, &value) || !(value & 0x8000))
        return 0.0f;

    return (FLOAT)(value & 0x7FFF) * unit;
}

/**
 * Decodes the current render clock of GT_PERF_STATUS: [15:8] in 50 MHz steps before Skylake, unslice ratio [16:8]
 * from Skylake, unslice ratio [19:11] from Ice Lake, both in 50/3 MHz steps. A GT in RC6 reads 0.
 */
static FLOAT PhpHwDecodeGtClock(
    _In_ ULONG GtPerfStatus
    )
{
    static CONST UCHAR legacyModels[] = { 0x2A, 0x3A, 0x3C, 0x45, 0x46, 0x3D, 0x47 }; // SNB..BDW client, the rows that read it
    static CONST UCHAR gen11Models[] = { 0x7D, 0x7E, 0x9D, 0x8C, 0x8D, 0xA7, 0xD7, 0x97, 0x9A, 0xB7, 0xBA, 0xBF }; // ICL..RPL

    if (PhpHwIdentity.DisplayFamily == 0x06)
    {
        for (ULONG i = 0; i < RTL_NUMBER_OF(legacyModels); i++)
        {
            if (PhpHwIdentity.DisplayModel == legacyModels[i])
                return (FLOAT)((GtPerfStatus >> 8) & 0xFF) * 50.0f;
        }

        for (ULONG i = 0; i < RTL_NUMBER_OF(gen11Models); i++)
        {
            if (PhpHwIdentity.DisplayModel == gen11Models[i])
                return (FLOAT)((GtPerfStatus >> 11) & 0x1FF) * 50.0f / 3.0f;
        }
    }

    return (FLOAT)((GtPerfStatus >> 8) & 0x1FF) * 50.0f / 3.0f;
}

/**
 * Converts the PACKAGE_RAPL_PERF_STATUS delta (1/1024 s units) to the percent of the interval RAPL limited the package.
 */
_Success_(return)
static BOOLEAN PhpHwDecodeWindowRaplLimited(
    _Out_ PFLOAT Percent
    )
{
    for (ULONG i = 0; i < PhpHwWindowCount; i++)
    {
        PPHP_HW_WINDOW window = &PhpHwWindows[i];
        FLOAT seconds;
        FLOAT percent;

        if (!(window->Valid & window->LastValid & (1ULL << PhpHwWinRaplPerfStatus)))
            continue;
        if (PhpHwFrequency.QuadPart == 0 || window->SampleTime.QuadPart <= window->LastSampleTime.QuadPart)
            return FALSE;

        seconds = (FLOAT)(window->SampleTime.QuadPart - window->LastSampleTime.QuadPart) / (FLOAT)PhpHwFrequency.QuadPart;
        percent = (FLOAT)(ULONG)(window->Value[PhpHwWinRaplPerfStatus] - window->LastValue[PhpHwWinRaplPerfStatus]) / 1024.0f / seconds * 100.0f;

        if (percent > 100.5f)
            return FALSE;

        *Percent = min(percent, 100.0f);
        return TRUE;
    }

    return FALSE;
}

/**
 * Decodes the window registers of the tick into package 0 and the chipset temperature.
 */
static VOID PhpHwDecodeWindows(
    VOID
    )
{
    PPHP_HW_PACKAGE package = &PhpHwPackages[0];
    PPH_HW_PACKAGE_SENSOR sensor = &PhpHwPackageSensors[0];
    ULONG value;
    FLOAT mhz;
    FLOAT dataRate;
    FLOAT readBandwidth;
    FLOAT writeBandwidth;
    FLOAT celsius;
    ULONG gear;

    PhpHwChipsetTemperatureValid = FALSE;

    if (PhpHwDecodeWindowMemoryClock(&mhz, &dataRate, &gear))
    {
        sensor->MemoryClockMhz = mhz;
        sensor->MemoryDataRate = dataRate;
        sensor->MemoryGear = gear;
        package->TickValid |= PH_HW_CAP_MEMORY_CLOCK;
    }
    else
    {
        dataRate = 0.0f;
    }

    if (PhpHwDecodeWindowBandwidth(dataRate, &readBandwidth, &writeBandwidth))
    {
        sensor->MemoryReadBandwidth = readBandwidth;
        sensor->MemoryWriteBandwidth = writeBandwidth;
        package->TickValid |= PH_HW_CAP_MEMORY_BANDWIDTH;
    }

    if (PhpHwWindowValue(PhpHwWinPackageTemperature, &value) && PhIntelMmioTemperatureToCelsius(value, &celsius))
    {
        package->TickMmioTemperature = celsius;
        package->TickMmioTemperatureValid = TRUE;
    }

    if (PhpHwWindowValue(PhpHwWinPp0Temperature, &value) && PhIntelMmioTemperatureToCelsius(value, &celsius))
    {
        sensor->CoresTemperature = celsius;
        package->TickValid |= PH_HW_CAP_CORES_TEMP;
    }

    if (PhpHwWindowValue(PhpHwWinPp1Temperature, &value) && PhIntelMmioTemperatureToCelsius(value, &celsius))
    {
        sensor->GraphicsTemperature = celsius;
        package->TickValid |= PH_HW_CAP_GRAPHICS_TEMP;
    }

    package->TickMmioPowerLimit1 = PhpHwDecodeWindowPowerLimit(PhpHwWinPowerLimit1);
    package->TickMmioPowerLimit2 = PhpHwDecodeWindowPowerLimit(PhpHwWinPowerLimit2);

    if (PhpHwWindowValue(PhpHwWinPchTemp, &value) && PhIntelPchTemperatureToCelsius((USHORT)value, &celsius))
    {
        PhpHwChipsetTemperature = celsius;
        PhpHwChipsetTemperatureValid = TRUE;
    }

    if (PhpHwGtNamedIndex != ULONG_MAX)
    {
        BOOLEAN valid = PhpHwWindowValue(PhpHwWinGtPerfStatus, &value);
        FLOAT mhz = 0.0f;

        if (valid)
        {
            PhpHwGtPerfStatusLast = (ULONG)value;
            mhz = PhpHwDecodeGtClock((ULONG)value);

            if (mhz >= PhpHwDecodeGtClock(PhpHwGtPerfStatusPeak))
                PhpHwGtPerfStatusPeak = (ULONG)value;

            // RP0 bounds the render clock; above it the decode is wrong for the part.
            if (PhpHwGtCaps && mhz > (FLOAT)((PhpHwGtCaps & 0xFF) * 50))
                valid = FALSE;
        }

        PhpHwSetNamedSensor(PhpHwGtNamedIndex, valid, mhz);
    }

    if (PhpHwRaplLimitedNamedIndex != ULONG_MAX)
    {
        FLOAT percent = 0.0f;
        BOOLEAN valid = PhpHwDecodeWindowRaplLimited(&percent);

        PhpHwSetNamedSensor(PhpHwRaplLimitedNamedIndex, valid, percent);
    }
}

// MCHBAR memory controller register layouts.
typedef enum _PHP_HW_INTEL_TIMINGS_FORMAT
{
    PhpHwIntelTimingsNone,
    PhpHwIntelTimingsSnb, // Sandy Bridge, Ivy Bridge
    PhpHwIntelTimingsHsw, // Haswell, Broadwell
    PhpHwIntelTimingsSkl, // Skylake through Comet Lake
    PhpHwIntelTimingsIcl, // Ice Lake, Rocket Lake
    PhpHwIntelTimingsTglU, // Tiger Lake U
    PhpHwIntelTimingsTglH, // Tiger Lake H
    PhpHwIntelTimingsAdl, // Alder Lake, Raptor Lake
    PhpHwIntelTimingsMtl, // Meteor, Arrow, Panther and Wildcat Lake
    PhpHwIntelTimingsLnl // Lunar Lake: the Meteor Lake layout without SC_GS_CFG
} PHP_HW_INTEL_TIMINGS_FORMAT;

#define PHP_HW_INTEL_TIMINGS_REGISTERS 5 // TC_PRE, SC_GS_CFG, TC_RFTP, TC_ODT, TC_ACT

/**
 * Determines the memory controller register layout of the processor model.
 */
static PHP_HW_INTEL_TIMINGS_FORMAT PhpHwIntelTimingsFormat(
    VOID
    )
{
    if (PhpHwIdentity.DisplayFamily != 0x06)
        return PhpHwIntelTimingsNone;

    switch (PhpHwIdentity.DisplayModel)
    {
    case 0x2a:
    case 0x3a:
        return PhpHwIntelTimingsSnb;
    case 0x3c:
    case 0x45:
    case 0x46:
    case 0x3d:
    case 0x47:
        return PhpHwIntelTimingsHsw;
    case 0x4e:
    case 0x5e:
    case 0x8e:
    case 0x9e:
    case 0xa5:
    case 0xa6:
        return PhpHwIntelTimingsSkl;
    case 0x7d:
    case 0x7e:
    case 0xa7:
        return PhpHwIntelTimingsIcl;
    case 0x8c:
        return PhpHwIntelTimingsTglU;
    case 0x8d:
        return PhpHwIntelTimingsTglH;
    case 0x97:
    case 0x9a:
    case 0xb7:
    case 0xba:
    case 0xbf:
        return PhpHwIntelTimingsAdl;
    case 0xaa:
    case 0xac:
    case 0xb5:
    case 0xc5:
    case 0xc6:
    case 0xcc:
    case 0xd5:
        return PhpHwIntelTimingsMtl;
    case 0xbd:
        return PhpHwIntelTimingsLnl;
    }

    return PhpHwIntelTimingsNone;
}

/**
 * Decodes the timings of one channel; Registers holds TC_PRE, SC_GS_CFG, TC_RFTP, TC_ODT and TC_ACT
 * in the layout of the format (unused entries are zero).
 *
 * \return FALSE when the values are implausible, as on an unpopulated channel.
 */
_Success_(return)
static BOOLEAN PhpHwDecodeIntelTimings(
    _In_ PHP_HW_INTEL_TIMINGS_FORMAT Format,
    _In_reads_(PHP_HW_INTEL_TIMINGS_REGISTERS) const ULONG64* Registers,
    _Out_ PPH_HW_MEMORY_TIMINGS Timings
    )
{
    ULONG64 pre = Registers[0];
    ULONG64 config = Registers[1];
    ULONG64 refresh = Registers[2];
    ULONG64 odt = Registers[3];
    ULONG64 act = Registers[4];
    PH_HW_MEMORY_TIMINGS timings;
    ULONG stretch;

    memset(&timings, 0, sizeof(PH_HW_MEMORY_TIMINGS));

    switch (Format)
    {
    case PhpHwIntelTimingsSnb:
        timings.RasToCasDelay = (ULONG)(pre & 0xF);
        timings.RowPrecharge = (ULONG)((pre >> 4) & 0xF);
        timings.CasLatency = (ULONG)((pre >> 8) & 0xF);
        timings.RowActive = (ULONG)((pre >> 16) & 0xFF);
        timings.RefreshCycle = (ULONG)((refresh >> 16) & 0x1FF);
        stretch = (ULONG)((config >> 30) & 0x3);
        timings.CommandRate = stretch <= 1 ? 1 : stretch;
        break;
    case PhpHwIntelTimingsHsw:
        timings.RasToCasDelay = (ULONG)(pre & 0x1F);
        timings.RowPrecharge = (ULONG)((pre >> 5) & 0x1F);
        timings.RowActive = (ULONG)((pre >> 10) & 0x3F);
        timings.CasLatency = (ULONG)(odt & 0x1F);
        timings.RefreshCycle = (ULONG)((refresh >> 16) & 0x1FF);
        stretch = (ULONG)((config >> 30) & 0x3);
        timings.CommandRate = stretch <= 1 ? 1 : stretch;
        break;
    case PhpHwIntelTimingsSkl:
        if ((config & 0x3) == 0x3) // SC_GS_CFG DRAM type: 0 DDR4, 1 DDR3, 2 LPDDR3
            return FALSE;

        timings.RowPrecharge = (ULONG)(pre & 0x3F);
        timings.RasToCasDelay = timings.RowPrecharge;
        timings.RowActive = (ULONG)((pre >> 8) & 0x7F);
        timings.CasLatency = (ULONG)((odt >> 16) & 0x1F);
        timings.RefreshCycle = (ULONG)((refresh >> 16) & 0x3FF);
        stretch = (ULONG)((config >> 2) & 0x3);
        timings.CommandRate = stretch == 3 ? 0 : stretch + 1;
        break;
    case PhpHwIntelTimingsIcl:
        timings.RowPrecharge = (ULONG)(pre & 0x3F);
        timings.RasToCasDelay = timings.RowPrecharge;
        timings.RowActive = (ULONG)((pre >> 9) & 0x7F);
        timings.CasLatency = (ULONG)((odt >> 16) & 0x3F);
        timings.RefreshCycle = (ULONG)((refresh >> 16) & 0x3FF);
        stretch = (ULONG)((config >> 3) & 0x3);
        timings.CommandRate = stretch == 3 ? 0 : stretch + 1;
        break;
    case PhpHwIntelTimingsTglU:
    case PhpHwIntelTimingsTglH:
        timings.RowPrecharge = (ULONG)(pre & 0x7F);
        timings.RowActive = (ULONG)((pre >> 33) & 0xFF);
        timings.RasToCasDelay = (ULONG)((pre >> 41) & 0x7F);
        timings.CasLatency = (ULONG)((odt >> 16) & 0x7F);
        timings.RefreshCycle = (ULONG)((refresh >> 17) & 0xFFF);
        stretch = (ULONG)((config >> 3) & 0x3);
        timings.CommandRate = stretch == 3 ? 0 : stretch + 1;
        break;
    case PhpHwIntelTimingsAdl:
        timings.RowPrecharge = (ULONG)(pre & 0xFF);
        timings.RowActive = (ULONG)((pre >> 42) & 0x1FF);
        timings.RasToCasDelay = (ULONG)((pre >> 51) & 0xFF);
        timings.CasLatency = (ULONG)((odt >> 16) & 0x7F);
        timings.RefreshCycle = (ULONG)((refresh >> 18) & 0x1FFF);
        stretch = (ULONG)((config >> 3) & 0x3);
        timings.CommandRate = stretch == 3 ? 0 : stretch + 1;
        break;
    case PhpHwIntelTimingsMtl:
    case PhpHwIntelTimingsLnl:
        timings.RowPrecharge = (ULONG)(pre & 0xFF);
        timings.RowActive = (ULONG)((pre >> 45) & 0x1FF);
        timings.RasToCasDelay = (ULONG)((act >> 22) & 0xFF);
        timings.CasLatency = (ULONG)((odt >> 16) & 0x7F);
        timings.RefreshCycle = (ULONG)((refresh >> 18) & 0x1FFF);
        timings.CommandRate = (config & (1ULL << 3)) ? 2 : 1;
        break;
    default:
        return FALSE;
    }

    // An unpopulated channel can keep plausible reset values; the TGL TC_PRE default decodes to 8-8-28.
    if ((Format == PhpHwIntelTimingsTglU || Format == PhpHwIntelTimingsTglH) && pre == 0x103803083008)
        return FALSE;

    if (
        timings.RefreshCycle == 0 ||
        timings.CasLatency < 4 || timings.CasLatency > 120 ||
        timings.RasToCasDelay < 4 || timings.RasToCasDelay > 200 ||
        timings.RowPrecharge < 4 || timings.RowPrecharge > 200 ||
        timings.RowActive < timings.RasToCasDelay
        )
    {
        return FALSE;
    }

    *Timings = timings;
    return TRUE;
}

/**
 * Reads the timings of the first populated channel through the host bridge window.
 */
static VOID PhpHwReadIntelTimings(
    _In_ PPHP_HW_WINDOW Window
    )
{
    static CONST USHORT offsets[][PHP_HW_INTEL_TIMINGS_REGISTERS] =
    {
        { 0 }, // None
        { 0x000, 0x004, 0x298, 0, 0 }, // Snb: TC_DBP, TC_RAP, TC_RFTP
        { 0x000, 0x004, 0x298, 0x014, 0 }, // Hsw: TC_BANK, TC_BANK_RANK_A, TC_RFTP, TC_BANK_RANK_D
        { 0x000, 0x01c, 0x23c, 0x070, 0 }, // Skl
        { 0x000, 0x088, 0x23c, 0x070, 0 }, // Icl
        { 0x000, 0x088, 0x23c, 0x070, 0 }, // TglU
        { 0x000, 0x088, 0x43c, 0x070, 0 }, // TglH
        { 0x000, 0x088, 0x43c, 0x070, 0 }, // Adl
        { 0x000, 0x088, 0x4a0, 0x070, 0x138 }, // Mtl
        { 0x000, 0, 0x4a0, 0x070, 0x138 }, // Lnl
    };
    PHP_HW_INTEL_TIMINGS_FORMAT format = PhpHwIntelTimingsFormat();
    PPHP_HW_PACKAGE package = &PhpHwPackages[0];
    KPH_HW_WINDOW_BATCH_ENTRY entries[4 * PHP_HW_INTEL_TIMINGS_REGISTERS];
    ULONG channels[4];
    ULONG channelCount;
    ULONG count = 0;
    ULONG base;
    ULONG stride;

    package->MemoryTimingsValid = FALSE;

    if (format == PhpHwIntelTimingsNone || !PhpHwPackageCount)
        return;

    C_ASSERT(RTL_NUMBER_OF(offsets) == PhpHwIntelTimingsLnl + 1);

    if (format >= PhpHwIntelTimingsTglH)
    {
        base = 0xE000;
        stride = 0x800;
    }
    else
    {
        base = 0x4000;
        stride = 0x400;
    }

    // Channels 0 and 1 of the first controller, then of the second at +0x10000.
    channels[0] = base;
    channels[1] = base + stride;
    channels[2] = 0x10000 + base;
    channels[3] = 0x10000 + base + stride;
    channelCount = format >= PhpHwIntelTimingsTglU ? 4 : 2;

    for (ULONG i = 0; i < channelCount; i++)
    {
        // A register the layout does not use (offset 0 past TC_PRE) is not sent.
        for (ULONG j = 0; j < PHP_HW_INTEL_TIMINGS_REGISTERS; j++)
        {
            PKPH_HW_WINDOW_BATCH_ENTRY entry;

            if (j && !offsets[format][j])
                continue;

            entry = &entries[count++];
            memset(entry, 0, sizeof(KPH_HW_WINDOW_BATCH_ENTRY));
            entry->Offset = channels[i] + offsets[format][j];
            entry->Width = (j == 0 && format >= PhpHwIntelTimingsTglU) ? 8 : 4;
            entry->Status = PHP_HW_ENTRY_UNSET;
        }
    }

    // A whole-batch failure leaves every entry unset, and an unset entry fails its channel.
    KphHwReadWindowBatch(&Window->Locator, 0, entries, count);

    for (ULONG i = 0, e = 0; i < channelCount; i++)
    {
        ULONG64 registers[PHP_HW_INTEL_TIMINGS_REGISTERS] = { 0 };
        BOOLEAN valid = TRUE;

        for (ULONG j = 0; j < PHP_HW_INTEL_TIMINGS_REGISTERS; j++)
        {
            if (j && !offsets[format][j])
                continue;

            if (NT_SUCCESS(entries[e].Status))
                registers[j] = entries[e].Value;
            else
                valid = FALSE;

            e++;
        }

        if (valid && PhpHwDecodeIntelTimings(format, registers, &package->MemoryTimings))
        {
            package->MemoryTimingsValid = TRUE;
            break;
        }
    }

    PhTraceInfo("Hardware sensors: Intel memory timings format %lu, valid %lu", (ULONG)format, (ULONG)package->MemoryTimingsValid);
}

/**
 * Reads MEMSS_PMA_CR_MEM_CONFIG and the MAD registers through the host bridge window. The counters of an IMC that
 * MEM_CONFIG shows disabled are dropped.
 */
static VOID PhpHwReadIntelMemoryConfig(
    _In_ PPHP_HW_WINDOW Window
    )
{
    static CONST PHP_HW_WINDOW_REG imcRegisters[2][7] =
    {
        {
            PhpHwWinImc0Reads, PhpHwWinImc0Writes, PhpHwWinImc0ReadsAlt, PhpHwWinImc0WritesAlt, PhpHwWinImc0Requests,
            PhpHwWinMc0Ch0RankTemperature, PhpHwWinMc0Ch1RankTemperature
        },
        {
            PhpHwWinImc1Reads, PhpHwWinImc1Writes, PhpHwWinImc1ReadsAlt, PhpHwWinImc1WritesAlt, PhpHwWinImc1Requests,
            PhpHwWinMc1Ch0RankTemperature, PhpHwWinMc1Ch1RankTemperature
        },
    };
    KPH_HW_WINDOW_BATCH_ENTRY entries[2 + 2 * 3];

    memset(entries, 0, sizeof(entries));
    entries[0].Offset = PH_INTEL_MCHBAR_MEMSS_MEM_CONFIG;
    entries[1].Offset = PH_INTEL_MCHBAR_MEMSS_MEM_CONFIG_NVL;

    for (ULONG i = 0; i < 2; i++)
    {
        entries[2 + i * 3].Offset = i * PH_INTEL_MCHBAR_IMC_STRIDE + PH_INTEL_MCHBAR_MAD_INTER_CHANNEL;
        entries[3 + i * 3].Offset = i * PH_INTEL_MCHBAR_IMC_STRIDE + PH_INTEL_MCHBAR_MAD_DIMM;
        entries[4 + i * 3].Offset = i * PH_INTEL_MCHBAR_IMC_STRIDE + PH_INTEL_MCHBAR_MAD_DIMM + 4;
    }

    for (ULONG i = 0; i < RTL_NUMBER_OF(entries); i++)
    {
        entries[i].Width = 4;
        entries[i].Status = PHP_HW_ENTRY_UNSET;
    }

    // A whole-batch failure leaves every entry unset; a part allow-lists one of the two MEM_CONFIG offsets.
    KphHwReadWindowBatch(&Window->Locator, 0, entries, RTL_NUMBER_OF(entries));

    for (ULONG i = 0; i < 2; i++)
    {
        if (NT_SUCCESS(entries[i].Status) && !(PhpHwMemoryConfig.Valid & 0x1))
        {
            PhpHwMemoryConfig.MemConfig = (ULONG)entries[i].Value;
            PhpHwMemoryConfig.Valid |= 0x1;
        }
    }

    for (ULONG i = 0; i < 2 * 3; i++)
    {
        if (NT_SUCCESS(entries[2 + i].Status))
        {
            PhpHwMemoryConfig.Mad[i / 3][i % 3] = (ULONG)entries[2 + i].Value;
            PhpHwMemoryConfig.Valid |= 0x2UL << i;
        }
    }

    // MC0_EN [0], MC1_EN [1]; neither set is not a configuration.
    if ((PhpHwMemoryConfig.Valid & 0x1) && (PhpHwMemoryConfig.MemConfig & 0x3))
    {
        for (ULONG i = 0; i < 2; i++)
        {
            if (PhpHwMemoryConfig.MemConfig & (1UL << i))
                continue;

            for (ULONG j = 0; j < RTL_NUMBER_OF(imcRegisters[i]); j++)
                PhpHwDisableWindowRegister(imcRegisters[i][j]);
        }
    }

    PhTraceInfo("Hardware sensors: Intel memory configuration 0x%08lx, valid 0x%lx", PhpHwMemoryConfig.MemConfig, PhpHwMemoryConfig.Valid);
}

/**
 * Finds the Intel host bridge, processor-thermal and PCH thermal windows and probes each register once.
 */
static VOID PhpHwProbeWindows(
    _In_ const KPH_HW_CAPABILITIES* Capabilities
    )
{
    static CONST PH_STRINGREF hostBridgeClass = PH_STRINGREF_INIT(L"PCI\\CC_0600");
    static CONST PH_STRINGREF signalProcessingClass = PH_STRINGREF_INIT(L"PCI\\CC_1180");
    ULONG value;
    FLOAT mhz;
    FLOAT dataRate;
    FLOAT celsius;
    ULONG gear;

    PhpHwWindowCount = 0;
    PhpHwMmioPowerUnit = 0.0f;
    PhpHwChipsetTemperatureValid = FALSE;
    PhpHwGtCaps = 0;
    PhpHwGtRpe = 0;
    PhpHwGtPerfStatusLast = 0;
    PhpHwGtPerfStatusPeak = 0;
    memset(&PhpHwMemoryConfig, 0, sizeof(PHP_HW_INTEL_MEMORY_CONFIG));

    if (!PhpHwWindowsSupported(Capabilities))
        return;

    if (!PhpHwWindowsEnumerated)
    {
        PhpHwWindowsEnumerated = TRUE;

        // The host bridge first; the processor-thermal device mirrors part of its MCHBAR.
        PhpHwWindowLocatorCount = PhpHwFindPciDevices(
            PHP_HW_INTEL_VENDOR_ID,
            PHP_HW_INTEL_VENDOR_ID,
            &hostBridgeClass,
            NULL,
            0,
            TRUE,
            PhpHwWindowLocators,
            1
            );
        PhpHwWindowHostBridge = PhpHwWindowLocatorCount != 0;
        PhpHwWindowLocatorCount += PhpHwFindPciDevices(
            PHP_HW_INTEL_VENDOR_ID,
            PHP_HW_INTEL_VENDOR_ID,
            &signalProcessingClass,
            NULL,
            0,
            FALSE,
            &PhpHwWindowLocators[PhpHwWindowLocatorCount],
            PHP_HW_MAX_WINDOWS - PhpHwWindowLocatorCount
            );

        PhTraceInfo("Hardware sensors: %lu window candidates", PhpHwWindowLocatorCount);
    }

    for (ULONG i = 0; i < PhpHwWindowLocatorCount; i++)
    {
        PPHP_HW_WINDOW window = &PhpHwWindows[PhpHwWindowCount];

        memset(window, 0, sizeof(PHP_HW_WINDOW));
        window->Locator = PhpHwWindowLocators[i];

        for (PHP_HW_WINDOW_REG reg = 0; reg < PhpHwWinMaximum; reg++)
        {
            if (PhpHwWindowRegisterValid(reg))
                window->Registers[reg].Disabled = TRUE;
        }

        PhpHwReadWindow(window);

        if (!window->Item.Disabled)
            PhpHwWindowCount++;
    }

    if (PhpHwWindowCount == 0)
        return;

    PhpHwSmbiosDataRate = 0;
    PhEnumSMBIOS(PhpHwSmbiosMemoryCallback, &PhpHwSmbiosDataRate);

    // The host bridge is the first candidate.
    if (PhpHwWindowHostBridge && RtlEqualMemory(&PhpHwWindows[0].Locator, &PhpHwWindowLocators[0], sizeof(KPH_HW_PCI_LOCATOR)))
    {
        PhpHwReadIntelMemoryConfig(&PhpHwWindows[0]);
        PhpHwReadIntelTimings(&PhpHwWindows[0]);
    }

    if (!PhpHwDecodeWindowMemoryClock(&mhz, &dataRate, &gear))
    {
        PhpHwDisableWindowRegister(PhpHwWinMcBiosReq);
        PhpHwDisableWindowRegister(PhpHwWinMcBiosData);
        PhpHwDisableWindowRegister(PhpHwWinSaPerfStatus);
        PhpHwDisableWindowRegister(PhpHwWinBclkFreq);
        PhpHwDisableWindowRegister(PhpHwWinMemssBiosData);
    }

    // The MMIO package temperature and TjMax stand in for the MSRs only.
    if (PhpHwRegisterActive(PhpHwRegThermStatus) || PhpHwRegisterActive(PhpHwRegPackageThermStatus))
    {
        PhpHwDisableWindowRegister(PhpHwWinPackageTemperature);
    }
    else if (PhpHwWindowValue(PhpHwWinTemperatureTarget, &value))
    {
        ULONG tjMax = PhIntelTemperatureTargetTjMax(value);

        if (tjMax && PhpHwPackageSensors[0].TjMax <= 0.0f)
            PhpHwPackageSensors[0].TjMax = (FLOAT)tjMax;
    }

    PhpHwDisableWindowRegister(PhpHwWinTemperatureTarget);

    if (!PhpHwWindowValue(PhpHwWinPackageTemperature, &value) || !PhIntelMmioTemperatureToCelsius(value, &celsius))
        PhpHwDisableWindowRegister(PhpHwWinPackageTemperature);
    if (!PhpHwWindowValue(PhpHwWinPp0Temperature, &value) || !PhIntelMmioTemperatureToCelsius(value, &celsius))
        PhpHwDisableWindowRegister(PhpHwWinPp0Temperature);
    if (!PhpHwWindowValue(PhpHwWinPp1Temperature, &value) || !PhIntelMmioTemperatureToCelsius(value, &celsius))
        PhpHwDisableWindowRegister(PhpHwWinPp1Temperature);

    // The power unit is constant; the limits change at runtime (DTT).
    if (PhpHwWindowValue(PhpHwWinPowerUnit, &value))
        PhpHwMmioPowerUnit = PhRaplPowerUnitWatts(value, &PhpHwIdentity);

    PhpHwDisableWindowRegister(PhpHwWinPowerUnit);

    if (PhpHwMmioPowerUnit <= 0.0f && PhpHwPowerUnit <= 0.0f)
    {
        PhpHwDisableWindowRegister(PhpHwWinPowerLimit1);
        PhpHwDisableWindowRegister(PhpHwWinPowerLimit2);
    }

    // The sensor is enabled by firmware (TSEL.ETS); it is never enabled here.
    if (
        !PhpHwWindowValue(PhpHwWinPchTsel, &value) || !(value & 0x1) ||
        !PhpHwWindowValue(PhpHwWinPchTemp, &value) || !PhIntelPchTemperatureToCelsius((USHORT)value, &celsius)
        )
    {
        PhpHwDisableWindowRegister(PhpHwWinPchTemp);
    }

    PhpHwDisableWindowRegister(PhpHwWinPchTsel);

    // RP_STATE_CAP RP0 of 0 or FFh: no integrated graphics (dGPU-only SKU or iGPU disabled).
    if (PhpHwWindowValue(PhpHwWinRpStateCap, &value) && (value & 0xFF) && (value & 0xFF) != 0xFF)
    {
        PhpHwGtCaps = value;

        if (PhpHwWindowValue(PhpHwWinFreqInfo, &value))
            PhpHwGtRpe = value;
    }
    else
    {
        PhpHwDisableWindowRegister(PhpHwWinGtPerfStatus);
    }

    PhpHwDisableWindowRegister(PhpHwWinRpStateCap);
    PhpHwDisableWindowRegister(PhpHwWinFreqInfo);

    PhTraceInfo("Hardware sensors: %lu windows", PhpHwWindowCount);
}

/**
 * Adds a TPMI or MSR uncore named sensor. Called on the provider thread.
 *
 * \return The index of the sensor, or ULONG_MAX when the table is full.
 */
static ULONG PhpHwAddNamedSensor(
    _In_ PH_HW_NAMED_SENSOR_KIND Kind,
    _In_ ULONG PackageIndex,
    _In_ ULONG Id,
    _In_ PPH_STRING Name
    )
{
    PPH_HW_NAMED_SENSOR sensor;
    SIZE_T count;
    ULONG index;

    PhAcquireQueuedLockExclusive(&PhpHwNamedSensorLock);

    if (PhpHwNamedSensorCount < PHP_HW_MAX_NAMED_SENSORS)
    {
        index = PhpHwNamedSensorCount++;
        sensor = &PhpHwNamedSensors[index];
        memset(sensor, 0, sizeof(PH_HW_NAMED_SENSOR));
        sensor->Kind = Kind;
        sensor->PackageIndex = PackageIndex;
        sensor->Id = Id;
        sensor->Active = TRUE;
        count = min(Name->Length / sizeof(WCHAR), PH_HW_NAMED_SENSOR_NAME_LENGTH - 1);
        memcpy(sensor->Name, Name->Buffer, count * sizeof(WCHAR));
    }
    else
    {
        index = ULONG_MAX;
    }

    PhReleaseQueuedLockExclusive(&PhpHwNamedSensorLock);

    return index;
}

static VOID PhpHwSetNamedSensor(
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

static BOOLEAN PhpHwSmbusSupported(
    _In_ const KPH_HW_CAPABILITIES* Capabilities
    )
{
    return FlagOn(Capabilities->Flags, KPH_HW_CAP_PCI_AVAILABLE) &&
        FlagOn(Capabilities->ImplementedKinds, 1ULL << KPH_DYN_HW_KIND_SMBUS_READ);
}

typedef enum _PHP_HW_SMBUS_RESULT
{
    PhpHwSmbusOk,
    PhpHwSmbusAbsent, // no acknowledge: an empty slot
    PhpHwSmbusBusy, // another agent holds the host or the bus
    PhpHwSmbusNotReady, // the host stopped decoding or moved
    PhpHwSmbusTimeout, // our transaction was killed; the slave is never asked again this session
    PhpHwSmbusRefused // not requested again this session
} PHP_HW_SMBUS_RESULT;

static PHP_HW_SMBUS_RESULT PhpHwSmbusClassify(
    _In_ NTSTATUS Status
    )
{
    if (NT_SUCCESS(Status))
        return PhpHwSmbusOk;

    switch (Status)
    {
    case STATUS_NO_SUCH_DEVICE:
        return PhpHwSmbusAbsent;
    case STATUS_RETRY:
        return PhpHwSmbusBusy;
    case STATUS_DEVICE_NOT_READY:
        return PhpHwSmbusNotReady;
    case STATUS_IO_TIMEOUT:
        return PhpHwSmbusTimeout;
    }

    return PhpHwSmbusRefused;
}

static VOID PhpHwInitializeSmbusEntry(
    _Out_ PKPH_HW_SMBUS_BATCH_ENTRY Entry,
    _In_ UCHAR Address,
    _In_ UCHAR Command,
    _In_ UCHAR Protocol
    )
{
    memset(Entry, 0, sizeof(KPH_HW_SMBUS_BATCH_ENTRY));
    Entry->Address = Address;
    Entry->Command = Command;
    Entry->Protocol = Protocol;
    Entry->Status = PHP_HW_ENTRY_UNSET;
}

static BOOLEAN PhpHwDimmSessionCurrent(
    _In_ ULONG RequestId
    )
{
    BOOLEAN current;

    PhAcquireQueuedLockShared(&PhpHwNamedSensorLock);
    current = PhpHwSmbusRequested && PhpHwSmbusRequestId == RequestId;
    PhReleaseQueuedLockShared(&PhpHwNamedSensorLock);

    return current;
}

/**
 * Sends an SMBus batch; a message-level failure becomes the status of every entry.
 *
 * \return FALSE when the session ended; the entries are not sent.
 */
static BOOLEAN PhpHwSendSmbusBatch(
    _In_ ULONG RequestId,
    _In_ PKPH_HW_PCI_LOCATOR Locator,
    _In_ UCHAR OpIndex,
    _Inout_updates_(Count) PKPH_HW_SMBUS_BATCH_ENTRY Entries,
    _In_ ULONG Count
    )
{
    NTSTATUS status;
    LARGE_INTEGER timeout;

    if (!PhpHwDimmSessionCurrent(RequestId))
        return FALSE;

    status = KphHwReadSmbusBatch(Locator, OpIndex, Entries, Count, !PhpHwIntel, PhTimeoutFromMilliseconds(&timeout, PHP_HW_SMBUS_TIMEOUT_MS));

    for (ULONG i = 0; i < Count; i++)
    {
        if (Entries[i].Status == PHP_HW_ENTRY_UNSET && !NT_SUCCESS(status))
            Entries[i].Status = status;
    }

    return TRUE;
}

/**
 * Decodes a JC-42.4 temperature: [12:0] two's complement in 1/16 degrees Celsius, read byte-swapped.
 */
static FLOAT PhpHwTsodToCelsius(
    _In_ USHORT Value
    )
{
    LONG raw = _byteswap_ushort(Value) & 0x1FFF;

    if (raw & 0x1000)
        raw -= 0x2000;

    return (FLOAT)raw * 0.0625f;
}

/**
 * Decodes the SPD5118 MR49:MR50 temperature: [12:2] two's complement in 1/4 degrees Celsius.
 */
static FLOAT PhpHwSpd5118ToCelsius(
    _In_ USHORT Value
    )
{
    LONG raw = (Value >> 2) & 0x7FF;

    if (raw & 0x400)
        raw -= 0x800;

    return (FLOAT)raw * 0.25f;
}

static BOOLEAN PhpHwDimmCelsiusValid(
    _In_ FLOAT Celsius
    )
{
    return Celsius >= PHP_HW_DIMM_MIN_CELSIUS && Celsius <= PHP_HW_DIMM_MAX_CELSIUS;
}

static UCHAR PhpHwDimmAddress(
    _In_ PHP_HW_DIMM_TYPE Type,
    _In_range_(0, PHP_HW_DIMM_COUNT - 1) ULONG Slot
    )
{
    return (UCHAR)((Type == PhpHwDimmTsod ? PHP_HW_TSOD_ADDRESS : PHP_HW_SPD_ADDRESS) + Slot);
}

typedef struct _PHP_HW_DIMM_PROBE_STATE
{
    BOOLEAN Busy; // a transaction timed out
    BOOLEAN Contended; // another agent held the host or the bus
    BOOLEAN NotReady; // the host stopped decoding or moved: the host is passed over
} PHP_HW_DIMM_PROBE_STATE, *PPHP_HW_DIMM_PROBE_STATE;

/**
 * Applies the status of one probe entry; a timed-out address is recorded in *TimedOut.
 */
static BOOLEAN PhpHwProbeSmbusEntry(
    _In_ const KPH_HW_SMBUS_BATCH_ENTRY* Entry,
    _In_range_(0, PHP_HW_DIMM_COUNT - 1) ULONG Slot,
    _Inout_ PUCHAR TimedOut,
    _Inout_ PPHP_HW_DIMM_PROBE_STATE State
    )
{
    switch (PhpHwSmbusClassify(Entry->Status))
    {
    case PhpHwSmbusOk:
        return TRUE;
    case PhpHwSmbusBusy:
        State->Contended = TRUE;
        break;
    case PhpHwSmbusNotReady:
        State->NotReady = TRUE;
        break;
    case PhpHwSmbusTimeout:
        *TimedOut |= (UCHAR)(1 << Slot);
        State->Busy = TRUE; // the remaining entries of the request were not run
        break;
    }

    return FALSE;
}

static BOOLEAN PhpHwDimmProbeStopped(
    _In_ const PHP_HW_DIMM_PROBE_STATE* State
    )
{
    return State->Busy || State->Contended || State->NotReady;
}

static PHP_HW_DIMM_PROBE PhpHwDimmProbeOutcome(
    _In_ const PHP_HW_DIMM_PROBE_STATE* State
    )
{
    if (State->NotReady)
        return PhpHwDimmProbeNone;
    if (State->Busy)
        return PhpHwDimmProbeRetry;

    return PhpHwDimmProbeContended;
}

static UCHAR PhpHwPmicRegister(
    _In_reads_(PHP_HW_PMIC_REPORT_REGISTERS) const UCHAR* Registers,
    _In_ UCHAR Command
    )
{
    for (ULONG i = 0; i < PHP_HW_PMIC_REPORT_REGISTERS; i++)
    {
        if (PhpHwPmicCommands[i] == Command)
            return Registers[i];
    }

    assert(FALSE);
    return 0;
}

/**
 * Decides whether a PMIC encodes the VDD and VDDQ set points in the whole byte (800 + 5n mV, a vendor high-voltage
 * mode) instead of the JEDEC [7:1].
 */
static BOOLEAN PhpHwPmicHighVoltage(
    _In_reads_(PHP_HW_PMIC_REPORT_REGISTERS) const UCHAR* Registers
    )
{
    UCHAR select = PhpHwPmicRegister(Registers, 0x30);
    UCHAR input = (select >> 3) & 0xF;
    UCHAR vid;
    FLOAT measured;
    FLOAT jedec;
    FLOAT wide;

    // Richtek: 2Bh[5:4] = 11b.
    if (PhpHwPmicRegister(Registers, 0x3C) == 0x8A && PhpHwPmicRegister(Registers, 0x3D) == 0x8C)
        return (PhpHwPmicRegister(Registers, 0x2B) & 0x30) == 0x30;

    // Otherwise only an ADC another agent left enabled on SWA or SWB tells; the decode closer to it wins.
    if (!(select & 0x80) || (input != 0 && input != 2))
        return FALSE;

    vid = PhpHwPmicRegister(Registers, input == 0 ? 0x21 : 0x25);
    measured = (FLOAT)PhpHwPmicRegister(Registers, 0x31) * 0.015f;
    jedec = 0.8f + (FLOAT)((vid >> 1) & 0x7F) * 0.005f - measured;
    wide = 0.8f + (FLOAT)vid * 0.005f - measured;

    return wide * wide < jedec * jedec;
}

/**
 * Decodes the PMIC telemetry: SWA/SWB/SWC current, or power when 1Bh[6], or the module total in 0Ch when also
 * 1Ah[1]; the die temperature is a bucket, published only from 85 C.
 */
static VOID PhpHwDecodePmicTelemetry(
    _In_ UCHAR Swa,
    _In_ UCHAR Swb,
    _In_ UCHAR Swc,
    _In_ UCHAR PowerMode,
    _In_ UCHAR TelemetryMode,
    _In_ UCHAR Temperature,
    _Out_writes_(PHP_HW_PMIC_SENSORS) PBOOLEAN Valid,
    _Out_writes_(PHP_HW_PMIC_SENSORS) PFLOAT Values
    )
{
    static CONST FLOAT buckets[8] = { 0.0f, 85.0f, 95.0f, 105.0f, 115.0f, 125.0f, 135.0f, 140.0f };
    FLOAT rails[PH_HW_PMIC_RAILS];
    UCHAR bucket = Temperature >> 5;

    memset(Valid, 0, PHP_HW_PMIC_SENSORS * sizeof(BOOLEAN));
    memset(Values, 0, PHP_HW_PMIC_SENSORS * sizeof(FLOAT));

    // 125 mA or 125 mW steps; SWB and SWC are 6 bits; a zero 0Ch is undefined (the meter is off), and so is the snapshot.
    rails[0] = (FLOAT)Swa * 0.125f;
    rails[1] = (FLOAT)(Swb & 0x3F) * 0.125f;
    rails[2] = (FLOAT)(Swc & 0x3F) * 0.125f;

    if (!(TelemetryMode & 0x40))
    {
        for (ULONG rail = 0; rail < PH_HW_PMIC_RAILS; rail++)
        {
            Valid[PHP_HW_PMIC_SENSOR_CURRENT + rail] = !!Swa;
            Values[PHP_HW_PMIC_SENSOR_CURRENT + rail] = rails[rail];
        }
    }
    else if (PowerMode & 0x02)
    {
        Valid[PHP_HW_PMIC_SENSOR_POWER + PH_HW_PMIC_RAILS] = !!Swa;
        Values[PHP_HW_PMIC_SENSOR_POWER + PH_HW_PMIC_RAILS] = rails[0];
    }
    else
    {
        for (ULONG rail = 0; rail < PH_HW_PMIC_RAILS; rail++)
        {
            Valid[PHP_HW_PMIC_SENSOR_POWER + rail] = rail || Swa;
            Values[PHP_HW_PMIC_SENSOR_POWER + rail] = rails[rail];
        }
    }

    if (bucket)
    {
        Valid[PHP_HW_PMIC_SENSOR_TEMPERATURE] = TRUE;
        Values[PHP_HW_PMIC_SENSOR_TEMPERATURE] = buckets[bucket];
    }
}

/**
 * Reads the DDR5 PMIC registers of the slots whose SPD5118 hub answered, and their set points, one batch per slot.
 * A busy or not-ready bus is retried up to PHP_HW_PMIC_TRIES batches; a PMIC that still does not answer, or timed
 * out, has no set points or telemetry for the session.
 *
 * \return FALSE when another agent still held the bus; the caller repeats the probe.
 */
static BOOLEAN PhpHwProbePmics(
    _In_ ULONG RequestId,
    _In_ PKPH_HW_PCI_LOCATOR Locator
    )
{
    static CONST FLOAT bases[PH_HW_PMIC_RAILS] = { 0.8f, 0.8f, 1.5f }; // VDD, VDDQ, VPP
    static CONST UCHAR setPoints[PH_HW_PMIC_RAILS] = { 0x21, 0x25, 0x27 }; // SWA, SWB, SWC
    KPH_HW_SMBUS_BATCH_ENTRY entries[PHP_HW_PMIC_REPORT_REGISTERS];

    PhpHwDimmVoltageMask = 0;

    for (ULONG i = 0; i < PHP_HW_DIMM_COUNT; i++)
        PhpHwDimms[i].Pmic = FALSE;

    for (ULONG slot = 0; slot < PHP_HW_DIMM_COUNT; slot++)
    {
        UCHAR registers[PHP_HW_PMIC_REPORT_REGISTERS];
        NTSTATUS status = STATUS_SUCCESS;
        ULONG tries = 0;
        BOOLEAN busy;
        BOOLEAN contended;
        BOOLEAN timedOut;
        BOOLEAN highVoltage;

        if (PhpHwDimms[slot].Type != PhpHwDimmSpd5118 || (PhpHwSmbusTimedOutPmic & (1 << slot)))
            continue;

        do
        {
            LARGE_INTEGER interval;

            if (tries)
                NtDelayExecution(FALSE, PhTimeoutFromMilliseconds(&interval, PHP_HW_SMBUS_TIMEOUT_MS));

            for (ULONG j = 0; j < PHP_HW_PMIC_REPORT_REGISTERS; j++)
                PhpHwInitializeSmbusEntry(&entries[j], (UCHAR)(PHP_HW_PMIC_ADDRESS + slot), PhpHwPmicCommands[j], KPH_DYN_HW_SMBUS_PROTOCOL_BYTE_DATA);

            if (!PhpHwSendSmbusBatch(RequestId, Locator, PHP_HW_SMBUS_OP_PMIC, entries, PHP_HW_PMIC_REPORT_REGISTERS))
                return TRUE;

            busy = FALSE;
            contended = FALSE;
            timedOut = FALSE;

            // After a timeout the kernel fails the rest of the batch with STATUS_RETRY; the timeout decides.
            for (ULONG j = 0; j < PHP_HW_PMIC_REPORT_REGISTERS; j++)
            {
                switch (PhpHwSmbusClassify(entries[j].Status))
                {
                case PhpHwSmbusBusy:
                    contended = TRUE;
                    break;
                case PhpHwSmbusNotReady:
                    busy = TRUE;
                    break;
                case PhpHwSmbusTimeout:
                    timedOut = TRUE;
                    break;
                }
            }
        } while ((busy || contended) && !timedOut && ++tries < PHP_HW_PMIC_TRIES);

        if (timedOut)
        {
            // A PMIC that hangs the bus is never asked again this session.
            PhpHwSmbusTimedOutPmic |= (UCHAR)(1 << slot);
            status = STATUS_IO_TIMEOUT;
        }
        else if (contended)
        {
            // Contention does not use up the set points.
            return FALSE;
        }
        else
        {
            for (ULONG j = 0; j < PHP_HW_PMIC_REPORT_REGISTERS && NT_SUCCESS(status); j++)
                status = entries[j].Status;
        }

        for (ULONG j = 0; j < PHP_HW_PMIC_REPORT_REGISTERS; j++)
            registers[j] = (UCHAR)entries[j].Value;

        PhAcquireQueuedLockExclusive(&PhpHwNamedSensorLock);

        if (PhpHwSmbusRequested && PhpHwSmbusRequestId == RequestId)
        {
            memcpy(PhpHwPmicReportRegisters[slot], registers, sizeof(registers));
            PhpHwPmicReportMask |= (UCHAR)(1 << slot);
            PhpHwPmicReportStatus[slot] = status;
        }

        PhReleaseQueuedLockExclusive(&PhpHwNamedSensorLock);

        if (!NT_SUCCESS(status))
        {
            PhTraceInfo("Hardware sensors: PMIC 0x%02X: %!STATUS!", PHP_HW_PMIC_ADDRESS + slot, status);
            continue;
        }

        highVoltage = PhpHwPmicHighVoltage(registers);

        PhTraceInfo(
            "Hardware sensors: PMIC 0x%02X: R21 0x%02X, R25 0x%02X, R27 0x%02X, R2B 0x%02X, vendor 0x%02X%02X, high voltage %u",
            PHP_HW_PMIC_ADDRESS + slot,
            PhpHwPmicRegister(registers, 0x21),
            PhpHwPmicRegister(registers, 0x25),
            PhpHwPmicRegister(registers, 0x27),
            PhpHwPmicRegister(registers, 0x2B),
            PhpHwPmicRegister(registers, 0x3C),
            PhpHwPmicRegister(registers, 0x3D),
            highVoltage
            );

        for (ULONG rail = 0; rail < PH_HW_PMIC_RAILS; rail++)
        {
            UCHAR vid = PhpHwPmicRegister(registers, setPoints[rail]);

            // JEDEC set point: [7:1] in 5 mV steps above the rail base; VPP has no high-voltage mode.
            if (highVoltage && rail != PHP_HW_PMIC_RAIL_VPP)
                PhpHwDimmVoltages[slot][rail] = bases[rail] + (FLOAT)vid * 0.005f;
            else
                PhpHwDimmVoltages[slot][rail] = bases[rail] + (FLOAT)((vid >> 1) & 0x7F) * 0.005f;

            PhpHwDimmVoltageMask |= 1UL << (slot * PH_HW_PMIC_RAILS + rail);
        }

        PhpHwDimms[slot].Pmic = TRUE;
        memset(&PhpHwDimms[slot].PmicItem, 0, sizeof(PHP_HW_ITEM));
    }

    return TRUE;
}

/**
 * Finds the DIMM thermal sensors on an SMBus host.
 *
 * \param Reports Receives what was read of each SPD, for the report.
 * \return PhpHwDimmProbeRetry when a transaction timed out, PhpHwDimmProbeContended when another
 * agent held the host or the bus.
 */
static PHP_HW_DIMM_PROBE PhpHwProbeDimmSensorsOnHost(
    _In_ ULONG RequestId,
    _In_ PKPH_HW_PCI_LOCATOR Locator,
    _Inout_updates_(PHP_HW_DIMM_COUNT) PPHP_HW_SPD_REPORT Reports
    )
{
    static CONST UCHAR tsodCommands[5] = { 0x00, 0x01, 0x06, 0x07, 0x05 }; // CAP, CONFIG, MANID, DEVID, TEMP
    static CONST UCHAR nvmBytes[PHP_HW_SPD_NVM_BYTES] = { 2, 3, 20, 21, 30, 31, 32, 33, 34, 35, 36, 37, 38, 39 };
    KPH_HW_SMBUS_BATCH_ENTRY entries[PHP_HW_DIMM_COUNT * PHP_HW_SPD_NVM_BYTES];
    ULONG slots[PHP_HW_DIMM_COUNT];
    BOOLEAN present[PHP_HW_DIMM_COUNT] = { 0 };
    BOOLEAN ddr5[PHP_HW_DIMM_COUNT] = { 0 };
    BOOLEAN noTsod[PHP_HW_DIMM_COUNT] = { 0 };
    USHORT identity[PHP_HW_DIMM_COUNT] = { 0 };
    PHP_HW_DIMM_PROBE_STATE state = { 0 };
    BOOLEAN spdAnswered = FALSE;
    BOOLEAN found = FALSE;
    ULONG count;

    C_ASSERT(PHP_HW_SPD_NVM_BYTES >= 5);

    for (ULONG i = 0; i < PHP_HW_DIMM_COUNT; i++)
    {
        PhpHwDimms[i].Type = PhpHwDimmNone;
        PhpHwDimms[i].Pmic = FALSE;
    }

    PhpHwDimmVoltageMask = 0;

    // An SPD5118 hub reads 51h, 18h from MR0:MR1; a DDR3/DDR4 EEPROM answers with SPD bytes 0 and 1.
    count = 0;

    for (ULONG i = 0; i < PHP_HW_DIMM_COUNT; i++)
    {
        if (PhpHwSmbusTimedOutSpd & (1 << i))
            continue;

        PhpHwInitializeSmbusEntry(&entries[count], (UCHAR)(PHP_HW_SPD_ADDRESS + i), 0x00, KPH_DYN_HW_SMBUS_PROTOCOL_WORD_DATA);
        slots[count++] = i;
    }

    if (count && !PhpHwSendSmbusBatch(RequestId, Locator, PHP_HW_SMBUS_OP_SPD, entries, count))
        return PhpHwDimmProbeNone;

    for (ULONG i = 0; i < count; i++)
    {
        if (PhpHwProbeSmbusEntry(&entries[i], slots[i], &PhpHwSmbusTimedOutSpd, &state))
        {
            present[slots[i]] = TRUE;
            identity[slots[i]] = entries[i].Value;
            ddr5[slots[i]] = entries[i].Value == 0x1851;
            spdAnswered = TRUE;
        }
        else if (PhpHwSmbusClassify(entries[i].Status) == PhpHwSmbusAbsent)
        {
            spdAnswered = TRUE;
        }
    }

    if (PhpHwDimmProbeStopped(&state))
        return PhpHwDimmProbeOutcome(&state);

    // A zero identity is an SPD5118 hub left on a non-zero NVM page when MR3:MR4 is zero too (MR11[2:0] the page):
    // it is never moved, and neither it nor a TSOD is read. A DDR4 SPD says in byte 14 bit 7 whether it has a
    // thermal sensor; the byte is trusted only when byte 2 says DDR4, as the bus may be on page 1.
    count = 0;

    for (ULONG i = 0; i < PHP_HW_DIMM_COUNT; i++)
    {
        UCHAR address = (UCHAR)(PHP_HW_SPD_ADDRESS + i);

        if (!present[i] || ddr5[i])
            continue;

        if (!identity[i])
        {
            PhpHwInitializeSmbusEntry(&entries[count * 2 + 0], address, 0x03, KPH_DYN_HW_SMBUS_PROTOCOL_WORD_DATA);
            PhpHwInitializeSmbusEntry(&entries[count * 2 + 1], address, 0x0B, KPH_DYN_HW_SMBUS_PROTOCOL_BYTE_DATA);
        }
        else
        {
            PhpHwInitializeSmbusEntry(&entries[count * 2 + 0], address, 0x02, KPH_DYN_HW_SMBUS_PROTOCOL_BYTE_DATA);
            PhpHwInitializeSmbusEntry(&entries[count * 2 + 1], address, 0x0E, KPH_DYN_HW_SMBUS_PROTOCOL_BYTE_DATA);
        }

        slots[count++] = i;
    }

    if (count)
    {
        if (!PhpHwSendSmbusBatch(RequestId, Locator, PHP_HW_SMBUS_OP_SPD, entries, count * 2))
            return PhpHwDimmProbeNone;

        for (ULONG i = 0; i < count; i++)
        {
            PKPH_HW_SMBUS_BATCH_ENTRY entry = &entries[i * 2];
            PPHP_HW_SPD_REPORT report = &Reports[slots[i]];

            BOOLEAN valid = PhpHwProbeSmbusEntry(&entry[0], slots[i], &PhpHwSmbusTimedOutSpd, &state);

            if (!PhpHwProbeSmbusEntry(&entry[1], slots[i], &PhpHwSmbusTimedOutSpd, &state) || !valid)
                continue;

            if (!identity[slots[i]])
            {
                report->HubRead = TRUE;
                report->Mr11 = (UCHAR)entry[1].Value;
                report->Locked = !entry[0].Value && (entry[1].Value & 0x07);
                noTsod[slots[i]] = report->Locked;
            }
            else
            {
                report->Ddr4Read = TRUE;
                report->DramType = (UCHAR)entry[0].Value;
                report->Ddr4Thermal = (UCHAR)entry[1].Value;
                noTsod[slots[i]] = (report->DramType == 0x0C || report->DramType == 0x0E) && !(report->Ddr4Thermal & 0x80);
            }
        }

        if (PhpHwDimmProbeStopped(&state))
            return PhpHwDimmProbeOutcome(&state);
    }

    // DDR5: MR5 bit 1 is the thermal sensor capability, MR26 bit 0 disables the sensor, MR49:MR50 is the temperature;
    // MR11 is the NVM page and addressing mode, MR18 bit 5 I3C mode.
    count = 0;

    for (ULONG i = 0; i < PHP_HW_DIMM_COUNT; i++)
    {
        UCHAR address = (UCHAR)(PHP_HW_SPD_ADDRESS + i);

        if (!ddr5[i])
            continue;

        PhpHwInitializeSmbusEntry(&entries[count * 5 + 0], address, 0x05, KPH_DYN_HW_SMBUS_PROTOCOL_BYTE_DATA);
        PhpHwInitializeSmbusEntry(&entries[count * 5 + 1], address, 0x1A, KPH_DYN_HW_SMBUS_PROTOCOL_BYTE_DATA);
        PhpHwInitializeSmbusEntry(&entries[count * 5 + 2], address, 0x31, KPH_DYN_HW_SMBUS_PROTOCOL_WORD_DATA);
        PhpHwInitializeSmbusEntry(&entries[count * 5 + 3], address, 0x0B, KPH_DYN_HW_SMBUS_PROTOCOL_BYTE_DATA);
        PhpHwInitializeSmbusEntry(&entries[count * 5 + 4], address, 0x12, KPH_DYN_HW_SMBUS_PROTOCOL_BYTE_DATA);
        slots[count++] = i;
    }

    if (count)
    {
        ULONG nvmCount = 0;
        ULONG nvmSlots[PHP_HW_DIMM_COUNT];

        if (!PhpHwSendSmbusBatch(RequestId, Locator, PHP_HW_SMBUS_OP_SPD, entries, count * 5))
            return PhpHwDimmProbeNone;

        for (ULONG i = 0; i < count; i++)
        {
            PKPH_HW_SMBUS_BATCH_ENTRY entry = &entries[i * 5];
            PPHP_HW_SPD_REPORT report = &Reports[slots[i]];
            BOOLEAN valid = TRUE;

            for (ULONG j = 0; j < 5; j++)
            {
                if (!PhpHwProbeSmbusEntry(&entry[j], slots[i], &PhpHwSmbusTimedOutSpd, &state))
                    valid = FALSE;
            }

            if (!valid)
                continue;

            report->HubRead = TRUE;
            report->Mr18Read = TRUE;
            report->Mr11 = (UCHAR)entry[3].Value;
            report->Mr18 = (UCHAR)entry[4].Value;

            if (
                (entry[0].Value & 0x02) &&
                !(entry[1].Value & 0x01) &&
                PhpHwDimmCelsiusValid(PhpHwSpd5118ToCelsius(entry[2].Value))
                )
            {
                PhpHwDimms[slots[i]].Type = PhpHwDimmSpd5118;
                found = TRUE;
            }

            // NVM page 0 is SPD bytes 0-127 at commands 80h-FFh, only with one-byte addressing (MR11[3] clear).
            if (!(report->Mr11 & 0x0F) && !(report->Mr18 & 0x20))
                nvmSlots[nvmCount++] = slots[i];
        }

        if (PhpHwDimmProbeStopped(&state))
            return PhpHwDimmProbeOutcome(&state);

        for (ULONG i = 0; i < nvmCount; i++)
        {
            for (ULONG j = 0; j < PHP_HW_SPD_NVM_BYTES; j++)
                PhpHwInitializeSmbusEntry(&entries[i * PHP_HW_SPD_NVM_BYTES + j], (UCHAR)(PHP_HW_SPD_ADDRESS + nvmSlots[i]), (UCHAR)(0x80 + nvmBytes[j]), KPH_DYN_HW_SMBUS_PROTOCOL_BYTE_DATA);
        }

        if (nvmCount)
        {
            if (!PhpHwSendSmbusBatch(RequestId, Locator, PHP_HW_SMBUS_OP_SPD, entries, nvmCount * PHP_HW_SPD_NVM_BYTES))
                return PhpHwDimmProbeNone;

            for (ULONG i = 0; i < nvmCount; i++)
            {
                PKPH_HW_SMBUS_BATCH_ENTRY entry = &entries[i * PHP_HW_SPD_NVM_BYTES];
                PPHP_HW_SPD_REPORT report = &Reports[nvmSlots[i]];
                BOOLEAN valid = TRUE;

                for (ULONG j = 0; j < PHP_HW_SPD_NVM_BYTES; j++)
                {
                    if (!PhpHwProbeSmbusEntry(&entry[j], nvmSlots[i], &PhpHwSmbusTimedOutSpd, &state))
                        valid = FALSE;

                    report->Nvm[j] = (UCHAR)entry[j].Value;
                }

                report->NvmRead = valid;
            }

            if (PhpHwDimmProbeStopped(&state))
                return PhpHwDimmProbeOutcome(&state);
        }

        if (found && !PhpHwProbePmics(RequestId, Locator))
        {
            state.Contended = TRUE;
            return PhpHwDimmProbeOutcome(&state);
        }
    }

    // DDR3/DDR4: a JC-42.4 sensor at 18h + slot; CAP [15:8] and CONFIG [15:11] are reserved zero.
    // Without an SPD answer (no SPD op) every slot is tried.
    count = 0;

    for (ULONG i = 0; i < PHP_HW_DIMM_COUNT; i++)
    {
        if (ddr5[i] || noTsod[i] || (spdAnswered && !present[i]) || (PhpHwSmbusTimedOutTsod & (1 << i)))
            continue;

        for (ULONG j = 0; j < RTL_NUMBER_OF(tsodCommands); j++)
            PhpHwInitializeSmbusEntry(&entries[count * 5 + j], (UCHAR)(PHP_HW_TSOD_ADDRESS + i), tsodCommands[j], KPH_DYN_HW_SMBUS_PROTOCOL_WORD_DATA);

        slots[count++] = i;
    }

    if (count)
    {
        if (!PhpHwSendSmbusBatch(RequestId, Locator, PHP_HW_SMBUS_OP_TSOD, entries, count * 5))
            return PhpHwDimmProbeNone;

        for (ULONG i = 0; i < count; i++)
        {
            PKPH_HW_SMBUS_BATCH_ENTRY entry = &entries[i * 5];
            BOOLEAN valid = TRUE;
            USHORT manufacturer;

            for (ULONG j = 0; j < 5; j++)
            {
                if (!PhpHwProbeSmbusEntry(&entry[j], slots[i], &PhpHwSmbusTimedOutTsod, &state))
                    valid = FALSE;
            }

            manufacturer = _byteswap_ushort(entry[2].Value);

            if (
                valid &&
                !(_byteswap_ushort(entry[0].Value) & 0xFF00) &&
                !(_byteswap_ushort(entry[1].Value) & 0xF800) &&
                manufacturer != 0 && manufacturer != 0xFFFF &&
                PhpHwDimmCelsiusValid(PhpHwTsodToCelsius(entry[4].Value))
                )
            {
                PhpHwDimms[slots[i]].Type = PhpHwDimmTsod;
                found = TRUE;
            }
        }

        if (PhpHwDimmProbeStopped(&state))
            return PhpHwDimmProbeOutcome(&state);
    }

    return found ? PhpHwDimmProbeFound : PhpHwDimmProbeNone;
}

static BOOLEAN PhpHwSpdReportsAnyRead(
    _In_reads_(PHP_HW_DIMM_COUNT) const PHP_HW_SPD_REPORT* Reports
    )
{
    for (ULONG i = 0; i < PHP_HW_DIMM_COUNT; i++)
    {
        if (Reports[i].Ddr4Read || Reports[i].HubRead)
            return TRUE;
    }

    return FALSE;
}

/**
 * Finds the DIMM thermal sensors on an SMBus host and records what was read of each SPD for the report. Runs on
 * the SMBus worker.
 *
 * \return PhpHwDimmProbeRetry when a transaction timed out, PhpHwDimmProbeContended when another
 * agent held the host or the bus.
 */
static PHP_HW_DIMM_PROBE PhpHwProbeDimmsOnHost(
    _In_ ULONG RequestId,
    _In_ PKPH_HW_PCI_LOCATOR Locator
    )
{
    PHP_HW_SPD_REPORT reports[PHP_HW_DIMM_COUNT];
    PHP_HW_DIMM_PROBE probe;

    memset(reports, 0, sizeof(reports));
    probe = PhpHwProbeDimmSensorsOnHost(RequestId, Locator, reports);

    PhAcquireQueuedLockExclusive(&PhpHwNamedSensorLock);

    // A host without SPD answers keeps what an earlier host showed (a locked hub, say).
    if (PhpHwSmbusRequested && PhpHwSmbusRequestId == RequestId && PhpHwSpdReportsAnyRead(reports))
    {
        memcpy(PhpHwSpdReports, reports, sizeof(PhpHwSpdReports));
        PhpHwSpdReportHost = PhpHwSmbusProbeHost;
    }

    PhReleaseQueuedLockExclusive(&PhpHwNamedSensorLock);

    return probe;
}

typedef struct _PHP_HW_DIMM_LOCATORS
{
    ULONG Count; // populated memory devices
    PPH_STRING Names[PHP_HW_DIMM_COUNT];
    PPH_STRING Banks[PHP_HW_DIMM_COUNT];
} PHP_HW_DIMM_LOCATORS, *PPHP_HW_DIMM_LOCATORS;

_Function_class_(PH_ENUM_SMBIOS_CALLBACK)
static BOOLEAN NTAPI PhpHwSmbiosDimmCallback(
    _In_ ULONG_PTR EnumHandle,
    _In_ UCHAR MajorVersion,
    _In_ UCHAR MinorVersion,
    _In_ PPH_SMBIOS_ENTRY Entry,
    _In_opt_ PVOID Context
    )
{
    PPHP_HW_DIMM_LOCATORS locators = Context;
    PPH_STRING name;

    if (!locators)
        return TRUE;
    if (Entry->Header.Type != SMBIOS_MEMORY_DEVICE_INFORMATION_TYPE || !Entry->MemoryDevice.Size.Value)
        return FALSE;

    if (
        locators->Count < PHP_HW_DIMM_COUNT &&
        PH_SMBIOS_CONTAINS_STRING(Entry, MemoryDevice, DeviceLocator) &&
        NT_SUCCESS(PhGetSMBIOSString(EnumHandle, Entry->MemoryDevice.DeviceLocator, &name))
        )
    {
        locators->Names[locators->Count] = name;

        if (
            PH_SMBIOS_CONTAINS_STRING(Entry, MemoryDevice, BankLocator) &&
            NT_SUCCESS(PhGetSMBIOSString(EnumHandle, Entry->MemoryDevice.BankLocator, &name))
            )
        {
            locators->Banks[locators->Count] = name;
        }
    }

    locators->Count++;

    return FALSE;
}

/**
 * Publishes the found DIMMs as named sensors. SMBIOS locators label them only when the populated
 * memory device count matches the DIMM sensor count.
 */
static VOID PhpHwNameDimms(
    _In_ ULONG RequestId
    )
{
    PHP_HW_DIMM_LOCATORS locators;
    ULONG dimmCount = 0;
    ULONG index = 0;

    for (ULONG i = 0; i < PHP_HW_DIMM_COUNT; i++)
    {
        if (PhpHwDimms[i].Type != PhpHwDimmNone)
            dimmCount++;
    }

    // The populated Type 17 entries are taken in the 50h + slot order (A1, A2, B1, B2).
    memset(&locators, 0, sizeof(PHP_HW_DIMM_LOCATORS));
    PhEnumSMBIOS(PhpHwSmbiosDimmCallback, &locators);

    // Boards that repeat the device locator per channel ("DIMM 1") are told apart by the bank locator.
    for (ULONG i = 0; i < PHP_HW_DIMM_COUNT; i++)
    {
        for (ULONG j = i + 1; j < PHP_HW_DIMM_COUNT && locators.Names[i]; j++)
        {
            if (!locators.Names[j] || !PhEqualString(locators.Names[i], locators.Names[j], TRUE))
                continue;

            for (ULONG k = 0; k < PHP_HW_DIMM_COUNT; k++)
            {
                if (locators.Names[k] && locators.Banks[k] && locators.Banks[k]->Length)
                    PhMoveReference(&locators.Names[k], PhFormatString(L"%s %s", locators.Banks[k]->Buffer, locators.Names[k]->Buffer));
            }

            i = PHP_HW_DIMM_COUNT;
            break;
        }
    }

    PhAcquireQueuedLockExclusive(&PhpHwNamedSensorLock);

    if (PhpHwSmbusRequested && PhpHwSmbusRequestId == RequestId)
    {
        PhpHwDimmReportHostCount = PhpHwSmbusLocatorCount;
        PhpHwDimmReportHost = PhpHwSmbusHost;

        for (ULONG i = 0; i < PHP_HW_DIMM_COUNT; i++)
            PhpHwDimmReportTypes[i] = (UCHAR)PhpHwDimms[i].Type;
    }

    for (ULONG i = 0; i < PHP_HW_DIMM_COUNT; i++)
    {
        static CONST PCWSTR rails[PH_HW_PMIC_RAILS] = { L"VDD", L"VDDQ", L"VPP" };
        PPHP_HW_DIMM dimm = &PhpHwDimms[i];
        PPH_HW_NAMED_SENSOR sensor = &PhpHwDimmSensors[i];
        PPH_HW_NAMED_SENSOR voltage;
        UCHAR address = PhpHwDimmAddress(dimm->Type, i);
        PPH_STRING slot;
        PPH_STRING name;

        dimm->Item.Disabled = FALSE;
        dimm->Item.RetryCount = 0;

        if (dimm->Type == PhpHwDimmNone)
            continue;

        if (locators.Count == dimmCount && locators.Names[index] && locators.Names[index]->Length)
            slot = PhReferenceObject(locators.Names[index]);
        else
            slot = PhFormatString(L"DIMM %lu", i);

        index++;

        if (PhpHwSmbusRequested && PhpHwSmbusRequestId == RequestId)
        {
            name = PhFormatString(L"%s (0x%02X)", slot->Buffer, address);
            memset(sensor, 0, sizeof(PH_HW_NAMED_SENSOR));
            sensor->Kind = PhHwNamedSensorDimmTemperature;
            sensor->Id = i;
            sensor->Active = TRUE;
            memcpy(sensor->Name, name->Buffer, min(name->Length / sizeof(WCHAR), PH_HW_NAMED_SENSOR_NAME_LENGTH - 1) * sizeof(WCHAR));
            PhDereferenceObject(name);

            for (ULONG rail = 0; rail < PH_HW_PMIC_RAILS; rail++)
            {
                ULONG id = i * PH_HW_PMIC_RAILS + rail;

                if (!(PhpHwDimmVoltageMask & (1UL << id)))
                    continue;

                voltage = &PhpHwDimmVoltageSensors[id];
                name = PhFormatString(L"%s %s", slot->Buffer, rails[rail]);
                memset(voltage, 0, sizeof(PH_HW_NAMED_SENSOR));
                voltage->Kind = PhHwNamedSensorDimmVoltage;
                voltage->Id = id;
                voltage->Active = TRUE;
                voltage->Valid = TRUE;
                voltage->Value = PhpHwDimmVoltages[i][rail];
                memcpy(voltage->Name, name->Buffer, min(name->Length / sizeof(WCHAR), PH_HW_NAMED_SENSOR_NAME_LENGTH - 1) * sizeof(WCHAR));
                PhDereferenceObject(name);
            }

            // The PMIC sensors are listed now and become valid on their first reading.
            for (ULONG j = 0; dimm->Pmic && j < PHP_HW_PMIC_SENSORS; j++)
            {
                PPH_HW_NAMED_SENSOR pmic = &PhpHwDimmPmicSensors[i * PHP_HW_PMIC_SENSORS + j];

                memset(pmic, 0, sizeof(PH_HW_NAMED_SENSOR));
                pmic->Active = TRUE;

                if (j < PHP_HW_PMIC_SENSOR_POWER)
                {
                    pmic->Kind = PhHwNamedSensorDimmCurrent;
                    pmic->Id = i * PH_HW_PMIC_RAILS + j;
                    name = PhFormatString(L"%s %s", slot->Buffer, rails[j]);
                }
                else if (j < PHP_HW_PMIC_SENSOR_POWER + PH_HW_PMIC_RAILS)
                {
                    pmic->Kind = PhHwNamedSensorDimmPower;
                    pmic->Id = i * (PH_HW_PMIC_RAILS + 1) + j - PHP_HW_PMIC_SENSOR_POWER;
                    name = PhFormatString(L"%s %s", slot->Buffer, rails[j - PHP_HW_PMIC_SENSOR_POWER]);
                }
                else if (j < PHP_HW_PMIC_SENSOR_TEMPERATURE)
                {
                    pmic->Kind = PhHwNamedSensorDimmPower;
                    pmic->Id = i * (PH_HW_PMIC_RAILS + 1) + PH_HW_PMIC_RAILS;
                    name = PhFormatString(L"%s total", slot->Buffer);
                }
                else
                {
                    pmic->Kind = PhHwNamedSensorDimmPmicTemperature;
                    pmic->Id = i;
                    name = PhFormatString(L"%s PMIC (coarse)", slot->Buffer);
                }

                memcpy(pmic->Name, name->Buffer, min(name->Length / sizeof(WCHAR), PH_HW_NAMED_SENSOR_NAME_LENGTH - 1) * sizeof(WCHAR));
                PhDereferenceObject(name);
            }
        }

        PhDereferenceObject(slot);
    }

    PhReleaseQueuedLockExclusive(&PhpHwNamedSensorLock);

    for (ULONG i = 0; i < PHP_HW_DIMM_COUNT; i++)
    {
        PhClearReference(&locators.Names[i]);
        PhClearReference(&locators.Banks[i]);
    }

    PhTraceInfo("Hardware sensors: %lu DIMM sensors, %lu SMBIOS memory devices", dimmCount, locators.Count);
}

static VOID PhpHwSetDimmSensor(
    _In_ ULONG RequestId,
    _In_range_(0, PHP_HW_DIMM_COUNT - 1) ULONG Slot,
    _In_ BOOLEAN Valid,
    _In_ FLOAT Value
    )
{
    PhAcquireQueuedLockExclusive(&PhpHwNamedSensorLock);

    if (PhpHwSmbusRequested && PhpHwSmbusRequestId == RequestId)
    {
        if (Valid)
            PhpHwDimmSensors[Slot].Value = Value;

        PhpHwDimmSensors[Slot].Valid = Valid;
        PhpHwDimmSensors[Slot].Active = PhpHwDimms[Slot].Type != PhpHwDimmNone && !PhpHwDimms[Slot].Item.Disabled;

        for (ULONG rail = 0; rail < PH_HW_PMIC_RAILS; rail++)
        {
            if (!PhpHwDimmSensors[Slot].Active)
                PhpHwDimmVoltageSensors[Slot * PH_HW_PMIC_RAILS + rail].Active = FALSE;
        }

        for (ULONG i = 0; !PhpHwDimmSensors[Slot].Active && i < PHP_HW_PMIC_SENSORS; i++)
            PhpHwDimmPmicSensors[Slot * PHP_HW_PMIC_SENSORS + i].Active = FALSE;
    }

    PhReleaseQueuedLockExclusive(&PhpHwNamedSensorLock);
}

/**
 * Publishes the PMIC telemetry of a slot; a disabled PMIC drops its sensors.
 */
static VOID PhpHwSetPmicSensors(
    _In_ ULONG RequestId,
    _In_range_(0, PHP_HW_DIMM_COUNT - 1) ULONG Slot,
    _In_reads_opt_(PHP_HW_PMIC_SENSORS) const BOOLEAN* Valid,
    _In_reads_opt_(PHP_HW_PMIC_SENSORS) const FLOAT* Values
    )
{
    PhAcquireQueuedLockExclusive(&PhpHwNamedSensorLock);

    for (ULONG i = 0; PhpHwSmbusRequested && PhpHwSmbusRequestId == RequestId && i < PHP_HW_PMIC_SENSORS; i++)
    {
        PPH_HW_NAMED_SENSOR sensor = &PhpHwDimmPmicSensors[Slot * PHP_HW_PMIC_SENSORS + i];

        if (!sensor->Active)
            continue;

        sensor->Valid = Valid && Values && Valid[i];

        if (sensor->Valid)
            sensor->Value = Values[i];
        if (PhpHwDimms[Slot].PmicItem.Disabled)
            sensor->Active = FALSE;
    }

    PhReleaseQueuedLockExclusive(&PhpHwNamedSensorLock);
}

static VOID PhpHwResetDimms(
    VOID
    )
{
    PhpHwSmbusHost = ULONG_MAX;
    PhpHwSmbusProbeHost = ULONG_MAX;
    PhpHwSmbusStrikes = 0;
    memset(PhpHwDimms, 0, sizeof(PhpHwDimms));
    PhpHwDimmVoltageMask = 0;

    PhAcquireQueuedLockExclusive(&PhpHwNamedSensorLock);
    memset(PhpHwDimmSensors, 0, sizeof(PhpHwDimmSensors));
    memset(PhpHwDimmVoltageSensors, 0, sizeof(PhpHwDimmVoltageSensors));
    memset(PhpHwDimmPmicSensors, 0, sizeof(PhpHwDimmPmicSensors));
    PhpHwDimmReportHostCount = PhpHwSmbusLocatorCount;
    PhpHwDimmReportHost = ULONG_MAX;
    memset(PhpHwDimmReportTypes, 0, sizeof(PhpHwDimmReportTypes));
    PhpHwPmicReportMask = 0;
    memset(PhpHwSpdReports, 0, sizeof(PhpHwSpdReports));
    PhpHwSpdReportHost = ULONG_MAX;
    PhReleaseQueuedLockExclusive(&PhpHwNamedSensorLock);
}

/**
 * Counts a timed-out or not-ready request; PHP_HW_SMBUS_STRIKE_LIMIT in a row end the SMBus for the session.
 */
static VOID PhpHwSmbusStrike(
    VOID
    )
{
    if (++PhpHwSmbusStrikes >= PHP_HW_SMBUS_STRIKE_LIMIT)
    {
        PhTraceInfo("Hardware sensors: SMBus unavailable after %lu failed requests", PhpHwSmbusStrikes);
        PhpHwResetDimms();
    }
}

/**
 * Probes the SMBus hosts in order until one has DIMM sensors.
 */
static VOID PhpHwProbeDimmHosts(
    _In_ ULONG RequestId
    )
{
    while (PhpHwSmbusProbeHost < PhpHwSmbusLocatorCount)
    {
        PHP_HW_DIMM_PROBE probe;

        probe = PhpHwProbeDimmsOnHost(RequestId, &PhpHwSmbusLocators[PhpHwSmbusProbeHost]);

        switch (probe)
        {
        case PhpHwDimmProbeRetry:
        case PhpHwDimmProbeContended:
            // Nothing is kept from a partial probe; it is repeated in full. Contention is not a strike.
            for (ULONG i = 0; i < PHP_HW_DIMM_COUNT; i++)
            {
                PhpHwDimms[i].Type = PhpHwDimmNone;
                PhpHwDimms[i].Pmic = FALSE;
            }

            if (probe == PhpHwDimmProbeRetry)
                PhpHwSmbusStrike();
            return;
        case PhpHwDimmProbeFound:
            PhpHwSmbusHost = PhpHwSmbusProbeHost;
            PhpHwSmbusProbeHost = ULONG_MAX;
            PhpHwSmbusStrikes = 0;
            PhpHwNameDimms(RequestId);
            return;
        }

        if (!PhpHwDimmSessionCurrent(RequestId))
            return;

        // A clean answer without sensors, or a host that is not ready: the next host starts afresh.
        PhpHwSmbusProbeHost++;
        PhpHwSmbusStrikes = 0;
        PhpHwSmbusTimedOutSpd = 0;
        PhpHwSmbusTimedOutTsod = 0;
        PhpHwSmbusTimedOutPmic = 0;
    }

    PhpHwSmbusProbeHost = ULONG_MAX;
}

static VOID PhpHwInvalidatePmics(
    _In_ ULONG RequestId
    )
{
    for (ULONG i = 0; i < PHP_HW_DIMM_COUNT; i++)
    {
        if (PhpHwDimms[i].Pmic)
            PhpHwSetPmicSensors(RequestId, i, NULL, NULL);
    }
}

/**
 * Reads the PMIC telemetry of the DIMMs whose PMIC answered the probe, in one batch.
 */
static VOID PhpHwReadPmics(
    _In_ ULONG RequestId,
    _Inout_ PBOOLEAN Busy,
    _Inout_ PBOOLEAN Contended,
    _Inout_ PBOOLEAN Answered
    )
{
    static CONST UCHAR commands[] = { 0x0C, 0x0E, 0x0F, 0x1A, 0x1B, 0x33 }; // SWA, SWB, SWC, power mode, telemetry mode, temperature
    KPH_HW_SMBUS_BATCH_ENTRY entries[PHP_HW_DIMM_COUNT * RTL_NUMBER_OF(commands)];
    ULONG slots[PHP_HW_DIMM_COUNT];
    ULONG count = 0;

    for (ULONG i = 0; i < PHP_HW_DIMM_COUNT; i++)
    {
        PPHP_HW_DIMM dimm = &PhpHwDimms[i];

        if (!dimm->Pmic || dimm->PmicItem.Disabled || dimm->Type == PhpHwDimmNone || dimm->Item.Disabled)
            continue;

        for (ULONG j = 0; j < RTL_NUMBER_OF(commands); j++)
            PhpHwInitializeSmbusEntry(&entries[count * RTL_NUMBER_OF(commands) + j], (UCHAR)(PHP_HW_PMIC_ADDRESS + i), commands[j], KPH_DYN_HW_SMBUS_PROTOCOL_BYTE_DATA);

        slots[count++] = i;
    }

    if (!count)
        return;

    if (!PhpHwSendSmbusBatch(RequestId, &PhpHwSmbusLocators[PhpHwSmbusHost], PHP_HW_SMBUS_OP_PMIC, entries, count * RTL_NUMBER_OF(commands)))
        return;

    for (ULONG i = 0; i < count; i++)
    {
        PKPH_HW_SMBUS_BATCH_ENTRY entry = &entries[i * RTL_NUMBER_OF(commands)];
        PPHP_HW_DIMM dimm = &PhpHwDimms[slots[i]];
        BOOLEAN valid[PHP_HW_PMIC_SENSORS];
        FLOAT values[PHP_HW_PMIC_SENSORS];
        PHP_HW_SMBUS_RESULT result = PhpHwSmbusOk;
        NTSTATUS status = STATUS_SUCCESS;

        // The most severe result decides; a timeout fails the rest of the batch with STATUS_RETRY.
        for (ULONG j = 0; j < RTL_NUMBER_OF(commands); j++)
        {
            static CONST UCHAR severity[] = { 0, 1, 2, 3, 5, 4 }; // by PHP_HW_SMBUS_RESULT: Ok, Absent, Busy, NotReady, Timeout, Refused
            PHP_HW_SMBUS_RESULT entryResult = PhpHwSmbusClassify(entry[j].Status);

            C_ASSERT(RTL_NUMBER_OF(severity) == PhpHwSmbusRefused + 1);

            if (severity[entryResult] > severity[result])
            {
                result = entryResult;
                status = entry[j].Status;
            }
        }

        switch (result)
        {
        case PhpHwSmbusOk:
            *Answered = TRUE;
            break;
        case PhpHwSmbusAbsent:
            *Answered = TRUE;
            break;
        case PhpHwSmbusBusy:
            // Another agent held the host or the bus: invalid this pass, nothing is counted.
            *Contended = TRUE;
            PhpHwSetPmicSensors(RequestId, slots[i], NULL, NULL);
            continue;
        case PhpHwSmbusNotReady:
            *Busy = TRUE;
            break;
        case PhpHwSmbusTimeout:
            *Busy = TRUE;
            dimm->PmicItem.Disabled = TRUE;
            break;
        case PhpHwSmbusRefused:
            dimm->PmicItem.Disabled = TRUE;
            break;
        }

        if (result == PhpHwSmbusOk)
            dimm->PmicItem.RetryCount = 0;
        else if (!dimm->PmicItem.Disabled && ++dimm->PmicItem.RetryCount >= PHP_HW_RETRY_LIMIT)
            dimm->PmicItem.Disabled = TRUE;

        if (dimm->PmicItem.Disabled)
            PhTraceInfo("Hardware sensor DIMM %lu PMIC unavailable: %!STATUS!", slots[i], status);

        if (result != PhpHwSmbusOk)
        {
            PhpHwSetPmicSensors(RequestId, slots[i], NULL, NULL);
            continue;
        }

        PhpHwDecodePmicTelemetry(
            (UCHAR)entry[0].Value,
            (UCHAR)entry[1].Value,
            (UCHAR)entry[2].Value,
            (UCHAR)entry[3].Value,
            (UCHAR)entry[4].Value,
            (UCHAR)entry[5].Value,
            valid,
            values
            );
        PhpHwSetPmicSensors(RequestId, slots[i], valid, values);
    }
}

/**
 * Reads the DIMM temperatures, one batch per op; a busy or timed-out batch ends the pass.
 */
static VOID PhpHwReadDimms(
    _In_ ULONG RequestId
    )
{
    static CONST struct
    {
        PHP_HW_DIMM_TYPE Type;
        UCHAR OpIndex;
        UCHAR Command;
    } reads[] =
    {
        { PhpHwDimmSpd5118, PHP_HW_SMBUS_OP_SPD, 0x31 },
        { PhpHwDimmTsod, PHP_HW_SMBUS_OP_TSOD, 0x05 },
    };
    KPH_HW_SMBUS_BATCH_ENTRY entries[PHP_HW_DIMM_COUNT];
    ULONG slots[PHP_HW_DIMM_COUNT];
    BOOLEAN answered = FALSE;
    BOOLEAN busy = FALSE;
    BOOLEAN contended = FALSE;
    UCHAR read = 0; // the slots this pass published

    for (ULONG r = 0; r < RTL_NUMBER_OF(reads) && !busy && !contended; r++)
    {
        ULONG count = 0;

        for (ULONG i = 0; i < PHP_HW_DIMM_COUNT; i++)
        {
            if (PhpHwDimms[i].Type != reads[r].Type || PhpHwDimms[i].Item.Disabled)
                continue;

            PhpHwInitializeSmbusEntry(&entries[count], PhpHwDimmAddress(reads[r].Type, i), reads[r].Command, KPH_DYN_HW_SMBUS_PROTOCOL_WORD_DATA);
            slots[count++] = i;
        }

        if (!count)
            continue;

        if (!PhpHwSendSmbusBatch(RequestId, &PhpHwSmbusLocators[PhpHwSmbusHost], reads[r].OpIndex, entries, count))
            return;

        for (ULONG i = 0; i < count; i++)
        {
            PPHP_HW_DIMM dimm = &PhpHwDimms[slots[i]];
            BOOLEAN valid = FALSE;
            FLOAT celsius = 0.0f;

            switch (PhpHwSmbusClassify(entries[i].Status))
            {
            case PhpHwSmbusOk:
                answered = TRUE;
                celsius = dimm->Type == PhpHwDimmSpd5118 ? PhpHwSpd5118ToCelsius(entries[i].Value) : PhpHwTsodToCelsius(entries[i].Value);
                valid = PhpHwDimmCelsiusValid(celsius);
                break;
            case PhpHwSmbusAbsent:
                answered = TRUE;
                break;
            case PhpHwSmbusBusy:
                // Another agent held the host or the bus: the value is invalid this pass, nothing is counted.
                contended = TRUE;
                read |= (UCHAR)(1 << slots[i]);
                PhpHwSetDimmSensor(RequestId, slots[i], FALSE, 0.0f);
                continue;
            case PhpHwSmbusNotReady:
                busy = TRUE;
                break;
            case PhpHwSmbusTimeout:
                // The kernel killed our transaction; a slave that hangs the bus is never asked again.
                busy = TRUE;
                dimm->Item.Disabled = TRUE;
                break;
            case PhpHwSmbusRefused:
                dimm->Item.Disabled = TRUE;
                break;
            }

            // A sensor that stops acknowledging, is not ready or reads out of range is dropped after PHP_HW_RETRY_LIMIT reads.
            if (valid)
                dimm->Item.RetryCount = 0;
            else if (!dimm->Item.Disabled && ++dimm->Item.RetryCount >= PHP_HW_RETRY_LIMIT)
                dimm->Item.Disabled = TRUE;

            if (dimm->Item.Disabled)
                PhTraceInfo("Hardware sensor DIMM %lu unavailable: %!STATUS!", slots[i], entries[i].Status);

            read |= (UCHAR)(1 << slots[i]);
            PhpHwSetDimmSensor(RequestId, slots[i], valid, celsius);
        }
    }

    if (!busy && !contended)
        PhpHwReadPmics(RequestId, &busy, &contended, &answered);
    else
        PhpHwInvalidatePmics(RequestId);

    // A contended pass ends early; the slots it did not reach are invalid, not stale.
    for (ULONG i = 0; contended && i < PHP_HW_DIMM_COUNT; i++)
    {
        if (PhpHwDimms[i].Type != PhpHwDimmNone && !PhpHwDimms[i].Item.Disabled && !(read & (1 << i)))
            PhpHwSetDimmSensor(RequestId, i, FALSE, 0.0f);
    }

    if (busy)
        PhpHwSmbusStrike();
    else if (answered)
        PhpHwSmbusStrikes = 0;
}

static BOOLEAN PhpHwDimmsRemaining(
    VOID
    )
{
    for (ULONG i = 0; i < PHP_HW_DIMM_COUNT; i++)
    {
        if (PhpHwDimms[i].Type != PhpHwDimmNone && !PhpHwDimms[i].Item.Disabled)
            return TRUE;
    }

    return FALSE;
}

/**
 * Probes and reads the DIMM sensors every PHP_HW_DIMM_INTERVAL_MS. Runs at normal priority: a request
 * holds the SMBus mutex and the kernel SMBus lock.
 */
_Function_class_(USER_THREAD_START_ROUTINE)
static NTSTATUS NTAPI PhpHwSmbusWorker(
    _In_ PVOID Parameter
    )
{
    ULONG requestId = 0;
    BOOLEAN active = FALSE;

    PhSetThreadName(NtCurrentThread(), L"HardwareSmbus");
    PhSetThreadBasePriority(NtCurrentThread(), THREAD_PRIORITY_NORMAL);

    while (TRUE)
    {
        LARGE_INTEGER timeout;
        BOOLEAN requested;
        ULONG id;

        NtWaitForSingleObject(
            PhpHwSmbusEvent,
            FALSE,
            active ? PhTimeoutFromMilliseconds(&timeout, PHP_HW_DIMM_INTERVAL_MS) : NULL
            );

        PhAcquireQueuedLockShared(&PhpHwNamedSensorLock);
        requested = PhpHwSmbusRequested;
        id = PhpHwSmbusRequestId;
        PhReleaseQueuedLockShared(&PhpHwNamedSensorLock);

        if (!requested)
        {
            if (active)
                PhpHwResetDimms();

            active = FALSE;
            continue;
        }

        if (id != requestId)
        {
            requestId = id;
            PhpHwResetDimms();
            PhpHwSmbusTimedOutSpd = 0;
            PhpHwSmbusTimedOutTsod = 0;
            PhpHwSmbusTimedOutPmic = 0;

            if (!PhpHwSmbusEnumerated)
            {
                static CONST PH_STRINGREF smbusClass = PH_STRINGREF_INIT(L"PCI\\CC_0C05");

                // Every SMBus function is listed; the driver refuses one without a dynamic data row.
                PhpHwSmbusEnumerated = TRUE;
                PhpHwSmbusLocatorCount = PhpHwFindPciDevices(
                    PHP_HW_ANY_VENDOR_ID,
                    PHP_HW_ANY_VENDOR_ID,
                    &smbusClass,
                    NULL,
                    0,
                    FALSE,
                    PhpHwSmbusLocators,
                    RTL_NUMBER_OF(PhpHwSmbusLocators)
                    );

                PhTraceInfo("Hardware sensors: %lu SMBus hosts", PhpHwSmbusLocatorCount);

                PhAcquireQueuedLockExclusive(&PhpHwNamedSensorLock);
                PhpHwDimmReportHostCount = PhpHwSmbusLocatorCount;
                PhReleaseQueuedLockExclusive(&PhpHwNamedSensorLock);
            }

            PhpHwSmbusProbeHost = PhpHwSmbusLocatorCount ? 0 : ULONG_MAX;
            active = TRUE;
        }

        if (!active)
            continue;

        if (PhpHwSmbusProbeHost != ULONG_MAX)
            PhpHwProbeDimmHosts(requestId);
        else if (PhpHwSmbusHost != ULONG_MAX)
            PhpHwReadDimms(requestId);

        if (PhpHwSmbusHost != ULONG_MAX && !PhpHwDimmsRemaining())
        {
            PhTraceInfo("Hardware sensors: no DIMM sensor left");
            PhpHwResetDimms();
        }

        if (PhpHwSmbusProbeHost == ULONG_MAX && PhpHwSmbusHost == ULONG_MAX)
            active = FALSE;
    }

    return STATUS_SUCCESS;
}

static VOID PhpHwSignalDimms(
    _In_ BOOLEAN Requested
    )
{
    PhAcquireQueuedLockExclusive(&PhpHwNamedSensorLock);
    PhpHwSmbusRequested = Requested;

    if (Requested)
        PhpHwSmbusRequestId++;

    PhReleaseQueuedLockExclusive(&PhpHwNamedSensorLock);

    if (PhpHwSmbusEvent)
        NtSetEvent(PhpHwSmbusEvent, NULL);
}

static BOOLEAN PhpHwRequestDimms(
    _In_ const KPH_HW_CAPABILITIES* Capabilities
    )
{
    if (!PhpHwSmbusSupported(Capabilities))
    {
        PhpHwSignalDimms(FALSE);
        return FALSE;
    }

    if (!PhpHwSmbusEvent)
    {
        if (!NT_SUCCESS(NtCreateEvent(&PhpHwSmbusEvent, EVENT_ALL_ACCESS, NULL, SynchronizationEvent, FALSE)))
            return FALSE;

        if (!NT_SUCCESS(PhCreateThread2(PhpHwSmbusWorker, NULL)))
        {
            NtClose(PhpHwSmbusEvent);
            PhpHwSmbusEvent = NULL;
            return FALSE;
        }
    }

    PhpHwSignalDimms(TRUE);
    return TRUE;
}

static VOID PhpHwCancelDimms(
    VOID
    )
{
    PhpHwSignalDimms(FALSE);
}

static BOOLEAN PhpHwDimmsActive(
    VOID
    )
{
    BOOLEAN active = FALSE;

    PhAcquireQueuedLockShared(&PhpHwNamedSensorLock);

    for (ULONG i = 0; i < PHP_HW_DIMM_COUNT; i++)
    {
        if (PhpHwDimmSensors[i].Active)
        {
            active = TRUE;
            break;
        }
    }

    PhReleaseQueuedLockShared(&PhpHwNamedSensorLock);

    return active;
}

static BOOLEAN PhpHwDimmPmicsActive(
    VOID
    )
{
    BOOLEAN active = FALSE;

    PhAcquireQueuedLockShared(&PhpHwNamedSensorLock);

    for (ULONG i = 0; i < RTL_NUMBER_OF(PhpHwDimmPmicSensors); i++)
    {
        if (PhpHwDimmPmicSensors[i].Active)
        {
            active = TRUE;
            break;
        }
    }

    PhReleaseQueuedLockShared(&PhpHwNamedSensorLock);

    return active;
}

static BOOLEAN PhpHwDimmVoltagesActive(
    VOID
    )
{
    BOOLEAN active = FALSE;

    PhAcquireQueuedLockShared(&PhpHwNamedSensorLock);

    for (ULONG i = 0; i < RTL_NUMBER_OF(PhpHwDimmVoltageSensors); i++)
    {
        if (PhpHwDimmVoltageSensors[i].Active)
        {
            active = TRUE;
            break;
        }
    }

    PhReleaseQueuedLockShared(&PhpHwNamedSensorLock);

    return active;
}

static BOOLEAN PhpHwHasAcpiId(
    _In_reads_(Count) PCWSTR Ids,
    _In_ ULONG Count,
    _In_reads_(MatchCount) PCPH_STRINGREF Match,
    _In_ ULONG MatchCount
    );

static PPH_BYTES PhpHwGetDeviceAcpiPath(
    _In_ HDEVINFO DeviceInfoSet,
    _In_ PSP_DEVINFO_DATA DeviceInfoData
    );

static BOOLEAN PhpHwGetDeviceString(
    _In_ HDEVINFO DeviceInfoSet,
    _In_ PSP_DEVINFO_DATA DeviceInfoData,
    _In_ const DEVPROPKEY* PropertyKey,
    _Out_writes_z_(Count) PWSTR Buffer,
    _In_ ULONG Count
    );

static BOOLEAN PhpHwSmuSupported(
    _In_ const KPH_HW_CAPABILITIES* Capabilities
    )
{
    return !PhpHwIntel &&
        FlagOn(Capabilities->Flags, KPH_HW_CAP_PCI_AVAILABLE) &&
        FlagOn(Capabilities->ImplementedKinds, (1ULL << KPH_DYN_HW_KIND_MAILBOX_TRANSACT) | (1ULL << KPH_DYN_HW_KIND_SMU_TABLE_READ));
}

static BOOLEAN PhpHwSmuSessionCurrentLocked(
    _In_ ULONG RequestId
    )
{
    return PhpHwSmuRequested && PhpHwSmuRequestId == RequestId;
}

static BOOLEAN PhpHwSmuSessionCurrent(
    _In_ ULONG RequestId
    )
{
    BOOLEAN current;

    PhAcquireQueuedLockShared(&PhpHwNamedSensorLock);
    current = PhpHwSmuSessionCurrentLocked(RequestId);
    PhReleaseQueuedLockShared(&PhpHwNamedSensorLock);

    return current;
}

/**
 * Drops every SMU sensor. The caller holds the named sensor lock exclusively.
 */
static VOID PhpHwSmuClearSensorsLocked(
    VOID
    )
{
    PhpHwSmuSensorCount = 0;
    memset(PhpHwSmuSensors, 0, sizeof(PhpHwSmuSensors));
    memset(PhpHwSmuMemoryClockValid, 0, sizeof(PhpHwSmuMemoryClockValid));
}

/**
 * Applies the session failure policy to an SMU source: contention (STATUS_RETRY) is skipped,
 * STATUS_DEVICE_BUSY and STATUS_IO_TIMEOUT count a strike; any other failure ends the source at once.
 */
static BOOLEAN PhpHwSmuApplyStatus(
    _Inout_ PPHP_HW_SMU_SOURCE Source,
    _In_ PCWSTR Name,
    _In_ ULONG PackageIndex,
    _In_ NTSTATUS Status
    )
{
    if (NT_SUCCESS(Status))
    {
        Source->Strikes = 0;
        return TRUE;
    }

    switch (Status)
    {
    case STATUS_RETRY:
        break;
    case STATUS_DEVICE_BUSY:
    case STATUS_IO_TIMEOUT:
        if (++Source->Strikes < PHP_HW_SMU_STRIKE_LIMIT)
            break;
        __fallthrough;
    default:
        Source->Disabled = TRUE;
        PhTraceInfo("Hardware sensors: SMU %ls of package %lu unavailable: %!STATUS!", Name, PackageIndex, Status);
        break;
    }

    return FALSE;
}

/**
 * Reads and decodes the PM table; a table version without a layout ends the table for the session.
 */
static VOID PhpHwSmuReadTable(
    _In_ ULONG RequestId,
    _In_ ULONG PackageIndex,
    _Inout_ PPH_HW_SMU_DECODE Decode
    )
{
    PPHP_HW_SMU_PACKAGE package = &PhpHwSmuPackages[PackageIndex];
    PPHP_HW_SMU_REPORT report = &PhpHwSmuReports[PackageIndex];
    NTSTATUS status;
    LARGE_INTEGER timeout;
    ULONG version = 0;
    ULONG length = 0;
    BOOLEAN decoded = FALSE;

    // The firmware version of each mailbox op, once per session; an empty batch runs only the version query.
    if (!package->FirmwareQueried)
    {
        BOOLEAN retry = FALSE;

        package->FirmwareQueried = TRUE;

        for (UCHAR i = 0; i < PHP_HW_SMU_MAX_MAILBOXES && PhpHwSmuSessionCurrent(RequestId); i++)
        {
            ULONG firmwareVersion = 0;

            if (package->FirmwareVersions[i])
                continue;

            status = KphHwMailboxBatch(&package->Root, i, NULL, 0, &firmwareVersion, PhTimeoutFromMilliseconds(&timeout, PHP_HW_MAILBOX_TIMEOUT_MS));

            if (status == STATUS_NOT_FOUND)
                break;

            if (NT_SUCCESS(status))
                package->FirmwareVersions[i] = firmwareVersion;
            else if (status == STATUS_RETRY || status == STATUS_DEVICE_BUSY || status == STATUS_IO_TIMEOUT)
                retry = TRUE;
        }

        // A transient failure is asked again on a later pass, up to PHP_HW_RETRY_LIMIT passes.
        if (retry && ++package->FirmwareTries < PHP_HW_RETRY_LIMIT)
            package->FirmwareQueried = FALSE;
    }

    PhAcquireQueuedLockExclusive(&PhpHwNamedSensorLock);

    if (PhpHwSmuSessionCurrentLocked(RequestId))
        memcpy(report->FirmwareVersions, package->FirmwareVersions, sizeof(report->FirmwareVersions));

    PhReleaseQueuedLockExclusive(&PhpHwNamedSensorLock);

    if (package->Table.Disabled)
        return;

    status = KphHwReadSmuTable(
        &package->Root,
        0,
        PhpHwSmuTableBuffer,
        KPH_DYN_HW_SMU_TABLE_MAX_SIZE,
        &version,
        &length,
        PhTimeoutFromMilliseconds(&timeout, PHP_HW_SMU_TABLE_TIMEOUT_MS)
        );

    length = min(length, KPH_DYN_HW_SMU_TABLE_MAX_SIZE);

    if (PhpHwSmuApplyStatus(&package->Table, L"PM table", PackageIndex, status))
    {
        decoded = PhHwSmuDecodePmTable(version, PhpHwSmuTableBuffer, length, package->CoreCount, &package->CoreSlots, Decode);

        if (!decoded)
        {
            PhTraceInfo("Hardware sensors: PM table version 0x%lx of package %lu not decoded", version, PackageIndex);
            package->Table.Disabled = TRUE;
        }
        else if (Decode->CoresEnded)
        {
            PhTraceInfo(
                "Hardware sensors: PM table 0x%lx of package %lu populated a core slot outside 0x%lx; per-core values end",
                version,
                PackageIndex,
                package->CoreSlots.Mask
                );
        }
    }

    PhAcquireQueuedLockExclusive(&PhpHwNamedSensorLock);

    if (!PhpHwSmuSessionCurrentLocked(RequestId))
    {
        PhReleaseQueuedLockExclusive(&PhpHwNamedSensorLock);
        return;
    }

    report->Read = TRUE;
    report->Decoded = decoded;
    report->Status = status;
    report->Version = version;
    report->Length = 0;

    if (NT_SUCCESS(status) && length)
    {
        if (report->Capacity < length)
        {
            if (report->Table)
                PhFree(report->Table);

            report->Table = PhAllocate(length);
            report->Capacity = length;
        }

        memcpy(report->Table, PhpHwSmuTableBuffer, length);
        report->Length = length;
    }

    PhReleaseQueuedLockExclusive(&PhpHwNamedSensorLock);
}

static PCWSTR PhpHwSmuHsmpName(
    _In_ PPHP_HW_SMU_PACKAGE Package
    )
{
    return Package->HsmpAcpi ? L"HSMP (ACPI)" : L"HSMP";
}

/**
 * Runs a batch on an HSMP mailbox op through the SMN pair of the root or the AMDI0097 MMIO mailbox.
 */
static NTSTATUS PhpHwSmuHsmpBatch(
    _In_ PPHP_HW_SMU_PACKAGE Package,
    _In_ UCHAR OpIndex,
    _Inout_updates_(Count) PKPH_HW_MAILBOX_ENTRY Entries,
    _In_ ULONG Count
    )
{
    LARGE_INTEGER timeout;

    PhTimeoutFromMilliseconds(&timeout, PHP_HW_MAILBOX_TIMEOUT_MS);

    if (Package->HsmpAcpi)
        return KphHwMailboxBatchAcpi(&Package->AcpiDevice->br, OpIndex, Entries, Count, NULL, &timeout);

    return KphHwMailboxBatch(&Package->Root, OpIndex, Entries, Count, NULL, &timeout);
}

/**
 * Assigns each AMDI0097 device to the package its _UID names. Matches the hardware ID only, so a
 * merely compatible device never gets its mailbox written.
 */
static VOID PhpHwSmuFindAcpiDevices(
    VOID
    )
{
    static CONST PH_STRINGREF hsmpIds[] =
    {
        PH_STRINGREF_INIT(L"ACPI\\AMDI0097"),
        PH_STRINGREF_INIT(L"*AMDI0097"),
    };
    HDEVINFO deviceInfoSet;
    SP_DEVINFO_DATA deviceInfoData;

    deviceInfoSet = SetupDiGetClassDevsW(NULL, L"ACPI", NULL, DIGCF_ALLCLASSES | DIGCF_PRESENT);

    if (deviceInfoSet == INVALID_HANDLE_VALUE)
        return;

    memset(&deviceInfoData, 0, sizeof(SP_DEVINFO_DATA));
    deviceInfoData.cbSize = sizeof(SP_DEVINFO_DATA);

    for (ULONG i = 0; SetupDiEnumDeviceInfo(deviceInfoSet, i, &deviceInfoData); i++)
    {
        WCHAR ids[512];
        WCHAR instanceId[200]; // MAX_DEVICE_ID_LEN
        DEVPROPTYPE propertyType;
        PPH_BYTES path;
        PH_STRINGREF instance;
        PH_STRINGREF prefix;
        PH_STRINGREF uidString;
        ULONG64 uid;

        memset(ids, 0, sizeof(ids));

        if (
            !SetupDiGetDevicePropertyW(deviceInfoSet, &deviceInfoData, &DEVPKEY_Device_HardwareIds, &propertyType, (PBYTE)ids, sizeof(ids) - sizeof(WCHAR) * 2, NULL, 0) ||
            propertyType != DEVPROP_TYPE_STRING_LIST ||
            !PhpHwHasAcpiId(ids, RTL_NUMBER_OF(ids), hsmpIds, RTL_NUMBER_OF(hsmpIds))
            )
        {
            continue;
        }

        // The instance ID of an ACPI device ends with its _UID, the socket.
        if (!PhpHwGetDeviceString(deviceInfoSet, &deviceInfoData, &DEVPKEY_Device_InstanceId, instanceId, RTL_NUMBER_OF(instanceId)))
            continue;

        PhInitializeStringRefLongHint(&instance, instanceId);

        if (
            !PhSplitStringRefAtLastChar(&instance, L'\\', &prefix, &uidString) ||
            !PhStringToUInt64(&uidString, 10, &uid) ||
            uid >= PhpHwSmuPackageCount ||
            PhpHwSmuPackages[uid].AcpiDevice
            )
        {
            continue;
        }

        path = PhpHwGetDeviceAcpiPath(deviceInfoSet, &deviceInfoData);

        if (!path)
            continue;

        PhpHwSmuPackages[uid].AcpiDevice = path;
        PhTraceInfo("Hardware sensors: HSMP device of package %lu is %s", (ULONG)uid, path->Buffer);
    }

    SetupDiDestroyDeviceInfoList(deviceInfoSet);
}

/**
 * Moves HSMP of a package to the MMIO mailbox of its AMDI0097 device; the probe starts over.
 *
 * \return TRUE when the package has a device to probe.
 */
static BOOLEAN PhpHwSmuUseAcpiHsmp(
    _In_ ULONG PackageIndex
    )
{
    PPHP_HW_SMU_PACKAGE package = &PhpHwSmuPackages[PackageIndex];

    if (package->HsmpAcpi || !PhpHwSmuAcpiAvailable)
        return FALSE;

    if (!PhpHwSmuAcpiEnumerated)
    {
        PhpHwSmuAcpiEnumerated = TRUE;
        PhpHwSmuFindAcpiDevices();
    }

    if (!package->AcpiDevice)
        return FALSE;

    package->HsmpAcpi = TRUE;
    package->HsmpProbeOpIndex = 0;
    package->Hsmp.Disabled = FALSE;
    package->Hsmp.Strikes = 0;

    return TRUE;
}

/**
 * Finds the HSMP mailbox: the op that answers TEST, on the SMN pair first, then the AMDI0097 device.
 *
 * \return TRUE when the mailbox was found.
 */
static BOOLEAN PhpHwSmuFindHsmp(
    _In_ ULONG PackageIndex
    )
{
    PPHP_HW_SMU_PACKAGE package = &PhpHwSmuPackages[PackageIndex];
    NTSTATUS status = STATUS_NOT_FOUND;

    for (; package->HsmpProbeOpIndex < PHP_HW_SMU_MAX_MAILBOXES; package->HsmpProbeOpIndex++)
    {
        KPH_HW_MAILBOX_ENTRY entry;

        memset(&entry, 0, sizeof(KPH_HW_MAILBOX_ENTRY));
        entry.Opcode = PH_HW_HSMP_TEST;
        entry.Args[0] = PHP_HW_SMU_TEST_ARGUMENT;

        status = PhpHwSmuHsmpBatch(package, package->HsmpProbeOpIndex, &entry, 1);

        if (NT_SUCCESS(status))
            status = entry.Status;

        if (NT_SUCCESS(status))
        {
            if (entry.Args[0] == PHP_HW_SMU_TEST_ARGUMENT + 1)
            {
                package->HsmpOpIndex = package->HsmpProbeOpIndex;
                package->Hsmp.Strikes = 0;
                PhTraceInfo("Hardware sensors: %ls of package %lu is mailbox %lu", PhpHwSmuHsmpName(package), PackageIndex, (ULONG)package->HsmpOpIndex);
                return TRUE;
            }

            status = STATUS_NOT_FOUND;
            continue;
        }

        // A mailbox without a TEST row; the next op is tried.
        if (status == STATUS_ACCESS_DENIED || status == STATUS_NOT_IMPLEMENTED)
        {
            status = STATUS_NOT_FOUND;
            continue;
        }

        // The last op (STATUS_NOT_FOUND), a refused root, or a busy mailbox probed again next pass.
        break;
    }

    PhpHwSmuApplyStatus(&package->Hsmp, PhpHwSmuHsmpName(package), PackageIndex, status);

    // Only a missing or refused SMN mailbox moves to ACPI; a busy mutex or mailbox does not.
    if (
        package->Hsmp.Disabled &&
        status != STATUS_RETRY && status != STATUS_DEVICE_BUSY && status != STATUS_IO_TIMEOUT &&
        PhpHwSmuUseAcpiHsmp(PackageIndex)
        )
    {
        return PhpHwSmuFindHsmp(PackageIndex);
    }

    return FALSE;
}

/**
 * Reads the HSMP GET commands of a package in one batch. A refused command is not asked again; a
 * failed handshake counts a strike.
 */
static VOID PhpHwSmuReadHsmp(
    _In_ ULONG PackageIndex,
    _Inout_ PPH_HW_SMU_DECODE Decode
    )
{
    PPHP_HW_SMU_PACKAGE package = &PhpHwSmuPackages[PackageIndex];
    KPH_HW_MAILBOX_ENTRY entries[PH_HW_HSMP_MAX_COMMANDS];
    ULONG commands[PH_HW_HSMP_MAX_COMMANDS];
    BOOLEAN answered = FALSE;
    NTSTATUS transient = STATUS_SUCCESS; // the first failed handshake; STATUS_RETRY only when every one was contention
    ULONG count = 0;
    NTSTATUS status;

    if (package->Hsmp.Disabled)
        return;
    if (package->HsmpOpIndex == UCHAR_MAX && !PhpHwSmuFindHsmp(PackageIndex))
        return;

    for (ULONG i = 0; i < PhHwSmuHsmpCommandCount; i++)
    {
        if (package->HsmpCommandDisabled & (1UL << i))
            continue;

        memset(&entries[count], 0, sizeof(KPH_HW_MAILBOX_ENTRY));
        entries[count].Opcode = PhHwSmuHsmpCommands[i];
        commands[count++] = i;
    }

    if (count == 0)
    {
        PhpHwSmuApplyStatus(&package->Hsmp, PhpHwSmuHsmpName(package), PackageIndex, STATUS_NOT_SUPPORTED);
        return;
    }

    status = PhpHwSmuHsmpBatch(package, package->HsmpOpIndex, entries, count);

    // The batch did not run: every entry keeps STATUS_UNSUCCESSFUL and the status is the mailbox's.
    if (!NT_SUCCESS(status) && status != STATUS_UNSUCCESSFUL && entries[0].Status == STATUS_UNSUCCESSFUL)
    {
        PhpHwSmuApplyStatus(&package->Hsmp, PhpHwSmuHsmpName(package), PackageIndex, status);
        return;
    }

    for (ULONG i = 0; i < count; i++)
    {
        ULONG command = commands[i];
        NTSTATUS entryStatus = entries[i].Status;

        if (NT_SUCCESS(entryStatus))
        {
            answered = TRUE;
            package->HsmpCommandFailures[command] = 0;
            PhHwSmuDecodeHsmp(entries[i].Opcode, entries[i].Args, Decode);
            continue;
        }

        switch (entryStatus)
        {
        case STATUS_RETRY: // not run: an earlier handshake failed, or another agent held the mailbox
            if (NT_SUCCESS(transient))
                transient = STATUS_RETRY;
            break;
        case STATUS_DEVICE_BUSY:
        case STATUS_IO_TIMEOUT:
            if (NT_SUCCESS(transient) || transient == STATUS_RETRY)
                transient = entryStatus;
            break;
        case STATUS_INTERNAL_ERROR: // a firmware prerequisite is not met
        case STATUS_UNSUCCESSFUL: // an unknown firmware answer
            if (++package->HsmpCommandFailures[command] < PHP_HW_RETRY_LIMIT)
                break;
            __fallthrough;
        default:
            package->HsmpCommandDisabled |= 1UL << command;
            PhTraceInfo("Hardware sensors: %ls command 0x%lx of package %lu unavailable: %!STATUS!", PhpHwSmuHsmpName(package), entries[i].Opcode, PackageIndex, entryStatus);
            break;
        }
    }

    // A failed handshake is a mailbox strike; a firmware answer counts against its command only.
    if (!NT_SUCCESS(transient))
        PhpHwSmuApplyStatus(&package->Hsmp, PhpHwSmuHsmpName(package), PackageIndex, transient);
    else if (answered)
        PhpHwSmuApplyStatus(&package->Hsmp, PhpHwSmuHsmpName(package), PackageIndex, STATUS_SUCCESS);
}

/**
 * Publishes the readings of one source of a package when the session is current. Sensors missing
 * from Decode are stale; the sensors of an ended source are dropped.
 */
static VOID PhpHwSmuPublish(
    _In_ ULONG RequestId,
    _In_ ULONG PackageIndex,
    _In_ UCHAR Origin,
    _In_ const PH_HW_SMU_DECODE* Decode
    )
{
    PPHP_HW_SMU_PACKAGE package = &PhpHwSmuPackages[PackageIndex];
    BOOLEAN active = Origin == PHP_HW_SMU_ORIGIN_TABLE ? !package->Table.Disabled : !package->Hsmp.Disabled;

    PhAcquireQueuedLockExclusive(&PhpHwNamedSensorLock);

    if (!PhpHwSmuSessionCurrentLocked(RequestId))
    {
        PhReleaseQueuedLockExclusive(&PhpHwNamedSensorLock);
        return;
    }

    for (ULONG i = 0; i < PhpHwSmuSensorCount; i++)
    {
        if (PhpHwSmuSensors[i].PackageIndex == PackageIndex && PhpHwSmuSensorOrigins[i] == Origin)
        {
            PhpHwSmuSensors[i].Valid = FALSE;
            PhpHwSmuSensors[i].Active = active;
        }
    }

    // An ended source takes the memory clock with it; the pass sets it again from a source still read.
    if (!active)
        PhpHwSmuMemoryClockValid[PackageIndex] = FALSE;

    for (ULONG i = 0; active && i < Decode->Count; i++)
    {
        const PH_HW_SMU_READING* reading = &Decode->Readings[i];
        ULONG id = reading->Id | (Origin == PHP_HW_SMU_ORIGIN_HSMP ? PH_HW_SMU_ID_HSMP : 0);
        PPH_HW_NAMED_SENSOR sensor = NULL;

        for (ULONG j = 0; j < PhpHwSmuSensorCount; j++)
        {
            if (
                PhpHwSmuSensorOrigins[j] == Origin &&
                PhpHwSmuSensors[j].Kind == reading->Kind &&
                PhpHwSmuSensors[j].PackageIndex == PackageIndex &&
                PhpHwSmuSensors[j].Id == id
                )
            {
                sensor = &PhpHwSmuSensors[j];
                break;
            }
        }

        if (!sensor)
        {
            PH_FORMAT format[3];
            ULONG formatCount = 0;

            if (PhpHwSmuSensorCount >= PHP_HW_SMU_MAX_SENSORS)
                continue;

            PhpHwSmuSensorOrigins[PhpHwSmuSensorCount] = Origin;
            sensor = &PhpHwSmuSensors[PhpHwSmuSensorCount++];
            memset(sensor, 0, sizeof(PH_HW_NAMED_SENSOR));
            sensor->Kind = reading->Kind;
            sensor->PackageIndex = PackageIndex;
            sensor->Id = id;

            PhInitFormatS(&format[formatCount++], reading->Name);
            if (reading->Index != PH_HW_SMU_NO_INDEX)
                PhInitFormatU(&format[formatCount++], reading->Index);
            if (reading->Suffix)
                PhInitFormatS(&format[formatCount++], reading->Suffix);

            if (!PhFormatToBuffer(format, formatCount, sensor->Name, sizeof(sensor->Name), NULL))
                sensor->Name[0] = UNICODE_NULL;
        }

        sensor->Active = TRUE;
        sensor->Valid = TRUE;
        sensor->Value = reading->Value;
    }

    PhReleaseQueuedLockExclusive(&PhpHwNamedSensorLock);
}

static VOID PhpHwSmuResetDecode(
    _Inout_ PPH_HW_SMU_DECODE Decode
    )
{
    Decode->Count = 0;
    Decode->MemoryClockValid = FALSE;
    Decode->CoresEnded = FALSE;
    Decode->MemoryClock = 0.0f;
    Decode->FrequencyLimitSourcesValid = FALSE;
    Decode->FrequencyLimitSources = 0;
}

static VOID PhpHwSmuReadPackage(
    _In_ ULONG RequestId,
    _In_ ULONG PackageIndex
    )
{
    PPHP_HW_SMU_PACKAGE package = &PhpHwSmuPackages[PackageIndex];
    BOOLEAN memoryClockValid = FALSE;
    FLOAT memoryClock = 0.0f;

    if (!package->HasRoot || !PhpHwSmuSessionCurrent(RequestId))
        return;

    PhpHwSmuResetDecode(PhpHwSmuDecode);
    PhpHwSmuReadTable(RequestId, PackageIndex, PhpHwSmuDecode);
    PhpHwSmuPublish(RequestId, PackageIndex, PHP_HW_SMU_ORIGIN_TABLE, PhpHwSmuDecode);

    if (PhpHwSmuDecode->MemoryClockValid)
    {
        memoryClockValid = TRUE;
        memoryClock = PhpHwSmuDecode->MemoryClock;
    }

    if (!PhpHwSmuSessionCurrent(RequestId))
        return;

    PhpHwSmuResetDecode(PhpHwSmuDecode);
    PhpHwSmuReadHsmp(PackageIndex, PhpHwSmuDecode);
    PhpHwSmuPublish(RequestId, PackageIndex, PHP_HW_SMU_ORIGIN_HSMP, PhpHwSmuDecode);

    if (PhpHwSmuDecode->FrequencyLimitSourcesValid)
    {
        PhAcquireQueuedLockExclusive(&PhpHwNamedSensorLock);

        if (PhpHwSmuSessionCurrentLocked(RequestId))
        {
            PhpHwSmuReports[PackageIndex].LimitSourcesValid = TRUE;
            PhpHwSmuReports[PackageIndex].LimitSources = PhpHwSmuDecode->FrequencyLimitSources;
        }

        PhReleaseQueuedLockExclusive(&PhpHwNamedSensorLock);
    }

    if (!memoryClockValid && PhpHwSmuDecode->MemoryClockValid)
    {
        memoryClockValid = TRUE;
        memoryClock = PhpHwSmuDecode->MemoryClock;
    }

    // The memory clock is current only when a source read it in this pass.
    PhAcquireQueuedLockExclusive(&PhpHwNamedSensorLock);

    if (PhpHwSmuSessionCurrentLocked(RequestId))
    {
        PhpHwSmuMemoryClockValid[PackageIndex] = memoryClockValid;
        PhpHwSmuMemoryClock[PackageIndex] = memoryClock;
    }

    PhReleaseQueuedLockExclusive(&PhpHwNamedSensorLock);
}

/**
 * Starts the worker state of a session from the request. Called on the SMU worker.
 *
 * \return TRUE when the session is current and has a package to read.
 */
static BOOLEAN PhpHwSmuStartSession(
    _In_ ULONG RequestId
    )
{
    for (ULONG i = 0; i < PhpHwSmuPackageCount; i++)
        PhClearReference(&PhpHwSmuPackages[i].AcpiDevice);

    memset(PhpHwSmuPackages, 0, sizeof(PhpHwSmuPackages));
    PhpHwSmuPackageCount = 0;

    PhAcquireQueuedLockShared(&PhpHwNamedSensorLock);

    if (PhpHwSmuSessionCurrentLocked(RequestId))
    {
        PhpHwSmuPackageCount = PhpHwSmuRequestPackageCount;

        for (ULONG i = 0; i < PhpHwSmuPackageCount; i++)
        {
            PhpHwSmuPackages[i].HasRoot = PhpHwSmuRequestHasRoot[i];
            PhpHwSmuPackages[i].Root = PhpHwSmuRequestRoots[i];
            PhpHwSmuPackages[i].CoreCount = PhpHwSmuRequestCoreCounts[i];
            PhpHwSmuPackages[i].HsmpOpIndex = UCHAR_MAX;
        }

        PhpHwSmuAcpiAvailable = PhpHwSmuRequestAcpi;
    }

    PhpHwSmuAcpiEnumerated = FALSE;

    PhReleaseQueuedLockShared(&PhpHwNamedSensorLock);

    if (!PhpHwSmuTableBuffer)
        PhpHwSmuTableBuffer = PhAllocate(KPH_DYN_HW_SMU_TABLE_MAX_SIZE);
    if (!PhpHwSmuDecode)
        PhpHwSmuDecode = PhAllocate(sizeof(PH_HW_SMU_DECODE));

    return PhpHwSmuPackageCount != 0;
}

static BOOLEAN PhpHwSmuSourcesRemaining(
    VOID
    )
{
    for (ULONG i = 0; i < PhpHwSmuPackageCount; i++)
    {
        PPHP_HW_SMU_PACKAGE package = &PhpHwSmuPackages[i];

        if (package->HasRoot && (!package->Table.Disabled || !package->Hsmp.Disabled))
            return TRUE;
    }

    return FALSE;
}

/**
 * Reads the PM table and the HSMP sensors at most once every PHP_HW_SMU_INTERVAL_MS. Nothing is
 * retried within a pass.
 */
_Function_class_(USER_THREAD_START_ROUTINE)
static NTSTATUS NTAPI PhpHwSmuWorker(
    _In_ PVOID Parameter
    )
{
    LARGE_INTEGER lastPass = { 0 };
    ULONG requestId = 0;
    BOOLEAN active = FALSE;

    PhSetThreadName(NtCurrentThread(), L"HardwareSmu");
    PhSetThreadBasePriority(NtCurrentThread(), THREAD_PRIORITY_NORMAL);

    while (TRUE)
    {
        LARGE_INTEGER timeout;
        LARGE_INTEGER now;
        BOOLEAN requested;
        ULONG id;

        if (active)
        {
            LONG64 elapsedMs = PHP_HW_SMU_INTERVAL_MS;

            if (lastPass.QuadPart)
            {
                PhQueryPerformanceCounter(&now);
                elapsedMs = (now.QuadPart - lastPass.QuadPart) * 1000 / PhpHwFrequency.QuadPart;
                elapsedMs = max(min(elapsedMs, PHP_HW_SMU_INTERVAL_MS), 0);
            }

            NtWaitForSingleObject(PhpHwSmuEvent, FALSE, PhTimeoutFromMilliseconds(&timeout, (ULONG)(PHP_HW_SMU_INTERVAL_MS - elapsedMs)));
        }
        else
        {
            NtWaitForSingleObject(PhpHwSmuEvent, FALSE, NULL);
        }

        PhAcquireQueuedLockShared(&PhpHwNamedSensorLock);
        requested = PhpHwSmuRequested;
        id = PhpHwSmuRequestId;
        PhReleaseQueuedLockShared(&PhpHwNamedSensorLock);

        if (!requested)
        {
            active = FALSE;
            continue;
        }

        if (id != requestId)
        {
            requestId = id;
            active = PhpHwSmuStartSession(requestId);
        }

        if (!active)
            continue;

        // A signal never shortens the interval, also across sessions.
        PhQueryPerformanceCounter(&now);

        if (lastPass.QuadPart && (now.QuadPart - lastPass.QuadPart) * 1000 < PhpHwFrequency.QuadPart * PHP_HW_SMU_INTERVAL_MS)
            continue;

        lastPass = now;

        for (ULONG i = 0; i < PhpHwSmuPackageCount; i++)
            PhpHwSmuReadPackage(requestId, i);

        if (!PhpHwSmuSourcesRemaining())
        {
            PhTraceInfo("Hardware sensors: no SMU source left");
            active = FALSE;
        }
    }

    return STATUS_SUCCESS;
}

/**
 * Starts an SMU sensor session for the packages with a root complex; a previous session ends and its
 * sensors are dropped.
 */
static BOOLEAN PhpHwRequestSmu(
    _In_ const KPH_HW_CAPABILITIES* Capabilities
    )
{
    ULONG packageCount = min(PhpHwPackageCount, PHP_HW_SMU_MAX_PACKAGES);
    BOOLEAN hasRoot = FALSE;

    for (ULONG i = 0; i < packageCount; i++)
    {
        if (PhpHwPackages[i].HasRoot)
            hasRoot = TRUE;
    }

    if (!PhpHwSmuSupported(Capabilities) || !hasRoot || !PhpHwFrequency.QuadPart)
        return FALSE;

    if (!PhpHwSmuEvent)
    {
        if (!NT_SUCCESS(NtCreateEvent(&PhpHwSmuEvent, EVENT_ALL_ACCESS, NULL, SynchronizationEvent, FALSE)))
            return FALSE;

        if (!NT_SUCCESS(PhCreateThread2(PhpHwSmuWorker, NULL)))
        {
            NtClose(PhpHwSmuEvent);
            PhpHwSmuEvent = NULL;
            return FALSE;
        }
    }

    PhAcquireQueuedLockExclusive(&PhpHwNamedSensorLock);

    PhpHwSmuClearSensorsLocked();
    PhpHwSmuRequestPackageCount = packageCount;

    for (ULONG i = 0; i < packageCount; i++)
    {
        PhpHwSmuRequestHasRoot[i] = PhpHwPackages[i].HasRoot;
        PhpHwSmuRequestRoots[i] = PhpHwPackages[i].Root;
        PhpHwSmuRequestCoreCounts[i] = PhpHwPackages[i].CoreCount;
    }

    PhpHwSmuRequestAcpi = FlagOn(Capabilities->Flags, KPH_HW_CAP_ACPI_AVAILABLE) &&
        FlagOn(Capabilities->ImplementedKinds, 1ULL << KPH_DYN_HW_KIND_MAILBOX_TRANSACT);
    PhpHwSmuRequested = TRUE;
    PhpHwSmuRequestId++;

    PhReleaseQueuedLockExclusive(&PhpHwNamedSensorLock);

    NtSetEvent(PhpHwSmuEvent, NULL);
    return TRUE;
}

/**
 * Ends the SMU session and drops its sensors; the worker publishes nothing more for it.
 */
static VOID PhpHwCancelSmu(
    VOID
    )
{
    PhAcquireQueuedLockExclusive(&PhpHwNamedSensorLock);
    PhpHwSmuRequested = FALSE;
    PhpHwSmuClearSensorsLocked();

    for (ULONG i = 0; i < PHP_HW_SMU_MAX_PACKAGES; i++)
    {
        PhpHwSmuReports[i].Read = FALSE;
        PhpHwSmuReports[i].LimitSourcesValid = FALSE;
        memset(PhpHwSmuReports[i].FirmwareVersions, 0, sizeof(PhpHwSmuReports[i].FirmwareVersions));
    }

    PhReleaseQueuedLockExclusive(&PhpHwNamedSensorLock);

    if (PhpHwSmuEvent)
        NtSetEvent(PhpHwSmuEvent, NULL);
}

/**
 * \return PH_HW_CAP_SMU when an SMU sensor is active, and PH_HW_CAP_MEMORY_CLOCK when an SMU memory
 * clock is current.
 */
static ULONG PhpHwSmuCapabilities(
    VOID
    )
{
    ULONG capabilities = 0;

    PhAcquireQueuedLockShared(&PhpHwNamedSensorLock);

    for (ULONG i = 0; i < PhpHwSmuSensorCount; i++)
    {
        if (PhpHwSmuSensors[i].Active)
        {
            capabilities |= PH_HW_CAP_SMU;
            break;
        }
    }

    for (ULONG i = 0; i < PHP_HW_SMU_MAX_PACKAGES; i++)
    {
        if (PhpHwSmuMemoryClockValid[i])
            capabilities |= PH_HW_CAP_MEMORY_CLOCK;
    }

    PhReleaseQueuedLockShared(&PhpHwNamedSensorLock);

    return capabilities;
}

/**
 * Retrieves the SMU memory clock of a package from the last pass of the current session.
 */
_Success_(return)
static BOOLEAN PhpHwSmuGetMemoryClock(
    _In_ ULONG PackageIndex,
    _Out_ PFLOAT Mhz
    )
{
    BOOLEAN valid = FALSE;

    if (PackageIndex >= PHP_HW_SMU_MAX_PACKAGES)
        return FALSE;

    PhAcquireQueuedLockShared(&PhpHwNamedSensorLock);

    if (PhpHwSmuRequested && PhpHwSmuMemoryClockValid[PackageIndex])
    {
        *Mhz = PhpHwSmuMemoryClock[PackageIndex];
        valid = TRUE;
    }

    PhReleaseQueuedLockShared(&PhpHwNamedSensorLock);

    return valid;
}

typedef enum _PHP_HW_FAN_DECODE
{
    PhpHwFanDecodeOk,
    PhpHwFanDecodeUnknown, // the speed is 0xFFFFFFFF (or all ones)
    PhpHwFanDecodeImplausible, // the speed is above PHP_HW_FAN_MAX_RPM
    PhpHwFanDecodeMalformed // not three integers, at the top level or in one package
} PHP_HW_FAN_DECODE;

/**
 * Reads the header of an ACPI method argument that lies entirely inside a buffer.
 *
 * \param Size Receives the size of the argument, header included.
 */
_Success_(return)
static BOOLEAN PhpHwAcpiReadArgument(
    _In_reads_bytes_(Length) const UCHAR* Buffer,
    _In_ ULONG Length,
    _In_ ULONG Offset,
    _Out_ PUSHORT Type,
    _Out_ PUSHORT DataLength,
    _Out_ PULONG Size
    )
{
    USHORT type;
    USHORT dataLength;
    ULONG size;

    if (Offset > Length || Length - Offset < PHP_HW_ACPI_ARGUMENT_HEADER)
        return FALSE;

    memcpy(&type, Buffer + Offset, sizeof(USHORT));
    memcpy(&dataLength, Buffer + Offset + sizeof(USHORT), sizeof(USHORT));
    size = PHP_HW_ACPI_ARGUMENT_HEADER + max((ULONG)sizeof(ULONG), (ULONG)dataLength);

    if (Length - Offset < size)
        return FALSE;

    *Type = type;
    *DataLength = dataLength;
    *Size = size;
    return TRUE;
}

/**
 * Reads consecutive 4- or 8-byte integer arguments that lie inside a buffer, up to the first other argument.
 *
 * \param End The end of the arguments: the output length, or the end of a package's data.
 * \return The number of integers read, at most Count.
 */
static ULONG PhpHwAcpiReadIntegersAt(
    _In_reads_bytes_(End) const UCHAR* Buffer,
    _In_ ULONG Offset,
    _In_ ULONG End,
    _Out_writes_to_(Count, return) PULONG64 Values,
    _In_ ULONG Count
    )
{
    ULONG read = 0;

    while (read < Count)
    {
        USHORT type;
        USHORT dataLength;
        ULONG size;

        if (!PhpHwAcpiReadArgument(Buffer, End, Offset, &type, &dataLength, &size))
            break;
        if (type != PHP_HW_ACPI_ARGUMENT_INTEGER || (dataLength != sizeof(ULONG) && dataLength != sizeof(ULONG64)))
            break;

        Values[read] = 0;
        memcpy(&Values[read], Buffer + Offset + PHP_HW_ACPI_ARGUMENT_HEADER, dataLength);
        read++;
        Offset += size;
    }

    return read;
}

/**
 * Finds the elements of an ACPI method output: the top-level arguments, or the elements of one top-level package.
 *
 * \param Offset Receives the offset of the first element.
 * \param End Receives the end of the elements.
 * \param Elements Receives the number of top-level elements, or ULONG_MAX for the elements of a package.
 */
_Success_(return)
static BOOLEAN PhpHwAcpiGetElements(
    _In_reads_bytes_(Length) const UCHAR* Buffer,
    _In_ ULONG Length,
    _Out_ PULONG Offset,
    _Out_ PULONG End,
    _Out_ PULONG Elements
    )
{
    ULONG signature;
    ULONG outputLength;
    ULONG count;
    USHORT type;
    USHORT dataLength;
    ULONG size;

    if (Length < PHP_HW_ACPI_OUTPUT_HEADER)
        return FALSE;

    memcpy(&signature, Buffer, sizeof(ULONG));
    memcpy(&outputLength, Buffer + sizeof(ULONG), sizeof(ULONG));
    memcpy(&count, Buffer + sizeof(ULONG) * 2, sizeof(ULONG));

    if (signature != PHP_HW_ACPI_OUTPUT_SIGNATURE || outputLength < PHP_HW_ACPI_OUTPUT_HEADER || outputLength > Length || !count)
        return FALSE;
    if (!PhpHwAcpiReadArgument(Buffer, outputLength, PHP_HW_ACPI_OUTPUT_HEADER, &type, &dataLength, &size))
        return FALSE;

    // The elements lie in the package data, which lies in the output.
    if (count == 1 && type == PHP_HW_ACPI_ARGUMENT_PACKAGE)
    {
        *Offset = PHP_HW_ACPI_OUTPUT_HEADER + PHP_HW_ACPI_ARGUMENT_HEADER;
        *End = PHP_HW_ACPI_OUTPUT_HEADER + PHP_HW_ACPI_ARGUMENT_HEADER + dataLength;
        *Elements = ULONG_MAX;
        return TRUE;
    }

    *Offset = PHP_HW_ACPI_OUTPUT_HEADER;
    *End = outputLength;
    *Elements = count;
    return TRUE;
}

/**
 * Reads the leading integers of an ACPI method output, at the top level or in one package.
 *
 * \return The number of integers read, at most Count.
 */
static ULONG PhpHwAcpiReadIntegers(
    _In_reads_bytes_(Length) const UCHAR* Buffer,
    _In_ ULONG Length,
    _Out_writes_to_(Count, return) PULONG64 Values,
    _In_ ULONG Count
    )
{
    ULONG offset;
    ULONG end;
    ULONG elements;

    if (!PhpHwAcpiGetElements(Buffer, Length, &offset, &end, &elements))
        return 0;

    return PhpHwAcpiReadIntegersAt(Buffer, offset, end, Values, min(Count, elements));
}

/**
 * Decodes an _FST result (ACPI 6.5 11.3.1.4): Revision, Control and Speed in RPM.
 * The elements come as top-level arguments or inside one package argument.
 *
 * \param Control Receives the control value: a percentage on a fine-grain fan, else an _FPS control.
 */
_Success_(return == PhpHwFanDecodeOk)
static PHP_HW_FAN_DECODE PhpHwDecodeFanStatus(
    _In_reads_bytes_(Length) const UCHAR* Buffer,
    _In_ ULONG Length,
    _Out_ PULONG Rpm,
    _Out_ PULONG64 Control
    )
{
    ULONG64 values[3];

    if (PhpHwAcpiReadIntegers(Buffer, Length, values, RTL_NUMBER_OF(values)) != RTL_NUMBER_OF(values))
        return PhpHwFanDecodeMalformed;

    if (values[2] == ULONG_MAX || values[2] == ULONG64_MAX)
        return PhpHwFanDecodeUnknown;
    if (values[2] > PHP_HW_FAN_MAX_RPM)
        return PhpHwFanDecodeImplausible;

    *Rpm = (ULONG)values[2];
    *Control = values[1];
    return PhpHwFanDecodeOk;
}

/**
 * Decodes an _FPS result (ACPI 6.5 11.3.1.2): Revision, then one package per state of Control, TripPoint, Speed,
 * NoiseLevel and Power.
 *
 * \return The number of states decoded, at most Count; a malformed state ends the table.
 */
static ULONG PhpHwDecodeFanStates(
    _In_reads_bytes_(Length) const UCHAR* Buffer,
    _In_ ULONG Length,
    _Out_writes_to_(Count, return) PPHP_HW_FAN_STATE States,
    _In_ ULONG Count
    )
{
    ULONG offset;
    ULONG end;
    ULONG elements;
    USHORT revisionType;
    USHORT revisionLength;
    ULONG revisionSize;
    ULONG states = 0;

    if (!PhpHwAcpiGetElements(Buffer, Length, &offset, &end, &elements) || !elements)
        return 0;
    if (!PhpHwAcpiReadArgument(Buffer, end, offset, &revisionType, &revisionLength, &revisionSize) || revisionType != PHP_HW_ACPI_ARGUMENT_INTEGER)
        return 0;

    offset += revisionSize;

    while (states < Count && states < elements - 1)
    {
        USHORT type;
        USHORT dataLength;
        ULONG size;
        ULONG64 values[5];

        if (!PhpHwAcpiReadArgument(Buffer, end, offset, &type, &dataLength, &size) || type != PHP_HW_ACPI_ARGUMENT_PACKAGE)
            break;

        if (PhpHwAcpiReadIntegersAt(
            Buffer,
            offset + PHP_HW_ACPI_ARGUMENT_HEADER,
            offset + PHP_HW_ACPI_ARGUMENT_HEADER + dataLength,
            values,
            RTL_NUMBER_OF(values)
            ) != RTL_NUMBER_OF(values))
        {
            break;
        }

        for (ULONG i = 0; i < RTL_NUMBER_OF(values); i++)
            States[states].Values[i] = values[i] > ULONG_MAX ? ULONG_MAX : (ULONG)values[i];

        states++;
        offset += size;
    }

    return states;
}

/**
 * Reads the first argument of an ACPI method output.
 *
 * \param Offset Receives the offset of the argument data.
 */
_Success_(return)
static BOOLEAN PhpHwAcpiFirstArgument(
    _In_reads_bytes_(Length) const UCHAR* Buffer,
    _In_ ULONG Length,
    _Out_ PUSHORT Type,
    _Out_ PUSHORT DataLength,
    _Out_ PULONG Offset
    )
{
    ULONG signature;
    ULONG outputLength;
    ULONG count;
    ULONG size;

    if (Length < PHP_HW_ACPI_OUTPUT_HEADER)
        return FALSE;

    memcpy(&signature, Buffer, sizeof(ULONG));
    memcpy(&outputLength, Buffer + sizeof(ULONG), sizeof(ULONG));
    memcpy(&count, Buffer + sizeof(ULONG) * 2, sizeof(ULONG));

    if (signature != PHP_HW_ACPI_OUTPUT_SIGNATURE || outputLength < PHP_HW_ACPI_OUTPUT_HEADER || outputLength > Length || !count)
        return FALSE;
    if (!PhpHwAcpiReadArgument(Buffer, outputLength, PHP_HW_ACPI_OUTPUT_HEADER, Type, DataLength, &size))
        return FALSE;

    *Offset = PHP_HW_ACPI_OUTPUT_HEADER + PHP_HW_ACPI_ARGUMENT_HEADER;
    return TRUE;
}

/**
 * Reads an ACPI method output that is one 4- or 8-byte integer.
 */
_Success_(return)
static BOOLEAN PhpHwAcpiReadInteger(
    _In_reads_bytes_(Length) const UCHAR* Buffer,
    _In_ ULONG Length,
    _Out_ PULONG64 Value
    )
{
    USHORT type;
    USHORT dataLength;
    ULONG offset;
    ULONG64 value = 0;

    if (!PhpHwAcpiFirstArgument(Buffer, Length, &type, &dataLength, &offset))
        return FALSE;
    if (type != PHP_HW_ACPI_ARGUMENT_INTEGER || (dataLength != sizeof(ULONG) && dataLength != sizeof(ULONG64)))
        return FALSE;

    memcpy(&value, Buffer + offset, dataLength);
    *Value = value;
    return TRUE;
}

/**
 * Reads an ACPI method output that is a string, or a buffer of UTF-16LE (_STR).
 *
 * \return The string without trailing NULs, or NULL when it is empty or of another type.
 */
_Ret_maybenull_
static PPH_STRING PhpHwAcpiReadString(
    _In_reads_bytes_(Length) const UCHAR* Buffer,
    _In_ ULONG Length
    )
{
    USHORT type;
    USHORT dataLength;
    ULONG offset;
    PPH_STRING string;

    if (!PhpHwAcpiFirstArgument(Buffer, Length, &type, &dataLength, &offset))
        return NULL;

    if (type == PHP_HW_ACPI_ARGUMENT_BUFFER)
    {
        string = PhCreateStringEx((PWCHAR)(Buffer + offset), dataLength & ~1);
    }
    else if (type == PHP_HW_ACPI_ARGUMENT_STRING)
    {
        string = PhZeroExtendToUtf16Ex((PCSTR)(Buffer + offset), dataLength);
    }
    else
    {
        return NULL;
    }

    // The string ends at its first NUL.
    for (SIZE_T i = 0; i < string->Length / sizeof(WCHAR); i++)
    {
        if (string->Buffer[i] == UNICODE_NULL)
        {
            string->Length = i * sizeof(WCHAR);
            break;
        }
    }

    if (!string->Length)
    {
        PhDereferenceObject(string);
        return NULL;
    }

    return string;
}

/**
 * Decodes a DPTF _TMP result: tenths of a kelvin; 0, all ones and readings outside
 * PHP_HW_DPTF_MIN_DECIKELVIN-PHP_HW_DPTF_MAX_DECIKELVIN are not readings.
 */
_Success_(return == PhpHwFanDecodeOk)
static PHP_HW_FAN_DECODE PhpHwDecodeDptfTemperature(
    _In_reads_bytes_(Length) const UCHAR* Buffer,
    _In_ ULONG Length,
    _Out_ PFLOAT Celsius
    )
{
    ULONG64 value;

    if (!PhpHwAcpiReadInteger(Buffer, Length, &value))
        return PhpHwFanDecodeMalformed;
    if (value == 0 || value == ULONG_MAX || value == ULONG64_MAX)
        return PhpHwFanDecodeUnknown;
    if (value < PHP_HW_DPTF_MIN_DECIKELVIN || value > PHP_HW_DPTF_MAX_DECIKELVIN)
        return PhpHwFanDecodeImplausible;

    *Celsius = (FLOAT)value / 10.0f - 273.15f;
    return PhpHwFanDecodeOk;
}

static BOOLEAN PhpHwFanSupported(
    _In_ const KPH_HW_CAPABILITIES* Capabilities
    )
{
    return FlagOn(Capabilities->Flags, KPH_HW_CAP_ACPI_AVAILABLE) &&
        FlagOn(Capabilities->ImplementedKinds, 1ULL << KPH_DYN_HW_KIND_ACPI_EVAL);
}

static BOOLEAN PhpHwFanSessionCurrentLocked(
    _In_ ULONG RequestId
    )
{
    return PhpHwFanRequested && PhpHwFanRequestId == RequestId;
}

static BOOLEAN PhpHwFanSessionCurrent(
    _In_ ULONG RequestId
    )
{
    BOOLEAN current;

    PhAcquireQueuedLockShared(&PhpHwNamedSensorLock);
    current = PhpHwFanSessionCurrentLocked(RequestId);
    PhReleaseQueuedLockShared(&PhpHwNamedSensorLock);

    return current;
}

/**
 * Drops every fan sensor. The caller holds the named sensor lock exclusively.
 */
static VOID PhpHwFanClearSensorsLocked(
    VOID
    )
{
    PhpHwFanSensorCount = 0;
    memset(PhpHwFanSensors, 0, sizeof(PhpHwFanSensors));
    memset(PhpHwFanControlSensors, 0, sizeof(PhpHwFanControlSensors));
    memset(PhpHwAcpiReports, 0, sizeof(PhpHwAcpiReports));
}

/**
 * Determines whether a REG_MULTI_SZ hardware or compatible ID list has one of a set of IDs.
 */
static BOOLEAN PhpHwHasAcpiId(
    _In_reads_(Count) PCWSTR Ids,
    _In_ ULONG Count,
    _In_reads_(MatchCount) PCPH_STRINGREF Match,
    _In_ ULONG MatchCount
    )
{
    for (ULONG i = 0; i < Count && Ids[i] != UNICODE_NULL; )
    {
        PH_STRINGREF id;

        id.Buffer = (PWSTR)&Ids[i];
        id.Length = PhCountStringZ(id.Buffer) * sizeof(WCHAR);

        for (ULONG j = 0; j < MatchCount; j++)
        {
            if (PhEqualStringRef(&id, &Match[j], TRUE))
                return TRUE;
        }

        i += (ULONG)(id.Length / sizeof(WCHAR)) + 1;
    }

    return FALSE;
}

/**
 * Reads the namespace path of a device as DEVPKEY_Device_BiosDeviceName prints it.
 *
 * \return The path, or NULL when the device has none or it is not a namespace path.
 */
static PPH_BYTES PhpHwGetDeviceAcpiPath(
    _In_ HDEVINFO DeviceInfoSet,
    _In_ PSP_DEVINFO_DATA DeviceInfoData
    )
{
    DEVPROPTYPE propertyType;
    ULONG requiredSize = 0;
    PWSTR buffer;
    SIZE_T count;
    PPH_BYTES path = NULL;

    if (
        SetupDiGetDevicePropertyW(DeviceInfoSet, DeviceInfoData, &DEVPKEY_Device_BiosDeviceName, &propertyType, NULL, 0, &requiredSize, 0) ||
        GetLastError() != ERROR_INSUFFICIENT_BUFFER ||
        propertyType != DEVPROP_TYPE_STRING ||
        requiredSize < sizeof(WCHAR) * 3
        )
    {
        return NULL;
    }

    buffer = PhAllocateZero(requiredSize + sizeof(UNICODE_NULL));

    if (
        !SetupDiGetDevicePropertyW(DeviceInfoSet, DeviceInfoData, &DEVPKEY_Device_BiosDeviceName, &propertyType, (PBYTE)buffer, requiredSize, NULL, 0) ||
        propertyType != DEVPROP_TYPE_STRING ||
        buffer[0] != L'\\'
        )
    {
        goto CleanupExit;
    }

    count = PhCountStringZ(buffer);

    if (count < 2)
        goto CleanupExit;

    for (SIZE_T i = 0; i < count; i++)
    {
        WCHAR c = buffer[i];

        if (!((c >= L'A' && c <= L'Z') || (c >= L'0' && c <= L'9') || c == L'_' || c == L'.' || c == L'\\'))
            goto CleanupExit;
    }

    path = PhCreateBytesEx(NULL, count);

    for (SIZE_T i = 0; i < count; i++)
        path->Buffer[i] = (CHAR)buffer[i];

CleanupExit:
    PhFree(buffer);

    return path;
}

/**
 * Reads a string property of a device; the buffer holds a valid string also on failure.
 *
 * \return TRUE when the property is a non-empty string.
 */
static BOOLEAN PhpHwGetDeviceString(
    _In_ HDEVINFO DeviceInfoSet,
    _In_ PSP_DEVINFO_DATA DeviceInfoData,
    _In_ const DEVPROPKEY* PropertyKey,
    _Out_writes_z_(Count) PWSTR Buffer,
    _In_ ULONG Count
    )
{
    DEVPROPTYPE propertyType;

    memset(Buffer, 0, Count * sizeof(WCHAR));

    return SetupDiGetDevicePropertyW(DeviceInfoSet, DeviceInfoData, PropertyKey, &propertyType, (PBYTE)Buffer, (Count - 1) * sizeof(WCHAR), NULL, 0) &&
        propertyType == DEVPROP_TYPE_STRING &&
        Buffer[0] != UNICODE_NULL;
}

/**
 * Determines whether a device has an ACPI fan (PNP0C0B) hardware or compatible ID; Intel DPTF fan
 * participants carry it as a compatible ID.
 */
static BOOLEAN PhpHwIsAcpiFan(
    _In_ HDEVINFO DeviceInfoSet,
    _In_ PSP_DEVINFO_DATA DeviceInfoData
    )
{
    static CONST DEVPROPKEY* keys[] = { &DEVPKEY_Device_HardwareIds, &DEVPKEY_Device_CompatibleIds };
    static CONST PH_STRINGREF fanIds[] =
    {
        PH_STRINGREF_INIT(L"ACPI\\PNP0C0B"),
        PH_STRINGREF_INIT(L"*PNP0C0B"),
        PH_STRINGREF_INIT(L"ACPI\\VEN_PNP&DEV_0C0B"),
    };

    for (ULONG i = 0; i < RTL_NUMBER_OF(keys); i++)
    {
        WCHAR ids[512];
        DEVPROPTYPE propertyType;

        memset(ids, 0, sizeof(ids));

        if (
            SetupDiGetDevicePropertyW(DeviceInfoSet, DeviceInfoData, keys[i], &propertyType, (PBYTE)ids, sizeof(ids) - sizeof(WCHAR) * 2, NULL, 0) &&
            propertyType == DEVPROP_TYPE_STRING_LIST &&
            PhpHwHasAcpiId(ids, RTL_NUMBER_OF(ids), fanIds, RTL_NUMBER_OF(fanIds))
            )
        {
            return TRUE;
        }
    }

    return FALSE;
}

/**
 * Determines whether a device is an Intel DPTF generic participant by its hardware ID; keep in sync with the
 * kphdynhw.xml DPTF row.
 */
static BOOLEAN PhpHwIsDptfParticipant(
    _In_ HDEVINFO DeviceInfoSet,
    _In_ PSP_DEVINFO_DATA DeviceInfoData
    )
{
    static CONST PH_STRINGREF participantIds[] =
    {
        PH_STRINGREF_INIT(L"ACPI\\INT3403"), PH_STRINGREF_INIT(L"*INT3403"),
        PH_STRINGREF_INIT(L"ACPI\\INTC1043"), PH_STRINGREF_INIT(L"*INTC1043"),
        PH_STRINGREF_INIT(L"ACPI\\INTC1046"), PH_STRINGREF_INIT(L"*INTC1046"),
        PH_STRINGREF_INIT(L"ACPI\\INTC1062"), PH_STRINGREF_INIT(L"*INTC1062"),
        PH_STRINGREF_INIT(L"ACPI\\INTC1069"), PH_STRINGREF_INIT(L"*INTC1069"),
        PH_STRINGREF_INIT(L"ACPI\\INTC10A1"), PH_STRINGREF_INIT(L"*INTC10A1"),
        PH_STRINGREF_INIT(L"ACPI\\INTC10D5"), PH_STRINGREF_INIT(L"*INTC10D5"),
        PH_STRINGREF_INIT(L"ACPI\\INTC10FD"), PH_STRINGREF_INIT(L"*INTC10FD"),
        PH_STRINGREF_INIT(L"ACPI\\INTC10F4"), PH_STRINGREF_INIT(L"*INTC10F4"),
    };
    WCHAR ids[512];
    DEVPROPTYPE propertyType;

    memset(ids, 0, sizeof(ids));

    return SetupDiGetDevicePropertyW(DeviceInfoSet, DeviceInfoData, &DEVPKEY_Device_HardwareIds, &propertyType, (PBYTE)ids, sizeof(ids) - sizeof(WCHAR) * 2, NULL, 0) &&
        propertyType == DEVPROP_TYPE_STRING_LIST &&
        PhpHwHasAcpiId(ids, RTL_NUMBER_OF(ids), participantIds, RTL_NUMBER_OF(participantIds));
}

/**
 * Determines whether a device has one of the IDs among its hardware IDs.
 */
static BOOLEAN PhpHwHasHardwareId(
    _In_ HDEVINFO DeviceInfoSet,
    _In_ PSP_DEVINFO_DATA DeviceInfoData,
    _In_reads_(Count) PCPH_STRINGREF Ids,
    _In_ ULONG Count
    )
{
    WCHAR ids[512];
    DEVPROPTYPE propertyType;

    memset(ids, 0, sizeof(ids));

    return SetupDiGetDevicePropertyW(DeviceInfoSet, DeviceInfoData, &DEVPKEY_Device_HardwareIds, &propertyType, (PBYTE)ids, sizeof(ids) - sizeof(WCHAR) * 2, NULL, 0) &&
        propertyType == DEVPROP_TYPE_STRING_LIST &&
        PhpHwHasAcpiId(ids, RTL_NUMBER_OF(ids), Ids, Count);
}

/**
 * Determines whether a device is an ASUS ATK0110 by its hardware ID.
 */
static BOOLEAN PhpHwIsAtk(
    _In_ HDEVINFO DeviceInfoSet,
    _In_ PSP_DEVINFO_DATA DeviceInfoData
    )
{
    static CONST PH_STRINGREF atkIds[] =
    {
        PH_STRINGREF_INIT(L"ACPI\\ATK0110"), PH_STRINGREF_INIT(L"*ATK0110"),
    };

    return PhpHwHasHardwareId(DeviceInfoSet, DeviceInfoData, atkIds, RTL_NUMBER_OF(atkIds));
}

/**
 * Determines whether a device is a Lenovo ThinkPad HKEY with DYTC (LEN0068, LEN0268) by its hardware ID.
 */
static BOOLEAN PhpHwIsDytc(
    _In_ HDEVINFO DeviceInfoSet,
    _In_ PSP_DEVINFO_DATA DeviceInfoData
    )
{
    static CONST PH_STRINGREF hkeyIds[] =
    {
        PH_STRINGREF_INIT(L"ACPI\\LEN0068"), PH_STRINGREF_INIT(L"*LEN0068"),
        PH_STRINGREF_INIT(L"ACPI\\LEN0268"), PH_STRINGREF_INIT(L"*LEN0268"),
    };

    return PhpHwHasHardwareId(DeviceInfoSet, DeviceInfoData, hkeyIds, RTL_NUMBER_OF(hkeyIds));
}

/**
 * Determines whether a device is an ACPI power meter (ACPI000D) by its hardware ID.
 */
static BOOLEAN PhpHwIsPowerMeter(
    _In_ HDEVINFO DeviceInfoSet,
    _In_ PSP_DEVINFO_DATA DeviceInfoData
    )
{
    static CONST PH_STRINGREF meterIds[] =
    {
        PH_STRINGREF_INIT(L"ACPI\\ACPI000D"),
        PH_STRINGREF_INIT(L"*ACPI000D"),
    };
    WCHAR ids[512];
    DEVPROPTYPE propertyType;

    memset(ids, 0, sizeof(ids));

    return SetupDiGetDevicePropertyW(DeviceInfoSet, DeviceInfoData, &DEVPKEY_Device_HardwareIds, &propertyType, (PBYTE)ids, sizeof(ids) - sizeof(WCHAR) * 2, NULL, 0) &&
        propertyType == DEVPROP_TYPE_STRING_LIST &&
        PhpHwHasAcpiId(ids, RTL_NUMBER_OF(ids), meterIds, RTL_NUMBER_OF(meterIds));
}

/**
 * Determines whether a device is an Intel DPTF power participant by its hardware ID; keep in sync with the
 * kphdynhw.xml DPTF power row.
 */
static BOOLEAN PhpHwIsDptfPowerParticipant(
    _In_ HDEVINFO DeviceInfoSet,
    _In_ PSP_DEVINFO_DATA DeviceInfoData
    )
{
    static CONST PH_STRINGREF powerIds[] =
    {
        PH_STRINGREF_INIT(L"ACPI\\INT3407"), PH_STRINGREF_INIT(L"*INT3407"),
        PH_STRINGREF_INIT(L"ACPI\\INT3532"), PH_STRINGREF_INIT(L"*INT3532"),
        PH_STRINGREF_INIT(L"ACPI\\INTC1047"), PH_STRINGREF_INIT(L"*INTC1047"),
        PH_STRINGREF_INIT(L"ACPI\\INTC1050"), PH_STRINGREF_INIT(L"*INTC1050"),
        PH_STRINGREF_INIT(L"ACPI\\INTC1060"), PH_STRINGREF_INIT(L"*INTC1060"),
        PH_STRINGREF_INIT(L"ACPI\\INTC1061"), PH_STRINGREF_INIT(L"*INTC1061"),
        PH_STRINGREF_INIT(L"ACPI\\INTC1065"), PH_STRINGREF_INIT(L"*INTC1065"),
        PH_STRINGREF_INIT(L"ACPI\\INTC1066"), PH_STRINGREF_INIT(L"*INTC1066"),
        PH_STRINGREF_INIT(L"ACPI\\INTC106C"), PH_STRINGREF_INIT(L"*INTC106C"),
        PH_STRINGREF_INIT(L"ACPI\\INTC106D"), PH_STRINGREF_INIT(L"*INTC106D"),
        PH_STRINGREF_INIT(L"ACPI\\INTC10A4"), PH_STRINGREF_INIT(L"*INTC10A4"),
        PH_STRINGREF_INIT(L"ACPI\\INTC10A5"), PH_STRINGREF_INIT(L"*INTC10A5"),
        PH_STRINGREF_INIT(L"ACPI\\INTC10D8"), PH_STRINGREF_INIT(L"*INTC10D8"),
        PH_STRINGREF_INIT(L"ACPI\\INTC10D9"), PH_STRINGREF_INIT(L"*INTC10D9"),
        PH_STRINGREF_INIT(L"ACPI\\INTC1100"), PH_STRINGREF_INIT(L"*INTC1100"),
        PH_STRINGREF_INIT(L"ACPI\\INTC1101"), PH_STRINGREF_INIT(L"*INTC1101"),
        PH_STRINGREF_INIT(L"ACPI\\INTC10F7"), PH_STRINGREF_INIT(L"*INTC10F7"),
        PH_STRINGREF_INIT(L"ACPI\\INTC10F8"), PH_STRINGREF_INIT(L"*INTC10F8"),
    };
    WCHAR ids[512];
    DEVPROPTYPE propertyType;

    memset(ids, 0, sizeof(ids));

    return SetupDiGetDevicePropertyW(DeviceInfoSet, DeviceInfoData, &DEVPKEY_Device_HardwareIds, &propertyType, (PBYTE)ids, sizeof(ids) - sizeof(WCHAR) * 2, NULL, 0) &&
        propertyType == DEVPROP_TYPE_STRING_LIST &&
        PhpHwHasAcpiId(ids, RTL_NUMBER_OF(ids), powerIds, RTL_NUMBER_OF(powerIds));
}

/**
 * Finds the present ACPI fans, DPTF generic participants, power meters, ATK0110 and ThinkPad HKEY devices that have a
 * namespace path; a DPTF power participant is listed once per power getter.
 *
 * \return The number of devices found.
 */
static ULONG PhpHwFindAcpiFans(
    _Out_writes_to_(Count, return) PPHP_HW_FAN Fans,
    _Out_writes_to_(Count, return) PPH_STRING* Names,
    _In_ ULONG Count
    )
{
    HDEVINFO deviceInfoSet;
    SP_DEVINFO_DATA deviceInfoData;
    ULONG found = 0;

    deviceInfoSet = SetupDiGetClassDevsW(NULL, L"ACPI", NULL, DIGCF_ALLCLASSES | DIGCF_PRESENT);

    if (deviceInfoSet == INVALID_HANDLE_VALUE)
        return 0;

    memset(&deviceInfoData, 0, sizeof(SP_DEVINFO_DATA));
    deviceInfoData.cbSize = sizeof(SP_DEVINFO_DATA);

    for (ULONG i = 0; found < Count && SetupDiEnumDeviceInfo(deviceInfoSet, i, &deviceInfoData); i++)
    {
        WCHAR name[128];
        PPH_BYTES path;
        PHP_HW_ACPI_KIND kind;

        if (PhpHwIsAcpiFan(deviceInfoSet, &deviceInfoData))
            kind = PhpHwAcpiFan;
        else if (PhpHwIsDptfParticipant(deviceInfoSet, &deviceInfoData))
            kind = PhpHwAcpiDptfTemperature;
        else if (PhpHwIsPowerMeter(deviceInfoSet, &deviceInfoData))
            kind = PhpHwAcpiPowerMeter;
        else if (PhpHwIsDptfPowerParticipant(deviceInfoSet, &deviceInfoData))
            kind = PhpHwAcpiDptfPower;
        else if (PhpHwIsAtk(deviceInfoSet, &deviceInfoData))
            kind = PhpHwAcpiAtk;
        else if (PhpHwIsDytc(deviceInfoSet, &deviceInfoData))
            kind = PhpHwAcpiDytc;
        else
            continue;

        path = PhpHwGetDeviceAcpiPath(deviceInfoSet, &deviceInfoData);

        if (!path)
            continue;

        if (
            !PhpHwGetDeviceString(deviceInfoSet, &deviceInfoData, &DEVPKEY_Device_FriendlyName, name, RTL_NUMBER_OF(name)) &&
            !PhpHwGetDeviceString(deviceInfoSet, &deviceInfoData, &DEVPKEY_Device_DeviceDesc, name, RTL_NUMBER_OF(name))
            )
        {
            PhCopyStringZ(L"Fan", SIZE_MAX, name, RTL_NUMBER_OF(name), NULL);
        }

        if (kind == PhpHwAcpiDptfPower)
        {
            static CONST UCHAR getters[] = { PHP_HW_POWER_OP_PMAX, PHP_HW_POWER_OP_PROP, PHP_HW_POWER_OP_PBSS };
            static CONST PCWSTR labels[] = { L"Maximum power", L"Rest of platform", L"Steady-state power" };

            C_ASSERT(RTL_NUMBER_OF(getters) == RTL_NUMBER_OF(labels));

            for (ULONG j = 0; j < RTL_NUMBER_OF(getters) && found < Count; j++)
            {
                memset(&Fans[found], 0, sizeof(PHP_HW_FAN));
                Fans[found].Path = j ? PhReferenceObject(path) : path;
                Fans[found].Kind = kind;
                Fans[found].Op = getters[j];
                Names[found] = PhCreateString((PWSTR)labels[j]);
                found++;
            }

            continue;
        }

        memset(&Fans[found], 0, sizeof(PHP_HW_FAN));
        Fans[found].Path = path;
        Fans[found].Kind = kind;
        Fans[found].ArgCount = kind == PhpHwAcpiDytc ? 1 : 0; // the row pins the command
        Names[found] = PhCreateString(name);
        found++;
    }

    SetupDiDestroyDeviceInfoList(deviceInfoSet);

    return found;
}

static PCSTR PhpHwAcpiLastSegment(
    _In_ PPH_BYTES Path
    )
{
    PCSTR segment = Path->Buffer;

    for (PCSTR p = Path->Buffer; *p; p++)
    {
        if (*p == '.' || *p == '\\')
            segment = p + 1;
    }

    return segment;
}

/**
 * Formats the sensor name of a fan; fans that share a device name are told apart by their namespace path.
 */
static VOID PhpHwNameFan(
    _In_ ULONG Index,
    _In_reads_(Count) const PPH_STRING* Names,
    _In_ ULONG Count,
    _Out_writes_z_(PH_HW_NAMED_SENSOR_NAME_LENGTH) PWSTR Name
    )
{
    PCSTR segment = PhpHwAcpiLastSegment(PhpHwFans[Index].Path);
    PH_FORMAT format[4];
    ULONG formatCount = 0;
    BOOLEAN sharedName = FALSE;
    BOOLEAN sharedSegment = FALSE;
    PPH_STRING name;
    SIZE_T count;

    for (ULONG i = 0; i < Count; i++)
    {
        if (i == Index || !PhEqualString(Names[i], Names[Index], TRUE))
            continue;

        sharedName = TRUE;

        if (strcmp(PhpHwAcpiLastSegment(PhpHwFans[i].Path), segment) == 0)
            sharedSegment = TRUE;
    }

    PhInitFormatSR(&format[formatCount++], Names[Index]->sr);

    if (sharedName)
    {
        PhInitFormatS(&format[formatCount++], L" (");
        PhInitFormatMultiByteS(&format[formatCount++], sharedSegment ? PhpHwFans[Index].Path->Buffer : segment);
        PhInitFormatS(&format[formatCount++], L")");
    }

    name = PhFormat(format, formatCount, 0);
    count = min(name->Length / sizeof(WCHAR), PH_HW_NAMED_SENSOR_NAME_LENGTH - 1);
    memcpy(Name, name->Buffer, count * sizeof(WCHAR));
    Name[count] = UNICODE_NULL;
    PhDereferenceObject(name);
}

/**
 * Evaluates an acpi_eval op of an ACPI device of the fan worker.
 *
 * \param ReturnLength Receives the bytes of Buffer written, 0 on failure.
 */
static NTSTATUS PhpHwEvalFanOpEx(
    _In_ const PHP_HW_FAN* Fan,
    _In_ UCHAR OpIndex,
    _Out_writes_bytes_to_(Length, *ReturnLength) PUCHAR Buffer,
    _In_ ULONG Length,
    _Out_ PULONG ReturnLength
    )
{
    ULONG arguments[1];
    LARGE_INTEGER timeout;
    NTSTATUS status;
    ULONG returnLength = 0;

    arguments[0] = Fan->Arg;

    status = KphHwEvalAcpi(
        &Fan->Path->br,
        OpIndex,
        arguments,
        Fan->ArgCount,
        Buffer,
        Length,
        &returnLength,
        PhTimeoutFromMilliseconds(&timeout, PHP_HW_FAN_TIMEOUT_MS)
        );

    *ReturnLength = NT_SUCCESS(status) ? min(returnLength, Length) : 0;

    return status;
}

static NTSTATUS PhpHwEvalFanOp(
    _In_ const PHP_HW_FAN* Fan,
    _In_ UCHAR OpIndex,
    _Out_writes_bytes_to_(PHP_HW_FAN_OUTPUT_LENGTH, *ReturnLength) PUCHAR Buffer,
    _Out_ PULONG ReturnLength
    )
{
    return PhpHwEvalFanOpEx(Fan, OpIndex, Buffer, PHP_HW_FAN_OUTPUT_LENGTH, ReturnLength);
}

/**
 * Evaluates the static methods of a DPTF participant: _STA (present and functioning), PTYP (a temperature
 * sensor, charger or battery), then _STR and the trip points.
 *
 * \param Name Receives the _STR description, or NULL.
 * \param Type Receives PTYP.
 * \param Trips Receives _PSV, _CRT, _AC0 and GTSH in tenths K, ULONG_MAX when absent.
 * \return STATUS_SUCCESS; STATUS_NOT_SUPPORTED when the participant carries no temperature; or the failure of
 * _STA or PTYP, for the session failure policy.
 */
static NTSTATUS PhpHwProbeDptfParticipant(
    _In_ const PHP_HW_FAN* Fan,
    _Outptr_result_maybenull_ PPH_STRING* Name,
    _Out_ PULONG Type,
    _Out_writes_(PHP_HW_DPTF_TRIPS) PULONG Trips
    )
{
    UCHAR buffer[PHP_HW_FAN_OUTPUT_LENGTH];
    ULONG returnLength;
    ULONG64 value;
    NTSTATUS status;

    *Name = NULL;
    *Type = 0;

    for (ULONG i = 0; i < PHP_HW_DPTF_TRIPS; i++)
        Trips[i] = ULONG_MAX;

    // A device without _STA is present and functioning (ACPI 6.5 6.3.7).
    status = PhpHwEvalFanOp(Fan, PHP_HW_DPTF_OP_STA, buffer, &returnLength);

    if (NT_SUCCESS(status))
    {
        if (!PhpHwAcpiReadInteger(buffer, returnLength, &value) || (value & 0x9) != 0x9)
            return STATUS_NOT_SUPPORTED;
    }
    else if (status != STATUS_OBJECT_NAME_NOT_FOUND)
    {
        return status;
    }

    status = PhpHwEvalFanOp(Fan, PHP_HW_DPTF_OP_PTYP, buffer, &returnLength);

    if (!NT_SUCCESS(status))
        return status == STATUS_OBJECT_NAME_NOT_FOUND ? STATUS_NOT_SUPPORTED : status;

    // The IETM (0), fans (4), the PCH (5) and power participants (11h) carry no participant temperature.
    if (!PhpHwAcpiReadInteger(buffer, returnLength, &value) || (value != 0x3 && value != 0xB && value != 0xC))
        return STATUS_NOT_SUPPORTED;

    *Type = (ULONG)value;

    if (NT_SUCCESS(PhpHwEvalFanOp(Fan, PHP_HW_DPTF_OP_STR, buffer, &returnLength)))
        *Name = PhpHwAcpiReadString(buffer, returnLength);

    for (ULONG i = 0; i < PHP_HW_DPTF_TRIPS; i++)
    {
        if (
            NT_SUCCESS(PhpHwEvalFanOp(Fan, (UCHAR)(PHP_HW_DPTF_OP_PSV + i), buffer, &returnLength)) &&
            PhpHwAcpiReadInteger(buffer, returnLength, &value) &&
            value < ULONG_MAX
            )
        {
            Trips[i] = (ULONG)value;
        }
    }

    return STATUS_SUCCESS;
}

static PH_HW_NAMED_SENSOR_KIND PhpHwAcpiSensorKind(
    _In_ const PHP_HW_FAN* Fan
    )
{
    switch (Fan->Kind)
    {
    case PhpHwAcpiFan:
        return PhHwNamedSensorFanSpeed;
    case PhpHwAcpiDptfTemperature:
        return PhHwNamedSensorAcpiTemperature;
    case PhpHwAcpiAtkReading:
        if (Fan->Op == PHP_HW_ATK_OP_RTMP)
            return PhHwNamedSensorAcpiTemperature;
        if (Fan->Op == PHP_HW_ATK_OP_RVLT)
            return PhHwNamedSensorAcpiVoltage;
        return PhHwNamedSensorFanSpeed;
    }

    return PhHwNamedSensorAcpiPower;
}

/**
 * Expands each ATK0110 device into one entry per sensor of its TSIF, VSIF and FSIF lists: a count, then packages
 * of {id, name, limit1, limit2, enable}. Only ids of the hwmon class and the list's type are kept; the reader rows
 * pin the class anyway. The device entry itself is dropped.
 *
 * \return The new number of entries.
 */
static ULONG PhpHwExpandAtk(
    _Inout_updates_(PHP_HW_MAX_FANS) PPHP_HW_FAN Fans,
    _Inout_updates_(PHP_HW_MAX_FANS) PPH_STRING* Names,
    _In_ ULONG Count
    )
{
    static CONST struct { UCHAR ListOp; UCHAR ReadOp; ULONG Class; } lists[] =
    {
        { PHP_HW_ATK_OP_TSIF, PHP_HW_ATK_OP_RTMP, 0x0603 },
        { PHP_HW_ATK_OP_VSIF, PHP_HW_ATK_OP_RVLT, 0x0602 },
        { PHP_HW_ATK_OP_FSIF, PHP_HW_ATK_OP_RFAN, 0x0604 },
    };
    PPH_BYTES paths[PHP_HW_MAX_FANS];
    ULONG deviceCount = 0;
    ULONG count = 0;
    PUCHAR buffer;

    // Keep the other entries in place; take the ATK devices out.
    for (ULONG i = 0; i < Count; i++)
    {
        if (Fans[i].Kind == PhpHwAcpiAtk)
        {
            paths[deviceCount++] = Fans[i].Path;
            PhDereferenceObject(Names[i]);
            continue;
        }

        Fans[count] = Fans[i];
        Names[count] = Names[i];
        count++;
    }

    if (!deviceCount)
        return count;

    buffer = PhAllocate(PHP_HW_ATK_LIST_OUTPUT_LENGTH);

    for (ULONG d = 0; d < deviceCount; d++)
    {
        PHP_HW_FAN device;

        memset(&device, 0, sizeof(PHP_HW_FAN));
        device.Path = paths[d];

        for (ULONG l = 0; l < RTL_NUMBER_OF(lists) && count < PHP_HW_MAX_FANS; l++)
        {
            NTSTATUS status;
            ULONG returnLength;
            ULONG offset;
            ULONG end;
            ULONG elements;
            USHORT type;
            USHORT dataLength;
            ULONG size;

            status = PhpHwEvalFanOpEx(&device, lists[l].ListOp, buffer, PHP_HW_ATK_LIST_OUTPUT_LENGTH, &returnLength);

            if (!NT_SUCCESS(status))
            {
                PhTraceInfo("Hardware sensors: ATK0110 %s list op %u: %!STATUS!", device.Path->Buffer, lists[l].ListOp, status);

                if (status == STATUS_IO_TIMEOUT || status == STATUS_DEVICE_BUSY)
                    break;

                continue;
            }

            if (!PhpHwAcpiGetElements(buffer, returnLength, &offset, &end, &elements) || !elements)
                continue;

            // The count, then the sensor packages.
            if (!PhpHwAcpiReadArgument(buffer, end, offset, &type, &dataLength, &size) || type != PHP_HW_ACPI_ARGUMENT_INTEGER)
                continue;

            offset += size;

            for (ULONG e = 1; e < elements && count < PHP_HW_MAX_FANS; e++)
            {
                ULONG fields[PHP_HW_ATK_SENSOR_FIELDS];
                USHORT fieldTypes[PHP_HW_ATK_SENSOR_FIELDS];
                USHORT fieldLengths[PHP_HW_ATK_SENSOR_FIELDS];
                ULONG fieldEnd;
                ULONG field;
                ULONG64 id;
                ULONG64 enable;
                PPH_STRING name = NULL;
                PPHP_HW_FAN entry;
                ULONG f;

                if (!PhpHwAcpiReadArgument(buffer, end, offset, &type, &dataLength, &size) || type != PHP_HW_ACPI_ARGUMENT_PACKAGE)
                    break;

                // {id, name, limit1, limit2, enable}; a short package or a zero enable is skipped.
                field = offset + PHP_HW_ACPI_ARGUMENT_HEADER;
                fieldEnd = field + dataLength;

                for (f = 0; f < PHP_HW_ATK_SENSOR_FIELDS; f++)
                {
                    ULONG fieldSize;

                    if (!PhpHwAcpiReadArgument(buffer, fieldEnd, field, &fieldTypes[f], &fieldLengths[f], &fieldSize))
                        break;

                    fields[f] = field;
                    field += fieldSize;
                }

                offset += size;

                if (
                    f < PHP_HW_ATK_SENSOR_FIELDS ||
                    PhpHwAcpiReadIntegersAt(buffer, fields[0], fieldEnd, &id, 1) != 1 ||
                    (id >> 16) != lists[l].Class ||
                    PhpHwAcpiReadIntegersAt(buffer, fields[4], fieldEnd, &enable, 1) != 1 ||
                    !enable
                    )
                {
                    continue;
                }

                if (fieldTypes[1] == PHP_HW_ACPI_ARGUMENT_STRING && fieldLengths[1] > 1)
                {
                    name = PhZeroExtendToUtf16Ex((PCSTR)(buffer + fields[1] + PHP_HW_ACPI_ARGUMENT_HEADER), fieldLengths[1]);
                    PhTrimToNullTerminatorString(name);
                }

                if (!name || !name->Length)
                    PhMoveReference(&name, PhFormatString(L"ATK 0x%08I64x", id));

                entry = &Fans[count];
                memset(entry, 0, sizeof(PHP_HW_FAN));
                entry->Path = PhReferenceObject(device.Path);
                entry->Kind = PhpHwAcpiAtkReading;
                entry->Op = lists[l].ReadOp;
                entry->Arg = (ULONG)(id & 0xFFFF);
                entry->ArgCount = 1;
                entry->Probed = TRUE;
                Names[count] = name;
                count++;
            }
        }

        PhDereferenceObject(paths[d]);
    }

    PhFree(buffer);

    return count;
}

/**
 * Finds the fans, DPTF participants, power meters, DPTF power getters, ATK0110 sensors and HKEY devices and lists their
 * sensors. Called on the fan worker. A DPTF participant or power meter is named by its NameSeg until its static methods
 * are read.
 *
 * \return TRUE when the session is current and has a device to read.
 */
static BOOLEAN PhpHwFanStartSession(
    _In_ ULONG RequestId
    )
{
    PPH_STRING names[PHP_HW_MAX_FANS];
    WCHAR sensorNames[PHP_HW_MAX_FANS][PH_HW_NAMED_SENSOR_NAME_LENGTH];
    ULONG ids[PHP_HW_MAX_FANS];
    BOOLEAN current;

    for (ULONG i = 0; i < PhpHwFanCount; i++)
        PhClearReference(&PhpHwFans[i].Path);

    PhpHwFanCount = PhpHwFindAcpiFans(PhpHwFans, names, PHP_HW_MAX_FANS);
    PhpHwFanCount = PhpHwExpandAtk(PhpHwFans, names, PhpHwFanCount);

    for (ULONG i = 0; i < PhpHwFanCount; i++)
    {
        PPH_BYTES path = PhpHwFans[i].Path;
        ULONG j;

        if (PhpHwFans[i].Kind == PhpHwAcpiDptfTemperature || PhpHwFans[i].Kind == PhpHwAcpiPowerMeter || PhpHwFans[i].Kind == PhpHwAcpiDytc)
            PhMoveReference(&names[i], PhZeroExtendToUtf16(PhpHwAcpiLastSegment(path)));

        // The id follows the device (getter, element) across sessions; a colliding hash takes the next free value.
        ids[i] = PhHashBytes((PUCHAR)path->Buffer, path->Length) ^ ((ULONG)PhpHwFans[i].Op * 0x9E3779B9) ^ (PhpHwFans[i].Arg * 0x85EBCA6B);

        for (j = 0; j < i; )
        {
            if (ids[j] == ids[i])
            {
                ids[i]++;
                j = 0;
            }
            else
            {
                j++;
            }
        }

        // A new session never shortens the interval of the previous one.
        PhpHwFans[i].LastEvaluation = PhpHwFanLastEvaluation;
        PhTraceInfo("Hardware sensors: ACPI device %lu %s op %u arg 0x%lx", i, path->Buffer, PhpHwFans[i].Op, PhpHwFans[i].Arg);
    }

    for (ULONG i = 0; i < PhpHwFanCount; i++)
        PhpHwNameFan(i, names, PhpHwFanCount, sensorNames[i]);

    for (ULONG i = 0; i < PhpHwFanCount; i++)
        PhDereferenceObject(names[i]);

    PhAcquireQueuedLockExclusive(&PhpHwNamedSensorLock);

    current = PhpHwFanSessionCurrentLocked(RequestId);

    if (current)
    {
        PhpHwFanClearSensorsLocked();

        for (ULONG i = 0; i < PhpHwFanCount; i++)
        {
            PPH_HW_NAMED_SENSOR sensor = &PhpHwFanSensors[i];
            PPHP_HW_ACPI_REPORT report = &PhpHwAcpiReports[i];
            SIZE_T length;

            sensor->Kind = PhpHwAcpiSensorKind(&PhpHwFans[i]);
            sensor->PackageIndex = 0;
            sensor->Id = ids[i];
            memcpy(sensor->Name, sensorNames[i], sizeof(sensor->Name));

            PhpHwFanControlSensors[i].Kind = PhHwNamedSensorFanControl;
            PhpHwFanControlSensors[i].Id = ids[i];
            memcpy(PhpHwFanControlSensors[i].Name, sensorNames[i], sizeof(PhpHwFanControlSensors[i].Name));

            report->Kind = PhpHwFans[i].Kind;
            report->Op = PhpHwFans[i].Op;
            report->Arg = PhpHwFans[i].Arg;
            report->Type = ULONG_MAX;
            length = min(PhpHwFans[i].Path->Length, sizeof(report->Path) - 1);
            memcpy(report->Path, PhpHwFans[i].Path->Buffer, length);
            report->Path[length] = ANSI_NULL;

            for (ULONG j = 0; j < PHP_HW_DPTF_TRIPS; j++)
                report->Trips[j] = ULONG_MAX;
            for (ULONG j = 0; j < PHP_HW_DYTC_COMMANDS; j++)
                report->Dytc[j] = ULONG_MAX;

            report->AveragingMs = ULONG_MAX;

            for (ULONG j = 0; j < PHP_HW_POWER_STATICS; j++)
                report->Power[j] = ULONG_MAX;
        }

        PhpHwFanSensorCount = PhpHwFanCount;
    }

    PhReleaseQueuedLockExclusive(&PhpHwNamedSensorLock);

    return current && PhpHwFanCount != 0;
}

/**
 * Publishes the static methods of an ACPI device: its name (with the NameSeg when another sensor has the same
 * name) and its report data; the path and kind of the report are kept.
 */
static VOID PhpHwSetAcpiStatic(
    _In_ ULONG RequestId,
    _In_ ULONG Index,
    _In_opt_ PPH_STRING Name,
    _In_ const PHP_HW_ACPI_REPORT* Report
    )
{
    PPH_STRING name = NULL;

    if (Name)
    {
        PCSTR path = PhpHwAcpiLastSegment(PhpHwFans[Index].Path);
        WCHAR segmentBuffer[5] = { 0 };

        for (ULONG i = 0; i < 4 && path[i]; i++)
            segmentBuffer[i] = (WCHAR)path[i];

        name = PhReferenceObject(Name);

        PhAcquireQueuedLockShared(&PhpHwNamedSensorLock);

        for (ULONG i = 0; i < PhpHwFanSensorCount; i++)
        {
            PH_STRINGREF other;

            if (i == Index)
                continue;

            PhInitializeStringRefLongHint(&other, PhpHwFanSensors[i].Name);

            if (PhEqualStringRef(&other, &Name->sr, TRUE))
            {
                PhMoveReference(&name, PhFormatString(L"%s (%s)", Name->Buffer, segmentBuffer));
                break;
            }
        }

        PhReleaseQueuedLockShared(&PhpHwNamedSensorLock);
    }

    PhAcquireQueuedLockExclusive(&PhpHwNamedSensorLock);

    if (PhpHwFanSessionCurrentLocked(RequestId) && Index < PhpHwFanSensorCount)
    {
        if (name)
        {
            PPH_HW_NAMED_SENSOR sensor = &PhpHwFanSensors[Index];
            SIZE_T count = min(name->Length / sizeof(WCHAR), PH_HW_NAMED_SENSOR_NAME_LENGTH - 1);

            memset(sensor->Name, 0, sizeof(sensor->Name));
            memcpy(sensor->Name, name->Buffer, count * sizeof(WCHAR));
        }

        PPHP_HW_ACPI_REPORT report = &PhpHwAcpiReports[Index];
        PHP_HW_ACPI_KIND kind = report->Kind;
        UCHAR op = report->Op;
        ULONG arg = report->Arg;
        CHAR path[sizeof(report->Path)];

        memcpy(path, report->Path, sizeof(path));
        *report = *Report;
        memcpy(report->Path, path, sizeof(report->Path));
        report->Kind = kind;
        report->Op = op;
        report->Arg = arg;
    }

    PhReleaseQueuedLockExclusive(&PhpHwNamedSensorLock);

    PhClearReference(&name);
}

/**
 * Applies the session failure policy to an ACPI device: contention (STATUS_RETRY) is skipped, STATUS_IO_TIMEOUT
 * counts a strike, STATUS_DEVICE_BUSY counts toward PHP_HW_FAN_BUSY_LIMIT, any other failure ends the device at once.
 */
static BOOLEAN PhpHwFanApplyStatus(
    _Inout_ PPHP_HW_FAN Fan,
    _In_ ULONG Index,
    _In_ NTSTATUS Status
    )
{
    BOOLEAN disable;

    if (NT_SUCCESS(Status))
    {
        Fan->Strikes = 0;
        Fan->BusyCount = 0;
        return TRUE;
    }

    switch (Status)
    {
    case STATUS_DEVICE_BUSY:
        disable = ++Fan->BusyCount >= PHP_HW_FAN_BUSY_LIMIT;
        break;
    case STATUS_RETRY:
        disable = FALSE;
        break;
    case STATUS_IO_TIMEOUT:
        Fan->BusyCount = 0;
        disable = ++Fan->Strikes >= PHP_HW_FAN_STRIKE_LIMIT;
        break;
    default:
        disable = TRUE;
        break;
    }

    if (disable)
    {
        Fan->Item.Disabled = TRUE;
        PhTraceInfo("Hardware sensors: ACPI device %lu unavailable: %!STATUS!", Index, Status);
    }

    return FALSE;
}

static VOID PhpHwSetFanSensor(
    _In_ ULONG RequestId,
    _In_ ULONG Index,
    _In_ BOOLEAN Valid,
    _In_ FLOAT Value
    )
{
    PhAcquireQueuedLockExclusive(&PhpHwNamedSensorLock);

    if (PhpHwFanSessionCurrentLocked(RequestId) && Index < PhpHwFanSensorCount)
    {
        if (Valid)
            PhpHwFanSensors[Index].Value = Value;

        PhpHwFanSensors[Index].Valid = Valid;
        PhpHwFanSensors[Index].Active = PhpHwFans[Index].Answered && !PhpHwFans[Index].Item.Disabled;
    }

    PhReleaseQueuedLockExclusive(&PhpHwNamedSensorLock);
}

/**
 * \return The interval of a device.
 */
static ULONG PhpHwFanIntervalMs(
    _In_ const PHP_HW_FAN* Fan
    )
{
    return max(Fan->IntervalMs, PHP_HW_FAN_INTERVAL_MS);
}

/**
 * \return The milliseconds since the last evaluation of a device, at most its interval.
 */
static ULONG PhpHwFanElapsedMs(
    _In_ const PHP_HW_FAN* Fan
    )
{
    LARGE_INTEGER now;
    LONG64 elapsedMs;

    if (!Fan->LastEvaluation.QuadPart)
        return PhpHwFanIntervalMs(Fan);

    PhQueryPerformanceCounter(&now);
    elapsedMs = (now.QuadPart - Fan->LastEvaluation.QuadPart) * 1000 / PhpHwFrequency.QuadPart;

    return (ULONG)max(min(elapsedMs, (LONG64)PhpHwFanIntervalMs(Fan)), 0);
}

/**
 * \return The milliseconds until the next device is due.
 */
static ULONG PhpHwFanWaitMs(
    VOID
    )
{
    ULONG waitMs = PHP_HW_FAN_INTERVAL_MS;

    for (ULONG i = 0; i < PhpHwFanCount; i++)
    {
        ULONG remainingMs;

        if (PhpHwFans[i].Item.Disabled)
            continue;

        remainingMs = PhpHwFanIntervalMs(&PhpHwFans[i]) - PhpHwFanElapsedMs(&PhpHwFans[i]);

        if (remainingMs < waitMs)
            waitMs = remainingMs;
    }

    return waitMs;
}

/**
 * Reads the DYTC query, get, function capabilities and MMC get of a ThinkPad HKEY once, for the report; the
 * profile is a label, so the device ends after it.
 */
static VOID PhpHwReadDytcStatic(
    _In_ ULONG RequestId,
    _In_ ULONG Index
    )
{
    PPHP_HW_FAN fan = &PhpHwFans[Index];
    UCHAR buffer[PHP_HW_FAN_OUTPUT_LENGTH];
    PHP_HW_ACPI_REPORT report;
    ULONG returnLength;
    ULONG64 value;

    memset(&report, 0, sizeof(PHP_HW_ACPI_REPORT));
    report.Type = ULONG_MAX;
    report.AveragingMs = ULONG_MAX;

    for (ULONG i = 0; i < PHP_HW_DPTF_TRIPS; i++)
        report.Trips[i] = ULONG_MAX;
    for (ULONG i = 0; i < PHP_HW_POWER_STATICS; i++)
        report.Power[i] = ULONG_MAX;
    for (ULONG i = 0; i < PHP_HW_DYTC_COMMANDS; i++)
        report.Dytc[i] = ULONG_MAX;

    for (UCHAR i = 0; i < PHP_HW_DYTC_COMMANDS; i++)
    {
        if (
            NT_SUCCESS(PhpHwEvalFanOp(fan, i, buffer, &returnLength)) &&
            PhpHwAcpiReadInteger(buffer, returnLength, &value) &&
            value < ULONG_MAX
            )
        {
            report.Dytc[i] = (ULONG)value;
        }

        // Without bit 8 of the query DYTC is not enabled; MMC get needs its function capability.
        if (i == 0 && (report.Dytc[0] == ULONG_MAX || !(report.Dytc[0] & 0x100)))
            break;
        if (i == 2 && (report.Dytc[2] == ULONG_MAX || !(report.Dytc[2] & (1UL << PHP_HW_DYTC_FC_MMC))))
            break;
    }

    fan->Probed = TRUE;
    fan->Item.Disabled = TRUE;
    PhpHwSetAcpiStatic(RequestId, Index, NULL, &report);
    PhpHwSetFanSensor(RequestId, Index, FALSE, 0.0f);
}

/**
 * Decodes an ATK0110 reader result: tenths of a degree, mV or RPM.
 */
_Success_(return == PhpHwFanDecodeOk)
static PHP_HW_FAN_DECODE PhpHwDecodeAtkReading(
    _In_ const PHP_HW_FAN* Fan,
    _In_reads_bytes_(Length) const UCHAR* Buffer,
    _In_ ULONG Length,
    _Out_ PFLOAT Value
    )
{
    ULONG64 raw;
    LONG value;

    if (!PhpHwAcpiReadInteger(Buffer, Length, &raw))
        return PhpHwFanDecodeMalformed;
    if (raw == ULONG_MAX || raw == ULONG64_MAX)
        return PhpHwFanDecodeUnknown;

    value = (LONG)(ULONG)raw;

    switch (Fan->Op)
    {
    case PHP_HW_ATK_OP_RTMP:
        if (value < -400 || value > 1500)
            return PhpHwFanDecodeImplausible;
        *Value = (FLOAT)value / 10.0f;
        return PhpHwFanDecodeOk;
    case PHP_HW_ATK_OP_RVLT:
        if (value < 0 || value > 20000)
            return PhpHwFanDecodeImplausible;
        *Value = (FLOAT)value / 1000.0f;
        return PhpHwFanDecodeOk;
    default:
        if (value < 0 || value > PHP_HW_FAN_MAX_RPM)
            return PhpHwFanDecodeImplausible;
        *Value = (FLOAT)value;
        return PhpHwFanDecodeOk;
    }
}

/**
 * Reads the static methods of a DPTF participant on its first due pass; the reading starts on the next.
 */
static VOID PhpHwReadDptfStatic(
    _In_ ULONG RequestId,
    _In_ ULONG Index
    )
{
    PPHP_HW_FAN fan = &PhpHwFans[Index];
    PHP_HW_ACPI_REPORT report;
    PPH_STRING name;
    NTSTATUS status;

    memset(&report, 0, sizeof(PHP_HW_ACPI_REPORT));
    report.AveragingMs = ULONG_MAX;
    status = PhpHwProbeDptfParticipant(fan, &name, &report.Type, report.Trips);

    if (NT_SUCCESS(status))
    {
        fan->Probed = TRUE;
        fan->Strikes = 0;
        fan->BusyCount = 0;
        PhpHwSetAcpiStatic(RequestId, Index, name, &report);
        PhClearReference(&name);
    }
    else if (status == STATUS_NOT_SUPPORTED)
    {
        fan->Item.Disabled = TRUE;
        PhTraceInfo("Hardware sensors: DPTF participant %s has no temperature", fan->Path->Buffer);
    }
    else
    {
        PhpHwFanApplyStatus(fan, Index, status);
    }

    PhpHwSetFanSensor(RequestId, Index, FALSE, 0.0f);
}

/**
 * Decodes an ACPI000D _PMC result (ACPI 6.5 10.4.1): PHP_HW_METER_PMC_INTEGERS integers, then the model, serial
 * and OEM strings, into the report.
 *
 * \return The number of leading integers.
 */
static ULONG PhpHwDecodePowerMeterCapabilities(
    _In_reads_bytes_(Length) const UCHAR* Buffer,
    _In_ ULONG Length,
    _Inout_ PPHP_HW_ACPI_REPORT Report
    )
{
    ULONG offset;
    ULONG end;
    ULONG elements;
    ULONG count = 0;

    if (!PhpHwAcpiGetElements(Buffer, Length, &offset, &end, &elements))
        return 0;

    for (ULONG i = 0; i < PHP_HW_METER_PMC_ELEMENTS && i < elements; i++)
    {
        USHORT type;
        USHORT dataLength;
        ULONG size;

        if (!PhpHwAcpiReadArgument(Buffer, end, offset, &type, &dataLength, &size))
            break;

        if (i < PHP_HW_METER_PMC_INTEGERS)
        {
            ULONG64 value = 0;

            if (type != PHP_HW_ACPI_ARGUMENT_INTEGER || (dataLength != sizeof(ULONG) && dataLength != sizeof(ULONG64)))
                break;

            memcpy(&value, Buffer + offset + PHP_HW_ACPI_ARGUMENT_HEADER, dataLength);
            Report->Meter[i] = value > ULONG_MAX ? ULONG_MAX : (ULONG)value;
            count++;
        }
        else
        {
            PSTR string = Report->MeterStrings[i - PHP_HW_METER_PMC_INTEGERS];
            const CHAR* data = (const CHAR*)(Buffer + offset + PHP_HW_ACPI_ARGUMENT_HEADER);
            ULONG length = 0;

            if (type != PHP_HW_ACPI_ARGUMENT_STRING)
                break;

            while (length < dataLength && length < sizeof(Report->MeterStrings[0]) - 1 && data[length])
            {
                string[length] = data[length];
                length++;
            }

            string[length] = ANSI_NULL;
        }

        offset += size;
    }

    Report->MeterCount = count;

    return count;
}

/**
 * Reads _STA, _PMC and _GAI of a power meter on its first due pass: a meter that cannot measure, or not in mW, ends.
 * The reading starts on the next pass; after it, readings are no closer than _GAI.
 */
static VOID PhpHwReadMeterStatic(
    _In_ ULONG RequestId,
    _In_ ULONG Index
    )
{
    PPHP_HW_FAN fan = &PhpHwFans[Index];
    UCHAR buffer[PHP_HW_METER_PMC_OUTPUT_LENGTH];
    PHP_HW_ACPI_REPORT report;
    PPH_STRING model = NULL;
    ULONG returnLength;
    ULONG64 value;
    NTSTATUS status;

    memset(&report, 0, sizeof(PHP_HW_ACPI_REPORT));
    report.Type = ULONG_MAX;
    report.AveragingMs = ULONG_MAX;

    for (ULONG i = 0; i < PHP_HW_DPTF_TRIPS; i++)
        report.Trips[i] = ULONG_MAX;

    // A device without _STA is present and functioning (ACPI 6.5 6.3.7).
    status = PhpHwEvalFanOp(fan, PHP_HW_METER_OP_STA, buffer, &returnLength);

    if (NT_SUCCESS(status))
    {
        if (!PhpHwAcpiReadInteger(buffer, returnLength, &value) || (value & 0x9) != 0x9)
            status = STATUS_NOT_SUPPORTED;
    }
    else if (status == STATUS_OBJECT_NAME_NOT_FOUND)
    {
        status = STATUS_SUCCESS;
    }

    if (NT_SUCCESS(status))
    {
        status = PhpHwEvalFanOpEx(fan, PHP_HW_METER_OP_PMC, buffer, sizeof(buffer), &returnLength);

        if (NT_SUCCESS(status))
        {
            PhpHwDecodePowerMeterCapabilities(buffer, returnLength, &report);

            if (report.MeterStrings[0][0])
                model = PhZeroExtendToUtf16(report.MeterStrings[0]);
            else if (report.MeterStrings[2][0])
                model = PhZeroExtendToUtf16(report.MeterStrings[2]);

            // [0] bit 0 measurement; [1] unit 0 is mW, the only unit accepted.
            if (report.MeterCount < 2 || !(report.Meter[0] & 0x1) || report.Meter[1] != 0)
                status = STATUS_NOT_SUPPORTED;
        }
        else if (status == STATUS_OBJECT_NAME_NOT_FOUND)
        {
            status = STATUS_NOT_SUPPORTED;
        }
    }

    if (NT_SUCCESS(status))
    {
        if (
            NT_SUCCESS(PhpHwEvalFanOp(fan, PHP_HW_METER_OP_GAI, buffer, &returnLength)) &&
            PhpHwAcpiReadInteger(buffer, returnLength, &value) &&
            value < ULONG_MAX
            )
        {
            report.AveragingMs = (ULONG)value;
            fan->MeterIntervalMs = (ULONG)min(value, PHP_HW_METER_MAX_INTERVAL_MS);
        }

        fan->Probed = TRUE;
        fan->Strikes = 0;
        fan->BusyCount = 0;
        PhpHwSetAcpiStatic(RequestId, Index, model, &report);
    }
    else if (status == STATUS_NOT_SUPPORTED)
    {
        fan->Item.Disabled = TRUE;
        PhTraceInfo("Hardware sensors: ACPI power meter %s is not a measuring mW meter", fan->Path->Buffer);
    }
    else
    {
        PhpHwFanApplyStatus(fan, Index, status);
    }

    PhClearReference(&model);
    PhpHwSetFanSensor(RequestId, Index, FALSE, 0.0f);
}

/**
 * Reads _STA and PTYP of a DPTF power participant entry on its first due pass, and the static getters of its type on
 * the PMAX entry, for the report; the reading starts on the next pass. PTYP 11h is a power
 * participant (PMAX, PSRC, ARTG, PBSS, CTYP, PROP), 0Ch a battery (PMAX, PBSS, RBHF, VBNL, CMPP); anything else,
 * or a getter outside the type, ends the entry.
 */
static VOID PhpHwReadDptfPowerStatic(
    _In_ ULONG RequestId,
    _In_ ULONG Index
    )
{
    static CONST UCHAR statics[PHP_HW_POWER_STATICS] =
    {
        PHP_HW_POWER_OP_PSRC, PHP_HW_POWER_OP_ARTG, PHP_HW_POWER_OP_CTYP, PHP_HW_POWER_OP_RBHF, PHP_HW_POWER_OP_VBNL, PHP_HW_POWER_OP_CMPP
    };
    static CONST BOOLEAN battery[PHP_HW_POWER_STATICS] = { FALSE, FALSE, FALSE, TRUE, TRUE, TRUE };
    PPHP_HW_FAN fan = &PhpHwFans[Index];
    UCHAR buffer[PHP_HW_FAN_OUTPUT_LENGTH];
    PHP_HW_ACPI_REPORT report;
    ULONG returnLength;
    ULONG64 value;
    NTSTATUS status;

    memset(&report, 0, sizeof(PHP_HW_ACPI_REPORT));
    report.Type = ULONG_MAX;
    report.AveragingMs = ULONG_MAX;

    for (ULONG i = 0; i < PHP_HW_DPTF_TRIPS; i++)
        report.Trips[i] = ULONG_MAX;
    for (ULONG i = 0; i < PHP_HW_POWER_STATICS; i++)
        report.Power[i] = ULONG_MAX;

    // A device without _STA is present and functioning (ACPI 6.5 6.3.7).
    status = PhpHwEvalFanOp(fan, PHP_HW_POWER_OP_STA, buffer, &returnLength);

    if (NT_SUCCESS(status))
    {
        if (!PhpHwAcpiReadInteger(buffer, returnLength, &value) || (value & 0x9) != 0x9)
            status = STATUS_NOT_SUPPORTED;
    }
    else if (status == STATUS_OBJECT_NAME_NOT_FOUND)
    {
        status = STATUS_SUCCESS;
    }

    if (NT_SUCCESS(status))
    {
        status = PhpHwEvalFanOp(fan, PHP_HW_POWER_OP_PTYP, buffer, &returnLength);

        if (NT_SUCCESS(status))
        {
            if (!PhpHwAcpiReadInteger(buffer, returnLength, &value) || (value != 0x11 && value != 0xC))
                status = STATUS_NOT_SUPPORTED;
            else if (value == 0xC && fan->Op == PHP_HW_POWER_OP_PROP)
                status = STATUS_NOT_SUPPORTED; // a battery has no rest-of-platform power
            else
                report.Type = (ULONG)value;
        }
        else if (status == STATUS_OBJECT_NAME_NOT_FOUND)
        {
            status = STATUS_NOT_SUPPORTED;
        }
    }

    if (NT_SUCCESS(status))
    {
        for (ULONG i = 0; fan->Op == PHP_HW_POWER_OP_PMAX && i < PHP_HW_POWER_STATICS; i++)
        {
            if (battery[i] != (report.Type == 0xC))
                continue;

            if (
                NT_SUCCESS(PhpHwEvalFanOp(fan, statics[i], buffer, &returnLength)) &&
                PhpHwAcpiReadInteger(buffer, returnLength, &value) &&
                value < ULONG_MAX
                )
            {
                report.Power[i] = (ULONG)value;
            }
        }

        fan->Probed = TRUE;
        fan->Strikes = 0;
        fan->BusyCount = 0;
        PhpHwSetAcpiStatic(RequestId, Index, NULL, &report);
    }
    else if (status == STATUS_NOT_SUPPORTED)
    {
        fan->Item.Disabled = TRUE;
        PhTraceInfo("Hardware sensors: DPTF power participant %s op %u: no such getter for its type", fan->Path->Buffer, fan->Op);
    }
    else
    {
        PhpHwFanApplyStatus(fan, Index, status);
    }

    PhpHwSetFanSensor(RequestId, Index, FALSE, 0.0f);
}

/**
 * Decodes a _PMM or DPTF power getter result: mW; all ones is unknown, above PHP_HW_METER_MAX_MILLIWATTS implausible.
 */
_Success_(return == PhpHwFanDecodeOk)
static PHP_HW_FAN_DECODE PhpHwDecodePowerMeterReading(
    _In_reads_bytes_(Length) const UCHAR* Buffer,
    _In_ ULONG Length,
    _Out_ PFLOAT Watts
    )
{
    ULONG64 value;

    if (!PhpHwAcpiReadInteger(Buffer, Length, &value))
        return PhpHwFanDecodeMalformed;
    if (value == ULONG_MAX || value == ULONG64_MAX)
        return PhpHwFanDecodeUnknown;
    if (value > PHP_HW_METER_MAX_MILLIWATTS)
        return PhpHwFanDecodeImplausible;

    *Watts = (FLOAT)value / 1000.0f;
    return PhpHwFanDecodeOk;
}

/**
 * Reads _FIF and _FPS of a fan on its first due pass; both are optional, so failures are not counted. The reading
 * starts on the next pass.
 */
static VOID PhpHwReadFanStatic(
    _In_ ULONG RequestId,
    _In_ ULONG Index
    )
{
    PPHP_HW_FAN fan = &PhpHwFans[Index];
    PUCHAR buffer;
    ULONG returnLength;
    ULONG64 fif[4];
    ULONG fifCount = 0;
    PHP_HW_FAN_STATE states[PHP_HW_FAN_MAX_STATES];
    ULONG stateCount = 0;

    buffer = PhAllocate(PHP_HW_FAN_FPS_OUTPUT_LENGTH);

    if (NT_SUCCESS(PhpHwEvalFanOp(fan, PHP_HW_FAN_OP_FIF, buffer, &returnLength)))
        fifCount = PhpHwAcpiReadIntegers(buffer, returnLength, fif, RTL_NUMBER_OF(fif));
    if (NT_SUCCESS(PhpHwEvalFanOpEx(fan, PHP_HW_FAN_OP_FPS, buffer, PHP_HW_FAN_FPS_OUTPUT_LENGTH, &returnLength)))
        stateCount = PhpHwDecodeFanStates(buffer, returnLength, states, RTL_NUMBER_OF(states));

    PhFree(buffer);

    fan->Probed = TRUE;
    fan->FineGrain = fifCount >= 2 && fif[1] == 1;

    PhAcquireQueuedLockExclusive(&PhpHwNamedSensorLock);

    if (PhpHwFanSessionCurrentLocked(RequestId) && Index < PhpHwFanSensorCount)
    {
        PPHP_HW_ACPI_REPORT report = &PhpHwAcpiReports[Index];

        report->FifCount = fifCount;

        for (ULONG i = 0; i < fifCount; i++)
            report->Fif[i] = fif[i] > ULONG_MAX ? ULONG_MAX : (ULONG)fif[i];

        report->StateCount = stateCount;
        memcpy(report->States, states, stateCount * sizeof(PHP_HW_FAN_STATE));
    }

    PhReleaseQueuedLockExclusive(&PhpHwNamedSensorLock);
}

/**
 * Publishes the control of a fine-grain fan, a percentage.
 */
static VOID PhpHwSetFanControlSensor(
    _In_ ULONG RequestId,
    _In_ ULONG Index,
    _In_ BOOLEAN Valid,
    _In_ FLOAT Value
    )
{
    PhAcquireQueuedLockExclusive(&PhpHwNamedSensorLock);

    if (PhpHwFanSessionCurrentLocked(RequestId) && Index < PhpHwFanSensorCount)
    {
        PPH_HW_NAMED_SENSOR sensor = &PhpHwFanControlSensors[Index];

        if (Valid)
            sensor->Value = Value;

        sensor->Valid = Valid;
        sensor->Active = PhpHwFans[Index].FineGrain && PhpHwFans[Index].Answered && !PhpHwFans[Index].Item.Disabled;
    }

    PhReleaseQueuedLockExclusive(&PhpHwNamedSensorLock);
}

/**
 * Evaluates the reading of each entry that is due: _FST of a fan, _TMP of a DPTF participant, _PMM of a meter, the
 * getter of a DPTF power entry, the reader of an ATK0110 sensor. A malformed result
 * ends the device for the session; an unknown reading is skipped.
 */
static VOID PhpHwReadFans(
    _In_ ULONG RequestId
    )
{
    UCHAR buffer[PHP_HW_FAN_OUTPUT_LENGTH];

    for (ULONG i = 0; i < PhpHwFanCount; i++)
    {
        PPHP_HW_FAN fan = &PhpHwFans[i];
        NTSTATUS status;
        ULONG returnLength;
        ULONG rpm = 0;
        ULONG64 control = 0;
        FLOAT value = 0.0f;
        BOOLEAN valid = FALSE;

        if (fan->Item.Disabled || PhpHwFanElapsedMs(fan) < PhpHwFanIntervalMs(fan))
            continue;
        if (!PhpHwFanSessionCurrent(RequestId))
            return;

        PhQueryPerformanceCounter(&fan->LastEvaluation);
        PhpHwFanLastEvaluation = fan->LastEvaluation;

        if (!fan->Probed)
        {
            if (fan->Kind == PhpHwAcpiDptfTemperature)
                PhpHwReadDptfStatic(RequestId, i);
            else if (fan->Kind == PhpHwAcpiPowerMeter)
                PhpHwReadMeterStatic(RequestId, i);
            else if (fan->Kind == PhpHwAcpiDptfPower)
                PhpHwReadDptfPowerStatic(RequestId, i);
            else if (fan->Kind == PhpHwAcpiDytc)
                PhpHwReadDytcStatic(RequestId, i);
            else
                PhpHwReadFanStatic(RequestId, i);

            PhQueryPerformanceCounter(&fan->LastEvaluation);
            PhpHwFanLastEvaluation = fan->LastEvaluation;
            continue;
        }

        switch (fan->Kind)
        {
        case PhpHwAcpiFan:
            status = PhpHwEvalFanOp(fan, PHP_HW_FAN_OP_FST, buffer, &returnLength);
            break;
        case PhpHwAcpiDptfTemperature:
            status = PhpHwEvalFanOp(fan, PHP_HW_DPTF_OP_TMP, buffer, &returnLength);
            break;
        case PhpHwAcpiPowerMeter:
            status = PhpHwEvalFanOp(fan, PHP_HW_METER_OP_PMM, buffer, &returnLength);
            break;
        case PhpHwAcpiDptfPower:
        case PhpHwAcpiAtkReading:
            status = PhpHwEvalFanOp(fan, fan->Op, buffer, &returnLength);
            break;
        default:
            status = STATUS_NOT_SUPPORTED;
            break;
        }

        if (PhpHwFanApplyStatus(fan, i, status))
        {
            PHP_HW_FAN_DECODE decode;

            if (fan->Kind == PhpHwAcpiFan)
            {
                decode = PhpHwDecodeFanStatus(buffer, returnLength, &rpm, &control);
                value = (FLOAT)rpm;
            }
            else if (fan->Kind == PhpHwAcpiDptfTemperature)
            {
                decode = PhpHwDecodeDptfTemperature(buffer, returnLength, &value);
            }
            else if (fan->Kind == PhpHwAcpiAtkReading)
            {
                decode = PhpHwDecodeAtkReading(fan, buffer, returnLength, &value);
            }
            else
            {
                decode = PhpHwDecodePowerMeterReading(buffer, returnLength, &value);
            }

            switch (decode)
            {
            case PhpHwFanDecodeOk:
                valid = TRUE;
                fan->Answered = TRUE;
                fan->Item.RetryCount = 0;
                fan->IntervalMs = fan->MeterIntervalMs;
                break;
            case PhpHwFanDecodeUnknown:
                // Firmware may report an unknown speed while the fan is spun down, or no reading yet.
                fan->Answered = TRUE;
                break;
            case PhpHwFanDecodeImplausible:
                fan->Answered = TRUE;
                if (++fan->Item.RetryCount >= PHP_HW_RETRY_LIMIT)
                {
                    fan->Item.Disabled = TRUE;
                    PhTraceInfo("Hardware sensors: ACPI device %lu unavailable: implausible reading", i);
                }
                break;
            case PhpHwFanDecodeMalformed:
                fan->Item.Disabled = TRUE;
                PhTraceInfo("Hardware sensors: ACPI device %lu unavailable: malformed result of %lu bytes", i, returnLength);
                break;
            }
        }

        PhpHwSetFanSensor(RequestId, i, valid, value);

        if (fan->Kind == PhpHwAcpiFan)
            PhpHwSetFanControlSensor(RequestId, i, valid && fan->FineGrain && control <= 100, (FLOAT)control);
    }
}

static BOOLEAN PhpHwFansRemaining(
    VOID
    )
{
    for (ULONG i = 0; i < PhpHwFanCount; i++)
    {
        if (!PhpHwFans[i].Item.Disabled)
            return TRUE;
    }

    return FALSE;
}

/**
 * Evaluates the ACPI fans, DPTF participants and power meters, each at most once per its interval.
 */
_Function_class_(USER_THREAD_START_ROUTINE)
static NTSTATUS NTAPI PhpHwFanWorker(
    _In_ PVOID Parameter
    )
{
    ULONG requestId = 0;
    BOOLEAN active = FALSE;

    PhSetThreadName(NtCurrentThread(), L"HardwareFan");
    PhSetThreadBasePriority(NtCurrentThread(), THREAD_PRIORITY_NORMAL);

    while (TRUE)
    {
        LARGE_INTEGER timeout;
        BOOLEAN requested;
        ULONG id;

        NtWaitForSingleObject(
            PhpHwFanEvent,
            FALSE,
            active ? PhTimeoutFromMilliseconds(&timeout, PhpHwFanWaitMs()) : NULL
            );

        PhAcquireQueuedLockShared(&PhpHwNamedSensorLock);
        requested = PhpHwFanRequested;
        id = PhpHwFanRequestId;
        PhReleaseQueuedLockShared(&PhpHwNamedSensorLock);

        if (!requested)
        {
            active = FALSE;
            continue;
        }

        if (id != requestId)
        {
            requestId = id;
            active = PhpHwFanStartSession(requestId);
        }

        if (!active)
            continue;

        PhpHwReadFans(requestId);

        if (!PhpHwFansRemaining())
        {
            PhTraceInfo("Hardware sensors: no ACPI device left");
            active = FALSE;
        }
    }

    return STATUS_SUCCESS;
}

/**
 * Starts an ACPI fan session; a previous session ends and its sensors are dropped.
 */
static BOOLEAN PhpHwRequestFans(
    _In_ const KPH_HW_CAPABILITIES* Capabilities
    )
{
    if (!PhpHwFanSupported(Capabilities) || !PhpHwFrequency.QuadPart)
        return FALSE;

    if (!PhpHwFanEvent)
    {
        if (!NT_SUCCESS(NtCreateEvent(&PhpHwFanEvent, EVENT_ALL_ACCESS, NULL, SynchronizationEvent, FALSE)))
            return FALSE;

        if (!NT_SUCCESS(PhCreateThread2(PhpHwFanWorker, NULL)))
        {
            NtClose(PhpHwFanEvent);
            PhpHwFanEvent = NULL;
            return FALSE;
        }
    }

    PhAcquireQueuedLockExclusive(&PhpHwNamedSensorLock);
    PhpHwFanClearSensorsLocked();
    PhpHwFanRequested = TRUE;
    PhpHwFanRequestId++;
    PhReleaseQueuedLockExclusive(&PhpHwNamedSensorLock);

    NtSetEvent(PhpHwFanEvent, NULL);
    return TRUE;
}

/**
 * Ends the ACPI fan session and drops its sensors; the worker publishes nothing more for it.
 */
static VOID PhpHwCancelFans(
    VOID
    )
{
    PhAcquireQueuedLockExclusive(&PhpHwNamedSensorLock);
    PhpHwFanRequested = FALSE;
    PhpHwFanClearSensorsLocked();
    PhReleaseQueuedLockExclusive(&PhpHwNamedSensorLock);

    if (PhpHwFanEvent)
        NtSetEvent(PhpHwFanEvent, NULL);
}

/**
 * \return PH_HW_CAP_FAN_SPEED, PH_HW_CAP_ACPI_TEMP, PH_HW_CAP_ACPI_POWER and PH_HW_CAP_FAN_CONTROL for the active sensors.
 */
static ULONG PhpHwFanCapabilities(
    VOID
    )
{
    ULONG capabilities = 0;

    PhAcquireQueuedLockShared(&PhpHwNamedSensorLock);

    for (ULONG i = 0; i < PhpHwFanSensorCount; i++)
    {
        if (PhpHwFanSensors[i].Active)
        {
            switch (PhpHwFanSensors[i].Kind)
            {
            case PhHwNamedSensorFanSpeed:
                capabilities |= PH_HW_CAP_FAN_SPEED;
                break;
            case PhHwNamedSensorAcpiTemperature:
            case PhHwNamedSensorAcpiVoltage:
                capabilities |= PH_HW_CAP_ACPI_TEMP;
                break;
            default:
                capabilities |= PH_HW_CAP_ACPI_POWER;
                break;
            }
        }
        if (PhpHwFanControlSensors[i].Active)
            capabilities |= PH_HW_CAP_FAN_CONTROL;
    }

    PhReleaseQueuedLockShared(&PhpHwNamedSensorLock);

    return capabilities;
}

//
// Windows performance counters (Perflib V2): ACPI thermal zones, energy meters and processor limits. The kernel
// re-reads every zone for each sample and waits for it without a timeout, so the query runs on its own worker. It
// needs no driver; the setting alone enables it.
//

#define PHP_HW_COUNTER_MAX_ITEMS 256
#define PHP_HW_COUNTER_MAX_SENSORS (PHP_HW_COUNTER_MAX_ITEMS * 2)
#define PHP_HW_COUNTER_VALUES 4
#define PHP_HW_COUNTER_ZONE_MINIMUM 2732 // tenths of a kelvin; at or below 0 C is a placeholder (0, 2330 = -40 C)
#define PHP_HW_COUNTER_ZONE_MAXIMUM 4500 // tenths of a kelvin; above 177 C is not a temperature (MPTF \_TZ.PSOC, PSYS)
#define PHP_HW_COUNTER_ZONE_DUMMY 3000 // 300.0 K, the constant that firmware dummy zones return
#define PHP_HW_COUNTER_RETRY_TICKS 30 // updates between attempts to open the query after a failure
#define PHP_HW_NAMED_SET_COUNTERS 7 // the named sensor set of the Windows counters; the sets before it are the driver's

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

// Worker state.
static HANDLE PhpHwCounterEvent = NULL;
static ULONG PhpHwCounterSession = 0; // the request the worker serves
static HANDLE PhpHwCounterQuery = NULL;
static ULONG PhpHwCounterAdded[RTL_NUMBER_OF(PhpHwCounters)]; // PhpHwCounters index of each sample block; ULONG_MAX when unknown
static ULONG PhpHwCounterAddedCount = 0;
static ULONG PhpHwCounterItemCount = 0;
static PHP_HW_COUNTER_ITEM PhpHwCounterItems[PHP_HW_COUNTER_MAX_ITEMS];
static ULONG PhpHwCounterBlockCount = 0;
static PHP_HW_COUNTER_BLOCK PhpHwCounterBlocks[RTL_NUMBER_OF(PhpHwCounters)];
static ULONG PhpHwCounterZoneLimitIndex = ULONG_MAX; // the lowest zone passive limit sensor

// Under PhpHwNamedSensorLock.
static BOOLEAN PhpHwCounterRequested = FALSE;
static ULONG PhpHwCounterRequestId = 0;
static ULONG PhpHwCounterSensorCount = 0;
static PH_HW_NAMED_SENSOR PhpHwCounterSensors[PHP_HW_COUNTER_MAX_SENSORS];
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
 * Determines whether the worker's request is still the requested one; a toggle during a sample drops the sample.
 * The caller holds the named sensor lock.
 */
_Requires_lock_held_(PhpHwNamedSensorLock)
static BOOLEAN PhpHwCounterCurrentLocked(
    VOID
    )
{
    return PhpHwCounterRequested && PhpHwCounterRequestId == PhpHwCounterSession;
}

/**
 * Adds a counter sensor. Called on the counter worker.
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

    if (PhpHwCounterCurrentLocked() && PhpHwCounterSensorCount < PHP_HW_COUNTER_MAX_SENSORS)
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
 * Closes the query, forgets the instances and drops their sensors. Called on the counter worker, which owns the
 * sensor table.
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
 * Opens the query and adds every counter whose set is registered. Called on the counter worker.
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
 * Finds or adds the instance of a set. Called on the counter worker.
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
 * Stores the instances of one counter of a sample. Called on the counter worker.
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
 * Publishes one sample. Called on the counter worker.
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
            if ((item->Seen & 1) && item->Values[0] >= PHP_HW_COUNTER_ZONE_MINIMUM &&
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

        if (PhpHwCounterCurrentLocked() && item->SensorIndex < PhpHwCounterSensorCount)
        {
            PhpHwCounterSensors[item->SensorIndex].Valid = valid;

            if (valid)
                PhpHwCounterSensors[item->SensorIndex].Value = value;
        }

        if (PhpHwCounterCurrentLocked() && item->LimitIndex < PhpHwCounterSensorCount)
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

    if (PhpHwCounterCurrentLocked())
    {
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
    }

    PhReleaseQueuedLockExclusive(&PhpHwNamedSensorLock);
}

/**
 * Takes one sample of the counters. Called on the counter worker.
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
 * Samples the Windows counters once per provider update while requested.
 */
_Function_class_(USER_THREAD_START_ROUTINE)
static NTSTATUS NTAPI PhpHwCounterWorker(
    _In_ PVOID Parameter
    )
{
    BOOLEAN active = FALSE;
    ULONG retryTicks = 0;

    PhSetThreadName(NtCurrentThread(), L"HardwareCounters");
    PhSetThreadBasePriority(NtCurrentThread(), THREAD_PRIORITY_NORMAL);

    while (TRUE)
    {
        BOOLEAN requested;
        ULONG id;

        NtWaitForSingleObject(PhpHwCounterEvent, FALSE, NULL);

        PhAcquireQueuedLockShared(&PhpHwNamedSensorLock);
        requested = PhpHwCounterRequested;
        id = PhpHwCounterRequestId;
        PhReleaseQueuedLockShared(&PhpHwNamedSensorLock);

        if (!requested)
        {
            if (active)
                PhpHwCounterClose();

            active = FALSE;
            continue;
        }

        // A new request opens the query; a failed open is tried again every PHP_HW_COUNTER_RETRY_TICKS updates.
        if (id != PhpHwCounterSession || (!active && ++retryTicks >= PHP_HW_COUNTER_RETRY_TICKS))
        {
            PhpHwCounterSession = id;
            retryTicks = 0;
            active = PhpHwCounterOpen();
        }

        if (active)
            PhpHwCounterSample();
    }

    return STATUS_SUCCESS;
}

/**
 * Starts, samples or stops the Windows counters. Called by the provider on each update.
 *
 * \param Enabled TRUE while hardware sensors are enabled in the settings.
 */
static VOID PhpHwUpdateCounters(
    _In_ BOOLEAN Enabled
    )
{
    static BOOLEAN started = FALSE;
    BOOLEAN changed = FALSE;

    if (Enabled && !PhpHwCounterEvent)
    {
        if (!NT_SUCCESS(NtCreateEvent(&PhpHwCounterEvent, EVENT_ALL_ACCESS, NULL, SynchronizationEvent, FALSE)))
            return;

        if (!NT_SUCCESS(PhCreateThread2(PhpHwCounterWorker, NULL)))
        {
            NtClose(PhpHwCounterEvent);
            PhpHwCounterEvent = NULL;
            return;
        }
    }

    if (!PhpHwCounterEvent)
        return;

    if (Enabled != started)
    {
        PhAcquireQueuedLockExclusive(&PhpHwNamedSensorLock);
        PhpHwCounterRequested = Enabled;

        if (Enabled)
            PhpHwCounterRequestId++;

        PhReleaseQueuedLockExclusive(&PhpHwNamedSensorLock);
        started = Enabled;
        changed = TRUE;
    }

    // Each update while enabled is one sample; a stop wakes the worker to close the query.
    if (Enabled || changed)
        NtSetEvent(PhpHwCounterEvent, NULL);
}

/**
 * Retrieves the hottest current ACPI thermal zone, the package temperature fallback on ARM64.
 */
_Success_(return)
static BOOLEAN PhpHwCounterHottestZone(
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
static BOOLEAN PhpHwCounterZoneEverSeen(
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
BOOLEAN PhHwCounterSensorsPresent(
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
 */
static VOID PhpHwWriteCounterDiagnostics(
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

static BOOLEAN PhpHwTpmiSupported(
    _In_ const KPH_HW_CAPABILITIES* Capabilities
    )
{
    return PhpHwIntel &&
        FlagOn(Capabilities->Flags, KPH_HW_CAP_PCI_AVAILABLE) &&
        FlagOn(Capabilities->ImplementedKinds, 1ULL << KPH_DYN_HW_KIND_VSEC_READ);
}

static VOID PhpHwInitializeTpmiEntry(
    _Out_ PKPH_HW_WINDOW_BATCH_ENTRY Entry,
    _In_ ULONG Offset
    )
{
    memset(Entry, 0, sizeof(KPH_HW_WINDOW_BATCH_ENTRY));
    Entry->Offset = Offset;
    Entry->Width = 8;
    Entry->Status = PHP_HW_ENTRY_UNSET;
}

/**
 * Reads qwords of a TPMI feature instance; a message-level failure becomes the status of every entry.
 */
static VOID PhpHwReadTpmi(
    _In_ PKPH_HW_PCI_LOCATOR Locator,
    _In_ UCHAR OpIndex,
    _In_ USHORT Instance,
    _Inout_updates_(Count) PKPH_HW_WINDOW_BATCH_ENTRY Entries,
    _In_ ULONG Count
    )
{
    NTSTATUS status;

    status = KphHwReadVsecBatch(Locator, OpIndex, Instance, Entries, Count);

    for (ULONG i = 0; i < Count; i++)
    {
        if (Entries[i].Status == PHP_HW_ENTRY_UNSET && !NT_SUCCESS(status))
            Entries[i].Status = status;
    }
}

/**
 * Decodes a TPMI RAPL UNIT register: 1/2^ESU joules, ESU [10:6].
 */
static FLOAT PhpHwTpmiEnergyUnitJoules(
    _In_ ULONG64 Unit
    )
{
    return 1.0f / (FLOAT)(1ULL << ((Unit >> PH_INTEL_TPMI_RAPL_ENERGY_UNIT_SHIFT) & 0x1F));
}

/**
 * Maps a TPMI device to a package through the INFO feature (package ID [23:16]).
 *
 * \return The package index, or ULONG_MAX when it cannot be determined.
 */
static ULONG PhpHwTpmiDevicePackage(
    _In_ PKPH_HW_PCI_LOCATOR Locator,
    _In_reads_(Count) const KPH_HW_VSEC_REGION* Regions,
    _In_ ULONG Count
    )
{
    ULONG package = ULONG_MAX;

    for (ULONG i = 0; i < Count; i++)
    {
        KPH_HW_WINDOW_BATCH_ENTRY entry;
        ULONG id;

        if (Regions[i].Discovery != KPH_DYN_HW_VSEC_DISCOVERY_TPMI || Regions[i].Id != PH_INTEL_TPMI_ID_INFO)
            continue;

        PhpHwInitializeTpmiEntry(&entry, PH_INTEL_TPMI_INFO_BUS_INFO);
        PhpHwReadTpmi(Locator, Regions[i].OpIndex, Regions[i].Instance, &entry, 1);

        if (!NT_SUCCESS(entry.Status) || entry.Value == MAXULONG64)
            return ULONG_MAX;

        id = (ULONG)((entry.Value >> 16) & 0xFF);

        if (package != ULONG_MAX && package != id)
            return ULONG_MAX;

        package = id;
    }

    return package < PhpHwPackageCount ? package : ULONG_MAX;
}

/**
 * Starts a TPMI region at the end of the table; it is kept only when PhpHwTpmiRegionCount is advanced.
 */
static PPHP_HW_TPMI_REGION PhpHwBeginTpmiRegion(
    _In_ PKPH_HW_PCI_LOCATOR Locator,
    _In_ PKPH_HW_VSEC_REGION Region
    )
{
    PPHP_HW_TPMI_REGION region;

    if (PhpHwTpmiRegionCount >= PHP_HW_MAX_TPMI_REGIONS)
        return NULL;

    region = &PhpHwTpmiRegions[PhpHwTpmiRegionCount];
    memset(region, 0, sizeof(PHP_HW_TPMI_REGION));
    region->Locator = *Locator;
    region->OpIndex = Region->OpIndex;
    region->Instance = Region->Instance;
    region->FirstEntry = PhpHwTpmiEntryCount;

    return region;
}

static PPHP_HW_TPMI_ENTRY PhpHwAddTpmiEntry(
    _Inout_ PPHP_HW_TPMI_REGION Region,
    _In_ ULONG Offset,
    _In_ PHP_HW_TPMI_ENTRY_KIND Kind,
    _In_ ULONG PackageIndex
    )
{
    PPHP_HW_TPMI_ENTRY entry;

    if (PhpHwTpmiEntryCount >= PHP_HW_MAX_TPMI_ENTRIES || Region->EntryCount >= PHP_HW_TPMI_REGION_ENTRIES)
        return NULL;

    entry = &PhpHwTpmiEntries[PhpHwTpmiEntryCount++];
    memset(entry, 0, sizeof(PHP_HW_TPMI_ENTRY));
    entry->Offset = Offset;
    entry->Kind = Kind;
    entry->PackageIndex = PackageIndex;
    entry->NamedIndex = ULONG_MAX;
    Region->EntryCount++;

    return entry;
}

static VOID PhpHwDropTpmiEntry(
    _Inout_ PPHP_HW_TPMI_REGION Region
    )
{
    PhpHwTpmiEntryCount--;
    Region->EntryCount--;
}

/**
 * Adds the energy counters of a TPMI RAPL instance whose MSRs are unavailable: 128-byte domains, each
 * a header qword (version [7:0], type [15:8], size [23:16] in 128 bytes, register flags [47:32])
 * followed by register n at +8n.
 */
static VOID PhpHwProbeTpmiRapl(
    _In_ ULONG DeviceIndex,
    _In_ PKPH_HW_PCI_LOCATOR Locator,
    _In_ PKPH_HW_VSEC_REGION Region,
    _In_ ULONG PackageIndex
    )
{
    KPH_HW_WINDOW_BATCH_ENTRY entries[PHP_HW_TPMI_REGION_ENTRIES * 2];
    ULONG domains[PHP_HW_TPMI_REGION_ENTRIES];
    PHP_HW_TPMI_ENTRY_KIND kinds[PHP_HW_TPMI_REGION_ENTRIES];
    ULONG domainCount = min(Region->Size / PH_INTEL_TPMI_RAPL_DOMAIN_SIZE, PHP_HW_TPMI_REGION_ENTRIES);
    PPHP_HW_TPMI_REGION region;
    ULONG count = 0;

    if (domainCount == 0)
        return;

    for (ULONG i = 0; i < domainCount; i++)
        PhpHwInitializeTpmiEntry(&entries[i], i * PH_INTEL_TPMI_RAPL_DOMAIN_SIZE);

    PhpHwReadTpmi(Locator, Region->OpIndex, Region->Instance, entries, domainCount);

    // Parsing stops at the first invalid or unknown domain.
    for (ULONG i = 0; i < domainCount; i++)
    {
        ULONG64 header = entries[i].Value;
        ULONG flags = (ULONG)((header >> 32) & 0xFFFF);
        PHP_HW_TPMI_ENTRY_KIND kind;
        PHP_HW_REG msr;

        if (!NT_SUCCESS(entries[i].Status) || header == MAXULONG64 || (header & 0xFF) == 0xFF)
            break;
        if (((header & 0xFF) >> 5) != 0 || ((header >> 16) & 0xFF) != 1)
            break;
        if (!(flags & (1UL << PH_INTEL_TPMI_RAPL_REG_UNIT)) || !(flags & (1UL << PH_INTEL_TPMI_RAPL_REG_ENERGY_STATUS)))
            break;

        switch ((header >> 8) & 0xFF)
        {
        case PH_INTEL_TPMI_RAPL_DOMAIN_SYSTEM:
            if (!(flags & (1UL << PH_INTEL_TPMI_RAPL_REG_DOMAIN_INFO)))
                continue;
            kind = PhpHwTpmiPlatformEnergy;
            msr = PhpHwRegPlatformEnergy;
            break;
        case PH_INTEL_TPMI_RAPL_DOMAIN_PACKAGE:
            kind = PhpHwTpmiPackageEnergy;
            msr = PhpHwRegPackageEnergy;
            break;
        case PH_INTEL_TPMI_RAPL_DOMAIN_MEMORY:
            kind = PhpHwTpmiDramEnergy;
            msr = PhpHwRegDramEnergy;
            break;
        default:
            kind = PhpHwTpmiEnergyMaximum; // unknown: stop
            msr = PhpHwRegMaximum;
            break;
        }

        if (kind == PhpHwTpmiEnergyMaximum)
            break;

        // The MSRs serve the package totals where they exist (GNR/SRF); TPMI stands in without them (DMR).
        if (PhpHwRegisterActive(msr))
            continue;

        domains[count] = i * PH_INTEL_TPMI_RAPL_DOMAIN_SIZE;
        kinds[count] = kind;
        count++;
    }

    if (count == 0)
        return;

    for (ULONG i = 0; i < count; i++)
    {
        PhpHwInitializeTpmiEntry(&entries[i * 2], domains[i] + PH_INTEL_TPMI_RAPL_REG_UNIT * 8);
        PhpHwInitializeTpmiEntry(&entries[i * 2 + 1], domains[i] + PH_INTEL_TPMI_RAPL_REG_DOMAIN_INFO * 8);
    }

    PhpHwReadTpmi(Locator, Region->OpIndex, Region->Instance, entries, count * 2);

    if (!(region = PhpHwBeginTpmiRegion(Locator, Region)))
        return;

    for (ULONG i = 0; i < count; i++)
    {
        PKPH_HW_WINDOW_BATCH_ENTRY unit = &entries[i * 2];
        PKPH_HW_WINDOW_BATCH_ENTRY domainInfo = &entries[i * 2 + 1];
        PPHP_HW_TPMI_ENTRY entry;

        if (!NT_SUCCESS(unit->Status) || unit->Value == MAXULONG64)
            continue;
        // Only the root domain enumerates the platform (PSYS) domain.
        if (kinds[i] == PhpHwTpmiPlatformEnergy && (!NT_SUCCESS(domainInfo->Status) || domainInfo->Value == MAXULONG64 || !(domainInfo->Value & 0x1)))
            continue;
        if (!(entry = PhpHwAddTpmiEntry(region, domains[i] + PH_INTEL_TPMI_RAPL_REG_ENERGY_STATUS * 8, kinds[i], PackageIndex)))
            break;

        entry->EnergyUnit = PhpHwTpmiEnergyUnitJoules(unit->Value);

        if (PackageIndex == ULONG_MAX)
        {
            static CONST PCWSTR names[PhpHwTpmiEnergyMaximum] = { L"Package", L"DRAM", L"Platform" };
            PH_FORMAT format[4];
            PPH_STRING name;

            PhInitFormatS(&format[0], (PWSTR)names[kinds[i]]);
            PhInitFormatS(&format[1], L" (TPMI ");
            PhInitFormatU(&format[2], DeviceIndex);
            PhInitFormatC(&format[3], L')');
            name = PhFormat(format, RTL_NUMBER_OF(format), 0);
            entry->NamedIndex = PhpHwAddNamedSensor(PhHwNamedSensorTpmiPower, 0, PhpHwTpmiEntryCount - 1, name);
            PhDereferenceObject(name);

            if (entry->NamedIndex == ULONG_MAX)
            {
                PhpHwDropTpmiEntry(region);
                break;
            }
        }
    }

    // The package domains of the partitions of one package are summed.
    if (region->EntryCount)
        PhpHwTpmiRegionCount++;
}

/**
 * Adds the cluster status registers of a TPMI uncore instance: header qword (version [7:0], cluster
 * mask [15:8]), cluster offsets at +08h (a byte per cluster, in qwords), ratio [6:0] in 100 MHz.
 */
static VOID PhpHwProbeTpmiUncore(
    _In_ PKPH_HW_PCI_LOCATOR Locator,
    _In_ PKPH_HW_VSEC_REGION Region,
    _In_ ULONG PackageIndex,
    _Inout_ PULONG DomainIndex
    )
{
    KPH_HW_WINDOW_BATCH_ENTRY entries[2];
    PPHP_HW_TPMI_REGION region;
    ULONG64 header;
    ULONG clusterCount = 0;
    ULONG domain;

    PhpHwInitializeTpmiEntry(&entries[0], 0);
    PhpHwInitializeTpmiEntry(&entries[1], PH_INTEL_TPMI_UNCORE_CLUSTER_OFFSET);
    PhpHwReadTpmi(Locator, Region->OpIndex, Region->Instance, entries, 2);

    if (!NT_SUCCESS(entries[0].Status) || !NT_SUCCESS(entries[1].Status))
        return;

    // A read-blocked feature reads all-ones.
    header = entries[0].Value;

    if (header == MAXULONG64 || ((header & 0xFF) >> 5) != 0)
        return;

    for (ULONG i = 0; i < 8; i++)
    {
        if (header & (1ULL << (8 + i)))
            clusterCount++;
    }

    if (clusterCount == 0 || !(region = PhpHwBeginTpmiRegion(Locator, Region)))
        return;

    domain = (*DomainIndex)++;

    for (ULONG i = 0; i < clusterCount; i++)
    {
        ULONG offset = (ULONG)((entries[1].Value >> (i * 8)) & 0xFF) * 8;
        PPHP_HW_TPMI_ENTRY entry;
        PH_FORMAT format[4];
        ULONG count = 2;
        PPH_STRING name;

        if (offset == 0 || offset + 8 > Region->Size)
            continue;
        if (!(entry = PhpHwAddTpmiEntry(region, offset, PhpHwTpmiUncoreStatus, PackageIndex)))
            break;

        PhInitFormatS(&format[0], PackageIndex == ULONG_MAX ? L"Uncore (unmapped) " : L"Uncore ");
        PhInitFormatU(&format[1], domain);

        if (clusterCount > 1)
        {
            PhInitFormatC(&format[2], L'.');
            PhInitFormatU(&format[3], i);
            count = 4;
        }

        name = PhFormat(format, count, 0);
        // Unmapped domains share package 0 with its mapped ones; their ids are kept apart.
        entry->NamedIndex = PhpHwAddNamedSensor(
            PhHwNamedSensorUncoreClock,
            PackageIndex == ULONG_MAX ? 0 : PackageIndex,
            (PackageIndex == ULONG_MAX ? PHP_HW_TPMI_UNMAPPED_ID : 0) | (domain << 3) | i,
            name
            );
        PhDereferenceObject(name);

        if (entry->NamedIndex == ULONG_MAX)
        {
            PhpHwDropTpmiEntry(region);
            break;
        }
    }

    if (region->EntryCount)
        PhpHwTpmiRegionCount++;
}

/**
 * Records the active SST-PP level of a TPMI SST instance for the report: its TDP, TjMax and core ratios.
 */
static VOID PhpHwProbeTpmiSst(
    _In_ PKPH_HW_PCI_LOCATOR Locator,
    _In_ const KPH_HW_VSEC_REGION* Region,
    _In_ ULONG PackageIndex
    )
{
    KPH_HW_WINDOW_BATCH_ENTRY entries[3];
    PPHP_HW_TPMI_REPORT report;
    ULONG64 header;
    ULONG pp;
    ULONG level;
    ULONG block;
    BOOLEAN locked;

    if (PhpHwTpmiReportCount >= PHP_HW_MAX_TPMI_REPORTS)
    {
        PhpHwTpmiReportsOmitted++;
        return;
    }

    PhpHwInitializeTpmiEntry(&entries[0], 0);
    PhpHwReadTpmi(Locator, Region->OpIndex, Region->Instance, entries, 1);

    // Interface version [7:0]: FFh is invalid, major [7:5] must be 0. SST-PP is cap bit 9.
    if (
        !NT_SUCCESS(entries[0].Status) ||
        (UCHAR)entries[0].Value == 0xFF ||
        ((entries[0].Value >> 5) & 0x7) ||
        !(entries[0].Value & 0x200)
        )
    {
        return;
    }

    header = entries[0].Value;
    pp = (ULONG)((header >> 24) & 0xFF) * 8;

    PhpHwInitializeTpmiEntry(&entries[0], pp + PH_INTEL_TPMI_SST_PP_FEATURES);
    PhpHwInitializeTpmiEntry(&entries[1], pp + PH_INTEL_TPMI_SST_PP_LEVELS);
    PhpHwInitializeTpmiEntry(&entries[2], pp + PH_INTEL_TPMI_SST_PP_STATUS);
    PhpHwReadTpmi(Locator, Region->OpIndex, Region->Instance, entries, 3);

    for (ULONG i = 0; i < 3; i++)
    {
        if (!NT_SUCCESS(entries[i].Status) || entries[i].Value == MAXULONG64)
            return;
    }

    level = (ULONG)(entries[2].Value & 0x7);
    locked = !!(entries[2].Value & 0x8);

    if (level > PH_INTEL_TPMI_SST_PP_MAX_LEVEL)
        return;

    // Level n is at byte n of the level offsets, the PP feature at byte 0 of the feature offsets; both in qwords.
    block = pp + (ULONG)((entries[1].Value >> (level * 8)) & 0xFF) * 8 + (ULONG)(entries[0].Value & 0xFF) * 8;

    PhpHwInitializeTpmiEntry(&entries[0], block + PH_INTEL_TPMI_SST_PP_INFO_1);
    PhpHwInitializeTpmiEntry(&entries[1], block + PH_INTEL_TPMI_SST_PP_INFO_11);
    PhpHwReadTpmi(Locator, Region->OpIndex, Region->Instance, entries, 2);

    for (ULONG i = 0; i < 2; i++)
    {
        if (!NT_SUCCESS(entries[i].Status) || entries[i].Value == MAXULONG64)
            return;
    }

    report = &PhpHwTpmiReports[PhpHwTpmiReportCount++];
    memset(report, 0, sizeof(PHP_HW_TPMI_REPORT));
    report->Feature = PH_INTEL_TPMI_ID_SST;
    report->Version = (UCHAR)header;
    report->Level = (UCHAR)level;
    report->Locked = locked;
    report->PackageIndex = PackageIndex;
    report->Instance = Region->Instance;
    report->Value = entries[0].Value;
    report->Ratios = entries[1].Value;
}

/**
 * Records the die-level reasons of a TPMI PLR instance for the report; they hold until cleared.
 */
static VOID PhpHwProbeTpmiPlr(
    _In_ PKPH_HW_PCI_LOCATOR Locator,
    _In_ const KPH_HW_VSEC_REGION* Region,
    _In_ ULONG PackageIndex
    )
{
    KPH_HW_WINDOW_BATCH_ENTRY entries[2];
    PPHP_HW_TPMI_REPORT report;

    if (PhpHwTpmiReportCount >= PHP_HW_MAX_TPMI_REPORTS)
    {
        PhpHwTpmiReportsOmitted++;
        return;
    }

    PhpHwInitializeTpmiEntry(&entries[0], 0);
    PhpHwInitializeTpmiEntry(&entries[1], PH_INTEL_TPMI_PLR_DIE_LEVEL);
    PhpHwReadTpmi(Locator, Region->OpIndex, Region->Instance, entries, 2);

    // An all-ones header is an invalid instance.
    if (!NT_SUCCESS(entries[0].Status) || entries[0].Value == MAXULONG64 || !NT_SUCCESS(entries[1].Status))
        return;

    report = &PhpHwTpmiReports[PhpHwTpmiReportCount++];
    memset(report, 0, sizeof(PHP_HW_TPMI_REPORT));
    report->Feature = PH_INTEL_TPMI_ID_PLR;
    report->PackageIndex = PackageIndex;
    report->Instance = Region->Instance;
    report->Value = entries[1].Value;
}

/**
 * Finds the OOBMSM devices and adds their TPMI RAPL and uncore registers; records SST and PLR for the report.
 */
static VOID PhpHwProbeTpmi(
    _In_ const KPH_HW_CAPABILITIES* Capabilities
    )
{
    static CONST PH_STRINGREF anyClass = PH_STRINGREF_INIT(L"PCI\\CC_");
    static CONST USHORT oobmsmIds[] = { 0x09A7, 0x09A1 }; // SPR/EMR/GNR/SRF, DMR
    PKPH_HW_VSEC_REGION regions;
    PULONG domainIndex;
    ULONG unmappedDomainIndex = 0;

    PhpHwTpmiRegionCount = 0;
    PhpHwTpmiEntryCount = 0;
    PhpHwTpmiReportCount = 0;
    PhpHwTpmiReportsOmitted = 0;

    if (!PhpHwTpmiSupported(Capabilities))
        return;

    if (!PhpHwTpmiEnumerated)
    {
        PhpHwTpmiEnumerated = TRUE;
        PhpHwTpmiLocatorCount = PhpHwFindPciDevices(
            PHP_HW_INTEL_VENDOR_ID,
            PHP_HW_INTEL_VENDOR_ID,
            &anyClass,
            oobmsmIds,
            RTL_NUMBER_OF(oobmsmIds),
            FALSE,
            PhpHwTpmiLocators,
            RTL_NUMBER_OF(PhpHwTpmiLocators)
            );

        PhTraceInfo("Hardware sensors: %lu OOBMSM devices", PhpHwTpmiLocatorCount);
    }

    if (PhpHwTpmiLocatorCount == 0)
        return;

    regions = PhAllocate(sizeof(KPH_HW_VSEC_REGION) * KPH_DYN_HW_VSEC_MAX_TABLE_REGIONS);
    domainIndex = PhAllocateZero(sizeof(ULONG) * PhpHwPackageCount);

    for (ULONG i = 0; i < PhpHwTpmiLocatorCount; i++)
    {
        PKPH_HW_PCI_LOCATOR locator = &PhpHwTpmiLocators[i];
        ULONG regionCount = 0;
        ULONG packageIndex;
        NTSTATUS status;

        status = KphHwQueryVsecRegions(locator, regions, KPH_DYN_HW_VSEC_MAX_TABLE_REGIONS, &regionCount);

        if (!NT_SUCCESS(status))
        {
            PhTraceInfo("Hardware sensors: TPMI device %lu unavailable: %!STATUS!", i, status);
            continue;
        }

        regionCount = min(regionCount, KPH_DYN_HW_VSEC_MAX_TABLE_REGIONS);
        packageIndex = PhpHwTpmiDevicePackage(locator, regions, regionCount);

        if (packageIndex == ULONG_MAX && PhpHwPackageCount == 1)
            packageIndex = 0;

        for (ULONG j = 0; j < regionCount; j++)
        {
            if (regions[j].Discovery != KPH_DYN_HW_VSEC_DISCOVERY_TPMI)
                continue;

            switch (regions[j].Id)
            {
            case PH_INTEL_TPMI_ID_RAPL:
                PhpHwProbeTpmiRapl(i, locator, &regions[j], packageIndex);
                break;
            case PH_INTEL_TPMI_ID_UNCORE:
                PhpHwProbeTpmiUncore(
                    locator,
                    &regions[j],
                    packageIndex,
                    packageIndex == ULONG_MAX ? &unmappedDomainIndex : &domainIndex[packageIndex]
                    );
                break;
            case PH_INTEL_TPMI_ID_SST:
                PhpHwProbeTpmiSst(locator, &regions[j], packageIndex);
                break;
            case PH_INTEL_TPMI_ID_PLR:
                PhpHwProbeTpmiPlr(locator, &regions[j], packageIndex);
                break;
            }
        }
    }

    PhFree(domainIndex);
    PhFree(regions);

    PhTraceInfo("Hardware sensors: %lu TPMI regions, %lu registers", PhpHwTpmiRegionCount, PhpHwTpmiEntryCount);
}

// Intel PMT telemetry samples by aggregator GUID (Intel-PMT XML): qword index, field, transform.
typedef enum _PHP_HW_PMT_TRANSFORM
{
    PhpHwPmtTemperature, // signed 8-bit degrees Celsius
    PhpHwPmtRatio100, // x 100 MHz
    PhpHwPmtRatio33 // x 33.33 MHz
} PHP_HW_PMT_TRANSFORM;

typedef struct _PHP_HW_PMT_SAMPLE
{
    ULONG Guid;
    USHORT Qword;
    UCHAR Lsb;
    UCHAR Transform; // PHP_HW_PMT_TRANSFORM
    PCWSTR Name;
} PHP_HW_PMT_SAMPLE, *PPHP_HW_PMT_SAMPLE;
typedef const PHP_HW_PMT_SAMPLE* PCPHP_HW_PMT_SAMPLE;

static CONST PHP_HW_PMT_SAMPLE PhpHwPmtSamples[] =
{
    // MTL PUNIT normal aggregator.
    { 0x130670B2, 11, 48, PhpHwPmtRatio33, L"Memory clock (QCLK)" },
    { 0x130670B2, 19, 0, PhpHwPmtTemperature, L"PCH" },
    { 0x130670B2, 19, 8, PhpHwPmtTemperature, L"DRAM" },
    { 0x130670B2, 19, 16, PhpHwPmtTemperature, L"System agent" },
    { 0x130670B2, 19, 24, PhpHwPmtTemperature, L"IPU" },
    { 0x130670B2, 19, 32, PhpHwPmtTemperature, L"Display engine" },
    { 0x130670B2, 19, 40, PhpHwPmtTemperature, L"VPU" },
    { 0x130670B2, 19, 48, PhpHwPmtTemperature, L"Media" },
    // MTL DMU normal aggregator: CORE_TEMPERATURE0-5, 32-bit DTS0-3 groups from q9 [63:32]; DTS1 is the maximum.
    { 0x1A067002, 9, 40, PhpHwPmtTemperature, L"P-core 0" },
    { 0x1A067002, 10, 8, PhpHwPmtTemperature, L"P-core 1" },
    { 0x1A067002, 10, 40, PhpHwPmtTemperature, L"P-core 2" },
    { 0x1A067002, 11, 8, PhpHwPmtTemperature, L"P-core 3" },
    { 0x1A067002, 11, 40, PhpHwPmtTemperature, L"P-core 4" },
    { 0x1A067002, 12, 8, PhpHwPmtTemperature, L"P-core 5" },
    // LNL.
    { 0x03072005, 2, 0, PhpHwPmtRatio33, L"Memory clock (MEMSS)" },
    { 0x03072005, 14, 0, PhpHwPmtTemperature, L"PCH" },
    { 0x03072005, 14, 16, PhpHwPmtTemperature, L"System agent" },
    { 0x03072005, 14, 24, PhpHwPmtTemperature, L"IPU" },
    { 0x03072005, 14, 32, PhpHwPmtTemperature, L"Display engine" },
    { 0x03072005, 14, 40, PhpHwPmtTemperature, L"VPU" },
    { 0x03072005, 14, 48, PhpHwPmtTemperature, L"Media" },
    { 0x03072005, 28, 0, PhpHwPmtRatio100, L"Core 0 average" },
    { 0x03072005, 28, 8, PhpHwPmtRatio100, L"Core 1 average" },
    { 0x03072005, 28, 16, PhpHwPmtRatio100, L"Core 2 average" },
    { 0x03072005, 28, 24, PhpHwPmtRatio100, L"Core 3 average" },
    { 0x03072005, 28, 32, PhpHwPmtRatio100, L"Core 4 average" },
    { 0x03072005, 28, 40, PhpHwPmtRatio100, L"Core 5 average" },
    { 0x03072005, 28, 48, PhpHwPmtRatio100, L"Core 6 average" },
    { 0x03072005, 28, 56, PhpHwPmtRatio100, L"Core 7 average" },
    // PTL and WCL share the q15 temperatures; PTL q2 is reserved.
    { 0x03086000, 15, 0, PhpHwPmtTemperature, L"PCH" },
    { 0x03086000, 15, 8, PhpHwPmtTemperature, L"System agent" },
    { 0x03086000, 15, 16, PhpHwPmtTemperature, L"IPU" },
    { 0x03086000, 15, 24, PhpHwPmtTemperature, L"Display engine" },
    { 0x03086000, 15, 32, PhpHwPmtTemperature, L"VPU" },
    { 0x03086000, 15, 40, PhpHwPmtTemperature, L"Media" },
    { 0x0308D000, 2, 0, PhpHwPmtRatio33, L"Memory clock (MEMSS)" },
    { 0x0308D000, 15, 0, PhpHwPmtTemperature, L"PCH" },
    { 0x0308D000, 15, 8, PhpHwPmtTemperature, L"System agent" },
    { 0x0308D000, 15, 16, PhpHwPmtTemperature, L"IPU" },
    { 0x0308D000, 15, 24, PhpHwPmtTemperature, L"Display engine" },
    { 0x0308D000, 15, 32, PhpHwPmtTemperature, L"VPU" },
    { 0x0308D000, 15, 40, PhpHwPmtTemperature, L"Media" },
};

/**
 * Applies the transform of a PMT sample to its qword.
 *
 * \return FALSE when the field is not a reading: a read-blocked (all-ones) qword, a temperature outside 1-125 C, or a
 * zero ratio.
 */
_Success_(return)
static BOOLEAN PhpHwDecodePmtSample(
    _In_ PCPHP_HW_PMT_SAMPLE Sample,
    _In_ ULONG64 Qword,
    _Out_ PFLOAT Value
    )
{
    UCHAR field = (UCHAR)(Qword >> Sample->Lsb);

    if (Qword == MAXULONG64)
        return FALSE;

    switch (Sample->Transform)
    {
    case PhpHwPmtTemperature:
        if ((SCHAR)field <= 0 || (SCHAR)field > 125)
            return FALSE;
        *Value = (FLOAT)(SCHAR)field;
        return TRUE;
    case PhpHwPmtRatio100:
        if (!field)
            return FALSE;
        *Value = (FLOAT)field * 100.0f;
        return TRUE;
    case PhpHwPmtRatio33:
        if (!field)
            return FALSE;
        *Value = (FLOAT)field * 100.0f / 3.0f;
        return TRUE;
    }

    return FALSE;
}

static PH_HW_NAMED_SENSOR_KIND PhpHwPmtSensorKind(
    _In_ PCPHP_HW_PMT_SAMPLE Sample
    )
{
    return Sample->Transform == PhpHwPmtTemperature ? PhHwNamedSensorPmtTemperature : PhHwNamedSensorUncoreClock;
}

/**
 * Finds the client PMT devices and lists the samples of the aggregators the table knows, matched by GUID.
 */
static VOID PhpHwProbePmt(
    _In_ const KPH_HW_CAPABILITIES* Capabilities
    )
{
    static CONST PH_STRINGREF anyClass = PH_STRINGREF_INIT(L"PCI\\CC_");
    static CONST USHORT pmtIds[] = { 0x7D0D, 0xAD0D, 0x647D, 0xB07D, 0xFD7D, 0xD70D }; // MTL-M, MTL-S/ARL-S, LNL, PTL, WCL, NVL
    PKPH_HW_VSEC_REGION regions;

    PhpHwPmtEntryCount = 0;

    if (!PhpHwTpmiSupported(Capabilities))
        return;

    if (!PhpHwPmtEnumerated)
    {
        PhpHwPmtEnumerated = TRUE;
        PhpHwPmtLocatorCount = PhpHwFindPciDevices(
            PHP_HW_INTEL_VENDOR_ID,
            PHP_HW_INTEL_VENDOR_ID,
            &anyClass,
            pmtIds,
            RTL_NUMBER_OF(pmtIds),
            FALSE,
            PhpHwPmtLocators,
            RTL_NUMBER_OF(PhpHwPmtLocators)
            );

        PhTraceInfo("Hardware sensors: %lu PMT devices", PhpHwPmtLocatorCount);
    }

    if (PhpHwPmtLocatorCount == 0)
        return;

    regions = PhAllocate(sizeof(KPH_HW_VSEC_REGION) * KPH_DYN_HW_VSEC_MAX_TABLE_REGIONS);

    for (ULONG i = 0; i < PhpHwPmtLocatorCount; i++)
    {
        ULONG regionCount = 0;
        NTSTATUS status;

        status = KphHwQueryVsecRegions(&PhpHwPmtLocators[i], regions, KPH_DYN_HW_VSEC_MAX_TABLE_REGIONS, &regionCount);

        if (!NT_SUCCESS(status))
        {
            PhTraceInfo("Hardware sensors: PMT device %lu unavailable: %!STATUS!", i, status);
            continue;
        }

        regionCount = min(regionCount, KPH_DYN_HW_VSEC_MAX_TABLE_REGIONS);

        for (ULONG j = 0; j < regionCount; j++)
        {
            KPH_HW_WINDOW_BATCH_ENTRY entries[PHP_HW_MAX_PMT_ENTRIES];
            ULONG samples[PHP_HW_MAX_PMT_ENTRIES];
            ULONG count = 0;

            if (regions[j].Discovery != KPH_DYN_HW_VSEC_DISCOVERY_PMT_TELEMETRY)
                continue;

            for (ULONG k = 0; k < RTL_NUMBER_OF(PhpHwPmtSamples) && count < RTL_NUMBER_OF(entries); k++)
            {
                if (PhpHwPmtSamples[k].Guid != regions[j].Id || (ULONG)PhpHwPmtSamples[k].Qword * 8 + 8 > regions[j].Size)
                    continue;

                PhpHwInitializeTpmiEntry(&entries[count], PhpHwPmtSamples[k].Qword * 8);
                samples[count++] = k;
            }

            if (!count)
                continue;

            PhTraceInfo("Hardware sensors: PMT device %lu region GUID 0x%08lx, %lu samples", i, regions[j].Id, count);
            PhpHwReadTpmi(&PhpHwPmtLocators[i], regions[j].OpIndex, regions[j].Instance, entries, count);

            // A sample that does not read, or reads blocked, at probe is not listed.
            for (ULONG k = 0; k < count && PhpHwPmtEntryCount < RTL_NUMBER_OF(PhpHwPmtEntries); k++)
            {
                PCPHP_HW_PMT_SAMPLE sample = &PhpHwPmtSamples[samples[k]];
                PPHP_HW_PMT_ENTRY entry;
                PPH_STRING name;

                // A field that reads 0 at probe may be idle; it is listed and shown once it reads.
                if (!NT_SUCCESS(entries[k].Status) || entries[k].Value == MAXULONG64)
                    continue;

                entry = &PhpHwPmtEntries[PhpHwPmtEntryCount++];
                entry->LocatorIndex = i;
                entry->OpIndex = regions[j].OpIndex;
                entry->Instance = regions[j].Instance;
                entry->Sample = samples[k];
                memset(&entry->Item, 0, sizeof(PHP_HW_ITEM));

                name = PhCreateString(sample->Name);
                entry->NamedIndex = PhpHwAddNamedSensor(
                    PhpHwPmtSensorKind(sample),
                    0,
                    PHP_HW_PMT_ID_BASE | (i << 16) | ((ULONG)(regions[j].Instance & 0xFF) << 8) | samples[k],
                    name
                    );
                PhDereferenceObject(name);
            }
        }
    }

    PhFree(regions);

    PhTraceInfo("Hardware sensors: %lu PMT samples", PhpHwPmtEntryCount);
}

/**
 * Reads the listed PMT samples, one batch per region; a sample that fails PHP_HW_RETRY_LIMIT reads in a row ends.
 */
static VOID PhpHwUpdatePmt(
    VOID
    )
{
    for (ULONG first = 0; first < PhpHwPmtEntryCount; )
    {
        PPHP_HW_PMT_ENTRY head = &PhpHwPmtEntries[first];
        KPH_HW_WINDOW_BATCH_ENTRY entries[PHP_HW_MAX_PMT_ENTRIES];
        ULONG indices[PHP_HW_MAX_PMT_ENTRIES];
        ULONG count = 0;
        ULONG next = first;
        NTSTATUS status;

        // Entries of a region are consecutive.
        while (next < PhpHwPmtEntryCount &&
            PhpHwPmtEntries[next].LocatorIndex == head->LocatorIndex &&
            PhpHwPmtEntries[next].OpIndex == head->OpIndex &&
            PhpHwPmtEntries[next].Instance == head->Instance)
        {
            if (!PhpHwPmtEntries[next].Item.Disabled)
            {
                PhpHwInitializeTpmiEntry(&entries[count], PhpHwPmtSamples[PhpHwPmtEntries[next].Sample].Qword * 8);
                indices[count++] = next;
            }

            next++;
        }

        status = count ? KphHwReadVsecBatch(&PhpHwPmtLocators[head->LocatorIndex], head->OpIndex, head->Instance, entries, count) : STATUS_SUCCESS;

        // Entry statuses apply to the samples; a batch that did not run applies its status to each, as TPMI does.
        for (ULONG i = 0; i < count; i++)
        {
            PPHP_HW_PMT_ENTRY entry = &PhpHwPmtEntries[indices[i]];
            NTSTATUS entryStatus = entries[i].Status == PHP_HW_ENTRY_UNSET && !NT_SUCCESS(status) ? status : entries[i].Status;
            FLOAT value = 0.0f;
            BOOLEAN valid;

            valid = PhpHwApplyWindowStatus(&entry->Item, PhpHwPmtSamples[entry->Sample].Qword * 8, entryStatus) &&
                PhpHwDecodePmtSample(&PhpHwPmtSamples[entry->Sample], entries[i].Value, &value);

            PhpHwSetNamedSensor(entry->NamedIndex, valid, value);
        }

        first = next;
    }
}

/**
 * Reads each TPMI region in one batch and decodes RAPL power and uncore clocks. A package sum needs
 * a delta from every one of its domains.
 */
static VOID PhpHwUpdateTpmi(
    VOID
    )
{
    static CONST ULONG capabilities[PhpHwTpmiEnergyMaximum] = { PH_HW_CAP_RAPL_PACKAGE, PH_HW_CAP_RAPL_DRAM, PH_HW_CAP_RAPL_PSYS };
    KPH_HW_WINDOW_BATCH_ENTRY entries[PHP_HW_TPMI_REGION_ENTRIES];
    ULONG indices[PHP_HW_TPMI_REGION_ENTRIES];

    for (ULONG i = 0; i < PhpHwPackageCount; i++)
    {
        memset(PhpHwPackages[i].TickTpmiEnergyRequired, 0, sizeof(PhpHwPackages[i].TickTpmiEnergyRequired));
        memset(PhpHwPackages[i].TickTpmiEnergyCount, 0, sizeof(PhpHwPackages[i].TickTpmiEnergyCount));
        memset(PhpHwPackages[i].TickTpmiPower, 0, sizeof(PhpHwPackages[i].TickTpmiPower));
    }

    for (ULONG r = 0; r < PhpHwTpmiRegionCount; r++)
    {
        PPHP_HW_TPMI_REGION region = &PhpHwTpmiRegions[r];
        BOOLEAN answered = FALSE;
        FLOAT seconds = 0.0f;
        ULONG count = 0;
        NTSTATUS status;

        region->LastSampleTime = region->SampleTime;

        for (ULONG i = region->FirstEntry; i < region->FirstEntry + region->EntryCount; i++)
        {
            PPHP_HW_TPMI_ENTRY entry = &PhpHwTpmiEntries[i];

            entry->LastValid = entry->Valid;
            entry->LastValue = entry->Value;
            entry->Valid = FALSE;

            if (region->Item.Disabled || entry->Item.Disabled)
            {
                PhpHwSetNamedSensor(entry->NamedIndex, FALSE, 0.0f);
                continue;
            }

            // A package value needs every domain still read; disabled domains leave the requirement.
            if (entry->Kind < PhpHwTpmiEnergyMaximum && entry->NamedIndex == ULONG_MAX && entry->PackageIndex < PhpHwPackageCount)
                PhpHwPackages[entry->PackageIndex].TickTpmiEnergyRequired[entry->Kind]++;

            PhpHwInitializeTpmiEntry(&entries[count], entry->Offset);
            indices[count++] = i;
        }

        if (count == 0)
        {
            region->Item.Disabled = TRUE;
            continue;
        }

        status = KphHwReadVsecBatch(&region->Locator, region->OpIndex, region->Instance, entries, count);
        PhQueryPerformanceCounter(&region->SampleTime);

        if (
            PhpHwFrequency.QuadPart &&
            region->LastSampleTime.QuadPart &&
            region->SampleTime.QuadPart > region->LastSampleTime.QuadPart
            )
        {
            seconds = (FLOAT)(region->SampleTime.QuadPart - region->LastSampleTime.QuadPart) / (FLOAT)PhpHwFrequency.QuadPart;
        }

        for (ULONG i = 0; i < count; i++)
        {
            PPHP_HW_TPMI_ENTRY entry = &PhpHwTpmiEntries[indices[i]];
            BOOLEAN valid = FALSE;
            FLOAT value = 0.0f;

            // A whole-batch failure leaves every entry unset.
            if (entries[i].Status != PHP_HW_ENTRY_UNSET)
            {
                answered = TRUE;

                if (PhpHwApplyWindowStatus(&entry->Item, entries[i].Offset, entries[i].Status) && entries[i].Value != MAXULONG64)
                {
                    entry->Value = entries[i].Value;
                    entry->Valid = TRUE;
                }
            }

            if (entry->Kind == PhpHwTpmiUncoreStatus)
            {
                if (entry->Valid)
                {
                    value = (FLOAT)(entry->Value & 0x7F) * 100.0f;
                    valid = TRUE;
                }
            }
            else if (entry->Valid && entry->LastValid && seconds > 0.0f)
            {
                // ENERGY_STATUS [31:0] wraps.
                value = (FLOAT)((ULONG)entry->Value - (ULONG)entry->LastValue) * entry->EnergyUnit / seconds;

                if (entry->NamedIndex != ULONG_MAX)
                {
                    valid = TRUE;
                }
                else if (entry->PackageIndex < PhpHwPackageCount)
                {
                    PhpHwPackages[entry->PackageIndex].TickTpmiPower[entry->Kind] += value;
                    PhpHwPackages[entry->PackageIndex].TickTpmiEnergyCount[entry->Kind]++;
                }
            }

            PhpHwSetNamedSensor(entry->NamedIndex, valid, value);
        }

        if (answered)
            region->Item.RetryCount = 0;
        else
            PhpHwApplyWindowStatus(&region->Item, region->Instance, status);
    }

    for (ULONG i = 0; i < PhpHwPackageCount; i++)
    {
        PPHP_HW_PACKAGE package = &PhpHwPackages[i];
        PPH_HW_PACKAGE_SENSOR sensor = &PhpHwPackageSensors[i];

        for (ULONG kind = 0; kind < PhpHwTpmiEnergyMaximum; kind++)
        {
            FLOAT watts = package->TickTpmiPower[kind];

            if (!package->TickTpmiEnergyRequired[kind] || package->TickTpmiEnergyCount[kind] != package->TickTpmiEnergyRequired[kind])
                continue;

            switch (kind)
            {
            case PhpHwTpmiPackageEnergy:
                sensor->PackagePower = watts;
                break;
            case PhpHwTpmiDramEnergy:
                sensor->DramPower = watts;
                break;
            case PhpHwTpmiPlatformEnergy:
                sensor->PlatformPower = watts;
                break;
            }

            package->TickValid |= capabilities[kind];
        }
    }
}

static VOID PhpHwRefreshCapabilities(
    VOID
    )
{
    ULONG capabilities = 0;

    if (PhpHwArm64)
    {
        for (ULONG i = 0; i < PhpHwProcessorCount; i++)
        {
            switch (PhpHwArmSource(i))
            {
            case PhpHwArmSourceAmu:
                capabilities |= PH_HW_CAP_EFFECTIVE_CLOCK | PH_HW_CAP_ACTIVE_CLOCK;
                break;
            case PhpHwArmSourcePmu:
                capabilities |= PH_HW_CAP_EFFECTIVE_CLOCK;
                break;
            }
        }
    }
    else if (PhpHwRegisterActive(PhpHwRegAperf))
    {
        capabilities |= PH_HW_CAP_EFFECTIVE_CLOCK;

        if (PhpHwRegisterActive(PhpHwRegMperf))
        {
            capabilities |= PH_HW_CAP_ACTIVE_CLOCK;

            // The AMD multiplier is derived from the active clock.
            if (!PhpHwIntel)
                capabilities |= PH_HW_CAP_MULTIPLIER;
        }
    }

    if (PhpHwRegisterActive(PhpHwRegPerfStatus))
    {
        capabilities |= PH_HW_CAP_MULTIPLIER;

        if (PhpHwVoltageSupported)
            capabilities |= PH_HW_CAP_VOLTAGE;
    }

    if (PhpHwRegisterActive(PhpHwRegThermStatus))
        capabilities |= PH_HW_CAP_CORE_TEMP | PH_HW_CAP_PACKAGE_TEMP | PH_HW_CAP_THROTTLE;
    if (PhpHwRegisterActive(PhpHwRegPackageThermStatus))
        capabilities |= PH_HW_CAP_PACKAGE_TEMP | PH_HW_CAP_THROTTLE;
    if (PhpHwRegisterActive(PhpHwRegLimitReasons))
        capabilities |= PH_HW_CAP_THROTTLE;

    if (PhpHwRegisterActive(PhpHwRegPackageEnergy))
        capabilities |= PH_HW_CAP_RAPL_PACKAGE;
    if (PhpHwRegisterActive(PhpHwRegPp0Energy) || PhpHwRegisterActive(PhpHwRegCoreEnergy))
        capabilities |= PH_HW_CAP_RAPL_CORE;
    if (PhpHwRegisterActive(PhpHwRegDramEnergy))
        capabilities |= PH_HW_CAP_RAPL_DRAM;
    if (PhpHwRegisterActive(PhpHwRegPlatformEnergy))
        capabilities |= PH_HW_CAP_RAPL_PSYS;
    if (PhpHwLpitNamedIndex != ULONG_MAX)
        capabilities |= PH_HW_CAP_RESIDENCY;
    if (PhpHwRaplLimitedNamedIndex != ULONG_MAX && PhpHwWindowRegisterActive(PhpHwWinRaplPerfStatus))
        capabilities |= PH_HW_CAP_RESIDENCY;
    if (PhpHwPwrmWindow)
        capabilities |= PH_HW_CAP_CHIPSET_TEMP;
    if (PhpHwGtNamedIndex != ULONG_MAX && PhpHwWindowRegisterActive(PhpHwWinGtPerfStatus))
        capabilities |= PH_HW_CAP_UNCORE_CLOCK;

    for (ULONG i = 0; i < PhpHwPmtEntryCount; i++)
    {
        if (!PhpHwPmtEntries[i].Item.Disabled && PhpHwPmtEntries[i].NamedIndex != ULONG_MAX)
            capabilities |= PhpHwPmtSamples[PhpHwPmtEntries[i].Sample].Transform == PhpHwPmtTemperature ? PH_HW_CAP_TPMI_POWER : PH_HW_CAP_UNCORE_CLOCK;
    }

    capabilities |= PhpHwCrosEcCapabilities();

    for (ULONG i = 0; i < PhpHwPackageCount; i++)
    {
        for (ULONG j = 0; j < PHP_HW_LIMIT_SENSORS; j++)
        {
            if (PhpHwPackages[i].LimitNamedIndex[j] != ULONG_MAX && PhpHwRegisterActive(j == PHP_HW_LIMIT_PL4 ? PhpHwRegVrCurrentConfig : PhpHwRegPlatformPowerLimit))
                capabilities |= PH_HW_CAP_PLATFORM_LIMITS;
        }
    }

    for (ULONG i = 0; i < PhpHwPackageCount; i++)
    {
        PPHP_HW_PACKAGE package = &PhpHwPackages[i];

        if (package->UncoreNamedIndex != ULONG_MAX && PhpHwRegisterActive(PhpHwRegUncorePerfStatus))
            capabilities |= PH_HW_CAP_UNCORE_CLOCK;

        if (!package->HasRoot)
            continue;

        if (!package->Tctl.Disabled)
            capabilities |= PH_HW_CAP_PACKAGE_TEMP;

        for (ULONG j = 0; j < PhpHwCcdCount; j++)
        {
            if ((package->CcdMask & (1UL << j)) && !package->Ccd[j].Disabled)
                capabilities |= PH_HW_CAP_CCD_TEMP;
        }

        if (!package->MemoryClock.Disabled)
            capabilities |= PH_HW_CAP_MEMORY_CLOCK;
        if (package->MemoryTimingsValid)
            capabilities |= PH_HW_CAP_MEMORY_TIMINGS;
    }

    if (PhpHwPackageCount && PhpHwPackages[0].MsrPowerLimit1 > 0.0f)
        capabilities |= PH_HW_CAP_POWER_LIMIT1;
    if (PhpHwPackageCount && PhpHwPackages[0].MsrPowerLimit2 > 0.0f)
        capabilities |= PH_HW_CAP_POWER_LIMIT2;

    if (PhpHwWindowCount)
    {
        if (
            PhpHwWindowRegisterActive(PhpHwWinMcBiosReq) ||
            PhpHwWindowRegisterActive(PhpHwWinMcBiosData) ||
            PhpHwWindowRegisterActive(PhpHwWinSaPerfStatus) ||
            PhpHwWindowRegisterActive(PhpHwWinMemssBiosData)
            )
        {
            capabilities |= PH_HW_CAP_MEMORY_CLOCK;
        }

        for (PHP_HW_WINDOW_REG reg = 0; reg < PhpHwWinMaximum; reg++)
        {
            if (PhpHwWindowRegInfo[reg].Imc && PhpHwWindowRegisterActive(reg))
                capabilities |= PH_HW_CAP_MEMORY_BANDWIDTH;
        }

        if (PhpHwWindowRegisterActive(PhpHwWinPackageTemperature))
            capabilities |= PH_HW_CAP_PACKAGE_TEMP;
        if (PhpHwWindowRegisterActive(PhpHwWinPp0Temperature))
            capabilities |= PH_HW_CAP_CORES_TEMP;
        if (PhpHwWindowRegisterActive(PhpHwWinPp1Temperature))
            capabilities |= PH_HW_CAP_GRAPHICS_TEMP;
        if (PhpHwWindowRegisterActive(PhpHwWinPowerLimit1))
            capabilities |= PH_HW_CAP_POWER_LIMIT1;
        if (PhpHwWindowRegisterActive(PhpHwWinPowerLimit2))
            capabilities |= PH_HW_CAP_POWER_LIMIT2;
        if (PhpHwWindowRegisterActive(PhpHwWinPchTemp))
            capabilities |= PH_HW_CAP_CHIPSET_TEMP;
        if (PhpHwPackageCount && PhpHwPackages[0].MemoryTimingsValid)
            capabilities |= PH_HW_CAP_MEMORY_TIMINGS;
    }

    if (PhpHwDimmsActive())
        capabilities |= PH_HW_CAP_DIMM_TEMP;
    if (PhpHwDimmVoltagesActive())
        capabilities |= PH_HW_CAP_DIMM_VOLTAGE;
    if (PhpHwDimmPmicsActive())
        capabilities |= PH_HW_CAP_DIMM_PMIC;

    capabilities |= PhpHwSmuCapabilities();
    capabilities |= PhpHwFanCapabilities();

    PhAcquireQueuedLockExclusive(&PhpHwNamedSensorLock);

    for (ULONG i = 0; i < PhpHwTpmiRegionCount; i++)
    {
        PPHP_HW_TPMI_REGION region = &PhpHwTpmiRegions[i];

        for (ULONG j = region->FirstEntry; j < region->FirstEntry + region->EntryCount; j++)
        {
            PPHP_HW_TPMI_ENTRY entry = &PhpHwTpmiEntries[j];
            BOOLEAN active = !region->Item.Disabled && !entry->Item.Disabled;

            if (entry->NamedIndex < PhpHwNamedSensorCount)
                PhpHwNamedSensors[entry->NamedIndex].Active = active;

            if (!active)
                continue;

            if (entry->Kind == PhpHwTpmiUncoreStatus)
                capabilities |= PH_HW_CAP_UNCORE_CLOCK;
            else if (entry->NamedIndex != ULONG_MAX)
                capabilities |= PH_HW_CAP_TPMI_POWER;
            else if (entry->Kind == PhpHwTpmiPackageEnergy)
                capabilities |= PH_HW_CAP_RAPL_PACKAGE;
            else if (entry->Kind == PhpHwTpmiDramEnergy)
                capabilities |= PH_HW_CAP_RAPL_DRAM;
            else
                capabilities |= PH_HW_CAP_RAPL_PSYS;
        }
    }

    for (ULONG i = 0; i < PhpHwPackageCount; i++)
    {
        ULONG index = PhpHwPackages[i].UncoreNamedIndex;

        if (index < PhpHwNamedSensorCount)
            PhpHwNamedSensors[index].Active = PhpHwRegisterActive(PhpHwRegUncorePerfStatus);

        for (ULONG j = 0; j < PHP_HW_LIMIT_SENSORS; j++)
        {
            index = PhpHwPackages[i].LimitNamedIndex[j];

            if (index < PhpHwNamedSensorCount)
                PhpHwNamedSensors[index].Active = PhpHwRegisterActive(j == PHP_HW_LIMIT_PL4 ? PhpHwRegVrCurrentConfig : PhpHwRegPlatformPowerLimit);
        }
    }

    if (PhpHwGtNamedIndex < PhpHwNamedSensorCount)
        PhpHwNamedSensors[PhpHwGtNamedIndex].Active = PhpHwWindowRegisterActive(PhpHwWinGtPerfStatus);
    if (PhpHwRaplLimitedNamedIndex < PhpHwNamedSensorCount)
        PhpHwNamedSensors[PhpHwRaplLimitedNamedIndex].Active = PhpHwWindowRegisterActive(PhpHwWinRaplPerfStatus);

    for (ULONG i = 0; i < PhpHwPmtEntryCount; i++)
    {
        if (PhpHwPmtEntries[i].NamedIndex < PhpHwNamedSensorCount)
            PhpHwNamedSensors[PhpHwPmtEntries[i].NamedIndex].Active = !PhpHwPmtEntries[i].Item.Disabled;
    }

    PhReleaseQueuedLockExclusive(&PhpHwNamedSensorLock);

    PhpHwCapabilities = capabilities;
}

/**
 * Decodes MSR_PKG_POWER_LIMIT: PL1 [14:0] enabled by bit 15, PL2 [46:32] enabled by bit 47.
 */
static VOID PhpHwDecodeMsrPowerLimit(
    _Inout_ PPHP_HW_PACKAGE Package,
    _In_ ULONG64 PowerLimit
    )
{
    Package->MsrPowerLimit1 = (PowerLimit & 0x8000) ? (FLOAT)(PowerLimit & 0x7FFF) * PhpHwPowerUnit : 0.0f;
    Package->MsrPowerLimit2 = (PowerLimit & (1ULL << 47)) ? (FLOAT)((PowerLimit >> 32) & 0x7FFF) * PhpHwPowerUnit : 0.0f;
}

/**
 * Reads the decode constants from the probe sample and drops registers that cannot be decoded.
 */
static VOID PhpHwProbeDecode(
    VOID
    )
{
    PPHP_HW_SAMPLE sample0 = &PhpHwPackages[0].Sample;
    FLOAT busMhz = (PhpHwIntel && PhpHwIsIntelNehalem()) ? 133.33f : 100.0f;
    ULONG64 raplUnit = 0;
    BOOLEAN raplUnitValid;
    BOOLEAN tjMax = FALSE;
    ULONG64 value;

    PhpHwTscMhz = 0.0f;
    PhpHwEnergyUnit = 0.0f;
    PhpHwDramEnergyUnit = 0.0f;
    PhpHwPlatformEnergyUnit = 0.0f;
    PhpHwPowerUnit = 0.0f;
    PhpHwVoltageSupported = FALSE;
    PhpHwTctlOffset = 0.0f;

    raplUnitValid = PhpHwSampleValue(sample0, PhpHwRegRaplUnit, &raplUnit);

    if (raplUnitValid)
    {
        PhpHwEnergyUnit = PhRaplEnergyUnitJoules(raplUnit, &PhpHwIdentity);

        if (PhpHwIntel)
        {
            PhpHwDramEnergyUnit = PhIntelRaplDramEnergyUnitJoules(raplUnit, &PhpHwIdentity);
            PhpHwPlatformEnergyUnit = PhIntelRaplPlatformEnergyUnitJoules(raplUnit, &PhpHwIdentity);
            PhpHwPowerUnit = PhRaplPowerUnitWatts(raplUnit, &PhpHwIdentity);
        }
    }

    for (ULONG i = 0; i < PhpHwPackageCount; i++)
    {
        PPHP_HW_PACKAGE package = &PhpHwPackages[i];
        PPH_HW_PACKAGE_SENSOR sensor = &PhpHwPackageSensors[i];
        FLOAT baseMhz = 0.0f;
        FLOAT powerLimit = 0.0f;

        package->TjMax = 0;
        package->MsrPowerLimit1 = 0.0f;
        package->MsrPowerLimit2 = 0.0f;

        if (PhpHwIntel)
        {
            if (PhpHwSampleValue(&package->Sample, PhpHwRegTemperatureTarget, &value))
                package->TjMax = PhIntelTemperatureTargetTjMax(value);
            if (!package->TjMax && (PhpHwRegisterActive(PhpHwRegThermStatus) || PhpHwRegisterActive(PhpHwRegPackageThermStatus)))
                package->TjMax = PhIntelFixedTjMax(&PhpHwIdentity);
            if (PhpHwSampleValue(&package->Sample, PhpHwRegPlatformInfo, &value))
                baseMhz = (FLOAT)((value >> 8) & 0xFF) * busMhz;
            if (raplUnitValid && PhpHwSampleValue(&package->Sample, PhpHwRegPowerLimit, &value))
            {
                powerLimit = (FLOAT)(value & 0x7FFF) * PhpHwPowerUnit;
                PhpHwDecodeMsrPowerLimit(package, value);
            }
        }
        else
        {
            // MPERF counts at the P0 frequency.
            if (PhpHwSampleValue(&package->Sample, PhpHwRegPStateDef0, &value))
                baseMhz = PhAmdPStateDefToMhz(value, &PhpHwIdentity);
        }

        if (i == 0)
            PhpHwTscMhz = baseMhz;
        if (package->TjMax)
            tjMax = TRUE;

        sensor->TjMax = (FLOAT)package->TjMax;
        sensor->BaseMhz = baseMhz;
        sensor->BusMhz = busMhz;
        sensor->PowerLimit = powerLimit;
    }

    if (PhpHwRegisterActive(PhpHwRegPerfStatus))
    {
        ULONG firstProcessor = PhpHwPackages[0].FirstProcessor;
        PPHP_HW_SAMPLE sample;

        if (PhpHwRegisters[PhpHwRegPerfStatus].Scope == PhpHwScopeThread)
            sample = &PhpHwProcessorSamples[firstProcessor];
        else
            sample = &PhpHwCores[PhpHwCpuSensors[firstProcessor].CoreIndex].Sample;

        if (PhpHwSampleValue(sample, PhpHwRegPerfStatus, &value))
        {
            if (PhpHwIntel)
            {
                PhpHwVoltageSupported = PhIntelPerfStatusToVolts(value, &PhpHwIdentity) > 0.0f;
            }
            else
            {
                UCHAR vid;

                PhpHwVoltageSupported = PhAmdPStateVid(value, &PhpHwIdentity, &vid) && PhAmdCpuVidToVolts(vid, &PhpHwIdentity) > 0.0f;
            }
        }
    }

    if (!PhpHwIntel)
    {
        SYSTEM_PROCESSOR_BRAND_STRING brandString;

        if (NT_SUCCESS(NtQuerySystemInformation(SystemProcessorBrandString, &brandString, sizeof(brandString), NULL)))
        {
            PPH_STRING brand;

            brandString.BrandString[RTL_NUMBER_OF(brandString.BrandString) - 1] = ANSI_NULL;
            brand = PhZeroExtendToUtf16Ex(brandString.BrandString, PhCountBytesZ(brandString.BrandString));
            PhpHwTctlOffset = PhAmdTctlOffset(&PhpHwIdentity, &brand->sr);
            PhDereferenceObject(brand);
        }
    }

    // MPERF is only needed for the active clock; APERF alone yields the effective clock.
    if (PhpHwTscMhz <= 0.0f)
        PhpHwDisableRegister(PhpHwRegMperf);

    if (!tjMax)
    {
        PhpHwDisableRegister(PhpHwRegThermStatus);
        PhpHwDisableRegister(PhpHwRegPackageThermStatus);
    }

    if (PhpHwEnergyUnit <= 0.0f)
    {
        PhpHwDisableRegister(PhpHwRegPackageEnergy);
        PhpHwDisableRegister(PhpHwRegPp0Energy);
        PhpHwDisableRegister(PhpHwRegCoreEnergy);
    }

    if (PhpHwDramEnergyUnit <= 0.0f)
        PhpHwDisableRegister(PhpHwRegDramEnergy);
    if (PhpHwPlatformEnergyUnit <= 0.0f)
        PhpHwDisableRegister(PhpHwRegPlatformEnergy);
}

/**
 * Adds the MSR uncore clock sensor of each package that TPMI does not already report.
 */
static VOID PhpHwProbeMsrUncore(
    VOID
    )
{
    static CONST PH_STRINGREF name = PH_STRINGREF_INIT(L"Uncore");
    BOOLEAN active = PhpHwRegisterActive(PhpHwRegUncorePerfStatus);

    for (ULONG i = 0; i < PhpHwPackageCount; i++)
    {
        PPHP_HW_PACKAGE package = &PhpHwPackages[i];
        BOOLEAN tpmi = FALSE;
        PPH_STRING string;

        package->UncoreNamedIndex = ULONG_MAX;

        if (!active)
            continue;

        // An unmapped TPMI domain is published on package 0.
        for (ULONG r = 0; r < PhpHwTpmiRegionCount && !tpmi; r++)
        {
            PPHP_HW_TPMI_REGION region = &PhpHwTpmiRegions[r];

            if (region->Item.Disabled)
                continue;

            for (ULONG j = region->FirstEntry; j < region->FirstEntry + region->EntryCount; j++)
            {
                PPHP_HW_TPMI_ENTRY entry = &PhpHwTpmiEntries[j];

                if (entry->Kind == PhpHwTpmiUncoreStatus && !entry->Item.Disabled &&
                    (entry->PackageIndex == ULONG_MAX ? 0 : entry->PackageIndex) == i)
                {
                    tpmi = TRUE;
                }
            }
        }

        if (tpmi)
            continue;

        string = PhCreateString2(&name);
        package->UncoreNamedIndex = PhpHwAddNamedSensor(PhHwNamedSensorUncoreClock, i, PHP_HW_MSR_UNCORE_ID, string);
        PhDereferenceObject(string);
    }
}

/**
 * Reads the counter frequency of the first usable LPIT residency counter, by the kernel's ordinal rule: native
 * entries that are enabled and have a SYSTEM_MEMORY counter at bit offset 0 of 32 or 64 bits.
 *
 * \param Frequency Receives the counter frequency in Hz; 0 is the TSC rate.
 */
_Success_(return)
static BOOLEAN PhpHwGetLpitFrequency(
    _Out_ PULONG64 Frequency
    )
{
    NTSTATUS status;
    PSYSTEM_FIRMWARE_TABLE_INFORMATION info;
    ULONG length = sizeof(SYSTEM_FIRMWARE_TABLE_INFORMATION) + 0x1000;
    BOOLEAN found = FALSE;

    info = PhAllocateZero(length);
    info->ProviderSignature = 'ACPI';
    info->TableID = 'TIPL';
    info->Action = SystemFirmwareTableGet;
    info->TableBufferLength = length - sizeof(SYSTEM_FIRMWARE_TABLE_INFORMATION);

    status = NtQuerySystemInformation(SystemFirmwareTableInformation, info, length, &length);

    if (status == STATUS_BUFFER_TOO_SMALL)
    {
        PhFree(info);
        info = PhAllocateZero(length);
        info->ProviderSignature = 'ACPI';
        info->TableID = 'TIPL';
        info->Action = SystemFirmwareTableGet;
        info->TableBufferLength = length - sizeof(SYSTEM_FIRMWARE_TABLE_INFORMATION);

        status = NtQuerySystemInformation(SystemFirmwareTableInformation, info, length, &length);
    }

    if (NT_SUCCESS(status) && info->TableBufferLength >= PHP_HW_LPIT_HEADER)
    {
        const UCHAR* table = info->TableBuffer;
        ULONG tableLength;
        ULONG offset = PHP_HW_LPIT_HEADER;

        memcpy(&tableLength, table + 4, sizeof(ULONG));
        tableLength = min(tableLength, info->TableBufferLength);

        while (offset + 8 <= tableLength)
        {
            ULONG type;
            ULONG entryLength;
            ULONG flags;

            memcpy(&type, table + offset, sizeof(ULONG));
            memcpy(&entryLength, table + offset + 4, sizeof(ULONG));

            if (entryLength < 8 || entryLength > tableLength - offset)
                break;

            if (type == 0 && entryLength >= PHP_HW_LPIT_NATIVE_LENGTH)
            {
                memcpy(&flags, table + offset + 12, sizeof(ULONG));

                // Not disabled (bit 0), has a counter (bit 1); the counter GAS at +36: space, bit width, bit offset.
                if (
                    !(flags & 0x3) &&
                    table[offset + 36] == 0 &&
                    (table[offset + 37] == 32 || table[offset + 37] == 64) &&
                    table[offset + 38] == 0
                    )
                {
                    memcpy(Frequency, table + offset + 48, sizeof(ULONG64));
                    found = TRUE;
                    break;
                }
            }

            offset += entryLength;
        }
    }

    PhFree(info);

    return found;
}

/**
 * \return TRUE when this CPU's row can hold an LPIT window: Intel x64 with the window kind.
 */
static BOOLEAN PhpHwLpitSupported(
    _In_ const KPH_HW_CAPABILITIES* Capabilities
    )
{
    return PhpHwIntel &&
        !PhpHwArm64 &&
        FlagOn(Capabilities->Flags, KPH_HW_CAP_CPU_HAS_ROW) &&
        FlagOn(Capabilities->ImplementedKinds, 1ULL << KPH_DYN_HW_KIND_WINDOW_READ);
}

/**
 * Reads the SLP_S0 residency counter of the LPIT window of this CPU's row.
 */
_Success_(NT_SUCCESS(return))
static NTSTATUS PhpHwReadLpitCounter(
    _Out_ PULONG Count
    )
{
    KPH_HW_WINDOW_BATCH_ENTRY entry;
    NTSTATUS status;

    memset(&entry, 0, sizeof(KPH_HW_WINDOW_BATCH_ENTRY));
    entry.Width = 4;
    entry.Status = PHP_HW_ENTRY_UNSET;

    status = KphHwReadPlatformWindowBatch(KPH_HW_PLATFORM_CPU, PHP_HW_LPIT_WINDOW, &entry, 1);

    if (NT_SUCCESS(status))
        status = entry.Status;
    if (NT_SUCCESS(status))
        *Count = (ULONG)entry.Value;

    return status;
}

/**
 * Decodes THERMAL_TSS0 of the PMC: valid when TSRV (bit 9), TSR [8:0] two's complement whole degrees.
 */
_Success_(return)
static BOOLEAN PhpHwDecodePwrmTemperature(
    _In_ ULONG Value,
    _Out_ PFLOAT Celsius
    )
{
    LONG celsius;

    if (Value == ULONG_MAX || !(Value & 0x200))
        return FALSE;

    celsius = (LONG)(Value & 0x1FF);

    if (celsius & 0x100)
        celsius -= 0x200;
    if (celsius < -40 || celsius > 150)
        return FALSE;

    *Celsius = (FLOAT)celsius;
    return TRUE;
}

/**
 * Reads registers of a PWRM platform window op; the status is the batch's, then the first entry's.
 */
static NTSTATUS PhpHwReadPwrm(
    _In_ UCHAR Window,
    _Inout_updates_(Count) PKPH_HW_WINDOW_BATCH_ENTRY Entries,
    _In_ ULONG Count
    )
{
    NTSTATUS status;

    status = KphHwReadPlatformWindowBatch(KPH_HW_PLATFORM_CPU, Window, Entries, Count);

    if (NT_SUCCESS(status))
        status = Entries[0].Status;

    return status;
}

/**
 * Selects the first PWRM op with a valid THERMAL_TSS0, unless the PCI PCH thermal device is the source.
 */
static VOID PhpHwProbePwrm(
    _In_ const KPH_HW_CAPABILITIES* Capabilities
    )
{
    PhpHwPwrmWindow = 0;
    PhpHwPwrmFailures = 0;
    memset(PhpHwPwrmMinMax, 0, sizeof(PhpHwPwrmMinMax));

    for (ULONG i = 0; i < PHP_HW_PWRM_WINDOW_COUNT; i++)
        PhpHwPwrmStatus[i] = STATUS_NOT_SUPPORTED;

    if (!PhpHwLpitSupported(Capabilities) || PhpHwWindowRegisterActive(PhpHwWinPchTemp))
        return;

    for (UCHAR i = 0; i < PHP_HW_PWRM_WINDOW_COUNT; i++)
    {
        KPH_HW_WINDOW_BATCH_ENTRY entries[3];
        FLOAT celsius;

        for (ULONG j = 0; j < RTL_NUMBER_OF(entries); j++)
        {
            memset(&entries[j], 0, sizeof(KPH_HW_WINDOW_BATCH_ENTRY));
            entries[j].Width = 4;
            entries[j].Status = PHP_HW_ENTRY_UNSET;
        }

        entries[0].Offset = PH_INTEL_PWRM_THERMAL_TSS0;
        entries[1].Offset = PH_INTEL_PWRM_THERMAL_MINTEMP;
        entries[2].Offset = PH_INTEL_PWRM_THERMAL_MAXTEMP;

        PhpHwPwrmStatus[i] = PhpHwReadPwrm(PHP_HW_PWRM_WINDOW_FIRST + i, entries, RTL_NUMBER_OF(entries));

        if (NT_SUCCESS(PhpHwPwrmStatus[i]) && PhpHwDecodePwrmTemperature((ULONG)entries[0].Value, &celsius))
        {
            PhpHwPwrmWindow = PHP_HW_PWRM_WINDOW_FIRST + i;
            PhpHwPwrmMinMax[0] = NT_SUCCESS(entries[1].Status) ? (ULONG)entries[1].Value : ULONG_MAX;
            PhpHwPwrmMinMax[1] = NT_SUCCESS(entries[2].Status) ? (ULONG)entries[2].Value : ULONG_MAX;
            break;
        }
    }
}

/**
 * Reads the PCH temperature from THERMAL_TSS0; repeated failed reads drop it.
 */
static VOID PhpHwUpdatePwrm(
    VOID
    )
{
    KPH_HW_WINDOW_BATCH_ENTRY entry;
    NTSTATUS status;
    FLOAT celsius;

    memset(&entry, 0, sizeof(KPH_HW_WINDOW_BATCH_ENTRY));
    entry.Offset = PH_INTEL_PWRM_THERMAL_TSS0;
    entry.Width = 4;
    entry.Status = PHP_HW_ENTRY_UNSET;

    status = PhpHwReadPwrm(PhpHwPwrmWindow, &entry, 1);

    PhpHwChipsetTemperatureValid = FALSE;

    if (NT_SUCCESS(status))
    {
        PhpHwPwrmFailures = 0;

        // TSRV clear is a reading in progress, not a failure.
        if (PhpHwDecodePwrmTemperature((ULONG)entry.Value, &celsius))
        {
            PhpHwChipsetTemperature = celsius;
            PhpHwChipsetTemperatureValid = TRUE;
        }
    }
    else if (++PhpHwPwrmFailures >= PHP_HW_RETRY_LIMIT)
    {
        PhTraceInfo("Hardware sensors: PWRM op %u unavailable: %!STATUS!", PhpHwPwrmWindow, status);
        PhpHwPwrmWindow = 0;
    }
}

/**
 * Lists the S0ix (SLP_S0) residency of the Intel LPIT when the counter reads.
 */
static VOID PhpHwProbeLpit(
    _In_ const KPH_HW_CAPABILITIES* Capabilities
    )
{
    ULONG64 frequency;
    ULONG count;
    PPH_STRING string;

    PhpHwLpitNamedIndex = ULONG_MAX;
    PhpHwLpitSampled = FALSE;
    PhpHwLpitStatus = STATUS_NOT_SUPPORTED;
    PhpHwLpitFrequency = 0.0;

    if (!PhpHwLpitSupported(Capabilities))
        return;

    if (!PhpHwGetLpitFrequency(&frequency))
    {
        PhpHwLpitStatus = STATUS_NOT_FOUND;
        return;
    }

    PhpHwLpitFrequency = frequency ? (DOUBLE)frequency : (DOUBLE)PhpHwTscMhz * 1000000.0;
    PhpHwLpitStatus = PhpHwReadLpitCounter(&count);

    if (!NT_SUCCESS(PhpHwLpitStatus) || PhpHwLpitFrequency <= 0.0)
        return;

    PhpHwLpitFailures = 0;
    string = PhCreateString(L"S0ix (SLP_S0)");
    PhpHwLpitNamedIndex = PhpHwAddNamedSensor(PhHwNamedSensorPlatformResidency, 0, 0, string);
    PhDereferenceObject(string);
}

/**
 * Publishes the share of the tick the platform spent in S0ix: the 32-bit counter delta over its frequency.
 */
static VOID PhpHwUpdateLpit(
    VOID
    )
{
    LARGE_INTEGER now;
    ULONG count;
    BOOLEAN valid = FALSE;
    FLOAT percent = 0.0f;

    PhpHwLpitStatus = PhpHwReadLpitCounter(&count);
    PhQueryPerformanceCounter(&now);

    if (NT_SUCCESS(PhpHwLpitStatus))
    {
        PhpHwLpitFailures = 0;

        if (PhpHwLpitSampled && now.QuadPart > PhpHwLpitTime.QuadPart)
        {
            DOUBLE seconds = (DOUBLE)(now.QuadPart - PhpHwLpitTime.QuadPart) / (DOUBLE)PhpHwFrequency.QuadPart;
            DOUBLE residency = (DOUBLE)(ULONG)(count - PhpHwLpitCount) / PhpHwLpitFrequency;

            percent = (FLOAT)(residency / seconds * 100.0);

            // A tick longer than one 32-bit wrap of the counter is ambiguous.
            valid = seconds * PhpHwLpitFrequency < 4294967296.0 && percent >= 0.0f && percent <= 100.5f;
            percent = min(percent, 100.0f);
        }

        PhpHwLpitCount = count;
        PhpHwLpitTime = now;
        PhpHwLpitSampled = TRUE;
    }
    else if (++PhpHwLpitFailures >= PHP_HW_RETRY_LIMIT)
    {
        PhTraceInfo("Hardware sensors: LPIT counter unavailable: %!STATUS!", PhpHwLpitStatus);

        PhAcquireQueuedLockExclusive(&PhpHwNamedSensorLock);

        if (PhpHwLpitNamedIndex < PhpHwNamedSensorCount)
        {
            PhpHwNamedSensors[PhpHwLpitNamedIndex].Valid = FALSE;
            PhpHwNamedSensors[PhpHwLpitNamedIndex].Active = FALSE;
        }

        PhReleaseQueuedLockExclusive(&PhpHwNamedSensorLock);

        PhpHwLpitNamedIndex = ULONG_MAX;
        return;
    }
    else
    {
        PhpHwLpitSampled = FALSE;
    }

    PhpHwSetNamedSensor(PhpHwLpitNamedIndex, valid, percent);
}

/**
 * Lists the MCHBAR graphics clock when the probe found integrated graphics.
 */
static VOID PhpHwProbeGtClock(
    VOID
    )
{
    PPH_STRING name;

    PhpHwGtNamedIndex = ULONG_MAX;

    if (!PhpHwWindowRegisterActive(PhpHwWinGtPerfStatus))
        return;

    name = PhCreateString(L"Graphics");
    PhpHwGtNamedIndex = PhpHwAddNamedSensor(PhHwNamedSensorUncoreClock, 0, PHP_HW_GT_CLOCK_ID, name);
    PhDereferenceObject(name);
}

/**
 * Lists the share of time RAPL limited the package when a window reads PACKAGE_RAPL_PERF_STATUS.
 */
static VOID PhpHwProbeRaplLimited(
    VOID
    )
{
    PPH_STRING name;

    PhpHwRaplLimitedNamedIndex = ULONG_MAX;

    if (!PhpHwWindowRegisterActive(PhpHwWinRaplPerfStatus))
        return;

    name = PhCreateString(L"Power limited (RAPL)");
    PhpHwRaplLimitedNamedIndex = PhpHwAddNamedSensor(PhHwNamedSensorPlatformResidency, 0, PHP_HW_RAPL_LIMITED_ID, name);
    PhDereferenceObject(name);
}

/**
 * Lists PL4 (MSR_VR_CURRENT_CONFIG on PL4 models) and the platform (PSYS) PL1/PL2 of each package that the probe
 * sample shows set or enabled.
 */
static VOID PhpHwProbePlatformLimits(
    VOID
    )
{
    static CONST PCWSTR names[PHP_HW_LIMIT_SENSORS] = { L"PL4", L"Platform PL1", L"Platform PL2" };

    for (ULONG i = 0; i < PhpHwPackageCount; i++)
    {
        PPHP_HW_PACKAGE package = &PhpHwPackages[i];
        BOOLEAN listed[PHP_HW_LIMIT_SENSORS] = { 0 };
        ULONG64 value;

        for (ULONG j = 0; j < PHP_HW_LIMIT_SENSORS; j++)
            package->LimitNamedIndex[j] = ULONG_MAX;

        if (PhpHwPowerUnit <= 0.0f)
            continue;

        if (PhpHwSampleValue(&package->Sample, PhpHwRegVrCurrentConfig, &value))
            listed[PHP_HW_LIMIT_PL4] = !!(value & 0x1FFF);

        if (PhpHwSampleValue(&package->Sample, PhpHwRegPlatformPowerLimit, &value))
        {
            listed[PHP_HW_LIMIT_PLATFORM_PL1] = !!(value & 0x8000);
            listed[PHP_HW_LIMIT_PLATFORM_PL2] = !!(value & (1ULL << 47));
        }

        for (ULONG j = 0; j < PHP_HW_LIMIT_SENSORS; j++)
        {
            PPH_STRING name;

            if (!listed[j])
                continue;

            name = PhCreateString(names[j]);
            package->LimitNamedIndex[j] = PhpHwAddNamedSensor(PhHwNamedSensorPowerLimit, i, j, name);
            PhDereferenceObject(name);
        }
    }
}

/**
 * Publishes PL4 and the platform PL1/PL2 of a package; a disabled limit is not valid.
 */
static VOID PhpHwUpdatePlatformLimits(
    _In_ PPHP_HW_PACKAGE Package
    )
{
    ULONG64 value;
    BOOLEAN valid;

    if (Package->LimitNamedIndex[PHP_HW_LIMIT_PL4] != ULONG_MAX)
    {
        valid = PhpHwSampleValue(&Package->Sample, PhpHwRegVrCurrentConfig, &value) && (value & 0x1FFF);
        PhpHwSetNamedSensor(Package->LimitNamedIndex[PHP_HW_LIMIT_PL4], valid, valid ? (FLOAT)(value & 0x1FFF) * PhpHwPowerUnit : 0.0f);
    }

    valid = PhpHwSampleValue(&Package->Sample, PhpHwRegPlatformPowerLimit, &value);

    if (Package->LimitNamedIndex[PHP_HW_LIMIT_PLATFORM_PL1] != ULONG_MAX)
        PhpHwSetNamedSensor(Package->LimitNamedIndex[PHP_HW_LIMIT_PLATFORM_PL1], valid && (value & 0x8000), valid ? (FLOAT)(value & 0x7FFF) * PhpHwPowerUnit : 0.0f);
    if (Package->LimitNamedIndex[PHP_HW_LIMIT_PLATFORM_PL2] != ULONG_MAX)
        PhpHwSetNamedSensor(Package->LimitNamedIndex[PHP_HW_LIMIT_PLATFORM_PL2], valid && (value & (1ULL << 47)), valid ? (FLOAT)((value >> 32) & 0x7FFF) * PhpHwPowerUnit : 0.0f);
}

/**
 * \return PH_HW_CAP_FAN_SPEED and PH_HW_CAP_ACPI_TEMP for the listed EC sensors.
 */
static ULONG PhpHwCrosEcCapabilities(
    VOID
    )
{
    ULONG capabilities = 0;

    for (ULONG i = 0; i < PHP_HW_CROS_EC_SENSORS; i++)
    {
        if (PhpHwCrosEcNamedIndex[i] == ULONG_MAX)
            continue;

        if (i >= PHP_HW_CROS_EC_TEMPS && i < PHP_HW_CROS_EC_TEMPS + PHP_HW_CROS_EC_FANS)
            capabilities |= PH_HW_CAP_FAN_SPEED;
        else
            capabilities |= PH_HW_CAP_ACPI_TEMP;
    }

    return capabilities;
}

static BOOLEAN PhpHwCrosEcSupported(
    _In_ const KPH_HW_CAPABILITIES* Capabilities
    )
{
    return !PhpHwArm64 &&
        FlagOn(Capabilities->Flags, KPH_HW_CAP_BOARD_ROW_SYSTEM) &&
        FlagOn(Capabilities->ImplementedKinds, 1ULL << KPH_DYN_HW_KIND_WINDOW_READ);
}

/**
 * Reads bytes of the ChromeOS EC memory map window of this board's row.
 *
 * \return Successful or errant status; on success every byte was read.
 */
_Success_(NT_SUCCESS(return))
static NTSTATUS PhpHwReadCrosEc(
    _In_ ULONG Offset,
    _Out_writes_(Count) PUCHAR Bytes,
    _In_ ULONG Count
    )
{
    KPH_HW_WINDOW_BATCH_ENTRY entries[PHP_HW_CROS_EC_READ];
    NTSTATUS status;

    assert(Count <= RTL_NUMBER_OF(entries));

    for (ULONG i = 0; i < Count; i++)
    {
        memset(&entries[i], 0, sizeof(KPH_HW_WINDOW_BATCH_ENTRY));
        entries[i].Offset = Offset + i;
        entries[i].Width = 1;
        entries[i].Status = PHP_HW_ENTRY_UNSET;
    }

    status = KphHwReadPlatformWindowBatch(KPH_HW_PLATFORM_BOARD, PHP_HW_CROS_EC_WINDOW, entries, Count);

    for (ULONG i = 0; NT_SUCCESS(status) && i < Count; i++)
        status = entries[i].Status;

    for (ULONG i = 0; NT_SUCCESS(status) && i < Count; i++)
        Bytes[i] = (UCHAR)entries[i].Value;

    return status;
}

/**
 * Lists the temperatures and fans of a ChromeOS EC memory map (Framework NPCX boards) that read at probe time.
 */
static VOID PhpHwProbeCrosEc(
    _In_ const KPH_HW_CAPABILITIES* Capabilities
    )
{
    UCHAR ids[PHP_HW_CROS_EC_ID_LENGTH];
    UCHAR values[PHP_HW_CROS_EC_READ];

    PhpHwCrosEcThermalVersion = 0;
    PhpHwCrosEcFailures = 0;
    PhpHwCrosEcStatus = STATUS_NOT_SUPPORTED;

    for (ULONG i = 0; i < RTL_NUMBER_OF(PhpHwCrosEcNamedIndex); i++)
        PhpHwCrosEcNamedIndex[i] = ULONG_MAX;

    if (!PhpHwCrosEcSupported(Capabilities))
        return;

    // The kernel checks 'E', 'C' at 20h before and after every batch; the map version gates the second sensor bank.
    PhpHwCrosEcStatus = PhpHwReadCrosEc(PHP_HW_CROS_EC_ID, ids, RTL_NUMBER_OF(ids));

    if (!NT_SUCCESS(PhpHwCrosEcStatus))
        return;

    PhpHwCrosEcThermalVersion = ids[3];

    if (!PhpHwCrosEcThermalVersion)
        return;

    PhpHwCrosEcStatus = PhpHwReadCrosEc(0, values, RTL_NUMBER_OF(values));

    if (!NT_SUCCESS(PhpHwCrosEcStatus))
        return;

    for (ULONG i = 0; i < PHP_HW_CROS_EC_SENSORS; i++)
    {
        PPH_STRING name;
        BOOLEAN fan = i >= PHP_HW_CROS_EC_TEMPS && i < PHP_HW_CROS_EC_TEMPS + PHP_HW_CROS_EC_FANS;

        if (fan)
        {
            ULONG offset = PHP_HW_CROS_EC_FAN + (i - PHP_HW_CROS_EC_TEMPS) * 2;
            USHORT rpm = (USHORT)(values[offset] | (values[offset + 1] << 8));

            if (rpm == 0xFFFF)
                continue;

            name = PhFormatString(L"EC fan %lu", i - PHP_HW_CROS_EC_TEMPS + 1);
            PhpHwCrosEcNamedIndex[i] = PhpHwAddNamedSensor(PhHwNamedSensorFanSpeed, 0, PHP_HW_CROS_EC_ID_BASE + i, name);
        }
        else
        {
            ULONG temperature = i < PHP_HW_CROS_EC_TEMPS ? i : i - PHP_HW_CROS_EC_FANS;
            ULONG offset = temperature < PHP_HW_CROS_EC_TEMPS ? temperature : PHP_HW_CROS_EC_TEMP_B + temperature - PHP_HW_CROS_EC_TEMPS;

            if (temperature >= PHP_HW_CROS_EC_TEMPS && PhpHwCrosEcThermalVersion < 2)
                continue;
            if (values[offset] == PHP_HW_CROS_EC_TEMP_ABSENT)
                continue;

            name = PhFormatString(L"EC temperature %lu", temperature + 1);
            PhpHwCrosEcNamedIndex[i] = PhpHwAddNamedSensor(PhHwNamedSensorAcpiTemperature, 0, PHP_HW_CROS_EC_ID_BASE + i, name);
        }

        PhDereferenceObject(name);
    }
}

/**
 * Publishes the EC temperatures (byte + 200 K; FCh-FFh are not readings) and fans (u16 LE RPM; FFFEh stalled).
 * Three failed reads in a row drop the EC.
 */
static VOID PhpHwUpdateCrosEc(
    VOID
    )
{
    UCHAR values[PHP_HW_CROS_EC_READ];
    BOOLEAN listed = FALSE;

    for (ULONG i = 0; i < PHP_HW_CROS_EC_SENSORS && !listed; i++)
        listed = PhpHwCrosEcNamedIndex[i] != ULONG_MAX;

    if (!listed)
        return;

    PhpHwCrosEcStatus = PhpHwReadCrosEc(0, values, RTL_NUMBER_OF(values));

    if (!NT_SUCCESS(PhpHwCrosEcStatus))
    {
        BOOLEAN drop = ++PhpHwCrosEcFailures >= PHP_HW_RETRY_LIMIT;

        if (drop)
            PhTraceInfo("Hardware sensors: ChromeOS EC unavailable: %!STATUS!", PhpHwCrosEcStatus);

        PhAcquireQueuedLockExclusive(&PhpHwNamedSensorLock);

        for (ULONG i = 0; i < PHP_HW_CROS_EC_SENSORS; i++)
        {
            ULONG index = PhpHwCrosEcNamedIndex[i];

            if (index >= PhpHwNamedSensorCount)
                continue;

            PhpHwNamedSensors[index].Valid = FALSE;

            if (drop)
            {
                PhpHwNamedSensors[index].Active = FALSE;
                PhpHwCrosEcNamedIndex[i] = ULONG_MAX;
            }
        }

        PhReleaseQueuedLockExclusive(&PhpHwNamedSensorLock);
        return;
    }

    PhpHwCrosEcFailures = 0;

    for (ULONG i = 0; i < PHP_HW_CROS_EC_SENSORS; i++)
    {
        if (PhpHwCrosEcNamedIndex[i] == ULONG_MAX)
            continue;

        if (i >= PHP_HW_CROS_EC_TEMPS && i < PHP_HW_CROS_EC_TEMPS + PHP_HW_CROS_EC_FANS)
        {
            ULONG offset = PHP_HW_CROS_EC_FAN + (i - PHP_HW_CROS_EC_TEMPS) * 2;
            USHORT rpm = (USHORT)(values[offset] | (values[offset + 1] << 8));

            if (rpm == 0xFFFE)
                rpm = 0;

            PhpHwSetNamedSensor(PhpHwCrosEcNamedIndex[i], rpm != 0xFFFF, (FLOAT)rpm);
        }
        else
        {
            ULONG temperature = i < PHP_HW_CROS_EC_TEMPS ? i : i - PHP_HW_CROS_EC_FANS;
            ULONG offset = temperature < PHP_HW_CROS_EC_TEMPS ? temperature : PHP_HW_CROS_EC_TEMP_B + temperature - PHP_HW_CROS_EC_TEMPS;
            UCHAR value = values[offset];

            PhpHwSetNamedSensor(PhpHwCrosEcNamedIndex[i], value < PHP_HW_CROS_EC_TEMP_INVALID, (FLOAT)value + 200.0f - 273.15f);
        }
    }
}

/**
 * Probes the TPMI sensors after the MSRs they stand in for and starts the DIMM and SMU sessions.
 *
 * \return TRUE when a capability is available or a probe or session is pending.
 */
static BOOLEAN PhpHwProbePciSensors(
    _In_ const KPH_HW_CAPABILITIES* Capabilities
    )
{
    BOOLEAN dimms;
    BOOLEAN smu;

    PhpHwProbeTpmi(Capabilities);
    PhpHwProbePmt(Capabilities);
    PhpHwProbeMsrUncore();
    PhpHwProbeGtClock();
    PhpHwProbeRaplLimited();
    PhpHwProbePlatformLimits();
    PhpHwProbeLpit(Capabilities);
    PhpHwProbePwrm(Capabilities);
    dimms = PhpHwRequestDimms(Capabilities);
    smu = PhpHwRequestSmu(Capabilities);
    PhpHwRefreshCapabilities();

    // Deltas start on the next tick.
    PhpHwResetSamples();
    PhpHwSampleTime.QuadPart = 0;
    PhpHwLastSampleTime.QuadPart = 0;

    PhTraceInfo(
        "Hardware sensors probed: capabilities 0x%lx, TSC %lu MHz, TjMax %lu",
        PhpHwCapabilities,
        (ULONG)PhpHwTscMhz,
        PhpHwPackages[0].TjMax
        );

    return PhpHwCapabilities != 0 || PhpHwSmnPending || dimms || smu;
}

/**
 * Probes the ARM64 plan on every LP: the counter frequency, the clock counters and the MIDR.
 *
 * \param Answered Receives FALSE when the driver did not answer the probe batch.
 * \return TRUE when a capability is available.
 */
static BOOLEAN PhpHwProbeArm64(
    _In_ const KPH_HW_CAPABILITIES* Capabilities,
    _Out_ PBOOLEAN Answered
    )
{
    ULONG count = 0;
    ULONG amuCount = 0;
    ULONG64 value;

    *Answered = TRUE;

    if (PhpHwProcessorCount == 0)
        return FALSE;

    if (
        Capabilities->Machine != IMAGE_FILE_MACHINE_ARM64 ||
        !FlagOn(Capabilities->ImplementedKinds, 1ULL << KPH_DYN_HW_KIND_SYSREG_READ)
        )
    {
        PhTraceInfo("Hardware sensors: no system register access, machine 0x%x kinds 0x%llx", Capabilities->Machine, Capabilities->ImplementedKinds);
        return FALSE;
    }

    PhpHwInitializeRegisterPlan();
    PhpHwResetSamples();
    PhpHwArmCounterHz = 0;
    memset(PhpHwArmMidr, 0, sizeof(ULONG) * PhpHwProcessorCount);
    memset(PhpHwArmStalls, 0, sizeof(UCHAR) * PhpHwProcessorCount);
    memset(PhpHwArmDisabled, 0, sizeof(ULONG) * PhpHwProcessorCount);

    for (ULONG i = 0; i < PhpHwProcessorCount; i++)
    {
        for (PHP_HW_REG reg = 0; reg < PhpHwRegMaximum; reg++)
        {
            if (PhpHwRegisters[reg].Register)
                PhpHwAddEntry(&count, reg, i, i);
        }
    }

    if (!PhpHwSendBatch(count))
    {
        PhTraceInfo("Hardware sensors probe unanswered");
        *Answered = FALSE;
        return FALSE;
    }

    // CNTFRQ_EL0 is programmed once by firmware for the whole system; any processor that answered gives it.
    for (ULONG i = 0; i < PhpHwProcessorCount; i++)
    {
        if (PhpHwSampleValue(&PhpHwProcessorSamples[i], PhpHwRegArmFrequency, &value) && value)
        {
            PhpHwArmCounterHz = value;
            break;
        }
    }

    for (ULONG i = 0; i < PhpHwProcessorCount; i++)
    {
        // KSI never writes AMCNTENSET0_EL0; firmware enables the counters or the AMU is unused.
        if (!PhpHwSampleValue(&PhpHwProcessorSamples[i], PhpHwRegArmAmuEnable, &value) || (value & PHP_HW_ARM_AMU_COUNTERS) != PHP_HW_ARM_AMU_COUNTERS)
        {
            PhpHwArmDisableRegister(PhpHwRegArmCycles, i);
            PhpHwArmDisableRegister(PhpHwRegArmConstantCycles, i);
        }

        if (PhpHwArmRegisterUsable(PhpHwRegArmCycles, i) && PhpHwArmRegisterUsable(PhpHwRegArmConstantCycles, i))
        {
            PhpHwArmDisableRegister(PhpHwRegArmPmuCycles, i);
            amuCount++;
        }
        else
        {
            PhpHwArmDisableRegister(PhpHwRegArmCycles, i);
            PhpHwArmDisableRegister(PhpHwRegArmConstantCycles, i);
        }

        if (i == 0 || PhpHwArmMidr[i] != PhpHwArmMidr[i - 1])
            PhTraceInfo("Hardware sensors: processor %lu MIDR 0x%08lx", i, PhpHwArmMidr[i]);
    }

    PhpHwRefreshCapabilities();

    // Deltas start on the next tick.
    PhpHwResetSamples();
    PhpHwSampleTime.QuadPart = 0;
    PhpHwLastSampleTime.QuadPart = 0;

    PhTraceInfo(
        "Hardware sensors probed: capabilities 0x%lx, counter %llu Hz, AMU on %lu of %lu processors",
        PhpHwCapabilities,
        PhpHwArmCounterHz,
        amuCount,
        PhpHwProcessorCount
        );

    return PhpHwCapabilities != 0;
}

/**
 * Probes one register per capability and reads the decode constants.
 *
 * \param Answered Receives FALSE when the result is not final.
 * \return TRUE when a capability is available or a probe is pending.
 */
static BOOLEAN PhpHwProbeProcessor(
    _In_ const KPH_HW_CAPABILITIES* Capabilities,
    _Out_ PBOOLEAN Answered
    )
{
    ULONG count = 0;
    ULONG firstProcessor;
    ULONG64 aperf;

    PhpHwCapabilities = 0;
    PhpHwSmnPending = FALSE;
    PhpHwWindowCount = 0;
    PhpHwTpmiRegionCount = 0;
    *Answered = TRUE;

    PhAcquireQueuedLockExclusive(&PhpHwNamedSensorLock);
    PhpHwNamedSensorCount = 0;
    PhReleaseQueuedLockExclusive(&PhpHwNamedSensorLock);

    PhpHwLpitNamedIndex = ULONG_MAX;
    PhpHwLpitStatus = STATUS_NOT_SUPPORTED;
    PhpHwLpitFrequency = 0.0;
    PhpHwPwrmWindow = 0;
    PhpHwPwrmStatus[0] = STATUS_NOT_SUPPORTED;
    PhpHwPwrmStatus[1] = STATUS_NOT_SUPPORTED;
    PhpHwGtNamedIndex = ULONG_MAX;
    PhpHwGtCaps = 0;
    PhpHwGtRpe = 0;
    PhpHwGtPerfStatusLast = 0;
    PhpHwGtPerfStatusPeak = 0;
    PhpHwRaplLimitedNamedIndex = ULONG_MAX;
    memset(&PhpHwMemoryConfig, 0, sizeof(PHP_HW_INTEL_MEMORY_CONFIG));
    PhpHwPmtEntryCount = 0;
    PhpHwCrosEcStatus = STATUS_NOT_SUPPORTED;

    for (ULONG i = 0; i < PHP_HW_CROS_EC_SENSORS; i++)
        PhpHwCrosEcNamedIndex[i] = ULONG_MAX;

    for (ULONG i = 0; i < PhpHwPackageCount; i++)
    {
        PhpHwPackages[i].UncoreNamedIndex = ULONG_MAX;
        memset(PhpHwPackages[i].LimitNamedIndex, 0xFF, sizeof(PhpHwPackages[i].LimitNamedIndex));
    }

    PhpHwCancelDimms();
    PhpHwCancelSmu();
    PhpHwCancelFans();

    // An absent section is not final: a later dyndata activation can supply one.
    if (!FlagOn(Capabilities->Flags, KPH_HW_CAP_SECTION_PRESENT))
    {
        if (!PhpHwNoDataTraced)
        {
            PhTraceInfo("Hardware sensors unavailable: no hardware data");
            PhpHwNoDataTraced = TRUE;
        }

        *Answered = FALSE;
        return FALSE;
    }

    if (PhpHwArm64)
        return PhpHwProbeArm64(Capabilities, Answered);

    if (
        !FlagOn(Capabilities->Flags, KPH_HW_CAP_CPU_HAS_ROW) ||
        !FlagOn(Capabilities->ImplementedKinds, 1ULL << KPH_DYN_HW_KIND_MSR_READ)
        )
    {
        PhTraceInfo("Hardware sensors: no MSR access, flags 0x%lx kinds 0x%llx", Capabilities->Flags, Capabilities->ImplementedKinds);

        // The SMBus and TPMI sensors do not depend on a CPU row.
        if (!PhpHwIntel)
            return FALSE;

        memset(PhpHwRegisters, 0, sizeof(PhpHwRegisters));
        PhpHwResetSamples();
        PhpHwProbeDecode();

        return PhpHwProbePciSensors(Capabilities);
    }

    PhpHwInitializeRegisterPlan();
    PhpHwResetSamples();

    firstProcessor = PhpHwPackages[0].FirstProcessor;

    for (PHP_HW_REG reg = 0; reg < PhpHwRegMaximum; reg++)
    {
        if (!PhpHwRegisters[reg].Register)
            continue;

        switch (PhpHwRegisters[reg].Scope)
        {
        case PhpHwScopeThread:
            PhpHwAddEntry(&count, reg, firstProcessor, firstProcessor);
            break;
        case PhpHwScopeCore:
            PhpHwAddEntry(&count, reg, firstProcessor, PhpHwCpuSensors[firstProcessor].CoreIndex);
            break;
        case PhpHwScopePackage:
            for (ULONG i = 0; i < PhpHwPackageCount; i++)
                PhpHwAddEntry(&count, reg, PhpHwPackages[i].FirstProcessor, i);
            break;
        }
    }

    if (!PhpHwSendBatch(count))
    {
        PhTraceInfo("Hardware sensors probe unanswered");
        *Answered = FALSE;
        return FALSE;
    }

    // The read-only APERF/MPERF aliases may fault or read zero under a hypervisor.
    if (!PhpHwIntel && (
        !PhpHwSampleValue(&PhpHwProcessorSamples[firstProcessor], PhpHwRegAperf, &aperf) || aperf == 0 ||
        !PhpHwRegisterActive(PhpHwRegMperf)
        ))
    {
        PhpHwSetRegister(PhpHwRegAperf, PH_INTEL_MSR_APERF, PhpHwScopeThread);
        PhpHwSetRegister(PhpHwRegMperf, PH_INTEL_MSR_MPERF, PhpHwScopeThread);
    }

    PhpHwProbeDecode();
    PhpHwPrepareSmn(Capabilities);

    if (PhpHwSmnPending && PhpHwAcquirePciMutant())
    {
        PhpHwProbeSmnLocked();
        PhpHwReleasePciMutant();
    }

    PhpHwProbeWindows(Capabilities);

    return PhpHwProbePciSensors(Capabilities);
}

/**
 * Probes the processor and platform sensors, then the board EC, then requests the ACPI fans.
 *
 * \param Answered Receives FALSE when the result is not final.
 * \return TRUE when a capability is available or a sensor session is pending.
 */
static BOOLEAN PhpHwProbe(
    _In_ const KPH_HW_CAPABILITIES* Capabilities,
    _Out_ PBOOLEAN Answered
    )
{
    BOOLEAN available;
    BOOLEAN platform;

    PhpHwFansOnly = FALSE;
    available = PhpHwProbeProcessor(Capabilities, Answered);

    if (!*Answered)
        return available;

    // The board row's EC does not depend on the CPU row.
    PhpHwProbeCrosEc(Capabilities);
    platform = PhpHwRequestFans(Capabilities) || PhpHwCrosEcCapabilities();

    if (!platform)
        return available;

    if (!available)
        PhpHwFansOnly = TRUE;

    return TRUE;
}

/**
 * Decodes the status half of MSR_CORE_PERF_LIMIT_REASONS (EDK2 SkylakeMsr.h, HaswellMsr.h); the log bits are
 * sticky and left alone.
 */
static ULONG PhpHwDecodeLimitReasons(
    _In_ ULONG64 LimitReasons
    )
{
    static CONST struct { UCHAR Bit; ULONG Flag; } skylake[] =
    {
        { 0, PH_HW_THROTTLE_PROCHOT }, { 1, PH_HW_THROTTLE_THERMAL }, { 4, PH_HW_THROTTLE_RESIDENCY },
        { 5, PH_HW_THROTTLE_RUNNING_AVERAGE_THERMAL }, { 6, PH_HW_THROTTLE_VR_THERMAL }, { 7, PH_HW_THROTTLE_VR_CURRENT },
        { 8, PH_HW_THROTTLE_ELECTRICAL }, { 10, PH_HW_THROTTLE_PL1 }, { 11, PH_HW_THROTTLE_PL2 },
        { 12, PH_HW_THROTTLE_MAX_TURBO }, { 13, PH_HW_THROTTLE_TURBO_ATTENUATION },
    };
    static CONST struct { UCHAR Bit; ULONG Flag; } haswell[] =
    {
        { 0, PH_HW_THROTTLE_PROCHOT }, { 1, PH_HW_THROTTLE_THERMAL }, { 4, PH_HW_THROTTLE_GRAPHICS },
        { 5, PH_HW_THROTTLE_AUTONOMOUS }, { 6, PH_HW_THROTTLE_VR_THERMAL }, { 8, PH_HW_THROTTLE_ELECTRICAL },
        { 9, PH_HW_THROTTLE_POWER_LIMIT }, { 10, PH_HW_THROTTLE_PL1 }, { 11, PH_HW_THROTTLE_PL2 },
        { 12, PH_HW_THROTTLE_MAX_TURBO }, { 13, PH_HW_THROTTLE_TURBO_ATTENUATION },
    };
    ULONG flags = 0;

    if (PhpHwRegisters[PhpHwRegLimitReasons].Register == PH_INTEL_MSR_HSW_CORE_PERF_LIMIT_REASONS)
    {
        for (ULONG i = 0; i < RTL_NUMBER_OF(haswell); i++)
        {
            if (LimitReasons & (1ULL << haswell[i].Bit))
                flags |= haswell[i].Flag;
        }
    }
    else
    {
        for (ULONG i = 0; i < RTL_NUMBER_OF(skylake); i++)
        {
            if (LimitReasons & (1ULL << skylake[i].Bit))
                flags |= skylake[i].Flag;
        }
    }

    return flags;
}

static ULONG PhpHwDecodeThrottle(
    _In_ ULONG64 ThermStatus
    )
{
    ULONG flags = 0;

    if (ThermStatus & (1ULL << 0))
        flags |= PH_HW_THROTTLE_THERMAL;
    if (ThermStatus & (1ULL << 2))
        flags |= PH_HW_THROTTLE_PROCHOT;
    if (ThermStatus & (1ULL << 4))
        flags |= PH_HW_THROTTLE_CRITICAL;
    if (ThermStatus & (1ULL << 10))
        flags |= PH_HW_THROTTLE_POWER_LIMIT;

    return flags;
}

static VOID PhpHwInvalidate(
    VOID
    )
{
    for (ULONG i = 0; i < PhpHwProcessorCount; i++)
        PhpHwCpuSensors[i].Valid = 0;
    for (ULONG i = 0; i < PhpHwPackageCount; i++)
    {
        PhpHwPackageSensors[i].Valid = 0;
        PhpHwPackageSensors[i].CcdValidMask = 0;
    }

    PhpHwSmbiosDataRate = 0;
    PhpHwDecodedDataRate = 0.0f;
    PhpHwDataRateRejected = FALSE;

    PhAcquireQueuedLockExclusive(&PhpHwNamedSensorLock);

    for (ULONG i = 0; i < PhpHwNamedSensorCount; i++)
        PhpHwNamedSensors[i].Valid = FALSE;

    for (ULONG i = 0; i < PHP_HW_DIMM_COUNT; i++)
    {
        PhpHwDimmSensors[i].Valid = FALSE;
        PhpHwDimmSensors[i].Active = FALSE;
    }

    for (ULONG i = 0; i < RTL_NUMBER_OF(PhpHwDimmPmicSensors); i++)
    {
        PhpHwDimmPmicSensors[i].Valid = FALSE;
        PhpHwDimmPmicSensors[i].Active = FALSE;
    }

    for (ULONG i = 0; i < PhpHwSmuSensorCount; i++)
    {
        PhpHwSmuSensors[i].Valid = FALSE;
        PhpHwSmuSensors[i].Active = FALSE;
    }

    for (ULONG i = 0; i < PhpHwFanSensorCount; i++)
    {
        PhpHwFanSensors[i].Valid = FALSE;
        PhpHwFanSensors[i].Active = FALSE;
        PhpHwFanControlSensors[i].Valid = FALSE;
        PhpHwFanControlSensors[i].Active = FALSE;
    }

    memset(PhpHwSmuMemoryClockValid, 0, sizeof(PhpHwSmuMemoryClockValid));

    PhReleaseQueuedLockExclusive(&PhpHwNamedSensorLock);

    PhHwCpuTemperature = 0.0f;
    PhHwCpuPower = 0.0f;
    PhpHwChipsetTemperatureValid = FALSE;
}

/**
 * Decodes the clocks of an ARM64 processor. A cycle counter that stops advancing for
 * PHP_HW_RETRY_LIMIT ticks is not read on that processor again this session.
 */
static VOID PhpHwDecodeProcessorArm64(
    _In_ ULONG Index
    )
{
    PPH_HW_CPU_SENSOR sensor = &PhpHwCpuSensors[Index];
    PPHP_HW_SAMPLE thread = &PhpHwProcessorSamples[Index];
    PHP_HW_ARM_SOURCE source = PhpHwArmSource(Index);
    BOOLEAN amu = source == PhpHwArmSourceAmu;
    PHP_HW_REG cyclesReg = amu ? PhpHwRegArmCycles : PhpHwRegArmPmuCycles;
    ULONG mask = (1UL << PhpHwRegArmCounter) | (1UL << cyclesReg) | (amu ? (1UL << PhpHwRegArmConstantCycles) : 0);
    PH_ARM_CYCLE_SAMPLE last;
    PH_ARM_CYCLE_SAMPLE current;
    PH_ARM_CLOCK_STATUS status;
    FLOAT effectiveMhz;
    FLOAT activeMhz;

    sensor->Valid = 0;

    if (source == PhpHwArmSourceNone)
        return;
    if ((thread->Valid & thread->LastValid & mask) != mask)
        return;

    last.Counter = thread->LastValue[PhpHwRegArmCounter];
    last.Cycles = thread->LastValue[cyclesReg];
    last.ConstantCycles = thread->LastValue[PhpHwRegArmConstantCycles];
    current.Counter = thread->Value[PhpHwRegArmCounter];
    current.Cycles = thread->Value[cyclesReg];
    current.ConstantCycles = thread->Value[PhpHwRegArmConstantCycles];

    status = PhArmCycleClocks(&last, &current, PhpHwArmCounterHz, amu, &effectiveMhz, &activeMhz);

    if (status == PhArmClockStalled)
    {
        if (++PhpHwArmStalls[Index] >= PHP_HW_RETRY_LIMIT)
        {
            PhTraceInfo("Hardware sensor 0x%lx unavailable on processor %lu: the cycle counter stalled", PhpHwRegisters[cyclesReg].Register, Index);

            PhpHwArmStalls[Index] = 0;
            PhpHwArmDisableRegister(cyclesReg, Index);

            if (amu)
                PhpHwArmDisableRegister(PhpHwRegArmConstantCycles, Index);
        }

        return;
    }

    PhpHwArmStalls[Index] = 0;

    if (status != PhArmClockValid)
        return;

    sensor->EffectiveMhz = effectiveMhz;
    sensor->Valid |= PH_HW_CAP_EFFECTIVE_CLOCK;

    if (amu)
    {
        sensor->ActiveMhz = activeMhz;
        sensor->Valid |= PH_HW_CAP_ACTIVE_CLOCK;
    }
}

static VOID PhpHwDecodeProcessor(
    _In_ ULONG Index
    )
{
    PPH_HW_CPU_SENSOR sensor = &PhpHwCpuSensors[Index];
    PPHP_HW_SAMPLE thread = &PhpHwProcessorSamples[Index];
    PPHP_HW_SAMPLE core = &PhpHwCores[sensor->CoreIndex].Sample;
    PPHP_HW_PACKAGE package = &PhpHwPackages[sensor->PackageIndex];
    PPH_HW_PACKAGE_SENSOR packageSensor = &PhpHwPackageSensors[sensor->PackageIndex];
    ULONG valid = 0;
    ULONG64 value;
    ULONG64 deltaAperf;
    ULONG64 deltaMperf;
    FLOAT seconds;

    if (PhpHwArm64)
    {
        PhpHwDecodeProcessorArm64(Index);
        return;
    }

    if (PhpHwSampleDelta(thread, PhpHwRegAperf, &deltaAperf))
    {
        if (PhpHwSampleSeconds(&seconds))
        {
            sensor->EffectiveMhz = PhEffectiveMhzFromAperf(deltaAperf, seconds);
            valid |= PH_HW_CAP_EFFECTIVE_CLOCK;
        }

        if (PhpHwSampleDelta(thread, PhpHwRegMperf, &deltaMperf) && deltaMperf != 0)
        {
            sensor->ActiveMhz = PhActiveMhzFromAperfMperf(deltaAperf, deltaMperf, PhpHwTscMhz);
            valid |= PH_HW_CAP_ACTIVE_CLOCK;
        }
    }

    if (PhpHwIntel)
    {
        if (PhpHwSampleValue(thread, PhpHwRegPerfStatus, &value))
        {
            ULONG ratio = PhIntelPerfStatusRatio(value, &PhpHwIdentity);

            if (ratio)
            {
                sensor->Multiplier = (FLOAT)ratio;
                valid |= PH_HW_CAP_MULTIPLIER;
            }

            if (PhpHwVoltageSupported)
            {
                sensor->Voltage = PhIntelPerfStatusToVolts(value, &PhpHwIdentity);
                valid |= PH_HW_CAP_VOLTAGE;
            }
        }

        if (PhpHwSampleValue(core, PhpHwRegThermStatus, &value) && package->TjMax)
        {
            FLOAT celsius;

            if (PhIntelThermStatusToCelsius(value, package->TjMax, PhpHwCoreReadoutMask, &celsius))
            {
                sensor->Temperature = celsius;
                valid |= PH_HW_CAP_CORE_TEMP;

                if (!(package->TickValid & PH_HW_CAP_CORE_TEMP) || celsius > package->TickMaxCoreTemperature)
                    package->TickMaxCoreTemperature = celsius;

                package->TickValid |= PH_HW_CAP_CORE_TEMP;
            }

            sensor->ThrottleFlags = PhpHwDecodeThrottle(value);
            valid |= PH_HW_CAP_THROTTLE;
            package->TickThrottleFlags |= sensor->ThrottleFlags;
            package->TickValid |= PH_HW_CAP_THROTTLE;
        }
    }
    else
    {
        // HwPstateStatus reports the requested P-state, not the boosted ratio.
        if ((valid & PH_HW_CAP_ACTIVE_CLOCK) && packageSensor->BusMhz > 0.0f)
        {
            sensor->Multiplier = sensor->ActiveMhz / packageSensor->BusMhz;
            valid |= PH_HW_CAP_MULTIPLIER;
        }

        if (PhpHwSampleValue(core, PhpHwRegPerfStatus, &value))
        {
            FLOAT mhz = PhAmdHwPStateStatusToMhz(value, &PhpHwIdentity);
            UCHAR vid;

            if (!(valid & PH_HW_CAP_MULTIPLIER) && mhz > 0.0f && packageSensor->BusMhz > 0.0f)
            {
                sensor->Multiplier = mhz / packageSensor->BusMhz;
                valid |= PH_HW_CAP_MULTIPLIER;
            }

            if (PhpHwVoltageSupported && PhAmdPStateVid(value, &PhpHwIdentity, &vid))
            {
                sensor->Voltage = PhAmdCpuVidToVolts(vid, &PhpHwIdentity);
                valid |= PH_HW_CAP_VOLTAGE;
            }
        }
    }

    sensor->Valid = valid;
}

static VOID PhpHwDecodeSmn(
    _In_ ULONG Index
    )
{
    PPHP_HW_PACKAGE package = &PhpHwPackages[Index];
    PPH_HW_PACKAGE_SENSOR sensor = &PhpHwPackageSensors[Index];
    ULONG value;
    FLOAT celsius;
    ULONG ccdValidMask = 0;

    if (!package->HasRoot)
        return;

    if (PhpHwReadSmn(package, &package->Tctl, PH_AMD_SMN_THM_TCON_CUR_TMP, &value) && PhAmdTctlToCelsius(value, &celsius))
    {
        sensor->Temperature = celsius - PhpHwTctlOffset;
        package->TickValid |= PH_HW_CAP_PACKAGE_TEMP;
    }

    for (ULONG i = 0; i < PhpHwCcdCount; i++)
    {
        if (!(package->CcdMask & (1UL << i)))
            continue;

        if (PhpHwReadSmn(package, &package->Ccd[i], PhpHwCcdBase + i * 4, &value) && PhAmdTccdToCelsius(value, &celsius))
        {
            sensor->CcdTemperature[i] = celsius;
            ccdValidMask |= 1UL << i;

            if (!(package->TickValid & PH_HW_CAP_CCD_TEMP) || celsius > package->TickMaxCoreTemperature)
                package->TickMaxCoreTemperature = celsius;

            package->TickValid |= PH_HW_CAP_CCD_TEMP;
        }
    }

    sensor->CcdValidMask = ccdValidMask;
}

/**
 * \return The lower of two limits, ignoring a limit of 0 (unavailable).
 */
static FLOAT PhpHwLowerLimit(
    _In_ FLOAT Limit1,
    _In_ FLOAT Limit2
    )
{
    if (Limit1 <= 0.0f)
        return Limit2;
    if (Limit2 <= 0.0f)
        return Limit1;

    return min(Limit1, Limit2);
}

static VOID PhpHwDecodePackage(
    _In_ ULONG Index
    )
{
    PPHP_HW_PACKAGE package = &PhpHwPackages[Index];
    PPH_HW_PACKAGE_SENSOR sensor = &PhpHwPackageSensors[Index];
    ULONG valid = package->TickValid;
    ULONG64 value;
    FLOAT watts;
    FLOAT mhz;
    FLOAT celsius;

    if (PhpHwSampleValue(&package->Sample, PhpHwRegPackageThermStatus, &value) && package->TjMax)
    {
        sensor->Temperature = PhIntelPackageThermStatusToCelsius(value, package->TjMax, PhpHwPackageReadoutMask);
        valid |= PH_HW_CAP_PACKAGE_TEMP | PH_HW_CAP_THROTTLE;
        package->TickThrottleFlags |= PhpHwDecodeThrottle(value);
    }
    else if (PhpHwIntel && (valid & PH_HW_CAP_CORE_TEMP))
    {
        sensor->Temperature = package->TickMaxCoreTemperature;
        valid |= PH_HW_CAP_PACKAGE_TEMP;
    }
    else if (package->TickMmioTemperatureValid)
    {
        sensor->Temperature = package->TickMmioTemperature;
        valid |= PH_HW_CAP_PACKAGE_TEMP;
    }
    else if (PhpHwArm64 && PhpHwCounterHottestZone(&celsius))
    {
        // No ARM64 part has a package temperature register; the hottest ACPI thermal zone stands in.
        sensor->Temperature = celsius;
        valid |= PH_HW_CAP_PACKAGE_TEMP;
    }

    // The active limit is the lower of the MSR and MMIO limits; a failed MSR read keeps the last value.
    if (PhpHwPowerUnit > 0.0f && PhpHwSampleValue(&package->Sample, PhpHwRegPowerLimit, &value))
        PhpHwDecodeMsrPowerLimit(package, value);

    if (PhpHwSampleValue(&package->Sample, PhpHwRegLimitReasons, &value))
    {
        package->TickThrottleFlags |= PhpHwDecodeLimitReasons(value);
        valid |= PH_HW_CAP_THROTTLE;
    }

    PhpHwUpdatePlatformLimits(package);

    // The uncore ratio is in units of the bus clock.
    if (package->UncoreNamedIndex != ULONG_MAX)
    {
        BOOLEAN uncoreValid = PhpHwSampleValue(&package->Sample, PhpHwRegUncorePerfStatus, &value) && PhIntelUncoreRatio(value);
        FLOAT busMhz = sensor->BusMhz > 0.0f ? sensor->BusMhz : 100.0f;

        PhpHwSetNamedSensor(package->UncoreNamedIndex, uncoreValid, uncoreValid ? (FLOAT)PhIntelUncoreRatio(value) * busMhz : 0.0f);
    }

    if (package->MsrPowerLimit1 > 0.0f || package->TickMmioPowerLimit1 > 0.0f)
    {
        sensor->ActivePowerLimit1 = PhpHwLowerLimit(package->MsrPowerLimit1, package->TickMmioPowerLimit1);
        valid |= PH_HW_CAP_POWER_LIMIT1;
    }

    if (package->MsrPowerLimit2 > 0.0f || package->TickMmioPowerLimit2 > 0.0f)
    {
        sensor->ActivePowerLimit2 = PhpHwLowerLimit(package->MsrPowerLimit2, package->TickMmioPowerLimit2);
        valid |= PH_HW_CAP_POWER_LIMIT2;
    }

    // The SMU memory clock of the current session wins over the UMC decode.
    if (PhpHwSmuGetMemoryClock(Index, &mhz))
    {
        sensor->MemoryClockMhz = mhz;
        sensor->MemoryDataRate = mhz * 2.0f;
        valid |= PH_HW_CAP_MEMORY_CLOCK;
    }
    else if (package->MemoryClockValid)
    {
        sensor->MemoryClockMhz = package->UmcMemoryClockMhz;
        sensor->MemoryDataRate = package->UmcMemoryDataRate;
        valid |= PH_HW_CAP_MEMORY_CLOCK;
    }

    if (package->MemoryTimingsValid)
    {
        sensor->MemoryTimings = package->MemoryTimings;
        valid |= PH_HW_CAP_MEMORY_TIMINGS;
    }

    if (valid & (PH_HW_CAP_CORE_TEMP | PH_HW_CAP_CCD_TEMP))
        sensor->MaxCoreTemperature = package->TickMaxCoreTemperature;
    if (!(valid & PH_HW_CAP_CCD_TEMP))
        sensor->CcdValidMask = 0;

    sensor->ThrottleFlags = package->TickThrottleFlags;

    if (PhpHwSampleWatts(&package->Sample, PhpHwRegPackageEnergy, PhpHwEnergyUnit, &watts))
    {
        sensor->PackagePower = watts;
        valid |= PH_HW_CAP_RAPL_PACKAGE;
    }

    if (PhpHwSampleWatts(&package->Sample, PhpHwRegPp0Energy, PhpHwEnergyUnit, &watts))
    {
        sensor->CorePower = watts;
        valid |= PH_HW_CAP_RAPL_CORE;
    }
    else if (PhpHwRegisterActive(PhpHwRegCoreEnergy) && package->CoreCount && package->TickCoreCount == package->CoreCount)
    {
        sensor->CorePower = package->TickCorePower;
        valid |= PH_HW_CAP_RAPL_CORE;
    }

    if (PhpHwSampleWatts(&package->Sample, PhpHwRegDramEnergy, PhpHwDramEnergyUnit, &watts))
    {
        package->DramEnergySeen |= watts > 0.0f;

        if (package->DramEnergySeen)
        {
            sensor->DramPower = watts;
            valid |= PH_HW_CAP_RAPL_DRAM;
        }
        else if (package->DramIdleTicks < PHP_HW_ENERGY_IDLE_TICKS)
        {
            package->DramIdleTicks++;
        }
    }

    if (PhpHwSampleWatts(&package->Sample, PhpHwRegPlatformEnergy, PhpHwPlatformEnergyUnit, &watts))
    {
        package->PlatformEnergySeen |= watts > 0.0f;

        if (package->PlatformEnergySeen)
        {
            sensor->PlatformPower = watts;
            valid |= PH_HW_CAP_RAPL_PSYS;
        }
        else if (package->PlatformIdleTicks < PHP_HW_ENERGY_IDLE_TICKS)
        {
            package->PlatformIdleTicks++;
        }
    }

    sensor->Valid = valid;
}

/**
 * Drops the RAPL DRAM and platform registers when no package's counter moved in PHP_HW_ENERGY_IDLE_TICKS samples.
 */
static VOID PhpHwDropIdleEnergy(
    VOID
    )
{
    BOOLEAN dramIdle = PhpHwRegisterActive(PhpHwRegDramEnergy) && PhpHwPackageCount;
    BOOLEAN platformIdle = PhpHwRegisterActive(PhpHwRegPlatformEnergy) && PhpHwPackageCount;

    for (ULONG i = 0; i < PhpHwPackageCount; i++)
    {
        dramIdle &= !PhpHwPackages[i].DramEnergySeen && PhpHwPackages[i].DramIdleTicks >= PHP_HW_ENERGY_IDLE_TICKS;
        platformIdle &= !PhpHwPackages[i].PlatformEnergySeen && PhpHwPackages[i].PlatformIdleTicks >= PHP_HW_ENERGY_IDLE_TICKS;
    }

    if (dramIdle)
    {
        PhTraceInfo("Hardware sensors: RAPL DRAM energy does not count; dropped");
        PhpHwDisableRegister(PhpHwRegDramEnergy);
    }

    if (platformIdle)
    {
        PhTraceInfo("Hardware sensors: RAPL platform energy does not count; dropped");
        PhpHwDisableRegister(PhpHwRegPlatformEnergy);
    }
}

static VOID PhpHwUpdateSensors(
    VOID
    )
{
    ULONG count = 0;
    FLOAT temperature = 0.0f;
    FLOAT power = 0.0f;

    for (ULONG i = 0; i < PhpHwProcessorCount; i++)
        PhpHwRollSample(&PhpHwProcessorSamples[i]);
    for (ULONG i = 0; i < PhpHwCoreCount; i++)
        PhpHwRollSample(&PhpHwCores[i].Sample);
    for (ULONG i = 0; i < PhpHwPackageCount; i++)
    {
        PPHP_HW_PACKAGE package = &PhpHwPackages[i];

        PhpHwRollSample(&package->Sample);
        package->TickValid = 0;
        package->TickThrottleFlags = 0;
        package->TickCoreCount = 0;
        package->TickMaxCoreTemperature = 0.0f;
        package->TickCorePower = 0.0f;
        package->TickMmioTemperatureValid = FALSE;
        package->TickMmioPowerLimit1 = 0.0f;
        package->TickMmioPowerLimit2 = 0.0f;
    }

    PhpHwLastSampleTime = PhpHwSampleTime;

    // Entries are grouped by processor so the driver pins each processor once.
    for (ULONG i = 0; i < PhpHwProcessorCount; i++)
    {
        PPH_HW_CPU_SENSOR sensor = &PhpHwCpuSensors[i];
        BOOLEAN firstOfCore = PhpHwCores[sensor->CoreIndex].FirstProcessor == i;
        BOOLEAN firstOfPackage = PhpHwPackages[sensor->PackageIndex].FirstProcessor == i;

        for (PHP_HW_REG reg = 0; reg < PhpHwRegMaximum; reg++)
        {
            if (!PhpHwRegisterActive(reg) || PhpHwRegisters[reg].ProbeOnly)
                continue;
            if (PhpHwArm64 && !PhpHwArmRegisterUsable(reg, i))
                continue;

            switch (PhpHwRegisters[reg].Scope)
            {
            case PhpHwScopeThread:
                PhpHwAddEntry(&count, reg, i, i);
                break;
            case PhpHwScopeCore:
                if (firstOfCore)
                    PhpHwAddEntry(&count, reg, i, sensor->CoreIndex);
                break;
            case PhpHwScopePackage:
                if (firstOfPackage)
                    PhpHwAddEntry(&count, reg, i, sensor->PackageIndex);
                break;
            }
        }
    }

    if (count)
        PhpHwSendBatch(count);
    else
        PhQueryPerformanceCounter(&PhpHwSampleTime);

    if (!PhpHwIntel && !PhpHwArm64)
    {
        for (ULONG i = 0; i < PhpHwCoreCount; i++)
        {
            PPHP_HW_CORE core = &PhpHwCores[i];
            FLOAT watts;

            if (PhpHwSampleWatts(&core->Sample, PhpHwRegCoreEnergy, PhpHwEnergyUnit, &watts))
            {
                PhpHwPackages[core->PackageIndex].TickCorePower += watts;
                PhpHwPackages[core->PackageIndex].TickCoreCount++;
            }
        }

        if (PhpHwMemoryClockTicks < PHP_HW_MEMORY_CLOCK_INTERVAL)
            PhpHwMemoryClockTicks++;

        // Indirect reads run only under Global\Access_PCI; without it they wait for a later tick.
        if (PhpHwSmnSupported(&PhpHwProbeCapabilities) && PhpHwAcquirePciMutant())
        {
            if (PhpHwSmnPending)
            {
                PhpHwProbeSmnLocked();
            }
            else
            {
                BOOLEAN memoryClock = PhpHwMemoryClockTicks >= PHP_HW_MEMORY_CLOCK_INTERVAL;

                if (memoryClock)
                    PhpHwMemoryClockTicks = 0;

                for (ULONG i = 0; i < PhpHwPackageCount; i++)
                {
                    PhpHwDecodeSmn(i);

                    if (memoryClock)
                        PhpHwReadMemoryClockLocked(i, FALSE);
                }
            }

            PhpHwReleasePciMutant();
        }
    }

    // One batch per window per tick serves every consumer of the window.
    if (PhpHwWindowCount)
    {
        for (ULONG i = 0; i < PhpHwWindowCount; i++)
            PhpHwReadWindow(&PhpHwWindows[i]);

        PhpHwDecodeWindows();
    }

    if (PhpHwTpmiRegionCount)
        PhpHwUpdateTpmi();

    if (PhpHwPmtEntryCount)
        PhpHwUpdatePmt();

    if (PhpHwLpitNamedIndex != ULONG_MAX)
        PhpHwUpdateLpit();

    if (PhpHwPwrmWindow)
        PhpHwUpdatePwrm();

    PhpHwUpdateCrosEc();

    for (ULONG i = 0; i < PhpHwProcessorCount; i++)
        PhpHwDecodeProcessor(i);

    for (ULONG i = 0; i < PhpHwPackageCount; i++)
    {
        PPH_HW_PACKAGE_SENSOR sensor = &PhpHwPackageSensors[i];

        PhpHwDecodePackage(i);

        if ((sensor->Valid & PH_HW_CAP_PACKAGE_TEMP) && sensor->Temperature > temperature)
            temperature = sensor->Temperature;
        if (sensor->Valid & PH_HW_CAP_RAPL_PACKAGE)
            power += sensor->PackagePower;
    }

    PhHwCpuTemperature = temperature;
    PhHwCpuPower = power;

    PhpHwDropIdleEnergy();
    PhpHwRefreshCapabilities();
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
        sensor->Value = Report->Value;
        sensor->UpdateId = PhpHwSourceUpdateId;
        source->NextIndex = index + 1;
    }

    PhReleaseQueuedLockExclusive(&PhpHwSourceDataLock);
}

/**
 * Invokes the callback of each registered source; while the gate is closed every source sensor is stale.
 */
static VOID PhpHwUpdateSources(
    _In_ BOOLEAN GateOpen
    )
{
    PhAcquireQueuedLockExclusive(&PhpHwSourceUpdateLock);

    if (!IsListEmpty(&PhpHwSourceListHead))
    {
        PhAcquireQueuedLockExclusive(&PhpHwSourceDataLock);
        PhpHwSourceUpdateId++;
        PhReleaseQueuedLockExclusive(&PhpHwSourceDataLock);

        if (GateOpen)
        {
            WritePointerRelease(&PhpHwSourceUpdateThread, NtCurrentThreadId());

            for (PLIST_ENTRY entry = PhpHwSourceListHead.Flink; entry != &PhpHwSourceListHead; entry = entry->Flink)
            {
                PPH_HW_SENSOR_SOURCE_REGISTRATION source = CONTAINING_RECORD(entry, PH_HW_SENSOR_SOURCE_REGISTRATION, ListEntry);
                LARGE_INTEGER start;
                LARGE_INTEGER end;

                source->NextIndex = 0;

                PhQueryPerformanceCounter(&start);
                source->Source.Callback(PhpHwSourceReport, source, source->Source.Context);
                PhQueryPerformanceCounter(&end);

                if (
                    !source->SlowTraced &&
                    PhpHwFrequency.QuadPart &&
                    (end.QuadPart - start.QuadPart) * 1000 > PhpHwFrequency.QuadPart * PHP_HW_SOURCE_BUDGET_MS
                    )
                {
                    PhTraceWarning(
                        "Hardware sensor source %lu callback took %llu ms",
                        source->Id,
                        (ULONG64)((end.QuadPart - start.QuadPart) * 1000 / PhpHwFrequency.QuadPart)
                        );
                    source->SlowTraced = TRUE;
                }
            }

            WritePointerRelease(&PhpHwSourceUpdateThread, NULL);
        }

        // Sensors are current against a finished update only.
        PhAcquireQueuedLockExclusive(&PhpHwSourceDataLock);
        PhpHwSourceCompletedUpdateId = PhpHwSourceUpdateId;
        PhReleaseQueuedLockExclusive(&PhpHwSourceDataLock);
    }

    PhReleaseQueuedLockExclusive(&PhpHwSourceUpdateLock);
}

/**
 * Publishes the provider state the hardware report shows. Runs on the provider thread.
 */
static VOID PhpHwPublishDiagnostics(
    VOID
    )
{
    PHP_HW_DIAGNOSTICS diagnostics;

    memset(&diagnostics, 0, sizeof(PHP_HW_DIAGNOSTICS));
    diagnostics.GateOpen = PhpHwGateOpen;
    diagnostics.ProbeAttempted = PhpHwProbeAttempted;
    diagnostics.Available = PhpHwAvailable;
    diagnostics.FansOnly = PhpHwFansOnly;
    diagnostics.Capabilities = PhpHwCapabilities | ((PhpHwArm64 && PhpHwCounterZoneEverSeen()) ? PH_HW_CAP_PACKAGE_TEMP : 0);
    diagnostics.TscMhz = PhpHwTscMhz;
    diagnostics.SmbiosDataRate = PhpHwSmbiosDataRate;
    diagnostics.DecodedDataRate = PhpHwDecodedDataRate;
    diagnostics.DataRateRejected = PhpHwDataRateRejected;
    diagnostics.LpitStatus = PhpHwLpitStatus;
    diagnostics.PwrmWindow = PhpHwPwrmWindow;
    memcpy(diagnostics.PwrmStatus, PhpHwPwrmStatus, sizeof(PhpHwPwrmStatus));
    memcpy(diagnostics.PwrmMinMax, PhpHwPwrmMinMax, sizeof(PhpHwPwrmMinMax));
    diagnostics.LpitFrequency = PhpHwLpitFrequency;
    diagnostics.LpitListed = PhpHwLpitNamedIndex != ULONG_MAX;
    diagnostics.PmtLocatorCount = PhpHwPmtLocatorCount;
    diagnostics.PmtEntryCount = PhpHwPmtEntryCount;
    diagnostics.GtListed = PhpHwGtNamedIndex != ULONG_MAX;
    diagnostics.GtCaps = PhpHwGtCaps;
    diagnostics.GtRpe = PhpHwGtRpe;
    diagnostics.GtPerfStatusLast = PhpHwGtPerfStatusLast;
    diagnostics.GtPerfStatusPeak = PhpHwGtPerfStatusPeak;
    diagnostics.MemoryConfig = PhpHwMemoryConfig;

    for (ULONG i = 0; i < RTL_NUMBER_OF(diagnostics.RankTemperature); i++)
    {
        if (PhpHwWindowValue((PHP_HW_WINDOW_REG)(PhpHwWinMc0Ch0RankTemperature + i), &diagnostics.RankTemperature[i]))
            diagnostics.RankTemperatureValid |= 1UL << i;
    }

    for (ULONG i = 0; i < RTL_NUMBER_OF(diagnostics.Requests); i++)
    {
        if (PhpHwWindowValue64((PHP_HW_WINDOW_REG)(PhpHwWinImc0Requests + i), &diagnostics.Requests[i]))
            diagnostics.RequestsValid |= 1UL << i;
    }
    diagnostics.CrosEcStatus = PhpHwCrosEcStatus;
    diagnostics.CrosEcThermalVersion = PhpHwCrosEcThermalVersion;
    diagnostics.Driver = PhpHwProbeCapabilities;
    diagnostics.WindowCount = PhpHwWindowCount;
    diagnostics.WindowLocatorCount = PhpHwWindowLocatorCount;
    diagnostics.TpmiRegionCount = PhpHwTpmiRegionCount;
    diagnostics.TpmiEntryCount = PhpHwTpmiEntryCount;
    diagnostics.TpmiReportCount = PhpHwTpmiReportCount;
    diagnostics.TpmiReportsOmitted = PhpHwTpmiReportsOmitted;
    memcpy(diagnostics.TpmiReports, PhpHwTpmiReports, sizeof(PHP_HW_TPMI_REPORT) * PhpHwTpmiReportCount);
    diagnostics.PackageCount = PhpHwPackageCount;

    for (ULONG i = 0; i < PhpHwRegMaximum; i++)
    {
        diagnostics.Registers[i] = PhpHwRegisters[i].Register;

        if (PhpHwRegisters[i].Item.Disabled)
            diagnostics.RegistersOff[i / 32] |= 1UL << (i % 32);
    }

    for (ULONG i = 0; i < min(PhpHwPackageCount, PHP_HW_DIAGNOSTICS_MAX_PACKAGES); i++)
    {
        PPHP_HW_PACKAGE package = &PhpHwPackages[i];

        diagnostics.Packages[i].HasRoot = package->HasRoot;
        diagnostics.Packages[i].TctlOff = package->Tctl.Disabled;
        diagnostics.Packages[i].MemoryClockOff = package->MemoryClock.Disabled;
        diagnostics.Packages[i].MemoryTimingsValid = package->MemoryTimingsValid;
        diagnostics.Packages[i].Root = package->Root;
        diagnostics.Packages[i].CcdMask = package->CcdMask;
        diagnostics.Packages[i].MemoryChannel = package->MemoryChannel;
    }

    PhAcquireQueuedLockExclusive(&PhpHwNamedSensorLock);
    PhpHwDiagnostics = diagnostics;
    PhReleaseQueuedLockExclusive(&PhpHwNamedSensorLock);
}

/**
 * Updates the hardware sensors. Called by the process provider right after the CPU information.
 */
VOID PhHwProviderUpdate(
    VOID
    )
{
    BOOLEAN gateOpen;

    // Sized here, after PhStatisticsSampleCount is loaded from the settings.
    if (!PhpHwHistoryInitialized)
    {
        PhInitializeCircularBuffer_FLOAT(&PhHwCpuTemperatureHistory, PhStatisticsSampleCount);
        PhInitializeCircularBuffer_FLOAT(&PhHwCpuPowerHistory, PhStatisticsSampleCount);

        for (ULONG i = 0; i < PhpHwPackageCount; i++)
        {
            PhInitializeCircularBuffer_FLOAT(&PhpHwPackageSensors[i].TemperatureHistory, PhStatisticsSampleCount);
            PhInitializeCircularBuffer_FLOAT(&PhpHwPackageSensors[i].PackagePowerHistory, PhStatisticsSampleCount);
        }

        PhpHwHistoryInitialized = TRUE;
    }

    gateOpen = PhHwSensorsEnabled();

    PhpHwUpdateSources(gateOpen);
    PhpHwUpdateCounters(!!PhCsEnableHardwareSensors);

    if (!PhpHwCpuSensors)
        return;

    if (gateOpen)
    {
        // Probe when the gate opens, which includes a driver reload.
        if (!PhpHwGateOpen)
        {
            PhpHwProbeAttempted = FALSE;
            PhpHwProbeRetryTicks = 0;
            PhpHwNoDataTraced = FALSE;
        }

        if (!PhpHwProbeAttempted)
        {
            // An unanswered probe is retried every PHP_HW_PROBE_RETRY_TICKS ticks.
            if (PhpHwProbeRetryTicks)
            {
                PhpHwProbeRetryTicks--;
            }
            else
            {
                KPH_HW_CAPABILITIES capabilities;
                BOOLEAN answered;

                PhpHwInvalidate();
                PhpHwAvailable = FALSE;

                if (NT_SUCCESS(KphHwQueryCapabilities(&capabilities)))
                {
                    PhpHwProbeCapabilities = capabilities;
                    PhpHwAvailable = PhpHwProbe(&capabilities, &answered);
                    PhpHwProbeAttempted = answered;
                }

                if (!PhpHwProbeAttempted)
                    PhpHwProbeRetryTicks = PHP_HW_PROBE_RETRY_TICKS - 1;
            }
        }
        else if (PhpHwAvailable)
        {
            if (PhpHwFansOnly)
            {
                PhpHwUpdateCrosEc();
                PhpHwCapabilities = PhpHwFanCapabilities() | PhpHwCrosEcCapabilities();
            }
            else
            {
                PhpHwUpdateSensors();
            }
        }
    }
    else if (PhpHwGateOpen)
    {
        PhpHwAvailable = FALSE;
        PhpHwCancelDimms();
        PhpHwCancelSmu();
        PhpHwCancelFans();
        PhpHwInvalidate();
    }

    PhpHwGateOpen = gateOpen;
    PhpHwPublishDiagnostics();
}

/**
 * Adds the current values to the hardware sensor histories. Called with the system histories.
 */
VOID PhHwProviderUpdateHistory(
    VOID
    )
{
    if (!PhpHwHistoryInitialized)
        return;

    PhAddItemCircularBuffer_FLOAT(&PhHwCpuTemperatureHistory, PhHwCpuTemperature);
    PhAddItemCircularBuffer_FLOAT(&PhHwCpuPowerHistory, PhHwCpuPower);

    for (ULONG i = 0; i < PhpHwPackageCount; i++)
    {
        PPH_HW_PACKAGE_SENSOR sensor = &PhpHwPackageSensors[i];

        PhAddItemCircularBuffer_FLOAT(&sensor->TemperatureHistory, (sensor->Valid & PH_HW_CAP_PACKAGE_TEMP) ? sensor->Temperature : 0.0f);
        PhAddItemCircularBuffer_FLOAT(&sensor->PackagePowerHistory, (sensor->Valid & PH_HW_CAP_RAPL_PACKAGE) ? sensor->PackagePower : 0.0f);
    }
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
            info.Current = sensor->UpdateId == PhpHwSourceCompletedUpdateId;

            Callback(&info, Context);
        }
    }

    PhReleaseQueuedLockShared(&PhpHwSourceDataLock);
}

/**
 * Determines whether a source has reported a sensor.
 */
BOOLEAN PhHwSourceSensorsPresent(
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
 * Retrieves a set of named sensors, in index order. The caller holds PhpHwNamedSensorLock.
 *
 * \param Set The set.
 * \param Count Receives the number of sensors in the set.
 * \return The sensors, or NULL past the last set.
 */
static PPH_HW_NAMED_SENSOR PhpHwGetNamedSensorSetLocked(
    _In_ ULONG Set,
    _Out_ PULONG Count
    )
{
    // The driver sets are empty while its sensors are unavailable; the Windows counters do not need it.
    if (Set < PHP_HW_NAMED_SET_COUNTERS && !PhpHwAvailable)
    {
        *Count = 0;
        return PhpHwNamedSensors;
    }

    switch (Set)
    {
    case 0:
        *Count = PhpHwNamedSensorCount;
        return PhpHwNamedSensors;
    case 1:
        *Count = PHP_HW_DIMM_COUNT;
        return PhpHwDimmSensors;
    case 2:
        *Count = RTL_NUMBER_OF(PhpHwDimmVoltageSensors);
        return PhpHwDimmVoltageSensors;
    case 3:
        *Count = PhpHwSmuSensorCount;
        return PhpHwSmuSensors;
    case 4:
        *Count = PhpHwFanSensorCount;
        return PhpHwFanSensors;
    case 5:
        *Count = RTL_NUMBER_OF(PhpHwDimmPmicSensors);
        return PhpHwDimmPmicSensors;
    case 6:
        *Count = PhpHwFanSensorCount;
        return PhpHwFanControlSensors;
    case PHP_HW_NAMED_SET_COUNTERS:
        *Count = PhpHwCounterSensorCount;
        return PhpHwCounterSensors;
    }

    *Count = 0;
    return NULL;
}

/**
 * Retrieves the number of named sensors.
 *
 * \return The number of named sensors, or 0 when hardware sensors are unavailable.
 */
ULONG PhHwGetNamedSensorCount(
    VOID
    )
{
    ULONG total = 0;
    ULONG count;

    PhAcquireQueuedLockShared(&PhpHwNamedSensorLock);

    for (ULONG set = 0; PhpHwGetNamedSensorSetLocked(set, &count); set++)
        total += count;

    PhReleaseQueuedLockShared(&PhpHwNamedSensorLock);

    return total;
}

/**
 * Copies a named sensor.
 *
 * \param Index The index of the sensor.
 * \param Sensor Receives a copy of the sensor.
 * \return TRUE when the sensor exists and is active.
 */
_Success_(return)
BOOLEAN PhHwGetNamedSensor(
    _In_ ULONG Index,
    _Out_ PPH_HW_NAMED_SENSOR Sensor
    )
{
    PPH_HW_NAMED_SENSOR sensors;
    BOOLEAN found = FALSE;
    ULONG count;

    PhAcquireQueuedLockShared(&PhpHwNamedSensorLock);

    for (ULONG set = 0; (sensors = PhpHwGetNamedSensorSetLocked(set, &count)); set++)
    {
        if (Index < count)
        {
            if (sensors[Index].Active)
            {
                *Sensor = sensors[Index];
                found = TRUE;
            }

            break;
        }

        Index -= count;
    }

    PhReleaseQueuedLockShared(&PhpHwNamedSensorLock);

    return found;
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
    PPH_HW_NAMED_SENSOR sensors;
    BOOLEAN found = FALSE;
    ULONG count;

    PhAcquireQueuedLockShared(&PhpHwNamedSensorLock);

    for (ULONG set = 0; !found && (sensors = PhpHwGetNamedSensorSetLocked(set, &count)); set++)
    {
        for (ULONG i = 0; i < count; i++)
        {
            if (
                sensors[i].Active &&
                sensors[i].Valid &&
                sensors[i].Kind == Kind &&
                sensors[i].PackageIndex == PackageIndex &&
                sensors[i].Id == Id
                )
            {
                *Value = sensors[i].Value;
                found = TRUE;
                break;
            }
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
 * Determines whether hardware sensors may be read: the setting is on and the driver has full access.
 *
 * \return TRUE when the gate is open.
 */
BOOLEAN NTAPI PhHwSensorsEnabled(
    VOID
    )
{
    return PhCsEnableHardwareSensors && KsiLevel() == KphLevelMax;
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
 * Determines whether the driver answered the hardware sensors probe since the gate opened.
 */
BOOLEAN PhHwSensorsProbed(
    VOID
    )
{
    return PhpHwGateOpen && PhpHwProbeAttempted;
}

/**
 * Determines whether hardware sensors are available.
 *
 * \return TRUE when the gate is open and at least one capability is available.
 */
BOOLEAN NTAPI PhHwSensorsAvailable(
    VOID
    )
{
    return PhpHwAvailable && PhpHwCapabilities != 0;
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
    if (!PhpHwAvailable)
        return 0;

    if (PhpHwArm64 && PhpHwCounterZoneEverSeen())
        return PhpHwCapabilities | PH_HW_CAP_PACKAGE_TEMP;

    return PhpHwCapabilities;
}

/**
 * Retrieves the sensor of a logical processor.
 *
 * \param Index The flat processor index, as used by PhCpusKernelUsage.
 * \return The sensor, or NULL when the index is out of range or the provider is disabled.
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
 * \return The sensor, or NULL when the index is out of range or the provider is disabled.
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
 * \return The number of packages, or 0 when the provider is disabled.
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
 * Formats a temperature in the unit selected by TemperatureUnitFahrenheit.
 *
 * \param Celsius The temperature in degrees Celsius.
 * \param Precision The number of decimal places.
 * \param Format Receives the value and the unit suffix.
 */
VOID NTAPI PhHwFormatTemperature(
    _In_ FLOAT Celsius,
    _In_ USHORT Precision,
    _Out_writes_(PH_HW_TEMPERATURE_FORMAT_COUNT) PPH_FORMAT Format
    )
{
    if (PhCsTemperatureUnitFahrenheit)
    {
        PhInitFormatF(&Format[0], Celsius * 1.8f + 32.0f, Precision);
        PhInitFormatS(&Format[1], L"\u00b0F");
    }
    else
    {
        PhInitFormatF(&Format[0], Celsius, Precision);
        PhInitFormatS(&Format[1], L"\u00b0C");
    }
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

        PhAppendStringBuilder2(Builder, first ? L" (" : L", ");
        PhAppendStringBuilder2(Builder, (PWSTR)Names[i].Key);
        first = FALSE;
    }

    if (!first)
        PhAppendCharStringBuilder(Builder, L')');

    PhAppendCharStringBuilder(Builder, L'\n');
}

static BOOLEAN PhpHwOddParity(
    _In_ UCHAR Value
    )
{
    return !!(PhCountBits(Value) & 1);
}

/**
 * Writes the DDR5 base configuration of SPD NVM page 0: key byte, module type and the minimum timings in ps.
 */
static VOID PhpHwAppendSpd5118Nvm(
    _Inout_ PPH_STRING_BUILDER Builder,
    _In_reads_(PHP_HW_SPD_NVM_BYTES) const UCHAR* Nvm
    )
{
    static CONST PCWSTR moduleTypes[16] =
    {
        NULL, L"RDIMM", L"UDIMM", L"SODIMM", L"LRDIMM", NULL, NULL, L"MRDIMM",
        NULL, NULL, NULL, L"CUDIMM", NULL, L"CSODIMM", L"CAMM2", NULL
    };
    USHORT timings[5];
    USHORT cycle = Nvm[2] | (Nvm[3] << 8);
    PCWSTR moduleType = moduleTypes[Nvm[1] & 0x0F];

    for (ULONG i = 0; i < RTL_NUMBER_OF(timings); i++)
        timings[i] = Nvm[4 + i * 2] | (Nvm[5 + i * 2] << 8);

    PhAppendFormatStringBuilder(
        Builder,
        L"\n    SPD NVM: key 0x%02x%s, module type 0x%02x%s%s, tCKAVGmin %u ps",
        Nvm[0],
        Nvm[0] == 0x12 ? L" (DDR5)" : Nvm[0] == 0x13 ? L" (LPDDR5)" : L"",
        Nvm[1],
        moduleType ? L" " : L"",
        moduleType ? moduleType : L"",
        cycle
        );

    if (cycle)
        PhAppendFormatStringBuilder(Builder, L" (%.0f MT/s)", 2000000.0 / cycle);

    PhAppendFormatStringBuilder(
        Builder,
        L", tAA %u ps, tRCD %u ps, tRP %u ps, tRAS %u ps, tRC %u ps",
        timings[0],
        timings[1],
        timings[2],
        timings[3],
        timings[4]
        );
}

/**
 * Writes the PMIC5100 registers of a slot and their decode.
 */
static VOID PhpHwAppendPmic(
    _Inout_ PPH_STRING_BUILDER Builder,
    _In_reads_(PHP_HW_PMIC_REPORT_REGISTERS) const UCHAR* Registers
    )
{
    static CONST struct
    {
        UCHAR Bank;
        UCHAR Id;
        PCWSTR Name;
    } vendors[] =
    {
        { 0x8A, 0x8C, L"Richtek" },
        { 0x04, 0x23, L"Renesas" },
        { 0x80, 0xB3, L"Renesas (IDT)" },
        { 0x0B, 0x2A, L"Monolithic Power Systems" },
        { 0x86, 0x32, L"Montage" },
        { 0x0B, 0x10, L"Anpec" },
        { 0x80, 0x97, L"Texas Instruments" },
        { 0x86, 0x9D, L"Rambus" },
    };
    UCHAR bank = PhpHwPmicRegister(Registers, 0x3C);
    UCHAR id = PhpHwPmicRegister(Registers, 0x3D);
    UCHAR revision = PhpHwPmicRegister(Registers, 0x3B);
    PCWSTR vendor = NULL;

    PhAppendStringBuilder2(Builder, L"\n    PMIC");

    for (ULONG i = 0; i < PHP_HW_PMIC_REPORT_REGISTERS; i++)
    {
        if (!i || PhpHwPmicCommands[i] != PhpHwPmicCommands[i - 1] + 1)
            PhAppendFormatStringBuilder(Builder, L" %02Xh:", PhpHwPmicCommands[i]);

        PhAppendFormatStringBuilder(Builder, L" %02x", Registers[i]);
    }

    // JEP106 bank and id, each with odd parity in bit 7.
    if (PhpHwOddParity(bank) && PhpHwOddParity(id) && (id & 0x7F) && (id & 0x7F) != 0x7F)
    {
        for (ULONG i = 0; i < RTL_NUMBER_OF(vendors); i++)
        {
            if (vendors[i].Bank == bank && vendors[i].Id == id)
            {
                vendor = vendors[i].Name;
                break;
            }
        }

        if (!vendor)
            vendor = L"unknown";
    }
    else
    {
        vendor = L"invalid";
    }

    PhAppendFormatStringBuilder(
        Builder,
        L"\n    PMIC vendor %s (0x%02x 0x%02x), revision %u.%u, %s mode, %s, %s, high voltage %s",
        vendor,
        bank,
        id,
        ((revision >> 4) & 0x3) + 1,
        (revision >> 1) & 0x7,
        (PhpHwPmicRegister(Registers, 0x2F) & 0x04) ? L"programmable" : L"secure",
        (PhpHwPmicRegister(Registers, 0x32) & 0x40) ? L"I3C" : L"I2C",
        (PhpHwPmicRegister(Registers, 0x1B) & 0x40) ? ((PhpHwPmicRegister(Registers, 0x1A) & 0x02) ? L"total power meter" : L"power meter") : L"current meter",
        PhpHwPmicHighVoltage(Registers) ? L"yes" : L"no"
        );
}

/**
 * Writes the state of the hardware sensors provider for the hardware report.
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
    static CONST PH_KEY_VALUE_PAIR driverFlagNames[] =
    {
        { L"section", (PVOID)KPH_HW_CAP_SECTION_PRESENT },
        { L"CPU row", (PVOID)KPH_HW_CAP_CPU_HAS_ROW },
        { L"PCI", (PVOID)KPH_HW_CAP_PCI_AVAILABLE },
        { L"ACPI", (PVOID)KPH_HW_CAP_ACPI_AVAILABLE },
        { L"baseboard row", (PVOID)KPH_HW_CAP_BOARD_ROW_BASEBOARD },
        { L"system row", (PVOID)KPH_HW_CAP_BOARD_ROW_SYSTEM },
    };
    static CONST PH_KEY_VALUE_PAIR dynDataNames[] =
    {
        { L"kernel", (PVOID)KPH_DYN_ACTIVE_KERNEL },
        { L"lxcore", (PVOID)KPH_DYN_ACTIVE_LXCORE },
        { L"hardware", (PVOID)KPH_DYN_ACTIVE_HARDWARE },
    };
    static CONST PCWSTR dimmTypes[] = { L"none", L"TSOD", L"SPD5118" };
    PHP_HW_DIAGNOSTICS diagnostics;
    const KPH_HW_CAPABILITIES* driver = &diagnostics.Driver;
    ULONG fanCount;
    PPHP_HW_ACPI_REPORT acpi;
    KPH_DYN_ACTIVE_FLAGS dynData;
    ULONG hostCount;
    ULONG host;
    UCHAR pmicMask;
    UCHAR types[PHP_HW_DIMM_COUNT];
    NTSTATUS pmicStatus[PHP_HW_DIMM_COUNT];
    UCHAR pmicRegisters[PHP_HW_DIMM_COUNT][PHP_HW_PMIC_REPORT_REGISTERS];
    PHP_HW_SPD_REPORT spd[PHP_HW_DIMM_COUNT];
    ULONG spdHost;
    PHP_HW_SMU_REPORT smu[PHP_HW_SMU_MAX_PACKAGES];

    // The provider and worker state is copied under the lock and formatted after it.
    PhAcquireQueuedLockShared(&PhpHwNamedSensorLock);

    diagnostics = PhpHwDiagnostics;
    fanCount = PhpHwFanSensorCount;
    acpi = fanCount ? PhAllocateCopy(PhpHwAcpiReports, fanCount * sizeof(PHP_HW_ACPI_REPORT)) : NULL;
    hostCount = PhpHwDimmReportHostCount;
    host = PhpHwDimmReportHost;
    pmicMask = PhpHwPmicReportMask;
    memcpy(types, PhpHwDimmReportTypes, sizeof(types));
    memcpy(pmicStatus, PhpHwPmicReportStatus, sizeof(pmicStatus));
    memcpy(pmicRegisters, PhpHwPmicReportRegisters, sizeof(pmicRegisters));
    memcpy(spd, PhpHwSpdReports, sizeof(spd));
    spdHost = PhpHwSpdReportHost;

    for (ULONG i = 0; i < PHP_HW_SMU_MAX_PACKAGES; i++)
    {
        smu[i] = PhpHwSmuReports[i];
        smu[i].Table = smu[i].Read && smu[i].Length ? PhAllocateCopy(PhpHwSmuReports[i].Table, smu[i].Length) : NULL;
    }

    PhReleaseQueuedLockShared(&PhpHwNamedSensorLock);

    PhAppendFormatStringBuilder(
        Builder,
        L"Setting: %s\nDriver level: %lu\nGate open: %s\nProbed: %s\nAvailable: %s\nFans only: %s\nCapabilities: ",
        PhCsEnableHardwareSensors ? L"enabled" : L"disabled",
        (ULONG)KsiLevel(),
        diagnostics.GateOpen ? L"yes" : L"no",
        diagnostics.ProbeAttempted ? L"yes" : L"no",
        diagnostics.Available ? L"yes" : L"no",
        diagnostics.FansOnly ? L"yes" : L"no"
        );
    PhpHwAppendFlags(Builder, diagnostics.Capabilities, capabilityNames, RTL_NUMBER_OF(capabilityNames));

    PhAppendStringBuilder2(Builder, L"Dynamic data: ");

    if (KsiLevel() >= KphLevelMin && NT_SUCCESS(KphQueryActiveDynData(&dynData)))
        PhpHwAppendFlags(Builder, dynData, dynDataNames, RTL_NUMBER_OF(dynDataNames));
    else
        PhAppendStringBuilder2(Builder, L"unavailable\n");

    if (driver->Version)
    {
        PhAppendFormatStringBuilder(Builder, L"Driver hardware: version %lu, machine 0x%04x", driver->Version, driver->Machine);

        // ARM64 rows match by the MIDR of each processor, listed under [Processor]; there is no CPUID identity.
        if (driver->Machine != IMAGE_FILE_MACHINE_ARM64)
        {
            PhAppendFormatStringBuilder(
                Builder,
                L", CPU %.12hs family 0x%x model 0x%x stepping 0x%x",
                (PCSTR)driver->CpuVendor,
                driver->DisplayFamily,
                driver->DisplayModel,
                driver->Stepping
                );
        }

        PhAppendFormatStringBuilder(
            Builder,
            L"\nDriver kinds: 0x%016I64x, identity kinds 0x%016I64x\n"
            L"Board hashes: baseboard 0x%016I64x, system 0x%016I64x\n"
            L"Driver flags: ",
            driver->ImplementedKinds,
            driver->ImplementedIdentityKinds,
            driver->BaseboardHash,
            driver->SystemHash
            );
        PhpHwAppendFlags(Builder, driver->Flags, driverFlagNames, RTL_NUMBER_OF(driverFlagNames));
    }
    else
    {
        PhAppendStringBuilder2(Builder, L"Driver hardware: not queried\n");
    }

    if (PhpHwArm64)
    {
        PhAppendFormatStringBuilder(
            Builder,
            L"CPU identity: ARM64, rows by MIDR (see [Processor]); system registers %s\n",
            (PhpHwCapabilities & PH_HW_CAP_EFFECTIVE_CLOCK) ? L"answered" : L"not answered"
            );
    }
    else
    {
        PhAppendFormatStringBuilder(
            Builder,
            L"CPU identity: family 0x%x model 0x%x stepping 0x%x%s\n",
            PhpHwIdentity.DisplayFamily,
            PhpHwIdentity.DisplayModel,
            PhpHwIdentity.Stepping,
            PhpHwIdentity.Hybrid ? L", hybrid" : L""
            );
    }

    PhAppendFormatStringBuilder(
        Builder,
        L"Processors: %lu, cores %lu, packages %lu, TSC %.0f MHz\n",
        PhpHwProcessorCount,
        PhpHwCoreCount,
        PhpHwPackageCount,
        diagnostics.TscMhz
        );

    PhAppendStringBuilder2(Builder, L"Registers:");

    for (ULONG i = 0; i < PhpHwRegMaximum; i++)
    {
        if (diagnostics.Registers[i])
            PhAppendFormatStringBuilder(Builder, L" 0x%lx%s", diagnostics.Registers[i], (diagnostics.RegistersOff[i / 32] & (1UL << (i % 32))) ? L" (off)" : L"");
    }

    PhAppendCharStringBuilder(Builder, L'\n');

    for (ULONG i = 0; i < min(diagnostics.PackageCount, PHP_HW_DIAGNOSTICS_MAX_PACKAGES); i++)
    {
        PPHP_HW_DIAGNOSTICS_PACKAGE package = &diagnostics.Packages[i];

        if (!package->HasRoot)
            continue;

        PhAppendFormatStringBuilder(
            Builder,
            L"Package %lu: root %04x:%02x:%02x.%x, Tctl %s, CCD mask 0x%lx, memory clock %s, channel 0x%lx, timings %s\n",
            i,
            package->Root.Segment,
            package->Root.Bus,
            package->Root.Device,
            package->Root.Function,
            package->TctlOff ? L"off" : L"on",
            package->CcdMask,
            package->MemoryClockOff ? L"off" : L"on",
            package->MemoryChannel,
            package->MemoryTimingsValid ? L"on" : L"off"
            );
    }

    PhAppendFormatStringBuilder(
        Builder,
        L"Windows: %lu of %lu devices\nTPMI: %lu regions, %lu entries\nPMT: %lu devices, %lu samples\nACPI devices: %lu\n",
        diagnostics.WindowCount,
        diagnostics.WindowLocatorCount,
        diagnostics.TpmiRegionCount,
        diagnostics.TpmiEntryCount,
        diagnostics.PmtLocatorCount,
        diagnostics.PmtEntryCount,
        fanCount
        );

    for (ULONG i = 0; i < fanCount; i++)
    {
        static CONST PCWSTR kindNames[] = { L"fan", L"DPTF participant", L"power meter", L"DPTF power participant", L"ATK0110", L"ATK0110 sensor", L"DYTC" };
        PPHP_HW_ACPI_REPORT report = &acpi[i];

        C_ASSERT(RTL_NUMBER_OF(kindNames) == PhpHwAcpiDytc + 1);

        PhAppendFormatStringBuilder(Builder, L"  %hs: %s", report->Path, kindNames[report->Kind < RTL_NUMBER_OF(kindNames) ? report->Kind : 0]);

        if (report->Kind == PhpHwAcpiAtkReading)
        {
            static CONST PCWSTR readers[] = { L"RTMP", L"RVLT", L"RFAN" };

            PhAppendFormatStringBuilder(Builder, L" %s 0x%04lx", readers[min((ULONG)(report->Op - PHP_HW_ATK_OP_RTMP), 2)], report->Arg);
        }

        if (report->Kind == PhpHwAcpiDytc)
        {
            static CONST PCWSTR commands[PHP_HW_DYTC_COMMANDS] = { L"query", L"get", L"capabilities", L"MMC" };

            for (ULONG j = 0; j < PHP_HW_DYTC_COMMANDS; j++)
            {
                if (report->Dytc[j] != ULONG_MAX)
                    PhAppendFormatStringBuilder(Builder, L", %s 0x%08lx", commands[j], report->Dytc[j]);
            }

            // GET: [11:8] function, [15:12] mode, bit 17 lap mode.
            if (report->Dytc[1] != ULONG_MAX)
                PhAppendFormatStringBuilder(Builder, L" (function %lu, mode %lu, lap %lu)", (report->Dytc[1] >> 8) & 0xF, (report->Dytc[1] >> 12) & 0xF, (report->Dytc[1] >> 17) & 1);
        }

        if (report->Kind == PhpHwAcpiDptfPower)
        {
            static CONST PCWSTR getterNames[PHP_HW_POWER_STATICS] = { L"PSRC", L"ARTG", L"CTYP", L"RBHF", L"VBNL", L"CMPP" };

            PhAppendFormatStringBuilder(
                Builder,
                L" %s",
                report->Op == PHP_HW_POWER_OP_PMAX ? L"PMAX" : report->Op == PHP_HW_POWER_OP_PROP ? L"PROP" : L"PBSS"
                );

            if (report->Type != ULONG_MAX)
                PhAppendFormatStringBuilder(Builder, L", PTYP 0x%lx", report->Type);

            for (ULONG j = 0; j < PHP_HW_POWER_STATICS; j++)
            {
                if (report->Power[j] != ULONG_MAX)
                    PhAppendFormatStringBuilder(Builder, L", %s %lu", getterNames[j], report->Power[j]);
            }
        }

        if (report->Kind == PhpHwAcpiPowerMeter)
        {
            if (report->MeterCount)
            {
                PhAppendStringBuilder2(Builder, L", _PMC");

                for (ULONG j = 0; j < report->MeterCount; j++)
                    PhAppendFormatStringBuilder(Builder, L" 0x%lx", report->Meter[j]);
            }

            for (ULONG j = 0; j < RTL_NUMBER_OF(report->MeterStrings); j++)
            {
                static CONST PCWSTR stringNames[] = { L"model", L"serial", L"OEM" };

                C_ASSERT(RTL_NUMBER_OF(stringNames) == RTL_NUMBER_OF(report->MeterStrings));

                if (report->MeterStrings[j][0])
                    PhAppendFormatStringBuilder(Builder, L", %s \"%hs\"", stringNames[j], report->MeterStrings[j]);
            }

            if (report->AveragingMs != ULONG_MAX)
                PhAppendFormatStringBuilder(Builder, L", _GAI %lu ms", report->AveragingMs);
        }

        if (report->Kind == PhpHwAcpiFan)
        {
            if (report->FifCount >= 3)
                PhAppendFormatStringBuilder(Builder, L", _FIF fine-grain %lu step %lu%%", report->Fif[1], report->Fif[2]);

            // _FPS states: control/trip point/RPM/noise in 0.1 dBA/power in mW; '-' is not supported.
            for (ULONG j = 0; j < report->StateCount; j++)
            {
                PhAppendStringBuilder2(Builder, j ? L" " : L", _FPS ");

                for (ULONG k = 0; k < RTL_NUMBER_OF(report->States[j].Values); k++)
                {
                    ULONG value = report->States[j].Values[k];

                    if (k)
                        PhAppendCharStringBuilder(Builder, L'/');

                    if (value == ULONG_MAX)
                        PhAppendCharStringBuilder(Builder, L'-');
                    else
                        PhAppendFormatStringBuilder(Builder, L"%lu", value);
                }
            }
        }

        if (report->Kind == PhpHwAcpiDptfTemperature)
        {
            static CONST PCWSTR tripNames[PHP_HW_DPTF_TRIPS] = { L"_PSV", L"_CRT", L"_AC0", L"GTSH" };

            if (report->Type != ULONG_MAX)
                PhAppendFormatStringBuilder(Builder, L", PTYP 0x%lx", report->Type);

            for (ULONG j = 0; j < PHP_HW_DPTF_TRIPS; j++)
            {
                if (report->Trips[j] == ULONG_MAX)
                    continue;

                // Trip points are tenths of a kelvin; GTSH is a hysteresis, in tenths of a degree.
                if (j == PHP_HW_DPTF_TRIPS - 1)
                    PhAppendFormatStringBuilder(Builder, L", %s %.1f C", tripNames[j], (FLOAT)report->Trips[j] / 10.0f);
                else
                    PhAppendFormatStringBuilder(Builder, L", %s %.1f C", tripNames[j], (FLOAT)report->Trips[j] / 10.0f - 273.15f);
            }
        }

        PhAppendCharStringBuilder(Builder, L'\n');
    }

    if (acpi)
        PhFree(acpi);

    if (diagnostics.SmbiosDataRate || diagnostics.DecodedDataRate > 0.0f)
    {
        PhAppendFormatStringBuilder(
            Builder,
            L"Memory data rate: decoded %.0f MT/s%s, SMBIOS configured %lu MT/s\n",
            diagnostics.DecodedDataRate,
            diagnostics.DataRateRejected ? L" (rejected)" : L"",
            diagnostics.SmbiosDataRate
            );
    }

    if (diagnostics.MemoryConfig.Valid & 0x1)
    {
        static CONST PCWSTR ddrTypes[] = { L"reserved", L"DDR5", L"LPDDR5", L"LPDDR4" };
        ULONG config = diagnostics.MemoryConfig.MemConfig;
        ULONG type = (config >> 6) & 0x7;

        PhAppendFormatStringBuilder(
            Builder,
            L"Memory configuration: MEM_CONFIG 0x%08lx (%s, MC0 %s, MC1 %s)\n",
            config,
            type < RTL_NUMBER_OF(ddrTypes) ? ddrTypes[type] : L"reserved",
            (config & 0x1) ? L"on" : L"off",
            (config & 0x2) ? L"on" : L"off"
            );
    }

    for (ULONG i = 0; i < 2; i++)
    {
        PULONG mad = diagnostics.MemoryConfig.Mad[i];

        if (
            !(diagnostics.MemoryConfig.Valid & (0xEUL << (i * 3))) &&
            !(diagnostics.RankTemperatureValid & (0x3UL << (i * 2))) &&
            !(diagnostics.RequestsValid & (1UL << i))
            )
        {
            continue;
        }

        PhAppendFormatStringBuilder(Builder, L"MC%lu:", i);

        if (diagnostics.MemoryConfig.Valid & (0x2UL << (i * 3)))
            PhAppendFormatStringBuilder(Builder, L" MAD_INTER_CHANNEL 0x%08lx", mad[0]);

        for (ULONG j = 0; j < 2; j++)
        {
            ULONG dimm = mad[1 + j];

            if (!(diagnostics.MemoryConfig.Valid & (0x4UL << (i * 3 + j))))
                continue;

            PhAppendFormatStringBuilder(Builder, L", ch%lu MAD_DIMM 0x%08lx", j, dimm);

            // Size in 0.5 GB: DIMM L [6:0], x16 [7], two ranks [9]; DIMM S [22:16], x16 [24], two ranks [26].
            for (ULONG k = 0; k < 2; k++)
            {
                ULONG size = (dimm >> (k * 16)) & 0x7F;

                if (size)
                {
                    PhAppendFormatStringBuilder(
                        Builder,
                        L" (DIMM %s %.1f GB x%lu %luR)",
                        k ? L"S" : L"L",
                        (FLOAT)size / 2.0f,
                        (dimm & (k ? 1UL << 24 : 1UL << 7)) ? 16 : 8,
                        (dimm & (k ? 1UL << 26 : 1UL << 9)) ? 2 : 1
                        );
                }
            }
        }

        for (ULONG j = 0; j < 2; j++)
        {
            if (diagnostics.RankTemperatureValid & (1UL << (i * 2 + j)))
                PhAppendFormatStringBuilder(Builder, L", ch%lu RANK_TEMPERATURE 0x%08lx", j, diagnostics.RankTemperature[i * 2 + j]);
        }

        if (diagnostics.RequestsValid & (1UL << i))
            PhAppendFormatStringBuilder(Builder, L", PWM_TOTAL_REQCOUNT %I64u", diagnostics.Requests[i]);

        PhAppendCharStringBuilder(Builder, L'\n');
    }

    if (diagnostics.RankTemperatureValid)
        PhAppendStringBuilder2(Builder, L"RANK_TEMPERATURE bytes are MR4 refresh-rate codes, subchannel 0 rank 0 lowest; 1x refresh is 09h on LPDDR5, 02h on DDR5\n");

    if (diagnostics.GtCaps)
    {
        PhAppendFormatStringBuilder(
            Builder,
            L"Graphics: RP0 %lu MHz, RP1 %lu MHz, RPn %lu MHz",
            (diagnostics.GtCaps & 0xFF) * 50,
            ((diagnostics.GtCaps >> 8) & 0xFF) * 50,
            ((diagnostics.GtCaps >> 16) & 0xFF) * 50
            );

        if (diagnostics.GtRpe)
            PhAppendFormatStringBuilder(Builder, L", RPE %lu MHz", ((diagnostics.GtRpe >> 8) & 0xFF) * 50);

        PhAppendFormatStringBuilder(
            Builder,
            L", GT_PERF_STATUS 0x%08lx (peak 0x%08lx), %s\n",
            diagnostics.GtPerfStatusLast,
            diagnostics.GtPerfStatusPeak,
            diagnostics.GtListed ? L"listed" : L"not listed"
            );
    }

    for (ULONG i = 0; i < diagnostics.TpmiReportCount; i++)
    {
        PPHP_HW_TPMI_REPORT report = &diagnostics.TpmiReports[i];

        PhAppendFormatStringBuilder(Builder, L"TPMI %s ", report->Feature == PH_INTEL_TPMI_ID_SST ? L"SST" : L"PLR");

        if (report->PackageIndex == ULONG_MAX)
            PhAppendStringBuilder2(Builder, L"(unmapped)");
        else
            PhAppendFormatStringBuilder(Builder, L"package %lu", report->PackageIndex);

        PhAppendFormatStringBuilder(Builder, L" instance %u: ", report->Instance);

        if (report->Feature == PH_INTEL_TPMI_ID_SST)
        {
            PhAppendFormatStringBuilder(
                Builder,
                L"version %u.%u, level %u%s, TDP %.3f W, TjMax %lu C, P0 %lu MHz, P1 %lu MHz, Pn %lu MHz, Pm %lu MHz\n",
                (report->Version >> 5) & 0x7,
                report->Version & 0x1F,
                report->Level,
                report->Locked ? L" (locked)" : L"",
                (DOUBLE)((report->Value >> 32) & 0x7FFF) / 8.0,
                (ULONG)((report->Value >> 47) & 0xFF),
                (ULONG)(report->Ratios & 0xFF) * 100,
                (ULONG)((report->Ratios >> 8) & 0xFF) * 100,
                (ULONG)((report->Ratios >> 16) & 0xFF) * 100,
                (ULONG)((report->Ratios >> 24) & 0xFF) * 100
                );
        }
        else
        {
            static CONST PCWSTR reasons[] = { L"frequency", L"current", L"power", L"thermal", L"platform", L"MCP", L"RAS", L"misc", L"QoS", L"DFC" };

            PhAppendFormatStringBuilder(Builder, L"reasons since cleared 0x%016I64x", report->Value);

            for (ULONG j = 0; j < RTL_NUMBER_OF(reasons); j++)
            {
                if (report->Value & (1ULL << j))
                    PhAppendFormatStringBuilder(Builder, L", %s", reasons[j]);
            }

            PhAppendCharStringBuilder(Builder, L'\n');
        }
    }

    if (diagnostics.TpmiReportsOmitted)
        PhAppendFormatStringBuilder(Builder, L"TPMI: %lu SST/PLR instances omitted\n", diagnostics.TpmiReportsOmitted);

    if (diagnostics.ProbeAttempted && diagnostics.LpitStatus != STATUS_NOT_SUPPORTED)
    {
        PhAppendFormatStringBuilder(
            Builder,
            L"LPIT: counter 0x%08lx, %.0f Hz, %s\n",
            diagnostics.LpitStatus,
            diagnostics.LpitFrequency,
            diagnostics.LpitListed ? L"listed" : L"not listed"
            );
    }

    if (diagnostics.ProbeAttempted && diagnostics.PwrmStatus[0] != STATUS_NOT_SUPPORTED)
    {
        static CONST PCWSTR offsets[PHP_HW_PWRM_WINDOW_COUNT] = { L"193Ch", L"1098h" };

        PhAppendStringBuilder2(Builder, L"PMC PWRM:");

        for (ULONG i = 0; i < PHP_HW_PWRM_WINDOW_COUNT; i++)
        {
            if (diagnostics.PwrmStatus[i] != STATUS_NOT_SUPPORTED)
                PhAppendFormatStringBuilder(Builder, L" %s 0x%08lx", offsets[i], diagnostics.PwrmStatus[i]);
        }

        if (diagnostics.PwrmWindow)
        {
            static CONST PCWSTR names[2] = { L"min", L"max" };

            PhAppendFormatStringBuilder(Builder, L", op %u", diagnostics.PwrmWindow);

            for (ULONG i = 0; i < 2; i++)
            {
                LONG celsius = (LONG)(diagnostics.PwrmMinMax[i] & 0x1FF);

                if (celsius & 0x100)
                    celsius -= 0x200;
                if (diagnostics.PwrmMinMax[i] != ULONG_MAX)
                    PhAppendFormatStringBuilder(Builder, L", %s %ld C", names[i], celsius);
            }
        }

        PhAppendCharStringBuilder(Builder, L'\n');
    }

    if (diagnostics.ProbeAttempted && diagnostics.CrosEcStatus != STATUS_NOT_SUPPORTED)
    {
        PhAppendFormatStringBuilder(
            Builder,
            L"ChromeOS EC: status 0x%08lx, thermal version %u\n",
            diagnostics.CrosEcStatus,
            diagnostics.CrosEcThermalVersion
            );
    }

    PhpHwWriteCounterDiagnostics(Builder);

    PhAppendFormatStringBuilder(Builder, L"SMBus: %lu hosts, DIMMs on ", hostCount);

    if (host != ULONG_MAX)
        PhAppendFormatStringBuilder(Builder, L"host %lu\n", host);
    else
        PhAppendStringBuilder2(Builder, L"none\n");

    if (spdHost != ULONG_MAX)
        PhAppendFormatStringBuilder(Builder, L"SPD probe: host %lu\n", spdHost);

    for (ULONG i = 0; i < PHP_HW_DIMM_COUNT; i++)
    {
        PPHP_HW_SPD_REPORT report = &spd[i];

        if (types[i] == PhpHwDimmNone && !(pmicMask & (1 << i)) && !report->Ddr4Read && !report->HubRead)
            continue;

        PhAppendFormatStringBuilder(Builder, L"  Slot %lu: %s", i, dimmTypes[types[i] < RTL_NUMBER_OF(dimmTypes) ? types[i] : 0]);

        if (report->Ddr4Read)
            PhAppendFormatStringBuilder(Builder, L", SPD byte 2 0x%02x, byte 14 0x%02x", report->DramType, report->Ddr4Thermal);

        if (report->HubRead)
        {
            PhAppendFormatStringBuilder(Builder, L", MR11 0x%02x", report->Mr11);

            if (report->Mr18Read)
                PhAppendFormatStringBuilder(Builder, L", MR18 0x%02x", report->Mr18);

            if (report->Locked)
                PhAppendFormatStringBuilder(Builder, L" (hub on page %u, locked)", report->Mr11 & 0x07);
            if (report->Mr11 & 0x08)
                PhAppendStringBuilder2(Builder, L" (2-byte addressing)");
            if (report->Mr18 & 0x20)
                PhAppendStringBuilder2(Builder, L" (I3C)");
        }

        if (report->NvmRead)
            PhpHwAppendSpd5118Nvm(Builder, report->Nvm);

        if (pmicMask & (1 << i))
        {
            if (NT_SUCCESS(pmicStatus[i]))
                PhpHwAppendPmic(Builder, pmicRegisters[i]);
            else
                PhAppendFormatStringBuilder(Builder, L"\n    PMIC 0x%08lx", pmicStatus[i]);
        }

        PhAppendCharStringBuilder(Builder, L'\n');
    }

    for (ULONG i = 0; i < PHP_HW_SMU_MAX_PACKAGES; i++)
    {
        static CONST PH_KEY_VALUE_PAIR limitNames[] =
        {
            { L"cHTC", (PVOID)PH_HW_HSMP_LIMIT_CHTC },
            { L"PROCHOT", (PVOID)PH_HW_HSMP_LIMIT_PROCHOT },
            { L"TDC", (PVOID)PH_HW_HSMP_LIMIT_TDC },
            { L"PPT", (PVOID)PH_HW_HSMP_LIMIT_PPT },
            { L"OPN max", (PVOID)PH_HW_HSMP_LIMIT_OPN_MAX },
            { L"reliability", (PVOID)PH_HW_HSMP_LIMIT_RELIABILITY },
            { L"APML", (PVOID)PH_HW_HSMP_LIMIT_APML },
            { L"HSMP", (PVOID)PH_HW_HSMP_LIMIT_HSMP },
        };
        PPHP_HW_SMU_REPORT report = &smu[i];
        const FLOAT* values = report->Table;
        ULONG count = values ? report->Length / sizeof(FLOAT) : 0;

        if (report->LimitSourcesValid)
        {
            PhAppendFormatStringBuilder(Builder, L"HSMP %lu: frequency limit sources: ", i);
            PhpHwAppendFlags(Builder, report->LimitSources, limitNames, RTL_NUMBER_OF(limitNames));
        }

        for (ULONG j = 0; j < PHP_HW_SMU_MAX_MAILBOXES; j++)
        {
            ULONG firmware = report->FirmwareVersions[j];

            if (firmware)
                PhAppendFormatStringBuilder(Builder, L"SMU %lu mailbox %lu firmware: 0x%08lx (%lu.%lu.%lu)\n", i, j, firmware, (firmware >> 16) & 0xFF, (firmware >> 8) & 0xFF, firmware & 0xFF);
        }

        if (!report->Read)
            continue;

        PhAppendFormatStringBuilder(
            Builder,
            L"PM table %lu: status 0x%08lx, version 0x%06lx, %lu bytes, %s\n",
            i,
            report->Status,
            report->Version,
            report->Length,
            report->Decoded ? L"decoded" : L"not decoded"
            );

        for (ULONG j = 0; j < count; j++)
        {
            if (j % 8 == 0)
                PhAppendFormatStringBuilder(Builder, L"  %4lu:", j);

            PhAppendFormatStringBuilder(Builder, L" %12g", values[j]);

            if (j % 8 == 7 || j + 1 == count)
                PhAppendCharStringBuilder(Builder, L'\n');
        }

        if (report->Table)
            PhFree(report->Table);
    }
}
