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
-- Reads the hardware monitor of the ASUS boards below through their firmware's
-- WMI method, called as the ACPI method it is: the PNP0C14 device whose _WDG
-- lists the monitoring GUID as a method names it WMxx, called as
-- WMxx(instance 0, method id, a 4-byte buffer). The boards are found by the
-- SMBIOS baseboard strings. Only the five read methods are ever called.
--

local nt = require('nt')

-- 466747A0-70EC-11DE-8A39-0800200C9A66 in the GUID byte layout of _WDG.
local GUID = string.char(0xa0, 0x47, 0x67, 0x46, 0xec, 0x70, 0xde, 0x11)
  .. string.char(0x8a, 0x39, 0x08, 0x00, 0x20, 0x0c, 0x9a, 0x66)
local WDG_BLOCK = 20
local WDG_METHOD = 0x2

local GET_VALUE = 0x52574543
local UPDATE_BUFFER = 0x51574543
local GET_INFO = 0x50574543
local GET_NUMBER = 0x50574572
local GET_VERSION = 0x50574574

local VERSION_MIN = 2
local SENSORS_MAX = 64
local NAME_MAX = 32
local SOURCE_SIO = 1
local SOURCE_EC = 2
local MILLISECONDS = 500

local ARGUMENT_INTEGER = 0
local ARGUMENT_STRING = 1
local ARGUMENT_BUFFER = 2
local ARGUMENT_PACKAGE = 3

local MANUFACTURER = 'ASUSTEK COMPUTER INC.'
local BOARDS = {
  ['PRIME X399-A'] = true,
  ['PRIME X470-PRO'] = true,
  ['ROG CROSSHAIR VI EXTREME'] = true,
  ['CROSSHAIR VI HERO'] = true,
  ['ROG CROSSHAIR VI HERO (WI-FI AC)'] = true,
  ['ROG CROSSHAIR VII HERO'] = true,
  ['ROG CROSSHAIR VII HERO (WI-FI)'] = true,
  ['ROG STRIX B450-E GAMING'] = true,
  ['ROG STRIX B450-F GAMING'] = true,
  ['ROG STRIX B450-F GAMING II'] = true,
  ['ROG STRIX B450-I GAMING'] = true,
  ['ROG STRIX X399-E GAMING'] = true,
  ['ROG STRIX X470-F GAMING'] = true,
  ['ROG STRIX X470-I GAMING'] = true,
  ['ROG ZENITH EXTREME'] = true,
  ['ROG ZENITH EXTREME ALPHA'] = true,
}

local kept = {}

-- The bytes of an SMBIOS string trimmed of spaces and tabs, with a to z made upper case.
local function normalized(text)
  local first, last = 1, #text
  while first <= last and (string.byte(text, first) == 0x20 or string.byte(text, first) == 0x09) do
    first = first + 1
  end
  while last >= first and (string.byte(text, last) == 0x20 or string.byte(text, last) == 0x09) do
    last = last - 1
  end
  local bytes = {}
  for i = first, last do
    local b = string.byte(text, i)
    if b >= 0x61 and b <= 0x7a then
      b = b - 0x20
    end
    bytes[#bytes + 1] = string.char(b)
  end
  return table.concat(bytes)
end

-- The manufacturer and product strings of the first SMBIOS type 2 (baseboard)
-- structure, or nil when the table has none.
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

local function board_listed()
  if kept.listed ~= nil then
    return kept.listed
  end
  local manufacturer, product = baseboard_strings()
  local listed = manufacturer ~= nil
    and normalized(manufacturer) == MANUFACTURER
    and BOARDS[normalized(product)] == true
  if not ksi.volatile then
    kept.listed = listed
  end
  return listed
end

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

-- The method _WDG names for the GUID on a device, or nil.
local function method_of(device)
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
    if string.sub(data, at, at + 15) == GUID and flags & WDG_METHOD ~= 0 then
      return 'WM' .. string.sub(data, at + 16, at + 17)
    end
  end
  return nil
end

-- The PNP0C14 device whose _WDG lists the GUID and its method: kept once
-- found in a kept script; nil when none does.
local function interface()
  if kept.device then
    return kept.device, kept.method
  end
  local list = nt.acpi.list()
  if not list then
    return nil
  end
  local device, method
  for _, entry in ipairs(list) do
    local wmi = false
    for _, id in ipairs(entry.ids) do
      if id == 'PNP0C14' then
        wmi = true
      end
    end
    if wmi and not device then
      method = method_of(entry.device)
      if method then
        device = entry.device
      end
    end
    if entry.device ~= device then
      entry.device:release()
    end
  end
  if device and not ksi.volatile then
    kept.device, kept.method = device, method
  end
  return device, method
end

-- One read method with its 4-byte argument: the record's type and data, or nil
-- and a status.
local function call(device, method, id, value)
  local args = string.pack('<I2I2I4', ARGUMENT_INTEGER, 4, 0)
    .. string.pack('<I2I2I4', ARGUMENT_INTEGER, 4, id)
    .. string.pack('<I2I2I4', ARGUMENT_BUFFER, 4, value)
  local output, failed = nt.acpi.eval(device, method, MILLISECONDS, args)
  if not output then
    return nil, failed
  end
  local kind, data = first_record(output)
  if not kind then
    return nil, nt.STATUS_ACPI_INVALID_DATA
  end
  return kind, data
end

local function call_integer(device, method, id, value)
  local kind, data = call(device, method, id, value)
  if not kind then
    return nil, data
  end
  if kind ~= ARGUMENT_INTEGER or #data < 4 then
    return nil, nt.STATUS_ACPI_INVALID_DATA
  end
  return string.unpack('<I4', data)
end

-- The sensor's name and its four enums from GET_INFO's package of five, or nil.
local function sensor_info(device, method, index)
  local kind, data = call(device, method, GET_INFO, index)
  if kind ~= ARGUMENT_PACKAGE then
    return nil
  end
  local fields = {}
  local at = 1
  while at + 3 <= #data and #fields < 5 do
    local fieldKind, length = string.unpack('<I2I2', data, at)
    local size = math.max(4, length)
    if at + 3 + size > #data then
      return nil
    end
    local bytes = string.sub(data, at + 4, at + 3 + length)
    if #fields == 0 then
      if fieldKind ~= ARGUMENT_STRING then
        return nil
      end
      local stop = string.find(bytes, '\0', 1, true)
      fields[1] = stop and string.sub(bytes, 1, stop - 1) or bytes
    else
      if fieldKind ~= ARGUMENT_INTEGER or length < 4 then
        return nil
      end
      fields[#fields + 1] = string.unpack('<I4', bytes) & 0xff
    end
    at = at + 4 + size
  end
  if #fields ~= 5 then
    return nil
  end
  return fields
end

-- The board's interface and nil, or nil and the status the routine returns.
local function begin()
  if not ksi.hw.firmware_table or not ksi.hw.device_control then
    ksi.trace('no firmware table or device control functions')
    return nil, nil, nt.STATUS_NOT_SUPPORTED
  end
  if not board_listed() then
    ksi.trace('no row for this board')
    return nil, nil, nt.STATUS_NOT_SUPPORTED
  end
  local device, method = interface()
  if not device then
    ksi.trace('no PNP0C14 device lists the monitoring GUID')
    return nil, nil, nt.STATUS_NOT_FOUND
  end
  return device, method
end

local function finish(device)
  if ksi.volatile then
    device:release()
  end
end

-- Returns the interface version (4 bytes), the sensor count (4 bytes) and for
-- each sensor its name (32 bytes) and data type, location, source and type
-- (a byte each).
function identify(input)
  if #input ~= 0 then
    ksi.trace('input of %d bytes', #input)
    return nt.STATUS_INVALID_PARAMETER
  end
  local device, method, status = begin()
  if not device then
    return status
  end
  local version, failed = call_integer(device, method, GET_VERSION, 0)
  if not version or version < VERSION_MIN then
    ksi.trace('version %s', tostring(version))
    finish(device)
    return version and nt.STATUS_NOT_SUPPORTED or failed
  end
  local count
  count, failed = call_integer(device, method, GET_NUMBER, 0)
  if not count or count == 0 or count > SENSORS_MAX then
    ksi.trace('count %s', tostring(count))
    finish(device)
    return count and nt.STATUS_NOT_SUPPORTED or failed
  end
  local output = { string.pack('<I4I4', version, count) }
  for index = 0, count - 1 do
    local fields = sensor_info(device, method, index)
    if not fields then
      ksi.trace('no information for sensor %d', index)
      finish(device)
      return nt.STATUS_ACPI_INVALID_DATA
    end
    local name = string.sub(fields[1], 1, NAME_MAX)
    output[#output + 1] = name .. string.rep('\0', NAME_MAX - #name)
    output[#output + 1] = string.char(fields[2], fields[3], fields[4], fields[5])
  end
  finish(device)
  return nt.STATUS_SUCCESS, table.concat(output)
end

-- Reads each sensor given by its source (a byte each, sensor 0 first): every
-- source present is refreshed once, then each sensor's raw value is read.
-- Returns a status and the low 32 bits of the value (4 bytes each) per sensor.
function read(input)
  if #input == 0 or #input > SENSORS_MAX then
    ksi.trace('input of %d bytes', #input)
    return nt.STATUS_INVALID_PARAMETER
  end
  for i = 1, #input do
    local source = string.byte(input, i)
    if source ~= SOURCE_SIO and source ~= SOURCE_EC then
      ksi.trace('sensor %d has source %d', i - 1, source)
      return nt.STATUS_INVALID_PARAMETER
    end
  end
  local device, method, status = begin()
  if not device then
    return status
  end
  for _, source in ipairs({ SOURCE_SIO, SOURCE_EC }) do
    if string.find(input, string.char(source), 1, true) then
      call(device, method, UPDATE_BUFFER, source)
    end
  end
  local output = {}
  for index = 0, #input - 1 do
    local value, failed = call_integer(device, method, GET_VALUE, index)
    if value then
      output[#output + 1] = string.pack('<i4I4', nt.STATUS_SUCCESS, value)
    else
      output[#output + 1] = string.pack('<i4I4', failed, 0)
    end
  end
  finish(device)
  return nt.STATUS_SUCCESS, table.concat(output)
end
