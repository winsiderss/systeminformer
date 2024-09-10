/* Capstone Disassembly Engine */
/* TMS320C64x Backend by Fotis Loukos <me@fotisl.com> 2016 */

#ifndef CS_TMS320C64X_MAP_H
#define CS_TMS320C64X_MAP_H

#include "capstone/capstone.h"

// return name of regiser in friendly string
const char *TMS320C64x_reg_name(csh handle, unsigned int reg);

// given internal insn id, return public instruction info
void TMS320C64x_get_insn_id(cs_struct *h, cs_insn *insn, unsigned int id);

const char *TMS320C64x_insn_name(csh handle, unsigned int id);

const char *TMS320C64x_group_name(csh handle, unsigned int id);

// map internal raw register to 'public' register
tms320c64x_reg TMS320C64x_map_register(unsigned int r);

// map register name to register ID
tms320c64x_reg TMS320C64x_reg_id(char *name);

#endif

