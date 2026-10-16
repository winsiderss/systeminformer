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

#ifndef PH_HWSMU_H
#define PH_HWSMU_H

#include <hwprv.h>

#define PH_HW_SMU_MAX_READINGS 160
#define PH_HW_SMU_MAX_CORE_SLOTS 16
#define PH_HW_SMU_NO_INDEX ULONG_MAX
#define PH_HW_HSMP_MAX_COMMANDS 32 // per-command state is one bit of a ULONG

// HSMP GET commands.
#define PH_HW_HSMP_TEST 0x01
#define PH_HW_HSMP_GET_SOCKET_POWER 0x04
#define PH_HW_HSMP_GET_SOCKET_POWER_LIMIT 0x06
#define PH_HW_HSMP_GET_SOCKET_POWER_LIMIT_MAX 0x07
#define PH_HW_HSMP_GET_FCLK_MCLK 0x0F
#define PH_HW_HSMP_GET_CCLK_THROTTLE_LIMIT 0x10
#define PH_HW_HSMP_GET_C0_PERCENT 0x11
#define PH_HW_HSMP_GET_DDR_BANDWIDTH 0x14
#define PH_HW_HSMP_GET_TEMP_MONITOR 0x15
#define PH_HW_HSMP_GET_SOCKET_FREQ_LIMIT 0x19
#define PH_HW_HSMP_GET_RAILS_SVI 0x1B
#define PH_HW_HSMP_GET_SOCKET_FMAX_FMIN 0x1C

// HSMP 19h socket frequency limit sources.
#define PH_HW_HSMP_LIMIT_CHTC 0x0001
#define PH_HW_HSMP_LIMIT_PROCHOT 0x0002
#define PH_HW_HSMP_LIMIT_TDC 0x0004
#define PH_HW_HSMP_LIMIT_PPT 0x0008
#define PH_HW_HSMP_LIMIT_OPN_MAX 0x0010
#define PH_HW_HSMP_LIMIT_RELIABILITY 0x0020
#define PH_HW_HSMP_LIMIT_APML 0x0040
#define PH_HW_HSMP_LIMIT_HSMP 0x0080

// Named sensor IDs of the SMU kinds; an HSMP reading has PH_HW_SMU_ID_HSMP set.
#define PH_HW_SMU_ID_HSMP 0x1000
typedef enum _PH_HW_SMU_ID
{
    PhHwSmuIdSocket = 0x00,
    PhHwSmuIdPpt,
    PhHwSmuIdPptSlow,
    PhHwSmuIdStapm,
    PhHwSmuIdTdc,
    PhHwSmuIdEdc,
    PhHwSmuIdThermal,
    PhHwSmuIdSocketMax,
    PhHwSmuIdPptFast,
    PhHwSmuIdPptApu,
    PhHwSmuIdTdcSoc,
    PhHwSmuIdEdcSoc,
    PhHwSmuIdGfxThermal,
    PhHwSmuIdSocThermal,
    PhHwSmuIdSkin,
    PhHwSmuIdTctl,
    PhHwSmuIdCoreRail = 0x10,
    PhHwSmuIdSocRail,
    PhHwSmuIdMiscRail,
    PhHwSmuIdSviRails,
    PhHwSmuIdMemRail,
    PhHwSmuIdCoreDomain,
    PhHwSmuIdCoreRailPeak,
    PhHwSmuIdCoreRailAverage,
    PhHwSmuIdCoreRail1, // second core rail (VDD1)
    PhHwSmuIdSoc = 0x20,
    PhHwSmuIdIod,
    PhHwSmuIdGfx,
    PhHwSmuIdHotspot,
    PhHwSmuIdVddp = 0x30,
    PhHwSmuIdVddg,
    PhHwSmuIdVddgIod,
    PhHwSmuIdVddgCcd,
    PhHwSmuIdVddm,
    PhHwSmuIdVsoc,
    PhHwSmuIdFclk = 0x40,
    PhHwSmuIdUclk,
    PhHwSmuIdMclk,
    PhHwSmuIdThrottleLimit,
    PhHwSmuIdSocketFrequencyLimit,
    PhHwSmuIdFmax,
    PhHwSmuIdFmin,
    PhHwSmuIdPackageC6 = 0x50,
    PhHwSmuIdSocketC0,
    PhHwSmuIdDramBandwidth,
    PhHwSmuIdDramBandwidthMax,
    PhHwSmuIdDramBandwidthUsed,
    PhHwSmuIdCoreRailVrm = 0x60,
    PhHwSmuIdSocRailVrm,
    PhHwSmuIdMiscRailVrm,
    PhHwSmuIdCoreRail1Vrm,
    PhHwSmuIdL3 = 0x100, // + CCX
    PhHwSmuIdCore = 0x200 // + core
} PH_HW_SMU_ID;

// Sensor name is Name, then Index unless PH_HW_SMU_NO_INDEX, then Suffix.
typedef struct _PH_HW_SMU_READING
{
    PH_HW_NAMED_SENSOR_KIND Kind;
    ULONG Id; // unique within the kind and package; orders the sensors
    PCWSTR Name;
    ULONG Index;
    PCWSTR Suffix; // optional
    FLOAT Value;
} PH_HW_SMU_READING, *PPH_HW_SMU_READING;

typedef struct _PH_HW_SMU_DECODE
{
    ULONG Count;
    BOOLEAN MemoryClockValid; // DDR MEMCLK, half the data rate; not in Readings
    BOOLEAN CoresEnded; // a populated core slot outside the locked set ended the per-core values
    BOOLEAN FrequencyLimitSourcesValid;
    FLOAT MemoryClock; // MHz
    ULONG FrequencyLimitSources; // HSMP 19h [15:0], PH_HW_HSMP_LIMIT_*; not in Readings
    PH_HW_SMU_READING Readings[PH_HW_SMU_MAX_READINGS];
} PH_HW_SMU_DECODE, *PPH_HW_SMU_DECODE;

// Per-core slot state of a PM table session; zero-initialize at session start.
typedef struct _PH_HW_SMU_CORE_SLOTS
{
    ULONG Mask; // the populated slots, locked when their count first equals the core count
    BOOLEAN Locked;
    BOOLEAN Ended; // no per-core values for the rest of the session
} PH_HW_SMU_CORE_SLOTS, *PPH_HW_SMU_CORE_SLOTS;

_Success_(return)
BOOLEAN PhHwSmuDecodePmTable(
    _In_ ULONG Version,
    _In_reads_bytes_(Length) PCVOID Table,
    _In_ ULONG Length,
    _In_ ULONG CoreCount,
    _Inout_ PPH_HW_SMU_CORE_SLOTS CoreSlots,
    _Inout_ PPH_HW_SMU_DECODE Decode
    );

extern CONST UCHAR PhHwSmuHsmpCommands[];
extern CONST ULONG PhHwSmuHsmpCommandCount;

VOID PhHwSmuDecodeHsmp(
    _In_ ULONG Opcode,
    _In_reads_(2) const ULONG* Response,
    _Inout_ PPH_HW_SMU_DECODE Decode
    );

#endif
