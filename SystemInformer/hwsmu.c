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

#include <phapp.h>
#include <hwprv.h>
#include <hwsmu.h>
#include <kphuser.h>

// AMD SMU PM tables (undocumented float arrays keyed by exact version) and HSMP responses.

#define PHP_HW_SMU_NONE 0xFFFF

typedef enum _PHP_HW_SMU_RANGE
{
    PhpHwSmuRangeTemperature,
    PhpHwSmuRangeTemperatureLimit,
    PhpHwSmuRangePower,
    PhpHwSmuRangePowerLimit,
    PhpHwSmuRangeCurrent,
    PhpHwSmuRangeCurrentLimit,
    PhpHwSmuRangeRailVoltage,
    PhpHwSmuRangeCoreVoltage,
    PhpHwSmuRangeFabricClock,
    PhpHwSmuRangeCoreClock,
    PhpHwSmuRangePercent,
    PhpHwSmuRangeBandwidth,
    PhpHwSmuRangeMaximum
} PHP_HW_SMU_RANGE;

// Plausible values, inclusive; anything else is dropped.
static CONST struct { FLOAT Minimum; FLOAT Maximum; } PhpHwSmuRanges[PhpHwSmuRangeMaximum] =
{
    { 1.0f, 150.0f }, // degrees Celsius; a zero is an unpopulated CCD or core
    { 40.0f, 150.0f },
    { 0.0f, 2000.0f }, // watts
    { 1.0f, 10000.0f }, // PBO motherboard limits reach thousands
    { 0.0f, 2000.0f }, // amperes
    { 1.0f, 10000.0f },
    { 0.2f, 2.5f }, // volts
    { 0.0f, 2.5f }, // a sleeping core reads zero
    { 100.0f, 10000.0f }, // MHz
    { 0.0f, 10000.0f }, // a sleeping core reads zero
    { 0.0f, 100.0f },
    { 0.0f, 4096000.0f }, // MB/s
};

typedef struct _PHP_HW_SMU_FIELD
{
    USHORT Index; // float index in the table
    UCHAR Kind; // PH_HW_NAMED_SENSOR_KIND
    UCHAR Range; // PHP_HW_SMU_RANGE
    USHORT Id; // PH_HW_SMU_ID
    PCWSTR Name;
} PHP_HW_SMU_FIELD, *PPHP_HW_SMU_FIELD;
typedef CONST PHP_HW_SMU_FIELD *PCPHP_HW_SMU_FIELD;

typedef struct _PHP_HW_SMU_LAYOUT
{
    ULONG Version;
    PCPHP_HW_SMU_FIELD Fields;
    ULONG FieldCount;
    USHORT Mclk; // DDR MEMCLK when MclkIsDdr, else published as is
    BOOLEAN MclkIsDdr;
    UCHAR L3Count;
    USHORT L3Temperature;
    UCHAR CoreSlots;
    USHORT CorePower; // per-core arrays of CoreSlots floats
    USHORT CoreVoltage;
    USHORT CoreTemperature;
    USHORT CoreClock; // GHz
    USHORT CoreEffectiveClock; // GHz; presence only
    USHORT CoreC0; // percent; presence only
    USHORT CoreCc6; // percent
    USHORT CoreCurrent; // amperes
} PHP_HW_SMU_LAYOUT, *PPHP_HW_SMU_LAYOUT;
typedef CONST PHP_HW_SMU_LAYOUT *PCPHP_HW_SMU_LAYOUT;

#define PHP_SMU_FIELD(Index, Kind, Range, Id, Name) { (Index), (UCHAR)PhHwNamedSensor##Kind, (UCHAR)PhpHwSmuRange##Range, (USHORT)PhHwSmuId##Id, (Name) }

// Matisse 0x240903 (<= 8 cores)
static CONST PHP_HW_SMU_FIELD PhpHwSmuFields240903[] =
{
    PHP_SMU_FIELD(0, SmuPowerLimit, PowerLimit, Ppt, L"PPT limit"),
    PHP_SMU_FIELD(1, SmuPower, Power, Ppt, L"PPT"),
    PHP_SMU_FIELD(2, SmuCurrentLimit, CurrentLimit, Tdc, L"TDC limit"),
    PHP_SMU_FIELD(3, SmuCurrent, Current, Tdc, L"TDC"),
    PHP_SMU_FIELD(4, SmuTemperatureLimit, TemperatureLimit, Thermal, L"Thermal limit"),
    PHP_SMU_FIELD(8, SmuCurrentLimit, CurrentLimit, Edc, L"EDC limit"),
    PHP_SMU_FIELD(24, SmuPower, Power, CoreRail, L"Core rail"),
    PHP_SMU_FIELD(25, SmuPower, Power, SocRail, L"SoC rail"),
    PHP_SMU_FIELD(29, SmuPower, Power, Socket, L"Socket"),
    PHP_SMU_FIELD(40, SmuVoltage, RailVoltage, CoreRail, L"Core rail"),
    PHP_SMU_FIELD(41, SmuCurrent, Current, CoreRail, L"Core rail"),
    PHP_SMU_FIELD(45, SmuVoltage, RailVoltage, SocRail, L"SoC rail"),
    PHP_SMU_FIELD(46, SmuCurrent, Current, SocRail, L"SoC rail"),
    PHP_SMU_FIELD(48, SmuClock, FabricClock, Fclk, L"Fabric (FCLK)"),
    PHP_SMU_FIELD(50, SmuMemoryClock, FabricClock, Uclk, L"Controller (UCLK)"),
    PHP_SMU_FIELD(115, SmuTemperature, Temperature, Soc, L"SoC"),
    PHP_SMU_FIELD(125, SmuVoltage, RailVoltage, Vddp, L"VDDP"),
    PHP_SMU_FIELD(126, SmuVoltage, RailVoltage, Vddg, L"VDDG"),
    PHP_SMU_FIELD(135, SmuResidency, Percent, PackageC6, L"Package C6"),
};

// Matisse 0x240803 (16 cores)
static CONST PHP_HW_SMU_FIELD PhpHwSmuFields240803[] =
{
    PHP_SMU_FIELD(0, SmuPowerLimit, PowerLimit, Ppt, L"PPT limit"),
    PHP_SMU_FIELD(1, SmuPower, Power, Ppt, L"PPT"),
    PHP_SMU_FIELD(2, SmuCurrentLimit, CurrentLimit, Tdc, L"TDC limit"),
    PHP_SMU_FIELD(3, SmuCurrent, Current, Tdc, L"TDC"),
    PHP_SMU_FIELD(4, SmuTemperatureLimit, TemperatureLimit, Thermal, L"Thermal limit"),
    PHP_SMU_FIELD(8, SmuCurrentLimit, CurrentLimit, Edc, L"EDC limit"),
    PHP_SMU_FIELD(24, SmuPower, Power, CoreRail, L"Core rail"),
    PHP_SMU_FIELD(25, SmuPower, Power, SocRail, L"SoC rail"),
    PHP_SMU_FIELD(29, SmuPower, Power, Socket, L"Socket"),
    PHP_SMU_FIELD(41, SmuVoltage, RailVoltage, CoreRail, L"Core rail"),
    PHP_SMU_FIELD(42, SmuCurrent, Current, CoreRail, L"Core rail"),
    PHP_SMU_FIELD(45, SmuVoltage, RailVoltage, SocRail, L"SoC rail"),
    PHP_SMU_FIELD(46, SmuCurrent, Current, SocRail, L"SoC rail"),
    PHP_SMU_FIELD(48, SmuClock, FabricClock, Fclk, L"Fabric (FCLK)"),
    PHP_SMU_FIELD(50, SmuMemoryClock, FabricClock, Uclk, L"Controller (UCLK)"),
    PHP_SMU_FIELD(115, SmuTemperature, Temperature, Soc, L"SoC"),
    PHP_SMU_FIELD(125, SmuVoltage, RailVoltage, Vddp, L"VDDP"),
    PHP_SMU_FIELD(126, SmuVoltage, RailVoltage, Vddg, L"VDDG"),
    PHP_SMU_FIELD(135, SmuResidency, Percent, PackageC6, L"Package C6"),
};

// Vermeer 0x380804 (16 cores): core arrays, package C6 and L3 not decoded (sources disagree)
static CONST PHP_HW_SMU_FIELD PhpHwSmuFields380804[] =
{
    PHP_SMU_FIELD(0, SmuPowerLimit, PowerLimit, Ppt, L"PPT limit"),
    PHP_SMU_FIELD(1, SmuPower, Power, Ppt, L"PPT"),
    PHP_SMU_FIELD(2, SmuCurrentLimit, CurrentLimit, Tdc, L"TDC limit"),
    PHP_SMU_FIELD(3, SmuCurrent, Current, Tdc, L"TDC"),
    PHP_SMU_FIELD(4, SmuTemperatureLimit, TemperatureLimit, Thermal, L"Thermal limit"),
    PHP_SMU_FIELD(8, SmuCurrentLimit, CurrentLimit, Edc, L"EDC limit"),
    PHP_SMU_FIELD(24, SmuPower, Power, CoreRail, L"Core rail"),
    PHP_SMU_FIELD(25, SmuPower, Power, SocRail, L"SoC rail"),
    PHP_SMU_FIELD(29, SmuPower, Power, Socket, L"Socket"),
    PHP_SMU_FIELD(41, SmuVoltage, RailVoltage, CoreRail, L"Core rail"),
    PHP_SMU_FIELD(42, SmuCurrent, Current, CoreRail, L"Core rail"),
    PHP_SMU_FIELD(45, SmuVoltage, RailVoltage, SocRail, L"SoC rail"),
    PHP_SMU_FIELD(46, SmuCurrent, Current, SocRail, L"SoC rail"),
    PHP_SMU_FIELD(48, SmuClock, FabricClock, Fclk, L"Fabric (FCLK)"),
    PHP_SMU_FIELD(50, SmuMemoryClock, FabricClock, Uclk, L"Controller (UCLK)"),
    PHP_SMU_FIELD(127, SmuTemperature, Temperature, Soc, L"SoC"),
    PHP_SMU_FIELD(137, SmuVoltage, RailVoltage, Vddp, L"VDDP"),
    PHP_SMU_FIELD(138, SmuVoltage, RailVoltage, VddgIod, L"VDDG IOD"),
    PHP_SMU_FIELD(139, SmuVoltage, RailVoltage, VddgCcd, L"VDDG CCD"),
};

// Vermeer 0x380805 (16 cores)
static CONST PHP_HW_SMU_FIELD PhpHwSmuFields380805[] =
{
    PHP_SMU_FIELD(0, SmuPowerLimit, PowerLimit, Ppt, L"PPT limit"),
    PHP_SMU_FIELD(1, SmuPower, Power, Ppt, L"PPT"),
    PHP_SMU_FIELD(2, SmuCurrentLimit, CurrentLimit, Tdc, L"TDC limit"),
    PHP_SMU_FIELD(3, SmuCurrent, Current, Tdc, L"TDC"),
    PHP_SMU_FIELD(4, SmuTemperatureLimit, TemperatureLimit, Thermal, L"Thermal limit"),
    PHP_SMU_FIELD(8, SmuCurrentLimit, CurrentLimit, Edc, L"EDC limit"),
    PHP_SMU_FIELD(24, SmuPower, Power, CoreRail, L"Core rail"),
    PHP_SMU_FIELD(25, SmuPower, Power, SocRail, L"SoC rail"),
    PHP_SMU_FIELD(29, SmuPower, Power, Socket, L"Socket"),
    PHP_SMU_FIELD(40, SmuVoltage, RailVoltage, CoreRail, L"Core rail"),
    PHP_SMU_FIELD(42, SmuCurrent, Current, CoreRail, L"Core rail"),
    PHP_SMU_FIELD(45, SmuVoltage, RailVoltage, SocRail, L"SoC rail"),
    PHP_SMU_FIELD(46, SmuCurrent, Current, SocRail, L"SoC rail"),
    PHP_SMU_FIELD(48, SmuClock, FabricClock, Fclk, L"Fabric (FCLK)"),
    PHP_SMU_FIELD(50, SmuMemoryClock, FabricClock, Uclk, L"Controller (UCLK)"),
    PHP_SMU_FIELD(127, SmuTemperature, Temperature, Soc, L"SoC"),
    PHP_SMU_FIELD(137, SmuVoltage, RailVoltage, Vddp, L"VDDP"),
    PHP_SMU_FIELD(138, SmuVoltage, RailVoltage, VddgIod, L"VDDG IOD"),
    PHP_SMU_FIELD(139, SmuVoltage, RailVoltage, VddgCcd, L"VDDG CCD"),
    PHP_SMU_FIELD(155, SmuResidency, Percent, PackageC6, L"Package C6"),
};

// Vermeer 0x380904 (<= 8 cores)
static CONST PHP_HW_SMU_FIELD PhpHwSmuFields380904[] =
{
    PHP_SMU_FIELD(0, SmuPowerLimit, PowerLimit, Ppt, L"PPT limit"),
    PHP_SMU_FIELD(1, SmuPower, Power, Ppt, L"PPT"),
    PHP_SMU_FIELD(2, SmuCurrentLimit, CurrentLimit, Tdc, L"TDC limit"),
    PHP_SMU_FIELD(3, SmuCurrent, Current, Tdc, L"TDC"),
    PHP_SMU_FIELD(4, SmuTemperatureLimit, TemperatureLimit, Thermal, L"Thermal limit"),
    PHP_SMU_FIELD(8, SmuCurrentLimit, CurrentLimit, Edc, L"EDC limit"),
    PHP_SMU_FIELD(24, SmuPower, Power, CoreRail, L"Core rail"),
    PHP_SMU_FIELD(25, SmuPower, Power, SocRail, L"SoC rail"),
    PHP_SMU_FIELD(29, SmuPower, Power, Socket, L"Socket"),
    PHP_SMU_FIELD(41, SmuVoltage, RailVoltage, CoreRail, L"Core rail"),
    PHP_SMU_FIELD(42, SmuCurrent, Current, CoreRail, L"Core rail"),
    PHP_SMU_FIELD(45, SmuVoltage, RailVoltage, SocRail, L"SoC rail"),
    PHP_SMU_FIELD(46, SmuCurrent, Current, SocRail, L"SoC rail"),
    PHP_SMU_FIELD(48, SmuClock, FabricClock, Fclk, L"Fabric (FCLK)"),
    PHP_SMU_FIELD(50, SmuMemoryClock, FabricClock, Uclk, L"Controller (UCLK)"),
    PHP_SMU_FIELD(127, SmuTemperature, Temperature, Soc, L"SoC"),
    PHP_SMU_FIELD(137, SmuVoltage, RailVoltage, Vddp, L"VDDP"),
    PHP_SMU_FIELD(138, SmuVoltage, RailVoltage, VddgIod, L"VDDG IOD"),
    PHP_SMU_FIELD(139, SmuVoltage, RailVoltage, VddgCcd, L"VDDG CCD"),
    PHP_SMU_FIELD(152, SmuResidency, Percent, PackageC6, L"Package C6"),
};

// Vermeer 0x380905 (<= 8 cores)
static CONST PHP_HW_SMU_FIELD PhpHwSmuFields380905[] =
{
    PHP_SMU_FIELD(0, SmuPowerLimit, PowerLimit, Ppt, L"PPT limit"),
    PHP_SMU_FIELD(1, SmuPower, Power, Ppt, L"PPT"),
    PHP_SMU_FIELD(2, SmuCurrentLimit, CurrentLimit, Tdc, L"TDC limit"),
    PHP_SMU_FIELD(3, SmuCurrent, Current, Tdc, L"TDC"),
    PHP_SMU_FIELD(4, SmuTemperatureLimit, TemperatureLimit, Thermal, L"Thermal limit"),
    PHP_SMU_FIELD(8, SmuCurrentLimit, CurrentLimit, Edc, L"EDC limit"),
    PHP_SMU_FIELD(24, SmuPower, Power, CoreRail, L"Core rail"),
    PHP_SMU_FIELD(25, SmuPower, Power, SocRail, L"SoC rail"),
    PHP_SMU_FIELD(29, SmuPower, Power, Socket, L"Socket"),
    PHP_SMU_FIELD(40, SmuVoltage, RailVoltage, CoreRail, L"Core rail"),
    PHP_SMU_FIELD(42, SmuCurrent, Current, CoreRail, L"Core rail"),
    PHP_SMU_FIELD(45, SmuVoltage, RailVoltage, SocRail, L"SoC rail"),
    PHP_SMU_FIELD(46, SmuCurrent, Current, SocRail, L"SoC rail"),
    PHP_SMU_FIELD(48, SmuClock, FabricClock, Fclk, L"Fabric (FCLK)"),
    PHP_SMU_FIELD(50, SmuMemoryClock, FabricClock, Uclk, L"Controller (UCLK)"),
    PHP_SMU_FIELD(127, SmuTemperature, Temperature, Soc, L"SoC"),
    PHP_SMU_FIELD(137, SmuVoltage, RailVoltage, Vddp, L"VDDP"),
    PHP_SMU_FIELD(138, SmuVoltage, RailVoltage, VddgIod, L"VDDG IOD"),
    PHP_SMU_FIELD(139, SmuVoltage, RailVoltage, VddgCcd, L"VDDG CCD"),
    PHP_SMU_FIELD(155, SmuResidency, Percent, PackageC6, L"Package C6"),
};

// Matisse 0x240802 and 0x240902: fabric clocks and SoC rails only
static CONST PHP_HW_SMU_FIELD PhpHwSmuFields240802[] =
{
    PHP_SMU_FIELD(44, SmuVoltage, RailVoltage, SocRail, L"SoC rail"),
    PHP_SMU_FIELD(47, SmuClock, FabricClock, Fclk, L"Fabric (FCLK)"),
    PHP_SMU_FIELD(49, SmuMemoryClock, FabricClock, Uclk, L"Controller (UCLK)"),
    PHP_SMU_FIELD(124, SmuVoltage, RailVoltage, Vddp, L"VDDP"),
    PHP_SMU_FIELD(125, SmuVoltage, RailVoltage, Vddg, L"VDDG"),
};

// Milan/Chagall 0x2d0008, Vermeer 0x2d0803 and 0x2d0903: fabric clocks and SoC rails only
static CONST PHP_HW_SMU_FIELD PhpHwSmuFields2d0803[] =
{
    PHP_SMU_FIELD(44, SmuVoltage, RailVoltage, SocRail, L"SoC rail"),
    PHP_SMU_FIELD(47, SmuClock, FabricClock, Fclk, L"Fabric (FCLK)"),
    PHP_SMU_FIELD(49, SmuMemoryClock, FabricClock, Uclk, L"Controller (UCLK)"),
    PHP_SMU_FIELD(136, SmuVoltage, RailVoltage, Vddp, L"VDDP"),
    PHP_SMU_FIELD(137, SmuVoltage, RailVoltage, VddgIod, L"VDDG IOD"),
};

// Milan/Chagall 0x380005, 0x380505, 0x380605 and 0x380705: fabric clocks and SoC rails only
static CONST PHP_HW_SMU_FIELD PhpHwSmuFields380005[] =
{
    PHP_SMU_FIELD(45, SmuVoltage, RailVoltage, SocRail, L"SoC rail"),
    PHP_SMU_FIELD(48, SmuClock, FabricClock, Fclk, L"Fabric (FCLK)"),
    PHP_SMU_FIELD(50, SmuMemoryClock, FabricClock, Uclk, L"Controller (UCLK)"),
    PHP_SMU_FIELD(137, SmuVoltage, RailVoltage, Vddp, L"VDDP"),
    PHP_SMU_FIELD(138, SmuVoltage, RailVoltage, VddgIod, L"VDDG IOD"),
    PHP_SMU_FIELD(139, SmuVoltage, RailVoltage, VddgCcd, L"VDDG CCD"),
};

// The APU header shared by Renoir and Cezanne: (limit, value) pairs from 0 to 23, rail powers 34 and 36.
// PPT APU reads zero without a split budget and takes the limit range; the graphics temperature is in the tails.
#define PHP_SMU_APU_HEADER \
    PHP_SMU_FIELD(0, SmuPowerLimit, PowerLimit, Stapm, L"STAPM limit"), \
    PHP_SMU_FIELD(1, SmuPower, Power, Stapm, L"STAPM"), \
    PHP_SMU_FIELD(2, SmuPowerLimit, PowerLimit, PptFast, L"PPT fast limit"), \
    PHP_SMU_FIELD(3, SmuPower, Power, PptFast, L"PPT fast"), \
    PHP_SMU_FIELD(4, SmuPowerLimit, PowerLimit, PptSlow, L"PPT slow limit"), \
    PHP_SMU_FIELD(5, SmuPower, Power, PptSlow, L"PPT slow"), \
    PHP_SMU_FIELD(6, SmuPowerLimit, PowerLimit, PptApu, L"PPT APU limit"), \
    PHP_SMU_FIELD(7, SmuPower, PowerLimit, PptApu, L"PPT APU"), \
    PHP_SMU_FIELD(8, SmuCurrentLimit, CurrentLimit, Tdc, L"TDC limit"), \
    PHP_SMU_FIELD(9, SmuCurrent, Current, Tdc, L"TDC"), \
    PHP_SMU_FIELD(10, SmuCurrentLimit, CurrentLimit, TdcSoc, L"TDC SoC limit"), \
    PHP_SMU_FIELD(11, SmuCurrent, Current, TdcSoc, L"TDC SoC"), \
    PHP_SMU_FIELD(12, SmuCurrentLimit, CurrentLimit, Edc, L"EDC limit"), \
    PHP_SMU_FIELD(13, SmuCurrent, Current, Edc, L"EDC"), \
    PHP_SMU_FIELD(16, SmuTemperatureLimit, TemperatureLimit, Thermal, L"Thermal limit"), \
    PHP_SMU_FIELD(17, SmuTemperature, Temperature, Tctl, L"Tctl"), \
    PHP_SMU_FIELD(18, SmuTemperatureLimit, TemperatureLimit, GfxThermal, L"Graphics thermal limit"), \
    PHP_SMU_FIELD(20, SmuTemperatureLimit, TemperatureLimit, SocThermal, L"SoC thermal limit"), \
    PHP_SMU_FIELD(21, SmuTemperature, Temperature, SocThermal, L"SoC thermal"), \
    PHP_SMU_FIELD(22, SmuTemperatureLimit, TemperatureLimit, Skin, L"Skin limit"), \
    PHP_SMU_FIELD(23, SmuTemperature, Temperature, Skin, L"Skin"), \
    PHP_SMU_FIELD(34, SmuPower, Power, CoreRail, L"Core rail"), \
    PHP_SMU_FIELD(36, SmuPower, Power, MemRail, L"Memory rail"), \
    PHP_SMU_FIELD(38, SmuPower, Power, Socket, L"Socket")

// Raven Ridge 0x1E0004: currents, SoC rail, iGPU and fabric; the header past TDC/EDC is unsourced
static CONST PHP_HW_SMU_FIELD PhpHwSmuFields1e0004[] =
{
    PHP_SMU_FIELD(7, SmuCurrent, Current, Tdc, L"TDC"),
    PHP_SMU_FIELD(11, SmuCurrent, Current, Edc, L"EDC"),
    PHP_SMU_FIELD(66, SmuCurrent, Current, SocRail, L"SoC rail"),
    PHP_SMU_FIELD(67, SmuPower, Power, SocRail, L"SoC rail"),
    PHP_SMU_FIELD(150, SmuVoltage, RailVoltage, Gfx, L"iGPU"),
    PHP_SMU_FIELD(151, SmuTemperature, Temperature, Gfx, L"iGPU"),
    PHP_SMU_FIELD(154, SmuClock, FabricClock, Gfx, L"iGPU"),
    PHP_SMU_FIELD(166, SmuClock, FabricClock, Fclk, L"Fabric (FCLK)"),
    PHP_SMU_FIELD(177, SmuMemoryClock, FabricClock, Uclk, L"Controller (UCLK)"),
};

// Renoir 0x370000, 0x370001: header, SoC rail and fabric clocks; per-core arrays and VDDP held
static CONST PHP_HW_SMU_FIELD PhpHwSmuFields370000[] =
{
    PHP_SMU_APU_HEADER,
    PHP_SMU_FIELD(100, SmuVoltage, RailVoltage, SocRail, L"SoC rail"),
    PHP_SMU_FIELD(301, SmuClock, FabricClock, Fclk, L"Fabric (FCLK)"),
    PHP_SMU_FIELD(302, SmuMemoryClock, FabricClock, Uclk, L"Controller (UCLK)"),
};

static CONST PHP_HW_SMU_FIELD PhpHwSmuFields370001[] =
{
    PHP_SMU_APU_HEADER,
    PHP_SMU_FIELD(100, SmuVoltage, RailVoltage, SocRail, L"SoC rail"),
    PHP_SMU_FIELD(361, SmuClock, FabricClock, Fclk, L"Fabric (FCLK)"),
    PHP_SMU_FIELD(362, SmuMemoryClock, FabricClock, Uclk, L"Controller (UCLK)"),
};

// Renoir 0x370002: the SoC rail moved to 102
static CONST PHP_HW_SMU_FIELD PhpHwSmuFields370002[] =
{
    PHP_SMU_APU_HEADER,
    PHP_SMU_FIELD(102, SmuVoltage, RailVoltage, SocRail, L"SoC rail"),
    PHP_SMU_FIELD(363, SmuClock, FabricClock, Fclk, L"Fabric (FCLK)"),
    PHP_SMU_FIELD(364, SmuMemoryClock, FabricClock, Uclk, L"Controller (UCLK)"),
};

// Renoir 0x370003, 0x370004; the iGPU clock uses the fabric range so a GHz value is dropped
static CONST PHP_HW_SMU_FIELD PhpHwSmuFields370003[] =
{
    PHP_SMU_APU_HEADER,
    PHP_SMU_FIELD(102, SmuVoltage, RailVoltage, SocRail, L"SoC rail"),
    PHP_SMU_FIELD(104, SmuPower, Power, SocRail, L"SoC rail"),
    PHP_SMU_FIELD(336, SmuPower, Power, L3, L"L3"),
    PHP_SMU_FIELD(338, SmuVoltage, RailVoltage, Vddm, L"VDDM"),
    PHP_SMU_FIELD(340, SmuTemperature, Temperature, L3, L"L3"),
    PHP_SMU_FIELD(362, SmuVoltage, RailVoltage, Gfx, L"iGPU"),
    PHP_SMU_FIELD(363, SmuTemperature, Temperature, Gfx, L"iGPU"),
    PHP_SMU_FIELD(365, SmuClock, FabricClock, Gfx, L"iGPU"),
    PHP_SMU_FIELD(371, SmuClock, FabricClock, Fclk, L"Fabric (FCLK)"),
    PHP_SMU_FIELD(372, SmuMemoryClock, FabricClock, Uclk, L"Controller (UCLK)"),
};

// Renoir 0x370005; EDC SoC (14/15) reads zero
static CONST PHP_HW_SMU_FIELD PhpHwSmuFields370005[] =
{
    PHP_SMU_APU_HEADER,
    PHP_SMU_FIELD(102, SmuVoltage, RailVoltage, SocRail, L"SoC rail"),
    PHP_SMU_FIELD(104, SmuPower, Power, SocRail, L"SoC rail"),
    PHP_SMU_FIELD(343, SmuPower, Power, L3, L"L3"),
    PHP_SMU_FIELD(345, SmuVoltage, RailVoltage, Vddm, L"VDDM"),
    PHP_SMU_FIELD(347, SmuTemperature, Temperature, L3, L"L3"),
    PHP_SMU_FIELD(369, SmuVoltage, RailVoltage, Gfx, L"iGPU"),
    PHP_SMU_FIELD(370, SmuTemperature, Temperature, Gfx, L"iGPU"),
    PHP_SMU_FIELD(372, SmuClock, FabricClock, Gfx, L"iGPU"),
    PHP_SMU_FIELD(378, SmuClock, FabricClock, Fclk, L"Fabric (FCLK)"),
    PHP_SMU_FIELD(379, SmuMemoryClock, FabricClock, Uclk, L"Controller (UCLK)"),
};

// Cezanne 0x400001: fabric clocks, SoC rails and iGPU only
static CONST PHP_HW_SMU_FIELD PhpHwSmuFields400001[] =
{
    PHP_SMU_FIELD(103, SmuVoltage, RailVoltage, SocRail, L"SoC rail"),
    PHP_SMU_FIELD(384, SmuVoltage, RailVoltage, Gfx, L"iGPU"),
    PHP_SMU_FIELD(385, SmuTemperature, Temperature, Gfx, L"iGPU"),
    PHP_SMU_FIELD(387, SmuClock, FabricClock, Gfx, L"iGPU"),
    PHP_SMU_FIELD(393, SmuClock, FabricClock, Fclk, L"Fabric (FCLK)"),
    PHP_SMU_FIELD(394, SmuMemoryClock, FabricClock, Uclk, L"Controller (UCLK)"),
    PHP_SMU_FIELD(551, SmuVoltage, RailVoltage, Vddp, L"VDDP"),
};

// Cezanne 0x400002: fabric clocks, SoC rails and iGPU only
static CONST PHP_HW_SMU_FIELD PhpHwSmuFields400002[] =
{
    PHP_SMU_FIELD(103, SmuVoltage, RailVoltage, SocRail, L"SoC rail"),
    PHP_SMU_FIELD(390, SmuVoltage, RailVoltage, Gfx, L"iGPU"),
    PHP_SMU_FIELD(391, SmuTemperature, Temperature, Gfx, L"iGPU"),
    PHP_SMU_FIELD(393, SmuClock, FabricClock, Gfx, L"iGPU"),
    PHP_SMU_FIELD(399, SmuClock, FabricClock, Fclk, L"Fabric (FCLK)"),
    PHP_SMU_FIELD(400, SmuMemoryClock, FabricClock, Uclk, L"Controller (UCLK)"),
    PHP_SMU_FIELD(557, SmuVoltage, RailVoltage, Vddp, L"VDDP"),
};

// Cezanne 0x400003: fabric clocks, SoC rails and iGPU only
static CONST PHP_HW_SMU_FIELD PhpHwSmuFields400003[] =
{
    PHP_SMU_FIELD(103, SmuVoltage, RailVoltage, SocRail, L"SoC rail"),
    PHP_SMU_FIELD(398, SmuVoltage, RailVoltage, Gfx, L"iGPU"),
    PHP_SMU_FIELD(399, SmuTemperature, Temperature, Gfx, L"iGPU"),
    PHP_SMU_FIELD(401, SmuClock, FabricClock, Gfx, L"iGPU"),
    PHP_SMU_FIELD(408, SmuClock, FabricClock, Fclk, L"Fabric (FCLK)"),
    PHP_SMU_FIELD(409, SmuMemoryClock, FabricClock, Uclk, L"Controller (UCLK)"),
    PHP_SMU_FIELD(564, SmuVoltage, RailVoltage, Vddp, L"VDDP"),
};

// Cezanne 0x400004 and 0x400005
static CONST PHP_HW_SMU_FIELD PhpHwSmuFields400005[] =
{
    PHP_SMU_APU_HEADER,
    PHP_SMU_FIELD(14, SmuCurrentLimit, CurrentLimit, EdcSoc, L"EDC SoC limit"),
    PHP_SMU_FIELD(15, SmuCurrent, Current, EdcSoc, L"EDC SoC"),
    PHP_SMU_FIELD(35, SmuPower, Power, SocRail, L"SoC rail"),
    PHP_SMU_FIELD(99, SmuVoltage, RailVoltage, CoreRail, L"Core rail"),
    PHP_SMU_FIELD(100, SmuCurrent, Current, CoreRail, L"Core rail"),
    PHP_SMU_FIELD(103, SmuVoltage, RailVoltage, SocRail, L"SoC rail"),
    PHP_SMU_FIELD(104, SmuCurrent, Current, SocRail, L"SoC rail"),
    PHP_SMU_FIELD(384, SmuPower, Power, L3, L"L3"),
    PHP_SMU_FIELD(385, SmuVoltage, RailVoltage, Vddm, L"VDDM"),
    PHP_SMU_FIELD(386, SmuTemperature, Temperature, L3, L"L3"),
    PHP_SMU_FIELD(399, SmuVoltage, RailVoltage, Gfx, L"iGPU"),
    PHP_SMU_FIELD(400, SmuTemperature, Temperature, Gfx, L"iGPU"),
    PHP_SMU_FIELD(402, SmuClock, FabricClock, Gfx, L"iGPU"),
    PHP_SMU_FIELD(409, SmuClock, FabricClock, Fclk, L"Fabric (FCLK)"),
    PHP_SMU_FIELD(410, SmuMemoryClock, FabricClock, Uclk, L"Controller (UCLK)"),
    PHP_SMU_FIELD(565, SmuVoltage, RailVoltage, Vddp, L"VDDP"),
};

// Raphael 0x540004: core arrays not decoded
static CONST PHP_HW_SMU_FIELD PhpHwSmuFields540004[] =
{
    PHP_SMU_FIELD(3, SmuPower, Power, Ppt, L"PPT"),
    PHP_SMU_FIELD(20, SmuPower, Power, CoreRail, L"Core rail"),
    PHP_SMU_FIELD(21, SmuPower, Power, SocRail, L"SoC rail"),
    PHP_SMU_FIELD(22, SmuPower, Power, MiscRail, L"Misc rail"),
    PHP_SMU_FIELD(47, SmuVoltage, RailVoltage, CoreRail, L"Core rail"),
    PHP_SMU_FIELD(48, SmuCurrent, Current, CoreRail, L"Core rail"),
    PHP_SMU_FIELD(50, SmuTemperature, Temperature, CoreRailVrm, L"Core rail VRM"),
    PHP_SMU_FIELD(52, SmuVoltage, RailVoltage, SocRail, L"SoC rail"),
    PHP_SMU_FIELD(53, SmuCurrent, Current, SocRail, L"SoC rail"),
    PHP_SMU_FIELD(55, SmuTemperature, Temperature, SocRailVrm, L"SoC rail VRM"),
    PHP_SMU_FIELD(57, SmuVoltage, RailVoltage, MiscRail, L"Misc rail"),
    PHP_SMU_FIELD(58, SmuCurrent, Current, MiscRail, L"Misc rail"),
    PHP_SMU_FIELD(60, SmuTemperature, Temperature, MiscRailVrm, L"Misc rail VRM"),
    PHP_SMU_FIELD(70, SmuClock, FabricClock, Fclk, L"Fabric (FCLK)"),
    PHP_SMU_FIELD(74, SmuMemoryClock, FabricClock, Uclk, L"Controller (UCLK)"),
    PHP_SMU_FIELD(211, SmuTemperature, Temperature, Iod, L"IOD hotspot"),
    PHP_SMU_FIELD(268, SmuVoltage, RailVoltage, Vddp, L"VDDP"),
};

// Raphael 0x540104 (1 CCD): SVI3 rails as 0x540004; per-core voltage, temperature and clock, no power lane
static CONST PHP_HW_SMU_FIELD PhpHwSmuFields540104[] =
{
    PHP_SMU_FIELD(3, SmuPower, Power, Ppt, L"PPT"),
    PHP_SMU_FIELD(11, SmuTemperature, Temperature, Iod, L"IOD hotspot"),
    PHP_SMU_FIELD(47, SmuVoltage, RailVoltage, CoreRail, L"Core rail"),
    PHP_SMU_FIELD(48, SmuCurrent, Current, CoreRail, L"Core rail"),
    PHP_SMU_FIELD(50, SmuTemperature, Temperature, CoreRailVrm, L"Core rail VRM"),
    PHP_SMU_FIELD(52, SmuVoltage, RailVoltage, SocRail, L"SoC rail"),
    PHP_SMU_FIELD(53, SmuCurrent, Current, SocRail, L"SoC rail"),
    PHP_SMU_FIELD(55, SmuTemperature, Temperature, SocRailVrm, L"SoC rail VRM"),
    PHP_SMU_FIELD(57, SmuVoltage, RailVoltage, MiscRail, L"Misc rail"),
    PHP_SMU_FIELD(58, SmuCurrent, Current, MiscRail, L"Misc rail"),
    PHP_SMU_FIELD(60, SmuTemperature, Temperature, MiscRailVrm, L"Misc rail VRM"),
    PHP_SMU_FIELD(70, SmuClock, FabricClock, Fclk, L"Fabric (FCLK)"),
    PHP_SMU_FIELD(74, SmuMemoryClock, FabricClock, Uclk, L"Controller (UCLK)"),
    PHP_SMU_FIELD(82, SmuVoltage, RailVoltage, Vsoc, L"VSOC set point"),
    PHP_SMU_FIELD(269, SmuVoltage, RailVoltage, Vddp, L"VDDP"),
    PHP_SMU_FIELD(275, SmuVoltage, RailVoltage, CoreRailPeak, L"Core rail peak"),
};

// Storm Peak SVI3 block (VID, V, I, -, VRM temperature for VDD, VDD1, SoC, MISC) and clocks; the header and cores are unknown.
#define PHP_SMU_STORM_PEAK_FIELDS(SviBase, FclkIndex) \
    PHP_SMU_FIELD((SviBase) + 1, SmuVoltage, RailVoltage, CoreRail, L"Core rail"), \
    PHP_SMU_FIELD((SviBase) + 2, SmuCurrent, Current, CoreRail, L"Core rail"), \
    PHP_SMU_FIELD((SviBase) + 4, SmuTemperature, Temperature, CoreRailVrm, L"Core rail VRM"), \
    PHP_SMU_FIELD((SviBase) + 6, SmuVoltage, RailVoltage, CoreRail1, L"Core rail 1"), \
    PHP_SMU_FIELD((SviBase) + 7, SmuCurrent, Current, CoreRail1, L"Core rail 1"), \
    PHP_SMU_FIELD((SviBase) + 9, SmuTemperature, Temperature, CoreRail1Vrm, L"Core rail 1 VRM"), \
    PHP_SMU_FIELD((SviBase) + 11, SmuVoltage, RailVoltage, SocRail, L"SoC rail"), \
    PHP_SMU_FIELD((SviBase) + 12, SmuCurrent, Current, SocRail, L"SoC rail"), \
    PHP_SMU_FIELD((SviBase) + 14, SmuTemperature, Temperature, SocRailVrm, L"SoC rail VRM"), \
    PHP_SMU_FIELD((SviBase) + 16, SmuVoltage, RailVoltage, MiscRail, L"Misc rail"), \
    PHP_SMU_FIELD((SviBase) + 17, SmuCurrent, Current, MiscRail, L"Misc rail"), \
    PHP_SMU_FIELD((SviBase) + 19, SmuTemperature, Temperature, MiscRailVrm, L"Misc rail VRM"), \
    PHP_SMU_FIELD((FclkIndex), SmuClock, FabricClock, Fclk, L"Fabric (FCLK)"), \
    PHP_SMU_FIELD((FclkIndex) + 5, SmuMemoryClock, FabricClock, Uclk, L"Controller (UCLK)")

// Storm Peak 0x5C0002/03, 0x5C0102/03, 0x5C0202 and 0x5C0302 (one corrected report)
static CONST PHP_HW_SMU_FIELD PhpHwSmuFields5c0002[] =
{
    PHP_SMU_STORM_PEAK_FIELDS(61, 101),
};

// Storm Peak 0x5C0203, 0x5C0303 and 0x5C0402/03 (0x5C0203 verified on a 7985WX)
static CONST PHP_HW_SMU_FIELD PhpHwSmuFields5c0203[] =
{
    PHP_SMU_STORM_PEAK_FIELDS(63, 103),
};

// Shimada Peak 0x730204-0x731004 (one dump family): SVI3 VDD and SoC/MISC rails, clocks and CLDO
static CONST PHP_HW_SMU_FIELD PhpHwSmuFields730204[] =
{
    PHP_SMU_FIELD(95, SmuVoltage, RailVoltage, CoreRail, L"Core rail"),
    PHP_SMU_FIELD(96, SmuCurrent, Current, CoreRail, L"Core rail"),
    PHP_SMU_FIELD(98, SmuTemperature, Temperature, CoreRailVrm, L"Core rail VRM"),
    PHP_SMU_FIELD(105, SmuVoltage, RailVoltage, SocRail, L"SoC rail"),
    PHP_SMU_FIELD(110, SmuVoltage, RailVoltage, MiscRail, L"Misc rail"),
    PHP_SMU_FIELD(131, SmuClock, FabricClock, Fclk, L"Fabric (FCLK)"),
    PHP_SMU_FIELD(135, SmuMemoryClock, FabricClock, Uclk, L"Controller (UCLK)"),
    PHP_SMU_FIELD(371, SmuVoltage, RailVoltage, Vddp, L"VDDP"),
    PHP_SMU_FIELD(453, SmuVoltage, RailVoltage, VddgIod, L"VDDG IOD"),
    PHP_SMU_FIELD(455, SmuVoltage, RailVoltage, VddgCcd, L"VDDG CCD"),
};

// Raphael 0x540000-0x540005 and 0x540100-0x540108: fabric clocks and SoC rails only
static CONST PHP_HW_SMU_FIELD PhpHwSmuFields540000[] =
{
    PHP_SMU_FIELD(52, SmuVoltage, RailVoltage, SocRail, L"SoC rail"),
    PHP_SMU_FIELD(70, SmuClock, FabricClock, Fclk, L"Fabric (FCLK)"),
    PHP_SMU_FIELD(74, SmuMemoryClock, FabricClock, Uclk, L"Controller (UCLK)"),
    PHP_SMU_FIELD(268, SmuVoltage, RailVoltage, Vddp, L"VDDP"),
};

// Raphael 0x540208: fabric clocks and SoC rails only
static CONST PHP_HW_SMU_FIELD PhpHwSmuFields540208[] =
{
    PHP_SMU_FIELD(53, SmuVoltage, RailVoltage, SocRail, L"SoC rail"),
    PHP_SMU_FIELD(71, SmuClock, FabricClock, Fclk, L"Fabric (FCLK)"),
    PHP_SMU_FIELD(75, SmuMemoryClock, FabricClock, Uclk, L"Controller (UCLK)"),
    PHP_SMU_FIELD(269, SmuVoltage, RailVoltage, Vddp, L"VDDP"),
};

// Granite Ridge and Fire Range 0x620105, 0x620205, 0x621102 and 0x621202: (limit, value) pairs from 0 to 11;
// STAPM and PPT fast read zero on AM5 and take the limit range. Misc rail VID (58) not decoded.
static CONST PHP_HW_SMU_FIELD PhpHwSmuFields620205[] =
{
    PHP_SMU_FIELD(0, SmuPowerLimit, PowerLimit, Stapm, L"STAPM limit"),
    PHP_SMU_FIELD(1, SmuPower, PowerLimit, Stapm, L"STAPM"),
    PHP_SMU_FIELD(2, SmuPowerLimit, PowerLimit, Ppt, L"PPT limit"),
    PHP_SMU_FIELD(3, SmuPower, Power, Ppt, L"PPT"),
    PHP_SMU_FIELD(4, SmuPowerLimit, PowerLimit, PptFast, L"PPT fast limit"),
    PHP_SMU_FIELD(5, SmuPower, PowerLimit, PptFast, L"PPT fast"),
    PHP_SMU_FIELD(8, SmuCurrentLimit, CurrentLimit, Tdc, L"TDC limit"),
    PHP_SMU_FIELD(9, SmuCurrent, Current, Tdc, L"TDC"),
    PHP_SMU_FIELD(10, SmuTemperatureLimit, TemperatureLimit, Thermal, L"Thermal limit"),
    PHP_SMU_FIELD(11, SmuTemperature, Temperature, Tctl, L"Tctl"),
    PHP_SMU_FIELD(18, SmuVoltage, RailVoltage, CoreRailPeak, L"Core rail peak"),
    PHP_SMU_FIELD(19, SmuVoltage, RailVoltage, CoreRailAverage, L"Core rail average"),
    PHP_SMU_FIELD(20, SmuPower, Power, CoreDomain, L"Core domain"),
    PHP_SMU_FIELD(21, SmuPower, Power, SocRail, L"SoC rail"),
    PHP_SMU_FIELD(22, SmuPower, Power, CoreRail, L"Core rail"),
    PHP_SMU_FIELD(23, SmuPower, Power, MemRail, L"Memory rail"),
    PHP_SMU_FIELD(49, SmuVoltage, RailVoltage, CoreRail, L"Core rail"),
    PHP_SMU_FIELD(50, SmuCurrent, Current, CoreRail, L"Core rail"),
    PHP_SMU_FIELD(52, SmuTemperature, Temperature, CoreRailVrm, L"Core rail VRM"),
    PHP_SMU_FIELD(54, SmuVoltage, RailVoltage, SocRail, L"SoC rail"),
    PHP_SMU_FIELD(55, SmuCurrent, Current, SocRail, L"SoC rail"),
    PHP_SMU_FIELD(57, SmuTemperature, Temperature, SocRailVrm, L"SoC rail VRM"),
    PHP_SMU_FIELD(59, SmuVoltage, RailVoltage, MiscRail, L"Misc rail"),
    PHP_SMU_FIELD(60, SmuCurrent, Current, MiscRail, L"Misc rail"),
    PHP_SMU_FIELD(62, SmuTemperature, Temperature, MiscRailVrm, L"Misc rail VRM"),
    PHP_SMU_FIELD(63, SmuCurrentLimit, CurrentLimit, Edc, L"EDC limit"),
    PHP_SMU_FIELD(71, SmuClock, FabricClock, Fclk, L"Fabric (FCLK)"),
    PHP_SMU_FIELD(75, SmuMemoryClock, FabricClock, Uclk, L"Controller (UCLK)"),
    PHP_SMU_FIELD(83, SmuVoltage, RailVoltage, Vsoc, L"VSOC set point"),
    PHP_SMU_FIELD(86, SmuVoltage, RailVoltage, Vddm, L"VDDM"),
    PHP_SMU_FIELD(107, SmuPower, Power, Gfx, L"iGPU"),
    PHP_SMU_FIELD(108, SmuClock, FabricClock, Gfx, L"iGPU"),
    PHP_SMU_FIELD(110, SmuCurrent, Current, Gfx, L"iGPU"),
    PHP_SMU_FIELD(259, SmuVoltage, RailVoltage, VddgIod, L"VDDG IOD"),
    PHP_SMU_FIELD(261, SmuVoltage, RailVoltage, VddgCcd, L"VDDG CCD"),
    PHP_SMU_FIELD(269, SmuVoltage, RailVoltage, Vddp, L"VDDP"),
    PHP_SMU_FIELD(270, SmuTemperature, Temperature, Hotspot, L"Hotspot"),
};

#define PHP_SMU_FIELDS(Fields) (Fields), RTL_NUMBER_OF(Fields)
#define PHP_SMU_CORE_SUFFIX L" (SMU)" // set apart from the per-processor sensors
#define PHP_SMU_NO_CORES 0, PHP_HW_SMU_NONE, PHP_HW_SMU_NONE, PHP_HW_SMU_NONE, PHP_HW_SMU_NONE, PHP_HW_SMU_NONE, PHP_HW_SMU_NONE, PHP_HW_SMU_NONE, PHP_HW_SMU_NONE
#define PHP_SMU_NO_L3 0, PHP_HW_SMU_NONE

// Version, fields, MCLK, DDR, L3 count and base, core slots, power, voltage, temperature, clock, effective clock, C0, CC6, current.
static CONST PHP_HW_SMU_LAYOUT PhpHwSmuLayouts[] =
{
    { 0x1e0004, PHP_SMU_FIELDS(PhpHwSmuFields1e0004), 178, FALSE, PHP_SMU_NO_L3, 4, PHP_HW_SMU_NONE, PHP_HW_SMU_NONE, 108, PHP_HW_SMU_NONE, PHP_HW_SMU_NONE, PHP_HW_SMU_NONE, PHP_HW_SMU_NONE, PHP_HW_SMU_NONE },
    { 0x240802, PHP_SMU_FIELDS(PhpHwSmuFields240802), 50, TRUE, PHP_SMU_NO_L3, PHP_SMU_NO_CORES },
    { 0x240803, PHP_SMU_FIELDS(PhpHwSmuFields240803), 51, TRUE, 4, 459, 16, 147, 163, 179, 227, 243, 259, 291, PHP_HW_SMU_NONE },
    { 0x240902, PHP_SMU_FIELDS(PhpHwSmuFields240802), 50, TRUE, PHP_SMU_NO_L3, PHP_SMU_NO_CORES },
    { 0x240903, PHP_SMU_FIELDS(PhpHwSmuFields240903), 51, TRUE, 2, 303, 8, 147, 155, 163, 187, 195, 203, 219, PHP_HW_SMU_NONE },
    { 0x2d0008, PHP_SMU_FIELDS(PhpHwSmuFields2d0803), 50, TRUE, PHP_SMU_NO_L3, PHP_SMU_NO_CORES },
    { 0x2d0803, PHP_SMU_FIELDS(PhpHwSmuFields2d0803), 50, TRUE, PHP_SMU_NO_L3, PHP_SMU_NO_CORES },
    { 0x2d0903, PHP_SMU_FIELDS(PhpHwSmuFields2d0803), 50, TRUE, PHP_SMU_NO_L3, PHP_SMU_NO_CORES },
    { 0x370000, PHP_SMU_FIELDS(PhpHwSmuFields370000), 303, FALSE, PHP_SMU_NO_L3, PHP_SMU_NO_CORES },
    { 0x370001, PHP_SMU_FIELDS(PhpHwSmuFields370001), 363, FALSE, PHP_SMU_NO_L3, PHP_SMU_NO_CORES },
    { 0x370002, PHP_SMU_FIELDS(PhpHwSmuFields370002), 365, FALSE, PHP_SMU_NO_L3, PHP_SMU_NO_CORES },
    { 0x370003, PHP_SMU_FIELDS(PhpHwSmuFields370003), 373, FALSE, PHP_SMU_NO_L3, 8, 192, 200, 208, 232, PHP_HW_SMU_NONE, PHP_HW_SMU_NONE, PHP_HW_SMU_NONE, PHP_HW_SMU_NONE },
    { 0x370004, PHP_SMU_FIELDS(PhpHwSmuFields370003), 373, FALSE, PHP_SMU_NO_L3, 8, 192, 200, 208, 232, PHP_HW_SMU_NONE, PHP_HW_SMU_NONE, PHP_HW_SMU_NONE, PHP_HW_SMU_NONE },
    { 0x370005, PHP_SMU_FIELDS(PhpHwSmuFields370005), 380, FALSE, PHP_SMU_NO_L3, 8, 199, 207, 215, 239, PHP_HW_SMU_NONE, PHP_HW_SMU_NONE, PHP_HW_SMU_NONE, PHP_HW_SMU_NONE },
    { 0x380005, PHP_SMU_FIELDS(PhpHwSmuFields380005), 51, TRUE, PHP_SMU_NO_L3, PHP_SMU_NO_CORES },
    { 0x380505, PHP_SMU_FIELDS(PhpHwSmuFields380005), 51, TRUE, PHP_SMU_NO_L3, PHP_SMU_NO_CORES },
    { 0x380605, PHP_SMU_FIELDS(PhpHwSmuFields380005), 51, TRUE, PHP_SMU_NO_L3, PHP_SMU_NO_CORES },
    { 0x380705, PHP_SMU_FIELDS(PhpHwSmuFields380005), 51, TRUE, PHP_SMU_NO_L3, PHP_SMU_NO_CORES },
    { 0x380804, PHP_SMU_FIELDS(PhpHwSmuFields380804), 51, TRUE, PHP_SMU_NO_L3, PHP_SMU_NO_CORES },
    { 0x380805, PHP_SMU_FIELDS(PhpHwSmuFields380805), 51, TRUE, 2, 544, 16, 172, 188, 204, 252, 268, 284, 316, PHP_HW_SMU_NONE },
    { 0x380904, PHP_SMU_FIELDS(PhpHwSmuFields380904), 51, TRUE, PHP_SMU_NO_L3, 8, 169, 177, 185, 209, 217, 225, 241, PHP_HW_SMU_NONE },
    { 0x380905, PHP_SMU_FIELDS(PhpHwSmuFields380905), 51, TRUE, PHP_SMU_NO_L3, 8, 172, 180, 188, 212, 220, 228, 244, PHP_HW_SMU_NONE },
    { 0x400001, PHP_SMU_FIELDS(PhpHwSmuFields400001), 395, FALSE, PHP_SMU_NO_L3, PHP_SMU_NO_CORES },
    { 0x400002, PHP_SMU_FIELDS(PhpHwSmuFields400002), 401, FALSE, PHP_SMU_NO_L3, PHP_SMU_NO_CORES },
    { 0x400003, PHP_SMU_FIELDS(PhpHwSmuFields400003), 410, FALSE, PHP_SMU_NO_L3, PHP_SMU_NO_CORES },
    { 0x400004, PHP_SMU_FIELDS(PhpHwSmuFields400005), 411, FALSE, PHP_SMU_NO_L3, 8, 200, 208, 216, 240, 248, 256, 272, PHP_HW_SMU_NONE },
    { 0x400005, PHP_SMU_FIELDS(PhpHwSmuFields400005), 411, FALSE, PHP_SMU_NO_L3, 8, 200, 208, 216, 240, 248, 256, 272, PHP_HW_SMU_NONE },
    { 0x540000, PHP_SMU_FIELDS(PhpHwSmuFields540000), 78, TRUE, PHP_SMU_NO_L3, PHP_SMU_NO_CORES },
    { 0x540001, PHP_SMU_FIELDS(PhpHwSmuFields540000), 78, TRUE, PHP_SMU_NO_L3, PHP_SMU_NO_CORES },
    { 0x540002, PHP_SMU_FIELDS(PhpHwSmuFields540000), 78, TRUE, PHP_SMU_NO_L3, PHP_SMU_NO_CORES },
    { 0x540003, PHP_SMU_FIELDS(PhpHwSmuFields540000), 78, TRUE, PHP_SMU_NO_L3, PHP_SMU_NO_CORES },
    { 0x540004, PHP_SMU_FIELDS(PhpHwSmuFields540004), 78, TRUE, 2, 539, PHP_SMU_NO_CORES },
    { 0x540005, PHP_SMU_FIELDS(PhpHwSmuFields540000), 78, TRUE, PHP_SMU_NO_L3, PHP_SMU_NO_CORES },
    { 0x540100, PHP_SMU_FIELDS(PhpHwSmuFields540000), 78, TRUE, PHP_SMU_NO_L3, PHP_SMU_NO_CORES },
    { 0x540101, PHP_SMU_FIELDS(PhpHwSmuFields540000), 78, TRUE, PHP_SMU_NO_L3, PHP_SMU_NO_CORES },
    { 0x540102, PHP_SMU_FIELDS(PhpHwSmuFields540000), 78, TRUE, PHP_SMU_NO_L3, PHP_SMU_NO_CORES },
    { 0x540103, PHP_SMU_FIELDS(PhpHwSmuFields540000), 78, TRUE, PHP_SMU_NO_L3, PHP_SMU_NO_CORES },
    { 0x540104, PHP_SMU_FIELDS(PhpHwSmuFields540104), 78, TRUE, PHP_SMU_NO_L3, 8, PHP_HW_SMU_NONE, 301, 309, 317, PHP_HW_SMU_NONE, PHP_HW_SMU_NONE, PHP_HW_SMU_NONE, PHP_HW_SMU_NONE },
    { 0x540105, PHP_SMU_FIELDS(PhpHwSmuFields540000), 78, TRUE, PHP_SMU_NO_L3, PHP_SMU_NO_CORES },
    { 0x540108, PHP_SMU_FIELDS(PhpHwSmuFields540000), 78, TRUE, PHP_SMU_NO_L3, PHP_SMU_NO_CORES },
    { 0x540208, PHP_SMU_FIELDS(PhpHwSmuFields540208), 79, TRUE, PHP_SMU_NO_L3, PHP_SMU_NO_CORES },
    { 0x5c0002, PHP_SMU_FIELDS(PhpHwSmuFields5c0002), 111, TRUE, PHP_SMU_NO_L3, PHP_SMU_NO_CORES },
    { 0x5c0003, PHP_SMU_FIELDS(PhpHwSmuFields5c0002), 111, TRUE, PHP_SMU_NO_L3, PHP_SMU_NO_CORES },
    { 0x5c0102, PHP_SMU_FIELDS(PhpHwSmuFields5c0002), 111, TRUE, PHP_SMU_NO_L3, PHP_SMU_NO_CORES },
    { 0x5c0103, PHP_SMU_FIELDS(PhpHwSmuFields5c0002), 111, TRUE, PHP_SMU_NO_L3, PHP_SMU_NO_CORES },
    { 0x5c0202, PHP_SMU_FIELDS(PhpHwSmuFields5c0002), 111, TRUE, PHP_SMU_NO_L3, PHP_SMU_NO_CORES },
    { 0x5c0203, PHP_SMU_FIELDS(PhpHwSmuFields5c0203), 113, TRUE, PHP_SMU_NO_L3, PHP_SMU_NO_CORES },
    { 0x5c0302, PHP_SMU_FIELDS(PhpHwSmuFields5c0002), 111, TRUE, PHP_SMU_NO_L3, PHP_SMU_NO_CORES },
    { 0x5c0303, PHP_SMU_FIELDS(PhpHwSmuFields5c0203), 113, TRUE, PHP_SMU_NO_L3, PHP_SMU_NO_CORES },
    { 0x5c0402, PHP_SMU_FIELDS(PhpHwSmuFields5c0203), 113, TRUE, PHP_SMU_NO_L3, PHP_SMU_NO_CORES },
    { 0x5c0403, PHP_SMU_FIELDS(PhpHwSmuFields5c0203), 113, TRUE, PHP_SMU_NO_L3, PHP_SMU_NO_CORES },
    { 0x620105, PHP_SMU_FIELDS(PhpHwSmuFields620205), 79, TRUE, PHP_SMU_NO_L3, 8, 333, 309, 317, 325, PHP_HW_SMU_NONE, 341, 357, 301 },
    { 0x620205, PHP_SMU_FIELDS(PhpHwSmuFields620205), 79, TRUE, 2, 595, 16, 365, 317, 333, 349, PHP_HW_SMU_NONE, 381, 413, 301 },
    { 0x621102, PHP_SMU_FIELDS(PhpHwSmuFields620205), 79, TRUE, PHP_SMU_NO_L3, 8, 333, 309, 317, 325, PHP_HW_SMU_NONE, 341, 357, 301 },
    { 0x621202, PHP_SMU_FIELDS(PhpHwSmuFields620205), 79, TRUE, 2, 595, 16, 365, 317, 333, 349, PHP_HW_SMU_NONE, 381, 413, 301 },
    { 0x730204, PHP_SMU_FIELDS(PhpHwSmuFields730204), 139, TRUE, PHP_SMU_NO_L3, PHP_SMU_NO_CORES },
    { 0x730404, PHP_SMU_FIELDS(PhpHwSmuFields730204), 139, TRUE, PHP_SMU_NO_L3, PHP_SMU_NO_CORES },
    { 0x730604, PHP_SMU_FIELDS(PhpHwSmuFields730204), 139, TRUE, PHP_SMU_NO_L3, PHP_SMU_NO_CORES },
    { 0x730804, PHP_SMU_FIELDS(PhpHwSmuFields730204), 139, TRUE, PHP_SMU_NO_L3, PHP_SMU_NO_CORES },
    { 0x730a04, PHP_SMU_FIELDS(PhpHwSmuFields730204), 139, TRUE, PHP_SMU_NO_L3, PHP_SMU_NO_CORES },
    { 0x730c04, PHP_SMU_FIELDS(PhpHwSmuFields730204), 139, TRUE, PHP_SMU_NO_L3, PHP_SMU_NO_CORES },
    { 0x730e04, PHP_SMU_FIELDS(PhpHwSmuFields730204), 139, TRUE, PHP_SMU_NO_L3, PHP_SMU_NO_CORES },
    { 0x731004, PHP_SMU_FIELDS(PhpHwSmuFields730204), 139, TRUE, PHP_SMU_NO_L3, PHP_SMU_NO_CORES },
};

C_ASSERT(PhHwNamedSensorMaximum <= UCHAR_MAX);
C_ASSERT(PH_HW_SMU_MAX_CORE_SLOTS <= sizeof(ULONG) * 8);
C_ASSERT(PH_HW_SMU_MAX_READINGS >= PH_HW_SMU_MAX_CORE_SLOTS * 6 + 64);
C_ASSERT(RTL_NUMBER_OF(PhpHwSmuFields620205) + 2 <= 64 && RTL_NUMBER_OF(PhpHwSmuFields400005) <= 64);
C_ASSERT(PhHwSmuIdTctl < PhHwSmuIdCoreRail && PhHwSmuIdCoreRail1 < PhHwSmuIdSoc);
C_ASSERT(PhHwSmuIdHotspot < PhHwSmuIdVddp && PhHwSmuIdVsoc < PhHwSmuIdFclk);

static PCPHP_HW_SMU_LAYOUT PhpHwSmuFindLayout(
    _In_ ULONG Version
    )
{
    for (ULONG i = 0; i < RTL_NUMBER_OF(PhpHwSmuLayouts); i++)
    {
        if (PhpHwSmuLayouts[i].Version == Version)
        {
            // The core readings and slot mask are sized for PH_HW_SMU_MAX_CORE_SLOTS.
            assert(PhpHwSmuLayouts[i].CoreSlots <= PH_HW_SMU_MAX_CORE_SLOTS);

            if (PhpHwSmuLayouts[i].CoreSlots > PH_HW_SMU_MAX_CORE_SLOTS)
                return NULL;

            return &PhpHwSmuLayouts[i];
        }
    }

    return NULL;
}

_Success_(return)
static BOOLEAN PhpHwSmuReadFloat(
    _In_reads_bytes_(Length) PCVOID Table,
    _In_ ULONG Length,
    _In_ ULONG Index,
    _Out_ PFLOAT Value
    )
{
    FLOAT value;

    if (Index == PHP_HW_SMU_NONE || Index >= Length / sizeof(FLOAT))
        return FALSE;

    memcpy(&value, (PCSTR)Table + Index * sizeof(FLOAT), sizeof(FLOAT));

    if (!(value >= -FLT_MAX && value <= FLT_MAX))
        return FALSE;

    *Value = value;
    return TRUE;
}

static BOOLEAN PhpHwSmuPlausible(
    _In_ PHP_HW_SMU_RANGE Range,
    _In_ FLOAT Value
    )
{
    return Value >= PhpHwSmuRanges[Range].Minimum && Value <= PhpHwSmuRanges[Range].Maximum;
}

static VOID PhpHwSmuAddReading(
    _Inout_ PPH_HW_SMU_DECODE Decode,
    _In_ PH_HW_NAMED_SENSOR_KIND Kind,
    _In_ PHP_HW_SMU_RANGE Range,
    _In_ ULONG Id,
    _In_ PCWSTR Name,
    _In_ ULONG Index,
    _In_opt_ PCWSTR Suffix,
    _In_ FLOAT Value
    )
{
    PPH_HW_SMU_READING reading;

    if (!(Value >= -FLT_MAX && Value <= FLT_MAX) || !PhpHwSmuPlausible(Range, Value))
        return;
    if (Decode->Count >= PH_HW_SMU_MAX_READINGS)
        return;

    reading = &Decode->Readings[Decode->Count++];
    reading->Kind = Kind;
    reading->Id = Id;
    reading->Name = Name;
    reading->Index = Index;
    reading->Suffix = Suffix;
    reading->Value = Value;
}

static VOID PhpHwSmuAddField(
    _Inout_ PPH_HW_SMU_DECODE Decode,
    _In_reads_bytes_(Length) PCVOID Table,
    _In_ ULONG Length,
    _In_ ULONG Index,
    _In_ PH_HW_NAMED_SENSOR_KIND Kind,
    _In_ PHP_HW_SMU_RANGE Range,
    _In_ ULONG Id,
    _In_ PCWSTR Name,
    _In_ ULONG NameIndex,
    _In_opt_ PCWSTR Suffix,
    _In_ FLOAT Scale
    )
{
    FLOAT value;

    if (PhpHwSmuReadFloat(Table, Length, Index, &value))
        PhpHwSmuAddReading(Decode, Kind, Range, Id, Name, NameIndex, Suffix, value * Scale);
}

// A fused-off core slot reads zero in every activity field.
static BOOLEAN PhpHwSmuCoreSlotPopulated(
    _In_ PCPHP_HW_SMU_LAYOUT Layout,
    _In_reads_bytes_(Length) PCVOID Table,
    _In_ ULONG Length,
    _In_ ULONG Slot
    )
{
    USHORT arrays[6];

    arrays[0] = Layout->CorePower;
    arrays[1] = Layout->CoreVoltage;
    arrays[2] = Layout->CoreClock;
    arrays[3] = Layout->CoreEffectiveClock;
    arrays[4] = Layout->CoreC0;
    arrays[5] = Layout->CoreCurrent;

    for (ULONG i = 0; i < RTL_NUMBER_OF(arrays); i++)
    {
        FLOAT value;

        if (arrays[i] != PHP_HW_SMU_NONE && PhpHwSmuReadFloat(Table, Length, arrays[i] + Slot, &value) && value != 0.0f)
            return TRUE;
    }

    return FALSE;
}

// Slots are physical; a locked slot is named by its rank (OS core order) and published even when zero.
static VOID PhpHwSmuDecodeCores(
    _In_ PCPHP_HW_SMU_LAYOUT Layout,
    _In_reads_bytes_(Length) PCVOID Table,
    _In_ ULONG Length,
    _In_ ULONG CoreCount,
    _Inout_ PPH_HW_SMU_CORE_SLOTS CoreSlots,
    _Inout_ PPH_HW_SMU_DECODE Decode
    )
{
    ULONG populated = 0;
    ULONG populatedCount = 0;
    ULONG core = 0;

    if (!Layout->CoreSlots || CoreSlots->Ended)
        return;

    for (ULONG slot = 0; slot < Layout->CoreSlots; slot++)
    {
        if (PhpHwSmuCoreSlotPopulated(Layout, Table, Length, slot))
        {
            populated |= 1UL << slot;
            populatedCount++;
        }
    }

    if (!CoreSlots->Locked)
    {
        if (!populatedCount || populatedCount != CoreCount)
            return;

        CoreSlots->Mask = populated;
        CoreSlots->Locked = TRUE;
    }
    else if (populated & ~CoreSlots->Mask)
    {
        CoreSlots->Ended = TRUE;
        Decode->CoresEnded = TRUE;
        return;
    }

    for (ULONG slot = 0; slot < Layout->CoreSlots; slot++)
    {
        ULONG id = PhHwSmuIdCore + core;

        if (!(CoreSlots->Mask & (1UL << slot)))
            continue;

        if (Layout->CoreTemperature != PHP_HW_SMU_NONE)
            PhpHwSmuAddField(Decode, Table, Length, Layout->CoreTemperature + slot, PhHwNamedSensorSmuTemperature, PhpHwSmuRangeTemperature, id, L"Core ", core, PHP_SMU_CORE_SUFFIX, 1.0f);
        if (Layout->CorePower != PHP_HW_SMU_NONE)
            PhpHwSmuAddField(Decode, Table, Length, Layout->CorePower + slot, PhHwNamedSensorSmuPower, PhpHwSmuRangePower, id, L"Core ", core, PHP_SMU_CORE_SUFFIX, 1.0f);
        if (Layout->CoreVoltage != PHP_HW_SMU_NONE)
            PhpHwSmuAddField(Decode, Table, Length, Layout->CoreVoltage + slot, PhHwNamedSensorSmuVoltage, PhpHwSmuRangeCoreVoltage, id, L"Core ", core, PHP_SMU_CORE_SUFFIX, 1.0f);
        if (Layout->CoreCurrent != PHP_HW_SMU_NONE)
            PhpHwSmuAddField(Decode, Table, Length, Layout->CoreCurrent + slot, PhHwNamedSensorSmuCurrent, PhpHwSmuRangeCurrent, id, L"Core ", core, PHP_SMU_CORE_SUFFIX, 1.0f);
        if (Layout->CoreClock != PHP_HW_SMU_NONE)
            PhpHwSmuAddField(Decode, Table, Length, Layout->CoreClock + slot, PhHwNamedSensorSmuClock, PhpHwSmuRangeCoreClock, id, L"Core ", core, PHP_SMU_CORE_SUFFIX, 1000.0f);
        if (Layout->CoreCc6 != PHP_HW_SMU_NONE)
            PhpHwSmuAddField(Decode, Table, Length, Layout->CoreCc6 + slot, PhHwNamedSensorSmuResidency, PhpHwSmuRangePercent, id, L"Core ", core, PHP_SMU_CORE_SUFFIX, 1.0f);

        core++;
    }
}

/**
 * Decodes a PM table; out-of-bounds, non-finite and implausible values are dropped.
 *
 * \param Version The table version.
 * \param Table The table.
 * \param Length The number of bytes copied.
 * \param CoreCount The core count of the package.
 * \param CoreSlots The per-core slot state of the session.
 * \param Decode Receives the readings, appended.
 * \return FALSE when the version has no layout.
 */
_Success_(return)
BOOLEAN PhHwSmuDecodePmTable(
    _In_ ULONG Version,
    _In_reads_bytes_(Length) PCVOID Table,
    _In_ ULONG Length,
    _In_ ULONG CoreCount,
    _Inout_ PPH_HW_SMU_CORE_SLOTS CoreSlots,
    _Inout_ PPH_HW_SMU_DECODE Decode
    )
{
    PCPHP_HW_SMU_LAYOUT layout;
    FLOAT value;

    if (!(layout = PhpHwSmuFindLayout(Version)))
        return FALSE;

    for (ULONG i = 0; i < layout->FieldCount; i++)
    {
        PCPHP_HW_SMU_FIELD field = &layout->Fields[i];

        PhpHwSmuAddField(
            Decode,
            Table,
            Length,
            field->Index,
            (PH_HW_NAMED_SENSOR_KIND)field->Kind,
            (PHP_HW_SMU_RANGE)field->Range,
            field->Id,
            field->Name,
            PH_HW_SMU_NO_INDEX,
            NULL,
            1.0f
            );
    }

    if (PhpHwSmuReadFloat(Table, Length, layout->Mclk, &value) && PhpHwSmuPlausible(PhpHwSmuRangeFabricClock, value))
    {
        if (layout->MclkIsDdr)
        {
            Decode->MemoryClock = value;
            Decode->MemoryClockValid = TRUE;
        }
        else
        {
            PhpHwSmuAddReading(Decode, PhHwNamedSensorSmuMemoryClock, PhpHwSmuRangeFabricClock, PhHwSmuIdMclk, L"MCLK", PH_HW_SMU_NO_INDEX, NULL, value);
        }
    }

    for (ULONG i = 0; i < layout->L3Count; i++)
    {
        PhpHwSmuAddField(Decode, Table, Length, layout->L3Temperature + i, PhHwNamedSensorSmuTemperature, PhpHwSmuRangeTemperature, PhHwSmuIdL3 + i, L"L3 ", i, NULL, 1.0f);
    }

    PhpHwSmuDecodeCores(layout, Table, Length, CoreCount, CoreSlots, Decode);

    return TRUE;
}

// HSMP GET commands read every pass; none takes an argument.
CONST UCHAR PhHwSmuHsmpCommands[] =
{
    PH_HW_HSMP_GET_SOCKET_POWER,
    PH_HW_HSMP_GET_SOCKET_POWER_LIMIT,
    PH_HW_HSMP_GET_SOCKET_POWER_LIMIT_MAX,
    PH_HW_HSMP_GET_FCLK_MCLK,
    PH_HW_HSMP_GET_CCLK_THROTTLE_LIMIT,
    PH_HW_HSMP_GET_C0_PERCENT,
    PH_HW_HSMP_GET_DDR_BANDWIDTH,
    PH_HW_HSMP_GET_TEMP_MONITOR,
    PH_HW_HSMP_GET_SOCKET_FREQ_LIMIT,
    PH_HW_HSMP_GET_RAILS_SVI,
    PH_HW_HSMP_GET_SOCKET_FMAX_FMIN,
};

CONST ULONG PhHwSmuHsmpCommandCount = RTL_NUMBER_OF(PhHwSmuHsmpCommands);
C_ASSERT(RTL_NUMBER_OF(PhHwSmuHsmpCommands) <= PH_HW_HSMP_MAX_COMMANDS);
C_ASSERT(PH_HW_HSMP_MAX_COMMANDS <= sizeof(ULONG) * 8);

/**
 * Decodes an HSMP GET response in the units of the HSMP interface.
 *
 * \param Opcode The command.
 * \param Response The first two response words.
 * \param Decode Receives the readings, appended.
 */
VOID PhHwSmuDecodeHsmp(
    _In_ ULONG Opcode,
    _In_reads_(2) const ULONG* Response,
    _Inout_ PPH_HW_SMU_DECODE Decode
    )
{
    ULONG value = Response[0];

    switch (Opcode)
    {
    case PH_HW_HSMP_GET_SOCKET_POWER: // mW
        PhpHwSmuAddReading(Decode, PhHwNamedSensorSmuPower, PhpHwSmuRangePower, PhHwSmuIdSocket, L"Socket", PH_HW_SMU_NO_INDEX, NULL, (FLOAT)value / 1000.0f);
        break;
    case PH_HW_HSMP_GET_SOCKET_POWER_LIMIT: // mW
        PhpHwSmuAddReading(Decode, PhHwNamedSensorSmuPowerLimit, PhpHwSmuRangePowerLimit, PhHwSmuIdSocket, L"Socket limit", PH_HW_SMU_NO_INDEX, NULL, (FLOAT)value / 1000.0f);
        break;
    case PH_HW_HSMP_GET_SOCKET_POWER_LIMIT_MAX: // mW
        PhpHwSmuAddReading(Decode, PhHwNamedSensorSmuPowerLimit, PhpHwSmuRangePowerLimit, PhHwSmuIdSocketMax, L"Socket limit (max)", PH_HW_SMU_NO_INDEX, NULL, (FLOAT)value / 1000.0f);
        break;
    case PH_HW_HSMP_GET_FCLK_MCLK: // MHz, MHz
        PhpHwSmuAddReading(Decode, PhHwNamedSensorSmuClock, PhpHwSmuRangeFabricClock, PhHwSmuIdFclk, L"Fabric (FCLK)", PH_HW_SMU_NO_INDEX, NULL, (FLOAT)value);

        if (PhpHwSmuPlausible(PhpHwSmuRangeFabricClock, (FLOAT)Response[1]))
        {
            Decode->MemoryClock = (FLOAT)Response[1];
            Decode->MemoryClockValid = TRUE;
        }
        break;
    case PH_HW_HSMP_GET_CCLK_THROTTLE_LIMIT: // MHz
        PhpHwSmuAddReading(Decode, PhHwNamedSensorSmuClockLimit, PhpHwSmuRangeFabricClock, PhHwSmuIdThrottleLimit, L"Core throttle limit", PH_HW_SMU_NO_INDEX, NULL, (FLOAT)value);
        break;
    case PH_HW_HSMP_GET_C0_PERCENT: // percent
        PhpHwSmuAddReading(Decode, PhHwNamedSensorSmuResidency, PhpHwSmuRangePercent, PhHwSmuIdSocketC0, L"Socket C0", PH_HW_SMU_NO_INDEX, NULL, (FLOAT)value);
        break;
    case PH_HW_HSMP_GET_DDR_BANDWIDTH: // max [31:20] GB/s, used [19:8] GB/s, percent [7:0]
        PhpHwSmuAddReading(Decode, PhHwNamedSensorSmuMemoryLoad, PhpHwSmuRangePercent, PhHwSmuIdDramBandwidth, L"DRAM bandwidth", PH_HW_SMU_NO_INDEX, NULL, (FLOAT)(value & 0xFF));
        PhpHwSmuAddReading(Decode, PhHwNamedSensorSmuBandwidth, PhpHwSmuRangeBandwidth, PhHwSmuIdDramBandwidthMax, L"DRAM bandwidth (max)", PH_HW_SMU_NO_INDEX, NULL, (FLOAT)(value >> 20) * 1000.0f);
        PhpHwSmuAddReading(Decode, PhHwNamedSensorSmuBandwidth, PhpHwSmuRangeBandwidth, PhHwSmuIdDramBandwidthUsed, L"DRAM bandwidth (used)", PH_HW_SMU_NO_INDEX, NULL, (FLOAT)((value >> 8) & 0xFFF) * 1000.0f);
        break;
    case PH_HW_HSMP_GET_TEMP_MONITOR: // integer [15:8], eighths [7:5]
        PhpHwSmuAddReading(Decode, PhHwNamedSensorSmuTemperature, PhpHwSmuRangeTemperature, PhHwSmuIdSocket, L"Socket", PH_HW_SMU_NO_INDEX, NULL, (FLOAT)((value >> 5) & 0x7FF) * 0.125f);
        break;
    case PH_HW_HSMP_GET_SOCKET_FREQ_LIMIT: // MHz [31:16], source [15:0]
        PhpHwSmuAddReading(Decode, PhHwNamedSensorSmuClockLimit, PhpHwSmuRangeFabricClock, PhHwSmuIdSocketFrequencyLimit, L"Socket frequency limit", PH_HW_SMU_NO_INDEX, NULL, (FLOAT)(value >> 16));
        Decode->FrequencyLimitSources = value & 0xFFFF;
        Decode->FrequencyLimitSourcesValid = TRUE;
        break;
    case PH_HW_HSMP_GET_RAILS_SVI: // mW
        PhpHwSmuAddReading(Decode, PhHwNamedSensorSmuPower, PhpHwSmuRangePower, PhHwSmuIdSviRails, L"SVI rails", PH_HW_SMU_NO_INDEX, NULL, (FLOAT)value / 1000.0f);
        break;
    case PH_HW_HSMP_GET_SOCKET_FMAX_FMIN: // fmax [31:16], fmin [15:0], MHz
        PhpHwSmuAddReading(Decode, PhHwNamedSensorSmuClockLimit, PhpHwSmuRangeFabricClock, PhHwSmuIdFmax, L"Socket Fmax", PH_HW_SMU_NO_INDEX, NULL, (FLOAT)(value >> 16));
        PhpHwSmuAddReading(Decode, PhHwNamedSensorSmuClockLimit, PhpHwSmuRangeFabricClock, PhHwSmuIdFmin, L"Socket Fmin", PH_HW_SMU_NO_INDEX, NULL, (FLOAT)(value & 0xFFFF));
        break;
    }
}
