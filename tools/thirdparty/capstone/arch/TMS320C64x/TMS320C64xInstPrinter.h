/* Capstone Disassembly Engine */
/* TMS320C64x Backend by Fotis Loukos <me@fotisl.com> 2016 */

#ifndef CS_TMS320C64XINSTPRINTER_H
#define CS_TMS320C64XINSTPRINTER_H

#include "../../MCInst.h"
#include "../../MCRegisterInfo.h"
#include "../../SStream.h"

void TMS320C64x_printInst(MCInst *MI, SStream *O, void *Info);

void TMS320C64x_post_printer(csh ud, cs_insn *insn, char *insn_asm, MCInst *mci);

#endif
