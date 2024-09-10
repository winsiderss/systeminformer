/* Capstone Disassembly Engine */
/* By Nguyen Anh Quynh, 2018 */

#ifndef CS_EVMINSTPRINTER_H
#define CS_EVMINSTPRINTER_H


#include "../capstone/include/capstone/capstone.h"
#include "../../MCInst.h"
#include "../../SStream.h"
#include "../../cs_priv.h"

struct SStream;

void EVM_printInst(MCInst *MI, struct SStream *O, void *Info);

#endif
