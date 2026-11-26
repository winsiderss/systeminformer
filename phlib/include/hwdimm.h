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

#ifndef _PH_HWDIMM_H
#define _PH_HWDIMM_H

EXTERN_C_START

#define PH_HW_PMIC_RAILS 3 // VDD (SWA), VDDQ (SWB), VPP (SWC)
#define PH_HW_PMIC_SENSOR_CURRENT 0 // + rail
#define PH_HW_PMIC_SENSOR_POWER (PH_HW_PMIC_SENSOR_CURRENT + PH_HW_PMIC_RAILS) // + rail; + PH_HW_PMIC_RAILS is the module total
#define PH_HW_PMIC_SENSOR_TEMPERATURE (PH_HW_PMIC_SENSOR_POWER + PH_HW_PMIC_RAILS + 1)
#define PH_HW_PMIC_SENSORS (PH_HW_PMIC_SENSOR_TEMPERATURE + 1)
#define PH_HW_PMIC_REPORT_REGISTERS 36

// PMIC5100 registers read for the report, in this order; 10h-19h are write-1-to-clear, 37h-39h the vendor password.
PHLIBAPI extern CONST UCHAR PhHwPmicCommands[PH_HW_PMIC_REPORT_REGISTERS];

PHLIBAPI
FLOAT
NTAPI
PhHwTsodToCelsius(
    _In_ USHORT Value
    );

PHLIBAPI
FLOAT
NTAPI
PhHwSpd5118ToCelsius(
    _In_ USHORT Value
    );

PHLIBAPI
BOOLEAN
NTAPI
PhHwPmicDefaultRange(
    _In_reads_(PH_HW_PMIC_REPORT_REGISTERS) PCUCHAR Registers,
    _In_ UCHAR SetPoint
    );

PHLIBAPI
BOOLEAN
NTAPI
PhHwPmicHighVoltage(
    _In_reads_(PH_HW_PMIC_REPORT_REGISTERS) PCUCHAR Registers
    );

PHLIBAPI
VOID
NTAPI
PhHwDecodePmicTelemetry(
    _In_ UCHAR Swa,
    _In_ UCHAR Swb,
    _In_ UCHAR Swc,
    _In_ UCHAR PowerMode,
    _In_ UCHAR TelemetryMode,
    _In_ UCHAR Temperature,
    _Out_writes_(PH_HW_PMIC_SENSORS) PBOOLEAN Valid,
    _Out_writes_(PH_HW_PMIC_SENSORS) PFLOAT Values
    );

EXTERN_C_END

#endif
