//===- ARMInstPrinter.h - Convert ARM MCInst to assembly syntax -*- C++ -*-===//
//
//                     The LLVM Compiler Infrastructure
//
// This file is distributed under the University of Illinois Open Source
// License. See LICENSE.TXT for details.
//
//===----------------------------------------------------------------------===//
//
// This class prints an ARM MCInst to a .s file.
//
//===----------------------------------------------------------------------===//

/* Capstone Disassembly Engine */
/* By Nguyen Anh Quynh <aquynh@gmail.com>, 2013-2015 */

#ifndef CS_ARMINSTPRINTER_H
#define CS_ARMINSTPRINTER_H

#include "../../MCInst.h"
#include "../../MCRegisterInfo.h"
#include "../../SStream.h"

void ARM_printInst(MCInst *MI, SStream *O, void *Info);
void ARM_post_printer(csh handle, cs_insn *pub_insn, char *mnem, MCInst *mci);

// setup handle->get_regname
void ARM_getRegName(cs_struct *handle, int value);

// specify vector data type for vector instructions
void ARM_addVectorDataType(MCInst *MI, arm_vectordata_type vd);

void ARM_addVectorDataSize(MCInst *MI, int size);

void ARM_addReg(MCInst *MI, int reg);

// load usermode registers (LDM, STM)
void ARM_addUserMode(MCInst *MI);

// sysreg for MRS/MSR
void ARM_addSysReg(MCInst *MI, arm_sysreg reg);

#endif
