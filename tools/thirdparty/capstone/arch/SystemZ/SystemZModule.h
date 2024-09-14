/* Capstone Disassembly Engine */
/* By Travis Finkenauer <tmfinken@gmail.com>, 2018 */

#ifndef CS_SYSTEMZ_MODULE_H
#define CS_SYSTEMZ_MODULE_H

#include "../../utils.h"

cs_err SystemZ_global_init(cs_struct *ud);
cs_err SystemZ_option(cs_struct *handle, cs_opt_type type, size_t value);

#endif
