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
-- Reads the model-specific registers of the processors. The registers each
-- processor family may be asked for are listed below; a register outside the
-- list, or one whose gate does not hold, is refused.
--

local nt = require('nt')

local ROWS = {
  {
    vendor = 'GenuineIntel',
    family = 0x06,
    models = {
      { 0x1a, 0x1a },
      { 0x1e, 0x1e },
      { 0x1f, 0x1f },
      { 0x25, 0x25 },
      { 0x2c, 0x2c },
      { 0x2e, 0x2e },
      { 0x2f, 0x2f },
    },
    msrs = function()
      return {
        [0x19c] = { gate = { 0x6, 0, 0, 1, 0x1, 0x1 } },
        [0x1a2] = true,
        [0xce] = true,
        [0x198] = true,
        [0xe7] = { gate = { 0x6, 2, 0, 1, 0x1, 0x1 } },
        [0xe8] = { gate = { 0x6, 2, 0, 1, 0x1, 0x1 } },
        [0x1ad] = true,
        [0x3f8] = true,
        [0x3f9] = true,
        [0x3fc] = true,
        [0x3fd] = true,
        [0x34] = true,
        [0x1b0] = { gate = { 0x6, 2, 3, 1, 0x1, 0x1 } },
      }
    end,
  },
  {
    vendor = 'GenuineIntel',
    family = 0x06,
    models = { { 0x2a, 0x2a } },
    msrs = function()
      return {
        [0x19c] = { gate = { 0x6, 0, 0, 1, 0x1, 0x1 } },
        [0x1a2] = true,
        [0xce] = true,
        [0x198] = true,
        [0xe7] = { gate = { 0x6, 2, 0, 1, 0x1, 0x1 } },
        [0xe8] = { gate = { 0x6, 2, 0, 1, 0x1, 0x1 } },
        [0x1ad] = true,
        [0x1b1] = { gate = { 0x6, 0, 6, 1, 0x1, 0x1 } },
        [0x606] = true,
        [0x611] = true,
        [0x639] = true,
        [0x610] = true,
        [0x614] = true,
        [0x613] = true,
        [0x60d] = true,
        [0x3f8] = true,
        [0x3f9] = true,
        [0x3fa] = true,
        [0x3fc] = true,
        [0x3fd] = true,
        [0x3fe] = true,
        [0x34] = true,
        [0x1b0] = { gate = { 0x6, 2, 3, 1, 0x1, 0x1 } },
        [0x641] = true,
      }
    end,
  },
  {
    vendor = 'GenuineIntel',
    family = 0x06,
    models = { { 0x2d, 0x2d } },
    msrs = function()
      return {
        [0x19c] = { gate = { 0x6, 0, 0, 1, 0x1, 0x1 } },
        [0x1a2] = true,
        [0xce] = true,
        [0x198] = true,
        [0xe7] = { gate = { 0x6, 2, 0, 1, 0x1, 0x1 } },
        [0xe8] = { gate = { 0x6, 2, 0, 1, 0x1, 0x1 } },
        [0x1ad] = true,
        [0x1b1] = { gate = { 0x6, 0, 6, 1, 0x1, 0x1 } },
        [0x606] = true,
        [0x611] = true,
        [0x639] = true,
        [0x610] = true,
        [0x614] = true,
        [0x613] = true,
        [0x60d] = true,
        [0x3f8] = true,
        [0x3f9] = true,
        [0x3fa] = true,
        [0x3fc] = true,
        [0x3fd] = true,
        [0x3fe] = true,
        [0x34] = true,
        [0x1b0] = { gate = { 0x6, 2, 3, 1, 0x1, 0x1 } },
        [0x619] = true,
      }
    end,
  },
  {
    vendor = 'GenuineIntel',
    family = 0x06,
    models = { { 0x3a, 0x3a } },
    msrs = function()
      return {
        [0x19c] = { gate = { 0x6, 0, 0, 1, 0x1, 0x1 } },
        [0x1a2] = true,
        [0xce] = true,
        [0x198] = true,
        [0xe7] = { gate = { 0x6, 2, 0, 1, 0x1, 0x1 } },
        [0xe8] = { gate = { 0x6, 2, 0, 1, 0x1, 0x1 } },
        [0x1ad] = true,
        [0x1b1] = { gate = { 0x6, 0, 6, 1, 0x1, 0x1 } },
        [0x606] = true,
        [0x611] = true,
        [0x639] = true,
        [0x610] = true,
        [0x614] = true,
        [0x613] = true,
        [0x60d] = true,
        [0x3f8] = true,
        [0x3f9] = true,
        [0x3fa] = true,
        [0x3fc] = true,
        [0x3fd] = true,
        [0x3fe] = true,
        [0x34] = true,
        [0x1b0] = { gate = { 0x6, 2, 3, 1, 0x1, 0x1 } },
        [0x641] = true,
      }
    end,
  },
  {
    vendor = 'GenuineIntel',
    family = 0x06,
    models = { { 0x3e, 0x3e } },
    msrs = function()
      return {
        [0x19c] = { gate = { 0x6, 0, 0, 1, 0x1, 0x1 } },
        [0x1a2] = true,
        [0xce] = true,
        [0x198] = true,
        [0xe7] = { gate = { 0x6, 2, 0, 1, 0x1, 0x1 } },
        [0xe8] = { gate = { 0x6, 2, 0, 1, 0x1, 0x1 } },
        [0x1ad] = true,
        [0x1b1] = { gate = { 0x6, 0, 6, 1, 0x1, 0x1 } },
        [0x606] = true,
        [0x611] = true,
        [0x639] = true,
        [0x610] = true,
        [0x614] = true,
        [0x613] = true,
        [0x60d] = true,
        [0x3f8] = true,
        [0x3f9] = true,
        [0x3fa] = true,
        [0x3fc] = true,
        [0x3fd] = true,
        [0x3fe] = true,
        [0x34] = true,
        [0x1b0] = { gate = { 0x6, 2, 3, 1, 0x1, 0x1 } },
        [0x619] = true,
        [0x1ae] = true,
      }
    end,
  },
  {
    vendor = 'GenuineIntel',
    family = 0x06,
    models = { { 0x3c, 0x3c }, { 0x46, 0x46 } },
    msrs = function()
      return {
        [0x19c] = { gate = { 0x6, 0, 0, 1, 0x1, 0x1 } },
        [0x1a2] = true,
        [0xce] = true,
        [0x198] = true,
        [0xe7] = { gate = { 0x6, 2, 0, 1, 0x1, 0x1 } },
        [0xe8] = { gate = { 0x6, 2, 0, 1, 0x1, 0x1 } },
        [0x1ad] = true,
        [0x1b1] = { gate = { 0x6, 0, 6, 1, 0x1, 0x1 } },
        [0x606] = true,
        [0x611] = true,
        [0x639] = true,
        [0x610] = true,
        [0x614] = true,
        [0x613] = true,
        [0x60d] = true,
        [0x3f8] = true,
        [0x3f9] = true,
        [0x3fa] = true,
        [0x3fc] = true,
        [0x3fd] = true,
        [0x3fe] = true,
        [0x34] = true,
        [0x1b0] = { gate = { 0x6, 2, 3, 1, 0x1, 0x1 } },
        [0x641] = true,
        [0x619] = true,
        [0x690] = true,
        [0x64c] = true,
        [0x1fc] = { mask = 0x840003 },
        [0x6b0] = true,
        [0x6b1] = true,
        [0x648] = true,
        [0x649] = true,
        [0x64a] = true,
        [0x64b] = true,
      }
    end,
  },
  {
    vendor = 'GenuineIntel',
    family = 0x06,
    models = { { 0x45, 0x45 } },
    msrs = function()
      return {
        [0x19c] = { gate = { 0x6, 0, 0, 1, 0x1, 0x1 } },
        [0x1a2] = true,
        [0xce] = true,
        [0x198] = true,
        [0xe7] = { gate = { 0x6, 2, 0, 1, 0x1, 0x1 } },
        [0xe8] = { gate = { 0x6, 2, 0, 1, 0x1, 0x1 } },
        [0x1ad] = true,
        [0x1b1] = { gate = { 0x6, 0, 6, 1, 0x1, 0x1 } },
        [0x606] = true,
        [0x611] = true,
        [0x639] = true,
        [0x610] = true,
        [0x614] = true,
        [0x613] = true,
        [0x60d] = true,
        [0x3f8] = true,
        [0x3f9] = true,
        [0x3fa] = true,
        [0x3fc] = true,
        [0x3fd] = true,
        [0x3fe] = true,
        [0x34] = true,
        [0x1b0] = { gate = { 0x6, 2, 3, 1, 0x1, 0x1 } },
        [0x641] = true,
        [0x619] = true,
        [0x690] = true,
        [0x64c] = true,
        [0x1fc] = { mask = 0x840003 },
        [0x6b0] = true,
        [0x6b1] = true,
        [0x630] = true,
        [0x631] = true,
        [0x632] = true,
        [0x648] = true,
        [0x649] = true,
        [0x64a] = true,
        [0x64b] = true,
      }
    end,
  },
  {
    vendor = 'GenuineIntel',
    family = 0x06,
    models = { { 0x3f, 0x3f } },
    msrs = function()
      return {
        [0x19c] = { gate = { 0x6, 0, 0, 1, 0x1, 0x1 } },
        [0x1a2] = true,
        [0xce] = true,
        [0x198] = true,
        [0xe7] = { gate = { 0x6, 2, 0, 1, 0x1, 0x1 } },
        [0xe8] = { gate = { 0x6, 2, 0, 1, 0x1, 0x1 } },
        [0x1ad] = true,
        [0x1b1] = { gate = { 0x6, 0, 6, 1, 0x1, 0x1 } },
        [0x606] = true,
        [0x611] = true,
        [0x639] = true,
        [0x610] = true,
        [0x614] = true,
        [0x613] = true,
        [0x60d] = true,
        [0x3f8] = true,
        [0x3f9] = true,
        [0x3fa] = true,
        [0x3fc] = true,
        [0x3fd] = true,
        [0x3fe] = true,
        [0x34] = true,
        [0x1b0] = { gate = { 0x6, 2, 3, 1, 0x1, 0x1 } },
        [0x619] = true,
        [0x61b] = true,
        [0x1ae] = true,
        [0x1af] = true,
        [0x648] = true,
        [0x649] = true,
        [0x64a] = true,
        [0x64b] = true,
      }
    end,
  },
  {
    vendor = 'GenuineIntel',
    family = 0x06,
    models = { { 0x3d, 0x3d }, { 0x47, 0x47 } },
    msrs = function()
      return {
        [0x19c] = { gate = { 0x6, 0, 0, 1, 0x1, 0x1 } },
        [0x1a2] = true,
        [0xce] = true,
        [0x198] = true,
        [0xe7] = { gate = { 0x6, 2, 0, 1, 0x1, 0x1 } },
        [0xe8] = { gate = { 0x6, 2, 0, 1, 0x1, 0x1 } },
        [0x1ad] = true,
        [0x1b1] = { gate = { 0x6, 0, 6, 1, 0x1, 0x1 } },
        [0x606] = true,
        [0x611] = true,
        [0x639] = true,
        [0x610] = true,
        [0x614] = true,
        [0x613] = true,
        [0x60d] = true,
        [0x3f8] = true,
        [0x3f9] = true,
        [0x3fa] = true,
        [0x3fc] = true,
        [0x3fd] = true,
        [0x3fe] = true,
        [0x34] = true,
        [0x1b0] = { gate = { 0x6, 2, 3, 1, 0x1, 0x1 } },
        [0x641] = true,
        [0x619] = true,
        [0x690] = true,
        [0x64c] = true,
        [0x1fc] = { mask = 0x840003 },
        [0x6b0] = true,
        [0x6b1] = true,
        [0x620] = true,
        [0x621] = true,
        [0x630] = true,
        [0x631] = true,
        [0x632] = true,
        [0x648] = true,
        [0x649] = true,
        [0x64a] = true,
        [0x64b] = true,
      }
    end,
  },
  {
    vendor = 'GenuineIntel',
    family = 0x06,
    models = { { 0x4f, 0x4f } },
    msrs = function()
      return {
        [0x19c] = { gate = { 0x6, 0, 0, 1, 0x1, 0x1 } },
        [0x1a2] = true,
        [0xce] = true,
        [0x198] = true,
        [0xe7] = { gate = { 0x6, 2, 0, 1, 0x1, 0x1 } },
        [0xe8] = { gate = { 0x6, 2, 0, 1, 0x1, 0x1 } },
        [0x1ad] = true,
        [0x1b1] = { gate = { 0x6, 0, 6, 1, 0x1, 0x1 } },
        [0x606] = true,
        [0x611] = true,
        [0x639] = true,
        [0x610] = true,
        [0x614] = true,
        [0x613] = true,
        [0x60d] = true,
        [0x3f8] = true,
        [0x3f9] = true,
        [0x3fa] = true,
        [0x3fc] = true,
        [0x3fd] = true,
        [0x3fe] = true,
        [0x34] = true,
        [0x1b0] = { gate = { 0x6, 2, 3, 1, 0x1, 0x1 } },
        [0x619] = true,
        [0x61b] = true,
        [0x1ae] = true,
        [0x1af] = true,
        [0x620] = true,
        [0x621] = true,
        [0x648] = true,
        [0x649] = true,
        [0x64a] = true,
        [0x64b] = true,
      }
    end,
  },
  {
    vendor = 'GenuineIntel',
    family = 0x06,
    models = { { 0x56, 0x56 } },
    msrs = function()
      return {
        [0x19c] = { gate = { 0x6, 0, 0, 1, 0x1, 0x1 } },
        [0x1a2] = true,
        [0xce] = true,
        [0x198] = true,
        [0xe7] = { gate = { 0x6, 2, 0, 1, 0x1, 0x1 } },
        [0xe8] = { gate = { 0x6, 2, 0, 1, 0x1, 0x1 } },
        [0x1ad] = true,
        [0x1b1] = { gate = { 0x6, 0, 6, 1, 0x1, 0x1 } },
        [0x606] = true,
        [0x611] = true,
        [0x639] = true,
        [0x610] = true,
        [0x614] = true,
        [0x613] = true,
        [0x60d] = true,
        [0x3f8] = true,
        [0x3f9] = true,
        [0x3fa] = true,
        [0x3fc] = true,
        [0x3fd] = true,
        [0x3fe] = true,
        [0x34] = true,
        [0x1b0] = { gate = { 0x6, 2, 3, 1, 0x1, 0x1 } },
        [0x619] = true,
        [0x61b] = true,
        [0x620] = true,
        [0x621] = true,
        [0x648] = true,
        [0x649] = true,
        [0x64a] = true,
        [0x64b] = true,
      }
    end,
  },
  {
    vendor = 'GenuineIntel',
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
    msrs = function()
      return {
        [0x19c] = { gate = { 0x6, 0, 0, 1, 0x1, 0x1 } },
        [0x1a2] = true,
        [0xce] = true,
        [0x198] = true,
        [0xe7] = { gate = { 0x6, 2, 0, 1, 0x1, 0x1 } },
        [0xe8] = { gate = { 0x6, 2, 0, 1, 0x1, 0x1 } },
        [0x1ad] = true,
        [0x1b1] = { gate = { 0x6, 0, 6, 1, 0x1, 0x1 } },
        [0x606] = true,
        [0x611] = true,
        [0x639] = true,
        [0x610] = true,
        [0x614] = true,
        [0x613] = true,
        [0x60d] = true,
        [0x3f8] = true,
        [0x3f9] = true,
        [0x3fa] = true,
        [0x3fc] = true,
        [0x3fd] = true,
        [0x3fe] = true,
        [0x34] = true,
        [0x1b0] = { gate = { 0x6, 2, 3, 1, 0x1, 0x1 } },
        [0x641] = true,
        [0x619] = true,
        [0x64d] = true,
        [0x64f] = true,
        [0x65c] = true,
        [0x64c] = true,
        [0x658] = true,
        [0x659] = true,
        [0x65a] = true,
        [0x65b] = true,
        [0x1fc] = { mask = 0x840003 },
        [0x652] = true,
        [0x620] = true,
        [0x621] = true,
        [0x630] = true,
        [0x631] = true,
        [0x632] = true,
        [0x660] = true,
        [0x64e] = true,
        [0x648] = true,
        [0x649] = true,
        [0x64a] = true,
        [0x64b] = true,
        [0x770] = { gate = { 0x6, 0, 7, 1, 0x1, 0x1 } },
        [0x771] = { gate = { 0x6, 0, 7, 1, 0x1, 0x1 } },
        [0x774] = { gate = { 0x6, 0, 7, 1, 0x1, 0x1 } },
        [0x777] = { gate = { 0x6, 0, 7, 1, 0x1, 0x1 } },
        [0x6b0] = true,
        [0x6b1] = true,
      }
    end,
  },
  {
    vendor = 'GenuineIntel',
    family = 0x06,
    models = { { 0x55, 0x55 }, { 0x6a, 0x6a }, { 0x6c, 0x6c } },
    msrs = function()
      return {
        [0x19c] = { gate = { 0x6, 0, 0, 1, 0x1, 0x1 } },
        [0x1a2] = true,
        [0xce] = true,
        [0x198] = true,
        [0xe7] = { gate = { 0x6, 2, 0, 1, 0x1, 0x1 } },
        [0xe8] = { gate = { 0x6, 2, 0, 1, 0x1, 0x1 } },
        [0x1ad] = true,
        [0x1b1] = { gate = { 0x6, 0, 6, 1, 0x1, 0x1 } },
        [0x606] = true,
        [0x611] = true,
        [0x639] = true,
        [0x610] = true,
        [0x614] = true,
        [0x613] = true,
        [0x60d] = true,
        [0x3f8] = true,
        [0x3f9] = true,
        [0x3fa] = true,
        [0x3fc] = true,
        [0x3fd] = true,
        [0x3fe] = true,
        [0x34] = true,
        [0x1b0] = { gate = { 0x6, 2, 3, 1, 0x1, 0x1 } },
        [0x619] = true,
        [0x61b] = true,
        [0x64f] = true,
        [0x1ae] = true,
        [0x620] = true,
        [0x621] = true,
        [0x660] = true,
        [0x64e] = true,
        [0x648] = true,
        [0x649] = true,
        [0x64a] = true,
        [0x64b] = true,
        [0x770] = { gate = { 0x6, 0, 7, 1, 0x1, 0x1 } },
        [0x771] = { gate = { 0x6, 0, 7, 1, 0x1, 0x1 } },
        [0x774] = { gate = { 0x6, 0, 7, 1, 0x1, 0x1 } },
        [0x777] = { gate = { 0x6, 0, 7, 1, 0x1, 0x1 } },
      }
    end,
  },
  {
    vendor = 'GenuineIntel',
    family = 0x06,
    models = { { 0x8f, 0x8f }, { 0xcf, 0xcf } },
    msrs = function()
      return {
        [0x19c] = { gate = { 0x6, 0, 0, 1, 0x1, 0x1 } },
        [0x1a2] = true,
        [0xce] = true,
        [0x198] = true,
        [0xe7] = { gate = { 0x6, 2, 0, 1, 0x1, 0x1 } },
        [0xe8] = { gate = { 0x6, 2, 0, 1, 0x1, 0x1 } },
        [0x1ad] = true,
        [0x1b1] = { gate = { 0x6, 0, 6, 1, 0x1, 0x1 } },
        [0x606] = true,
        [0x611] = true,
        [0x639] = true,
        [0x610] = true,
        [0x614] = true,
        [0x613] = true,
        [0x60d] = true,
        [0x3f8] = true,
        [0x3f9] = true,
        [0x3fa] = true,
        [0x3fc] = true,
        [0x3fd] = true,
        [0x3fe] = true,
        [0x34] = true,
        [0x1b0] = { gate = { 0x6, 2, 3, 1, 0x1, 0x1 } },
        [0x619] = true,
        [0x61b] = true,
        [0x64f] = true,
        [0x1ae] = true,
        [0x620] = true,
        [0x621] = true,
        [0x660] = true,
        [0x64e] = true,
        [0x648] = true,
        [0x649] = true,
        [0x64a] = true,
        [0x64b] = true,
        [0x770] = { gate = { 0x6, 0, 7, 1, 0x1, 0x1 } },
        [0x771] = { gate = { 0x6, 0, 7, 1, 0x1, 0x1 } },
        [0x774] = { gate = { 0x6, 0, 7, 1, 0x1, 0x1 } },
        [0x777] = { gate = { 0x6, 0, 7, 1, 0x1, 0x1 } },
        [0x64d] = true,
      }
    end,
  },
  {
    vendor = 'GenuineIntel',
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
    msrs = function()
      return {
        [0x19c] = { gate = { 0x6, 0, 0, 1, 0x1, 0x1 } },
        [0x1a2] = true,
        [0xce] = true,
        [0x198] = true,
        [0xe7] = { gate = { 0x6, 2, 0, 1, 0x1, 0x1 } },
        [0xe8] = { gate = { 0x6, 2, 0, 1, 0x1, 0x1 } },
        [0x1ad] = true,
        [0x1b1] = { gate = { 0x6, 0, 6, 1, 0x1, 0x1 } },
        [0x606] = true,
        [0x611] = true,
        [0x639] = true,
        [0x610] = true,
        [0x614] = true,
        [0x613] = true,
        [0x60d] = true,
        [0x3f8] = true,
        [0x3f9] = true,
        [0x3fa] = true,
        [0x3fc] = true,
        [0x3fd] = true,
        [0x3fe] = true,
        [0x34] = true,
        [0x1b0] = { gate = { 0x6, 2, 3, 1, 0x1, 0x1 } },
        [0x641] = true,
        [0x619] = true,
        [0x64d] = true,
        [0x64f] = true,
        [0x65c] = true,
        [0x64c] = true,
        [0x658] = true,
        [0x659] = true,
        [0x65a] = true,
        [0x65b] = true,
        [0x1fc] = { mask = 0x840003 },
        [0x652] = true,
        [0x620] = true,
        [0x621] = true,
        [0x630] = true,
        [0x631] = true,
        [0x632] = true,
        [0x660] = true,
        [0x64e] = true,
        [0x648] = true,
        [0x649] = true,
        [0x64a] = true,
        [0x64b] = true,
        [0x770] = { gate = { 0x6, 0, 7, 1, 0x1, 0x1 } },
        [0x771] = { gate = { 0x6, 0, 7, 1, 0x1, 0x1 } },
        [0x774] = { gate = { 0x6, 0, 7, 1, 0x1, 0x1 } },
        [0x777] = { gate = { 0x6, 0, 7, 1, 0x1, 0x1 } },
      }
    end,
  },
  {
    vendor = 'GenuineIntel',
    family = 0x06,
    models = { { 0x8a, 0x8a } },
    msrs = function()
      return {
        [0x19c] = { gate = { 0x6, 0, 0, 1, 0x1, 0x1 } },
        [0x1a2] = true,
        [0xce] = true,
        [0x198] = true,
        [0xe7] = { gate = { 0x6, 2, 0, 1, 0x1, 0x1 } },
        [0xe8] = { gate = { 0x6, 2, 0, 1, 0x1, 0x1 } },
        [0x1ad] = true,
        [0x1b1] = { gate = { 0x6, 0, 6, 1, 0x1, 0x1 } },
        [0x606] = true,
        [0x611] = true,
        [0x639] = true,
        [0x610] = true,
        [0x614] = true,
        [0x641] = true,
        [0x619] = true,
        [0x64d] = true,
      }
    end,
  },
  {
    vendor = 'GenuineIntel',
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
    msrs = function()
      return {
        [0x19c] = { gate = { 0x6, 0, 0, 1, 0x1, 0x1 } },
        [0x1a2] = true,
        [0xce] = true,
        [0x198] = true,
        [0xe7] = { gate = { 0x6, 2, 0, 1, 0x1, 0x1 } },
        [0xe8] = { gate = { 0x6, 2, 0, 1, 0x1, 0x1 } },
        [0x1ad] = true,
        [0x1b1] = { gate = { 0x6, 0, 6, 1, 0x1, 0x1 } },
        [0x606] = true,
        [0x611] = true,
        [0x639] = true,
        [0x610] = true,
        [0x614] = true,
        [0x613] = true,
        [0x60d] = true,
        [0x3f8] = true,
        [0x3f9] = true,
        [0x3fa] = true,
        [0x3fc] = true,
        [0x3fd] = true,
        [0x3fe] = true,
        [0x34] = true,
        [0x1b0] = { gate = { 0x6, 2, 3, 1, 0x1, 0x1 } },
        [0x641] = true,
        [0x619] = true,
        [0x64d] = true,
        [0x64f] = true,
        [0x601] = true,
        [0x65c] = true,
        [0x64c] = true,
        [0x658] = true,
        [0x659] = true,
        [0x65a] = true,
        [0x65b] = true,
        [0x1fc] = { mask = 0x840003 },
        [0x652] = true,
        [0x620] = true,
        [0x621] = true,
        [0x630] = true,
        [0x631] = true,
        [0x632] = true,
        [0x660] = true,
        [0x64e] = true,
        [0x648] = true,
        [0x649] = true,
        [0x64a] = true,
        [0x64b] = true,
        [0x770] = { gate = { 0x6, 0, 7, 1, 0x1, 0x1 } },
        [0x771] = { gate = { 0x6, 0, 7, 1, 0x1, 0x1 } },
        [0x774] = { gate = { 0x6, 0, 7, 1, 0x1, 0x1 } },
        [0x777] = { gate = { 0x6, 0, 7, 1, 0x1, 0x1 } },
        [0x650] = true,
        [0x664] = { gate = { 0x1a, 0, 24, 8, 0x20, 0x20 } },
      }
    end,
  },
  {
    vendor = 'GenuineIntel',
    family = 0x06,
    models = { { 0xbe, 0xbe } },
    msrs = function()
      return {
        [0x19c] = { gate = { 0x6, 0, 0, 1, 0x1, 0x1 } },
        [0x1a2] = true,
        [0xce] = true,
        [0x198] = true,
        [0xe7] = { gate = { 0x6, 2, 0, 1, 0x1, 0x1 } },
        [0xe8] = { gate = { 0x6, 2, 0, 1, 0x1, 0x1 } },
        [0x1ad] = true,
        [0x1b1] = { gate = { 0x6, 0, 6, 1, 0x1, 0x1 } },
        [0x606] = true,
        [0x611] = true,
        [0x639] = true,
        [0x610] = true,
        [0x614] = true,
        [0x613] = true,
        [0x60d] = true,
        [0x3f8] = true,
        [0x3f9] = true,
        [0x3fa] = true,
        [0x3fc] = true,
        [0x3fd] = true,
        [0x3fe] = true,
        [0x34] = true,
        [0x1b0] = { gate = { 0x6, 2, 3, 1, 0x1, 0x1 } },
        [0x641] = true,
        [0x619] = true,
        [0x64d] = true,
        [0x64f] = true,
        [0x601] = true,
        [0x65c] = true,
        [0x64c] = true,
        [0x658] = true,
        [0x659] = true,
        [0x65a] = true,
        [0x65b] = true,
        [0x1fc] = { mask = 0x840003 },
        [0x652] = true,
        [0x620] = true,
        [0x621] = true,
        [0x630] = true,
        [0x631] = true,
        [0x632] = true,
        [0x660] = true,
        [0x64e] = true,
        [0x648] = true,
        [0x649] = true,
        [0x64a] = true,
        [0x64b] = true,
        [0x770] = { gate = { 0x6, 0, 7, 1, 0x1, 0x1 } },
        [0x771] = { gate = { 0x6, 0, 7, 1, 0x1, 0x1 } },
        [0x774] = { gate = { 0x6, 0, 7, 1, 0x1, 0x1 } },
        [0x777] = { gate = { 0x6, 0, 7, 1, 0x1, 0x1 } },
        [0x664] = true,
      }
    end,
  },
  {
    vendor = 'GenuineIntel',
    family = 0x06,
    models = { { 0xad, 0xad }, { 0xae, 0xae }, { 0xaf, 0xaf }, { 0xb6, 0xb6 }, { 0xdd, 0xdd } },
    msrs = function()
      return {
        [0x19c] = { gate = { 0x6, 0, 0, 1, 0x1, 0x1 } },
        [0x1a2] = true,
        [0xce] = true,
        [0x198] = true,
        [0xe7] = { gate = { 0x6, 2, 0, 1, 0x1, 0x1 } },
        [0xe8] = { gate = { 0x6, 2, 0, 1, 0x1, 0x1 } },
        [0x1ad] = true,
        [0x1b1] = { gate = { 0x6, 0, 6, 1, 0x1, 0x1 } },
      }
    end,
  },
  {
    vendor = 'GenuineIntel',
    family = 0x06,
    models = {
      { 0x37, 0x37 },
      { 0x4d, 0x4d },
      { 0x4a, 0x4a },
      { 0x5a, 0x5a },
      { 0x4c, 0x4c },
      { 0x75, 0x75 },
    },
    msrs = function()
      return {
        [0x19c] = { gate = { 0x6, 0, 0, 1, 0x1, 0x1 } },
        [0x1a2] = true,
        [0xcd] = true,
        [0xce] = true,
        [0x198] = true,
        [0xe7] = { gate = { 0x6, 2, 0, 1, 0x1, 0x1 } },
        [0xe8] = { gate = { 0x6, 2, 0, 1, 0x1, 0x1 } },
        [0x1ad] = true,
        [0x1b1] = { gate = { 0x6, 0, 6, 1, 0x1, 0x1 } },
        [0x606] = true,
        [0x611] = true,
        [0x610] = true,
        [0x614] = true,
        [0x3fa] = true,
        [0x660] = true,
        [0x1b0] = { gate = { 0x6, 2, 3, 1, 0x1, 0x1 } },
      }
    end,
  },
  {
    vendor = 'GenuineIntel',
    family = 0x06,
    models = { { 0x57, 0x57 }, { 0x85, 0x85 } },
    msrs = function()
      return {
        [0x19c] = { gate = { 0x6, 0, 0, 1, 0x1, 0x1 } },
        [0x1a2] = true,
        [0xce] = true,
        [0x198] = true,
        [0xe7] = { gate = { 0x6, 2, 0, 1, 0x1, 0x1 } },
        [0xe8] = { gate = { 0x6, 2, 0, 1, 0x1, 0x1 } },
        [0x1ad] = true,
        [0x1b1] = { gate = { 0x6, 0, 6, 1, 0x1, 0x1 } },
        [0x606] = true,
        [0x611] = true,
        [0x610] = true,
        [0x614] = true,
        [0x613] = true,
        [0x619] = true,
        [0x61b] = true,
        [0x60d] = true,
        [0x3f8] = true,
        [0x3f9] = true,
        [0x3ff] = true,
        [0x34] = true,
        [0x1b0] = { gate = { 0x6, 2, 3, 1, 0x1, 0x1 } },
        [0x648] = true,
        [0x649] = true,
        [0x64a] = true,
        [0x64b] = true,
      }
    end,
  },
  {
    vendor = 'GenuineIntel',
    family = 0x06,
    models = { { 0x36, 0x36 } },
    msrs = function()
      return {
        [0x19c] = { gate = { 0x6, 0, 0, 1, 0x1, 0x1 } },
        [0x1a2] = true,
        [0x198] = true,
        [0xe7] = { gate = { 0x6, 2, 0, 1, 0x1, 0x1 } },
        [0xe8] = { gate = { 0x6, 2, 0, 1, 0x1, 0x1 } },
        [0x1b0] = { gate = { 0x6, 2, 3, 1, 0x1, 0x1 } },
      }
    end,
  },
  {
    vendor = 'GenuineIntel',
    family = 0x06,
    models = {
      { 0x5c, 0x5c },
      { 0x5f, 0x5f },
      { 0x7a, 0x7a },
      { 0x86, 0x86 },
      { 0x96, 0x96 },
      { 0x9c, 0x9c },
    },
    msrs = function()
      return {
        [0x19c] = { gate = { 0x6, 0, 0, 1, 0x1, 0x1 } },
        [0x1a2] = true,
        [0xce] = true,
        [0x198] = true,
        [0xe7] = { gate = { 0x6, 2, 0, 1, 0x1, 0x1 } },
        [0xe8] = { gate = { 0x6, 2, 0, 1, 0x1, 0x1 } },
        [0x1ad] = true,
        [0x1b1] = { gate = { 0x6, 0, 6, 1, 0x1, 0x1 } },
        [0x606] = true,
        [0x611] = true,
        [0x639] = true,
        [0x610] = true,
        [0x614] = true,
        [0x619] = true,
        [0x3fa] = true,
        [0x660] = true,
        [0x664] = true,
        [0x1b0] = { gate = { 0x6, 2, 3, 1, 0x1, 0x1 } },
      }
    end,
  },
  {
    vendor = 'GenuineIntel',
    family = 0x12,
    models = { { 0x01, 0x01 }, { 0x03, 0x03 } },
    msrs = function()
      return {
        [0x19c] = { gate = { 0x6, 0, 0, 1, 0x1, 0x1 } },
        [0x1a2] = true,
        [0xce] = true,
        [0x198] = true,
        [0xe7] = { gate = { 0x6, 2, 0, 1, 0x1, 0x1 } },
        [0xe8] = { gate = { 0x6, 2, 0, 1, 0x1, 0x1 } },
        [0x1ad] = true,
        [0x1b1] = { gate = { 0x6, 0, 6, 1, 0x1, 0x1 } },
        [0x606] = true,
        [0x611] = true,
        [0x639] = true,
        [0x610] = true,
        [0x614] = true,
        [0x613] = true,
        [0x60d] = true,
        [0x3f8] = true,
        [0x3f9] = true,
        [0x3fa] = true,
        [0x3fc] = true,
        [0x3fd] = true,
        [0x3fe] = true,
        [0x34] = true,
        [0x1b0] = { gate = { 0x6, 2, 3, 1, 0x1, 0x1 } },
        [0x641] = true,
        [0x619] = true,
        [0x64d] = true,
        [0x64f] = true,
        [0x601] = true,
        [0x65c] = true,
        [0x64c] = true,
        [0x658] = true,
        [0x659] = true,
        [0x65a] = true,
        [0x65b] = true,
        [0x1fc] = { mask = 0x840003 },
        [0x652] = true,
        [0x620] = true,
        [0x621] = true,
        [0x630] = true,
        [0x631] = true,
        [0x632] = true,
        [0x660] = true,
        [0x64e] = true,
        [0x648] = true,
        [0x649] = true,
        [0x64a] = true,
        [0x64b] = true,
        [0x770] = { gate = { 0x6, 0, 7, 1, 0x1, 0x1 } },
        [0x771] = { gate = { 0x6, 0, 7, 1, 0x1, 0x1 } },
        [0x774] = { gate = { 0x6, 0, 7, 1, 0x1, 0x1 } },
        [0x777] = { gate = { 0x6, 0, 7, 1, 0x1, 0x1 } },
      }
    end,
  },
  {
    vendor = 'GenuineIntel',
    family = 0x13,
    models = { { 0x01, 0x01 } },
    msrs = function()
      return {
        [0x19c] = { gate = { 0x6, 0, 0, 1, 0x1, 0x1 } },
        [0x1a2] = true,
        [0xce] = true,
        [0x198] = true,
        [0xe7] = { gate = { 0x6, 2, 0, 1, 0x1, 0x1 } },
        [0xe8] = { gate = { 0x6, 2, 0, 1, 0x1, 0x1 } },
        [0x1ad] = true,
        [0x1b1] = { gate = { 0x6, 0, 6, 1, 0x1, 0x1 } },
      }
    end,
  },
  {
    vendor = 'AuthenticAMD',
    family = 0x17,
    models = { { 0x00, 0x0f } },
    msrs = function()
      return {
        [0x8b] = true,
        [0xc00000e7] = true,
        [0xc00000e8] = true,
        [0xe7] = { gate = { 0x6, 2, 0, 1, 0x1, 0x1 } },
        [0xe8] = { gate = { 0x6, 2, 0, 1, 0x1, 0x1 } },
        [0xc0010015] = true,
        [0xc0010061] = true,
        [0xc0010063] = true,
        [0xc0010064] = true,
        [0xc0010065] = true,
        [0xc0010066] = true,
        [0xc0010067] = true,
        [0xc0010068] = true,
        [0xc0010069] = true,
        [0xc001006a] = true,
        [0xc001006b] = true,
        [0xc0010293] = true,
        [0xc0010299] = { gate = { 0x80000007, 3, 14, 1, 0x1, 0x1 } },
        [0xc001029a] = { gate = { 0x80000007, 3, 14, 1, 0x1, 0x1 } },
        [0xc001029b] = { gate = { 0x80000007, 3, 14, 1, 0x1, 0x1 } },
      }
    end,
  },
  {
    vendor = 'AuthenticAMD',
    family = 0x17,
    models = { { 0x10, 0x1f }, { 0x20, 0x2f } },
    msrs = function()
      return {
        [0x8b] = true,
        [0xc00000e7] = true,
        [0xc00000e8] = true,
        [0xe7] = { gate = { 0x6, 2, 0, 1, 0x1, 0x1 } },
        [0xe8] = { gate = { 0x6, 2, 0, 1, 0x1, 0x1 } },
        [0xc0010015] = true,
        [0xc0010061] = true,
        [0xc0010063] = true,
        [0xc0010064] = true,
        [0xc0010065] = true,
        [0xc0010066] = true,
        [0xc0010067] = true,
        [0xc0010068] = true,
        [0xc0010069] = true,
        [0xc001006a] = true,
        [0xc001006b] = true,
        [0xc0010293] = true,
        [0xc0010299] = { gate = { 0x80000007, 3, 14, 1, 0x1, 0x1 } },
        [0xc001029a] = { gate = { 0x80000007, 3, 14, 1, 0x1, 0x1 } },
        [0xc001029b] = { gate = { 0x80000007, 3, 14, 1, 0x1, 0x1 } },
      }
    end,
  },
  {
    vendor = 'AuthenticAMD',
    family = 0x17,
    models = { { 0x50, 0x5f } },
    msrs = function()
      return {
        [0x8b] = true,
        [0xc00000e7] = true,
        [0xc00000e8] = true,
        [0xe7] = { gate = { 0x6, 2, 0, 1, 0x1, 0x1 } },
        [0xe8] = { gate = { 0x6, 2, 0, 1, 0x1, 0x1 } },
        [0xc0010015] = true,
        [0xc0010061] = true,
        [0xc0010063] = true,
        [0xc0010064] = true,
        [0xc0010065] = true,
        [0xc0010066] = true,
        [0xc0010067] = true,
        [0xc0010068] = true,
        [0xc0010069] = true,
        [0xc001006a] = true,
        [0xc001006b] = true,
        [0xc0010293] = true,
        [0xc0010299] = { gate = { 0x80000007, 3, 14, 1, 0x1, 0x1 } },
        [0xc001029a] = { gate = { 0x80000007, 3, 14, 1, 0x1, 0x1 } },
        [0xc001029b] = { gate = { 0x80000007, 3, 14, 1, 0x1, 0x1 } },
      }
    end,
  },
  {
    vendor = 'AuthenticAMD',
    family = 0x17,
    models = { { 0x30, 0x3f } },
    msrs = function()
      return {
        [0x8b] = true,
        [0xc00000e7] = true,
        [0xc00000e8] = true,
        [0xe7] = { gate = { 0x6, 2, 0, 1, 0x1, 0x1 } },
        [0xe8] = { gate = { 0x6, 2, 0, 1, 0x1, 0x1 } },
        [0xc0010015] = true,
        [0xc0010061] = true,
        [0xc0010063] = true,
        [0xc0010064] = true,
        [0xc0010065] = true,
        [0xc0010066] = true,
        [0xc0010067] = true,
        [0xc0010068] = true,
        [0xc0010069] = true,
        [0xc001006a] = true,
        [0xc001006b] = true,
        [0xc0010293] = true,
        [0xc0010299] = { gate = { 0x80000007, 3, 14, 1, 0x1, 0x1 } },
        [0xc001029a] = { gate = { 0x80000007, 3, 14, 1, 0x1, 0x1 } },
        [0xc001029b] = { gate = { 0x80000007, 3, 14, 1, 0x1, 0x1 } },
        [0xc00102b0] = { gate = { 0x80000008, 1, 27, 1, 0x1, 0x1 } },
        [0xc00102b1] = { gate = { 0x80000008, 1, 27, 1, 0x1, 0x1 } },
        [0xc00102b2] = { gate = { 0x80000008, 1, 27, 1, 0x1, 0x1 } },
        [0xc00102b3] = { gate = { 0x80000008, 1, 27, 1, 0x1, 0x1 } },
        [0xc00102b4] = { gate = { 0x80000008, 1, 27, 1, 0x1, 0x1 } },
        [0xc00102b5] = { gate = { 0x80000008, 1, 27, 1, 0x1, 0x1 } },
      }
    end,
  },
  {
    vendor = 'AuthenticAMD',
    family = 0x17,
    models = { { 0x40, 0x4f } },
    msrs = function()
      return {
        [0x8b] = true,
        [0xc00000e7] = true,
        [0xc00000e8] = true,
        [0xe7] = { gate = { 0x6, 2, 0, 1, 0x1, 0x1 } },
        [0xe8] = { gate = { 0x6, 2, 0, 1, 0x1, 0x1 } },
        [0xc0010015] = true,
        [0xc0010061] = true,
        [0xc0010063] = true,
        [0xc0010064] = true,
        [0xc0010065] = true,
        [0xc0010066] = true,
        [0xc0010067] = true,
        [0xc0010068] = true,
        [0xc0010069] = true,
        [0xc001006a] = true,
        [0xc001006b] = true,
        [0xc0010293] = true,
        [0xc0010299] = { gate = { 0x80000007, 3, 14, 1, 0x1, 0x1 } },
        [0xc001029a] = { gate = { 0x80000007, 3, 14, 1, 0x1, 0x1 } },
        [0xc001029b] = { gate = { 0x80000007, 3, 14, 1, 0x1, 0x1 } },
        [0xc00102b0] = { gate = { 0x80000008, 1, 27, 1, 0x1, 0x1 } },
        [0xc00102b1] = { gate = { 0x80000008, 1, 27, 1, 0x1, 0x1 } },
        [0xc00102b2] = { gate = { 0x80000008, 1, 27, 1, 0x1, 0x1 } },
        [0xc00102b3] = { gate = { 0x80000008, 1, 27, 1, 0x1, 0x1 } },
        [0xc00102b4] = { gate = { 0x80000008, 1, 27, 1, 0x1, 0x1 } },
        [0xc00102b5] = { gate = { 0x80000008, 1, 27, 1, 0x1, 0x1 } },
      }
    end,
  },
  {
    vendor = 'AuthenticAMD',
    family = 0x17,
    models = { { 0x60, 0x6f } },
    msrs = function()
      return {
        [0x8b] = true,
        [0xc00000e7] = true,
        [0xc00000e8] = true,
        [0xe7] = { gate = { 0x6, 2, 0, 1, 0x1, 0x1 } },
        [0xe8] = { gate = { 0x6, 2, 0, 1, 0x1, 0x1 } },
        [0xc0010015] = true,
        [0xc0010061] = true,
        [0xc0010063] = true,
        [0xc0010064] = true,
        [0xc0010065] = true,
        [0xc0010066] = true,
        [0xc0010067] = true,
        [0xc0010068] = true,
        [0xc0010069] = true,
        [0xc001006a] = true,
        [0xc001006b] = true,
        [0xc0010293] = true,
        [0xc0010299] = { gate = { 0x80000007, 3, 14, 1, 0x1, 0x1 } },
        [0xc001029a] = { gate = { 0x80000007, 3, 14, 1, 0x1, 0x1 } },
        [0xc001029b] = { gate = { 0x80000007, 3, 14, 1, 0x1, 0x1 } },
        [0xc00102b0] = { gate = { 0x80000008, 1, 27, 1, 0x1, 0x1 } },
        [0xc00102b1] = { gate = { 0x80000008, 1, 27, 1, 0x1, 0x1 } },
        [0xc00102b2] = { gate = { 0x80000008, 1, 27, 1, 0x1, 0x1 } },
        [0xc00102b3] = { gate = { 0x80000008, 1, 27, 1, 0x1, 0x1 } },
        [0xc00102b4] = { gate = { 0x80000008, 1, 27, 1, 0x1, 0x1 } },
        [0xc00102b5] = { gate = { 0x80000008, 1, 27, 1, 0x1, 0x1 } },
      }
    end,
  },
  {
    vendor = 'AuthenticAMD',
    family = 0x17,
    models = { { 0x70, 0x7f } },
    msrs = function()
      return {
        [0x8b] = true,
        [0xc00000e7] = true,
        [0xc00000e8] = true,
        [0xe7] = { gate = { 0x6, 2, 0, 1, 0x1, 0x1 } },
        [0xe8] = { gate = { 0x6, 2, 0, 1, 0x1, 0x1 } },
        [0xc0010015] = true,
        [0xc0010061] = true,
        [0xc0010063] = true,
        [0xc0010064] = true,
        [0xc0010065] = true,
        [0xc0010066] = true,
        [0xc0010067] = true,
        [0xc0010068] = true,
        [0xc0010069] = true,
        [0xc001006a] = true,
        [0xc001006b] = true,
        [0xc0010293] = true,
        [0xc0010299] = { gate = { 0x80000007, 3, 14, 1, 0x1, 0x1 } },
        [0xc001029a] = { gate = { 0x80000007, 3, 14, 1, 0x1, 0x1 } },
        [0xc001029b] = { gate = { 0x80000007, 3, 14, 1, 0x1, 0x1 } },
        [0xc00102b0] = { gate = { 0x80000008, 1, 27, 1, 0x1, 0x1 } },
        [0xc00102b1] = { gate = { 0x80000008, 1, 27, 1, 0x1, 0x1 } },
        [0xc00102b2] = { gate = { 0x80000008, 1, 27, 1, 0x1, 0x1 } },
        [0xc00102b3] = { gate = { 0x80000008, 1, 27, 1, 0x1, 0x1 } },
        [0xc00102b4] = { gate = { 0x80000008, 1, 27, 1, 0x1, 0x1 } },
        [0xc00102b5] = { gate = { 0x80000008, 1, 27, 1, 0x1, 0x1 } },
      }
    end,
  },
  {
    vendor = 'AuthenticAMD',
    family = 0x17,
    models = { { 0x90, 0x9f } },
    msrs = function()
      return {
        [0x8b] = true,
        [0xc00000e7] = true,
        [0xc00000e8] = true,
        [0xe7] = { gate = { 0x6, 2, 0, 1, 0x1, 0x1 } },
        [0xe8] = { gate = { 0x6, 2, 0, 1, 0x1, 0x1 } },
        [0xc0010015] = true,
        [0xc0010061] = true,
        [0xc0010063] = true,
        [0xc0010064] = true,
        [0xc0010065] = true,
        [0xc0010066] = true,
        [0xc0010067] = true,
        [0xc0010068] = true,
        [0xc0010069] = true,
        [0xc001006a] = true,
        [0xc001006b] = true,
        [0xc0010293] = true,
        [0xc0010299] = { gate = { 0x80000007, 3, 14, 1, 0x1, 0x1 } },
        [0xc001029a] = { gate = { 0x80000007, 3, 14, 1, 0x1, 0x1 } },
        [0xc001029b] = { gate = { 0x80000007, 3, 14, 1, 0x1, 0x1 } },
        [0xc00102b0] = { gate = { 0x80000008, 1, 27, 1, 0x1, 0x1 } },
        [0xc00102b1] = { gate = { 0x80000008, 1, 27, 1, 0x1, 0x1 } },
        [0xc00102b2] = { gate = { 0x80000008, 1, 27, 1, 0x1, 0x1 } },
        [0xc00102b3] = { gate = { 0x80000008, 1, 27, 1, 0x1, 0x1 } },
        [0xc00102b4] = { gate = { 0x80000008, 1, 27, 1, 0x1, 0x1 } },
        [0xc00102b5] = { gate = { 0x80000008, 1, 27, 1, 0x1, 0x1 } },
      }
    end,
  },
  {
    vendor = 'AuthenticAMD',
    family = 0x17,
    models = { { 0xa0, 0xaf } },
    msrs = function()
      return {
        [0x8b] = true,
        [0xc00000e7] = true,
        [0xc00000e8] = true,
        [0xe7] = { gate = { 0x6, 2, 0, 1, 0x1, 0x1 } },
        [0xe8] = { gate = { 0x6, 2, 0, 1, 0x1, 0x1 } },
        [0xc0010015] = true,
        [0xc0010061] = true,
        [0xc0010063] = true,
        [0xc0010064] = true,
        [0xc0010065] = true,
        [0xc0010066] = true,
        [0xc0010067] = true,
        [0xc0010068] = true,
        [0xc0010069] = true,
        [0xc001006a] = true,
        [0xc001006b] = true,
        [0xc0010293] = true,
        [0xc0010299] = { gate = { 0x80000007, 3, 14, 1, 0x1, 0x1 } },
        [0xc001029a] = { gate = { 0x80000007, 3, 14, 1, 0x1, 0x1 } },
        [0xc001029b] = { gate = { 0x80000007, 3, 14, 1, 0x1, 0x1 } },
        [0xc00102b0] = { gate = { 0x80000008, 1, 27, 1, 0x1, 0x1 } },
        [0xc00102b1] = { gate = { 0x80000008, 1, 27, 1, 0x1, 0x1 } },
        [0xc00102b2] = { gate = { 0x80000008, 1, 27, 1, 0x1, 0x1 } },
        [0xc00102b3] = { gate = { 0x80000008, 1, 27, 1, 0x1, 0x1 } },
        [0xc00102b4] = { gate = { 0x80000008, 1, 27, 1, 0x1, 0x1 } },
        [0xc00102b5] = { gate = { 0x80000008, 1, 27, 1, 0x1, 0x1 } },
      }
    end,
  },
  {
    vendor = 'HygonGenuine',
    family = 0x18,
    models = { { 0x00, 0x0f } },
    msrs = function()
      return {
        [0x8b] = true,
        [0xc00000e7] = true,
        [0xc00000e8] = true,
        [0xe7] = { gate = { 0x6, 2, 0, 1, 0x1, 0x1 } },
        [0xe8] = { gate = { 0x6, 2, 0, 1, 0x1, 0x1 } },
        [0xc0010015] = true,
        [0xc0010061] = true,
        [0xc0010063] = true,
        [0xc0010064] = true,
        [0xc0010065] = true,
        [0xc0010066] = true,
        [0xc0010067] = true,
        [0xc0010068] = true,
        [0xc0010069] = true,
        [0xc001006a] = true,
        [0xc001006b] = true,
        [0xc0010293] = true,
        [0xc0010299] = { gate = { 0x80000007, 3, 14, 1, 0x1, 0x1 } },
        [0xc001029a] = { gate = { 0x80000007, 3, 14, 1, 0x1, 0x1 } },
        [0xc001029b] = { gate = { 0x80000007, 3, 14, 1, 0x1, 0x1 } },
      }
    end,
  },
  {
    vendor = 'CentaurHauls',
    family = 0x06,
    models = { { 0x0f, 0x0f } },
    msrs = function()
      return {
        [0x1423] = true,
        [0xe7] = { gate = { 0x6, 2, 0, 1, 0x1, 0x1 } },
        [0xe8] = { gate = { 0x6, 2, 0, 1, 0x1, 0x1 } },
      }
    end,
  },
  {
    vendor = 'CentaurHauls',
    family = 0x07,
    models = { { 0x1b, 0x1b } },
    msrs = function()
      return {
        [0x1423] = true,
        [0xe7] = { gate = { 0x6, 2, 0, 1, 0x1, 0x1 } },
        [0xe8] = { gate = { 0x6, 2, 0, 1, 0x1, 0x1 } },
      }
    end,
  },
  {
    vendor = 'CentaurHauls',
    family = 0x07,
    models = { { 0x3b, 0x3b }, { 0x5b, 0x5b } },
    msrs = function()
      return {
        [0x1423] = true,
        [0x1415] = true,
        [0x1416] = true,
        [0xe7] = { gate = { 0x6, 2, 0, 1, 0x1, 0x1 } },
        [0xe8] = { gate = { 0x6, 2, 0, 1, 0x1, 0x1 } },
      }
    end,
  },
  {
    vendor = 'CentaurHauls',
    family = 0x07,
    models = { { 0x6b, 0x6b }, { 0x7b, 0x7b } },
    msrs = function()
      return {
        [0x1423] = true,
        [0x175a] = true,
        [0x175b] = true,
        [0xe7] = { gate = { 0x6, 2, 0, 1, 0x1, 0x1 } },
        [0xe8] = { gate = { 0x6, 2, 0, 1, 0x1, 0x1 } },
      }
    end,
  },
  {
    vendor = '  Shanghai  ',
    family = 0x07,
    models = { { 0x3b, 0x3b }, { 0x5b, 0x5b } },
    msrs = function()
      return {
        [0x1423] = true,
        [0x1415] = true,
        [0x1416] = true,
        [0xe7] = { gate = { 0x6, 2, 0, 1, 0x1, 0x1 } },
        [0xe8] = { gate = { 0x6, 2, 0, 1, 0x1, 0x1 } },
      }
    end,
  },
  {
    vendor = '  Shanghai  ',
    family = 0x07,
    models = { { 0x6b, 0x6b }, { 0x7b, 0x7b } },
    msrs = function()
      return {
        [0x1423] = true,
        [0x175a] = true,
        [0x175b] = true,
        [0xe7] = { gate = { 0x6, 2, 0, 1, 0x1, 0x1 } },
        [0xe8] = { gate = { 0x6, 2, 0, 1, 0x1, 0x1 } },
      }
    end,
  },
  {
    vendor = 'AuthenticAMD',
    family = 0x19,
    models = { { 0x00, 0x0f } },
    msrs = function()
      return {
        [0x8b] = true,
        [0xc00000e7] = true,
        [0xc00000e8] = true,
        [0xe7] = { gate = { 0x6, 2, 0, 1, 0x1, 0x1 } },
        [0xe8] = { gate = { 0x6, 2, 0, 1, 0x1, 0x1 } },
        [0xc0010015] = true,
        [0xc0010061] = true,
        [0xc0010063] = true,
        [0xc0010064] = true,
        [0xc0010065] = true,
        [0xc0010066] = true,
        [0xc0010067] = true,
        [0xc0010068] = true,
        [0xc0010069] = true,
        [0xc001006a] = true,
        [0xc001006b] = true,
        [0xc0010293] = true,
        [0xc0010299] = { gate = { 0x80000007, 3, 14, 1, 0x1, 0x1 } },
        [0xc001029a] = { gate = { 0x80000007, 3, 14, 1, 0x1, 0x1 } },
        [0xc001029b] = { gate = { 0x80000007, 3, 14, 1, 0x1, 0x1 } },
        [0xc00102b0] = { gate = { 0x80000008, 1, 27, 1, 0x1, 0x1 } },
        [0xc00102b1] = { gate = { 0x80000008, 1, 27, 1, 0x1, 0x1 } },
        [0xc00102b2] = { gate = { 0x80000008, 1, 27, 1, 0x1, 0x1 } },
        [0xc00102b3] = { gate = { 0x80000008, 1, 27, 1, 0x1, 0x1 } },
        [0xc00102b4] = { gate = { 0x80000008, 1, 27, 1, 0x1, 0x1 } },
        [0xc00102b5] = { gate = { 0x80000008, 1, 27, 1, 0x1, 0x1 } },
      }
    end,
  },
  {
    vendor = 'AuthenticAMD',
    family = 0x19,
    models = { { 0x30, 0x3f } },
    msrs = function()
      return {
        [0x8b] = true,
        [0xc00000e7] = true,
        [0xc00000e8] = true,
        [0xe7] = { gate = { 0x6, 2, 0, 1, 0x1, 0x1 } },
        [0xe8] = { gate = { 0x6, 2, 0, 1, 0x1, 0x1 } },
        [0xc0010015] = true,
        [0xc0010061] = true,
        [0xc0010063] = true,
        [0xc0010064] = true,
        [0xc0010065] = true,
        [0xc0010066] = true,
        [0xc0010067] = true,
        [0xc0010068] = true,
        [0xc0010069] = true,
        [0xc001006a] = true,
        [0xc001006b] = true,
        [0xc0010293] = true,
        [0xc0010299] = { gate = { 0x80000007, 3, 14, 1, 0x1, 0x1 } },
        [0xc001029a] = { gate = { 0x80000007, 3, 14, 1, 0x1, 0x1 } },
        [0xc001029b] = { gate = { 0x80000007, 3, 14, 1, 0x1, 0x1 } },
        [0xc00102b0] = { gate = { 0x80000008, 1, 27, 1, 0x1, 0x1 } },
        [0xc00102b1] = { gate = { 0x80000008, 1, 27, 1, 0x1, 0x1 } },
        [0xc00102b2] = { gate = { 0x80000008, 1, 27, 1, 0x1, 0x1 } },
        [0xc00102b3] = { gate = { 0x80000008, 1, 27, 1, 0x1, 0x1 } },
        [0xc00102b4] = { gate = { 0x80000008, 1, 27, 1, 0x1, 0x1 } },
        [0xc00102b5] = { gate = { 0x80000008, 1, 27, 1, 0x1, 0x1 } },
      }
    end,
  },
  {
    vendor = 'AuthenticAMD',
    family = 0x19,
    models = { { 0x10, 0x17 }, { 0x19, 0x1f } },
    msrs = function()
      return {
        [0x8b] = true,
        [0xc00000e7] = true,
        [0xc00000e8] = true,
        [0xe7] = { gate = { 0x6, 2, 0, 1, 0x1, 0x1 } },
        [0xe8] = { gate = { 0x6, 2, 0, 1, 0x1, 0x1 } },
        [0xc0010015] = true,
        [0xc0010061] = true,
        [0xc0010063] = true,
        [0xc0010064] = true,
        [0xc0010065] = true,
        [0xc0010066] = true,
        [0xc0010067] = true,
        [0xc0010068] = true,
        [0xc0010069] = true,
        [0xc001006a] = true,
        [0xc001006b] = true,
        [0xc0010293] = true,
        [0xc0010299] = { gate = { 0x80000007, 3, 14, 1, 0x1, 0x1 } },
        [0xc001029a] = { gate = { 0x80000007, 3, 14, 1, 0x1, 0x1 } },
        [0xc001029b] = { gate = { 0x80000007, 3, 14, 1, 0x1, 0x1 } },
        [0xc00102b0] = { gate = { 0x80000008, 1, 27, 1, 0x1, 0x1 } },
        [0xc00102b1] = { gate = { 0x80000008, 1, 27, 1, 0x1, 0x1 } },
        [0xc00102b2] = { gate = { 0x80000008, 1, 27, 1, 0x1, 0x1 } },
        [0xc00102b3] = { gate = { 0x80000008, 1, 27, 1, 0x1, 0x1 } },
        [0xc00102b4] = { gate = { 0x80000008, 1, 27, 1, 0x1, 0x1 } },
        [0xc00102b5] = { gate = { 0x80000008, 1, 27, 1, 0x1, 0x1 } },
      }
    end,
  },
  {
    vendor = 'AuthenticAMD',
    family = 0x19,
    models = { { 0x90, 0x9f } },
    msrs = function()
      return {
        [0x8b] = true,
        [0xc00000e7] = true,
        [0xc00000e8] = true,
        [0xe7] = { gate = { 0x6, 2, 0, 1, 0x1, 0x1 } },
        [0xe8] = { gate = { 0x6, 2, 0, 1, 0x1, 0x1 } },
        [0xc0010015] = true,
        [0xc0010061] = true,
        [0xc0010063] = true,
        [0xc0010064] = true,
        [0xc0010065] = true,
        [0xc0010066] = true,
        [0xc0010067] = true,
        [0xc0010068] = true,
        [0xc0010069] = true,
        [0xc001006a] = true,
        [0xc001006b] = true,
        [0xc0010293] = true,
        [0xc0010299] = { gate = { 0x80000007, 3, 14, 1, 0x1, 0x1 } },
        [0xc001029a] = { gate = { 0x80000007, 3, 14, 1, 0x1, 0x1 } },
        [0xc001029b] = { gate = { 0x80000007, 3, 14, 1, 0x1, 0x1 } },
        [0xc00102b0] = { gate = { 0x80000008, 1, 27, 1, 0x1, 0x1 } },
        [0xc00102b1] = { gate = { 0x80000008, 1, 27, 1, 0x1, 0x1 } },
        [0xc00102b2] = { gate = { 0x80000008, 1, 27, 1, 0x1, 0x1 } },
        [0xc00102b3] = { gate = { 0x80000008, 1, 27, 1, 0x1, 0x1 } },
        [0xc00102b4] = { gate = { 0x80000008, 1, 27, 1, 0x1, 0x1 } },
        [0xc00102b5] = { gate = { 0x80000008, 1, 27, 1, 0x1, 0x1 } },
      }
    end,
  },
  {
    vendor = 'AuthenticAMD',
    family = 0x19,
    models = { { 0x18, 0x18 } },
    msrs = function()
      return {
        [0x8b] = true,
        [0xc00000e7] = true,
        [0xc00000e8] = true,
        [0xe7] = { gate = { 0x6, 2, 0, 1, 0x1, 0x1 } },
        [0xe8] = { gate = { 0x6, 2, 0, 1, 0x1, 0x1 } },
        [0xc0010015] = true,
        [0xc0010061] = true,
        [0xc0010063] = true,
        [0xc0010064] = true,
        [0xc0010065] = true,
        [0xc0010066] = true,
        [0xc0010067] = true,
        [0xc0010068] = true,
        [0xc0010069] = true,
        [0xc001006a] = true,
        [0xc001006b] = true,
        [0xc0010293] = true,
        [0xc0010299] = { gate = { 0x80000007, 3, 14, 1, 0x1, 0x1 } },
        [0xc001029a] = { gate = { 0x80000007, 3, 14, 1, 0x1, 0x1 } },
        [0xc001029b] = { gate = { 0x80000007, 3, 14, 1, 0x1, 0x1 } },
        [0xc00102b0] = { gate = { 0x80000008, 1, 27, 1, 0x1, 0x1 } },
        [0xc00102b1] = { gate = { 0x80000008, 1, 27, 1, 0x1, 0x1 } },
        [0xc00102b2] = { gate = { 0x80000008, 1, 27, 1, 0x1, 0x1 } },
        [0xc00102b3] = { gate = { 0x80000008, 1, 27, 1, 0x1, 0x1 } },
        [0xc00102b4] = { gate = { 0x80000008, 1, 27, 1, 0x1, 0x1 } },
        [0xc00102b5] = { gate = { 0x80000008, 1, 27, 1, 0x1, 0x1 } },
      }
    end,
  },
  {
    vendor = 'AuthenticAMD',
    family = 0x19,
    models = { { 0x20, 0x2f } },
    msrs = function()
      return {
        [0x8b] = true,
        [0xc00000e7] = true,
        [0xc00000e8] = true,
        [0xe7] = { gate = { 0x6, 2, 0, 1, 0x1, 0x1 } },
        [0xe8] = { gate = { 0x6, 2, 0, 1, 0x1, 0x1 } },
        [0xc0010015] = true,
        [0xc0010061] = true,
        [0xc0010063] = true,
        [0xc0010064] = true,
        [0xc0010065] = true,
        [0xc0010066] = true,
        [0xc0010067] = true,
        [0xc0010068] = true,
        [0xc0010069] = true,
        [0xc001006a] = true,
        [0xc001006b] = true,
        [0xc0010293] = true,
        [0xc0010299] = { gate = { 0x80000007, 3, 14, 1, 0x1, 0x1 } },
        [0xc001029a] = { gate = { 0x80000007, 3, 14, 1, 0x1, 0x1 } },
        [0xc001029b] = { gate = { 0x80000007, 3, 14, 1, 0x1, 0x1 } },
        [0xc00102b0] = { gate = { 0x80000008, 1, 27, 1, 0x1, 0x1 } },
        [0xc00102b1] = { gate = { 0x80000008, 1, 27, 1, 0x1, 0x1 } },
        [0xc00102b2] = { gate = { 0x80000008, 1, 27, 1, 0x1, 0x1 } },
        [0xc00102b3] = { gate = { 0x80000008, 1, 27, 1, 0x1, 0x1 } },
        [0xc00102b4] = { gate = { 0x80000008, 1, 27, 1, 0x1, 0x1 } },
        [0xc00102b5] = { gate = { 0x80000008, 1, 27, 1, 0x1, 0x1 } },
      }
    end,
  },
  {
    vendor = 'AuthenticAMD',
    family = 0x19,
    models = { { 0x40, 0x4f } },
    msrs = function()
      return {
        [0x8b] = true,
        [0xc00000e7] = true,
        [0xc00000e8] = true,
        [0xe7] = { gate = { 0x6, 2, 0, 1, 0x1, 0x1 } },
        [0xe8] = { gate = { 0x6, 2, 0, 1, 0x1, 0x1 } },
        [0xc0010015] = true,
        [0xc0010061] = true,
        [0xc0010063] = true,
        [0xc0010064] = true,
        [0xc0010065] = true,
        [0xc0010066] = true,
        [0xc0010067] = true,
        [0xc0010068] = true,
        [0xc0010069] = true,
        [0xc001006a] = true,
        [0xc001006b] = true,
        [0xc0010293] = true,
        [0xc0010299] = { gate = { 0x80000007, 3, 14, 1, 0x1, 0x1 } },
        [0xc001029a] = { gate = { 0x80000007, 3, 14, 1, 0x1, 0x1 } },
        [0xc001029b] = { gate = { 0x80000007, 3, 14, 1, 0x1, 0x1 } },
        [0xc00102b0] = { gate = { 0x80000008, 1, 27, 1, 0x1, 0x1 } },
        [0xc00102b1] = { gate = { 0x80000008, 1, 27, 1, 0x1, 0x1 } },
        [0xc00102b2] = { gate = { 0x80000008, 1, 27, 1, 0x1, 0x1 } },
        [0xc00102b3] = { gate = { 0x80000008, 1, 27, 1, 0x1, 0x1 } },
        [0xc00102b4] = { gate = { 0x80000008, 1, 27, 1, 0x1, 0x1 } },
        [0xc00102b5] = { gate = { 0x80000008, 1, 27, 1, 0x1, 0x1 } },
      }
    end,
  },
  {
    vendor = 'AuthenticAMD',
    family = 0x19,
    models = { { 0x50, 0x5f } },
    msrs = function()
      return {
        [0x8b] = true,
        [0xc00000e7] = true,
        [0xc00000e8] = true,
        [0xe7] = { gate = { 0x6, 2, 0, 1, 0x1, 0x1 } },
        [0xe8] = { gate = { 0x6, 2, 0, 1, 0x1, 0x1 } },
        [0xc0010015] = true,
        [0xc0010061] = true,
        [0xc0010063] = true,
        [0xc0010064] = true,
        [0xc0010065] = true,
        [0xc0010066] = true,
        [0xc0010067] = true,
        [0xc0010068] = true,
        [0xc0010069] = true,
        [0xc001006a] = true,
        [0xc001006b] = true,
        [0xc0010293] = true,
        [0xc0010299] = { gate = { 0x80000007, 3, 14, 1, 0x1, 0x1 } },
        [0xc001029a] = { gate = { 0x80000007, 3, 14, 1, 0x1, 0x1 } },
        [0xc001029b] = { gate = { 0x80000007, 3, 14, 1, 0x1, 0x1 } },
        [0xc00102b0] = { gate = { 0x80000008, 1, 27, 1, 0x1, 0x1 } },
        [0xc00102b1] = { gate = { 0x80000008, 1, 27, 1, 0x1, 0x1 } },
        [0xc00102b2] = { gate = { 0x80000008, 1, 27, 1, 0x1, 0x1 } },
        [0xc00102b3] = { gate = { 0x80000008, 1, 27, 1, 0x1, 0x1 } },
        [0xc00102b4] = { gate = { 0x80000008, 1, 27, 1, 0x1, 0x1 } },
        [0xc00102b5] = { gate = { 0x80000008, 1, 27, 1, 0x1, 0x1 } },
      }
    end,
  },
  {
    vendor = 'AuthenticAMD',
    family = 0x19,
    models = { { 0x60, 0x6f } },
    msrs = function()
      return {
        [0x8b] = true,
        [0xc00000e7] = true,
        [0xc00000e8] = true,
        [0xe7] = { gate = { 0x6, 2, 0, 1, 0x1, 0x1 } },
        [0xe8] = { gate = { 0x6, 2, 0, 1, 0x1, 0x1 } },
        [0xc0010015] = true,
        [0xc0010061] = true,
        [0xc0010063] = true,
        [0xc0010064] = true,
        [0xc0010065] = true,
        [0xc0010066] = true,
        [0xc0010067] = true,
        [0xc0010068] = true,
        [0xc0010069] = true,
        [0xc001006a] = true,
        [0xc001006b] = true,
        [0xc0010293] = true,
        [0xc0010299] = { gate = { 0x80000007, 3, 14, 1, 0x1, 0x1 } },
        [0xc001029a] = { gate = { 0x80000007, 3, 14, 1, 0x1, 0x1 } },
        [0xc001029b] = { gate = { 0x80000007, 3, 14, 1, 0x1, 0x1 } },
        [0xc00102b0] = { gate = { 0x80000008, 1, 27, 1, 0x1, 0x1 } },
        [0xc00102b1] = { gate = { 0x80000008, 1, 27, 1, 0x1, 0x1 } },
        [0xc00102b2] = { gate = { 0x80000008, 1, 27, 1, 0x1, 0x1 } },
        [0xc00102b3] = { gate = { 0x80000008, 1, 27, 1, 0x1, 0x1 } },
        [0xc00102b4] = { gate = { 0x80000008, 1, 27, 1, 0x1, 0x1 } },
        [0xc00102b5] = { gate = { 0x80000008, 1, 27, 1, 0x1, 0x1 } },
      }
    end,
  },
  {
    vendor = 'AuthenticAMD',
    family = 0x19,
    models = { { 0x70, 0x7f } },
    msrs = function()
      return {
        [0x8b] = true,
        [0xc00000e7] = true,
        [0xc00000e8] = true,
        [0xe7] = { gate = { 0x6, 2, 0, 1, 0x1, 0x1 } },
        [0xe8] = { gate = { 0x6, 2, 0, 1, 0x1, 0x1 } },
        [0xc0010015] = true,
        [0xc0010061] = true,
        [0xc0010063] = true,
        [0xc0010064] = true,
        [0xc0010065] = true,
        [0xc0010066] = true,
        [0xc0010067] = true,
        [0xc0010068] = true,
        [0xc0010069] = true,
        [0xc001006a] = true,
        [0xc001006b] = true,
        [0xc0010293] = true,
        [0xc0010299] = { gate = { 0x80000007, 3, 14, 1, 0x1, 0x1 } },
        [0xc001029a] = { gate = { 0x80000007, 3, 14, 1, 0x1, 0x1 } },
        [0xc001029b] = { gate = { 0x80000007, 3, 14, 1, 0x1, 0x1 } },
        [0xc00102b0] = { gate = { 0x80000008, 1, 27, 1, 0x1, 0x1 } },
        [0xc00102b1] = { gate = { 0x80000008, 1, 27, 1, 0x1, 0x1 } },
        [0xc00102b2] = { gate = { 0x80000008, 1, 27, 1, 0x1, 0x1 } },
        [0xc00102b3] = { gate = { 0x80000008, 1, 27, 1, 0x1, 0x1 } },
        [0xc00102b4] = { gate = { 0x80000008, 1, 27, 1, 0x1, 0x1 } },
        [0xc00102b5] = { gate = { 0x80000008, 1, 27, 1, 0x1, 0x1 } },
      }
    end,
  },
  {
    vendor = 'AuthenticAMD',
    family = 0x19,
    models = { { 0xa0, 0xaf } },
    msrs = function()
      return {
        [0x8b] = true,
        [0xc00000e7] = true,
        [0xc00000e8] = true,
        [0xe7] = { gate = { 0x6, 2, 0, 1, 0x1, 0x1 } },
        [0xe8] = { gate = { 0x6, 2, 0, 1, 0x1, 0x1 } },
        [0xc0010015] = true,
        [0xc0010061] = true,
        [0xc0010063] = true,
        [0xc0010064] = true,
        [0xc0010065] = true,
        [0xc0010066] = true,
        [0xc0010067] = true,
        [0xc0010068] = true,
        [0xc0010069] = true,
        [0xc001006a] = true,
        [0xc001006b] = true,
        [0xc0010293] = true,
        [0xc0010299] = { gate = { 0x80000007, 3, 14, 1, 0x1, 0x1 } },
        [0xc001029a] = { gate = { 0x80000007, 3, 14, 1, 0x1, 0x1 } },
        [0xc001029b] = { gate = { 0x80000007, 3, 14, 1, 0x1, 0x1 } },
        [0xc00102b0] = { gate = { 0x80000008, 1, 27, 1, 0x1, 0x1 } },
        [0xc00102b1] = { gate = { 0x80000008, 1, 27, 1, 0x1, 0x1 } },
        [0xc00102b2] = { gate = { 0x80000008, 1, 27, 1, 0x1, 0x1 } },
        [0xc00102b3] = { gate = { 0x80000008, 1, 27, 1, 0x1, 0x1 } },
        [0xc00102b4] = { gate = { 0x80000008, 1, 27, 1, 0x1, 0x1 } },
        [0xc00102b5] = { gate = { 0x80000008, 1, 27, 1, 0x1, 0x1 } },
      }
    end,
  },
  {
    vendor = 'AuthenticAMD',
    family = 0x1a,
    models = { { 0x00, 0x07 }, { 0x09, 0x0f } },
    msrs = function()
      return {
        [0x8b] = true,
        [0xc00000e7] = true,
        [0xc00000e8] = true,
        [0xe7] = { gate = { 0x6, 2, 0, 1, 0x1, 0x1 } },
        [0xe8] = { gate = { 0x6, 2, 0, 1, 0x1, 0x1 } },
        [0xc0010015] = true,
        [0xc0010061] = true,
        [0xc0010063] = true,
        [0xc0010064] = true,
        [0xc0010065] = true,
        [0xc0010066] = true,
        [0xc0010067] = true,
        [0xc0010068] = true,
        [0xc0010069] = true,
        [0xc001006a] = true,
        [0xc001006b] = true,
        [0xc0010293] = true,
        [0xc0010299] = { gate = { 0x80000007, 3, 14, 1, 0x1, 0x1 } },
        [0xc001029a] = { gate = { 0x80000007, 3, 14, 1, 0x1, 0x1 } },
        [0xc001029b] = { gate = { 0x80000007, 3, 14, 1, 0x1, 0x1 } },
        [0xc00102b0] = { gate = { 0x80000008, 1, 27, 1, 0x1, 0x1 } },
        [0xc00102b1] = { gate = { 0x80000008, 1, 27, 1, 0x1, 0x1 } },
        [0xc00102b2] = { gate = { 0x80000008, 1, 27, 1, 0x1, 0x1 } },
        [0xc00102b3] = { gate = { 0x80000008, 1, 27, 1, 0x1, 0x1 } },
        [0xc00102b4] = { gate = { 0x80000008, 1, 27, 1, 0x1, 0x1 } },
        [0xc00102b5] = { gate = { 0x80000008, 1, 27, 1, 0x1, 0x1 } },
      }
    end,
  },
  {
    vendor = 'AuthenticAMD',
    family = 0x1a,
    models = { { 0x08, 0x08 } },
    msrs = function()
      return {
        [0x8b] = true,
        [0xc00000e7] = true,
        [0xc00000e8] = true,
        [0xe7] = { gate = { 0x6, 2, 0, 1, 0x1, 0x1 } },
        [0xe8] = { gate = { 0x6, 2, 0, 1, 0x1, 0x1 } },
        [0xc0010015] = true,
        [0xc0010061] = true,
        [0xc0010063] = true,
        [0xc0010064] = true,
        [0xc0010065] = true,
        [0xc0010066] = true,
        [0xc0010067] = true,
        [0xc0010068] = true,
        [0xc0010069] = true,
        [0xc001006a] = true,
        [0xc001006b] = true,
        [0xc0010293] = true,
        [0xc0010299] = { gate = { 0x80000007, 3, 14, 1, 0x1, 0x1 } },
        [0xc001029a] = { gate = { 0x80000007, 3, 14, 1, 0x1, 0x1 } },
        [0xc001029b] = { gate = { 0x80000007, 3, 14, 1, 0x1, 0x1 } },
        [0xc00102b0] = { gate = { 0x80000008, 1, 27, 1, 0x1, 0x1 } },
        [0xc00102b1] = { gate = { 0x80000008, 1, 27, 1, 0x1, 0x1 } },
        [0xc00102b2] = { gate = { 0x80000008, 1, 27, 1, 0x1, 0x1 } },
        [0xc00102b3] = { gate = { 0x80000008, 1, 27, 1, 0x1, 0x1 } },
        [0xc00102b4] = { gate = { 0x80000008, 1, 27, 1, 0x1, 0x1 } },
        [0xc00102b5] = { gate = { 0x80000008, 1, 27, 1, 0x1, 0x1 } },
      }
    end,
  },
  {
    vendor = 'AuthenticAMD',
    family = 0x1a,
    models = { { 0x10, 0x1f } },
    msrs = function()
      return {
        [0x8b] = true,
        [0xc00000e7] = true,
        [0xc00000e8] = true,
        [0xe7] = { gate = { 0x6, 2, 0, 1, 0x1, 0x1 } },
        [0xe8] = { gate = { 0x6, 2, 0, 1, 0x1, 0x1 } },
        [0xc0010015] = true,
        [0xc0010061] = true,
        [0xc0010063] = true,
        [0xc0010064] = true,
        [0xc0010065] = true,
        [0xc0010066] = true,
        [0xc0010067] = true,
        [0xc0010068] = true,
        [0xc0010069] = true,
        [0xc001006a] = true,
        [0xc001006b] = true,
        [0xc0010293] = true,
        [0xc0010299] = { gate = { 0x80000007, 3, 14, 1, 0x1, 0x1 } },
        [0xc001029a] = { gate = { 0x80000007, 3, 14, 1, 0x1, 0x1 } },
        [0xc001029b] = { gate = { 0x80000007, 3, 14, 1, 0x1, 0x1 } },
        [0xc00102b0] = { gate = { 0x80000008, 1, 27, 1, 0x1, 0x1 } },
        [0xc00102b1] = { gate = { 0x80000008, 1, 27, 1, 0x1, 0x1 } },
        [0xc00102b2] = { gate = { 0x80000008, 1, 27, 1, 0x1, 0x1 } },
        [0xc00102b3] = { gate = { 0x80000008, 1, 27, 1, 0x1, 0x1 } },
        [0xc00102b4] = { gate = { 0x80000008, 1, 27, 1, 0x1, 0x1 } },
        [0xc00102b5] = { gate = { 0x80000008, 1, 27, 1, 0x1, 0x1 } },
      }
    end,
  },
  {
    vendor = 'AuthenticAMD',
    family = 0x1a,
    models = { { 0x20, 0x2f } },
    msrs = function()
      return {
        [0x8b] = true,
        [0xc00000e7] = true,
        [0xc00000e8] = true,
        [0xe7] = { gate = { 0x6, 2, 0, 1, 0x1, 0x1 } },
        [0xe8] = { gate = { 0x6, 2, 0, 1, 0x1, 0x1 } },
        [0xc0010015] = true,
        [0xc0010061] = true,
        [0xc0010063] = true,
        [0xc0010064] = true,
        [0xc0010065] = true,
        [0xc0010066] = true,
        [0xc0010067] = true,
        [0xc0010068] = true,
        [0xc0010069] = true,
        [0xc001006a] = true,
        [0xc001006b] = true,
        [0xc0010293] = true,
        [0xc0010299] = { gate = { 0x80000007, 3, 14, 1, 0x1, 0x1 } },
        [0xc001029a] = { gate = { 0x80000007, 3, 14, 1, 0x1, 0x1 } },
        [0xc001029b] = { gate = { 0x80000007, 3, 14, 1, 0x1, 0x1 } },
        [0xc00102b0] = { gate = { 0x80000008, 1, 27, 1, 0x1, 0x1 } },
        [0xc00102b1] = { gate = { 0x80000008, 1, 27, 1, 0x1, 0x1 } },
        [0xc00102b2] = { gate = { 0x80000008, 1, 27, 1, 0x1, 0x1 } },
        [0xc00102b3] = { gate = { 0x80000008, 1, 27, 1, 0x1, 0x1 } },
        [0xc00102b4] = { gate = { 0x80000008, 1, 27, 1, 0x1, 0x1 } },
        [0xc00102b5] = { gate = { 0x80000008, 1, 27, 1, 0x1, 0x1 } },
      }
    end,
  },
  {
    vendor = 'AuthenticAMD',
    family = 0x1a,
    models = { { 0x40, 0x4f } },
    msrs = function()
      return {
        [0x8b] = true,
        [0xc00000e7] = true,
        [0xc00000e8] = true,
        [0xe7] = { gate = { 0x6, 2, 0, 1, 0x1, 0x1 } },
        [0xe8] = { gate = { 0x6, 2, 0, 1, 0x1, 0x1 } },
        [0xc0010015] = true,
        [0xc0010061] = true,
        [0xc0010063] = true,
        [0xc0010064] = true,
        [0xc0010065] = true,
        [0xc0010066] = true,
        [0xc0010067] = true,
        [0xc0010068] = true,
        [0xc0010069] = true,
        [0xc001006a] = true,
        [0xc001006b] = true,
        [0xc0010293] = true,
        [0xc0010299] = { gate = { 0x80000007, 3, 14, 1, 0x1, 0x1 } },
        [0xc001029a] = { gate = { 0x80000007, 3, 14, 1, 0x1, 0x1 } },
        [0xc001029b] = { gate = { 0x80000007, 3, 14, 1, 0x1, 0x1 } },
        [0xc00102b0] = { gate = { 0x80000008, 1, 27, 1, 0x1, 0x1 } },
        [0xc00102b1] = { gate = { 0x80000008, 1, 27, 1, 0x1, 0x1 } },
        [0xc00102b2] = { gate = { 0x80000008, 1, 27, 1, 0x1, 0x1 } },
        [0xc00102b3] = { gate = { 0x80000008, 1, 27, 1, 0x1, 0x1 } },
        [0xc00102b4] = { gate = { 0x80000008, 1, 27, 1, 0x1, 0x1 } },
        [0xc00102b5] = { gate = { 0x80000008, 1, 27, 1, 0x1, 0x1 } },
      }
    end,
  },
  {
    vendor = 'AuthenticAMD',
    family = 0x1a,
    models = { { 0x50, 0x5f } },
    msrs = function()
      return {
        [0x8b] = true,
        [0xc00000e7] = true,
        [0xc00000e8] = true,
        [0xe7] = { gate = { 0x6, 2, 0, 1, 0x1, 0x1 } },
        [0xe8] = { gate = { 0x6, 2, 0, 1, 0x1, 0x1 } },
        [0xc0010015] = true,
        [0xc0010061] = true,
        [0xc0010063] = true,
        [0xc0010064] = true,
        [0xc0010065] = true,
        [0xc0010066] = true,
        [0xc0010067] = true,
        [0xc0010068] = true,
        [0xc0010069] = true,
        [0xc001006a] = true,
        [0xc001006b] = true,
        [0xc0010293] = true,
        [0xc0010299] = { gate = { 0x80000007, 3, 14, 1, 0x1, 0x1 } },
        [0xc001029a] = { gate = { 0x80000007, 3, 14, 1, 0x1, 0x1 } },
        [0xc001029b] = { gate = { 0x80000007, 3, 14, 1, 0x1, 0x1 } },
        [0xc00102b0] = { gate = { 0x80000008, 1, 27, 1, 0x1, 0x1 } },
        [0xc00102b1] = { gate = { 0x80000008, 1, 27, 1, 0x1, 0x1 } },
        [0xc00102b2] = { gate = { 0x80000008, 1, 27, 1, 0x1, 0x1 } },
        [0xc00102b3] = { gate = { 0x80000008, 1, 27, 1, 0x1, 0x1 } },
        [0xc00102b4] = { gate = { 0x80000008, 1, 27, 1, 0x1, 0x1 } },
        [0xc00102b5] = { gate = { 0x80000008, 1, 27, 1, 0x1, 0x1 } },
      }
    end,
  },
  {
    vendor = 'AuthenticAMD',
    family = 0x1a,
    models = { { 0x60, 0x6f } },
    msrs = function()
      return {
        [0x8b] = true,
        [0xc00000e7] = true,
        [0xc00000e8] = true,
        [0xe7] = { gate = { 0x6, 2, 0, 1, 0x1, 0x1 } },
        [0xe8] = { gate = { 0x6, 2, 0, 1, 0x1, 0x1 } },
        [0xc0010015] = true,
        [0xc0010061] = true,
        [0xc0010063] = true,
        [0xc0010064] = true,
        [0xc0010065] = true,
        [0xc0010066] = true,
        [0xc0010067] = true,
        [0xc0010068] = true,
        [0xc0010069] = true,
        [0xc001006a] = true,
        [0xc001006b] = true,
        [0xc0010293] = true,
        [0xc0010299] = { gate = { 0x80000007, 3, 14, 1, 0x1, 0x1 } },
        [0xc001029a] = { gate = { 0x80000007, 3, 14, 1, 0x1, 0x1 } },
        [0xc001029b] = { gate = { 0x80000007, 3, 14, 1, 0x1, 0x1 } },
        [0xc00102b0] = { gate = { 0x80000008, 1, 27, 1, 0x1, 0x1 } },
        [0xc00102b1] = { gate = { 0x80000008, 1, 27, 1, 0x1, 0x1 } },
        [0xc00102b2] = { gate = { 0x80000008, 1, 27, 1, 0x1, 0x1 } },
        [0xc00102b3] = { gate = { 0x80000008, 1, 27, 1, 0x1, 0x1 } },
        [0xc00102b4] = { gate = { 0x80000008, 1, 27, 1, 0x1, 0x1 } },
        [0xc00102b5] = { gate = { 0x80000008, 1, 27, 1, 0x1, 0x1 } },
      }
    end,
  },
  {
    vendor = 'AuthenticAMD',
    family = 0x1a,
    models = { { 0x70, 0x7f } },
    msrs = function()
      return {
        [0x8b] = true,
        [0xc00000e7] = true,
        [0xc00000e8] = true,
        [0xe7] = { gate = { 0x6, 2, 0, 1, 0x1, 0x1 } },
        [0xe8] = { gate = { 0x6, 2, 0, 1, 0x1, 0x1 } },
        [0xc0010015] = true,
        [0xc0010061] = true,
        [0xc0010063] = true,
        [0xc0010064] = true,
        [0xc0010065] = true,
        [0xc0010066] = true,
        [0xc0010067] = true,
        [0xc0010068] = true,
        [0xc0010069] = true,
        [0xc001006a] = true,
        [0xc001006b] = true,
        [0xc0010293] = true,
        [0xc0010299] = { gate = { 0x80000007, 3, 14, 1, 0x1, 0x1 } },
        [0xc001029a] = { gate = { 0x80000007, 3, 14, 1, 0x1, 0x1 } },
        [0xc001029b] = { gate = { 0x80000007, 3, 14, 1, 0x1, 0x1 } },
        [0xc00102b0] = { gate = { 0x80000008, 1, 27, 1, 0x1, 0x1 } },
        [0xc00102b1] = { gate = { 0x80000008, 1, 27, 1, 0x1, 0x1 } },
        [0xc00102b2] = { gate = { 0x80000008, 1, 27, 1, 0x1, 0x1 } },
        [0xc00102b3] = { gate = { 0x80000008, 1, 27, 1, 0x1, 0x1 } },
        [0xc00102b4] = { gate = { 0x80000008, 1, 27, 1, 0x1, 0x1 } },
        [0xc00102b5] = { gate = { 0x80000008, 1, 27, 1, 0x1, 0x1 } },
      }
    end,
  },
  {
    vendor = 'AuthenticAMD',
    family = 0x1a,
    models = { { 0x80, 0x8f } },
    msrs = function()
      return {
        [0x8b] = true,
        [0xc00000e7] = true,
        [0xc00000e8] = true,
        [0xe7] = { gate = { 0x6, 2, 0, 1, 0x1, 0x1 } },
        [0xe8] = { gate = { 0x6, 2, 0, 1, 0x1, 0x1 } },
        [0xc0010015] = true,
        [0xc0010061] = true,
        [0xc0010063] = true,
        [0xc0010064] = true,
        [0xc0010065] = true,
        [0xc0010066] = true,
        [0xc0010067] = true,
        [0xc0010068] = true,
        [0xc0010069] = true,
        [0xc001006a] = true,
        [0xc001006b] = true,
        [0xc0010293] = true,
        [0xc0010299] = { gate = { 0x80000007, 3, 14, 1, 0x1, 0x1 } },
        [0xc001029a] = { gate = { 0x80000007, 3, 14, 1, 0x1, 0x1 } },
        [0xc001029b] = { gate = { 0x80000007, 3, 14, 1, 0x1, 0x1 } },
        [0xc00102b0] = { gate = { 0x80000008, 1, 27, 1, 0x1, 0x1 } },
        [0xc00102b1] = { gate = { 0x80000008, 1, 27, 1, 0x1, 0x1 } },
        [0xc00102b2] = { gate = { 0x80000008, 1, 27, 1, 0x1, 0x1 } },
        [0xc00102b3] = { gate = { 0x80000008, 1, 27, 1, 0x1, 0x1 } },
        [0xc00102b4] = { gate = { 0x80000008, 1, 27, 1, 0x1, 0x1 } },
        [0xc00102b5] = { gate = { 0x80000008, 1, 27, 1, 0x1, 0x1 } },
      }
    end,
  },
  {
    vendor = 'AuthenticAMD',
    family = 0x1a,
    models = { { 0x90, 0x9f } },
    msrs = function()
      return {
        [0x8b] = true,
        [0xc00000e7] = true,
        [0xc00000e8] = true,
        [0xe7] = { gate = { 0x6, 2, 0, 1, 0x1, 0x1 } },
        [0xe8] = { gate = { 0x6, 2, 0, 1, 0x1, 0x1 } },
        [0xc0010015] = true,
        [0xc0010061] = true,
        [0xc0010063] = true,
        [0xc0010064] = true,
        [0xc0010065] = true,
        [0xc0010066] = true,
        [0xc0010067] = true,
        [0xc0010068] = true,
        [0xc0010069] = true,
        [0xc001006a] = true,
        [0xc001006b] = true,
        [0xc0010293] = true,
        [0xc0010299] = { gate = { 0x80000007, 3, 14, 1, 0x1, 0x1 } },
        [0xc001029a] = { gate = { 0x80000007, 3, 14, 1, 0x1, 0x1 } },
        [0xc001029b] = { gate = { 0x80000007, 3, 14, 1, 0x1, 0x1 } },
        [0xc00102b0] = { gate = { 0x80000008, 1, 27, 1, 0x1, 0x1 } },
        [0xc00102b1] = { gate = { 0x80000008, 1, 27, 1, 0x1, 0x1 } },
        [0xc00102b2] = { gate = { 0x80000008, 1, 27, 1, 0x1, 0x1 } },
        [0xc00102b3] = { gate = { 0x80000008, 1, 27, 1, 0x1, 0x1 } },
        [0xc00102b4] = { gate = { 0x80000008, 1, 27, 1, 0x1, 0x1 } },
        [0xc00102b5] = { gate = { 0x80000008, 1, 27, 1, 0x1, 0x1 } },
      }
    end,
  },
  {
    vendor = 'AuthenticAMD',
    family = 0x1a,
    models = { { 0xa0, 0xaf } },
    msrs = function()
      return {
        [0x8b] = true,
        [0xc00000e7] = true,
        [0xc00000e8] = true,
        [0xe7] = { gate = { 0x6, 2, 0, 1, 0x1, 0x1 } },
        [0xe8] = { gate = { 0x6, 2, 0, 1, 0x1, 0x1 } },
        [0xc0010015] = true,
        [0xc0010061] = true,
        [0xc0010063] = true,
        [0xc0010064] = true,
        [0xc0010065] = true,
        [0xc0010066] = true,
        [0xc0010067] = true,
        [0xc0010068] = true,
        [0xc0010069] = true,
        [0xc001006a] = true,
        [0xc001006b] = true,
        [0xc0010293] = true,
        [0xc0010299] = { gate = { 0x80000007, 3, 14, 1, 0x1, 0x1 } },
        [0xc001029a] = { gate = { 0x80000007, 3, 14, 1, 0x1, 0x1 } },
        [0xc001029b] = { gate = { 0x80000007, 3, 14, 1, 0x1, 0x1 } },
        [0xc00102b0] = { gate = { 0x80000008, 1, 27, 1, 0x1, 0x1 } },
        [0xc00102b1] = { gate = { 0x80000008, 1, 27, 1, 0x1, 0x1 } },
        [0xc00102b2] = { gate = { 0x80000008, 1, 27, 1, 0x1, 0x1 } },
        [0xc00102b3] = { gate = { 0x80000008, 1, 27, 1, 0x1, 0x1 } },
        [0xc00102b4] = { gate = { 0x80000008, 1, 27, 1, 0x1, 0x1 } },
        [0xc00102b5] = { gate = { 0x80000008, 1, 27, 1, 0x1, 0x1 } },
      }
    end,
  },
  {
    vendor = 'AuthenticAMD',
    family = 0x1a,
    models = { { 0xc0, 0xcf } },
    msrs = function()
      return {
        [0x8b] = true,
        [0xc00000e7] = true,
        [0xc00000e8] = true,
        [0xe7] = { gate = { 0x6, 2, 0, 1, 0x1, 0x1 } },
        [0xe8] = { gate = { 0x6, 2, 0, 1, 0x1, 0x1 } },
        [0xc0010015] = true,
        [0xc0010061] = true,
        [0xc0010063] = true,
        [0xc0010064] = true,
        [0xc0010065] = true,
        [0xc0010066] = true,
        [0xc0010067] = true,
        [0xc0010068] = true,
        [0xc0010069] = true,
        [0xc001006a] = true,
        [0xc001006b] = true,
        [0xc0010293] = true,
        [0xc0010299] = { gate = { 0x80000007, 3, 14, 1, 0x1, 0x1 } },
        [0xc001029a] = { gate = { 0x80000007, 3, 14, 1, 0x1, 0x1 } },
        [0xc001029b] = { gate = { 0x80000007, 3, 14, 1, 0x1, 0x1 } },
        [0xc00102b0] = { gate = { 0x80000008, 1, 27, 1, 0x1, 0x1 } },
        [0xc00102b1] = { gate = { 0x80000008, 1, 27, 1, 0x1, 0x1 } },
        [0xc00102b2] = { gate = { 0x80000008, 1, 27, 1, 0x1, 0x1 } },
        [0xc00102b3] = { gate = { 0x80000008, 1, 27, 1, 0x1, 0x1 } },
        [0xc00102b4] = { gate = { 0x80000008, 1, 27, 1, 0x1, 0x1 } },
        [0xc00102b5] = { gate = { 0x80000008, 1, 27, 1, 0x1, 0x1 } },
      }
    end,
  },
  {
    vendor = 'AuthenticAMD',
    family = 0x1a,
    models = { { 0xd0, 0xd7 } },
    msrs = function()
      return {
        [0x8b] = true,
        [0xc00000e7] = true,
        [0xc00000e8] = true,
        [0xe7] = { gate = { 0x6, 2, 0, 1, 0x1, 0x1 } },
        [0xe8] = { gate = { 0x6, 2, 0, 1, 0x1, 0x1 } },
        [0xc0010015] = true,
        [0xc0010061] = true,
        [0xc0010063] = true,
        [0xc0010064] = true,
        [0xc0010065] = true,
        [0xc0010066] = true,
        [0xc0010067] = true,
        [0xc0010068] = true,
        [0xc0010069] = true,
        [0xc001006a] = true,
        [0xc001006b] = true,
        [0xc0010293] = true,
        [0xc0010299] = { gate = { 0x80000007, 3, 14, 1, 0x1, 0x1 } },
        [0xc001029a] = { gate = { 0x80000007, 3, 14, 1, 0x1, 0x1 } },
        [0xc001029b] = { gate = { 0x80000007, 3, 14, 1, 0x1, 0x1 } },
        [0xc00102b0] = { gate = { 0x80000008, 1, 27, 1, 0x1, 0x1 } },
        [0xc00102b1] = { gate = { 0x80000008, 1, 27, 1, 0x1, 0x1 } },
        [0xc00102b2] = { gate = { 0x80000008, 1, 27, 1, 0x1, 0x1 } },
        [0xc00102b3] = { gate = { 0x80000008, 1, 27, 1, 0x1, 0x1 } },
        [0xc00102b4] = { gate = { 0x80000008, 1, 27, 1, 0x1, 0x1 } },
        [0xc00102b5] = { gate = { 0x80000008, 1, 27, 1, 0x1, 0x1 } },
      }
    end,
  },
  {
    vendor = 'AuthenticAMD',
    family = 0x1a,
    models = { { 0xd8, 0xef } },
    msrs = function()
      return {
        [0x8b] = true,
        [0xc00000e7] = true,
        [0xc00000e8] = true,
        [0xe7] = { gate = { 0x6, 2, 0, 1, 0x1, 0x1 } },
        [0xe8] = { gate = { 0x6, 2, 0, 1, 0x1, 0x1 } },
        [0xc0010015] = true,
        [0xc0010061] = true,
        [0xc0010063] = true,
        [0xc0010064] = true,
        [0xc0010065] = true,
        [0xc0010066] = true,
        [0xc0010067] = true,
        [0xc0010068] = true,
        [0xc0010069] = true,
        [0xc001006a] = true,
        [0xc001006b] = true,
        [0xc0010293] = true,
        [0xc0010299] = { gate = { 0x80000007, 3, 14, 1, 0x1, 0x1 } },
        [0xc001029a] = { gate = { 0x80000007, 3, 14, 1, 0x1, 0x1 } },
        [0xc001029b] = { gate = { 0x80000007, 3, 14, 1, 0x1, 0x1 } },
        [0xc00102b0] = { gate = { 0x80000008, 1, 27, 1, 0x1, 0x1 } },
        [0xc00102b1] = { gate = { 0x80000008, 1, 27, 1, 0x1, 0x1 } },
        [0xc00102b2] = { gate = { 0x80000008, 1, 27, 1, 0x1, 0x1 } },
        [0xc00102b3] = { gate = { 0x80000008, 1, 27, 1, 0x1, 0x1 } },
        [0xc00102b4] = { gate = { 0x80000008, 1, 27, 1, 0x1, 0x1 } },
        [0xc00102b5] = { gate = { 0x80000008, 1, 27, 1, 0x1, 0x1 } },
      }
    end,
  },
  {
    vendor = 'AuthenticAMD',
    family = 0x10,
    models = { { 0x00, 0x0f } },
    msrs = function()
      return {
        [0xc0010063] = true,
        [0xe7] = { gate = { 0x6, 2, 0, 1, 0x1, 0x1 } },
        [0xe8] = { gate = { 0x6, 2, 0, 1, 0x1, 0x1 } },
        [0xc0010064] = true,
        [0xc0010065] = true,
        [0xc0010066] = true,
        [0xc0010067] = true,
        [0xc0010068] = true,
        [0xc0010071] = true,
      }
    end,
  },
  {
    vendor = 'AuthenticAMD',
    family = 0x11,
    models = { { 0x00, 0x0f } },
    msrs = function()
      return {
        [0xc0010063] = true,
        [0xe7] = { gate = { 0x6, 2, 0, 1, 0x1, 0x1 } },
        [0xe8] = { gate = { 0x6, 2, 0, 1, 0x1, 0x1 } },
        [0xc0010064] = true,
        [0xc0010065] = true,
        [0xc0010066] = true,
        [0xc0010067] = true,
        [0xc0010068] = true,
        [0xc0010069] = true,
        [0xc001006a] = true,
        [0xc001006b] = true,
        [0xc0010071] = true,
      }
    end,
  },
  {
    vendor = 'AuthenticAMD',
    family = 0x12,
    models = { { 0x00, 0x0f } },
    msrs = function()
      return {
        [0xc0010063] = true,
        [0xe7] = { gate = { 0x6, 2, 0, 1, 0x1, 0x1 } },
        [0xe8] = { gate = { 0x6, 2, 0, 1, 0x1, 0x1 } },
        [0xc0010064] = true,
        [0xc0010065] = true,
        [0xc0010066] = true,
        [0xc0010067] = true,
        [0xc0010068] = true,
        [0xc0010069] = true,
        [0xc001006a] = true,
        [0xc001006b] = true,
        [0xc0010071] = true,
      }
    end,
  },
  {
    vendor = 'AuthenticAMD',
    family = 0x14,
    models = { { 0x00, 0x0f } },
    msrs = function()
      return {
        [0xc0010063] = true,
        [0xe7] = { gate = { 0x6, 2, 0, 1, 0x1, 0x1 } },
        [0xe8] = { gate = { 0x6, 2, 0, 1, 0x1, 0x1 } },
        [0xc0010064] = true,
        [0xc0010065] = true,
        [0xc0010066] = true,
        [0xc0010067] = true,
        [0xc0010068] = true,
        [0xc0010069] = true,
        [0xc001006a] = true,
        [0xc001006b] = true,
        [0xc0010071] = true,
      }
    end,
  },
  {
    vendor = 'AuthenticAMD',
    family = 0x15,
    models = { { 0x00, 0x0f }, { 0x10, 0x1f }, { 0x30, 0x3f } },
    msrs = function()
      return {
        [0xc0010063] = true,
        [0xe7] = { gate = { 0x6, 2, 0, 1, 0x1, 0x1 } },
        [0xe8] = { gate = { 0x6, 2, 0, 1, 0x1, 0x1 } },
        [0xc0010064] = true,
        [0xc0010065] = true,
        [0xc0010066] = true,
        [0xc0010067] = true,
        [0xc0010068] = true,
        [0xc0010069] = true,
        [0xc001006a] = true,
        [0xc001006b] = true,
        [0xc0010071] = true,
        [0xc001007a] = { gate = { 0x80000007, 3, 12, 1, 0x1, 0x1 } },
        [0xc001007b] = { gate = { 0x80000007, 3, 12, 1, 0x1, 0x1 } },
        [0xc0010280] = { gate = { 0x80000007, 3, 12, 1, 0x1, 0x1 } },
      }
    end,
  },
  {
    vendor = 'AuthenticAMD',
    family = 0x16,
    models = { { 0x00, 0x0f }, { 0x30, 0x3f } },
    msrs = function()
      return {
        [0xc0010063] = true,
        [0xe7] = { gate = { 0x6, 2, 0, 1, 0x1, 0x1 } },
        [0xe8] = { gate = { 0x6, 2, 0, 1, 0x1, 0x1 } },
        [0xc0010064] = true,
        [0xc0010065] = true,
        [0xc0010066] = true,
        [0xc0010067] = true,
        [0xc0010068] = true,
        [0xc0010069] = true,
        [0xc001006a] = true,
        [0xc001006b] = true,
        [0xc0010071] = true,
        [0xc001007a] = { gate = { 0x80000007, 3, 12, 1, 0x1, 0x1 } },
        [0xc001007b] = { gate = { 0x80000007, 3, 12, 1, 0x1, 0x1 } },
        [0xc0010280] = { gate = { 0x80000007, 3, 12, 1, 0x1, 0x1 } },
      }
    end,
  },
  {
    vendor = 'AuthenticAMD',
    family = 0x15,
    models = { { 0x60, 0x6f }, { 0x70, 0x7f } },
    msrs = function()
      return {
        [0xc0010063] = true,
        [0xe7] = { gate = { 0x6, 2, 0, 1, 0x1, 0x1 } },
        [0xe8] = { gate = { 0x6, 2, 0, 1, 0x1, 0x1 } },
        [0xc0010064] = true,
        [0xc0010065] = true,
        [0xc0010066] = true,
        [0xc0010067] = true,
        [0xc0010068] = true,
        [0xc0010069] = true,
        [0xc001006a] = true,
        [0xc001006b] = true,
        [0xc0010071] = true,
        [0xc001007a] = { gate = { 0x80000007, 3, 12, 1, 0x1, 0x1 } },
        [0xc001007b] = { gate = { 0x80000007, 3, 12, 1, 0x1, 0x1 } },
        [0xc0010280] = { gate = { 0x80000007, 3, 12, 1, 0x1, 0x1 } },
      }
    end,
  },
}

local GATE_LEAVES = { 0x6, 0x1a, 0x80000007, 0x80000008 }

local kept = {}
local records = {}
local permitted = {}

local function find_row(cpu)
  for _, row in ipairs(ROWS) do
    if row.vendor == cpu.vendor and row.family == cpu.family then
      for _, range in ipairs(row.models) do
        if cpu.model >= range[1] and cpu.model <= range[2] then
          return row
        end
      end
    end
  end
  return nil
end

local function query(processors, gates)
  local items = {}
  for _, p in ipairs(processors) do
    items[#items + 1] = string.pack('<I2I2I4I4', p.group, p.number, 0, 0)
    items[#items + 1] = string.pack('<I2I2I4I4', p.group, p.number, 1, 0)
    if gates then
      items[#items + 1] = string.pack('<I2I2I4I4', p.group, p.number, 0x80000000, 0)
      for _, leaf in ipairs(GATE_LEAVES) do
        items[#items + 1] = string.pack('<I2I2I4I4', p.group, p.number, leaf, 0)
      end
    else
      items[#items + 1] = string.pack('<I2I2I4I4', p.group, p.number, 0x7, 0)
    end
  end

  local results, status = ksi.hw.cpuid(table.concat(items))
  if not results then
    ksi.trace('cpuid failed: 0x%x', status)
    return status
  end

  local at = 1
  for _, p in ipairs(processors) do
    local start = at
    local function take()
      local s, a, b, c, d = string.unpack('<i4I4I4I4I4', results, at)
      at = at + 20
      if not nt.NT_SUCCESS(s) then
        p.invalid = true
      end
      return { a, b, c, d }
    end

    local basic = take()
    local version = take()
    local signature = version[1]
    local base = (signature >> 8) & 0xf
    local family = base
    if base == 0xf then
      family = family + ((signature >> 20) & 0xff)
    end
    local model = (signature >> 4) & 0xf
    if base >= 0x6 then
      model = model | (((signature >> 16) & 0xf) << 4)
    end

    p.vendor = string.pack('<I4I4I4', basic[2], basic[4], basic[3])
    p.family = family
    p.model = model
    p.stepping = signature & 0xf
    p.signature = signature
    p.basic = basic[1]

    if gates then
      p.extended = take()[1]
      p.leaves = {}
      for _, leaf in ipairs(GATE_LEAVES) do
        p.leaves[leaf] = take()
      end
      p.bytes = string.sub(results, start, start + 27) .. string.sub(results, start + 40, at - 1)
    else
      local structured = take()
      p.hybrid = 0
      if p.basic >= 0x7 then
        p.hybrid = (structured[4] >> 15) & 1
      end
    end
  end

  return nt.STATUS_SUCCESS
end

local function gate_holds(p, gate)
  local leaf, register, shift, width, low, high = table.unpack(gate)
  local maximum = (leaf >= 0x80000000) and p.extended or p.basic
  if leaf > maximum then
    return false
  end
  local registers = p.leaves[leaf]
  if not registers then
    ksi.trace('CPUID leaf 0x%x is not read for gates', leaf)
    return false
  end
  local value = (registers[register + 1] >> shift) & ((1 << width) - 1)
  return value >= low and value <= high
end

-- Returns the identity of the processor the input names.
function identify(input)
  if not ksi.hw.cpuid then
    ksi.trace('no CPUID function')
    if #input == 4 then
      local group, number = string.unpack('<I2I2', input)
      kept[(group << 16) | number] = nil
    end
    return nt.STATUS_NOT_SUPPORTED
  end
  if #input ~= 4 then
    ksi.trace('input of %d bytes', #input)
    return nt.STATUS_INVALID_PARAMETER
  end

  local group, number = string.unpack('<I2I2', input)
  kept[(group << 16) | number] = nil
  local processor = { group = group, number = number }
  local status = query({ processor }, false)
  if not nt.NT_SUCCESS(status) then
    return status
  end
  if processor.invalid then
    ksi.trace('processor %d:%d is not active', group, number)
    return nt.STATUS_INVALID_PARAMETER
  end

  return nt.STATUS_SUCCESS,
    string.pack(
      '<c12I4I4I4I4I4',
      processor.vendor,
      processor.family,
      processor.model,
      processor.stepping,
      processor.signature,
      processor.hybrid
    )
end

local function read_msr(input)
  if #input % 8 ~= 0 then
    ksi.trace('input of %d bytes', #input)
    return nt.STATUS_INVALID_PARAMETER
  end

  local items = {}
  local processors = {}
  local unknown = {}
  local byKey = {}

  for at = 1, #input, 8 do
    local group, number, index = string.unpack('<I2I2I4', input, at)
    local key = (group << 16) | number
    local p = byKey[key]
    if not p then
      p = { group = group, number = number, record = kept[key] }
      if not p.record then
        unknown[#unknown + 1] = p
      end
      byKey[key] = p
      processors[#processors + 1] = p
    end
    items[#items + 1] = { processor = p, index = index }
  end

  if #items == 0 then
    return nt.STATUS_SUCCESS, ''
  end

  if #unknown > 0 then
    local status = query(unknown, true)
    if not nt.NT_SUCCESS(status) then
      return status
    end
  end

  for _, p in ipairs(unknown) do
    if p.invalid then
      ksi.trace('processor %d:%d is not active', p.group, p.number)
    else
      local record = records[p.bytes]
      if not record then
        record = p
        record.row = find_row(p)
        if record.row then
          if not permitted[record.row] then
            permitted[record.row] = record.row.msrs()
          end
          record.permitted = permitted[record.row]
        else
          record.permitted = {}
        end
        record.gates = {}
        records[p.bytes] = record
      end
      kept[(p.group << 16) | p.number] = record
      p.record = record
    end
  end
  for _, p in ipairs(processors) do
    local record = p.record
    if record and not record.row then
      ksi.trace(
        'no registers are listed for %s family 0x%x ' .. 'model 0x%x',
        record.vendor,
        record.family,
        record.model
      )
    end
  end

  local reads = {}
  for _, item in ipairs(items) do
    local p = item.processor
    local record = p.record
    local index = item.index
    item.status = nt.STATUS_SUCCESS
    item.value = 0

    if not record then
      item.status = nt.STATUS_INVALID_PARAMETER
    else
      local entry = record.permitted[index]
      if not entry then
        if record.row then
          ksi.trace('MSR 0x%x is not permitted', index)
        end
        item.status = nt.STATUS_NOT_FOUND
      else
        local gate = type(entry) == 'table' and entry.gate
        if gate then
          if record.gates[gate] == nil then
            record.gates[gate] = gate_holds(record, gate)
          end
          if not record.gates[gate] then
            ksi.trace('MSR 0x%x is gated off on processor %d:%d', index, p.group, p.number)
            item.status = nt.STATUS_NOT_FOUND
          end
        end
        if nt.NT_SUCCESS(item.status) then
          item.entry = entry
          reads[#reads + 1] = item
        end
      end
    end
  end

  if #reads > 0 then
    local batch = input
    if #reads < #items then
      local packed = {}
      for i, item in ipairs(reads) do
        packed[i] = string.pack('<I2I2I4', item.processor.group, item.processor.number, item.index)
      end
      batch = table.concat(packed)
    end
    local results, failure = ksi.hw.msr_read(batch)
    if not results then
      ksi.trace('msr_read failed: 0x%x', failure)
      return failure
    end
    for i, item in ipairs(reads) do
      local s, _, value = string.unpack('<i4I4I8', results, (i - 1) * 16 + 1)
      if not nt.NT_SUCCESS(s) then
        item.status = s
      elseif type(item.entry) == 'table' and item.entry.mask then
        item.value = value & item.entry.mask
      else
        item.value = value
      end
    end
  end

  local output = {}
  for i, item in ipairs(items) do
    output[i] = string.pack('<i4I4I8', item.status, 0, item.value)
  end

  return nt.STATUS_SUCCESS, table.concat(output)
end

-- Reads a list of registers, each on the processor its item names.
function read(input)
  if ksi.hw.msr_read then
    return read_msr(input)
  end
  ksi.trace('no register read function')
  return nt.STATUS_NOT_SUPPORTED
end
