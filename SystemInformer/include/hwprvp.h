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
    PhpHwFamilySmu,
    PhpHwFamilyHsmp,
    PhpHwFamilySmbus,
    PhpHwFamilyAcpi,
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

// TRUE during an update that serves a report request (D11): each pass ends with its report-only reads.
extern BOOLEAN PhpHwReportReads;
extern ULONG PhpHwReportWaitMilliseconds; // how long a report waits for the update that serves it

BOOLEAN PhpHwFamiliesNotShipped(
    VOID
    );

// The named-sensor store: a collector adds its sensors at the probe and sets them in its passes.

#define PHP_HW_MAX_NAMED_SENSORS 128

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

VOID PhpHwWriteCounterDiagnostics(
    _Inout_ PPH_STRING_BUILDER Builder
    );

BOOLEAN PhpHwStatusNotShipped(
    _In_ NTSTATUS Status
    );

// Discovery (hwprvdev.c): what the provider asks Windows or the driver connection.

#define PHP_HW_ANY_VENDOR_ID 0xFFFF // no PCI function has it
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

_Success_(return)
BOOLEAN PhpHwQueryBaseboard(
    _Out_ PPH_STRING* Manufacturer,
    _Out_ PPH_STRING* Product
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
