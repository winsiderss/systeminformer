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
-- Reads coolers and power supplies through their USB HID collections, opened by
-- device name; a collection the script does not list is not opened.
--

local nt = require('nt')

local DRIVER = '\\Driver\\HidUsb'
local READ_LENGTH = 1024
local READS_MAX = 3
local MILLISECONDS = 2500
local REQUEST_MILLISECONDS = 500
local DEVICES_MAX = 8
local VALUES_MAX = 24
local NAME_MAX = 512

local KIND_TEMPERATURE = 1 -- millidegrees C
local KIND_FAN = 2 -- RPM
local KIND_FLOW = 3 -- dL/h
local KIND_VOLTAGE = 4 -- mV
local KIND_CURRENT = 5 -- mA
local KIND_POWER = 6 -- mW

local function be16(report, offset)
  return (string.unpack('>I2', report, offset + 1))
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

-- Rows by vendor << 16 | product: the status report's ID, its minimum length
-- and the decoder.
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

-- The vendor and product of a device's only or first collection
-- (HID\VID_xxxx&PID_xxxx), or nil.
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

-- The collection opened by its device name with Access, or nil and a status.
local function open_collection(device, access)
  local name = text_of((ksi.hw.device_property(device, nt.DevicePropertyPhysicalDeviceObjectName)))
  if not name or #name == 0 then
    return nil, nt.STATUS_OBJECT_NAME_NOT_FOUND
  end
  local opened, failed = ksi.hw.device_open(name, access)
  if not opened then
    ksi.trace('the collection did not open: 0x%x', failed)
  end
  return opened, failed
end

-- The next status report of a passive row, decoded into values; a status.
local function passive(opened, row, values)
  local report, status
  for _ = 1, READS_MAX do
    report, status = ksi.hw.device_read(opened, READ_LENGTH, MILLISECONDS)
    if not report or string.byte(report, 1) == row.report then
      break
    end
    report, status = nil, nt.STATUS_DEVICE_PROTOCOL_ERROR
  end
  if not report then
    return status
  end
  if #report < row.minimum then
    ksi.trace('a report of %d bytes', #report)
    return nt.STATUS_DEVICE_PROTOCOL_ERROR
  end
  row.decode(values, report)
  return nt.STATUS_SUCCESS
end

-- One output report (report ID 00h, request padded to Size); the reply after
-- its report ID, or nil and a status.
local function exchange(opened, request, size)
  local written, failed = ksi.hw.device_write(
    opened,
    '\0' .. request .. string.rep('\0', size - #request),
    REQUEST_MILLISECONDS
  )
  if not written then
    return nil, failed
  end
  local reply, status = ksi.hw.device_read(opened, READ_LENGTH, REQUEST_MILLISECONDS)
  if not reply then
    return nil, status
  end
  if string.byte(reply, 1) ~= 0 then
    return nil, nt.STATUS_DEVICE_PROTOCOL_ERROR
  end
  return string.sub(reply, 2)
end

-- One controller command; its 16-byte reply, nil and a status, or nil alone for
-- a sensor not connected.
local function commander_command(opened, command, channel)
  local reply, failed = exchange(opened, string.char(command, channel), 63)
  if not reply then
    return nil, failed
  end
  if #reply < 16 then
    return nil, nt.STATUS_DEVICE_PROTOCOL_ERROR
  end
  local code = string.byte(reply, 1)
  if code == 0x11 then
    return nil
  end
  if code ~= 0 then
    ksi.trace('command 0x%x: reply 0x%x', command, code)
    return nil, nt.STATUS_IO_DEVICE_ERROR
  end
  return reply
end

-- The controller's connected temperatures and fans and its three rails; a
-- status.
local function commander(opened, values)
  local temperatures, failed = commander_command(opened, 0x10, 0)
  if not temperatures then
    return failed or nt.STATUS_IO_DEVICE_ERROR
  end
  local fans, fansFailed = commander_command(opened, 0x20, 0)
  if not fans then
    return fansFailed or nt.STATUS_IO_DEVICE_ERROR
  end
  for channel = 0, 3 do
    if string.byte(temperatures, channel + 2) ~= 0 then
      local reply, status = commander_command(opened, 0x11, channel)
      if status then
        return status
      end
      if reply then
        values[#values + 1] = { KIND_TEMPERATURE, channel, string.unpack('>i2', reply, 2) * 10 }
      end
    end
  end
  for channel = 0, 5 do
    if string.byte(fans, channel + 2) ~= 0 then
      local reply, status = commander_command(opened, 0x21, channel)
      if not reply then
        return status or nt.STATUS_IO_DEVICE_ERROR
      end
      values[#values + 1] = { KIND_FAN, channel, (string.unpack('>I2', reply, 2)) }
    end
  end
  for rail = 0, 2 do
    local reply, status = commander_command(opened, 0x12, rail)
    if not reply then
      return status or nt.STATUS_IO_DEVICE_ERROR
    end
    values[#values + 1] = { KIND_VOLTAGE, rail, (string.unpack('>I2', reply, 2)) }
  end
  return nt.STATUS_SUCCESS
end

-- A LINEAR11 value (5-bit exponent, 11-bit mantissa, both signed) times Scale.
local function linear11(raw, scale)
  local exponent = raw >> 11
  local mantissa = raw & 0x7ff
  if exponent >= 16 then
    exponent = exponent - 32
  end
  if mantissa >= 0x400 then
    mantissa = mantissa - 0x800
  end
  if exponent >= 0 then
    return (mantissa * scale) << exponent
  end
  return (mantissa * scale) // (1 << -exponent)
end

-- A power supply command ([length][command][parameter]); its 16-bit value, nil
-- if unanswered, or nil and a status.
local function psu_command(opened, length, command, parameter)
  local reply, failed = exchange(opened, string.char(length, command, parameter), 64)
  if not reply then
    return nil, failed
  end
  if #reply < 4 then
    return nil, nt.STATUS_DEVICE_PROTOCOL_ERROR
  end
  if string.byte(reply, 1) ~= length or string.byte(reply, 2) ~= command then
    return nil
  end
  return (string.unpack('<I2', reply, 3))
end

-- A power supply's values; the rail selection is put back on 12 V however the
-- reads end. A status.
local function psu(opened, values)
  local function read_value(kind, index, command, scale)
    local raw, failed = psu_command(opened, 3, command, 0)
    if failed then
      return failed
    end
    if raw then
      values[#values + 1] = { kind, index, linear11(raw, scale) }
    end
  end
  local _, failed = psu_command(opened, 0xfe, 0x03, 0)
  if failed then
    return failed
  end
  local status = read_value(KIND_TEMPERATURE, 0, 0x8d, 1000)
    or read_value(KIND_TEMPERATURE, 1, 0x8e, 1000)
    or read_value(KIND_FAN, 0, 0x90, 1)
    or read_value(KIND_VOLTAGE, 0, 0x88, 1000)
    or read_value(KIND_POWER, 0, 0xee, 1000)
  if status then
    return status
  end
  local selected <close> = setmetatable({}, {
    __close = function(self)
      if self.sent then
        psu_command(opened, 2, 0x00, 0)
      end
    end,
  })
  for rail = 0, 2 do
    selected.sent = true
    local _, selectFailed = psu_command(opened, 2, 0x00, rail)
    if selectFailed then
      return selectFailed
    end
    status = read_value(KIND_VOLTAGE, 1 + rail, 0x8b, 1000)
      or read_value(KIND_CURRENT, rail, 0x8c, 1000)
      or read_value(KIND_POWER, 1 + rail, 0x96, 1000)
    if status then
      return status
    end
  end
  return nt.STATUS_SUCCESS
end

-- The request/response rows by vendor << 16 | product.
local ACTIVE = {
  [0x1b1c0c10] = commander,
  [0x1b1c1d00] = commander,
}
for _, product in ipairs({
  0x1c03,
  0x1c04,
  0x1c05,
  0x1c06,
  0x1c07,
  0x1c08,
  0x1c09,
  0x1c0a,
  0x1c0b,
  0x1c0c,
  0x1c0d,
  0x1c1e,
  0x1c1f,
  0x1c23,
  0x1c27,
}) do
  ACTIVE[0x1b1c0000 | product] = psu
end

-- Each listed collection opened with Access and sampled; the records.
-- Together opens every listed collection before the first sample, so their
-- pushed reports are waited for at once.
local function collect(rows, access, sample, together)
  local output = {}
  local found = {}
  local devices = ksi.hw.devices(DRIVER)
  for _, device in ipairs(devices or {}) do
    local vendor, product = identify(device)
    local row = vendor and rows[(vendor << 16) | product]
    if row and #found < DEVICES_MAX then
      local unit = { device = device, vendor = vendor, product = product, row = row }
      if together then
        unit.opened, unit.status = open_collection(device, access)
      end
      found[#found + 1] = unit
    else
      device:release()
    end
  end
  for _, unit in ipairs(found) do
    local values = {}
    local opened, status = unit.opened, unit.status
    if not together then
      opened, status = open_collection(unit.device, access)
    end
    if opened then
      status = sample(opened, unit.row, values)
      opened:release()
    end
    if not nt.NT_SUCCESS(status) then
      values = {}
    end
    local entries = {}
    for i = 1, math.min(#values, VALUES_MAX) do
      entries[i] = string.pack('<BBxxi4', values[i][1], values[i][2], values[i][3])
    end
    output[#output + 1] = string.pack('<i4I2I2Bxxx', status, unit.vendor, unit.product, #entries)
      .. table.concat(entries)
      .. string.rep('\0', (VALUES_MAX - #entries) * 8)
    unit.device:release()
  end
  return table.concat(output)
end

local function checked(input)
  if not ksi.hw.devices or not ksi.hw.device_open or not ksi.hw.device_read then
    ksi.trace('no device functions')
    return nt.STATUS_NOT_SUPPORTED
  end
  if #input ~= 0 then
    return nt.STATUS_INVALID_PARAMETER
  end
  return nt.STATUS_SUCCESS
end

-- Reads the listed push-status collections; no input. Returns a status and per
-- device: status, vendor and product (2 bytes each), value count (1, 3
-- reserved), then 24 values of kind (KIND_*), index (1 each, 2 reserved) and
-- value (4, signed).
function read(input)
  local status = checked(input)
  if not nt.NT_SUCCESS(status) then
    return status
  end
  return nt.STATUS_SUCCESS, collect(ROWS, nt.FILE_READ_DATA, passive, true)
end

-- Reads the listed request/response collections, as read; the caller holds the
-- devices' tool mutex.
function corsair(input)
  local status = checked(input)
  if not nt.NT_SUCCESS(status) or not ksi.hw.device_write then
    return nt.NT_SUCCESS(status) and nt.STATUS_NOT_SUPPORTED or status
  end
  return nt.STATUS_SUCCESS,
    collect(ACTIVE, nt.FILE_READ_DATA | nt.FILE_WRITE_DATA, function(opened, sequence, values)
      return sequence(opened, values)
    end)
end
