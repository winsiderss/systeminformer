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
-- Reads the voltage settings of the Intel overclocking mailbox (MSR 150h) on
-- processor 0:0 of the client parts below: for each plane, the read command
-- (10h, the plane in [47:40], run/busy [63]) after the mailbox is idle, then
-- the reply once it is idle again. No other value is ever written.
--

local nt = require('nt')

local MSR_OC_MAILBOX = 0x150
local RUN_BUSY = 1 << 63
local COMMAND_READ = 0x10
local PLANES = 5
local POLLS = 100
local POLL_US = 10

local MODELS = {
  [0x3c] = true,
  [0x3d] = true,
  [0x45] = true,
  [0x46] = true,
  [0x47] = true,
  [0x4e] = true,
  [0x5e] = true,
  [0x7d] = true,
  [0x7e] = true,
  [0x8c] = true,
  [0x8d] = true,
  [0x8e] = true,
  [0x97] = true,
  [0x9a] = true,
  [0x9e] = true,
  [0xa5] = true,
  [0xa6] = true,
  [0xa7] = true,
  [0xb7] = true,
  [0xba] = true,
  [0xbf] = true,
}

local kept = {}

-- True when processor 0:0 is an Intel family 6 model listed above; nil and a
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
  local model = ((signature >> 4) & 0xf) | (((signature >> 16) & 0xf) << 4)
  local listed = string.pack('<I4I4I4', b, d, c) == 'GenuineIntel'
    and family == 6
    and MODELS[model] == true
  if not listed then
    ksi.trace('family 0x%x model 0x%x not listed', family, model)
  end
  kept.cpu = listed
  return listed
end

local function read_mailbox()
  local results, failed = ksi.hw.msr_read(string.pack('<I2I2I4', 0, 0, MSR_OC_MAILBOX))
  if not results then
    return nil, failed
  end
  local status, _, value = string.unpack('<i4I4I8', results)
  if not nt.NT_SUCCESS(status) then
    return nil, status
  end
  return value
end

-- The mailbox polled until run/busy is clear: its value, or nil and a status.
local function idle()
  for _ = 1, POLLS do
    local value, failed = read_mailbox()
    if not value then
      return nil, failed
    end
    if value & RUN_BUSY == 0 then
      return value
    end
    ksi.stall(POLL_US)
  end
  return nil, nt.STATUS_IO_TIMEOUT
end

-- One plane's read: a status, the completion code and the data.
local function read_plane(plane)
  local value, failed = idle()
  if not value then
    ksi.trace('plane %d: the mailbox is not idle: 0x%x', plane, failed)
    return failed == nt.STATUS_IO_TIMEOUT and nt.STATUS_DEVICE_BUSY or failed, 0, 0
  end
  local command = RUN_BUSY | (plane << 40) | (COMMAND_READ << 32)
  local results
  results, failed = ksi.hw.msr_write(string.pack('<I2I2I4I8', 0, 0, MSR_OC_MAILBOX, command))
  if not results then
    return failed, 0, 0
  end
  local status = string.unpack('<i4', results)
  if not nt.NT_SUCCESS(status) then
    ksi.trace('plane %d: the command was not written: 0x%x', plane, status)
    return status, 0, 0
  end
  value, failed = idle()
  if not value then
    ksi.trace('plane %d: no reply: 0x%x', plane, failed)
    return failed, 0, 0
  end
  local code = (value >> 32) & 0xff
  if code ~= 0 then
    ksi.trace('plane %d: completion code 0x%x', plane, code)
    return nt.STATUS_UNSUCCESSFUL, code, 0
  end
  return nt.STATUS_SUCCESS, 0, value & 0xffffffff
end

-- Returns for each plane (core, GPU, cache, planes 3 and 4) a status, the
-- completion code and the data of the read command (4 bytes each).
function read(input)
  if #input ~= 0 then
    ksi.trace('input of %d bytes', #input)
    return nt.STATUS_INVALID_PARAMETER
  end
  if not ksi.hw.cpuid or not ksi.hw.msr_read or not ksi.hw.msr_write then
    ksi.trace('no CPUID or MSR functions')
    return nt.STATUS_NOT_SUPPORTED
  end
  local listed, failed = cpu_listed()
  if listed == nil then
    return failed
  end
  if not listed then
    return nt.STATUS_NOT_SUPPORTED
  end
  local lock <close>, lockFailed = ksi.lock('mailbox')
  if not lock then
    ksi.trace('the mailbox lock was not taken: 0x%x', lockFailed)
    return lockFailed
  end
  local output = {}
  for plane = 0, PLANES - 1 do
    output[#output + 1] = string.pack('<i4I4I4', read_plane(plane))
  end
  return nt.STATUS_SUCCESS, table.concat(output)
end
