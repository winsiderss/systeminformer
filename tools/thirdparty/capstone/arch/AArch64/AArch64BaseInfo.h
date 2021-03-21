//===-- AArch64BaseInfo.h - Top level definitions for AArch64- --*- C++ -*-===//
//
//                     The LLVM Compiler Infrastructure
//
// This file is distributed under the University of Illinois Open Source
// License. See LICENSE.TXT for details.
//
//===----------------------------------------------------------------------===//
//
// This file contains small standalone helper functions and enum definitions for
// the AArch64 target useful for the compiler back-end and the MC libraries.
// As such, it deliberately does not include references to LLVM core
// code gen types, passes, etc..
//
//===----------------------------------------------------------------------===//

/* Capstone Disassembly Engine */
/* By Nguyen Anh Quynh <aquynh@gmail.com>, 2013-2015 */

#ifndef CS_LLVM_AARCH64_BASEINFO_H
#define CS_LLVM_AARCH64_BASEINFO_H

#include <ctype.h>
#include <string.h>

#ifndef __cplusplus
#if defined (WIN32) || defined (WIN64) || defined (_WIN32) || defined (_WIN64)
#define inline /* inline */
#endif
#endif

inline static unsigned getWRegFromXReg(unsigned Reg)
{
	switch (Reg) {
		case ARM64_REG_X0: return ARM64_REG_W0;
		case ARM64_REG_X1: return ARM64_REG_W1;
		case ARM64_REG_X2: return ARM64_REG_W2;
		case ARM64_REG_X3: return ARM64_REG_W3;
		case ARM64_REG_X4: return ARM64_REG_W4;
		case ARM64_REG_X5: return ARM64_REG_W5;
		case ARM64_REG_X6: return ARM64_REG_W6;
		case ARM64_REG_X7: return ARM64_REG_W7;
		case ARM64_REG_X8: return ARM64_REG_W8;
		case ARM64_REG_X9: return ARM64_REG_W9;
		case ARM64_REG_X10: return ARM64_REG_W10;
		case ARM64_REG_X11: return ARM64_REG_W11;
		case ARM64_REG_X12: return ARM64_REG_W12;
		case ARM64_REG_X13: return ARM64_REG_W13;
		case ARM64_REG_X14: return ARM64_REG_W14;
		case ARM64_REG_X15: return ARM64_REG_W15;
		case ARM64_REG_X16: return ARM64_REG_W16;
		case ARM64_REG_X17: return ARM64_REG_W17;
		case ARM64_REG_X18: return ARM64_REG_W18;
		case ARM64_REG_X19: return ARM64_REG_W19;
		case ARM64_REG_X20: return ARM64_REG_W20;
		case ARM64_REG_X21: return ARM64_REG_W21;
		case ARM64_REG_X22: return ARM64_REG_W22;
		case ARM64_REG_X23: return ARM64_REG_W23;
		case ARM64_REG_X24: return ARM64_REG_W24;
		case ARM64_REG_X25: return ARM64_REG_W25;
		case ARM64_REG_X26: return ARM64_REG_W26;
		case ARM64_REG_X27: return ARM64_REG_W27;
		case ARM64_REG_X28: return ARM64_REG_W28;
		case ARM64_REG_FP: return ARM64_REG_W29;
		case ARM64_REG_LR: return ARM64_REG_W30;
		case ARM64_REG_SP: return ARM64_REG_WSP;
		case ARM64_REG_XZR: return ARM64_REG_WZR;
	}

	// For anything else, return it unchanged.
	return Reg;
}

// // Enums corresponding to AArch64 condition codes
// The CondCodes constants map directly to the 4-bit encoding of the
// condition field for predicated instructions.
typedef enum A64CC_CondCode { // Meaning (integer)     Meaning (floating-point)
	A64CC_EQ = 0,        // Equal                      Equal
	A64CC_NE,            // Not equal                  Not equal, or unordered
	A64CC_HS,            // Unsigned higher or same    >, ==, or unordered
	A64CC_LO,            // Unsigned lower or same     Less than
	A64CC_MI,            // Minus, negative            Less than
	A64CC_PL,            // Plus, positive or zero     >, ==, or unordered
	A64CC_VS,            // Overflow                   Unordered
	A64CC_VC,            // No overflow                Ordered
	A64CC_HI,            // Unsigned higher            Greater than, or unordered
	A64CC_LS,            // Unsigned lower or same     Less than or equal
	A64CC_GE,            // Greater than or equal      Greater than or equal
	A64CC_LT,            // Less than                  Less than, or unordered
	A64CC_GT,            // Signed greater than        Greater than
	A64CC_LE,            // Signed less than or equal  <, ==, or unordered
	A64CC_AL,            // Always (unconditional)     Always (unconditional)
	A64CC_NV,             // Always (unconditional)     Always (unconditional)
	// Note the NV exists purely to disassemble 0b1111. Execution is "always".
	A64CC_Invalid
} A64CC_CondCode;

inline static const char *getCondCodeName(A64CC_CondCode CC)
{
	switch (CC) {
		default: return NULL;	// never reach
		case A64CC_EQ:  return "eq";
		case A64CC_NE:  return "ne";
		case A64CC_HS:  return "hs";
		case A64CC_LO:  return "lo";
		case A64CC_MI:  return "mi";
		case A64CC_PL:  return "pl";
		case A64CC_VS:  return "vs";
		case A64CC_VC:  return "vc";
		case A64CC_HI:  return "hi";
		case A64CC_LS:  return "ls";
		case A64CC_GE:  return "ge";
		case A64CC_LT:  return "lt";
		case A64CC_GT:  return "gt";
		case A64CC_LE:  return "le";
		case A64CC_AL:  return "al";
		case A64CC_NV:  return "nv";
	}
}

inline static A64CC_CondCode getInvertedCondCode(A64CC_CondCode Code)
{
	// To reverse a condition it's necessary to only invert the low bit:
	return (A64CC_CondCode)((unsigned)Code ^ 0x1);
}

/// Instances of this class can perform bidirectional mapping from random
/// identifier strings to operand encodings. For example "MSR" takes a named
/// system-register which must be encoded somehow and decoded for printing. This
/// central location means that the information for those transformations is not
/// duplicated and remains in sync.
///
/// FIXME: currently the algorithm is a completely unoptimised linear
/// search. Obviously this could be improved, but we would probably want to work
/// out just how often these instructions are emitted before working on it. It
/// might even be optimal to just reorder the tables for the common instructions
/// rather than changing the algorithm.
typedef struct A64NamedImmMapper_Mapping {
	const char *Name;
	uint32_t Value;
} A64NamedImmMapper_Mapping;

typedef struct A64NamedImmMapper {
	const A64NamedImmMapper_Mapping *Pairs;
	size_t NumPairs;
	uint32_t TooBigImm;
} A64NamedImmMapper;

typedef struct A64SysRegMapper {
	const A64NamedImmMapper_Mapping *SysRegPairs;
	const A64NamedImmMapper_Mapping *InstPairs;
	size_t NumInstPairs;
} A64SysRegMapper;

extern const A64SysRegMapper AArch64_MSRMapper;
extern const A64SysRegMapper AArch64_MRSMapper;

extern const A64NamedImmMapper A64DB_DBarrierMapper;
extern const A64NamedImmMapper A64AT_ATMapper;
extern const A64NamedImmMapper A64DC_DCMapper;
extern const A64NamedImmMapper A64IC_ICMapper;
extern const A64NamedImmMapper A64ISB_ISBMapper;
extern const A64NamedImmMapper A64PRFM_PRFMMapper;
extern const A64NamedImmMapper A64PState_PStateMapper;
extern const A64NamedImmMapper A64TLBI_TLBIMapper;

enum {
	A64AT_Invalid = -1,    // Op0 Op1  CRn   CRm   Op2
	A64AT_S1E1R = 0x43c0,  // 01  000  0111  1000  000
	A64AT_S1E2R = 0x63c0,  // 01  100  0111  1000  000
	A64AT_S1E3R = 0x73c0,  // 01  110  0111  1000  000
	A64AT_S1E1W = 0x43c1,  // 01  000  0111  1000  001
	A64AT_S1E2W = 0x63c1,  // 01  100  0111  1000  001
	A64AT_S1E3W = 0x73c1,  // 01  110  0111  1000  001
	A64AT_S1E0R = 0x43c2,  // 01  000  0111  1000  010
	A64AT_S1E0W = 0x43c3,  // 01  000  0111  1000  011
	A64AT_S12E1R = 0x63c4, // 01  100  0111  1000  100
	A64AT_S12E1W = 0x63c5, // 01  100  0111  1000  101
	A64AT_S12E0R = 0x63c6, // 01  100  0111  1000  110
	A64AT_S12E0W = 0x63c7  // 01  100  0111  1000  111
};

enum A64DBValues {
	A64DB_Invalid = -1,
	A64DB_OSHLD = 0x1,
	A64DB_OSHST = 0x2,
	A64DB_OSH =   0x3,
	A64DB_NSHLD = 0x5,
	A64DB_NSHST = 0x6,
	A64DB_NSH =   0x7,
	A64DB_ISHLD = 0x9,
	A64DB_ISHST = 0xa,
	A64DB_ISH =   0xb,
	A64DB_LD =    0xd,
	A64DB_ST =    0xe,
	A64DB_SY =    0xf
};

enum A64DCValues {
	A64DC_Invalid = -1,   // Op1  CRn   CRm   Op2
	A64DC_ZVA   = 0x5ba1, // 01  011  0111  0100  001
	A64DC_IVAC  = 0x43b1, // 01  000  0111  0110  001
	A64DC_ISW   = 0x43b2, // 01  000  0111  0110  010
	A64DC_CVAC  = 0x5bd1, // 01  011  0111  1010  001
	A64DC_CSW   = 0x43d2, // 01  000  0111  1010  010
	A64DC_CVAU  = 0x5bd9, // 01  011  0111  1011  001
	A64DC_CIVAC = 0x5bf1, // 01  011  0111  1110  001
	A64DC_CISW  = 0x43f2  // 01  000  0111  1110  010
};

enum A64ICValues {
	A64IC_Invalid = -1,     // Op1  CRn   CRm   Op2
	A64IC_IALLUIS = 0x0388, // 000  0111  0001  000
	A64IC_IALLU = 0x03a8,   // 000  0111  0101  000
	A64IC_IVAU = 0x1ba9     // 011  0111  0101  001
};

enum A64ISBValues {
	A64ISB_Invalid = -1,
	A64ISB_SY = 0xf
};

enum A64PRFMValues {
	A64PRFM_Invalid = -1,
	A64PRFM_PLDL1KEEP = 0x00,
	A64PRFM_PLDL1STRM = 0x01,
	A64PRFM_PLDL2KEEP = 0x02,
	A64PRFM_PLDL2STRM = 0x03,
	A64PRFM_PLDL3KEEP = 0x04,
	A64PRFM_PLDL3STRM = 0x05,
	A64PRFM_PLIL1KEEP = 0x08,
	A64PRFM_PLIL1STRM = 0x09,
	A64PRFM_PLIL2KEEP = 0x0a,
	A64PRFM_PLIL2STRM = 0x0b,
	A64PRFM_PLIL3KEEP = 0x0c,
	A64PRFM_PLIL3STRM = 0x0d,
	A64PRFM_PSTL1KEEP = 0x10,
	A64PRFM_PSTL1STRM = 0x11,
	A64PRFM_PSTL2KEEP = 0x12,
	A64PRFM_PSTL2STRM = 0x13,
	A64PRFM_PSTL3KEEP = 0x14,
	A64PRFM_PSTL3STRM = 0x15
};

enum A64PStateValues {
	A64PState_Invalid = -1,
	A64PState_SPSel = 0x05,
	A64PState_DAIFSet = 0x1e,
	A64PState_DAIFClr = 0x1f,
	A64PState_PAN	= 0x4,
	A64PState_UAO	= 0x3
};

typedef enum A64SE_ShiftExtSpecifiers {
	A64SE_Invalid = -1,
	A64SE_LSL,
	A64SE_MSL,
	A64SE_LSR,
	A64SE_ASR,
	A64SE_ROR,

	A64SE_UXTB,
	A64SE_UXTH,
	A64SE_UXTW,
	A64SE_UXTX,

	A64SE_SXTB,
	A64SE_SXTH,
	A64SE_SXTW,
	A64SE_SXTX
} A64SE_ShiftExtSpecifiers;

typedef enum A64Layout_VectorLayout {
	A64Layout_Invalid = -1,
	A64Layout_VL_8B,
	A64Layout_VL_4H,
	A64Layout_VL_2S,
	A64Layout_VL_1D,

	A64Layout_VL_16B,
	A64Layout_VL_8H,
	A64Layout_VL_4S,
	A64Layout_VL_2D,

	// Bare layout for the 128-bit vector
	// (only show ".b", ".h", ".s", ".d" without vector number)
	A64Layout_VL_B,
	A64Layout_VL_H,
	A64Layout_VL_S,
	A64Layout_VL_D
} A64Layout_VectorLayout;

inline static const char *A64VectorLayoutToString(A64Layout_VectorLayout Layout)
{
	switch (Layout) {
		case A64Layout_VL_8B:  return ".8b";
		case A64Layout_VL_4H:  return ".4h";
		case A64Layout_VL_2S:  return ".2s";
		case A64Layout_VL_1D:  return ".1d";
		case A64Layout_VL_16B:  return ".16b";
		case A64Layout_VL_8H:  return ".8h";
		case A64Layout_VL_4S:  return ".4s";
		case A64Layout_VL_2D:  return ".2d";
		case A64Layout_VL_B:  return ".b";
		case A64Layout_VL_H:  return ".h";
		case A64Layout_VL_S:  return ".s";
		case A64Layout_VL_D:  return ".d";
		default: return NULL;	// never reach
	}
}

enum A64SysRegROValues {
	A64SysReg_MDCCSR_EL0        = 0x9808, // 10  011  0000  0001  000
	A64SysReg_DBGDTRRX_EL0      = 0x9828, // 10  011  0000  0101  000
	A64SysReg_MDRAR_EL1         = 0x8080, // 10  000  0001  0000  000
	A64SysReg_OSLSR_EL1         = 0x808c, // 10  000  0001  0001  100
	A64SysReg_DBGAUTHSTATUS_EL1 = 0x83f6, // 10  000  0111  1110  110
	A64SysReg_PMCEID0_EL0       = 0xdce6, // 11  011  1001  1100  110
	A64SysReg_PMCEID1_EL0       = 0xdce7, // 11  011  1001  1100  111
	A64SysReg_MIDR_EL1          = 0xc000, // 11  000  0000  0000  000
	A64SysReg_CCSIDR_EL1        = 0xc800, // 11  001  0000  0000  000
	A64SysReg_CLIDR_EL1         = 0xc801, // 11  001  0000  0000  001
	A64SysReg_CTR_EL0           = 0xd801, // 11  011  0000  0000  001
	A64SysReg_MPIDR_EL1         = 0xc005, // 11  000  0000  0000  101
	A64SysReg_REVIDR_EL1        = 0xc006, // 11  000  0000  0000  110
	A64SysReg_AIDR_EL1          = 0xc807, // 11  001  0000  0000  111
	A64SysReg_DCZID_EL0         = 0xd807, // 11  011  0000  0000  111
	A64SysReg_ID_PFR0_EL1       = 0xc008, // 11  000  0000  0001  000
	A64SysReg_ID_PFR1_EL1       = 0xc009, // 11  000  0000  0001  001
	A64SysReg_ID_DFR0_EL1       = 0xc00a, // 11  000  0000  0001  010
	A64SysReg_ID_AFR0_EL1       = 0xc00b, // 11  000  0000  0001  011
	A64SysReg_ID_MMFR0_EL1      = 0xc00c, // 11  000  0000  0001  100
	A64SysReg_ID_MMFR1_EL1      = 0xc00d, // 11  000  0000  0001  101
	A64SysReg_ID_MMFR2_EL1      = 0xc00e, // 11  000  0000  0001  110
	A64SysReg_ID_MMFR3_EL1      = 0xc00f, // 11  000  0000  0001  111
	A64SysReg_ID_MMFR4_EL1      = 0xc016, // 11  000  0000  0010  110
	A64SysReg_ID_ISAR0_EL1      = 0xc010, // 11  000  0000  0010  000
	A64SysReg_ID_ISAR1_EL1      = 0xc011, // 11  000  0000  0010  001
	A64SysReg_ID_ISAR2_EL1      = 0xc012, // 11  000  0000  0010  010
	A64SysReg_ID_ISAR3_EL1      = 0xc013, // 11  000  0000  0010  011
	A64SysReg_ID_ISAR4_EL1      = 0xc014, // 11  000  0000  0010  100
	A64SysReg_ID_ISAR5_EL1      = 0xc015, // 11  000  0000  0010  101
	A64SysReg_ID_A64PFR0_EL1   = 0xc020, // 11  000  0000  0100  000
	A64SysReg_ID_A64PFR1_EL1   = 0xc021, // 11  000  0000  0100  001
	A64SysReg_ID_A64DFR0_EL1   = 0xc028, // 11  000  0000  0101  000
	A64SysReg_ID_A64DFR1_EL1   = 0xc029, // 11  000  0000  0101  001
	A64SysReg_ID_A64AFR0_EL1   = 0xc02c, // 11  000  0000  0101  100
	A64SysReg_ID_A64AFR1_EL1   = 0xc02d, // 11  000  0000  0101  101
	A64SysReg_ID_A64ISAR0_EL1  = 0xc030, // 11  000  0000  0110  000
	A64SysReg_ID_A64ISAR1_EL1  = 0xc031, // 11  000  0000  0110  001
	A64SysReg_ID_A64MMFR0_EL1  = 0xc038, // 11  000  0000  0111  000
	A64SysReg_ID_A64MMFR1_EL1  = 0xc039, // 11  000  0000  0111  001
	A64SysReg_ID_A64MMFR2_EL1  = 0xC03A, // 11  000  0000  0111  010
  	A64SysReg_LORC_EL1         = 0xc523, // 11  000  1010  0100  011
  	A64SysReg_LOREA_EL1        = 0xc521, // 11  000  1010  0100  001
  	A64SysReg_LORID_EL1        = 0xc527, // 11  000  1010  0100  111
  	A64SysReg_LORN_EL1         = 0xc522, // 11  000  1010  0100  010
  	A64SysReg_LORSA_EL1        = 0xc520, // 11  000  1010  0100  000
	A64SysReg_MVFR0_EL1         = 0xc018, // 11  000  0000  0011  000
	A64SysReg_MVFR1_EL1         = 0xc019, // 11  000  0000  0011  001
	A64SysReg_MVFR2_EL1         = 0xc01a, // 11  000  0000  0011  010
	A64SysReg_RVBAR_EL1         = 0xc601, // 11  000  1100  0000  001
	A64SysReg_RVBAR_EL2         = 0xe601, // 11  100  1100  0000  001
	A64SysReg_RVBAR_EL3         = 0xf601, // 11  110  1100  0000  001
	A64SysReg_ISR_EL1           = 0xc608, // 11  000  1100  0001  000
	A64SysReg_CNTPCT_EL0        = 0xdf01, // 11  011  1110  0000  001
	A64SysReg_CNTVCT_EL0        = 0xdf02,  // 11  011  1110  0000  010

	// Trace registers
	A64SysReg_TRCSTATR          = 0x8818, // 10  001  0000  0011  000
	A64SysReg_TRCIDR8           = 0x8806, // 10  001  0000  0000  110
	A64SysReg_TRCIDR9           = 0x880e, // 10  001  0000  0001  110
	A64SysReg_TRCIDR10          = 0x8816, // 10  001  0000  0010  110
	A64SysReg_TRCIDR11          = 0x881e, // 10  001  0000  0011  110
	A64SysReg_TRCIDR12          = 0x8826, // 10  001  0000  0100  110
	A64SysReg_TRCIDR13          = 0x882e, // 10  001  0000  0101  110
	A64SysReg_TRCIDR0           = 0x8847, // 10  001  0000  1000  111
	A64SysReg_TRCIDR1           = 0x884f, // 10  001  0000  1001  111
	A64SysReg_TRCIDR2           = 0x8857, // 10  001  0000  1010  111
	A64SysReg_TRCIDR3           = 0x885f, // 10  001  0000  1011  111
	A64SysReg_TRCIDR4           = 0x8867, // 10  001  0000  1100  111
	A64SysReg_TRCIDR5           = 0x886f, // 10  001  0000  1101  111
	A64SysReg_TRCIDR6           = 0x8877, // 10  001  0000  1110  111
	A64SysReg_TRCIDR7           = 0x887f, // 10  001  0000  1111  111
	A64SysReg_TRCOSLSR          = 0x888c, // 10  001  0001  0001  100
	A64SysReg_TRCPDSR           = 0x88ac, // 10  001  0001  0101  100
	A64SysReg_TRCDEVAFF0        = 0x8bd6, // 10  001  0111  1010  110
	A64SysReg_TRCDEVAFF1        = 0x8bde, // 10  001  0111  1011  110
	A64SysReg_TRCLSR            = 0x8bee, // 10  001  0111  1101  110
	A64SysReg_TRCAUTHSTATUS     = 0x8bf6, // 10  001  0111  1110  110
	A64SysReg_TRCDEVARCH        = 0x8bfe, // 10  001  0111  1111  110
	A64SysReg_TRCDEVID          = 0x8b97, // 10  001  0111  0010  111
	A64SysReg_TRCDEVTYPE        = 0x8b9f, // 10  001  0111  0011  111
	A64SysReg_TRCPIDR4          = 0x8ba7, // 10  001  0111  0100  111
	A64SysReg_TRCPIDR5          = 0x8baf, // 10  001  0111  0101  111
	A64SysReg_TRCPIDR6          = 0x8bb7, // 10  001  0111  0110  111
	A64SysReg_TRCPIDR7          = 0x8bbf, // 10  001  0111  0111  111
	A64SysReg_TRCPIDR0          = 0x8bc7, // 10  001  0111  1000  111
	A64SysReg_TRCPIDR1          = 0x8bcf, // 10  001  0111  1001  111
	A64SysReg_TRCPIDR2          = 0x8bd7, // 10  001  0111  1010  111
	A64SysReg_TRCPIDR3          = 0x8bdf, // 10  001  0111  1011  111
	A64SysReg_TRCCIDR0          = 0x8be7, // 10  001  0111  1100  111
	A64SysReg_TRCCIDR1          = 0x8bef, // 10  001  0111  1101  111
	A64SysReg_TRCCIDR2          = 0x8bf7, // 10  001  0111  1110  111
	A64SysReg_TRCCIDR3          = 0x8bff, // 10  001  0111  1111  111

	// GICv3 registers
	A64SysReg_ICC_IAR1_EL1      = 0xc660, // 11  000  1100  1100  000
	A64SysReg_ICC_IAR0_EL1      = 0xc640, // 11  000  1100  1000  000
	A64SysReg_ICC_HPPIR1_EL1    = 0xc662, // 11  000  1100  1100  010
	A64SysReg_ICC_HPPIR0_EL1    = 0xc642, // 11  000  1100  1000  010
	A64SysReg_ICC_RPR_EL1       = 0xc65b, // 11  000  1100  1011  011
	A64SysReg_ICH_VTR_EL2       = 0xe659, // 11  100  1100  1011  001
	A64SysReg_ICH_EISR_EL2      = 0xe65b, // 11  100  1100  1011  011
	A64SysReg_ICH_ELSR_EL2      = 0xe65d  // 11  100  1100  1011  101
};

enum A64SysRegWOValues {
	A64SysReg_DBGDTRTX_EL0      = 0x9828, // 10  011  0000  0101  000
	A64SysReg_OSLAR_EL1         = 0x8084, // 10  000  0001  0000  100
	A64SysReg_PMSWINC_EL0       = 0xdce4,  // 11  011  1001  1100  100

	// Trace Registers
	A64SysReg_TRCOSLAR          = 0x8884, // 10  001  0001  0000  100
	A64SysReg_TRCLAR            = 0x8be6, // 10  001  0111  1100  110

	// GICv3 registers
	A64SysReg_ICC_EOIR1_EL1     = 0xc661, // 11  000  1100  1100  001
	A64SysReg_ICC_EOIR0_EL1     = 0xc641, // 11  000  1100  1000  001
	A64SysReg_ICC_DIR_EL1       = 0xc659, // 11  000  1100  1011  001
	A64SysReg_ICC_SGI1R_EL1     = 0xc65d, // 11  000  1100  1011  101
	A64SysReg_ICC_ASGI1R_EL1    = 0xc65e, // 11  000  1100  1011  110
	A64SysReg_ICC_SGI0R_EL1     = 0xc65f  // 11  000  1100  1011  111
};

enum A64SysRegValues {
	A64SysReg_Invalid = -1,               // Op0 Op1  CRn   CRm   Op2
	A64SysReg_PAN               = 0xc213, // 11  000  0100  0010  011
	A64SysReg_UAO               = 0xc214, // 11  000  0100  0010  100
	A64SysReg_OSDTRRX_EL1       = 0x8002, // 10  000  0000  0000  010
	A64SysReg_OSDTRTX_EL1       = 0x801a, // 10  000  0000  0011  010
	A64SysReg_TEECR32_EL1       = 0x9000, // 10  010  0000  0000  000
	A64SysReg_MDCCINT_EL1       = 0x8010, // 10  000  0000  0010  000
	A64SysReg_MDSCR_EL1         = 0x8012, // 10  000  0000  0010  010
	A64SysReg_DBGDTR_EL0        = 0x9820, // 10  011  0000  0100  000
	A64SysReg_OSECCR_EL1        = 0x8032, // 10  000  0000  0110  010
	A64SysReg_DBGVCR32_EL2      = 0xa038, // 10  100  0000  0111  000
	A64SysReg_DBGBVR0_EL1       = 0x8004, // 10  000  0000  0000  100
	A64SysReg_DBGBVR1_EL1       = 0x800c, // 10  000  0000  0001  100
	A64SysReg_DBGBVR2_EL1       = 0x8014, // 10  000  0000  0010  100
	A64SysReg_DBGBVR3_EL1       = 0x801c, // 10  000  0000  0011  100
	A64SysReg_DBGBVR4_EL1       = 0x8024, // 10  000  0000  0100  100
	A64SysReg_DBGBVR5_EL1       = 0x802c, // 10  000  0000  0101  100
	A64SysReg_DBGBVR6_EL1       = 0x8034, // 10  000  0000  0110  100
	A64SysReg_DBGBVR7_EL1       = 0x803c, // 10  000  0000  0111  100
	A64SysReg_DBGBVR8_EL1       = 0x8044, // 10  000  0000  1000  100
	A64SysReg_DBGBVR9_EL1       = 0x804c, // 10  000  0000  1001  100
	A64SysReg_DBGBVR10_EL1      = 0x8054, // 10  000  0000  1010  100
	A64SysReg_DBGBVR11_EL1      = 0x805c, // 10  000  0000  1011  100
	A64SysReg_DBGBVR12_EL1      = 0x8064, // 10  000  0000  1100  100
	A64SysReg_DBGBVR13_EL1      = 0x806c, // 10  000  0000  1101  100
	A64SysReg_DBGBVR14_EL1      = 0x8074, // 10  000  0000  1110  100
	A64SysReg_DBGBVR15_EL1      = 0x807c, // 10  000  0000  1111  100
	A64SysReg_DBGBCR0_EL1       = 0x8005, // 10  000  0000  0000  101
	A64SysReg_DBGBCR1_EL1       = 0x800d, // 10  000  0000  0001  101
	A64SysReg_DBGBCR2_EL1       = 0x8015, // 10  000  0000  0010  101
	A64SysReg_DBGBCR3_EL1       = 0x801d, // 10  000  0000  0011  101
	A64SysReg_DBGBCR4_EL1       = 0x8025, // 10  000  0000  0100  101
	A64SysReg_DBGBCR5_EL1       = 0x802d, // 10  000  0000  0101  101
	A64SysReg_DBGBCR6_EL1       = 0x8035, // 10  000  0000  0110  101
	A64SysReg_DBGBCR7_EL1       = 0x803d, // 10  000  0000  0111  101
	A64SysReg_DBGBCR8_EL1       = 0x8045, // 10  000  0000  1000  101
	A64SysReg_DBGBCR9_EL1       = 0x804d, // 10  000  0000  1001  101
	A64SysReg_DBGBCR10_EL1      = 0x8055, // 10  000  0000  1010  101
	A64SysReg_DBGBCR11_EL1      = 0x805d, // 10  000  0000  1011  101
	A64SysReg_DBGBCR12_EL1      = 0x8065, // 10  000  0000  1100  101
	A64SysReg_DBGBCR13_EL1      = 0x806d, // 10  000  0000  1101  101
	A64SysReg_DBGBCR14_EL1      = 0x8075, // 10  000  0000  1110  101
	A64SysReg_DBGBCR15_EL1      = 0x807d, // 10  000  0000  1111  101
	A64SysReg_DBGWVR0_EL1       = 0x8006, // 10  000  0000  0000  110
	A64SysReg_DBGWVR1_EL1       = 0x800e, // 10  000  0000  0001  110
	A64SysReg_DBGWVR2_EL1       = 0x8016, // 10  000  0000  0010  110
	A64SysReg_DBGWVR3_EL1       = 0x801e, // 10  000  0000  0011  110
	A64SysReg_DBGWVR4_EL1       = 0x8026, // 10  000  0000  0100  110
	A64SysReg_DBGWVR5_EL1       = 0x802e, // 10  000  0000  0101  110
	A64SysReg_DBGWVR6_EL1       = 0x8036, // 10  000  0000  0110  110
	A64SysReg_DBGWVR7_EL1       = 0x803e, // 10  000  0000  0111  110
	A64SysReg_DBGWVR8_EL1       = 0x8046, // 10  000  0000  1000  110
	A64SysReg_DBGWVR9_EL1       = 0x804e, // 10  000  0000  1001  110
	A64SysReg_DBGWVR10_EL1      = 0x8056, // 10  000  0000  1010  110
	A64SysReg_DBGWVR11_EL1      = 0x805e, // 10  000  0000  1011  110
	A64SysReg_DBGWVR12_EL1      = 0x8066, // 10  000  0000  1100  110
	A64SysReg_DBGWVR13_EL1      = 0x806e, // 10  000  0000  1101  110
	A64SysReg_DBGWVR14_EL1      = 0x8076, // 10  000  0000  1110  110
	A64SysReg_DBGWVR15_EL1      = 0x807e, // 10  000  0000  1111  110
	A64SysReg_DBGWCR0_EL1       = 0x8007, // 10  000  0000  0000  111
	A64SysReg_DBGWCR1_EL1       = 0x800f, // 10  000  0000  0001  111
	A64SysReg_DBGWCR2_EL1       = 0x8017, // 10  000  0000  0010  111
	A64SysReg_DBGWCR3_EL1       = 0x801f, // 10  000  0000  0011  111
	A64SysReg_DBGWCR4_EL1       = 0x8027, // 10  000  0000  0100  111
	A64SysReg_DBGWCR5_EL1       = 0x802f, // 10  000  0000  0101  111
	A64SysReg_DBGWCR6_EL1       = 0x8037, // 10  000  0000  0110  111
	A64SysReg_DBGWCR7_EL1       = 0x803f, // 10  000  0000  0111  111
	A64SysReg_DBGWCR8_EL1       = 0x8047, // 10  000  0000  1000  111
	A64SysReg_DBGWCR9_EL1       = 0x804f, // 10  000  0000  1001  111
	A64SysReg_DBGWCR10_EL1      = 0x8057, // 10  000  0000  1010  111
	A64SysReg_DBGWCR11_EL1      = 0x805f, // 10  000  0000  1011  111
	A64SysReg_DBGWCR12_EL1      = 0x8067, // 10  000  0000  1100  111
	A64SysReg_DBGWCR13_EL1      = 0x806f, // 10  000  0000  1101  111
	A64SysReg_DBGWCR14_EL1      = 0x8077, // 10  000  0000  1110  111
	A64SysReg_DBGWCR15_EL1      = 0x807f, // 10  000  0000  1111  111
	A64SysReg_TEEHBR32_EL1      = 0x9080, // 10  010  0001  0000  000
	A64SysReg_OSDLR_EL1         = 0x809c, // 10  000  0001  0011  100
	A64SysReg_DBGPRCR_EL1       = 0x80a4, // 10  000  0001  0100  100
	A64SysReg_DBGCLAIMSET_EL1   = 0x83c6, // 10  000  0111  1000  110
	A64SysReg_DBGCLAIMCLR_EL1   = 0x83ce, // 10  000  0111  1001  110
	A64SysReg_CSSELR_EL1        = 0xd000, // 11  010  0000  0000  000
	A64SysReg_VPIDR_EL2         = 0xe000, // 11  100  0000  0000  000
	A64SysReg_VMPIDR_EL2        = 0xe005, // 11  100  0000  0000  101
	A64SysReg_CPACR_EL1         = 0xc082, // 11  000  0001  0000  010
  	A64SysReg_CPACR_EL12        = 0xe882, // 11  101  0001  0000  010
	A64SysReg_SCTLR_EL1         = 0xc080, // 11  000  0001  0000  000
  	A64SysReg_SCTLR_EL12        = 0xe880, // 11  101  0001  0000  000
	A64SysReg_SCTLR_EL2         = 0xe080, // 11  100  0001  0000  000
	A64SysReg_SCTLR_EL3         = 0xf080, // 11  110  0001  0000  000
	A64SysReg_ACTLR_EL1         = 0xc081, // 11  000  0001  0000  001
	A64SysReg_ACTLR_EL2         = 0xe081, // 11  100  0001  0000  001
	A64SysReg_ACTLR_EL3         = 0xf081, // 11  110  0001  0000  001
	A64SysReg_HCR_EL2           = 0xe088, // 11  100  0001  0001  000
	A64SysReg_SCR_EL3           = 0xf088, // 11  110  0001  0001  000
	A64SysReg_MDCR_EL2          = 0xe089, // 11  100  0001  0001  001
	A64SysReg_SDER32_EL3        = 0xf089, // 11  110  0001  0001  001
	A64SysReg_CPTR_EL2          = 0xe08a, // 11  100  0001  0001  010
	A64SysReg_CPTR_EL3          = 0xf08a, // 11  110  0001  0001  010
	A64SysReg_HSTR_EL2          = 0xe08b, // 11  100  0001  0001  011
	A64SysReg_HACR_EL2          = 0xe08f, // 11  100  0001  0001  111
	A64SysReg_MDCR_EL3          = 0xf099, // 11  110  0001  0011  001
	A64SysReg_TTBR0_EL1         = 0xc100, // 11  000  0010  0000  000
  	A64SysReg_TTBR0_EL12        = 0xe900, // 11  101  0010  0000  000
	A64SysReg_TTBR0_EL2         = 0xe100, // 11  100  0010  0000  000
	A64SysReg_TTBR0_EL3         = 0xf100, // 11  110  0010  0000  000
	A64SysReg_TTBR1_EL1         = 0xc101, // 11  000  0010  0000  001
  	A64SysReg_TTBR1_EL12        = 0xe901, // 11  101  0010  0000  001
  	A64SysReg_TTBR1_EL2         = 0xe101, // 11  100  0010  0000  001
	A64SysReg_TCR_EL1           = 0xc102, // 11  000  0010  0000  010
  	A64SysReg_TCR_EL12          = 0xe902, // 11  101  0010  0000  010
	A64SysReg_TCR_EL2           = 0xe102, // 11  100  0010  0000  010
	A64SysReg_TCR_EL3           = 0xf102, // 11  110  0010  0000  010
	A64SysReg_VTTBR_EL2         = 0xe108, // 11  100  0010  0001  000
	A64SysReg_VTCR_EL2          = 0xe10a, // 11  100  0010  0001  010
	A64SysReg_DACR32_EL2        = 0xe180, // 11  100  0011  0000  000
	A64SysReg_SPSR_EL1          = 0xc200, // 11  000  0100  0000  000
  	A64SysReg_SPSR_EL12         = 0xea00, // 11  101  0100  0000  000
	A64SysReg_SPSR_EL2          = 0xe200, // 11  100  0100  0000  000
	A64SysReg_SPSR_EL3          = 0xf200, // 11  110  0100  0000  000
	A64SysReg_ELR_EL1           = 0xc201, // 11  000  0100  0000  001
  	A64SysReg_ELR_EL12          = 0xea01, // 11  101  0100  0000  001
	A64SysReg_ELR_EL2           = 0xe201, // 11  100  0100  0000  001
	A64SysReg_ELR_EL3           = 0xf201, // 11  110  0100  0000  001
	A64SysReg_SP_EL0            = 0xc208, // 11  000  0100  0001  000
	A64SysReg_SP_EL1            = 0xe208, // 11  100  0100  0001  000
	A64SysReg_SP_EL2            = 0xf208, // 11  110  0100  0001  000
	A64SysReg_SPSel             = 0xc210, // 11  000  0100  0010  000
	A64SysReg_NZCV              = 0xda10, // 11  011  0100  0010  000
	A64SysReg_DAIF              = 0xda11, // 11  011  0100  0010  001
	A64SysReg_CurrentEL         = 0xc212, // 11  000  0100  0010  010
	A64SysReg_SPSR_irq          = 0xe218, // 11  100  0100  0011  000
	A64SysReg_SPSR_abt          = 0xe219, // 11  100  0100  0011  001
	A64SysReg_SPSR_und          = 0xe21a, // 11  100  0100  0011  010
	A64SysReg_SPSR_fiq          = 0xe21b, // 11  100  0100  0011  011
	A64SysReg_FPCR              = 0xda20, // 11  011  0100  0100  000
	A64SysReg_FPSR              = 0xda21, // 11  011  0100  0100  001
	A64SysReg_DSPSR_EL0         = 0xda28, // 11  011  0100  0101  000
	A64SysReg_DLR_EL0           = 0xda29, // 11  011  0100  0101  001
	A64SysReg_IFSR32_EL2        = 0xe281, // 11  100  0101  0000  001
	A64SysReg_AFSR0_EL1         = 0xc288, // 11  000  0101  0001  000
  	A64SysReg_AFSR0_EL12        = 0xea88, // 11  101  0101  0001  000
	A64SysReg_AFSR0_EL2         = 0xe288, // 11  100  0101  0001  000
	A64SysReg_AFSR0_EL3         = 0xf288, // 11  110  0101  0001  000
	A64SysReg_AFSR1_EL1         = 0xc289, // 11  000  0101  0001  001
 	A64SysReg_AFSR1_EL12        = 0xea89, // 11  101  0101  0001  001
	A64SysReg_AFSR1_EL2         = 0xe289, // 11  100  0101  0001  001
	A64SysReg_AFSR1_EL3         = 0xf289, // 11  110  0101  0001  001
	A64SysReg_ESR_EL1           = 0xc290, // 11  000  0101  0010  000
	A64SysReg_ESR_EL12          = 0xea90, // 11  101  0101  0010  000
	A64SysReg_ESR_EL2           = 0xe290, // 11  100  0101  0010  000
	A64SysReg_ESR_EL3           = 0xf290, // 11  110  0101  0010  000
	A64SysReg_FPEXC32_EL2       = 0xe298, // 11  100  0101  0011  000
	A64SysReg_FAR_EL1           = 0xc300, // 11  000  0110  0000  000
	A64SysReg_FAR_EL12          = 0xeb00, // 11  101  0110  0000  000
	A64SysReg_FAR_EL2           = 0xe300, // 11  100  0110  0000  000
	A64SysReg_FAR_EL3           = 0xf300, // 11  110  0110  0000  000
	A64SysReg_HPFAR_EL2         = 0xe304, // 11  100  0110  0000  100
	A64SysReg_PAR_EL1           = 0xc3a0, // 11  000  0111  0100  000
	A64SysReg_PMCR_EL0          = 0xdce0, // 11  011  1001  1100  000
	A64SysReg_PMCNTENSET_EL0    = 0xdce1, // 11  011  1001  1100  001
	A64SysReg_PMCNTENCLR_EL0    = 0xdce2, // 11  011  1001  1100  010
	A64SysReg_PMOVSCLR_EL0      = 0xdce3, // 11  011  1001  1100  011
	A64SysReg_PMSELR_EL0        = 0xdce5, // 11  011  1001  1100  101
	A64SysReg_PMCCNTR_EL0       = 0xdce8, // 11  011  1001  1101  000
	A64SysReg_PMXEVTYPER_EL0    = 0xdce9, // 11  011  1001  1101  001
	A64SysReg_PMXEVCNTR_EL0     = 0xdcea, // 11  011  1001  1101  010
	A64SysReg_PMUSERENR_EL0     = 0xdcf0, // 11  011  1001  1110  000
	A64SysReg_PMINTENSET_EL1    = 0xc4f1, // 11  000  1001  1110  001
	A64SysReg_PMINTENCLR_EL1    = 0xc4f2, // 11  000  1001  1110  010
	A64SysReg_PMOVSSET_EL0      = 0xdcf3, // 11  011  1001  1110  011
	A64SysReg_MAIR_EL1          = 0xc510, // 11  000  1010  0010  000
	A64SysReg_MAIR_EL12         = 0xed10, // 11  101  1010  0010  000
	A64SysReg_MAIR_EL2          = 0xe510, // 11  100  1010  0010  000
	A64SysReg_MAIR_EL3          = 0xf510, // 11  110  1010  0010  000
	A64SysReg_AMAIR_EL1         = 0xc518, // 11  000  1010  0011  000
	A64SysReg_AMAIR_EL12        = 0xed18, // 11  101  1010  0011  000
	A64SysReg_AMAIR_EL2         = 0xe518, // 11  100  1010  0011  000
	A64SysReg_AMAIR_EL3         = 0xf518, // 11  110  1010  0011  000
	A64SysReg_VBAR_EL1          = 0xc600, // 11  000  1100  0000  000
	A64SysReg_VBAR_EL12         = 0xee00, // 11  101  1100  0000  000
	A64SysReg_VBAR_EL2          = 0xe600, // 11  100  1100  0000  000
	A64SysReg_VBAR_EL3          = 0xf600, // 11  110  1100  0000  000
	A64SysReg_RMR_EL1           = 0xc602, // 11  000  1100  0000  010
	A64SysReg_RMR_EL2           = 0xe602, // 11  100  1100  0000  010
	A64SysReg_RMR_EL3           = 0xf602, // 11  110  1100  0000  010
	A64SysReg_CONTEXTIDR_EL1    = 0xc681, // 11  000  1101  0000  001
	A64SysReg_CONTEXTIDR_EL12   = 0xee81, // 11  101  1101  0000  001
	A64SysReg_CONTEXTIDR_EL2    = 0xe681, // 11  100  1101  0000  001
	A64SysReg_TPIDR_EL0         = 0xde82, // 11  011  1101  0000  010
	A64SysReg_TPIDR_EL2         = 0xe682, // 11  100  1101  0000  010
	A64SysReg_TPIDR_EL3         = 0xf682, // 11  110  1101  0000  010
	A64SysReg_TPIDRRO_EL0       = 0xde83, // 11  011  1101  0000  011
	A64SysReg_TPIDR_EL1         = 0xc684, // 11  000  1101  0000  100
	A64SysReg_CNTFRQ_EL0        = 0xdf00, // 11  011  1110  0000  000
	A64SysReg_CNTVOFF_EL2       = 0xe703, // 11  100  1110  0000  011
	A64SysReg_CNTKCTL_EL1       = 0xc708, // 11  000  1110  0001  000
	A64SysReg_CNTKCTL_EL12      = 0xef08, // 11  101  1110  0001  000
	A64SysReg_CNTHCTL_EL2       = 0xe708, // 11  100  1110  0001  000
	A64SysReg_CNTHVCTL_EL2      = 0xe719, // 11  100  1110  0011  001
	A64SysReg_CNTHV_CVAL_EL2    = 0xe71a, // 11  100  1110  0011  010
	A64SysReg_CNTHV_TVAL_EL2    = 0xe718, // 11  100  1110  0011  000
	A64SysReg_CNTP_TVAL_EL0     = 0xdf10, // 11  011  1110  0010  000
	A64SysReg_CNTP_TVAL_EL02    = 0xef10, // 11  101  1110  0010  000
	A64SysReg_CNTHP_TVAL_EL2    = 0xe710, // 11  100  1110  0010  000
	A64SysReg_CNTPS_TVAL_EL1    = 0xff10, // 11  111  1110  0010  000
	A64SysReg_CNTP_CTL_EL0      = 0xdf11, // 11  011  1110  0010  001
	A64SysReg_CNTHP_CTL_EL2     = 0xe711, // 11  100  1110  0010  001
	A64SysReg_CNTPS_CTL_EL1     = 0xff11, // 11  111  1110  0010  001
	A64SysReg_CNTP_CVAL_EL0     = 0xdf12, // 11  011  1110  0010  010
	A64SysReg_CNTP_CVAL_EL02    = 0xef12, // 11  101  1110  0010  010
	A64SysReg_CNTHP_CVAL_EL2    = 0xe712, // 11  100  1110  0010  010
	A64SysReg_CNTPS_CVAL_EL1    = 0xff12, // 11  111  1110  0010  010
	A64SysReg_CNTV_TVAL_EL0     = 0xdf18, // 11  011  1110  0011  000
	A64SysReg_CNTV_TVAL_EL02    = 0xef18, // 11  101  1110  0011  000
	A64SysReg_CNTV_CTL_EL0      = 0xdf19, // 11  011  1110  0011  001
	A64SysReg_CNTV_CTL_EL02	    = 0xef19, // 11  101  1110  0011  001
	A64SysReg_CNTV_CVAL_EL0     = 0xdf1a, // 11  011  1110  0011  010
	A64SysReg_CNTV_CVAL_EL02    = 0xef1a, // 11  101  1110  0011  010
	A64SysReg_PMEVCNTR0_EL0     = 0xdf40, // 11  011  1110  1000  000
	A64SysReg_PMEVCNTR1_EL0     = 0xdf41, // 11  011  1110  1000  001
	A64SysReg_PMEVCNTR2_EL0     = 0xdf42, // 11  011  1110  1000  010
	A64SysReg_PMEVCNTR3_EL0     = 0xdf43, // 11  011  1110  1000  011
	A64SysReg_PMEVCNTR4_EL0     = 0xdf44, // 11  011  1110  1000  100
	A64SysReg_PMEVCNTR5_EL0     = 0xdf45, // 11  011  1110  1000  101
	A64SysReg_PMEVCNTR6_EL0     = 0xdf46, // 11  011  1110  1000  110
	A64SysReg_PMEVCNTR7_EL0     = 0xdf47, // 11  011  1110  1000  111
	A64SysReg_PMEVCNTR8_EL0     = 0xdf48, // 11  011  1110  1001  000
	A64SysReg_PMEVCNTR9_EL0     = 0xdf49, // 11  011  1110  1001  001
	A64SysReg_PMEVCNTR10_EL0    = 0xdf4a, // 11  011  1110  1001  010
	A64SysReg_PMEVCNTR11_EL0    = 0xdf4b, // 11  011  1110  1001  011
	A64SysReg_PMEVCNTR12_EL0    = 0xdf4c, // 11  011  1110  1001  100
	A64SysReg_PMEVCNTR13_EL0    = 0xdf4d, // 11  011  1110  1001  101
	A64SysReg_PMEVCNTR14_EL0    = 0xdf4e, // 11  011  1110  1001  110
	A64SysReg_PMEVCNTR15_EL0    = 0xdf4f, // 11  011  1110  1001  111
	A64SysReg_PMEVCNTR16_EL0    = 0xdf50, // 11  011  1110  1010  000
	A64SysReg_PMEVCNTR17_EL0    = 0xdf51, // 11  011  1110  1010  001
	A64SysReg_PMEVCNTR18_EL0    = 0xdf52, // 11  011  1110  1010  010
	A64SysReg_PMEVCNTR19_EL0    = 0xdf53, // 11  011  1110  1010  011
	A64SysReg_PMEVCNTR20_EL0    = 0xdf54, // 11  011  1110  1010  100
	A64SysReg_PMEVCNTR21_EL0    = 0xdf55, // 11  011  1110  1010  101
	A64SysReg_PMEVCNTR22_EL0    = 0xdf56, // 11  011  1110  1010  110
	A64SysReg_PMEVCNTR23_EL0    = 0xdf57, // 11  011  1110  1010  111
	A64SysReg_PMEVCNTR24_EL0    = 0xdf58, // 11  011  1110  1011  000
	A64SysReg_PMEVCNTR25_EL0    = 0xdf59, // 11  011  1110  1011  001
	A64SysReg_PMEVCNTR26_EL0    = 0xdf5a, // 11  011  1110  1011  010
	A64SysReg_PMEVCNTR27_EL0    = 0xdf5b, // 11  011  1110  1011  011
	A64SysReg_PMEVCNTR28_EL0    = 0xdf5c, // 11  011  1110  1011  100
	A64SysReg_PMEVCNTR29_EL0    = 0xdf5d, // 11  011  1110  1011  101
	A64SysReg_PMEVCNTR30_EL0    = 0xdf5e, // 11  011  1110  1011  110
	A64SysReg_PMCCFILTR_EL0     = 0xdf7f, // 11  011  1110  1111  111
	A64SysReg_PMEVTYPER0_EL0    = 0xdf60, // 11  011  1110  1100  000
	A64SysReg_PMEVTYPER1_EL0    = 0xdf61, // 11  011  1110  1100  001
	A64SysReg_PMEVTYPER2_EL0    = 0xdf62, // 11  011  1110  1100  010
	A64SysReg_PMEVTYPER3_EL0    = 0xdf63, // 11  011  1110  1100  011
	A64SysReg_PMEVTYPER4_EL0    = 0xdf64, // 11  011  1110  1100  100
	A64SysReg_PMEVTYPER5_EL0    = 0xdf65, // 11  011  1110  1100  101
	A64SysReg_PMEVTYPER6_EL0    = 0xdf66, // 11  011  1110  1100  110
	A64SysReg_PMEVTYPER7_EL0    = 0xdf67, // 11  011  1110  1100  111
	A64SysReg_PMEVTYPER8_EL0    = 0xdf68, // 11  011  1110  1101  000
	A64SysReg_PMEVTYPER9_EL0    = 0xdf69, // 11  011  1110  1101  001
	A64SysReg_PMEVTYPER10_EL0   = 0xdf6a, // 11  011  1110  1101  010
	A64SysReg_PMEVTYPER11_EL0   = 0xdf6b, // 11  011  1110  1101  011
	A64SysReg_PMEVTYPER12_EL0   = 0xdf6c, // 11  011  1110  1101  100
	A64SysReg_PMEVTYPER13_EL0   = 0xdf6d, // 11  011  1110  1101  101
	A64SysReg_PMEVTYPER14_EL0   = 0xdf6e, // 11  011  1110  1101  110
	A64SysReg_PMEVTYPER15_EL0   = 0xdf6f, // 11  011  1110  1101  111
	A64SysReg_PMEVTYPER16_EL0   = 0xdf70, // 11  011  1110  1110  000
	A64SysReg_PMEVTYPER17_EL0   = 0xdf71, // 11  011  1110  1110  001
	A64SysReg_PMEVTYPER18_EL0   = 0xdf72, // 11  011  1110  1110  010
	A64SysReg_PMEVTYPER19_EL0   = 0xdf73, // 11  011  1110  1110  011
	A64SysReg_PMEVTYPER20_EL0   = 0xdf74, // 11  011  1110  1110  100
	A64SysReg_PMEVTYPER21_EL0   = 0xdf75, // 11  011  1110  1110  101
	A64SysReg_PMEVTYPER22_EL0   = 0xdf76, // 11  011  1110  1110  110
	A64SysReg_PMEVTYPER23_EL0   = 0xdf77, // 11  011  1110  1110  111
	A64SysReg_PMEVTYPER24_EL0   = 0xdf78, // 11  011  1110  1111  000
	A64SysReg_PMEVTYPER25_EL0   = 0xdf79, // 11  011  1110  1111  001
	A64SysReg_PMEVTYPER26_EL0   = 0xdf7a, // 11  011  1110  1111  010
	A64SysReg_PMEVTYPER27_EL0   = 0xdf7b, // 11  011  1110  1111  011
	A64SysReg_PMEVTYPER28_EL0   = 0xdf7c, // 11  011  1110  1111  100
	A64SysReg_PMEVTYPER29_EL0   = 0xdf7d, // 11  011  1110  1111  101
	A64SysReg_PMEVTYPER30_EL0   = 0xdf7e, // 11  011  1110  1111  110

	// Trace registers
	A64SysReg_TRCPRGCTLR        = 0x8808, // 10  001  0000  0001  000
	A64SysReg_TRCPROCSELR       = 0x8810, // 10  001  0000  0010  000
	A64SysReg_TRCCONFIGR        = 0x8820, // 10  001  0000  0100  000
	A64SysReg_TRCAUXCTLR        = 0x8830, // 10  001  0000  0110  000
	A64SysReg_TRCEVENTCTL0R     = 0x8840, // 10  001  0000  1000  000
	A64SysReg_TRCEVENTCTL1R     = 0x8848, // 10  001  0000  1001  000
	A64SysReg_TRCSTALLCTLR      = 0x8858, // 10  001  0000  1011  000
	A64SysReg_TRCTSCTLR         = 0x8860, // 10  001  0000  1100  000
	A64SysReg_TRCSYNCPR         = 0x8868, // 10  001  0000  1101  000
	A64SysReg_TRCCCCTLR         = 0x8870, // 10  001  0000  1110  000
	A64SysReg_TRCBBCTLR         = 0x8878, // 10  001  0000  1111  000
	A64SysReg_TRCTRACEIDR       = 0x8801, // 10  001  0000  0000  001
	A64SysReg_TRCQCTLR          = 0x8809, // 10  001  0000  0001  001
	A64SysReg_TRCVICTLR         = 0x8802, // 10  001  0000  0000  010
	A64SysReg_TRCVIIECTLR       = 0x880a, // 10  001  0000  0001  010
	A64SysReg_TRCVISSCTLR       = 0x8812, // 10  001  0000  0010  010
	A64SysReg_TRCVIPCSSCTLR     = 0x881a, // 10  001  0000  0011  010
	A64SysReg_TRCVDCTLR         = 0x8842, // 10  001  0000  1000  010
	A64SysReg_TRCVDSACCTLR      = 0x884a, // 10  001  0000  1001  010
	A64SysReg_TRCVDARCCTLR      = 0x8852, // 10  001  0000  1010  010
	A64SysReg_TRCSEQEVR0        = 0x8804, // 10  001  0000  0000  100
	A64SysReg_TRCSEQEVR1        = 0x880c, // 10  001  0000  0001  100
	A64SysReg_TRCSEQEVR2        = 0x8814, // 10  001  0000  0010  100
	A64SysReg_TRCSEQRSTEVR      = 0x8834, // 10  001  0000  0110  100
	A64SysReg_TRCSEQSTR         = 0x883c, // 10  001  0000  0111  100
	A64SysReg_TRCEXTINSELR      = 0x8844, // 10  001  0000  1000  100
	A64SysReg_TRCCNTRLDVR0      = 0x8805, // 10  001  0000  0000  101
	A64SysReg_TRCCNTRLDVR1      = 0x880d, // 10  001  0000  0001  101
	A64SysReg_TRCCNTRLDVR2      = 0x8815, // 10  001  0000  0010  101
	A64SysReg_TRCCNTRLDVR3      = 0x881d, // 10  001  0000  0011  101
	A64SysReg_TRCCNTCTLR0       = 0x8825, // 10  001  0000  0100  101
	A64SysReg_TRCCNTCTLR1       = 0x882d, // 10  001  0000  0101  101
	A64SysReg_TRCCNTCTLR2       = 0x8835, // 10  001  0000  0110  101
	A64SysReg_TRCCNTCTLR3       = 0x883d, // 10  001  0000  0111  101
	A64SysReg_TRCCNTVR0         = 0x8845, // 10  001  0000  1000  101
	A64SysReg_TRCCNTVR1         = 0x884d, // 10  001  0000  1001  101
	A64SysReg_TRCCNTVR2         = 0x8855, // 10  001  0000  1010  101
	A64SysReg_TRCCNTVR3         = 0x885d, // 10  001  0000  1011  101
	A64SysReg_TRCIMSPEC0        = 0x8807, // 10  001  0000  0000  111
	A64SysReg_TRCIMSPEC1        = 0x880f, // 10  001  0000  0001  111
	A64SysReg_TRCIMSPEC2        = 0x8817, // 10  001  0000  0010  111
	A64SysReg_TRCIMSPEC3        = 0x881f, // 10  001  0000  0011  111
	A64SysReg_TRCIMSPEC4        = 0x8827, // 10  001  0000  0100  111
	A64SysReg_TRCIMSPEC5        = 0x882f, // 10  001  0000  0101  111
	A64SysReg_TRCIMSPEC6        = 0x8837, // 10  001  0000  0110  111
	A64SysReg_TRCIMSPEC7        = 0x883f, // 10  001  0000  0111  111
	A64SysReg_TRCRSCTLR2        = 0x8890, // 10  001  0001  0010  000
	A64SysReg_TRCRSCTLR3        = 0x8898, // 10  001  0001  0011  000
	A64SysReg_TRCRSCTLR4        = 0x88a0, // 10  001  0001  0100  000
	A64SysReg_TRCRSCTLR5        = 0x88a8, // 10  001  0001  0101  000
	A64SysReg_TRCRSCTLR6        = 0x88b0, // 10  001  0001  0110  000
	A64SysReg_TRCRSCTLR7        = 0x88b8, // 10  001  0001  0111  000
	A64SysReg_TRCRSCTLR8        = 0x88c0, // 10  001  0001  1000  000
	A64SysReg_TRCRSCTLR9        = 0x88c8, // 10  001  0001  1001  000
	A64SysReg_TRCRSCTLR10       = 0x88d0, // 10  001  0001  1010  000
	A64SysReg_TRCRSCTLR11       = 0x88d8, // 10  001  0001  1011  000
	A64SysReg_TRCRSCTLR12       = 0x88e0, // 10  001  0001  1100  000
	A64SysReg_TRCRSCTLR13       = 0x88e8, // 10  001  0001  1101  000
	A64SysReg_TRCRSCTLR14       = 0x88f0, // 10  001  0001  1110  000
	A64SysReg_TRCRSCTLR15       = 0x88f8, // 10  001  0001  1111  000
	A64SysReg_TRCRSCTLR16       = 0x8881, // 10  001  0001  0000  001
	A64SysReg_TRCRSCTLR17       = 0x8889, // 10  001  0001  0001  001
	A64SysReg_TRCRSCTLR18       = 0x8891, // 10  001  0001  0010  001
	A64SysReg_TRCRSCTLR19       = 0x8899, // 10  001  0001  0011  001
	A64SysReg_TRCRSCTLR20       = 0x88a1, // 10  001  0001  0100  001
	A64SysReg_TRCRSCTLR21       = 0x88a9, // 10  001  0001  0101  001
	A64SysReg_TRCRSCTLR22       = 0x88b1, // 10  001  0001  0110  001
	A64SysReg_TRCRSCTLR23       = 0x88b9, // 10  001  0001  0111  001
	A64SysReg_TRCRSCTLR24       = 0x88c1, // 10  001  0001  1000  001
	A64SysReg_TRCRSCTLR25       = 0x88c9, // 10  001  0001  1001  001
	A64SysReg_TRCRSCTLR26       = 0x88d1, // 10  001  0001  1010  001
	A64SysReg_TRCRSCTLR27       = 0x88d9, // 10  001  0001  1011  001
	A64SysReg_TRCRSCTLR28       = 0x88e1, // 10  001  0001  1100  001
	A64SysReg_TRCRSCTLR29       = 0x88e9, // 10  001  0001  1101  001
	A64SysReg_TRCRSCTLR30       = 0x88f1, // 10  001  0001  1110  001
	A64SysReg_TRCRSCTLR31       = 0x88f9, // 10  001  0001  1111  001
	A64SysReg_TRCSSCCR0         = 0x8882, // 10  001  0001  0000  010
	A64SysReg_TRCSSCCR1         = 0x888a, // 10  001  0001  0001  010
	A64SysReg_TRCSSCCR2         = 0x8892, // 10  001  0001  0010  010
	A64SysReg_TRCSSCCR3         = 0x889a, // 10  001  0001  0011  010
	A64SysReg_TRCSSCCR4         = 0x88a2, // 10  001  0001  0100  010
	A64SysReg_TRCSSCCR5         = 0x88aa, // 10  001  0001  0101  010
	A64SysReg_TRCSSCCR6         = 0x88b2, // 10  001  0001  0110  010
	A64SysReg_TRCSSCCR7         = 0x88ba, // 10  001  0001  0111  010
	A64SysReg_TRCSSCSR0         = 0x88c2, // 10  001  0001  1000  010
	A64SysReg_TRCSSCSR1         = 0x88ca, // 10  001  0001  1001  010
	A64SysReg_TRCSSCSR2         = 0x88d2, // 10  001  0001  1010  010
	A64SysReg_TRCSSCSR3         = 0x88da, // 10  001  0001  1011  010
	A64SysReg_TRCSSCSR4         = 0x88e2, // 10  001  0001  1100  010
	A64SysReg_TRCSSCSR5         = 0x88ea, // 10  001  0001  1101  010
	A64SysReg_TRCSSCSR6         = 0x88f2, // 10  001  0001  1110  010
	A64SysReg_TRCSSCSR7         = 0x88fa, // 10  001  0001  1111  010
	A64SysReg_TRCSSPCICR0       = 0x8883, // 10  001  0001  0000  011
	A64SysReg_TRCSSPCICR1       = 0x888b, // 10  001  0001  0001  011
	A64SysReg_TRCSSPCICR2       = 0x8893, // 10  001  0001  0010  011
	A64SysReg_TRCSSPCICR3       = 0x889b, // 10  001  0001  0011  011
	A64SysReg_TRCSSPCICR4       = 0x88a3, // 10  001  0001  0100  011
	A64SysReg_TRCSSPCICR5       = 0x88ab, // 10  001  0001  0101  011
	A64SysReg_TRCSSPCICR6       = 0x88b3, // 10  001  0001  0110  011
	A64SysReg_TRCSSPCICR7       = 0x88bb, // 10  001  0001  0111  011
	A64SysReg_TRCPDCR           = 0x88a4, // 10  001  0001  0100  100
	A64SysReg_TRCACVR0          = 0x8900, // 10  001  0010  0000  000
	A64SysReg_TRCACVR1          = 0x8910, // 10  001  0010  0010  000
	A64SysReg_TRCACVR2          = 0x8920, // 10  001  0010  0100  000
	A64SysReg_TRCACVR3          = 0x8930, // 10  001  0010  0110  000
	A64SysReg_TRCACVR4          = 0x8940, // 10  001  0010  1000  000
	A64SysReg_TRCACVR5          = 0x8950, // 10  001  0010  1010  000
	A64SysReg_TRCACVR6          = 0x8960, // 10  001  0010  1100  000
	A64SysReg_TRCACVR7          = 0x8970, // 10  001  0010  1110  000
	A64SysReg_TRCACVR8          = 0x8901, // 10  001  0010  0000  001
	A64SysReg_TRCACVR9          = 0x8911, // 10  001  0010  0010  001
	A64SysReg_TRCACVR10         = 0x8921, // 10  001  0010  0100  001
	A64SysReg_TRCACVR11         = 0x8931, // 10  001  0010  0110  001
	A64SysReg_TRCACVR12         = 0x8941, // 10  001  0010  1000  001
	A64SysReg_TRCACVR13         = 0x8951, // 10  001  0010  1010  001
	A64SysReg_TRCACVR14         = 0x8961, // 10  001  0010  1100  001
	A64SysReg_TRCACVR15         = 0x8971, // 10  001  0010  1110  001
	A64SysReg_TRCACATR0         = 0x8902, // 10  001  0010  0000  010
	A64SysReg_TRCACATR1         = 0x8912, // 10  001  0010  0010  010
	A64SysReg_TRCACATR2         = 0x8922, // 10  001  0010  0100  010
	A64SysReg_TRCACATR3         = 0x8932, // 10  001  0010  0110  010
	A64SysReg_TRCACATR4         = 0x8942, // 10  001  0010  1000  010
	A64SysReg_TRCACATR5         = 0x8952, // 10  001  0010  1010  010
	A64SysReg_TRCACATR6         = 0x8962, // 10  001  0010  1100  010
	A64SysReg_TRCACATR7         = 0x8972, // 10  001  0010  1110  010
	A64SysReg_TRCACATR8         = 0x8903, // 10  001  0010  0000  011
	A64SysReg_TRCACATR9         = 0x8913, // 10  001  0010  0010  011
	A64SysReg_TRCACATR10        = 0x8923, // 10  001  0010  0100  011
	A64SysReg_TRCACATR11        = 0x8933, // 10  001  0010  0110  011
	A64SysReg_TRCACATR12        = 0x8943, // 10  001  0010  1000  011
	A64SysReg_TRCACATR13        = 0x8953, // 10  001  0010  1010  011
	A64SysReg_TRCACATR14        = 0x8963, // 10  001  0010  1100  011
	A64SysReg_TRCACATR15        = 0x8973, // 10  001  0010  1110  011
	A64SysReg_TRCDVCVR0         = 0x8904, // 10  001  0010  0000  100
	A64SysReg_TRCDVCVR1         = 0x8924, // 10  001  0010  0100  100
	A64SysReg_TRCDVCVR2         = 0x8944, // 10  001  0010  1000  100
	A64SysReg_TRCDVCVR3         = 0x8964, // 10  001  0010  1100  100
	A64SysReg_TRCDVCVR4         = 0x8905, // 10  001  0010  0000  101
	A64SysReg_TRCDVCVR5         = 0x8925, // 10  001  0010  0100  101
	A64SysReg_TRCDVCVR6         = 0x8945, // 10  001  0010  1000  101
	A64SysReg_TRCDVCVR7         = 0x8965, // 10  001  0010  1100  101
	A64SysReg_TRCDVCMR0         = 0x8906, // 10  001  0010  0000  110
	A64SysReg_TRCDVCMR1         = 0x8926, // 10  001  0010  0100  110
	A64SysReg_TRCDVCMR2         = 0x8946, // 10  001  0010  1000  110
	A64SysReg_TRCDVCMR3         = 0x8966, // 10  001  0010  1100  110
	A64SysReg_TRCDVCMR4         = 0x8907, // 10  001  0010  0000  111
	A64SysReg_TRCDVCMR5         = 0x8927, // 10  001  0010  0100  111
	A64SysReg_TRCDVCMR6         = 0x8947, // 10  001  0010  1000  111
	A64SysReg_TRCDVCMR7         = 0x8967, // 10  001  0010  1100  111
	A64SysReg_TRCCIDCVR0        = 0x8980, // 10  001  0011  0000  000
	A64SysReg_TRCCIDCVR1        = 0x8990, // 10  001  0011  0010  000
	A64SysReg_TRCCIDCVR2        = 0x89a0, // 10  001  0011  0100  000
	A64SysReg_TRCCIDCVR3        = 0x89b0, // 10  001  0011  0110  000
	A64SysReg_TRCCIDCVR4        = 0x89c0, // 10  001  0011  1000  000
	A64SysReg_TRCCIDCVR5        = 0x89d0, // 10  001  0011  1010  000
	A64SysReg_TRCCIDCVR6        = 0x89e0, // 10  001  0011  1100  000
	A64SysReg_TRCCIDCVR7        = 0x89f0, // 10  001  0011  1110  000
	A64SysReg_TRCVMIDCVR0       = 0x8981, // 10  001  0011  0000  001
	A64SysReg_TRCVMIDCVR1       = 0x8991, // 10  001  0011  0010  001
	A64SysReg_TRCVMIDCVR2       = 0x89a1, // 10  001  0011  0100  001
	A64SysReg_TRCVMIDCVR3       = 0x89b1, // 10  001  0011  0110  001
	A64SysReg_TRCVMIDCVR4       = 0x89c1, // 10  001  0011  1000  001
	A64SysReg_TRCVMIDCVR5       = 0x89d1, // 10  001  0011  1010  001
	A64SysReg_TRCVMIDCVR6       = 0x89e1, // 10  001  0011  1100  001
	A64SysReg_TRCVMIDCVR7       = 0x89f1, // 10  001  0011  1110  001
	A64SysReg_TRCCIDCCTLR0      = 0x8982, // 10  001  0011  0000  010
	A64SysReg_TRCCIDCCTLR1      = 0x898a, // 10  001  0011  0001  010
	A64SysReg_TRCVMIDCCTLR0     = 0x8992, // 10  001  0011  0010  010
	A64SysReg_TRCVMIDCCTLR1     = 0x899a, // 10  001  0011  0011  010
	A64SysReg_TRCITCTRL         = 0x8b84, // 10  001  0111  0000  100
	A64SysReg_TRCCLAIMSET       = 0x8bc6, // 10  001  0111  1000  110
	A64SysReg_TRCCLAIMCLR       = 0x8bce, // 10  001  0111  1001  110

	// GICv3 registers
	A64SysReg_ICC_BPR1_EL1      = 0xc663, // 11  000  1100  1100  011
	A64SysReg_ICC_BPR0_EL1      = 0xc643, // 11  000  1100  1000  011
	A64SysReg_ICC_PMR_EL1       = 0xc230, // 11  000  0100  0110  000
	A64SysReg_ICC_CTLR_EL1      = 0xc664, // 11  000  1100  1100  100
	A64SysReg_ICC_CTLR_EL3      = 0xf664, // 11  110  1100  1100  100
	A64SysReg_ICC_SRE_EL1       = 0xc665, // 11  000  1100  1100  101
	A64SysReg_ICC_SRE_EL2       = 0xe64d, // 11  100  1100  1001  101
	A64SysReg_ICC_SRE_EL3       = 0xf665, // 11  110  1100  1100  101
	A64SysReg_ICC_IGRPEN0_EL1   = 0xc666, // 11  000  1100  1100  110
	A64SysReg_ICC_IGRPEN1_EL1   = 0xc667, // 11  000  1100  1100  111
	A64SysReg_ICC_IGRPEN1_EL3   = 0xf667, // 11  110  1100  1100  111
	A64SysReg_ICC_SEIEN_EL1     = 0xc668, // 11  000  1100  1101  000
	A64SysReg_ICC_AP0R0_EL1     = 0xc644, // 11  000  1100  1000  100
	A64SysReg_ICC_AP0R1_EL1     = 0xc645, // 11  000  1100  1000  101
	A64SysReg_ICC_AP0R2_EL1     = 0xc646, // 11  000  1100  1000  110
	A64SysReg_ICC_AP0R3_EL1     = 0xc647, // 11  000  1100  1000  111
	A64SysReg_ICC_AP1R0_EL1     = 0xc648, // 11  000  1100  1001  000
	A64SysReg_ICC_AP1R1_EL1     = 0xc649, // 11  000  1100  1001  001
	A64SysReg_ICC_AP1R2_EL1     = 0xc64a, // 11  000  1100  1001  010
	A64SysReg_ICC_AP1R3_EL1     = 0xc64b, // 11  000  1100  1001  011
	A64SysReg_ICH_AP0R0_EL2     = 0xe640, // 11  100  1100  1000  000
	A64SysReg_ICH_AP0R1_EL2     = 0xe641, // 11  100  1100  1000  001
	A64SysReg_ICH_AP0R2_EL2     = 0xe642, // 11  100  1100  1000  010
	A64SysReg_ICH_AP0R3_EL2     = 0xe643, // 11  100  1100  1000  011
	A64SysReg_ICH_AP1R0_EL2     = 0xe648, // 11  100  1100  1001  000
	A64SysReg_ICH_AP1R1_EL2     = 0xe649, // 11  100  1100  1001  001
	A64SysReg_ICH_AP1R2_EL2     = 0xe64a, // 11  100  1100  1001  010
	A64SysReg_ICH_AP1R3_EL2     = 0xe64b, // 11  100  1100  1001  011
	A64SysReg_ICH_HCR_EL2       = 0xe658, // 11  100  1100  1011  000
	A64SysReg_ICH_MISR_EL2      = 0xe65a, // 11  100  1100  1011  010
	A64SysReg_ICH_VMCR_EL2      = 0xe65f, // 11  100  1100  1011  111
	A64SysReg_ICH_VSEIR_EL2     = 0xe64c, // 11  100  1100  1001  100
	A64SysReg_ICH_LR0_EL2       = 0xe660, // 11  100  1100  1100  000
	A64SysReg_ICH_LR1_EL2       = 0xe661, // 11  100  1100  1100  001
	A64SysReg_ICH_LR2_EL2       = 0xe662, // 11  100  1100  1100  010
	A64SysReg_ICH_LR3_EL2       = 0xe663, // 11  100  1100  1100  011
	A64SysReg_ICH_LR4_EL2       = 0xe664, // 11  100  1100  1100  100
	A64SysReg_ICH_LR5_EL2       = 0xe665, // 11  100  1100  1100  101
	A64SysReg_ICH_LR6_EL2       = 0xe666, // 11  100  1100  1100  110
	A64SysReg_ICH_LR7_EL2       = 0xe667, // 11  100  1100  1100  111
	A64SysReg_ICH_LR8_EL2       = 0xe668, // 11  100  1100  1101  000
	A64SysReg_ICH_LR9_EL2       = 0xe669, // 11  100  1100  1101  001
	A64SysReg_ICH_LR10_EL2      = 0xe66a, // 11  100  1100  1101  010
	A64SysReg_ICH_LR11_EL2      = 0xe66b, // 11  100  1100  1101  011
	A64SysReg_ICH_LR12_EL2      = 0xe66c, // 11  100  1100  1101  100
	A64SysReg_ICH_LR13_EL2      = 0xe66d, // 11  100  1100  1101  101
	A64SysReg_ICH_LR14_EL2      = 0xe66e, // 11  100  1100  1101  110
	A64SysReg_ICH_LR15_EL2      = 0xe66f, // 11  100  1100  1101  111

	// Statistical profiling registers
	A64SysReg_PMSIDR_EL1        = 0xc4cf, // 11  000  1001  1001  111
	A64SysReg_PMBIDR_EL1        = 0xc4d7, // 11  000  1001  1010  111
	A64SysReg_PMBLIMITR_EL1     = 0xc4d0, // 11  000  1001  1010  000
	A64SysReg_PMBPTR_EL1        = 0xc4d1, // 11  000  1001  1010  001
	A64SysReg_PMBSR_EL1         = 0xc4d3, // 11  000  1001  1010  011
	A64SysReg_PMSCR_EL1         = 0xc4c8, // 11  000  1001  1001  000
	A64SysReg_PMSCR_EL12        = 0xecc8, // 11  101  1001  1001  000
	A64SysReg_PMSCR_EL2         = 0xe4c8, // 11  100  1001  1001  000
	A64SysReg_PMSICR_EL1        = 0xc4ca, // 11  000  1001  1001  010
	A64SysReg_PMSIRR_EL1        = 0xc4cb, // 11  000  1001  1001  011
	A64SysReg_PMSFCR_EL1        = 0xc4cc, // 11  000  1001  1001  100
	A64SysReg_PMSEVFR_EL1       = 0xc4cd, // 11  000  1001  1001  101
	A64SysReg_PMSLATFR_EL1      = 0xc4ce  // 11  000  1001  1001  110
};

// Cyclone specific system registers
enum A64CycloneSysRegValues {
	A64SysReg_CPM_IOACC_CTL_EL3 = 0xff90
};

enum A64TLBIValues {
	A64TLBI_Invalid = -1,          // Op0 Op1  CRn   CRm   Op2
	A64TLBI_IPAS2E1IS    = 0x6401, // 01  100  1000  0000  001
	A64TLBI_IPAS2LE1IS   = 0x6405, // 01  100  1000  0000  101
	A64TLBI_VMALLE1IS    = 0x4418, // 01  000  1000  0011  000
	A64TLBI_ALLE2IS      = 0x6418, // 01  100  1000  0011  000
	A64TLBI_ALLE3IS      = 0x7418, // 01  110  1000  0011  000
	A64TLBI_VAE1IS       = 0x4419, // 01  000  1000  0011  001
	A64TLBI_VAE2IS       = 0x6419, // 01  100  1000  0011  001
	A64TLBI_VAE3IS       = 0x7419, // 01  110  1000  0011  001
	A64TLBI_ASIDE1IS     = 0x441a, // 01  000  1000  0011  010
	A64TLBI_VAAE1IS      = 0x441b, // 01  000  1000  0011  011
	A64TLBI_ALLE1IS      = 0x641c, // 01  100  1000  0011  100
	A64TLBI_VALE1IS      = 0x441d, // 01  000  1000  0011  101
	A64TLBI_VALE2IS      = 0x641d, // 01  100  1000  0011  101
	A64TLBI_VALE3IS      = 0x741d, // 01  110  1000  0011  101
	A64TLBI_VMALLS12E1IS = 0x641e, // 01  100  1000  0011  110
	A64TLBI_VAALE1IS     = 0x441f, // 01  000  1000  0011  111
	A64TLBI_IPAS2E1      = 0x6421, // 01  100  1000  0100  001
	A64TLBI_IPAS2LE1     = 0x6425, // 01  100  1000  0100  101
	A64TLBI_VMALLE1      = 0x4438, // 01  000  1000  0111  000
	A64TLBI_ALLE2        = 0x6438, // 01  100  1000  0111  000
	A64TLBI_ALLE3        = 0x7438, // 01  110  1000  0111  000
	A64TLBI_VAE1         = 0x4439, // 01  000  1000  0111  001
	A64TLBI_VAE2         = 0x6439, // 01  100  1000  0111  001
	A64TLBI_VAE3         = 0x7439, // 01  110  1000  0111  001
	A64TLBI_ASIDE1       = 0x443a, // 01  000  1000  0111  010
	A64TLBI_VAAE1        = 0x443b, // 01  000  1000  0111  011
	A64TLBI_ALLE1        = 0x643c, // 01  100  1000  0111  100
	A64TLBI_VALE1        = 0x443d, // 01  000  1000  0111  101
	A64TLBI_VALE2        = 0x643d, // 01  100  1000  0111  101
	A64TLBI_VALE3        = 0x743d, // 01  110  1000  0111  101
	A64TLBI_VMALLS12E1   = 0x643e, // 01  100  1000  0111  110
	A64TLBI_VAALE1       = 0x443f  // 01  000  1000  0111  111
};

bool A64Imms_isLogicalImmBits(unsigned RegWidth, uint32_t Bits, uint64_t *Imm);

const char *A64NamedImmMapper_toString(const A64NamedImmMapper *N, uint32_t Value, bool *Valid);

uint32_t A64NamedImmMapper_fromString(const A64NamedImmMapper *N, char *Name, bool *Valid);

bool A64NamedImmMapper_validImm(const A64NamedImmMapper *N, uint32_t Value);

void A64SysRegMapper_toString(const A64SysRegMapper *S, uint32_t Bits, char *result);

#endif
