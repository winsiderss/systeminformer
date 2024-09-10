/* Capstone Disassembly Engine */
/* M680X Backend by Wolfgang Schwotzer <wolfgang.schwotzer@gmx.net> 2017 */

#ifndef CS_M680XDISASSEMBLER_H
#define CS_M680XDISASSEMBLER_H

#include "../../MCInst.h"

bool M680X_getInstruction(csh ud, const uint8_t *code, size_t code_len,
	MCInst *instr, uint16_t *size, uint64_t address, void *info);
void M680X_get_insn_id(cs_struct *h, cs_insn *insn, unsigned int id);
void M680X_reg_access(const cs_insn *insn,
	cs_regs regs_read, uint8_t *regs_read_count,
	cs_regs regs_write, uint8_t *regs_write_count);

#endif

