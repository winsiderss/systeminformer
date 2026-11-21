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
-- Reads PCI configuration space of the functions listed below: the device 18h
-- functions of AMD family 10h to 16h processors and two AMD bridges. Each row
-- lists the ranges a read must lie inside.
--

local ROWS = {
  {
    vendor = 0x1022,
    devices = { 0x1203 },
    reads = function()
      return {
        { 0x064, 0x4 },
        { 0x0a4, 0x4 },
        { 0x0d4, 0x4 },
        { 0x0e8, 0x4 },
      }
    end,
  },
  {
    vendor = 0x1022,
    devices = { 0x1303 },
    reads = function()
      return {
        { 0x064, 0x4 },
        { 0x0a4, 0x4 },
        { 0x0d4, 0x4 },
        { 0x0e8, 0x4 },
      }
    end,
  },
  {
    vendor = 0x1022,
    devices = { 0x1703 },
    reads = function()
      return {
        { 0x064, 0x4 },
        { 0x0a4, 0x4 },
        { 0x0d4, 0x4 },
        { 0x0e8, 0x4 },
      }
    end,
  },
  {
    vendor = 0x1022,
    devices = { 0x1603 },
    reads = function()
      return {
        { 0x064, 0x4 },
        { 0x0a4, 0x4 },
        { 0x0d4, 0x4 },
        { 0x0e8, 0x4 },
      }
    end,
  },
  {
    vendor = 0x1022,
    devices = { 0x1403 },
    reads = function()
      return {
        { 0x064, 0x4 },
        { 0x0a4, 0x4 },
        { 0x0d4, 0x4 },
        { 0x0e8, 0x4 },
      }
    end,
  },
  {
    vendor = 0x1022,
    devices = { 0x141d },
    reads = function()
      return {
        { 0x064, 0x4 },
        { 0x0a4, 0x4 },
        { 0x0d4, 0x4 },
        { 0x0e8, 0x4 },
      }
    end,
  },
  {
    vendor = 0x1022,
    devices = { 0x1533 },
    reads = function()
      return {
        { 0x064, 0x4 },
        { 0x0a4, 0x4 },
        { 0x0d4, 0x4 },
        { 0x0e8, 0x4 },
      }
    end,
  },
  {
    vendor = 0x1022,
    devices = { 0x1583 },
    reads = function()
      return {
        { 0x064, 0x4 },
        { 0x0a4, 0x4 },
        { 0x0d4, 0x4 },
        { 0x0e8, 0x4 },
      }
    end,
  },
  {
    vendor = 0x1022,
    devices = { 0x1204, 0x1704, 0x1404 },
    reads = function()
      return {
        { 0x15c, 0x4 },
      }
    end,
  },
  {
    vendor = 0x1022,
    devices = { 0x1604, 0x141e, 0x1574, 0x15b4, 0x1534, 0x1584 },
    reads = function()
      return {
        { 0x15c, 0x4 },
        { 0x1b8, 0x4 },
      }
    end,
  },
  {
    vendor = 0x1022,
    devices = { 0x1605, 0x141f, 0x1575, 0x15b5, 0x1535, 0x1585 },
    reads = function()
      return {
        { 0x0e0, 0x4 },
        { 0x0e8, 0x4 },
      }
    end,
  },
  {
    vendor = 0x1022,
    devices = { 0x14db, 0x14dd },
    reads = function()
      return {
        { 0x000, 0xc4 },
        { 0x0cc, 0x34 },
        { 0x100, 0xf00 },
      }
    end,
  },
}

local kept = {}

-- The header bytes a function is identified by.
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

-- Identifies the function at a locator and finds its row. Returns the
-- session, or nil and the status of the call.
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
        row = k.row,
        ranges = k.ranges,
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
  local s = { device = device, vendor = vendor, id = id, row = row }
  if not ksi.volatile then
    s.ranges = row.reads()
    s.kept = {
      device = device,
      key = header_key(header),
      vendor = vendor,
      id = id,
      row = row,
      ranges = s.ranges,
    }
    kept[locator] = s.kept
  end
  return s
end

-- Returns the IDs of the function the input names and the number of ranges
-- its row lists.
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
  local ranges = s.ranges or s.row.reads()
  return ksi.STATUS_SUCCESS, string.pack('<I2I2I4', s.vendor, s.id, #ranges)
end

local function permitted(ranges, offset, width)
  for _, range in ipairs(ranges) do
    local delta = offset - range[1]
    if offset >= range[1] and delta < range[2] and width <= range[2] - delta then
      return true
    end
  end
  return false
end

-- Reads a list of registers of the function the input names.
function read(input)
  if #input < 12 or (#input - 4) % 8 ~= 0 then
    ksi.trace('input of %d bytes', #input)
    return ksi.STATUS_INVALID_PARAMETER
  end
  local s, refused = session(string.unpack('<I4', input))
  if not s then
    return refused
  end
  local closer <close> = (not s.kept) and s.device or nil
  local ranges = s.ranges or s.row.reads()
  local output = {}

  for i = 1, (#input - 4) // 8 do
    local offset, width = string.unpack('<I4I4', input, i * 8 - 3)
    local status, value = ksi.STATUS_SUCCESS, 0
    if width ~= 1 and width ~= 2 and width ~= 4 then
      ksi.trace('width %d at 0x%x', width, offset)
      status = ksi.STATUS_INVALID_PARAMETER
    elseif not permitted(ranges, offset, width) then
      ksi.trace('0x%x of %d bytes is not permitted', offset, width)
      status = ksi.STATUS_ACCESS_DENIED
    else
      local got, failed = ksi.hw.pci_read(s.device, offset, width)
      if got then
        value = got
      else
        ksi.trace('0x%x of %d bytes not read: 0x%x', offset, width, failed)
        status = ksi.STATUS_DEVICE_NOT_READY
      end
    end
    output[i] = string.pack('<I4I4', status, value)
  end

  return ksi.STATUS_SUCCESS, table.concat(output)
end
