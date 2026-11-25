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

#ifndef PH_SYSSCCPUHW_H
#define PH_SYSSCCPUHW_H

EXTERN_C_START

// The System Information CPU section's hardware sensor values: the window-free half (syssccpuhw.c).

#define PH_SIP_CPU_SENSOR_GRAPHS (PH_HW_CAP_PACKAGE_TEMP | PH_HW_CAP_RAPL_PACKAGE)
#define PH_SIP_CPU_POWER_GRAPH_MINIMUM 10.0f // watts

typedef struct _PH_SIP_CPU_SENSORS
{
    ULONG Valid; // PH_HW_CAP_* bits with at least one current value
    FLOAT Temperature; // Hottest package
    FLOAT CcdTemperature; // Hottest CCD
    FLOAT Voltage; // Highest logical processor
    FLOAT ActiveMhz; // Average over logical processors
    FLOAT Multiplier; // Average over logical processors
    FLOAT PackagePower; // Sum over packages
    FLOAT CorePower;
    FLOAT DramPower;
    ULONG ThrottleFlags;
} PH_SIP_CPU_SENSORS, *PPH_SIP_CPU_SENSORS;

ULONG PhSipCpuSelectSensorGraphs(
    _In_ ULONG Capabilities
    );

VOID PhSipCpuQueryHardwareSensors(
    _Out_ PPH_SIP_CPU_SENSORS Sensors
    );

_Ret_maybenull_
PPH_STRING PhSipCpuFormatSensorGraphText(
    _In_ ULONG Capability
    );

PPH_STRING PhSipCpuFormatSensorGraphLabelY(
    _In_ ULONG Capability,
    _In_ FLOAT Value
    );

PPH_STRING PhSipCpuFormatSensorGraphTooltipText(
    _In_ ULONG Capability,
    _In_ ULONG Index
    );

FLOAT PhSipCpuSensorGraphScale(
    _In_ ULONG Capability,
    _In_reads_(Count) const FLOAT* Data,
    _In_ ULONG Count
    );

EXTERN_C_END

#endif
