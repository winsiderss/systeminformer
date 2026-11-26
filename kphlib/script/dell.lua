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
-- Reads the fans and temperatures of the Dell machines below through the
-- firmware's SMM interface (port B2h, processor 0), found by the manufacturer
-- and product name of the SMBIOS system structure. The rows carry their
-- quirks: no fan command on the machines where one stalls
-- the system for about 500 ms, no fan type command where it upsets the fans,
-- and never a fan control command. Two commands in a row that take longer
-- than SLOW_MS end the routine and every later one: firmware this slow stalls
-- every processor while it runs.
--

local nt = require('nt')

local SMI_PORT = 0xb2
local SLOW_MS = 50

local GET_SPEED = 0x02a3
local GET_FAN_TYPE = 0x03a3
local GET_NOMINAL_SPEED = 0x04a3
local GET_TEMP = 0x10a3
local GET_TEMP_TYPE = 0x11a3
local GET_SIGNATURE1 = 0xfea3
local GET_SIGNATURE2 = 0xffa3

local SIGNATURE_EAX = 0x44494147 -- DIAG
local SIGNATURE_EDX = 0x44454c4c -- DELL

local FANS_MAX = 4
local TEMPS_MAX = 10
local FAN_STATES_MAX = 3
local FAN_MULTIPLIER = 30
local FAN_RPM_THRESHOLD = 1000
local TEMP_MAX = 127
local ABSENT = 0xff

-- Manufacturer and product name, each matched contained, or the whole string
-- where exact.
local MACHINES = {
  { 'DELL INC.', 'G5 5590', exact = true },
  { 'DELL INC.', 'G5 5505', exact = true },
  { 'DELL COMPUTER', 'INSPIRON' },
  { 'DELL COMPUTER', 'LATITUDE' },
  { 'DELL INC.', 'INSPIRON' },
  { 'DELL INC.', 'LATITUDE' },
  { 'DELL INC.', 'MM061' },
  { 'DELL INC.', 'MP061' },
  { 'DELL INC.', 'OPTIPLEX 7080', exact = true },
  { 'DELL INC.', 'OPTIPLEX 7060', exact = true },
  { 'DELL INC.', 'OPTIPLEX 7050', exact = true },
  { 'DELL INC.', 'OPTIPLEX 7040', exact = true },
  { 'DELL INC.', 'PRECISION' },
  { 'DELL INC.', 'VOSTRO' },
  { 'DELL INC.', 'STUDIO' },
  { 'DELL INC.', 'MXC051' },
  { 'DELL INC.', 'XPS' },
}

-- Fan commands stall these for about 500 ms.
local NO_FANS = {
  ['INSPIRON 7720'] = true,
  ['VOSTRO 3360'] = true,
  ['XPS13 9333'] = true,
  ['DELL SYSTEM XPS L502X'] = true,
}

-- The fan type command makes these fans' speed go up and down.
local NO_FAN_TYPE = {
  ['STUDIO XPS 8000'] = true,
  ['STUDIO XPS 8100'] = true,
  ['INSPIRON 580'] = true,
  ['INSPIRON 3505'] = true,
}

local kept = {}

-- The bytes of an SMBIOS string trimmed of spaces and tabs, with a to z made
-- upper case.
local function normalized(text)
  local first, last = 1, #text
  while first <= last and (string.byte(text, first) == 0x20 or string.byte(text, first) == 0x09) do
    first = first + 1
  end
  while last >= first and (string.byte(text, last) == 0x20 or string.byte(text, last) == 0x09) do
    last = last - 1
  end
  local bytes = {}
  for i = first, last do
    local b = string.byte(text, i)
    if b >= 0x61 and b <= 0x7a then
      b = b - 0x20
    end
    bytes[#bytes + 1] = string.char(b)
  end
  return table.concat(bytes)
end

-- The manufacturer and product name strings of the first SMBIOS type 1
-- structure, or nil when the table has none.
local function system_strings()
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
    if kind == 1 then
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

-- This machine's quirks, from the tables once in a kept script; nil when the
-- rows do not list it.
local function machine()
  if kept.machine ~= nil then
    return kept.machine or nil
  end
  local manufacturer, product = system_strings()
  if not manufacturer then
    ksi.trace('no SMBIOS system structure')
    return nil
  end
  manufacturer, product = normalized(manufacturer), normalized(product)
  local found = false
  for _, row in ipairs(MACHINES) do
    local named
    if row.exact then
      named = product == row[2]
    else
      named = string.find(product, row[2], 1, true) ~= nil
    end
    if string.find(manufacturer, row[1], 1, true) and named then
      found = { fans = not NO_FANS[product], fan_type = not NO_FAN_TYPE[product] }
      break
    end
  end
  if not ksi.volatile then
    kept.machine = found
  end
  return found or nil
end

-- One SMM command on processor 0: eax and edx of the answer, nil when the
-- firmware refused it. Two slow commands in a row mark the firmware slow.
local function smm(eax, ebx)
  local item = string.pack('<I2I2I4I8I8I8I8I8I8', 0, 0, SMI_PORT, eax, ebx, 0, 0, 0, 0)
  local started = ksi.time()
  local results, failed = ksi.hw.smi(item)
  if ksi.time() - started > SLOW_MS * 10000 then
    ksi.trace('command 0x%04x took more than %d ms', eax, SLOW_MS)
    kept.slow = kept.late
    kept.late = true
  else
    kept.late = nil
  end
  if not results then
    ksi.trace('command 0x%04x was not sent: 0x%08x', eax, failed)
    return nil
  end
  local status, _, rax, _, _, rdx, _, _, flags = string.unpack('<i4I4I8I8I8I8I8I8I8', results)
  local answer = rax & 0xffffffff
  if
    status ~= nt.STATUS_SUCCESS
    or flags & 1 ~= 0
    or answer & 0xffff == 0xffff
    or answer == eax
  then
    return nil
  end
  return answer, rdx & 0xffffffff
end

local function signature_present()
  for _, command in ipairs({ GET_SIGNATURE1, GET_SIGNATURE2 }) do
    local eax, edx = smm(command, 0)
    if eax == SIGNATURE_EAX and edx == SIGNATURE_EDX then
      return true
    end
    if kept.slow then
      return false
    end
  end
  return false
end

-- The machine's quirks and nil, or nil and the status the routine returns.
local function begin(input, length)
  if not ksi.hw.smi or not ksi.hw.firmware_table then
    ksi.trace('no SMI or firmware table functions')
    return nil, nt.STATUS_NOT_SUPPORTED
  end
  if #input ~= length then
    ksi.trace('input of %d bytes', #input)
    return nil, nt.STATUS_INVALID_PARAMETER
  end
  if kept.slow then
    ksi.trace('the firmware was slow')
    return nil, nt.STATUS_IO_TIMEOUT
  end
  local m = machine()
  if not m then
    ksi.trace('no row for this machine')
    return nil, nt.STATUS_NOT_SUPPORTED
  end
  return m
end

-- Returns the fan speed multiplier (4 bytes), each fan's type (4 bytes; FFh
-- for none), each temperature sensor's type (10 bytes; FFh for none) and two
-- zero bytes.
function identify(input)
  local m, status = begin(input, 0)
  if not m then
    return status
  end
  if not signature_present() then
    if kept.slow then
      return nt.STATUS_IO_TIMEOUT
    end
    ksi.trace('no Dell SMM signature')
    return nt.STATUS_NOT_FOUND
  end
  kept.signature = true
  local multiplier = FAN_MULTIPLIER
  local fans = {}
  for fan = 0, FANS_MAX - 1 do
    local kind = ABSENT
    if m.fans then
      local answer
      if m.fan_type then
        answer = smm(GET_FAN_TYPE, fan)
        if answer then
          kind = answer & 0xff
        end
      end
      if not answer then
        answer = smm(GET_SPEED, fan)
        if answer then
          kind = 0xfe
        end
      end
      if answer then
        for state = 0, FAN_STATES_MAX - 1 do
          local nominal = smm(GET_NOMINAL_SPEED, fan | (state << 8))
          if nominal and nominal & 0xffff > FAN_RPM_THRESHOLD then
            multiplier = 1
          end
        end
      end
    end
    if kept.slow then
      return nt.STATUS_IO_TIMEOUT
    end
    fans[#fans + 1] = string.char(kind)
  end
  local temps = {}
  for sensor = 0, TEMPS_MAX - 1 do
    local answer = smm(GET_TEMP_TYPE, sensor)
    if kept.slow then
      return nt.STATUS_IO_TIMEOUT
    end
    temps[#temps + 1] = string.char(answer and answer & 0xff or ABSENT)
  end
  local reply = string.pack('<I4', multiplier) .. table.concat(fans) .. table.concat(temps)
  return nt.STATUS_SUCCESS, reply .. '\0\0'
end

-- Reads the fans and temperature sensors whose bits the input sets (a fan
-- mask and a sensor mask, 4 bytes each): each fan's speed before the
-- multiplier (4 bytes) and each sensor's degrees C (4 bytes, signed), -1 for
-- one not asked for or without a reading.
function read(input)
  local m, status = begin(input, 8)
  if not m then
    return status
  end
  local fanMask, tempMask = string.unpack('<I4I4', input)
  if fanMask >> FANS_MAX ~= 0 or tempMask >> TEMPS_MAX ~= 0 or (fanMask ~= 0 and not m.fans) then
    ksi.trace('masks 0x%x 0x%x', fanMask, tempMask)
    return nt.STATUS_INVALID_PARAMETER
  end
  if not kept.signature then
    if not signature_present() then
      if kept.slow then
        return nt.STATUS_IO_TIMEOUT
      end
      ksi.trace('no Dell SMM signature')
      return nt.STATUS_NOT_FOUND
    end
    kept.signature = true
  end
  local output = {}
  for fan = 0, FANS_MAX - 1 do
    local value = -1
    if fanMask & (1 << fan) ~= 0 then
      local answer = smm(GET_SPEED, fan)
      if kept.slow then
        return nt.STATUS_IO_TIMEOUT
      end
      if answer then
        value = answer & 0xffff
      end
    end
    output[#output + 1] = string.pack('<i4', value)
  end
  for sensor = 0, TEMPS_MAX - 1 do
    local value = -1
    if tempMask & (1 << sensor) ~= 0 then
      local answer = smm(GET_TEMP, sensor)
      if kept.slow then
        return nt.STATUS_IO_TIMEOUT
      end
      -- 99h is a reading in flux; above TEMP_MAX none (C1h is a powered-off
      -- GPU).
      if answer and answer & 0xff ~= 0x99 and answer & 0xff <= TEMP_MAX then
        value = answer & 0xff
      end
    end
    output[#output + 1] = string.pack('<i4', value)
  end
  return nt.STATUS_SUCCESS, table.concat(output)
end
