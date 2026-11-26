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
-- Reads registers of the AMD system management network through the index and
-- data pair of the root complex. The root complexes and the address ranges
-- each processor family may be asked for are listed below.
--

local nt = require('nt')

local ROWS = {
  {
    vendor = 'AuthenticAMD',
    family = 0x17,
    models = { { 0x00, 0x0f } },
    pci = { 0x1022, 0x1450 },
    pin = { 0, 0 },
    index = 0x60,
    data = 0x64,
    targets = function()
      return {
        { 0x00059800, 0x00059c00, 0x0, 0 },
        { 0x0005a000, 0x0005a100, 0x0, 0 },
        { 0x00050000, 0x00051000, 0x100000, 8 },
      }, {}
    end,
  },
  {
    vendor = 'AuthenticAMD',
    family = 0x17,
    models = { { 0x10, 0x1f }, { 0x20, 0x2f } },
    pci = { 0x1022, 0x15d0 },
    pin = { 0, 0 },
    index = 0x60,
    data = 0x64,
    targets = function()
      return {
        { 0x00059800, 0x00059c00, 0x0, 0 },
        { 0x0005a000, 0x0005a100, 0x0, 0 },
        { 0x00050000, 0x00051000, 0x100000, 8 },
      }, {}
    end,
  },
  {
    vendor = 'AuthenticAMD',
    family = 0x17,
    models = { { 0x50, 0x5f } },
    class = { 0x1022, 0x060000, 0x00 },
    pin = { 0, 0 },
    index = 0x60,
    data = 0x64,
    targets = function()
      return {
        { 0x00059800, 0x00059c00, 0x0, 0 },
        { 0x0005a000, 0x0005a100, 0x0, 0 },
        { 0x00050000, 0x00051000, 0x100000, 8 },
      }, {}
    end,
  },
  {
    vendor = 'AuthenticAMD',
    family = 0x17,
    models = { { 0x30, 0x3f } },
    pci = { 0x1022, 0x1480 },
    pin = { 0, 0 },
    index = 0x60,
    data = 0x64,
    targets = function()
      return {
        { 0x00059800, 0x00059c00, 0x0, 0 },
        { 0x0005a000, 0x0005a100, 0x0, 0 },
        { 0x00050000, 0x00051000, 0x100000, 8 },
      }, {}
    end,
  },
  {
    vendor = 'AuthenticAMD',
    family = 0x17,
    models = { { 0x40, 0x4f } },
    class = { 0x1022, 0x060000, 0x00 },
    pin = { 0, 0 },
    index = 0x60,
    data = 0x64,
    targets = function()
      return {
        { 0x00059800, 0x00059c00, 0x0, 0 },
        { 0x0005a000, 0x0005a100, 0x0, 0 },
        { 0x00050000, 0x00051000, 0x100000, 8 },
      }, {}
    end,
  },
  {
    vendor = 'AuthenticAMD',
    family = 0x17,
    models = { { 0x60, 0x6f } },
    pci = { 0x1022, 0x1630 },
    pin = { 0, 0 },
    index = 0x60,
    data = 0x64,
    targets = function()
      return {
        { 0x00059800, 0x00059c00, 0x0, 0 },
        { 0x0005a000, 0x0005a100, 0x0, 0 },
        { 0x00050000, 0x00051000, 0x100000, 8 },
      }, {}
    end,
  },
  {
    vendor = 'AuthenticAMD',
    family = 0x17,
    models = { { 0x70, 0x7f } },
    pci = { 0x1022, 0x1480 },
    pin = { 0, 0 },
    index = 0x60,
    data = 0x64,
    targets = function()
      return {
        { 0x00059800, 0x00059c00, 0x0, 0 },
        { 0x0005a000, 0x0005a100, 0x0, 0 },
        { 0x00050000, 0x00051000, 0x100000, 8 },
      }, {}
    end,
  },
  {
    vendor = 'AuthenticAMD',
    family = 0x17,
    models = { { 0x90, 0x9f } },
    pci = { 0x1022, 0x1645 },
    pin = { 0, 0 },
    index = 0x60,
    data = 0x64,
    targets = function()
      return {
        { 0x00059800, 0x00059c00, 0x0, 0 },
        { 0x0005a000, 0x0005a100, 0x0, 0 },
        { 0x00050000, 0x00051000, 0x100000, 8 },
      }, {}
    end,
  },
  {
    vendor = 'AuthenticAMD',
    family = 0x17,
    models = { { 0x90, 0x9f } },
    class = { 0x1022, 0x060000, 0x00 },
    pin = { 0, 0 },
    index = 0x60,
    data = 0x64,
    targets = function()
      return {
        { 0x00059800, 0x00059c00, 0x0, 0 },
        { 0x0005a000, 0x0005a100, 0x0, 0 },
        { 0x00050000, 0x00051000, 0x100000, 8 },
      }, {}
    end,
  },
  {
    vendor = 'AuthenticAMD',
    family = 0x17,
    models = { { 0xa0, 0xaf } },
    pci = { 0x1022, 0x14b5 },
    pin = { 0, 0 },
    index = 0x60,
    data = 0x64,
    targets = function()
      return {
        { 0x00059800, 0x00059c00, 0x0, 0 },
        { 0x0005a000, 0x0005a100, 0x0, 0 },
        { 0x00050000, 0x00051000, 0x100000, 8 },
      }, {}
    end,
  },
  {
    vendor = 'HygonGenuine',
    family = 0x18,
    models = { { 0x00, 0x0f } },
    class = { 0x1d94, 0x060000, 0x00 },
    pin = { 0, 0 },
    index = 0x60,
    data = 0x64,
    targets = function()
      return {
        { 0x00059800, 0x00059c00, 0x0, 0 },
        { 0x0005a000, 0x0005a100, 0x0, 0 },
        { 0x00050000, 0x00051000, 0x100000, 8 },
      }, {}
    end,
  },
  {
    vendor = 'AuthenticAMD',
    family = 0x19,
    models = { { 0x00, 0x0f } },
    pci = { 0x1022, 0x1480 },
    pin = { 0, 0 },
    index = 0x60,
    data = 0x64,
    targets = function()
      return {
        { 0x00059800, 0x00059c00, 0x0, 0 },
        { 0x0005a000, 0x0005a100, 0x0, 0 },
        { 0x00050000, 0x00051000, 0x100000, 8 },
      }, {}
    end,
  },
  {
    vendor = 'AuthenticAMD',
    family = 0x19,
    models = { { 0x30, 0x3f } },
    pci = { 0x1022, 0x1480 },
    pin = { 0, 0 },
    index = 0x60,
    data = 0x64,
    targets = function()
      return {
        { 0x00059800, 0x00059c00, 0x0, 0 },
        { 0x00050000, 0x00051000, 0x100000, 8 },
      }, {}
    end,
  },
  {
    vendor = 'AuthenticAMD',
    family = 0x19,
    models = { { 0x10, 0x17 }, { 0x19, 0x1f } },
    pci = { 0x1022, 0x14a4 },
    pin = { 0, 0 },
    index = 0x60,
    data = 0x64,
    targets = function()
      return {
        { 0x00059800, 0x00059c00, 0x0, 0 },
        { 0x00050000, 0x00051000, 0x100000, 12 },
      }, {}
    end,
  },
  {
    vendor = 'AuthenticAMD',
    family = 0x19,
    models = { { 0x90, 0x9f } },
    pci = { 0x1022, 0x14f8 },
    pin = { 0, 0 },
    index = 0x60,
    data = 0x64,
    targets = function()
      return {
        { 0x00059800, 0x00059c00, 0x0, 0 },
      }, {}
    end,
  },
  {
    vendor = 'AuthenticAMD',
    family = 0x19,
    models = { { 0x18, 0x18 } },
    pci = { 0x1022, 0x14a4 },
    pin = { 0, 0 },
    index = 0x60,
    data = 0x64,
    targets = function()
      return {
        { 0x00059800, 0x00059c00, 0x0, 0 },
        { 0x00050000, 0x00051000, 0x100000, 12 },
      }, {}
    end,
  },
  {
    vendor = 'AuthenticAMD',
    family = 0x19,
    models = { { 0x20, 0x2f } },
    pci = { 0x1022, 0x1480 },
    pin = { 0, 0 },
    index = 0x60,
    data = 0x64,
    targets = function()
      return {
        { 0x00059800, 0x00059c00, 0x0, 0 },
        { 0x0005a000, 0x0005a100, 0x0, 0 },
        { 0x00050000, 0x00051000, 0x100000, 8 },
      }, {}
    end,
  },
  {
    vendor = 'AuthenticAMD',
    family = 0x19,
    models = { { 0x40, 0x4f } },
    pci = { 0x1022, 0x14b5 },
    pin = { 0, 0 },
    index = 0x60,
    data = 0x64,
    targets = function()
      return {
        { 0x00059800, 0x00059c00, 0x0, 0 },
        { 0x0005a000, 0x0005a100, 0x0, 0 },
        { 0x00050000, 0x00051000, 0x100000, 8 },
      }, {}
    end,
  },
  {
    vendor = 'AuthenticAMD',
    family = 0x19,
    models = { { 0x50, 0x5f } },
    pci = { 0x1022, 0x1630 },
    pin = { 0, 0 },
    index = 0x60,
    data = 0x64,
    targets = function()
      return {
        { 0x00059800, 0x00059c00, 0x0, 0 },
        { 0x0005a000, 0x0005a100, 0x0, 0 },
        { 0x00050000, 0x00051000, 0x100000, 8 },
      }, {}
    end,
  },
  {
    vendor = 'AuthenticAMD',
    family = 0x19,
    models = { { 0x60, 0x6f } },
    pci = { 0x1022, 0x14d8 },
    pin = { 0, 0 },
    index = 0x60,
    data = 0x64,
    targets = function()
      return {
        { 0x00059800, 0x00059c00, 0x0, 0 },
        { 0x00050000, 0x00051000, 0x100000, 8 },
      }, {}
    end,
  },
  {
    vendor = 'AuthenticAMD',
    family = 0x19,
    models = { { 0x70, 0x7f } },
    pci = { 0x1022, 0x14e8 },
    pin = { 0, 0 },
    index = 0x60,
    data = 0x64,
    targets = function()
      return {
        { 0x00059800, 0x00059c00, 0x0, 0 },
        { 0x00050000, 0x00051000, 0x100000, 8 },
      }, {}
    end,
  },
  {
    vendor = 'AuthenticAMD',
    family = 0x19,
    models = { { 0xa0, 0xaf } },
    class = { 0x1022, 0x060000, 0x00 },
    pin = { 0, 0 },
    index = 0x60,
    data = 0x64,
    targets = function()
      return {
        { 0x00059800, 0x00059c00, 0x0, 0 },
        { 0x00050000, 0x00051000, 0x100000, 12 },
      }, {}
    end,
  },
  {
    vendor = 'AuthenticAMD',
    family = 0x1a,
    models = { { 0x00, 0x07 }, { 0x09, 0x0f } },
    pci = { 0x1022, 0x153a },
    pin = { 0, 0 },
    index = 0x60,
    data = 0x64,
    targets = function()
      return {
        { 0x00059800, 0x00059c00, 0x0, 0 },
        { 0x00050000, 0x00051000, 0x100000, 12 },
      }, {}
    end,
  },
  {
    vendor = 'AuthenticAMD',
    family = 0x1a,
    models = { { 0x08, 0x08 } },
    pci = { 0x1022, 0x153a },
    pin = { 0, 0 },
    index = 0x60,
    data = 0x64,
    targets = function()
      return {
        { 0x00059800, 0x00059c00, 0x0, 0 },
        { 0x00050000, 0x00051000, 0x100000, 12 },
      }, {}
    end,
  },
  {
    vendor = 'AuthenticAMD',
    family = 0x1a,
    models = { { 0x10, 0x1f } },
    pci = { 0x1022, 0x153a },
    pin = { 0, 0 },
    index = 0x60,
    data = 0x64,
    targets = function()
      return {
        { 0x00059800, 0x00059c00, 0x0, 0 },
        { 0x00050000, 0x00051000, 0x100000, 12 },
      }, {}
    end,
  },
  {
    vendor = 'AuthenticAMD',
    family = 0x1a,
    models = { { 0x20, 0x2f } },
    pci = { 0x1022, 0x1507 },
    pin = { 0, 0 },
    index = 0x60,
    data = 0x64,
    targets = function()
      return {
        { 0x00059800, 0x00059c00, 0x0, 0 },
        { 0x00050000, 0x00051000, 0x100000, 8 },
      }, {}
    end,
  },
  {
    vendor = 'AuthenticAMD',
    family = 0x1a,
    models = { { 0x40, 0x4f } },
    pci = { 0x1022, 0x14d8 },
    pin = { 0, 0 },
    index = 0x60,
    data = 0x64,
    targets = function()
      return {
        { 0x00059800, 0x00059c00, 0x0, 0 },
        { 0x00050000, 0x00051000, 0x100000, 8 },
      }, {}
    end,
  },
  {
    vendor = 'AuthenticAMD',
    family = 0x1a,
    models = { { 0x50, 0x5f } },
    class = { 0x1022, 0x060000, 0x00 },
    pin = { 0, 0 },
    index = 0x60,
    data = 0x64,
    targets = function()
      return {
        { 0x00059800, 0x00059c00, 0x0, 0 },
      }, {}
    end,
  },
  {
    vendor = 'AuthenticAMD',
    family = 0x1a,
    models = { { 0x60, 0x6f } },
    pci = { 0x1022, 0x1122 },
    pin = { 0, 0 },
    index = 0x60,
    data = 0x64,
    targets = function()
      return {
        { 0x00059800, 0x00059c00, 0x0, 0 },
        { 0x00050000, 0x00051000, 0x100000, 8 },
      }, {}
    end,
  },
  {
    vendor = 'AuthenticAMD',
    family = 0x1a,
    models = { { 0x70, 0x7f } },
    class = { 0x1022, 0x060000, 0x00 },
    pin = { 0, 0 },
    index = 0x60,
    data = 0x64,
    targets = function()
      return {
        { 0x00059800, 0x00059c00, 0x0, 0 },
        { 0x00050000, 0x00051000, 0x100000, 8 },
      }, {}
    end,
  },
  {
    vendor = 'AuthenticAMD',
    family = 0x1a,
    models = { { 0x80, 0x8f } },
    pci = { 0x1022, 0x115b },
    pin = { 0, 0 },
    index = 0x60,
    data = 0x64,
    targets = function()
      return {
        { 0x00059800, 0x00059c00, 0x0, 0 },
      }, {}
    end,
  },
  {
    vendor = 'AuthenticAMD',
    family = 0x1a,
    models = { { 0x90, 0x9f } },
    class = { 0x1022, 0x060000, 0x00 },
    pin = { 0, 0 },
    index = 0x60,
    data = 0x64,
    targets = function()
      return {
        { 0x00059800, 0x00059c00, 0x0, 0 },
      }, {}
    end,
  },
  {
    vendor = 'AuthenticAMD',
    family = 0x15,
    models = { { 0x60, 0x6f }, { 0x70, 0x7f } },
    pci = { 0x1022, 0x1576 },
    pin = { 0, 0 },
    index = 0xb8,
    data = 0xbc,
    targets = function()
      return {}, {
        [0xd8200ca4] = true,
        [0xd8200c64] = true,
      }
    end,
  },
}

local ALL_ONES = 0xffffffff

local kept = {}

-- The header bytes a function is identified by.
local function header_key(header)
  return string.sub(header, 1, 4) .. string.sub(header, 9, 12) .. string.sub(header, 15, 15)
end

local function cpu_identity()
  if kept.cpu then
    return kept.cpu
  end
  local items = string.pack('<I2I2I4I4', 0, 0, 0, 0) .. string.pack('<I2I2I4I4', 0, 0, 1, 0)
  local results, status = ksi.hw.cpuid(items)
  if not results then
    ksi.trace('cpuid failed: 0x%x', status)
    return nil, status
  end
  local s0, _, b, c, d = string.unpack('<i4I4I4I4I4', results, 1)
  local s1, signature = string.unpack('<i4I4', results, 21)
  if not nt.NT_SUCCESS(s0) or not nt.NT_SUCCESS(s1) then
    ksi.trace('CPUID of processor 0:0 failed: 0x%x 0x%x', s0, s1)
    return nil, not nt.NT_SUCCESS(s0) and s0 or s1
  end
  local base = (signature >> 8) & 0xf
  local family = base
  if base == 0xf then
    family = family + ((signature >> 20) & 0xff)
  end
  local model = (signature >> 4) & 0xf
  if base >= 0x6 then
    model = model | (((signature >> 16) & 0xf) << 4)
  end
  kept.cpu = {
    vendor = string.pack('<I4I4I4', b, d, c),
    family = family,
    model = model,
    stepping = signature & 0xf,
  }
  return kept.cpu
end

local function cpu_matches(row, cpu)
  if row.vendor ~= cpu.vendor or row.family ~= cpu.family then
    return false
  end
  for _, range in ipairs(row.models) do
    if cpu.model >= range[1] and cpu.model <= range[2] then
      return true
    end
  end
  return false
end

local function find_row(cpu, root)
  for _, row in ipairs(ROWS) do
    if
      row.pci
      and cpu_matches(row, cpu)
      and row.pci[1] == root.vendor
      and row.pci[2] == root.device
    then
      return row
    end
  end
  for _, row in ipairs(ROWS) do
    if
      row.class
      and cpu_matches(row, cpu)
      and row.class[1] == root.vendor
      and row.class[2] == root.class
      and row.class[3] == root.header
      and row.pin[1] == root.slot
      and row.pin[2] == root.fn
    then
      return row
    end
  end
  return nil
end

-- Identifies the processor and the root complex at a locator and finds the
-- row for both. Returns the session, or nil and the status of the call.
local function session(input)
  if not ksi.hw.pci_open or not ksi.hw.cpuid then
    ksi.trace('no PCI or CPUID functions')
    return nil, nt.STATUS_NOT_SUPPORTED
  end
  if #input < 4 or #input % 4 ~= 0 then
    ksi.trace('input of %d bytes', #input)
    return nil, nt.STATUS_INVALID_PARAMETER
  end

  local locator = string.unpack('<I4', input)
  local cpu, failed = cpu_identity()
  if not cpu then
    return nil, failed
  end

  local root = {
    segment = locator >> 16,
    bus = (locator >> 8) & 0xff,
    slot = (locator >> 3) & 0x1f,
    fn = locator & 0x7,
  }

  local k = kept[locator]
  if k then
    local header = ksi.hw.pci_read_block(k.device, 0, 16)
    if header and header_key(header) == k.key then
      return {
        device = k.device,
        kept = k,
        cpu = cpu,
        root = k.root,
        row = k.row,
      }
    end
    ksi.trace('the function kept for 0x%x changed', locator)
    kept[locator] = nil
    k.device:release()
  end

  local device, status = nt.pci.open(root.segment, root.bus, root.slot, root.fn)
  if not device then
    ksi.trace('no PCI function at 0x%x: 0x%x', locator, status)
    return nil, status
  end

  local header, readFailed = ksi.hw.pci_read_block(device, 0, 16)
  if not header then
    ksi.trace('configuration header of 0x%x not read: 0x%x', locator, readFailed)
    local refused = nt.pci.refused(device, readFailed)
    device:release()
    return nil, refused
  end
  root.vendor, root.device = string.unpack('<I2I2', header, 1)
  if root.vendor == 0xffff then
    ksi.trace('no device answers at 0x%x', locator)
    device:release()
    return nil, nt.STATUS_NOT_FOUND
  end
  root.class = string.unpack('<I4', header, 9) >> 8
  root.header = string.byte(header, 15) & 0x7f

  local row = find_row(cpu, root)
  if not row then
    ksi.trace(
      'no row for %s family 0x%x model 0x%x and PCI %04x:%04x',
      cpu.vendor,
      cpu.family,
      cpu.model,
      root.vendor,
      root.device
    )
    device:release()
    return nil, nt.STATUS_NOT_SUPPORTED
  end
  if row.pin[1] ~= root.slot or row.pin[2] ~= root.fn then
    ksi.trace('the index and data pair is pinned to %d.%d', row.pin[1], row.pin[2])
    device:release()
    return nil, nt.STATUS_ACCESS_DENIED
  end

  local s = { device = device, cpu = cpu, root = root, row = row }
  if not ksi.volatile and not nt.is_device_removable(device) then
    local ranges, values = row.targets()
    s.kept = {
      device = device,
      key = header_key(header),
      root = root,
      row = row,
      ranges = ranges,
      values = values,
    }
    kept[locator] = s.kept
  end
  return s
end

local function allowed(ranges, values, address)
  for _, range in ipairs(ranges) do
    if address >= range[1] then
      local step = 0
      if range[4] > 1 and range[3] ~= 0 then
        step = math.min((address - range[1]) // range[3], range[4] - 1)
      end
      if address - step * range[3] < range[2] then
        return true
      end
    end
  end
  return values[address] == true
end

-- One index write and data read, checked by reading the index back; a moved
-- index is tried once more.
local function cycle(s, address)
  local device, row = s.device, s.row
  local value, check
  for _ = 1, 2 do
    local written, failed = ksi.hw.pci_write(device, row.index, 4, address)
    if not written then
      ksi.trace('index write at 0x%x failed: 0x%x', row.index, failed)
      return failed, 0
    end
    value, failed = ksi.hw.pci_read(device, row.data, 4)
    if not value then
      ksi.trace('data read at 0x%x failed: 0x%x', row.data, failed)
      return failed, 0
    end
    check, failed = ksi.hw.pci_read(device, row.index, 4)
    if not check then
      ksi.trace('index read at 0x%x failed: 0x%x', row.index, failed)
      return failed, 0
    end
    if check == address then
      if value == ALL_ONES then
        ksi.trace('SMN 0x%x read all-ones', address)
        return nt.STATUS_DEVICE_NOT_READY, 0
      end
      return nt.STATUS_SUCCESS, value
    end
  end
  ksi.trace('index 0x%x moved to 0x%x', address, check)
  return nt.STATUS_RETRY, 0
end

-- Returns the identity of the root complex the input names and of the
-- processor.
function identify(input)
  if not ksi.hw.pci_open or not ksi.hw.cpuid then
    ksi.trace('no PCI or CPUID functions')
    return nt.STATUS_NOT_SUPPORTED
  end
  if #input ~= 4 then
    ksi.trace('input of %d bytes', #input)
    return nt.STATUS_INVALID_PARAMETER
  end
  local lock <close>, failed = ksi.lock('pci')
  if not lock then
    ksi.trace('the pci lock was not taken: 0x%x', failed)
    return failed
  end
  local s, refused = session(input)
  if not s then
    return refused
  end
  if not s.kept then
    s.device:release()
  end
  return nt.STATUS_SUCCESS,
    string.pack(
      '<I2I2I4I4I4',
      s.root.vendor,
      s.root.device,
      s.cpu.family,
      s.cpu.model,
      s.cpu.stepping
    )
end

-- Reads a list of SMN addresses through the root complex the input names.
function read(input)
  local lock <close>, failed = ksi.lock('pci')
  if not lock then
    ksi.trace('the pci lock was not taken: 0x%x', failed)
    return failed
  end

  local s, refused = session(input)
  if not s then
    return refused
  end
  local closer <close> = (not s.kept) and s.device or nil

  local ranges, values
  if s.kept then
    ranges, values = s.kept.ranges, s.kept.values
  else
    ranges, values = s.row.targets()
  end
  local count = #input // 4 - 1
  local output = {}

  for i = 1, count do
    local address = string.unpack('<I4', input, i * 4 + 1)
    local status, value = nt.STATUS_SUCCESS, 0
    if address % 4 ~= 0 then
      ksi.trace('SMN 0x%x is not aligned', address)
      status = nt.STATUS_INVALID_PARAMETER
    elseif not allowed(ranges, values, address) then
      ksi.trace('SMN 0x%x is not permitted', address)
      status = nt.STATUS_ACCESS_DENIED
    else
      status, value = cycle(s, address)
    end
    output[i] = string.pack('<i4I4', status, value)
  end

  return nt.STATUS_SUCCESS, table.concat(output)
end
