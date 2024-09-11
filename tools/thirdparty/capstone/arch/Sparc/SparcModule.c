/* Capstone Disassembly Engine */
/* By Nguyen Anh Quynh <aquynh@gmail.com>, 2013-2015 */

#ifdef CAPSTONE_HAS_SPARC

#include "../../utils.h"
#include "../../MCRegisterInfo.h"
#include "SparcDisassembler.h"
#include "SparcInstPrinter.h"
#include "SparcMapping.h"
#include "SparcModule.h"

cs_err Sparc_global_init(cs_struct *ud)
{
	MCRegisterInfo *mri;
	mri = cs_mem_malloc(sizeof(*mri));

	Sparc_init(mri);
	ud->printer = Sparc_printInst;
	ud->printer_info = mri;
	ud->getinsn_info = mri;
	ud->disasm = Sparc_getInstruction;
	ud->post_printer = Sparc_post_printer;

	ud->reg_name = Sparc_reg_name;
	ud->insn_id = Sparc_get_insn_id;
	ud->insn_name = Sparc_insn_name;
	ud->group_name = Sparc_group_name;

	return CS_ERR_OK;
}

cs_err Sparc_option(cs_struct *handle, cs_opt_type type, size_t value)
{
	if (type == CS_OPT_SYNTAX)
		handle->syntax = (int) value;

	if (type == CS_OPT_MODE) {
		handle->mode = (cs_mode)value;
	}

	return CS_ERR_OK;
}

#endif
