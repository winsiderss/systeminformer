//===-- SystemZMCTargetDesc.cpp - SystemZ target descriptions -------------===//
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

#include "SystemZMCTargetDesc.h"

#define GET_REGINFO_ENUM
#include "SystemZGenRegisterInfo.inc"

const unsigned SystemZMC_GR32Regs[16] = {
	SystemZ_R0L, SystemZ_R1L, SystemZ_R2L, SystemZ_R3L,
	SystemZ_R4L, SystemZ_R5L, SystemZ_R6L, SystemZ_R7L,
	SystemZ_R8L, SystemZ_R9L, SystemZ_R10L, SystemZ_R11L,
	SystemZ_R12L, SystemZ_R13L, SystemZ_R14L, SystemZ_R15L
};

const unsigned SystemZMC_GRH32Regs[16] = {
	SystemZ_R0H, SystemZ_R1H, SystemZ_R2H, SystemZ_R3H,
	SystemZ_R4H, SystemZ_R5H, SystemZ_R6H, SystemZ_R7H,
	SystemZ_R8H, SystemZ_R9H, SystemZ_R10H, SystemZ_R11H,
	SystemZ_R12H, SystemZ_R13H, SystemZ_R14H, SystemZ_R15H
};

const unsigned SystemZMC_GR64Regs[16] = {
	SystemZ_R0D, SystemZ_R1D, SystemZ_R2D, SystemZ_R3D,
	SystemZ_R4D, SystemZ_R5D, SystemZ_R6D, SystemZ_R7D,
	SystemZ_R8D, SystemZ_R9D, SystemZ_R10D, SystemZ_R11D,
	SystemZ_R12D, SystemZ_R13D, SystemZ_R14D, SystemZ_R15D
};

const unsigned SystemZMC_GR128Regs[16] = {
	SystemZ_R0Q, 0, SystemZ_R2Q, 0,
	SystemZ_R4Q, 0, SystemZ_R6Q, 0,
	SystemZ_R8Q, 0, SystemZ_R10Q, 0,
	SystemZ_R12Q, 0, SystemZ_R14Q, 0
};

const unsigned SystemZMC_FP32Regs[16] = {
	SystemZ_F0S, SystemZ_F1S, SystemZ_F2S, SystemZ_F3S,
	SystemZ_F4S, SystemZ_F5S, SystemZ_F6S, SystemZ_F7S,
	SystemZ_F8S, SystemZ_F9S, SystemZ_F10S, SystemZ_F11S,
	SystemZ_F12S, SystemZ_F13S, SystemZ_F14S, SystemZ_F15S
};

const unsigned SystemZMC_FP64Regs[16] = {
	SystemZ_F0D, SystemZ_F1D, SystemZ_F2D, SystemZ_F3D,
	SystemZ_F4D, SystemZ_F5D, SystemZ_F6D, SystemZ_F7D,
	SystemZ_F8D, SystemZ_F9D, SystemZ_F10D, SystemZ_F11D,
	SystemZ_F12D, SystemZ_F13D, SystemZ_F14D, SystemZ_F15D
};

const unsigned SystemZMC_FP128Regs[16] = {
	SystemZ_F0Q, SystemZ_F1Q, 0, 0,
	SystemZ_F4Q, SystemZ_F5Q, 0, 0,
	SystemZ_F8Q, SystemZ_F9Q, 0, 0,
	SystemZ_F12Q, SystemZ_F13Q, 0, 0
};

unsigned SystemZMC_getFirstReg(unsigned Reg)
{
	static unsigned Map[SystemZ_NUM_TARGET_REGS];
	static int Initialized = 0;
	unsigned I;

	if (!Initialized) {
		Initialized = 1;
		for (I = 0; I < 16; ++I) {
			Map[SystemZMC_GR32Regs[I]] = I;
			Map[SystemZMC_GRH32Regs[I]] = I;
			Map[SystemZMC_GR64Regs[I]] = I;
			Map[SystemZMC_GR128Regs[I]] = I;
			Map[SystemZMC_FP32Regs[I]] = I;
			Map[SystemZMC_FP64Regs[I]] = I;
			Map[SystemZMC_FP128Regs[I]] = I;
		}
	}

	// assert(Reg < SystemZ_NUM_TARGET_REGS);
	return Map[Reg];
}

#endif
