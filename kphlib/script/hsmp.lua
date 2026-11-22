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
-- Talks to the AMD HSMP mailbox of a socket: in the memory window of the
-- socket's AMDI0097 device when the firmware describes one, otherwise behind
-- the root complex's system management network pair. The root complexes,
-- their mailbox registers and the commands each processor family may be sent
-- are listed below.
--

local nt = require('nt')

local concat, pack, unpack = table.concat, table.pack, table.unpack

local ROWS = {
  {
    vendor = 'AuthenticAMD',
    family = 0x19,
    models = { { 0x00, 0x0f } },
    pci = { 0x1022, 0x1480 },
    pin = { 0, 0 },
    mailbox = function()
      return {
        index = 0xc4,
        data = 0xc8,
        msg = 0x03b10534,
        rsp = 0x03b10980,
        arg = 0x03b109e0,
        args = 8,
        version = 0x02,
        version_arg = 0x0,
        commands = {
          [0x01] = { words = 1, mask = { 0xffffffff } },
          [0x02] = { words = 1 },
          [0x03] = { words = 1 },
          [0x04] = { words = 1 },
          [0x06] = { words = 1 },
          [0x07] = { words = 1 },
          [0x0a] = { words = 1, mask = { 0xffff } },
          [0x0b] = { words = 1 },
          [0x0f] = { words = 2 },
          [0x10] = { words = 1 },
          [0x11] = { words = 1 },
          [0x13] = { words = 1, mask = { 0xff0000 } },
          [0x14] = { words = 1 },
          [0x15] = { words = 1 },
          [0x16] = { words = 1, mask = { 0xff } },
          [0x17] = { words = 1, mask = { 0xff } },
          [0x18] = { words = 1, mask = { 0xff } },
          [0x19] = { words = 1 },
          [0x1a] = { words = 1, mask = { 0xffffffff } },
          [0x1b] = { words = 1 },
          [0x1c] = { words = 1 },
          [0x1d] = { words = 1, mask = { 0xff07 } },
          [0x1e] = { words = 1, mask = { 0xff07 } },
          [0x23] = { words = 1 },
          [0x30] = { words = 1 },
          [0x31] = { words = 2, mask = { 0xffff } },
          [0x32] = { words = 2 },
          [0x34] = { words = 1, mask = { 0xffff } },
          [0x35] = { words = 1 },
          [0x36] = { words = 1, mask = { 0xf } },
          [0x37] = { words = 3, mask = { 0x1 } },
        },
      }
    end,
  },
  {
    vendor = 'AuthenticAMD',
    family = 0x19,
    models = { { 0x30, 0x3f } },
    pci = { 0x1022, 0x1480 },
    pin = { 0, 0 },
    mailbox = function()
      return {
        index = 0xc4,
        data = 0xc8,
        msg = 0x03b10534,
        rsp = 0x03b10980,
        arg = 0x03b109e0,
        args = 8,
        version = 0x02,
        version_arg = 0x0,
        commands = {
          [0x01] = { words = 1, mask = { 0xffffffff } },
          [0x02] = { words = 1 },
          [0x03] = { words = 1 },
          [0x04] = { words = 1 },
          [0x06] = { words = 1 },
          [0x07] = { words = 1 },
          [0x0a] = { words = 1, mask = { 0xffff } },
          [0x0b] = { words = 1 },
          [0x0f] = { words = 2 },
          [0x10] = { words = 1 },
          [0x11] = { words = 1 },
          [0x13] = { words = 1, mask = { 0xff0000 } },
          [0x14] = { words = 1 },
          [0x15] = { words = 1 },
          [0x16] = { words = 1, mask = { 0xff } },
          [0x17] = { words = 1, mask = { 0xff } },
          [0x18] = { words = 1, mask = { 0xff } },
          [0x19] = { words = 1 },
          [0x1a] = { words = 1, mask = { 0xffffffff } },
          [0x1b] = { words = 1 },
          [0x1c] = { words = 1 },
          [0x1d] = { words = 1, mask = { 0xff07 } },
          [0x1e] = { words = 1, mask = { 0xff07 } },
          [0x23] = { words = 1 },
          [0x30] = { words = 1 },
          [0x31] = { words = 2, mask = { 0xffff } },
          [0x32] = { words = 2 },
          [0x34] = { words = 1, mask = { 0xffff } },
          [0x35] = { words = 1 },
          [0x36] = { words = 1, mask = { 0xf } },
          [0x37] = { words = 3, mask = { 0x1 } },
        },
      }
    end,
  },
  {
    vendor = 'AuthenticAMD',
    family = 0x19,
    models = { { 0x10, 0x17 }, { 0x19, 0x1f } },
    pci = { 0x1022, 0x14a4 },
    pin = { 0, 0 },
    mailbox = function()
      return {
        index = 0xc4,
        data = 0xc8,
        msg = 0x03b10534,
        rsp = 0x03b10980,
        arg = 0x03b109e0,
        args = 8,
        version = 0x02,
        version_arg = 0x0,
        commands = {
          [0x01] = { words = 1, mask = { 0xffffffff } },
          [0x02] = { words = 1 },
          [0x03] = { words = 1 },
          [0x04] = { words = 1 },
          [0x06] = { words = 1 },
          [0x07] = { words = 1 },
          [0x0a] = { words = 1, mask = { 0xffff } },
          [0x0b] = { words = 1 },
          [0x0f] = { words = 2 },
          [0x10] = { words = 1 },
          [0x11] = { words = 1 },
          [0x13] = { words = 1, mask = { 0xff0000 } },
          [0x14] = { words = 1 },
          [0x15] = { words = 1 },
          [0x16] = { words = 1, mask = { 0xff } },
          [0x17] = { words = 1, mask = { 0xff } },
          [0x18] = { words = 1, mask = { 0xff } },
          [0x19] = { words = 1 },
          [0x1a] = { words = 1, mask = { 0xffffffff } },
          [0x1b] = { words = 1 },
          [0x1c] = { words = 1 },
          [0x1d] = { words = 1, mask = { 0xff07 } },
          [0x1e] = { words = 1, mask = { 0xff07 } },
          [0x23] = { words = 1 },
          [0x30] = { words = 1 },
          [0x31] = { words = 2, mask = { 0xffff } },
          [0x32] = { words = 2 },
          [0x34] = { words = 1, mask = { 0xffff } },
          [0x35] = { words = 1 },
          [0x36] = { words = 1, mask = { 0xf } },
          [0x37] = { words = 3, mask = { 0x1 } },
        },
      }
    end,
  },
  {
    vendor = 'AuthenticAMD',
    family = 0x19,
    models = { { 0x90, 0x9f } },
    pci = { 0x1022, 0x14f8 },
    pin = { 0, 0 },
    mailbox = function()
      return {
        index = 0xc4,
        data = 0xc8,
        msg = 0x03b10534,
        rsp = 0x03b10980,
        arg = 0x03b109e0,
        args = 8,
        version = 0x02,
        version_arg = 0x0,
        commands = {
          [0x01] = { words = 1, mask = { 0xffffffff } },
          [0x02] = { words = 1 },
          [0x03] = { words = 1 },
          [0x04] = { words = 1 },
          [0x06] = { words = 1 },
          [0x07] = { words = 1 },
          [0x0a] = { words = 1, mask = { 0xffff } },
          [0x0b] = { words = 1 },
          [0x0f] = { words = 2 },
          [0x10] = { words = 1 },
          [0x11] = { words = 1 },
          [0x13] = { words = 1, mask = { 0xff0000 } },
          [0x14] = { words = 1 },
          [0x15] = { words = 1 },
          [0x16] = { words = 1, mask = { 0xff } },
          [0x17] = { words = 1, mask = { 0xff } },
          [0x18] = { words = 1, mask = { 0xff } },
          [0x19] = { words = 1 },
          [0x1a] = { words = 1, mask = { 0xffffffff } },
          [0x1b] = { words = 1 },
          [0x1c] = { words = 1 },
          [0x1d] = { words = 1, mask = { 0xff07 } },
          [0x1e] = { words = 1, mask = { 0xff07 } },
          [0x23] = { words = 1 },
          [0x30] = { words = 1 },
          [0x31] = { words = 2, mask = { 0xffff } },
          [0x32] = { words = 2 },
          [0x34] = { words = 1, mask = { 0xffff } },
          [0x35] = { words = 1 },
          [0x36] = { words = 1, mask = { 0xf } },
          [0x37] = { words = 3, mask = { 0x1 } },
        },
      }
    end,
  },
  {
    vendor = 'AuthenticAMD',
    family = 0x19,
    models = { { 0x18, 0x18 } },
    pci = { 0x1022, 0x14a4 },
    pin = { 0, 0 },
    mailbox = function()
      return {
        index = 0xc4,
        data = 0xc8,
        msg = 0x03b10534,
        rsp = 0x03b10980,
        arg = 0x03b109e0,
        args = 8,
        version = 0x02,
        version_arg = 0x0,
        commands = {
          [0x01] = { words = 1, mask = { 0xffffffff } },
          [0x02] = { words = 1 },
          [0x03] = { words = 1 },
          [0x04] = { words = 1 },
          [0x06] = { words = 1 },
          [0x07] = { words = 1 },
          [0x0a] = { words = 1, mask = { 0xffff } },
          [0x0b] = { words = 1 },
          [0x0f] = { words = 2 },
          [0x10] = { words = 1 },
          [0x11] = { words = 1 },
          [0x13] = { words = 1, mask = { 0xff0000 } },
          [0x14] = { words = 1 },
          [0x15] = { words = 1 },
          [0x16] = { words = 1, mask = { 0xff } },
          [0x17] = { words = 1, mask = { 0xff } },
          [0x18] = { words = 1, mask = { 0xff } },
          [0x19] = { words = 1 },
          [0x1a] = { words = 1, mask = { 0xffffffff } },
          [0x1b] = { words = 1 },
          [0x1c] = { words = 1 },
          [0x1d] = { words = 1, mask = { 0xff07 } },
          [0x1e] = { words = 1, mask = { 0xff07 } },
          [0x23] = { words = 1 },
          [0x30] = { words = 1 },
          [0x31] = { words = 2, mask = { 0xffff } },
          [0x32] = { words = 2 },
          [0x34] = { words = 1, mask = { 0xffff } },
          [0x35] = { words = 1 },
          [0x36] = { words = 1, mask = { 0xf } },
          [0x37] = { words = 3, mask = { 0x1 } },
        },
      }
    end,
  },
  {
    vendor = 'AuthenticAMD',
    family = 0x19,
    models = { { 0xa0, 0xaf } },
    class = { 0x1022, 0x060000, 0x00 },
    pin = { 0, 0 },
    mailbox = function()
      return {
        index = 0xc4,
        data = 0xc8,
        msg = 0x03b10534,
        rsp = 0x03b10980,
        arg = 0x03b109e0,
        args = 8,
        version = 0x02,
        version_arg = 0x0,
        commands = {
          [0x01] = { words = 1, mask = { 0xffffffff } },
          [0x02] = { words = 1 },
          [0x03] = { words = 1 },
          [0x04] = { words = 1 },
          [0x06] = { words = 1 },
          [0x07] = { words = 1 },
          [0x0a] = { words = 1, mask = { 0xffff } },
          [0x0b] = { words = 1 },
          [0x0f] = { words = 2 },
          [0x10] = { words = 1 },
          [0x11] = { words = 1 },
          [0x13] = { words = 1, mask = { 0xff0000 } },
          [0x14] = { words = 1 },
          [0x15] = { words = 1 },
          [0x16] = { words = 1, mask = { 0xff } },
          [0x17] = { words = 1, mask = { 0xff } },
          [0x18] = { words = 1, mask = { 0xff } },
          [0x19] = { words = 1 },
          [0x1a] = { words = 1, mask = { 0xffffffff } },
          [0x1b] = { words = 1 },
          [0x1c] = { words = 1 },
          [0x1d] = { words = 1, mask = { 0xff07 } },
          [0x1e] = { words = 1, mask = { 0xff07 } },
          [0x23] = { words = 1 },
          [0x30] = { words = 1 },
          [0x31] = { words = 2, mask = { 0xffff } },
          [0x32] = { words = 2 },
          [0x34] = { words = 1, mask = { 0xffff } },
          [0x35] = { words = 1 },
          [0x36] = { words = 1, mask = { 0xf } },
          [0x37] = { words = 3, mask = { 0x1 } },
        },
      }
    end,
  },
  {
    vendor = 'AuthenticAMD',
    family = 0x1a,
    models = { { 0x00, 0x07 }, { 0x09, 0x0f } },
    pci = { 0x1022, 0x153a },
    pin = { 0, 0 },
    mailbox = function()
      return {
        index = 0xc4,
        data = 0xc8,
        msg = 0x03b10934,
        rsp = 0x03b10980,
        arg = 0x03b109e0,
        args = 8,
        version = 0x02,
        version_arg = 0x0,
        commands = {
          [0x01] = { words = 1, mask = { 0xffffffff } },
          [0x02] = { words = 1 },
          [0x03] = { words = 1 },
          [0x04] = { words = 1 },
          [0x06] = { words = 1 },
          [0x07] = { words = 1 },
          [0x0a] = { words = 1, mask = { 0xffff } },
          [0x0b] = { words = 1 },
          [0x0f] = { words = 2 },
          [0x10] = { words = 1 },
          [0x11] = { words = 1 },
          [0x13] = { words = 1, mask = { 0xff0000 } },
          [0x14] = { words = 1 },
          [0x15] = { words = 1 },
          [0x16] = { words = 1, mask = { 0xff } },
          [0x17] = { words = 1, mask = { 0xff } },
          [0x18] = { words = 1, mask = { 0xff } },
          [0x19] = { words = 1 },
          [0x1a] = { words = 1, mask = { 0xffffffff } },
          [0x1b] = { words = 1 },
          [0x1c] = { words = 1 },
          [0x1d] = { words = 1, mask = { 0xff07 } },
          [0x1e] = { words = 1, mask = { 0xff07 } },
          [0x23] = { words = 1 },
          [0x30] = { words = 1 },
          [0x31] = { words = 2, mask = { 0xffff } },
          [0x32] = { words = 2 },
          [0x34] = { words = 1, mask = { 0xffff } },
          [0x35] = { words = 1 },
          [0x36] = { words = 1, mask = { 0xf } },
          [0x37] = { words = 3, mask = { 0x1 } },
        },
      }
    end,
  },
  {
    vendor = 'AuthenticAMD',
    family = 0x1a,
    models = { { 0x08, 0x08 } },
    pci = { 0x1022, 0x153a },
    pin = { 0, 0 },
    mailbox = function()
      return {
        index = 0xc4,
        data = 0xc8,
        msg = 0x03b10934,
        rsp = 0x03b10980,
        arg = 0x03b109e0,
        args = 8,
        version = 0x02,
        version_arg = 0x0,
        commands = {
          [0x01] = { words = 1, mask = { 0xffffffff } },
          [0x02] = { words = 1 },
          [0x03] = { words = 1 },
          [0x04] = { words = 1 },
          [0x06] = { words = 1 },
          [0x07] = { words = 1 },
          [0x0a] = { words = 1, mask = { 0xffff } },
          [0x0b] = { words = 1 },
          [0x0f] = { words = 2 },
          [0x10] = { words = 1 },
          [0x11] = { words = 1 },
          [0x13] = { words = 1, mask = { 0xff0000 } },
          [0x14] = { words = 1 },
          [0x15] = { words = 1 },
          [0x16] = { words = 1, mask = { 0xff } },
          [0x17] = { words = 1, mask = { 0xff } },
          [0x18] = { words = 1, mask = { 0xff } },
          [0x19] = { words = 1 },
          [0x1a] = { words = 1, mask = { 0xffffffff } },
          [0x1b] = { words = 1 },
          [0x1c] = { words = 1 },
          [0x1d] = { words = 1, mask = { 0xff07 } },
          [0x1e] = { words = 1, mask = { 0xff07 } },
          [0x23] = { words = 1 },
          [0x30] = { words = 1 },
          [0x31] = { words = 2, mask = { 0xffff } },
          [0x32] = { words = 2 },
          [0x34] = { words = 1, mask = { 0xffff } },
          [0x35] = { words = 1 },
          [0x36] = { words = 1, mask = { 0xf } },
          [0x37] = { words = 3, mask = { 0x1 } },
        },
      }
    end,
  },
  {
    vendor = 'AuthenticAMD',
    family = 0x1a,
    models = { { 0x10, 0x1f } },
    pci = { 0x1022, 0x153a },
    pin = { 0, 0 },
    mailbox = function()
      return {
        index = 0xc4,
        data = 0xc8,
        msg = 0x03b10934,
        rsp = 0x03b10980,
        arg = 0x03b109e0,
        args = 8,
        version = 0x02,
        version_arg = 0x0,
        commands = {
          [0x01] = { words = 1, mask = { 0xffffffff } },
          [0x02] = { words = 1 },
          [0x03] = { words = 1 },
          [0x04] = { words = 1 },
          [0x06] = { words = 1 },
          [0x07] = { words = 1 },
          [0x0a] = { words = 1, mask = { 0xffff } },
          [0x0b] = { words = 1 },
          [0x0f] = { words = 2 },
          [0x10] = { words = 1 },
          [0x11] = { words = 1 },
          [0x13] = { words = 1, mask = { 0xff0000 } },
          [0x14] = { words = 1 },
          [0x15] = { words = 1 },
          [0x16] = { words = 1, mask = { 0xff } },
          [0x17] = { words = 1, mask = { 0xff } },
          [0x18] = { words = 1, mask = { 0xff } },
          [0x19] = { words = 1 },
          [0x1a] = { words = 1, mask = { 0xffffffff } },
          [0x1b] = { words = 1 },
          [0x1c] = { words = 1 },
          [0x1d] = { words = 1, mask = { 0xff07 } },
          [0x1e] = { words = 1, mask = { 0xff07 } },
          [0x23] = { words = 1 },
          [0x30] = { words = 1 },
          [0x31] = { words = 2, mask = { 0xffff } },
          [0x32] = { words = 2 },
          [0x34] = { words = 1, mask = { 0xffff } },
          [0x35] = { words = 1 },
          [0x36] = { words = 1, mask = { 0xf } },
          [0x37] = { words = 3, mask = { 0x1 } },
        },
      }
    end,
  },
}

local WAIT_MS = 1000
local STALE_MS = 100
local RESTORE_TRIES = 1000
local ALL_ONES = 0xffffffff
local ARGS = 8

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
  kept.cpu = { vendor = string.pack('<I4I4I4', b, d, c), family = family, model = model }
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

local function session(input)
  if not ksi.hw.pci_open or not ksi.hw.cpuid then
    ksi.trace('no PCI or CPUID functions')
    return nil, nt.STATUS_NOT_SUPPORTED
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
      return { device = k.device, kept = k, mailbox = k.mailbox, root = k.root }
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
    device:release()
    return nil, readFailed
  end
  root.vendor, root.device = string.unpack('<I2I2', header, 1)
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
    ksi.trace('the mailbox is pinned to %d.%d', row.pin[1], row.pin[2])
    device:release()
    return nil, nt.STATUS_ACCESS_DENIED
  end

  local s = { device = device, mailbox = row.mailbox(), root = root }
  if not ksi.volatile and not nt.is_device_removable(device) then
    s.kept = { device = device, key = header_key(header), root = root, mailbox = s.mailbox }
    kept[locator] = s.kept
  end
  return s
end

local function poll(every, spins, sleep, timeout, body)
  local limit = ksi.time() + timeout * 10000
  local round = 0
  while true do
    local late = ksi.time() >= limit
    if body(late) or late then
      return
    end
    if round < spins or sleep == 0 then
      ksi.stall(every)
    else
      ksi.sleep(sleep)
    end
    round = round + 1
  end
end

local function smn_read(s, address)
  local m = s.mailbox
  for _ = 1, 2 do
    if not ksi.hw.pci_write(s.device, m.index, 4, address) then
      ksi.trace('index write for SMN 0x%x failed', address)
      return nt.STATUS_DEVICE_NOT_READY, 0
    end
    local value = ksi.hw.pci_read(s.device, m.data, 4)
    if not value then
      ksi.trace('data read of SMN 0x%x failed', address)
      return nt.STATUS_DEVICE_NOT_READY, 0
    end
    local check = ksi.hw.pci_read(s.device, m.index, 4)
    if not check then
      ksi.trace('index read for SMN 0x%x failed', address)
      return nt.STATUS_DEVICE_NOT_READY, 0
    end
    if check == address then
      return nt.STATUS_SUCCESS, value
    end
  end
  ksi.trace('the index of SMN 0x%x moved', address)
  return nt.STATUS_RETRY, 0
end

local function smn_write(s, address, value, before, after)
  local m = s.mailbox
  if not ksi.hw.pci_write(s.device, m.index, 4, address) then
    ksi.trace('index write for SMN 0x%x failed', address)
    return nt.STATUS_RETRY
  end
  local check = ksi.hw.pci_read(s.device, m.index, 4)
  if not check then
    ksi.trace('index read for SMN 0x%x failed', address)
    return nt.STATUS_RETRY
  end
  if check ~= address then
    ksi.trace('the index of SMN 0x%x is not ours before the write', address)
    return nt.STATUS_RETRY
  end
  if before then
    before()
  end
  local written = ksi.hw.pci_write(s.device, m.data, 4, value)
  if after then
    after()
  end
  if not written then
    ksi.trace('data write of SMN 0x%x failed', address)
    return nt.STATUS_DEVICE_NOT_READY
  end
  check = ksi.hw.pci_read(s.device, m.index, 4)
  if not check then
    ksi.trace('index read for SMN 0x%x failed', address)
    return nt.STATUS_DEVICE_NOT_READY
  end
  if check ~= address then
    ksi.trace('the index of SMN 0x%x moved during the write', address)
    return nt.STATUS_DEVICE_DATA_ERROR
  end
  return nt.STATUS_SUCCESS
end

local function store(s, register, value, mark)
  local st = smn_write(s, register, value, mark)
  if st == nt.STATUS_RETRY then
    return st, false
  end
  if nt.NT_SUCCESS(st) or st ~= nt.STATUS_DEVICE_DATA_ERROR then
    return st, true
  end
  local rs, held = smn_read(s, register)
  if rs == nt.STATUS_RETRY then
    return nt.STATUS_DEVICE_DATA_ERROR, true
  end
  if not nt.NT_SUCCESS(rs) then
    return rs, true
  end
  if held ~= value then
    ksi.trace('SMN 0x%x holds 0x%x, not 0x%x', register, held, value)
    return nt.STATUS_RETRY, true
  end
  return nt.STATUS_SUCCESS, true
end

local function wait(s, timeout)
  local st, resp = nt.STATUS_IO_TIMEOUT, 0
  poll(50, 20, 1, timeout, function(late)
    local rs, value = smn_read(s, s.mailbox.rsp)
    if rs == nt.STATUS_RETRY and not late then
      value = 0
    elseif not nt.NT_SUCCESS(rs) then
      st = rs
      return true
    end
    if value == ALL_ONES then
      ksi.trace('RSP reads all-ones')
      st = nt.STATUS_DEVICE_NOT_READY
      return true
    end
    if value ~= 0 then
      st, resp = nt.STATUS_SUCCESS, value
      return true
    end
    return false
  end)
  return st, resp
end

local function post(s, op, mark)
  local m = s.mailbox
  local bs, before = smn_read(s, m.msg)
  local st = nt.STATUS_RETRY
  poll(0, 0, 1, WAIT_MS, function(late)
    local ws = smn_write(s, m.msg, op, nil, mark)
    if ws == nt.STATUS_RETRY then
      ws = smn_write(s, m.msg, op, nil, mark)
    end
    if ws == nt.STATUS_RETRY or nt.NT_SUCCESS(ws) then
      st = ws
      return true
    end
    if ws ~= nt.STATUS_DEVICE_DATA_ERROR then
      st = ws
      return true
    end
    if not nt.NT_SUCCESS(bs) then
      st = nt.STATUS_SUCCESS
      return true
    end
    local as, after = smn_read(s, m.msg)
    if not nt.NT_SUCCESS(as) then
      st = nt.STATUS_SUCCESS
      return true
    end
    if (after == op and before ~= op) or after ~= before or after == op then
      st = nt.STATUS_SUCCESS
      return true
    end
    if late then
      st = nt.STATUS_RETRY
      return true
    end
    return false
  end)
  if not nt.NT_SUCCESS(st) then
    ksi.trace('MSG 0x%x not posted: 0x%x', op, st)
  end
  return st
end

local function restore_idle(s, idle)
  for _ = 1, RESTORE_TRIES do
    local st = store(s, s.mailbox.rsp, idle)
    if nt.NT_SUCCESS(st) then
      return st
    end
    if st == nt.STATUS_DEVICE_NOT_READY then
      break
    end
    ksi.sleep(1)
  end
  ksi.trace('the idle RSP 0x%x was not put back', idle)
  return nt.STATUS_DEVICE_DATA_ERROR
end

local function read_result(s, register)
  local st, value = nt.STATUS_RETRY, 0
  poll(0, 0, 1, WAIT_MS, function()
    st, value = smn_read(s, register)
    return st ~= nt.STATUS_RETRY
  end)
  return st, value
end

local ANSWERS = {
  [0xfc] = nt.STATUS_DEVICE_BUSY,
  [0xfd] = nt.STATUS_INTERNAL_ERROR,
  [0xfe] = nt.STATUS_NOT_IMPLEMENTED,
}

local function zeros()
  return { 0, 0, 0, 0, 0, 0, 0, 0 }
end

local function command_body(s, op, args, words, state)
  local m = s.mailbox
  local results = zeros()
  local st, resp = wait(s, STALE_MS)
  if st == nt.STATUS_IO_TIMEOUT then
    ksi.trace('RSP stayed zero: stale, written over')
    st, resp = nt.STATUS_SUCCESS, 0
  end
  if not nt.NT_SUCCESS(st) then
    return st, false, results
  end
  state.idle = resp
  local issued
  st, issued = store(s, m.rsp, 0, function()
    state.clearing = true
  end)
  if not nt.NT_SUCCESS(st) then
    state.restore = issued and st ~= nt.STATUS_RETRY
    state.clearing = false
    return st, false, results
  end
  state.restore = true
  state.clearing = false
  for i = 1, math.min(m.args, ARGS) do
    st = store(s, m.arg + (i - 1) * 4, args[i])
    if not nt.NT_SUCCESS(st) then
      return st, false, results
    end
  end
  st = post(s, op, function()
    state.posting = true
  end)
  if st == nt.STATUS_RETRY then
    return st, false, results
  end
  state.restore = false
  if not nt.NT_SUCCESS(st) then
    return st, false, results
  end
  st, resp = wait(s, WAIT_MS)
  if not nt.NT_SUCCESS(st) then
    if st == nt.STATUS_IO_TIMEOUT then
      ksi.trace('RSP stayed zero')
    end
    return st, false, results
  end
  if resp ~= 0x01 then
    ksi.trace('MSG 0x%x answered 0x%x', op, resp)
    return ANSWERS[resp] or nt.STATUS_UNSUCCESSFUL, true, results, resp
  end
  for i = 1, words do
    st, results[i] = read_result(s, m.arg + (i - 1) * 4)
    if not nt.NT_SUCCESS(st) then
      return st, false, zeros(), resp
    end
  end
  return nt.STATUS_SUCCESS, true, results, resp
end

local function transact(s, op, args, words)
  local pci <close>, failed = ksi.lock('pci')
  if not pci then
    ksi.trace('the pci lock was not taken: 0x%x', failed)
    return failed, false, zeros(), 0
  end
  local state = { restore = false, idle = 0 }
  local putback <close> = setmetatable({}, {
    __close = function(_, err)
      if err and (state.restore or state.clearing) and not state.posting and state.idle ~= 0 then
        restore_idle(s, state.idle)
      end
    end,
  })
  local st, completed, results, answer = command_body(s, op, args, words, state)
  if state.restore and state.idle ~= 0 then
    local rs = restore_idle(s, state.idle)
    state.restore = false
    if not nt.NT_SUCCESS(rs) then
      st = rs
    end
  end
  return st, completed, results, answer or 0
end

local function protocol_version(s)
  local m = s.mailbox
  local args = { m.version_arg, 0, 0, 0, 0, 0, 0, 0 }
  local st, _, first = transact(s, m.version, args, 1)
  if not nt.NT_SUCCESS(st) then
    return st, 0
  end
  if first[1] == 0 or first[1] == ALL_ONES then
    ksi.trace('version 0x%x', first[1])
    return nt.STATUS_DEVICE_NOT_READY, 0
  end
  local again
  st, _, again = transact(s, m.version, args, 1)
  if not nt.NT_SUCCESS(st) then
    return st, 0
  end
  if again[1] == 0 or again[1] == ALL_ONES then
    ksi.trace('version 0x%x', again[1])
    return nt.STATUS_DEVICE_NOT_READY, 0
  end
  if again[1] ~= first[1] then
    ksi.trace('version 0x%x then 0x%x', first[1], again[1])
    return nt.STATUS_RETRY, 0
  end
  return nt.STATUS_SUCCESS, first[1]
end

local ACPI_ID = 'AMDI0097'
local ACPI_MS = 100
local ACPI_OUTPUT = 0x426f6541
local ACPI_INTEGER = 0
local ACPI_STRING = 1
local ACPI_BUFFER = 2
local ACPI_PACKAGE = 3
local DSD_GUID = '\x9d\x61\x4d\xb7\x07\x57\xbd\x48\xa6\x9f\x4e\xa2\x87\x1f\xc2\xf6'
local MEMORY = 1
local READ_ONLY = 0x0001
local PAGE = 0x1000
local SOCKET_DIGITS = 9

local function acpi_row(cpu)
  for _, row in ipairs(ROWS) do
    if cpu_matches(row, cpu) then
      return row
    end
  end
  return nil
end

-- The records from at to last of an ACPI answer: { kind, length, at } each,
-- or nil when one runs past last.
local function acpi_records(data, at, last)
  local list = {}
  while at <= last do
    if at + 3 > last then
      return nil
    end
    local kind, length = string.unpack('<I2I2', data, at)
    if at + 3 + math.max(4, length) > last then
      return nil
    end
    list[#list + 1] = { kind = kind, length = length, at = at + 4 }
    at = at + 4 + math.max(4, length)
  end
  return list
end

local function acpi_answer(device, name)
  local output, failed = ksi.hw.acpi_eval(device, name, ACPI_MS)
  if not output then
    ksi.trace('%s not evaluated: 0x%x', name, failed)
    return nil
  end
  if #output < 12 then
    return nil
  end
  local signature, length, count = string.unpack('<I4I4I4', output, 1)
  if signature ~= ACPI_OUTPUT or length > #output then
    ksi.trace('%s answered no output buffer', name)
    return nil
  end
  local list = acpi_records(output, 13, length)
  if not list or #list ~= count then
    ksi.trace('%s answer malformed', name)
    return nil
  end
  return output, list
end

local function acpi_integer(output, r)
  if r.kind ~= ACPI_INTEGER or (r.length ~= 4 and r.length ~= 8) then
    return nil
  end
  return string.unpack(r.length == 4 and '<I4' or '<i8', output, r.at)
end

local function acpi_text(output, r)
  if r.kind ~= ACPI_STRING or r.length < 1 then
    return nil
  end
  return string.sub(output, r.at, r.at + r.length - 2)
end

-- The socket a device's _UID names: false when it names none, nil when the
-- _UID cannot be evaluated.
local function acpi_socket(device)
  local output, list = acpi_answer(device, '_UID')
  if not output then
    return nil
  end
  if #list ~= 1 then
    return false
  end
  local number = acpi_integer(output, list[1])
  if number then
    return number
  end
  local text = acpi_text(output, list[1])
  if not text then
    return false
  end
  local at = 1
  local i, d = string.byte(text, 1, 2)
  if (i == 0x49 or i == 0x69) and (d == 0x44 or d == 0x64) then
    at = 3
  end
  if at > #text or #text - at + 1 > SOCKET_DIGITS then
    return false
  end
  local socket = 0
  for c = at, #text do
    local byte = string.byte(text, c)
    if byte < 0x30 or byte > 0x39 then
      return false
    end
    socket = socket * 10 + byte - 0x30
  end
  return socket
end

-- The three register offsets of a device's _DSD, or nil.
local function acpi_offsets(device)
  local output, top = acpi_answer(device, '_DSD')
  if not output then
    return nil
  end
  local elements = top
  if #top == 1 and top[1].kind == ACPI_PACKAGE then
    elements = acpi_records(output, top[1].at, top[1].at + top[1].length - 1)
    if not elements then
      return nil
    end
  end
  if
    #elements ~= 2
    or elements[1].kind ~= ACPI_BUFFER
    or elements[1].length ~= 16
    or string.sub(output, elements[1].at, elements[1].at + 15) ~= DSD_GUID
    or elements[2].kind ~= ACPI_PACKAGE
  then
    ksi.trace('_DSD is not the HSMP properties')
    return nil
  end
  local properties = acpi_records(output, elements[2].at, elements[2].at + elements[2].length - 1)
  if not properties then
    return nil
  end
  local found = {}
  for _, p in ipairs(properties) do
    if p.kind ~= ACPI_PACKAGE then
      return nil
    end
    local pair = acpi_records(output, p.at, p.at + p.length - 1)
    if not pair or #pair ~= 2 then
      return nil
    end
    local name, value = acpi_text(output, pair[1]), acpi_integer(output, pair[2])
    if not name or not value then
      return nil
    end
    if name == 'MsgIdOffset' or name == 'MsgRspOffset' or name == 'MsgArgOffset' then
      if found[name] then
        ksi.trace('_DSD names %s twice', name)
        return nil
      end
      found[name] = value
    end
  end
  local msg, rsp, arg = found.MsgIdOffset, found.MsgRspOffset, found.MsgArgOffset
  if not msg or not rsp or not arg then
    ksi.trace('_DSD lacks an offset')
    return nil
  end
  return msg, rsp, arg
end

-- The device's one memory window, or nil.
local function acpi_window(device)
  local resources, failed = nt.resources(device)
  if not resources then
    ksi.trace('resources not read: 0x%x', failed)
    return nil
  end
  local window
  for _, r in ipairs(resources) do
    if r.kind == MEMORY then
      if window then
        ksi.trace('more than one memory resource')
        return nil
      end
      window = r
    end
  end
  if not window then
    return nil
  end
  if
    window.raw ~= window.translated
    or window.raw == 0
    or window.raw % PAGE ~= 0
    or window.flags & READ_ONLY ~= 0
  then
    ksi.trace('window 0x%x refused', window.raw)
    return nil
  end
  local requirements = nt.requirements(device)
  if not requirements then
    return nil
  end
  for _, r in ipairs(requirements) do
    if r.minimum == window.raw and r.maximum == window.raw + window.length - 1 then
      return window.raw, window.length
    end
  end
  ksi.trace('no requirement pins 0x%x', window.raw)
  return nil
end

-- The registers' span: its first offset and its end, or nil when the offsets
-- are zero, unaligned, overlapping, in two pages or outside the window.
local function acpi_span(msg, rsp, arg, args, length)
  local spans = { { msg, msg + 4 }, { rsp, rsp + 4 }, { arg, arg + args * 4 } }
  for i, a in ipairs(spans) do
    if a[1] == 0 or a[1] % 4 ~= 0 then
      return nil
    end
    for j = i + 1, #spans do
      local b = spans[j]
      if a[1] < b[2] and b[1] < a[2] then
        return nil
      end
    end
  end
  local first = math.min(msg, rsp, arg)
  local last = math.max(msg + 4, rsp + 4, arg + args * 4)
  if first // PAGE ~= (last - 1) // PAGE or last > length then
    return nil
  end
  return first, last
end

local function address_bits()
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
    nt.NT_SUCCESS(s0)
    and maximum >= 0x80000008
    and nt.NT_SUCCESS(s1)
    and eax & 0xff >= 32
    and eax & 0xff < 64
  then
    return eax & 0xff
  end
  return 36
end

-- The mailbox of a socket's AMDI0097 device: the device, its window, the
-- offsets from its _DSD and the page mapped, kept when the script is not
-- volatile. Nil and the status of the call when it is refused.
local function acpi_session(socket)
  local k = kept.acpi and kept.acpi[socket]
  if k then
    return k
  end
  if not ksi.hw.devices or not ksi.hw.cpuid or not ksi.hw.map then
    ksi.trace('no ACPI, CPUID or map functions')
    return nil, nt.STATUS_NOT_SUPPORTED
  end
  local cpu, failed = cpu_identity()
  if not cpu then
    return nil, failed
  end
  local row = acpi_row(cpu)
  if not row then
    ksi.trace('no row for %s family 0x%x model 0x%x', cpu.vendor, cpu.family, cpu.model)
    return nil, nt.STATUS_NOT_SUPPORTED
  end
  local device
  local ordinal = 0
  while true do
    local d = nt.acpi.find(ACPI_ID, ordinal)
    if not d then
      ksi.trace('no %s device of socket %d', ACPI_ID, socket)
      return nil, nt.STATUS_NOT_FOUND
    end
    local named = acpi_socket(d)
    if named == nil then
      d:release()
      return nil, nt.STATUS_NOT_SUPPORTED
    end
    if named == socket then
      device = d
      break
    end
    d:release()
    ordinal = ordinal + 1
  end
  if nt.is_device_removable(device) then
    ksi.trace('%s device of socket %d may be removed', ACPI_ID, socket)
    device:release()
    return nil, nt.STATUS_NOT_SUPPORTED
  end
  local m = row.mailbox()
  local base, length = acpi_window(device)
  local msg, rsp, arg
  if base then
    msg, rsp, arg = acpi_offsets(device)
  end
  local first, last
  if msg then
    first, last = acpi_span(msg, rsp, arg, m.args, length)
  end
  if not first then
    device:release()
    return nil, nt.STATUS_NOT_SUPPORTED
  end
  local windowEnd = base + length
  if math.ult(windowEnd, base) or math.ult(-PAGE, windowEnd) then
    device:release()
    return nil, nt.STATUS_DEVICE_CONFIGURATION_ERROR
  end
  local bits, bitsFailed = address_bits()
  if not bits then
    device:release()
    return nil, bitsFailed
  end
  local page = first & -PAGE
  local st = nt.check_range(base + page, base + page + PAGE, bits)
  if not nt.NT_SUCCESS(st) then
    device:release()
    return nil, st
  end
  local map, mapFailed = ksi.hw.map(base + page, PAGE, true)
  if not map then
    ksi.trace('mailbox page 0x%x not mapped: 0x%x', base + page, mapFailed)
    device:release()
    return nil, mapFailed
  end
  local s = {
    device = device,
    map = map,
    base = base,
    length = length,
    page = page,
    msg = msg,
    rsp = rsp,
    arg = arg,
    mailbox = m,
  }
  if not ksi.volatile then
    s.kept = true
    kept.acpi = kept.acpi or {}
    kept.acpi[socket] = s
  end
  return s
end

local function acpi_drop(socket, s)
  if s.kept and kept.acpi[socket] == s then
    kept.acpi[socket] = nil
    s.map:release()
    s.device:release()
  end
end

local function acpi_moved(s)
  local resources = nt.resources(s.device)
  if not resources then
    return true
  end
  for _, r in ipairs(resources) do
    if r.kind == MEMORY then
      return r.raw ~= s.base or r.length ~= s.length
    end
  end
  return true
end

local function acpi_read(s, register)
  return ksi.hw.map_read(s.map, register - s.page, 4)
end

local function acpi_write(s, register, value)
  ksi.hw.map_write(s.map, register - s.page, 4, value)
end

local function acpi_wait(s, timeout)
  local st, resp = nt.STATUS_IO_TIMEOUT, 0
  poll(50, 20, 1, timeout, function()
    local value = acpi_read(s, s.rsp)
    if value == ALL_ONES then
      ksi.trace('RSP reads all-ones')
      st = nt.STATUS_DEVICE_NOT_READY
      return true
    end
    if value ~= 0 then
      st, resp = nt.STATUS_SUCCESS, value
      return true
    end
    return false
  end)
  return st, resp
end

local function acpi_command_body(s, op, args, words, state)
  local results = zeros()
  local st, resp = acpi_wait(s, STALE_MS)
  if st == nt.STATUS_IO_TIMEOUT then
    ksi.trace('RSP stayed zero: stale, written over')
    st, resp = nt.STATUS_SUCCESS, 0
  end
  if not nt.NT_SUCCESS(st) then
    return st, false, results
  end
  state.idle = resp
  state.clearing = true
  acpi_write(s, s.rsp, 0)
  state.restore = true
  state.clearing = false
  for i = 1, math.min(s.mailbox.args, ARGS) do
    acpi_write(s, s.arg + (i - 1) * 4, args[i])
  end
  state.posting = true
  acpi_write(s, s.msg, op)
  state.restore = false
  st, resp = acpi_wait(s, WAIT_MS)
  if not nt.NT_SUCCESS(st) then
    if st == nt.STATUS_IO_TIMEOUT then
      ksi.trace('RSP stayed zero')
    end
    return st, false, results
  end
  if resp ~= 0x01 then
    ksi.trace('MSG 0x%x answered 0x%x', op, resp)
    return ANSWERS[resp] or nt.STATUS_UNSUCCESSFUL, true, results, resp
  end
  for i = 1, words do
    results[i] = acpi_read(s, s.arg + (i - 1) * 4)
  end
  return nt.STATUS_SUCCESS, true, results, resp
end

local function acpi_transact(s, op, args, words)
  local state = { restore = false, idle = 0 }
  local putback <close> = setmetatable({}, {
    __close = function(_, err)
      if err and (state.restore or state.clearing) and not state.posting and state.idle ~= 0 then
        acpi_write(s, s.rsp, state.idle)
      end
    end,
  })
  local st, completed, results, answer = acpi_command_body(s, op, args, words, state)
  if state.restore and state.idle ~= 0 then
    acpi_write(s, s.rsp, state.idle)
    state.restore = false
  end
  return st, completed, results, answer or 0
end

local function acpi_protocol_version(s)
  local m = s.mailbox
  local args = { m.version_arg, 0, 0, 0, 0, 0, 0, 0 }
  local st, _, first = acpi_transact(s, m.version, args, 1)
  if not nt.NT_SUCCESS(st) then
    return st, 0
  end
  if first[1] == 0 or first[1] == ALL_ONES then
    ksi.trace('version 0x%x', first[1])
    return nt.STATUS_DEVICE_NOT_READY, 0
  end
  local again
  st, _, again = acpi_transact(s, m.version, args, 1)
  if not nt.NT_SUCCESS(st) then
    return st, 0
  end
  if again[1] == 0 or again[1] == ALL_ONES then
    ksi.trace('version 0x%x', again[1])
    return nt.STATUS_DEVICE_NOT_READY, 0
  end
  if again[1] ~= first[1] then
    ksi.trace('version 0x%x then 0x%x', first[1], again[1])
    return nt.STATUS_RETRY, 0
  end
  return nt.STATUS_SUCCESS, first[1]
end

-- The ACPI way to a socket's mailbox: its session, or false when the socket
-- has no AMDI0097 device and the root complex is used. Nil and the status of
-- the call when a device is found but refused. A kept script remembers a
-- socket without a device.
local function acpi_way(socket)
  if not ksi.hw.devices or not ksi.hw.map or (kept.direct and kept.direct[socket]) then
    return false
  end
  local s, refused = acpi_session(socket)
  if s then
    return s
  end
  if refused ~= nt.STATUS_NOT_FOUND then
    return nil, refused
  end
  if not ksi.volatile then
    kept.direct = kept.direct or {}
    kept.direct[socket] = true
  end
  return false
end

-- The commands of an input from byte at: { opcode, arguments... } each, or
-- nil when an opcode is 0.
local function entries_of(input, at)
  local entries = {}
  for i = 1, (#input - at + 1) // 36 do
    local e = pack(string.unpack('<I4I4I4I4I4I4I4I4I4', input, at + (i - 1) * 36))
    if e[1] == 0 then
      ksi.trace('entry %d has opcode 0', i)
      return nil
    end
    entries[i] = e
  end
  return entries
end

-- The arguments of an entry as the row's command permits them, or nil.
local function permitted_args(c, e)
  if not c then
    return nil
  end
  local mask, fixed = c.mask or {}, c.fixed or {}
  local args = {}
  for a = 1, ARGS do
    local value = e[a + 1]
    if (value & ~(mask[a] or 0)) ~= 0 then
      return nil
    end
    args[a] = value | (fixed[a] or 0)
  end
  return args
end

local function root_identify(input)
  local s, refused = session(input)
  if not s then
    return refused
  end
  local closer <close> = (not s.kept) and s.device or nil

  local lock <close>, failed = ksi.lock('mailbox')
  if not lock then
    ksi.trace('the mailbox lock was not taken: 0x%x', failed)
    return failed
  end

  local st, version = protocol_version(s)
  if not nt.NT_SUCCESS(st) then
    return st
  end
  return nt.STATUS_SUCCESS, string.pack('<I4I4', version, 0)
end

local function root_command(input, entries)
  local s, refused = session(input)
  if not s then
    return refused
  end
  local closer <close> = (not s.kept) and s.device or nil

  local lock <close>, failed = ksi.lock('mailbox')
  if not lock then
    ksi.trace('the mailbox lock was not taken: 0x%x', failed)
    return failed
  end

  local st = protocol_version(s)
  if not nt.NT_SUCCESS(st) then
    return st
  end

  local output = {}
  local stopped = false
  for i, e in ipairs(entries) do
    local op = e[1]
    local c = s.mailbox.commands[op]
    local status, answer = nt.STATUS_SUCCESS, 0
    local results = zeros()
    local args = permitted_args(c, e)
    if not args then
      ksi.trace('command 0x%x is not permitted', op)
      status = nt.STATUS_ACCESS_DENIED
    elseif stopped then
      status = nt.STATUS_CANCELLED
    else
      local completed
      status, completed, results, answer = transact(s, op, args, c.words)
      stopped = not completed
    end
    output[i] = string.pack('<i4I4I4I4I4I4I4I4I4I4', status, answer, unpack(results, 1, ARGS))
  end

  return nt.STATUS_SUCCESS, concat(output)
end

local function acpi_identify(s, socket)
  local device <close> = (not s.kept) and s.device or nil
  local mapping <close> = (not s.kept) and s.map or nil

  local lock <close>, failed = ksi.lock('mailbox')
  if not lock then
    ksi.trace('the mailbox lock was not taken: 0x%x', failed)
    return failed
  end

  if acpi_moved(s) then
    ksi.trace('the window moved')
    acpi_drop(socket, s)
    return nt.STATUS_DEVICE_NOT_READY
  end
  local st, version = acpi_protocol_version(s)
  if nt.NT_SUCCESS(st) and acpi_moved(s) then
    ksi.trace('the window moved')
    acpi_drop(socket, s)
    st = nt.STATUS_DEVICE_NOT_READY
  end
  if not nt.NT_SUCCESS(st) then
    return st
  end
  return nt.STATUS_SUCCESS, string.pack('<I4I4', version, 0)
end

local function acpi_command(s, socket, entries)
  local device <close> = (not s.kept) and s.device or nil
  local mapping <close> = (not s.kept) and s.map or nil

  local lock <close>, failed = ksi.lock('mailbox')
  if not lock then
    ksi.trace('the mailbox lock was not taken: 0x%x', failed)
    return failed
  end

  if acpi_moved(s) then
    ksi.trace('the window moved before the batch')
    acpi_drop(socket, s)
    return nt.STATUS_DEVICE_NOT_READY
  end
  local st = acpi_protocol_version(s)
  if not nt.NT_SUCCESS(st) then
    return st
  end

  local output = {}
  local stopped = false
  local changed = false
  for i, e in ipairs(entries) do
    local op = e[1]
    local c = s.mailbox.commands[op]
    local status, answer = nt.STATUS_SUCCESS, 0
    local results = zeros()
    local args = permitted_args(c, e)
    if not args then
      ksi.trace('command 0x%x is not permitted', op)
      status = nt.STATUS_ACCESS_DENIED
    elseif stopped then
      status = nt.STATUS_CANCELLED
    elseif acpi_moved(s) then
      ksi.trace('the window moved before entry %d', i)
      status = nt.STATUS_DEVICE_NOT_READY
      stopped = true
      changed = true
    else
      local completed
      status, completed, results, answer = acpi_transact(s, op, args, c.words)
      stopped = not completed
    end
    output[i] = string.pack('<i4I4I4I4I4I4I4I4I4I4', status, answer, unpack(results, 1, ARGS))
  end

  if acpi_moved(s) then
    ksi.trace('the window moved during the batch')
    acpi_drop(socket, s)
    return nt.STATUS_DEVICE_NOT_READY
  end
  if changed then
    acpi_drop(socket, s)
  end
  return nt.STATUS_SUCCESS, concat(output)
end

-- Returns the version of the mailbox of a socket: in the window of its
-- AMDI0097 device when it has one, otherwise behind the root complex the
-- input names.
function identify(input)
  if #input ~= 8 then
    ksi.trace('input of %d bytes', #input)
    return nt.STATUS_INVALID_PARAMETER
  end
  local socket = string.unpack('<I4', input, 5)
  local s, refused = acpi_way(socket)
  if s == nil then
    return refused
  end
  if s then
    return acpi_identify(s, socket)
  end
  return root_identify(input)
end

-- Sends a list of the row's commands to the mailbox of a socket: in the
-- window of its AMDI0097 device when it has one, otherwise behind the root
-- complex the input names.
function command(input)
  if #input < 8 + 36 or (#input - 8) % 36 ~= 0 then
    ksi.trace('input of %d bytes', #input)
    return nt.STATUS_INVALID_PARAMETER
  end
  local entries = entries_of(input, 9)
  if not entries then
    return nt.STATUS_INVALID_PARAMETER
  end
  local socket = string.unpack('<I4', input, 5)
  local s, refused = acpi_way(socket)
  if s == nil then
    return refused
  end
  if s then
    return acpi_command(s, socket, entries)
  end
  return root_command(input, entries)
end
