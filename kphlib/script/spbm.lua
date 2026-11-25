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
-- Reads the power budget page of the NVIDIA N1X: words the SoC's power
-- firmware keeps current with its rails' power and energy, its temperatures
-- and its power limits. A part is found by processor 0's MIDR and by the DSDT
-- reading the page itself; the page of a part that is not listed is never
-- mapped. Nothing is written.
--

local nt = require('nt')

local MIDR = 0x4000
local PAGE = 0x1000
local ADDRESS_BITS = 48

-- The channels of KPH_HW_SPBM_SAMPLE, in its order.
local TEMPERATURE = {
  hottest = 0,
  soc = 1,
  cluster0_e = 2,
  cluster0_p = 3,
  cluster1_e = 4,
  cluster1_p = 5,
  gpu = 6,
  uncore = 7,
  dla = 8,
}
local TEMPERATURE_MAX = 9

local POWER = {
  system = 0,
  package = 1,
  cpu_gpu = 2,
  cpu_p = 3,
  cpu_e = 4,
  rail_a = 5,
  dc_input = 6,
  gpu = 7,
  rail_b = 8,
  package_average = 9,
  system_average = 10,
  system_short_average = 11,
}
local POWER_MAX = 12

local ENERGY = {
  package = 0,
  cpu_e = 1,
  cpu_p = 2,
  gpu = 3,
  system = 4,
}
local ENERGY_MAX = 5

local LIMIT = {
  pl1 = 0,
  pl2 = 1,
  pl1_request = 2,
  pl2_request = 3,
}
local LIMIT_MAX = 4

-- A temperature word is tenths of a kelvin ('dk') or whole degrees Celsius
-- ('c'); powers and limits are milliwatts, energies millijoules.
local ROWS = {
  {
    implementer = 0x41,
    parts = { 0xd85, 0xd87 },
    base = 0x1c238000,
    probe = 0x818,
    temperatures = {
      [TEMPERATURE.hottest] = { 0x81c, 'c' },
      [TEMPERATURE.soc] = { 0x818, 'dk' },
      [TEMPERATURE.cluster0_e] = { 0x820, 'dk' },
      [TEMPERATURE.cluster0_p] = { 0x824, 'dk' },
      [TEMPERATURE.cluster1_e] = { 0x828, 'dk' },
      [TEMPERATURE.cluster1_p] = { 0x82c, 'dk' },
      [TEMPERATURE.gpu] = { 0x830, 'dk' },
      [TEMPERATURE.uncore] = { 0x834, 'dk' },
      [TEMPERATURE.dla] = { 0x838, 'dk' },
    },
    powers = {
      [POWER.system] = 0x300,
      [POWER.package] = 0x304,
      [POWER.cpu_gpu] = 0x308,
      [POWER.cpu_p] = 0x30c,
      [POWER.cpu_e] = 0x310,
      [POWER.rail_a] = 0x314,
      [POWER.dc_input] = 0x31c,
      [POWER.gpu] = 0x324,
      [POWER.rail_b] = 0x338,
      [POWER.package_average] = 0x800,
      [POWER.system_average] = 0x80c,
      [POWER.system_short_average] = 0x810,
    },
    energies = {
      [ENERGY.package] = 0x344,
      [ENERGY.cpu_e] = 0x350,
      [ENERGY.cpu_p] = 0x35c,
      [ENERGY.gpu] = 0x374,
      [ENERGY.system] = 0x3ac,
    },
    limits = {
      [LIMIT.pl1] = 0x160,
      [LIMIT.pl2] = 0x164,
      [LIMIT.pl1_request] = 0x100,
      [LIMIT.pl2_request] = 0x104,
    },
  },
}

local kept = {}

-- Processor 0's MIDR, or nil.
local function midr()
  local list = ksi.hw.processors()
  if #list < 4 then
    return nil
  end
  local group, number = string.unpack('<I2I2', list)
  local results = ksi.hw.sysreg_read(string.pack('<I2I2I4', group, number, MIDR))
  if not results then
    return nil
  end
  local status, _, value = string.unpack('<i4I4I8', results)
  if status ~= nt.STATUS_SUCCESS then
    return nil
  end
  return value
end

-- The row of this part: from the tables once in a kept script, nil when there
-- is none.
local function part()
  if kept.part ~= nil then
    return kept.part or nil
  end
  local found = false
  local value = midr()
  local dsdt = ksi.hw.firmware_table('ACPI', 'DSDT')
  if value and dsdt and #dsdt > 36 then
    local implementer = (value >> 24) & 0xff
    local number = (value >> 4) & 0xfff
    for _, row in ipairs(ROWS) do
      if row.implementer == implementer then
        for _, p in ipairs(row.parts) do
          if p == number then
            -- The DSDT's own thermal zone reads this word: a DWordConst of it.
            local word = string.pack('<BI4', 0x0c, row.base + row.probe)
            if string.find(dsdt, word, 37, true) then
              found = row
            end
            break
          end
        end
      end
      if found then
        break
      end
    end
  else
    ksi.trace('no MIDR or no DSDT')
  end
  if not ksi.volatile then
    kept.part = found
  end
  return found or nil
end

-- The page, mapped once in a kept script.
local function page(row)
  if kept.page then
    return kept.page
  end
  local st = nt.check_range(row.base, row.base + PAGE, ADDRESS_BITS)
  if st ~= nt.STATUS_SUCCESS then
    return nil, st
  end
  local m, failed = ksi.hw.map(row.base, PAGE)
  if not m then
    return nil, failed
  end
  if not ksi.volatile then
    kept.page = m
  end
  return m
end

-- Packs one kind of channel: the mask of the row's channels, then each
-- channel's value (0 for one the row does not have).
local function channels(m, list, max, convert)
  local mask = 0
  local values = {}
  for c = 0, max - 1 do
    local at = list[c]
    local value = 0
    if at then
      mask = mask | (1 << c)
      value = convert(m, at)
    end
    values[#values + 1] = value
  end
  return mask, values
end

local function word(m, at)
  return ksi.hw.map_read(m, at, 4)
end

-- Milli-degrees Celsius.
local function temperature(m, at)
  local value = ksi.hw.map_read(m, at[1], 4)
  if at[2] == 'c' then
    return value * 1000
  end
  return value * 100 - 273150
end

-- Returns one sample: <I4> masks of the temperature, power, energy and limit
-- channels the row has, then <i4> per temperature channel (milli-degrees
-- Celsius), <I4> per power channel (milliwatts), <I4> per energy channel
-- (millijoules, a count that wraps) and <I4> per limit channel (milliwatts).
function read(input)
  if not ksi.hw.map or not ksi.hw.sysreg_read or not ksi.hw.firmware_table then
    ksi.trace('no map, system register or firmware table functions')
    return nt.STATUS_NOT_SUPPORTED
  end
  if #input ~= 0 then
    ksi.trace('input of %d bytes', #input)
    return nt.STATUS_INVALID_PARAMETER
  end
  local row = part()
  if not row then
    ksi.trace('no row for this part')
    return nt.STATUS_NOT_SUPPORTED
  end
  local m, failed = page(row)
  if not m then
    ksi.trace('page not mapped: 0x%08x', failed)
    return failed
  end
  local tm, t = channels(m, row.temperatures, TEMPERATURE_MAX, temperature)
  local pm, p = channels(m, row.powers, POWER_MAX, word)
  local em, e = channels(m, row.energies, ENERGY_MAX, word)
  local lm, l = channels(m, row.limits, LIMIT_MAX, word)
  local out = { string.pack('<I4I4I4I4', tm, pm, em, lm) }
  for _, v in ipairs(t) do
    out[#out + 1] = string.pack('<i4', v)
  end
  for _, list in ipairs({ p, e, l }) do
    for _, v in ipairs(list) do
      out[#out + 1] = string.pack('<I4', v)
    end
  end
  return nt.STATUS_SUCCESS, table.concat(out)
end
