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
-- Reads the AMD PMF metrics table of family 1Ah models 80h to 8Fh: the
-- firmware writes a sample into its table (0Eh) and names the table's address
-- and size (0Fh) through the PMF mailbox, whose page sits 10000h above the
-- base the root complex's SMN registers 13B102E8h and 13B102ECh hold. No
-- other command is ever sent.
--

local nt = require('nt')

local concat = table.concat

local FAMILY = 0x1a
local MODEL_FIRST = 0x80
local MODEL_LAST = 0x8f
local ROOT_VENDOR = 0x1022
local ROOT_DEVICE = 0x115b
local ACPI_IDS =
  { 'AMDI0102', 'AMDI0103', 'AMDI0105', 'AMDI0107', 'AMDI0108', 'AMDI0109', 'AMDI0112' }

local SMN_INDEX = 0x60
local SMN_DATA = 0x64
local SMN_BASE_LOW = 0x13b102e8
local SMN_BASE_HIGH = 0x13b102ec
local BASE_LOW_MASK = 0xfff00000
local BASE_HIGH_MASK = 0xffff
local MAILBOX_OFFSET = 0x10000

local MSG = 0xa04
local RSP = 0xa08
local ARGS = { 0xa0c, 0xaac, 0xab0 }

local LOG_SAMPLE = 0x0e
local TABLE_ADDRESS = 0x0f

local RESULT_OK = 0x01
local ANSWERS = {
  [0xfc] = nt.STATUS_DEVICE_BUSY,
  [0xfd] = nt.STATUS_INTERNAL_ERROR,
  [0xfe] = nt.STATUS_NOT_IMPLEMENTED,
}

local TABLE_SIZE = 3648
local WAIT_MS = 1000
local SETTLE_MS = 3
local ALL_ONES = 0xffffffff

local PAGE = 0x1000
local LEGACY_END = 0x100000
local FOUR_GB = 0x100000000

local MSR_SYSCFG = 0xc0010010
local MSR_TOP_MEM = 0xc001001a
local MSR_TOP_MEM2 = 0xc001001d
local MSR_SMM_ADDR = 0xc0010112
local MSR_SMM_MASK = 0xc0010113
local MSR_IORR_BASE0 = 0xc0010016
local SYSCFG_VAR_DRAM_EN = 1 << 20
local SYSCFG_TOM2_EN = 1 << 21
local TOP_MEM_LOW_BITS = 0x7fffff
local SMM_MASK_TVALID = 1 << 1
local SMM_LOW_BITS = 0x1ffff
local IORR_MASK_VALID = 1 << 11
local IORR_BASE_RDMEM = 1 << 4
local IORR_LOW_BITS = 0xfff
local IORR_COUNT = 2

local kept = {}

-- True when processor 0:0 is AMD family 1Ah model 80h to 8Fh; nil and a
-- status when CPUID failed.
local function cpu_listed()
  if kept.cpu ~= nil then
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
  local family = (signature >> 8) & 0xf
  local model = (signature >> 4) & 0xf
  if family == 0xf then
    family = family + ((signature >> 20) & 0xff)
    model = model | (((signature >> 16) & 0xf) << 4)
  end
  local listed = string.pack('<I4I4I4', b, d, c) == 'AuthenticAMD'
    and family == FAMILY
    and model >= MODEL_FIRST
    and model <= MODEL_LAST
  if not listed then
    ksi.trace('family 0x%x model 0x%x not listed', family, model)
  end
  kept.cpu = listed
  return listed
end

-- True when a PMF ACPI device is present.
local function pmf_present()
  if kept.present ~= nil then
    return kept.present
  end
  for _, id in ipairs(ACPI_IDS) do
    local device = nt.acpi.find(id, 0)
    if device then
      device:release()
      if not ksi.volatile then
        kept.present = true
      end
      return true
    end
  end
  ksi.trace('no PMF device')
  if not ksi.volatile then
    kept.present = false
  end
  return false
end

-- The physical address width of processor 0:0: CPUID 80000008h EAX[7:0], 36
-- when the leaf is absent or its width is outside 32 to 63.
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
    nt.NT_SUCCESS(s0)
    and maximum >= 0x80000008
    and nt.NT_SUCCESS(s1)
    and eax & 0xff >= 32
    and eax & 0xff < 64
  then
    kept.bits = eax & 0xff
    return kept.bits
  end
  if nt.NT_SUCCESS(s0) and nt.NT_SUCCESS(s1) then
    kept.bits = 36
  end
  return 36
end

-- The MSRs of the AMD address map, read on processor 0:0; a register that
-- faults is nil.
local function address_map()
  local indexes = { MSR_SYSCFG, MSR_TOP_MEM, MSR_TOP_MEM2, MSR_SMM_ADDR, MSR_SMM_MASK }
  for i = 0, IORR_COUNT * 2 - 1 do
    indexes[#indexes + 1] = MSR_IORR_BASE0 + i
  end
  local items = {}
  for i, index in ipairs(indexes) do
    items[i] = string.pack('<I2I2I4', 0, 0, index)
  end
  local results, status = ksi.hw.msr_read(concat(items))
  if not results then
    ksi.trace('msr_read failed: 0x%x', status)
    return nil, status
  end
  local map = {}
  for i, index in ipairs(indexes) do
    local s, _, value = string.unpack('<i4I4I8', results, (i - 1) * 16 + 1)
    if nt.NT_SUCCESS(s) then
      map[index] = value
    end
  end
  return map
end

local function amd_region(base, mask, low, limit)
  local m = mask & limit & ~low
  local first = base & m
  return first, first + ((~m & limit) + 1)
end

-- DRAM as the processor decodes it: below TOP_MEM or TOP_MEM2, not TSEG, not
-- read from I/O by an IORR. A hidden register skips the check that needs it.
local function check_amd_dram(first, last, bits)
  local limit = (1 << bits) - 1
  local msrs, failed = address_map()
  if not msrs then
    return failed
  end

  local syscfg = msrs[MSR_SYSCFG]
  if not syscfg then
    ksi.trace('DRAM bound skipped: SYSCFG hidden')
  elseif syscfg & SYSCFG_VAR_DRAM_EN == 0 then
    ksi.trace('range 0x%x refused: SYSCFG 0x%x has no TOP_MEM', first, syscfg)
    return nt.STATUS_CONFLICTING_ADDRESSES
  elseif not msrs[MSR_TOP_MEM] then
    ksi.trace('DRAM bound skipped: TOP_MEM hidden')
  else
    local top = msrs[MSR_TOP_MEM] & (limit & ~TOP_MEM_LOW_BITS)
    if top == 0 then
      ksi.trace('range 0x%x refused: TOP_MEM is zero', first)
      return nt.STATUS_CONFLICTING_ADDRESSES
    end
    local inside = not math.ult(first, LEGACY_END) and not math.ult(top, last)
    local skipped = false
    if not inside and syscfg & SYSCFG_TOM2_EN ~= 0 then
      if not msrs[MSR_TOP_MEM2] then
        ksi.trace('DRAM bound skipped: TOP_MEM2 hidden')
        skipped = true
      else
        local top2 = msrs[MSR_TOP_MEM2] & (limit & ~TOP_MEM_LOW_BITS)
        inside = not math.ult(first, FOUR_GB) and not math.ult(top2, last)
      end
    end
    if not inside and not skipped then
      ksi.trace('range 0x%x-0x%x refused: not below TOP_MEM 0x%x or TOP_MEM2', first, last, top)
      return nt.STATUS_CONFLICTING_ADDRESSES
    end
  end

  local base, mask = msrs[MSR_SMM_ADDR], msrs[MSR_SMM_MASK]
  if not base or not mask then
    ksi.trace('TSEG check skipped: TSEG hidden')
  else
    local tsegFirst, tsegLast = amd_region(base, mask, SMM_LOW_BITS, limit)
    if mask & SMM_MASK_TVALID ~= 0 and math.ult(first, tsegLast) and math.ult(tsegFirst, last) then
      ksi.trace('range 0x%x-0x%x refused: TSEG 0x%x-0x%x', first, last, tsegFirst, tsegLast)
      return nt.STATUS_CONFLICTING_ADDRESSES
    end
  end

  for i = 0, IORR_COUNT - 1 do
    local ibase = msrs[MSR_IORR_BASE0 + i * 2]
    local imask = msrs[MSR_IORR_BASE0 + i * 2 + 1]
    if not ibase or not imask then
      ksi.trace('IORR %d check skipped: IORR hidden', i)
    elseif imask & IORR_MASK_VALID ~= 0 and ibase & IORR_BASE_RDMEM == 0 then
      local ioFirst, ioLast = amd_region(ibase, imask, IORR_LOW_BITS, limit)
      if math.ult(first, ioLast) and math.ult(ioFirst, last) then
        ksi.trace('range 0x%x-0x%x refused: IORR %d reads I/O', first, last, i)
        return nt.STATUS_CONFLICTING_ADDRESSES
      end
    end
  end

  return nt.STATUS_SUCCESS
end

-- One SMN read through the root's index pair: index, data, index back, twice.
local function smn_read(root, address)
  for _ = 1, 2 do
    if not ksi.hw.pci_write(root, SMN_INDEX, 4, address) then
      ksi.trace('index write for SMN 0x%x failed', address)
      return nil, nt.STATUS_DEVICE_NOT_READY
    end
    local value = ksi.hw.pci_read(root, SMN_DATA, 4)
    local check = ksi.hw.pci_read(root, SMN_INDEX, 4)
    if not value or not check then
      ksi.trace('SMN 0x%x not read', address)
      return nil, nt.STATUS_DEVICE_NOT_READY
    end
    if check == address then
      return value
    end
  end
  ksi.trace('the index of SMN 0x%x moved', address)
  return nil, nt.STATUS_RETRY
end

-- The root complex 0:0.0 when it is the 1Ah M80h root; nil and a status.
local function open_root()
  local root, status = nt.pci.open(0, 0, 0, 0)
  if not root then
    ksi.trace('no root complex at 0:0.0: 0x%x', status)
    return nil, status
  end
  local header, failed = ksi.hw.pci_read_block(root, 0, 4)
  if not header then
    local refused = nt.pci.refused(root, failed)
    root:release()
    return nil, refused
  end
  local vendor, device = string.unpack('<I2I2', header)
  if vendor ~= ROOT_VENDOR or device ~= ROOT_DEVICE then
    ksi.trace('root complex %04x:%04x not listed', vendor, device)
    root:release()
    return nil, nt.STATUS_NOT_SUPPORTED
  end
  return root
end

-- The mailbox page mapped, kept in a kept script; nil and a status.
local function mailbox()
  if kept.mailbox then
    return kept.mailbox
  end
  local root, failed = open_root()
  if not root then
    return nil, failed
  end
  local low, high
  do
    local closer <close> = root
    local pci <close>, lockFailed = ksi.lock('pci')
    if not pci then
      ksi.trace('the PCI lock was not taken: 0x%x', lockFailed)
      return nil, lockFailed
    end
    low, failed = smn_read(root, SMN_BASE_LOW)
    if not low then
      return nil, failed
    end
    high, failed = smn_read(root, SMN_BASE_HIGH)
    if not high then
      return nil, failed
    end
  end
  local base = ((high & BASE_HIGH_MASK) << 32) | (low & BASE_LOW_MASK)
  if base == 0 then
    ksi.trace('PMF base 0')
    return nil, nt.STATUS_DEVICE_NOT_READY
  end
  local page = base + MAILBOX_OFFSET
  local bits, bitsFailed = address_bits()
  if not bits then
    return nil, bitsFailed
  end
  local st = nt.check_range(page, page + PAGE, bits)
  if not nt.NT_SUCCESS(st) then
    ksi.trace('mailbox page 0x%x refused: 0x%x', page, st)
    return nil, st
  end
  local map, mapFailed = ksi.hw.map(page, PAGE, true)
  if not map then
    ksi.trace('mailbox page 0x%x not mapped: 0x%x', page, mapFailed)
    return nil, mapFailed
  end
  if not ksi.volatile then
    kept.mailbox = map
  end
  return map
end

-- Runs body() until it returns true or timeout milliseconds passed: a stall
-- between the first 20 rounds, then a sleep.
local function poll(timeout, body)
  local limit = ksi.time() + timeout * 10000
  local round = 0
  while true do
    local late = ksi.time() >= limit
    if body() or late then
      return
    end
    if round < 20 then
      ksi.stall(50)
    else
      ksi.sleep(1)
    end
    round = round + 1
  end
end

-- RSP polled until it is not zero.
local function wait(m)
  local st, resp = nt.STATUS_IO_TIMEOUT, 0
  poll(WAIT_MS, function()
    local value = ksi.hw.map_read(m, RSP, 4)
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

-- One command with argument 0, after the last one finished; for 0Fh the three
-- result words.
local function transact(m, op)
  local st = wait(m)
  if st == nt.STATUS_IO_TIMEOUT then
    ksi.trace('RSP stayed zero: the mailbox is in use')
    return nt.STATUS_DEVICE_BUSY
  end
  if not nt.NT_SUCCESS(st) then
    return st
  end
  ksi.hw.map_write(m, RSP, 4, 0)
  ksi.hw.map_write(m, ARGS[1], 4, 0)
  ksi.hw.map_write(m, MSG, 4, op)
  local resp
  st, resp = wait(m)
  if not nt.NT_SUCCESS(st) then
    ksi.trace('MSG 0x%x not answered: 0x%x', op, st)
    return st
  end
  if resp ~= RESULT_OK then
    ksi.trace('MSG 0x%x answered 0x%x', op, resp)
    return ANSWERS[resp] or nt.STATUS_UNSUCCESSFUL
  end
  if op ~= TABLE_ADDRESS then
    return nt.STATUS_SUCCESS
  end
  ksi.sleep(SETTLE_MS)
  return nt.STATUS_SUCCESS,
    ksi.hw.map_read(m, ARGS[1], 4),
    ksi.hw.map_read(m, ARGS[2], 4),
    ksi.hw.map_read(m, ARGS[3], 4)
end

-- The table's address after a fresh sample; nil and a status.
local function sample(m)
  local lock <close>, lockFailed = ksi.lock('mailbox')
  if not lock then
    ksi.trace('the mailbox lock was not taken: 0x%x', lockFailed)
    return nil, lockFailed
  end
  local st = transact(m, LOG_SAMPLE)
  if not nt.NT_SUCCESS(st) then
    return nil, st
  end
  local low, high, size
  st, low, high, size = transact(m, TABLE_ADDRESS)
  if not nt.NT_SUCCESS(st) then
    return nil, st
  end
  if size ~= TABLE_SIZE then
    ksi.trace('table size %d, not %d', size, TABLE_SIZE)
    return nil, nt.STATUS_NOT_SUPPORTED
  end
  return (high << 32) | low
end

-- The table mapped: the kept mapping while the address holds, else checked
-- and mapped anew.
local function table_map(base)
  local kt = kept.table
  if kt then
    if kt.base == base then
      return kt.map
    end
    ksi.trace('table moved from 0x%x to 0x%x', kt.base, base)
    kept.table = nil
    kt.map:release()
  end
  if base == 0 then
    ksi.trace('table base 0')
    return nil, nt.STATUS_DEVICE_NOT_READY
  end
  local last = base + TABLE_SIZE
  if math.ult(last, base) then
    ksi.trace('table base 0x%x overflows', base)
    return nil, nt.STATUS_INTEGER_OVERFLOW
  end
  local first = base & ~(PAGE - 1)
  local rangeEnd = (last + PAGE - 1) & ~(PAGE - 1)
  local bits, failed = address_bits()
  if not bits then
    return nil, failed
  end
  local st = check_amd_dram(first, rangeEnd, bits)
  if nt.NT_SUCCESS(st) then
    st = nt.check_range(first, rangeEnd, bits)
  end
  if not nt.NT_SUCCESS(st) then
    ksi.trace('table at 0x%x refused: 0x%x', base, st)
    return nil, st
  end
  local m, mapFailed = ksi.hw.map(base, TABLE_SIZE)
  if not m then
    ksi.trace('table at 0x%x not mapped: 0x%x', base, mapFailed)
    return nil, mapFailed
  end
  if not ksi.volatile then
    kept.table = { map = m, base = base }
  end
  return m
end

-- Returns the metrics table (3648 bytes) after asking the firmware for a
-- sample.
function read(input)
  if #input ~= 0 then
    ksi.trace('input of %d bytes', #input)
    return nt.STATUS_INVALID_PARAMETER
  end
  if not ksi.hw.cpuid or not ksi.hw.pci_open or not ksi.hw.devices or not ksi.hw.map then
    ksi.trace('no CPUID, PCI, device or map functions')
    return nt.STATUS_NOT_SUPPORTED
  end
  local listed, failed = cpu_listed()
  if listed == nil then
    return failed
  end
  if not listed or not pmf_present() then
    return nt.STATUS_NOT_SUPPORTED
  end
  local m
  m, failed = mailbox()
  if not m then
    return failed
  end
  local closer <close> = (not kept.mailbox) and m or nil
  local base
  base, failed = sample(m)
  if not base then
    return failed
  end
  local t
  t, failed = table_map(base)
  if not t then
    return failed
  end
  local mapping <close> = (not kept.table) and t or nil
  local bytes = ksi.hw.map_read_block(t, 0, TABLE_SIZE, 4)
  if ksi.hw.map_read(t, 0, 4) ~= string.unpack('<I4', bytes) then
    ksi.trace('table changed during the copy')
    return nt.STATUS_RETRY
  end
  return nt.STATUS_SUCCESS, bytes
end
