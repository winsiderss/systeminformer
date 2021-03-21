/* Capstone Disassembly Engine */
/* By Nguyen Anh Quynh <aquynh@gmail.com>, 2013-2015 */

#ifndef CS_XCORE_MAP_H
#define CS_XCORE_MAP_H

#include "capstone/capstone.h"

// return name of regiser in friendly string
const char *XCore_reg_name(csh handle, unsigned int reg);

// given internal insn id, return public instruction info
void XCore_get_insn_id(cs_struct *h, cs_insn *insn, unsigned int id);

const char *XCore_insn_name(csh handle, unsigned int id);

const char *XCore_group_name(csh handle, unsigned int id);

// map internal raw register to 'public' register
xcore_reg XCore_map_register(unsigned int r);

// map register name to register ID
xcore_reg XCore_reg_id(char *name);

#endif

