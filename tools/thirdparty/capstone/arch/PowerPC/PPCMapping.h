/* Capstone Disassembly Engine */
/* By Nguyen Anh Quynh <aquynh@gmail.com>, 2013-2015 */

#ifndef CS_PPC_MAP_H
#define CS_PPC_MAP_H

#include "capstone/capstone.h"

// return name of regiser in friendly string
const char *PPC_reg_name(csh handle, unsigned int reg);

// given internal insn id, return public instruction info
void PPC_get_insn_id(cs_struct *h, cs_insn *insn, unsigned int id);

const char *PPC_insn_name(csh handle, unsigned int id);
const char *PPC_group_name(csh handle, unsigned int id);

// map internal raw register to 'public' register
ppc_reg PPC_map_register(unsigned int r);

struct ppc_alias {
	unsigned int id;	// instruction id
	int cc;	// code condition
	const char *mnem;
};

// given alias mnemonic, return instruction ID & CC
bool PPC_alias_insn(const char *name, struct ppc_alias *alias);

// check if this insn is relative branch
bool PPC_abs_branch(cs_struct *h, unsigned int id);

// map instruction name to public instruction ID
ppc_insn PPC_map_insn(const char *name);

#endif

