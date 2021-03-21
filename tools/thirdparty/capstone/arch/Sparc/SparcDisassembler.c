//===------ SparcDisassembler.cpp - Disassembler for PowerPC ------*- C++ -*-===//
//
//                     The LLVM Compiler Infrastructure
//
// This file is distributed under the University of Illinois Open Source
// License. See LICENSE.TXT for details.
//
//===----------------------------------------------------------------------===//

/* Capstone Disassembly Engine */
/* By Nguyen Anh Quynh <aquynh@gmail.com>, 2013-2015 */

#ifdef CAPSTONE_HAS_SPARC

#include <stdio.h>	// DEBUG
#include <stdlib.h>
#include <string.h>

#include "../../cs_priv.h"
#include "../../utils.h"

#include "SparcDisassembler.h"

#include "../../MCInst.h"
#include "../../MCInstrDesc.h"
#include "../../MCFixedLenDisassembler.h"
#include "../../MCRegisterInfo.h"
#include "../../MCDisassembler.h"
#include "../../MathExtras.h"


#define GET_REGINFO_MC_DESC
#define GET_REGINFO_ENUM
#include "SparcGenRegisterInfo.inc"
static const unsigned IntRegDecoderTable[] = {
	SP_G0,  SP_G1,  SP_G2,  SP_G3,
	SP_G4,  SP_G5,  SP_G6,  SP_G7,
	SP_O0,  SP_O1,  SP_O2,  SP_O3,
	SP_O4,  SP_O5,  SP_O6,  SP_O7,
	SP_L0,  SP_L1,  SP_L2,  SP_L3,
	SP_L4,  SP_L5,  SP_L6,  SP_L7,
	SP_I0,  SP_I1,  SP_I2,  SP_I3,
	SP_I4,  SP_I5,  SP_I6,  SP_I7
};

static const unsigned FPRegDecoderTable[] = {
	SP_F0,   SP_F1,   SP_F2,   SP_F3,
	SP_F4,   SP_F5,   SP_F6,   SP_F7,
	SP_F8,   SP_F9,   SP_F10,  SP_F11,
	SP_F12,  SP_F13,  SP_F14,  SP_F15,
	SP_F16,  SP_F17,  SP_F18,  SP_F19,
	SP_F20,  SP_F21,  SP_F22,  SP_F23,
	SP_F24,  SP_F25,  SP_F26,  SP_F27,
	SP_F28,  SP_F29,  SP_F30,  SP_F31
};

static const unsigned DFPRegDecoderTable[] = {
	SP_D0,   SP_D16,  SP_D1,   SP_D17,
	SP_D2,   SP_D18,  SP_D3,   SP_D19,
	SP_D4,   SP_D20,  SP_D5,   SP_D21,
	SP_D6,   SP_D22,  SP_D7,   SP_D23,
	SP_D8,   SP_D24,  SP_D9,   SP_D25,
	SP_D10,  SP_D26,  SP_D11,  SP_D27,
	SP_D12,  SP_D28,  SP_D13,  SP_D29,
	SP_D14,  SP_D30,  SP_D15,  SP_D31
};

static const unsigned QFPRegDecoderTable[] = {
	SP_Q0,  SP_Q8,   ~0U,  ~0U,
	SP_Q1,  SP_Q9,   ~0U,  ~0U,
	SP_Q2,  SP_Q10,  ~0U,  ~0U,
	SP_Q3,  SP_Q11,  ~0U,  ~0U,
	SP_Q4,  SP_Q12,  ~0U,  ~0U,
	SP_Q5,  SP_Q13,  ~0U,  ~0U,
	SP_Q6,  SP_Q14,  ~0U,  ~0U,
	SP_Q7,  SP_Q15,  ~0U,  ~0U
};

static const unsigned FCCRegDecoderTable[] = {
	SP_FCC0, SP_FCC1, SP_FCC2, SP_FCC3
};

static uint64_t getFeatureBits(int mode)
{
	// support everything
	return (uint64_t)-1;
}

static DecodeStatus DecodeIntRegsRegisterClass(MCInst *Inst, unsigned RegNo,
		uint64_t Address, const void *Decoder)
{
	unsigned Reg;

	if (RegNo > 31)
		return MCDisassembler_Fail;

	Reg = IntRegDecoderTable[RegNo];
	MCOperand_CreateReg0(Inst, Reg);

	return MCDisassembler_Success;
}

static DecodeStatus DecodeI64RegsRegisterClass(MCInst *Inst, unsigned RegNo,
		uint64_t Address, const void *Decoder)
{
	unsigned Reg;

	if (RegNo > 31)
		return MCDisassembler_Fail;

	Reg = IntRegDecoderTable[RegNo];
	MCOperand_CreateReg0(Inst, Reg);

	return MCDisassembler_Success;
}

static DecodeStatus DecodeFPRegsRegisterClass(MCInst *Inst, unsigned RegNo,
		uint64_t Address, const void *Decoder)
{
	unsigned Reg;

	if (RegNo > 31)
		return MCDisassembler_Fail;

	Reg = FPRegDecoderTable[RegNo];
	MCOperand_CreateReg0(Inst, Reg);

	return MCDisassembler_Success;
}

static DecodeStatus DecodeDFPRegsRegisterClass(MCInst *Inst, unsigned RegNo,
		uint64_t Address, const void *Decoder)
{
	unsigned Reg;

	if (RegNo > 31)
		return MCDisassembler_Fail;

	Reg = DFPRegDecoderTable[RegNo];
	MCOperand_CreateReg0(Inst, Reg);

	return MCDisassembler_Success;
}

static DecodeStatus DecodeQFPRegsRegisterClass(MCInst *Inst, unsigned RegNo,
		uint64_t Address, const void *Decoder)
{
	unsigned Reg;

	if (RegNo > 31)
		return MCDisassembler_Fail;

	Reg = QFPRegDecoderTable[RegNo];
	if (Reg == ~0U)
		return MCDisassembler_Fail;

	MCOperand_CreateReg0(Inst, Reg);

	return MCDisassembler_Success;
}

static DecodeStatus DecodeFCCRegsRegisterClass(MCInst *Inst, unsigned RegNo,
		uint64_t Address, const void *Decoder)
{
	if (RegNo > 3)
		return MCDisassembler_Fail;

	MCOperand_CreateReg0(Inst, FCCRegDecoderTable[RegNo]);

	return MCDisassembler_Success;
}


static DecodeStatus DecodeLoadInt(MCInst *Inst, unsigned insn, uint64_t Address,
		const void *Decoder);
static DecodeStatus DecodeLoadFP(MCInst *Inst, unsigned insn, uint64_t Address,
		const void *Decoder);
static DecodeStatus DecodeLoadDFP(MCInst *Inst, unsigned insn, uint64_t Address,
		const void *Decoder);
static DecodeStatus DecodeLoadQFP(MCInst *Inst, unsigned insn, uint64_t Address,
		const void *Decoder);
static DecodeStatus DecodeStoreInt(MCInst *Inst, unsigned insn,
		uint64_t Address, const void *Decoder);
static DecodeStatus DecodeStoreFP(MCInst *Inst, unsigned insn,
		uint64_t Address, const void *Decoder);
static DecodeStatus DecodeStoreDFP(MCInst *Inst, unsigned insn,
		uint64_t Address, const void *Decoder);
static DecodeStatus DecodeStoreQFP(MCInst *Inst, unsigned insn,
		uint64_t Address, const void *Decoder);
static DecodeStatus DecodeCall(MCInst *Inst, unsigned insn,
		uint64_t Address, const void *Decoder);
static DecodeStatus DecodeSIMM13(MCInst *Inst, unsigned insn,
		uint64_t Address, const void *Decoder);
static DecodeStatus DecodeJMPL(MCInst *Inst, unsigned insn, uint64_t Address,
		const void *Decoder);
static DecodeStatus DecodeReturn(MCInst *MI, unsigned insn, uint64_t Address,
		const void *Decoder);
static DecodeStatus DecodeSWAP(MCInst *Inst, unsigned insn, uint64_t Address,
		const void *Decoder);


#define GET_SUBTARGETINFO_ENUM
#include "SparcGenSubtargetInfo.inc"
#include "SparcGenDisassemblerTables.inc"

/// readInstruction - read four bytes and return 32 bit word.
static DecodeStatus readInstruction32(const uint8_t *code, size_t len, uint32_t *Insn)
{
	if (len < 4)
		// not enough data
		return MCDisassembler_Fail;

	// Encoded as a big-endian 32-bit word in the stream.
	*Insn = (code[3] <<  0) |
		(code[2] <<  8) |
		(code[1] << 16) |
		((uint32_t) code[0] << 24);

	return MCDisassembler_Success;
}

bool Sparc_getInstruction(csh ud, const uint8_t *code, size_t code_len, MCInst *MI,
		uint16_t *size, uint64_t address, void *info)
{
	uint32_t Insn;
	DecodeStatus Result;
	
	Result = readInstruction32(code, code_len, &Insn);
	if (Result == MCDisassembler_Fail)
		return false;

	if (MI->flat_insn->detail) {
		memset(MI->flat_insn->detail, 0, offsetof(cs_detail, sparc)+sizeof(cs_sparc));
	}

	Result = decodeInstruction_4(DecoderTableSparc32, MI, Insn, address,
			(MCRegisterInfo *)info, 0);
	if (Result != MCDisassembler_Fail) {
		*size = 4;
		return true;
	}

	return false;
}

typedef DecodeStatus (*DecodeFunc)(MCInst *MI, unsigned insn, uint64_t Address,
		const void *Decoder);

static DecodeStatus DecodeMem(MCInst *MI, unsigned insn, uint64_t Address,
		const void *Decoder,
		bool isLoad, DecodeFunc DecodeRD)
{
	DecodeStatus status;
	unsigned rd = fieldFromInstruction_4(insn, 25, 5);
	unsigned rs1 = fieldFromInstruction_4(insn, 14, 5);
	bool isImm = fieldFromInstruction_4(insn, 13, 1) != 0;
	unsigned rs2 = 0;
	unsigned simm13 = 0;

	if (isImm)
		simm13 = SignExtend32(fieldFromInstruction_4(insn, 0, 13), 13);
	else
		rs2 = fieldFromInstruction_4(insn, 0, 5);

	if (isLoad) {
		status = DecodeRD(MI, rd, Address, Decoder);
		if (status != MCDisassembler_Success)
			return status;
	}

	// Decode rs1.
	status = DecodeIntRegsRegisterClass(MI, rs1, Address, Decoder);
	if (status != MCDisassembler_Success)
		return status;

	// Decode imm|rs2.
	if (isImm)
		MCOperand_CreateImm0(MI, simm13);
	else {
		status = DecodeIntRegsRegisterClass(MI, rs2, Address, Decoder);
		if (status != MCDisassembler_Success)
			return status;
	}

	if (!isLoad) {
		status = DecodeRD(MI, rd, Address, Decoder);
		if (status != MCDisassembler_Success)
			return status;
	}

	return MCDisassembler_Success;
}

static DecodeStatus DecodeLoadInt(MCInst *Inst, unsigned insn, uint64_t Address,
		const void *Decoder)
{
	return DecodeMem(Inst, insn, Address, Decoder, true,
			DecodeIntRegsRegisterClass);
}

static DecodeStatus DecodeLoadFP(MCInst *Inst, unsigned insn, uint64_t Address,
		const void *Decoder)
{
	return DecodeMem(Inst, insn, Address, Decoder, true,
			DecodeFPRegsRegisterClass);
}

static DecodeStatus DecodeLoadDFP(MCInst *Inst, unsigned insn, uint64_t Address,
		const void *Decoder)
{
	return DecodeMem(Inst, insn, Address, Decoder, true,
			DecodeDFPRegsRegisterClass);
}

static DecodeStatus DecodeLoadQFP(MCInst *Inst, unsigned insn, uint64_t Address,
		const void *Decoder)
{
	return DecodeMem(Inst, insn, Address, Decoder, true,
			DecodeQFPRegsRegisterClass);
}

static DecodeStatus DecodeStoreInt(MCInst *Inst, unsigned insn,
		uint64_t Address, const void *Decoder)
{
	return DecodeMem(Inst, insn, Address, Decoder, false,
			DecodeIntRegsRegisterClass);
}

static DecodeStatus DecodeStoreFP(MCInst *Inst, unsigned insn, uint64_t Address,
		const void *Decoder)
{
	return DecodeMem(Inst, insn, Address, Decoder, false,
			DecodeFPRegsRegisterClass);
}

static DecodeStatus DecodeStoreDFP(MCInst *Inst, unsigned insn,
		uint64_t Address, const void *Decoder)
{
	return DecodeMem(Inst, insn, Address, Decoder, false,
			DecodeDFPRegsRegisterClass);
}

static DecodeStatus DecodeStoreQFP(MCInst *Inst, unsigned insn,
		uint64_t Address, const void *Decoder)
{
	return DecodeMem(Inst, insn, Address, Decoder, false,
			DecodeQFPRegsRegisterClass);
}

static DecodeStatus DecodeCall(MCInst *MI, unsigned insn,
		uint64_t Address, const void *Decoder)
{
	unsigned tgt = fieldFromInstruction_4(insn, 0, 30);
	tgt <<= 2;

	MCOperand_CreateImm0(MI, tgt);

	return MCDisassembler_Success;
}

static DecodeStatus DecodeSIMM13(MCInst *MI, unsigned insn,
		uint64_t Address, const void *Decoder)
{
	unsigned tgt = SignExtend32(fieldFromInstruction_4(insn, 0, 13), 13);

	MCOperand_CreateImm0(MI, tgt);

	return MCDisassembler_Success;
}

static DecodeStatus DecodeJMPL(MCInst *MI, unsigned insn, uint64_t Address,
		const void *Decoder)
{
	DecodeStatus status;
	unsigned rd = fieldFromInstruction_4(insn, 25, 5);
	unsigned rs1 = fieldFromInstruction_4(insn, 14, 5);
	unsigned isImm = fieldFromInstruction_4(insn, 13, 1);
	unsigned rs2 = 0;
	unsigned simm13 = 0;

	if (isImm)
		simm13 = SignExtend32(fieldFromInstruction_4(insn, 0, 13), 13);
	else
		rs2 = fieldFromInstruction_4(insn, 0, 5);

	// Decode RD.
	status = DecodeIntRegsRegisterClass(MI, rd, Address, Decoder);
	if (status != MCDisassembler_Success)
		return status;

	// Decode RS1.
	status = DecodeIntRegsRegisterClass(MI, rs1, Address, Decoder);
	if (status != MCDisassembler_Success)
		return status;

	// Decode RS1 | SIMM13.
	if (isImm)
		MCOperand_CreateImm0(MI, simm13);
	else {
		status = DecodeIntRegsRegisterClass(MI, rs2, Address, Decoder);
		if (status != MCDisassembler_Success)
			return status;
	}

	return MCDisassembler_Success;
}

static DecodeStatus DecodeReturn(MCInst *MI, unsigned insn, uint64_t Address,
		const void *Decoder)
{
	DecodeStatus status;
	unsigned rs1 = fieldFromInstruction_4(insn, 14, 5);
	unsigned isImm = fieldFromInstruction_4(insn, 13, 1);
	unsigned rs2 = 0;
	unsigned simm13 = 0;
	if (isImm)
		simm13 = SignExtend32(fieldFromInstruction_4(insn, 0, 13), 13);
	else
		rs2 = fieldFromInstruction_4(insn, 0, 5);

	// Decode RS1.
	status = DecodeIntRegsRegisterClass(MI, rs1, Address, Decoder);
	if (status != MCDisassembler_Success)
		return status;

	// Decode RS2 | SIMM13.
	if (isImm)
		MCOperand_CreateImm0(MI, simm13);
	else {
		status = DecodeIntRegsRegisterClass(MI, rs2, Address, Decoder);
		if (status != MCDisassembler_Success)
			return status;
	}

	return MCDisassembler_Success;
}

static DecodeStatus DecodeSWAP(MCInst *MI, unsigned insn, uint64_t Address,
		const void *Decoder)
{
	DecodeStatus status;
	unsigned rd = fieldFromInstruction_4(insn, 25, 5);
	unsigned rs1 = fieldFromInstruction_4(insn, 14, 5);
	unsigned isImm = fieldFromInstruction_4(insn, 13, 1);
	unsigned rs2 = 0;
	unsigned simm13 = 0;

	if (isImm)
		simm13 = SignExtend32(fieldFromInstruction_4(insn, 0, 13), 13);
	else
		rs2 = fieldFromInstruction_4(insn, 0, 5);

	// Decode RD.
	status = DecodeIntRegsRegisterClass(MI, rd, Address, Decoder);
	if (status != MCDisassembler_Success)
		return status;

	// Decode RS1.
	status = DecodeIntRegsRegisterClass(MI, rs1, Address, Decoder);
	if (status != MCDisassembler_Success)
		return status;

	// Decode RS1 | SIMM13.
	if (isImm)
		MCOperand_CreateImm0(MI, simm13);
	else {
		status = DecodeIntRegsRegisterClass(MI, rs2, Address, Decoder);
		if (status != MCDisassembler_Success)
			return status;
	}

	return MCDisassembler_Success;
}

void Sparc_init(MCRegisterInfo *MRI)
{
	/*
	InitMCRegisterInfo(SparcRegDesc, 119, RA, PC,
			SparcMCRegisterClasses, 8,
			SparcRegUnitRoots,
			86,
			SparcRegDiffLists,
			SparcRegStrings,
			SparcSubRegIdxLists,
			7,
			SparcSubRegIdxRanges,
			SparcRegEncodingTable);
	*/

	MCRegisterInfo_InitMCRegisterInfo(MRI, SparcRegDesc, 119,
			0, 0,
			SparcMCRegisterClasses, 8,
			0, 0,
			SparcRegDiffLists,
			0,
			SparcSubRegIdxLists, 7,
			0);
}

#endif
