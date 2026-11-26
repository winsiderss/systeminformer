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
-- Reads Gigabyte board temperatures through the firmware WMI method, evaluated
-- as ACPI WMxx(0, 125h, index buffer) on boards whose SMBIOS baseboard
-- manufacturer is Gigabyte.
--

local nt = require('nt')

-- DEADBEEF-2001-0000-00A0-C90629100000 in the GUID byte layout of _WDG.
local GUID = string.char(0xef, 0xbe, 0xad, 0xde, 0x01, 0x20, 0x00, 0x00)
  .. string.char(0x00, 0xa0, 0xc9, 0x06, 0x29, 0x10, 0x00, 0x00)
local WDG_BLOCK = 20
local WDG_METHOD = 0x2

local TEMPERATURE = 0x125
local SENSORS = 6
local MILLISECONDS = 500
local MANUFACTURER = 'GIGABYTE'

local ARGUMENT_INTEGER = 0
local ARGUMENT_BUFFER = 2

local kept = {}

-- The manufacturer string of the first SMBIOS type 2 (baseboard) structure, or
-- nil.
local function baseboard_manufacturer()
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
      return strings[string.byte(smbios, at + 4)] or ''
    end
    if kind == 127 then
      return nil
    end
    at = ends + 2
  end
  return nil
end

-- Whether the board's manufacturer string starts with Gigabyte, any case.
local function gigabyte_board()
  if kept.board == nil then
    local manufacturer = baseboard_manufacturer() or ''
    local board = string.upper(string.sub(manufacturer, 1, #MANUFACTURER)) == MANUFACTURER
    if ksi.volatile then
      return board
    end
    kept.board = board
  end
  return kept.board
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

-- The PNP0C14 device listing the GUID and its method (kept in a kept script),
-- or nil.
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

-- One sensor's temperature query: its integer, or nil and a status.
local function temperature(device, method, index)
  local args = string.pack('<I2I2I4', ARGUMENT_INTEGER, 4, 0)
    .. string.pack('<I2I2I4', ARGUMENT_INTEGER, 4, TEMPERATURE)
    .. string.pack('<I2I2I4', ARGUMENT_BUFFER, 4, index)
  local output, failed = nt.acpi.eval(device, method, MILLISECONDS, args)
  if not output then
    if failed == nt.STATUS_TIMEOUT then
      failed = nt.STATUS_IO_TIMEOUT
    end
    return nil, failed
  end
  local kind, data = first_record(output)
  if kind ~= ARGUMENT_INTEGER or #data < 4 then
    return nil, nt.STATUS_ACPI_INVALID_DATA
  end
  return string.unpack('<I4', data)
end

-- Reads the six sensors: a status and the method's low 32 bits each; the low
-- byte is signed degrees C, 0 if absent.
function read(input)
  if #input ~= 0 then
    ksi.trace('input of %d bytes', #input)
    return nt.STATUS_INVALID_PARAMETER
  end
  if not ksi.hw.firmware_table or not ksi.hw.device_control then
    ksi.trace('no firmware table or device control functions')
    return nt.STATUS_NOT_SUPPORTED
  end
  if not gigabyte_board() then
    ksi.trace('not a Gigabyte board')
    return nt.STATUS_NOT_SUPPORTED
  end
  local device, method = interface()
  if not device then
    ksi.trace('no PNP0C14 device lists the temperature GUID')
    return nt.STATUS_NOT_FOUND
  end
  local output = {}
  for index = 0, SENSORS - 1 do
    local value, failed = temperature(device, method, index)
    if value then
      output[#output + 1] = string.pack('<i4I4', nt.STATUS_SUCCESS, value)
    else
      output[#output + 1] = string.pack('<i4I4', failed, 0)
    end
  end
  if ksi.volatile then
    device:release()
  end
  return nt.STATUS_SUCCESS, table.concat(output)
end
