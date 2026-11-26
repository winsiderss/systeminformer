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
-- Reads Alienware and Dell G temperatures and fan speeds through the firmware
-- WMI method, evaluated as ACPI WMxx(0, 14h, operation buffer) on listed
-- machines only.
--

local nt = require('nt')

-- A70591CE-A997-11DA-B012-B622A1EF5492 in the GUID byte layout of _WDG.
local GUID = string.char(0xce, 0x91, 0x05, 0xa7, 0x97, 0xa9, 0xda, 0x11)
  .. string.char(0xb0, 0x12, 0xb6, 0x22, 0xa1, 0xef, 0x54, 0x92)
local WDG_BLOCK = 20
local WDG_METHOD = 0x2

local THERMAL_INFORMATION = 0x14
local GET_DESCRIPTION = 0x02
local GET_RESOURCE = 0x03
local GET_TEMPERATURE = 0x04
local GET_FAN_SPEED = 0x05
local FAILED = 0xffffffff
local FAILED_2 = 0xfffffffe
local RESOURCES_MAX = 16
local MILLISECONDS = 500

local KIND_TEMPERATURE = 1
local KIND_FAN = 2

local ARGUMENT_INTEGER = 0
local ARGUMENT_BUFFER = 2

-- System manufacturer and a text the product name contains (upper case).
local MACHINES = {
  { 'ALIENWARE', 'ALIENWARE 16 AREA-51' },
  { 'ALIENWARE', 'ALIENWARE 16X AURORA' },
  { 'ALIENWARE', 'ALIENWARE 18 AREA-51' },
  { 'ALIENWARE', 'ALIENWARE 16 AURORA' },
  { 'ALIENWARE', 'ALIENWARE AREA-51 AAT2250' },
  { 'ALIENWARE', 'ALIENWARE AREA-51M' },
  { 'ALIENWARE', 'ALIENWARE M15' },
  { 'ALIENWARE', 'ALIENWARE M16 R1' },
  { 'ALIENWARE', 'ALIENWARE M16 R2' },
  { 'ALIENWARE', 'ALIENWARE M17' },
  { 'ALIENWARE', 'ALIENWARE M18' },
  { 'ALIENWARE', 'ALIENWARE X15' },
  { 'ALIENWARE', 'ALIENWARE X16' },
  { 'ALIENWARE', 'ALIENWARE X17' },
  { 'DELL INC.', 'DELL G15' },
  { 'DELL INC.', 'DELL G16' },
  { 'DELL INC.', 'G3' },
  { 'DELL INC.', 'G5' },
}

local kept = {}

-- The bytes of an SMBIOS string trimmed of spaces and tabs, with a to z made
-- upper case.
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

-- The manufacturer and product name strings of the first SMBIOS type 1
-- structure, or nil.
local function system_strings()
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
    if kind == 1 then
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

-- Whether the rows list this machine: kept once in a kept script.
local function listed()
  if kept.listed ~= nil then
    return kept.listed
  end
  local found = false
  local manufacturer, product = system_strings()
  if manufacturer then
    manufacturer, product = normalized(manufacturer), normalized(product)
    for _, row in ipairs(MACHINES) do
      if string.find(manufacturer, row[1], 1, true) and string.find(product, row[2], 1, true) then
        found = true
        break
      end
    end
  end
  if not ksi.volatile then
    kept.listed = found
  end
  return found
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

-- One thermal information read: its 32-bit result, or nil and a status.
local function information(device, method, operation, argument)
  local args = string.pack('<I2I2I4', ARGUMENT_INTEGER, 4, 0)
    .. string.pack('<I2I2I4', ARGUMENT_INTEGER, 4, THERMAL_INFORMATION)
    .. string.pack('<I2I2BBBB', ARGUMENT_BUFFER, 4, operation, argument, 0, 0)
  local output, failed = nt.acpi.eval(device, method, MILLISECONDS, args)
  if not output then
    if failed == nt.STATUS_TIMEOUT then
      failed = nt.STATUS_IO_TIMEOUT
    end
    return nil, failed
  end
  local kind, data = first_record(output)
  if (kind ~= ARGUMENT_INTEGER and kind ~= ARGUMENT_BUFFER) or #data < 4 then
    return nil, nt.STATUS_ACPI_INVALID_DATA
  end
  local value = string.unpack('<I4', data)
  if value == FAILED or value == FAILED_2 then
    return nil, nt.STATUS_NOT_SUPPORTED
  end
  return value
end

local function finish(device)
  if ksi.volatile then
    device:release()
  end
end

-- The machine's interface, or nil and the status the routine returns.
local function begin()
  if not ksi.hw.firmware_table or not ksi.hw.device_control then
    ksi.trace('no firmware table or device control functions')
    return nil, nt.STATUS_NOT_SUPPORTED
  end
  if not listed() then
    ksi.trace('no row for this machine')
    return nil, nt.STATUS_NOT_SUPPORTED
  end
  local device, method = interface()
  if not device then
    ksi.trace('no PNP0C14 device lists the thermal GUID')
    return nil, nt.STATUS_NOT_FOUND
  end
  return device, method
end

-- Returns the fan and temperature sensor counts, then each fan's id and each
-- temperature sensor's id (a byte each).
function identify(input)
  if #input ~= 0 then
    ksi.trace('input of %d bytes', #input)
    return nt.STATUS_INVALID_PARAMETER
  end
  local device, method = begin()
  if not device then
    return method
  end
  local description, failed = information(device, method, GET_DESCRIPTION, 0)
  if not description then
    ksi.trace('no description: 0x%x', failed)
    finish(device)
    return failed
  end
  local fans, temperatures = description & 0xff, (description >> 8) & 0xff
  for shift = 0, 24, 8 do
    if (description >> shift) & 0xff > RESOURCES_MAX then
      ksi.trace('description 0x%08x', description)
      finish(device)
      return nt.STATUS_ACPI_INVALID_DATA
    end
  end
  local output = { string.char(fans, temperatures) }
  for index = 0, fans + temperatures - 1 do
    local id
    id, failed = information(device, method, GET_RESOURCE, index)
    if not id then
      ksi.trace('no resource %d: 0x%x', index, failed)
      finish(device)
      return failed
    end
    output[#output + 1] = string.char(id & 0xff)
  end
  finish(device)
  return nt.STATUS_SUCCESS, table.concat(output)
end

-- Reads sensors given as kind (1 temperature, 2 fan) and id (a byte each): a
-- status and 4-byte value each, C or RPM.
function read(input)
  if #input == 0 or #input % 2 ~= 0 or #input > 4 * RESOURCES_MAX then
    ksi.trace('input of %d bytes', #input)
    return nt.STATUS_INVALID_PARAMETER
  end
  for at = 1, #input, 2 do
    local kind = string.byte(input, at)
    if kind ~= KIND_TEMPERATURE and kind ~= KIND_FAN then
      ksi.trace('sensor %d has kind %d', (at - 1) // 2, kind)
      return nt.STATUS_INVALID_PARAMETER
    end
  end
  local device, method = begin()
  if not device then
    return method
  end
  local output = {}
  for at = 1, #input, 2 do
    local kind, id = string.byte(input, at, at + 1)
    local operation = kind == KIND_TEMPERATURE and GET_TEMPERATURE or GET_FAN_SPEED
    local value, failed = information(device, method, operation, id)
    if value then
      output[#output + 1] = string.pack('<i4I4', nt.STATUS_SUCCESS, value)
    else
      output[#output + 1] = string.pack('<i4I4', failed, 0)
    end
  end
  finish(device)
  return nt.STATUS_SUCCESS, table.concat(output)
end
