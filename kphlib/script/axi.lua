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
-- Reads power supply registers through the USB serial dongle, opened by device
-- name; the output page is put back however the reads end.
--

local nt = require('nt')

local DRIVERS = { '\\Driver\\USBHUB3', '\\Driver\\usbhub' }
local DONGLES = { [0x1b1c1c00] = true }
local DEVICES_MAX = 4
local VALUES_MAX = 24
local NAME_MAX = 512
local MILLISECONDS = 1000
local READS_MAX = 4

local SET_BAUD_RATE = 0x1b0004
local SET_LINE_CONTROL = 0x1b000c
local SET_TIMEOUTS = 0x1b001c
local PURGE = 0x1b004c
local SET_HANDFLOW = 0x1b0064

local KIND_TEMPERATURE = 1
local KIND_FAN = 2
local KIND_VOLTAGE = 4
local KIND_CURRENT = 5
local KIND_POWER = 6

local PAGE = 0x00
local OUTPUTS = 3

-- The symbol of each nibble; a frame header is its command's symbol shifted
-- left once, low two bits clear.
local SYMBOLS =
  { 0x55, 0x56, 0x59, 0x5a, 0x65, 0x66, 0x69, 0x6a, 0x95, 0x96, 0x99, 0x9a, 0xa5, 0xa6, 0xa9, 0xaa }
local NIBBLES = {}
for nibble, symbol in ipairs(SYMBOLS) do
  NIBBLES[symbol] = nibble - 1
end
local REQUEST = SYMBOLS[1] & 0xfc
local REPLY = SYMBOLS[(7 << 1) + 1] & 0xfc

local function hex4(text, at)
  local part = string.sub(text, at, at + 3)
  local n = tonumber(part, 16)
  if not n or #part ~= 4 then
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

-- The vendor and product of a listed dongle (its first hardware ID), or nil.
local function listed(device)
  local id = text_of((ksi.hw.device_property_data(device, nt.DEVPKEY_Device_HardwareIds)))
  if not id or string.sub(id, 1, 8) ~= 'USB\\VID_' or string.sub(id, 13, 17) ~= '&PID_' then
    return nil
  end
  local vendor, product = hex4(id, 9), hex4(id, 18)
  if not vendor or not product or not DONGLES[(vendor << 16) | product] then
    return nil
  end
  return vendor, product
end

local function control(opened, code, input)
  local output, status = ksi.hw.device_control(opened, code, input, 0, MILLISECONDS)
  if not output then
    ksi.trace('serial request 0x%x failed: 0x%x', code, status)
    return status
  end
  return nt.STATUS_SUCCESS
end

-- 115200 8N1, no flow control, MILLISECONDS read timeout, both buffers purged;
-- the first failure.
local function setup(opened)
  local requests = {
    { SET_BAUD_RATE, string.pack('<I4', 115200) },
    { SET_LINE_CONTROL, string.pack('BBB', 0, 0, 8) },
    { SET_HANDFLOW, string.rep('\0', 16) },
    { SET_TIMEOUTS, string.pack('<I4I4I4I4I4', 0, 0, MILLISECONDS, 0, MILLISECONDS) },
    { PURGE, string.pack('<I4', 0x0c) },
  }
  for _, request in ipairs(requests) do
    local status = control(opened, request[1], request[2])
    if status ~= nt.STATUS_SUCCESS then
      return status
    end
  end
  return nt.STATUS_SUCCESS
end

-- One framed command; its decoded Length-byte reply, or nil and a status.
local function command(opened, payload, length)
  local frame = { string.char(REQUEST) }
  for i = 1, #payload do
    local byte = string.byte(payload, i)
    frame[#frame + 1] = string.char(SYMBOLS[(byte & 0xf) + 1], SYMBOLS[(byte >> 4) + 1])
  end
  frame[#frame + 1] = '\0'
  local written, failed = ksi.hw.device_write(opened, table.concat(frame), MILLISECONDS)
  if not written then
    return nil, failed
  end
  local wanted = 1 + length * 2
  local received = ''
  for _ = 1, READS_MAX do
    local bytes, status = ksi.hw.device_read(opened, wanted - #received, MILLISECONDS)
    if not bytes then
      return nil, status
    end
    if #bytes == 0 then
      break
    end
    received = received .. bytes
    -- the previous reply's optional terminator
    while string.byte(received, 1) == 0 do
      received = string.sub(received, 2)
    end
    if #received >= wanted then
      break
    end
  end
  if #received < wanted then
    ksi.trace('a reply of %d symbols, %d wanted', #received, wanted)
    return nil, nt.STATUS_IO_TIMEOUT
  end
  if string.byte(received, 1) ~= REPLY then
    ksi.trace('a reply of header 0x%x', string.byte(received, 1))
    return nil, nt.STATUS_DEVICE_PROTOCOL_ERROR
  end
  local data = {}
  for i = 2, wanted - 1, 2 do
    local low, high = NIBBLES[string.byte(received, i)], NIBBLES[string.byte(received, i + 1)]
    if not low or not high then
      return nil, nt.STATUS_DEVICE_PROTOCOL_ERROR
    end
    data[#data + 1] = string.char(low | (high << 4))
  end
  return table.concat(data)
end

-- A register of the supply: Length bytes, or nil and a status.
local function read_register(opened, register, length)
  local _, failed = command(opened, string.char(0x13, 0x03, 0x06, 0x01, 0x07, length, register), 1)
  if failed then
    return nil, failed
  end
  local data, status = command(opened, string.char(0x08, 0x07, length), length + 1)
  if not data then
    return nil, status
  end
  return string.sub(data, 1, length)
end

local function write_register(opened, register, data)
  local _, failed = command(opened, string.char(0x13, 0x01, 0x04, #data + 1, register) .. data, 1)
  return failed
end

-- The output page selected and read back; a status.
local function select_page(opened, page)
  local failed = write_register(opened, PAGE, string.char(page))
  if failed then
    return failed
  end
  local back, status = read_register(opened, PAGE, 1)
  if not back then
    return status
  end
  if string.byte(back) ~= page then
    ksi.trace('page %d selected, %d read back', page, string.byte(back))
    return nt.STATUS_DEVICE_PROTOCOL_ERROR
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

-- The supply's values; a status.
local function supply(opened, values)
  local function value(kind, index, register, scale)
    local data, failed = read_register(opened, register, 2)
    if not data then
      return failed
    end
    values[#values + 1] = { kind, index, linear11((string.unpack('<I2', data)), scale) }
  end
  local status = setup(opened)
  if status ~= nt.STATUS_SUCCESS then
    return status
  end
  local _, failed = command(opened, string.char(0x11, 0x02, 0x64, 0, 0, 0, 0), 1)
  if failed then
    return failed
  end
  local model, modelFailed = read_register(opened, 0x9a, 7)
  if not model then
    return modelFailed
  end
  if string.sub(model, 1, 2) ~= 'AX' then
    ksi.trace('not a listed supply')
    return nt.STATUS_NOT_SUPPORTED
  end
  status = value(KIND_TEMPERATURE, 0, 0x8e, 1000)
    or value(KIND_FAN, 0, 0x90, 1)
    or value(KIND_VOLTAGE, 0, 0x88, 1000)
    or value(KIND_CURRENT, 0, 0x89, 1000)
    or value(KIND_POWER, 0, 0x97, 1000)
    or value(KIND_POWER, 1, 0xee, 1000)
  if status then
    return status
  end
  local saved, savedFailed = read_register(opened, PAGE, 1)
  if not saved then
    return savedFailed
  end
  local restore <close> = setmetatable({}, {
    __close = function()
      select_page(opened, string.byte(saved))
    end,
  })
  for page = 0, OUTPUTS - 1 do
    status = select_page(opened, page)
    if status ~= nt.STATUS_SUCCESS then
      return status
    end
    status = value(KIND_VOLTAGE, 1 + page, 0x8b, 1000)
      or value(KIND_CURRENT, 1 + page, 0x8c, 1000)
      or value(KIND_POWER, 2 + page, 0x96, 1000)
    if status then
      return status
    end
  end
  return nt.STATUS_SUCCESS
end

-- Reads every listed dongle's supply; no input. Returns a status and per supply
-- a record as hid.lua's read; the caller holds the supply's tool mutex.
function read(input)
  if
    not ksi.hw.devices
    or not ksi.hw.device_open
    or not ksi.hw.device_read
    or not ksi.hw.device_write
    or not ksi.hw.device_control
  then
    ksi.trace('no device functions')
    return nt.STATUS_NOT_SUPPORTED
  end
  if #input ~= 0 then
    return nt.STATUS_INVALID_PARAMETER
  end
  local output = {}
  for _, driver in ipairs(DRIVERS) do
    local devices = ksi.hw.devices(driver)
    for _, device in ipairs(devices or {}) do
      local vendor, product = listed(device)
      if #output < DEVICES_MAX and vendor then
        local values = {}
        local status
        local name =
          text_of((ksi.hw.device_property(device, nt.DevicePropertyPhysicalDeviceObjectName)))
        local opened, failed
        if name and #name > 0 then
          opened, failed = ksi.hw.device_open(name, nt.FILE_READ_DATA | nt.FILE_WRITE_DATA)
        else
          failed = nt.STATUS_OBJECT_NAME_NOT_FOUND
        end
        if opened then
          status = supply(opened, values)
          opened:release()
        else
          ksi.trace('the dongle did not open: 0x%x', failed)
          status = failed
        end
        if status ~= nt.STATUS_SUCCESS then
          values = {}
        end
        local entries = {}
        for i = 1, math.min(#values, VALUES_MAX) do
          entries[i] = string.pack('<BBxxi4', values[i][1], values[i][2], values[i][3])
        end
        output[#output + 1] = string.pack('<i4I2I2Bxxx', status, vendor, product, #entries)
          .. table.concat(entries)
          .. string.rep('\0', (VALUES_MAX - #entries) * 8)
      end
      device:release()
    end
  end
  return nt.STATUS_SUCCESS, table.concat(output)
end
