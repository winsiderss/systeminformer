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
-- Reads the memory map of the ChromeOS embedded controller through its port
-- window on the boards listed below, found by the manufacturer and product
-- name of the SMBIOS system structure. A signature in the map is checked
-- before and after every read.
--

local ROWS = {
  {
    manufacturer = 'Framework',
    product = 'Laptop 13 (AMD Ryzen 7040Series)',
    port = 0xe00,
    signature = { 0x20, 0x45, 0x43 },
    reads = function()
      return {
        { 0x00, 0x10 },
        { 0x10, 0x08 },
        { 0x18, 0x08 },
        { 0x20, 0x08 },
        { 0x40, 0x20 },
      }
    end,
  },
  {
    manufacturer = 'Framework',
    product = 'Laptop 13 (AMD Ryzen 7040 Series)',
    port = 0xe00,
    signature = { 0x20, 0x45, 0x43 },
    reads = function()
      return {
        { 0x00, 0x10 },
        { 0x10, 0x08 },
        { 0x18, 0x08 },
        { 0x20, 0x08 },
        { 0x40, 0x20 },
      }
    end,
  },
  {
    manufacturer = 'Framework',
    product = 'Laptop 13 (AMD Ryzen AI 300 Series)',
    port = 0xe00,
    signature = { 0x20, 0x45, 0x43 },
    reads = function()
      return {
        { 0x00, 0x10 },
        { 0x10, 0x08 },
        { 0x18, 0x08 },
        { 0x20, 0x08 },
        { 0x40, 0x20 },
      }
    end,
  },
  {
    manufacturer = 'Framework',
    product = 'Laptop 13 (Intel Core Ultra Series 1)',
    port = 0xe00,
    signature = { 0x20, 0x45, 0x43 },
    reads = function()
      return {
        { 0x00, 0x10 },
        { 0x10, 0x08 },
        { 0x18, 0x08 },
        { 0x20, 0x08 },
        { 0x40, 0x20 },
      }
    end,
  },
  {
    manufacturer = 'Framework',
    product = 'Laptop 13 Pro (Intel Core Ultra Series 3)',
    port = 0xe00,
    signature = { 0x20, 0x45, 0x43 },
    reads = function()
      return {
        { 0x00, 0x10 },
        { 0x10, 0x08 },
        { 0x18, 0x08 },
        { 0x20, 0x08 },
        { 0x40, 0x20 },
      }
    end,
  },
  {
    manufacturer = 'Framework',
    product = 'Laptop 12 (13th Gen Intel Core)',
    port = 0xe00,
    signature = { 0x20, 0x45, 0x43 },
    reads = function()
      return {
        { 0x00, 0x10 },
        { 0x10, 0x08 },
        { 0x18, 0x08 },
        { 0x20, 0x08 },
        { 0x40, 0x20 },
      }
    end,
  },
  {
    manufacturer = 'Framework',
    product = 'Laptop 12 (Intel Core Series 3)',
    port = 0xe00,
    signature = { 0x20, 0x45, 0x43 },
    reads = function()
      return {
        { 0x00, 0x10 },
        { 0x10, 0x08 },
        { 0x18, 0x08 },
        { 0x20, 0x08 },
        { 0x40, 0x20 },
      }
    end,
  },
  {
    manufacturer = 'Framework',
    product = 'Laptop 16 (AMD Ryzen 7040 Series)',
    port = 0xe00,
    signature = { 0x20, 0x45, 0x43 },
    reads = function()
      return {
        { 0x00, 0x10 },
        { 0x10, 0x08 },
        { 0x18, 0x08 },
        { 0x20, 0x08 },
        { 0x40, 0x20 },
      }
    end,
  },
  {
    manufacturer = 'Framework',
    product = 'Laptop 16 (AMD Ryzen AI 300 Series)',
    port = 0xe00,
    signature = { 0x20, 0x45, 0x43 },
    reads = function()
      return {
        { 0x00, 0x10 },
        { 0x10, 0x08 },
        { 0x18, 0x08 },
        { 0x20, 0x08 },
        { 0x40, 0x20 },
      }
    end,
  },
  {
    manufacturer = 'Framework',
    product = 'Desktop (AMD Ryzen AI Max 300 Series)',
    port = 0xe00,
    signature = { 0x20, 0x45, 0x43 },
    reads = function()
      return {
        { 0x00, 0x10 },
        { 0x10, 0x08 },
        { 0x18, 0x08 },
        { 0x20, 0x08 },
        { 0x40, 0x20 },
      }
    end,
  },
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

-- The row of this board: from the table once in a kept script, nil when there
-- is none.
local function board()
  if kept.board ~= nil then
    return kept.board or nil
  end
  local manufacturer, product = system_strings()
  if not manufacturer then
    ksi.trace('no SMBIOS system structure')
    return nil
  end
  manufacturer, product = normalized(manufacturer), normalized(product)
  local found = false
  for _, row in ipairs(ROWS) do
    if normalized(row.manufacturer) == manufacturer and normalized(row.product) == product then
      found = { row = row, ranges = row.reads() }
      break
    end
  end
  if not ksi.volatile then
    kept.board = found
  end
  return found or nil
end

local function signature_present(b)
  local signature = b.row.signature
  for i = 2, #signature do
    if ksi.hw.port_read(b.row.port + signature[1] + i - 2, 1) ~= signature[i] then
      return false
    end
  end
  return true
end

local function permitted(ranges, offset)
  for _, range in ipairs(ranges) do
    if offset >= range[1] and offset - range[1] < range[2] then
      return true
    end
  end
  return false
end

-- Returns the identification bytes of the embedded controller's memory map:
-- the signature and the version bytes after it.
function identify(input)
  if not ksi.hw.port_read or not ksi.hw.firmware_table then
    ksi.trace('no port or firmware table functions')
    return ksi.STATUS_NOT_SUPPORTED
  end
  if #input ~= 0 then
    ksi.trace('input of %d bytes', #input)
    return ksi.STATUS_INVALID_PARAMETER
  end
  local b = board()
  if not b then
    ksi.trace('no row for this board')
    return ksi.STATUS_NOT_SUPPORTED
  end
  local lock <close>, failed = ksi.lock('isa')
  if not lock then
    ksi.trace('the isa lock was not taken: 0x%08x', failed)
    return failed
  end
  if not signature_present(b) then
    ksi.trace('no signature at 0x%x', b.row.port + b.row.signature[1])
    return ksi.STATUS_NOT_FOUND
  end
  local signature = b.row.signature
  local bytes = {}
  for i = 0, 7 do
    local offset = signature[1] + i
    local value = 0
    if i < #signature - 1 then
      value = signature[i + 2]
    elseif permitted(b.ranges, offset) then
      value = ksi.hw.port_read(b.row.port + offset, 1)
    end
    bytes[#bytes + 1] = string.char(value)
  end
  return ksi.STATUS_SUCCESS, table.concat(bytes)
end

-- Reads bytes of the embedded controller's memory map.
function read(input)
  if not ksi.hw.port_read or not ksi.hw.firmware_table then
    ksi.trace('no port or firmware table functions')
    return ksi.STATUS_NOT_SUPPORTED
  end
  if #input == 0 or #input % 4 ~= 0 then
    ksi.trace('input of %d bytes', #input)
    return ksi.STATUS_INVALID_PARAMETER
  end
  local b = board()
  if not b then
    ksi.trace('no row for this board')
    return ksi.STATUS_NOT_SUPPORTED
  end
  local count = #input // 4
  local offsets = {}
  local any = false
  for i = 1, count do
    local offset = string.unpack('<I4', input, i * 4 - 3)
    if permitted(b.ranges, offset) then
      offsets[i] = offset
      any = true
    else
      ksi.trace('0x%x is not permitted', offset)
    end
  end
  if not any then
    ksi.trace('nothing permitted')
    return ksi.STATUS_INVALID_PARAMETER
  end
  local lock <close>, failed = ksi.lock('isa')
  if not lock then
    ksi.trace('the isa lock was not taken: 0x%08x', failed)
    return failed
  end
  if not signature_present(b) then
    ksi.trace('no signature at 0x%x', b.row.port + b.row.signature[1])
    return ksi.STATUS_NOT_FOUND
  end
  local values = {}
  for i = 1, count do
    if offsets[i] then
      values[i] = ksi.hw.port_read(b.row.port + offsets[i], 1)
    end
  end
  local present = signature_present(b)
  if not present then
    ksi.trace('the signature went away during the reads')
  end
  local output = {}
  for i = 1, count do
    if not offsets[i] then
      output[i] = string.pack('<I4I4', ksi.STATUS_ACCESS_DENIED, 0)
    elseif present then
      output[i] = string.pack('<I4I4', ksi.STATUS_SUCCESS, values[i])
    else
      output[i] = string.pack('<I4I4', ksi.STATUS_DEVICE_NOT_READY, 0)
    end
  end
  return ksi.STATUS_SUCCESS, table.concat(output)
end
