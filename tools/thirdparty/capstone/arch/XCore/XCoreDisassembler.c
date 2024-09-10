//===------ XCoreDisassembler.cpp - Disassembler for PowerPC ------*- C++ -*-===//
//
//                     The LLVM Compiler Infrastructure
//
// This file is distributed under the University of Illinois Open Source
// License. See LICENSE.TXT for details.
//
//===----------------------------------------------------------------------===//

/* Capstone Disassembly Engine */
/* By Nguyen Anh Quynh <aquynh@gmail.com>, 2013-2015 */

#ifdef CAPSTONE_HAS_XCORE

#include <stdio.h>	// DEBUG
#include <stdlib.h>
#include <string.h>

#include "../../cs_priv.h"
#include "../../utils.h"

#include "XCoreDisassembler.h"

#include "../../MCInst.h"
#include "../../MCInstrDesc.h"
#include "../../MCFixedLenDisassembler.h"
#include "../../MCRegisterInfo.h"
#include "../../MCDisassembler.h"
#include "../../MathExtras.h"

static uint64_t getFeatureBits(int mode)
{
	// support everything
	return (uint64_t)-1;
}

static bool readInstruction16(const uint8_t *code, size_t code_len, uint16_t *insn)
{
	if (code_len < 2)
		// insufficient data
		return false;

	// Encoded as a little-endian 16-bit word in the stream.
	*insn = (code[0] <<  0) | (code[1] <<  8);
	return true;
}

static bool readInstruction32(const uint8_t *code, size_t code_len, uint32_t *insn)
{
	if (code_len < 4)
		// insufficient data
		return false;

	// Encoded as a little-endian 32-bit word in the stream.
	*insn = (code[0] << 0) | (code[1] << 8) | (code[2] << 16) | ((uint32_t) code[3] << 24);

	return true;
}

static unsigned getReg(const MCRegisterInfo *MRI, unsigned RC, unsigned RegNo)
{
	const MCRegisterClass *rc = MCRegisterInfo_getRegClass(MRI, RC);
	return rc->RegsBegin[RegNo];
}

static DecodeStatus DecodeGRRegsRegisterClass(MCInst *Inst, unsigned RegNo,
		uint64_t Address, const void *Decoder);

static DecodeStatus DecodeRRegsRegisterClass(MCInst *Inst, unsigned RegNo,
		uint64_t Address, const void *Decoder);

static DecodeStatus DecodeBitpOperand(MCInst *Inst, unsigned Val,
		uint64_t Address, const void *Decoder);

static DecodeStatus DecodeNegImmOperand(MCInst *Inst, unsigned Val,
		uint64_t Address, const void *Decoder);

static DecodeStatus Decode2RInstruction(MCInst *Inst, unsigned Insn,
		uint64_t Address, const void *Decoder);

static DecodeStatus Decode2RImmInstruction(MCInst *Inst, unsigned Insn,
		uint64_t Address, const void *Decoder);

static DecodeStatus DecodeR2RInstruction(MCInst *Inst, unsigned Insn,
		uint64_t Address, const void *Decoder);

static DecodeStatus Decode2RSrcDstInstruction(MCInst *Inst, unsigned Insn,
		uint64_t Address, const void *Decoder);

static DecodeStatus DecodeRUSInstruction(MCInst *Inst, unsigned Insn,
		uint64_t Address, const void *Decoder);

static DecodeStatus DecodeRUSBitpInstruction(MCInst *Inst, unsigned Insn,
		uint64_t Address, const void *Decoder);

static DecodeStatus DecodeRUSSrcDstBitpInstruction(MCInst *Inst, unsigned Insn,
		uint64_t Address, const void *Decoder);

static DecodeStatus DecodeL2RInstruction(MCInst *Inst, unsigned Insn,
		uint64_t Address, const void *Decoder);

static DecodeStatus DecodeLR2RInstruction(MCInst *Inst, unsigned Insn,
		uint64_t Address, const void *Decoder);

static DecodeStatus Decode3RInstruction(MCInst *Inst, unsigned Insn,
		uint64_t Address, const void *Decoder);

static DecodeStatus Decode3RImmInstruction(MCInst *Inst, unsigned Insn,
		uint64_t Address, const void *Decoder);

static DecodeStatus Decode2RUSInstruction(MCInst *Inst, unsigned Insn,
		uint64_t Address, const void *Decoder);

static DecodeStatus Decode2RUSBitpInstruction(MCInst *Inst, unsigned Insn,
		uint64_t Address, const void *Decoder);

static DecodeStatus DecodeL3RInstruction(MCInst *Inst, unsigned Insn,
		uint64_t Address, const void *Decoder);

static DecodeStatus DecodeL3RSrcDstInstruction(MCInst *Inst, unsigned Insn,
		uint64_t Address, const void *Decoder);

static DecodeStatus DecodeL2RUSInstruction(MCInst *Inst, unsigned Insn,
		uint64_t Address, const void *Decoder);

static DecodeStatus DecodeL2RUSBitpInstruction(MCInst *Inst, unsigned Insn,
		uint64_t Address, const void *Decoder);

static DecodeStatus DecodeL6RInstruction(MCInst *Inst, unsigned Insn,
		uint64_t Address, const void *Decoder);

static DecodeStatus DecodeL5RInstruction(MCInst *Inst, unsigned Insn,
		uint64_t Address, const void *Decoder);

static DecodeStatus DecodeL4RSrcDstInstruction(MCInst *Inst, unsigned Insn,
		uint64_t Address, const void *Decoder);

static DecodeStatus DecodeL4RSrcDstSrcDstInstruction(MCInst *Inst, unsigned Insn,
		uint64_t Address, const void *Decoder);

#include "XCoreGenDisassemblerTables.inc"

#define GET_REGINFO_ENUM
#define GET_REGINFO_MC_DESC
#include "XCoreGenRegisterInfo.inc"

static DecodeStatus DecodeGRRegsRegisterClass(MCInst *Inst, unsigned RegNo,
		uint64_t Address, const void *Decoder)
{
	unsigned Reg;

	if (RegNo > 11)
		return MCDisassembler_Fail;

	Reg = getReg(Decoder, XCore_GRRegsRegClassID, RegNo);
	MCOperand_CreateReg0(Inst, Reg);

	return MCDisassembler_Success;
}

static DecodeStatus DecodeRRegsRegisterClass(MCInst *Inst, unsigned RegNo,
		uint64_t Address, const void *Decoder)
{
	unsigned Reg;
	if (RegNo > 15)
		return MCDisassembler_Fail;

	Reg = getReg(Decoder, XCore_RRegsRegClassID, RegNo);
	MCOperand_CreateReg0(Inst, Reg);

	return MCDisassembler_Success;
}

static DecodeStatus DecodeBitpOperand(MCInst *Inst, unsigned Val,
		uint64_t Address, const void *Decoder)
{
	static const unsigned Values[] = {
		32 /*bpw*/, 1, 2, 3, 4, 5, 6, 7, 8, 16, 24, 32
	};

	if (Val > 11)
		return MCDisassembler_Fail;

	MCOperand_CreateImm0(Inst, Values[Val]);
	return MCDisassembler_Success;
}

static DecodeStatus DecodeNegImmOperand(MCInst *Inst, unsigned Val,
		uint64_t Address, const void *Decoder)
{
	MCOperand_CreateImm0(Inst, -(int64_t)Val);
	return MCDisassembler_Success;
}

static DecodeStatus Decode2OpInstruction(unsigned Insn, unsigned *Op1, unsigned *Op2)
{
	unsigned Op1High, Op2High;
	unsigned Combined = fieldFromInstruction_4(Insn, 6, 5);

	if (Combined < 27)
		return MCDisassembler_Fail;

	if (fieldFromInstruction_4(Insn, 5, 1)) {
		if (Combined == 31)
			return MCDisassembler_Fail;
		Combined += 5;
	}

	Combined -= 27;
	Op1High = Combined % 3;
	Op2High = Combined / 3;
	*Op1 = (Op1High << 2) | fieldFromInstruction_4(Insn, 2, 2);
	*Op2 = (Op2High << 2) | fieldFromInstruction_4(Insn, 0, 2);

	return MCDisassembler_Success;
}

static DecodeStatus Decode3OpInstruction(unsigned Insn,
		unsigned *Op1, unsigned *Op2, unsigned *Op3)
{
	unsigned Op1High, Op2High, Op3High;
	unsigned Combined = fieldFromInstruction_4(Insn, 6, 5);
	if (Combined >= 27)
		return MCDisassembler_Fail;

	Op1High = Combined % 3;
	Op2High = (Combined / 3) % 3;
	Op3High = Combined / 9;
	*Op1 = (Op1High << 2) | fieldFromInstruction_4(Insn, 4, 2);
	*Op2 = (Op2High << 2) | fieldFromInstruction_4(Insn, 2, 2);
	*Op3 = (Op3High << 2) | fieldFromInstruction_4(Insn, 0, 2);

	return MCDisassembler_Success;
}

#define GET_INSTRINFO_ENUM
#include "XCoreGenInstrInfo.inc"
static DecodeStatus Decode2OpInstructionFail(MCInst *Inst, unsigned Insn, uint64_t Address,
		const void *Decoder)
{
	// Try and decode as a 3R instruction.
	unsigned Opcode = fieldFromInstruction_4(Insn, 11, 5);
	switch (Opcode) {
		case 0x0:
			MCInst_setOpcode(Inst, XCore_STW_2rus);
			return Decode2RUSInstruction(Inst, Insn, Address, Decoder);
		case 0x1:
			MCInst_setOpcode(Inst, XCore_LDW_2rus);
			return Decode2RUSInstruction(Inst, Insn, Address, Decoder);
		case 0x2:
			MCInst_setOpcode(Inst, XCore_ADD_3r);
			return Decode3RInstruction(Inst, Insn, Address, Decoder);
		case 0x3:
			MCInst_setOpcode(Inst, XCore_SUB_3r);
			return Decode3RInstruction(Inst, Insn, Address, Decoder);
		case 0x4:
			MCInst_setOpcode(Inst, XCore_SHL_3r);
			return Decode3RInstruction(Inst, Insn, Address, Decoder);
		case 0x5:
			MCInst_setOpcode(Inst, XCore_SHR_3r);
			return Decode3RInstruction(Inst, Insn, Address, Decoder);
		case 0x6:
			MCInst_setOpcode(Inst, XCore_EQ_3r);
			return Decode3RInstruction(Inst, Insn, Address, Decoder);
		case 0x7:
			MCInst_setOpcode(Inst, XCore_AND_3r);
			return Decode3RInstruction(Inst, Insn, Address, Decoder);
		case 0x8:
			MCInst_setOpcode(Inst, XCore_OR_3r);
			return Decode3RInstruction(Inst, Insn, Address, Decoder);
		case 0x9:
			MCInst_setOpcode(Inst, XCore_LDW_3r);
			return Decode3RInstruction(Inst, Insn, Address, Decoder);
		case 0x10:
			MCInst_setOpcode(Inst, XCore_LD16S_3r);
			return Decode3RInstruction(Inst, Insn, Address, Decoder);
		case 0x11:
			MCInst_setOpcode(Inst, XCore_LD8U_3r);
			return Decode3RInstruction(Inst, Insn, Address, Decoder);
		case 0x12:
			MCInst_setOpcode(Inst, XCore_ADD_2rus);
			return Decode2RUSInstruction(Inst, Insn, Address, Decoder);
		case 0x13:
			MCInst_setOpcode(Inst, XCore_SUB_2rus);
			return Decode2RUSInstruction(Inst, Insn, Address, Decoder);
		case 0x14:
			MCInst_setOpcode(Inst, XCore_SHL_2rus);
			return Decode2RUSBitpInstruction(Inst, Insn, Address, Decoder);
		case 0x15:
			MCInst_setOpcode(Inst, XCore_SHR_2rus);
			return Decode2RUSBitpInstruction(Inst, Insn, Address, Decoder);
		case 0x16:
			MCInst_setOpcode(Inst, XCore_EQ_2rus);
			return Decode2RUSInstruction(Inst, Insn, Address, Decoder);
		case 0x17:
			MCInst_setOpcode(Inst, XCore_TSETR_3r);
			return Decode3RImmInstruction(Inst, Insn, Address, Decoder);
		case 0x18:
			MCInst_setOpcode(Inst, XCore_LSS_3r);
			return Decode3RInstruction(Inst, Insn, Address, Decoder);
		case 0x19:
			MCInst_setOpcode(Inst, XCore_LSU_3r);
			return Decode3RInstruction(Inst, Insn, Address, Decoder);
	}

	return MCDisassembler_Fail;
}

static DecodeStatus Decode2RInstruction(MCInst *Inst, unsigned Insn, uint64_t Address,
		const void *Decoder)
{
	unsigned Op1, Op2;
	DecodeStatus S = Decode2OpInstruction(Insn, &Op1, &Op2);
	if (S != MCDisassembler_Success)
		return Decode2OpInstructionFail(Inst, Insn, Address, Decoder);

	DecodeGRRegsRegisterClass(Inst, Op1, Address, Decoder);
	DecodeGRRegsRegisterClass(Inst, Op2, Address, Decoder);

	return S;
}

static DecodeStatus Decode2RImmInstruction(MCInst *Inst, unsigned Insn, uint64_t Address,
		const void *Decoder)
{
	unsigned Op1, Op2;
	DecodeStatus S = Decode2OpInstruction(Insn, &Op1, &Op2);
	if (S != MCDisassembler_Success)
		return Decode2OpInstructionFail(Inst, Insn, Address, Decoder);

	MCOperand_CreateImm0(Inst, Op1);
	DecodeGRRegsRegisterClass(Inst, Op2, Address, Decoder);

	return S;
}

static DecodeStatus DecodeR2RInstruction(MCInst *Inst, unsigned Insn, uint64_t Address,
		const void *Decoder)
{
	unsigned Op1, Op2;
	DecodeStatus S = Decode2OpInstruction(Insn, &Op2, &Op1);
	if (S != MCDisassembler_Success)
		return Decode2OpInstructionFail(Inst, Insn, Address, Decoder);

	DecodeGRRegsRegisterClass(Inst, Op1, Address, Decoder);
	DecodeGRRegsRegisterClass(Inst, Op2, Address, Decoder);

	return S;
}

static DecodeStatus Decode2RSrcDstInstruction(MCInst *Inst, unsigned Insn, uint64_t Address,
		const void *Decoder)
{
	unsigned Op1, Op2;
	DecodeStatus S = Decode2OpInstruction(Insn, &Op1, &Op2);
	if (S != MCDisassembler_Success)
		return Decode2OpInstructionFail(Inst, Insn, Address, Decoder);

	DecodeGRRegsRegisterClass(Inst, Op1, Address, Decoder);
	DecodeGRRegsRegisterClass(Inst, Op1, Address, Decoder);
	DecodeGRRegsRegisterClass(Inst, Op2, Address, Decoder);

	return S;
}

static DecodeStatus DecodeRUSInstruction(MCInst *Inst, unsigned Insn, uint64_t Address,
		const void *Decoder)
{
	unsigned Op1, Op2;
	DecodeStatus S = Decode2OpInstruction(Insn, &Op1, &Op2);
	if (S != MCDisassembler_Success)
		return Decode2OpInstructionFail(Inst, Insn, Address, Decoder);

	DecodeGRRegsRegisterClass(Inst, Op1, Address, Decoder);
	MCOperand_CreateImm0(Inst, Op2);

	return S;
}

static DecodeStatus DecodeRUSBitpInstruction(MCInst *Inst, unsigned Insn, uint64_t Address,
		const void *Decoder)
{
	unsigned Op1, Op2;
	DecodeStatus S = Decode2OpInstruction(Insn, &Op1, &Op2);
	if (S != MCDisassembler_Success)
		return Decode2OpInstructionFail(Inst, Insn, Address, Decoder);

	DecodeGRRegsRegisterClass(Inst, Op1, Address, Decoder);
	DecodeBitpOperand(Inst, Op2, Address, Decoder);

	return S;
}

static DecodeStatus DecodeRUSSrcDstBitpInstruction(MCInst *Inst, unsigned Insn, uint64_t Address,
		const void *Decoder)
{
	unsigned Op1, Op2;
	DecodeStatus S = Decode2OpInstruction(Insn, &Op1, &Op2);
	if (S != MCDisassembler_Success)
		return Decode2OpInstructionFail(Inst, Insn, Address, Decoder);

	DecodeGRRegsRegisterClass(Inst, Op1, Address, Decoder);
	DecodeGRRegsRegisterClass(Inst, Op1, Address, Decoder);
	DecodeBitpOperand(Inst, Op2, Address, Decoder);

	return S;
}

static DecodeStatus DecodeL2OpInstructionFail(MCInst *Inst, unsigned Insn, uint64_t Address,
		const void *Decoder)
{
	// Try and decode as a L3R / L2RUS instruction.
	unsigned Opcode = fieldFromInstruction_4(Insn, 16, 4) |
		fieldFromInstruction_4(Insn, 27, 5) << 4;
	switch (Opcode) {
		case 0x0c:
			MCInst_setOpcode(Inst, XCore_STW_l3r);
			return DecodeL3RInstruction(Inst, Insn, Address, Decoder);
		case 0x1c:
			MCInst_setOpcode(Inst, XCore_XOR_l3r);
			return DecodeL3RInstruction(Inst, Insn, Address, Decoder);
		case 0x2c:
			MCInst_setOpcode(Inst, XCore_ASHR_l3r);
			return DecodeL3RInstruction(Inst, Insn, Address, Decoder);
		case 0x3c:
			MCInst_setOpcode(Inst, XCore_LDAWF_l3r);
			return DecodeL3RInstruction(Inst, Insn, Address, Decoder);
		case 0x4c:
			MCInst_setOpcode(Inst, XCore_LDAWB_l3r);
			return DecodeL3RInstruction(Inst, Insn, Address, Decoder);
		case 0x5c:
			MCInst_setOpcode(Inst, XCore_LDA16F_l3r);
			return DecodeL3RInstruction(Inst, Insn, Address, Decoder);
		case 0x6c:
			MCInst_setOpcode(Inst, XCore_LDA16B_l3r);
			return DecodeL3RInstruction(Inst, Insn, Address, Decoder);
		case 0x7c:
			MCInst_setOpcode(Inst, XCore_MUL_l3r);
			return DecodeL3RInstruction(Inst, Insn, Address, Decoder);
		case 0x8c:
			MCInst_setOpcode(Inst, XCore_DIVS_l3r);
			return DecodeL3RInstruction(Inst, Insn, Address, Decoder);
		case 0x9c:
			MCInst_setOpcode(Inst, XCore_DIVU_l3r);
			return DecodeL3RInstruction(Inst, Insn, Address, Decoder);
		case 0x10c:
			MCInst_setOpcode(Inst, XCore_ST16_l3r);
			return DecodeL3RInstruction(Inst, Insn, Address, Decoder);
		case 0x11c:
			MCInst_setOpcode(Inst, XCore_ST8_l3r);
			return DecodeL3RInstruction(Inst, Insn, Address, Decoder);
		case 0x12c:
			MCInst_setOpcode(Inst, XCore_ASHR_l2rus);
			return DecodeL2RUSBitpInstruction(Inst, Insn, Address, Decoder);
		case 0x12d:
			MCInst_setOpcode(Inst, XCore_OUTPW_l2rus);
			return DecodeL2RUSBitpInstruction(Inst, Insn, Address, Decoder);
		case 0x12e:
			MCInst_setOpcode(Inst, XCore_INPW_l2rus);
			return DecodeL2RUSBitpInstruction(Inst, Insn, Address, Decoder);
		case 0x13c:
			MCInst_setOpcode(Inst, XCore_LDAWF_l2rus);
			return DecodeL2RUSInstruction(Inst, Insn, Address, Decoder);
		case 0x14c:
			MCInst_setOpcode(Inst, XCore_LDAWB_l2rus);
			return DecodeL2RUSInstruction(Inst, Insn, Address, Decoder);
		case 0x15c:
			MCInst_setOpcode(Inst, XCore_CRC_l3r);
			return DecodeL3RSrcDstInstruction(Inst, Insn, Address, Decoder);
		case 0x18c:
			MCInst_setOpcode(Inst, XCore_REMS_l3r);
			return DecodeL3RInstruction(Inst, Insn, Address, Decoder);
		case 0x19c:
			MCInst_setOpcode(Inst, XCore_REMU_l3r);
			return DecodeL3RInstruction(Inst, Insn, Address, Decoder);
	}

	return MCDisassembler_Fail;
}

static DecodeStatus DecodeL2RInstruction(MCInst *Inst, unsigned Insn, uint64_t Address,
		const void *Decoder)
{
	unsigned Op1, Op2;
	DecodeStatus S = Decode2OpInstruction(fieldFromInstruction_4(Insn, 0, 16), &Op1, &Op2);
	if (S != MCDisassembler_Success)
		return DecodeL2OpInstructionFail(Inst, Insn, Address, Decoder);

	DecodeGRRegsRegisterClass(Inst, Op1, Address, Decoder);
	DecodeGRRegsRegisterClass(Inst, Op2, Address, Decoder);

	return S;
}

static DecodeStatus DecodeLR2RInstruction(MCInst *Inst, unsigned Insn, uint64_t Address,
		const void *Decoder)
{
	unsigned Op1, Op2;
	DecodeStatus S = Decode2OpInstruction(fieldFromInstruction_4(Insn, 0, 16), &Op1, &Op2);
	if (S != MCDisassembler_Success)
		return DecodeL2OpInstructionFail(Inst, Insn, Address, Decoder);

	DecodeGRRegsRegisterClass(Inst, Op2, Address, Decoder);
	DecodeGRRegsRegisterClass(Inst, Op1, Address, Decoder);

	return S;
}

static DecodeStatus Decode3RInstruction(MCInst *Inst, unsigned Insn, uint64_t Address,
		const void *Decoder)
{
	unsigned Op1, Op2, Op3;
	DecodeStatus S = Decode3OpInstruction(Insn, &Op1, &Op2, &Op3);
	if (S == MCDisassembler_Success) {
		DecodeGRRegsRegisterClass(Inst, Op1, Address, Decoder);
		DecodeGRRegsRegisterClass(Inst, Op2, Address, Decoder);
		DecodeGRRegsRegisterClass(Inst, Op3, Address, Decoder);
	}

	return S;
}

static DecodeStatus Decode3RImmInstruction(MCInst *Inst, unsigned Insn, uint64_t Address,
		const void *Decoder)
{
	unsigned Op1, Op2, Op3;
	DecodeStatus S = Decode3OpInstruction(Insn, &Op1, &Op2, &Op3);
	if (S == MCDisassembler_Success) {
		MCOperand_CreateImm0(Inst, Op1);
		DecodeGRRegsRegisterClass(Inst, Op2, Address, Decoder);
		DecodeGRRegsRegisterClass(Inst, Op3, Address, Decoder);
	}

	return S;
}

static DecodeStatus Decode2RUSInstruction(MCInst *Inst, unsigned Insn, uint64_t Address,
		const void *Decoder)
{
	unsigned Op1, Op2, Op3;
	DecodeStatus S = Decode3OpInstruction(Insn, &Op1, &Op2, &Op3);
	if (S == MCDisassembler_Success) {
		DecodeGRRegsRegisterClass(Inst, Op1, Address, Decoder);
		DecodeGRRegsRegisterClass(Inst, Op2, Address, Decoder);
		MCOperand_CreateImm0(Inst, Op3);
	}

	return S;
}

static DecodeStatus Decode2RUSBitpInstruction(MCInst *Inst, unsigned Insn, uint64_t Address,
		const void *Decoder)
{
	unsigned Op1, Op2, Op3;
	DecodeStatus S = Decode3OpInstruction(Insn, &Op1, &Op2, &Op3);
	if (S == MCDisassembler_Success) {
		DecodeGRRegsRegisterClass(Inst, Op1, Address, Decoder);
		DecodeGRRegsRegisterClass(Inst, Op2, Address, Decoder);
		DecodeBitpOperand(Inst, Op3, Address, Decoder);
	}

	return S;
}

static DecodeStatus DecodeL3RInstruction(MCInst *Inst, unsigned Insn, uint64_t Address,
		const void *Decoder)
{
	unsigned Op1, Op2, Op3;
	DecodeStatus S =
		Decode3OpInstruction(fieldFromInstruction_4(Insn, 0, 16), &Op1, &Op2, &Op3);
	if (S == MCDisassembler_Success) {
		DecodeGRRegsRegisterClass(Inst, Op1, Address, Decoder);
		DecodeGRRegsRegisterClass(Inst, Op2, Address, Decoder);
		DecodeGRRegsRegisterClass(Inst, Op3, Address, Decoder);
	}

	return S;
}

static DecodeStatus DecodeL3RSrcDstInstruction(MCInst *Inst, unsigned Insn, uint64_t Address,
		const void *Decoder)
{
	unsigned Op1, Op2, Op3;
	DecodeStatus S =
		Decode3OpInstruction(fieldFromInstruction_4(Insn, 0, 16), &Op1, &Op2, &Op3);
	if (S == MCDisassembler_Success) {
		DecodeGRRegsRegisterClass(Inst, Op1, Address, Decoder);
		DecodeGRRegsRegisterClass(Inst, Op1, Address, Decoder);
		DecodeGRRegsRegisterClass(Inst, Op2, Address, Decoder);
		DecodeGRRegsRegisterClass(Inst, Op3, Address, Decoder);
	}

	return S;
}

static DecodeStatus DecodeL2RUSInstruction(MCInst *Inst, unsigned Insn, uint64_t Address,
		const void *Decoder)
{
	unsigned Op1, Op2, Op3;
	DecodeStatus S =
		Decode3OpInstruction(fieldFromInstruction_4(Insn, 0, 16), &Op1, &Op2, &Op3);
	if (S == MCDisassembler_Success) {
		DecodeGRRegsRegisterClass(Inst, Op1, Address, Decoder);
		DecodeGRRegsRegisterClass(Inst, Op2, Address, Decoder);
		MCOperand_CreateImm0(Inst, Op3);
	}

	return S;
}

static DecodeStatus DecodeL2RUSBitpInstruction(MCInst *Inst, unsigned Insn, uint64_t Address,
		const void *Decoder)
{
	unsigned Op1, Op2, Op3;
	DecodeStatus S =
		Decode3OpInstruction(fieldFromInstruction_4(Insn, 0, 16), &Op1, &Op2, &Op3);
	if (S == MCDisassembler_Success) {
		DecodeGRRegsRegisterClass(Inst, Op1, Address, Decoder);
		DecodeGRRegsRegisterClass(Inst, Op2, Address, Decoder);
		DecodeBitpOperand(Inst, Op3, Address, Decoder);
	}

	return S;
}

static DecodeStatus DecodeL6RInstruction(MCInst *Inst, unsigned Insn, uint64_t Address,
		const void *Decoder)
{
	unsigned Op1, Op2, Op3, Op4, Op5, Op6;
	DecodeStatus S =
		Decode3OpInstruction(fieldFromInstruction_4(Insn, 0, 16), &Op1, &Op2, &Op3);
	if (S != MCDisassembler_Success)
		return S;

	S = Decode3OpInstruction(fieldFromInstruction_4(Insn, 16, 16), &Op4, &Op5, &Op6);
	if (S != MCDisassembler_Success)
		return S;

	DecodeGRRegsRegisterClass(Inst, Op1, Address, Decoder);
	DecodeGRRegsRegisterClass(Inst, Op4, Address, Decoder);
	DecodeGRRegsRegisterClass(Inst, Op2, Address, Decoder);
	DecodeGRRegsRegisterClass(Inst, Op3, Address, Decoder);
	DecodeGRRegsRegisterClass(Inst, Op5, Address, Decoder);
	DecodeGRRegsRegisterClass(Inst, Op6, Address, Decoder);
	return S;
}

static DecodeStatus DecodeL5RInstructionFail(MCInst *Inst, unsigned Insn, uint64_t Address,
		const void *Decoder)
{
	unsigned Opcode;

	// Try and decode as a L6R instruction.
	MCInst_clear(Inst);
	Opcode = fieldFromInstruction_4(Insn, 27, 5);
	switch (Opcode) {
		default:
			break;
		case 0x00:
			MCInst_setOpcode(Inst, XCore_LMUL_l6r);
			return DecodeL6RInstruction(Inst, Insn, Address, Decoder);
	}

	return MCDisassembler_Fail;
}

static DecodeStatus DecodeL5RInstruction(MCInst *Inst, unsigned Insn, uint64_t Address,
		const void *Decoder)
{
	unsigned Op1, Op2, Op3, Op4, Op5;
	DecodeStatus S =
		Decode3OpInstruction(fieldFromInstruction_4(Insn, 0, 16), &Op1, &Op2, &Op3);
	if (S != MCDisassembler_Success)
		return DecodeL5RInstructionFail(Inst, Insn, Address, Decoder);

	S = Decode2OpInstruction(fieldFromInstruction_4(Insn, 16, 16), &Op4, &Op5);
	if (S != MCDisassembler_Success)
		return DecodeL5RInstructionFail(Inst, Insn, Address, Decoder);

	DecodeGRRegsRegisterClass(Inst, Op1, Address, Decoder);
	DecodeGRRegsRegisterClass(Inst, Op4, Address, Decoder);
	DecodeGRRegsRegisterClass(Inst, Op2, Address, Decoder);
	DecodeGRRegsRegisterClass(Inst, Op3, Address, Decoder);
	DecodeGRRegsRegisterClass(Inst, Op5, Address, Decoder);
	return S;
}

static DecodeStatus DecodeL4RSrcDstInstruction(MCInst *Inst, unsigned Insn, uint64_t Address,
		const void *Decoder)
{
	unsigned Op1, Op2, Op3;
	unsigned Op4 = fieldFromInstruction_4(Insn, 16, 4);
	DecodeStatus S =
		Decode3OpInstruction(fieldFromInstruction_4(Insn, 0, 16), &Op1, &Op2, &Op3);
	if (S == MCDisassembler_Success) {
		DecodeGRRegsRegisterClass(Inst, Op1, Address, Decoder);
		S = DecodeGRRegsRegisterClass(Inst, Op4, Address, Decoder);
	}

	if (S == MCDisassembler_Success) {
		DecodeGRRegsRegisterClass(Inst, Op4, Address, Decoder);
		DecodeGRRegsRegisterClass(Inst, Op2, Address, Decoder);
		DecodeGRRegsRegisterClass(Inst, Op3, Address, Decoder);
	}
	return S;
}

static DecodeStatus DecodeL4RSrcDstSrcDstInstruction(MCInst *Inst, unsigned Insn, uint64_t Address,
		const void *Decoder)
{
	unsigned Op1, Op2, Op3;
	unsigned Op4 = fieldFromInstruction_4(Insn, 16, 4);
	DecodeStatus S =
		Decode3OpInstruction(fieldFromInstruction_4(Insn, 0, 16), &Op1, &Op2, &Op3);
	if (S == MCDisassembler_Success) {
		DecodeGRRegsRegisterClass(Inst, Op1, Address, Decoder);
		S = DecodeGRRegsRegisterClass(Inst, Op4, Address, Decoder);
	}

	if (S == MCDisassembler_Success) {
		DecodeGRRegsRegisterClass(Inst, Op1, Address, Decoder);
		DecodeGRRegsRegisterClass(Inst, Op4, Address, Decoder);
		DecodeGRRegsRegisterClass(Inst, Op2, Address, Decoder);
		DecodeGRRegsRegisterClass(Inst, Op3, Address, Decoder);
	}

	return S;
}

#define GET_SUBTARGETINFO_ENUM
#include "XCoreGenInstrInfo.inc"
bool XCore_getInstruction(csh ud, const uint8_t *code, size_t code_len, MCInst *MI,
		uint16_t *size, uint64_t address, void *info)
{
	uint16_t insn16;
	uint32_t insn32;
	DecodeStatus Result;

	if (!readInstruction16(code, code_len, &insn16)) {
		return false;
	}

	if (MI->flat_insn->detail) {
		memset(MI->flat_insn->detail, 0, offsetof(cs_detail, xcore)+sizeof(cs_xcore));
	}

	// Calling the auto-generated decoder function.
	Result = decodeInstruction_2(DecoderTable16, MI, insn16, address, info, 0);
	if (Result != MCDisassembler_Fail) {
		*size = 2;
		return true;
	}

	if (!readInstruction32(code, code_len, &insn32)) {
		return false;
	}

	// Calling the auto-generated decoder function.
	Result = decodeInstruction_4(DecoderTable32, MI, insn32, address, info, 0);
	if (Result != MCDisassembler_Fail) {
		*size = 4;
		return true;
	}

	return false;
}

void XCore_init(MCRegisterInfo *MRI)
{
	/*
	InitMCRegisterInfo(XCoreRegDesc, 17, RA, PC,
			XCoreMCRegisterClasses, 2,
			XCoreRegUnitRoots,
			16,
			XCoreRegDiffLists,
			XCoreRegStrings,
			XCoreSubRegIdxLists,
			1,
			XCoreSubRegIdxRanges,
			XCoreRegEncodingTable);
	*/


	MCRegisterInfo_InitMCRegisterInfo(MRI, XCoreRegDesc, 17,
			0, 0,
			XCoreMCRegisterClasses, 2,
			0, 0,
			XCoreRegDiffLists,
			0,
			XCoreSubRegIdxLists, 1,
			0);
}

#endif
