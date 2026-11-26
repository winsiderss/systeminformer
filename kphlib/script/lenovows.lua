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
-- Reads the temperatures and fan speeds of the Lenovo ThinkStations below from
-- their Microchip embedded controller through its EMI window at 900h, found by
-- the SMBIOS system structure (manufacturer LENOVO, the product name's machine
-- type). The window is claimed
-- with application ID 1 and released after; a window another agent holds is
-- busy. The MCHP signature is checked before the reads.
--

local nt = require('nt')

local EMI = 0x900
local EMI_ADDRESS = 0x902
local EMI_DATA = 0x904
local EMI_APPLICATION = 0x90c
local APPLICATION_ID = 0x01

local MODELS = {
  -- 0: P620 (PX)
  {
    types = { '30EU', '30EV' },
    temps = { 0, 1, 2, 3, 4, 5, 6, 7, 8, 9, 10, 11, 12, 13, 14, 31, 32 },
    fans = { 0, 1, 2, 3, 4, 5, 6, 7, 8, 9, 10, 11, 12, 13, 14, 15 },
  },
  -- 1: P7
  {
    types = { '30F2', '30F3' },
    temps = { 0, 2, 3, 6, 7, 8, 9, 10, 11, 12, 13, 14, 31 },
    fans = { 0, 2, 3, 4, 5, 6, 7, 8, 10, 11, 14 },
  },
  -- 2: P5
  {
    types = { '30G9', '30GA' },
    temps = { 0, 2, 3, 6, 7, 8, 9, 10, 11, 12, 13, 14, 31 },
    fans = { 0, 5, 6, 7, 8, 10, 11, 14 },
  },
  -- 3: P8
  {
    types = { '30HH', '30HJ' },
    temps = { 0, 1, 2, 8, 9, 13, 14, 15, 16, 17, 19, 20, 33 },
    fans = { 0, 1, 2, 3, 4, 5, 6, 7, 8, 9, 10, 11, 12, 14 },
  },
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
-- structure, or nil when the table has none.
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

-- The model of this workstation, its number from 0: from the table once in a
-- kept script, nil when there is none.
local function model()
  if kept.model ~= nil then
    return kept.model or nil
  end
  local manufacturer, product = system_strings()
  if not manufacturer then
    ksi.trace('no SMBIOS system structure')
    return nil
  end
  local found = false
  if normalized(manufacturer) == 'LENOVO' then
    local machine = string.sub(normalized(product), 1, 4)
    for number, m in ipairs(MODELS) do
      for _, machineType in ipairs(m.types) do
        if machineType == machine then
          found = number - 1
        end
      end
    end
  end
  if not ksi.volatile then
    kept.model = found
  end
  return found or nil
end

-- A byte of the controller's memory: the dword holding it addressed, then its
-- lane read.
local function ec_byte(address)
  ksi.hw.port_write(EMI_ADDRESS, 2, address & 0x7ffc)
  return ksi.hw.port_read(EMI_DATA + (address & 3), 1)
end

local function signature_present()
  ksi.hw.port_write(EMI_ADDRESS, 2, 0)
  return ksi.hw.port_read(EMI_DATA, 1) == 0x4d
    and ksi.hw.port_read(EMI_DATA + 1, 1) == 0x43
    and ksi.hw.port_read(EMI_DATA + 2, 1) == 0x48
    and ksi.hw.port_read(EMI_DATA + 3, 1) == 0x50
end

local claimed = false

-- Releases the window when the scope that claims it ends.
local release = setmetatable({}, {
  __close = function()
    if claimed then
      claimed = false
      if ksi.hw.port_read(EMI_APPLICATION, 1) == APPLICATION_ID then
        ksi.hw.port_write(EMI_APPLICATION, 1, APPLICATION_ID)
      end
    end
  end,
})

-- Returns the model number (4 bytes), each mapped temperature in degrees C
-- (4 bytes, signed; -1 when the controller has no reading) and each mapped
-- fan's speed in RPM (4 bytes), in the model's map order.
function read(input)
  if not ksi.hw.port_read or not ksi.hw.port_write or not ksi.hw.firmware_table then
    ksi.trace('no port or firmware table functions')
    return nt.STATUS_NOT_SUPPORTED
  end
  if #input ~= 0 then
    ksi.trace('input of %d bytes', #input)
    return nt.STATUS_INVALID_PARAMETER
  end
  local number = model()
  if not number then
    ksi.trace('no row for this workstation')
    return nt.STATUS_NOT_SUPPORTED
  end
  local m = MODELS[number + 1]
  local lock <close>, failed = ksi.lock('isa')
  if not lock then
    ksi.trace('the isa lock was not taken: 0x%08x', failed)
    return failed
  end
  if ksi.hw.port_read(EMI_APPLICATION, 1) ~= 0 then
    ksi.trace('the window is held by another agent')
    return nt.STATUS_DEVICE_BUSY
  end
  local claim <close> = release
  claimed = true
  ksi.hw.port_write(EMI_APPLICATION, 1, APPLICATION_ID)
  if ksi.hw.port_read(EMI_APPLICATION, 1) ~= APPLICATION_ID then
    ksi.trace('the window was not granted')
    return nt.STATUS_DEVICE_BUSY
  end
  if not signature_present() then
    ksi.trace('no MCHP signature at 0x%x', EMI)
    return nt.STATUS_NOT_FOUND
  end
  local output = { string.pack('<I4', number) }
  for _, channel in ipairs(m.temps) do
    local value = ec_byte(0x200 + 0x81 + channel)
    output[#output + 1] = string.pack('<i4', value > 0x40 and value - 0x40 or -1)
  end
  for _, channel in ipairs(m.fans) do
    local low = ec_byte(0x400 + 0x20 + channel * 2)
    local high = ec_byte(0x400 + 0x21 + channel * 2)
    output[#output + 1] = string.pack('<I4', (high << 8) | low)
  end
  return nt.STATUS_SUCCESS, table.concat(output)
end
