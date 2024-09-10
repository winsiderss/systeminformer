//==-- AArch64InstPrinter.cpp - Convert AArch64 MCInst to assembly syntax --==//
//
//                     The LLVM Compiler Infrastructure
//
// This file is distributed under the University of Illinois Open Source
// License. See LICENSE.TXT for details.
//
//===----------------------------------------------------------------------===//
//
// This class prints an AArch64 MCInst to a .s file.
//
//===----------------------------------------------------------------------===//

/* Capstone Disassembly Engine */
/* By Nguyen Anh Quynh <aquynh@gmail.com>, 2013-2016 */

#ifdef CAPSTONE_HAS_ARM64

#include "../capstone/include/capstone/platform.h"
#include <stdio.h>
#include <stdlib.h>

#include "AArch64InstPrinter.h"
#include "AArch64BaseInfo.h"
#include "../../utils.h"
#include "../../MCInst.h"
#include "../../SStream.h"
#include "../../MCRegisterInfo.h"
#include "../../MathExtras.h"

#include "AArch64Mapping.h"
#include "AArch64AddressingModes.h"

#define GET_REGINFO_ENUM
#include "AArch64GenRegisterInfo.inc"

#define GET_INSTRINFO_ENUM
#include "AArch64GenInstrInfo.inc"


static const char *getRegisterName(unsigned RegNo, int AltIdx);
static void printOperand(MCInst *MI, unsigned OpNo, SStream *O);
static bool printSysAlias(MCInst *MI, SStream *O);
static char *printAliasInstr(MCInst *MI, SStream *OS, void *info);
static void printInstruction(MCInst *MI, SStream *O, MCRegisterInfo *MRI);
static void printShifter(MCInst *MI, unsigned OpNum, SStream *O);

static cs_ac_type get_op_access(cs_struct *h, unsigned int id, unsigned int index)
{
#ifndef CAPSTONE_DIET
	uint8_t *arr = AArch64_get_op_access(h, id);

	if (arr[index] == CS_AC_IGNORE)
		return 0;

	return arr[index];
#else
	return 0;
#endif
}

static void set_mem_access(MCInst *MI, bool status)
{
	MI->csh->doing_mem = status;

	if (MI->csh->detail != CS_OPT_ON)
		return;

	if (status) {
#ifndef CAPSTONE_DIET
		uint8_t access;
		access = get_op_access(MI->csh, MCInst_getOpcode(MI), MI->ac_idx);
		MI->flat_insn->detail->arm64.operands[MI->flat_insn->detail->arm64.op_count].access = access;
		MI->ac_idx++;
#endif
		MI->flat_insn->detail->arm64.operands[MI->flat_insn->detail->arm64.op_count].type = ARM64_OP_MEM;
		MI->flat_insn->detail->arm64.operands[MI->flat_insn->detail->arm64.op_count].mem.base = ARM64_REG_INVALID;
		MI->flat_insn->detail->arm64.operands[MI->flat_insn->detail->arm64.op_count].mem.index = ARM64_REG_INVALID;
		MI->flat_insn->detail->arm64.operands[MI->flat_insn->detail->arm64.op_count].mem.disp = 0;
	} else {
		// done, create the next operand slot
		MI->flat_insn->detail->arm64.op_count++;
	}
}

void AArch64_printInst(MCInst *MI, SStream *O, void *Info)
{
	// Check for special encodings and print the canonical alias instead.
	unsigned Opcode = MCInst_getOpcode(MI);
	int LSB;
	int Width;
	char *mnem;

	if (Opcode == AArch64_SYSxt && printSysAlias(MI, O))
		return;

	// SBFM/UBFM should print to a nicer aliased form if possible.
	if (Opcode == AArch64_SBFMXri || Opcode == AArch64_SBFMWri ||
			Opcode == AArch64_UBFMXri || Opcode == AArch64_UBFMWri) {
		MCOperand *Op0 = MCInst_getOperand(MI, 0);
		MCOperand *Op1 = MCInst_getOperand(MI, 1);
		MCOperand *Op2 = MCInst_getOperand(MI, 2);
		MCOperand *Op3 = MCInst_getOperand(MI, 3);

		bool IsSigned = (Opcode == AArch64_SBFMXri || Opcode == AArch64_SBFMWri);
		bool Is64Bit = (Opcode == AArch64_SBFMXri || Opcode == AArch64_UBFMXri);

		if (MCOperand_isImm(Op2) && MCOperand_getImm(Op2) == 0 && MCOperand_isImm(Op3)) {
			const char *AsmMnemonic = NULL;

			switch (MCOperand_getImm(Op3)) {
				default:
					break;
				case 7:
					if (IsSigned)
						AsmMnemonic = "sxtb";
					else if (!Is64Bit)
						AsmMnemonic = "uxtb";
					break;
				case 15:
					if (IsSigned)
						AsmMnemonic = "sxth";
					else if (!Is64Bit)
						AsmMnemonic = "uxth";
					break;
				case 31:
					// *xtw is only valid for signed 64-bit operations.
					if (Is64Bit && IsSigned)
						AsmMnemonic = "sxtw";
					break;
			}

			if (AsmMnemonic) {
				SStream_concat(O, "%s\t%s, %s", AsmMnemonic,
						getRegisterName(MCOperand_getReg(Op0), AArch64_NoRegAltName),
						getRegisterName(getWRegFromXReg(MCOperand_getReg(Op1)), AArch64_NoRegAltName));

				if (MI->csh->detail) {
#ifndef CAPSTONE_DIET
					uint8_t access;
					access = get_op_access(MI->csh, MCInst_getOpcode(MI), MI->ac_idx);
					MI->flat_insn->detail->arm64.operands[MI->flat_insn->detail->arm64.op_count].access = access;
					MI->ac_idx++;
#endif
					MI->flat_insn->detail->arm64.operands[MI->flat_insn->detail->arm64.op_count].type = ARM64_OP_REG;
					MI->flat_insn->detail->arm64.operands[MI->flat_insn->detail->arm64.op_count].reg = MCOperand_getReg(Op0);
					MI->flat_insn->detail->arm64.op_count++;
#ifndef CAPSTONE_DIET
					access = get_op_access(MI->csh, MCInst_getOpcode(MI), MI->ac_idx);
					MI->flat_insn->detail->arm64.operands[MI->flat_insn->detail->arm64.op_count].access = access;
					MI->ac_idx++;
#endif
					MI->flat_insn->detail->arm64.operands[MI->flat_insn->detail->arm64.op_count].type = ARM64_OP_REG;
					MI->flat_insn->detail->arm64.operands[MI->flat_insn->detail->arm64.op_count].reg = getWRegFromXReg(MCOperand_getReg(Op1));
					MI->flat_insn->detail->arm64.op_count++;
				}

				MCInst_setOpcodePub(MI, AArch64_map_insn(AsmMnemonic));

				return;
			}
		}

		// All immediate shifts are aliases, implemented using the Bitfield
		// instruction. In all cases the immediate shift amount shift must be in
		// the range 0 to (reg.size -1).
		if (MCOperand_isImm(Op2) && MCOperand_isImm(Op3)) {
			const char *AsmMnemonic = NULL;
			int shift = 0;
			int immr = (int)MCOperand_getImm(Op2);
			int imms = (int)MCOperand_getImm(Op3);

			if (Opcode == AArch64_UBFMWri && imms != 0x1F && ((imms + 1) == immr)) {
				AsmMnemonic = "lsl";
				shift = 31 - imms;
			} else if (Opcode == AArch64_UBFMXri && imms != 0x3f &&
					((imms + 1 == immr))) {
				AsmMnemonic = "lsl";
				shift = 63 - imms;
			} else if (Opcode == AArch64_UBFMWri && imms == 0x1f) {
				AsmMnemonic = "lsr";
				shift = immr;
			} else if (Opcode == AArch64_UBFMXri && imms == 0x3f) {
				AsmMnemonic = "lsr";
				shift = immr;
			} else if (Opcode == AArch64_SBFMWri && imms == 0x1f) {
				AsmMnemonic = "asr";
				shift = immr;
			} else if (Opcode == AArch64_SBFMXri && imms == 0x3f) {
				AsmMnemonic = "asr";
				shift = immr;
			}

			if (AsmMnemonic) {
				SStream_concat(O, "%s\t%s, %s, ", AsmMnemonic,
						getRegisterName(MCOperand_getReg(Op0), AArch64_NoRegAltName),
						getRegisterName(MCOperand_getReg(Op1), AArch64_NoRegAltName));

				printInt32Bang(O, shift);

				MCInst_setOpcodePub(MI, AArch64_map_insn(AsmMnemonic));

				if (MI->csh->detail) {
#ifndef CAPSTONE_DIET
					uint8_t access;
					access = get_op_access(MI->csh, MCInst_getOpcode(MI), MI->ac_idx);
					MI->flat_insn->detail->arm64.operands[MI->flat_insn->detail->arm64.op_count].access = access;
					MI->ac_idx++;
#endif
					MI->flat_insn->detail->arm64.operands[MI->flat_insn->detail->arm64.op_count].type = ARM64_OP_REG;
					MI->flat_insn->detail->arm64.operands[MI->flat_insn->detail->arm64.op_count].reg = MCOperand_getReg(Op0);
					MI->flat_insn->detail->arm64.op_count++;
#ifndef CAPSTONE_DIET
					access = get_op_access(MI->csh, MCInst_getOpcode(MI), MI->ac_idx);
					MI->flat_insn->detail->arm64.operands[MI->flat_insn->detail->arm64.op_count].access = access;
					MI->ac_idx++;
#endif
					MI->flat_insn->detail->arm64.operands[MI->flat_insn->detail->arm64.op_count].type = ARM64_OP_REG;
					MI->flat_insn->detail->arm64.operands[MI->flat_insn->detail->arm64.op_count].reg = MCOperand_getReg(Op1);
					MI->flat_insn->detail->arm64.op_count++;
#ifndef CAPSTONE_DIET
					access = get_op_access(MI->csh, MCInst_getOpcode(MI), MI->ac_idx);
					MI->flat_insn->detail->arm64.operands[MI->flat_insn->detail->arm64.op_count].access = access;
					MI->ac_idx++;
#endif
					MI->flat_insn->detail->arm64.operands[MI->flat_insn->detail->arm64.op_count].type = ARM64_OP_IMM;
					MI->flat_insn->detail->arm64.operands[MI->flat_insn->detail->arm64.op_count].imm = shift;
					MI->flat_insn->detail->arm64.op_count++;
				}

				return;
			}
		}

		// SBFIZ/UBFIZ aliases
		if (MCOperand_getImm(Op2) > MCOperand_getImm(Op3)) {
			SStream_concat(O, "%s\t%s, %s, ", (IsSigned ? "sbfiz" : "ubfiz"),
					getRegisterName(MCOperand_getReg(Op0), AArch64_NoRegAltName),
					getRegisterName(MCOperand_getReg(Op1), AArch64_NoRegAltName));
			printInt32Bang(O, (int)((Is64Bit ? 64 : 32) - MCOperand_getImm(Op2)));
			SStream_concat0(O, ", ");
			printInt32Bang(O, (int)MCOperand_getImm(Op3) + 1);

			MCInst_setOpcodePub(MI, AArch64_map_insn(IsSigned ? "sbfiz" : "ubfiz"));

			if (MI->csh->detail) {
#ifndef CAPSTONE_DIET
				uint8_t access;
				access = get_op_access(MI->csh, MCInst_getOpcode(MI), MI->ac_idx);
				MI->flat_insn->detail->arm64.operands[MI->flat_insn->detail->arm64.op_count].access = access;
				MI->ac_idx++;
#endif
				MI->flat_insn->detail->arm64.operands[MI->flat_insn->detail->arm64.op_count].type = ARM64_OP_REG;
				MI->flat_insn->detail->arm64.operands[MI->flat_insn->detail->arm64.op_count].reg = MCOperand_getReg(Op0);
				MI->flat_insn->detail->arm64.op_count++;
#ifndef CAPSTONE_DIET
				access = get_op_access(MI->csh, MCInst_getOpcode(MI), MI->ac_idx);
				MI->flat_insn->detail->arm64.operands[MI->flat_insn->detail->arm64.op_count].access = access;
				MI->ac_idx++;
#endif
				MI->flat_insn->detail->arm64.operands[MI->flat_insn->detail->arm64.op_count].type = ARM64_OP_REG;
				MI->flat_insn->detail->arm64.operands[MI->flat_insn->detail->arm64.op_count].reg = MCOperand_getReg(Op1);
				MI->flat_insn->detail->arm64.op_count++;
#ifndef CAPSTONE_DIET
				access = get_op_access(MI->csh, MCInst_getOpcode(MI), MI->ac_idx);
				MI->flat_insn->detail->arm64.operands[MI->flat_insn->detail->arm64.op_count].access = access;
				MI->ac_idx++;
#endif
				MI->flat_insn->detail->arm64.operands[MI->flat_insn->detail->arm64.op_count].type = ARM64_OP_IMM;
				MI->flat_insn->detail->arm64.operands[MI->flat_insn->detail->arm64.op_count].imm = (Is64Bit ? 64 : 32) - (int)MCOperand_getImm(Op2);
				MI->flat_insn->detail->arm64.op_count++;
#ifndef CAPSTONE_DIET
				access = get_op_access(MI->csh, MCInst_getOpcode(MI), MI->ac_idx);
				MI->flat_insn->detail->arm64.operands[MI->flat_insn->detail->arm64.op_count].access = access;
				MI->ac_idx++;
#endif
				MI->flat_insn->detail->arm64.operands[MI->flat_insn->detail->arm64.op_count].type = ARM64_OP_IMM;
				MI->flat_insn->detail->arm64.operands[MI->flat_insn->detail->arm64.op_count].imm = MCOperand_getImm(Op3) + 1;
				MI->flat_insn->detail->arm64.op_count++;
			}

			return;
		}

		// Otherwise SBFX/UBFX is the preferred form
		SStream_concat(O, "%s\t%s, %s, ", (IsSigned ? "sbfx" : "ubfx"),
				getRegisterName(MCOperand_getReg(Op0), AArch64_NoRegAltName),
				getRegisterName(MCOperand_getReg(Op1), AArch64_NoRegAltName));
		printInt32Bang(O, (int)MCOperand_getImm(Op2));
		SStream_concat0(O, ", ");
		printInt32Bang(O, (int)MCOperand_getImm(Op3) - (int)MCOperand_getImm(Op2) + 1);

		MCInst_setOpcodePub(MI, AArch64_map_insn(IsSigned ? "sbfx" : "ubfx"));

		if (MI->csh->detail) {
#ifndef CAPSTONE_DIET
			uint8_t access;
			access = get_op_access(MI->csh, MCInst_getOpcode(MI), MI->ac_idx);
			MI->flat_insn->detail->arm64.operands[MI->flat_insn->detail->arm64.op_count].access = access;
			MI->ac_idx++;
#endif
			MI->flat_insn->detail->arm64.operands[MI->flat_insn->detail->arm64.op_count].type = ARM64_OP_REG;
			MI->flat_insn->detail->arm64.operands[MI->flat_insn->detail->arm64.op_count].reg = MCOperand_getReg(Op0);
			MI->flat_insn->detail->arm64.op_count++;
#ifndef CAPSTONE_DIET
			access = get_op_access(MI->csh, MCInst_getOpcode(MI), MI->ac_idx);
			MI->flat_insn->detail->arm64.operands[MI->flat_insn->detail->arm64.op_count].access = access;
			MI->ac_idx++;
#endif
			MI->flat_insn->detail->arm64.operands[MI->flat_insn->detail->arm64.op_count].type = ARM64_OP_REG;
			MI->flat_insn->detail->arm64.operands[MI->flat_insn->detail->arm64.op_count].reg = MCOperand_getReg(Op1);
			MI->flat_insn->detail->arm64.op_count++;
#ifndef CAPSTONE_DIET
			access = get_op_access(MI->csh, MCInst_getOpcode(MI), MI->ac_idx);
			MI->flat_insn->detail->arm64.operands[MI->flat_insn->detail->arm64.op_count].access = access;
			MI->ac_idx++;
#endif
			MI->flat_insn->detail->arm64.operands[MI->flat_insn->detail->arm64.op_count].type = ARM64_OP_IMM;
			MI->flat_insn->detail->arm64.operands[MI->flat_insn->detail->arm64.op_count].imm = MCOperand_getImm(Op2);
			MI->flat_insn->detail->arm64.op_count++;
#ifndef CAPSTONE_DIET
			access = get_op_access(MI->csh, MCInst_getOpcode(MI), MI->ac_idx);
			MI->flat_insn->detail->arm64.operands[MI->flat_insn->detail->arm64.op_count].access = access;
			MI->ac_idx++;
#endif
			MI->flat_insn->detail->arm64.operands[MI->flat_insn->detail->arm64.op_count].type = ARM64_OP_IMM;
			MI->flat_insn->detail->arm64.operands[MI->flat_insn->detail->arm64.op_count].imm = MCOperand_getImm(Op3) - MCOperand_getImm(Op2) + 1;
			MI->flat_insn->detail->arm64.op_count++;
		}

		return;
	}

	if (Opcode == AArch64_BFMXri || Opcode == AArch64_BFMWri) {
		MCOperand *Op0 = MCInst_getOperand(MI, 0); // Op1 == Op0
		MCOperand *Op2 = MCInst_getOperand(MI, 2);
		int ImmR = (int)MCOperand_getImm(MCInst_getOperand(MI, 3));
		int ImmS = (int)MCOperand_getImm(MCInst_getOperand(MI, 4));

		// BFI alias
		if (ImmS < ImmR) {
			int BitWidth = Opcode == AArch64_BFMXri ? 64 : 32;
			LSB = (BitWidth - ImmR) % BitWidth;
			Width = ImmS + 1;

			SStream_concat(O, "bfi\t%s, %s, ",
					getRegisterName(MCOperand_getReg(Op0), AArch64_NoRegAltName),
					getRegisterName(MCOperand_getReg(Op2), AArch64_NoRegAltName));
			printInt32Bang(O, LSB);
			SStream_concat0(O, ", ");
			printInt32Bang(O, Width);
			MCInst_setOpcodePub(MI, AArch64_map_insn("bfi"));

			if (MI->csh->detail) {
#ifndef CAPSTONE_DIET
				uint8_t access;
				access = get_op_access(MI->csh, MCInst_getOpcode(MI), MI->ac_idx);
				MI->flat_insn->detail->arm64.operands[MI->flat_insn->detail->arm64.op_count].access = access;
				MI->ac_idx++;
#endif
				MI->flat_insn->detail->arm64.operands[MI->flat_insn->detail->arm64.op_count].type = ARM64_OP_REG;
				MI->flat_insn->detail->arm64.operands[MI->flat_insn->detail->arm64.op_count].reg = MCOperand_getReg(Op0);
				MI->flat_insn->detail->arm64.op_count++;
#ifndef CAPSTONE_DIET
				access = get_op_access(MI->csh, MCInst_getOpcode(MI), MI->ac_idx);
				MI->flat_insn->detail->arm64.operands[MI->flat_insn->detail->arm64.op_count].access = access;
				MI->ac_idx++;
#endif
				MI->flat_insn->detail->arm64.operands[MI->flat_insn->detail->arm64.op_count].type = ARM64_OP_REG;
				MI->flat_insn->detail->arm64.operands[MI->flat_insn->detail->arm64.op_count].reg = MCOperand_getReg(Op2);
				MI->flat_insn->detail->arm64.op_count++;
#ifndef CAPSTONE_DIET
				access = get_op_access(MI->csh, MCInst_getOpcode(MI), MI->ac_idx);
				MI->flat_insn->detail->arm64.operands[MI->flat_insn->detail->arm64.op_count].access = access;
				MI->ac_idx++;
#endif
				MI->flat_insn->detail->arm64.operands[MI->flat_insn->detail->arm64.op_count].type = ARM64_OP_IMM;
				MI->flat_insn->detail->arm64.operands[MI->flat_insn->detail->arm64.op_count].imm = LSB;
				MI->flat_insn->detail->arm64.op_count++;
#ifndef CAPSTONE_DIET
				access = get_op_access(MI->csh, MCInst_getOpcode(MI), MI->ac_idx);
				MI->flat_insn->detail->arm64.operands[MI->flat_insn->detail->arm64.op_count].access = access;
				MI->ac_idx++;
#endif
				MI->flat_insn->detail->arm64.operands[MI->flat_insn->detail->arm64.op_count].type = ARM64_OP_IMM;
				MI->flat_insn->detail->arm64.operands[MI->flat_insn->detail->arm64.op_count].imm = Width;
				MI->flat_insn->detail->arm64.op_count++;
			}

			return;
		}

		LSB = ImmR;
		Width = ImmS - ImmR + 1;
		// Otherwise BFXIL the preferred form
		SStream_concat(O, "bfxil\t%s, %s, ",
				getRegisterName(MCOperand_getReg(Op0), AArch64_NoRegAltName),
				getRegisterName(MCOperand_getReg(Op2), AArch64_NoRegAltName));
		printInt32Bang(O, LSB);
		SStream_concat0(O, ", ");
		printInt32Bang(O, Width);
		MCInst_setOpcodePub(MI, AArch64_map_insn("bfxil"));

		if (MI->csh->detail) {
#ifndef CAPSTONE_DIET
			uint8_t access;
			access = get_op_access(MI->csh, MCInst_getOpcode(MI), MI->ac_idx);
			MI->flat_insn->detail->arm64.operands[MI->flat_insn->detail->arm64.op_count].access = access;
			MI->ac_idx++;
#endif
			MI->flat_insn->detail->arm64.operands[MI->flat_insn->detail->arm64.op_count].type = ARM64_OP_REG;
			MI->flat_insn->detail->arm64.operands[MI->flat_insn->detail->arm64.op_count].reg = MCOperand_getReg(Op0);
			MI->flat_insn->detail->arm64.op_count++;
#ifndef CAPSTONE_DIET
			access = get_op_access(MI->csh, MCInst_getOpcode(MI), MI->ac_idx);
			MI->flat_insn->detail->arm64.operands[MI->flat_insn->detail->arm64.op_count].access = access;
			MI->ac_idx++;
#endif
			MI->flat_insn->detail->arm64.operands[MI->flat_insn->detail->arm64.op_count].type = ARM64_OP_REG;
			MI->flat_insn->detail->arm64.operands[MI->flat_insn->detail->arm64.op_count].reg = MCOperand_getReg(Op2);
			MI->flat_insn->detail->arm64.op_count++;
#ifndef CAPSTONE_DIET
			access = get_op_access(MI->csh, MCInst_getOpcode(MI), MI->ac_idx);
			MI->flat_insn->detail->arm64.operands[MI->flat_insn->detail->arm64.op_count].access = access;
			MI->ac_idx++;
#endif
			MI->flat_insn->detail->arm64.operands[MI->flat_insn->detail->arm64.op_count].type = ARM64_OP_IMM;
			MI->flat_insn->detail->arm64.operands[MI->flat_insn->detail->arm64.op_count].imm = LSB;
			MI->flat_insn->detail->arm64.op_count++;
#ifndef CAPSTONE_DIET
			access = get_op_access(MI->csh, MCInst_getOpcode(MI), MI->ac_idx);
			MI->flat_insn->detail->arm64.operands[MI->flat_insn->detail->arm64.op_count].access = access;
			MI->ac_idx++;
#endif
			MI->flat_insn->detail->arm64.operands[MI->flat_insn->detail->arm64.op_count].type = ARM64_OP_IMM;
			MI->flat_insn->detail->arm64.operands[MI->flat_insn->detail->arm64.op_count].imm = Width;
			MI->flat_insn->detail->arm64.op_count++;
		}

		return;
	}

	mnem = printAliasInstr(MI, O, Info);
	if (mnem) {
		MCInst_setOpcodePub(MI, AArch64_map_insn(mnem));
		cs_mem_free(mnem);

		switch(MCInst_getOpcode(MI)) {
			default: break;
			case AArch64_UMOVvi64:
				 arm64_op_addVectorElementSizeSpecifier(MI, ARM64_VESS_D);
				 break;
			case AArch64_UMOVvi32:
				 arm64_op_addVectorElementSizeSpecifier(MI, ARM64_VESS_S);
				 break;
		}
	} else {
		printInstruction(MI, O, Info);
	}
}

static bool printSysAlias(MCInst *MI, SStream *O)
{
	// unsigned Opcode = MCInst_getOpcode(MI);
	//assert(Opcode == AArch64_SYSxt && "Invalid opcode for SYS alias!");

	const char *Asm = NULL;
	MCOperand *Op1 = MCInst_getOperand(MI, 0);
	MCOperand *Cn = MCInst_getOperand(MI, 1);
	MCOperand *Cm = MCInst_getOperand(MI, 2);
	MCOperand *Op2 = MCInst_getOperand(MI, 3);

	unsigned Op1Val = (unsigned)MCOperand_getImm(Op1);
	unsigned CnVal = (unsigned)MCOperand_getImm(Cn);
	unsigned CmVal = (unsigned)MCOperand_getImm(Cm);
	unsigned Op2Val = (unsigned)MCOperand_getImm(Op2);
	unsigned insn_id = ARM64_INS_INVALID;
	unsigned op_ic = 0, op_dc = 0, op_at = 0, op_tlbi = 0;

	if (CnVal == 7) {
		switch (CmVal) {
			default:
				break;

				// IC aliases
			case 1:
				if (Op1Val == 0 && Op2Val == 0) {
					Asm = "ic\tialluis";
					insn_id = ARM64_INS_IC;
					op_ic = ARM64_IC_IALLUIS;
				}
				break;
			case 5:
				if (Op1Val == 0 && Op2Val == 0) {
					Asm = "ic\tiallu";
					insn_id = ARM64_INS_IC;
					op_ic = ARM64_IC_IALLU;
				} else if (Op1Val == 3 && Op2Val == 1) {
					Asm = "ic\tivau";
					insn_id = ARM64_INS_IC;
					op_ic = ARM64_IC_IVAU;
				}
				break;

				// DC aliases
			case 4:
				if (Op1Val == 3 && Op2Val == 1) {
					Asm = "dc\tzva";
					insn_id = ARM64_INS_DC;
					op_dc = ARM64_DC_ZVA;
				}
				break;
			case 6:
				if (Op1Val == 0 && Op2Val == 1) {
					Asm = "dc\tivac";
					insn_id = ARM64_INS_DC;
					op_dc = ARM64_DC_IVAC;
				}
				if (Op1Val == 0 && Op2Val == 2) {
					Asm = "dc\tisw";
					insn_id = ARM64_INS_DC;
					op_dc = ARM64_DC_ISW;
				}
				break;
			case 10:
				if (Op1Val == 3 && Op2Val == 1) {
					Asm = "dc\tcvac";
					insn_id = ARM64_INS_DC;
					op_dc = ARM64_DC_CVAC;
				} else if (Op1Val == 0 && Op2Val == 2) {
					Asm = "dc\tcsw";
					insn_id = ARM64_INS_DC;
					op_dc = ARM64_DC_CSW;
				}
				break;
			case 11:
				if (Op1Val == 3 && Op2Val == 1) {
					Asm = "dc\tcvau";
					insn_id = ARM64_INS_DC;
					op_dc = ARM64_DC_CVAU;
				}
				break;
			case 14:
				if (Op1Val == 3 && Op2Val == 1) {
					Asm = "dc\tcivac";
					insn_id = ARM64_INS_DC;
					op_dc = ARM64_DC_CIVAC;
				} else if (Op1Val == 0 && Op2Val == 2) {
					Asm = "dc\tcisw";
					insn_id = ARM64_INS_DC;
					op_dc = ARM64_DC_CISW;
				}
				break;

				// AT aliases
			case 8:
				switch (Op1Val) {
					default:
						break;
					case 0:
						switch (Op2Val) {
							default:
								break;
							case 0: Asm = "at\ts1e1r"; insn_id = ARM64_INS_AT; op_at = ARM64_AT_S1E1R; break;
							case 1: Asm = "at\ts1e1w"; insn_id = ARM64_INS_AT; op_at = ARM64_AT_S1E1W; break;
							case 2: Asm = "at\ts1e0r"; insn_id = ARM64_INS_AT; op_at = ARM64_AT_S1E0R; break;
							case 3: Asm = "at\ts1e0w"; insn_id = ARM64_INS_AT; op_at = ARM64_AT_S1E0W; break;
						}
						break;
					case 4:
						switch (Op2Val) {
							default:
								break;
							case 0: Asm = "at\ts1e2r"; insn_id = ARM64_INS_AT; op_at = ARM64_AT_S1E2R; break;
							case 1: Asm = "at\ts1e2w"; insn_id = ARM64_INS_AT; op_at = ARM64_AT_S1E2W; break;
							case 4: Asm = "at\ts12e1r"; insn_id = ARM64_INS_AT; op_at = ARM64_AT_S1E1R; break;
							case 5: Asm = "at\ts12e1w"; insn_id = ARM64_INS_AT; op_at = ARM64_AT_S1E1W; break;
							case 6: Asm = "at\ts12e0r"; insn_id = ARM64_INS_AT; op_at = ARM64_AT_S1E0R; break;
							case 7: Asm = "at\ts12e0w"; insn_id = ARM64_INS_AT; op_at = ARM64_AT_S1E0W; break;
						}
						break;
					case 6:
						switch (Op2Val) {
							default:
								break;
							case 0: Asm = "at\ts1e3r"; insn_id = ARM64_INS_AT; op_at = ARM64_AT_S1E3R; break;
							case 1: Asm = "at\ts1e3w"; insn_id = ARM64_INS_AT; op_at = ARM64_AT_S1E3W; break;
						}
						break;
				}
				break;
		}
	} else if (CnVal == 8) {
		// TLBI aliases
		switch (CmVal) {
			default:
				break;
			case 3:
				switch (Op1Val) {
					default:
						break;
					case 0:
						switch (Op2Val) {
							default:
								break;
							case 0: Asm = "tlbi\tvmalle1is"; insn_id = ARM64_INS_TLBI; op_tlbi = ARM64_TLBI_VMALLE1IS; break;
							case 1: Asm = "tlbi\tvae1is"; insn_id = ARM64_INS_TLBI; op_tlbi = ARM64_TLBI_VAE1IS; break;
							case 2: Asm = "tlbi\taside1is"; insn_id = ARM64_INS_TLBI; op_tlbi = ARM64_TLBI_ASIDE1IS; break;
							case 3: Asm = "tlbi\tvaae1is"; insn_id = ARM64_INS_TLBI; op_tlbi = ARM64_TLBI_VAAE1IS; break;
							case 5: Asm = "tlbi\tvale1is"; insn_id = ARM64_INS_TLBI; op_tlbi = ARM64_TLBI_VALE1IS; break;
							case 7: Asm = "tlbi\tvaale1is"; insn_id = ARM64_INS_TLBI; op_tlbi = ARM64_TLBI_VAALE1IS; break;
						}
						break;
					case 4:
						switch (Op2Val) {
							default:
								break;
							case 0: Asm = "tlbi\talle2is"; insn_id = ARM64_INS_TLBI; op_tlbi = ARM64_TLBI_ALLE2IS; break;
							case 1: Asm = "tlbi\tvae2is"; insn_id = ARM64_INS_TLBI; op_tlbi = ARM64_TLBI_VAE2IS; break;
							case 4: Asm = "tlbi\talle1is"; insn_id = ARM64_INS_TLBI; op_tlbi = ARM64_TLBI_ALLE1IS; break;
							case 5: Asm = "tlbi\tvale2is"; insn_id = ARM64_INS_TLBI; op_tlbi = ARM64_TLBI_VALE2IS; break;
							case 6: Asm = "tlbi\tvmalls12e1is"; insn_id = ARM64_INS_TLBI; op_tlbi = ARM64_TLBI_VMALLS12E1IS; break;
						}
						break;
					case 6:
						switch (Op2Val) {
							default:
								break;
							case 0: Asm = "tlbi\talle3is"; insn_id = ARM64_INS_TLBI; op_tlbi = ARM64_TLBI_ALLE3IS; break;
							case 1: Asm = "tlbi\tvae3is"; insn_id = ARM64_INS_TLBI; op_tlbi = ARM64_TLBI_VAE3IS; break;
							case 5: Asm = "tlbi\tvale3is"; insn_id = ARM64_INS_TLBI; op_tlbi = ARM64_TLBI_VALE3IS; break;
						}
						break;
				}
				break;
			case 0:
				switch (Op1Val) {
					default:
						break;
					case 4:
						switch (Op2Val) {
							default:
								break;
							case 1: Asm = "tlbi\tipas2e1is"; insn_id = ARM64_INS_TLBI; op_tlbi = ARM64_TLBI_IPAS2E1IS; break;
							case 5: Asm = "tlbi\tipas2le1is"; insn_id = ARM64_INS_TLBI; op_tlbi = ARM64_TLBI_IPAS2LE1IS; break;
						}
						break;
				}
				break;
			case 4:
				switch (Op1Val) {
					default:
						break;
					case 4:
						switch (Op2Val) {
							default:
								break;
							case 1: Asm = "tlbi\tipas2e1"; insn_id = ARM64_INS_TLBI; op_tlbi = ARM64_TLBI_IPAS2E1; break;
							case 5: Asm = "tlbi\tipas2le1"; insn_id = ARM64_INS_TLBI; op_tlbi = ARM64_TLBI_IPAS2LE1; break;
						}
						break;
				}
				break;
			case 7:
				switch (Op1Val) {
					default:
						break;
					case 0:
						switch (Op2Val) {
							default:
								break;
							case 0: Asm = "tlbi\tvmalle1"; insn_id = ARM64_INS_TLBI; op_tlbi = ARM64_TLBI_VMALLE1; break;
							case 1: Asm = "tlbi\tvae1"; insn_id = ARM64_INS_TLBI; op_tlbi = ARM64_TLBI_VAE1; break;
							case 2: Asm = "tlbi\taside1"; insn_id = ARM64_INS_TLBI; op_tlbi = ARM64_TLBI_ASIDE1; break;
							case 3: Asm = "tlbi\tvaae1"; insn_id = ARM64_INS_TLBI; op_tlbi = ARM64_TLBI_VAAE1; break;
							case 5: Asm = "tlbi\tvale1"; insn_id = ARM64_INS_TLBI; op_tlbi = ARM64_TLBI_VALE1; break;
							case 7: Asm = "tlbi\tvaale1"; insn_id = ARM64_INS_TLBI; op_tlbi = ARM64_TLBI_VAALE1; break;
						}
						break;
					case 4:
						switch (Op2Val) {
							default:
								break;
							case 0: Asm = "tlbi\talle2"; insn_id = ARM64_INS_TLBI; op_tlbi = ARM64_TLBI_ALLE2; break;
							case 1: Asm = "tlbi\tvae2"; insn_id = ARM64_INS_TLBI; op_tlbi = ARM64_TLBI_VAE2; break;
							case 4: Asm = "tlbi\talle1"; insn_id = ARM64_INS_TLBI; op_tlbi = ARM64_TLBI_ALLE1; break;
							case 5: Asm = "tlbi\tvale2"; insn_id = ARM64_INS_TLBI; op_tlbi = ARM64_TLBI_VALE2; break;
							case 6: Asm = "tlbi\tvmalls12e1"; insn_id = ARM64_INS_TLBI; op_tlbi = ARM64_TLBI_VMALLS12E1; break;
						}
						break;
					case 6:
						switch (Op2Val) {
							default:
								break;
							case 0: Asm = "tlbi\talle3"; insn_id = ARM64_INS_TLBI; op_tlbi = ARM64_TLBI_ALLE3; break;
							case 1: Asm = "tlbi\tvae3"; insn_id = ARM64_INS_TLBI;  op_tlbi = ARM64_TLBI_VAE3; break;
							case 5: Asm = "tlbi\tvale3"; insn_id = ARM64_INS_TLBI; op_tlbi = ARM64_TLBI_VALE3; break;
						}
						break;
				}
				break;
		}
	}

	if (Asm) {
		MCInst_setOpcodePub(MI, insn_id);
		SStream_concat0(O, Asm);
		if (MI->csh->detail) {
#ifndef CAPSTONE_DIET
			uint8_t access;
			access = get_op_access(MI->csh, MCInst_getOpcode(MI), MI->ac_idx);
			MI->flat_insn->detail->arm64.operands[MI->flat_insn->detail->arm64.op_count].access = access;
			MI->ac_idx++;
#endif
			MI->flat_insn->detail->arm64.operands[MI->flat_insn->detail->arm64.op_count].type = ARM64_OP_SYS;
			MI->flat_insn->detail->arm64.operands[MI->flat_insn->detail->arm64.op_count].sys = op_ic + op_dc + op_at + op_tlbi;
			MI->flat_insn->detail->arm64.op_count++;
		}

		if (!strstr(Asm, "all")) {
			unsigned Reg = MCOperand_getReg(MCInst_getOperand(MI, 4));
			SStream_concat(O, ", %s", getRegisterName(Reg, AArch64_NoRegAltName));
			if (MI->csh->detail) {
#ifndef CAPSTONE_DIET
				uint8_t access;
				access = get_op_access(MI->csh, MCInst_getOpcode(MI), MI->ac_idx);
				MI->flat_insn->detail->arm64.operands[MI->flat_insn->detail->arm64.op_count].access = access;
				MI->ac_idx++;
#endif
				MI->flat_insn->detail->arm64.operands[MI->flat_insn->detail->arm64.op_count].type = ARM64_OP_REG;
				MI->flat_insn->detail->arm64.operands[MI->flat_insn->detail->arm64.op_count].reg = Reg;
				MI->flat_insn->detail->arm64.op_count++;
			}
		}
	}

	return Asm != NULL;
}

static void printOperand(MCInst *MI, unsigned OpNo, SStream *O)
{
	MCOperand *Op = MCInst_getOperand(MI, OpNo);

	if (MCOperand_isReg(Op)) {
		unsigned Reg = MCOperand_getReg(Op);
		SStream_concat0(O, getRegisterName(Reg, AArch64_NoRegAltName));
		if (MI->csh->detail) {
			if (MI->csh->doing_mem) {
				if (MI->flat_insn->detail->arm64.operands[MI->flat_insn->detail->arm64.op_count].mem.base == ARM64_REG_INVALID) {
					MI->flat_insn->detail->arm64.operands[MI->flat_insn->detail->arm64.op_count].mem.base = Reg;
				}
				else if (MI->flat_insn->detail->arm64.operands[MI->flat_insn->detail->arm64.op_count].mem.index == ARM64_REG_INVALID) {
					MI->flat_insn->detail->arm64.operands[MI->flat_insn->detail->arm64.op_count].mem.index = Reg;
				}
			} else {
#ifndef CAPSTONE_DIET
				uint8_t access;
				access = get_op_access(MI->csh, MCInst_getOpcode(MI), MI->ac_idx);
				MI->flat_insn->detail->arm64.operands[MI->flat_insn->detail->arm64.op_count].access = access;
				MI->ac_idx++;
#endif
				MI->flat_insn->detail->arm64.operands[MI->flat_insn->detail->arm64.op_count].type = ARM64_OP_REG;
				MI->flat_insn->detail->arm64.operands[MI->flat_insn->detail->arm64.op_count].reg = Reg;
				MI->flat_insn->detail->arm64.op_count++;
			}
		}
	} else if (MCOperand_isImm(Op)) {
		int64_t imm = MCOperand_getImm(Op);

		if (MI->Opcode == AArch64_ADR) {
			imm += MI->address;
			printUInt64Bang(O, imm);
		} else {
			if (MI->csh->doing_mem) {
				if (MI->csh->imm_unsigned)
					printUInt64Bang(O, imm);
				else
					printInt64Bang(O, imm);
			} else
				printUInt64Bang(O, imm);
		}

		if (MI->csh->detail) {
			if (MI->csh->doing_mem) {
				MI->flat_insn->detail->arm64.operands[MI->flat_insn->detail->arm64.op_count].mem.disp = (int32_t)imm;
			} else {
#ifndef CAPSTONE_DIET
				uint8_t access;
				access = get_op_access(MI->csh, MCInst_getOpcode(MI), MI->ac_idx);
				MI->flat_insn->detail->arm64.operands[MI->flat_insn->detail->arm64.op_count].access = access;
				MI->ac_idx++;
#endif
				MI->flat_insn->detail->arm64.operands[MI->flat_insn->detail->arm64.op_count].type = ARM64_OP_IMM;
				MI->flat_insn->detail->arm64.operands[MI->flat_insn->detail->arm64.op_count].imm = imm;
				MI->flat_insn->detail->arm64.op_count++;
			}
		}
	}
}

static void printHexImm(MCInst *MI, unsigned OpNo, SStream *O)
{
	MCOperand *Op = MCInst_getOperand(MI, OpNo);
	SStream_concat(O, "#%#llx", MCOperand_getImm(Op));
	if (MI->csh->detail) {
#ifndef CAPSTONE_DIET
		uint8_t access;
		access = get_op_access(MI->csh, MCInst_getOpcode(MI), MI->ac_idx);
		MI->flat_insn->detail->arm64.operands[MI->flat_insn->detail->arm64.op_count].access = access;
		MI->ac_idx++;
#endif
		MI->flat_insn->detail->arm64.operands[MI->flat_insn->detail->arm64.op_count].type = ARM64_OP_IMM;
		MI->flat_insn->detail->arm64.operands[MI->flat_insn->detail->arm64.op_count].imm = MCOperand_getImm(Op);
		MI->flat_insn->detail->arm64.op_count++;
	}
}

static void printPostIncOperand(MCInst *MI, unsigned OpNo,
		unsigned Imm, SStream *O)
{
	MCOperand *Op = MCInst_getOperand(MI, OpNo);

	if (MCOperand_isReg(Op)) {
		unsigned Reg = MCOperand_getReg(Op);
		if (Reg == AArch64_XZR) {
			printInt32Bang(O, Imm);
			if (MI->csh->detail) {
#ifndef CAPSTONE_DIET
				uint8_t access;
				access = get_op_access(MI->csh, MCInst_getOpcode(MI), MI->ac_idx);
				MI->flat_insn->detail->arm64.operands[MI->flat_insn->detail->arm64.op_count].access = access;
				MI->ac_idx++;
#endif
				MI->flat_insn->detail->arm64.operands[MI->flat_insn->detail->arm64.op_count].type = ARM64_OP_IMM;
				MI->flat_insn->detail->arm64.operands[MI->flat_insn->detail->arm64.op_count].imm = Imm;
				MI->flat_insn->detail->arm64.op_count++;
			}
		} else {
			SStream_concat0(O, getRegisterName(Reg, AArch64_NoRegAltName));
			if (MI->csh->detail) {
#ifndef CAPSTONE_DIET
				uint8_t access;
				access = get_op_access(MI->csh, MCInst_getOpcode(MI), MI->ac_idx);
				MI->flat_insn->detail->arm64.operands[MI->flat_insn->detail->arm64.op_count].access = access;
				MI->ac_idx++;
#endif
				MI->flat_insn->detail->arm64.operands[MI->flat_insn->detail->arm64.op_count].type = ARM64_OP_REG;
				MI->flat_insn->detail->arm64.operands[MI->flat_insn->detail->arm64.op_count].reg = Reg;
				MI->flat_insn->detail->arm64.op_count++;
			}
		}
	}
	//llvm_unreachable("unknown operand kind in printPostIncOperand64");
}

static void printPostIncOperand2(MCInst *MI, unsigned OpNo, SStream *O, int Amount)
{
	printPostIncOperand(MI, OpNo, Amount, O);
}

static void printVRegOperand(MCInst *MI, unsigned OpNo, SStream *O)
{
	MCOperand *Op = MCInst_getOperand(MI, OpNo);
	//assert(Op.isReg() && "Non-register vreg operand!");
	unsigned Reg = MCOperand_getReg(Op);
	SStream_concat0(O, getRegisterName(Reg, AArch64_vreg));
	if (MI->csh->detail) {
#ifndef CAPSTONE_DIET
		uint8_t access;
		access = get_op_access(MI->csh, MCInst_getOpcode(MI), MI->ac_idx);
		MI->flat_insn->detail->arm64.operands[MI->flat_insn->detail->arm64.op_count].access = access;
		MI->ac_idx++;
#endif
		MI->flat_insn->detail->arm64.operands[MI->flat_insn->detail->arm64.op_count].type = ARM64_OP_REG;
		MI->flat_insn->detail->arm64.operands[MI->flat_insn->detail->arm64.op_count].reg = AArch64_map_vregister(Reg);
		MI->flat_insn->detail->arm64.op_count++;
	}
}

static void printSysCROperand(MCInst *MI, unsigned OpNo, SStream *O)
{
	MCOperand *Op = MCInst_getOperand(MI, OpNo);
	//assert(Op.isImm() && "System instruction C[nm] operands must be immediates!");
	SStream_concat(O, "c%u", MCOperand_getImm(Op));
	if (MI->csh->detail) {
#ifndef CAPSTONE_DIET
		uint8_t access;
		access = get_op_access(MI->csh, MCInst_getOpcode(MI), MI->ac_idx);
		MI->flat_insn->detail->arm64.operands[MI->flat_insn->detail->arm64.op_count].access = access;
		MI->ac_idx++;
#endif
		MI->flat_insn->detail->arm64.operands[MI->flat_insn->detail->arm64.op_count].type = ARM64_OP_CIMM;
		MI->flat_insn->detail->arm64.operands[MI->flat_insn->detail->arm64.op_count].imm = MCOperand_getImm(Op);
		MI->flat_insn->detail->arm64.op_count++;
	}
}

static void printAddSubImm(MCInst *MI, unsigned OpNum, SStream *O)
{
	MCOperand *MO = MCInst_getOperand(MI, OpNum);
	if (MCOperand_isImm(MO)) {
		unsigned Val = (MCOperand_getImm(MO) & 0xfff);
		//assert(Val == MO.getImm() && "Add/sub immediate out of range!");
		unsigned Shift = AArch64_AM_getShiftValue((int)MCOperand_getImm(MCInst_getOperand(MI, OpNum + 1)));

		printInt32Bang(O, Val);

		if (MI->csh->detail) {
#ifndef CAPSTONE_DIET
			uint8_t access;
			access = get_op_access(MI->csh, MCInst_getOpcode(MI), MI->ac_idx);
			MI->flat_insn->detail->arm64.operands[MI->flat_insn->detail->arm64.op_count].access = access;
			MI->ac_idx++;
#endif
			MI->flat_insn->detail->arm64.operands[MI->flat_insn->detail->arm64.op_count].type = ARM64_OP_IMM;
			MI->flat_insn->detail->arm64.operands[MI->flat_insn->detail->arm64.op_count].imm = Val;
			MI->flat_insn->detail->arm64.op_count++;
		}

		if (Shift != 0)
			printShifter(MI, OpNum + 1, O);
	}
}

static void printLogicalImm32(MCInst *MI, unsigned OpNum, SStream *O)
{
	int64_t Val = MCOperand_getImm(MCInst_getOperand(MI, OpNum));

	Val = AArch64_AM_decodeLogicalImmediate(Val, 32);
	printUInt32Bang(O, (int)Val);

	if (MI->csh->detail) {
#ifndef CAPSTONE_DIET
		uint8_t access;
		access = get_op_access(MI->csh, MCInst_getOpcode(MI), MI->ac_idx);
		MI->flat_insn->detail->arm64.operands[MI->flat_insn->detail->arm64.op_count].access = access;
		MI->ac_idx++;
#endif
		MI->flat_insn->detail->arm64.operands[MI->flat_insn->detail->arm64.op_count].type = ARM64_OP_IMM;
		MI->flat_insn->detail->arm64.operands[MI->flat_insn->detail->arm64.op_count].imm = Val;
		MI->flat_insn->detail->arm64.op_count++;
	}
}

static void printLogicalImm64(MCInst *MI, unsigned OpNum, SStream *O)
{
	int64_t Val = MCOperand_getImm(MCInst_getOperand(MI, OpNum));
	Val = AArch64_AM_decodeLogicalImmediate(Val, 64);

	switch(MI->flat_insn->id) {
		default:
			printInt64Bang(O, Val);
			break;
		case ARM64_INS_ORR:
		case ARM64_INS_AND:
		case ARM64_INS_EOR:
		case ARM64_INS_TST:
			// do not print number in negative form
			if (Val >= 0 && Val <= HEX_THRESHOLD)
				SStream_concat(O, "#%u", (int)Val);
			else
				SStream_concat(O, "#0x%"PRIx64, Val);
			break;
	}

	if (MI->csh->detail) {
#ifndef CAPSTONE_DIET
		uint8_t access;
		access = get_op_access(MI->csh, MCInst_getOpcode(MI), MI->ac_idx);
		MI->flat_insn->detail->arm64.operands[MI->flat_insn->detail->arm64.op_count].access = access;
		MI->ac_idx++;
#endif
		MI->flat_insn->detail->arm64.operands[MI->flat_insn->detail->arm64.op_count].type = ARM64_OP_IMM;
		MI->flat_insn->detail->arm64.operands[MI->flat_insn->detail->arm64.op_count].imm = (int64_t)Val;
		MI->flat_insn->detail->arm64.op_count++;
	}
}

static void printShifter(MCInst *MI, unsigned OpNum, SStream *O)
{
	unsigned Val = (unsigned)MCOperand_getImm(MCInst_getOperand(MI, OpNum));

	// LSL #0 should not be printed.
	if (AArch64_AM_getShiftType(Val) == AArch64_AM_LSL &&
			AArch64_AM_getShiftValue(Val) == 0)
		return;

	SStream_concat(O, ", %s ", AArch64_AM_getShiftExtendName(AArch64_AM_getShiftType(Val)));
	printInt32BangDec(O, AArch64_AM_getShiftValue(Val));
	if (MI->csh->detail) {
		arm64_shifter shifter = ARM64_SFT_INVALID;
		switch(AArch64_AM_getShiftType(Val)) {
			default:	// never reach
			case AArch64_AM_LSL:
				shifter = ARM64_SFT_LSL;
				break;
			case AArch64_AM_LSR:
				shifter = ARM64_SFT_LSR;
				break;
			case AArch64_AM_ASR:
				shifter = ARM64_SFT_ASR;
				break;
			case AArch64_AM_ROR:
				shifter = ARM64_SFT_ROR;
				break;
			case AArch64_AM_MSL:
				shifter = ARM64_SFT_MSL;
				break;
		}

		MI->flat_insn->detail->arm64.operands[MI->flat_insn->detail->arm64.op_count - 1].shift.type = shifter;
		MI->flat_insn->detail->arm64.operands[MI->flat_insn->detail->arm64.op_count - 1].shift.value = AArch64_AM_getShiftValue(Val);
	}
}

static void printShiftedRegister(MCInst *MI, unsigned OpNum, SStream *O)
{
	SStream_concat0(O, getRegisterName(MCOperand_getReg(MCInst_getOperand(MI, OpNum)), AArch64_NoRegAltName));
	if (MI->csh->detail) {
#ifndef CAPSTONE_DIET
		uint8_t access;
		access = get_op_access(MI->csh, MCInst_getOpcode(MI), MI->ac_idx);
		MI->flat_insn->detail->arm64.operands[MI->flat_insn->detail->arm64.op_count].access = access;
		MI->ac_idx++;
#endif
		MI->flat_insn->detail->arm64.operands[MI->flat_insn->detail->arm64.op_count].type = ARM64_OP_REG;
		MI->flat_insn->detail->arm64.operands[MI->flat_insn->detail->arm64.op_count].reg = MCOperand_getReg(MCInst_getOperand(MI, OpNum));
		MI->flat_insn->detail->arm64.op_count++;
	}
	printShifter(MI, OpNum + 1, O);
}

static void printArithExtend(MCInst *MI, unsigned OpNum, SStream *O)
{
	unsigned Val = (unsigned)MCOperand_getImm(MCInst_getOperand(MI, OpNum));
	AArch64_AM_ShiftExtendType ExtType = AArch64_AM_getArithExtendType(Val);
	unsigned ShiftVal = AArch64_AM_getArithShiftValue(Val);

	// If the destination or first source register operand is [W]SP, print
	// UXTW/UXTX as LSL, and if the shift amount is also zero, print nothing at
	// all.
	if (ExtType == AArch64_AM_UXTW || ExtType == AArch64_AM_UXTX) {
		unsigned Dest = MCOperand_getReg(MCInst_getOperand(MI, 0));
		unsigned Src1 = MCOperand_getReg(MCInst_getOperand(MI, 1));
		if ( ((Dest == AArch64_SP || Src1 == AArch64_SP) &&
					ExtType == AArch64_AM_UXTX) ||
				((Dest == AArch64_WSP || Src1 == AArch64_WSP) &&
				 ExtType == AArch64_AM_UXTW) ) {
			if (ShiftVal != 0) {
				SStream_concat0(O, ", lsl ");
				printInt32Bang(O, ShiftVal);
				if (MI->csh->detail) {
					MI->flat_insn->detail->arm64.operands[MI->flat_insn->detail->arm64.op_count - 1].shift.type = ARM64_SFT_LSL;
					MI->flat_insn->detail->arm64.operands[MI->flat_insn->detail->arm64.op_count - 1].shift.value = ShiftVal;
				}
			}

			return;
		}
	}

	SStream_concat(O, ", %s", AArch64_AM_getShiftExtendName(ExtType));
	if (MI->csh->detail) {
		arm64_extender ext = ARM64_EXT_INVALID;
		switch(ExtType) {
			default:	// never reach
			case AArch64_AM_UXTB:
				ext = ARM64_EXT_UXTB;
				break;
			case AArch64_AM_UXTH:
				ext = ARM64_EXT_UXTH;
				break;
			case AArch64_AM_UXTW:
				ext = ARM64_EXT_UXTW;
				break;
			case AArch64_AM_UXTX:
				ext = ARM64_EXT_UXTX;
				break;
			case AArch64_AM_SXTB:
				ext = ARM64_EXT_SXTB;
				break;
			case AArch64_AM_SXTH:
				ext = ARM64_EXT_SXTH;
				break;
			case AArch64_AM_SXTW:
				ext = ARM64_EXT_SXTW;
				break;
			case AArch64_AM_SXTX:
				ext = ARM64_EXT_SXTX;
				break;
		}

		MI->flat_insn->detail->arm64.operands[MI->flat_insn->detail->arm64.op_count - 1].ext = ext;
	}

	if (ShiftVal != 0) {
		SStream_concat0(O, " ");
		printInt32Bang(O, ShiftVal);
		if (MI->csh->detail) {
			MI->flat_insn->detail->arm64.operands[MI->flat_insn->detail->arm64.op_count - 1].shift.type = ARM64_SFT_LSL;
			MI->flat_insn->detail->arm64.operands[MI->flat_insn->detail->arm64.op_count - 1].shift.value = ShiftVal;
		}
	}
}

static void printExtendedRegister(MCInst *MI, unsigned OpNum, SStream *O)
{
	unsigned Reg = MCOperand_getReg(MCInst_getOperand(MI, OpNum));

	SStream_concat0(O, getRegisterName(Reg, AArch64_NoRegAltName));
	if (MI->csh->detail) {
#ifndef CAPSTONE_DIET
		uint8_t access;
		access = get_op_access(MI->csh, MCInst_getOpcode(MI), MI->ac_idx);
		MI->flat_insn->detail->arm64.operands[MI->flat_insn->detail->arm64.op_count].access = access;
		MI->ac_idx++;
#endif
		MI->flat_insn->detail->arm64.operands[MI->flat_insn->detail->arm64.op_count].type = ARM64_OP_REG;
		MI->flat_insn->detail->arm64.operands[MI->flat_insn->detail->arm64.op_count].reg = Reg;
		MI->flat_insn->detail->arm64.op_count++;
	}

	printArithExtend(MI, OpNum + 1, O);
}

static void printMemExtend(MCInst *MI, unsigned OpNum, SStream *O, char SrcRegKind, unsigned Width)
{
	unsigned SignExtend = (unsigned)MCOperand_getImm(MCInst_getOperand(MI, OpNum));
	unsigned DoShift = (unsigned)MCOperand_getImm(MCInst_getOperand(MI, OpNum + 1));

	// sxtw, sxtx, uxtw or lsl (== uxtx)
	bool IsLSL = !SignExtend && SrcRegKind == 'x';
	if (IsLSL) {
		SStream_concat0(O, "lsl");
		if (MI->csh->detail) {
			MI->flat_insn->detail->arm64.operands[MI->flat_insn->detail->arm64.op_count].shift.type = ARM64_SFT_LSL;
		}
	} else {
		SStream_concat(O, "%cxt%c", (SignExtend ? 's' : 'u'), SrcRegKind);
		if (MI->csh->detail) {
			if (!SignExtend) {
				switch(SrcRegKind) {
					default: break;
					case 'b':
							 MI->flat_insn->detail->arm64.operands[MI->flat_insn->detail->arm64.op_count].ext = ARM64_EXT_UXTB;
							 break;
					case 'h':
							 MI->flat_insn->detail->arm64.operands[MI->flat_insn->detail->arm64.op_count].ext = ARM64_EXT_UXTH;
							 break;
					case 'w':
							 MI->flat_insn->detail->arm64.operands[MI->flat_insn->detail->arm64.op_count].ext = ARM64_EXT_UXTW;
							 break;
				}
			} else {
					switch(SrcRegKind) {
						default: break;
						case 'b':
							MI->flat_insn->detail->arm64.operands[MI->flat_insn->detail->arm64.op_count].ext = ARM64_EXT_SXTB;
							break;
						case 'h':
							MI->flat_insn->detail->arm64.operands[MI->flat_insn->detail->arm64.op_count].ext = ARM64_EXT_SXTH;
							break;
						case 'w':
							MI->flat_insn->detail->arm64.operands[MI->flat_insn->detail->arm64.op_count].ext = ARM64_EXT_SXTW;
							break;
						case 'x':
							MI->flat_insn->detail->arm64.operands[MI->flat_insn->detail->arm64.op_count].ext = ARM64_EXT_SXTX;
							break;
					}
			}
		}
	}

	if (DoShift || IsLSL) {
		SStream_concat(O, " #%u", Log2_32(Width / 8));
		if (MI->csh->detail) {
			MI->flat_insn->detail->arm64.operands[MI->flat_insn->detail->arm64.op_count].shift.type = ARM64_SFT_LSL;
			MI->flat_insn->detail->arm64.operands[MI->flat_insn->detail->arm64.op_count].shift.value = Log2_32(Width / 8);
		}
	}
}

static void printCondCode(MCInst *MI, unsigned OpNum, SStream *O)
{
	A64CC_CondCode CC = (A64CC_CondCode)MCOperand_getImm(MCInst_getOperand(MI, OpNum));
	SStream_concat0(O, getCondCodeName(CC));

	if (MI->csh->detail)
		MI->flat_insn->detail->arm64.cc = (arm64_cc)(CC + 1);
}

static void printInverseCondCode(MCInst *MI, unsigned OpNum, SStream *O)
{
	A64CC_CondCode CC = (A64CC_CondCode)MCOperand_getImm(MCInst_getOperand(MI, OpNum));
	SStream_concat0(O, getCondCodeName(getInvertedCondCode(CC)));

	if (MI->csh->detail) {
		MI->flat_insn->detail->arm64.cc = (arm64_cc)(getInvertedCondCode(CC) + 1);
	}
}

static void printImmScale(MCInst *MI, unsigned OpNum, SStream *O, int Scale)
{
	int64_t val = Scale * MCOperand_getImm(MCInst_getOperand(MI, OpNum));

	printInt64Bang(O, val);

	if (MI->csh->detail) {
		if (MI->csh->doing_mem) {
			MI->flat_insn->detail->arm64.operands[MI->flat_insn->detail->arm64.op_count].mem.disp = (int32_t)val;
		} else {
#ifndef CAPSTONE_DIET
			uint8_t access;
			access = get_op_access(MI->csh, MCInst_getOpcode(MI), MI->ac_idx);
			MI->flat_insn->detail->arm64.operands[MI->flat_insn->detail->arm64.op_count].access = access;
			MI->ac_idx++;
#endif
			MI->flat_insn->detail->arm64.operands[MI->flat_insn->detail->arm64.op_count].type = ARM64_OP_IMM;
			MI->flat_insn->detail->arm64.operands[MI->flat_insn->detail->arm64.op_count].imm = val;
			MI->flat_insn->detail->arm64.op_count++;
		}
	}
}

static void printUImm12Offset(MCInst *MI, unsigned OpNum, unsigned Scale, SStream *O)
{
	MCOperand *MO = MCInst_getOperand(MI, OpNum);

	if (MCOperand_isImm(MO)) {
		int64_t val = Scale * MCOperand_getImm(MO);
		printInt64Bang(O, val);
		if (MI->csh->detail) {
			if (MI->csh->doing_mem) {
				MI->flat_insn->detail->arm64.operands[MI->flat_insn->detail->arm64.op_count].mem.disp = (int32_t)val;
			} else {
#ifndef CAPSTONE_DIET
				uint8_t access;
				access = get_op_access(MI->csh, MCInst_getOpcode(MI), MI->ac_idx);
				MI->flat_insn->detail->arm64.operands[MI->flat_insn->detail->arm64.op_count].access = access;
				MI->ac_idx++;
#endif
				MI->flat_insn->detail->arm64.operands[MI->flat_insn->detail->arm64.op_count].type = ARM64_OP_IMM;
				MI->flat_insn->detail->arm64.operands[MI->flat_insn->detail->arm64.op_count].imm = (int)val;
				MI->flat_insn->detail->arm64.op_count++;
			}
		}
	}
}

static void printUImm12Offset2(MCInst *MI, unsigned OpNum, SStream *O, int Scale)
{
	printUImm12Offset(MI, OpNum, Scale, O);
}

static void printPrefetchOp(MCInst *MI, unsigned OpNum, SStream *O)
{
	unsigned prfop = (unsigned)MCOperand_getImm(MCInst_getOperand(MI, OpNum));
	bool Valid;
	const char *Name = A64NamedImmMapper_toString(&A64PRFM_PRFMMapper, prfop, &Valid);

	if (Valid) {
		SStream_concat0(O, Name);
		if (MI->csh->detail) {
			MI->flat_insn->detail->arm64.operands[MI->flat_insn->detail->arm64.op_count].type = ARM64_OP_PREFETCH;
			// we have to plus 1 to prfop because 0 is a valid value of prfop
			MI->flat_insn->detail->arm64.operands[MI->flat_insn->detail->arm64.op_count].prefetch = prfop + 1;
			MI->flat_insn->detail->arm64.op_count++;
		}
	} else {
		printInt32Bang(O, prfop);
		if (MI->csh->detail) {
#ifndef CAPSTONE_DIET
			uint8_t access;
			access = get_op_access(MI->csh, MCInst_getOpcode(MI), MI->ac_idx);
			MI->flat_insn->detail->arm64.operands[MI->flat_insn->detail->arm64.op_count].access = access;
			MI->ac_idx++;
#endif
			MI->flat_insn->detail->arm64.operands[MI->flat_insn->detail->arm64.op_count].type = ARM64_OP_IMM;
			MI->flat_insn->detail->arm64.operands[MI->flat_insn->detail->arm64.op_count].imm = prfop;
			MI->flat_insn->detail->arm64.op_count++;
		}
	}
}

static void printFPImmOperand(MCInst *MI, unsigned OpNum, SStream *O)
{
	MCOperand *MO = MCInst_getOperand(MI, OpNum);
	double FPImm = MCOperand_isFPImm(MO) ? MCOperand_getFPImm(MO) : AArch64_AM_getFPImmFloat((int)MCOperand_getImm(MO));

	// 8 decimal places are enough to perfectly represent permitted floats.
#if defined(_KERNEL_MODE)
	// Issue #681: Windows kernel does not support formatting float point
	SStream_concat(O, "#<float_point_unsupported>");
#else
	SStream_concat(O, "#%.8f", FPImm);
#endif
	if (MI->csh->detail) {
#ifndef CAPSTONE_DIET
		uint8_t access;
		access = get_op_access(MI->csh, MCInst_getOpcode(MI), MI->ac_idx);
		MI->flat_insn->detail->arm64.operands[MI->flat_insn->detail->arm64.op_count].access = access;
		MI->ac_idx++;
#endif
		MI->flat_insn->detail->arm64.operands[MI->flat_insn->detail->arm64.op_count].type = ARM64_OP_FP;
		MI->flat_insn->detail->arm64.operands[MI->flat_insn->detail->arm64.op_count].fp = FPImm;
		MI->flat_insn->detail->arm64.op_count++;
	}
}

//static unsigned getNextVectorRegister(unsigned Reg, unsigned Stride = 1)
static unsigned getNextVectorRegister(unsigned Reg, unsigned Stride)
{
	while (Stride--) {
		switch (Reg) {
			default:
				// llvm_unreachable("Vector register expected!");
			case AArch64_Q0:  Reg = AArch64_Q1;  break;
			case AArch64_Q1:  Reg = AArch64_Q2;  break;
			case AArch64_Q2:  Reg = AArch64_Q3;  break;
			case AArch64_Q3:  Reg = AArch64_Q4;  break;
			case AArch64_Q4:  Reg = AArch64_Q5;  break;
			case AArch64_Q5:  Reg = AArch64_Q6;  break;
			case AArch64_Q6:  Reg = AArch64_Q7;  break;
			case AArch64_Q7:  Reg = AArch64_Q8;  break;
			case AArch64_Q8:  Reg = AArch64_Q9;  break;
			case AArch64_Q9:  Reg = AArch64_Q10; break;
			case AArch64_Q10: Reg = AArch64_Q11; break;
			case AArch64_Q11: Reg = AArch64_Q12; break;
			case AArch64_Q12: Reg = AArch64_Q13; break;
			case AArch64_Q13: Reg = AArch64_Q14; break;
			case AArch64_Q14: Reg = AArch64_Q15; break;
			case AArch64_Q15: Reg = AArch64_Q16; break;
			case AArch64_Q16: Reg = AArch64_Q17; break;
			case AArch64_Q17: Reg = AArch64_Q18; break;
			case AArch64_Q18: Reg = AArch64_Q19; break;
			case AArch64_Q19: Reg = AArch64_Q20; break;
			case AArch64_Q20: Reg = AArch64_Q21; break;
			case AArch64_Q21: Reg = AArch64_Q22; break;
			case AArch64_Q22: Reg = AArch64_Q23; break;
			case AArch64_Q23: Reg = AArch64_Q24; break;
			case AArch64_Q24: Reg = AArch64_Q25; break;
			case AArch64_Q25: Reg = AArch64_Q26; break;
			case AArch64_Q26: Reg = AArch64_Q27; break;
			case AArch64_Q27: Reg = AArch64_Q28; break;
			case AArch64_Q28: Reg = AArch64_Q29; break;
			case AArch64_Q29: Reg = AArch64_Q30; break;
			case AArch64_Q30: Reg = AArch64_Q31; break;
							   // Vector lists can wrap around.
			case AArch64_Q31: Reg = AArch64_Q0; break;
		}
	}

	return Reg;
}

static void printVectorList(MCInst *MI, unsigned OpNum, SStream *O, char *LayoutSuffix, MCRegisterInfo *MRI, arm64_vas vas, arm64_vess vess)
{
#define GETREGCLASS_CONTAIN0(_class, _reg) MCRegisterClass_contains(MCRegisterInfo_getRegClass(MRI, _class), _reg)

	unsigned Reg = MCOperand_getReg(MCInst_getOperand(MI, OpNum));
	unsigned NumRegs = 1, FirstReg, i;

	SStream_concat0(O, "{");

	// Work out how many registers there are in the list (if there is an actual
	// list).
	if (GETREGCLASS_CONTAIN0(AArch64_DDRegClassID , Reg) ||
			GETREGCLASS_CONTAIN0(AArch64_QQRegClassID, Reg))
		NumRegs = 2;
	else if (GETREGCLASS_CONTAIN0(AArch64_DDDRegClassID, Reg) ||
			GETREGCLASS_CONTAIN0(AArch64_QQQRegClassID, Reg))
		NumRegs = 3;
	else if (GETREGCLASS_CONTAIN0(AArch64_DDDDRegClassID, Reg) ||
			GETREGCLASS_CONTAIN0(AArch64_QQQQRegClassID, Reg))
		NumRegs = 4;

	// Now forget about the list and find out what the first register is.
	if ((FirstReg = MCRegisterInfo_getSubReg(MRI, Reg, AArch64_dsub0)))
		Reg = FirstReg;
	else if ((FirstReg = MCRegisterInfo_getSubReg(MRI, Reg, AArch64_qsub0)))
		Reg = FirstReg;

	// If it's a D-reg, we need to promote it to the equivalent Q-reg before
	// printing (otherwise getRegisterName fails).
	if (GETREGCLASS_CONTAIN0(AArch64_FPR64RegClassID, Reg)) {
		const MCRegisterClass *FPR128RC = MCRegisterInfo_getRegClass(MRI, AArch64_FPR128RegClassID);
		Reg = MCRegisterInfo_getMatchingSuperReg(MRI, Reg, AArch64_dsub, FPR128RC);
	}

	for (i = 0; i < NumRegs; ++i, Reg = getNextVectorRegister(Reg, 1)) {
		SStream_concat(O, "%s%s", getRegisterName(Reg, AArch64_vreg), LayoutSuffix);
		if (i + 1 != NumRegs)
			SStream_concat0(O, ", ");
		if (MI->csh->detail) {
#ifndef CAPSTONE_DIET
			uint8_t access;
			access = get_op_access(MI->csh, MCInst_getOpcode(MI), MI->ac_idx);
			MI->flat_insn->detail->arm64.operands[MI->flat_insn->detail->arm64.op_count].access = access;
			MI->ac_idx++;
#endif
			MI->flat_insn->detail->arm64.operands[MI->flat_insn->detail->arm64.op_count].type = ARM64_OP_REG;
			MI->flat_insn->detail->arm64.operands[MI->flat_insn->detail->arm64.op_count].reg = AArch64_map_vregister(Reg);
			MI->flat_insn->detail->arm64.operands[MI->flat_insn->detail->arm64.op_count].vas = vas;
			MI->flat_insn->detail->arm64.operands[MI->flat_insn->detail->arm64.op_count].vess = vess;
			MI->flat_insn->detail->arm64.op_count++;
		}
	}

	SStream_concat0(O, "}");
}

static void printTypedVectorList(MCInst *MI, unsigned OpNum, SStream *O, unsigned NumLanes, char LaneKind, MCRegisterInfo *MRI)
{
	char Suffix[32];
	arm64_vas vas = 0;
	arm64_vess vess = 0;

	if (NumLanes) {
		cs_snprintf(Suffix, sizeof(Suffix), ".%u%c", NumLanes, LaneKind);
		switch(LaneKind) {
			default: break;
			case 'b':
				switch(NumLanes) {
					default: break;
					case 8:
							 vas = ARM64_VAS_8B;
							 break;
					case 16:
							 vas = ARM64_VAS_16B;
							 break;
				}
				break;
			case 'h':
				switch(NumLanes) {
					default: break;
					case 4:
							 vas = ARM64_VAS_4H;
							 break;
					case 8:
							 vas = ARM64_VAS_8H;
							 break;
				}
				break;
			case 's':
				switch(NumLanes) {
					default: break;
					case 2:
							 vas = ARM64_VAS_2S;
							 break;
					case 4:
							 vas = ARM64_VAS_4S;
							 break;
				}
				break;
			case 'd':
				switch(NumLanes) {
					default: break;
					case 1:
							 vas = ARM64_VAS_1D;
							 break;
					case 2:
							 vas = ARM64_VAS_2D;
							 break;
				}
				break;
			case 'q':
				switch(NumLanes) {
					default: break;
					case 1:
							 vas = ARM64_VAS_1Q;
							 break;
				}
				break;
		}
	} else {
		cs_snprintf(Suffix, sizeof(Suffix), ".%c", LaneKind);
		switch(LaneKind) {
			default: break;
			case 'b':
					 vess = ARM64_VESS_B;
					 break;
			case 'h':
					 vess = ARM64_VESS_H;
					 break;
			case 's':
					 vess = ARM64_VESS_S;
					 break;
			case 'd':
					 vess = ARM64_VESS_D;
					 break;
		}
	}

	printVectorList(MI, OpNum, O, Suffix, MRI, vas, vess);
}

static void printVectorIndex(MCInst *MI, unsigned OpNum, SStream *O)
{
	SStream_concat0(O, "[");
	printInt32(O, (int)MCOperand_getImm(MCInst_getOperand(MI, OpNum)));
	SStream_concat0(O, "]");
	if (MI->csh->detail) {
		MI->flat_insn->detail->arm64.operands[MI->flat_insn->detail->arm64.op_count - 1].vector_index = (int)MCOperand_getImm(MCInst_getOperand(MI, OpNum));
	}
}

static void printAlignedLabel(MCInst *MI, unsigned OpNum, SStream *O)
{
	MCOperand *Op = MCInst_getOperand(MI, OpNum);

	// If the label has already been resolved to an immediate offset (say, when
	// we're running the disassembler), just print the immediate.
	if (MCOperand_isImm(Op)) {
		uint64_t imm = (MCOperand_getImm(Op) * 4) + MI->address;
		printUInt64Bang(O, imm);
		if (MI->csh->detail) {
#ifndef CAPSTONE_DIET
			uint8_t access;
			access = get_op_access(MI->csh, MCInst_getOpcode(MI), MI->ac_idx);
			MI->flat_insn->detail->arm64.operands[MI->flat_insn->detail->arm64.op_count].access = access;
			MI->ac_idx++;
#endif
			MI->flat_insn->detail->arm64.operands[MI->flat_insn->detail->arm64.op_count].type = ARM64_OP_IMM;
			MI->flat_insn->detail->arm64.operands[MI->flat_insn->detail->arm64.op_count].imm = imm;
			MI->flat_insn->detail->arm64.op_count++;
		}
		return;
	}
}

static void printAdrpLabel(MCInst *MI, unsigned OpNum, SStream *O)
{
	MCOperand *Op = MCInst_getOperand(MI, OpNum);

	if (MCOperand_isImm(Op)) {
		// ADRP sign extends a 21-bit offset, shifts it left by 12
		// and adds it to the value of the PC with its bottom 12 bits cleared
		uint64_t imm = (MCOperand_getImm(Op) * 0x1000) + (MI->address & ~0xfff);
		printUInt64Bang(O, imm);

		if (MI->csh->detail) {
#ifndef CAPSTONE_DIET
			uint8_t access;
			access = get_op_access(MI->csh, MCInst_getOpcode(MI), MI->ac_idx);
			MI->flat_insn->detail->arm64.operands[MI->flat_insn->detail->arm64.op_count].access = access;
			MI->ac_idx++;
#endif
			MI->flat_insn->detail->arm64.operands[MI->flat_insn->detail->arm64.op_count].type = ARM64_OP_IMM;
			MI->flat_insn->detail->arm64.operands[MI->flat_insn->detail->arm64.op_count].imm = imm;
			MI->flat_insn->detail->arm64.op_count++;
		}
		return;
	}
}

static void printBarrierOption(MCInst *MI, unsigned OpNo, SStream *O)
{
	unsigned Val = (unsigned)MCOperand_getImm(MCInst_getOperand(MI, OpNo));
	unsigned Opcode = MCInst_getOpcode(MI);
	bool Valid;
	const char *Name;

	if (Opcode == AArch64_ISB)
		Name = A64NamedImmMapper_toString(&A64ISB_ISBMapper, Val, &Valid);
	else
		Name = A64NamedImmMapper_toString(&A64DB_DBarrierMapper, Val, &Valid);

	if (Valid) {
		SStream_concat0(O, Name);
		if (MI->csh->detail) {
#ifndef CAPSTONE_DIET
			uint8_t access;
			access = get_op_access(MI->csh, MCInst_getOpcode(MI), MI->ac_idx);
			MI->flat_insn->detail->arm64.operands[MI->flat_insn->detail->arm64.op_count].access = access;
			MI->ac_idx++;
#endif
			MI->flat_insn->detail->arm64.operands[MI->flat_insn->detail->arm64.op_count].type = ARM64_OP_BARRIER;
			MI->flat_insn->detail->arm64.operands[MI->flat_insn->detail->arm64.op_count].barrier = Val;
			MI->flat_insn->detail->arm64.op_count++;
		}
	} else {
		printUInt32Bang(O, Val);
		if (MI->csh->detail) {
#ifndef CAPSTONE_DIET
			uint8_t access;
			access = get_op_access(MI->csh, MCInst_getOpcode(MI), MI->ac_idx);
			MI->flat_insn->detail->arm64.operands[MI->flat_insn->detail->arm64.op_count].access = access;
			MI->ac_idx++;
#endif
			MI->flat_insn->detail->arm64.operands[MI->flat_insn->detail->arm64.op_count].type = ARM64_OP_IMM;
			MI->flat_insn->detail->arm64.operands[MI->flat_insn->detail->arm64.op_count].imm = Val;
			MI->flat_insn->detail->arm64.op_count++;
		}
	}
}

static void printMRSSystemRegister(MCInst *MI, unsigned OpNo, SStream *O)
{
	unsigned Val = (unsigned)MCOperand_getImm(MCInst_getOperand(MI, OpNo));
	char Name[128];

	A64SysRegMapper_toString(&AArch64_MRSMapper, Val, Name);

	SStream_concat0(O, Name);
	if (MI->csh->detail) {
#ifndef CAPSTONE_DIET
		uint8_t access;
		access = get_op_access(MI->csh, MCInst_getOpcode(MI), MI->ac_idx);
		MI->flat_insn->detail->arm64.operands[MI->flat_insn->detail->arm64.op_count].access = access;
		MI->ac_idx++;
#endif
		MI->flat_insn->detail->arm64.operands[MI->flat_insn->detail->arm64.op_count].type = ARM64_OP_REG_MRS;
		MI->flat_insn->detail->arm64.operands[MI->flat_insn->detail->arm64.op_count].reg = Val;
		MI->flat_insn->detail->arm64.op_count++;
	}
}

static void printMSRSystemRegister(MCInst *MI, unsigned OpNo, SStream *O)
{
	unsigned Val = (unsigned)MCOperand_getImm(MCInst_getOperand(MI, OpNo));
	char Name[128];

	A64SysRegMapper_toString(&AArch64_MSRMapper, Val, Name);

	SStream_concat0(O, Name);
	if (MI->csh->detail) {
#ifndef CAPSTONE_DIET
		uint8_t access;
		access = get_op_access(MI->csh, MCInst_getOpcode(MI), MI->ac_idx);
		MI->flat_insn->detail->arm64.operands[MI->flat_insn->detail->arm64.op_count].access = access;
		MI->ac_idx++;
#endif
		MI->flat_insn->detail->arm64.operands[MI->flat_insn->detail->arm64.op_count].type = ARM64_OP_REG_MSR;
		MI->flat_insn->detail->arm64.operands[MI->flat_insn->detail->arm64.op_count].reg = Val;
		MI->flat_insn->detail->arm64.op_count++;
	}
}

static void printSystemPStateField(MCInst *MI, unsigned OpNo, SStream *O)
{
	unsigned Val = (unsigned)MCOperand_getImm(MCInst_getOperand(MI, OpNo));
	bool Valid;
	const char *Name;

	Name = A64NamedImmMapper_toString(&A64PState_PStateMapper, Val, &Valid);
	if (Valid) {
		SStream_concat0(O, Name);
		if (MI->csh->detail) {
#ifndef CAPSTONE_DIET
			uint8_t access;
			access = get_op_access(MI->csh, MCInst_getOpcode(MI), MI->ac_idx);
			MI->flat_insn->detail->arm64.operands[MI->flat_insn->detail->arm64.op_count].access = access;
			MI->ac_idx++;
#endif
			MI->flat_insn->detail->arm64.operands[MI->flat_insn->detail->arm64.op_count].type = ARM64_OP_PSTATE;
			MI->flat_insn->detail->arm64.operands[MI->flat_insn->detail->arm64.op_count].pstate = Val;
			MI->flat_insn->detail->arm64.op_count++;
		}
	} else {
#ifndef CAPSTONE_DIET
		unsigned char access;
#endif
		printInt32Bang(O, Val);
#ifndef CAPSTONE_DIET
		access = get_op_access(MI->csh, MCInst_getOpcode(MI), MI->ac_idx);
		MI->flat_insn->detail->arm64.operands[MI->flat_insn->detail->arm64.op_count].access = access;
		MI->ac_idx++;
#endif
		MI->flat_insn->detail->arm64.operands[MI->flat_insn->detail->arm64.op_count].type = ARM64_OP_IMM;
		MI->flat_insn->detail->arm64.operands[MI->flat_insn->detail->arm64.op_count].imm = Val;
		MI->flat_insn->detail->arm64.op_count++;
	}
}

static void printSIMDType10Operand(MCInst *MI, unsigned OpNo, SStream *O)
{
	uint8_t RawVal = (uint8_t)MCOperand_getImm(MCInst_getOperand(MI, OpNo));
	uint64_t Val = AArch64_AM_decodeAdvSIMDModImmType10(RawVal);
	SStream_concat(O, "#%#016llx", Val);
	if (MI->csh->detail) {
#ifndef CAPSTONE_DIET
		unsigned char access;
		access = get_op_access(MI->csh, MCInst_getOpcode(MI), MI->ac_idx);
		MI->flat_insn->detail->arm64.operands[MI->flat_insn->detail->arm64.op_count].access = access;
		MI->ac_idx++;
#endif
		MI->flat_insn->detail->arm64.operands[MI->flat_insn->detail->arm64.op_count].type = ARM64_OP_IMM;
		MI->flat_insn->detail->arm64.operands[MI->flat_insn->detail->arm64.op_count].imm = Val;
		MI->flat_insn->detail->arm64.op_count++;
	}
}


#define PRINT_ALIAS_INSTR
#include "AArch64GenAsmWriter.inc"

void AArch64_post_printer(csh handle, cs_insn *flat_insn, char *insn_asm, MCInst *mci)
{
	if (((cs_struct *)handle)->detail != CS_OPT_ON)
		return;

	if (mci->csh->detail) {
		unsigned opcode = MCInst_getOpcode(mci);
		switch (opcode) {
			default:
				break;
			case AArch64_LD1Fourv16b_POST:
			case AArch64_LD1Fourv1d_POST:
			case AArch64_LD1Fourv2d_POST:
			case AArch64_LD1Fourv2s_POST:
			case AArch64_LD1Fourv4h_POST:
			case AArch64_LD1Fourv4s_POST:
			case AArch64_LD1Fourv8b_POST:
			case AArch64_LD1Fourv8h_POST:
			case AArch64_LD1Onev16b_POST:
			case AArch64_LD1Onev1d_POST:
			case AArch64_LD1Onev2d_POST:
			case AArch64_LD1Onev2s_POST:
			case AArch64_LD1Onev4h_POST:
			case AArch64_LD1Onev4s_POST:
			case AArch64_LD1Onev8b_POST:
			case AArch64_LD1Onev8h_POST:
			case AArch64_LD1Rv16b_POST:
			case AArch64_LD1Rv1d_POST:
			case AArch64_LD1Rv2d_POST:
			case AArch64_LD1Rv2s_POST:
			case AArch64_LD1Rv4h_POST:
			case AArch64_LD1Rv4s_POST:
			case AArch64_LD1Rv8b_POST:
			case AArch64_LD1Rv8h_POST:
			case AArch64_LD1Threev16b_POST:
			case AArch64_LD1Threev1d_POST:
			case AArch64_LD1Threev2d_POST:
			case AArch64_LD1Threev2s_POST:
			case AArch64_LD1Threev4h_POST:
			case AArch64_LD1Threev4s_POST:
			case AArch64_LD1Threev8b_POST:
			case AArch64_LD1Threev8h_POST:
			case AArch64_LD1Twov16b_POST:
			case AArch64_LD1Twov1d_POST:
			case AArch64_LD1Twov2d_POST:
			case AArch64_LD1Twov2s_POST:
			case AArch64_LD1Twov4h_POST:
			case AArch64_LD1Twov4s_POST:
			case AArch64_LD1Twov8b_POST:
			case AArch64_LD1Twov8h_POST:
			case AArch64_LD1i16_POST:
			case AArch64_LD1i32_POST:
			case AArch64_LD1i64_POST:
			case AArch64_LD1i8_POST:
			case AArch64_LD2Rv16b_POST:
			case AArch64_LD2Rv1d_POST:
			case AArch64_LD2Rv2d_POST:
			case AArch64_LD2Rv2s_POST:
			case AArch64_LD2Rv4h_POST:
			case AArch64_LD2Rv4s_POST:
			case AArch64_LD2Rv8b_POST:
			case AArch64_LD2Rv8h_POST:
			case AArch64_LD2Twov16b_POST:
			case AArch64_LD2Twov2d_POST:
			case AArch64_LD2Twov2s_POST:
			case AArch64_LD2Twov4h_POST:
			case AArch64_LD2Twov4s_POST:
			case AArch64_LD2Twov8b_POST:
			case AArch64_LD2Twov8h_POST:
			case AArch64_LD2i16_POST:
			case AArch64_LD2i32_POST:
			case AArch64_LD2i64_POST:
			case AArch64_LD2i8_POST:
			case AArch64_LD3Rv16b_POST:
			case AArch64_LD3Rv1d_POST:
			case AArch64_LD3Rv2d_POST:
			case AArch64_LD3Rv2s_POST:
			case AArch64_LD3Rv4h_POST:
			case AArch64_LD3Rv4s_POST:
			case AArch64_LD3Rv8b_POST:
			case AArch64_LD3Rv8h_POST:
			case AArch64_LD3Threev16b_POST:
			case AArch64_LD3Threev2d_POST:
			case AArch64_LD3Threev2s_POST:
			case AArch64_LD3Threev4h_POST:
			case AArch64_LD3Threev4s_POST:
			case AArch64_LD3Threev8b_POST:
			case AArch64_LD3Threev8h_POST:
			case AArch64_LD3i16_POST:
			case AArch64_LD3i32_POST:
			case AArch64_LD3i64_POST:
			case AArch64_LD3i8_POST:
			case AArch64_LD4Fourv16b_POST:
			case AArch64_LD4Fourv2d_POST:
			case AArch64_LD4Fourv2s_POST:
			case AArch64_LD4Fourv4h_POST:
			case AArch64_LD4Fourv4s_POST:
			case AArch64_LD4Fourv8b_POST:
			case AArch64_LD4Fourv8h_POST:
			case AArch64_LD4Rv16b_POST:
			case AArch64_LD4Rv1d_POST:
			case AArch64_LD4Rv2d_POST:
			case AArch64_LD4Rv2s_POST:
			case AArch64_LD4Rv4h_POST:
			case AArch64_LD4Rv4s_POST:
			case AArch64_LD4Rv8b_POST:
			case AArch64_LD4Rv8h_POST:
			case AArch64_LD4i16_POST:
			case AArch64_LD4i32_POST:
			case AArch64_LD4i64_POST:
			case AArch64_LD4i8_POST:
			case AArch64_LDPDpost:
			case AArch64_LDPDpre:
			case AArch64_LDPQpost:
			case AArch64_LDPQpre:
			case AArch64_LDPSWpost:
			case AArch64_LDPSWpre:
			case AArch64_LDPSpost:
			case AArch64_LDPSpre:
			case AArch64_LDPWpost:
			case AArch64_LDPWpre:
			case AArch64_LDPXpost:
			case AArch64_LDPXpre:
			case AArch64_LDRBBpost:
			case AArch64_LDRBBpre:
			case AArch64_LDRBpost:
			case AArch64_LDRBpre:
			case AArch64_LDRDpost:
			case AArch64_LDRDpre:
			case AArch64_LDRHHpost:
			case AArch64_LDRHHpre:
			case AArch64_LDRHpost:
			case AArch64_LDRHpre:
			case AArch64_LDRQpost:
			case AArch64_LDRQpre:
			case AArch64_LDRSBWpost:
			case AArch64_LDRSBWpre:
			case AArch64_LDRSBXpost:
			case AArch64_LDRSBXpre:
			case AArch64_LDRSHWpost:
			case AArch64_LDRSHWpre:
			case AArch64_LDRSHXpost:
			case AArch64_LDRSHXpre:
			case AArch64_LDRSWpost:
			case AArch64_LDRSWpre:
			case AArch64_LDRSpost:
			case AArch64_LDRSpre:
			case AArch64_LDRWpost:
			case AArch64_LDRWpre:
			case AArch64_LDRXpost:
			case AArch64_LDRXpre:
			case AArch64_ST1Fourv16b_POST:
			case AArch64_ST1Fourv1d_POST:
			case AArch64_ST1Fourv2d_POST:
			case AArch64_ST1Fourv2s_POST:
			case AArch64_ST1Fourv4h_POST:
			case AArch64_ST1Fourv4s_POST:
			case AArch64_ST1Fourv8b_POST:
			case AArch64_ST1Fourv8h_POST:
			case AArch64_ST1Onev16b_POST:
			case AArch64_ST1Onev1d_POST:
			case AArch64_ST1Onev2d_POST:
			case AArch64_ST1Onev2s_POST:
			case AArch64_ST1Onev4h_POST:
			case AArch64_ST1Onev4s_POST:
			case AArch64_ST1Onev8b_POST:
			case AArch64_ST1Onev8h_POST:
			case AArch64_ST1Threev16b_POST:
			case AArch64_ST1Threev1d_POST:
			case AArch64_ST1Threev2d_POST:
			case AArch64_ST1Threev2s_POST:
			case AArch64_ST1Threev4h_POST:
			case AArch64_ST1Threev4s_POST:
			case AArch64_ST1Threev8b_POST:
			case AArch64_ST1Threev8h_POST:
			case AArch64_ST1Twov16b_POST:
			case AArch64_ST1Twov1d_POST:
			case AArch64_ST1Twov2d_POST:
			case AArch64_ST1Twov2s_POST:
			case AArch64_ST1Twov4h_POST:
			case AArch64_ST1Twov4s_POST:
			case AArch64_ST1Twov8b_POST:
			case AArch64_ST1Twov8h_POST:
			case AArch64_ST1i16_POST:
			case AArch64_ST1i32_POST:
			case AArch64_ST1i64_POST:
			case AArch64_ST1i8_POST:
			case AArch64_ST2Twov16b_POST:
			case AArch64_ST2Twov2d_POST:
			case AArch64_ST2Twov2s_POST:
			case AArch64_ST2Twov4h_POST:
			case AArch64_ST2Twov4s_POST:
			case AArch64_ST2Twov8b_POST:
			case AArch64_ST2Twov8h_POST:
			case AArch64_ST2i16_POST:
			case AArch64_ST2i32_POST:
			case AArch64_ST2i64_POST:
			case AArch64_ST2i8_POST:
			case AArch64_ST3Threev16b_POST:
			case AArch64_ST3Threev2d_POST:
			case AArch64_ST3Threev2s_POST:
			case AArch64_ST3Threev4h_POST:
			case AArch64_ST3Threev4s_POST:
			case AArch64_ST3Threev8b_POST:
			case AArch64_ST3Threev8h_POST:
			case AArch64_ST3i16_POST:
			case AArch64_ST3i32_POST:
			case AArch64_ST3i64_POST:
			case AArch64_ST3i8_POST:
			case AArch64_ST4Fourv16b_POST:
			case AArch64_ST4Fourv2d_POST:
			case AArch64_ST4Fourv2s_POST:
			case AArch64_ST4Fourv4h_POST:
			case AArch64_ST4Fourv4s_POST:
			case AArch64_ST4Fourv8b_POST:
			case AArch64_ST4Fourv8h_POST:
			case AArch64_ST4i16_POST:
			case AArch64_ST4i32_POST:
			case AArch64_ST4i64_POST:
			case AArch64_ST4i8_POST:
			case AArch64_STPDpost:
			case AArch64_STPDpre:
			case AArch64_STPQpost:
			case AArch64_STPQpre:
			case AArch64_STPSpost:
			case AArch64_STPSpre:
			case AArch64_STPWpost:
			case AArch64_STPWpre:
			case AArch64_STPXpost:
			case AArch64_STPXpre:
			case AArch64_STRBBpost:
			case AArch64_STRBBpre:
			case AArch64_STRBpost:
			case AArch64_STRBpre:
			case AArch64_STRDpost:
			case AArch64_STRDpre:
			case AArch64_STRHHpost:
			case AArch64_STRHHpre:
			case AArch64_STRHpost:
			case AArch64_STRHpre:
			case AArch64_STRQpost:
			case AArch64_STRQpre:
			case AArch64_STRSpost:
			case AArch64_STRSpre:
			case AArch64_STRWpost:
			case AArch64_STRWpre:
			case AArch64_STRXpost:
			case AArch64_STRXpre:
				flat_insn->detail->arm64.writeback = true;
				break;
		}
	}
}

#endif
