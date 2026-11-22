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
-- Reads the system registers of the processors. The registers each processor
-- family may be asked for are listed below; a register outside the list, or
-- one whose prerequisite does not hold, is refused.
--

local nt = require('nt')

local ROWS = {
  {
    implementer = 0x51,
    parts = { { 0x001, 0x001 } },
    variants = { { 0x1, 0x4 } },
    revisions = { { 0x0, 0xf } },
    sysregs = function()
      return {
        [0x4000] = true,
        [0x4005] = true,
        [0x4006] = true,
        [0x4020] = true,
        [0x4021] = true,
        [0x4028] = true,
        [0x5f00] = true,
        [0x5f02] = true,
        [0x5ce8] = { gate = { 0x4028, 8, 4, 0x1, 0xe } },
      }
    end,
  },
  {
    implementer = 0x51,
    parts = { { 0x002, 0x002 } },
    variants = { { 0x1, 0x3 } },
    revisions = { { 0x0, 0xf } },
    sysregs = function()
      return {
        [0x4000] = true,
        [0x4005] = true,
        [0x4006] = true,
        [0x4020] = true,
        [0x4021] = true,
        [0x4028] = true,
        [0x5f00] = true,
        [0x5f02] = true,
        [0x5ce8] = { gate = { 0x4028, 8, 4, 0x1, 0xe } },
      }
    end,
  },
  {
    implementer = 0x41,
    parts = { { 0xd4b, 0xd4b }, { 0xd4c, 0xd4c } },
    variants = { { 0x0, 0xf } },
    revisions = { { 0x0, 0xf } },
    sysregs = function()
      return {
        [0x4000] = true,
        [0x4005] = true,
        [0x4006] = true,
        [0x4020] = true,
        [0x4021] = true,
        [0x4028] = true,
        [0x5f00] = true,
        [0x5f02] = true,
        [0x5ce8] = { gate = { 0x4028, 8, 4, 0x1, 0xe } },
      }
    end,
  },
  {
    implementer = 0x41,
    parts = { { 0xd41, 0xd41 } },
    variants = { { 0x0, 0xf } },
    revisions = { { 0x0, 0xf } },
    sysregs = function()
      return {
        [0x4000] = true,
        [0x4005] = true,
        [0x4006] = true,
        [0x4020] = true,
        [0x4021] = true,
        [0x4028] = true,
        [0x5f00] = true,
        [0x5f02] = true,
        [0x5ce8] = { gate = { 0x4028, 8, 4, 0x1, 0xe } },
      }
    end,
  },
  {
    implementer = 0x41,
    parts = { { 0xd05, 0xd05 } },
    variants = { { 0x0, 0xf } },
    revisions = { { 0x0, 0xf } },
    sysregs = function()
      return {
        [0x4000] = true,
        [0x4005] = true,
        [0x4006] = true,
        [0x4020] = true,
        [0x4021] = true,
        [0x4028] = true,
        [0x5f00] = true,
        [0x5f02] = true,
        [0x5ce8] = { gate = { 0x4028, 8, 4, 0x1, 0xe } },
      }
    end,
  },
  {
    implementer = 0x41,
    parts = { { 0xd03, 0xd03 } },
    variants = { { 0x0, 0x0 } },
    revisions = { { 0x0, 0xf } },
    sysregs = function()
      return {
        [0x4000] = true,
        [0x4005] = true,
        [0x4006] = true,
        [0x4020] = true,
        [0x4021] = true,
        [0x4028] = true,
        [0x5f00] = true,
        [0x5f02] = true,
        [0x5ce8] = { gate = { 0x4028, 8, 4, 0x1, 0xe } },
      }
    end,
  },
  {
    implementer = 0x41,
    parts = { { 0xd08, 0xd08 } },
    variants = { { 0x0, 0x1 } },
    revisions = { { 0x0, 0xf } },
    sysregs = function()
      return {
        [0x4000] = true,
        [0x4005] = true,
        [0x4006] = true,
        [0x4020] = true,
        [0x4021] = true,
        [0x4028] = true,
        [0x5f00] = true,
        [0x5f02] = true,
        [0x5ce8] = { gate = { 0x4028, 8, 4, 0x1, 0xe } },
      }
    end,
  },
  {
    implementer = 0x41,
    parts = { { 0xd0b, 0xd0b } },
    variants = { { 0x0, 0x4 } },
    revisions = { { 0x0, 0xf } },
    sysregs = function()
      return {
        [0x4000] = true,
        [0x4005] = true,
        [0x4006] = true,
        [0x4020] = true,
        [0x4021] = true,
        [0x4028] = true,
        [0x5f00] = true,
        [0x5f02] = true,
        [0x5ce8] = { gate = { 0x4028, 8, 4, 0x1, 0xe } },
      }
    end,
  },
  {
    implementer = 0x41,
    parts = { { 0xd85, 0xd85 }, { 0xd87, 0xd87 } },
    variants = { { 0x0, 0x1 } },
    revisions = { { 0x0, 0xf } },
    sysregs = function()
      return {
        [0x4000] = true,
        [0x4005] = true,
        [0x4006] = true,
        [0x4020] = true,
        [0x4021] = true,
        [0x4028] = true,
        [0x5f00] = true,
        [0x5f02] = true,
        [0x5ce8] = { gate = { 0x4028, 8, 4, 0x1, 0xe } },
      }
    end,
  },
  {
    implementer = 0x51,
    parts = { { 0x800, 0x805 } },
    variants = { { 0x0, 0xf } },
    revisions = { { 0x0, 0xf } },
    sysregs = function()
      return {
        [0x4000] = true,
        [0x4005] = true,
        [0x4006] = true,
        [0x4020] = true,
        [0x4021] = true,
        [0x4028] = true,
        [0x5f00] = true,
        [0x5f02] = true,
        [0x5ce8] = { gate = { 0x4028, 8, 4, 0x1, 0xe } },
      }
    end,
  },
}

local AMU = {
  [0x5e91] = true,
  [0x5e92] = true,
  [0x5e95] = true,
  [0x5e96] = true,
  [0x5ea0] = true,
  [0x5ea1] = true,
  [0x5ea2] = true,
  [0x5ea3] = true,
  [0x5ee0] = true,
  [0x5ee1] = true,
  [0x5ee2] = true,
  [0x5ee3] = true,
  [0x5ee4] = true,
  [0x5ee5] = true,
  [0x5ee6] = true,
  [0x5ee7] = true,
  [0x5ee8] = true,
  [0x5ee9] = true,
  [0x5eea] = true,
  [0x5eeb] = true,
  [0x5eec] = true,
  [0x5eed] = true,
  [0x5eee] = true,
  [0x5eef] = true,
  [0x5ef0] = true,
  [0x5ef1] = true,
  [0x5ef2] = true,
  [0x5ef3] = true,
  [0x5ef4] = true,
  [0x5ef5] = true,
  [0x5ef6] = true,
  [0x5ef7] = true,
  [0x5ef8] = true,
  [0x5ef9] = true,
  [0x5efa] = true,
  [0x5efb] = true,
  [0x5efc] = true,
  [0x5efd] = true,
  [0x5efe] = true,
  [0x5eff] = true,
}

local PMCCNTR = 0x5ce8
local ID_AA64PFR0 = 0x4020
local ID_AA64DFR0 = 0x4028
local MIDR = 0x4000

local kept = {}
local records = {}
local permitted = {}

-- Returns the identity of the processor the input names.
function identify(input)
  if not ksi.hw.sysreg_read then
    ksi.trace('no register read function')
    if #input == 4 then
      local group, number = string.unpack('<I2I2', input)
      kept[(group << 16) | number] = nil
    end
    return nt.STATUS_NOT_SUPPORTED
  end
  if #input ~= 4 then
    ksi.trace('input of %d bytes', #input)
    return nt.STATUS_INVALID_PARAMETER
  end

  local group, number = string.unpack('<I2I2', input)
  kept[(group << 16) | number] = nil
  local results, status = ksi.hw.sysreg_read(string.pack('<I2I2I4', group, number, MIDR))
  if not results then
    ksi.trace('sysreg_read failed: 0x%x', status)
    return status
  end

  local failure, _, value = string.unpack('<i4I4I8', results, 1)
  if not nt.NT_SUCCESS(failure) then
    ksi.trace('MIDR_EL1 of processor %d:%d not read: 0x%x', group, number, failure)
    return failure
  end

  return nt.STATUS_SUCCESS, string.pack('<c12I4I4I4I4I4', '', 0, 0, 0, value & 0xffffffff, 0)
end

local function within(ranges, value)
  for _, range in ipairs(ranges) do
    if value >= range[1] and value <= range[2] then
      return true
    end
  end
  return false
end

local function find_row(midr)
  local implementer = (midr >> 24) & 0xff
  local variant = (midr >> 20) & 0xf
  local part = (midr >> 4) & 0xfff
  local revision = midr & 0xf
  for _, row in ipairs(ROWS) do
    if
      row.implementer == implementer
      and within(row.parts, part)
      and within(row.variants, variant)
      and within(row.revisions, revision)
    then
      return row
    end
  end
  return nil
end

local function read_ids(processors, codes)
  local items = {}
  for _, p in ipairs(processors) do
    for _, code in ipairs(codes[p]) do
      items[#items + 1] = string.pack('<I2I2I4', p.group, p.number, code)
    end
  end
  if #items == 0 then
    return nt.STATUS_SUCCESS
  end

  local results, status = ksi.hw.sysreg_read(table.concat(items))
  if not results then
    ksi.trace('sysreg_read failed: 0x%x', status)
    return status
  end

  local at = 1
  for _, p in ipairs(processors) do
    for _, code in ipairs(codes[p]) do
      local s, _, value = string.unpack('<i4I4I8', results, at)
      at = at + 16
      p.ids[code] = { status = s, value = value }
    end
  end

  return nt.STATUS_SUCCESS
end

local function prerequisite(p, code)
  if AMU[code] then
    return p.amu
  end
  if code == PMCCNTR then
    return p.pmu
  end
  return true
end

local function read_sysreg(input)
  if #input % 8 ~= 0 then
    ksi.trace('input of %d bytes', #input)
    return nt.STATUS_INVALID_PARAMETER
  end

  local items = {}
  local processors = {}
  local unknown = {}
  local byKey = {}

  for at = 1, #input, 8 do
    local group, number, code = string.unpack('<I2I2I4', input, at)
    local key = (group << 16) | number
    local p = byKey[key]
    if not p then
      p = { group = group, number = number, record = kept[key] }
      if not p.record then
        p.ids = {}
        p.record = p
        unknown[#unknown + 1] = p
      end
      byKey[key] = p
      processors[#processors + 1] = p
    end
    items[#items + 1] = { processor = p, code = code }
  end

  if #items == 0 then
    return nt.STATUS_SUCCESS, ''
  end

  local codes = {}
  for _, p in ipairs(unknown) do
    codes[p] = { MIDR, ID_AA64PFR0, ID_AA64DFR0 }
  end
  local status = read_ids(unknown, codes)
  if not nt.NT_SUCCESS(status) then
    return status
  end

  local gates = {}
  for _, p in ipairs(unknown) do
    local midr = p.ids[MIDR]
    gates[p] = {}
    if not nt.NT_SUCCESS(midr.status) then
      ksi.trace('MIDR_EL1 of processor %d:%d not read: 0x%x', p.group, p.number, midr.status)
      p.failure = midr.status
    else
      local midrValue = midr.value & 0xffffffff
      p.row = find_row(midrValue)
      if p.row then
        if not permitted[p.row] then
          permitted[p.row] = p.row.sysregs()
        end
        p.permitted = permitted[p.row]
      else
        ksi.trace('no registers are listed for MIDR 0x%x', midrValue)
        p.permitted = {}
      end

      local pfr0 = p.ids[ID_AA64PFR0]
      p.amu = nt.NT_SUCCESS(pfr0.status) and ((pfr0.value >> 44) & 0xf) >= 1
      local dfr0 = p.ids[ID_AA64DFR0]
      local version = (dfr0.value >> 8) & 0xf
      p.pmu = nt.NT_SUCCESS(dfr0.status) and version >= 1 and version < 0xf

      local seen = {}
      for _, entry in pairs(p.permitted) do
        local gate = type(entry) == 'table' and entry.gate
        if gate and not p.ids[gate[1]] and not seen[gate[1]] then
          seen[gate[1]] = true
          gates[p][#gates[p] + 1] = gate[1]
        end
      end
    end
  end

  status = read_ids(unknown, gates)
  if not nt.NT_SUCCESS(status) then
    return status
  end

  for _, p in ipairs(unknown) do
    if not p.failure then
      local parts = {}
      local whole = true
      for _, list in ipairs({ codes[p], gates[p] }) do
        for _, code in ipairs(list) do
          local id = p.ids[code]
          whole = whole and nt.NT_SUCCESS(id.status)
          parts[#parts + 1] = string.pack('<i4I8', id.status, id.value)
        end
      end
      if whole then
        local bytes = table.concat(parts)
        if not records[bytes] then
          records[bytes] = p
        end
        p.record = records[bytes]
        kept[(p.group << 16) | p.number] = p.record
      end
    end
  end

  local reads = {}
  for _, item in ipairs(items) do
    local p = item.processor
    local record = p.record
    local code = item.code
    item.status = nt.STATUS_SUCCESS
    item.value = 0

    if p.failure then
      item.status = p.failure
    else
      local entry = record.permitted[code]
      if not entry then
        if record.row then
          ksi.trace('system register 0x%x is not permitted', code)
        end
        item.status = nt.STATUS_NOT_FOUND
      else
        local gate = type(entry) == 'table' and entry.gate
        if gate then
          local id = record.ids[gate[1]]
          local value = (id.value >> gate[2]) & ((1 << gate[3]) - 1)
          if not nt.NT_SUCCESS(id.status) or value < gate[4] or value > gate[5] then
            ksi.trace(
              'system register 0x%x is gated off on ' .. 'processor %d:%d',
              code,
              p.group,
              p.number
            )
            item.status = nt.STATUS_NOT_FOUND
          end
        end
        if nt.NT_SUCCESS(item.status) and not prerequisite(record, code) then
          ksi.trace(
            'system register 0x%x is not implemented on ' .. 'processor %d:%d',
            code,
            p.group,
            p.number
          )
          item.status = nt.STATUS_NOT_SUPPORTED
        end
        if nt.NT_SUCCESS(item.status) then
          reads[#reads + 1] = item
        end
      end
    end
  end

  if #reads > 0 then
    local packed = {}
    for i, item in ipairs(reads) do
      packed[i] = string.pack('<I2I2I4', item.processor.group, item.processor.number, item.code)
    end
    local results, failure = ksi.hw.sysreg_read(table.concat(packed))
    if not results then
      ksi.trace('sysreg_read failed: 0x%x', failure)
      return failure
    end
    for i, item in ipairs(reads) do
      local s, _, value = string.unpack('<i4I4I8', results, (i - 1) * 16 + 1)
      item.status = s
      if nt.NT_SUCCESS(s) then
        item.value = value
      end
    end
  end

  local output = {}
  for i, item in ipairs(items) do
    output[i] = string.pack('<i4I4I8', item.status, 0, item.value)
  end

  return nt.STATUS_SUCCESS, table.concat(output)
end

-- Reads a list of registers, each on the processor its item names.
function read(input)
  if ksi.hw.sysreg_read then
    return read_sysreg(input)
  end
  ksi.trace('no register read function')
  return nt.STATUS_NOT_SUPPORTED
end
