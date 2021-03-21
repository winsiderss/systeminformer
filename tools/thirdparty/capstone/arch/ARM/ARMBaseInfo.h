//===-- ARMBaseInfo.h - Top level definitions for ARM -------- --*- C++ -*-===//
//
//                     The LLVM Compiler Infrastructure
//
// This file is distributed under the University of Illinois Open Source
// License. See LICENSE.TXT for details.
//
//===----------------------------------------------------------------------===//
//
// This file contains small standalone helper functions and enum definitions for
// the ARM target useful for the compiler back-end and the MC libraries.
// As such, it deliberately does not include references to LLVM core
// code gen types, passes, etc..
//
//===----------------------------------------------------------------------===//

/* Capstone Disassembly Engine */
/* By Nguyen Anh Quynh <aquynh@gmail.com>, 2013-2015 */

#ifndef CS_ARMBASEINFO_H
#define CS_ARMBASEINFO_H

#include "../capstone/include/capstone/arm.h"

// Defines symbolic names for ARM registers.  This defines a mapping from
// register name to register number.
//
#define GET_REGINFO_ENUM
#include "ARMGenRegisterInfo.inc"

// Enums corresponding to ARM condition codes
// The CondCodes constants map directly to the 4-bit encoding of the
// condition field for predicated instructions.
typedef enum ARMCC_CondCodes { // Meaning (integer)          Meaning (floating-point)
	ARMCC_EQ,            // Equal                      Equal
	ARMCC_NE,            // Not equal                  Not equal, or unordered
	ARMCC_HS,            // Carry set                  >, ==, or unordered
	ARMCC_LO,            // Carry clear                Less than
	ARMCC_MI,            // Minus, negative            Less than
	ARMCC_PL,            // Plus, positive or zero     >, ==, or unordered
	ARMCC_VS,            // Overflow                   Unordered
	ARMCC_VC,            // No overflow                Not unordered
	ARMCC_HI,            // Unsigned higher            Greater than, or unordered
	ARMCC_LS,            // Unsigned lower or same     Less than or equal
	ARMCC_GE,            // Greater than or equal      Greater than or equal
	ARMCC_LT,            // Less than                  Less than, or unordered
	ARMCC_GT,            // Greater than               Greater than
	ARMCC_LE,            // Less than or equal         <, ==, or unordered
	ARMCC_AL             // Always (unconditional)     Always (unconditional)
} ARMCC_CondCodes;

inline static ARMCC_CondCodes ARMCC_getOppositeCondition(ARMCC_CondCodes CC)
{
	switch (CC) {
		case ARMCC_EQ: return ARMCC_NE;
		case ARMCC_NE: return ARMCC_EQ;
		case ARMCC_HS: return ARMCC_LO;
		case ARMCC_LO: return ARMCC_HS;
		case ARMCC_MI: return ARMCC_PL;
		case ARMCC_PL: return ARMCC_MI;
		case ARMCC_VS: return ARMCC_VC;
		case ARMCC_VC: return ARMCC_VS;
		case ARMCC_HI: return ARMCC_LS;
		case ARMCC_LS: return ARMCC_HI;
		case ARMCC_GE: return ARMCC_LT;
		case ARMCC_LT: return ARMCC_GE;
		case ARMCC_GT: return ARMCC_LE;
		case ARMCC_LE: return ARMCC_GT;
		default: return ARMCC_AL;
	}
}

inline static const char *ARMCC_ARMCondCodeToString(ARMCC_CondCodes CC)
{
	switch (CC) {
		case ARMCC_EQ:  return "eq";
		case ARMCC_NE:  return "ne";
		case ARMCC_HS:  return "hs";
		case ARMCC_LO:  return "lo";
		case ARMCC_MI:  return "mi";
		case ARMCC_PL:  return "pl";
		case ARMCC_VS:  return "vs";
		case ARMCC_VC:  return "vc";
		case ARMCC_HI:  return "hi";
		case ARMCC_LS:  return "ls";
		case ARMCC_GE:  return "ge";
		case ARMCC_LT:  return "lt";
		case ARMCC_GT:  return "gt";
		case ARMCC_LE:  return "le";
		case ARMCC_AL:  return "al";
		default: return "";
	}
}

inline static const char *ARM_PROC_IFlagsToString(unsigned val)
{
	switch (val) {
		case ARM_CPSFLAG_F: return "f";
		case ARM_CPSFLAG_I: return "i";
		case ARM_CPSFLAG_A: return "a";
		default: return "";
	}
}

inline static const char *ARM_PROC_IModToString(unsigned val)
{
	switch (val) {
		case ARM_CPSMODE_IE: return "ie";
		case ARM_CPSMODE_ID: return "id";
		default: return "";
	}
}

inline static const char *ARM_MB_MemBOptToString(unsigned val, bool HasV8)
{
	switch (val) {
		default: return "BUGBUG";
		case ARM_MB_SY:    return "sy";
		case ARM_MB_ST:    return "st";
		case ARM_MB_LD: return HasV8 ? "ld" : "#0xd";
		case ARM_MB_RESERVED_12: return "#0xc";
		case ARM_MB_ISH:   return "ish";
		case ARM_MB_ISHST: return "ishst";
		case ARM_MB_ISHLD: return HasV8 ?  "ishld" : "#0x9";
		case ARM_MB_RESERVED_8: return "#0x8";
		case ARM_MB_NSH:   return "nsh";
		case ARM_MB_NSHST: return "nshst";
		case ARM_MB_NSHLD: return HasV8 ? "nshld" : "#0x5";
		case ARM_MB_RESERVED_4: return "#0x4";
		case ARM_MB_OSH:   return "osh";
		case ARM_MB_OSHST: return "oshst";
		case ARM_MB_OSHLD: return HasV8 ? "oshld" : "#0x1";
		case ARM_MB_RESERVED_0: return "#0x0";
	}
}

enum ARM_ISB_InstSyncBOpt {
    ARM_ISB_RESERVED_0 = 0,
    ARM_ISB_RESERVED_1 = 1,
    ARM_ISB_RESERVED_2 = 2,
    ARM_ISB_RESERVED_3 = 3,
    ARM_ISB_RESERVED_4 = 4,
    ARM_ISB_RESERVED_5 = 5,
    ARM_ISB_RESERVED_6 = 6,
    ARM_ISB_RESERVED_7 = 7,
    ARM_ISB_RESERVED_8 = 8,
    ARM_ISB_RESERVED_9 = 9,
    ARM_ISB_RESERVED_10 = 10,
    ARM_ISB_RESERVED_11 = 11,
    ARM_ISB_RESERVED_12 = 12,
    ARM_ISB_RESERVED_13 = 13,
    ARM_ISB_RESERVED_14 = 14,
    ARM_ISB_SY = 15
};

inline static const char *ARM_ISB_InstSyncBOptToString(unsigned val)
{
	switch (val) {
		default: // never reach
		case ARM_ISB_RESERVED_0:  return "#0x0";
		case ARM_ISB_RESERVED_1:  return "#0x1";
		case ARM_ISB_RESERVED_2:  return "#0x2";
		case ARM_ISB_RESERVED_3:  return "#0x3";
		case ARM_ISB_RESERVED_4:  return "#0x4";
		case ARM_ISB_RESERVED_5:  return "#0x5";
		case ARM_ISB_RESERVED_6:  return "#0x6";
		case ARM_ISB_RESERVED_7:  return "#0x7";
		case ARM_ISB_RESERVED_8:  return "#0x8";
		case ARM_ISB_RESERVED_9:  return "#0x9";
		case ARM_ISB_RESERVED_10: return "#0xa";
		case ARM_ISB_RESERVED_11: return "#0xb";
		case ARM_ISB_RESERVED_12: return "#0xc";
		case ARM_ISB_RESERVED_13: return "#0xd";
		case ARM_ISB_RESERVED_14: return "#0xe";
		case ARM_ISB_SY:          return "sy";
	}
}

/// isARMLowRegister - Returns true if the register is a low register (r0-r7).
///
static inline bool isARMLowRegister(unsigned Reg)
{
	//using namespace ARM;
	switch (Reg) {
		case ARM_R0:  case ARM_R1:  case ARM_R2:  case ARM_R3:
		case ARM_R4:  case ARM_R5:  case ARM_R6:  case ARM_R7:
			return true;
		default:
			return false;
	}
}

/// ARMII - This namespace holds all of the target specific flags that
/// instruction info tracks.
///
/// ARM Index Modes
enum ARMII_IndexMode {
	ARMII_IndexModeNone  = 0,
	ARMII_IndexModePre   = 1,
	ARMII_IndexModePost  = 2,
	ARMII_IndexModeUpd   = 3
};

/// ARM Addressing Modes
typedef enum ARMII_AddrMode {
	ARMII_AddrModeNone    = 0,
	ARMII_AddrMode1       = 1,
	ARMII_AddrMode2       = 2,
	ARMII_AddrMode3       = 3,
	ARMII_AddrMode4       = 4,
	ARMII_AddrMode5       = 5,
	ARMII_AddrMode6       = 6,
	ARMII_AddrModeT1_1    = 7,
	ARMII_AddrModeT1_2    = 8,
	ARMII_AddrModeT1_4    = 9,
	ARMII_AddrModeT1_s    = 10, // i8 * 4 for pc and sp relative data
	ARMII_AddrModeT2_i12  = 11,
	ARMII_AddrModeT2_i8   = 12,
	ARMII_AddrModeT2_so   = 13,
	ARMII_AddrModeT2_pc   = 14, // +/- i12 for pc relative data
	ARMII_AddrModeT2_i8s4 = 15, // i8 * 4
	ARMII_AddrMode_i12    = 16
} ARMII_AddrMode;

inline static const char *ARMII_AddrModeToString(ARMII_AddrMode addrmode)
{
	switch (addrmode) {
		case ARMII_AddrModeNone:    return "AddrModeNone";
		case ARMII_AddrMode1:       return "AddrMode1";
		case ARMII_AddrMode2:       return "AddrMode2";
		case ARMII_AddrMode3:       return "AddrMode3";
		case ARMII_AddrMode4:       return "AddrMode4";
		case ARMII_AddrMode5:       return "AddrMode5";
		case ARMII_AddrMode6:       return "AddrMode6";
		case ARMII_AddrModeT1_1:    return "AddrModeT1_1";
		case ARMII_AddrModeT1_2:    return "AddrModeT1_2";
		case ARMII_AddrModeT1_4:    return "AddrModeT1_4";
		case ARMII_AddrModeT1_s:    return "AddrModeT1_s";
		case ARMII_AddrModeT2_i12:  return "AddrModeT2_i12";
		case ARMII_AddrModeT2_i8:   return "AddrModeT2_i8";
		case ARMII_AddrModeT2_so:   return "AddrModeT2_so";
		case ARMII_AddrModeT2_pc:   return "AddrModeT2_pc";
		case ARMII_AddrModeT2_i8s4: return "AddrModeT2_i8s4";
		case ARMII_AddrMode_i12:    return "AddrMode_i12";
	}
}

/// Target Operand Flag enum.
enum ARMII_TOF {
	//===------------------------------------------------------------------===//
	// ARM Specific MachineOperand flags.

	ARMII_MO_NO_FLAG,

	/// MO_LO16 - On a symbol operand, this represents a relocation containing
	/// lower 16 bit of the address. Used only via movw instruction.
	ARMII_MO_LO16,

	/// MO_HI16 - On a symbol operand, this represents a relocation containing
	/// higher 16 bit of the address. Used only via movt instruction.
	ARMII_MO_HI16,

	/// MO_LO16_NONLAZY - On a symbol operand "FOO", this represents a
	/// relocation containing lower 16 bit of the non-lazy-ptr indirect symbol,
	/// i.e. "FOO$non_lazy_ptr".
	/// Used only via movw instruction.
	ARMII_MO_LO16_NONLAZY,

	/// MO_HI16_NONLAZY - On a symbol operand "FOO", this represents a
	/// relocation containing lower 16 bit of the non-lazy-ptr indirect symbol,
	/// i.e. "FOO$non_lazy_ptr". Used only via movt instruction.
	ARMII_MO_HI16_NONLAZY,

	/// MO_LO16_NONLAZY_PIC - On a symbol operand "FOO", this represents a
	/// relocation containing lower 16 bit of the PC relative address of the
	/// non-lazy-ptr indirect symbol, i.e. "FOO$non_lazy_ptr - LABEL".
	/// Used only via movw instruction.
	ARMII_MO_LO16_NONLAZY_PIC,

	/// MO_HI16_NONLAZY_PIC - On a symbol operand "FOO", this represents a
	/// relocation containing lower 16 bit of the PC relative address of the
	/// non-lazy-ptr indirect symbol, i.e. "FOO$non_lazy_ptr - LABEL".
	/// Used only via movt instruction.
	ARMII_MO_HI16_NONLAZY_PIC,

	/// MO_PLT - On a symbol operand, this represents an ELF PLT reference on a
	/// call operand.
	ARMII_MO_PLT
};

enum {
	//===------------------------------------------------------------------===//
	// Instruction Flags.

	//===------------------------------------------------------------------===//
	// This four-bit field describes the addressing mode used.
	ARMII_AddrModeMask  = 0x1f, // The AddrMode enums are declared in ARMBaseInfo.h

	// IndexMode - Unindex, pre-indexed, or post-indexed are valid for load
	// and store ops only.  Generic "updating" flag is used for ld/st multiple.
	// The index mode enums are declared in ARMBaseInfo.h
	ARMII_IndexModeShift = 5,
	ARMII_IndexModeMask  = 3 << ARMII_IndexModeShift,

	//===------------------------------------------------------------------===//
	// Instruction encoding formats.
	//
	ARMII_FormShift     = 7,
	ARMII_FormMask      = 0x3f << ARMII_FormShift,

	// Pseudo instructions
	ARMII_Pseudo        = 0  << ARMII_FormShift,

	// Multiply instructions
	ARMII_MulFrm        = 1  << ARMII_FormShift,

	// Branch instructions
	ARMII_BrFrm         = 2  << ARMII_FormShift,
	ARMII_BrMiscFrm     = 3  << ARMII_FormShift,

	// Data Processing instructions
	ARMII_DPFrm         = 4  << ARMII_FormShift,
	ARMII_DPSoRegFrm    = 5  << ARMII_FormShift,

	// Load and Store
	ARMII_LdFrm         = 6  << ARMII_FormShift,
	ARMII_StFrm         = 7  << ARMII_FormShift,
	ARMII_LdMiscFrm     = 8  << ARMII_FormShift,
	ARMII_StMiscFrm     = 9  << ARMII_FormShift,
	ARMII_LdStMulFrm    = 10 << ARMII_FormShift,

	ARMII_LdStExFrm     = 11 << ARMII_FormShift,

	// Miscellaneous arithmetic instructions
	ARMII_ArithMiscFrm  = 12 << ARMII_FormShift,
	ARMII_SatFrm        = 13 << ARMII_FormShift,

	// Extend instructions
	ARMII_ExtFrm        = 14 << ARMII_FormShift,

	// VFP formats
	ARMII_VFPUnaryFrm   = 15 << ARMII_FormShift,
	ARMII_VFPBinaryFrm  = 16 << ARMII_FormShift,
	ARMII_VFPConv1Frm   = 17 << ARMII_FormShift,
	ARMII_VFPConv2Frm   = 18 << ARMII_FormShift,
	ARMII_VFPConv3Frm   = 19 << ARMII_FormShift,
	ARMII_VFPConv4Frm   = 20 << ARMII_FormShift,
	ARMII_VFPConv5Frm   = 21 << ARMII_FormShift,
	ARMII_VFPLdStFrm    = 22 << ARMII_FormShift,
	ARMII_VFPLdStMulFrm = 23 << ARMII_FormShift,
	ARMII_VFPMiscFrm    = 24 << ARMII_FormShift,

	// Thumb format
	ARMII_ThumbFrm      = 25 << ARMII_FormShift,

	// Miscelleaneous format
	ARMII_MiscFrm       = 26 << ARMII_FormShift,

	// NEON formats
	ARMII_NGetLnFrm     = 27 << ARMII_FormShift,
	ARMII_NSetLnFrm     = 28 << ARMII_FormShift,
	ARMII_NDupFrm       = 29 << ARMII_FormShift,
	ARMII_NLdStFrm      = 30 << ARMII_FormShift,
	ARMII_N1RegModImmFrm= 31 << ARMII_FormShift,
	ARMII_N2RegFrm      = 32 << ARMII_FormShift,
	ARMII_NVCVTFrm      = 33 << ARMII_FormShift,
	ARMII_NVDupLnFrm    = 34 << ARMII_FormShift,
	ARMII_N2RegVShLFrm  = 35 << ARMII_FormShift,
	ARMII_N2RegVShRFrm  = 36 << ARMII_FormShift,
	ARMII_N3RegFrm      = 37 << ARMII_FormShift,
	ARMII_N3RegVShFrm   = 38 << ARMII_FormShift,
	ARMII_NVExtFrm      = 39 << ARMII_FormShift,
	ARMII_NVMulSLFrm    = 40 << ARMII_FormShift,
	ARMII_NVTBLFrm      = 41 << ARMII_FormShift,

	//===------------------------------------------------------------------===//
	// Misc flags.

	// UnaryDP - Indicates this is a unary data processing instruction, i.e.
	// it doesn't have a Rn operand.
	ARMII_UnaryDP       = 1 << 13,

	// Xform16Bit - Indicates this Thumb2 instruction may be transformed into
	// a 16-bit Thumb instruction if certain conditions are met.
	ARMII_Xform16Bit    = 1 << 14,

	// ThumbArithFlagSetting - The instruction is a 16-bit flag setting Thumb
	// instruction. Used by the parser to determine whether to require the 'S'
	// suffix on the mnemonic (when not in an IT block) or preclude it (when
	// in an IT block).
	ARMII_ThumbArithFlagSetting = 1 << 18,

	//===------------------------------------------------------------------===//
	// Code domain.
	ARMII_DomainShift   = 15,
	ARMII_DomainMask    = 7 << ARMII_DomainShift,
	ARMII_DomainGeneral = 0 << ARMII_DomainShift,
	ARMII_DomainVFP     = 1 << ARMII_DomainShift,
	ARMII_DomainNEON    = 2 << ARMII_DomainShift,
	ARMII_DomainNEONA8  = 4 << ARMII_DomainShift,

	//===------------------------------------------------------------------===//
	// Field shifts - such shifts are used to set field while generating
	// machine instructions.
	//
	// FIXME: This list will need adjusting/fixing as the MC code emitter
	// takes shape and the ARMCodeEmitter.cpp bits go away.
	ARMII_ShiftTypeShift = 4,

	ARMII_M_BitShift     = 5,
	ARMII_ShiftImmShift  = 5,
	ARMII_ShiftShift     = 7,
	ARMII_N_BitShift     = 7,
	ARMII_ImmHiShift     = 8,
	ARMII_SoRotImmShift  = 8,
	ARMII_RegRsShift     = 8,
	ARMII_ExtRotImmShift = 10,
	ARMII_RegRdLoShift   = 12,
	ARMII_RegRdShift     = 12,
	ARMII_RegRdHiShift   = 16,
	ARMII_RegRnShift     = 16,
	ARMII_S_BitShift     = 20,
	ARMII_W_BitShift     = 21,
	ARMII_AM3_I_BitShift = 22,
	ARMII_D_BitShift     = 22,
	ARMII_U_BitShift     = 23,
	ARMII_P_BitShift     = 24,
	ARMII_I_BitShift     = 25,
	ARMII_CondShift      = 28
};

#endif
