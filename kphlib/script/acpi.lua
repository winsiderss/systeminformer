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
-- Finds the ACPI devices the rows below cover and evaluates the methods each
-- row permits, with the arguments it permits.
--

local nt = require('nt')

local ROWS = {
  {
    ids = {
      'PNP0C0B',
      'INT3404',
      'INTC1044',
      'INTC1048',
      'INTC1063',
      'INTC106A',
      'INTC10A2',
      'INTC10D6',
      'INTC10FE',
      'INTC10F5',
    },
    evals = function()
      return {
        { name = '_FST', results = 0x9, max = 0x40 },
        { name = '_FIF', results = 0x1, max = 0x40 },
        { name = '_FPS', results = 0x9, max = 0x800 },
      }
    end,
  },
  {
    ids = {
      'INT3403',
      'INTC1043',
      'INTC1046',
      'INTC1062',
      'INTC1069',
      'INTC10A1',
      'INTC10D5',
      'INTC10FD',
      'INTC10F4',
    },
    evals = function()
      return {
        { name = '_STA', results = 0x1, max = 0x20 },
        { name = '_STR', results = 0x6, max = 0x100 },
        { name = 'PTYP', results = 0x1, max = 0x20 },
        { name = '_TMP', results = 0x1, max = 0x20 },
        { name = '_PSV', results = 0x1, max = 0x20 },
        { name = '_CRT', results = 0x1, max = 0x20 },
        { name = '_AC0', results = 0x1, max = 0x20 },
        { name = 'GTSH', results = 0x1, max = 0x20 },
      }
    end,
  },
  {
    ids = { 'ATK0110' },
    evals = function()
      return {
        { name = 'TSIF', results = 0x9, max = 0x800 },
        { name = 'VSIF', results = 0x9, max = 0x800 },
        { name = 'FSIF', results = 0x9, max = 0x800 },
        {
          name = 'RTMP',
          results = 0x1,
          max = 0x20,
          args = { { 0xffff, 0xffff, 0x6030000 } },
        },
        {
          name = 'RVLT',
          results = 0x1,
          max = 0x20,
          args = { { 0xffff, 0xffff, 0x6020000 } },
        },
        {
          name = 'RFAN',
          results = 0x1,
          max = 0x20,
          args = { { 0xffff, 0xffff, 0x6040000 } },
        },
      }
    end,
  },
  {
    ids = { 'LEN0068', 'LEN0268' },
    evals = function()
      return {
        {
          name = 'DYTC',
          results = 0x1,
          max = 0x20,
          args = { { 0x0, 0x0, 0x0 } },
        },
        {
          name = 'DYTC',
          results = 0x1,
          max = 0x20,
          args = { { 0x0, 0x0, 0x2 } },
        },
        {
          name = 'DYTC',
          results = 0x1,
          max = 0x20,
          args = { { 0x0, 0x0, 0x3 } },
        },
        {
          name = 'DYTC',
          results = 0x1,
          max = 0x20,
          args = { { 0x0, 0x0, 0x8 } },
        },
      }
    end,
  },
  {
    ids = { 'PNP0C09' },
    evals = function()
      return {
        {
          name = 'TMP0',
          results = 0x1,
          max = 0x20,
          require = { 'IBM0068', 'LEN0068', 'LEN0268' },
        },
        {
          name = 'TMP1',
          results = 0x1,
          max = 0x20,
          require = { 'IBM0068', 'LEN0068', 'LEN0268' },
        },
        {
          name = 'TMP2',
          results = 0x1,
          max = 0x20,
          require = { 'IBM0068', 'LEN0068', 'LEN0268' },
        },
        {
          name = 'TMP3',
          results = 0x1,
          max = 0x20,
          require = { 'IBM0068', 'LEN0068', 'LEN0268' },
        },
        {
          name = 'TMP4',
          results = 0x1,
          max = 0x20,
          require = { 'IBM0068', 'LEN0068', 'LEN0268' },
        },
        {
          name = 'TMP5',
          results = 0x1,
          max = 0x20,
          require = { 'IBM0068', 'LEN0068', 'LEN0268' },
        },
        {
          name = 'TMP6',
          results = 0x1,
          max = 0x20,
          require = { 'IBM0068', 'LEN0068', 'LEN0268' },
        },
        {
          name = 'TMP7',
          results = 0x1,
          max = 0x20,
          require = { 'IBM0068', 'LEN0068', 'LEN0268' },
        },
      }
    end,
  },
  {
    ids = { 'ACPI000D' },
    evals = function()
      return {
        { name = '_STA', results = 0x1, max = 0x20 },
        { name = '_PMC', results = 0x3, max = 0x200 },
        { name = '_PMM', results = 0x1, max = 0x20 },
        { name = '_GAI', results = 0x1, max = 0x20 },
      }
    end,
  },
  {
    ids = {
      'INT3407',
      'INT3532',
      'INTC1047',
      'INTC1050',
      'INTC1060',
      'INTC1061',
      'INTC1065',
      'INTC1066',
      'INTC106C',
      'INTC106D',
      'INTC10A4',
      'INTC10A5',
      'INTC10D8',
      'INTC10D9',
      'INTC1100',
      'INTC1101',
      'INTC10F7',
      'INTC10F8',
    },
    evals = function()
      return {
        { name = '_STA', results = 0x1, max = 0x20 },
        { name = 'PTYP', results = 0x1, max = 0x20 },
        { name = 'PSRC', results = 0x1, max = 0x20 },
        { name = 'PMAX', results = 0x1, max = 0x20 },
        { name = 'ARTG', results = 0x1, max = 0x20 },
        { name = 'PBSS', results = 0x1, max = 0x20 },
        { name = 'PROP', results = 0x1, max = 0x20 },
        { name = 'CTYP', results = 0x1, max = 0x20 },
        { name = 'RBHF', results = 0x1, max = 0x20 },
        { name = 'VBNL', results = 0x1, max = 0x20 },
        { name = 'CMPP', results = 0x1, max = 0x20 },
      }
    end,
  },
  {
    ids = { '17761776' },
    evals = function()
      return {
        { name = 'NFAN', results = 0x8, max = 0x200 },
        { name = 'NTMP', results = 0x8, max = 0x200 },
        {
          name = 'GFAN',
          results = 0x1,
          max = 0x20,
          args = { { 0x7, 0x7, 0x0 } },
        },
        {
          name = 'GTMP',
          results = 0x1,
          max = 0x20,
          args = { { 0x7, 0x7, 0x0 } },
        },
      }
    end,
  },
}

local MILLISECONDS_MAX = 0xfffffffe

local ARGUMENT_INTEGER = 0
local ARGUMENT_PACKAGE = 3

local kept = {}

local function row_has(row, id)
  for _, candidate in ipairs(row.ids) do
    if candidate == id then
      return true
    end
  end
  return false
end

local function first_id(row, ids)
  for _, id in ipairs(ids) do
    if row_has(row, id) then
      return id
    end
  end
  return nil
end

local function has_id(entry, id)
  for _, candidate in ipairs(entry.ids) do
    if candidate == id then
      return true
    end
  end
  return false
end

-- The ACPI devices of the call, each { device, ids }: listed once a call, the
-- path read once a device; the routines release them at their end, but those a
-- row keeps.
local listed

local function devices()
  if not listed then
    listed = nt.acpi.list() or {}
  end
  return listed
end

local function release_devices()
  if not listed then
    return
  end
  for _, entry in ipairs(listed) do
    if not entry.held then
      entry.device:release()
    end
  end
  listed = nil
end

local function path_of(entry)
  if entry.path == nil and entry.failed == nil then
    entry.path, entry.failed = nt.acpi.path(entry.device)
  end
  return entry.path, entry.failed
end

-- Visits each device a row covers, by the row's IDs in order and each ID's
-- devices in the driver's order, once a path. A device another row keeps is
-- opened again, so no two rows share one.
local function each_device(row, visit)
  local seen = {}
  local ordinal = 0
  for _, id in ipairs(row.ids) do
    local index = 0
    for _, entry in ipairs(devices()) do
      if has_id(entry, id) then
        local path, failed = path_of(entry)
        if not path then
          ksi.trace('%s %d has no path: 0x%08x', id, index, failed)
        elseif not seen[path] then
          seen[path] = true
          local device = entry.device
          if entry.held then
            device = nt.acpi.open(path)
          end
          if device then
            local stop, held = visit(ordinal, device, entry.ids, path)
            if held and device == entry.device then
              entry.held = true
            elseif not held and device ~= entry.device then
              device:release()
            end
            ordinal = ordinal + 1
            if stop then
              return
            end
          end
        end
        index = index + 1
      end
    end
  end
end

local function present(ids)
  local yes = kept.present
  if yes then
    for _, id in ipairs(ids) do
      if yes[id] then
        return true
      end
    end
  end
  local found
  for _, id in ipairs(ids) do
    for _, entry in ipairs(devices()) do
      if not found and has_id(entry, id) then
        found = id
      end
    end
  end
  if found then
    if not ksi.volatile then
      kept.present = yes or {}
      kept.present[found] = true
    end
    return true
  end
  return false
end

-- A row's evaluations by name, each name's in row order and by argument count.
local function row_names(number)
  local names = kept.names and kept.names[number]
  if not names then
    names = {}
    for _, eval in ipairs(ROWS[number].evals()) do
      local entry = names[eval.name]
      if not entry then
        entry = { all = {}, counted = {} }
        names[eval.name] = entry
      end
      entry.all[#entry.all + 1] = eval
      local count = #(eval.args or {})
      entry.counted[count] = entry.counted[count] or {}
      local counted = entry.counted[count]
      counted[#counted + 1] = eval
    end
    if not ksi.volatile then
      kept.names = kept.names or {}
      kept.names[number] = names
    end
  end
  return names
end

local function argument_values(arguments)
  local values = {}
  local at = 1
  while at <= #arguments do
    if at + 3 > #arguments then
      return nil, true
    end
    local kind, length = string.unpack('<I2I2', arguments, at)
    if at + 3 + math.max(4, length) > #arguments then
      return nil, true
    end
    if kind ~= ARGUMENT_INTEGER or length ~= 4 then
      return nil, false
    end
    values[#values + 1] = string.unpack('<I4', arguments, at + 4)
    at = at + 8
  end
  return values
end

local function allowed(eval, values)
  for i, arg in ipairs(eval.args or {}) do
    local value = values[i]
    if (value & ~arg[1]) ~= arg[3] or (value & arg[1]) > arg[2] then
      return false
    end
  end
  return true
end

local function result_types(output)
  local count = string.unpack('<I4', output, 9)
  local types = 0
  local at = 13
  for _ = 1, count do
    local kind, data = string.unpack('<I2I2', output, at)
    if kind > ARGUMENT_PACKAGE then
      return nil
    end
    types = types | (1 << kind)
    at = at + 4 + math.max(4, data)
  end
  return types
end

local function evaluate_on(device, record, milliseconds)
  local entry = row_names(record.row)[record.name]
  if not entry then
    ksi.trace('row %d does not permit %s', record.row, record.name)
    return nt.STATUS_ACCESS_DENIED
  end
  local candidates = entry.all
  if candidates[1].require and not present(candidates[1].require) then
    ksi.trace('%s: no required device is present', record.name)
    return nt.STATUS_NOT_FOUND
  end
  local values, malformed = argument_values(record.arguments)
  if not values then
    if malformed then
      ksi.trace('%s: argument bytes do not parse', record.name)
      return nt.STATUS_INVALID_PARAMETER
    end
    ksi.trace('%s: arguments that no row describes', record.name)
    return nt.STATUS_ACCESS_DENIED
  end
  local counted = entry.counted[#values]
  if not counted then
    ksi.trace('%s takes no %d arguments', record.name, #values)
    return nt.STATUS_INVALID_PARAMETER
  end
  local eval
  for _, candidate in ipairs(counted) do
    if allowed(candidate, values) then
      eval = candidate
      break
    end
  end
  if not eval then
    ksi.trace('%s: argument values not permitted', record.name)
    return nt.STATUS_ACCESS_DENIED
  end
  local arguments = #values > 0 and record.arguments or nil
  local output, failed = nt.acpi.eval(device, record.name, milliseconds, arguments)
  if not output then
    ksi.trace('%s failed: 0x%08x', record.name, failed)
    -- STATUS_TIMEOUT is a success code; the item's answer is an error
    if failed == nt.STATUS_TIMEOUT then
      return nt.STATUS_IO_TIMEOUT
    end
    return failed
  end
  if #output > eval.max then
    ksi.trace('%s output of %d bytes exceeds 0x%x', record.name, #output, eval.max)
    return nt.STATUS_BUFFER_OVERFLOW
  end
  local types = result_types(output)
  if not types or (types & ~eval.results) ~= 0 then
    ksi.trace('%s result types are not permitted', record.name)
    return nt.STATUS_OBJECT_TYPE_MISMATCH
  end
  return nt.STATUS_SUCCESS, output
end

-- Keeps the device found for a row and ordinal; false in a volatile script.
local function remember(number, ordinal, device)
  if ksi.volatile then
    return false
  end
  kept[number] = kept[number] or {}
  kept[number][ordinal] = { device = device }
  return true
end

local function evaluate(record, milliseconds)
  local row = ROWS[record.row]
  local devices = kept[record.row]
  local k = devices and devices[record.ordinal]
  local status, output
  if k then
    status, output = evaluate_on(k.device, record, milliseconds)
    if status == nt.STATUS_NO_SUCH_DEVICE then
      ksi.trace('row %d device %d is gone', record.row, record.ordinal)
      devices[record.ordinal] = nil
      k.device:release()
    end
    return status, output
  end
  each_device(row, function(ordinal, device)
    if ordinal ~= record.ordinal then
      return false, false
    end
    status, output = evaluate_on(device, record, milliseconds)
    return true, remember(record.row, ordinal, device)
  end)
  if not status then
    ksi.trace('row %d has no device %d', record.row, record.ordinal)
    return nt.STATUS_NOT_FOUND
  end
  return status, output
end

-- The room of a path in identify's output, its zero terminator included.
local PATH_MAX = 128

local function pad(data)
  return data .. string.rep('\0', (8 - #data % 8) % 8)
end

local function identify_call(input)
  if not ksi.hw.devices then
    return nt.STATUS_NOT_SUPPORTED
  end
  if #input ~= 0 then
    ksi.trace('input of %d bytes', #input)
    return nt.STATUS_INVALID_PARAMETER
  end
  local old = kept
  kept = {}
  for number = 1, #ROWS do
    for _, k in pairs(old[number] or {}) do
      k.device:release()
    end
  end
  local output = {}
  for number, row in ipairs(ROWS) do
    each_device(row, function(ordinal, device, ids, path)
      local id = first_id(row, ids)
      if #path >= PATH_MAX then
        ksi.trace('%s %d: a path of %d characters is not given', id, ordinal, #path)
        path = ''
      end
      output[#output + 1] = string.pack(
        '<I4I4c8c' .. PATH_MAX,
        number,
        ordinal,
        id .. string.rep('\0', 8 - #id),
        path .. string.rep('\0', PATH_MAX - #path)
      )
      return false, remember(number, ordinal, device)
    end)
  end
  return nt.STATUS_SUCCESS, table.concat(output)
end

local function eval_call(input)
  if not ksi.hw.device_control then
    return nt.STATUS_NOT_SUPPORTED
  end
  if #input < 4 then
    ksi.trace('input of %d bytes', #input)
    return nt.STATUS_INVALID_PARAMETER
  end
  local milliseconds = string.unpack('<I4', input)
  if milliseconds == 0 or milliseconds > MILLISECONDS_MAX then
    ksi.trace('%u milliseconds', milliseconds)
    return nt.STATUS_INVALID_PARAMETER
  end
  local records = {}
  local at = 5
  while at <= #input do
    if at + 15 > #input then
      ksi.trace('record at %d is truncated', at)
      return nt.STATUS_INVALID_PARAMETER
    end
    local row, ordinal, names, arguments = string.unpack('<I4I4I4I4', input, at)
    local size = 16 + names + arguments
    if not ROWS[row] or names == 0 or names > 255 or at + size - 1 > #input then
      ksi.trace('record at %d is malformed', at)
      return nt.STATUS_INVALID_PARAMETER
    end
    records[#records + 1] = {
      row = row,
      ordinal = ordinal,
      name = string.sub(input, at + 16, at + 15 + names),
      arguments = string.sub(input, at + 16 + names, at + size - 1),
    }
    at = at + size + (8 - size % 8) % 8
  end
  if #records == 0 then
    ksi.trace('no record')
    return nt.STATUS_INVALID_PARAMETER
  end
  local output = {}
  for i, record in ipairs(records) do
    local status, result = evaluate(record, milliseconds)
    result = result or ''
    output[i] = string.pack('<i4I4', status, #result) .. pad(result)
  end
  return nt.STATUS_SUCCESS, table.concat(output)
end

-- Releases the call's devices when a routine ends, by return or by error.
local release = setmetatable({}, { __close = release_devices })

-- Returns the row, ordinal, matching ID and namespace path of each device a row
-- covers.
function identify(input)
  local closer <close> = release
  return identify_call(input)
end

-- Evaluates the permitted methods of the devices the rows cover.
function eval(input)
  local closer <close> = release
  return eval_call(input)
end
