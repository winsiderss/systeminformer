//===-- X86BaseInfo.h - Top level definitions for X86 -------- --*- C++ -*-===//
//
//                     The LLVM Compiler Infrastructure
//
// This file is distributed under the University of Illinois Open Source
// License. See LICENSE.TXT for details.
//
//===----------------------------------------------------------------------===//
//
// This file contains small standalone helper functions and enum definitions for
// the X86 target useful for the compiler back-end and the MC libraries.
// As such, it deliberately does not include references to LLVM core
// code gen types, passes, etc..
//
//===----------------------------------------------------------------------===//

#ifndef CS_X86_BASEINFO_H
#define CS_X86_BASEINFO_H

/* Capstone Disassembly Engine */
/* By Nguyen Anh Quynh <aquynh@gmail.com>, 2013-2015 */

// Enums for memory operand decoding.  Each memory operand is represented with
// a 5 operand sequence in the form:
//   [BaseReg, ScaleAmt, IndexReg, Disp, Segment]
// These enums help decode this.
enum {
	X86_AddrBaseReg = 0,
	X86_AddrScaleAmt = 1,
	X86_AddrIndexReg = 2,
	X86_AddrDisp = 3,

	/// AddrSegmentReg - The operand # of the segment in the memory operand.
	X86_AddrSegmentReg = 4,

	/// AddrNumOperands - Total number of operands in a memory reference.
	X86_AddrNumOperands = 5
};

#endif
