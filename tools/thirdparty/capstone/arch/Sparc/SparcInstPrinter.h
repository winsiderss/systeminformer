/* Capstone Disassembly Engine */
/* By Nguyen Anh Quynh <aquynh@gmail.com>, 2013-2015 */

#ifndef CS_SPARCINSTPRINTER_H
#define CS_SPARCINSTPRINTER_H

#include "../../MCInst.h"
#include "../../MCRegisterInfo.h"
#include "../../SStream.h"

void Sparc_printInst(MCInst *MI, SStream *O, void *Info);

void Sparc_post_printer(csh ud, cs_insn *insn, char *insn_asm, MCInst *mci);

void Sparc_addReg(MCInst *MI, int reg);

#endif
