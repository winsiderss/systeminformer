/*
 * Copyright (c) 2022 Winsider Seminars & Solutions, Inc.  All rights reserved.
 *
 * This file is part of System Informer.
 *
 * Authors:
 *
 *     jxy-s   2023-2026
 *
 */

#pragma once

#include <kphdyn.h>

_Must_inspect_result_
NTSTATUS KphDynDataLookup(
    _In_reads_bytes_(Length) PKPH_DYN_CONFIG Config,
    _In_ ULONG Length,
    _In_ USHORT Class,
    _In_ USHORT Machine,
    _In_ ULONG TimeDateStamp,
    _In_ ULONG SizeOfImage,
    _Outptr_opt_ PKPH_DYN_DATA* Data,
    _In_ ULONG FieldsLength,
    _Outptr_opt_result_buffer_(FieldsLength) PVOID* Fields
    );

_Must_inspect_result_
NTSTATUS KphDynDataLookupScriptSection(
    _In_reads_bytes_(Length) PKPH_DYN_CONFIG Config,
    _In_ ULONG Length,
    _Outptr_result_bytebuffer_(*SectionLength) PKPH_DYN_SCRIPT_SECTION* Section,
    _Out_ PULONG SectionLength
    );

_Must_inspect_result_
NTSTATUS KphDynDataLookupScript(
    _In_reads_bytes_(SectionLength) PKPH_DYN_SCRIPT_SECTION Section,
    _In_ ULONG SectionLength,
    _In_ PCANSI_STRING Name,
    _Outptr_ PKPH_DYN_SCRIPT* Script
    );

_Must_inspect_result_
NTSTATUS KphDynDataLookupScriptRoutine(
    _In_reads_bytes_(SectionLength) PKPH_DYN_SCRIPT_SECTION Section,
    _In_ ULONG SectionLength,
    _In_ PKPH_DYN_SCRIPT Script,
    _In_ PCANSI_STRING Name,
    _Outptr_ PKPH_DYN_SCRIPT_ROUTINE* Routine
    );
