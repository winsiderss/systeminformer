/* Capstone Disassembly Engine */
/* By Nguyen Anh Quynh <aquynh@gmail.com>, 2013-2015 */

#ifndef CS_MIPS_MAP_H
#define CS_MIPS_MAP_H

#include "capstone/capstone.h"

// return name of regiser in friendly string
const char *Mips_reg_name(csh handle, unsigned int reg);

// given internal insn id, return public instruction info
void Mips_get_insn_id(cs_struct *h, cs_insn *insn, unsigned int id);

const char *Mips_insn_name(csh handle, unsigned int id);

const char *Mips_group_name(csh handle, unsigned int id);

// map instruction name to instruction ID
mips_reg Mips_map_insn(const char *name);

// map internal raw register to 'public' register
mips_reg Mips_map_register(unsigned int r);

#endif
