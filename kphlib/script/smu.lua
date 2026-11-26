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
-- Talks to the AMD firmware mailbox behind the root complex's system
-- management network pair. The root complexes, their mailbox registers and
-- the commands each processor family may be sent are listed below.
--

local nt = require('nt')

local concat, pack, unpack = table.concat, table.pack, table.unpack

local ROWS = {
  {
    vendor = 'AuthenticAMD',
    family = 0x17,
    models = { { 0x10, 0x1f }, { 0x20, 0x2f } },
    pci = { 0x1022, 0x15d0 },
    pin = { 0, 0 },
    mailbox = function()
      return {
        index = 0x60,
        data = 0x64,
        msg = 0x03b10a20,
        rsp = 0x03b10a80,
        arg = 0x03b10a88,
        args = 6,
        version = 0x02,
        version_arg = 0x1,
        commands = {
          [0x02] = {
            words = 1,
            internal = false,
            mask = { 0x0, 0x0, 0x0, 0x0, 0x0, 0x0 },
            max = { 0x0, 0x0, 0x0, 0x0, 0x0, 0x0 },
            fixed = { 0x1, 0x0, 0x0, 0x0, 0x0, 0x0 },
          },
          [0x0c] = {
            words = 1,
            internal = false,
            mask = { 0x0, 0x0, 0x0, 0x0, 0x0, 0x0 },
            max = { 0x0, 0x0, 0x0, 0x0, 0x0, 0x0 },
            fixed = { 0x0, 0x0, 0x0, 0x0, 0x0, 0x0 },
          },
          [0x3d] = {
            words = 0,
            internal = true,
            mask = { 0x0, 0x0, 0x0, 0x0, 0x0, 0x0 },
            max = { 0x0, 0x0, 0x0, 0x0, 0x0, 0x0 },
            fixed = { 0x3, 0x0, 0x0, 0x0, 0x0, 0x0 },
          },
          [0x0a] = {
            words = 0,
            internal = true,
            mask = { 0x0, 0x0, 0x0, 0x0, 0x0, 0x0 },
            max = { 0x0, 0x0, 0x0, 0x0, 0x0, 0x0 },
            fixed = { 0x3, 0x0, 0x0, 0x0, 0x0, 0x0 },
          },
          [0x0b] = {
            words = 1,
            internal = true,
            mask = { 0x0, 0x0, 0x0, 0x0, 0x0, 0x0 },
            max = { 0x0, 0x0, 0x0, 0x0, 0x0, 0x0 },
            fixed = { 0x3, 0x0, 0x0, 0x0, 0x0, 0x0 },
          },
        },
        table = {
          bound = 'amd-dram',
          version = 0x0c,
          transfer = 0x3d,
          setup = 0x0a,
          base = 0x0b,
          base_low = 0,
          base_high = 255,
          size_word = 255,
          versions = {
            [0x1e0004] = 0x608,
          },
        },
      }
    end,
  },
  {
    vendor = 'AuthenticAMD',
    family = 0x17,
    models = { { 0x30, 0x3f } },
    pci = { 0x1022, 0x1480 },
    pin = { 0, 0 },
    mailbox = function()
      return {
        index = 0x60,
        data = 0x64,
        msg = 0x03b10524,
        rsp = 0x03b10570,
        arg = 0x03b10a40,
        args = 6,
        version = 0x02,
        version_arg = 0x1,
        commands = {
          [0x02] = {
            words = 1,
            internal = false,
            mask = { 0x0, 0x0, 0x0, 0x0, 0x0, 0x0 },
            max = { 0x0, 0x0, 0x0, 0x0, 0x0, 0x0 },
            fixed = { 0x1, 0x0, 0x0, 0x0, 0x0, 0x0 },
          },
          [0x08] = {
            words = 1,
            internal = false,
            mask = { 0x0, 0x0, 0x0, 0x0, 0x0, 0x0 },
            max = { 0x0, 0x0, 0x0, 0x0, 0x0, 0x0 },
            fixed = { 0x0, 0x0, 0x0, 0x0, 0x0, 0x0 },
          },
          [0x05] = {
            words = 0,
            internal = true,
            mask = { 0x0, 0x0, 0x0, 0x0, 0x0, 0x0 },
            max = { 0x0, 0x0, 0x0, 0x0, 0x0, 0x0 },
            fixed = { 0x0, 0x0, 0x0, 0x0, 0x0, 0x0 },
          },
          [0x06] = {
            words = 2,
            internal = true,
            mask = { 0x0, 0x0, 0x0, 0x0, 0x0, 0x0 },
            max = { 0x0, 0x0, 0x0, 0x0, 0x0, 0x0 },
            fixed = { 0x1, 0x1, 0x0, 0x0, 0x0, 0x0 },
          },
        },
        table = {
          bound = 'amd-dram',
          version = 0x08,
          transfer = 0x05,
          setup = 0x00,
          base = 0x06,
          base_low = 0,
          base_high = 1,
          size_word = 255,
          versions = {
            [0x240003] = 0x18ac,
            [0x240503] = 0xd7c,
            [0x240603] = 0xab0,
            [0x240703] = 0x7e4,
          },
        },
      }
    end,
  },
  {
    vendor = 'AuthenticAMD',
    family = 0x17,
    models = { { 0x60, 0x6f } },
    pci = { 0x1022, 0x1630 },
    pin = { 0, 0 },
    mailbox = function()
      return {
        index = 0x60,
        data = 0x64,
        msg = 0x03b10a20,
        rsp = 0x03b10a80,
        arg = 0x03b10a88,
        args = 6,
        version = 0x02,
        version_arg = 0x1,
        commands = {
          [0x02] = {
            words = 1,
            internal = false,
            mask = { 0x0, 0x0, 0x0, 0x0, 0x0, 0x0 },
            max = { 0x0, 0x0, 0x0, 0x0, 0x0, 0x0 },
            fixed = { 0x1, 0x0, 0x0, 0x0, 0x0, 0x0 },
          },
          [0x06] = {
            words = 1,
            internal = false,
            mask = { 0x0, 0x0, 0x0, 0x0, 0x0, 0x0 },
            max = { 0x0, 0x0, 0x0, 0x0, 0x0, 0x0 },
            fixed = { 0x0, 0x0, 0x0, 0x0, 0x0, 0x0 },
          },
          [0x65] = {
            words = 0,
            internal = true,
            mask = { 0x0, 0x0, 0x0, 0x0, 0x0, 0x0 },
            max = { 0x0, 0x0, 0x0, 0x0, 0x0, 0x0 },
            fixed = { 0x3, 0x0, 0x0, 0x0, 0x0, 0x0 },
          },
          [0x66] = {
            words = 2,
            internal = true,
            mask = { 0x0, 0x0, 0x0, 0x0, 0x0, 0x0 },
            max = { 0x0, 0x0, 0x0, 0x0, 0x0, 0x0 },
            fixed = { 0x1, 0x1, 0x0, 0x0, 0x0, 0x0 },
          },
        },
        table = {
          bound = 'amd-dram',
          version = 0x06,
          transfer = 0x65,
          setup = 0x00,
          base = 0x66,
          base_low = 0,
          base_high = 1,
          size_word = 255,
          versions = {
            [0x370000] = 0x794,
            [0x370001] = 0x884,
            [0x370002] = 0x88c,
            [0x370003] = 0x8ac,
            [0x370004] = 0x8ac,
            [0x370005] = 0x8c8,
          },
        },
      }
    end,
  },
  {
    vendor = 'AuthenticAMD',
    family = 0x17,
    models = { { 0x70, 0x7f } },
    pci = { 0x1022, 0x1480 },
    pin = { 0, 0 },
    mailbox = function()
      return {
        index = 0x60,
        data = 0x64,
        msg = 0x03b10524,
        rsp = 0x03b10570,
        arg = 0x03b10a40,
        args = 6,
        version = 0x02,
        version_arg = 0x1,
        commands = {
          [0x02] = {
            words = 1,
            internal = false,
            mask = { 0x0, 0x0, 0x0, 0x0, 0x0, 0x0 },
            max = { 0x0, 0x0, 0x0, 0x0, 0x0, 0x0 },
            fixed = { 0x1, 0x0, 0x0, 0x0, 0x0, 0x0 },
          },
          [0x08] = {
            words = 1,
            internal = false,
            mask = { 0x0, 0x0, 0x0, 0x0, 0x0, 0x0 },
            max = { 0x0, 0x0, 0x0, 0x0, 0x0, 0x0 },
            fixed = { 0x0, 0x0, 0x0, 0x0, 0x0, 0x0 },
          },
          [0x05] = {
            words = 0,
            internal = true,
            mask = { 0x0, 0x0, 0x0, 0x0, 0x0, 0x0 },
            max = { 0x0, 0x0, 0x0, 0x0, 0x0, 0x0 },
            fixed = { 0x0, 0x0, 0x0, 0x0, 0x0, 0x0 },
          },
          [0x06] = {
            words = 2,
            internal = true,
            mask = { 0x0, 0x0, 0x0, 0x0, 0x0, 0x0 },
            max = { 0x0, 0x0, 0x0, 0x0, 0x0, 0x0 },
            fixed = { 0x1, 0x1, 0x0, 0x0, 0x0, 0x0 },
          },
        },
        table = {
          bound = 'amd-dram',
          version = 0x08,
          transfer = 0x05,
          setup = 0x00,
          base = 0x06,
          base_low = 0,
          base_high = 1,
          size_word = 255,
          versions = {
            [0x240802] = 0x7e0,
            [0x240803] = 0x7e4,
            [0x240902] = 0x514,
            [0x240903] = 0x518,
          },
        },
      }
    end,
  },
  {
    vendor = 'AuthenticAMD',
    family = 0x19,
    models = { { 0x00, 0x0f } },
    pci = { 0x1022, 0x1480 },
    pin = { 0, 0 },
    mailbox = function()
      return {
        index = 0x60,
        data = 0x64,
        msg = 0x03b10524,
        rsp = 0x03b10570,
        arg = 0x03b10a40,
        args = 6,
        version = 0x02,
        version_arg = 0x1,
        commands = {
          [0x02] = {
            words = 1,
            internal = false,
            mask = { 0x0, 0x0, 0x0, 0x0, 0x0, 0x0 },
            max = { 0x0, 0x0, 0x0, 0x0, 0x0, 0x0 },
            fixed = { 0x1, 0x0, 0x0, 0x0, 0x0, 0x0 },
          },
          [0x08] = {
            words = 1,
            internal = false,
            mask = { 0x0, 0x0, 0x0, 0x0, 0x0, 0x0 },
            max = { 0x0, 0x0, 0x0, 0x0, 0x0, 0x0 },
            fixed = { 0x0, 0x0, 0x0, 0x0, 0x0, 0x0 },
          },
          [0x05] = {
            words = 0,
            internal = true,
            mask = { 0x0, 0x0, 0x0, 0x0, 0x0, 0x0 },
            max = { 0x0, 0x0, 0x0, 0x0, 0x0, 0x0 },
            fixed = { 0x0, 0x0, 0x0, 0x0, 0x0, 0x0 },
          },
          [0x06] = {
            words = 2,
            internal = true,
            mask = { 0x0, 0x0, 0x0, 0x0, 0x0, 0x0 },
            max = { 0x0, 0x0, 0x0, 0x0, 0x0, 0x0 },
            fixed = { 0x1, 0x1, 0x0, 0x0, 0x0, 0x0 },
          },
        },
        table = {
          bound = 'amd-dram',
          version = 0x08,
          transfer = 0x05,
          setup = 0x00,
          base = 0x06,
          base_low = 0,
          base_high = 1,
          size_word = 255,
          versions = {
            [0x2d0008] = 0x1ab0,
            [0x380005] = 0x1bb0,
            [0x380505] = 0xf30,
            [0x380605] = 0xc10,
            [0x380705] = 0x8f0,
          },
        },
      }
    end,
  },
  {
    vendor = 'AuthenticAMD',
    family = 0x19,
    models = { { 0x18, 0x18 } },
    pci = { 0x1022, 0x14a4 },
    pin = { 0, 0 },
    mailbox = function()
      return {
        index = 0x60,
        data = 0x64,
        msg = 0x03b10524,
        rsp = 0x03b10570,
        arg = 0x03b10a40,
        args = 6,
        version = 0x02,
        version_arg = 0x1,
        commands = {
          [0x02] = {
            words = 1,
            internal = false,
            mask = { 0x0, 0x0, 0x0, 0x0, 0x0, 0x0 },
            max = { 0x0, 0x0, 0x0, 0x0, 0x0, 0x0 },
            fixed = { 0x1, 0x0, 0x0, 0x0, 0x0, 0x0 },
          },
          [0x05] = {
            words = 1,
            internal = false,
            mask = { 0x0, 0x0, 0x0, 0x0, 0x0, 0x0 },
            max = { 0x0, 0x0, 0x0, 0x0, 0x0, 0x0 },
            fixed = { 0x0, 0x0, 0x0, 0x0, 0x0, 0x0 },
          },
          [0x03] = {
            words = 0,
            internal = true,
            mask = { 0x0, 0x0, 0x0, 0x0, 0x0, 0x0 },
            max = { 0x0, 0x0, 0x0, 0x0, 0x0, 0x0 },
            fixed = { 0x0, 0x0, 0x0, 0x0, 0x0, 0x0 },
          },
          [0x04] = {
            words = 2,
            internal = true,
            mask = { 0x0, 0x0, 0x0, 0x0, 0x0, 0x0 },
            max = { 0x0, 0x0, 0x0, 0x0, 0x0, 0x0 },
            fixed = { 0x1, 0x1, 0x0, 0x0, 0x0, 0x0 },
          },
        },
        table = {
          bound = 'amd-dram',
          version = 0x05,
          transfer = 0x03,
          setup = 0x00,
          base = 0x04,
          base_low = 0,
          base_high = 1,
          size_word = 255,
          versions = {
            [0x5c0002] = 0x1e3c,
            [0x5c0003] = 0x1e48,
            [0x5c0102] = 0x1a14,
            [0x5c0103] = 0x1a20,
            [0x5c0202] = 0x15ec,
            [0x5c0203] = 0x15f8,
            [0x5c0302] = 0xd9c,
            [0x5c0303] = 0xda8,
            [0x5c0402] = 0x974,
            [0x5c0403] = 0x980,
          },
          dfmap = {
            device = 0x18,
            fn = 0x7,
            offset = 0xe00,
            count = 0x10,
            stride = 0x10,
            limit = 0x4,
            control = 0x8,
            shift = 0x1c,
            mask = 0xfffffff,
            valid = 0x0,
            remap = 0x1,
            hole = 0x104,
            hole_mask = 0xff000000,
            hole_valid = 0x0,
          },
        },
      }
    end,
  },
  {
    vendor = 'AuthenticAMD',
    family = 0x19,
    models = { { 0x20, 0x2f } },
    pci = { 0x1022, 0x1480 },
    pin = { 0, 0 },
    mailbox = function()
      return {
        index = 0x60,
        data = 0x64,
        msg = 0x03b10524,
        rsp = 0x03b10570,
        arg = 0x03b10a40,
        args = 6,
        version = 0x02,
        version_arg = 0x1,
        commands = {
          [0x02] = {
            words = 1,
            internal = false,
            mask = { 0x0, 0x0, 0x0, 0x0, 0x0, 0x0 },
            max = { 0x0, 0x0, 0x0, 0x0, 0x0, 0x0 },
            fixed = { 0x1, 0x0, 0x0, 0x0, 0x0, 0x0 },
          },
          [0x08] = {
            words = 1,
            internal = false,
            mask = { 0x0, 0x0, 0x0, 0x0, 0x0, 0x0 },
            max = { 0x0, 0x0, 0x0, 0x0, 0x0, 0x0 },
            fixed = { 0x0, 0x0, 0x0, 0x0, 0x0, 0x0 },
          },
          [0x05] = {
            words = 0,
            internal = true,
            mask = { 0x0, 0x0, 0x0, 0x0, 0x0, 0x0 },
            max = { 0x0, 0x0, 0x0, 0x0, 0x0, 0x0 },
            fixed = { 0x0, 0x0, 0x0, 0x0, 0x0, 0x0 },
          },
          [0x06] = {
            words = 2,
            internal = true,
            mask = { 0x0, 0x0, 0x0, 0x0, 0x0, 0x0 },
            max = { 0x0, 0x0, 0x0, 0x0, 0x0, 0x0 },
            fixed = { 0x1, 0x1, 0x0, 0x0, 0x0, 0x0 },
          },
        },
        table = {
          bound = 'amd-dram',
          version = 0x08,
          transfer = 0x05,
          setup = 0x00,
          base = 0x06,
          base_low = 0,
          base_high = 1,
          size_word = 255,
          versions = {
            [0x2d0803] = 0x894,
            [0x2d0903] = 0x594,
            [0x380804] = 0x8a4,
            [0x380805] = 0x8f0,
            [0x380904] = 0x5a4,
            [0x380905] = 0x5d0,
          },
        },
      }
    end,
  },
  {
    vendor = 'AuthenticAMD',
    family = 0x19,
    models = { { 0x40, 0x4f } },
    pci = { 0x1022, 0x14b5 },
    pin = { 0, 0 },
    mailbox = function()
      return {
        index = 0x60,
        data = 0x64,
        msg = 0x03b10a20,
        rsp = 0x03b10a80,
        arg = 0x03b10a88,
        args = 6,
        version = 0x02,
        version_arg = 0x1,
        commands = {
          [0x02] = {
            words = 1,
            internal = false,
            mask = { 0x0, 0x0, 0x0, 0x0, 0x0, 0x0 },
            max = { 0x0, 0x0, 0x0, 0x0, 0x0, 0x0 },
            fixed = { 0x1, 0x0, 0x0, 0x0, 0x0, 0x0 },
          },
          [0x06] = {
            words = 1,
            internal = false,
            mask = { 0x0, 0x0, 0x0, 0x0, 0x0, 0x0 },
            max = { 0x0, 0x0, 0x0, 0x0, 0x0, 0x0 },
            fixed = { 0x0, 0x0, 0x0, 0x0, 0x0, 0x0 },
          },
          [0x65] = {
            words = 0,
            internal = true,
            mask = { 0x0, 0x0, 0x0, 0x0, 0x0, 0x0 },
            max = { 0x0, 0x0, 0x0, 0x0, 0x0, 0x0 },
            fixed = { 0x3, 0x0, 0x0, 0x0, 0x0, 0x0 },
          },
          [0x66] = {
            words = 2,
            internal = true,
            mask = { 0x0, 0x0, 0x0, 0x0, 0x0, 0x0 },
            max = { 0x0, 0x0, 0x0, 0x0, 0x0, 0x0 },
            fixed = { 0x1, 0x1, 0x0, 0x0, 0x0, 0x0 },
          },
        },
        table = {
          bound = 'amd-dram',
          version = 0x06,
          transfer = 0x65,
          setup = 0x00,
          base = 0x66,
          base_low = 0,
          base_high = 1,
          size_word = 255,
          versions = {
            [0x450004] = 0xaa4,
            [0x450005] = 0xab0,
          },
        },
      }
    end,
  },
  {
    vendor = 'AuthenticAMD',
    family = 0x19,
    models = { { 0x50, 0x5f } },
    pci = { 0x1022, 0x1630 },
    pin = { 0, 0 },
    mailbox = function()
      return {
        index = 0x60,
        data = 0x64,
        msg = 0x03b10a20,
        rsp = 0x03b10a80,
        arg = 0x03b10a88,
        args = 6,
        version = 0x02,
        version_arg = 0x1,
        commands = {
          [0x02] = {
            words = 1,
            internal = false,
            mask = { 0x0, 0x0, 0x0, 0x0, 0x0, 0x0 },
            max = { 0x0, 0x0, 0x0, 0x0, 0x0, 0x0 },
            fixed = { 0x1, 0x0, 0x0, 0x0, 0x0, 0x0 },
          },
          [0x06] = {
            words = 1,
            internal = false,
            mask = { 0x0, 0x0, 0x0, 0x0, 0x0, 0x0 },
            max = { 0x0, 0x0, 0x0, 0x0, 0x0, 0x0 },
            fixed = { 0x0, 0x0, 0x0, 0x0, 0x0, 0x0 },
          },
          [0x65] = {
            words = 0,
            internal = true,
            mask = { 0x0, 0x0, 0x0, 0x0, 0x0, 0x0 },
            max = { 0x0, 0x0, 0x0, 0x0, 0x0, 0x0 },
            fixed = { 0x0, 0x0, 0x0, 0x0, 0x0, 0x0 },
          },
          [0x66] = {
            words = 2,
            internal = true,
            mask = { 0x0, 0x0, 0x0, 0x0, 0x0, 0x0 },
            max = { 0x0, 0x0, 0x0, 0x0, 0x0, 0x0 },
            fixed = { 0x1, 0x1, 0x0, 0x0, 0x0, 0x0 },
          },
        },
        table = {
          bound = 'amd-dram',
          version = 0x06,
          transfer = 0x65,
          setup = 0x00,
          base = 0x66,
          base_low = 0,
          base_high = 1,
          size_word = 255,
          versions = {
            [0x400001] = 0x910,
            [0x400002] = 0x928,
            [0x400003] = 0x94c,
            [0x400004] = 0x944,
            [0x400005] = 0x944,
          },
        },
      }
    end,
  },
  {
    vendor = 'AuthenticAMD',
    family = 0x19,
    models = { { 0x60, 0x6f } },
    pci = { 0x1022, 0x14d8 },
    pin = { 0, 0 },
    mailbox = function()
      return {
        index = 0x60,
        data = 0x64,
        msg = 0x03b10524,
        rsp = 0x03b10570,
        arg = 0x03b10a40,
        args = 6,
        version = 0x02,
        version_arg = 0x1,
        commands = {
          [0x02] = {
            words = 1,
            internal = false,
            mask = { 0x0, 0x0, 0x0, 0x0, 0x0, 0x0 },
            max = { 0x0, 0x0, 0x0, 0x0, 0x0, 0x0 },
            fixed = { 0x1, 0x0, 0x0, 0x0, 0x0, 0x0 },
          },
          [0x05] = {
            words = 1,
            internal = false,
            mask = { 0x0, 0x0, 0x0, 0x0, 0x0, 0x0 },
            max = { 0x0, 0x0, 0x0, 0x0, 0x0, 0x0 },
            fixed = { 0x0, 0x0, 0x0, 0x0, 0x0, 0x0 },
          },
          [0x03] = {
            words = 0,
            internal = true,
            mask = { 0x0, 0x0, 0x0, 0x0, 0x0, 0x0 },
            max = { 0x0, 0x0, 0x0, 0x0, 0x0, 0x0 },
            fixed = { 0x0, 0x0, 0x0, 0x0, 0x0, 0x0 },
          },
          [0x04] = {
            words = 2,
            internal = true,
            mask = { 0x0, 0x0, 0x0, 0x0, 0x0, 0x0 },
            max = { 0x0, 0x0, 0x0, 0x0, 0x0, 0x0 },
            fixed = { 0x1, 0x1, 0x0, 0x0, 0x0, 0x0 },
          },
        },
        table = {
          bound = 'amd-dram',
          version = 0x05,
          transfer = 0x03,
          setup = 0x00,
          base = 0x04,
          base_low = 0,
          base_high = 1,
          size_word = 255,
          versions = {
            [0x540000] = 0x828,
            [0x540001] = 0x82c,
            [0x540002] = 0x87c,
            [0x540003] = 0x89c,
            [0x540004] = 0x8bc,
            [0x540005] = 0x8c8,
            [0x540100] = 0x618,
            [0x540101] = 0x61c,
            [0x540102] = 0x66c,
            [0x540103] = 0x68c,
            [0x540104] = 0x6a8,
            [0x540105] = 0x6b4,
            [0x540108] = 0x6bc,
            [0x540208] = 0x8d0,
          },
          dfmap = {
            device = 0x18,
            fn = 0x7,
            offset = 0xe00,
            count = 0x10,
            stride = 0x10,
            limit = 0x4,
            control = 0x8,
            shift = 0x1c,
            mask = 0xfffffff,
            valid = 0x0,
            remap = 0x1,
            hole = 0x104,
            hole_mask = 0xff000000,
            hole_valid = 0x0,
          },
        },
      }
    end,
  },
  {
    vendor = 'AuthenticAMD',
    family = 0x19,
    models = { { 0x70, 0x7f } },
    pci = { 0x1022, 0x14e8 },
    pin = { 0, 0 },
    mailbox = function()
      return {
        index = 0x60,
        data = 0x64,
        msg = 0x03b10a20,
        rsp = 0x03b10a80,
        arg = 0x03b10a88,
        args = 6,
        version = 0x02,
        version_arg = 0x1,
        commands = {
          [0x02] = {
            words = 1,
            internal = false,
            mask = { 0x0, 0x0, 0x0, 0x0, 0x0, 0x0 },
            max = { 0x0, 0x0, 0x0, 0x0, 0x0, 0x0 },
            fixed = { 0x1, 0x0, 0x0, 0x0, 0x0, 0x0 },
          },
          [0x06] = {
            words = 1,
            internal = false,
            mask = { 0x0, 0x0, 0x0, 0x0, 0x0, 0x0 },
            max = { 0x0, 0x0, 0x0, 0x0, 0x0, 0x0 },
            fixed = { 0x0, 0x0, 0x0, 0x0, 0x0, 0x0 },
          },
          [0x65] = {
            words = 0,
            internal = true,
            mask = { 0x0, 0x0, 0x0, 0x0, 0x0, 0x0 },
            max = { 0x0, 0x0, 0x0, 0x0, 0x0, 0x0 },
            fixed = { 0x3, 0x0, 0x0, 0x0, 0x0, 0x0 },
          },
          [0x66] = {
            words = 2,
            internal = true,
            mask = { 0x0, 0x0, 0x0, 0x0, 0x0, 0x0 },
            max = { 0x0, 0x0, 0x0, 0x0, 0x0, 0x0 },
            fixed = { 0x1, 0x1, 0x0, 0x0, 0x0, 0x0 },
          },
        },
        table = {
          bound = 'amd-dram',
          version = 0x06,
          transfer = 0x65,
          setup = 0x00,
          base = 0x66,
          base_low = 0,
          base_high = 1,
          size_word = 255,
          versions = {
            [0x4c0003] = 0xb18,
            [0x4c0004] = 0xb1c,
            [0x4c0005] = 0xaf8,
            [0x4c0006] = 0xafc,
            [0x4c0007] = 0xb00,
            [0x4c0008] = 0xaf0,
            [0x4c0009] = 0xb00,
          },
          dfmap = {
            device = 0x18,
            fn = 0x7,
            offset = 0xe00,
            count = 0x10,
            stride = 0x10,
            limit = 0x4,
            control = 0x8,
            shift = 0x1c,
            mask = 0xfffffff,
            valid = 0x0,
            remap = 0x1,
            hole = 0x104,
            hole_mask = 0xff000000,
            hole_valid = 0x0,
          },
        },
      }
    end,
  },
  {
    vendor = 'AuthenticAMD',
    family = 0x1a,
    models = { { 0x08, 0x08 } },
    pci = { 0x1022, 0x153a },
    pin = { 0, 0 },
    mailbox = function()
      return {
        index = 0x60,
        data = 0x64,
        msg = 0x03b10924,
        rsp = 0x03b10970,
        arg = 0x03b10a40,
        args = 6,
        version = 0x02,
        version_arg = 0x1,
        commands = {
          [0x02] = {
            words = 1,
            internal = false,
            mask = { 0x0, 0x0, 0x0, 0x0, 0x0, 0x0 },
            max = { 0x0, 0x0, 0x0, 0x0, 0x0, 0x0 },
            fixed = { 0x1, 0x0, 0x0, 0x0, 0x0, 0x0 },
          },
          [0x05] = {
            words = 1,
            internal = false,
            mask = { 0x0, 0x0, 0x0, 0x0, 0x0, 0x0 },
            max = { 0x0, 0x0, 0x0, 0x0, 0x0, 0x0 },
            fixed = { 0x0, 0x0, 0x0, 0x0, 0x0, 0x0 },
          },
          [0x03] = {
            words = 0,
            internal = true,
            mask = { 0x0, 0x0, 0x0, 0x0, 0x0, 0x0 },
            max = { 0x0, 0x0, 0x0, 0x0, 0x0, 0x0 },
            fixed = { 0x0, 0x0, 0x0, 0x0, 0x0, 0x0 },
          },
          [0x04] = {
            words = 2,
            internal = true,
            mask = { 0x0, 0x0, 0x0, 0x0, 0x0, 0x0 },
            max = { 0x0, 0x0, 0x0, 0x0, 0x0, 0x0 },
            fixed = { 0x1, 0x1, 0x0, 0x0, 0x0, 0x0 },
          },
        },
        table = {
          bound = 'amd-dram',
          version = 0x05,
          transfer = 0x03,
          setup = 0x00,
          base = 0x04,
          base_low = 0,
          base_high = 1,
          size_word = 255,
          versions = {
            [0x730204] = 0xb1c,
            [0x730404] = 0x1004,
            [0x730604] = 0x14ec,
            [0x730804] = 0x19d4,
            [0x730a04] = 0x1ebc,
            [0x730c04] = 0x23a4,
            [0x730e04] = 0x288c,
            [0x731004] = 0x2d74,
          },
          dfmap = {
            device = 0x18,
            fn = 0x7,
            offset = 0xe00,
            count = 0x10,
            stride = 0x10,
            limit = 0x4,
            control = 0x8,
            shift = 0x1c,
            mask = 0xfffffff,
            valid = 0x0,
            remap = 0x1,
            hole = 0x104,
            hole_mask = 0xff000000,
            hole_valid = 0x0,
          },
        },
      }
    end,
  },
  {
    vendor = 'AuthenticAMD',
    family = 0x1a,
    models = { { 0x20, 0x2f } },
    pci = { 0x1022, 0x1507 },
    pin = { 0, 0 },
    mailbox = function()
      return {
        index = 0x60,
        data = 0x64,
        msg = 0x03b10a20,
        rsp = 0x03b10a80,
        arg = 0x03b10a88,
        args = 6,
        version = 0x02,
        version_arg = 0x1,
        commands = {
          [0x02] = {
            words = 1,
            internal = false,
            mask = { 0x0, 0x0, 0x0, 0x0, 0x0, 0x0 },
            max = { 0x0, 0x0, 0x0, 0x0, 0x0, 0x0 },
            fixed = { 0x1, 0x0, 0x0, 0x0, 0x0, 0x0 },
          },
          [0x06] = {
            words = 1,
            internal = false,
            mask = { 0x0, 0x0, 0x0, 0x0, 0x0, 0x0 },
            max = { 0x0, 0x0, 0x0, 0x0, 0x0, 0x0 },
            fixed = { 0x0, 0x0, 0x0, 0x0, 0x0, 0x0 },
          },
          [0x65] = {
            words = 0,
            internal = true,
            mask = { 0x0, 0x0, 0x0, 0x0, 0x0, 0x0 },
            max = { 0x0, 0x0, 0x0, 0x0, 0x0, 0x0 },
            fixed = { 0x3, 0x0, 0x0, 0x0, 0x0, 0x0 },
          },
          [0x66] = {
            words = 2,
            internal = true,
            mask = { 0x0, 0x0, 0x0, 0x0, 0x0, 0x0 },
            max = { 0x0, 0x0, 0x0, 0x0, 0x0, 0x0 },
            fixed = { 0x1, 0x1, 0x0, 0x0, 0x0, 0x0 },
          },
        },
        table = {
          bound = 'amd-dram',
          version = 0x06,
          transfer = 0x65,
          setup = 0x00,
          base = 0x66,
          base_low = 0,
          base_high = 1,
          size_word = 255,
          versions = {
            [0x5d0008] = 0xd54,
          },
          dfmap = {
            device = 0x18,
            fn = 0x7,
            offset = 0xe00,
            count = 0x10,
            stride = 0x10,
            limit = 0x4,
            control = 0x8,
            shift = 0x1c,
            mask = 0xfffffff,
            valid = 0x0,
            remap = 0x1,
            hole = 0x104,
            hole_mask = 0xff000000,
            hole_valid = 0x0,
          },
        },
      }
    end,
  },
  {
    vendor = 'AuthenticAMD',
    family = 0x1a,
    models = { { 0x40, 0x4f } },
    pci = { 0x1022, 0x14d8 },
    pin = { 0, 0 },
    mailbox = function()
      return {
        index = 0x60,
        data = 0x64,
        msg = 0x03b10524,
        rsp = 0x03b10570,
        arg = 0x03b10a40,
        args = 6,
        version = 0x02,
        version_arg = 0x1,
        commands = {
          [0x02] = {
            words = 1,
            internal = false,
            mask = { 0x0, 0x0, 0x0, 0x0, 0x0, 0x0 },
            max = { 0x0, 0x0, 0x0, 0x0, 0x0, 0x0 },
            fixed = { 0x1, 0x0, 0x0, 0x0, 0x0, 0x0 },
          },
          [0x05] = {
            words = 1,
            internal = false,
            mask = { 0x0, 0x0, 0x0, 0x0, 0x0, 0x0 },
            max = { 0x0, 0x0, 0x0, 0x0, 0x0, 0x0 },
            fixed = { 0x0, 0x0, 0x0, 0x0, 0x0, 0x0 },
          },
          [0x03] = {
            words = 0,
            internal = true,
            mask = { 0x0, 0x0, 0x0, 0x0, 0x0, 0x0 },
            max = { 0x0, 0x0, 0x0, 0x0, 0x0, 0x0 },
            fixed = { 0x0, 0x0, 0x0, 0x0, 0x0, 0x0 },
          },
          [0x04] = {
            words = 2,
            internal = true,
            mask = { 0x0, 0x0, 0x0, 0x0, 0x0, 0x0 },
            max = { 0x0, 0x0, 0x0, 0x0, 0x0, 0x0 },
            fixed = { 0x1, 0x1, 0x0, 0x0, 0x0, 0x0 },
          },
        },
        table = {
          bound = 'amd-dram',
          version = 0x05,
          transfer = 0x03,
          setup = 0x00,
          base = 0x04,
          base_low = 0,
          base_high = 1,
          size_word = 255,
          versions = {
            [0x620105] = 0x724,
            [0x620205] = 0x994,
            [0x621102] = 0x724,
            [0x621202] = 0x994,
          },
          dfmap = {
            device = 0x18,
            fn = 0x7,
            offset = 0xe00,
            count = 0x10,
            stride = 0x10,
            limit = 0x4,
            control = 0x8,
            shift = 0x1c,
            mask = 0xfffffff,
            valid = 0x0,
            remap = 0x1,
            hole = 0x104,
            hole_mask = 0xff000000,
            hole_valid = 0x0,
          },
        },
      }
    end,
  },
  {
    vendor = 'AuthenticAMD',
    family = 0x1a,
    models = { { 0x60, 0x6f } },
    pci = { 0x1022, 0x1122 },
    pin = { 0, 0 },
    mailbox = function()
      return {
        index = 0x60,
        data = 0x64,
        msg = 0x03b10a20,
        rsp = 0x03b10a80,
        arg = 0x03b10a88,
        args = 6,
        version = 0x02,
        version_arg = 0x1,
        commands = {
          [0x02] = {
            words = 1,
            internal = false,
            mask = { 0x0, 0x0, 0x0, 0x0, 0x0, 0x0 },
            max = { 0x0, 0x0, 0x0, 0x0, 0x0, 0x0 },
            fixed = { 0x1, 0x0, 0x0, 0x0, 0x0, 0x0 },
          },
          [0x06] = {
            words = 1,
            internal = false,
            mask = { 0x0, 0x0, 0x0, 0x0, 0x0, 0x0 },
            max = { 0x0, 0x0, 0x0, 0x0, 0x0, 0x0 },
            fixed = { 0x0, 0x0, 0x0, 0x0, 0x0, 0x0 },
          },
          [0x65] = {
            words = 0,
            internal = true,
            mask = { 0x0, 0x0, 0x0, 0x0, 0x0, 0x0 },
            max = { 0x0, 0x0, 0x0, 0x0, 0x0, 0x0 },
            fixed = { 0x3, 0x0, 0x0, 0x0, 0x0, 0x0 },
          },
          [0x66] = {
            words = 2,
            internal = true,
            mask = { 0x0, 0x0, 0x0, 0x0, 0x0, 0x0 },
            max = { 0x0, 0x0, 0x0, 0x0, 0x0, 0x0 },
            fixed = { 0x1, 0x1, 0x0, 0x0, 0x0, 0x0 },
          },
        },
      }
    end,
  },
}

local WAIT_MS = 250
local RESTORE_TRIES = 1000
local ALL_ONES = 0xffffffff

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
  kept.cpu = { vendor = string.pack('<I4I4I4', b, d, c), family = family, model = model }
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
    if
      row.pci
      and cpu_matches(row, cpu)
      and row.pci[1] == root.vendor
      and row.pci[2] == root.device
    then
      return row
    end
  end
  for _, row in ipairs(ROWS) do
    if
      row.class
      and cpu_matches(row, cpu)
      and row.class[1] == root.vendor
      and row.class[2] == root.class
      and row.class[3] == root.header
      and row.pin[1] == root.slot
      and row.pin[2] == root.fn
    then
      return row
    end
  end
  return nil
end

-- Identifies the processor and the root complex at a locator and finds the
-- row for both. Returns the session, or nil and the status of the call.
local function session(input)
  if not ksi.hw.pci_open or not ksi.hw.cpuid then
    ksi.trace('no PCI or CPUID functions')
    return nil, nt.STATUS_NOT_SUPPORTED
  end

  local locator = string.unpack('<I4', input)
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
      return { device = k.device, kept = k, mailbox = k.mailbox, root = k.root }
    end
    ksi.trace('the function kept for 0x%x changed', locator)
    kept[locator] = nil
    if k.table then
      k.table.map:release()
    end
    k.device:release()
  end

  local device, status = nt.pci.open(root.segment, root.bus, root.slot, root.fn)
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
  root.vendor, root.device = string.unpack('<I2I2', header, 1)
  root.class = string.unpack('<I4', header, 9) >> 8
  root.header = string.byte(header, 15) & 0x7f

  local row = find_row(cpu, root)
  if not row then
    ksi.trace(
      'no row for %s family 0x%x model 0x%x and PCI %04x:%04x',
      cpu.vendor,
      cpu.family,
      cpu.model,
      root.vendor,
      root.device
    )
    device:release()
    return nil, nt.STATUS_NOT_SUPPORTED
  end
  if row.pin[1] ~= root.slot or row.pin[2] ~= root.fn then
    ksi.trace('the mailbox is pinned to %d.%d', row.pin[1], row.pin[2])
    device:release()
    return nil, nt.STATUS_ACCESS_DENIED
  end

  local s = { device = device, mailbox = row.mailbox(), root = root }
  if not ksi.volatile and not nt.is_device_removable(device) then
    s.kept = { device = device, key = header_key(header), root = root, mailbox = s.mailbox }
    kept[locator] = s.kept
  end
  return s
end

-- Runs body(late) once a round until it returns true or the round that
-- started late has run; between rounds a stall for the first spins rounds
-- (or always, with no sleep), else a sleep.
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

-- One SMN read: index, data, index back, twice.
local function smn_read(s, address)
  local m = s.mailbox
  for _ = 1, 2 do
    local written, failed = ksi.hw.pci_write(s.device, m.index, 4, address)
    if not written then
      ksi.trace('index write for SMN 0x%x failed', address)
      return failed, 0
    end
    local value
    value, failed = ksi.hw.pci_read(s.device, m.data, 4)
    if not value then
      ksi.trace('data read of SMN 0x%x failed', address)
      return failed, 0
    end
    local check
    check, failed = ksi.hw.pci_read(s.device, m.index, 4)
    if not check then
      ksi.trace('index read for SMN 0x%x failed', address)
      return failed, 0
    end
    if check == address then
      return nt.STATUS_SUCCESS, value
    end
  end
  ksi.trace('the index of SMN 0x%x moved', address)
  return nt.STATUS_RETRY, 0
end

-- One SMN write: index, index back (failed or moved: not issued), data, index
-- back (moved: unproven); before and after, when given, run around the data
-- write.
local function smn_write(s, address, value, before, after)
  local m = s.mailbox
  if not ksi.hw.pci_write(s.device, m.index, 4, address) then
    ksi.trace('index write for SMN 0x%x failed', address)
    return nt.STATUS_RETRY
  end
  local check = ksi.hw.pci_read(s.device, m.index, 4)
  if not check then
    ksi.trace('index read for SMN 0x%x failed', address)
    return nt.STATUS_RETRY
  end
  if check ~= address then
    ksi.trace('the index of SMN 0x%x is not ours before the write', address)
    return nt.STATUS_RETRY
  end
  if before then
    before()
  end
  local written, failed = ksi.hw.pci_write(s.device, m.data, 4, value)
  if after then
    after()
  end
  if not written then
    ksi.trace('data write of SMN 0x%x failed', address)
    return failed
  end
  check, failed = ksi.hw.pci_read(s.device, m.index, 4)
  if not check then
    ksi.trace('index read for SMN 0x%x failed', address)
    return failed
  end
  if check ~= address then
    ksi.trace('the index of SMN 0x%x moved during the write', address)
    return nt.STATUS_DEVICE_DATA_ERROR
  end
  return nt.STATUS_SUCCESS
end

-- A store: an unproven write is settled by reading the register back.
local function store(s, register, value, mark)
  local st = smn_write(s, register, value, mark)
  if st == nt.STATUS_RETRY then
    return st, false
  end
  if nt.NT_SUCCESS(st) or st ~= nt.STATUS_DEVICE_DATA_ERROR then
    return st, true
  end
  local rs, held = smn_read(s, register)
  if rs == nt.STATUS_RETRY then
    return nt.STATUS_DEVICE_DATA_ERROR, true
  end
  if not nt.NT_SUCCESS(rs) then
    return rs, true
  end
  if held ~= value then
    ksi.trace('SMN 0x%x holds 0x%x, not 0x%x', register, held, value)
    return nt.STATUS_RETRY, true
  end
  return nt.STATUS_SUCCESS, true
end

-- RSP polled until it is not zero.
local function wait(s)
  local st, resp = nt.STATUS_IO_TIMEOUT, 0
  poll(50, 20, 1, WAIT_MS, function(late)
    local rs, value = smn_read(s, s.mailbox.rsp)
    if rs == nt.STATUS_RETRY and not late then
      value = 0
    elseif not nt.NT_SUCCESS(rs) then
      st = rs
      return true
    end
    if value == ALL_ONES then
      ksi.trace('RSP reads all-ones')
      st = nt.STATUS_DEVICE_NOT_READY
      return true
    end
    if value ~= 0 then
      st, resp = nt.STATUS_SUCCESS, value
      return true
    end
    return false
  end)
  if st == nt.STATUS_IO_TIMEOUT then
    ksi.trace('RSP stayed zero')
  end
  return st, resp
end

-- MSG read before, written, written again only when proven not landed.
local function post(s, op, mark)
  local m = s.mailbox
  local bs, before = smn_read(s, m.msg)
  local st = nt.STATUS_RETRY
  poll(0, 0, 1, WAIT_MS, function(late)
    local ws = smn_write(s, m.msg, op, nil, mark)
    if ws == nt.STATUS_RETRY then
      ws = smn_write(s, m.msg, op, nil, mark)
    end
    if ws == nt.STATUS_RETRY or nt.NT_SUCCESS(ws) then
      st = ws
      return true
    end
    if ws ~= nt.STATUS_DEVICE_DATA_ERROR then
      st = ws
      return true
    end
    if not nt.NT_SUCCESS(bs) then
      st = nt.STATUS_SUCCESS
      return true
    end
    local as, after = smn_read(s, m.msg)
    if not nt.NT_SUCCESS(as) then
      st = nt.STATUS_SUCCESS
      return true
    end
    if (after == op and before ~= op) or after ~= before or after == op then
      st = nt.STATUS_SUCCESS
      return true
    end
    if late then
      st = nt.STATUS_RETRY
      return true
    end
    return false
  end)
  if not nt.NT_SUCCESS(st) then
    ksi.trace('MSG 0x%x not posted: 0x%x', op, st)
  end
  return st
end

-- The idle RSP put back.
local function restore_idle(s, idle)
  for _ = 1, RESTORE_TRIES do
    local st = store(s, s.mailbox.rsp, idle)
    if nt.NT_SUCCESS(st) then
      return st
    end
    if st ~= nt.STATUS_RETRY and st ~= nt.STATUS_DEVICE_DATA_ERROR then
      break
    end
    ksi.sleep(1)
  end
  ksi.trace('the idle RSP 0x%x was not put back', idle)
  return nt.STATUS_DEVICE_DATA_ERROR
end

-- One result word, re-read while the index is contended.
local function read_result(s, register)
  local st, value = nt.STATUS_RETRY, 0
  poll(0, 0, 1, WAIT_MS, function()
    st, value = smn_read(s, register)
    return st ~= nt.STATUS_RETRY
  end)
  return st, value
end

local ANSWERS = {
  [0xfc] = nt.STATUS_DEVICE_BUSY,
  [0xfd] = nt.STATUS_INTERNAL_ERROR,
  [0xfe] = nt.STATUS_NOT_IMPLEMENTED,
}

-- The command up to its exit; state.restore says whether the idle RSP must be
-- put back. For a Lua error in between, state.clearing says RSP may have been
-- cleared before restore was set, and state.posting that MSG was written.
local function command_body(s, op, args, words, state)
  local m = s.mailbox
  local results = { 0, 0, 0, 0, 0, 0 }
  local st, resp = wait(s)
  if st == nt.STATUS_IO_TIMEOUT then
    ksi.trace('the mailbox stayed busy')
    return nt.STATUS_DEVICE_BUSY, false, results
  end
  if not nt.NT_SUCCESS(st) then
    return st, false, results
  end
  state.idle = resp
  local issued
  st, issued = store(s, m.rsp, 0, function()
    state.clearing = true
  end)
  if not nt.NT_SUCCESS(st) then
    state.restore = issued and st ~= nt.STATUS_RETRY
    state.clearing = false
    return st, false, results
  end
  state.restore = true
  state.clearing = false
  for i = 1, math.min(m.args, 6) do
    st = store(s, m.arg + (i - 1) * 4, args[i])
    if not nt.NT_SUCCESS(st) then
      return st, false, results
    end
  end
  st = post(s, op, function()
    state.posting = true
  end)
  if st == nt.STATUS_RETRY then
    return st, false, results
  end
  state.restore = false
  if not nt.NT_SUCCESS(st) then
    return st, false, results
  end
  st, resp = wait(s)
  if not nt.NT_SUCCESS(st) then
    return st, false, results
  end
  if resp ~= 0x01 then
    ksi.trace('MSG 0x%x answered 0x%x', op, resp)
    return ANSWERS[resp] or nt.STATUS_UNSUCCESSFUL, true, results, resp
  end
  for i = 1, words do
    st, results[i] = read_result(s, m.arg + (i - 1) * 4)
    if not nt.NT_SUCCESS(st) then
      return st, false, { 0, 0, 0, 0, 0, 0 }, resp
    end
  end
  return nt.STATUS_SUCCESS, true, results, resp
end

-- The command under the pci lock, then the idle RSP put back on its exit; a
-- Lua error that ends the call (a limit) puts it back from the close handler.
local function transact(s, op, args, words)
  local pci <close>, failed = ksi.lock('pci')
  if not pci then
    ksi.trace('the pci lock was not taken: 0x%x', failed)
    return failed, false, { 0, 0, 0, 0, 0, 0 }, 0
  end
  local state = { restore = false, idle = 0 }
  local putback <close> = setmetatable({}, {
    __close = function(_, err)
      if err and (state.restore or state.clearing) and not state.posting and state.idle ~= 0 then
        restore_idle(s, state.idle)
      end
    end,
  })
  local st, completed, results, answer = command_body(s, op, args, words, state)
  if state.restore and state.idle ~= 0 then
    local rs = restore_idle(s, state.idle)
    state.restore = false
    if not nt.NT_SUCCESS(rs) then
      st = rs
    end
  end
  return st, completed, results, answer or 0
end

-- A command with only the row's fixed argument bits.
local function row_command(s, op)
  local c = s.mailbox.commands[op]
  local fixed = c and c.fixed or { 0, 0, 0, 0, 0, 0 }
  return transact(s, op, fixed, c and c.words or 0)
end

-- The firmware version, asked twice; the two answers must agree.
local function firmware_version(s)
  local m = s.mailbox
  local args = { m.version_arg, 0, 0, 0, 0, 0 }
  local st, _, first = transact(s, m.version, args, 1)
  if not nt.NT_SUCCESS(st) then
    return st, 0
  end
  if first[1] == 0 or first[1] == ALL_ONES then
    ksi.trace('firmware version 0x%x', first[1])
    return nt.STATUS_DEVICE_NOT_READY, 0
  end
  local again
  st, _, again = transact(s, m.version, args, 1)
  if not nt.NT_SUCCESS(st) then
    return st, 0
  end
  if again[1] == 0 or again[1] == ALL_ONES then
    ksi.trace('firmware version 0x%x', again[1])
    return nt.STATUS_DEVICE_NOT_READY, 0
  end
  if again[1] ~= first[1] then
    ksi.trace('firmware version 0x%x then 0x%x', first[1], again[1])
    return nt.STATUS_RETRY, 0
  end
  return nt.STATUS_SUCCESS, first[1]
end

-- Returns the firmware version, the table version and the table length of
-- the mailbox behind the root complex the input names.
function identify(input)
  if #input ~= 4 then
    ksi.trace('input of %d bytes', #input)
    return nt.STATUS_INVALID_PARAMETER
  end
  local s, refused = session(input)
  if not s then
    return refused
  end
  local closer <close> = (not s.kept) and s.device or nil

  local lock <close>, failed = ksi.lock('mailbox')
  if not lock then
    ksi.trace('the mailbox lock was not taken: 0x%x', failed)
    return failed
  end

  local st, firmware = firmware_version(s)
  if not nt.NT_SUCCESS(st) then
    return st
  end

  local version, length = 0, 0
  local t = s.mailbox.table
  if t then
    local completed, results
    st, completed, results = row_command(s, t.version)
    if not nt.NT_SUCCESS(st) then
      ksi.trace('the table version was not read: 0x%x', st)
      return st
    end
    version = results[1]
    if t.versions[version] then
      length = t.versions[version]
    else
      ksi.trace('table version 0x%x is not in the row', version)
    end
  end

  return nt.STATUS_SUCCESS, string.pack('<I4I4I4I4', firmware, version, length, 0)
end

-- Sends a list of the row's commands to the mailbox behind the root complex
-- the input names.
function command(input)
  if #input < 4 or (#input - 4) % 28 ~= 0 then
    ksi.trace('input of %d bytes', #input)
    return nt.STATUS_INVALID_PARAMETER
  end
  local entries = {}
  for i = 1, (#input - 4) // 28 do
    local e = pack(string.unpack('<I4I4I4I4I4I4I4', input, 5 + (i - 1) * 28))
    if e[1] == 0 then
      ksi.trace('entry %d has opcode 0', i)
      return nt.STATUS_INVALID_PARAMETER
    end
    entries[i] = e
  end

  local s, refused = session(input)
  if not s then
    return refused
  end
  local closer <close> = (not s.kept) and s.device or nil

  local lock <close>, failed = ksi.lock('mailbox')
  if not lock then
    ksi.trace('the mailbox lock was not taken: 0x%x', failed)
    return failed
  end

  local st = firmware_version(s)
  if not nt.NT_SUCCESS(st) then
    return st
  end

  local output = {}
  local stopped = false
  for i, e in ipairs(entries) do
    local op = e[1]
    local c = s.mailbox.commands[op]
    local status, answer = nt.STATUS_SUCCESS, 0
    local results = { 0, 0, 0, 0, 0, 0 }
    local args
    if c and not c.internal then
      args = {}
      for a = 1, 6 do
        local value = e[a + 1]
        if (value & ~c.mask[a]) ~= 0 or value > c.max[a] then
          args = nil
          break
        end
        args[a] = value | c.fixed[a]
      end
    end
    if not args then
      ksi.trace('command 0x%x is not permitted', op)
      status = nt.STATUS_ACCESS_DENIED
    elseif stopped then
      status = nt.STATUS_CANCELLED
    else
      local completed
      status, completed, results, answer = transact(s, op, args, c.words)
      stopped = not completed
    end
    output[i] = string.pack('<i4I4I4I4I4I4I4I4', status, answer, unpack(results, 1, 6))
  end

  return nt.STATUS_SUCCESS, concat(output)
end

local PAGE = 0x1000
local LEGACY_END = 0x100000
local FOUR_GB = 0x100000000

local MSR_SYSCFG = 0xc0010010
local MSR_TOP_MEM = 0xc001001a
local MSR_TOP_MEM2 = 0xc001001d
local MSR_SMM_ADDR = 0xc0010112
local MSR_SMM_MASK = 0xc0010113
local MSR_IORR_BASE0 = 0xc0010016
local SYSCFG_VAR_DRAM_EN = 1 << 20
local SYSCFG_TOM2_EN = 1 << 21
local TOP_MEM_LOW_BITS = 0x7fffff
local SMM_MASK_TVALID = 1 << 1
local SMM_LOW_BITS = 0x1ffff
local IORR_MASK_VALID = 1 << 11
local IORR_BASE_RDMEM = 1 << 4
local IORR_LOW_BITS = 0xfff
local IORR_COUNT = 2

-- The physical address width of processor 0:0: CPUID 80000008h EAX[7:0], 36
-- when the leaf is absent or its width is outside 32 to 63.
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
  ksi.trace('address width 36: CPUID 80000008h not usable')
  if nt.NT_SUCCESS(s0) and nt.NT_SUCCESS(s1) then
    kept.bits = 36
  end
  return 36
end

-- The MSRs of the AMD address map, read on processor 0:0; a register that
-- faults (a hypervisor hides several) is nil.
local function address_map()
  local indexes = { MSR_SYSCFG, MSR_TOP_MEM, MSR_TOP_MEM2, MSR_SMM_ADDR, MSR_SMM_MASK }
  for i = 0, IORR_COUNT * 2 - 1 do
    indexes[#indexes + 1] = MSR_IORR_BASE0 + i
  end
  local items = {}
  for i, index in ipairs(indexes) do
    items[i] = string.pack('<I2I2I4', 0, 0, index)
  end
  local results, status = ksi.hw.msr_read(concat(items))
  if not results then
    ksi.trace('msr_read failed: 0x%x', status)
    return nil, status
  end
  local map = {}
  for i, index in ipairs(indexes) do
    local s, _, value = string.unpack('<i4I4I8', results, (i - 1) * 16 + 1)
    if nt.NT_SUCCESS(s) then
      map[index] = value
    end
  end
  return map
end

local function amd_region(base, mask, low, limit)
  local m = mask & limit & ~low
  local first = base & m
  return first, first + ((~m & limit) + 1)
end

-- DRAM as the processor decodes it: below TOP_MEM or TOP_MEM2, not TSEG, not
-- read from I/O by an IORR. A hidden register skips the check that needs it.
local function check_amd_dram(first, last, bits)
  local limit = (1 << bits) - 1
  local msrs, failed = address_map()
  if not msrs then
    return failed
  end

  local syscfg = msrs[MSR_SYSCFG]
  if not syscfg then
    ksi.trace('DRAM bound skipped: SYSCFG hidden')
  elseif syscfg & SYSCFG_VAR_DRAM_EN == 0 then
    ksi.trace('range 0x%x refused: SYSCFG 0x%x has no TOP_MEM', first, syscfg)
    return nt.STATUS_CONFLICTING_ADDRESSES
  elseif not msrs[MSR_TOP_MEM] then
    ksi.trace('DRAM bound skipped: TOP_MEM hidden')
  else
    local top = msrs[MSR_TOP_MEM] & (limit & ~TOP_MEM_LOW_BITS)
    if top == 0 then
      ksi.trace('range 0x%x refused: TOP_MEM is zero', first)
      return nt.STATUS_CONFLICTING_ADDRESSES
    end
    local inside = not math.ult(first, LEGACY_END) and not math.ult(top, last)
    local skipped = false
    if not inside and syscfg & SYSCFG_TOM2_EN ~= 0 then
      if not msrs[MSR_TOP_MEM2] then
        ksi.trace('DRAM bound skipped: TOP_MEM2 hidden')
        skipped = true
      else
        local top2 = msrs[MSR_TOP_MEM2] & (limit & ~TOP_MEM_LOW_BITS)
        inside = not math.ult(first, FOUR_GB) and not math.ult(top2, last)
      end
    end
    if not inside and not skipped then
      ksi.trace(
        'range 0x%x-0x%x refused: not below TOP_MEM 0x%x or ' .. 'TOP_MEM2',
        first,
        last,
        top
      )
      return nt.STATUS_CONFLICTING_ADDRESSES
    end
  end

  local base, mask = msrs[MSR_SMM_ADDR], msrs[MSR_SMM_MASK]
  if not base or not mask then
    ksi.trace('TSEG check skipped: TSEG hidden')
  else
    local tsegFirst, tsegLast = amd_region(base, mask, SMM_LOW_BITS, limit)
    if mask & SMM_MASK_TVALID ~= 0 and math.ult(first, tsegLast) and math.ult(tsegFirst, last) then
      ksi.trace('range 0x%x-0x%x refused: TSEG 0x%x-0x%x', first, last, tsegFirst, tsegLast)
      return nt.STATUS_CONFLICTING_ADDRESSES
    end
  end

  for i = 0, IORR_COUNT - 1 do
    local ibase = msrs[MSR_IORR_BASE0 + i * 2]
    local imask = msrs[MSR_IORR_BASE0 + i * 2 + 1]
    if not ibase or not imask then
      ksi.trace('IORR %d check skipped: IORR hidden', i)
    elseif imask & IORR_MASK_VALID ~= 0 and ibase & IORR_BASE_RDMEM == 0 then
      local ioFirst, ioLast = amd_region(ibase, imask, IORR_LOW_BITS, limit)
      if math.ult(first, ioLast) and math.ult(ioFirst, last) then
        ksi.trace('range 0x%x-0x%x refused: IORR %d reads I/O', first, last, i)
        return nt.STATUS_CONFLICTING_ADDRESSES
      end
    end
  end

  return nt.STATUS_SUCCESS
end

-- The ECAM page of a function, from the MCFG ranges; nil when no range
-- decodes the bus or the table is absent or malformed.
local function ecam_page(segment, bus, device, fn)
  local mcfg = kept.mcfg or ksi.hw.firmware_table('ACPI', 'MCFG')
  if not mcfg or #mcfg < 44 or (#mcfg - 44) % 16 ~= 0 then
    ksi.trace('no MCFG ranges')
    return nil
  end
  kept.mcfg = mcfg
  for at = 45, #mcfg, 16 do
    local base, entrySegment, startBus, endBus = string.unpack('<I8I2I1I1', mcfg, at)
    if entrySegment == segment and bus >= startBus and bus <= endBus then
      return base + (bus << 20) + (device << 15) + (fn << 12)
    end
  end
  return nil
end

-- DRAM as the data fabric decodes it, through the fabric's ECAM page. Without
-- an ECAM range for it the check is skipped.
local function check_amd_df(df, root, first, last, bits)
  local page = ecam_page(root.segment, root.bus, df.device, df.fn)
  if not page then
    ksi.trace('data fabric bound skipped: no ECAM range for %x:%x', root.segment, root.bus)
    return nt.STATUS_SUCCESS
  end
  local st = nt.check_range(page, page + PAGE, bits)
  if not nt.NT_SUCCESS(st) then
    ksi.trace('ECAM page 0x%x refused: 0x%x', page, st)
    return st
  end
  local m <close>, failed = ksi.hw.map(page, PAGE)
  if not m then
    ksi.trace('ECAM page 0x%x not mapped: 0x%x', page, failed)
    return failed
  end
  if ksi.hw.map_read(m, 0, 4) == 0xffffffff then
    ksi.trace('data fabric at 0x%x reads all-ones', page)
    return nt.STATUS_DEVICE_NOT_READY
  end
  local holeValid, holeBase = false, 0
  if df.hole ~= 0 then
    local hole = ksi.hw.map_read(m, df.hole, 4)
    holeValid = hole & (1 << df.hole_valid) ~= 0
    holeBase = hole & df.hole_mask
  end
  for i = 0, df.count - 1 do
    local offset = df.offset + i * df.stride
    local control = ksi.hw.map_read(m, offset + df.control, 4)
    if control & (1 << df.valid) ~= 0 then
      local base = ksi.hw.map_read(m, offset, 4)
      local limit = ksi.hw.map_read(m, offset + df.limit, 4)
      local mapFirst = (base & df.mask) << df.shift
      local mapLast = ((limit & df.mask) << df.shift) + (1 << df.shift)
      if not math.ult(first, mapFirst) and not math.ult(mapLast, last) then
        if math.ult(first, LEGACY_END) then
          ksi.trace('range 0x%x refused: in the legacy region', first)
          return nt.STATUS_CONFLICTING_ADDRESSES
        end
        if
          holeValid
          and control & (1 << df.remap) ~= 0
          and math.ult(first, FOUR_GB)
          and math.ult(holeBase, last)
        then
          ksi.trace('range 0x%x refused: DRAM hole at 0x%x', first, holeBase)
          return nt.STATUS_CONFLICTING_ADDRESSES
        end
        return nt.STATUS_SUCCESS
      end
    end
  end
  ksi.trace('range 0x%x-0x%x refused: no data fabric DRAM map', first, last)
  return nt.STATUS_CONFLICTING_ADDRESSES
end

-- The table base: the setup command when the row has one, then the base
-- command.
local function table_base(s, t)
  local st, results
  if t.setup ~= 0 then
    st = row_command(s, t.setup)
    if not nt.NT_SUCCESS(st) then
      return st, 0
    end
  end
  local completed
  st, completed, results = row_command(s, t.base)
  if not nt.NT_SUCCESS(st) then
    return st, 0
  end
  local base = results[t.base_low + 1]
  if t.base_high ~= 0xff then
    base = base | (results[t.base_high + 1] << 32)
  end
  return nt.STATUS_SUCCESS, base, results
end

-- Returns the power-management table of the mailbox behind the root complex
-- the input names: its version, its length and its bytes.
function metrics(input)
  if #input ~= 4 then
    ksi.trace('input of %d bytes', #input)
    return nt.STATUS_INVALID_PARAMETER
  end
  local s, refused = session(input)
  if not s then
    return refused
  end
  local closer <close> = (not s.kept) and s.device or nil
  local t = s.mailbox.table
  if not t then
    ksi.trace('the row has no table')
    return nt.STATUS_NOT_FOUND
  end
  local bits, failed = address_bits()
  if not bits then
    return failed
  end

  local lock <close>, lockFailed = ksi.lock('mailbox')
  if not lock then
    ksi.trace('the mailbox lock was not taken: 0x%x', lockFailed)
    return lockFailed
  end

  local st = firmware_version(s)
  if not nt.NT_SUCCESS(st) then
    return st
  end
  local commands = s.mailbox.commands
  for _, op in ipairs({ t.version, t.base, t.transfer, t.setup }) do
    if op ~= 0 and not commands[op] then
      ksi.trace('table command 0x%x is not in the row', op)
      return nt.STATUS_ACCESS_DENIED
    end
  end

  local completed, results
  st, completed, results = row_command(s, t.version)
  if not nt.NT_SUCCESS(st) then
    ksi.trace('the table version was not read: 0x%x', st)
    return st
  end
  local version = results[1]
  local size = t.versions[version]
  if not size then
    ksi.trace('table version 0x%x is not supported', version)
    return nt.STATUS_NOT_SUPPORTED
  end

  if t.setup ~= 0 and t.transfer ~= 0 then
    st = row_command(s, t.setup)
    if not nt.NT_SUCCESS(st) then
      return st
    end
  end
  if t.transfer ~= 0 then
    st = row_command(s, t.transfer)
    if not nt.NT_SUCCESS(st) then
      return st
    end
  end

  local base, check, reply
  st, base, reply = table_base(s, t)
  if not nt.NT_SUCCESS(st) then
    return st
  end
  st, check = table_base(s, t)
  if not nt.NT_SUCCESS(st) then
    return st
  end
  if check ~= base then
    ksi.trace('table base 0x%x moved to 0x%x', base, check)
    return nt.STATUS_RETRY
  end
  if t.size_word ~= 0xff then
    local reported = reply[t.size_word + 1]
    if reported ~= 0 and reported < size then
      ksi.trace('reported table size %d below %d', reported, size)
      return nt.STATUS_DEVICE_CONFIGURATION_ERROR
    end
  end
  if base == 0 then
    ksi.trace('table base 0')
    return nt.STATUS_DEVICE_NOT_READY
  end
  local last = base + size
  if math.ult(last, base) then
    ksi.trace('table base 0x%x plus %d overflows', base, size)
    return nt.STATUS_INTEGER_OVERFLOW
  end
  if base % PAGE ~= 0 or math.ult(-PAGE, last) then
    ksi.trace('table range 0x%x-0x%x invalid', base, last)
    return nt.STATUS_DEVICE_CONFIGURATION_ERROR
  end
  local rangeEnd = (last + PAGE - 1) & ~(PAGE - 1)

  local k = s.kept
  local kt = k and k.table
  if kt and (kt.base ~= base or kt.size ~= size) then
    ksi.trace('table at 0x%x size %d was 0x%x size %d', base, size, kt.base, kt.size)
    k.table = nil
    kt.map:release()
    kt = nil
  end

  if not kt then
    st = check_amd_dram(base, rangeEnd, bits)
    if not nt.NT_SUCCESS(st) then
      ksi.trace('table version 0x%x base 0x%x refused: 0x%x', version, base, st)
      return st
    end
    if t.dfmap then
      st = check_amd_df(t.dfmap, s.root, base, rangeEnd, bits)
      if not nt.NT_SUCCESS(st) then
        ksi.trace('table base 0x%x refused by the data fabric: 0x%x', base, st)
        return st
      end
    end
    st = nt.check_range(base, rangeEnd, bits)
    if not nt.NT_SUCCESS(st) then
      ksi.trace('table version 0x%x refused: 0x%x', version, st)
      return st
    end
  end

  if size % 4 ~= 0 then
    ksi.trace('table length %d is not whole words', size)
    return nt.STATUS_DEVICE_CONFIGURATION_ERROR
  end

  local m = kt and kt.map
  if not m then
    local mapFailed
    m, mapFailed = ksi.hw.map(base, size)
    if not m then
      ksi.trace('table at 0x%x not mapped: 0x%x', base, mapFailed)
      return mapFailed
    end
    if k then
      k.table = { map = m, base = base, size = size }
    end
  end
  local bytes
  do
    local mapping <close> = (not k) and m or nil
    bytes = ksi.hw.map_read_block(m, 0, size, 4)
    if ksi.hw.map_read(m, 0, 4) ~= string.unpack('<I4', bytes) then
      ksi.trace('table changed during the copy')
      return nt.STATUS_RETRY
    end
  end

  st, check = table_base(s, t)
  if not nt.NT_SUCCESS(st) then
    return st
  end
  if check ~= base then
    ksi.trace('table base 0x%x moved to 0x%x', base, check)
    return nt.STATUS_RETRY
  end

  return nt.STATUS_SUCCESS, string.pack('<I4I4', version, size) .. bytes
end
