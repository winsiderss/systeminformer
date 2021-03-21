/* ======================================================================== */
/* ========================= LICENSING & COPYRIGHT ======================== */
/* ======================================================================== */
/*
 *                                  MUSASHI
 *                                Version 3.4
 *
 * A portable Motorola M680x0 processor emulation engine.
 * Copyright 1998-2001 Karl Stenerud.  All rights reserved.
 *
 * Permission is hereby granted, free of charge, to any person obtaining a copy
 * of this software and associated documentation files (the "Software"), to deal
 * in the Software without restriction, including without limitation the rights
 * to use, copy, modify, merge, publish, distribute, sublicense, and/or sell
 * copies of the Software, and to permit persons to whom the Software is
 * furnished to do so, subject to the following conditions:
 *
 * The above copyright notice and this permission notice shall be included in
 * all copies or substantial portions of the Software.

 * THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
 * IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
 * FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
 * AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
 * LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
 * OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN
 * THE SOFTWARE.
 */

/* The code below is based on MUSASHI but has been heavily modified for Capstone by
 * Daniel Collin <daniel@collin.com> 2015-2019 */

/* ======================================================================== */
/* ================================ INCLUDES ============================== */
/* ======================================================================== */

#include <stdlib.h>
#include <stdio.h>
#include <string.h>

#include "../../cs_priv.h"
#include "../../utils.h"

#include "../../MCInst.h"
#include "../../MCInstrDesc.h"
#include "../../MCRegisterInfo.h"
#include "M68KInstPrinter.h"
#include "M68KDisassembler.h"

/* ======================================================================== */
/* ============================ GENERAL DEFINES =========================== */
/* ======================================================================== */

/* Bit Isolation Functions */
#define BIT_0(A)  ((A) & 0x00000001)
#define BIT_1(A)  ((A) & 0x00000002)
#define BIT_2(A)  ((A) & 0x00000004)
#define BIT_3(A)  ((A) & 0x00000008)
#define BIT_4(A)  ((A) & 0x00000010)
#define BIT_5(A)  ((A) & 0x00000020)
#define BIT_6(A)  ((A) & 0x00000040)
#define BIT_7(A)  ((A) & 0x00000080)
#define BIT_8(A)  ((A) & 0x00000100)
#define BIT_9(A)  ((A) & 0x00000200)
#define BIT_A(A)  ((A) & 0x00000400)
#define BIT_B(A)  ((A) & 0x00000800)
#define BIT_C(A)  ((A) & 0x00001000)
#define BIT_D(A)  ((A) & 0x00002000)
#define BIT_E(A)  ((A) & 0x00004000)
#define BIT_F(A)  ((A) & 0x00008000)
#define BIT_10(A) ((A) & 0x00010000)
#define BIT_11(A) ((A) & 0x00020000)
#define BIT_12(A) ((A) & 0x00040000)
#define BIT_13(A) ((A) & 0x00080000)
#define BIT_14(A) ((A) & 0x00100000)
#define BIT_15(A) ((A) & 0x00200000)
#define BIT_16(A) ((A) & 0x00400000)
#define BIT_17(A) ((A) & 0x00800000)
#define BIT_18(A) ((A) & 0x01000000)
#define BIT_19(A) ((A) & 0x02000000)
#define BIT_1A(A) ((A) & 0x04000000)
#define BIT_1B(A) ((A) & 0x08000000)
#define BIT_1C(A) ((A) & 0x10000000)
#define BIT_1D(A) ((A) & 0x20000000)
#define BIT_1E(A) ((A) & 0x40000000)
#define BIT_1F(A) ((A) & 0x80000000)

/* These are the CPU types understood by this disassembler */
#define TYPE_68000 1
#define TYPE_68010 2
#define TYPE_68020 4
#define TYPE_68030 8
#define TYPE_68040 16

#define M68000_ONLY		TYPE_68000

#define M68010_ONLY		TYPE_68010
#define M68010_LESS		(TYPE_68000 | TYPE_68010)
#define M68010_PLUS		(TYPE_68010 | TYPE_68020 | TYPE_68030 | TYPE_68040)

#define M68020_ONLY 	TYPE_68020
#define M68020_LESS 	(TYPE_68010 | TYPE_68020)
#define M68020_PLUS		(TYPE_68020 | TYPE_68030 | TYPE_68040)

#define M68030_ONLY 	TYPE_68030
#define M68030_LESS 	(TYPE_68010 | TYPE_68020 | TYPE_68030)
#define M68030_PLUS		(TYPE_68030 | TYPE_68040)

#define M68040_PLUS		TYPE_68040

enum {
	M68K_CPU_TYPE_INVALID,
	M68K_CPU_TYPE_68000,
	M68K_CPU_TYPE_68010,
	M68K_CPU_TYPE_68EC020,
	M68K_CPU_TYPE_68020,
	M68K_CPU_TYPE_68030,	/* Supported by disassembler ONLY */
	M68K_CPU_TYPE_68040		/* Supported by disassembler ONLY */
};

/* Extension word formats */
#define EXT_8BIT_DISPLACEMENT(A)          ((A)&0xff)
#define EXT_FULL(A)                       BIT_8(A)
#define EXT_EFFECTIVE_ZERO(A)             (((A)&0xe4) == 0xc4 || ((A)&0xe2) == 0xc0)
#define EXT_BASE_REGISTER_PRESENT(A)      (!BIT_7(A))
#define EXT_INDEX_REGISTER_PRESENT(A)     (!BIT_6(A))
#define EXT_INDEX_REGISTER(A)             (((A)>>12)&7)
#define EXT_INDEX_PRE_POST(A)             (EXT_INDEX_PRESENT(A) && (A)&3)
#define EXT_INDEX_PRE(A)                  (EXT_INDEX_PRESENT(A) && ((A)&7) < 4 && ((A)&7) != 0)
#define EXT_INDEX_POST(A)                 (EXT_INDEX_PRESENT(A) && ((A)&7) > 4)
#define EXT_INDEX_SCALE(A)                (((A)>>9)&3)
#define EXT_INDEX_LONG(A)                 BIT_B(A)
#define EXT_INDEX_AR(A)                   BIT_F(A)
#define EXT_BASE_DISPLACEMENT_PRESENT(A)  (((A)&0x30) > 0x10)
#define EXT_BASE_DISPLACEMENT_WORD(A)     (((A)&0x30) == 0x20)
#define EXT_BASE_DISPLACEMENT_LONG(A)     (((A)&0x30) == 0x30)
#define EXT_OUTER_DISPLACEMENT_PRESENT(A) (((A)&3) > 1 && ((A)&0x47) < 0x44)
#define EXT_OUTER_DISPLACEMENT_WORD(A)    (((A)&3) == 2 && ((A)&0x47) < 0x44)
#define EXT_OUTER_DISPLACEMENT_LONG(A)    (((A)&3) == 3 && ((A)&0x47) < 0x44)

#define IS_BITSET(val,b) ((val) & (1 << (b)))
#define BITFIELD_MASK(sb,eb)  (((1 << ((sb) + 1))-1) & (~((1 << (eb))-1)))
#define BITFIELD(val,sb,eb) ((BITFIELD_MASK(sb,eb) & (val)) >> (eb))

///////////////////////////////////////////////////////////////////////////////////////////////////////////////////////

static unsigned int m68k_read_disassembler_16(const m68k_info *info, const uint64_t addr)
{
	const uint16_t v0 = info->code[addr + 0];
	const uint16_t v1 = info->code[addr + 1];
	return (v0 << 8) | v1;
}

static unsigned int m68k_read_disassembler_32(const m68k_info *info, const uint64_t addr)
{
	const uint32_t v0 = info->code[addr + 0];
	const uint32_t v1 = info->code[addr + 1];
	const uint32_t v2 = info->code[addr + 2];
	const uint32_t v3 = info->code[addr + 3];
	return (v0 << 24) | (v1 << 16) | (v2 << 8) | v3;
}

static uint64_t m68k_read_disassembler_64(const m68k_info *info, const uint64_t addr)
{
	const uint64_t v0 = info->code[addr + 0];
	const uint64_t v1 = info->code[addr + 1];
	const uint64_t v2 = info->code[addr + 2];
	const uint64_t v3 = info->code[addr + 3];
	const uint64_t v4 = info->code[addr + 4];
	const uint64_t v5 = info->code[addr + 5];
	const uint64_t v6 = info->code[addr + 6];
	const uint64_t v7 = info->code[addr + 7];
	return (v0 << 56) | (v1 << 48) | (v2 << 40) | (v3 << 32) | (v4 << 24) | (v5 << 16) | (v6 << 8) | v7;
}

static unsigned int m68k_read_safe_16(const m68k_info *info, const uint64_t address)
{
	const uint64_t addr = (address - info->baseAddress) & info->address_mask;
	if (info->code_len < addr + 2) {
		return 0xaaaa;
	}
	return m68k_read_disassembler_16(info, addr);
}

static unsigned int m68k_read_safe_32(const m68k_info *info, const uint64_t address)
{
	const uint64_t addr = (address - info->baseAddress) & info->address_mask;
	if (info->code_len < addr + 4) {
		return 0xaaaaaaaa;
	}
	return m68k_read_disassembler_32(info, addr);
}

static uint64_t m68k_read_safe_64(const m68k_info *info, const uint64_t address)
{
	const uint64_t addr = (address - info->baseAddress) & info->address_mask;
	if (info->code_len < addr + 8) {
		return 0xaaaaaaaaaaaaaaaaLL;
	}
	return m68k_read_disassembler_64(info, addr);
}

/* ======================================================================== */
/* =============================== PROTOTYPES ============================= */
/* ======================================================================== */

/* make signed integers 100% portably */
static int make_int_8(int value);
static int make_int_16(int value);

/* Stuff to build the opcode handler jump table */
static void d68000_invalid(m68k_info *info);
static int instruction_is_valid(m68k_info *info, const unsigned int word_check);

typedef struct {
	void (*instruction)(m68k_info *info);   /* handler function */
	uint16_t word2_mask;              		/* mask the 2nd word */
	uint16_t word2_match;             		/* what to match after masking */
} instruction_struct;

/* ======================================================================== */
/* ================================= DATA ================================= */
/* ======================================================================== */

static instruction_struct g_instruction_table[0x10000];

/* used by ops like asr, ror, addq, etc */
static uint32_t g_3bit_qdata_table[8] = {8, 1, 2, 3, 4, 5, 6, 7};

static uint32_t g_5bit_data_table[32] = {
	32,  1,  2,  3,  4,  5,  6,  7,  8,  9, 10, 11, 12, 13, 14, 15,
	16, 17, 18, 19, 20, 21, 22, 23, 24, 25, 26, 27, 28, 29, 30, 31
};

static m68k_insn s_branch_lut[] = {
	M68K_INS_INVALID, M68K_INS_INVALID, M68K_INS_BHI, M68K_INS_BLS,
	M68K_INS_BCC, M68K_INS_BCS, M68K_INS_BNE, M68K_INS_BEQ,
	M68K_INS_BVC, M68K_INS_BVS, M68K_INS_BPL, M68K_INS_BMI,
	M68K_INS_BGE, M68K_INS_BLT, M68K_INS_BGT, M68K_INS_BLE,
};

static m68k_insn s_dbcc_lut[] = {
	M68K_INS_DBT, M68K_INS_DBF, M68K_INS_DBHI, M68K_INS_DBLS,
	M68K_INS_DBCC, M68K_INS_DBCS, M68K_INS_DBNE, M68K_INS_DBEQ,
	M68K_INS_DBVC, M68K_INS_DBVS, M68K_INS_DBPL, M68K_INS_DBMI,
	M68K_INS_DBGE, M68K_INS_DBLT, M68K_INS_DBGT, M68K_INS_DBLE,
};

static m68k_insn s_scc_lut[] = {
	M68K_INS_ST, M68K_INS_SF, M68K_INS_SHI, M68K_INS_SLS,
	M68K_INS_SCC, M68K_INS_SCS, M68K_INS_SNE, M68K_INS_SEQ,
	M68K_INS_SVC, M68K_INS_SVS, M68K_INS_SPL, M68K_INS_SMI,
	M68K_INS_SGE, M68K_INS_SLT, M68K_INS_SGT, M68K_INS_SLE,
};

static m68k_insn s_trap_lut[] = {
	M68K_INS_TRAPT, M68K_INS_TRAPF, M68K_INS_TRAPHI, M68K_INS_TRAPLS,
	M68K_INS_TRAPCC, M68K_INS_TRAPCS, M68K_INS_TRAPNE, M68K_INS_TRAPEQ,
	M68K_INS_TRAPVC, M68K_INS_TRAPVS, M68K_INS_TRAPPL, M68K_INS_TRAPMI,
	M68K_INS_TRAPGE, M68K_INS_TRAPLT, M68K_INS_TRAPGT, M68K_INS_TRAPLE,
};

/* ======================================================================== */
/* =========================== UTILITY FUNCTIONS ========================== */
/* ======================================================================== */

#define LIMIT_CPU_TYPES(info, ALLOWED_CPU_TYPES)	\
	do {						\
		if (!(info->type & ALLOWED_CPU_TYPES)) {	\
			d68000_invalid(info);		\
			return;				\
		}					\
	} while (0)

static unsigned int peek_imm_8(const m68k_info *info)  { return (m68k_read_safe_16((info), (info)->pc)&0xff); }
static unsigned int peek_imm_16(const m68k_info *info) { return m68k_read_safe_16((info), (info)->pc); }
static unsigned int peek_imm_32(const m68k_info *info) { return m68k_read_safe_32((info), (info)->pc); }
static unsigned long long peek_imm_64(const m68k_info *info) { return m68k_read_safe_64((info), (info)->pc); }

static unsigned int read_imm_8(m68k_info *info)  { const unsigned int value = peek_imm_8(info);  (info)->pc+=2; return value; }
static unsigned int read_imm_16(m68k_info *info) { const unsigned int value = peek_imm_16(info); (info)->pc+=2; return value; }
static unsigned int read_imm_32(m68k_info *info) { const unsigned int value = peek_imm_32(info); (info)->pc+=4; return value; }
static unsigned long long read_imm_64(m68k_info *info) { const unsigned long long value = peek_imm_64(info); (info)->pc+=8; return value; }

/* Fake a split interface */
#define get_ea_mode_str_8(instruction) get_ea_mode_str(instruction, 0)
#define get_ea_mode_str_16(instruction) get_ea_mode_str(instruction, 1)
#define get_ea_mode_str_32(instruction) get_ea_mode_str(instruction, 2)

#define get_imm_str_s8() get_imm_str_s(0)
#define get_imm_str_s16() get_imm_str_s(1)
#define get_imm_str_s32() get_imm_str_s(2)

#define get_imm_str_u8() get_imm_str_u(0)
#define get_imm_str_u16() get_imm_str_u(1)
#define get_imm_str_u32() get_imm_str_u(2)


/* 100% portable signed int generators */
static int make_int_8(int value)
{
	return (value & 0x80) ? value | ~0xff : value & 0xff;
}

static int make_int_16(int value)
{
	return (value & 0x8000) ? value | ~0xffff : value & 0xffff;
}

static void get_with_index_address_mode(m68k_info *info, cs_m68k_op* op, uint32_t instruction, uint32_t size, bool is_pc)
{
	uint32_t extension = read_imm_16(info);

	op->address_mode = M68K_AM_AREGI_INDEX_BASE_DISP;

	if (EXT_FULL(extension)) {
		uint32_t preindex;
		uint32_t postindex;

		op->mem.base_reg = M68K_REG_INVALID;
		op->mem.index_reg = M68K_REG_INVALID;

		/* Not sure how to deal with this?
		   if (EXT_EFFECTIVE_ZERO(extension)) {
		   strcpy(mode, "0");
		   break;
		   }
		 */

		op->mem.in_disp = EXT_BASE_DISPLACEMENT_PRESENT(extension) ? (EXT_BASE_DISPLACEMENT_LONG(extension) ? read_imm_32(info) : read_imm_16(info)) : 0;
		op->mem.out_disp = EXT_OUTER_DISPLACEMENT_PRESENT(extension) ? (EXT_OUTER_DISPLACEMENT_LONG(extension) ? read_imm_32(info) : read_imm_16(info)) : 0;

		if (EXT_BASE_REGISTER_PRESENT(extension)) {
			if (is_pc) {
				op->mem.base_reg = M68K_REG_PC;
			} else {
				op->mem.base_reg = M68K_REG_A0 + (instruction & 7);
			}
		}

		if (EXT_INDEX_REGISTER_PRESENT(extension)) {
			if (EXT_INDEX_AR(extension)) {
				op->mem.index_reg = M68K_REG_A0 + EXT_INDEX_REGISTER(extension);
			} else {
				op->mem.index_reg = M68K_REG_D0 + EXT_INDEX_REGISTER(extension);
			}

			op->mem.index_size = EXT_INDEX_LONG(extension) ? 1 : 0;

			if (EXT_INDEX_SCALE(extension)) {
				op->mem.scale = 1 << EXT_INDEX_SCALE(extension);
			}
		}

		preindex = (extension & 7) > 0 && (extension & 7) < 4;
		postindex = (extension & 7) > 4;

		if (preindex) {
			op->address_mode = is_pc ? M68K_AM_PC_MEMI_PRE_INDEX : M68K_AM_MEMI_PRE_INDEX;
		} else if (postindex) {
			op->address_mode = is_pc ? M68K_AM_PC_MEMI_POST_INDEX : M68K_AM_MEMI_POST_INDEX;
		}

		return;
	}

	op->mem.index_reg = (EXT_INDEX_AR(extension) ? M68K_REG_A0 : M68K_REG_D0) + EXT_INDEX_REGISTER(extension);
	op->mem.index_size = EXT_INDEX_LONG(extension) ? 1 : 0;

	if (EXT_8BIT_DISPLACEMENT(extension) == 0) {
		if (is_pc) {
			op->mem.base_reg = M68K_REG_PC;
			op->address_mode = M68K_AM_PCI_INDEX_BASE_DISP;
		} else {
			op->mem.base_reg = M68K_REG_A0 + (instruction & 7);
		}
	} else {
		if (is_pc) {
			op->mem.base_reg = M68K_REG_PC;
			op->address_mode = M68K_AM_PCI_INDEX_8_BIT_DISP;
		} else {
			op->mem.base_reg = M68K_REG_A0 + (instruction & 7);
			op->address_mode = M68K_AM_AREGI_INDEX_8_BIT_DISP;
		}

		op->mem.disp = (int8_t)(extension & 0xff);
	}

	if (EXT_INDEX_SCALE(extension)) {
		op->mem.scale = 1 << EXT_INDEX_SCALE(extension);
	}
}

/* Make string of effective address mode */
static void get_ea_mode_op(m68k_info *info, cs_m68k_op* op, uint32_t instruction, uint32_t size)
{
	// default to memory

	op->type = M68K_OP_MEM;

	switch (instruction & 0x3f) {
		case 0x00: case 0x01: case 0x02: case 0x03: case 0x04: case 0x05: case 0x06: case 0x07:
			/* data register direct */
			op->address_mode = M68K_AM_REG_DIRECT_DATA;
			op->reg = M68K_REG_D0 + (instruction & 7);
			op->type = M68K_OP_REG;
			break;

		case 0x08: case 0x09: case 0x0a: case 0x0b: case 0x0c: case 0x0d: case 0x0e: case 0x0f:
			/* address register direct */
			op->address_mode = M68K_AM_REG_DIRECT_ADDR;
			op->reg = M68K_REG_A0 + (instruction & 7);
			op->type = M68K_OP_REG;
			break;

		case 0x10: case 0x11: case 0x12: case 0x13: case 0x14: case 0x15: case 0x16: case 0x17:
			/* address register indirect */
			op->address_mode = M68K_AM_REGI_ADDR;
			op->reg = M68K_REG_A0 + (instruction & 7);
			break;

		case 0x18: case 0x19: case 0x1a: case 0x1b: case 0x1c: case 0x1d: case 0x1e: case 0x1f:
			/* address register indirect with postincrement */
			op->address_mode = M68K_AM_REGI_ADDR_POST_INC;
			op->reg = M68K_REG_A0 + (instruction & 7);
			break;

		case 0x20: case 0x21: case 0x22: case 0x23: case 0x24: case 0x25: case 0x26: case 0x27:
			/* address register indirect with predecrement */
			op->address_mode = M68K_AM_REGI_ADDR_PRE_DEC;
			op->reg = M68K_REG_A0 + (instruction & 7);
			break;

		case 0x28: case 0x29: case 0x2a: case 0x2b: case 0x2c: case 0x2d: case 0x2e: case 0x2f:
			/* address register indirect with displacement*/
			op->address_mode = M68K_AM_REGI_ADDR_DISP;
			op->mem.base_reg = M68K_REG_A0 + (instruction & 7);
			op->mem.disp = (int16_t)read_imm_16(info);
			break;

		case 0x30: case 0x31: case 0x32: case 0x33: case 0x34: case 0x35: case 0x36: case 0x37:
			/* address register indirect with index */
			get_with_index_address_mode(info, op, instruction, size, false);
			break;

		case 0x38:
			/* absolute short address */
			op->address_mode = M68K_AM_ABSOLUTE_DATA_SHORT;
			op->imm = read_imm_16(info);
			break;

		case 0x39:
			/* absolute long address */
			op->address_mode = M68K_AM_ABSOLUTE_DATA_LONG;
			op->imm = read_imm_32(info);
			break;

		case 0x3a:
			/* program counter with displacement */
			op->address_mode = M68K_AM_PCI_DISP;
			op->mem.disp = (int16_t)read_imm_16(info);
			break;

		case 0x3b:
			/* program counter with index */
			get_with_index_address_mode(info, op, instruction, size, true);
			break;

		case 0x3c:
			op->address_mode = M68K_AM_IMMEDIATE;
			op->type = M68K_OP_IMM;

			if (size == 1)
				op->imm = read_imm_8(info) & 0xff;
			else if (size == 2)
				op->imm = read_imm_16(info) & 0xffff;
			else if (size == 4)
				op->imm = read_imm_32(info);
			else
				op->imm = read_imm_64(info);

			break;

		default:
			break;
	}
}

static void set_insn_group(m68k_info *info, m68k_group_type group)
{
	info->groups[info->groups_count++] = (uint8_t)group;
}

static cs_m68k* build_init_op(m68k_info *info, int opcode, int count, int size)
{
	cs_m68k* ext;

	MCInst_setOpcode(info->inst, opcode);

	ext = &info->extension;

	ext->op_count = (uint8_t)count;
	ext->op_size.type = M68K_SIZE_TYPE_CPU;
	ext->op_size.cpu_size = size;

	return ext;
}

static void build_re_gen_1(m68k_info *info, bool isDreg, int opcode, uint8_t size)
{
	cs_m68k_op* op0;
	cs_m68k_op* op1;
	cs_m68k* ext = build_init_op(info, opcode, 2, size);

	op0 = &ext->operands[0];
	op1 = &ext->operands[1];

	if (isDreg) {
		op0->address_mode = M68K_AM_REG_DIRECT_DATA;
		op0->reg = M68K_REG_D0 + ((info->ir >> 9 ) & 7);
	} else {
		op0->address_mode = M68K_AM_REG_DIRECT_ADDR;
		op0->reg = M68K_REG_A0 + ((info->ir >> 9 ) & 7);
	}

	get_ea_mode_op(info, op1, info->ir, size);
}

static void build_re_1(m68k_info *info, int opcode, uint8_t size)
{
	build_re_gen_1(info, true, opcode, size);
}

static void build_er_gen_1(m68k_info *info, bool isDreg, int opcode, uint8_t size)
{
	cs_m68k_op* op0;
	cs_m68k_op* op1;
	cs_m68k* ext = build_init_op(info, opcode, 2, size);

	op0 = &ext->operands[0];
	op1 = &ext->operands[1];

	get_ea_mode_op(info, op0, info->ir, size);

	if (isDreg) {
		op1->address_mode = M68K_AM_REG_DIRECT_DATA;
		op1->reg = M68K_REG_D0 + ((info->ir >> 9) & 7);
	} else {
		op1->address_mode = M68K_AM_REG_DIRECT_ADDR;
		op1->reg = M68K_REG_A0 + ((info->ir >> 9) & 7);
	}
}

static void build_rr(m68k_info *info, int opcode, uint8_t size, int imm)
{
	cs_m68k_op* op0;
	cs_m68k_op* op1;
	cs_m68k_op* op2;
	cs_m68k* ext = build_init_op(info, opcode, 2, size);

	op0 = &ext->operands[0];
	op1 = &ext->operands[1];
	op2 = &ext->operands[2];

	op0->address_mode = M68K_AM_REG_DIRECT_DATA;
	op0->reg = M68K_REG_D0 + (info->ir & 7);

	op1->address_mode = M68K_AM_REG_DIRECT_DATA;
	op1->reg = M68K_REG_D0 + ((info->ir >> 9) & 7);

	if (imm > 0) {
		ext->op_count = 3;
		op2->type = M68K_OP_IMM;
		op2->address_mode = M68K_AM_IMMEDIATE;
		op2->imm = imm;
	}
}

static void build_r(m68k_info *info, int opcode, uint8_t size)
{
	cs_m68k_op* op0;
	cs_m68k_op* op1;
	cs_m68k* ext = build_init_op(info, opcode, 2, size);

	op0 = &ext->operands[0];
	op1 = &ext->operands[1];

	op0->address_mode = M68K_AM_REG_DIRECT_DATA;
	op0->reg = M68K_REG_D0 + ((info->ir >> 9) & 7);

	op1->address_mode = M68K_AM_REG_DIRECT_DATA;
	op1->reg = M68K_REG_D0 + (info->ir & 7);
}

static void build_imm_ea(m68k_info *info, int opcode, uint8_t size, int imm)
{
	cs_m68k_op* op0;
	cs_m68k_op* op1;
	cs_m68k* ext = build_init_op(info, opcode, 2, size);

	op0 = &ext->operands[0];
	op1 = &ext->operands[1];

	op0->type = M68K_OP_IMM;
	op0->address_mode = M68K_AM_IMMEDIATE;
	op0->imm = imm;

	get_ea_mode_op(info, op1, info->ir, size);
}

static void build_3bit_d(m68k_info *info, int opcode, int size)
{
	cs_m68k_op* op0;
	cs_m68k_op* op1;
	cs_m68k* ext = build_init_op(info, opcode, 2, size);

	op0 = &ext->operands[0];
	op1 = &ext->operands[1];

	op0->type = M68K_OP_IMM;
	op0->address_mode = M68K_AM_IMMEDIATE;
	op0->imm = g_3bit_qdata_table[(info->ir >> 9) & 7];

	op1->address_mode = M68K_AM_REG_DIRECT_DATA;
	op1->reg = M68K_REG_D0 + (info->ir & 7);
}

static void build_3bit_ea(m68k_info *info, int opcode, int size)
{
	cs_m68k_op* op0;
	cs_m68k_op* op1;
	cs_m68k* ext = build_init_op(info, opcode, 2, size);

	op0 = &ext->operands[0];
	op1 = &ext->operands[1];

	op0->type = M68K_OP_IMM;
	op0->address_mode = M68K_AM_IMMEDIATE;
	op0->imm = g_3bit_qdata_table[(info->ir >> 9) & 7];

	get_ea_mode_op(info, op1, info->ir, size);
}

static void build_mm(m68k_info *info, int opcode, uint8_t size, int imm)
{
	cs_m68k_op* op0;
	cs_m68k_op* op1;
	cs_m68k_op* op2;
	cs_m68k* ext = build_init_op(info, opcode, 2, size);

	op0 = &ext->operands[0];
	op1 = &ext->operands[1];
	op2 = &ext->operands[2];

	op0->address_mode = M68K_AM_REGI_ADDR_PRE_DEC;
	op0->reg = M68K_REG_A0 + (info->ir & 7);

	op1->address_mode = M68K_AM_REGI_ADDR_PRE_DEC;
	op1->reg = M68K_REG_A0 + ((info->ir >> 9) & 7);

	if (imm > 0) {
		ext->op_count = 3;
		op2->type = M68K_OP_IMM;
		op2->address_mode = M68K_AM_IMMEDIATE;
		op2->imm = imm;
	}
}

static void build_ea(m68k_info *info, int opcode, uint8_t size)
{
	cs_m68k* ext = build_init_op(info, opcode, 1, size);
	get_ea_mode_op(info, &ext->operands[0], info->ir, size);
}

static void build_ea_a(m68k_info *info, int opcode, uint8_t size)
{
	cs_m68k_op* op0;
	cs_m68k_op* op1;
	cs_m68k* ext = build_init_op(info, opcode, 2, size);

	op0 = &ext->operands[0];
	op1 = &ext->operands[1];

	get_ea_mode_op(info, op0, info->ir, size);

	op1->address_mode = M68K_AM_REG_DIRECT_ADDR;
	op1->reg = M68K_REG_A0 + ((info->ir >> 9) & 7);
}

static void build_ea_ea(m68k_info *info, int opcode, int size)
{
	cs_m68k_op* op0;
	cs_m68k_op* op1;
	cs_m68k* ext = build_init_op(info, opcode, 2, size);

	op0 = &ext->operands[0];
	op1 = &ext->operands[1];

	get_ea_mode_op(info, op0, info->ir, size);
	get_ea_mode_op(info, op1, (((info->ir>>9) & 7) | ((info->ir>>3) & 0x38)), size);
}

static void build_pi_pi(m68k_info *info, int opcode, int size)
{
	cs_m68k_op* op0;
	cs_m68k_op* op1;
	cs_m68k* ext = build_init_op(info, opcode, 2, size);

	op0 = &ext->operands[0];
	op1 = &ext->operands[1];

	op0->address_mode = M68K_AM_REGI_ADDR_POST_INC;
	op0->reg = M68K_REG_A0 + (info->ir & 7);

	op1->address_mode = M68K_AM_REGI_ADDR_POST_INC;
	op1->reg = M68K_REG_A0 + ((info->ir >> 9) & 7);
}

static void build_imm_special_reg(m68k_info *info, int opcode, int imm, int size, m68k_reg reg)
{
	cs_m68k_op* op0;
	cs_m68k_op* op1;
	cs_m68k* ext = build_init_op(info, opcode, 2, size);

	op0 = &ext->operands[0];
	op1 = &ext->operands[1];

	op0->type = M68K_OP_IMM;
	op0->address_mode = M68K_AM_IMMEDIATE;
	op0->imm = imm;

	op1->address_mode = M68K_AM_NONE;
	op1->reg = reg;
}

static void build_relative_branch(m68k_info *info, int opcode, int size, int displacement)
{
	cs_m68k_op* op;
	cs_m68k* ext = build_init_op(info, opcode, 1, size);

	op = &ext->operands[0];

	op->type = M68K_OP_BR_DISP;
	op->address_mode = M68K_AM_BRANCH_DISPLACEMENT;
	op->br_disp.disp = displacement;
	op->br_disp.disp_size = size;

	set_insn_group(info, M68K_GRP_JUMP);
	set_insn_group(info, M68K_GRP_BRANCH_RELATIVE);
}

static void build_absolute_jump_with_immediate(m68k_info *info, int opcode, int size, int immediate)
{
	cs_m68k_op* op;
	cs_m68k* ext = build_init_op(info, opcode, 1, size);

	op = &ext->operands[0];

	op->type = M68K_OP_IMM;
	op->address_mode = M68K_AM_IMMEDIATE;
	op->imm = immediate;

	set_insn_group(info, M68K_GRP_JUMP);
}

static void build_bcc(m68k_info *info, int size, int displacement)
{
	build_relative_branch(info, s_branch_lut[(info->ir >> 8) & 0xf], size, displacement);
}

static void build_trap(m68k_info *info, int size, int immediate)
{
	build_absolute_jump_with_immediate(info, s_trap_lut[(info->ir >> 8) & 0xf], size, immediate);
}

static void build_dbxx(m68k_info *info, int opcode, int size, int displacement)
{
	cs_m68k_op* op0;
	cs_m68k_op* op1;
	cs_m68k* ext = build_init_op(info, opcode, 2, size);

	op0 = &ext->operands[0];
	op1 = &ext->operands[1];

	op0->address_mode = M68K_AM_REG_DIRECT_DATA;
	op0->reg = M68K_REG_D0 + (info->ir & 7);

	op1->type = M68K_OP_BR_DISP;
	op1->address_mode = M68K_AM_BRANCH_DISPLACEMENT;
	op1->br_disp.disp = displacement;
	op1->br_disp.disp_size = M68K_OP_BR_DISP_SIZE_LONG;

	set_insn_group(info, M68K_GRP_JUMP);
	set_insn_group(info, M68K_GRP_BRANCH_RELATIVE);
}

static void build_dbcc(m68k_info *info, int size, int displacement)
{
	build_dbxx(info, s_dbcc_lut[(info->ir >> 8) & 0xf], size, displacement);
}

static void build_d_d_ea(m68k_info *info, int opcode, int size)
{
	cs_m68k_op* op0;
	cs_m68k_op* op1;
	cs_m68k_op* op2;
	uint32_t extension = read_imm_16(info);
	cs_m68k* ext = build_init_op(info, opcode, 3, size);

	op0 = &ext->operands[0];
	op1 = &ext->operands[1];
	op2 = &ext->operands[2];

	op0->address_mode = M68K_AM_REG_DIRECT_DATA;
	op0->reg = M68K_REG_D0 + (extension & 7);

	op1->address_mode = M68K_AM_REG_DIRECT_DATA;
	op1->reg = M68K_REG_D0 + ((extension >> 6) & 7);

	get_ea_mode_op(info, op2, info->ir, size);
}

static void build_bitfield_ins(m68k_info *info, int opcode, int has_d_arg)
{
	uint8_t offset;
	uint8_t width;
	cs_m68k_op* op_ea;
	cs_m68k_op* op1;
	cs_m68k* ext = build_init_op(info, opcode, 1, 0);
	uint32_t extension = read_imm_16(info);

	op_ea = &ext->operands[0];
	op1 = &ext->operands[1];

	if (BIT_B(extension))
		offset = (extension >> 6) & 7;
	else
		offset = (extension >> 6) & 31;

	if (BIT_5(extension))
		width = extension & 7;
	else
		width = (uint8_t)g_5bit_data_table[extension & 31];

	if (has_d_arg) {
		ext->op_count = 2;
		op1->address_mode = M68K_AM_REG_DIRECT_DATA;
		op1->reg = M68K_REG_D0 + ((extension >> 12) & 7);
	}

	get_ea_mode_op(info, op_ea, info->ir, 1);

	op_ea->mem.bitfield = 1;
	op_ea->mem.width = width;
	op_ea->mem.offset = offset;
}

static void build_d(m68k_info *info, int opcode, int size)
{
	cs_m68k* ext = build_init_op(info, opcode, 1, size);
	cs_m68k_op* op;

	op = &ext->operands[0];

	op->address_mode = M68K_AM_REG_DIRECT_DATA;
	op->reg = M68K_REG_D0 + (info->ir & 7);
}

static uint16_t reverse_bits(uint32_t v)
{
	uint32_t r = v; // r will be reversed bits of v; first get LSB of v
	uint32_t s = 16 - 1; // extra shift needed at end

	for (v >>= 1; v; v >>= 1) {
		r <<= 1;
		r |= v & 1;
		s--;
	}

	return r <<= s; // shift when v's highest bits are zero
}

static uint8_t reverse_bits_8(uint32_t v)
{
	uint32_t r = v; // r will be reversed bits of v; first get LSB of v
	uint32_t s = 8 - 1; // extra shift needed at end

	for (v >>= 1; v; v >>= 1) {
		r <<= 1;
		r |= v & 1;
		s--;
	}

	return r <<= s; // shift when v's highest bits are zero
}


static void build_movem_re(m68k_info *info, int opcode, int size)
{
	cs_m68k_op* op0;
	cs_m68k_op* op1;
	cs_m68k* ext = build_init_op(info, opcode, 2, size);

	op0 = &ext->operands[0];
	op1 = &ext->operands[1];

	op0->type = M68K_OP_REG_BITS;
	op0->register_bits = read_imm_16(info);

	get_ea_mode_op(info, op1, info->ir, size);

	if (op1->address_mode == M68K_AM_REGI_ADDR_PRE_DEC)
		op0->register_bits = reverse_bits(op0->register_bits);
}

static void build_movem_er(m68k_info *info, int opcode, int size)
{
	cs_m68k_op* op0;
	cs_m68k_op* op1;
	cs_m68k* ext = build_init_op(info, opcode, 2, size);

	op0 = &ext->operands[0];
	op1 = &ext->operands[1];

	op1->type = M68K_OP_REG_BITS;
	op1->register_bits = read_imm_16(info);

	get_ea_mode_op(info, op0, info->ir, size);
}

static void build_imm(m68k_info *info, int opcode, int data)
{
	cs_m68k_op* op;
	cs_m68k* ext = build_init_op(info, opcode, 1, 0);

	MCInst_setOpcode(info->inst, opcode);

	op = &ext->operands[0];

	op->type = M68K_OP_IMM;
	op->address_mode = M68K_AM_IMMEDIATE;
	op->imm = data;
}

static void build_illegal(m68k_info *info, int data)
{
	build_imm(info, M68K_INS_ILLEGAL, data);
}

static void build_invalid(m68k_info *info, int data)
{
	build_imm(info, M68K_INS_INVALID, data);
}

static void build_cas2(m68k_info *info, int size)
{
	uint32_t word3;
	uint32_t extension;
	cs_m68k_op* op0;
	cs_m68k_op* op1;
	cs_m68k_op* op2;
	cs_m68k* ext = build_init_op(info, M68K_INS_CAS2, 3, size);
	int reg_0, reg_1;

	/* cas2 is the only 3 words instruction, word2 and word3 have the same motif bits to check */
	word3 = peek_imm_32(info) & 0xffff;
	if (!instruction_is_valid(info, word3))
		return;

	op0 = &ext->operands[0];
	op1 = &ext->operands[1];
	op2 = &ext->operands[2];

	extension = read_imm_32(info);

	op0->address_mode = M68K_AM_NONE;
	op0->type = M68K_OP_REG_PAIR;
	op0->reg_pair.reg_0 = ((extension >> 16) & 7) + M68K_REG_D0;
	op0->reg_pair.reg_1 = (extension & 7) + M68K_REG_D0;

	op1->address_mode = M68K_AM_NONE;
	op1->type = M68K_OP_REG_PAIR;
	op1->reg_pair.reg_0 = ((extension >> 22) & 7) + M68K_REG_D0;
	op1->reg_pair.reg_1 = ((extension >> 6) & 7) + M68K_REG_D0;

	reg_0 = (extension >> 28) & 7;
	reg_1 = (extension >> 12) & 7;

	op2->address_mode = M68K_AM_NONE;
	op2->type = M68K_OP_REG_PAIR;
	op2->reg_pair.reg_0 = reg_0 + (BIT_1F(extension) ? 8 : 0) + M68K_REG_D0;
	op2->reg_pair.reg_1 = reg_1 + (BIT_F(extension) ? 8 : 0) + M68K_REG_D0;
}

static void build_chk2_cmp2(m68k_info *info, int size)
{
	cs_m68k_op* op0;
	cs_m68k_op* op1;
	cs_m68k* ext = build_init_op(info, M68K_INS_CHK2, 2, size);

	uint32_t extension = read_imm_16(info);

	if (BIT_B(extension))
		MCInst_setOpcode(info->inst, M68K_INS_CHK2);
	else
		MCInst_setOpcode(info->inst, M68K_INS_CMP2);

	op0 = &ext->operands[0];
	op1 = &ext->operands[1];

	get_ea_mode_op(info, op0, info->ir, size);

	op1->address_mode = M68K_AM_NONE;
	op1->type = M68K_OP_REG;
	op1->reg = (BIT_F(extension) ? M68K_REG_A0 : M68K_REG_D0) + ((extension >> 12) & 7);
}

static void build_move16(m68k_info *info, int data[2], int modes[2])
{
	cs_m68k* ext = build_init_op(info, M68K_INS_MOVE16, 2, 0);
	int i;

	for (i = 0; i < 2; ++i) {
		cs_m68k_op* op = &ext->operands[i];
		const int d = data[i];
		const int m = modes[i];

		op->type = M68K_OP_MEM;

		if (m == M68K_AM_REGI_ADDR_POST_INC || m == M68K_AM_REG_DIRECT_ADDR) {
			op->address_mode = m;
			op->reg = M68K_REG_A0 + d;
		} else {
			op->address_mode = m;
			op->imm = d;
		}
	}
}

static void build_link(m68k_info *info, int disp, int size)
{
	cs_m68k_op* op0;
	cs_m68k_op* op1;
	cs_m68k* ext = build_init_op(info, M68K_INS_LINK, 2, size);

	op0 = &ext->operands[0];
	op1 = &ext->operands[1];

	op0->address_mode = M68K_AM_NONE;
	op0->reg = M68K_REG_A0 + (info->ir & 7);

	op1->address_mode = M68K_AM_IMMEDIATE;
	op1->type = M68K_OP_IMM;
	op1->imm = disp;
}

static void build_cpush_cinv(m68k_info *info, int op_offset)
{
	cs_m68k_op* op0;
	cs_m68k_op* op1;
	cs_m68k* ext = build_init_op(info, M68K_INS_INVALID, 2, 0);

	switch ((info->ir >> 3) & 3) { // scope
		// Invalid
		case 0:
			d68000_invalid(info);
			return;
			// Line
		case 1:
			MCInst_setOpcode(info->inst, op_offset + 0);
			break;
			// Page
		case 2:
			MCInst_setOpcode(info->inst, op_offset + 1);
			break;
			// All
		case 3:
			ext->op_count = 1;
			MCInst_setOpcode(info->inst, op_offset + 2);
			break;
	}

	op0 = &ext->operands[0];
	op1 = &ext->operands[1];

	op0->address_mode = M68K_AM_IMMEDIATE;
	op0->type = M68K_OP_IMM;
	op0->imm = (info->ir >> 6) & 3;

	op1->type = M68K_OP_MEM;
	op1->address_mode = M68K_AM_REG_DIRECT_ADDR;
	op1->imm = M68K_REG_A0 + (info->ir & 7);
}

static void build_movep_re(m68k_info *info, int size)
{
	cs_m68k_op* op0;
	cs_m68k_op* op1;
	cs_m68k* ext = build_init_op(info, M68K_INS_MOVEP, 2, size);

	op0 = &ext->operands[0];
	op1 = &ext->operands[1];

	op0->reg = M68K_REG_D0 + ((info->ir >> 9) & 7);

	op1->address_mode = M68K_AM_REGI_ADDR_DISP;
	op1->type = M68K_OP_MEM;
	op1->mem.base_reg = M68K_REG_A0 + (info->ir & 7);
	op1->mem.disp = (int16_t)read_imm_16(info);
}

static void build_movep_er(m68k_info *info, int size)
{
	cs_m68k_op* op0;
	cs_m68k_op* op1;
	cs_m68k* ext = build_init_op(info, M68K_INS_MOVEP, 2, size);

	op0 = &ext->operands[0];
	op1 = &ext->operands[1];

	op0->address_mode = M68K_AM_REGI_ADDR_DISP;
	op0->type = M68K_OP_MEM;
	op0->mem.base_reg = M68K_REG_A0 + (info->ir & 7);
	op0->mem.disp = (int16_t)read_imm_16(info);

	op1->reg = M68K_REG_D0 + ((info->ir >> 9) & 7);
}

static void build_moves(m68k_info *info, int size)
{
	cs_m68k_op* op0;
	cs_m68k_op* op1;
	cs_m68k* ext = build_init_op(info, M68K_INS_MOVES, 2, size);
	uint32_t extension = read_imm_16(info);

	op0 = &ext->operands[0];
	op1 = &ext->operands[1];

	if (BIT_B(extension)) {
		op0->reg = (BIT_F(extension) ? M68K_REG_A0 : M68K_REG_D0) + ((extension >> 12) & 7);
		get_ea_mode_op(info, op1, info->ir, size);
	} else {
		get_ea_mode_op(info, op0, info->ir, size);
		op1->reg = (BIT_F(extension) ? M68K_REG_A0 : M68K_REG_D0) + ((extension >> 12) & 7);
	}
}

static void build_er_1(m68k_info *info, int opcode, uint8_t size)
{
	build_er_gen_1(info, true, opcode, size);
}

/* ======================================================================== */
/* ========================= INSTRUCTION HANDLERS ========================= */
/* ======================================================================== */
/* Instruction handler function names follow this convention:
 *
 * d68000_NAME_EXTENSIONS(void)
 * where NAME is the name of the opcode it handles and EXTENSIONS are any
 * extensions for special instances of that opcode.
 *
 * Examples:
 *   d68000_add_er_8(): add opcode, from effective address to register,
 *                      size = byte
 *
 *   d68000_asr_s_8(): arithmetic shift right, static count, size = byte
 *
 *
 * Common extensions:
 * 8   : size = byte
 * 16  : size = word
 * 32  : size = long
 * rr  : register to register
 * mm  : memory to memory
 * r   : register
 * s   : static
 * er  : effective address -> register
 * re  : register -> effective address
 * ea  : using effective address mode of operation
 * d   : data register direct
 * a   : address register direct
 * ai  : address register indirect
 * pi  : address register indirect with postincrement
 * pd  : address register indirect with predecrement
 * di  : address register indirect with displacement
 * ix  : address register indirect with index
 * aw  : absolute word
 * al  : absolute long
 */


static void d68000_invalid(m68k_info *info)
{
	build_invalid(info, info->ir);
}

static void d68000_illegal(m68k_info *info)
{
	build_illegal(info, info->ir);
}

static void d68000_1010(m68k_info *info)
{
	build_invalid(info, info->ir);
}

static void d68000_1111(m68k_info *info)
{
	build_invalid(info, info->ir);
}

static void d68000_abcd_rr(m68k_info *info)
{
	build_rr(info, M68K_INS_ABCD, 1, 0);
}

static void d68000_abcd_mm(m68k_info *info)
{
	build_mm(info, M68K_INS_ABCD, 1, 0);
}

static void d68000_add_er_8(m68k_info *info)
{
	build_er_1(info, M68K_INS_ADD, 1);
}

static void d68000_add_er_16(m68k_info *info)
{
	build_er_1(info, M68K_INS_ADD, 2);
}

static void d68000_add_er_32(m68k_info *info)
{
	build_er_1(info, M68K_INS_ADD, 4);
}

static void d68000_add_re_8(m68k_info *info)
{
	build_re_1(info, M68K_INS_ADD, 1);
}

static void d68000_add_re_16(m68k_info *info)
{
	build_re_1(info, M68K_INS_ADD, 2);
}

static void d68000_add_re_32(m68k_info *info)
{
	build_re_1(info, M68K_INS_ADD, 4);
}

static void d68000_adda_16(m68k_info *info)
{
	build_ea_a(info, M68K_INS_ADDA, 2);
}

static void d68000_adda_32(m68k_info *info)
{
	build_ea_a(info, M68K_INS_ADDA, 4);
}

static void d68000_addi_8(m68k_info *info)
{
	build_imm_ea(info, M68K_INS_ADDI, 1, read_imm_8(info));
}

static void d68000_addi_16(m68k_info *info)
{
	build_imm_ea(info, M68K_INS_ADDI, 2, read_imm_16(info));
}

static void d68000_addi_32(m68k_info *info)
{
	build_imm_ea(info, M68K_INS_ADDI, 4, read_imm_32(info));
}

static void d68000_addq_8(m68k_info *info)
{
	build_3bit_ea(info, M68K_INS_ADDQ, 1);
}

static void d68000_addq_16(m68k_info *info)
{
	build_3bit_ea(info, M68K_INS_ADDQ, 2);
}

static void d68000_addq_32(m68k_info *info)
{
	build_3bit_ea(info, M68K_INS_ADDQ, 4);
}

static void d68000_addx_rr_8(m68k_info *info)
{
	build_rr(info, M68K_INS_ADDX, 1, 0);
}

static void d68000_addx_rr_16(m68k_info *info)
{
	build_rr(info, M68K_INS_ADDX, 2, 0);
}

static void d68000_addx_rr_32(m68k_info *info)
{
	build_rr(info, M68K_INS_ADDX, 4, 0);
}

static void d68000_addx_mm_8(m68k_info *info)
{
	build_mm(info, M68K_INS_ADDX, 1, 0);
}

static void d68000_addx_mm_16(m68k_info *info)
{
	build_mm(info, M68K_INS_ADDX, 2, 0);
}

static void d68000_addx_mm_32(m68k_info *info)
{
	build_mm(info, M68K_INS_ADDX, 4, 0);
}

static void d68000_and_er_8(m68k_info *info)
{
	build_er_1(info, M68K_INS_AND, 1);
}

static void d68000_and_er_16(m68k_info *info)
{
	build_er_1(info, M68K_INS_AND, 2);
}

static void d68000_and_er_32(m68k_info *info)
{
	build_er_1(info, M68K_INS_AND, 4);
}

static void d68000_and_re_8(m68k_info *info)
{
	build_re_1(info, M68K_INS_AND, 1);
}

static void d68000_and_re_16(m68k_info *info)
{
	build_re_1(info, M68K_INS_AND, 2);
}

static void d68000_and_re_32(m68k_info *info)
{
	build_re_1(info, M68K_INS_AND, 4);
}

static void d68000_andi_8(m68k_info *info)
{
	build_imm_ea(info, M68K_INS_ANDI, 1, read_imm_8(info));
}

static void d68000_andi_16(m68k_info *info)
{
	build_imm_ea(info, M68K_INS_ANDI, 2, read_imm_16(info));
}

static void d68000_andi_32(m68k_info *info)
{
	build_imm_ea(info, M68K_INS_ANDI, 4, read_imm_32(info));
}

static void d68000_andi_to_ccr(m68k_info *info)
{
	build_imm_special_reg(info, M68K_INS_ANDI, read_imm_8(info), 1, M68K_REG_CCR);
}

static void d68000_andi_to_sr(m68k_info *info)
{
	build_imm_special_reg(info, M68K_INS_ANDI, read_imm_16(info), 2, M68K_REG_SR);
}

static void d68000_asr_s_8(m68k_info *info)
{
	build_3bit_d(info, M68K_INS_ASR, 1);
}

static void d68000_asr_s_16(m68k_info *info)
{
	build_3bit_d(info, M68K_INS_ASR, 2);
}

static void d68000_asr_s_32(m68k_info *info)
{
	build_3bit_d(info, M68K_INS_ASR, 4);
}

static void d68000_asr_r_8(m68k_info *info)
{
	build_r(info, M68K_INS_ASR, 1);
}

static void d68000_asr_r_16(m68k_info *info)
{
	build_r(info, M68K_INS_ASR, 2);
}

static void d68000_asr_r_32(m68k_info *info)
{
	build_r(info, M68K_INS_ASR, 4);
}

static void d68000_asr_ea(m68k_info *info)
{
	build_ea(info, M68K_INS_ASR, 2);
}

static void d68000_asl_s_8(m68k_info *info)
{
	build_3bit_d(info, M68K_INS_ASL, 1);
}

static void d68000_asl_s_16(m68k_info *info)
{
	build_3bit_d(info, M68K_INS_ASL, 2);
}

static void d68000_asl_s_32(m68k_info *info)
{
	build_3bit_d(info, M68K_INS_ASL, 4);
}

static void d68000_asl_r_8(m68k_info *info)
{
	build_r(info, M68K_INS_ASL, 1);
}

static void d68000_asl_r_16(m68k_info *info)
{
	build_r(info, M68K_INS_ASL, 2);
}

static void d68000_asl_r_32(m68k_info *info)
{
	build_r(info, M68K_INS_ASL, 4);
}

static void d68000_asl_ea(m68k_info *info)
{
	build_ea(info, M68K_INS_ASL, 2);
}

static void d68000_bcc_8(m68k_info *info)
{
	build_bcc(info, 1, make_int_8(info->ir));
}

static void d68000_bcc_16(m68k_info *info)
{
	build_bcc(info, 2, make_int_16(read_imm_16(info)));
}

static void d68020_bcc_32(m68k_info *info)
{
	LIMIT_CPU_TYPES(info, M68020_PLUS);
	build_bcc(info, 4, read_imm_32(info));
}

static void d68000_bchg_r(m68k_info *info)
{
	build_re_1(info, M68K_INS_BCHG, 1);
}

static void d68000_bchg_s(m68k_info *info)
{
	build_imm_ea(info, M68K_INS_BCHG, 1, read_imm_8(info));
}

static void d68000_bclr_r(m68k_info *info)
{
	build_re_1(info, M68K_INS_BCLR, 1);
}

static void d68000_bclr_s(m68k_info *info)
{
	build_imm_ea(info, M68K_INS_BCLR, 1, read_imm_8(info));
}

static void d68010_bkpt(m68k_info *info)
{
	LIMIT_CPU_TYPES(info, M68010_PLUS);
	build_absolute_jump_with_immediate(info, M68K_INS_BKPT, 0, info->ir & 7);
}

static void d68020_bfchg(m68k_info *info)
{
	LIMIT_CPU_TYPES(info, M68020_PLUS);
	build_bitfield_ins(info, M68K_INS_BFCHG, false);
}


static void d68020_bfclr(m68k_info *info)
{
	LIMIT_CPU_TYPES(info, M68020_PLUS);
	build_bitfield_ins(info, M68K_INS_BFCLR, false);
}

static void d68020_bfexts(m68k_info *info)
{
	LIMIT_CPU_TYPES(info, M68020_PLUS);
	build_bitfield_ins(info, M68K_INS_BFEXTS, true);
}

static void d68020_bfextu(m68k_info *info)
{
	LIMIT_CPU_TYPES(info, M68020_PLUS);
	build_bitfield_ins(info, M68K_INS_BFEXTU, true);
}

static void d68020_bfffo(m68k_info *info)
{
	LIMIT_CPU_TYPES(info, M68020_PLUS);
	build_bitfield_ins(info, M68K_INS_BFFFO, true);
}

static void d68020_bfins(m68k_info *info)
{
	cs_m68k* ext = &info->extension;
	cs_m68k_op temp;

	LIMIT_CPU_TYPES(info, M68020_PLUS);
	build_bitfield_ins(info, M68K_INS_BFINS, true);

	// a bit hacky but we need to flip the args on only this instruction

	temp = ext->operands[0];
	ext->operands[0] = ext->operands[1];
	ext->operands[1] = temp;
}

static void d68020_bfset(m68k_info *info)
{
	LIMIT_CPU_TYPES(info, M68020_PLUS);
	build_bitfield_ins(info, M68K_INS_BFSET, false);
}

static void d68020_bftst(m68k_info *info)
{
	build_bitfield_ins(info, M68K_INS_BFTST, false);
}

static void d68000_bra_8(m68k_info *info)
{
	build_relative_branch(info, M68K_INS_BRA, 1, make_int_8(info->ir));
}

static void d68000_bra_16(m68k_info *info)
{
	build_relative_branch(info, M68K_INS_BRA, 2, make_int_16(read_imm_16(info)));
}

static void d68020_bra_32(m68k_info *info)
{
	LIMIT_CPU_TYPES(info, M68020_PLUS);
	build_relative_branch(info, M68K_INS_BRA, 4, read_imm_32(info));
}

static void d68000_bset_r(m68k_info *info)
{
	build_re_1(info, M68K_INS_BSET, 1);
}

static void d68000_bset_s(m68k_info *info)
{
	build_imm_ea(info, M68K_INS_BSET, 1, read_imm_8(info));
}

static void d68000_bsr_8(m68k_info *info)
{
	build_relative_branch(info, M68K_INS_BSR, 1, make_int_8(info->ir));
}

static void d68000_bsr_16(m68k_info *info)
{
	build_relative_branch(info, M68K_INS_BSR, 2, make_int_16(read_imm_16(info)));
}

static void d68020_bsr_32(m68k_info *info)
{
	LIMIT_CPU_TYPES(info, M68020_PLUS);
	build_relative_branch(info, M68K_INS_BSR, 4, peek_imm_32(info));
}

static void d68000_btst_r(m68k_info *info)
{
	build_re_1(info, M68K_INS_BTST, 4);
}

static void d68000_btst_s(m68k_info *info)
{
	build_imm_ea(info, M68K_INS_BTST, 1, read_imm_8(info));
}

static void d68020_callm(m68k_info *info)
{
	LIMIT_CPU_TYPES(info, M68020_ONLY);
	build_imm_ea(info, M68K_INS_CALLM, 0, read_imm_8(info));
}

static void d68020_cas_8(m68k_info *info)
{
	LIMIT_CPU_TYPES(info, M68020_PLUS);
	build_d_d_ea(info, M68K_INS_CAS, 1);
}

static void d68020_cas_16(m68k_info *info)
{
	LIMIT_CPU_TYPES(info, M68020_PLUS);
	build_d_d_ea(info, M68K_INS_CAS, 2);
}

static void d68020_cas_32(m68k_info *info)
{
	LIMIT_CPU_TYPES(info, M68020_PLUS);
	build_d_d_ea(info, M68K_INS_CAS, 4);
}

static void d68020_cas2_16(m68k_info *info)
{
	build_cas2(info, 2);
}

static void d68020_cas2_32(m68k_info *info)
{
	build_cas2(info, 4);
}

static void d68000_chk_16(m68k_info *info)
{
	build_er_1(info, M68K_INS_CHK, 2);
}

static void d68020_chk_32(m68k_info *info)
{
	LIMIT_CPU_TYPES(info, M68020_PLUS);
	build_er_1(info, M68K_INS_CHK, 4);
}

static void d68020_chk2_cmp2_8(m68k_info *info)
{
	LIMIT_CPU_TYPES(info, M68020_PLUS);
	build_chk2_cmp2(info, 1);
}

static void d68020_chk2_cmp2_16(m68k_info *info)
{
	LIMIT_CPU_TYPES(info, M68020_PLUS);
	build_chk2_cmp2(info, 2);
}

static void d68020_chk2_cmp2_32(m68k_info *info)
{
	LIMIT_CPU_TYPES(info, M68020_PLUS);
	build_chk2_cmp2(info, 4);
}

static void d68040_cinv(m68k_info *info)
{
	LIMIT_CPU_TYPES(info, M68040_PLUS);
	build_cpush_cinv(info, M68K_INS_CINVL);
}

static void d68000_clr_8(m68k_info *info)
{
	build_ea(info, M68K_INS_CLR, 1);
}

static void d68000_clr_16(m68k_info *info)
{
	build_ea(info, M68K_INS_CLR, 2);
}

static void d68000_clr_32(m68k_info *info)
{
	build_ea(info, M68K_INS_CLR, 4);
}

static void d68000_cmp_8(m68k_info *info)
{
	build_er_1(info, M68K_INS_CMP, 1);
}

static void d68000_cmp_16(m68k_info *info)
{
	build_er_1(info, M68K_INS_CMP, 2);
}

static void d68000_cmp_32(m68k_info *info)
{
	build_er_1(info, M68K_INS_CMP, 4);
}

static void d68000_cmpa_16(m68k_info *info)
{
	build_ea_a(info, M68K_INS_CMPA, 2);
}

static void d68000_cmpa_32(m68k_info *info)
{
	build_ea_a(info, M68K_INS_CMPA, 4);
}

static void d68000_cmpi_8(m68k_info *info)
{
	build_imm_ea(info, M68K_INS_CMPI, 1, read_imm_8(info));
}

static void d68020_cmpi_pcdi_8(m68k_info *info)
{
	LIMIT_CPU_TYPES(info, M68010_PLUS);
	build_imm_ea(info, M68K_INS_CMPI, 1, read_imm_8(info));
}

static void d68020_cmpi_pcix_8(m68k_info *info)
{
	LIMIT_CPU_TYPES(info, M68010_PLUS);
	build_imm_ea(info, M68K_INS_CMPI, 1, read_imm_8(info));
}

static void d68000_cmpi_16(m68k_info *info)
{
	build_imm_ea(info, M68K_INS_CMPI, 2, read_imm_16(info));
}

static void d68020_cmpi_pcdi_16(m68k_info *info)
{
	LIMIT_CPU_TYPES(info, M68010_PLUS);
	build_imm_ea(info, M68K_INS_CMPI, 2, read_imm_16(info));
}

static void d68020_cmpi_pcix_16(m68k_info *info)
{
	LIMIT_CPU_TYPES(info, M68010_PLUS);
	build_imm_ea(info, M68K_INS_CMPI, 2, read_imm_16(info));
}

static void d68000_cmpi_32(m68k_info *info)
{
	build_imm_ea(info, M68K_INS_CMPI, 4, read_imm_32(info));
}

static void d68020_cmpi_pcdi_32(m68k_info *info)
{
	LIMIT_CPU_TYPES(info, M68010_PLUS);
	build_imm_ea(info, M68K_INS_CMPI, 4, read_imm_32(info));
}

static void d68020_cmpi_pcix_32(m68k_info *info)
{
	LIMIT_CPU_TYPES(info, M68010_PLUS);
	build_imm_ea(info, M68K_INS_CMPI, 4, read_imm_32(info));
}

static void d68000_cmpm_8(m68k_info *info)
{
	build_pi_pi(info, M68K_INS_CMPM, 1);
}

static void d68000_cmpm_16(m68k_info *info)
{
	build_pi_pi(info, M68K_INS_CMPM, 2);
}

static void d68000_cmpm_32(m68k_info *info)
{
	build_pi_pi(info, M68K_INS_CMPM, 4);
}

static void make_cpbcc_operand(cs_m68k_op* op, int size, int displacement)
{
	op->address_mode = M68K_AM_BRANCH_DISPLACEMENT;
	op->type = M68K_OP_BR_DISP;
	op->br_disp.disp = displacement;
	op->br_disp.disp_size = size;
}

static void d68020_cpbcc_16(m68k_info *info)
{
	cs_m68k_op* op0;
	cs_m68k* ext;
	LIMIT_CPU_TYPES(info, M68020_PLUS);

	// these are all in row with the extension so just doing a add here is fine
	info->inst->Opcode += (info->ir & 0x2f);

	ext = build_init_op(info, M68K_INS_FBF, 1, 2);
	op0 = &ext->operands[0];

	make_cpbcc_operand(op0, M68K_OP_BR_DISP_SIZE_WORD, make_int_16(read_imm_16(info)));

	set_insn_group(info, M68K_GRP_JUMP);
	set_insn_group(info, M68K_GRP_BRANCH_RELATIVE);
}

static void d68020_cpbcc_32(m68k_info *info)
{
	cs_m68k* ext;
	cs_m68k_op* op0;

	LIMIT_CPU_TYPES(info, M68020_PLUS);

	LIMIT_CPU_TYPES(info, M68020_PLUS);

	// these are all in row with the extension so just doing a add here is fine
	info->inst->Opcode += (info->ir & 0x2f);

	ext = build_init_op(info, M68K_INS_FBF, 1, 4);
	op0 = &ext->operands[0];

	make_cpbcc_operand(op0, M68K_OP_BR_DISP_SIZE_LONG, read_imm_32(info));

	set_insn_group(info, M68K_GRP_JUMP);
	set_insn_group(info, M68K_GRP_BRANCH_RELATIVE);
}

static void d68020_cpdbcc(m68k_info *info)
{
	cs_m68k* ext;
	cs_m68k_op* op0;
	cs_m68k_op* op1;
	uint32_t ext1, ext2;

	LIMIT_CPU_TYPES(info, M68020_PLUS);

	ext1 = read_imm_16(info);
	ext2 = read_imm_16(info);

	// these are all in row with the extension so just doing a add here is fine
	info->inst->Opcode += (ext1 & 0x2f);

	ext = build_init_op(info, M68K_INS_FDBF, 2, 0);
	op0 = &ext->operands[0];
	op1 = &ext->operands[1];

	op0->reg = M68K_REG_D0 + (info->ir & 7);

	make_cpbcc_operand(op1, M68K_OP_BR_DISP_SIZE_WORD, make_int_16(ext2) + 2);

	set_insn_group(info, M68K_GRP_JUMP);
	set_insn_group(info, M68K_GRP_BRANCH_RELATIVE);
}

static void fmove_fpcr(m68k_info *info, uint32_t extension)
{
	cs_m68k_op* special;
	cs_m68k_op* op_ea;

	int regsel = (extension >> 10) & 0x7;
	int dir = (extension >> 13) & 0x1;

	cs_m68k* ext = build_init_op(info, M68K_INS_FMOVE, 2, 4);

	special = &ext->operands[0];
	op_ea = &ext->operands[1];

	if (!dir) {
		cs_m68k_op* t = special;
		special = op_ea;
		op_ea = t;
	}

	get_ea_mode_op(info, op_ea, info->ir, 4);

	if (regsel & 4)
		special->reg = M68K_REG_FPCR;
	else if (regsel & 2)
		special->reg = M68K_REG_FPSR;
	else if (regsel & 1)
		special->reg = M68K_REG_FPIAR;
}

static void fmovem(m68k_info *info, uint32_t extension)
{
	cs_m68k_op* op_reglist;
	cs_m68k_op* op_ea;
	int dir = (extension >> 13) & 0x1;
	int mode = (extension >> 11) & 0x3;
	uint32_t reglist = extension & 0xff;
	cs_m68k* ext = build_init_op(info, M68K_INS_FMOVEM, 2, 0);

	op_reglist = &ext->operands[0];
	op_ea = &ext->operands[1];

	// flip args around

	if (!dir) {
		cs_m68k_op* t = op_reglist;
		op_reglist = op_ea;
		op_ea = t;
	}

	get_ea_mode_op(info, op_ea, info->ir, 0);

	switch (mode) {
		case 1 : // Dynamic list in dn register
			op_reglist->reg = M68K_REG_D0 + ((reglist >> 4) & 7);
			break;

		case 0 :
			op_reglist->address_mode = M68K_AM_NONE;
			op_reglist->type = M68K_OP_REG_BITS;
			op_reglist->register_bits = reglist << 16;
			break;

		case 2 : // Static list
			op_reglist->address_mode = M68K_AM_NONE;
			op_reglist->type = M68K_OP_REG_BITS;
			op_reglist->register_bits = ((uint32_t)reverse_bits_8(reglist)) << 16;
			break;
	}
}

static void d68020_cpgen(m68k_info *info)
{
	cs_m68k *ext;
	cs_m68k_op* op0;
	cs_m68k_op* op1;
	bool supports_single_op;
	uint32_t next;
	int rm, src, dst, opmode;


	LIMIT_CPU_TYPES(info, M68020_PLUS);

	supports_single_op = true;

	next = read_imm_16(info);

	rm = (next >> 14) & 0x1;
	src = (next >> 10) & 0x7;
	dst = (next >> 7) & 0x7;
	opmode = next & 0x3f;

	// special handling for fmovecr

	if (BITFIELD(info->ir, 5, 0) == 0 && BITFIELD(next, 15, 10) == 0x17) {
		cs_m68k_op* op0;
		cs_m68k_op* op1;
		cs_m68k* ext = build_init_op(info, M68K_INS_FMOVECR, 2, 0);

		op0 = &ext->operands[0];
		op1 = &ext->operands[1];

		op0->address_mode = M68K_AM_IMMEDIATE;
		op0->type = M68K_OP_IMM;
		op0->imm = next & 0x3f;

		op1->reg = M68K_REG_FP0 + ((next >> 7) & 7);

		return;
	}

	// deal with extended move stuff

	switch ((next >> 13) & 0x7) {
		// fmovem fpcr
		case 0x4:	// FMOVEM ea, FPCR
		case 0x5:	// FMOVEM FPCR, ea
			fmove_fpcr(info, next);
			return;

		// fmovem list
		case 0x6:
		case 0x7:
			fmovem(info, next);
			return;
	}

	// See comment bellow on why this is being done

	if ((next >> 6) & 1)
		opmode &= ~4;

	// special handling of some instructions here

	switch (opmode) {
		case 0x00: MCInst_setOpcode(info->inst, M68K_INS_FMOVE); supports_single_op = false; break;
		case 0x01: MCInst_setOpcode(info->inst, M68K_INS_FINT); break;
		case 0x02: MCInst_setOpcode(info->inst, M68K_INS_FSINH); break;
		case 0x03: MCInst_setOpcode(info->inst, M68K_INS_FINTRZ); break;
		case 0x04: MCInst_setOpcode(info->inst, M68K_INS_FSQRT); break;
		case 0x06: MCInst_setOpcode(info->inst, M68K_INS_FLOGNP1); break;
		case 0x08: MCInst_setOpcode(info->inst, M68K_INS_FETOXM1); break;
		case 0x09: MCInst_setOpcode(info->inst, M68K_INS_FATANH); break;
		case 0x0a: MCInst_setOpcode(info->inst, M68K_INS_FATAN); break;
		case 0x0c: MCInst_setOpcode(info->inst, M68K_INS_FASIN); break;
		case 0x0d: MCInst_setOpcode(info->inst, M68K_INS_FATANH); break;
		case 0x0e: MCInst_setOpcode(info->inst, M68K_INS_FSIN); break;
		case 0x0f: MCInst_setOpcode(info->inst, M68K_INS_FTAN); break;
		case 0x10: MCInst_setOpcode(info->inst, M68K_INS_FETOX); break;
		case 0x11: MCInst_setOpcode(info->inst, M68K_INS_FTWOTOX); break;
		case 0x12: MCInst_setOpcode(info->inst, M68K_INS_FTENTOX); break;
		case 0x14: MCInst_setOpcode(info->inst, M68K_INS_FLOGN); break;
		case 0x15: MCInst_setOpcode(info->inst, M68K_INS_FLOG10); break;
		case 0x16: MCInst_setOpcode(info->inst, M68K_INS_FLOG2); break;
		case 0x18: MCInst_setOpcode(info->inst, M68K_INS_FABS); break;
		case 0x19: MCInst_setOpcode(info->inst, M68K_INS_FCOSH); break;
		case 0x1a: MCInst_setOpcode(info->inst, M68K_INS_FNEG); break;
		case 0x1c: MCInst_setOpcode(info->inst, M68K_INS_FACOS); break;
		case 0x1d: MCInst_setOpcode(info->inst, M68K_INS_FCOS); break;
		case 0x1e: MCInst_setOpcode(info->inst, M68K_INS_FGETEXP); break;
		case 0x1f: MCInst_setOpcode(info->inst, M68K_INS_FGETMAN); break;
		case 0x20: MCInst_setOpcode(info->inst, M68K_INS_FDIV); supports_single_op = false; break;
		case 0x21: MCInst_setOpcode(info->inst, M68K_INS_FMOD); supports_single_op = false; break;
		case 0x22: MCInst_setOpcode(info->inst, M68K_INS_FADD); supports_single_op = false; break;
		case 0x23: MCInst_setOpcode(info->inst, M68K_INS_FMUL); supports_single_op = false; break;
		case 0x24: MCInst_setOpcode(info->inst, M68K_INS_FSGLDIV); supports_single_op = false; break;
		case 0x25: MCInst_setOpcode(info->inst, M68K_INS_FREM); break;
		case 0x26: MCInst_setOpcode(info->inst, M68K_INS_FSCALE); break;
		case 0x27: MCInst_setOpcode(info->inst, M68K_INS_FSGLMUL); break;
		case 0x28: MCInst_setOpcode(info->inst, M68K_INS_FSUB); supports_single_op = false; break;
		case 0x38: MCInst_setOpcode(info->inst, M68K_INS_FCMP); supports_single_op = false; break;
		case 0x3a: MCInst_setOpcode(info->inst, M68K_INS_FTST); break;
		default:
			break;
	}

	// Some trickery here! It's not documented but if bit 6 is set this is a s/d opcode and then
	// if bit 2 is set it's a d. As we already have set our opcode in the code above we can just
	// offset it as the following 2 op codes (if s/d is supported) will always be directly after it

	if ((next >> 6) & 1) {
		if ((next >> 2) & 1)
			info->inst->Opcode += 2;
		else
			info->inst->Opcode += 1;
	}

	ext = &info->extension;

	ext->op_count = 2;
	ext->op_size.type = M68K_SIZE_TYPE_CPU;
	ext->op_size.cpu_size = 0;

	op0 = &ext->operands[0];
	op1 = &ext->operands[1];

	if (rm == 0 && supports_single_op && src == dst) {
		ext->op_count = 1;
		op0->reg = M68K_REG_FP0 + dst;
		return;
	}

	if (rm == 1) {
		switch (src) {
			case 0x00 :
				ext->op_size.cpu_size = M68K_CPU_SIZE_LONG;
				get_ea_mode_op(info, op0, info->ir, 4);
				break;

			case 0x06 :
				ext->op_size.cpu_size = M68K_CPU_SIZE_BYTE;
				get_ea_mode_op(info, op0, info->ir, 1);
				break;

			case 0x04 :
				ext->op_size.cpu_size = M68K_CPU_SIZE_WORD;
				get_ea_mode_op(info, op0, info->ir, 2);
				break;

			case 0x01 :
				ext->op_size.type = M68K_SIZE_TYPE_FPU;
				ext->op_size.fpu_size = M68K_FPU_SIZE_SINGLE;
				get_ea_mode_op(info, op0, info->ir, 4);
				op0->type = M68K_OP_FP_SINGLE;
				break;

			case 0x05:
				ext->op_size.type = M68K_SIZE_TYPE_FPU;
				ext->op_size.fpu_size = M68K_FPU_SIZE_DOUBLE;
				get_ea_mode_op(info, op0, info->ir, 8);
				op0->type = M68K_OP_FP_DOUBLE;
				break;

			default :
				ext->op_size.type = M68K_SIZE_TYPE_FPU;
				ext->op_size.fpu_size = M68K_FPU_SIZE_EXTENDED;
				break;
		}
	} else {
		op0->reg = M68K_REG_FP0 + src;
	}

	op1->reg = M68K_REG_FP0 + dst;
}

static void d68020_cprestore(m68k_info *info)
{
	cs_m68k* ext;
	LIMIT_CPU_TYPES(info, M68020_PLUS);

	ext = build_init_op(info, M68K_INS_FRESTORE, 1, 0);
	get_ea_mode_op(info, &ext->operands[0], info->ir, 1);
}

static void d68020_cpsave(m68k_info *info)
{
	cs_m68k* ext;

	LIMIT_CPU_TYPES(info, M68020_PLUS);

	ext = build_init_op(info, M68K_INS_FSAVE, 1, 0);
	get_ea_mode_op(info, &ext->operands[0], info->ir, 1);
}

static void d68020_cpscc(m68k_info *info)
{
	cs_m68k* ext;

	LIMIT_CPU_TYPES(info, M68020_PLUS);
	ext = build_init_op(info, M68K_INS_FSF, 1, 1);

	// these are all in row with the extension so just doing a add here is fine
	info->inst->Opcode += (read_imm_16(info) & 0x2f);

	get_ea_mode_op(info, &ext->operands[0], info->ir, 1);
}

static void d68020_cptrapcc_0(m68k_info *info)
{
	uint32_t extension1;
	LIMIT_CPU_TYPES(info, M68020_PLUS);

	extension1 = read_imm_16(info);

	build_init_op(info, M68K_INS_FTRAPF, 0, 0);

	// these are all in row with the extension so just doing a add here is fine
	info->inst->Opcode += (extension1 & 0x2f);
}

static void d68020_cptrapcc_16(m68k_info *info)
{
	uint32_t extension1, extension2;
	cs_m68k_op* op0;
	cs_m68k* ext;

	LIMIT_CPU_TYPES(info, M68020_PLUS);

	extension1 = read_imm_16(info);
	extension2 = read_imm_16(info);

	ext = build_init_op(info, M68K_INS_FTRAPF, 1, 2);

	// these are all in row with the extension so just doing a add here is fine
	info->inst->Opcode += (extension1 & 0x2f);

	op0 = &ext->operands[0];

	op0->address_mode = M68K_AM_IMMEDIATE;
	op0->type = M68K_OP_IMM;
	op0->imm = extension2;
}

static void d68020_cptrapcc_32(m68k_info *info)
{
	uint32_t extension1, extension2;
	cs_m68k* ext;
	cs_m68k_op* op0;

	LIMIT_CPU_TYPES(info, M68020_PLUS);

	extension1 = read_imm_16(info);
	extension2 = read_imm_32(info);

	ext = build_init_op(info, M68K_INS_FTRAPF, 1, 2);

	// these are all in row with the extension so just doing a add here is fine
	info->inst->Opcode += (extension1 & 0x2f);

	op0 = &ext->operands[0];

	op0->address_mode = M68K_AM_IMMEDIATE;
	op0->type = M68K_OP_IMM;
	op0->imm = extension2;
}

static void d68040_cpush(m68k_info *info)
{
	LIMIT_CPU_TYPES(info, M68040_PLUS);
	build_cpush_cinv(info, M68K_INS_CPUSHL);
}

static void d68000_dbra(m68k_info *info)
{
	build_dbxx(info, M68K_INS_DBRA, 0, make_int_16(read_imm_16(info)));
}

static void d68000_dbcc(m68k_info *info)
{
	build_dbcc(info, 0, make_int_16(read_imm_16(info)));
}

static void d68000_divs(m68k_info *info)
{
	build_er_1(info, M68K_INS_DIVS, 2);
}

static void d68000_divu(m68k_info *info)
{
	build_er_1(info, M68K_INS_DIVU, 2);
}

static void d68020_divl(m68k_info *info)
{
	uint32_t extension, insn_signed;
	cs_m68k* ext;
	cs_m68k_op* op0;
	cs_m68k_op* op1;
	uint32_t reg_0, reg_1;

	LIMIT_CPU_TYPES(info, M68020_PLUS);

	extension = read_imm_16(info);
	insn_signed = 0;

	if (BIT_B((extension)))
		insn_signed = 1;

	ext = build_init_op(info, insn_signed ? M68K_INS_DIVS : M68K_INS_DIVU, 2, 4);

	op0 = &ext->operands[0];
	op1 = &ext->operands[1];

	get_ea_mode_op(info, op0, info->ir, 4);

	reg_0 = extension & 7;
	reg_1 = (extension >> 12) & 7;

	op1->address_mode = M68K_AM_NONE;
	op1->type = M68K_OP_REG_PAIR;
	op1->reg_pair.reg_0 = reg_0 + M68K_REG_D0;
	op1->reg_pair.reg_1 = reg_1 + M68K_REG_D0;

	if ((reg_0 == reg_1) || !BIT_A(extension)) {
		op1->type = M68K_OP_REG;
		op1->reg = M68K_REG_D0 + reg_1;
	}
}

static void d68000_eor_8(m68k_info *info)
{
	build_re_1(info, M68K_INS_EOR, 1);
}

static void d68000_eor_16(m68k_info *info)
{
	build_re_1(info, M68K_INS_EOR, 2);
}

static void d68000_eor_32(m68k_info *info)
{
	build_re_1(info, M68K_INS_EOR, 4);
}

static void d68000_eori_8(m68k_info *info)
{
	build_imm_ea(info, M68K_INS_EORI, 1, read_imm_8(info));
}

static void d68000_eori_16(m68k_info *info)
{
	build_imm_ea(info, M68K_INS_EORI, 2, read_imm_16(info));
}

static void d68000_eori_32(m68k_info *info)
{
	build_imm_ea(info, M68K_INS_EORI, 4, read_imm_32(info));
}

static void d68000_eori_to_ccr(m68k_info *info)
{
	build_imm_special_reg(info, M68K_INS_EORI, read_imm_8(info), 1, M68K_REG_CCR);
}

static void d68000_eori_to_sr(m68k_info *info)
{
	build_imm_special_reg(info, M68K_INS_EORI, read_imm_16(info), 2, M68K_REG_SR);
}

static void d68000_exg_dd(m68k_info *info)
{
	build_r(info, M68K_INS_EXG, 4);
}

static void d68000_exg_aa(m68k_info *info)
{
	cs_m68k_op* op0;
	cs_m68k_op* op1;
	cs_m68k* ext = build_init_op(info, M68K_INS_EXG, 2, 4);

	op0 = &ext->operands[0];
	op1 = &ext->operands[1];

	op0->address_mode = M68K_AM_NONE;
	op0->reg = M68K_REG_A0 + ((info->ir >> 9) & 7);

	op1->address_mode = M68K_AM_NONE;
	op1->reg = M68K_REG_A0 + (info->ir & 7);
}

static void d68000_exg_da(m68k_info *info)
{
	cs_m68k_op* op0;
	cs_m68k_op* op1;
	cs_m68k* ext = build_init_op(info, M68K_INS_EXG, 2, 4);

	op0 = &ext->operands[0];
	op1 = &ext->operands[1];

	op0->address_mode = M68K_AM_NONE;
	op0->reg = M68K_REG_D0 + ((info->ir >> 9) & 7);

	op1->address_mode = M68K_AM_NONE;
	op1->reg = M68K_REG_A0 + (info->ir & 7);
}

static void d68000_ext_16(m68k_info *info)
{
	build_d(info, M68K_INS_EXT, 2);
}

static void d68000_ext_32(m68k_info *info)
{
	build_d(info, M68K_INS_EXT, 4);
}

static void d68020_extb_32(m68k_info *info)
{
	LIMIT_CPU_TYPES(info, M68020_PLUS);
	build_d(info, M68K_INS_EXTB, 4);
}

static void d68000_jmp(m68k_info *info)
{
	cs_m68k* ext = build_init_op(info, M68K_INS_JMP, 1, 0);
	set_insn_group(info, M68K_GRP_JUMP);
	get_ea_mode_op(info, &ext->operands[0], info->ir, 4);
}

static void d68000_jsr(m68k_info *info)
{
	cs_m68k* ext = build_init_op(info, M68K_INS_JSR, 1, 0);
	set_insn_group(info, M68K_GRP_JUMP);
	get_ea_mode_op(info, &ext->operands[0], info->ir, 4);
}

static void d68000_lea(m68k_info *info)
{
	build_ea_a(info, M68K_INS_LEA, 4);
}

static void d68000_link_16(m68k_info *info)
{
	build_link(info, read_imm_16(info), 2);
}

static void d68020_link_32(m68k_info *info)
{
	LIMIT_CPU_TYPES(info, M68020_PLUS);
	build_link(info, read_imm_32(info), 4);
}

static void d68000_lsr_s_8(m68k_info *info)
{
	build_3bit_d(info, M68K_INS_LSR, 1);
}

static void d68000_lsr_s_16(m68k_info *info)
{
	build_3bit_d(info, M68K_INS_LSR, 2);
}

static void d68000_lsr_s_32(m68k_info *info)
{
	build_3bit_d(info, M68K_INS_LSR, 4);
}

static void d68000_lsr_r_8(m68k_info *info)
{
	build_r(info, M68K_INS_LSR, 1);
}

static void d68000_lsr_r_16(m68k_info *info)
{
	build_r(info, M68K_INS_LSR, 2);
}

static void d68000_lsr_r_32(m68k_info *info)
{
	build_r(info, M68K_INS_LSR, 4);
}

static void d68000_lsr_ea(m68k_info *info)
{
	build_ea(info, M68K_INS_LSR, 2);
}

static void d68000_lsl_s_8(m68k_info *info)
{
	build_3bit_d(info, M68K_INS_LSL, 1);
}

static void d68000_lsl_s_16(m68k_info *info)
{
	build_3bit_d(info, M68K_INS_LSL, 2);
}

static void d68000_lsl_s_32(m68k_info *info)
{
	build_3bit_d(info, M68K_INS_LSL, 4);
}

static void d68000_lsl_r_8(m68k_info *info)
{
	build_r(info, M68K_INS_LSL, 1);
}

static void d68000_lsl_r_16(m68k_info *info)
{
	build_r(info, M68K_INS_LSL, 2);
}

static void d68000_lsl_r_32(m68k_info *info)
{
	build_r(info, M68K_INS_LSL, 4);
}

static void d68000_lsl_ea(m68k_info *info)
{
	build_ea(info, M68K_INS_LSL, 2);
}

static void d68000_move_8(m68k_info *info)
{
	build_ea_ea(info, M68K_INS_MOVE, 1);
}

static void d68000_move_16(m68k_info *info)
{
	build_ea_ea(info, M68K_INS_MOVE, 2);
}

static void d68000_move_32(m68k_info *info)
{
	build_ea_ea(info, M68K_INS_MOVE, 4);
}

static void d68000_movea_16(m68k_info *info)
{
	build_ea_a(info, M68K_INS_MOVEA, 2);
}

static void d68000_movea_32(m68k_info *info)
{
	build_ea_a(info, M68K_INS_MOVEA, 4);
}

static void d68000_move_to_ccr(m68k_info *info)
{
	cs_m68k_op* op0;
	cs_m68k_op* op1;
	cs_m68k* ext = build_init_op(info, M68K_INS_MOVE, 2, 2);

	op0 = &ext->operands[0];
	op1 = &ext->operands[1];

	get_ea_mode_op(info, op0, info->ir, 1);

	op1->address_mode = M68K_AM_NONE;
	op1->reg = M68K_REG_CCR;
}

static void d68010_move_fr_ccr(m68k_info *info)
{
	cs_m68k_op* op0;
	cs_m68k_op* op1;
	cs_m68k* ext;

	LIMIT_CPU_TYPES(info, M68010_PLUS);

	ext = build_init_op(info, M68K_INS_MOVE, 2, 2);

	op0 = &ext->operands[0];
	op1 = &ext->operands[1];

	op0->address_mode = M68K_AM_NONE;
	op0->reg = M68K_REG_CCR;

	get_ea_mode_op(info, op1, info->ir, 1);
}

static void d68000_move_fr_sr(m68k_info *info)
{
	cs_m68k_op* op0;
	cs_m68k_op* op1;
	cs_m68k* ext = build_init_op(info, M68K_INS_MOVE, 2, 2);

	op0 = &ext->operands[0];
	op1 = &ext->operands[1];

	op0->address_mode = M68K_AM_NONE;
	op0->reg = M68K_REG_SR;

	get_ea_mode_op(info, op1, info->ir, 2);
}

static void d68000_move_to_sr(m68k_info *info)
{
	cs_m68k_op* op0;
	cs_m68k_op* op1;
	cs_m68k* ext = build_init_op(info, M68K_INS_MOVE, 2, 2);

	op0 = &ext->operands[0];
	op1 = &ext->operands[1];

	get_ea_mode_op(info, op0, info->ir, 2);

	op1->address_mode = M68K_AM_NONE;
	op1->reg = M68K_REG_SR;
}

static void d68000_move_fr_usp(m68k_info *info)
{
	cs_m68k_op* op0;
	cs_m68k_op* op1;
	cs_m68k* ext = build_init_op(info, M68K_INS_MOVE, 2, 0);

	op0 = &ext->operands[0];
	op1 = &ext->operands[1];

	op0->address_mode = M68K_AM_NONE;
	op0->reg = M68K_REG_USP;

	op1->address_mode = M68K_AM_NONE;
	op1->reg = M68K_REG_A0 + (info->ir & 7);
}

static void d68000_move_to_usp(m68k_info *info)
{
	cs_m68k_op* op0;
	cs_m68k_op* op1;
	cs_m68k* ext = build_init_op(info, M68K_INS_MOVE, 2, 0);

	op0 = &ext->operands[0];
	op1 = &ext->operands[1];

	op0->address_mode = M68K_AM_NONE;
	op0->reg = M68K_REG_A0 + (info->ir & 7);

	op1->address_mode = M68K_AM_NONE;
	op1->reg = M68K_REG_USP;
}

static void d68010_movec(m68k_info *info)
{
	uint32_t extension;
	m68k_reg reg;
	cs_m68k* ext;
	cs_m68k_op* op0;
	cs_m68k_op* op1;


	LIMIT_CPU_TYPES(info, M68010_PLUS);

	extension = read_imm_16(info);
	reg = M68K_REG_INVALID;

	ext = build_init_op(info, M68K_INS_MOVEC, 2, 0);

	op0 = &ext->operands[0];
	op1 = &ext->operands[1];

	switch (extension & 0xfff) {
		case 0x000: reg = M68K_REG_SFC; break;
		case 0x001: reg = M68K_REG_DFC; break;
		case 0x800: reg = M68K_REG_USP; break;
		case 0x801: reg = M68K_REG_VBR; break;
		case 0x002: reg = M68K_REG_CACR; break;
		case 0x802: reg = M68K_REG_CAAR; break;
		case 0x803: reg = M68K_REG_MSP; break;
		case 0x804: reg = M68K_REG_ISP; break;
		case 0x003: reg = M68K_REG_TC; break;
		case 0x004: reg = M68K_REG_ITT0; break;
		case 0x005: reg = M68K_REG_ITT1; break;
		case 0x006: reg = M68K_REG_DTT0; break;
		case 0x007: reg = M68K_REG_DTT1; break;
		case 0x805: reg = M68K_REG_MMUSR; break;
		case 0x806: reg = M68K_REG_URP; break;
		case 0x807: reg = M68K_REG_SRP; break;
	}

	if (BIT_1(info->ir)) {
		op0->reg = (BIT_F(extension) ? M68K_REG_A0 : M68K_REG_D0) + ((extension >> 12) & 7);
		op1->reg = reg;
	} else {
		op0->reg = reg;
		op1->reg = (BIT_F(extension) ? M68K_REG_A0 : M68K_REG_D0) + ((extension >> 12) & 7);
	}
}

static void d68000_movem_pd_16(m68k_info *info)
{
	build_movem_re(info, M68K_INS_MOVEM, 2);
}

static void d68000_movem_pd_32(m68k_info *info)
{
	build_movem_re(info, M68K_INS_MOVEM, 4);
}

static void d68000_movem_er_16(m68k_info *info)
{
	build_movem_er(info, M68K_INS_MOVEM, 2);
}

static void d68000_movem_er_32(m68k_info *info)
{
	build_movem_er(info, M68K_INS_MOVEM, 4);
}

static void d68000_movem_re_16(m68k_info *info)
{
	build_movem_re(info, M68K_INS_MOVEM, 2);
}

static void d68000_movem_re_32(m68k_info *info)
{
	build_movem_re(info, M68K_INS_MOVEM, 4);
}

static void d68000_movep_re_16(m68k_info *info)
{
	build_movep_re(info, 2);
}

static void d68000_movep_re_32(m68k_info *info)
{
	build_movep_re(info, 4);
}

static void d68000_movep_er_16(m68k_info *info)
{
	build_movep_er(info, 2);
}

static void d68000_movep_er_32(m68k_info *info)
{
	build_movep_er(info, 4);
}

static void d68010_moves_8(m68k_info *info)
{
	LIMIT_CPU_TYPES(info, M68010_PLUS);
	build_moves(info, 1);
}

static void d68010_moves_16(m68k_info *info)
{
	//uint32_t extension;
	LIMIT_CPU_TYPES(info, M68010_PLUS);
	build_moves(info, 2);
}

static void d68010_moves_32(m68k_info *info)
{
	LIMIT_CPU_TYPES(info, M68010_PLUS);
	build_moves(info, 4);
}

static void d68000_moveq(m68k_info *info)
{
	cs_m68k_op* op0;
	cs_m68k_op* op1;

	cs_m68k* ext = build_init_op(info, M68K_INS_MOVEQ, 2, 0);

	op0 = &ext->operands[0];
	op1 = &ext->operands[1];

	op0->type = M68K_OP_IMM;
	op0->address_mode = M68K_AM_IMMEDIATE;
	op0->imm = (info->ir & 0xff);

	op1->address_mode = M68K_AM_REG_DIRECT_DATA;
	op1->reg = M68K_REG_D0 + ((info->ir >> 9) & 7);
}

static void d68040_move16_pi_pi(m68k_info *info)
{
	int data[] = { info->ir & 7, (read_imm_16(info) >> 12) & 7 };
	int modes[] = { M68K_AM_REGI_ADDR_POST_INC, M68K_AM_REGI_ADDR_POST_INC };

	LIMIT_CPU_TYPES(info, M68040_PLUS);

	build_move16(info, data, modes);
}

static void d68040_move16_pi_al(m68k_info *info)
{
	int data[] = { info->ir & 7, read_imm_32(info) };
	int modes[] = { M68K_AM_REGI_ADDR_POST_INC, M68K_AM_ABSOLUTE_DATA_LONG };

	LIMIT_CPU_TYPES(info, M68040_PLUS);

	build_move16(info, data, modes);
}

static void d68040_move16_al_pi(m68k_info *info)
{
	int data[] = { read_imm_32(info), info->ir & 7 };
	int modes[] = { M68K_AM_ABSOLUTE_DATA_LONG, M68K_AM_REGI_ADDR_POST_INC };

	LIMIT_CPU_TYPES(info, M68040_PLUS);

	build_move16(info, data, modes);
}

static void d68040_move16_ai_al(m68k_info *info)
{
	int data[] = { info->ir & 7, read_imm_32(info) };
	int modes[] = { M68K_AM_REG_DIRECT_ADDR, M68K_AM_ABSOLUTE_DATA_LONG };

	LIMIT_CPU_TYPES(info, M68040_PLUS);

	build_move16(info, data, modes);
}

static void d68040_move16_al_ai(m68k_info *info)
{
	int data[] = { read_imm_32(info), info->ir & 7 };
	int modes[] = { M68K_AM_ABSOLUTE_DATA_LONG, M68K_AM_REG_DIRECT_ADDR };

	LIMIT_CPU_TYPES(info, M68040_PLUS);

	build_move16(info, data, modes);
}

static void d68000_muls(m68k_info *info)
{
	build_er_1(info, M68K_INS_MULS, 2);
}

static void d68000_mulu(m68k_info *info)
{
	build_er_1(info, M68K_INS_MULU, 2);
}

static void d68020_mull(m68k_info *info)
{
	uint32_t extension, insn_signed;
	cs_m68k* ext;
	cs_m68k_op* op0;
	cs_m68k_op* op1;
	uint32_t reg_0, reg_1;

	LIMIT_CPU_TYPES(info, M68020_PLUS);

	extension = read_imm_16(info);
	insn_signed = 0;

	if (BIT_B((extension)))
		insn_signed = 1;

	ext = build_init_op(info, insn_signed ? M68K_INS_MULS : M68K_INS_MULU, 2, 4);

	op0 = &ext->operands[0];
	op1 = &ext->operands[1];

	get_ea_mode_op(info, op0, info->ir, 4);

	reg_0 = extension & 7;
	reg_1 = (extension >> 12) & 7;

	op1->address_mode = M68K_AM_NONE;
	op1->type = M68K_OP_REG_PAIR;
	op1->reg_pair.reg_0 = reg_0 + M68K_REG_D0;
	op1->reg_pair.reg_1 = reg_1 + M68K_REG_D0;

	if (!BIT_A(extension)) {
		op1->type = M68K_OP_REG;
		op1->reg = M68K_REG_D0 + reg_1;
	}
}

static void d68000_nbcd(m68k_info *info)
{
	build_ea(info, M68K_INS_NBCD, 1);
}

static void d68000_neg_8(m68k_info *info)
{
	build_ea(info, M68K_INS_NEG, 1);
}

static void d68000_neg_16(m68k_info *info)
{
	build_ea(info, M68K_INS_NEG, 2);
}

static void d68000_neg_32(m68k_info *info)
{
	build_ea(info, M68K_INS_NEG, 4);
}

static void d68000_negx_8(m68k_info *info)
{
	build_ea(info, M68K_INS_NEGX, 1);
}

static void d68000_negx_16(m68k_info *info)
{
	build_ea(info, M68K_INS_NEGX, 2);
}

static void d68000_negx_32(m68k_info *info)
{
	build_ea(info, M68K_INS_NEGX, 4);
}

static void d68000_nop(m68k_info *info)
{
	MCInst_setOpcode(info->inst, M68K_INS_NOP);
}

static void d68000_not_8(m68k_info *info)
{
	build_ea(info, M68K_INS_NOT, 1);
}

static void d68000_not_16(m68k_info *info)
{
	build_ea(info, M68K_INS_NOT, 2);
}

static void d68000_not_32(m68k_info *info)
{
	build_ea(info, M68K_INS_NOT, 4);
}

static void d68000_or_er_8(m68k_info *info)
{
	build_er_1(info, M68K_INS_OR, 1);
}

static void d68000_or_er_16(m68k_info *info)
{
	build_er_1(info, M68K_INS_OR, 2);
}

static void d68000_or_er_32(m68k_info *info)
{
	build_er_1(info, M68K_INS_OR, 4);
}

static void d68000_or_re_8(m68k_info *info)
{
	build_re_1(info, M68K_INS_OR, 1);
}

static void d68000_or_re_16(m68k_info *info)
{
	build_re_1(info, M68K_INS_OR, 2);
}

static void d68000_or_re_32(m68k_info *info)
{
	build_re_1(info, M68K_INS_OR, 4);
}

static void d68000_ori_8(m68k_info *info)
{
	build_imm_ea(info, M68K_INS_ORI, 1, read_imm_8(info));
}

static void d68000_ori_16(m68k_info *info)
{
	build_imm_ea(info, M68K_INS_ORI, 2, read_imm_16(info));
}

static void d68000_ori_32(m68k_info *info)
{
	build_imm_ea(info, M68K_INS_ORI, 4, read_imm_32(info));
}

static void d68000_ori_to_ccr(m68k_info *info)
{
	build_imm_special_reg(info, M68K_INS_ORI, read_imm_8(info), 1, M68K_REG_CCR);
}

static void d68000_ori_to_sr(m68k_info *info)
{
	build_imm_special_reg(info, M68K_INS_ORI, read_imm_16(info), 2, M68K_REG_SR);
}

static void d68020_pack_rr(m68k_info *info)
{
	LIMIT_CPU_TYPES(info, M68020_PLUS);
	build_rr(info, M68K_INS_PACK, 0, read_imm_16(info));
}

static void d68020_pack_mm(m68k_info *info)
{
	LIMIT_CPU_TYPES(info, M68020_PLUS);
	build_mm(info, M68K_INS_PACK, 0, read_imm_16(info));
}

static void d68000_pea(m68k_info *info)
{
	build_ea(info, M68K_INS_PEA, 4);
}

static void d68000_reset(m68k_info *info)
{
	MCInst_setOpcode(info->inst, M68K_INS_RESET);
}

static void d68000_ror_s_8(m68k_info *info)
{
	build_3bit_d(info, M68K_INS_ROR, 1);
}

static void d68000_ror_s_16(m68k_info *info)
{
	build_3bit_d(info, M68K_INS_ROR, 2);
}

static void d68000_ror_s_32(m68k_info *info)
{
	build_3bit_d(info, M68K_INS_ROR, 4);
}

static void d68000_ror_r_8(m68k_info *info)
{
	build_r(info, M68K_INS_ROR, 1);
}

static void d68000_ror_r_16(m68k_info *info)
{
	build_r(info, M68K_INS_ROR, 2);
}

static void d68000_ror_r_32(m68k_info *info)
{
	build_r(info, M68K_INS_ROR, 4);
}

static void d68000_ror_ea(m68k_info *info)
{
	build_ea(info, M68K_INS_ROR, 2);
}

static void d68000_rol_s_8(m68k_info *info)
{
	build_3bit_d(info, M68K_INS_ROL, 1);
}

static void d68000_rol_s_16(m68k_info *info)
{
	build_3bit_d(info, M68K_INS_ROL, 2);
}

static void d68000_rol_s_32(m68k_info *info)
{
	build_3bit_d(info, M68K_INS_ROL, 4);
}

static void d68000_rol_r_8(m68k_info *info)
{
	build_r(info, M68K_INS_ROL, 1);
}

static void d68000_rol_r_16(m68k_info *info)
{
	build_r(info, M68K_INS_ROL, 2);
}

static void d68000_rol_r_32(m68k_info *info)
{
	build_r(info, M68K_INS_ROL, 4);
}

static void d68000_rol_ea(m68k_info *info)
{
	build_ea(info, M68K_INS_ROL, 2);
}

static void d68000_roxr_s_8(m68k_info *info)
{
	build_3bit_d(info, M68K_INS_ROXR, 1);
}

static void d68000_roxr_s_16(m68k_info *info)
{
	build_3bit_d(info, M68K_INS_ROXR, 2);
}

static void d68000_roxr_s_32(m68k_info *info)
{
	build_3bit_d(info, M68K_INS_ROXR, 4);
}

static void d68000_roxr_r_8(m68k_info *info)
{
	build_3bit_d(info, M68K_INS_ROXR, 4);
}

static void d68000_roxr_r_16(m68k_info *info)
{
	build_r(info, M68K_INS_ROXR, 2);
}

static void d68000_roxr_r_32(m68k_info *info)
{
	build_r(info, M68K_INS_ROXR, 4);
}

static void d68000_roxr_ea(m68k_info *info)
{
	build_ea(info, M68K_INS_ROXR, 2);
}

static void d68000_roxl_s_8(m68k_info *info)
{
	build_3bit_d(info, M68K_INS_ROXL, 1);
}

static void d68000_roxl_s_16(m68k_info *info)
{
	build_3bit_d(info, M68K_INS_ROXL, 2);
}

static void d68000_roxl_s_32(m68k_info *info)
{
	build_3bit_d(info, M68K_INS_ROXL, 4);
}

static void d68000_roxl_r_8(m68k_info *info)
{
	build_r(info, M68K_INS_ROXL, 1);
}

static void d68000_roxl_r_16(m68k_info *info)
{
	build_r(info, M68K_INS_ROXL, 2);
}

static void d68000_roxl_r_32(m68k_info *info)
{
	build_r(info, M68K_INS_ROXL, 4);
}

static void d68000_roxl_ea(m68k_info *info)
{
	build_ea(info, M68K_INS_ROXL, 2);
}

static void d68010_rtd(m68k_info *info)
{
	set_insn_group(info, M68K_GRP_RET);
	LIMIT_CPU_TYPES(info, M68010_PLUS);
	build_absolute_jump_with_immediate(info, M68K_INS_RTD, 0, read_imm_16(info));
}

static void d68000_rte(m68k_info *info)
{
	set_insn_group(info, M68K_GRP_IRET);
	MCInst_setOpcode(info->inst, M68K_INS_RTE);
}

static void d68020_rtm(m68k_info *info)
{
	cs_m68k* ext;
	cs_m68k_op* op;

	set_insn_group(info, M68K_GRP_RET);

	LIMIT_CPU_TYPES(info, M68020_ONLY);

	build_absolute_jump_with_immediate(info, M68K_INS_RTM, 0, 0);

	ext = &info->extension;
	op = &ext->operands[0];

	op->address_mode = M68K_AM_NONE;
	op->type = M68K_OP_REG;

	if (BIT_3(info->ir)) {
		op->reg = M68K_REG_A0 + (info->ir & 7);
	} else {
		op->reg = M68K_REG_D0 + (info->ir & 7);
	}
}

static void d68000_rtr(m68k_info *info)
{
	set_insn_group(info, M68K_GRP_RET);
	MCInst_setOpcode(info->inst, M68K_INS_RTR);
}

static void d68000_rts(m68k_info *info)
{
	set_insn_group(info, M68K_GRP_RET);
	MCInst_setOpcode(info->inst, M68K_INS_RTS);
}

static void d68000_sbcd_rr(m68k_info *info)
{
	build_rr(info, M68K_INS_SBCD, 1, 0);
}

static void d68000_sbcd_mm(m68k_info *info)
{
	build_mm(info, M68K_INS_SBCD, 0, read_imm_16(info));
}

static void d68000_scc(m68k_info *info)
{
	cs_m68k* ext = build_init_op(info, s_scc_lut[(info->ir >> 8) & 0xf], 1, 1);
	get_ea_mode_op(info, &ext->operands[0], info->ir, 1);
}

static void d68000_stop(m68k_info *info)
{
	build_absolute_jump_with_immediate(info, M68K_INS_STOP, 0, read_imm_16(info));
}

static void d68000_sub_er_8(m68k_info *info)
{
	build_er_1(info, M68K_INS_SUB, 1);
}

static void d68000_sub_er_16(m68k_info *info)
{
	build_er_1(info, M68K_INS_SUB, 2);
}

static void d68000_sub_er_32(m68k_info *info)
{
	build_er_1(info, M68K_INS_SUB, 4);
}

static void d68000_sub_re_8(m68k_info *info)
{
	build_re_1(info, M68K_INS_SUB, 1);
}

static void d68000_sub_re_16(m68k_info *info)
{
	build_re_1(info, M68K_INS_SUB, 2);
}

static void d68000_sub_re_32(m68k_info *info)
{
	build_re_1(info, M68K_INS_SUB, 4);
}

static void d68000_suba_16(m68k_info *info)
{
	build_ea_a(info, M68K_INS_SUBA, 2);
}

static void d68000_suba_32(m68k_info *info)
{
	build_ea_a(info, M68K_INS_SUBA, 4);
}

static void d68000_subi_8(m68k_info *info)
{
	build_imm_ea(info, M68K_INS_SUBI, 1, read_imm_8(info));
}

static void d68000_subi_16(m68k_info *info)
{
	build_imm_ea(info, M68K_INS_SUBI, 2, read_imm_16(info));
}

static void d68000_subi_32(m68k_info *info)
{
	build_imm_ea(info, M68K_INS_SUBI, 4, read_imm_32(info));
}

static void d68000_subq_8(m68k_info *info)
{
	build_3bit_ea(info, M68K_INS_SUBQ, 1);
}

static void d68000_subq_16(m68k_info *info)
{
	build_3bit_ea(info, M68K_INS_SUBQ, 2);
}

static void d68000_subq_32(m68k_info *info)
{
	build_3bit_ea(info, M68K_INS_SUBQ, 4);
}

static void d68000_subx_rr_8(m68k_info *info)
{
	build_rr(info, M68K_INS_SUBX, 1, 0);
}

static void d68000_subx_rr_16(m68k_info *info)
{
	build_rr(info, M68K_INS_SUBX, 2, 0);
}

static void d68000_subx_rr_32(m68k_info *info)
{
	build_rr(info, M68K_INS_SUBX, 4, 0);
}

static void d68000_subx_mm_8(m68k_info *info)
{
	build_mm(info, M68K_INS_SUBX, 1, 0);
}

static void d68000_subx_mm_16(m68k_info *info)
{
	build_mm(info, M68K_INS_SUBX, 2, 0);
}

static void d68000_subx_mm_32(m68k_info *info)
{
	build_mm(info, M68K_INS_SUBX, 4, 0);
}

static void d68000_swap(m68k_info *info)
{
	build_d(info, M68K_INS_SWAP, 0);
}

static void d68000_tas(m68k_info *info)
{
	build_ea(info, M68K_INS_TAS, 1);
}

static void d68000_trap(m68k_info *info)
{
	build_absolute_jump_with_immediate(info, M68K_INS_TRAP, 0, info->ir&0xf);
}

static void d68020_trapcc_0(m68k_info *info)
{
	LIMIT_CPU_TYPES(info, M68020_PLUS);
	build_trap(info, 0, 0);

	info->extension.op_count = 0;
}

static void d68020_trapcc_16(m68k_info *info)
{
	LIMIT_CPU_TYPES(info, M68020_PLUS);
	build_trap(info, 2, read_imm_16(info));
}

static void d68020_trapcc_32(m68k_info *info)
{
	LIMIT_CPU_TYPES(info, M68020_PLUS);
	build_trap(info, 4, read_imm_32(info));
}

static void d68000_trapv(m68k_info *info)
{
	MCInst_setOpcode(info->inst, M68K_INS_TRAPV);
}

static void d68000_tst_8(m68k_info *info)
{
	build_ea(info, M68K_INS_TST, 1);
}

static void d68020_tst_pcdi_8(m68k_info *info)
{
	LIMIT_CPU_TYPES(info, M68020_PLUS);
	build_ea(info, M68K_INS_TST, 1);
}

static void d68020_tst_pcix_8(m68k_info *info)
{
	LIMIT_CPU_TYPES(info, M68020_PLUS);
	build_ea(info, M68K_INS_TST, 1);
}

static void d68020_tst_i_8(m68k_info *info)
{
	LIMIT_CPU_TYPES(info, M68020_PLUS);
	build_ea(info, M68K_INS_TST, 1);
}

static void d68000_tst_16(m68k_info *info)
{
	build_ea(info, M68K_INS_TST, 2);
}

static void d68020_tst_a_16(m68k_info *info)
{
	LIMIT_CPU_TYPES(info, M68020_PLUS);
	build_ea(info, M68K_INS_TST, 2);
}

static void d68020_tst_pcdi_16(m68k_info *info)
{
	LIMIT_CPU_TYPES(info, M68020_PLUS);
	build_ea(info, M68K_INS_TST, 2);
}

static void d68020_tst_pcix_16(m68k_info *info)
{
	LIMIT_CPU_TYPES(info, M68020_PLUS);
	build_ea(info, M68K_INS_TST, 2);
}

static void d68020_tst_i_16(m68k_info *info)
{
	LIMIT_CPU_TYPES(info, M68020_PLUS);
	build_ea(info, M68K_INS_TST, 2);
}

static void d68000_tst_32(m68k_info *info)
{
	build_ea(info, M68K_INS_TST, 4);
}

static void d68020_tst_a_32(m68k_info *info)
{
	LIMIT_CPU_TYPES(info, M68020_PLUS);
	build_ea(info, M68K_INS_TST, 4);
}

static void d68020_tst_pcdi_32(m68k_info *info)
{
	LIMIT_CPU_TYPES(info, M68020_PLUS);
	build_ea(info, M68K_INS_TST, 4);
}

static void d68020_tst_pcix_32(m68k_info *info)
{
	LIMIT_CPU_TYPES(info, M68020_PLUS);
	build_ea(info, M68K_INS_TST, 4);
}

static void d68020_tst_i_32(m68k_info *info)
{
	LIMIT_CPU_TYPES(info, M68020_PLUS);
	build_ea(info, M68K_INS_TST, 4);
}

static void d68000_unlk(m68k_info *info)
{
	cs_m68k_op* op;
	cs_m68k* ext = build_init_op(info, M68K_INS_UNLK, 1, 0);

	op = &ext->operands[0];

	op->address_mode = M68K_AM_REG_DIRECT_ADDR;
	op->reg = M68K_REG_A0 + (info->ir & 7);
}

static void d68020_unpk_rr(m68k_info *info)
{
	LIMIT_CPU_TYPES(info, M68020_PLUS);
	build_rr(info, M68K_INS_UNPK, 0, read_imm_16(info));
}

static void d68020_unpk_mm(m68k_info *info)
{
	LIMIT_CPU_TYPES(info, M68020_PLUS);
	build_mm(info, M68K_INS_UNPK, 0, read_imm_16(info));
}

/* This table is auto-generated. Look in contrib/m68k_instruction_tbl_gen for more info */
#include "M68KInstructionTable.inc"

static int instruction_is_valid(m68k_info *info, const unsigned int word_check)
{
	const unsigned int instruction = info->ir;
	instruction_struct *i = &g_instruction_table[instruction];

	if ( (i->word2_mask && ((word_check & i->word2_mask) != i->word2_match)) ||
		(i->instruction == d68000_invalid) ) {
		d68000_invalid(info);
		return 0;
	}

	return 1;
}

static int exists_reg_list(uint16_t *regs, uint8_t count, m68k_reg reg)
{
	uint8_t i;

	for (i = 0; i < count; ++i) {
		if (regs[i] == (uint16_t)reg)
			return 1;
	}

	return 0;
}

static void add_reg_to_rw_list(m68k_info *info, m68k_reg reg, int write)
{
	if (reg == M68K_REG_INVALID)
		return;

	if (write)
	{
		if (exists_reg_list(info->regs_write, info->regs_write_count, reg))
			return;

		info->regs_write[info->regs_write_count] = (uint16_t)reg;
		info->regs_write_count++;
	}
	else
	{
		if (exists_reg_list(info->regs_read, info->regs_read_count, reg))
			return;

		info->regs_read[info->regs_read_count] = (uint16_t)reg;
		info->regs_read_count++;
	}
}

static void update_am_reg_list(m68k_info *info, cs_m68k_op *op, int write)
{
	switch (op->address_mode) {
		case M68K_AM_REG_DIRECT_ADDR:
		case M68K_AM_REG_DIRECT_DATA:
			add_reg_to_rw_list(info, op->reg, write);
			break;

		case M68K_AM_REGI_ADDR_POST_INC:
		case M68K_AM_REGI_ADDR_PRE_DEC:
			add_reg_to_rw_list(info, op->reg, 1);
			break;

		case M68K_AM_REGI_ADDR:
		case M68K_AM_REGI_ADDR_DISP:
			add_reg_to_rw_list(info, op->reg, 0);
			break;

		case M68K_AM_AREGI_INDEX_8_BIT_DISP:
		case M68K_AM_AREGI_INDEX_BASE_DISP:
		case M68K_AM_MEMI_POST_INDEX:
		case M68K_AM_MEMI_PRE_INDEX:
		case M68K_AM_PCI_INDEX_8_BIT_DISP:
		case M68K_AM_PCI_INDEX_BASE_DISP:
		case M68K_AM_PC_MEMI_PRE_INDEX:
		case M68K_AM_PC_MEMI_POST_INDEX:
			add_reg_to_rw_list(info, op->mem.index_reg, 0);
			add_reg_to_rw_list(info, op->mem.base_reg, 0);
			break;

		// no register(s) in the other addressing modes
		default:
			break;
	}
}

static void update_bits_range(m68k_info *info, m68k_reg reg_start, uint8_t bits, int write)
{
	int i;

	for (i = 0; i < 8; ++i) {
		if (bits & (1 << i)) {
			add_reg_to_rw_list(info, reg_start + i, write);
		}
	}
}

static void update_reg_list_regbits(m68k_info *info, cs_m68k_op *op, int write)
{
	uint32_t bits = op->register_bits;
	update_bits_range(info, M68K_REG_D0, bits & 0xff, write);
	update_bits_range(info, M68K_REG_A0, (bits >> 8) & 0xff, write);
	update_bits_range(info, M68K_REG_FP0, (bits >> 16) & 0xff, write);
}

static void update_op_reg_list(m68k_info *info, cs_m68k_op *op, int write)
{
	switch ((int)op->type) {
		case M68K_OP_REG:
			add_reg_to_rw_list(info, op->reg, write);
			break;

		case M68K_OP_MEM:
			update_am_reg_list(info, op, write);
			break;

		case M68K_OP_REG_BITS:
			update_reg_list_regbits(info, op, write);
			break;

		case M68K_OP_REG_PAIR:
			add_reg_to_rw_list(info, op->reg_pair.reg_0, write);
			add_reg_to_rw_list(info, op->reg_pair.reg_1, write);
			break;
	}
}

static void build_regs_read_write_counts(m68k_info *info)
{
	int i;

	if (!info->extension.op_count)
		return;

	if (info->extension.op_count == 1) {
		update_op_reg_list(info, &info->extension.operands[0], 1);
	} else {
		// first operand is always read
		update_op_reg_list(info, &info->extension.operands[0], 0);

		// remaning write
		for (i = 1; i < info->extension.op_count; ++i)
			update_op_reg_list(info, &info->extension.operands[i], 1);
	}
}

static void m68k_setup_internals(m68k_info* info, MCInst* inst, unsigned int pc, unsigned int cpu_type)
{
	info->inst = inst;
	info->pc = pc;
	info->ir = 0;
	info->type = cpu_type;
	info->address_mask = 0xffffffff;

	switch(info->type) {
		case M68K_CPU_TYPE_68000:
			info->type = TYPE_68000;
			info->address_mask = 0x00ffffff;
			break;
		case M68K_CPU_TYPE_68010:
			info->type = TYPE_68010;
			info->address_mask = 0x00ffffff;
			break;
		case M68K_CPU_TYPE_68EC020:
			info->type = TYPE_68020;
			info->address_mask = 0x00ffffff;
			break;
		case M68K_CPU_TYPE_68020:
			info->type = TYPE_68020;
			info->address_mask = 0xffffffff;
			break;
		case M68K_CPU_TYPE_68030:
			info->type = TYPE_68030;
			info->address_mask = 0xffffffff;
			break;
		case M68K_CPU_TYPE_68040:
			info->type = TYPE_68040;
			info->address_mask = 0xffffffff;
			break;
		default:
			info->address_mask = 0;
			return;
	}
}

/* ======================================================================== */
/* ================================= API ================================== */
/* ======================================================================== */

/* Disasemble one instruction at pc and store in str_buff */
static unsigned int m68k_disassemble(m68k_info *info, uint64_t pc)
{
	MCInst *inst = info->inst;
	cs_m68k* ext = &info->extension;
	int i;
	unsigned int size;

	inst->Opcode = M68K_INS_INVALID;

	memset(ext, 0, sizeof(cs_m68k));
	ext->op_size.type = M68K_SIZE_TYPE_CPU;

	for (i = 0; i < M68K_OPERAND_COUNT; ++i)
		ext->operands[i].type = M68K_OP_REG;

	info->ir = peek_imm_16(info);
	if (instruction_is_valid(info, peek_imm_32(info) & 0xffff)) {
		info->ir = read_imm_16(info);
		g_instruction_table[info->ir].instruction(info);
	}

	size = info->pc - (unsigned int)pc;
	info->pc = (unsigned int)pc;

	return size;
}

bool M68K_getInstruction(csh ud, const uint8_t* code, size_t code_len, MCInst* instr, uint16_t* size, uint64_t address, void* inst_info)
{
#ifdef M68K_DEBUG
	SStream ss;
#endif
	int s;
	int cpu_type = M68K_CPU_TYPE_68000;
	cs_struct* handle = instr->csh;
	m68k_info *info = (m68k_info*)handle->printer_info;

	// code len has to be at least 2 bytes to be valid m68k

	if (code_len < 2) {
		*size = 0;
		return false;
	}

	if (instr->flat_insn->detail) {
		memset(instr->flat_insn->detail, 0, offsetof(cs_detail, m68k)+sizeof(cs_m68k));
	}

	info->groups_count = 0;
	info->regs_read_count = 0;
	info->regs_write_count = 0;
	info->code = code;
	info->code_len = code_len;
	info->baseAddress = address;

	if (handle->mode & CS_MODE_M68K_010)
		cpu_type = M68K_CPU_TYPE_68010;
	if (handle->mode & CS_MODE_M68K_020)
		cpu_type = M68K_CPU_TYPE_68020;
	if (handle->mode & CS_MODE_M68K_030)
		cpu_type = M68K_CPU_TYPE_68030;
	if (handle->mode & CS_MODE_M68K_040)
		cpu_type = M68K_CPU_TYPE_68040;
	if (handle->mode & CS_MODE_M68K_060)
		cpu_type = M68K_CPU_TYPE_68040;	// 060 = 040 for now

	m68k_setup_internals(info, instr, (unsigned int)address, cpu_type);
	s = m68k_disassemble(info, address);

	if (s == 0) {
		*size = 2;
		return false;
	}

	build_regs_read_write_counts(info);

#ifdef M68K_DEBUG
	SStream_Init(&ss);
	M68K_printInst(instr, &ss, info);
#endif

	// Make sure we always stay within range
	if (s > (int)code_len)
		*size = (uint16_t)code_len;
	else
		*size = (uint16_t)s;

	return true;
}

