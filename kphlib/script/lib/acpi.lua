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
-- ACPI devices: found by an ID or a namespace path, their IDs and path, all
-- from the devices of the ACPI driver. Reached as nt.acpi.
--

return function(nt)
  local A = {}

  -- UTF-16 text as one byte a code unit: the unit itself below 80h, FFh above.
  local function units(text)
    local out = {}
    for i = 1, #text - 1, 2 do
      local low, high = string.byte(text, i, i + 1)
      out[#out + 1] = (high == 0 and low < 0x80) and string.char(low) or '\255'
    end
    return table.concat(out)
  end

  -- Whether every character of a string is one of A-Z, _ and, past the first
  -- when first is set, 0-9.
  local function name(text, first)
    for i = 1, #text do
      local c = string.byte(text, i)
      local letter = (c >= 0x41 and c <= 0x5A) or c == 0x5F
      local digit = c >= 0x30 and c <= 0x39
      if not letter and not (digit and not (first and i == 1)) then
        return false
      end
    end
    return true
  end

  -- Where a UTF-16 code unit first is in UTF-16 text (at a unit, not between
  -- two), nil when it is not.
  local function find_unit(text, unit, at)
    local found = string.find(text, unit, at, true)
    while found and found % 2 == 0 do
      found = string.find(text, unit, found + 1, true)
    end
    return found
  end

  -- The ACPI identity of a hardware or compatible ID (UTF-16): ACPI\ or *
  -- dropped, the vendor form VEN_v&DEV_d joined, upper case; nil unless it is
  -- then 7 or 8 of A-Z, 0-9 and _. Upper case is taken of the UTF-16 bytes:
  -- a unit over 7Fh stays over 7Fh and is refused.
  local function identity(id)
    local rest = string.upper(id)
    if string.sub(rest, 1, 10) == 'A\0C\0P\0I\0\\\0' then
      rest = string.sub(rest, 11)
    elseif string.sub(rest, 1, 2) == '*\0' then
      rest = string.sub(rest, 3)
    else
      return nil
    end
    if string.sub(rest, 1, 8) == 'V\0E\0N\0_\0' then
      rest = string.sub(rest, 9)
      local amp = find_unit(rest, '&\0', 1) or #rest + 1
      if string.sub(rest, amp, amp + 9) ~= '&\0D\0E\0V\0_\0' then
        return nil
      end
      rest = string.sub(rest, 1, amp - 1) .. string.sub(rest, amp + 10)
    end
    if #rest ~= 14 and #rest ~= 16 then
      return nil
    end
    rest = units(rest)
    if not name(rest, false) then
      return nil
    end
    return rest
  end

  -- The identities of a device's hardware IDs, then its compatible IDs; an ID
  -- property that cannot be read is passed over.
  local function identities(device)
    local list = {}
    for _, key in ipairs({ nt.DEVPKEY_Device_HardwareIds, nt.DEVPKEY_Device_CompatibleIds }) do
      local bytes = ksi.hw.device_property_data(device, key)
      if bytes then
        local text = bytes .. '\0\0'
        local at = 1
        while true do
          local stop = find_unit(text, '\0\0', at)
          if not stop or stop == at then
            break
          end
          list[#list + 1] = identity(string.sub(text, at, stop - 1))
          at = stop + 2
        end
      end
    end
    return list
  end

  -- A namespace path normalized: a backslash, then segments of 1 to 4 of A-Z,
  -- _ and (past the first) 0-9 between dots, each without its trailing
  -- underscore padding. Nil when it is not such a path.
  local function normalize(path)
    if #path < 2 or string.byte(path, 1) ~= 0x5C then
      return nil
    end
    local segments = {}
    local at = 2
    while at <= #path + 1 do
      local stop = string.find(path, '.', at, true) or #path + 1
      local segment = string.sub(path, at, stop - 1)
      if #segment < 1 or #segment > 4 or not name(segment, true) then
        return nil
      end
      local last = #segment
      while last > 1 and string.byte(segment, last) == 0x5F do
        last = last - 1
      end
      segments[#segments + 1] = string.sub(segment, 1, last)
      at = stop + 1
    end
    return '\\' .. table.concat(segments, '.')
  end

  -- The devices of \Driver\ACPI, nil when they cannot be listed.
  local function devices()
    return ksi.hw.devices('\\Driver\\ACPI')
  end

  -- The device's namespace path as its BIOS name gives it, normalized. Nil and
  -- STATUS_NOT_FOUND when it has none that is a path.
  function A.path(device)
    local bytes, kind = ksi.hw.device_property_data(device, nt.DEVPKEY_Device_BiosDeviceName)
    if not bytes or kind ~= nt.DEVPROP_TYPE_STRING or #bytes < 4 or #bytes % 2 ~= 0 then
      return nil, nt.STATUS_NOT_FOUND
    end
    if #bytes // 2 - 1 >= 0xFFFF or string.sub(bytes, -2) ~= '\0\0' then
      return nil, nt.STATUS_NOT_FOUND
    end
    local text = units(string.sub(bytes, 1, -3))
    if string.byte(text, 1) ~= 0x5C then
      text = '\\' .. text
    end
    local path = normalize(text)
    if not path then
      return nil, nt.STATUS_NOT_FOUND
    end
    return path
  end

  -- The device's IDs as ACPI identities, hardware IDs first.
  function A.ids(device)
    return identities(device)
  end

  -- The devices of the ACPI driver in its order, each as { device, ids }, so a
  -- script that looks for many IDs walks them once. Nil and STATUS_NOT_FOUND
  -- when they cannot be listed.
  function A.list()
    local list = devices()
    if not list then
      return nil, nt.STATUS_NOT_FOUND
    end
    local out = {}
    for i, d in ipairs(list) do
      out[i] = { device = d, ids = identities(d) }
    end
    return out
  end

  -- The device of the ACPI driver that has an ID, the ordinal-th (from 0) of
  -- them. Nil and STATUS_NOT_FOUND when there is none.
  function A.find(id, ordinal)
    ordinal = ordinal or 0
    if type(id) ~= 'string' or #id < 1 or #id > 8 then
      error("bad argument #1 to 'find' (1 to 8 characters)", 2)
    end
    if type(ordinal) ~= 'number' or ordinal < 0 or ordinal > 0xFFFFFFFF then
      error("bad argument #2 to 'find' (out of range)", 2)
    end
    local list = devices()
    if not list then
      return nil, nt.STATUS_NOT_FOUND
    end
    local found
    for _, d in ipairs(list) do
      local match = false
      if not found then
        for _, identity in ipairs(identities(d)) do
          if identity == id then
            match = true
            break
          end
        end
      end
      if match and ordinal == 0 then
        found = d
      else
        if match then
          ordinal = ordinal - 1
        end
        d:release()
      end
    end
    if not found then
      return nil, nt.STATUS_NOT_FOUND
    end
    return found
  end

  -- The device of the ACPI driver at a namespace path, padded or not. Nil and
  -- STATUS_NOT_FOUND when there is none.
  function A.open(path)
    if type(path) ~= 'string' then
      error("bad argument #1 to 'open' (string expected)", 2)
    end
    local wanted = #path <= 0xFFFF and normalize(path)
    local list = wanted and devices()
    if not list then
      return nil, nt.STATUS_NOT_FOUND
    end
    local found
    for _, d in ipairs(list) do
      if not found and A.path(d) == wanted then
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

  return A
end
