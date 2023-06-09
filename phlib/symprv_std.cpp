/*
 * Copyright (c) 2022 Winsider Seminars & Solutions, Inc.  All rights reserved.
 *
 * This file is part of System Informer.
 *
 * Authors:
 *
 *     dmex    2023
 *
 */

#include <ph.h>
#include <ref.h>
#include <refp.h>

#include <stacktrace>

using namespace std;

EXTERN_C VOID PhPrintCurrentStacktrace(
    VOID
    )
{
#ifdef DEBUG
    stacktrace trace = stacktrace::current(1);
    string result = to_string(trace);

    OutputDebugStringA(result.c_str());
#endif
}

EXTERN_C PPH_STRING PhGetStacktraceAsString(
    VOID
    )
{
#ifdef DEBUG
    stacktrace trace = stacktrace::current(1);
    string result = to_string(trace);

    return PhConvertUtf8ToUtf16(const_cast<PSTR>(result.c_str()));
#else
    return NULL;
#endif
}

EXTERN_C PPH_STRING PhGetStacktraceSymbolFromAddress(
    _In_ PVOID Address
    )
{
#ifdef DEBUG
    string result;

    __std_stacktrace_address_to_string(
        Address,
        &result,
        _Stacktrace_string_fill_impl
        );

    return PhConvertUtf8ToUtf16(const_cast<PSTR>(result.c_str()));
#else
    return NULL;
#endif
}

EXTERN_C PPH_STRING PhGetObjectTypeStacktraceToString(
    _In_ PVOID Object
    )
{
#ifdef DEBUG
    PPH_OBJECT_HEADER objectHeader = reinterpret_cast<PPH_OBJECT_HEADER>(PhObjectToObjectHeader(Object));
    string result;

    __std_stacktrace_to_string(
        objectHeader->StackBackTrace,
        ARRAYSIZE(objectHeader->StackBackTrace),
        &result,
        _Stacktrace_string_fill_impl
        );

    return PhConvertUtf8ToUtf16(const_cast<PSTR>(result.c_str()));
#else
    return NULL;
#endif
}
