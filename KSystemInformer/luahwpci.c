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
#include <script.h>
#include <initguid.h>
#include <wdmguid.h>
#include <devpkey.h>

#include <trace.h>

#define KPH_LUA_HW_PCI_CONFIG_LENGTH       0x1000
#define KPH_LUA_HW_PCI_LOCATION_CHARS      128
#define KPH_LUA_HW_PCI_ID_VENDOR           8
#define KPH_LUA_HW_PCI_ID_DEVICE_PREFIX    12
#define KPH_LUA_HW_PCI_ID_DEVICE           17
#define KPH_LUA_HW_PCI_ID_CHARS            21
#define KPH_LUA_HW_PCI_ID_CLASS            3
#define KPH_LUA_HW_PCI_ID_PROG_IF          7
#define KPH_LUA_HW_PCI_ID_CLASS_CHARS      9
#define KPH_LUA_HW_PCI_ID_CLASS_MIN_CHARS  13

typedef union _KPH_LUA_HW_PCI_ADDRESS
{
    struct
    {
        ULONG Function : 16;
        ULONG Device : 16;
    };
    ULONG Value;
} KPH_LUA_HW_PCI_ADDRESS, *PKPH_LUA_HW_PCI_ADDRESS;

C_ASSERT(sizeof(KPH_LUA_HW_PCI_ADDRESS) == sizeof(ULONG));

typedef struct _KPH_LUA_HW_PCI_LOCATOR
{
    ULONG Segment;
    ULONG Bus;
    ULONG Device;
    ULONG Function;
} KPH_LUA_HW_PCI_LOCATOR, *PKPH_LUA_HW_PCI_LOCATOR;

typedef struct _KPH_LUA_HW_PCI_ENTRY
{
    KPH_LUA_HW_PCI_LOCATOR Locator;
    USHORT VendorId;
    USHORT DeviceId;
    UCHAR BaseClass;
    UCHAR SubClass;
    UCHAR ProgIf;
} KPH_LUA_HW_PCI_ENTRY, *PKPH_LUA_HW_PCI_ENTRY;

typedef struct _KPH_LUA_HW_PCI_DEVICE
{
    PDEVICE_OBJECT Pdo;
    PDEVICE_OBJECT Top;
    BUS_INTERFACE_STANDARD Bus;
} KPH_LUA_HW_PCI_DEVICE, *PKPH_LUA_HW_PCI_DEVICE;

KPH_PROTECTED_DATA_SECTION_RO_PUSH();
DECLARE_CONST_UNICODE_STRING(KphpLuaHwPciDriverName, L"\\Driver\\pci");
DECLARE_CONST_UNICODE_STRING(KphpLuaHwPciVendorPrefix, L"PCI\\VEN_");
DECLARE_CONST_UNICODE_STRING(KphpLuaHwPciDevicePrefix, L"&DEV_");
DECLARE_CONST_UNICODE_STRING(KphpLuaHwPciBusPrefix, L"PCI\\");
DECLARE_CONST_UNICODE_STRING(KphpLuaHwPciClassPrefix, L"CC_");
KPH_PROTECTED_DATA_SECTION_RO_POP();
static PAGED_LOOKASIDE_LIST KphpLuaHwPciDeviceLookaside = { 0 };

KPH_PAGED_FILE();

/**
 * \brief Parses up to four hexadecimal digits.
 *
 * \param[in] Text The digits.
 * \param[in] Count The number of digits, at most four.
 * \param[out] Value Receives the value.
 *
 * \return TRUE if all digits are hexadecimal digits.
 */
_IRQL_requires_max_(PASSIVE_LEVEL)
_Must_inspect_result_
BOOLEAN KphpLuaHwPciParseHex(
    _In_reads_(Count) PCWCH Text,
    _In_range_(1, 4) ULONG Count,
    _Out_ PUSHORT Value
    )
{
    USHORT value;

    KPH_PAGED_CODE_PASSIVE();

    *Value = 0;

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
 * \brief Reads the IDs from the first hardware ID of the form
 * PCI\VEN_vvvv&DEV_dddd.
 *
 * \param[in] DeviceObject The bus-enumerated device object.
 * \param[out] VendorId Receives the vendor ID.
 * \param[out] DeviceId Receives the device ID.
 *
 * \return Successful or errant status.
 */
_IRQL_requires_max_(PASSIVE_LEVEL)
_Must_inspect_result_
NTSTATUS KphpLuaHwPciReadHardwareId(
    _In_ PDEVICE_OBJECT DeviceObject,
    _Out_ PUSHORT VendorId,
    _Out_ PUSHORT DeviceId
    )
{
    NTSTATUS status;
    PWCHAR ids;
    ULONG length;
    UNICODE_STRING id;
    UNICODE_STRING part;
    USHORT vendorId;
    USHORT deviceId;

    KPH_PAGED_CODE_PASSIVE();

    *VendorId = 0;
    *DeviceId = 0;

    status = KphLuaHwQueryProperty(DeviceObject,
                                   DevicePropertyHardwareID,
                                   &ids,
                                   &length);
    if (!NT_SUCCESS(status))
    {
        KphTracePrint(TRACE_LEVEL_VERBOSE,
                      HARDWARE,
                      "KphLuaHwQueryProperty failed: %!STATUS!",
                      status);

        ids = NULL;
        goto Exit;
    }

    status = STATUS_NOT_FOUND;

    RtlInitUnicodeString(&id, ids);

    while (id.Length > 0)
    {
        if (id.Length >= (KPH_LUA_HW_PCI_ID_CHARS * sizeof(WCHAR)))
        {
            part.Buffer = id.Buffer;
            part.Length = KphpLuaHwPciVendorPrefix.Length;
            part.MaximumLength = part.Length;

            if (RtlEqualUnicodeString(&part, &KphpLuaHwPciVendorPrefix, TRUE) &&
                KphpLuaHwPciParseHex(&id.Buffer[KPH_LUA_HW_PCI_ID_VENDOR],
                                     4,
                                     &vendorId))
            {
                part.Buffer = &id.Buffer[KPH_LUA_HW_PCI_ID_DEVICE_PREFIX];
                part.Length = KphpLuaHwPciDevicePrefix.Length;
                part.MaximumLength = part.Length;

                if (RtlEqualUnicodeString(&part,
                                          &KphpLuaHwPciDevicePrefix,
                                          TRUE) &&
                    KphpLuaHwPciParseHex(&id.Buffer[KPH_LUA_HW_PCI_ID_DEVICE],
                                         4,
                                         &deviceId))
                {
                    *VendorId = vendorId;
                    *DeviceId = deviceId;
                    status = STATUS_SUCCESS;
                    break;
                }
            }
        }

        RtlInitUnicodeString(&id, Add2Ptr(id.Buffer, id.Length + sizeof(WCHAR)));
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
        KphFree(ids, KPH_TAG_LUA_HARDWARE_PROPERTY);
    }

    return status;
}

/**
 * \brief Reads the class code from the first compatible ID carrying a
 * CC_ccsspp component.
 *
 * \param[in] DeviceObject The bus-enumerated device object.
 * \param[out] BaseClass Receives the base class.
 * \param[out] SubClass Receives the sub class.
 * \param[out] ProgIf Receives the programming interface.
 *
 * \return Successful or errant status.
 */
_IRQL_requires_max_(PASSIVE_LEVEL)
_Must_inspect_result_
NTSTATUS KphpLuaHwPciReadClassCode(
    _In_ PDEVICE_OBJECT DeviceObject,
    _Out_ PUCHAR BaseClass,
    _Out_ PUCHAR SubClass,
    _Out_ PUCHAR ProgIf
    )
{
    NTSTATUS status;
    PWCHAR ids;
    ULONG length;
    UNICODE_STRING id;
    UNICODE_STRING part;

    KPH_PAGED_CODE_PASSIVE();

    *BaseClass = 0;
    *SubClass = 0;
    *ProgIf = 0;

    status = KphLuaHwQueryProperty(DeviceObject,
                                   DevicePropertyCompatibleIDs,
                                   &ids,
                                   &length);
    if (!NT_SUCCESS(status))
    {
        KphTracePrint(TRACE_LEVEL_VERBOSE,
                      HARDWARE,
                      "KphLuaHwQueryProperty failed: %!STATUS!",
                      status);

        ids = NULL;
        goto Exit;
    }

    status = STATUS_NOT_FOUND;

    RtlInitUnicodeString(&id, ids);

    while ((id.Length > 0) && (status == STATUS_NOT_FOUND))
    {
        ULONG count;
        ULONG first;

        part.Buffer = id.Buffer;
        part.Length = KphpLuaHwPciBusPrefix.Length;
        part.MaximumLength = part.Length;

        count = id.Length / sizeof(WCHAR);
        first = KphpLuaHwPciBusPrefix.Length / sizeof(WCHAR);

        if ((count >= KPH_LUA_HW_PCI_ID_CLASS_MIN_CHARS) &&
            RtlEqualUnicodeString(&part, &KphpLuaHwPciBusPrefix, TRUE))
        {
            for (ULONG i = first;
                 (i + KPH_LUA_HW_PCI_ID_CLASS_CHARS) <= count;
                 i++)
            {
                USHORT classes;
                USHORT progIf;
                ULONG end;

                if ((i != first) && (id.Buffer[i - 1] != L'&'))
                {
                    continue;
                }

                part.Buffer = &id.Buffer[i];
                part.Length = KphpLuaHwPciClassPrefix.Length;
                part.MaximumLength = part.Length;

                if (!RtlEqualUnicodeString(&part,
                                           &KphpLuaHwPciClassPrefix,
                                           TRUE))
                {
                    continue;
                }

                end = i + KPH_LUA_HW_PCI_ID_CLASS_CHARS;

                if ((end < count) && (id.Buffer[end] != L'&'))
                {
                    continue;
                }

                if (KphpLuaHwPciParseHex(
                        &id.Buffer[i + KPH_LUA_HW_PCI_ID_CLASS],
                        4,
                        &classes) &&
                    KphpLuaHwPciParseHex(
                        &id.Buffer[i + KPH_LUA_HW_PCI_ID_PROG_IF],
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
        KphFree(ids, KPH_TAG_LUA_HARDWARE_PROPERTY);
    }

    return status;
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
ULONG KphpLuaHwPciParseLocation(
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

        if (Numbers[numberCount - 1] > USHORT_MAX)
        {
            return 0;
        }
    }

    return numberCount;
}

/**
 * \brief Reads where a PCI physical device object is.
 *
 * \param[in] Pdo A bus-enumerated device object of the PCI bus driver.
 * \param[out] Locator Receives the segment, bus, device and function.
 *
 * \return Successful or errant status.
 */
_IRQL_requires_max_(PASSIVE_LEVEL)
_Must_inspect_result_
NTSTATUS KphpLuaHwPciReadLocator(
    _In_ PDEVICE_OBJECT Pdo,
    _Out_ PKPH_LUA_HW_PCI_LOCATOR Locator
    )
{
    NTSTATUS status;
    PCI_SEGMENT_BUS_NUMBER busNumber;
    KPH_LUA_HW_PCI_ADDRESS address;
    ULONG resultLength;
    WCHAR location[KPH_LUA_HW_PCI_LOCATION_CHARS];
    ULONG locationChars;
    ULONG numbers[4];
    ULONG numberCount;
    ULONG segment;

    KPH_PAGED_CODE_PASSIVE();

    RtlZeroMemory(Locator, sizeof(KPH_LUA_HW_PCI_LOCATOR));

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

        goto Exit;
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

        goto Exit;
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

        goto Exit;
    }

    locationChars = (min(resultLength, sizeof(location)) / sizeof(WCHAR));

    numberCount = KphpLuaHwPciParseLocation(location, locationChars, numbers);
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

        status = STATUS_NOT_FOUND;
        goto Exit;
    }

    if ((numbers[0] != busNumber.u.bits.BusNumber) ||
        (numbers[1] != address.Device) ||
        (numbers[2] != address.Function))
    {
        KphTracePrint(TRACE_LEVEL_VERBOSE,
                      HARDWARE,
                      "PCI location %lu:%lu.%lu differs from bus %lu address "
                      "%lu.%lu",
                      numbers[0],
                      numbers[1],
                      numbers[2],
                      (ULONG)busNumber.u.bits.BusNumber,
                      (ULONG)address.Device,
                      (ULONG)address.Function);

        status = STATUS_NOT_FOUND;
        goto Exit;
    }

    Locator->Segment = segment;
    Locator->Bus = numbers[0];
    Locator->Device = numbers[1];
    Locator->Function = numbers[2];

    status = STATUS_SUCCESS;

Exit:

    return status;
}

/**
 * \brief Frees the list of PCI functions.
 *
 * \param[in] Context The list.
 */
_Function_class_(KPH_SCRIPT_CLEANUP_ROUTINE)
_IRQL_requires_max_(PASSIVE_LEVEL)
VOID KSIAPI KphpLuaHwFreePciList(
    _In_opt_ PVOID Context
    )
{
    KPH_PAGED_CODE_PASSIVE();

    NT_ASSERT(Context);

    KphFree(Context, KPH_TAG_LUA_HARDWARE_PCI_LIST);
}

/**
 * \brief Lists the PCI functions from their PnP properties
 * (ksi.hw.pci_devices).
 *
 * \details May raise a Lua error. No request is sent to any device stack.
 *
 * \param[in] State The Lua state.
 *
 * \return The number of results: the array of functions, or nil and a status.
 */
_IRQL_requires_max_(PASSIVE_LEVEL)
int KphpLuaHwPciDevices(
    _In_ lua_State* State
    )
{
    int results;
    NTSTATUS status;
    PDEVICE_OBJECT* devices;
    ULONG count;
    ULONG length;
    PKPH_LUA_HW_PCI_ENTRY entries;
    ULONG found;
    int size;
    int cleanup;

    KPH_PAGED_CODE_PASSIVE();

    entries = NULL;
    found = 0;

    status = KphLuaHwReferenceDevices(&KphpLuaHwPciDriverName,
                                      &devices,
                                      &count);
    if (status == STATUS_NOT_FOUND)
    {
        lua_newtable(State);

        results = 1;
        goto Exit;
    }

    if (!NT_SUCCESS(status))
    {
        KphTracePrint(TRACE_LEVEL_VERBOSE,
                      HARDWARE,
                      "KphLuaHwReferenceDevices failed: %!STATUS!",
                      status);

        results = KphLuaNilErrorStatus(State, status);
        goto Exit;
    }

    status = RtlULongMult(count, sizeof(KPH_LUA_HW_PCI_ENTRY), &length);
    if (!NT_SUCCESS(status))
    {
        KphTracePrint(TRACE_LEVEL_VERBOSE,
                      HARDWARE,
                      "RtlULongMult failed: %!STATUS!",
                      status);

        results = KphLuaNilErrorStatus(State, status);
        goto Exit;
    }

    entries = KphAllocatePaged(length, KPH_TAG_LUA_HARDWARE_PCI_LIST);
    if (!entries)
    {
        KphTracePrint(TRACE_LEVEL_VERBOSE,
                      HARDWARE,
                      "Failed to allocate the PCI list");

        status = STATUS_INSUFFICIENT_RESOURCES;
        results = KphLuaNilErrorStatus(State, status);
        goto Exit;
    }

    for (ULONG i = 0; i < count; i++)
    {
        PKPH_LUA_HW_PCI_ENTRY entry;

        if (!BooleanFlagOn(devices[i]->Flags, DO_BUS_ENUMERATED_DEVICE))
        {
            continue;
        }

        entry = &entries[found];

        status = KphpLuaHwPciReadHardwareId(devices[i],
                                            &entry->VendorId,
                                            &entry->DeviceId);
        if (status == STATUS_NOT_FOUND)
        {
            continue;
        }

        if (!NT_SUCCESS(status))
        {
            KphTracePrint(TRACE_LEVEL_VERBOSE,
                          HARDWARE,
                          "KphpLuaHwPciReadHardwareId failed: %!STATUS!",
                          status);

            results = KphLuaNilErrorStatus(State, status);
            goto Exit;
        }

        status = KphpLuaHwPciReadLocator(devices[i], &entry->Locator);
        if (status == STATUS_NOT_FOUND)
        {
            continue;
        }

        if (!NT_SUCCESS(status))
        {
            KphTracePrint(TRACE_LEVEL_VERBOSE,
                          HARDWARE,
                          "KphpLuaHwPciReadLocator failed: %!STATUS!",
                          status);

            results = KphLuaNilErrorStatus(State, status);
            goto Exit;
        }

        status = KphpLuaHwPciReadClassCode(devices[i],
                                           &entry->BaseClass,
                                           &entry->SubClass,
                                           &entry->ProgIf);
        if (status == STATUS_NOT_FOUND)
        {
            entry->BaseClass = 0;
            entry->SubClass = 0;
            entry->ProgIf = 0;
        }
        else if (!NT_SUCCESS(status))
        {
            KphTracePrint(TRACE_LEVEL_VERBOSE,
                          HARDWARE,
                          "KphpLuaHwPciReadClassCode failed: %!STATUS!",
                          status);

            results = KphLuaNilErrorStatus(State, status);
            goto Exit;
        }

        found++;
    }

    KphLuaHwDereferenceDevices(devices, count);
    devices = NULL;

    KphScriptPushCleanup(State, KphpLuaHwFreePciList, entries);
    cleanup = lua_gettop(State);

    KphScriptCharge(State, count);

    NT_VERIFY(NT_SUCCESS(RtlULongToInt(found, &size)));

    lua_createtable(State, size, 0);

    for (ULONG i = 0; i < found; i++)
    {
        PKPH_LUA_HW_PCI_ENTRY entry;

        entry = &entries[i];

        lua_createtable(State, 0, 9);
        lua_pushinteger(State, entry->Locator.Segment);
        lua_setfield(State, -2, "segment");
        lua_pushinteger(State, entry->Locator.Bus);
        lua_setfield(State, -2, "bus");
        lua_pushinteger(State, entry->Locator.Device);
        lua_setfield(State, -2, "device");
        lua_pushinteger(State, entry->Locator.Function);
        lua_setfield(State, -2, "fn");
        lua_pushinteger(State, entry->VendorId);
        lua_setfield(State, -2, "vendor");
        lua_pushinteger(State, entry->DeviceId);
        lua_setfield(State, -2, "device_id");
        lua_pushinteger(State, entry->BaseClass);
        lua_setfield(State, -2, "class");
        lua_pushinteger(State, entry->SubClass);
        lua_setfield(State, -2, "subclass");
        lua_pushinteger(State, entry->ProgIf);
        lua_setfield(State, -2, "prog_if");

        lua_rawseti(State, -2, (lua_Integer)i + 1);
    }

    KphScriptReleaseCleanup(State, cleanup);
    lua_remove(State, cleanup);
    entries = NULL;

    results = 1;

Exit:

    if (entries)
    {
        KphFree(entries, KPH_TAG_LUA_HARDWARE_PCI_LIST);
    }

    if (devices)
    {
        KphLuaHwDereferenceDevices(devices, count);
    }

    return results;
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
NTSTATUS KphpLuaHwPciSendPnp(
    _In_ PDEVICE_OBJECT Top,
    _In_ UCHAR MinorFunction,
    _In_ PIO_STACK_LOCATION Parameters,
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
NTSTATUS KphpLuaHwPciConfirmPdo(
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

    status = KphpLuaHwPciSendPnp(Top,
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
NTSTATUS KphpLuaHwPciQueryBusInterface(
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

    status = KphpLuaHwPciSendPnp(Top,
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

        status = STATUS_NOT_FOUND;
        goto Exit;
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

        status = STATUS_NOT_FOUND;
        goto Exit;
    }

    status = STATUS_SUCCESS;

Exit:

    return status;
}

/**
 * \brief Releases a device object of ksi.hw.pci_open.
 *
 * \param[in] Context The device.
 */
_Function_class_(KPH_SCRIPT_CLEANUP_ROUTINE)
_IRQL_requires_max_(PASSIVE_LEVEL)
VOID KSIAPI KphpLuaHwReleasePciDevice(
    _In_opt_ PVOID Context
    )
{
    PKPH_LUA_HW_PCI_DEVICE device;

    KPH_PAGED_CODE_PASSIVE();

    NT_ASSERT(Context);

    device = Context;

    if (device->Bus.InterfaceDereference)
    {
        device->Bus.InterfaceDereference(device->Bus.Context);
    }

    if (device->Top)
    {
        ObDereferenceObject(device->Top);
    }

    if (device->Pdo)
    {
        ObDereferenceObject(device->Pdo);
    }

    KphFreeToPagedLookaside(&KphpLuaHwPciDeviceLookaside, device);
}

/**
 * \brief Finds and references the PCI physical device object at a locator.
 *
 * \param[in] Locator The locator of the device.
 * \param[out] Pdo Receives the referenced physical device object.
 * \param[out] Count Receives the number of device objects searched.
 *
 * \return Successful or errant status.
 */
_IRQL_requires_max_(PASSIVE_LEVEL)
_Must_inspect_result_
NTSTATUS KphpLuaHwPciFindPdo(
    _In_ PKPH_LUA_HW_PCI_LOCATOR Locator,
    _Outptr_ PDEVICE_OBJECT* Pdo,
    _Out_ PULONG Count
    )
{
    NTSTATUS status;
    PDEVICE_OBJECT* devices;
    ULONG count;
    KPH_LUA_HW_PCI_LOCATOR locator;
    PDEVICE_OBJECT pdo;

    KPH_PAGED_CODE_PASSIVE();

    *Pdo = NULL;
    *Count = 0;

    pdo = NULL;

    status = KphLuaHwReferenceDevices(&KphpLuaHwPciDriverName,
                                      &devices,
                                      &count);
    if (!NT_SUCCESS(status))
    {
        KphTracePrint(TRACE_LEVEL_VERBOSE,
                      HARDWARE,
                      "KphLuaHwReferenceDevices failed: %!STATUS!",
                      status);

        return status;
    }

    *Count = count;

    status = STATUS_NOT_FOUND;

    for (ULONG i = 0; i < count; i++)
    {
        if (!BooleanFlagOn(devices[i]->Flags, DO_BUS_ENUMERATED_DEVICE))
        {
            continue;
        }

        status = KphpLuaHwPciReadLocator(devices[i], &locator);
        if (!NT_SUCCESS(status))
        {
            continue;
        }

        if ((locator.Segment == Locator->Segment) &&
            (locator.Bus == Locator->Bus) &&
            (locator.Device == Locator->Device) &&
            (locator.Function == Locator->Function))
        {
            pdo = devices[i];
            ObReferenceObject(pdo);
            status = STATUS_SUCCESS;
            break;
        }
    }

    if (!pdo)
    {
        status = STATUS_NOT_FOUND;
    }

    KphLuaHwDereferenceDevices(devices, count);

    if (!NT_SUCCESS(status))
    {
        KphTracePrint(TRACE_LEVEL_VERBOSE,
                      HARDWARE,
                      "No PCI device at %04lx:%02lx:%02lx.%lx",
                      Locator->Segment,
                      Locator->Bus,
                      Locator->Device,
                      Locator->Function);
    }

    *Pdo = pdo;

    return status;
}

/**
 * \brief Opens a PCI function at a location (ksi.hw.pci_open).
 *
 * \details May raise a Lua error.
 *
 * \param[in] State The Lua state.
 *
 * \return The number of results: the device object, or nil and a status.
 */
_IRQL_requires_max_(PASSIVE_LEVEL)
int KphpLuaHwPciOpen(
    _In_ lua_State* State
    )
{
    int results;
    NTSTATUS status;
    lua_Integer segment;
    lua_Integer bus;
    lua_Integer slot;
    lua_Integer function;
    KPH_LUA_HW_PCI_LOCATOR locator;
    PKPH_LUA_HW_PCI_DEVICE device;
    ULONG count;

    KPH_PAGED_CODE_PASSIVE();

    count = 0;

    segment = luaL_checkinteger(State, 1);
    bus = luaL_checkinteger(State, 2);
    slot = luaL_checkinteger(State, 3);
    function = luaL_checkinteger(State, 4);

    luaL_argcheck(State,
                  (segment >= 0) && (segment <= USHORT_MAX),
                  1,
                  "out of range");
    luaL_argcheck(State, (bus >= 0) && (bus <= 255), 2, "out of range");
    luaL_argcheck(State, (slot >= 0) && (slot <= 31), 3, "out of range");
    luaL_argcheck(State, (function >= 0) && (function <= 7), 4, "out of range");

    locator.Segment = (ULONG)segment;
    locator.Bus = (ULONG)bus;
    locator.Device = (ULONG)slot;
    locator.Function = (ULONG)function;

    device = KphAllocateFromPagedLookaside(&KphpLuaHwPciDeviceLookaside);
    if (!device)
    {
        KphTracePrint(TRACE_LEVEL_VERBOSE,
                      HARDWARE,
                      "Failed to allocate a PCI device");

        status = STATUS_INSUFFICIENT_RESOURCES;
        results = KphLuaNilErrorStatus(State, status);
        goto Exit;
    }

    NT_ASSERT(sizeof(KPH_LUA_HW_PCI_DEVICE) <= KphpLuaHwPciDeviceLookaside.L.Size);

    status = KphpLuaHwPciFindPdo(&locator, &device->Pdo, &count);
    if (!NT_SUCCESS(status))
    {
        KphTracePrint(TRACE_LEVEL_VERBOSE,
                      HARDWARE,
                      "KphpLuaHwPciFindPdo failed: %!STATUS!",
                      status);

        results = KphLuaNilErrorStatus(State, status);
        goto Exit;
    }

    device->Top = IoGetAttachedDeviceReference(device->Pdo);

    status = KphpLuaHwPciConfirmPdo(device->Top, device->Pdo);
    if (!NT_SUCCESS(status))
    {
        KphTracePrint(TRACE_LEVEL_VERBOSE,
                      HARDWARE,
                      "KphpLuaHwPciConfirmPdo failed: %!STATUS!",
                      status);

        results = KphLuaNilErrorStatus(State, status);
        goto Exit;
    }

    status = KphpLuaHwPciQueryBusInterface(device->Top, &device->Bus);
    if (!NT_SUCCESS(status))
    {
        KphTracePrint(TRACE_LEVEL_VERBOSE,
                      HARDWARE,
                      "KphpLuaHwPciQueryBusInterface failed: %!STATUS!",
                      status);

        results = KphLuaNilErrorStatus(State, status);
        goto Exit;
    }

    KphScriptPushObject(State, KphpLuaHwReleasePciDevice, device);
    device = NULL;

    results = 1;

Exit:

    if (device)
    {
        KphpLuaHwReleasePciDevice(device);
    }

    KphScriptCharge(State, count);

    return results;
}

/**
 * \brief Checks an offset and a length within configuration space.
 *
 * \details May raise a Lua error.
 *
 * \param[in] State The Lua state.
 * \param[in] OffsetIndex The index of the offset argument.
 * \param[in] Length The number of bytes.
 *
 * \return The offset.
 */
_IRQL_requires_max_(PASSIVE_LEVEL)
ULONG KphpLuaHwPciCheckOffset(
    _In_ lua_State* State,
    _In_ int OffsetIndex,
    _In_ ULONG Length
    )
{
    lua_Integer offset;

    KPH_PAGED_CODE_PASSIVE();

    offset = luaL_checkinteger(State, OffsetIndex);

    luaL_argcheck(State,
                  (offset >= 0) &&
                  (offset <= KPH_LUA_HW_PCI_CONFIG_LENGTH) &&
                  (Length <= (KPH_LUA_HW_PCI_CONFIG_LENGTH - (ULONG)offset)),
                  OffsetIndex,
                  "out of range");

    return (ULONG)offset;
}

/**
 * \brief Checks an access width argument.
 *
 * \details May raise a Lua error.
 *
 * \param[in] State The Lua state.
 * \param[in] Index The index of the argument.
 *
 * \return The width: 1, 2 or 4.
 */
_IRQL_requires_max_(PASSIVE_LEVEL)
ULONG KphpLuaHwPciCheckWidth(
    _In_ lua_State* State,
    _In_ int Index
    )
{
    lua_Integer width;

    KPH_PAGED_CODE_PASSIVE();

    width = luaL_checkinteger(State, Index);

    luaL_argcheck(State,
                  (width == 1) || (width == 2) || (width == 4),
                  Index,
                  "1, 2 or 4");

    return (ULONG)width;
}

/**
 * \brief Retrieves the status of a configuration access the bus driver
 * refused.
 *
 * \param[in] Device The device.
 *
 * \return STATUS_DEVICE_POWERED_OFF when the function is not in D0,
 * STATUS_IO_DEVICE_ERROR otherwise.
 */
_IRQL_requires_max_(PASSIVE_LEVEL)
NTSTATUS KphpLuaHwPciRefusedStatus(
    _In_ PKPH_LUA_HW_PCI_DEVICE Device
    )
{
    NTSTATUS status;
    CM_POWER_DATA power;
    ULONG length;
    DEVPROPTYPE type;

    KPH_PAGED_CODE_PASSIVE();

    RtlZeroMemory(&power, sizeof(power));
    length = 0;
    type = DEVPROP_TYPE_EMPTY;

    status = IoGetDevicePropertyData(Device->Pdo,
                                     &DEVPKEY_Device_PowerData,
                                     LOCALE_NEUTRAL,
                                     0,
                                     sizeof(power),
                                     &power,
                                     &length,
                                     &type);
    if (!NT_SUCCESS(status) ||
        (type != DEVPROP_TYPE_BINARY) ||
        (length != sizeof(power)))
    {
        KphTracePrint(TRACE_LEVEL_VERBOSE,
                      HARDWARE,
                      "PowerData query: %!STATUS! type %lu length %lu",
                      status,
                      (ULONG)type,
                      length);

        return STATUS_IO_DEVICE_ERROR;
    }

    if (power.PD_MostRecentPowerState != PowerDeviceD0)
    {
        return STATUS_DEVICE_POWERED_OFF;
    }

    return STATUS_IO_DEVICE_ERROR;
}

/**
 * \brief Reads configuration space (ksi.hw.pci_read).
 *
 * \details May raise a Lua error.
 *
 * \param[in] State The Lua state.
 *
 * \return The number of results: the value, or nil and a status.
 */
_IRQL_requires_max_(PASSIVE_LEVEL)
int KphpLuaHwPciRead(
    _In_ lua_State* State
    )
{
    int results;
    NTSTATUS status;
    PKPH_LUA_HW_PCI_DEVICE device;
    ULONG width;
    ULONG offset;
    ULONG value;
    ULONG bytes;

    KPH_PAGED_CODE_PASSIVE();

    device = KphScriptCheckCleanup(State, 1, KphpLuaHwReleasePciDevice);
    width = KphpLuaHwPciCheckWidth(State, 3);
    offset = KphpLuaHwPciCheckOffset(State, 2, width);

    value = 0;

    bytes = device->Bus.GetBusData(device->Bus.Context,
                                   PCI_WHICHSPACE_CONFIG,
                                   &value,
                                   offset,
                                   width);
    if (bytes != width)
    {
        KphTracePrint(TRACE_LEVEL_VERBOSE,
                      HARDWARE,
                      "Configuration read 0x%lx of %lu bytes gave %lu",
                      offset,
                      width,
                      bytes);

        status = KphpLuaHwPciRefusedStatus(device);

        results = KphLuaNilErrorStatus(State, status);
        goto Exit;
    }

    lua_pushinteger(State, value);

    results = 1;

Exit:

    return results;
}

/**
 * \brief Reads a block of configuration space (ksi.hw.pci_read_block).
 *
 * \details May raise a Lua error.
 *
 * \param[in] State The Lua state.
 *
 * \return The number of results: the bytes, or nil and a status.
 */
_IRQL_requires_max_(PASSIVE_LEVEL)
int KphpLuaHwPciReadBlock(
    _In_ lua_State* State
    )
{
    int results;
    NTSTATUS status;
    PKPH_LUA_HW_PCI_DEVICE device;
    lua_Integer length;
    ULONG offset;
    ULONG bytes;
    luaL_Buffer buffer;
    PCHAR data;

    KPH_PAGED_CODE_PASSIVE();

    device = KphScriptCheckCleanup(State, 1, KphpLuaHwReleasePciDevice);

    length = luaL_checkinteger(State, 3);

    luaL_argcheck(State,
                  (length >= 1) && (length <= KPH_LUA_HW_PCI_CONFIG_LENGTH),
                  3,
                  "out of range");

    offset = KphpLuaHwPciCheckOffset(State, 2, (ULONG)length);

    NT_ASSERT(length >= 1);

    KphScriptCharge(State, (ULONG64)length);

    data = luaL_buffinitsize(State, &buffer, (SIZE_T)length);

    bytes = device->Bus.GetBusData(device->Bus.Context,
                                   PCI_WHICHSPACE_CONFIG,
                                   data,
                                   offset,
                                   (ULONG)length);
    if (bytes != (ULONG)length)
    {
        KphTracePrint(TRACE_LEVEL_VERBOSE,
                      HARDWARE,
                      "Configuration read 0x%lx of %lu bytes gave %lu",
                      offset,
                      (ULONG)length,
                      bytes);

        status = KphpLuaHwPciRefusedStatus(device);

        results = KphLuaNilErrorStatus(State, status);
        goto Exit;
    }

    luaL_pushresultsize(&buffer, (SIZE_T)length);

    results = 1;

Exit:

    return results;
}

/**
 * \brief Writes configuration space (ksi.hw.pci_write).
 *
 * \details May raise a Lua error.
 *
 * \param[in] State The Lua state.
 *
 * \return The number of results: true, or nil and a status.
 */
_IRQL_requires_max_(PASSIVE_LEVEL)
int KphpLuaHwPciWrite(
    _In_ lua_State* State
    )
{
    int results;
    NTSTATUS status;
    PKPH_LUA_HW_PCI_DEVICE device;
    ULONG width;
    ULONG offset;
    lua_Integer value;
    ULONG data;
    ULONG bytes;

    KPH_PAGED_CODE_PASSIVE();

    device = KphScriptCheckCleanup(State, 1, KphpLuaHwReleasePciDevice);
    width = KphpLuaHwPciCheckWidth(State, 3);
    offset = KphpLuaHwPciCheckOffset(State, 2, width);
    value = luaL_checkinteger(State, 4);

    luaL_argcheck(State,
                  (value >= 0) &&
                  ((ULONG64)value <= (ULONG_MAX >> ((4 - width) * 8))),
                  4,
                  "out of range");

    data = (ULONG)value;

    bytes = device->Bus.SetBusData(device->Bus.Context,
                                   PCI_WHICHSPACE_CONFIG,
                                   &data,
                                   offset,
                                   width);
    if (bytes != width)
    {
        KphTracePrint(TRACE_LEVEL_VERBOSE,
                      HARDWARE,
                      "Configuration write 0x%lx of %lu bytes gave %lu",
                      offset,
                      width,
                      bytes);

        status = KphpLuaHwPciRefusedStatus(device);

        results = KphLuaNilErrorStatus(State, status);
        goto Exit;
    }

    lua_pushboolean(State, TRUE);

    results = 1;

Exit:

    return results;
}

/**
 * \brief Lists the memory and port resources of a PCI function
 * (ksi.hw.pci_resources).
 *
 * \details May raise a Lua error.
 *
 * \param[in] State The Lua state.
 *
 * \return The number of results: the array of resources, or nil and a
 * status.
 */
_IRQL_requires_max_(PASSIVE_LEVEL)
int KphpLuaHwPciResources(
    _In_ lua_State* State
    )
{
    PKPH_LUA_HW_PCI_DEVICE device;

    KPH_PAGED_CODE_PASSIVE();

    device = KphScriptCheckCleanup(State, 1, KphpLuaHwReleasePciDevice);

    return KphLuaHwPushResources(State, device->Pdo);
}

/**
 * \brief Tells whether a PCI function is expected never to be removed
 * (ksi.hw.pci_fixed).
 *
 * \details May raise a Lua error.
 *
 * \param[in] State The Lua state.
 *
 * \return One, a boolean.
 */
_IRQL_requires_max_(PASSIVE_LEVEL)
int KphpLuaHwPciFixed(
    _In_ lua_State* State
    )
{
    NTSTATUS status;
    PKPH_LUA_HW_PCI_DEVICE device;
    DEVICE_REMOVAL_POLICY policy;
    ULONG length;

    KPH_PAGED_CODE_PASSIVE();

    device = KphScriptCheckCleanup(State, 1, KphpLuaHwReleasePciDevice);

    status = IoGetDeviceProperty(device->Pdo,
                                 DevicePropertyRemovalPolicy,
                                 sizeof(policy),
                                 &policy,
                                 &length);
    if (!NT_SUCCESS(status))
    {
        KphTracePrint(TRACE_LEVEL_VERBOSE,
                      HARDWARE,
                      "IoGetDeviceProperty failed: %!STATUS!",
                      status);

        policy = RemovalPolicyExpectOrderlyRemoval;
    }

    lua_pushboolean(State, (policy == RemovalPolicyExpectNoRemoval));

    return 1;
}

/**
 * \brief Tells whether a value is a device object of ksi.hw.pci_open.
 *
 * \param[in] State The Lua state.
 * \param[in] Index The index of the value.
 *
 * \return The physical device object, valid while the object is, or NULL.
 */
_IRQL_requires_max_(PASSIVE_LEVEL)
PDEVICE_OBJECT KphLuaHwPciTestDevice(
    _In_ lua_State* State,
    _In_ int Index
    )
{
    PKPH_LUA_HW_PCI_DEVICE device;

    KPH_PAGED_CODE_PASSIVE();

    device = KphScriptTestCleanup(State, Index, KphpLuaHwReleasePciDevice);
    if (!device)
    {
        return NULL;
    }

    return device->Pdo;
}

KPH_PROTECTED_DATA_SECTION_RO_PUSH();
static const luaL_Reg KphpLuaHwPciLibrary[] =
{
    { "pci_devices", KphpLuaHwPciDevices },
    { "pci_fixed", KphpLuaHwPciFixed },
    { "pci_open", KphpLuaHwPciOpen },
    { "pci_read", KphpLuaHwPciRead },
    { "pci_read_block", KphpLuaHwPciReadBlock },
    { "pci_resources", KphpLuaHwPciResources },
    { "pci_write", KphpLuaHwPciWrite },
    { NULL, NULL }
};
KPH_PROTECTED_DATA_SECTION_RO_POP();

/**
 * \brief Initializes the PCI functions of the hardware library.
 */
_IRQL_requires_max_(PASSIVE_LEVEL)
VOID KphLuaInitializeHardwarePci(
    VOID
    )
{
    KPH_PAGED_CODE_PASSIVE();

    KphInitializePagedLookaside(&KphpLuaHwPciDeviceLookaside,
                                sizeof(KPH_LUA_HW_PCI_DEVICE),
                                KPH_TAG_LUA_HARDWARE_PCI);
}

/**
 * \brief Cleans up the PCI functions of the hardware library.
 */
_IRQL_requires_max_(PASSIVE_LEVEL)
VOID KphLuaCleanupHardwarePci(
    VOID
    )
{
    KPH_PAGED_CODE_PASSIVE();

    KphDeletePagedLookaside(&KphpLuaHwPciDeviceLookaside);
}

/**
 * \brief Adds the PCI functions to the hardware library.
 *
 * \details May raise a Lua error.
 *
 * \param[in] State The Lua state, with the library table on top.
 */
_IRQL_requires_max_(PASSIVE_LEVEL)
VOID KphLuaOpenHardwarePci(
    _In_ lua_State* State
    )
{
    KPH_PAGED_CODE_PASSIVE();

    luaL_setfuncs(State, KphpLuaHwPciLibrary, 0);
}
