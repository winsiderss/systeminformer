#ifndef CAPSTONE_M680X_H
#define CAPSTONE_M680X_H

/* Capstone Disassembly Engine */
/* M680X Backend by Wolfgang Schwotzer <wolfgang.schwotzer@gmx.net> 2017 */

#ifdef __cplusplus
extern "C" {
#endif

#include "platform.h"

#ifdef _MSC_VER
#pragma warning(disable:4201)
#endif

#define M680X_OPERAND_COUNT 9

/// M680X registers and special registers
typedef enum m680x_reg {
	M680X_REG_INVALID = 0,

	M680X_REG_A, ///< M6800/1/2/3/9, HD6301/9
	M680X_REG_B, ///< M6800/1/2/3/9, HD6301/9
	M680X_REG_E, ///< HD6309
	M680X_REG_F, ///< HD6309
	M680X_REG_0, ///< HD6309

	M680X_REG_D, ///< M6801/3/9, HD6301/9
	M680X_REG_W, ///< HD6309

	M680X_REG_CC, ///< M6800/1/2/3/9, M6301/9
	M680X_REG_DP, ///< M6809/M6309
	M680X_REG_MD, ///< M6309

	M680X_REG_HX, ///< M6808
	M680X_REG_H, ///< M6808
	M680X_REG_X, ///< M6800/1/2/3/9, M6301/9
	M680X_REG_Y, ///< M6809/M6309
	M680X_REG_S, ///< M6809/M6309
	M680X_REG_U, ///< M6809/M6309
	M680X_REG_V, ///< M6309

	M680X_REG_Q, ///< M6309

	M680X_REG_PC, ///< M6800/1/2/3/9, M6301/9

	M680X_REG_TMP2, ///< CPU12
	M680X_REG_TMP3, ///< CPU12

	M680X_REG_ENDING,   ///< <-- mark the end of the list of registers
} m680x_reg;

/// Operand type for instruction's operands
typedef enum m680x_op_type {
	M680X_OP_INVALID = 0, ///< = CS_OP_INVALID (Uninitialized).
	M680X_OP_REGISTER,    ///< = Register operand.
	M680X_OP_IMMEDIATE,   ///< = Immediate operand.
	M680X_OP_INDEXED,     ///< = Indexed addressing operand.
	M680X_OP_EXTENDED,    ///< = Extended addressing operand.
	M680X_OP_DIRECT,      ///< = Direct addressing operand.
	M680X_OP_RELATIVE,    ///< = Relative addressing operand.
	M680X_OP_CONSTANT,    ///< = constant operand (Displayed as number only).
				///< Used e.g. for a bit index or page number.
} m680x_op_type;

// Supported bit values for mem.idx.offset_bits
#define M680X_OFFSET_NONE      0
#define M680X_OFFSET_BITS_5    5
#define M680X_OFFSET_BITS_8    8
#define M680X_OFFSET_BITS_9    9
#define M680X_OFFSET_BITS_16  16

// Supported bit flags for mem.idx.flags
// These flags can be combined
#define M680X_IDX_INDIRECT     1
#define M680X_IDX_NO_COMMA     2
#define M680X_IDX_POST_INC_DEC 4

/// Instruction's operand referring to indexed addressing
typedef struct m680x_op_idx {
	m680x_reg base_reg;	///< base register (or M680X_REG_INVALID if
				///< irrelevant)
	m680x_reg offset_reg;	///< offset register (or M680X_REG_INVALID if
				///< irrelevant)
	int16_t offset;		///< 5-,8- or 16-bit offset. See also offset_bits.
	uint16_t offset_addr;	///< = offset addr. if base_reg == M680X_REG_PC.
				///< calculated as offset + PC
	uint8_t offset_bits;	///< offset width in bits for indexed addressing
	int8_t inc_dec;		///< inc. or dec. value:
				///<    0: no inc-/decrement
				///<    1 .. 8: increment by 1 .. 8
				///<    -1 .. -8: decrement by 1 .. 8
				///< if flag M680X_IDX_POST_INC_DEC set it is post
				///< inc-/decrement otherwise pre inc-/decrement
	uint8_t flags;		///< 8-bit flags (see above)
} m680x_op_idx;

/// Instruction's memory operand referring to relative addressing (Bcc/LBcc)
typedef struct m680x_op_rel {
	uint16_t address;	///< The absolute address.
				///< calculated as PC + offset. PC is the first
				///< address after the instruction.
	int16_t offset;		///< the offset/displacement value
} m680x_op_rel;

/// Instruction's operand referring to extended addressing
typedef struct m680x_op_ext {
	uint16_t address;      ///< The absolute address
	bool indirect;         ///< true if extended indirect addressing
} m680x_op_ext;

/// Instruction operand
typedef struct cs_m680x_op {
	m680x_op_type type;
	union {
		int32_t imm;		///< immediate value for IMM operand
		m680x_reg reg;		///< register value for REG operand
		m680x_op_idx idx;	///< Indexed addressing operand
		m680x_op_rel rel;	///< Relative address. operand (Bcc/LBcc)
		m680x_op_ext ext;	///< Extended address
		uint8_t direct_addr;	///<</ Direct address (lower 8-bit)
		uint8_t const_val;	///< constant value (bit index, page nr.)
	};
	uint8_t size;			///< size of this operand (in bytes)
	/// How is this operand accessed? (READ, WRITE or READ|WRITE)
	/// This field is combined of cs_ac_type.
	/// NOTE: this field is irrelevant if engine is compiled in DIET 
	uint8_t access;
} cs_m680x_op;

/// Group of M680X instructions
typedef enum m680x_group_type {
	M680X_GRP_INVALID = 0,	/// = CS_GRP_INVALID
	// Generic groups
	// all jump instructions (conditional+direct+indirect jumps)
	M680X_GRP_JUMP,		///< = CS_GRP_JUMP
	// all call instructions
	M680X_GRP_CALL,		///< = CS_GRP_CALL
	// all return instructions
	M680X_GRP_RET,		///< = CS_GRP_RET
	// all interrupt instructions (int+syscall)
	M680X_GRP_INT,		///< = CS_GRP_INT
	// all interrupt return instructions
	M680X_GRP_IRET,		///< = CS_GRP_IRET
	// all privileged instructions
	M680X_GRP_PRIV,		///< = CS_GRP_PRIVILEDGE; not used
	// all relative branching instructions
	M680X_GRP_BRAREL,	///< = CS_GRP_BRANCH_RELATIVE

	// Architecture-specific groups
	M680X_GRP_ENDING,	// <-- mark the end of the list of groups
} m680x_group_type;

// M680X instruction flags:

/// The first (register) operand is part of the
/// instruction mnemonic
#define M680X_FIRST_OP_IN_MNEM    1
/// The second (register) operand is part of the
/// instruction mnemonic
#define M680X_SECOND_OP_IN_MNEM   2

/// The M680X instruction and it's operands
typedef struct cs_m680x {
	uint8_t flags;		///< See: M680X instruction flags
	uint8_t op_count;	///< number of operands for the instruction or 0
	cs_m680x_op operands[M680X_OPERAND_COUNT]; ///< operands for this insn.
} cs_m680x;

/// M680X instruction IDs
typedef enum m680x_insn {
	M680X_INS_INVLD = 0,
	M680X_INS_ABA, ///< M6800/1/2/3
	M680X_INS_ABX,
	M680X_INS_ABY,
	M680X_INS_ADC,
	M680X_INS_ADCA,
	M680X_INS_ADCB,
	M680X_INS_ADCD,
	M680X_INS_ADCR,
	M680X_INS_ADD,
	M680X_INS_ADDA,
	M680X_INS_ADDB,
	M680X_INS_ADDD,
	M680X_INS_ADDE,
	M680X_INS_ADDF,
	M680X_INS_ADDR,
	M680X_INS_ADDW,
	M680X_INS_AIM,
	M680X_INS_AIS,
	M680X_INS_AIX,
	M680X_INS_AND,
	M680X_INS_ANDA,
	M680X_INS_ANDB,
	M680X_INS_ANDCC,
	M680X_INS_ANDD,
	M680X_INS_ANDR,
	M680X_INS_ASL,
	M680X_INS_ASLA,
	M680X_INS_ASLB,
	M680X_INS_ASLD, ///< or LSLD
	M680X_INS_ASR,
	M680X_INS_ASRA,
	M680X_INS_ASRB,
	M680X_INS_ASRD,
	M680X_INS_ASRX,
	M680X_INS_BAND,
	M680X_INS_BCC, ///< or BHS
	M680X_INS_BCLR,
	M680X_INS_BCS, ///< or BLO
	M680X_INS_BEOR,
	M680X_INS_BEQ,
	M680X_INS_BGE,
	M680X_INS_BGND,
	M680X_INS_BGT,
	M680X_INS_BHCC,
	M680X_INS_BHCS,
	M680X_INS_BHI,
	M680X_INS_BIAND,
	M680X_INS_BIEOR,
	M680X_INS_BIH,
	M680X_INS_BIL,
	M680X_INS_BIOR,
	M680X_INS_BIT,
	M680X_INS_BITA,
	M680X_INS_BITB,
	M680X_INS_BITD,
	M680X_INS_BITMD,
	M680X_INS_BLE,
	M680X_INS_BLS,
	M680X_INS_BLT,
	M680X_INS_BMC,
	M680X_INS_BMI,
	M680X_INS_BMS,
	M680X_INS_BNE,
	M680X_INS_BOR,
	M680X_INS_BPL,
	M680X_INS_BRCLR,
	M680X_INS_BRSET,
	M680X_INS_BRA,
	M680X_INS_BRN,
	M680X_INS_BSET,
	M680X_INS_BSR,
	M680X_INS_BVC,
	M680X_INS_BVS,
	M680X_INS_CALL,
	M680X_INS_CBA, ///< M6800/1/2/3
	M680X_INS_CBEQ,
	M680X_INS_CBEQA,
	M680X_INS_CBEQX,
	M680X_INS_CLC, ///< M6800/1/2/3
	M680X_INS_CLI, ///< M6800/1/2/3
	M680X_INS_CLR,
	M680X_INS_CLRA,
	M680X_INS_CLRB,
	M680X_INS_CLRD,
	M680X_INS_CLRE,
	M680X_INS_CLRF,
	M680X_INS_CLRH,
	M680X_INS_CLRW,
	M680X_INS_CLRX,
	M680X_INS_CLV, ///< M6800/1/2/3
	M680X_INS_CMP,
	M680X_INS_CMPA,
	M680X_INS_CMPB,
	M680X_INS_CMPD,
	M680X_INS_CMPE,
	M680X_INS_CMPF,
	M680X_INS_CMPR,
	M680X_INS_CMPS,
	M680X_INS_CMPU,
	M680X_INS_CMPW,
	M680X_INS_CMPX,
	M680X_INS_CMPY,
	M680X_INS_COM,
	M680X_INS_COMA,
	M680X_INS_COMB,
	M680X_INS_COMD,
	M680X_INS_COME,
	M680X_INS_COMF,
	M680X_INS_COMW,
	M680X_INS_COMX,
	M680X_INS_CPD,
	M680X_INS_CPHX,
	M680X_INS_CPS,
	M680X_INS_CPX, ///< M6800/1/2/3
	M680X_INS_CPY,
	M680X_INS_CWAI,
	M680X_INS_DAA,
	M680X_INS_DBEQ,
	M680X_INS_DBNE,
	M680X_INS_DBNZ,
	M680X_INS_DBNZA,
	M680X_INS_DBNZX,
	M680X_INS_DEC,
	M680X_INS_DECA,
	M680X_INS_DECB,
	M680X_INS_DECD,
	M680X_INS_DECE,
	M680X_INS_DECF,
	M680X_INS_DECW,
	M680X_INS_DECX,
	M680X_INS_DES, ///< M6800/1/2/3
	M680X_INS_DEX, ///< M6800/1/2/3
	M680X_INS_DEY,
	M680X_INS_DIV,
	M680X_INS_DIVD,
	M680X_INS_DIVQ,
	M680X_INS_EDIV,
	M680X_INS_EDIVS,
	M680X_INS_EIM,
	M680X_INS_EMACS,
	M680X_INS_EMAXD,
	M680X_INS_EMAXM,
	M680X_INS_EMIND,
	M680X_INS_EMINM,
	M680X_INS_EMUL,
	M680X_INS_EMULS,
	M680X_INS_EOR,
	M680X_INS_EORA,
	M680X_INS_EORB,
	M680X_INS_EORD,
	M680X_INS_EORR,
	M680X_INS_ETBL,
	M680X_INS_EXG,
	M680X_INS_FDIV,
	M680X_INS_IBEQ,
	M680X_INS_IBNE,
	M680X_INS_IDIV,
	M680X_INS_IDIVS,
	M680X_INS_ILLGL,
	M680X_INS_INC,
	M680X_INS_INCA,
	M680X_INS_INCB,
	M680X_INS_INCD,
	M680X_INS_INCE,
	M680X_INS_INCF,
	M680X_INS_INCW,
	M680X_INS_INCX,
	M680X_INS_INS, ///< M6800/1/2/3
	M680X_INS_INX, ///< M6800/1/2/3
	M680X_INS_INY,
	M680X_INS_JMP,
	M680X_INS_JSR,
	M680X_INS_LBCC, ///< or LBHS
	M680X_INS_LBCS, ///< or LBLO
	M680X_INS_LBEQ,
	M680X_INS_LBGE,
	M680X_INS_LBGT,
	M680X_INS_LBHI,
	M680X_INS_LBLE,
	M680X_INS_LBLS,
	M680X_INS_LBLT,
	M680X_INS_LBMI,
	M680X_INS_LBNE,
	M680X_INS_LBPL,
	M680X_INS_LBRA,
	M680X_INS_LBRN,
	M680X_INS_LBSR,
	M680X_INS_LBVC,
	M680X_INS_LBVS,
	M680X_INS_LDA,
	M680X_INS_LDAA, ///< M6800/1/2/3
	M680X_INS_LDAB, ///< M6800/1/2/3
	M680X_INS_LDB,
	M680X_INS_LDBT,
	M680X_INS_LDD,
	M680X_INS_LDE,
	M680X_INS_LDF,
	M680X_INS_LDHX,
	M680X_INS_LDMD,
	M680X_INS_LDQ,
	M680X_INS_LDS,
	M680X_INS_LDU,
	M680X_INS_LDW,
	M680X_INS_LDX,
	M680X_INS_LDY,
	M680X_INS_LEAS,
	M680X_INS_LEAU,
	M680X_INS_LEAX,
	M680X_INS_LEAY,
	M680X_INS_LSL,
	M680X_INS_LSLA,
	M680X_INS_LSLB,
	M680X_INS_LSLD,
	M680X_INS_LSLX,
	M680X_INS_LSR,
	M680X_INS_LSRA,
	M680X_INS_LSRB,
	M680X_INS_LSRD, ///< or ASRD
	M680X_INS_LSRW,
	M680X_INS_LSRX,
	M680X_INS_MAXA,
	M680X_INS_MAXM,
	M680X_INS_MEM,
	M680X_INS_MINA,
	M680X_INS_MINM,
	M680X_INS_MOV,
	M680X_INS_MOVB,
	M680X_INS_MOVW,
	M680X_INS_MUL,
	M680X_INS_MULD,
	M680X_INS_NEG,
	M680X_INS_NEGA,
	M680X_INS_NEGB,
	M680X_INS_NEGD,
	M680X_INS_NEGX,
	M680X_INS_NOP,
	M680X_INS_NSA,
	M680X_INS_OIM,
	M680X_INS_ORA,
	M680X_INS_ORAA, ///< M6800/1/2/3
	M680X_INS_ORAB, ///< M6800/1/2/3
	M680X_INS_ORB,
	M680X_INS_ORCC,
	M680X_INS_ORD,
	M680X_INS_ORR,
	M680X_INS_PSHA, ///< M6800/1/2/3
	M680X_INS_PSHB, ///< M6800/1/2/3
	M680X_INS_PSHC,
	M680X_INS_PSHD,
	M680X_INS_PSHH,
	M680X_INS_PSHS,
	M680X_INS_PSHSW,
	M680X_INS_PSHU,
	M680X_INS_PSHUW,
	M680X_INS_PSHX, ///< M6800/1/2/3
	M680X_INS_PSHY,
	M680X_INS_PULA, ///< M6800/1/2/3
	M680X_INS_PULB, ///< M6800/1/2/3
	M680X_INS_PULC,
	M680X_INS_PULD,
	M680X_INS_PULH,
	M680X_INS_PULS,
	M680X_INS_PULSW,
	M680X_INS_PULU,
	M680X_INS_PULUW,
	M680X_INS_PULX, ///< M6800/1/2/3
	M680X_INS_PULY,
	M680X_INS_REV,
	M680X_INS_REVW,
	M680X_INS_ROL,
	M680X_INS_ROLA,
	M680X_INS_ROLB,
	M680X_INS_ROLD,
	M680X_INS_ROLW,
	M680X_INS_ROLX,
	M680X_INS_ROR,
	M680X_INS_RORA,
	M680X_INS_RORB,
	M680X_INS_RORD,
	M680X_INS_RORW,
	M680X_INS_RORX,
	M680X_INS_RSP,
	M680X_INS_RTC,
	M680X_INS_RTI,
	M680X_INS_RTS,
	M680X_INS_SBA, ///< M6800/1/2/3
	M680X_INS_SBC,
	M680X_INS_SBCA,
	M680X_INS_SBCB,
	M680X_INS_SBCD,
	M680X_INS_SBCR,
	M680X_INS_SEC,
	M680X_INS_SEI,
	M680X_INS_SEV,
	M680X_INS_SEX,
	M680X_INS_SEXW,
	M680X_INS_SLP,
	M680X_INS_STA,
	M680X_INS_STAA, ///< M6800/1/2/3
	M680X_INS_STAB, ///< M6800/1/2/3
	M680X_INS_STB,
	M680X_INS_STBT,
	M680X_INS_STD,
	M680X_INS_STE,
	M680X_INS_STF,
	M680X_INS_STOP,
	M680X_INS_STHX,
	M680X_INS_STQ,
	M680X_INS_STS,
	M680X_INS_STU,
	M680X_INS_STW,
	M680X_INS_STX,
	M680X_INS_STY,
	M680X_INS_SUB,
	M680X_INS_SUBA,
	M680X_INS_SUBB,
	M680X_INS_SUBD,
	M680X_INS_SUBE,
	M680X_INS_SUBF,
	M680X_INS_SUBR,
	M680X_INS_SUBW,
	M680X_INS_SWI,
	M680X_INS_SWI2,
	M680X_INS_SWI3,
	M680X_INS_SYNC,
	M680X_INS_TAB, ///< M6800/1/2/3
	M680X_INS_TAP, ///< M6800/1/2/3
	M680X_INS_TAX,
	M680X_INS_TBA, ///< M6800/1/2/3
	M680X_INS_TBEQ,
	M680X_INS_TBL,
	M680X_INS_TBNE,
	M680X_INS_TEST,
	M680X_INS_TFM,
	M680X_INS_TFR,
	M680X_INS_TIM,
	M680X_INS_TPA, ///< M6800/1/2/3
	M680X_INS_TST,
	M680X_INS_TSTA,
	M680X_INS_TSTB,
	M680X_INS_TSTD,
	M680X_INS_TSTE,
	M680X_INS_TSTF,
	M680X_INS_TSTW,
	M680X_INS_TSTX,
	M680X_INS_TSX, ///< M6800/1/2/3
	M680X_INS_TSY,
	M680X_INS_TXA,
	M680X_INS_TXS, ///< M6800/1/2/3
	M680X_INS_TYS,
	M680X_INS_WAI, ///< M6800/1/2/3
	M680X_INS_WAIT,
	M680X_INS_WAV,
	M680X_INS_WAVR,
	M680X_INS_XGDX, ///< HD6301
	M680X_INS_XGDY,
	M680X_INS_ENDING,   // <-- mark the end of the list of instructions
} m680x_insn;

#ifdef __cplusplus
}
#endif

#endif
