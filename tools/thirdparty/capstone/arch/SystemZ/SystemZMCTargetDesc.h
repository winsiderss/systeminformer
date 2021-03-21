//===-- SystemZMCTargetDesc.h - SystemZ target descriptions -----*- C++ -*-===//
//
//                     The LLVM Compiler Infrastructure
//
// This file is distributed under the University of Illinois Open Source
// License. See LICENSE.TXT for details.
//
//===----------------------------------------------------------------------===//

/* Capstone Disassembly Engine */
/* By Nguyen Anh Quynh <aquynh@gmail.com>, 2013-2015 */

#ifndef CS_SYSTEMZMCTARGETDESC_H
#define CS_SYSTEMZMCTARGETDESC_H

// Maps of asm register numbers to LLVM register numbers, with 0 indicating
// an invalid register.  In principle we could use 32-bit and 64-bit register
// classes directly, provided that we relegated the GPR allocation order
// in SystemZRegisterInfo.td to an AltOrder and left the default order
// as %r0-%r15.  It seems better to provide the same interface for
// all classes though.
extern const unsigned SystemZMC_GR32Regs[16];
extern const unsigned SystemZMC_GRH32Regs[16];
extern const unsigned SystemZMC_GR64Regs[16];
extern const unsigned SystemZMC_GR128Regs[16];
extern const unsigned SystemZMC_FP32Regs[16];
extern const unsigned SystemZMC_FP64Regs[16];
extern const unsigned SystemZMC_FP128Regs[16];

// Return the 0-based number of the first architectural register that
// contains the given LLVM register.   E.g. R1D -> 1.
unsigned SystemZMC_getFirstReg(unsigned Reg);

// Defines symbolic names for SystemZ registers.
// This defines a mapping from register name to register number.
//#define GET_REGINFO_ENUM
//#include "SystemZGenRegisterInfo.inc"

// Defines symbolic names for the SystemZ instructions.
//#define GET_INSTRINFO_ENUM
//#include "SystemZGenInstrInfo.inc"

//#define GET_SUBTARGETINFO_ENUM
//#include "SystemZGenSubtargetInfo.inc"

#endif
