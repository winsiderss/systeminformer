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
-- Reads bytes of an ACPI embedded controller through its command and data
-- ports. A controller whose _GLK is 1 is read with the ACPI Global Lock held.
-- The boards whose controller has banks, and the registers each permits, are
-- listed below; elsewhere bank 0 is read and the bank is never written.
--

local nt = require('nt')

local OBF = 0x01
local IBF = 0x02
local RD_EC = 0x80
local WR_EC = 0x81
local FLOATING = 0xff
local PORT = 2
local WAIT_US = 550
local WAIT_SPINS = 8
local WAIT_MS = 100
local PUT_BACK_ROUNDS = 182
local GLOBAL_LOCK_MS = 100
local EVAL_MS = 100

-- Each board: the SMBIOS baseboard manufacturer and products, the bank
-- register, and the { bank, first, last } registers it permits.
local BOARDS = {
  {
    manufacturer = 'ASUSTeK COMPUTER INC.',
    products = {
      'ROG CROSSHAIR VIII HERO',
      'ROG CROSSHAIR VIII HERO (WI-FI)',
      'ROG CROSSHAIR VIII FORMULA',
    },
    bank_register = 0xff,
    allow = {
      { 0, 0x3a, 0x3e },
      { 0, 0xa2, 0xa3 },
      { 0, 0xb0, 0xb1 },
      { 0, 0xb4, 0xb5 },
      { 0, 0xbc, 0xbd },
      { 0, 0xf4, 0xf4 },
      { 1, 0x00, 0x01 },
    },
  },
}

local ANY_BOARD = { allow = { { 0, 0x00, 0xff } } }

-- What a script kept between calls finds again: the board's row and each
-- controller.
local kept = {}

-- The manufacturer and product strings of the first SMBIOS type 2 (baseboard)
-- structure, or nil when the table has none.
local function baseboard_strings()
  local smbios = ksi.hw.firmware_table('RSMB', 0)
  if not smbios or #smbios < 8 then
    return nil
  end
  local last = math.min(#smbios, 8 + string.unpack('<I4', smbios, 5))
  local at = 9
  while at + 3 <= last do
    local kind, length = string.byte(smbios, at, at + 1)
    if length < 4 or at + length - 1 > last then
      return nil
    end
    local ends = string.find(smbios, '\0\0', at + length, true)
    if not ends or ends > last then
      return nil
    end
    if kind == 2 then
      if length < 6 then
        return nil
      end
      local strings = {}
      local i = at + length
      while i < ends + 1 and string.byte(smbios, i) ~= 0 do
        local stop = string.find(smbios, '\0', i, true)
        strings[#strings + 1] = string.sub(smbios, i, stop - 1)
        i = stop + 1
      end
      local manufacturer, product = string.byte(smbios, at + 4, at + 5)
      return strings[manufacturer] or '', strings[product] or ''
    end
    if kind == 127 then
      return nil
    end
    at = ends + 2
  end
  return nil
end

-- The row of this board, or the row of every other board.
local function board()
  if kept.board then
    return kept.board
  end
  local row = ANY_BOARD
  local manufacturer, product = baseboard_strings()
  for _, b in ipairs(BOARDS) do
    if manufacturer == b.manufacturer then
      for _, p in ipairs(b.products) do
        if product == p then
          row = b
        end
      end
    end
  end
  if not ksi.volatile then
    kept.board = row
  end
  return row
end

local function permitted(row, bank, reg)
  for _, a in ipairs(row.allow) do
    if bank == a[1] and reg >= a[2] and reg <= a[3] then
      return true
    end
  end
  return false
end

-- Waits until the status masked is want: spins, then sleeps, within WAIT_MS.
local function wait(s, mask, want)
  local limit = ksi.time() + WAIT_MS * 10000
  local round = 0
  while true do
    local status = ksi.hw.port_read(s.command, 1)
    if status == FLOATING then
      ksi.trace('EC status reads 0xff')
      return nt.STATUS_DEVICE_NOT_READY
    end
    if (status & mask) == want then
      return nt.STATUS_SUCCESS
    end
    if ksi.time() >= limit then
      ksi.trace('EC status 0x%02x, waiting for 0x%02x under 0x%02x', status, want, mask)
      return nt.STATUS_IO_TIMEOUT
    end
    if round < WAIT_SPINS then
      ksi.stall(WAIT_US)
    else
      ksi.sleep(1)
    end
    round = round + 1
  end
end

local function ec_read(s, address)
  local st = wait(s, IBF, 0)
  if st ~= nt.STATUS_SUCCESS then
    return st, 0
  end
  ksi.hw.port_write(s.command, 1, RD_EC)
  st = wait(s, IBF, 0)
  if st ~= nt.STATUS_SUCCESS then
    return st, 0
  end
  ksi.hw.port_write(s.data, 1, address)
  st = wait(s, OBF, OBF)
  if st ~= nt.STATUS_SUCCESS then
    return st, 0
  end
  return nt.STATUS_SUCCESS, ksi.hw.port_read(s.data, 1)
end

local function ec_write(s, address, value)
  for _, step in ipairs({ { s.command, WR_EC }, { s.data, address }, { s.data, value } }) do
    local st = wait(s, IBF, 0)
    if st ~= nt.STATUS_SUCCESS then
      return st
    end
    ksi.hw.port_write(step[1], 1, step[2])
  end
  return wait(s, IBF, 0)
end

-- The bank put back from a close handler: counted waits, as a limit may have
-- ended the call.
local function put_back(s, address, value)
  for _, step in ipairs({ { s.command, WR_EC }, { s.data, address }, { s.data, value } }) do
    local empty = false
    for _ = 1, PUT_BACK_ROUNDS do
      local status = ksi.hw.port_read(s.command, 1)
      if status ~= FLOATING and (status & IBF) == 0 then
        empty = true
        break
      end
      ksi.stall(WAIT_US)
    end
    if not empty then
      ksi.trace('EC input buffer full in the put-back')
      return
    end
    ksi.hw.port_write(step[1], 1, step[2])
  end
end

-- Whether the controller asks for the Global Lock: its _GLK, 0 when it has none.
local function global_lock_wanted(device)
  local output, failed = nt.acpi.eval(device, '_GLK', EVAL_MS)
  if not output then
    if failed == nt.STATUS_OBJECT_NAME_NOT_FOUND then
      return false
    end
    ksi.trace('_GLK failed: 0x%08x', failed)
    return nil, failed
  end
  if string.unpack('<I4', output, 9) < 1 then
    return nil, nt.STATUS_ACPI_INVALID_DATA
  end
  local kind, length = string.unpack('<I2I2', output, 13)
  if kind ~= 0 or (length ~= 4 and length ~= 8) or #output < 16 + length then
    ksi.trace('_GLK is not an integer')
    return nil, nt.STATUS_ACPI_INVALID_DATA
  end
  return string.unpack('<I4', output, 17) ~= 0
end

-- The Global Lock's guard when the controller wants it, false when it does
-- not, or nil and a status.
local function global_lock(s)
  if not s.glk then
    return false
  end
  return nt.acpi.global_lock(s.device, GLOBAL_LOCK_MS)
end

-- The controller at an ordinal of the PNP0C09 devices: its device, data and
-- command ports and whether it wants the Global Lock; kept between calls.
local function controller(ordinal)
  local key = 'ec' .. ordinal
  if kept[key] then
    return kept[key]
  end
  local device, missing = nt.acpi.find('PNP0C09', ordinal)
  if not device then
    ksi.trace('no PNP0C09 %d: 0x%x', ordinal, missing)
    return nil, missing
  end
  local resources, failed = nt.resources(device)
  if not resources then
    device:release()
    ksi.trace('resources not read: 0x%x', failed)
    return nil, failed
  end
  local ports = {}
  for _, r in ipairs(resources) do
    if r.kind == PORT then
      ports[#ports + 1] = r
    end
  end
  if #ports < 2 or ports[1].translated > 0xffff or ports[2].translated > 0xffff then
    device:release()
    ksi.trace('%d port descriptors', #ports)
    return nil, nt.STATUS_DEVICE_CONFIGURATION_ERROR
  end
  local glk, refused = global_lock_wanted(device)
  if glk == nil then
    device:release()
    return nil, refused
  end
  local s =
    { device = device, data = ports[1].translated, command = ports[2].translated, glk = glk }
  if not ksi.volatile then
    kept[key] = s
  end
  return s
end

-- The ports and flags of the controller at an ordinal: input <I4 ordinal;
-- output <I4I4I4 data port, command port, flags (1 the Global Lock is taken,
-- 2 this board's controller has banks).
function identify(input)
  if not ksi.hw.port_read then
    return nt.STATUS_NOT_SUPPORTED
  end
  if #input ~= 4 then
    return nt.STATUS_INVALID_PARAMETER
  end
  local s, refused = controller(string.unpack('<I4', input))
  if not s then
    return refused
  end
  local flags = (s.glk and 1 or 0) | (board().bank_register and 2 or 0)
  local output = string.pack('<I4I4I4', s.data, s.command, flags)
  if ksi.volatile then
    s.device:release()
  end
  return nt.STATUS_SUCCESS, output
end

-- Reads bytes of the controller at an ordinal: input <I4 ordinal, then <I4I4
-- bank and register per item; output <i4I4 status and value per item. A bank
-- written is put back before the call returns.
function read(input)
  if not ksi.hw.port_read then
    return nt.STATUS_NOT_SUPPORTED
  end
  if #input < 12 or (#input - 4) % 8 ~= 0 then
    ksi.trace('input of %d bytes', #input)
    return nt.STATUS_INVALID_PARAMETER
  end
  local row = board()
  local count = (#input - 4) // 8
  local items = {}
  local any = false
  for i = 1, count do
    local bank, reg = string.unpack('<I4I4', input, 5 + (i - 1) * 8)
    items[i] = { bank = bank, reg = reg, allowed = permitted(row, bank, reg) }
    any = any or items[i].allowed
  end
  local output = {}
  if not any then
    for i = 1, count do
      output[i] = string.pack('<i4I4', nt.STATUS_ACCESS_DENIED, 0)
    end
    return nt.STATUS_SUCCESS, table.concat(output)
  end
  local s, refused = controller(string.unpack('<I4', input))
  if not s then
    return refused
  end
  local release <close> = setmetatable({}, {
    __close = function()
      if ksi.volatile then
        s.device:release()
      end
    end,
  })
  local held <close>, refusal = global_lock(s)
  if held == nil then
    ksi.trace('the Global Lock was not taken: 0x%08x', refusal)
    return refusal
  end
  local lock <close>, failed = ksi.lock('isa')
  if not lock then
    ksi.trace('the isa lock was not taken: 0x%08x', failed)
    return failed
  end
  if (ksi.hw.port_read(s.command, 1) & OBF) ~= 0 then
    ksi.trace('the EC holds a byte of another transaction')
    return nt.STATUS_DEVICE_BUSY
  end
  local c = {}
  local bank <close> = setmetatable({}, {
    __close = function()
      if c.changed then
        put_back(s, row.bank_register, c.orig_bank)
      end
    end,
  })
  local stopped = false
  for i, item in ipairs(items) do
    local status, value = nt.STATUS_ACCESS_DENIED, 0
    if item.allowed and stopped then
      status = nt.STATUS_CANCELLED
    elseif item.allowed then
      status = nt.STATUS_SUCCESS
      if row.bank_register then
        if not c.orig_bank then
          status, c.orig_bank = ec_read(s, row.bank_register)
          if status ~= nt.STATUS_SUCCESS then
            c.orig_bank = nil
          end
          c.bank = c.orig_bank
        end
        if status == nt.STATUS_SUCCESS and item.bank ~= c.bank then
          c.changed = true
          status = ec_write(s, row.bank_register, item.bank)
          if status == nt.STATUS_SUCCESS then
            c.bank = item.bank
          end
        end
      end
      if status == nt.STATUS_SUCCESS then
        status, value = ec_read(s, item.reg)
      end
      stopped = status ~= nt.STATUS_SUCCESS
    end
    output[i] = string.pack('<i4I4', status, value)
  end
  return nt.STATUS_SUCCESS, table.concat(output)
end
