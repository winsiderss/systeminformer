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
-- Reads devices on the SMBus of the AMD FCH host and the Intel host. The
-- hosts, how each one's SMBus base is reached, and the reads permitted on
-- each port are listed below.
--

local HOSTS = {
  [0x1022790b] = {
    layouts = {
      {
        gate_min = 0x0,
        gate_max = 0x48,
        pm = 0x1,
        flags = 0x0,
        pm_length = 0x0,
        pm_base = 0x0,
        decode_low = 0x2c,
        decode_high = 0x2d,
        enable_mask = 0x1,
        base_mask = 0xffe0,
        select = 0x2,
        select_mask = 0x18,
        select_shift = 0x3,
        select_alt = 0x0,
        choose = 0x0,
        choose_mask = 0x0,
        en_reg = 0x0,
        en_mask = 0x0,
        index_port = 0xcd6,
        data_port = 0xcd7,
      },
      {
        gate_min = 0x49,
        gate_max = 0x50,
        pm = 0x1,
        flags = 0x0,
        pm_length = 0x0,
        pm_base = 0x0,
        decode_low = 0x0,
        decode_high = 0x1,
        enable_mask = 0x10,
        base_mask = 0xff00,
        select = 0x2,
        select_mask = 0x18,
        select_shift = 0x3,
        select_alt = 0x0,
        choose = 0x0,
        choose_mask = 0x0,
        en_reg = 0x0,
        en_mask = 0x0,
        index_port = 0xcd6,
        data_port = 0xcd7,
      },
      {
        gate_min = 0x51,
        gate_max = 0x71,
        pm = 0x2,
        flags = 0x0,
        pm_length = 0x8,
        pm_base = 0xfed80300,
        decode_low = 0x0,
        decode_high = 0x1,
        enable_mask = 0x10,
        base_mask = 0xff00,
        select = 0x2,
        select_mask = 0x18,
        select_shift = 0x3,
        select_alt = 0x0,
        choose = 0x0,
        choose_mask = 0x0,
        en_reg = 0x0,
        en_mask = 0x0,
        index_port = 0x0,
        data_port = 0x0,
      },
    },
    reads = {
      [0] = {
        { 0x18, 0x1f, 0x00, 0x07, false, true },
        { 0x50, 0x57, 0x00, 0xff, true, true },
        { 0x48, 0x4f, 0x04, 0x06, true, false },
        { 0x48, 0x4f, 0x08, 0x0f, true, false },
        { 0x48, 0x4f, 0x1a, 0x1b, true, false },
        { 0x48, 0x4f, 0x20, 0x2f, true, false },
        { 0x48, 0x4f, 0x30, 0x33, true, false },
        { 0x48, 0x4f, 0x3b, 0x3d, true, false },
      },
    },
  },
  [0x1022780b] = {
    layouts = {
      {
        gate_min = 0x0,
        gate_max = 0x1e,
        pm = 0x1,
        flags = 0x0,
        pm_length = 0x0,
        pm_base = 0x0,
        decode_low = 0x2c,
        decode_high = 0x2d,
        enable_mask = 0x1,
        base_mask = 0xffe0,
        select = 0x2e,
        select_mask = 0x6,
        select_shift = 0x1,
        select_alt = 0x0,
        choose = 0x0,
        choose_mask = 0x0,
        en_reg = 0x0,
        en_mask = 0x0,
        index_port = 0xcd6,
        data_port = 0xcd7,
      },
      {
        gate_min = 0x1f,
        gate_max = 0x40,
        pm = 0x1,
        flags = 0x0,
        pm_length = 0x0,
        pm_base = 0x0,
        decode_low = 0x2c,
        decode_high = 0x2d,
        enable_mask = 0x1,
        base_mask = 0xffe0,
        select = 0x2,
        select_mask = 0x18,
        select_shift = 0x3,
        select_alt = 0x0,
        choose = 0x0,
        choose_mask = 0x0,
        en_reg = 0x0,
        en_mask = 0x0,
        index_port = 0xcd6,
        data_port = 0xcd7,
      },
      {
        gate_min = 0x41,
        gate_max = 0xff,
        pm = 0x1,
        flags = 0x0,
        pm_length = 0x0,
        pm_base = 0x0,
        decode_low = 0x0,
        decode_high = 0x1,
        enable_mask = 0x10,
        base_mask = 0xff00,
        select = 0x2,
        select_mask = 0x18,
        select_shift = 0x3,
        select_alt = 0x0,
        choose = 0x0,
        choose_mask = 0x0,
        en_reg = 0x0,
        en_mask = 0x0,
        index_port = 0xcd6,
        data_port = 0xcd7,
      },
    },
    reads = {
      [0] = {
        { 0x18, 0x1f, 0x00, 0x07, false, true },
        { 0x50, 0x57, 0x00, 0xff, true, true },
      },
    },
  },
  [0x1d94790b] = {
    layouts = {
      {
        gate_min = 0x49,
        gate_max = 0x59,
        pm = 0x1,
        flags = 0x0,
        pm_length = 0x0,
        pm_base = 0x0,
        decode_low = 0x0,
        decode_high = 0x1,
        enable_mask = 0x10,
        base_mask = 0xff00,
        select = 0x2,
        select_mask = 0x18,
        select_shift = 0x3,
        select_alt = 0x0,
        choose = 0x0,
        choose_mask = 0x0,
        en_reg = 0x0,
        en_mask = 0x0,
        index_port = 0xcd6,
        data_port = 0xcd7,
      },
    },
    reads = {
      [0] = {
        { 0x18, 0x1f, 0x00, 0x07, false, true },
        { 0x50, 0x57, 0x00, 0xff, true, true },
        { 0x48, 0x4f, 0x04, 0x06, true, false },
        { 0x48, 0x4f, 0x08, 0x0f, true, false },
        { 0x48, 0x4f, 0x1a, 0x1b, true, false },
        { 0x48, 0x4f, 0x20, 0x2f, true, false },
        { 0x48, 0x4f, 0x30, 0x33, true, false },
        { 0x48, 0x4f, 0x3b, 0x3d, true, false },
      },
    },
  },
  [0x10024385] = {
    layouts = {
      {
        gate_min = 0x0,
        gate_max = 0x3f,
        pm = 0x3,
        flags = 0x2,
        pm_length = 0x0,
        pm_base = 0x0,
        decode_low = 0x90,
        decode_high = 0x91,
        enable_mask = 0x0,
        base_mask = 0xfff0,
        select = 0x0,
        select_mask = 0x0,
        select_shift = 0x0,
        select_alt = 0x0,
        choose = 0x0,
        choose_mask = 0x0,
        en_reg = 0xd2,
        en_mask = 0x1,
        index_port = 0x0,
        data_port = 0x0,
      },
      {
        gate_min = 0x40,
        gate_max = 0xff,
        pm = 0x1,
        flags = 0x0,
        pm_length = 0x0,
        pm_base = 0x0,
        decode_low = 0x2c,
        decode_high = 0x2d,
        enable_mask = 0x1,
        base_mask = 0xffe0,
        select = 0x2c,
        select_mask = 0x6,
        select_shift = 0x1,
        select_alt = 0x2e,
        choose = 0x2f,
        choose_mask = 0x1,
        en_reg = 0x0,
        en_mask = 0x0,
        index_port = 0xcd6,
        data_port = 0xcd7,
      },
    },
    reads = {
      [0] = {
        { 0x18, 0x1f, 0x00, 0x07, false, true },
        { 0x50, 0x57, 0x00, 0xff, true, true },
      },
    },
  },
  [0x10024353] = {
    layouts = {
      {
        gate_min = 0x0,
        gate_max = 0xff,
        pm = 0x3,
        flags = 0x2,
        pm_length = 0x0,
        pm_base = 0x0,
        decode_low = 0x90,
        decode_high = 0x91,
        enable_mask = 0x0,
        base_mask = 0xfff0,
        select = 0x0,
        select_mask = 0x0,
        select_shift = 0x0,
        select_alt = 0x0,
        choose = 0x0,
        choose_mask = 0x0,
        en_reg = 0xd2,
        en_mask = 0x1,
        index_port = 0x0,
        data_port = 0x0,
      },
    },
    reads = {
      [0] = {
        { 0x50, 0x57, 0x00, 0xff, true, true },
      },
    },
  },
  [0x10024363] = {
    layouts = {
      {
        gate_min = 0x0,
        gate_max = 0xff,
        pm = 0x3,
        flags = 0x2,
        pm_length = 0x0,
        pm_base = 0x0,
        decode_low = 0x90,
        decode_high = 0x91,
        enable_mask = 0x0,
        base_mask = 0xfff0,
        select = 0x0,
        select_mask = 0x0,
        select_shift = 0x0,
        select_alt = 0x0,
        choose = 0x0,
        choose_mask = 0x0,
        en_reg = 0xd2,
        en_mask = 0x1,
        index_port = 0x0,
        data_port = 0x0,
      },
    },
    reads = {
      [0] = {
        { 0x50, 0x57, 0x00, 0xff, true, true },
      },
    },
  },
  [0x10024372] = {
    layouts = {
      {
        gate_min = 0x0,
        gate_max = 0xff,
        pm = 0x3,
        flags = 0x2,
        pm_length = 0x0,
        pm_base = 0x0,
        decode_low = 0x90,
        decode_high = 0x91,
        enable_mask = 0x0,
        base_mask = 0xfff0,
        select = 0x0,
        select_mask = 0x0,
        select_shift = 0x0,
        select_alt = 0x0,
        choose = 0x0,
        choose_mask = 0x0,
        en_reg = 0xd2,
        en_mask = 0x1,
        index_port = 0x0,
        data_port = 0x0,
      },
    },
    reads = {
      [0] = {
        { 0x50, 0x57, 0x00, 0xff, true, true },
      },
    },
  },
}

local I801_DEVICES = {
  [0x80863b30] = true,
  [0x80861c22] = true,
  [0x80861e22] = true,
  [0x80868c22] = true,
  [0x80869c22] = true,
  [0x80868ca2] = true,
  [0x80869ca2] = true,
  [0x80860f12] = true,
  [0x80862292] = true,
  [0x8086a123] = true,
  [0x80869d23] = true,
  [0x80865ad4] = true,
  [0x8086a2a3] = true,
  [0x8086a323] = true,
  [0x80869da3] = true,
  [0x808634a3] = true,
  [0x808638a3] = true,
  [0x808602a3] = true,
  [0x808606a3] = true,
  [0x8086a3a3] = true,
  [0x80864b23] = true,
  [0x8086a0a3] = true,
  [0x808643a3] = true,
  [0x80864da3] = true,
  [0x80867aa3] = true,
  [0x808651a3] = true,
  [0x808654a3] = true,
  [0x80867a23] = true,
  [0x80867e22] = true,
  [0x8086ae22] = true,
  [0x80867f23] = true,
  [0x80867722] = true,
  [0x8086e322] = true,
  [0x8086e422] = true,
  [0x80864d22] = true,
  [0x80866e23] = true,
  [0x808631d4] = true,
  [0x80861d22] = true,
  [0x80868d22] = true,
  [0x8086a1a3] = true,
  [0x8086a223] = true,
  [0x80861bc9] = true,
  [0x80865796] = true,
  [0x80865827] = true,
  [0x80862330] = true,
  [0x808623b0] = true,
  [0x80861f3c] = true,
  [0x808619df] = true,
  [0x808618df] = true,
  [0x80862930] = true,
  [0x80863a30] = true,
  [0x80863a60] = true,
}

local I801_READS = {
  [0] = {
    { 0x18, 0x1f, 0x00, 0x07, false, true },
    { 0x50, 0x57, 0x00, 0xff, true, true },
    { 0x48, 0x4f, 0x04, 0x06, true, false },
    { 0x48, 0x4f, 0x08, 0x0f, true, false },
    { 0x48, 0x4f, 0x1a, 0x1b, true, false },
    { 0x48, 0x4f, 0x20, 0x2f, true, false },
    { 0x48, 0x4f, 0x30, 0x33, true, false },
    { 0x48, 0x4f, 0x3b, 0x3d, true, false },
  },
}

local KIND_FCH = 1
local KIND_INTEL = 2

local CONFIG = 0x02
local VIEW = 0x02
local PAIR = 0x01
local FLAG_KEEP_PORT = 0x01
local FLAG_NO_SEMAPHORE = 0x02
local LEGACY_IO_END = 0x100
local WAIT_MS = 35
local PAGE = 0x1000

local HST_STS = 0x00
local HST_CNT = 0x02
local HST_CMD = 0x03
local HST_ADD = 0x04
local HST_DAT0 = 0x05
local HST_DAT1 = 0x06
local SMB_CTL = 0x08

local OK = 0
local MOVED = 1
local NOT_READY = 2
local UNSUPPORTED = 3

local kept = {}

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
  local s0, maximum = string.unpack('<I4I4', results, 1)
  local s1, eax = string.unpack('<I4I4', results, 21)
  if
    s0 == ksi.STATUS_SUCCESS
    and maximum >= 0x80000008
    and s1 == ksi.STATUS_SUCCESS
    and eax & 0xff >= 32
    and eax & 0xff < 64
  then
    kept.bits = eax & 0xff
    return kept.bits
  end
  if s0 == ksi.STATUS_SUCCESS and s1 == ksi.STATUS_SUCCESS then
    kept.bits = 36
  end
  return 36
end

local function check_range(first, last, bits)
  if math.ult(1 << bits, last) then
    ksi.trace('range end 0x%x beyond %d address bits', last, bits)
    return ksi.STATUS_CONFLICTING_ADDRESSES
  end
  local nvdimm = ksi.hw.is_nvdimm(first, last - first)
  if nvdimm == nil then
    ksi.trace('NVDIMM ranges unknown')
    return ksi.STATUS_DEVICE_CONFIGURATION_ERROR
  end
  if nvdimm then
    ksi.trace('range 0x%x-0x%x overlaps NVDIMM', first, last)
    return ksi.STATUS_CONFLICTING_ADDRESSES
  end
  local ram, ramFailed = ksi.hw.is_ram(first, last - first)
  if ram == nil then
    ksi.trace('memory ranges unknown: 0x%x', ramFailed)
    return ramFailed
  end
  if ram then
    ksi.trace('range 0x%x-0x%x overlaps memory', first, last)
    return ksi.STATUS_CONFLICTING_ADDRESSES
  end
  return ksi.STATUS_SUCCESS
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

-- The revision, the layout whose gate holds the revision dword, and for a
-- layout in configuration space the base and enable registers.
local function read_host(s)
  local rev = ksi.hw.pci_read(s.device, 0x08, 1)
  if not rev or rev == 0xff then
    return NOT_READY
  end
  local index
  for i, l in ipairs(s.host.layouts) do
    local g = ksi.hw.pci_read(s.device, 0x08, 4)
    if not g or g == 0xffffffff then
      return NOT_READY
    end
    local f = g & 0xff
    if f >= l.gate_min and f <= l.gate_max then
      index = i
      break
    end
  end
  if not index then
    return UNSUPPORTED
  end
  local l = s.host.layouts[index]
  local cb, hc = 0, 0
  if l.pm == 3 then
    cb = ksi.hw.pci_read(s.device, l.decode_low, 2)
    if not cb or cb == 0xffff then
      return NOT_READY
    end
    hc = ksi.hw.pci_read(s.device, l.en_reg, 1)
    if not hc or hc == 0xff then
      return NOT_READY
    end
  end
  return OK, rev, index, cb, hc
end

-- The isa lock, unless the read already holds it for the whole transaction.
local function isa_lock(s)
  if s.isa_held then
    return nil
  end
  local lock, failed = ksi.lock('isa')
  if not lock then
    ksi.trace('the isa lock was not taken: 0x%x', failed)
    error(failed)
  end
  return lock
end

-- One PM register through the pair, under the isa lock; the index is checked
-- around the data access and put back only while it is still ours.
local function pair_in(s, reg)
  local lock <close> = isa_lock(s)
  local port = s.index_port
  local orig = ksi.hw.port_read(port, 1)
  local state = { done = false }
  local putback <close> = setmetatable({}, {
    __close = function(_, err)
      if err and not state.done and orig ~= reg and ksi.hw.port_read(port, 1) == reg then
        ksi.hw.port_write(port, 1, orig)
      end
    end,
  })
  ksi.hw.port_write(port, 1, reg)
  local idx = ksi.hw.port_read(port, 1)
  if idx == 0xff then
    state.done = true
    return NOT_READY, 0
  end
  if idx ~= reg then
    state.done = true
    return MOVED, 0
  end
  local t = ksi.hw.port_read(port + 1, 1)
  idx = ksi.hw.port_read(port, 1)
  if idx ~= reg then
    state.done = true
    return MOVED, 0
  end
  ksi.hw.port_write(port, 1, orig)
  state.done = true
  return OK, t
end

local function pair_modify(s, reg, mask, bits)
  local lock <close> = isa_lock(s)
  local port = s.index_port
  local orig = ksi.hw.port_read(port, 1)
  local state = { done = false }
  local putback <close> = setmetatable({}, {
    __close = function(_, err)
      if err and not state.done and orig ~= reg and ksi.hw.port_read(port, 1) == reg then
        ksi.hw.port_write(port, 1, orig)
      end
    end,
  })
  ksi.hw.port_write(port, 1, reg)
  local idx = ksi.hw.port_read(port, 1)
  if idx == 0xff then
    state.done = true
    return NOT_READY, 0
  end
  if idx ~= reg then
    state.done = true
    return MOVED, 0
  end
  local old = ksi.hw.port_read(port + 1, 1)
  idx = ksi.hw.port_read(port, 1)
  if idx ~= reg then
    state.done = true
    return MOVED, 0
  end
  ksi.hw.port_write(port + 1, 1, (old & ~mask) | (bits & mask))
  idx = ksi.hw.port_read(port, 1)
  if idx ~= reg then
    state.done = true
    return MOVED, 0
  end
  local t = ksi.hw.port_read(port + 1, 1)
  idx = ksi.hw.port_read(port, 1)
  if idx ~= reg then
    state.done = true
    return MOVED, 0
  end
  ksi.hw.port_write(port, 1, orig)
  state.done = true
  return OK, t
end

local function pm_in(s, reg)
  if s.view then
    return OK, ksi.hw.map_read(s.view, reg, 1)
  end
  return pair_in(s, reg)
end

-- The SMBus base from the PM decode registers: low, high, low, high again.
local function pm_decode(s)
  local l = s.layout
  local st, lo = pm_in(s, l.decode_low)
  if st ~= OK then
    return st, 0
  end
  local hi
  st, hi = pm_in(s, l.decode_high)
  if st ~= OK then
    return st, 0
  end
  local lo2
  st, lo2 = pm_in(s, l.decode_low)
  if st ~= OK then
    return st, 0
  end
  local hi2
  st, hi2 = pm_in(s, l.decode_high)
  if st ~= OK then
    return st, 0
  end
  if lo ~= lo2 or hi ~= hi2 then
    return MOVED, 0
  end
  if lo == 0xff and hi == 0xff then
    return NOT_READY, 0
  end
  local base = ((hi << 8) | lo) & l.base_mask
  if lo & l.enable_mask == 0 then
    return NOT_READY, base
  end
  return OK, base
end

local function select_port(s, to)
  local l = s.layout
  local bits = (to << l.select_shift) & l.select_mask
  local chk
  if s.view then
    local v = ksi.hw.map_read(s.view, s.pm_select, 1)
    ksi.hw.map_write(s.view, s.pm_select, 1, (v & ~l.select_mask) | bits)
    chk = ksi.hw.map_read(s.view, s.pm_select, 1)
  else
    local st
    st, chk = pair_modify(s, s.pm_select, l.select_mask, bits)
    if st ~= OK then
      return st
    end
  end
  if chk & l.select_mask ~= bits then
    return MOVED
  end
  return OK
end

local function restore_port(s)
  local l = s.layout
  local st, sel = pm_in(s, s.pm_select)
  if st ~= OK then
    return st
  end
  if (sel & l.select_mask) >> l.select_shift ~= s.port then
    return OK
  end
  return select_port(s, s.original_port)
end

-- The host reads as it did when it was opened.
local function confirm(s)
  local l = s.layout
  if s.pm_open then
    local st, b = pm_decode(s)
    if st ~= OK then
      return st
    end
    if b ~= s.pm_base then
      return NOT_READY
    end
    local sel
    st, sel = pm_in(s, s.pm_select)
    if st ~= OK then
      return st
    end
    if (sel & l.select_mask) >> l.select_shift ~= s.port then
      return NOT_READY
    end
  end
  local st, rev, index, cb, hc = read_host(s)
  if st ~= OK or rev ~= s.revision or index ~= s.index or cb ~= s.cbase or hc ~= s.hostc then
    return NOT_READY
  end
  return OK
end

local function host_in(s, reg)
  return ksi.hw.port_read(s.base + reg, 1)
end

local function host_out(s, reg, value)
  ksi.hw.port_write(s.base + reg, 1, value & 0xff)
end

-- Runs a step to its end even when the call reaches a limit meanwhile.
local function uninterrupted(step)
  local run <close> = setmetatable({}, {
    __close = function(_, failed)
      if failed == nil then
        step()
      end
    end,
  })
end

-- Forgets what is kept for a locator, then releases it.
local function forget(locator)
  local k = kept[locator]
  kept[locator] = nil
  if k.view then
    k.view:release()
  end
  k.device:release()
end

-- The PCI function the input names, its row and its layout.
local function session(locator)
  local segment, bus = locator >> 16, (locator >> 8) & 0xff
  local slot, fn = (locator >> 3) & 0x1f, locator & 0x7
  local k = kept[locator]
  local device, ids, failed
  if k then
    ids = ksi.hw.pci_read(k.device, 0, 4)
    if ids and ids == k.ids then
      device = k.device
    else
      ksi.trace('the function kept for 0x%x changed', locator)
      forget(locator)
      k = nil
    end
  end
  if not device then
    local status
    device, status = ksi.hw.pci_open(segment, bus, slot, fn)
    if not device then
      ksi.trace('no PCI function at 0x%x: 0x%x', locator, status)
      return nil, status
    end
    ids, failed = ksi.hw.pci_read(device, 0, 4)
    if not ids then
      ksi.trace('IDs of 0x%x not read: 0x%x', locator, failed)
      device:release()
      return nil, ksi.STATUS_DEVICE_NOT_READY
    end
  end
  local key = ((ids & 0xffff) << 16) | (ids >> 16)
  local s
  if I801_DEVICES[key] then
    s = { device = device, kind = KIND_INTEL, vendor = ids & 0xffff, id = ids >> 16 }
  else
    local host = HOSTS[key]
    if not host then
      ksi.trace('no row for PCI %04x:%04x', ids & 0xffff, ids >> 16)
      device:release()
      return nil, ksi.STATUS_NOT_SUPPORTED
    end
    s = { device = device, kind = KIND_FCH, host = host, vendor = ids & 0xffff, id = ids >> 16 }
    local st, rev, index, cb, hc = read_host(s)
    if st ~= OK then
      ksi.trace('host 0x%x not read: %d', locator, st)
      if k then
        forget(locator)
      else
        device:release()
      end
      return nil, st == UNSUPPORTED and ksi.STATUS_NOT_SUPPORTED or ksi.STATUS_DEVICE_NOT_READY
    end
    s.revision, s.index, s.cbase, s.hostc = rev, index, cb, hc
    s.layout = host.layouts[index]
    if k and k.view and k.view_index ~= index then
      local view = k.view
      k.view = nil
      view:release()
    end
  end
  if not k and not ksi.volatile and ksi.hw.pci_fixed(device) then
    k = { device = device, ids = ids }
    kept[locator] = k
  end
  s.kept = k
  return s
end

local function pm_failed(st, reg)
  if st == MOVED then
    ksi.trace('PM index moved at 0x%x', reg)
    return ksi.STATUS_RETRY
  end
  ksi.trace('PM pair not ready at 0x%x', reg)
  return ksi.STATUS_DEVICE_NOT_READY
end

-- The mapped view of the PM registers, behind the general check.
local function map_view(s)
  local l = s.layout
  local k = s.kept
  if k and k.view then
    return k.view
  end
  local first = l.pm_base & ~(PAGE - 1)
  local last = (l.pm_base + l.pm_length + PAGE - 1) & ~(PAGE - 1)
  local bits, failed = address_bits()
  if not bits then
    return nil, failed
  end
  local st = check_range(first, last, bits)
  if st ~= ksi.STATUS_SUCCESS then
    ksi.trace('PM view 0x%x refused: 0x%x', l.pm_base, st)
    return nil, st
  end
  local m, mapFailed = ksi.hw.map(l.pm_base, l.pm_length, true)
  if not m then
    ksi.trace('PM view 0x%x not mapped: 0x%x', l.pm_base, mapFailed)
    return nil, mapFailed
  end
  if k then
    k.view, k.view_index = m, s.index
  end
  return m
end

-- Releases a view the host does not keep.
local function close_view(s, m)
  if not (s.kept and s.kept.view == m) then
    m:release()
  end
end

-- Releases a view, kept or not.
local function drop_view(s, m)
  if s.kept and s.kept.view == m then
    s.kept.view = nil
  end
  m:release()
end

-- Reaches the PM registers the layout names and finds the SMBus base.
local function open_host(s)
  local l = s.layout
  if l.pm == 3 then
    if s.hostc & l.en_mask == 0 then
      ksi.trace('host disabled: 0x%x', s.hostc)
      return ksi.STATUS_DEVICE_NOT_READY
    end
    s.base = s.cbase & l.base_mask
  else
    if l.pm == 2 then
      local m, failed = map_view(s)
      if not m then
        return failed
      end
      s.view = m
    else
      if l.data_port ~= l.index_port + 1 then
        ksi.trace('PM pair 0x%x 0x%x', l.index_port, l.data_port)
        return ksi.STATUS_DEVICE_CONFIGURATION_ERROR
      end
      s.index_port = l.index_port
      if l.en_mask ~= 0 then
        local st, gate = pair_in(s, l.en_reg)
        if st ~= OK then
          return pm_failed(st, l.en_reg)
        end
        if gate ~= 0xff and gate & l.en_mask == l.en_mask then
          local m, failed = map_view(s)
          if not m then
            return failed
          end
          local agreed = true
          for _, r in ipairs({
            l.decode_low,
            l.decode_high,
            l.en_reg,
          }) do
            local vv = ksi.hw.map_read(m, r, 1)
            local pv
            st, pv = pair_in(s, r)
            if st ~= OK then
              drop_view(s, m)
              return pm_failed(st, r)
            end
            if vv == 0xff or vv ~= pv then
              agreed = false
              break
            end
          end
          if agreed then
            s.view = m
          else
            drop_view(s, m)
          end
        end
      end
    end
    s.pm_open = true
    s.pm_select = l.select
    if l.choose_mask ~= 0 then
      local st, ch = pm_in(s, l.choose)
      if st ~= OK then
        return pm_failed(st, l.choose)
      end
      if ch & l.choose_mask ~= 0 then
        s.pm_select = l.select_alt
      end
    end
    local st, base = pm_decode(s)
    if st ~= OK then
      return pm_failed(st, l.decode_low)
    end
    s.pm_base = base
    s.base = base
  end
  if s.base == 0 then
    ksi.trace('SMBus base 0')
    return ksi.STATUS_DEVICE_NOT_READY
  end
  if s.base < LEGACY_IO_END then
    ksi.trace('SMBus base 0x%x', s.base)
    return ksi.STATUS_DEVICE_CONFIGURATION_ERROR
  end
  return ksi.STATUS_SUCCESS
end

local function i801_host(s)
  local c = ksi.hw.pci_read(s.device, 0x04, 2)
  if not c or c == 0xffff then
    return nil
  end
  local b = ksi.hw.pci_read(s.device, 0x20, 4)
  if not b or b == 0xffffffff then
    return nil
  end
  local h = ksi.hw.pci_read(s.device, 0x40, 1)
  if not h then
    return nil
  end
  return c, b, h
end

local function i801_confirm(s)
  local c, b, h = i801_host(s)
  return c ~= nil and c == s.cmd and b == s.bar and h == s.hostc
end

local function i801_open(s)
  if s.cmd & 0x01 == 0 or s.hostc & 0x01 == 0 then
    ksi.trace('host disabled: 0x%x 0x%x', s.cmd, s.hostc)
    return ksi.STATUS_DEVICE_NOT_READY
  end
  if s.hostc & 0x04 ~= 0 or s.bar & 0x01 == 0 or s.bar & ~0xffe1 ~= 0 then
    ksi.trace('host BAR 0x%x HOSTC 0x%x', s.bar, s.hostc)
    return ksi.STATUS_DEVICE_CONFIGURATION_ERROR
  end
  s.base = s.bar & 0xffe0
  if s.base == 0 then
    ksi.trace('SMBus base 0')
    return ksi.STATUS_DEVICE_NOT_READY
  end
  if s.base < LEGACY_IO_END then
    ksi.trace('SMBus base 0x%x', s.base)
    return ksi.STATUS_DEVICE_CONFIGURATION_ERROR
  end
  return ksi.STATUS_SUCCESS
end

-- Returns the host's IDs, revision, kind (1 for the AMD FCH, 2 for the Intel
-- host) and SMBus base.
function identify(input)
  if #input ~= 4 then
    ksi.trace('input of %d bytes', #input)
    return ksi.STATUS_INVALID_PARAMETER
  end
  local lock <close>, failed = ksi.lock('smbus')
  if not lock then
    ksi.trace('the smbus lock was not taken: 0x%x', failed)
    return failed
  end
  local s, refused = session(string.unpack('<I4', input))
  if not s then
    return refused
  end
  local closer <close> = (not s.kept) and s.device or nil
  if s.kind == KIND_INTEL then
    s.cmd, s.bar, s.hostc = i801_host(s)
    if not s.cmd then
      ksi.trace('host configuration reads all ones')
      return ksi.STATUS_DEVICE_NOT_READY
    end
    local st = i801_open(s)
    if st ~= ksi.STATUS_SUCCESS then
      return st
    end
    local revision = ksi.hw.pci_read(s.device, 0x08, 1)
    if not revision then
      ksi.trace('host revision not read')
      return ksi.STATUS_DEVICE_NOT_READY
    end
    return ksi.STATUS_SUCCESS,
      string.pack('<I2I2I4I4I4', s.vendor, s.id, revision, KIND_INTEL, s.base)
  end
  local st = open_host(s)
  if s.view then
    close_view(s, s.view)
  end
  if st ~= ksi.STATUS_SUCCESS then
    return st
  end
  return ksi.STATUS_SUCCESS, string.pack('<I2I2I4I4I4', s.vendor, s.id, s.revision, 1, s.base)
end

-- The transaction up to the end of the wait; a transfer that does not end
-- is killed.
local function transact(s, hs, c, n)
  local sts = host_in(s, HST_STS)
  if sts & 0x01 ~= 0 then
    ksi.trace('the host is busy: 0x%x', sts)
    return ksi.STATUS_RETRY
  end
  if sts & 0x1e ~= 0 then
    host_out(s, HST_STS, sts & 0x1e)
    sts = host_in(s, HST_STS)
    if sts & 0x1e ~= 0 then
      ksi.trace('host status 0x%x does not clear', sts)
      return ksi.STATUS_DEVICE_NOT_READY
    end
  end
  host_out(s, HST_ADD, hs)
  host_out(s, HST_CMD, c)
  host_out(s, HST_CNT, n)
  host_out(s, HST_CNT, n | 0x40)
  ksi.stall(250)
  local done = false
  poll(25, 80, 1, WAIT_MS, function()
    sts = host_in(s, HST_STS)
    done = sts & 0x01 == 0 and sts & 0x1e ~= 0
    return done
  end)
  if not done then
    local k = host_in(s, HST_CNT)
    host_out(s, HST_CNT, (k & ~0x40) | 0x02)
    ksi.sleep(1)
    k = host_in(s, HST_CNT)
    host_out(s, HST_CNT, k & ~0x42)
    k = host_in(s, HST_STS)
    if k & 0x1e ~= 0 then
      host_out(s, HST_STS, k & 0x1e)
    end
    ksi.trace('transaction to 0x%x timed out', hs)
    return ksi.STATUS_IO_TIMEOUT
  end
  return ksi.STATUS_SUCCESS, sts
end

-- The transaction after the data reads: a collision, the clear, the errors.
local function finish(s, hs, c, n, sts)
  local a2 = host_in(s, HST_ADD)
  local c2 = host_in(s, HST_CMD)
  local n2 = host_in(s, HST_CNT)
  if a2 ~= hs or c2 ~= c or n2 & 0x1c ~= n then
    ksi.trace("another agent's transaction on the host")
    return ksi.STATUS_RETRY, true
  end
  if sts & 0x1e ~= 0 then
    host_out(s, HST_STS, sts & 0x1e)
  end
  if sts & 0x18 ~= 0 then
    ksi.trace('transaction to 0x%x failed: 0x%x', hs, sts)
    return ksi.STATUS_RETRY, true
  end
  if sts & 0x04 ~= 0 then
    ksi.trace('no device answered at 0x%x', hs)
    return ksi.STATUS_NO_SUCH_DEVICE, false
  end
  return ksi.STATUS_SUCCESS, false
end

-- Puts the port select back (five tries 1 ms apart) and releases the
-- semaphore; true when the port was not put back.
local function release(s)
  local bad = false
  if s.port_changed then
    local rs = MOVED
    for _ = 1, 5 do
      rs = restore_port(s)
      if rs ~= MOVED then
        break
      end
      ksi.sleep(1)
    end
    bad = rs ~= OK
    s.port_changed = false
  end
  if s.layout.flags & FLAG_NO_SEMAPHORE == 0 then
    local ctl = host_in(s, SMB_CTL)
    host_out(s, SMB_CTL, ctl | 0x20)
  end
  return bad
end

local function i801_transact(s, hs, c, n)
  local sts = host_in(s, HST_STS)
  if sts & 0x01 ~= 0 then
    ksi.trace('the host is busy: 0x%x', sts)
    s.collided = true
    return ksi.STATUS_RETRY
  end
  if sts & 0x9e ~= 0 then
    host_out(s, HST_STS, sts & 0x9e)
    sts = host_in(s, HST_STS)
    if sts & 0x9e ~= 0 then
      ksi.trace('host status 0x%x does not clear', sts)
      return ksi.STATUS_DEVICE_NOT_READY
    end
  end
  s.started = true
  host_out(s, HST_ADD, hs)
  host_out(s, HST_CMD, c)
  host_out(s, HST_CNT, n | 0x40)
  ksi.stall(25)
  local done = false
  poll(25, 80, 1, WAIT_MS, function()
    sts = host_in(s, HST_STS)
    done = sts & 0x01 == 0 and sts & 0x1e ~= 0
    return done
  end)
  if not done then
    local k = host_in(s, HST_CNT)
    host_out(s, HST_CNT, (k & ~0x40) | 0x02)
    ksi.sleep(1)
    k = host_in(s, HST_CNT)
    host_out(s, HST_CNT, k & ~0x42)
    k = host_in(s, HST_STS)
    if k & 0x9e ~= 0 then
      host_out(s, HST_STS, k & 0x9e)
    end
    ksi.trace('transaction to 0x%x timed out', hs)
    return ksi.STATUS_IO_TIMEOUT
  end
  return ksi.STATUS_SUCCESS, sts
end

local function i801_finish(s, hs, c, n, sts)
  local a2 = host_in(s, HST_ADD)
  local c2 = host_in(s, HST_CMD)
  local n2 = host_in(s, HST_CNT)
  if a2 ~= hs or c2 ~= c or n2 & 0x1c ~= n then
    ksi.trace("another agent's transaction on the host")
    s.collided = true
    return ksi.STATUS_RETRY, true
  end
  if sts & 0x9e ~= 0 then
    host_out(s, HST_STS, sts & 0x9e)
  end
  if sts & 0x18 ~= 0 then
    ksi.trace('transaction to 0x%x failed: 0x%x', hs, sts)
    return ksi.STATUS_RETRY, true
  end
  if sts & 0x04 ~= 0 then
    ksi.trace('no device answered at 0x%x', hs)
    return ksi.STATUS_NO_SUCH_DEVICE, false
  end
  return ksi.STATUS_SUCCESS, false
end

local function i801_release(s)
  local clear = 0
  if s.started and not s.collided then
    clear = 0x9e
  end
  host_out(s, HST_STS, 0x40 | clear)
end

local function permitted(lines, address, command, protocol)
  for _, line in ipairs(lines) do
    if
      address >= line[1]
      and address <= line[2]
      and command >= line[3]
      and command <= line[4]
      and ((protocol == 0 and line[5]) or (protocol == 1 and line[6]))
    then
      return true
    end
  end
  return false
end

-- The isa lock for the whole transaction when the PM registers are reached
-- through the pair; false when they are not.
local function pair_lock(s)
  if s.pm_open and not s.view then
    return ksi.lock('isa')
  end
  return false
end

local function i801_read(s, entries, port)
  s.cmd, s.bar, s.hostc = i801_host(s)
  if not s.cmd then
    ksi.trace('host configuration reads all ones')
    return ksi.STATUS_DEVICE_NOT_READY
  end
  local lines = I801_READS[port]
  local allowed = 0
  for _, e in ipairs(entries) do
    e.allowed = lines and permitted(lines, e[1], e[2], e[3])
    if e.allowed then
      allowed = allowed + 1
    else
      ksi.trace('0x%x command 0x%x protocol %d is not permitted', e[1], e[2], e[3])
    end
  end
  if allowed == 0 then
    local denied = {}
    for i = 1, #entries do
      denied[i] = string.pack('<I4I4', ksi.STATUS_ACCESS_DENIED, 0)
    end
    return ksi.STATUS_SUCCESS, table.concat(denied)
  end
  local st = i801_open(s)
  if st ~= ksi.STATUS_SUCCESS then
    return st
  end

  local putback <close> = setmetatable({}, {
    __close = function()
      if s.owned or (s.claim and s.claim & 0x40 == 0) then
        i801_release(s)
      end
    end,
  })
  uninterrupted(function()
    s.claim = host_in(s, HST_STS)
  end)
  if s.claim & 0x40 ~= 0 then
    ksi.trace('the host is in use')
    return ksi.STATUS_RETRY
  end
  s.owned = true
  if host_in(s, HST_STS) & 0x01 ~= 0 then
    ksi.trace('the host is busy')
    return ksi.STATUS_RETRY
  end

  local output = {}
  local stopped
  for i, e in ipairs(entries) do
    local es, value = ksi.STATUS_SUCCESS, 0
    if not e.allowed then
      es = ksi.STATUS_ACCESS_DENIED
    elseif stopped then
      es = ksi.STATUS_CANCELLED
    elseif not i801_confirm(s) then
      ksi.trace('the host changed before entry %d', i)
      es = ksi.STATUS_RETRY
      stopped = true
    else
      local hs = (e[1] << 1) | 1
      local cnt = e[3] == 1 and 0x0c or 0x08
      local sts
      es, sts = i801_transact(s, hs, e[2], cnt)
      if es == ksi.STATUS_SUCCESS then
        local v = host_in(s, HST_DAT0)
        if e[3] == 1 then
          v = v | (host_in(s, HST_DAT1) << 8)
        end
        local abort
        es, abort = i801_finish(s, hs, e[2], cnt, sts)
        if es == ksi.STATUS_SUCCESS then
          value = v
        elseif abort then
          stopped = true
        end
      else
        stopped = true
      end
    end
    output[i] = { es, value }
  end

  if not i801_confirm(s) then
    ksi.trace('the host changed during the read')
    for _, o in ipairs(output) do
      if o[1] == ksi.STATUS_SUCCESS or o[1] == ksi.STATUS_NO_SUCH_DEVICE then
        o[1], o[2] = ksi.STATUS_DEVICE_NOT_READY, 0
      end
    end
  end

  local packed = {}
  for i, o in ipairs(output) do
    packed[i] = string.pack('<I4I4', o[1], o[2])
  end
  return ksi.STATUS_SUCCESS, table.concat(packed)
end

-- Reads bytes and words from devices on the host's SMBus port.
function read(input)
  if #input < 12 or (#input - 8) % 4 ~= 0 then
    ksi.trace('input of %d bytes', #input)
    return ksi.STATUS_INVALID_PARAMETER
  end
  local locator, port = string.unpack('<I4I4', input)
  local entries = {}
  for i = 1, (#input - 8) // 4 do
    local address, command, protocol, zero = string.unpack('<I1I1I1I1', input, 9 + (i - 1) * 4)
    if address > 0x7f or protocol > 1 or zero ~= 0 then
      ksi.trace('entry %d malformed', i)
      return ksi.STATUS_INVALID_PARAMETER
    end
    entries[i] = { address, command, protocol }
  end

  local lock <close>, failed = ksi.lock('smbus')
  if not lock then
    ksi.trace('the smbus lock was not taken: 0x%x', failed)
    return failed
  end

  local s, refused = session(locator)
  if not s then
    return refused
  end
  local closer <close> = (not s.kept) and s.device or nil
  if s.kind == KIND_INTEL then
    return i801_read(s, entries, port)
  end
  s.port = port

  local lines = s.host.reads[port]
  local allowed = 0
  for _, e in ipairs(entries) do
    e.allowed = lines and permitted(lines, e[1], e[2], e[3])
    if e.allowed then
      allowed = allowed + 1
    else
      ksi.trace('0x%x command 0x%x protocol %d is not permitted', e[1], e[2], e[3])
    end
  end
  if allowed == 0 then
    local denied = {}
    for i = 1, #entries do
      denied[i] = string.pack('<I4I4', ksi.STATUS_ACCESS_DENIED, 0)
    end
    return ksi.STATUS_SUCCESS, table.concat(denied)
  end

  local view <close> = setmetatable({}, {
    __close = function()
      if s.view then
        close_view(s, s.view)
      end
    end,
  })
  local st = open_host(s)
  if st ~= ksi.STATUS_SUCCESS then
    return st
  end

  local l = s.layout
  local isa <close>, isaFailed = pair_lock(s)
  if isa == nil then
    ksi.trace('the isa lock was not taken: 0x%x', isaFailed)
    return isaFailed
  end
  s.isa_held = isa ~= false
  local putback <close> = setmetatable({}, {
    __close = function(_, err)
      if err and (s.owned or (s.requested and host_in(s, SMB_CTL) & 0x10 ~= 0)) then
        release(s)
      end
    end,
  })
  if l.flags & FLAG_NO_SEMAPHORE == 0 then
    local ctl = host_in(s, SMB_CTL)
    if ctl & 0x10 ~= 0 then
      ksi.trace('the host semaphore is taken')
      return ksi.STATUS_RETRY
    end
    local granted = false
    for _ = 1, 5 do
      s.requested = true
      host_out(s, SMB_CTL, ctl | 0x10)
      ctl = host_in(s, SMB_CTL)
      if ctl & 0x10 ~= 0 then
        granted = true
        break
      end
      ksi.sleep(1)
    end
    if not granted then
      ksi.trace('the host semaphore was not granted')
      s.requested = false
      return ksi.STATUS_RETRY
    end
  end
  s.owned = true

  local status = ksi.STATUS_SUCCESS
  local keep = false
  local acquired = false
  repeat
    local sts = host_in(s, HST_STS)
    if sts & 0x01 ~= 0 then
      ksi.trace('the host is busy')
      status = ksi.STATUS_RETRY
      break
    end
    if s.pm_open then
      local sel
      st, sel = pm_in(s, s.pm_select)
      if st ~= OK then
        keep = st == NOT_READY
        status = pm_failed(st, s.pm_select)
        break
      end
      s.original_port = (sel & l.select_mask) >> l.select_shift
      if s.original_port ~= port then
        if l.flags & FLAG_KEEP_PORT ~= 0 then
          keep = true
          ksi.trace('the row keeps port %d', s.original_port)
          status = ksi.STATUS_NOT_SUPPORTED
          break
        end
        s.port_changed = true
        st = select_port(s, port)
        if st ~= OK then
          keep = st == NOT_READY
          status = pm_failed(st, s.pm_select)
          break
        end
      end
    end
    sts = host_in(s, HST_STS)
    if sts & 0x01 ~= 0 then
      ksi.trace('the host is busy')
      status = ksi.STATUS_RETRY
      break
    end
    acquired = true
  until true

  local output = {}
  if acquired then
    local stopped
    for i, e in ipairs(entries) do
      local es, value = ksi.STATUS_SUCCESS, 0
      if not e.allowed then
        es = ksi.STATUS_ACCESS_DENIED
      elseif stopped then
        es = ksi.STATUS_CANCELLED
      else
        if not s.pm_open or s.view then
          if confirm(s) ~= OK then
            ksi.trace('the host changed before entry %d', i)
            es = ksi.STATUS_RETRY
            stopped = true
          end
        end
        if es == ksi.STATUS_SUCCESS then
          local hs = (e[1] << 1) | 1
          local cnt = e[3] == 1 and 0x0c or 0x08
          local sts
          es, sts = transact(s, hs, e[2], cnt)
          if es == ksi.STATUS_SUCCESS then
            local v = host_in(s, HST_DAT0)
            if e[3] == 1 then
              v = v | (host_in(s, HST_DAT1) << 8)
            end
            local abort
            es, abort = finish(s, hs, e[2], cnt, sts)
            if es == ksi.STATUS_SUCCESS then
              value = v
            elseif abort then
              stopped = true
            end
          else
            stopped = true
          end
        end
      end
      output[i] = { es, value }
    end

    local cs = confirm(s)
    if cs ~= OK then
      ksi.trace('the host changed during the read')
      local to = cs == MOVED and ksi.STATUS_RETRY or ksi.STATUS_DEVICE_NOT_READY
      for _, o in ipairs(output) do
        if o[1] == ksi.STATUS_SUCCESS or o[1] == ksi.STATUS_NO_SUCH_DEVICE then
          o[1], o[2] = to, 0
        end
      end
    end
  end

  local bad = release(s)
  s.owned = false
  if bad and not keep then
    ksi.trace('the port select was not put back')
    status = ksi.STATUS_DEVICE_CONFIGURATION_ERROR
  end
  if status ~= ksi.STATUS_SUCCESS then
    return status
  end

  local packed = {}
  for i, o in ipairs(output) do
    packed[i] = string.pack('<I4I4', o[1], o[2])
  end
  return ksi.STATUS_SUCCESS, table.concat(packed)
end
