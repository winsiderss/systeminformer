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

typedef struct _KPH_LUA_HW_PCI_DEVICE
{
    PDEVICE_OBJECT Pdo;
    BUS_INTERFACE_STANDARD Bus;
} KPH_LUA_HW_PCI_DEVICE, *PKPH_LUA_HW_PCI_DEVICE;

KPH_PROTECTED_DATA_SECTION_RO_PUSH();
DECLARE_CONST_UNICODE_STRING(KphpLuaHwPciDriverName, L"\\Driver\\pci");
KPH_PROTECTED_DATA_SECTION_RO_POP();
static PAGED_LOOKASIDE_LIST KphpLuaHwPciDeviceLookaside = { 0 };

KPH_PAGED_FILE();

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

    if (device->Pdo)
    {
        ObDereferenceObject(device->Pdo);
    }

    KphFreeToPagedLookaside(&KphpLuaHwPciDeviceLookaside, device);
}

/**
 * \brief Opens the PCI function of a device (ksi.hw.pci_open).
 *
 * \details May raise a Lua error.
 *
 * \param[in] State The Lua state.
 *
 * \return The number of results: the PCI device object, or nil and a status.
 */
_IRQL_requires_max_(PASSIVE_LEVEL)
int KphpLuaHwPciOpen(
    _In_ lua_State* State
    )
{
    int results;
    NTSTATUS status;
    PDEVICE_OBJECT pdo;
    PDRIVER_OBJECT driver;
    BOOLEAN pci;
    PKPH_LUA_HW_PCI_DEVICE device;
    PDEVICE_OBJECT top;

    KPH_PAGED_CODE_PASSIVE();

    pdo = KphLuaHwTestDevice(State, 1);

    luaL_argcheck(State, pdo, 1, "a device object");

    NT_ASSERT(pdo);

    top = NULL;
    device = NULL;

    status = ObReferenceObjectByName((PUNICODE_STRING)&KphpLuaHwPciDriverName,
                                     OBJ_DONT_REPARSE,
                                     NULL,
                                     0,
                                     *IoDriverObjectType,
                                     KernelMode,
                                     NULL,
                                     (PVOID*)&driver);
    if (!NT_SUCCESS(status))
    {
        KphTracePrint(TRACE_LEVEL_VERBOSE,
                      HARDWARE,
                      "ObReferenceObjectByName failed: %!STATUS!",
                      status);

        results = KphLuaNilErrorStatus(State, status);
        goto Exit;
    }

    pci = (pdo->DriverObject == driver);

    ObDereferenceObject(driver);

    luaL_argcheck(State, pci, 1, "a device of the PCI bus driver");

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

    NT_ASSERT(sizeof(KPH_LUA_HW_PCI_DEVICE) <=
              KphpLuaHwPciDeviceLookaside.L.Size);

    ObReferenceObject(pdo);
    device->Pdo = pdo;

    top = IoGetAttachedDeviceReference(device->Pdo);

    status = KphpLuaHwPciConfirmPdo(top, device->Pdo);
    if (!NT_SUCCESS(status))
    {
        KphTracePrint(TRACE_LEVEL_VERBOSE,
                      HARDWARE,
                      "KphpLuaHwPciConfirmPdo failed: %!STATUS!",
                      status);

        results = KphLuaNilErrorStatus(State, status);
        goto Exit;
    }

    status = KphpLuaHwPciQueryBusInterface(top, &device->Bus);
    if (!NT_SUCCESS(status))
    {
        KphTracePrint(TRACE_LEVEL_VERBOSE,
                      HARDWARE,
                      "KphpLuaHwPciQueryBusInterface failed: %!STATUS!",
                      status);

        results = KphLuaNilErrorStatus(State, status);
        goto Exit;
    }

    ObDereferenceObject(top);
    top = NULL;

    KphScriptPushObject(State, KphpLuaHwReleasePciDevice, device);
    device = NULL;

    results = 1;

Exit:

    if (top)
    {
        ObDereferenceObject(top);
    }

    if (device)
    {
        KphpLuaHwReleasePciDevice(device);
    }

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
