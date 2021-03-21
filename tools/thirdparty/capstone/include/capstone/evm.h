#ifndef CAPSTONE_EVM_H
#define CAPSTONE_EVM_H

/* Capstone Disassembly Engine */
/* By Nguyen Anh Quynh <aquynh@gmail.com>, 2013-2018 */

#ifdef __cplusplus
extern "C" {
#endif

#include "platform.h"

#ifdef _MSC_VER
#pragma warning(disable:4201)
#endif

/// Instruction structure
typedef struct cs_evm {
    unsigned char pop;    ///< number of items popped from the stack
    unsigned char push;   ///< number of items pushed into the stack
    unsigned int  fee;    ///< gas fee for the instruction
} cs_evm;

/// EVM instruction
typedef enum evm_insn {
	EVM_INS_STOP = 0,
	EVM_INS_ADD = 1,
	EVM_INS_MUL = 2,
	EVM_INS_SUB = 3,
	EVM_INS_DIV = 4,
	EVM_INS_SDIV = 5,
	EVM_INS_MOD = 6,
	EVM_INS_SMOD = 7,
	EVM_INS_ADDMOD = 8,
	EVM_INS_MULMOD = 9,
	EVM_INS_EXP = 10,
	EVM_INS_SIGNEXTEND = 11,
	EVM_INS_LT = 16,
	EVM_INS_GT = 17,
	EVM_INS_SLT = 18,
	EVM_INS_SGT = 19,
	EVM_INS_EQ = 20,
	EVM_INS_ISZERO = 21,
	EVM_INS_AND = 22,
	EVM_INS_OR = 23,
	EVM_INS_XOR = 24,
	EVM_INS_NOT = 25,
	EVM_INS_BYTE = 26,
	EVM_INS_SHA3 = 32,
	EVM_INS_ADDRESS = 48,
	EVM_INS_BALANCE = 49,
	EVM_INS_ORIGIN = 50,
	EVM_INS_CALLER = 51,
	EVM_INS_CALLVALUE = 52,
	EVM_INS_CALLDATALOAD = 53,
	EVM_INS_CALLDATASIZE = 54,
	EVM_INS_CALLDATACOPY = 55,
	EVM_INS_CODESIZE = 56,
	EVM_INS_CODECOPY = 57,
	EVM_INS_GASPRICE = 58,
	EVM_INS_EXTCODESIZE = 59,
	EVM_INS_EXTCODECOPY = 60,
	EVM_INS_RETURNDATASIZE = 61,
	EVM_INS_RETURNDATACOPY = 62,
	EVM_INS_BLOCKHASH = 64,
	EVM_INS_COINBASE = 65,
	EVM_INS_TIMESTAMP = 66,
	EVM_INS_NUMBER = 67,
	EVM_INS_DIFFICULTY = 68,
	EVM_INS_GASLIMIT = 69,
	EVM_INS_POP = 80,
	EVM_INS_MLOAD = 81,
	EVM_INS_MSTORE = 82,
	EVM_INS_MSTORE8 = 83,
	EVM_INS_SLOAD = 84,
	EVM_INS_SSTORE = 85,
	EVM_INS_JUMP = 86,
	EVM_INS_JUMPI = 87,
	EVM_INS_PC = 88,
	EVM_INS_MSIZE = 89,
	EVM_INS_GAS = 90,
	EVM_INS_JUMPDEST = 91,
	EVM_INS_PUSH1 = 96,
	EVM_INS_PUSH2 = 97,
	EVM_INS_PUSH3 = 98,
	EVM_INS_PUSH4 = 99,
	EVM_INS_PUSH5 = 100,
	EVM_INS_PUSH6 = 101,
	EVM_INS_PUSH7 = 102,
	EVM_INS_PUSH8 = 103,
	EVM_INS_PUSH9 = 104,
	EVM_INS_PUSH10 = 105,
	EVM_INS_PUSH11 = 106,
	EVM_INS_PUSH12 = 107,
	EVM_INS_PUSH13 = 108,
	EVM_INS_PUSH14 = 109,
	EVM_INS_PUSH15 = 110,
	EVM_INS_PUSH16 = 111,
	EVM_INS_PUSH17 = 112,
	EVM_INS_PUSH18 = 113,
	EVM_INS_PUSH19 = 114,
	EVM_INS_PUSH20 = 115,
	EVM_INS_PUSH21 = 116,
	EVM_INS_PUSH22 = 117,
	EVM_INS_PUSH23 = 118,
	EVM_INS_PUSH24 = 119,
	EVM_INS_PUSH25 = 120,
	EVM_INS_PUSH26 = 121,
	EVM_INS_PUSH27 = 122,
	EVM_INS_PUSH28 = 123,
	EVM_INS_PUSH29 = 124,
	EVM_INS_PUSH30 = 125,
	EVM_INS_PUSH31 = 126,
	EVM_INS_PUSH32 = 127,
	EVM_INS_DUP1 = 128,
	EVM_INS_DUP2 = 129,
	EVM_INS_DUP3 = 130,
	EVM_INS_DUP4 = 131,
	EVM_INS_DUP5 = 132,
	EVM_INS_DUP6 = 133,
	EVM_INS_DUP7 = 134,
	EVM_INS_DUP8 = 135,
	EVM_INS_DUP9 = 136,
	EVM_INS_DUP10 = 137,
	EVM_INS_DUP11 = 138,
	EVM_INS_DUP12 = 139,
	EVM_INS_DUP13 = 140,
	EVM_INS_DUP14 = 141,
	EVM_INS_DUP15 = 142,
	EVM_INS_DUP16 = 143,
	EVM_INS_SWAP1 = 144,
	EVM_INS_SWAP2 = 145,
	EVM_INS_SWAP3 = 146,
	EVM_INS_SWAP4 = 147,
	EVM_INS_SWAP5 = 148,
	EVM_INS_SWAP6 = 149,
	EVM_INS_SWAP7 = 150,
	EVM_INS_SWAP8 = 151,
	EVM_INS_SWAP9 = 152,
	EVM_INS_SWAP10 = 153,
	EVM_INS_SWAP11 = 154,
	EVM_INS_SWAP12 = 155,
	EVM_INS_SWAP13 = 156,
	EVM_INS_SWAP14 = 157,
	EVM_INS_SWAP15 = 158,
	EVM_INS_SWAP16 = 159,
	EVM_INS_LOG0 = 160,
	EVM_INS_LOG1 = 161,
	EVM_INS_LOG2 = 162,
	EVM_INS_LOG3 = 163,
	EVM_INS_LOG4 = 164,
	EVM_INS_CREATE = 240,
	EVM_INS_CALL = 241,
	EVM_INS_CALLCODE = 242,
	EVM_INS_RETURN = 243,
	EVM_INS_DELEGATECALL = 244,
	EVM_INS_CALLBLACKBOX = 245,
	EVM_INS_STATICCALL = 250,
	EVM_INS_REVERT = 253,
	EVM_INS_SUICIDE = 255,

	EVM_INS_INVALID = 512,
	EVM_INS_ENDING,   // <-- mark the end of the list of instructions
} evm_insn;

/// Group of EVM instructions
typedef enum evm_insn_group {
	EVM_GRP_INVALID = 0, ///< = CS_GRP_INVALID

	EVM_GRP_JUMP,          ///< all jump instructions

	EVM_GRP_MATH = 8,      ///< math instructions
	EVM_GRP_STACK_WRITE,   ///< instructions write to stack
	EVM_GRP_STACK_READ,    ///< instructions read from stack
	EVM_GRP_MEM_WRITE,     ///< instructions write to memory
	EVM_GRP_MEM_READ,      ///< instructions read from memory
	EVM_GRP_STORE_WRITE,   ///< instructions write to storage
	EVM_GRP_STORE_READ,    ///< instructions read from storage
	EVM_GRP_HALT,    ///< instructions halt execution

	EVM_GRP_ENDING,   ///< <-- mark the end of the list of groups
} evm_insn_group;

#ifdef __cplusplus
}
#endif

#endif
