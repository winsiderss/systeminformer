//===- AArch64AddressingModes.h - AArch64 Addressing Modes ------*- C++ -*-===//
//
//                     The LLVM Compiler Infrastructure
//
// This file is distributed under the University of Illinois Open Source
// License. See LICENSE.TXT for details.
//
//===----------------------------------------------------------------------===//
//
// This file contains the AArch64 addressing mode implementation stuff.
//
//===----------------------------------------------------------------------===//

#ifndef CS_AARCH64_ADDRESSINGMODES_H
#define CS_AARCH64_ADDRESSINGMODES_H

/* Capstone Disassembly Engine */
/* By Nguyen Anh Quynh <aquynh@gmail.com>, 2013-2015 */

#include "../../MathExtras.h"

/// AArch64_AM - AArch64 Addressing Mode Stuff

//===----------------------------------------------------------------------===//
// Shifts
//

typedef enum AArch64_AM_ShiftExtendType {
	AArch64_AM_InvalidShiftExtend = -1,
	AArch64_AM_LSL = 0,
	AArch64_AM_LSR,
	AArch64_AM_ASR,
	AArch64_AM_ROR,
	AArch64_AM_MSL,

	AArch64_AM_UXTB,
	AArch64_AM_UXTH,
	AArch64_AM_UXTW,
	AArch64_AM_UXTX,

	AArch64_AM_SXTB,
	AArch64_AM_SXTH,
	AArch64_AM_SXTW,
	AArch64_AM_SXTX,
} AArch64_AM_ShiftExtendType;

/// getShiftName - Get the string encoding for the shift type.
static inline const char *AArch64_AM_getShiftExtendName(AArch64_AM_ShiftExtendType ST)
{
	switch (ST) {
		default: return NULL; // never reach
		case AArch64_AM_LSL: return "lsl";
		case AArch64_AM_LSR: return "lsr";
		case AArch64_AM_ASR: return "asr";
		case AArch64_AM_ROR: return "ror";
		case AArch64_AM_MSL: return "msl";
		case AArch64_AM_UXTB: return "uxtb";
		case AArch64_AM_UXTH: return "uxth";
		case AArch64_AM_UXTW: return "uxtw";
		case AArch64_AM_UXTX: return "uxtx";
		case AArch64_AM_SXTB: return "sxtb";
		case AArch64_AM_SXTH: return "sxth";
		case AArch64_AM_SXTW: return "sxtw";
		case AArch64_AM_SXTX: return "sxtx";
	}
}

/// getShiftType - Extract the shift type.
static inline AArch64_AM_ShiftExtendType AArch64_AM_getShiftType(unsigned Imm)
{
	switch ((Imm >> 6) & 0x7) {
		default: return AArch64_AM_InvalidShiftExtend;
		case 0: return AArch64_AM_LSL;
		case 1: return AArch64_AM_LSR;
		case 2: return AArch64_AM_ASR;
		case 3: return AArch64_AM_ROR;
		case 4: return AArch64_AM_MSL;
	}
}

/// getShiftValue - Extract the shift value.
static inline unsigned AArch64_AM_getShiftValue(unsigned Imm)
{
	return Imm & 0x3f;
}

//===----------------------------------------------------------------------===//
// Extends
//

/// getArithShiftValue - get the arithmetic shift value.
static inline unsigned AArch64_AM_getArithShiftValue(unsigned Imm)
{
	return Imm & 0x7;
}

/// getExtendType - Extract the extend type for operands of arithmetic ops.
static inline AArch64_AM_ShiftExtendType AArch64_AM_getExtendType(unsigned Imm)
{
	// assert((Imm & 0x7) == Imm && "invalid immediate!");
	switch (Imm) {
		default: // llvm_unreachable("Compiler bug!");
		case 0: return AArch64_AM_UXTB;
		case 1: return AArch64_AM_UXTH;
		case 2: return AArch64_AM_UXTW;
		case 3: return AArch64_AM_UXTX;
		case 4: return AArch64_AM_SXTB;
		case 5: return AArch64_AM_SXTH;
		case 6: return AArch64_AM_SXTW;
		case 7: return AArch64_AM_SXTX;
	}
}

static inline AArch64_AM_ShiftExtendType AArch64_AM_getArithExtendType(unsigned Imm)
{
	return AArch64_AM_getExtendType((Imm >> 3) & 0x7);
}

static inline uint64_t ror(uint64_t elt, unsigned size)
{
	return ((elt & 1) << (size-1)) | (elt >> 1);
}

/// decodeLogicalImmediate - Decode a logical immediate value in the form
/// "N:immr:imms" (where the immr and imms fields are each 6 bits) into the
/// integer value it represents with regSize bits.
static inline uint64_t AArch64_AM_decodeLogicalImmediate(uint64_t val, unsigned regSize)
{
	// Extract the N, imms, and immr fields.
	unsigned N = (val >> 12) & 1;
	unsigned immr = (val >> 6) & 0x3f;
	unsigned imms = val & 0x3f;
	unsigned i;

	// assert((regSize == 64 || N == 0) && "undefined logical immediate encoding");
	int len = 31 - countLeadingZeros((N << 6) | (~imms & 0x3f));
	// assert(len >= 0 && "undefined logical immediate encoding");
	unsigned size = (1 << len);
	unsigned R = immr & (size - 1);
	unsigned S = imms & (size - 1);
	// assert(S != size - 1 && "undefined logical immediate encoding");
	uint64_t pattern = (1ULL << (S + 1)) - 1;
	for (i = 0; i < R; ++i)
		pattern = ror(pattern, size);

	// Replicate the pattern to fill the regSize.
	while (size != regSize) {
		pattern |= (pattern << size);
		size *= 2;
	}

	return pattern;
}

/// isValidDecodeLogicalImmediate - Check to see if the logical immediate value
/// in the form "N:immr:imms" (where the immr and imms fields are each 6 bits)
/// is a valid encoding for an integer value with regSize bits.
static inline bool AArch64_AM_isValidDecodeLogicalImmediate(uint64_t val, unsigned regSize)
{
	unsigned size;
	unsigned S;
	int len;
	// Extract the N and imms fields needed for checking.
	unsigned N = (val >> 12) & 1;
	unsigned imms = val & 0x3f;

	if (regSize == 32 && N != 0) // undefined logical immediate encoding
		return false;
	len = 31 - countLeadingZeros((N << 6) | (~imms & 0x3f));
	if (len < 0) // undefined logical immediate encoding
		return false;
	size = (1 << len);
	S = imms & (size - 1);
	if (S == size - 1) // undefined logical immediate encoding
		return false;

	return true;
}

//===----------------------------------------------------------------------===//
// Floating-point Immediates
//
static inline float AArch64_AM_getFPImmFloat(unsigned Imm)
{
	// We expect an 8-bit binary encoding of a floating-point number here.
	union {
		uint32_t I;
		float F;
	} FPUnion;

	uint8_t Sign = (Imm >> 7) & 0x1;
	uint8_t Exp = (Imm >> 4) & 0x7;
	uint8_t Mantissa = Imm & 0xf;

	//   8-bit FP    iEEEE Float Encoding
	//   abcd efgh   aBbbbbbc defgh000 00000000 00000000
	//
	// where B = NOT(b);

	FPUnion.I = 0;
	FPUnion.I |= ((uint32_t)Sign) << 31;
	FPUnion.I |= ((Exp & 0x4) != 0 ? 0 : 1) << 30;
	FPUnion.I |= ((Exp & 0x4) != 0 ? 0x1f : 0) << 25;
	FPUnion.I |= (Exp & 0x3) << 23;
	FPUnion.I |= Mantissa << 19;

	return FPUnion.F;
}

//===--------------------------------------------------------------------===//
// AdvSIMD Modified Immediates
//===--------------------------------------------------------------------===//

static inline uint64_t AArch64_AM_decodeAdvSIMDModImmType10(uint8_t Imm)
{
	static const uint32_t lookup[16] = {
		0x00000000, 0x000000ff, 0x0000ff00, 0x0000ffff, 
		0x00ff0000, 0x00ff00ff, 0x00ffff00, 0x00ffffff, 
		0xff000000, 0xff0000ff, 0xff00ff00, 0xff00ffff, 
		0xffff0000, 0xffff00ff, 0xffffff00, 0xffffffff
        };
	return lookup[Imm & 0x0f] | ((uint64_t)lookup[Imm >> 4] << 32);
}

#endif
