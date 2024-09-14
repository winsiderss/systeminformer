/* Capstone Disassembly Engine */
/* M68K Backend by Daniel Collin <daniel@collin.com> 2015-2016 */

#ifndef CS_M68KDISASSEMBLER_H
#define CS_M68KDISASSEMBLER_H

#include "../../MCInst.h"

/* Private, For internal use only */
typedef struct m68k_info {
	const uint8_t *code;
	size_t code_len;
	uint64_t baseAddress;
	MCInst *inst;
	unsigned int pc;        /* program counter */
	unsigned int ir;        /* instruction register */
	unsigned int type;
	unsigned int address_mask; /* Address mask to simulate address lines */
	cs_m68k extension;
	uint16_t regs_read[20]; // list of implicit registers read by this insn
	uint8_t regs_read_count; // number of implicit registers read by this insn
	uint16_t regs_write[20]; // list of implicit registers modified by this insn
	uint8_t regs_write_count; // number of implicit registers modified by this insn
	uint8_t groups[8];
	uint8_t groups_count;
} m68k_info;

bool M68K_getInstruction(csh ud, const uint8_t* code, size_t code_len, MCInst* instr, uint16_t* size, uint64_t address, void* info);

#endif
