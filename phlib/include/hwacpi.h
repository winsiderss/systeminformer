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

#ifndef _PH_HWACPI_H
#define _PH_HWACPI_H

EXTERN_C_START

#define PH_HW_FAN_MAX_RPM 30000
#define PH_HW_FAN_MAX_STATES 16
#define PH_HW_DPTF_TRIPS 4
#define PH_HW_METER_PMC_INTEGERS 11
#define PH_HW_METER_PMC_ELEMENTS 14
#define PH_HW_METER_MAX_MILLIWATTS 10000000
#define PH_HW_ATK_OP_TSIF 0
#define PH_HW_ATK_OP_VSIF 1
#define PH_HW_ATK_OP_FSIF 2
#define PH_HW_ATK_OP_RTMP 3
#define PH_HW_ATK_OP_RVLT 4
#define PH_HW_ATK_OP_RFAN 5
#define PH_HW_DYTC_COMMANDS 4
#define PH_HW_POWER_STATICS 6
#define PH_HW_DPTF_MIN_DECIKELVIN 2000
#define PH_HW_DPTF_MAX_DECIKELVIN 4500
#define PH_HW_ACPI_OUTPUT_SIGNATURE 'BoeA'
#define PH_HW_ACPI_OUTPUT_HEADER 12
#define PH_HW_ACPI_ARGUMENT_HEADER 4
#define PH_HW_ACPI_ARGUMENT_INTEGER 0
#define PH_HW_ACPI_ARGUMENT_STRING 1
#define PH_HW_ACPI_ARGUMENT_BUFFER 2
#define PH_HW_ACPI_ARGUMENT_PACKAGE 3

typedef enum _PH_HW_FAN_DECODE
{
    PhHwFanDecodeOk,
    PhHwFanDecodeUnknown,
    PhHwFanDecodeImplausible,
    PhHwFanDecodeMalformed
} PH_HW_FAN_DECODE;

typedef enum _PH_HW_ACPI_KIND
{
    PhHwAcpiFan,
    PhHwAcpiDptfTemperature,
    PhHwAcpiPowerMeter,
    PhHwAcpiDptfPower,
    PhHwAcpiAtk,
    PhHwAcpiAtkReading,
    PhHwAcpiDytc,
    PhHwAcpiHotkeyTemperature
} PH_HW_ACPI_KIND;

typedef struct _PH_HW_FAN_STATE
{
    ULONG Values[5];
} PH_HW_FAN_STATE, *PPH_HW_FAN_STATE;

typedef struct _PH_HW_ACPI_REPORT
{
    CHAR Path[64];
    PH_HW_ACPI_KIND Kind;
    ULONG Type;
    ULONG Trips[PH_HW_DPTF_TRIPS];
    ULONG FifCount;
    ULONG Fif[4];
    ULONG StateCount;
    PH_HW_FAN_STATE States[PH_HW_FAN_MAX_STATES];
    ULONG MeterCount;
    ULONG Meter[PH_HW_METER_PMC_INTEGERS];
    CHAR MeterStrings[PH_HW_METER_PMC_ELEMENTS - PH_HW_METER_PMC_INTEGERS][64];
    ULONG AveragingMs;
    ULONG Power[PH_HW_POWER_STATICS];
    UCHAR Op;
    ULONG Arg;
    ULONG Dytc[PH_HW_DYTC_COMMANDS];
} PH_HW_ACPI_REPORT, *PPH_HW_ACPI_REPORT;

PHLIBAPI
_Success_(return)
BOOLEAN
NTAPI
PhHwAcpiReadArgument(
    _In_reads_bytes_(Length) const UCHAR* Buffer,
    _In_ ULONG Length,
    _In_ ULONG Offset,
    _Out_ PUSHORT Type,
    _Out_ PUSHORT DataLength,
    _Out_ PULONG Size
    );

PHLIBAPI
ULONG
NTAPI
PhHwAcpiReadIntegersAt(
    _In_reads_bytes_(End) const UCHAR* Buffer,
    _In_ ULONG Offset,
    _In_ ULONG End,
    _Out_writes_to_(Count, return) PULONG64 Values,
    _In_ ULONG Count
    );

PHLIBAPI
_Success_(return)
BOOLEAN
NTAPI
PhHwAcpiGetElements(
    _In_reads_bytes_(Length) const UCHAR* Buffer,
    _In_ ULONG Length,
    _Out_ PULONG Offset,
    _Out_ PULONG End,
    _Out_ PULONG Elements
    );

PHLIBAPI
ULONG
NTAPI
PhHwAcpiReadIntegers(
    _In_reads_bytes_(Length) const UCHAR* Buffer,
    _In_ ULONG Length,
    _Out_writes_to_(Count, return) PULONG64 Values,
    _In_ ULONG Count
    );

PHLIBAPI
_Success_(return == PhHwFanDecodeOk)
PH_HW_FAN_DECODE
NTAPI
PhHwDecodeFanStatus(
    _In_reads_bytes_(Length) const UCHAR* Buffer,
    _In_ ULONG Length,
    _Out_ PULONG Rpm,
    _Out_ PULONG64 Control
    );

PHLIBAPI
ULONG
NTAPI
PhHwDecodeFanStates(
    _In_reads_bytes_(Length) const UCHAR* Buffer,
    _In_ ULONG Length,
    _Out_writes_to_(Count, return) PPH_HW_FAN_STATE States,
    _In_ ULONG Count
    );

PHLIBAPI
_Success_(return)
BOOLEAN
NTAPI
PhHwAcpiFirstArgument(
    _In_reads_bytes_(Length) const UCHAR* Buffer,
    _In_ ULONG Length,
    _Out_ PUSHORT Type,
    _Out_ PUSHORT DataLength,
    _Out_ PULONG Offset
    );

PHLIBAPI
_Success_(return)
BOOLEAN
NTAPI
PhHwAcpiReadInteger(
    _In_reads_bytes_(Length) const UCHAR* Buffer,
    _In_ ULONG Length,
    _Out_ PULONG64 Value
    );

PHLIBAPI
_Ret_maybenull_
PPH_STRING
NTAPI
PhHwAcpiReadString(
    _In_reads_bytes_(Length) const UCHAR* Buffer,
    _In_ ULONG Length
    );

PHLIBAPI
_Success_(return == PhHwFanDecodeOk)
PH_HW_FAN_DECODE
NTAPI
PhHwDecodeDptfTemperature(
    _In_reads_bytes_(Length) const UCHAR* Buffer,
    _In_ ULONG Length,
    _Out_ PFLOAT Celsius
    );

PHLIBAPI
_Success_(return == PhHwFanDecodeOk)
PH_HW_FAN_DECODE
NTAPI
PhHwDecodeAtkReading(
    _In_ UCHAR Op,
    _In_reads_bytes_(Length) const UCHAR* Buffer,
    _In_ ULONG Length,
    _Out_ PFLOAT Value
    );

PHLIBAPI
ULONG
NTAPI
PhHwDecodePowerMeterCapabilities(
    _In_reads_bytes_(Length) const UCHAR* Buffer,
    _In_ ULONG Length,
    _Inout_ PPH_HW_ACPI_REPORT Report
    );

PHLIBAPI
_Success_(return == PhHwFanDecodeOk)
PH_HW_FAN_DECODE
NTAPI
PhHwDecodePowerMeterReading(
    _In_reads_bytes_(Length) const UCHAR* Buffer,
    _In_ ULONG Length,
    _Out_ PFLOAT Watts
    );

EXTERN_C_END

#endif
