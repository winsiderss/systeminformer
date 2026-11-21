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
    return nil, ksi.STATUS_NOT_SUPPORTED
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
      }
    end
    ksi.trace('the function kept for 0x%x changed', locator)
    kept[locator] = nil
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
    return nil, ksi.STATUS_NOT_FOUND
  end
  local row = find_row(vendor, id)
  if not row then
    ksi.trace('no row for PCI %04x:%04x', vendor, id)
    device:release()
    return nil, ksi.STATUS_NOT_SUPPORTED
  end
  if not ksi.hw.pci_fixed(device) then
    ksi.trace('%04x:%04x may be removed', vendor, id)
    device:release()
    return nil, ksi.STATUS_NOT_SUPPORTED
  end
  local s = { device = device, vendor = vendor, id = id, operations = row.operations() }
  if not ksi.volatile then
    s.kept = {
      device = device,
      key = header_key(header),
      vendor = vendor,
      id = id,
      operations = s.operations,
    }
    kept[locator] = s.kept
  end
  return s
end

-- Returns the PCI IDs at a locator and the number of operations of its row.
function identify(input)
  if #input ~= 4 then
    ksi.trace('input of %d bytes', #input)
    return ksi.STATUS_INVALID_PARAMETER
  end
  local s, refused = session(string.unpack('<I4', input))
  if not s then
    return refused
  end
  if not s.kept then
    s.device:release()
  end
  return ksi.STATUS_SUCCESS, string.pack('<I2I2I4', s.vendor, s.id, #s.operations)
end
