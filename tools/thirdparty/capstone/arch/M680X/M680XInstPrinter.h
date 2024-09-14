/* Capstone Disassembly Engine */
/* M680X Backend by Wolfgang Schwotzer <wolfgang.schwotzer@gmx.net> 2017 */

#ifndef CS_M680XINSTPRINTER_H
#define CS_M680XINSTPRINTER_H


#include "../capstone/include/capstone/capstone.h"
#include "../../MCRegisterInfo.h"
#include "../../MCInst.h"

struct SStream;

void M680X_init(MCRegisterInfo *MRI);

void M680X_printInst(MCInst *MI, struct SStream *O, void *Info);
const char *M680X_reg_name(csh handle, unsigned int reg);
const char *M680X_insn_name(csh handle, unsigned int id);
const char *M680X_group_name(csh handle, unsigned int id);
void M680X_post_printer(csh handle, cs_insn *flat_insn, char *insn_asm,
	MCInst *mci);

#endif


