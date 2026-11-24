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
-- Reads the die temperature of an AMD Promontory 21 chipset through the index
-- and data pair in its USB controller's memory BAR, under the pci lock, writing
-- the saved index back whatever ends the read.
--

local nt = require('nt')

local VENDOR = 0x1022
local DEVICES =
  { [0x43f7] = true, [0x43f9] = true, [0x43fa] = true, [0x43fc] = true, [0x43fd] = true }
local INDEX = 0x3000
local DATA = 0x3008
local SELECTOR = 0x0001e520
local PAGE = 0x1000
local DIES_MAX = 8

local kept = {}

local function header_key(header)
  return string.sub(header, 1, 4) .. string.sub(header, 9, 12) .. string.sub(header, 15, 15)
end

local function address_bits()
  if kept.bits then
    return kept.bits
  end
  local results, status = ksi.hw.cpuid(
    string.pack('<I2I2I4I4', 0, 0, 0x80000000, 0) .. string.pack('<I2I2I4I4', 0, 0, 0x80000008, 0)
  )
  if not results then
    ksi.trace('cpuid failed: 0x%x', status)
    return nil, status
  end
  local s0, maximum = string.unpack('<i4I4', results, 1)
  local s1, eax = string.unpack('<i4I4', results, 21)
  local bits = 36
  if
    nt.NT_SUCCESS(s0)
    and maximum >= 0x80000008
    and nt.NT_SUCCESS(s1)
    and eax & 0xff >= 32
    and eax & 0xff < 64
  then
    bits = eax & 0xff
  end
  if not ksi.volatile then
    kept.bits = bits
  end
  return bits
end

local function release(s)
  if s.map then
    s.map:release()
    s.map = nil
  end
  s.device:release()
end

-- The function at a locator, IDs checked, kept while its header is unchanged;
-- nil and a status.
local function session(locator)
  local k = kept[locator]
  if k then
    local header = ksi.hw.pci_read_block(k.device, 0, 16)
    if header and header_key(header) == k.key then
      return k
    end
    ksi.trace('the function kept for 0x%x changed', locator)
    kept[locator] = nil
    release(k)
  end
  local device, status =
    nt.pci.open(locator >> 16, (locator >> 8) & 0xff, (locator >> 3) & 0x1f, locator & 0x7)
  if not device then
    ksi.trace('no PCI function at 0x%x: 0x%x', locator, status)
    return nil, status
  end
  local header, readFailed = ksi.hw.pci_read_block(device, 0, 16)
  if not header then
    local refused = nt.pci.refused(device, readFailed)
    device:release()
    return nil, refused
  end
  local vendor, id = string.unpack('<I2I2', header, 1)
  if vendor ~= VENDOR or not DEVICES[id] then
    ksi.trace('%04x:%04x is not a listed controller', vendor, id)
    device:release()
    return nil, nt.STATUS_NOT_SUPPORTED
  end
  if string.byte(header, 15) & 0x7f ~= 0 then
    ksi.trace('header type 0x%x', string.byte(header, 15))
    device:release()
    return nil, nt.STATUS_DEVICE_CONFIGURATION_ERROR
  end
  local s = { device = device, key = header_key(header), kept = false }
  if not ksi.volatile and not nt.is_device_removable(device) then
    s.kept = true
    kept[locator] = s
  end
  return s
end

local function config(s, offset, width)
  local value = ksi.hw.pci_read(s.device, offset, width)
  if not value or value == (1 << (width * 8)) - 1 then
    return nil
  end
  return value
end

-- STATUS_SUCCESS when the function is in D0 (or has no power management), a
-- status otherwise.
local function in_d0(s)
  if s.pm == nil then
    local pciStatus = config(s, 0x06, 2)
    if not pciStatus then
      return nt.STATUS_DEVICE_NOT_READY
    end
    local pm = 0
    if pciStatus & 0x10 ~= 0 then
      local pointer = config(s, 0x34, 1)
      local visited = {}
      pointer = pointer and pointer & 0xfc or 0
      while pointer ~= 0 do
        if pointer < 0x40 or visited[pointer] then
          return nt.STATUS_DEVICE_CONFIGURATION_ERROR
        end
        visited[pointer] = true
        local capability = config(s, pointer, 2)
        if not capability then
          return nt.STATUS_DEVICE_NOT_READY
        end
        if capability & 0xff == 0x01 then
          pm = pointer
          break
        end
        pointer = (capability >> 8) & 0xfc
      end
    end
    s.pm = pm
  end
  if s.pm == 0 then
    return nt.STATUS_SUCCESS
  end
  local pmcsr = config(s, s.pm + 4, 2)
  if not pmcsr then
    return nt.STATUS_DEVICE_NOT_READY
  end
  if pmcsr & 0x3 ~= 0 then
    ksi.trace('the controller is in D%d', pmcsr & 0x3)
    return nt.STATUS_DEVICE_POWERED_OFF
  end
  return nt.STATUS_SUCCESS
end

-- BAR 0's base while the function decodes memory, or nil and a status.
local function bar(s)
  local command = config(s, 0x04, 2)
  if not command or command & 0x2 == 0 then
    ksi.trace('the controller does not decode memory')
    return nil, nt.STATUS_DEVICE_NOT_READY
  end
  local low = config(s, 0x10, 4)
  if not low then
    return nil, nt.STATUS_DEVICE_NOT_READY
  end
  if low & 0x1 ~= 0 then
    return nil, nt.STATUS_DEVICE_CONFIGURATION_ERROR
  end
  local base = low & 0xfffffff0
  if low & 0x6 == 0x4 then
    local high = ksi.hw.pci_read(s.device, 0x14, 4)
    if not high then
      return nil, nt.STATUS_DEVICE_NOT_READY
    end
    base = (high << 32) | base
  elseif low & 0x6 ~= 0 then
    return nil, nt.STATUS_DEVICE_CONFIGURATION_ERROR
  end
  if base == 0 then
    return nil, nt.STATUS_DEVICE_NOT_READY
  end
  return base
end

-- The writable page holding the pair, mapped once per base after checking it
-- lies in a memory resource.
local function page(s, base)
  if s.map and s.base == base then
    return s.map
  end
  if s.map then
    s.map:release()
    s.map = nil
  end
  local resources, failed = nt.resources(s.device)
  if not resources then
    return nil, failed
  end
  local length
  for _, r in ipairs(resources) do
    if r.kind == 1 and r.raw == base and r.translated == base then
      length = r.length
    end
  end
  if not length or length < DATA + 4 then
    ksi.trace('BAR 0x%x is not a memory resource holding the pair', base)
    return nil, nt.STATUS_DEVICE_CONFIGURATION_ERROR
  end
  local bits, bitsFailed = address_bits()
  if not bits then
    return nil, bitsFailed
  end
  local st = nt.check_range(base + INDEX, base + INDEX + PAGE, bits)
  if not nt.NT_SUCCESS(st) then
    return nil, st
  end
  local m, mapFailed = ksi.hw.map(base + INDEX, PAGE, true)
  if not m then
    ksi.trace('page 0x%x not mapped: 0x%x', base + INDEX, mapFailed)
    return nil, mapFailed
  end
  s.map, s.base = m, base
  return m
end

-- One die's raw reading: the data register's low byte, or nil and a status.
local function temperature(s)
  local st = in_d0(s)
  if not nt.NT_SUCCESS(st) then
    return nil, st
  end
  local base, failed = bar(s)
  if not base then
    return nil, failed
  end
  local m, mapFailed = page(s, base)
  if not m then
    return nil, mapFailed
  end
  local lock <close>, refused = ksi.lock('pci')
  if not lock then
    return nil, refused
  end
  local saved = ksi.hw.map_read(m, 0, 4)
  if saved == 0xffffffff then
    ksi.trace('the index reads all-ones')
    return nil, nt.STATUS_DEVICE_NOT_READY
  end
  local restore <close> = setmetatable({}, {
    __close = function()
      ksi.hw.map_write(m, 0, 4, saved)
      ksi.hw.map_read(m, 0, 4)
    end,
  })
  ksi.hw.map_write(m, 0, 4, SELECTOR)
  local data = ksi.hw.map_read(m, DATA - INDEX, 4)
  if data == 0xffffffff then
    ksi.trace('the data reads all-ones')
    return nil, nt.STATUS_DEVICE_NOT_READY
  end
  return data & 0xff
end

-- Per 4-byte locator (segment << 16 | bus << 8 | device << 3 | function): an i4
-- status and the raw byte as u4; 0 is no reading.
function read(input)
  if not ksi.hw.pci_open or not ksi.hw.map or not ksi.hw.cpuid then
    ksi.trace('no PCI, memory or CPUID functions')
    return nt.STATUS_NOT_SUPPORTED
  end
  if #input == 0 or #input % 4 ~= 0 or #input > 4 * DIES_MAX then
    ksi.trace('input of %d bytes', #input)
    return nt.STATUS_INVALID_PARAMETER
  end
  local output = {}
  for at = 1, #input, 4 do
    local s, refused = session(string.unpack('<I4', input, at))
    local raw, status = nil, refused
    if s then
      raw, status = temperature(s)
      if not s.kept then
        release(s)
      end
    end
    if raw then
      output[#output + 1] = string.pack('<i4I4', nt.STATUS_SUCCESS, raw)
    else
      output[#output + 1] = string.pack('<i4I4', status, 0)
    end
  end
  return nt.STATUS_SUCCESS, table.concat(output)
end
