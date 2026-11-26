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
-- Reads the temperature of an AQtion network controller through its memory BAR:
-- a status block on the second generation, a firmware mailbox request under the
-- RAM semaphore on the first (firmware 2 to 4).
--

local nt = require('nt')

local VENDOR = 0x1d6a
-- The generation of each device.
local DEVICES = {
  [0x07b1] = 1,
  [0x08b1] = 1,
  [0x09b1] = 1,
  [0x87b1] = 1,
  [0x88b1] = 1,
  [0x89b1] = 1,
  [0x00c0] = 2,
  [0x04c0] = 2,
  [0x94c0] = 2,
  [0x93c0] = 2,
  [0x14c0] = 2,
  [0x34c0] = 2,
  [0x12c0] = 2,
  [0x11c0] = 2,
}
local BLOCK = 0x13000
local HEALTH = 0x620
local PAGE = 0x1000
local TRIES = 16
local CONTROLLERS_MAX = 8

local VERSION = 0x18
local MIF_COMMAND = 0x200
local MIF_ADDRESS = 0x208
local MIF_VALUE = 0x20c
local MAILBOX = 0x360
local CONTROL2 = 0x36c
local STATE2 = 0x374
local SEMAPHORE = 0x3a8
local TEMPERATURE_REQUEST = 0x40000
local MAILBOX_TEMPERATURE = 0x50
local MIF_READ = 0x8000
local MIF_BUSY = 0x100
local POLLS = 100
local ANSWER_STALL = 100
local MIF_STALL = 10

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
  local s = { device = device, key = header_key(header), kept = false, generation = DEVICES[id] }
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

-- The page at Offset, mapped once per base after checking it lies in a memory
-- resource of the function.
local function page(s, base, offset, writable)
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
  if not length or length < offset + PAGE then
    ksi.trace('BAR 0x%x is not a memory resource holding the page', base)
    return nil, nt.STATUS_DEVICE_CONFIGURATION_ERROR
  end
  local bits, bitsFailed = address_bits()
  if not bits then
    return nil, bitsFailed
  end
  local st = nt.check_range(base + offset, base + offset + PAGE, bits)
  if not nt.NT_SUCCESS(st) then
    return nil, st
  end
  local m, mapFailed = ksi.hw.map(base + offset, PAGE, writable)
  if not m then
    ksi.trace('page 0x%x not mapped: 0x%x', base + offset, mapFailed)
    return nil, mapFailed
  end
  s.map, s.base = m, base
  return m
end

-- The second generation's temperature in millidegrees C, or nil and a status.
local function block_temperature(m)
  for _ = 1, TRIES do
    local before = ksi.hw.map_read(m, 0, 4)
    if before == 0xffffffff then
      ksi.trace('the block reads all-ones')
      return nil, nt.STATUS_DEVICE_NOT_READY
    end
    if before & 0xffff == before >> 16 then
      local health = ksi.hw.map_read(m, HEALTH, 4)
      local after = ksi.hw.map_read(m, 0, 4)
      if after == before then
        local raw = (health >> 8) & 0xff
        return (raw >= 0x80 and raw - 0x100 or raw) * 1000
      end
    end
  end
  ksi.trace('the firmware kept writing the block')
  return nil, nt.STATUS_DEVICE_BUSY
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

-- Polls Done up to POLLS times, Stall microseconds apart; whether it held.
local function poll(done, stall)
  for _ = 1, POLLS do
    if done() then
      return true
    end
    ksi.stall(stall)
  end
  return done()
end

-- One dword of firmware RAM through the memory interface, caller holding the
-- RAM semaphore; nil and a status.
local function ram_read(m, address)
  ksi.hw.map_write(m, MIF_ADDRESS, 4, address)
  ksi.hw.map_write(m, MIF_COMMAND, 4, MIF_READ)
  local done = poll(function()
    return ksi.hw.map_read(m, MIF_COMMAND, 4) & MIF_BUSY == 0
      or ksi.hw.map_read(m, MIF_ADDRESS, 4) ~= address
  end, MIF_STALL)
  if not done then
    return nil, nt.STATUS_IO_TIMEOUT
  end
  return ksi.hw.map_read(m, MIF_VALUE, 4)
end

-- The first generation's PHY temperature in millidegrees C, or nil and a
-- status.
local function mailbox_temperature(m)
  local version = ksi.hw.map_read(m, VERSION, 4)
  if version == 0xffffffff then
    return nil, nt.STATUS_DEVICE_NOT_READY
  end
  local major = version >> 24
  if major < 2 or major > 4 then
    ksi.trace('firmware 0x%x has no temperature', version)
    return nil, nt.STATUS_NOT_SUPPORTED
  end
  local lock <close>, refused = ksi.lock('pci')
  if not lock then
    return nil, refused
  end
  local control = ksi.hw.map_read(m, CONTROL2, 4)
  local asked = control & TEMPERATURE_REQUEST
  ksi.hw.map_write(m, CONTROL2, 4, control ~ TEMPERATURE_REQUEST)
  if
    not poll(function()
      return ksi.hw.map_read(m, STATE2, 4) & TEMPERATURE_REQUEST ~= asked
    end, ANSWER_STALL)
  then
    ksi.trace('the firmware did not answer')
    return nil, nt.STATUS_IO_TIMEOUT
  end
  local mailbox = ksi.hw.map_read(m, MAILBOX, 4)
  local taken = false
  local held <close> = setmetatable({}, {
    __close = function()
      if taken then
        ksi.hw.map_write(m, SEMAPHORE, 4, 1)
      end
    end,
  })
  if
    not poll(function()
      uninterrupted(function()
        taken = ksi.hw.map_read(m, SEMAPHORE, 4) == 1
      end)
      return taken
    end, ANSWER_STALL)
  then
    ksi.trace('the RAM semaphore is held')
    return nil, nt.STATUS_DEVICE_BUSY
  end
  local value, failed = ram_read(m, mailbox + MAILBOX_TEMPERATURE)
  if not value then
    return nil, failed
  end
  local raw = value & 0xffff
  if raw >= 0x8000 then
    raw = raw - 0x10000
  end
  return raw * 1000 // 256
end

-- The temperature in millidegrees C, or nil and a status.
local function temperature(s)
  local st = in_d0(s)
  if not nt.NT_SUCCESS(st) then
    return nil, st
  end
  local base, failed = bar(s)
  if not base then
    return nil, failed
  end
  if s.generation == 1 then
    local m, mapFailed = page(s, base, 0, true)
    if not m then
      return nil, mapFailed
    end
    return mailbox_temperature(m)
  end
  local m, mapFailed = page(s, base, BLOCK, false)
  if not m then
    return nil, mapFailed
  end
  return block_temperature(m)
end

-- Per 4-byte locator (segment << 16 | bus << 8 | device << 3 | function): an i4
-- status and i4 millidegrees C.
function read(input)
  if not ksi.hw.pci_open or not ksi.hw.map or not ksi.hw.cpuid then
    ksi.trace('no PCI, memory or CPUID functions')
    return nt.STATUS_NOT_SUPPORTED
  end
  if #input == 0 or #input % 4 ~= 0 or #input > 4 * CONTROLLERS_MAX then
    ksi.trace('input of %d bytes', #input)
    return nt.STATUS_INVALID_PARAMETER
  end
  local output = {}
  for at = 1, #input, 4 do
    local s, refused = session(string.unpack('<I4', input, at))
    local millidegrees, status = nil, refused
    if s then
      millidegrees, status = temperature(s)
      if not s.kept then
        release(s)
      end
    end
    if millidegrees then
      output[#output + 1] = string.pack('<i4i4', nt.STATUS_SUCCESS, millidegrees)
    else
      output[#output + 1] = string.pack('<i4i4', status, 0)
    end
  end
  return nt.STATUS_SUCCESS, table.concat(output)
end
