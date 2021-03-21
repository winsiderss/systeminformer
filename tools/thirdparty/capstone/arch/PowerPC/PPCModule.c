/* Capstone Disassembly Engine */
/* By Nguyen Anh Quynh <aquynh@gmail.com>, 2013-2015 */

#ifdef CAPSTONE_HAS_POWERPC

#include "../../utils.h"
#include "../../MCRegisterInfo.h"
#include "PPCDisassembler.h"
#include "PPCInstPrinter.h"
#include "PPCMapping.h"
#include "PPCModule.h"

cs_err PPC_global_init(cs_struct *ud)
{
	MCRegisterInfo *mri;
	mri = (MCRegisterInfo *) cs_mem_malloc(sizeof(*mri));

	PPC_init(mri);
	ud->printer = PPC_printInst;
	ud->printer_info = mri;
	ud->getinsn_info = mri;
	ud->disasm = PPC_getInstruction;
	ud->post_printer = PPC_post_printer;

	ud->reg_name = PPC_reg_name;
	ud->insn_id = PPC_get_insn_id;
	ud->insn_name = PPC_insn_name;
	ud->group_name = PPC_group_name;

	return CS_ERR_OK;
}

cs_err PPC_option(cs_struct *handle, cs_opt_type type, size_t value)
{
	if (type == CS_OPT_SYNTAX)
		handle->syntax = (int) value;

	if (type == CS_OPT_MODE) {
		handle->mode = (cs_mode)value;
	}

	return CS_ERR_OK;
}

#endif
