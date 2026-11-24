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
-- Reads the Qualcomm TSENS die temperature sensors of the Snapdragon parts
-- listed below. A part is found by the OEM table ID of the DSDT and the SKU in
-- the version string of the SMBIOS processor structure; the sensors' addresses
-- are fixed by the silicon, so a part that is not listed is never read. Each
-- controller has a SROT block (version, control) and a TM block (the sensors'
-- status registers). Nothing is written.
--

local nt = require('nt')

local ROLE = {
  none = 0,
  cpu_top = 1,
  cpu_bottom = 2,
  cluster_top = 3,
  cluster_bottom = 4,
  aoss = 5,
  memory = 6,
  video = 7,
  npu = 8,
  gpu = 9,
  camera = 10,
}

local CONTROLLERS_MAX = 8
local SENSORS = 16
local SROT_LENGTH = 8
local TM_STATUS = 0xa0
local STATUS_VALID = 0x200000
local VALID_TRIES = 3
local MPIDR = 0x4005
local BLOCK = 0x1000
local ADDRESS_BITS = 48

-- The sensors of a CPU cluster's controller (X1E80100): 0 the
-- AOSS, 1 to 8 the four cores' top and bottom, 9 and 10 the cluster's.
local function hamoa_cluster(cluster)
  local s = { [0] = { ROLE.aoss, cluster } }
  for core = 0, 3 do
    s[1 + core * 2] = { ROLE.cpu_top, cluster, core }
    s[2 + core * 2] = { ROLE.cpu_bottom, cluster, core }
  end
  s[9] = { ROLE.cluster_top, cluster }
  s[10] = { ROLE.cluster_bottom, cluster }
  return s
end

local ROWS = {
  {
    dsdt = 'SDM8380',
    skus = { 'X1E78100', 'X1E80100', 'X1E84100', 'X1P64100', 'X1P66100' },
    controllers = function()
      local c0 = hamoa_cluster(0)
      c0[11] = { ROLE.memory, 0 }
      c0[12] = { ROLE.video, 0 }
      local c3 = { [0] = { ROLE.aoss, 3 } }
      for i = 0, 3 do
        c3[1 + i] = { ROLE.npu, i }
      end
      for i = 0, 7 do
        c3[5 + i] = { ROLE.gpu, i }
      end
      c3[13] = { ROLE.camera, 0 }
      c3[14] = { ROLE.camera, 1 }
      return {
        { srot = 0x0c222000, tm = 0x0c271000, sensors = c0 },
        { srot = 0x0c223000, tm = 0x0c272000, sensors = hamoa_cluster(1) },
        { srot = 0x0c224000, tm = 0x0c273000, sensors = hamoa_cluster(2) },
        { srot = 0x0c225000, tm = 0x0c274000, sensors = c3 },
      }
    end,
  },
}

local kept = {}

-- The bytes of a string with everything but 0 to 9 and A to Z removed and a to
-- z made upper case.
local function alphanumeric(text)
  local bytes = {}
  for i = 1, #text do
    local b = string.byte(text, i)
    if b >= 0x61 and b <= 0x7a then
      b = b - 0x20
    end
    if (b >= 0x30 and b <= 0x39) or (b >= 0x41 and b <= 0x5a) then
      bytes[#bytes + 1] = string.char(b)
    end
  end
  return table.concat(bytes)
end

-- The version string of the first SMBIOS processor (type 4) structure, or nil.
local function processor_version()
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
    if kind == 4 then
      if length < 0x11 then
        return nil
      end
      local strings = {}
      local i = at + length
      while i < ends + 1 and string.byte(smbios, i) ~= 0 do
        local stop = string.find(smbios, '\0', i, true)
        strings[#strings + 1] = string.sub(smbios, i, stop - 1)
        i = stop + 1
      end
      return strings[string.byte(smbios, at + 0x10)] or ''
    end
    if kind == 127 then
      return nil
    end
    at = ends + 2
  end
  return nil
end

-- The OEM table ID of the DSDT, trimmed of spaces, or nil.
local function dsdt_table_id()
  local dsdt = ksi.hw.firmware_table('ACPI', 'DSDT')
  if not dsdt or #dsdt < 36 then
    return nil
  end
  local id = string.sub(dsdt, 17, 24)
  local last = #id
  while last > 0 and (string.byte(id, last) == 0x20 or string.byte(id, last) == 0) do
    last = last - 1
  end
  return string.sub(id, 1, last)
end

-- Each processor's group and number by its cluster and core (MPIDR_EL1's Aff2
-- and Aff1), from one read on every processor.
local function processors_by_core()
  local list = ksi.hw.processors()
  local items, numbers = {}, {}
  for at = 1, #list, 4 do
    local group, number = string.unpack('<I2I2', list, at)
    items[#items + 1] = string.pack('<I2I2I4', group, number, MPIDR)
    numbers[#numbers + 1] = { group, number }
  end
  local results = ksi.hw.sysreg_read(table.concat(items))
  local map = {}
  if not results then
    return map
  end
  for i, n in ipairs(numbers) do
    local status, _, value = string.unpack('<i4I4I8', results, (i - 1) * 16 + 1)
    if status == nt.STATUS_SUCCESS then
      map[((value >> 16) & 0xff) * 256 + ((value >> 8) & 0xff)] = n
    end
  end
  return map
end

-- The row of this part: from the tables once in a kept script, nil when there
-- is none.
local function part()
  if kept.part ~= nil then
    return kept.part or nil
  end
  local found = false
  local id = dsdt_table_id()
  local version = processor_version()
  if id and version then
    version = alphanumeric(version)
    for _, row in ipairs(ROWS) do
      if row.dsdt == id then
        for _, sku in ipairs(row.skus) do
          if string.find(version, sku, 1, true) then
            found = { row = row, controllers = row.controllers() }
            break
          end
        end
      end
      if found then
        break
      end
    end
  else
    ksi.trace('no DSDT or no SMBIOS processor structure')
  end
  if not ksi.volatile then
    kept.part = found
  end
  return found or nil
end

-- The controller's blocks, mapped once in a kept script.
local function blocks(p, index)
  local c = p.controllers[index]
  if c.srot_map then
    return c.srot_map, c.tm_map
  end
  for _, base in ipairs({ c.srot, c.tm }) do
    local st = nt.check_range(base, base + BLOCK, ADDRESS_BITS)
    if st ~= nt.STATUS_SUCCESS then
      return nil, nil, st
    end
  end
  local srot, failed = ksi.hw.map(c.srot, BLOCK)
  if not srot then
    return nil, nil, failed
  end
  local tm
  tm, failed = ksi.hw.map(c.tm, BLOCK)
  if not tm then
    return nil, nil, failed
  end
  if not ksi.volatile then
    c.srot_map, c.tm_map = srot, tm
  end
  return srot, tm
end

-- Returns each controller's SROT version and control words and the role of each
-- of its sensors: <I4 count>, then per controller (CONTROLLERS_MAX of them)
-- <I4 version> <I4 control> and per sensor <I1 role> <I1 index> <I1 group>
-- <I1 number>, the processor of a CPU core's sensors (0xff when none).
function identify(input)
  if not ksi.hw.map or not ksi.hw.sysreg_read or not ksi.hw.firmware_table then
    ksi.trace('no map, system register or firmware table functions')
    return nt.STATUS_NOT_SUPPORTED
  end
  if #input ~= 0 then
    ksi.trace('input of %d bytes', #input)
    return nt.STATUS_INVALID_PARAMETER
  end
  local p = part()
  if not p then
    ksi.trace('no row for this part')
    return nt.STATUS_NOT_SUPPORTED
  end
  local cores = processors_by_core()
  local out = { string.pack('<I4', #p.controllers) }
  for index = 1, CONTROLLERS_MAX do
    local c = p.controllers[index]
    if not c then
      out[#out + 1] = string.rep('\0', SROT_LENGTH + SENSORS * 4)
    else
      local srot, _, failed = blocks(p, index)
      if not srot then
        ksi.trace('controller %d not mapped: 0x%08x', index - 1, failed)
        return failed
      end
      out[#out + 1] = string.pack('<I4I4', ksi.hw.map_read(srot, 0, 4), ksi.hw.map_read(srot, 4, 4))
      for s = 0, SENSORS - 1 do
        local role = c.sensors[s]
        local group, number = 0xff, 0xff
        if role and (role[1] == ROLE.cpu_top or role[1] == ROLE.cpu_bottom) then
          local n = cores[role[2] * 256 + role[3]]
          if n then
            group, number = n[1], n[2]
          end
        end
        out[#out + 1] = string.pack(
          '<I1I1I1I1',
          role and role[1] or ROLE.none,
          role and role[2] or 0,
          group,
          number
        )
      end
    end
  end
  return nt.STATUS_SUCCESS, table.concat(out)
end

-- Reads the status registers of the sensors each controller's SROT control
-- word enables: <I4> per sensor per controller of the part, 0 for a sensor not
-- enabled. A sensor not valid yet is read again, up to VALID_TRIES times.
function read(input)
  if not ksi.hw.map or not ksi.hw.sysreg_read or not ksi.hw.firmware_table then
    ksi.trace('no map, system register or firmware table functions')
    return nt.STATUS_NOT_SUPPORTED
  end
  if #input ~= 0 then
    ksi.trace('input of %d bytes', #input)
    return nt.STATUS_INVALID_PARAMETER
  end
  local p = part()
  if not p then
    ksi.trace('no row for this part')
    return nt.STATUS_NOT_SUPPORTED
  end
  local out = {}
  for index = 1, #p.controllers do
    local srot, tm, failed = blocks(p, index)
    if not srot then
      ksi.trace('controller %d not mapped: 0x%08x', index - 1, failed)
      return failed
    end
    local enabled = (ksi.hw.map_read(srot, 4, 4) >> 3) & 0xffff
    for s = 0, SENSORS - 1 do
      local value = 0
      if (enabled >> s) & 1 == 1 then
        for _ = 1, VALID_TRIES do
          value = ksi.hw.map_read(tm, TM_STATUS + s * 4, 4)
          if value & STATUS_VALID ~= 0 then
            break
          end
        end
      end
      out[#out + 1] = string.pack('<I4', value)
    end
  end
  return nt.STATUS_SUCCESS, table.concat(out)
end
