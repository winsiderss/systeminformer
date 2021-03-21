//===-- X86ATTInstPrinter.cpp - AT&T assembly instruction printing --------===//
//
//                     The LLVM Compiler Infrastructure
//
// This file is distributed under the University of Illinois Open Source
// License. See LICENSE.TXT for details.
//
//===----------------------------------------------------------------------===//
//
// This file includes code for rendering MCInst instances as AT&T-style
// assembly.
//
//===----------------------------------------------------------------------===//

/* Capstone Disassembly Engine */
/* By Nguyen Anh Quynh <aquynh@gmail.com>, 2013-2015 */

// this code is only relevant when DIET mode is disable
#if defined(CAPSTONE_HAS_X86) && !defined(CAPSTONE_DIET) && !defined(CAPSTONE_X86_ATT_DISABLE)

#if defined (WIN32) || defined (WIN64) || defined (_WIN32) || defined (_WIN64)
#pragma warning(disable:4996)			// disable MSVC's warning on strncpy()
#pragma warning(disable:28719)		// disable MSVC's warning on strncpy()
#endif

#if !defined(CAPSTONE_HAS_OSXKERNEL)
#include <ctype.h>
#endif
#include "../capstone/include/capstone/platform.h"

#if defined(CAPSTONE_HAS_OSXKERNEL)
#include <Availability.h>
#include <libkern/libkern.h>
#else
#include <stdio.h>
#include <stdlib.h>
#endif

#include <string.h>

#include "../../utils.h"
#include "../../MCInst.h"
#include "../../SStream.h"
#include "../../MCRegisterInfo.h"
#include "X86Mapping.h"
#include "X86BaseInfo.h"


#define GET_INSTRINFO_ENUM
#ifdef CAPSTONE_X86_REDUCE
#include "X86GenInstrInfo_reduce.inc"
#else
#include "X86GenInstrInfo.inc"
#endif

static void printMemReference(MCInst *MI, unsigned Op, SStream *O);
static void printOperand(MCInst *MI, unsigned OpNo, SStream *O);


static void set_mem_access(MCInst *MI, bool status)
{
	if (MI->csh->detail != CS_OPT_ON)
		return;

	MI->csh->doing_mem = status;
	if (!status)
		// done, create the next operand slot
		MI->flat_insn->detail->x86.op_count++;
}

static void printopaquemem(MCInst *MI, unsigned OpNo, SStream *O)
{
	switch(MI->csh->mode) {
		case CS_MODE_16:
			switch(MI->flat_insn->id) {
				default:
					MI->x86opsize = 2;
					break;
				case X86_INS_LJMP:
				case X86_INS_LCALL:
					MI->x86opsize = 4;
					break;
				case X86_INS_SGDT:
				case X86_INS_SIDT:
				case X86_INS_LGDT:
				case X86_INS_LIDT:
					MI->x86opsize = 6;
					break;
			}
			break;
		case CS_MODE_32:
			switch(MI->flat_insn->id) {
				default:
					MI->x86opsize = 4;
					break;
				case X86_INS_LJMP:
				case X86_INS_LCALL:
				case X86_INS_SGDT:
				case X86_INS_SIDT:
				case X86_INS_LGDT:
				case X86_INS_LIDT:
					MI->x86opsize = 6;
					break;
			}
			break;
		case CS_MODE_64:
			switch(MI->flat_insn->id) {
				default:
					MI->x86opsize = 8;
					break;
				case X86_INS_LJMP:
				case X86_INS_LCALL:
				case X86_INS_SGDT:
				case X86_INS_SIDT:
				case X86_INS_LGDT:
				case X86_INS_LIDT:
					MI->x86opsize = 10;
					break;
			}
			break;
		default:	// never reach
			break;
	}

	printMemReference(MI, OpNo, O);
}

static void printi8mem(MCInst *MI, unsigned OpNo, SStream *O)
{
	MI->x86opsize = 1;
	printMemReference(MI, OpNo, O);
}

static void printi16mem(MCInst *MI, unsigned OpNo, SStream *O)
{
	MI->x86opsize = 2;

	printMemReference(MI, OpNo, O);
}

static void printi32mem(MCInst *MI, unsigned OpNo, SStream *O)
{
	MI->x86opsize = 4;

	printMemReference(MI, OpNo, O);
}

static void printi64mem(MCInst *MI, unsigned OpNo, SStream *O)
{
	MI->x86opsize = 8;
	printMemReference(MI, OpNo, O);
}

static void printi128mem(MCInst *MI, unsigned OpNo, SStream *O)
{
	MI->x86opsize = 16;
	printMemReference(MI, OpNo, O);
}

#ifndef CAPSTONE_X86_REDUCE
static void printi256mem(MCInst *MI, unsigned OpNo, SStream *O)
{
	MI->x86opsize = 32;
	printMemReference(MI, OpNo, O);
}

static void printi512mem(MCInst *MI, unsigned OpNo, SStream *O)
{
	MI->x86opsize = 64;
	printMemReference(MI, OpNo, O);
}

static void printf32mem(MCInst *MI, unsigned OpNo, SStream *O)
{
	switch(MCInst_getOpcode(MI)) {
		default:
			MI->x86opsize = 4;
			break;
		case X86_FBSTPm:
		case X86_FBLDm:
			// TODO: fix this in tablegen instead
			MI->x86opsize = 10;
			break;
		case X86_FSTENVm:
		case X86_FLDENVm:
			// TODO: fix this in tablegen instead
			switch(MI->csh->mode) {
				default:    // never reach
					break;
				case CS_MODE_16:
					MI->x86opsize = 14;
					break;
				case CS_MODE_32:
				case CS_MODE_64:
					MI->x86opsize = 28;
					break;
			}
			break;
	}

	printMemReference(MI, OpNo, O);
}

static void printf64mem(MCInst *MI, unsigned OpNo, SStream *O)
{
	MI->x86opsize = 8;
	printMemReference(MI, OpNo, O);
}

static void printf80mem(MCInst *MI, unsigned OpNo, SStream *O)
{
	MI->x86opsize = 10;
	printMemReference(MI, OpNo, O);
}

static void printf128mem(MCInst *MI, unsigned OpNo, SStream *O)
{
	MI->x86opsize = 16;
	printMemReference(MI, OpNo, O);
}

static void printf256mem(MCInst *MI, unsigned OpNo, SStream *O)
{
	MI->x86opsize = 32;
	printMemReference(MI, OpNo, O);
}

static void printf512mem(MCInst *MI, unsigned OpNo, SStream *O)
{
	MI->x86opsize = 64;
	printMemReference(MI, OpNo, O);
}

static void printSSECC(MCInst *MI, unsigned Op, SStream *OS)
{
	uint8_t Imm = (uint8_t)(MCOperand_getImm(MCInst_getOperand(MI, Op)) & 7);
	switch (Imm) {
		default: break;	// never reach
		case    0: SStream_concat0(OS, "eq"); op_addSseCC(MI, X86_SSE_CC_EQ); break;
		case    1: SStream_concat0(OS, "lt"); op_addSseCC(MI, X86_SSE_CC_LT); break;
		case    2: SStream_concat0(OS, "le"); op_addSseCC(MI, X86_SSE_CC_LE); break;
		case    3: SStream_concat0(OS, "unord"); op_addSseCC(MI, X86_SSE_CC_UNORD); break;
		case    4: SStream_concat0(OS, "neq"); op_addSseCC(MI, X86_SSE_CC_NEQ); break;
		case    5: SStream_concat0(OS, "nlt"); op_addSseCC(MI, X86_SSE_CC_NLT); break;
		case    6: SStream_concat0(OS, "nle"); op_addSseCC(MI, X86_SSE_CC_NLE); break;
		case    7: SStream_concat0(OS, "ord"); op_addSseCC(MI, X86_SSE_CC_ORD); break;
	}

	MI->popcode_adjust = Imm + 1;
}

static void printAVXCC(MCInst *MI, unsigned Op, SStream *O)
{
	uint8_t Imm = (uint8_t)(MCOperand_getImm(MCInst_getOperand(MI, Op)) & 0x1f);
	switch (Imm) {
		default: break;//printf("Invalid avxcc argument!\n"); break;
		case    0: SStream_concat0(O, "eq"); op_addAvxCC(MI, X86_AVX_CC_EQ); break;
		case    1: SStream_concat0(O, "lt"); op_addAvxCC(MI, X86_AVX_CC_LT); break;
		case    2: SStream_concat0(O, "le"); op_addAvxCC(MI, X86_AVX_CC_LE); break;
		case    3: SStream_concat0(O, "unord"); op_addAvxCC(MI, X86_AVX_CC_UNORD); break;
		case    4: SStream_concat0(O, "neq"); op_addAvxCC(MI, X86_AVX_CC_NEQ); break;
		case    5: SStream_concat0(O, "nlt"); op_addAvxCC(MI, X86_AVX_CC_NLT); break;
		case    6: SStream_concat0(O, "nle"); op_addAvxCC(MI, X86_AVX_CC_NLE); break;
		case    7: SStream_concat0(O, "ord"); op_addAvxCC(MI, X86_AVX_CC_ORD); break;
		case    8: SStream_concat0(O, "eq_uq"); op_addAvxCC(MI, X86_AVX_CC_EQ_UQ); break;
		case    9: SStream_concat0(O, "nge"); op_addAvxCC(MI, X86_AVX_CC_NGE); break;
		case  0xa: SStream_concat0(O, "ngt"); op_addAvxCC(MI, X86_AVX_CC_NGT); break;
		case  0xb: SStream_concat0(O, "false"); op_addAvxCC(MI, X86_AVX_CC_FALSE); break;
		case  0xc: SStream_concat0(O, "neq_oq"); op_addAvxCC(MI, X86_AVX_CC_NEQ_OQ); break;
		case  0xd: SStream_concat0(O, "ge"); op_addAvxCC(MI, X86_AVX_CC_GE); break;
		case  0xe: SStream_concat0(O, "gt"); op_addAvxCC(MI, X86_AVX_CC_GT); break;
		case  0xf: SStream_concat0(O, "true"); op_addAvxCC(MI, X86_AVX_CC_TRUE); break;
		case 0x10: SStream_concat0(O, "eq_os"); op_addAvxCC(MI, X86_AVX_CC_EQ_OS); break;
		case 0x11: SStream_concat0(O, "lt_oq"); op_addAvxCC(MI, X86_AVX_CC_LT_OQ); break;
		case 0x12: SStream_concat0(O, "le_oq"); op_addAvxCC(MI, X86_AVX_CC_LE_OQ); break;
		case 0x13: SStream_concat0(O, "unord_s"); op_addAvxCC(MI, X86_AVX_CC_UNORD_S); break;
		case 0x14: SStream_concat0(O, "neq_us"); op_addAvxCC(MI, X86_AVX_CC_NEQ_US); break;
		case 0x15: SStream_concat0(O, "nlt_uq"); op_addAvxCC(MI, X86_AVX_CC_NLT_UQ); break;
		case 0x16: SStream_concat0(O, "nle_uq"); op_addAvxCC(MI, X86_AVX_CC_NLE_UQ); break;
		case 0x17: SStream_concat0(O, "ord_s"); op_addAvxCC(MI, X86_AVX_CC_ORD_S); break;
		case 0x18: SStream_concat0(O, "eq_us"); op_addAvxCC(MI, X86_AVX_CC_EQ_US); break;
		case 0x19: SStream_concat0(O, "nge_uq"); op_addAvxCC(MI, X86_AVX_CC_NGE_UQ); break;
		case 0x1a: SStream_concat0(O, "ngt_uq"); op_addAvxCC(MI, X86_AVX_CC_NGT_UQ); break;
		case 0x1b: SStream_concat0(O, "false_os"); op_addAvxCC(MI, X86_AVX_CC_FALSE_OS); break;
		case 0x1c: SStream_concat0(O, "neq_os"); op_addAvxCC(MI, X86_AVX_CC_NEQ_OS); break;
		case 0x1d: SStream_concat0(O, "ge_oq"); op_addAvxCC(MI, X86_AVX_CC_GE_OQ); break;
		case 0x1e: SStream_concat0(O, "gt_oq"); op_addAvxCC(MI, X86_AVX_CC_GT_OQ); break;
		case 0x1f: SStream_concat0(O, "true_us"); op_addAvxCC(MI, X86_AVX_CC_TRUE_US); break;
	}

	MI->popcode_adjust = Imm + 1;
}

static void printXOPCC(MCInst *MI, unsigned Op, SStream *O)
{
	int64_t Imm = MCOperand_getImm(MCInst_getOperand(MI, Op));

	switch (Imm) {
		default: // llvm_unreachable("Invalid xopcc argument!");
		case 0: SStream_concat0(O, "lt"); op_addXopCC(MI, X86_XOP_CC_LT); break;
		case 1: SStream_concat0(O, "le"); op_addXopCC(MI, X86_XOP_CC_LE); break;
		case 2: SStream_concat0(O, "gt"); op_addXopCC(MI, X86_XOP_CC_GT); break;
		case 3: SStream_concat0(O, "ge"); op_addXopCC(MI, X86_XOP_CC_GE); break;
		case 4: SStream_concat0(O, "eq"); op_addXopCC(MI, X86_XOP_CC_EQ); break;
		case 5: SStream_concat0(O, "neq"); op_addXopCC(MI, X86_XOP_CC_NEQ); break;
		case 6: SStream_concat0(O, "false"); op_addXopCC(MI, X86_XOP_CC_FALSE); break;
		case 7: SStream_concat0(O, "true"); op_addXopCC(MI, X86_XOP_CC_TRUE); break;
	}
}

static void printRoundingControl(MCInst *MI, unsigned Op, SStream *O)
{
	int64_t Imm = MCOperand_getImm(MCInst_getOperand(MI, Op)) & 0x3;
	switch (Imm) {
		case 0: SStream_concat0(O, "{rn-sae}"); op_addAvxSae(MI); op_addAvxRoundingMode(MI, X86_AVX_RM_RN); break;
		case 1: SStream_concat0(O, "{rd-sae}"); op_addAvxSae(MI); op_addAvxRoundingMode(MI, X86_AVX_RM_RD); break;
		case 2: SStream_concat0(O, "{ru-sae}"); op_addAvxSae(MI); op_addAvxRoundingMode(MI, X86_AVX_RM_RU); break;
		case 3: SStream_concat0(O, "{rz-sae}"); op_addAvxSae(MI); op_addAvxRoundingMode(MI, X86_AVX_RM_RZ); break;
		default: break;	// nev0er reach
	}
}

#endif

static void printRegName(SStream *OS, unsigned RegNo);

// local printOperand, without updating public operands
static void _printOperand(MCInst *MI, unsigned OpNo, SStream *O)
{
	MCOperand *Op  = MCInst_getOperand(MI, OpNo);
	if (MCOperand_isReg(Op)) {
		printRegName(O, MCOperand_getReg(Op));
	} else if (MCOperand_isImm(Op)) {
		// Print X86 immediates as signed values.
		int64_t imm = MCOperand_getImm(Op);
		uint8_t encsize;
		uint8_t opsize = X86_immediate_size(MCInst_getOpcode(MI), &encsize);

		if (imm < 0) {
			if (MI->csh->imm_unsigned) {
				if (opsize) {
					switch(opsize) {
						default:
							break;
						case 1:
							imm &= 0xff;
							break;
						case 2:
							imm &= 0xffff;
							break;
						case 4:
							imm &= 0xffffffff;
							break;
					}
				}

				SStream_concat(O, "$0x%"PRIx64, imm);
			} else {
				if (imm < -HEX_THRESHOLD)
					SStream_concat(O, "$-0x%"PRIx64, -imm);
				else
					SStream_concat(O, "$-%"PRIu64, -imm);
			}
		} else {
			if (imm > HEX_THRESHOLD)
				SStream_concat(O, "$0x%"PRIx64, imm);
			else
				SStream_concat(O, "$%"PRIu64, imm);
		}
	}
}

// convert Intel access info to AT&T access info
static void get_op_access(cs_struct *h, unsigned int id, uint8_t *access, uint64_t *eflags)
{
	uint8_t count, i;
	uint8_t *arr = X86_get_op_access(h, id, eflags);

	if (!arr) {
		access[0] = 0;
		return;
	}

	// find the non-zero last entry
	for(count = 0; arr[count]; count++);

	if (count == 0)
		return;

	// copy in reverse order this access array from Intel syntax -> AT&T syntax
	count--;
	for(i = 0; i <= count; i++) {
		if (arr[count - i] != CS_AC_IGNORE)
			access[i] = arr[count - i];
		else
			access[i] = 0;
	}
}

static void printSrcIdx(MCInst *MI, unsigned Op, SStream *O)
{
	MCOperand *SegReg;
	int reg;

	if (MI->csh->detail) {
		uint8_t access[6];

		MI->flat_insn->detail->x86.operands[MI->flat_insn->detail->x86.op_count].type = X86_OP_MEM;
		MI->flat_insn->detail->x86.operands[MI->flat_insn->detail->x86.op_count].size = MI->x86opsize;
		MI->flat_insn->detail->x86.operands[MI->flat_insn->detail->x86.op_count].mem.segment = X86_REG_INVALID;
		MI->flat_insn->detail->x86.operands[MI->flat_insn->detail->x86.op_count].mem.base = X86_REG_INVALID;
		MI->flat_insn->detail->x86.operands[MI->flat_insn->detail->x86.op_count].mem.index = X86_REG_INVALID;
		MI->flat_insn->detail->x86.operands[MI->flat_insn->detail->x86.op_count].mem.scale = 1;
		MI->flat_insn->detail->x86.operands[MI->flat_insn->detail->x86.op_count].mem.disp = 0;

		get_op_access(MI->csh, MCInst_getOpcode(MI), access, &MI->flat_insn->detail->x86.eflags);
		MI->flat_insn->detail->x86.operands[MI->flat_insn->detail->x86.op_count].access = access[MI->flat_insn->detail->x86.op_count];
	}

	SegReg = MCInst_getOperand(MI, Op+1);
	reg = MCOperand_getReg(SegReg);

	// If this has a segment register, print it.
	if (reg) {
		_printOperand(MI, Op+1, O);
		if (MI->csh->detail) {
			MI->flat_insn->detail->x86.operands[MI->flat_insn->detail->x86.op_count].mem.segment = reg;
		}

		SStream_concat0(O, ":");
	}

	SStream_concat0(O, "(");
	set_mem_access(MI, true);

	printOperand(MI, Op, O);

	SStream_concat0(O, ")");
	set_mem_access(MI, false);
}

static void printDstIdx(MCInst *MI, unsigned Op, SStream *O)
{
	if (MI->csh->detail) {
		uint8_t access[6];

		MI->flat_insn->detail->x86.operands[MI->flat_insn->detail->x86.op_count].type = X86_OP_MEM;
		MI->flat_insn->detail->x86.operands[MI->flat_insn->detail->x86.op_count].size = MI->x86opsize;
		MI->flat_insn->detail->x86.operands[MI->flat_insn->detail->x86.op_count].mem.segment = X86_REG_INVALID;
		MI->flat_insn->detail->x86.operands[MI->flat_insn->detail->x86.op_count].mem.base = X86_REG_INVALID;
		MI->flat_insn->detail->x86.operands[MI->flat_insn->detail->x86.op_count].mem.index = X86_REG_INVALID;
		MI->flat_insn->detail->x86.operands[MI->flat_insn->detail->x86.op_count].mem.scale = 1;
		MI->flat_insn->detail->x86.operands[MI->flat_insn->detail->x86.op_count].mem.disp = 0;

		get_op_access(MI->csh, MCInst_getOpcode(MI), access, &MI->flat_insn->detail->x86.eflags);
		MI->flat_insn->detail->x86.operands[MI->flat_insn->detail->x86.op_count].access = access[MI->flat_insn->detail->x86.op_count];
	}

	// DI accesses are always ES-based on non-64bit mode
	if (MI->csh->mode != CS_MODE_64) {
		SStream_concat0(O, "%es:(");
		if (MI->csh->detail) {
			MI->flat_insn->detail->x86.operands[MI->flat_insn->detail->x86.op_count].mem.segment = X86_REG_ES;
		}
	} else
		SStream_concat0(O, "(");

	set_mem_access(MI, true);

	printOperand(MI, Op, O);

	SStream_concat0(O, ")");
	set_mem_access(MI, false);
}

static void printSrcIdx8(MCInst *MI, unsigned OpNo, SStream *O)
{
	MI->x86opsize = 1;
	printSrcIdx(MI, OpNo, O);
}

static void printSrcIdx16(MCInst *MI, unsigned OpNo, SStream *O)
{
	MI->x86opsize = 2;
	printSrcIdx(MI, OpNo, O);
}

static void printSrcIdx32(MCInst *MI, unsigned OpNo, SStream *O)
{
	MI->x86opsize = 4;
	printSrcIdx(MI, OpNo, O);
}

static void printSrcIdx64(MCInst *MI, unsigned OpNo, SStream *O)
{
	MI->x86opsize = 8;
	printSrcIdx(MI, OpNo, O);
}

static void printDstIdx8(MCInst *MI, unsigned OpNo, SStream *O)
{
	MI->x86opsize = 1;
	printDstIdx(MI, OpNo, O);
}

static void printDstIdx16(MCInst *MI, unsigned OpNo, SStream *O)
{
	MI->x86opsize = 2;
	printDstIdx(MI, OpNo, O);
}

static void printDstIdx32(MCInst *MI, unsigned OpNo, SStream *O)
{
	MI->x86opsize = 4;
	printDstIdx(MI, OpNo, O);
}

static void printDstIdx64(MCInst *MI, unsigned OpNo, SStream *O)
{
	MI->x86opsize = 8;
	printDstIdx(MI, OpNo, O);
}

static void printMemOffset(MCInst *MI, unsigned Op, SStream *O)
{
	MCOperand *DispSpec = MCInst_getOperand(MI, Op);
	MCOperand *SegReg = MCInst_getOperand(MI, Op+1);
	int reg;

	if (MI->csh->detail) {
		uint8_t access[6];

		MI->flat_insn->detail->x86.operands[MI->flat_insn->detail->x86.op_count].type = X86_OP_MEM;
		MI->flat_insn->detail->x86.operands[MI->flat_insn->detail->x86.op_count].size = MI->x86opsize;
		MI->flat_insn->detail->x86.operands[MI->flat_insn->detail->x86.op_count].mem.segment = X86_REG_INVALID;
		MI->flat_insn->detail->x86.operands[MI->flat_insn->detail->x86.op_count].mem.base = X86_REG_INVALID;
		MI->flat_insn->detail->x86.operands[MI->flat_insn->detail->x86.op_count].mem.index = X86_REG_INVALID;
		MI->flat_insn->detail->x86.operands[MI->flat_insn->detail->x86.op_count].mem.scale = 1;
		MI->flat_insn->detail->x86.operands[MI->flat_insn->detail->x86.op_count].mem.disp = 0;

		get_op_access(MI->csh, MCInst_getOpcode(MI), access, &MI->flat_insn->detail->x86.eflags);
		MI->flat_insn->detail->x86.operands[MI->flat_insn->detail->x86.op_count].access = access[MI->flat_insn->detail->x86.op_count];
	}

	// If this has a segment register, print it.
	reg = MCOperand_getReg(SegReg);
	if (reg) {
		_printOperand(MI, Op + 1, O);
		SStream_concat0(O, ":");
		if (MI->csh->detail) {
			MI->flat_insn->detail->x86.operands[MI->flat_insn->detail->x86.op_count].mem.segment = reg;
		}
	}

	if (MCOperand_isImm(DispSpec)) {
		int64_t imm = MCOperand_getImm(DispSpec);
		if (MI->csh->detail)
			MI->flat_insn->detail->x86.operands[MI->flat_insn->detail->x86.op_count].mem.disp = imm;
		if (imm < 0) {
			SStream_concat(O, "0x%"PRIx64, arch_masks[MI->csh->mode] & imm);
		} else {
			if (imm > HEX_THRESHOLD)
				SStream_concat(O, "0x%"PRIx64, imm);
			else
				SStream_concat(O, "%"PRIu64, imm);
		}
	}

	if (MI->csh->detail)
		MI->flat_insn->detail->x86.op_count++;
}

#ifndef CAPSTONE_X86_REDUCE
static void printU8Imm(MCInst *MI, unsigned Op, SStream *O)
{
	uint8_t val = MCOperand_getImm(MCInst_getOperand(MI, Op)) & 0xff;

	if (val > HEX_THRESHOLD)
		SStream_concat(O, "$0x%x", val);
	else
		SStream_concat(O, "$%u", val);

	if (MI->csh->detail) {
		MI->flat_insn->detail->x86.operands[MI->flat_insn->detail->x86.op_count].type = X86_OP_IMM;
		MI->flat_insn->detail->x86.operands[MI->flat_insn->detail->x86.op_count].imm = val;
		MI->flat_insn->detail->x86.operands[MI->flat_insn->detail->x86.op_count].size = 1;
		MI->flat_insn->detail->x86.op_count++;
	}
}
#endif

static void printMemOffs8(MCInst *MI, unsigned OpNo, SStream *O)
{
	MI->x86opsize = 1;
	printMemOffset(MI, OpNo, O);
}

static void printMemOffs16(MCInst *MI, unsigned OpNo, SStream *O)
{
	MI->x86opsize = 2;
	printMemOffset(MI, OpNo, O);
}

static void printMemOffs32(MCInst *MI, unsigned OpNo, SStream *O)
{
	MI->x86opsize = 4;
	printMemOffset(MI, OpNo, O);
}

static void printMemOffs64(MCInst *MI, unsigned OpNo, SStream *O)
{
	MI->x86opsize = 8;
	printMemOffset(MI, OpNo, O);
}

/// printPCRelImm - This is used to print an immediate value that ends up
/// being encoded as a pc-relative value (e.g. for jumps and calls).  These
/// print slightly differently than normal immediates.  For example, a $ is not
/// emitted.
static void printPCRelImm(MCInst *MI, unsigned OpNo, SStream *O)
{
	MCOperand *Op = MCInst_getOperand(MI, OpNo);
	if (MCOperand_isImm(Op)) {
		int64_t imm = MCOperand_getImm(Op) + MI->flat_insn->size + MI->address;

		// truncat imm for non-64bit
		if (MI->csh->mode != CS_MODE_64) {
			imm = imm & 0xffffffff;
		}

		if (MI->csh->mode == CS_MODE_16 &&
				(MI->Opcode != X86_JMP_4 && MI->Opcode != X86_CALLpcrel32))
			imm = imm & 0xffff;

		// Hack: X86 16bit with opcode X86_JMP_4
		if (MI->csh->mode == CS_MODE_16 &&
				(MI->Opcode == X86_JMP_4 && MI->x86_prefix[2] != 0x66))
			imm = imm & 0xffff;

		// CALL/JMP rel16 is special
		if (MI->Opcode == X86_CALLpcrel16 || MI->Opcode == X86_JMP_2)
			imm = imm & 0xffff;

		if (imm < 0) {
			SStream_concat(O, "0x%"PRIx64, imm);
		} else {
			if (imm > HEX_THRESHOLD)
				SStream_concat(O, "0x%"PRIx64, imm);
			else
				SStream_concat(O, "%"PRIu64, imm);
		}
		if (MI->csh->detail) {
			MI->flat_insn->detail->x86.operands[MI->flat_insn->detail->x86.op_count].type = X86_OP_IMM;
			MI->has_imm = true;
			MI->flat_insn->detail->x86.operands[MI->flat_insn->detail->x86.op_count].imm = imm;
			MI->flat_insn->detail->x86.op_count++;
		}
	}
}

static void printOperand(MCInst *MI, unsigned OpNo, SStream *O)
{
	MCOperand *Op  = MCInst_getOperand(MI, OpNo);
	if (MCOperand_isReg(Op)) {
		unsigned int reg = MCOperand_getReg(Op);
		printRegName(O, reg);
		if (MI->csh->detail) {
			if (MI->csh->doing_mem) {
				MI->flat_insn->detail->x86.operands[MI->flat_insn->detail->x86.op_count].mem.base = reg;
			} else {
				uint8_t access[6];

				MI->flat_insn->detail->x86.operands[MI->flat_insn->detail->x86.op_count].type = X86_OP_REG;
				MI->flat_insn->detail->x86.operands[MI->flat_insn->detail->x86.op_count].reg = reg;
				MI->flat_insn->detail->x86.operands[MI->flat_insn->detail->x86.op_count].size = MI->csh->regsize_map[reg];

				get_op_access(MI->csh, MCInst_getOpcode(MI), access, &MI->flat_insn->detail->x86.eflags);
				MI->flat_insn->detail->x86.operands[MI->flat_insn->detail->x86.op_count].access = access[MI->flat_insn->detail->x86.op_count];

				MI->flat_insn->detail->x86.op_count++;
			}
		}
	} else if (MCOperand_isImm(Op)) {
		// Print X86 immediates as signed values.
		uint8_t encsize;
		int64_t imm = MCOperand_getImm(Op);
		uint8_t opsize = X86_immediate_size(MCInst_getOpcode(MI), &encsize);

		if (opsize == 1)    // print 1 byte immediate in positive form
			imm = imm & 0xff;

		switch(MI->flat_insn->id) {
			default:
				if (imm >= 0) {
					if (imm > HEX_THRESHOLD)
						SStream_concat(O, "$0x%"PRIx64, imm);
					else
						SStream_concat(O, "$%"PRIu64, imm);
				} else {
					if (MI->csh->imm_unsigned) {
						if (opsize) {
							switch(opsize) {
								default:
									break;
								case 1:
									imm &= 0xff;
									break;
								case 2:
									imm &= 0xffff;
									break;
								case 4:
									imm &= 0xffffffff;
									break;
							}
						}

						SStream_concat(O, "$0x%"PRIx64, imm);
					} else {
						if (imm == 0x8000000000000000LL)  // imm == -imm
							SStream_concat0(O, "$0x8000000000000000");
						else if (imm < -HEX_THRESHOLD)
							SStream_concat(O, "$-0x%"PRIx64, -imm);
						else
							SStream_concat(O, "$-%"PRIu64, -imm);
					}
				}
				break;

			case X86_INS_MOVABS:
				// do not print number in negative form
				SStream_concat(O, "$0x%"PRIx64, imm);
				break;

			case X86_INS_IN:
			case X86_INS_OUT:
			case X86_INS_INT:
				// do not print number in negative form
				imm = imm & 0xff;
				if (imm >= 0 && imm <= HEX_THRESHOLD)
					SStream_concat(O, "$%u", imm);
				else {
					SStream_concat(O, "$0x%x", imm);
				}
				break;

			case X86_INS_LCALL:
			case X86_INS_LJMP:
				// always print address in positive form
				if (OpNo == 1) {	// selector is ptr16
					imm = imm & 0xffff;
					opsize = 2;
				}
				SStream_concat(O, "$0x%"PRIx64, imm);
				break;

			case X86_INS_AND:
			case X86_INS_OR:
			case X86_INS_XOR:
				// do not print number in negative form
				if (imm >= 0 && imm <= HEX_THRESHOLD)
					SStream_concat(O, "$%u", imm);
				else {
					imm = arch_masks[opsize? opsize : MI->imm_size] & imm;
					SStream_concat(O, "$0x%"PRIx64, imm);
				}
				break;

			case X86_INS_RET:
			case X86_INS_RETF:
				// RET imm16
				if (imm >= 0 && imm <= HEX_THRESHOLD)
					SStream_concat(O, "$%u", imm);
				else {
					imm = 0xffff & imm;
					SStream_concat(O, "$0x%x", imm);
				}
				break;
		}

		if (MI->csh->detail) {
			if (MI->csh->doing_mem) {
				MI->flat_insn->detail->x86.operands[MI->flat_insn->detail->x86.op_count].type = X86_OP_MEM;
				MI->flat_insn->detail->x86.operands[MI->flat_insn->detail->x86.op_count].mem.disp = imm;
			} else {
				MI->flat_insn->detail->x86.operands[MI->flat_insn->detail->x86.op_count].type = X86_OP_IMM;
				MI->has_imm = true;
				MI->flat_insn->detail->x86.operands[MI->flat_insn->detail->x86.op_count].imm = imm;

				if (opsize > 0) {
					MI->flat_insn->detail->x86.operands[MI->flat_insn->detail->x86.op_count].size = opsize;
					MI->flat_insn->detail->x86.encoding.imm_size = encsize;
				} else if (MI->op1_size > 0)
					MI->flat_insn->detail->x86.operands[MI->flat_insn->detail->x86.op_count].size = MI->op1_size;
				else
					MI->flat_insn->detail->x86.operands[MI->flat_insn->detail->x86.op_count].size = MI->imm_size;

				MI->flat_insn->detail->x86.op_count++;
			}
		}
	}
}

static void printMemReference(MCInst *MI, unsigned Op, SStream *O)
{
	MCOperand *BaseReg  = MCInst_getOperand(MI, Op + X86_AddrBaseReg);
	MCOperand *IndexReg  = MCInst_getOperand(MI, Op + X86_AddrIndexReg);
	MCOperand *DispSpec = MCInst_getOperand(MI, Op + X86_AddrDisp);
	MCOperand *SegReg = MCInst_getOperand(MI, Op + X86_AddrSegmentReg);
	uint64_t ScaleVal;
	int segreg;
	int64_t DispVal = 1;

	if (MI->csh->detail) {
		uint8_t access[6];

		MI->flat_insn->detail->x86.operands[MI->flat_insn->detail->x86.op_count].type = X86_OP_MEM;
		MI->flat_insn->detail->x86.operands[MI->flat_insn->detail->x86.op_count].size = MI->x86opsize;
		MI->flat_insn->detail->x86.operands[MI->flat_insn->detail->x86.op_count].mem.segment = X86_REG_INVALID;
		MI->flat_insn->detail->x86.operands[MI->flat_insn->detail->x86.op_count].mem.base = MCOperand_getReg(BaseReg);
		MI->flat_insn->detail->x86.operands[MI->flat_insn->detail->x86.op_count].mem.index = MCOperand_getReg(IndexReg);
		MI->flat_insn->detail->x86.operands[MI->flat_insn->detail->x86.op_count].mem.scale = 1;
		MI->flat_insn->detail->x86.operands[MI->flat_insn->detail->x86.op_count].mem.disp = 0;

		get_op_access(MI->csh, MCInst_getOpcode(MI), access, &MI->flat_insn->detail->x86.eflags);
		MI->flat_insn->detail->x86.operands[MI->flat_insn->detail->x86.op_count].access = access[MI->flat_insn->detail->x86.op_count];
	}

	// If this has a segment register, print it.
	segreg = MCOperand_getReg(SegReg);
	if (segreg) {
		_printOperand(MI, Op + X86_AddrSegmentReg, O);
		if (MI->csh->detail) {
			MI->flat_insn->detail->x86.operands[MI->flat_insn->detail->x86.op_count].mem.segment = segreg;
		}

		SStream_concat0(O, ":");
	}

	if (MCOperand_isImm(DispSpec)) {
		DispVal = MCOperand_getImm(DispSpec);
		if (MI->csh->detail)
			MI->flat_insn->detail->x86.operands[MI->flat_insn->detail->x86.op_count].mem.disp = DispVal;
		if (DispVal) {
			if (MCOperand_getReg(IndexReg) || MCOperand_getReg(BaseReg)) {
				printInt64(O, DispVal);
			} else {
				// only immediate as address of memory
				if (DispVal < 0) {
					SStream_concat(O, "0x%"PRIx64, arch_masks[MI->csh->mode] & DispVal);
				} else {
					if (DispVal > HEX_THRESHOLD)
						SStream_concat(O, "0x%"PRIx64, DispVal);
					else
						SStream_concat(O, "%"PRIu64, DispVal);
				}
			}
		} else {
		}
	}

	if (MCOperand_getReg(IndexReg) || MCOperand_getReg(BaseReg)) {
		SStream_concat0(O, "(");

		if (MCOperand_getReg(BaseReg))
			_printOperand(MI, Op + X86_AddrBaseReg, O);

		if (MCOperand_getReg(IndexReg)) {
			SStream_concat0(O, ", ");
			_printOperand(MI, Op + X86_AddrIndexReg, O);
			ScaleVal = MCOperand_getImm(MCInst_getOperand(MI, Op + X86_AddrScaleAmt));
			if (MI->csh->detail)
				MI->flat_insn->detail->x86.operands[MI->flat_insn->detail->x86.op_count].mem.scale = (int)ScaleVal;
			if (ScaleVal != 1) {
				SStream_concat(O, ", %u", ScaleVal);
			}
		}
		SStream_concat0(O, ")");
	} else {
		if (!DispVal)
			SStream_concat0(O, "0");
	}

	if (MI->csh->detail)
		MI->flat_insn->detail->x86.op_count++;
}

static void printanymem(MCInst *MI, unsigned OpNo, SStream *O)
{
	switch(MI->Opcode) {
		default: break;
		case X86_LEA16r:
				 MI->x86opsize = 2;
				 break;
		case X86_LEA32r:
		case X86_LEA64_32r:
				 MI->x86opsize = 4;
				 break;
		case X86_LEA64r:
				 MI->x86opsize = 8;
				 break;
	}
	printMemReference(MI, OpNo, O);
}

#include "X86InstPrinter.h"

#define GET_REGINFO_ENUM
#include "X86GenRegisterInfo.inc"

// Include the auto-generated portion of the assembly writer.
#define PRINT_ALIAS_INSTR
#ifdef CAPSTONE_X86_REDUCE
#include "X86GenAsmWriter_reduce.inc"
#else
#include "X86GenAsmWriter.inc"
#endif

static void printRegName(SStream *OS, unsigned RegNo)
{
	SStream_concat(OS, "%%%s", getRegisterName(RegNo));
}

void X86_ATT_printInst(MCInst *MI, SStream *OS, void *info)
{
	char *mnem;
	x86_reg reg, reg2;
	enum cs_ac_type access1, access2;
	int i;

	// perhaps this instruction does not need printer
	if (MI->assembly[0]) {
		strncpy(OS->buffer, MI->assembly, sizeof(OS->buffer));
		return;
	}

	// Output CALLpcrel32 as "callq" in 64-bit mode.
	// In Intel annotation it's always emitted as "call".
	//
	// TODO: Probably this hack should be redesigned via InstAlias in
	// InstrInfo.td as soon as Requires clause is supported properly
	// for InstAlias.
	if (MI->csh->mode == CS_MODE_64 && MCInst_getOpcode(MI) == X86_CALLpcrel32) {
		SStream_concat0(OS, "callq\t");
		MCInst_setOpcodePub(MI, X86_INS_CALL);
		printPCRelImm(MI, 0, OS);
		return;
	}

	// Try to print any aliases first.
	mnem = printAliasInstr(MI, OS, info);
	if (mnem)
		cs_mem_free(mnem);
	else
		printInstruction(MI, OS, info);

	// HACK TODO: fix this in machine description
	switch(MI->flat_insn->id) {
		default: break;
		case X86_INS_SYSEXIT:
				 SStream_Init(OS);
				 SStream_concat0(OS, "sysexit");
				 break;
	}

	if (MI->has_imm) {
		// if op_count > 1, then this operand's size is taken from the destination op
		if (MI->flat_insn->detail->x86.op_count > 1) {
			if (MI->flat_insn->id != X86_INS_LCALL && MI->flat_insn->id != X86_INS_LJMP) {
				for (i = 0; i < MI->flat_insn->detail->x86.op_count; i++) {
					if (MI->flat_insn->detail->x86.operands[i].type == X86_OP_IMM)
						MI->flat_insn->detail->x86.operands[i].size =
							MI->flat_insn->detail->x86.operands[MI->flat_insn->detail->x86.op_count - 1].size;
				}
			}
		} else
			MI->flat_insn->detail->x86.operands[0].size = MI->imm_size;
	}

	if (MI->csh->detail) {
		uint8_t access[6] = {0};

		// some instructions need to supply immediate 1 in the first op
		switch(MCInst_getOpcode(MI)) {
			default:
				break;
			case X86_SHL8r1:
			case X86_SHL16r1:
			case X86_SHL32r1:
			case X86_SHL64r1:
			case X86_SAL8r1:
			case X86_SAL16r1:
			case X86_SAL32r1:
			case X86_SAL64r1:
			case X86_SHR8r1:
			case X86_SHR16r1:
			case X86_SHR32r1:
			case X86_SHR64r1:
			case X86_SAR8r1:
			case X86_SAR16r1:
			case X86_SAR32r1:
			case X86_SAR64r1:
			case X86_RCL8r1:
			case X86_RCL16r1:
			case X86_RCL32r1:
			case X86_RCL64r1:
			case X86_RCR8r1:
			case X86_RCR16r1:
			case X86_RCR32r1:
			case X86_RCR64r1:
			case X86_ROL8r1:
			case X86_ROL16r1:
			case X86_ROL32r1:
			case X86_ROL64r1:
			case X86_ROR8r1:
			case X86_ROR16r1:
			case X86_ROR32r1:
			case X86_ROR64r1:
			case X86_SHL8m1:
			case X86_SHL16m1:
			case X86_SHL32m1:
			case X86_SHL64m1:
			case X86_SAL8m1:
			case X86_SAL16m1:
			case X86_SAL32m1:
			case X86_SAL64m1:
			case X86_SHR8m1:
			case X86_SHR16m1:
			case X86_SHR32m1:
			case X86_SHR64m1:
			case X86_SAR8m1:
			case X86_SAR16m1:
			case X86_SAR32m1:
			case X86_SAR64m1:
			case X86_RCL8m1:
			case X86_RCL16m1:
			case X86_RCL32m1:
			case X86_RCL64m1:
			case X86_RCR8m1:
			case X86_RCR16m1:
			case X86_RCR32m1:
			case X86_RCR64m1:
			case X86_ROL8m1:
			case X86_ROL16m1:
			case X86_ROL32m1:
			case X86_ROL64m1:
			case X86_ROR8m1:
			case X86_ROR16m1:
			case X86_ROR32m1:
			case X86_ROR64m1:
				// shift all the ops right to leave 1st slot for this new register op
				memmove(&(MI->flat_insn->detail->x86.operands[1]), &(MI->flat_insn->detail->x86.operands[0]),
						sizeof(MI->flat_insn->detail->x86.operands[0]) * (ARR_SIZE(MI->flat_insn->detail->x86.operands) - 1));
				MI->flat_insn->detail->x86.operands[0].type = X86_OP_IMM;
				MI->flat_insn->detail->x86.operands[0].imm = 1;
				MI->flat_insn->detail->x86.operands[0].size = 1;
				MI->flat_insn->detail->x86.op_count++;
		}

		// special instruction needs to supply register op
		// first op can be embedded in the asm by llvm.
		// so we have to add the missing register as the first operand

		//printf(">>> opcode = %u\n", MCInst_getOpcode(MI));

		reg = X86_insn_reg_att(MCInst_getOpcode(MI), &access1);
		if (reg) {
			// shift all the ops right to leave 1st slot for this new register op
			memmove(&(MI->flat_insn->detail->x86.operands[1]), &(MI->flat_insn->detail->x86.operands[0]),
					sizeof(MI->flat_insn->detail->x86.operands[0]) * (ARR_SIZE(MI->flat_insn->detail->x86.operands) - 1));
			MI->flat_insn->detail->x86.operands[0].type = X86_OP_REG;
			MI->flat_insn->detail->x86.operands[0].reg = reg;
			MI->flat_insn->detail->x86.operands[0].size = MI->csh->regsize_map[reg];
			MI->flat_insn->detail->x86.operands[0].access = access1;

			MI->flat_insn->detail->x86.op_count++;
		} else {
			if (X86_insn_reg_att2(MCInst_getOpcode(MI), &reg, &access1, &reg2, &access2)) {

				MI->flat_insn->detail->x86.operands[0].type = X86_OP_REG;
				MI->flat_insn->detail->x86.operands[0].reg = reg;
				MI->flat_insn->detail->x86.operands[0].size = MI->csh->regsize_map[reg];
				MI->flat_insn->detail->x86.operands[0].access = access1;
				MI->flat_insn->detail->x86.operands[1].type = X86_OP_REG;
				MI->flat_insn->detail->x86.operands[1].reg = reg2;
				MI->flat_insn->detail->x86.operands[1].size = MI->csh->regsize_map[reg2];
				MI->flat_insn->detail->x86.operands[0].access = access2;
				MI->flat_insn->detail->x86.op_count = 2;
			}
		}

#ifndef CAPSTONE_DIET
		get_op_access(MI->csh, MCInst_getOpcode(MI), access, &MI->flat_insn->detail->x86.eflags);
		MI->flat_insn->detail->x86.operands[0].access = access[0];
		MI->flat_insn->detail->x86.operands[1].access = access[1];
#endif
	}
}

#endif
