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

#ifndef PH_HWPRVP_H
#define PH_HWPRVP_H

#include <phapp.h>
#include <phplug.h>
#include <hwprv.h>
#include <procprv.h>
#include <cpusensor.h>

extern ULONG PhpHwProcessorCount;
extern ULONG PhpHwPackageCount;
extern PPH_HW_CPU_SENSOR PhpHwCpuSensors;
extern PPH_HW_PACKAGE_SENSOR PhpHwPackageSensors;

extern BOOLEAN PhpHwGateOpen;
extern BOOLEAN PhpHwProbed;
extern BOOLEAN PhpHwAvailable;
extern ULONG PhpHwCapabilities;

extern BOOLEAN PhpHwChipsetTemperatureValid;
extern FLOAT PhpHwChipsetTemperature;

extern PH_QUEUED_LOCK PhpHwNamedSensorLock;
extern PPH_HW_NAMED_SENSOR PhpHwNamedSensors;
extern ULONG PhpHwNamedSensorCount;

// the application's, as phsettings.h declares them
extern ULONG PhCsUpdateInterval;
extern ULONG PhCsEnableHardwareSensors;
extern ULONG PhCsTemperatureUnitFahrenheit;

#define PHP_HW_SECOND 10000000ULL // in PhpHwQueryTime units
#define PHP_HW_RETRY_BACKOFF 10 // updates between tries after two STATUS_RETRY in a row

// The driver families in update order.
typedef enum _PHP_HW_FAMILY_ID
{
    PhpHwFamilyCpu,
    PhpHwFamilySmn,
    PhpHwFamilyMmio,
    PhpHwFamilyPlatform,
    PhpHwFamilyPciConfig,
    PhpHwFamilyVsec,
    PhpHwFamilyCrosEc,
    PhpHwFamilyBoardEc,
    PhpHwFamilyGigabyte,
    PhpHwFamilyAlienware,
    PhpHwFamilyHp,
    PhpHwFamilyProm21,
    PhpHwFamilyAqtion,
    PhpHwFamilyVrm,
    PhpHwFamilySch5027,
    PhpHwFamilySmu,
    PhpHwFamilyHsmp,
    PhpHwFamilySmbus,
    PhpHwFamilyAcpi,
    PhpHwFamilyTsens, // after cpu: its cores' temperatures go on the cpu collector's sensors
    PhpHwFamilySpbm,
    PhpHwFamilyMaximum
} PHP_HW_FAMILY_ID;

// A probe or pass returns STATUS_SUCCESS, STATUS_RETRY (no strike; see PHP_HW_RETRY_BACKOFF), STATUS_NOT_SUPPORTED
// (the family is absent for the session) or another status (a failed pass, handled by the collector).
_Function_class_(PHP_HW_FAMILY_PROBE)
typedef NTSTATUS (NTAPI PHP_HW_FAMILY_PROBE)(
    VOID
    );

_Function_class_(PHP_HW_FAMILY_PASS)
typedef NTSTATUS (NTAPI PHP_HW_FAMILY_PASS)(
    VOID
    );

_Function_class_(PHP_HW_FAMILY_END)
typedef VOID (NTAPI PHP_HW_FAMILY_END)(
    VOID
    );

_Function_class_(PHP_HW_FAMILY_DIAGNOSTICS)
typedef VOID (NTAPI PHP_HW_FAMILY_DIAGNOSTICS)(
    _Inout_ PPH_STRING_BUILDER Builder
    );

typedef struct _PHP_HW_FAMILY
{
    PCWSTR Name;
    ULONG64 Interval; // the least time between passes; 0 for every update
    PHP_HW_FAMILY_PROBE* Probe;
    PHP_HW_FAMILY_PASS* Pass;
    PHP_HW_FAMILY_END* End;
    PHP_HW_FAMILY_DIAGNOSTICS* Diagnostics;
    ULONG Capabilities; // PH_HW_CAP_* of the family's named sensors, set by its pass

    // the core's, for the session
    BOOLEAN Absent;
    BOOLEAN NotShipped; // absent because the script is not shipped with this build
    BOOLEAN Ran;
    ULONG64 LastRun;
    ULONG RetryStreak;
    ULONG BackoffUpdates;
} PHP_HW_FAMILY, *PPHP_HW_FAMILY;

extern PHP_HW_FAMILY PhpHwFamilies[PhpHwFamilyMaximum];

// Collector cpu (hwprvcpu.c).

_Function_class_(PHP_HW_FAMILY_PROBE)
NTSTATUS NTAPI PhpHwCpuProbe(
    VOID
    );

_Function_class_(PHP_HW_FAMILY_PASS)
NTSTATUS NTAPI PhpHwCpuPass(
    VOID
    );

_Function_class_(PHP_HW_FAMILY_END)
VOID NTAPI PhpHwCpuEnd(
    VOID
    );

_Function_class_(PHP_HW_FAMILY_DIAGNOSTICS)
VOID NTAPI PhpHwCpuDiagnostics(
    _Inout_ PPH_STRING_BUILDER Builder
    );

_Function_class_(PHP_HW_FAMILY_PROBE)
NTSTATUS NTAPI PhpHwSmnProbe(
    VOID
    );

_Function_class_(PHP_HW_FAMILY_PASS)
NTSTATUS NTAPI PhpHwSmnPass(
    VOID
    );

_Function_class_(PHP_HW_FAMILY_END)
VOID NTAPI PhpHwSmnEnd(
    VOID
    );

_Function_class_(PHP_HW_FAMILY_DIAGNOSTICS)
VOID NTAPI PhpHwSmnDiagnostics(
    _Inout_ PPH_STRING_BUILDER Builder
    );

_Function_class_(PHP_HW_FAMILY_PROBE)
NTSTATUS NTAPI PhpHwMmioProbe(
    VOID
    );

_Function_class_(PHP_HW_FAMILY_PASS)
NTSTATUS NTAPI PhpHwMmioPass(
    VOID
    );

_Function_class_(PHP_HW_FAMILY_END)
VOID NTAPI PhpHwMmioEnd(
    VOID
    );

_Function_class_(PHP_HW_FAMILY_DIAGNOSTICS)
VOID NTAPI PhpHwMmioDiagnostics(
    _Inout_ PPH_STRING_BUILDER Builder
    );

FLOAT PhpHwMmioPowerLimit(
    _In_ ULONG Package,
    _In_ BOOLEAN Second
    );

BOOLEAN PhpHwMmioChipsetPlanned(
    VOID
    );

_Success_(return)
BOOLEAN PhpHwSmnQueryRoot(
    _In_ ULONG Package,
    _Out_ PULONG Locator
    );

_Function_class_(PHP_HW_FAMILY_PROBE)
NTSTATUS NTAPI PhpHwSmuProbe(
    VOID
    );

_Function_class_(PHP_HW_FAMILY_PASS)
NTSTATUS NTAPI PhpHwSmuPass(
    VOID
    );

_Function_class_(PHP_HW_FAMILY_END)
VOID NTAPI PhpHwSmuEnd(
    VOID
    );

_Function_class_(PHP_HW_FAMILY_DIAGNOSTICS)
VOID NTAPI PhpHwSmuDiagnostics(
    _Inout_ PPH_STRING_BUILDER Builder
    );

_Success_(return)
BOOLEAN PhpHwSmuQueryMemoryClock(
    _In_ ULONG Package,
    _Out_ PFLOAT MemoryClock
    );

typedef struct _PHP_HW_SMU_SENSOR
{
    PH_HW_NAMED_SENSOR_KIND Kind;
    ULONG Id;
    ULONG Index; // in the named-sensor store
} PHP_HW_SMU_SENSOR, *PPHP_HW_SMU_SENSOR;

typedef struct _PHP_HW_SMU_SENSORS
{
    ULONG Count;
    PHP_HW_SMU_SENSOR Sensors[PH_HW_SMU_MAX_READINGS];
} PHP_HW_SMU_SENSORS, *PPHP_HW_SMU_SENSORS;

VOID PhpHwSmuPublish(
    _Inout_ PPHP_HW_SMU_SENSORS Sensors,
    _In_ ULONG PackageIndex,
    _In_ ULONG IdFlags,
    _In_ const PH_HW_SMU_DECODE* Decode
    );

_Function_class_(PHP_HW_FAMILY_PROBE)
NTSTATUS NTAPI PhpHwCrosEcProbe(
    VOID
    );

_Function_class_(PHP_HW_FAMILY_PASS)
NTSTATUS NTAPI PhpHwCrosEcPass(
    VOID
    );

_Function_class_(PHP_HW_FAMILY_END)
VOID NTAPI PhpHwCrosEcEnd(
    VOID
    );

_Function_class_(PHP_HW_FAMILY_DIAGNOSTICS)
VOID NTAPI PhpHwCrosEcDiagnostics(
    _Inout_ PPH_STRING_BUILDER Builder
    );

_Function_class_(PHP_HW_FAMILY_PROBE)
NTSTATUS NTAPI PhpHwBoardEcProbe(
    VOID
    );

_Function_class_(PHP_HW_FAMILY_PASS)
NTSTATUS NTAPI PhpHwBoardEcPass(
    VOID
    );

_Function_class_(PHP_HW_FAMILY_END)
VOID NTAPI PhpHwBoardEcEnd(
    VOID
    );

_Function_class_(PHP_HW_FAMILY_DIAGNOSTICS)
VOID NTAPI PhpHwBoardEcDiagnostics(
    _Inout_ PPH_STRING_BUILDER Builder
    );

_Function_class_(PHP_HW_FAMILY_PROBE)
NTSTATUS NTAPI PhpHwGigabyteProbe(
    VOID
    );

_Function_class_(PHP_HW_FAMILY_PASS)
NTSTATUS NTAPI PhpHwGigabytePass(
    VOID
    );

_Function_class_(PHP_HW_FAMILY_END)
VOID NTAPI PhpHwGigabyteEnd(
    VOID
    );

_Function_class_(PHP_HW_FAMILY_DIAGNOSTICS)
VOID NTAPI PhpHwGigabyteDiagnostics(
    _Inout_ PPH_STRING_BUILDER Builder
    );

_Function_class_(PHP_HW_FAMILY_PROBE)
NTSTATUS NTAPI PhpHwAlienwareProbe(
    VOID
    );

_Function_class_(PHP_HW_FAMILY_PASS)
NTSTATUS NTAPI PhpHwAlienwarePass(
    VOID
    );

_Function_class_(PHP_HW_FAMILY_END)
VOID NTAPI PhpHwAlienwareEnd(
    VOID
    );

_Function_class_(PHP_HW_FAMILY_DIAGNOSTICS)
VOID NTAPI PhpHwAlienwareDiagnostics(
    _Inout_ PPH_STRING_BUILDER Builder
    );

_Function_class_(PHP_HW_FAMILY_PROBE)
NTSTATUS NTAPI PhpHwHpProbe(
    VOID
    );

_Function_class_(PHP_HW_FAMILY_PASS)
NTSTATUS NTAPI PhpHwHpPass(
    VOID
    );

_Function_class_(PHP_HW_FAMILY_END)
VOID NTAPI PhpHwHpEnd(
    VOID
    );

_Function_class_(PHP_HW_FAMILY_DIAGNOSTICS)
VOID NTAPI PhpHwHpDiagnostics(
    _Inout_ PPH_STRING_BUILDER Builder
    );

typedef enum _PHP_HW_PMBUS_FAMILY
{
    PhpHwPmbusFamilyNone,
    PhpHwPmbusFamilyInfineonXdpe122, // VOUT in VID, the scheme in VOUT_MODE; the rest LINEAR11
    PhpHwPmbusFamilyTiVid, // TPS53679, TPS53681: VOUT in VID, the scheme in VOUT_MODE; the rest LINEAR11
    PhpHwPmbusFamilyTiLinear, // TPS53676, TPS53685, TPS536C7: LINEAR16 VOUT, the rest LINEAR11
    PhpHwPmbusFamilyRenesasRaa, // ISL68xxx, ISL69xxx, RAA: DIRECT with fixed coefficients
    PhpHwPmbusFamilyIr35221, // LINEAR16 VOUT, the rest LINEAR11
    PhpHwPmbusFamilyMps // MP2975, MP2973: VIN alone (the rest needs vendor page 2)
} PHP_HW_PMBUS_FAMILY;

typedef enum _PHP_HW_PMBUS_QUANTITY
{
    PhpHwPmbusVin, // READ_VIN 88h
    PhpHwPmbusVout, // READ_VOUT 8Bh
    PhpHwPmbusIin, // READ_IIN 89h
    PhpHwPmbusIout, // READ_IOUT 8Ch
    PhpHwPmbusPin, // READ_PIN 97h
    PhpHwPmbusPout, // READ_POUT 96h
    PhpHwPmbusTemperature, // READ_TEMPERATURE_1 8Dh
    PhpHwPmbusQuantityMaximum
} PHP_HW_PMBUS_QUANTITY;

_Success_(return)
BOOLEAN PhpHwPmbusDecode(
    _In_ PHP_HW_PMBUS_FAMILY Family,
    _In_ PHP_HW_PMBUS_QUANTITY Quantity,
    _In_ UCHAR VoutMode,
    _In_ USHORT Raw,
    _Out_ PDOUBLE Value
    );

PHP_HW_PMBUS_FAMILY PhpHwPmbusIdentify(
    _In_ UCHAR Revision,
    _In_reads_bytes_(MfrIdLength) const UCHAR* MfrId,
    _In_ ULONG MfrIdLength,
    _In_reads_bytes_(ModelLength) const UCHAR* Model,
    _In_ ULONG ModelLength,
    _In_reads_bytes_(DeviceIdLength) const UCHAR* DeviceId,
    _In_ ULONG DeviceIdLength
    );

_Function_class_(PHP_HW_FAMILY_PROBE)
NTSTATUS NTAPI PhpHwSch5027Probe(
    VOID
    );

_Function_class_(PHP_HW_FAMILY_PASS)
NTSTATUS NTAPI PhpHwSch5027Pass(
    VOID
    );

_Function_class_(PHP_HW_FAMILY_END)
VOID NTAPI PhpHwSch5027End(
    VOID
    );

_Function_class_(PHP_HW_FAMILY_DIAGNOSTICS)
VOID NTAPI PhpHwSch5027Diagnostics(
    _Inout_ PPH_STRING_BUILDER Builder
    );

_Function_class_(PHP_HW_FAMILY_PROBE)
NTSTATUS NTAPI PhpHwVrmProbe(
    VOID
    );

_Function_class_(PHP_HW_FAMILY_PASS)
NTSTATUS NTAPI PhpHwVrmPass(
    VOID
    );

_Function_class_(PHP_HW_FAMILY_END)
VOID NTAPI PhpHwVrmEnd(
    VOID
    );

_Function_class_(PHP_HW_FAMILY_DIAGNOSTICS)
VOID NTAPI PhpHwVrmDiagnostics(
    _Inout_ PPH_STRING_BUILDER Builder
    );

_Function_class_(PHP_HW_FAMILY_PROBE)
NTSTATUS NTAPI PhpHwProm21Probe(
    VOID
    );

_Function_class_(PHP_HW_FAMILY_PASS)
NTSTATUS NTAPI PhpHwProm21Pass(
    VOID
    );

_Function_class_(PHP_HW_FAMILY_END)
VOID NTAPI PhpHwProm21End(
    VOID
    );

_Function_class_(PHP_HW_FAMILY_DIAGNOSTICS)
VOID NTAPI PhpHwProm21Diagnostics(
    _Inout_ PPH_STRING_BUILDER Builder
    );

_Function_class_(PHP_HW_FAMILY_PROBE)
NTSTATUS NTAPI PhpHwAqtionProbe(
    VOID
    );

_Function_class_(PHP_HW_FAMILY_PASS)
NTSTATUS NTAPI PhpHwAqtionPass(
    VOID
    );

_Function_class_(PHP_HW_FAMILY_END)
VOID NTAPI PhpHwAqtionEnd(
    VOID
    );

_Function_class_(PHP_HW_FAMILY_DIAGNOSTICS)
VOID NTAPI PhpHwAqtionDiagnostics(
    _Inout_ PPH_STRING_BUILDER Builder
    );

_Function_class_(PHP_HW_FAMILY_PROBE)
NTSTATUS NTAPI PhpHwTsensProbe(
    VOID
    );

_Function_class_(PHP_HW_FAMILY_PASS)
NTSTATUS NTAPI PhpHwTsensPass(
    VOID
    );

_Function_class_(PHP_HW_FAMILY_END)
VOID NTAPI PhpHwTsensEnd(
    VOID
    );

_Function_class_(PHP_HW_FAMILY_DIAGNOSTICS)
VOID NTAPI PhpHwTsensDiagnostics(
    _Inout_ PPH_STRING_BUILDER Builder
    );

_Function_class_(PHP_HW_FAMILY_PROBE)
NTSTATUS NTAPI PhpHwSpbmProbe(
    VOID
    );

_Function_class_(PHP_HW_FAMILY_PASS)
NTSTATUS NTAPI PhpHwSpbmPass(
    VOID
    );

_Function_class_(PHP_HW_FAMILY_END)
VOID NTAPI PhpHwSpbmEnd(
    VOID
    );

_Function_class_(PHP_HW_FAMILY_DIAGNOSTICS)
VOID NTAPI PhpHwSpbmDiagnostics(
    _Inout_ PPH_STRING_BUILDER Builder
    );

_Function_class_(PHP_HW_FAMILY_PROBE)
NTSTATUS NTAPI PhpHwVsecProbe(
    VOID
    );

_Function_class_(PHP_HW_FAMILY_PASS)
NTSTATUS NTAPI PhpHwVsecPass(
    VOID
    );

_Function_class_(PHP_HW_FAMILY_END)
VOID NTAPI PhpHwVsecEnd(
    VOID
    );

_Function_class_(PHP_HW_FAMILY_DIAGNOSTICS)
VOID NTAPI PhpHwVsecDiagnostics(
    _Inout_ PPH_STRING_BUILDER Builder
    );

ULONG PhpHwVsecPackageCaps(
    _In_ ULONG PackageIndex
    );

_Function_class_(PHP_HW_FAMILY_PROBE)
NTSTATUS NTAPI PhpHwAcpiProbe(
    VOID
    );

_Function_class_(PHP_HW_FAMILY_PASS)
NTSTATUS NTAPI PhpHwAcpiPass(
    VOID
    );

_Function_class_(PHP_HW_FAMILY_END)
VOID NTAPI PhpHwAcpiEnd(
    VOID
    );

_Function_class_(PHP_HW_FAMILY_DIAGNOSTICS)
VOID NTAPI PhpHwAcpiDiagnostics(
    _Inout_ PPH_STRING_BUILDER Builder
    );

_Function_class_(PHP_HW_FAMILY_PROBE)
NTSTATUS NTAPI PhpHwSmbusProbe(
    VOID
    );

_Function_class_(PHP_HW_FAMILY_PASS)
NTSTATUS NTAPI PhpHwSmbusPass(
    VOID
    );

_Function_class_(PHP_HW_FAMILY_END)
VOID NTAPI PhpHwSmbusEndSession(
    VOID
    );

_Function_class_(PHP_HW_FAMILY_DIAGNOSTICS)
VOID NTAPI PhpHwSmbusDiagnostics(
    _Inout_ PPH_STRING_BUILDER Builder
    );

_Function_class_(PHP_HW_FAMILY_PROBE)
NTSTATUS NTAPI PhpHwHsmpProbe(
    VOID
    );

_Function_class_(PHP_HW_FAMILY_PASS)
NTSTATUS NTAPI PhpHwHsmpPass(
    VOID
    );

_Function_class_(PHP_HW_FAMILY_END)
VOID NTAPI PhpHwHsmpEnd(
    VOID
    );

_Function_class_(PHP_HW_FAMILY_DIAGNOSTICS)
VOID NTAPI PhpHwHsmpDiagnostics(
    _Inout_ PPH_STRING_BUILDER Builder
    );

_Success_(return)
BOOLEAN PhpHwHsmpQueryMemoryClock(
    _In_ ULONG Package,
    _Out_ PFLOAT MemoryClock
    );

_Function_class_(PHP_HW_FAMILY_PROBE)
NTSTATUS NTAPI PhpHwPlatformProbe(
    VOID
    );

_Function_class_(PHP_HW_FAMILY_PASS)
NTSTATUS NTAPI PhpHwPlatformPass(
    VOID
    );

_Function_class_(PHP_HW_FAMILY_END)
VOID NTAPI PhpHwPlatformEnd(
    VOID
    );

_Function_class_(PHP_HW_FAMILY_DIAGNOSTICS)
VOID NTAPI PhpHwPlatformDiagnostics(
    _Inout_ PPH_STRING_BUILDER Builder
    );

// What cpu's probe found, for the families that depend on it (called on the provider thread, after cpu's probe).

BOOLEAN PhpHwCpuIsArm64(
    VOID
    );

_Success_(return)
BOOLEAN PhpHwCpuQueryIntel(
    _Out_ PPH_CPU_IDENTITY Identity
    );

_Success_(return)
BOOLEAN PhpHwCpuQueryAmdLegacy(
    _Out_ PPH_CPU_IDENTITY Identity
    );

BOOLEAN PhpHwCpuThermalPlanned(
    VOID
    );

BOOLEAN PhpHwCpuEnergyPlanned(
    _In_ ULONG Kind
    );

FLOAT PhpHwCpuQueryPowerUnit(
    VOID
    );

ULONG PhpHwCpuPackageValid(
    _In_ ULONG PackageIndex
    );

// TRUE during an update that serves a report request: each pass ends with its report-only reads.
extern BOOLEAN PhpHwReportReads;
extern ULONG PhpHwReportWaitMilliseconds; // how long a report waits for the update that serves it

BOOLEAN PhpHwFamiliesNotShipped(
    VOID
    );

// The named-sensor store: a collector adds its sensors at the probe and sets them in its passes.

#define PHP_HW_MAX_NAMED_SENSORS 384

ULONG PhpHwAddNamedSensor(
    _In_ PH_HW_NAMED_SENSOR_KIND Kind,
    _In_ ULONG PackageIndex,
    _In_ ULONG Id,
    _In_ PCWSTR Name
    );

VOID PhpHwSetNamedSensor(
    _In_ ULONG Index,
    _In_ BOOLEAN Valid,
    _In_ FLOAT Value
    );

// Collector counters (hwprvctr.c): runs on the setting alone, gate open or not. Its sensors are under PhpHwNamedSensorLock.

extern ULONG PhpHwCounterSensorCount;
extern PPH_HW_NAMED_SENSOR PhpHwCounterSensors;

VOID PhpHwUpdateCounters(
    _In_ BOOLEAN Enabled
    );

_Success_(return)
BOOLEAN PhpHwCounterHottestZone(
    _Out_ PFLOAT Celsius
    );

BOOLEAN PhpHwCounterZoneEverSeen(
    VOID
    );

_Success_(return)
BOOLEAN PhpHwCounterPackagePower(
    _Out_ PFLOAT Watts
    );

VOID PhpHwWriteCounterDiagnostics(
    _Inout_ PPH_STRING_BUILDER Builder
    );

BOOLEAN PhpHwStatusNotShipped(
    _In_ NTSTATUS Status
    );

// Discovery (hwprvdev.c): what the provider asks Windows or the driver connection.

#define PHP_HW_ANY_VENDOR_ID 0xFFFF // no PCI function has it
#define PHP_HW_ANY_CLASS_CODE 0xFFFF // no PCI function has it
#define PHP_HW_MEMORY_DEVICE_MAX 8

typedef struct _PHP_HW_PROCESSOR
{
    PROCESSOR_NUMBER Number;
    ULONG CoreIndex;
    ULONG PackageIndex;
} PHP_HW_PROCESSOR, *PPHP_HW_PROCESSOR;

typedef struct _PHP_HW_TOPOLOGY
{
    ULONG ProcessorCount;
    ULONG CoreCount;
    ULONG PackageCount;
    PPHP_HW_PROCESSOR Processors; // in flat processor index order
} PHP_HW_TOPOLOGY, *PPHP_HW_TOPOLOGY;

typedef struct _PHP_HW_MEMORY_DEVICES
{
    ULONG Count; // populated memory devices
    ULONG ConfiguredSpeed; // MT/s, the highest; 0 when unknown
    PPH_STRING Locators[PHP_HW_MEMORY_DEVICE_MAX];
    PPH_STRING Banks[PHP_HW_MEMORY_DEVICE_MAX];
} PHP_HW_MEMORY_DEVICES, *PPHP_HW_MEMORY_DEVICES;

BOOLEAN PhpHwDriverReady(
    VOID
    );

USHORT PhpHwQueryMachine(
    VOID
    );

ULONG PhpHwQueryBuildNumber(
    VOID
    );

BOOLEAN PhpHwQueryCoreBoost(
    VOID
    );

VOID PhpHwPciQueryLegacyClock(
    _In_ PCPH_CPU_IDENTITY Identity,
    _Out_ PULONG P0Definition,
    _Out_ PULONG ClockPowerTimingControl0
    );

_Function_class_(PHP_HW_FAMILY_PROBE)
NTSTATUS NTAPI PhpHwPciProbe(
    VOID
    );

_Function_class_(PHP_HW_FAMILY_PASS)
NTSTATUS NTAPI PhpHwPciPass(
    VOID
    );

_Function_class_(PHP_HW_FAMILY_END)
VOID NTAPI PhpHwPciEnd(
    VOID
    );

_Function_class_(PHP_HW_FAMILY_DIAGNOSTICS)
VOID NTAPI PhpHwPciDiagnostics(
    _Inout_ PPH_STRING_BUILDER Builder
    );

ULONG PhpHwPciPackageCaps(
    _In_ ULONG PackageIndex
    );

_Success_(return)
BOOLEAN PhpHwQueryTopology(
    _Out_ PPHP_HW_TOPOLOGY Topology
    );

VOID PhpHwFreeTopology(
    _Inout_ PPHP_HW_TOPOLOGY Topology
    );

_Success_(return)
BOOLEAN PhpHwQueryComputeUnits(
    _In_ PPHP_HW_TOPOLOGY Topology,
    _Out_writes_(Topology->ProcessorCount) PBOOLEAN FirstOfUnit,
    _Out_ PULONG SampleRatio
    );

ULONG64 PhpHwQueryTime(
    VOID
    );

ULONG PhpHwEnumeratePciFunctions(
    _In_ USHORT VendorId,
    _In_ USHORT AlternateVendorId,
    _In_ USHORT ClassCode,
    _In_reads_opt_(DeviceIdCount) const USHORT* DeviceIds,
    _In_ ULONG DeviceIdCount,
    _In_ BOOLEAN FunctionZeroOnly,
    _Out_writes_to_(Count, return) PULONG Locators,
    _In_ ULONG Count
    );

VOID PhpHwQueryMemoryDevices(
    _Out_ PPHP_HW_MEMORY_DEVICES Devices
    );

VOID PhpHwFreeMemoryDevices(
    _Inout_ PPHP_HW_MEMORY_DEVICES Devices
    );

VOID PhpHwQueryAcpiDeviceNames(
    _In_reads_(Count) const PCSTR* Paths,
    _In_ ULONG Count,
    _Out_writes_(Count) PPH_STRING* Names
    );

_Success_(return)
BOOLEAN PhpHwQueryBaseboard(
    _Out_ PPH_STRING* Manufacturer,
    _Out_ PPH_STRING* Product
    );

_Success_(return)
BOOLEAN PhpHwQuerySystem(
    _Out_ PPH_STRING* Manufacturer,
    _Out_ PPH_STRING* Product,
    _Out_ PPH_STRING* Family
    );

_Success_(return)
BOOLEAN PhpHwAcpiLenovoFans(
    _In_ PCWSTR Manufacturer,
    _In_ PCWSTR Product,
    _In_ PCWSTR Family,
    _Out_writes_(2) PCSTR* Fields,
    _Out_ PULONG Multiplier
    );

_Success_(return)
BOOLEAN PhpHwQueryLpitFrequency(
    _Out_ PULONG64 Frequency
    );

_Ret_maybenull_
PPH_STRING PhpHwQueryProcessorName(
    VOID
    );

#endif
