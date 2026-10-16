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

#include "devices.h"

#define MB_FAMILY_NUVOTON 1
#define MB_FAMILY_NUVOTON_EC 2
#define MB_FAMILY_ITE 3
#define MB_FAMILY_FINTEK 4
#define MB_FAMILY_SMSC 5

#define MB_DECODER_DEFAULT 0
#define MB_DECODER_W836 1
#define MB_DECODER_NCT610X 2

#define MB_CUSTOMER_MSI(Id) ((Id) == 0x200 || (Id) == 0x201 || (Id) == 0x207 || (Id) == 0x20D)

typedef struct _MB_NCT67_MAP
{
    const USHORT* InputRegisters;
    PCWSTR const* InputLabels;
    const USHORT* FixedRegisters;
    const USHORT* FixedSourceRegisters;
    const USHORT* MonitorRegisters;
    const USHORT* MonitorSourceRegisters;
    const USHORT* TsiRegisters;
    const USHORT* FanRegisters;
    ULONG FixedWordMask;
    ULONG MonitorByteMask;
} MB_NCT67_MAP, *PMB_NCT67_MAP;
typedef const MB_NCT67_MAP* PCMB_NCT67_MAP;

// A temperature register that holds one source, read without a selector.
typedef struct _MB_NCT67_DIRECT
{
    USHORT Register;
    UCHAR Source;
} MB_NCT67_DIRECT, *PMB_NCT67_DIRECT;
typedef const MB_NCT67_DIRECT* PCMB_NCT67_DIRECT;

typedef struct _MB_NCT67_VARIANT
{
    UCHAR InputCount;
    BOOLEAN Scale6798;
    UCHAR PwmCount;
    UCHAR MonitorCount;
    UCHAR FixedCount;
    UCHAR TsiCount;
    PCWSTR const* Labels;
    ULONG TemperatureMask;
    ULONG VirtualMask;
    UCHAR FanCount;
    USHORT FanMinimumRpm;
    UCHAR AlternateCount;
    PCMB_NCT67_MAP Map;
    PCMB_NCT67_DIRECT Directs;
    UCHAR DirectCount;
} MB_NCT67_VARIANT, *PMB_NCT67_VARIANT;
typedef const MB_NCT67_VARIANT* PCMB_NCT67_VARIANT;

typedef struct _MB_NCT610X_VARIANT
{
    UCHAR FanCount;
    UCHAR PwmCount;
    UCHAR TsiCount;
} MB_NCT610X_VARIANT, *PMB_NCT610X_VARIANT;
typedef const MB_NCT610X_VARIANT* PCMB_NCT610X_VARIANT;

#define MB_W836_SOURCE_SELECT 0x1
#define MB_W836_UHG 0x2
#define MB_W836_AUX_SHARED 0x4

typedef struct _MB_W836_VARIANT
{
    UCHAR InputCount;
    UCHAR FanCount;
    UCHAR PwmCount;
    UCHAR Flags;
} MB_W836_VARIANT, *PMB_W836_VARIANT;
typedef const MB_W836_VARIANT* PCMB_W836_VARIANT;

#define MB_ITE_SCALED 0x1
#define MB_ITE_AVCC3 0x2
#define MB_ITE_NEWER_PWM 0x4
#define MB_ITE_8BIT_FANS 0x8
#define MB_ITE_TEMP_CONFIG 0x10
#define MB_ITE_ALT_FAN6 0x20
#define MB_ITE_IN7_INTERNAL 0x40
#define MB_ITE_BANK_SELECT 0x80 // 06h bits 6:5 swap 10h-AFh; a non-zero bank drops the sample

typedef struct _MB_ITE_VARIANT
{
    USHORT LsbMicrovolts;
    UCHAR Flags;
    UCHAR TemperatureCount;
    UCHAR FanCount;
    UCHAR PwmCount;
} MB_ITE_VARIANT, *PMB_ITE_VARIANT;
typedef const MB_ITE_VARIANT* PCMB_ITE_VARIANT;

typedef struct _MB_FINTEK_VARIANT
{
    USHORT InputMask;
    UCHAR TemperatureStart;
    UCHAR TemperatureCount;
    UCHAR FanCount;
    BOOLEAN Temperature16;
} MB_FINTEK_VARIANT, *PMB_FINTEK_VARIANT;
typedef const MB_FINTEK_VARIANT* PCMB_FINTEK_VARIANT;

typedef struct _MB_SMSC_VARIANT
{
    const USHORT* Nominal;
    UCHAR InputCount;
} MB_SMSC_VARIANT, *PMB_SMSC_VARIANT;
typedef const MB_SMSC_VARIANT* PCMB_SMSC_VARIANT;

struct _MB_CHIP
{
    UCHAR Family;
    USHORT Id;
    USHORT Mask;
    PCWSTR Name;
    const VOID* Variant;
    UCHAR Decoder;
};
typedef struct _MB_CHIP MB_CHIP, *PMB_CHIP;
typedef const MB_CHIP* PCMB_CHIP;

typedef struct _MB_BOARD_VOLTAGE
{
    UCHAR Index;
    PCWSTR Name;
    FLOAT Ri;
    FLOAT Rf;
    FLOAT Vf;
} MB_BOARD_VOLTAGE, *PMB_BOARD_VOLTAGE;

typedef struct _MB_BOARD_LABEL
{
    UCHAR Index;
    PCWSTR Name;
} MB_BOARD_LABEL, *PMB_BOARD_LABEL;

struct _MB_BOARD
{
    PH_STRINGREF Manufacturer;
    PCPH_STRINGREF Products; // exact, or prefixes with MB_BOARD_PRODUCT_PREFIX; terminated by an entry with a NULL Buffer
    USHORT ChipId;
    USHORT ChipMask;
    const MB_BOARD_VOLTAGE* Voltages;
    ULONG VoltageCount;
    const MB_BOARD_LABEL* Temperatures;
    ULONG TemperatureCount;
    const MB_BOARD_LABEL* Fans;
    ULONG FanCount;
    const MB_BOARD_LABEL* Controls;
    ULONG ControlCount;
    ULONG Flags; // MB_BOARD_*
    PCMB_CHIP Chip; // optional: the chip the board's firmware maps, in place of the identified one
};
typedef struct _MB_BOARD MB_BOARD, *PMB_BOARD;

typedef const MB_BOARD* PCMB_BOARD;

#define MB_BOARD_NCT6687_DR 0x1 // NCT6687D-R firmware map
#define MB_BOARD_PRODUCT_PREFIX 0x2 // Products are prefixes

typedef struct _MB_DECODE_CONTEXT
{
    PMB_CHIP_STATE State;
    PCMB_REGISTERS Registers;
    PMB_SENSOR Sensors;
    ULONG Count;
    ULONG Capacity;
    BOOLEAN Rejected; // the snapshot is not a sample; Sensors is untouched
} MB_DECODE_CONTEXT, *PMB_DECODE_CONTEXT;

typedef struct _MB_REGISTER_LIST
{
    PUSHORT Registers;
    ULONG Count;
    ULONG Capacity;
    ULONG Present[MB_REGISTER_SPACE / 32];
} MB_REGISTER_LIST, *PMB_REGISTER_LIST;

static CONST USHORT Nct67InputRegisters[] =
{
    0x480, 0x481, 0x482, 0x483, 0x484, 0x485, 0x486, 0x487,
    0x488, 0x489, 0x48a, 0x48b, 0x48c, 0x48d, 0x48e, 0x48f,
    0x470, 0x471
};

static CONST PCWSTR Nct67InputLabels[] =
{
    L"CPUVCORE", L"VIN1", L"AVSB", L"3VCC", L"VIN0", L"VIN8", L"VIN4", L"3VSB",
    L"VBAT", L"VTT", L"VIN5", L"VIN6", L"VIN2", L"VIN3", L"VIN7", L"VIN9",
    L"VHIF", L"VIN10"
};

#define NCT67_SCALED_INPUTS 0x0000018C
#define NCT6798_SCALED_INPUTS 0x00010F8C
#define NCT67_VBAT_INPUT 8
#define NCT67_REG_VBAT 0x5D

static CONST USHORT Nct67FixedRegisters[] = { 0x27, 0x150, 0x670, 0x672, 0x674, 0x676, 0x678, 0x67a };
static CONST USHORT Nct67FixedSourceRegisters[] = { 0x621, 0x622, 0xc26, 0xc27, 0xc28, 0xc29, 0xc2a, 0xc2b };
static CONST USHORT Nct67MonitorRegisters[] = { 0x73, 0x75, 0x77, 0x79, 0x7b, 0x7d, 0x4a0 };
static CONST USHORT Nct67MonitorSourceRegisters[] = { 0x100, 0x200, 0x300, 0x800, 0x900, 0xa00, 0xb00 };
static CONST USHORT Nct67TsiRegisters[] = { 0x409, 0x40b, 0x40d, 0x40f, 0x411, 0x413, 0x415, 0x417 };
static CONST USHORT Nct67FanRegisters[] = { 0x4c0, 0x4c2, 0x4c4, 0x4c6, 0x4c8, 0x4ca, 0x4ce };
static CONST USHORT Nct67PwmRegisters[] = { 0x01, 0x03, 0x11, 0x13, 0x15, 0xa09, 0xb09 };

// Temperature of source n + 1, whether or not a register selects it.
static CONST USHORT Nct67AlternateRegisters[] = { 0x490, 0x491, 0x492, 0x493, 0x494, 0x495, 0x496 };

static CONST USHORT Nct6775InputRegisters[] = { 0x20, 0x21, 0x22, 0x23, 0x24, 0x25, 0x26, 0x550, 0x551, 0x552 };

// Inputs without a known pin assignment are numbered.
static CONST PCWSTR Nct6775InputLabels[] =
{
    L"CPUVCORE", L"Voltage 1", L"AVCC", L"3VCC", L"Voltage 4", L"Voltage 5", L"Voltage 6", L"3VSB", L"VBAT", L"Voltage 9"
};

static CONST USHORT Nct6775FixedRegisters[] = { 0x27, 0x150, 0x250, 0x62b, 0x62c, 0x62d };
static CONST USHORT Nct6775FixedSourceRegisters[] = { 0x621, 0x622, 0x623, 0x624, 0x625, 0x626 };
static CONST USHORT Nct6775MonitorRegisters[] = { 0x73, 0x75, 0x77 };
static CONST USHORT Nct6775TsiRegisters[] = { 0x669 };
static CONST USHORT Nct6775FanRegisters[] = { 0x656, 0x658, 0x65a, 0x65c, 0x65e };

static CONST MB_NCT67_MAP Nct6779Map =
{
    Nct67InputRegisters, Nct67InputLabels,
    Nct67FixedRegisters, Nct67FixedSourceRegisters,
    Nct67MonitorRegisters, Nct67MonitorSourceRegisters,
    Nct67TsiRegisters, Nct67FanRegisters,
    0x02, 0x40
};

static CONST MB_NCT67_MAP Nct6775Map =
{
    Nct6775InputRegisters, Nct6775InputLabels,
    Nct6775FixedRegisters, Nct6775FixedSourceRegisters,
    Nct6775MonitorRegisters, Nct67MonitorSourceRegisters,
    Nct6775TsiRegisters, Nct6775FanRegisters,
    0x06, 0x00
};

static CONST MB_NCT67_MAP Nct6776Map =
{
    Nct6775InputRegisters, Nct6775InputLabels,
    Nct6775FixedRegisters, Nct6775FixedSourceRegisters,
    Nct6775MonitorRegisters, Nct67MonitorSourceRegisters,
    Nct67TsiRegisters, Nct6775FanRegisters,
    0x06, 0x00
};

static CONST PCWSTR Nct67TemperatureLabelsA[32] =
{
    NULL, L"SYSTIN", L"CPUTIN", L"AUXTIN0", L"AUXTIN1", L"AUXTIN2", L"AUXTIN3", NULL,
    L"SMBUSMASTER 0", L"SMBUSMASTER 1", L"SMBUSMASTER 2", L"SMBUSMASTER 3",
    L"SMBUSMASTER 4", L"SMBUSMASTER 5", L"SMBUSMASTER 6", L"SMBUSMASTER 7",
    L"PECI Agent 0", L"PECI Agent 1", L"PCH_CHIP_CPU_MAX_TEMP", L"PCH_CHIP_TEMP",
    L"PCH_CPU_TEMP", L"PCH_MCH_TEMP", L"Agent0 Dimm0", L"Agent0 Dimm1",
    L"Agent1 Dimm0", L"Agent1 Dimm1", L"BYTE_TEMP0", L"BYTE_TEMP1",
    L"PECI Agent 0 Calibration", L"PECI Agent 1 Calibration", NULL, L"Virtual_TEMP"
};

static CONST PCWSTR Nct67TemperatureLabelsB[32] =
{
    NULL, L"SYSTIN", L"CPUTIN", L"AUXTIN0", L"AUXTIN1", L"AUXTIN2", L"AUXTIN3", L"AUXTIN4",
    L"SMBUSMASTER 0", L"SMBUSMASTER 1", L"Virtual_TEMP", L"Virtual_TEMP",
    NULL, L"AUXTIN5", NULL, NULL,
    L"PECI Agent 0", L"PECI Agent 1", L"PCH_CHIP_CPU_MAX_TEMP", L"PCH_CHIP_TEMP",
    L"PCH_CPU_TEMP", L"PCH_MCH_TEMP", L"Agent0 Dimm0", L"Agent0 Dimm1",
    L"Agent1 Dimm0", L"Agent1 Dimm1", L"BYTE_TEMP0", L"BYTE_TEMP1",
    L"PECI/TSI Agent 0 Calibration", L"PECI/TSI Agent 1 Calibration", NULL, L"Virtual_TEMP"
};

static CONST PCWSTR Nct6775TemperatureLabels[32] =
{
    NULL, L"SYSTIN", L"CPUTIN", L"AUXTIN", L"AMD SB-TSI", L"PECI Agent 0", L"PECI Agent 1", L"PECI Agent 2",
    L"PECI Agent 3", L"PECI Agent 4", L"PECI Agent 5", L"PECI Agent 6",
    L"PECI Agent 7", L"PCH_CHIP_CPU_MAX_TEMP", L"PCH_CHIP_TEMP", L"PCH_CPU_TEMP",
    L"PCH_MCH_TEMP", L"PCH_DIM0_TEMP", L"PCH_DIM1_TEMP", L"PCH_DIM2_TEMP",
    L"PCH_DIM3_TEMP"
};

static CONST PCWSTR Nct6776TemperatureLabels[32] =
{
    NULL, L"SYSTIN", L"CPUTIN", L"AUXTIN", L"SMBUSMASTER 0", L"SMBUSMASTER 1", L"SMBUSMASTER 2", L"SMBUSMASTER 3",
    L"SMBUSMASTER 4", L"SMBUSMASTER 5", L"SMBUSMASTER 6", L"SMBUSMASTER 7",
    L"PECI Agent 0", L"PECI Agent 1", L"PCH_CHIP_CPU_MAX_TEMP", L"PCH_CHIP_TEMP",
    L"PCH_CPU_TEMP", L"PCH_MCH_TEMP", L"PCH_DIM0_TEMP", L"PCH_DIM1_TEMP",
    L"PCH_DIM2_TEMP", L"PCH_DIM3_TEMP", L"BYTE_TEMP"
};

static CONST MB_NCT67_VARIANT Nct6775 = { 9, FALSE, 3, 3, 6, 1, Nct6775TemperatureLabels, 0x001ffffe, 0x00000000, 4, 1350000 / 0xffff, 0, &Nct6775Map };
static CONST MB_NCT67_VARIANT Nct6776 = { 9, FALSE, 3, 3, 6, 8, Nct6776TemperatureLabels, 0x007ffffe, 0x00000000, 5, 1350000 / 0x1fff, 0, &Nct6776Map };
static CONST MB_NCT67_VARIANT Nct6779 = { 15, FALSE, 5, 5, 2, 0, Nct67TemperatureLabelsA, 0x07ffff7e, 0x00000000, 7, 0, 0, &Nct6779Map };
static CONST MB_NCT67_VARIANT Nct6791 = { 15, FALSE, 6, 5, 2, 8, Nct67TemperatureLabelsA, 0x87ffff7e, 0x80000000, 7, 0, 0, &Nct6779Map };
static CONST MB_NCT67_VARIANT Nct6792 = { 15, FALSE, 6, 6, 2, 8, Nct67TemperatureLabelsA, 0x9fffff7e, 0x80000000, 7, 0, 0, &Nct6779Map };
static CONST MB_NCT67_VARIANT Nct6793 = { 15, FALSE, 6, 6, 2, 8, Nct67TemperatureLabelsA, 0xbfff037e, 0x80000000, 7, 0, 0, &Nct6779Map };
static CONST MB_NCT67_VARIANT Nct6795 = { 15, FALSE, 6, 6, 2, 2, Nct67TemperatureLabelsA, 0xbfffff7e, 0x80000000, 7, 0, 0, &Nct6779Map };
static CONST MB_NCT67_VARIANT Nct6796 = { 15, FALSE, 7, 6, 2, 2, Nct67TemperatureLabelsB, 0xbfff0ffe, 0x80000c00, 7, 0, 0, &Nct6779Map };
static CONST MB_NCT67_VARIANT Nct6797 = { 15, FALSE, 7, 6, 2, 2, Nct67TemperatureLabelsA, 0xbfffff7e, 0x80000000, 7, 0, 0, &Nct6779Map };

// NCT6798D, NCT6799D, NCT6701D: PCH, Agent DIMM, BYTE_TEMP and calibrated PECI/TSI sources by register.
static CONST MB_NCT67_DIRECT Nct6798Directs[] =
{
    { 0x400, 18 }, { 0x401, 19 }, { 0x402, 20 }, { 0x403, 21 },
    { 0x405, 22 }, { 0x406, 23 }, { 0x407, 24 }, { 0x408, 25 },
    { 0x419, 26 }, { 0x41a, 27 }, { 0x4f4, 28 }, { 0x4f5, 29 },
};

static CONST MB_NCT67_VARIANT Nct6798 = { 15, TRUE, 7, 7, 8, 2, Nct67TemperatureLabelsB, 0xbfff0ffe, 0x80000c00, 7, 0, 7, &Nct6779Map, Nct6798Directs, RTL_NUMBER_OF(Nct6798Directs) };
static CONST MB_NCT67_VARIANT Nct6799 = { 18, TRUE, 7, 7, 8, 2, Nct67TemperatureLabelsB, 0xbfff2ffe, 0x80000c00, 7, 0, 7, &Nct6779Map, Nct6798Directs, RTL_NUMBER_OF(Nct6798Directs) };
static CONST MB_NCT67_VARIANT Nct6701 = { 16, TRUE, 7, 7, 8, 2, Nct67TemperatureLabelsB, 0xbfff2ffe, 0x80000c00, 7, 0, 7, &Nct6779Map, Nct6798Directs, RTL_NUMBER_OF(Nct6798Directs) };

// NCT6796D-S (ASRock boards at the NCT6799D ID D802h): AUXTIN4 at 27h selected by C16h, AUXTIN5 at 449h,
// PECI_0 at 720h; inputs 0-15 only. AUXTIN5 is keyed 13 (Nct67TemperatureLabelsB).
static CONST USHORT Nct6796sFixedSourceRegisters[] = { 0xc16, 0x622, 0xc26, 0xc27, 0xc28, 0xc29, 0xc2a, 0xc2b };

static CONST MB_NCT67_MAP Nct6796sMap =
{
    Nct67InputRegisters, Nct67InputLabels,
    Nct67FixedRegisters, Nct6796sFixedSourceRegisters,
    Nct67MonitorRegisters, Nct67MonitorSourceRegisters,
    Nct67TsiRegisters, Nct67FanRegisters,
    0x02, 0x40
};

static CONST MB_NCT67_DIRECT Nct6796sDirects[] =
{
    { 0x449, 13 }, { 0x720, 16 },
};

static CONST MB_NCT67_VARIANT Nct6796s = { 16, TRUE, 7, 7, 8, 2, Nct67TemperatureLabelsB, 0xbfff2ffe, 0x80000c00, 7, 0, 7, &Nct6796sMap, Nct6796sDirects, RTL_NUMBER_OF(Nct6796sDirects) };

// NCT6102D/6104D/6106D and NCT6112D/6114D/6116D/6126D: in0-in8 at 00h-05h,
// 07h-09h, or at 300h-305h, 307h-309h; whichever bank reads.
static CONST USHORT Nct610xInputRegisters[] = { 0x00, 0x01, 0x02, 0x03, 0x04, 0x05, 0x07, 0x08, 0x09 };
static CONST USHORT Nct610xAltInputRegisters[] = { 0x300, 0x301, 0x302, 0x303, 0x304, 0x305, 0x307, 0x308, 0x309 };
static CONST USHORT Nct610xFixedHalfRegisters[] = { 0x16, 0x1b, 0x1b, 0x16, 0x1b, 0x1b }; // half degree of 10h-15h, bit n
static CONST USHORT Nct610xMonitorSourceRegisters[] = { 0x110, 0x120, 0x130 }; // monitors 18h-1Ah
static CONST USHORT Nct610xPwmRegisters[] = { 0x4a, 0x4b, 0x4c, 0xd8, 0xd9 };

#define NCT610X_REG_FIXED 0x10
#define NCT610X_REG_MONITOR 0x18
#define NCT610X_REG_SOURCE 0xb0
#define NCT610X_REG_TSI 0x59
#define NCT610X_REG_FAN_RPM 0x30
#define NCT610X_REG_VBAT 0x318
#define NCT610X_FIXED_COUNT 6
#define NCT610X_VBAT_INPUT 8
#define NCT610X_TEMPERATURE_MASK 0x007ffffe // sources 1-22 of Nct6776TemperatureLabels

static CONST MB_NCT610X_VARIANT Nct6106 = { 3, 3, 8 };
static CONST MB_NCT610X_VARIANT Nct6116 = { 5, 5, 2 };

// SMSC SCH311x and SCH5127: 8-bit inputs scaled reg * nominal / 192 mV.
static CONST USHORT SmscInputRegisters[] = { 0x20, 0x21, 0x22, 0x23, 0x24, 0x99, 0x9a, 0x1f };
static CONST USHORT Sch311xNominal[] = { 2500, 1500, 3300, 5000, 12000, 3300, 3300 };
static CONST USHORT Sch5127Nominal[] = { 2500, 2250, 3300, 1125, 1125, 3300, 3300, 1500 };
static CONST PCWSTR SmscInputLabels[] = { L"Voltage 0", L"Voltage 1", L"VCC", L"Voltage 3", L"Voltage 4", L"VTR", L"VBAT", L"Voltage 7" };
static CONST PCWSTR SmscTemperatureLabels[] = { L"Remote diode 1", L"Internal", L"Remote diode 2" };

#define SMSC_REG_CONFIG 0x40 // bit 0 monitoring started, bit 2 ready
#define SMSC_CONFIG_RUNNING 0x05
#define SMSC_REG_TEMP 0x25
#define SMSC_REG_FAN 0x28 // low byte first; the low byte read latches the high
#define SMSC_REG_PWM 0x30
#define SMSC_TEMPERATURES 3
#define SMSC_FANS 3
#define SMSC_PWMS 3

static CONST MB_SMSC_VARIANT Sch311x = { Sch311xNominal, RTL_NUMBER_OF(Sch311xNominal) };
static CONST MB_SMSC_VARIANT Sch5127 = { Sch5127Nominal, RTL_NUMBER_OF(Sch5127Nominal) };

static CONST MB_W836_VARIANT W83627ehf = { 10, 5, 4, 0 };
static CONST MB_W836_VARIANT W83627dhg = { 9, 5, 4, 0 };
static CONST MB_W836_VARIANT W83627uhg = { 9, 2, 2, MB_W836_UHG };
static CONST MB_W836_VARIANT W83667hg = { 9, 5, 3, MB_W836_AUX_SHARED };
static CONST MB_W836_VARIANT W83667hgb = { 9, 5, 3, MB_W836_SOURCE_SELECT | MB_W836_AUX_SHARED };

static CONST USHORT W836FixedRegisters[] = { 0x27, 0x150, 0x250, 0x7e };
static CONST USHORT W836FanRegisters[] = { 0x28, 0x29, 0x2a, 0x3f, 0x553 };
static CONST USHORT W836PwmRegisters[] = { 0x01, 0x03, 0x11, 0x61 };

// Percent of the 8 mV LSB per input; zero for an input the chip lacks.
static CONST USHORT W836UhgScales[] = { 100, 100, 416, 428, 100, 100, 0, 416, 425 };

static CONST PCWSTR W836UhgInputLabels[] =
{
    L"CPUVCORE", L"Voltage 1", L"Voltage 2", L"Voltage 3", L"Voltage 4", L"Voltage 5", NULL, L"Voltage 7", L"VBAT"
};

static CONST PCWSTR W836TemperatureLabels[] =
{
    L"SYSTIN", L"CPUTIN", L"AUXTIN", L"AMDTSI", L"PECI Agent 1", L"PECI Agent 2", L"PECI Agent 3", L"PECI Agent 4"
};

#define W836_REG_FANDIV1 0x47
#define W836_REG_SOURCE_SELECT1 0x49
#define W836_REG_SOURCE_SELECT2 0x4a
#define W836_REG_FANDIV2 0x4b
#define W836_REG_SMI_OVT 0x4c
#define W836_REG_DIODE 0x59
#define W836_REG_TEMP4_SOURCE 0x7d
#define W836_REG_TEMP3_CONFIG 0x252
#define W836_AUX_INPUT 6

#define NCT668X_MONITORS 32
#define NCT668X_FANS 16
#define NCT668X_PWMS 8
#define NCT668X_REG_MON(x) (0x100 + (x) * 2)
#define NCT668X_REG_FAN(x) (0x140 + (x) * 2)
#define NCT668X_REG_PWM(x) (0x160 + (x))
#define NCT668X_REG_MON_CFG(x) (0x1a0 + (x))
#define NCT668X_REG_FANIN_CFG(x) (0x1c0 + (x))
#define NCT668X_REG_FANOUT_CFG(x) (0x1d0 + (x))
#define NCT668X_VOLTAGE_START 0x60
#define NCT668X_REG_BUILD 0x604 // build date 604h-606h, version 608h-609h
#define NCT668X_BUILD_LENGTH 6
#define NCT668X_DR_REG_FAN_MODE 0x80f
#define NCT668X_DR_REG_ENGINE_STATUS 0xcf8
#define NCT668X_DR_PWMS 2 // CPU and pump duty stay at 160h-161h
#define NCT668X_DR_PWM_FIRST 10
#define NCT668X_DR_REG_PWM(x) (0xe00 + (x) - NCT668X_DR_PWM_FIRST) // x >= NCT668X_DR_PWM_FIRST
#define NCT668X_DR_PWM9 9
#define NCT668X_DR_REG_PWM9 0xc93

static CONST PCWSTR Nct668xMonitorLabels[] =
{
    NULL, L"Local", L"Diode 0 (curr)", L"Diode 1 (curr)", L"Diode 2 (curr)",
    L"Diode 0 (volt)", L"Diode 1 (volt)", L"Diode 2 (volt)",
    L"Thermistor 14", L"Thermistor 15", L"Thermistor 16", L"Thermistor 0",
    L"Thermistor 1", L"Thermistor 2", L"Thermistor 3", L"Thermistor 4",
    L"Thermistor 5", L"Thermistor 6", L"Thermistor 7", L"Thermistor 8",
    L"Thermistor 9", L"Thermistor 10", L"Thermistor 11", L"Thermistor 12",
    L"Thermistor 13", NULL, NULL, NULL, NULL, NULL, NULL, NULL,
    L"PECI 0.0", L"PECI 1.0", L"PECI 2.0", L"PECI 3.0",
    L"PECI 0.1", L"PECI 1.1", L"PECI 2.1", L"PECI 3.1",
    L"PECI DIMM 0", L"PECI DIMM 1", L"PECI DIMM 2", L"PECI DIMM 3",
    NULL, NULL, NULL, NULL,
    L"PCH CPU", L"PCH CHIP", L"PCH CHIP CPU MAX", L"PCH MCH",
    L"PCH DIMM 0", L"PCH DIMM 1", L"PCH DIMM 2", L"PCH DIMM 3",
    L"SMBus 0", L"SMBus 1", L"SMBus 2", L"SMBus 3", L"SMBus 4", L"SMBus 5",
    L"DIMM 0", L"DIMM 1", L"DIMM 2", L"DIMM 3",
    L"AMD TSI Addr 90h", L"AMD TSI Addr 92h", L"AMD TSI Addr 94h", L"AMD TSI Addr 96h",
    L"AMD TSI Addr 98h", L"AMD TSI Addr 9ah", L"AMD TSI Addr 9ch", L"AMD TSI Addr 9dh",
    NULL, NULL, NULL, NULL, NULL, NULL,
    L"Virtual 0", L"Virtual 1", L"Virtual 2", L"Virtual 3",
    L"Virtual 4", L"Virtual 5", L"Virtual 6", L"Virtual 7",
    NULL, NULL, NULL, NULL, NULL, NULL, NULL, NULL,
    L"VCC", L"VSB", L"AVSB", L"VTT", L"VBAT", L"VREF",
    L"VIN0", L"VIN1", L"VIN2", L"VIN3", L"VIN4", L"VIN5", L"VIN6", L"VIN7",
    L"VIN8", L"VIN9", L"VIN10", L"VIN11", L"VIN12", L"VIN13", L"VIN14", L"VIN15",
    L"VIN16"
};

static CONST PCWSTR Nct6687MsiTemperatureLabels[] =
{
    L"CPU", L"System", L"VRM MOS", L"PCH", L"CPU Socket", L"PCIe x1", L"M2_1"
};

static CONST MB_ITE_VARIANT It8705 = { 16000, MB_ITE_8BIT_FANS, 3, 3, 3 };
static CONST MB_ITE_VARIANT It8716 = { 16000, 0, 3, 5, 3 };
static CONST MB_ITE_VARIANT It8781 = { 16000, 0, 3, 3, 3 };
static CONST MB_ITE_VARIANT It8721 = { 12000, MB_ITE_SCALED | MB_ITE_IN7_INTERNAL | MB_ITE_NEWER_PWM, 3, 5, 3 };
static CONST MB_ITE_VARIANT It8771 = { 12000, MB_ITE_SCALED | MB_ITE_IN7_INTERNAL | MB_ITE_NEWER_PWM, 3, 3, 3 };
static CONST MB_ITE_VARIANT It8732 = { 10900, MB_ITE_SCALED | MB_ITE_IN7_INTERNAL | MB_ITE_NEWER_PWM, 3, 4, 4 };
static CONST MB_ITE_VARIANT It8792 = { 10900, MB_ITE_SCALED | MB_ITE_IN7_INTERNAL | MB_ITE_NEWER_PWM, 3, 3, 3 };
static CONST MB_ITE_VARIANT It8603 = { 12000, MB_ITE_SCALED | MB_ITE_IN7_INTERNAL | MB_ITE_AVCC3 | MB_ITE_NEWER_PWM, 3, 3, 3 };
static CONST MB_ITE_VARIANT It8613 = { 12000, MB_ITE_SCALED | MB_ITE_IN7_INTERNAL | MB_ITE_AVCC3 | MB_ITE_NEWER_PWM, 4, 5, 4 };
static CONST MB_ITE_VARIANT It8620 = { 12000, MB_ITE_SCALED | MB_ITE_IN7_INTERNAL | MB_ITE_NEWER_PWM | MB_ITE_TEMP_CONFIG, 6, 6, 6 };
static CONST MB_ITE_VARIANT It8622 = { 12000, MB_ITE_SCALED | MB_ITE_IN7_INTERNAL | MB_ITE_AVCC3 | MB_ITE_NEWER_PWM, 4, 5, 5 };
static CONST MB_ITE_VARIANT It8625 = { 11000, MB_ITE_SCALED | MB_ITE_IN7_INTERNAL | MB_ITE_AVCC3 | MB_ITE_NEWER_PWM | MB_ITE_ALT_FAN6 | MB_ITE_BANK_SELECT, 3, 6, 6 };
static CONST MB_ITE_VARIANT It8631 = { 12000, MB_ITE_SCALED | MB_ITE_IN7_INTERNAL | MB_ITE_NEWER_PWM, 2, 2, 2 };
static CONST MB_ITE_VARIANT It8655 = { 10900, MB_ITE_SCALED | MB_ITE_IN7_INTERNAL | MB_ITE_AVCC3 | MB_ITE_NEWER_PWM | MB_ITE_BANK_SELECT, 6, 3, 3 };
static CONST MB_ITE_VARIANT It8665 = { 10900, MB_ITE_SCALED | MB_ITE_IN7_INTERNAL | MB_ITE_AVCC3 | MB_ITE_NEWER_PWM | MB_ITE_ALT_FAN6 | MB_ITE_BANK_SELECT, 6, 6, 6 };
static CONST MB_ITE_VARIANT It8686 = { 12000, MB_ITE_SCALED | MB_ITE_IN7_INTERNAL | MB_ITE_AVCC3 | MB_ITE_NEWER_PWM | MB_ITE_BANK_SELECT, 6, 6, 5 };
static CONST MB_ITE_VARIANT It8689 = { 12000, MB_ITE_SCALED | MB_ITE_IN7_INTERNAL | MB_ITE_AVCC3 | MB_ITE_NEWER_PWM | MB_ITE_TEMP_CONFIG | MB_ITE_BANK_SELECT, 6, 6, 6 };
static CONST MB_ITE_VARIANT It8696 = { 12000, MB_ITE_SCALED | MB_ITE_IN7_INTERNAL | MB_ITE_AVCC3 | MB_ITE_NEWER_PWM | MB_ITE_BANK_SELECT, 6, 6, 6 };
static CONST MB_ITE_VARIANT It8736 = { 10900, MB_ITE_SCALED | MB_ITE_IN7_INTERNAL, 3, 4, 3 };
static CONST MB_ITE_VARIANT It8738 = { 10900, MB_ITE_SCALED | MB_ITE_IN7_INTERNAL | MB_ITE_AVCC3 | MB_ITE_NEWER_PWM, 3, 3, 3 };

static CONST USHORT IteFanLowRegisters[] = { 0x0d, 0x0e, 0x0f, 0x80, 0x82, 0x4c };
static CONST USHORT IteFanHighRegisters[] = { 0x18, 0x19, 0x1a, 0x81, 0x83, 0x4d };
static CONST USHORT ItePwmControlRegisters[] = { 0x15, 0x16, 0x17, 0x7f, 0xa7, 0xaf };
static CONST USHORT ItePwmDutyRegisters[] = { 0x63, 0x6b, 0x73, 0x7b, 0xa3, 0xab };
static CONST USHORT IteInputRegisters[] = { 0x20, 0x21, 0x22, 0x23, 0x24, 0x25, 0x26, 0x27, 0x28, 0x2f };
static CONST PCWSTR IteInputLabels[] =
{
    L"VIN0", L"VIN1", L"VIN2", L"VIN3", L"VIN4", L"VIN5", L"VIN6", L"3VSB", L"VBAT", L"AVCC3"
};

#define ITE_REG_BANK 0x06
#define ITE_REG_FAN_DIVISOR 0x0b
#define ITE_REG_FAN_16BIT 0x0c
#define ITE_REG_FAN_MAIN_CTRL 0x13
#define ITE_REG_TEMP(n) ((USHORT)(0x29 + (n)))
#define ITE_REG_TEMP456_ENABLE 0x77
#define ITE_REG_FAN6_ALT_LOW 0x93
#define ITE_REG_FAN6_ALT_HIGH 0x94

static CONST MB_FINTEK_VARIANT F71808e = { 0x1bf, 1, 2, 3, FALSE };
static CONST MB_FINTEK_VARIANT F71808a = { 0x18f, 1, 2, 2, FALSE };
static CONST MB_FINTEK_VARIANT F71858fg = { 0x007, 0, 3, 3, TRUE };
static CONST MB_FINTEK_VARIANT F71862fg = { 0x1ff, 1, 3, 3, FALSE };
static CONST MB_FINTEK_VARIANT F71868a = { 0x3ff, 1, 3, 3, FALSE };
static CONST MB_FINTEK_VARIANT F71882fg = { 0x1ff, 1, 3, 4, FALSE };
static CONST MB_FINTEK_VARIANT F8000 = { 0x007, 0, 3, 3, FALSE };
static CONST MB_FINTEK_VARIANT F81768d = { 0x7ff, 1, 3, 3, FALSE };
static CONST MB_FINTEK_VARIANT F81865f = { 0x07f, 1, 2, 2, FALSE };
static CONST MB_FINTEK_VARIANT F81866a = { 0x0ff, 0, 3, 3, FALSE };

#define FINTEK_REG_IN(n) (0x20 + (n))
#define FINTEK_REG_TEMP(n) (0x70 + 2 * (n))
#define FINTEK_REG_TEMP_CONFIG 0x69
#define FINTEK_REG_PWM_ENABLE 0x96
#define FINTEK_REG_FAN(n) (0xa0 + 16 * (n))
#define FINTEK_REG_FAN_TARGET(n) (0xa2 + 16 * (n))
#define FINTEK_REG_PWM(n) (0xa3 + 16 * (n))
#define FINTEK_REG_FAN_FULL_SPEED(n) (0xa4 + 16 * (n))

static CONST PCWSTR FintekInputLabels[] =
{
    L"VCC3V", L"VIN1", L"VIN2", L"VIN3", L"VIN4", L"VIN5", L"VIN6", L"VSB3V", L"VBAT", L"VIN9", L"VIN10"
};

#define FINTEK_SCALED_INPUTS 0x181

static CONST MB_CHIP SuperIoChips[] =
{
    { MB_FAMILY_NUVOTON, 0xc450, 0xfff8, L"NCT6102D/NCT6104D/NCT6106D", &Nct6106, MB_DECODER_NCT610X },
    { MB_FAMILY_NUVOTON, 0xd280, 0xfff8, L"NCT6112D/NCT6114D/NCT6116D/NCT6126D", &Nct6116, MB_DECODER_NCT610X },
    { MB_FAMILY_NUVOTON, 0xb470, 0xfff8, L"NCT6775F", &Nct6775 },
    { MB_FAMILY_NUVOTON, 0xc330, 0xfff8, L"NCT6776D/F", &Nct6776 },
    { MB_FAMILY_NUVOTON, 0xc560, 0xfff8, L"NCT6779D", &Nct6779 },
    { MB_FAMILY_NUVOTON, 0xc800, 0xfff8, L"NCT6791D", &Nct6791 },
    { MB_FAMILY_NUVOTON, 0xc910, 0xfff8, L"NCT6792D", &Nct6792 },
    { MB_FAMILY_NUVOTON, 0xd120, 0xfff8, L"NCT6793D", &Nct6793 },
    { MB_FAMILY_NUVOTON, 0xd350, 0xfff8, L"NCT6795D", &Nct6795 },
    { MB_FAMILY_NUVOTON, 0xd420, 0xfff8, L"NCT6796D", &Nct6796 },
    { MB_FAMILY_NUVOTON, 0xd450, 0xfff8, L"NCT6797D", &Nct6797 },
    { MB_FAMILY_NUVOTON, 0xd428, 0xfff8, L"NCT6798D", &Nct6798 },
    { MB_FAMILY_NUVOTON, 0xd806, 0xffff, L"NCT6701D", &Nct6701 }, // inside the D800 match; first hit wins
    { MB_FAMILY_NUVOTON, 0xd800, 0xfff8, L"NCT6799D", &Nct6799 },
    { MB_FAMILY_NUVOTON, 0x8850, 0xfff0, L"W83627EHF", &W83627ehf, MB_DECODER_W836 },
    { MB_FAMILY_NUVOTON, 0x8860, 0xfff0, L"W83627EHG", &W83627ehf, MB_DECODER_W836 },
    { MB_FAMILY_NUVOTON, 0xa020, 0xfff0, L"W83627DHG", &W83627dhg, MB_DECODER_W836 },
    { MB_FAMILY_NUVOTON, 0xb070, 0xfff0, L"W83627DHG-P", &W83627dhg, MB_DECODER_W836 },
    { MB_FAMILY_NUVOTON, 0xa230, 0xfff0, L"W83627UHG", &W83627uhg, MB_DECODER_W836 },
    { MB_FAMILY_NUVOTON, 0xa510, 0xfff0, L"W83667HG", &W83667hg, MB_DECODER_W836 },
    { MB_FAMILY_NUVOTON, 0xb350, 0xfff0, L"W83667HG-B", &W83667hgb, MB_DECODER_W836 },
    { MB_FAMILY_NUVOTON_EC, 0xc730, 0xfff0, L"NCT6683D", NULL },
    { MB_FAMILY_NUVOTON_EC, 0xd440, 0xfff0, L"NCT6686D", NULL },
    { MB_FAMILY_NUVOTON_EC, 0xd590, 0xfff0, L"NCT6687D", NULL },
    { MB_FAMILY_ITE, 0x8705, 0xffff, L"IT8705F", &It8705 },
    { MB_FAMILY_ITE, 0x8712, 0xffff, L"IT8712F", &It8705 },
    { MB_FAMILY_ITE, 0x8716, 0xffff, L"IT8716F", &It8716 },
    { MB_FAMILY_ITE, 0x8718, 0xffff, L"IT8718F", &It8716 },
    { MB_FAMILY_ITE, 0x8720, 0xffff, L"IT8720F", &It8716 },
    { MB_FAMILY_ITE, 0x8726, 0xffff, L"IT8726F", &It8716 },
    { MB_FAMILY_ITE, 0x8721, 0xffff, L"IT8721F", &It8721 },
    { MB_FAMILY_ITE, 0x8728, 0xffff, L"IT8728F", &It8721 },
    { MB_FAMILY_ITE, 0x8732, 0xffff, L"IT8732F", &It8732 },
    { MB_FAMILY_ITE, 0x8771, 0xffff, L"IT8771E", &It8771 },
    { MB_FAMILY_ITE, 0x8772, 0xffff, L"IT8772E", &It8771 },
    { MB_FAMILY_ITE, 0x8781, 0xffff, L"IT8781F", &It8781 },
    { MB_FAMILY_ITE, 0x8782, 0xffff, L"IT8782F", &It8781 },
    { MB_FAMILY_ITE, 0x8783, 0xffff, L"IT8783E/F", &It8781 },
    { MB_FAMILY_ITE, 0x8786, 0xffff, L"IT8786E", &It8771 },
    { MB_FAMILY_ITE, 0x8790, 0xffff, L"IT8790E", &It8771 },
    { MB_FAMILY_ITE, 0x8785, 0xffff, L"IT8785E", &It8771 },
    { MB_FAMILY_ITE, 0x8736, 0xffff, L"IT8736F", &It8736 },
    { MB_FAMILY_ITE, 0x8738, 0xffff, L"IT8738E", &It8738 },
    { MB_FAMILY_ITE, 0x8733, 0xffff, L"IT8792E", &It8792 },
    { MB_FAMILY_ITE, 0x8695, 0xffff, L"IT87952E", &It8792 },
    { MB_FAMILY_ITE, 0x8603, 0xffff, L"IT8603E", &It8603 },
    { MB_FAMILY_ITE, 0x8623, 0xffff, L"IT8623E", &It8603 },
    { MB_FAMILY_ITE, 0x8606, 0xffff, L"IT8606E", &It8603 },
    { MB_FAMILY_ITE, 0x8607, 0xffff, L"IT8607E", &It8603 },
    { MB_FAMILY_ITE, 0x8613, 0xffff, L"IT8613E", &It8613 },
    { MB_FAMILY_ITE, 0x8620, 0xffff, L"IT8620E", &It8620 },
    { MB_FAMILY_ITE, 0x8628, 0xffff, L"IT8628E", &It8620 },
    { MB_FAMILY_ITE, 0x8622, 0xffff, L"IT8622E", &It8622 },
    { MB_FAMILY_ITE, 0x8625, 0xffff, L"IT8625E", &It8625 },
    { MB_FAMILY_ITE, 0x8631, 0xffff, L"IT8631E", &It8631 },
    { MB_FAMILY_ITE, 0x8638, 0xffff, L"IT8638E", &It8631 },
    { MB_FAMILY_ITE, 0x8655, 0xffff, L"IT8655E", &It8655 },
    { MB_FAMILY_ITE, 0x8665, 0xffff, L"IT8665E", &It8665 },
    { MB_FAMILY_ITE, 0x8686, 0xffff, L"IT8686E", &It8686 },
    { MB_FAMILY_ITE, 0x8688, 0xffff, L"IT8688E", &It8686 },
    { MB_FAMILY_ITE, 0x8689, 0xffff, L"IT8689E", &It8689 },
    { MB_FAMILY_ITE, 0x8696, 0xffff, L"IT8696E", &It8696 },
    { MB_FAMILY_ITE, 0x8698, 0xffff, L"IT8698E", &It8689 },
    { MB_FAMILY_SMSC, 0x007c, 0xffff, L"SCH3112", &Sch311x },
    { MB_FAMILY_SMSC, 0x007d, 0xffff, L"SCH3114", &Sch311x },
    { MB_FAMILY_SMSC, 0x007f, 0xffff, L"SCH3116", &Sch311x },
    { MB_FAMILY_SMSC, 0x0086, 0xffff, L"SCH5127", &Sch5127 },
    { MB_FAMILY_FINTEK, 0x0901, 0xffff, L"F71808E", &F71808e },
    { MB_FAMILY_FINTEK, 0x1001, 0xffff, L"F71808A", &F71808a },
    { MB_FAMILY_FINTEK, 0x0507, 0xffff, L"F71858FG", &F71858fg },
    { MB_FAMILY_FINTEK, 0x0903, 0xffff, L"F71858AD", &F71858fg },
    { MB_FAMILY_FINTEK, 0x0601, 0xffff, L"F71862FG", &F71862fg },
    { MB_FAMILY_FINTEK, 0x1106, 0xffff, L"F71868A", &F71868a },
    { MB_FAMILY_FINTEK, 0x0814, 0xffff, L"F71869", &F71862fg },
    { MB_FAMILY_FINTEK, 0x1007, 0xffff, L"F71869A", &F71862fg },
    { MB_FAMILY_FINTEK, 0x0541, 0xffff, L"F71882FG", &F71882fg },
    { MB_FAMILY_FINTEK, 0x0723, 0xffff, L"F71889FG", &F71862fg },
    { MB_FAMILY_FINTEK, 0x0909, 0xffff, L"F71889ED", &F71862fg },
    { MB_FAMILY_FINTEK, 0x1005, 0xffff, L"F71889A", &F71862fg },
    { MB_FAMILY_FINTEK, 0x0581, 0xffff, L"F8000", &F8000 },
    { MB_FAMILY_FINTEK, 0x1210, 0xffff, L"F81768D", &F81768d },
    { MB_FAMILY_FINTEK, 0x0704, 0xffff, L"F81865F", &F81865f },
    { MB_FAMILY_FINTEK, 0x1010, 0xffff, L"F81866A", &F81866a },
    { MB_FAMILY_FINTEK, 0x1806, 0xffff, L"F81968", &F81866a },
    { MB_FAMILY_FINTEK, 0x1502, 0xffff, L"F81804/F81966", &F81866a },
};

// Board-gated chip identities; never matched by ID.
static CONST MB_CHIP Nct6796sChip = { MB_FAMILY_NUVOTON, 0xd802, 0xffff, L"NCT6796D-S", &Nct6796s };

// ASRock boards whose D802h chip is an NCT6796D-S; labels stay generic until confirmed.
static CONST PH_STRINGREF AsrockNct6796sProducts[] =
{
    PH_STRINGREF_INIT(L"X870E Nova WiFi"),
    { 0, NULL }
};

static CONST PH_STRINGREF AsrockNct6796sPrefixProducts[] =
{
    PH_STRINGREF_INIT(L"B650M-HDV/M.2"),
    { 0, NULL }
};

static CONST PH_STRINGREF GigabyteB650AorusEliteProducts[] =
{
    PH_STRINGREF_INIT(L"B650 AORUS ELITE"),
    PH_STRINGREF_INIT(L"B650 AORUS ELITE AX"),
    PH_STRINGREF_INIT(L"B650 AORUS ELITE V2"),
    PH_STRINGREF_INIT(L"B650 AORUS ELITE AX V2"),
    PH_STRINGREF_INIT(L"B650 AORUS ELITE AX ICE"),
    PH_STRINGREF_INIT(L"B650E AORUS ELITE AX ICE"),
    PH_STRINGREF_INIT(L"B650 GAMING X AX"),
    PH_STRINGREF_INIT(L"B650M AORUS PRO"),
    PH_STRINGREF_INIT(L"B650M AORUS PRO AX"),
    { 0, NULL }
};

static CONST MB_BOARD_VOLTAGE GigabyteB650AorusEliteVoltages[] =
{
    { 0, L"Vcore", 0, 1, 0 },
    { 1, L"+3.3V", 29.4f, 45.3f, 0 },
    { 2, L"+12V", 10, 2, 0 },
    { 3, L"+5V", 15, 10, 0 },
    { 4, L"Vcore SoC", 0, 1, 0 },
    { 5, L"Vcore Misc", 0, 1, 0 },
    { 6, L"Dual DDR5 5V", 1.5f, 1, 0 },
    { 7, L"+3V Standby", 10, 10, 0 },
    { 8, L"CMOS Battery", 10, 10, 0 },
    { 9, L"AVCC3", 10, 10, 0 },
};

static CONST MB_BOARD_LABEL GigabyteB650AorusEliteTemperatures[] =
{
    { 0, L"System" }, { 1, L"PCH" }, { 2, L"CPU" }, { 3, L"PCIe x16" }, { 4, L"VRM MOS" }, { 5, L"VSoC MOS" },
};

static CONST MB_BOARD_LABEL GigabyteB650AorusEliteFans[] =
{
    { 0, L"CPU Fan" }, { 1, L"System Fan #1" }, { 2, L"System Fan #2" }, { 3, L"System Fan #3" },
    { 4, L"System Fan #4 / Pump" }, { 5, L"CPU Optional Fan" },
};

static CONST PH_STRINGREF AsusRogStrixX670eProducts[] =
{
    PH_STRINGREF_INIT(L"ROG STRIX X670E-A GAMING WIFI"),
    PH_STRINGREF_INIT(L"ROG STRIX X670E-E GAMING WIFI"),
    PH_STRINGREF_INIT(L"ROG STRIX X670E-F GAMING WIFI"),
    { 0, NULL }
};

static CONST MB_BOARD_VOLTAGE AsusRogStrixX670eVoltages[] =
{
    { 1, L"+5V", 4, 1, 0 },
    { 2, L"AVCC", 34, 34, 0 },
    { 3, L"+3.3V", 34, 34, 0 },
    { 4, L"+12V", 11, 1, 0 },
    { 7, L"+3V Standby", 34, 34, 0 },
    { 8, L"CMOS Battery", 34, 34, 0 },
};

static CONST MB_BOARD_LABEL AsusRogStrixX670eTemperatures[] =
{
    { 1, L"Motherboard" },
    { 2, L"VRM" },
};

static CONST PH_STRINGREF AsusPrimeB650PlusProducts[] =
{
    PH_STRINGREF_INIT(L"PRIME B650-PLUS"),
    { 0, NULL }
};

static CONST MB_BOARD_VOLTAGE AsusPrimeB650PlusVoltages[] =
{
    { 0, L"Vcore", 0, 1, 0 },
    { 1, L"+5V", 4, 1, 0 },
    { 2, L"AVSB", 34, 34, 0 },
    { 3, L"+3.3V", 34, 34, 0 },
    { 4, L"+12V", 11, 1, 0 },
    { 7, L"+3V Standby", 34, 34, 0 },
    { 8, L"CMOS Battery", 34, 34, 0 },
    { 10, L"CPU VDDIO Memory", 1, 1, 0 },
};

static CONST MB_BOARD_LABEL AsusPrimeB650PlusTemperatures[] =
{
    { 1, L"Motherboard" },
    { 3, L"CPU Package" },
};

static CONST MB_BOARD_LABEL AsusPrimeB650PlusFans[] =
{
    { 0, L"Chassis Fan #1" }, { 1, L"CPU Fan" }, { 2, L"Chassis Fan #2" }, { 3, L"Chassis Fan #3" },
    { 4, L"CPU Optional Fan" }, { 5, L"AIO Pump" },
};

static CONST MB_BOARD_LABEL AsusPrimeB650PlusControls[] =
{
    { 0, L"Chassis Fan #1" }, { 1, L"CPU Fan" }, { 2, L"Chassis Fan #2" }, { 3, L"Chassis Fan #3" }, { 5, L"AIO Pump" },
};

static CONST PH_STRINGREF MsiB550aProProducts[] =
{
    PH_STRINGREF_INIT(L"B550-A PRO (MS-7C56)"),
    PH_STRINGREF_INIT(L"PRO B550-VC (MS-7C56)"),
    PH_STRINGREF_INIT(L"MPG B550 GAMING PLUS (MS-7C56)"),
    { 0, NULL }
};

static CONST MB_BOARD_VOLTAGE MsiB550aProVoltages[] =
{
    { 16, L"+12V", 11, 1, 0 },
    { 17, L"+5V", 4, 1, 0 },
    { 18, L"CPU Northbridge/SoC", 0, 1, 0 },
    { 19, L"DIMM", 1, 1, 0 },
    { 20, L"Vcore", 0, 1, 0 },
    { 21, L"Chipset", 0, 1, 0 },
    { 22, L"CPU System Agent", 0, 1, 0 },
    { 24, L"+3.3V", 0, 1, 0 },
    { 29, L"+1.8V", 0, 1, 0 },
    { 31, L"CPU VDDP", 0, 1, 0 },
    { 27, L"+3V Standby", 0, 1, 0 },
    { 28, L"AVSB", 0, 1, 0 },
};

static CONST MB_BOARD_LABEL MsiB550aProTemperatures[] =
{
    { 0, L"CPU Core" }, { 1, L"System" }, { 2, L"VRM MOS" }, { 3, L"Chipset" }, { 4, L"CPU Socket" }, { 5, L"PCIe x1" },
};

static CONST MB_BOARD_LABEL MsiB550aProFans[] =
{
    { 0, L"CPU Fan" }, { 1, L"Pump Fan" }, { 2, L"System Fan #1" }, { 3, L"System Fan #2" },
    { 4, L"System Fan #3" }, { 5, L"System Fan #4" }, { 6, L"System Fan #5" }, { 7, L"System Fan #6" },
};

// MSI 800-series boards with the NCT6687D-R firmware map (fans and duties).
static CONST PH_STRINGREF MsiNct6687drProducts[] =
{
    PH_STRINGREF_INIT(L"PRO B840-P WIFI (MS-7E57)"),
    PH_STRINGREF_INIT(L"B840M GAMING PLUS WIFI6E (MS-7E77)"),
    PH_STRINGREF_INIT(L"B850 GAMING PLUS WIFI (MS-7E56)"),
    PH_STRINGREF_INIT(L"B850 GAMING PLUS WIFI6E (MS-7E80)"),
    PH_STRINGREF_INIT(L"B850M GAMING PLUS WIFI6E (MS-7E81)"),
    PH_STRINGREF_INIT(L"B850 GAMING PRO WIFI6E (MS-7E89)"),
    PH_STRINGREF_INIT(L"PRO B850-P WIFI (MS-7E56)"),
    PH_STRINGREF_INIT(L"PRO B850M-A WIFI (MS-7E66)"),
    PH_STRINGREF_INIT(L"PRO B850M-P WIFI (MS-7E71)"),
    PH_STRINGREF_INIT(L"MAG B850M MORTAR WIFI (MS-7E61)"),
    PH_STRINGREF_INIT(L"MAG B850 TOMAHAWK MAX WIFI (MS-7E62)"),
    PH_STRINGREF_INIT(L"MPG B850 EDGE TI WIFI (MS-7E62)"),
    PH_STRINGREF_INIT(L"MPG B850I EDGE TI WIFI (MS-7E79)"),
    PH_STRINGREF_INIT(L"B850MPOWER (MS-7E83)"),
    PH_STRINGREF_INIT(L"PRO B850-S WIFI6E (MS-7E80)"),
    PH_STRINGREF_INIT(L"MAG B860M MORTAR WIFI (MS-7E40)"),
    PH_STRINGREF_INIT(L"X870 GAMING PLUS WIFI (MS-7E47)"),
    PH_STRINGREF_INIT(L"X870E GAMING PLUS WIFI (MS-7E70)"),
    PH_STRINGREF_INIT(L"MAG X870 TOMAHAWK WIFI (MS-7E51)"),
    PH_STRINGREF_INIT(L"PRO X870-P WIFI (MS-7E47)"),
    PH_STRINGREF_INIT(L"PRO X870E-P WIFI (MS-7E70)"),
    PH_STRINGREF_INIT(L"MAG X870E TOMAHAWK WIFI (MS-7E59)"),
    PH_STRINGREF_INIT(L"MAG X870E TOMAHAWK MAX WIFI PZ (MS-7E84)"),
    PH_STRINGREF_INIT(L"MPG X870E CARBON WIFI (MS-7E49)"),
    PH_STRINGREF_INIT(L"MPG X870E EDGE TI WIFI (MS-7E59)"),
    PH_STRINGREF_INIT(L"MEG X870E GODLIKE (MS-7E48)"),
    PH_STRINGREF_INIT(L"MEG Z890 GODLIKE (MS-7E21)"),
    PH_STRINGREF_INIT(L"MEG Z890 ACE (MS-7E22)"),
    PH_STRINGREF_INIT(L"MEG Z890M ACE (MS-7E23)"),
    PH_STRINGREF_INIT(L"MPG Z890 CARBON WIFI (MS-7E17)"),
    PH_STRINGREF_INIT(L"MPG Z890M CARBON WIFI (MS-7E18)"),
    PH_STRINGREF_INIT(L"MPG Z890 EDGE TI WIFI (MS-7E19)"),
    PH_STRINGREF_INIT(L"MPG Z890I EDGE TI WIFI (MS-7E33)"),
    PH_STRINGREF_INIT(L"Z890 GAMING PLUS WIFI (MS-7E34)"),
    PH_STRINGREF_INIT(L"MAG Z890 TOMAHAWK WIFI (MS-7E32)"),
    PH_STRINGREF_INIT(L"PRO Z890-A WIFI (MS-7E32)"),
    PH_STRINGREF_INIT(L"PRO Z890-P WIFI (MS-7E34)"),
    PH_STRINGREF_INIT(L"PRO Z890-S WIFI (MS-7E54)"),
    PH_STRINGREF_INIT(L"PRO Z890-S WIFI WHITE (MS-7E54)"),
    { 0, NULL }
};

// Fan and duty channel indices.
static CONST MB_BOARD_LABEL MsiNct6687drFans[] =
{
    { 0, L"CPU Fan" }, { 1, L"Pump Fan" }, { 2, L"Chipset Fan / Pump Fan #2" }, { 3, L"EZ-Conn Fan" },
    { 9, L"System Fan #7" }, { 15, L"System Fan #1" }, { 14, L"System Fan #2" }, { 13, L"System Fan #3" },
    { 12, L"System Fan #4" }, { 11, L"System Fan #5" }, { 10, L"System Fan #6" },
};

#define MB_BOARD_ENTRIES(Array) (Array), RTL_NUMBER_OF(Array)

static CONST MB_BOARD SuperIoBoards[] =
{
    {
        PH_STRINGREF_INIT(L"Gigabyte"), GigabyteB650AorusEliteProducts, 0x8689, 0xffff,
        MB_BOARD_ENTRIES(GigabyteB650AorusEliteVoltages),
        MB_BOARD_ENTRIES(GigabyteB650AorusEliteTemperatures),
        MB_BOARD_ENTRIES(GigabyteB650AorusEliteFans),
        MB_BOARD_ENTRIES(GigabyteB650AorusEliteFans),
    },
    {
        PH_STRINGREF_INIT(L"ASUSTeK"), AsusRogStrixX670eProducts, 0xd800, 0xfff8,
        MB_BOARD_ENTRIES(AsusRogStrixX670eVoltages),
        MB_BOARD_ENTRIES(AsusRogStrixX670eTemperatures),
        NULL, 0,
        NULL, 0,
    },
    {
        PH_STRINGREF_INIT(L"ASUSTeK"), AsusPrimeB650PlusProducts, 0xd800, 0xfff8,
        MB_BOARD_ENTRIES(AsusPrimeB650PlusVoltages),
        MB_BOARD_ENTRIES(AsusPrimeB650PlusTemperatures),
        MB_BOARD_ENTRIES(AsusPrimeB650PlusFans),
        MB_BOARD_ENTRIES(AsusPrimeB650PlusControls),
    },
    {
        PH_STRINGREF_INIT(L"Micro-Star International"), MsiB550aProProducts, 0xd590, 0xfff0,
        MB_BOARD_ENTRIES(MsiB550aProVoltages),
        MB_BOARD_ENTRIES(MsiB550aProTemperatures),
        MB_BOARD_ENTRIES(MsiB550aProFans),
        MB_BOARD_ENTRIES(MsiB550aProFans),
    },
    {
        PH_STRINGREF_INIT(L"Micro-Star International"), MsiNct6687drProducts, 0xd590, 0xfff0,
        NULL, 0,
        NULL, 0,
        MB_BOARD_ENTRIES(MsiNct6687drFans),
        MB_BOARD_ENTRIES(MsiNct6687drFans),
        MB_BOARD_NCT6687_DR,
    },
    {
        PH_STRINGREF_INIT(L"ASRock"), AsrockNct6796sProducts, 0xd802, 0xffff,
        NULL, 0,
        NULL, 0,
        NULL, 0,
        NULL, 0,
        0,
        &Nct6796sChip,
    },
    {
        PH_STRINGREF_INIT(L"ASRock"), AsrockNct6796sPrefixProducts, 0xd802, 0xffff,
        NULL, 0,
        NULL, 0,
        NULL, 0,
        NULL, 0,
        MB_BOARD_PRODUCT_PREFIX,
        &Nct6796sChip,
    },
    {
        PH_STRINGREF_INIT(L"MSI"), MsiB550aProProducts, 0xd590, 0xfff0,
        MB_BOARD_ENTRIES(MsiB550aProVoltages),
        MB_BOARD_ENTRIES(MsiB550aProTemperatures),
        MB_BOARD_ENTRIES(MsiB550aProFans),
        MB_BOARD_ENTRIES(MsiB550aProFans),
    },
};

static PCMB_BOARD SuperIoFindBoard(
    _In_ PCMB_CHIP Chip,
    _In_ USHORT ChipId,
    _In_opt_ PCPH_STRINGREF Manufacturer,
    _In_opt_ PCPH_STRINGREF Product
    )
{
    if (!Manufacturer || !Product)
        return NULL;

    for (ULONG i = 0; i < RTL_NUMBER_OF(SuperIoBoards); i++)
    {
        PCMB_BOARD board = &SuperIoBoards[i];

        if ((ChipId & board->ChipMask) != board->ChipId || (board->ChipId & Chip->Mask) != Chip->Id)
            continue;
        if (!PhStartsWithStringRef(Manufacturer, &board->Manufacturer, TRUE))
            continue;

        for (PCPH_STRINGREF product = board->Products; product->Buffer; product++)
        {
            BOOLEAN match;

            if (board->Flags & MB_BOARD_PRODUCT_PREFIX)
                match = PhStartsWithStringRef(Product, product, TRUE);
            else
                match = PhEqualStringRef(Product, product, TRUE);

            if (match)
                return board;
        }
    }

    return NULL;
}

/**
 * Matches an identified chip to its decoder and, by SMBIOS, its board map.
 *
 * \param Identity The chip the driver identified.
 * \param Manufacturer The SMBIOS baseboard manufacturer, if known.
 * \param Product The SMBIOS baseboard product, if known.
 * \param State Receives the chip state; untouched on failure.
 * \return TRUE when the chip has a decoder.
 */
_Success_(return)
BOOLEAN SuperIoIdentifyChip(
    _In_ const KPH_HW_SUPERIO_CHIP* Identity,
    _In_opt_ PCPH_STRINGREF Manufacturer,
    _In_opt_ PCPH_STRINGREF Product,
    _Out_ PMB_CHIP_STATE State
    )
{
    for (ULONG i = 0; i < RTL_NUMBER_OF(SuperIoChips); i++)
    {
        PCMB_CHIP chip = &SuperIoChips[i];

        if (chip->Family != Identity->VendorFamily || (Identity->ChipId & chip->Mask) != chip->Id)
            continue;

        memset(State, 0, sizeof(MB_CHIP_STATE));
        State->Chip = chip;
        State->Board = SuperIoFindBoard(chip, Identity->ChipId, Manufacturer, Product);
        State->CustomerId = Identity->CustomerId;

        if (State->Board && State->Board->Chip)
            State->Chip = State->Board->Chip;
        return TRUE;
    }

    return FALSE;
}

PCWSTR SuperIoGetChipName(
    _In_ PCMB_CHIP_STATE State
    )
{
    return State->Chip->Name;
}

//
// Register lists
//

static VOID SuperIoAddRegister(
    _Inout_ PMB_REGISTER_LIST List,
    _In_ USHORT Register
    )
{
    if (Register >= MB_REGISTER_SPACE || (List->Present[Register / 32] & (1ul << (Register % 32))))
        return;
    if (List->Count >= List->Capacity)
        return;

    List->Present[Register / 32] |= 1ul << (Register % 32);
    List->Registers[List->Count++] = Register;
}

static VOID SuperIoAddWord(
    _Inout_ PMB_REGISTER_LIST List,
    _In_ USHORT Register
    )
{
    SuperIoAddRegister(List, Register);
    SuperIoAddRegister(List, Register + 1);
}

static VOID SuperIoAddNct67Registers(
    _In_ PCMB_NCT67_VARIANT Variant,
    _Inout_ PMB_REGISTER_LIST List
    )
{
    PCMB_NCT67_MAP map = Variant->Map;

    for (ULONG i = 0; i < Variant->InputCount; i++)
        SuperIoAddRegister(List, map->InputRegisters[i]);
    SuperIoAddRegister(List, NCT67_REG_VBAT);

    for (ULONG i = 0; i < Variant->FixedCount; i++)
    {
        SuperIoAddRegister(List, map->FixedSourceRegisters[i]);

        if (map->FixedWordMask & (1ul << i))
            SuperIoAddWord(List, map->FixedRegisters[i]);
        else
            SuperIoAddRegister(List, map->FixedRegisters[i]);
    }

    for (ULONG i = 0; i < Variant->MonitorCount; i++)
    {
        SuperIoAddRegister(List, map->MonitorSourceRegisters[i]);

        if (map->MonitorByteMask & (1ul << i))
            SuperIoAddRegister(List, map->MonitorRegisters[i]);
        else
            SuperIoAddWord(List, map->MonitorRegisters[i]);
    }

    for (ULONG i = 0; i < Variant->AlternateCount; i++)
        SuperIoAddRegister(List, Nct67AlternateRegisters[i]);
    for (ULONG i = 0; i < Variant->DirectCount; i++)
        SuperIoAddRegister(List, Variant->Directs[i].Register);
    for (ULONG i = 0; i < Variant->TsiCount; i++)
        SuperIoAddWord(List, map->TsiRegisters[i]);
    for (ULONG i = 0; i < Variant->FanCount; i++)
        SuperIoAddWord(List, map->FanRegisters[i]);
    for (ULONG i = 0; i < Variant->PwmCount; i++)
        SuperIoAddRegister(List, Nct67PwmRegisters[i]);
}

static VOID SuperIoAddW836Registers(
    _In_ PCMB_W836_VARIANT Variant,
    _Inout_ PMB_REGISTER_LIST List
    )
{
    for (ULONG i = 0; i < Variant->InputCount; i++)
        SuperIoAddRegister(List, Nct6775InputRegisters[i]);
    SuperIoAddRegister(List, NCT67_REG_VBAT);

    SuperIoAddRegister(List, W836FixedRegisters[0]);
    SuperIoAddWord(List, W836FixedRegisters[1]);
    SuperIoAddWord(List, W836FixedRegisters[2]);

    if (Variant->Flags & MB_W836_AUX_SHARED)
        SuperIoAddRegister(List, W836_REG_TEMP3_CONFIG);

    if (Variant->Flags & (MB_W836_SOURCE_SELECT | MB_W836_UHG))
    {
        SuperIoAddRegister(List, W836_REG_SOURCE_SELECT1);
        SuperIoAddRegister(List, W836_REG_SOURCE_SELECT2);
    }

    if (Variant->Flags & MB_W836_SOURCE_SELECT)
    {
        SuperIoAddRegister(List, W836_REG_TEMP4_SOURCE);
        SuperIoAddRegister(List, W836FixedRegisters[3]);
    }

    SuperIoAddRegister(List, W836_REG_FANDIV1);
    SuperIoAddRegister(List, W836_REG_FANDIV2);
    SuperIoAddRegister(List, W836_REG_SMI_OVT);
    SuperIoAddRegister(List, W836_REG_DIODE);

    for (ULONG i = 0; i < Variant->FanCount; i++)
        SuperIoAddRegister(List, W836FanRegisters[i]);
    for (ULONG i = 0; i < Variant->PwmCount; i++)
        SuperIoAddRegister(List, W836PwmRegisters[i]);
}

static VOID SuperIoAddNct610xRegisters(
    _In_ PCMB_NCT610X_VARIANT Variant,
    _Inout_ PMB_REGISTER_LIST List
    )
{
    for (ULONG i = 0; i < RTL_NUMBER_OF(Nct610xInputRegisters); i++)
    {
        SuperIoAddRegister(List, Nct610xInputRegisters[i]);
        SuperIoAddRegister(List, Nct610xAltInputRegisters[i]);
    }
    SuperIoAddRegister(List, NCT610X_REG_VBAT);

    for (ULONG i = 0; i < NCT610X_FIXED_COUNT; i++)
    {
        SuperIoAddRegister(List, (USHORT)(NCT610X_REG_FIXED + i));
        SuperIoAddRegister(List, Nct610xFixedHalfRegisters[i]);
        SuperIoAddRegister(List, (USHORT)(NCT610X_REG_SOURCE + i));
    }

    for (ULONG i = 0; i < RTL_NUMBER_OF(Nct610xMonitorSourceRegisters); i++)
    {
        SuperIoAddRegister(List, (USHORT)(NCT610X_REG_MONITOR + i));
        SuperIoAddRegister(List, Nct610xMonitorSourceRegisters[i]);
    }

    for (ULONG i = 0; i < Variant->TsiCount; i++)
        SuperIoAddWord(List, (USHORT)(NCT610X_REG_TSI + i * 2));
    for (ULONG i = 0; i < Variant->FanCount; i++)
        SuperIoAddWord(List, (USHORT)(NCT610X_REG_FAN_RPM + i * 2));
    for (ULONG i = 0; i < Variant->PwmCount; i++)
        SuperIoAddRegister(List, Nct610xPwmRegisters[i]);
}

static VOID SuperIoAddNct668xRegisters(
    _In_opt_ PCMB_BOARD Board,
    _Inout_ PMB_REGISTER_LIST List
    )
{
    for (USHORT i = 0; i < NCT668X_MONITORS; i++)
    {
        SuperIoAddRegister(List, NCT668X_REG_MON_CFG(i));
        SuperIoAddWord(List, NCT668X_REG_MON(i));
    }

    for (USHORT i = 0; i < NCT668X_FANS; i++)
    {
        SuperIoAddRegister(List, NCT668X_REG_FANIN_CFG(i));
        SuperIoAddWord(List, NCT668X_REG_FAN(i));
    }

    for (USHORT i = 0; i < NCT668X_PWMS; i++)
    {
        SuperIoAddRegister(List, NCT668X_REG_FANOUT_CFG(i));
        SuperIoAddRegister(List, NCT668X_REG_PWM(i));
    }

    // EC firmware build date and version, for the report.
    for (USHORT i = 0; i < NCT668X_BUILD_LENGTH; i++)
        SuperIoAddRegister(List, (USHORT)(NCT668X_REG_BUILD + i));

    if (Board && (Board->Flags & MB_BOARD_NCT6687_DR))
    {
        SuperIoAddRegister(List, NCT668X_DR_REG_FAN_MODE);
        SuperIoAddRegister(List, NCT668X_DR_REG_ENGINE_STATUS);
        SuperIoAddRegister(List, NCT668X_DR_REG_PWM9);

        for (USHORT i = NCT668X_DR_PWM_FIRST; i < NCT668X_FANS; i++)
            SuperIoAddRegister(List, NCT668X_DR_REG_PWM(i));
    }
}

static VOID SuperIoAddIteRegisters(
    _In_ PCMB_ITE_VARIANT Variant,
    _Inout_ PMB_REGISTER_LIST List
    )
{
    if (Variant->LsbMicrovolts)
    {
        for (ULONG i = 0; i < ((Variant->Flags & MB_ITE_AVCC3) ? 10ul : 9ul); i++)
            SuperIoAddRegister(List, IteInputRegisters[i]);
    }

    for (USHORT i = 0; i < Variant->TemperatureCount; i++)
        SuperIoAddRegister(List, ITE_REG_TEMP(i));
    if (Variant->Flags & MB_ITE_TEMP_CONFIG)
        SuperIoAddRegister(List, ITE_REG_TEMP456_ENABLE);

    if (Variant->Flags & MB_ITE_BANK_SELECT)
        SuperIoAddRegister(List, ITE_REG_BANK);

    SuperIoAddRegister(List, ITE_REG_FAN_MAIN_CTRL);
    SuperIoAddRegister(List, ITE_REG_FAN_16BIT);
    SuperIoAddRegister(List, ITE_REG_FAN_DIVISOR);

    for (ULONG i = 0; i < Variant->FanCount; i++)
    {
        if (i == 5 && (Variant->Flags & MB_ITE_ALT_FAN6))
        {
            SuperIoAddRegister(List, ITE_REG_FAN6_ALT_LOW);
            SuperIoAddRegister(List, ITE_REG_FAN6_ALT_HIGH);
            continue;
        }

        SuperIoAddRegister(List, IteFanLowRegisters[i]);
        if (!(Variant->Flags & MB_ITE_8BIT_FANS))
            SuperIoAddRegister(List, IteFanHighRegisters[i]);
    }

    for (ULONG i = 0; i < Variant->PwmCount; i++)
    {
        if (Variant->Flags & MB_ITE_NEWER_PWM)
            SuperIoAddRegister(List, ItePwmDutyRegisters[i]);
        else
            SuperIoAddRegister(List, ItePwmControlRegisters[i]);
    }
}

static VOID SuperIoAddFintekRegisters(
    _In_ PCMB_FINTEK_VARIANT Variant,
    _Inout_ PMB_REGISTER_LIST List
    )
{
    for (USHORT i = 0; i < RTL_NUMBER_OF(FintekInputLabels); i++)
    {
        if (Variant->InputMask & (1u << i))
            SuperIoAddRegister(List, FINTEK_REG_IN(i));
    }

    for (USHORT i = Variant->TemperatureStart; i < Variant->TemperatureStart + Variant->TemperatureCount; i++)
    {
        if (Variant->Temperature16)
            SuperIoAddWord(List, FINTEK_REG_TEMP(i));
        else
            SuperIoAddRegister(List, FINTEK_REG_TEMP(i));
    }

    if (Variant->Temperature16)
        SuperIoAddRegister(List, FINTEK_REG_TEMP_CONFIG);

    SuperIoAddRegister(List, FINTEK_REG_PWM_ENABLE);

    for (USHORT i = 0; i < Variant->FanCount; i++)
    {
        SuperIoAddWord(List, FINTEK_REG_FAN(i));
        SuperIoAddWord(List, FINTEK_REG_FAN_TARGET(i));
        SuperIoAddRegister(List, FINTEK_REG_PWM(i));
        SuperIoAddWord(List, FINTEK_REG_FAN_FULL_SPEED(i));
    }
}

static VOID SuperIoAddSmscRegisters(
    _In_ PCMB_SMSC_VARIANT Variant,
    _Inout_ PMB_REGISTER_LIST List
    )
{
    SuperIoAddRegister(List, SMSC_REG_CONFIG);

    for (ULONG i = 0; i < Variant->InputCount; i++)
        SuperIoAddRegister(List, SmscInputRegisters[i]);
    for (USHORT i = 0; i < SMSC_TEMPERATURES; i++)
        SuperIoAddRegister(List, (USHORT)(SMSC_REG_TEMP + i));
    for (USHORT i = 0; i < SMSC_FANS; i++)
        SuperIoAddWord(List, (USHORT)(SMSC_REG_FAN + i * 2));
    for (USHORT i = 0; i < SMSC_PWMS; i++)
        SuperIoAddRegister(List, (USHORT)(SMSC_REG_PWM + i));
}

/**
 * Lists the hardware monitor registers the decoder of a chip reads.
 *
 * \param State The chip state from SuperIoIdentifyChip.
 * \param Registers Receives the registers, (bank or page << 8) | index.
 * \param Capacity The number of elements in Registers.
 * \return The number of registers.
 */
ULONG SuperIoGetRegisters(
    _In_ PCMB_CHIP_STATE State,
    _Out_writes_to_(Capacity, return) PUSHORT Registers,
    _In_ ULONG Capacity
    )
{
    MB_REGISTER_LIST list;

    memset(&list, 0, sizeof(MB_REGISTER_LIST));
    list.Registers = Registers;
    list.Capacity = Capacity;

    switch (State->Chip->Family)
    {
    case MB_FAMILY_NUVOTON:
        if (State->Chip->Decoder == MB_DECODER_W836)
            SuperIoAddW836Registers(State->Chip->Variant, &list);
        else if (State->Chip->Decoder == MB_DECODER_NCT610X)
            SuperIoAddNct610xRegisters(State->Chip->Variant, &list);
        else
            SuperIoAddNct67Registers(State->Chip->Variant, &list);
        break;
    case MB_FAMILY_NUVOTON_EC:
        SuperIoAddNct668xRegisters(State->Board, &list);
        break;
    case MB_FAMILY_ITE:
        SuperIoAddIteRegisters(State->Chip->Variant, &list);
        break;
    case MB_FAMILY_FINTEK:
        SuperIoAddFintekRegisters(State->Chip->Variant, &list);
        break;
    case MB_FAMILY_SMSC:
        SuperIoAddSmscRegisters(State->Chip->Variant, &list);
        break;
    }

    return list.Count;
}

//
// Decoding
//

_Success_(return)
static BOOLEAN SuperIoRead8(
    _In_ PCMB_REGISTERS Registers,
    _In_ USHORT Register,
    _Out_ PUCHAR Value
    )
{
    if (Register >= MB_REGISTER_SPACE || !(Registers->Valid[Register / 32] & (1ul << (Register % 32))))
        return FALSE;

    *Value = Registers->Value[Register];
    return TRUE;
}

// High byte at the register, low byte at the next; both from the same poll.
_Success_(return)
static BOOLEAN SuperIoRead16(
    _In_ PCMB_REGISTERS Registers,
    _In_ USHORT Register,
    _Out_ PUSHORT Value
    )
{
    UCHAR high;
    UCHAR low;

    if (!SuperIoRead8(Registers, Register, &high) || !SuperIoRead8(Registers, Register + 1, &low))
        return FALSE;
    if (Registers->Age[Register] != Registers->Age[Register + 1])
        return FALSE;

    *Value = (USHORT)((high << 8) | low);
    return TRUE;
}

// Low byte at the register, high byte at the next; both from the same poll.
_Success_(return)
static BOOLEAN SuperIoRead16Le(
    _In_ PCMB_REGISTERS Registers,
    _In_ USHORT Register,
    _Out_ PUSHORT Value
    )
{
    UCHAR low;
    UCHAR high;

    if (!SuperIoRead8(Registers, Register, &low) || !SuperIoRead8(Registers, (USHORT)(Register + 1), &high))
        return FALSE;
    if (Registers->Age[Register] != Registers->Age[Register + 1])
        return FALSE;

    *Value = (USHORT)((high << 8) | low);
    return TRUE;
}

static PMB_SENSOR SuperIoAddSensor(
    _Inout_ PMB_DECODE_CONTEXT Context,
    _In_ ULONG Type,
    _In_ ULONG SensorId,
    _In_ FLOAT Value
    )
{
    PMB_SENSOR sensor;

    if (Context->Count >= Context->Capacity)
        return NULL;

    sensor = &Context->Sensors[Context->Count++];
    sensor->Type = Type;
    sensor->SensorId = SensorId;
    sensor->Value = Value;
    sensor->Name[0] = UNICODE_NULL;

    return sensor;
}

static VOID SuperIoSetName(
    _Inout_ PMB_SENSOR Sensor,
    _In_ PCWSTR Name
    )
{
    PhCopyStringZ(Name, SIZE_MAX, Sensor->Name, RTL_NUMBER_OF(Sensor->Name), NULL);
}

static VOID SuperIoSetNumberedName(
    _Inout_ PMB_SENSOR Sensor,
    _In_ PCWSTR Prefix,
    _In_ ULONG Number
    )
{
    PH_FORMAT format[2];

    PhInitFormatS(&format[0], Prefix);
    PhInitFormatU(&format[1], Number);

    if (!PhFormatToBuffer(format, RTL_NUMBER_OF(format), Sensor->Name, sizeof(Sensor->Name), NULL))
        Sensor->Name[0] = UNICODE_NULL;
}

static PCWSTR SuperIoFindLabel(
    _In_reads_opt_(Count) const MB_BOARD_LABEL* Labels,
    _In_ ULONG Count,
    _In_ ULONG Index
    )
{
    for (ULONG i = 0; i < Count; i++)
    {
        if (Labels[i].Index == Index)
            return Labels[i].Name;
    }

    return NULL;
}

static const MB_BOARD_VOLTAGE* SuperIoFindVoltage(
    _In_ PCMB_BOARD Board,
    _In_ ULONG Index
    )
{
    for (ULONG i = 0; i < Board->VoltageCount; i++)
    {
        if (Board->Voltages[i].Index == Index)
            return &Board->Voltages[i];
    }

    return NULL;
}

/**
 * Reports a voltage input.
 *
 * \param Raw The chip reading in volts, before internal scaling.
 * \param Scaled TRUE if the chip halves the input internally.
 */
static VOID SuperIoAddVoltage(
    _Inout_ PMB_DECODE_CONTEXT Context,
    _In_ ULONG Index,
    _In_ FLOAT Raw,
    _In_ BOOLEAN Scaled,
    _In_ PCWSTR Label
    )
{
    PCMB_BOARD board = Context->State->Board;
    const MB_BOARD_VOLTAGE* entry = NULL;
    PMB_SENSOR sensor;
    FLOAT value;

    if (Index < 32)
    {
        // Inputs that never read above zero are not connected.
        if (Raw > 0)
            Context->State->SeenVoltages |= 1ul << Index;
        if (!(Context->State->SeenVoltages & (1ul << Index)))
            return;
    }

    if (board && board->VoltageCount)
    {
        if (!(entry = SuperIoFindVoltage(board, Index)))
            return;

        value = entry->Rf != 0 ? Raw + (Raw - entry->Vf) * entry->Ri / entry->Rf : Raw;
    }
    else
    {
        value = Scaled ? Raw * 2 : Raw;
    }

    if (sensor = SuperIoAddSensor(Context, PhHwSensorTypeVoltage, Index, value))
        SuperIoSetName(sensor, entry ? entry->Name : Label);
}

static VOID SuperIoAddTemperature(
    _Inout_ PMB_DECODE_CONTEXT Context,
    _In_ ULONG SensorId,
    _In_ ULONG Key,
    _In_ FLOAT Value,
    _In_ PCWSTR Label,
    _In_ ULONG Number
    )
{
    PCMB_BOARD board = Context->State->Board;
    PCWSTR name = NULL;
    PMB_SENSOR sensor;

    if (board)
        name = SuperIoFindLabel(board->Temperatures, board->TemperatureCount, Key);

    if (sensor = SuperIoAddSensor(Context, PhHwSensorTypeTemperature, SensorId, Value))
    {
        if (name)
            SuperIoSetName(sensor, name);
        else if (Number)
            SuperIoSetNumberedName(sensor, Label, Number);
        else
            SuperIoSetName(sensor, Label);
    }
}

static VOID SuperIoAddFan(
    _Inout_ PMB_DECODE_CONTEXT Context,
    _In_ ULONG Index,
    _In_ FLOAT Rpm,
    _In_ BOOLEAN Present
    )
{
    PCMB_BOARD board = Context->State->Board;
    PCWSTR name = NULL;
    PMB_SENSOR sensor;

    // Without a presence bit, a fan shows once it was seen spinning.
    if (Rpm > 0)
        Context->State->SeenFans |= 1ul << Index;
    if (!Present && !(Context->State->SeenFans & (1ul << Index)))
        return;

    if (board)
        name = SuperIoFindLabel(board->Fans, board->FanCount, Index);

    if (sensor = SuperIoAddSensor(Context, PhHwSensorTypeFan, Index, Rpm))
    {
        if (name)
            SuperIoSetName(sensor, name);
        else
            SuperIoSetNumberedName(sensor, L"Fan #", Index + 1);
    }
}

static VOID SuperIoAddControl(
    _Inout_ PMB_DECODE_CONTEXT Context,
    _In_ ULONG Index,
    _In_ FLOAT Percent
    )
{
    PCMB_BOARD board = Context->State->Board;
    PCWSTR name = NULL;
    PMB_SENSOR sensor;

    if (board)
        name = SuperIoFindLabel(board->Controls, board->ControlCount, Index);

    if (sensor = SuperIoAddSensor(Context, PhHwSensorTypeControl, Index, Percent))
    {
        if (name)
            SuperIoSetName(sensor, name);
        else
            SuperIoSetNumberedName(sensor, L"Fan #", Index + 1);
    }
}

// 9-bit temperature: signed degrees in the high byte, half degree in bit 7 of the low byte.
static FLOAT SuperIoTemperature9(
    _In_ USHORT Value
    )
{
    return (FLOAT)((SHORT)Value >> 7) * 0.5f;
}

static VOID SuperIoDecodeNct67(
    _Inout_ PMB_DECODE_CONTEXT Context
    )
{
    PCMB_NCT67_VARIANT variant = Context->State->Chip->Variant;
    PCMB_NCT67_MAP map = variant->Map;
    PCMB_REGISTERS registers = Context->Registers;
    PCWSTR const* labels = variant->Labels;
    ULONG scaled = variant->Scale6798 ? NCT6798_SCALED_INPUTS : NCT67_SCALED_INPUTS;
    ULONG mask = 0;
    UCHAR value8;
    USHORT value16;

    for (ULONG i = 0; i < variant->InputCount; i++)
    {
        if (!SuperIoRead8(registers, map->InputRegisters[i], &value8))
            continue;

        // VBAT reads only while firmware left its monitor enabled (5Dh bit 0).
        if (i == NCT67_VBAT_INPUT)
        {
            UCHAR vbat;

            if (!SuperIoRead8(registers, NCT67_REG_VBAT, &vbat) || !(vbat & 1))
                continue;
        }

        SuperIoAddVoltage(Context, i, value8 * 0.008f, !!(scaled & (1ul << i)), map->InputLabels[i]);
    }

    // Fixed registers first, then fan control monitors; a non-virtual source is shown once.
    for (ULONG i = 0; i < variant->FixedCount; i++)
    {
        UCHAR source;
        FLOAT temperature;

        if (!SuperIoRead8(registers, map->FixedSourceRegisters[i], &source))
            continue;

        source &= 0x1f;

        if (!source || (mask & (1ul << source)) || !(variant->TemperatureMask & (1ul << source)) || !labels[source])
            continue;

        if (map->FixedWordMask & (1ul << i))
        {
            if (!SuperIoRead16(registers, map->FixedRegisters[i], &value16))
                continue;
            temperature = SuperIoTemperature9(value16);
        }
        else
        {
            if (!SuperIoRead8(registers, map->FixedRegisters[i], &value8))
                continue;
            temperature = (SCHAR)value8;
        }

        mask |= 1ul << source;
        SuperIoAddTemperature(Context, source, source, temperature, labels[source], 0);
    }

    for (ULONG i = 0; i < variant->MonitorCount; i++)
    {
        UCHAR source;
        FLOAT temperature;
        BOOLEAN isVirtual;

        if (!SuperIoRead8(registers, map->MonitorSourceRegisters[i], &source))
            continue;

        source &= 0x1f;

        if (!source || !(variant->TemperatureMask & (1ul << source)) || !labels[source])
            continue;

        isVirtual = !!(variant->VirtualMask & (1ul << source));

        if (!isVirtual && (mask & (1ul << source)))
            continue;

        if (map->MonitorByteMask & (1ul << i))
        {
            if (!SuperIoRead8(registers, map->MonitorRegisters[i], &value8))
                continue;
            temperature = (SCHAR)value8;
        }
        else
        {
            if (!SuperIoRead16(registers, map->MonitorRegisters[i], &value16))
                continue;
            temperature = SuperIoTemperature9(value16);
        }

        if (isVirtual)
        {
            // Each fan's virtual temperature is its own sensor.
            SuperIoAddTemperature(Context, 0x100 + i, 0x100 + i, temperature, L"Virtual TEMP", i + 1);
        }
        else
        {
            mask |= 1ul << source;
            SuperIoAddTemperature(Context, source, source, temperature, labels[source], 0);
        }
    }

    // Sources no register selects; unconnected inputs read at or below zero.
    for (ULONG i = 0; i < variant->AlternateCount; i++)
    {
        ULONG source = i + 1;

        if ((mask & (1ul << source)) || !(variant->TemperatureMask & (1ul << source)) || !labels[source])
            continue;
        if (!SuperIoRead8(registers, Nct67AlternateRegisters[i], &value8) || (SCHAR)value8 <= 0 || (SCHAR)value8 > 125)
            continue;

        mask |= 1ul << source;
        SuperIoAddTemperature(Context, source, source, (SCHAR)value8, labels[source], 0);
    }

    // Sources with a register of their own; the same range check as the alternates.
    for (ULONG i = 0; i < variant->DirectCount; i++)
    {
        ULONG source = variant->Directs[i].Source;

        if ((mask & (1ul << source)) || !(variant->TemperatureMask & (1ul << source)) || !labels[source])
            continue;
        if (!SuperIoRead8(registers, variant->Directs[i].Register, &value8) || (SCHAR)value8 <= 0 || (SCHAR)value8 > 125)
            continue;

        mask |= 1ul << source;
        SuperIoAddTemperature(Context, source, source, (SCHAR)value8, labels[source], 0);
    }

    // AMD SB-TSI, 8.3 fixed point; zero when not connected.
    for (ULONG i = 0; i < variant->TsiCount; i++)
    {
        PMB_SENSOR sensor;

        if (!SuperIoRead16(registers, map->TsiRegisters[i], &value16) || !value16)
            continue;

        if (sensor = SuperIoAddSensor(Context, PhHwSensorTypeTemperature, 0x200 + i, (value16 >> 5) * 0.125f))
        {
            PH_FORMAT format[3];

            PhInitFormatS(&format[0], L"TSI");
            PhInitFormatU(&format[1], i);
            PhInitFormatS(&format[2], L"_TEMP");

            if (!PhFormatToBuffer(format, RTL_NUMBER_OF(format), sensor->Name, sizeof(sensor->Name), NULL))
                sensor->Name[0] = UNICODE_NULL;
        }
    }

    // At or below the minimum, the counter has run out and the fan is stopped.
    for (ULONG i = 0; i < variant->FanCount; i++)
    {
        if (SuperIoRead16(registers, map->FanRegisters[i], &value16))
            SuperIoAddFan(Context, i, value16 > variant->FanMinimumRpm ? (FLOAT)value16 : 0, FALSE);
    }

    for (ULONG i = 0; i < variant->PwmCount; i++)
    {
        if (SuperIoRead8(registers, Nct67PwmRegisters[i], &value8))
            SuperIoAddControl(Context, i, value8 * 100.0f / 255.0f);
    }
}

static VOID SuperIoDecodeNct610x(
    _Inout_ PMB_DECODE_CONTEXT Context
    )
{
    PCMB_NCT610X_VARIANT variant = Context->State->Chip->Variant;
    PCMB_REGISTERS registers = Context->Registers;
    const USHORT* inputs;
    ULONG mask = 0;
    UCHAR value8;
    USHORT value16;

    // The voltage bank is the first that reads non-zero, the 00h bank when both do in that poll; it is kept for the session.
    if (!Context->State->VoltageBank)
    {
        for (ULONG i = 0; i < RTL_NUMBER_OF(Nct610xInputRegisters) && Context->State->VoltageBank != 1; i++)
        {
            if (SuperIoRead8(registers, Nct610xInputRegisters[i], &value8) && value8)
                Context->State->VoltageBank = 1;
            else if (!Context->State->VoltageBank && SuperIoRead8(registers, Nct610xAltInputRegisters[i], &value8) && value8)
                Context->State->VoltageBank = 2;
        }
    }

    inputs = Context->State->VoltageBank == 2 ? Nct610xAltInputRegisters : Nct610xInputRegisters;

    for (ULONG i = 0; i < RTL_NUMBER_OF(Nct610xInputRegisters); i++)
    {
        if (!SuperIoRead8(registers, inputs[i], &value8))
            continue;

        // VBAT reads only while firmware left its monitor enabled (318h bit 0).
        if (i == NCT610X_VBAT_INPUT)
        {
            UCHAR vbat;

            if (!SuperIoRead8(registers, NCT610X_REG_VBAT, &vbat) || !(vbat & 1))
                continue;
        }

        SuperIoAddVoltage(Context, i, value8 * 0.008f, !!(NCT67_SCALED_INPUTS & (1ul << i)), Nct6775InputLabels[i]);
    }

    // Fixed registers 10h-15h with their B0h-B5h sources, then the fan control monitors; a source is shown once.
    for (ULONG i = 0; i < NCT610X_FIXED_COUNT; i++)
    {
        UCHAR source;
        UCHAR half;
        FLOAT temperature;

        if (!SuperIoRead8(registers, (USHORT)(NCT610X_REG_SOURCE + i), &source))
            continue;

        source &= 0x1f;

        if (!source || (mask & (1ul << source)) || !(NCT610X_TEMPERATURE_MASK & (1ul << source)) || !Nct6776TemperatureLabels[source])
            continue;
        if (!SuperIoRead8(registers, (USHORT)(NCT610X_REG_FIXED + i), &value8))
            continue;

        temperature = (SCHAR)value8;

        if (SuperIoRead8(registers, Nct610xFixedHalfRegisters[i], &half) && (half & (1u << i)))
            temperature += 0.5f;

        mask |= 1ul << source;
        SuperIoAddTemperature(Context, source, source, temperature, Nct6776TemperatureLabels[source], 0);
    }

    for (ULONG i = 0; i < RTL_NUMBER_OF(Nct610xMonitorSourceRegisters); i++)
    {
        UCHAR source;

        if (!SuperIoRead8(registers, Nct610xMonitorSourceRegisters[i], &source))
            continue;

        source &= 0x1f;

        if (!source || (mask & (1ul << source)) || !(NCT610X_TEMPERATURE_MASK & (1ul << source)) || !Nct6776TemperatureLabels[source])
            continue;
        if (!SuperIoRead8(registers, (USHORT)(NCT610X_REG_MONITOR + i), &value8))
            continue;

        mask |= 1ul << source;
        SuperIoAddTemperature(Context, source, source, (SCHAR)value8, Nct6776TemperatureLabels[source], 0);
    }

    // AMD SB-TSI, 8.3 fixed point; zero when not connected.
    for (ULONG i = 0; i < variant->TsiCount; i++)
    {
        PMB_SENSOR sensor;

        if (!SuperIoRead16(registers, (USHORT)(NCT610X_REG_TSI + i * 2), &value16) || !value16)
            continue;

        if (sensor = SuperIoAddSensor(Context, PhHwSensorTypeTemperature, 0x200 + i, (value16 >> 5) * 0.125f))
        {
            PH_FORMAT format[3];

            PhInitFormatS(&format[0], L"TSI");
            PhInitFormatU(&format[1], i);
            PhInitFormatS(&format[2], L"_TEMP");

            if (!PhFormatToBuffer(format, RTL_NUMBER_OF(format), sensor->Name, sizeof(sensor->Name), NULL))
                sensor->Name[0] = UNICODE_NULL;
        }
    }

    // 16-bit RPM, high byte first.
    for (ULONG i = 0; i < variant->FanCount; i++)
    {
        if (SuperIoRead16(registers, (USHORT)(NCT610X_REG_FAN_RPM + i * 2), &value16))
            SuperIoAddFan(Context, i, (FLOAT)value16, FALSE);
    }

    for (ULONG i = 0; i < variant->PwmCount; i++)
    {
        if (SuperIoRead8(registers, Nct610xPwmRegisters[i], &value8))
            SuperIoAddControl(Context, i, value8 * 100.0f / 255.0f);
    }
}

static VOID SuperIoDecodeW836(
    _Inout_ PMB_DECODE_CONTEXT Context
    )
{
    PCMB_W836_VARIANT variant = Context->State->Chip->Variant;
    PCMB_REGISTERS registers = Context->Registers;
    UCHAR sources[RTL_NUMBER_OF(W836FixedRegisters)] = { 0, 1, 2, 0 };
    UCHAR divisors[RTL_NUMBER_OF(W836FanRegisters)];
    ULONG present = 0x7;
    BOOLEAN skipAux = FALSE;
    BOOLEAN vbatValid;
    UCHAR select1 = 0;
    UCHAR select2 = 0;
    UCHAR vbat;
    UCHAR fanDiv1;
    UCHAR fanDiv2;
    UCHAR smiOvt;
    UCHAR diode;
    UCHAR value8;
    USHORT value16;

    if (variant->Flags & (MB_W836_SOURCE_SELECT | MB_W836_UHG))
    {
        if (!SuperIoRead8(registers, W836_REG_SOURCE_SELECT1, &select1) || !SuperIoRead8(registers, W836_REG_SOURCE_SELECT2, &select2))
            present = 0;
    }

    if (present && (variant->Flags & MB_W836_SOURCE_SELECT))
    {
        UCHAR select4;

        sources[0] = select2 >> 5;
        sources[1] = select1 & 7;
        sources[2] = (select1 >> 4) & 7;

        // The fourth register is shown only for a source the others do not report.
        if (SuperIoRead8(registers, W836_REG_TEMP4_SOURCE, &select4))
        {
            select4 &= 7;

            if (select4 != sources[0] && select4 != sources[1] && select4 != sources[2])
            {
                sources[3] = select4;
                present |= 0x8;
            }
        }
    }
    else if (present && (variant->Flags & MB_W836_UHG))
    {
        // Temperature 2 codes 0 and 2-5 select CPUTIN and PECI agents 1-4.
        select1 &= 7;

        if (select1 == 0)
            sources[1] = 1;
        else if (select1 >= 2 && select1 <= 5)
            sources[1] = select1 + 2;
        else
            present &= ~0x2ul;

        sources[2] = select2 >> 5;

        if (sources[2] == 2 || sources[2] == 3 || sources[2] == sources[0] || ((present & 0x2) && sources[2] == sources[1]))
            present &= ~0x4ul;
    }

    // The AUXTIN pin is VIN3 when 252h bit 0 is set, else a temperature.
    if (variant->Flags & MB_W836_AUX_SHARED)
    {
        if (sources[2] == 2 && SuperIoRead8(registers, W836_REG_TEMP3_CONFIG, &value8) && (value8 & 1))
            present &= ~0x4ul;

        // Temperatures 3 and 4 can take the AUXTIN pin.
        for (ULONG i = 2; i <= 3; i++)
        {
            if ((present & (1ul << i)) && sources[i] == 2)
                skipAux = TRUE;
        }
    }

    vbatValid = SuperIoRead8(registers, NCT67_REG_VBAT, &vbat);

    for (ULONG i = 0; i < variant->InputCount; i++)
    {
        FLOAT raw;

        if (i == W836_AUX_INPUT && skipAux)
            continue;
        // VBAT reads only while firmware left its monitor enabled (5Dh bit 0).
        if (i == NCT67_VBAT_INPUT && !(vbatValid && (vbat & 1)))
            continue;
        if (!SuperIoRead8(registers, Nct6775InputRegisters[i], &value8))
            continue;

        if (variant->Flags & MB_W836_UHG)
        {
            if (!W836UhgScales[i])
                continue;

            raw = value8 * 0.008f * W836UhgScales[i] / 100.0f;
            SuperIoAddVoltage(Context, i, raw, FALSE, W836UhgInputLabels[i]);
        }
        else
        {
            raw = value8 * 0.008f;
            SuperIoAddVoltage(Context, i, raw, !!(NCT67_SCALED_INPUTS & (1ul << i)), Nct6775InputLabels[i]);
        }
    }

    // 27h and 7Eh are whole degrees; 150h and 250h carry a half degree.
    for (ULONG i = 0; i < RTL_NUMBER_OF(W836FixedRegisters); i++)
    {
        FLOAT temperature;

        if (!(present & (1ul << i)))
            continue;

        if (i == 1 || i == 2)
        {
            if (!SuperIoRead16(registers, W836FixedRegisters[i], &value16))
                continue;
            temperature = SuperIoTemperature9(value16);
        }
        else
        {
            if (!SuperIoRead8(registers, W836FixedRegisters[i], &value8))
                continue;
            temperature = (SCHAR)value8;
        }

        SuperIoAddTemperature(Context, i, i, temperature, W836TemperatureLabels[sources[i]], 0);
    }

    // Divisor exponents as firmware left them: two low bits and a high bit per fan.
    if (
        vbatValid &&
        SuperIoRead8(registers, W836_REG_FANDIV1, &fanDiv1) &&
        SuperIoRead8(registers, W836_REG_FANDIV2, &fanDiv2) &&
        SuperIoRead8(registers, W836_REG_SMI_OVT, &smiOvt) &&
        SuperIoRead8(registers, W836_REG_DIODE, &diode)
        )
    {
        divisors[0] = ((fanDiv1 >> 4) & 3) | ((vbat >> 3) & 4);
        divisors[1] = ((fanDiv1 >> 6) & 3) | ((vbat >> 4) & 4);
        divisors[2] = ((fanDiv2 >> 6) & 3) | ((vbat >> 5) & 4);
        divisors[3] = (diode & 3) | ((smiOvt >> 5) & 4);
        divisors[4] = ((diode >> 2) & 3) | ((diode >> 5) & 4);

        for (ULONG i = 0; i < variant->FanCount; i++)
        {
            // 47h bit 2 set enables fan 4; bit 1 set takes the fan 5 pin.
            if ((i == 3 && !(fanDiv1 & 0x4)) || (i == 4 && (fanDiv1 & 0x2)))
                continue;
            if (!SuperIoRead8(registers, W836FanRegisters[i], &value8))
                continue;

            // 0 and FFh: no reading.
            SuperIoAddFan(Context, i, value8 && value8 != 0xff ? 1350000.0f / ((ULONG)value8 << divisors[i]) : 0, FALSE);
        }
    }

    for (ULONG i = 0; i < variant->PwmCount; i++)
    {
        if (SuperIoRead8(registers, W836PwmRegisters[i], &value8))
            SuperIoAddControl(Context, i, value8 * 100.0f / 255.0f);
    }
}

static VOID SuperIoDecodeNct668x(
    _Inout_ PMB_DECODE_CONTEXT Context
    )
{
    PCMB_REGISTERS registers = Context->Registers;
    PCMB_BOARD board = Context->State->Board;
    BOOLEAN dr;
    BOOLEAN msiLabels;
    UCHAR value8;
    USHORT value16;

    dr = board && (board->Flags & MB_BOARD_NCT6687_DR);
    msiLabels = !dr && Context->State->Chip->Id == 0xd590 && MB_CUSTOMER_MSI(Context->State->CustomerId);

    for (USHORT i = 0; i < NCT668X_MONITORS; i++)
    {
        UCHAR source;

        if (!SuperIoRead8(registers, NCT668X_REG_MON_CFG(i), &source))
            continue;

        source &= 0x7f;

        if (source >= RTL_NUMBER_OF(Nct668xMonitorLabels) || !Nct668xMonitorLabels[source])
            continue;
        if (!SuperIoRead16(registers, NCT668X_REG_MON(i), &value16))
            continue;

        if (source < NCT668X_VOLTAGE_START)
        {
            PCWSTR label = Nct668xMonitorLabels[source];

            if (msiLabels && i < RTL_NUMBER_OF(Nct6687MsiTemperatureLabels))
                label = Nct6687MsiTemperatureLabels[i];

            SuperIoAddTemperature(Context, i, i, SuperIoTemperature9(value16), label, 0);
        }
        else if (board && board->VoltageCount)
        {
            // Board dividers apply to the 12-bit reading.
            SuperIoAddVoltage(Context, i, 0.001f * (16 * (value16 >> 8) + ((value16 & 0xff) >> 4)), FALSE, Nct668xMonitorLabels[source]);
        }
        else
        {
            BOOLEAN scaled = source == 0x60 || source == 0x61 || source == 0x62 || source == 0x64; // VCC, VSB, AVSB, VBAT

            SuperIoAddVoltage(Context, i, (value16 >> 8) * 0.016f, scaled, Nct668xMonitorLabels[source]);
        }
    }

    for (USHORT i = 0; i < NCT668X_FANS; i++)
    {
        if (!SuperIoRead8(registers, NCT668X_REG_FANIN_CFG(i), &value8) || !(value8 & 0x80))
            continue;
        if (SuperIoRead16(registers, NCT668X_REG_FAN(i), &value16))
            SuperIoAddFan(Context, i, value16, TRUE);
    }

    for (USHORT i = 0; i < (dr ? NCT668X_DR_PWMS : NCT668X_PWMS); i++)
    {
        if (!SuperIoRead8(registers, NCT668X_REG_FANOUT_CFG(i), &value8) || !(value8 & 0x80))
            continue;
        if (SuperIoRead8(registers, NCT668X_REG_PWM(i), &value8))
            SuperIoAddControl(Context, i, value8 * 100.0f / 255.0f);
    }

    if (dr)
    {
        // The relocated duties have no FANOUT_CFG bit.
        if (SuperIoRead8(registers, NCT668X_DR_REG_PWM9, &value8))
            SuperIoAddControl(Context, NCT668X_DR_PWM9, value8 * 100.0f / 255.0f);

        for (USHORT i = NCT668X_DR_PWM_FIRST; i < NCT668X_FANS; i++)
        {
            if (SuperIoRead8(registers, NCT668X_DR_REG_PWM(i), &value8))
                SuperIoAddControl(Context, i, value8 * 100.0f / 255.0f);
        }
    }
}

static VOID SuperIoDecodeIte(
    _Inout_ PMB_DECODE_CONTEXT Context
    )
{
    PCMB_ITE_VARIANT variant = Context->State->Chip->Variant;
    PCMB_REGISTERS registers = Context->Registers;
    UCHAR fanMain = 0;
    UCHAR fan16 = 0;
    UCHAR divisors = 0;
    UCHAR temp456 = 0;
    UCHAR value8;

    // Firmware that leaves a non-zero bank swaps the registers read below; this sample is dropped.
    if ((variant->Flags & MB_ITE_BANK_SELECT) && SuperIoRead8(registers, ITE_REG_BANK, &value8) && ((value8 >> 5) & 3))
    {
        Context->Rejected = TRUE;
        return;
    }

    if (variant->LsbMicrovolts)
    {
        for (ULONG i = 0; i < ((variant->Flags & MB_ITE_AVCC3) ? 10ul : 9ul); i++)
        {
            if (!SuperIoRead8(registers, IteInputRegisters[i], &value8))
                continue;

            SuperIoAddVoltage(
                Context,
                i,
                value8 * variant->LsbMicrovolts / 1000000.0f,
                (variant->Flags & MB_ITE_SCALED) && (i >= 8 || (i == 7 && (variant->Flags & MB_ITE_IN7_INTERNAL))),
                IteInputLabels[i]
                );
        }
    }

    if (variant->Flags & MB_ITE_TEMP_CONFIG)
    {
        if (!SuperIoRead8(registers, ITE_REG_TEMP456_ENABLE, &temp456))
            temp456 = 0;
    }

    for (ULONG i = 0; i < variant->TemperatureCount; i++)
    {
        // Temperatures 4-6 share 2Ch-2Eh with in10-in12; 77h says which.
        if (i >= 3 && (variant->Flags & MB_ITE_TEMP_CONFIG) && ((temp456 >> (2 * (i - 3))) & 3) < 2)
            continue;
        if (!SuperIoRead8(registers, ITE_REG_TEMP(i), &value8))
            continue;
        // Absent sensors read 0 or 127.
        if ((SCHAR)value8 <= 0 || value8 == 0x7f)
            continue;

        SuperIoAddTemperature(Context, i, i, (SCHAR)value8, L"Temperature #", i + 1);
    }

    SuperIoRead8(registers, ITE_REG_FAN_MAIN_CTRL, &fanMain);
    SuperIoRead8(registers, ITE_REG_FAN_16BIT, &fan16);
    SuperIoRead8(registers, ITE_REG_FAN_DIVISOR, &divisors);

    for (ULONG i = 0; i < variant->FanCount; i++)
    {
        BOOLEAN present;
        ULONG count;
        UCHAR low;
        UCHAR high;
        FLOAT rpm;

        // Fans 1-3: 13h bits 4-6; fans 4/5: 0Ch bits 4/5; fan 6: 0Ch bit 2 or 0Bh bit 3.
        if (i < 3)
            present = !!(fanMain & (0x10 << i));
        else if (i < 5)
            present = !!(fan16 & (0x10 << (i - 3)));
        else if (variant->Flags & MB_ITE_ALT_FAN6)
            present = !!(divisors & 0x08);
        else
            present = !!(fan16 & 0x04);

        if (!present)
            continue;

        if (i == 5 && (variant->Flags & MB_ITE_ALT_FAN6))
        {
            if (!SuperIoRead8(registers, ITE_REG_FAN6_ALT_LOW, &low) || !SuperIoRead8(registers, ITE_REG_FAN6_ALT_HIGH, &high))
                continue;
        }
        else if (variant->Flags & MB_ITE_8BIT_FANS)
        {
            if (!SuperIoRead8(registers, IteFanLowRegisters[i], &low))
                continue;
            high = 0;
        }
        else
        {
            if (!SuperIoRead8(registers, IteFanLowRegisters[i], &low) || !SuperIoRead8(registers, IteFanHighRegisters[i], &high))
                continue;
        }

        count = low | ((ULONG)high << 8);

        if (variant->Flags & MB_ITE_8BIT_FANS)
        {
            // Fan 3 divides by 8 when 0Bh bit 6 is set, else 2.
            ULONG divisor = i < 2 ? 1ul << ((divisors >> (3 * i)) & 7) : ((divisors & 0x40) ? 8 : 2);

            rpm = count && count < 0xff ? 1350000.0f / (count * divisor) : 0;
        }
        else
        {
            rpm = count && count < 0xffff ? 1350000.0f / (count * 2) : 0;
        }

        SuperIoAddFan(Context, i, rpm, TRUE);
    }

    for (ULONG i = 0; i < variant->PwmCount; i++)
    {
        if (variant->Flags & MB_ITE_NEWER_PWM)
        {
            if (SuperIoRead8(registers, ItePwmDutyRegisters[i], &value8))
                SuperIoAddControl(Context, i, value8 * 100.0f / 255.0f);
        }
        else
        {
            // Bit 7 set: automatic mode, duty not readable.
            if (SuperIoRead8(registers, ItePwmControlRegisters[i], &value8) && !(value8 & 0x80))
                SuperIoAddControl(Context, i, (value8 & 0x7f) * 100.0f / 127.0f);
        }
    }
}

static FLOAT SuperIoFintekRpm(
    _In_ USHORT Count
    )
{
    // Counts above 1500000/366 are below the detectable speed (FAN_MIN_DETECT).
    return Count && Count < 1500000 / 366 ? 1500000.0f / Count : 0;
}

static VOID SuperIoDecodeFintek(
    _Inout_ PMB_DECODE_CONTEXT Context
    )
{
    PCMB_FINTEK_VARIANT variant = Context->State->Chip->Variant;
    PCMB_REGISTERS registers = Context->Registers;
    UCHAR pwmEnable;
    UCHAR value8;
    USHORT value16;

    for (USHORT i = 0; i < RTL_NUMBER_OF(FintekInputLabels); i++)
    {
        if (!(variant->InputMask & (1u << i)) || !SuperIoRead8(registers, FINTEK_REG_IN(i), &value8))
            continue;

        SuperIoAddVoltage(Context, i, value8 * 0.008f, !!(FINTEK_SCALED_INPUTS & (1u << i)), FintekInputLabels[i]);
    }

    for (USHORT i = 0; i < variant->TemperatureCount; i++)
    {
        USHORT nr = variant->TemperatureStart + i;
        FLOAT temperature;

        if (variant->Temperature16)
        {
            UCHAR config;
            BOOLEAN negative;
            ULONG magnitude;

            if (!SuperIoRead16(registers, FINTEK_REG_TEMP(nr), &value16) || !SuperIoRead8(registers, FINTEK_REG_TEMP_CONFIG, &config))
                continue;

            // F71858FG: 1/8 degree in bits 15:5, sign per TEMP_TABLE_SEL (69h bit 0).
            if (config & 1)
            {
                negative = !!(value16 & 0x0001);
                magnitude = (value16 >> 5) & 0x7ff;
            }
            else
            {
                negative = !!(value16 & 0x8000);
                magnitude = (value16 >> 5) & 0x3ff;
            }

            temperature = magnitude * 0.125f - (negative ? 128.0f : 0.0f);
        }
        else
        {
            if (!SuperIoRead8(registers, FINTEK_REG_TEMP(nr), &value8) || value8 == 0x7f || value8 == 0x80)
                continue;

            temperature = (SCHAR)value8;
        }

        SuperIoAddTemperature(Context, i, i, temperature, L"Temperature #", i + 1);
    }

    if (!SuperIoRead8(registers, FINTEK_REG_PWM_ENABLE, &pwmEnable))
        pwmEnable = 0;

    for (USHORT i = 0; i < variant->FanCount; i++)
    {
        USHORT target;
        USHORT fullSpeed;

        if (SuperIoRead16(registers, FINTEK_REG_FAN(i), &value16))
            SuperIoAddFan(Context, i, SuperIoFintekRpm(value16), FALSE);

        // Duty mode (96h bit 2n): the duty register; RPM mode: target over full speed.
        if (pwmEnable & (1u << (2 * i)))
        {
            if (SuperIoRead8(registers, FINTEK_REG_PWM(i), &value8))
                SuperIoAddControl(Context, i, value8 * 100.0f / 255.0f);
        }
        else if (
            SuperIoRead16(registers, FINTEK_REG_FAN_TARGET(i), &target) &&
            SuperIoRead16(registers, FINTEK_REG_FAN_FULL_SPEED(i), &fullSpeed) &&
            SuperIoFintekRpm(fullSpeed) > 0
            )
        {
            SuperIoAddControl(Context, i, min(SuperIoFintekRpm(target) * 100.0f / SuperIoFintekRpm(fullSpeed), 100.0f));
        }
    }
}

static VOID SuperIoDecodeSmsc(
    _Inout_ PMB_DECODE_CONTEXT Context
    )
{
    PCMB_SMSC_VARIANT variant = Context->State->Chip->Variant;
    PCMB_REGISTERS registers = Context->Registers;
    UCHAR value8;

    // Readings are valid only while firmware runs the monitor.
    if (!SuperIoRead8(registers, SMSC_REG_CONFIG, &value8) || (value8 & SMSC_CONFIG_RUNNING) != SMSC_CONFIG_RUNNING)
        return;

    for (ULONG i = 0; i < variant->InputCount; i++)
    {
        if (SuperIoRead8(registers, SmscInputRegisters[i], &value8))
            SuperIoAddVoltage(Context, i, value8 * variant->Nominal[i] / 192.0f / 1000.0f, FALSE, SmscInputLabels[i]);
    }

    // 80h is an open or shorted diode.
    for (USHORT i = 0; i < SMSC_TEMPERATURES; i++)
    {
        if (SuperIoRead8(registers, (USHORT)(SMSC_REG_TEMP + i), &value8) && value8 != 0x80)
            SuperIoAddTemperature(Context, i, i, (SCHAR)value8, SmscTemperatureLabels[i], 0);
    }

    // RPM = 5 400 000 / count; 0 and FFFFh are a stopped fan.
    for (USHORT i = 0; i < SMSC_FANS; i++)
    {
        USHORT count;

        if (!SuperIoRead16Le(registers, (USHORT)(SMSC_REG_FAN + i * 2), &count))
            continue;

        SuperIoAddFan(Context, i, count && count != 0xffff ? 5400000.0f / count : 0, FALSE);
    }

    for (USHORT i = 0; i < SMSC_PWMS; i++)
    {
        if (SuperIoRead8(registers, SMSC_REG_PWM + i, &value8))
            SuperIoAddControl(Context, i, value8 * 100.0f / 255.0f);
    }
}

/**
 * Decodes a register snapshot into sensors.
 *
 * \param State The chip state; keeps which inputs were seen.
 * \param Registers The register snapshot.
 * \param Sensors Receives the sensors; untouched when the sample is rejected.
 * \param Capacity The number of elements in Sensors.
 * \param Rejected Receives TRUE when the snapshot is not a sample (an ITE bank left selected).
 * \return The number of sensors, zero when rejected.
 */
ULONG SuperIoDecode(
    _Inout_ PMB_CHIP_STATE State,
    _In_ PCMB_REGISTERS Registers,
    _Out_writes_to_(Capacity, return) PMB_SENSOR Sensors,
    _In_ ULONG Capacity,
    _Out_ PBOOLEAN Rejected
    )
{
    MB_DECODE_CONTEXT context;

    context.State = State;
    context.Registers = Registers;
    context.Sensors = Sensors;
    context.Count = 0;
    context.Capacity = Capacity;
    context.Rejected = FALSE;

    switch (State->Chip->Family)
    {
    case MB_FAMILY_NUVOTON:
        if (State->Chip->Decoder == MB_DECODER_W836)
            SuperIoDecodeW836(&context);
        else if (State->Chip->Decoder == MB_DECODER_NCT610X)
            SuperIoDecodeNct610x(&context);
        else
            SuperIoDecodeNct67(&context);
        break;
    case MB_FAMILY_NUVOTON_EC:
        SuperIoDecodeNct668x(&context);
        break;
    case MB_FAMILY_ITE:
        SuperIoDecodeIte(&context);
        break;
    case MB_FAMILY_FINTEK:
        SuperIoDecodeFintek(&context);
        break;
    case MB_FAMILY_SMSC:
        SuperIoDecodeSmsc(&context);
        break;
    }

    *Rejected = context.Rejected;
    return context.Count;
}
