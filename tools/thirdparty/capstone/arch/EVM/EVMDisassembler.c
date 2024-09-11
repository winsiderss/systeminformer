/* Capstone Disassembly Engine */
/* By Nguyen Anh Quynh, 2018 */

#include <string.h>
#include <stddef.h> // offsetof macro
                    // alternatively #include "../../utils.h" like everyone else

#include "EVMDisassembler.h"
#include "EVMMapping.h"

static short opcodes[256] = {
	EVM_INS_STOP,
	EVM_INS_ADD,
	EVM_INS_MUL,
	EVM_INS_SUB,
	EVM_INS_DIV,
	EVM_INS_SDIV,
	EVM_INS_MOD,
	EVM_INS_SMOD,
	EVM_INS_ADDMOD,
	EVM_INS_MULMOD,
	EVM_INS_EXP,
	EVM_INS_SIGNEXTEND,
	-1,
	-1,
	-1,
	-1,
	EVM_INS_LT,
	EVM_INS_GT,
	EVM_INS_SLT,
	EVM_INS_SGT,
	EVM_INS_EQ,
	EVM_INS_ISZERO,
	EVM_INS_AND,
	EVM_INS_OR,
	EVM_INS_XOR,
	EVM_INS_NOT,
	EVM_INS_BYTE,
	-1,
	-1,
	-1,
	-1,
	-1,
	EVM_INS_SHA3,
	-1,
	-1,
	-1,
	-1,
	-1,
	-1,
	-1,
	-1,
	-1,
	-1,
	-1,
	-1,
	-1,
	-1,
	-1,
	EVM_INS_ADDRESS,
	EVM_INS_BALANCE,
	EVM_INS_ORIGIN,
	EVM_INS_CALLER,
	EVM_INS_CALLVALUE,
	EVM_INS_CALLDATALOAD,
	EVM_INS_CALLDATASIZE,
	EVM_INS_CALLDATACOPY,
	EVM_INS_CODESIZE,
	EVM_INS_CODECOPY,
	EVM_INS_GASPRICE,
	EVM_INS_EXTCODESIZE,
	EVM_INS_EXTCODECOPY,
	EVM_INS_RETURNDATASIZE,
	EVM_INS_RETURNDATACOPY,
	-1,
	EVM_INS_BLOCKHASH,
	EVM_INS_COINBASE,
	EVM_INS_TIMESTAMP,
	EVM_INS_NUMBER,
	EVM_INS_DIFFICULTY,
	EVM_INS_GASLIMIT,
	-1,
	-1,
	-1,
	-1,
	-1,
	-1,
	-1,
	-1,
	-1,
	-1,
	EVM_INS_POP,
	EVM_INS_MLOAD,
	EVM_INS_MSTORE,
	EVM_INS_MSTORE8,
	EVM_INS_SLOAD,
	EVM_INS_SSTORE,
	EVM_INS_JUMP,
	EVM_INS_JUMPI,
	EVM_INS_PC,
	EVM_INS_MSIZE,
	EVM_INS_GAS,
	EVM_INS_JUMPDEST,
	-1,
	-1,
	-1,
	-1,
	EVM_INS_PUSH1,
	EVM_INS_PUSH2,
	EVM_INS_PUSH3,
	EVM_INS_PUSH4,
	EVM_INS_PUSH5,
	EVM_INS_PUSH6,
	EVM_INS_PUSH7,
	EVM_INS_PUSH8,
	EVM_INS_PUSH9,
	EVM_INS_PUSH10,
	EVM_INS_PUSH11,
	EVM_INS_PUSH12,
	EVM_INS_PUSH13,
	EVM_INS_PUSH14,
	EVM_INS_PUSH15,
	EVM_INS_PUSH16,
	EVM_INS_PUSH17,
	EVM_INS_PUSH18,
	EVM_INS_PUSH19,
	EVM_INS_PUSH20,
	EVM_INS_PUSH21,
	EVM_INS_PUSH22,
	EVM_INS_PUSH23,
	EVM_INS_PUSH24,
	EVM_INS_PUSH25,
	EVM_INS_PUSH26,
	EVM_INS_PUSH27,
	EVM_INS_PUSH28,
	EVM_INS_PUSH29,
	EVM_INS_PUSH30,
	EVM_INS_PUSH31,
	EVM_INS_PUSH32,
	EVM_INS_DUP1,
	EVM_INS_DUP2,
	EVM_INS_DUP3,
	EVM_INS_DUP4,
	EVM_INS_DUP5,
	EVM_INS_DUP6,
	EVM_INS_DUP7,
	EVM_INS_DUP8,
	EVM_INS_DUP9,
	EVM_INS_DUP10,
	EVM_INS_DUP11,
	EVM_INS_DUP12,
	EVM_INS_DUP13,
	EVM_INS_DUP14,
	EVM_INS_DUP15,
	EVM_INS_DUP16,
	EVM_INS_SWAP1,
	EVM_INS_SWAP2,
	EVM_INS_SWAP3,
	EVM_INS_SWAP4,
	EVM_INS_SWAP5,
	EVM_INS_SWAP6,
	EVM_INS_SWAP7,
	EVM_INS_SWAP8,
	EVM_INS_SWAP9,
	EVM_INS_SWAP10,
	EVM_INS_SWAP11,
	EVM_INS_SWAP12,
	EVM_INS_SWAP13,
	EVM_INS_SWAP14,
	EVM_INS_SWAP15,
	EVM_INS_SWAP16,
	EVM_INS_LOG0,
	EVM_INS_LOG1,
	EVM_INS_LOG2,
	EVM_INS_LOG3,
	EVM_INS_LOG4,
	-1,
	-1,
	-1,
	-1,
	-1,
	-1,
	-1,
	-1,
	-1,
	-1,
	-1,
	-1,
	-1,
	-1,
	-1,
	-1,
	-1,
	-1,
	-1,
	-1,
	-1,
	-1,
	-1,
	-1,
	-1,
	-1,
	-1,
	-1,
	-1,
	-1,
	-1,
	-1,
	-1,
	-1,
	-1,
	-1,
	-1,
	-1,
	-1,
	-1,
	-1,
	-1,
	-1,
	-1,
	-1,
	-1,
	-1,
	-1,
	-1,
	-1,
	-1,
	-1,
	-1,
	-1,
	-1,
	-1,
	-1,
	-1,
	-1,
	-1,
	-1,
	-1,
	-1,
	-1,
	-1,
	-1,
	-1,
	-1,
	-1,
	-1,
	-1,
	-1,
	-1,
	-1,
	-1,
	EVM_INS_CREATE,
	EVM_INS_CALL,
	EVM_INS_CALLCODE,
	EVM_INS_RETURN,
	EVM_INS_DELEGATECALL,
	EVM_INS_CALLBLACKBOX,
	-1,
	-1,
	-1,
	-1,
	EVM_INS_STATICCALL,
	-1,
	-1,
	EVM_INS_REVERT,
	-1,
	EVM_INS_SUICIDE,
};

bool EVM_getInstruction(csh ud, const uint8_t *code, size_t code_len,
	MCInst *MI, uint16_t *size, uint64_t address, void *inst_info)
{
	unsigned char opcode;

	if (code_len == 0)
		return false;

	opcode = code[0];
	if (opcodes[opcode] == -1) {
		// invalid opcode
		return false;
	}

	// valid opcode
	MI->address = address;
	MI->OpcodePub = MI->Opcode = opcode;

	if (opcode >= EVM_INS_PUSH1 && opcode <= EVM_INS_PUSH32) {
		unsigned char len = (opcode - EVM_INS_PUSH1 + 1);
		if (code_len < 1 + (size_t)len) {
			// not enough data
			return false;
		}

		*size = 1 + len;
		memcpy(MI->evm_data, code + 1, len);
	} else
		*size = 1;

	if (MI->flat_insn->detail) {
		memset(MI->flat_insn->detail, 0, offsetof(cs_detail, evm)+sizeof(cs_evm));
		EVM_get_insn_id((cs_struct *)ud, MI->flat_insn, opcode);

		if (MI->flat_insn->detail->evm.pop) {
			MI->flat_insn->detail->groups[MI->flat_insn->detail->groups_count] = EVM_GRP_STACK_READ;
			MI->flat_insn->detail->groups_count++;
		}

		if (MI->flat_insn->detail->evm.push) {
			MI->flat_insn->detail->groups[MI->flat_insn->detail->groups_count] = EVM_GRP_STACK_WRITE;
			MI->flat_insn->detail->groups_count++;
		}

		// setup groups
		switch(opcode) {
			default:
				break;
			case EVM_INS_ADD:
			case EVM_INS_MUL:
			case EVM_INS_SUB:
			case EVM_INS_DIV:
			case EVM_INS_SDIV:
			case EVM_INS_MOD:
			case EVM_INS_SMOD:
			case EVM_INS_ADDMOD:
			case EVM_INS_MULMOD:
			case EVM_INS_EXP:
			case EVM_INS_SIGNEXTEND:
				MI->flat_insn->detail->groups[MI->flat_insn->detail->groups_count] = EVM_GRP_MATH;
				MI->flat_insn->detail->groups_count++;
				break;

			case EVM_INS_MSTORE:
			case EVM_INS_MSTORE8:
			case EVM_INS_CALLDATACOPY:
			case EVM_INS_CODECOPY:
			case EVM_INS_EXTCODECOPY:
				MI->flat_insn->detail->groups[MI->flat_insn->detail->groups_count] = EVM_GRP_MEM_WRITE;
				MI->flat_insn->detail->groups_count++;
				break;

			case EVM_INS_MLOAD:
			case EVM_INS_CREATE:
			case EVM_INS_CALL:
			case EVM_INS_CALLCODE:
			case EVM_INS_RETURN:
			case EVM_INS_DELEGATECALL:
			case EVM_INS_REVERT:
				MI->flat_insn->detail->groups[MI->flat_insn->detail->groups_count] = EVM_GRP_MEM_READ;
				MI->flat_insn->detail->groups_count++;
				break;

			case EVM_INS_SSTORE:
				MI->flat_insn->detail->groups[MI->flat_insn->detail->groups_count] = EVM_GRP_STORE_WRITE;
				MI->flat_insn->detail->groups_count++;
				break;

			case EVM_INS_SLOAD:
				MI->flat_insn->detail->groups[MI->flat_insn->detail->groups_count] = EVM_GRP_STORE_READ;
				MI->flat_insn->detail->groups_count++;
				break;

			case EVM_INS_JUMP:
			case EVM_INS_JUMPI:
				MI->flat_insn->detail->groups[MI->flat_insn->detail->groups_count] = EVM_GRP_JUMP;
				MI->flat_insn->detail->groups_count++;
				break;

			case EVM_INS_STOP:
			case EVM_INS_SUICIDE:
				MI->flat_insn->detail->groups[MI->flat_insn->detail->groups_count] = EVM_GRP_HALT;
				MI->flat_insn->detail->groups_count++;
				break;

		}
	}

	return true;
}
