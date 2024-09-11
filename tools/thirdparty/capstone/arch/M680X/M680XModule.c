/* Capstone Disassembly Engine */
/* M680X Backend by Wolfgang Schwotzer <wolfgang.schwotzer@gmx.net> 2017 */

#ifdef CAPSTONE_HAS_M680X

#include "../../utils.h"
#include "../../MCRegisterInfo.h"
#include "M680XDisassembler.h"
#include "M680XDisassemblerInternals.h"
#include "M680XInstPrinter.h"
#include "M680XModule.h"

cs_err M680X_global_init(cs_struct *ud)
{
	m680x_info *info;
	cs_err errcode;

	/* Do some validation checks */
	errcode = M680X_disassembler_init(ud);

	if (errcode != CS_ERR_OK)
		return errcode;

	errcode =  M680X_instprinter_init(ud);

	if (errcode != CS_ERR_OK)
		return errcode;

	// verify if requested mode is valid
	if (ud->mode & ~(CS_MODE_M680X_6800 | CS_MODE_M680X_6801 |
			CS_MODE_M680X_6805 | CS_MODE_M680X_6808 |
			CS_MODE_M680X_6809 | CS_MODE_M680X_6811 |
			CS_MODE_M680X_6301 | CS_MODE_M680X_6309 |
			CS_MODE_M680X_CPU12 | CS_MODE_M680X_HCS08)) {
		// At least one mode is not supported by M680X
		return CS_ERR_MODE;
	}

	if (!(ud->mode & (CS_MODE_M680X_6800 | CS_MODE_M680X_6801 |
				CS_MODE_M680X_6805 | CS_MODE_M680X_6808 |
				CS_MODE_M680X_6809 | CS_MODE_M680X_6811 |
				CS_MODE_M680X_6301 | CS_MODE_M680X_6309 |
				CS_MODE_M680X_CPU12 | CS_MODE_M680X_HCS08))) {
		// At least the cpu type has to be selected. No default.
		return CS_ERR_MODE;
	}

	info = cs_mem_malloc(sizeof(m680x_info));

	if (!info)
		return CS_ERR_MEM;

	ud->printer = M680X_printInst;
	ud->printer_info = info;
	ud->getinsn_info = NULL;
	ud->disasm = M680X_getInstruction;
	ud->reg_name = M680X_reg_name;
	ud->insn_id = M680X_get_insn_id;
	ud->insn_name = M680X_insn_name;
	ud->group_name = M680X_group_name;
	ud->skipdata_size = 1;
	ud->post_printer = NULL;
#ifndef CAPSTONE_DIET
	ud->reg_access = M680X_reg_access;
#endif

	return CS_ERR_OK;
}

cs_err M680X_option(cs_struct *handle, cs_opt_type type, size_t value)
{
	//TODO
	return CS_ERR_OK;
}

#endif

