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
-- Talks to the AMD HSMP mailbox behind the root complex's system management
-- network pair. The root complexes, their mailbox registers and the commands
-- each processor family may be sent are listed below.
--

local concat, pack, unpack = table.concat, table.pack, table.unpack

local ROWS = {
    {
        vendor = "AuthenticAMD",
        family = 0x19,
        models = { { 0x00, 0x0f } },
        pci = { 0x1022, 0x1480 },
        pin = { 0, 0 },
        mailbox = function()
            return {
                index = 0xc4,
                data = 0xc8,
                msg = 0x03b10534,
                rsp = 0x03b10980,
                arg = 0x03b109e0,
                args = 8,
                version = 0x02,
                version_arg = 0x0,
                commands = {
                    [0x01] = { words = 1, mask = { 0xffffffff } },
                    [0x02] = { words = 1 },
                    [0x03] = { words = 1 },
                    [0x04] = { words = 1 },
                    [0x06] = { words = 1 },
                    [0x07] = { words = 1 },
                    [0x0a] = { words = 1, mask = { 0xffff } },
                    [0x0b] = { words = 1 },
                    [0x0f] = { words = 2 },
                    [0x10] = { words = 1 },
                    [0x11] = { words = 1 },
                    [0x13] = { words = 1, mask = { 0xff0000 } },
                    [0x14] = { words = 1 },
                    [0x15] = { words = 1 },
                    [0x16] = { words = 1, mask = { 0xff } },
                    [0x17] = { words = 1, mask = { 0xff } },
                    [0x18] = { words = 1, mask = { 0xff } },
                    [0x19] = { words = 1 },
                    [0x1a] = { words = 1, mask = { 0xffffffff } },
                    [0x1b] = { words = 1 },
                    [0x1c] = { words = 1 },
                    [0x1d] = { words = 1, mask = { 0xff07 } },
                    [0x1e] = { words = 1, mask = { 0xff07 } },
                    [0x23] = { words = 1 },
                    [0x30] = { words = 1 },
                    [0x31] = { words = 2, mask = { 0xffff } },
                    [0x32] = { words = 2 },
                    [0x34] = { words = 1, mask = { 0xffff } },
                    [0x35] = { words = 1 },
                    [0x36] = { words = 1, mask = { 0xf } },
                    [0x37] = { words = 3, mask = { 0x1 } },
                },
            }
        end,
    },
    {
        vendor = "AuthenticAMD",
        family = 0x19,
        models = { { 0x30, 0x3f } },
        pci = { 0x1022, 0x1480 },
        pin = { 0, 0 },
        mailbox = function()
            return {
                index = 0xc4,
                data = 0xc8,
                msg = 0x03b10534,
                rsp = 0x03b10980,
                arg = 0x03b109e0,
                args = 8,
                version = 0x02,
                version_arg = 0x0,
                commands = {
                    [0x01] = { words = 1, mask = { 0xffffffff } },
                    [0x02] = { words = 1 },
                    [0x03] = { words = 1 },
                    [0x04] = { words = 1 },
                    [0x06] = { words = 1 },
                    [0x07] = { words = 1 },
                    [0x0a] = { words = 1, mask = { 0xffff } },
                    [0x0b] = { words = 1 },
                    [0x0f] = { words = 2 },
                    [0x10] = { words = 1 },
                    [0x11] = { words = 1 },
                    [0x13] = { words = 1, mask = { 0xff0000 } },
                    [0x14] = { words = 1 },
                    [0x15] = { words = 1 },
                    [0x16] = { words = 1, mask = { 0xff } },
                    [0x17] = { words = 1, mask = { 0xff } },
                    [0x18] = { words = 1, mask = { 0xff } },
                    [0x19] = { words = 1 },
                    [0x1a] = { words = 1, mask = { 0xffffffff } },
                    [0x1b] = { words = 1 },
                    [0x1c] = { words = 1 },
                    [0x1d] = { words = 1, mask = { 0xff07 } },
                    [0x1e] = { words = 1, mask = { 0xff07 } },
                    [0x23] = { words = 1 },
                    [0x30] = { words = 1 },
                    [0x31] = { words = 2, mask = { 0xffff } },
                    [0x32] = { words = 2 },
                    [0x34] = { words = 1, mask = { 0xffff } },
                    [0x35] = { words = 1 },
                    [0x36] = { words = 1, mask = { 0xf } },
                    [0x37] = { words = 3, mask = { 0x1 } },
                },
            }
        end,
    },
    {
        vendor = "AuthenticAMD",
        family = 0x19,
        models = { { 0x10, 0x17 }, { 0x19, 0x1f } },
        pci = { 0x1022, 0x14a4 },
        pin = { 0, 0 },
        mailbox = function()
            return {
                index = 0xc4,
                data = 0xc8,
                msg = 0x03b10534,
                rsp = 0x03b10980,
                arg = 0x03b109e0,
                args = 8,
                version = 0x02,
                version_arg = 0x0,
                commands = {
                    [0x01] = { words = 1, mask = { 0xffffffff } },
                    [0x02] = { words = 1 },
                    [0x03] = { words = 1 },
                    [0x04] = { words = 1 },
                    [0x06] = { words = 1 },
                    [0x07] = { words = 1 },
                    [0x0a] = { words = 1, mask = { 0xffff } },
                    [0x0b] = { words = 1 },
                    [0x0f] = { words = 2 },
                    [0x10] = { words = 1 },
                    [0x11] = { words = 1 },
                    [0x13] = { words = 1, mask = { 0xff0000 } },
                    [0x14] = { words = 1 },
                    [0x15] = { words = 1 },
                    [0x16] = { words = 1, mask = { 0xff } },
                    [0x17] = { words = 1, mask = { 0xff } },
                    [0x18] = { words = 1, mask = { 0xff } },
                    [0x19] = { words = 1 },
                    [0x1a] = { words = 1, mask = { 0xffffffff } },
                    [0x1b] = { words = 1 },
                    [0x1c] = { words = 1 },
                    [0x1d] = { words = 1, mask = { 0xff07 } },
                    [0x1e] = { words = 1, mask = { 0xff07 } },
                    [0x23] = { words = 1 },
                    [0x30] = { words = 1 },
                    [0x31] = { words = 2, mask = { 0xffff } },
                    [0x32] = { words = 2 },
                    [0x34] = { words = 1, mask = { 0xffff } },
                    [0x35] = { words = 1 },
                    [0x36] = { words = 1, mask = { 0xf } },
                    [0x37] = { words = 3, mask = { 0x1 } },
                },
            }
        end,
    },
    {
        vendor = "AuthenticAMD",
        family = 0x19,
        models = { { 0x90, 0x9f } },
        pci = { 0x1022, 0x14f8 },
        pin = { 0, 0 },
        mailbox = function()
            return {
                index = 0xc4,
                data = 0xc8,
                msg = 0x03b10534,
                rsp = 0x03b10980,
                arg = 0x03b109e0,
                args = 8,
                version = 0x02,
                version_arg = 0x0,
                commands = {
                    [0x01] = { words = 1, mask = { 0xffffffff } },
                    [0x02] = { words = 1 },
                    [0x03] = { words = 1 },
                    [0x04] = { words = 1 },
                    [0x06] = { words = 1 },
                    [0x07] = { words = 1 },
                    [0x0a] = { words = 1, mask = { 0xffff } },
                    [0x0b] = { words = 1 },
                    [0x0f] = { words = 2 },
                    [0x10] = { words = 1 },
                    [0x11] = { words = 1 },
                    [0x13] = { words = 1, mask = { 0xff0000 } },
                    [0x14] = { words = 1 },
                    [0x15] = { words = 1 },
                    [0x16] = { words = 1, mask = { 0xff } },
                    [0x17] = { words = 1, mask = { 0xff } },
                    [0x18] = { words = 1, mask = { 0xff } },
                    [0x19] = { words = 1 },
                    [0x1a] = { words = 1, mask = { 0xffffffff } },
                    [0x1b] = { words = 1 },
                    [0x1c] = { words = 1 },
                    [0x1d] = { words = 1, mask = { 0xff07 } },
                    [0x1e] = { words = 1, mask = { 0xff07 } },
                    [0x23] = { words = 1 },
                    [0x30] = { words = 1 },
                    [0x31] = { words = 2, mask = { 0xffff } },
                    [0x32] = { words = 2 },
                    [0x34] = { words = 1, mask = { 0xffff } },
                    [0x35] = { words = 1 },
                    [0x36] = { words = 1, mask = { 0xf } },
                    [0x37] = { words = 3, mask = { 0x1 } },
                },
            }
        end,
    },
    {
        vendor = "AuthenticAMD",
        family = 0x19,
        models = { { 0x18, 0x18 } },
        pci = { 0x1022, 0x14a4 },
        pin = { 0, 0 },
        mailbox = function()
            return {
                index = 0xc4,
                data = 0xc8,
                msg = 0x03b10534,
                rsp = 0x03b10980,
                arg = 0x03b109e0,
                args = 8,
                version = 0x02,
                version_arg = 0x0,
                commands = {
                    [0x01] = { words = 1, mask = { 0xffffffff } },
                    [0x02] = { words = 1 },
                    [0x03] = { words = 1 },
                    [0x04] = { words = 1 },
                    [0x06] = { words = 1 },
                    [0x07] = { words = 1 },
                    [0x0a] = { words = 1, mask = { 0xffff } },
                    [0x0b] = { words = 1 },
                    [0x0f] = { words = 2 },
                    [0x10] = { words = 1 },
                    [0x11] = { words = 1 },
                    [0x13] = { words = 1, mask = { 0xff0000 } },
                    [0x14] = { words = 1 },
                    [0x15] = { words = 1 },
                    [0x16] = { words = 1, mask = { 0xff } },
                    [0x17] = { words = 1, mask = { 0xff } },
                    [0x18] = { words = 1, mask = { 0xff } },
                    [0x19] = { words = 1 },
                    [0x1a] = { words = 1, mask = { 0xffffffff } },
                    [0x1b] = { words = 1 },
                    [0x1c] = { words = 1 },
                    [0x1d] = { words = 1, mask = { 0xff07 } },
                    [0x1e] = { words = 1, mask = { 0xff07 } },
                    [0x23] = { words = 1 },
                    [0x30] = { words = 1 },
                    [0x31] = { words = 2, mask = { 0xffff } },
                    [0x32] = { words = 2 },
                    [0x34] = { words = 1, mask = { 0xffff } },
                    [0x35] = { words = 1 },
                    [0x36] = { words = 1, mask = { 0xf } },
                    [0x37] = { words = 3, mask = { 0x1 } },
                },
            }
        end,
    },
    {
        vendor = "AuthenticAMD",
        family = 0x19,
        models = { { 0xa0, 0xaf } },
        class = { 0x1022, 0x060000, 0x00 },
        pin = { 0, 0 },
        mailbox = function()
            return {
                index = 0xc4,
                data = 0xc8,
                msg = 0x03b10534,
                rsp = 0x03b10980,
                arg = 0x03b109e0,
                args = 8,
                version = 0x02,
                version_arg = 0x0,
                commands = {
                    [0x01] = { words = 1, mask = { 0xffffffff } },
                    [0x02] = { words = 1 },
                    [0x03] = { words = 1 },
                    [0x04] = { words = 1 },
                    [0x06] = { words = 1 },
                    [0x07] = { words = 1 },
                    [0x0a] = { words = 1, mask = { 0xffff } },
                    [0x0b] = { words = 1 },
                    [0x0f] = { words = 2 },
                    [0x10] = { words = 1 },
                    [0x11] = { words = 1 },
                    [0x13] = { words = 1, mask = { 0xff0000 } },
                    [0x14] = { words = 1 },
                    [0x15] = { words = 1 },
                    [0x16] = { words = 1, mask = { 0xff } },
                    [0x17] = { words = 1, mask = { 0xff } },
                    [0x18] = { words = 1, mask = { 0xff } },
                    [0x19] = { words = 1 },
                    [0x1a] = { words = 1, mask = { 0xffffffff } },
                    [0x1b] = { words = 1 },
                    [0x1c] = { words = 1 },
                    [0x1d] = { words = 1, mask = { 0xff07 } },
                    [0x1e] = { words = 1, mask = { 0xff07 } },
                    [0x23] = { words = 1 },
                    [0x30] = { words = 1 },
                    [0x31] = { words = 2, mask = { 0xffff } },
                    [0x32] = { words = 2 },
                    [0x34] = { words = 1, mask = { 0xffff } },
                    [0x35] = { words = 1 },
                    [0x36] = { words = 1, mask = { 0xf } },
                    [0x37] = { words = 3, mask = { 0x1 } },
                },
            }
        end,
    },
    {
        vendor = "AuthenticAMD",
        family = 0x1a,
        models = { { 0x00, 0x07 }, { 0x09, 0x0f } },
        pci = { 0x1022, 0x153a },
        pin = { 0, 0 },
        mailbox = function()
            return {
                index = 0xc4,
                data = 0xc8,
                msg = 0x03b10934,
                rsp = 0x03b10980,
                arg = 0x03b109e0,
                args = 8,
                version = 0x02,
                version_arg = 0x0,
                commands = {
                    [0x01] = { words = 1, mask = { 0xffffffff } },
                    [0x02] = { words = 1 },
                    [0x03] = { words = 1 },
                    [0x04] = { words = 1 },
                    [0x06] = { words = 1 },
                    [0x07] = { words = 1 },
                    [0x0a] = { words = 1, mask = { 0xffff } },
                    [0x0b] = { words = 1 },
                    [0x0f] = { words = 2 },
                    [0x10] = { words = 1 },
                    [0x11] = { words = 1 },
                    [0x13] = { words = 1, mask = { 0xff0000 } },
                    [0x14] = { words = 1 },
                    [0x15] = { words = 1 },
                    [0x16] = { words = 1, mask = { 0xff } },
                    [0x17] = { words = 1, mask = { 0xff } },
                    [0x18] = { words = 1, mask = { 0xff } },
                    [0x19] = { words = 1 },
                    [0x1a] = { words = 1, mask = { 0xffffffff } },
                    [0x1b] = { words = 1 },
                    [0x1c] = { words = 1 },
                    [0x1d] = { words = 1, mask = { 0xff07 } },
                    [0x1e] = { words = 1, mask = { 0xff07 } },
                    [0x23] = { words = 1 },
                    [0x30] = { words = 1 },
                    [0x31] = { words = 2, mask = { 0xffff } },
                    [0x32] = { words = 2 },
                    [0x34] = { words = 1, mask = { 0xffff } },
                    [0x35] = { words = 1 },
                    [0x36] = { words = 1, mask = { 0xf } },
                    [0x37] = { words = 3, mask = { 0x1 } },
                },
            }
        end,
    },
    {
        vendor = "AuthenticAMD",
        family = 0x1a,
        models = { { 0x08, 0x08 } },
        pci = { 0x1022, 0x153a },
        pin = { 0, 0 },
        mailbox = function()
            return {
                index = 0xc4,
                data = 0xc8,
                msg = 0x03b10934,
                rsp = 0x03b10980,
                arg = 0x03b109e0,
                args = 8,
                version = 0x02,
                version_arg = 0x0,
                commands = {
                    [0x01] = { words = 1, mask = { 0xffffffff } },
                    [0x02] = { words = 1 },
                    [0x03] = { words = 1 },
                    [0x04] = { words = 1 },
                    [0x06] = { words = 1 },
                    [0x07] = { words = 1 },
                    [0x0a] = { words = 1, mask = { 0xffff } },
                    [0x0b] = { words = 1 },
                    [0x0f] = { words = 2 },
                    [0x10] = { words = 1 },
                    [0x11] = { words = 1 },
                    [0x13] = { words = 1, mask = { 0xff0000 } },
                    [0x14] = { words = 1 },
                    [0x15] = { words = 1 },
                    [0x16] = { words = 1, mask = { 0xff } },
                    [0x17] = { words = 1, mask = { 0xff } },
                    [0x18] = { words = 1, mask = { 0xff } },
                    [0x19] = { words = 1 },
                    [0x1a] = { words = 1, mask = { 0xffffffff } },
                    [0x1b] = { words = 1 },
                    [0x1c] = { words = 1 },
                    [0x1d] = { words = 1, mask = { 0xff07 } },
                    [0x1e] = { words = 1, mask = { 0xff07 } },
                    [0x23] = { words = 1 },
                    [0x30] = { words = 1 },
                    [0x31] = { words = 2, mask = { 0xffff } },
                    [0x32] = { words = 2 },
                    [0x34] = { words = 1, mask = { 0xffff } },
                    [0x35] = { words = 1 },
                    [0x36] = { words = 1, mask = { 0xf } },
                    [0x37] = { words = 3, mask = { 0x1 } },
                },
            }
        end,
    },
    {
        vendor = "AuthenticAMD",
        family = 0x1a,
        models = { { 0x10, 0x1f } },
        pci = { 0x1022, 0x153a },
        pin = { 0, 0 },
        mailbox = function()
            return {
                index = 0xc4,
                data = 0xc8,
                msg = 0x03b10934,
                rsp = 0x03b10980,
                arg = 0x03b109e0,
                args = 8,
                version = 0x02,
                version_arg = 0x0,
                commands = {
                    [0x01] = { words = 1, mask = { 0xffffffff } },
                    [0x02] = { words = 1 },
                    [0x03] = { words = 1 },
                    [0x04] = { words = 1 },
                    [0x06] = { words = 1 },
                    [0x07] = { words = 1 },
                    [0x0a] = { words = 1, mask = { 0xffff } },
                    [0x0b] = { words = 1 },
                    [0x0f] = { words = 2 },
                    [0x10] = { words = 1 },
                    [0x11] = { words = 1 },
                    [0x13] = { words = 1, mask = { 0xff0000 } },
                    [0x14] = { words = 1 },
                    [0x15] = { words = 1 },
                    [0x16] = { words = 1, mask = { 0xff } },
                    [0x17] = { words = 1, mask = { 0xff } },
                    [0x18] = { words = 1, mask = { 0xff } },
                    [0x19] = { words = 1 },
                    [0x1a] = { words = 1, mask = { 0xffffffff } },
                    [0x1b] = { words = 1 },
                    [0x1c] = { words = 1 },
                    [0x1d] = { words = 1, mask = { 0xff07 } },
                    [0x1e] = { words = 1, mask = { 0xff07 } },
                    [0x23] = { words = 1 },
                    [0x30] = { words = 1 },
                    [0x31] = { words = 2, mask = { 0xffff } },
                    [0x32] = { words = 2 },
                    [0x34] = { words = 1, mask = { 0xffff } },
                    [0x35] = { words = 1 },
                    [0x36] = { words = 1, mask = { 0xf } },
                    [0x37] = { words = 3, mask = { 0x1 } },
                },
            }
        end,
    },
}

local WAIT_MS = 1000
local STALE_MS = 100
local RESTORE_TRIES = 1000
local ALL_ONES = 0xffffffff
local ARGS = 8

local kept = {}

-- The header bytes a function is identified by.
local function header_key(header)
    return string.sub(header, 1, 4) .. string.sub(header, 9, 12) ..
           string.sub(header, 15, 15)
end

local function cpu_identity()
    if kept.cpu then
        return kept.cpu
    end
    local items = string.pack("<I2I2I4I4", 0, 0, 0, 0) ..
                  string.pack("<I2I2I4I4", 0, 0, 1, 0)
    local results, status = ksi.hw.cpuid(items)
    if not results then
        ksi.trace("cpuid failed: 0x%x", status)
        return nil, status
    end
    local s0, _, b, c, d = string.unpack("<I4I4I4I4I4", results, 1)
    local s1, signature = string.unpack("<I4I4", results, 21)
    if s0 ~= ksi.STATUS_SUCCESS or s1 ~= ksi.STATUS_SUCCESS then
        ksi.trace("CPUID of processor 0:0 failed: 0x%x 0x%x", s0, s1)
        return nil, s0 ~= ksi.STATUS_SUCCESS and s0 or s1
    end
    local base = (signature >> 8) & 0xf
    local family = base
    if base == 0xf then
        family = family + ((signature >> 20) & 0xff)
    end
    local model = (signature >> 4) & 0xf
    if base >= 0x6 then
        model = model | (((signature >> 16) & 0xf) << 4)
    end
    kept.cpu = { vendor = string.pack("<I4I4I4", b, d, c), family = family,
                 model = model }
    return kept.cpu
end

local function cpu_matches(row, cpu)
    if row.vendor ~= cpu.vendor or row.family ~= cpu.family then
        return false
    end
    for _, range in ipairs(row.models) do
        if cpu.model >= range[1] and cpu.model <= range[2] then
            return true
        end
    end
    return false
end

local function find_row(cpu, root)
    for _, row in ipairs(ROWS) do
        if row.pci and cpu_matches(row, cpu) and row.pci[1] == root.vendor and
           row.pci[2] == root.device then
            return row
        end
    end
    for _, row in ipairs(ROWS) do
        if row.class and cpu_matches(row, cpu) and
           row.class[1] == root.vendor and row.class[2] == root.class and
           row.class[3] == root.header and row.pin[1] == root.slot and
           row.pin[2] == root.fn then
            return row
        end
    end
    return nil
end

local function session(input)
    if not ksi.hw.pci_open or not ksi.hw.cpuid then
        ksi.trace("no PCI or CPUID functions")
        return nil, ksi.STATUS_NOT_SUPPORTED
    end

    local locator = string.unpack("<I4", input)
    local cpu, failed = cpu_identity()
    if not cpu then
        return nil, failed
    end

    local root = {
        segment = locator >> 16,
        bus = (locator >> 8) & 0xff,
        slot = (locator >> 3) & 0x1f,
        fn = locator & 0x7,
    }

    local k = kept[locator]
    if k then
        local header = ksi.hw.pci_read_block(k.device, 0, 16)
        if header and header_key(header) == k.key then
            return { device = k.device, kept = k, mailbox = k.mailbox,
                     root = k.root }
        end
        ksi.trace("the function kept for 0x%x changed", locator)
        kept[locator] = nil
        k.device:release()
    end

    local device, status = ksi.hw.pci_open(root.segment, root.bus, root.slot,
                                           root.fn)
    if not device then
        ksi.trace("no PCI function at 0x%x: 0x%x", locator, status)
        return nil, status
    end

    local header, readFailed = ksi.hw.pci_read_block(device, 0, 16)
    if not header then
        ksi.trace("configuration header of 0x%x not read: 0x%x", locator,
                  readFailed)
        device:release()
        return nil, readFailed
    end
    root.vendor, root.device = string.unpack("<I2I2", header, 1)
    root.class = string.unpack("<I4", header, 9) >> 8
    root.header = string.byte(header, 15) & 0x7f

    local row = find_row(cpu, root)
    if not row then
        ksi.trace("no row for %s family 0x%x model 0x%x and PCI %04x:%04x",
                  cpu.vendor, cpu.family, cpu.model, root.vendor, root.device)
        device:release()
        return nil, ksi.STATUS_NOT_SUPPORTED
    end
    if row.pin[1] ~= root.slot or row.pin[2] ~= root.fn then
        ksi.trace("the mailbox is pinned to %d.%d", row.pin[1], row.pin[2])
        device:release()
        return nil, ksi.STATUS_ACCESS_DENIED
    end

    local s = { device = device, mailbox = row.mailbox(), root = root }
    if not ksi.volatile and ksi.hw.pci_fixed(device) then
        s.kept = { device = device, key = header_key(header), root = root,
                   mailbox = s.mailbox }
        kept[locator] = s.kept
    end
    return s
end

local function poll(every, spins, sleep, timeout, body)
    local limit = ksi.time() + timeout * 10000
    local round = 0
    while true do
        local late = ksi.time() >= limit
        if body(late) or late then
            return
        end
        if round < spins or sleep == 0 then
            ksi.stall(every)
        else
            ksi.sleep(sleep)
        end
        round = round + 1
    end
end

local function smn_read(s, address)
    local m = s.mailbox
    for _ = 1, 2 do
        if not ksi.hw.pci_write(s.device, m.index, 4, address) then
            ksi.trace("index write for SMN 0x%x failed", address)
            return ksi.STATUS_DEVICE_NOT_READY, 0
        end
        local value = ksi.hw.pci_read(s.device, m.data, 4)
        if not value then
            ksi.trace("data read of SMN 0x%x failed", address)
            return ksi.STATUS_DEVICE_NOT_READY, 0
        end
        local check = ksi.hw.pci_read(s.device, m.index, 4)
        if not check then
            ksi.trace("index read for SMN 0x%x failed", address)
            return ksi.STATUS_DEVICE_NOT_READY, 0
        end
        if check == address then
            return ksi.STATUS_SUCCESS, value
        end
    end
    ksi.trace("the index of SMN 0x%x moved", address)
    return ksi.STATUS_RETRY, 0
end

local function smn_write(s, address, value, before, after)
    local m = s.mailbox
    if not ksi.hw.pci_write(s.device, m.index, 4, address) then
        ksi.trace("index write for SMN 0x%x failed", address)
        return ksi.STATUS_RETRY
    end
    local check = ksi.hw.pci_read(s.device, m.index, 4)
    if not check then
        ksi.trace("index read for SMN 0x%x failed", address)
        return ksi.STATUS_RETRY
    end
    if check ~= address then
        ksi.trace("the index of SMN 0x%x is not ours before the write", address)
        return ksi.STATUS_RETRY
    end
    if before then
        before()
    end
    local written = ksi.hw.pci_write(s.device, m.data, 4, value)
    if after then
        after()
    end
    if not written then
        ksi.trace("data write of SMN 0x%x failed", address)
        return ksi.STATUS_DEVICE_NOT_READY
    end
    check = ksi.hw.pci_read(s.device, m.index, 4)
    if not check then
        ksi.trace("index read for SMN 0x%x failed", address)
        return ksi.STATUS_DEVICE_NOT_READY
    end
    if check ~= address then
        ksi.trace("the index of SMN 0x%x moved during the write", address)
        return ksi.STATUS_DEVICE_DATA_ERROR
    end
    return ksi.STATUS_SUCCESS
end

local function store(s, register, value, mark)
    local st = smn_write(s, register, value, mark)
    if st == ksi.STATUS_RETRY then
        return st, false
    end
    if st == ksi.STATUS_SUCCESS or st ~= ksi.STATUS_DEVICE_DATA_ERROR then
        return st, true
    end
    local rs, held = smn_read(s, register)
    if rs == ksi.STATUS_RETRY then
        return ksi.STATUS_DEVICE_DATA_ERROR, true
    end
    if rs ~= ksi.STATUS_SUCCESS then
        return rs, true
    end
    if held ~= value then
        ksi.trace("SMN 0x%x holds 0x%x, not 0x%x", register, held, value)
        return ksi.STATUS_RETRY, true
    end
    return ksi.STATUS_SUCCESS, true
end

local function wait(s, timeout)
    local st, resp = ksi.STATUS_IO_TIMEOUT, 0
    poll(50, 20, 1, timeout, function(late)
        local rs, value = smn_read(s, s.mailbox.rsp)
        if rs == ksi.STATUS_RETRY and not late then
            value = 0
        elseif rs ~= ksi.STATUS_SUCCESS then
            st = rs
            return true
        end
        if value == ALL_ONES then
            ksi.trace("RSP reads all-ones")
            st = ksi.STATUS_DEVICE_NOT_READY
            return true
        end
        if value ~= 0 then
            st, resp = ksi.STATUS_SUCCESS, value
            return true
        end
        return false
    end)
    return st, resp
end

local function post(s, op, mark)
    local m = s.mailbox
    local bs, before = smn_read(s, m.msg)
    local st = ksi.STATUS_RETRY
    poll(0, 0, 1, WAIT_MS, function(late)
        local ws = smn_write(s, m.msg, op, nil, mark)
        if ws == ksi.STATUS_RETRY then
            ws = smn_write(s, m.msg, op, nil, mark)
        end
        if ws == ksi.STATUS_RETRY or ws == ksi.STATUS_SUCCESS then
            st = ws
            return true
        end
        if ws ~= ksi.STATUS_DEVICE_DATA_ERROR then
            st = ws
            return true
        end
        if bs ~= ksi.STATUS_SUCCESS then
            st = ksi.STATUS_SUCCESS
            return true
        end
        local as, after = smn_read(s, m.msg)
        if as ~= ksi.STATUS_SUCCESS then
            st = ksi.STATUS_SUCCESS
            return true
        end
        if (after == op and before ~= op) or after ~= before or
           after == op then
            st = ksi.STATUS_SUCCESS
            return true
        end
        if late then
            st = ksi.STATUS_RETRY
            return true
        end
        return false
    end)
    if st ~= ksi.STATUS_SUCCESS then
        ksi.trace("MSG 0x%x not posted: 0x%x", op, st)
    end
    return st
end

local function restore_idle(s, idle)
    for _ = 1, RESTORE_TRIES do
        local st = store(s, s.mailbox.rsp, idle)
        if st == ksi.STATUS_SUCCESS then
            return st
        end
        if st == ksi.STATUS_DEVICE_NOT_READY then
            break
        end
        ksi.sleep(1)
    end
    ksi.trace("the idle RSP 0x%x was not put back", idle)
    return ksi.STATUS_DEVICE_DATA_ERROR
end

local function read_result(s, register)
    local st, value = ksi.STATUS_RETRY, 0
    poll(0, 0, 1, WAIT_MS, function()
        st, value = smn_read(s, register)
        return st ~= ksi.STATUS_RETRY
    end)
    return st, value
end

local ANSWERS = {
    [0xfc] = ksi.STATUS_DEVICE_BUSY,
    [0xfd] = ksi.STATUS_INTERNAL_ERROR,
    [0xfe] = ksi.STATUS_NOT_IMPLEMENTED,
}

local function zeros()
    return { 0, 0, 0, 0, 0, 0, 0, 0 }
end

local function command_body(s, op, args, words, state)
    local m = s.mailbox
    local results = zeros()
    local st, resp = wait(s, STALE_MS)
    if st == ksi.STATUS_IO_TIMEOUT then
        ksi.trace("RSP stayed zero: stale, written over")
        st, resp = ksi.STATUS_SUCCESS, 0
    end
    if st ~= ksi.STATUS_SUCCESS then
        return st, false, results
    end
    state.idle = resp
    local issued
    st, issued = store(s, m.rsp, 0, function() state.clearing = true end)
    if st ~= ksi.STATUS_SUCCESS then
        state.restore = issued and st ~= ksi.STATUS_RETRY
        state.clearing = false
        return st, false, results
    end
    state.restore = true
    state.clearing = false
    for i = 1, math.min(m.args, ARGS) do
        st = store(s, m.arg + (i - 1) * 4, args[i])
        if st ~= ksi.STATUS_SUCCESS then
            return st, false, results
        end
    end
    st = post(s, op, function() state.posting = true end)
    if st == ksi.STATUS_RETRY then
        return st, false, results
    end
    state.restore = false
    if st ~= ksi.STATUS_SUCCESS then
        return st, false, results
    end
    st, resp = wait(s, WAIT_MS)
    if st ~= ksi.STATUS_SUCCESS then
        if st == ksi.STATUS_IO_TIMEOUT then
            ksi.trace("RSP stayed zero")
        end
        return st, false, results
    end
    if resp ~= 0x01 then
        ksi.trace("MSG 0x%x answered 0x%x", op, resp)
        return ANSWERS[resp] or ksi.STATUS_UNSUCCESSFUL, true, results, resp
    end
    for i = 1, words do
        st, results[i] = read_result(s, m.arg + (i - 1) * 4)
        if st ~= ksi.STATUS_SUCCESS then
            return st, false, zeros(), resp
        end
    end
    return ksi.STATUS_SUCCESS, true, results, resp
end

local function transact(s, op, args, words)
    local pci <close>, failed = ksi.lock("pci")
    if not pci then
        ksi.trace("the pci lock was not taken: 0x%x", failed)
        return failed, false, zeros(), 0
    end
    local state = { restore = false, idle = 0 }
    local putback <close> = setmetatable({}, {
        __close = function(_, err)
            if err and (state.restore or state.clearing) and
               not state.posting and state.idle ~= 0 then
                restore_idle(s, state.idle)
            end
        end,
    })
    local st, completed, results, answer = command_body(s, op, args, words,
                                                        state)
    if state.restore and state.idle ~= 0 then
        local rs = restore_idle(s, state.idle)
        state.restore = false
        if rs ~= ksi.STATUS_SUCCESS then
            st = rs
        end
    end
    return st, completed, results, answer or 0
end

local function protocol_version(s)
    local m = s.mailbox
    local args = { m.version_arg, 0, 0, 0, 0, 0, 0, 0 }
    local st, _, first = transact(s, m.version, args, 1)
    if st ~= ksi.STATUS_SUCCESS then
        return st, 0
    end
    if first[1] == 0 or first[1] == ALL_ONES then
        ksi.trace("version 0x%x", first[1])
        return ksi.STATUS_DEVICE_NOT_READY, 0
    end
    local again
    st, _, again = transact(s, m.version, args, 1)
    if st ~= ksi.STATUS_SUCCESS then
        return st, 0
    end
    if again[1] == 0 or again[1] == ALL_ONES then
        ksi.trace("version 0x%x", again[1])
        return ksi.STATUS_DEVICE_NOT_READY, 0
    end
    if again[1] ~= first[1] then
        ksi.trace("version 0x%x then 0x%x", first[1], again[1])
        return ksi.STATUS_RETRY, 0
    end
    return ksi.STATUS_SUCCESS, first[1]
end

-- Returns the version of the mailbox behind the root complex the input names.
function identify(input)
    if #input ~= 4 then
        ksi.trace("input of %d bytes", #input)
        return ksi.STATUS_INVALID_PARAMETER
    end
    local s, refused = session(input)
    if not s then
        return refused
    end
    local closer <close> = (not s.kept) and s.device or nil

    local lock <close>, failed = ksi.lock("mailbox")
    if not lock then
        ksi.trace("the mailbox lock was not taken: 0x%x", failed)
        return failed
    end

    local st, version = protocol_version(s)
    if st ~= ksi.STATUS_SUCCESS then
        return st
    end
    return ksi.STATUS_SUCCESS, string.pack("<I4I4", version, 0)
end

-- Sends a list of the row's commands to the mailbox behind the root complex
-- the input names.
function command(input)
    if #input < 4 or (#input - 4) % 36 ~= 0 then
        ksi.trace("input of %d bytes", #input)
        return ksi.STATUS_INVALID_PARAMETER
    end
    local entries = {}
    for i = 1, (#input - 4) // 36 do
        local e = pack(string.unpack("<I4I4I4I4I4I4I4I4I4", input,
                                     5 + (i - 1) * 36))
        if e[1] == 0 then
            ksi.trace("entry %d has opcode 0", i)
            return ksi.STATUS_INVALID_PARAMETER
        end
        entries[i] = e
    end

    local s, refused = session(input)
    if not s then
        return refused
    end
    local closer <close> = (not s.kept) and s.device or nil

    local lock <close>, failed = ksi.lock("mailbox")
    if not lock then
        ksi.trace("the mailbox lock was not taken: 0x%x", failed)
        return failed
    end

    local st = protocol_version(s)
    if st ~= ksi.STATUS_SUCCESS then
        return st
    end

    local output = {}
    local stopped = false
    for i, e in ipairs(entries) do
        local op = e[1]
        local c = s.mailbox.commands[op]
        local status, answer = ksi.STATUS_SUCCESS, 0
        local results = zeros()
        local args
        if c then
            local mask, fixed = c.mask or {}, c.fixed or {}
            args = {}
            for a = 1, ARGS do
                local value = e[a + 1]
                if (value & ~(mask[a] or 0)) ~= 0 then
                    args = nil
                    break
                end
                args[a] = value | (fixed[a] or 0)
            end
        end
        if not args then
            ksi.trace("command 0x%x is not permitted", op)
            status = ksi.STATUS_ACCESS_DENIED
        elseif stopped then
            status = ksi.STATUS_CANCELLED
        else
            local completed
            status, completed, results, answer = transact(s, op, args, c.words)
            stopped = not completed
        end
        output[i] = string.pack("<I4I4I4I4I4I4I4I4I4I4", status, answer,
                                unpack(results, 1, ARGS))
    end

    return ksi.STATUS_SUCCESS, concat(output)
end
