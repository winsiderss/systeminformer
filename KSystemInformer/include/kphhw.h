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

#pragma once
#include <kph.h>
#include <kphmsgdefs.h>

#if defined(_M_X64)
typedef union _KPH_CPUID
{
    struct
    {
        ULONG EAX;
        ULONG EBX;
        ULONG ECX;
        ULONG EDX;
    };

    INT32 Data[4];
} KPH_CPUID, *PKPH_CPUID;

typedef union _KPH_CPUID_1
{
    struct
    {
        union
        {
            struct
            {
                ULONG Stepping : 4;
                ULONG Model : 4;
                ULONG Family : 4;
                ULONG ProcessorType : 2;
                ULONG Reserved1 : 2;
                ULONG ExtendedModel : 4;
                ULONG ExtendedFamily : 8;
                ULONG Reserved2 : 4;
            };

            ULONG Signature;
        };

        ULONG AdditionalInformation;
        ULONG FeatureInformation[2];
    };

    KPH_CPUID DUMMYSTRUCTNAME;
} KPH_CPUID_1, *PKPH_CPUID_1;

#define KphCpuId(Info, Function, SubFunction)                                  \
    __cpuidex((Info)->Data, Function, SubFunction)
#endif

typedef struct _KPH_HW_CPU_IDENTITY
{
    ULONG Vendor[3];
    USHORT DisplayFamily;
    UCHAR DisplayModel;
    UCHAR Stepping;
    ULONG MaxBasicLeaf;
    ULONG MaxExtendedLeaf;
    UCHAR PhysicalAddressBits;
} KPH_HW_CPU_IDENTITY, *PKPH_HW_CPU_IDENTITY;
typedef const KPH_HW_CPU_IDENTITY* PCKPH_HW_CPU_IDENTITY;

#define KPH_HW_ACPI_HEADER_LENGTH 36

#include <pshpack1.h>

typedef struct _KPH_HW_ACPI_GAS
{
    UCHAR SpaceId;
    UCHAR BitWidth;
    UCHAR BitOffset;
    UCHAR AccessSize;
    ULONG64 Address;
} KPH_HW_ACPI_GAS, *PKPH_HW_ACPI_GAS;

#include <poppack.h>

C_ASSERT(sizeof(KPH_HW_ACPI_GAS) == 12);

_IRQL_requires_max_(PASSIVE_LEVEL)
VOID KphInitializeHardware(
    VOID
    );

_IRQL_requires_max_(PASSIVE_LEVEL)
VOID KphCleanupHardware(
    VOID
    );

_IRQL_requires_max_(PASSIVE_LEVEL)
PCKPH_HW_CPU_IDENTITY KphGetHardwareCpuIdentity(
    VOID
    );

_IRQL_requires_max_(PASSIVE_LEVEL)
PCKPH_DYN_HW_BOARD_IDENTITY KphGetHardwareBoardIdentity(
    VOID
    );

_IRQL_requires_max_(PASSIVE_LEVEL)
UCHAR KphGetHardwareChassisType(
    VOID
    );

_IRQL_requires_max_(PASSIVE_LEVEL)
_Must_inspect_result_
NTSTATUS KphHwQueryCapabilities(
    _Out_ PKPH_HW_CAPABILITIES Capabilities,
    _In_ KPROCESSOR_MODE AccessMode
    );

_IRQL_requires_max_(PASSIVE_LEVEL)
_Must_inspect_result_
NTSTATUS KphHwReferenceDriver(
    _In_ PCUNICODE_STRING Name,
    _Outptr_ PDRIVER_OBJECT* Driver
    );

typedef
_Function_class_(KPH_HW_ENUMERATE_DEVICES_CALLBACK)
_Must_inspect_result_
BOOLEAN
KSIAPI
KPH_HW_ENUMERATE_DEVICES_CALLBACK(
    _In_ PDEVICE_OBJECT DeviceObject,
    _In_opt_ PVOID Context
    );
typedef KPH_HW_ENUMERATE_DEVICES_CALLBACK* PKPH_HW_ENUMERATE_DEVICES_CALLBACK;

_IRQL_requires_max_(PASSIVE_LEVEL)
_Must_inspect_result_
NTSTATUS KphHwEnumerateDriverDevices(
    _In_ PDRIVER_OBJECT Driver,
    _In_ PKPH_HW_ENUMERATE_DEVICES_CALLBACK Callback,
    _In_opt_ PVOID Context
    );

_IRQL_requires_max_(PASSIVE_LEVEL)
_Must_inspect_result_
BOOLEAN KphHwExpectsNoRemoval(
    _In_ PDEVICE_OBJECT Pdo
    );

_IRQL_requires_max_(PASSIVE_LEVEL)
_Must_inspect_result_
NTSTATUS KphHwQueryBootConfiguration(
    _In_ PDEVICE_OBJECT Pdo,
    _In_ DEVICE_REGISTRY_PROPERTY Property,
    _Outptr_result_bytebuffer_(*Length) PCM_RESOURCE_LIST* List,
    _Out_ PULONG Length
    );

typedef enum _KPH_HW_FIND_MEMORY
{
    KPH_HW_FIND_MEMORY_BY_START,
    KPH_HW_FIND_MEMORY_BY_ORDINAL,
    KPH_HW_FIND_MEMORY_BY_MEMORY_ORDINAL
} KPH_HW_FIND_MEMORY;

_IRQL_requires_max_(PASSIVE_LEVEL)
_Must_inspect_result_
_Success_(return)
BOOLEAN KphHwFindMemoryDescriptor(
    _In_reads_bytes_(ListLength) PCM_RESOURCE_LIST List,
    _In_ ULONG ListLength,
    _In_ KPH_HW_FIND_MEMORY Mode,
    _Inout_ PULONG Ordinal,
    _Inout_ PULONG64 Start,
    _Out_ PULONG64 Length,
    _Out_opt_ PUSHORT Flags
    );

#if defined(_M_X64)
_IRQL_requires_max_(PASSIVE_LEVEL)
_Must_inspect_result_
BOOLEAN KphHwCpuidGatePasses(
    _In_ PCKPH_HW_CPU_IDENTITY Cpu,
    _In_ PCKPH_DYN_HW_GATE Gate
    );
#endif

_IRQL_requires_max_(PASSIVE_LEVEL)
_Must_inspect_result_
BOOLEAN KphHwIsActiveProcessor(
    _In_ const PROCESSOR_NUMBER* Processor
    );

_IRQL_requires_max_(PASSIVE_LEVEL)
VOID KphHwPinProcessor(
    _In_ const PROCESSOR_NUMBER* Processor,
    _Out_ PGROUP_AFFINITY Previous
    );

_IRQL_requires_max_(PASSIVE_LEVEL)
VOID KphHwUnpinProcessor(
    _In_ PGROUP_AFFINITY Previous
    );

_IRQL_requires_max_(PASSIVE_LEVEL)
_Must_inspect_result_
NTSTATUS KphHwCaptureDeadline(
    _In_opt_ PLARGE_INTEGER Timeout,
    _Out_ PULONG64 Deadline,
    _In_ KPROCESSOR_MODE AccessMode
    );

_IRQL_requires_max_(PASSIVE_LEVEL)
BOOLEAN KphHwDeadlinePassed(
    _In_ ULONG64 Deadline
    );

// hwmsr

_IRQL_requires_max_(PASSIVE_LEVEL)
_Must_inspect_result_
NTSTATUS KphHwReadMsrBatch(
    _In_ ULONG Count,
    _Inout_updates_(Count) PKPH_HW_MSR_BATCH_ENTRY Entries,
    _In_ KPROCESSOR_MODE AccessMode
    );

// hwsysreg

_IRQL_requires_max_(PASSIVE_LEVEL)
_Must_inspect_result_
NTSTATUS KphHwReadSysregBatch(
    _In_ ULONG Count,
    _Inout_updates_(Count) PKPH_HW_SYSREG_BATCH_ENTRY Entries,
    _In_ KPROCESSOR_MODE AccessMode
    );

// hwpci

_IRQL_requires_max_(PASSIVE_LEVEL)
VOID KphInitializeHardwarePci(
    VOID
    );

_IRQL_requires_max_(PASSIVE_LEVEL)
VOID KphCleanupHardwarePci(
    VOID
    );

_IRQL_requires_max_(PASSIVE_LEVEL)
_Must_inspect_result_
BOOLEAN KphHwPciAvailable(
    VOID
    );

_IRQL_requires_max_(PASSIVE_LEVEL)
_Must_inspect_result_
NTSTATUS KphHwPciIdPresent(
    _In_ USHORT VendorId,
    _In_ USHORT DeviceId
    );

#if defined(_M_X64)
_IRQL_requires_max_(PASSIVE_LEVEL)
_Must_inspect_result_
NTSTATUS KphHwPciResolvePortIndirect(
    _In_ PKPH_HW_PCI_LOCATOR Locator,
    _In_ PKPH_DYN_HW_SECTION Section,
    _In_ ULONG OpIndex,
    _Outptr_ PKPH_DYN_HW_OP* Op
    );
#endif

_IRQL_requires_max_(PASSIVE_LEVEL)
_Must_inspect_result_
NTSTATUS KphHwReadPciConfig(
    _In_ PKPH_HW_PCI_LOCATOR Locator,
    _In_ ULONG Offset,
    _In_ ULONG Length,
    _Out_writes_bytes_(Length) PVOID Buffer,
    _Out_opt_ PULONG ReturnLength,
    _In_ KPROCESSOR_MODE AccessMode
    );

_IRQL_requires_max_(PASSIVE_LEVEL)
_Must_inspect_result_
NTSTATUS KphHwReadPciIndirect(
    _In_ PKPH_HW_PCI_LOCATOR Locator,
    _In_ ULONG Target,
    _Out_ PULONG Value,
    _In_ KPROCESSOR_MODE AccessMode
    );

typedef struct _KPH_HW_PCI_BUS
{
    PDEVICE_OBJECT Top;
    BUS_INTERFACE_STANDARD Interface;
} KPH_HW_PCI_BUS, *PKPH_HW_PCI_BUS;

#if defined(_M_X64)
typedef struct _KPH_HW_SMN_SESSION
{
    KPH_HW_PCI_LOCATOR Locator;
    USHORT IndexOffset;
    USHORT DataOffset;
    PDEVICE_OBJECT Pdo;
    KPH_HW_PCI_BUS Bus;
} KPH_HW_SMN_SESSION, *PKPH_HW_SMN_SESSION;

_IRQL_requires_max_(PASSIVE_LEVEL)
_Must_inspect_result_
NTSTATUS KphHwPciOpenSmn(
    _In_ PKPH_HW_PCI_LOCATOR Locator,
    _In_ PKPH_DYN_HW_SECTION Section,
    _In_ USHORT Kind,
    _In_ ULONG OpIndex,
    _Outptr_ PKPH_DYN_HW_OP* Op,
    _Outptr_ PKPH_DYN_HW_OP* Mailbox,
    _Out_ PKPH_HW_SMN_SESSION Session
    );

_IRQL_requires_max_(PASSIVE_LEVEL)
VOID KphHwPciCloseSmn(
    _Inout_ PKPH_HW_SMN_SESSION Session
    );

_IRQL_requires_max_(PASSIVE_LEVEL)
_Must_inspect_result_
NTSTATUS KphHwPciSmnRead(
    _In_ PKPH_HW_SMN_SESSION Session,
    _In_ ULONG Address,
    _Out_ PULONG Value
    );

_IRQL_requires_max_(PASSIVE_LEVEL)
_Must_inspect_result_
NTSTATUS KphHwPciSmnWrite(
    _In_ PKPH_HW_SMN_SESSION Session,
    _In_ ULONG Address,
    _In_ ULONG Value
    );
#endif

_IRQL_requires_max_(PASSIVE_LEVEL)
_Must_inspect_result_
NTSTATUS KphHwPciReadWindow(
    _In_ PKPH_HW_PCI_LOCATOR Locator,
    _In_ PKPH_DYN_HW_SECTION Section,
    _In_ ULONG OpIndex,
    _In_ ULONG Count,
    _Inout_updates_(Count) PKPH_HW_WINDOW_BATCH_ENTRY Entries
    );

typedef struct _KPH_HW_VSEC_LOCATION
{
    ULONG64 Base;
    ULONG Size;
    ULONG Id;
} KPH_HW_VSEC_LOCATION, *PKPH_HW_VSEC_LOCATION;

_IRQL_requires_max_(PASSIVE_LEVEL)
_Must_inspect_result_
NTSTATUS KphHwPciReadVsec(
    _In_ PKPH_HW_PCI_LOCATOR Locator,
    _In_ PKPH_DYN_HW_SECTION Section,
    _In_ ULONG OpIndex,
    _In_ ULONG Instance,
    _In_ ULONG Count,
    _Inout_updates_(Count) PKPH_HW_WINDOW_BATCH_ENTRY Entries
    );

_IRQL_requires_max_(PASSIVE_LEVEL)
_Must_inspect_result_
NTSTATUS KphHwPciQueryVsecRegions(
    _In_ PKPH_HW_PCI_LOCATOR Locator,
    _In_ PKPH_DYN_HW_SECTION Section,
    _In_ ULONG Count,
    _Out_writes_(Count) PKPH_HW_VSEC_REGION Regions,
    _Out_ PULONG Total
    );

#if defined(_M_X64)
#define KPH_HW_SMBUS_I801_HOSTC 0x40

typedef struct _KPH_HW_SMBUS_HOST
{
    UCHAR HostKind; // KPH_DYN_HW_SMBUS_HOST_*
    UCHAR Revision;
    UCHAR Hostc;
    UCHAR Reserved;
    USHORT Command;
    USHORT VendorId;
    USHORT DeviceId;
    USHORT Reserved2;
    ULONG Bar;
    PKPH_DYN_HW_OP HostOp;
} KPH_HW_SMBUS_HOST, *PKPH_HW_SMBUS_HOST;

_IRQL_requires_max_(PASSIVE_LEVEL)
_Must_inspect_result_
NTSTATUS KphHwPciReadSmbusHost(
    _In_ PKPH_HW_PCI_LOCATOR Locator,
    _In_ PKPH_DYN_HW_SECTION Section,
    _In_ ULONG OpIndex,
    _Outptr_ PKPH_DYN_HW_OP* Op,
    _Out_ PKPH_HW_SMBUS_HOST Host
    );
#endif

// hwwindow

_IRQL_requires_max_(PASSIVE_LEVEL)
VOID KphInitializeHardwareWindow(
    VOID
    );

_IRQL_requires_max_(PASSIVE_LEVEL)
_Must_inspect_result_
NTSTATUS KphHwWindowCheckRange(
    _In_ ULONG64 Begin,
    _In_ ULONG64 End
    );

_IRQL_requires_max_(PASSIVE_LEVEL)
_Must_inspect_result_
NTSTATUS KphHwWindowLpitBase(
    _In_ UCHAR Entry,
    _In_ ULONG Width,
    _Out_ PULONG64 Base
    );

_IRQL_requires_max_(PASSIVE_LEVEL)
_Must_inspect_result_
NTSTATUS KphHwWindowLpitRelativeBase(
    _In_ UCHAR Entry,
    _In_ ULONG64 Offset,
    _In_ ULONG Size,
    _Out_ PULONG64 Base
    );

_IRQL_requires_max_(PASSIVE_LEVEL)
_Must_inspect_result_
BOOLEAN KphHwWindowValidateEntries(
    _In_ PKPH_DYN_HW_OP Op,
    _In_ ULONG Count,
    _Inout_updates_(Count) PKPH_HW_WINDOW_BATCH_ENTRY Entries,
    _Out_ PULONG SpanBegin,
    _Out_ PULONG SpanEnd
    );

_IRQL_requires_max_(PASSIVE_LEVEL)
_Must_inspect_result_
NTSTATUS KphHwWindowMapAndRead(
    _In_ PKPH_DYN_HW_OP Op,
    _In_ ULONG64 Base,
    _In_ ULONG SpanBegin,
    _In_ ULONG SpanEnd,
    _In_ ULONG Count,
    _Inout_updates_(Count) PKPH_HW_WINDOW_BATCH_ENTRY Entries
    );

_IRQL_requires_max_(PASSIVE_LEVEL)
_Must_inspect_result_
NTSTATUS KphHwReadWindowBatch(
    _In_ PKPH_HW_PCI_LOCATOR Locator,
    _In_ ULONG OpIndex,
    _In_ ULONG Count,
    _Inout_updates_(Count) PKPH_HW_WINDOW_BATCH_ENTRY Entries,
    _In_ KPROCESSOR_MODE AccessMode
    );

_IRQL_requires_max_(PASSIVE_LEVEL)
_Must_inspect_result_
NTSTATUS KphHwReadPlatformWindowBatch(
    _In_ UCHAR Source,
    _In_ ULONG OpIndex,
    _In_ ULONG Count,
    _Inout_updates_(Count) PKPH_HW_WINDOW_BATCH_ENTRY Entries,
    _In_ KPROCESSOR_MODE AccessMode
    );

// hwport

#if defined(_M_X64)
_IRQL_requires_max_(PASSIVE_LEVEL)
VOID KphHwPortPairPorts(
    _In_ UCHAR PortPair,
    _Out_ PUSHORT IndexPort,
    _Out_ PUSHORT DataPort
    );
#endif

_IRQL_requires_max_(PASSIVE_LEVEL)
_Must_inspect_result_
NTSTATUS KphHwReadPortIndirectBatch(
    _In_ PKPH_HW_PCI_LOCATOR Locator,
    _In_ ULONG OpIndex,
    _In_ ULONG Count,
    _Inout_updates_(Count) PKPH_HW_PORT_INDIRECT_BATCH_ENTRY Entries,
    _In_ KPROCESSOR_MODE AccessMode
    );

_IRQL_requires_max_(PASSIVE_LEVEL)
_Must_inspect_result_
NTSTATUS KphHwReadFixedIoWindow(
    _In_ PKPH_DYN_HW_OP Op,
    _In_ ULONG Count,
    _Inout_updates_(Count) PKPH_HW_WINDOW_BATCH_ENTRY Entries
    );

// hwsuperio

_IRQL_requires_max_(PASSIVE_LEVEL)
VOID KphInitializeHardwareSuperIo(
    VOID
    );

_IRQL_requires_max_(PASSIVE_LEVEL)
VOID KphCleanupHardwareSuperIo(
    VOID
    );

_IRQL_requires_max_(PASSIVE_LEVEL)
_Acquires_lock_(_Global_critical_region_)
VOID KphHwAcquireIsaLock(
    VOID
    );

_IRQL_requires_max_(PASSIVE_LEVEL)
_Releases_lock_(_Global_critical_region_)
VOID KphHwReleaseIsaLock(
    VOID
    );

_IRQL_requires_max_(PASSIVE_LEVEL)
_Must_inspect_result_
NTSTATUS KphHwReadSuperIoBatch(
    _In_ UCHAR Slot,
    _In_ ULONG OpIndex,
    _In_ ULONG Count,
    _Inout_updates_opt_(Count) PKPH_HW_SUPERIO_BATCH_ENTRY Entries,
    _Out_opt_ PKPH_HW_SUPERIO_CHIP Chip,
    _In_opt_ PLARGE_INTEGER Timeout,
    _In_ KPROCESSOR_MODE AccessMode
    );

// hwsmbus

_IRQL_requires_max_(PASSIVE_LEVEL)
VOID KphInitializeHardwareSmbus(
    VOID
    );

_IRQL_requires_max_(PASSIVE_LEVEL)
VOID KphCleanupHardwareSmbus(
    VOID
    );

_IRQL_requires_max_(PASSIVE_LEVEL)
_Must_inspect_result_
NTSTATUS KphHwReadSmbusBatch(
    _In_ PKPH_HW_PCI_LOCATOR Locator,
    _In_ ULONG OpIndex,
    _In_ ULONG Count,
    _Inout_updates_(Count) PKPH_HW_SMBUS_BATCH_ENTRY Entries,
    _In_opt_ PLARGE_INTEGER Timeout,
    _In_ KPROCESSOR_MODE AccessMode
    );

// hwvsec

_IRQL_requires_max_(PASSIVE_LEVEL)
_Must_inspect_result_
NTSTATUS KphHwVsecCopyTable(
    _In_ PKPH_HW_PCI_LOCATOR Locator,
    _In_ ULONG64 Base,
    _In_ ULONG EntryCount,
    _In_ ULONG EntryStride,
    _In_ ULONG EntryDwords,
    _Out_writes_(EntryCount * EntryDwords) PULONG Buffer
    );

_IRQL_requires_max_(PASSIVE_LEVEL)
_Must_inspect_result_
BOOLEAN KphHwVsecValidateEntries(
    _In_ PKPH_DYN_HW_OP Op,
    _In_ PKPH_HW_VSEC_LOCATION Location,
    _In_ ULONG Count,
    _Inout_updates_(Count) PKPH_HW_WINDOW_BATCH_ENTRY Entries,
    _Out_ PULONG SpanBegin,
    _Out_ PULONG SpanEnd
    );

_IRQL_requires_max_(PASSIVE_LEVEL)
_Must_inspect_result_
NTSTATUS KphHwVsecMapAndRead(
    _In_ PKPH_HW_PCI_LOCATOR Locator,
    _In_ PKPH_HW_VSEC_LOCATION Location,
    _In_ ULONG SpanBegin,
    _In_ ULONG SpanEnd,
    _In_ ULONG Count,
    _Inout_updates_(Count) PKPH_HW_WINDOW_BATCH_ENTRY Entries
    );

_IRQL_requires_max_(PASSIVE_LEVEL)
_Must_inspect_result_
NTSTATUS KphHwReadVsecBatch(
    _In_ PKPH_HW_PCI_LOCATOR Locator,
    _In_ ULONG OpIndex,
    _In_ ULONG Instance,
    _In_ ULONG Count,
    _Inout_updates_(Count) PKPH_HW_WINDOW_BATCH_ENTRY Entries,
    _In_ KPROCESSOR_MODE AccessMode
    );

_IRQL_requires_max_(PASSIVE_LEVEL)
_Must_inspect_result_
NTSTATUS KphHwQueryVsecRegions(
    _In_ PKPH_HW_PCI_LOCATOR Locator,
    _In_ ULONG Count,
    _Out_writes_to_opt_(Count, *ReturnCount) PKPH_HW_VSEC_REGION Regions,
    _Out_ PULONG ReturnCount,
    _In_ KPROCESSOR_MODE AccessMode
    );

// hwacpi

_IRQL_requires_max_(PASSIVE_LEVEL)
VOID KphInitializeHardwareAcpi(
    VOID
    );

_IRQL_requires_max_(PASSIVE_LEVEL)
VOID KphCleanupHardwareAcpi(
    VOID
    );

_IRQL_requires_max_(PASSIVE_LEVEL)
_Must_inspect_result_
BOOLEAN KphHwAcpiAvailable(
    VOID
    );

_IRQL_requires_max_(PASSIVE_LEVEL)
_Must_inspect_result_
NTSTATUS KphHwAcpiCapturePath(
    _In_ PANSI_STRING Path,
    _Outptr_ PANSI_STRING* Captured,
    _In_ KPROCESSOR_MODE AccessMode
    );

_IRQL_requires_max_(PASSIVE_LEVEL)
_Must_inspect_result_
NTSTATUS KphHwEvalAcpi(
    _In_ PANSI_STRING Path,
    _In_ ULONG OpIndex,
    _In_ ULONG ArgCount,
    _In_reads_(KPH_HW_ACPI_MAX_ARGS) PCULONG Args,
    _In_ ULONG Length,
    _Out_writes_bytes_opt_(Length) PVOID Buffer,
    _Out_opt_ PULONG ReturnLength,
    _In_opt_ PLARGE_INTEGER Timeout,
    _In_ KPROCESSOR_MODE AccessMode
    );

_IRQL_requires_max_(PASSIVE_LEVEL)
_Must_inspect_result_
NTSTATUS KphHwReadAcpiWindowBatch(
    _In_ PANSI_STRING Path,
    _In_ ULONG OpIndex,
    _In_ ULONG Count,
    _Inout_updates_(Count) PKPH_HW_WINDOW_BATCH_ENTRY Entries,
    _In_ KPROCESSOR_MODE AccessMode
    );

#if defined(_M_X64)
typedef struct _KPH_HW_MMIO_SESSION
{
    ANSI_STRING Path;
    PDEVICE_OBJECT Pdo;
    ULONG64 Base;
    PVOID Mapping;
    ULONG MapOffset;
    ULONG MapLength;
} KPH_HW_MMIO_SESSION, *PKPH_HW_MMIO_SESSION;

_IRQL_requires_max_(PASSIVE_LEVEL)
_Must_inspect_result_
NTSTATUS KphHwAcpiOpenMailbox(
    _In_ PCANSI_STRING Path,
    _In_ PKPH_DYN_HW_SECTION Section,
    _In_ ULONG OpIndex,
    _Outptr_ PKPH_DYN_HW_OP* Op,
    _Out_ PKPH_HW_MMIO_SESSION Session
    );

_IRQL_requires_max_(PASSIVE_LEVEL)
_Must_inspect_result_
BOOLEAN KphHwAcpiMailboxMoved(
    _In_ PKPH_HW_MMIO_SESSION Session,
    _In_ PKPH_DYN_HW_OP Op
    );

_IRQL_requires_max_(PASSIVE_LEVEL)
VOID KphHwAcpiCloseMailbox(
    _Inout_ PKPH_HW_MMIO_SESSION Session
    );
#endif

// hwmailbox

_IRQL_requires_max_(PASSIVE_LEVEL)
VOID KphInitializeHardwareMailbox(
    VOID
    );

_IRQL_requires_max_(PASSIVE_LEVEL)
VOID KphCleanupHardwareMailbox(
    VOID
    );

_IRQL_requires_max_(PASSIVE_LEVEL)
_Must_inspect_result_
NTSTATUS KphHwMailboxBatch(
    _In_ PKPH_HW_PCI_LOCATOR Locator,
    _In_ ULONG OpIndex,
    _In_ ULONG Count,
    _Inout_updates_opt_(Count) PKPH_HW_MAILBOX_ENTRY Entries,
    _Out_opt_ PULONG FirmwareVersion,
    _In_opt_ PLARGE_INTEGER Timeout,
    _In_ KPROCESSOR_MODE AccessMode
    );

_IRQL_requires_max_(PASSIVE_LEVEL)
_Must_inspect_result_
NTSTATUS KphHwMailboxBatchAcpi(
    _In_ PANSI_STRING Path,
    _In_ ULONG OpIndex,
    _In_ ULONG Count,
    _Inout_updates_opt_(Count) PKPH_HW_MAILBOX_ENTRY Entries,
    _Out_opt_ PULONG FirmwareVersion,
    _In_opt_ PLARGE_INTEGER Timeout,
    _In_ KPROCESSOR_MODE AccessMode
    );

_IRQL_requires_max_(PASSIVE_LEVEL)
_Must_inspect_result_
NTSTATUS KphHwReadSmuTable(
    _In_ PKPH_HW_PCI_LOCATOR Locator,
    _In_ ULONG OpIndex,
    _In_ ULONG Length,
    _Out_writes_bytes_(Length) PVOID Buffer,
    _Out_opt_ PULONG Version,
    _Out_opt_ PULONG ReturnLength,
    _In_opt_ PLARGE_INTEGER Timeout,
    _In_ KPROCESSOR_MODE AccessMode
    );
