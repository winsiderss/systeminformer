/* Capstone Disassembly Engine */
/* By Nguyen Anh Quynh, 2018 */

#ifdef CAPSTONE_HAS_EVM

#include "../../cs_priv.h"
#include "EVMDisassembler.h"
#include "EVMInstPrinter.h"
#include "EVMMapping.h"
#include "EVMModule.h"

cs_err EVM_global_init(cs_struct *ud)
{
	// verify if requested mode is valid
	if (ud->mode)
		return CS_ERR_MODE;

	ud->printer = EVM_printInst;
	ud->printer_info = NULL;
	ud->insn_id = EVM_get_insn_id;
	ud->insn_name = EVM_insn_name;
	ud->group_name = EVM_group_name;
	ud->disasm = EVM_getInstruction;

	return CS_ERR_OK;
}

cs_err EVM_option(cs_struct *handle, cs_opt_type type, size_t value)
{
	return CS_ERR_OK;
}

#endif
