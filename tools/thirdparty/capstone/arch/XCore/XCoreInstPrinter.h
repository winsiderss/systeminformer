/* Capstone Disassembly Engine */
/* By Nguyen Anh Quynh <aquynh@gmail.com>, 2013-2015 */

#ifndef CS_XCOREINSTPRINTER_H
#define CS_XCOREINSTPRINTER_H

#include "../../MCInst.h"
#include "../../MCRegisterInfo.h"
#include "../../SStream.h"

void XCore_printInst(MCInst *MI, SStream *O, void *Info);

void XCore_post_printer(csh ud, cs_insn *insn, char *insn_asm, MCInst *mci);

// extract details from assembly code @code
void XCore_insn_extract(MCInst *MI, const char *code);

#endif
