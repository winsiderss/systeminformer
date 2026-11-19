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

#include <ph.h>

#include <hwacpi.h>

/**
 * Reads the header of an ACPI method argument that lies entirely inside a buffer.
 *
 * \param Size Receives the size of the argument, header included.
 */
_Success_(return)
BOOLEAN NTAPI PhHwAcpiReadArgument(
    _In_reads_bytes_(Length) const UCHAR* Buffer,
    _In_ ULONG Length,
    _In_ ULONG Offset,
    _Out_ PUSHORT Type,
    _Out_ PUSHORT DataLength,
    _Out_ PULONG Size
    )
{
    USHORT type;
    USHORT dataLength;
    ULONG size;

    if (Offset > Length || Length - Offset < PH_HW_ACPI_ARGUMENT_HEADER)
        return FALSE;

    memcpy(&type, Buffer + Offset, sizeof(USHORT));
    memcpy(&dataLength, Buffer + Offset + sizeof(USHORT), sizeof(USHORT));
    size = PH_HW_ACPI_ARGUMENT_HEADER + max((ULONG)sizeof(ULONG), (ULONG)dataLength);

    if (Length - Offset < size)
        return FALSE;

    *Type = type;
    *DataLength = dataLength;
    *Size = size;
    return TRUE;
}

/**
 * Reads consecutive 4- or 8-byte integer arguments that lie inside a buffer, up to the first other argument.
 *
 * \param End The end of the arguments: the output length, or the end of a package's data.
 * \return The number of integers read, at most Count.
 */
ULONG NTAPI PhHwAcpiReadIntegersAt(
    _In_reads_bytes_(End) const UCHAR* Buffer,
    _In_ ULONG Offset,
    _In_ ULONG End,
    _Out_writes_to_(Count, return) PULONG64 Values,
    _In_ ULONG Count
    )
{
    ULONG read = 0;

    while (read < Count)
    {
        USHORT type;
        USHORT dataLength;
        ULONG size;

        if (!PhHwAcpiReadArgument(Buffer, End, Offset, &type, &dataLength, &size))
            break;
        if (type != PH_HW_ACPI_ARGUMENT_INTEGER || (dataLength != sizeof(ULONG) && dataLength != sizeof(ULONG64)))
            break;

        Values[read] = 0;
        memcpy(&Values[read], Buffer + Offset + PH_HW_ACPI_ARGUMENT_HEADER, dataLength);
        read++;
        Offset += size;
    }

    return read;
}

/**
 * Finds the elements of an ACPI method output: the top-level arguments, or the elements of one top-level package.
 *
 * \param Offset Receives the offset of the first element.
 * \param End Receives the end of the elements.
 * \param Elements Receives the number of top-level elements, or ULONG_MAX for the elements of a package.
 */
_Success_(return)
BOOLEAN NTAPI PhHwAcpiGetElements(
    _In_reads_bytes_(Length) const UCHAR* Buffer,
    _In_ ULONG Length,
    _Out_ PULONG Offset,
    _Out_ PULONG End,
    _Out_ PULONG Elements
    )
{
    ULONG signature;
    ULONG outputLength;
    ULONG count;
    USHORT type;
    USHORT dataLength;
    ULONG size;

    if (Length < PH_HW_ACPI_OUTPUT_HEADER)
        return FALSE;

    memcpy(&signature, Buffer, sizeof(ULONG));
    memcpy(&outputLength, Buffer + sizeof(ULONG), sizeof(ULONG));
    memcpy(&count, Buffer + sizeof(ULONG) * 2, sizeof(ULONG));

    if (signature != PH_HW_ACPI_OUTPUT_SIGNATURE || outputLength < PH_HW_ACPI_OUTPUT_HEADER || outputLength > Length || !count)
        return FALSE;
    if (!PhHwAcpiReadArgument(Buffer, outputLength, PH_HW_ACPI_OUTPUT_HEADER, &type, &dataLength, &size))
        return FALSE;

    // The elements lie in the package data, which lies in the output.
    if (count == 1 && type == PH_HW_ACPI_ARGUMENT_PACKAGE)
    {
        *Offset = PH_HW_ACPI_OUTPUT_HEADER + PH_HW_ACPI_ARGUMENT_HEADER;
        *End = PH_HW_ACPI_OUTPUT_HEADER + PH_HW_ACPI_ARGUMENT_HEADER + dataLength;
        *Elements = ULONG_MAX;
        return TRUE;
    }

    *Offset = PH_HW_ACPI_OUTPUT_HEADER;
    *End = outputLength;
    *Elements = count;
    return TRUE;
}

/**
 * Reads the leading integers of an ACPI method output, at the top level or in one package.
 *
 * \return The number of integers read, at most Count.
 */
ULONG NTAPI PhHwAcpiReadIntegers(
    _In_reads_bytes_(Length) const UCHAR* Buffer,
    _In_ ULONG Length,
    _Out_writes_to_(Count, return) PULONG64 Values,
    _In_ ULONG Count
    )
{
    ULONG offset;
    ULONG end;
    ULONG elements;

    if (!PhHwAcpiGetElements(Buffer, Length, &offset, &end, &elements))
        return 0;

    return PhHwAcpiReadIntegersAt(Buffer, offset, end, Values, min(Count, elements));
}

/**
 * Decodes an _FST result (ACPI 6.5 11.3.1.4): Revision, Control and Speed in RPM.
 * The elements come as top-level arguments or inside one package argument.
 *
 * \param Control Receives the control value: a percentage on a fine-grain fan, else an _FPS control.
 */
_Success_(return == PhHwFanDecodeOk)
PH_HW_FAN_DECODE NTAPI PhHwDecodeFanStatus(
    _In_reads_bytes_(Length) const UCHAR* Buffer,
    _In_ ULONG Length,
    _Out_ PULONG Rpm,
    _Out_ PULONG64 Control
    )
{
    ULONG64 values[3];

    if (PhHwAcpiReadIntegers(Buffer, Length, values, RTL_NUMBER_OF(values)) != RTL_NUMBER_OF(values))
        return PhHwFanDecodeMalformed;

    if (values[2] == ULONG_MAX || values[2] == ULONG64_MAX)
        return PhHwFanDecodeUnknown;
    if (values[2] > PH_HW_FAN_MAX_RPM)
        return PhHwFanDecodeImplausible;

    *Rpm = (ULONG)values[2];
    *Control = values[1];
    return PhHwFanDecodeOk;
}

/**
 * Decodes an _FPS result (ACPI 6.5 11.3.1.2): Revision, then one package per state of Control, TripPoint, Speed,
 * NoiseLevel and Power.
 *
 * \return The number of states decoded, at most Count; a malformed state ends the table.
 */
ULONG NTAPI PhHwDecodeFanStates(
    _In_reads_bytes_(Length) const UCHAR* Buffer,
    _In_ ULONG Length,
    _Out_writes_to_(Count, return) PPH_HW_FAN_STATE States,
    _In_ ULONG Count
    )
{
    ULONG offset;
    ULONG end;
    ULONG elements;
    USHORT revisionType;
    USHORT revisionLength;
    ULONG revisionSize;
    ULONG states = 0;

    if (!PhHwAcpiGetElements(Buffer, Length, &offset, &end, &elements) || !elements)
        return 0;
    if (!PhHwAcpiReadArgument(Buffer, end, offset, &revisionType, &revisionLength, &revisionSize) || revisionType != PH_HW_ACPI_ARGUMENT_INTEGER)
        return 0;

    offset += revisionSize;

    while (states < Count && states < elements - 1)
    {
        USHORT type;
        USHORT dataLength;
        ULONG size;
        ULONG64 values[5];

        if (!PhHwAcpiReadArgument(Buffer, end, offset, &type, &dataLength, &size) || type != PH_HW_ACPI_ARGUMENT_PACKAGE)
            break;

        if (PhHwAcpiReadIntegersAt(
            Buffer,
            offset + PH_HW_ACPI_ARGUMENT_HEADER,
            offset + PH_HW_ACPI_ARGUMENT_HEADER + dataLength,
            values,
            RTL_NUMBER_OF(values)
            ) != RTL_NUMBER_OF(values))
        {
            break;
        }

        for (ULONG i = 0; i < RTL_NUMBER_OF(values); i++)
            States[states].Values[i] = values[i] > ULONG_MAX ? ULONG_MAX : (ULONG)values[i];

        states++;
        offset += size;
    }

    return states;
}

/**
 * Reads the first argument of an ACPI method output.
 *
 * \param Offset Receives the offset of the argument data.
 */
_Success_(return)
BOOLEAN NTAPI PhHwAcpiFirstArgument(
    _In_reads_bytes_(Length) const UCHAR* Buffer,
    _In_ ULONG Length,
    _Out_ PUSHORT Type,
    _Out_ PUSHORT DataLength,
    _Out_ PULONG Offset
    )
{
    ULONG signature;
    ULONG outputLength;
    ULONG count;
    ULONG size;

    if (Length < PH_HW_ACPI_OUTPUT_HEADER)
        return FALSE;

    memcpy(&signature, Buffer, sizeof(ULONG));
    memcpy(&outputLength, Buffer + sizeof(ULONG), sizeof(ULONG));
    memcpy(&count, Buffer + sizeof(ULONG) * 2, sizeof(ULONG));

    if (signature != PH_HW_ACPI_OUTPUT_SIGNATURE || outputLength < PH_HW_ACPI_OUTPUT_HEADER || outputLength > Length || !count)
        return FALSE;
    if (!PhHwAcpiReadArgument(Buffer, outputLength, PH_HW_ACPI_OUTPUT_HEADER, Type, DataLength, &size))
        return FALSE;

    *Offset = PH_HW_ACPI_OUTPUT_HEADER + PH_HW_ACPI_ARGUMENT_HEADER;
    return TRUE;
}

/**
 * Reads an ACPI method output that is one 4- or 8-byte integer.
 */
_Success_(return)
BOOLEAN NTAPI PhHwAcpiReadInteger(
    _In_reads_bytes_(Length) const UCHAR* Buffer,
    _In_ ULONG Length,
    _Out_ PULONG64 Value
    )
{
    USHORT type;
    USHORT dataLength;
    ULONG offset;
    ULONG64 value = 0;

    if (!PhHwAcpiFirstArgument(Buffer, Length, &type, &dataLength, &offset))
        return FALSE;
    if (type != PH_HW_ACPI_ARGUMENT_INTEGER || (dataLength != sizeof(ULONG) && dataLength != sizeof(ULONG64)))
        return FALSE;

    memcpy(&value, Buffer + offset, dataLength);
    *Value = value;
    return TRUE;
}

/**
 * Reads an ACPI method output that is a string, or a buffer of UTF-16LE (_STR).
 *
 * \return The string without trailing NULs, or NULL when it is empty or of another type.
 */
_Ret_maybenull_
PPH_STRING NTAPI PhHwAcpiReadString(
    _In_reads_bytes_(Length) const UCHAR* Buffer,
    _In_ ULONG Length
    )
{
    USHORT type;
    USHORT dataLength;
    ULONG offset;
    PPH_STRING string;

    if (!PhHwAcpiFirstArgument(Buffer, Length, &type, &dataLength, &offset))
        return NULL;

    if (type == PH_HW_ACPI_ARGUMENT_BUFFER)
    {
        string = PhCreateStringEx((PCWCHAR)(Buffer + offset), dataLength & ~1);
    }
    else if (type == PH_HW_ACPI_ARGUMENT_STRING)
    {
        string = PhZeroExtendToUtf16Ex((PCSTR)(Buffer + offset), dataLength);
    }
    else
    {
        return NULL;
    }

    // The string ends at its first NUL.
    for (SIZE_T i = 0; i < string->Length / sizeof(WCHAR); i++)
    {
        if (string->Buffer[i] == UNICODE_NULL)
        {
            string->Length = i * sizeof(WCHAR);
            break;
        }
    }

    if (!string->Length)
    {
        PhDereferenceObject(string);
        return NULL;
    }

    return string;
}

/**
 * Decodes a DPTF _TMP result: tenths of a kelvin; 0, all ones and readings outside
 * PH_HW_DPTF_MIN_DECIKELVIN-PH_HW_DPTF_MAX_DECIKELVIN are not readings.
 */
_Success_(return == PhHwFanDecodeOk)
PH_HW_FAN_DECODE NTAPI PhHwDecodeDptfTemperature(
    _In_reads_bytes_(Length) const UCHAR* Buffer,
    _In_ ULONG Length,
    _Out_ PFLOAT Celsius
    )
{
    ULONG64 value;

    if (!PhHwAcpiReadInteger(Buffer, Length, &value))
        return PhHwFanDecodeMalformed;
    if (value == 0 || value == ULONG_MAX || value == ULONG64_MAX)
        return PhHwFanDecodeUnknown;
    if (value < PH_HW_DPTF_MIN_DECIKELVIN || value > PH_HW_DPTF_MAX_DECIKELVIN)
        return PhHwFanDecodeImplausible;

    *Celsius = (FLOAT)value / 10.0f - 273.15f;
    return PhHwFanDecodeOk;
}

/**
 * Decodes an ATK0110 reader result: tenths of a degree, mV or RPM.
 *
 * \param Op The reader: PH_HW_ATK_OP_RTMP, PH_HW_ATK_OP_RVLT or PH_HW_ATK_OP_RFAN.
 */
_Success_(return == PhHwFanDecodeOk)
PH_HW_FAN_DECODE NTAPI PhHwDecodeAtkReading(
    _In_ UCHAR Op,
    _In_reads_bytes_(Length) const UCHAR* Buffer,
    _In_ ULONG Length,
    _Out_ PFLOAT Value
    )
{
    ULONG64 raw;
    LONG value;

    if (!PhHwAcpiReadInteger(Buffer, Length, &raw))
        return PhHwFanDecodeMalformed;
    if (raw == ULONG_MAX || raw == ULONG64_MAX)
        return PhHwFanDecodeUnknown;

    value = (LONG)(ULONG)raw;

    switch (Op)
    {
    case PH_HW_ATK_OP_RTMP:
        if (value < -400 || value > 1500)
            return PhHwFanDecodeImplausible;
        *Value = (FLOAT)value / 10.0f;
        return PhHwFanDecodeOk;
    case PH_HW_ATK_OP_RVLT:
        if (value < 0 || value > 20000)
            return PhHwFanDecodeImplausible;
        *Value = (FLOAT)value / 1000.0f;
        return PhHwFanDecodeOk;
    default:
        if (value < 0 || value > PH_HW_FAN_MAX_RPM)
            return PhHwFanDecodeImplausible;
        *Value = (FLOAT)value;
        return PhHwFanDecodeOk;
    }
}

/**
 * Decodes an ACPI000D _PMC result (ACPI 6.5 10.4.1): PH_HW_METER_PMC_INTEGERS integers, then the model, serial
 * and OEM strings, into the report.
 *
 * \return The number of leading integers.
 */
ULONG NTAPI PhHwDecodePowerMeterCapabilities(
    _In_reads_bytes_(Length) const UCHAR* Buffer,
    _In_ ULONG Length,
    _Inout_ PPH_HW_ACPI_REPORT Report
    )
{
    ULONG offset;
    ULONG end;
    ULONG elements;
    ULONG count = 0;

    if (!PhHwAcpiGetElements(Buffer, Length, &offset, &end, &elements))
        return 0;

    for (ULONG i = 0; i < PH_HW_METER_PMC_ELEMENTS && i < elements; i++)
    {
        USHORT type;
        USHORT dataLength;
        ULONG size;

        if (!PhHwAcpiReadArgument(Buffer, end, offset, &type, &dataLength, &size))
            break;

        if (i < PH_HW_METER_PMC_INTEGERS)
        {
            ULONG64 value = 0;

            if (type != PH_HW_ACPI_ARGUMENT_INTEGER || (dataLength != sizeof(ULONG) && dataLength != sizeof(ULONG64)))
                break;

            memcpy(&value, Buffer + offset + PH_HW_ACPI_ARGUMENT_HEADER, dataLength);
            Report->Meter[i] = value > ULONG_MAX ? ULONG_MAX : (ULONG)value;
            count++;
        }
        else
        {
            PSTR string = Report->MeterStrings[i - PH_HW_METER_PMC_INTEGERS];
            PCCH data = (PCCH)(Buffer + offset + PH_HW_ACPI_ARGUMENT_HEADER);
            ULONG length = 0;

            if (type != PH_HW_ACPI_ARGUMENT_STRING)
                break;

            while (length < dataLength && length < sizeof(Report->MeterStrings[0]) - 1 && data[length])
            {
                string[length] = data[length];
                length++;
            }

            string[length] = ANSI_NULL;
        }

        offset += size;
    }

    Report->MeterCount = count;

    return count;
}

/**
 * Decodes a _PMM or DPTF power getter result: mW; all ones is unknown, above PH_HW_METER_MAX_MILLIWATTS implausible.
 */
_Success_(return == PhHwFanDecodeOk)
PH_HW_FAN_DECODE NTAPI PhHwDecodePowerMeterReading(
    _In_reads_bytes_(Length) const UCHAR* Buffer,
    _In_ ULONG Length,
    _Out_ PFLOAT Watts
    )
{
    ULONG64 value;

    if (!PhHwAcpiReadInteger(Buffer, Length, &value))
        return PhHwFanDecodeMalformed;
    if (value == ULONG_MAX || value == ULONG64_MAX)
        return PhHwFanDecodeUnknown;
    if (value > PH_HW_METER_MAX_MILLIWATTS)
        return PhHwFanDecodeImplausible;

    *Watts = (FLOAT)value / 1000.0f;
    return PhHwFanDecodeOk;
}
