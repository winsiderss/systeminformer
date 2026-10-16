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

#include <kph.h>
#include <kphdyndata.h>
#include <kphhw.h>
#include <initguid.h>
#include <wdmguid.h>

#include <trace.h>

#define KPH_HW_PCI_MAX_DEVICES             32
#define KPH_HW_PCI_MAX_FUNCTIONS           8
#define KPH_HW_PCI_LOCATION_CHARS          128
#define KPH_HW_PCI_CACHE_ENTRIES           8
#define KPH_HW_PCI_IDS_LENGTH                                                  \
    ((ULONG)RTL_SIZEOF_THROUGH_FIELD(PCI_COMMON_HEADER, BIST))
#define KPH_HW_PCI_CAPABILITY_START        0x40
#define KPH_HW_PCI_CAPABILITY_POINTER_MASK 0xFC
#define KPH_HW_PCI_POWER_STATE_D0          0
#define KPH_HW_PCI_EXTENDED_START          0x100
#define KPH_HW_PCI_EXTENDED_TTL                                                \
    ((KPH_DYN_HW_PCI_CONFIG_LENGTH - KPH_HW_PCI_EXTENDED_START) / 8)
#define KPH_HW_PCI_EXT_CAP_NEXT_MASK       0xFFC
#define KPH_HW_PCI_EXT_CAP_VSEC            0x000B
#define KPH_HW_PCI_EXT_CAP_DVSEC           0x0023
#define KPH_HW_PCI_VSEC_HEADER1_OFFSET     0x4
#define KPH_HW_PCI_DVSEC_ID_OFFSET         0x8
#define KPH_HW_PCI_VSEC_ENTRIES_OFFSET     0xA
#define KPH_HW_PCI_VSEC_ENTRY_SIZE_OFFSET  0xB
#define KPH_HW_PCI_VSEC_TABLE_OFFSET       0xC
#define KPH_HW_PCI_VSEC_HEADER_LENGTH      0x10
#define KPH_HW_PCI_VSEC_REVISION           1
#define KPH_HW_PCI_VSEC_INTEL_VENDOR       0x8086
#define KPH_HW_PCI_VSEC_MAX_CAPABILITIES   8
#define KPH_HW_VSEC_BIR_MASK               0x7ul
#define KPH_HW_VSEC_TABLE_SHIFT            3
#define KPH_HW_VSEC_PMT_ENTRY_DWORDS       3
#define KPH_HW_VSEC_PMT_ACCESS_BARID       2
#define KPH_HW_VSEC_PMT_ACCESS_LOCAL       3
#define KPH_HW_VSEC_PMT_ACCESS_RESERVED    0xF
#define KPH_HW_VSEC_PMT_TYPE_PUNIT_FIXED   2
#define KPH_HW_VSEC_PMT_FIXED_BLOCK_GUID   0x10000000
#define KPH_HW_VSEC_TPMI_ENTRY_DWORDS      2
#define KPH_HW_VSEC_TPMI_CAP_UNIT          1024
#define KPH_HW_PCI_MAX_IDS                 256
#define KPH_HW_PCI_ID_VENDOR               8
#define KPH_HW_PCI_ID_DEVICE_PREFIX        12
#define KPH_HW_PCI_ID_DEVICE               17
#define KPH_HW_PCI_ID_CHARS                21
#define KPH_HW_PCI_ID_CLASS                3
#define KPH_HW_PCI_ID_PROG_IF              7
#define KPH_HW_PCI_ID_CLASS_CHARS          9
#define KPH_HW_PCI_ID_CLASS_MIN_CHARS      13
#define KPH_HW_PCI_HEADER_LAYOUTS          3
#define KPH_HW_SMN_APERTURE_LOW            0xFFF00000ul
#define KPH_HW_SMN_APERTURE_HIGH           0x0000FFFFul

typedef union _KPH_HW_PCI_PNP_ADDRESS
{
    struct
    {
        ULONG Function : 16;
        ULONG Device : 16;
    };

    ULONG Value;
} KPH_HW_PCI_PNP_ADDRESS, *PKPH_HW_PCI_PNP_ADDRESS;
C_ASSERT(sizeof(KPH_HW_PCI_PNP_ADDRESS) == sizeof(ULONG));

typedef union _KPH_HW_PCI_PMCSR
{
    struct
    {
        USHORT PowerState : 2;
        USHORT Reserved : 14;
    };

    USHORT Value;
} KPH_HW_PCI_PMCSR, *PKPH_HW_PCI_PMCSR;
C_ASSERT(sizeof(KPH_HW_PCI_PMCSR) == sizeof(USHORT));

typedef union _KPH_HW_PCI_EXT_CAP_HEADER
{
    struct
    {
        ULONG Id : 16;
        ULONG Version : 4;
        ULONG Next : 12;
    };

    ULONG Value;
} KPH_HW_PCI_EXT_CAP_HEADER, *PKPH_HW_PCI_EXT_CAP_HEADER;
C_ASSERT(sizeof(KPH_HW_PCI_EXT_CAP_HEADER) == sizeof(ULONG));

typedef union _KPH_HW_PCI_VSEC_HEADER1
{
    struct
    {
        ULONG Id : 16;
        ULONG Revision : 4;
        ULONG Length : 12;
    };

    ULONG Value;
} KPH_HW_PCI_VSEC_HEADER1, *PKPH_HW_PCI_VSEC_HEADER1;
C_ASSERT(sizeof(KPH_HW_PCI_VSEC_HEADER1) == sizeof(ULONG));

typedef union _KPH_HW_VSEC_PMT_HEADER
{
    struct
    {
        ULONG AccessType : 4;
        ULONG TelemetryType : 4;
        ULONG Reserved0 : 4;
        ULONG Size : 16;
        ULONG Reserved1 : 4;
    };

    ULONG Value;
} KPH_HW_VSEC_PMT_HEADER, *PKPH_HW_VSEC_PMT_HEADER;
C_ASSERT(sizeof(KPH_HW_VSEC_PMT_HEADER) == sizeof(ULONG));

typedef union _KPH_HW_VSEC_TPMI_HEADER
{
    struct
    {
        ULONG FeatureId : 8;
        ULONG Instances : 8;
        ULONG Size : 16;
    };

    ULONG Value;
} KPH_HW_VSEC_TPMI_HEADER, *PKPH_HW_VSEC_TPMI_HEADER;
C_ASSERT(sizeof(KPH_HW_VSEC_TPMI_HEADER) == sizeof(ULONG));

typedef union _KPH_HW_VSEC_TPMI_LOCATION
{
    struct
    {
        ULONG CapOffset : 16;
        ULONG Reserved : 16;
    };

    ULONG Value;
} KPH_HW_VSEC_TPMI_LOCATION, *PKPH_HW_VSEC_TPMI_LOCATION;
C_ASSERT(sizeof(KPH_HW_VSEC_TPMI_LOCATION) == sizeof(ULONG));

typedef struct _KPH_HW_PCI_IDS
{
    USHORT VendorId;
    USHORT DeviceId;
    UCHAR BaseClass;
    UCHAR SubClass;
    UCHAR ProgIf;
    UCHAR HeaderType;
} KPH_HW_PCI_IDS, *PKPH_HW_PCI_IDS;

typedef struct _KPH_HW_PCI_DEVICE
{
    KPH_HW_PCI_LOCATOR Locator;
    PDEVICE_OBJECT Pdo;
    KPH_HW_PCI_IDS Ids;
    ULONG64 LastUse;
    BOOLEAN CapabilitiesWalked;
    BOOLEAN CapabilitiesValid;
    UCHAR PmCapability;
    ULONG64 BarBase[PCI_TYPE0_ADDRESSES];
    ULONG64 BarLength[PCI_TYPE0_ADDRESSES];
} KPH_HW_PCI_DEVICE, *PKPH_HW_PCI_DEVICE;

typedef struct _KPH_HW_PCI_FIND_PDO
{
    PKPH_HW_PCI_LOCATOR Locator;
    PDEVICE_OBJECT Pdo;
} KPH_HW_PCI_FIND_PDO, *PKPH_HW_PCI_FIND_PDO;

KPH_PROTECTED_DATA_SECTION_RO_PUSH();
DECLARE_CONST_UNICODE_STRING(KphpHwPciDriverName, L"\\Driver\\pci");
DECLARE_CONST_UNICODE_STRING(KphpHwPciVendorPrefix, L"PCI\\VEN_");
DECLARE_CONST_UNICODE_STRING(KphpHwPciDevicePrefix, L"&DEV_");
DECLARE_CONST_UNICODE_STRING(KphpHwPciBusPrefix, L"PCI\\");
DECLARE_CONST_UNICODE_STRING(KphpHwPciClassPrefix, L"CC_");
KPH_PROTECTED_DATA_SECTION_RO_POP();
KPH_PROTECTED_DATA_SECTION_PUSH();
static BOOLEAN KphpHwPciIdsKnown = FALSE;
static ULONG KphpHwPciIdCount = 0;
static ULONG KphpHwPciIds[KPH_HW_PCI_MAX_IDS] = { 0 };
KPH_PROTECTED_DATA_SECTION_POP();

static KPH_RWLOCK KphpHwPciLock;
static KPH_HW_PCI_DEVICE KphpHwPciCache[KPH_HW_PCI_CACHE_ENTRIES] = { 0 };
static ULONG64 KphpHwPciCacheClock = 0;
static KPH_HW_PCI_DEVICE KphpHwPciUncached = { 0 };

KPH_PAGED_FILE();

/**
 * \brief Checks whether a vendor and device ID is in the presence list.
 *
 * \param[in] Id VendorId | DeviceId << 16.
 *
 * \return TRUE if the list holds the ID.
 */
_IRQL_requires_max_(PASSIVE_LEVEL)
_Must_inspect_result_
BOOLEAN KphpHwPciIdRecorded(
    _In_ ULONG Id
    )
{
    KPH_PAGED_CODE_PASSIVE();

    for (ULONG i = 0; i < KphpHwPciIdCount; i++)
    {
        if (KphpHwPciIds[i] == Id)
        {
            return TRUE;
        }
    }

    return FALSE;
}

/**
 * \brief Parses up to four hexadecimal digits.
 *
 * \param[in] Text The digits.
 * \param[in] Count The number of digits, at most four.
 * \param[out] Value Receives the value, untouched on failure.
 *
 * \return TRUE if all digits are hexadecimal digits.
 */
_IRQL_requires_max_(PASSIVE_LEVEL)
_Must_inspect_result_
_Success_(return)
BOOLEAN KphpHwPciParseHex(
    _In_reads_(Count) PCWCH Text,
    _In_range_(1, 4) ULONG Count,
    _Out_ PUSHORT Value
    )
{
    USHORT value;

    KPH_PAGED_CODE_PASSIVE();
    NT_ASSERT((Count >= 1) && (Count <= 4));

    value = 0;

    for (ULONG i = 0; i < Count; i++)
    {
        WCHAR c;

        c = Text[i];

        if ((c >= L'0') && (c <= L'9'))
        {
            value = (USHORT)((value << 4) | (c - L'0'));
        }
        else if ((c >= L'A') && (c <= L'F'))
        {
            value = (USHORT)((value << 4) | (c - L'A' + 10));
        }
        else if ((c >= L'a') && (c <= L'f'))
        {
            value = (USHORT)((value << 4) | (c - L'a' + 10));
        }
        else
        {
            return FALSE;
        }
    }

    *Value = value;

    return TRUE;
}

/**
 * \brief Reads a multi-string identifier property of a device.
 *
 * \param[in] DeviceObject The bus-enumerated device object.
 * \param[in] Property DevicePropertyHardwareID or
 * DevicePropertyCompatibleIDs.
 * \param[out] Ids Receives the identifiers, double-terminated and freed with
 * KphFree using KPH_TAG_HW_PCI_BUFFER, NULL on failure.
 *
 * \return Successful or errant status.
 */
_IRQL_requires_max_(PASSIVE_LEVEL)
_Must_inspect_result_
NTSTATUS KphpHwPciReadIdList(
    _In_ PDEVICE_OBJECT DeviceObject,
    _In_ DEVICE_REGISTRY_PROPERTY Property,
    _Outptr_result_z_ PWCHAR* Ids
    )
{
    NTSTATUS status;
    PWCHAR ids;
    ULONG length;
    ULONG allocationLength;

    KPH_PAGED_CODE_PASSIVE();
    NT_ASSERT((Property == DevicePropertyHardwareID) ||
              (Property == DevicePropertyCompatibleIDs));

    *Ids = NULL;

    ids = NULL;
    length = 0;

    status = IoGetDeviceProperty(DeviceObject, Property, 0, NULL, &length);
    if (status != STATUS_BUFFER_TOO_SMALL)
    {
        KphTracePrint(TRACE_LEVEL_VERBOSE,
                      HARDWARE,
                      "IoGetDeviceProperty failed: %!STATUS!",
                      status);

        if (NT_SUCCESS(status))
        {
            status = STATUS_NOT_FOUND;
        }

        goto Exit;
    }

    status = RtlULongAdd(length, (3 * sizeof(UNICODE_NULL)), &allocationLength);
    if (!NT_SUCCESS(status))
    {
        KphTracePrint(TRACE_LEVEL_VERBOSE,
                      HARDWARE,
                      "RtlULongAdd failed: %!STATUS!",
                      status);

        goto Exit;
    }

    ids = KphAllocatePaged(allocationLength, KPH_TAG_HW_PCI_BUFFER);
    if (!ids)
    {
        KphTracePrint(TRACE_LEVEL_VERBOSE,
                      HARDWARE,
                      "Failed to allocate device IDs");

        status = STATUS_INSUFFICIENT_RESOURCES;
        goto Exit;
    }

    status = IoGetDeviceProperty(DeviceObject, Property, length, ids, &length);
    if (!NT_SUCCESS(status))
    {
        KphTracePrint(TRACE_LEVEL_VERBOSE,
                      HARDWARE,
                      "IoGetDeviceProperty failed: %!STATUS!",
                      status);

        goto Exit;
    }

    *Ids = ids;
    ids = NULL;

    status = STATUS_SUCCESS;

Exit:

    if (ids)
    {
        KphFree(ids, KPH_TAG_HW_PCI_BUFFER);
    }

    return status;
}

/**
 * \brief Reads the IDs from the first hardware ID of the form
 * PCI\VEN_vvvv&DEV_dddd.
 *
 * \param[in] DeviceObject The bus-enumerated device object.
 * \param[out] Id Receives VendorId | DeviceId << 16, untouched on failure.
 *
 * \return Successful or errant status.
 */
_IRQL_requires_max_(PASSIVE_LEVEL)
_Must_inspect_result_
NTSTATUS KphpHwPciReadHardwareId(
    _In_ PDEVICE_OBJECT DeviceObject,
    _Out_ PULONG Id
    )
{
    NTSTATUS status;
    PWCHAR ids;
    UNICODE_STRING id;
    UNICODE_STRING part;
    USHORT vendorId;
    USHORT deviceId;

    KPH_PAGED_CODE_PASSIVE();

    status = KphpHwPciReadIdList(DeviceObject, DevicePropertyHardwareID, &ids);
    if (!NT_SUCCESS(status))
    {
        KphTracePrint(TRACE_LEVEL_VERBOSE,
                      HARDWARE,
                      "KphpHwPciReadIdList failed: %!STATUS!",
                      status);

        goto Exit;
    }

    status = STATUS_NOT_FOUND;

    RtlInitUnicodeString(&id, ids);

    while (id.Length > 0)
    {
        if (id.Length >= (KPH_HW_PCI_ID_CHARS * sizeof(WCHAR)))
        {
            part.Buffer = id.Buffer;
            part.Length = KphpHwPciVendorPrefix.Length;
            part.MaximumLength = part.Length;

            if (RtlEqualUnicodeString(&part, &KphpHwPciVendorPrefix, TRUE) &&
                KphpHwPciParseHex(&id.Buffer[KPH_HW_PCI_ID_VENDOR],
                                  4,
                                  &vendorId))
            {
                part.Buffer = &id.Buffer[KPH_HW_PCI_ID_DEVICE_PREFIX];
                part.Length = KphpHwPciDevicePrefix.Length;
                part.MaximumLength = part.Length;

                if (RtlEqualUnicodeString(&part,
                                          &KphpHwPciDevicePrefix,
                                          TRUE) &&
                    KphpHwPciParseHex(&id.Buffer[KPH_HW_PCI_ID_DEVICE],
                                      4,
                                      &deviceId))
                {
                    *Id = ((ULONG)vendorId | ((ULONG)deviceId << 16));
                    status = STATUS_SUCCESS;
                    break;
                }
            }
        }

        RtlInitUnicodeString(&id, &id.Buffer[id.Length / sizeof(WCHAR) + 1]);
    }

    if (status == STATUS_NOT_FOUND)
    {
        KphTracePrint(TRACE_LEVEL_VERBOSE,
                      HARDWARE,
                      "No hardware ID has a vendor and device (first %ws)",
                      ids);
    }

Exit:

    if (ids)
    {
        KphFree(ids, KPH_TAG_HW_PCI_BUFFER);
    }

    return status;
}

/**
 * \brief Reads the class code from the first compatible ID carrying a
 * CC_ccsspp component, as in PCI\VEN_vvvv&DEV_dddd&CC_ccsspp or
 * PCI\CC_ccsspp.
 *
 * \param[in] DeviceObject The bus-enumerated device object.
 * \param[out] BaseClass Receives the base class, untouched on failure.
 * \param[out] SubClass Receives the sub class, untouched on failure.
 * \param[out] ProgIf Receives the programming interface, untouched on
 * failure.
 *
 * \return Successful or errant status.
 */
_IRQL_requires_max_(PASSIVE_LEVEL)
_Must_inspect_result_
NTSTATUS KphpHwPciReadClassCode(
    _In_ PDEVICE_OBJECT DeviceObject,
    _Out_ PUCHAR BaseClass,
    _Out_ PUCHAR SubClass,
    _Out_ PUCHAR ProgIf
    )
{
    NTSTATUS status;
    PWCHAR ids;
    UNICODE_STRING id;
    UNICODE_STRING part;

    KPH_PAGED_CODE_PASSIVE();

    status = KphpHwPciReadIdList(DeviceObject,
                                 DevicePropertyCompatibleIDs,
                                 &ids);
    if (!NT_SUCCESS(status))
    {
        KphTracePrint(TRACE_LEVEL_VERBOSE,
                      HARDWARE,
                      "KphpHwPciReadIdList failed: %!STATUS!",
                      status);

        goto Exit;
    }

    status = STATUS_NOT_FOUND;

    RtlInitUnicodeString(&id, ids);

    while ((id.Length > 0) && (status == STATUS_NOT_FOUND))
    {
        ULONG count;

        part.Buffer = id.Buffer;
        part.Length = KphpHwPciBusPrefix.Length;
        part.MaximumLength = part.Length;

        count = id.Length / sizeof(WCHAR);

        if ((count >= (KPH_HW_PCI_ID_CLASS_MIN_CHARS)) &&
            RtlEqualUnicodeString(&part, &KphpHwPciBusPrefix, TRUE))
        {
            for (ULONG i = KphpHwPciBusPrefix.Length / sizeof(WCHAR);
                 (i + KPH_HW_PCI_ID_CLASS_CHARS) <= count;
                 i++)
            {
                USHORT classes;
                USHORT progIf;
                ULONG end;

                if ((i != (KphpHwPciBusPrefix.Length / sizeof(WCHAR))) &&
                    (id.Buffer[i - 1] != L'&'))
                {
                    continue;
                }

                part.Buffer = &id.Buffer[i];
                part.Length = KphpHwPciClassPrefix.Length;
                part.MaximumLength = part.Length;

                if (!RtlEqualUnicodeString(&part, &KphpHwPciClassPrefix, TRUE))
                {
                    continue;
                }

                end = i + KPH_HW_PCI_ID_CLASS_CHARS;

                if ((end < count) && (id.Buffer[end] != L'&'))
                {
                    continue;
                }

                if (KphpHwPciParseHex(&id.Buffer[i + KPH_HW_PCI_ID_CLASS],
                                      4,
                                      &classes) &&
                    KphpHwPciParseHex(&id.Buffer[i + KPH_HW_PCI_ID_PROG_IF],
                                      2,
                                      &progIf))
                {
                    *BaseClass = (UCHAR)(classes >> 8);
                    *SubClass = (UCHAR)(classes & 0xFF);
                    *ProgIf = (UCHAR)progIf;
                    status = STATUS_SUCCESS;
                    break;
                }
            }
        }

        RtlInitUnicodeString(&id, &id.Buffer[id.Length / sizeof(WCHAR) + 1]);
    }

    if (status == STATUS_NOT_FOUND)
    {
        KphTracePrint(TRACE_LEVEL_VERBOSE,
                      HARDWARE,
                      "No compatible ID has a class code (first %ws)",
                      ids);
    }

Exit:

    if (ids)
    {
        KphFree(ids, KPH_TAG_HW_PCI_BUFFER);
    }

    return status;
}

/**
 * \brief Records the ID of a PCI function.
 *
 * \param[in] DeviceObject A device object of the PCI bus driver.
 * \param[in,out] Context Points to a BOOLEAN that is cleared when presence
 * becomes unknown.
 *
 * \return TRUE to stop once presence is unknown, FALSE to continue.
 */
_Function_class_(KPH_HW_ENUMERATE_DEVICES_CALLBACK)
_IRQL_requires_max_(PASSIVE_LEVEL)
_Must_inspect_result_
BOOLEAN KSIAPI KphpHwPciRecordDevice(
    _In_ PDEVICE_OBJECT DeviceObject,
    _In_opt_ PVOID Context
    )
{
    NTSTATUS status;
    PBOOLEAN known;
    ULONG id;

    KPH_PAGED_CODE_PASSIVE();

    NT_ASSERT(Context);

    known = Context;

    if (!BooleanFlagOn(DeviceObject->Flags, DO_BUS_ENUMERATED_DEVICE))
    {
        return FALSE;
    }

    if (!*known)
    {
        return TRUE;
    }

    status = KphpHwPciReadHardwareId(DeviceObject, &id);
    if (status == STATUS_NOT_FOUND)
    {
        return FALSE;
    }

    if (!NT_SUCCESS(status))
    {
        KphTracePrint(TRACE_LEVEL_VERBOSE,
                      HARDWARE,
                      "KphpHwPciReadHardwareId failed: %!STATUS!",
                      status);

        *known = FALSE;
        return FALSE;
    }

    if (!KphpHwPciIdRecorded(id))
    {
        if (KphpHwPciIdCount < ARRAYSIZE(KphpHwPciIds))
        {
            KphpHwPciIds[KphpHwPciIdCount++] = id;
        }
        else
        {
            KphTracePrint(TRACE_LEVEL_VERBOSE,
                          HARDWARE,
                          "PCI ID list is full at %lu IDs",
                          KphpHwPciIdCount);

            *known = FALSE;
        }
    }

    return FALSE;
}

/**
 * \brief Records the IDs of the PCI functions present.
 *
 * \details A read failure or ID overflow leaves presence unknown; a device
 * without a PCI\VEN_ hardware ID is skipped.
 */
_IRQL_requires_max_(PASSIVE_LEVEL)
VOID KphpHwPciInitializeDevices(
    VOID
    )
{
    NTSTATUS status;
    PDRIVER_OBJECT driver;
    BOOLEAN known;

    KPH_PAGED_CODE_PASSIVE();

    driver = NULL;

    status = KphHwReferenceDriver(&KphpHwPciDriverName, &driver);
    if (!NT_SUCCESS(status))
    {
        KphTracePrint(TRACE_LEVEL_VERBOSE,
                      HARDWARE,
                      "KphHwReferenceDriver failed: %!STATUS!",
                      status);

        KphpHwPciIdsKnown = TRUE;
        goto Exit;
    }

    known = TRUE;

    status = KphHwEnumerateDriverDevices(driver, KphpHwPciRecordDevice, &known);
    if (!NT_SUCCESS(status))
    {
        KphTracePrint(TRACE_LEVEL_VERBOSE,
                      HARDWARE,
                      "KphHwEnumerateDriverDevices failed: %!STATUS!",
                      status);

        goto Exit;
    }

    KphpHwPciIdsKnown = known;

Exit:

    if (driver)
    {
        ObDereferenceObject(driver);
    }

    KphTracePrint(TRACE_LEVEL_VERBOSE,
                  HARDWARE,
                  "PCI devices: %lu IDs, presence known %!bool!",
                  KphpHwPciIdCount,
                  KphpHwPciIdsKnown);
}

/**
 * \brief Initializes the PCI infrastructure.
 */
_IRQL_requires_max_(PASSIVE_LEVEL)
VOID KphInitializeHardwarePci(
    VOID
    )
{
    KPH_PAGED_CODE_PASSIVE();

    KphInitializeRWLock(&KphpHwPciLock);

    KphpHwPciInitializeDevices();
}

/**
 * \brief Checks whether the PCI bus driver is present.
 *
 * \return TRUE if the PCI bus driver object resolves, FALSE otherwise.
 */
_IRQL_requires_max_(PASSIVE_LEVEL)
_Must_inspect_result_
BOOLEAN KphHwPciAvailable(
    VOID
    )
{
    NTSTATUS status;
    PDRIVER_OBJECT driver;

    KPH_PAGED_CODE_PASSIVE();

    status = KphHwReferenceDriver(&KphpHwPciDriverName, &driver);
    if (!NT_SUCCESS(status))
    {
        KphTracePrint(TRACE_LEVEL_VERBOSE,
                      HARDWARE,
                      "KphHwReferenceDriver failed: %!STATUS!",
                      status);

        return FALSE;
    }

    ObDereferenceObject(driver);

    return TRUE;
}

/**
 * \brief Extracts the decimal integers of a PCI location string in order.
 *
 * \param[in] Location The location string.
 * \param[in] Count The number of characters in the string.
 * \param[out] Numbers Receives up to four integers.
 *
 * \return The number of integers, or zero if there are more than four or one
 * does not fit in a USHORT.
 */
_IRQL_requires_max_(PASSIVE_LEVEL)
_Must_inspect_result_
ULONG KphpHwPciParseLocation(
    _In_reads_(Count) PCWCH Location,
    _In_ ULONG Count,
    _Out_writes_(4) PULONG Numbers
    )
{
    ULONG numberCount;
    BOOLEAN inNumber;

    KPH_PAGED_CODE_PASSIVE();

    RtlZeroMemory(Numbers, sizeof(ULONG) * 4);

    numberCount = 0;
    inNumber = FALSE;

    for (ULONG i = 0; (i < Count) && (Location[i] != UNICODE_NULL); i++)
    {
        WCHAR c;

        c = Location[i];

        if ((c < L'0') || (c > L'9'))
        {
            inNumber = FALSE;
            continue;
        }

        if (!inNumber)
        {
            if (numberCount == 4)
            {
                return 0;
            }

            inNumber = TRUE;
            numberCount++;
        }

        Numbers[numberCount - 1] = (Numbers[numberCount - 1] * 10) + (c - L'0');

        if (Numbers[numberCount - 1] > MAXUSHORT)
        {
            return 0;
        }
    }

    return numberCount;
}

/**
 * \brief Checks whether a PCI physical device object is at a locator.
 *
 * \param[in] Pdo A bus-enumerated device object of the PCI bus driver.
 * \param[in] Locator The locator to match.
 *
 * \return TRUE if the device is at the locator, FALSE otherwise.
 */
_IRQL_requires_max_(PASSIVE_LEVEL)
_Must_inspect_result_
BOOLEAN KphpHwPciMatchLocator(
    _In_ PDEVICE_OBJECT Pdo,
    _In_ PKPH_HW_PCI_LOCATOR Locator
    )
{
    NTSTATUS status;
    ULONG busNumber;
    KPH_HW_PCI_PNP_ADDRESS address;
    ULONG resultLength;
    WCHAR location[KPH_HW_PCI_LOCATION_CHARS];
    ULONG locationChars;
    ULONG numbers[4];
    ULONG numberCount;
    ULONG segment;

    KPH_PAGED_CODE_PASSIVE();

    status = IoGetDeviceProperty(Pdo,
                                 DevicePropertyBusNumber,
                                 sizeof(busNumber),
                                 &busNumber,
                                 &resultLength);
    if (!NT_SUCCESS(status))
    {
        KphTracePrint(TRACE_LEVEL_VERBOSE,
                      HARDWARE,
                      "IoGetDeviceProperty failed: %!STATUS!",
                      status);

        return FALSE;
    }

    if (busNumber != Locator->Bus)
    {
        return FALSE;
    }

    status = IoGetDeviceProperty(Pdo,
                                 DevicePropertyAddress,
                                 sizeof(address.Value),
                                 &address.Value,
                                 &resultLength);
    if (!NT_SUCCESS(status))
    {
        KphTracePrint(TRACE_LEVEL_VERBOSE,
                      HARDWARE,
                      "IoGetDeviceProperty failed: %!STATUS!",
                      status);

        return FALSE;
    }

    if ((address.Device != Locator->Device) ||
        (address.Function != Locator->Function))
    {
        return FALSE;
    }

    status = IoGetDeviceProperty(Pdo,
                                 DevicePropertyLocationInformation,
                                 sizeof(location),
                                 location,
                                 &resultLength);
    if (!NT_SUCCESS(status))
    {
        KphTracePrint(TRACE_LEVEL_VERBOSE,
                      HARDWARE,
                      "IoGetDeviceProperty failed: %!STATUS!",
                      status);

        return FALSE;
    }

    locationChars = (min(resultLength, sizeof(location)) / sizeof(WCHAR));

    numberCount = KphpHwPciParseLocation(location, locationChars, numbers);
    if (numberCount == 4)
    {
        segment = numbers[0];
        RtlMoveMemory(&numbers[0], &numbers[1], sizeof(ULONG) * 3);
    }
    else if (numberCount == 3)
    {
        segment = 0;
    }
    else
    {
        KphTracePrint(TRACE_LEVEL_VERBOSE,
                      HARDWARE,
                      "PCI location has %lu numbers",
                      numberCount);

        return FALSE;
    }

    return ((segment == Locator->Segment) &&
            (numbers[0] == Locator->Bus) &&
            (numbers[1] == Locator->Device) &&
            (numbers[2] == Locator->Function));
}

/**
 * \brief Checks whether a PCI vendor and device ID was present at init.
 *
 * \param[in] VendorId The vendor ID.
 * \param[in] DeviceId The device ID.
 *
 * \return STATUS_SUCCESS, STATUS_NOT_FOUND, or STATUS_NOT_SUPPORTED when
 * presence is unknown.
 */
_IRQL_requires_max_(PASSIVE_LEVEL)
_Must_inspect_result_
NTSTATUS KphHwPciIdPresent(
    _In_ USHORT VendorId,
    _In_ USHORT DeviceId
    )
{
    KPH_PAGED_CODE_PASSIVE();

    if (!KphpHwPciIdsKnown)
    {
        KphTracePrint(TRACE_LEVEL_VERBOSE, HARDWARE, "PCI ID presence unknown");

        return STATUS_NOT_SUPPORTED;
    }

    if (KphpHwPciIdRecorded((ULONG)VendorId | ((ULONG)DeviceId << 16)))
    {
        return STATUS_SUCCESS;
    }

    return STATUS_NOT_FOUND;
}

/**
 * \brief Matches a PCI physical device object at a locator.
 *
 * \param[in] DeviceObject A device object of the PCI bus driver.
 * \param[in,out] Context The find context; receives the referenced match.
 *
 * \return TRUE when the device matched, FALSE to continue enumerating.
 */
_Function_class_(KPH_HW_ENUMERATE_DEVICES_CALLBACK)
_IRQL_requires_max_(PASSIVE_LEVEL)
_Must_inspect_result_
BOOLEAN KSIAPI KphpHwPciFindPdoCallback(
    _In_ PDEVICE_OBJECT DeviceObject,
    _In_opt_ PVOID Context
    )
{
    PKPH_HW_PCI_FIND_PDO context;

    KPH_PAGED_CODE_PASSIVE();

    NT_ASSERT(Context);

    context = Context;

    if (!BooleanFlagOn(DeviceObject->Flags, DO_BUS_ENUMERATED_DEVICE) ||
        !KphpHwPciMatchLocator(DeviceObject, context->Locator))
    {
        return FALSE;
    }

    ObReferenceObject(DeviceObject);
    context->Pdo = DeviceObject;

    return TRUE;
}

/**
 * \brief Finds and references the PCI physical device object at a locator.
 *
 * \param[in] Locator The locator of the device.
 * \param[out] Pdo Receives the referenced physical device object.
 *
 * \return Successful or errant status.
 */
_IRQL_requires_max_(PASSIVE_LEVEL)
_Must_inspect_result_
NTSTATUS KphpHwPciFindPdo(
    _In_ PKPH_HW_PCI_LOCATOR Locator,
    _Outptr_ PDEVICE_OBJECT* Pdo
    )
{
    NTSTATUS status;
    PDRIVER_OBJECT driver;
    KPH_HW_PCI_FIND_PDO context;

    KPH_PAGED_CODE_PASSIVE();

    *Pdo = NULL;

    driver = NULL;

    status = KphHwReferenceDriver(&KphpHwPciDriverName, &driver);
    if (!NT_SUCCESS(status))
    {
        KphTracePrint(TRACE_LEVEL_VERBOSE,
                      HARDWARE,
                      "KphHwReferenceDriver failed: %!STATUS!",
                      status);

        goto Exit;
    }

    context.Locator = Locator;
    context.Pdo = NULL;

    status = KphHwEnumerateDriverDevices(driver,
                                         KphpHwPciFindPdoCallback,
                                         &context);
    if (!NT_SUCCESS(status))
    {
        KphTracePrint(TRACE_LEVEL_VERBOSE,
                      HARDWARE,
                      "KphHwEnumerateDriverDevices failed: %!STATUS!",
                      status);

        goto Exit;
    }

    if (!context.Pdo)
    {
        KphTracePrint(TRACE_LEVEL_VERBOSE,
                      HARDWARE,
                      "No PCI device at %04lx:%02lx:%02lx.%lx",
                      (ULONG)Locator->Segment,
                      (ULONG)Locator->Bus,
                      (ULONG)Locator->Device,
                      (ULONG)Locator->Function);

        status = STATUS_NOT_FOUND;
        goto Exit;
    }

    *Pdo = context.Pdo;
    status = STATUS_SUCCESS;

Exit:

    if (driver)
    {
        ObDereferenceObject(driver);
    }

    return status;
}

/**
 * \brief Sends a synchronous PnP request to the top of a device stack.
 *
 * \param[in] Top The top of the device stack.
 * \param[in] MinorFunction The PnP minor function.
 * \param[in] Parameters The parameters of the request.
 * \param[out] Information Receives the information of the completed request.
 *
 * \return Successful or errant status.
 */
_IRQL_requires_max_(PASSIVE_LEVEL)
_Must_inspect_result_
NTSTATUS KphpHwPciSendPnp(
    _In_ PDEVICE_OBJECT Top,
    _In_ UCHAR MinorFunction,
    _In_ const IO_STACK_LOCATION* Parameters,
    _Out_ PULONG_PTR Information
    )
{
    NTSTATUS status;
    KEVENT event;
    IO_STATUS_BLOCK ioStatus;
    PIRP irp;
    PIO_STACK_LOCATION stack;

    KPH_PAGED_CODE_PASSIVE();

    *Information = 0;

    RtlZeroMemory(&ioStatus, sizeof(ioStatus));
    KeInitializeEvent(&event, NotificationEvent, FALSE);

    irp = IoBuildSynchronousFsdRequest(IRP_MJ_PNP,
                                       Top,
                                       NULL,
                                       0,
                                       NULL,
                                       &event,
                                       &ioStatus);
    if (!irp)
    {
        KphTracePrint(TRACE_LEVEL_VERBOSE,
                      HARDWARE,
                      "IoBuildSynchronousFsdRequest failed");

        return STATUS_INSUFFICIENT_RESOURCES;
    }

    //
    // PnP requires an unhandled request to complete as not supported.
    //
    irp->IoStatus.Status = STATUS_NOT_SUPPORTED;
    irp->IoStatus.Information = 0;

    stack = IoGetNextIrpStackLocation(irp);
    stack->MinorFunction = MinorFunction;
    stack->Parameters = Parameters->Parameters;

    status = IoCallDriver(Top, irp);
    if (status == STATUS_PENDING)
    {
        KeWaitForSingleObject(&event, Executive, KernelMode, FALSE, NULL);
        status = ioStatus.Status;
    }

    if (NT_SUCCESS(status))
    {
        *Information = ioStatus.Information;
    }

    return status;
}

/**
 * \brief Confirms that a device stack belongs to a physical device object.
 *
 * \param[in] Top The top of the device stack.
 * \param[in] Pdo The expected physical device object.
 *
 * \return Successful or errant status.
 */
_IRQL_requires_max_(PASSIVE_LEVEL)
_Must_inspect_result_
NTSTATUS KphpHwPciConfirmPdo(
    _In_ PDEVICE_OBJECT Top,
    _In_ PDEVICE_OBJECT Pdo
    )
{
    NTSTATUS status;
    IO_STACK_LOCATION parameters;
    ULONG_PTR information;
    PDEVICE_RELATIONS relations;

    KPH_PAGED_CODE_PASSIVE();

    RtlZeroMemory(&parameters, sizeof(parameters));
    parameters.Parameters.QueryDeviceRelations.Type = TargetDeviceRelation;

    status = KphpHwPciSendPnp(Top,
                              IRP_MN_QUERY_DEVICE_RELATIONS,
                              &parameters,
                              &information);
    if (!NT_SUCCESS(status))
    {
        KphTracePrint(TRACE_LEVEL_VERBOSE,
                      HARDWARE,
                      "TargetDeviceRelation failed: %!STATUS!",
                      status);

        return STATUS_NOT_FOUND;
    }

    relations = (PDEVICE_RELATIONS)information;
    if (!relations)
    {
        KphTracePrint(TRACE_LEVEL_VERBOSE,
                      HARDWARE,
                      "TargetDeviceRelation returned no relations");

        return STATUS_NOT_FOUND;
    }

    if ((relations->Count == 1) && (relations->Objects[0] == Pdo))
    {
        status = STATUS_SUCCESS;
    }
    else
    {
        KphTracePrint(TRACE_LEVEL_VERBOSE,
                      HARDWARE,
                      "TargetDeviceRelation names another device (%lu)",
                      relations->Count);

        status = STATUS_NOT_FOUND;
    }

    for (ULONG i = 0; i < relations->Count; i++)
    {
        ObDereferenceObject(relations->Objects[i]);
    }

    KphFreePool(relations);

    return status;
}

/**
 * \brief Queries the standard bus interface of a device stack.
 *
 * \param[in] Top The top of the device stack.
 * \param[out] Bus Receives the referenced bus interface.
 *
 * \return Successful or errant status.
 */
_IRQL_requires_max_(PASSIVE_LEVEL)
_Must_inspect_result_
NTSTATUS KphpHwPciQueryBusInterface(
    _In_ PDEVICE_OBJECT Top,
    _Out_ PBUS_INTERFACE_STANDARD Bus
    )
{
    NTSTATUS status;
    IO_STACK_LOCATION parameters;
    ULONG_PTR information;

    KPH_PAGED_CODE_PASSIVE();

    RtlZeroMemory(Bus, sizeof(BUS_INTERFACE_STANDARD));

    RtlZeroMemory(&parameters, sizeof(parameters));
    parameters.Parameters.QueryInterface.InterfaceType =
        &GUID_BUS_INTERFACE_STANDARD;
    parameters.Parameters.QueryInterface.Size = sizeof(BUS_INTERFACE_STANDARD);
    parameters.Parameters.QueryInterface.Version = 1;
    parameters.Parameters.QueryInterface.Interface = (PINTERFACE)Bus;
    parameters.Parameters.QueryInterface.InterfaceSpecificData = NULL;

    status = KphpHwPciSendPnp(Top,
                              IRP_MN_QUERY_INTERFACE,
                              &parameters,
                              &information);
    if (!NT_SUCCESS(status))
    {
        KphTracePrint(TRACE_LEVEL_VERBOSE,
                      HARDWARE,
                      "IRP_MN_QUERY_INTERFACE failed: %!STATUS!",
                      status);

        RtlZeroMemory(Bus, sizeof(BUS_INTERFACE_STANDARD));
        return STATUS_NOT_FOUND;
    }

    if ((Bus->Size < sizeof(BUS_INTERFACE_STANDARD)) ||
        !Bus->GetBusData ||
        !Bus->SetBusData ||
        !Bus->InterfaceDereference)
    {
        KphTracePrint(TRACE_LEVEL_VERBOSE,
                      HARDWARE,
                      "Bus interface incomplete (size %lu)",
                      (ULONG)Bus->Size);

        if (Bus->InterfaceDereference)
        {
            Bus->InterfaceDereference(Bus->Context);
        }

        RtlZeroMemory(Bus, sizeof(BUS_INTERFACE_STANDARD));
        return STATUS_NOT_FOUND;
    }

    return STATUS_SUCCESS;
}

/**
 * \brief Releases a bus interface queried by KphpHwPciQueryPdoBus.
 *
 * \param[in,out] Bus The bus interface to release, zeroed on return.
 */
_IRQL_requires_max_(PASSIVE_LEVEL)
VOID KphpHwPciReleaseBus(
    _Inout_ PKPH_HW_PCI_BUS Bus
    )
{
    KPH_PAGED_CODE_PASSIVE();

    if (Bus->Interface.InterfaceDereference)
    {
        Bus->Interface.InterfaceDereference(Bus->Interface.Context);
    }

    if (Bus->Top)
    {
        ObDereferenceObject(Bus->Top);
    }

    RtlZeroMemory(Bus, sizeof(KPH_HW_PCI_BUS));
}

/**
 * \brief Releases a PCI device resolved by KphpHwPciResolve.
 *
 * \param[in,out] Device The device to release, zeroed on return. Its bus
 * interface must already be released.
 */
_IRQL_requires_max_(PASSIVE_LEVEL)
VOID KphpHwPciClose(
    _Inout_ PKPH_HW_PCI_DEVICE Device
    )
{
    KPH_PAGED_CODE_PASSIVE();

    if (Device->Pdo)
    {
        ObDereferenceObject(Device->Pdo);
    }

    RtlZeroMemory(Device, sizeof(KPH_HW_PCI_DEVICE));
}

/**
 * \brief Queries the standard bus interface through the stack of a PDO.
 *
 * \param[in] Pdo The physical device object.
 * \param[in] Confirm TRUE to first confirm that the stack belongs to the PDO.
 * \param[out] Bus Receives the bus interface and the referenced top of the
 * stack, released with KphpHwPciReleaseBus.
 *
 * \return Successful or errant status.
 */
_IRQL_requires_max_(PASSIVE_LEVEL)
_Must_inspect_result_
NTSTATUS KphpHwPciQueryPdoBus(
    _In_ PDEVICE_OBJECT Pdo,
    _In_ BOOLEAN Confirm,
    _Out_ PKPH_HW_PCI_BUS Bus
    )
{
    NTSTATUS status;
    PDEVICE_OBJECT top;
    BUS_INTERFACE_STANDARD busInterface;

    KPH_PAGED_CODE_PASSIVE();

    RtlZeroMemory(Bus, sizeof(KPH_HW_PCI_BUS));

    top = IoGetAttachedDeviceReference(Pdo);

    if (Confirm)
    {
        status = KphpHwPciConfirmPdo(top, Pdo);
        if (!NT_SUCCESS(status))
        {
            KphTracePrint(TRACE_LEVEL_VERBOSE,
                          HARDWARE,
                          "KphpHwPciConfirmPdo failed: %!STATUS!",
                          status);

            goto Exit;
        }
    }

    status = KphpHwPciQueryBusInterface(top, &busInterface);
    if (!NT_SUCCESS(status))
    {
        KphTracePrint(TRACE_LEVEL_VERBOSE,
                      HARDWARE,
                      "KphpHwPciQueryBusInterface failed: %!STATUS!",
                      status);

        goto Exit;
    }

    Bus->Top = top;
    Bus->Interface = busInterface;
    top = NULL;

    status = STATUS_SUCCESS;

Exit:

    if (top)
    {
        ObDereferenceObject(top);
    }

    return status;
}

/**
 * \brief Reads the identifiers and class code through a bus interface.
 *
 * \param[in] Bus The bus interface of the device.
 * \param[out] Ids Receives the identifiers, untouched on failure.
 *
 * \return TRUE if the device answered, FALSE otherwise.
 */
_IRQL_requires_max_(PASSIVE_LEVEL)
_Must_inspect_result_
_Success_(return)
BOOLEAN KphpHwPciReadIds(
    _In_ PBUS_INTERFACE_STANDARD Bus,
    _Out_ PKPH_HW_PCI_IDS Ids
    )
{
    PCI_COMMON_HEADER header;
    ULONG bytes;

    KPH_PAGED_CODE_PASSIVE();

    RtlZeroMemory(&header, sizeof(header));

    bytes = Bus->GetBusData(Bus->Context,
                            PCI_WHICHSPACE_CONFIG,
                            &header,
                            0,
                            KPH_HW_PCI_IDS_LENGTH);
    if ((bytes != KPH_HW_PCI_IDS_LENGTH) ||
        (header.VendorID == PCI_INVALID_VENDORID))
    {
        KphTracePrint(TRACE_LEVEL_VERBOSE,
                      HARDWARE,
                      "PCI identity read failed: %lu bytes, %04lx:%04lx",
                      bytes,
                      (ULONG)header.VendorID,
                      (ULONG)header.DeviceID);

        return FALSE;
    }

    Ids->VendorId = header.VendorID;
    Ids->DeviceId = header.DeviceID;
    Ids->BaseClass = header.BaseClass;
    Ids->SubClass = header.SubClass;
    Ids->ProgIf = header.ProgIf;
    Ids->HeaderType = (UCHAR)(header.HeaderType &
                              KPH_DYN_HW_PCI_HEADER_LAYOUT_MASK);

    return TRUE;
}

/**
 * \brief Looks up the op table of a PCI identity in the hardware section.
 *
 * \param[in] Section The hardware section.
 * \param[in] Ids The identifiers of the device.
 * \param[in] Locator The locator of the device.
 * \param[in] AllowPciRow TRUE to try the PCI row before the CPU-keyed rows.
 * \param[in] AnyHeaderType TRUE when the header type in Ids is unknown and
 * each header layout is tried for the class-keyed row.
 * \param[out] Table Receives the op table.
 *
 * \return Successful or errant status.
 */
_IRQL_requires_max_(PASSIVE_LEVEL)
_Must_inspect_result_
NTSTATUS KphpHwPciLookupTable(
    _In_ PKPH_DYN_HW_SECTION Section,
    _In_ PKPH_HW_PCI_IDS Ids,
    _In_ PKPH_HW_PCI_LOCATOR Locator,
    _In_ BOOLEAN AllowPciRow,
    _In_ BOOLEAN AnyHeaderType,
    _Outptr_ PKPH_DYN_HW_TABLE* Table
    )
{
    NTSTATUS status;
    KPH_DYN_HW_IDENTITY identity;
#if defined(_M_X64)
    PCKPH_HW_CPU_IDENTITY cpu;
    KPH_DYN_HW_CPU_IDENTITY cpuIdentity;
    ULONG headerTypes;
#endif

    KPH_PAGED_CODE_PASSIVE();

    *Table = NULL;

    if (AllowPciRow)
    {
        RtlZeroMemory(&identity, sizeof(identity));
        identity.Pci.VendorId = Ids->VendorId;
        identity.Pci.DeviceId = Ids->DeviceId;

        status = KphDynHwFindTable(Section,
                                   KPH_DYN_HW_IDENTITY_PCI,
                                   &identity,
                                   Table);
        if (NT_SUCCESS(status))
        {
            return status;
        }
    }

#if defined(_M_X64)
    cpu = KphGetHardwareCpuIdentity();

    RtlZeroMemory(&cpuIdentity, sizeof(cpuIdentity));
    RtlCopyMemory(cpuIdentity.Vendor, cpu->Vendor, sizeof(cpuIdentity.Vendor));
    cpuIdentity.DisplayFamily = cpu->DisplayFamily;
    cpuIdentity.DisplayModel = cpu->DisplayModel;

    RtlZeroMemory(&identity, sizeof(identity));
    identity.CpuAndPci.Cpu = cpuIdentity;
    identity.CpuAndPci.Pci.VendorId = Ids->VendorId;
    identity.CpuAndPci.Pci.DeviceId = Ids->DeviceId;

    status = KphDynHwFindTable(Section,
                               KPH_DYN_HW_IDENTITY_CPU_AND_PCI,
                               &identity,
                               Table);
    if (NT_SUCCESS(status))
    {
        return status;
    }

    NT_ASSERT(Locator->Device < KPH_HW_PCI_MAX_DEVICES);
    NT_ASSERT(Locator->Function < KPH_HW_PCI_MAX_FUNCTIONS);

    headerTypes = (AnyHeaderType ? KPH_HW_PCI_HEADER_LAYOUTS : 1);

    for (ULONG i = 0; i < headerTypes; i++)
    {
        RtlZeroMemory(&identity, sizeof(identity));
        identity.CpuAndPciClass.Cpu = cpuIdentity;
        identity.CpuAndPciClass.VendorId = Ids->VendorId;
        identity.CpuAndPciClass.BaseClass = Ids->BaseClass;
        identity.CpuAndPciClass.SubClass = Ids->SubClass;
        identity.CpuAndPciClass.ProgIf = Ids->ProgIf;
        identity.CpuAndPciClass.HeaderType = (AnyHeaderType ? (UCHAR)i : Ids->HeaderType);
        identity.CpuAndPciClass.DevFn = (UCHAR)((Locator->Device << 3) | Locator->Function);

        status = KphDynHwFindTable(Section,
                                   KPH_DYN_HW_IDENTITY_CPU_AND_PCI_CLASS,
                                   &identity,
                                   Table);
        if (NT_SUCCESS(status))
        {
            return status;
        }
    }
#else
    UNREFERENCED_PARAMETER(Locator);
    UNREFERENCED_PARAMETER(AnyHeaderType);
#endif

    KphTracePrint(TRACE_LEVEL_VERBOSE,
                  HARDWARE,
                  "PCI %04lx:%04lx has no table",
                  (ULONG)Ids->VendorId,
                  (ULONG)Ids->DeviceId);

    return STATUS_NOT_FOUND;
}

/**
 * \brief Finds the op table of a device in the hardware section.
 *
 * \param[in] Section The hardware section.
 * \param[in] Device The opened device.
 * \param[in] AllowPciRow TRUE to try the PCI row before the CPU-keyed rows.
 * \param[out] Table Receives the op table.
 *
 * \return Successful or errant status.
 */
_IRQL_requires_max_(PASSIVE_LEVEL)
_Must_inspect_result_
NTSTATUS KphpHwPciFindTable(
    _In_ PKPH_DYN_HW_SECTION Section,
    _In_ PKPH_HW_PCI_DEVICE Device,
    _In_ BOOLEAN AllowPciRow,
    _Outptr_ PKPH_DYN_HW_TABLE* Table
    )
{
    KPH_PAGED_CODE_PASSIVE();

    return KphpHwPciLookupTable(Section,
                                &Device->Ids,
                                &Device->Locator,
                                AllowPciRow,
                                FALSE,
                                Table);
}

/**
 * \brief Checks from PnP properties alone whether the hardware section has a
 * table for a PDO, before any request is sent to its stack.
 *
 * \param[in] Section The hardware section.
 * \param[in] Pdo The physical device object.
 * \param[in] Locator The locator of the device.
 * \param[in] AllowPciRow TRUE to try the PCI row before the CPU-keyed rows.
 *
 * \return Successful or errant status.
 */
_IRQL_requires_max_(PASSIVE_LEVEL)
_Must_inspect_result_
NTSTATUS KphpHwPciGateByPnp(
    _In_ PKPH_DYN_HW_SECTION Section,
    _In_ PDEVICE_OBJECT Pdo,
    _In_ PKPH_HW_PCI_LOCATOR Locator,
    _In_ BOOLEAN AllowPciRow
    )
{
    NTSTATUS status;
    ULONG id;
    KPH_HW_PCI_IDS ids;
    PKPH_DYN_HW_TABLE table;

    KPH_PAGED_CODE_PASSIVE();

    RtlZeroMemory(&ids, sizeof(ids));

    status = KphpHwPciReadHardwareId(Pdo, &id);
    if (!NT_SUCCESS(status))
    {
        KphTracePrint(TRACE_LEVEL_VERBOSE,
                      HARDWARE,
                      "KphpHwPciReadHardwareId failed: %!STATUS!",
                      status);

        return status;
    }

    ids.VendorId = LOWORD(id);
    ids.DeviceId = HIWORD(id);

    status = KphpHwPciReadClassCode(Pdo,
                                    &ids.BaseClass,
                                    &ids.SubClass,
                                    &ids.ProgIf);
    if (!NT_SUCCESS(status))
    {
        KphTracePrint(TRACE_LEVEL_VERBOSE,
                      HARDWARE,
                      "KphpHwPciReadClassCode failed: %!STATUS!",
                      status);

        return status;
    }

    status = KphpHwPciLookupTable(Section,
                                  &ids,
                                  Locator,
                                  AllowPciRow,
                                  TRUE,
                                  &table);
    if (!NT_SUCCESS(status))
    {
        KphTracePrint(TRACE_LEVEL_VERBOSE,
                      HARDWARE,
                      "PCI %04lx:%04lx class %02lx%02lx%02lx at "
                      "%04lx:%02lx:%02lx.%lx is not allow-listed",
                      (ULONG)ids.VendorId,
                      (ULONG)ids.DeviceId,
                      (ULONG)ids.BaseClass,
                      (ULONG)ids.SubClass,
                      (ULONG)ids.ProgIf,
                      (ULONG)Locator->Segment,
                      (ULONG)Locator->Bus,
                      (ULONG)Locator->Device,
                      (ULONG)Locator->Function);

        return status;
    }

    return STATUS_SUCCESS;
}

/**
 * \brief Resolves the PCI device at a locator through its PDO.
 *
 * \param[in] Section The hardware section.
 * \param[in] Locator The locator of the device.
 * \param[in] AllowPciRow TRUE to try the PCI row before the CPU-keyed rows.
 * \param[out] Device Receives the resolved device, released with
 * KphpHwPciClose.
 * \param[out] Bus Receives the bus interface and the referenced top of the
 * stack, released with KphpHwPciReleaseBus.
 *
 * \return Successful or errant status.
 */
_IRQL_requires_max_(PASSIVE_LEVEL)
_Must_inspect_result_
NTSTATUS KphpHwPciResolve(
    _In_ PKPH_DYN_HW_SECTION Section,
    _In_ PKPH_HW_PCI_LOCATOR Locator,
    _In_ BOOLEAN AllowPciRow,
    _Out_ PKPH_HW_PCI_DEVICE Device,
    _Out_ PKPH_HW_PCI_BUS Bus
    )
{
    NTSTATUS status;
    KPH_HW_PCI_DEVICE device;
    KPH_HW_PCI_BUS bus;

    KPH_PAGED_CODE_PASSIVE();

    RtlZeroMemory(Device, sizeof(KPH_HW_PCI_DEVICE));
    RtlZeroMemory(Bus, sizeof(KPH_HW_PCI_BUS));

    RtlZeroMemory(&device, sizeof(device));
    RtlZeroMemory(&bus, sizeof(bus));

    status = KphpHwPciFindPdo(Locator, &device.Pdo);
    if (!NT_SUCCESS(status))
    {
        KphTracePrint(TRACE_LEVEL_VERBOSE,
                      HARDWARE,
                      "KphpHwPciFindPdo failed: %!STATUS!",
                      status);

        goto Exit;
    }

    status = KphpHwPciGateByPnp(Section, device.Pdo, Locator, AllowPciRow);
    if (!NT_SUCCESS(status))
    {
        KphTracePrint(TRACE_LEVEL_VERBOSE,
                      HARDWARE,
                      "KphpHwPciGateByPnp failed: %!STATUS!",
                      status);

        goto Exit;
    }

    status = KphpHwPciQueryPdoBus(device.Pdo, TRUE, &bus);
    if (!NT_SUCCESS(status))
    {
        KphTracePrint(TRACE_LEVEL_VERBOSE,
                      HARDWARE,
                      "KphpHwPciQueryPdoBus failed: %!STATUS!",
                      status);

        goto Exit;
    }

    if (!KphpHwPciReadIds(&bus.Interface, &device.Ids))
    {
        KphTracePrint(TRACE_LEVEL_VERBOSE, HARDWARE, "KphpHwPciReadIds failed");

        status = STATUS_NOT_FOUND;
        goto Exit;
    }

    device.Locator = *Locator;

    *Device = device;
    RtlZeroMemory(&device, sizeof(device));

    *Bus = bus;
    RtlZeroMemory(&bus, sizeof(bus));

    status = STATUS_SUCCESS;

Exit:

    KphpHwPciReleaseBus(&bus);
    KphpHwPciClose(&device);

    return status;
}

/**
 * \brief Releases what KphpHwPciOpen returned.
 *
 * \param[in,out] Bus The bus interface to release.
 */
_IRQL_requires_max_(PASSIVE_LEVEL)
_Requires_lock_held_(KphpHwPciLock)
VOID KphpHwPciRelease(
    _Inout_ PKPH_HW_PCI_BUS Bus
    )
{
    KPH_PAGED_CODE_PASSIVE();

    KphpHwPciReleaseBus(Bus);
    KphpHwPciClose(&KphpHwPciUncached);
}

/**
 * \brief Revalidates a cached device through its PDO.
 *
 * \param[in] Device The cached device.
 * \param[out] Bus Receives the bus interface and the referenced top of the
 * stack, released with KphpHwPciReleaseBus.
 *
 * \return STATUS_SUCCESS if the device still answers with its identity,
 * otherwise an errant status.
 */
_IRQL_requires_max_(PASSIVE_LEVEL)
_Must_inspect_result_
NTSTATUS KphpHwPciRevalidate(
    _In_ PKPH_HW_PCI_DEVICE Device,
    _Out_ PKPH_HW_PCI_BUS Bus
    )
{
    NTSTATUS status;
    KPH_HW_PCI_BUS bus;
    KPH_HW_PCI_IDS ids;

    KPH_PAGED_CODE_PASSIVE();

    RtlZeroMemory(Bus, sizeof(KPH_HW_PCI_BUS));

    status = KphpHwPciQueryPdoBus(Device->Pdo, FALSE, &bus);
    if (!NT_SUCCESS(status))
    {
        KphTracePrint(TRACE_LEVEL_VERBOSE,
                      HARDWARE,
                      "PCI cache entry %04lx:%04lx interface failed: "
                      "%!STATUS!",
                      (ULONG)Device->Ids.VendorId,
                      (ULONG)Device->Ids.DeviceId,
                      status);

        goto Exit;
    }

    if (!KphpHwPciReadIds(&bus.Interface, &ids))
    {
        KphTracePrint(TRACE_LEVEL_VERBOSE,
                      HARDWARE,
                      "PCI cache entry %04lx:%04lx identity read failed",
                      (ULONG)Device->Ids.VendorId,
                      (ULONG)Device->Ids.DeviceId);

        status = STATUS_NOT_FOUND;
        goto Exit;
    }

    if ((ids.VendorId != Device->Ids.VendorId) ||
        (ids.DeviceId != Device->Ids.DeviceId) ||
        (ids.BaseClass != Device->Ids.BaseClass) ||
        (ids.SubClass != Device->Ids.SubClass) ||
        (ids.ProgIf != Device->Ids.ProgIf) ||
        (ids.HeaderType != Device->Ids.HeaderType))
    {
        KphTracePrint(TRACE_LEVEL_VERBOSE,
                      HARDWARE,
                      "PCI cache entry %04lx:%04lx class %02lx%02lx%02lx "
                      "header %02lx now %04lx:%04lx class %02lx%02lx%02lx "
                      "header %02lx",
                      (ULONG)Device->Ids.VendorId,
                      (ULONG)Device->Ids.DeviceId,
                      (ULONG)Device->Ids.BaseClass,
                      (ULONG)Device->Ids.SubClass,
                      (ULONG)Device->Ids.ProgIf,
                      (ULONG)Device->Ids.HeaderType,
                      (ULONG)ids.VendorId,
                      (ULONG)ids.DeviceId,
                      (ULONG)ids.BaseClass,
                      (ULONG)ids.SubClass,
                      (ULONG)ids.ProgIf,
                      (ULONG)ids.HeaderType);

        status = STATUS_NOT_FOUND;
        goto Exit;
    }

    *Bus = bus;
    RtlZeroMemory(&bus, sizeof(bus));

    status = STATUS_SUCCESS;

Exit:

    KphpHwPciReleaseBus(&bus);

    return status;
}

/**
 * \brief Opens the allow-listed PCI device at a locator through the cache.
 *
 * \details A cached device is revalidated on each open. Only devices with a
 * table that expect no removal are cached.
 *
 * \param[in] Locator The locator of the device.
 * \param[in] Section The hardware section.
 * \param[in] AllowPciRow TRUE to try the PCI row before the CPU-keyed rows.
 * \param[out] Device Receives the device, valid while the lock is held.
 * \param[out] Bus Receives the bus interface and the referenced top of the
 * stack, released with KphpHwPciRelease before the lock.
 * \param[out] Table Receives the op table.
 *
 * \return Successful or errant status.
 */
_IRQL_requires_max_(PASSIVE_LEVEL)
_Requires_lock_held_(KphpHwPciLock)
_Must_inspect_result_
NTSTATUS KphpHwPciOpen(
    _In_ PKPH_HW_PCI_LOCATOR Locator,
    _In_ PKPH_DYN_HW_SECTION Section,
    _In_ BOOLEAN AllowPciRow,
    _Outptr_ PKPH_HW_PCI_DEVICE* Device,
    _Out_ PKPH_HW_PCI_BUS Bus,
    _Outptr_ PKPH_DYN_HW_TABLE* Table
    )
{
    NTSTATUS status;
    PKPH_HW_PCI_DEVICE device;
    KPH_HW_PCI_DEVICE resolved;
    KPH_HW_PCI_BUS bus;
    PKPH_DYN_HW_TABLE table;

    KPH_PAGED_CODE_PASSIVE();
    NT_ASSERT(!KphpHwPciUncached.Pdo);

    *Device = NULL;
    *Table = NULL;
    RtlZeroMemory(Bus, sizeof(KPH_HW_PCI_BUS));

    device = NULL;
    RtlZeroMemory(&resolved, sizeof(resolved));
    RtlZeroMemory(&bus, sizeof(bus));

    for (ULONG i = 0; i < ARRAYSIZE(KphpHwPciCache); i++)
    {
        PKPH_HW_PCI_DEVICE entry;

        entry = &KphpHwPciCache[i];

        if (entry->Pdo &&
            (entry->Locator.Segment == Locator->Segment) &&
            (entry->Locator.Bus == Locator->Bus) &&
            (entry->Locator.Device == Locator->Device) &&
            (entry->Locator.Function == Locator->Function))
        {
            device = entry;
            break;
        }
    }

    if (device)
    {
        status = KphpHwPciRevalidate(device, &bus);
        if (!NT_SUCCESS(status))
        {
            KphTracePrint(TRACE_LEVEL_VERBOSE,
                          HARDWARE,
                          "KphpHwPciRevalidate failed: %!STATUS!",
                          status);

            KphpHwPciClose(device);
            device = NULL;
        }
    }

    if (device)
    {
        status = KphpHwPciFindTable(Section, device, AllowPciRow, &table);
        if (!NT_SUCCESS(status))
        {
            KphTracePrint(TRACE_LEVEL_VERBOSE,
                          HARDWARE,
                          "KphpHwPciFindTable failed: %!STATUS!",
                          status);

            goto Exit;
        }
    }
    else
    {
        status = KphpHwPciResolve(Section,
                                  Locator,
                                  AllowPciRow,
                                  &resolved,
                                  &bus);
        if (!NT_SUCCESS(status))
        {
            KphTracePrint(TRACE_LEVEL_VERBOSE,
                          HARDWARE,
                          "KphpHwPciResolve failed: %!STATUS!",
                          status);

            goto Exit;
        }

        status = KphpHwPciFindTable(Section, &resolved, AllowPciRow, &table);
        if (!NT_SUCCESS(status))
        {
            KphTracePrint(TRACE_LEVEL_VERBOSE,
                          HARDWARE,
                          "KphpHwPciFindTable failed: %!STATUS!",
                          status);

            goto Exit;
        }

        if (KphHwExpectsNoRemoval(resolved.Pdo))
        {
            device = &KphpHwPciCache[0];

            for (ULONG i = 0; i < ARRAYSIZE(KphpHwPciCache); i++)
            {
                if (!KphpHwPciCache[i].Pdo)
                {
                    device = &KphpHwPciCache[i];
                    break;
                }

                if (KphpHwPciCache[i].LastUse < device->LastUse)
                {
                    device = &KphpHwPciCache[i];
                }
            }

            KphpHwPciClose(device);
        }
        else
        {
            device = &KphpHwPciUncached;
        }

        *device = resolved;
        RtlZeroMemory(&resolved, sizeof(resolved));
    }

    device->LastUse = ++KphpHwPciCacheClock;

    *Device = device;
    *Table = table;

    *Bus = bus;
    RtlZeroMemory(&bus, sizeof(bus));

    status = STATUS_SUCCESS;

Exit:

    KphpHwPciReleaseBus(&bus);
    KphpHwPciClose(&resolved);

    return status;
}

/**
 * \brief Releases the cached PCI devices.
 */
_IRQL_requires_max_(PASSIVE_LEVEL)
VOID KphCleanupHardwarePci(
    VOID
    )
{
    KPH_PAGED_CODE_PASSIVE();

    KphAcquireRWLockExclusive(&KphpHwPciLock);

    for (ULONG i = 0; i < ARRAYSIZE(KphpHwPciCache); i++)
    {
        KphpHwPciClose(&KphpHwPciCache[i]);
    }

    NT_ASSERT(!KphpHwPciUncached.Pdo);

    KphReleaseRWLock(&KphpHwPciLock);

    KphDeleteRWLock(&KphpHwPciLock);
}

/**
 * \brief Checks that a PCI locator is well formed.
 *
 * \param[in] Locator The locator to check.
 *
 * \return TRUE if the locator is well formed, FALSE otherwise.
 */
_IRQL_requires_max_(PASSIVE_LEVEL)
_Must_inspect_result_
BOOLEAN KphpHwPciValidLocator(
    _In_ PKPH_HW_PCI_LOCATOR Locator
    )
{
    KPH_PAGED_CODE_PASSIVE();

    return ((Locator->Device < KPH_HW_PCI_MAX_DEVICES) &&
            (Locator->Function < KPH_HW_PCI_MAX_FUNCTIONS));
}

/**
 * \brief Reads allow-listed PCI configuration space.
 *
 * \param[in] Locator The locator of the device.
 * \param[in] Offset The configuration space offset.
 * \param[in] Length The number of bytes to read.
 * \param[out] Buffer Receives the configuration space bytes.
 * \param[out] ReturnLength Receives the number of bytes read.
 * \param[in] AccessMode The mode in which to perform access checks.
 *
 * \return Successful or errant status.
 */
_IRQL_requires_max_(PASSIVE_LEVEL)
_Must_inspect_result_
NTSTATUS KphHwReadPciConfig(
    _In_ PKPH_HW_PCI_LOCATOR Locator,
    _In_ ULONG Offset,
    _In_ ULONG Length,
    _Out_writes_bytes_(Length) PVOID Buffer,
    _Out_opt_ PULONG ReturnLength,
    _In_ KPROCESSOR_MODE AccessMode
    )
{
    NTSTATUS status;
    KPH_HW_PCI_LOCATOR locator;
    ULONG end;
    PKPH_DYN dyn;
    BOOLEAN locked;
    PKPH_HW_PCI_DEVICE device;
    KPH_HW_PCI_BUS bus;
    PKPH_DYN_HW_TABLE table;
    PKPH_DYN_HW_OP op;
    PVOID buffer;
    ULONG bytes;

    KPH_PAGED_CODE_PASSIVE();

    dyn = NULL;
    locked = FALSE;
    RtlZeroMemory(&bus, sizeof(bus));
    buffer = NULL;

    locator = *Locator;

    if ((Length == 0) || (Length > KPH_DYN_HW_PCI_CONFIG_LENGTH))
    {
        KphTracePrint(TRACE_LEVEL_VERBOSE,
                      HARDWARE,
                      "Invalid PCI config length %lu",
                      Length);

        status = STATUS_INVALID_PARAMETER;
        goto Exit;
    }

    status = RtlULongAdd(Offset, Length, &end);
    if (!NT_SUCCESS(status))
    {
        KphTracePrint(TRACE_LEVEL_VERBOSE,
                      HARDWARE,
                      "RtlULongAdd failed: %!STATUS!",
                      status);

        goto Exit;
    }

    if (end > KPH_DYN_HW_PCI_CONFIG_LENGTH)
    {
        KphTracePrint(TRACE_LEVEL_VERBOSE,
                      HARDWARE,
                      "Invalid PCI config range 0x%lx-0x%lx",
                      Offset,
                      end);

        status = STATUS_INVALID_PARAMETER;
        goto Exit;
    }

    if (!KphpHwPciValidLocator(&locator))
    {
        KphTracePrint(TRACE_LEVEL_VERBOSE, HARDWARE, "Invalid PCI locator");

        status = STATUS_INVALID_PARAMETER;
        goto Exit;
    }

    dyn = KphReferenceDynData();
    if (!dyn || !dyn->Hardware.Section)
    {
        status = STATUS_NOINTERFACE;
        goto Exit;
    }

    KphAcquireRWLockExclusive(&KphpHwPciLock);
    locked = TRUE;

    status = KphpHwPciOpen(&locator,
                           dyn->Hardware.Section,
                           TRUE,
                           &device,
                           &bus,
                           &table);
    if (!NT_SUCCESS(status))
    {
        KphTracePrint(TRACE_LEVEL_VERBOSE,
                      HARDWARE,
                      "KphpHwPciOpen failed: %!STATUS!",
                      status);

        goto Exit;
    }

    status = KphDynHwFindPciReadOp(table, Offset, Length, &op);
    if (!NT_SUCCESS(status))
    {
        KphTracePrint(TRACE_LEVEL_VERBOSE,
                      HARDWARE,
                      "KphDynHwFindPciReadOp failed: %!STATUS!",
                      status);

        status = STATUS_ACCESS_DENIED;
        goto Exit;
    }

    //
    // The bus driver may touch the buffer at raised IRQL.
    //
    buffer = KphAllocateNPaged(Length, KPH_TAG_HW_PCI_BUFFER);
    if (!buffer)
    {
        KphTracePrint(TRACE_LEVEL_VERBOSE,
                      HARDWARE,
                      "Failed to allocate PCI config buffer");

        status = STATUS_INSUFFICIENT_RESOURCES;
        goto Exit;
    }

    bytes = bus.Interface.GetBusData(bus.Interface.Context,
                                     PCI_WHICHSPACE_CONFIG,
                                     buffer,
                                     Offset,
                                     Length);
    if (bytes != Length)
    {
        KphTracePrint(TRACE_LEVEL_VERBOSE,
                      HARDWARE,
                      "GetBusData read %lu of %lu bytes",
                      bytes,
                      Length);

        KphpHwPciReleaseBus(&bus);
        KphpHwPciClose(device);
        status = STATUS_DEVICE_NOT_READY;
        goto Exit;
    }

    KphpHwPciRelease(&bus);

    KphReleaseRWLock(&KphpHwPciLock);
    locked = FALSE;

    status = KphCopyToMode(Buffer, buffer, Length, AccessMode);
    if (!NT_SUCCESS(status))
    {
        goto Exit;
    }

    if (ReturnLength)
    {
        status = KphWriteULongToMode(ReturnLength, Length, AccessMode);
        if (!NT_SUCCESS(status))
        {
            goto Exit;
        }
    }

    status = STATUS_SUCCESS;

Exit:

    if (locked)
    {
        KphpHwPciRelease(&bus);
        KphReleaseRWLock(&KphpHwPciLock);
    }

    if (buffer)
    {
        KphFree(buffer, KPH_TAG_HW_PCI_BUFFER);
    }

    if (dyn)
    {
        KphDereferenceObject(dyn);
    }

    return status;
}

/**
 * \brief Reads configuration space through a bus interface.
 *
 * \param[in] Bus The bus interface of the device.
 * \param[in] Offset The configuration space offset.
 * \param[out] Buffer Receives the bytes.
 * \param[in] Length The number of bytes to read.
 *
 * \return TRUE if every byte was read, FALSE otherwise.
 */
_IRQL_requires_max_(PASSIVE_LEVEL)
_Must_inspect_result_
_Success_(return)
BOOLEAN KphpHwPciReadConfig(
    _In_ PBUS_INTERFACE_STANDARD Bus,
    _In_ ULONG Offset,
    _Out_writes_bytes_(Length) PVOID Buffer,
    _In_ ULONG Length
    )
{
    KPH_PAGED_CODE_PASSIVE();

    return (Bus->GetBusData(Bus->Context,
                            PCI_WHICHSPACE_CONFIG,
                            Buffer,
                            Offset,
                            Length) == Length);
}

/**
 * \brief Finds the power management capability of a device.
 *
 * \param[in] Bus The bus interface of the device.
 * \param[in] HeaderType The header layout of the device.
 * \param[out] PmCapability Receives the PM capability offset, zero when none,
 * untouched on failure.
 *
 * \return Successful or errant status; STATUS_DEVICE_NOT_READY when a read
 * failed or read all-ones.
 */
_IRQL_requires_max_(PASSIVE_LEVEL)
_Must_inspect_result_
NTSTATUS KphpHwPciFindPmCapability(
    _In_ PBUS_INTERFACE_STANDARD Bus,
    _In_ UCHAR HeaderType,
    _Out_ PUCHAR PmCapability
    )
{
    USHORT pciStatus;
    ULONG pointerOffset;
    UCHAR pointer;
    ULONG64 visited;

    KPH_PAGED_CODE_PASSIVE();

    if (!KphpHwPciReadConfig(Bus,
                             FIELD_OFFSET(PCI_COMMON_HEADER, Status),
                             &pciStatus,
                             sizeof(pciStatus)) ||
        (pciStatus == MAXUSHORT))
    {
        KphTracePrint(TRACE_LEVEL_VERBOSE, HARDWARE, "PCI status read failed");

        return STATUS_DEVICE_NOT_READY;
    }

    if (!BooleanFlagOn(pciStatus, PCI_STATUS_CAPABILITIES_LIST))
    {
        *PmCapability = 0;
        return STATUS_SUCCESS;
    }

    switch (HeaderType)
    {
        case PCI_DEVICE_TYPE:
        {
            pointerOffset = FIELD_OFFSET(PCI_COMMON_HEADER,
                                         u.type0.CapabilitiesPtr);
            break;
        }
        case PCI_BRIDGE_TYPE:
        {
            pointerOffset = FIELD_OFFSET(PCI_COMMON_HEADER,
                                         u.type1.CapabilitiesPtr);
            break;
        }
        case PCI_CARDBUS_BRIDGE_TYPE:
        {
            pointerOffset = FIELD_OFFSET(PCI_COMMON_HEADER,
                                         u.type2.CapabilitiesPtr);
            break;
        }
        default:
        {
            KphTracePrint(TRACE_LEVEL_VERBOSE,
                          HARDWARE,
                          "PCI header type %lu has no capabilities pointer",
                          (ULONG)HeaderType);

            return STATUS_INVALID_DEVICE_STATE;
        }
    }

    if (!KphpHwPciReadConfig(Bus, pointerOffset, &pointer, sizeof(pointer)))
    {
        KphTracePrint(TRACE_LEVEL_VERBOSE,
                      HARDWARE,
                      "PCI capabilities pointer read failed");

        return STATUS_DEVICE_NOT_READY;
    }

    visited = 0;
    pointer &= KPH_HW_PCI_CAPABILITY_POINTER_MASK;

    while (pointer)
    {
        PCI_CAPABILITIES_HEADER header;
        ULONG slot;

        if (pointer < KPH_HW_PCI_CAPABILITY_START)
        {
            KphTracePrint(TRACE_LEVEL_VERBOSE,
                          HARDWARE,
                          "PCI capability at 0x%lx is in the header",
                          (ULONG)pointer);

            return STATUS_INVALID_DEVICE_STATE;
        }

        slot = ((pointer - KPH_HW_PCI_CAPABILITY_START) / 4);

        if (BooleanFlagOn(visited, 1ull << slot))
        {
            KphTracePrint(TRACE_LEVEL_VERBOSE,
                          HARDWARE,
                          "PCI capability chain loops at 0x%lx",
                          (ULONG)pointer);

            return STATUS_INVALID_DEVICE_STATE;
        }

        SetFlag(visited, 1ull << slot);

        if (!KphpHwPciReadConfig(Bus, pointer, &header, sizeof(header)) ||
            ((header.CapabilityID == MAXUCHAR) && (header.Next == MAXUCHAR)))
        {
            KphTracePrint(TRACE_LEVEL_VERBOSE,
                          HARDWARE,
                          "PCI capability read failed at 0x%lx",
                          (ULONG)pointer);

            return STATUS_DEVICE_NOT_READY;
        }

        if (header.CapabilityID == PCI_CAPABILITY_ID_POWER_MANAGEMENT)
        {
            *PmCapability = pointer;
            return STATUS_SUCCESS;
        }

        pointer = (header.Next & KPH_HW_PCI_CAPABILITY_POINTER_MASK);
    }

    *PmCapability = 0;

    return STATUS_SUCCESS;
}

/**
 * \brief Walks the capability chain of a device once and caches the result.
 *
 * \param[in,out] Device The device.
 * \param[in] Bus The bus interface of the device.
 *
 * \return Successful or errant status; success does not imply the chain is
 * valid.
 */
_IRQL_requires_max_(PASSIVE_LEVEL)
_Requires_lock_held_(KphpHwPciLock)
_Must_inspect_result_
NTSTATUS KphpHwPciEnsureCapabilities(
    _Inout_ PKPH_HW_PCI_DEVICE Device,
    _In_ PBUS_INTERFACE_STANDARD Bus
    )
{
    NTSTATUS status;

    KPH_PAGED_CODE_PASSIVE();

    if (Device->CapabilitiesWalked)
    {
        return STATUS_SUCCESS;
    }

    status = KphpHwPciFindPmCapability(Bus,
                                       Device->Ids.HeaderType,
                                       &Device->PmCapability);
    if (status == STATUS_DEVICE_NOT_READY)
    {
        KphTracePrint(TRACE_LEVEL_VERBOSE,
                      HARDWARE,
                      "KphpHwPciFindPmCapability failed: %!STATUS!",
                      status);

        return status;
    }

    Device->CapabilitiesWalked = TRUE;
    Device->CapabilitiesValid = NT_SUCCESS(status);

    return STATUS_SUCCESS;
}

/**
 * \brief Performs one locked index write, data read and index re-read.
 *
 * \param[in] Bus The bus interface of the device.
 * \param[in] IndexOffset The configuration offset of the index register.
 * \param[in] DataOffset The configuration offset of the data register.
 * \param[in] Target The index register value.
 * \param[out] Value Receives the data register value.
 *
 * \return Successful or errant status.
 */
_IRQL_requires_max_(PASSIVE_LEVEL)
_Requires_lock_held_(KphpHwPciLock)
_Must_inspect_result_
NTSTATUS KphpHwPciIndirectCycle(
    _In_ PBUS_INTERFACE_STANDARD Bus,
    _In_ USHORT IndexOffset,
    _In_ USHORT DataOffset,
    _In_ ULONG Target,
    _Out_ PULONG Value
    )
{
    ULONG index;
    ULONG value;
    ULONG check;
    ULONG bytes;

    KPH_PAGED_CODE_PASSIVE();

    *Value = 0;

    value = 0;
    check = 0;

    for (ULONG i = 0; i < 2; i++)
    {
        index = Target;

        bytes = Bus->SetBusData(Bus->Context,
                                PCI_WHICHSPACE_CONFIG,
                                &index,
                                IndexOffset,
                                sizeof(index));
        if (bytes != sizeof(index))
        {
            KphTracePrint(TRACE_LEVEL_VERBOSE,
                          HARDWARE,
                          "Index write at 0x%lx failed: %lu bytes",
                          (ULONG)IndexOffset,
                          bytes);

            return STATUS_DEVICE_NOT_READY;
        }

        bytes = Bus->GetBusData(Bus->Context,
                                PCI_WHICHSPACE_CONFIG,
                                &value,
                                DataOffset,
                                sizeof(value));
        if (bytes != sizeof(value))
        {
            KphTracePrint(TRACE_LEVEL_VERBOSE,
                          HARDWARE,
                          "Data read at 0x%lx failed: %lu bytes",
                          (ULONG)DataOffset,
                          bytes);

            return STATUS_DEVICE_NOT_READY;
        }

        bytes = Bus->GetBusData(Bus->Context,
                                PCI_WHICHSPACE_CONFIG,
                                &check,
                                IndexOffset,
                                sizeof(check));
        if (bytes != sizeof(check))
        {
            KphTracePrint(TRACE_LEVEL_VERBOSE,
                          HARDWARE,
                          "Index read at 0x%lx failed: %lu bytes",
                          (ULONG)IndexOffset,
                          bytes);

            return STATUS_DEVICE_NOT_READY;
        }

        if (check == Target)
        {
            *Value = value;
            return STATUS_SUCCESS;
        }
    }

    KphTracePrint(TRACE_LEVEL_VERBOSE,
                  HARDWARE,
                  "Index 0x%lx moved to 0x%lx",
                  Target,
                  check);

    return STATUS_RETRY;
}

/**
 * \brief Reads an allow-listed register through a PCI index/data pair.
 *
 * \param[in] Locator The locator of the device holding the index/data pair.
 * \param[in] Target The index register value.
 * \param[out] Value Receives the data register value.
 * \param[in] AccessMode The mode in which to perform access checks.
 *
 * \return Successful or errant status.
 */
_IRQL_requires_max_(PASSIVE_LEVEL)
_Must_inspect_result_
NTSTATUS KphHwReadPciIndirect(
    _In_ PKPH_HW_PCI_LOCATOR Locator,
    _In_ ULONG Target,
    _Out_ PULONG Value,
    _In_ KPROCESSOR_MODE AccessMode
    )
{
#if defined(_M_X64)
    NTSTATUS status;
    KPH_HW_PCI_LOCATOR locator;
    PKPH_DYN dyn;
    BOOLEAN locked;
    PKPH_HW_PCI_DEVICE device;
    KPH_HW_PCI_BUS bus;
    PKPH_DYN_HW_TABLE table;
    PKPH_DYN_HW_OP op;
    PKPH_DYN_HW_PCI_INDIRECT_READ indirect;
    BOOLEAN readFailed;
    ULONG value;

    KPH_PAGED_CODE_PASSIVE();

    dyn = NULL;
    locked = FALSE;
    device = NULL;
    RtlZeroMemory(&bus, sizeof(bus));
    readFailed = FALSE;
    value = 0;

    locator = *Locator;

    if ((Target & 3) != 0)
    {
        KphTracePrint(TRACE_LEVEL_VERBOSE,
                      HARDWARE,
                      "Unaligned PCI indirect target 0x%lx",
                      Target);

        status = STATUS_INVALID_PARAMETER;
        goto Exit;
    }

    if (!KphpHwPciValidLocator(&locator))
    {
        KphTracePrint(TRACE_LEVEL_VERBOSE, HARDWARE, "Invalid PCI locator");

        status = STATUS_INVALID_PARAMETER;
        goto Exit;
    }

    dyn = KphReferenceDynData();
    if (!dyn || !dyn->Hardware.Section)
    {
        status = STATUS_NOINTERFACE;
        goto Exit;
    }

    KphAcquireRWLockExclusive(&KphpHwPciLock);
    locked = TRUE;

    status = KphpHwPciOpen(&locator,
                           dyn->Hardware.Section,
                           FALSE,
                           &device,
                           &bus,
                           &table);
    if (!NT_SUCCESS(status))
    {
        KphTracePrint(TRACE_LEVEL_VERBOSE,
                      HARDWARE,
                      "KphpHwPciOpen failed: %!STATUS!",
                      status);

        goto Exit;
    }

    status = KphDynHwFindPciIndirectOp(table, &op);
    if (!NT_SUCCESS(status))
    {
        KphTracePrint(TRACE_LEVEL_VERBOSE,
                      HARDWARE,
                      "KphDynHwFindPciIndirectOp failed: %!STATUS!",
                      status);

        goto Exit;
    }

    indirect = (PKPH_DYN_HW_PCI_INDIRECT_READ)op->Body;

    if ((locator.Device != indirect->PinDevice) ||
        (locator.Function != indirect->PinFunction))
    {
        KphTracePrint(TRACE_LEVEL_VERBOSE,
                      HARDWARE,
                      "PCI indirect pair is pinned to %lu.%lu",
                      (ULONG)indirect->PinDevice,
                      (ULONG)indirect->PinFunction);

        status = STATUS_ACCESS_DENIED;
        goto Exit;
    }

    if (!KphDynHwIndirectTargetAllowed(op, Target))
    {
        KphTracePrint(TRACE_LEVEL_VERBOSE,
                      HARDWARE,
                      "PCI indirect target 0x%lx not allowed",
                      Target);

        status = STATUS_ACCESS_DENIED;
        goto Exit;
    }

    status = KphpHwPciIndirectCycle(&bus.Interface,
                                    indirect->IndexOffset,
                                    indirect->DataOffset,
                                    Target,
                                    &value);
    if (!NT_SUCCESS(status))
    {
        KphTracePrint(TRACE_LEVEL_VERBOSE,
                      HARDWARE,
                      "KphpHwPciIndirectCycle failed: %!STATUS!",
                      status);

        readFailed = (status == STATUS_DEVICE_NOT_READY);
        goto Exit;
    }

    if (value == MAXULONG)
    {
        KphTracePrint(TRACE_LEVEL_VERBOSE,
                      HARDWARE,
                      "PCI indirect target 0x%lx read all-ones",
                      Target);

        readFailed = TRUE;
        status = STATUS_DEVICE_NOT_READY;
        goto Exit;
    }

    KphpHwPciRelease(&bus);

    KphReleaseRWLock(&KphpHwPciLock);
    locked = FALSE;

    status = KphWriteULongToMode(Value, value, AccessMode);
    if (!NT_SUCCESS(status))
    {
        goto Exit;
    }

    status = STATUS_SUCCESS;

Exit:

    if (locked)
    {
        if (readFailed)
        {
            NT_ASSERT(device);

            KphpHwPciReleaseBus(&bus);
            KphpHwPciClose(device);
        }

        KphpHwPciRelease(&bus);
        KphReleaseRWLock(&KphpHwPciLock);
    }

    if (dyn)
    {
        KphDereferenceObject(dyn);
    }

    return status;
#else
    KPH_PAGED_CODE_PASSIVE();

    UNREFERENCED_PARAMETER(Locator);
    UNREFERENCED_PARAMETER(Target);
    UNREFERENCED_PARAMETER(Value);
    UNREFERENCED_PARAMETER(AccessMode);

    return STATUS_NOT_SUPPORTED;
#endif
}

/**
 * \brief Locates a memory BAR and reads its base.
 *
 * \param[in] Device The device.
 * \param[in] Bus The bus interface of the device.
 * \param[in] BarIndex The index of the BAR.
 * \param[out] Base Receives the base address, untouched on failure.
 * \param[in,out] ReadFailed Set TRUE when a read failed or read all-ones.
 *
 * \return Successful or errant status.
 */
_IRQL_requires_max_(PASSIVE_LEVEL)
_Requires_lock_held_(KphpHwPciLock)
_Must_inspect_result_
NTSTATUS KphpHwPciLocateMemoryBar(
    _In_ PKPH_HW_PCI_DEVICE Device,
    _In_ PBUS_INTERFACE_STANDARD Bus,
    _In_ UCHAR BarIndex,
    _Out_ PULONG64 Base,
    _Inout_ PBOOLEAN ReadFailed
    )
{
    ULONG count;
    ULONG index;

    KPH_PAGED_CODE_PASSIVE();

    switch (Device->Ids.HeaderType)
    {
        case PCI_DEVICE_TYPE:
        {
            count = PCI_TYPE0_ADDRESSES;
            break;
        }
        case PCI_BRIDGE_TYPE:
        {
            count = PCI_TYPE1_ADDRESSES;
            break;
        }
        default:
        {
            KphTracePrint(TRACE_LEVEL_VERBOSE,
                          HARDWARE,
                          "PCI header type %lu has no BARs",
                          (ULONG)Device->Ids.HeaderType);

            return STATUS_DEVICE_CONFIGURATION_ERROR;
        }
    }

    index = 0;

    while ((index < count) && (index <= BarIndex))
    {
        ULONG offset;
        ULONG low;
        ULONG high;

        offset = (FIELD_OFFSET(PCI_COMMON_HEADER, u.type0.BaseAddresses) +
                  (index * sizeof(ULONG)));

        if (!KphpHwPciReadConfig(Bus, offset, &low, sizeof(low)) ||
            (low == MAXULONG))
        {
            KphTracePrint(TRACE_LEVEL_VERBOSE,
                          HARDWARE,
                          "PCI BAR %lu read failed",
                          index);

            *ReadFailed = TRUE;
            return STATUS_DEVICE_NOT_READY;
        }

        if (BooleanFlagOn(low, PCI_ADDRESS_IO_SPACE) ||
            ((low & PCI_ADDRESS_MEMORY_TYPE_MASK) != PCI_TYPE_64BIT))
        {
            if (index == BarIndex)
            {
                if (BooleanFlagOn(low, PCI_ADDRESS_IO_SPACE) ||
                    ((low & PCI_ADDRESS_MEMORY_TYPE_MASK) != PCI_TYPE_32BIT))
                {
                    KphTracePrint(TRACE_LEVEL_VERBOSE,
                                  HARDWARE,
                                  "PCI BAR %lu is not a memory BAR (0x%lx)",
                                  index,
                                  low);

                    return STATUS_DEVICE_CONFIGURATION_ERROR;
                }

                *Base = (low & PCI_ADDRESS_MEMORY_ADDRESS_MASK);
                return STATUS_SUCCESS;
            }

            index++;
            continue;
        }

        if (((index + 1) >= count) || ((index + 1) == BarIndex))
        {
            KphTracePrint(TRACE_LEVEL_VERBOSE,
                          HARDWARE,
                          "PCI BAR %lu is the upper half of BAR %lu",
                          (ULONG)BarIndex,
                          index);

            return STATUS_DEVICE_CONFIGURATION_ERROR;
        }

        if (index == BarIndex)
        {
            if (!KphpHwPciReadConfig(Bus,
                                     offset + sizeof(ULONG),
                                     &high,
                                     sizeof(high)) ||
                (high == MAXULONG))
            {
                KphTracePrint(TRACE_LEVEL_VERBOSE,
                              HARDWARE,
                              "PCI BAR %lu upper half read failed",
                              index);

                *ReadFailed = TRUE;
                return STATUS_DEVICE_NOT_READY;
            }

            *Base = (((ULONG64)high << 32) |
                     (low & PCI_ADDRESS_MEMORY_ADDRESS_MASK));

            return STATUS_SUCCESS;
        }

        index += 2;
    }

    KphTracePrint(TRACE_LEVEL_VERBOSE,
                  HARDWARE,
                  "PCI BAR %lu out of %lu",
                  (ULONG)BarIndex,
                  count);

    return STATUS_DEVICE_CONFIGURATION_ERROR;
}

/**
 * \brief Finds the boot configuration length of the memory range at a base.
 *
 * \param[in] Pdo The physical device object.
 * \param[in] Base The raw base of the BAR.
 * \param[out] Length Receives the length of the range, untouched on failure.
 *
 * \return Successful or errant status.
 */
_IRQL_requires_max_(PASSIVE_LEVEL)
_Must_inspect_result_
NTSTATUS KphpHwPciBootRangeLength(
    _In_ PDEVICE_OBJECT Pdo,
    _In_ ULONG64 Base,
    _Out_ PULONG64 Length
    )
{
    NTSTATUS status;
    PCM_RESOURCE_LIST raw;
    PCM_RESOURCE_LIST translated;
    ULONG rawLength;
    ULONG translatedLength;
    ULONG ordinal;
    ULONG64 start;
    ULONG64 length;
    ULONG64 translatedLengthOfRange;

    KPH_PAGED_CODE_PASSIVE();

    raw = NULL;
    translated = NULL;

    status = KphHwQueryBootConfiguration(Pdo,
                                         DevicePropertyBootConfiguration,
                                         &raw,
                                         &rawLength);
    if (!NT_SUCCESS(status))
    {
        KphTracePrint(TRACE_LEVEL_VERBOSE,
                      HARDWARE,
                      "KphHwQueryBootConfiguration failed: %!STATUS!",
                      status);

        goto Exit;
    }

    status = KphHwQueryBootConfiguration(
                              Pdo,
                              DevicePropertyBootConfigurationTranslated,
                              &translated,
                              &translatedLength);
    if (!NT_SUCCESS(status))
    {
        KphTracePrint(TRACE_LEVEL_VERBOSE,
                      HARDWARE,
                      "KphHwQueryBootConfiguration failed: %!STATUS!",
                      status);

        goto Exit;
    }

    ordinal = 0;
    start = Base;

    if (!KphHwFindMemoryDescriptor(raw,
                                   rawLength,
                                   KPH_HW_FIND_MEMORY_BY_START,
                                   &ordinal,
                                   &start,
                                   &length,
                                   NULL))
    {
        KphTracePrint(TRACE_LEVEL_VERBOSE,
                      HARDWARE,
                      "No boot memory range starts at 0x%I64x",
                      Base);

        status = STATUS_DEVICE_CONFIGURATION_ERROR;
        goto Exit;
    }

    if (!KphHwFindMemoryDescriptor(translated,
                                   translatedLength,
                                   KPH_HW_FIND_MEMORY_BY_ORDINAL,
                                   &ordinal,
                                   &start,
                                   &translatedLengthOfRange,
                                   NULL))
    {
        KphTracePrint(TRACE_LEVEL_VERBOSE,
                      HARDWARE,
                      "No translated boot memory range %lu",
                      ordinal);

        status = STATUS_DEVICE_CONFIGURATION_ERROR;
        goto Exit;
    }

    if ((start != Base) || (translatedLengthOfRange != length))
    {
        KphTracePrint(TRACE_LEVEL_VERBOSE,
                      HARDWARE,
                      "Boot memory range 0x%I64x+0x%I64x translates to "
                      "0x%I64x+0x%I64x",
                      Base,
                      length,
                      start,
                      translatedLengthOfRange);

        status = STATUS_DEVICE_CONFIGURATION_ERROR;
        goto Exit;
    }

    *Length = length;
    status = STATUS_SUCCESS;

Exit:

    if (translated)
    {
        KphFree(translated, KPH_TAG_HW_BOOT_CONFIG);
    }

    if (raw)
    {
        KphFree(raw, KPH_TAG_HW_BOOT_CONFIG);
    }

    return status;
}

/**
 * \brief Checks that a device is in D0.
 *
 * \param[in,out] Device The device.
 * \param[in] Bus The bus interface of the device.
 * \param[in,out] ReadFailed Set TRUE when a read failed or read all-ones.
 *
 * \return Successful or errant status.
 */
_IRQL_requires_max_(PASSIVE_LEVEL)
_Requires_lock_held_(KphpHwPciLock)
_Must_inspect_result_
NTSTATUS KphpHwPciCheckD0(
    _Inout_ PKPH_HW_PCI_DEVICE Device,
    _In_ PBUS_INTERFACE_STANDARD Bus,
    _Inout_ PBOOLEAN ReadFailed
    )
{
    NTSTATUS status;
    ULONG pmcsrOffset;
    KPH_HW_PCI_PMCSR pmcsr;

    KPH_PAGED_CODE_PASSIVE();

    status = KphpHwPciEnsureCapabilities(Device, Bus);
    if (!NT_SUCCESS(status))
    {
        KphTracePrint(TRACE_LEVEL_VERBOSE,
                      HARDWARE,
                      "KphpHwPciEnsureCapabilities failed: %!STATUS!",
                      status);

        *ReadFailed = TRUE;
        return status;
    }

    if (!Device->CapabilitiesValid)
    {
        KphTracePrint(TRACE_LEVEL_VERBOSE,
                      HARDWARE,
                      "PCI %04lx:%04lx capability chain is malformed",
                      (ULONG)Device->Ids.VendorId,
                      (ULONG)Device->Ids.DeviceId);

        return STATUS_DEVICE_CONFIGURATION_ERROR;
    }

    if (!Device->PmCapability)
    {
        return STATUS_SUCCESS;
    }

    NT_VERIFY(NT_SUCCESS(RtlULongAdd(Device->PmCapability,
                                     FIELD_OFFSET(PCI_PM_CAPABILITY, PMCSR),
                                     &pmcsrOffset)));

    if (!KphpHwPciReadConfig(Bus,
                             pmcsrOffset,
                             &pmcsr.Value,
                             sizeof(pmcsr.Value)) ||
        (pmcsr.Value == MAXUSHORT))
    {
        KphTracePrint(TRACE_LEVEL_VERBOSE,
                      HARDWARE,
                      "PCI %04lx:%04lx PMCSR read failed",
                      (ULONG)Device->Ids.VendorId,
                      (ULONG)Device->Ids.DeviceId);

        *ReadFailed = TRUE;
        return STATUS_DEVICE_NOT_READY;
    }

    if (pmcsr.PowerState != KPH_HW_PCI_POWER_STATE_D0)
    {
        KphTracePrint(TRACE_LEVEL_VERBOSE,
                      HARDWARE,
                      "PCI %04lx:%04lx is in D%lu",
                      (ULONG)Device->Ids.VendorId,
                      (ULONG)Device->Ids.DeviceId,
                      (ULONG)pmcsr.PowerState);

        return STATUS_DEVICE_POWERED_OFF;
    }

    return STATUS_SUCCESS;
}

/**
 * \brief Derives the base and length of a memory BAR.
 *
 * \param[in,out] Device The device.
 * \param[in] Bus The bus interface of the device.
 * \param[in] BarIndex The index of the BAR.
 * \param[out] Base Receives the base, untouched on failure.
 * \param[out] Length Receives the length, untouched on failure.
 * \param[in,out] ReadFailed Set TRUE when a read failed or read all-ones.
 *
 * \return Successful or errant status.
 */
_IRQL_requires_max_(PASSIVE_LEVEL)
_Requires_lock_held_(KphpHwPciLock)
_Must_inspect_result_
NTSTATUS KphpHwPciDeriveMemoryBar(
    _Inout_ PKPH_HW_PCI_DEVICE Device,
    _In_ PBUS_INTERFACE_STANDARD Bus,
    _In_ UCHAR BarIndex,
    _Out_ PULONG64 Base,
    _Out_ PULONG64 Length,
    _Inout_ PBOOLEAN ReadFailed
    )
{
    NTSTATUS status;
    USHORT command;
    ULONG64 base;
    ULONG64 length;

    KPH_PAGED_CODE_PASSIVE();

    if (!KphpHwPciReadConfig(Bus,
                             FIELD_OFFSET(PCI_COMMON_HEADER, Command),
                             &command,
                             sizeof(command)) ||
        (command == MAXUSHORT))
    {
        KphTracePrint(TRACE_LEVEL_VERBOSE,
                      HARDWARE,
                      "PCI %04lx:%04lx command read failed",
                      (ULONG)Device->Ids.VendorId,
                      (ULONG)Device->Ids.DeviceId);

        *ReadFailed = TRUE;
        return STATUS_DEVICE_NOT_READY;
    }

    if (!BooleanFlagOn(command, PCI_ENABLE_MEMORY_SPACE))
    {
        KphTracePrint(TRACE_LEVEL_VERBOSE,
                      HARDWARE,
                      "PCI %04lx:%04lx does not decode memory",
                      (ULONG)Device->Ids.VendorId,
                      (ULONG)Device->Ids.DeviceId);

        return STATUS_DEVICE_NOT_READY;
    }

    status = KphpHwPciLocateMemoryBar(Device, Bus, BarIndex, &base, ReadFailed);
    if (!NT_SUCCESS(status))
    {
        KphTracePrint(TRACE_LEVEL_VERBOSE,
                      HARDWARE,
                      "KphpHwPciLocateMemoryBar failed: %!STATUS!",
                      status);

        return status;
    }

    if (!base)
    {
        KphTracePrint(TRACE_LEVEL_VERBOSE,
                      HARDWARE,
                      "PCI %04lx:%04lx BAR %lu is not assigned",
                      (ULONG)Device->Ids.VendorId,
                      (ULONG)Device->Ids.DeviceId,
                      (ULONG)BarIndex);

        return STATUS_DEVICE_NOT_READY;
    }

    NT_ASSERT(BarIndex < ARRAYSIZE(Device->BarBase));

    if (Device->BarBase[BarIndex] != base)
    {
        status = KphpHwPciBootRangeLength(Device->Pdo, base, &length);
        if (!NT_SUCCESS(status))
        {
            KphTracePrint(TRACE_LEVEL_VERBOSE,
                          HARDWARE,
                          "PCI %04lx:%04lx BAR %lu length unknown: %!STATUS!",
                          (ULONG)Device->Ids.VendorId,
                          (ULONG)Device->Ids.DeviceId,
                          (ULONG)BarIndex,
                          status);

            return STATUS_DEVICE_CONFIGURATION_ERROR;
        }

        Device->BarBase[BarIndex] = base;
        Device->BarLength[BarIndex] = length;
    }

    *Base = base;
    *Length = Device->BarLength[BarIndex];

    return STATUS_SUCCESS;
}

/**
 * \brief Derives the AMD PMC aperture base from two SMN registers.
 *
 * \param[in] Bus The bus interface of the root complex.
 * \param[in] IndexOffset The configuration offset of the index register.
 * \param[in] DataOffset The configuration offset of the data register.
 * \param[in] Low The SMN address of the base low register.
 * \param[in] High The SMN address of the base high register.
 * \param[in] Offset The offset in the aperture.
 * \param[out] Base Receives the aperture base plus Offset, untouched on
 * failure.
 * \param[in,out] ReadFailed Set TRUE when a read failed or read all-ones.
 *
 * \return Successful or errant status.
 */
_IRQL_requires_max_(PASSIVE_LEVEL)
_Requires_lock_held_(KphpHwPciLock)
_Must_inspect_result_
NTSTATUS KphpHwPciDeriveSmnAperture(
    _In_ PBUS_INTERFACE_STANDARD Bus,
    _In_ USHORT IndexOffset,
    _In_ USHORT DataOffset,
    _In_ ULONG Low,
    _In_ ULONG High,
    _In_ ULONG64 Offset,
    _Out_ PULONG64 Base,
    _Inout_ PBOOLEAN ReadFailed
    )
{
    NTSTATUS status;
    ULONG low;
    ULONG high;
    ULONG64 base;

    KPH_PAGED_CODE_PASSIVE();
    NT_ASSERT(Offset < KPH_DYN_HW_SMN_APERTURE_SIZE);

    low = 0;
    high = 0;

    status = KphpHwPciIndirectCycle(Bus, IndexOffset, DataOffset, Low, &low);
    if (!NT_SUCCESS(status))
    {
        KphTracePrint(TRACE_LEVEL_VERBOSE,
                      HARDWARE,
                      "KphpHwPciIndirectCycle failed: %!STATUS!",
                      status);

        if (status == STATUS_DEVICE_NOT_READY)
        {
            *ReadFailed = TRUE;
        }

        return status;
    }

    status = KphpHwPciIndirectCycle(Bus, IndexOffset, DataOffset, High, &high);
    if (!NT_SUCCESS(status))
    {
        KphTracePrint(TRACE_LEVEL_VERBOSE,
                      HARDWARE,
                      "KphpHwPciIndirectCycle failed: %!STATUS!",
                      status);

        if (status == STATUS_DEVICE_NOT_READY)
        {
            *ReadFailed = TRUE;
        }

        return status;
    }

    if ((low == MAXULONG) || (high == MAXULONG))
    {
        KphTracePrint(TRACE_LEVEL_VERBOSE,
                      HARDWARE,
                      "SMN aperture registers read all-ones");

        *ReadFailed = TRUE;
        return STATUS_DEVICE_NOT_READY;
    }

    base = (((ULONG64)(high & KPH_HW_SMN_APERTURE_HIGH) << 32) |
            (low & KPH_HW_SMN_APERTURE_LOW));

    if (!base)
    {
        KphTracePrint(TRACE_LEVEL_VERBOSE,
                      HARDWARE,
                      "SMN aperture is not assigned");

        return STATUS_DEVICE_NOT_READY;
    }

    *Base = (base + Offset);

    return STATUS_SUCCESS;
}

/**
 * \brief Derives the physical base of an allow-listed window of a device.
 *
 * \param[in,out] Device The device.
 * \param[in] Bus The bus interface of the device.
 * \param[in] Table The op table of the device.
 * \param[in] Op The window_read op.
 * \param[out] Base Receives the base, aligned to the window size, untouched
 * on failure.
 * \param[in,out] ReadFailed Set TRUE when a read failed or read all-ones.
 *
 * \return Successful or errant status.
 */
_IRQL_requires_max_(PASSIVE_LEVEL)
_Requires_lock_held_(KphpHwPciLock)
_Must_inspect_result_
NTSTATUS KphpHwPciDeriveWindowBase(
    _Inout_ PKPH_HW_PCI_DEVICE Device,
    _In_ PBUS_INTERFACE_STANDARD Bus,
    _In_ PKPH_DYN_HW_TABLE Table,
    _In_ PKPH_DYN_HW_OP Op,
    _Out_ PULONG64 Base,
    _Inout_ PBOOLEAN ReadFailed
    )
{
    NTSTATUS status;
    PKPH_DYN_HW_WINDOW_READ window;
    ULONG64 base;

    KPH_PAGED_CODE_PASSIVE();

    window = (PKPH_DYN_HW_WINDOW_READ)Op->Body;

    if (BooleanFlagOn(Op->Flags, KPH_DYN_HW_FLAG_REQUIRE_D0))
    {
        status = KphpHwPciCheckD0(Device, Bus, ReadFailed);
        if (!NT_SUCCESS(status))
        {
            KphTracePrint(TRACE_LEVEL_VERBOSE,
                          HARDWARE,
                          "KphpHwPciCheckD0 failed: %!STATUS!",
                          status);

            return status;
        }
    }

    if (window->BaseKind == KPH_DYN_HW_WINDOW_BASE_STANDARD_BAR)
    {
        ULONG64 length;

        status = KphpHwPciDeriveMemoryBar(Device,
                                          Bus,
                                          window->BarIndex,
                                          &base,
                                          &length,
                                          ReadFailed);
        if (!NT_SUCCESS(status))
        {
            KphTracePrint(TRACE_LEVEL_VERBOSE,
                          HARDWARE,
                          "KphpHwPciDeriveMemoryBar failed: %!STATUS!",
                          status);

            return status;
        }

        if (length < window->WindowSize)
        {
            KphTracePrint(TRACE_LEVEL_VERBOSE,
                          HARDWARE,
                          "BAR length 0x%I64x below window size 0x%lx",
                          length,
                          window->WindowSize);

            return STATUS_DEVICE_CONFIGURATION_ERROR;
        }
    }
    else if (window->BaseKind == KPH_DYN_HW_WINDOW_BASE_SMN_DERIVED)
    {
        PKPH_DYN_HW_OP indirectOp;
        PKPH_DYN_HW_PCI_INDIRECT_READ indirect;

        NT_VERIFY(NT_SUCCESS(KphDynHwFindPciIndirectOp(Table, &indirectOp)));

        indirect = (PKPH_DYN_HW_PCI_INDIRECT_READ)indirectOp->Body;

        if ((Device->Locator.Device != indirect->PinDevice) ||
            (Device->Locator.Function != indirect->PinFunction))
        {
            KphTracePrint(TRACE_LEVEL_VERBOSE,
                          HARDWARE,
                          "PCI indirect pair is pinned to %lu.%lu",
                          (ULONG)indirect->PinDevice,
                          (ULONG)indirect->PinFunction);

            return STATUS_ACCESS_DENIED;
        }

        status = KphpHwPciDeriveSmnAperture(Bus,
                                            indirect->IndexOffset,
                                            indirect->DataOffset,
                                            KphDynHwWindowAux(Op, 0),
                                            KphDynHwWindowAux(Op, 1),
                                            window->FixedBase,
                                            &base,
                                            ReadFailed);
        if (!NT_SUCCESS(status))
        {
            KphTracePrint(TRACE_LEVEL_VERBOSE,
                          HARDWARE,
                          "KphpHwPciDeriveSmnAperture failed: %!STATUS!",
                          status);

            return status;
        }
    }
    else
    {
        ULONG64 value;
        ULONG width;

        NT_ASSERT(window->BaseKind == KPH_DYN_HW_WINDOW_BASE_CONFIG_REG);

        value = 0;
        width = (window->Is64 ? sizeof(ULONG64) : sizeof(ULONG));

        if (!KphpHwPciReadConfig(Bus, window->ConfigOffset, &value, width) ||
            (value == (window->Is64 ? MAXULONG64 : MAXULONG)))
        {
            KphTracePrint(TRACE_LEVEL_VERBOSE,
                          HARDWARE,
                          "Window base register 0x%lx read failed",
                          (ULONG)window->ConfigOffset);

            *ReadFailed = TRUE;
            return STATUS_DEVICE_NOT_READY;
        }

        if ((window->EnableBit != KPH_DYN_HW_WINDOW_ENABLE_NONE) &&
            !BooleanFlagOn(value, 1ull << window->EnableBit))
        {
            KphTracePrint(TRACE_LEVEL_VERBOSE,
                          HARDWARE,
                          "Window base register 0x%lx is disabled: 0x%I64x",
                          (ULONG)window->ConfigOffset,
                          value);

            return STATUS_DEVICE_NOT_READY;
        }

        base = (value & window->ConfigMask);

        if (!base)
        {
            KphTracePrint(TRACE_LEVEL_VERBOSE,
                          HARDWARE,
                          "Window base register 0x%lx is not assigned",
                          (ULONG)window->ConfigOffset);

            return STATUS_DEVICE_NOT_READY;
        }
    }

    if ((base & (window->WindowSize - 1)) != 0)
    {
        KphTracePrint(TRACE_LEVEL_VERBOSE,
                      HARDWARE,
                      "Window base 0x%I64x is not aligned to 0x%lx",
                      base,
                      window->WindowSize);

        return STATUS_DEVICE_CONFIGURATION_ERROR;
    }

    *Base = base;

    return STATUS_SUCCESS;
}

/**
 * \brief Reads a batch of allow-listed registers of a device window.
 *
 * \param[in] Locator The locator of the device.
 * \param[in] Section The hardware section.
 * \param[in] OpIndex The index of the op among the window_read ops.
 * \param[in] Count The number of entries.
 * \param[in,out] Entries The batch entries, in system memory.
 *
 * \return STATUS_SUCCESS when the batch was processed, otherwise an errant
 * status for the whole batch.
 */
_IRQL_requires_max_(PASSIVE_LEVEL)
_Must_inspect_result_
NTSTATUS KphHwPciReadWindow(
    _In_ PKPH_HW_PCI_LOCATOR Locator,
    _In_ PKPH_DYN_HW_SECTION Section,
    _In_ ULONG OpIndex,
    _In_ ULONG Count,
    _Inout_updates_(Count) PKPH_HW_WINDOW_BATCH_ENTRY Entries
    )
{
    NTSTATUS status;
    KPH_HW_PCI_LOCATOR locator;
    PKPH_HW_PCI_DEVICE device;
    KPH_HW_PCI_BUS bus;
    PKPH_DYN_HW_TABLE table;
    PKPH_DYN_HW_OP op;
    BOOLEAN readFailed;
    ULONG spanBegin;
    ULONG spanEnd;
    ULONG64 base;
    ULONG64 check;

    KPH_PAGED_CODE_PASSIVE();

    locator = *Locator;

    if (!KphpHwPciValidLocator(&locator))
    {
        KphTracePrint(TRACE_LEVEL_VERBOSE, HARDWARE, "Invalid PCI locator");

        return STATUS_INVALID_PARAMETER;
    }

    device = NULL;
    readFailed = FALSE;
    RtlZeroMemory(&bus, sizeof(bus));

    KphAcquireRWLockExclusive(&KphpHwPciLock);

    status = KphpHwPciOpen(&locator, Section, TRUE, &device, &bus, &table);
    if (!NT_SUCCESS(status))
    {
        KphTracePrint(TRACE_LEVEL_VERBOSE,
                      HARDWARE,
                      "KphpHwPciOpen failed: %!STATUS!",
                      status);

        goto Exit;
    }

    //
    // An uncached device may be removed; only cached devices are served.
    //
    if (device == &KphpHwPciUncached)
    {
        KphTracePrint(TRACE_LEVEL_VERBOSE,
                      HARDWARE,
                      "PCI %04lx:%04lx window refused: removal policy is not "
                      "ExpectNoRemoval",
                      (ULONG)device->Ids.VendorId,
                      (ULONG)device->Ids.DeviceId);

        status = STATUS_NOT_SUPPORTED;
        goto Exit;
    }

    status = KphDynHwFindWindowOp(table, OpIndex, &op);
    if (!NT_SUCCESS(status))
    {
        KphTracePrint(TRACE_LEVEL_VERBOSE,
                      HARDWARE,
                      "KphDynHwFindWindowOp failed: %!STATUS!",
                      status);

        goto Exit;
    }

    if (!KphHwWindowValidateEntries(op, Count, Entries, &spanBegin, &spanEnd))
    {
        KphTracePrint(TRACE_LEVEL_VERBOSE,
                      HARDWARE,
                      "No window entry is allowed");

        status = STATUS_INVALID_PARAMETER;
        goto Exit;
    }

    status = KphpHwPciDeriveWindowBase(device,
                                       &bus.Interface,
                                       table,
                                       op,
                                       &base,
                                       &readFailed);
    if (!NT_SUCCESS(status))
    {
        KphTracePrint(TRACE_LEVEL_VERBOSE,
                      HARDWARE,
                      "KphpHwPciDeriveWindowBase failed: %!STATUS!",
                      status);

        goto Exit;
    }

    status = KphHwWindowMapAndRead(op,
                                   base,
                                   spanBegin,
                                   spanEnd,
                                   Count,
                                   Entries);
    if (!NT_SUCCESS(status))
    {
        KphTracePrint(TRACE_LEVEL_VERBOSE,
                      HARDWARE,
                      "KphHwWindowMapAndRead failed: %!STATUS!",
                      status);

        goto Exit;
    }

    check = 0;

    status = KphpHwPciDeriveWindowBase(device,
                                       &bus.Interface,
                                       table,
                                       op,
                                       &check,
                                       &readFailed);
    if (!NT_SUCCESS(status) || (check != base))
    {
        KphTracePrint(TRACE_LEVEL_VERBOSE,
                      HARDWARE,
                      "Window base 0x%I64x changed to 0x%I64x: %!STATUS!",
                      base,
                      check,
                      status);

        for (ULONG i = 0; i < Count; i++)
        {
            if (Entries[i].Status == STATUS_SUCCESS)
            {
                Entries[i].Value = 0;
                Entries[i].Status = STATUS_DEVICE_NOT_READY;
            }
        }
    }

    status = STATUS_SUCCESS;

Exit:

    if (readFailed)
    {
        NT_ASSERT(device);

        KphpHwPciReleaseBus(&bus);
        KphpHwPciClose(device);
    }

    KphpHwPciRelease(&bus);

    KphReleaseRWLock(&KphpHwPciLock);

    return status;
}

#if defined(_M_X64)

/**
 * \brief Reads the SMBus host controller registers and resolves its
 * smbus_read op and, for a PIIX4 host, the smbus_host op of its revision.
 *
 * \details Nothing is written. An all-ones read closes the device.
 *
 * \param[in] Locator The locator of the host controller.
 * \param[in] Section The hardware section.
 * \param[in] OpIndex The index of the op among the smbus_read ops.
 * \param[out] Op Receives the op, valid while the section is referenced.
 * \param[out] Host Receives the registers read.
 *
 * \return Successful or errant status.
 */
_IRQL_requires_max_(PASSIVE_LEVEL)
_Must_inspect_result_
NTSTATUS KphHwPciReadSmbusHost(
    _In_ PKPH_HW_PCI_LOCATOR Locator,
    _In_ PKPH_DYN_HW_SECTION Section,
    _In_ ULONG OpIndex,
    _Outptr_ PKPH_DYN_HW_OP* Op,
    _Out_ PKPH_HW_SMBUS_HOST Host
    )
{
    NTSTATUS status;
    KPH_HW_PCI_LOCATOR locator;
    PKPH_HW_PCI_DEVICE device;
    KPH_HW_PCI_BUS bus;
    PKPH_DYN_HW_TABLE table;
    PKPH_DYN_HW_OP op;
    PKPH_DYN_HW_OP hostOp;
    KPH_HW_SMBUS_HOST host;
    BOOLEAN readFailed;
    ULONG barOffset;

    KPH_PAGED_CODE_PASSIVE();

    *Op = NULL;
    RtlZeroMemory(Host, sizeof(KPH_HW_SMBUS_HOST));

    locator = *Locator;

    if (!KphpHwPciValidLocator(&locator))
    {
        KphTracePrint(TRACE_LEVEL_VERBOSE, HARDWARE, "Invalid PCI locator");

        return STATUS_INVALID_PARAMETER;
    }

    device = NULL;
    readFailed = FALSE;
    RtlZeroMemory(&bus, sizeof(bus));
    RtlZeroMemory(&host, sizeof(host));

    KphAcquireRWLockExclusive(&KphpHwPciLock);

    status = KphpHwPciOpen(&locator, Section, TRUE, &device, &bus, &table);
    if (!NT_SUCCESS(status))
    {
        KphTracePrint(TRACE_LEVEL_VERBOSE,
                      HARDWARE,
                      "KphpHwPciOpen failed: %!STATUS!",
                      status);

        goto Exit;
    }

    status = KphDynHwFindSmbusOp(table, OpIndex, &op);
    if (!NT_SUCCESS(status))
    {
        KphTracePrint(TRACE_LEVEL_VERBOSE,
                      HARDWARE,
                      "KphDynHwFindSmbusOp failed: %!STATUS!",
                      status);

        goto Exit;
    }

    host.HostKind = ((PKPH_DYN_HW_SMBUS_READ)op->Body)->HostKind;
    host.VendorId = device->Ids.VendorId;
    host.DeviceId = device->Ids.DeviceId;

    if (host.HostKind == KPH_DYN_HW_SMBUS_HOST_I801)
    {
        if (device->Ids.HeaderType != PCI_DEVICE_TYPE)
        {
            KphTracePrint(TRACE_LEVEL_VERBOSE,
                          HARDWARE,
                          "SMBus host header type %lu",
                          (ULONG)device->Ids.HeaderType);

            status = STATUS_DEVICE_CONFIGURATION_ERROR;
            goto Exit;
        }

        if (!KphpHwPciReadConfig(&bus.Interface,
                                 FIELD_OFFSET(PCI_COMMON_HEADER, Command),
                                 &host.Command,
                                 sizeof(host.Command)) ||
            (host.Command == MAXUSHORT))
        {
            KphTracePrint(TRACE_LEVEL_VERBOSE,
                          HARDWARE,
                          "SMBus host command read failed");

            readFailed = TRUE;
            status = STATUS_DEVICE_NOT_READY;
            goto Exit;
        }

        barOffset = FIELD_OFFSET(PCI_COMMON_HEADER, u.type0.BaseAddresses[4]);

        if (!KphpHwPciReadConfig(&bus.Interface,
                                 barOffset,
                                 &host.Bar,
                                 sizeof(host.Bar)) ||
            (host.Bar == MAXULONG))
        {
            KphTracePrint(TRACE_LEVEL_VERBOSE,
                          HARDWARE,
                          "SMBus host BAR read failed");

            readFailed = TRUE;
            status = STATUS_DEVICE_NOT_READY;
            goto Exit;
        }

        if (!KphpHwPciReadConfig(&bus.Interface,
                                 KPH_HW_SMBUS_I801_HOSTC,
                                 &host.Hostc,
                                 sizeof(host.Hostc)))
        {
            KphTracePrint(TRACE_LEVEL_VERBOSE,
                          HARDWARE,
                          "SMBus host HOSTC read failed");

            readFailed = TRUE;
            status = STATUS_DEVICE_NOT_READY;
            goto Exit;
        }
    }
    else
    {
        NT_ASSERT(host.HostKind == KPH_DYN_HW_SMBUS_HOST_PIIX4);

        if (!KphpHwPciReadConfig(&bus.Interface,
                                 FIELD_OFFSET(PCI_COMMON_HEADER, RevisionID),
                                 &host.Revision,
                                 sizeof(host.Revision)) ||
            (host.Revision == MAXUCHAR))
        {
            KphTracePrint(TRACE_LEVEL_VERBOSE,
                          HARDWARE,
                          "SMBus host revision read failed");

            readFailed = TRUE;
            status = STATUS_DEVICE_NOT_READY;
            goto Exit;
        }

        for (ULONG i = 0;; i++)
        {
            ULONG value;

            status = KphDynHwFindSmbusHostOp(table, i, &hostOp);
            if (!NT_SUCCESS(status))
            {
                KphTracePrint(TRACE_LEVEL_VERBOSE,
                              HARDWARE,
                              "SMBus host revision 0x%02lx has no layout",
                              (ULONG)host.Revision);

                status = STATUS_NOT_SUPPORTED;
                goto Exit;
            }

            NT_ASSERT(hostOp->Gate.Kind == KPH_DYN_HW_GATE_PCI_CONFIG);

            if (!KphpHwPciReadConfig(&bus.Interface,
                                     hostOp->Gate.Register,
                                     &value,
                                     sizeof(value)) ||
                (value == MAXULONG))
            {
                KphTracePrint(TRACE_LEVEL_VERBOSE,
                              HARDWARE,
                              "SMBus host gate register 0x%02lx read failed",
                              (ULONG)hostOp->Gate.Register);

                readFailed = TRUE;
                status = STATUS_DEVICE_NOT_READY;
                goto Exit;
            }

            if (KphDynHwPciConfigFieldInRange(&hostOp->Gate, value))
            {
                host.HostOp = hostOp;
                break;
            }
        }
    }

    *Op = op;
    *Host = host;

    status = STATUS_SUCCESS;

Exit:

    if (readFailed)
    {
        NT_ASSERT(device);

        KphpHwPciReleaseBus(&bus);
        KphpHwPciClose(device);
    }

    KphpHwPciRelease(&bus);

    KphReleaseRWLock(&KphpHwPciLock);

    return status;
}

#endif

/**
 * \brief Checks whether an extended capability is an Intel VSEC or DVSEC with
 * a discovery ID.
 *
 * \param[in] Bus The bus interface of the device.
 * \param[in] Position The configuration offset of the capability.
 * \param[in] Id The extended capability ID.
 * \param[in] Discovery The DVSEC/VSEC ID (KPH_DYN_HW_VSEC_DISCOVERY_*).
 * \param[out] Match Receives TRUE when the capability matches.
 * \param[in,out] ReadFailed Set TRUE when a read failed or read all-ones.
 *
 * \return Successful or errant status.
 */
_IRQL_requires_max_(PASSIVE_LEVEL)
_Must_inspect_result_
NTSTATUS KphpHwPciVsecMatches(
    _In_ PBUS_INTERFACE_STANDARD Bus,
    _In_ ULONG Position,
    _In_ USHORT Id,
    _In_ UCHAR Discovery,
    _Out_ PBOOLEAN Match,
    _Inout_ PBOOLEAN ReadFailed
    )
{
    KPH_HW_PCI_VSEC_HEADER1 header1;
    USHORT dvsecId;
    ULONG vsecEnd;
    ULONG header1Offset;
    ULONG dvsecIdOffset;

    KPH_PAGED_CODE_PASSIVE();

    *Match = FALSE;

    if ((Id != KPH_HW_PCI_EXT_CAP_DVSEC) && (Id != KPH_HW_PCI_EXT_CAP_VSEC))
    {
        return STATUS_SUCCESS;
    }

    NT_VERIFY(NT_SUCCESS(RtlULongAdd(Position,
                                     KPH_HW_PCI_VSEC_HEADER_LENGTH,
                                     &vsecEnd)));
    if (vsecEnd > KPH_DYN_HW_PCI_CONFIG_LENGTH)
    {
        return STATUS_SUCCESS;
    }

    NT_VERIFY(NT_SUCCESS(RtlULongAdd(Position,
                                     KPH_HW_PCI_VSEC_HEADER1_OFFSET,
                                     &header1Offset)));

    if (!KphpHwPciReadConfig(Bus,
                             header1Offset,
                             &header1.Value,
                             sizeof(header1.Value)) ||
        (header1.Value == MAXULONG))
    {
        KphTracePrint(TRACE_LEVEL_VERBOSE,
                      HARDWARE,
                      "VSEC header read failed at 0x%lx",
                      header1Offset);

        *ReadFailed = TRUE;
        return STATUS_DEVICE_NOT_READY;
    }

    if ((header1.Revision != KPH_HW_PCI_VSEC_REVISION) ||
        (header1.Length < KPH_HW_PCI_VSEC_HEADER_LENGTH))
    {
        return STATUS_SUCCESS;
    }

    if (Id == KPH_HW_PCI_EXT_CAP_VSEC)
    {
        *Match = (header1.Id == Discovery);
        return STATUS_SUCCESS;
    }

    if (header1.Id != KPH_HW_PCI_VSEC_INTEL_VENDOR)
    {
        return STATUS_SUCCESS;
    }

    NT_VERIFY(NT_SUCCESS(RtlULongAdd(Position,
                                     KPH_HW_PCI_DVSEC_ID_OFFSET,
                                     &dvsecIdOffset)));

    if (!KphpHwPciReadConfig(Bus, dvsecIdOffset, &dvsecId, sizeof(dvsecId)))
    {
        KphTracePrint(TRACE_LEVEL_VERBOSE,
                      HARDWARE,
                      "DVSEC ID read failed at 0x%lx",
                      dvsecIdOffset);

        *ReadFailed = TRUE;
        return STATUS_DEVICE_NOT_READY;
    }

    *Match = (dvsecId == Discovery);

    return STATUS_SUCCESS;
}

/**
 * \brief Finds the Intel VSEC and DVSEC capabilities with a discovery ID.
 *
 * \param[in] Bus The bus interface of the device.
 * \param[in] Discovery The DVSEC/VSEC ID (KPH_DYN_HW_VSEC_DISCOVERY_*).
 * \param[out] Positions Receives the configuration offsets of the matches.
 * \param[out] Count Receives the number of matches.
 * \param[in,out] ReadFailed Set TRUE when a read failed or read all-ones.
 *
 * \return Successful or errant status.
 */
_IRQL_requires_max_(PASSIVE_LEVEL)
_Must_inspect_result_
NTSTATUS KphpHwPciFindVsecCapabilities(
    _In_ PBUS_INTERFACE_STANDARD Bus,
    _In_ UCHAR Discovery,
    _Out_writes_to_(KPH_HW_PCI_VSEC_MAX_CAPABILITIES, *Count) PULONG Positions,
    _Out_ PULONG Count,
    _Inout_ PBOOLEAN ReadFailed
    )
{
    NTSTATUS status;
    ULONG position;
    ULONG count;

    KPH_PAGED_CODE_PASSIVE();

    *Count = 0;

    position = KPH_HW_PCI_EXTENDED_START;
    count = 0;

    for (ULONG ttl = KPH_HW_PCI_EXTENDED_TTL; ttl; ttl--)
    {
        KPH_HW_PCI_EXT_CAP_HEADER header;
        ULONG next;
        BOOLEAN match;

        if (!KphpHwPciReadConfig(Bus,
                                 position,
                                 &header.Value,
                                 sizeof(header.Value)))
        {
            KphTracePrint(TRACE_LEVEL_VERBOSE,
                          HARDWARE,
                          "Extended capability read failed at 0x%lx",
                          position);

            *ReadFailed = TRUE;
            return STATUS_DEVICE_NOT_READY;
        }

        if (header.Value == MAXULONG)
        {
            if (position == KPH_HW_PCI_EXTENDED_START)
            {
                return STATUS_SUCCESS;
            }

            KphTracePrint(TRACE_LEVEL_VERBOSE,
                          HARDWARE,
                          "Extended capability at 0x%lx read all-ones",
                          position);

            *ReadFailed = TRUE;
            return STATUS_DEVICE_NOT_READY;
        }

        status = KphpHwPciVsecMatches(Bus,
                                      position,
                                      (USHORT)header.Id,
                                      Discovery,
                                      &match,
                                      ReadFailed);
        if (!NT_SUCCESS(status))
        {
            return status;
        }

        if (match && (count < KPH_HW_PCI_VSEC_MAX_CAPABILITIES))
        {
            Positions[count++] = position;
        }

        next = (header.Next & KPH_HW_PCI_EXT_CAP_NEXT_MASK);

        if (!next)
        {
            *Count = count;
            return STATUS_SUCCESS;
        }

        if (next < KPH_HW_PCI_EXTENDED_START)
        {
            KphTracePrint(TRACE_LEVEL_VERBOSE,
                          HARDWARE,
                          "Extended capability at 0x%lx links to 0x%lx",
                          position,
                          next);

            return STATUS_DEVICE_CONFIGURATION_ERROR;
        }

        position = next;
    }

    KphTracePrint(TRACE_LEVEL_VERBOSE,
                  HARDWARE,
                  "Extended capability chain does not end");

    return STATUS_DEVICE_CONFIGURATION_ERROR;
}

/**
 * \brief Records a region located in a BAR of the device.
 *
 * \param[in] BarBase The base of the BAR.
 * \param[in] BarLength The length of the BAR.
 * \param[in] Offset The offset of the region in the BAR.
 * \param[in] Size The size of the region.
 * \param[in] Id The GUID or feature ID of the region.
 * \param[out] Location Receives the region, untouched on failure.
 *
 * \return TRUE if the region lies in the BAR.
 */
_IRQL_requires_max_(PASSIVE_LEVEL)
_Must_inspect_result_
_Success_(return)
BOOLEAN KphpHwPciVsecRegionInBar(
    _In_ ULONG64 BarBase,
    _In_ ULONG64 BarLength,
    _In_ ULONG64 Offset,
    _In_ ULONG Size,
    _In_ ULONG Id,
    _Out_ PKPH_HW_VSEC_LOCATION Location
    )
{
    NTSTATUS status;
    ULONG64 end;
    ULONG64 base;

    KPH_PAGED_CODE_PASSIVE();

    status = RtlULongLongAdd(Offset, Size, &end);
    if (!NT_SUCCESS(status))
    {
        KphTracePrint(TRACE_LEVEL_VERBOSE,
                      HARDWARE,
                      "RtlULongLongAdd failed: %!STATUS!",
                      status);

        return FALSE;
    }

    if (end > BarLength)
    {
        KphTracePrint(TRACE_LEVEL_VERBOSE,
                      HARDWARE,
                      "VSEC region 0x%I64x+0x%lx exceeds BAR length 0x%I64x",
                      Offset,
                      Size,
                      BarLength);

        return FALSE;
    }

    status = RtlULongLongAdd(BarBase, Offset, &base);
    if (!NT_SUCCESS(status))
    {
        KphTracePrint(TRACE_LEVEL_VERBOSE,
                      HARDWARE,
                      "RtlULongLongAdd failed: %!STATUS!",
                      status);

        return FALSE;
    }

    Location->Base = base;
    Location->Size = Size;
    Location->Id = Id;

    return TRUE;
}

typedef struct _KPH_HW_VSEC_BARS
{
    UCHAR Derived;
    NTSTATUS Status[PCI_TYPE0_ADDRESSES];
    ULONG64 Base[PCI_TYPE0_ADDRESSES];
    ULONG64 Length[PCI_TYPE0_ADDRESSES];
} KPH_HW_VSEC_BARS, *PKPH_HW_VSEC_BARS;

/**
 * \brief Derives a memory BAR once per locate.
 *
 * \param[in,out] Device The device.
 * \param[in] Bus The bus interface of the device.
 * \param[in] BarIndex The index of the BAR, below PCI_TYPE0_ADDRESSES.
 * \param[in,out] Bars The BARs derived during this locate.
 * \param[out] Base Receives the base, untouched on failure.
 * \param[out] Length Receives the length, untouched on failure.
 * \param[in,out] ReadFailed Set TRUE when a read failed or read all-ones.
 *
 * \return Successful or errant status.
 */
_IRQL_requires_max_(PASSIVE_LEVEL)
_Requires_lock_held_(KphpHwPciLock)
_Must_inspect_result_
NTSTATUS KphpHwPciVsecDeriveBar(
    _Inout_ PKPH_HW_PCI_DEVICE Device,
    _In_ PBUS_INTERFACE_STANDARD Bus,
    _In_ UCHAR BarIndex,
    _Inout_ PKPH_HW_VSEC_BARS Bars,
    _Out_ PULONG64 Base,
    _Out_ PULONG64 Length,
    _Inout_ PBOOLEAN ReadFailed
    )
{
    NTSTATUS status;
    ULONG64 base;
    ULONG64 length;

    KPH_PAGED_CODE_PASSIVE();
    NT_ASSERT(BarIndex < PCI_TYPE0_ADDRESSES);

    if (!BooleanFlagOn(Bars->Derived, 1 << BarIndex))
    {
        base = 0;
        length = 0;

        status = KphpHwPciDeriveMemoryBar(Device,
                                          Bus,
                                          BarIndex,
                                          &base,
                                          &length,
                                          ReadFailed);
        if (!NT_SUCCESS(status) && *ReadFailed)
        {
            KphTracePrint(TRACE_LEVEL_VERBOSE,
                          HARDWARE,
                          "KphpHwPciDeriveMemoryBar failed: %!STATUS!",
                          status);

            return status;
        }

        SetFlag(Bars->Derived, 1 << BarIndex);
        Bars->Status[BarIndex] = status;
        Bars->Base[BarIndex] = base;
        Bars->Length[BarIndex] = length;
    }

    if (!NT_SUCCESS(Bars->Status[BarIndex]))
    {
        return Bars->Status[BarIndex];
    }

    *Base = Bars->Base[BarIndex];
    *Length = Bars->Length[BarIndex];

    return STATUS_SUCCESS;
}

typedef struct _KPH_HW_VSEC_TABLE
{
    PULONG Entries;
    UCHAR Count;
    ULONG EntryDwords;
    ULONG Stride;
    UCHAR Bar;
    ULONG Offset;
    ULONG64 BarBase;
    ULONG64 BarLength;
} KPH_HW_VSEC_TABLE, *PKPH_HW_VSEC_TABLE;

/**
 * \brief Reads the discovery table of a VSEC or DVSEC capability.
 *
 * \param[in] Locator The locator of the device.
 * \param[in,out] Device The device.
 * \param[in] Bus The bus interface of the device.
 * \param[in] Vsec The vsec_read op body.
 * \param[in] Position The configuration offset of the capability.
 * \param[in] EntryDwords The dwords of an entry the discovery kind reads.
 * \param[in,out] Bars The BARs derived during this locate.
 * \param[out] Table Receives the table; Entries is freed by the caller.
 * \param[in,out] ReadFailed Set TRUE when a read failed or read all-ones.
 *
 * \return Successful or errant status.
 */
_IRQL_requires_max_(PASSIVE_LEVEL)
_Requires_lock_held_(KphpHwPciLock)
_Must_inspect_result_
NTSTATUS KphpHwPciVsecReadTable(
    _In_ PKPH_HW_PCI_LOCATOR Locator,
    _Inout_ PKPH_HW_PCI_DEVICE Device,
    _In_ PBUS_INTERFACE_STANDARD Bus,
    _In_ PKPH_DYN_HW_VSEC_READ Vsec,
    _In_ ULONG Position,
    _In_ ULONG EntryDwords,
    _Inout_ PKPH_HW_VSEC_BARS Bars,
    _Out_ PKPH_HW_VSEC_TABLE Table,
    _Inout_ PBOOLEAN ReadFailed
    )
{
    NTSTATUS status;
    ULONG entriesOffset;
    ULONG entrySizeOffset;
    ULONG tableRegisterOffset;
    UCHAR entries;
    UCHAR entrySize;
    ULONG tableRegister;
    ULONG tableLength;
    ULONG tableEnd;
    ULONG64 tableBase;
    SIZE_T tableDwords;
    SIZE_T tableSize;

    KPH_PAGED_CODE_PASSIVE();

    RtlZeroMemory(Table, sizeof(*Table));

    NT_VERIFY(NT_SUCCESS(RtlULongAdd(Position,
                                     KPH_HW_PCI_VSEC_ENTRIES_OFFSET,
                                     &entriesOffset)));
    NT_VERIFY(NT_SUCCESS(RtlULongAdd(Position,
                                     KPH_HW_PCI_VSEC_ENTRY_SIZE_OFFSET,
                                     &entrySizeOffset)));
    NT_VERIFY(NT_SUCCESS(RtlULongAdd(Position,
                                     KPH_HW_PCI_VSEC_TABLE_OFFSET,
                                     &tableRegisterOffset)));

    if (!KphpHwPciReadConfig(Bus,
                             entriesOffset,
                             &entries,
                             sizeof(entries)) ||
        !KphpHwPciReadConfig(Bus,
                             entrySizeOffset,
                             &entrySize,
                             sizeof(entrySize)) ||
        !KphpHwPciReadConfig(Bus,
                             tableRegisterOffset,
                             &tableRegister,
                             sizeof(tableRegister)) ||
        (tableRegister == MAXULONG))
    {
        KphTracePrint(TRACE_LEVEL_VERBOSE,
                      HARDWARE,
                      "VSEC discovery header read failed at 0x%lx",
                      Position);

        *ReadFailed = TRUE;
        return STATUS_DEVICE_NOT_READY;
    }

    Table->Bar = (UCHAR)(tableRegister & KPH_HW_VSEC_BIR_MASK);
    Table->Offset = (tableRegister & ~KPH_HW_VSEC_BIR_MASK);

    if (BooleanFlagOn(Vsec->Quirks, KPH_DYN_HW_VSEC_QUIRK_TABLE_SHIFT))
    {
        Table->Offset >>= KPH_HW_VSEC_TABLE_SHIFT;
    }

    if (!entries ||
        (entrySize < EntryDwords) ||
        (Table->Bar >= PCI_TYPE0_ADDRESSES) ||
        ((Table->Offset % sizeof(ULONG)) != 0))
    {
        KphTracePrint(TRACE_LEVEL_VERBOSE,
                      HARDWARE,
                      "VSEC discovery table invalid: %lu entries of %lu "
                      "dwords, BAR %lu offset 0x%lx",
                      (ULONG)entries,
                      (ULONG)entrySize,
                      (ULONG)Table->Bar,
                      Table->Offset);

        return STATUS_DEVICE_CONFIGURATION_ERROR;
    }

    status = KphpHwPciVsecDeriveBar(Device,
                                    Bus,
                                    Table->Bar,
                                    Bars,
                                    &Table->BarBase,
                                    &Table->BarLength,
                                    ReadFailed);
    if (!NT_SUCCESS(status))
    {
        KphTracePrint(TRACE_LEVEL_VERBOSE,
                      HARDWARE,
                      "KphpHwPciVsecDeriveBar failed: %!STATUS!",
                      status);

        return status;
    }

    NT_VERIFY(NT_SUCCESS(RtlULongMult(entrySize,
                                      sizeof(ULONG),
                                      &Table->Stride)));
    NT_VERIFY(NT_SUCCESS(RtlULongMult(entries, Table->Stride, &tableLength)));

    status = RtlULongAdd(Table->Offset, tableLength, &tableEnd);
    if (!NT_SUCCESS(status))
    {
        KphTracePrint(TRACE_LEVEL_VERBOSE,
                      HARDWARE,
                      "RtlULongAdd failed: %!STATUS!",
                      status);

        return status;
    }

    if (tableEnd > Table->BarLength)
    {
        KphTracePrint(TRACE_LEVEL_VERBOSE,
                      HARDWARE,
                      "VSEC discovery table end 0x%lx exceeds BAR length "
                      "0x%I64x",
                      tableEnd,
                      Table->BarLength);

        return STATUS_DEVICE_CONFIGURATION_ERROR;
    }

    status = RtlULongLongAdd(Table->BarBase, Table->Offset, &tableBase);
    if (!NT_SUCCESS(status))
    {
        KphTracePrint(TRACE_LEVEL_VERBOSE,
                      HARDWARE,
                      "RtlULongLongAdd failed: %!STATUS!",
                      status);

        return status;
    }

    NT_VERIFY(NT_SUCCESS(RtlSizeTMult(entries, EntryDwords, &tableDwords)));
    NT_VERIFY(NT_SUCCESS(RtlSizeTMult(tableDwords, sizeof(ULONG), &tableSize)));

    Table->Entries = KphAllocatePaged(tableSize, KPH_TAG_HW_VSEC);
    if (!Table->Entries)
    {
        KphTracePrint(TRACE_LEVEL_VERBOSE,
                      HARDWARE,
                      "Failed to allocate VSEC discovery table");

        return STATUS_INSUFFICIENT_RESOURCES;
    }

    status = KphHwVsecCopyTable(Locator,
                                tableBase,
                                entries,
                                Table->Stride,
                                EntryDwords,
                                Table->Entries);
    if (!NT_SUCCESS(status))
    {
        KphTracePrint(TRACE_LEVEL_VERBOSE,
                      HARDWARE,
                      "KphHwVsecCopyTable failed: %!STATUS!",
                      status);

        KphFree(Table->Entries, KPH_TAG_HW_VSEC);
        Table->Entries = NULL;
        return status;
    }

    Table->Count = entries;
    Table->EntryDwords = EntryDwords;

    return STATUS_SUCCESS;
}

/**
 * \brief Locates the region of a PMT telemetry discovery entry.
 *
 * \param[in,out] Device The device.
 * \param[in] Bus The bus interface of the device.
 * \param[in] Vsec The vsec_read op body.
 * \param[in] Table The discovery table.
 * \param[in] Index The index of the entry.
 * \param[in,out] Bars The BARs derived during this locate.
 * \param[out] Location Receives the region.
 * \param[in,out] ReadFailed Set TRUE when a read failed or read all-ones.
 *
 * \return STATUS_SUCCESS when the entry selects a region, STATUS_NOT_FOUND
 * when it does not, or an errant status when a read failed.
 */
_IRQL_requires_max_(PASSIVE_LEVEL)
_Requires_lock_held_(KphpHwPciLock)
_Must_inspect_result_
_Success_(NT_SUCCESS(return))
NTSTATUS KphpHwPciVsecLocatePmt(
    _Inout_ PKPH_HW_PCI_DEVICE Device,
    _In_ PBUS_INTERFACE_STANDARD Bus,
    _In_ PKPH_DYN_HW_VSEC_READ Vsec,
    _In_ PKPH_HW_VSEC_TABLE Table,
    _In_ ULONG Index,
    _Inout_ PKPH_HW_VSEC_BARS Bars,
    _Out_ PKPH_HW_VSEC_LOCATION Location,
    _Inout_ PBOOLEAN ReadFailed
    )
{
    NTSTATUS status;
    PULONG entry;
    KPH_HW_VSEC_PMT_HEADER header;
    ULONG size;
    ULONG guid;
    ULONG baseRegister;
    UCHAR regionBar;
    ULONG64 entryEnd;
    ULONG64 regionOffset;
    ULONG64 regionBarBase;
    ULONG64 regionBarLength;

    KPH_PAGED_CODE_PASSIVE();

    entry = &Table->Entries[Index * Table->EntryDwords];

    header.Value = entry[0];
    size = (header.Size * sizeof(ULONG));
    guid = entry[1];
    baseRegister = entry[2];

    if (!size || (header.AccessType == KPH_HW_VSEC_PMT_ACCESS_RESERVED))
    {
        return STATUS_NOT_FOUND;
    }

    if (BooleanFlagOn(Vsec->Quirks, KPH_DYN_HW_VSEC_QUIRK_EARLY_HW) &&
        ((header.TelemetryType == KPH_HW_VSEC_PMT_TYPE_PUNIT_FIXED) ||
         (guid == KPH_HW_VSEC_PMT_FIXED_BLOCK_GUID)))
    {
        return STATUS_NOT_FOUND;
    }

    if ((Vsec->Guid != KPH_DYN_HW_VSEC_GUID_ANY) && (guid != Vsec->Guid))
    {
        return STATUS_NOT_FOUND;
    }

    if (size > Vsec->MaxRegionSize)
    {
        return STATUS_NOT_FOUND;
    }

    if (header.AccessType == KPH_HW_VSEC_PMT_ACCESS_LOCAL)
    {
        if (baseRegister & KPH_HW_VSEC_BIR_MASK)
        {
            KphTracePrint(TRACE_LEVEL_VERBOSE,
                          HARDWARE,
                          "PMT local entry has BIR bits set: 0x%08lx",
                          baseRegister);

            return STATUS_NOT_FOUND;
        }

        regionBar = Table->Bar;

        if (BooleanFlagOn(Vsec->Quirks, KPH_DYN_HW_VSEC_QUIRK_EARLY_HW))
        {
            regionOffset = baseRegister;
        }
        else
        {
            NT_VERIFY(NT_SUCCESS(RtlULongLongMult(Index + 1ull,
                                                  Table->Stride,
                                                  &entryEnd)));
            NT_VERIFY(NT_SUCCESS(RtlULongLongAdd(Table->Offset,
                                                 entryEnd,
                                                 &regionOffset)));
            NT_VERIFY(NT_SUCCESS(RtlULongLongAdd(regionOffset,
                                                 baseRegister,
                                                 &regionOffset)));
        }
    }
    else if (header.AccessType == KPH_HW_VSEC_PMT_ACCESS_BARID)
    {
        regionBar = (UCHAR)(baseRegister & KPH_HW_VSEC_BIR_MASK);
        regionOffset = (baseRegister & ~KPH_HW_VSEC_BIR_MASK);

        if (regionBar >= PCI_TYPE0_ADDRESSES)
        {
            KphTracePrint(TRACE_LEVEL_VERBOSE,
                          HARDWARE,
                          "PMT entry names BAR %lu",
                          (ULONG)regionBar);

            return STATUS_NOT_FOUND;
        }
    }
    else
    {
        KphTracePrint(TRACE_LEVEL_VERBOSE,
                      HARDWARE,
                      "PMT entry access type %lu is unknown",
                      (ULONG)header.AccessType);

        return STATUS_NOT_FOUND;
    }

    status = KphpHwPciVsecDeriveBar(Device,
                                    Bus,
                                    regionBar,
                                    Bars,
                                    &regionBarBase,
                                    &regionBarLength,
                                    ReadFailed);
    if (!NT_SUCCESS(status))
    {
        if (!*ReadFailed)
        {
            return STATUS_NOT_FOUND;
        }

        KphTracePrint(TRACE_LEVEL_VERBOSE,
                      HARDWARE,
                      "KphpHwPciVsecDeriveBar failed: %!STATUS!",
                      status);

        return status;
    }

    if (!KphpHwPciVsecRegionInBar(regionBarBase,
                                  regionBarLength,
                                  regionOffset,
                                  size,
                                  guid,
                                  Location))
    {
        return STATUS_NOT_FOUND;
    }

    return STATUS_SUCCESS;
}

/**
 * \brief Locates the instance regions of a TPMI discovery entry.
 *
 * \param[in] Vsec The vsec_read op body.
 * \param[in] Table The discovery table.
 * \param[in] Index The index of the entry.
 * \param[out] Locations Receives the regions, MaxRegions entries.
 * \param[in,out] Count The number of regions in Locations.
 */
_IRQL_requires_max_(PASSIVE_LEVEL)
VOID KphpHwPciVsecLocateTpmi(
    _In_ PKPH_DYN_HW_VSEC_READ Vsec,
    _In_ PKPH_HW_VSEC_TABLE Table,
    _In_ ULONG Index,
    _Out_writes_to_(KPH_DYN_HW_VSEC_MAX_REGIONS, *Count)
        PKPH_HW_VSEC_LOCATION Locations,
    _Inout_ PULONG Count
    )
{
    PULONG entry;
    KPH_HW_VSEC_TPMI_HEADER header;
    KPH_HW_VSEC_TPMI_LOCATION location;
    ULONG size;
    ULONG64 capOffset;
    ULONG64 featureOffset;

    KPH_PAGED_CODE_PASSIVE();

    entry = &Table->Entries[Index * Table->EntryDwords];

    header.Value = entry[0];
    location.Value = entry[1];

    if (header.FeatureId != Vsec->FeatureId)
    {
        return;
    }

    size = (header.Size * sizeof(ULONG));

    if (!size || (size > Vsec->MaxRegionSize))
    {
        return;
    }

    NT_VERIFY(NT_SUCCESS(RtlULongLongMult(location.CapOffset,
                                          KPH_HW_VSEC_TPMI_CAP_UNIT,
                                          &capOffset)));
    NT_VERIFY(NT_SUCCESS(RtlULongLongAdd(Table->Offset,
                                         capOffset,
                                         &featureOffset)));

    for (ULONG i = 0;
         (i < header.Instances) && (*Count < Vsec->MaxRegions);
         i++)
    {
        ULONG64 instanceOffset;
        ULONG64 regionOffset;

        NT_VERIFY(NT_SUCCESS(RtlULongLongMult(i, size, &instanceOffset)));
        NT_VERIFY(NT_SUCCESS(RtlULongLongAdd(featureOffset,
                                             instanceOffset,
                                             &regionOffset)));

        if (!KphpHwPciVsecRegionInBar(Table->BarBase,
                                      Table->BarLength,
                                      regionOffset,
                                      size,
                                      Vsec->FeatureId,
                                      &Locations[*Count]))
        {
            break;
        }

        (*Count)++;
    }
}

/**
 * \brief Locates the regions a vsec_read op selects on a device.
 *
 * \param[in] Locator The locator of the device.
 * \param[in,out] Device The device.
 * \param[in] Bus The bus interface of the device.
 * \param[in] Op The vsec_read op.
 * \param[out] Locations Receives the regions, MaxRegions entries.
 * \param[out] Count Receives the number of regions; zero on failure.
 * \param[in,out] ReadFailed Set TRUE when a read failed or read all-ones.
 *
 * \return Successful or errant status.
 */
_IRQL_requires_max_(PASSIVE_LEVEL)
_Requires_lock_held_(KphpHwPciLock)
_Must_inspect_result_
NTSTATUS KphpHwPciLocateVsec(
    _In_ PKPH_HW_PCI_LOCATOR Locator,
    _Inout_ PKPH_HW_PCI_DEVICE Device,
    _In_ PBUS_INTERFACE_STANDARD Bus,
    _In_ PKPH_DYN_HW_OP Op,
    _Out_writes_to_(KPH_DYN_HW_VSEC_MAX_REGIONS, *Count)
        PKPH_HW_VSEC_LOCATION Locations,
    _Out_ PULONG Count,
    _Inout_ PBOOLEAN ReadFailed
    )
{
    NTSTATUS status;
    PKPH_DYN_HW_VSEC_READ vsec;
    ULONG positions[KPH_HW_PCI_VSEC_MAX_CAPABILITIES];
    ULONG capabilityCount;
    ULONG entryDwords;
    ULONG count;
    KPH_HW_VSEC_TABLE table;
    KPH_HW_VSEC_BARS bars;

    KPH_PAGED_CODE_PASSIVE();

    vsec = (PKPH_DYN_HW_VSEC_READ)Op->Body;

    NT_ASSERT(Op->Kind == KPH_DYN_HW_KIND_VSEC_READ);
    NT_ASSERT((vsec->MaxRegions >= 1) &&
              (vsec->MaxRegions <= KPH_DYN_HW_VSEC_MAX_REGIONS));

    *Count = 0;

    RtlZeroMemory(&bars, sizeof(bars));

    table.Entries = NULL;
    count = 0;

    if (BooleanFlagOn(Op->Flags, KPH_DYN_HW_FLAG_REQUIRE_D0))
    {
        status = KphpHwPciCheckD0(Device, Bus, ReadFailed);
        if (!NT_SUCCESS(status))
        {
            KphTracePrint(TRACE_LEVEL_VERBOSE,
                          HARDWARE,
                          "KphpHwPciCheckD0 failed: %!STATUS!",
                          status);

            goto Exit;
        }
    }

    status = KphpHwPciFindVsecCapabilities(Bus,
                                           vsec->Discovery,
                                           positions,
                                           &capabilityCount,
                                           ReadFailed);
    if (!NT_SUCCESS(status))
    {
        KphTracePrint(TRACE_LEVEL_VERBOSE,
                      HARDWARE,
                      "KphpHwPciFindVsecCapabilities failed: %!STATUS!",
                      status);

        goto Exit;
    }

    if (vsec->Discovery == KPH_DYN_HW_VSEC_DISCOVERY_PMT_TELEMETRY)
    {
        entryDwords = KPH_HW_VSEC_PMT_ENTRY_DWORDS;
    }
    else
    {
        NT_ASSERT(vsec->Discovery == KPH_DYN_HW_VSEC_DISCOVERY_TPMI);
        entryDwords = KPH_HW_VSEC_TPMI_ENTRY_DWORDS;
    }

    for (ULONG c = 0; (c < capabilityCount) && (count < vsec->MaxRegions); c++)
    {
        status = KphpHwPciVsecReadTable(Locator,
                                        Device,
                                        Bus,
                                        vsec,
                                        positions[c],
                                        entryDwords,
                                        &bars,
                                        &table,
                                        ReadFailed);
        if (!NT_SUCCESS(status))
        {
            KphTracePrint(TRACE_LEVEL_VERBOSE,
                          HARDWARE,
                          "KphpHwPciVsecReadTable failed: %!STATUS!",
                          status);

            goto Exit;
        }

        for (ULONG i = 0; (i < table.Count) && (count < vsec->MaxRegions); i++)
        {
            if (vsec->Discovery != KPH_DYN_HW_VSEC_DISCOVERY_PMT_TELEMETRY)
            {
                KphpHwPciVsecLocateTpmi(vsec, &table, i, Locations, &count);
                continue;
            }

            status = KphpHwPciVsecLocatePmt(Device,
                                            Bus,
                                            vsec,
                                            &table,
                                            i,
                                            &bars,
                                            &Locations[count],
                                            ReadFailed);
            if (status == STATUS_NOT_FOUND)
            {
                continue;
            }

            if (!NT_SUCCESS(status))
            {
                KphTracePrint(TRACE_LEVEL_VERBOSE,
                              HARDWARE,
                              "KphpHwPciVsecLocatePmt failed: %!STATUS!",
                              status);

                goto Exit;
            }

            count++;
        }

        KphFree(table.Entries, KPH_TAG_HW_VSEC);
        table.Entries = NULL;
    }

    *Count = count;
    status = STATUS_SUCCESS;

Exit:

    if (table.Entries)
    {
        KphFree(table.Entries, KPH_TAG_HW_VSEC);
    }

    return status;
}

/**
 * \brief Refuses MMIO for a device the PDO cache does not hold.
 *
 * \param[in] Device The opened device.
 *
 * \return STATUS_SUCCESS for a cached device, STATUS_NOT_SUPPORTED otherwise.
 */
_IRQL_requires_max_(PASSIVE_LEVEL)
_Requires_lock_held_(KphpHwPciLock)
_Must_inspect_result_
NTSTATUS KphpHwPciVsecRequireCached(
    _In_ PKPH_HW_PCI_DEVICE Device
    )
{
    KPH_PAGED_CODE_PASSIVE();

    if (Device != &KphpHwPciUncached)
    {
        return STATUS_SUCCESS;
    }

    KphTracePrint(TRACE_LEVEL_VERBOSE,
                  HARDWARE,
                  "PCI %04lx:%04lx VSEC refused: removal policy is not "
                  "ExpectNoRemoval",
                  (ULONG)Device->Ids.VendorId,
                  (ULONG)Device->Ids.DeviceId);

    return STATUS_NOT_SUPPORTED;
}

/**
 * \brief Reads a batch of allow-listed registers of a VSEC region.
 *
 * \param[in] Locator The locator of the device.
 * \param[in] Section The hardware section.
 * \param[in] OpIndex The index of the op among the vsec_read ops of the device.
 * \param[in] Instance The index of the region among those the op exposes.
 * \param[in] Count The number of entries.
 * \param[in,out] Entries The batch entries, in system memory.
 *
 * \return Successful or errant status.
 */
_IRQL_requires_max_(PASSIVE_LEVEL)
_Must_inspect_result_
NTSTATUS KphHwPciReadVsec(
    _In_ PKPH_HW_PCI_LOCATOR Locator,
    _In_ PKPH_DYN_HW_SECTION Section,
    _In_ ULONG OpIndex,
    _In_ ULONG Instance,
    _In_ ULONG Count,
    _Inout_updates_(Count) PKPH_HW_WINDOW_BATCH_ENTRY Entries
    )
{
    NTSTATUS status;
    KPH_HW_PCI_LOCATOR locator;
    PKPH_HW_PCI_DEVICE device;
    KPH_HW_PCI_BUS bus;
    PKPH_DYN_HW_TABLE table;
    PKPH_DYN_HW_OP op;
    BOOLEAN readFailed;
    PKPH_HW_VSEC_LOCATION locations;
    ULONG regionCount;
    KPH_HW_VSEC_LOCATION location;
    ULONG spanBegin;
    ULONG spanEnd;

    KPH_PAGED_CODE_PASSIVE();

    locator = *Locator;

    if (!KphpHwPciValidLocator(&locator))
    {
        KphTracePrint(TRACE_LEVEL_VERBOSE, HARDWARE, "Invalid PCI locator");

        return STATUS_INVALID_PARAMETER;
    }

    locations = KphAllocatePaged((KPH_DYN_HW_VSEC_MAX_REGIONS *
                                  sizeof(KPH_HW_VSEC_LOCATION)),
                                 KPH_TAG_HW_VSEC);
    if (!locations)
    {
        KphTracePrint(TRACE_LEVEL_VERBOSE,
                      HARDWARE,
                      "Failed to allocate VSEC regions");

        return STATUS_INSUFFICIENT_RESOURCES;
    }

    device = NULL;
    readFailed = FALSE;
    RtlZeroMemory(&bus, sizeof(bus));

    KphAcquireRWLockExclusive(&KphpHwPciLock);

    status = KphpHwPciOpen(&locator, Section, TRUE, &device, &bus, &table);
    if (!NT_SUCCESS(status))
    {
        KphTracePrint(TRACE_LEVEL_VERBOSE,
                      HARDWARE,
                      "KphpHwPciOpen failed: %!STATUS!",
                      status);

        goto Exit;
    }

    status = KphpHwPciVsecRequireCached(device);
    if (!NT_SUCCESS(status))
    {
        KphTracePrint(TRACE_LEVEL_VERBOSE,
                      HARDWARE,
                      "KphpHwPciVsecRequireCached failed: %!STATUS!",
                      status);

        goto Exit;
    }

    status = KphDynHwFindVsecOp(table, OpIndex, &op);
    if (!NT_SUCCESS(status))
    {
        KphTracePrint(TRACE_LEVEL_VERBOSE,
                      HARDWARE,
                      "KphDynHwFindVsecOp failed: %!STATUS!",
                      status);

        goto Exit;
    }

    status = KphpHwPciLocateVsec(&locator,
                                 device,
                                 &bus.Interface,
                                 op,
                                 locations,
                                 &regionCount,
                                 &readFailed);
    if (!NT_SUCCESS(status))
    {
        KphTracePrint(TRACE_LEVEL_VERBOSE,
                      HARDWARE,
                      "KphpHwPciLocateVsec failed: %!STATUS!",
                      status);

        goto Exit;
    }

    if (Instance >= regionCount)
    {
        KphTracePrint(TRACE_LEVEL_VERBOSE,
                      HARDWARE,
                      "VSEC instance %lu out of %lu",
                      Instance,
                      regionCount);

        status = STATUS_NOT_FOUND;
        goto Exit;
    }

    location = locations[Instance];

    if (!KphHwVsecValidateEntries(op,
                                  &location,
                                  Count,
                                  Entries,
                                  &spanBegin,
                                  &spanEnd))
    {
        KphTracePrint(TRACE_LEVEL_VERBOSE,
                      HARDWARE,
                      "No VSEC entry is allowed");

        status = STATUS_INVALID_PARAMETER;
        goto Exit;
    }

    status = KphHwVsecMapAndRead(&locator,
                                 &location,
                                 spanBegin,
                                 spanEnd,
                                 Count,
                                 Entries);
    if (!NT_SUCCESS(status))
    {
        KphTracePrint(TRACE_LEVEL_VERBOSE,
                      HARDWARE,
                      "KphHwVsecMapAndRead failed: %!STATUS!",
                      status);

        goto Exit;
    }

    status = KphpHwPciLocateVsec(&locator,
                                 device,
                                 &bus.Interface,
                                 op,
                                 locations,
                                 &regionCount,
                                 &readFailed);
    if (!NT_SUCCESS(status) ||
        (Instance >= regionCount) ||
        (locations[Instance].Base != location.Base) ||
        (locations[Instance].Size != location.Size) ||
        (locations[Instance].Id != location.Id))
    {
        KphTracePrint(TRACE_LEVEL_VERBOSE,
                      HARDWARE,
                      "VSEC region 0x%I64x moved: %!STATUS!",
                      location.Base,
                      status);

        for (ULONG i = 0; i < Count; i++)
        {
            if (Entries[i].Status == STATUS_SUCCESS)
            {
                Entries[i].Value = 0;
                Entries[i].Status = STATUS_DEVICE_NOT_READY;
            }
        }
    }

    status = STATUS_SUCCESS;

Exit:

    if (readFailed)
    {
        NT_ASSERT(device);

        KphpHwPciReleaseBus(&bus);
        KphpHwPciClose(device);
    }

    KphpHwPciRelease(&bus);

    KphReleaseRWLock(&KphpHwPciLock);

    KphFree(locations, KPH_TAG_HW_VSEC);

    return status;
}

/**
 * \brief Lists the regions the vsec_read ops of a device expose.
 *
 * \param[in] Locator The locator of the device.
 * \param[in] Section The hardware section.
 * \param[in] Count The capacity of Regions.
 * \param[out] Regions Receives the regions, in system memory, when they fit.
 * \param[out] Total Receives the number of regions.
 *
 * \return Successful or errant status; STATUS_NOT_FOUND when the device has
 * no vsec_read op.
 */
_IRQL_requires_max_(PASSIVE_LEVEL)
_Must_inspect_result_
NTSTATUS KphHwPciQueryVsecRegions(
    _In_ PKPH_HW_PCI_LOCATOR Locator,
    _In_ PKPH_DYN_HW_SECTION Section,
    _In_ ULONG Count,
    _Out_writes_(Count) PKPH_HW_VSEC_REGION Regions,
    _Out_ PULONG Total
    )
{
    NTSTATUS status;
    KPH_HW_PCI_LOCATOR locator;
    PKPH_HW_PCI_DEVICE device;
    KPH_HW_PCI_BUS bus;
    PKPH_DYN_HW_TABLE table;
    BOOLEAN readFailed;
    PKPH_HW_VSEC_LOCATION locations;
    ULONG total;
    ULONG opIndex;

    KPH_PAGED_CODE_PASSIVE();

    *Total = 0;

    locator = *Locator;

    if (!KphpHwPciValidLocator(&locator))
    {
        KphTracePrint(TRACE_LEVEL_VERBOSE, HARDWARE, "Invalid PCI locator");

        return STATUS_INVALID_PARAMETER;
    }

    locations = KphAllocatePaged(KPH_DYN_HW_VSEC_MAX_REGIONS *
                                 sizeof(KPH_HW_VSEC_LOCATION),
                                 KPH_TAG_HW_VSEC);
    if (!locations)
    {
        KphTracePrint(TRACE_LEVEL_VERBOSE,
                      HARDWARE,
                      "Failed to allocate VSEC regions");

        return STATUS_INSUFFICIENT_RESOURCES;
    }

    device = NULL;
    readFailed = FALSE;
    RtlZeroMemory(&bus, sizeof(bus));
    total = 0;

    KphAcquireRWLockExclusive(&KphpHwPciLock);

    status = KphpHwPciOpen(&locator, Section, TRUE, &device, &bus, &table);
    if (!NT_SUCCESS(status))
    {
        KphTracePrint(TRACE_LEVEL_VERBOSE,
                      HARDWARE,
                      "KphpHwPciOpen failed: %!STATUS!",
                      status);

        goto Exit;
    }

    status = KphpHwPciVsecRequireCached(device);
    if (!NT_SUCCESS(status))
    {
        KphTracePrint(TRACE_LEVEL_VERBOSE,
                      HARDWARE,
                      "KphpHwPciVsecRequireCached failed: %!STATUS!",
                      status);

        goto Exit;
    }

    for (opIndex = 0; opIndex <= MAXUCHAR; opIndex++)
    {
        PKPH_DYN_HW_OP op;
        PKPH_DYN_HW_VSEC_READ vsec;
        ULONG regionCount;

        status = KphDynHwFindVsecOp(table, opIndex, &op);
        if (!NT_SUCCESS(status))
        {
            break;
        }

        vsec = (PKPH_DYN_HW_VSEC_READ)op->Body;

        status = KphpHwPciLocateVsec(&locator,
                                     device,
                                     &bus.Interface,
                                     op,
                                     locations,
                                     &regionCount,
                                     &readFailed);
        if (!NT_SUCCESS(status))
        {
            KphTracePrint(TRACE_LEVEL_VERBOSE,
                          HARDWARE,
                          "KphpHwPciLocateVsec failed: %!STATUS!",
                          status);

            goto Exit;
        }

        for (ULONG i = 0; i < regionCount; i++)
        {
            if (total < Count)
            {
                RtlZeroMemory(&Regions[total], sizeof(KPH_HW_VSEC_REGION));
                Regions[total].OpIndex = (UCHAR)opIndex;
                Regions[total].Discovery = vsec->Discovery;
                Regions[total].Instance = (USHORT)i;
                Regions[total].Id = locations[i].Id;
                Regions[total].Size = locations[i].Size;
            }

            total++;
        }
    }

    if (opIndex == 0)
    {
        KphTracePrint(TRACE_LEVEL_VERBOSE,
                      HARDWARE,
                      "PCI %04lx:%04lx has no vsec_read op",
                      (ULONG)device->Ids.VendorId,
                      (ULONG)device->Ids.DeviceId);

        status = STATUS_NOT_FOUND;
        goto Exit;
    }

    *Total = total;
    status = STATUS_SUCCESS;

Exit:

    if (readFailed)
    {
        NT_ASSERT(device);

        KphpHwPciReleaseBus(&bus);
        KphpHwPciClose(device);
    }

    KphpHwPciRelease(&bus);

    KphReleaseRWLock(&KphpHwPciLock);

    KphFree(locations, KPH_TAG_HW_VSEC);

    return status;
}

#if defined(_M_X64)

/**
 * \brief Opens the SMN index/data pair of a root complex for a mailbox op.
 *
 * \param[in] Locator The locator of the root complex.
 * \param[in] Section The hardware section.
 * \param[in] Kind KPH_DYN_HW_KIND_MAILBOX_TRANSACT or
 * KPH_DYN_HW_KIND_SMU_TABLE_READ.
 * \param[in] OpIndex The index of the op among the ops of the kind.
 * \param[out] Op Receives the op, valid while the section is referenced.
 * \param[out] Mailbox Receives the mailbox_transact op (Op itself for a
 * mailbox_transact op).
 * \param[out] Session Receives the session, closed with KphHwPciCloseSmn.
 *
 * \return Successful or errant status.
 */
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
    )
{
    NTSTATUS status;
    KPH_HW_PCI_LOCATOR locator;
    PKPH_HW_PCI_DEVICE device;
    KPH_HW_PCI_BUS bus;
    PKPH_DYN_HW_TABLE table;
    PKPH_DYN_HW_OP op;
    PKPH_DYN_HW_OP mailboxOp;
    PKPH_DYN_HW_MAILBOX_TRANSACT mailbox;
    PKPH_DYN_HW_MAILBOX_SMN smn;
    BOOLEAN readFailed;

    KPH_PAGED_CODE_PASSIVE();
    NT_ASSERT((Kind == KPH_DYN_HW_KIND_MAILBOX_TRANSACT) ||
              (Kind == KPH_DYN_HW_KIND_SMU_TABLE_READ));

    *Op = NULL;
    *Mailbox = NULL;
    RtlZeroMemory(Session, sizeof(KPH_HW_SMN_SESSION));

    locator = *Locator;

    if (!KphpHwPciValidLocator(&locator))
    {
        KphTracePrint(TRACE_LEVEL_VERBOSE, HARDWARE, "Invalid PCI locator");

        return STATUS_INVALID_PARAMETER;
    }

    device = NULL;
    RtlZeroMemory(&bus, sizeof(bus));
    readFailed = FALSE;

    KphAcquireRWLockExclusive(&KphpHwPciLock);

    status = KphpHwPciOpen(&locator, Section, FALSE, &device, &bus, &table);
    if (!NT_SUCCESS(status))
    {
        KphTracePrint(TRACE_LEVEL_VERBOSE,
                      HARDWARE,
                      "KphpHwPciOpen failed: %!STATUS!",
                      status);

        goto Exit;
    }

    //
    // The handshake spans PCI lock releases; only cached devices are used.
    //
    if (device == &KphpHwPciUncached)
    {
        KphTracePrint(TRACE_LEVEL_VERBOSE,
                      HARDWARE,
                      "PCI %04lx:%04lx SMN pair refused: removal policy is not "
                      "ExpectNoRemoval",
                      (ULONG)device->Ids.VendorId,
                      (ULONG)device->Ids.DeviceId);

        status = STATUS_NOT_SUPPORTED;
        goto Exit;
    }

    if (Kind == KPH_DYN_HW_KIND_MAILBOX_TRANSACT)
    {
        status = KphDynHwFindMailboxOp(table, OpIndex, &op);
        if (!NT_SUCCESS(status))
        {
            KphTracePrint(TRACE_LEVEL_VERBOSE,
                          HARDWARE,
                          "KphDynHwFindMailboxOp failed: %!STATUS!",
                          status);

            goto Exit;
        }

        mailboxOp = op;
    }
    else
    {
        PKPH_DYN_HW_SMU_TABLE_READ smuTable;

        status = KphDynHwFindSmuTableOp(table, OpIndex, &op);
        if (!NT_SUCCESS(status))
        {
            KphTracePrint(TRACE_LEVEL_VERBOSE,
                          HARDWARE,
                          "KphDynHwFindSmuTableOp failed: %!STATUS!",
                          status);

            goto Exit;
        }

        smuTable = (PKPH_DYN_HW_SMU_TABLE_READ)op->Body;

        NT_VERIFY(NT_SUCCESS(KphDynHwFindMailboxOp(table,
                                                   smuTable->MailboxOp,
                                                   &mailboxOp)));
    }

    mailbox = (PKPH_DYN_HW_MAILBOX_TRANSACT)mailboxOp->Body;

    NT_ASSERT(mailbox->Transport == KPH_DYN_HW_MAILBOX_TRANSPORT_SMN_PAIR);

    smn = &mailbox->Address.Smn;

    if ((locator.Device != smn->PinDevice) ||
        (locator.Function != smn->PinFunction))
    {
        KphTracePrint(TRACE_LEVEL_VERBOSE,
                      HARDWARE,
                      "SMN pair is pinned to %lu.%lu",
                      (ULONG)smn->PinDevice,
                      (ULONG)smn->PinFunction);

        status = STATUS_ACCESS_DENIED;
        goto Exit;
    }

    ObReferenceObject(device->Pdo);

    Session->Locator = locator;
    Session->IndexOffset = smn->IndexOffset;
    Session->DataOffset = smn->DataOffset;
    Session->Pdo = device->Pdo;
    Session->Bus = bus;
    RtlZeroMemory(&bus, sizeof(bus));

    *Op = op;
    *Mailbox = mailboxOp;

    status = STATUS_SUCCESS;

Exit:

    if (readFailed)
    {
        NT_ASSERT(device);

        KphpHwPciReleaseBus(&bus);
        KphpHwPciClose(device);
    }

    KphpHwPciRelease(&bus);

    KphReleaseRWLock(&KphpHwPciLock);

    return status;
}

/**
 * \brief Closes an SMN session opened by KphHwPciOpenSmn.
 *
 * \param[in,out] Session The session, zeroed on return.
 */
_IRQL_requires_max_(PASSIVE_LEVEL)
VOID KphHwPciCloseSmn(
    _Inout_ PKPH_HW_SMN_SESSION Session
    )
{
    KPH_PAGED_CODE_PASSIVE();

    KphpHwPciReleaseBus(&Session->Bus);

    if (Session->Pdo)
    {
        ObDereferenceObject(Session->Pdo);
    }

    RtlZeroMemory(Session, sizeof(KPH_HW_SMN_SESSION));
}

/**
 * \brief Reads an SMN register through the pair of a session.
 *
 * \param[in] Session The session.
 * \param[in] Address The SMN address, 4-aligned.
 * \param[out] Value Receives the register value.
 *
 * \return Successful or errant status.
 */
_IRQL_requires_max_(PASSIVE_LEVEL)
_Must_inspect_result_
NTSTATUS KphHwPciSmnRead(
    _In_ PKPH_HW_SMN_SESSION Session,
    _In_ ULONG Address,
    _Out_ PULONG Value
    )
{
    NTSTATUS status;

    KPH_PAGED_CODE_PASSIVE();
    NT_ASSERT(Session->Bus.Interface.GetBusData);
    NT_ASSERT((Address & 3) == 0);

    KphAcquireRWLockExclusive(&KphpHwPciLock);

    status = KphpHwPciIndirectCycle(&Session->Bus.Interface,
                                    Session->IndexOffset,
                                    Session->DataOffset,
                                    Address,
                                    Value);

    KphReleaseRWLock(&KphpHwPciLock);

    return status;
}

/**
 * \brief Writes an SMN register through the pair of a session.
 *
 * \param[in] Session The session.
 * \param[in] Address The SMN address, 4-aligned.
 * \param[in] Value The value to write.
 *
 * \return Successful or errant status; STATUS_DEVICE_NOT_READY when an
 * access failed, STATUS_RETRY when the index moved.
 */
_IRQL_requires_max_(PASSIVE_LEVEL)
_Must_inspect_result_
NTSTATUS KphHwPciSmnWrite(
    _In_ PKPH_HW_SMN_SESSION Session,
    _In_ ULONG Address,
    _In_ ULONG Value
    )
{
    NTSTATUS status;
    PBUS_INTERFACE_STANDARD bus;
    ULONG index;
    ULONG value;
    ULONG check;
    ULONG bytes;

    KPH_PAGED_CODE_PASSIVE();
    NT_ASSERT(Session->Bus.Interface.SetBusData);
    NT_ASSERT((Address & 3) == 0);

    bus = &Session->Bus.Interface;
    index = Address;
    value = Value;
    check = 0;

    KphAcquireRWLockExclusive(&KphpHwPciLock);

    bytes = bus->SetBusData(bus->Context,
                            PCI_WHICHSPACE_CONFIG,
                            &index,
                            Session->IndexOffset,
                            sizeof(index));
    if (bytes != sizeof(index))
    {
        KphTracePrint(TRACE_LEVEL_VERBOSE,
                      HARDWARE,
                      "SMN index write failed: %lu bytes",
                      bytes);

        status = STATUS_DEVICE_NOT_READY;
        goto Exit;
    }

    bytes = bus->SetBusData(bus->Context,
                            PCI_WHICHSPACE_CONFIG,
                            &value,
                            Session->DataOffset,
                            sizeof(value));
    if (bytes != sizeof(value))
    {
        KphTracePrint(TRACE_LEVEL_VERBOSE,
                      HARDWARE,
                      "SMN data write failed: %lu bytes",
                      bytes);

        status = STATUS_DEVICE_NOT_READY;
        goto Exit;
    }

    bytes = bus->GetBusData(bus->Context,
                            PCI_WHICHSPACE_CONFIG,
                            &check,
                            Session->IndexOffset,
                            sizeof(check));
    if (bytes != sizeof(check))
    {
        KphTracePrint(TRACE_LEVEL_VERBOSE,
                      HARDWARE,
                      "SMN index read failed: %lu bytes",
                      bytes);

        status = STATUS_DEVICE_NOT_READY;
        goto Exit;
    }

    if (check != Address)
    {
        KphTracePrint(TRACE_LEVEL_VERBOSE,
                      HARDWARE,
                      "SMN index 0x%lx moved to 0x%lx",
                      Address,
                      check);

        status = STATUS_RETRY;
        goto Exit;
    }

    status = STATUS_SUCCESS;

Exit:

    KphReleaseRWLock(&KphpHwPciLock);

    return status;
}

/**
 * \brief Resolves the indirect port read operation of a PCI function.
 *
 * \param[in] Locator The locator of the function.
 * \param[in] Section The hardware section.
 * \param[in] OpIndex The index of the op among the port_indirect_read ops.
 * \param[out] Op Receives the op, valid while the section is referenced; NULL
 * on failure.
 *
 * \return Successful or errant status.
 */
_IRQL_requires_max_(PASSIVE_LEVEL)
_Must_inspect_result_
NTSTATUS KphHwPciResolvePortIndirect(
    _In_ PKPH_HW_PCI_LOCATOR Locator,
    _In_ PKPH_DYN_HW_SECTION Section,
    _In_ ULONG OpIndex,
    _Outptr_ PKPH_DYN_HW_OP* Op
    )
{
    NTSTATUS status;
    KPH_HW_PCI_LOCATOR locator;
    PKPH_HW_PCI_DEVICE device;
    KPH_HW_PCI_BUS bus;
    PKPH_DYN_HW_TABLE table;
    PKPH_DYN_HW_OP op;
    PKPH_DYN_HW_PORT_INDIRECT_READ port;

    KPH_PAGED_CODE_PASSIVE();

    *Op = NULL;

    locator = *Locator;

    if (!KphpHwPciValidLocator(&locator))
    {
        KphTracePrint(TRACE_LEVEL_VERBOSE, HARDWARE, "Invalid PCI locator");

        return STATUS_INVALID_PARAMETER;
    }

    RtlZeroMemory(&bus, sizeof(bus));

    KphAcquireRWLockExclusive(&KphpHwPciLock);

    status = KphpHwPciOpen(&locator, Section, FALSE, &device, &bus, &table);
    if (!NT_SUCCESS(status))
    {
        KphTracePrint(TRACE_LEVEL_VERBOSE,
                      HARDWARE,
                      "KphpHwPciOpen failed: %!STATUS!",
                      status);

        goto Exit;
    }

    if (device == &KphpHwPciUncached)
    {
        KphTracePrint(TRACE_LEVEL_VERBOSE,
                      HARDWARE,
                      "PCI %04lx:%04lx port pair refused: removal policy is "
                      "not ExpectNoRemoval",
                      (ULONG)device->Ids.VendorId,
                      (ULONG)device->Ids.DeviceId);

        status = STATUS_NOT_SUPPORTED;
        goto Exit;
    }

    status = KphDynHwFindPortIndirectOp(table, OpIndex, &op);
    if (!NT_SUCCESS(status))
    {
        KphTracePrint(TRACE_LEVEL_VERBOSE,
                      HARDWARE,
                      "KphDynHwFindPortIndirectOp failed: %!STATUS!",
                      status);

        goto Exit;
    }

    port = (PKPH_DYN_HW_PORT_INDIRECT_READ)op->Body;

    if ((locator.Device != port->PinDevice) ||
        (locator.Function != port->PinFunction))
    {
        KphTracePrint(TRACE_LEVEL_VERBOSE,
                      HARDWARE,
                      "Port pair is pinned to %lu.%lu",
                      (ULONG)port->PinDevice,
                      (ULONG)port->PinFunction);

        status = STATUS_ACCESS_DENIED;
        goto Exit;
    }

    *Op = op;

    status = STATUS_SUCCESS;

Exit:

    KphpHwPciRelease(&bus);

    KphReleaseRWLock(&KphpHwPciLock);

    return status;
}

#endif
