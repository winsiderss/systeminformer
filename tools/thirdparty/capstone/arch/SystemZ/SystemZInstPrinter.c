//===-- SystemZInstPrinter.cpp - Convert SystemZ MCInst to assembly syntax --------===//
//
//                     The LLVM Compiler Infrastructure
//
// This file is distributed under the University of Illinois Open Source
// License. See LICENSE.TXT for details.
//
//===----------------------------------------------------------------------===//
//
// This class prints an SystemZ MCInst to a .s file.
//
//===----------------------------------------------------------------------===//

/* Capstone Disassembly Engine */
/* By Nguyen Anh Quynh <aquynh@gmail.com>, 2013-2015 */

#ifdef CAPSTONE_HAS_SYSZ

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <capstone/platform.h>

#include "SystemZInstPrinter.h"
#include "../../MCInst.h"
#include "../../utils.h"
#include "../../SStream.h"
#include "../../MCRegisterInfo.h"
#include "../../MathExtras.h"
#include "SystemZMapping.h"

static const char *getRegisterName(unsigned RegNo);

void SystemZ_post_printer(csh ud, cs_insn *insn, char *insn_asm, MCInst *mci)
{
	/*
	   if (((cs_struct *)ud)->detail != CS_OPT_ON)
	   return;
	 */
}

static void printAddress(MCInst *MI, unsigned Base, int64_t Disp, unsigned Index, SStream *O)
{
	printInt64(O, Disp);

	if (Base) {
		SStream_concat0(O, "(");
		if (Index)
			SStream_concat(O, "%%%s, ", getRegisterName(Index));
		SStream_concat(O, "%%%s)", getRegisterName(Base));

		if (MI->csh->detail) {
			MI->flat_insn->detail->sysz.operands[MI->flat_insn->detail->sysz.op_count].type = SYSZ_OP_MEM;
			MI->flat_insn->detail->sysz.operands[MI->flat_insn->detail->sysz.op_count].mem.base = (uint8_t)SystemZ_map_register(Base);
			MI->flat_insn->detail->sysz.operands[MI->flat_insn->detail->sysz.op_count].mem.index = (uint8_t)SystemZ_map_register(Index);
			MI->flat_insn->detail->sysz.operands[MI->flat_insn->detail->sysz.op_count].mem.disp = Disp;
			MI->flat_insn->detail->sysz.op_count++;
		}
	} else if (!Index) {
		if (MI->csh->detail) {
			MI->flat_insn->detail->sysz.operands[MI->flat_insn->detail->sysz.op_count].type = SYSZ_OP_IMM;
			MI->flat_insn->detail->sysz.operands[MI->flat_insn->detail->sysz.op_count].imm = Disp;
			MI->flat_insn->detail->sysz.op_count++;
		}
	} else {
		SStream_concat(O, "(%%%s)", getRegisterName(Index));
		if (MI->csh->detail) {
			MI->flat_insn->detail->sysz.operands[MI->flat_insn->detail->sysz.op_count].type = SYSZ_OP_MEM;
			MI->flat_insn->detail->sysz.operands[MI->flat_insn->detail->sysz.op_count].mem.base = (uint8_t)SystemZ_map_register(Base);
			MI->flat_insn->detail->sysz.operands[MI->flat_insn->detail->sysz.op_count].mem.index = (uint8_t)SystemZ_map_register(Index);
			MI->flat_insn->detail->sysz.operands[MI->flat_insn->detail->sysz.op_count].mem.disp = Disp;
			MI->flat_insn->detail->sysz.op_count++;
		}
	}
}

static void _printOperand(MCInst *MI, MCOperand *MO, SStream *O)
{
	if (MCOperand_isReg(MO)) {
		unsigned reg;

		reg = MCOperand_getReg(MO);
		SStream_concat(O, "%%%s", getRegisterName(reg));
		reg = SystemZ_map_register(reg);

		if (MI->csh->detail) {
			MI->flat_insn->detail->sysz.operands[MI->flat_insn->detail->sysz.op_count].type = SYSZ_OP_REG;
			MI->flat_insn->detail->sysz.operands[MI->flat_insn->detail->sysz.op_count].reg = reg;
			MI->flat_insn->detail->sysz.op_count++;
		}
	} else if (MCOperand_isImm(MO)) {
		int64_t Imm = MCOperand_getImm(MO);

		printInt64(O, Imm);

		if (MI->csh->detail) {
			MI->flat_insn->detail->sysz.operands[MI->flat_insn->detail->sysz.op_count].type = SYSZ_OP_IMM;
			MI->flat_insn->detail->sysz.operands[MI->flat_insn->detail->sysz.op_count].imm = Imm;
			MI->flat_insn->detail->sysz.op_count++;
		}
	}
}

static void printU4ImmOperand(MCInst *MI, int OpNum, SStream *O)
{
	int64_t Value = MCOperand_getImm(MCInst_getOperand(MI, OpNum));
	// assert(isUInt<4>(Value) && "Invalid u4imm argument");
	printInt64(O, Value);

	if (MI->csh->detail) {
		MI->flat_insn->detail->sysz.operands[MI->flat_insn->detail->sysz.op_count].type = SYSZ_OP_IMM;
		MI->flat_insn->detail->sysz.operands[MI->flat_insn->detail->sysz.op_count].imm = Value;
		MI->flat_insn->detail->sysz.op_count++;
	}
}

static void printU6ImmOperand(MCInst *MI, int OpNum, SStream *O)
{
	uint32_t Value = (uint32_t)MCOperand_getImm(MCInst_getOperand(MI, OpNum));
	// assert(isUInt<6>(Value) && "Invalid u6imm argument");

	printUInt32(O, Value);

	if (MI->csh->detail) {
		MI->flat_insn->detail->sysz.operands[MI->flat_insn->detail->sysz.op_count].type = SYSZ_OP_IMM;
		MI->flat_insn->detail->sysz.operands[MI->flat_insn->detail->sysz.op_count].imm = (int64_t)Value;
		MI->flat_insn->detail->sysz.op_count++;
	}
}

static void printS8ImmOperand(MCInst *MI, int OpNum, SStream *O)
{
	int8_t Value = (int8_t)MCOperand_getImm(MCInst_getOperand(MI, OpNum));
	// assert(isInt<8>(Value) && "Invalid s8imm argument");

	if (Value >= 0) {
		if (Value > HEX_THRESHOLD)
			SStream_concat(O, "0x%x", Value);
		else
			SStream_concat(O, "%u", Value);
	} else {
		if (Value < -HEX_THRESHOLD)
			SStream_concat(O, "-0x%x", -Value);
		else
			SStream_concat(O, "-%u", -Value);
	}

	if (MI->csh->detail) {
		MI->flat_insn->detail->sysz.operands[MI->flat_insn->detail->sysz.op_count].type = SYSZ_OP_IMM;
		MI->flat_insn->detail->sysz.operands[MI->flat_insn->detail->sysz.op_count].imm = (int64_t)Value;
		MI->flat_insn->detail->sysz.op_count++;
	}
}

static void printU8ImmOperand(MCInst *MI, int OpNum, SStream *O)
{
	uint8_t Value = (uint8_t)MCOperand_getImm(MCInst_getOperand(MI, OpNum));
	// assert(isUInt<8>(Value) && "Invalid u8imm argument");

	if (Value > HEX_THRESHOLD)
		SStream_concat(O, "0x%x", Value);
	else
		SStream_concat(O, "%u", Value);

	if (MI->csh->detail) {
		MI->flat_insn->detail->sysz.operands[MI->flat_insn->detail->sysz.op_count].type = SYSZ_OP_IMM;
		MI->flat_insn->detail->sysz.operands[MI->flat_insn->detail->sysz.op_count].imm = (int64_t)Value;
		MI->flat_insn->detail->sysz.op_count++;
	}
}

static void printS16ImmOperand(MCInst *MI, int OpNum, SStream *O)
{
	int16_t Value = (int16_t)MCOperand_getImm(MCInst_getOperand(MI, OpNum));
	// assert(isInt<16>(Value) && "Invalid s16imm argument");

	if (Value >= 0) {
		if (Value > HEX_THRESHOLD)
			SStream_concat(O, "0x%x", Value);
		else
			SStream_concat(O, "%u", Value);
	} else {
		if (Value < -HEX_THRESHOLD)
			SStream_concat(O, "-0x%x", -Value);
		else
			SStream_concat(O, "-%u", -Value);
	}

	if (MI->csh->detail) {
		MI->flat_insn->detail->sysz.operands[MI->flat_insn->detail->sysz.op_count].type = SYSZ_OP_IMM;
		MI->flat_insn->detail->sysz.operands[MI->flat_insn->detail->sysz.op_count].imm = (int64_t)Value;
		MI->flat_insn->detail->sysz.op_count++;
	}
}

static void printU16ImmOperand(MCInst *MI, int OpNum, SStream *O)
{
	uint16_t Value = (uint16_t)MCOperand_getImm(MCInst_getOperand(MI, OpNum));
	// assert(isUInt<16>(Value) && "Invalid u16imm argument");

	if (Value > HEX_THRESHOLD)
		SStream_concat(O, "0x%x", Value);
	else
		SStream_concat(O, "%u", Value);

	if (MI->csh->detail) {
		MI->flat_insn->detail->sysz.operands[MI->flat_insn->detail->sysz.op_count].type = SYSZ_OP_IMM;
		MI->flat_insn->detail->sysz.operands[MI->flat_insn->detail->sysz.op_count].imm = (int64_t)Value;
		MI->flat_insn->detail->sysz.op_count++;
	}
}

static void printS32ImmOperand(MCInst *MI, int OpNum, SStream *O)
{
	int32_t Value = (int32_t)MCOperand_getImm(MCInst_getOperand(MI, OpNum));
	// assert(isInt<32>(Value) && "Invalid s32imm argument");

	printInt32(O, Value);

	if (MI->csh->detail) {
		MI->flat_insn->detail->sysz.operands[MI->flat_insn->detail->sysz.op_count].type = SYSZ_OP_IMM;
		MI->flat_insn->detail->sysz.operands[MI->flat_insn->detail->sysz.op_count].imm = (int64_t)Value;
		MI->flat_insn->detail->sysz.op_count++;
	}
}

static void printU32ImmOperand(MCInst *MI, int OpNum, SStream *O)
{
	uint32_t Value = (uint32_t)MCOperand_getImm(MCInst_getOperand(MI, OpNum));
	// assert(isUInt<32>(Value) && "Invalid u32imm argument");

	printUInt32(O, Value);

	if (MI->csh->detail) {
		MI->flat_insn->detail->sysz.operands[MI->flat_insn->detail->sysz.op_count].type = SYSZ_OP_IMM;
		MI->flat_insn->detail->sysz.operands[MI->flat_insn->detail->sysz.op_count].imm = (int64_t)Value;
		MI->flat_insn->detail->sysz.op_count++;
	}
}

static void printAccessRegOperand(MCInst *MI, int OpNum, SStream *O)
{
	int64_t Value = MCOperand_getImm(MCInst_getOperand(MI, OpNum));
	// assert(Value < 16 && "Invalid access register number");
	SStream_concat(O, "%%a%u", (unsigned int)Value);

	if (MI->csh->detail) {
		MI->flat_insn->detail->sysz.operands[MI->flat_insn->detail->sysz.op_count].type = SYSZ_OP_ACREG;
		MI->flat_insn->detail->sysz.operands[MI->flat_insn->detail->sysz.op_count].reg = (unsigned int)Value;
		MI->flat_insn->detail->sysz.op_count++;
	}
}

static void printPCRelOperand(MCInst *MI, int OpNum, SStream *O)
{
	MCOperand *MO = MCInst_getOperand(MI, OpNum);

	if (MCOperand_isImm(MO)) {
		int64_t imm = MCOperand_getImm(MO);

		printInt64(O, imm);

		if (MI->csh->detail) {
			MI->flat_insn->detail->sysz.operands[MI->flat_insn->detail->sysz.op_count].type = SYSZ_OP_IMM;
			MI->flat_insn->detail->sysz.operands[MI->flat_insn->detail->sysz.op_count].imm = imm;
			MI->flat_insn->detail->sysz.op_count++;
		}
	}
}

static void printPCRelTLSOperand(MCInst *MI, int OpNum, SStream *O)
{
	// Output the PC-relative operand.
	printPCRelOperand(MI, OpNum, O);
}

static void printOperand(MCInst *MI, int OpNum, SStream *O)
{
	_printOperand(MI, MCInst_getOperand(MI, OpNum), O);
}

static void printBDAddrOperand(MCInst *MI, int OpNum, SStream *O)
{
	printAddress(MI, MCOperand_getReg(MCInst_getOperand(MI, OpNum)),
			MCOperand_getImm(MCInst_getOperand(MI, OpNum + 1)), 0, O);
}

static void printBDXAddrOperand(MCInst *MI, int OpNum, SStream *O)
{
	printAddress(MI, MCOperand_getReg(MCInst_getOperand(MI, OpNum)),
			MCOperand_getImm(MCInst_getOperand(MI, OpNum + 1)),
			MCOperand_getReg(MCInst_getOperand(MI, OpNum + 2)), O);
}

static void printBDLAddrOperand(MCInst *MI, int OpNum, SStream *O)
{
	unsigned Base = MCOperand_getReg(MCInst_getOperand(MI, OpNum));
	uint64_t Disp = (uint64_t)MCOperand_getImm(MCInst_getOperand(MI, OpNum + 1));
	uint64_t Length = (uint64_t)MCOperand_getImm(MCInst_getOperand(MI, OpNum + 2));

	if (Disp > HEX_THRESHOLD)
		SStream_concat(O, "0x%"PRIx64, Disp);
	else
		SStream_concat(O, "%"PRIu64, Disp);

	if (Length > HEX_THRESHOLD)
		SStream_concat(O, "(0x%"PRIx64, Length);
	else
		SStream_concat(O, "(%"PRIu64, Length);

	if (Base)
		SStream_concat(O, ", %%%s", getRegisterName(Base));
	SStream_concat0(O, ")");

	if (MI->csh->detail) {
		MI->flat_insn->detail->sysz.operands[MI->flat_insn->detail->sysz.op_count].type = SYSZ_OP_MEM;
		MI->flat_insn->detail->sysz.operands[MI->flat_insn->detail->sysz.op_count].mem.base = (uint8_t)SystemZ_map_register(Base);
		MI->flat_insn->detail->sysz.operands[MI->flat_insn->detail->sysz.op_count].mem.length = Length;
		MI->flat_insn->detail->sysz.operands[MI->flat_insn->detail->sysz.op_count].mem.disp = (int64_t)Disp;
		MI->flat_insn->detail->sysz.op_count++;
	}
}

static void printCond4Operand(MCInst *MI, int OpNum, SStream *O)
{
	static const char *const CondNames[] = {
		"o", "h", "nle", "l", "nhe", "lh", "ne",
		"e", "nlh", "he", "nl", "le", "nh", "no"
	};

	uint64_t Imm = MCOperand_getImm(MCInst_getOperand(MI, OpNum));
	// assert(Imm > 0 && Imm < 15 && "Invalid condition");
	SStream_concat0(O, CondNames[Imm - 1]);

	if (MI->csh->detail)
		MI->flat_insn->detail->sysz.cc = (sysz_cc)Imm;
}

#define PRINT_ALIAS_INSTR
#include "SystemZGenAsmWriter.inc"

void SystemZ_printInst(MCInst *MI, SStream *O, void *Info)
{
	printInstruction(MI, O, Info);
}

#endif
