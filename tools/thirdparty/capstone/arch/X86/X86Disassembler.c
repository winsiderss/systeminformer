//===-- X86Disassembler.cpp - Disassembler for x86 and x86_64 -------------===//
//
//                     The LLVM Compiler Infrastructure
//
// This file is distributed under the University of Illinois Open Source
// License. See LICENSE.TXT for details.
//
//===----------------------------------------------------------------------===//
//
// This file is part of the X86 Disassembler.
// It contains code to translate the data produced by the decoder into
//  MCInsts.
// Documentation for the disassembler can be found in X86Disassembler.h.
//
//===----------------------------------------------------------------------===//

/* Capstone Disassembly Engine */
/* By Nguyen Anh Quynh <aquynh@gmail.com>, 2013-2015 */

#ifdef CAPSTONE_HAS_X86

#if defined (WIN32) || defined (WIN64) || defined (_WIN32) || defined (_WIN64)
#pragma warning(disable:4996)			// disable MSVC's warning on strncpy()
#pragma warning(disable:28719)		// disable MSVC's warning on strncpy()
#endif

#include "../capstone/include/capstone/platform.h"

#if defined(CAPSTONE_HAS_OSXKERNEL)
#include <Availability.h>
#endif

#include <string.h>

#include "../../cs_priv.h"

#include "X86Disassembler.h"
#include "X86DisassemblerDecoderCommon.h"
#include "X86DisassemblerDecoder.h"
#include "../../MCInst.h"
#include "../../utils.h"
#include "X86Mapping.h"

#define GET_REGINFO_ENUM
#define GET_REGINFO_MC_DESC
#include "X86GenRegisterInfo.inc"

#define GET_INSTRINFO_ENUM
#ifdef CAPSTONE_X86_REDUCE
#include "X86GenInstrInfo_reduce.inc"
#else
#include "X86GenInstrInfo.inc"
#endif

// Fill-ins to make the compiler happy.  These constants are never actually
//   assigned; they are just filler to make an automatically-generated switch
//   statement work.
enum {
	X86_BX_SI = 500,
	X86_BX_DI = 501,
	X86_BP_SI = 502,
	X86_BP_DI = 503,
	X86_sib   = 504,
	X86_sib64 = 505
};

//
// Private code that translates from struct InternalInstructions to MCInsts.
//

/// translateRegister - Translates an internal register to the appropriate LLVM
///   register, and appends it as an operand to an MCInst.
///
/// @param mcInst     - The MCInst to append to.
/// @param reg        - The Reg to append.
static void translateRegister(MCInst *mcInst, Reg reg)
{
#define ENTRY(x) X86_##x,
	static const uint8_t llvmRegnums[] = {
		ALL_REGS
			0
	};
#undef ENTRY

	uint8_t llvmRegnum = llvmRegnums[reg];
	MCOperand_CreateReg0(mcInst, llvmRegnum);
}

static const uint8_t segmentRegnums[SEG_OVERRIDE_max] = {
	0,        // SEG_OVERRIDE_NONE
	X86_CS,
	X86_SS,
	X86_DS,
	X86_ES,
	X86_FS,
	X86_GS
};

/// translateSrcIndex   - Appends a source index operand to an MCInst.
///
/// @param mcInst       - The MCInst to append to.
/// @param insn         - The internal instruction.
static bool translateSrcIndex(MCInst *mcInst, InternalInstruction *insn)
{
	unsigned baseRegNo;

	if (insn->mode == MODE_64BIT)
		baseRegNo = insn->isPrefix67 ? X86_ESI : X86_RSI;
	else if (insn->mode == MODE_32BIT)
		baseRegNo = insn->isPrefix67 ? X86_SI : X86_ESI;
	else {
		// assert(insn->mode == MODE_16BIT);
		baseRegNo = insn->isPrefix67 ? X86_ESI : X86_SI;
	}

	MCOperand_CreateReg0(mcInst, baseRegNo);

	MCOperand_CreateReg0(mcInst, segmentRegnums[insn->segmentOverride]);

	return false;
}

/// translateDstIndex   - Appends a destination index operand to an MCInst.
///
/// @param mcInst       - The MCInst to append to.
/// @param insn         - The internal instruction.
static bool translateDstIndex(MCInst *mcInst, InternalInstruction *insn)
{
	unsigned baseRegNo;

	if (insn->mode == MODE_64BIT)
		baseRegNo = insn->isPrefix67 ? X86_EDI : X86_RDI;
	else if (insn->mode == MODE_32BIT)
		baseRegNo = insn->isPrefix67 ? X86_DI : X86_EDI;
	else {
		// assert(insn->mode == MODE_16BIT);
		baseRegNo = insn->isPrefix67 ? X86_EDI : X86_DI;
	}

	MCOperand_CreateReg0(mcInst, baseRegNo);

	return false;
}

/// translateImmediate  - Appends an immediate operand to an MCInst.
///
/// @param mcInst       - The MCInst to append to.
/// @param immediate    - The immediate value to append.
/// @param operand      - The operand, as stored in the descriptor table.
/// @param insn         - The internal instruction.
static void translateImmediate(MCInst *mcInst, uint64_t immediate,
		const OperandSpecifier *operand, InternalInstruction *insn)
{
	OperandType type;

	type = (OperandType)operand->type;
	if (type == TYPE_RELv) {
		//isBranch = true;
		//pcrel = insn->startLocation + insn->immediateOffset + insn->immediateSize;
		switch (insn->displacementSize) {
			case 1:
				if (immediate & 0x80)
					immediate |= ~(0xffull);
				break;
			case 2:
				if (immediate & 0x8000)
					immediate |= ~(0xffffull);
				break;
			case 4:
				if (immediate & 0x80000000)
					immediate |= ~(0xffffffffull);
				break;
			case 8:
				break;
			default:
				break;
		}
	} // By default sign-extend all X86 immediates based on their encoding.
	else if (type == TYPE_IMM8 || type == TYPE_IMM16 || type == TYPE_IMM32 ||
			type == TYPE_IMM64 || type == TYPE_IMMv) {

		switch (operand->encoding) {
			default:
				break;
			case ENCODING_IB:
				if(immediate & 0x80)
					immediate |= ~(0xffull);
				break;
			case ENCODING_IW:
				if(immediate & 0x8000)
					immediate |= ~(0xffffull);
				break;
			case ENCODING_ID:
				if(immediate & 0x80000000)
					immediate |= ~(0xffffffffull);
				break;
			case ENCODING_IO:
				break;
		}
	} else if (type == TYPE_IMM3) {
#ifndef CAPSTONE_X86_REDUCE
		// Check for immediates that printSSECC can't handle.
		if (immediate >= 8) {
			unsigned NewOpc = 0;

			switch (MCInst_getOpcode(mcInst)) {
				default: break;	// never reach
				case X86_CMPPDrmi:  NewOpc = X86_CMPPDrmi_alt;  break;
				case X86_CMPPDrri:  NewOpc = X86_CMPPDrri_alt;  break;
				case X86_CMPPSrmi:  NewOpc = X86_CMPPSrmi_alt;  break;
				case X86_CMPPSrri:  NewOpc = X86_CMPPSrri_alt;  break;
				case X86_CMPSDrm:   NewOpc = X86_CMPSDrm_alt;   break;
				case X86_CMPSDrr:   NewOpc = X86_CMPSDrr_alt;   break;
				case X86_CMPSSrm:   NewOpc = X86_CMPSSrm_alt;   break;
				case X86_CMPSSrr:   NewOpc = X86_CMPSSrr_alt;   break;
				case X86_VPCOMBri:  NewOpc = X86_VPCOMBri_alt;  break;
				case X86_VPCOMBmi:  NewOpc = X86_VPCOMBmi_alt;  break;
				case X86_VPCOMWri:  NewOpc = X86_VPCOMWri_alt;  break;
				case X86_VPCOMWmi:  NewOpc = X86_VPCOMWmi_alt;  break;
				case X86_VPCOMDri:  NewOpc = X86_VPCOMDri_alt;  break;
				case X86_VPCOMDmi:  NewOpc = X86_VPCOMDmi_alt;  break;
				case X86_VPCOMQri:  NewOpc = X86_VPCOMQri_alt;  break;
				case X86_VPCOMQmi:  NewOpc = X86_VPCOMQmi_alt;  break;
				case X86_VPCOMUBri: NewOpc = X86_VPCOMUBri_alt; break;
				case X86_VPCOMUBmi: NewOpc = X86_VPCOMUBmi_alt; break;
				case X86_VPCOMUWri: NewOpc = X86_VPCOMUWri_alt; break;
				case X86_VPCOMUWmi: NewOpc = X86_VPCOMUWmi_alt; break;
				case X86_VPCOMUDri: NewOpc = X86_VPCOMUDri_alt; break;
				case X86_VPCOMUDmi: NewOpc = X86_VPCOMUDmi_alt; break;
				case X86_VPCOMUQri: NewOpc = X86_VPCOMUQri_alt; break;
				case X86_VPCOMUQmi: NewOpc = X86_VPCOMUQmi_alt; break;
			}
			// Switch opcode to the one that doesn't get special printing.
			if (NewOpc != 0) {
				MCInst_setOpcode(mcInst, NewOpc);
			}
		}
#endif
	} else if (type == TYPE_IMM5) {
#ifndef CAPSTONE_X86_REDUCE
		// Check for immediates that printAVXCC can't handle.
		if (immediate >= 32) {
			unsigned NewOpc = 0;

			switch (MCInst_getOpcode(mcInst)) {
				default: break; // unexpected opcode
				case X86_VCMPPDrmi:   NewOpc = X86_VCMPPDrmi_alt;   break;
				case X86_VCMPPDrri:   NewOpc = X86_VCMPPDrri_alt;   break;
				case X86_VCMPPSrmi:   NewOpc = X86_VCMPPSrmi_alt;   break;
				case X86_VCMPPSrri:   NewOpc = X86_VCMPPSrri_alt;   break;
				case X86_VCMPSDrm:    NewOpc = X86_VCMPSDrm_alt;    break;
				case X86_VCMPSDrr:    NewOpc = X86_VCMPSDrr_alt;    break;
				case X86_VCMPSSrm:    NewOpc = X86_VCMPSSrm_alt;    break;
				case X86_VCMPSSrr:    NewOpc = X86_VCMPSSrr_alt;    break;
				case X86_VCMPPDYrmi:  NewOpc = X86_VCMPPDYrmi_alt;  break;
				case X86_VCMPPDYrri:  NewOpc = X86_VCMPPDYrri_alt;  break;
				case X86_VCMPPSYrmi:  NewOpc = X86_VCMPPSYrmi_alt;  break;
				case X86_VCMPPSYrri:  NewOpc = X86_VCMPPSYrri_alt;  break;
				case X86_VCMPPDZrmi:  NewOpc = X86_VCMPPDZrmi_alt;  break;
				case X86_VCMPPDZrri:  NewOpc = X86_VCMPPDZrri_alt;  break;
				case X86_VCMPPDZrrib: NewOpc = X86_VCMPPDZrrib_alt; break;
				case X86_VCMPPSZrmi:  NewOpc = X86_VCMPPSZrmi_alt;  break;
				case X86_VCMPPSZrri:  NewOpc = X86_VCMPPSZrri_alt;  break;
				case X86_VCMPPSZrrib: NewOpc = X86_VCMPPSZrrib_alt; break;
				case X86_VCMPSDZrm:   NewOpc = X86_VCMPSDZrmi_alt;  break;
				case X86_VCMPSDZrr:   NewOpc = X86_VCMPSDZrri_alt;  break;
				case X86_VCMPSSZrm:   NewOpc = X86_VCMPSSZrmi_alt;  break;
				case X86_VCMPSSZrr:   NewOpc = X86_VCMPSSZrri_alt;  break;
			}
			// Switch opcode to the one that doesn't get special printing.
			if (NewOpc != 0) {
				MCInst_setOpcode(mcInst, NewOpc);
			}
		}
#endif
	} else if (type == TYPE_AVX512ICC) {
#ifndef CAPSTONE_X86_REDUCE
		if (immediate >= 8 || ((immediate & 0x3) == 3)) {
			unsigned NewOpc = 0;
			switch (MCInst_getOpcode(mcInst)) {
				default: // llvm_unreachable("unexpected opcode");
				case X86_VPCMPBZ128rmi:    NewOpc = X86_VPCMPBZ128rmi_alt;    break;
				case X86_VPCMPBZ128rmik:   NewOpc = X86_VPCMPBZ128rmik_alt;   break;
				case X86_VPCMPBZ128rri:    NewOpc = X86_VPCMPBZ128rri_alt;    break;
				case X86_VPCMPBZ128rrik:   NewOpc = X86_VPCMPBZ128rrik_alt;   break;
				case X86_VPCMPBZ256rmi:    NewOpc = X86_VPCMPBZ256rmi_alt;    break;
				case X86_VPCMPBZ256rmik:   NewOpc = X86_VPCMPBZ256rmik_alt;   break;
				case X86_VPCMPBZ256rri:    NewOpc = X86_VPCMPBZ256rri_alt;    break;
				case X86_VPCMPBZ256rrik:   NewOpc = X86_VPCMPBZ256rrik_alt;   break;
				case X86_VPCMPBZrmi:       NewOpc = X86_VPCMPBZrmi_alt;       break;
				case X86_VPCMPBZrmik:      NewOpc = X86_VPCMPBZrmik_alt;      break;
				case X86_VPCMPBZrri:       NewOpc = X86_VPCMPBZrri_alt;       break;
				case X86_VPCMPBZrrik:      NewOpc = X86_VPCMPBZrrik_alt;      break;
				case X86_VPCMPDZ128rmi:    NewOpc = X86_VPCMPDZ128rmi_alt;    break;
				case X86_VPCMPDZ128rmib:   NewOpc = X86_VPCMPDZ128rmib_alt;   break;
				case X86_VPCMPDZ128rmibk:  NewOpc = X86_VPCMPDZ128rmibk_alt;  break;
				case X86_VPCMPDZ128rmik:   NewOpc = X86_VPCMPDZ128rmik_alt;   break;
				case X86_VPCMPDZ128rri:    NewOpc = X86_VPCMPDZ128rri_alt;    break;
				case X86_VPCMPDZ128rrik:   NewOpc = X86_VPCMPDZ128rrik_alt;   break;
				case X86_VPCMPDZ256rmi:    NewOpc = X86_VPCMPDZ256rmi_alt;    break;
				case X86_VPCMPDZ256rmib:   NewOpc = X86_VPCMPDZ256rmib_alt;   break;
				case X86_VPCMPDZ256rmibk:  NewOpc = X86_VPCMPDZ256rmibk_alt;  break;
				case X86_VPCMPDZ256rmik:   NewOpc = X86_VPCMPDZ256rmik_alt;   break;
				case X86_VPCMPDZ256rri:    NewOpc = X86_VPCMPDZ256rri_alt;    break;
				case X86_VPCMPDZ256rrik:   NewOpc = X86_VPCMPDZ256rrik_alt;   break;
				case X86_VPCMPDZrmi:       NewOpc = X86_VPCMPDZrmi_alt;       break;
				case X86_VPCMPDZrmib:      NewOpc = X86_VPCMPDZrmib_alt;      break;
				case X86_VPCMPDZrmibk:     NewOpc = X86_VPCMPDZrmibk_alt;     break;
				case X86_VPCMPDZrmik:      NewOpc = X86_VPCMPDZrmik_alt;      break;
				case X86_VPCMPDZrri:       NewOpc = X86_VPCMPDZrri_alt;       break;
				case X86_VPCMPDZrrik:      NewOpc = X86_VPCMPDZrrik_alt;      break;
				case X86_VPCMPQZ128rmi:    NewOpc = X86_VPCMPQZ128rmi_alt;    break;
				case X86_VPCMPQZ128rmib:   NewOpc = X86_VPCMPQZ128rmib_alt;   break;
				case X86_VPCMPQZ128rmibk:  NewOpc = X86_VPCMPQZ128rmibk_alt;  break;
				case X86_VPCMPQZ128rmik:   NewOpc = X86_VPCMPQZ128rmik_alt;   break;
				case X86_VPCMPQZ128rri:    NewOpc = X86_VPCMPQZ128rri_alt;    break;
				case X86_VPCMPQZ128rrik:   NewOpc = X86_VPCMPQZ128rrik_alt;   break;
				case X86_VPCMPQZ256rmi:    NewOpc = X86_VPCMPQZ256rmi_alt;    break;
				case X86_VPCMPQZ256rmib:   NewOpc = X86_VPCMPQZ256rmib_alt;   break;
				case X86_VPCMPQZ256rmibk:  NewOpc = X86_VPCMPQZ256rmibk_alt;  break;
				case X86_VPCMPQZ256rmik:   NewOpc = X86_VPCMPQZ256rmik_alt;   break;
				case X86_VPCMPQZ256rri:    NewOpc = X86_VPCMPQZ256rri_alt;    break;
				case X86_VPCMPQZ256rrik:   NewOpc = X86_VPCMPQZ256rrik_alt;   break;
				case X86_VPCMPQZrmi:       NewOpc = X86_VPCMPQZrmi_alt;       break;
				case X86_VPCMPQZrmib:      NewOpc = X86_VPCMPQZrmib_alt;      break;
				case X86_VPCMPQZrmibk:     NewOpc = X86_VPCMPQZrmibk_alt;     break;
				case X86_VPCMPQZrmik:      NewOpc = X86_VPCMPQZrmik_alt;      break;
				case X86_VPCMPQZrri:       NewOpc = X86_VPCMPQZrri_alt;       break;
				case X86_VPCMPQZrrik:      NewOpc = X86_VPCMPQZrrik_alt;      break;
				case X86_VPCMPUBZ128rmi:   NewOpc = X86_VPCMPUBZ128rmi_alt;   break;
				case X86_VPCMPUBZ128rmik:  NewOpc = X86_VPCMPUBZ128rmik_alt;  break;
				case X86_VPCMPUBZ128rri:   NewOpc = X86_VPCMPUBZ128rri_alt;   break;
				case X86_VPCMPUBZ128rrik:  NewOpc = X86_VPCMPUBZ128rrik_alt;  break;
				case X86_VPCMPUBZ256rmi:   NewOpc = X86_VPCMPUBZ256rmi_alt;   break;
				case X86_VPCMPUBZ256rmik:  NewOpc = X86_VPCMPUBZ256rmik_alt;  break;
				case X86_VPCMPUBZ256rri:   NewOpc = X86_VPCMPUBZ256rri_alt;   break;
				case X86_VPCMPUBZ256rrik:  NewOpc = X86_VPCMPUBZ256rrik_alt;  break;
				case X86_VPCMPUBZrmi:      NewOpc = X86_VPCMPUBZrmi_alt;      break;
				case X86_VPCMPUBZrmik:     NewOpc = X86_VPCMPUBZrmik_alt;     break;
				case X86_VPCMPUBZrri:      NewOpc = X86_VPCMPUBZrri_alt;      break;
				case X86_VPCMPUBZrrik:     NewOpc = X86_VPCMPUBZrrik_alt;     break;
				case X86_VPCMPUDZ128rmi:   NewOpc = X86_VPCMPUDZ128rmi_alt;   break;
				case X86_VPCMPUDZ128rmib:  NewOpc = X86_VPCMPUDZ128rmib_alt;  break;
				case X86_VPCMPUDZ128rmibk: NewOpc = X86_VPCMPUDZ128rmibk_alt; break;
				case X86_VPCMPUDZ128rmik:  NewOpc = X86_VPCMPUDZ128rmik_alt;  break;
				case X86_VPCMPUDZ128rri:   NewOpc = X86_VPCMPUDZ128rri_alt;   break;
				case X86_VPCMPUDZ128rrik:  NewOpc = X86_VPCMPUDZ128rrik_alt;  break;
				case X86_VPCMPUDZ256rmi:   NewOpc = X86_VPCMPUDZ256rmi_alt;   break;
				case X86_VPCMPUDZ256rmib:  NewOpc = X86_VPCMPUDZ256rmib_alt;  break;
				case X86_VPCMPUDZ256rmibk: NewOpc = X86_VPCMPUDZ256rmibk_alt; break;
				case X86_VPCMPUDZ256rmik:  NewOpc = X86_VPCMPUDZ256rmik_alt;  break;
				case X86_VPCMPUDZ256rri:   NewOpc = X86_VPCMPUDZ256rri_alt;   break;
				case X86_VPCMPUDZ256rrik:  NewOpc = X86_VPCMPUDZ256rrik_alt;  break;
				case X86_VPCMPUDZrmi:      NewOpc = X86_VPCMPUDZrmi_alt;      break;
				case X86_VPCMPUDZrmib:     NewOpc = X86_VPCMPUDZrmib_alt;     break;
				case X86_VPCMPUDZrmibk:    NewOpc = X86_VPCMPUDZrmibk_alt;    break;
				case X86_VPCMPUDZrmik:     NewOpc = X86_VPCMPUDZrmik_alt;     break;
				case X86_VPCMPUDZrri:      NewOpc = X86_VPCMPUDZrri_alt;      break;
				case X86_VPCMPUDZrrik:     NewOpc = X86_VPCMPUDZrrik_alt;     break;
				case X86_VPCMPUQZ128rmi:   NewOpc = X86_VPCMPUQZ128rmi_alt;   break;
				case X86_VPCMPUQZ128rmib:  NewOpc = X86_VPCMPUQZ128rmib_alt;  break;
				case X86_VPCMPUQZ128rmibk: NewOpc = X86_VPCMPUQZ128rmibk_alt; break;
				case X86_VPCMPUQZ128rmik:  NewOpc = X86_VPCMPUQZ128rmik_alt;  break;
				case X86_VPCMPUQZ128rri:   NewOpc = X86_VPCMPUQZ128rri_alt;   break;
				case X86_VPCMPUQZ128rrik:  NewOpc = X86_VPCMPUQZ128rrik_alt;  break;
				case X86_VPCMPUQZ256rmi:   NewOpc = X86_VPCMPUQZ256rmi_alt;   break;
				case X86_VPCMPUQZ256rmib:  NewOpc = X86_VPCMPUQZ256rmib_alt;  break;
				case X86_VPCMPUQZ256rmibk: NewOpc = X86_VPCMPUQZ256rmibk_alt; break;
				case X86_VPCMPUQZ256rmik:  NewOpc = X86_VPCMPUQZ256rmik_alt;  break;
				case X86_VPCMPUQZ256rri:   NewOpc = X86_VPCMPUQZ256rri_alt;   break;
				case X86_VPCMPUQZ256rrik:  NewOpc = X86_VPCMPUQZ256rrik_alt;  break;
				case X86_VPCMPUQZrmi:      NewOpc = X86_VPCMPUQZrmi_alt;      break;
				case X86_VPCMPUQZrmib:     NewOpc = X86_VPCMPUQZrmib_alt;     break;
				case X86_VPCMPUQZrmibk:    NewOpc = X86_VPCMPUQZrmibk_alt;    break;
				case X86_VPCMPUQZrmik:     NewOpc = X86_VPCMPUQZrmik_alt;     break;
				case X86_VPCMPUQZrri:      NewOpc = X86_VPCMPUQZrri_alt;      break;
				case X86_VPCMPUQZrrik:     NewOpc = X86_VPCMPUQZrrik_alt;     break;
				case X86_VPCMPUWZ128rmi:   NewOpc = X86_VPCMPUWZ128rmi_alt;   break;
				case X86_VPCMPUWZ128rmik:  NewOpc = X86_VPCMPUWZ128rmik_alt;  break;
				case X86_VPCMPUWZ128rri:   NewOpc = X86_VPCMPUWZ128rri_alt;   break;
				case X86_VPCMPUWZ128rrik:  NewOpc = X86_VPCMPUWZ128rrik_alt;  break;
				case X86_VPCMPUWZ256rmi:   NewOpc = X86_VPCMPUWZ256rmi_alt;   break;
				case X86_VPCMPUWZ256rmik:  NewOpc = X86_VPCMPUWZ256rmik_alt;  break;
				case X86_VPCMPUWZ256rri:   NewOpc = X86_VPCMPUWZ256rri_alt;   break;
				case X86_VPCMPUWZ256rrik:  NewOpc = X86_VPCMPUWZ256rrik_alt;  break;
				case X86_VPCMPUWZrmi:      NewOpc = X86_VPCMPUWZrmi_alt;      break;
				case X86_VPCMPUWZrmik:     NewOpc = X86_VPCMPUWZrmik_alt;     break;
				case X86_VPCMPUWZrri:      NewOpc = X86_VPCMPUWZrri_alt;      break;
				case X86_VPCMPUWZrrik:     NewOpc = X86_VPCMPUWZrrik_alt;     break;
				case X86_VPCMPWZ128rmi:    NewOpc = X86_VPCMPWZ128rmi_alt;    break;
				case X86_VPCMPWZ128rmik:   NewOpc = X86_VPCMPWZ128rmik_alt;   break;
				case X86_VPCMPWZ128rri:    NewOpc = X86_VPCMPWZ128rri_alt;    break;
				case X86_VPCMPWZ128rrik:   NewOpc = X86_VPCMPWZ128rrik_alt;   break;
				case X86_VPCMPWZ256rmi:    NewOpc = X86_VPCMPWZ256rmi_alt;    break;
				case X86_VPCMPWZ256rmik:   NewOpc = X86_VPCMPWZ256rmik_alt;   break;
				case X86_VPCMPWZ256rri:    NewOpc = X86_VPCMPWZ256rri_alt;    break;
				case X86_VPCMPWZ256rrik:   NewOpc = X86_VPCMPWZ256rrik_alt;   break;
				case X86_VPCMPWZrmi:       NewOpc = X86_VPCMPWZrmi_alt;       break;
				case X86_VPCMPWZrmik:      NewOpc = X86_VPCMPWZrmik_alt;      break;
				case X86_VPCMPWZrri:       NewOpc = X86_VPCMPWZrri_alt;       break;
				case X86_VPCMPWZrrik:      NewOpc = X86_VPCMPWZrrik_alt;      break;
			}
			// Switch opcode to the one that doesn't get special printing.
			if (NewOpc != 0) {
				MCInst_setOpcode(mcInst, NewOpc);
			}
		}
#endif
	}

	switch (type) {
		case TYPE_XMM32:
		case TYPE_XMM64:
		case TYPE_XMM128:
			MCOperand_CreateReg0(mcInst, X86_XMM0 + ((uint32_t)immediate >> 4));
			return;
		case TYPE_XMM256:
			MCOperand_CreateReg0(mcInst, X86_YMM0 + ((uint32_t)immediate >> 4));
			return;
		case TYPE_XMM512:
			MCOperand_CreateReg0(mcInst, X86_ZMM0 + ((uint32_t)immediate >> 4));
			return;
		case TYPE_REL8:
			if(immediate & 0x80)
				immediate |= ~(0xffull);
			break;
		case TYPE_REL32:
		case TYPE_REL64:
			if(immediate & 0x80000000)
				immediate |= ~(0xffffffffull);
			break;
		default:
			// operand is 64 bits wide.  Do nothing.
			break;
	}

	MCOperand_CreateImm0(mcInst, immediate);

	if (type == TYPE_MOFFS8 || type == TYPE_MOFFS16 ||
			type == TYPE_MOFFS32 || type == TYPE_MOFFS64) {
		MCOperand_CreateReg0(mcInst, segmentRegnums[insn->segmentOverride]);
	}
}

/// translateRMRegister - Translates a register stored in the R/M field of the
///   ModR/M byte to its LLVM equivalent and appends it to an MCInst.
/// @param mcInst       - The MCInst to append to.
/// @param insn         - The internal instruction to extract the R/M field
///                       from.
/// @return             - 0 on success; -1 otherwise
static bool translateRMRegister(MCInst *mcInst, InternalInstruction *insn)
{
	if (insn->eaBase == EA_BASE_sib || insn->eaBase == EA_BASE_sib64) {
		//debug("A R/M register operand may not have a SIB byte");
		return true;
	}

	switch (insn->eaBase) {
		case EA_BASE_NONE:
			//debug("EA_BASE_NONE for ModR/M base");
			return true;
#define ENTRY(x) case EA_BASE_##x:
			ALL_EA_BASES
#undef ENTRY
				//debug("A R/M register operand may not have a base; "
				//      "the operand must be a register.");
				return true;
#define ENTRY(x)                                                      \
		case EA_REG_##x:                                                    \
			MCOperand_CreateReg0(mcInst, X86_##x); break;
			ALL_REGS
#undef ENTRY
		default:
				//debug("Unexpected EA base register");
				return true;
	}

	return false;
}

/// translateRMMemory - Translates a memory operand stored in the Mod and R/M
///   fields of an internal instruction (and possibly its SIB byte) to a memory
///   operand in LLVM's format, and appends it to an MCInst.
///
/// @param mcInst       - The MCInst to append to.
/// @param insn         - The instruction to extract Mod, R/M, and SIB fields
///                       from.
/// @return             - 0 on success; nonzero otherwise
static bool translateRMMemory(MCInst *mcInst, InternalInstruction *insn)
{
	// Addresses in an MCInst are represented as five operands:
	//   1. basereg       (register)  The R/M base, or (if there is a SIB) the
	//                                SIB base
	//   2. scaleamount   (immediate) 1, or (if there is a SIB) the specified
	//                                scale amount
	//   3. indexreg      (register)  x86_registerNONE, or (if there is a SIB)
	//                                the index (which is multiplied by the
	//                                scale amount)
	//   4. displacement  (immediate) 0, or the displacement if there is one
	//   5. segmentreg    (register)  x86_registerNONE for now, but could be set
	//                                if we have segment overrides

	bool IndexIs512, IndexIs128, IndexIs256;
	int scaleAmount, indexReg;
#ifndef CAPSTONE_X86_REDUCE
	uint32_t Opcode;
#endif

	if (insn->eaBase == EA_BASE_sib || insn->eaBase == EA_BASE_sib64) {
		if (insn->sibBase != SIB_BASE_NONE) {
			switch (insn->sibBase) {
#define ENTRY(x)                                          \
				case SIB_BASE_##x:                                  \
				MCOperand_CreateReg0(mcInst, X86_##x); break;
				ALL_SIB_BASES
#undef ENTRY
				default:
					//debug("Unexpected sibBase");
					return true;
			}
		} else {
			MCOperand_CreateReg0(mcInst, 0);
		}

		// Check whether we are handling VSIB addressing mode for GATHER.
		// If sibIndex was set to SIB_INDEX_NONE, index offset is 4 and
		// we should use SIB_INDEX_XMM4|YMM4 for VSIB.
		// I don't see a way to get the correct IndexReg in readSIB:
		//   We can tell whether it is VSIB or SIB after instruction ID is decoded,
		//   but instruction ID may not be decoded yet when calling readSIB.
#ifndef CAPSTONE_X86_REDUCE
		Opcode = MCInst_getOpcode(mcInst);
#endif
		IndexIs128 = (
#ifndef CAPSTONE_X86_REDUCE
				Opcode == X86_VGATHERDPDrm ||
				Opcode == X86_VGATHERDPDYrm ||
				Opcode == X86_VGATHERQPDrm ||
				Opcode == X86_VGATHERDPSrm ||
				Opcode == X86_VGATHERQPSrm ||
				Opcode == X86_VPGATHERDQrm ||
				Opcode == X86_VPGATHERDQYrm ||
				Opcode == X86_VPGATHERQQrm ||
				Opcode == X86_VPGATHERDDrm ||
				Opcode == X86_VPGATHERQDrm ||
#endif
				false
				);
		IndexIs256 = (
#ifndef CAPSTONE_X86_REDUCE
				Opcode == X86_VGATHERQPDYrm ||
				Opcode == X86_VGATHERDPSYrm ||
				Opcode == X86_VGATHERQPSYrm ||
				Opcode == X86_VGATHERDPDZrm ||
				Opcode == X86_VPGATHERDQZrm ||
				Opcode == X86_VPGATHERQQYrm ||
				Opcode == X86_VPGATHERDDYrm ||
				Opcode == X86_VPGATHERQDYrm ||
#endif
				false
				);
		IndexIs512 = (
#ifndef CAPSTONE_X86_REDUCE
				Opcode == X86_VGATHERQPDZrm ||
				Opcode == X86_VGATHERDPSZrm ||
				Opcode == X86_VGATHERQPSZrm ||
				Opcode == X86_VPGATHERQQZrm ||
				Opcode == X86_VPGATHERDDZrm ||
				Opcode == X86_VPGATHERQDZrm ||
#endif
				false
				);

		if (IndexIs128 || IndexIs256 || IndexIs512) {
			unsigned IndexOffset = insn->sibIndex -
				(insn->addressSize == 8 ? SIB_INDEX_RAX:SIB_INDEX_EAX);
			SIBIndex IndexBase = IndexIs512 ? SIB_INDEX_ZMM0 :
				IndexIs256 ? SIB_INDEX_YMM0 : SIB_INDEX_XMM0;

			insn->sibIndex = (SIBIndex)(IndexBase + (insn->sibIndex == SIB_INDEX_NONE ? 4 : IndexOffset));
		}

		if (insn->sibIndex != SIB_INDEX_NONE) {
			switch (insn->sibIndex) {
				default:
					//debug("Unexpected sibIndex");
					return true;
#define ENTRY(x)                                          \
				case SIB_INDEX_##x:                                 \
					indexReg = X86_##x; break;
					EA_BASES_32BIT
						EA_BASES_64BIT
						REGS_XMM
						REGS_YMM
						REGS_ZMM
#undef ENTRY
			}
		} else {
			indexReg = 0;
		}

		scaleAmount = insn->sibScale;
	} else {
		switch (insn->eaBase) {
			case EA_BASE_NONE:
				if (insn->eaDisplacement == EA_DISP_NONE) {
					//debug("EA_BASE_NONE and EA_DISP_NONE for ModR/M base");
					return true;
				}
				if (insn->mode == MODE_64BIT) {
					if (insn->prefix3 == 0x67)	// address-size prefix overrides RIP relative addressing
						MCOperand_CreateReg0(mcInst, X86_EIP);
					else
						MCOperand_CreateReg0(mcInst, X86_RIP); // Section 2.2.1.6
				} else {
					MCOperand_CreateReg0(mcInst, 0);
				}

				indexReg = 0;
				break;
			case EA_BASE_BX_SI:
				MCOperand_CreateReg0(mcInst, X86_BX);
				indexReg = X86_SI;
				break;
			case EA_BASE_BX_DI:
				MCOperand_CreateReg0(mcInst, X86_BX);
				indexReg = X86_DI;
				break;
			case EA_BASE_BP_SI:
				MCOperand_CreateReg0(mcInst, X86_BP);
				indexReg = X86_SI;
				break;
			case EA_BASE_BP_DI:
				MCOperand_CreateReg0(mcInst, X86_BP);
				indexReg = X86_DI;
				break;
			default:
				indexReg = 0;
				switch (insn->eaBase) {
					default:
						//debug("Unexpected eaBase");
						return true;
						// Here, we will use the fill-ins defined above.  However,
						//   BX_SI, BX_DI, BP_SI, and BP_DI are all handled above and
						//   sib and sib64 were handled in the top-level if, so they're only
						//   placeholders to keep the compiler happy.
#define ENTRY(x)                                        \
					case EA_BASE_##x:                                 \
						  MCOperand_CreateReg0(mcInst, X86_##x); break;
						ALL_EA_BASES
#undef ENTRY
#define ENTRY(x) case EA_REG_##x:
							ALL_REGS
#undef ENTRY
							//debug("A R/M memory operand may not be a register; "
							//      "the base field must be a base.");
							return true;
				}
		}

		scaleAmount = 1;
	}

	MCOperand_CreateImm0(mcInst, scaleAmount);
	MCOperand_CreateReg0(mcInst, indexReg);
	MCOperand_CreateImm0(mcInst, insn->displacement);

	MCOperand_CreateReg0(mcInst, segmentRegnums[insn->segmentOverride]);

	return false;
}

/// translateRM - Translates an operand stored in the R/M (and possibly SIB)
///   byte of an instruction to LLVM form, and appends it to an MCInst.
///
/// @param mcInst       - The MCInst to append to.
/// @param operand      - The operand, as stored in the descriptor table.
/// @param insn         - The instruction to extract Mod, R/M, and SIB fields
///                       from.
/// @return             - 0 on success; nonzero otherwise
static bool translateRM(MCInst *mcInst, const OperandSpecifier *operand,
		InternalInstruction *insn)
{
	switch (operand->type) {
		case TYPE_R8:
		case TYPE_R16:
		case TYPE_R32:
		case TYPE_R64:
		case TYPE_Rv:
		case TYPE_MM64:
		case TYPE_XMM:
		case TYPE_XMM32:
		case TYPE_XMM64:
		case TYPE_XMM128:
		case TYPE_XMM256:
		case TYPE_XMM512:
		case TYPE_VK1:
		case TYPE_VK8:
		case TYPE_VK16:
		case TYPE_DEBUGREG:
		case TYPE_CONTROLREG:
			return translateRMRegister(mcInst, insn);
		case TYPE_M:
		case TYPE_M8:
		case TYPE_M16:
		case TYPE_M32:
		case TYPE_M64:
		case TYPE_M128:
		case TYPE_M256:
		case TYPE_M512:
		case TYPE_Mv:
		case TYPE_M32FP:
		case TYPE_M64FP:
		case TYPE_M80FP:
		case TYPE_M1616:
		case TYPE_M1632:
		case TYPE_M1664:
		case TYPE_LEA:
			return translateRMMemory(mcInst, insn);
		default:
			//debug("Unexpected type for a R/M operand");
			return true;
	}
}

/// translateFPRegister - Translates a stack position on the FPU stack to its
///   LLVM form, and appends it to an MCInst.
///
/// @param mcInst       - The MCInst to append to.
/// @param stackPos     - The stack position to translate.
static void translateFPRegister(MCInst *mcInst, uint8_t stackPos)
{
	MCOperand_CreateReg0(mcInst, X86_ST0 + stackPos);
}

/// translateMaskRegister - Translates a 3-bit mask register number to
///   LLVM form, and appends it to an MCInst.
///
/// @param mcInst       - The MCInst to append to.
/// @param maskRegNum   - Number of mask register from 0 to 7.
/// @return             - false on success; true otherwise.
static bool translateMaskRegister(MCInst *mcInst, uint8_t maskRegNum)
{
	if (maskRegNum >= 8) {
		// debug("Invalid mask register number");
		return true;
	}

	MCOperand_CreateReg0(mcInst, X86_K0 + maskRegNum);

	return false;
}

/// translateOperand - Translates an operand stored in an internal instruction
///   to LLVM's format and appends it to an MCInst.
///
/// @param mcInst       - The MCInst to append to.
/// @param operand      - The operand, as stored in the descriptor table.
/// @param insn         - The internal instruction.
/// @return             - false on success; true otherwise.
static bool translateOperand(MCInst *mcInst, const OperandSpecifier *operand, InternalInstruction *insn)
{
	switch (operand->encoding) {
		case ENCODING_REG:
			translateRegister(mcInst, insn->reg);
			return false;
		case ENCODING_WRITEMASK:
			return translateMaskRegister(mcInst, insn->writemask);
		CASE_ENCODING_RM:
			return translateRM(mcInst, operand, insn);
		case ENCODING_CB:
		case ENCODING_CW:
		case ENCODING_CD:
		case ENCODING_CP:
		case ENCODING_CO:
		case ENCODING_CT:
			//debug("Translation of code offsets isn't supported.");
			return true;
		case ENCODING_IB:
		case ENCODING_IW:
		case ENCODING_ID:
		case ENCODING_IO:
		case ENCODING_Iv:
		case ENCODING_Ia:
			translateImmediate(mcInst, insn->immediates[insn->numImmediatesTranslated++], operand, insn);
			return false;
		case ENCODING_SI:
			return translateSrcIndex(mcInst, insn);
		case ENCODING_DI:
			return translateDstIndex(mcInst, insn);
		case ENCODING_RB:
		case ENCODING_RW:
		case ENCODING_RD:
		case ENCODING_RO:
		case ENCODING_Rv:
			translateRegister(mcInst, insn->opcodeRegister);
			return false;
		case ENCODING_FP:
			translateFPRegister(mcInst, insn->modRM & 7);
			return false;
		case ENCODING_VVVV:
			translateRegister(mcInst, insn->vvvv);
			return false;
		case ENCODING_DUP:
			return translateOperand(mcInst, &insn->operands[operand->type - TYPE_DUP0], insn);
		default:
			//debug("Unhandled operand encoding during translation");
			return true;
	}
}

static bool translateInstruction(MCInst *mcInst, InternalInstruction *insn)
{
	int index;

	if (!insn->spec) {
		//debug("Instruction has no specification");
		return true;
	}

	MCInst_setOpcode(mcInst, insn->instructionID);

	// If when reading the prefix bytes we determined the overlapping 0xf2 or 0xf3
	// prefix bytes should be disassembled as xrelease and xacquire then set the
	// opcode to those instead of the rep and repne opcodes.
#ifndef CAPSTONE_X86_REDUCE
	if (insn->xAcquireRelease) {
		if (MCInst_getOpcode(mcInst) == X86_REP_PREFIX)
			MCInst_setOpcode(mcInst, X86_XRELEASE_PREFIX);
		else if (MCInst_getOpcode(mcInst) == X86_REPNE_PREFIX)
			MCInst_setOpcode(mcInst, X86_XACQUIRE_PREFIX);
	}
#endif

	insn->numImmediatesTranslated = 0;

	for (index = 0; index < X86_MAX_OPERANDS; ++index) {
		if (insn->operands[index].encoding != ENCODING_NONE) {
			if (translateOperand(mcInst, &insn->operands[index], insn)) {
				return true;
			}
		}
	}

	return false;
}

static int reader(const struct reader_info *info, uint8_t *byte, uint64_t address)
{
	if (address - info->offset >= info->size)
		// out of buffer range
		return -1;

	*byte = info->code[address - info->offset];

	return 0;
}

// copy x86 detail information from internal structure to public structure
static void update_pub_insn(cs_insn *pub, InternalInstruction *inter)
{
	if (inter->vectorExtensionType != 0)
		memcpy(pub->detail->x86.opcode, inter->vectorExtensionPrefix, sizeof(pub->detail->x86.opcode));
	else {
		if (inter->twoByteEscape) {
			if (inter->threeByteEscape) {
				pub->detail->x86.opcode[0] = inter->twoByteEscape;
				pub->detail->x86.opcode[1] = inter->threeByteEscape;
				pub->detail->x86.opcode[2] = inter->opcode;
			} else {
				pub->detail->x86.opcode[0] = inter->twoByteEscape;
				pub->detail->x86.opcode[1] = inter->opcode;
			}
		} else {
				pub->detail->x86.opcode[0] = inter->opcode;
		}
	}

	pub->detail->x86.rex = inter->rexPrefix;

	pub->detail->x86.addr_size = inter->addressSize;

	pub->detail->x86.modrm = inter->orgModRM;
	pub->detail->x86.encoding.modrm_offset = inter->modRMOffset;

	pub->detail->x86.sib = inter->sib;
	pub->detail->x86.sib_index = x86_map_sib_index(inter->sibIndex);
	pub->detail->x86.sib_scale = inter->sibScale;
	pub->detail->x86.sib_base = x86_map_sib_base(inter->sibBase);

	pub->detail->x86.disp = inter->displacement;
	if (inter->consumedDisplacement) {
		pub->detail->x86.encoding.disp_offset = inter->displacementOffset;
		pub->detail->x86.encoding.disp_size = inter->displacementSize;
	}

	pub->detail->x86.encoding.imm_offset = inter->immediateOffset;
	if (pub->detail->x86.encoding.imm_size == 0 && inter->immediateOffset != 0)
		pub->detail->x86.encoding.imm_size = inter->immediateSize;
}

void X86_init(MCRegisterInfo *MRI)
{
	/*
	   InitMCRegisterInfo(X86RegDesc, 234,
	   RA, PC,
	   X86MCRegisterClasses, 79,
	   X86RegUnitRoots, 119, X86RegDiffLists, X86RegStrings,
	   X86SubRegIdxLists, 7,
	   X86SubRegIdxRanges, X86RegEncodingTable);
	*/

	MCRegisterInfo_InitMCRegisterInfo(MRI, X86RegDesc, 234,
			0, 0,
			X86MCRegisterClasses, 79,
			0, 0, X86RegDiffLists, 0,
			X86SubRegIdxLists, 7,
			0);
}

// Public interface for the disassembler
bool X86_getInstruction(csh ud, const uint8_t *code, size_t code_len,
		MCInst *instr, uint16_t *size, uint64_t address, void *_info)
{
	cs_struct *handle = (cs_struct *)(uintptr_t)ud;
	InternalInstruction insn = {0};
	struct reader_info info;
	int ret;
	bool result;

	info.code = code;
	info.size = code_len;
	info.offset = address;

	if (instr->flat_insn->detail) {
		// instr->flat_insn->detail initialization: 3 alternatives

		// 1. The whole structure, this is how it's done in other arch disassemblers
		// Probably overkill since cs_detail is huge because of the 36 operands of ARM
		
		//memset(instr->flat_insn->detail, 0, sizeof(cs_detail));

		// 2. Only the part relevant to x86
		memset(instr->flat_insn->detail, 0, offsetof(cs_detail, x86) + sizeof(cs_x86));

		// 3. The relevant part except for x86.operands
		// sizeof(cs_x86) is 0x1c0, sizeof(x86.operands) is 0x180
		// marginally faster, should be okay since x86.op_count is set to 0

		//memset(instr->flat_insn->detail, 0, offsetof(cs_detail, x86)+offsetof(cs_x86, operands));
	}

	if (handle->mode & CS_MODE_16)
		ret = decodeInstruction(&insn,
				reader, &info,
				address,
				MODE_16BIT);
	else if (handle->mode & CS_MODE_32)
		ret = decodeInstruction(&insn,
				reader, &info,
				address,
				MODE_32BIT);
	else
		ret = decodeInstruction(&insn,
				reader, &info,
				address,
				MODE_64BIT);

	if (ret) {
		*size = (uint16_t)(insn.readerCursor - address);
		// handle some special cases here.
		// FIXME: fix this in the next major update.
		switch(*size) {
			default:
				break;
			case 2: {
						unsigned char b1 = 0, b2 = 0;

						reader(&info, &b1, address);
						reader(&info, &b2, address + 1);
						if (b1 == 0x0f && b2 == 0xff) {
							instr->Opcode = X86_UD0;
							instr->OpcodePub = X86_INS_UD0;
							strncpy(instr->assembly, "ud0", 4);
							if (instr->flat_insn->detail) {
								instr->flat_insn->detail->x86.opcode[0] = b1;
								instr->flat_insn->detail->x86.opcode[1] = b2;
							}
							return true;
						}
				}
				return false;
			case 4: {
#ifndef CAPSTONE_X86_REDUCE
						if (handle->mode != CS_MODE_16) {
							unsigned char b1 = 0, b2 = 0, b3 = 0, b4 = 0;

							reader(&info, &b1, address);
							reader(&info, &b2, address + 1);
							reader(&info, &b3, address + 2);
							reader(&info, &b4, address + 3);

							if (b1 == 0xf3 && b2 == 0x0f && b3 == 0x1e && b4 == 0xfa) {
								instr->Opcode = X86_ENDBR64;
								instr->OpcodePub = X86_INS_ENDBR64;
								strncpy(instr->assembly, "endbr64", 8);
								if (instr->flat_insn->detail) {
									instr->flat_insn->detail->x86.opcode[0] = b1;
									instr->flat_insn->detail->x86.opcode[1] = b2;
									instr->flat_insn->detail->x86.opcode[2] = b3;
									instr->flat_insn->detail->x86.opcode[3] = b4;
								}
								return true;
							} else if (b1 == 0xf3 && b2 == 0x0f && b3 == 0x1e && b4 == 0xfb) {
								instr->Opcode = X86_ENDBR32;
								instr->OpcodePub = X86_INS_ENDBR32;
								strncpy(instr->assembly, "endbr32", 8);
								if (instr->flat_insn->detail) {
									instr->flat_insn->detail->x86.opcode[0] = b1;
									instr->flat_insn->detail->x86.opcode[1] = b2;
									instr->flat_insn->detail->x86.opcode[2] = b3;
									instr->flat_insn->detail->x86.opcode[3] = b4;
								}
								return true;
							}
						}
#endif
				}
				return false;
		}

		return false;
	} else {
		*size = (uint16_t)insn.length;

		result = (!translateInstruction(instr, &insn)) ?  true : false;
		if (result) {
			// quick fix for #904. TODO: fix this properly in the next update
			if (handle->mode & CS_MODE_64) {
				if (instr->Opcode == X86_LES16rm || instr->Opcode == X86_LES32rm)
					// LES is invalid in x64
					return false;
				if (instr->Opcode == X86_LDS16rm || instr->Opcode == X86_LDS32rm)
					// LDS is invalid in x64
					return false;
			}

			instr->imm_size = insn.immSize;

			// copy all prefixes
			instr->x86_prefix[0] = insn.prefix0;
			instr->x86_prefix[1] = insn.prefix1;
			instr->x86_prefix[2] = insn.prefix2;
			instr->x86_prefix[3] = insn.prefix3;
			instr->xAcquireRelease = insn.xAcquireRelease;

			if (handle->detail) {
				update_pub_insn(instr->flat_insn, &insn);
			}
		}

		return result;
	}
}

#endif
