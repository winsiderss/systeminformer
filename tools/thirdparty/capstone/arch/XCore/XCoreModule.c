/* Capstone Disassembly Engine */
/* By Nguyen Anh Quynh <aquynh@gmail.com>, 2013-2015 */

#ifdef CAPSTONE_HAS_XCORE

#include "../../utils.h"
#include "../../MCRegisterInfo.h"
#include "XCoreDisassembler.h"
#include "XCoreInstPrinter.h"
#include "XCoreMapping.h"
#include "XCoreModule.h"

cs_err XCore_global_init(cs_struct *ud)
{
	MCRegisterInfo *mri;
	mri = cs_mem_malloc(sizeof(*mri));

	XCore_init(mri);
	ud->printer = XCore_printInst;
	ud->printer_info = mri;
	ud->getinsn_info = mri;
	ud->disasm = XCore_getInstruction;
	ud->post_printer = XCore_post_printer;

	ud->reg_name = XCore_reg_name;
	ud->insn_id = XCore_get_insn_id;
	ud->insn_name = XCore_insn_name;
	ud->group_name = XCore_group_name;

	return CS_ERR_OK;
}

cs_err XCore_option(cs_struct *handle, cs_opt_type type, size_t value)
{
	// Do not set mode because only CS_MODE_BIG_ENDIAN is valid; we cannot
	// test for CS_MODE_LITTLE_ENDIAN because it is 0

	return CS_ERR_OK;
}

#endif
