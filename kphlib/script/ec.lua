--
-- Copyright (c) 2022 Winsider Seminars & Solutions, Inc.  All rights reserved.
--
-- This file is part of System Informer.
--
-- Authors:
--
--     jxy-s   2026
--

--
-- Reads bytes of an ACPI embedded controller through its command and data
-- ports, under the Global Lock when _GLK asks; only the boards listed below
-- switch banks, elsewhere bank 0 is read.
--

local nt = require('nt')

local OBF = 0x01
local IBF = 0x02
local RD_EC = 0x80
local WR_EC = 0x81
local FLOATING = 0xff
local PORT = 2
local WAIT_US = 550
local WAIT_SPINS = 8
local WAIT_MS = 100
local PUT_BACK_ROUNDS = 182
local GLOBAL_LOCK_MS = 100
local EVAL_MS = 100

-- Sensor kinds: temperature in degrees C (signed), voltage in mV, current in A,
-- fan or flow in RPM.
local TEMPERATURE = 1
local VOLTAGE = 2
local CURRENT = 3
local FAN = 4
local POWER = 5

-- A sensor's form: little-endian bytes, and a value of 2^-n of the unit.
local LITTLE = 0x10
local EIGHTHS = 0x03

local BOARD_MANUFACTURER = 'ASUSTeK COMPUTER INC.'
local MSI_MANUFACTURER = 'Micro-Star International'
local MSI_FIRMWARE = 0xa0
local MSI_FIRMWARE_LENGTH = 12
local MSI_CPU = 0x68
local MSI_GPU = 0x80
local BANK_REGISTER = 0xff

-- Each family: sensor -> { kind, size, bank, register, label, form }; size 2 is
-- big-endian unless form is LITTLE.
local FAMILIES = {
  amd_400 = {
    temp_chipset = { TEMPERATURE, 1, 0, 0x3a, 'Chipset' },
    temp_cpu = { TEMPERATURE, 1, 0, 0x3b, 'CPU' },
    temp_mb = { TEMPERATURE, 1, 0, 0x3c, 'Motherboard' },
    temp_t_sensor = { TEMPERATURE, 1, 0, 0x3d, 'T Sensor' },
    temp_vrm = { TEMPERATURE, 1, 0, 0x3e, 'VRM' },
    in_cpu_core = { VOLTAGE, 2, 0, 0xa2, 'CPU Core' },
    fan_cpu_opt = { FAN, 2, 0, 0xbc, 'CPU Optional Fan' },
    fan_vrm_hs = { FAN, 2, 0, 0xb2, 'VRM Heatsink Fan' },
    fan_water_flow = { FAN, 2, 0, 0xb4, 'Water Flow' },
    curr_cpu = { CURRENT, 1, 0, 0xf4, 'CPU' },
    temp_water_in = { TEMPERATURE, 1, 1, 0x0d, 'Water In' },
    temp_water_out = { TEMPERATURE, 1, 1, 0x0b, 'Water Out' },
  },
  amd_500 = {
    temp_chipset = { TEMPERATURE, 1, 0, 0x3a, 'Chipset' },
    temp_cpu = { TEMPERATURE, 1, 0, 0x3b, 'CPU' },
    temp_mb = { TEMPERATURE, 1, 0, 0x3c, 'Motherboard' },
    temp_t_sensor = { TEMPERATURE, 1, 0, 0x3d, 'T Sensor' },
    temp_vrm = { TEMPERATURE, 1, 0, 0x3e, 'VRM' },
    in_cpu_core = { VOLTAGE, 2, 0, 0xa2, 'CPU Core' },
    fan_cpu_opt = { FAN, 2, 0, 0xb0, 'CPU Optional Fan' },
    fan_vrm_hs = { FAN, 2, 0, 0xb2, 'VRM Heatsink Fan' },
    fan_chipset = { FAN, 2, 0, 0xb4, 'Chipset Fan' },
    fan_water_flow = { FAN, 2, 0, 0xbc, 'Water Flow' },
    curr_cpu = { CURRENT, 1, 0, 0xf4, 'CPU' },
    temp_water_in = { TEMPERATURE, 1, 1, 0x00, 'Water In' },
    temp_water_out = { TEMPERATURE, 1, 1, 0x01, 'Water Out' },
    temp_water_block_in = { TEMPERATURE, 1, 1, 0x02, 'Water Block In' },
    temp_water_block_out = { TEMPERATURE, 1, 1, 0x03, 'Water Block Out' },
    temp_t_sensor_2 = { TEMPERATURE, 1, 1, 0x0a, 'T Sensor 2' },
    temp_sensor_extra_1 = { TEMPERATURE, 1, 1, 0x09, 'Extra #1' },
    temp_sensor_extra_2 = { TEMPERATURE, 1, 1, 0x0b, 'Extra #2' },
    temp_sensor_extra_3 = { TEMPERATURE, 1, 1, 0x0c, 'Extra #3' },
  },
  amd_600 = {
    temp_cpu = { TEMPERATURE, 1, 0, 0x30, 'CPU' },
    temp_cpu_package = { TEMPERATURE, 1, 0, 0x31, 'CPU Package' },
    temp_mb = { TEMPERATURE, 1, 0, 0x32, 'Motherboard' },
    temp_t_sensor = { TEMPERATURE, 1, 0, 0x36, 'T Sensor' },
    temp_t_sensor_alt1 = { TEMPERATURE, 1, 0, 0x37, 'T Sensor' },
    temp_vrm = { TEMPERATURE, 1, 0, 0x33, 'VRM' },
    fan_cpu_opt = { FAN, 2, 0, 0xb0, 'CPU Optional Fan' },
    temp_water_in = { TEMPERATURE, 1, 1, 0x00, 'Water In' },
    temp_water_out = { TEMPERATURE, 1, 1, 0x01, 'Water Out' },
  },
  amd_800 = {
    temp_cpu = { TEMPERATURE, 1, 0, 0x30, 'CPU' },
    temp_cpu_package = { TEMPERATURE, 1, 0, 0x31, 'CPU Package' },
    temp_mb = { TEMPERATURE, 1, 0, 0x32, 'Motherboard' },
    temp_t_sensor = { TEMPERATURE, 1, 0, 0x36, 'T Sensor' },
    temp_vrm = { TEMPERATURE, 1, 0, 0x33, 'VRM' },
    temp_vrm_soc = { TEMPERATURE, 1, 4, 0x18, 'VRM SOC' },
    temp_dimm = { TEMPERATURE, 1, 0, 0x55, 'DIMM' },
    in_vrm_core = { VOLTAGE, 2, 1, 0x5a, 'VRM Vcore', EIGHTHS },
    in_vrm_soc = { VOLTAGE, 2, 4, 0x1a, 'VRM SOC', LITTLE },
    power_vrm_core = { POWER, 2, 1, 0x5c, 'VRM Vcore', EIGHTHS },
    power_vrm_soc = { POWER, 2, 4, 0x1e, 'VRM SOC', LITTLE },
    fan_cpu_opt = { FAN, 2, 0, 0xb0, 'CPU Optional Fan' },
  },
  amd_trx_50 = {
    temp_cpu = { TEMPERATURE, 1, 0, 0x30, 'CPU' },
    temp_cpu_package = { TEMPERATURE, 1, 0, 0x31, 'CPU Package' },
    temp_t_sensor = { TEMPERATURE, 1, 1, 0x04, 'T Sensor' },
    temp_vrme = { TEMPERATURE, 1, 0, 0x33, 'VRM East' },
    temp_vrmw = { TEMPERATURE, 1, 0, 0x34, 'VRM West' },
    fan_cpu_opt = { FAN, 2, 0, 0xb0, 'CPU Optional Fan' },
    fan_vrme_hs = { FAN, 2, 0, 0xbc, 'VRM West Heatsink Fan' },
    fan_vrmw_hs = { FAN, 2, 0, 0xb4, 'VRM East Heatsink Fan' },
  },
  amd_wrx_90 = {
    temp_cpu_package = { TEMPERATURE, 1, 0, 0x31, 'CPU Package' },
    temp_t_sensor = { TEMPERATURE, 1, 1, 0x04, 'T Sensor' },
    temp_vrme = { TEMPERATURE, 1, 0, 0x33, 'VRM East' },
    temp_vrmw = { TEMPERATURE, 1, 0, 0x34, 'VRM West' },
    fan_cpu_opt = { FAN, 2, 0, 0xb0, 'CPU Optional Fan' },
    fan_vrme_hs = { FAN, 2, 0, 0xbc, 'VRM East Heatsink Fan' },
    fan_vrmw_hs = { FAN, 2, 0, 0xb4, 'VRM West Heatsink Fan' },
    fan_usb4 = { FAN, 2, 0, 0xb6, 'USB4 Fan' },
    fan_m2 = { FAN, 2, 0, 0xbe, 'M.2 Fan' },
  },
  intel_200 = {
    temp_chipset = { TEMPERATURE, 1, 0, 0x3a, 'Chipset' },
    temp_cpu = { TEMPERATURE, 1, 0, 0x3b, 'CPU' },
    temp_mb = { TEMPERATURE, 1, 0, 0x3c, 'Motherboard' },
    temp_t_sensor = { TEMPERATURE, 1, 0, 0x3d, 'T Sensor' },
    fan_cpu_opt = { FAN, 2, 0, 0xbc, 'CPU Optional Fan' },
  },
  intel_300 = {
    temp_chipset = { TEMPERATURE, 1, 0, 0x3a, 'Chipset' },
    temp_cpu = { TEMPERATURE, 1, 0, 0x3b, 'CPU' },
    temp_mb = { TEMPERATURE, 1, 0, 0x3c, 'Motherboard' },
    temp_t_sensor = { TEMPERATURE, 1, 0, 0x3d, 'T Sensor' },
    temp_vrm = { TEMPERATURE, 1, 0, 0x3e, 'VRM' },
    fan_cpu_opt = { FAN, 2, 0, 0xb0, 'CPU Optional Fan' },
    fan_vrm_hs = { FAN, 2, 0, 0xb2, 'VRM Heatsink Fan' },
    fan_water_flow = { FAN, 2, 0, 0xbc, 'Water Flow' },
    temp_water_in = { TEMPERATURE, 1, 1, 0x00, 'Water In' },
    temp_water_out = { TEMPERATURE, 1, 1, 0x01, 'Water Out' },
  },
  intel_400 = {
    temp_chipset = { TEMPERATURE, 1, 0, 0x3a, 'Chipset' },
    temp_cpu = { TEMPERATURE, 1, 0, 0x3b, 'CPU' },
    temp_mb = { TEMPERATURE, 1, 0, 0x3c, 'Motherboard' },
    temp_t_sensor = { TEMPERATURE, 1, 0, 0x3d, 'T Sensor' },
    temp_vrm = { TEMPERATURE, 1, 0, 0x3e, 'VRM' },
    fan_cpu_opt = { FAN, 2, 0, 0xb0, 'CPU Optional Fan' },
    fan_vrm_hs = { FAN, 2, 0, 0xb2, 'VRM Heatsink Fan' },
  },
  intel_600 = {
    temp_t_sensor = { TEMPERATURE, 1, 0, 0x3d, 'T Sensor' },
    temp_vrm = { TEMPERATURE, 1, 0, 0x3e, 'VRM' },
    fan_cpu_opt = { FAN, 2, 0, 0xb0, 'CPU Optional Fan' },
    fan_water_flow = { FAN, 2, 0, 0xbe, 'Water Flow' },
    temp_water_in = { TEMPERATURE, 1, 1, 0x00, 'Water In' },
    temp_water_out = { TEMPERATURE, 1, 1, 0x01, 'Water Out' },
    temp_water_block_in = { TEMPERATURE, 1, 1, 0x02, 'Water Block In' },
  },
  intel_700 = {
    temp_t_sensor = { TEMPERATURE, 1, 1, 0x09, 'T Sensor' },
    temp_vrm = { TEMPERATURE, 1, 0, 0x33, 'VRM' },
    fan_cpu_opt = { FAN, 2, 0, 0xb0, 'CPU Optional Fan' },
    fan_water_flow = { FAN, 2, 0, 0xbc, 'Water Flow' },
    temp_water_in = { TEMPERATURE, 1, 1, 0x00, 'Water In' },
    temp_water_out = { TEMPERATURE, 1, 1, 0x01, 'Water Out' },
    temp_t_sensor_2 = { TEMPERATURE, 1, 1, 0x05, 'T Sensor 2' },
  },
}

-- Each board: the SMBIOS baseboard products, the family and the sensors it has.
local BOARDS = {
  {
    products = {
      'ROG CROSSHAIR VIII DARK HERO',
    },
    family = 'amd_500',
    sensors = {
      'temp_chipset',
      'temp_cpu',
      'temp_mb',
      'temp_t_sensor',
      'temp_vrm',
      'in_cpu_core',
      'fan_cpu_opt',
      'fan_water_flow',
      'curr_cpu',
      'temp_water_in',
      'temp_water_out',
    },
  },
  {
    products = {
      'ROG CROSSHAIR VIII FORMULA',
      'ROG CROSSHAIR VIII HERO',
      'ROG CROSSHAIR VIII HERO (WI-FI)',
    },
    family = 'amd_500',
    sensors = {
      'temp_chipset',
      'temp_cpu',
      'temp_mb',
      'temp_t_sensor',
      'temp_vrm',
      'in_cpu_core',
      'fan_cpu_opt',
      'fan_chipset',
      'fan_water_flow',
      'curr_cpu',
      'temp_water_in',
      'temp_water_out',
    },
  },
  {
    products = {
      'ROG CROSSHAIR VIII IMPACT',
    },
    family = 'amd_500',
    sensors = {
      'temp_chipset',
      'temp_cpu',
      'temp_mb',
      'temp_t_sensor',
      'temp_vrm',
      'in_cpu_core',
      'fan_chipset',
      'curr_cpu',
    },
  },
  {
    products = {
      'ROG CROSSHAIR X670E EXTREME',
    },
    family = 'amd_600',
    sensors = {
      'temp_cpu',
      'temp_cpu_package',
      'temp_mb',
      'temp_t_sensor',
      'temp_vrm',
      'temp_water_in',
      'temp_water_out',
    },
  },
  {
    products = {
      'ROG CROSSHAIR X670E GENE',
    },
    family = 'amd_600',
    sensors = {
      'temp_cpu',
      'temp_cpu_package',
      'temp_mb',
      'temp_t_sensor',
      'temp_vrm',
    },
  },
  {
    products = {
      'ROG CROSSHAIR X670E HERO',
    },
    family = 'amd_600',
    sensors = {
      'temp_cpu',
      'temp_cpu_package',
      'temp_mb',
      'temp_vrm',
      'temp_water_in',
      'temp_water_out',
    },
  },
  {
    products = {
      'ROG CROSSHAIR X870E APEX',
    },
    family = 'amd_800',
    sensors = {
      'temp_vrm',
      'temp_vrm_soc',
      'temp_dimm',
      'in_vrm_core',
      'in_vrm_soc',
      'power_vrm_core',
      'power_vrm_soc',
      'fan_cpu_opt',
    },
  },
  {
    products = {
      'ROG CROSSHAIR X870E HERO',
    },
    family = 'amd_800',
    sensors = {
      'temp_cpu',
      'temp_cpu_package',
      'temp_mb',
      'temp_t_sensor',
      'temp_vrm',
      'fan_cpu_opt',
    },
  },
  {
    products = {
      'MAXIMUS VI HERO',
    },
    family = 'intel_300',
    sensors = {
      'temp_chipset',
      'temp_cpu',
      'temp_mb',
      'temp_t_sensor',
      'temp_vrm',
      'fan_cpu_opt',
      'fan_water_flow',
      'temp_water_in',
      'temp_water_out',
    },
  },
  {
    products = {
      'ROG MAXIMUS X HERO',
    },
    family = 'intel_300',
    sensors = {
      'temp_chipset',
      'temp_cpu',
      'temp_mb',
      'temp_t_sensor',
      'temp_vrm',
      'fan_cpu_opt',
    },
  },
  {
    products = {
      'ROG MAXIMUS XI HERO',
      'ROG MAXIMUS XI HERO (WI-FI)',
    },
    family = 'intel_300',
    sensors = {
      'temp_chipset',
      'temp_cpu',
      'temp_mb',
      'temp_t_sensor',
      'temp_vrm',
      'fan_cpu_opt',
      'fan_water_flow',
      'temp_water_in',
      'temp_water_out',
    },
  },
  {
    products = {
      'ROG MAXIMUS Z690 FORMULA',
    },
    family = 'intel_600',
    sensors = {
      'temp_t_sensor',
      'temp_vrm',
      'fan_water_flow',
      'temp_water_in',
      'temp_water_out',
    },
  },
  {
    products = {
      'ROG MAXIMUS Z790 EXTREME',
    },
    family = 'intel_700',
    sensors = {
      'temp_t_sensor',
      'temp_vrm',
      'fan_water_flow',
      'temp_water_in',
      'temp_water_out',
    },
  },
  {
    products = {
      'ROG MAXIMUS Z790 HERO',
    },
    family = 'intel_700',
    sensors = {
      'temp_t_sensor',
      'temp_vrm',
      'fan_cpu_opt',
      'fan_water_flow',
      'temp_water_in',
      'temp_water_out',
    },
  },
  {
    products = {
      'PRIME X470-PRO',
    },
    family = 'amd_400',
    sensors = {
      'temp_chipset',
      'temp_cpu',
      'temp_mb',
      'temp_t_sensor',
      'temp_vrm',
      'in_cpu_core',
      'fan_cpu_opt',
      'curr_cpu',
    },
  },
  {
    products = {
      'PRIME X570-PRO',
    },
    family = 'amd_500',
    sensors = {
      'temp_chipset',
      'temp_cpu',
      'temp_mb',
      'temp_t_sensor',
      'temp_vrm',
      'fan_chipset',
    },
  },
  {
    products = {
      'PRIME X670E-PRO WIFI',
    },
    family = 'amd_600',
    sensors = {
      'temp_cpu',
      'temp_cpu_package',
      'temp_mb',
      'temp_t_sensor_alt1',
      'temp_vrm',
      'fan_cpu_opt',
    },
  },
  {
    products = {
      'PRIME Z270-A',
    },
    family = 'intel_200',
    sensors = {
      'temp_chipset',
      'temp_cpu',
      'temp_mb',
      'temp_t_sensor',
      'fan_cpu_opt',
    },
  },
  {
    products = {
      'ProArt B550-CREATOR',
    },
    family = 'amd_500',
    sensors = {
      'temp_chipset',
      'temp_cpu',
      'temp_mb',
      'temp_t_sensor',
      'fan_cpu_opt',
    },
  },
  {
    products = {
      'ProArt X570-CREATOR WIFI',
    },
    family = 'amd_500',
    sensors = {
      'temp_chipset',
      'temp_cpu',
      'temp_mb',
      'temp_t_sensor',
      'temp_vrm',
      'in_cpu_core',
      'fan_cpu_opt',
      'curr_cpu',
    },
  },
  {
    products = {
      'ProArt X670E-CREATOR WIFI',
    },
    family = 'amd_600',
    sensors = {
      'temp_cpu',
      'temp_cpu_package',
      'temp_mb',
      'temp_t_sensor',
      'temp_vrm',
    },
  },
  {
    products = {
      'ProArt X870E-CREATOR WIFI',
    },
    family = 'amd_800',
    sensors = {
      'temp_cpu',
      'temp_cpu_package',
      'temp_mb',
      'temp_t_sensor',
      'temp_vrm',
      'fan_cpu_opt',
    },
  },
  {
    products = {
      'ProArt Z690-CREATOR WIFI',
    },
    family = 'intel_600',
    sensors = {
      'temp_t_sensor',
      'temp_vrm',
      'fan_cpu_opt',
    },
  },
  {
    products = {
      'Pro WS TRX50-SAGE WIFI',
      'Pro WS TRX50-SAGE WIFI A',
    },
    family = 'amd_trx_50',
    sensors = {
      'temp_cpu',
      'temp_cpu_package',
      'temp_t_sensor',
      'temp_vrme',
      'temp_vrmw',
      'fan_cpu_opt',
      'fan_vrme_hs',
      'fan_vrmw_hs',
    },
  },
  {
    products = {
      'Pro WS WRX90E-SAGE SE',
    },
    family = 'amd_wrx_90',
    sensors = {
      'temp_cpu_package',
      'temp_t_sensor',
      'temp_vrme',
      'temp_vrmw',
      'fan_cpu_opt',
      'fan_vrme_hs',
      'fan_vrmw_hs',
      'fan_usb4',
      'fan_m2',
    },
  },
  {
    products = {
      'Pro WS X570-ACE',
    },
    family = 'amd_500',
    sensors = {
      'temp_chipset',
      'temp_cpu',
      'temp_mb',
      'temp_t_sensor',
      'temp_vrm',
      'in_cpu_core',
      'fan_chipset',
      'curr_cpu',
    },
  },
  {
    products = {
      'ROG STRIX B550-E GAMING',
    },
    family = 'amd_500',
    sensors = {
      'temp_chipset',
      'temp_cpu',
      'temp_mb',
      'temp_t_sensor',
      'temp_vrm',
      'fan_cpu_opt',
    },
  },
  {
    products = {
      'ROG STRIX B550-I GAMING',
    },
    family = 'amd_500',
    sensors = {
      'temp_chipset',
      'temp_cpu',
      'temp_mb',
      'temp_t_sensor',
      'temp_vrm',
      'in_cpu_core',
      'fan_vrm_hs',
      'curr_cpu',
    },
  },
  {
    products = {
      'ROG STRIX B650E-E GAMING WIFI',
    },
    family = 'amd_600',
    sensors = {
      'temp_cpu',
      'temp_cpu_package',
      'temp_mb',
      'temp_vrm',
      'fan_cpu_opt',
    },
  },
  {
    products = {
      'ROG STRIX B650E-I GAMING WIFI',
    },
    family = 'amd_600',
    sensors = {
      'temp_cpu',
      'temp_mb',
      'temp_t_sensor',
      'temp_vrm',
    },
  },
  {
    products = {
      'ROG STRIX B850-E GAMING WIFI',
    },
    family = 'amd_800',
    sensors = {
      'temp_cpu',
      'temp_cpu_package',
      'temp_mb',
      'temp_t_sensor',
      'temp_vrm',
      'fan_cpu_opt',
    },
  },
  {
    products = {
      'ROG STRIX B850-I GAMING WIFI',
    },
    family = 'amd_800',
    sensors = {
      'temp_cpu',
      'temp_cpu_package',
      'temp_mb',
      'temp_vrm',
    },
  },
  {
    products = {
      'ROG STRIX X470-F GAMING',
    },
    family = 'amd_400',
    sensors = {
      'temp_chipset',
      'temp_cpu',
      'temp_mb',
      'temp_t_sensor',
      'in_cpu_core',
      'fan_cpu_opt',
      'curr_cpu',
    },
  },
  {
    products = {
      'ROG STRIX X470-I GAMING',
    },
    family = 'amd_400',
    sensors = {
      'temp_chipset',
      'temp_cpu',
      'temp_mb',
      'temp_t_sensor',
      'temp_vrm',
      'in_cpu_core',
      'curr_cpu',
    },
  },
  {
    products = {
      'ROG STRIX X570-E GAMING',
    },
    family = 'amd_500',
    sensors = {
      'temp_chipset',
      'temp_cpu',
      'temp_mb',
      'temp_t_sensor',
      'in_cpu_core',
      'fan_chipset',
      'curr_cpu',
    },
  },
  {
    products = {
      'ROG STRIX X570-E GAMING WIFI II',
    },
    family = 'amd_500',
    sensors = {
      'temp_chipset',
      'temp_cpu',
      'temp_mb',
      'temp_t_sensor',
      'in_cpu_core',
      'curr_cpu',
    },
  },
  {
    products = {
      'ROG STRIX X570-F GAMING',
    },
    family = 'amd_500',
    sensors = {
      'temp_chipset',
      'temp_cpu',
      'temp_mb',
      'temp_t_sensor',
      'fan_chipset',
    },
  },
  {
    products = {
      'ROG STRIX X570-I GAMING',
    },
    family = 'amd_500',
    sensors = {
      'temp_chipset',
      'temp_t_sensor',
      'temp_vrm',
      'in_cpu_core',
      'fan_vrm_hs',
      'fan_chipset',
      'curr_cpu',
    },
  },
  {
    products = {
      'ROG STRIX X670E-A GAMING WIFI',
      'ROG STRIX X670E-E GAMING WIFI',
    },
    family = 'amd_600',
    sensors = {
      'temp_cpu',
      'temp_cpu_package',
      'temp_mb',
      'temp_vrm',
    },
  },
  {
    products = {
      'ROG STRIX X670E-I GAMING WIFI',
    },
    family = 'amd_600',
    sensors = {
      'temp_cpu',
      'temp_cpu_package',
      'temp_mb',
      'temp_vrm',
    },
  },
  {
    products = {
      'ROG STRIX X870-F GAMING WIFI',
    },
    family = 'amd_800',
    sensors = {
      'temp_cpu',
      'temp_cpu_package',
      'temp_mb',
      'temp_t_sensor',
      'temp_vrm',
    },
  },
  {
    products = {
      'ROG STRIX X870-I GAMING WIFI',
    },
    family = 'amd_800',
    sensors = {
      'temp_cpu',
      'temp_cpu_package',
      'temp_mb',
      'temp_vrm',
    },
  },
  {
    products = {
      'ROG STRIX X870E-E GAMING WIFI',
      'ROG STRIX X870E-E GAMING WIFI7 R2',
    },
    family = 'amd_800',
    sensors = {
      'temp_cpu',
      'temp_cpu_package',
      'temp_mb',
      'temp_t_sensor',
      'temp_vrm',
      'fan_cpu_opt',
    },
  },
  {
    products = {
      'ROG STRIX X870E-H GAMING WIFI7',
    },
    family = 'amd_800',
    sensors = {
      'temp_cpu',
      'temp_cpu_package',
      'temp_mb',
      'temp_t_sensor',
      'temp_vrm',
      'fan_cpu_opt',
    },
  },
  {
    products = {
      'ROG STRIX Z390-E GAMING',
      'ROG STRIX Z390-F GAMING',
    },
    family = 'intel_300',
    sensors = {
      'temp_chipset',
      'temp_t_sensor',
      'temp_vrm',
      'fan_cpu_opt',
    },
  },
  {
    products = {
      'ROG STRIX Z490-A GAMING',
      'ROG STRIX Z490-F GAMING',
    },
    family = 'intel_400',
    sensors = {
      'temp_chipset',
      'temp_cpu',
      'temp_mb',
      'temp_t_sensor',
      'temp_vrm',
      'fan_cpu_opt',
      'fan_vrm_hs',
    },
  },
  {
    products = {
      'ROG STRIX Z690-A GAMING WIFI D4',
    },
    family = 'intel_600',
    sensors = {
      'temp_t_sensor',
      'temp_vrm',
    },
  },
  {
    products = {
      'ROG STRIX Z690-E GAMING WIFI',
    },
    family = 'intel_600',
    sensors = {
      'temp_t_sensor',
      'temp_vrm',
    },
  },
  {
    products = {
      'ROG STRIX Z790-E GAMING WIFI II',
    },
    family = 'intel_700',
    sensors = {
      'temp_t_sensor',
      'temp_vrm',
      'fan_cpu_opt',
    },
  },
  {
    products = {
      'ROG STRIX Z790-H GAMING WIFI',
    },
    family = 'intel_700',
    sensors = {
      'temp_t_sensor',
      'temp_vrm',
    },
  },
  {
    products = {
      'ROG STRIX Z790-I GAMING WIFI',
    },
    family = 'intel_700',
    sensors = {
      'temp_t_sensor',
      'temp_vrm',
      'temp_t_sensor_2',
    },
  },
  {
    products = {
      'TUF GAMING X670E-PLUS',
      'TUF GAMING X670E-PLUS WIFI',
    },
    family = 'amd_600',
    sensors = {
      'temp_cpu',
      'temp_cpu_package',
      'temp_mb',
      'temp_vrm',
      'fan_cpu_opt',
      'temp_water_in',
      'temp_water_out',
    },
  },
  {
    products = {
      'ROG ZENITH II EXTREME',
      'ROG ZENITH II EXTREME ALPHA',
    },
    family = 'amd_500',
    sensors = {
      'temp_chipset',
      'temp_cpu',
      'temp_mb',
      'temp_t_sensor',
      'temp_vrm',
      'in_cpu_core',
      'fan_cpu_opt',
      'fan_vrm_hs',
      'fan_chipset',
      'fan_water_flow',
      'curr_cpu',
      'temp_water_in',
      'temp_water_out',
      'temp_water_block_in',
      'temp_water_block_out',
      'temp_t_sensor_2',
      'temp_sensor_extra_1',
      'temp_sensor_extra_2',
      'temp_sensor_extra_3',
    },
  },
}

-- MSI laptop firmware string -> whether the GPU temperature is at MSI_GPU.
local MSI_FIRMWARES = {
  ['14C1EMS1.012'] = true,
  ['14C1EMS1.101'] = true,
  ['14C1EMS1.102'] = true,
  ['17F2EMS1.103'] = true,
  ['17F2EMS1.104'] = true,
  ['17F2EMS1.106'] = true,
  ['17F2EMS1.107'] = true,
  ['16U8EMS2.100'] = true,
  ['1552EMS1.118'] = true,
  ['1592EMS1.111'] = true,
  ['16V4EMS1.114'] = true,
  ['158LEMS1.103'] = false,
  ['158LEMS1.105'] = false,
  ['158LEMS1.106'] = false,
  ['1542EMS1.102'] = true,
  ['1542EMS1.104'] = true,
  ['17FKEMS1.108'] = false,
  ['17FKEMS1.109'] = false,
  ['17FKEMS1.10A'] = false,
  ['14F1EMS1.115'] = false,
  ['14JKEMS1.104'] = false,
  ['1582EMS1.107'] = true,
  ['1583EMS1.109'] = true,
  ['1585EMS1.115'] = true,
  ['16S6EMS1.111'] = false,
  ['1552EMS1.115'] = false,
  ['16R6EMS1.104'] = false,
  ['1594EMS1.109'] = true,
  ['17L1EMS1.107'] = true,
  ['1571EMS1.106'] = true,
  ['1824EMS1.108'] = true,
  ['182LIMS1.108'] = true,
  ['182LIMS1.111'] = true,
  ['182KIMS1.113'] = true,
}

local ANY_BOARD = { allow = { { 0, 0x00, 0xff } } }

-- Kept between calls: the board's row and each controller.
local kept = {}

-- The manufacturer and product strings of the first SMBIOS baseboard structure,
-- or nil.
local function baseboard_strings()
  local smbios = ksi.hw.firmware_table('RSMB', 0)
  if not smbios or #smbios < 8 then
    return nil
  end
  local last = math.min(#smbios, 8 + string.unpack('<I4', smbios, 5))
  local at = 9
  while at + 3 <= last do
    local kind, length = string.byte(smbios, at, at + 1)
    if length < 4 or at + length - 1 > last then
      return nil
    end
    local ends = string.find(smbios, '\0\0', at + length, true)
    if not ends or ends > last then
      return nil
    end
    if kind == 2 then
      if length < 6 then
        return nil
      end
      local strings = {}
      local i = at + length
      while i < ends + 1 and string.byte(smbios, i) ~= 0 do
        local stop = string.find(smbios, '\0', i, true)
        strings[#strings + 1] = string.sub(smbios, i, stop - 1)
        i = stop + 1
      end
      local manufacturer, product = string.byte(smbios, at + 4, at + 5)
      return strings[manufacturer] or '', strings[product] or ''
    end
    if kind == 127 then
      return nil
    end
    at = ends + 2
  end
  return nil
end

local msi_row

-- This board's row, ANY_BOARD when unlisted, or nil and a status when the MSI
-- firmware string is unreadable.
local function board()
  if kept.board then
    return kept.board
  end
  local row = ANY_BOARD
  local manufacturer, product = baseboard_strings()
  if manufacturer and string.sub(manufacturer, 1, #MSI_MANUFACTURER) == MSI_MANUFACTURER then
    local failed
    row, failed = msi_row()
    if not row then
      return nil, failed
    end
  elseif manufacturer == BOARD_MANUFACTURER then
    for _, b in ipairs(BOARDS) do
      for _, p in ipairs(b.products) do
        if product == p then
          local family = FAMILIES[b.family]
          row = { bank_register = BANK_REGISTER, sensors = {}, allow = {} }
          for _, name in ipairs(b.sensors) do
            local e = family[name]
            row.sensors[#row.sensors + 1] = e
            row.allow[#row.allow + 1] = { e[3], e[4], e[4] + e[2] - 1 }
          end
        end
      end
    end
  end
  if not ksi.volatile then
    kept.board = row
  end
  return row
end

local function permitted(row, bank, reg)
  for _, a in ipairs(row.allow) do
    if bank == a[1] and reg >= a[2] and reg <= a[3] then
      return true
    end
  end
  return false
end

-- Waits until the status masked is want: spins, then sleeps, within WAIT_MS.
local function wait(s, mask, want)
  local limit = ksi.time() + WAIT_MS * 10000
  local round = 0
  while true do
    local status = ksi.hw.port_read(s.command, 1)
    if status == FLOATING then
      ksi.trace('EC status reads 0xff')
      return nt.STATUS_DEVICE_NOT_READY
    end
    if (status & mask) == want then
      return nt.STATUS_SUCCESS
    end
    if ksi.time() >= limit then
      ksi.trace('EC status 0x%02x, waiting for 0x%02x under 0x%02x', status, want, mask)
      return nt.STATUS_IO_TIMEOUT
    end
    if round < WAIT_SPINS then
      ksi.stall(WAIT_US)
    else
      ksi.sleep(1)
    end
    round = round + 1
  end
end

local function ec_read(s, address)
  local st = wait(s, IBF, 0)
  if st ~= nt.STATUS_SUCCESS then
    return st, 0
  end
  ksi.hw.port_write(s.command, 1, RD_EC)
  st = wait(s, IBF, 0)
  if st ~= nt.STATUS_SUCCESS then
    return st, 0
  end
  ksi.hw.port_write(s.data, 1, address)
  st = wait(s, OBF, OBF)
  if st ~= nt.STATUS_SUCCESS then
    return st, 0
  end
  return nt.STATUS_SUCCESS, ksi.hw.port_read(s.data, 1)
end

local function ec_write(s, address, value)
  for _, step in ipairs({ { s.command, WR_EC }, { s.data, address }, { s.data, value } }) do
    local st = wait(s, IBF, 0)
    if st ~= nt.STATUS_SUCCESS then
      return st
    end
    ksi.hw.port_write(step[1], 1, step[2])
  end
  return wait(s, IBF, 0)
end

-- Puts the bank back from a close handler: counted waits, since a time limit
-- may have ended the call.
local function put_back(s, address, value)
  for _, step in ipairs({ { s.command, WR_EC }, { s.data, address }, { s.data, value } }) do
    local empty = false
    for _ = 1, PUT_BACK_ROUNDS do
      local status = ksi.hw.port_read(s.command, 1)
      if status ~= FLOATING and (status & IBF) == 0 then
        empty = true
        break
      end
      ksi.stall(WAIT_US)
    end
    if not empty then
      ksi.trace('EC input buffer full in the put-back')
      return
    end
    ksi.hw.port_write(step[1], 1, step[2])
  end
end

-- Whether the controller asks for the Global Lock: its _GLK, 0 when it has
-- none.
local function global_lock_wanted(device)
  local output, failed = nt.acpi.eval(device, '_GLK', EVAL_MS)
  if not output then
    if failed == nt.STATUS_OBJECT_NAME_NOT_FOUND then
      return false
    end
    ksi.trace('_GLK failed: 0x%08x', failed)
    return nil, failed
  end
  if string.unpack('<I4', output, 9) < 1 then
    return nil, nt.STATUS_ACPI_INVALID_DATA
  end
  local kind, length = string.unpack('<I2I2', output, 13)
  if kind ~= 0 or (length ~= 4 and length ~= 8) or #output < 16 + length then
    ksi.trace('_GLK is not an integer')
    return nil, nt.STATUS_ACPI_INVALID_DATA
  end
  return string.unpack('<I4', output, 17) ~= 0
end

-- The Global Lock's guard, false when the controller does not want it, or nil
-- and a status.
local function global_lock(s)
  if not s.glk then
    return false
  end
  return nt.acpi.global_lock(s.device, GLOBAL_LOCK_MS)
end

-- The controller at a PNP0C09 ordinal: device, ports and whether it wants the
-- Global Lock; kept between calls.
local function controller(ordinal)
  local key = 'ec' .. ordinal
  if kept[key] then
    return kept[key]
  end
  local device, missing = nt.acpi.find('PNP0C09', ordinal)
  if not device then
    ksi.trace('no PNP0C09 %d: 0x%x', ordinal, missing)
    return nil, missing
  end
  local resources, failed = nt.resources(device)
  if not resources then
    device:release()
    ksi.trace('resources not read: 0x%x', failed)
    return nil, failed
  end
  local ports = {}
  for _, r in ipairs(resources) do
    if r.kind == PORT then
      ports[#ports + 1] = r
    end
  end
  if #ports < 2 or ports[1].translated > 0xffff or ports[2].translated > 0xffff then
    device:release()
    ksi.trace('%d port descriptors', #ports)
    return nil, nt.STATUS_DEVICE_CONFIGURATION_ERROR
  end
  local glk, refused = global_lock_wanted(device)
  if glk == nil then
    device:release()
    return nil, refused
  end
  local s =
    { device = device, data = ports[1].translated, command = ports[2].translated, glk = glk }
  if not ksi.volatile then
    kept[key] = s
  end
  return s
end

-- Calls fn under the Global Lock (when wanted) and the isa lock; returns fn's
-- results or the refusing status.
local function with_locks(s, fn)
  local held <close>, refusal = global_lock(s)
  if held == nil then
    ksi.trace('the Global Lock was not taken: 0x%08x', refusal)
    return refusal
  end
  local lock <close>, failed = ksi.lock('isa')
  if not lock then
    ksi.trace('the isa lock was not taken: 0x%08x', failed)
    return failed
  end
  if (ksi.hw.port_read(s.command, 1) & OBF) ~= 0 then
    ksi.trace('the EC holds a byte of another transaction')
    return nt.STATUS_DEVICE_BUSY
  end
  return fn()
end

-- An MSI laptop's row by firmware string, ANY_BOARD when no controller or
-- unlisted, or nil and a status.
msi_row = function()
  local s, missing = controller(0)
  if not s then
    ksi.trace('no controller: 0x%08x', missing)
    return ANY_BOARD
  end
  local release <close> = setmetatable({}, {
    __close = function()
      if ksi.volatile then
        s.device:release()
      end
    end,
  })
  local bytes = {}
  local status = with_locks(s, function()
    for i = 0, MSI_FIRMWARE_LENGTH - 1 do
      local st, value = ec_read(s, MSI_FIRMWARE + i)
      if st ~= nt.STATUS_SUCCESS then
        return st
      end
      bytes[#bytes + 1] = string.char(value)
    end
    return nt.STATUS_SUCCESS
  end)
  if status ~= nt.STATUS_SUCCESS then
    return nil, status
  end
  local gpu = MSI_FIRMWARES[table.concat(bytes)]
  if gpu == nil then
    ksi.trace('firmware string not listed')
    return ANY_BOARD
  end
  local row = {
    sensors = { { TEMPERATURE, 1, 0, MSI_CPU, 'CPU' } },
    allow = { { 0, MSI_CPU, MSI_CPU } },
  }
  if gpu then
    row.sensors[2] = { TEMPERATURE, 1, 0, MSI_GPU, 'GPU' }
    row.allow[2] = { 0, MSI_GPU, MSI_GPU }
  end
  return row
end

-- Input <I4 ordinal; output <I4I4I4 data port, command port, flags (1 Global
-- Lock taken, 2 board has banks).
function identify(input)
  if not ksi.hw.port_read then
    return nt.STATUS_NOT_SUPPORTED
  end
  if #input ~= 4 then
    return nt.STATUS_INVALID_PARAMETER
  end
  local s, refused = controller(string.unpack('<I4', input))
  if not s then
    return refused
  end
  local row = board()
  local flags = (s.glk and 1 or 0) | (row and row.bank_register and 2 or 0)
  local output = string.pack('<I4I4I4', s.data, s.command, flags)
  if ksi.volatile then
    s.device:release()
  end
  return nt.STATUS_SUCCESS, output
end

-- Output <I1I1I1I1I1s1 kind, size, bank, register, form, label per sensor;
-- STATUS_NOT_FOUND on an unlisted board.
function sensors(input)
  if #input ~= 0 then
    return nt.STATUS_INVALID_PARAMETER
  end
  local row, failed = board()
  if not row then
    return failed
  end
  if not row.sensors then
    return nt.STATUS_NOT_FOUND
  end
  local output = {}
  for i, e in ipairs(row.sensors) do
    output[i] = string.pack('<I1I1I1I1I1s1', e[1], e[2], e[3], e[4], e[6] or 0, e[5])
  end
  return nt.STATUS_SUCCESS, table.concat(output)
end

-- Input <I4 ordinal, then <I4I4 bank, register per item; output <i4I4 status,
-- value per item; restores the bank.
function read(input)
  if not ksi.hw.port_read then
    return nt.STATUS_NOT_SUPPORTED
  end
  if #input < 12 or (#input - 4) % 8 ~= 0 then
    ksi.trace('input of %d bytes', #input)
    return nt.STATUS_INVALID_PARAMETER
  end
  local row, unread = board()
  if not row then
    return unread
  end
  local count = (#input - 4) // 8
  local items = {}
  local any = false
  for i = 1, count do
    local bank, reg = string.unpack('<I4I4', input, 5 + (i - 1) * 8)
    items[i] = { bank = bank, reg = reg, allowed = permitted(row, bank, reg) }
    any = any or items[i].allowed
  end
  local output = {}
  if not any then
    for i = 1, count do
      output[i] = string.pack('<i4I4', nt.STATUS_ACCESS_DENIED, 0)
    end
    return nt.STATUS_SUCCESS, table.concat(output)
  end
  local s, refused = controller(string.unpack('<I4', input))
  if not s then
    return refused
  end
  local release <close> = setmetatable({}, {
    __close = function()
      if ksi.volatile then
        s.device:release()
      end
    end,
  })
  return with_locks(s, function()
    local c = {}
    local bank <close> = setmetatable({}, {
      __close = function()
        if c.changed then
          put_back(s, row.bank_register, c.orig_bank)
        end
      end,
    })
    local stopped = false
    for i, item in ipairs(items) do
      local status, value = nt.STATUS_ACCESS_DENIED, 0
      if item.allowed and stopped then
        status = nt.STATUS_CANCELLED
      elseif item.allowed then
        status = nt.STATUS_SUCCESS
        if row.bank_register then
          if not c.orig_bank then
            status, c.orig_bank = ec_read(s, row.bank_register)
            if status ~= nt.STATUS_SUCCESS then
              c.orig_bank = nil
            end
            c.bank = c.orig_bank
          end
          if status == nt.STATUS_SUCCESS and item.bank ~= c.bank then
            c.changed = true
            status = ec_write(s, row.bank_register, item.bank)
            if status == nt.STATUS_SUCCESS then
              c.bank = item.bank
            end
          end
        end
        if status == nt.STATUS_SUCCESS then
          status, value = ec_read(s, item.reg)
        end
        stopped = status ~= nt.STATUS_SUCCESS
      end
      output[i] = string.pack('<i4I4', status, value)
    end
    return nt.STATUS_SUCCESS, table.concat(output)
  end)
end
