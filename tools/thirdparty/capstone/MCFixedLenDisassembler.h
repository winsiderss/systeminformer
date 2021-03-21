//===-- llvm/MC/MCFixedLenDisassembler.h - Decoder driver -------*- C++ -*-===//
//
//                     The LLVM Compiler Infrastructure
//
// This file is distributed under the University of Illinois Open Source
// License. See LICENSE.TXT for details.
//
//===----------------------------------------------------------------------===//
// Fixed length disassembler decoder state machine driver.
//===----------------------------------------------------------------------===//

/* Capstone Disassembly Engine */
/* By Nguyen Anh Quynh <aquynh@gmail.com>, 2013-2015 */

#ifndef CS_LLVM_MC_MCFIXEDLENDISASSEMBLER_H
#define CS_LLVM_MC_MCFIXEDLENDISASSEMBLER_H

// Disassembler state machine opcodes.
enum DecoderOps {
	MCD_OPC_ExtractField = 1, // OPC_ExtractField(uint8_t Start, uint8_t Len)
	MCD_OPC_FilterValue,      // OPC_FilterValue(uleb128 Val, uint16_t NumToSkip)
	MCD_OPC_CheckField,       // OPC_CheckField(uint8_t Start, uint8_t Len,
							  //                uleb128 Val, uint16_t NumToSkip)
	MCD_OPC_CheckPredicate,   // OPC_CheckPredicate(uleb128 PIdx, uint16_t NumToSkip)
	MCD_OPC_Decode,           // OPC_Decode(uleb128 Opcode, uleb128 DIdx)
	MCD_OPC_SoftFail,         // OPC_SoftFail(uleb128 PMask, uleb128 NMask)
	MCD_OPC_Fail              // OPC_Fail()
};

#endif
