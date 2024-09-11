/* Capstone Disassembly Engine */
/* By Nguyen Anh Quynh <aquynh@gmail.com>, 2013-2015 */

#if defined(CAPSTONE_HAS_OSXKERNEL)
#include <Availability.h>
#include <libkern/libkern.h>
#else
#include <stdio.h>
#include <stdlib.h>
#endif
#include <string.h>

#include "MCInst.h"
#include "utils.h"

#define MCINST_CACHE (ARR_SIZE(mcInst->Operands) - 1)

void MCInst_Init(MCInst *inst)
{
	unsigned int i;

	for (i = 0; i < 48; i++) {
		inst->Operands[i].Kind = kInvalid;
	}

	inst->Opcode = 0;
	inst->OpcodePub = 0;
	inst->size = 0;
	inst->has_imm = false;
	inst->op1_size = 0;
	inst->writeback = false;
	inst->ac_idx = 0;
	inst->popcode_adjust = 0;
	inst->assembly[0] = '\0';
	inst->xAcquireRelease = 0;
}

void MCInst_clear(MCInst *inst)
{
	inst->size = 0;
}

// do not free @Op
void MCInst_insert0(MCInst *inst, int index, MCOperand *Op)
{
	int i;

	for(i = inst->size; i > index; i--)
		//memcpy(&(inst->Operands[i]), &(inst->Operands[i-1]), sizeof(MCOperand));
		inst->Operands[i] = inst->Operands[i-1];

	inst->Operands[index] = *Op;
	inst->size++;
}

void MCInst_setOpcode(MCInst *inst, unsigned Op)
{
	inst->Opcode = Op;
}

void MCInst_setOpcodePub(MCInst *inst, unsigned Op)
{
	inst->OpcodePub = Op;
}

unsigned MCInst_getOpcode(const MCInst *inst)
{
	return inst->Opcode;
}

unsigned MCInst_getOpcodePub(const MCInst *inst)
{
	return inst->OpcodePub;
}

MCOperand *MCInst_getOperand(MCInst *inst, unsigned i)
{
	return &inst->Operands[i];
}

unsigned MCInst_getNumOperands(const MCInst *inst)
{
	return inst->size;
}

// This addOperand2 function doesnt free Op
void MCInst_addOperand2(MCInst *inst, MCOperand *Op)
{
	inst->Operands[inst->size] = *Op;

	inst->size++;
}

bool MCOperand_isValid(const MCOperand *op)
{
	return op->Kind != kInvalid;
}

bool MCOperand_isReg(const MCOperand *op)
{
	return op->Kind == kRegister;
}

bool MCOperand_isImm(const MCOperand *op)
{
	return op->Kind == kImmediate;
}

bool MCOperand_isFPImm(const MCOperand *op)
{
	return op->Kind == kFPImmediate;
}

/// getReg - Returns the register number.
unsigned MCOperand_getReg(const MCOperand *op)
{
	return op->RegVal;
}

/// setReg - Set the register number.
void MCOperand_setReg(MCOperand *op, unsigned Reg)
{
	op->RegVal = Reg;
}

int64_t MCOperand_getImm(MCOperand *op)
{
	return op->ImmVal;
}

void MCOperand_setImm(MCOperand *op, int64_t Val)
{
	op->ImmVal = Val;
}

double MCOperand_getFPImm(const MCOperand *op)
{
	return op->FPImmVal;
}

void MCOperand_setFPImm(MCOperand *op, double Val)
{
	op->FPImmVal = Val;
}

MCOperand *MCOperand_CreateReg1(MCInst *mcInst, unsigned Reg)
{
	MCOperand *op = &(mcInst->Operands[MCINST_CACHE]);

	op->Kind = kRegister;
	op->RegVal = Reg;

	return op;
}

void MCOperand_CreateReg0(MCInst *mcInst, unsigned Reg)
{
	MCOperand *op = &(mcInst->Operands[mcInst->size]);
	mcInst->size++;

	op->Kind = kRegister;
	op->RegVal = Reg;
}

MCOperand *MCOperand_CreateImm1(MCInst *mcInst, int64_t Val)
{
	MCOperand *op = &(mcInst->Operands[MCINST_CACHE]);

	op->Kind = kImmediate;
	op->ImmVal = Val;

	return op;
}

void MCOperand_CreateImm0(MCInst *mcInst, int64_t Val)
{
	MCOperand *op = &(mcInst->Operands[mcInst->size]);
	mcInst->size++;

	op->Kind = kImmediate;
	op->ImmVal = Val;
}
