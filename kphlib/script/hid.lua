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
-- Reads coolers that push their status as HID input reports: each listed
-- collection of the USB HID driver is opened by its device name, and one read
-- returns the next status report, which is decoded by the device's row. The
-- read waits at most a little over two report intervals. A collection the
-- script does not list is not opened.
--

local nt = require('nt')

local DRIVER = '\\Driver\\HidUsb'
local READ_LENGTH = 1024
local READS_MAX = 3
local MILLISECONDS = 2500
local DEVICES_MAX = 8
local VALUES_MAX = 24
local NAME_MAX = 512

local KIND_TEMPERATURE = 1 -- millidegrees C
local KIND_FAN = 2 -- RPM
local KIND_FLOW = 3 -- dL/h

local function be16(report, offset)
  return string.unpack('>I2', report, offset + 1)
end

-- Temperatures as big-endian centidegrees, 7FFFh not connected.
local function centidegrees(values, report, first, count)
  for i = 0, count - 1 do
    local raw = be16(report, first + i * 2)
    if raw ~= 0x7fff then
      values[#values + 1] = { KIND_TEMPERATURE, i, ((raw ~ 0x8000) - 0x8000) * 10 }
    end
  end
end

local function speeds(values, kind, report, offsets, at)
  for i, offset in ipairs(offsets) do
    values[#values + 1] = { kind, i - 1, be16(report, offset + at) }
  end
end

-- The rows by vendor << 16 | product: the status report's ID, its minimum
-- length and the decoder.
local ROWS = {
  [0x1e71170e] = {
    report = 0x04,
    minimum = 7,
    decode = function(values, report)
      values[#values + 1] = {
        KIND_TEMPERATURE,
        0,
        string.byte(report, 2) * 1000 + string.byte(report, 3) * 100,
      }
      speeds(values, KIND_FAN, report, { 3, 5 }, 0)
    end,
  },
}

-- An Aquacomputer row: temperatures, fan blocks (speed at block + at), flows.
local function aquacomputer(product, minimum, first, count, fans, at, flows)
  ROWS[0x0c700000 | product] = {
    report = 0x01,
    minimum = minimum,
    decode = function(values, report)
      centidegrees(values, report, first, count)
      speeds(values, KIND_FAN, report, fans, at)
      speeds(values, KIND_FLOW, report, flows, 0)
    end,
  }
end

aquacomputer(0xf00e, 0x76, 0x57, 1, { 0x6c, 0x5f }, 8, {})
aquacomputer(0xf00a, 0x37, 0x2f, 4, {}, 8, {})
aquacomputer(0xf010, 0x5a, 0x32, 4, {}, 8, {})
aquacomputer(0xf011, 0xe2, 0x3d, 4, { 0x7d, 0x8a, 0x97, 0xa4, 0xb1, 0xbe, 0xcb, 0xd8 }, 8, { 0x7b })
aquacomputer(0xf00d, 0xa1, 0x34, 4, { 0x70, 0x7d, 0x8a, 0x97 }, 8, { 0x6e })
aquacomputer(0xf012, 0x65, 0x55, 2, {}, 8, { 0x51 })
aquacomputer(0xf00b, 0x59, 0x2d, 2, { 0x41, 0x4b }, 6, { 0x37 })

local function hex4(text, at)
  local n = tonumber(string.sub(text, at, at + 3), 16)
  if not n or #string.sub(text, at, at + 3) ~= 4 then
    return nil
  end
  return n
end

-- The first string of a UTF-16 property, as text; nil past NAME_MAX bytes.
local function text_of(bytes)
  if not bytes or #bytes > NAME_MAX then
    return nil
  end
  local chars = {}
  for i = 1, #bytes - 1, 2 do
    local low, high = string.byte(bytes, i, i + 1)
    if low == 0 and high == 0 then
      break
    end
    if high ~= 0 or low < 0x20 or low > 0x7e then
      return nil
    end
    chars[#chars + 1] = string.char(low)
  end
  return table.concat(chars)
end

-- The vendor and product of a listed collection, or nil: its first hardware
-- ID is HID\VID_xxxx&PID_xxxx, and it is the device's only or first
-- collection.
local function identify(device)
  local id = text_of((ksi.hw.device_property_data(device, nt.DEVPKEY_Device_HardwareIds)))
  if not id or string.sub(id, 1, 8) ~= 'HID\\VID_' or string.sub(id, 13, 17) ~= '&PID_' then
    return nil
  end
  local vendor, product = hex4(id, 9), hex4(id, 18)
  if not vendor or not product then
    return nil
  end
  local upper = string.upper(id)
  local at = string.find(upper, '&COL', 1, true)
  if at and string.sub(upper, at, at + 5) ~= '&COL01' then
    return nil
  end
  return vendor, product
end

-- The next status report of a collection, or nil and a status.
local function status_report(device, row)
  local name = text_of((ksi.hw.device_property(device, nt.DevicePropertyPhysicalDeviceObjectName)))
  if not name or #name == 0 then
    return nil, nt.STATUS_OBJECT_NAME_NOT_FOUND
  end
  local opened, failed = ksi.hw.device_open(name, nt.FILE_READ_DATA)
  if not opened then
    ksi.trace('the collection did not open: 0x%x', failed)
    return nil, failed
  end
  local report, status
  for _ = 1, READS_MAX do
    report, status = ksi.hw.device_read(opened, READ_LENGTH, MILLISECONDS)
    if not report or string.byte(report, 1) == row.report then
      break
    end
    report, status = nil, nt.STATUS_DEVICE_PROTOCOL_ERROR
  end
  opened:release()
  if not report then
    return nil, status
  end
  if #report < row.minimum then
    ksi.trace('a report of %d bytes', #report)
    return nil, nt.STATUS_DEVICE_PROTOCOL_ERROR
  end
  return report
end

-- Reads the listed collections; no input. Returns a status and per device: a
-- status, the vendor and product (2 bytes each), the number of values (1
-- byte, then 3 reserved), then 24 values of 8 bytes: kind (1 temperature in
-- millidegrees C, 2 fan RPM, 3 flow in dL/h), index (1 byte each, then 2
-- reserved) and value (4 bytes, signed).
function read(input)
  if not ksi.hw.devices or not ksi.hw.device_open or not ksi.hw.device_read then
    ksi.trace('no device functions')
    return nt.STATUS_NOT_SUPPORTED
  end
  if #input ~= 0 then
    return nt.STATUS_INVALID_PARAMETER
  end
  local output = {}
  local devices = ksi.hw.devices(DRIVER)
  for _, device in ipairs(devices or {}) do
    local vendor, product = identify(device)
    local row = vendor and ROWS[(vendor << 16) | product]
    if row and #output < DEVICES_MAX then
      local values = {}
      local report, failed = status_report(device, row)
      if report then
        row.decode(values, report)
      end
      local entries = {}
      for i = 1, math.min(#values, VALUES_MAX) do
        entries[i] = string.pack('<BBxxi4', values[i][1], values[i][2], values[i][3])
      end
      output[#output + 1] = string.pack(
        '<i4I2I2Bxxx',
        report and nt.STATUS_SUCCESS or failed,
        vendor,
        product,
        #entries
      ) .. table.concat(entries) .. string.rep('\0', (VALUES_MAX - #entries) * 8)
    end
    device:release()
  end
  return nt.STATUS_SUCCESS, table.concat(output)
end
