/* Capstone Disassembly Engine */
/* TMS320C64x Backend by Fotis Loukos <me@fotisl.com> 2016 */

#ifndef CS_TMS320C64XDISASSEMBLER_H
#define CS_TMS320C64XDISASSEMBLER_H

#include <stdint.h>

#include "capstone/capstone.h"
#include "../../MCRegisterInfo.h"
#include "../../MCInst.h"

void TMS320C64x_init(MCRegisterInfo *MRI);

bool TMS320C64x_getInstruction(csh ud, const uint8_t *code, size_t code_len,
		MCInst *instr, uint16_t *size, uint64_t address, void *info);

#endif

