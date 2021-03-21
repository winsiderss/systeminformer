/* Capstone Disassembly Engine */
/* By Nguyen Anh Quynh <aquynh@gmail.com>, 2013-2015 */

#ifndef CS_PPCINSTPRINTER_H
#define CS_PPCINSTPRINTER_H

#include "../../MCInst.h"
#include "../../MCRegisterInfo.h"
#include "../../SStream.h"

void PPC_printInst(MCInst *MI, SStream *O, void *Info);

void PPC_post_printer(csh ud, cs_insn *insn, char *insn_asm, MCInst *mci);

#endif
