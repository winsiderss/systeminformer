/* Capstone Disassembly Engine */
/* By Dang Hoang Vu <danghvu@gmail.com> 2013 */

#ifdef CAPSTONE_HAS_MIPS

#include "../../utils.h"
#include "../../MCRegisterInfo.h"
#include "MipsDisassembler.h"
#include "MipsInstPrinter.h"
#include "MipsMapping.h"
#include "MipsModule.h"

// Returns mode value with implied bits set
static cs_mode updated_mode(cs_mode mode)
{
	if (mode & CS_MODE_MIPS32R6) {
		mode |= CS_MODE_32;
	}

	return mode;
}

cs_err Mips_global_init(cs_struct *ud)
{
	MCRegisterInfo *mri;
	mri = cs_mem_malloc(sizeof(*mri));

	Mips_init(mri);
	ud->printer = Mips_printInst;
	ud->printer_info = mri;
	ud->getinsn_info = mri;
	ud->reg_name = Mips_reg_name;
	ud->insn_id = Mips_get_insn_id;
	ud->insn_name = Mips_insn_name;
	ud->group_name = Mips_group_name;

	ud->disasm = Mips_getInstruction;

	return CS_ERR_OK;
}

cs_err Mips_option(cs_struct *handle, cs_opt_type type, size_t value)
{
	if (type == CS_OPT_MODE) {
		handle->mode = updated_mode(value);
		return CS_ERR_OK;
	}

	return CS_ERR_OPTION;
}

#endif
