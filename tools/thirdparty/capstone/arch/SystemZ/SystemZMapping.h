/* Capstone Disassembly Engine */
/* By Nguyen Anh Quynh <aquynh@gmail.com>, 2013-2015 */

#ifndef CS_SYSZ_MAP_H
#define CS_SYSZ_MAP_H

#include "capstone/capstone.h"

// return name of regiser in friendly string
const char *SystemZ_reg_name(csh handle, unsigned int reg);

// given internal insn id, return public instruction info
void SystemZ_get_insn_id(cs_struct *h, cs_insn *insn, unsigned int id);

const char *SystemZ_insn_name(csh handle, unsigned int id);

const char *SystemZ_group_name(csh handle, unsigned int id);

// map internal raw register to 'public' register
sysz_reg SystemZ_map_register(unsigned int r);

#endif

