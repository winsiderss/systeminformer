//===-- Sparc.h - Top-level interface for Sparc representation --*- C++ -*-===//
//
//                     The LLVM Compiler Infrastructure
//
// This file is distributed under the University of Illinois Open Source
// License. See LICENSE.TXT for details.
//
//===----------------------------------------------------------------------===//
//
// This file contains the entry points for global functions defined in the LLVM
// Sparc back-end.
//
//===----------------------------------------------------------------------===//

/* Capstone Disassembly Engine */
/* By Nguyen Anh Quynh <aquynh@gmail.com>, 2013-2015 */

#ifndef CS_SPARC_TARGET_SPARC_H
#define CS_SPARC_TARGET_SPARC_H

#include "capstone/sparc.h"

inline static const char *SPARCCondCodeToString(sparc_cc CC)
{
	switch (CC) {
		default:	return NULL;	// unreachable
		case SPARC_CC_ICC_A:  return "a";
		case SPARC_CC_ICC_N:   return "n";
		case SPARC_CC_ICC_NE:  return "ne";
		case SPARC_CC_ICC_E:   return "e";
		case SPARC_CC_ICC_G:   return "g";
		case SPARC_CC_ICC_LE:  return "le";
		case SPARC_CC_ICC_GE:  return "ge";
		case SPARC_CC_ICC_L:   return "l";
		case SPARC_CC_ICC_GU:  return "gu";
		case SPARC_CC_ICC_LEU: return "leu";
		case SPARC_CC_ICC_CC:  return "cc";
		case SPARC_CC_ICC_CS:  return "cs";
		case SPARC_CC_ICC_POS: return "pos";
		case SPARC_CC_ICC_NEG: return "neg";
		case SPARC_CC_ICC_VC:  return "vc";
		case SPARC_CC_ICC_VS:  return "vs";

		case SPARC_CC_FCC_A:   return "a";
		case SPARC_CC_FCC_N:   return "n";
		case SPARC_CC_FCC_U:   return "u";
		case SPARC_CC_FCC_G:   return "g";
		case SPARC_CC_FCC_UG:  return "ug";
		case SPARC_CC_FCC_L:   return "l";
		case SPARC_CC_FCC_UL:  return "ul";
		case SPARC_CC_FCC_LG:  return "lg";
		case SPARC_CC_FCC_NE:  return "ne";
		case SPARC_CC_FCC_E:   return "e";
		case SPARC_CC_FCC_UE:  return "ue";
		case SPARC_CC_FCC_GE:  return "ge";
		case SPARC_CC_FCC_UGE: return "uge";
		case SPARC_CC_FCC_LE:  return "le";
		case SPARC_CC_FCC_ULE: return "ule";
		case SPARC_CC_FCC_O:   return "o";
	}
}

#endif
