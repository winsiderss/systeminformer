/* Capstone Disassembly Engine */
/* By Travis Finkenauer <tmfinken@gmail.com>, 2018 */

#ifndef CS_MIPS_MODULE_H
#define CS_MIPS_MODULE_H

#include "../../utils.h"

cs_err Mips_global_init(cs_struct *ud);
cs_err Mips_option(cs_struct *handle, cs_opt_type type, size_t value);

#endif
