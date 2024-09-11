/* Capstone Disassembly Engine */
/* By Nguyen Anh Quynh <aquynh@gmail.com>, 2013-2015 */

#ifndef CS_SPARC_MAP_H
#define CS_SPARC_MAP_H

#include "capstone/capstone.h"

// return name of regiser in friendly string
const char *Sparc_reg_name(csh handle, unsigned int reg);

// given internal insn id, return public instruction info
void Sparc_get_insn_id(cs_struct *h, cs_insn *insn, unsigned int id);

const char *Sparc_insn_name(csh handle, unsigned int id);

const char *Sparc_group_name(csh handle, unsigned int id);

// map internal raw register to 'public' register
sparc_reg Sparc_map_register(unsigned int r);

// map instruction name to instruction ID (public)
// this is for alias instructions only
sparc_reg Sparc_map_insn(const char *name);

// map CC string to CC id
sparc_cc Sparc_map_ICC(const char *name);

sparc_cc Sparc_map_FCC(const char *name);

sparc_hint Sparc_map_hint(const char *name);

#endif

