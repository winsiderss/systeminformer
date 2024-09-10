/* Capstone Disassembly Engine */
/* By Nguyen Anh Quynh, 2018 */

#include "../capstone/include/capstone/capstone.h"

void EVM_get_insn_id(cs_struct *h, cs_insn *insn, unsigned int id);
const char *EVM_insn_name(csh handle, unsigned int id);
const char *EVM_group_name(csh handle, unsigned int id);
