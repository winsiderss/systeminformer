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
-- Reads the SMART data of SATA drives behind USB bridges known to translate
-- ATA PASS-THROUGH, through the SCSI pass-through of each disk the USB storage
-- drivers enumerate. The bridge is named by the disk's parent USB device; a
-- disk behind any other bridge is sent nothing. CHECK POWER MODE runs first
-- and a drive in standby is not woken.
--

local nt = require('nt')

local DRIVERS = { '\\Driver\\USBSTOR', '\\Driver\\UASPStor' }
local PASS_THROUGH = 0x4d004
local HEADER = 56
local SENSE = 32
local SENSE_AT = HEADER
local DATA_AT = HEADER + SENSE
local SECTOR = 512
local ID_MAX = 400
local DISKS_MAX = 16
local MILLISECONDS = 10000
local SECONDS = 5

local DATA_IN = 1
local DATA_NONE = 2

-- The bridges by vendor << 16 | product; a vendor's entry with an except list
-- takes every product but those.
local SAT = {
  [0x152d0561] = true,
  [0x152d0565] = true,
  [0x152d0567] = true,
  [0x152d0576] = true,
  [0x152d0578] = true,
  [0x152d0579] = true,
  [0x152d0580] = true,
  [0x152d1337] = true,
  [0x152d1561] = true,
  [0x152d1576] = true,
  [0x152d2351] = true,
  [0x152d2567] = true,
  [0x152d2590] = true,
  [0x152d3562] = true,
  [0x152d3569] = true,
  [0x152d578e] = true,
  [0x152d8561] = true,
  [0x152d9561] = true,
  [0x152da561] = true,
  [0x152da578] = true,
  [0x152da580] = true,
  [0x152db567] = true,
  [0x0bda9201] = true,
  [0x25371066] = true,
  [0x25371068] = true,
  [0x21090711] = true,
  [0x21090715] = true,
  [0x21090716] = true,
  [0x21090717] = true,
  [0x05e30731] = true,
  [0x05e30735] = true,
  [0x05e30761] = true,
  [0x05e32013] = true,
  [0x1f750611] = true,
  [0x1f750621] = true,
  [0x7825a2a4] = true,
}
local SAT_VENDORS = {
  [0x174c] = { [0x2362] = true, [0x2364] = true },
  [0x13fd] = { [0x0540] = true, [0x1040] = true, [0x1150] = true },
}

local function hex4(text, at)
  local n = 0
  for i = at, at + 3 do
    local c = string.byte(text, i)
    local d
    if c and c >= 0x30 and c <= 0x39 then
      d = c - 0x30
    elseif c and c >= 0x41 and c <= 0x46 then
      d = c - 0x37
    elseif c and c >= 0x61 and c <= 0x66 then
      d = c - 0x57
    else
      return nil
    end
    n = n * 16 + d
  end
  return n
end

-- The parent's instance ID (UTF-16) and its vendor and product, read from a
-- USB\VID_xxxx&PID_xxxx prefix; nil and a status when there is none.
local function parent(device)
  local bytes, kind = ksi.hw.device_property_data(device, nt.DEVPKEY_Device_Parent)
  if not bytes then
    return nil, kind
  end
  if kind ~= nt.DEVPROP_TYPE_STRING or #bytes > ID_MAX + 2 or #bytes % 2 ~= 0 then
    return nil, nt.STATUS_DEVICE_CONFIGURATION_ERROR
  end
  local chars = {}
  for i = 1, #bytes - 1, 2 do
    local low, high = string.byte(bytes, i, i + 1)
    if low == 0 and high == 0 then
      break
    end
    chars[#chars + 1] = high == 0 and string.char(low) or '?'
  end
  local text = table.concat(chars)
  if string.sub(text, 1, 8) ~= 'USB\\VID_' or string.sub(text, 13, 17) ~= '&PID_' then
    ksi.trace('the parent is not a USB device')
    return nil, nt.STATUS_NOT_SUPPORTED
  end
  local vendor, product = hex4(text, 9), hex4(text, 18)
  if not vendor or not product then
    return nil, nt.STATUS_NOT_SUPPORTED
  end
  return string.sub(bytes, 1, #text * 2), vendor, product
end

local function translates(vendor, product)
  if SAT[(vendor << 16) | product] then
    return true
  end
  local except = SAT_VENDORS[vendor]
  return except ~= nil and not except[product]
end

-- One ATA command in an ATA PASS-THROUGH (12) CDB: the SCSI status, the sense
-- and the data; nil and a status when the request fails.
local function ata(device, protocol, flags, features, count, mid, high, command, dataIn, length)
  local cdb = string.pack(
    'BBBBBBBBBBBB',
    0xa1,
    protocol << 1,
    flags,
    features,
    count,
    0,
    mid,
    high,
    0,
    command,
    0,
    0
  ) .. string.rep('\0', 4)
  local header = string.pack(
    '<I2BBBBBBBxxxI4I4xxxxI8I4c16xxxx',
    HEADER,
    0,
    0,
    0,
    0,
    12,
    SENSE,
    dataIn,
    length,
    SECONDS,
    length > 0 and DATA_AT or 0,
    SENSE_AT,
    cdb
  )
  local input = header .. string.rep('\0', SENSE + length)
  local output, status = ksi.hw.device_control(device, PASS_THROUGH, input, #input, MILLISECONDS)
  if not output then
    ksi.trace('pass-through 0x%x failed: 0x%x', command, status)
    return nil, status
  end
  if #output < HEADER then
    return nil, nt.STATUS_DEVICE_PROTOCOL_ERROR
  end
  local _, scsiStatus, _, _, _, _, senseLength, _, transferred =
    string.unpack('<I2BBBBBBBxxxI4', output)
  return scsiStatus,
    string.sub(output, SENSE_AT + 1, SENSE_AT + math.min(senseLength, SENSE)),
    string.sub(output, DATA_AT + 1, DATA_AT + math.min(transferred, length))
end

-- The ATA status and count registers the bridge returned in the sense data,
-- descriptor or fixed format; nil when there are none.
local function registers(sense)
  local code = string.byte(sense, 1)
  if not code then
    return nil
  end
  code = code & 0x7f
  if code == 0x72 or code == 0x73 then
    local at = 9
    while at + 1 <= #sense do
      local kind, length = string.byte(sense, at, at + 1)
      if kind == 0x09 and length >= 0x0c and at + 13 <= #sense then
        return string.byte(sense, at + 13), string.byte(sense, at + 5)
      end
      at = at + 2 + length
    end
    return nil
  end
  if (code == 0x70 or code == 0x71) and #sense >= 14 then
    local asc, ascq = string.byte(sense, 13, 14)
    if asc == 0x00 and ascq == 0x1d then
      return string.byte(sense, 5), string.byte(sense, 7)
    end
  end
  return nil
end

-- STATUS_SUCCESS and the count register (00h is standby) when the drive
-- answered CHECK POWER MODE, a status otherwise.
local function power_mode(device)
  local scsiStatus, sense = ata(device, 3, 0x20, 0, 0, 0, 0, 0xe5, DATA_NONE, 0)
  if not scsiStatus then
    return sense
  end
  local ataStatus, count = registers(sense)
  if not ataStatus then
    ksi.trace('no ATA registers returned (SCSI status 0x%x)', scsiStatus)
    return nt.STATUS_DEVICE_PROTOCOL_ERROR
  end
  if ataStatus & 0x01 ~= 0 then
    return nt.STATUS_IO_DEVICE_ERROR
  end
  return nt.STATUS_SUCCESS, count
end

-- SMART READ DATA: 512 bytes, or nil and a status.
local function smart(device)
  local scsiStatus, sense, data = ata(device, 4, 0x0e, 0xd0, 1, 0x4f, 0xc2, 0xb0, DATA_IN, SECTOR)
  if not scsiStatus then
    return nil, sense
  end
  if scsiStatus ~= 0 or #data ~= SECTOR then
    ksi.trace('SMART READ DATA: SCSI status 0x%x, %d bytes', scsiStatus, #data)
    return nil, nt.STATUS_IO_DEVICE_ERROR
  end
  return data
end

local function disk(device)
  local id, vendor, product = parent(device)
  if not id then
    local failed = vendor
    return string.pack('<i4I2I2Bxxx', failed, 0, 0, 0), '', ''
  end
  if not translates(vendor, product) then
    ksi.trace('%04x:%04x is not a listed bridge', vendor, product)
    return string.pack('<i4I2I2Bxxx', nt.STATUS_NOT_SUPPORTED, vendor, product, 0), id, ''
  end
  local status, count = power_mode(device)
  if not nt.NT_SUCCESS(status) then
    return string.pack('<i4I2I2Bxxx', status, vendor, product, 0), id, ''
  end
  if count == 0 then
    return string.pack('<i4I2I2Bxxx', nt.STATUS_DEVICE_POWERED_OFF, vendor, product, 0), id, ''
  end
  local data, failed = smart(device)
  if not data then
    return string.pack('<i4I2I2Bxxx', failed, vendor, product, count), id, ''
  end
  return string.pack('<i4I2I2Bxxx', nt.STATUS_SUCCESS, vendor, product, count), id, data
end

-- Reads every disk of the USB storage drivers; no input. Returns a status and
-- per disk: a status, the bridge's vendor and product (2 bytes each), the
-- power mode's count register (1 byte, then 3 reserved), the parent's
-- instance ID (UTF-16, 400 bytes, zero-padded) and the SMART data (512 bytes,
-- zero unless the status is STATUS_SUCCESS). STATUS_NOT_SUPPORTED is a bridge
-- not listed and STATUS_DEVICE_POWERED_OFF a drive in standby.
function read(input)
  if not ksi.hw.devices or not ksi.hw.device_control or not ksi.hw.device_property_data then
    ksi.trace('no device functions')
    return nt.STATUS_NOT_SUPPORTED
  end
  if #input ~= 0 then
    return nt.STATUS_INVALID_PARAMETER
  end
  local output = {}
  for _, name in ipairs(DRIVERS) do
    local devices = ksi.hw.devices(name)
    for _, device in ipairs(devices or {}) do
      if #output < DISKS_MAX then
        local head, id, data = disk(device)
        output[#output + 1] = head
          .. id
          .. string.rep('\0', ID_MAX - #id)
          .. data
          .. string.rep('\0', SECTOR - #data)
      end
      device:release()
    end
  end
  return nt.STATUS_SUCCESS, table.concat(output)
end
