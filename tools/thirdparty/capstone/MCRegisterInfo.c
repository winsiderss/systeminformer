//=== MC/MCRegisterInfo.cpp - Target Register Description -------*- C++ -*-===//
//
//                     The LLVM Compiler Infrastructure
//
// This file is distributed under the University of Illinois Open Source
// License. See LICENSE.TXT for details.
//
//===----------------------------------------------------------------------===//
//
// This file implements MCRegisterInfo functions.
//
//===----------------------------------------------------------------------===//

/* Capstone Disassembly Engine */
/* By Nguyen Anh Quynh <aquynh@gmail.com>, 2013-2015 */

#include "MCRegisterInfo.h"

/// DiffListIterator - Base iterator class that can traverse the
/// differentially encoded register and regunit lists in DiffLists.
/// Don't use this class directly, use one of the specialized sub-classes
/// defined below.
typedef struct DiffListIterator {
	uint16_t Val;
	const MCPhysReg *List;
} DiffListIterator;

void MCRegisterInfo_InitMCRegisterInfo(MCRegisterInfo *RI,
		const MCRegisterDesc *D, unsigned NR,
		unsigned RA, unsigned PC,
		const MCRegisterClass *C, unsigned NC,
		uint16_t (*RURoots)[2], unsigned NRU,
		const MCPhysReg *DL,
		const char *Strings,
		const uint16_t *SubIndices, unsigned NumIndices,
		const uint16_t *RET)
{
	RI->Desc = D;
	RI->NumRegs = NR;
	RI->RAReg = RA;
	RI->PCReg = PC;
	RI->Classes = C;
	RI->DiffLists = DL;
	RI->RegStrings = Strings;
	RI->NumClasses = NC;
	RI->RegUnitRoots = RURoots;
	RI->NumRegUnits = NRU;
	RI->SubRegIndices = SubIndices;
	RI->NumSubRegIndices = NumIndices;
	RI->RegEncodingTable = RET;
}

static void DiffListIterator_init(DiffListIterator *d, MCPhysReg InitVal, const MCPhysReg *DiffList)
{
	d->Val = InitVal;
	d->List = DiffList;
}

static uint16_t DiffListIterator_getVal(DiffListIterator *d)
{
	return d->Val;
}

static bool DiffListIterator_next(DiffListIterator *d)
{
	MCPhysReg D;

	if (d->List == 0)
		return false;

	D = *d->List;
	d->List++;
	d->Val += D;

	if (!D)
		d->List = 0;

	return (D != 0);
}

static bool DiffListIterator_isValid(DiffListIterator *d)
{
	return (d->List != 0);
}

unsigned MCRegisterInfo_getMatchingSuperReg(const MCRegisterInfo *RI, unsigned Reg, unsigned SubIdx, const MCRegisterClass *RC)
{
	DiffListIterator iter;

	if (Reg >= RI->NumRegs) {
		return 0;
	}

	DiffListIterator_init(&iter, (MCPhysReg)Reg, RI->DiffLists + RI->Desc[Reg].SuperRegs);
	DiffListIterator_next(&iter);

	while(DiffListIterator_isValid(&iter)) {
		uint16_t val = DiffListIterator_getVal(&iter);
		if (MCRegisterClass_contains(RC, val) && Reg ==  MCRegisterInfo_getSubReg(RI, val, SubIdx))
			return val;

		DiffListIterator_next(&iter);
	}

	return 0;
}

unsigned MCRegisterInfo_getSubReg(const MCRegisterInfo *RI, unsigned Reg, unsigned Idx)
{
	DiffListIterator iter;
	const uint16_t *SRI = RI->SubRegIndices + RI->Desc[Reg].SubRegIndices;

	DiffListIterator_init(&iter, (MCPhysReg)Reg, RI->DiffLists + RI->Desc[Reg].SubRegs);
	DiffListIterator_next(&iter);

	while(DiffListIterator_isValid(&iter)) {
		if (*SRI == Idx)
			return DiffListIterator_getVal(&iter);
		DiffListIterator_next(&iter);
		++SRI;
	}

	return 0;
}

const MCRegisterClass* MCRegisterInfo_getRegClass(const MCRegisterInfo *RI, unsigned i)
{
	//assert(i < getNumRegClasses() && "Register Class ID out of range");
	if (i >= RI->NumClasses)
		return 0;
	return &(RI->Classes[i]);
}

bool MCRegisterClass_contains(const MCRegisterClass *c, unsigned Reg)
{
	unsigned InByte = Reg % 8;
	unsigned Byte = Reg / 8;

	if (Byte >= c->RegSetSize)
		return false;

	return (c->RegSet[Byte] & (1 << InByte)) != 0;
}
