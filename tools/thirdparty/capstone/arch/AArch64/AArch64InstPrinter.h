//===-- AArch64InstPrinter.h - Convert AArch64 MCInst to assembly syntax --===//
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
/* By Nguyen Anh Quynh <aquynh@gmail.com>, 2013-2015 */

#ifndef CS_LLVM_AARCH64INSTPRINTER_H
#define CS_LLVM_AARCH64INSTPRINTER_H

#include "../../MCInst.h"
#include "../../MCRegisterInfo.h"
#include "../../SStream.h"

void AArch64_printInst(MCInst *MI, SStream *O, void *);

void AArch64_post_printer(csh handle, cs_insn *pub_insn, char *insn_asm, MCInst *mci);

#endif
