/* Capstone Disassembly Engine */
/* By Nguyen Anh Quynh <aquynh@gmail.com>, 2013-2015 */

#ifndef CS_ARM_MAP_H
#define CS_ARM_MAP_H

#include "../../include/capstone/capstone.h"
#include "../../utils.h"

// return name of regiser in friendly string
const char *ARM_reg_name(csh handle, unsigned int reg);
const char *ARM_reg_name2(csh handle, unsigned int reg);

// given internal insn id, return public instruction ID
void ARM_get_insn_id(cs_struct *h, cs_insn *insn, unsigned int id);

const char *ARM_insn_name(csh handle, unsigned int id);

const char *ARM_group_name(csh handle, unsigned int id);

// check if this insn is relative branch
bool ARM_rel_branch(cs_struct *h, unsigned int insn_id);

bool ARM_blx_to_arm_mode(cs_struct *h, unsigned int insn_id);

uint8_t *ARM_get_op_access(cs_struct *h, unsigned int id);

void ARM_reg_access(const cs_insn *insn,
		cs_regs regs_read, uint8_t *regs_read_count,
		cs_regs regs_write, uint8_t *regs_write_count);

#endif
