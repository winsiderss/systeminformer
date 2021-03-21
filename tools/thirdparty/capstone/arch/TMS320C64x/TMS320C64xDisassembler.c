/* Capstone Disassembly Engine */
/* TMS320C64x Backend by Fotis Loukos <me@fotisl.com> 2016 */

#ifdef CAPSTONE_HAS_TMS320C64X

#include <string.h>

#include "../../cs_priv.h"
#include "../../utils.h"

#include "TMS320C64xDisassembler.h"

#include "../../MCInst.h"
#include "../../MCInstrDesc.h"
#include "../../MCFixedLenDisassembler.h"
#include "../../MCRegisterInfo.h"
#include "../../MCDisassembler.h"
#include "../../MathExtras.h"

static uint64_t getFeatureBits(int mode);

static DecodeStatus DecodeGPRegsRegisterClass(MCInst *Inst, unsigned RegNo,
		uint64_t Address, void *Decoder);

static DecodeStatus DecodeControlRegsRegisterClass(MCInst *Inst, unsigned RegNo,
		uint64_t Address, void *Decoder);

static DecodeStatus DecodeScst5(MCInst *Inst, unsigned Val,
		uint64_t Address, void *Decoder);

static DecodeStatus DecodeScst16(MCInst *Inst, unsigned Val,
		uint64_t Address, void *Decoder);

static DecodeStatus DecodePCRelScst7(MCInst *Inst, unsigned Val,
		uint64_t Address, void *Decoder);

static DecodeStatus DecodePCRelScst10(MCInst *Inst, unsigned Val,
		uint64_t Address, void *Decoder);

static DecodeStatus DecodePCRelScst12(MCInst *Inst, unsigned Val,
		uint64_t Address, void *Decoder);

static DecodeStatus DecodePCRelScst21(MCInst *Inst, unsigned Val,
		uint64_t Address, void *Decoder);

static DecodeStatus DecodeMemOperand(MCInst *Inst, unsigned Val,
		uint64_t Address, void *Decoder);

static DecodeStatus DecodeMemOperandSc(MCInst *Inst, unsigned Val,
		uint64_t Address, void *Decoder);

static DecodeStatus DecodeMemOperand2(MCInst *Inst, unsigned Val,
		uint64_t Address, void *Decoder);

static DecodeStatus DecodeRegPair5(MCInst *Inst, unsigned RegNo,
		uint64_t Address, void *Decoder);

static DecodeStatus DecodeRegPair4(MCInst *Inst, unsigned RegNo,
		uint64_t Address, void *Decoder);

static DecodeStatus DecodeCondRegister(MCInst *Inst, unsigned Val,
		uint64_t Address, void *Decoder);

static DecodeStatus DecodeCondRegisterZero(MCInst *Inst, unsigned Val,
		uint64_t Address, void *Decoder);

static DecodeStatus DecodeSide(MCInst *Inst, unsigned Val,
		uint64_t Address, void *Decoder);

static DecodeStatus DecodeParallel(MCInst *Inst, unsigned Val,
		uint64_t Address, void *Decoder);

static DecodeStatus DecodeCrosspathX1(MCInst *Inst, unsigned Val,
		uint64_t Address, void *Decoder);

static DecodeStatus DecodeCrosspathX2(MCInst *Inst, unsigned Val,
		uint64_t Address, void *Decoder);

static DecodeStatus DecodeCrosspathX3(MCInst *Inst, unsigned Val,
		uint64_t Address, void *Decoder);

static DecodeStatus DecodeNop(MCInst *Inst, unsigned Val,
		uint64_t Address, void *Decoder);

#include "TMS320C64xGenDisassemblerTables.inc"

#define GET_REGINFO_ENUM
#define GET_REGINFO_MC_DESC
#include "TMS320C64xGenRegisterInfo.inc"

static const unsigned GPRegsDecoderTable[] = {
	TMS320C64x_A0,  TMS320C64x_A1,  TMS320C64x_A2,  TMS320C64x_A3,
	TMS320C64x_A4,  TMS320C64x_A5,  TMS320C64x_A6,  TMS320C64x_A7,
	TMS320C64x_A8,  TMS320C64x_A9,  TMS320C64x_A10, TMS320C64x_A11,
	TMS320C64x_A12, TMS320C64x_A13, TMS320C64x_A14, TMS320C64x_A15,
	TMS320C64x_A16, TMS320C64x_A17, TMS320C64x_A18, TMS320C64x_A19,
	TMS320C64x_A20, TMS320C64x_A21, TMS320C64x_A22, TMS320C64x_A23,
	TMS320C64x_A24, TMS320C64x_A25, TMS320C64x_A26, TMS320C64x_A27,
	TMS320C64x_A28, TMS320C64x_A29, TMS320C64x_A30, TMS320C64x_A31
};

static const unsigned ControlRegsDecoderTable[] = {
	TMS320C64x_AMR,    TMS320C64x_CSR,  TMS320C64x_ISR,   TMS320C64x_ICR,
	TMS320C64x_IER,    TMS320C64x_ISTP, TMS320C64x_IRP,   TMS320C64x_NRP,
	~0U,               ~0U,             TMS320C64x_TSCL,  TMS320C64x_TSCH,
	~0U,               TMS320C64x_ILC,  TMS320C64x_RILC,  TMS320C64x_REP,
	TMS320C64x_PCE1,   TMS320C64x_DNUM, ~0U,              ~0U,
	~0U,               TMS320C64x_SSR,  TMS320C64x_GPLYA, TMS320C64x_GPLYB,
	TMS320C64x_GFPGFR, TMS320C64x_DIER, TMS320C64x_TSR,   TMS320C64x_ITSR,
	TMS320C64x_NTSR,   TMS320C64x_ECR,  ~0U,              TMS320C64x_IERR
};

static uint64_t getFeatureBits(int mode)
{
	// support everything
	return (uint64_t)-1;
}

static unsigned getReg(const unsigned *RegTable, unsigned RegNo)
{
	if(RegNo > 31)
		return ~0U;
	return RegTable[RegNo];
}

static DecodeStatus DecodeGPRegsRegisterClass(MCInst *Inst, unsigned RegNo,
		uint64_t Address, void *Decoder)
{
	unsigned Reg;

	if(RegNo > 31)
		return MCDisassembler_Fail;

	Reg = getReg(GPRegsDecoderTable, RegNo);
	if(Reg == ~0U)
		return MCDisassembler_Fail;
	MCOperand_CreateReg0(Inst, Reg);

	return MCDisassembler_Success;
}

static DecodeStatus DecodeControlRegsRegisterClass(MCInst *Inst, unsigned RegNo,
		uint64_t Address, void *Decoder)
{
	unsigned Reg;

	if(RegNo > 31)
		return MCDisassembler_Fail;

	Reg = getReg(ControlRegsDecoderTable, RegNo);
	if(Reg == ~0U)
		return MCDisassembler_Fail;
	MCOperand_CreateReg0(Inst, Reg);

	return MCDisassembler_Success;
}

static DecodeStatus DecodeScst5(MCInst *Inst, unsigned Val,
		uint64_t Address, void *Decoder)
{
	int32_t imm;

	imm = Val;
	/* Sign extend 5 bit value */
	if(imm & (1 << (5 - 1)))
		imm |= ~((1 << 5) - 1);

	MCOperand_CreateImm0(Inst, imm);

	return MCDisassembler_Success;
}

static DecodeStatus DecodeScst16(MCInst *Inst, unsigned Val,
		uint64_t Address, void *Decoder)
{
	int32_t imm;

	imm = Val;
	/* Sign extend 16 bit value */
	if(imm & (1 << (16 - 1)))
		imm |= ~((1 << 16) - 1);

	MCOperand_CreateImm0(Inst, imm);

	return MCDisassembler_Success;
}

static DecodeStatus DecodePCRelScst7(MCInst *Inst, unsigned Val,
		uint64_t Address, void *Decoder)
{
	int32_t imm;

	imm = Val;
	/* Sign extend 7 bit value */
	if(imm & (1 << (7 - 1)))
		imm |= ~((1 << 7) - 1);

	/* Address is relative to the address of the first instruction in the fetch packet */
	MCOperand_CreateImm0(Inst, (Address & ~31) + (imm * 4));

	return MCDisassembler_Success;
}

static DecodeStatus DecodePCRelScst10(MCInst *Inst, unsigned Val,
		uint64_t Address, void *Decoder)
{
	int32_t imm;

	imm = Val;
	/* Sign extend 10 bit value */
	if(imm & (1 << (10 - 1)))
		imm |= ~((1 << 10) - 1);

	/* Address is relative to the address of the first instruction in the fetch packet */
	MCOperand_CreateImm0(Inst, (Address & ~31) + (imm * 4));

	return MCDisassembler_Success;
}

static DecodeStatus DecodePCRelScst12(MCInst *Inst, unsigned Val,
		uint64_t Address, void *Decoder)
{
	int32_t imm;

	imm = Val;
	/* Sign extend 12 bit value */
	if(imm & (1 << (12 - 1)))
		imm |= ~((1 << 12) - 1);

	/* Address is relative to the address of the first instruction in the fetch packet */
	MCOperand_CreateImm0(Inst, (Address & ~31) + (imm * 4));

	return MCDisassembler_Success;
}

static DecodeStatus DecodePCRelScst21(MCInst *Inst, unsigned Val,
		uint64_t Address, void *Decoder)
{
	int32_t imm;

	imm = Val;
	/* Sign extend 21 bit value */
	if(imm & (1 << (21 - 1)))
		imm |= ~((1 << 21) - 1);

	/* Address is relative to the address of the first instruction in the fetch packet */
	MCOperand_CreateImm0(Inst, (Address & ~31) + (imm * 4));

	return MCDisassembler_Success;
}

static DecodeStatus DecodeMemOperand(MCInst *Inst, unsigned Val,
		uint64_t Address, void *Decoder)
{
	return DecodeMemOperandSc(Inst, Val | (1 << 15), Address, Decoder);
}

static DecodeStatus DecodeMemOperandSc(MCInst *Inst, unsigned Val,
		uint64_t Address, void *Decoder)
{
	uint8_t scaled, base, offset, mode, unit;
	unsigned basereg, offsetreg;

	scaled = (Val >> 15) & 1;
	base = (Val >> 10) & 0x1f;
	offset = (Val >> 5) & 0x1f;
	mode = (Val >> 1) & 0xf;
	unit = Val & 1;

	if((base >= TMS320C64X_REG_A0) && (base <= TMS320C64X_REG_A31))
		base = (base - TMS320C64X_REG_A0 + TMS320C64X_REG_B0);
	else if((base >= TMS320C64X_REG_B0) && (base <= TMS320C64X_REG_B31))
		base = (base - TMS320C64X_REG_B0 + TMS320C64X_REG_A0);
	basereg = getReg(GPRegsDecoderTable, base);
	if (basereg ==  ~0U)
		return MCDisassembler_Fail;

	switch(mode) {
		case 0:
		case 1:
		case 8:
		case 9:
		case 10:
		case 11:
			MCOperand_CreateImm0(Inst, (scaled << 19) | (basereg << 12) | (offset << 5) | (mode << 1) | unit);
			break;
		case 4:
		case 5:
		case 12:
		case 13:
		case 14:
		case 15:
			if((offset >= TMS320C64X_REG_A0) && (offset <= TMS320C64X_REG_A31))
				offset = (offset - TMS320C64X_REG_A0 + TMS320C64X_REG_B0);
			else if((offset >= TMS320C64X_REG_B0) && (offset <= TMS320C64X_REG_B31))
				offset = (offset - TMS320C64X_REG_B0 + TMS320C64X_REG_A0);
			offsetreg = getReg(GPRegsDecoderTable, offset);
			if (offsetreg ==  ~0U)
				return MCDisassembler_Fail;
			MCOperand_CreateImm0(Inst, (scaled << 19) | (basereg << 12) | (offsetreg << 5) | (mode << 1) | unit);
			break;
		default:
			return MCDisassembler_Fail;
	}

	return MCDisassembler_Success;
}

static DecodeStatus DecodeMemOperand2(MCInst *Inst, unsigned Val,
		uint64_t Address, void *Decoder)
{
	uint16_t offset;
	unsigned basereg;

	if(Val & 1)
		basereg = TMS320C64X_REG_B15;
	else
		basereg = TMS320C64X_REG_B14;

	offset = (Val >> 1) & 0x7fff;
	MCOperand_CreateImm0(Inst, (offset << 7) | basereg);

	return MCDisassembler_Success;
}

static DecodeStatus DecodeRegPair5(MCInst *Inst, unsigned RegNo,
		uint64_t Address, void *Decoder)
{
	unsigned Reg;

	if(RegNo > 31)
		return MCDisassembler_Fail;

	Reg = getReg(GPRegsDecoderTable, RegNo);
	MCOperand_CreateReg0(Inst, Reg);

	return MCDisassembler_Success;
}

static DecodeStatus DecodeRegPair4(MCInst *Inst, unsigned RegNo,
		uint64_t Address, void *Decoder)
{
	unsigned Reg;

	if(RegNo > 15)
		return MCDisassembler_Fail;

	Reg = getReg(GPRegsDecoderTable, RegNo << 1);
	MCOperand_CreateReg0(Inst, Reg);

	return MCDisassembler_Success;
}

static DecodeStatus DecodeCondRegister(MCInst *Inst, unsigned Val,
		uint64_t Address, void *Decoder)
{
	DecodeStatus ret = MCDisassembler_Success;

	if(!Inst->flat_insn->detail)
		return MCDisassembler_Success;

	switch(Val) {
		case 0:
		case 7:
			Inst->flat_insn->detail->tms320c64x.condition.reg = TMS320C64X_REG_INVALID;
			break;
		case 1:
			Inst->flat_insn->detail->tms320c64x.condition.reg = TMS320C64X_REG_B0;
			break;
		case 2:
			Inst->flat_insn->detail->tms320c64x.condition.reg = TMS320C64X_REG_B1;
			break;
		case 3:
			Inst->flat_insn->detail->tms320c64x.condition.reg = TMS320C64X_REG_B2;
			break;
		case 4:
			Inst->flat_insn->detail->tms320c64x.condition.reg = TMS320C64X_REG_A1;
			break;
		case 5:
			Inst->flat_insn->detail->tms320c64x.condition.reg = TMS320C64X_REG_A2;
			break;
		case 6:
			Inst->flat_insn->detail->tms320c64x.condition.reg = TMS320C64X_REG_A0;
			break;
		default:
			Inst->flat_insn->detail->tms320c64x.condition.reg = TMS320C64X_REG_INVALID;
			ret = MCDisassembler_Fail;
			break;
	}

	return ret;
}

static DecodeStatus DecodeCondRegisterZero(MCInst *Inst, unsigned Val,
		uint64_t Address, void *Decoder)
{
	DecodeStatus ret = MCDisassembler_Success;

	if(!Inst->flat_insn->detail)
		return MCDisassembler_Success;

	switch(Val) {
		case 0:
			Inst->flat_insn->detail->tms320c64x.condition.zero = 0;
			break;
		case 1:
			Inst->flat_insn->detail->tms320c64x.condition.zero = 1;
			break;
		default:
			Inst->flat_insn->detail->tms320c64x.condition.zero = 0;
			ret = MCDisassembler_Fail;
			break;
	}

	return ret;
}

static DecodeStatus DecodeSide(MCInst *Inst, unsigned Val,
		uint64_t Address, void *Decoder)
{
	DecodeStatus ret = MCDisassembler_Success;
	MCOperand *op;
	int i;

	/* This is pretty messy, probably we should find a better way */
	if(Val == 1) {
		for(i = 0; i < Inst->size; i++) {
			op = &Inst->Operands[i];
			if(op->Kind == kRegister) {
				if((op->RegVal >= TMS320C64X_REG_A0) && (op->RegVal <= TMS320C64X_REG_A31))
					op->RegVal = (op->RegVal - TMS320C64X_REG_A0 + TMS320C64X_REG_B0);
				else if((op->RegVal >= TMS320C64X_REG_B0) && (op->RegVal <= TMS320C64X_REG_B31))
					op->RegVal = (op->RegVal - TMS320C64X_REG_B0 + TMS320C64X_REG_A0);
			}
		}
	}

	if(!Inst->flat_insn->detail)
		return MCDisassembler_Success;

	switch(Val) {
		case 0:
			Inst->flat_insn->detail->tms320c64x.funit.side = 1;
			break;
		case 1:
			Inst->flat_insn->detail->tms320c64x.funit.side = 2;
			break;
		default:
			Inst->flat_insn->detail->tms320c64x.funit.side = 0;
			ret = MCDisassembler_Fail;
			break;
	}

	return ret;
}

static DecodeStatus DecodeParallel(MCInst *Inst, unsigned Val,
		uint64_t Address, void *Decoder)
{
	DecodeStatus ret = MCDisassembler_Success;

	if(!Inst->flat_insn->detail)
		return MCDisassembler_Success;

	switch(Val) {
		case 0:
			Inst->flat_insn->detail->tms320c64x.parallel = 0;
			break;
		case 1:
			Inst->flat_insn->detail->tms320c64x.parallel = 1;
			break;
		default:
			Inst->flat_insn->detail->tms320c64x.parallel = -1;
			ret = MCDisassembler_Fail;
			break;
	}

	return ret;
}

static DecodeStatus DecodeCrosspathX1(MCInst *Inst, unsigned Val,
		uint64_t Address, void *Decoder)
{
	DecodeStatus ret = MCDisassembler_Success;
	MCOperand *op;

	if(!Inst->flat_insn->detail)
		return MCDisassembler_Success;

	switch(Val) {
		case 0:
			Inst->flat_insn->detail->tms320c64x.funit.crosspath = 0;
			break;
		case 1:
			Inst->flat_insn->detail->tms320c64x.funit.crosspath = 1;
			op = &Inst->Operands[0];
			if(op->Kind == kRegister) {
				if((op->RegVal >= TMS320C64X_REG_A0) && (op->RegVal <= TMS320C64X_REG_A31))
					op->RegVal = (op->RegVal - TMS320C64X_REG_A0 + TMS320C64X_REG_B0);
				else if((op->RegVal >= TMS320C64X_REG_B0) && (op->RegVal <= TMS320C64X_REG_B31))
					op->RegVal = (op->RegVal - TMS320C64X_REG_B0 + TMS320C64X_REG_A0);
			}
			break;
		default:
			Inst->flat_insn->detail->tms320c64x.funit.crosspath = -1;
			ret = MCDisassembler_Fail;
			break;
	}

	return ret;
}

static DecodeStatus DecodeCrosspathX2(MCInst *Inst, unsigned Val,
		uint64_t Address, void *Decoder)
{
	DecodeStatus ret = MCDisassembler_Success;
	MCOperand *op;

	if(!Inst->flat_insn->detail)
		return MCDisassembler_Success;

	switch(Val) {
		case 0:
			Inst->flat_insn->detail->tms320c64x.funit.crosspath = 0;
			break;
		case 1:
			Inst->flat_insn->detail->tms320c64x.funit.crosspath = 1;
			op = &Inst->Operands[1];
			if(op->Kind == kRegister) {
				if((op->RegVal >= TMS320C64X_REG_A0) && (op->RegVal <= TMS320C64X_REG_A31))
					op->RegVal = (op->RegVal - TMS320C64X_REG_A0 + TMS320C64X_REG_B0);
				else if((op->RegVal >= TMS320C64X_REG_B0) && (op->RegVal <= TMS320C64X_REG_B31))
					op->RegVal = (op->RegVal - TMS320C64X_REG_B0 + TMS320C64X_REG_A0);
			}
			break;
		default:
			Inst->flat_insn->detail->tms320c64x.funit.crosspath = -1;
			ret = MCDisassembler_Fail;
			break;
	}

	return ret;
}

static DecodeStatus DecodeCrosspathX3(MCInst *Inst, unsigned Val,
		uint64_t Address, void *Decoder)
{
	DecodeStatus ret = MCDisassembler_Success;
	MCOperand *op;

	if(!Inst->flat_insn->detail)
		return MCDisassembler_Success;

	switch(Val) {
		case 0:
			Inst->flat_insn->detail->tms320c64x.funit.crosspath = 0;
			break;
		case 1:
			Inst->flat_insn->detail->tms320c64x.funit.crosspath = 2;
			op = &Inst->Operands[2];
			if(op->Kind == kRegister) {
				if((op->RegVal >= TMS320C64X_REG_A0) && (op->RegVal <= TMS320C64X_REG_A31))
					op->RegVal = (op->RegVal - TMS320C64X_REG_A0 + TMS320C64X_REG_B0);
				else if((op->RegVal >= TMS320C64X_REG_B0) && (op->RegVal <= TMS320C64X_REG_B31))
					op->RegVal = (op->RegVal - TMS320C64X_REG_B0 + TMS320C64X_REG_A0);
			}
			break;
		default:
			Inst->flat_insn->detail->tms320c64x.funit.crosspath = -1;
			ret = MCDisassembler_Fail;
			break;
	}

	return ret;
}


static DecodeStatus DecodeNop(MCInst *Inst, unsigned Val,
		uint64_t Address, void *Decoder)
{
	MCOperand_CreateImm0(Inst, Val + 1);

	return MCDisassembler_Success;
}

#define GET_INSTRINFO_ENUM
#include "TMS320C64xGenInstrInfo.inc"

bool TMS320C64x_getInstruction(csh ud, const uint8_t *code, size_t code_len,
		MCInst *MI, uint16_t *size, uint64_t address, void *info)
{
	uint32_t insn;
	DecodeStatus result;

	if(code_len < 4) {
		*size = 0;
		return MCDisassembler_Fail;
	}

	if(MI->flat_insn->detail)
		memset(MI->flat_insn->detail, 0, offsetof(cs_detail, tms320c64x)+sizeof(cs_tms320c64x));

	insn = (code[3] << 0) | (code[2] << 8) | (code[1] << 16) | ((uint32_t) code[0] << 24);
	result = decodeInstruction_4(DecoderTable32, MI, insn, address, info, 0);

	if(result == MCDisassembler_Success) {
		*size = 4;
		return true;
	}

	MCInst_clear(MI);
	*size = 0;
	return false;
}

void TMS320C64x_init(MCRegisterInfo *MRI)
{
	MCRegisterInfo_InitMCRegisterInfo(MRI, TMS320C64xRegDesc, 90,
			0, 0,
			TMS320C64xMCRegisterClasses, 7,
			0, 0,
			TMS320C64xRegDiffLists,
			0,
			TMS320C64xSubRegIdxLists, 1,
			0);
}

#endif
