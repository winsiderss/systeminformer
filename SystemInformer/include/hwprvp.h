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
#include <hwprv.h>

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
