/* Capstone Disassembly Engine */
/* By Dang Hoang Vu <danghvu@gmail.com> 2013 */

#ifdef CAPSTONE_HAS_X86

#include "../../cs_priv.h"
#include "../../MCRegisterInfo.h"
#include "X86Disassembler.h"
#include "X86InstPrinter.h"
#include "X86Mapping.h"
#include "X86Module.h"

cs_err X86_global_init(cs_struct *ud)
{
	MCRegisterInfo *mri;
	mri = cs_mem_malloc(sizeof(*mri));

	X86_init(mri);

	// by default, we use Intel syntax
	ud->printer = X86_Intel_printInst;
	ud->syntax = CS_OPT_SYNTAX_INTEL;
	ud->printer_info = mri;
	ud->disasm = X86_getInstruction;
	ud->reg_name = X86_reg_name;
	ud->insn_id = X86_get_insn_id;
	ud->insn_name = X86_insn_name;
	ud->group_name = X86_group_name;
	ud->post_printer = NULL;;
#ifndef CAPSTONE_DIET
	ud->reg_access = X86_reg_access;
#endif

	if (ud->mode == CS_MODE_64)
		ud->regsize_map = regsize_map_64;
	else
		ud->regsize_map = regsize_map_32;

	return CS_ERR_OK;
}

cs_err X86_option(cs_struct *handle, cs_opt_type type, size_t value)
{
	switch(type) {
		default:
			break;
		case CS_OPT_MODE:
			if (value == CS_MODE_64)
				handle->regsize_map = regsize_map_64;
			else
				handle->regsize_map = regsize_map_32;

			handle->mode = (cs_mode)value;
			break;
		case CS_OPT_SYNTAX:
			switch(value) {
				default:
					// wrong syntax value
					handle->errnum = CS_ERR_OPTION;
					return CS_ERR_OPTION;

				case CS_OPT_SYNTAX_DEFAULT:
				case CS_OPT_SYNTAX_INTEL:
					handle->syntax = CS_OPT_SYNTAX_INTEL;
					handle->printer = X86_Intel_printInst;
					break;

				case CS_OPT_SYNTAX_MASM:
					handle->printer = X86_Intel_printInst;
					handle->syntax = (int)value;
					break;

				case CS_OPT_SYNTAX_ATT:
#if !defined(CAPSTONE_DIET) && !defined(CAPSTONE_X86_ATT_DISABLE)
					handle->printer = X86_ATT_printInst;
					handle->syntax = CS_OPT_SYNTAX_ATT;
					break;
#elif !defined(CAPSTONE_DIET) && defined(CAPSTONE_X86_ATT_DISABLE)
					// ATT syntax is unsupported
					handle->errnum = CS_ERR_X86_ATT;
					return CS_ERR_X86_ATT;
#else	// CAPSTONE_DIET
					// this is irrelevant in CAPSTONE_DIET mode
					handle->errnum = CS_ERR_DIET;
					return CS_ERR_DIET;
#endif
			}
			break;
	}

	return CS_ERR_OK;
}

#endif
