//===-- XCoreInstPrinter.cpp - Convert XCore MCInst to assembly syntax --------===//
//
//                     The LLVM Compiler Infrastructure
//
// This file is distributed under the University of Illinois Open Source
// License. See LICENSE.TXT for details.
//
//===----------------------------------------------------------------------===//
//
// This class prints an XCore MCInst to a .s file.
//
//===----------------------------------------------------------------------===//

/* Capstone Disassembly Engine */
/* By Nguyen Anh Quynh <aquynh@gmail.com>, 2013-2015 */

#ifdef CAPSTONE_HAS_XCORE

#if defined (WIN32) || defined (WIN64) || defined (_WIN32) || defined (_WIN64)
#pragma warning(disable : 4996)			// disable MSVC's warning on strcpy()
#pragma warning(disable : 28719)		// disable MSVC's warning on strcpy()
#endif

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <capstone/platform.h>

#include "XCoreInstPrinter.h"
#include "../../MCInst.h"
#include "../../utils.h"
#include "../../SStream.h"
#include "../../MCRegisterInfo.h"
#include "../../MathExtras.h"
#include "XCoreMapping.h"

static const char *getRegisterName(unsigned RegNo);

void XCore_post_printer(csh ud, cs_insn *insn, char *insn_asm, MCInst *mci)
{
	/*
	   if (((cs_struct *)ud)->detail != CS_OPT_ON)
	   return;
	 */
}

// stw sed, sp[3]
void XCore_insn_extract(MCInst *MI, const char *code)
{
	int id;
	char *p, *p2;
	char tmp[128];

	strcpy(tmp, code); // safe because code is way shorter than 128 bytes

	// find the first space
	p = strchr(tmp, ' ');
	if (p) {
		p++;
		// find the next ','
		p2 = strchr(p, ',');
		if (p2) {
			*p2 = '\0';
			id = XCore_reg_id(p);
			if (id) {
				// register
				if (MI->csh->detail) {
					MI->flat_insn->detail->xcore.operands[MI->flat_insn->detail->xcore.op_count].type = XCORE_OP_REG;
					MI->flat_insn->detail->xcore.operands[MI->flat_insn->detail->xcore.op_count].reg = id;
					MI->flat_insn->detail->xcore.op_count++;
				}
			}
			// next should be register, or memory?
			// skip space
			p2++;
			while(*p2 && *p2 == ' ')
				p2++;
			if (*p2) {
				// find '['
				p = p2;
				while(*p && *p != '[')
					p++;
				if (*p) {
					// this is '['
					*p = '\0';
					id = XCore_reg_id(p2);
					if (id) {
						// base register
						if (MI->csh->detail) {
							MI->flat_insn->detail->xcore.operands[MI->flat_insn->detail->xcore.op_count].type = XCORE_OP_MEM;
							MI->flat_insn->detail->xcore.operands[MI->flat_insn->detail->xcore.op_count].mem.base = (uint8_t)id;
							MI->flat_insn->detail->xcore.operands[MI->flat_insn->detail->xcore.op_count].mem.index = XCORE_REG_INVALID;
							MI->flat_insn->detail->xcore.operands[MI->flat_insn->detail->xcore.op_count].mem.disp = 0;
							MI->flat_insn->detail->xcore.operands[MI->flat_insn->detail->xcore.op_count].mem.direct = 1;
						}

						p++;
						p2 = p;
						// until ']'
						while(*p && *p != ']')
							p++;
						if (*p) {
							*p = '\0';
							// p2 is either index, or disp
							id = XCore_reg_id(p2);
							if (id) {
								// index register
								if (MI->csh->detail) {
									MI->flat_insn->detail->xcore.operands[MI->flat_insn->detail->xcore.op_count].mem.index = (uint8_t)id;
								}
							} else {
								// a number means disp
								if (MI->csh->detail) {
									MI->flat_insn->detail->xcore.operands[MI->flat_insn->detail->xcore.op_count].mem.disp = atoi(p2);
								}
							}
						}

						if (MI->csh->detail) {
							MI->flat_insn->detail->xcore.op_count++;
						}
					}
				} else {
					// a register?
					id = XCore_reg_id(p2);
					if (id) {
						// register
						if (MI->csh->detail) {
							MI->flat_insn->detail->xcore.operands[MI->flat_insn->detail->xcore.op_count].type = XCORE_OP_REG;
							MI->flat_insn->detail->xcore.operands[MI->flat_insn->detail->xcore.op_count].reg = id;
							MI->flat_insn->detail->xcore.op_count++;
						}
					}
				}
			}
		} else {
			id = XCore_reg_id(p);
			if (id) {
				// register
				if (MI->csh->detail) {
					MI->flat_insn->detail->xcore.operands[MI->flat_insn->detail->xcore.op_count].type = XCORE_OP_REG;
					MI->flat_insn->detail->xcore.operands[MI->flat_insn->detail->xcore.op_count].reg = id;
					MI->flat_insn->detail->xcore.op_count++;
				}
			}
		}
	}
}

static void set_mem_access(MCInst *MI, bool status, int reg)
{
	if (MI->csh->detail != CS_OPT_ON)
		return;

	MI->csh->doing_mem = status;
	if (status) {
		if (reg != 0xffff && reg != -0xffff) {
			MI->flat_insn->detail->xcore.operands[MI->flat_insn->detail->xcore.op_count].type = XCORE_OP_MEM;
			if (reg) {
				MI->flat_insn->detail->xcore.operands[MI->flat_insn->detail->xcore.op_count].mem.base = (uint8_t)reg;
			} else {
				MI->flat_insn->detail->xcore.operands[MI->flat_insn->detail->xcore.op_count].mem.base = XCORE_REG_INVALID;
			}
			MI->flat_insn->detail->xcore.operands[MI->flat_insn->detail->xcore.op_count].mem.index = XCORE_REG_INVALID;
			MI->flat_insn->detail->xcore.operands[MI->flat_insn->detail->xcore.op_count].mem.disp = 0;
			MI->flat_insn->detail->xcore.operands[MI->flat_insn->detail->xcore.op_count].mem.direct = 1;
		} else {
			// the last op should be the memory base
			MI->flat_insn->detail->xcore.op_count--;
			MI->flat_insn->detail->xcore.operands[MI->flat_insn->detail->xcore.op_count].type = XCORE_OP_MEM;
			MI->flat_insn->detail->xcore.operands[MI->flat_insn->detail->xcore.op_count].mem.base = (uint8_t)MI->flat_insn->detail->xcore.operands[MI->flat_insn->detail->xcore.op_count].reg;
			MI->flat_insn->detail->xcore.operands[MI->flat_insn->detail->xcore.op_count].mem.index = XCORE_REG_INVALID;
			MI->flat_insn->detail->xcore.operands[MI->flat_insn->detail->xcore.op_count].mem.disp = 0;
			if (reg > 0)
				MI->flat_insn->detail->xcore.operands[MI->flat_insn->detail->xcore.op_count].mem.direct = 1;
			else
				MI->flat_insn->detail->xcore.operands[MI->flat_insn->detail->xcore.op_count].mem.direct = -1;
		}
	} else {
		if (reg) {
			MI->flat_insn->detail->xcore.operands[MI->flat_insn->detail->xcore.op_count].mem.index = (uint8_t)reg;
			// done, create the next operand slot
			MI->flat_insn->detail->xcore.op_count++;
		}
	}
}

static void _printOperand(MCInst *MI, MCOperand *MO, SStream *O)
{
	if (MCOperand_isReg(MO)) {
		unsigned reg;

		reg = MCOperand_getReg(MO);
		SStream_concat0(O, getRegisterName(reg));

		if (MI->csh->detail) {
			if (MI->csh->doing_mem) {
				if (MI->flat_insn->detail->xcore.operands[MI->flat_insn->detail->xcore.op_count].mem.base == ARM_REG_INVALID)
					MI->flat_insn->detail->xcore.operands[MI->flat_insn->detail->xcore.op_count].mem.base = (uint8_t)reg;
				else
					MI->flat_insn->detail->xcore.operands[MI->flat_insn->detail->xcore.op_count].mem.index = (uint8_t)reg;
			} else {
				MI->flat_insn->detail->xcore.operands[MI->flat_insn->detail->xcore.op_count].type = XCORE_OP_REG;
				MI->flat_insn->detail->xcore.operands[MI->flat_insn->detail->xcore.op_count].reg = reg;
				MI->flat_insn->detail->xcore.op_count++;
			}
		}
	} else if (MCOperand_isImm(MO)) {
		int32_t Imm = (int32_t)MCOperand_getImm(MO);

		printInt32(O, Imm);

		if (MI->csh->detail) {
			if (MI->csh->doing_mem) {
				MI->flat_insn->detail->xcore.operands[MI->flat_insn->detail->xcore.op_count].mem.disp = Imm;
			} else {
				MI->flat_insn->detail->xcore.operands[MI->flat_insn->detail->xcore.op_count].type = XCORE_OP_IMM;
				MI->flat_insn->detail->xcore.operands[MI->flat_insn->detail->xcore.op_count].imm = Imm;
				MI->flat_insn->detail->xcore.op_count++;
			}
		}
	}
}

static void printOperand(MCInst *MI, int OpNum, SStream *O)
{
	if (OpNum >= MI->size)
		return;

	_printOperand(MI, MCInst_getOperand(MI, OpNum), O);
}

static void printInlineJT(MCInst *MI, int OpNum, SStream *O)
{
}

static void printInlineJT32(MCInst *MI, int OpNum, SStream *O)
{
}

#define PRINT_ALIAS_INSTR
#include "XCoreGenAsmWriter.inc"

void XCore_printInst(MCInst *MI, SStream *O, void *Info)
{
	printInstruction(MI, O, Info);
	set_mem_access(MI, false, 0);
}

#endif
