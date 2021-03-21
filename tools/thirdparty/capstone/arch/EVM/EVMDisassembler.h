/* Capstone Disassembly Engine */
/* By Nguyen Anh Quynh, 2018 */

#ifndef CS_EVMDISASSEMBLER_H
#define CS_EVMDISASSEMBLER_H

#include "../../MCInst.h"

bool EVM_getInstruction(csh ud, const uint8_t *code, size_t code_len,
		MCInst *instr, uint16_t *size, uint64_t address, void *info);

#endif
