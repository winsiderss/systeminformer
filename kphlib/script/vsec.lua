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
-- Reads Intel telemetry regions (PMT and TPMI) of the PCI functions listed
-- below. The regions are found by walking the function's extended
-- capabilities to its discovery tables. Each operation of a row selects
-- regions of one discovery kind and lists the ranges a read must lie inside.
--

local nt = require('nt')

local ROWS = {
  {
    vendor = 0x8086,
    devices = { 0x9a0d, 0x467d, 0xa77d },
    operations = function()
      return {
        {
          discovery = 2,
          selector = 0x0,
          shift = true,
          early = true,
          regions = 16,
          size = 0x40000,
          ranges = {
            { 0x0, 0x40000, 8 },
            { 0x0, 0x40000, 4 },
          },
        },
      }
    end,
  },
  {
    vendor = 0x8086,
    devices = { 0x7d0d, 0xad0d, 0x647d, 0xb07d, 0xfd7d, 0xd70d },
    operations = function()
      return {
        {
          discovery = 2,
          selector = 0x0,
          regions = 16,
          size = 0x40000,
          ranges = {
            { 0x0, 0x40000, 8 },
            { 0x0, 0x40000, 4 },
          },
        },
      }
    end,
  },
  {
    vendor = 0x8086,
    devices = { 0x09a7, 0x09a1 },
    operations = function()
      return {
        {
          discovery = 2,
          selector = 0x0,
          regions = 32,
          size = 0x40000,
          ranges = {
            { 0x0, 0x40000, 8 },
          },
        },
        {
          discovery = 66,
          selector = 0x0,
          regions = 16,
          size = 0x1000,
          ranges = {
            { 0x0, 0x1000, 8 },
          },
        },
        {
          discovery = 66,
          selector = 0x2,
          regions = 16,
          size = 0x1000,
          ranges = {
            { 0x0, 0x1000, 8 },
          },
        },
        {
          discovery = 66,
          selector = 0x5,
          regions = 16,
          size = 0x1000,
          ranges = {
            { 0x0, 0x1000, 8 },
          },
        },
        {
          discovery = 66,
          selector = 0xc,
          regions = 16,
          size = 0x1000,
          ranges = {
            { 0x0, 0x8, 8 },
            { 0x18, 0x8, 8 },
          },
        },
        {
          discovery = 66,
          selector = 0x81,
          regions = 16,
          size = 0x1000,
          ranges = {
            { 0x0, 0x10, 8 },
          },
        },
      }
    end,
  },
}

local PAGE = 0x1000
local HUGE_PAGE_END = -PAGE
local KIND_PMT = 2
local CAP_VSEC = 0x000b
local CAP_DVSEC = 0x0023
local REVISION = 1
local DVSEC_VENDOR = 0x8086
local EXTENDED_START = 0x100
local EXTENDED_TTL = (0x1000 - 0x100) // 8
local MATCHES_MAX = 8
local PMT_DWORDS = 3
local TPMI_DWORDS = 2
local PMT_LOCAL = 3
local PMT_BAR = 2
local PMT_RESERVED = 0xf
local PMT_PUNIT_FIXED = 2
local PMT_FIXED_GUID = 0x10000000
local TPMI_UNIT = 1024

local kept = {}

local function header_key(header)
  return string.sub(header, 1, 4) .. string.sub(header, 9, 12) .. string.sub(header, 15, 15)
end

local function find_row(vendor, device)
  for _, row in ipairs(ROWS) do
    if row.vendor == vendor then
      for _, id in ipairs(row.devices) do
        if id == device then
          return row
        end
      end
    end
  end
  return nil
end

-- Identifies the PCI function at a locator. Returns the session, or nil and
-- the status of the call.
local function session(locator)
  if not ksi.hw.pci_open then
    ksi.trace('no PCI functions')
    return nil, nt.STATUS_NOT_SUPPORTED
  end
  local k = kept[locator]
  if k then
    local header = ksi.hw.pci_read_block(k.device, 0, 16)
    if header and header_key(header) == k.key then
      return {
        device = k.device,
        kept = k,
        vendor = k.vendor,
        id = k.id,
        operations = k.operations,
        header = k.type,
      }
    end
    ksi.trace('the function kept for 0x%x changed', locator)
    kept[locator] = nil
    for _, w in pairs(k.maps) do
      w.map:release()
    end
    k.device:release()
  end
  local device, status =
    ksi.hw.pci_open(locator >> 16, (locator >> 8) & 0xff, (locator >> 3) & 0x1f, locator & 0x7)
  if not device then
    ksi.trace('no PCI function at 0x%x: 0x%x', locator, status)
    return nil, status
  end
  local header, readFailed = ksi.hw.pci_read_block(device, 0, 16)
  if not header then
    ksi.trace('configuration header of 0x%x not read: 0x%x', locator, readFailed)
    device:release()
    return nil, readFailed
  end
  local vendor, id = string.unpack('<I2I2', header, 1)
  if vendor == 0xffff then
    ksi.trace('no device answers at 0x%x', locator)
    device:release()
    return nil, nt.STATUS_NOT_FOUND
  end
  local row = find_row(vendor, id)
  if not row then
    ksi.trace('no row for PCI %04x:%04x', vendor, id)
    device:release()
    return nil, nt.STATUS_NOT_SUPPORTED
  end
  if nt.is_device_removable(device) then
    ksi.trace('%04x:%04x may be removed', vendor, id)
    device:release()
    return nil, nt.STATUS_NOT_SUPPORTED
  end
  local s = {
    device = device,
    vendor = vendor,
    id = id,
    operations = row.operations(),
    header = string.byte(header, 15) & 0x7f,
  }
  if not ksi.volatile then
    s.kept = {
      device = device,
      key = header_key(header),
      vendor = vendor,
      id = id,
      operations = s.operations,
      type = s.header,
      walks = {},
      maps = {},
    }
    kept[locator] = s.kept
  end
  return s
end

local function drop(s, locator)
  local k = s.kept
  if k and kept[locator] == k then
    kept[locator] = nil
    for _, w in pairs(k.maps) do
      w.map:release()
    end
    k.device:release()
  end
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
  if
    s0 == nt.STATUS_SUCCESS
    and maximum >= 0x80000008
    and s1 == nt.STATUS_SUCCESS
    and eax & 0xff >= 32
    and eax & 0xff < 64
  then
    kept.bits = eax & 0xff
    return kept.bits
  end
  if s0 == nt.STATUS_SUCCESS and s1 == nt.STATUS_SUCCESS then
    kept.bits = 36
  end
  return 36
end

local function check_range(first, last, bits)
  if math.ult(1 << bits, last) then
    ksi.trace('range end 0x%x beyond %d address bits', last, bits)
    return nt.STATUS_CONFLICTING_ADDRESSES
  end
  local nvdimm = ksi.hw.is_nvdimm(first, last - first)
  if nvdimm == nil then
    ksi.trace('NVDIMM ranges unknown')
    return nt.STATUS_DEVICE_CONFIGURATION_ERROR
  end
  if nvdimm then
    ksi.trace('range 0x%x-0x%x overlaps NVDIMM', first, last)
    return nt.STATUS_CONFLICTING_ADDRESSES
  end
  local ram, ramFailed = ksi.hw.is_ram(first, last - first)
  if ram == nil then
    ksi.trace('memory ranges unknown: 0x%x', ramFailed)
    return ramFailed
  end
  if ram then
    ksi.trace('range 0x%x-0x%x overlaps memory', first, last)
    return nt.STATUS_CONFLICTING_ADDRESSES
  end
  return nt.STATUS_SUCCESS
end

local function config_read(s, offset, width, ones)
  local value = ksi.hw.pci_read(s.device, offset, width)
  if not value or value == ones then
    ksi.trace('configuration 0x%x of %04x:%04x not read', offset, s.vendor, s.id)
    return nil
  end
  return value
end

-- The power-management capability: 0 when the device has none, nil and a
-- status when the list cannot be walked.
local function pm_capability(s)
  local pciStatus = config_read(s, 0x06, 2, 0xffff)
  if not pciStatus then
    return nil, nt.STATUS_DEVICE_NOT_READY
  end
  if pciStatus & 0x10 == 0 then
    return 0
  end
  local pointerOffset
  if s.header == 0 or s.header == 1 then
    pointerOffset = 0x34
  elseif s.header == 2 then
    pointerOffset = 0x14
  else
    ksi.trace('header type 0x%x has no capability list', s.header)
    return nil, nt.STATUS_DEVICE_CONFIGURATION_ERROR
  end
  local pointer = ksi.hw.pci_read(s.device, pointerOffset, 1)
  if not pointer then
    ksi.trace('capability pointer of %04x:%04x not read', s.vendor, s.id)
    return nil, nt.STATUS_DEVICE_NOT_READY
  end
  local visited = {}
  pointer = pointer & 0xfc
  while pointer ~= 0 do
    if pointer < 0x40 then
      ksi.trace('capability at 0x%x inside the header', pointer)
      return nil, nt.STATUS_DEVICE_CONFIGURATION_ERROR
    end
    if visited[pointer] then
      ksi.trace('capability list loops at 0x%x', pointer)
      return nil, nt.STATUS_DEVICE_CONFIGURATION_ERROR
    end
    visited[pointer] = true
    local capability = config_read(s, pointer, 2, 0xffff)
    if not capability then
      return nil, nt.STATUS_DEVICE_NOT_READY
    end
    if capability & 0xff == 0x01 then
      return pointer
    end
    pointer = (capability >> 8) & 0xfc
  end
  return 0
end

local function check_d0(s)
  if not s.pm then
    local pm, failed = pm_capability(s)
    if not pm then
      return failed
    end
    s.pm = pm
  end
  local pm = s.pm
  if pm == 0 then
    return nt.STATUS_SUCCESS
  end
  local pmcsr = config_read(s, pm + 4, 2, 0xffff)
  if not pmcsr then
    return nt.STATUS_DEVICE_NOT_READY
  end
  if pmcsr & 0x3 ~= 0 then
    ksi.trace('%04x:%04x is in D%d', s.vendor, s.id, pmcsr & 0x3)
    return nt.STATUS_DEVICE_POWERED_OFF
  end
  return nt.STATUS_SUCCESS
end

-- A memory BAR's base and its register. On failure, nil, the status and
-- whether a configuration read failed.
local function bar_base(s, index)
  local command = config_read(s, 0x04, 2, 0xffff)
  if not command then
    return nil, nt.STATUS_DEVICE_NOT_READY, true
  end
  if command & 0x2 == 0 then
    ksi.trace('%04x:%04x does not decode memory', s.vendor, s.id)
    return nil, nt.STATUS_DEVICE_NOT_READY
  end
  local count
  if s.header == 0 then
    count = 6
  elseif s.header == 1 then
    count = 2
  else
    ksi.trace('header type 0x%x has no BARs', s.header)
    return nil, nt.STATUS_DEVICE_CONFIGURATION_ERROR
  end
  local base, register
  local i = 0
  while i < count and i <= index do
    local low = config_read(s, 0x10 + i * 4, 4, 0xffffffff)
    if not low then
      return nil, nt.STATUS_DEVICE_NOT_READY, true
    end
    if low & 0x1 ~= 0 or low & 0x6 ~= 0x4 then
      if i == index then
        if low & 0x1 ~= 0 or low & 0x6 ~= 0 then
          ksi.trace('BAR %d is not 32-bit memory: 0x%x', i, low)
          return nil, nt.STATUS_DEVICE_CONFIGURATION_ERROR
        end
        base, register = low & 0xfffffff0, low
        break
      end
      i = i + 1
    else
      if i + 1 >= count or i + 1 == index then
        ksi.trace('BAR %d is the high half of BAR %d', index, i)
        return nil, nt.STATUS_DEVICE_CONFIGURATION_ERROR
      end
      if i == index then
        local high = config_read(s, 0x10 + i * 4 + 4, 4, 0xffffffff)
        if not high then
          return nil, nt.STATUS_DEVICE_NOT_READY, true
        end
        base, register = (high << 32) | (low & 0xfffffff0), low
        break
      end
      i = i + 2
    end
  end
  if not base then
    ksi.trace('BAR %d beyond the %d BARs', index, count)
    return nil, nt.STATUS_DEVICE_CONFIGURATION_ERROR
  end
  if base == 0 then
    ksi.trace('BAR %d of %04x:%04x is not assigned', index, s.vendor, s.id)
    return nil, nt.STATUS_DEVICE_NOT_READY
  end
  return base, register
end

-- A memory BAR: { base, length, register }. On failure, nil, the status and
-- whether a configuration read failed.
local function memory_bar(s, index)
  local base, register, readFailed = bar_base(s, index)
  if not base then
    return nil, register, readFailed
  end
  local resources, failed = ksi.hw.pci_resources(s.device)
  if not resources then
    ksi.trace('resources of %04x:%04x not read: 0x%x', s.vendor, s.id, failed)
    return nil, nt.STATUS_DEVICE_CONFIGURATION_ERROR
  end
  for _, r in ipairs(resources) do
    if r.kind == 1 and r.raw == base then
      if r.translated ~= base then
        ksi.trace('BAR 0x%x translates to 0x%x', base, r.translated)
        return nil, nt.STATUS_DEVICE_CONFIGURATION_ERROR
      end
      return { base = base, length = r.length, register = register }
    end
  end
  ksi.trace('no memory resource at 0x%x', base)
  return nil, nt.STATUS_DEVICE_CONFIGURATION_ERROR
end

-- A BAR derived once per walk: { base, length, register }, or nil, the
-- status and whether a read failed.
local function bar(s, bars, index)
  local b = bars[index]
  if not b then
    local failed, readFailed
    b, failed, readFailed = memory_bar(s, index)
    if not b then
      if readFailed then
        return nil, failed, true
      end
      b = { failed = failed }
    end
    bars[index] = b
  end
  if b.failed then
    return nil, b.failed
  end
  return b
end

-- The configuration offsets of the capabilities of a discovery kind, or nil,
-- the status and whether a read failed.
local function capabilities(s, kind)
  local found = {}
  local position = EXTENDED_START
  for _ = 1, EXTENDED_TTL do
    local header = ksi.hw.pci_read(s.device, position, 4)
    if not header then
      return nil, nt.STATUS_DEVICE_NOT_READY, true
    end
    if header == 0xffffffff then
      if position == EXTENDED_START then
        return found
      end
      ksi.trace('extended capability at 0x%x read all-ones', position)
      return nil, nt.STATUS_DEVICE_NOT_READY, true
    end
    local id = header & 0xffff
    local match = false
    if (id == CAP_VSEC or id == CAP_DVSEC) and position + 0x10 <= 0x1000 then
      local header1 = config_read(s, position + 4, 4, 0xffffffff)
      if not header1 then
        return nil, nt.STATUS_DEVICE_NOT_READY, true
      end
      if (header1 >> 16) & 0xf == REVISION and header1 >> 20 >= 0x10 then
        if id == CAP_VSEC then
          match = header1 & 0xffff == kind
        elseif header1 & 0xffff == DVSEC_VENDOR then
          local dvsecId = ksi.hw.pci_read(s.device, position + 8, 2)
          if not dvsecId then
            return nil, nt.STATUS_DEVICE_NOT_READY, true
          end
          match = dvsecId == kind
        end
      end
    end
    if match then
      if #found >= MATCHES_MAX then
        ksi.trace('discovery capability at 0x%x beyond %d', position, MATCHES_MAX)
        return nil, nt.STATUS_DEVICE_CONFIGURATION_ERROR
      end
      found[#found + 1] = position
    end
    local link = (header >> 20) & 0xffc
    if link == 0 then
      return found
    end
    if link < EXTENDED_START then
      ksi.trace('extended capability at 0x%x links to 0x%x', position, link)
      return nil, nt.STATUS_DEVICE_CONFIGURATION_ERROR
    end
    position = link
  end
  ksi.trace('extended capability chain does not end')
  return nil, nt.STATUS_DEVICE_CONFIGURATION_ERROR
end

-- The discovery table of the capability at a position: its first dwords of
-- every entry. Nil and nil when the capability is skipped; nil, the status
-- and whether a read failed when the walk fails.
local function table_of(s, op, position, dwords, bars)
  local entries = ksi.hw.pci_read(s.device, position + 0xa, 1)
  local entrySize = ksi.hw.pci_read(s.device, position + 0xb, 1)
  local register = ksi.hw.pci_read(s.device, position + 0xc, 4)
  if not entries or not entrySize or not register or register == 0xffffffff then
    ksi.trace('discovery header at 0x%x not read', position)
    return nil, nt.STATUS_DEVICE_NOT_READY, true
  end
  local t = { bar = register & 0x7, offset = register & 0xfffffff8, register = register }
  if op.shift then
    t.offset = t.offset >> 3
  end
  if entries == 0 or entrySize < dwords or t.bar >= 6 or t.offset & 0x3 ~= 0 then
    ksi.trace('discovery table at 0x%x skipped', position)
    return nil
  end
  local b, failed, readFailed = bar(s, bars, t.bar)
  if not b then
    return nil, failed, readFailed
  end
  local base, length = b.base, b.length
  t.base, t.length, t.stride, t.barRegister = base, length, entrySize * 4, b.register
  local tableLength = entries * t.stride
  if t.offset + tableLength > 0xffffffff or t.offset + tableLength > length then
    ksi.trace('discovery table at 0x%x leaves BAR %d', position, t.bar)
    return nil
  end
  local first = base + t.offset
  local bits, bitsFailed = address_bits()
  if not bits then
    return nil, bitsFailed
  end
  local last = (first + tableLength + PAGE - 1) & -PAGE
  if math.ult(last, first) or math.ult(HUGE_PAGE_END, last) then
    ksi.trace('discovery table 0x%x ends beyond the address space', first)
    return nil, nt.STATUS_DEVICE_CONFIGURATION_ERROR
  end
  local st = check_range(first & -PAGE, last, bits)
  if st ~= nt.STATUS_SUCCESS then
    return nil, st
  end
  local start = first & -8
  local m <close>, mapFailed = ksi.hw.map(start, first - start + tableLength)
  if not m then
    ksi.trace('discovery table 0x%x not mapped: 0x%x', first, mapFailed)
    return nil, mapFailed
  end
  local skip = first - start
  for i = 0, entries - 1 do
    for j = 0, dwords - 1 do
      t[#t + 1] = ksi.hw.map_read(m, skip + i * t.stride + j * 4, 4)
    end
  end
  t.count = entries
  return t
end

-- The region of PMT entry i of a table, or nil and nil when it is skipped,
-- or nil, the status and true when the walk fails.
local function pmt_region(s, op, t, i, bars)
  local header, guid, register = t[i * PMT_DWORDS + 1], t[i * PMT_DWORDS + 2], t[i * PMT_DWORDS + 3]
  local access = header & 0xf
  local size = ((header >> 12) & 0xffff) * 4
  if size == 0 or access == PMT_RESERVED then
    return nil
  end
  if op.early and ((header >> 4) & 0xf == PMT_PUNIT_FIXED or guid == PMT_FIXED_GUID) then
    return nil
  end
  if op.selector ~= 0 and guid ~= op.selector then
    return nil
  end
  if size > op.size then
    return nil
  end
  local index, offset
  if access == PMT_LOCAL then
    if register & 0x7 ~= 0 then
      return nil
    end
    index = t.bar
    if op.early then
      offset = register
    else
      offset = t.offset + (i + 1) * t.stride + register
    end
  elseif access == PMT_BAR then
    index, offset = register & 0x7, register & 0xfffffff8
    if index >= 6 then
      return nil
    end
  else
    return nil
  end
  local b, failed, readFailed = bar(s, bars, index)
  if not b then
    if readFailed then
      return nil, failed, true
    end
    return nil
  end
  if offset + size > b.length then
    return nil
  end
  return {
    base = b.base + offset,
    size = size,
    id = guid,
    bar = index,
    barBase = b.base,
    barRegister = b.register,
  }
end

-- Appends the regions of TPMI entry i of a table: one per instance, until an
-- instance leaves the BAR or the operation has its regions.
local function tpmi_regions(op, t, i, position, regions)
  local header, location = t[i * TPMI_DWORDS + 1], t[i * TPMI_DWORDS + 2]
  if header & 0xff ~= op.selector then
    return
  end
  local size = ((header >> 16) & 0xffff) * 4
  if size == 0 or size > op.size then
    return
  end
  local offset = t.offset + (location & 0xffff) * TPMI_UNIT
  for instance = 0, ((header >> 8) & 0xff) - 1 do
    if #regions >= op.regions or offset + instance * size + size > t.length then
      return
    end
    regions[#regions + 1] = {
      base = t.base + offset + instance * size,
      size = size,
      id = op.selector,
      position = (position << 16) | (i << 8) | instance,
      bar = t.bar,
      barBase = t.base,
      barRegister = t.barRegister,
      capability = position,
      tableRegister = t.register,
    }
  end
end

-- The regions of an operation: each { base, size, id, position } and what
-- locates it (its BAR and register, its capability and table register). Nil,
-- the status and whether a read failed when the walk fails.
local function walk(s, op)
  local st = check_d0(s)
  if st ~= nt.STATUS_SUCCESS then
    return nil, st, st == nt.STATUS_DEVICE_NOT_READY
  end
  local positions, failed, readFailed = capabilities(s, op.discovery)
  if not positions then
    return nil, failed, readFailed
  end
  local dwords = op.discovery == KIND_PMT and PMT_DWORDS or TPMI_DWORDS
  local bars = {}
  local regions = {}
  for _, position in ipairs(positions) do
    if #regions >= op.regions then
      break
    end
    local t, tableFailed, tableRead = table_of(s, op, position, dwords, bars)
    if not t and tableFailed then
      return nil, tableFailed, tableRead
    end
    if t and op.discovery == KIND_PMT then
      for i = 0, t.count - 1 do
        if #regions >= op.regions then
          break
        end
        local r, regionFailed, regionRead = pmt_region(s, op, t, i, bars)
        if regionFailed then
          return nil, regionFailed, regionRead
        end
        if r then
          r.position = (position << 16) | (i << 8)
          r.capability, r.tableRegister = position, t.register
          regions[#regions + 1] = r
        end
      end
    elseif t then
      for i = 0, t.count - 1 do
        tpmi_regions(op, t, i, position, regions)
      end
    end
  end
  return regions
end

-- Returns the PCI IDs at a locator and the number of operations of its row.
function identify(input)
  if #input ~= 4 then
    ksi.trace('input of %d bytes', #input)
    return nt.STATUS_INVALID_PARAMETER
  end
  local s, refused = session(string.unpack('<I4', input))
  if not s then
    return refused
  end
  if not s.kept then
    s.device:release()
  end
  return nt.STATUS_SUCCESS, string.pack('<I2I2I4', s.vendor, s.id, #s.operations)
end

-- Lists the regions of every operation of the function at a locator.
function regions(input)
  if #input ~= 4 then
    ksi.trace('input of %d bytes', #input)
    return nt.STATUS_INVALID_PARAMETER
  end
  local locator = string.unpack('<I4', input)
  local s, refused = session(locator)
  if not s then
    return refused
  end
  local out = {}
  for index, op in ipairs(s.operations) do
    local list, failed, readFailed = walk(s, op)
    if not list then
      if not s.kept then
        s.device:release()
      elseif readFailed then
        drop(s, locator)
      end
      return failed
    end
    for i, r in ipairs(list) do
      out[#out + 1] =
        string.pack('<I1I1I2I4I4I4', index - 1, op.discovery, i - 1, r.id, r.size, r.position)
    end
  end
  if not s.kept then
    s.device:release()
  end
  return nt.STATUS_SUCCESS, table.concat(out)
end

-- The regions of an operation, walked or kept. A kept list is checked by the
-- asked region's BAR register and its capability's table register; nil and
-- nil when they changed.
local function regions_of(s, index, op, position)
  local k = s.kept
  local list = k and k.walks[index]
  if list then
    for _, r in ipairs(list) do
      if r.position == position then
        if
          ksi.hw.pci_read(s.device, 0x10 + r.bar * 4, 4) ~= r.barRegister
          or ksi.hw.pci_read(s.device, r.capability + 0xc, 4) ~= r.tableRegister
        then
          ksi.trace('region 0x%x moved', position)
          return nil
        end
        break
      end
    end
    return list
  end
  local fresh, failed, readFailed = walk(s, op)
  if fresh and k then
    k.walks[index] = fresh
  end
  return fresh, failed, readFailed
end

local function permitted(ranges, offset, width)
  for _, r in ipairs(ranges) do
    if r[3] == width and offset >= r[1] and offset - r[1] < r[2] then
      return true
    end
  end
  return false
end

local function all_ones(width)
  if width == 8 then
    return -1
  end
  return 0xffffffff
end

-- Reads registers of a region of an operation of the function at a locator.
function read(input)
  if #input < 24 or (#input - 16) % 8 ~= 0 then
    ksi.trace('input of %d bytes', #input)
    return nt.STATUS_INVALID_PARAMETER
  end
  local locator, index, position, id = string.unpack('<I4I4I4I4', input)
  local s, refused = session(locator)
  if not s then
    return refused
  end
  local op = s.operations[index + 1]
  if not op then
    ksi.trace('no operation %d', index)
    if not s.kept then
      s.device:release()
    end
    return nt.STATUS_INVALID_PARAMETER
  end
  local list, failed, readFailed = regions_of(s, index + 1, op, position)
  if not list and not failed then
    drop(s, locator)
    s, refused = session(locator)
    if not s then
      return refused
    end
    list, failed, readFailed = regions_of(s, index + 1, op, position)
  end
  if not list then
    if not s.kept then
      s.device:release()
    elseif readFailed then
      drop(s, locator)
    end
    return failed
  end
  local region
  for _, r in ipairs(list) do
    if r.position == position then
      region = r
      break
    end
  end
  if not region or region.id ~= id then
    ksi.trace('no region 0x%x of ID 0x%x', position, id)
    if not s.kept then
      s.device:release()
    end
    return nt.STATUS_CONTEXT_MISMATCH
  end

  local entries = {}
  local spanBegin, spanEnd
  for at = 17, #input, 8 do
    local offset, width = string.unpack('<I4I4', input, at)
    local e = { offset = offset, width = width }
    if (width ~= 4 and width ~= 8) or offset % width ~= 0 then
      e.status = nt.STATUS_INVALID_PARAMETER
    elseif not permitted(op.ranges, offset, width) or offset + width > region.size then
      e.status = nt.STATUS_ACCESS_DENIED
    elseif (region.base + offset) % width ~= 0 then
      e.status = nt.STATUS_DATATYPE_MISALIGNMENT
    else
      spanBegin = math.min(spanBegin or offset, offset)
      spanEnd = math.max(spanEnd or 0, offset + width)
    end
    entries[#entries + 1] = e
  end

  if spanBegin then
    local maps = s.kept and s.kept.maps
    local key = (index << 32) | position
    local w = maps and maps[key]
    if w and (spanBegin < w.first or spanEnd > w.last) then
      spanBegin = math.min(spanBegin, w.first)
      spanEnd = math.max(spanEnd, w.last)
      maps[key] = nil
      w.map:release()
      w = nil
    end
    if not w then
      local first = region.base & -PAGE
      local last = (region.base + region.size + PAGE - 1) & -PAGE
      if math.ult(last, first) or math.ult(HUGE_PAGE_END, last) then
        ksi.trace('region 0x%x ends beyond the address space', region.base)
        if not s.kept then
          s.device:release()
        end
        return nt.STATUS_DEVICE_CONFIGURATION_ERROR
      end
      local bits, bitsFailed = address_bits()
      local st = bits and check_range(first, last, bits) or bitsFailed
      if st ~= nt.STATUS_SUCCESS then
        if not s.kept then
          s.device:release()
        end
        return st
      end
      local start = (region.base + spanBegin) & -8
      local m, mapFailed = ksi.hw.map(start, region.base + spanEnd - start)
      if not m then
        ksi.trace('region 0x%x not mapped: 0x%x', region.base, mapFailed)
        if not s.kept then
          s.device:release()
        end
        return mapFailed
      end
      w = { map = m, start = start, first = spanBegin, last = spanEnd }
      if maps then
        maps[key] = w
      end
    end
    local mapping <close> = (not maps) and w.map or nil
    local stopped = false
    for _, e in ipairs(entries) do
      if e.status == nil then
        if stopped then
          e.status = nt.STATUS_CANCELLED
        else
          e.value = ksi.hw.map_read(w.map, region.base + e.offset - w.start, e.width)
          e.status = nt.STATUS_SUCCESS
          if e.value == all_ones(e.width) then
            local st = check_d0(s)
            local base = st == nt.STATUS_SUCCESS and bar_base(s, region.bar)
            if base ~= region.barBase then
              ksi.trace('offset 0x%x read all-ones; the region is gone', e.offset)
              e.status, e.value = nt.STATUS_DEVICE_NOT_READY, 0
              stopped = true
            end
          end
        end
      end
    end
    if stopped and maps and maps[key] then
      maps[key] = nil
      w.map:release()
    end
  end
  if not s.kept then
    s.device:release()
  end

  local out = {}
  for _, e in ipairs(entries) do
    out[#out + 1] = string.pack('<i4I4I8', e.status, 0, e.value or 0)
  end
  return nt.STATUS_SUCCESS, table.concat(out)
end
