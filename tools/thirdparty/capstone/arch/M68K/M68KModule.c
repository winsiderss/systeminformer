/* Capstone Disassembly Engine */
/* M68K Backend by Daniel Collin <daniel@collin.com> 2015 */

#ifdef CAPSTONE_HAS_M68K

#include "../../utils.h"
#include "../../MCRegisterInfo.h"
#include "M68KDisassembler.h"
#include "M68KInstPrinter.h"
#include "M68KModule.h"

cs_err M68K_global_init(cs_struct *ud)
{
	m68k_info *info;

	info = cs_mem_malloc(sizeof(m68k_info));
	if (!info) {
		return CS_ERR_MEM;
	}

	ud->printer = M68K_printInst;
	ud->printer_info = info;
	ud->getinsn_info = NULL;
	ud->disasm = M68K_getInstruction;
	ud->skipdata_size = 2;
	ud->post_printer = NULL;

	ud->reg_name = M68K_reg_name;
	ud->insn_id = M68K_get_insn_id;
	ud->insn_name = M68K_insn_name;
	ud->group_name = M68K_group_name;

	return CS_ERR_OK;
}

cs_err M68K_option(cs_struct *handle, cs_opt_type type, size_t value)
{
	return CS_ERR_OK;
}

#endif

