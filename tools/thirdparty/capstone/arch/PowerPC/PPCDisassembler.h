/* Capstone Disassembly Engine */
/* By Nguyen Anh Quynh <aquynh@gmail.com>, 2013-2015 */

#ifndef CS_PPCDISASSEMBLER_H
#define CS_PPCDISASSEMBLER_H

#include "capstone/capstone.h"
#include "../../MCRegisterInfo.h"
#include "../../MCInst.h"

void PPC_init(MCRegisterInfo *MRI);

bool PPC_getInstruction(csh ud, const uint8_t *code, size_t code_len,
		MCInst *instr, uint16_t *size, uint64_t address, void *info);

#endif

