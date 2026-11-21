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
-- Identifies a Super I/O chip at a configuration port and reads its hardware
-- monitor registers. The chips and registers each row permits are listed
-- below.
--

local FAMILY_NUVOTON = 1
local FAMILY_NCT668X = 2
local FAMILY_ITE = 3
local FAMILY_FINTEK = 4
local FAMILY_SMSC = 5

local SLOTS = { [0] = 0x2e, [1] = 0x4e }

local CHASSIS = { { 0x01, 0x07 }, { 0x0f, 0x1d } }

local BASE_MIN = 0x0100
local BASE_LAST = 0x0ccf

local EXIT_NONE = 0
local EXIT_KEY_AA = 1
local EXIT_CONFIG_02 = 2

local BANK = 0x4e
local PAGE_FREE = 0xff
local PAGE_WAIT = 500000

local kept = {}

local function ite_monitor()
  return {
    ids = { { 0x58, 0x90, 0x7f } },
    reads = {
      { 0x00, 0x1 },
      { 0x06, 0x1 },
      { 0x0b, 0x5 },
      { 0x13, 0x8 },
      { 0x1e, 0x2 },
      { 0x20, 0x10 },
      { 0x4c, 0x2 },
      { 0x51, 0x1 },
      { 0x56, 0x4 },
      { 0x60, 0x20 },
      { 0x80, 0x4 },
      { 0x92, 0x3 },
      { 0xa0, 0x10 },
    },
  }
end

local ITE_ROWS = {
  {
    chips = {
      0x8705,
      0x8712,
      0x8716,
      0x8718,
      0x8720,
      0x8721,
      0x8726,
      0x8728,
      0x8732,
      0x8771,
      0x8772,
      0x8781,
      0x8782,
      0x8783,
      0x8786,
      0x8603,
      0x8613,
      0x8620,
      0x8622,
      0x8623,
      0x8625,
      0x8628,
      0x8631,
      0x8638,
      0x8655,
      0x8665,
      0x8686,
      0x8688,
      0x8689,
      0x8696,
      0x8698,
      0x8606,
      0x8607,
      0x8736,
      0x8738,
      0x8785,
    },
    ldn = 0x04,
    exit = EXIT_CONFIG_02,
    bank_mask = 0,
    monitor = ite_monitor,
  },
  {
    chips = { 0x8790, 0x8733, 0x8695 },
    ldn = 0x04,
    exit = EXIT_NONE,
    bank_mask = 0,
    monitor = ite_monitor,
  },
}

local NUVOTON_ROWS = {
  {
    chips = {
      0xc450,
      0xc451,
      0xc452,
      0xc453,
      0xc454,
      0xc455,
      0xc456,
      0xc457,
    },
    ldn = 0x0b,
    exit = EXIT_KEY_AA,
    bank_mask = 0xff,
    monitor = function()
      return {
        ids = {
          { 0x80fe, 0x5c, 0x5c },
          { 0x00fe, 0xa3, 0xa3 },
        },
        reads = {
          { 0x00fe, 0x1 },
          { 0x80fe, 0x1 },
          { 0x0000, 0x1c },
          { 0x0020, 0x6 },
          { 0x0030, 0x6 },
          { 0x004a, 0x3 },
          { 0x0051, 0x4 },
          { 0x0059, 0x10 },
          { 0x006b, 0x1 },
          { 0x0090, 0x14 },
          { 0x00b0, 0xd },
          { 0x00c0, 0x18 },
          { 0x00e0, 0x6 },
          { 0x00f3, 0x1 },
          { 0x00f6, 0x1 },
          { 0x0110, 0xe },
          { 0x0120, 0xe },
          { 0x0130, 0xe },
          { 0x0160, 0xe },
          { 0x0170, 0xe },
          { 0x0180, 0xe },
          { 0x0204, 0x2 },
          { 0x0300, 0xa },
          { 0x0311, 0x3 },
          { 0x0318, 0x2 },
        },
      }
    end,
  },
  {
    chips = {
      0xd280,
      0xd281,
      0xd282,
      0xd283,
      0xd284,
      0xd285,
      0xd286,
      0xd287,
      0xd2a0,
      0xd2a1,
      0xd2a2,
      0xd2a3,
      0xd2a4,
      0xd2a5,
      0xd2a6,
      0xd2a7,
    },
    ldn = 0x0b,
    exit = EXIT_KEY_AA,
    bank_mask = 0xff,
    monitor = function()
      return {
        ids = {
          { 0x80fe, 0x5c, 0x5c },
          { 0x00fe, 0xa3, 0xa3 },
        },
        reads = {
          { 0x00fe, 0x1 },
          { 0x80fe, 0x1 },
          { 0x0000, 0x1c },
          { 0x0020, 0xa },
          { 0x0030, 0xa },
          { 0x004a, 0x3 },
          { 0x0051, 0x4 },
          { 0x0059, 0x4 },
          { 0x006b, 0x1 },
          { 0x0090, 0x14 },
          { 0x00b0, 0xd },
          { 0x00c0, 0x18 },
          { 0x00d8, 0x2 },
          { 0x00e0, 0xa },
          { 0x00f3, 0x1 },
          { 0x00f5, 0x2 },
          { 0x0110, 0xe },
          { 0x0120, 0xe },
          { 0x0130, 0xe },
          { 0x0160, 0xe },
          { 0x0170, 0xe },
          { 0x0180, 0xe },
          { 0x0190, 0xe },
          { 0x01a0, 0xe },
          { 0x01d0, 0x8 },
          { 0x01e0, 0x8 },
          { 0x0204, 0x2 },
          { 0x0300, 0xa },
          { 0x0311, 0x3 },
          { 0x0318, 0x2 },
        },
      }
    end,
  },
  {
    chips = {
      0xb470,
      0xb471,
      0xb472,
      0xb473,
      0xb474,
      0xb475,
      0xb476,
      0xb477,
    },
    ldn = 0x0b,
    exit = EXIT_KEY_AA,
    bank_mask = 0xff,
    monitor = function()
      return {
        ids = {
          { 0x804f, 0x5c, 0x5c },
          { 0x004f, 0xa3, 0xa3 },
        },
        reads = {
          { 0x004f, 0x1 },
          { 0x804f, 0x1 },
          { 0x0001, 0x1 },
          { 0x0003, 0x1 },
          { 0x0004, 0x1 },
          { 0x0011, 0x1 },
          { 0x0012, 0x1 },
          { 0x0020, 0x8 },
          { 0x005d, 0x2 },
          { 0x0073, 0x6 },
          { 0x0150, 0x7 },
          { 0x0250, 0x7 },
          { 0x0454, 0x3 },
          { 0x0459, 0x3 },
          { 0x0506, 0x2 },
          { 0x0550, 0x3 },
          { 0x0621, 0xe },
          { 0x0630, 0xa },
          { 0x0641, 0x4 },
          { 0x0656, 0xa },
          { 0x0661, 0x4 },
          { 0x0669, 0x2 },
          { 0x0100, 0xa },
          { 0x0121, 0xc },
          { 0x0134, 0x2 },
          { 0x0200, 0xa },
          { 0x0221, 0xc },
          { 0x0234, 0x2 },
          { 0x0300, 0xa },
          { 0x0321, 0xc },
          { 0x0334, 0x2 },
        },
      }
    end,
  },
  {
    chips = {
      0xc330,
      0xc331,
      0xc332,
      0xc333,
      0xc334,
      0xc335,
      0xc336,
      0xc337,
    },
    ldn = 0x0b,
    exit = EXIT_KEY_AA,
    bank_mask = 0xff,
    monitor = function()
      return {
        ids = {
          { 0x804f, 0x5c, 0x5c },
          { 0x004f, 0xa3, 0xa3 },
        },
        reads = {
          { 0x004f, 0x1 },
          { 0x804f, 0x1 },
          { 0x0001, 0x1 },
          { 0x0003, 0x1 },
          { 0x0004, 0x1 },
          { 0x0011, 0x1 },
          { 0x0020, 0x8 },
          { 0x005d, 0x2 },
          { 0x0073, 0x6 },
          { 0x0150, 0x7 },
          { 0x0250, 0x7 },
          { 0x0401, 0x4 },
          { 0x0409, 0x10 },
          { 0x0454, 0x3 },
          { 0x0459, 0x3 },
          { 0x0550, 0x3 },
          { 0x0621, 0xe },
          { 0x0630, 0xa },
          { 0x0644, 0x6 },
          { 0x0656, 0xa },
          { 0x0100, 0xa },
          { 0x0121, 0xc },
          { 0x0134, 0x2 },
          { 0x0200, 0xa },
          { 0x0221, 0xc },
          { 0x0234, 0x2 },
          { 0x0300, 0xa },
          { 0x0321, 0xc },
          { 0x0334, 0x2 },
        },
      }
    end,
  },
  {
    chips = {
      0xc560,
      0xc561,
      0xc562,
      0xc563,
      0xc564,
      0xc565,
      0xc566,
      0xc567,
    },
    ldn = 0x0b,
    exit = EXIT_KEY_AA,
    bank_mask = 0xff,
    monitor = function()
      return {
        ids = {
          { 0x804f, 0x5c, 0x5c },
          { 0x004f, 0xa3, 0xa3 },
        },
        reads = {
          { 0x004f, 0x1 },
          { 0x804f, 0x1 },
          { 0x0001, 0x1 },
          { 0x0003, 0x1 },
          { 0x0011, 0x1 },
          { 0x0013, 0x1 },
          { 0x0015, 0x1 },
          { 0x0027, 0x1 },
          { 0x005d, 0x2 },
          { 0x0073, 0xc },
          { 0x0150, 0x2 },
          { 0x0409, 0x10 },
          { 0x0459, 0x3 },
          { 0x0470, 0x2 },
          { 0x0480, 0xf },
          { 0x04b0, 0xc },
          { 0x04c0, 0x10 },
          { 0x0568, 0x1 },
          { 0x0621, 0x6 },
          { 0x0100, 0xa },
          { 0x0121, 0xc },
          { 0x0134, 0x2 },
          { 0x0200, 0xa },
          { 0x0221, 0xc },
          { 0x0234, 0x2 },
          { 0x0300, 0xa },
          { 0x0321, 0xc },
          { 0x0334, 0x2 },
          { 0x0800, 0xa },
          { 0x0821, 0xc },
          { 0x0834, 0x2 },
          { 0x0900, 0xa },
          { 0x0921, 0xc },
          { 0x0934, 0x2 },
          { 0x0a00, 0xa },
          { 0x0a21, 0xc },
          { 0x0a34, 0x2 },
          { 0x0b00, 0xa },
          { 0x0b21, 0xc },
          { 0x0b34, 0x2 },
        },
      }
    end,
  },
  {
    chips = {
      0xc800,
      0xc801,
      0xc802,
      0xc803,
      0xc804,
      0xc805,
      0xc806,
      0xc807,
      0xc910,
      0xc911,
      0xc912,
      0xc913,
      0xc914,
      0xc915,
      0xc916,
      0xc917,
      0xd120,
      0xd121,
      0xd122,
      0xd123,
      0xd124,
      0xd125,
      0xd126,
      0xd127,
      0xd350,
      0xd351,
      0xd352,
      0xd353,
      0xd354,
      0xd355,
      0xd356,
      0xd357,
      0xd420,
      0xd421,
      0xd422,
      0xd423,
      0xd424,
      0xd425,
      0xd426,
      0xd427,
      0xd450,
      0xd451,
      0xd452,
      0xd453,
      0xd454,
      0xd455,
      0xd456,
      0xd457,
    },
    ldn = 0x0b,
    exit = EXIT_KEY_AA,
    bank_mask = 0xff,
    monitor = function()
      return {
        ids = {
          { 0x804f, 0x5c, 0x5c },
          { 0x004f, 0xa3, 0xa3 },
        },
        reads = {
          { 0x004f, 0x1 },
          { 0x804f, 0x1 },
          { 0x0001, 0x1 },
          { 0x0003, 0x1 },
          { 0x0011, 0x1 },
          { 0x0013, 0x1 },
          { 0x0015, 0x1 },
          { 0x0027, 0x1 },
          { 0x005d, 0x2 },
          { 0x0073, 0xc },
          { 0x0150, 0x2 },
          { 0x0409, 0x10 },
          { 0x0459, 0x3 },
          { 0x0470, 0x2 },
          { 0x0480, 0xf },
          { 0x04b0, 0xc },
          { 0x04c0, 0x10 },
          { 0x0568, 0x1 },
          { 0x0621, 0x6 },
          { 0x045d, 0x1 },
          { 0x0449, 0x1 },
          { 0x04a0, 0x1 },
          { 0x0720, 0x2 },
          { 0x0100, 0xa },
          { 0x0121, 0xc },
          { 0x0134, 0x2 },
          { 0x0200, 0xa },
          { 0x0221, 0xc },
          { 0x0234, 0x2 },
          { 0x0300, 0xa },
          { 0x0321, 0xc },
          { 0x0334, 0x2 },
          { 0x0800, 0xa },
          { 0x0821, 0xc },
          { 0x0834, 0x2 },
          { 0x0900, 0xa },
          { 0x0921, 0xc },
          { 0x0934, 0x2 },
          { 0x0a00, 0xa },
          { 0x0a21, 0xc },
          { 0x0a34, 0x2 },
          { 0x0b00, 0xa },
          { 0x0b21, 0xc },
          { 0x0b34, 0x2 },
        },
      }
    end,
  },
  {
    chips = {
      0xd428,
      0xd429,
      0xd42a,
      0xd42b,
      0xd42c,
      0xd42d,
      0xd42e,
      0xd42f,
    },
    ldn = 0x0b,
    exit = EXIT_KEY_AA,
    bank_mask = 0xff,
    monitor = function()
      return {
        ids = {
          { 0x804f, 0x5c, 0x5c },
          { 0x004f, 0xa3, 0xa3 },
        },
        reads = {
          { 0x004f, 0x1 },
          { 0x804f, 0x1 },
          { 0x0001, 0x1 },
          { 0x0003, 0x1 },
          { 0x0011, 0x1 },
          { 0x0013, 0x1 },
          { 0x0015, 0x1 },
          { 0x0027, 0x1 },
          { 0x005d, 0x2 },
          { 0x0073, 0xc },
          { 0x0150, 0x2 },
          { 0x0400, 0x9 },
          { 0x0409, 0x10 },
          { 0x0419, 0x2 },
          { 0x0459, 0x3 },
          { 0x0470, 0x2 },
          { 0x0480, 0xf },
          { 0x04b0, 0xc },
          { 0x04c0, 0x10 },
          { 0x0568, 0x1 },
          { 0x0621, 0x6 },
          { 0x045d, 0x1 },
          { 0x0490, 0x7 },
          { 0x04a0, 0x3 },
          { 0x04f4, 0x2 },
          { 0x0670, 0xb },
          { 0x0720, 0x2 },
          { 0x0c26, 0x6 },
          { 0x0100, 0xa },
          { 0x0121, 0xc },
          { 0x0134, 0x2 },
          { 0x0200, 0xa },
          { 0x0221, 0xc },
          { 0x0234, 0x2 },
          { 0x0300, 0xa },
          { 0x0321, 0xc },
          { 0x0334, 0x2 },
          { 0x0800, 0xa },
          { 0x0821, 0xc },
          { 0x0834, 0x2 },
          { 0x0900, 0xa },
          { 0x0921, 0xc },
          { 0x0934, 0x2 },
          { 0x0a00, 0xa },
          { 0x0a21, 0xc },
          { 0x0a34, 0x2 },
          { 0x0b00, 0xa },
          { 0x0b21, 0xc },
          { 0x0b34, 0x2 },
        },
      }
    end,
  },
  {
    chips = {
      0xd800,
      0xd801,
      0xd802,
      0xd803,
      0xd804,
      0xd805,
      0xd806,
      0xd807,
    },
    ldn = 0x0b,
    exit = EXIT_KEY_AA,
    bank_mask = 0xff,
    monitor = function()
      return {
        ids = {
          { 0x804f, 0x5c, 0x5c },
          { 0x004f, 0xa3, 0xa3 },
        },
        reads = {
          { 0x004f, 0x1 },
          { 0x804f, 0x1 },
          { 0x0001, 0x1 },
          { 0x0003, 0x1 },
          { 0x0011, 0x1 },
          { 0x0013, 0x1 },
          { 0x0015, 0x1 },
          { 0x0027, 0x1 },
          { 0x005d, 0x2 },
          { 0x0073, 0xc },
          { 0x0150, 0x2 },
          { 0x0400, 0x9 },
          { 0x0409, 0x10 },
          { 0x0419, 0x2 },
          { 0x0449, 0x1 },
          { 0x049e, 0x1 },
          { 0x04f4, 0x2 },
          { 0x0720, 0x2 },
          { 0x0c16, 0x1 },
          { 0x0459, 0x3 },
          { 0x0470, 0x2 },
          { 0x0480, 0xf },
          { 0x04b0, 0xc },
          { 0x04c0, 0x10 },
          { 0x0568, 0x1 },
          { 0x0621, 0x6 },
          { 0x045d, 0x1 },
          { 0x048f, 0x1 },
          { 0x0490, 0x7 },
          { 0x04a0, 0x3 },
          { 0x0670, 0xb },
          { 0x0c01, 0x1 },
          { 0x0c26, 0x6 },
          { 0x0100, 0xa },
          { 0x0121, 0xc },
          { 0x0134, 0x2 },
          { 0x0200, 0xa },
          { 0x0221, 0xc },
          { 0x0234, 0x2 },
          { 0x0300, 0xa },
          { 0x0321, 0xc },
          { 0x0334, 0x2 },
          { 0x0800, 0xa },
          { 0x0821, 0xc },
          { 0x0834, 0x2 },
          { 0x0900, 0xa },
          { 0x0921, 0xc },
          { 0x0934, 0x2 },
          { 0x0a00, 0xa },
          { 0x0a21, 0xc },
          { 0x0a34, 0x2 },
          { 0x0b00, 0xa },
          { 0x0b21, 0xc },
          { 0x0b34, 0x2 },
        },
      }
    end,
  },
  {
    chips = {
      0x8850,
      0x8851,
      0x8852,
      0x8853,
      0x8854,
      0x8855,
      0x8856,
      0x8857,
      0x8858,
      0x8859,
      0x885a,
      0x885b,
      0x885c,
      0x885d,
      0x885e,
      0x885f,
      0x8860,
      0x8861,
      0x8862,
      0x8863,
      0x8864,
      0x8865,
      0x8866,
      0x8867,
      0x8868,
      0x8869,
      0x886a,
      0x886b,
      0x886c,
      0x886d,
      0x886e,
      0x886f,
      0xa020,
      0xa021,
      0xa022,
      0xa023,
      0xa024,
      0xa025,
      0xa026,
      0xa027,
      0xa028,
      0xa029,
      0xa02a,
      0xa02b,
      0xa02c,
      0xa02d,
      0xa02e,
      0xa02f,
      0xb070,
      0xb071,
      0xb072,
      0xb073,
      0xb074,
      0xb075,
      0xb076,
      0xb077,
      0xb078,
      0xb079,
      0xb07a,
      0xb07b,
      0xb07c,
      0xb07d,
      0xb07e,
      0xb07f,
      0xa230,
      0xa231,
      0xa232,
      0xa233,
      0xa234,
      0xa235,
      0xa236,
      0xa237,
      0xa238,
      0xa239,
      0xa23a,
      0xa23b,
      0xa23c,
      0xa23d,
      0xa23e,
      0xa23f,
      0xa510,
      0xa511,
      0xa512,
      0xa513,
      0xa514,
      0xa515,
      0xa516,
      0xa517,
      0xa518,
      0xa519,
      0xa51a,
      0xa51b,
      0xa51c,
      0xa51d,
      0xa51e,
      0xa51f,
      0xb350,
      0xb351,
      0xb352,
      0xb353,
      0xb354,
      0xb355,
      0xb356,
      0xb357,
      0xb358,
      0xb359,
      0xb35a,
      0xb35b,
      0xb35c,
      0xb35d,
      0xb35e,
      0xb35f,
    },
    ldn = 0x0b,
    exit = EXIT_KEY_AA,
    bank_mask = 0x87,
    monitor = function()
      return {
        ids = {
          { 0x804f, 0x5c, 0x5c },
          { 0x004f, 0xa3, 0xa3 },
        },
        reads = {
          { 0x004f, 0x1 },
          { 0x804f, 0x1 },
          { 0x0000, 0x18 },
          { 0x0020, 0x20 },
          { 0x0047, 0x6 },
          { 0x0059, 0x1 },
          { 0x005d, 0x1 },
          { 0x0060, 0xd },
          { 0x007c, 0x3 },
          { 0x0150, 0x7 },
          { 0x0250, 0x7 },
          { 0x0454, 0x3 },
          { 0x0459, 0x3 },
          { 0x0550, 0x4 },
        },
      }
    end,
  },
  {
    chips = {
      0x5210,
      0x5211,
      0x5212,
      0x5213,
      0x5214,
      0x5215,
      0x5216,
      0x5217,
      0x5218,
      0x5219,
      0x521a,
      0x521b,
      0x521c,
      0x521d,
      0x521e,
      0x521f,
      0x5230,
      0x5231,
      0x5232,
      0x5233,
      0x5234,
      0x5235,
      0x5236,
      0x5237,
      0x5238,
      0x5239,
      0x523a,
      0x523b,
      0x523c,
      0x523d,
      0x523e,
      0x523f,
      0x5240,
      0x5241,
      0x5242,
      0x5243,
      0x5244,
      0x5245,
      0x5246,
      0x5247,
      0x5248,
      0x5249,
      0x524a,
      0x524b,
      0x524c,
      0x524d,
      0x524e,
      0x524f,
    },
    ldn = 0x0b,
    exit = EXIT_KEY_AA,
    bank_mask = 0x87,
    monitor = function()
      return {
        ids = {
          { 0x804f, 0x5c, 0x5c },
          { 0x004f, 0xa3, 0xa3 },
        },
        reads = {
          { 0x004f, 0x1 },
          { 0x804f, 0x1 },
          { 0x0020, 0x1e },
          { 0x0040, 0x1 },
          { 0x0047, 0x5 },
          { 0x0056, 0x8 },
          { 0x0150, 0x7 },
          { 0x0250, 0x7 },
          { 0x0453, 0x1 },
          { 0x0459, 0x3 },
          { 0x0550, 0x2 },
          { 0x0554, 0x4 },
        },
      }
    end,
  },
  {
    chips = {
      0x8280,
      0x8281,
      0x8282,
      0x8283,
      0x8284,
      0x8285,
      0x8286,
      0x8287,
      0x8288,
      0x8289,
      0x828a,
      0x828b,
      0x828c,
      0x828d,
      0x828e,
      0x828f,
      0x8540,
      0x8541,
      0x8542,
      0x8543,
      0x8544,
      0x8545,
      0x8546,
      0x8547,
      0x8548,
      0x8549,
      0x854a,
      0x854b,
      0x854c,
      0x854d,
      0x854e,
      0x854f,
    },
    ldn = 0x0b,
    exit = EXIT_KEY_AA,
    bank_mask = 0x87,
    monitor = function()
      return {
        ids = {
          { 0x804f, 0x5c, 0x5c },
          { 0x004f, 0xa3, 0xa3 },
        },
        reads = {
          { 0x004f, 0x1 },
          { 0x804f, 0x1 },
          { 0x0000, 0x5 },
          { 0x0010, 0x3 },
          { 0x0018, 0x1 },
          { 0x0020, 0x1e },
          { 0x0040, 0x1 },
          { 0x0047, 0x5 },
          { 0x0056, 0x4 },
          { 0x005d, 0x1 },
          { 0x0150, 0x7 },
          { 0x0250, 0x7 },
          { 0x0453, 0x1 },
          { 0x0459, 0x3 },
          { 0x0550, 0x2 },
          { 0x0554, 0x4 },
        },
      }
    end,
  },
}
local NCT668X_ROWS = {
  {
    chips = {
      0xc730,
      0xc731,
      0xc732,
      0xc733,
      0xc734,
      0xc735,
      0xc736,
      0xc737,
      0xc738,
      0xc739,
      0xc73a,
      0xc73b,
      0xc73c,
      0xc73d,
      0xc73e,
      0xc73f,
      0xd440,
      0xd441,
      0xd442,
      0xd443,
      0xd444,
      0xd445,
      0xd446,
      0xd447,
      0xd448,
      0xd449,
      0xd44a,
      0xd44b,
      0xd44c,
      0xd44d,
      0xd44e,
      0xd44f,
      0xd590,
      0xd591,
      0xd592,
      0xd593,
      0xd594,
      0xd595,
      0xd596,
      0xd597,
      0xd598,
      0xd599,
      0xd59a,
      0xd59b,
      0xd59c,
      0xd59d,
      0xd59e,
      0xd59f,
    },
    ldn = 0x0b,
    exit = EXIT_KEY_AA,
    active = true,
    bank_mask = 0x00,
    customers = {
      0x0805,
      0x0a0e,
      0x0200,
      0x0201,
      0x0207,
      0x020d,
      0x162b,
      0x0e2c,
      0x0e1b,
      0x1631,
      0x163e,
      0x1621,
      0x1633,
      0x163d,
    },
    customer = 0x0602,
    monitor = function()
      return {
        ids = {},
        reads = {
          { 0x0100, 0x40 },
          { 0x0140, 0x20 },
          { 0x0160, 0x10 },
          { 0x0180, 0x1 },
          { 0x01a0, 0x20 },
          { 0x01c0, 0x10 },
          { 0x01d0, 0x10 },
          { 0x0604, 0x6 },
          { 0x080f, 0x1 },
          { 0x0852, 0x2 },
          { 0x0a00, 0x1 },
          { 0x0c93, 0x1 },
          { 0x0cf8, 0x1 },
          { 0x0e00, 0x6 },
        },
      }
    end,
  },
}

local FINTEK_ROWS = {
  {
    chips = {
      0x0901,
      0x1001,
      0x0601,
      0x1106,
      0x0814,
      0x1007,
      0x0541,
      0x0723,
      0x0909,
      0x1005,
      0x0581,
      0x1210,
      0x0704,
      0x1010,
      0x1806,
      0x1502,
    },
    ldn = 0x04,
    exit = EXIT_KEY_AA,
    active = true,
    bank_mask = 0x00,
    config_id = { 0x23, 0x1934 },
    monitor = function()
      return {
        ids = {},
        reads = {
          { 0x0001, 0x1 },
          { 0x0020, 0xb },
          { 0x0069, 0x1 },
          { 0x006b, 0x5 },
          { 0x0070, 0x7 },
          { 0x0094, 0x1 },
          { 0x0096, 0x1 },
          { 0x0098, 0x2 },
          { 0x00a0, 0x40 },
        },
      }
    end,
  },
  {
    chips = {
      0x0507,
      0x0903,
    },
    ldn = 0x02,
    exit = EXIT_KEY_AA,
    active = true,
    bank_mask = 0x00,
    config_id = { 0x23, 0x1934 },
    monitor = function()
      return {
        ids = {},
        reads = {
          { 0x0001, 0x1 },
          { 0x0020, 0xb },
          { 0x0069, 0x1 },
          { 0x006b, 0x5 },
          { 0x0070, 0x6 },
          { 0x0094, 0x1 },
          { 0x0096, 0x1 },
          { 0x0098, 0x2 },
          { 0x00a0, 0x40 },
        },
      }
    end,
  },
  {
    chips = {
      0x0406,
      0x0341,
    },
    ldn = 0x04,
    exit = EXIT_KEY_AA,
    active = true,
    bank_mask = 0x00,
    config_id = { 0x23, 0x1934 },
    monitor = function()
      return {
        ids = {},
        reads = {
          { 0x0000, 0x2 },
          { 0x0010, 0xe },
          { 0x0020, 0x10 },
          { 0x0040, 0x1a },
          { 0x0060, 0xc },
          { 0x0070, 0xc },
          { 0x0080, 0xc },
          { 0x00a0, 0xa },
          { 0x00b0, 0xa },
          { 0x00c0, 0xa },
        },
      }
    end,
  },
}
local SMSC_ROWS = {
  {
    chips = {
      0x7c,
      0x7d,
      0x7f,
    },
    ldn = 0x0a,
    exit = EXIT_KEY_AA,
    active = false,
    bank_mask = 0x00,
    monitor = function()
      return {
        ids = {
          { 0x003e, 0x5c, 0x5c },
          { 0x003d, 0x8c, 0x8c },
        },
        reads = {
          { 0x0020, 0xe },
          { 0x0030, 0x3 },
          { 0x003d, 0x2 },
          { 0x0040, 0x1 },
          { 0x0084, 0x5 },
          { 0x0099, 0x2 },
        },
      }
    end,
  },
  {
    chips = {
      0x86,
    },
    ldn = 0x0a,
    exit = EXIT_KEY_AA,
    active = false,
    bank_mask = 0x00,
    monitor = function()
      return {
        ids = {
          { 0x003e, 0x5c, 0x5c },
          { 0x003d, 0x8e, 0x8e },
        },
        reads = {
          { 0x001f, 0xf },
          { 0x0030, 0x3 },
          { 0x003d, 0x2 },
          { 0x0040, 0x1 },
          { 0x0084, 0x6 },
          { 0x0099, 0x2 },
        },
      }
    end,
  },
}

local function answers(id)
  return (id ~= 0) and (id ~= 0xffff)
end

local function ite_key(s)
  ksi.hw.port_write(s.port, 1, 0x87)
  ksi.hw.port_write(s.port, 1, 0x01)
  ksi.hw.port_write(s.port, 1, 0x55)
  ksi.hw.port_write(s.port, 1, s.port == 0x2e and 0x55 or 0xaa)
end

local function nuvoton_key(s)
  ksi.hw.port_write(s.port, 1, 0x87)
  ksi.hw.port_write(s.port, 1, 0x87)
end

local function smsc_key(s)
  ksi.hw.port_write(s.port, 1, 0x55)
end

local FAMILIES = {
  { family = FAMILY_ITE, rows = ITE_ROWS, key = ite_key },
  { family = FAMILY_NUVOTON, rows = NUVOTON_ROWS, key = nuvoton_key },
  { family = FAMILY_NCT668X, rows = NCT668X_ROWS, key = nuvoton_key, paged = true },
  { family = FAMILY_FINTEK, rows = FINTEK_ROWS, key = nuvoton_key, remember = false },
  {
    family = FAMILY_SMSC,
    rows = SMSC_ROWS,
    key = smsc_key,
    byte_id = true,
    block = 0x80,
    pair = 0x70,
  },
}

local function find_row(family, id)
  for _, row in ipairs(family.rows) do
    for _, chip in ipairs(row.chips) do
      if chip == id then
        return row
      end
    end
  end
  return nil
end

local function chassis_type()
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
    if kind == 3 then
      if length < 6 then
        return nil
      end
      return string.byte(smbios, at + 5) & 0x7f
    end
    if kind == 127 then
      return nil
    end
    local strings = string.find(smbios, '\0\0', at + length, true)
    if not strings then
      return nil
    end
    at = strings + 2
  end
  return nil
end

local function chassis_permitted()
  local kind = kept.chassis or chassis_type()
  if not kind then
    ksi.trace('no chassis type in the SMBIOS table')
    return false
  end
  kept.chassis = kind
  for _, range in ipairs(CHASSIS) do
    if kind >= range[1] and kind <= range[2] then
      return true
    end
  end
  ksi.trace('chassis type 0x%02x is not permitted', kind)
  return false
end

local function rc(s, reg)
  ksi.hw.port_write(s.port, 1, reg)
  return ksi.hw.port_read(s.port + 1, 1)
end

local function wc(s, reg, value)
  ksi.hw.port_write(s.port, 1, reg)
  ksi.hw.port_write(s.port + 1, 1, value)
end

local function rcw(s, reg)
  local hi = rc(s, reg)
  local lo = rc(s, reg + 1)
  return (hi << 8) | lo
end

-- The chip ID at CR20h, one byte for a family that has one, and whether a
-- chip answers.
local function chip_id(s, family)
  if family.byte_id then
    local id = rc(s, 0x20)
    return id, id ~= 0 and id ~= 0xff
  end
  local id = rcw(s, 0x20)
  return id, answers(id)
end

local function exit_config(s)
  if s.entered then
    if s.row.exit == EXIT_KEY_AA then
      ksi.hw.port_write(s.port, 1, 0xaa)
    end
    if s.row.exit == EXIT_CONFIG_02 then
      wc(s, 0x02, 0x02)
    end
  end
  s.entered = false
end

local function abandon(s)
  if s.entered then
    ksi.hw.port_write(s.port, 1, 0xaa)
    if s.family.family == FAMILY_ITE and answers(rcw(s, 0x20)) then
      wc(s, 0x02, 0x02)
    end
    if select(2, chip_id(s, s.family)) then
      ksi.trace('port 0x%02x still answers after the exit', s.port)
      s.busy = true
    end
  end
  s.entered = false
end

local function close(s)
  if s.ldn_select and not s.contended and s.orig_ldn ~= s.row.ldn then
    local current = rc(s, 0x07)
    if current == s.row.ldn then
      wc(s, 0x07, s.orig_ldn)
    elseif s.ldn_set or current ~= s.orig_ldn then
      ksi.trace('LDN 0x%02x selected, not 0x%02x: not restored', current, s.row.ldn)
      s.contended = true
    end
  end
  s.ldn_select = false
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
  ksi.hw.port_write(s.port, 1, 0x20)
  local hi = ksi.hw.port_read(s.port + 1, 1)
  ksi.hw.port_write(s.port, 1, 0x21)
  local lo = ksi.hw.port_read(s.port + 1, 1)
  local id = (hi << 8) | lo
  s.already_open = answers(id)
  if s.already_open then
    for _, family in ipairs(FAMILIES) do
      if find_row(family, family.byte_id and (id >> 8) or id) then
        s.open_family = family
        return true
      end
    end
    ksi.trace('port 0x%02x answers 0x%04x before any key', s.port, id)
    return false
  end
  return true
end

-- Releases the page this call claimed, only while the page register still
-- reads it.
local function release_page(s)
  if s.claimed then
    if ksi.hw.port_read(s.base + 4, 1) == s.claimed then
      ksi.hw.port_write(s.base + 4, 1, PAGE_FREE)
    end
    s.claimed = nil
  end
end

-- Reads a register of a paged monitor: waits for the page register to be
-- free within what is left of the call's waits, claims the page, selects the
-- index, reads the data and releases the page.
local function rp(s, reg)
  local page = reg >> 8
  local start = ksi.time()
  while ksi.hw.port_read(s.base + 4, 1) ~= PAGE_FREE do
    if ksi.time() - start >= s.wait then
      s.wait = 0
      ksi.trace('the page register stays busy for register 0x%04x', reg)
      return ksi.STATUS_IO_TIMEOUT, 0
    end
    ksi.sleep(1)
  end
  s.wait = s.wait - (ksi.time() - start)
  ksi.hw.port_write(s.base + 4, 1, page)
  if ksi.hw.port_read(s.base + 4, 1) ~= page then
    ksi.trace('page 0x%02x was not claimed', page)
    return ksi.STATUS_RETRY, 0
  end
  s.claimed = page
  ksi.hw.port_write(s.base + 5, 1, reg & 0xff)
  local selected = ksi.hw.port_read(s.base + 5, 1) == (reg & 0xff)
  local value = selected and ksi.hw.port_read(s.base + 6, 1) or 0
  if ksi.hw.port_read(s.base + 4, 1) ~= page then
    ksi.trace('page 0x%02x was taken under register 0x%04x', page, reg)
    s.claimed = nil
    return ksi.STATUS_RETRY, 0
  end
  ksi.hw.port_write(s.base + 4, 1, PAGE_FREE)
  s.claimed = nil
  if not selected then
    ksi.trace('the monitor index moved under register 0x%04x', reg)
    return ksi.STATUS_RETRY, 0
  end
  return ksi.STATUS_SUCCESS, value
end

local function customer(s)
  local status, hi = rp(s, s.row.customer)
  if status ~= ksi.STATUS_SUCCESS then
    return status
  end
  local lo
  status, lo = rp(s, s.row.customer + 1)
  if status ~= ksi.STATUS_SUCCESS then
    return status
  end
  local id = (hi << 8) | lo
  for _, listed in ipairs(s.row.customers) do
    if listed == id then
      s.customer = id
      return ksi.STATUS_SUCCESS
    end
  end
  ksi.trace('customer 0x%04x is not permitted', id)
  return ksi.STATUS_NOT_FOUND
end

local function configure(s, family)
  s.family = family
  s.row = nil
  s.entered = false
  s.identified = false
  s.ldn_select = false
  s.ldn_set = false
  s.contended = false
  s.busy = false
  s.idphase = true
  if not s.already_open then
    s.entered = true
    family.key(s)
  end
  local id, answered = chip_id(s, family)
  s.entered = (not s.already_open) and answered
  s.row = find_row(family, id)
  if not s.row then
    ksi.trace('chip 0x%04x at port 0x%02x is not known to family %d', id, s.port, family.family)
    return ksi.STATUS_NOT_FOUND
  end
  local config = s.row.config_id
  if config and rcw(s, config[1]) ~= config[2] then
    ksi.trace('chip 0x%04x has no configuration ID 0x%04x', id, config[2])
    return ksi.STATUS_NOT_FOUND
  end
  s.identified = true
  s.orig_ldn = rc(s, 0x07)
  s.ldn_select = true
  wc(s, 0x07, s.row.ldn)
  s.ldn_set = true
  if s.row.active ~= false and (rc(s, 0x30) & 1) == 0 then
    ksi.trace('LDN 0x%02x is not active', s.row.ldn)
    return ksi.STATUS_DEVICE_NOT_READY
  end
  local base = rcw(s, 0x60)
  ksi.sleep(1)
  if rc(s, 0x07) ~= s.row.ldn or rcw(s, 0x60) ~= base then
    ksi.trace('LDN 0x%02x base 0x%04x changed while read', s.row.ldn, base)
    s.contended = true
    return ksi.STATUS_RETRY
  end
  if base == 0 or base == 0xffff then
    ksi.trace('monitor base 0x%04x is not set', base)
    return ksi.STATUS_DEVICE_NOT_READY
  end
  if family.family == FAMILY_FINTEK and (base & 7) == 5 then
    base = base - 5
  end
  local block = family.block or 8
  if base % block ~= 0 then
    ksi.trace('monitor base 0x%04x is misaligned', base)
    return ksi.STATUS_DEVICE_CONFIGURATION_ERROR
  end
  if base < BASE_MIN or base + block - 1 > BASE_LAST then
    ksi.trace('monitor base 0x%04x is outside the permitted range', base)
    return ksi.STATUS_DEVICE_CONFIGURATION_ERROR
  end
  s.base = base
  s.chip = id
  close(s)
  s.idphase = false
  if s.contended then
    return ksi.STATUS_RETRY
  end
  if family.paged then
    return customer(s)
  end
  return ksi.STATUS_SUCCESS
end

local function finish(s, status)
  release_page(s)
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

-- The families in the order a slot is tried: the one that identified last
-- first, in a kept script.
local function family_order(s)
  return kept.orders and kept.orders[s.port] or FAMILIES
end

local function remember_family(s)
  if ksi.volatile or family_order(s)[1] == s.family then
    return
  end
  local order = { s.family }
  for _, family in ipairs(FAMILIES) do
    if family ~= s.family then
      order[#order + 1] = family
    end
  end
  kept.orders = kept.orders or {}
  kept.orders[s.port] = order
end

local function identify_chip(s)
  local status = ksi.STATUS_NOT_FOUND
  for _, family in ipairs(family_order(s)) do
    if not s.open_family or s.open_family == family then
      status = finish(s, configure(s, family))
      if s.identified and status == ksi.STATUS_SUCCESS then
        remember_family(s)
      end
      if s.identified or s.busy then
        return status
      end
    end
  end
  return status
end

local function permitted(monitor, reg)
  local allowed = monitor.allowed
  if not allowed then
    allowed = {}
    for _, range in ipairs(monitor.reads) do
      for listed = range[1], range[1] + range[2] - 1 do
        allowed[listed] = true
      end
    end
    monitor.allowed = allowed
  end
  return allowed[reg] == true
end

local function read_bank(s)
  if not s.bank_read then
    ksi.hw.port_write(s.base + 5, 1, BANK)
    s.orig_bank = ksi.hw.port_read(s.base + 6, 1)
    s.bank = s.orig_bank
    s.bank_read = true
  end
end

local function put_back_bank(s)
  local mask = s.row.bank_mask
  if not s.bank_read or s.contended or ((s.bank ~ s.orig_bank) & mask) == 0 then
    return
  end
  ksi.hw.port_write(s.base + 5, 1, BANK)
  local current = ksi.hw.port_read(s.base + 6, 1)
  if (current & mask) ~= (s.bank & mask) then
    ksi.trace('bank 0x%02x selected, not 0x%02x: 0x%02x not restored', current, s.bank, s.orig_bank)
    kept[s.port] = nil
    return
  end
  ksi.hw.port_write(s.base + 6, 1, (current & ~mask) | (s.orig_bank & mask))
end

local function rm(s, reg)
  local pair = s.family.pair
  if pair then
    ksi.hw.port_write(s.base + pair, 1, reg & 0xff)
    return ksi.hw.port_read(s.base + pair + 1, 1)
  end
  local mask = s.row.bank_mask
  if mask ~= 0 then
    read_bank(s)
    local want = reg >> 8
    if want ~= (s.bank & mask) then
      ksi.hw.port_write(s.base + 5, 1, BANK)
      local current = ksi.hw.port_read(s.base + 6, 1)
      if (current & mask) ~= (s.bank & mask) then
        ksi.trace('bank 0x%02x selected, not 0x%02x', current, s.bank)
        s.contended = true
        return nil
      end
      s.bank = (current & ~mask) | want
      ksi.hw.port_write(s.base + 6, 1, s.bank)
    end
  end
  ksi.hw.port_write(s.base + 5, 1, reg & 0xff)
  local value = ksi.hw.port_read(s.base + 6, 1)
  if ksi.hw.port_read(s.base + 5, 1) ~= (reg & 0xff) then
    ksi.trace('the monitor index moved under register 0x%04x', reg)
    return nil
  end
  if mask ~= 0 then
    ksi.hw.port_write(s.base + 5, 1, BANK)
    local now = ksi.hw.port_read(s.base + 6, 1)
    if (now & mask) ~= (s.bank & mask) then
      ksi.trace('bank 0x%02x selected under register 0x%04x', now, reg)
      s.contended = true
      return nil
    end
  end
  return value
end

local function vendor(s, monitor)
  local mask = s.row.bank_mask
  local first = 0
  if mask ~= 0 then
    read_bank(s)
    first = s.bank & mask
  end
  for pass = 1, 2 do
    for _, want in ipairs(monitor.ids) do
      if (((want[1] >> 8) & mask) == first) == (pass == 1) then
        local got = rm(s, want[1])
        if not got then
          return ksi.STATUS_RETRY
        end
        if got ~= want[2] and got ~= want[3] then
          ksi.trace(
            'register 0x%04x is 0x%02x, not 0x%02x or 0x%02x',
            want[1],
            got,
            want[2],
            want[3]
          )
          return ksi.STATUS_NOT_FOUND
        end
      end
    end
  end
  return ksi.STATUS_SUCCESS
end

local function session(input)
  local slot = string.unpack('<I4', input)
  local port = SLOTS[slot]
  if not port then
    ksi.trace('slot %d does not exist', slot)
    return nil, ksi.STATUS_INVALID_PARAMETER
  end
  if not chassis_permitted() then
    return nil, ksi.STATUS_NOT_SUPPORTED
  end
  return { port = port, wait = PAGE_WAIT }
end

-- Returns the family, chip and customer of the chip at a slot.
function identify(input)
  if not ksi.hw.port_read then
    ksi.trace('no port functions')
    return ksi.STATUS_NOT_SUPPORTED
  end
  if #input ~= 4 then
    ksi.trace('input of %d bytes', #input)
    return ksi.STATUS_INVALID_PARAMETER
  end
  local s, refused = session(input)
  if not s then
    return refused
  end
  kept[s.port] = nil
  local lock <close>, failed = ksi.lock('isa')
  if not lock then
    ksi.trace('the isa lock was not taken: 0x%08x', failed)
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
  local status = identify_chip(s)
  if status ~= ksi.STATUS_SUCCESS then
    return status
  end
  return ksi.STATUS_SUCCESS, string.pack('<I4I4I4', s.family.family, s.chip, s.customer or 0)
end

-- Reads hardware monitor registers of the chip at a slot.
function read(input)
  if not ksi.hw.port_read then
    ksi.trace('no port functions')
    return ksi.STATUS_NOT_SUPPORTED
  end
  if #input < 8 or (#input % 4) ~= 0 then
    ksi.trace('input of %d bytes', #input)
    return ksi.STATUS_INVALID_PARAMETER
  end
  local s, refused = session(input)
  if not s then
    return refused
  end
  local lock <close>, failed = ksi.lock('isa')
  if not lock then
    ksi.trace('the isa lock was not taken: 0x%08x', failed)
    return failed
  end
  local putback <close> = setmetatable({}, {
    __close = function()
      finish(s, ksi.STATUS_SUCCESS)
    end,
  })
  local k = kept[s.port]
  local status, monitor
  if k then
    s.row, s.base, monitor, s.family = k.row, k.base, k.monitor, k.family
  else
    if not probe(s) then
      return ksi.STATUS_NOT_FOUND
    end
    status = identify_chip(s)
    if status ~= ksi.STATUS_SUCCESS then
      return status
    end
    monitor = s.row.monitor()
  end
  local count = (#input - 4) // 4
  local any = false
  for i = 1, count do
    if permitted(monitor, string.unpack('<I4', input, 1 + 4 * i)) then
      any = true
    end
  end
  if not any then
    ksi.trace('no register asked for is permitted')
    return ksi.STATUS_INVALID_PARAMETER
  end
  local bank <close> = setmetatable({}, {
    __close = function()
      put_back_bank(s)
    end,
  })
  status = vendor(s, monitor)
  if status == ksi.STATUS_SUCCESS and k and s.family.paged then
    status = customer(s)
  end
  if status ~= ksi.STATUS_SUCCESS then
    kept[s.port] = nil
    return status
  end
  local output = {}
  local stopped = false
  for i = 1, count do
    local reg = string.unpack('<I4', input, 1 + 4 * i)
    local item = ksi.STATUS_ACCESS_DENIED
    local value = 0
    if permitted(monitor, reg) then
      item = ksi.STATUS_RETRY
      if s.family.paged then
        item, value = rp(s, reg)
      elseif not stopped then
        value = rm(s, reg)
        stopped = (value == nil)
        item = stopped and ksi.STATUS_RETRY or ksi.STATUS_SUCCESS
        value = value or 0
      end
    else
      ksi.trace('register 0x%x is not permitted', reg)
    end
    output[i] = string.pack('<I4I4', item, value)
  end
  if stopped then
    kept[s.port] = nil
  elseif not k and not ksi.volatile and s.family.remember ~= false then
    kept[s.port] = { row = s.row, base = s.base, monitor = monitor, family = s.family }
  end
  return ksi.STATUS_SUCCESS, table.concat(output)
end
