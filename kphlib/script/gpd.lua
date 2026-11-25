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
-- Reads the fan speed of the GPD handhelds below from their embedded
-- controller's RAM through its I2EC window (the Super I/O port pair, index 2Eh
-- and 2Fh, registers 10h to 12h), found by the manufacturer and product name of
-- the SMBIOS system structure. The Win Max 2 is
-- not listed: its reads need writes to the controller first.
--

local nt = require('nt')

local ROWS = {
  { product = 'G1617-01', port = 0x4e, rpm = 0x0478 },
  { product = 'G1617-02', port = 0x4e, rpm = 0x0478 },
  { product = 'G1617-02-L', port = 0x4e, rpm = 0x0478 },
  { product = 'G1628-04', port = 0x4e, rpm = 0x0478 },
  { product = 'G1628-04-L', port = 0x4e, rpm = 0x0478 },
  { product = 'G1622-01', port = 0x4e, rpm = 0x0478 },
  { product = 'G1622-01-L', port = 0x4e, rpm = 0x0478 },
  { product = 'G1618-05', port = 0x4e, rpm = 0x0478 },
  { product = 'G1688-08', port = 0x4e, rpm = 0x0476 },
}

local I2EC_INDEX = 0x2e
local I2EC_DATA = 0x2f
local I2EC_ADDRESS_LOW = 0x10
local I2EC_ADDRESS_HIGH = 0x11
local I2EC_VALUE = 0x12

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

-- The row of this handheld: from the table once in a kept script, nil when
-- there is none.
local function board()
  if kept.board ~= nil then
    return kept.board or nil
  end
  local manufacturer, product = system_strings()
  if not manufacturer then
    ksi.trace('no SMBIOS system structure')
    return nil
  end
  local found = false
  if normalized(manufacturer) == 'GPD' then
    product = normalized(product)
    for _, row in ipairs(ROWS) do
      if row.product == product then
        found = row
        break
      end
    end
  end
  if not ksi.volatile then
    kept.board = found
  end
  return found or nil
end

local function i2ec(port, register, value)
  ksi.hw.port_write(port, 1, I2EC_INDEX)
  ksi.hw.port_write(port + 1, 1, register)
  ksi.hw.port_write(port, 1, I2EC_DATA)
  if value then
    ksi.hw.port_write(port + 1, 1, value)
  else
    return ksi.hw.port_read(port + 1, 1)
  end
end

-- A byte of the controller's RAM.
local function ec_byte(port, address)
  i2ec(port, I2EC_ADDRESS_HIGH, address >> 8)
  i2ec(port, I2EC_ADDRESS_LOW, address & 0xff)
  return i2ec(port, I2EC_VALUE)
end

-- Returns the fan speed in RPM: the controller's two bytes, high first.
function read(input)
  if not ksi.hw.port_read or not ksi.hw.port_write or not ksi.hw.firmware_table then
    ksi.trace('no port or firmware table functions')
    return nt.STATUS_NOT_SUPPORTED
  end
  if #input ~= 0 then
    ksi.trace('input of %d bytes', #input)
    return nt.STATUS_INVALID_PARAMETER
  end
  local b = board()
  if not b then
    ksi.trace('no row for this handheld')
    return nt.STATUS_NOT_SUPPORTED
  end
  local lock <close>, failed = ksi.lock('isa')
  if not lock then
    ksi.trace('the isa lock was not taken: 0x%08x', failed)
    return failed
  end
  local high = ec_byte(b.port, b.rpm)
  local low = ec_byte(b.port, b.rpm + 1)
  return nt.STATUS_SUCCESS, string.pack('<I4', (high << 8) | low)
end
