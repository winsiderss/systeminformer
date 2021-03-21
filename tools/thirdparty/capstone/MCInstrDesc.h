//===-- llvm/MC/MCInstrDesc.h - Instruction Descriptors -*- C++ -*-===//
//
//                     The LLVM Compiler Infrastructure
//
// This file is distributed under the University of Illinois Open Source
// License. See LICENSE.TXT for details.
//
//===----------------------------------------------------------------------===//
//
// This file defines the MCOperandInfo and MCInstrDesc classes, which
// are used to describe target instructions and their operands.
//
//===----------------------------------------------------------------------===//

/* Capstone Disassembly Engine */
/* By Nguyen Anh Quynh <aquynh@gmail.com>, 2013-2015 */

#ifndef CS_LLVM_MC_MCINSTRDESC_H
#define CS_LLVM_MC_MCINSTRDESC_H

#include "../capstone/include/capstone/capstone.h"

//===----------------------------------------------------------------------===//
// Machine Operand Flags and Description
//===----------------------------------------------------------------------===//

// Operand constraints
enum MCOI_OperandConstraint {
	MCOI_TIED_TO = 0,    // Must be allocated the same register as.
	MCOI_EARLY_CLOBBER   // Operand is an early clobber register operand
};

/// OperandFlags - These are flags set on operands, but should be considered
/// private, all access should go through the MCOperandInfo accessors.
/// See the accessors for a description of what these are.
enum MCOI_OperandFlags {
	MCOI_LookupPtrRegClass = 0,
	MCOI_Predicate,
	MCOI_OptionalDef
};

/// Operand Type - Operands are tagged with one of the values of this enum.
enum MCOI_OperandType {
	MCOI_OPERAND_UNKNOWN,
	MCOI_OPERAND_IMMEDIATE,
	MCOI_OPERAND_REGISTER,
	MCOI_OPERAND_MEMORY,
	MCOI_OPERAND_PCREL
};


/// MCOperandInfo - This holds information about one operand of a machine
/// instruction, indicating the register class for register operands, etc.
///
typedef struct MCOperandInfo {
	/// RegClass - This specifies the register class enumeration of the operand
	/// if the operand is a register.  If isLookupPtrRegClass is set, then this is
	/// an index that is passed to TargetRegisterInfo::getPointerRegClass(x) to
	/// get a dynamic register class.
	int16_t RegClass;

	/// Flags - These are flags from the MCOI::OperandFlags enum.
	uint8_t Flags;

	/// OperandType - Information about the type of the operand.
	uint8_t OperandType;

	/// Lower 16 bits are used to specify which constraints are set. The higher 16
	/// bits are used to specify the value of constraints (4 bits each).
	uint32_t Constraints;
	/// Currently no other information.
} MCOperandInfo;


//===----------------------------------------------------------------------===//
// Machine Instruction Flags and Description
//===----------------------------------------------------------------------===//

/// MCInstrDesc flags - These should be considered private to the
/// implementation of the MCInstrDesc class.  Clients should use the predicate
/// methods on MCInstrDesc, not use these directly.  These all correspond to
/// bitfields in the MCInstrDesc::Flags field.
enum {
	MCID_Variadic = 0,
	MCID_HasOptionalDef,
	MCID_Pseudo,
	MCID_Return,
	MCID_Call,
	MCID_Barrier,
	MCID_Terminator,
	MCID_Branch,
	MCID_IndirectBranch,
	MCID_Compare,
	MCID_MoveImm,
	MCID_Bitcast,
	MCID_Select,
	MCID_DelaySlot,
	MCID_FoldableAsLoad,
	MCID_MayLoad,
	MCID_MayStore,
	MCID_Predicable,
	MCID_NotDuplicable,
	MCID_UnmodeledSideEffects,
	MCID_Commutable,
	MCID_ConvertibleTo3Addr,
	MCID_UsesCustomInserter,
	MCID_HasPostISelHook,
	MCID_Rematerializable,
	MCID_CheapAsAMove,
	MCID_ExtraSrcRegAllocReq,
	MCID_ExtraDefRegAllocReq,
	MCID_RegSequence,
	MCID_ExtractSubreg,
	MCID_InsertSubreg
};

/// MCInstrDesc - Describe properties that are true of each instruction in the
/// target description file.  This captures information about side effects,
/// register use and many other things.  There is one instance of this struct
/// for each target instruction class, and the MachineInstr class points to
/// this struct directly to describe itself.
typedef struct MCInstrDesc {
	unsigned short  Opcode;        // The opcode number
	unsigned char  NumOperands;   // Num of args (may be more if variable_ops)
	unsigned char  NumDefs;       // Num of args that are definitions
	unsigned short  SchedClass;    // enum identifying instr sched class
	unsigned char  Size;          // Number of bytes in encoding.
	unsigned        Flags;         // Flags identifying machine instr class
	uint64_t        TSFlags;       // Target Specific Flag values
	char ImplicitUses;  // Registers implicitly read by this instr
	char ImplicitDefs;  // Registers implicitly defined by this instr
	const MCOperandInfo *OpInfo;   // 'NumOperands' entries about operands
	uint64_t DeprecatedFeatureMask;// Feature bits that this is deprecated on, if any     
	// A complex method to determine is a certain is deprecated or not, and return        
	// the reason for deprecation.
	//bool (*ComplexDeprecationInfo)(MCInst &, MCSubtargetInfo &, std::string &);           
	unsigned char ComplexDeprecationInfo;	// dummy field, just to satisfy initializer
} MCInstrDesc;

bool MCOperandInfo_isPredicate(const MCOperandInfo *m);

bool MCOperandInfo_isOptionalDef(const MCOperandInfo *m);

#endif
