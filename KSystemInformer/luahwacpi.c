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

KPH_PAGED_FILE();

/**
 * \brief Checks a device object argument.
 *
 * \details May raise a Lua error.
 *
 * \param[in] State The Lua state.
 * \param[in] Index The index of the argument.
 *
 * \return The physical device object, valid while the object is.
 */
_IRQL_requires_max_(PASSIVE_LEVEL)
PDEVICE_OBJECT KphpLuaHwAcpiCheckDevice(
    _In_ lua_State* State,
    _In_ int Index
    )
{
    PDEVICE_OBJECT pdo;

    KPH_PAGED_CODE_PASSIVE();

    pdo = KphLuaHwTestDevice(State, Index);

    luaL_argcheck(State, pdo, Index, "a device object");

    NT_ASSERT(pdo);

    return pdo;
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
 * \return The number of results: the output buffer, or nil, a status and,
 * when the output does not fit, the size required.
 */
_IRQL_requires_max_(PASSIVE_LEVEL)
int KphpLuaHwAcpiEval(
    _In_ lua_State* State
    )
{
    int results;
    NTSTATUS status;
    PDEVICE_OBJECT pdo;
    PCSTR name;
    SIZE_T nameLength;
    lua_Integer milliseconds;
    PCSTR arguments;
    SIZE_T argumentsLength;
    lua_Integer size;
    ULONG count;
    ULONG inputLength;
    PVOID input;
    ULONG signature;
    ULONG required;
    PVOID request;
    PVOID output;
    ULONG information;
    ULONG length;
    int cleanup;
    BYTE stackBuffer[sizeof(ACPI_EVAL_INPUT_BUFFER_COMPLEX_EX)];

    KPH_PAGED_CODE_PASSIVE();

    pdo = KphpLuaHwAcpiCheckDevice(State, 1);
    name = luaL_checklstring(State, 2, &nameLength);
    milliseconds = luaL_checkinteger(State, 3);
    arguments = luaL_optlstring(State, 4, "", &argumentsLength);
    size = luaL_optinteger(State, 5, PAGE_SIZE);

    count = 0;
    required = 0;

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
    luaL_argcheck(State, (size >= 1) && (size <= ULONG_MAX), 5, "out of range");

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

    input = KphAllocatePagedA(inputLength,
                              KPH_TAG_LUA_HARDWARE_ACPI_EVAL,
                              stackBuffer);
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

    status = KphLuaHwSendRequest(State,
                                 pdo,
                                 NULL,
                                 IOCTL_ACPI_ASYNC_EVAL_METHOD_EX,
                                 FALSE,
                                 input,
                                 inputLength,
                                 (ULONG)size,
                                 (ULONG)milliseconds,
                                 &request,
                                 &output,
                                 &information);
    if (status == STATUS_BUFFER_OVERFLOW)
    {
        signature = ((PACPI_EVAL_OUTPUT_BUFFER)output)->Signature;
        required = ((PACPI_EVAL_OUTPUT_BUFFER)output)->Length;

        KphLuaHwReleaseRequest(request);
        request = NULL;

        if (signature != ACPI_EVAL_OUTPUT_BUFFER_SIGNATURE)
        {
            KphTracePrint(TRACE_LEVEL_VERBOSE,
                          HARDWARE,
                          "ACPI overflow answer without an output header");

            status = STATUS_ACPI_INVALID_DATA;
        }
        else
        {
            KphTracePrint(TRACE_LEVEL_VERBOSE,
                          HARDWARE,
                          "ACPI output of %lu bytes is larger than %lu",
                          required,
                          (ULONG)size);
        }
    }

    KphFreeA(input, KPH_TAG_LUA_HARDWARE_ACPI_EVAL, stackBuffer);
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

        if (status == STATUS_BUFFER_OVERFLOW)
        {
            lua_pushinteger(State, (lua_Integer)required);
            results++;
        }

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
        KphFreeA(input, KPH_TAG_LUA_HARDWARE_ACPI_EVAL, stackBuffer);
    }

    return results;
}

KPH_PROTECTED_DATA_SECTION_RO_PUSH();
static const luaL_Reg KphpLuaHwAcpiLibrary[] =
{
    { "acpi_eval", KphpLuaHwAcpiEval },
    { NULL, NULL }
};
KPH_PROTECTED_DATA_SECTION_RO_POP();

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
