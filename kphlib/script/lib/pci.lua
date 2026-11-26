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
-- PCI functions: the device of the PCI bus driver at a location. Reached as
-- nt.pci.
--

return function(nt)
  local M = {}
  local remove = table.remove

  -- the location text the driver read: 128 characters and the terminator
  local LOCATION_MAX = 256

  local function check(value, maximum, argument)
    if type(value) ~= 'number' or value < 0 or value > maximum then
      error(string.format("bad argument #%d to 'find' (out of range)", argument), 3)
    end
  end

  -- A 32-bit property's bytes as a number, nil when unread or another length.
  local function uint32(bytes)
    if not bytes or #bytes ~= 4 then
      return nil
    end
    return string.unpack('<I4', bytes)
  end

  -- The decimal numbers of a location (UTF-16) up to its terminator, nil when
  -- there are more than four or one is over 65535.
  local function location_numbers(text)
    local numbers = {}
    local inNumber = false
    for i = 1, #text - 1, 2 do
      local c = string.unpack('<I2', text, i)
      if c == 0 then
        break
      end
      if c < 0x30 or c > 0x39 then
        inNumber = false
      else
        if not inNumber then
          if #numbers == 4 then
            return nil
          end
          inNumber = true
          numbers[#numbers + 1] = 0
        end
        local n = numbers[#numbers] * 10 + (c - 0x30)
        if n > 0xFFFF then
          return nil
        end
        numbers[#numbers] = n
      end
    end
    return numbers
  end

  -- Whether a device is at a location: its bus number, its address, then its
  -- location, which must name the same bus, device and function, and the
  -- segment when it has four numbers (0 when it has three).
  local function at(d, segment, bus, device, fn)
    local number = uint32(ksi.hw.device_property(d, nt.DevicePropertyBusNumber))
    if not number or number & 0xFF ~= bus then
      return false
    end
    local address = uint32((ksi.hw.device_property_data(d, nt.DEVPKEY_Device_Address)))
    if not address or address >> 16 ~= device or address & 0xFFFF ~= fn then
      return false
    end
    local text = ksi.hw.device_property(d, nt.DevicePropertyLocationInformation)
    if not text or #text > LOCATION_MAX then
      return false
    end
    local n = location_numbers(text)
    if not n then
      return false
    end
    local s = 0
    if #n == 4 then
      s = remove(n, 1)
    elseif #n ~= 3 then
      return false
    end
    return s == segment and n[1] == bus and n[2] == device and n[3] == fn
  end

  -- The device of \Driver\pci at a location. Nil and the status of the call
  -- when the devices cannot be listed, nil and STATUS_NOT_FOUND when none is
  -- there.
  function M.find(segment, bus, device, fn)
    check(segment, 0xFFFF, 1)
    check(bus, 0xFF, 2)
    check(device, 0x1F, 3)
    check(fn, 0x7, 4)
    local devices, status = ksi.hw.devices('\\Driver\\pci')
    if not devices then
      return nil, status
    end
    local found
    for _, d in ipairs(devices) do
      if not found and at(d, segment, bus, device, fn) then
        found = d
      else
        d:release()
      end
    end
    if not found then
      return nil, nt.STATUS_NOT_FOUND
    end
    return found
  end

  -- A PCI device object of the function at a location: find, then
  -- ksi.hw.pci_open. Nil and the status of the call when either refuses.
  function M.open(segment, bus, device, fn)
    local d, status = M.find(segment, bus, device, fn)
    if not d then
      return nil, status
    end
    local opened, failed = ksi.hw.pci_open(d)
    d:release()
    return opened, failed
  end

  -- The function's most recent power state, a DEVICE_POWER_STATE. Nil and a
  -- status when its power data cannot be read.
  function M.power_state(device)
    local bytes, kind = ksi.hw.device_property_data(device, nt.DEVPKEY_Device_PowerData)
    if not bytes then
      return nil, kind
    end
    if kind ~= nt.DEVPROP_TYPE_BINARY or #bytes ~= 56 then
      return nil, nt.STATUS_DEVICE_CONFIGURATION_ERROR
    end
    return (string.unpack('<I4', bytes, 5))
  end

  -- The status of an access the bus refused: STATUS_DEVICE_POWERED_OFF for a
  -- device error of a function that is not in D0, the status given otherwise.
  function M.refused(device, status)
    if status == nt.STATUS_IO_DEVICE_ERROR then
      local state = M.power_state(device)
      if state and state ~= nt.PowerDeviceD0 then
        return nt.STATUS_DEVICE_POWERED_OFF
      end
    end
    return status
  end

  return M
end
