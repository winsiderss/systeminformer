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
-- Reads HP business machine sensors from the firmware WMI data block, evaluated
-- as ACPI WQxx(instance) (bracketed by WCxx(1) and WCxx(0) when the block is
-- expensive).
--

local nt = require('nt')

-- 8F1F6435-9F42-42C8-BADC-0E9424F20C9A in the GUID byte layout of _WDG.
local GUID = string.char(0x35, 0x64, 0x1f, 0x8f, 0x42, 0x9f, 0xc8, 0x42)
  .. string.char(0xba, 0xdc, 0x0e, 0x94, 0x24, 0xf2, 0x0c, 0x9a)
local WDG_BLOCK = 20
local WDG_EXPENSIVE = 0x1
local WDG_METHOD = 0x2
local WDG_EVENT = 0x8

local INSTANCES_MAX = 32
local ELEMENTS_MAX = 32
local NAME_MAX = 32
local MILLISECONDS = 500

local ARGUMENT_INTEGER = 0
local ARGUMENT_STRING = 1
local ARGUMENT_BUFFER = 2
local ARGUMENT_PACKAGE = 3

local TYPE_TEMPERATURE = 2
local TYPE_VOLTAGE = 3
local TYPE_CURRENT = 4
local TYPE_AIR_FLOW = 12

local UNITS_OTHER = 1
local UNITS_DEGREES_C = 2
local UNITS_DEGREES_F = 3
local UNITS_DEGREES_K = 4
local UNITS_VOLTS = 5
local UNITS_AMPS = 6
local UNITS_RPM = 19

local STATUS_NO_CONTACT = 12

-- The kinds identify gives.
local KIND_TEMPERATURE = 1 -- millidegrees C
local KIND_VOLTAGE = 2 -- millivolts
local KIND_CURRENT = 3 -- milliamperes
local KIND_FAN = 4 -- RPM

local kept = {}

-- The first record of an evaluation's output: its type and data, or nil.
local function first_record(output)
  if #output < 16 or string.unpack('<I4', output, 9) < 1 then
    return nil
  end
  local kind, length = string.unpack('<I2I2', output, 13)
  if 16 + length > #output then
    return nil
  end
  return kind, string.sub(output, 17, 16 + length)
end

-- The package's elements: a list of { kind, data }, or nil.
local function package_elements(data)
  local list = {}
  local at = 1
  while at + 3 <= #data do
    if #list == ELEMENTS_MAX then
      return nil
    end
    local kind, length = string.unpack('<I2I2', data, at)
    if at + 3 + length > #data then
      return nil
    end
    list[#list + 1] = { kind, string.sub(data, at + 4, at + 3 + length) }
    at = at + 4 + math.max(4, length)
  end
  return list
end

-- A string or WMI string buffer (16-bit byte length, UTF-16) as trimmed ASCII
-- ('?' outside 20h-7Eh), or nil.
local function text(element)
  local kind, data = element[1], element[2]
  local bytes = {}
  if kind == ARGUMENT_STRING then
    local stop = string.find(data, '\0', 1, true)
    data = stop and string.sub(data, 1, stop - 1) or data
    for i = 1, #data do
      local b = string.byte(data, i)
      bytes[#bytes + 1] = (b >= 0x20 and b <= 0x7e) and string.char(b) or '?'
    end
  elseif kind == ARGUMENT_BUFFER then
    if #data <= 2 then
      return nil
    end
    local length = string.unpack('<I2', data)
    if length > #data - 2 or length % 2 ~= 0 then
      return nil
    end
    for at = 3, 2 + length, 2 do
      local unit = string.unpack('<I2', data, at)
      if unit == 0 then
        break
      end
      bytes[#bytes + 1] = (unit >= 0x20 and unit <= 0x7e) and string.char(unit) or '?'
    end
  else
    return nil
  end
  local first, last = 1, #bytes
  while first <= last and bytes[first] == ' ' do
    first = first + 1
  end
  while last >= first and bytes[last] == ' ' do
    last = last - 1
  end
  return table.concat(bytes, '', first, last)
end

-- An integer element's low 32 bits, or nil.
local function integer(element)
  if element[1] ~= ARGUMENT_INTEGER or #element[2] < 4 then
    return nil
  end
  return string.unpack('<I4', element[2])
end

-- An instance's name, type, status, units, modifier and reading, or nil.
-- Package: Name, Description, SensorType, OtherSensorType, OperationalStatus,
-- then Size, PossibleStates[Size], CurrentState, BaseUnits, UnitModifier,
-- CurrentReading, RateUnits; or the older CurrentState, PossibleStates[],
-- BaseUnits, UnitModifier, CurrentReading.
local function sensor_of(elements)
  local n = #elements
  if n <= 6 then
    return nil
  end
  local newer = elements[6][1] == ARGUMENT_INTEGER
  local count = newer and n - 11 or n - 9
  if count < 1 or (newer and integer(elements[6]) ~= count) then
    return nil
  end
  -- 's' a string, 'i' an integer, by position
  local shape = { 's', 's', 'i', 's', 'i' }
  if newer then
    shape[6] = 'i'
  else
    shape[6] = 's'
  end
  for _ = 1, count do
    shape[#shape + 1] = 's'
  end
  if newer then
    for _, s in ipairs({ 's', 'i', 'i', 'i', 'i' }) do
      shape[#shape + 1] = s
    end
  else
    for _, s in ipairs({ 'i', 'i', 'i' }) do
      shape[#shape + 1] = s
    end
  end
  for i = 1, n do
    if shape[i] == 's' and not text(elements[i]) then
      return nil
    end
    if shape[i] == 'i' and not integer(elements[i]) then
      return nil
    end
  end
  local last = newer and n - 1 or n
  return {
    name = text(elements[1]),
    type = integer(elements[3]),
    status = integer(elements[5]),
    units = integer(elements[last - 2]),
    modifier = integer(elements[last - 1]),
    reading = integer(elements[last]),
  }
end

-- The kind a sensor is shown as, or nil when it is none of them.
local function kind_of(sensor)
  local units = sensor.units
  if sensor.type == TYPE_TEMPERATURE then
    if
      units == UNITS_DEGREES_C
      or units == UNITS_DEGREES_F
      or units == UNITS_DEGREES_K
      or (units == UNITS_OTHER and string.find(sensor.name, 'Thermal Index', 1, true))
    then
      return KIND_TEMPERATURE
    end
  elseif sensor.type == TYPE_VOLTAGE and units == UNITS_VOLTS then
    return KIND_VOLTAGE
  elseif sensor.type == TYPE_CURRENT and units == UNITS_AMPS then
    return KIND_CURRENT
  elseif sensor.type == TYPE_AIR_FLOW and units == UNITS_RPM then
    return KIND_FAN
  end
  return nil
end

-- Divides rounding half away from zero.
local function divide(value, by)
  if value >= 0 then
    return (value + by // 2) // by
  end
  return -((-value + by // 2) // by)
end

-- A sensor's reading in its kind's unit (milli, RPM for a fan), clamped to
-- signed 32 bits.
local function scaled(sensor, kind)
  local modifier = sensor.modifier
  if modifier >= 0x80000000 then
    modifier = modifier - 0x100000000
  end
  local target = kind == KIND_FAN and 0 or -3
  local value = sensor.reading
  while value ~= 0 and modifier < target do
    value = divide(value, 10)
    modifier = modifier + 1
  end
  while value ~= 0 and modifier > target and value <= 0x7fffffff do
    value = value * 10
    modifier = modifier - 1
  end
  if kind == KIND_TEMPERATURE then
    if sensor.units == UNITS_DEGREES_F then
      value = divide((value - 32000) * 5, 9)
    elseif sensor.units == UNITS_DEGREES_K then
      value = value - 273150
    end
  end
  return math.max(-0x80000000, math.min(0x7fffffff, value))
end

-- The GUID's data block in a device's _WDG: object id, instance count and
-- flags, or nil.
local function block_of(device)
  local output = nt.acpi.eval(device, '_WDG', MILLISECONDS)
  if not output then
    return nil
  end
  local kind, data = first_record(output)
  if kind ~= ARGUMENT_BUFFER then
    return nil
  end
  for at = 1, #data - WDG_BLOCK + 1, WDG_BLOCK do
    local flags = string.byte(data, at + 19)
    if string.sub(data, at, at + 15) == GUID and flags & (WDG_METHOD | WDG_EVENT) == 0 then
      return string.sub(data, at + 16, at + 17), string.byte(data, at + 18), flags
    end
  end
  return nil
end

-- The PNP0C14 device listing the block, with its id, count and flags (kept in a
-- kept script), or nil.
local function interface()
  if kept.device then
    return kept.device, kept.id, kept.count, kept.flags
  end
  local list = nt.acpi.list()
  if not list then
    return nil
  end
  local device, id, count, flags
  for _, entry in ipairs(list) do
    local wmi = false
    for _, hid in ipairs(entry.ids) do
      if hid == 'PNP0C14' then
        wmi = true
      end
    end
    if wmi and not device then
      id, count, flags = block_of(entry.device)
      if id then
        device = entry.device
      end
    end
    if entry.device ~= device then
      entry.device:release()
    end
  end
  if device and not ksi.volatile then
    kept.device, kept.id, kept.count, kept.flags = device, id, count, flags
  end
  return device, id, count, flags
end

local function integer_argument(value)
  return string.pack('<I2I2I4', ARGUMENT_INTEGER, 4, value)
end

-- One instance's checked fields, or nil and a status.
local function query(device, id, instance)
  local output, failed = nt.acpi.eval(device, 'WQ' .. id, MILLISECONDS, integer_argument(instance))
  if not output then
    if failed == nt.STATUS_TIMEOUT then
      failed = nt.STATUS_IO_TIMEOUT
    end
    return nil, failed
  end
  local kind, data = first_record(output)
  if kind ~= ARGUMENT_PACKAGE then
    return nil, nt.STATUS_ACPI_INVALID_DATA
  end
  local elements = package_elements(data)
  local sensor = elements and sensor_of(elements)
  if not sensor then
    return nil, nt.STATUS_ACPI_INVALID_DATA
  end
  return sensor
end

-- Runs queries with an expensive block collected around them; nil and a status
-- when collecting fails.
local function collected(device, id, flags, queries)
  if flags & WDG_EXPENSIVE == 0 then
    local result = queries()
    return result
  end
  local asked = false
  local release <close> = setmetatable({}, {
    __close = function()
      if asked then
        nt.acpi.eval(device, 'WC' .. id, MILLISECONDS, integer_argument(0))
      end
    end,
  })
  asked = true
  local output, failed = nt.acpi.eval(device, 'WC' .. id, MILLISECONDS, integer_argument(1))
  if not output then
    asked = false
    if failed == nt.STATUS_TIMEOUT then
      failed = nt.STATUS_IO_TIMEOUT
    end
    return nil, failed
  end
  local result = queries()
  return result
end

local function finish(device)
  if ksi.volatile then
    device:release()
  end
end

-- The machine's block, or nil and the status the routine returns.
local function begin()
  if not ksi.hw.device_control then
    ksi.trace('no device control function')
    return nil, nt.STATUS_NOT_SUPPORTED
  end
  local device, id, count, flags = interface()
  if not device then
    ksi.trace('no PNP0C14 device lists the sensor block')
    return nil, nt.STATUS_NOT_FOUND
  end
  if count == 0 or count > INSTANCES_MAX then
    ksi.trace('%d instances', count)
    finish(device)
    return nil, nt.STATUS_NOT_SUPPORTED
  end
  return device, id, count, flags
end

-- Returns kind and instance (a byte each) and name (32 bytes) of each
-- temperature, voltage, current or fan sensor.
function identify(input)
  if #input ~= 0 then
    ksi.trace('input of %d bytes', #input)
    return nt.STATUS_INVALID_PARAMETER
  end
  local device, id, count, flags = begin()
  if not device then
    return id
  end
  local output = {}
  local result, failed = collected(device, id, flags, function()
    for instance = 0, count - 1 do
      local sensor = query(device, id, instance)
      local kind = sensor and kind_of(sensor)
      if kind then
        local name = string.sub(sensor.name, 1, NAME_MAX - 1)
        output[#output + 1] = string.char(kind, instance)
          .. name
          .. string.rep('\0', NAME_MAX - #name)
      end
    end
    return true
  end)
  finish(device)
  if not result then
    ksi.trace('collection: 0x%x', failed)
    return failed
  end
  if #output == 0 then
    return nt.STATUS_NOT_SUPPORTED
  end
  return nt.STATUS_SUCCESS, table.concat(output)
end

-- Reads sensors given as kind and instance (a byte each): a status and a signed
-- 4-byte value each in the kind's unit.
function read(input)
  if #input == 0 or #input % 2 ~= 0 or #input > 2 * INSTANCES_MAX then
    ksi.trace('input of %d bytes', #input)
    return nt.STATUS_INVALID_PARAMETER
  end
  for at = 1, #input, 2 do
    local kind = string.byte(input, at)
    if kind < KIND_TEMPERATURE or kind > KIND_FAN then
      ksi.trace('sensor %d has kind %d', (at - 1) // 2, kind)
      return nt.STATUS_INVALID_PARAMETER
    end
  end
  local device, id, count, flags = begin()
  if not device then
    return id
  end
  local output = {}
  local result, failed = collected(device, id, flags, function()
    for at = 1, #input, 2 do
      local kind, instance = string.byte(input, at, at + 1)
      local sensor, status
      if instance < count then
        sensor, status = query(device, id, instance)
      else
        status = nt.STATUS_INVALID_PARAMETER
      end
      if sensor and kind_of(sensor) ~= kind then
        sensor, status = nil, nt.STATUS_ACPI_INVALID_DATA
      elseif sensor and sensor.status == STATUS_NO_CONTACT then
        sensor, status = nil, nt.STATUS_DEVICE_NOT_READY
      end
      if sensor then
        output[#output + 1] = string.pack('<i4i4', nt.STATUS_SUCCESS, scaled(sensor, kind))
      else
        output[#output + 1] = string.pack('<i4i4', status, 0)
      end
    end
    return true
  end)
  finish(device)
  if not result then
    ksi.trace('collection: 0x%x', failed)
    return failed
  end
  return nt.STATUS_SUCCESS, table.concat(output)
end
