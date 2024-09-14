/* Capstone Disassembly Engine */
/* By Nguyen Anh Quynh, 2018 */

#include "EVMInstPrinter.h"
#include "EVMMapping.h"


void EVM_printInst(MCInst *MI, struct SStream *O, void *PrinterInfo)
{
	SStream_concat(O, EVM_insn_name((csh)MI->csh, MI->Opcode));

	if (MI->Opcode >= EVM_INS_PUSH1 && MI->Opcode <= EVM_INS_PUSH32) {
		unsigned int i;

		SStream_concat0(O, "\t");
		for (i = 0; i < MI->Opcode - EVM_INS_PUSH1 + 1; i++) {
			SStream_concat(O, "%02x", MI->evm_data[i]);
		}
	}
}
