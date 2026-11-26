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

#ifndef _PH_KPHHW_H
#define _PH_KPHHW_H

#include <kphuser.h>

EXTERN_C_START

typedef struct _KPH_HW_CPU_IDENTITY
{
    CHAR Vendor[12];
    ULONG Family;
    ULONG Model;
    ULONG Stepping;
    ULONG Signature;
    ULONG Hybrid;
} KPH_HW_CPU_IDENTITY, *PKPH_HW_CPU_IDENTITY;

typedef struct _KPH_HW_CPU_ITEM
{
    USHORT Group;
    USHORT Number;
    ULONG Msr;
} KPH_HW_CPU_ITEM, *PKPH_HW_CPU_ITEM;

typedef const KPH_HW_CPU_ITEM* PCKPH_HW_CPU_ITEM;

typedef struct _KPH_HW_VALUE64
{
    NTSTATUS Status;
    ULONG Reserved;
    ULONG64 Value;
} KPH_HW_VALUE64, *PKPH_HW_VALUE64;

typedef struct _KPH_HW_SUPERIO_CHIP
{
    ULONG Family;
    ULONG Chip;
    ULONG Customer;
} KPH_HW_SUPERIO_CHIP, *PKPH_HW_SUPERIO_CHIP;

typedef struct _KPH_HW_VALUE32
{
    NTSTATUS Status;
    ULONG Value;
} KPH_HW_VALUE32, *PKPH_HW_VALUE32;

typedef struct _KPH_HW_SMN_IDENTITY
{
    USHORT Vendor;
    USHORT Device;
    ULONG Family;
    ULONG Model;
    ULONG Stepping;
} KPH_HW_SMN_IDENTITY, *PKPH_HW_SMN_IDENTITY;

typedef struct _KPH_HW_SMU_IDENTITY
{
    ULONG FirmwareVersion;
    ULONG TableVersion;
    ULONG TableLength;
    ULONG Reserved;
} KPH_HW_SMU_IDENTITY, *PKPH_HW_SMU_IDENTITY;

typedef struct _KPH_HW_SMU_TABLE
{
    ULONG Version;
    ULONG Length;
} KPH_HW_SMU_TABLE, *PKPH_HW_SMU_TABLE;

typedef struct _KPH_HW_SMU_COMMAND
{
    ULONG Opcode;
    ULONG Arguments[6];
} KPH_HW_SMU_COMMAND, *PKPH_HW_SMU_COMMAND;

typedef const KPH_HW_SMU_COMMAND* PCKPH_HW_SMU_COMMAND;

typedef struct _KPH_HW_SMU_RESULT
{
    NTSTATUS Status;
    ULONG Response;
    ULONG Results[6];
} KPH_HW_SMU_RESULT, *PKPH_HW_SMU_RESULT;

typedef struct _KPH_HW_HSMP_IDENTITY
{
    ULONG Version;
    ULONG Reserved;
} KPH_HW_HSMP_IDENTITY, *PKPH_HW_HSMP_IDENTITY;

typedef struct _KPH_HW_HSMP_COMMAND
{
    ULONG Opcode;
    ULONG Arguments[8];
} KPH_HW_HSMP_COMMAND, *PKPH_HW_HSMP_COMMAND;

typedef const KPH_HW_HSMP_COMMAND* PCKPH_HW_HSMP_COMMAND;

typedef struct _KPH_HW_HSMP_RESULT
{
    NTSTATUS Status;
    ULONG Response;
    ULONG Results[8];
} KPH_HW_HSMP_RESULT, *PKPH_HW_HSMP_RESULT;

typedef struct _KPH_HW_ACPI_DEVICE
{
    ULONG Row;
    ULONG Ordinal;
    CHAR Id[8];
    CHAR Path[128];
} KPH_HW_ACPI_DEVICE, *PKPH_HW_ACPI_DEVICE;

typedef struct _KPH_HW_SMBUS_HOST
{
    USHORT Vendor;
    USHORT Device;
    ULONG Revision;
    ULONG Kind;
    ULONG Base;
} KPH_HW_SMBUS_HOST, *PKPH_HW_SMBUS_HOST;

typedef struct _KPH_HW_SMBUS_ITEM
{
    UCHAR Address;
    UCHAR Command;
    UCHAR Protocol;
    UCHAR Reserved;
} KPH_HW_SMBUS_ITEM, *PKPH_HW_SMBUS_ITEM;

typedef const KPH_HW_SMBUS_ITEM* PCKPH_HW_SMBUS_ITEM;

#define KPH_HW_SMBUS_BYTE 0
#define KPH_HW_SMBUS_WORD 1
#define KPH_HW_SMBUS_BLOCK 2
#define KPH_HW_SMBUS_PAGED_WORD 3
#define KPH_HW_SMBUS_PAGED_BYTE 4

#define KPH_HW_PMBUS_BLOCK 32

typedef struct _KPH_HW_PMBUS_IDENTITY
{
    UCHAR Revision;
    UCHAR MfrIdLength;
    UCHAR ModelLength;
    UCHAR DeviceIdLength;
    UCHAR MfrId[KPH_HW_PMBUS_BLOCK];
    UCHAR Model[KPH_HW_PMBUS_BLOCK];
    UCHAR DeviceId[KPH_HW_PMBUS_BLOCK];
} KPH_HW_PMBUS_IDENTITY, *PKPH_HW_PMBUS_IDENTITY;

typedef struct _KPH_HW_PCI_MMIO
{
    USHORT Vendor;
    USHORT Device;
    ULONG Windows;
} KPH_HW_PCI_MMIO, *PKPH_HW_PCI_MMIO;

typedef struct _KPH_HW_WINDOW_ITEM
{
    ULONG Offset;
    ULONG Width;
} KPH_HW_WINDOW_ITEM, *PKPH_HW_WINDOW_ITEM;

typedef const KPH_HW_WINDOW_ITEM* PCKPH_HW_WINDOW_ITEM;

typedef struct _KPH_HW_PCI_CONFIG
{
    USHORT Vendor;
    USHORT Device;
    ULONG Ranges;
} KPH_HW_PCI_CONFIG, *PKPH_HW_PCI_CONFIG;

typedef struct _KPH_HW_CROSEC_IDENTITY
{
    CHAR Id[2];
    UCHAR IdVersion;
    UCHAR ThermalVersion;
    UCHAR BatteryVersion;
    UCHAR SwitchesVersion;
    UCHAR EventsVersion;
    UCHAR HostCommandFlags;
} KPH_HW_CROSEC_IDENTITY, *PKPH_HW_CROSEC_IDENTITY;

#define KPH_HW_EC_GLOBAL_LOCK 0x1
#define KPH_HW_EC_BANKED 0x2

typedef struct _KPH_HW_EC_IDENTITY
{
    ULONG DataPort;
    ULONG CommandPort;
    ULONG Flags; // KPH_HW_EC_*
} KPH_HW_EC_IDENTITY, *PKPH_HW_EC_IDENTITY;

typedef struct _KPH_HW_EC_REGISTER
{
    ULONG Bank;
    ULONG Register;
} KPH_HW_EC_REGISTER, *PKPH_HW_EC_REGISTER;
typedef const KPH_HW_EC_REGISTER* PCKPH_HW_EC_REGISTER;

typedef enum _KPH_HW_EC_KIND
{
    KphHwEcTemperature = 1,
    KphHwEcVoltage = 2,
    KphHwEcCurrent = 3,
    KphHwEcFan = 4,
    KphHwEcPower = 5
} KPH_HW_EC_KIND;

#define KPH_HW_EC_LITTLE_ENDIAN 0x10
#define KPH_HW_EC_SHIFT_MASK 0x0F

#define KPH_HW_EC_LABEL_LENGTH 32

typedef struct _KPH_HW_EC_SENSOR
{
    UCHAR Kind;
    UCHAR Size;
    UCHAR Bank;
    UCHAR Register;
    UCHAR Form;
    CHAR Label[KPH_HW_EC_LABEL_LENGTH];
} KPH_HW_EC_SENSOR, *PKPH_HW_EC_SENSOR;

#define KPH_HW_ALIENWARE_RESOURCES 16
#define KPH_HW_ALIENWARE_TEMPERATURE 1
#define KPH_HW_ALIENWARE_FAN 2

typedef struct _KPH_HW_ALIENWARE_IDENTITY
{
    UCHAR FanCount;
    UCHAR TemperatureCount;
    UCHAR Fans[KPH_HW_ALIENWARE_RESOURCES];
    UCHAR Temperatures[KPH_HW_ALIENWARE_RESOURCES];
} KPH_HW_ALIENWARE_IDENTITY, *PKPH_HW_ALIENWARE_IDENTITY;

typedef struct _KPH_HW_ALIENWARE_SENSOR
{
    UCHAR Kind;
    UCHAR Id;
} KPH_HW_ALIENWARE_SENSOR, *PKPH_HW_ALIENWARE_SENSOR;
typedef const KPH_HW_ALIENWARE_SENSOR* PCKPH_HW_ALIENWARE_SENSOR;

#define KPH_HW_HP_SENSORS 32
#define KPH_HW_HP_NAME_LENGTH 32

typedef enum _KPH_HW_HP_KIND
{
    KphHwHpTemperature = 1,
    KphHwHpVoltage = 2,
    KphHwHpCurrent = 3,
    KphHwHpFan = 4,
} KPH_HW_HP_KIND;

typedef struct _KPH_HW_HP_SENSOR
{
    UCHAR Kind; // KPH_HW_HP_KIND
    UCHAR Instance;
    CHAR Name[KPH_HW_HP_NAME_LENGTH];
} KPH_HW_HP_SENSOR, *PKPH_HW_HP_SENSOR;

typedef struct _KPH_HW_HP_ITEM
{
    UCHAR Kind; // KPH_HW_HP_KIND
    UCHAR Instance;
} KPH_HW_HP_ITEM, *PKPH_HW_HP_ITEM;
typedef const KPH_HW_HP_ITEM* PCKPH_HW_HP_ITEM;

typedef struct _KPH_HW_VSEC
{
    USHORT Vendor;
    USHORT Device;
    ULONG Operations;
} KPH_HW_VSEC, *PKPH_HW_VSEC;

typedef struct _KPH_HW_VSEC_REGION
{
    UCHAR Operation;
    UCHAR Discovery;
    USHORT Index;
    ULONG Id;
    ULONG Size;
    ULONG Position;
} KPH_HW_VSEC_REGION, *PKPH_HW_VSEC_REGION;

#define KPH_HW_TSENS_CONTROLLERS 8
#define KPH_HW_TSENS_SENSORS 16
#define KPH_HW_TSENS_NO_PROCESSOR 0xFF

typedef enum _KPH_HW_TSENS_ROLE
{
    KphHwTsensRoleNone,
    KphHwTsensRoleCpuTop,
    KphHwTsensRoleCpuBottom,
    KphHwTsensRoleClusterTop,
    KphHwTsensRoleClusterBottom,
    KphHwTsensRoleAoss,
    KphHwTsensRoleMemory,
    KphHwTsensRoleVideo,
    KphHwTsensRoleNpu,
    KphHwTsensRoleGpu,
    KphHwTsensRoleCamera,
    KphHwTsensRoleMaximum
} KPH_HW_TSENS_ROLE;

typedef struct _KPH_HW_TSENS_SENSOR
{
    UCHAR Role;
    UCHAR Index;
    UCHAR Group;
    UCHAR Number;
} KPH_HW_TSENS_SENSOR, *PKPH_HW_TSENS_SENSOR;

typedef struct _KPH_HW_TSENS_CONTROLLER
{
    ULONG Version;
    ULONG Control;
    KPH_HW_TSENS_SENSOR Sensors[KPH_HW_TSENS_SENSORS];
} KPH_HW_TSENS_CONTROLLER, *PKPH_HW_TSENS_CONTROLLER;

typedef struct _KPH_HW_TSENS_IDENTITY
{
    ULONG Count;
    KPH_HW_TSENS_CONTROLLER Controllers[KPH_HW_TSENS_CONTROLLERS];
} KPH_HW_TSENS_IDENTITY, *PKPH_HW_TSENS_IDENTITY;

typedef enum _KPH_HW_SPBM_TEMPERATURE
{
    KphHwSpbmTemperatureHottest,
    KphHwSpbmTemperatureSoc,
    KphHwSpbmTemperatureCluster0E,
    KphHwSpbmTemperatureCluster0P,
    KphHwSpbmTemperatureCluster1E,
    KphHwSpbmTemperatureCluster1P,
    KphHwSpbmTemperatureGpu,
    KphHwSpbmTemperatureUncore,
    KphHwSpbmTemperatureDla,
    KphHwSpbmTemperatureMaximum
} KPH_HW_SPBM_TEMPERATURE;

typedef enum _KPH_HW_SPBM_POWER
{
    KphHwSpbmPowerSystem,
    KphHwSpbmPowerPackage,
    KphHwSpbmPowerCpuGpu,
    KphHwSpbmPowerCpuP,
    KphHwSpbmPowerCpuE,
    KphHwSpbmPowerGpu,
    KphHwSpbmPowerMemoryA,
    KphHwSpbmPowerMemoryB,
    KphHwSpbmPowerMemoryC,
    KphHwSpbmPowerDcInput,
    KphHwSpbmPowerPackageAverage,
    KphHwSpbmPowerSystemAverage,
    KphHwSpbmPowerSystemShortAverage,
    KphHwSpbmPowerMaximum
} KPH_HW_SPBM_POWER;

typedef enum _KPH_HW_SPBM_ENERGY
{
    KphHwSpbmEnergyPackage,
    KphHwSpbmEnergyCpuE,
    KphHwSpbmEnergyCpuP,
    KphHwSpbmEnergyGpu,
    KphHwSpbmEnergySystem,
    KphHwSpbmEnergyMaximum
} KPH_HW_SPBM_ENERGY;

typedef enum _KPH_HW_SPBM_LIMIT
{
    KphHwSpbmLimitPl1,
    KphHwSpbmLimitPl2,
    KphHwSpbmLimitPl1Request,
    KphHwSpbmLimitPl2Request,
    KphHwSpbmLimitInForce, // 0 when unknown
    KphHwSpbmLimitMaximum
} KPH_HW_SPBM_LIMIT;

typedef enum _KPH_HW_SPBM_VOLTAGE
{
    KphHwSpbmVoltageCpuP,
    KphHwSpbmVoltageCpuE,
    KphHwSpbmVoltageMemoryC,
    KphHwSpbmVoltageMaximum
} KPH_HW_SPBM_VOLTAGE;

typedef struct _KPH_HW_SPBM_SAMPLE
{
    ULONG TemperatureMask;
    ULONG PowerMask;
    ULONG EnergyMask;
    ULONG LimitMask;
    ULONG VoltageMask;
    LONG Temperature[KphHwSpbmTemperatureMaximum]; // milli-degrees Celsius
    ULONG Power[KphHwSpbmPowerMaximum]; // milliwatts
    ULONG Energy[KphHwSpbmEnergyMaximum]; // millijoules, wrapping
    ULONG Limit[KphHwSpbmLimitMaximum]; // milliwatts
    ULONG Voltage[KphHwSpbmVoltageMaximum]; // millivolts
} KPH_HW_SPBM_SAMPLE, *PKPH_HW_SPBM_SAMPLE;

#define KPH_HW_VSEC_DISCOVERY_PMT 2
#define KPH_HW_VSEC_DISCOVERY_TPMI 66

#define KPH_HW_WINDOW_MCHBAR 1
#define KPH_HW_WINDOW_BAR0 2

#define KPH_HW_PLATFORM_LPIT 1
#define KPH_HW_PLATFORM_PWRM 2

#define KPH_HW_ACPI_ROW_PNP0C0B 1
#define KPH_HW_ACPI_ROW_INT3403 2
#define KPH_HW_ACPI_ROW_ATK0110 3
#define KPH_HW_ACPI_ROW_LEN0068 4
#define KPH_HW_ACPI_ROW_PNP0C09 5
#define KPH_HW_ACPI_ROW_ACPI000D 6
#define KPH_HW_ACPI_ROW_INT3407 7
#define KPH_HW_ACPI_ROW_17761776 8
#define KPH_HW_ACPI_ROW_QCOM0C58 9

C_ASSERT(sizeof(KPH_HW_CPU_IDENTITY) == 32);
C_ASSERT(sizeof(KPH_HW_CPU_ITEM) == 8);
C_ASSERT(sizeof(KPH_HW_VALUE64) == 16);
C_ASSERT(sizeof(KPH_HW_SUPERIO_CHIP) == 12);
C_ASSERT(sizeof(KPH_HW_VALUE32) == 8);
C_ASSERT(sizeof(KPH_HW_SMN_IDENTITY) == 16);
C_ASSERT(sizeof(KPH_HW_SMU_IDENTITY) == 16);
C_ASSERT(sizeof(KPH_HW_SMU_TABLE) == 8);
C_ASSERT(sizeof(KPH_HW_SMU_COMMAND) == 28);
C_ASSERT(sizeof(KPH_HW_SMU_RESULT) == 32);
C_ASSERT(sizeof(KPH_HW_HSMP_IDENTITY) == 8);
C_ASSERT(sizeof(KPH_HW_HSMP_COMMAND) == 36);
C_ASSERT(sizeof(KPH_HW_HSMP_RESULT) == 40);
C_ASSERT(sizeof(KPH_HW_ACPI_DEVICE) == 144);
C_ASSERT(sizeof(KPH_HW_SMBUS_HOST) == 16);
C_ASSERT(sizeof(KPH_HW_SMBUS_ITEM) == 4);
C_ASSERT(sizeof(KPH_HW_PMBUS_IDENTITY) == 100);
C_ASSERT(sizeof(KPH_HW_PCI_MMIO) == 8);
C_ASSERT(sizeof(KPH_HW_WINDOW_ITEM) == 8);
C_ASSERT(sizeof(KPH_HW_PCI_CONFIG) == 8);
C_ASSERT(sizeof(KPH_HW_CROSEC_IDENTITY) == 8);
C_ASSERT(sizeof(KPH_HW_EC_IDENTITY) == 12);
C_ASSERT(sizeof(KPH_HW_EC_REGISTER) == 8);
C_ASSERT(sizeof(KPH_HW_EC_SENSOR) == 37);
C_ASSERT(sizeof(KPH_HW_ALIENWARE_IDENTITY) == 34);
C_ASSERT(sizeof(KPH_HW_ALIENWARE_SENSOR) == 2);
C_ASSERT(sizeof(KPH_HW_HP_SENSOR) == 34);
C_ASSERT(sizeof(KPH_HW_HP_ITEM) == 2);
C_ASSERT(sizeof(KPH_HW_VSEC) == 8);
C_ASSERT(sizeof(KPH_HW_VSEC_REGION) == 16);
C_ASSERT(sizeof(KPH_HW_TSENS_SENSOR) == 4);
C_ASSERT(sizeof(KPH_HW_TSENS_CONTROLLER) == 72);
C_ASSERT(sizeof(KPH_HW_TSENS_IDENTITY) == 580);
C_ASSERT(sizeof(KPH_HW_SPBM_SAMPLE) == 160);

PHLIBAPI
NTSTATUS
NTAPI
KphHwCpuIdentify(
    _In_ USHORT Group,
    _In_ USHORT Number,
    _Out_ PKPH_HW_CPU_IDENTITY Identity
    );

PHLIBAPI
NTSTATUS
NTAPI
KphHwCpuRead(
    _In_reads_(Count) PCKPH_HW_CPU_ITEM Items,
    _In_ ULONG Count,
    _Out_writes_(Count) PKPH_HW_VALUE64 Values
    );

PHLIBAPI
NTSTATUS
NTAPI
KphHwSuperIoIdentify(
    _In_ ULONG Slot,
    _Out_ PKPH_HW_SUPERIO_CHIP Chip
    );

PHLIBAPI
VOID
NTAPI
KphHwSetScriptMode(
    _In_ ULONG Flags
    );

PHLIBAPI
NTSTATUS
NTAPI
KphHwSuperIoRead(
    _In_ ULONG Slot,
    _In_reads_(Count) PCULONG Registers,
    _In_ ULONG Count,
    _Out_writes_(Count) PKPH_HW_VALUE32 Values
    );

PHLIBAPI
NTSTATUS
NTAPI
KphHwSmnIdentify(
    _In_ ULONG Locator,
    _Out_ PKPH_HW_SMN_IDENTITY Identity
    );

PHLIBAPI
NTSTATUS
NTAPI
KphHwSmnRead(
    _In_ ULONG Locator,
    _In_reads_(Count) PCULONG Addresses,
    _In_ ULONG Count,
    _Out_writes_(Count) PKPH_HW_VALUE32 Values
    );

PHLIBAPI
NTSTATUS
NTAPI
KphHwSmuIdentify(
    _In_ ULONG Locator,
    _Out_ PKPH_HW_SMU_IDENTITY Identity
    );

PHLIBAPI
NTSTATUS
NTAPI
KphHwSmuReadTable(
    _In_ ULONG Locator,
    _Out_ PKPH_HW_SMU_TABLE Header,
    _Out_writes_bytes_to_(Length, *ReturnLength) PVOID Table,
    _In_ ULONG Length,
    _Out_ PULONG ReturnLength
    );

PHLIBAPI
NTSTATUS
NTAPI
KphHwSmuCommand(
    _In_ ULONG Locator,
    _In_reads_(Count) PCKPH_HW_SMU_COMMAND Commands,
    _In_ ULONG Count,
    _Out_writes_(Count) PKPH_HW_SMU_RESULT Results
    );

PHLIBAPI
NTSTATUS
NTAPI
KphHwHsmpIdentify(
    _In_ ULONG Locator,
    _In_ ULONG Socket,
    _Out_ PKPH_HW_HSMP_IDENTITY Identity
    );

PHLIBAPI
NTSTATUS
NTAPI
KphHwHsmpCommand(
    _In_ ULONG Locator,
    _In_ ULONG Socket,
    _In_reads_(Count) PCKPH_HW_HSMP_COMMAND Commands,
    _In_ ULONG Count,
    _Out_writes_(Count) PKPH_HW_HSMP_RESULT Results
    );

#define KPH_HW_HSMP_METRICS_SIZE 3648

PHLIBAPI
NTSTATUS
NTAPI
KphHwHsmpReadMetrics(
    _In_ ULONG Socket,
    _Out_writes_bytes_(KPH_HW_HSMP_METRICS_SIZE) PVOID Table
    );

PHLIBAPI
NTSTATUS
NTAPI
KphHwSmbusIdentify(
    _In_ ULONG Locator,
    _Out_ PKPH_HW_SMBUS_HOST Host
    );

PHLIBAPI
NTSTATUS
NTAPI
KphHwSmbusRead(
    _In_ ULONG Locator,
    _In_ ULONG Port,
    _In_reads_(Count) PCKPH_HW_SMBUS_ITEM Items,
    _In_ ULONG Count,
    _Out_writes_(Count) PKPH_HW_VALUE32 Values
    );

PHLIBAPI
NTSTATUS
NTAPI
KphHwPciMmioIdentify(
    _In_ ULONG Locator,
    _Out_ PKPH_HW_PCI_MMIO Device
    );

PHLIBAPI
NTSTATUS
NTAPI
KphHwPciMmioRead(
    _In_ ULONG Locator,
    _In_ ULONG Window,
    _In_reads_(Count) PCKPH_HW_WINDOW_ITEM Items,
    _In_ ULONG Count,
    _Out_writes_(Count) PKPH_HW_VALUE64 Values
    );

PHLIBAPI
NTSTATUS
NTAPI
KphHwPciConfigIdentify(
    _In_ ULONG Locator,
    _Out_ PKPH_HW_PCI_CONFIG Device
    );

PHLIBAPI
NTSTATUS
NTAPI
KphHwPciConfigRead(
    _In_ ULONG Locator,
    _In_reads_(Count) PCKPH_HW_WINDOW_ITEM Items,
    _In_ ULONG Count,
    _Out_writes_(Count) PKPH_HW_VALUE32 Values
    );

PHLIBAPI
NTSTATUS
NTAPI
KphHwPciConfigReadBlock(
    _In_ ULONG Locator,
    _In_ ULONG Offset,
    _Out_writes_bytes_(Length) PVOID Buffer,
    _In_ ULONG Length
    );

PHLIBAPI
NTSTATUS
NTAPI
KphHwCrosEcIdentify(
    _Out_ PKPH_HW_CROSEC_IDENTITY Identity
    );

PHLIBAPI
NTSTATUS
NTAPI
KphHwCrosEcRead(
    _In_reads_(Count) PCULONG Offsets,
    _In_ ULONG Count,
    _Out_writes_(Count) PKPH_HW_VALUE32 Values
    );

PHLIBAPI
NTSTATUS
NTAPI
KphHwEcIdentify(
    _In_ ULONG Ordinal,
    _Out_ PKPH_HW_EC_IDENTITY Identity
    );

PHLIBAPI
NTSTATUS
NTAPI
KphHwEcSensors(
    _Out_writes_to_(Capacity, *Count) PKPH_HW_EC_SENSOR Sensors,
    _In_ ULONG Capacity,
    _Out_ PULONG Count
    );

PHLIBAPI
NTSTATUS
NTAPI
KphHwAlienwareIdentify(
    _Out_ PKPH_HW_ALIENWARE_IDENTITY Identity
    );

PHLIBAPI
NTSTATUS
NTAPI
KphHwAlienwareRead(
    _In_reads_(Count) PCKPH_HW_ALIENWARE_SENSOR Sensors,
    _In_ ULONG Count,
    _Out_writes_(Count) PKPH_HW_VALUE32 Values
    );

PHLIBAPI
NTSTATUS
NTAPI
KphHwHpIdentify(
    _Out_writes_to_(Capacity, *Count) PKPH_HW_HP_SENSOR Sensors,
    _In_ ULONG Capacity,
    _Out_ PULONG Count
    );

PHLIBAPI
NTSTATUS
NTAPI
KphHwHpRead(
    _In_reads_(Count) PCKPH_HW_HP_ITEM Items,
    _In_ ULONG Count,
    _Out_writes_(Count) PKPH_HW_VALUE32 Values
    );

PHLIBAPI
NTSTATUS
NTAPI
KphHwPmbusIdentify(
    _In_ ULONG Locator,
    _In_ ULONG Port,
    _In_ UCHAR Address,
    _Out_ PKPH_HW_PMBUS_IDENTITY Identity
    );

#define KPH_HW_PROM21_DIES 8

PHLIBAPI
NTSTATUS
NTAPI
KphHwProm21Read(
    _In_reads_(Count) const ULONG* Locators,
    _In_ ULONG Count,
    _Out_writes_(Count) PKPH_HW_VALUE32 Values
    );

#define KPH_HW_USB_DISKS 16
#define KPH_HW_USB_DISK_ID 200
#define KPH_HW_USB_DISK_ATA 1
#define KPH_HW_USB_DISK_NVME 2

typedef struct _KPH_HW_USB_DISK
{
    NTSTATUS Status;
    USHORT VendorId;
    USHORT ProductId;
    UCHAR PowerCount;
    UCHAR Kind; // KPH_HW_USB_DISK_*
    UCHAR Reserved[2];
    WCHAR InstanceId[KPH_HW_USB_DISK_ID]; // the disk unit's
    UCHAR Data[512];
} KPH_HW_USB_DISK, *PKPH_HW_USB_DISK;

C_ASSERT(sizeof(KPH_HW_USB_DISK) == 924);

PHLIBAPI
NTSTATUS
NTAPI
KphHwUsbDiskRead(
    _Out_writes_to_(KPH_HW_USB_DISKS, *Count) PKPH_HW_USB_DISK Disks,
    _Out_ PULONG Count
    );

#define KPH_HW_HID_DEVICES 8
#define KPH_HW_HID_VALUES 24
#define KPH_HW_HID_TEMPERATURE 1
#define KPH_HW_HID_FAN 2
#define KPH_HW_HID_FLOW 3
#define KPH_HW_HID_VOLTAGE 4
#define KPH_HW_HID_CURRENT 5
#define KPH_HW_HID_POWER 6

typedef struct _KPH_HW_HID_VALUE
{
    UCHAR Kind; // KPH_HW_HID_*
    UCHAR Index;
    UCHAR Reserved[2];
    LONG Value;
} KPH_HW_HID_VALUE, *PKPH_HW_HID_VALUE;

typedef struct _KPH_HW_HID_DEVICE
{
    NTSTATUS Status;
    USHORT VendorId;
    USHORT ProductId;
    UCHAR Count;
    UCHAR Reserved[3];
    KPH_HW_HID_VALUE Values[KPH_HW_HID_VALUES];
} KPH_HW_HID_DEVICE, *PKPH_HW_HID_DEVICE;

C_ASSERT(sizeof(KPH_HW_HID_DEVICE) == 204);

PHLIBAPI
NTSTATUS
NTAPI
KphHwHidRead(
    _Out_writes_to_(KPH_HW_HID_DEVICES, *Count) PKPH_HW_HID_DEVICE Devices,
    _Out_ PULONG Count
    );

PHLIBAPI
NTSTATUS
NTAPI
KphHwCorsairRead(
    _Out_writes_to_(KPH_HW_HID_DEVICES, *Count) PKPH_HW_HID_DEVICE Devices,
    _Out_ PULONG Count
    );

PHLIBAPI
NTSTATUS
NTAPI
KphHwAxiRead(
    _Out_writes_to_(KPH_HW_HID_DEVICES, *Count) PKPH_HW_HID_DEVICE Devices,
    _Out_ PULONG Count
    );

#define KPH_HW_AQTION_CONTROLLERS 8

PHLIBAPI
NTSTATUS
NTAPI
KphHwAqtionRead(
    _In_reads_(Count) const ULONG* Locators,
    _In_ ULONG Count,
    _Out_writes_(Count) PKPH_HW_VALUE32 Values
    );

#define KPH_HW_GIGABYTE_SENSORS 6

PHLIBAPI
NTSTATUS
NTAPI
KphHwGigabyteRead(
    _Out_writes_(KPH_HW_GIGABYTE_SENSORS) PKPH_HW_VALUE32 Values
    );

PHLIBAPI
NTSTATUS
NTAPI
KphHwEcRead(
    _In_ ULONG Ordinal,
    _In_reads_(Count) PCKPH_HW_EC_REGISTER Registers,
    _In_ ULONG Count,
    _Out_writes_(Count) PKPH_HW_VALUE32 Values
    );

PHLIBAPI
NTSTATUS
NTAPI
KphHwGpdReadFan(
    _Out_ PULONG Rpm
    );

typedef enum _KPH_HW_OC_PLANES
{
    KphHwOcPlaneCore,
    KphHwOcPlaneGpu,
    KphHwOcPlaneCache,
    KphHwOcPlaneSystemAgent,
    KphHwOcPlaneAnalogIo,
    MaxKphHwOcPlane,
} KPH_HW_OC_PLANES_ENUM;

typedef struct _KPH_HW_OC_PLANE
{
    NTSTATUS Status;
    ULONG Code;
    ULONG Data;
} KPH_HW_OC_PLANE, *PKPH_HW_OC_PLANE;

C_ASSERT(sizeof(KPH_HW_OC_PLANE) == 12);

PHLIBAPI
NTSTATUS
NTAPI
KphHwOcMailboxRead(
    _Out_writes_(MaxKphHwOcPlane) PKPH_HW_OC_PLANE Planes
    );

#define KPH_HW_ASUSWMI_SENSORS 64
#define KPH_HW_ASUSWMI_VOLTAGE 0
#define KPH_HW_ASUSWMI_TEMPERATURE 1
#define KPH_HW_ASUSWMI_FAN 2
#define KPH_HW_ASUSWMI_CURRENT 3
#define KPH_HW_ASUSWMI_WATER_FLOW 4
#define KPH_HW_ASUSWMI_UNSIGNED 1

typedef struct _KPH_HW_ASUSWMI_SENSOR
{
    CHAR Name[32];
    UCHAR DataType; // KPH_HW_ASUSWMI_*
    UCHAR Location;
    UCHAR Source;
    UCHAR Type;
} KPH_HW_ASUSWMI_SENSOR, *PKPH_HW_ASUSWMI_SENSOR;

typedef struct _KPH_HW_ASUSWMI_IDENTITY
{
    ULONG Version;
    ULONG Count;
    KPH_HW_ASUSWMI_SENSOR Sensors[KPH_HW_ASUSWMI_SENSORS];
} KPH_HW_ASUSWMI_IDENTITY, *PKPH_HW_ASUSWMI_IDENTITY;

C_ASSERT(sizeof(KPH_HW_ASUSWMI_SENSOR) == 36);

PHLIBAPI
NTSTATUS
NTAPI
KphHwAsusWmiIdentify(
    _Out_ PKPH_HW_ASUSWMI_IDENTITY Identity
    );

PHLIBAPI
NTSTATUS
NTAPI
KphHwAsusWmiRead(
    _In_reads_(Count) const UCHAR* Sources,
    _In_ ULONG Count,
    _Out_writes_(Count) PKPH_HW_VALUE32 Values
    );

#define KPH_HW_DELL_FANS 4
#define KPH_HW_DELL_TEMPS 10

typedef struct _KPH_HW_DELL_IDENTITY
{
    ULONG Multiplier; // RPM per unit of a fan speed
    UCHAR FanTypes[KPH_HW_DELL_FANS]; // FFh none, FEh present of no known type
    UCHAR TempTypes[KPH_HW_DELL_TEMPS]; // FFh none
    UCHAR Reserved[2];
} KPH_HW_DELL_IDENTITY, *PKPH_HW_DELL_IDENTITY;

C_ASSERT(sizeof(KPH_HW_DELL_IDENTITY) == 20);

PHLIBAPI
NTSTATUS
NTAPI
KphHwDellIdentify(
    _Out_ PKPH_HW_DELL_IDENTITY Identity
    );

PHLIBAPI
NTSTATUS
NTAPI
KphHwDellRead(
    _In_ ULONG FanMask,
    _In_ ULONG TempMask,
    _Out_writes_(KPH_HW_DELL_FANS + KPH_HW_DELL_TEMPS) PLONG Values
    );

PHLIBAPI
NTSTATUS
NTAPI
KphHwLenovoWsRead(
    _Out_writes_bytes_to_(Length, *ReturnLength) PVOID Buffer,
    _In_ ULONG Length,
    _Out_ PULONG ReturnLength
    );

PHLIBAPI
NTSTATUS
NTAPI
KphHwVsecIdentify(
    _In_ ULONG Locator,
    _Out_ PKPH_HW_VSEC Device
    );

PHLIBAPI
NTSTATUS
NTAPI
KphHwVsecRegions(
    _In_ ULONG Locator,
    _Out_writes_to_opt_(Count, *ReturnCount) PKPH_HW_VSEC_REGION Regions,
    _In_ ULONG Count,
    _Out_ PULONG ReturnCount
    );

PHLIBAPI
NTSTATUS
NTAPI
KphHwVsecRead(
    _In_ ULONG Locator,
    _In_ ULONG Operation,
    _In_ ULONG Position,
    _In_ ULONG Id,
    _In_reads_(Count) PCKPH_HW_WINDOW_ITEM Items,
    _In_ ULONG Count,
    _Out_writes_(Count) PKPH_HW_VALUE64 Values
    );

PHLIBAPI
NTSTATUS
NTAPI
KphHwPlatformRead(
    _In_ ULONG Window,
    _In_reads_(Count) PCKPH_HW_WINDOW_ITEM Items,
    _In_ ULONG Count,
    _Out_writes_(Count) PKPH_HW_VALUE64 Values
    );

PHLIBAPI
NTSTATUS
NTAPI
KphHwAcpiIdentify(
    _Out_writes_to_opt_(Count, *ReturnCount) PKPH_HW_ACPI_DEVICE Devices,
    _In_ ULONG Count,
    _Out_ PULONG ReturnCount
    );

PHLIBAPI
NTSTATUS
NTAPI
KphHwAcpiEval(
    _In_ ULONG Milliseconds,
    _In_reads_bytes_(EvalsLength) PCVOID Evals,
    _In_ ULONG EvalsLength,
    _Out_writes_bytes_to_(Length, *ReturnLength) PVOID Output,
    _In_ ULONG Length,
    _Out_ PULONG ReturnLength
    );

PHLIBAPI
NTSTATUS
NTAPI
KphHwTsensIdentify(
    _Out_ PKPH_HW_TSENS_IDENTITY Identity
    );

PHLIBAPI
NTSTATUS
NTAPI
KphHwTsensRead(
    _In_ ULONG Count,
    _Out_writes_(Count * KPH_HW_TSENS_SENSORS) PULONG Status
    );

PHLIBAPI
NTSTATUS
NTAPI
KphHwSpbmRead(
    _Out_ PKPH_HW_SPBM_SAMPLE Sample
    );

PHLIBAPI
VOID
NTAPI
KphHwAcpiAppendEval(
    _Inout_ PPH_BYTES_BUILDER Evals,
    _In_ ULONG Row,
    _In_ ULONG Ordinal,
    _In_ PCPH_BYTESREF Name,
    _In_opt_ PCPH_BYTESREF Arguments
    );

EXTERN_C_END

#endif
