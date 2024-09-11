/* Capstone Disassembly Engine */
/* By Travis Finkenauer <tmfinken@gmail.com>, 2018 */

#ifndef CS_TMS320C64X_MODULE_H
#define CS_TMS320C64X_MODULE_H

#include "../../utils.h"

cs_err TMS320C64x_global_init(cs_struct *ud);
cs_err TMS320C64x_option(cs_struct *handle, cs_opt_type type, size_t value);

#endif
