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
#include <acpiioct.h>
#include <initguid.h>
#include <devpkey.h>

#include <trace.h>

#define KPH_LUA_HW_ACPI_ID_LENGTH 8

typedef struct _KPH_LUA_HW_ACPI_DEVICE
{
    PDEVICE_OBJECT Pdo;
} KPH_LUA_HW_ACPI_DEVICE, *PKPH_LUA_HW_ACPI_DEVICE;

KPH_PROTECTED_DATA_SECTION_RO_PUSH();
DECLARE_CONST_UNICODE_STRING(KphpLuaHwAcpiDriverName, L"\\Driver\\ACPI");
DECLARE_CONST_UNICODE_STRING(KphpLuaHwAcpiIdPrefix, L"ACPI\\");
DECLARE_CONST_UNICODE_STRING(KphpLuaHwAcpiVenPrefix, L"VEN_");
DECLARE_CONST_UNICODE_STRING(KphpLuaHwAcpiDevPrefix, L"&DEV_");
static const DEVICE_REGISTRY_PROPERTY KphpLuaHwAcpiIdProperties[] =
{
    DevicePropertyHardwareID,
    DevicePropertyCompatibleIDs,
};
KPH_PROTECTED_DATA_SECTION_RO_POP();
static PAGED_LOOKASIDE_LIST KphpLuaHwAcpiDeviceLookaside = { 0 };

KPH_PAGED_FILE();

/**
 * \brief Checks whether a character may appear in a namespace path segment.
 *
 * \param[in] Character The character.
 * \param[in] First TRUE for the first character of a segment.
 *
 * \return TRUE for A-Z and _, and 0-9 past the first character.
 */
_IRQL_requires_max_(PASSIVE_LEVEL)
_Must_inspect_result_
BOOLEAN KphpLuaHwAcpiPathCharValid(
    _In_ CHAR Character,
    _In_ BOOLEAN First
    )
{
    KPH_PAGED_CODE_PASSIVE();

    return (((Character >= 'A') && (Character <= 'Z')) ||
            (Character == '_') ||
            (!First && (Character >= '0') && (Character <= '9')));
}

/**
 * \brief Normalizes an absolute namespace path as BiosDeviceName prints it.
 *
 * \details Trailing underscore padding is removed, so "\_SB_.EC0_" and
 * "\_SB.EC0" are the same path. The path never grows.
 *
 * \param[in,out] Path The path, normalized in place; undefined on failure.
 *
 * \return TRUE if the path is well formed.
 */
_IRQL_requires_max_(PASSIVE_LEVEL)
_Must_inspect_result_
_Success_(return)
BOOLEAN KphpLuaHwAcpiNormalizePath(
    _Inout_ PANSI_STRING Path
    )
{
    PCHAR buffer;
    ULONG length;
    ULONG offset;
    ULONG out;

    KPH_PAGED_CODE_PASSIVE();

    buffer = Path->Buffer;
    length = Path->Length;

    if ((length < 2) || (buffer[0] != OBJ_NAME_PATH_SEPARATOR))
    {
        return FALSE;
    }

    offset = 1;
    out = 1;

    while (offset < length)
    {
        ULONG start;
        ULONG end;

        start = offset;

        while ((offset < length) && (buffer[offset] != '.'))
        {
            if (!KphpLuaHwAcpiPathCharValid(buffer[offset],
                                            (offset == start)))
            {
                return FALSE;
            }

            offset++;
        }

        end = offset;

        if ((end == start) || ((end - start) > 4))
        {
            return FALSE;
        }

        while (((end - start) > 1) && (buffer[end - 1] == '_'))
        {
            end--;
        }

        RtlMoveMemory(&buffer[out], &buffer[start], end - start);
        out += (end - start);

        if (offset < length)
        {
            offset++;

            if (offset >= length)
            {
                return FALSE;
            }

            buffer[out++] = '.';
        }
    }

    NT_ASSERT(out <= length);

    Path->Length = (USHORT)out;

    return TRUE;
}

/**
 * \brief Retrieves the namespace path of an ACPI device, normalized.
 *
 * \param[in] Pdo The physical device object.
 * \param[out] Path Receives the normalized path, freed with KphFree and
 * KPH_TAG_LUA_HARDWARE_ACPI_PATH.
 *
 * \return Successful or errant status.
 */
_IRQL_requires_max_(PASSIVE_LEVEL)
_Must_inspect_result_
NTSTATUS KphpLuaHwAcpiQueryPath(
    _In_ PDEVICE_OBJECT Pdo,
    _Outptr_ PANSI_STRING* Path
    )
{
    NTSTATUS status;
    PWCHAR name;
    PANSI_STRING path;
    ULONG required;
    ULONG length;
    ULONG count;
    ULONG prefix;
    ULONG pathLength;
    ULONG allocationLength;
    DEVPROPTYPE type;

    KPH_PAGED_CODE_PASSIVE();

    *Path = NULL;

    name = NULL;
    path = NULL;
    required = 0;
    type = DEVPROP_TYPE_EMPTY;

    status = IoGetDevicePropertyData(Pdo,
                                     &DEVPKEY_Device_BiosDeviceName,
                                     LOCALE_NEUTRAL,
                                     0,
                                     0,
                                     NULL,
                                     &required,
                                     &type);
    if ((status != STATUS_BUFFER_TOO_SMALL) ||
        (type != DEVPROP_TYPE_STRING) ||
        (required < (2 * sizeof(WCHAR))) ||
        !IS_ALIGNED(required, sizeof(WCHAR)))
    {
        KphTracePrint(TRACE_LEVEL_VERBOSE,
                      HARDWARE,
                      "BiosDeviceName query: %!STATUS! type %lu length %lu",
                      status,
                      (ULONG)type,
                      required);

        status = STATUS_NOT_FOUND;
        goto Exit;
    }

    length = required;
    count = ((length / sizeof(WCHAR)) - 1);

    if (count >= USHORT_MAX)
    {
        KphTracePrint(TRACE_LEVEL_VERBOSE,
                      HARDWARE,
                      "BiosDeviceName is too long: %lu",
                      count);

        status = STATUS_NOT_FOUND;
        goto Exit;
    }

    name = KphAllocatePaged(length, KPH_TAG_LUA_HARDWARE_ACPI_PATH);
    if (!name)
    {
        KphTracePrint(TRACE_LEVEL_VERBOSE,
                      HARDWARE,
                      "Failed to allocate ACPI path buffer");

        status = STATUS_INSUFFICIENT_RESOURCES;
        goto Exit;
    }

    status = IoGetDevicePropertyData(Pdo,
                                     &DEVPKEY_Device_BiosDeviceName,
                                     LOCALE_NEUTRAL,
                                     0,
                                     length,
                                     name,
                                     &required,
                                     &type);
    if (!NT_SUCCESS(status))
    {
        KphTracePrint(TRACE_LEVEL_VERBOSE,
                      HARDWARE,
                      "IoGetDevicePropertyData failed: %!STATUS!",
                      status);

        status = STATUS_NOT_FOUND;
        goto Exit;
    }

    if ((type != DEVPROP_TYPE_STRING) ||
        (required != length) ||
        (name[count] != UNICODE_NULL))
    {
        KphTracePrint(TRACE_LEVEL_VERBOSE,
                      HARDWARE,
                      "BiosDeviceName changed: type %lu length %lu",
                      (ULONG)type,
                      required);

        status = STATUS_NOT_FOUND;
        goto Exit;
    }

    prefix = ((name[0] == OBJ_NAME_PATH_SEPARATOR) ? 0 : 1);

    NT_VERIFY(NT_SUCCESS(RtlULongAdd(count, prefix, &pathLength)));
    NT_VERIFY(NT_SUCCESS(RtlULongAdd(sizeof(ANSI_STRING),
                                     pathLength,
                                     &allocationLength)));

    path = KphAllocatePaged(allocationLength, KPH_TAG_LUA_HARDWARE_ACPI_PATH);
    if (!path)
    {
        KphTracePrint(TRACE_LEVEL_VERBOSE,
                      HARDWARE,
                      "Failed to allocate ACPI path");

        status = STATUS_INSUFFICIENT_RESOURCES;
        goto Exit;
    }

    path->Buffer = Add2Ptr(path, sizeof(ANSI_STRING));
    path->Length = (USHORT)pathLength;
    path->MaximumLength = (USHORT)pathLength;
    path->Buffer[0] = OBJ_NAME_PATH_SEPARATOR;

    for (ULONG i = 0; i < count; i++)
    {
        if ((name[i] == UNICODE_NULL) || (name[i] > 0x7F))
        {
            KphTracePrint(TRACE_LEVEL_VERBOSE,
                          HARDWARE,
                          "BiosDeviceName has a non-ASCII character at %lu",
                          i);

            status = STATUS_NOT_FOUND;
            goto Exit;
        }

        path->Buffer[prefix + i] = (CHAR)name[i];
    }

    if (!KphpLuaHwAcpiNormalizePath(path))
    {
        KphTracePrint(TRACE_LEVEL_VERBOSE,
                      HARDWARE,
                      "BiosDeviceName %Z is malformed",
                      path);

        status = STATUS_NOT_FOUND;
        goto Exit;
    }

    *Path = path;
    path = NULL;
    status = STATUS_SUCCESS;

Exit:

    if (path)
    {
        KphFree(path, KPH_TAG_LUA_HARDWARE_ACPI_PATH);
    }

    if (name)
    {
        KphFree(name, KPH_TAG_LUA_HARDWARE_ACPI_PATH);
    }

    return status;
}

/**
 * \brief Checks that an ACPI identity is well formed.
 *
 * \param[in] Id The identity characters.
 *
 * \return TRUE if the first 7 or 8 characters are [A-Z0-9_] and the rest zero.
 */
_IRQL_requires_max_(PASSIVE_LEVEL)
_Must_inspect_result_
BOOLEAN KphpLuaHwAcpiIdValid(
    _In_reads_(KPH_LUA_HW_ACPI_ID_LENGTH + 1) PCCH Id
    )
{
    ULONG length;

    KPH_PAGED_CODE_PASSIVE();

    for (length = 0; length < KPH_LUA_HW_ACPI_ID_LENGTH; length++)
    {
        CHAR character;

        character = Id[length];

        if (character == ANSI_NULL)
        {
            break;
        }

        if (((character < 'A') || (character > 'Z')) &&
            ((character < '0') || (character > '9')) &&
            (character != '_'))
        {
            return FALSE;
        }
    }

    if ((length != 7) && (length != 8))
    {
        return FALSE;
    }

    for (ULONG i = length; i <= KPH_LUA_HW_ACPI_ID_LENGTH; i++)
    {
        if (Id[i] != ANSI_NULL)
        {
            return FALSE;
        }
    }

    return TRUE;
}

/**
 * \brief Converts a hardware or compatible ID to an ACPI identity.
 *
 * \param[in] String The ID string.
 * \param[out] Id Receives the identity, zero terminated and padded.
 *
 * \return TRUE if the string names an ID.
 */
_IRQL_requires_max_(PASSIVE_LEVEL)
_Must_inspect_result_
_Success_(return)
BOOLEAN KphpLuaHwAcpiNormalizeId(
    _In_ PCUNICODE_STRING String,
    _Out_writes_(KPH_LUA_HW_ACPI_ID_LENGTH + 1) PCHAR Id
    )
{
    CHAR id[KPH_LUA_HW_ACPI_ID_LENGTH + 1];
    UNICODE_STRING string;
    UNICODE_STRING part;
    ULONG out;

    KPH_PAGED_CODE_PASSIVE();

    RtlZeroMemory(Id, KPH_LUA_HW_ACPI_ID_LENGTH + 1);

    string = *String;

    if (RtlPrefixUnicodeString(&KphpLuaHwAcpiIdPrefix, &string, TRUE))
    {
        string.Buffer += (KphpLuaHwAcpiIdPrefix.Length / sizeof(WCHAR));
        string.Length -= KphpLuaHwAcpiIdPrefix.Length;
    }
    else if ((string.Length >= sizeof(WCHAR)) && (string.Buffer[0] == L'*'))
    {
        string.Buffer++;
        string.Length -= sizeof(WCHAR);
    }
    else
    {
        KphTracePrint(TRACE_LEVEL_VERBOSE,
                      HARDWARE,
                      "ID %wZ is not an ACPI ID",
                      String);

        return FALSE;
    }

    string.MaximumLength = string.Length;

    RtlZeroMemory(id, sizeof(id));
    out = 0;

    if (RtlPrefixUnicodeString(&KphpLuaHwAcpiVenPrefix, &string, TRUE))
    {
        string.Buffer += (KphpLuaHwAcpiVenPrefix.Length / sizeof(WCHAR));
        string.Length -= KphpLuaHwAcpiVenPrefix.Length;
        string.MaximumLength = string.Length;

        part = string;
        part.Length = 0;

        while ((part.Length < string.Length) &&
               (string.Buffer[part.Length / sizeof(WCHAR)] != L'&'))
        {
            part.Length += sizeof(WCHAR);
        }

        string.Buffer += (part.Length / sizeof(WCHAR));
        string.Length -= part.Length;
        string.MaximumLength = string.Length;

        if (!RtlPrefixUnicodeString(&KphpLuaHwAcpiDevPrefix, &string, TRUE))
        {
            KphTracePrint(TRACE_LEVEL_VERBOSE,
                          HARDWARE,
                          "ACPI ID %wZ has no &DEV_",
                          String);

            return FALSE;
        }

        string.Buffer += (KphpLuaHwAcpiDevPrefix.Length / sizeof(WCHAR));
        string.Length -= KphpLuaHwAcpiDevPrefix.Length;
        string.MaximumLength = string.Length;

        if ((((SIZE_T)part.Length + string.Length) / sizeof(WCHAR)) >
            KPH_LUA_HW_ACPI_ID_LENGTH)
        {
            KphTracePrint(TRACE_LEVEL_VERBOSE,
                          HARDWARE,
                          "ACPI ID %wZ is too long",
                          String);

            return FALSE;
        }

        for (ULONG i = 0; i < (part.Length / sizeof(WCHAR)); i++)
        {
            WCHAR c;

            c = RtlUpcaseUnicodeChar(part.Buffer[i]);

            if ((c == UNICODE_NULL) || (c > 0x7F))
            {
                KphTracePrint(TRACE_LEVEL_VERBOSE,
                              HARDWARE,
                              "ACPI ID %wZ is not ASCII",
                              String);

                return FALSE;
            }

            id[out++] = (CHAR)c;
        }
    }

    if ((out + (string.Length / sizeof(WCHAR))) > KPH_LUA_HW_ACPI_ID_LENGTH)
    {
        KphTracePrint(TRACE_LEVEL_VERBOSE,
                      HARDWARE,
                      "ACPI ID %wZ is too long",
                      String);

        return FALSE;
    }

    for (ULONG i = 0; i < (string.Length / sizeof(WCHAR)); i++)
    {
        WCHAR c;

        c = RtlUpcaseUnicodeChar(string.Buffer[i]);

        if ((c == UNICODE_NULL) || (c > 0x7F))
        {
            KphTracePrint(TRACE_LEVEL_VERBOSE,
                          HARDWARE,
                          "ACPI ID %wZ is not ASCII",
                          String);

            return FALSE;
        }

        id[out++] = (CHAR)c;
    }

    if (!KphpLuaHwAcpiIdValid(id))
    {
        KphTracePrint(TRACE_LEVEL_VERBOSE,
                      HARDWARE,
                      "ACPI ID %wZ is not a valid ID",
                      String);

        return FALSE;
    }

    RtlCopyMemory(Id, id, sizeof(id));

    return TRUE;
}

/**
 * \brief Pushes the normalized hardware and compatible IDs of an ACPI device
 * onto a table, or tells whether one of them is an ID.
 *
 * \details May raise a Lua error when a table is given.
 *
 * \param[in] State The Lua state.
 * \param[in] Pdo The physical device object.
 * \param[in] Table The index of the table to append to, or zero.
 * \param[in] Match The ID to look for when Table is zero.
 *
 * \return TRUE when Match is one of the IDs.
 */
_IRQL_requires_max_(PASSIVE_LEVEL)
BOOLEAN KphpLuaHwAcpiEnumerateIds(
    _In_ lua_State* State,
    _In_ PDEVICE_OBJECT Pdo,
    _In_ int Table,
    _In_opt_ PCANSI_STRING Match
    )
{
    NTSTATUS status;
    PWCHAR ids;
    ULONG length;
    UNICODE_STRING id;
    CHAR acpiId[KPH_LUA_HW_ACPI_ID_LENGTH + 1];
    ANSI_STRING string;
    lua_Integer index;
    BOOLEAN found;

    KPH_PAGED_CODE_PASSIVE();

    NT_ASSERT(Table || Match);

    index = Table ? (lua_Integer)luaL_len(State, Table) : 0;

    for (ULONG i = 0; i < ARRAYSIZE(KphpLuaHwAcpiIdProperties); i++)
    {
        status = KphLuaHwQueryProperty(Pdo,
                                       KphpLuaHwAcpiIdProperties[i],
                                       &ids,
                                       &length);
        if (!NT_SUCCESS(status))
        {
            KphTracePrint(TRACE_LEVEL_VERBOSE,
                          HARDWARE,
                          "KphLuaHwQueryProperty failed: %!STATUS!",
                          status);

            continue;
        }

        if (Table)
        {
            KphScriptPushCleanup(State, KphLuaHwFreeProperty, ids);
        }

        found = FALSE;

        RtlInitUnicodeString(&id, ids);

        while (id.Length > 0)
        {
            if (KphpLuaHwAcpiNormalizeId(&id, acpiId))
            {
                if (Table)
                {
                    lua_pushstring(State, acpiId);
                    lua_rawseti(State, Table, ++index);
                }
                else
                {
                    RtlInitAnsiString(&string, acpiId);

                    if (RtlEqualString(&string, Match, FALSE))
                    {
                        found = TRUE;
                        break;
                    }
                }
            }

            RtlInitUnicodeString(&id, Add2Ptr(id.Buffer, id.Length + sizeof(WCHAR)));
        }

        if (Table)
        {
            KphScriptReleaseCleanup(State, lua_gettop(State));
            lua_pop(State, 1);
        }
        else
        {
            KphFree(ids, KPH_TAG_LUA_HARDWARE_PROPERTY);
        }

        if (found)
        {
            return TRUE;
        }
    }

    return FALSE;
}

/**
 * \brief Releases a device object of ksi.hw.acpi_find and ksi.hw.acpi_open.
 *
 * \param[in] Context The device.
 */
_Function_class_(KPH_SCRIPT_CLEANUP_ROUTINE)
_IRQL_requires_max_(PASSIVE_LEVEL)
VOID KSIAPI KphpLuaHwAcpiReleaseDevice(
    _In_opt_ PVOID Context
    )
{
    PKPH_LUA_HW_ACPI_DEVICE device;

    KPH_PAGED_CODE_PASSIVE();

    NT_ASSERT(Context);

    device = Context;

    ObDereferenceObject(device->Pdo);

    KphFreeToPagedLookaside(&KphpLuaHwAcpiDeviceLookaside, device);
}

/**
 * \brief Pushes a device object holding a referenced physical device object.
 *
 * \details May raise a Lua error. The reference is the object's, or released
 * here when the object cannot be made.
 *
 * \param[in] State The Lua state.
 * \param[in] Pdo The referenced physical device object.
 *
 * \return The number of results: the device object, or nil and a status.
 */
_IRQL_requires_max_(PASSIVE_LEVEL)
int KphpLuaHwAcpiPushDevice(
    _In_ lua_State* State,
    _In_ PDEVICE_OBJECT Pdo
    )
{
    int results;
    PKPH_LUA_HW_ACPI_DEVICE device;

    KPH_PAGED_CODE_PASSIVE();

    device = KphAllocateFromPagedLookaside(&KphpLuaHwAcpiDeviceLookaside);
    if (!device)
    {
        KphTracePrint(TRACE_LEVEL_VERBOSE,
                      HARDWARE,
                      "Failed to allocate an ACPI device");

        results = KphLuaNilErrorStatus(State, STATUS_INSUFFICIENT_RESOURCES);
        goto Exit;
    }

    NT_ASSERT(sizeof(KPH_LUA_HW_ACPI_DEVICE) <=
              KphpLuaHwAcpiDeviceLookaside.L.Size);

    device->Pdo = Pdo;
    Pdo = NULL;

    KphScriptPushObject(State, KphpLuaHwAcpiReleaseDevice, device);

    results = 1;

Exit:

    if (Pdo)
    {
        ObDereferenceObject(Pdo);
    }

    return results;
}

/**
 * \brief Finds an ACPI device by one of its IDs (ksi.hw.acpi_find).
 *
 * \details May raise a Lua error.
 *
 * \param[in] State The Lua state.
 *
 * \return The number of results: the device object, or nil and a status.
 */
_IRQL_requires_max_(PASSIVE_LEVEL)
int KphpLuaHwAcpiFind(
    _In_ lua_State* State
    )
{
    int results;
    NTSTATUS status;
    ANSI_STRING match;
    SIZE_T length;
    lua_Integer ordinal;
    PDEVICE_OBJECT* devices;
    ULONG count;
    PDEVICE_OBJECT pdo;

    KPH_PAGED_CODE_PASSIVE();

    match.Buffer = (PCHAR)luaL_checklstring(State, 1, &length);
    ordinal = luaL_optinteger(State, 2, 0);

    luaL_argcheck(State,
                  (length >= 1) && (length <= KPH_LUA_HW_ACPI_ID_LENGTH),
                  1,
                  "1 to 8 characters");
    luaL_argcheck(State,
                  (ordinal >= 0) && (ordinal <= ULONG_MAX),
                  2,
                  "out of range");

    match.Length = (USHORT)length;
    match.MaximumLength = (USHORT)length;

    pdo = NULL;
    count = 0;

    status = KphLuaHwReferenceDevices(&KphpLuaHwAcpiDriverName,
                                      &devices,
                                      &count);
    if (!NT_SUCCESS(status))
    {
        KphTracePrint(TRACE_LEVEL_VERBOSE,
                      HARDWARE,
                      "KphLuaHwReferenceDevices failed: %!STATUS!",
                      status);

        results = KphLuaNilErrorStatus(State, STATUS_NOT_FOUND);
        goto Exit;
    }

    for (ULONG i = 0; i < count; i++)
    {
        if (!BooleanFlagOn(devices[i]->Flags, DO_BUS_ENUMERATED_DEVICE))
        {
            continue;
        }

        if (!KphpLuaHwAcpiEnumerateIds(State, devices[i], 0, &match))
        {
            continue;
        }

        if (ordinal)
        {
            ordinal--;
            continue;
        }

        pdo = devices[i];
        ObReferenceObject(pdo);
        break;
    }

    KphLuaHwDereferenceDevices(devices, count);

    if (pdo)
    {
        results = KphpLuaHwAcpiPushDevice(State, pdo);
    }
    else
    {
        KphTracePrint(TRACE_LEVEL_VERBOSE,
                      HARDWARE,
                      "No ACPI device has the ID %Z",
                      &match);

        results = KphLuaNilErrorStatus(State, STATUS_NOT_FOUND);
    }

    KphScriptCharge(State, count);

Exit:

    return results;
}

/**
 * \brief Finds and references the ACPI physical device object at a path.
 *
 * \param[in] Path The normalized namespace path.
 * \param[out] Count Receives the number of device objects looked at.
 *
 * \return The referenced physical device object, or NULL.
 */
_IRQL_requires_max_(PASSIVE_LEVEL)
_Must_inspect_result_
PDEVICE_OBJECT KphpLuaHwAcpiFindPdo(
    _In_ PCANSI_STRING Path,
    _Out_ PULONG Count
    )
{
    NTSTATUS status;
    PDEVICE_OBJECT* devices;
    ULONG count;
    PDEVICE_OBJECT pdo;
    PANSI_STRING path;
    BOOLEAN equal;

    KPH_PAGED_CODE_PASSIVE();

    *Count = 0;

    pdo = NULL;

    status = KphLuaHwReferenceDevices(&KphpLuaHwAcpiDriverName,
                                      &devices,
                                      &count);
    if (!NT_SUCCESS(status))
    {
        KphTracePrint(TRACE_LEVEL_VERBOSE,
                      HARDWARE,
                      "KphLuaHwReferenceDevices failed: %!STATUS!",
                      status);

        return NULL;
    }

    for (ULONG i = 0; i < count; i++)
    {
        if (!BooleanFlagOn(devices[i]->Flags, DO_BUS_ENUMERATED_DEVICE))
        {
            continue;
        }

        status = KphpLuaHwAcpiQueryPath(devices[i], &path);
        if (!NT_SUCCESS(status))
        {
            KphTracePrint(TRACE_LEVEL_VERBOSE,
                          HARDWARE,
                          "KphpLuaHwAcpiQueryPath failed: %!STATUS!",
                          status);

            continue;
        }

        equal = RtlEqualString(path, Path, FALSE);

        KphFree(path, KPH_TAG_LUA_HARDWARE_ACPI_PATH);

        if (equal)
        {
            pdo = devices[i];
            ObReferenceObject(pdo);
            break;
        }
    }

    KphLuaHwDereferenceDevices(devices, count);

    *Count = count;

    return pdo;
}

/**
 * \brief Opens the ACPI device at a namespace path (ksi.hw.acpi_open).
 *
 * \details May raise a Lua error.
 *
 * \param[in] State The Lua state.
 *
 * \return The number of results: the device object, or nil and a status.
 */
_IRQL_requires_max_(PASSIVE_LEVEL)
int KphpLuaHwAcpiOpen(
    _In_ lua_State* State
    )
{
    int results;
    PCSTR text;
    SIZE_T length;
    PCHAR buffer;
    ANSI_STRING path;
    PDEVICE_OBJECT pdo;
    ULONG count;

    KPH_PAGED_CODE_PASSIVE();

    text = luaL_checklstring(State, 1, &length);

    if ((length < 2) || (length > USHORT_MAX))
    {
        KphTracePrint(TRACE_LEVEL_VERBOSE,
                      HARDWARE,
                      "ACPI path of %Iu characters",
                      length);

        results = KphLuaNilErrorStatus(State, STATUS_NOT_FOUND);
        goto Exit;
    }

    buffer = KphAllocatePaged(length, KPH_TAG_LUA_HARDWARE_ACPI_PATH);
    if (!buffer)
    {
        KphTracePrint(TRACE_LEVEL_VERBOSE,
                      HARDWARE,
                      "Failed to allocate an ACPI path");

        results = KphLuaNilErrorStatus(State, STATUS_INSUFFICIENT_RESOURCES);
        goto Exit;
    }

    RtlCopyMemory(buffer, text, length);

    path.Buffer = buffer;
    path.Length = (USHORT)length;
    path.MaximumLength = (USHORT)length;

    pdo = NULL;
    count = 0;

    if (KphpLuaHwAcpiNormalizePath(&path))
    {
        pdo = KphpLuaHwAcpiFindPdo(&path, &count);
    }
    else
    {
        KphTracePrint(TRACE_LEVEL_VERBOSE,
                      HARDWARE,
                      "ACPI path is malformed");
    }

    KphFree(buffer, KPH_TAG_LUA_HARDWARE_ACPI_PATH);

    if (pdo)
    {
        results = KphpLuaHwAcpiPushDevice(State, pdo);
    }
    else
    {
        KphTracePrint(TRACE_LEVEL_VERBOSE,
                      HARDWARE,
                      "No ACPI device at the path");

        results = KphLuaNilErrorStatus(State, STATUS_NOT_FOUND);
    }

    KphScriptCharge(State, count);

Exit:

    return results;
}

/**
 * \brief Lists the IDs of an ACPI device (ksi.hw.acpi_ids).
 *
 * \details May raise a Lua error.
 *
 * \param[in] State The Lua state.
 *
 * \return One, the array of IDs, hardware IDs first.
 */
_IRQL_requires_max_(PASSIVE_LEVEL)
int KphpLuaHwAcpiIds(
    _In_ lua_State* State
    )
{
    PKPH_LUA_HW_ACPI_DEVICE device;

    KPH_PAGED_CODE_PASSIVE();

    device = KphScriptCheckCleanup(State, 1, KphpLuaHwAcpiReleaseDevice);

    lua_newtable(State);

    KphpLuaHwAcpiEnumerateIds(State, device->Pdo, lua_gettop(State), NULL);

    KphScriptCharge(State, lua_rawlen(State, -1));

    return 1;
}

/**
 * \brief Frees a namespace path.
 *
 * \param[in] Context The path.
 */
_Function_class_(KPH_SCRIPT_CLEANUP_ROUTINE)
_IRQL_requires_max_(PASSIVE_LEVEL)
VOID KSIAPI KphpLuaHwAcpiFreePath(
    _In_opt_ PVOID Context
    )
{
    KPH_PAGED_CODE_PASSIVE();

    NT_ASSERT(Context);

    KphFree(Context, KPH_TAG_LUA_HARDWARE_ACPI_PATH);
}

/**
 * \brief Retrieves the namespace path of an ACPI device (ksi.hw.acpi_path).
 *
 * \details May raise a Lua error.
 *
 * \param[in] State The Lua state.
 *
 * \return The number of results: the path, or nil and a status.
 */
_IRQL_requires_max_(PASSIVE_LEVEL)
int KphpLuaHwAcpiPath(
    _In_ lua_State* State
    )
{
    int results;
    NTSTATUS status;
    PKPH_LUA_HW_ACPI_DEVICE device;
    PANSI_STRING path;
    int cleanup;

    KPH_PAGED_CODE_PASSIVE();

    device = KphScriptCheckCleanup(State, 1, KphpLuaHwAcpiReleaseDevice);

    status = KphpLuaHwAcpiQueryPath(device->Pdo, &path);
    if (!NT_SUCCESS(status))
    {
        KphTracePrint(TRACE_LEVEL_VERBOSE,
                      HARDWARE,
                      "KphpLuaHwAcpiQueryPath failed: %!STATUS!",
                      status);

        results = KphLuaNilErrorStatus(State, status);
        goto Exit;
    }

    KphScriptPushCleanup(State, KphpLuaHwAcpiFreePath, path);
    cleanup = lua_gettop(State);

    lua_pushlstring(State, path->Buffer, path->Length);

    KphScriptReleaseCleanup(State, cleanup);
    lua_remove(State, cleanup);

    results = 1;

Exit:

    return results;
}

/**
 * \brief Lists the memory and port resources of an ACPI device
 * (ksi.hw.acpi_resources).
 *
 * \details May raise a Lua error.
 *
 * \param[in] State The Lua state.
 *
 * \return The number of results: the array of resources, or nil and a
 * status.
 */
_IRQL_requires_max_(PASSIVE_LEVEL)
int KphpLuaHwAcpiResources(
    _In_ lua_State* State
    )
{
    PKPH_LUA_HW_ACPI_DEVICE device;

    KPH_PAGED_CODE_PASSIVE();

    device = KphScriptCheckCleanup(State, 1, KphpLuaHwAcpiReleaseDevice);

    return KphLuaHwPushResources(State, device->Pdo);
}

/**
 * \brief Lists the memory requirements of an ACPI device
 * (ksi.hw.acpi_requirements).
 *
 * \details May raise a Lua error.
 *
 * \param[in] State The Lua state.
 *
 * \return The number of results: the array of requirements, or nil and a
 * status.
 */
_IRQL_requires_max_(PASSIVE_LEVEL)
int KphpLuaHwAcpiRequirements(
    _In_ lua_State* State
    )
{
    int results;
    NTSTATUS status;
    PKPH_LUA_HW_ACPI_DEVICE device;
    PIO_RESOURCE_REQUIREMENTS_LIST requirements;
    ULONG length;
    ULONG offset;
    ULONG next;
    lua_Integer index;
    int cleanup;

    KPH_PAGED_CODE_PASSIVE();

    results = 0;
    cleanup = 0;

    device = KphScriptCheckCleanup(State, 1, KphpLuaHwAcpiReleaseDevice);

    status = KphLuaHwQueryProperty(device->Pdo,
                                   DevicePropertyResourceRequirements,
                                   &requirements,
                                   &length);
    if (!NT_SUCCESS(status))
    {
        KphTracePrint(TRACE_LEVEL_VERBOSE,
                      HARDWARE,
                      "KphLuaHwQueryProperty failed: %!STATUS!",
                      status);

        goto Exit;
    }

    KphScriptPushCleanup(State, KphLuaHwFreeProperty, requirements);
    cleanup = lua_gettop(State);

    if (length < UFIELD_OFFSET(IO_RESOURCE_REQUIREMENTS_LIST, List))
    {
        KphTracePrint(TRACE_LEVEL_VERBOSE,
                      HARDWARE,
                      "Resource requirements of %lu bytes are malformed",
                      length);

        status = STATUS_DEVICE_CONFIGURATION_ERROR;
        goto Exit;
    }

    lua_newtable(State);

    index = 0;
    offset = FIELD_OFFSET(IO_RESOURCE_REQUIREMENTS_LIST, List);

    for (ULONG i = 0; i < requirements->AlternativeLists; i++)
    {
        PIO_RESOURCE_LIST list;
        ULONG descriptorsLength;

        status = RtlULongAdd(offset,
                             FIELD_OFFSET(IO_RESOURCE_LIST, Descriptors),
                             &next);
        if (!NT_SUCCESS(status) || (next > length))
        {
            KphTracePrint(TRACE_LEVEL_VERBOSE,
                          HARDWARE,
                          "Resource requirements truncated at %lu",
                          offset);

            status = STATUS_DEVICE_CONFIGURATION_ERROR;
            goto Exit;
        }

        list = Add2Ptr(requirements, offset);

        status = RtlULongMult(list->Count,
                              sizeof(IO_RESOURCE_DESCRIPTOR),
                              &descriptorsLength);
        if (NT_SUCCESS(status))
        {
            status = RtlULongAdd(next, descriptorsLength, &next);
        }

        if (!NT_SUCCESS(status) || (next > length))
        {
            KphTracePrint(TRACE_LEVEL_VERBOSE,
                          HARDWARE,
                          "Resource requirements list %lu overruns",
                          i);

            status = STATUS_DEVICE_CONFIGURATION_ERROR;
            goto Exit;
        }

        for (ULONG j = 0; j < list->Count; j++)
        {
            PIO_RESOURCE_DESCRIPTOR descriptor;
            ULONGLONG alignment;
            ULONGLONG minimum;
            ULONGLONG maximum;
            ULONGLONG size;

            descriptor = &list->Descriptors[j];

            if ((descriptor->Type != CmResourceTypeMemory) &&
                (descriptor->Type != CmResourceTypeMemoryLarge))
            {
                continue;
            }

            alignment = 0;
            minimum = 0;
            maximum = 0;

            size = RtlIoDecodeMemIoResource(descriptor,
                                            &alignment,
                                            &minimum,
                                            &maximum);
            if (!size)
            {
                continue;
            }

            KphScriptCharge(State, 1);

            lua_createtable(State, 0, 5);
            lua_pushinteger(State, (lua_Integer)minimum);
            lua_setfield(State, -2, "minimum");
            lua_pushinteger(State, (lua_Integer)maximum);
            lua_setfield(State, -2, "maximum");
            lua_pushinteger(State, (lua_Integer)size);
            lua_setfield(State, -2, "length");
            lua_pushinteger(State, (lua_Integer)alignment);
            lua_setfield(State, -2, "alignment");
            lua_pushinteger(State, (lua_Integer)descriptor->Flags);
            lua_setfield(State, -2, "flags");

            lua_rawseti(State, -2, ++index);
        }

        offset = next;
    }

    KphScriptReleaseCleanup(State, cleanup);
    lua_remove(State, cleanup);

    results = 1;
    status = STATUS_SUCCESS;

Exit:

    if (!NT_SUCCESS(status))
    {
        if (cleanup)
        {
            KphScriptReleaseCleanup(State, cleanup);
            lua_settop(State, cleanup - 1);
        }

        results = KphLuaNilErrorStatus(State, status);
    }

    return results;
}

/**
 * \brief Checks that an output buffer of an evaluation is well formed.
 *
 * \param[in] Buffer The output buffer.
 * \param[in] Length The number of bytes the ACPI driver returned.
 * \param[out] OutputLength Receives the length of the output buffer.
 *
 * \return Successful or errant status.
 */
_IRQL_requires_max_(PASSIVE_LEVEL)
_Must_inspect_result_
NTSTATUS KphpLuaHwAcpiValidateOutput(
    _In_reads_bytes_(Length) PVOID Buffer,
    _In_ ULONG Length,
    _Out_ PULONG OutputLength
    )
{
    PACPI_EVAL_OUTPUT_BUFFER output;
    ULONG headerLength;
    ULONG argumentHeaderLength;
    ULONG offset;

    KPH_PAGED_CODE_PASSIVE();

    *OutputLength = 0;

    headerLength = FIELD_OFFSET(ACPI_EVAL_OUTPUT_BUFFER, Argument);
    argumentHeaderLength = FIELD_OFFSET(ACPI_METHOD_ARGUMENT_V1, Data);

    if (Length < headerLength)
    {
        KphTracePrint(TRACE_LEVEL_VERBOSE,
                      HARDWARE,
                      "ACPI output is too short: %lu",
                      Length);

        return STATUS_ACPI_INVALID_DATA;
    }

    output = Buffer;

    if ((output->Signature != ACPI_EVAL_OUTPUT_BUFFER_SIGNATURE) ||
        (output->Length < headerLength) ||
        (output->Length > Length))
    {
        KphTracePrint(TRACE_LEVEL_VERBOSE,
                      HARDWARE,
                      "ACPI output header is malformed: 0x%08lx %lu of %lu",
                      output->Signature,
                      output->Length,
                      Length);

        return STATUS_ACPI_INVALID_DATA;
    }

    offset = headerLength;

    for (ULONG i = 0; i < output->Count; i++)
    {
        PACPI_METHOD_ARGUMENT_V1 argument;
        ULONG dataLength;
        ULONG argumentLength;

        if ((output->Length - offset) < argumentHeaderLength)
        {
            KphTracePrint(TRACE_LEVEL_VERBOSE,
                          HARDWARE,
                          "ACPI output argument %lu is truncated",
                          i);

            return STATUS_ACPI_INVALID_DATA;
        }

        argument = Add2Ptr(output, offset);
        dataLength = max((ULONG)sizeof(ULONG), argument->DataLength);

        NT_VERIFY(NT_SUCCESS(RtlULongAdd(argumentHeaderLength,
                                         dataLength,
                                         &argumentLength)));

        if (argumentLength > (output->Length - offset))
        {
            KphTracePrint(TRACE_LEVEL_VERBOSE,
                          HARDWARE,
                          "ACPI output argument %lu overruns: %lu",
                          i,
                          argumentLength);

            return STATUS_ACPI_INVALID_DATA;
        }

        offset += argumentLength;
    }

    *OutputLength = output->Length;

    return STATUS_SUCCESS;
}

/**
 * \brief Counts the argument records of an evaluation.
 *
 * \param[in] Arguments The records, back to back.
 * \param[in] Length The number of bytes of the records.
 * \param[out] Count Receives the number of records.
 *
 * \return TRUE if the records fill the bytes exactly.
 */
_IRQL_requires_max_(PASSIVE_LEVEL)
_Must_inspect_result_
BOOLEAN KphpLuaHwAcpiCountArguments(
    _In_reads_bytes_(Length) PCVOID Arguments,
    _In_ ULONG Length,
    _Out_ PULONG Count
    )
{
    ULONG headerLength;
    ULONG offset;
    ULONG count;

    KPH_PAGED_CODE_PASSIVE();

    *Count = 0;

    headerLength = FIELD_OFFSET(ACPI_METHOD_ARGUMENT_V1, Data);
    offset = 0;
    count = 0;

    while (offset < Length)
    {
        PACPI_METHOD_ARGUMENT_V1 argument;
        ULONG argumentLength;

        if ((Length - offset) < headerLength)
        {
            return FALSE;
        }

        argument = (PACPI_METHOD_ARGUMENT_V1)Add2Ptr(Arguments, offset);

        NT_VERIFY(NT_SUCCESS(RtlULongAdd(headerLength,
                                         max((ULONG)sizeof(ULONG),
                                             argument->DataLength),
                                         &argumentLength)));

        if (argumentLength > (Length - offset))
        {
            return FALSE;
        }

        offset += argumentLength;
        count++;
    }

    *Count = count;

    return TRUE;
}

/**
 * \brief Evaluates an object of an ACPI device (ksi.hw.acpi_eval).
 *
 * \details May raise a Lua error.
 *
 * \param[in] State The Lua state.
 *
 * \return The number of results: the output buffer, or nil and a status.
 */
_IRQL_requires_max_(PASSIVE_LEVEL)
int KphpLuaHwAcpiEval(
    _In_ lua_State* State
    )
{
    int results;
    NTSTATUS status;
    PKPH_LUA_HW_ACPI_DEVICE device;
    PCSTR name;
    SIZE_T nameLength;
    lua_Integer milliseconds;
    PCSTR arguments;
    SIZE_T argumentsLength;
    ULONG count;
    ULONG inputLength;
    PVOID input;
    ULONG outputLength;
    ULONG required;
    PVOID request;
    PVOID output;
    ULONG information;
    ULONG length;
    int cleanup;

    KPH_PAGED_CODE_PASSIVE();

    device = KphScriptCheckCleanup(State, 1, KphpLuaHwAcpiReleaseDevice);
    name = luaL_checklstring(State, 2, &nameLength);
    milliseconds = luaL_checkinteger(State, 3);
    arguments = luaL_optlstring(State, 4, "", &argumentsLength);

    count = 0;

    luaL_argcheck(State,
                  (nameLength >= 1) &&
                  (nameLength < RTL_FIELD_SIZE(ACPI_EVAL_INPUT_BUFFER_EX,
                                               MethodName)),
                  2,
                  "1 to 255 characters");
    luaL_argcheck(State,
                  (milliseconds >= 1) && (milliseconds < ULONG_MAX),
                  3,
                  "out of range");
    luaL_argcheck(State, argumentsLength <= ULONG_MAX, 4, "too long");

    KphScriptCharge(State,
                    argumentsLength /
                    FIELD_OFFSET(ACPI_METHOD_ARGUMENT_V1, Data));

    luaL_argcheck(State,
                  KphpLuaHwAcpiCountArguments(arguments,
                                              (ULONG)argumentsLength,
                                              &count) &&
                  (count <= 7),
                  4,
                  "not at most seven argument records");

    request = NULL;
    input = NULL;

    if (argumentsLength)
    {
        status = RtlULongAdd(FIELD_OFFSET(ACPI_EVAL_INPUT_BUFFER_COMPLEX_EX,
                                          Argument),
                             (ULONG)argumentsLength,
                             &inputLength);
        if (!NT_SUCCESS(status))
        {
            KphTracePrint(TRACE_LEVEL_VERBOSE,
                          HARDWARE,
                          "RtlULongAdd failed: %!STATUS!",
                          status);

            status = STATUS_INSUFFICIENT_RESOURCES;
            results = KphLuaNilErrorStatus(State, status);
            goto Exit;
        }
    }
    else
    {
        inputLength = sizeof(ACPI_EVAL_INPUT_BUFFER_EX);
    }

    input = KphAllocatePaged(inputLength, KPH_TAG_LUA_HARDWARE_ACPI_EVAL);
    if (!input)
    {
        KphTracePrint(TRACE_LEVEL_VERBOSE,
                      HARDWARE,
                      "Failed to allocate an ACPI evaluation input");

        status = STATUS_INSUFFICIENT_RESOURCES;
        results = KphLuaNilErrorStatus(State, status);
        goto Exit;
    }

    if (argumentsLength)
    {
        PACPI_EVAL_INPUT_BUFFER_COMPLEX_EX complex;

        complex = input;
        complex->Signature = ACPI_EVAL_INPUT_BUFFER_COMPLEX_SIGNATURE_EX;
        RtlCopyMemory(complex->MethodName, name, nameLength);
        complex->Size = (ULONG)argumentsLength;
        complex->ArgumentCount = count;
        RtlCopyMemory(complex->Argument, arguments, argumentsLength);
    }
    else
    {
        PACPI_EVAL_INPUT_BUFFER_EX simple;

        simple = input;
        simple->Signature = ACPI_EVAL_INPUT_BUFFER_SIGNATURE_EX;
        RtlCopyMemory(simple->MethodName, name, nameLength);
    }

    outputLength = PAGE_SIZE;

    for (ULONG attempt = 0; attempt < 2; attempt++)
    {
        status = KphLuaHwSendRequest(State,
                                     device->Pdo,
                                     NULL,
                                     IOCTL_ACPI_ASYNC_EVAL_METHOD_EX,
                                     FALSE,
                                     input,
                                     inputLength,
                                     outputLength,
                                     (ULONG)milliseconds,
                                     &request,
                                     &output,
                                     &information);
        if (status != STATUS_BUFFER_OVERFLOW)
        {
            break;
        }

        required = ((PACPI_EVAL_OUTPUT_BUFFER)output)->Length;

        if (((PACPI_EVAL_OUTPUT_BUFFER)output)->Signature !=
            ACPI_EVAL_OUTPUT_BUFFER_SIGNATURE)
        {
            KphTracePrint(TRACE_LEVEL_VERBOSE,
                          HARDWARE,
                          "ACPI overflow answer without an output header");

            KphLuaHwReleaseRequest(request);
            request = NULL;
            status = STATUS_ACPI_INVALID_DATA;
            break;
        }

        KphLuaHwReleaseRequest(request);
        request = NULL;

        KphTracePrint(TRACE_LEVEL_VERBOSE,
                      HARDWARE,
                      "ACPI output of %lu bytes is larger than %lu",
                      required,
                      outputLength);

        if (required <= outputLength)
        {
            break;
        }

        outputLength = required;
    }

    KphFree(input, KPH_TAG_LUA_HARDWARE_ACPI_EVAL);
    input = NULL;

    if (status == STATUS_SI_SCRIPT_LIMIT)
    {
        KphScriptCharge(State, 0);
    }

    if (status != STATUS_SUCCESS)
    {
        KphTracePrint(TRACE_LEVEL_VERBOSE,
                      HARDWARE,
                      "KphLuaHwSendRequest failed: %!STATUS!",
                      status);

        results = KphLuaNilErrorStatus(State, status);
        goto Exit;
    }

    NT_ASSERT(output);

    KphScriptPushCleanup(State, KphLuaHwReleaseRequest, request);
    cleanup = lua_gettop(State);

    status = KphpLuaHwAcpiValidateOutput(output, information, &length);
    if (!NT_SUCCESS(status))
    {
        KphTracePrint(TRACE_LEVEL_VERBOSE,
                      HARDWARE,
                      "KphpLuaHwAcpiValidateOutput failed: %!STATUS!",
                      status);

        KphScriptReleaseCleanup(State, cleanup);
        lua_remove(State, cleanup);
        results = KphLuaNilErrorStatus(State, status);
        goto Exit;
    }

    lua_pushlstring(State, output, length);

    KphScriptReleaseCleanup(State, cleanup);
    lua_remove(State, cleanup);

    results = 1;

Exit:

    if (input)
    {
        KphFree(input, KPH_TAG_LUA_HARDWARE_ACPI_EVAL);
    }

    return results;
}

/**
 * \brief Tells whether a value is a device object of ksi.hw.acpi_find or
 * ksi.hw.acpi_open.
 *
 * \param[in] State The Lua state.
 * \param[in] Index The index of the value.
 *
 * \return The physical device object, valid while the object is, or NULL.
 */
_IRQL_requires_max_(PASSIVE_LEVEL)
PDEVICE_OBJECT KphLuaHwAcpiTestDevice(
    _In_ lua_State* State,
    _In_ int Index
    )
{
    PKPH_LUA_HW_ACPI_DEVICE device;

    KPH_PAGED_CODE_PASSIVE();

    device = KphScriptTestCleanup(State, Index, KphpLuaHwAcpiReleaseDevice);
    if (!device)
    {
        return NULL;
    }

    return device->Pdo;
}

KPH_PROTECTED_DATA_SECTION_RO_PUSH();
static const luaL_Reg KphpLuaHwAcpiLibrary[] =
{
    { "acpi_eval", KphpLuaHwAcpiEval },
    { "acpi_find", KphpLuaHwAcpiFind },
    { "acpi_ids", KphpLuaHwAcpiIds },
    { "acpi_open", KphpLuaHwAcpiOpen },
    { "acpi_path", KphpLuaHwAcpiPath },
    { "acpi_requirements", KphpLuaHwAcpiRequirements },
    { "acpi_resources", KphpLuaHwAcpiResources },
    { NULL, NULL }
};
KPH_PROTECTED_DATA_SECTION_RO_POP();

/**
 * \brief Initializes the ACPI functions of the hardware library.
 */
_IRQL_requires_max_(PASSIVE_LEVEL)
VOID KphLuaInitializeHardwareAcpi(
    VOID
    )
{
    KPH_PAGED_CODE_PASSIVE();

    KphInitializePagedLookaside(&KphpLuaHwAcpiDeviceLookaside,
                                sizeof(KPH_LUA_HW_ACPI_DEVICE),
                                KPH_TAG_LUA_HARDWARE_ACPI);
}

/**
 * \brief Cleans up the ACPI functions of the hardware library.
 */
_IRQL_requires_max_(PASSIVE_LEVEL)
VOID KphLuaCleanupHardwareAcpi(
    VOID
    )
{
    KPH_PAGED_CODE_PASSIVE();

    KphDeletePagedLookaside(&KphpLuaHwAcpiDeviceLookaside);
}

/**
 * \brief Adds the ACPI functions to the hardware library.
 *
 * \details May raise a Lua error.
 *
 * \param[in] State The Lua state, with the library table on top.
 */
_IRQL_requires_max_(PASSIVE_LEVEL)
VOID KphLuaOpenHardwareAcpi(
    _In_ lua_State* State
    )
{
    KPH_PAGED_CODE_PASSIVE();

    luaL_setfuncs(State, KphpLuaHwAcpiLibrary, 0);
}
