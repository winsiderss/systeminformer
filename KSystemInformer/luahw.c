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
    KphLuaInitializeHardwareDevice();
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
    KphLuaCleanupHardwareDevice();
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

        lua_pushliteral(State, "");
        results = 1;
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
 * \brief References the device objects of a driver object.
 *
 * \param[in] Driver The driver object.
 * \param[out] Devices Receives the referenced device objects, or NULL when the
 * driver has none; the caller dereferences each and frees the list with
 * KPH_TAG_LUA_HARDWARE_DEVICES.
 * \param[out] Count Receives the number of device objects.
 *
 * \return Successful or errant status.
 */
_IRQL_requires_max_(PASSIVE_LEVEL)
_Must_inspect_result_
NTSTATUS KphLuaHwReferenceDriverDevices(
    _In_ PDRIVER_OBJECT Driver,
    _Outptr_result_buffer_maybenull_(*Count) PDEVICE_OBJECT** Devices,
    _Out_ PULONG Count
    )
{
    NTSTATUS status;
    PDEVICE_OBJECT* devices;
    ULONG count;
    ULONG length;
    ULONG capacity;

    KPH_PAGED_CODE_PASSIVE();

    *Devices = NULL;
    *Count = 0;

    devices = NULL;
    count = 0;

    status = IoEnumerateDeviceObjectList(Driver, NULL, 0, &count);
    if ((status != STATUS_BUFFER_TOO_SMALL) || !count)
    {
        KphTracePrint(TRACE_LEVEL_VERBOSE,
                      HARDWARE,
                      "IoEnumerateDeviceObjectList returned: %!STATUS!",
                      status);

        if (NT_SUCCESS(status) || (status == STATUS_BUFFER_TOO_SMALL))
        {
            status = STATUS_SUCCESS;
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

        status = IoEnumerateDeviceObjectList(Driver, devices, length, &count);
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

    return status;
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
    KphLuaOpenHardwareDevice(State);
    KphLuaOpenHardwareSysreg(State);

    lua_setfield(State, -2, KPH_LUA_HARDWARE_LIBRARY_NAME);
}
