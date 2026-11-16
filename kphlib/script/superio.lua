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
-- Identifies an ITE Super I/O chip at a configuration port and reads its
-- hardware monitor registers. The chips and registers each row permits are
-- listed below.
--

local FAMILY = 3

local SLOTS = { [0] = 0x2e, [1] = 0x4e }

local CHASSIS = { { 0x01, 0x07 }, { 0x0f, 0x1d } }

local BASE_MIN = 0x0100
local BASE_LAST = 0x0ccf

local EXIT_NONE = 0
local EXIT_KEY_AA = 1
local EXIT_CONFIG_02 = 2

local function ite_monitor()
    return {
        ids = { { 0x58, 0x90, 0x7f } },
        reads = {
            { 0x00, 0x1 }, { 0x06, 0x1 }, { 0x0b, 0x5 }, { 0x13, 0x8 },
            { 0x1e, 0x2 }, { 0x20, 0x10 }, { 0x4c, 0x2 }, { 0x51, 0x1 },
            { 0x56, 0x4 }, { 0x60, 0x20 }, { 0x80, 0x4 }, { 0x92, 0x3 },
            { 0xa0, 0x10 },
        },
    }
end

local ROWS = {
    {
        chips = {
            0x8705, 0x8712, 0x8716, 0x8718, 0x8720, 0x8721, 0x8726, 0x8728,
            0x8732, 0x8771, 0x8772, 0x8781, 0x8782, 0x8783, 0x8786, 0x8603,
            0x8613, 0x8620, 0x8622, 0x8623, 0x8625, 0x8628, 0x8631, 0x8638,
            0x8655, 0x8665, 0x8686, 0x8688, 0x8689, 0x8696, 0x8698, 0x8606,
            0x8607, 0x8736, 0x8738, 0x8785,
        },
        ldn = 0x04,
        exit = EXIT_CONFIG_02,
        monitor = ite_monitor,
    },
    {
        chips = { 0x8790, 0x8733, 0x8695 },
        ldn = 0x04,
        exit = EXIT_NONE,
        monitor = ite_monitor,
    },
}

local function answers(id)
    return (id ~= 0) and (id ~= 0xffff)
end

local function find_row(id)
    for _, row in ipairs(ROWS) do
        for _, chip in ipairs(row.chips) do
            if chip == id then
                return row
            end
        end
    end
    return nil
end

local function chassis_type()
    local smbios = hw.firmware_table("RSMB", 0)
    if not smbios or #smbios < 8 then
        return nil
    end
    local last = math.min(#smbios, 8 + string.unpack("<I4", smbios, 5))
    local at = 9
    while at + 3 <= last do
        local kind, length = string.byte(smbios, at, at + 1)
        if length < 4 or at + length - 1 > last then
            return nil
        end
        if kind == 3 then
            if length < 6 then
                return nil
            end
            return string.byte(smbios, at + 5) & 0x7f
        end
        if kind == 127 then
            return nil
        end
        local strings = string.find(smbios, "\0\0", at + length, true)
        if not strings then
            return nil
        end
        at = strings + 2
    end
    return nil
end

local function chassis_permitted()
    local kind = chassis_type()
    if not kind then
        ksi.trace("no chassis type in the SMBIOS table")
        return false
    end
    for _, range in ipairs(CHASSIS) do
        if kind >= range[1] and kind <= range[2] then
            return true
        end
    end
    ksi.trace("chassis type 0x%02x is not permitted", kind)
    return false
end

local function rc(s, reg)
    hw.port_write(s.port, 1, reg)
    return hw.port_read(s.port + 1, 1)
end

local function wc(s, reg, value)
    hw.port_write(s.port, 1, reg)
    hw.port_write(s.port + 1, 1, value)
end

local function rcw(s, reg)
    local hi = rc(s, reg)
    local lo = rc(s, reg + 1)
    return (hi << 8) | lo
end

local function exit_config(s)
    if s.entered then
        if s.row.exit == EXIT_KEY_AA then
            hw.port_write(s.port, 1, 0xaa)
        end
        if s.row.exit == EXIT_CONFIG_02 then
            wc(s, 0x02, 0x02)
        end
    end
    s.entered = false
end

local function abandon(s)
    if s.entered then
        hw.port_write(s.port, 1, 0xaa)
        if answers(rcw(s, 0x20)) then
            wc(s, 0x02, 0x02)
        end
        if answers(rcw(s, 0x20)) then
            ksi.trace("port 0x%02x still answers after the exit", s.port)
            s.busy = true
        end
    end
    s.entered = false
end

local function close(s)
    if s.ldn_set and not s.contended and s.orig_ldn ~= s.row.ldn then
        local current = rc(s, 0x07)
        if current == s.row.ldn then
            wc(s, 0x07, s.orig_ldn)
        else
            ksi.trace("LDN 0x%02x selected, not 0x%02x: not restored",
                      current, s.row.ldn)
            s.contended = true
        end
    end
    s.ldn_set = false
    if not s.contended then
        if s.identified then
            exit_config(s)
        else
            abandon(s)
        end
    end
end

local function probe(s)
    hw.port_write(s.port, 1, 0x20)
    local hi = hw.port_read(s.port + 1, 1)
    hw.port_write(s.port, 1, 0x21)
    local lo = hw.port_read(s.port + 1, 1)
    local id = (hi << 8) | lo
    s.already_open = answers(id)
    if s.already_open and not find_row(id) then
        ksi.trace("port 0x%02x answers 0x%04x before any key", s.port, id)
        return false
    end
    return true
end

local function configure(s)
    s.idphase = true
    if not s.already_open then
        s.entered = true
        hw.port_write(s.port, 1, 0x87)
        hw.port_write(s.port, 1, 0x01)
        hw.port_write(s.port, 1, 0x55)
        hw.port_write(s.port, 1, s.port == 0x2e and 0x55 or 0xaa)
    end
    local id = rcw(s, 0x20)
    s.entered = (not s.already_open) and answers(id)
    s.row = find_row(id)
    if not s.row then
        ksi.trace("chip 0x%04x at port 0x%02x is not known", id, s.port)
        return ksi.STATUS_NOT_FOUND
    end
    s.identified = true
    s.orig_ldn = rc(s, 0x07)
    wc(s, 0x07, s.row.ldn)
    s.ldn_set = true
    if (rc(s, 0x30) & 1) == 0 then
        ksi.trace("LDN 0x%02x is not active", s.row.ldn)
        return ksi.STATUS_DEVICE_NOT_READY
    end
    local base = rcw(s, 0x60)
    ksi.sleep(1)
    if rc(s, 0x07) ~= s.row.ldn or rcw(s, 0x60) ~= base then
        ksi.trace("LDN 0x%02x base 0x%04x changed while read", s.row.ldn,
                  base)
        s.contended = true
        return ksi.STATUS_RETRY
    end
    if base == 0 or base == 0xffff then
        ksi.trace("monitor base 0x%04x is not set", base)
        return ksi.STATUS_DEVICE_NOT_READY
    end
    if (base & 7) ~= 0 then
        ksi.trace("monitor base 0x%04x is misaligned", base)
        return ksi.STATUS_DEVICE_CONFIGURATION_ERROR
    end
    if base < BASE_MIN or base + 7 > BASE_LAST then
        ksi.trace("monitor base 0x%04x is outside the permitted range", base)
        return ksi.STATUS_DEVICE_CONFIGURATION_ERROR
    end
    s.base = base
    s.chip = id
    close(s)
    s.idphase = false
    if s.contended then
        return ksi.STATUS_RETRY
    end
    return ksi.STATUS_SUCCESS
end

local function permitted(monitor, reg)
    for _, range in ipairs(monitor.reads) do
        if reg >= range[1] and reg < range[1] + range[2] then
            return true
        end
    end
    return false
end

local function rm(s, reg)
    hw.port_write(s.base + 5, 1, reg & 0xff)
    local value = hw.port_read(s.base + 6, 1)
    if hw.port_read(s.base + 5, 1) ~= (reg & 0xff) then
        return nil
    end
    return value
end

local function finish(s, status)
    if s.idphase then
        close(s)
        s.idphase = false
        if s.contended then
            status = ksi.STATUS_RETRY
        elseif s.busy then
            status = ksi.STATUS_DEVICE_BUSY
        end
    end
    return status
end

local function session(input)
    local slot = string.unpack("<I4", input)
    local port = SLOTS[slot]
    if not port then
        ksi.trace("slot %d does not exist", slot)
        return nil, ksi.STATUS_INVALID_PARAMETER
    end
    if not chassis_permitted() then
        return nil, ksi.STATUS_NOT_SUPPORTED
    end
    return { port = port }
end

-- Returns the family, chip and customer of the chip at a slot.
function identify(input)
    if not hw.port_read then
        return ksi.STATUS_NOT_SUPPORTED
    end
    if #input ~= 4 then
        ksi.trace("input of %d bytes", #input)
        return ksi.STATUS_INVALID_PARAMETER
    end
    local s, refused = session(input)
    if not s then
        return refused
    end
    local lock <close>, failed = hw.lock("isa")
    if not lock then
        ksi.trace("the isa lock was not taken: 0x%08x", failed)
        return failed
    end
    local putback <close> = setmetatable({}, {
        __close = function()
            finish(s, ksi.STATUS_SUCCESS)
        end,
    })
    if not probe(s) then
        return ksi.STATUS_NOT_FOUND
    end
    local status = finish(s, configure(s))
    if status ~= ksi.STATUS_SUCCESS then
        return status
    end
    return ksi.STATUS_SUCCESS, string.pack("<I4I4I4", FAMILY, s.chip, 0)
end

-- Reads hardware monitor registers of the chip at a slot.
function read(input)
    if not hw.port_read then
        return ksi.STATUS_NOT_SUPPORTED
    end
    if #input < 8 or (#input % 4) ~= 0 then
        ksi.trace("input of %d bytes", #input)
        return ksi.STATUS_INVALID_PARAMETER
    end
    local s, refused = session(input)
    if not s then
        return refused
    end
    local lock <close>, failed = hw.lock("isa")
    if not lock then
        ksi.trace("the isa lock was not taken: 0x%08x", failed)
        return failed
    end
    local putback <close> = setmetatable({}, {
        __close = function()
            finish(s, ksi.STATUS_SUCCESS)
        end,
    })
    if not probe(s) then
        return ksi.STATUS_NOT_FOUND
    end
    local status = finish(s, configure(s))
    if status ~= ksi.STATUS_SUCCESS then
        return status
    end
    local monitor = s.row.monitor()
    local count = (#input - 4) // 4
    local any = false
    for i = 1, count do
        if permitted(monitor, string.unpack("<I4", input, 1 + 4 * i)) then
            any = true
        end
    end
    if not any then
        ksi.trace("no register asked for is permitted")
        return ksi.STATUS_INVALID_PARAMETER
    end
    for _, want in ipairs(monitor.ids) do
        local got = rm(s, want[1])
        if not got then
            ksi.trace("the monitor index moved under register 0x%02x", want[1])
            return ksi.STATUS_RETRY
        end
        if got ~= want[2] and got ~= want[3] then
            ksi.trace("register 0x%02x is 0x%02x, not 0x%02x or 0x%02x",
                      want[1], got, want[2], want[3])
            return ksi.STATUS_NOT_FOUND
        end
    end
    local output = {}
    local moved = false
    for i = 1, count do
        local reg = string.unpack("<I4", input, 1 + 4 * i)
        local item = ksi.STATUS_ACCESS_DENIED
        local value = 0
        if permitted(monitor, reg) then
            item = ksi.STATUS_RETRY
            if not moved then
                value = rm(s, reg)
                moved = (value == nil)
                item = moved and ksi.STATUS_RETRY or ksi.STATUS_SUCCESS
                value = value or 0
                if moved then
                    ksi.trace("the monitor index moved under register 0x%02x",
                              reg)
                end
            end
        else
            ksi.trace("register 0x%x is not permitted", reg)
        end
        output[i] = string.pack("<I4I4", item, value)
    end
    return ksi.STATUS_SUCCESS, table.concat(output)
end
