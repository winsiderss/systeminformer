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
-- Reads registers behind a memory window of a PCI function: the memory
-- controller hub window whose base a configuration register holds, a BAR, or
-- (windows 3 to 6) a server memory controller's window whose base two
-- configuration registers hold.
-- The devices, their windows and the offsets each may be asked for are listed
-- below; every one but the last is a function of an Intel processor, the last
-- (the PCH thermal function, a row without family and models) is read on any.
--

local nt = require('nt')

local ROWS = {
  {
    family = 0x06,
    models = { { 0x6a, 0x6a }, { 0x6c, 0x6c } },
    vendor = 0x8086,
    devices = { 0x3451 },
    window = 3,
    controller = 0,
    size = 0x4000,
    align = 0x1000,
    d0 = false,
    reads = function()
      return {
        { 0x2290, 0x10, 8 },
      }
    end,
  },
  {
    family = 0x06,
    models = { { 0x6a, 0x6a }, { 0x6c, 0x6c } },
    vendor = 0x8086,
    devices = { 0x3451 },
    window = 4,
    controller = 1,
    size = 0x4000,
    align = 0x1000,
    d0 = false,
    reads = function()
      return {
        { 0x2290, 0x10, 8 },
      }
    end,
  },
  {
    family = 0x06,
    models = { { 0x6a, 0x6a }, { 0x6c, 0x6c } },
    vendor = 0x8086,
    devices = { 0x3451 },
    window = 5,
    controller = 2,
    size = 0x4000,
    align = 0x1000,
    d0 = false,
    reads = function()
      return {
        { 0x2290, 0x10, 8 },
      }
    end,
  },
  {
    family = 0x06,
    models = { { 0x6a, 0x6a }, { 0x6c, 0x6c } },
    vendor = 0x8086,
    devices = { 0x3451 },
    window = 6,
    controller = 3,
    size = 0x4000,
    align = 0x1000,
    d0 = false,
    reads = function()
      return {
        { 0x2290, 0x10, 8 },
      }
    end,
  },
  {
    family = 0x06,
    models = { { 0x2a, 0x2a } },
    vendor = 0x8086,
    devices = { 0x0100, 0x0108, 0x010c },
    window = 1,
    offset = 0x48,
    width = 8,
    mask = 0x7fffff8000,
    enable = 0,
    size = 0x8000,
    d0 = false,
    reads = function()
      return {
        { 0x5040, 0xc, 4 },
        { 0x5050, 0x8, 4 },
        { 0x5e00, 0x8, 4 },
        { 0x4000, 0x8, 4 },
        { 0x4298, 0x4, 4 },
        { 0x4400, 0x8, 4 },
        { 0x4698, 0x4, 4 },
        { 0x5948, 0x4, 4 },
        { 0x5994, 0x8, 4 },
      }
    end,
  },
  {
    family = 0x06,
    models = { { 0x3a, 0x3a } },
    vendor = 0x8086,
    devices = { 0x0154, 0x0150, 0x0158, 0x015c },
    window = 1,
    offset = 0x48,
    width = 8,
    mask = 0x7fffff8000,
    enable = 0,
    size = 0x8000,
    d0 = false,
    reads = function()
      return {
        { 0x5040, 0xc, 4 },
        { 0x5050, 0x8, 4 },
        { 0x5e00, 0x8, 4 },
        { 0x4000, 0x8, 4 },
        { 0x4298, 0x4, 4 },
        { 0x4400, 0x8, 4 },
        { 0x4698, 0x4, 4 },
        { 0x5948, 0x4, 4 },
        { 0x5994, 0x8, 4 },
      }
    end,
  },
  {
    family = 0x06,
    models = { { 0x3c, 0x3c }, { 0x46, 0x46 } },
    vendor = 0x8086,
    devices = { 0x0c00, 0x0c04, 0x0c08 },
    window = 1,
    offset = 0x48,
    width = 8,
    mask = 0x7fffff8000,
    enable = 0,
    size = 0x8000,
    d0 = false,
    reads = function()
      return {
        { 0x5040, 0xc, 4 },
        { 0x5050, 0x8, 4 },
        { 0x5e00, 0x8, 4 },
        { 0x4000, 0x8, 4 },
        { 0x4014, 0x4, 4 },
        { 0x4298, 0x4, 4 },
        { 0x4400, 0x8, 4 },
        { 0x4414, 0x4, 4 },
        { 0x4698, 0x4, 4 },
        { 0x5948, 0x4, 4 },
        { 0x5994, 0x8, 4 },
      }
    end,
  },
  {
    family = 0x06,
    models = { { 0x3c, 0x3c }, { 0x46, 0x46 } },
    vendor = 0x8086,
    devices = { 0x0a03 },
    window = 2,
    size = 0x8000,
    d0 = true,
    reads = function()
      return {
        { 0x58e0, 0x10, 4 },
        { 0x5930, 0x10, 4 },
        { 0x59a0, 0x8, 4 },
      }
    end,
  },
  {
    family = 0x06,
    models = { { 0x45, 0x45 } },
    vendor = 0x8086,
    devices = { 0x0a04 },
    window = 1,
    offset = 0x48,
    width = 8,
    mask = 0x7fffff8000,
    enable = 0,
    size = 0x8000,
    d0 = false,
    reads = function()
      return {
        { 0x5040, 0xc, 4 },
        { 0x5050, 0x8, 4 },
        { 0x5e00, 0x8, 4 },
        { 0x4000, 0x8, 4 },
        { 0x4014, 0x4, 4 },
        { 0x4298, 0x4, 4 },
        { 0x4400, 0x8, 4 },
        { 0x4414, 0x4, 4 },
        { 0x4698, 0x4, 4 },
        { 0x5948, 0x4, 4 },
        { 0x5994, 0x8, 4 },
      }
    end,
  },
  {
    family = 0x06,
    models = { { 0x45, 0x45 } },
    vendor = 0x8086,
    devices = { 0x0a03 },
    window = 2,
    size = 0x8000,
    d0 = true,
    reads = function()
      return {
        { 0x58e0, 0x10, 4 },
        { 0x5930, 0x10, 4 },
        { 0x59a0, 0x8, 4 },
      }
    end,
  },
  {
    family = 0x06,
    models = { { 0x3d, 0x3d }, { 0x47, 0x47 } },
    vendor = 0x8086,
    devices = { 0x1604 },
    window = 1,
    offset = 0x48,
    width = 8,
    mask = 0x7fffff8000,
    enable = 0,
    size = 0x8000,
    d0 = false,
    reads = function()
      return {
        { 0x5040, 0xc, 4 },
        { 0x5050, 0x8, 4 },
        { 0x5e00, 0x8, 4 },
        { 0x4000, 0x8, 4 },
        { 0x4014, 0x4, 4 },
        { 0x4298, 0x4, 4 },
        { 0x4400, 0x8, 4 },
        { 0x4414, 0x4, 4 },
        { 0x4698, 0x4, 4 },
        { 0x5948, 0x4, 4 },
        { 0x5994, 0x8, 4 },
      }
    end,
  },
  {
    family = 0x06,
    models = { { 0x3d, 0x3d }, { 0x47, 0x47 } },
    vendor = 0x8086,
    devices = { 0x1603 },
    window = 2,
    size = 0x8000,
    d0 = true,
    reads = function()
      return {
        { 0x58e0, 0x10, 4 },
        { 0x5930, 0x10, 4 },
        { 0x59a0, 0x8, 4 },
      }
    end,
  },
  {
    family = 0x06,
    models = {
      { 0x4e, 0x4e },
      { 0x5e, 0x5e },
      { 0x8e, 0x8e },
      { 0x9e, 0x9e },
      { 0xa5, 0xa5 },
      { 0xa6, 0xa6 },
      { 0x66, 0x66 },
    },
    vendor = 0x8086,
    devices = {
      0x190c,
      0x1904,
      0x1900,
      0x1910,
      0x190f,
      0x191f,
      0x1918,
      0x590c,
      0x5904,
      0x5914,
      0x590f,
      0x591f,
      0x5910,
      0x5918,
      0x590d,
      0x3ecc,
      0x3ed0,
      0x3e10,
      0x3ec4,
      0x3e0f,
      0x3e1f,
      0x3ec2,
      0x3e30,
      0x3e18,
      0x3ec6,
      0x3e31,
      0x3e33,
      0x3eca,
      0x3e32,
      0x3e34,
      0x3e35,
      0x9b44,
      0x9b54,
      0x9b64,
      0x9b51,
      0x9b61,
      0x9b71,
      0x9b33,
      0x9b43,
      0x9b53,
      0x9b63,
      0x9b73,
      0x1924,
      0x3e20,
    },
    window = 1,
    offset = 0x48,
    width = 8,
    mask = 0x7fffff8000,
    enable = 0,
    size = 0x8000,
    d0 = false,
    reads = function()
      return {
        { 0x5918, 0x8, 4 },
        { 0x5040, 0xc, 4 },
        { 0x5050, 0x8, 4 },
        { 0x58e0, 0x10, 4 },
        { 0x5930, 0x10, 4 },
        { 0x59a0, 0x8, 4 },
        { 0x5978, 0xc, 4 },
        { 0x599c, 0x4, 4 },
        { 0x5e00, 0x8, 4 },
        { 0x4000, 0x4, 4 },
        { 0x401c, 0x4, 4 },
        { 0x4070, 0x4, 4 },
        { 0x423c, 0x4, 4 },
        { 0x4400, 0x4, 4 },
        { 0x441c, 0x4, 4 },
        { 0x4470, 0x4, 4 },
        { 0x463c, 0x4, 4 },
        { 0x5948, 0x4, 4 },
        { 0x5994, 0x8, 4 },
      }
    end,
  },
  {
    family = 0x06,
    models = {
      { 0x4e, 0x4e },
      { 0x5e, 0x5e },
      { 0x8e, 0x8e },
      { 0x9e, 0x9e },
      { 0xa5, 0xa5 },
      { 0xa6, 0xa6 },
      { 0x66, 0x66 },
    },
    vendor = 0x8086,
    devices = { 0x5a02, 0x5a04 },
    window = 1,
    offset = 0x48,
    width = 8,
    mask = 0x7fffff8000,
    enable = 0,
    size = 0x8000,
    d0 = false,
    reads = function()
      return {
        { 0x5918, 0x8, 4 },
        { 0x5040, 0xc, 4 },
        { 0x5050, 0x8, 4 },
        { 0x58e0, 0x10, 4 },
        { 0x5930, 0x10, 4 },
        { 0x59a0, 0x8, 4 },
        { 0x5978, 0xc, 4 },
        { 0x599c, 0x4, 4 },
        { 0x5e00, 0x8, 4 },
        { 0x4000, 0x4, 4 },
        { 0x4070, 0x4, 4 },
        { 0x423c, 0x4, 4 },
        { 0x4400, 0x4, 4 },
        { 0x4470, 0x4, 4 },
        { 0x463c, 0x4, 4 },
        { 0x5948, 0x4, 4 },
        { 0x5994, 0x8, 4 },
      }
    end,
  },
  {
    family = 0x06,
    models = {
      { 0x4e, 0x4e },
      { 0x5e, 0x5e },
      { 0x8e, 0x8e },
      { 0x9e, 0x9e },
      { 0xa5, 0xa5 },
      { 0xa6, 0xa6 },
      { 0x66, 0x66 },
    },
    vendor = 0x8086,
    devices = { 0x1903, 0x3e83, 0x5a03 },
    window = 2,
    size = 0x8000,
    d0 = true,
    reads = function()
      return {
        { 0x5820, 0x4, 4 },
        { 0x58e0, 0x10, 4 },
        { 0x58f0, 0x4, 4 },
        { 0x5930, 0x10, 4 },
        { 0x59a0, 0x8, 4 },
        { 0x5978, 0xc, 4 },
        { 0x599c, 0x4, 4 },
      }
    end,
  },
  {
    family = 0x06,
    models = {
      { 0x7d, 0x7d },
      { 0x7e, 0x7e },
      { 0x9d, 0x9d },
      { 0x8c, 0x8c },
      { 0x8d, 0x8d },
      { 0xa7, 0xa7 },
      { 0xd7, 0xd7 },
    },
    vendor = 0x8086,
    devices = { 0x8a02, 0x8a12, 0x4c43, 0x4c53 },
    window = 1,
    offset = 0x48,
    width = 8,
    mask = 0x7fffff0000,
    enable = 0,
    size = 0x10000,
    d0 = false,
    reads = function()
      return {
        { 0x5040, 0xc, 4 },
        { 0x5050, 0x8, 4 },
        { 0x58e0, 0x10, 4 },
        { 0x5930, 0x10, 4 },
        { 0x59a0, 0x8, 4 },
        { 0x5978, 0xc, 4 },
        { 0x599c, 0x4, 4 },
        { 0x5e00, 0x8, 4 },
        { 0x4000, 0x4, 4 },
        { 0x4070, 0x4, 4 },
        { 0x4088, 0x4, 4 },
        { 0x423c, 0x4, 4 },
        { 0x4400, 0x4, 4 },
        { 0x4470, 0x4, 4 },
        { 0x4488, 0x4, 4 },
        { 0x463c, 0x4, 4 },
        { 0x5948, 0x4, 4 },
        { 0x5994, 0x8, 4 },
        { 0x5ef0, 0x4, 4 },
      }
    end,
  },
  {
    family = 0x06,
    models = {
      { 0x7d, 0x7d },
      { 0x7e, 0x7e },
      { 0x9d, 0x9d },
      { 0x8c, 0x8c },
      { 0x8d, 0x8d },
      { 0xa7, 0xa7 },
      { 0xd7, 0xd7 },
    },
    vendor = 0x8086,
    devices = { 0x9a02, 0x9a04, 0x9a12, 0x9a14, 0x9a36, 0x9a26 },
    window = 1,
    offset = 0x48,
    width = 8,
    mask = 0x7ffffe0000,
    enable = 0,
    size = 0x20000,
    d0 = false,
    reads = function()
      return {
        { 0x5058, 0x8, 8 },
        { 0x50a0, 0x8, 8 },
        { 0xd858, 0x8, 8 },
        { 0xd8a0, 0x8, 8 },
        { 0x15058, 0x8, 8 },
        { 0x150a0, 0x8, 8 },
        { 0x1d858, 0x8, 8 },
        { 0x1d8a0, 0x8, 8 },
        { 0x58e0, 0x10, 4 },
        { 0x5930, 0x10, 4 },
        { 0x59a0, 0x8, 4 },
        { 0x5978, 0xc, 4 },
        { 0x599c, 0x4, 4 },
        { 0x5e00, 0x8, 4 },
        { 0x4000, 0x8, 8 },
        { 0x4070, 0x4, 4 },
        { 0x4088, 0x4, 4 },
        { 0x423c, 0x4, 4 },
        { 0x4400, 0x8, 8 },
        { 0x4470, 0x4, 4 },
        { 0x4488, 0x4, 4 },
        { 0x463c, 0x4, 4 },
        { 0x14000, 0x8, 8 },
        { 0x14070, 0x4, 4 },
        { 0x14088, 0x4, 4 },
        { 0x1423c, 0x4, 4 },
        { 0x14400, 0x8, 8 },
        { 0x14470, 0x4, 4 },
        { 0x14488, 0x4, 4 },
        { 0x1463c, 0x4, 4 },
        { 0xe000, 0x8, 8 },
        { 0xe070, 0x4, 4 },
        { 0xe088, 0x4, 4 },
        { 0xe43c, 0x4, 4 },
        { 0xe800, 0x8, 8 },
        { 0xe870, 0x4, 4 },
        { 0xe888, 0x4, 4 },
        { 0xec3c, 0x4, 4 },
        { 0x1e000, 0x8, 8 },
        { 0x1e070, 0x4, 4 },
        { 0x1e088, 0x4, 4 },
        { 0x1e43c, 0x4, 4 },
        { 0x1e800, 0x8, 8 },
        { 0x1e870, 0x4, 4 },
        { 0x1e888, 0x4, 4 },
        { 0x1ec3c, 0x4, 4 },
        { 0x5948, 0x4, 4 },
        { 0x5994, 0x8, 4 },
        { 0x5ef0, 0x4, 4 },
      }
    end,
  },
  {
    family = 0x06,
    models = {
      { 0x7d, 0x7d },
      { 0x7e, 0x7e },
      { 0x9d, 0x9d },
      { 0x8c, 0x8c },
      { 0x8d, 0x8d },
      { 0xa7, 0xa7 },
      { 0xd7, 0xd7 },
    },
    vendor = 0x8086,
    devices = { 0x8a03, 0x9a03 },
    window = 2,
    size = 0x8000,
    d0 = true,
    reads = function()
      return {
        { 0x5820, 0x4, 4 },
        { 0x58e0, 0x10, 4 },
        { 0x58f0, 0x4, 4 },
        { 0x5930, 0x10, 4 },
        { 0x59a0, 0x8, 4 },
      }
    end,
  },
  {
    family = 0x06,
    models = {
      { 0x7d, 0x7d },
      { 0x7e, 0x7e },
      { 0x9d, 0x9d },
      { 0x8c, 0x8c },
      { 0x8d, 0x8d },
      { 0xa7, 0xa7 },
      { 0xd7, 0xd7 },
    },
    vendor = 0x8086,
    devices = {
      0x4639,
      0x463c,
      0x4642,
      0x4643,
      0xa731,
      0xa732,
      0xa733,
      0xa741,
      0xa744,
      0xa745,
    },
    window = 1,
    offset = 0x48,
    width = 8,
    mask = 0x3fffffe0000,
    enable = 0,
    size = 0x20000,
    d0 = false,
    reads = function()
      return {
        { 0x5918, 0x8, 4 },
        { 0x5f60, 0x8, 4 },
        { 0x5058, 0x8, 8 },
        { 0x50a0, 0x8, 8 },
        { 0xd858, 0x8, 8 },
        { 0xd8a0, 0x8, 8 },
        { 0x15058, 0x8, 8 },
        { 0x150a0, 0x8, 8 },
        { 0x1d858, 0x8, 8 },
        { 0x1d8a0, 0x8, 8 },
        { 0x58e0, 0x10, 4 },
        { 0x5930, 0x10, 4 },
        { 0x59a0, 0x8, 4 },
        { 0x5978, 0xc, 4 },
        { 0x599c, 0x4, 4 },
        { 0x5e00, 0x8, 4 },
        { 0xe000, 0x8, 8 },
        { 0xe070, 0x4, 4 },
        { 0xe088, 0x4, 4 },
        { 0xe43c, 0x4, 4 },
        { 0xe800, 0x8, 8 },
        { 0xe870, 0x4, 4 },
        { 0xe888, 0x4, 4 },
        { 0xec3c, 0x4, 4 },
        { 0x1e000, 0x8, 8 },
        { 0x1e070, 0x4, 4 },
        { 0x1e088, 0x4, 4 },
        { 0x1e43c, 0x4, 4 },
        { 0x1e800, 0x8, 8 },
        { 0x1e870, 0x4, 4 },
        { 0x1e888, 0x4, 4 },
        { 0x1ec3c, 0x4, 4 },
        { 0x5948, 0x4, 4 },
        { 0x5994, 0x8, 4 },
        { 0x5ef0, 0x4, 4 },
      }
    end,
  },
  {
    family = 0x06,
    models = {
      { 0x97, 0x97 },
      { 0x9a, 0x9a },
      { 0xb7, 0xb7 },
      { 0xba, 0xba },
      { 0xbf, 0xbf },
      { 0xaa, 0xaa },
      { 0xac, 0xac },
      { 0xc5, 0xc5 },
      { 0xc6, 0xc6 },
      { 0xb5, 0xb5 },
      { 0xbd, 0xbd },
      { 0xcc, 0xcc },
      { 0xd5, 0xd5 },
      { 0xe5, 0xe5 },
    },
    vendor = 0x8086,
    devices = {
      0x4660,
      0x4641,
      0x4601,
      0x4602,
      0x4609,
      0x460a,
      0x4621,
      0x4623,
      0x4629,
      0x4637,
      0x463b,
      0x4648,
      0x4649,
      0x4650,
      0x4668,
      0x4670,
      0x4614,
      0x4617,
      0x4618,
      0x461b,
      0x461c,
      0xa700,
      0xa701,
      0xa702,
      0xa703,
      0xa704,
      0xa705,
      0xa706,
      0xa707,
      0xa708,
      0xa709,
      0xa70a,
      0xa70b,
      0xa715,
      0xa716,
      0xa717,
      0xa718,
      0xa719,
      0xa71a,
      0xa71b,
      0xa71c,
      0xa728,
      0xa729,
      0xa72a,
      0x4640,
      0x4630,
      0xa740,
      0x4664,
      0x466c,
      0x4644,
      0x464c,
      0x4610,
      0x4673,
      0x4661,
      0x4619,
      0x4647,
      0x4639,
      0x463c,
      0x4642,
      0x4643,
      0xa731,
      0xa732,
      0xa733,
      0xa741,
      0xa744,
      0xa745,
    },
    window = 1,
    offset = 0x48,
    width = 8,
    mask = 0x3fffffe0000,
    enable = 0,
    size = 0x20000,
    d0 = false,
    reads = function()
      return {
        { 0x5918, 0x8, 4 },
        { 0x5f60, 0x8, 4 },
        { 0x5058, 0x8, 8 },
        { 0x50a0, 0x8, 8 },
        { 0xd858, 0x8, 8 },
        { 0xd8a0, 0x8, 8 },
        { 0x15058, 0x8, 8 },
        { 0x150a0, 0x8, 8 },
        { 0x1d858, 0x8, 8 },
        { 0x1d8a0, 0x8, 8 },
        { 0x58e0, 0x10, 4 },
        { 0x5930, 0x10, 4 },
        { 0x59a0, 0x8, 4 },
        { 0x5978, 0xc, 4 },
        { 0x599c, 0x4, 4 },
        { 0x5e00, 0x8, 4 },
        { 0xe000, 0x8, 8 },
        { 0xe070, 0x4, 4 },
        { 0xe088, 0x4, 4 },
        { 0xe43c, 0x4, 4 },
        { 0xe800, 0x8, 8 },
        { 0xe870, 0x4, 4 },
        { 0xe888, 0x4, 4 },
        { 0xec3c, 0x4, 4 },
        { 0x1e000, 0x8, 8 },
        { 0x1e070, 0x4, 4 },
        { 0x1e088, 0x4, 4 },
        { 0x1e43c, 0x4, 4 },
        { 0x1e800, 0x8, 8 },
        { 0x1e870, 0x4, 4 },
        { 0x1e888, 0x4, 4 },
        { 0x1ec3c, 0x4, 4 },
        { 0x5948, 0x4, 4 },
        { 0x5994, 0x8, 4 },
        { 0x5ef0, 0x4, 4 },
      }
    end,
  },
  {
    family = 0x06,
    models = {
      { 0x97, 0x97 },
      { 0x9a, 0x9a },
      { 0xb7, 0xb7 },
      { 0xba, 0xba },
      { 0xbf, 0xbf },
      { 0xaa, 0xaa },
      { 0xac, 0xac },
      { 0xc5, 0xc5 },
      { 0xc6, 0xc6 },
      { 0xb5, 0xb5 },
      { 0xbd, 0xbd },
      { 0xcc, 0xcc },
      { 0xd5, 0xd5 },
      { 0xe5, 0xe5 },
    },
    vendor = 0x8086,
    devices = {
      0x7d00,
      0x7d01,
      0x7d02,
      0x7d05,
      0x7d10,
      0x7d14,
      0x7d15,
      0x7d16,
      0x7d21,
      0x7d22,
      0x7d23,
      0x7d24,
      0x7d28,
      0x7d06,
      0x7d1a,
      0x7d1b,
      0x7d1c,
      0x7d20,
      0x7d29,
      0x7d2a,
      0x7d2d,
      0x7d2f,
      0x7d30,
      0x7d35,
    },
    window = 1,
    offset = 0x48,
    width = 8,
    mask = 0x3fffffe0000,
    enable = 0,
    size = 0x20000,
    d0 = false,
    reads = function()
      return {
        { 0x58e0, 0x10, 4 },
        { 0x5918, 0x8, 4 },
        { 0x5930, 0x10, 4 },
        { 0x5978, 0xc, 4 },
        { 0x599c, 0x4, 4 },
        { 0x59a0, 0x8, 4 },
        { 0x5f60, 0x8, 4 },
        { 0x58f0, 0x4, 4 },
        { 0x13d04, 0x4, 4 },
        { 0xd800, 0x4, 4 },
        { 0xd80c, 0x8, 4 },
        { 0x1d800, 0x4, 4 },
        { 0x1d80c, 0x8, 4 },
        { 0xd840, 0x8, 8 },
        { 0x1d840, 0x8, 8 },
        { 0xe424, 0x4, 4 },
        { 0xec24, 0x4, 4 },
        { 0x1e424, 0x4, 4 },
        { 0x1ec24, 0x4, 4 },
        { 0x13d10, 0x4, 4 },
        { 0xd858, 0x8, 8 },
        { 0xd8a0, 0x8, 8 },
        { 0x1d858, 0x8, 8 },
        { 0x1d8a0, 0x8, 8 },
        { 0xe000, 0x8, 8 },
        { 0xe070, 0x4, 4 },
        { 0xe088, 0x4, 4 },
        { 0xe138, 0x4, 4 },
        { 0xe4a0, 0x4, 4 },
        { 0xe800, 0x8, 8 },
        { 0xe870, 0x4, 4 },
        { 0xe888, 0x4, 4 },
        { 0xe938, 0x4, 4 },
        { 0xeca0, 0x4, 4 },
        { 0x1e000, 0x8, 8 },
        { 0x1e070, 0x4, 4 },
        { 0x1e088, 0x4, 4 },
        { 0x1e138, 0x4, 4 },
        { 0x1e4a0, 0x4, 4 },
        { 0x1e800, 0x8, 8 },
        { 0x1e870, 0x4, 4 },
        { 0x1e888, 0x4, 4 },
        { 0x1e938, 0x4, 4 },
        { 0x1eca0, 0x4, 4 },
      }
    end,
  },
  {
    family = 0x06,
    models = {
      { 0x97, 0x97 },
      { 0x9a, 0x9a },
      { 0xb7, 0xb7 },
      { 0xba, 0xba },
      { 0xbf, 0xbf },
      { 0xaa, 0xaa },
      { 0xac, 0xac },
      { 0xc5, 0xc5 },
      { 0xc6, 0xc6 },
      { 0xb5, 0xb5 },
      { 0xbd, 0xbd },
      { 0xcc, 0xcc },
      { 0xd5, 0xd5 },
      { 0xe5, 0xe5 },
    },
    vendor = 0x8086,
    devices = { 0x6400, 0x6410 },
    window = 1,
    offset = 0x48,
    width = 8,
    mask = 0x3fffffe0000,
    enable = 0,
    size = 0x20000,
    d0 = false,
    reads = function()
      return {
        { 0x58e0, 0x10, 4 },
        { 0x5918, 0x8, 4 },
        { 0x5930, 0x10, 4 },
        { 0x5978, 0xc, 4 },
        { 0x599c, 0x4, 4 },
        { 0x59a0, 0x8, 4 },
        { 0x5f60, 0x8, 4 },
        { 0x58f0, 0x4, 4 },
        { 0x13d04, 0x4, 4 },
        { 0xd800, 0x4, 4 },
        { 0xd80c, 0x8, 4 },
        { 0x1d800, 0x4, 4 },
        { 0x1d80c, 0x8, 4 },
        { 0xd840, 0x8, 8 },
        { 0x1d840, 0x8, 8 },
        { 0xe424, 0x4, 4 },
        { 0xec24, 0x4, 4 },
        { 0x1e424, 0x4, 4 },
        { 0x1ec24, 0x4, 4 },
        { 0xd858, 0x8, 8 },
        { 0xd8a0, 0x8, 8 },
        { 0x1d858, 0x8, 8 },
        { 0x1d8a0, 0x8, 8 },
        { 0xe000, 0x8, 8 },
        { 0xe070, 0x4, 4 },
        { 0xe138, 0x4, 4 },
        { 0xe4a0, 0x4, 4 },
        { 0xe800, 0x8, 8 },
        { 0xe870, 0x4, 4 },
        { 0xe938, 0x4, 4 },
        { 0xeca0, 0x4, 4 },
        { 0x1e000, 0x8, 8 },
        { 0x1e070, 0x4, 4 },
        { 0x1e138, 0x4, 4 },
        { 0x1e4a0, 0x4, 4 },
        { 0x1e800, 0x8, 8 },
        { 0x1e870, 0x4, 4 },
        { 0x1e938, 0x4, 4 },
        { 0x1eca0, 0x4, 4 },
      }
    end,
  },
  {
    family = 0x06,
    models = {
      { 0x97, 0x97 },
      { 0x9a, 0x9a },
      { 0xb7, 0xb7 },
      { 0xba, 0xba },
      { 0xbf, 0xbf },
      { 0xaa, 0xaa },
      { 0xac, 0xac },
      { 0xc5, 0xc5 },
      { 0xc6, 0xc6 },
      { 0xb5, 0xb5 },
      { 0xbd, 0xbd },
      { 0xcc, 0xcc },
      { 0xd5, 0xd5 },
      { 0xe5, 0xe5 },
    },
    vendor = 0x8086,
    devices = {
      0xb000,
      0xb001,
      0xb002,
      0xb003,
      0xb004,
      0xb005,
      0xb008,
      0xb009,
      0xb00a,
      0xb00b,
      0xb00c,
      0xb011,
      0xb014,
      0xb015,
      0xb01a,
      0xb028,
      0xb029,
      0xb02a,
      0xb02b,
      0xfd00,
      0xfd01,
      0xfd02,
      0xfd03,
      0xfd04,
    },
    window = 1,
    offset = 0x48,
    width = 8,
    mask = 0x3fffffe0000,
    enable = 0,
    size = 0x20000,
    d0 = false,
    reads = function()
      return {
        { 0x58e0, 0x10, 4 },
        { 0x5918, 0x8, 4 },
        { 0x5930, 0x10, 4 },
        { 0x5978, 0xc, 4 },
        { 0x599c, 0x4, 4 },
        { 0x59a0, 0x8, 4 },
        { 0x5f60, 0x8, 4 },
        { 0x58f0, 0x4, 4 },
        { 0x13d04, 0x4, 4 },
        { 0xd800, 0x4, 4 },
        { 0xd80c, 0x8, 4 },
        { 0x1d800, 0x4, 4 },
        { 0x1d80c, 0x8, 4 },
        { 0xd840, 0x8, 8 },
        { 0x1d840, 0x8, 8 },
        { 0xe424, 0x4, 4 },
        { 0xec24, 0x4, 4 },
        { 0x1e424, 0x4, 4 },
        { 0x1ec24, 0x4, 4 },
        { 0x13d10, 0x4, 4 },
        { 0xd858, 0x8, 8 },
        { 0xd8a0, 0x8, 8 },
        { 0x1d858, 0x8, 8 },
        { 0x1d8a0, 0x8, 8 },
        { 0xe000, 0x8, 8 },
        { 0xe070, 0x4, 4 },
        { 0xe088, 0x4, 4 },
        { 0xe138, 0x4, 4 },
        { 0xe4a0, 0x4, 4 },
        { 0xe800, 0x8, 8 },
        { 0xe870, 0x4, 4 },
        { 0xe888, 0x4, 4 },
        { 0xe938, 0x4, 4 },
        { 0xeca0, 0x4, 4 },
        { 0x1e000, 0x8, 8 },
        { 0x1e070, 0x4, 4 },
        { 0x1e088, 0x4, 4 },
        { 0x1e138, 0x4, 4 },
        { 0x1e4a0, 0x4, 4 },
        { 0x1e800, 0x8, 8 },
        { 0x1e870, 0x4, 4 },
        { 0x1e888, 0x4, 4 },
        { 0x1e938, 0x4, 4 },
        { 0x1eca0, 0x4, 4 },
      }
    end,
  },
  {
    family = 0x06,
    models = {
      { 0x97, 0x97 },
      { 0x9a, 0x9a },
      { 0xb7, 0xb7 },
      { 0xba, 0xba },
      { 0xbf, 0xbf },
      { 0xaa, 0xaa },
      { 0xac, 0xac },
      { 0xc5, 0xc5 },
      { 0xc6, 0xc6 },
      { 0xb5, 0xb5 },
      { 0xbd, 0xbd },
      { 0xcc, 0xcc },
      { 0xd5, 0xd5 },
      { 0xe5, 0xe5 },
    },
    vendor = 0x8086,
    devices = { 0x461d, 0xa71d, 0x7d03, 0xad03, 0x641d, 0xb01d, 0xfd1d },
    window = 2,
    size = 0x8000,
    d0 = true,
    reads = function()
      return {
        { 0x5820, 0x4, 4 },
        { 0x58e0, 0x10, 4 },
        { 0x58f0, 0x4, 4 },
        { 0x5930, 0x10, 4 },
        { 0x59a0, 0x8, 4 },
        { 0x5978, 0xc, 4 },
        { 0x599c, 0x4, 4 },
      }
    end,
  },
  {
    family = 0x06,
    models = { { 0xbe, 0xbe } },
    vendor = 0x8086,
    devices = {
      0x4614,
      0x4617,
      0x461b,
      0x461c,
      0x4673,
      0x4674,
      0x4675,
      0x4677,
      0x4678,
      0x4679,
      0x467c,
      0x4632,
      0x4676,
      0x464a,
      0x4646,
      0x4652,
    },
    window = 1,
    offset = 0x48,
    width = 8,
    mask = 0x3fffffe0000,
    enable = 0,
    size = 0x20000,
    d0 = false,
    reads = function()
      return {
        { 0x5918, 0x8, 4 },
        { 0x5f60, 0x8, 4 },
        { 0x5058, 0x8, 8 },
        { 0x50a0, 0x8, 8 },
        { 0xd858, 0x8, 8 },
        { 0xd8a0, 0x8, 8 },
        { 0x15058, 0x8, 8 },
        { 0x150a0, 0x8, 8 },
        { 0x1d858, 0x8, 8 },
        { 0x1d8a0, 0x8, 8 },
        { 0x58e0, 0x10, 4 },
        { 0x5930, 0x10, 4 },
        { 0x59a0, 0x8, 4 },
        { 0x5978, 0xc, 4 },
        { 0x599c, 0x4, 4 },
        { 0x5e00, 0x8, 4 },
        { 0xe000, 0x8, 8 },
        { 0xe070, 0x4, 4 },
        { 0xe088, 0x4, 4 },
        { 0xe43c, 0x4, 4 },
        { 0xe800, 0x8, 8 },
        { 0xe870, 0x4, 4 },
        { 0xe888, 0x4, 4 },
        { 0xec3c, 0x4, 4 },
        { 0x1e000, 0x8, 8 },
        { 0x1e070, 0x4, 4 },
        { 0x1e088, 0x4, 4 },
        { 0x1e43c, 0x4, 4 },
        { 0x1e800, 0x8, 8 },
        { 0x1e870, 0x4, 4 },
        { 0x1e888, 0x4, 4 },
        { 0x1ec3c, 0x4, 4 },
        { 0x5948, 0x4, 4 },
        { 0x5994, 0x8, 4 },
        { 0x5ef0, 0x4, 4 },
      }
    end,
  },
  {
    family = 0x06,
    models = {
      { 0x5c, 0x5c },
      { 0x5f, 0x5f },
      { 0x7a, 0x7a },
      { 0x86, 0x86 },
      { 0x96, 0x96 },
      { 0x9c, 0x9c },
    },
    vendor = 0x8086,
    devices = { 0x5af0, 0x31f0 },
    window = 1,
    offset = 0x48,
    width = 8,
    mask = 0x7fffff8000,
    enable = 0,
    size = 0x8000,
    d0 = false,
    reads = function()
      return {
        { 0x1000, 0x4, 4 },
        { 0x1200, 0x4, 4 },
        { 0x1400, 0x4, 4 },
        { 0x1600, 0x4, 4 },
        { 0x7070, 0x4, 4 },
      }
    end,
  },
  {
    family = 0x06,
    models = {
      { 0x5c, 0x5c },
      { 0x5f, 0x5f },
      { 0x7a, 0x7a },
      { 0x86, 0x86 },
      { 0x96, 0x96 },
      { 0x9c, 0x9c },
    },
    vendor = 0x8086,
    devices = {
      0x4e22,
      0x4e26,
      0x4e12,
      0x4e14,
      0x4e24,
      0x4e28,
      0x4510,
      0x4512,
      0x4514,
      0x4516,
      0x4518,
      0x451a,
      0x451c,
      0x451e,
      0x4522,
      0x4524,
      0x4526,
      0x4528,
      0x452a,
      0x452c,
      0x452e,
      0x4532,
      0x4536,
      0x4538,
      0x453a,
    },
    window = 1,
    offset = 0x48,
    width = 8,
    mask = 0x7fffff8000,
    enable = 0,
    size = 0x8000,
    d0 = false,
    reads = function()
      return {
        { 0x5000, 0x4, 4 },
        { 0x500c, 0x8, 4 },
        { 0x58e0, 0x8, 4 },
        { 0x5948, 0x4, 4 },
        { 0x5994, 0x8, 4 },
        { 0x59a0, 0x8, 4 },
        { 0x5ef0, 0x4, 4 },
      }
    end,
  },
  {
    family = 0x12,
    models = { { 0x01, 0x01 }, { 0x03, 0x03 } },
    vendor = 0x8086,
    devices = { 0xd701, 0xd702, 0xd704, 0xd705, 0xd70e },
    window = 1,
    offset = 0x48,
    width = 8,
    mask = 0x3fffffe0000,
    enable = 0,
    size = 0x20000,
    d0 = false,
    reads = function()
      return {
        { 0x12904, 0x4, 4 },
        { 0xd800, 0x4, 4 },
        { 0xd80c, 0x8, 4 },
        { 0x1d800, 0x4, 4 },
        { 0x1d80c, 0x8, 4 },
        { 0xd840, 0x8, 8 },
        { 0x1d840, 0x8, 8 },
        { 0xd858, 0x8, 8 },
        { 0xd8a0, 0x8, 8 },
        { 0x1d858, 0x8, 8 },
        { 0x1d8a0, 0x8, 8 },
      }
    end,
  },
  {
    family = 0x12,
    models = { { 0x01, 0x01 }, { 0x03, 0x03 } },
    vendor = 0x8086,
    devices = { 0xd703, 0xad03 },
    window = 2,
    size = 0x8000,
    d0 = true,
    reads = function()
      return {
        { 0x5820, 0x4, 4 },
        { 0x58e0, 0x10, 4 },
        { 0x58f0, 0x4, 4 },
        { 0x5930, 0x10, 4 },
        { 0x59a0, 0x8, 4 },
        { 0x5978, 0xc, 4 },
        { 0x599c, 0x4, 4 },
      }
    end,
  },
  {
    vendor = 0x8086,
    devices = {
      0x9c24,
      0x8c24,
      0x9ca4,
      0x9d31,
      0xa131,
      0x9df9,
      0xa379,
      0x02f9,
      0x06f9,
      0xa1b1,
      0x8d24,
      0xa2b1,
      0x8ca4,
    },
    window = 2,
    size = 0x100,
    d0 = true,
    reads = function()
      return {
        { 0x0000, 0x2, 2 },
        { 0x0008, 0x1, 1 },
      }
    end,
  },
}

local PAGE = 0x1000
local HUGE_PAGE_END = -PAGE
local MC_BASE = 0xd0
local MC_BASE_MASK = 0x1fffffff
local MC_BASE_SHIFT = 23
local MC_MEM0 = 0xd8
local MC_MEM_MASK = 0x7ff
local MC_MEM_SHIFT = 12
local WINDOW_LAST = 6

local kept = {}

-- The header bytes a function is identified by.
local function header_key(header)
  return string.sub(header, 1, 4) .. string.sub(header, 9, 12) .. string.sub(header, 15, 15)
end

local function cpu_identity()
  if kept.cpu then
    return kept.cpu
  end
  local items = string.pack('<I2I2I4I4', 0, 0, 0, 0) .. string.pack('<I2I2I4I4', 0, 0, 1, 0)
  local results, status = ksi.hw.cpuid(items)
  if not results then
    ksi.trace('cpuid failed: 0x%x', status)
    return nil, status
  end
  local s0, _, b, c, d = string.unpack('<i4I4I4I4I4', results, 1)
  local s1, signature = string.unpack('<i4I4', results, 21)
  if not nt.NT_SUCCESS(s0) or not nt.NT_SUCCESS(s1) then
    ksi.trace('CPUID of processor 0:0 failed: 0x%x 0x%x', s0, s1)
    return nil, not nt.NT_SUCCESS(s0) and s0 or s1
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
  kept.cpu = {
    vendor = string.pack('<I4I4I4', b, d, c),
    family = family,
    model = model,
  }
  return kept.cpu
end

local function row_matches(row, cpu, vendor, device)
  if row.vendor ~= vendor then
    return false
  end
  if row.family then
    if cpu.vendor ~= 'GenuineIntel' or row.family ~= cpu.family then
      return false
    end
    local modelMatch = false
    for _, range in ipairs(row.models) do
      if cpu.model >= range[1] and cpu.model <= range[2] then
        modelMatch = true
      end
    end
    if not modelMatch then
      return false
    end
  end
  for _, id in ipairs(row.devices) do
    if id == device then
      return true
    end
  end
  return false
end

-- Identifies the processor and the PCI function at a locator. Returns the
-- session, or nil and the status of the call.
local function session(locator)
  if not ksi.hw.pci_open or not ksi.hw.cpuid then
    ksi.trace('no PCI or CPUID functions')
    return nil, nt.STATUS_NOT_SUPPORTED
  end
  local cpu, failed = cpu_identity()
  if not cpu then
    return nil, failed
  end
  local k = kept[locator]
  if k then
    local header = ksi.hw.pci_read_block(k.device, 0, 16)
    if header and header_key(header) == k.key then
      return {
        device = k.device,
        kept = k,
        fixed = true,
        vendor = k.vendor,
        id = k.id,
        rows = k.rows,
        header = k.type,
      }
    end
    ksi.trace('the function kept for 0x%x changed', locator)
    kept[locator] = nil
    for _, w in pairs(k.maps) do
      w.map:release()
    end
    k.device:release()
  end
  local device, status =
    nt.pci.open(locator >> 16, (locator >> 8) & 0xff, (locator >> 3) & 0x1f, locator & 0x7)
  if not device then
    ksi.trace('no PCI function at 0x%x: 0x%x', locator, status)
    return nil, status
  end
  local header, readFailed = ksi.hw.pci_read_block(device, 0, 16)
  if not header then
    ksi.trace('configuration header of 0x%x not read: 0x%x', locator, readFailed)
    local refused = nt.pci.refused(device, readFailed)
    device:release()
    return nil, refused
  end
  local vendor, id = string.unpack('<I2I2', header, 1)
  if vendor == 0xffff then
    ksi.trace('no device answers at 0x%x', locator)
    device:release()
    return nil, nt.STATUS_NOT_FOUND
  end
  local rows = {}
  for _, row in ipairs(ROWS) do
    if row_matches(row, cpu, vendor, id) then
      rows[#rows + 1] = row
    end
  end
  if #rows == 0 then
    ksi.trace(
      'no row for %s family 0x%x model 0x%x and PCI %04x:%04x',
      cpu.vendor,
      cpu.family,
      cpu.model,
      vendor,
      id
    )
    device:release()
    return nil, nt.STATUS_NOT_SUPPORTED
  end
  local s = {
    device = device,
    vendor = vendor,
    id = id,
    rows = rows,
    header = string.byte(header, 15) & 0x7f,
  }
  if not ksi.volatile then
    s.fixed = not nt.is_device_removable(device)
    if s.fixed then
      local reads = {}
      for _, row in ipairs(rows) do
        reads[row] = row.reads()
      end
      s.kept = {
        device = device,
        key = header_key(header),
        vendor = vendor,
        id = id,
        rows = rows,
        type = s.header,
        reads = reads,
        maps = {},
      }
      kept[locator] = s.kept
    end
  end
  return s
end

local function address_bits()
  if kept.bits then
    return kept.bits
  end
  local results, status = ksi.hw.cpuid(
    string.pack('<I2I2I4I4', 0, 0, 0x80000000, 0) .. string.pack('<I2I2I4I4', 0, 0, 0x80000008, 0)
  )
  if not results then
    ksi.trace('cpuid failed: 0x%x', status)
    return nil, status
  end
  local s0, maximum = string.unpack('<i4I4', results, 1)
  local s1, eax = string.unpack('<i4I4', results, 21)
  if
    nt.NT_SUCCESS(s0)
    and maximum >= 0x80000008
    and nt.NT_SUCCESS(s1)
    and eax & 0xff >= 32
    and eax & 0xff < 64
  then
    kept.bits = eax & 0xff
    return kept.bits
  end
  if nt.NT_SUCCESS(s0) and nt.NT_SUCCESS(s1) then
    kept.bits = 36
  end
  return 36
end

local function config_read(s, offset, width, ones)
  local value = ksi.hw.pci_read(s.device, offset, width)
  if not value or value == ones then
    ksi.trace('configuration 0x%x of %04x:%04x not read', offset, s.vendor, s.id)
    return nil
  end
  return value
end

-- The power-management capability: 0 when the device has none, nil and a
-- status when the list cannot be walked.
local function pm_capability(s)
  local pciStatus = config_read(s, 0x06, 2, 0xffff)
  if not pciStatus then
    return nil, nt.STATUS_DEVICE_NOT_READY
  end
  if pciStatus & 0x10 == 0 then
    return 0
  end
  local pointerOffset
  if s.header == 0 or s.header == 1 then
    pointerOffset = 0x34
  elseif s.header == 2 then
    pointerOffset = 0x14
  else
    ksi.trace('header type 0x%x has no capability list', s.header)
    return nil, nt.STATUS_DEVICE_CONFIGURATION_ERROR
  end
  local pointer = ksi.hw.pci_read(s.device, pointerOffset, 1)
  if not pointer then
    ksi.trace('capability pointer of %04x:%04x not read', s.vendor, s.id)
    return nil, nt.STATUS_DEVICE_NOT_READY
  end
  local visited = {}
  pointer = pointer & 0xfc
  while pointer ~= 0 do
    if pointer < 0x40 then
      ksi.trace('capability at 0x%x inside the header', pointer)
      return nil, nt.STATUS_DEVICE_CONFIGURATION_ERROR
    end
    if visited[pointer] then
      ksi.trace('capability list loops at 0x%x', pointer)
      return nil, nt.STATUS_DEVICE_CONFIGURATION_ERROR
    end
    visited[pointer] = true
    local capability = config_read(s, pointer, 2, 0xffff)
    if not capability then
      return nil, nt.STATUS_DEVICE_NOT_READY
    end
    if capability & 0xff == 0x01 then
      return pointer
    end
    pointer = (capability >> 8) & 0xfc
  end
  return 0
end

local function check_d0(s)
  if not s.pm then
    local pm, failed = pm_capability(s)
    if not pm then
      return failed
    end
    s.pm = pm
  end
  local pm = s.pm
  if pm == 0 then
    return nt.STATUS_SUCCESS
  end
  local pmcsr = config_read(s, pm + 4, 2, 0xffff)
  if not pmcsr then
    return nt.STATUS_DEVICE_NOT_READY
  end
  if pmcsr & 0x3 ~= 0 then
    ksi.trace('%04x:%04x is in D%d', s.vendor, s.id, pmcsr & 0x3)
    return nt.STATUS_DEVICE_POWERED_OFF
  end
  return nt.STATUS_SUCCESS
end

local function memory_bar(s, index)
  local command = config_read(s, 0x04, 2, 0xffff)
  if not command then
    return nil, nt.STATUS_DEVICE_NOT_READY
  end
  if command & 0x2 == 0 then
    ksi.trace('%04x:%04x does not decode memory', s.vendor, s.id)
    return nil, nt.STATUS_DEVICE_NOT_READY
  end
  local count
  if s.header == 0 then
    count = 6
  elseif s.header == 1 then
    count = 2
  else
    ksi.trace('header type 0x%x has no BARs', s.header)
    return nil, nt.STATUS_DEVICE_CONFIGURATION_ERROR
  end
  local base
  local i = 0
  while i < count and i <= index do
    local low = config_read(s, 0x10 + i * 4, 4, 0xffffffff)
    if not low then
      return nil, nt.STATUS_DEVICE_NOT_READY
    end
    if low & 0x1 ~= 0 or low & 0x6 ~= 0x4 then
      if i == index then
        if low & 0x1 ~= 0 or low & 0x6 ~= 0 then
          ksi.trace('BAR %d is not 32-bit memory: 0x%x', i, low)
          return nil, nt.STATUS_DEVICE_CONFIGURATION_ERROR
        end
        base = low & 0xfffffff0
        break
      end
      i = i + 1
    else
      if i + 1 >= count or i + 1 == index then
        ksi.trace('BAR %d is the high half of BAR %d', index, i)
        return nil, nt.STATUS_DEVICE_CONFIGURATION_ERROR
      end
      if i == index then
        local high = config_read(s, 0x10 + i * 4 + 4, 4, 0xffffffff)
        if not high then
          return nil, nt.STATUS_DEVICE_NOT_READY
        end
        base = (high << 32) | (low & 0xfffffff0)
        break
      end
      i = i + 2
    end
  end
  if not base then
    ksi.trace('BAR %d beyond the %d BARs', index, count)
    return nil, nt.STATUS_DEVICE_CONFIGURATION_ERROR
  end
  if base == 0 then
    ksi.trace('BAR %d of %04x:%04x is not assigned', index, s.vendor, s.id)
    return nil, nt.STATUS_DEVICE_NOT_READY
  end
  return base
end

local function range_length(s, base)
  local resources, failed = nt.resources(s.device)
  if not resources then
    ksi.trace('resources of %04x:%04x not read: 0x%x', s.vendor, s.id, failed)
    return nil, failed
  end
  for _, r in ipairs(resources) do
    if r.kind == 1 and r.raw == base then
      if r.translated ~= base then
        ksi.trace('BAR 0x%x translates to 0x%x', base, r.translated)
        return nil, nt.STATUS_DEVICE_CONFIGURATION_ERROR
      end
      return r.length
    end
  end
  ksi.trace('no memory resource at 0x%x', base)
  return nil, nt.STATUS_DEVICE_CONFIGURATION_ERROR
end

-- The window's base, from the configuration register or the BAR; with length,
-- a BAR is also checked against the device's resources.
local function derive(s, row, length)
  if row.d0 then
    local st = check_d0(s)
    if not nt.NT_SUCCESS(st) then
      return nil, st
    end
  end
  local base
  if row.controller then
    local high = ksi.hw.pci_read(s.device, MC_BASE, 4)
    local low = ksi.hw.pci_read(s.device, MC_MEM0 + 4 * row.controller, 4)
    if not high or not low or high == 0xffffffff or low == 0xffffffff then
      ksi.trace('controller %d base registers not read', row.controller)
      return nil, nt.STATUS_DEVICE_NOT_READY
    end
    base = ((high & MC_BASE_MASK) << MC_BASE_SHIFT) | ((low & MC_MEM_MASK) << MC_MEM_SHIFT)
    if base == 0 then
      ksi.trace('controller %d window is not assigned', row.controller)
      return nil, nt.STATUS_DEVICE_NOT_READY
    end
  elseif row.window == 2 then
    local failed
    base, failed = memory_bar(s, 0)
    if not base then
      return nil, failed
    end
    if length then
      local size, sizeFailed = range_length(s, base)
      if not size then
        return nil, sizeFailed
      end
      if math.ult(size, row.size) then
        ksi.trace('BAR length 0x%x below window size 0x%x', size, row.size)
        return nil, nt.STATUS_DEVICE_CONFIGURATION_ERROR
      end
    end
  else
    local value = ksi.hw.pci_read(s.device, row.offset, 4)
    local high = 0
    if value and row.width == 8 then
      high = ksi.hw.pci_read(s.device, row.offset + 4, 4)
    end
    local ones = row.width == 8 and -1 or 0xffffffff
    if value and high then
      value = value | (high << 32)
    end
    if not value or not high or value == ones then
      ksi.trace('window base register 0x%x not read', row.offset)
      return nil, nt.STATUS_DEVICE_NOT_READY
    end
    if row.enable and value & (1 << row.enable) == 0 then
      ksi.trace('window base register 0x%x is disabled: 0x%x', row.offset, value)
      return nil, nt.STATUS_DEVICE_NOT_READY
    end
    base = value & row.mask
    if base == 0 then
      ksi.trace('window base register 0x%x is not assigned', row.offset)
      return nil, nt.STATUS_DEVICE_NOT_READY
    end
  end
  if base & ((row.align or row.size) - 1) ~= 0 then
    ksi.trace('window base 0x%x is not aligned to 0x%x', base, row.align or row.size)
    return nil, nt.STATUS_DEVICE_CONFIGURATION_ERROR
  end
  return base
end

local function permitted(ranges, offset, width)
  for _, r in ipairs(ranges) do
    if r[3] == width and offset >= r[1] and offset - r[1] < r[2] then
      return true
    end
  end
  return false
end

local function all_ones(width)
  if width == 8 then
    return -1
  end
  return (1 << (width * 8)) - 1
end

-- Returns the PCI IDs at a locator and the windows the rows give it.
function identify(input)
  if #input ~= 4 then
    ksi.trace('input of %d bytes', #input)
    return nt.STATUS_INVALID_PARAMETER
  end
  local s, refused = session(string.unpack('<I4', input))
  if not s then
    return refused
  end
  if not s.kept then
    s.device:release()
  end
  local mask = 0
  for _, row in ipairs(s.rows) do
    mask = mask | (1 << (row.window - 1))
  end
  return nt.STATUS_SUCCESS, string.pack('<I2I2I4', s.vendor, s.id, mask)
end

-- Reads registers of a window of the PCI function at a locator.
function read(input)
  if #input < 16 or (#input - 8) % 8 ~= 0 then
    ksi.trace('input of %d bytes', #input)
    return nt.STATUS_INVALID_PARAMETER
  end
  local locator, window = string.unpack('<I4I4', input)
  if window < 1 or window > WINDOW_LAST then
    ksi.trace('window %d', window)
    return nt.STATUS_INVALID_PARAMETER
  end
  local entries = {}
  for i = 1, (#input - 8) // 8 do
    local offset, width = string.unpack('<I4I4', input, 9 + (i - 1) * 8)
    if (width ~= 1 and width ~= 2 and width ~= 4 and width ~= 8) or offset % width ~= 0 then
      ksi.trace('entry %d: offset 0x%x width %d', i, offset, width)
      return nt.STATUS_INVALID_PARAMETER
    end
    entries[i] = { offset = offset, width = width }
  end

  local s, refused = session(locator)
  if not s then
    return refused
  end
  local closer <close> = (not s.kept) and s.device or nil

  local row
  for _, r in ipairs(s.rows) do
    if r.window == window then
      row = r
      break
    end
  end
  if not row then
    ksi.trace('%04x:%04x has no window %d', s.vendor, s.id, window)
    return nt.STATUS_NOT_SUPPORTED
  end
  local fixed = s.fixed
  if fixed == nil then
    fixed = not nt.is_device_removable(s.device)
  end
  if not fixed then
    ksi.trace('%04x:%04x may be removed', s.vendor, s.id)
    return nt.STATUS_NOT_SUPPORTED
  end

  local ranges = s.kept and s.kept.reads[row] or row.reads()
  local spanBegin, spanEnd
  for _, e in ipairs(entries) do
    e.status, e.value = nt.STATUS_ACCESS_DENIED, 0
    if permitted(ranges, e.offset, e.width) then
      e.status = nil
      if not spanBegin or e.offset < spanBegin then
        spanBegin = e.offset
      end
      if not spanEnd or e.offset + e.width > spanEnd then
        spanEnd = e.offset + e.width
      end
    else
      ksi.trace('offset 0x%x width %d is not permitted', e.offset, e.width)
    end
  end

  if spanBegin then
    spanBegin = spanBegin & -8
    local maps = s.kept and s.kept.maps
    local w = maps and maps[window]
    local base, failed = derive(s, row, not w)
    local had = w ~= nil
    if w and (not base or w.base ~= base) then
      maps[window] = nil
      w.map:release()
      w = nil
    elseif w and (spanBegin < w.first or spanEnd > w.last) then
      spanBegin = math.min(spanBegin, w.first)
      spanEnd = math.max(spanEnd, w.last)
      maps[window] = nil
      w.map:release()
      w = nil
    end
    if had and not w and base then
      base, failed = derive(s, row, true)
    end
    if not base then
      return failed
    end
    local m, first
    if w then
      m, first = w.map, w.first
    else
      local windowEnd = base + row.size
      if math.ult(windowEnd, base) or math.ult(HUGE_PAGE_END, windowEnd) then
        ksi.trace('window 0x%x size 0x%x ends beyond the address ' .. 'space', base, row.size)
        return nt.STATUS_DEVICE_CONFIGURATION_ERROR
      end
      local bits, bitsFailed = address_bits()
      if not bits then
        return bitsFailed
      end
      local st = nt.check_range(base & -PAGE, (windowEnd + PAGE - 1) & -PAGE, bits)
      if not nt.NT_SUCCESS(st) then
        return st
      end
      local mapFailed
      m, mapFailed = ksi.hw.map(base + spanBegin, spanEnd - spanBegin)
      if not m then
        ksi.trace('window 0x%x not mapped: 0x%x', base, mapFailed)
        return mapFailed
      end
      first = spanBegin
      if maps then
        maps[window] = { map = m, base = base, first = spanBegin, last = spanEnd }
      end
    end
    local mapping <close> = (not maps) and m or nil
    local stopped = false
    for _, e in ipairs(entries) do
      if e.status == nil then
        if stopped then
          e.status = nt.STATUS_CANCELLED
        else
          e.value = ksi.hw.map_read(m, e.offset - first, e.width)
          e.status = nt.STATUS_SUCCESS
          if e.value == all_ones(e.width) then
            local again, refusedAgain = derive(s, row, false)
            if not again or again ~= base then
              ksi.trace(
                'offset 0x%x read all-ones; the ' .. 'window is gone: 0x%x',
                e.offset,
                refusedAgain or 0
              )
              e.status, e.value = nt.STATUS_DEVICE_NOT_READY, 0
              stopped = true
            end
          end
        end
      end
    end
    if not maps then
      m:release()
    end

    local check, checkFailed = derive(s, row, not w)
    if not check or check ~= base then
      ksi.trace('window base 0x%x changed to 0x%x: 0x%x', base, check or 0, checkFailed or 0)
      if maps and maps[window] then
        local gone = maps[window]
        maps[window] = nil
        gone.map:release()
      end
      for _, e in ipairs(entries) do
        if nt.NT_SUCCESS(e.status) then
          e.status, e.value = nt.STATUS_DEVICE_NOT_READY, 0
        end
      end
    end
  end

  local output = {}
  for i, e in ipairs(entries) do
    output[i] = string.pack('<i4I4I8', e.status, 0, e.value)
  end
  return nt.STATUS_SUCCESS, table.concat(output)
end
