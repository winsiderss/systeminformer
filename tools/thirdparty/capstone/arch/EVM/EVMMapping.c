/* Capstone Disassembly Engine */
/* By Nguyen Anh Quynh, 2018 */

#ifdef CAPSTONE_HAS_EVM

#include <string.h>

#include "../../cs_priv.h"
#include "../../utils.h"

#include "EVMMapping.h"

#ifndef CAPSTONE_DIET
static cs_evm insns[256] = {
#include "EVMMappingInsn.inc"
};
#endif

// look for @id in @insns, given its size in @max.
// return -1 if not found
static int evm_insn_find(cs_evm *insns, unsigned int max, unsigned int id)
{
	if (id >= max)
		return -1;

	if (insns[id].fee == 0xffffffff)
		// unused opcode
		return -1;

	return (int)id;
}

// fill in details
void EVM_get_insn_id(cs_struct *h, cs_insn *insn, unsigned int id)
{
	insn->id = id;
#ifndef CAPSTONE_DIET
	if (evm_insn_find(insns, ARR_SIZE(insns), id) > 0) {
		if (h->detail) {
			memcpy(&insn->detail->evm, &insns[id], sizeof(insns[id]));
		}
	}
#endif
}

#ifndef CAPSTONE_DIET
static name_map insn_name_maps[256] = {
	{ EVM_INS_STOP, "stop" },
	{ EVM_INS_ADD, "add" },
	{ EVM_INS_MUL, "mul" },
	{ EVM_INS_SUB, "sub" },
	{ EVM_INS_DIV, "div" },
	{ EVM_INS_SDIV, "sdiv" },
	{ EVM_INS_MOD, "mod" },
	{ EVM_INS_SMOD, "smod" },
	{ EVM_INS_ADDMOD, "addmod" },
	{ EVM_INS_MULMOD, "mulmod" },
	{ EVM_INS_EXP, "exp" },
	{ EVM_INS_SIGNEXTEND, "signextend" },
	{ EVM_INS_INVALID, NULL },
	{ EVM_INS_INVALID, NULL },
	{ EVM_INS_INVALID, NULL },
	{ EVM_INS_INVALID, NULL },
	{ EVM_INS_LT, "lt" },
	{ EVM_INS_GT, "gt" },
	{ EVM_INS_SLT, "slt" },
	{ EVM_INS_SGT, "sgt" },
	{ EVM_INS_EQ, "eq" },
	{ EVM_INS_ISZERO, "iszero" },
	{ EVM_INS_AND, "and" },
	{ EVM_INS_OR, "or" },
	{ EVM_INS_XOR, "xor" },
	{ EVM_INS_NOT, "not" },
	{ EVM_INS_BYTE, "byte" },
	{ EVM_INS_INVALID, NULL },
	{ EVM_INS_INVALID, NULL },
	{ EVM_INS_INVALID, NULL },
	{ EVM_INS_INVALID, NULL },
	{ EVM_INS_INVALID, NULL },
	{ EVM_INS_SHA3, "sha3" },
	{ EVM_INS_INVALID, NULL },
	{ EVM_INS_INVALID, NULL },
	{ EVM_INS_INVALID, NULL },
	{ EVM_INS_INVALID, NULL },
	{ EVM_INS_INVALID, NULL },
	{ EVM_INS_INVALID, NULL },
	{ EVM_INS_INVALID, NULL },
	{ EVM_INS_INVALID, NULL },
	{ EVM_INS_INVALID, NULL },
	{ EVM_INS_INVALID, NULL },
	{ EVM_INS_INVALID, NULL },
	{ EVM_INS_INVALID, NULL },
	{ EVM_INS_INVALID, NULL },
	{ EVM_INS_INVALID, NULL },
	{ EVM_INS_INVALID, NULL },
	{ EVM_INS_ADDRESS, "address" },
	{ EVM_INS_BALANCE, "balance" },
	{ EVM_INS_ORIGIN, "origin" },
	{ EVM_INS_CALLER, "caller" },
	{ EVM_INS_CALLVALUE, "callvalue" },
	{ EVM_INS_CALLDATALOAD, "calldataload" },
	{ EVM_INS_CALLDATASIZE, "calldatasize" },
	{ EVM_INS_CALLDATACOPY, "calldatacopy" },
	{ EVM_INS_CODESIZE, "codesize" },
	{ EVM_INS_CODECOPY, "codecopy" },
	{ EVM_INS_GASPRICE, "gasprice" },
	{ EVM_INS_EXTCODESIZE, "extcodesize" },
	{ EVM_INS_EXTCODECOPY, "extcodecopy" },
	{ EVM_INS_RETURNDATASIZE, "returndatasize" },
	{ EVM_INS_RETURNDATACOPY, "returndatacopy" },
	{ EVM_INS_INVALID, NULL },
	{ EVM_INS_BLOCKHASH, "blockhash" },
	{ EVM_INS_COINBASE, "coinbase" },
	{ EVM_INS_TIMESTAMP, "timestamp" },
	{ EVM_INS_NUMBER, "number" },
	{ EVM_INS_DIFFICULTY, "difficulty" },
	{ EVM_INS_GASLIMIT, "gaslimit" },
	{ EVM_INS_INVALID, NULL },
	{ EVM_INS_INVALID, NULL },
	{ EVM_INS_INVALID, NULL },
	{ EVM_INS_INVALID, NULL },
	{ EVM_INS_INVALID, NULL },
	{ EVM_INS_INVALID, NULL },
	{ EVM_INS_INVALID, NULL },
	{ EVM_INS_INVALID, NULL },
	{ EVM_INS_INVALID, NULL },
	{ EVM_INS_INVALID, NULL },
	{ EVM_INS_POP, "pop" },
	{ EVM_INS_MLOAD, "mload" },
	{ EVM_INS_MSTORE, "mstore" },
	{ EVM_INS_MSTORE8, "mstore8" },
	{ EVM_INS_SLOAD, "sload" },
	{ EVM_INS_SSTORE, "sstore" },
	{ EVM_INS_JUMP, "jump" },
	{ EVM_INS_JUMPI, "jumpi" },
	{ EVM_INS_PC, "pc" },
	{ EVM_INS_MSIZE, "msize" },
	{ EVM_INS_GAS, "gas" },
	{ EVM_INS_JUMPDEST, "jumpdest" },
	{ EVM_INS_INVALID, NULL },
	{ EVM_INS_INVALID, NULL },
	{ EVM_INS_INVALID, NULL },
	{ EVM_INS_INVALID, NULL },
	{ EVM_INS_PUSH1, "push1" },
	{ EVM_INS_PUSH2, "push2" },
	{ EVM_INS_PUSH3, "push3" },
	{ EVM_INS_PUSH4, "push4" },
	{ EVM_INS_PUSH5, "push5" },
	{ EVM_INS_PUSH6, "push6" },
	{ EVM_INS_PUSH7, "push7" },
	{ EVM_INS_PUSH8, "push8" },
	{ EVM_INS_PUSH9, "push9" },
	{ EVM_INS_PUSH10, "push10" },
	{ EVM_INS_PUSH11, "push11" },
	{ EVM_INS_PUSH12, "push12" },
	{ EVM_INS_PUSH13, "push13" },
	{ EVM_INS_PUSH14, "push14" },
	{ EVM_INS_PUSH15, "push15" },
	{ EVM_INS_PUSH16, "push16" },
	{ EVM_INS_PUSH17, "push17" },
	{ EVM_INS_PUSH18, "push18" },
	{ EVM_INS_PUSH19, "push19" },
	{ EVM_INS_PUSH20, "push20" },
	{ EVM_INS_PUSH21, "push21" },
	{ EVM_INS_PUSH22, "push22" },
	{ EVM_INS_PUSH23, "push23" },
	{ EVM_INS_PUSH24, "push24" },
	{ EVM_INS_PUSH25, "push25" },
	{ EVM_INS_PUSH26, "push26" },
	{ EVM_INS_PUSH27, "push27" },
	{ EVM_INS_PUSH28, "push28" },
	{ EVM_INS_PUSH29, "push29" },
	{ EVM_INS_PUSH30, "push30" },
	{ EVM_INS_PUSH31, "push31" },
	{ EVM_INS_PUSH32, "push32" },
	{ EVM_INS_DUP1, "dup1" },
	{ EVM_INS_DUP2, "dup2" },
	{ EVM_INS_DUP3, "dup3" },
	{ EVM_INS_DUP4, "dup4" },
	{ EVM_INS_DUP5, "dup5" },
	{ EVM_INS_DUP6, "dup6" },
	{ EVM_INS_DUP7, "dup7" },
	{ EVM_INS_DUP8, "dup8" },
	{ EVM_INS_DUP9, "dup9" },
	{ EVM_INS_DUP10, "dup10" },
	{ EVM_INS_DUP11, "dup11" },
	{ EVM_INS_DUP12, "dup12" },
	{ EVM_INS_DUP13, "dup13" },
	{ EVM_INS_DUP14, "dup14" },
	{ EVM_INS_DUP15, "dup15" },
	{ EVM_INS_DUP16, "dup16" },
	{ EVM_INS_SWAP1, "swap1" },
	{ EVM_INS_SWAP2, "swap2" },
	{ EVM_INS_SWAP3, "swap3" },
	{ EVM_INS_SWAP4, "swap4" },
	{ EVM_INS_SWAP5, "swap5" },
	{ EVM_INS_SWAP6, "swap6" },
	{ EVM_INS_SWAP7, "swap7" },
	{ EVM_INS_SWAP8, "swap8" },
	{ EVM_INS_SWAP9, "swap9" },
	{ EVM_INS_SWAP10, "swap10" },
	{ EVM_INS_SWAP11, "swap11" },
	{ EVM_INS_SWAP12, "swap12" },
	{ EVM_INS_SWAP13, "swap13" },
	{ EVM_INS_SWAP14, "swap14" },
	{ EVM_INS_SWAP15, "swap15" },
	{ EVM_INS_SWAP16, "swap16" },
	{ EVM_INS_LOG0, "log0" },
	{ EVM_INS_LOG1, "log1" },
	{ EVM_INS_LOG2, "log2" },
	{ EVM_INS_LOG3, "log3" },
	{ EVM_INS_LOG4, "log4" },
	{ EVM_INS_INVALID, NULL },
	{ EVM_INS_INVALID, NULL },
	{ EVM_INS_INVALID, NULL },
	{ EVM_INS_INVALID, NULL },
	{ EVM_INS_INVALID, NULL },
	{ EVM_INS_INVALID, NULL },
	{ EVM_INS_INVALID, NULL },
	{ EVM_INS_INVALID, NULL },
	{ EVM_INS_INVALID, NULL },
	{ EVM_INS_INVALID, NULL },
	{ EVM_INS_INVALID, NULL },
	{ EVM_INS_INVALID, NULL },
	{ EVM_INS_INVALID, NULL },
	{ EVM_INS_INVALID, NULL },
	{ EVM_INS_INVALID, NULL },
	{ EVM_INS_INVALID, NULL },
	{ EVM_INS_INVALID, NULL },
	{ EVM_INS_INVALID, NULL },
	{ EVM_INS_INVALID, NULL },
	{ EVM_INS_INVALID, NULL },
	{ EVM_INS_INVALID, NULL },
	{ EVM_INS_INVALID, NULL },
	{ EVM_INS_INVALID, NULL },
	{ EVM_INS_INVALID, NULL },
	{ EVM_INS_INVALID, NULL },
	{ EVM_INS_INVALID, NULL },
	{ EVM_INS_INVALID, NULL },
	{ EVM_INS_INVALID, NULL },
	{ EVM_INS_INVALID, NULL },
	{ EVM_INS_INVALID, NULL },
	{ EVM_INS_INVALID, NULL },
	{ EVM_INS_INVALID, NULL },
	{ EVM_INS_INVALID, NULL },
	{ EVM_INS_INVALID, NULL },
	{ EVM_INS_INVALID, NULL },
	{ EVM_INS_INVALID, NULL },
	{ EVM_INS_INVALID, NULL },
	{ EVM_INS_INVALID, NULL },
	{ EVM_INS_INVALID, NULL },
	{ EVM_INS_INVALID, NULL },
	{ EVM_INS_INVALID, NULL },
	{ EVM_INS_INVALID, NULL },
	{ EVM_INS_INVALID, NULL },
	{ EVM_INS_INVALID, NULL },
	{ EVM_INS_INVALID, NULL },
	{ EVM_INS_INVALID, NULL },
	{ EVM_INS_INVALID, NULL },
	{ EVM_INS_INVALID, NULL },
	{ EVM_INS_INVALID, NULL },
	{ EVM_INS_INVALID, NULL },
	{ EVM_INS_INVALID, NULL },
	{ EVM_INS_INVALID, NULL },
	{ EVM_INS_INVALID, NULL },
	{ EVM_INS_INVALID, NULL },
	{ EVM_INS_INVALID, NULL },
	{ EVM_INS_INVALID, NULL },
	{ EVM_INS_INVALID, NULL },
	{ EVM_INS_INVALID, NULL },
	{ EVM_INS_INVALID, NULL },
	{ EVM_INS_INVALID, NULL },
	{ EVM_INS_INVALID, NULL },
	{ EVM_INS_INVALID, NULL },
	{ EVM_INS_INVALID, NULL },
	{ EVM_INS_INVALID, NULL },
	{ EVM_INS_INVALID, NULL },
	{ EVM_INS_INVALID, NULL },
	{ EVM_INS_INVALID, NULL },
	{ EVM_INS_INVALID, NULL },
	{ EVM_INS_INVALID, NULL },
	{ EVM_INS_INVALID, NULL },
	{ EVM_INS_INVALID, NULL },
	{ EVM_INS_INVALID, NULL },
	{ EVM_INS_INVALID, NULL },
	{ EVM_INS_INVALID, NULL },
	{ EVM_INS_INVALID, NULL },
	{ EVM_INS_CREATE, "create" },
	{ EVM_INS_CALL, "call" },
	{ EVM_INS_CALLCODE, "callcode" },
	{ EVM_INS_RETURN, "return" },
	{ EVM_INS_DELEGATECALL, "delegatecall" },
	{ EVM_INS_CALLBLACKBOX, "callblackbox" },
	{ EVM_INS_INVALID, NULL },
	{ EVM_INS_INVALID, NULL },
	{ EVM_INS_INVALID, NULL },
	{ EVM_INS_INVALID, NULL },
	{ EVM_INS_STATICCALL, "staticcall" },
	{ EVM_INS_INVALID, NULL },
	{ EVM_INS_INVALID, NULL },
	{ EVM_INS_REVERT, "revert" },
	{ EVM_INS_INVALID, NULL },
	{ EVM_INS_SUICIDE, "suicide" },
};
#endif

const char *EVM_insn_name(csh handle, unsigned int id)
{
#ifndef CAPSTONE_DIET
	if (id >= ARR_SIZE(insn_name_maps))
		return NULL;
	else
		return insn_name_maps[id].name;
#else
	return NULL;
#endif
}

#ifndef CAPSTONE_DIET
static name_map group_name_maps[] = {
	// generic groups
	{ EVM_GRP_INVALID, NULL },
	{ EVM_GRP_JUMP,	"jump" },
	// special groups
	{ EVM_GRP_MATH,	"math" },
	{ EVM_GRP_STACK_WRITE, "stack_write" },
	{ EVM_GRP_STACK_READ, "stack_read" },
	{ EVM_GRP_MEM_WRITE, "mem_write" },
	{ EVM_GRP_MEM_READ, "mem_read" },
	{ EVM_GRP_STORE_WRITE, "store_write" },
	{ EVM_GRP_STORE_READ, "store_read" },
	{ EVM_GRP_HALT, "halt" },
};
#endif

const char *EVM_group_name(csh handle, unsigned int id)
{
#ifndef CAPSTONE_DIET
	return id2name(group_name_maps, ARR_SIZE(group_name_maps), id);
#else
	return NULL;
#endif
}
#endif
