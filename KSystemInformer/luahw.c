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

#include <trace.h>

#define KPH_LUA_HARDWARE_LIBRARY_NAME "hw"

static BOOLEAN KphpLuaHwInitialized = FALSE;

KPH_PAGED_FILE();

/**
 * \brief Initializes the hardware library.
 */
_IRQL_requires_max_(PASSIVE_LEVEL)
VOID KphLuaInitializeHardware(
    VOID
    )
{
    KPH_PAGED_CODE_PASSIVE();

    KphLuaInitializeHardwareCpu();
    KphLuaInitializeHardwarePci();
    KphLuaInitializeHardwareMemory();
    KphLuaInitializeHardwareSysreg();

    KphpLuaHwInitialized = TRUE;
}

/**
 * \brief Cleans up the hardware library.
 */
_IRQL_requires_max_(PASSIVE_LEVEL)
VOID KphLuaCleanupHardware(
    VOID
    )
{
    KPH_PAGED_CODE_PASSIVE();

    if (!KphpLuaHwInitialized)
    {
        return;
    }

    KphLuaCleanupHardwareSysreg();
    KphLuaCleanupHardwareMemory();
    KphLuaCleanupHardwarePci();
    KphLuaCleanupHardwareCpu();
}

/**
 * \brief Checks that an argument is an integer from 0 to ULONG_MAX.
 *
 * \details May raise a Lua error.
 *
 * \param[in] State The Lua state.
 * \param[in] Index The index of the argument.
 *
 * \return The argument.
 */
_IRQL_requires_max_(PASSIVE_LEVEL)
ULONG KphpLuaHwCheckUlong(
    _In_ lua_State* State,
    _In_ int Index
    )
{
    lua_Integer value;

    KPH_PAGED_CODE_PASSIVE();

    value = luaL_checkinteger(State, Index);

    luaL_argcheck(State,
                  (value >= 0) && (value <= ULONG_MAX),
                  Index,
                  "out of range");

    return (ULONG)value;
}

/**
 * \brief Checks a four-character argument.
 *
 * \details May raise a Lua error.
 *
 * \param[in] State The Lua state.
 * \param[in] Index The index of the argument.
 *
 * \return The characters.
 */
_IRQL_requires_max_(PASSIVE_LEVEL)
PCSTR KphpLuaHwCheckSignature(
    _In_ lua_State* State,
    _In_ int Index
    )
{
    PCSTR signature;
    size_t length;

    KPH_PAGED_CODE_PASSIVE();

    signature = luaL_checklstring(State, Index, &length);

    luaL_argcheck(State, length == 4, Index, "not 4 characters");

    return signature;
}

/**
 * \brief Retrieves a firmware table (ksi.hw.firmware_table).
 *
 * \details May raise a Lua error.
 *
 * \param[in] State The Lua state.
 *
 * \return The number of results: the table, or nil and a status.
 */
_IRQL_requires_max_(PASSIVE_LEVEL)
int KphpLuaHwFirmwareTable(
    _In_ lua_State* State
    )
{
    int results;
    NTSTATUS status;
    PCSTR provider;
    PCSTR id;
    SYSTEM_FIRMWARE_TABLE_INFORMATION query;
    PSYSTEM_FIRMWARE_TABLE_INFORMATION info;
    ULONG headerLength;
    ULONG infoLength;
    ULONG tableLength;
    luaL_Buffer buffer;

    KPH_PAGED_CODE_PASSIVE();

    RtlZeroMemory(&query, sizeof(query));

    provider = KphpLuaHwCheckSignature(State, 1);

    query.ProviderSignature = (((ULONG)(UCHAR)provider[0] << 24) |
                               ((ULONG)(UCHAR)provider[1] << 16) |
                               ((ULONG)(UCHAR)provider[2] << 8) |
                               (ULONG)(UCHAR)provider[3]);

    if (lua_type(State, 2) == LUA_TSTRING)
    {
        id = KphpLuaHwCheckSignature(State, 2);

        query.TableID = ((ULONG)(UCHAR)id[0] |
                         ((ULONG)(UCHAR)id[1] << 8) |
                         ((ULONG)(UCHAR)id[2] << 16) |
                         ((ULONG)(UCHAR)id[3] << 24));
    }
    else
    {
        query.TableID = KphpLuaHwCheckUlong(State, 2);
    }

    query.Action = SystemFirmwareTable_Get;

    status = ZwQuerySystemInformation(SystemFirmwareTableInformation,
                                      &query,
                                      sizeof(query),
                                      NULL);
    if (NT_SUCCESS(status) && (query.TableBufferLength == 0))
    {
        KphTracePrint(TRACE_LEVEL_VERBOSE,
                      HARDWARE,
                      "Firmware table 0x%08lx 0x%08lx is empty",
                      query.ProviderSignature,
                      query.TableID);

        status = STATUS_NOT_FOUND;
        results = KphLuaNilErrorStatus(State, status);
        goto Exit;
    }

    if ((status != STATUS_BUFFER_TOO_SMALL) || (query.TableBufferLength == 0))
    {
        KphTracePrint(TRACE_LEVEL_VERBOSE,
                      HARDWARE,
                      "Firmware table 0x%08lx 0x%08lx: %!STATUS! length %lu",
                      query.ProviderSignature,
                      query.TableID,
                      status,
                      query.TableBufferLength);

        if (NT_SUCCESS(status))
        {
            status = STATUS_UNSUCCESSFUL;
        }

        results = KphLuaNilErrorStatus(State, status);
        goto Exit;
    }

    tableLength = query.TableBufferLength;
    headerLength = FIELD_OFFSET(SYSTEM_FIRMWARE_TABLE_INFORMATION, TableBuffer);

    status = RtlULongAdd(headerLength, tableLength, &infoLength);
    if (!NT_SUCCESS(status))
    {
        KphTracePrint(TRACE_LEVEL_VERBOSE,
                      HARDWARE,
                      "RtlULongAdd failed: %!STATUS!",
                      status);

        results = KphLuaNilErrorStatus(State, status);
        goto Exit;
    }

    info = (PSYSTEM_FIRMWARE_TABLE_INFORMATION)luaL_buffinitsize(State,
                                                                 &buffer,
                                                                 infoLength);

    info->ProviderSignature = query.ProviderSignature;
    info->Action = SystemFirmwareTable_Get;
    info->TableID = query.TableID;
    info->TableBufferLength = tableLength;

    status = ZwQuerySystemInformation(SystemFirmwareTableInformation,
                                      info,
                                      infoLength,
                                      NULL);
    if (!NT_SUCCESS(status))
    {
        KphTracePrint(TRACE_LEVEL_VERBOSE,
                      HARDWARE,
                      "Firmware table 0x%08lx 0x%08lx: %!STATUS!",
                      query.ProviderSignature,
                      query.TableID,
                      status);

        results = KphLuaNilErrorStatus(State, status);
        goto Exit;
    }

    if (info->TableBufferLength > tableLength)
    {
        KphTracePrint(TRACE_LEVEL_VERBOSE,
                      HARDWARE,
                      "Firmware table 0x%08lx 0x%08lx grew to %lu",
                      query.ProviderSignature,
                      query.TableID,
                      info->TableBufferLength);

        status = STATUS_BUFFER_OVERFLOW;
        results = KphLuaNilErrorStatus(State, status);
        goto Exit;
    }

    tableLength = info->TableBufferLength;

    RtlMoveMemory(info, info->TableBuffer, tableLength);

    luaL_pushresultsize(&buffer, tableLength);

    results = 1;

Exit:

    return results;
}

#if defined(_M_X64)

/**
 * \brief Checks the width argument of a port access: 1, 2 or 4.
 *
 * \details May raise a Lua error.
 *
 * \param[in] State The Lua state.
 * \param[in] Index The index of the argument.
 *
 * \return The width in bytes.
 */
_IRQL_requires_max_(PASSIVE_LEVEL)
ULONG KphpLuaHwCheckWidth(
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
                  "not 1, 2 or 4");

    return (ULONG)width;
}

/**
 * \brief Checks the port argument of a port access.
 *
 * \details May raise a Lua error.
 *
 * \param[in] State The Lua state.
 * \param[in] Index The index of the argument.
 *
 * \return The port.
 */
_IRQL_requires_max_(PASSIVE_LEVEL)
USHORT KphpLuaHwCheckPort(
    _In_ lua_State* State,
    _In_ int Index
    )
{
    lua_Integer port;

    KPH_PAGED_CODE_PASSIVE();

    port = luaL_checkinteger(State, Index);

    luaL_argcheck(State,
                  (port >= 0) && (port <= USHORT_MAX),
                  Index,
                  "out of range");

    return (USHORT)port;
}

/**
 * \brief Reads an I/O port (ksi.hw.port_read).
 *
 * \details May raise a Lua error.
 *
 * \param[in] State The Lua state.
 *
 * \return One, the value read.
 */
_IRQL_requires_max_(PASSIVE_LEVEL)
int KphpLuaHwPortRead(
    _In_ lua_State* State
    )
{
    USHORT port;
    ULONG width;
    ULONG value;

    KPH_PAGED_CODE_PASSIVE();

    port = KphpLuaHwCheckPort(State, 1);
    width = KphpLuaHwCheckWidth(State, 2);

    switch (width)
    {
        case 1:
        {
            value = READ_PORT_UCHAR((PUCHAR)(ULONG_PTR)port);
            break;
        }
        case 2:
        {
            value = READ_PORT_USHORT((PUSHORT)(ULONG_PTR)port);
            break;
        }
        case 4:
        {
            value = READ_PORT_ULONG((PULONG)(ULONG_PTR)port);
            break;
        }
        DEFAULT_UNREACHABLE;
    }

    lua_pushinteger(State, (lua_Integer)value);

    return 1;
}

/**
 * \brief Writes an I/O port (ksi.hw.port_write).
 *
 * \details May raise a Lua error.
 *
 * \param[in] State The Lua state.
 *
 * \return Zero, the number of results.
 */
_IRQL_requires_max_(PASSIVE_LEVEL)
int KphpLuaHwPortWrite(
    _In_ lua_State* State
    )
{
    USHORT port;
    ULONG width;
    lua_Integer value;
    lua_Integer maximum;

    KPH_PAGED_CODE_PASSIVE();

    port = KphpLuaHwCheckPort(State, 1);
    width = KphpLuaHwCheckWidth(State, 2);
    value = luaL_checkinteger(State, 3);

    maximum = (lua_Integer)((1ull << (width * 8)) - 1);

    luaL_argcheck(State, (value >= 0) && (value <= maximum), 3, "out of range");

    switch (width)
    {
        case 1:
        {
            WRITE_PORT_UCHAR((PUCHAR)(ULONG_PTR)port, (UCHAR)value);
            break;
        }
        case 2:
        {
            WRITE_PORT_USHORT((PUSHORT)(ULONG_PTR)port, (USHORT)value);
            break;
        }
        case 4:
        {
            WRITE_PORT_ULONG((PULONG)(ULONG_PTR)port, (ULONG)value);
            break;
        }
        DEFAULT_UNREACHABLE;
    }

    return 0;
}

#endif

/**
 * \brief References the device objects of a driver.
 *
 * \param[in] DriverName The name of the driver object.
 * \param[out] Devices Receives the referenced device objects, released with
 * KphLuaHwDereferenceDevices.
 * \param[out] Count Receives the number of device objects.
 *
 * \return Successful or errant status; STATUS_NOT_FOUND when the driver does
 * not exist or has no device objects.
 */
_IRQL_requires_max_(PASSIVE_LEVEL)
_Must_inspect_result_
NTSTATUS KphLuaHwReferenceDevices(
    _In_ PCUNICODE_STRING DriverName,
    _Outptr_result_buffer_(*Count) PDEVICE_OBJECT** Devices,
    _Out_ PULONG Count
    )
{
    NTSTATUS status;
    PDRIVER_OBJECT driver;
    PDEVICE_OBJECT* devices;
    ULONG count;
    ULONG length;
    ULONG capacity;

    KPH_PAGED_CODE_PASSIVE();

    *Devices = NULL;
    *Count = 0;

    driver = NULL;
    devices = NULL;

    status = ObReferenceObjectByName((PUNICODE_STRING)DriverName,
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

        driver = NULL;
        status = STATUS_NOT_FOUND;
        goto Exit;
    }

    count = 0;

    status = IoEnumerateDeviceObjectList(driver, NULL, 0, &count);
    if ((status != STATUS_BUFFER_TOO_SMALL) || !count)
    {
        KphTracePrint(TRACE_LEVEL_VERBOSE,
                      HARDWARE,
                      "IoEnumerateDeviceObjectList returned: %!STATUS!",
                      status);

        if (NT_SUCCESS(status))
        {
            status = STATUS_NOT_FOUND;
        }

        goto Exit;
    }

    for (ULONG attempt = 0; attempt < 2; attempt++)
    {
        status = RtlULongMult(count, sizeof(PDEVICE_OBJECT), &length);
        if (!NT_SUCCESS(status))
        {
            KphTracePrint(TRACE_LEVEL_VERBOSE,
                          HARDWARE,
                          "RtlULongMult failed: %!STATUS!",
                          status);

            goto Exit;
        }

        devices = KphAllocateNPaged(length, KPH_TAG_LUA_HARDWARE_DEVICES);
        if (!devices)
        {
            KphTracePrint(TRACE_LEVEL_VERBOSE,
                          HARDWARE,
                          "Failed to allocate the device list");

            status = STATUS_INSUFFICIENT_RESOURCES;
            goto Exit;
        }

        status = IoEnumerateDeviceObjectList(driver, devices, length, &count);
        if (status != STATUS_BUFFER_TOO_SMALL)
        {
            break;
        }

        capacity = length / sizeof(PDEVICE_OBJECT);

        for (ULONG i = 0; (i < count) && (i < capacity); i++)
        {
            ObDereferenceObject(devices[i]);
        }

        KphFree(devices, KPH_TAG_LUA_HARDWARE_DEVICES);
        devices = NULL;
    }

    if (!NT_SUCCESS(status))
    {
        KphTracePrint(TRACE_LEVEL_VERBOSE,
                      HARDWARE,
                      "IoEnumerateDeviceObjectList failed: %!STATUS!",
                      status);

        goto Exit;
    }

    *Devices = devices;
    *Count = count;
    devices = NULL;

Exit:

    if (devices)
    {
        KphFree(devices, KPH_TAG_LUA_HARDWARE_DEVICES);
    }

    if (driver)
    {
        ObDereferenceObject(driver);
    }

    return status;
}

/**
 * \brief Releases the device objects of KphLuaHwReferenceDevices.
 *
 * \param[in] Devices The device objects.
 * \param[in] Count The number of device objects.
 */
_IRQL_requires_max_(PASSIVE_LEVEL)
VOID KphLuaHwDereferenceDevices(
    _In_reads_(Count) PDEVICE_OBJECT* Devices,
    _In_ ULONG Count
    )
{
    KPH_PAGED_CODE_PASSIVE();

    for (ULONG i = 0; i < Count; i++)
    {
        ObDereferenceObject(Devices[i]);
    }

    KphFree(Devices, KPH_TAG_LUA_HARDWARE_DEVICES);
}

/**
 * \brief Retrieves a property of a device of variable length.
 *
 * \param[in] Pdo The physical device object.
 * \param[in] Property The property.
 * \param[out] Buffer Receives the property followed by two zero characters,
 * freed with KPH_TAG_LUA_HARDWARE_PROPERTY.
 * \param[out] Length Receives the length of the property.
 *
 * \return Successful or errant status; STATUS_NOT_FOUND when the device has
 * no such property.
 */
_IRQL_requires_max_(PASSIVE_LEVEL)
_Must_inspect_result_
NTSTATUS KphLuaHwQueryProperty(
    _In_ PDEVICE_OBJECT Pdo,
    _In_ DEVICE_REGISTRY_PROPERTY Property,
    _Outptr_result_bytebuffer_(*Length) PVOID* Buffer,
    _Out_ PULONG Length
    )
{
    NTSTATUS status;
    PVOID buffer;
    ULONG length;
    ULONG allocationLength;

    KPH_PAGED_CODE_PASSIVE();

    *Buffer = NULL;
    *Length = 0;

    buffer = NULL;
    length = 0;

    status = IoGetDeviceProperty(Pdo, Property, 0, NULL, &length);
    if ((status != STATUS_BUFFER_TOO_SMALL) || !length)
    {
        KphTracePrint(TRACE_LEVEL_VERBOSE,
                      HARDWARE,
                      "IoGetDeviceProperty returned: %!STATUS!",
                      status);

        status = STATUS_NOT_FOUND;
        goto Exit;
    }

    status = RtlULongAdd(length, 2 * sizeof(UNICODE_NULL), &allocationLength);
    if (!NT_SUCCESS(status))
    {
        KphTracePrint(TRACE_LEVEL_VERBOSE,
                      HARDWARE,
                      "RtlULongAdd failed: %!STATUS!",
                      status);

        goto Exit;
    }

    buffer = KphAllocatePaged(allocationLength, KPH_TAG_LUA_HARDWARE_PROPERTY);
    if (!buffer)
    {
        KphTracePrint(TRACE_LEVEL_VERBOSE,
                      HARDWARE,
                      "Failed to allocate a device property");

        status = STATUS_INSUFFICIENT_RESOURCES;
        goto Exit;
    }

    status = IoGetDeviceProperty(Pdo, Property, length, buffer, &length);
    if (!NT_SUCCESS(status))
    {
        KphTracePrint(TRACE_LEVEL_VERBOSE,
                      HARDWARE,
                      "IoGetDeviceProperty failed: %!STATUS!",
                      status);

        goto Exit;
    }

    *Buffer = buffer;
    *Length = length;
    buffer = NULL;

Exit:

    if (buffer)
    {
        KphFree(buffer, KPH_TAG_LUA_HARDWARE_PROPERTY);
    }

    return status;
}

/**
 * \brief Frees a device property.
 *
 * \param[in] Context The property.
 */
_Function_class_(KPH_SCRIPT_CLEANUP_ROUTINE)
_IRQL_requires_max_(PASSIVE_LEVEL)
VOID KSIAPI KphLuaHwFreeProperty(
    _In_opt_ PVOID Context
    )
{
    KPH_PAGED_CODE_PASSIVE();

    NT_ASSERT(Context);

    KphFree(Context, KPH_TAG_LUA_HARDWARE_PROPERTY);
}

/**
 * \brief Walks a resource list to its end, or to a memory or port descriptor.
 *
 * \param[in] List The resource list.
 * \param[in] Available The number of bytes available at the list.
 * \param[in] Ordinal The memory or port descriptor to find, from zero, or
 * ULONG_MAX to walk the whole list.
 * \param[out] Size Receives the size of the list when the whole list was
 * walked, zero otherwise.
 *
 * \return The descriptor found, NULL when there is no such descriptor or the
 * list does not fit.
 */
_IRQL_requires_max_(PASSIVE_LEVEL)
PCM_PARTIAL_RESOURCE_DESCRIPTOR KphpLuaHwWalkResources(
    _In_reads_bytes_(Available) PCM_RESOURCE_LIST List,
    _In_ ULONG Available,
    _In_ ULONG Ordinal,
    _Out_ PULONG Size
    )
{
    NTSTATUS status;
    ULONG fullHeaderLength;
    ULONG offset;
    ULONG next;

    KPH_PAGED_CODE_PASSIVE();

    *Size = 0;

    if (Available < UFIELD_OFFSET(CM_RESOURCE_LIST, List))
    {
        KphTracePrint(TRACE_LEVEL_VERBOSE,
                      HARDWARE,
                      "Resource list of %lu bytes is cut short",
                      Available);

        return NULL;
    }

    fullHeaderLength = FIELD_OFFSET(CM_FULL_RESOURCE_DESCRIPTOR,
                                    PartialResourceList.PartialDescriptors);
    offset = FIELD_OFFSET(CM_RESOURCE_LIST, List);

    for (ULONG i = 0; i < List->Count; i++)
    {
        PCM_FULL_RESOURCE_DESCRIPTOR full;
        ULONG partialCount;

        status = RtlULongAdd(offset, fullHeaderLength, &next);
        if (!NT_SUCCESS(status) || (next > Available))
        {
            KphTracePrint(TRACE_LEVEL_VERBOSE,
                          HARDWARE,
                          "Resource list of %lu bytes is cut short",
                          Available);

            return NULL;
        }

        full = Add2Ptr(List, offset);
        partialCount = full->PartialResourceList.Count;
        offset = next;

        for (ULONG j = 0; j < partialCount; j++)
        {
            PCM_PARTIAL_RESOURCE_DESCRIPTOR partial;

            status = RtlULongAdd(offset,
                                 sizeof(CM_PARTIAL_RESOURCE_DESCRIPTOR),
                                 &next);
            if (!NT_SUCCESS(status) || (next > Available))
            {
                KphTracePrint(TRACE_LEVEL_VERBOSE,
                              HARDWARE,
                              "Resource list of %lu bytes is cut short",
                              Available);

                return NULL;
            }

            partial = Add2Ptr(List, offset);
            offset = next;

            if (partial->Type == CmResourceTypeDeviceSpecific)
            {
                status = RtlULongAdd(offset,
                                     partial->u.DeviceSpecificData.DataSize,
                                     &next);
                if (!NT_SUCCESS(status) || (next > Available))
                {
                    KphTracePrint(TRACE_LEVEL_VERBOSE,
                                  HARDWARE,
                                  "Resource list of %lu bytes is cut short",
                                  Available);

                    return NULL;
                }

                offset = next;
                continue;
            }

            if ((partial->Type != CmResourceTypeMemory) &&
                (partial->Type != CmResourceTypeMemoryLarge) &&
                (partial->Type != CmResourceTypePort))
            {
                continue;
            }

            if (Ordinal == 0)
            {
                return partial;
            }

            Ordinal--;
        }
    }

    *Size = offset;

    return NULL;
}

/**
 * \brief Pushes a table of the memory and port resources of a device
 * (ksi.hw.pci_resources, ksi.hw.acpi_resources).
 *
 * \details May raise a Lua error. The resources the PnP manager assigned, or
 * when it assigned none, the boot configuration. Pushes an array of tables
 * with the fields kind (1 memory, 2 port), raw, translated, length and flags,
 * empty when the device has none; or nil and a status.
 *
 * \param[in] State The Lua state.
 * \param[in] Pdo The physical device object.
 *
 * \return The number of values pushed.
 */
_IRQL_requires_max_(PASSIVE_LEVEL)
int KphLuaHwPushResources(
    _In_ lua_State* State,
    _In_ PDEVICE_OBJECT Pdo
    )
{
    int results;
    NTSTATUS status;
    PVOID allocated;
    ULONG allocatedLength;
    PVOID raw;
    ULONG rawLength;
    PVOID translated;
    ULONG translatedLength;
    ULONG size;
    lua_Integer index;
    int top;

    KPH_PAGED_CODE_PASSIVE();

    results = 0;
    raw = NULL;
    rawLength = 0;
    translated = NULL;
    translatedLength = 0;

    top = lua_gettop(State);

    status = KphLuaHwQueryProperty(Pdo,
                                   DevicePropertyAllocatedResources,
                                   &allocated,
                                   &allocatedLength);
    if (NT_SUCCESS(status))
    {
        KphScriptPushCleanup(State, KphLuaHwFreeProperty, allocated);

        //
        // The raw list is followed by the translated list.
        //
        KphpLuaHwWalkResources(allocated, allocatedLength, ULONG_MAX, &size);
        if (!size || (size >= allocatedLength))
        {
            KphTracePrint(TRACE_LEVEL_VERBOSE,
                          HARDWARE,
                          "Allocated resources of %lu bytes are not two lists",
                          allocatedLength);

            status = STATUS_DEVICE_CONFIGURATION_ERROR;
            goto Exit;
        }

        raw = allocated;
        rawLength = size;
        translated = Add2Ptr(allocated, size);
        translatedLength = allocatedLength - size;
        goto WalkResources;
    }

    if (status != STATUS_NOT_FOUND)
    {
        KphTracePrint(TRACE_LEVEL_VERBOSE,
                      HARDWARE,
                      "KphLuaHwQueryProperty failed: %!STATUS!",
                      status);

        goto Exit;
    }

    status = KphLuaHwQueryProperty(Pdo,
                                   DevicePropertyBootConfiguration,
                                   &raw,
                                   &rawLength);
    if (status == STATUS_NOT_FOUND)
    {
        goto WalkResources;
    }

    if (!NT_SUCCESS(status))
    {
        KphTracePrint(TRACE_LEVEL_VERBOSE,
                      HARDWARE,
                      "KphLuaHwQueryProperty failed: %!STATUS!",
                      status);

        goto Exit;
    }

    KphScriptPushCleanup(State, KphLuaHwFreeProperty, raw);

    status = KphLuaHwQueryProperty(Pdo,
                                   DevicePropertyBootConfigurationTranslated,
                                   &translated,
                                   &translatedLength);
    if (NT_SUCCESS(status))
    {
        KphScriptPushCleanup(State, KphLuaHwFreeProperty, translated);
    }
    else
    {
        KphTracePrint(TRACE_LEVEL_VERBOSE,
                      HARDWARE,
                      "KphLuaHwQueryProperty failed: %!STATUS!",
                      status);
    }

WalkResources:

    lua_newtable(State);

    index = 0;

    for (ULONG i = 0; raw; i++)
    {
        PCM_PARTIAL_RESOURCE_DESCRIPTOR descriptor;
        PCM_PARTIAL_RESOURCE_DESCRIPTOR other;

        descriptor = KphpLuaHwWalkResources(raw, rawLength, i, &size);
        if (!descriptor)
        {
            break;
        }

        KphScriptCharge(State, 1);

        other = NULL;

        if (translated)
        {
            other = KphpLuaHwWalkResources(translated,
                                           translatedLength,
                                           i,
                                           &size);
        }

        if (!other)
        {
            other = descriptor;
        }

        lua_createtable(State, 0, 5);
        lua_pushinteger(State,
                        (descriptor->Type == CmResourceTypePort) ? 2 : 1);
        lua_setfield(State, -2, "kind");
        lua_pushinteger(State,
                        (lua_Integer)descriptor->u.Generic.Start.QuadPart);
        lua_setfield(State, -2, "raw");
        lua_pushinteger(State, (lua_Integer)other->u.Generic.Start.QuadPart);
        lua_setfield(State, -2, "translated");
        lua_pushinteger(State,
                        (lua_Integer)RtlCmDecodeMemIoResource(descriptor,
                                                              NULL));
        lua_setfield(State, -2, "length");
        lua_pushinteger(State, (lua_Integer)descriptor->Flags);
        lua_setfield(State, -2, "flags");

        lua_rawseti(State, -2, ++index);
    }

    if (lua_gettop(State) > (top + 1))
    {
        for (int i = top + 1; i < lua_gettop(State); i++)
        {
            KphScriptReleaseCleanup(State, i);
        }

        lua_replace(State, top + 1);
        lua_settop(State, top + 1);
    }

    results = 1;
    status = STATUS_SUCCESS;

Exit:

    if (!NT_SUCCESS(status))
    {
        for (int i = top + 1; i <= lua_gettop(State); i++)
        {
            KphScriptReleaseCleanup(State, i);
        }

        lua_settop(State, top);

        results = KphLuaNilErrorStatus(State, status);
    }

    return results;
}

KPH_PROTECTED_DATA_SECTION_RO_PUSH();
static const luaL_Reg KphpLuaHwLibrary[] =
{
#if defined(_M_X64)
    { "port_read", KphpLuaHwPortRead },
    { "port_write", KphpLuaHwPortWrite },
#endif
    { "firmware_table", KphpLuaHwFirmwareTable },
    { NULL, NULL }
};
KPH_PROTECTED_DATA_SECTION_RO_POP();

/**
 * \brief Opens the hardware library as the field hw of the library of the
 * driver.
 *
 * \details May raise a Lua error.
 *
 * \param[in] State The Lua state, with the library of the driver on top.
 */
_IRQL_requires_max_(PASSIVE_LEVEL)
VOID KphLuaOpenHardwareLibrary(
    _In_ lua_State* State
    )
{
    KPH_PAGED_CODE_PASSIVE();

    luaL_newlib(State, KphpLuaHwLibrary);

    KphLuaOpenHardwareCpu(State);
    KphLuaOpenHardwarePci(State);
    KphLuaOpenHardwareMemory(State);
    KphLuaOpenHardwareSysreg(State);

    lua_setfield(State, -2, KPH_LUA_HARDWARE_LIBRARY_NAME);
}
