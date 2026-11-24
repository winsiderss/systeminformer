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
-- SPB functions: transfers to a target on a simple peripheral bus (I2C, SPI)
-- through the resource hub's file of its connection (nt.connection_open).
-- Reached as nt.spb.
--

return function(nt)
  local M = {}

  M.IOCTL_SPB_LOCK_CONTROLLER = 0x00041800
  M.IOCTL_SPB_UNLOCK_CONTROLLER = 0x00041804
  M.IOCTL_SPB_EXECUTE_SEQUENCE = 0x00041808
  M.IOCTL_SPB_LOCK_CONNECTION = 0x0004180C
  M.IOCTL_SPB_UNLOCK_CONNECTION = 0x00041810
  M.IOCTL_SPB_FULL_DUPLEX = 0x00041814

  -- 64-bit SPB_TRANSFER_LIST layout: header, entry, entry's pointer and length
  local SPB_HEADER = 16
  local SPB_ENTRY = 32
  local SPB_POINTER = 16
  local SPB_LENGTH = 24
  local SPB_FROM_DEVICE = 1
  local SPB_TO_DEVICE = 2
  local SPB_SIMPLE_NON_PAGED = 3

  -- Sends { write = bytes } and { read = length, delay = us } transfers as one
  -- request; the reads, or nil and a status.
  function M.sequence(device, transfers, milliseconds, code)
    local parts = { string.pack('<I4I4I4I4', SPB_HEADER + SPB_ENTRY, 0, #transfers, 0) }
    local buffers = {}
    for i, t in ipairs(transfers) do
      local direction = t.read and SPB_FROM_DEVICE or SPB_TO_DEVICE
      parts[#parts + 1] =
        string.pack('<I4I4I4I4I8I4I4', direction, t.delay or 0, SPB_SIMPLE_NON_PAGED, 0, 0, 0, 0)
      buffers[i] = {
        bytes = t.write,
        length = t.read,
        at = SPB_HEADER + (i - 1) * SPB_ENTRY + SPB_POINTER,
        length_at = SPB_HEADER + (i - 1) * SPB_ENTRY + SPB_LENGTH,
      }
    end
    local output, second = ksi.hw.device_control(
      device,
      code or M.IOCTL_SPB_EXECUTE_SEQUENCE,
      table.concat(parts),
      0,
      milliseconds,
      { buffers = buffers }
    )
    if not output then
      return nil, second
    end
    local reads = {}
    for i, t in ipairs(transfers) do
      if t.read then
        reads[#reads + 1] = second[i]
      end
    end
    return reads
  end

  -- Reads and writes until unlock are one bus operation; not every controller
  -- supports it (STATUS_NOT_SUPPORTED). Releasing the device does not unlock.
  function M.lock(device, milliseconds)
    local output, failed =
      ksi.hw.device_control(device, M.IOCTL_SPB_LOCK_CONTROLLER, '', 0, milliseconds)
    if not output then
      return failed
    end
    return nt.STATUS_SUCCESS
  end

  function M.unlock(device, milliseconds)
    local output, failed =
      ksi.hw.device_control(device, M.IOCTL_SPB_UNLOCK_CONTROLLER, '', 0, milliseconds)
    if not output then
      return failed
    end
    return nt.STATUS_SUCCESS
  end

  return M
end
