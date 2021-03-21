//===------ SystemZDisassembler.cpp - Disassembler for PowerPC ------*- C++ -*-===//
//
//                     The LLVM Compiler Infrastructure
//
// This file is distributed under the University of Illinois Open Source
// License. See LICENSE.TXT for details.
//
//===----------------------------------------------------------------------===//

/* Capstone Disassembly Engine */
/* By Nguyen Anh Quynh <aquynh@gmail.com>, 2013-2015 */

#ifdef CAPSTONE_HAS_SYSZ

#include <stdio.h>	// DEBUG
#include <stdlib.h>
#include <string.h>

#include "../../cs_priv.h"
#include "../../utils.h"

#include "SystemZDisassembler.h"

#include "../../MCInst.h"
#include "../../MCInstrDesc.h"
#include "../../MCFixedLenDisassembler.h"
#include "../../MCRegisterInfo.h"
#include "../../MCDisassembler.h"
#include "../../MathExtras.h"

#include "SystemZMCTargetDesc.h"

static uint64_t getFeatureBits(int mode)
{
	// support everything
	return (uint64_t)-1;
}

static DecodeStatus decodeRegisterClass(MCInst *Inst, uint64_t RegNo, const unsigned *Regs)
{
	//assert(RegNo < 16 && "Invalid register");
	RegNo = Regs[RegNo];
	if (RegNo == 0)
		return MCDisassembler_Fail;

	MCOperand_CreateReg0(Inst, (unsigned)RegNo);
	return MCDisassembler_Success;
}

static DecodeStatus DecodeGR32BitRegisterClass(MCInst *Inst, uint64_t RegNo,
		uint64_t Address, const void *Decoder)
{
	return decodeRegisterClass(Inst, RegNo, SystemZMC_GR32Regs);
}

static DecodeStatus DecodeGRH32BitRegisterClass(MCInst *Inst, uint64_t RegNo,
		uint64_t Address, const void *Decoder)
{
	return decodeRegisterClass(Inst, RegNo, SystemZMC_GRH32Regs);
}

static DecodeStatus DecodeGR64BitRegisterClass(MCInst *Inst, uint64_t RegNo,
		uint64_t Address, const void *Decoder)
{
	return decodeRegisterClass(Inst, RegNo, SystemZMC_GR64Regs);
}

static DecodeStatus DecodeGR128BitRegisterClass(MCInst *Inst, uint64_t RegNo,
		uint64_t Address, const void *Decoder)
{
	return decodeRegisterClass(Inst, RegNo, SystemZMC_GR128Regs);
}

static DecodeStatus DecodeADDR64BitRegisterClass(MCInst *Inst, uint64_t RegNo,
		uint64_t Address, const void *Decoder)
{
	return decodeRegisterClass(Inst, RegNo, SystemZMC_GR64Regs);
}

static DecodeStatus DecodeFP32BitRegisterClass(MCInst *Inst, uint64_t RegNo,
		uint64_t Address, const void *Decoder) 
{
	return decodeRegisterClass(Inst, RegNo, SystemZMC_FP32Regs);
}

static DecodeStatus DecodeFP64BitRegisterClass(MCInst *Inst, uint64_t RegNo,
		uint64_t Address, const void *Decoder)
{
	return decodeRegisterClass(Inst, RegNo, SystemZMC_FP64Regs);
}

static DecodeStatus DecodeFP128BitRegisterClass(MCInst *Inst, uint64_t RegNo,
		uint64_t Address, const void *Decoder)
{
	return decodeRegisterClass(Inst, RegNo, SystemZMC_FP128Regs);
}

static DecodeStatus decodeUImmOperand(MCInst *Inst, uint64_t Imm)
{
	//assert(isUInt<N>(Imm) && "Invalid immediate");
	MCOperand_CreateImm0(Inst, Imm);
	return MCDisassembler_Success;
}

static DecodeStatus decodeSImmOperand(MCInst *Inst, uint64_t Imm, unsigned N)
{
	//assert(isUInt<N>(Imm) && "Invalid immediate");
	MCOperand_CreateImm0(Inst, SignExtend64(Imm, N));
	return MCDisassembler_Success;
}

static DecodeStatus decodeAccessRegOperand(MCInst *Inst, uint64_t Imm,
		uint64_t Address, const void *Decoder)
{
	return decodeUImmOperand(Inst, Imm);
}

static DecodeStatus decodeU4ImmOperand(MCInst *Inst, uint64_t Imm,
		uint64_t Address, const void *Decoder)
{
	return decodeUImmOperand(Inst, Imm);
}

static DecodeStatus decodeU6ImmOperand(MCInst *Inst, uint64_t Imm,
		uint64_t Address, const void *Decoder)
{
	return decodeUImmOperand(Inst, Imm);
}

static DecodeStatus decodeU8ImmOperand(MCInst *Inst, uint64_t Imm,
		uint64_t Address, const void *Decoder)
{
	return decodeUImmOperand(Inst, Imm);
}

static DecodeStatus decodeU16ImmOperand(MCInst *Inst, uint64_t Imm,
		uint64_t Address, const void *Decoder)
{
	return decodeUImmOperand(Inst, Imm);
}

static DecodeStatus decodeU32ImmOperand(MCInst *Inst, uint64_t Imm,
		uint64_t Address, const void *Decoder)
{
	return decodeUImmOperand(Inst, Imm);
}

static DecodeStatus decodeS8ImmOperand(MCInst *Inst, uint64_t Imm,
		uint64_t Address, const void *Decoder)
{
	return decodeSImmOperand(Inst, Imm, 8);
}

static DecodeStatus decodeS16ImmOperand(MCInst *Inst, uint64_t Imm,
		uint64_t Address, const void *Decoder) 
{
	return decodeSImmOperand(Inst, Imm, 16);
}

static DecodeStatus decodeS32ImmOperand(MCInst *Inst, uint64_t Imm,
		uint64_t Address, const void *Decoder)
{
	return decodeSImmOperand(Inst, Imm, 32);
}

static DecodeStatus decodePCDBLOperand(MCInst *Inst, uint64_t Imm,
		uint64_t Address, unsigned N)
{
	//assert(isUInt<N>(Imm) && "Invalid PC-relative offset");
	MCOperand_CreateImm0(Inst, SignExtend64(Imm, N) * 2 + Address);
	return MCDisassembler_Success;
}

static DecodeStatus decodePC16DBLOperand(MCInst *Inst, uint64_t Imm,
		uint64_t Address, const void *Decoder)
{
	return decodePCDBLOperand(Inst, Imm, Address, 16);
}

static DecodeStatus decodePC32DBLOperand(MCInst *Inst, uint64_t Imm,
		uint64_t Address,
		const void *Decoder)
{
	return decodePCDBLOperand(Inst, Imm, Address, 32);
}

static DecodeStatus decodeBDAddr12Operand(MCInst *Inst, uint64_t Field,
		const unsigned *Regs)
{
	uint64_t Base = Field >> 12;
	uint64_t Disp = Field & 0xfff;
	//assert(Base < 16 && "Invalid BDAddr12");

	MCOperand_CreateReg0(Inst, Base == 0 ? 0 : Regs[Base]);
	MCOperand_CreateImm0(Inst, Disp);

	return MCDisassembler_Success;
}

static DecodeStatus decodeBDAddr20Operand(MCInst *Inst, uint64_t Field,
		const unsigned *Regs)
{
	uint64_t Base = Field >> 20;
	uint64_t Disp = ((Field << 12) & 0xff000) | ((Field >> 8) & 0xfff);
	//assert(Base < 16 && "Invalid BDAddr20");

	MCOperand_CreateReg0(Inst, Base == 0 ? 0 : Regs[Base]);
	MCOperand_CreateImm0(Inst, SignExtend64(Disp, 20));
	return MCDisassembler_Success;
}

static DecodeStatus decodeBDXAddr12Operand(MCInst *Inst, uint64_t Field,
		const unsigned *Regs)
{
	uint64_t Index = Field >> 16;
	uint64_t Base = (Field >> 12) & 0xf;
	uint64_t Disp = Field & 0xfff;

	//assert(Index < 16 && "Invalid BDXAddr12");
	MCOperand_CreateReg0(Inst, Base == 0 ? 0 : Regs[Base]);
	MCOperand_CreateImm0(Inst, Disp);
	MCOperand_CreateReg0(Inst, Index == 0 ? 0 : Regs[Index]);

	return MCDisassembler_Success;
}

static DecodeStatus decodeBDXAddr20Operand(MCInst *Inst, uint64_t Field,
		const unsigned *Regs)
{
	uint64_t Index = Field >> 24;
	uint64_t Base = (Field >> 20) & 0xf;
	uint64_t Disp = ((Field & 0xfff00) >> 8) | ((Field & 0xff) << 12);

	//assert(Index < 16 && "Invalid BDXAddr20");
	MCOperand_CreateReg0(Inst, Base == 0 ? 0 : Regs[Base]);
	MCOperand_CreateImm0(Inst, SignExtend64(Disp, 20));
	MCOperand_CreateReg0(Inst, Index == 0 ? 0 : Regs[Index]);

	return MCDisassembler_Success;
}

static DecodeStatus decodeBDLAddr12Len8Operand(MCInst *Inst, uint64_t Field,
		const unsigned *Regs)
{
	uint64_t Length = Field >> 16;
	uint64_t Base = (Field >> 12) & 0xf;
	uint64_t Disp = Field & 0xfff;
	//assert(Length < 256 && "Invalid BDLAddr12Len8");

	MCOperand_CreateReg0(Inst, Base == 0 ? 0 : Regs[Base]);
	MCOperand_CreateImm0(Inst, Disp);
	MCOperand_CreateImm0(Inst, Length + 1);

	return MCDisassembler_Success;
}

static DecodeStatus decodeBDAddr32Disp12Operand(MCInst *Inst, uint64_t Field,
		uint64_t Address, const void *Decoder)
{
	return decodeBDAddr12Operand(Inst, Field, SystemZMC_GR32Regs);
}

static DecodeStatus decodeBDAddr32Disp20Operand(MCInst *Inst, uint64_t Field,
		uint64_t Address, const void *Decoder)
{
	return decodeBDAddr20Operand(Inst, Field, SystemZMC_GR32Regs);
}

static DecodeStatus decodeBDAddr64Disp12Operand(MCInst *Inst, uint64_t Field,
		uint64_t Address, const void *Decoder)
{
	return decodeBDAddr12Operand(Inst, Field, SystemZMC_GR64Regs);
}

static DecodeStatus decodeBDAddr64Disp20Operand(MCInst *Inst, uint64_t Field,
		uint64_t Address, const void *Decoder)
{
	return decodeBDAddr20Operand(Inst, Field, SystemZMC_GR64Regs);
}

static DecodeStatus decodeBDXAddr64Disp12Operand(MCInst *Inst, uint64_t Field,
		uint64_t Address, const void *Decoder)
{
	return decodeBDXAddr12Operand(Inst, Field, SystemZMC_GR64Regs);
}

static DecodeStatus decodeBDXAddr64Disp20Operand(MCInst *Inst, uint64_t Field,
		uint64_t Address, const void *Decoder)
{
	return decodeBDXAddr20Operand(Inst, Field, SystemZMC_GR64Regs);
}

static DecodeStatus decodeBDLAddr64Disp12Len8Operand(MCInst *Inst, uint64_t Field,
		uint64_t Address, const void *Decoder)
{
	return decodeBDLAddr12Len8Operand(Inst, Field, SystemZMC_GR64Regs);
}

#define GET_SUBTARGETINFO_ENUM
#include "SystemZGenSubtargetInfo.inc"
#include "SystemZGenDisassemblerTables.inc"
bool SystemZ_getInstruction(csh ud, const uint8_t *code, size_t code_len, MCInst *MI,
		uint16_t *size, uint64_t address, void *info)
{
	uint64_t Inst;
	const uint8_t *Table;
	uint16_t I; 

	// The top 2 bits of the first byte specify the size.
	if (*code < 0x40) {
		*size = 2;
		Table = DecoderTable16;
	} else if (*code < 0xc0) {
		*size = 4;
		Table = DecoderTable32;
	} else {
		*size = 6;
		Table = DecoderTable48;
	}

	if (code_len < *size)
		// short of input data
		return false;

	if (MI->flat_insn->detail) {
		memset(MI->flat_insn->detail, 0, offsetof(cs_detail, sysz)+sizeof(cs_sysz));
	}

	// Construct the instruction.
	Inst = 0;
	for (I = 0; I < *size; ++I)
		Inst = (Inst << 8) | code[I];

	return decodeInstruction(Table, MI, Inst, address, info, 0);
}

#define GET_REGINFO_ENUM
#define GET_REGINFO_MC_DESC
#include "SystemZGenRegisterInfo.inc"
void SystemZ_init(MCRegisterInfo *MRI)
{
	/*
	InitMCRegisterInfo(SystemZRegDesc, 98, RA, PC,
			SystemZMCRegisterClasses, 12,
			SystemZRegUnitRoots,
			49,
			SystemZRegDiffLists,
			SystemZRegStrings,
			SystemZSubRegIdxLists,
			7,
			SystemZSubRegIdxRanges,
			SystemZRegEncodingTable);
	*/

	MCRegisterInfo_InitMCRegisterInfo(MRI, SystemZRegDesc, 98,
			0, 0,
			SystemZMCRegisterClasses, 12,
			0, 0,
			SystemZRegDiffLists,
			0,
			SystemZSubRegIdxLists, 7,
			0);
}

#endif
