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

M.DEVPROP_TYPE_UINT32 = 0x00000007
M.DEVPROP_TYPE_STRING = 0x00000012

M.DevicePropertyBootConfiguration = 0x3
M.DevicePropertyBootConfigurationTranslated = 0x4
M.DevicePropertyResourceRequirements = 0x14
M.DevicePropertyAllocatedResources = 0x15

M.RemovalPolicyExpectNoRemoval = 1
M.RemovalPolicyExpectOrderlyRemoval = 2
M.RemovalPolicyExpectSurpriseRemoval = 3

-- Whether a device may go away while a script holds it: true unless the system
-- expects it never to be removed, and true when its policy cannot be read.
function M.is_device_removable(device)
  local bytes, kind = ksi.hw.device_property_data(device, M.DEVPKEY_Device_RemovalPolicy)
  if not bytes or kind ~= M.DEVPROP_TYPE_UINT32 or #bytes ~= 4 then
    return true
  end
  return string.unpack('<I4', bytes) ~= M.RemovalPolicyExpectNoRemoval
end

M.pci = require('pci')(M)
M.acpi = require('acpi')(M)

return M
