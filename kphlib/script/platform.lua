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
-- Reads Intel platform registers found through the ACPI low power idle table:
-- the residency counter itself, and the power management block that holds it.
-- The processors, the windows each one has and the reads permitted in them are
-- listed below.
--

local ROWS = {
    {
        family = 0x06,
        models = {
            { 0x4e, 0x4e }, { 0x5e, 0x5e }, { 0x8e, 0x8e }, { 0x9e, 0x9e },
            { 0xa5, 0xa5 }, { 0xa6, 0xa6 }, { 0x66, 0x66 },
        },
        lpit = { entry = 0, size = 4 },
        pwrm = {
        },
    },
    {
        family = 0x06,
        models = {
            { 0x7d, 0x7d }, { 0x7e, 0x7e }, { 0x9d, 0x9d }, { 0x8c, 0x8c },
            { 0x8d, 0x8d }, { 0xa7, 0xa7 }, { 0xd7, 0xd7 },
        },
        lpit = { entry = 0, size = 4 },
        pwrm = {
            { entry = 0, offset = 0x193c, size = 0x10000,
              reads = { { 0x1500, 0x8, 4 }, { 0x1560, 0x4, 4 } } },
            { entry = 0, offset = 0x1098, size = 0x10000,
              reads = { { 0x1500, 0x8, 4 }, { 0x1560, 0x4, 4 } } },
        },
    },
    {
        family = 0x06,
        models = {
            { 0x8a, 0x8a },
        },
        lpit = { entry = 0, size = 4 },
        pwrm = {
        },
    },
    {
        family = 0x06,
        models = {
            { 0x97, 0x97 }, { 0x9a, 0x9a }, { 0xb7, 0xb7 }, { 0xba, 0xba },
            { 0xbf, 0xbf }, { 0xaa, 0xaa }, { 0xac, 0xac }, { 0xc5, 0xc5 },
            { 0xc6, 0xc6 }, { 0xb5, 0xb5 }, { 0xbd, 0xbd }, { 0xcc, 0xcc },
            { 0xd5, 0xd5 }, { 0xe5, 0xe5 },
        },
        lpit = { entry = 0, size = 4 },
        pwrm = {
            { entry = 0, offset = 0x193c, size = 0x10000,
              reads = { { 0x1500, 0x8, 4 }, { 0x1560, 0x4, 4 } } },
            { entry = 0, offset = 0x1098, size = 0x10000,
              reads = { { 0x1500, 0x8, 4 }, { 0x1560, 0x4, 4 } } },
        },
    },
    {
        family = 0x06,
        models = {
            { 0xbe, 0xbe },
        },
        lpit = { entry = 0, size = 4 },
        pwrm = {
            { entry = 0, offset = 0x193c, size = 0x10000,
              reads = { { 0x1500, 0x8, 4 }, { 0x1560, 0x4, 4 } } },
            { entry = 0, offset = 0x1098, size = 0x10000,
              reads = { { 0x1500, 0x8, 4 }, { 0x1560, 0x4, 4 } } },
        },
    },
    {
        family = 0x06,
        models = {
            { 0x5c, 0x5c }, { 0x5f, 0x5f }, { 0x7a, 0x7a }, { 0x86, 0x86 },
            { 0x96, 0x96 }, { 0x9c, 0x9c },
        },
        lpit = { entry = 0, size = 4 },
        pwrm = {
        },
    },
    {
        family = 0x12,
        models = {
            { 0x01, 0x01 }, { 0x03, 0x03 },
        },
        lpit = { entry = 0, size = 4 },
        pwrm = {
            { entry = 0, offset = 0x193c, size = 0x10000,
              reads = { { 0x1500, 0x8, 4 }, { 0x1560, 0x4, 4 } } },
            { entry = 0, offset = 0x1098, size = 0x10000,
              reads = { { 0x1500, 0x8, 4 }, { 0x1560, 0x4, 4 } } },
        },
    },
}

local WINDOW_LPIT = 1
local WINDOW_PWRM = 2

local PAGE = 0x1000
local HUGE_PAGE_END = -PAGE

local LPIT_HEADER = 36
local LPI_NATIVE = 0
local LPI_LENGTH = 56
local SPACE_MEMORY = 0

local function cpu_identity()
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
    return {
        vendor = string.pack("<I4I4I4", b, d, c),
        family = family,
        model = model,
    }
end

local function find_row(cpu)
    if cpu.vendor ~= "GenuineIntel" then
        return nil
    end
    for _, row in ipairs(ROWS) do
        if row.family == cpu.family then
            for _, range in ipairs(row.models) do
                if cpu.model >= range[1] and cpu.model <= range[2] then
                    return row
                end
            end
        end
    end
    return nil
end

local function lpit_counters()
    local lpit = ksi.hw.firmware_table("ACPI", "LPIT")
    if not lpit then
        ksi.trace("no LPIT")
        return {}
    end
    local counters = {}
    local length = #lpit
    local offset = LPIT_HEADER
    while offset < length do
        if length - offset < 8 then
            ksi.trace("LPIT entry at %d of %d truncated", offset, length)
            return {}
        end
        local kind, size = string.unpack("<I4I4", lpit, offset + 1)
        local finish = offset + size
        if size < 8 or finish > length then
            ksi.trace("LPIT entry at %d length %d malformed, table %d",
                      offset, size, length)
            return {}
        end
        if kind == LPI_NATIVE and size >= LPI_LENGTH then
            local flags = string.unpack("<I4", lpit, offset + 13)
            local space, width, shift = string.byte(lpit, offset + 37,
                                                    offset + 39)
            local address = string.unpack("<I8", lpit, offset + 41)
            if flags & 0x3 == 0 and space == SPACE_MEMORY and shift == 0 and
               (width == 32 or width == 64) then
                counters[#counters + 1] = { address = address,
                                            width = width // 8 }
            end
        end
        offset = finish
    end
    return counters
end

local function counter_at(entry)
    local counter = lpit_counters()[entry + 1]
    if not counter then
        ksi.trace("LPIT counter %d not found", entry)
        return nil, ksi.STATUS_NOT_FOUND
    end
    if counter.address == 0 then
        ksi.trace("LPIT counter %d has no address", entry)
        return nil, ksi.STATUS_DEVICE_NOT_READY
    end
    return counter
end

local function lpit_window(row)
    local w = row.lpit
    local counter, failed = counter_at(w.entry)
    if not counter then
        return nil, failed
    end
    if counter.width ~= w.size or counter.address % w.size ~= 0 then
        ksi.trace("LPIT counter %d at 0x%x width %d, window %d", w.entry,
                  counter.address, counter.width, w.size)
        return nil, ksi.STATUS_DEVICE_CONFIGURATION_ERROR
    end
    return counter.address, w.size, { { 0, w.size, w.size } }
end

local function pwrm_window(row)
    local counters
    for _, w in ipairs(row.pwrm) do
        counters = counters or lpit_counters()
        local counter = counters[w.entry + 1]
        if not counter then
            ksi.trace("LPIT counter %d not found", w.entry)
            return nil, ksi.STATUS_NOT_FOUND
        end
        if counter.address == 0 then
            ksi.trace("LPIT counter %d has no address", w.entry)
            return nil, ksi.STATUS_DEVICE_NOT_READY
        end
        if not math.ult(counter.address, w.offset) and
           (counter.address - w.offset) % w.size == 0 then
            return counter.address - w.offset, w.size, w.reads
        end
        ksi.trace("LPIT counter %d at 0x%x, block offset 0x%x size 0x%x",
                  w.entry, counter.address, w.offset, w.size)
    end
    return nil, ksi.STATUS_DEVICE_CONFIGURATION_ERROR
end

local function address_bits()
    local results, status = ksi.hw.cpuid(
        string.pack("<I2I2I4I4", 0, 0, 0x80000000, 0) ..
        string.pack("<I2I2I4I4", 0, 0, 0x80000008, 0))
    if not results then
        ksi.trace("cpuid failed: 0x%x", status)
        return nil, status
    end
    local s0, maximum = string.unpack("<I4I4", results, 1)
    local s1, eax = string.unpack("<I4I4", results, 21)
    if s0 == ksi.STATUS_SUCCESS and maximum >= 0x80000008 and
       s1 == ksi.STATUS_SUCCESS and eax & 0xff >= 32 and eax & 0xff < 64 then
        return eax & 0xff
    end
    return 36
end

local function check_range(first, last, bits)
    if math.ult(1 << bits, last) then
        ksi.trace("range end 0x%x beyond %d address bits", last, bits)
        return ksi.STATUS_CONFLICTING_ADDRESSES
    end
    local nvdimm = ksi.hw.is_nvdimm(first, last - first)
    if nvdimm == nil then
        ksi.trace("NVDIMM ranges unknown")
        return ksi.STATUS_DEVICE_CONFIGURATION_ERROR
    end
    if nvdimm then
        ksi.trace("range 0x%x-0x%x overlaps NVDIMM", first, last)
        return ksi.STATUS_CONFLICTING_ADDRESSES
    end
    local ram, ramFailed = ksi.hw.is_ram(first, last - first)
    if ram == nil then
        ksi.trace("memory ranges unknown: 0x%x", ramFailed)
        return ramFailed
    end
    if ram then
        ksi.trace("range 0x%x-0x%x overlaps memory", first, last)
        return ksi.STATUS_CONFLICTING_ADDRESSES
    end
    return ksi.STATUS_SUCCESS
end

local function permitted(ranges, offset, width)
    for _, r in ipairs(ranges) do
        if r[3] == width and offset >= r[1] and offset - r[1] < r[2] then
            return true
        end
    end
    return false
end

local function map_and_read(entries, base, size, spanBegin, spanEnd)
    local windowEnd = base + size
    if math.ult(windowEnd, base) or math.ult(HUGE_PAGE_END, windowEnd) then
        ksi.trace("window 0x%x size 0x%x ends beyond the address space",
                  base, size)
        return ksi.STATUS_DEVICE_CONFIGURATION_ERROR
    end
    local bits, bitsFailed = address_bits()
    if not bits then
        return bitsFailed
    end
    local st = check_range(base & -PAGE, (windowEnd + PAGE - 1) & -PAGE,
                           bits)
    if st ~= ksi.STATUS_SUCCESS then
        return st
    end
    spanBegin = spanBegin & -8
    local m, mapFailed = ksi.hw.map(base + spanBegin, spanEnd - spanBegin)
    if not m then
        ksi.trace("window 0x%x not mapped: 0x%x", base, mapFailed)
        return mapFailed
    end
    local mapping <close> = m
    for _, e in ipairs(entries) do
        if e.status == nil then
            e.value = ksi.hw.map_read(m, e.offset - spanBegin, e.width)
            e.status = ksi.STATUS_SUCCESS
        end
    end
    return ksi.STATUS_SUCCESS
end

-- Reads registers of a platform window: the LPIT residency counter, or the
-- power management block that holds it.
function read(input)
    if not ksi.hw.cpuid or not ksi.hw.firmware_table then
        ksi.trace("no CPUID or firmware table functions")
        return ksi.STATUS_NOT_SUPPORTED
    end
    if #input < 12 or (#input - 4) % 8 ~= 0 then
        ksi.trace("input of %d bytes", #input)
        return ksi.STATUS_INVALID_PARAMETER
    end
    local window = string.unpack("<I4", input)
    if window ~= WINDOW_LPIT and window ~= WINDOW_PWRM then
        ksi.trace("window %d", window)
        return ksi.STATUS_INVALID_PARAMETER
    end
    local entries = {}
    for i = 1, (#input - 4) // 8 do
        local offset, width = string.unpack("<I4I4", input, 5 + (i - 1) * 8)
        if (width ~= 1 and width ~= 2 and width ~= 4 and width ~= 8) or
           offset % width ~= 0 then
            ksi.trace("entry %d: offset 0x%x width %d", i, offset, width)
            return ksi.STATUS_INVALID_PARAMETER
        end
        entries[i] = { offset = offset, width = width }
    end

    local cpu, failed = cpu_identity()
    if not cpu then
        return failed
    end
    local row = find_row(cpu)
    if not row then
        ksi.trace("no row for %s family 0x%x model 0x%x", cpu.vendor,
                  cpu.family, cpu.model)
        return ksi.STATUS_NOT_SUPPORTED
    end
    if (window == WINDOW_LPIT and not row.lpit) or
       (window == WINDOW_PWRM and #row.pwrm == 0) then
        ksi.trace("family 0x%x model 0x%x has no window %d", cpu.family,
                  cpu.model, window)
        return ksi.STATUS_NOT_SUPPORTED
    end

    local candidates = {}
    if window == WINDOW_LPIT then
        candidates[1] = { { 0, row.lpit.size, row.lpit.size } }
    else
        for i, w in ipairs(row.pwrm) do
            candidates[i] = w.reads
        end
    end
    local any = false
    for _, e in ipairs(entries) do
        e.status, e.value = ksi.STATUS_ACCESS_DENIED, 0
        for _, ranges in ipairs(candidates) do
            if permitted(ranges, e.offset, e.width) then
                any = true
            end
        end
    end

    if any then
        local base, size, ranges
        if window == WINDOW_LPIT then
            base, size, ranges = lpit_window(row)
        else
            base, size, ranges = pwrm_window(row)
        end
        if not base then
            return size
        end
        local spanBegin, spanEnd
        for _, e in ipairs(entries) do
            if permitted(ranges, e.offset, e.width) then
                e.status = nil
                if not spanBegin or e.offset < spanBegin then
                    spanBegin = e.offset
                end
                if not spanEnd or e.offset + e.width > spanEnd then
                    spanEnd = e.offset + e.width
                end
            else
                ksi.trace("offset 0x%x width %d is not permitted", e.offset,
                          e.width)
            end
        end
        if spanBegin then
            local st = map_and_read(entries, base, size, spanBegin, spanEnd)
            if st ~= ksi.STATUS_SUCCESS then
                return st
            end
        end
    end

    local output = {}
    for i, e in ipairs(entries) do
        output[i] = string.pack("<I4I4I8", e.status, 0, e.value)
    end
    return ksi.STATUS_SUCCESS, table.concat(output)
end
