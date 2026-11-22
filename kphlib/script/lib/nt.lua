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
-- The Windows NT definitions scripts use: status values and the tests of their
-- severity, device property keys and values, and the parts (nt.pci, nt.acpi). A
-- script reaches it with local nt = require('nt').
--

local M = {}

-- A status as Windows writes it, as the signed 32-bit value the driver gives.
local function status(value)
  return (value ~ 0x80000000) - 0x80000000
end

-- A DEVPROPKEY as devpkey.h writes it: the GUID's fields, then the property id.
local function devpkey(data1, data2, data3, b0, b1, b2, b3, b4, b5, b6, b7, pid)
  return {
    guid = string.pack('<I4I2I2BBBBBBBB', data1, data2, data3, b0, b1, b2, b3, b4, b5, b6, b7),
    pid = pid,
  }
end

M.STATUS_ACCESS_DENIED = status(0xC0000022)
M.STATUS_ACPI_INVALID_DATA = status(0xC014000F)
M.STATUS_ACPI_INVALID_TABLE = status(0xC0140019)
M.STATUS_BUFFER_OVERFLOW = status(0x80000005)
M.STATUS_BUFFER_TOO_SMALL = status(0xC0000023)
M.STATUS_CANCELLED = status(0xC0000120)
M.STATUS_CONFLICTING_ADDRESSES = status(0xC0000018)
M.STATUS_CONTEXT_MISMATCH = status(0xC0000719)
M.STATUS_DATATYPE_MISALIGNMENT = status(0x80000002)
M.STATUS_DEVICE_BUSY = status(0x80000011)
M.STATUS_DEVICE_CONFIGURATION_ERROR = status(0xC0000182)
M.STATUS_DEVICE_DATA_ERROR = status(0xC000009C)
M.STATUS_DEVICE_NOT_READY = status(0xC00000A3)
M.STATUS_DEVICE_POWERED_OFF = status(0x8000000F)
M.STATUS_INFO_LENGTH_MISMATCH = status(0xC0000004)
M.STATUS_INSUFFICIENT_RESOURCES = status(0xC000009A)
M.STATUS_INTEGER_OVERFLOW = status(0xC0000095)
M.STATUS_INTERNAL_ERROR = status(0xC00000E5)
M.STATUS_INVALID_BUFFER_SIZE = status(0xC0000206)
M.STATUS_INVALID_PARAMETER = status(0xC000000D)
M.STATUS_IO_DEVICE_ERROR = status(0xC0000185)
M.STATUS_IO_TIMEOUT = status(0xC00000B5)
M.STATUS_NOT_FOUND = status(0xC0000225)
M.STATUS_NOT_IMPLEMENTED = status(0xC0000002)
M.STATUS_NOT_SUPPORTED = status(0xC00000BB)
M.STATUS_NO_SUCH_DEVICE = status(0xC000000E)
M.STATUS_OBJECT_NAME_NOT_FOUND = status(0xC0000034)
M.STATUS_OBJECT_TYPE_MISMATCH = status(0xC0000024)
M.STATUS_RETRY = status(0xC000022D)
M.STATUS_SUCCESS = status(0x00000000)
M.STATUS_UNSUCCESSFUL = status(0xC0000001)

function M.NT_SUCCESS(s)
  return s >= 0
end

function M.NT_INFORMATION(s)
  return (s >> 30) & 3 == 1
end

function M.NT_WARNING(s)
  return (s >> 30) & 3 == 2
end

function M.NT_ERROR(s)
  return (s >> 30) & 3 == 3
end

M.DEVPKEY_Device_RemovalPolicy =
  devpkey(0xa45c254e, 0xdf1c, 0x4efd, 0x80, 0x20, 0x67, 0xd1, 0x46, 0xa8, 0x50, 0xe0, 33)
M.DEVPKEY_Device_BusNumber =
  devpkey(0xa45c254e, 0xdf1c, 0x4efd, 0x80, 0x20, 0x67, 0xd1, 0x46, 0xa8, 0x50, 0xe0, 23)
M.DEVPKEY_Device_Address =
  devpkey(0xa45c254e, 0xdf1c, 0x4efd, 0x80, 0x20, 0x67, 0xd1, 0x46, 0xa8, 0x50, 0xe0, 30)
M.DEVPKEY_Device_LocationInfo =
  devpkey(0xa45c254e, 0xdf1c, 0x4efd, 0x80, 0x20, 0x67, 0xd1, 0x46, 0xa8, 0x50, 0xe0, 15)
M.DEVPKEY_Device_HardwareIds =
  devpkey(0xa45c254e, 0xdf1c, 0x4efd, 0x80, 0x20, 0x67, 0xd1, 0x46, 0xa8, 0x50, 0xe0, 3)
M.DEVPKEY_Device_CompatibleIds =
  devpkey(0xa45c254e, 0xdf1c, 0x4efd, 0x80, 0x20, 0x67, 0xd1, 0x46, 0xa8, 0x50, 0xe0, 4)
M.DEVPKEY_Device_BiosDeviceName =
  devpkey(0x540b947e, 0x8b40, 0x45bc, 0xa8, 0xa2, 0x6a, 0x0b, 0x89, 0x4c, 0xbd, 0xa2, 10)
M.DEVPKEY_Device_PowerData =
  devpkey(0xa45c254e, 0xdf1c, 0x4efd, 0x80, 0x20, 0x67, 0xd1, 0x46, 0xa8, 0x50, 0xe0, 32)

M.DEVPROP_TYPE_UINT32 = 0x00000007
M.DEVPROP_TYPE_STRING = 0x00000012
M.DEVPROP_TYPE_BINARY = 0x00001003

M.DevicePropertyBootConfiguration = 0x3
M.DevicePropertyBootConfigurationTranslated = 0x4
M.DevicePropertyResourceRequirements = 0x14
M.DevicePropertyAllocatedResources = 0x15

M.RemovalPolicyExpectNoRemoval = 1
M.RemovalPolicyExpectOrderlyRemoval = 2
M.RemovalPolicyExpectSurpriseRemoval = 3

M.PowerDeviceD0 = 1

-- Whether a device may go away while a script holds it: true unless the system
-- expects it never to be removed, and true when its policy cannot be read.
function M.is_device_removable(device)
  local bytes, kind = ksi.hw.device_property_data(device, M.DEVPKEY_Device_RemovalPolicy)
  if not bytes or kind ~= M.DEVPROP_TYPE_UINT32 or #bytes ~= 4 then
    return true
  end
  return string.unpack('<I4', bytes) ~= M.RemovalPolicyExpectNoRemoval
end

-- A registry property of a device; nil when the device does not have it (the
-- system's STATUS_OBJECT_NAME_NOT_FOUND, or no bytes), nil and the status when
-- it cannot be read.
local function registry_list(device, id)
  local bytes, failed = ksi.hw.device_property(device, id)
  if not bytes then
    if failed == M.STATUS_OBJECT_NAME_NOT_FOUND then
      return nil
    end
    return nil, failed
  end
  if #bytes == 0 then
    return nil
  end
  return bytes
end

-- The memory and port descriptors (types 3, 7 and 1) of a CM_RESOURCE_LIST in
-- bytes first to last, in order, device-specific data (type 5) stepped over;
-- and the list's size, or 0 when it is cut short.
local function cm_descriptors(bytes, first, last)
  local found = {}
  if last - first + 1 < 4 then
    return found, 0
  end
  local at = first + 4
  for _ = 1, string.unpack('<I4', bytes, first) do
    if at + 15 > last then
      return found, 0
    end
    local count = string.unpack('<I4', bytes, at + 12)
    at = at + 16
    for _ = 1, count do
      if at + 19 > last then
        return found, 0
      end
      local descriptor = at
      local kind = string.byte(bytes, descriptor)
      at = at + 20
      if kind == 5 then
        at = at + string.unpack('<I4', bytes, descriptor + 4)
        if at - 1 > last then
          return found, 0
        end
      elseif kind == 1 or kind == 3 or kind == 7 then
        found[#found + 1] = descriptor
      end
    end
  end
  return found, at - first
end

-- The length of a CM_PARTIAL_RESOURCE_DESCRIPTOR, as RtlCmDecodeMemIoResource
-- gives it.
local function cm_length(bytes, at)
  local kind, flags = string.byte(bytes, at), string.unpack('<I2', bytes, at + 2)
  local length = string.unpack('<I4', bytes, at + 12)
  if kind == 1 or kind == 3 then
    return length
  elseif flags & 0x200 ~= 0 then
    return length << 8
  elseif flags & 0x400 ~= 0 then
    return length << 16
  elseif flags & 0x800 ~= 0 then
    return length << 32
  end
  return 0
end

-- The memory and port resources of a device: those the PnP manager assigned,
-- or when it assigned none, the boot configuration. Each is { kind (1 memory,
-- 2 port), raw, translated, length, flags }; nil and a status on failure.
function M.resources(device)
  local raw, rawLast, translated, translatedFirst, translatedLast
  local allocated, failed = registry_list(device, M.DevicePropertyAllocatedResources)
  if allocated then
    -- the raw list is followed by the translated list
    local _, size = cm_descriptors(allocated, 1, #allocated)
    if size == 0 or size >= #allocated then
      return nil, M.STATUS_DEVICE_CONFIGURATION_ERROR
    end
    raw, rawLast = allocated, size
    translated, translatedFirst, translatedLast = allocated, size + 1, #allocated
  elseif failed then
    return nil, failed
  else
    raw, failed = registry_list(device, M.DevicePropertyBootConfiguration)
    if failed then
      return nil, failed
    end
    if raw then
      rawLast = #raw
      translated = registry_list(device, M.DevicePropertyBootConfigurationTranslated)
      if translated then
        translatedFirst, translatedLast = 1, #translated
      end
    end
  end
  local list = {}
  if not raw then
    return list
  end
  local paired = translated and cm_descriptors(translated, translatedFirst, translatedLast) or {}
  for i, at in ipairs((cm_descriptors(raw, 1, rawLast))) do
    local other, otherAt = translated, paired[i]
    if not otherAt then
      other, otherAt = raw, at
    end
    list[i] = {
      kind = string.byte(raw, at) == 1 and 2 or 1,
      raw = string.unpack('<i8', raw, at + 4),
      translated = string.unpack('<i8', other, otherAt + 4),
      length = cm_length(raw, at),
      flags = string.unpack('<I2', raw, at + 2),
    }
  end
  return list
end

-- The memory requirements of a device, each { minimum, maximum, length,
-- alignment, flags } as RtlIoDecodeMemIoResource gives them, those of length 0
-- left out; nil and a status on failure (STATUS_NOT_FOUND for a list of no
-- bytes).
function M.requirements(device)
  local bytes, failed = ksi.hw.device_property(device, M.DevicePropertyResourceRequirements)
  if not bytes then
    return nil, failed
  end
  if #bytes == 0 then
    return nil, M.STATUS_NOT_FOUND
  end
  if #bytes < 32 then
    return nil, M.STATUS_DEVICE_CONFIGURATION_ERROR
  end
  local list = {}
  local at = 33
  for _ = 1, string.unpack('<I4', bytes, 29) do
    if at + 7 > #bytes then
      return nil, M.STATUS_DEVICE_CONFIGURATION_ERROR
    end
    local count = string.unpack('<I4', bytes, at + 4)
    local descriptors = at + 8
    at = descriptors + count * 32
    if at - 1 > #bytes then
      return nil, M.STATUS_DEVICE_CONFIGURATION_ERROR
    end
    for j = 0, count - 1 do
      local descriptor = descriptors + j * 32
      local kind = string.byte(bytes, descriptor + 1)
      if kind == 3 or kind == 7 then
        local flags = string.unpack('<I2', bytes, descriptor + 4)
        local length, alignment, minimum, maximum =
          string.unpack('<I4I4i8i8', bytes, descriptor + 8)
        if kind == 7 then
          local shift = (flags & 0x200 ~= 0 and 8)
            or (flags & 0x400 ~= 0 and 16)
            or (flags & 0x800 ~= 0 and 32)
          length = shift and length << shift or 0
          alignment = shift and alignment << shift or 0
        end
        if length ~= 0 then
          list[#list + 1] = {
            minimum = minimum,
            maximum = maximum,
            length = length,
            alignment = alignment,
            flags = flags,
          }
        end
      end
    end
  end
  return list
end

-- Whether first to last (exclusive) overlaps a range of ranges, each its base
-- and length as two unsigned 64-bit values; addresses compare unsigned, an end
-- past 2^64 is 2^64 - 1.
local function overlaps(ranges, first, last)
  for at = 1, #ranges - 15, 16 do
    local base, length = string.unpack('<I8I8', ranges, at)
    local stop = base + length
    if math.ult(stop, base) then
      stop = -1
    end
    if math.ult(first, stop) and math.ult(base, last) then
      return true
    end
  end
  return false
end

-- The end of base and length, 2^64 - 1 when it is past 2^64.
local function range_end(base, length)
  local stop = base + length
  if math.ult(stop, base) then
    return -1
  end
  return stop
end

-- Whether a physical range overlaps memory the operating system owns; nil and
-- a status when the ranges cannot be had.
function M.is_ram(base, length)
  local ranges, failed = ksi.hw.memory_ranges()
  if not ranges then
    return nil, failed
  end
  return overlaps(ranges, base, range_end(base, length))
end

-- The system physical address ranges of the NFIT, as overlaps takes them;
-- empty when there is no NFIT, nil and a status when it cannot be read or is
-- malformed.
local nfit

local function nfit_ranges()
  if nfit then
    return nfit
  end
  local bytes, failed = ksi.hw.firmware_table('ACPI', 'NFIT')
  if not bytes then
    if failed ~= M.STATUS_NOT_FOUND then
      return nil, failed
    end
    nfit = ''
    return nfit
  end
  local length = #bytes
  if length < 36 or string.unpack('<I4', bytes, 5) ~= length or length < 40 then
    return nil, M.STATUS_ACPI_INVALID_TABLE
  end
  local found = {}
  local at = 41
  while at <= length do
    if length - at + 1 < 4 then
      return nil, M.STATUS_ACPI_INVALID_TABLE
    end
    local kind, size = string.unpack('<I2I2', bytes, at)
    if size < 4 or size > length - at + 1 then
      return nil, M.STATUS_ACPI_INVALID_TABLE
    end
    if kind == 0 then
      if size < 56 then
        return nil, M.STATUS_ACPI_INVALID_TABLE
      end
      local base, span = string.unpack('<I8I8', bytes, at + 32)
      if math.ult(base + span, base) then
        return nil, M.STATUS_ACPI_INVALID_TABLE
      end
      found[#found + 1] = string.pack('<I8I8', base, span)
    end
    at = at + size
  end
  nfit = table.concat(found)
  return nfit
end

-- Whether a physical range overlaps memory the firmware describes as
-- non-volatile; nil and a status when the NFIT cannot be read or is malformed.
function M.is_nvdimm(base, length)
  local ranges, failed = nfit_ranges()
  if not ranges then
    return nil, failed
  end
  return overlaps(ranges, base, range_end(base, length))
end

-- Whether a script may map first to last: STATUS_SUCCESS, or the refusal (an
-- end beyond the address bits, non-volatile memory or memory the operating
-- system owns, or not knowing).
function M.check_range(first, last, bits)
  if math.ult(1 << bits, last) then
    ksi.trace('range end 0x%x beyond %d address bits', last, bits)
    return M.STATUS_CONFLICTING_ADDRESSES
  end
  local nvdimm = M.is_nvdimm(first, last - first)
  if nvdimm == nil then
    ksi.trace('NVDIMM ranges unknown')
    return M.STATUS_DEVICE_CONFIGURATION_ERROR
  end
  if nvdimm then
    ksi.trace('range 0x%x-0x%x overlaps NVDIMM', first, last)
    return M.STATUS_CONFLICTING_ADDRESSES
  end
  local ram, ramFailed = M.is_ram(first, last - first)
  if ram == nil then
    ksi.trace('memory ranges unknown: 0x%x', ramFailed)
    return ramFailed
  end
  if ram then
    ksi.trace('range 0x%x-0x%x overlaps memory', first, last)
    return M.STATUS_CONFLICTING_ADDRESSES
  end
  return M.STATUS_SUCCESS
end

M.pci = require('pci')(M)
M.acpi = require('acpi')(M)

return M
