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
#define PHP_HW_RETRY_BACKOFF 10 // updates between tries after two STATUS_RETRY in a row (D9)

// The driver families in update order (spec 3.2).
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
    PhpHwFamilySmu,
    PhpHwFamilyHsmp,
    PhpHwFamilySmbus,
    PhpHwFamilyAcpi,
    PhpHwFamilyTsens, // after cpu: its cores' temperatures go on the cpu collector's sensors
    PhpHwFamilyMaximum
} PHP_HW_FAMILY_ID;

// A probe or pass returns STATUS_SUCCESS, STATUS_RETRY (no strike; see PHP_HW_RETRY_BACKOFF), STATUS_NOT_SUPPORTED
// (the family is absent for the session) or another status (a failed pass, handled by the collector).
typedef NTSTATUS (*PPHP_HW_FAMILY_PROBE)(VOID);
typedef NTSTATUS (*PPHP_HW_FAMILY_PASS)(VOID);
typedef VOID (*PPHP_HW_FAMILY_END)(VOID);
typedef VOID (*PPHP_HW_FAMILY_DIAGNOSTICS)(_Inout_ PPH_STRING_BUILDER Builder);

typedef struct _PHP_HW_FAMILY
{
    PCWSTR Name;
    ULONG64 Interval; // the least time between passes; 0 for every update
    PPHP_HW_FAMILY_PROBE Probe;
    PPHP_HW_FAMILY_PASS Pass;
    PPHP_HW_FAMILY_END End;
    PPHP_HW_FAMILY_DIAGNOSTICS Diagnostics;
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

NTSTATUS PhpHwCpuProbe(
    VOID
    );

NTSTATUS PhpHwCpuPass(
    VOID
    );

VOID PhpHwCpuEnd(
    VOID
    );

VOID PhpHwCpuDiagnostics(
    _Inout_ PPH_STRING_BUILDER Builder
    );

NTSTATUS PhpHwSmnProbe(
    VOID
    );

NTSTATUS PhpHwSmnPass(
    VOID
    );

VOID PhpHwSmnEnd(
    VOID
    );

VOID PhpHwSmnDiagnostics(
    _Inout_ PPH_STRING_BUILDER Builder
    );

NTSTATUS PhpHwMmioProbe(
    VOID
    );

NTSTATUS PhpHwMmioPass(
    VOID
    );

VOID PhpHwMmioEnd(
    VOID
    );

VOID PhpHwMmioDiagnostics(
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

NTSTATUS PhpHwSmuProbe(
    VOID
    );

NTSTATUS PhpHwSmuPass(
    VOID
    );

VOID PhpHwSmuEnd(
    VOID
    );

VOID PhpHwSmuDiagnostics(
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

NTSTATUS PhpHwCrosEcProbe(
    VOID
    );

NTSTATUS PhpHwCrosEcPass(
    VOID
    );

VOID PhpHwCrosEcEnd(
    VOID
    );

VOID PhpHwCrosEcDiagnostics(
    _Inout_ PPH_STRING_BUILDER Builder
    );

NTSTATUS PhpHwBoardEcProbe(
    VOID
    );

NTSTATUS PhpHwBoardEcPass(
    VOID
    );

VOID PhpHwBoardEcEnd(
    VOID
    );

VOID PhpHwBoardEcDiagnostics(
    _Inout_ PPH_STRING_BUILDER Builder
    );

NTSTATUS PhpHwTsensProbe(
    VOID
    );

NTSTATUS PhpHwTsensPass(
    VOID
    );

VOID PhpHwTsensEnd(
    VOID
    );

VOID PhpHwTsensDiagnostics(
    _Inout_ PPH_STRING_BUILDER Builder
    );

NTSTATUS PhpHwVsecProbe(
    VOID
    );

NTSTATUS PhpHwVsecPass(
    VOID
    );

VOID PhpHwVsecEnd(
    VOID
    );

VOID PhpHwVsecDiagnostics(
    _Inout_ PPH_STRING_BUILDER Builder
    );

NTSTATUS PhpHwAcpiProbe(
    VOID
    );

NTSTATUS PhpHwAcpiPass(
    VOID
    );

VOID PhpHwAcpiEnd(
    VOID
    );

VOID PhpHwAcpiDiagnostics(
    _Inout_ PPH_STRING_BUILDER Builder
    );

NTSTATUS PhpHwSmbusProbe(
    VOID
    );

NTSTATUS PhpHwSmbusPass(
    VOID
    );

VOID PhpHwSmbusEndSession(
    VOID
    );

VOID PhpHwSmbusDiagnostics(
    _Inout_ PPH_STRING_BUILDER Builder
    );

NTSTATUS PhpHwHsmpProbe(
    VOID
    );

NTSTATUS PhpHwHsmpPass(
    VOID
    );

VOID PhpHwHsmpEnd(
    VOID
    );

VOID PhpHwHsmpDiagnostics(
    _Inout_ PPH_STRING_BUILDER Builder
    );

_Success_(return)
BOOLEAN PhpHwHsmpQueryMemoryClock(
    _In_ ULONG Package,
    _Out_ PFLOAT MemoryClock
    );

NTSTATUS PhpHwPlatformProbe(
    VOID
    );

NTSTATUS PhpHwPlatformPass(
    VOID
    );

VOID PhpHwPlatformEnd(
    VOID
    );

VOID PhpHwPlatformDiagnostics(
    _Inout_ PPH_STRING_BUILDER Builder
    );

// What cpu's probe found, for the families that depend on it (called on the provider thread, after cpu's probe).

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

// TRUE during an update that serves a report request (D11): each pass ends with its report-only reads.
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
extern PH_HW_NAMED_SENSOR PhpHwCounterSensors[];

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
BOOLEAN PhpHwCounterClusterPower(
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

NTSTATUS PhpHwPciProbe(
    VOID
    );

NTSTATUS PhpHwPciPass(
    VOID
    );

VOID PhpHwPciEnd(
    VOID
    );

VOID PhpHwPciDiagnostics(
    _Inout_ PPH_STRING_BUILDER Builder
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
