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

#include <kphlibbase.h>
#include <kphdyndata.h>

/**
 * \brief Searches for a dynamic data configuration.
 *
 * \param[in] Config The dynamic data configuration.
 * \param[in] Length The length of the dynamic data configuration.
 * \param[in] Class The class to search for.
 * \param[in] Machine The machine to search for.
 * \param[in] TimeDateStamp The time date stamp to search for.
 * \param[in] SizeOfImage The size of image to search for.
 * \param[out] Data Receives a pointer into the dynamic data configuration if
 * the requested data is found.
 * \param[in] FieldsLength The expected length, in bytes, of the fields
 * structure for the requested class.
 * \param[out] Fields Receives a pointer into the dynamic data configuration for
 * the related fields if the requested data is found.
 *
 * \return Successful or errant status.
 */
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
    )
{
    NTSTATUS status;
    ULONG dataLength;
    ULONG length;

    if (Data)
    {
        *Data = NULL;
    }

    if (Fields)
    {
        *Fields = NULL;
    }

    status = RtlULongSub(Length,
                         RTL_SIZEOF_THROUGH_FIELD(KPH_DYN_CONFIG, ScriptLength),
                         &length);
    if (!NT_SUCCESS(status))
    {
        return STATUS_SI_DYNDATA_INVALID_LENGTH;
    }

    if (Config->Version != KPH_DYN_CONFIGURATION_VERSION)
    {
        return STATUS_SI_DYNDATA_VERSION_MISMATCH;
    }

    status = RtlULongMult(sizeof(KPH_DYN_DATA), Config->Count, &dataLength);
    if (!NT_SUCCESS(status))
    {
        return STATUS_SI_DYNDATA_INVALID_LENGTH;
    }

    status = RtlULongSub(length, dataLength, &length);
    if (!NT_SUCCESS(status))
    {
        return STATUS_SI_DYNDATA_INVALID_LENGTH;
    }

    status = RtlULongSub(length, Config->ScriptLength, &length);
    if (!NT_SUCCESS(status))
    {
        return STATUS_SI_DYNDATA_INVALID_LENGTH;
    }

    for (ULONG i = 0; i < Config->Count; i++)
    {
        PKPH_DYN_DATA data;

        data = &Config->Data[i];

        if ((data->Class != Class) ||
            (data->Machine != Machine) ||
            (data->TimeDateStamp != TimeDateStamp) ||
            (data->SizeOfImage != SizeOfImage))
        {
            continue;
        }

        if (Fields)
        {
            PVOID start;
            ULONG end;

            status = RtlULongAdd(data->Offset, FieldsLength, &end);
            if (!NT_SUCCESS(status) || (end > length))
            {
                return STATUS_SI_DYNDATA_INVALID_LENGTH;
            }

            start = &Config->Data[Config->Count];

            *Fields = Add2Ptr(start, data->Offset);
        }

        if (Data)
        {
            *Data = data;
        }

        return STATUS_SUCCESS;
    }

    return STATUS_SI_DYNDATA_UNSUPPORTED_KERNEL;
}

/**
 * \brief Locates the script section of a dynamic data configuration.
 *
 * \param[in] Config The dynamic data configuration.
 * \param[in] Length The length of the dynamic data configuration.
 * \param[out] Section Receives a pointer into the dynamic data configuration
 * to the script section.
 * \param[out] SectionLength Receives the length of the script section.
 *
 * \return Successful or errant status.
 */
_Must_inspect_result_
NTSTATUS KphDynDataLookupScriptSection(
    _In_reads_bytes_(Length) PKPH_DYN_CONFIG Config,
    _In_ ULONG Length,
    _Outptr_result_bytebuffer_(*SectionLength) PKPH_DYN_SCRIPT_SECTION* Section,
    _Out_ PULONG SectionLength
    )
{
    NTSTATUS status;
    PKPH_DYN_SCRIPT_SECTION section;
    ULONG length;

    *Section = NULL;
    *SectionLength = 0;

    if (Length < RTL_SIZEOF_THROUGH_FIELD(KPH_DYN_CONFIG, ScriptLength))
    {
        return STATUS_SI_DYNDATA_INVALID_LENGTH;
    }

    if (Config->Version != KPH_DYN_CONFIGURATION_VERSION)
    {
        return STATUS_SI_DYNDATA_VERSION_MISMATCH;
    }

    if (Config->ScriptLength == 0)
    {
        return STATUS_NOT_FOUND;
    }

    status = RtlULongAdd(Config->ScriptOffset, Config->ScriptLength, &length);
    if (!NT_SUCCESS(status) || (length > Length))
    {
        return STATUS_SI_DYNDATA_INVALID_LENGTH;
    }

    if (Config->ScriptLength < UFIELD_OFFSET(KPH_DYN_SCRIPT_SECTION, Scripts))
    {
        return STATUS_SI_DYNDATA_INVALID_LENGTH;
    }

    section = Add2Ptr(Config, Config->ScriptOffset);

    status = RtlULongMult(section->Count, sizeof(KPH_DYN_SCRIPT), &length);
    if (!NT_SUCCESS(status))
    {
        return STATUS_SI_DYNDATA_INVALID_LENGTH;
    }

    status = RtlULongAdd(length,
                         UFIELD_OFFSET(KPH_DYN_SCRIPT_SECTION, Scripts),
                         &length);
    if (!NT_SUCCESS(status) || (length > Config->ScriptLength))
    {
        return STATUS_SI_DYNDATA_INVALID_LENGTH;
    }

    *Section = section;
    *SectionLength = Config->ScriptLength;

    return STATUS_SUCCESS;
}

/**
 * \brief Checks that a span of a script section lies within it.
 *
 * \param[in] SectionLength The length of the script section.
 * \param[in] Offset The offset of the span from the start of the section.
 * \param[in] Length The length of the span.
 *
 * \return TRUE if the span lies within the section, FALSE otherwise.
 */
_Must_inspect_result_
BOOLEAN KphpDynDataScriptSpan(
    _In_ ULONG SectionLength,
    _In_ ULONG Offset,
    _In_ ULONG Length
    )
{
    NTSTATUS status;
    ULONG end;

    status = RtlULongAdd(Offset, Length, &end);
    if (!NT_SUCCESS(status))
    {
        return FALSE;
    }

    if (end > SectionLength)
    {
        return FALSE;
    }

    return TRUE;
}

/**
 * \brief Compares a name in a script section with a name.
 *
 * \param[in] Section The script section.
 * \param[in] Offset The offset of the name from the start of the section.
 * \param[in] Length The length of the name.
 * \param[in] Name The name to compare with.
 *
 * \return Negative if the name in the section orders before the name, zero
 * if they are equal, positive otherwise.
 */
_Must_inspect_result_
LONG KphpDynDataScriptNameCompare(
    _In_ PKPH_DYN_SCRIPT_SECTION Section,
    _In_ ULONG Offset,
    _In_ USHORT Length,
    _In_ PCANSI_STRING Name
    )
{
    ANSI_STRING first;
    ANSI_STRING second;

    first.Buffer = Add2Ptr(Section, Offset);
    first.Length = Length;
    first.MaximumLength = Length;

    second.Buffer = Name->Buffer;
    second.Length = Name->Length;
    second.MaximumLength = Name->MaximumLength;

    return RtlCompareString(&first, &second, FALSE);
}

/**
 * \brief Searches a script section for a script.
 *
 * \details The scripts of a section are ordered by name.
 *
 * \param[in] Section The script section.
 * \param[in] SectionLength The length of the script section.
 * \param[in] Name The name of the script.
 * \param[out] Script Receives a pointer into the script section to the
 * script. Its routines and its chunk lie within the section.
 *
 * \return Successful or errant status.
 */
_Must_inspect_result_
NTSTATUS KphDynDataLookupScript(
    _In_reads_bytes_(SectionLength) PKPH_DYN_SCRIPT_SECTION Section,
    _In_ ULONG SectionLength,
    _In_ PCANSI_STRING Name,
    _Outptr_ PKPH_DYN_SCRIPT* Script
    )
{
    NTSTATUS status;
    PKPH_DYN_SCRIPT script;
    ULONG length;
    ULONG low;
    ULONG high;
    ULONG middle;
    LONG order;

    *Script = NULL;

    low = 0;
    high = Section->Count;

    while (low < high)
    {
        middle = low + ((high - low) / 2);

        script = &Section->Scripts[middle];

        if (!KphpDynDataScriptSpan(SectionLength,
                                   script->NameOffset,
                                   script->NameLength))
        {
            return STATUS_SI_DYNDATA_INVALID_LENGTH;
        }

        order = KphpDynDataScriptNameCompare(Section,
                                             script->NameOffset,
                                             script->NameLength,
                                             Name);
        if (order < 0)
        {
            low = middle + 1;
            continue;
        }

        if (order > 0)
        {
            high = middle;
            continue;
        }

        status = RtlULongMult(script->RoutineCount,
                              sizeof(KPH_DYN_SCRIPT_ROUTINE),
                              &length);
        if (!NT_SUCCESS(status))
        {
            return STATUS_SI_DYNDATA_INVALID_LENGTH;
        }

        if (!KphpDynDataScriptSpan(SectionLength,
                                   script->RoutinesOffset,
                                   length))
        {
            return STATUS_SI_DYNDATA_INVALID_LENGTH;
        }

        if (!KphpDynDataScriptSpan(SectionLength,
                                   script->ChunkOffset,
                                   script->ChunkLength))
        {
            return STATUS_SI_DYNDATA_INVALID_LENGTH;
        }

        *Script = script;

        return STATUS_SUCCESS;
    }

    return STATUS_NOT_FOUND;
}

/**
 * \brief Searches a script for a routine it exports.
 *
 * \details The routines of a script are ordered by name.
 *
 * \param[in] Section The script section.
 * \param[in] SectionLength The length of the script section.
 * \param[in] Script The script, from KphDynDataLookupScript.
 * \param[in] Name The name of the routine.
 * \param[out] Routine Receives a pointer into the script section to the
 * routine.
 *
 * \return Successful or errant status.
 */
_Must_inspect_result_
NTSTATUS KphDynDataLookupScriptRoutine(
    _In_reads_bytes_(SectionLength) PKPH_DYN_SCRIPT_SECTION Section,
    _In_ ULONG SectionLength,
    _In_ PKPH_DYN_SCRIPT Script,
    _In_ PCANSI_STRING Name,
    _Outptr_ PKPH_DYN_SCRIPT_ROUTINE* Routine
    )
{
    PKPH_DYN_SCRIPT_ROUTINE routines;
    PKPH_DYN_SCRIPT_ROUTINE routine;
    ULONG low;
    ULONG high;
    ULONG middle;
    LONG order;

    *Routine = NULL;

    routines = Add2Ptr(Section, Script->RoutinesOffset);

    low = 0;
    high = Script->RoutineCount;

    while (low < high)
    {
        middle = low + ((high - low) / 2);

        routine = &routines[middle];

        if (!KphpDynDataScriptSpan(SectionLength,
                                   routine->NameOffset,
                                   routine->NameLength))
        {
            return STATUS_SI_DYNDATA_INVALID_LENGTH;
        }

        order = KphpDynDataScriptNameCompare(Section,
                                             routine->NameOffset,
                                             routine->NameLength,
                                             Name);
        if (order < 0)
        {
            low = middle + 1;
            continue;
        }

        if (order > 0)
        {
            high = middle;
            continue;
        }

        *Routine = routine;

        return STATUS_SUCCESS;
    }

    return STATUS_NOT_FOUND;
}
