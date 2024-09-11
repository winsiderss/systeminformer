//===-- PPCPredicates.h - PPC Branch Predicate Information ------*- C++ -*-===//
//
//                     The LLVM Compiler Infrastructure
//
// This file is distributed under the University of Illinois Open Source
// License. See LICENSE.TXT for details.
//
//===----------------------------------------------------------------------===//
//
// This file describes the PowerPC branch predicates.
//
//===----------------------------------------------------------------------===//

/* Capstone Disassembly Engine */
/* By Nguyen Anh Quynh <aquynh@gmail.com>, 2013-2015 */

#ifndef CS_POWERPC_PPCPREDICATES_H
#define CS_POWERPC_PPCPREDICATES_H

#include "capstone/ppc.h"

// NOTE: duplicate of ppc_bc in ppc.h to maitain code compatibility with LLVM
typedef enum ppc_predicate {
  PPC_PRED_LT       = (0 << 5) | 12,
  PPC_PRED_LE       = (1 << 5) |  4,
  PPC_PRED_EQ       = (2 << 5) | 12,
  PPC_PRED_GE       = (0 << 5) |  4,
  PPC_PRED_GT       = (1 << 5) | 12,
  PPC_PRED_NE       = (2 << 5) |  4,
  PPC_PRED_UN       = (3 << 5) | 12,
  PPC_PRED_NU       = (3 << 5) |  4,
  PPC_PRED_LT_MINUS = (0 << 5) | 14,
  PPC_PRED_LE_MINUS = (1 << 5) |  6,
  PPC_PRED_EQ_MINUS = (2 << 5) | 14,
  PPC_PRED_GE_MINUS = (0 << 5) |  6,
  PPC_PRED_GT_MINUS = (1 << 5) | 14,
  PPC_PRED_NE_MINUS = (2 << 5) |  6,
  PPC_PRED_UN_MINUS = (3 << 5) | 14,
  PPC_PRED_NU_MINUS = (3 << 5) |  6,
  PPC_PRED_LT_PLUS  = (0 << 5) | 15,
  PPC_PRED_LE_PLUS  = (1 << 5) |  7,
  PPC_PRED_EQ_PLUS  = (2 << 5) | 15,
  PPC_PRED_GE_PLUS  = (0 << 5) |  7,
  PPC_PRED_GT_PLUS  = (1 << 5) | 15,
  PPC_PRED_NE_PLUS  = (2 << 5) |  7,
  PPC_PRED_UN_PLUS  = (3 << 5) | 15,
  PPC_PRED_NU_PLUS  = (3 << 5) |  7,

  // When dealing with individual condition-register bits, we have simple set
  // and unset predicates.
  PPC_PRED_BIT_SET =   1024,
  PPC_PRED_BIT_UNSET = 1025
} ppc_predicate;

/// Invert the specified predicate.  != -> ==, < -> >=.
ppc_predicate InvertPredicate(ppc_predicate Opcode);

/// Assume the condition register is set by MI(a,b), return the predicate if
/// we modify the instructions such that condition register is set by MI(b,a).
ppc_predicate getSwappedPredicate(ppc_predicate Opcode);

#endif
